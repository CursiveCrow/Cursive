// =============================================================================
// where_clause.cpp - Generic Constraint Clause Parsing
// =============================================================================
//
// This file parses generic constraint clauses introduced by `|:`.
// Legacy `where` syntax is rejected with a parse error.
//
// Syntax:
//   |: Bitcopy(T)
//   |: Clone(U)
//
// Predicates are separated by semicolons or newlines.
//
// =============================================================================

#include "02_source/parser/parser.h"

#include <optional>
#include <vector>

#include "00_core/assert_spec.h"
#include "02_source/lexer/keyword_policy.h"

namespace cursive::ast {

// Use lexer types
using cursive::lexer::Token;
using cursive::lexer::TokenKind;
using cursive::lexer::IsIdentTok;

// Forward declarations for helper functions
bool IsKw(const Parser& parser, std::string_view kw);
bool IsOp(const Parser& parser, std::string_view op);
bool IsPunc(const Parser& parser, std::string_view p);
void SkipNewlines(Parser& parser);

// Forward declaration for type parsing
ParseElemResult<std::shared_ptr<Type>> ParseType(Parser parser);
std::shared_ptr<Type> MakeTypePrim(const core::Span& span, std::string_view name);

// =============================================================================
// ParseWhereClauseOpt - Parse optional generic constraint clause
// =============================================================================
//
// Accepted:
//   |: Pred(Type)
//
// Rejected:
//   where Pred(Type)

namespace {

bool IsPredicateReqName(std::string_view name) {
  return name == "Bitcopy" || name == "Clone" || name == "Drop" ||
         name == "FfiSafe";
}

bool StartsPredicateReq(Parser parser) {
  while (Tok(parser) && Tok(parser)->kind == TokenKind::Newline) {
    Advance(parser);
  }

  const Token* pred_tok = Tok(parser);
  if (!pred_tok || !IsIdentTok(*pred_tok) ||
      !IsPredicateReqName(pred_tok->lexeme)) {
    return false;
  }

  Parser after_name = parser;
  Advance(after_name);
  return IsPunc(after_name, "(");
}

ParseElemResult<std::optional<WhereClause>> ParsePredicateClauseImpl(Parser parser) {
  // Skip any newlines before clause start.
  while (Tok(parser) && Tok(parser)->kind == TokenKind::Newline) {
    Advance(parser);
  }

  const bool legacy_where = IsKw(parser, "where");
  if (!legacy_where && !IsOp(parser, "|:")) {
    return {parser, std::nullopt};
  }

  if (!legacy_where) {
    Parser after_clause = parser;
    Advance(after_clause);
    if (!StartsPredicateReq(after_clause)) {
      return {parser, std::nullopt};
    }
  }

  SPEC_RULE("Parse-Where-Clause");
  Parser start = parser;
  Parser next = parser;
  if (legacy_where) {
    EmitParseSyntaxErr(next, TokSpan(next));
  }
  Advance(next);  // consume |: (or legacy where)

  // Skip newlines after |:
  while (Tok(next) && Tok(next)->kind == TokenKind::Newline) {
    Advance(next);
  }

  WhereClause clause;

  // Parse first predicate
  auto parse_predicate = [&](Parser p) -> ParseElemResult<WherePredicate> {
    Parser pred_start = p;
    const Token* pred_tok = Tok(p);
    if (!pred_tok || !IsIdentTok(*pred_tok)) {
      EmitParseSyntaxErr(p, TokSpan(p));
    }
    ParseElemResult<Identifier> pred_name = ParseIdent(p);
    Parser after_name = pred_name.parser;

    if (!IsPredicateReqName(pred_name.elem) || !IsPunc(after_name, "(")) {
      SPEC_RULE("Parse-PredicateReq-Err");
      EmitParseSyntaxErr(after_name, TokSpan(after_name));

      WherePredicate pred;
      pred.predicate = pred_name.elem;
      pred.type = MakeTypePrim(SpanBetween(pred_start, after_name), "!");
      pred.span = SpanBetween(pred_start, after_name);
      return {after_name, pred};
    }

    SPEC_RULE("Parse-PredicateReq-Predicate");
    Advance(after_name);

    ParseElemResult<std::shared_ptr<Type>> ty = ParseType(after_name);
    Parser after_type = ty.parser;

    if (!IsPunc(after_type, ")")) {
      EmitParseSyntaxErr(after_type, TokSpan(after_type));
    } else {
      Advance(after_type);
    }

    WherePredicate pred;
    pred.predicate = pred_name.elem;
    pred.type = ty.elem;
    pred.span = SpanBetween(pred_start, after_type);
    return {after_type, pred};
  };

  ParseElemResult<WherePredicate> first = parse_predicate(next);
  clause.predicates.push_back(first.elem);
  next = first.parser;

  // Parse additional predicates separated by terminators
  while (true) {
    Parser after_term = next;
    bool consumed = false;
    while (Tok(after_term) &&
           (Tok(after_term)->kind == TokenKind::Newline ||
            IsPunc(after_term, ";"))) {
      Advance(after_term);
      consumed = true;
    }

    if (!consumed) {
      break;
    }

    const Token* next_tok = Tok(after_term);
    if (next_tok && IsIdentTok(*next_tok)) {
      ParseElemResult<WherePredicate> pred = parse_predicate(after_term);
      clause.predicates.push_back(pred.elem);
      next = pred.parser;
      continue;
    }

    // Trailing terminator allowed.
    next = after_term;
    break;
  }

  clause.span = SpanBetween(start, next);
  return {next, clause};
}

}  // namespace

ParseElemResult<std::optional<WhereClause>> ParseWhereClauseOpt(Parser parser) {
  return ParsePredicateClauseImpl(parser);
}

ParseElemResult<std::optional<WhereClause>> ParsePredicateClauseOpt(
    Parser parser) {
  return ParsePredicateClauseImpl(parser);
}

}  // namespace cursive::ast
