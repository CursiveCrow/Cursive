// =============================================================================
// MIGRATION MAPPING: llvm_call.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   - Section 6.12.4 LLVM Call/Return Lowering (lines 17560-17600)
//   - LLVMCall-ByValue rule
//   - LLVMCall-SRet rule
//   - LLVMRetLower-SRet rule
//   - CallVTable for dynamic dispatch
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/llvm/llvm_call_abi.cpp
//   - Lines 1-100: ComputeCallABI implementation
//   - Lines 51-91: ABICall computation, error handling
//   - Lines 93-100+: Return type handling (SRet vs ByValue)
//
// DEPENDENCIES:
//   - cursive/include/05_codegen/llvm/llvm_call.h
//   - cursive/include/05_codegen/llvm/llvm_emit.h (LLVMEmitter)
//   - cursive/include/05_codegen/abi/abi_calls.h (ABICall, ABICallResult)
//   - llvm/IR/DerivedTypes.h
//   - llvm/IR/Function.h
//
// REFACTORING NOTES:
//   1. ComputeCallABI translates Cursive ABI to LLVM
//   2. Handles SRet (struct return) via hidden parameter
//   3. Handles ByValue for small types
//   4. Computes LLVM FunctionType from Cursive signature
//   5. Maps ParamMode (Move vs Copy) to ABI passing
//   6. Special handling for Valid pointers
//
// ABI CALL COMPUTATION:
//   ComputeCallABI(params, ret_type):
//     1. Build param list with modes
//     2. Call ABICall for ABI info
//     3. Determine return kind (SRet vs ByValue)
//     4. Build LLVM param types
//     5. Build LLVM function type
//     6. Return ABICallResult
//
// SRET HANDLING:
//   If ret_kind == SRet:
//     - Return type is void
//     - First parameter is sret pointer
//     - Caller allocates return space
//
// BYVALUE HANDLING:
//   Small types passed directly in registers
//   Large types may use pointer + byval attribute
// =============================================================================

#include "cursive/05_codegen/llvm/llvm_call.h"

// TODO: Migrate implementation from cursive-bootstrap/src/04_codegen/llvm/llvm_call_abi.cpp

