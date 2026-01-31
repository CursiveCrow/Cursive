// =================================================================
// File: 03_analysis/types/expr/method_call.h
// Construct: Method Call Expression Type Checking
// Spec Section: 5.2.12
// Spec Rules: T-MethodCall, T-Record-MethodCall, T-Modal-Transition,
//             T-Modal-Method, T-Dynamic-MethodCall, T-Opaque-Project,
//             MethodCall-RecvPerm-Err, Step-MethodCall
// =================================================================
#pragma once

#include "cursive0/00_core/span.h"
#include "cursive0/03_analysis/types/context.h"
#include "cursive0/03_analysis/types/type_infer.h"
#include "cursive0/03_analysis/types/type_stmt.h"
#include "cursive0/02_syntax/ast.h"

namespace cursive0::analysis::expr {

// §5.2.12 Method Call Expression Typing
ExprTypeResult TypeMethodCallExprImpl(const ScopeContext& ctx,
                                      const StmtTypeContext& type_ctx,
                                      const syntax::MethodCallExpr& expr,
                                      const TypeEnv& env,
                                      const core::Span& span);

}  // namespace cursive0::analysis::expr
