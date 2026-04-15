// =============================================================================
// array_literal.cpp - Array Literal Expression Parsing
// =============================================================================
//
// SPEC REFERENCE: CursiveSpecification.md
// - Parse-Array-Literal (Lines 5214-5217)
// - Parse-ExprList-* (Lines 5828-5834)
// - Parse-ExprListTail-* (Lines 5960-5973)
//
// =============================================================================

#include "02_source/parser/parser.h"

#include <array>
#include <memory>
#include <vector>

#include "00_core/assert_spec.h"
#include "00_core/span.h"
#include "02_source/ast/ast.h"
#include "02_source/lexer/keyword_policy.h"

namespace cursive::ast {

// Forward declarations from other modules
ExprPtr MakeExpr(const core::Span& span, ExprNode node);
bool IsPunc(const Parser& parser, std::string_view punc);
void SkipNewlines(Parser& parser);
ParseElemResult<ExprPtr> ParseExpr(Parser parser);

// =============================================================================
// ParseExprListTail - Parse tail of expression list
// =============================================================================
//
// SPEC: Lines 5960-5973
// Used for comma-separated expression lists in arrays, tuples, etc.

ParseElemResult<std::vector<ExprPtr>> ParseExprListTail(
    ListState<ExprPtr> state) {
  SkipNewlines(state.parser);
  const std::array<TokenKindMatch, 3> end_set = {
      MatchPunct(")"), MatchPunct("]"), MatchPunct("}")};
  if (ListDone(state, end_set)) {
    SPEC_RULE("Parse-ExprListTail-End");
    return {state.parser, state.elems};
  }
  if (IsPunc(state.parser, ",")) {
    Parser after = state.parser;
    Advance(after);
    SkipNewlines(after);
    if (IsPunc(after, ")") || IsPunc(after, "]") || IsPunc(after, "}")) {
      SPEC_RULE("Parse-ExprListTail-TrailingComma");
      EmitTrailingCommaErr(state.parser, end_set);
      after.diags = state.parser.diags;
      return {after, state.elems};
    }
    SPEC_RULE("Parse-ExprListTail-Comma");
    state.parser = after;
    state = ListCons(state, ParseExpr);
    return ParseExprListTail(std::move(state));
  }
  EmitParseSyntaxErr(state.parser, TokSpan(state.parser));
  return {state.parser, state.elems};
}

// =============================================================================
// ParseArrayLiteralExpr - Parse array literal [e1, e2, ...]
// =============================================================================
//
// SPEC: Lines 5214-5217
// Assumes parser is at "[".
// Handles:
// - [] empty array
// - [e] single element
// - [e1, e2, ...] element list

ParseElemResult<ExprPtr> ParseArrayLiteralExpr(Parser parser) {
  Parser start = parser;
  Parser next = parser;
  Advance(next);  // consume "["
  SkipNewlines(next);

  // Empty array []
  if (IsPunc(next, "]")) {
    SPEC_RULE("Parse-Array-Literal-Empty");
    Parser after = next;
    Advance(after);
    ArrayExpr arr;
    return {after, MakeExpr(SpanBetween(start, after), arr)};
  }

  // Parse first expression
  ParseElemResult<ExprPtr> first = ParseExpr(next);
  Parser after_first = first.parser;
  SkipNewlines(after_first);

  // Check for closing bracket (single-element array)
  if (IsPunc(after_first, "]")) {
    SPEC_RULE("Parse-Array-Literal-Single");
    Parser after = after_first;
    Advance(after);
    ArrayExpr arr;
    arr.elements.push_back(first.elem);
    return {after, MakeExpr(SpanBetween(start, after), arr)};
  }

  // Parse comma-separated list
  if (IsPunc(after_first, ",")) {
    SPEC_RULE("Parse-Array-Literal-List");
    ListState<ExprPtr> state = ListSeed(after_first, first.elem);
    ParseElemResult<std::vector<ExprPtr>> rest =
        ParseExprListTail(std::move(state));
    if (!IsPunc(rest.parser, "]")) {
      EmitParseSyntaxErr(rest.parser, TokSpan(rest.parser));
      Parser sync = rest.parser;
      SyncStmt(sync);
      return {sync, MakeExpr(SpanBetween(start, sync), ErrorExpr{})};
    }
    Parser after = rest.parser;
    Advance(after);
    ArrayExpr arr;
    arr.elements = std::move(rest.elem);
    return {after, MakeExpr(SpanBetween(start, after), arr)};
  }

  EmitParseSyntaxErr(after_first, TokSpan(after_first));
  Parser sync = after_first;
  SyncStmt(sync);
  return {sync, MakeExpr(SpanBetween(start, sync), ErrorExpr{})};
}

}  // namespace cursive::ast
