#include <cassert>
#include <string>
#include <utility>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/sema/context.h"
#include "cursive0/sema/scopes.h"
#include "cursive0/sema/scopes_lookup.h"
#include "cursive0/sema/types.h"
#include "cursive0/semantics/state.h"
#include "cursive0/semantics/value.h"
#include "cursive0/syntax/ast.h"

namespace {

using cursive0::sema::Entity;
using cursive0::sema::EntityKind;
using cursive0::sema::EntitySource;
using cursive0::sema::ModuleNames;
using cursive0::sema::NameMap;
using cursive0::sema::NameMapTable;
using cursive0::sema::PathKey;
using cursive0::sema::Scope;
using cursive0::sema::ScopeContext;
using cursive0::semantics::LookupVal;
using cursive0::semantics::LookupValPath;
using cursive0::semantics::Sigma;
using cursive0::semantics::Value;

cursive0::syntax::RecordDecl MakeRecordDecl(std::string name) {
  cursive0::syntax::RecordDecl decl;
  decl.vis = cursive0::syntax::Visibility::Public;
  decl.name = std::move(name);
  decl.implements = {};
  decl.members = {};
  decl.span = {};
  decl.doc = {};
  return decl;
}

Value MakeIntValue(std::string type, std::uint64_t value) {
  cursive0::semantics::IntVal v;
  v.type = std::move(type);
  v.negative = false;
  v.magnitude = cursive0::core::UInt128FromU64(value);
  return Value{v};
}

void EnsureScopes(ScopeContext& ctx) {
  if (ctx.scopes.empty()) {
    ctx.scopes = {Scope{}, Scope{}, cursive0::sema::UniverseBindings()};
  }
}

void AddScopeEntity(ScopeContext& ctx,
                    std::string name,
                    EntityKind kind,
                    cursive0::syntax::ModulePath origin) {
  EnsureScopes(ctx);
  Entity ent{kind, std::move(origin), std::nullopt, EntitySource::Decl};
  ctx.scopes[0][cursive0::sema::IdKeyOf(name)] = ent;
}

void AddNameMap(NameMapTable& maps,
                const cursive0::syntax::ModulePath& module,
                std::string name,
                EntityKind kind) {
  NameMap& map = maps[cursive0::sema::PathKeyOf(module)];
  Entity ent{kind, module, std::nullopt, EntitySource::Decl};
  map.emplace(cursive0::sema::IdKeyOf(name), ent);
}

}  // namespace

int main() {
  SPEC_COV("LookupVal-Path");
  SPEC_COV("LookupVal-RecordCtor");
  SPEC_COV("LookupValPath-Proc");
  SPEC_COV("LookupValPath-RecordCtor");
  SPEC_COV("LookupValPath-Static");

  {
    ScopeContext sema_ctx;
    NameMapTable name_maps;
    ModuleNames module_names;
    module_names.push_back("M");

    const cursive0::syntax::ModulePath module = {"M"};
    AddScopeEntity(sema_ctx, "STATIC", EntityKind::Value, module);
    AddNameMap(name_maps, module, "STATIC", EntityKind::Value);

    Sigma sigma;
    const auto addr = cursive0::semantics::AllocAddr(sigma);
    sigma.static_addrs[{cursive0::sema::PathKeyOf(module), "STATIC"}] = addr;
    cursive0::semantics::WriteAddr(sigma, addr, MakeIntValue("i32", 7));

    cursive0::semantics::SemanticsContext ctx;
    ctx.sema = &sema_ctx;
    ctx.name_maps = &name_maps;
    ctx.module_names = &module_names;

    const auto val = LookupVal(ctx, sigma, "STATIC");
    assert(val.has_value());

    const auto val_path = LookupValPath(ctx, sigma, cursive0::sema::PathKeyOf(module), "STATIC");
    assert(val_path.has_value());
  }

  {
    ScopeContext sema_ctx;
    NameMapTable name_maps;
    ModuleNames module_names;
    module_names.push_back("M");

    const cursive0::syntax::ModulePath module = {"M"};
    AddNameMap(name_maps, module, "Rec", EntityKind::Type);

    auto record = MakeRecordDecl("Rec");
    const auto key = cursive0::sema::PathKeyOf(module);
    PathKey record_key = key;
    record_key.push_back("Rec");
    sema_ctx.sigma.types[record_key] = record;

    cursive0::semantics::SemanticsContext ctx;
    ctx.sema = &sema_ctx;
    ctx.name_maps = &name_maps;
    ctx.module_names = &module_names;

    Sigma sigma;
    const auto val_path = LookupValPath(ctx, sigma, key, "Rec");
    assert(val_path.has_value());
  }

  {
    ScopeContext sema_ctx;
    NameMapTable name_maps;
    ModuleNames module_names;
    const cursive0::syntax::ModulePath module = {"M"};
    AddScopeEntity(sema_ctx, "Rec", EntityKind::Type, module);

    auto record = MakeRecordDecl("Rec");
    PathKey record_key = cursive0::sema::PathKeyOf(module);
    record_key.push_back("Rec");
    sema_ctx.sigma.types[record_key] = record;

    Sigma sigma;
    cursive0::semantics::SemanticsContext ctx;
    ctx.sema = &sema_ctx;
    ctx.name_maps = &name_maps;
    ctx.module_names = &module_names;

    const auto val = LookupVal(ctx, sigma, "Rec");
    assert(val.has_value());
  }

  {
    ScopeContext sema_ctx;
    NameMapTable name_maps;
    ModuleNames module_names;

    cursive0::semantics::SemanticsContext ctx;
    ctx.sema = &sema_ctx;
    ctx.name_maps = &name_maps;
    ctx.module_names = &module_names;

    Sigma sigma;
    const auto val = LookupValPath(ctx, sigma, {"Region"}, "new_scoped");
    assert(val.has_value());
  }

  return 0;
}
