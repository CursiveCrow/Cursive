// =================================================================
// File: 03_analysis/types/expr/alignof.cpp
// Construct: Alignof Expression Type Checking
// Spec Section: 5.2.12
// Spec Rules: T-Alignof
// =================================================================
#include "cursive0/03_analysis/types/expr/alignof.h"

#include "cursive0/00_core/assert_spec.h"
#include "cursive0/03_analysis/types/type_expr.h"
#include "cursive0/03_analysis/types/type_lower.h"

namespace cursive0::analysis::expr {

namespace {

static inline void SpecDefsAlignof() {
  SPEC_DEF("T-Alignof", "5.2.12");
}

}  // namespace

ExprTypeResult TypeAlignofExprImpl(const ScopeContext& ctx,
                                   const syntax::AlignofExpr& expr) {
  SpecDefsAlignof();
  SPEC_RULE("T-Alignof");
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
