#include "cursive0/02_syntax/lexer.h"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <vector>

#include "cursive0/00_core/assert_spec.h"
#include "cursive0/00_core/diagnostic_messages.h"
#include "cursive0/00_core/source_text.h"
#include "cursive0/00_core/span.h"

namespace cursive0::syntax {

namespace {

bool Match2(const std::vector<core::UnicodeScalar>& scalars,
            std::size_t i,
            core::UnicodeScalar a,
            core::UnicodeScalar b) {
  return i + 1 < scalars.size() && scalars[i] == a && scalars[i + 1] == b;
}

std::optional<DocKind> DocMarker(const std::vector<core::UnicodeScalar>& scalars,
                                 std::size_t i) {
  if (i + 2 >= scalars.size()) {
    return std::nullopt;
  }
  if (scalars[i] == '/' && scalars[i + 1] == '/' && scalars[i + 2] == '/') {
    return DocKind::LineDoc;
  }
  if (scalars[i] == '/' && scalars[i + 1] == '/' && scalars[i + 2] == '!') {
    return DocKind::ModuleDoc;
  }
  return std::nullopt;
}

std::string DocBody(const std::vector<core::UnicodeScalar>& scalars,
                    std::size_t i,
                    std::size_t j) {
  if (j <= i + 3 || i + 3 > scalars.size()) {
    return std::string();
  }
  const std::size_t end = std::min(j, scalars.size());
  std::vector<core::UnicodeScalar> body;
  body.reserve(end - (i + 3));
  body.insert(body.end(), scalars.begin() + i + 3, scalars.begin() + end);
  if (!body.empty() && body.front() == 0x20) {
    body.erase(body.begin());
  }
  return core::EncodeUtf8(body);
}

core::Span SpanOfText(const core::SourceFile& source,
                      const std::vector<core::UnicodeScalar>& scalars,
                      std::size_t i,
                      std::size_t j) {
  const auto offsets = core::Utf8Offsets(scalars);
  const std::size_t start = offsets[i];
  const std::size_t end = offsets[j];
  return core::SpanOf(source, start, end);
}

}  // namespace

bool IsWhitespace(core::UnicodeScalar c) {
  return c == 0x20 || c == 0x09 || c == 0x0C;
}

bool IsLineFeed(core::UnicodeScalar c) {
  return c == core::kLF;
}

CommentScanResult ScanLineComment(const core::SourceFile& source,
                                  std::size_t start) {
  SPEC_RULE("Scan-Line-Comment");
  CommentScanResult result;
  const auto& scalars = source.scalars;
  const std::size_t n = scalars.size();
  if (!Match2(scalars, start, '/', '/')) {
    result.ok = false;
    result.next = start;
    result.range = ScalarRange{start, start};
    return result;
  }
  std::size_t j = n;
  for (std::size_t p = start; p < n; ++p) {
    if (scalars[p] == core::kLF) {
      j = p;
      break;
    }
  }
  result.ok = true;
  result.next = j;
  result.range = ScalarRange{start, j};
  return result;
}

CommentScanResult ScanDocComment(const core::SourceFile& source,
                                 std::size_t start) {
  SPEC_RULE("Doc-Comment");
  CommentScanResult result = ScanLineComment(source, start);
  const auto& scalars = source.scalars;
  const auto kind = DocMarker(scalars, start);
  if (!kind.has_value()) {
    result.ok = false;
    return result;
  }
  DocComment doc;
  doc.kind = *kind;
  doc.text = DocBody(scalars, start, result.next);
  doc.span = SpanOfText(source, scalars, start, result.next);
  result.doc = doc;
  return result;
}

CommentScanResult ScanBlockComment(const core::SourceFile& source,
                                   std::size_t start) {
  SPEC_RULE("Block-Start");
  SPEC_RULE("Block-End");
  SPEC_RULE("Block-Done");
  SPEC_RULE("Block-Step");
  CommentScanResult result;
  const auto& scalars = source.scalars;
  const std::size_t n = scalars.size();
  std::size_t i = start;
  std::size_t depth = 0;

  if (!Match2(scalars, i, '/', '*')) {
    result.ok = false;
    return result;
  }

  depth = 1;
  i += 2;

  while (i < n) {
    if (Match2(scalars, i, '/', '*')) {
      ++depth;
      i += 2;
      continue;
    }
    if (Match2(scalars, i, '*', '/')) {
      if (depth == 1) {
        i += 2;
        result.ok = true;
        result.next = i;
        result.range = ScalarRange{start, i};
        return result;
      }
      --depth;
      i += 2;
      continue;
    }
    ++i;
  }

  SPEC_RULE("Block-Comment-Unterminated");
  const auto span = SpanOfText(source, scalars, start, start + 2);
  const auto diag = core::MakeDiagnostic("E-SRC-0306", span);
  if (diag.has_value()) {
    result.diags = core::Emit(result.diags, *diag);
  }
  result.ok = false;
  result.next = n;
  result.range = ScalarRange{start, n};
  return result;
}

}  // namespace cursive0::syntax
