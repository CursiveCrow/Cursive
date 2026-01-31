// =============================================================================
// MIGRATION MAPPING: tuple_access.cpp
// =============================================================================
// This file contains parsing logic for tuple element access (expr.0, expr.1).
//
// SPEC REFERENCE: C0updated.md, Lines 5418-5421
// -----------------------------------------------------------------------------
// **(Postfix-TupleIndex)** Lines 5418-5421
// IsPunc(Tok(P), ".")    t = Tok(Advance(P))    t.kind = IntLiteral    idx = IntValue(t)
// ──────────────────────────────────────────────────────────────────────────────────────────────────────────
// Γ ⊢ PostfixStep(P, e) ⇓ (Advance(Advance(P)), TupleAccess(e, idx))
//
// SEMANTICS:
// - Tuple element access uses dot notation with integer literal: expr.0, expr.1
// - The base expression is evaluated first, then the indexed element is accessed
// - The index must be an integer literal token (not an expression)
// - Zero-indexed: first element is .0, second is .1, etc.
// - Returns TupleAccess AST node containing base expression and index token
//
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_expr.cpp
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
// 1. Tuple Access Branch (in PostfixStep function, after field access check)
//    Source: parser_expr.cpp, lines 752-761
//    ```cpp
//    if (tok && tok->kind == TokenKind::IntLiteral) {
//      SPEC_RULE("Postfix-TupleIndex");
//      Token index = *tok;
//      Parser after = next;
//      Advance(after);
//      TupleAccessExpr access;
//      access.base = expr;
//      access.index = index;
//      return {after, MakeExpr(SpanCover(expr->span, index.span), access)};
//    }
//    ```
//
// CONTEXT (lines 738-741 setup):
// The dot is checked first, shared with field access:
//    ```cpp
//    if (IsPunc(parser, ".")) {
//      Parser next = parser;
//      Advance(next);
//      const Token* tok = Tok(next);
//      // ... field access check first (identifier) ...
//      // ... then tuple access check (integer literal) ...
//    }
//    ```
//
// AST DEFINITIONS (from ast.h, lines 474-477):
// ```cpp
// struct TupleAccessExpr {
//   ExprPtr base;
//   Token index;    // Stores the full token, not just numeric value
// };
// ```
//
// DEPENDENCIES:
// - Requires: IsPunc, Tok, Advance helpers
// - Requires: TokenKind::IntLiteral constant
// - Requires: TupleAccessExpr AST node type
// - Requires: MakeExpr, SpanCover helpers for AST construction
// - Produces: ExprPtr containing TupleAccessExpr
//
// REFACTORING NOTES:
// - Shares the initial dot check with field_access.cpp
// - The index is stored as a Token (not parsed integer) to preserve span info
// - Tuple bounds checking happens during semantic analysis, not parsing
// - Consider: combined field/tuple access module or keep separate?
// - Span covers from base expression start to index token end
// =============================================================================
