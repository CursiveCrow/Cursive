// =================================================================
// File: 03_analysis/types/expr/alloc.h
// Construct: Allocation Expression Type Checking
// Spec Section: 5.2.12
// Spec Rules: T-Alloc-Explicit, T-Alloc-Implicit, Alloc-Region-NotFound-Err,
//             Alloc-Implicit-NoRegion-Err
// =================================================================
#pragma once

#include "cursive0/03_analysis/types/context.h"
#include "cursive0/03_analysis/types/type_infer.h"
#include "cursive0/03_analysis/types/type_stmt.h"
#include "cursive0/02_syntax/ast.h"

namespace cursive0::analysis::expr {

// §5.2.12 Allocation Expression Typing
ExprTypeResult TypeAllocExprImpl(const ScopeContext& ctx,
                                 const StmtTypeContext& type_ctx,
                                 const syntax::AllocExpr& expr,
                                 const TypeEnv& env);

}  // namespace cursive0::analysis::expr
