// =================================================================
// File: 03_analysis/types/expr/range.h
// Construct: Range Expression Type Checking
// Spec Section: 5.2.12
// Spec Rules: Range-Full, Range-To, Range-ToInclusive, Range-From,
//             Range-Exclusive, Range-Inclusive
// =================================================================
#pragma once

#include "cursive0/03_analysis/types/context.h"
#include "cursive0/03_analysis/types/type_infer.h"
#include "cursive0/03_analysis/types/type_stmt.h"
#include "cursive0/02_syntax/ast.h"

namespace cursive0::analysis::expr {

// §5.2.12 Range Expression Typing
// Handles: .., ..n, ..=n, n.., n..m, n..=m
ExprTypeResult TypeRangeExprImpl(const ScopeContext& ctx,
                                 const StmtTypeContext& type_ctx,
                                 const syntax::RangeExpr& expr,
                                 const TypeEnv& env);

}  // namespace cursive0::analysis::expr
