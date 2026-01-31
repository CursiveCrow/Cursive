#pragma once

#include <functional>
#include <optional>
#include <string_view>

#include "cursive0/03_analysis/types/types.h"
#include "cursive0/02_syntax/ast.h"

namespace cursive0::analysis {

struct PlaceTypeResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  TypeRef type;
};

using PlaceTypeFn = std::function<PlaceTypeResult(const syntax::ExprPtr&)>;

}  // namespace cursive0::analysis
