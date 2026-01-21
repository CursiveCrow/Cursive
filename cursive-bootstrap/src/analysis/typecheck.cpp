#include "cursive0/sema/typecheck.h"

#include <vector>

#include "cursive0/core/diagnostics.h"
#include "cursive0/sema/collect_toplevel.h"
#include "cursive0/sema/init_planner.h"
#include "cursive0/sema/type_decls.h"

namespace cursive0::sema {

TypecheckResult TypecheckModules(ScopeContext& ctx,
                                 const std::vector<syntax::ASTModule>& modules) {
  TypecheckResult result;
  ExprTypeMap* prev_expr_types = ctx.expr_types;
  ctx.expr_types = &result.expr_types;
  struct ExprTypesReset {
    ScopeContext& ctx;
    ExprTypeMap* prev;
    ~ExprTypesReset() { ctx.expr_types = prev; }
  } expr_types_reset{ctx, prev_expr_types};
  const auto name_maps = CollectNameMaps(ctx);
  if (!name_maps.diags.empty()) {
    result.diags.insert(result.diags.end(),
                        name_maps.diags.begin(),
                        name_maps.diags.end());
  }
  if (core::HasError(result.diags)) {
    result.ok = false;
    return result;
  }

  const auto decls = DeclTypingModules(ctx, modules, name_maps.name_maps);
  if (!decls.diags.empty()) {
    result.diags.insert(result.diags.end(),
                        decls.diags.begin(),
                        decls.diags.end());
  }

  if (!core::HasError(result.diags)) {
    const auto init_plan = BuildInitPlan(ctx, name_maps.name_maps);
    if (!init_plan.diags.empty()) {
      result.diags.insert(result.diags.end(),
                          init_plan.diags.begin(),
                          init_plan.diags.end());
    }
    if (init_plan.ok) {
      result.init_plan = init_plan.plan;
    }
  }

  if (!core::HasError(result.diags)) {
    const bool require_main = !ctx.project ||
        ctx.project->assembly.kind == "executable";
    if (require_main) {
      const auto main_check = MainCheckProject(ctx, modules);
      if (!main_check.diags.empty()) {
        result.diags.insert(result.diags.end(),
                            main_check.diags.begin(),
                            main_check.diags.end());
      }
    } else {
      SPEC_RULE("Main-Bypass-Lib");
    }
  }

  result.ok = !core::HasError(result.diags);
  return result;
}

}  // namespace cursive0::sema
