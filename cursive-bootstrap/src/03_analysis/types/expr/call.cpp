// =================================================================
// File: 03_analysis/types/expr/call.cpp
// Construct: Call Expression Type Checking
// Spec Section: 5.2.12, 13.1.2
// Spec Rules: T-Call, T-Generic-Call
// =================================================================
#include "cursive0/03_analysis/types/expr/call.h"

#include "cursive0/00_core/assert_spec.h"
#include "cursive0/03_analysis/composite/records.h"
#include "cursive0/03_analysis/generics/monomorphize.h"
#include "cursive0/03_analysis/memory/calls.h"
#include "cursive0/03_analysis/resolve/scopes.h"
#include "cursive0/03_analysis/resolve/scopes_lookup.h"
#include "cursive0/03_analysis/types/type_expr.h"
#include "cursive0/03_analysis/types/type_lower.h"

namespace cursive0::analysis::expr {

namespace {

static inline void SpecDefsCall() {
  SPEC_DEF("T-Call", "5.2.12");
  SPEC_DEF("T-Generic-Call", "13.1.2");
  SPEC_DEF("T-Record-Default", "5.2.12");
}

static const syntax::ASTModule* FindModuleByPathForGeneric(
    const ScopeContext& ctx,
    const syntax::ModulePath& path) {
  for (const auto& mod : ctx.sigma.mods) {
    if (mod.path == path) {
      return &mod;
    }
  }
  return nullptr;
}

static const syntax::ProcedureDecl* FindProcedureInModule(
    const syntax::ASTModule& module, std::string_view name) {
  for (const auto& item : module.items) {
    if (const auto* proc = std::get_if<syntax::ProcedureDecl>(&item)) {
      if (IdEq(proc->name, name)) {
        return proc;
      }
    }
  }
  return nullptr;
}

}  // namespace

std::optional<TypeSubst> BuildGenericCallSubst(
    const ScopeContext& ctx,
    const syntax::ExprPtr& callee,
    const std::vector<std::shared_ptr<syntax::Type>>& generic_args) {
  if (!callee) {
    return std::nullopt;
  }

  std::string name;
  std::optional<syntax::ModulePath> origin;

  if (const auto* ident = std::get_if<syntax::IdentifierExpr>(&callee->node)) {
    const auto ent = ResolveValueName(ctx, ident->name);
    if (!ent || !ent->origin_opt) {
      return std::nullopt;
    }
    origin = ent->origin_opt;
    name = ent->target_opt.value_or(std::string(ident->name));
  } else if (const auto* path_expr = std::get_if<syntax::PathExpr>(&callee->node)) {
    origin = path_expr->path;
    name = path_expr->name;
  } else {
    return std::nullopt;
  }

  const auto* module = FindModuleByPathForGeneric(ctx, *origin);
  if (!module) {
    return std::nullopt;
  }

  const auto* proc = FindProcedureInModule(*module, name);
  if (!proc || !proc->generic_params) {
    return std::nullopt;
  }

  if (generic_args.size() != proc->generic_params->params.size()) {
    return std::nullopt;
  }

  std::vector<TypeRef> lowered_args;
  lowered_args.reserve(generic_args.size());
  for (const auto& arg : generic_args) {
    const auto lowered = LowerType(ctx, arg);
    if (!lowered.ok) {
      return std::nullopt;
    }
    lowered_args.push_back(lowered.type);
  }

  return BuildSubstitution(proc->generic_params->params, lowered_args);
}

ExprTypeResult TypeCallExprImpl(const ScopeContext& ctx,
                                const StmtTypeContext& type_ctx,
                                const syntax::CallExpr& node,
                                const TypeEnv& env) {
  SpecDefsCall();

  auto type_expr = [&](const syntax::ExprPtr& inner) {
    return TypeExpr(ctx, type_ctx, inner, env);
  };

  // Special case: RegionOptions default constructor
  if (node.callee && node.args.empty()) {
    if (const auto* ident =
            std::get_if<syntax::IdentifierExpr>(&node.callee->node);
        ident && IdEq(ident->name, "RegionOptions")) {
      ExprTypeResult r;
      r.ok = true;
      r.type = MakeTypePath({"RegionOptions"});
      SPEC_RULE("T-Record-Default");
      return r;
    }
  }

  // Try record default constructor call
  const auto record =
      TypeRecordDefaultCall(ctx, node.callee, node.args, type_expr);
  if (record.ok || record.diag_id.has_value()) {
    return record;
  }

  PlaceTypeFn type_place = [&](const syntax::ExprPtr& inner) {
    return TypePlace(ctx, type_ctx, inner, env);
  };

  // Handle generic procedure calls (§13.1.2 T-Generic-Call)
  if (!node.generic_args.empty()) {
    const auto subst_opt = BuildGenericCallSubst(ctx, node.callee, node.generic_args);
    if (!subst_opt.has_value()) {
      ExprTypeResult r;
      r.diag_id = "Generic-Call-Subst-Fail";
      return r;
    }
    const auto call = TypeCallWithSubst(ctx, node.callee, node.args,
                                         *subst_opt, type_expr, &type_place);
    ExprTypeResult r;
    if (!call.ok) {
      r.diag_id = call.diag_id;
      return r;
    }
    if (type_ctx.require_pure) {
      const auto callee_type = type_expr(node.callee);
      if (callee_type.ok) {
        const auto* func =
            std::get_if<TypeFunc>(&StripPerm(callee_type.type)->node);
        if (func && !ParamsPure(ctx, func->params)) {
          r.diag_id = "E-SEM-2802";
          return r;
        }
      }
    }
    r.ok = true;
    r.type = call.type;
    SPEC_RULE("T-Generic-Call");
    return r;
  }

  // Non-generic call path
  const auto call =
      TypeCall(ctx, node.callee, node.args, type_expr, &type_place);
  ExprTypeResult r;
  if (!call.ok) {
    r.diag_id = call.diag_id;
    return r;
  }
  if (type_ctx.require_pure) {
    const auto callee_type = type_expr(node.callee);
    if (callee_type.ok) {
      const auto* func =
          std::get_if<TypeFunc>(&StripPerm(callee_type.type)->node);
      if (func && !ParamsPure(ctx, func->params)) {
        r.diag_id = "E-SEM-2802";
        return r;
      }
    }
  }
  r.ok = true;
  r.type = call.type;
  return r;
}

}  // namespace cursive0::analysis::expr
