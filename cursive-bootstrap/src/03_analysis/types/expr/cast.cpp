// =================================================================
// File: 03_analysis/types/expr/cast.cpp
// Construct: Cast Expression Type Checking
// Spec Section: 5.2.12
// Spec Rules: T-Cast, CastValid
// =================================================================
#include "cursive0/03_analysis/types/expr/cast.h"

#include "cursive0/00_core/assert_spec.h"
#include "cursive0/03_analysis/types/type_expr.h"
#include "cursive0/03_analysis/types/type_lower.h"

namespace cursive0::analysis::expr {

ExprTypeResult TypeCastExprImpl(const ScopeContext& ctx,
                                const StmtTypeContext& type_ctx,
                                const syntax::CastExpr& expr,
                                const TypeEnv& env) {
  ExprTypeResult result;

  if (!expr.value || !expr.type) {
    return result;
  }

  // Type the source expression
  const auto value_result = TypeExpr(ctx, type_ctx, expr.value, env);
  if (!value_result.ok) {
    result.diag_id = value_result.diag_id;
    return result;
  }

  // Lower the target type
  const auto target = LowerType(ctx, expr.type);
  if (!target.ok) {
    result.diag_id = target.diag_id;
    return result;
  }

  // Check if cast is valid
  if (!CastValid(value_result.type, target.type)) {
    SPEC_RULE("T-Cast-Invalid");
    result.diag_id = "T-Cast-Invalid";
    return result;
  }

  SPEC_RULE("T-Cast");
  result.ok = true;
  result.type = target.type;
  return result;
}

}  // namespace cursive0::analysis::expr
