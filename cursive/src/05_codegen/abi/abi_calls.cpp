// =============================================================================
// MIGRATION MAPPING: abi_calls.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   - Section 6.2.3 ABI-Call rule (lines 15282-15285)
//   - Section 6.2.4 Call Lowering for Procedures and Methods (lines 15303-15391)
//   - LowerCallJudg (line 15305)
//   - MethodSymbol rules (lines 15309-15332)
//   - BuiltinMethodSym rules (lines 15334-15342)
//   - Lower-Args rules (lines 15344-15356)
//   - Lower-RecvArg rules (lines 15358-15366)
//   - Lower-MethodCall rules (lines 15368-15386)
//   - SeqIR helper (lines 15388-15390)
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/abi/abi_calls.cpp
//   - ABICall function (lines 199-226 in abi_params.cpp)
//
// RELATED SOURCE FILES:
//   - cursive-bootstrap/src/04_codegen/lower/lower_expr_calls.cpp
//
// DEPENDENCIES:
//   - cursive/include/05_codegen/abi/abi.h (ABICallInfo)
//   - ABIParam, ABIRet functions
//   - Method lookup and symbol resolution
//
// REFACTORING NOTES:
//   1. ABICall computes PassKind for all params and return
//   2. Returns ABICallInfo with:
//      - param_kinds: vector of PassKind
//      - ret_kind: PassKind for return
//      - has_sret: true if ret_kind == SRet
//   3. Call lowering transforms procedure calls to IR:
//      - Lower receiver (base expression)
//      - Lower arguments
//      - Emit call with correct ABI
//   4. MethodCall lowering handles:
//      - Static dispatch (regular methods)
//      - Dynamic dispatch (class methods on $Class)
//      - Capability methods (FileSystem, HeapAllocator)
//   5. Panic out-parameter appended when NeedsPanicOut
//
// CALL LOWERING STEPS:
//   1. Evaluate receiver expression
//   2. Evaluate arguments left-to-right
//   3. Resolve method symbol
//   4. Emit CallIR or CallVTableIR
//   5. Handle panic check after call
// =============================================================================

#include "cursive/05_codegen/abi/abi.h"

// TODO: Migrate implementation from cursive-bootstrap/src/04_codegen/abi/abi_calls.cpp
