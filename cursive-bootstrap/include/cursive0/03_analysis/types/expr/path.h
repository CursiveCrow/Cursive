// =================================================================
// File: 03_analysis/types/expr/path.h
// Construct: Path and Identifier Expression Type Checking
// Spec Section: 5.2.12
// Spec Rules: T-Ident, P-Ident, T-Path-Value, ValueUse-NonBitcopyPlace
// =================================================================
#pragma once

#include "cursive0/03_analysis/types/context.h"
#include "cursive0/03_analysis/types/place_types.h"
#include "cursive0/03_analysis/types/type_infer.h"
#include "cursive0/03_analysis/types/type_stmt.h"
#include "cursive0/02_syntax/ast.h"

namespace cursive0::analysis::expr {

// (T-Ident) Identifier Expression Value Typing
ExprTypeResult TypeIdentifierExprImpl(const ScopeContext& ctx,
                                      const syntax::IdentifierExpr& expr,
                                      const TypeEnv& env);

// (P-Ident) Identifier Expression Place Typing
PlaceTypeResult TypeIdentifierPlaceImpl(const ScopeContext& ctx,
                                        const syntax::IdentifierExpr& expr,
                                        const TypeEnv& env);

// (T-Path-Value) Path Expression Typing
ExprTypeResult TypePathExprImpl(const ScopeContext& ctx,
                                const syntax::PathExpr& expr);

}  // namespace cursive0::analysis::expr
