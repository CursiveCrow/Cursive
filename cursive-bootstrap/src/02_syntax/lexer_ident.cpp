#include "cursive0/02_syntax/lexer.h"

#include <cstddef>
#include <string_view>
#include <vector>

#include "cursive0/00_core/assert_spec.h"
#include "cursive0/00_core/diagnostic_messages.h"
#include "cursive0/00_core/keywords.h"
#include "cursive0/00_core/source_text.h"
#include "cursive0/00_core/span.h"
#include "cursive0/00_core/unicode.h"

namespace cursive0::syntax {

namespace {

bool IsKeyword(std::string_view ident) {
  return core::IsKeyword(ident);
}


}  // namespace

IdentScanResult ScanIdentToken(const core::SourceFile& source,
                               std::size_t start) {
  SPEC_RULE("Lex-Identifier");
  SPEC_RULE("Lex-Ident-Token");
  IdentScanResult result;
  const auto& scalars = source.scalars;
  if (start >= scalars.size() || !core::IsIdentStart(scalars[start])) {
    result.ok = false;
    result.next = start;
    return result;
  }

  std::size_t end = start + 1;
  while (end < scalars.size() && core::IsIdentContinue(scalars[end])) {
    ++end;
  }

  const auto offsets = core::Utf8Offsets(scalars);
  const std::size_t byte_start = offsets[start];
  const std::size_t byte_end = offsets[end];
  result.lexeme = source.text.substr(byte_start, byte_end - byte_start);
  result.ok = true;
  result.next = end;

  for (std::size_t i = start; i < end; ++i) {
    if (!core::IsNonCharacter(scalars[i])) {
      continue;
    }
    SPEC_RULE("Lex-Ident-InvalidUnicode");
    const auto span = core::SpanOf(source, offsets[i], offsets[i + 1]);
    if (auto diag = core::MakeDiagnostic("E-SRC-0307", span)) {
      result.diags = core::Emit(result.diags, *diag);
    }
    break;
  }

  if (result.lexeme == "true" || result.lexeme == "false") {
    result.kind = TokenKind::BoolLiteral;
  } else if (result.lexeme == "null") {
    result.kind = TokenKind::NullLiteral;
  } else if (IsKeyword(result.lexeme)) {
    result.kind = TokenKind::Keyword;
  } else {
    result.kind = TokenKind::Identifier;
  }

  return result;
}

}  // namespace cursive0::syntax
