#pragma once

#include <optional>
#include <string_view>

#include "cursive0/analysis/memory/calls.h"
#include "cursive0/analysis/types/context.h"
#include "cursive0/analysis/types/place_types.h"
#include "cursive0/analysis/types/types.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::analysis {

ExprTypeResult TypeTupleExpr(const ScopeContext& ctx,
                             const syntax::TupleExpr& expr,
                             const ExprTypeFn& type_expr);

ExprTypeResult TypeTupleAccessValue(const ScopeContext& ctx,
                                    const syntax::TupleAccessExpr& expr,
                                    const ExprTypeFn& type_expr);

PlaceTypeResult TypeTupleAccessPlace(const ScopeContext& ctx,
                                     const syntax::TupleAccessExpr& expr,
                                     const PlaceTypeFn& type_place);

}  // namespace cursive0::analysis
