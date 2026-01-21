#pragma once

#include <functional>
#include <optional>
#include <string_view>

#include "cursive0/sema/types.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::sema {

struct PlaceTypeResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  TypeRef type;
};

using PlaceTypeFn = std::function<PlaceTypeResult(const syntax::ExprPtr&)>;

}  // namespace cursive0::sema
