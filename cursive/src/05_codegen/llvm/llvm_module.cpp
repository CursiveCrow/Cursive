// =============================================================================
// MIGRATION MAPPING: llvm_module.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   - Section 6.12 LLVM 21 Backend Requirements (lines 17287-17650)
//   - Section 6.12.1 LLVM Module Header (lines 17289-17291)
//   - LLVMHeader = [TargetDataLayout, TargetTriple]
//   - Target: x86_64-pc-windows-msvc
//   - DataLayout: Win64 model
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/llvm/llvm_module.cpp
//   - Lines 1-55: LLVMEmitter constructor, SetupModule
//   - Lines 22-28: Constructor initializes context, module, builder
//   - Lines 35-44: SetupModule sets target triple and data layout
//   - Lines 46-100: Helper functions (BuildScope, CreateEntryAlloca, ByteGEP, etc.)
//
// DEPENDENCIES:
//   - cursive/include/05_codegen/llvm/llvm_module.h
//   - cursive/include/05_codegen/llvm/llvm_emit.h (LLVMEmitter)
//   - llvm/IR/Module.h
//   - llvm/IR/LLVMContext.h
//   - llvm/IR/IRBuilder.h
//   - llvm/TargetParser/Triple.h
//
// REFACTORING NOTES:
//   1. LLVMEmitter owns module, context ref, and IRBuilder
//   2. SetupModule configures target-specific settings
//   3. Target triple: x86_64-pc-windows-msvc
//   4. Data layout: Win64 ABI model
//   5. Helper functions for memory operations
//   6. CreateEntryAlloca for stack allocation
//   7. ByteGEP for byte-level pointer arithmetic
//   8. StoreAtOffset/LoadAtOffset for struct field access
//
// MODULE SETUP:
//   LLVMEmitter(ctx, module_name):
//     context_ = ctx
//     module_ = new Module(module_name, ctx)
//     builder_ = new IRBuilder<>(ctx)
//
//   SetupModule():
//     module_->setTargetTriple("x86_64-pc-windows-msvc")
//     module_->setDataLayout("e-m:w-p270:32:32-...")
//
// HELPER FUNCTIONS:
//   CreateEntryAlloca(func, ty, name) -> AllocaInst*
//   ByteGEP(emitter, builder, base, offset) -> Value*
//   StoreAtOffset(emitter, builder, base, offset, value) -> void
//   LoadAtOffset(emitter, builder, base, offset, ty) -> Value*
// =============================================================================

#include "cursive/05_codegen/llvm/llvm_module.h"

// TODO: Migrate implementation from cursive-bootstrap/src/04_codegen/llvm/llvm_module.cpp

