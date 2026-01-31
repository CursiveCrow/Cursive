// =================================================================
// File: 03_analysis/types/expr/alloc.cpp
// Construct: Allocation Expression Type Checking
// Spec Section: 5.2.12
// Spec Rules: T-Alloc-Explicit, T-Alloc-Implicit, Alloc-Region-NotFound-Err,
//             Alloc-Implicit-NoRegion-Err
// =================================================================
#include "cursive0/03_analysis/types/expr/alloc.h"

#include "cursive0/00_core/assert_spec.h"
#include "cursive0/03_analysis/memory/regions.h"
#include "cursive0/03_analysis/types/type_expr.h"

namespace cursive0::analysis::expr {

ExprTypeResult TypeAllocExprImpl(const ScopeContext& ctx,
                                 const StmtTypeContext& type_ctx,
                                 const syntax::AllocExpr& expr,
                                 const TypeEnv& env) {
  ExprTypeResult result;
  if (type_ctx.require_pure) {
    result.diag_id = "E-SEM-2802";
    return result;
  }
  if (!expr.value) {
    return result;
  }

  if (expr.region_opt.has_value()) {
    const auto binding = BindOf(env, *expr.region_opt);
    if (!binding.has_value()) {
      result.diag_id = "ResolveExpr-Ident-Err";
      return result;
    }
    if (!RegionActiveType(binding->type)) {
      SPEC_RULE("Alloc-Region-NotFound-Err");
      result.diag_id = "Alloc-Region-NotFound-Err";
      return result;
    }
    const auto inner = TypeExpr(ctx, type_ctx, expr.value, env);
    if (!inner.ok) {
      result.diag_id = inner.diag_id;
      return result;
    }
    SPEC_RULE("T-Alloc-Explicit");
    result.ok = true;
    result.type = inner.type;
    return result;
  }

  const auto region = InnermostActiveRegion(env);
  if (!region.has_value()) {
    SPEC_RULE("Alloc-Implicit-NoRegion-Err");
    result.diag_id = "Alloc-Implicit-NoRegion-Err";
    return result;
  }

  const auto inner = TypeExpr(ctx, type_ctx, expr.value, env);
  if (!inner.ok) {
    result.diag_id = inner.diag_id;
    return result;
  }
  SPEC_RULE("T-Alloc-Implicit");
  result.ok = true;
  result.type = inner.type;
  return result;
}

}  // namespace cursive0::analysis::expr
