#pragma once

#include "cursive0/analysis/memory/calls.h"
#include "cursive0/analysis/types/context.h"
#include "cursive0/analysis/types/type_infer.h"
#include "cursive0/analysis/types/type_stmt.h"
#include "cursive0/analysis/types/types.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::analysis {

ExprTypeResult TypeMatchExpr(const ScopeContext& ctx,
                             const StmtTypeContext& type_ctx,
                             const syntax::MatchExpr& expr,
                             const TypeEnv& env);

CheckResult CheckMatchExpr(const ScopeContext& ctx,
                           const StmtTypeContext& type_ctx,
                           const syntax::MatchExpr& expr,
                           const TypeEnv& env,
                           const TypeRef& expected);

}  // namespace cursive0::analysis
