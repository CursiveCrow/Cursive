// =============================================================================
// MIGRATION MAPPING: expr/transmute_expr.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.5: Memory and Pointers - Transmute
//   - transmute expression typing
//   - Unsafe context requirement
//   - Bit pattern reinterpretation
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_infer.cpp
//   Transmute expression typing
//
// KEY CONTENT TO MIGRATE:
//   TRANSMUTE EXPRESSION (transmute<From, To>(expr)):
//   1. Verify inside unsafe context
//   2. Type expression against From type
//   3. Verify size compatibility (From and To same size)
//   4. Result is To type
//
//   TYPING:
//   InUnsafe = true
//   Gamma |- expr : From
//   Size(From) = Size(To)
//   --------------------------------------------------
//   Gamma |- transmute<From, To>(expr) : To
//
//   UNSAFE REQUIREMENT:
//   - transmute only valid in unsafe blocks
//   - Error outside unsafe context
//
//   SIZE COMPATIBILITY:
//   - From and To must have same size
//   - No implicit padding/truncation
//   - Compile-time size check
//
//   USE CASES:
//   - Float to bits: transmute<f32, u32>(f)
//   - Pointer casts: transmute<*imm A, *imm B>(p)
//   - Union access (unsafe)
//
//   NO SEMANTIC PRESERVATION:
//   - Only bit pattern preserved
//   - No type conversion logic
//   - Programmer ensures validity
//
//   EXAMPLE:
//   unsafe {
//       let bits: u32 = transmute<f32, u32>(3.14f)
//       let f: f32 = transmute<u32, f32>(bits)
//   }
//
// DEPENDENCIES:
//   - UnsafeContext verification
//   - ExprTypeFn for source typing
//   - LowerType for From, To types
//   - SizeOf for size compatibility
//
// SPEC RULES:
//   - Transmute semantics
//   - Unsafe context requirement
//   - Size compatibility rule
//
// RESULT TYPE:
//   To type
//
// NOTES:
//   - Extremely unsafe - no guarantees
//   - Size must match exactly
//   - Use sparingly and carefully
//
// =============================================================================

