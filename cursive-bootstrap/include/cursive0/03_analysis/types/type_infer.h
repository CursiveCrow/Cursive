#pragma once

#include <functional>
#include <optional>
#include <string_view>

#include "cursive0/03_analysis/memory/calls.h"
#include "cursive0/03_analysis/types/context.h"
#include "cursive0/03_analysis/types/place_types.h"
#include "cursive0/03_analysis/types/types.h"
#include "cursive0/02_syntax/ast.h"

namespace cursive0::analysis {

struct CheckResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
};

using MatchCheckFn =
    std::function<CheckResult(const syntax::MatchExpr&, const TypeRef& expected)>;

using IdentTypeFn = std::function<ExprTypeResult(std::string_view name)>;

ExprTypeResult InferExpr(const ScopeContext& ctx,
                         const syntax::ExprPtr& expr,
                         const ExprTypeFn& type_expr,
                         const PlaceTypeFn& type_place,
                         const IdentTypeFn& type_ident);

ExprTypeResult InferExpr(const ScopeContext& ctx,
                         const syntax::ExprPtr& expr,
                         const ExprTypeFn& type_expr,
                         const PlaceTypeFn& type_place,
                         const IdentTypeFn& type_ident,
                         const MatchCheckFn& match_check);

CheckResult CheckExpr(const ScopeContext& ctx,
                      const syntax::ExprPtr& expr,
                      const TypeRef& expected,
                      const ExprTypeFn& type_expr,
                      const PlaceTypeFn& type_place,
                      const IdentTypeFn& type_ident);

CheckResult CheckExpr(const ScopeContext& ctx,
                      const syntax::ExprPtr& expr,
                      const TypeRef& expected,
                      const ExprTypeFn& type_expr,
                      const PlaceTypeFn& type_place,
                      const IdentTypeFn& type_ident,
                      const MatchCheckFn& match_check);

}  // namespace cursive0::analysis
