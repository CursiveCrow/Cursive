// =============================================================================
// MIGRATION MAPPING: item/extern_block.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 18: Foreign Function Interface
//   - extern block grammar
//   - FfiSafe types
//   - Foreign contracts (@foreign_assumes, @foreign_ensures)
//   - ABI strings ("C", "C-unwind", etc.)
//   - Capability isolation
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_decls.cpp
//   Extern block typing
//
// KEY CONTENT TO MIGRATE:
//   EXTERN BLOCK:
//   1. Parse ABI string ("C", "C-unwind", "system", etc.)
//   2. For each extern procedure:
//      a. Lower parameter types
//      b. Lower return type
//      c. Verify all types are FfiSafe
//      d. Process foreign contracts
//   3. Register extern procedures in environment
//
//   TYPING:
//   ABI string valid
//   All param types FfiSafe
//   Return type FfiSafe
//   Foreign contracts well-formed
//   --------------------------------------------------
//   Gamma |- extern "ABI" { decls }
//
//   FFISAFE TYPES:
//   - Primitives: i8-i128, u8-u128, isize, usize, f16, f32, f64, char, ()
//   - Raw pointers: *imm T, *mut T
//   - Arrays of FfiSafe types
//   - Records/Enums with [[layout(C)]]
//
//   NOT FFISAFE:
//   - bool (ABI-incompatible)
//   - Modal types
//   - Safe pointers Ptr<T>
//   - Dynamic class objects $ClassName
//   - Tuples, Unions, Slices
//   - String/bytes types
//   - Context
//
//   FOREIGN CONTRACTS:
//   |= @foreign_assumes(pred, ...)  - caller ensures
//   |= @foreign_ensures(pred)       - callee ensures on success
//   |= @foreign_ensures(@error: pred)  - callee ensures on error
//   |= @foreign_ensures(@null_result: pred)  - when result is null
//
//   CAPABILITY ISOLATION:
//   - Extern procedures CANNOT access Context
//   - Capabilities must be passed explicitly
//   - No capability-bearing values in signatures
//
//   CALL SAFETY:
//   - Calls to extern procedures require unsafe { }
//
// DEPENDENCIES:
//   - LowerType() for types
//   - FfiSafeCheck() for type validation
//   - ForeignContractTyping()
//   - ABIValidation()
//
// SPEC RULES:
//   - FfiSafe type rules
//   - Foreign contract semantics
//   - Capability isolation rules
//
// RESULT:
//   Extern procedures added to environment
//   Diagnostics for FfiSafe violations
//
// =============================================================================

