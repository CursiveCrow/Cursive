// =============================================================================
// MIGRATION MAPPING: tuple_literal.cpp
// =============================================================================
// This file should contain parsing logic for tuple literal expressions and
// the disambiguation between tuples and parenthesized expressions.
//
// SPEC REFERENCE: C0updated.md
// -----------------------------------------------------------------------------
// **Primary Expression Rules** (Lines 5204-5212)
//
// **(Parse-Parenthesized-Expr)** Lines 5204-5207
//   IsPunc(Tok(P), "(")    ¬ TupleParen(P)
//   Γ ⊢ ParseExpr(Advance(P)) ⇓ (P_1, e)    IsPunc(Tok(P_1), ")")
//   ────────────────────────────────────────────────────────────────────────────
//   Γ ⊢ ParsePrimary(P) ⇓ (Advance(P_1), e)
//
// **(Parse-Tuple-Literal)** Lines 5209-5212
//   IsPunc(Tok(P), "(")    TupleParen(P)
//   Γ ⊢ ParseTupleExprElems(Advance(P)) ⇓ (P_1, elems)    IsPunc(Tok(P_1), ")")
//   ────────────────────────────────────────────────────────────────────────────
//   Γ ⊢ ParsePrimary(P) ⇓ (Advance(P_1), TupleExpr(elems))
//
// **Tuple Expression Elements** (Lines 5836-5856)
//
// **(Parse-TupleExprElems-Empty)** Lines 5838-5841
//   IsPunc(Tok(P), ")")
//   ────────────────────────────────────────────────────────────────────────────
//   Γ ⊢ ParseTupleExprElems(P) ⇓ (P, [])
//
// **(Parse-TupleExprElems-Single)** Lines 5843-5846
//   Γ ⊢ ParseExpr(P) ⇓ (P_1, e)    IsPunc(Tok(P_1), ";")
//   ────────────────────────────────────────────────────────────────────────────
//   Γ ⊢ ParseTupleExprElems(P) ⇓ (Advance(P_1), [e])
//
// **(Parse-TupleExprElems-TrailingComma)** Lines 5848-5851
//   Γ ⊢ ParseExpr(P) ⇓ (P_1, e)    IsPunc(Tok(P_1), ",")
//   IsPunc(Tok(Advance(P_1)), ")")    TrailingCommaAllowed(P_0, P_1, {Punctuator(")")})
//   ────────────────────────────────────────────────────────────────────────────
//   Γ ⊢ ParseTupleExprElems(P) ⇓ (Advance(P_1), [e])
//
// **(Parse-TupleExprElems-Many)** Lines 5853-5856
//   Γ ⊢ ParseExpr(P) ⇓ (P_1, e_1)    IsPunc(Tok(P_1), ",")
//   Γ ⊢ ParseExpr(Advance(P_1)) ⇓ (P_2, e_2)
//   Γ ⊢ ParseExprListTail(P_2, [e_2]) ⇓ (P_3, es)
//   ────────────────────────────────────────────────────────────────────────────
//   Γ ⊢ ParseTupleExprElems(P) ⇓ (P_3, [e_1] ++ es)
//
// **Tuple vs Parenthesized Disambiguation** (Lines 5858-5877)
//
// ParenDelta(Punctuator("(")) = 1
// ParenDelta(Punctuator(")")) = -1
// ParenDelta(t) = 0 if t.kind ∉ {Punctuator("("), Punctuator(")")}
//
// TupleScan(P, d) rules:
// - Returns false if EOF
// - Returns false if ) found at depth 1 (no separator seen)
// - Returns true if , or ; found at depth 1 (separator seen = tuple)
// - Recurses with adjusted depth otherwise
//
// TupleParen(P) ⇔ IsPunc(Tok(P), "(") ∧
//                 (IsPunc(Tok(Advance(P)), ")") ∨ TupleScan(Advance(P), 1) ⇓ true)
//
// SEMANTICS:
// - () is the unit tuple (empty tuple)
// - (e) is a parenthesized expression (NOT a tuple)
// - (e;) is a single-element tuple (semicolon required)
// - (e1, e2) is a multi-element tuple (comma-separated)
// - (e1, e2,) with trailing comma allowed on multi-line only
//
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_expr.cpp
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
// 1. ParenDelta helper function
//    Source: parser_expr.cpp, lines 343-354
//    Purpose: Compute paren depth delta for a token
//
// 2. TupleScan function
//    Source: parser_expr.cpp, lines 356-382
//    Purpose: Scan ahead to determine if parens contain tuple or single expr
//    Returns: TupleScanResult with is_tuple boolean
//
// 3. TupleParen function
//    Source: parser_expr.cpp, lines 384-395
//    Purpose: Entry point for tuple vs paren disambiguation
//    Returns: true if parens contain a tuple, false for parenthesized expr
//
// 4. Parenthesized expression parsing
//    Source: parser_expr.cpp, lines 1206-1220
//    Purpose: Parse (expr) when TupleParen returns false
//    SPEC_RULE: "Parse-Parenthesized-Expr"
//
// 5. Tuple literal parsing
//    Source: parser_expr.cpp, lines 1221-1236
//    Purpose: Parse (elem1, elem2, ...) when TupleParen returns true
//    SPEC_RULE: "Parse-Tuple-Literal"
//
// 6. ParseTupleExprElems function
//    Source: parser_expr.cpp, lines 2175-2218
//    Purpose: Parse comma-separated or semicolon-terminated tuple elements
//    SPEC_RULE: "Parse-TupleExprElems-Empty", "Parse-TupleExprElems-Single",
//               "Parse-TupleExprElems-TrailingComma", "Parse-TupleExprElems-Many"
//
// DEPENDENCIES:
// - Requires: ParseExpr (recursive expression parsing)
// - Requires: ParseExprListTail (tail list parsing)
// - Requires: TupleExpr AST node type
// - Requires: IsPunc, Tok, Advance helpers
// - Requires: SkipNewlines for multi-line handling
// - Requires: EmitTrailingCommaErr for single-line trailing comma error
// - Requires: MakeExpr, SpanBetween helpers
//
// REFACTORING NOTES:
// - TupleScan is a lookahead disambiguation that doesn't consume tokens
// - Single-element tuple requires (e;) syntax to distinguish from (e)
// - Trailing comma handling requires TrailingCommaAllowed check
// - Empty parens () always produce empty tuple
// - Consider making disambiguation more efficient or stateless
// =============================================================================
