#pragma once

#include <optional>
#include <string_view>

#include "cursive0/analysis/memory/calls.h"
#include "cursive0/analysis/types/context.h"
#include "cursive0/analysis/types/place_types.h"
#include "cursive0/analysis/types/types.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::analysis {

ExprTypeResult TypeArrayExpr(const ScopeContext& ctx,
                             const syntax::ArrayExpr& expr,
                             const ExprTypeFn& type_expr);

ExprTypeResult TypeIndexAccessValue(const ScopeContext& ctx,
                                    const syntax::IndexAccessExpr& expr,
                                    const ExprTypeFn& type_expr,
                                    bool dynamic_context);

PlaceTypeResult TypeIndexAccessPlace(const ScopeContext& ctx,
                                     const syntax::IndexAccessExpr& expr,
                                     const PlaceTypeFn& type_place,
                                     const ExprTypeFn& type_expr,
                                     bool dynamic_context);

ExprTypeResult CoerceArrayToSlice(const ScopeContext& ctx,
                                  const TypeRef& type);

}  // namespace cursive0::analysis
