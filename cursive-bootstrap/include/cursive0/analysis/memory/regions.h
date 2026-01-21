#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "cursive0/core/span.h"
#include "cursive0/analysis/memory/borrow_bind.h"
#include "cursive0/analysis/types/context.h"
#include "cursive0/analysis/types/type_stmt.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::analysis {

struct ProvCheckResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  std::optional<core::Span> span;
};

TypeRef RegionActiveTypeRef();
bool RegionActiveType(const TypeRef& type);
std::optional<std::string> InnermostActiveRegion(const TypeEnv& env);
std::string FreshRegionName(const TypeEnv& env);

ProvCheckResult ProvBindCheck(const ScopeContext& ctx,
                              const syntax::ModulePath& module_path,
                              const std::vector<syntax::Param>& params,
                              const std::shared_ptr<syntax::Block>& body,
                              const std::optional<BindSelfParam>& self_param);

}  // namespace cursive0::analysis
