// =============================================================================
// parser_consume.cpp - Token Consumption Helpers
// =============================================================================
//
// SPEC REFERENCE: CursiveSpecification.md Section 3.3.5 (Lines 3385-3438)
//
// This file implements token consumption operations:
//   - TokenMatches: Check if token matches a specification
//   - TokenInEndSet: Check if token is in end set (for list parsing)
//   - RecordListStart/RecordListCons/ListDone: canonical list small-step traces
//   - ConsumeKind: Consume token by kind
//   - ConsumeKeyword: Consume keyword token
//   - ConsumeOperator: Consume operator token
//   - ConsumePunct: Consume punctuator token
//   - TrailingComma: Detect trailing comma
//   - TrailingCommaAllowed: Check if trailing comma is valid
//   - EmitTrailingCommaErr: Emit error for invalid trailing comma
//
// =============================================================================

#include "02_source/parser/parser.h"

#include <string_view>

#include "00_core/assert_spec.h"
#include "00_core/diagnostic_messages.h"
#include "00_core/diagnostics.h"

namespace cursive::ast {

// Use lexer types
using cursive::lexer::Token;
using cursive::lexer::TokenKind;

namespace {

// =============================================================================
// MatchLexeme - Determine if token kind requires lexeme matching
// =============================================================================

bool MatchLexeme(TokenKind kind) {
  return kind == TokenKind::Keyword || kind == TokenKind::Operator ||
         kind == TokenKind::Punctuator;
}

}  // namespace

// =============================================================================
// TokenMatches - Check if token matches specification
// =============================================================================
//
// Checks if a token matches a TokenKindMatch specification.
// For keywords, operators, and punctuators, both kind and lexeme must match.
// For other token kinds, only the kind needs to match.

bool TokenMatches(const Token& tok, const TokenKindMatch& match) {
  if (tok.kind != match.kind) {
    return false;
  }
  if (MatchLexeme(match.kind)) {
    return tok.lexeme == match.lexeme;
  }
  return true;
}

// =============================================================================
// TokenInEndSet - Check if token is in end set
// =============================================================================
//
// SPEC: Section 3.3.5 lines 3410-3427 (List Parsing)
// Used for list parsing to detect list terminators.

bool TokenInEndSet(const Token& tok,
                   std::span<const TokenKindMatch> end_set) {
  for (const auto& match : end_set) {
    if (TokenMatches(tok, match)) {
      return true;
    }
  }
  return false;
}

// =============================================================================
// RecordListStart / RecordListCons / ListDone - Canonical List Parsing Steps
// =============================================================================
//
// SPEC: List-Start / List-Cons / List-Done (lines 3416-3427)
// These helpers back the parser-wide list combinators in parser.h so the
// canonical list rules live in a source file that the generated rule registry
// can see, and so real parser paths can emit the generic list traces.

void RecordListStart() { SPEC_RULE("List-Start"); }

void RecordListCons() { SPEC_RULE("List-Cons"); }

bool ListDone(const Parser& parser,
              std::span<const TokenKindMatch> end_set) {
  const Token* tok = Tok(parser);
  if (!tok || !TokenInEndSet(*tok, end_set)) {
    return false;
  }
  SPEC_RULE("List-Done");
  return true;
}

// =============================================================================
// ConsumeKind - Consume token by kind
// =============================================================================
//
// SPEC: Tok-Consume-Kind (lines 3390-3393)
//   Tok(P).kind = k
//   ----------------------------------------
//   <Consume(P, k)> -> <ConsumeDone(Advance(P))>

bool ConsumeKind(Parser& parser, TokenKind kind) {
  SPEC_RULE("Tok-Consume-Kind");
  const Token* tok = Tok(parser);
  if (!tok || tok->kind != kind) {
    return false;
  }
  Advance(parser);
  return true;
}

// =============================================================================
// ConsumeKeyword - Consume keyword token
// =============================================================================
//
// SPEC: Tok-Consume-Keyword (lines 3395-3398)
//   IsKw(Tok(P), s)
//   ----------------------------------------
//   <Consume(P, Keyword(s))> -> <ConsumeDone(Advance(P))>

bool ConsumeKeyword(Parser& parser, std::string_view keyword) {
  SPEC_RULE("Tok-Consume-Keyword");
  const Token* tok = Tok(parser);
  if (!tok || tok->kind != TokenKind::Keyword || tok->lexeme != keyword) {
    return false;
  }
  Advance(parser);
  return true;
}

// =============================================================================
// ConsumeOperator - Consume operator token
// =============================================================================
//
// SPEC: Tok-Consume-Operator (lines 3400-3403)
//   IsOp(Tok(P), s)
//   ----------------------------------------
//   <Consume(P, Operator(s))> -> <ConsumeDone(Advance(P))>

bool ConsumeOperator(Parser& parser, std::string_view op) {
  SPEC_RULE("Tok-Consume-Operator");
  const Token* tok = Tok(parser);
  if (!tok || tok->kind != TokenKind::Operator || tok->lexeme != op) {
    return false;
  }
  Advance(parser);
  return true;
}

// =============================================================================
// ConsumePunct - Consume punctuator token
// =============================================================================
//
// SPEC: Tok-Consume-Punct (lines 3405-3408)
//   IsPunc(Tok(P), s)
//   ----------------------------------------
//   <Consume(P, Punctuator(s))> -> <ConsumeDone(Advance(P))>

bool ConsumePunct(Parser& parser, std::string_view punct) {
  SPEC_RULE("Tok-Consume-Punct");
  const Token* tok = Tok(parser);
  if (!tok || tok->kind != TokenKind::Punctuator || tok->lexeme != punct) {
    return false;
  }
  Advance(parser);
  return true;
}

// =============================================================================
// TrailingComma - Detect trailing comma
// =============================================================================
//
// SPEC: TrailingComma predicate (line 3430)
//   TrailingComma(P, EndSet) <=> IsPunc(Tok(P), ",") && Tok(Advance(P)) in EndSet
//
// Detects trailing comma before end delimiter.

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

// =============================================================================
// TrailingCommaAllowed - Check if trailing comma is valid
// =============================================================================
//
// SPEC: TrailingCommaAllowed predicate (line 3433)
//   TrailingCommaAllowed(P_0, P, EndSet) <=>
//     TrailingComma(P, EndSet) && TokLine(Tok(P)) < TokLine(Tok(Advance(P)))
//
// Trailing comma is allowed only when closing delimiter is on different line.

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

// =============================================================================
// EmitTrailingCommaErr - Emit error for invalid trailing comma
// =============================================================================
//
// SPEC: Trailing-Comma-Err (lines 3435-3438)
//   TrailingComma && !TrailingCommaAllowed => Emit(Code(Trailing-Comma-Err))
//
// Emits E-SRC-0521 diagnostic for single-line trailing commas.

bool EmitTrailingCommaErr(Parser& parser,
                          std::span<const TokenKindMatch> end_set) {
  SPEC_RULE("Trailing-Comma-Err");
  if (!TrailingComma(parser, end_set)) {
    return false;
  }
  if (TrailingCommaAllowed(parser, end_set)) {
    return false;
  }
  auto diag = core::MakeDiagnosticById("E-SRC-0521", TokSpan(parser));
  if (!diag) {
    return true;
  }
  core::Emit(parser.diags, *diag);
  return true;
}

}  // namespace cursive::ast
