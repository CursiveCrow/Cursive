#pragma once

#include <optional>
#include <string_view>

#include "cursive0/sema/calls.h"
#include "cursive0/sema/context.h"
#include "cursive0/sema/place_types.h"
#include "cursive0/sema/types.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::sema {

ExprTypeResult TypeTupleExpr(const ScopeContext& ctx,
                             const syntax::TupleExpr& expr,
                             const ExprTypeFn& type_expr);

ExprTypeResult TypeTupleAccessValue(const ScopeContext& ctx,
                                    const syntax::TupleAccessExpr& expr,
                                    const ExprTypeFn& type_expr);

PlaceTypeResult TypeTupleAccessPlace(const ScopeContext& ctx,
                                     const syntax::TupleAccessExpr& expr,
                                     const PlaceTypeFn& type_place);

}  // namespace cursive0::sema
