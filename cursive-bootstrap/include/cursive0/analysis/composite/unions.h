#pragma once

#include <optional>
#include <string_view>

#include "cursive0/analysis/memory/calls.h"
#include "cursive0/analysis/types/context.h"
#include "cursive0/analysis/types/types.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::analysis {

struct UnionAccessResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
};

ExprTypeResult TypeUnionIntro(const ScopeContext& ctx,
                              const TypeRef& value_type,
                              const TypeRef& union_type);

UnionAccessResult CheckUnionDirectAccess(const ScopeContext& ctx,
                                         const syntax::FieldAccessExpr& expr,
                                         const ExprTypeFn& type_expr);

}  // namespace cursive0::analysis
