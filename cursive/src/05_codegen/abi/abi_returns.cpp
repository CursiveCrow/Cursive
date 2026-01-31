// =============================================================================
// MIGRATION MAPPING: abi_returns.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   - Section 6.2.3 ABI Return Passing (lines 15272-15280)
//   - ABI-Ret-ByValue rule (lines 15272-15275)
//   - ABI-Ret-ByRef rule (lines 15277-15280)
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/abi/abi_params.cpp
//   - ABIRet function (lines 172-196)
//
// DEPENDENCIES:
//   - cursive/include/05_codegen/abi/abi.h (PassKind enum)
//   - cursive/include/05_codegen/layout/layout.h (SizeOf, AlignOf)
//   - ByValOk predicate
//
// REFACTORING NOTES:
//   1. Return passing uses same ByValOk criteria as parameters
//   2. Zero-sized types always ByValue (no storage needed)
//   3. Small types (ByValOk) returned by value in registers
//   4. Large types use SRet (struct return pointer)
//   5. SRet adds hidden first parameter for return storage
//   6. Caller provides storage, callee writes to it
//
// RETURN ABI RULES:
//   - sizeof(T) == 0: ByValue (void-like)
//   - ByValOk(T): ByValue (register)
//   - else: SRet (pointer)
//
// SRET CONVENTION:
//   - Original: proc foo() -> LargeStruct
//   - ABI: proc foo(sret: *mut LargeStruct) -> ()
//   - Caller allocates, passes pointer
//   - Callee fills in return value
// =============================================================================

#include "cursive/05_codegen/abi/abi.h"

// TODO: Migrate implementation from cursive-bootstrap/src/04_codegen/abi/abi_params.cpp
