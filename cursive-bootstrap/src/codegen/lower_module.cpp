#include "cursive0/codegen/lower_module.h"

#include <algorithm>
#include <optional>
#include <utility>
#include <variant>

#include "cursive0/codegen/abi.h"
#include "cursive0/codegen/cleanup.h"
#include "cursive0/codegen/dyn_dispatch.h"
#include "cursive0/codegen/globals.h"
#include "cursive0/codegen/layout.h"
#include "cursive0/codegen/lower_proc.h"
#include "cursive0/codegen/lower_stmt.h"
#include "cursive0/codegen/mangle.h"
#include "cursive0/core/assert_spec.h"
#include "cursive0/sema/classes.h"
#include "cursive0/sema/record_methods.h"
#include "cursive0/sema/types.h"

namespace cursive0::codegen {

namespace {

sema::ScopeContext BuildScope(const syntax::ModulePath& module_path,
                              LowerCtx& ctx) {
  sema::ScopeContext scope;
  if (ctx.sigma) {
    scope.sigma = *ctx.sigma;
    scope.current_module = module_path;
  }
  return scope;
}

sema::LowerTypeResult LowerTypeForMethod(const sema::ScopeContext& scope,
                                         const std::shared_ptr<syntax::Type>& type) {
  if (!type) {
    return {false, std::nullopt, nullptr};
  }
  if (auto lowered = LowerTypeForLayout(scope, type)) {
    return {true, std::nullopt, *lowered};
  }
  return {false, std::nullopt, nullptr};
}

sema::TypeRef SubstSelfType(const sema::TypeRef& self,
                            const sema::TypeRef& type) {
  if (!type || !self) {
    return type;
  }
  return std::visit(
      [&](const auto& node) -> sema::TypeRef {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, sema::TypePathType>) {
          if (node.path.size() == 1 && node.path[0] == "Self") {
            return self;
          }
          return type;
        } else if constexpr (std::is_same_v<T, sema::TypePerm>) {
          return sema::MakeTypePerm(node.perm, SubstSelfType(self, node.base));
        } else if constexpr (std::is_same_v<T, sema::TypeTuple>) {
          std::vector<sema::TypeRef> elems;
          elems.reserve(node.elements.size());
          for (const auto& elem : node.elements) {
            elems.push_back(SubstSelfType(self, elem));
          }
          return sema::MakeTypeTuple(std::move(elems));
        } else if constexpr (std::is_same_v<T, sema::TypeArray>) {
          return sema::MakeTypeArray(SubstSelfType(self, node.element), node.length);
        } else if constexpr (std::is_same_v<T, sema::TypeSlice>) {
          return sema::MakeTypeSlice(SubstSelfType(self, node.element));
        } else if constexpr (std::is_same_v<T, sema::TypeUnion>) {
          std::vector<sema::TypeRef> members;
          members.reserve(node.members.size());
          for (const auto& member : node.members) {
            members.push_back(SubstSelfType(self, member));
          }
          return sema::MakeTypeUnion(std::move(members));
        } else if constexpr (std::is_same_v<T, sema::TypeFunc>) {
          std::vector<sema::TypeFuncParam> params;
          params.reserve(node.params.size());
          for (const auto& param : node.params) {
            params.push_back(sema::TypeFuncParam{param.mode,
                                                 SubstSelfType(self, param.type)});
          }
          return sema::MakeTypeFunc(std::move(params), SubstSelfType(self, node.ret));
        } else if constexpr (std::is_same_v<T, sema::TypePtr>) {
          return sema::MakeTypePtr(SubstSelfType(self, node.element), node.state);
        } else if constexpr (std::is_same_v<T, sema::TypeRawPtr>) {
          return sema::MakeTypeRawPtr(node.qual, SubstSelfType(self, node.element));
        } else {
          return type;
        }
      },
      type->node);
}

sema::TypeRef LowerReturnType(const sema::ScopeContext& scope,
                              const std::shared_ptr<syntax::Type>& ret_opt,
                              const sema::TypeRef& self_type) {
  if (!ret_opt) {
    return sema::MakeTypePrim("()");
  }
  if (auto lowered = LowerTypeForLayout(scope, ret_opt)) {
    return SubstSelfType(self_type, *lowered);
  }
  return sema::MakeTypePrim("()");
}

IRParam LowerParam(const syntax::Param& param,
                   const sema::ScopeContext& scope,
                   const sema::TypeRef& self_type) {
  IRParam out;
  out.mode = param.mode.has_value() ? std::optional<sema::ParamMode>(sema::ParamMode::Move)
                                    : std::nullopt;
  out.name = param.name;
  if (param.type) {
    if (auto lowered = LowerTypeForLayout(scope, param.type)) {
      out.type = SubstSelfType(self_type, *lowered);
    }
  }
  return out;
}

ProcIR LowerProcLike(const std::string& symbol,
                     const std::vector<IRParam>& params,
                     const sema::TypeRef& ret_type,
                     const syntax::Block& body,
                     const syntax::ModulePath& module_path,
                     LowerCtx& ctx) {
  ProcIR ir;
  ir.symbol = symbol;
  ctx.module_path = module_path;

  ctx.PushScope(false, false);

  for (const auto& param : params) {
    ir.params.push_back(param);
    const bool has_resp = param.mode.has_value();
    ctx.RegisterVar(param.name, param.type, has_resp, false);
  }

  ir.ret = ret_type ? ret_type : sema::MakeTypePrim("()");

  if (NeedsPanicOut(ir.symbol)) {
    IRParam panic_param;
    panic_param.mode = sema::ParamMode::Move;
    panic_param.name = std::string(kPanicOutName);
    panic_param.type = PanicOutType();
    ir.params.push_back(std::move(panic_param));
  }

  auto body_res = LowerBlock(body, ctx);

  CleanupPlan cleanup_plan = ComputeCleanupPlanForCurrentScope(ctx);
  IRPtr cleanup_ir = EmitCleanup(cleanup_plan, ctx);
  ctx.PopScope();

  std::vector<IRPtr> body_seq;
  if (body_res.ir) {
    body_seq.push_back(body_res.ir);
  }
  if (cleanup_ir) {
    body_seq.push_back(cleanup_ir);
  }

  IRReturn ret;
  ret.value = body_res.value;
  body_seq.push_back(MakeIR(std::move(ret)));

  ir.body = SeqIR(std::move(body_seq));
  return ir;
}

std::vector<sema::TypeRef> SortedImplementations(
    const sema::ScopeContext& scope,
    const syntax::ClassPath& class_path) {
  std::vector<std::pair<sema::TypeRef, sema::TypeKey>> types;
  for (const auto& [path_key, decl] : scope.sigma.types) {
    if (!std::holds_alternative<syntax::RecordDecl>(decl) &&
        !std::holds_alternative<syntax::EnumDecl>(decl) &&
        !std::holds_alternative<syntax::ModalDecl>(decl)) {
      continue;
    }
    sema::TypePath path;
    path.reserve(path_key.size());
    for (const auto& comp : path_key) {
      path.push_back(comp);
    }
    auto type = sema::MakeTypePath(path);
    if (sema::TypeImplementsClass(scope, type, class_path)) {
      types.push_back({type, sema::TypeKeyOf(type)});
    }
  }

  std::sort(types.begin(), types.end(),
            [](const auto& lhs, const auto& rhs) {
              return sema::TypeKeyLess(lhs.second, rhs.second);
            });

  std::vector<sema::TypeRef> out;
  out.reserve(types.size());
  for (const auto& entry : types) {
    out.push_back(entry.first);
  }
  return out;
}

std::vector<sema::TypeRef> DefaultUserList(
    const sema::ScopeContext& scope,
    const syntax::ClassPath& class_path,
    const syntax::ClassMethodDecl& method) {
  std::vector<sema::TypeRef> out;
  const auto types = SortedImplementations(scope, class_path);
  for (const auto& type : types) {
    const auto lookup = sema::LookupMethodStatic(scope, type, method.name);
    if (lookup.ok && lookup.class_method && lookup.owner_class == class_path) {
      out.push_back(type);
    }
  }
  return out;
}

ProcIR LowerRecordMethod(const syntax::RecordDecl& record,
                         const syntax::MethodDecl& method,
                         const syntax::ModulePath& module_path,
                         LowerCtx& ctx) {
  const auto scope = BuildScope(module_path, ctx);

  sema::TypePath record_path = module_path;
  record_path.push_back(record.name);
  auto self_type = sema::MakeTypePath(record_path);

  const auto recv_type = sema::RecvTypeForReceiver(
      scope, self_type, method.receiver,
      [&](const std::shared_ptr<syntax::Type>& type) {
        return LowerTypeForMethod(scope, type);
      });
  const auto recv_mode = sema::RecvModeOf(method.receiver);

  std::vector<IRParam> params;
  IRParam self_param;
  self_param.mode = recv_mode;
  self_param.name = "self";
  self_param.type = recv_type.ok ? recv_type.type : self_type;
  params.push_back(self_param);

  for (const auto& param : method.params) {
    params.push_back(LowerParam(param, scope, self_type));
  }

  const auto ret_type = LowerReturnType(scope, method.return_type_opt, self_type);
  const std::string sym = MangleMethod(record_path, method);
  return LowerProcLike(sym, params, ret_type, *method.body, module_path, ctx);
}

ProcIR LowerStateMethod(const syntax::ModalDecl& modal,
                        const syntax::StateBlock& state,
                        const syntax::StateMethodDecl& method,
                        const syntax::ModulePath& module_path,
                        LowerCtx& ctx) {
  const auto scope = BuildScope(module_path, ctx);

  sema::TypePath modal_path = module_path;
  modal_path.push_back(modal.name);
  auto state_type = sema::MakeTypeModalState(modal_path, state.name);
  auto recv_type = sema::MakeTypePerm(sema::Permission::Const, state_type);

  std::vector<IRParam> params;
  params.push_back(IRParam{std::nullopt, "self", recv_type});
  for (const auto& param : method.params) {
    params.push_back(LowerParam(param, scope, nullptr));
  }

  const auto ret_type = LowerReturnType(scope, method.return_type_opt, nullptr);
  const std::string sym = MangleStateMethod(modal_path, state.name, method);
  return LowerProcLike(sym, params, ret_type, *method.body, module_path, ctx);
}

ProcIR LowerTransition(const syntax::ModalDecl& modal,
                       const syntax::StateBlock& state,
                       const syntax::TransitionDecl& trans,
                       const syntax::ModulePath& module_path,
                       LowerCtx& ctx) {
  const auto scope = BuildScope(module_path, ctx);

  sema::TypePath modal_path = module_path;
  modal_path.push_back(modal.name);
  auto state_type = sema::MakeTypeModalState(modal_path, state.name);
  auto recv_type = sema::MakeTypePerm(sema::Permission::Unique, state_type);

  std::vector<IRParam> params;
  params.push_back(IRParam{sema::ParamMode::Move, "self", recv_type});
  for (const auto& param : trans.params) {
    params.push_back(LowerParam(param, scope, nullptr));
  }

  auto ret_type = sema::MakeTypeModalState(modal_path, trans.target_state);
  const std::string sym = MangleTransition(modal_path, state.name, trans);
  return LowerProcLike(sym, params, ret_type, *trans.body, module_path, ctx);
}

std::vector<ProcIR> LowerClassMethodBody(const syntax::ClassDecl& class_decl,
                                         const syntax::ClassMethodDecl& method,
                                         const syntax::ModulePath& module_path,
                                         LowerCtx& ctx) {
  if (!method.body_opt) {
    SPEC_RULE("CG-Item-ClassMethod-Abstract");
    return {};
  }
  SPEC_RULE("CG-Item-ClassMethod-Body");

  const auto scope = BuildScope(module_path, ctx);
  sema::TypePath class_path = module_path;
  class_path.push_back(class_decl.name);

  std::vector<ProcIR> procs;
  const auto users = DefaultUserList(scope, class_path, method);
  for (const auto& self_type : users) {
    const auto recv_type = sema::RecvTypeForReceiver(
        scope, self_type, method.receiver,
        [&](const std::shared_ptr<syntax::Type>& type) {
          return LowerTypeForMethod(scope, type);
        });
    const auto recv_mode = sema::RecvModeOf(method.receiver);

    std::vector<IRParam> params;
    IRParam self_param;
    self_param.mode = recv_mode;
    self_param.name = "self";
    self_param.type = recv_type.ok ? recv_type.type : self_type;
    params.push_back(self_param);

    for (const auto& param : method.params) {
      params.push_back(LowerParam(param, scope, self_type));
    }

    const auto ret_type = LowerReturnType(scope, method.return_type_opt, self_type);
    const std::string sym = MangleDefaultImpl(self_type, class_path, method.name);
    procs.push_back(LowerProcLike(sym, params, ret_type, *method.body_opt, module_path, ctx));
  }

  return procs;
}

}  // namespace

IRDecls LowerModule(const syntax::ASTModule& module, LowerCtx& ctx) {
  SPEC_RULE("Lower-Module");

  IRDecls decls;
  ctx.module_path = module.path;

  for (const auto& item : module.items) {
    std::visit(
        [&](const auto& node) {
          using T = std::decay_t<decltype(node)>;
          if constexpr (std::is_same_v<T, syntax::UsingDecl>) {
            SPEC_RULE("CG-Item-Using");
            return;
          } else if constexpr (std::is_same_v<T, syntax::TypeAliasDecl>) {
            SPEC_RULE("CG-Item-TypeAlias");
            return;
          } else if constexpr (std::is_same_v<T, syntax::StaticDecl>) {
            SPEC_RULE("CG-Item-Static");
            auto res = EmitGlobal(node, module.path, ctx);
            decls.insert(decls.end(), res.decls.begin(), res.decls.end());
            return;
          } else if constexpr (std::is_same_v<T, syntax::ProcedureDecl>) {
            if (node.name == "main") {
              SPEC_RULE("CG-Item-Procedure-Main");
            } else {
              SPEC_RULE("CG-Item-Procedure");
            }
            decls.push_back(LowerProc(node, module.path, ctx));
            return;
          } else if constexpr (std::is_same_v<T, syntax::RecordDecl>) {
            SPEC_RULE("CG-Item-Record");
            for (const auto& member : node.members) {
              if (const auto* method = std::get_if<syntax::MethodDecl>(&member)) {
                SPEC_RULE("CG-Item-Method");
                decls.push_back(LowerRecordMethod(node, *method, module.path, ctx));
              }
            }
            return;
          } else if constexpr (std::is_same_v<T, syntax::ModalDecl>) {
            SPEC_RULE("CG-Item-Modal");
            for (const auto& state : node.states) {
              for (const auto& member : state.members) {
                if (const auto* method = std::get_if<syntax::StateMethodDecl>(&member)) {
                  SPEC_RULE("CG-Item-StateMethod");
                  decls.push_back(LowerStateMethod(node, state, *method, module.path, ctx));
                }
              }
              for (const auto& member : state.members) {
                if (const auto* trans = std::get_if<syntax::TransitionDecl>(&member)) {
                  SPEC_RULE("CG-Item-Transition");
                  decls.push_back(LowerTransition(node, state, *trans, module.path, ctx));
                }
              }
            }
            return;
          } else if constexpr (std::is_same_v<T, syntax::ClassDecl>) {
            SPEC_RULE("CG-Item-Class");
            for (const auto& item : node.items) {
              if (const auto* method = std::get_if<syntax::ClassMethodDecl>(&item)) {
                auto procs = LowerClassMethodBody(node, *method, module.path, ctx);
                for (auto& proc : procs) {
                  decls.push_back(std::move(proc));
                }
              }
            }

            const auto scope = BuildScope(module.path, ctx);
            syntax::ClassPath class_path = module.path;
            class_path.push_back(node.name);
            const auto types = SortedImplementations(scope, class_path);
            for (const auto& type : types) {
              decls.push_back(EmitVTable(type, class_path, node, ctx));
            }
            return;
          } else if constexpr (std::is_same_v<T, syntax::EnumDecl>) {
            SPEC_RULE("CG-Item-Enum");
            return;
          } else if constexpr (std::is_same_v<T, syntax::ErrorItem>) {
            SPEC_RULE("CG-Item-ErrorItem");
            return;
          }
        },
        item);
  }

  auto init_fn = EmitModuleInitFn(module.path, module, ctx);
  decls.push_back(init_fn);

  auto deinit_fn = EmitModuleDeinitFn(module.path, module, ctx);
  decls.push_back(deinit_fn);

  return decls;
}

}  // namespace cursive0::codegen
