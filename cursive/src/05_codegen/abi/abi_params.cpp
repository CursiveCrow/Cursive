// =============================================================================
// MIGRATION MAPPING: abi_params.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   - Section 6.2.3 ABI Parameter and Return Passing (lines 15247-15301)
//   - PassKind = {ByValue, ByRef, SRet} (line 15249)
//   - ByValMax = 2 * PtrSize (line 15250)
//   - ByValAlign = PtrAlign (line 15251)
//   - ByValOk predicate (line 15252)
//   - ABI-Param-ByRef-Alias rule (lines 15257-15260)
//   - ABI-Param-ByValue-Move rule (lines 15262-15265)
//   - ABI-Param-ByRef-Move rule (lines 15267-15270)
//   - Panic Out-Parameter (lines 15287-15301)
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/abi/abi_params.cpp
//   - RuntimeSymbols set (lines 16-107)
//   - ByValOk function (lines 124-132)
//   - ABIParam function (lines 135-169)
//   - PanicRecordType function (lines 233-241)
//   - PanicOutType function (lines 244-246)
//   - NeedsPanicOut function (lines 249-268)
//   - PanicOutParams function (lines 271-281)
//
// DEPENDENCIES:
//   - cursive/include/05_codegen/abi/abi.h (PassKind enum, ABICallInfo)
//   - cursive/include/05_codegen/layout/layout.h (SizeOf, AlignOf)
//   - cursive/include/04_analysis/types/types.h (TypeRef, ParamMode)
//
// REFACTORING NOTES:
//   1. ByValOk(T) iff sizeof(T) <= 16 and alignof(T) <= 8
//   2. Non-move params (borrowed/aliased) always ByRef
//   3. Move params: ByValue if zero-sized or ByValOk, else ByRef
//   4. PanicOutParam = {move, "__panic", *mut PanicRecord}
//   5. PanicRecord = {panic: bool, code: u32}
//   6. NeedsPanicOut excludes: record ctors, entry sym, runtime symbols
//   7. Runtime symbols list is hardcoded for known runtime functions
//
// PARAMETER PASSING RULES:
//   - Borrowed (no mode): always by reference
//   - Move + small: by value
//   - Move + large: by reference
// =============================================================================

#include "cursive/05_codegen/abi/abi.h"

// TODO: Migrate implementation from cursive-bootstrap/src/04_codegen/abi/abi_params.cpp
