// =============================================================================
// MIGRATION MAPPING: llvm_attr.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   - Section 6.12 LLVM 21 Backend Requirements
//   - LLVM-PtrAttrs-* rules (attribute emission)
//   - Pointer attribute inference from type
//   - Function attribute mapping
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/llvm/llvm_attrs.cpp
//   - Lines 1-100: Attribute helper functions
//   - Lines 15-61: AddArgAttrs, AddPtrAttrs
//   - Lines 46-61: AddArgAttrs for alias/readonly
//   - Lines 63-98: AddPtrAttrs for nonnull/noundef/dereferenceable
//
// DEPENDENCIES:
//   - cursive/include/05_codegen/llvm/llvm_attr.h
//   - cursive/include/05_codegen/llvm/llvm_emit.h (LLVMEmitter)
//   - cursive/include/05_codegen/layout/layout.h (SizeOf, AlignOf)
//   - llvm/IR/Attributes.h
//   - llvm/IR/Function.h
//
// REFACTORING NOTES:
//   1. LLVM attributes encode optimization hints
//   2. Permission-based attributes:
//      - unique -> noalias
//      - const -> readonly
//   3. Pointer state attributes:
//      - Valid -> nonnull, noundef
//      - Size known -> dereferenceable(size)
//      - Align known -> align(align)
//   4. Raw pointers get no special attributes
//   5. Null/Expired pointers get no attributes
//
// ATTRIBUTE INFERENCE:
//   AddArgAttrs(builder, type):
//     If Ptr or Func type:
//       If unique: add NoAlias
//       If const: add ReadOnly
//
//   AddPtrAttrs(builder, type, scope):
//     If Valid Ptr:
//       Add NonNull, NoUndef
//       If size known: add Dereferenceable(size)
//       If align known: add Alignment(align)
//     If RawPtr: no attributes
//     If Null/Expired: no attributes
//
// FUNCTION ATTRIBUTES:
//   - nounwind (no exceptions)
//   - mustprogress (forward progress)
//   - willreturn (returns on all paths)
// =============================================================================

#include "cursive/05_codegen/llvm/llvm_attr.h"

// TODO: Migrate implementation from cursive-bootstrap/src/04_codegen/llvm/llvm_attrs.cpp

