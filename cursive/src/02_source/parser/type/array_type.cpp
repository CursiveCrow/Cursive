// =============================================================================
// MIGRATION MAPPING: array_type.cpp
// =============================================================================
// This file should contain parsing logic for array types: [T; n] where T is
// the element type and n is a compile-time length expression.
//
// SPEC REFERENCE: C0updated.md, Section 3.3.7, Lines 4763-4766
// =============================================================================
//
// FORMAL RULES FROM SPEC:
// -----------------------------------------------------------------------------
//
// **(Parse-Array-Type)** Lines 4763-4766
// IsPunc(Tok(P), "[")
// Γ ⊢ ParseType(Advance(P)) ⇓ (P_1, t)
// IsPunc(Tok(P_1), ";")
// Γ ⊢ ParseExpr(Advance(P_1)) ⇓ (P_2, e)
// IsPunc(Tok(P_2), "]")
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseNonPermType(P) ⇓ (Advance(P_2), TypeArray(t, e))
//
// SEMANTICS:
// - Array types have a fixed compile-time length
// - Syntax: [ElementType; length_expression]
// - Length must be a constant expression evaluating to usize
// - Distinguishable from slices by presence of semicolon and length
// - Examples: [i32; 10], [u8; 256], [Point; N] (where N is const)
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_types.cpp
// =============================================================================
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
//
// 1. Array type parsing in ParseNonPermType (Lines 460-480)
//    ─────────────────────────────────────────────────────────────────────────
//    if (IsPuncTok(*tok, "[")) {
//      Parser start = parser;
//      Parser next = parser;
//      Advance(next);
//      ParseElemResult<std::shared_ptr<Type>> elem = ParseType(next);
//      if (IsPunc(elem.parser, ";")) {
//        SPEC_RULE("Parse-Array-Type");
//        Parser after_semi = elem.parser;
//        Advance(after_semi);
//        ParseElemResult<std::shared_ptr<Expr>> len = ParseExpr(after_semi);
//        if (!IsPunc(len.parser, "]")) {
//          EmitParseSyntaxErr(len.parser, TokSpan(len.parser));
//          return {len.parser, MakeTypePrim(SpanBetween(start, len.parser), "!")};
//        }
//        Parser after_r = len.parser;
//        Advance(after_r);
//        TypeArray arr;
//        arr.element = elem.elem;
//        arr.length = len.elem;
//        return {after_r, MakeTypeNode(SpanBetween(start, after_r), arr)};
//      }
//      // ... slice type case follows (see slice_type.cpp)
//    }
//
// PARSING FLOW:
// -----------------------------------------------------------------------------
//   1. Match opening "[" bracket
//   2. Advance past "["
//   3. Parse element type via ParseType
//   4. If semicolon ";" found:
//      a. Advance past ";"
//      b. Parse length expression via ParseExpr
//      c. Expect closing "]" bracket
//      d. Advance past "]"
//      e. Construct TypeArray node
//   5. If no semicolon, fall through to slice parsing
//
// ERROR HANDLING:
// -----------------------------------------------------------------------------
//   - If closing "]" is missing after length expression:
//     - Emit syntax error diagnostic
//     - Return error type (TypePrim with "!")
//
// DEPENDENCIES:
// =============================================================================
// - ParseType function (recursive call for element type)
// - ParseExpr function (for length expression)
// - MakeTypeNode helper (type_common.cpp)
// - MakeTypePrim helper (type_common.cpp)
// - TypeArray AST node type
// - Token predicates: IsPuncTok, IsPunc
// - Parser utilities: Tok, Advance, SpanBetween, TokSpan
// - EmitParseSyntaxErr diagnostic helper
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
// - Array and slice types share the "[" prefix; semicolon distinguishes them
// - Consider extracting the bracket-type parsing into a shared helper
// - Length expression is parsed as a full expression (could be N, N+1, etc.)
// - The length expression should be validated during semantic analysis
// - Span covers from "[" to "]" inclusive
// =============================================================================
