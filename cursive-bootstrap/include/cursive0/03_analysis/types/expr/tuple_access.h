// =================================================================
// File: 03_analysis/types/expr/tuple_access.h
// Construct: Tuple Access Expression Type Checking
// Spec Section: 5.2.12
// Spec Rules: T-Tuple-Access, P-Tuple-Access
// =================================================================
#pragma once

#include "cursive0/03_analysis/types/context.h"
#include "cursive0/03_analysis/types/type_infer.h"
#include "cursive0/03_analysis/types/type_stmt.h"
#include "cursive0/02_syntax/ast.h"

namespace cursive0::analysis::expr {

// §5.2.12 Tuple Access Expression Typing
ExprTypeResult TypeTupleAccessExprImpl(const ScopeContext& ctx,
                                       const StmtTypeContext& type_ctx,
                                       const syntax::TupleAccessExpr& expr,
                                       const TypeEnv& env);

PlaceTypeResult TypeTupleAccessPlaceImpl(const ScopeContext& ctx,
                                         const StmtTypeContext& type_ctx,
                                         const syntax::TupleAccessExpr& expr,
                                         const TypeEnv& env);

}  // namespace cursive0::analysis::expr
