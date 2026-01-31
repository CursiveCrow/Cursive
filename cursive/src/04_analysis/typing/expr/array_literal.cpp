// =============================================================================
// MIGRATION MAPPING: expr/array_literal.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.2.6: Arrays and Slices (lines 8922-9055)
//   - T-Array-Literal-List (lines 8926-8929):
//     All elements have type T => ArrayExpr([e1...en]) : TypeArray(T, n)
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/composite/arrays_slices.cpp
//   Array literal typing
//
// KEY CONTENT TO MIGRATE:
//   ARRAY LITERAL TYPING:
//   1. Type each element expression
//   2. All elements must have a common type T
//   3. Result type is [T; n] where n is element count
//
//   ELEMENT TYPE UNIFICATION:
//   - If expected type is known, check elements against it
//   - If synthesizing, infer common type from elements
//   - All elements must be compatible (subtype of common type)
//
//   ARRAY REPEAT SYNTAX:
//   - [value; count] syntax for repeated values
//   - count must be a constant expression (ConstLen)
//   - value type becomes element type
//
// DEPENDENCIES:
//   - ExprTypeFn for element typing
//   - Subtyping() for element compatibility
//   - ConstLen() for repeat count
//   - MakeTypeArray() for result construction
//
// SPEC RULES IMPLEMENTED:
//   - T-Array-Literal-List: Array from element list
//   - (Array repeat handled separately)
//
// RESULT TYPE:
//   ExprTypeResult { bool ok; optional<string_view> diag_id; TypeRef type; }
//
// ERROR CASES:
//   - Empty array without type annotation
//   - Incompatible element types
//   - Non-constant repeat count
//
// =============================================================================

