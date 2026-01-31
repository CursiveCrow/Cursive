// =================================================================
// File: 03_analysis/types/expr/transmute.h
// Construct: Transmute Expression Type Checking
// Spec Section: 5.2.12
// Spec Rules: T-Transmute, T-Transmute-SizeEq, T-Transmute-AlignEq,
//             Transmute-Unsafe-Err
// =================================================================
#pragma once

#include "cursive0/00_core/span.h"
#include "cursive0/03_analysis/types/context.h"
#include "cursive0/03_analysis/types/type_infer.h"
#include "cursive0/03_analysis/types/type_stmt.h"
#include "cursive0/02_syntax/ast.h"

namespace cursive0::analysis::expr {

// §5.2.12 Transmute Expression Typing
ExprTypeResult TypeTransmuteExprImpl(const ScopeContext& ctx,
                                     const StmtTypeContext& type_ctx,
                                     const syntax::TransmuteExpr& expr,
                                     const TypeEnv& env,
                                     const core::Span& span);

}  // namespace cursive0::analysis::expr
