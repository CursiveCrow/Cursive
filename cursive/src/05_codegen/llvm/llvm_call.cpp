// =============================================================================
// MIGRATION MAPPING: llvm_call.cpp
// =============================================================================
//
// SPEC REFERENCE: CursiveSpecification.md
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
// =============================================================================

#include "05_codegen/llvm/llvm_call.h"

#include "00_core/spec_trace.h"
#include "00_core/symbols.h"
#include "05_codegen/abi/abi.h"
#include "05_codegen/checks/checks.h"
#include "05_codegen/layout/layout.h"
#include "05_codegen/llvm/llvm_emit.h"
#include "05_codegen/llvm/emit/internal_helpers.h"
#include "05_codegen/llvm/llvm_ir_panic.h"
#include "05_codegen/llvm/llvm_types.h"

#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"

#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

namespace cursive::codegen {


namespace {

const analysis::ScopeContext& BuildScope(const LowerCtx* ctx) {
  static const analysis::ScopeContext kEmptyScope{};

  struct ScopeCache {
    const LowerCtx* ctx = nullptr;
    const analysis::Sigma* sigma = nullptr;
    std::vector<std::string> module_path;
    analysis::ScopeContext scope;
  };

  thread_local ScopeCache cache;
  if (!ctx || !ctx->sigma) {
    return kEmptyScope;
  }

  if (cache.ctx != ctx || cache.sigma != ctx->sigma ||
      cache.module_path != ctx->module_path) {
    cache.ctx = ctx;
    cache.sigma = ctx->sigma;
    cache.module_path = ctx->module_path;
    cache.scope = analysis::ScopeContext{};
    cache.scope.sigma = *ctx->sigma;
    cache.scope.sigma_source = ctx->sigma;
    cache.scope.current_module = ctx->module_path;
  }

  return cache.scope;
}

analysis::TypeRef StripPermLocal(const analysis::TypeRef& type) {
  if (!type) {
    return type;
  }
  if (const auto* perm = std::get_if<analysis::TypePerm>(&type->node)) {
    return StripPermLocal(perm->base);
  }
  return type;
}

const char* UnwindPersonalitySymbolForModule(const llvm::Module& module) {
  const std::string triple = module.getTargetTriple().str();
  if (triple.find("windows-msvc") != std::string::npos) {
    return "__C_specific_handler";
  }
  return "__gxx_personality_v0";
}

}  // namespace

// =============================================================================
// §6.12.9 LLVM Call Signature Lowering
// =============================================================================

bool IsValidPtrType(const analysis::TypeRef& type) {
  const auto stripped = StripPermLocal(type);
  if (!stripped) {
    return false;
  }
  if (const auto* ptr = std::get_if<analysis::TypePtr>(&stripped->node)) {
    return ptr->state.has_value() && *ptr->state == analysis::PtrState::Valid;
  }
  return false;
}

llvm::AllocaInst* CreateEntryAlloca(llvm::Function* func,
                                    llvm::Type* ty,
                                    const std::string& name) {
  if (!func || !ty) {
    return nullptr;
  }
  llvm::IRBuilder<> entry_builder(&func->getEntryBlock(),
                                   func->getEntryBlock().begin());
  return entry_builder.CreateAlloca(ty, nullptr, name);
}

llvm::AllocaInst* AcquireReusableEntryAlloca(llvm::Function* func,
                                             llvm::Type* ty,
                                             std::string_view name,
                                             unsigned ordinal) {
  struct ScratchBucket {
    llvm::Type* ty = nullptr;
    std::string name;
    std::vector<llvm::AllocaInst*> slots;
  };

  struct ScratchCache {
    llvm::Function* func = nullptr;
    std::vector<ScratchBucket> buckets;
  };

  thread_local ScratchCache cache;
  if (cache.func != func) {
    cache.func = func;
    cache.buckets.clear();
  }

  for (auto& bucket : cache.buckets) {
    if (bucket.ty != ty || bucket.name != name) {
      continue;
    }
    while (bucket.slots.size() <= ordinal) {
      llvm::AllocaInst* slot =
          CreateEntryAlloca(func, ty, bucket.name);
      if (!slot) {
        return nullptr;
      }
      bucket.slots.push_back(slot);
    }
    return bucket.slots[ordinal];
  }

  ScratchBucket bucket;
  bucket.ty = ty;
  bucket.name = std::string(name);
  while (bucket.slots.size() <= ordinal) {
    llvm::AllocaInst* slot = CreateEntryAlloca(func, ty, bucket.name);
    if (!slot) {
      return nullptr;
    }
    bucket.slots.push_back(slot);
  }
  llvm::AllocaInst* result = bucket.slots[ordinal];
  cache.buckets.push_back(std::move(bucket));
  return result;
}

llvm::Value* CoerceValue(llvm::IRBuilderBase* builder_base,
                         llvm::Value* value,
                         llvm::Type* target) {
  if (!builder_base || !value || !target) {
    return value;
  }

  auto* builder = static_cast<llvm::IRBuilder<>*>(builder_base);

  if (value->getType() == target) {
    return value;
  }

  // Unit/zero-sized aggregate targets are represented as empty structs in the
  // current lowering pipeline. Coercion into unit should erase the value,
  // never emit a scalar->aggregate bitcast.
  if (const auto* struct_ty = llvm::dyn_cast<llvm::StructType>(target)) {
    if (struct_ty->getNumElements() == 0) {
      return llvm::Constant::getNullValue(target);
    }
  }

  // Coerce fixed-size arrays to slices/views represented as {ptr, len}.
  // This implements Coerce-Array-Slice by materializing a pointer to the
  // array storage plus the static element count.
  if (auto* arr_ty = llvm::dyn_cast<llvm::ArrayType>(value->getType())) {
    auto* target_struct = llvm::dyn_cast<llvm::StructType>(target);
    if (target_struct &&
        target_struct->getNumElements() == 2 &&
        target_struct->getElementType(0)->isPointerTy() &&
        target_struct->getElementType(1)->isIntegerTy()) {
      llvm::Type* ptr_field_ty = target_struct->getElementType(0);
      llvm::Type* len_field_ty = target_struct->getElementType(1);

      llvm::Function* fn = builder->GetInsertBlock()
                               ? builder->GetInsertBlock()->getParent()
                               : nullptr;
      if (!fn) {
        return llvm::Constant::getNullValue(target);
      }

      llvm::Value* data_ptr = nullptr;
      const std::uint64_t elem_count = arr_ty->getNumElements();
      if (elem_count == 0) {
        data_ptr =
            llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(ptr_field_ty));
      } else {
        llvm::AllocaInst* slot = CreateEntryAlloca(fn, arr_ty, "array_to_slice");
        if (!slot) {
          return llvm::Constant::getNullValue(target);
        }
        builder->CreateStore(value, slot);

        llvm::Value* zero = llvm::ConstantInt::get(llvm::Type::getInt64Ty(builder->getContext()), 0);
        llvm::Value* elem_ptr = builder->CreateGEP(
            arr_ty,
            slot,
            {zero, zero});
        data_ptr = CoerceValue(builder, elem_ptr, ptr_field_ty);
      }
      if (!data_ptr) {
        data_ptr =
            llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(ptr_field_ty));
      }

      llvm::Value* len_val = llvm::ConstantInt::get(len_field_ty, elem_count);
      llvm::Value* slice_val = llvm::Constant::getNullValue(target_struct);
      slice_val = builder->CreateInsertValue(slice_val, data_ptr, {0u});
      slice_val = builder->CreateInsertValue(slice_val, len_val, {1u});
      return slice_val;
    }
  }

  // Pointer to pointer (bitcast in opaque pointer world is identity)
  if (value->getType()->isPointerTy() && target->isPointerTy()) {
    return builder->CreateBitCast(value, target);
  }

  // Integer coercion
  if (value->getType()->isIntegerTy() && target->isIntegerTy()) {
    unsigned from_bits = value->getType()->getIntegerBitWidth();
    unsigned to_bits = target->getIntegerBitWidth();
    if (from_bits < to_bits) {
      return builder->CreateZExt(value, target);
    }
    if (from_bits > to_bits) {
      return builder->CreateTrunc(value, target);
    }
  }

  // Float coercion
  if (value->getType()->isFloatingPointTy() && target->isFloatingPointTy()) {
    unsigned from_bits = value->getType()->getPrimitiveSizeInBits();
    unsigned to_bits = target->getPrimitiveSizeInBits();
    if (from_bits < to_bits) {
      return builder->CreateFPExt(value, target);
    }
    if (from_bits > to_bits) {
      return builder->CreateFPTrunc(value, target);
    }
  }
  if (value->getType()->isFirstClassType() && target->isFirstClassType()) {
    llvm::BasicBlock* insert_block = builder->GetInsertBlock();
    llvm::Function* fn = insert_block ? insert_block->getParent() : nullptr;
    llvm::Module* module = fn ? fn->getParent() : nullptr;
    if (fn && module) {
      const llvm::DataLayout& layout = module->getDataLayout();
      const std::uint64_t src_bits = layout.getTypeSizeInBits(value->getType());
      const std::uint64_t dst_bits = layout.getTypeSizeInBits(target);
      if (src_bits == dst_bits && src_bits > 0 &&
          !llvm::CastInst::isBitCastable(value->getType(), target)) {
        llvm::AllocaInst* slot = CreateEntryAlloca(fn, value->getType(), "coerce_bits");
        if (slot) {
          builder->CreateStore(value, slot);
          llvm::Value* cast_ptr =
              builder->CreateBitCast(slot, llvm::PointerType::get(target, 0));
          llvm::LoadInst* loaded = builder->CreateLoad(target, cast_ptr);
          loaded->setAlignment(llvm::Align(1));
          return loaded;
        }
      }
    }
  }

  if (llvm::CastInst::isBitCastable(value->getType(), target)) {
    return builder->CreateBitCast(value, target);
  }
  return llvm::Constant::getNullValue(target);
}

// -----------------------------------------------------------------------------
// Argument Lowering
// -----------------------------------------------------------------------------

std::vector<llvm::Type*> ComputeLLVMParamTypes(
    LLVMEmitter& emitter,
    const std::vector<IRParam>& params,
    const std::vector<PassKind>& param_kinds,
    bool has_sret) {

  std::vector<llvm::Type*> param_types;

  // Add sret pointer if needed
  if (has_sret) {
    param_types.push_back(emitter.GetOpaquePtr());
  }

  const analysis::ScopeContext& scope = BuildScope(emitter.GetCurrentCtx());

  for (std::size_t i = 0; i < params.size(); ++i) {
    if (i >= param_kinds.size()) {
      break;
    }

    const auto kind = param_kinds[i];
    if (kind == PassKind::ByRef) {
      SPEC_RULE("LLVMArgLower-ByRef");
      param_types.push_back(emitter.GetOpaquePtr());
      continue;
    }

    const auto size = SizeOf(scope, params[i].type);
    if (!size.has_value()) {
      SPEC_RULE("LLVMArgLower-Err");
      param_types.push_back(emitter.GetOpaquePtr());
      continue;
    }

    if (*size == 0) {
      // Zero-sized types don't need a parameter
      continue;
    }

    if (IsValidPtrType(params[i].type)) {
      SPEC_RULE("LLVMArgLower-ByValue-PtrValid");
    } else {
      SPEC_RULE("LLVMArgLower-ByValue-Other");
    }

    llvm::Type* llvm_ty = emitter.GetLLVMType(params[i].type);
    if (!llvm_ty) {
      SPEC_RULE("LLVMArgLower-Err");
      llvm_ty = emitter.GetOpaquePtr();
    }
    param_types.push_back(llvm_ty);
  }

  return param_types;
}

// -----------------------------------------------------------------------------
// Return Value Lowering
// -----------------------------------------------------------------------------

llvm::Type* ComputeLLVMReturnType(LLVMEmitter& emitter,
                                  const analysis::TypeRef& ret_type,
                                  PassKind ret_kind) {
  if (ret_kind == PassKind::SRet) {
    SPEC_RULE("LLVMRetLower-SRet");
    return llvm::Type::getVoidTy(emitter.GetContext());
  }

  const analysis::ScopeContext& scope = BuildScope(emitter.GetCurrentCtx());
  const auto size = SizeOf(scope, ret_type);

  if (!size.has_value()) {
    SPEC_RULE("LLVMRetLower-Err");
    return llvm::Type::getVoidTy(emitter.GetContext());
  }

  if (*size == 0) {
    SPEC_RULE("LLVMRetLower-ByValue-ZST");
    return llvm::Type::getVoidTy(emitter.GetContext());
  }

  SPEC_RULE("LLVMRetLower-ByValue");
  llvm::Type* llvm_ty = emitter.GetLLVMType(ret_type);
  if (!llvm_ty) {
    SPEC_RULE("LLVMRetLower-Err");
    return llvm::Type::getVoidTy(emitter.GetContext());
  }

  return llvm_ty;
}

// -----------------------------------------------------------------------------
// Call Emission
// -----------------------------------------------------------------------------

llvm::Value* EmitABICall(LLVMEmitter& emitter,
                         llvm::IRBuilderBase* builder_base,
                         llvm::Value* callee,
                         const std::vector<IRParam>& params,
                         const analysis::TypeRef& ret_type,
                         const std::vector<llvm::Value*>& args,
                         bool use_c_abi_aggregate_sret,
                         bool ffi_import_boundary,
                         bool ffi_import_catch,
                         std::optional<unsigned> call_conv_override,
                         const std::vector<IRValue>* source_args,
                         llvm::Value** result_storage_out,
                         llvm::Value* preferred_result_storage) {
  if (!builder_base || !callee) {
    return nullptr;
  }

  auto* builder = static_cast<llvm::IRBuilder<>*>(builder_base);

  ABICallResult abi = emitter.ComputeCallABI(
      params,
      ret_type,
      use_c_abi_aggregate_sret,
      /*foreign_boundary_mode_independent=*/
      (ffi_import_boundary || use_c_abi_aggregate_sret));
  if (!abi.func_type) {
    return nullptr;
  }

  llvm::Function* func = builder->GetInsertBlock()
                             ? builder->GetInsertBlock()->getParent()
                             : nullptr;
  if (!func) {
    return nullptr;
  }

  std::vector<llvm::Value*> call_args(abi.func_type->getNumParams(), nullptr);
  llvm::Value* sret_alloca = nullptr;
  struct ScratchUse {
    llvm::Type* ty = nullptr;
    std::string_view name;
    unsigned count = 0;
  };
  std::vector<ScratchUse> scratch_uses;

  auto next_scratch_ordinal =
      [&](llvm::Type* ty, std::string_view name) -> unsigned {
    for (auto& use : scratch_uses) {
      if (use.ty == ty && use.name == name) {
        return use.count++;
      }
    }
    scratch_uses.push_back(ScratchUse{ty, name, 1});
    return 0;
  };

  if (result_storage_out) {
    *result_storage_out = nullptr;
  }

  auto existing_arg_storage = [&](std::size_t index,
                                  llvm::Type* elem_ty) -> llvm::Value* {
    if (!source_args || index >= source_args->size()) {
      return nullptr;
    }
    llvm::Value* storage = emitter.GetAddressableStorage((*source_args)[index]);
    if (!storage || !storage->getType()->isPointerTy()) {
      return nullptr;
    }
    if (elem_ty) {
      llvm::Type* target_ptr_ty = llvm::PointerType::get(elem_ty, 0);
      if (storage->getType() != target_ptr_ty) {
        storage = CoerceValue(builder, storage, target_ptr_ty);
      }
    }
    return storage;
  };

  // Handle sret parameter
  if (abi.has_sret) {
    llvm::Type* ret_ty = emitter.GetLLVMType(ret_type);
    if (preferred_result_storage && ret_ty &&
        preferred_result_storage->getType()->isPointerTy()) {
      llvm::Type* target_ptr_ty = llvm::PointerType::get(ret_ty, 0);
      sret_alloca = preferred_result_storage;
      if (sret_alloca->getType() != target_ptr_ty) {
        sret_alloca = CoerceValue(builder, sret_alloca, target_ptr_ty);
      }
    } else if (result_storage_out) {
      // Published aggregate results must not alias other still-live call
      // results, but they can reuse previously released aggregate temp
      // storage once the prior owner is dead.
      sret_alloca = emitter.AcquireReusableAggregateStorage(func, ret_ty, "sret");
    } else {
      const unsigned ordinal = next_scratch_ordinal(ret_ty, "sret");
      sret_alloca = AcquireReusableEntryAlloca(func, ret_ty, "sret", ordinal);
    }
    call_args[0] = sret_alloca;
    if (result_storage_out) {
      *result_storage_out = sret_alloca;
    }
  }

  // Map arguments according to ABI
  for (std::size_t i = 0; i < params.size(); ++i) {
    if (i >= abi.param_indices.size()) {
      break;
    }
    if (!abi.param_indices[i].has_value()) {
      continue;
    }

    unsigned idx = *abi.param_indices[i];
    if (idx >= call_args.size() || i >= args.size()) {
      continue;
    }

    llvm::Value* arg = args[i];
    if (!arg) {
      continue;
    }

    const ABIArgCarrierKind carrier =
        i < abi.param_carriers.size() ? abi.param_carriers[i]
                                      : ABIArgCarrierKind::Direct;

    if (abi.param_kinds[i] == PassKind::ByRef) {
      // Need to pass by reference - create temporary if not already a pointer
      llvm::Type* elem_ty = emitter.GetLLVMType(params[i].type);
      if (!arg->getType()->isPointerTy()) {
        llvm::Value* storage = existing_arg_storage(i, elem_ty);
        if (storage) {
          call_args[idx] = storage;
          continue;
        }
        const unsigned ordinal = next_scratch_ordinal(elem_ty, "byref_arg");
        llvm::AllocaInst* slot =
            AcquireReusableEntryAlloca(func, elem_ty, "byref_arg", ordinal);
        if (slot) {
          llvm::Value* stored = CoerceValue(builder, arg, elem_ty);
          builder->CreateStore(stored, slot);
          call_args[idx] = slot;
        }
        continue;
      }
      llvm::Type* target_ty = abi.param_types[idx];
      if (target_ty && arg->getType() != target_ty) {
        arg = CoerceValue(builder, arg, target_ty);
      }
      call_args[idx] = arg;
      continue;
    }

    if (carrier == ABIArgCarrierKind::Indirect) {
      llvm::Type* elem_ty = emitter.GetLLVMType(params[i].type);
      llvm::Value* ptr_arg = arg;
      if (!ptr_arg->getType()->isPointerTy()) {
        if (llvm::Value* storage = existing_arg_storage(i, elem_ty)) {
          ptr_arg = storage;
        } else {
        const unsigned ordinal = next_scratch_ordinal(elem_ty, "indirect_arg");
        llvm::AllocaInst* slot =
            AcquireReusableEntryAlloca(func, elem_ty, "indirect_arg", ordinal);
        if (slot) {
          llvm::Value* stored = CoerceValue(builder, arg, elem_ty);
          builder->CreateStore(stored, slot);
          ptr_arg = slot;
        } else {
          continue;
        }
        }
      }
      llvm::Type* target_ty = abi.param_types[idx];
      if (target_ty && ptr_arg->getType() != target_ty) {
        ptr_arg = CoerceValue(builder, ptr_arg, target_ty);
      }
      call_args[idx] = ptr_arg;
      continue;
    }

    // By value
    llvm::Type* target_ty = abi.param_types[idx];
    if (target_ty && arg->getType() != target_ty) {
      arg = CoerceValue(builder, arg, target_ty);
    }
    call_args[idx] = arg;
  }

  // Fill in null values for any missing arguments
  for (std::size_t i = 0; i < call_args.size(); ++i) {
    if (!call_args[i]) {
      call_args[i] = llvm::Constant::getNullValue(abi.param_types[i]);
    }
  }

  auto release_consumed_move_arg_temps = [&]() -> void {
    if (!source_args) {
      return;
    }

    const std::size_t released_count =
        std::min(source_args->size(), params.size());
    for (std::size_t i = 0; i < released_count; ++i) {
      if (!params[i].mode.has_value()) {
        continue;
      }

      emitter.ReleaseMoveConsumedStorage((*source_args)[i]);
    }
  };

  llvm::Instruction* call_like_inst = nullptr;
  llvm::Value* direct_result = nullptr;
  llvm::BasicBlock* normal_block = nullptr;
  llvm::BasicBlock* unwind_block = nullptr;
  unsigned call_conv = llvm::CallingConv::C;
  if (call_conv_override.has_value()) {
    call_conv = *call_conv_override;
  }
  if (auto* callee_fn = llvm::dyn_cast<llvm::Function>(callee)) {
    call_conv = callee_fn->getCallingConv();
  }

  if (ffi_import_boundary) {
    if (ffi_import_catch) {
      SPEC_RULE("CodeGen-UnwindCatch-Import");
    } else {
      SPEC_RULE("CodeGen-UnwindAbort-Import");
    }

    llvm::Type* i32_ty = llvm::Type::getInt32Ty(emitter.GetContext());
    llvm::FunctionType* personality_ty =
        llvm::FunctionType::get(i32_ty, /*isVarArg=*/true);
    llvm::FunctionCallee personality =
        emitter.GetModule().getOrInsertFunction(
            UnwindPersonalitySymbolForModule(emitter.GetModule()),
            personality_ty);
    func->setPersonalityFn(llvm::cast<llvm::Constant>(personality.getCallee()));

    normal_block = llvm::BasicBlock::Create(emitter.GetContext(),
                                            "ffi.invoke.cont",
                                            func);
    unwind_block = llvm::BasicBlock::Create(emitter.GetContext(),
                                            "ffi.invoke.unwind",
                                            func);

    llvm::InvokeInst* invoke_inst =
        builder->CreateInvoke(abi.func_type,
                              callee,
                              normal_block,
                              unwind_block,
                              call_args);
    invoke_inst->setCallingConv(call_conv);
    call_like_inst = invoke_inst;

    builder->SetInsertPoint(unwind_block);

    llvm::Type* i8_ty = llvm::Type::getInt8Ty(emitter.GetContext());
    auto* i8_ptr_ty = llvm::PointerType::get(i8_ty, 0);
    auto* lpad_ty = llvm::StructType::get(i8_ptr_ty, i32_ty);
    llvm::LandingPadInst* landing_pad =
        builder->CreateLandingPad(lpad_ty, 1, "ffi_lpad");
    landing_pad->setCleanup(true);
    landing_pad->addClause(llvm::ConstantPointerNull::get(i8_ptr_ty));

    if (ffi_import_catch) {
      const std::uint16_t panic_code = PanicCode(PanicReason::Other);
      StorePanicRecord(emitter, builder, panic_code);

      if (abi.has_sret && sret_alloca) {
        llvm::Type* ret_ty = emitter.GetLLVMType(ret_type);
        if (ret_ty) {
          builder->CreateStore(llvm::Constant::getNullValue(ret_ty), sret_alloca);
        }
      }

      builder->CreateBr(normal_block);
    } else {
      const std::uint16_t panic_code = PanicCode(PanicReason::Other);
      const std::string panic_sym = PanicSym();
      llvm::Function* panic_fn = emitter.GetModule().getFunction(panic_sym);
      if (!panic_fn) {
        llvm::FunctionType* panic_ty = llvm::FunctionType::get(
            llvm::Type::getVoidTy(emitter.GetContext()),
            {i32_ty},
            false);
        panic_fn = llvm::Function::Create(
            panic_ty,
            llvm::GlobalValue::ExternalLinkage,
            panic_sym,
            &emitter.GetModule());
        panic_fn->setCallingConv(llvm::CallingConv::C);
      }
      llvm::Value* panic_arg = llvm::ConstantInt::get(i32_ty, panic_code);
      builder->CreateCall(panic_fn->getFunctionType(), panic_fn, {panic_arg});
      builder->CreateUnreachable();
    }

    builder->SetInsertPoint(normal_block);

    if (!invoke_inst->getType()->isVoidTy()) {
      if (ffi_import_catch) {
        llvm::PHINode* result_phi =
            builder->CreatePHI(invoke_inst->getType(), 2, "ffi_invoke_result");
        result_phi->addIncoming(invoke_inst, invoke_inst->getParent());
        result_phi->addIncoming(llvm::Constant::getNullValue(invoke_inst->getType()),
                                unwind_block);
        direct_result = result_phi;
      } else {
        direct_result = invoke_inst;
      }
    }
  } else {
    llvm::CallInst* call_inst =
        builder->CreateCall(abi.func_type, callee, call_args);
    call_inst->setCallingConv(call_conv);
    call_like_inst = call_inst;
    if (!call_inst->getType()->isVoidTy()) {
      direct_result = call_inst;
    }
  }

  release_consumed_move_arg_temps();

  // Handle return value
  if (abi.has_sret && sret_alloca) {
    if (!result_storage_out) {
      return builder->CreateLoad(emitter.GetLLVMType(ret_type), sret_alloca);
    }
    return nullptr;
  }

  if (direct_result) {
    if (ret_type) {
      llvm::Type* expected = emitter.GetLLVMType(ret_type);
      if (expected && direct_result->getType() != expected) {
        direct_result = CoerceValue(builder, direct_result, expected);
      }
    }
    return direct_result;
  }

  if (call_like_inst && !call_like_inst->getType()->isVoidTy()) {
    return call_like_inst;
  }

  if (ret_type) {
    return llvm::Constant::getNullValue(emitter.GetLLVMType(ret_type));
  }

  return nullptr;
}

// -----------------------------------------------------------------------------
// Calling Convention
// -----------------------------------------------------------------------------

unsigned GetCursiveCallingConv() {
  // Cursive uses C calling convention on Windows
  return llvm::CallingConv::C;
}

bool IsCCallingConv(std::string_view symbol) {
  // Check for extern "C" symbols
  return IsExternC(symbol);
}

bool IsExternC(std::string_view symbol) {
  // Extern C symbols don't have mangled names
  // Check if symbol starts with mangling prefix
  if (symbol.empty()) {
    return false;
  }
  // If it starts with underscore and a capital letter, likely extern C
  // This is a heuristic - actual determination would need symbol table lookup
  if (symbol[0] == '_' && symbol.size() > 1) {
    char second = symbol[1];
    // Mangled names typically start with _C or _Z
    if (second == 'C' || second == 'Z') {
      return false;  // Mangled
    }
    return true;  // Likely extern C
  }
  // Simple names are extern C
  return symbol.find("::") == std::string_view::npos;
}

// -----------------------------------------------------------------------------
// Procedure ABI Wrapper
// -----------------------------------------------------------------------------

  ABICallResult LLVMEmitter::ComputeProcABI(
      const std::string &symbol,
      const std::vector<IRParam> &params,
      analysis::TypeRef ret_type,
      bool use_c_abi_aggregate_sret,
      bool foreign_boundary_mode_independent)
  {
    std::vector<IRParam> augmented = params;
    if (RequiresHostedEnvParam(symbol) && !emit_detail::HasLeadingHostedEnvParam(augmented))
    {
      augmented.insert(augmented.begin(), HostedEnvParam());
    }
    const bool needs_panic_out =
        current_ctx_ ? current_ctx_->NeedsPanicOutForSymbol(symbol)
                     : NeedsPanicOut(symbol);
    if (needs_panic_out &&
        (augmented.empty() || augmented.back().name != std::string(kPanicOutName)))
    {
      augmented.push_back(PanicOutParam());
    }

    analysis::TypeRef abi_ret = ret_type;
    if (const LowerCtx::AsyncProcInfo *async_info =
            current_ctx_ ? current_ctx_->LookupAsyncProc(symbol) : nullptr;
        async_info && async_info->is_resume &&
        emit_detail::HasNamedParam(augmented, kAsyncOutParamName))
    {
      abi_ret = analysis::MakeTypePrim("()");
    }

    if (!use_c_abi_aggregate_sret && current_ctx_)
    {
      if (const auto *sig = current_ctx_->LookupProcSig(symbol);
          sig && sig->abi.has_value())
      {
        use_c_abi_aggregate_sret = true;
        foreign_boundary_mode_independent = true;
      }
    }

    return ComputeCallABI(augmented,
                          abi_ret,
                          use_c_abi_aggregate_sret,
                          foreign_boundary_mode_independent);
  }
}  // namespace cursive::codegen
