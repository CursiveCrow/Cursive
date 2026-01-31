// =============================================================================
// MIGRATION MAPPING: expr/cast.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.2.13: Operators - Cast expressions (as)
//   - Numeric casts between int/float types
//   - Pointer casts in unsafe context
//   - No implicit casts (explicit required)
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_expr.cpp
//   CastExpr typing
//
// KEY CONTENT TO MIGRATE:
//   NUMERIC CASTS:
//   - Integer to integer (different widths)
//   - Float to float (different widths)
//   - Integer to float
//   - Float to integer (truncation)
//   - Always explicit via 'as' operator
//
//   VALID CAST PAIRS:
//   - i8..i128, u8..u128, isize, usize: any to any
//   - f16, f32, f64: any to any
//   - int <-> float: allowed
//   - bool, char: cannot cast to/from
//
//   POINTER CASTS (unsafe):
//   - Raw pointer type conversions
//   - Requires unsafe context
//   - *imm T as *mut T: qual change (unsafe)
//   - *T as *U: type change (unsafe)
//   - integer as *T: integer to pointer (unsafe)
//   - *T as usize: pointer to integer (unsafe)
//
// DEPENDENCIES:
//   - ExprTypeFn for source expression
//   - LowerType() for target type
//   - IsNumericType(), IsIntType(), IsFloatType()
//   - IsRawPtrType() for pointer detection
//   - IsInUnsafeSpan() for unsafe context
//
// SPEC RULES (approximate):
//   - T-Cast-Numeric: Numeric type conversion
//   - T-Cast-Ptr-Unsafe: Pointer cast in unsafe
//   - Cast-Invalid-Err: Invalid cast pair
//   - Cast-Ptr-NotUnsafe-Err: Pointer cast outside unsafe
//
// RESULT TYPE:
//   ExprTypeResult { bool ok; optional<string_view> diag_id; TypeRef type; }
//   (type is the target type if cast is valid)
//
// ERROR CASES:
//   - Invalid cast pair (e.g., bool to int)
//   - Pointer cast outside unsafe
//   - Cast to/from incompatible types
//
// =============================================================================

