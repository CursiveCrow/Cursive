#pragma once

#include <optional>
#include <string_view>

#include "cursive0/sema/calls.h"
#include "cursive0/sema/context.h"
#include "cursive0/sema/place_types.h"
#include "cursive0/sema/types.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::sema {

ExprTypeResult TypeArrayExpr(const ScopeContext& ctx,
                             const syntax::ArrayExpr& expr,
                             const ExprTypeFn& type_expr);

ExprTypeResult TypeIndexAccessValue(const ScopeContext& ctx,
                                    const syntax::IndexAccessExpr& expr,
                                    const ExprTypeFn& type_expr);

PlaceTypeResult TypeIndexAccessPlace(const ScopeContext& ctx,
                                     const syntax::IndexAccessExpr& expr,
                                     const PlaceTypeFn& type_place,
                                     const ExprTypeFn& type_expr);

ExprTypeResult CoerceArrayToSlice(const ScopeContext& ctx,
                                  const TypeRef& type);

}  // namespace cursive0::sema
