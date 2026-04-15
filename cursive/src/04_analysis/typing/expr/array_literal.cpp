// =================================================================
// File: 04_analysis/typing/expr/array_literal.cpp
// Construct: Array Literal Expression Type Checking
// Spec Section: 5.2.6
// Spec Rules: T-Array-Literal-List
// =================================================================

#include "04_analysis/typing/expr/array_literal.h"

#include <cstdint>

#include "00_core/assert_spec.h"
#include "04_analysis/typing/type_equiv.h"
#include "04_analysis/typing/types.h"

namespace cursive::analysis::expr {

namespace {

static inline void SpecDefsArrayLiteral() {
  SPEC_DEF("T-Array-Literal-List", "5.2.6");
}

}  // namespace

// T-Array-Literal-List: Array from element list
// [e_1, e_2, ..., e_n] : [T; n] where all e_i : T
ExprTypeResult TypeArrayExprImpl(const ScopeContext& ctx,
                                 const ast::ArrayExpr& expr,
                                 const TypeExprFn& type_expr) {
  SpecDefsArrayLiteral();
  (void)ctx;
  ExprTypeResult result;

  // Empty array literal requires type annotation
  if (expr.elements.empty()) {
    return result;
  }

  // Type the first element to get the base type
  const auto first_type = type_expr(expr.elements.front());
  if (!first_type.ok) {
    result.diag_id = first_type.diag_id;
    return result;
  }

  // All remaining elements must have equivalent type
  for (std::size_t i = 1; i < expr.elements.size(); ++i) {
    const auto elem_type = type_expr(expr.elements[i]);
    if (!elem_type.ok) {
      result.diag_id = elem_type.diag_id;
      return result;
    }
    const auto equiv = TypeEquiv(first_type.type, elem_type.type);
    if (!equiv.ok) {
      result.diag_id = equiv.diag_id;
      return result;
    }
    if (!equiv.equiv) {
      return result;
    }
  }

  SPEC_RULE("T-Array-Literal-List");
  result.ok = true;
  result.type =
      MakeTypeArray(first_type.type,
                    static_cast<std::uint64_t>(expr.elements.size()));
  return result;
}

}  // namespace cursive::analysis::expr
