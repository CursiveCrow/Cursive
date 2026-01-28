#include "cursive0/analysis/resolve/scopes_lookup.h"

#include <algorithm>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/symbols.h"

namespace cursive0::analysis {

namespace {

static inline void SpecDefsLookup() {
  SPEC_DEF("ValueKind", "5.1.3");
  SPEC_DEF("TypeKind", "5.1.3");
  SPEC_DEF("ClassKind", "5.1.3");
  SPEC_DEF("ModuleKind", "5.1.3");
  SPEC_DEF("RegionAlias", "5.1.3");
  SPEC_DEF("RegionAliasName", "5.1.3");
  SPEC_DEF("AliasMap", "5.1.5");
  SPEC_DEF("ModuleNames", "5.1.5");
  SPEC_DEF("StringOfPath", "3.4.1");
}

bool ContainsName(const ModuleNames& names, std::string_view name) {
  return std::find(names.begin(), names.end(), name) != names.end();
}

syntax::ModulePath AliasExpand(const syntax::ModulePath& path,
                               const AliasMap& alias) {
  SpecDefsLookup();
  if (path.empty()) {
    return path;
  }
  const auto key = IdKeyOf(path.front());
  const auto it = alias.find(key);
  if (it == alias.end()) {
    SPEC_RULE("AliasExpand-None");
    return path;
  }
  SPEC_RULE("AliasExpand-Yes");
  syntax::ModulePath out = it->second;
  out.insert(out.end(), path.begin() + 1, path.end());
  return out;
}

AliasMap AliasMapForCurrentModule(const ScopeContext& ctx,
                                  const NameMapTable& name_maps) {
  const auto current_key = PathKeyOf(CurrentModule(ctx));
  const auto it = name_maps.find(current_key);
  if (it == name_maps.end()) {
    return {};
  }
  return AliasMapOf(it->second);
}

}  // namespace

AliasMap AliasMapOf(const NameMap& names) {
  SpecDefsLookup();
  AliasMap out;
  for (const auto& [key, ent] : names) {
    if (ent.kind != EntityKind::ModuleAlias) {
      continue;
    }
    if (!ent.origin_opt.has_value()) {
      continue;
    }
    out.emplace(key, *ent.origin_opt);
  }
  return out;
}

ModuleNames ModuleNamesOf(const project::Project& project) {
  SpecDefsLookup();
  ModuleNames out;
  out.reserve(project.modules.size());
  for (const auto& module : project.modules) {
    out.push_back(module.path);
  }
  return out;
}

bool ValueKind(const Entity& ent) {
  SpecDefsLookup();
  return ent.kind == EntityKind::Value;
}

bool TypeKind(const Entity& ent) {
  SpecDefsLookup();
  return ent.kind == EntityKind::Type;
}

bool ClassKind(const Entity& ent) {
  SpecDefsLookup();
  return ent.kind == EntityKind::Class;
}

bool ModuleKind(const Entity& ent) {
  SpecDefsLookup();
  return ent.kind == EntityKind::ModuleAlias;
}

bool RegionAlias(const Entity& ent) {
  SpecDefsLookup();
  return ent.source == EntitySource::RegionAlias;
}

std::optional<Entity> Lookup(const ScopeContext& ctx, std::string_view name) {
  SpecDefsLookup();
  const auto key = IdKeyOf(name);
  const auto& scopes = Scopes(ctx);
  for (const auto& scope : scopes) {
    const auto it = scope.find(key);
    if (it != scope.end()) {
      SPEC_RULE("Lookup-Unqualified");
      return it->second;
    }
  }
  SPEC_RULE("Lookup-Unqualified-None");
  return std::nullopt;
}

std::optional<Entity> ResolveValueName(const ScopeContext& ctx,
                                       std::string_view name) {
  SpecDefsLookup();
  // Map ~ (receiver reference) to self
  const std::string_view lookup_name = (name == "~") ? "self" : name;
  const auto ent = Lookup(ctx, lookup_name);
  if (!ent.has_value()) {
    return std::nullopt;
  }
  if (!ValueKind(*ent)) {
    return std::nullopt;
  }
  SPEC_RULE("Resolve-Value-Name");
  return ent;
}

std::optional<Entity> ResolveTypeName(const ScopeContext& ctx,
                                      std::string_view name) {
  SpecDefsLookup();
  const auto ent = Lookup(ctx, name);
  if (!ent.has_value()) {
    return std::nullopt;
  }
  if (!TypeKind(*ent)) {
    return std::nullopt;
  }
  SPEC_RULE("Resolve-Type-Name");
  return ent;
}

std::optional<Entity> ResolveClassName(const ScopeContext& ctx,
                                       std::string_view name) {
  SpecDefsLookup();
  const auto ent = Lookup(ctx, name);
  if (!ent.has_value()) {
    return std::nullopt;
  }
  if (!ClassKind(*ent)) {
    return std::nullopt;
  }
  SPEC_RULE("Resolve-Class-Name");
  return ent;
}

std::optional<Entity> ResolveModuleName(const ScopeContext& ctx,
                                        std::string_view name) {
  SpecDefsLookup();
  const auto ent = Lookup(ctx, name);
  if (!ent.has_value()) {
    return std::nullopt;
  }
  if (!ModuleKind(*ent)) {
    return std::nullopt;
  }
  SPEC_RULE("Resolve-Module-Name");
  return ent;
}

bool RegionAliasName(const ScopeContext& ctx, std::string_view name) {
  SpecDefsLookup();
  const auto ent = ResolveValueName(ctx, name);
  return ent.has_value() && RegionAlias(*ent);
}

ResolveModulePathResult ResolveModulePath(const ScopeContext& ctx,
                                          const syntax::ModulePath& path,
                                          const AliasMap& alias,
                                          const ModuleNames& module_names) {
  SpecDefsLookup();
  (void)ctx;
  const auto expanded = AliasExpand(path, alias);
  const auto path_name = core::StringOfPath(expanded);
  if (ContainsName(module_names, path_name)) {
    SPEC_RULE("Resolve-ModulePath");
    return {true, std::nullopt, expanded};
  }
  SPEC_RULE("ResolveModulePath-Err");
  return {false, "ResolveModulePath-Err", std::nullopt};
}

ResolveQualifiedResult ResolveQualified(const ScopeContext& ctx,
                                        const NameMapTable& name_maps,
                                        const ModuleNames& module_names,
                                        const syntax::ModulePath& path,
                                        std::string_view name,
                                        EntityKind kind,
                                        CanAccessFn can_access) {
  SpecDefsLookup();
  const AliasMap alias = AliasMapForCurrentModule(ctx, name_maps);
  const auto module_result = ResolveModulePath(ctx, path, alias, module_names);
  if (!module_result.ok) {
    return {false, module_result.diag_id, std::nullopt};
  }
  const auto& module_path = *module_result.path;
  const auto map_it = name_maps.find(PathKeyOf(module_path));
  if (map_it == name_maps.end()) {
    return {false, std::nullopt, std::nullopt};
  }
  const auto& names = map_it->second;
  const auto ent_it = names.find(IdKeyOf(name));
  if (ent_it == names.end()) {
    return {false, std::nullopt, std::nullopt};
  }
  if (ent_it->second.kind != kind) {
    return {false, std::nullopt, std::nullopt};
  }
  if (can_access) {
    const auto access = can_access(ctx, module_path, name);
    if (!access.ok) {
      return {false, access.diag_id, std::nullopt};
    }
  }
  SPEC_RULE("Resolve-Qualified");
  return {true, std::nullopt, ent_it->second};
}

}  // namespace cursive0::analysis
