#include <cassert>

#include "cursive0/sema/scopes_intro.h"

namespace {

using cursive0::sema::Entity;
using cursive0::sema::EntityKind;
using cursive0::sema::EntitySource;
using cursive0::sema::IdKeyOf;
using cursive0::sema::Intro;
using cursive0::sema::Scope;
using cursive0::sema::ScopeContext;
using cursive0::sema::ShadowIntro;
using cursive0::sema::ValidateModuleNames;

Entity MakeValueEntity() {
  return Entity{EntityKind::Value, std::nullopt, std::nullopt,
                EntitySource::Decl};
}

ScopeContext MakeContext() {
  ScopeContext ctx;
  ctx.scopes = {Scope{}, Scope{}, Scope{}};
  return ctx;
}

}  // namespace

int main() {
  const auto ent = MakeValueEntity();

  {
    SPEC_COV("Intro-Ok");
    auto ctx = MakeContext();
    const auto result = Intro(ctx, "x", ent);
    assert(result.ok);
    assert(!result.diag_id.has_value());
    assert(ctx.scopes.front().count(IdKeyOf("x")) == 1);
  }

  {
    SPEC_COV("Intro-Dup");
    auto ctx = MakeContext();
    ctx.scopes.front().emplace(IdKeyOf("x"), ent);
    const auto result = Intro(ctx, "x", ent);
    assert(!result.ok);
    assert(!result.diag_id.has_value());
  }

  {
    SPEC_COV("Intro-Shadow-Required");
    auto ctx = MakeContext();
    ctx.scopes[1].emplace(IdKeyOf("x"), ent);
    const auto result = Intro(ctx, "x", ent);
    assert(!result.ok);
    assert(result.diag_id == "Intro-Shadow-Required");
  }

  {
    SPEC_COV("Intro-Reserved-Gen-Err");
    auto ctx = MakeContext();
    ctx.scopes.front().emplace(IdKeyOf("gen_tmp"), ent);
    const auto result = Intro(ctx, "gen_tmp", ent);
    assert(!result.ok);
    assert(result.diag_id == "Intro-Reserved-Gen-Err");
  }

  {
    SPEC_COV("Intro-Reserved-Cursive-Err");
    auto ctx = MakeContext();
    const auto result = Intro(ctx, "cursive", ent);
    assert(!result.ok);
    assert(result.diag_id == "Intro-Reserved-Cursive-Err");
  }

  {
    SPEC_COV("Shadow-Ok");
    auto ctx = MakeContext();
    ctx.scopes[1].emplace(IdKeyOf("x"), ent);
    const auto result = ShadowIntro(ctx, "x", ent);
    assert(result.ok);
    assert(!result.diag_id.has_value());
    assert(ctx.scopes.front().count(IdKeyOf("x")) == 1);
  }

  {
    SPEC_COV("Shadow-Unnecessary");
    auto ctx = MakeContext();
    const auto result = ShadowIntro(ctx, "x", ent);
    assert(!result.ok);
    assert(result.diag_id == "Shadow-Unnecessary");
  }

  {
    SPEC_COV("Shadow-Reserved-Gen-Err");
    auto ctx = MakeContext();
    const auto result = ShadowIntro(ctx, "gen_tmp", ent);
    assert(!result.ok);
    assert(result.diag_id == "Shadow-Reserved-Gen-Err");
  }

  {
    SPEC_COV("Shadow-Reserved-Cursive-Err");
    auto ctx = MakeContext();
    const auto result = ShadowIntro(ctx, "cursive", ent);
    assert(!result.ok);
    assert(result.diag_id == "Shadow-Reserved-Cursive-Err");
  }

  {
    auto ctx = MakeContext();
    ctx.scopes.front().emplace(IdKeyOf("x"), ent);
    const auto result = ShadowIntro(ctx, "x", ent);
    assert(!result.ok);
    assert(!result.diag_id.has_value());
  }

  {
    SPEC_COV("Validate-Module-Ok");
    Scope names;
    names.emplace(IdKeyOf("ok_name"), ent);
    const auto result = ValidateModuleNames(names);
    assert(result.ok);
    assert(!result.diag_id.has_value());
  }

  {
    SPEC_COV("Validate-Module-Keyword-Err");
    Scope names;
    names.emplace(IdKeyOf("let"), ent);
    const auto result = ValidateModuleNames(names);
    assert(!result.ok);
    assert(result.diag_id == "Validate-Module-Keyword-Err");
  }

  {
    SPEC_COV("Validate-Module-Prim-Shadow-Err");
    Scope names;
    names.emplace(IdKeyOf("i32"), ent);
    const auto result = ValidateModuleNames(names);
    assert(!result.ok);
    assert(result.diag_id == "Validate-Module-Prim-Shadow-Err");
  }

  {
    SPEC_COV("Validate-Module-Special-Shadow-Err");
    Scope names;
    names.emplace(IdKeyOf("Self"), ent);
    const auto result = ValidateModuleNames(names);
    assert(!result.ok);
    assert(result.diag_id == "Validate-Module-Special-Shadow-Err");
  }

  {
    SPEC_COV("Validate-Module-Async-Shadow-Err");
    Scope names;
    names.emplace(IdKeyOf("Async"), ent);
    const auto result = ValidateModuleNames(names);
    assert(!result.ok);
    assert(result.diag_id == "Validate-Module-Async-Shadow-Err");
  }

  {
    Scope names;
    names.emplace(IdKeyOf("let"), ent);
    names.emplace(IdKeyOf("i32"), ent);
    const auto result = ValidateModuleNames(names);
    assert(!result.ok);
    assert(result.diag_id == "Validate-Module-Keyword-Err");
  }

  return 0;
}
