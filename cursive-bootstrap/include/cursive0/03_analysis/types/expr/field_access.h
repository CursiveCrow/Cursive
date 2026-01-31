// =================================================================
// File: 03_analysis/types/expr/field_access.h
// Construct: Field Access Expression Type Checking
// Spec Section: 5.2.12
// Spec Rules: T-Field-Record, T-Field-Record-Perm, P-Field-Record,
//             P-Field-Record-Perm, FieldAccess-Unknown, FieldAccess-NotVisible
// =================================================================
#pragma once

#include "cursive0/03_analysis/types/context.h"
#include "cursive0/03_analysis/types/type_infer.h"
#include "cursive0/03_analysis/types/type_stmt.h"
#include "cursive0/02_syntax/ast.h"

namespace cursive0::analysis::expr {

// §5.2.12 Field Access Expression Typing
ExprTypeResult TypeFieldAccessExprImpl(const ScopeContext& ctx,
                                       const StmtTypeContext& type_ctx,
                                       const syntax::FieldAccessExpr& expr,
                                       const TypeEnv& env);

// §5.2.12 Field Access Place Typing
PlaceTypeResult TypeFieldAccessPlaceImpl(const ScopeContext& ctx,
                                         const StmtTypeContext& type_ctx,
                                         const syntax::FieldAccessExpr& expr,
                                         const TypeEnv& env);

}  // namespace cursive0::analysis::expr
