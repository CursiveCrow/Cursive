// =================================================================
// File: 03_analysis/types/expr/array_repeat.h
// Construct: Array Repeat Expression Type Checking
// Spec Section: 5.2.12
// Spec Rules: T-Array-Repeat
// =================================================================
#pragma once

#include <functional>

#include "cursive0/03_analysis/types/context.h"
#include "cursive0/03_analysis/types/type_infer.h"
#include "cursive0/02_syntax/ast.h"

namespace cursive0::analysis::expr {

// Type function for recursive expression typing
using TypeExprFn = std::function<ExprTypeResult(const syntax::ExprPtr&)>;

// Type check an array repeat expression [value; count]
ExprTypeResult TypeArrayRepeatExprImpl(const ScopeContext& ctx,
                                       const syntax::ArrayRepeatExpr& expr,
                                       TypeExprFn type_expr);

}  // namespace cursive0::analysis::expr
