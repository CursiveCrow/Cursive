#include "cursive0/04_codegen/llvm/llvm_emit.h"
#include "cursive0/04_codegen/abi/abi.h"
#include "cursive0/04_codegen/cleanup.h"
#include "cursive0/04_codegen/async_frame.h"
#include "cursive0/04_codegen/layout/layout.h"
#include "cursive0/00_core/assert_spec.h"
#include "cursive0/00_core/symbols.h"
#include "cursive0/runtime/runtime_interface.h"

// LLVM Includes
#include "llvm/IR/Attributes.h"
#include "llvm/IR/Comdat.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/TargetParser/Triple.h"

namespace cursive0::codegen {

LLVMEmitter::LLVMEmitter(llvm::LLVMContext& ctx, const std::string& module_name)
    : context_(ctx),
      module_(std::make_unique<llvm::Module>(module_name, ctx)),
      builder_(std::make_unique<llvm::IRBuilder<>>(ctx)) {
}

LLVMEmitter::~LLVMEmitter() = default;

std::unique_ptr<llvm::Module> LLVMEmitter::ReleaseModule() {
  return std::move(module_);
}

// T-LLVM-001: LLVM module header and target constants
void LLVMEmitter::SetupModule() {
  SPEC_DEF("TargetTriple", "§6.12.1");
  SPEC_DEF("TargetDataLayout", "§6.12.1");

  // Spec §6.12.1: Target MUST be x86_64-pc-windows-msvc
  module_->setTargetTriple(llvm::Triple("x86_64-pc-windows-msvc"));

  // Spec §6.12.1: DataLayout MUST match Win64 model
  module_->setDataLayout("e-m:w-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128");
}

namespace {

analysis::ScopeContext BuildScope(const LowerCtx* ctx) {
  analysis::ScopeContext scope;
  if (ctx && ctx->sigma) {
    scope.sigma = *ctx->sigma;
    scope.current_module = ctx->module_path;
  }
  return scope;
}

llvm::AllocaInst* CreateEntryAlloca(llvm::Function* func,
                                    llvm::Type* ty,
                                    const std::string& name) {
  if (!func || !ty) {
    return nullptr;
  }
  llvm::IRBuilder<> entry_builder(&func->getEntryBlock(), func->getEntryBlock().begin());
  return entry_builder.CreateAlloca(ty, nullptr, name);
}

llvm::Value* ByteGEP(LLVMEmitter& emitter,
                     llvm::IRBuilder<>* builder,
                     llvm::Value* base_ptr,
                     std::uint64_t offset) {
  if (!builder || !base_ptr) {
    return nullptr;
  }
  llvm::LLVMContext& ctx = emitter.GetContext();
  llvm::Value* idx = llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx), offset);
  return builder->CreateGEP(llvm::Type::getInt8Ty(ctx), base_ptr, idx);
}

void StoreAtOffset(LLVMEmitter& emitter,
                   llvm::IRBuilder<>* builder,
                   llvm::Value* base_ptr,
                   std::uint64_t offset,
                   llvm::Value* value) {
  if (!builder || !base_ptr || !value) {
    return;
  }
  llvm::Value* ptr = offset == 0 ? base_ptr : ByteGEP(emitter, builder, base_ptr, offset);
  builder->CreateStore(value, ptr);
}

llvm::Value* LoadPanicOutPtr(LLVMEmitter& emitter, llvm::IRBuilder<>* builder) {
  if (!builder) {
    return nullptr;
  }
  llvm::Value* slot = emitter.GetLocal(std::string(kPanicOutName));
  if (!slot) {
    return nullptr;
  }
  if (auto* alloca = llvm::dyn_cast<llvm::AllocaInst>(slot)) {
    return builder->CreateLoad(alloca->getAllocatedType(), alloca);
  }
  return builder->CreateLoad(emitter.GetOpaquePtr(), slot);
}

llvm::Value* CoerceValue(llvm::IRBuilder<>* builder,
                         llvm::Value* value,
                         llvm::Type* target) {
  if (!builder || !value || !target) {
    return value;
  }
  if (value->getType() == target) {
    return value;
  }
  if (value->getType()->isPointerTy() && target->isPointerTy()) {
    return builder->CreateBitCast(value, target);
  }
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
  return builder->CreateBitCast(value, target);
}

llvm::Value* EmitABICall(LLVMEmitter& emitter,
                         llvm::IRBuilder<>* builder,
                         llvm::Value* callee,
                         const std::vector<IRParam>& params,
                         const analysis::TypeRef& ret_type,
                         const std::vector<llvm::Value*>& args) {
  if (!builder || !callee) {
    return nullptr;
  }

  ABICallResult abi = emitter.ComputeCallABI(params, ret_type);
  if (!abi.func_type) {
    return nullptr;
  }

  llvm::Function* func = builder->GetInsertBlock()->getParent();
  std::vector<llvm::Value*> call_args(abi.func_type->getNumParams(), nullptr);
  llvm::AllocaInst* sret_alloca = nullptr;

  if (abi.has_sret) {
    llvm::Type* ret_ty = emitter.GetLLVMType(ret_type);
    sret_alloca = CreateEntryAlloca(func, ret_ty, "sret_async");
    call_args[0] = sret_alloca;
  }

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
    if (abi.param_kinds[i] == PassKind::ByRef) {
      llvm::Type* elem_ty = emitter.GetLLVMType(params[i].type);
      if (!arg->getType()->isPointerTy()) {
        llvm::AllocaInst* slot = CreateEntryAlloca(func, elem_ty, "byref_arg");
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
    llvm::Type* target_ty = abi.param_types[idx];
    if (target_ty && arg->getType() != target_ty) {
      arg = CoerceValue(builder, arg, target_ty);
    }
    call_args[idx] = arg;
  }

  for (std::size_t i = 0; i < call_args.size(); ++i) {
    if (!call_args[i]) {
      call_args[i] = llvm::Constant::getNullValue(abi.param_types[i]);
    }
  }

  llvm::Value* callee_val = callee;
  llvm::Type* callee_ty = abi.func_type->getPointerTo();
  if (callee_val->getType() != callee_ty && callee_val->getType()->isPointerTy()) {
    callee_val = builder->CreateBitCast(callee_val, callee_ty);
  }

  llvm::CallInst* call_inst = builder->CreateCall(abi.func_type, callee_val, call_args);
  call_inst->setCallingConv(llvm::CallingConv::C);

  if (abi.has_sret && sret_alloca) {
    return builder->CreateLoad(emitter.GetLLVMType(ret_type), sret_alloca);
  }
  if (!call_inst->getType()->isVoidTy()) {
    return call_inst;
  }
  if (ret_type) {
    return llvm::Constant::getNullValue(emitter.GetLLVMType(ret_type));
  }
  return nullptr;
}

IRParam MakeParam(std::string name, std::optional<analysis::ParamMode> mode, analysis::TypeRef type) {
  IRParam param;
  param.mode = mode;
  param.name = std::move(name);
  param.type = std::move(type);
  return param;
}

bool IsDropGlueSymbol(std::string_view sym) {
  static const std::string prefix =
      core::Mangle(core::StringOfPath({"cursive", "runtime", "drop"}));
  return sym.size() >= prefix.size() && sym.compare(0, prefix.size(), prefix) == 0;
}

}  // namespace

llvm::Module* LLVMEmitter::EmitModule(const IRDecls& decls, LowerCtx& ctx) {
  SPEC_RULE("LowerIR-Module");
  SPEC_RULE("EmitLLVM-Ok");
  current_ctx_ = &ctx;

  SetupModule();
  DeclareRuntime();

  IRDecls expanded = decls;
  for (const auto& proc : ctx.extra_procs) {
    expanded.push_back(proc);
  }

  auto declare_proc = [&](const std::string& sym,
                          const std::vector<IRParam>& params,
                          const analysis::TypeRef& ret,
                          bool linkonce_drop_glue) {
    if (functions_.count(sym)) {
      return;
    }

    ABICallResult abi = ComputeCallABI(params, ret);
    llvm::FunctionType* ft = abi.func_type;
    if (!ft) {
      ft = llvm::FunctionType::get(llvm::Type::getVoidTy(context_), {}, false);
    }

    llvm::Function* f = llvm::Function::Create(
        ft, llvm::GlobalValue::ExternalLinkage, sym, module_.get());
    f->setCallingConv(llvm::CallingConv::C);
    if (linkonce_drop_glue) {
      f->setLinkage(llvm::GlobalValue::LinkOnceODRLinkage);
      f->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);
      if (auto* comdat = module_->getOrInsertComdat(sym)) {
        comdat->setSelectionKind(llvm::Comdat::Any);
        f->setComdat(comdat);
      }
    }

    if (abi.has_sret) {
      auto sret_ptr = analysis::MakeTypePerm(
          analysis::Permission::Unique,
          analysis::MakeTypePtr(ret, analysis::PtrState::Valid));
      AddPtrAttributes(f, 0, sret_ptr);
      f->addParamAttr(0, llvm::Attribute::StructRet);
    }

    for (std::size_t i = 0; i < params.size(); ++i) {
      if (i >= abi.param_kinds.size()) {
        break;
      }
      if (i >= abi.param_indices.size()) {
        break;
      }
      if (!abi.param_indices[i].has_value()) {
        continue;
      }
      unsigned idx = *abi.param_indices[i];
      if (abi.param_kinds[i] == PassKind::ByRef) {
        auto byref_ptr = analysis::MakeTypePerm(
            analysis::Permission::Const,
            analysis::MakeTypePtr(params[i].type, analysis::PtrState::Valid));
        AddPtrAttributes(f, idx, byref_ptr);
        AddPtrAttributes(f, idx, params[i].type);
      } else {
        AddPtrAttributes(f, idx, params[i].type);
      }
    }

    functions_[sym] = f;
  };

  // Emit drop glue procedures for any referenced types.
  for (const auto& entry : ctx.drop_glue_types) {
    const std::string& sym = entry.first;
    const analysis::TypeRef& type = entry.second;
    bool exists = false;
    for (const auto& decl : expanded) {
      if (auto* proc = std::get_if<ProcIR>(&decl)) {
        if (proc->symbol == sym) {
          exists = true;
          break;
        }
      }
    }
    if (exists) {
      continue;
    }
    ProcIR proc;
    proc.symbol = sym;
    proc.params.push_back(MakeParam("data", analysis::ParamMode::Move,
                                    analysis::MakeTypeRawPtr(analysis::RawPtrQual::Imm,
                                                         analysis::MakeTypePrim("()"))));
    proc.params.push_back(MakeParam(std::string(kPanicOutName), analysis::ParamMode::Move,
                                    PanicOutType()));
    proc.ret = analysis::MakeTypePrim("()");
    proc.body = EmitDropGlue(type, ctx);
    ctx.RegisterProcSig(proc);
    expanded.push_back(std::move(proc));
  }

  // Pass 1: declare functions
  for (const auto& decl : expanded) {
    if (auto* proc = std::get_if<ProcIR>(&decl)) {
      declare_proc(proc->symbol, proc->params, proc->ret, IsDropGlueSymbol(proc->symbol));
    } else if (auto* ext = std::get_if<ExternProcIR>(&decl)) {
      declare_proc(ext->symbol, ext->params, ext->ret, false);
    }
  }

  // Declare external procedures referenced from other modules.
  for (const auto& [sym, sig] : ctx.proc_sigs) {
    if (functions_.count(sym)) {
      continue;
    }
    declare_proc(sym, sig.params, sig.ret, false);
  }

  // Pass 2: emit definitions and globals
  for (const auto& decl : expanded) {
    EmitDecl(decl);
  }

  if (ctx.main_symbol.has_value()) {
    EmitEntryPoint();
  }

  return module_.get();
}

void LLVMEmitter::EmitDecl(const IRDecl& decl) {
  struct Visitor {
    LLVMEmitter& emitter;
    void operator()(const ProcIR& proc) { emitter.EmitProc(proc); }
    void operator()(const GlobalConst& global) { emitter.EmitGlobalConst(global); }
    void operator()(const GlobalZero& global) { emitter.EmitGlobalZero(global); }
    void operator()(const GlobalVTable& vtable) { emitter.EmitVTable(vtable); }
    void operator()(const ExternProcIR&) { /* extern procs have no body */ }
  };
  std::visit(Visitor{*this}, decl);
  if (current_ctx_ && current_ctx_->codegen_failed) {
    SPEC_RULE("LowerIRDecl-Err");
  }
}

void LLVMEmitter::EmitProc(const ProcIR& proc) {
  if (IsDropGlueSymbol(proc.symbol)) {
    SPEC_RULE("LowerIRDecl-Proc-Gen");
  } else {
    SPEC_RULE("LowerIRDecl-Proc-User");
  }

  llvm::Function* func = functions_[proc.symbol];
  if (!func) {
    return;
  }

  ClearLocals();
  ClearTempValues();
  ClearSymbolAliases();
  active_regions_.clear();
  if (current_ctx_) {
    current_ctx_->scope_stack.clear();
    current_ctx_->binding_states.clear();
    current_ctx_->PushScope(false, false);
  }

  llvm::BasicBlock* entry = llvm::BasicBlock::Create(context_, "entry", func);
  auto* builder = static_cast<llvm::IRBuilder<>*>(builder_.get());
  builder->SetInsertPoint(entry);

  ABICallResult abi = ComputeCallABI(proc.params, proc.ret);

  // Map parameters into locals according to ABI
  for (std::size_t i = 0; i < proc.params.size(); ++i) {
    if (i >= abi.param_indices.size()) {
      break;
    }
    if (!abi.param_indices[i].has_value()) {
      continue;
    }
    unsigned idx = *abi.param_indices[i];
    if (idx >= func->arg_size()) {
      continue;
    }
    llvm::Argument* arg = func->getArg(idx);
    arg->setName(proc.params[i].name);

    if (i < abi.param_kinds.size() && abi.param_kinds[i] == PassKind::ByRef) {
      SPEC_RULE("BindSlot-Param-ByRef");
      SetLocal(proc.params[i].name, arg);
      if (current_ctx_ && proc.params[i].type) {
        const bool has_resp = proc.params[i].mode.has_value();
        current_ctx_->RegisterVar(proc.params[i].name, proc.params[i].type, has_resp, false);
      }
      continue;
    }

    SPEC_RULE("BindSlot-Param-ByValue");
    llvm::Type* llvm_ty = GetLLVMType(proc.params[i].type);
    llvm::IRBuilder<> entry_builder(&func->getEntryBlock(), func->getEntryBlock().begin());
    llvm::AllocaInst* alloca = entry_builder.CreateAlloca(llvm_ty, nullptr, proc.params[i].name);
    builder->CreateStore(arg, alloca);
    SetLocal(proc.params[i].name, alloca);
    if (current_ctx_ && proc.params[i].type) {
      const bool has_resp = proc.params[i].mode.has_value();
      current_ctx_->RegisterVar(proc.params[i].name, proc.params[i].type, has_resp, false);
    }
  }

  if (NeedsPanicOut(proc.symbol)) {
    EmitIR(MakeIR(IRClearPanic{}));
  }

  const LowerCtx::AsyncProcInfo* async_info =
      current_ctx_ ? current_ctx_->LookupAsyncProc(proc.symbol) : nullptr;

  auto emit_async_wrapper = [&](const LowerCtx::AsyncProcInfo& info) {
    llvm::Function* alloc_fn = GetFunction(BuiltinSymAsyncAllocFrame());
    if (!alloc_fn) {
      alloc_fn = module_->getFunction(BuiltinSymAsyncAllocFrame());
    }
    if (!alloc_fn) {
      if (current_ctx_) {
        current_ctx_->ReportCodegenFailure();
      }
      return;
    }

    llvm::Type* usize_ty = GetLLVMType(analysis::MakeTypePrim("usize"));
    llvm::Value* size_val = llvm::ConstantInt::get(usize_ty, info.frame_size);
    llvm::Value* align_val = llvm::ConstantInt::get(usize_ty, info.frame_align);
    llvm::Value* frame_ptr = builder->CreateCall(alloc_fn, {size_val, align_val});
    frame_ptr = CoerceValue(builder, frame_ptr, GetOpaquePtr());

    llvm::Value* resume_state_val =
        llvm::ConstantInt::get(llvm::Type::getInt64Ty(context_), 0);
    StoreAtOffset(*this, builder, frame_ptr, kAsyncFrameResumeStateOffset, resume_state_val);

    llvm::Function* resume_fn = GetFunction(info.resume_symbol);
    if (!resume_fn) {
      resume_fn = module_->getFunction(info.resume_symbol);
    }
    if (!resume_fn) {
      if (current_ctx_) {
        current_ctx_->ReportCodegenFailure();
      }
      return;
    }
    llvm::Value* resume_ptr = builder->CreateBitCast(resume_fn, GetOpaquePtr());
    StoreAtOffset(*this, builder, frame_ptr, kAsyncFrameResumeFnOffset, resume_ptr);

    const auto scope = BuildScope(current_ctx_);
    for (const auto& name : info.param_names) {
      const auto slot_it = info.slots.find(name);
      if (slot_it == info.slots.end()) {
        continue;
      }
      const auto& slot = slot_it->second;
      if (auto size_opt = SizeOf(scope, slot.type)) {
        if (*size_opt == 0) {
          continue;
        }
      }
      llvm::Type* slot_ty = GetLLVMType(slot.type);
      if (!slot_ty) {
        if (current_ctx_) {
          current_ctx_->ReportCodegenFailure();
        }
        continue;
      }

      IRValue local_val;
      local_val.kind = IRValue::Kind::Local;
      local_val.name = name;
      llvm::Value* value = EvaluateIRValue(local_val);
      if (!value) {
        if (current_ctx_) {
          current_ctx_->ReportCodegenFailure();
        }
        continue;
      }
      value = CoerceValue(builder, value, slot_ty);

      llvm::Value* slot_ptr = ByteGEP(*this, builder, frame_ptr, slot.offset);
      slot_ptr = builder->CreateBitCast(slot_ptr, slot_ty->getPointerTo());
      builder->CreateStore(value, slot_ptr);
    }

    llvm::Value* input_ptr = llvm::Constant::getNullValue(GetOpaquePtr());
    llvm::Value* panic_ptr = nullptr;
    if (info.resume_needs_panic_out) {
      panic_ptr = LoadPanicOutPtr(*this, builder);
      if (!panic_ptr) {
        panic_ptr = llvm::Constant::getNullValue(GetOpaquePtr());
      }
    }

    const LowerCtx::ProcSigInfo* resume_sig =
        current_ctx_ ? current_ctx_->LookupProcSig(info.resume_symbol) : nullptr;
    if (!resume_sig) {
      if (current_ctx_) {
        current_ctx_->ReportCodegenFailure();
      }
      return;
    }
    std::vector<llvm::Value*> resume_args;
    resume_args.push_back(frame_ptr);
    resume_args.push_back(input_ptr);
    if (info.resume_needs_panic_out) {
      resume_args.push_back(panic_ptr);
    }
    llvm::Value* resume_val =
        EmitABICall(*this, builder, resume_fn, resume_sig->params, resume_sig->ret, resume_args);
    if (!resume_val) {
      resume_val = llvm::Constant::getNullValue(GetLLVMType(proc.ret));
    }

    if (func->arg_size() > 0 && func->hasParamAttribute(0, llvm::Attribute::StructRet)) {
      llvm::Argument* sret = func->getArg(0);
      builder->CreateStore(resume_val, sret);
      builder->CreateRetVoid();
      return;
    }
    builder->CreateRet(resume_val);
  };

  auto emit_async_resume = [&](const LowerCtx::AsyncProcInfo& info) {
    auto get_param_arg = [&](std::string_view name) -> llvm::Value* {
      for (std::size_t i = 0; i < proc.params.size(); ++i) {
        if (proc.params[i].name != name) {
          continue;
        }
        if (i >= abi.param_indices.size()) {
          return nullptr;
        }
        if (!abi.param_indices[i].has_value()) {
          return nullptr;
        }
        unsigned idx = *abi.param_indices[i];
        if (idx >= func->arg_size()) {
          return nullptr;
        }
        llvm::Value* arg = func->getArg(idx);
        if (i < abi.param_kinds.size() && abi.param_kinds[i] == PassKind::ByRef) {
          llvm::Type* param_ty = GetLLVMType(proc.params[i].type);
          if (!param_ty) {
            return nullptr;
          }
          return builder->CreateLoad(param_ty, arg);
        }
        return arg;
      }
      return nullptr;
    };

    llvm::Value* frame_ptr = get_param_arg("__c0_async_frame");
    if (!frame_ptr) {
      if (current_ctx_) {
        current_ctx_->ReportCodegenFailure();
      }
      return;
    }
    frame_ptr = CoerceValue(builder, frame_ptr, GetOpaquePtr());

    llvm::Value* input_ptr = get_param_arg("__c0_async_input");
    if (input_ptr) {
      input_ptr = CoerceValue(builder, input_ptr, GetOpaquePtr());
    } else {
      input_ptr = llvm::Constant::getNullValue(GetOpaquePtr());
    }

    const auto scope = BuildScope(current_ctx_);
    for (const auto& name : info.param_names) {
      if (GetLocal(name)) {
        continue;
      }
      const auto slot_it = info.slots.find(name);
      if (slot_it == info.slots.end()) {
        continue;
      }
      const auto& slot = slot_it->second;
      if (auto size_opt = SizeOf(scope, slot.type)) {
        if (*size_opt == 0) {
          continue;
        }
      }
      llvm::Type* slot_ty = GetLLVMType(slot.type);
      if (!slot_ty) {
        if (current_ctx_) {
          current_ctx_->ReportCodegenFailure();
        }
        continue;
      }
      llvm::Value* slot_ptr = ByteGEP(*this, builder, frame_ptr, slot.offset);
      slot_ptr = builder->CreateBitCast(slot_ptr, slot_ty->getPointerTo());
      SetLocal(name, slot_ptr);
      if (current_ctx_) {
        current_ctx_->RegisterVar(name, slot.type, true, false);
      }
    }

    llvm::IntegerType* state_ty = llvm::Type::getInt64Ty(context_);
    llvm::Value* state_val = builder->CreateLoad(
        state_ty, ByteGEP(*this, builder, frame_ptr, kAsyncFrameResumeStateOffset));
    llvm::BasicBlock* entry_state =
        llvm::BasicBlock::Create(context_, "async_entry", func);
    llvm::SwitchInst* sw = builder->CreateSwitch(state_val, entry_state, 1);
    sw->addCase(llvm::ConstantInt::get(state_ty, 0), entry_state);

    AsyncEmitState async_state;
    async_state.info = &info;
    async_state.frame_ptr = frame_ptr;
    async_state.input_ptr = input_ptr;
    async_state.resume_switch = sw;
    SetAsyncState(&async_state);

    builder->SetInsertPoint(entry_state);
    EmitIR(proc.body);
    SetAsyncState(nullptr);
  };

  bool handled_async = false;
  if (async_info && async_info->is_wrapper) {
    emit_async_wrapper(*async_info);
    handled_async = true;
  } else if (async_info && async_info->is_resume) {
    emit_async_resume(*async_info);
    handled_async = true;
  }

  if (!handled_async) {
    EmitIR(proc.body);
  }

  // Ensure every block is terminated even if the body omits an explicit return.
  llvm::Type* ret_ty = func->getReturnType();
  for (auto& block : *func) {
    if (block.getTerminator()) {
      continue;
    }
    builder->SetInsertPoint(&block);
    if (ret_ty->isVoidTy()) {
      builder->CreateRetVoid();
    } else {
      builder->CreateRet(llvm::Constant::getNullValue(ret_ty));
    }
  }

  if (current_ctx_) {
    current_ctx_->PopScope();
  }
  ClearLocals();
  ClearTempValues();
  ClearSymbolAliases();
}

// Wrapper to expose single function entry point
llvm::Module* EmitLLVM(const IRDecls& decls, LowerCtx& ctx, llvm::LLVMContext& llvm_ctx) {
  auto emitter = std::make_unique<LLVMEmitter>(llvm_ctx, "cursive_module");
  llvm::Module* m = emitter->EmitModule(decls, ctx);

  (void)emitter->ReleaseModule();

  return m;
}

} // namespace cursive0::codegen
