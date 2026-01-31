// =================================================================
// File: 03_analysis/types/expr/move.cpp
// Construct: Move Expression Type Checking
// Spec Section: 5.2.12
// Spec Rules: T-Move
// =================================================================
#include "cursive0/03_analysis/types/expr/move.h"

#include "cursive0/00_core/assert_spec.h"
#include "cursive0/03_analysis/types/type_expr.h"

namespace cursive0::analysis::expr {

// (T-Move)
ExprTypeResult TypeMoveExprImpl(const ScopeContext& ctx,
                                const StmtTypeContext& type_ctx,
                                const syntax::MoveExpr& expr,
                                const TypeEnv& env) {
  ExprTypeResult result;
  if (type_ctx.require_pure) {
    result.diag_id = "E-SEM-2802";
    return result;
  }

  const auto place = TypePlace(ctx, type_ctx, expr.place, env);
  if (!place.ok) {
    result.diag_id = place.diag_id;
    return result;
  }

  SPEC_RULE("T-Move");
  result.ok = true;
  result.type = place.type;
  return result;
}

}  // namespace cursive0::analysis::expr
