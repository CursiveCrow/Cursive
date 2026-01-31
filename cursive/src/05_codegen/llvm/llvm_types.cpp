// =============================================================================
// MIGRATION MAPPING: llvm_types.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   - Section 6.12.3 LLVMTy Judgment (lines 17416-17560)
//   - LLVMTy-Prim rule (lines 17456-17459)
//   - LLVMTy-Perm rule (lines 17461-17464)
//   - LLVMTy-Ptr rule (lines 17466-17469)
//   - LLVMTy-RawPtr rule (lines 17471-17474)
//   - LLVMTy-Func rule (lines 17476-17479)
//   - LLVMTy-Record rule (lines 17493-17496)
//   - LLVMTy-Tuple rule (lines 17498-17501)
//   - LLVMTy-Array rule (lines 17503-17506)
//   - LLVMTy-Slice, LLVMTy-Union, LLVMTy-Modal, etc.
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/llvm/llvm_types.cpp
//   - Lines 1-100: Type mapping helpers
//   - Lines 19-39: BuildScope, IsUnitType helpers
//   - Lines 41-50: AlignUp helper
//   - Lines 52-98: AppendPad, AppendStructElems
//   - Lines 100+: GetLLVMType implementation
//
// DEPENDENCIES:
//   - cursive/include/05_codegen/llvm/llvm_types.h
//   - cursive/include/05_codegen/llvm/llvm_emit.h (LLVMEmitter)
//   - cursive/include/05_codegen/layout/layout.h (SizeOf, AlignOf, RecordLayoutOf)
//   - llvm/IR/DerivedTypes.h
//   - llvm/IR/Type.h
//
// REFACTORING NOTES:
//   1. GetLLVMType maps Cursive types to LLVM types
//   2. Primitives map directly (i32 -> i32, etc.)
//   3. Records/Tuples become LLVM structs with padding
//   4. Arrays become LLVM arrays
//   5. Pointers use opaque ptr in LLVM 21
//   6. Functions become LLVM function types
//   7. Unions use tagged struct layout
//   8. Modals use tagged/niche layout
//
// TYPE MAPPING:
//   TypePrim("i32") -> IntegerType::getInt32Ty
//   TypePrim("f64") -> Type::getDoubleTy
//   TypePrim("bool") -> IntegerType::getInt8Ty
//   TypePrim("()") -> StructType::get({})
//   TypePtr/TypeRawPtr -> PointerType (opaque)
//   TypeFunc -> FunctionType
//   TypeRecord -> StructType with padding
//   TypeTuple -> StructType with padding
//   TypeArray -> ArrayType
//   TypeSlice -> StructType { ptr, usize }
//   TypeUnion -> Tagged struct or niche
//   TypeModal -> Tagged struct or niche
//
// STRUCT LAYOUT:
//   AppendStructElems builds struct with explicit padding:
//   1. For each field at offset:
//      - Add padding bytes if needed
//      - Add field type
//   2. Add tail padding to reach total size
// =============================================================================

#include "cursive/05_codegen/llvm/llvm_types.h"

// TODO: Migrate implementation from cursive-bootstrap/src/04_codegen/llvm/llvm_types.cpp

