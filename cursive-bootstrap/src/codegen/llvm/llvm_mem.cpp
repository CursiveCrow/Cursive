#include "cursive0/codegen/llvm/llvm_emit.h"
#include "cursive0/core/assert_spec.h"

// LLVM Includes
#include "llvm/IR/IRBuilder.h"

namespace cursive0::codegen {

// T-LLVM-005: Memory Intrinsics

void LLVMEmitter::EmitMemCpy(llvm::Value* dst, llvm::Value* src, llvm::Value* size, uint64_t align) {
    SPEC_RULE("MemIntrinsics-Copy");
    
    // AggMemcpy overlap is unknown per spec -> use memmove.
    auto* builder = static_cast<llvm::IRBuilder<>*>(builder_.get());
    builder->CreateMemMove(dst, llvm::MaybeAlign(align), src, llvm::MaybeAlign(align), size);
}

void LLVMEmitter::EmitMemSet(llvm::Value* dst, llvm::Value* val, llvm::Value* size, uint64_t align) {
    SPEC_RULE("MemIntrinsics-Set");
    
    auto* builder = static_cast<llvm::IRBuilder<>*>(builder_.get());
    builder->CreateMemSet(dst, val, size, llvm::MaybeAlign(align));
}

void LLVMEmitter::EmitMemMove(llvm::Value* dst, llvm::Value* src, llvm::Value* size, uint64_t align) {
    SPEC_RULE("MemIntrinsics-Move");
    
    auto* builder = static_cast<llvm::IRBuilder<>*>(builder_.get());
    builder->CreateMemMove(dst, llvm::MaybeAlign(align), src, llvm::MaybeAlign(align), size);
}

} // namespace cursive0::codegen
