// =================================================================
// File: 03_analysis/types/expr/sizeof.cpp
// Construct: Sizeof Expression Type Checking
// Spec Section: 5.2.12
// Spec Rules: T-Sizeof
// =================================================================
#include "cursive0/03_analysis/types/expr/sizeof.h"

#include "cursive0/00_core/assert_spec.h"
#include "cursive0/03_analysis/types/type_expr.h"
#include "cursive0/03_analysis/types/type_lower.h"

namespace cursive0::analysis::expr {

namespace {

static inline void SpecDefsSizeof() {
  SPEC_DEF("T-Sizeof", "5.2.12");
}

}  // namespace

ExprTypeResult TypeSizeofExprImpl(const ScopeContext& ctx,
                                  const syntax::SizeofExpr& expr) {
  SpecDefsSizeof();
  SPEC_RULE("T-Sizeof");
  ExprTypeResult r;
  if (!expr.type) {
    return r;
  }
  const auto lowered = LowerType(ctx, expr.type);
  if (!lowered.ok) {
    r.diag_id = lowered.diag_id;
    return r;
  }
  const auto wf = TypeWF(ctx, lowered.type);
  if (!wf.ok) {
    r.diag_id = wf.diag_id;
    return r;
  }
  r.ok = true;
  r.type = MakeTypePrim("usize");
  return r;
}

}  // namespace cursive0::analysis::expr
