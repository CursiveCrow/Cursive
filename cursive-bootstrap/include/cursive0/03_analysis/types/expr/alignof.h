// =================================================================
// File: 03_analysis/types/expr/alignof.h
// Construct: Alignof Expression Type Checking
// Spec Section: 5.2.12
// Spec Rules: T-Alignof
// =================================================================
#pragma once

#include "cursive0/03_analysis/types/context.h"
#include "cursive0/03_analysis/types/type_infer.h"
#include "cursive0/02_syntax/ast.h"

namespace cursive0::analysis::expr {

// Type check an alignof expression
ExprTypeResult TypeAlignofExprImpl(const ScopeContext& ctx,
                                   const syntax::AlignofExpr& expr);

}  // namespace cursive0::analysis::expr
