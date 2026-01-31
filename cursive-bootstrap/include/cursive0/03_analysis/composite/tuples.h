#pragma once

#include <optional>
#include <string_view>

#include "cursive0/03_analysis/memory/calls.h"
#include "cursive0/03_analysis/types/context.h"
#include "cursive0/03_analysis/types/place_types.h"
#include "cursive0/03_analysis/types/types.h"
#include "cursive0/02_syntax/ast.h"

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
