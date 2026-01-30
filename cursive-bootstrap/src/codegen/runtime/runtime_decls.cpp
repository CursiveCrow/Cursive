#include "cursive0/codegen/llvm/llvm_emit.h"
#include "cursive0/codegen/abi/abi.h"
#include "cursive0/codegen/layout/layout.h"
#include "cursive0/codegen/mangle.h"
#include "cursive0/core/assert_spec.h"
#include "cursive0/runtime/runtime_interface.h"
#include "cursive0/analysis/caps/cap_filesystem.h"
#include "cursive0/analysis/caps/cap_heap.h"
#include "cursive0/analysis/caps/cap_concurrency.h"
#include "cursive0/analysis/resolve/scopes.h"
#include "cursive0/analysis/memory/string_bytes.h"
#include "cursive0/analysis/types/types.h"

// LLVM Includes
#include "llvm/IR/Attributes.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"

namespace cursive0::codegen {

namespace {

analysis::ScopeContext BuildScope(const LowerCtx* ctx) {
  analysis::ScopeContext scope;
  if (ctx && ctx->sigma) {
    scope.sigma = *ctx->sigma;
    scope.current_module = ctx->module_path;
  }
  return scope;
}

IRParam MakeParam(std::string name,
                  std::optional<analysis::ParamMode> mode,
                  analysis::TypeRef type) {
  IRParam param;
  param.mode = mode;
  param.name = std::move(name);
  param.type = std::move(type);
  return param;
}

std::optional<analysis::ParamMode> LowerParamMode(std::optional<syntax::ParamMode> mode) {
  if (!mode.has_value()) {
    return std::nullopt;
  }
  switch (*mode) {
    case syntax::ParamMode::Move:
      return analysis::ParamMode::Move;
  }
  return std::nullopt;
}

analysis::TypeRef TypePrim(std::string_view name) {
  return analysis::MakeTypePrim(std::string(name));
}

analysis::TypeRef TypePath(std::initializer_list<std::string> names) {
  analysis::TypePath path;
  for (const auto& name : names) {
    path.push_back(name);
  }
  return analysis::MakeTypePath(std::move(path));
}

analysis::TypeRef TypePtrU8() {
  return analysis::MakeTypePtr(analysis::MakeTypePrim("u8"),
                               analysis::PtrState::Valid);
}

analysis::TypeRef TypeModalState(std::initializer_list<std::string> path,
                             std::string_view state) {
  analysis::TypePath out;
  for (const auto& name : path) {
    out.push_back(name);
  }
  return analysis::MakeTypeModalState(std::move(out), std::string(state));
}

analysis::TypeRef TypeUnion(std::vector<analysis::TypeRef> members) {
  return analysis::MakeTypeUnion(std::move(members));
}

}  // namespace

void LLVMEmitter::DeclareRuntime() {
  SPEC_RULE("RuntimeDecls");
  SPEC_RULE("RuntimeDeclsCover");
  SPEC_RULE("RuntimeDeclsOk");
  SPEC_RULE("DeclAttrsOk");

  const analysis::ScopeContext scope = BuildScope(current_ctx_);

  auto declare_fn = [&](const std::string& name,
                        const std::vector<IRParam>& params,
                        analysis::TypeRef ret,
                        bool noreturn) -> llvm::Function* {
    ABICallResult abi = ComputeCallABI(params, ret);
    llvm::FunctionType* ft = abi.func_type;
    if (!ft) {
      ft = llvm::FunctionType::get(llvm::Type::getVoidTy(context_), {}, false);
    }
    llvm::Function* f = llvm::Function::Create(
        ft, llvm::Function::ExternalLinkage, name, module_.get());
    f->setCallingConv(llvm::CallingConv::C);
    f->addFnAttr(llvm::Attribute::NoUnwind);
    if (noreturn) {
      f->addFnAttr(llvm::Attribute::NoReturn);
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

    functions_[name] = f;
    if (current_ctx_) {
      ProcIR sig;
      sig.symbol = name;
      sig.params = params;
      sig.ret = ret;
      current_ctx_->RegisterProcSig(sig);
    }
    return f;
  };

  // Panic
  {
    SPEC_RULE("RuntimeSym-Panic");
    std::vector<IRParam> params;
    params.push_back(MakeParam("code", analysis::ParamMode::Move, TypePrim("u32")));
    declare_fn(RuntimePanicSym(), params, TypePrim("!"), true);
  }

  // Context init
  {
    std::vector<IRParam> params;
    declare_fn(ContextInitSym(), params, TypePath({"Context"}), false);
  }

  // Spec trace emit
  {
    std::vector<IRParam> params;
    params.push_back(MakeParam("rule_id", std::nullopt,
                               analysis::MakeTypeString(analysis::StringState::View)));
    params.push_back(MakeParam("payload", std::nullopt,
                               analysis::MakeTypeString(analysis::StringState::View)));
    declare_fn(RuntimeSpecTraceEmitSym(), params, TypePrim("()"), false);
  }

  // String/Bytes drop
  {
    std::vector<IRParam> params;
    params.push_back(MakeParam("value", analysis::ParamMode::Move,
                               analysis::MakeTypeString(analysis::StringState::Managed)));
    declare_fn(BuiltinSymStringDropManaged(), params, TypePrim("()"), false);
  }
  {
    std::vector<IRParam> params;
    params.push_back(MakeParam("value", analysis::ParamMode::Move,
                               analysis::MakeTypeBytes(analysis::BytesState::Managed)));
    declare_fn(BuiltinSymBytesDropManaged(), params, TypePrim("()"), false);
  }

  // Region procs
  {
    std::vector<IRParam> params;
    params.push_back(MakeParam("options", std::nullopt, TypePath({"RegionOptions"})));
    declare_fn(RegionSymNewScoped(), params,
               TypeModalState({"Region"}, "Active"), false);
  }
  {
    std::vector<IRParam> params;
    auto region_active = TypeModalState({"Region"}, "Active");
    auto self_ty = analysis::MakeTypePerm(analysis::Permission::Unique, region_active);
    params.push_back(MakeParam("self", std::nullopt, self_ty));
    params.push_back(MakeParam("size", std::nullopt, TypePrim("usize")));
    params.push_back(MakeParam("align", std::nullopt, TypePrim("usize")));
    auto ret_ty = analysis::MakeTypeRawPtr(analysis::RawPtrQual::Mut,
                                           analysis::MakeTypePrim("u8"));
    declare_fn(RegionSymAlloc(), params, ret_ty, false);
  }
  {
    std::vector<IRParam> params;
    auto region_active = TypeModalState({"Region"}, "Active");
    auto self_ty = analysis::MakeTypePerm(analysis::Permission::Unique, region_active);
    params.push_back(MakeParam("self", std::nullopt, self_ty));
    declare_fn(RegionSymMark(), params, TypePrim("usize"), false);
  }
  {
    std::vector<IRParam> params;
    auto region_active = TypeModalState({"Region"}, "Active");
    auto self_ty = analysis::MakeTypePerm(analysis::Permission::Unique, region_active);
    params.push_back(MakeParam("self", std::nullopt, self_ty));
    params.push_back(MakeParam("mark", std::nullopt, TypePrim("usize")));
    declare_fn(RegionSymResetTo(), params, TypePrim("()"), false);
  }
  {
    std::vector<IRParam> params;
    auto region_active = TypeModalState({"Region"}, "Active");
    auto self_ty = analysis::MakeTypePerm(analysis::Permission::Unique, region_active);
    params.push_back(MakeParam("self", std::nullopt, self_ty));
    declare_fn(RegionSymResetUnchecked(), params,
               TypeModalState({"Region"}, "Active"), false);
  }
  {
    std::vector<IRParam> params;
    auto region_active = TypeModalState({"Region"}, "Active");
    auto self_ty = analysis::MakeTypePerm(analysis::Permission::Unique, region_active);
    params.push_back(MakeParam("self", std::nullopt, self_ty));
    declare_fn(RegionSymFreeze(), params,
               TypeModalState({"Region"}, "Frozen"), false);
  }
  {
    std::vector<IRParam> params;
    auto region_frozen = TypeModalState({"Region"}, "Frozen");
    auto self_ty = analysis::MakeTypePerm(analysis::Permission::Unique, region_frozen);
    params.push_back(MakeParam("self", std::nullopt, self_ty));
    declare_fn(RegionSymThaw(), params,
               TypeModalState({"Region"}, "Active"), false);
  }
  {
    std::vector<IRParam> params;
    auto region_active = TypeModalState({"Region"}, "Active");
    auto region_frozen = TypeModalState({"Region"}, "Frozen");
    std::vector<analysis::TypeRef> members;
    members.push_back(region_active);
    members.push_back(region_frozen);
    auto union_ty = TypeUnion(std::move(members));
    auto self_ty = analysis::MakeTypePerm(analysis::Permission::Unique, union_ty);
    params.push_back(MakeParam("self", std::nullopt, self_ty));
    declare_fn(RegionSymFreeUnchecked(), params,
               TypeModalState({"Region"}, "Freed"), false);
  }
  {
    std::vector<IRParam> params;
    auto addr_ty = analysis::MakeTypeRawPtr(analysis::RawPtrQual::Imm,
                                            analysis::MakeTypePrim("u8"));
    params.push_back(MakeParam("addr", std::nullopt, addr_ty));
    declare_fn(RegionSymAddrIsActive(), params, TypePrim("bool"), false);
  }
  {
    std::vector<IRParam> params;
    auto addr_ty = analysis::MakeTypeRawPtr(analysis::RawPtrQual::Imm,
                                            analysis::MakeTypePrim("u8"));
    params.push_back(MakeParam("addr", std::nullopt, addr_ty));
    params.push_back(MakeParam("base", std::nullopt, addr_ty));
    declare_fn(RegionSymAddrTagFrom(), params, TypePrim("()"), false);
  }

  // String builtins
  {
    syntax::ModulePath path;
    path.push_back("string");
    const std::vector<std::string> names = {
        "from", "as_view", "to_managed", "clone_with",
        "append", "length", "is_empty",
    };
    for (const auto& name : names) {
      auto func_type = analysis::LookupStringBytesBuiltinType(path, name);
      if (!func_type || !std::holds_alternative<analysis::TypeFunc>((*func_type)->node)) {
        if (current_ctx_) {
          current_ctx_->ReportCodegenFailure();
        }
        continue;
      }
      const auto& func = std::get<analysis::TypeFunc>((*func_type)->node);
      std::vector<IRParam> params;
      params.reserve(func.params.size());
      for (std::size_t i = 0; i < func.params.size(); ++i) {
        params.push_back(MakeParam("arg" + std::to_string(i),
                                   func.params[i].mode,
                                   func.params[i].type));
      }
      declare_fn(BuiltinSym("string::" + name), params, func.ret, false);
    }
  }

  // Bytes builtins
  {
    syntax::ModulePath path;
    path.push_back("bytes");
    const std::vector<std::string> names = {
        "with_capacity", "from_slice", "as_view", "to_managed",
        "view", "view_string", "as_slice", "append", "length", "is_empty",
    };
    for (const auto& name : names) {
      auto func_type = analysis::LookupStringBytesBuiltinType(path, name);
      if (!func_type || !std::holds_alternative<analysis::TypeFunc>((*func_type)->node)) {
        if (current_ctx_) {
          current_ctx_->ReportCodegenFailure();
        }
        continue;
      }
      const auto& func = std::get<analysis::TypeFunc>((*func_type)->node);
      std::vector<IRParam> params;
      params.reserve(func.params.size());
      for (std::size_t i = 0; i < func.params.size(); ++i) {
        params.push_back(MakeParam("arg" + std::to_string(i),
                                   func.params[i].mode,
                                   func.params[i].type));
      }
      declare_fn(BuiltinSym("bytes::" + name), params, func.ret, false);
    }
  }

  // FileSystem builtins
  {
    const std::vector<std::string> names = {
        "open_read", "open_write", "open_append", "create_write",
        "read_file", "read_bytes", "write_file", "write_stdout",
        "write_stderr", "exists", "remove", "open_dir", "create_dir",
        "ensure_dir", "kind", "restrict",
    };

    for (const auto& name : names) {
      const auto sig = analysis::LookupFileSystemMethodSig(name);
      if (!sig.has_value()) {
        if (current_ctx_) {
          current_ctx_->ReportCodegenFailure();
        }
        continue;
      }

      std::vector<IRParam> params;
      auto self_ty = analysis::MakeTypePerm(sig->recv_perm,
                                        analysis::MakeTypeDynamic({"FileSystem"}));
      params.push_back(MakeParam("self", std::nullopt, self_ty));

      for (const auto& param : sig->params) {
        if (!param.type) {
          continue;
        }
        auto lowered = LowerTypeForLayout(scope, param.type);
        if (!lowered.has_value()) {
          if (current_ctx_) {
            current_ctx_->ReportCodegenFailure();
          }
          continue;
        }
        params.push_back(MakeParam(param.name, param.mode.has_value()
                                                ? std::optional<analysis::ParamMode>(analysis::ParamMode::Move)
                                                : std::nullopt,
                                   *lowered));
      }
      declare_fn(BuiltinSym("FileSystem::" + name), params, sig->ret, false);
    }
  }

  // HeapAllocator builtins
  {
    const std::vector<std::string> names = {
        "with_quota", "alloc_raw", "dealloc_raw",
    };

    for (const auto& name : names) {
      const auto sig = analysis::LookupHeapAllocatorMethodSig(name);
      if (!sig.has_value()) {
        if (current_ctx_) {
          current_ctx_->ReportCodegenFailure();
        }
        continue;
      }

      std::vector<IRParam> params;
      auto self_ty = analysis::MakeTypePerm(sig->recv_perm,
                                        analysis::MakeTypeDynamic({"HeapAllocator"}));
      params.push_back(MakeParam("self", std::nullopt, self_ty));

      for (const auto& param : sig->params) {
        if (!param.type) {
          continue;
        }
        auto lowered = LowerTypeForLayout(scope, param.type);
        if (!lowered.has_value()) {
          if (current_ctx_) {
            current_ctx_->ReportCodegenFailure();
          }
          continue;
        }
        params.push_back(MakeParam(param.name, param.mode.has_value()
                                                ? std::optional<analysis::ParamMode>(analysis::ParamMode::Move)
                                                : std::nullopt,
                                   *lowered));
      }
      declare_fn(BuiltinSym("HeapAllocator::" + name), params, sig->ret, false);
    }
  }

  // File/DirIter modal methods and transitions
  {
    auto declare_modal = [&](const syntax::ModalDecl& decl) {
      analysis::TypePath modal_path;
      modal_path.push_back(decl.name);

      for (const auto& state : decl.states) {
        for (const auto& member : state.members) {
          if (const auto* method = std::get_if<syntax::StateMethodDecl>(&member)) {
            std::vector<IRParam> params;
            auto self_ty = TypeModalState({decl.name}, state.name);
            params.push_back(MakeParam("self", std::nullopt, self_ty));

            for (const auto& param : method->params) {
              if (!param.type) {
                continue;
              }
              auto lowered = LowerTypeForLayout(scope, param.type);
              if (!lowered.has_value()) {
                if (current_ctx_) {
                  current_ctx_->ReportCodegenFailure();
                }
                continue;
              }
              params.push_back(MakeParam(param.name, LowerParamMode(param.mode), *lowered));
            }

            analysis::TypeRef ret = TypePrim("()");
            if (method->return_type_opt) {
              auto lowered = LowerTypeForLayout(scope, method->return_type_opt);
              if (lowered.has_value()) {
                ret = *lowered;
              } else if (current_ctx_) {
                current_ctx_->ReportCodegenFailure();
              }
            }

            const std::string sym = MangleStateMethod(modal_path, state.name, *method);
            declare_fn(sym, params, ret, false);
            continue;
          }

          if (const auto* trans = std::get_if<syntax::TransitionDecl>(&member)) {
            std::vector<IRParam> params;
            auto self_ty = TypeModalState({decl.name}, state.name);
            params.push_back(MakeParam("self", analysis::ParamMode::Move, self_ty));

            for (const auto& param : trans->params) {
              if (!param.type) {
                continue;
              }
              auto lowered = LowerTypeForLayout(scope, param.type);
              if (!lowered.has_value()) {
                if (current_ctx_) {
                  current_ctx_->ReportCodegenFailure();
                }
                continue;
              }
              params.push_back(MakeParam(param.name, LowerParamMode(param.mode), *lowered));
            }

            analysis::TypeRef ret = TypeModalState({decl.name}, trans->target_state);
            const std::string sym = MangleTransition(modal_path, state.name, *trans);
            declare_fn(sym, params, ret, false);
          }
        }
      }
    };

    declare_modal(analysis::BuildFileModalDecl());
    declare_modal(analysis::BuildDirIterModalDecl());
    declare_modal(analysis::BuildCancelTokenModalDecl());
  }

  // Async frame allocation runtime
  {
    std::vector<IRParam> params;
    params.push_back(MakeParam("size", analysis::ParamMode::Move, TypePrim("usize")));
    params.push_back(MakeParam("align", analysis::ParamMode::Move, TypePrim("usize")));
    declare_fn(BuiltinSymAsyncAllocFrame(), params, TypePtrU8(), false);
  }
  {
    std::vector<IRParam> params;
    params.push_back(MakeParam("frame", analysis::ParamMode::Move, TypePtrU8()));
    declare_fn(BuiltinSymAsyncFreeFrame(), params, TypePrim("()"), false);
  }

  // Structured concurrency runtime
  // Note: All parameters use Move mode to ensure ByValue ABI, matching the
  // C function signatures in the runtime. Using nullopt would result in ByRef
  // passing which doesn't match the C ABI.
  {
    std::vector<IRParam> params;
    // domain is a 16-byte struct passed by invisible reference on x64 per C ABI
    params.push_back(MakeParam("domain", analysis::ParamMode::Move,
                               analysis::MakeTypeDynamic({"ExecutionDomain"})));
    params.push_back(MakeParam("cancel_token", analysis::ParamMode::Move,
                               TypePtrU8()));
    params.push_back(MakeParam("name", analysis::ParamMode::Move,
                               TypePtrU8()));
    declare_fn("cursive0_parallel_begin", params, TypePtrU8(), false);
  }
  {
    std::vector<IRParam> params;
    params.push_back(MakeParam("ctx_ptr", analysis::ParamMode::Move, TypePtrU8()));
    declare_fn("cursive0_parallel_join", params, TypePrim("i32"), false);
  }
  {
    std::vector<IRParam> params;
    params.push_back(MakeParam("env", analysis::ParamMode::Move, TypePtrU8()));
    params.push_back(MakeParam("env_size", analysis::ParamMode::Move, TypePrim("usize")));
    std::vector<analysis::TypeFuncParam> fn_params;
    fn_params.push_back({std::nullopt, TypePtrU8()});
    fn_params.push_back({std::nullopt, TypePtrU8()});
    fn_params.push_back({std::nullopt, TypePtrU8()});
    auto fn_ty = analysis::MakeTypeFunc(std::move(fn_params), TypePrim("()"));
    params.push_back(MakeParam("body", analysis::ParamMode::Move, fn_ty));
    params.push_back(MakeParam("result_size", analysis::ParamMode::Move, TypePrim("usize")));
    declare_fn("cursive0_spawn_create", params, TypePtrU8(), false);
  }
  {
    std::vector<IRParam> params;
    params.push_back(MakeParam("handle_ptr", analysis::ParamMode::Move, TypePtrU8()));
    declare_fn("cursive0_spawn_wait", params, TypePtrU8(), false);
  }
  {
    std::vector<IRParam> params;
    // Range is a 16-byte struct (start, end)
    params.push_back(MakeParam("range", analysis::ParamMode::Move, analysis::MakeTypeRange()));
    params.push_back(MakeParam("elem_size", analysis::ParamMode::Move, TypePrim("usize")));
    params.push_back(MakeParam("result_size", analysis::ParamMode::Move, TypePrim("usize")));
    std::vector<analysis::TypeFuncParam> fn_params;
    fn_params.push_back({std::nullopt, TypePtrU8()});
    fn_params.push_back({std::nullopt, TypePtrU8()});
    fn_params.push_back({std::nullopt, TypePtrU8()});
    fn_params.push_back({std::nullopt, TypePtrU8()});
    auto fn_ty = analysis::MakeTypeFunc(std::move(fn_params), TypePrim("()"));
    params.push_back(MakeParam("body", analysis::ParamMode::Move, fn_ty));
    params.push_back(MakeParam("captured_env", analysis::ParamMode::Move, TypePtrU8()));
    // reduce_op is a string@View (16 bytes) - but C expects const char*, so use ptr
    params.push_back(MakeParam("reduce_op", analysis::ParamMode::Move, TypePtrU8()));
    params.push_back(MakeParam("reduce_result", analysis::ParamMode::Move, TypePtrU8()));
    params.push_back(MakeParam("reduce_fn", analysis::ParamMode::Move, fn_ty));
    params.push_back(MakeParam("ordered", analysis::ParamMode::Move, TypePrim("i32")));
    params.push_back(MakeParam("chunk_size", analysis::ParamMode::Move, TypePrim("usize")));
    declare_fn("cursive0_dispatch_run", params, TypePrim("()"), false);
  }
  {
    std::vector<IRParam> params;
    declare_fn("cursive0_cancel_token_new", params, TypePtrU8(), false);
  }
  {
    std::vector<IRParam> params;
    params.push_back(MakeParam("token_ptr", analysis::ParamMode::Move, TypePtrU8()));
    declare_fn("cursive0_cancel_token_cancel", params, TypePrim("()"), false);
  }
  {
    std::vector<IRParam> params;
    params.push_back(MakeParam("token_ptr", analysis::ParamMode::Move, TypePtrU8()));
    declare_fn("cursive0_cancel_token_is_cancelled", params, TypePrim("i32"), false);
  }
  {
    std::vector<IRParam> params;
    params.push_back(MakeParam("ctx_ptr", std::nullopt, TypePtrU8()));
    params.push_back(MakeParam("code", std::nullopt, TypePrim("u32")));
    declare_fn("cursive0_parallel_work_panic", params, TypePrim("()"), false);
  }

  // Context and ExecutionDomain builtins
  {
    std::vector<IRParam> params;
    params.push_back(MakeParam("self", std::nullopt, TypePath({"Context"})));
    declare_fn(BuiltinSymContextCpu(), params,
               analysis::MakeTypeDynamic({"CpuDomain"}), false);
  }
  {
    std::vector<IRParam> params;
    params.push_back(MakeParam("self", std::nullopt, TypePath({"Context"})));
    declare_fn(BuiltinSymContextGpu(), params,
               analysis::MakeTypeDynamic({"GpuDomain"}), false);
  }
  {
    std::vector<IRParam> params;
    params.push_back(MakeParam("self", std::nullopt, TypePath({"Context"})));
    declare_fn(BuiltinSymContextInline(), params,
               analysis::MakeTypeDynamic({"InlineDomain"}), false);
  }
  {
    std::vector<IRParam> params;
    params.push_back(MakeParam("self", std::nullopt,
                               analysis::MakeTypeDynamic({"ExecutionDomain"})));
    declare_fn(BuiltinSymExecutionDomainName(), params,
               analysis::MakeTypeString(analysis::StringState::View), false);
  }
  {
    std::vector<IRParam> params;
    params.push_back(MakeParam("self", std::nullopt,
                               analysis::MakeTypeDynamic({"ExecutionDomain"})));
    declare_fn(BuiltinSymExecutionDomainMaxConcurrency(), params,
               TypePrim("u64"), false);
  }
  {
    std::vector<IRParam> params;
    declare_fn(BuiltinSymCancelTokenNew(), params,
               TypeModalState({"CancelToken"}, "Active"), false);
  }
}

} // namespace cursive0::codegen

