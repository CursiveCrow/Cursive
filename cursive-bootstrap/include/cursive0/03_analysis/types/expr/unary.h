// =================================================================
// File: 03_analysis/types/expr/unary.h
// Construct: Unary Expression Type Checking
// Spec Section: 5.2.12
// Spec Rules: T-Not-Bool, T-Not-Int, T-Neg, T-Modal-Widen
// =================================================================
#pragma once

#include "cursive0/00_core/span.h"
#include "cursive0/03_analysis/types/context.h"
#include "cursive0/03_analysis/types/type_infer.h"
#include "cursive0/03_analysis/types/type_stmt.h"
#include "cursive0/02_syntax/ast.h"

namespace cursive0::analysis::expr {

// §5.2.12 Unary Expression Typing
// Handles: ! (logical not, bitwise not), - (negation), widen
ExprTypeResult TypeUnaryExprImpl(const ScopeContext& ctx,
                                 const StmtTypeContext& type_ctx,
                                 const syntax::UnaryExpr& expr,
                                 const TypeEnv& env,
                                 const core::Span& span = core::Span{});

}  // namespace cursive0::analysis::expr
