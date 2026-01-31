// =============================================================================
// MIGRATION MAPPING: abi_lowering.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   - Section 6.2 ABI Lowering (Cursive0) (lines 15154-15391)
//   - Section 6.2.1 Default Calling Convention (lines 15156-15159)
//   - CallConvDefault = Cursive0ABI (line 15159)
//
// SOURCE FILE:
//   - cursive-bootstrap/src/04_codegen/llvm/llvm_call_abi.cpp (LLVM ABI lowering)
//
// DEPENDENCIES:
//   - cursive/include/05_codegen/abi/abi.h (ABI types and functions)
//   - ABITy, ABIParam, ABIRet, ABICall functions
//   - LLVM type generation
//
// REFACTORING NOTES:
//   1. ABI lowering transforms high-level calls to low-level IR
//   2. Cursive0ABI is the default calling convention
//   3. Lowering handles:
//      - Parameter passing (ByValue vs ByRef)
//      - Return value handling (ByValue vs SRet)
//      - Receiver transformations (self parameter)
//      - Panic out-parameter insertion
//   4. Coordinates between type layout and LLVM codegen
//   5. Must match runtime expectations for FFI
//
// LOWERING RESPONSIBILITIES:
//   - Convert Cursive procedure signatures to ABI signatures
//   - Insert hidden parameters (sret, panic out)
//   - Generate argument marshalling code
//   - Generate return value marshalling code
//   - Handle method receiver as first argument
//
// ABI SIGNATURE TRANSFORMATION:
//   Cursive: proc foo(a: T1, move b: T2) -> R
//   ABI: foo(sret_opt, a_ptr, b_val_or_ptr, panic_out) -> result_or_void
// =============================================================================

#include "cursive/05_codegen/abi/abi.h"

// TODO: Implement ABI lowering coordination
