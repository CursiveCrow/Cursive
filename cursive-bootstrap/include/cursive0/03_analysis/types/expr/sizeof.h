// =================================================================
// File: 03_analysis/types/expr/sizeof.h
// Construct: Sizeof Expression Type Checking
// Spec Section: 5.2.12
// Spec Rules: T-Sizeof
// =================================================================
#pragma once

#include "cursive0/03_analysis/types/context.h"
#include "cursive0/03_analysis/types/type_infer.h"
#include "cursive0/02_syntax/ast.h"

namespace cursive0::analysis::expr {

// Type check a sizeof expression
ExprTypeResult TypeSizeofExprImpl(const ScopeContext& ctx,
                                  const syntax::SizeofExpr& expr);

}  // namespace cursive0::analysis::expr
