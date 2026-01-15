#pragma once

#include "cursive0/sema/calls.h"
#include "cursive0/sema/context.h"
#include "cursive0/sema/type_infer.h"
#include "cursive0/sema/type_stmt.h"
#include "cursive0/sema/types.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::sema {

ExprTypeResult TypeMatchExpr(const ScopeContext& ctx,
                             const StmtTypeContext& type_ctx,
                             const syntax::MatchExpr& expr,
                             const TypeEnv& env);

CheckResult CheckMatchExpr(const ScopeContext& ctx,
                           const StmtTypeContext& type_ctx,
                           const syntax::MatchExpr& expr,
                           const TypeEnv& env,
                           const TypeRef& expected);

}  // namespace cursive0::sema
