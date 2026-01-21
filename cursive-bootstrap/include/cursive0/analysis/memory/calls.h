#pragma once

#include <functional>
#include <optional>
#include <string_view>
#include <vector>

#include "cursive0/analysis/types/context.h"
#include "cursive0/analysis/types/place_types.h"
#include "cursive0/analysis/types/types.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::analysis {

struct ExprTypeResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  TypeRef type;
};

using ExprTypeFn = std::function<ExprTypeResult(const syntax::ExprPtr&)>;

struct CallTypeResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  TypeRef type;
  bool record_callee = false;
};

CallTypeResult TypeCall(const ScopeContext& ctx,
                        const syntax::ExprPtr& callee,
                        const std::vector<syntax::Arg>& args,
                        const ExprTypeFn& type_expr,
                        const PlaceTypeFn* type_place = nullptr);

bool IsRecordCallee(const ScopeContext& ctx,
                    const syntax::ExprPtr& callee,
                    const std::vector<syntax::Arg>& args);

}  // namespace cursive0::analysis
