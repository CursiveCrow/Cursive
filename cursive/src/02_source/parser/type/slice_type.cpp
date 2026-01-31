// =============================================================================
// MIGRATION MAPPING: slice_type.cpp
// =============================================================================
// This file should contain parsing logic for slice types: [T] which represents
// a dynamically-sized view into a contiguous sequence of T.
//
// SPEC REFERENCE: C0updated.md, Section 3.3.7, Lines 4768-4771
// =============================================================================
//
// FORMAL RULES FROM SPEC:
// -----------------------------------------------------------------------------
//
// **(Parse-Slice-Type)** Lines 4768-4771
// IsPunc(Tok(P), "[")
// Γ ⊢ ParseType(Advance(P)) ⇓ (P_1, t)
// IsPunc(Tok(P_1), "]")
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseNonPermType(P) ⇓ (Advance(P_1), TypeSlice(t))
//
// SEMANTICS:
// - Slice types represent a view into contiguous memory
// - Syntax: [ElementType]
// - No length - dynamically sized at runtime
// - Distinguishable from arrays by absence of semicolon
// - Examples: [i32], [u8], [Point]
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_types.cpp
// =============================================================================
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
//
// 1. Slice type parsing in ParseNonPermType (Lines 481-491)
//    ─────────────────────────────────────────────────────────────────────────
//    // Continuation from array_type.cpp - inside the "[" branch
//    if (IsPunc(elem.parser, "]")) {
//      SPEC_RULE("Parse-Slice-Type");
//      Parser after_r = elem.parser;
//      Advance(after_r);
//      TypeSlice slice;
//      slice.element = elem.elem;
//      return {after_r, MakeTypeNode(SpanBetween(start, after_r), slice)};
//    }
//    EmitParseSyntaxErr(elem.parser, TokSpan(elem.parser));
//    return {elem.parser, MakeTypePrim(SpanBetween(start, elem.parser), "!")};
//
// PARSING FLOW:
// -----------------------------------------------------------------------------
//   1. Match opening "[" bracket (shared with array)
//   2. Advance past "["
//   3. Parse element type via ParseType
//   4. If immediately followed by "]" (no semicolon):
//      a. This is a slice type
//      b. Advance past "]"
//      c. Construct TypeSlice node
//   5. If followed by ";" then it's an array type (see array_type.cpp)
//
// ERROR HANDLING:
// -----------------------------------------------------------------------------
//   - If neither ";" nor "]" follows the element type:
//     - Emit syntax error diagnostic
//     - Return error type (TypePrim with "!")
//
// DEPENDENCIES:
// =============================================================================
// - ParseType function (recursive call for element type)
// - MakeTypeNode helper (type_common.cpp)
// - MakeTypePrim helper (type_common.cpp)
// - TypeSlice AST node type
// - Token predicates: IsPuncTok, IsPunc
// - Parser utilities: Tok, Advance, SpanBetween, TokSpan
// - EmitParseSyntaxErr diagnostic helper
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
// - Slice and array types share the "[" prefix parsing
// - The distinguishing factor is the presence of ";" for arrays
// - Consider combining array_type.cpp and slice_type.cpp into bracket_types.cpp
// - Slices are fat pointers (ptr + len) at runtime, but this is semantic info
// - Span covers from "[" to "]" inclusive
// =============================================================================
