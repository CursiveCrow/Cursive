// =============================================================================
// MIGRATION MAPPING: array_literal.cpp
// =============================================================================
// This file should contain parsing logic for array literal expressions,
// including empty arrays, element lists, and array repeat syntax.
//
// SPEC REFERENCE: C0updated.md
// -----------------------------------------------------------------------------
// **(Parse-Array-Literal)** Lines 5214-5217
//   IsPunc(Tok(P), "[")
//   Γ ⊢ ParseExprList(Advance(P)) ⇓ (P_1, elems)    IsPunc(Tok(P_1), "]")
//   ────────────────────────────────────────────────────────────────────────────
//   Γ ⊢ ParsePrimary(P) ⇓ (Advance(P_1), ArrayExpr(elems))
//
// **Expression List Rules** (Lines 5828-5834)
//
// **(Parse-ExprList-Cons)** Lines 5828-5830
//   Γ ⊢ ParseExpr(P) ⇓ (P_1, e)    Γ ⊢ ParseExprListTail(P_1, [e]) ⇓ (P_2, es)
//   ────────────────────────────────────────────────────────────────────────────
//   Γ ⊢ ParseExprList(P) ⇓ (P_2, es)
//
// **(Parse-ExprList-Empty)** Lines 5831-5834
//   Tok(P) ∈ {Punctuator(")"), Punctuator("]")}
//   Γ ⊢ Emit(Code(Unsupported-Construct))
//   ────────────────────────────────────────────────────────────────────────────
//   Γ ⊢ ParseExprList(P) ⇓ (P, [])
//
// **Expression List Tail Rules** (Lines 5960-5973)
//
// **(Parse-ExprListTail-End)** Lines 5960-5963
//   Tok(P) ∈ {Punctuator(")"), Punctuator("]"), Punctuator("}")}
//   ────────────────────────────────────────────────────────────────────────────
//   Γ ⊢ ParseExprListTail(P, xs) ⇓ (P, xs)
//
// **(Parse-ExprListTail-TrailingComma)** Lines 5965-5968
//   IsPunc(Tok(P), ",")    IsPunc(Tok(Advance(P)), "]")
//   TrailingCommaAllowed(P_0, P, {Punctuator("]")})
//   ────────────────────────────────────────────────────────────────────────────
//   Γ ⊢ ParseExprListTail(P, xs) ⇓ (Advance(P), xs)
//
// **(Parse-ExprListTail-Comma)** Lines 5970-5973
//   IsPunc(Tok(P), ",")    Γ ⊢ ParseExpr(Advance(P)) ⇓ (P_1, e)
//   Γ ⊢ ParseExprListTail(P_1, xs ++ [e]) ⇓ (P_2, ys)
//   ────────────────────────────────────────────────────────────────────────────
//   Γ ⊢ ParseExprListTail(P, xs) ⇓ (P_2, ys)
//
// SEMANTICS:
// - [] is an empty array (emits Unsupported-Construct in bootstrap)
// - [e] is a single-element array
// - [e1, e2, ...] is a multi-element array
// - [e; n] is array repeat syntax (e repeated n times)
// - Trailing comma allowed only on multi-line
//
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_expr.cpp
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
// 1. Empty array parsing
//    Source: parser_expr.cpp, lines 1244-1251
//    Purpose: Parse [] as empty ArrayExpr
//    SPEC_RULE: "Parse-Array-Literal-Empty"
//
// 2. Array repeat syntax parsing
//    Source: parser_expr.cpp, lines 1258-1278
//    Purpose: Parse [expr; count] as ArrayRepeatExpr
//    SPEC_RULE: "Parse-Array-Repeat"
//
// 3. Single-element array parsing
//    Source: parser_expr.cpp, lines 1281-1289
//    Purpose: Parse [expr] as single-element ArrayExpr
//    SPEC_RULE: "Parse-Array-Literal-Single"
//
// 4. Multi-element array parsing
//    Source: parser_expr.cpp, lines 1291-1308
//    Purpose: Parse [e1, e2, ...] as ArrayExpr
//    SPEC_RULE: "Parse-Array-Literal-List"
//
// 5. ParseExprList function
//    Source: parser_expr.cpp, lines 2138-2154
//    Purpose: Parse comma-separated expression list
//
// 6. ParseExprListTail function
//    Source: parser_expr.cpp, lines 2156-2173
//    Purpose: Parse tail of comma-separated expression list
//
// DEPENDENCIES:
// - Requires: ParseExpr (recursive expression parsing)
// - Requires: ArrayExpr, ArrayRepeatExpr AST node types
// - Requires: IsPunc, Tok, Advance helpers
// - Requires: SkipNewlines for multi-line handling
// - Requires: EmitParseSyntaxErr for error handling
// - Requires: EmitUnsupportedConstruct for empty array diagnostic
// - Requires: MakeExpr, SpanBetween helpers
// - Requires: SyncStmt for error recovery
//
// REFACTORING NOTES:
// - Bootstrap parses array repeat [e; n] as distinct from element lists
// - Empty arrays emit Unsupported-Construct (bootstrap restriction)
// - The spec's ParseExprList is used for general expression lists
// - ParseExprListTail handles the accumulator-based tail parsing
// - Consider unifying with tuple element parsing (similar structure)
// =============================================================================
