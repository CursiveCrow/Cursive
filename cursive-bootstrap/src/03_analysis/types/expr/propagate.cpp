// =================================================================
// File: 03_analysis/types/expr/propagate.cpp
// Construct: Propagate Expression Type Checking (?)
// Spec Section: 5.2.12
// Spec Rules: T-Propagate
// =================================================================
#include "cursive0/03_analysis/types/expr/propagate.h"

#include "cursive0/00_core/assert_spec.h"
#include "cursive0/03_analysis/types/subtyping.h"
#include "cursive0/03_analysis/types/type_expr.h"

namespace cursive0::analysis::expr {

// (T-Propagate)
ExprTypeResult TypePropagateExprImpl(const ScopeContext& ctx,
                                     const StmtTypeContext& type_ctx,
                                     const syntax::PropagateExpr& expr,
                                     const TypeEnv& env) {
  ExprTypeResult result;

  const auto inner = TypeExpr(ctx, type_ctx, expr.value, env);
  if (!inner.ok) {
    result.diag_id = inner.diag_id;
    return result;
  }

  const auto stripped = StripPerm(inner.type);
  if (!stripped) {
    return result;
  }

  const auto* union_type = std::get_if<TypeUnion>(&stripped->node);
  if (!union_type) {
    return result;
  }

  if (union_type->members.empty()) {
    return result;
  }

  if (!type_ctx.return_type) {
    return result;
  }

  std::optional<TypeRef> success;
  for (const auto& member : union_type->members) {
    const auto sub = Subtyping(ctx, member, type_ctx.return_type);
    if (!sub.ok) {
      result.diag_id = sub.diag_id;
      return result;
    }
    if (sub.subtype) {
      continue;
    }
    if (success.has_value()) {
      return result;
    }
    success = member;
  }

  if (!success.has_value()) {
    return result;
  }

  SPEC_RULE("T-Propagate");
  result.ok = true;
  result.type = *success;
  return result;
}

}  // namespace cursive0::analysis::expr
