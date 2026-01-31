#pragma once

#include <optional>
#include <string_view>

#include "cursive0/03_analysis/types/context.h"
#include "cursive0/03_analysis/resolve/scopes.h"

namespace cursive0::analysis {

struct IntroResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
};

IntroResult Intro(ScopeContext& ctx, std::string_view name, const Entity& ent);
IntroResult ShadowIntro(ScopeContext& ctx,
                        std::string_view name,
                        const Entity& ent);

bool InScope(const Scope& scope, std::string_view name);
bool InOuter(const ScopeContext& ctx, std::string_view name);

struct ValidateModuleNamesResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
};

ValidateModuleNamesResult ValidateModuleNames(const Scope& names);

}  // namespace cursive0::analysis
