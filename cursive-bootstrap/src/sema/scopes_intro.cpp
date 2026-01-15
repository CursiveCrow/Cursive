#include "cursive0/sema/scopes_intro.h"

#include <algorithm>
#include <vector>

#include "cursive0/core/assert_spec.h"

namespace cursive0::sema {

namespace {

static inline void SpecDefsIntro() {
  SPEC_DEF("dom", "5.1.2");
  SPEC_DEF("Scopes", "5.1.2");
  SPEC_DEF("InScope", "5.1.2");
  SPEC_DEF("InOuter", "5.1.2");
  SPEC_DEF("Names", "5.1.2");
}

bool ContainsKey(const std::vector<IdKey>& keys, const IdKey& key) {
  return std::find(keys.begin(), keys.end(), key) != keys.end();
}

bool IsUniverseProtectedKey(const IdKey& key) {
  return ContainsKey(PrimTypeKeys(), key) ||
         ContainsKey(SpecialTypeKeys(), key) ||
         ContainsKey(AsyncTypeKeys(), key);
}

bool IsModuleScopeCurrent(const ScopeList& scopes) {
  if (scopes.size() < 2) {
    return false;
  }
  return &scopes.front() == &ModuleScope(scopes);
}

}  // namespace

bool InScope(const Scope& scope, std::string_view name) {
  SpecDefsIntro();
  return scope.find(IdKeyOf(name)) != scope.end();
}

bool InOuter(const ScopeContext& ctx, std::string_view name) {
  SpecDefsIntro();
  const auto& scopes = Scopes(ctx);
  if (scopes.empty()) {
    return false;
  }
  for (std::size_t i = 1; i < scopes.size(); ++i) {
    if (InScope(scopes[i], name)) {
      return true;
    }
  }
  return false;
}

IntroResult Intro(ScopeContext& ctx, std::string_view name, const Entity& ent) {
  SpecDefsIntro();
  if (ReservedGen(name)) {
    SPEC_RULE("Intro-Reserved-Gen-Err");
    return {false, "Intro-Reserved-Gen-Err"};
  }
  if (ReservedCursive(name)) {
    SPEC_RULE("Intro-Reserved-Cursive-Err");
    return {false, "Intro-Reserved-Cursive-Err"};
  }

  auto& scopes = Scopes(ctx);
  if (scopes.empty()) {
    return {false, std::nullopt};
  }

  if (InScope(scopes.front(), name)) {
    SPEC_RULE("Intro-Dup");
    return {false, std::nullopt};
  }
  if (InOuter(ctx, name)) {
    SPEC_RULE("Intro-Shadow-Required");
    return {false, "Intro-Shadow-Required"};
  }

  const auto key = IdKeyOf(name);
  if (IsModuleScopeCurrent(scopes) && IsUniverseProtectedKey(key)) {
    return {false, std::nullopt};
  }

  SPEC_RULE("Intro-Ok");
  scopes.front().emplace(key, ent);
  return {true, std::nullopt};
}

IntroResult ShadowIntro(ScopeContext& ctx,
                        std::string_view name,
                        const Entity& ent) {
  SpecDefsIntro();
  if (ReservedGen(name)) {
    SPEC_RULE("Shadow-Reserved-Gen-Err");
    return {false, "Shadow-Reserved-Gen-Err"};
  }
  if (ReservedCursive(name)) {
    SPEC_RULE("Shadow-Reserved-Cursive-Err");
    return {false, "Shadow-Reserved-Cursive-Err"};
  }

  auto& scopes = Scopes(ctx);
  if (scopes.empty()) {
    return {false, std::nullopt};
  }

  if (InScope(scopes.front(), name)) {
    return {false, std::nullopt};
  }
  if (!InOuter(ctx, name)) {
    SPEC_RULE("Shadow-Unnecessary");
    return {false, "Shadow-Unnecessary"};
  }

  const auto key = IdKeyOf(name);
  if (IsModuleScopeCurrent(scopes) && IsUniverseProtectedKey(key)) {
    return {false, std::nullopt};
  }

  SPEC_RULE("Shadow-Ok");
  scopes.front().emplace(key, ent);
  return {true, std::nullopt};
}

ValidateModuleNamesResult ValidateModuleNames(const Scope& names) {
  SpecDefsIntro();
  std::vector<IdKey> keys;
  keys.reserve(names.size());
  for (const auto& [key, ent] : names) {
    (void)ent;
    keys.push_back(key);
  }
  std::sort(keys.begin(), keys.end());

  for (const auto& key : keys) {
    if (KeywordKey(key)) {
      SPEC_RULE("Validate-Module-Keyword-Err");
      return {false, "Validate-Module-Keyword-Err"};
    }
  }
  for (const auto& key : keys) {
    if (ContainsKey(PrimTypeKeys(), key)) {
      SPEC_RULE("Validate-Module-Prim-Shadow-Err");
      return {false, "Validate-Module-Prim-Shadow-Err"};
    }
  }
  for (const auto& key : keys) {
    if (ContainsKey(SpecialTypeKeys(), key)) {
      SPEC_RULE("Validate-Module-Special-Shadow-Err");
      return {false, "Validate-Module-Special-Shadow-Err"};
    }
  }
  for (const auto& key : keys) {
    if (ContainsKey(AsyncTypeKeys(), key)) {
      SPEC_RULE("Validate-Module-Async-Shadow-Err");
      return {false, "Validate-Module-Async-Shadow-Err"};
    }
  }

  SPEC_RULE("Validate-Module-Ok");
  return {true, std::nullopt};
}

}  // namespace cursive0::sema
