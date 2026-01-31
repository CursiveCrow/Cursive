// =============================================================================
// MIGRATION MAPPING: expr/binary.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.2.13: Operators (search for binary operator rules)
//   - Arithmetic: +, -, *, /, %, ** on numeric types
//   - Comparison: ==, !=, <, <=, >, >= return bool
//   - Logical: &&, || on bool, short-circuit
//   - Bitwise: &, |, ^, <<, >> on integer types
//   - Range: .., ..= produce TypeRange
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_expr.cpp
//   BinaryExpr typing
//
// KEY CONTENT TO MIGRATE:
//   ARITHMETIC OPERATORS:
//   - Both operands must be same numeric type
//   - Result type is same as operand type
//   - No implicit numeric coercion (i32 + i64 is error)
//   - ** (power) may have different rules
//
//   COMPARISON OPERATORS:
//   - Operands must be compatible types
//   - Result is always bool
//   - Equality (==, !=) requires Eq class
//   - Ordering (<, <=, >, >=) requires Comparable class
//
//   LOGICAL OPERATORS:
//   - Both operands must be bool
//   - Result is bool
//   - Short-circuit evaluation (semantics, not typing)
//
//   BITWISE OPERATORS:
//   - Both operands must be same integer type
//   - Result type is same as operand type
//   - Shift amount must be non-negative (runtime check)
//
//   RANGE OPERATORS:
//   - .. creates exclusive range
//   - ..= creates inclusive range
//   - Operands must be same integer type
//   - Result is TypeRange (internal type)
//
// DEPENDENCIES:
//   - ExprTypeFn for operand typing
//   - TypeEquiv() for type matching
//   - IsNumericType(), IsIntType(), IsBoolType() helpers
//   - MakeTypePrim() for result types
//   - MakeTypeRange() for range result
//
// SPEC RULES (approximate):
//   - T-Add, T-Sub, T-Mul, T-Div, T-Mod, T-Pow: Arithmetic
//   - T-Eq, T-Ne, T-Lt, T-Le, T-Gt, T-Ge: Comparison
//   - T-And, T-Or: Logical
//   - T-BitAnd, T-BitOr, T-BitXor, T-Shl, T-Shr: Bitwise
//   - T-Range, T-RangeInclusive: Range
//
// RESULT TYPE:
//   ExprTypeResult { bool ok; optional<string_view> diag_id; TypeRef type; }
//
// ERROR CASES:
//   - Type mismatch between operands
//   - Non-numeric type for arithmetic
//   - Non-bool for logical
//   - Non-integer for bitwise
//
// =============================================================================

