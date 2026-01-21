#pragma once

#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "cursive0/project/project.h"
#include "cursive0/sema/context.h"
#include "cursive0/sema/scopes.h"

namespace cursive0::sema {

using NameMap = Scope;
using NameMapTable = std::map<PathKey, NameMap>;
using AliasMap = std::map<IdKey, syntax::ModulePath>;
using ModuleNames = std::vector<std::string>;

struct AccessResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
};

using CanAccessFn = AccessResult (*)(const ScopeContext& ctx,
                                     const syntax::ModulePath& module_path,
                                     std::string_view name);

AliasMap AliasMapOf(const NameMap& names);
ModuleNames ModuleNamesOf(const project::Project& project);

bool ValueKind(const Entity& ent);
bool TypeKind(const Entity& ent);
bool ClassKind(const Entity& ent);
bool ModuleKind(const Entity& ent);
bool RegionAlias(const Entity& ent);

std::optional<Entity> Lookup(const ScopeContext& ctx, std::string_view name);
std::optional<Entity> ResolveValueName(const ScopeContext& ctx,
                                       std::string_view name);
std::optional<Entity> ResolveTypeName(const ScopeContext& ctx,
                                      std::string_view name);
std::optional<Entity> ResolveClassName(const ScopeContext& ctx,
                                       std::string_view name);
std::optional<Entity> ResolveModuleName(const ScopeContext& ctx,
                                        std::string_view name);

bool RegionAliasName(const ScopeContext& ctx, std::string_view name);

struct ResolveModulePathResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  std::optional<syntax::ModulePath> path;
};

ResolveModulePathResult ResolveModulePath(const ScopeContext& ctx,
                                          const syntax::ModulePath& path,
                                          const AliasMap& alias,
                                          const ModuleNames& module_names);

struct ResolveQualifiedResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  std::optional<Entity> entity;
};

ResolveQualifiedResult ResolveQualified(const ScopeContext& ctx,
                                        const NameMapTable& name_maps,
                                        const ModuleNames& module_names,
                                        const syntax::ModulePath& path,
                                        std::string_view name,
                                        EntityKind kind,
                                        CanAccessFn can_access);

}  // namespace cursive0::sema
