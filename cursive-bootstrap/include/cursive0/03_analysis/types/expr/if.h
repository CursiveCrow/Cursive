// =================================================================
// File: 03_analysis/types/expr/if.h
// Construct: If Expression Type Checking
// Spec Section: 5.2.12
// Spec Rules: T-If, T-If-No-Else, Chk-If, Chk-If-No-Else
// =================================================================
#pragma once

#include "cursive0/03_analysis/types/context.h"
#include "cursive0/03_analysis/types/type_infer.h"
#include "cursive0/03_analysis/types/type_stmt.h"
#include "cursive0/02_syntax/ast.h"

namespace cursive0::analysis::expr {

// §5.2.12 If Expression Typing
ExprTypeResult TypeIfExprImpl(const ScopeContext& ctx,
                              const StmtTypeContext& type_ctx,
                              const syntax::IfExpr& expr,
                              const TypeEnv& env);

// §5.2.12 If Expression Checking (against expected type)
CheckResult CheckIfExprImpl(const ScopeContext& ctx,
                            const StmtTypeContext& type_ctx,
                            const syntax::IfExpr& expr,
                            const TypeRef& expected,
                            const TypeEnv& env);

}  // namespace cursive0::analysis::expr
