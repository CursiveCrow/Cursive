#include "cursive0/syntax/parser.h"

#include <string_view>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/diagnostic_messages.h"
#include "cursive0/core/diagnostics.h"

namespace cursive0::syntax {

namespace {

bool MatchLexeme(TokenKind kind) {
  return kind == TokenKind::Keyword || kind == TokenKind::Operator ||
         kind == TokenKind::Punctuator;
}

}  // namespace

bool TokenMatches(const Token& tok, const TokenKindMatch& match) {
  if (tok.kind != match.kind) {
    return false;
  }
  if (MatchLexeme(match.kind)) {
    return tok.lexeme == match.lexeme;
  }
  return true;
}

bool TokenInEndSet(const Token& tok,
                   std::span<const TokenKindMatch> end_set) {
  for (const auto& match : end_set) {
    if (TokenMatches(tok, match)) {
      return true;
    }
  }
  return false;
}

bool ConsumeKind(Parser& parser, TokenKind kind) {
  SPEC_RULE("Tok-Consume-Kind");
  const Token* tok = Tok(parser);
  if (!tok || tok->kind != kind) {
    return false;
  }
  Advance(parser);
  return true;
}

bool ConsumeKeyword(Parser& parser, std::string_view keyword) {
  SPEC_RULE("Tok-Consume-Keyword");
  const Token* tok = Tok(parser);
  if (!tok || tok->kind != TokenKind::Keyword || tok->lexeme != keyword) {
    return false;
  }
  Advance(parser);
  return true;
}

bool ConsumeOperator(Parser& parser, std::string_view op) {
  SPEC_RULE("Tok-Consume-Operator");
  const Token* tok = Tok(parser);
  if (!tok || tok->kind != TokenKind::Operator || tok->lexeme != op) {
    return false;
  }
  Advance(parser);
  return true;
}

bool ConsumePunct(Parser& parser, std::string_view punct) {
  SPEC_RULE("Tok-Consume-Punct");
  const Token* tok = Tok(parser);
  if (!tok || tok->kind != TokenKind::Punctuator || tok->lexeme != punct) {
    return false;
  }
  Advance(parser);
  return true;
}

bool TrailingComma(const Parser& parser,
                   std::span<const TokenKindMatch> end_set) {
  const Token* tok = Tok(parser);
  if (!tok || tok->kind != TokenKind::Punctuator || tok->lexeme != ",") {
    return false;
  }
  const Parser next = AdvanceOrEOF(parser);
  const Token* next_tok = Tok(next);
  if (!next_tok) {
    return false;
  }
  return TokenInEndSet(*next_tok, end_set);
}

bool TrailingCommaAllowed(const Parser& parser,
                          std::span<const TokenKindMatch> end_set) {
  if (!TrailingComma(parser, end_set)) {
    return false;
  }
  const Token* comma = Tok(parser);
  const Parser next = AdvanceOrEOF(parser);
  const Token* end_tok = Tok(next);
  if (!comma || !end_tok) {
    return false;
  }
  return comma->span.start_line < end_tok->span.start_line;
}

bool EmitTrailingCommaErr(Parser& parser,
                          std::span<const TokenKindMatch> end_set) {
  SPEC_RULE("Trailing-Comma-Err");
  if (!TrailingComma(parser, end_set)) {
    return false;
  }
  if (TrailingCommaAllowed(parser, end_set)) {
    return false;
  }
  auto diag = core::MakeDiagnostic("E-SRC-0521", TokSpan(parser));
  if (!diag) {
    return true;
  }
  parser.diags = core::Emit(parser.diags, *diag);
  return true;
}

}  // namespace cursive0::syntax
