#pragma once

#include <functional>
#include <optional>
#include <string_view>

#include "cursive0/sema/calls.h"
#include "cursive0/sema/context.h"
#include "cursive0/sema/place_types.h"
#include "cursive0/sema/types.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::sema {

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

}  // namespace cursive0::sema
