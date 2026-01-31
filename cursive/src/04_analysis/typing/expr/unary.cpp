// =============================================================================
// MIGRATION MAPPING: expr/unary.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.2.13: Operators (search for unary operator rules)
//   - Negation: - on signed numeric types
//   - Logical not: ! on bool
//   - Bitwise not: ~ on integer types (if supported)
//   - Dereference: * on pointer types (see deref.cpp)
//   - Address-of: & on place expressions (see addr_of.cpp)
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_expr.cpp
//   UnaryExpr typing
//
// KEY CONTENT TO MIGRATE:
//   NEGATION (-):
//   - Operand must be signed integer or float
//   - Result type is same as operand type
//   - Unsigned integers cannot be negated
//
//   LOGICAL NOT (!):
//   - Operand must be bool
//   - Result is bool
//
//   BITWISE NOT (~):
//   - Operand must be integer type
//   - Result type is same as operand type
//   - (Check if ~ is supported in Cursive0)
//
//   TYPE CHECKING:
//   - Check operand type against operator requirements
//   - Return appropriate result type
//   - Error for unsupported operand types
//
// DEPENDENCIES:
//   - ExprTypeFn for operand typing
//   - IsSignedIntType(), IsFloatType(), IsBoolType(), IsIntType()
//   - MakeTypePrim() for result types
//
// NOTE: Dereference (*) and Address-of (&) are in separate files
//       due to their complexity and place expression handling.
//
// SPEC RULES (approximate):
//   - T-Neg: Numeric negation
//   - T-Not: Logical negation
//   - T-BitNot: Bitwise complement (if supported)
//
// RESULT TYPE:
//   ExprTypeResult { bool ok; optional<string_view> diag_id; TypeRef type; }
//
// ERROR CASES:
//   - Negation of unsigned integer
//   - Negation of non-numeric type
//   - Logical not of non-bool
//
// =============================================================================

