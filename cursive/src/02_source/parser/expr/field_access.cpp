// =============================================================================
// MIGRATION MAPPING: field_access.cpp
// =============================================================================
// This file contains parsing logic for field access expressions (expr.field).
//
// SPEC REFERENCE: C0updated.md, Lines 5413-5416
// -----------------------------------------------------------------------------
// **(Postfix-Field)** Lines 5413-5416
// IsPunc(Tok(P), ".")    IsIdent(Tok(Advance(P)))    name = Lexeme(Tok(Advance(P)))
// ──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
// Γ ⊢ PostfixStep(P, e) ⇓ (Advance(Advance(P)), FieldAccess(e, name))
//
// SEMANTICS:
// - Field access uses dot notation: expr.field_name
// - The base expression is evaluated first, then the field is accessed
// - The name after the dot must be a valid identifier token
// - Returns FieldAccess AST node containing base expression and field name
//
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_expr.cpp
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
// 1. Field Access Branch (in PostfixStep function)
//    Source: parser_expr.cpp, lines 738-765
//    ```cpp
//    if (IsPunc(parser, ".")) {
//      Parser next = parser;
//      Advance(next);
//      const Token* tok = Tok(next);
//      if (tok && IsIdentTok(*tok)) {
//        SPEC_RULE("Postfix-Field");
//        Identifier name = tok->lexeme;
//        Parser after = next;
//        Advance(after);
//        FieldAccessExpr field;
//        field.base = expr;
//        field.name = name;
//        return {after, MakeExpr(SpanCover(expr->span, tok->span), field)};
//      }
//      // [continues with tuple index check, then error handling]
//    }
//    ```
//
// NOTE: The dot check starts the field/tuple access branch. If the token
// after dot is an identifier, it's field access. If it's an integer literal,
// it's tuple access (see tuple_access.cpp).
//
// ERROR HANDLING (lines 762-765):
// If neither identifier nor integer follows the dot:
//    ```cpp
//    EmitParseSyntaxErr(next, TokSpan(next));
//    Parser sync = next;
//    SyncStmt(sync);
//    return {sync, MakeExpr(SpanBetween(parser, sync), ErrorExpr{})};
//    ```
//
// AST DEFINITIONS (from ast.h, lines 469-472):
// ```cpp
// struct FieldAccessExpr {
//   ExprPtr base;
//   Identifier name;
// };
// ```
//
// DEPENDENCIES:
// - Requires: IsPunc, IsIdentTok, Tok, Advance helpers
// - Requires: FieldAccessExpr AST node type
// - Requires: MakeExpr, SpanCover helpers for AST construction
// - Requires: EmitParseSyntaxErr, SyncStmt for error handling
// - Produces: ExprPtr containing FieldAccessExpr
//
// REFACTORING NOTES:
// - This is part of the PostfixStep function which handles all postfix ops
// - The current implementation shares the initial dot check with tuple access
// - Consider: separate entry point or combined field/tuple module?
// - Span covers from base expression start to field name token end
// - Error recovery uses SyncStmt to skip to statement boundary
// =============================================================================
