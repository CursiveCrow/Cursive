#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <optional>
#include <string>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/source_text.h"
#include "cursive0/core/span.h"
#include "cursive0/core/unicode.h"
#include "cursive0/syntax/lexer.h"
#include "cursive0/syntax/token.h"

namespace {

using cursive0::core::LineStarts;
using cursive0::core::SourceFile;
using cursive0::core::Span;
using cursive0::core::SpanOf;
using cursive0::core::UnicodeScalar;
using cursive0::syntax::IdentScanResult;
using cursive0::syntax::ScanIdentToken;
using cursive0::syntax::TokenKind;

static std::vector<UnicodeScalar> AsScalars(const std::string& text) {
  std::vector<UnicodeScalar> out;
  out.reserve(text.size());
  for (unsigned char c : text) {
    out.push_back(static_cast<UnicodeScalar>(c));
  }
  return out;
}

static std::vector<UnicodeScalar> Scalars(
    std::initializer_list<UnicodeScalar> list) {
  return std::vector<UnicodeScalar>(list);
}

static SourceFile MakeSourceFile(const std::string& path,
                                 const std::vector<UnicodeScalar>& scalars) {
  SourceFile s;
  s.path = path;
  s.scalars = scalars;
  s.text = cursive0::core::EncodeUtf8(scalars);
  s.bytes.assign(s.text.begin(), s.text.end());
  s.byte_len = s.text.size();
  s.line_starts = LineStarts(scalars);
  s.line_count = s.line_starts.size();
  return s;
}

static void ExpectSpanEq(const Span& a, const Span& b) {
  assert(a.file == b.file);
  assert(a.start_offset == b.start_offset);
  assert(a.end_offset == b.end_offset);
  assert(a.start_line == b.start_line);
  assert(a.start_col == b.start_col);
  assert(a.end_line == b.end_line);
  assert(a.end_col == b.end_col);
}

static std::optional<UnicodeScalar> FindNonAsciiIdentChar() {
  for (UnicodeScalar c = 0x80; c <= 0x10FFFF; ++c) {
    if (c >= 0xD800 && c <= 0xDFFF) {
      continue;
    }
    if (cursive0::core::IsIdentStart(c) &&
        cursive0::core::IsIdentContinue(c)) {
      return c;
    }
  }
  return std::nullopt;
}

static std::vector<UnicodeScalar> NonCharacterCandidates() {
  std::vector<UnicodeScalar> out;
  for (UnicodeScalar c = 0xFDD0; c <= 0xFDEF; ++c) {
    out.push_back(c);
  }
  for (UnicodeScalar plane = 0; plane <= 0x10; ++plane) {
    out.push_back(static_cast<UnicodeScalar>(plane * 0x10000u + 0xFFFEu));
    out.push_back(static_cast<UnicodeScalar>(plane * 0x10000u + 0xFFFFu));
  }
  return out;
}

}  // namespace

int main() {
  SPEC_COV("Lex-Identifier");
  SPEC_COV("Lex-Ident-InvalidUnicode");
  SPEC_COV("Lex-Ident-Token");

  {
    const auto source = MakeSourceFile("ident_basic.cursive",
                                       AsScalars("abc_123"));
    IdentScanResult res = ScanIdentToken(source, 0);
    assert(res.ok);
    assert(res.next == 7);
    assert(res.kind == TokenKind::Identifier);
    assert(res.lexeme == "abc_123");
    assert(res.diags.empty());
  }

  {
    const auto source = MakeSourceFile("kw_let.cursive", AsScalars("let"));
    IdentScanResult res = ScanIdentToken(source, 0);
    assert(res.ok);
    assert(res.kind == TokenKind::Keyword);
    assert(res.lexeme == "let");
  }

  {
    const auto source = MakeSourceFile("kw_using.cursive", AsScalars("using"));
    IdentScanResult res = ScanIdentToken(source, 0);
    assert(res.ok);
    assert(res.kind == TokenKind::Keyword);
    assert(res.lexeme == "using");
  }

  {
    const auto source = MakeSourceFile("ident_use.cursive", AsScalars("use"));
    IdentScanResult res = ScanIdentToken(source, 0);
    assert(res.ok);
    assert(res.kind == TokenKind::Identifier);
    assert(res.lexeme == "use");
  }

  {
    const auto source = MakeSourceFile("kw_true.cursive", AsScalars("true"));
    IdentScanResult res = ScanIdentToken(source, 0);
    assert(res.ok);
    assert(res.kind == TokenKind::BoolLiteral);
    assert(res.lexeme == "true");
  }

  {
    const auto source = MakeSourceFile("kw_null.cursive", AsScalars("null"));
    IdentScanResult res = ScanIdentToken(source, 0);
    assert(res.ok);
    assert(res.kind == TokenKind::NullLiteral);
    assert(res.lexeme == "null");
  }

  {
    const auto source = MakeSourceFile("ctx_in.cursive", AsScalars("in"));
    IdentScanResult res = ScanIdentToken(source, 0);
    assert(res.ok);
    assert(res.kind == TokenKind::Identifier);
    assert(res.lexeme == "in");
  }

  {
    const auto source = MakeSourceFile("future_async.cursive",
                                       AsScalars("async"));
    IdentScanResult res = ScanIdentToken(source, 0);
    assert(res.ok);
    assert(res.kind == TokenKind::Identifier);
    assert(res.lexeme == "async");
  }

  {
    const auto candidate = FindNonAsciiIdentChar();
    assert(candidate.has_value());
    const auto scalars = Scalars({*candidate, *candidate});
    const auto source = MakeSourceFile("ident_unicode.cursive", scalars);
    IdentScanResult res = ScanIdentToken(source, 0);
    assert(res.ok);
    assert(res.next == 2);
    assert(res.kind == TokenKind::Identifier);
    assert(res.lexeme == cursive0::core::EncodeUtf8(scalars));
  }

  {
    std::optional<UnicodeScalar> candidate;
    for (const auto c : NonCharacterCandidates()) {
      if (cursive0::core::IsIdentContinue(c)) {
        candidate = c;
        break;
      }
    }

    if (candidate.has_value()) {
      const auto scalars = Scalars({'a', *candidate, 'b'});
      const auto source = MakeSourceFile("ident_nonchar.cursive", scalars);
      IdentScanResult res = ScanIdentToken(source, 0);
      assert(res.ok);
      assert(res.next == scalars.size());
      assert(res.diags.size() == 1);
      assert(res.diags[0].code == "E-SRC-0307");
      const auto offsets = cursive0::core::Utf8Offsets(scalars);
      const Span expect = SpanOf(source, offsets[1], offsets[2]);
      assert(res.diags[0].span.has_value());
      ExpectSpanEq(*res.diags[0].span, expect);
    } else {
      assert(cursive0::core::IsNonCharacter(0xFDD0));
    }
  }

  return 0;
}
