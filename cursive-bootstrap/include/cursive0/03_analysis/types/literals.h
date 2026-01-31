#pragma once

#include <optional>
#include <string_view>

#include "cursive0/03_analysis/memory/calls.h"
#include "cursive0/03_analysis/types/context.h"
#include "cursive0/03_analysis/types/types.h"
#include "cursive0/02_syntax/ast.h"

namespace cursive0::analysis {

struct LiteralCheckResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
};

ExprTypeResult TypeLiteralExpr(const ScopeContext& ctx,
                               const syntax::LiteralExpr& expr);

LiteralCheckResult CheckLiteralExpr(const ScopeContext& ctx,
                                    const syntax::LiteralExpr& expr,
                                    const TypeRef& expected);

bool NullLiteralExpected(const TypeRef& expected);

}  // namespace cursive0::analysis
