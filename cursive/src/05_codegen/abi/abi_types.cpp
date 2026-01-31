// =============================================================================
// MIGRATION MAPPING: abi_types.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   - Section 6.2.2 ABI Type Lowering (lines 15162-15245)
//   - ABIType = {size, align} (line 15164)
//   - ABI-Prim rule (lines 15167-15170)
//   - ABI-Perm rule (lines 15172-15175)
//   - ABI-Ptr rule (lines 15177-15180)
//   - ABI-RawPtr rule (lines 15182-15185)
//   - ABI-Func rule (lines 15187-15190)
//   - ABI-Alias rule (lines 15192-15195)
//   - ABI-Record rule (lines 15197-15200)
//   - ABI-Tuple rule (lines 15202-15205)
//   - ABI-Array rule (lines 15207-15210)
//   - ABI-Slice rule (lines 15212-15215)
//   - ABI-Range rule (lines 15217-15220)
//   - ABI-Enum rule (lines 15222-15225)
//   - ABI-Union rule (lines 15227-15230)
//   - ABI-Modal rule (lines 15232-15235)
//   - ABI-Dynamic rule (lines 15237-15240)
//   - ABI-StringBytes rule (lines 15242-15245)
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/abi/abi_types.cpp
//   - ABITy function (type visitor dispatch)
//
// DEPENDENCIES:
//   - cursive/include/05_codegen/abi/abi.h (ABIType struct)
//   - cursive/include/05_codegen/layout/layout.h (SizeOf, AlignOf)
//   - cursive/include/04_analysis/types/types.h (all type variants)
//
// REFACTORING NOTES:
//   1. ABITy maps Cursive types to ABI representation
//   2. Most types: ABITy(T) = {sizeof(T), alignof(T)}
//   3. Permissions stripped: ABITy(TypePerm(p, T)) = ABITy(T)
//   4. All pointer-like types: {PtrSize, PtrAlign}
//   5. Function types treated as function pointers
//   6. Type aliases resolved before ABI computation
//   7. Used for parameter/return classification
//
// ABI TYPE CATEGORIES:
//   - Primitives: direct size/align
//   - Pointers/Funcs: PtrSize x PtrAlign
//   - Aggregates: computed layout size/align
//   - Strings/Bytes: state-dependent (View vs Managed)
// =============================================================================

#include "cursive/05_codegen/abi/abi.h"

// TODO: Migrate implementation from cursive-bootstrap/src/04_codegen/abi/abi_types.cpp
