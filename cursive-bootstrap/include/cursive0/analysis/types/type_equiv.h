#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

#include "cursive0/analysis/types/context.h"
#include "cursive0/analysis/types/types.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::analysis {

struct ConstLenResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  std::optional<std::uint64_t> value;
};

ConstLenResult ConstLen(const ScopeContext& ctx, const syntax::ExprPtr& expr);

struct TypeEquivResult {
  bool ok = true;
  std::optional<std::string_view> diag_id;
  bool equiv = false;
};

TypeEquivResult TypeEquiv(const TypeRef& lhs, const TypeRef& rhs);

}  // namespace cursive0::analysis
