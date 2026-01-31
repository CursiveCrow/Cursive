// =================================================================
// File: 03_analysis/types/expr/call.h
// Construct: Call Expression Type Checking
// Spec Section: 5.2.12, 13.1.2
// Spec Rules: T-Call, T-Generic-Call
// =================================================================
#pragma once

#include <optional>
#include <vector>

#include "cursive0/03_analysis/generics/monomorphize.h"
#include "cursive0/03_analysis/types/context.h"
#include "cursive0/03_analysis/types/type_infer.h"
#include "cursive0/03_analysis/types/type_stmt.h"
#include "cursive0/02_syntax/ast.h"

namespace cursive0::analysis::expr {

// Build substitution map for a generic procedure call (§13.1.2)
std::optional<TypeSubst> BuildGenericCallSubst(
    const ScopeContext& ctx,
    const syntax::ExprPtr& callee,
    const std::vector<std::shared_ptr<syntax::Type>>& generic_args);

// Type check a call expression
ExprTypeResult TypeCallExprImpl(const ScopeContext& ctx,
                                const StmtTypeContext& type_ctx,
                                const syntax::CallExpr& expr,
                                const TypeEnv& env);

}  // namespace cursive0::analysis::expr
