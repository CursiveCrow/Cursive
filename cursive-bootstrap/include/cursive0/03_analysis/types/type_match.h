#pragma once

#include "cursive0/03_analysis/memory/calls.h"
#include "cursive0/03_analysis/types/context.h"
#include "cursive0/03_analysis/types/type_infer.h"
#include "cursive0/03_analysis/types/type_stmt.h"
#include "cursive0/03_analysis/types/types.h"
#include "cursive0/02_syntax/ast.h"

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
