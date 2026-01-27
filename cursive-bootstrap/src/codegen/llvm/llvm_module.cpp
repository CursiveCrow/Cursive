#include "cursive0/codegen/llvm/llvm_emit.h"
#include "cursive0/codegen/abi/abi.h"
#include "cursive0/codegen/cleanup.h"
#include "cursive0/core/assert_spec.h"
#include "cursive0/core/symbols.h"

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

  EmitIR(proc.body);

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
