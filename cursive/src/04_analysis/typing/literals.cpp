// =============================================================================
// MIGRATION MAPPING: literals.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.2.10: Literal Expressions (lines 9174-9285)
//   - IntTypes, FloatTypes sets (lines 9176-9177)
//   - FloatFormat, FloatBitWidth definitions (lines 9178-9179)
//   - IEEE754 encoding definitions (lines 9180-9186)
//   - DefaultInt = i32, DefaultFloat = f32 (lines 9187-9188)
//   - IntValue, FloatValue functions (line 9189)
//   - StripIntSuffix, StripFloatSuffix (lines 9193-9198)
//   - InRange, RangeOf definitions (lines 9204-9208)
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/literals.cpp
//   Lines 1-449 (complete file)
//
// KEY CONTENT TO MIGRATE:
//   SPEC DEFINITIONS (lines 17-29):
//   - SpecDefsLiterals() references "5.2.10"
//
//   CONSTANTS (lines 31-46):
//   - kPointerSizeBytes = 8, kPointerSizeBits = 64
//   - kIntSuffixes array: i128, u128, isize, usize, i64, u64, i32, u32, i16, u16, i8, u8
//   - kFloatSuffixes array: f16, f32, f64, f
//   - kIntTypes, kFloatTypes arrays
//
//   SUFFIX PARSING (lines 48-91):
//   - EndsWith() helper
//   - MatchSuffix() - find matching suffix
//   - IntSuffix() - extract integer suffix from token
//   - FloatSuffix() - extract float suffix from token
//   - IntCore() - strip suffix from integer literal
//
//   INTEGER PARSING (lines 93-183):
//   - DigitValue() - single digit value for any base
//   - ParseIntCore() - full integer parsing with UInt128
//     * Handles 0x, 0o, 0b prefixes
//     * Handles underscore separators
//     * Overflow detection
//   - IntValue() - parse token to UInt128
//
//   TYPE CHECKING (lines 185-273):
//   - IsIntTypeName(), IsFloatTypeName() - type name validation
//   - IntWidthOf() - bit width for type name
//   - IsUnsignedIntType(), IsSignedIntType()
//   - InRangeUnsigned(), InRangeSigned() - value range checks
//   - InRangeInt() - combined range check
//
//   LITERAL TYPING (lines 277-352):
//   - TypeLiteralExpr() main function
//     * IntLiteral: suffix or default i32 (T-Int-Literal-Suffix, T-Int-Literal-Default)
//     * FloatLiteral: suffix required, 'f' defaults to f32 (T-Float-Literal-Explicit, T-Float-Literal-Infer)
//     * BoolLiteral: bool type (T-Bool-Literal)
//     * CharLiteral: char type (T-Char-Literal)
//     * StringLiteral: string@View (T-String-Literal)
//
//   LITERAL CHECKING (lines 354-446):
//   - NullLiteralExpected() - check if null is valid for type
//   - CheckLiteralExpr() - check literal against expected type
//     * IntLiteral: check range for target type (Chk-Int-Literal)
//     * FloatLiteral: explicit suffix must match, 'f' accepts any (Chk-Float-Literal-*)
//     * NullLiteral: must expect raw pointer (Chk-Null-Literal)
//
// DEPENDENCIES:
//   - ScopeContext for context (currently unused)
//   - syntax::LiteralExpr, syntax::Token
//   - core::UInt128 for large integer support
//   - TypeRef and MakeType* constructors
//   - TypePrim, TypePerm, TypeRefine, TypeRawPtr type nodes
//
// REFACTORING NOTES:
//   1. UInt128 dependency is for i128/u128 literal support
//   2. Float suffix 'f' is special - accepts any float type from context
//   3. String literals always produce string@View (borrowed)
//   4. NullLiteral requires type context (raw pointer expected)
//   5. Consider separating:
//      - literal_parse.cpp (suffix extraction, value parsing)
//      - literal_type.cpp (type inference and checking)
//
// SPEC RULES IMPLEMENTED:
//   - T-Int-Literal-Suffix, T-Int-Literal-Default
//   - T-Float-Literal-Explicit, T-Float-Literal-Infer
//   - T-Bool-Literal, T-Char-Literal, T-String-Literal
//   - Chk-Int-Literal, Chk-Float-Literal-Explicit, Chk-Float-Literal-Infer
//   - Chk-Float-Literal-Mismatch-Err (E-TYP-1531)
//   - Chk-Null-Literal, Chk-Null-Literal-Err
//
// =============================================================================

