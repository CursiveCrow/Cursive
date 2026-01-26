#include "cursive0/codegen/llvm/llvm_emit.h"
#include "cursive0/core/assert_spec.h"
#include "cursive0/codegen/ir_model.h"
#include "cursive0/codegen/layout/layout.h"
#include "cursive0/runtime/runtime_interface.h"

// LLVM Includes
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace cursive0::codegen {

// T-LLVM-010: Binding Storage and Validity

void LLVMEmitter::EmitBindVar(const IRBindVar& bind) {
    SPEC_RULE("BindSlot-Local");
    SPEC_RULE("BindValid-Init");
    SPEC_RULE("UpdateValid-BindVar");

    auto* builder = static_cast<llvm::IRBuilder<>*>(builder_.get());
    const bool debug_obj = std::getenv("CURSIVE0_DEBUG_OBJ") != nullptr;

    llvm::Function* func = builder->GetInsertBlock()->getParent();
    llvm::IRBuilder<> entry_builder(&func->getEntryBlock(), func->getEntryBlock().begin());

    analysis::TypeRef bind_type = bind.type;
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

    analysis::ProvenanceKind prov = bind.prov;
    std::optional<std::string> prov_region = bind.prov_region;
    if (prov == analysis::ProvenanceKind::Bottom && current_ctx_) {
        if (const auto* state = current_ctx_->GetBindingState(bind.name)) {
            prov = state->prov;
            if (!prov_region.has_value()) {
                prov_region = state->prov_region;
            }
        }
    }
    const bool use_region = prov == analysis::ProvenanceKind::Region;

    const auto* derived = current_ctx_ ? current_ctx_->LookupDerivedValue(bind.value) : nullptr;
    if (derived && derived->kind == DerivedValueInfo::Kind::LoadFromAddr) {
        llvm::Value* addr = EvaluateIRValue(derived->base);
        if (addr) {
            if (current_ctx_ && bind_type) {
                IRValue local_val;
                local_val.kind = IRValue::Kind::Local;
                local_val.name = bind.name;
                current_ctx_->RegisterValueType(local_val, bind_type);
            }
            SetLocal(bind.name, addr);
            if (current_ctx_ && bind_type) {
                current_ctx_->RegisterVar(bind.name, bind_type, true, false, prov,
                                          prov_region);
            }
            return;
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

    llvm::Value* slot = nullptr;
    if (use_region) {
        SPEC_RULE("BindSlot-Region");
        std::optional<IRValue> region_value;
        if (prov_region.has_value()) {
            IRValue region_local;
            region_local.kind = IRValue::Kind::Local;
            region_local.name = *prov_region;
            region_value = region_local;
        } else if (const IRValue* active_region = CurrentActiveRegion()) {
            region_value = *active_region;
        }

        if (!region_value.has_value()) {
            if (current_ctx_) {
                current_ctx_->ReportCodegenFailure();
            }
            return;
        }

        llvm::Value* region_val = EvaluateIRValue(*region_value);
        if (!region_val) {
            if (current_ctx_) {
                current_ctx_->ReportCodegenFailure();
            }
            return;
        }

        analysis::ScopeContext scope;
        if (current_ctx_) {
            if (current_ctx_->sigma) {
                scope.sigma = *current_ctx_->sigma;
            }
            scope.current_module = current_ctx_->module_path;
        }
        const auto size_opt = SizeOf(scope, bind_type);
        const auto align_opt = AlignOf(scope, bind_type);
        if (!size_opt.has_value() || !align_opt.has_value()) {
            if (current_ctx_) {
                current_ctx_->ReportCodegenFailure();
            }
            return;
        }

        llvm::Type* usize_ty = GetLLVMType(analysis::MakeTypePrim("usize"));
        llvm::Value* size_val =
            llvm::ConstantInt::get(usize_ty, static_cast<uint64_t>(*size_opt));
        llvm::Value* align_val =
            llvm::ConstantInt::get(usize_ty, static_cast<uint64_t>(*align_opt));

        std::string sym = RegionSymAlloc();
        if (auto alias = LookupSymbolAlias(sym)) {
            sym = *alias;
        }
        llvm::Function* alloc_fn = GetFunction(sym);
        if (!alloc_fn) {
            alloc_fn = module_->getFunction(sym);
        }
        if (!alloc_fn) {
            if (current_ctx_) {
                current_ctx_->ReportCodegenFailure();
            }
            return;
        }

        slot = builder->CreateCall(alloc_fn, {region_val, size_val, align_val});
    } else {
        llvm::AllocaInst* alloca = entry_builder.CreateAlloca(llvm_ty, nullptr, bind.name);

        // Zero-initialize to avoid poison/undef in partially-initialized aggregates.
        llvm::Value* zero = llvm::Constant::getNullValue(llvm_ty);
        builder->CreateStore(zero, alloca);
        slot = alloca;
    }

    if (val) {
        if (val->getType() != llvm_ty) {
            if (val->getType()->isPointerTy() && llvm_ty->isPointerTy()) {
                val = builder->CreateBitCast(val, llvm_ty);
            } else {
                if (debug_obj) {
                    std::string expected_str;
                    llvm::raw_string_ostream expected_os(expected_str);
                    llvm_ty->print(expected_os);
                    expected_os.flush();

                    std::string actual_str;
                    llvm::raw_string_ostream actual_os(actual_str);
                    val->getType()->print(actual_os);
                    actual_os.flush();

                    std::string bind_type_str = bind_type ? analysis::TypeToString(bind_type) : "<null>";
                    std::string value_type_str = "<null>";
                    if (current_ctx_) {
                        if (auto inferred = current_ctx_->LookupValueType(bind.value)) {
                            value_type_str = analysis::TypeToString(inferred);
                        }
                    }
                    std::cerr << "[cursivec0] bind type mismatch for `" << bind.name << "`\n";
                    std::cerr << "  bind.type: " << bind_type_str << "\n";
                    std::cerr << "  value.type: " << value_type_str << "\n";
                    std::cerr << "  expected llvm: " << expected_str << "\n";
                    std::cerr << "  actual llvm: " << actual_str << "\n";
                }
                if (current_ctx_) {
                    current_ctx_->ReportCodegenFailure();
                }
            }
        }
        builder->CreateStore(val, slot);
    }

    if (current_ctx_ && bind_type) {
        IRValue local_val;
        local_val.kind = IRValue::Kind::Local;
        local_val.name = bind.name;
        current_ctx_->RegisterValueType(local_val, bind_type);
        current_ctx_->RegisterVar(bind.name, bind_type, true, false, prov,
                                  prov_region);
    }
    SetLocal(bind.name, slot);
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

void EmitDropIfValid(LLVMEmitter& emitter, const std::string& name, analysis::TypeRef type) {
    SPEC_RULE("BindValid-DropIfValid");
    // For bootstrap, always emit drop (conservative)
    // A full implementation would check validity flag first
}

} // namespace cursive0::codegen
