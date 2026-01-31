// =================================================================
// File: 03_analysis/types/expr/tuple_access.cpp
// Construct: Tuple Access Expression Type Checking
// Spec Section: 5.2.12
// Spec Rules: T-Tuple-Access, P-Tuple-Access
// =================================================================
#include "cursive0/03_analysis/types/expr/tuple_access.h"

#include "cursive0/00_core/assert_spec.h"
#include "cursive0/03_analysis/composite/tuples.h"
#include "cursive0/03_analysis/types/type_expr.h"

namespace cursive0::analysis::expr {

ExprTypeResult TypeTupleAccessExprImpl(const ScopeContext& ctx,
                                       const StmtTypeContext& type_ctx,
                                       const syntax::TupleAccessExpr& expr,
                                       const TypeEnv& env) {
  auto type_expr = [&](const syntax::ExprPtr& inner) {
    return TypeExpr(ctx, type_ctx, inner, env);
  };
  return TypeTupleAccessValue(ctx, expr, type_expr);
}

PlaceTypeResult TypeTupleAccessPlaceImpl(const ScopeContext& ctx,
                                         const StmtTypeContext& type_ctx,
                                         const syntax::TupleAccessExpr& expr,
                                         const TypeEnv& env) {
  PlaceTypeFn type_place = [&](const syntax::ExprPtr& inner) {
    return TypePlace(ctx, type_ctx, inner, env);
  };
  return TypeTupleAccessPlace(ctx, expr, type_place);
}

}  // namespace cursive0::analysis::expr
