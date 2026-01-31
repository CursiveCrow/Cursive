// =============================================================================
// MIGRATION MAPPING: expr/literal.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.2.10: Literal Expressions (lines 9174-9285)
//   - T-Int-Literal-Suffix (lines 9210-9213)
//   - T-Int-Literal-Default (lines 9215-9218)
//   - T-Float-Literal-Explicit (lines 9220-9223)
//   - T-Float-Literal-Infer (lines 9225-9228)
//   - T-Bool-Literal (lines 9230-9233)
//   - T-Char-Literal (lines 9235-9238)
//   - T-String-Literal (lines 9240-9243)
//   - Syn-Literal (lines 9245-9248)
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/literals.cpp
//   TypeLiteralExpr() function (lines 277-352)
//
// CROSS-REFERENCE: See also ../literals.cpp for shared parsing logic
//
// KEY CONTENT TO MIGRATE:
//   LITERAL TYPE INFERENCE:
//   - IntLiteral with suffix: TypePrim(suffix) if InRange
//   - IntLiteral without suffix: TypePrim("i32") if InRange
//   - FloatLiteral with explicit suffix (f16/f32/f64): TypePrim(suffix)
//   - FloatLiteral with bare 'f': TypePrim("f32")
//   - BoolLiteral: TypePrim("bool")
//   - CharLiteral: TypePrim("char")
//   - StringLiteral: TypeString(View)
//   - NullLiteral: Error - requires type context
//
//   INTEGER HANDLING:
//   1. Parse literal value (handles 0x, 0o, 0b, decimal)
//   2. Check for suffix (i8..i128, u8..u128, isize, usize)
//   3. Validate value is in range for type
//   4. Return typed literal or error
//
//   FLOAT HANDLING:
//   1. Check for suffix (f16, f32, f64, f)
//   2. Bare 'f' suffix defaults to f32
//   3. No suffix is an error (lexer should reject, but handle gracefully)
//
// DEPENDENCIES:
//   - IntValue() from literals.cpp for integer parsing
//   - IntSuffix(), FloatSuffix() for suffix extraction
//   - InRangeInt() for range validation
//   - MakeTypePrim(), MakeTypeString() for type construction
//
// SPEC RULES IMPLEMENTED:
//   - T-Int-Literal-Suffix: Integer with explicit suffix
//   - T-Int-Literal-Default: Integer defaults to i32
//   - T-Float-Literal-Explicit: Float with f16/f32/f64 suffix
//   - T-Float-Literal-Infer: Float with bare 'f' -> f32
//   - T-Bool-Literal, T-Char-Literal, T-String-Literal
//   - Syn-Literal: Literal synthesis rule
//
// RESULT TYPE:
//   ExprTypeResult { bool ok; optional<string_view> diag_id; TypeRef type; }
//
// =============================================================================

