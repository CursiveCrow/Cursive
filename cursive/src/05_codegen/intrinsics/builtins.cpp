// =============================================================================
// MIGRATION MAPPING: builtins.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   - Built-in types and operations (throughout)
//   - Primitive arithmetic operations
//   - Boolean operations
//   - Comparison operations
//   - Bitwise operations
//   - String/bytes built-in methods
//   - Ptr built-in methods
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/ir_lowering.cpp
//   - Binary operation LLVM emission
//   - Unary operation LLVM emission
//   - Comparison operation handling
//   - Overflow checking for arithmetic
//
// DEPENDENCIES:
//   - cursive/include/05_codegen/intrinsics/builtins.h
//   - cursive/include/05_codegen/ir/ir_model.h
//   - cursive/include/04_analysis/types/types.h
//
// REFACTORING NOTES:
//   1. Built-in operations map directly to LLVM instructions
//   2. Signed vs unsigned affects comparison and division
//   3. Overflow behavior defined by spec (wrap, trap, or UB)
//   4. Float operations follow IEEE 754 semantics
//   5. Pointer arithmetic uses GEP
//
// ARITHMETIC BUILTINS:
//   i8-i128, u8-u128: +, -, *, /, %, ** (power)
//   f16, f32, f64: +, -, *, /, ** (power)
//
// COMPARISON BUILTINS:
//   All numeric: ==, !=, <, <=, >, >=
//   bool: ==, !=
//   char: ==, !=, <, <=, >, >=
//   Ptr: ==, != (pointer equality)
//
// BITWISE BUILTINS:
//   Integer types: &, |, ^, <<, >>
//   Shift amount must be < bit width
//
// BOOLEAN BUILTINS:
//   bool: &&, ||, ! (short-circuit for && and ||)
//
// STRING/BYTES BUILTINS:
//   length() -> usize
//   is_empty() -> bool
//   (other methods via class implementations)
//
// PTR BUILTINS:
//   null() -> Ptr<T>@Null
//   is_null() -> bool
//   is_valid() -> bool
// =============================================================================

#include "cursive/05_codegen/intrinsics/builtins.h"

// TODO: Implement built-in operation lowering

