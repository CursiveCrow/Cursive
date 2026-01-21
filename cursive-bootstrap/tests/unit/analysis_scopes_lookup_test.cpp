#include <cassert>

#include "cursive0/analysis/resolve/scopes_lookup.h"

namespace {

using cursive0::analysis::AccessResult;
using cursive0::analysis::AliasMap;
using cursive0::analysis::Entity;
using cursive0::analysis::EntityKind;
using cursive0::analysis::EntitySource;
using cursive0::analysis::IdKeyOf;
using cursive0::analysis::Lookup;
using cursive0::analysis::ModuleNames;
using cursive0::analysis::NameMap;
using cursive0::analysis::NameMapTable;
using cursive0::analysis::PathKeyOf;
using cursive0::analysis::ResolveClassName;
using cursive0::analysis::ResolveModuleName;
using cursive0::analysis::ResolveModulePath;
using cursive0::analysis::ResolveQualified;
using cursive0::analysis::ResolveTypeName;
using cursive0::analysis::ResolveValueName;
using cursive0::analysis::Scope;
using cursive0::analysis::ScopeContext;
using cursive0::syntax::ModulePath;

Entity MakeEntity(EntityKind kind,
                  EntitySource source = EntitySource::Decl,
                  std::optional<ModulePath> origin = std::nullopt,
                  std::optional<cursive0::syntax::Identifier> target =
                      std::nullopt) {
  return Entity{kind, std::move(origin), std::move(target), source};
}

ScopeContext MakeContext() {
  ScopeContext ctx;
  ctx.scopes = {Scope{}, Scope{}, Scope{}};
  return ctx;
}

AccessResult CanAccessOk(const ScopeContext&, const ModulePath&,
                         std::string_view) {
  return {true, std::nullopt};
}

}  // namespace

int main() {
  {
    SPEC_COV("Lookup-Unqualified");
    auto ctx = MakeContext();
    ctx.scopes.front().emplace(IdKeyOf("x"), MakeEntity(EntityKind::Value));
    const auto ent = Lookup(ctx, "x");
    assert(ent.has_value());
    assert(ent->kind == EntityKind::Value);
  }

  {
    SPEC_COV("Lookup-Unqualified-None");
    auto ctx = MakeContext();
    const auto ent = Lookup(ctx, "x");
    assert(!ent.has_value());
  }

  {
    SPEC_COV("Resolve-Value-Name");
    auto ctx = MakeContext();
    ctx.scopes.front().emplace(IdKeyOf("v"), MakeEntity(EntityKind::Value));
    const auto ent = ResolveValueName(ctx, "v");
    assert(ent.has_value());
    assert(ent->kind == EntityKind::Value);
  }

  {
    SPEC_COV("Resolve-Type-Name");
    auto ctx = MakeContext();
    ctx.scopes.front().emplace(IdKeyOf("T"), MakeEntity(EntityKind::Type));
    const auto ent = ResolveTypeName(ctx, "T");
    assert(ent.has_value());
    assert(ent->kind == EntityKind::Type);
  }

  {
    SPEC_COV("Resolve-Class-Name");
    auto ctx = MakeContext();
    ctx.scopes.front().emplace(IdKeyOf("C"), MakeEntity(EntityKind::Class));
    const auto ent = ResolveClassName(ctx, "C");
    assert(ent.has_value());
    assert(ent->kind == EntityKind::Class);
  }

  {
    SPEC_COV("Resolve-Module-Name");
    auto ctx = MakeContext();
    ctx.scopes.front().emplace(
        IdKeyOf("m"),
        MakeEntity(EntityKind::ModuleAlias, EntitySource::Decl,
                   ModulePath{"m"}));
    const auto ent = ResolveModuleName(ctx, "m");
    assert(ent.has_value());
    assert(ent->kind == EntityKind::ModuleAlias);
  }

  {
    auto ctx = MakeContext();
    ctx.scopes.front().emplace(IdKeyOf("x"), MakeEntity(EntityKind::Type));
    ctx.scopes[1].emplace(IdKeyOf("x"), MakeEntity(EntityKind::Value));
    const auto ent = ResolveValueName(ctx, "x");
    assert(!ent.has_value());
  }

  {
    SPEC_COV("AliasExpand-None");
    SPEC_COV("Resolve-ModulePath");
    auto ctx = MakeContext();
    ModuleNames module_names = {"core::io"};
    AliasMap alias;
    const auto result =
        ResolveModulePath(ctx, ModulePath{"core", "io"}, alias, module_names);
    assert(result.ok);
    assert(result.path.has_value());
    assert(*result.path == ModulePath({"core", "io"}));
  }

  {
    SPEC_COV("AliasExpand-Yes");
    SPEC_COV("Resolve-ModulePath");
    auto ctx = MakeContext();
    ModuleNames module_names = {"my::io"};
    AliasMap alias;
    alias.emplace(IdKeyOf("core"), ModulePath{"my"});
    const auto result =
        ResolveModulePath(ctx, ModulePath{"core", "io"}, alias, module_names);
    assert(result.ok);
    assert(result.path.has_value());
    assert(*result.path == ModulePath({"my", "io"}));
  }

  {
    SPEC_COV("ResolveModulePath-Err");
    auto ctx = MakeContext();
    ModuleNames module_names = {"core::io"};
    AliasMap alias;
    const auto result = ResolveModulePath(ctx, ModulePath{"other"}, alias,
                                          module_names);
    assert(!result.ok);
    assert(result.diag_id == "ResolveModulePath-Err");
  }

  {
    SPEC_COV("Resolve-Qualified");
    auto ctx = MakeContext();
    ctx.current_module = ModulePath{"main"};

    NameMapTable maps;
    maps.emplace(PathKeyOf(ctx.current_module), NameMap{});

    const ModulePath target = {"pkg", "mod"};
    NameMap names;
    names.emplace(IdKeyOf("Thing"),
                  MakeEntity(EntityKind::Type, EntitySource::Decl, target));
    maps.emplace(PathKeyOf(target), names);

    ModuleNames module_names = {"pkg::mod"};
    const auto result = ResolveQualified(ctx, maps, module_names, target,
                                         "Thing", EntityKind::Type,
                                         CanAccessOk);
    assert(result.ok);
    assert(result.entity.has_value());
    assert(result.entity->kind == EntityKind::Type);
  }

  return 0;
}
