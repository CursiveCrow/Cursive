#pragma once

#include <optional>
#include <string_view>

#include "cursive0/sema/calls.h"
#include "cursive0/sema/context.h"
#include "cursive0/sema/types.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::sema {

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

}  // namespace cursive0::sema
