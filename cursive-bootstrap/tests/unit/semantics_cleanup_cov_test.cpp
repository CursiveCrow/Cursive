#undef NDEBUG
#include <cassert>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/int128.h"
#include "cursive0/sema/context.h"
#include "cursive0/sema/scopes.h"
#include "cursive0/sema/types.h"
#include "cursive0/semantics/cleanup.h"
#include "cursive0/semantics/control.h"
#include "cursive0/semantics/state.h"
#include "cursive0/semantics/value.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::semantics {
void DestroyBindings(const SemanticsContext& ctx,
                     const std::vector<Binding>& bindings,
                     Sigma& sigma);
}

namespace {

using cursive0::core::UInt128FromU64;
using cursive0::semantics::BindInfo;
using cursive0::semantics::BindState;
using cursive0::semantics::BindVal;
using cursive0::semantics::Binding;
using cursive0::semantics::Cleanup;
using cursive0::semantics::CleanupItem;
using cursive0::semantics::CleanupScope;
using cursive0::semantics::CleanupStatus;
using cursive0::semantics::ControlKind;
using cursive0::semantics::DropActionOut;
using cursive0::semantics::DropStaticActionOut;
using cursive0::semantics::DropStatus;
using cursive0::semantics::DropSubvalue;
using cursive0::semantics::DropValueOut;
using cursive0::semantics::Movability;
using cursive0::semantics::Responsibility;
using cursive0::semantics::SetState;
using cursive0::semantics::Sigma;
using cursive0::semantics::SemanticsContext;
using cursive0::semantics::UnitVal;
using cursive0::semantics::Value;
using cursive0::syntax::Block;
using cursive0::syntax::ClassPath;
using cursive0::syntax::ErrorStmt;
using cursive0::syntax::Expr;
using cursive0::syntax::FieldDecl;
using cursive0::syntax::IdentifierExpr;
using cursive0::syntax::RecordDecl;
using cursive0::syntax::Type;
using cursive0::syntax::TypePath;
using cursive0::syntax::TypePathType;
using cursive0::syntax::TypePrim;
using cursive0::syntax::Visibility;

cursive0::core::Span EmptySpan() {
  return {};
}

static std::shared_ptr<Type> MakePrimType(std::string name) {
  auto type = std::make_shared<Type>();
  type->span = EmptySpan();
  type->node = TypePrim{std::move(name)};
  return type;
}

static std::shared_ptr<Type> MakePathType(TypePath path) {
  auto type = std::make_shared<Type>();
  type->span = EmptySpan();
  type->node = TypePathType{std::move(path)};
  return type;
}

static FieldDecl MakeField(std::string name, std::shared_ptr<Type> type) {
  FieldDecl field;
  field.vis = Visibility::Public;
  field.name = std::move(name);
  field.type = std::move(type);
  field.init_opt = nullptr;
  field.span = EmptySpan();
  field.doc_opt = std::nullopt;
  return field;
}

static RecordDecl MakeRecordDecl(std::string name,
                                 std::vector<FieldDecl> fields,
                                 std::vector<ClassPath> implements = {}) {
  RecordDecl decl;
  decl.vis = Visibility::Public;
  decl.name = std::move(name);
  decl.implements = std::move(implements);
  decl.members.reserve(fields.size());
  for (auto& field : fields) {
    decl.members.push_back(std::move(field));
  }
  decl.span = EmptySpan();
  decl.doc = {};
  return decl;
}

static std::shared_ptr<Block> MakeEmptyBlock() {
  auto block = std::make_shared<Block>();
  block->span = EmptySpan();
  return block;
}

static std::shared_ptr<Block> MakeErrorBlock() {
  auto block = std::make_shared<Block>();
  block->span = EmptySpan();
  block->stmts.push_back(ErrorStmt{});
  return block;
}

static Binding MakeBadBinding() {
  Binding binding;
  binding.scope_id = 999;
  binding.bind_id = 1;
  binding.name = "bad";
  binding.addr = 1;
  return binding;
}

static Value MakeInt(std::string type, std::uint64_t value) {
  cursive0::semantics::IntVal v;
  v.type = std::move(type);
  v.negative = false;
  v.magnitude = UInt128FromU64(value);
  return Value{v};
}

void TestCleanupListRules() {
  SPEC_COV("Cleanup-Cons-Defer-Ok");
  SPEC_COV("Cleanup-Cons-Defer-Panic");
  SPEC_COV("Cleanup-Cons-Drop");
  SPEC_COV("Cleanup-Cons-Drop-Panic");
  SPEC_COV("Cleanup-Cons-DropStatic");
  SPEC_COV("Cleanup-Cons-DropStatic-Panic");
  SPEC_COV("Cleanup-Empty");

  SemanticsContext ctx;

  {
    Sigma sigma;
    const auto status = Cleanup(ctx, {}, sigma);
    assert(status == CleanupStatus::Ok);
  }

  {
    Sigma sigma;
    cursive0::semantics::PushScope(sigma, nullptr);
    Binding binding;
    BindVal(sigma, "x", Value{UnitVal{}}, BindInfo{}, &binding);
    const auto status = Cleanup(ctx, {cursive0::semantics::DropBinding{binding}}, sigma);
    assert(status == CleanupStatus::Ok);
  }

  {
    Sigma sigma;
    Binding binding = MakeBadBinding();
    const auto status = Cleanup(ctx, {cursive0::semantics::DropBinding{binding}}, sigma);
    assert(status == CleanupStatus::Panic);
  }

  {
    Sigma sigma;
    const auto path = cursive0::sema::PathKeyOf(cursive0::syntax::ModulePath{"main"});
    const auto addr = cursive0::semantics::AllocAddr(sigma);
    sigma.static_addrs[{path, "x"}] = addr;
    sigma.store[addr] = Value{UnitVal{}};
    const auto status = Cleanup(
        ctx, {cursive0::semantics::DropStatic{path, "x"}}, sigma);
    assert(status == CleanupStatus::Ok);
  }

  {
    Sigma sigma;
    const auto path = cursive0::sema::PathKeyOf(cursive0::syntax::ModulePath{"main"});
    const auto status = Cleanup(
        ctx, {cursive0::semantics::DropStatic{path, "missing"}}, sigma);
    assert(status == CleanupStatus::Panic);
  }

  {
    Sigma sigma;
    auto block = MakeEmptyBlock();
    const auto status = Cleanup(
        ctx, {cursive0::semantics::DeferBlock{block}}, sigma);
    assert(status == CleanupStatus::Ok);
  }

  {
    Sigma sigma;
    auto block = MakeErrorBlock();
    const auto status = Cleanup(
        ctx, {cursive0::semantics::DeferBlock{block}}, sigma);
    assert(status == CleanupStatus::Panic);
  }
}

void TestCleanupScopeSteps() {
  SPEC_COV("Cleanup-Step-Defer-Abort");
  SPEC_COV("Cleanup-Step-Defer-Ok");
  SPEC_COV("Cleanup-Step-Defer-Panic");
  SPEC_COV("Cleanup-Step-Drop-Abort");
  SPEC_COV("Cleanup-Step-DropStatic-Abort");
  SPEC_COV("Cleanup-Step-DropStatic-Ok");
  SPEC_COV("Cleanup-Step-DropStatic-Panic");
  SPEC_COV("CleanupScope-From-SmallStep");

  SemanticsContext ctx;

  {
    Sigma sigma;
    cursive0::semantics::PushScope(sigma, nullptr);
    Binding binding;
    BindVal(sigma, "x", Value{UnitVal{}}, BindInfo{}, &binding);
    auto* scope = cursive0::semantics::CurrentScope(sigma);
    scope->cleanup = {cursive0::semantics::DropBinding{binding}};
    const auto status = CleanupScope(ctx, *scope, sigma);
    assert(status == CleanupStatus::Ok);
  }

  {
    Sigma sigma;
    cursive0::semantics::PushScope(sigma, nullptr);
    Binding bad = MakeBadBinding();
    auto* scope = cursive0::semantics::CurrentScope(sigma);
    scope->cleanup = {cursive0::semantics::DropBinding{bad}};
    const auto status = CleanupScope(ctx, *scope, sigma);
    assert(status == CleanupStatus::Panic);
  }

  {
    Sigma sigma;
    cursive0::semantics::PushScope(sigma, nullptr);
    Binding bad1 = MakeBadBinding();
    Binding bad2 = MakeBadBinding();
    auto* scope = cursive0::semantics::CurrentScope(sigma);
    scope->cleanup = {cursive0::semantics::DropBinding{bad2},
                      cursive0::semantics::DropBinding{bad1}};
    const auto status = CleanupScope(ctx, *scope, sigma);
    assert(status == CleanupStatus::Abort);
  }

  {
    Sigma sigma;
    const auto path = cursive0::sema::PathKeyOf(cursive0::syntax::ModulePath{"main"});
    const auto addr = cursive0::semantics::AllocAddr(sigma);
    sigma.static_addrs[{path, "x"}] = addr;
    sigma.store[addr] = Value{UnitVal{}};
    cursive0::semantics::PushScope(sigma, nullptr);
    auto* scope = cursive0::semantics::CurrentScope(sigma);
    scope->cleanup = {cursive0::semantics::DropStatic{path, "x"}};
    const auto status = CleanupScope(ctx, *scope, sigma);
    assert(status == CleanupStatus::Ok);
  }

  {
    Sigma sigma;
    const auto path = cursive0::sema::PathKeyOf(cursive0::syntax::ModulePath{"main"});
    cursive0::semantics::PushScope(sigma, nullptr);
    auto* scope = cursive0::semantics::CurrentScope(sigma);
    scope->cleanup = {cursive0::semantics::DropStatic{path, "missing"}};
    const auto status = CleanupScope(ctx, *scope, sigma);
    assert(status == CleanupStatus::Panic);
  }

  {
    Sigma sigma;
    const auto path = cursive0::sema::PathKeyOf(cursive0::syntax::ModulePath{"main"});
    cursive0::semantics::PushScope(sigma, nullptr);
    auto* scope = cursive0::semantics::CurrentScope(sigma);
    scope->cleanup = {cursive0::semantics::DropStatic{path, "missing"},
                      cursive0::semantics::DropStatic{path, "missing"}};
    const auto status = CleanupScope(ctx, *scope, sigma);
    assert(status == CleanupStatus::Abort);
  }

  {
    Sigma sigma;
    cursive0::semantics::PushScope(sigma, nullptr);
    auto* scope = cursive0::semantics::CurrentScope(sigma);
    scope->cleanup = {cursive0::semantics::DeferBlock{MakeEmptyBlock()}};
    const auto status = CleanupScope(ctx, *scope, sigma);
    assert(status == CleanupStatus::Ok);
  }

  {
    Sigma sigma;
    cursive0::semantics::PushScope(sigma, nullptr);
    auto* scope = cursive0::semantics::CurrentScope(sigma);
    scope->cleanup = {cursive0::semantics::DeferBlock{MakeErrorBlock()}};
    const auto status = CleanupScope(ctx, *scope, sigma);
    assert(status == CleanupStatus::Panic);
  }

  {
    Sigma sigma;
    cursive0::semantics::PushScope(sigma, nullptr);
    auto* scope = cursive0::semantics::CurrentScope(sigma);
    scope->cleanup = {cursive0::semantics::DeferBlock{MakeErrorBlock()},
                      cursive0::semantics::DeferBlock{MakeErrorBlock()}};
    const auto status = CleanupScope(ctx, *scope, sigma);
    // std::cerr << "DoubleDefer Panic Status=" << (int)status << " Expected=" << (int)CleanupStatus::Abort << "\n";
    assert(status == CleanupStatus::Abort);
  }
}

void TestDropActionAndDestroy() {
  SPEC_COV("Destroy-Cons");
  SPEC_COV("Destroy-Empty");
  SPEC_COV("DropAction-Moved");
  SPEC_COV("DropAction-Partial");
  SPEC_COV("DropStaticAction");

  SemanticsContext ctx;

  {
    Sigma sigma;
    cursive0::semantics::PushScope(sigma, nullptr);
    Binding binding;
    BindVal(sigma, "x", Value{UnitVal{}}, BindInfo{}, &binding);
    BindState moved;
    moved.kind = BindState::Kind::Moved;
    SetState(sigma, binding, moved);
    const auto status = DropActionOut(ctx, binding, sigma);
    assert(status == DropStatus::Ok);
  }

  {
    cursive0::sema::ScopeContext sema_ctx;
    RecordDecl rec = MakeRecordDecl(
        "Rec",
        {MakeField("x", MakePrimType("i32"))});
    sema_ctx.sigma.types[cursive0::sema::PathKeyOf({"Rec"})] = rec;

    SemanticsContext ctx_rec;
    ctx_rec.sema = &sema_ctx;

    Sigma sigma;
    cursive0::semantics::PushScope(sigma, nullptr);
    cursive0::semantics::RecordVal rec_val;
    rec_val.record_type = cursive0::sema::MakeTypePath({"Rec"});
    rec_val.fields = {{"x", Value{UnitVal{}}}};
    Binding binding;
    BindVal(sigma, "r", Value{rec_val}, BindInfo{}, &binding);

    BindState partial;
    partial.kind = BindState::Kind::PartiallyMoved;
    partial.fields.insert("x");
    SetState(sigma, binding, partial);

    const auto status = DropActionOut(ctx_rec, binding, sigma);
    (void)status;
  }

  {
    Sigma sigma;
    const auto path = cursive0::sema::PathKeyOf(cursive0::syntax::ModulePath{"main"});
    const auto addr = cursive0::semantics::AllocAddr(sigma);
    sigma.static_addrs[{path, "x"}] = addr;
    sigma.store[addr] = Value{UnitVal{}};
    const auto status = DropStaticActionOut(ctx, path, "x", sigma);
    assert(status == DropStatus::Ok);
  }

  {
    Sigma sigma;
    cursive0::semantics::PushScope(sigma, nullptr);
    Binding binding;
    BindVal(sigma, "x", Value{UnitVal{}}, BindInfo{}, &binding);
    cursive0::semantics::DestroyBindings(ctx, {binding}, sigma);
    cursive0::semantics::DestroyBindings(ctx, {}, sigma);
  }
}

void TestDropSubvalueAndDropValueOut() {
  SPEC_COV("DropSubvalue-Do");
  SPEC_COV("DropSubvalue-Skip");
  SPEC_COV("DropValueOut-ChildPanic");
  SPEC_COV("DropValueOut-DropPanic");
  SPEC_COV("DropValueOut-Ok");

  SemanticsContext ctx;

  {
    Sigma sigma;
    const auto status = DropValueOut(ctx,
                                     cursive0::sema::MakeTypePrim("()"),
                                     Value{UnitVal{}},
                                     {},
                                     sigma);
    assert(status == DropStatus::Ok);
  }

  {
    Sigma sigma;
    cursive0::semantics::PushScope(sigma, nullptr);
    Binding binding;
    BindVal(sigma, "x", Value{UnitVal{}}, BindInfo{}, &binding);
    Expr place;
    place.span = EmptySpan();
    place.node = IdentifierExpr{"x"};
    const auto status = DropSubvalue(ctx, place, nullptr, Value{UnitVal{}}, sigma);
    assert(status == DropStatus::Ok);
  }

  {
    Sigma sigma;
    BindInfo info;
    info.mov = Movability::Immov;
    info.resp = Responsibility::Resp;
    cursive0::semantics::PushScope(sigma, nullptr);
    Binding binding;
    BindVal(sigma, "y", Value{UnitVal{}}, info, &binding);
    Expr place;
    place.span = EmptySpan();
    place.node = IdentifierExpr{"y"};
    const auto status = DropSubvalue(ctx, place, nullptr, Value{UnitVal{}}, sigma);
    assert(status == DropStatus::Ok);
  }

  {
    cursive0::sema::ScopeContext sema_ctx;
    RecordDecl needs_drop = MakeRecordDecl(
        "NeedsDrop",
        {},
        {ClassPath{"Drop"}});
    sema_ctx.sigma.types[cursive0::sema::PathKeyOf({"NeedsDrop"})] = needs_drop;

    SemanticsContext ctx_drop;
    ctx_drop.sema = &sema_ctx;

    Sigma sigma;
    cursive0::semantics::RecordVal rec_val;
    rec_val.record_type = cursive0::sema::MakeTypePath({"NeedsDrop"});
    rec_val.fields = {};
    const auto status = DropValueOut(ctx_drop,
                                     cursive0::sema::MakeTypePath({"NeedsDrop"}),
                                     Value{rec_val},
                                     {},
                                     sigma);
    assert(status == DropStatus::Panic);
  }

  {
    cursive0::sema::ScopeContext sema_ctx;
    RecordDecl needs_drop = MakeRecordDecl(
        "NeedsDrop",
        {},
        {ClassPath{"Drop"}});
    RecordDecl parent = MakeRecordDecl(
        "Parent",
        {MakeField("child", MakePathType({"NeedsDrop"}))});
    sema_ctx.sigma.types[cursive0::sema::PathKeyOf({"NeedsDrop"})] = needs_drop;
    sema_ctx.sigma.types[cursive0::sema::PathKeyOf({"Parent"})] = parent;

    SemanticsContext ctx_drop;
    ctx_drop.sema = &sema_ctx;

    cursive0::semantics::RecordVal child_val;
    child_val.record_type = cursive0::sema::MakeTypePath({"NeedsDrop"});
    child_val.fields = {};

    cursive0::semantics::RecordVal parent_val;
    parent_val.record_type = cursive0::sema::MakeTypePath({"Parent"});
    parent_val.fields = {{"child", Value{child_val}}};

    Sigma sigma;
    const auto status = DropValueOut(ctx_drop,
                                     cursive0::sema::MakeTypePath({"Parent"}),
                                     Value{parent_val},
                                     {},
                                     sigma);
    assert(status == DropStatus::Panic);
  }
}

}  // namespace

int main() {
  TestCleanupListRules();
  TestCleanupScopeSteps();
  TestDropActionAndDestroy();
  TestDropSubvalueAndDropValueOut();
  return 0;
}
