#include "cursive0/codegen/llvm/llvm_emit.h"
#include "cursive0/codegen/abi/abi.h"
#include "cursive0/codegen/layout/layout.h"
#include "cursive0/codegen/mangle.h"
#include "cursive0/core/assert_spec.h"
#include "cursive0/runtime/runtime_interface.h"
#include "cursive0/analysis/caps/cap_filesystem.h"
#include "cursive0/analysis/caps/cap_heap.h"
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
    // Polymorphic value parameter; default to unit for declaration if unknown.
    params.push_back(MakeParam("value", std::nullopt, TypePrim("()")));
    declare_fn(RegionSymAlloc(), params, TypePrim("()"), false);
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
        "view", "view_string", "append", "length", "is_empty",
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
  }
}

} // namespace cursive0::codegen
