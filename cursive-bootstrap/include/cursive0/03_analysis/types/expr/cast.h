// =================================================================
// File: 03_analysis/types/expr/cast.h
// Construct: Cast Expression Type Checking
// Spec Section: 5.2.12
// Spec Rules: T-Cast, CastValid
// =================================================================
#pragma once

#include "cursive0/03_analysis/types/context.h"
#include "cursive0/03_analysis/types/type_infer.h"
#include "cursive0/03_analysis/types/type_stmt.h"
#include "cursive0/02_syntax/ast.h"

namespace cursive0::analysis::expr {

// §5.2.12 Cast Expression Typing
ExprTypeResult TypeCastExprImpl(const ScopeContext& ctx,
                                const StmtTypeContext& type_ctx,
                                const syntax::CastExpr& expr,
                                const TypeEnv& env);

}  // namespace cursive0::analysis::expr
