// =============================================================================
// MIGRATION MAPPING: expr/unsafe_block_expr.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.5: Memory and Pointers - Unsafe blocks
//   - unsafe block expression grammar
//   - Unsafe context for raw pointer operations
//   - LexSecure restrictions lifted in unsafe
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_infer.cpp
//   Unsafe block expression typing portions
//
// KEY CONTENT TO MIGRATE:
//   UNSAFE BLOCK EXPRESSION (unsafe { body }):
//   1. Push unsafe context
//   2. Type body in unsafe context
//   3. Pop unsafe context
//   4. Result is body type
//
//   TYPING:
//   InUnsafe' = true
//   Gamma; InUnsafe' |- body : T
//   --------------------------------------------------
//   Gamma |- unsafe { body } : T
//
//   UNSAFE CONTEXT ENABLES:
//   - Raw pointer dereference (*ptr where ptr: *imm T or *mut T)
//   - Calls to extern procedures
//   - transmute expressions
//   - Bidirectional control characters (LexSecure lifted)
//   - Manual memory operations
//
//   RAW POINTER OPERATIONS:
//   - *imm T: immutable raw pointer, can read
//   - *mut T: mutable raw pointer, can read/write
//   - Both may be null or invalid
//   - No automatic safety checks
//
//   TRANSMUTE:
//   - transmute<From, To>(expr) - reinterpret bit pattern
//   - Only valid in unsafe blocks
//   - No runtime checks
//
//   FFI CALLS:
//   - extern procedure calls require unsafe
//   - Foreign contracts may be assumed (unchecked)
//
// DEPENDENCIES:
//   - BlockTypeFn for body typing
//   - UnsafeContext tracking
//   - Raw pointer type handling
//
// SPEC RULES:
//   - Unsafe block semantics
//   - LexSecure restrictions (lifted in unsafe)
//
// RESULT TYPE:
//   Type of body expression
//
// NOTES:
//   - unsafe does not change the type, only enables operations
//   - Sound code must maintain invariants manually
//   - Used for FFI and low-level memory operations
//
// =============================================================================

