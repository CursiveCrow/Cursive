// =================================================================
// File: 03_analysis/types/expr/if.cpp
// Construct: If Expression Type Checking
// Spec Section: 5.2.12
// Spec Rules: T-If, T-If-No-Else, Chk-If, Chk-If-No-Else
// =================================================================
#include "cursive0/03_analysis/types/expr/if.h"

#include "cursive0/00_core/assert_spec.h"
#include "cursive0/03_analysis/types/type_equiv.h"
#include "cursive0/03_analysis/types/type_expr.h"

namespace cursive0::analysis::expr {

// (T-If), (T-If-No-Else)
ExprTypeResult TypeIfExprImpl(const ScopeContext& ctx,
                              const StmtTypeContext& type_ctx,
                              const syntax::IfExpr& expr,
                              const TypeEnv& env) {
  ExprTypeResult result;

  const auto cond = TypeExpr(ctx, type_ctx, expr.cond, env);
  if (!cond.ok) {
    result.diag_id = cond.diag_id;
    return result;
  }
  if (!IsPrimType(cond.type, "bool")) {
    return result;
  }

  const auto then_result = TypeExpr(ctx, type_ctx, expr.then_expr, env);
  if (!then_result.ok) {
    result.diag_id = then_result.diag_id;
    return result;
  }

  if (!expr.else_expr) {
    if (!IsPrimType(then_result.type, "()")) {
      return result;
    }
    SPEC_RULE("T-If-No-Else");
    result.ok = true;
    result.type = MakeTypePrim("()");
    return result;
  }

  const auto else_result = TypeExpr(ctx, type_ctx, expr.else_expr, env);
  if (!else_result.ok) {
    result.diag_id = else_result.diag_id;
    return result;
  }

  const auto equiv = TypeEquiv(then_result.type, else_result.type);
  if (!equiv.ok) {
    result.diag_id = equiv.diag_id;
    return result;
  }
  if (!equiv.equiv) {
    return result;
  }

  SPEC_RULE("T-If");
  result.ok = true;
  result.type = then_result.type;
  return result;
}

// (Chk-If), (Chk-If-No-Else)
CheckResult CheckIfExprImpl(const ScopeContext& ctx,
                            const StmtTypeContext& type_ctx,
                            const syntax::IfExpr& expr,
                            const TypeRef& expected,
                            const TypeEnv& env) {
  CheckResult result;

  const auto cond = TypeExpr(ctx, type_ctx, expr.cond, env);
  if (!cond.ok) {
    result.diag_id = cond.diag_id;
    return result;
  }
  if (!IsPrimType(cond.type, "bool")) {
    return result;
  }

  const auto then_check =
      CheckExprAgainst(ctx, type_ctx, expr.then_expr, expected, env);
  if (!then_check.ok) {
    result.diag_id = then_check.diag_id;
    return result;
  }

  if (!expr.else_expr) {
    if (!IsPrimType(expected, "()")) {
      return result;
    }
    SPEC_RULE("Chk-If-No-Else");
    result.ok = true;
    return result;
  }

  const auto else_check =
      CheckExprAgainst(ctx, type_ctx, expr.else_expr, expected, env);
  if (!else_check.ok) {
    result.diag_id = else_check.diag_id;
    return result;
  }

  SPEC_RULE("Chk-If");
  result.ok = true;
  return result;
}

}  // namespace cursive0::analysis::expr
