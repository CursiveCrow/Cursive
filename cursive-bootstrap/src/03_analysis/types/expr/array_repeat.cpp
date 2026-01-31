// =================================================================
// File: 03_analysis/types/expr/array_repeat.cpp
// Construct: Array Repeat Expression Type Checking
// Spec Section: 5.2.12
// Spec Rules: T-Array-Repeat
// =================================================================
#include "cursive0/03_analysis/types/expr/array_repeat.h"

#include "cursive0/00_core/assert_spec.h"
#include "cursive0/03_analysis/types/expr/common.h"
#include "cursive0/03_analysis/types/type_equiv.h"
#include "cursive0/03_analysis/types/type_expr.h"

namespace cursive0::analysis::expr {

namespace {

static inline void SpecDefsArrayRepeat() {
  SPEC_DEF("T-Array-Repeat", "5.2.12");
}

}  // namespace

ExprTypeResult TypeArrayRepeatExprImpl(const ScopeContext& ctx,
                                       const syntax::ArrayRepeatExpr& expr,
                                       TypeExprFn type_expr) {
  SpecDefsArrayRepeat();
  SPEC_RULE("T-Array-Repeat");
  ExprTypeResult r;
  if (!expr.value || !expr.count) {
    return r;
  }
  // Type check the value expression
  const auto value_type = type_expr(expr.value);
  if (!value_type.ok) {
    r.diag_id = value_type.diag_id;
    return r;
  }
  // Type check the count expression
  const auto count_type = type_expr(expr.count);
  if (!count_type.ok) {
    r.diag_id = count_type.diag_id;
    return r;
  }
  // Count must be usize or compatible integer type
  const auto count_prim = GetPrimName(count_type.type);
  if (!count_prim.has_value() ||
      (!IsIntType(*count_prim) && *count_prim != "usize")) {
    r.diag_id = "E-TYP-1816";
    return r;
  }
  // Element type must be Bitcopy for repeat initialization
  if (!BitcopyType(ctx, value_type.type)) {
    r.diag_id = "E-TYP-1817";
    return r;
  }
  // Evaluate count as compile-time constant
  const auto len = ConstLen(ctx, expr.count);
  if (!len.ok || !len.value.has_value()) {
    r.diag_id = len.diag_id.value_or("E-TYP-1816");
    return r;
  }
  r.ok = true;
  r.type = MakeTypeArray(value_type.type, *len.value);
  return r;
}

}  // namespace cursive0::analysis::expr
