// =============================================================================
// MIGRATION MAPPING: token.cpp
// =============================================================================
//
// SPEC REFERENCE: CursiveSpecification.md
//   Section 1.6.2 - Token Spans (lines 581-612)
//   Section 3.2.1 - Inputs, Outputs, and Records (lines 1997-2046)
//   Section 3.2.4 - Token Kinds (lines 2107-2118)
//
// SOURCE FILE: cursive-bootstrap/src/02_syntax/token.cpp
//   Lines 1-53 (entire file)
//
// DEPENDENCIES:
//   - cursive/include/02_source/token.h (Token, RawToken, TokenKind, EofToken)
//   - cursive/src/00_core/assert_spec.cpp (SPEC_DEF, SPEC_RULE)
//   - cursive/src/00_core/span.cpp (SpanOf)
//   - cursive/src/01_input/source.cpp (SourceFile)
//
// =============================================================================
// CONTENT TO MIGRATE:
// =============================================================================
//
// 1. SpecDefsTokenTypes() - Spec definition helper (source lines 7-11)
//    SPEC_DEF("TokenKind", "1.6.2")
//    SPEC_DEF("RawToken", "1.6.2")
//    SPEC_DEF("Token", "1.6.2")
//
// 2. NoUnknownOk() - Token validation (source lines 13-22)
//    Implements: No-Unknown-Ok rule from spec 1.6.2
//    Returns false if any token has Unknown kind
//
// 3. AttachSpan() - Single token span attachment (source lines 24-32)
//    Implements: Attach-Token-Ok rule from spec 1.6.2
//    Converts RawToken (byte offsets) to Token (with Span)
//    Uses core::SpanOf(source, start_offset, end_offset)
//
// 4. AttachSpans() - Batch span attachment (source lines 34-44)
//    Implements: Attach-Tokens-Ok rule from spec 1.6.2
//    Applies AttachSpan to vector of RawTokens
//
// 5. MakeEofToken() - EOF token construction (source lines 46-51)
//    Implements: TokenEOF from spec 3.2.1 line 2011
//    EOFSpan(S) = SpanOfText(S, |T|, |T|)
//    Creates token at end of source with zero-width span
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
//
// - Consider renaming AttachSpan/AttachSpans to more descriptive names
// - The spec defines TokenKind_C0 = TokenKind_(3.2.4) + {Unknown}
//   Ensure TokenKind enum includes all spec-defined kinds
// - Spec 3.2.4 line 2109 defines:
//   TokenKind in {Identifier, Keyword(k), IntLiteral, FloatLiteral,
//                 StringLiteral, CharLiteral, BoolLiteral, NullLiteral,
//                 Operator(o), Punctuator(p), Newline, Unknown}
// - OperatorSet defined at line 2112
// - PunctuatorSet defined at line 2115
//
// =============================================================================

#include "02_source/lexer/token.h"

#include "00_core/assert_spec.h"

namespace cursive::lexer {

namespace {

void SpecDefsTokenTypes() {
  SPEC_DEF("TokenKind", "1.6.2");
  SPEC_DEF("RawToken", "1.6.2");
  SPEC_DEF("Token", "1.6.2");
}

}  // namespace

bool NoUnknownOk(const std::vector<Token>& tokens) {
  SPEC_RULE("No-Unknown-Ok");
  SpecDefsTokenTypes();
  for (const auto& tok : tokens) {
    if (tok.kind == TokenKind::Unknown) {
      return false;
    }
  }
  return true;
}

Token AttachSpan(const core::SourceFile& source, const RawToken& raw) {
  SPEC_RULE("Attach-Token-Ok");
  SpecDefsTokenTypes();
  Token tok;
  tok.kind = raw.kind;
  tok.lexeme = raw.lexeme;
  tok.span = core::SpanOf(source, raw.start_offset, raw.end_offset);
  return tok;
}

std::vector<Token> AttachSpans(const core::SourceFile& source,
                               const std::vector<RawToken>& raws) {
  SPEC_RULE("Attach-Tokens-Ok");
  SpecDefsTokenTypes();
  std::vector<Token> out;
  out.reserve(raws.size());
  for (const auto& raw : raws) {
    out.push_back(AttachSpan(source, raw));
  }
  return out;
}

Token MakeEofToken(const core::SourceFile& source) {
  SPEC_DEF("TokenEOF", "3.2.1");
  Token eof;
  eof.kind = TokenKind::Unknown;
  eof.lexeme.clear();
  eof.span = core::SpanOf(source, source.byte_len, source.byte_len);
  return eof;
}

}  // namespace cursive::lexer
