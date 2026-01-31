#pragma once

#include <optional>
#include <string_view>
#include <vector>

#include "cursive0/00_core/span.h"
#include "cursive0/03_analysis/types/context.h"
#include "cursive0/03_analysis/types/types.h"
#include "cursive0/02_syntax/ast.h"

namespace cursive0::analysis {

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

}  // namespace cursive0::analysis
