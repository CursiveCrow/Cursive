#include "cursive0/codegen/llvm_emit.h"
#include "cursive0/core/assert_spec.h"
#include "cursive0/codegen/ir_model.h"

// LLVM Includes
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"

namespace cursive0::codegen {

// T-LLVM-010: Binding Storage and Validity

void LLVMEmitter::EmitBindVar(const IRBindVar& bind) {
    SPEC_RULE("BindSlot-Local");
    SPEC_RULE("BindValid-Init");
    SPEC_RULE("UpdateValid-BindVar");

    auto* builder = static_cast<llvm::IRBuilder<>*>(builder_.get());

    llvm::Function* func = builder->GetInsertBlock()->getParent();
    llvm::IRBuilder<> entry_builder(&func->getEntryBlock(), func->getEntryBlock().begin());

    sema::TypeRef bind_type = bind.type;
    if (current_ctx_) {
        if (!bind_type) {
            if (const auto* state = current_ctx_->GetBindingState(bind.name)) {
                bind_type = state->type;
            }
        }
        if (!bind_type) {
            bind_type = current_ctx_->LookupValueType(bind.value);
        }
    }

    llvm::Value* val = EvaluateIRValue(bind.value);
    llvm::Type* llvm_ty = bind_type ? GetLLVMType(bind_type)
                                    : (val ? val->getType() : nullptr);

    if (!llvm_ty) {
        if (current_ctx_) {
            current_ctx_->ReportCodegenFailure();
        }
        return;
    }

    llvm::AllocaInst* alloca = entry_builder.CreateAlloca(llvm_ty, nullptr, bind.name);

    // Zero-initialize to avoid poison/undef in partially-initialized aggregates.
    llvm::Value* zero = llvm::Constant::getNullValue(llvm_ty);
    builder->CreateStore(zero, alloca);

    if (val) {
        if (val->getType() != llvm_ty) {
            if (val->getType()->isPointerTy() && llvm_ty->isPointerTy()) {
                val = builder->CreateBitCast(val, llvm_ty);
            } else {
                if (current_ctx_) {
                    current_ctx_->ReportCodegenFailure();
                }
            }
        }
        builder->CreateStore(val, alloca);
    }

    if (current_ctx_ && bind_type) {
        current_ctx_->RegisterVar(bind.name, bind_type, true, false);
    }
    SetLocal(bind.name, alloca);
}

// T-LLVM-010: Additional validity tracking functions

void EmitValidityCheck(LLVMEmitter& emitter, const std::string& name) {
    SPEC_RULE("BindValid-Check");
    // In a full implementation, this would check a validity flag
    // For bootstrap, we assume all bindings are valid
}

void EmitMarkMoved(LLVMEmitter& emitter, const std::string& name) {
    SPEC_RULE("BindValid-Move");
    // In a full implementation, this would set the validity flag to moved
    // For bootstrap, this is a no-op
}

void EmitMarkPartiallyMoved(LLVMEmitter& emitter, const std::string& name) {
    SPEC_RULE("BindValid-PartialMove");
    // For bootstrap, this is a no-op
}

void EmitClearMoved(LLVMEmitter& emitter, const std::string& name) {
    SPEC_RULE("BindValid-Reassign");
    // For bootstrap, this is a no-op
}

void EmitDropIfValid(LLVMEmitter& emitter, const std::string& name, sema::TypeRef type) {
    SPEC_RULE("BindValid-DropIfValid");
    // For bootstrap, always emit drop (conservative)
    // A full implementation would check validity flag first
}

} // namespace cursive0::codegen
