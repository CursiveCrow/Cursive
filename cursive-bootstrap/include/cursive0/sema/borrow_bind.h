#pragma once

#include <optional>
#include <string_view>
#include <vector>

#include "cursive0/core/span.h"
#include "cursive0/sema/context.h"
#include "cursive0/sema/types.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::sema {

struct BindSelfParam {
  TypeRef type;
  std::optional<ParamMode> mode;
};

struct BindCheckResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  std::optional<core::Span> span;
};

BindCheckResult BindCheckBody(const ScopeContext& ctx,
                              const syntax::ModulePath& module_path,
                              const std::vector<syntax::Param>& params,
                              const std::shared_ptr<syntax::Block>& body,
                              const std::optional<BindSelfParam>& self_param);

}  // namespace cursive0::sema
