#pragma once

#include <optional>
#include <string_view>

#include "cursive0/sema/context.h"
#include "cursive0/sema/types.h"

namespace cursive0::sema {

struct SubtypingResult {
  bool ok = true;
  std::optional<std::string_view> diag_id;
  bool subtype = false;
};

SubtypingResult Subtyping(const ScopeContext& ctx,
                          const TypeRef& lhs,
                          const TypeRef& rhs);

}  // namespace cursive0::sema
