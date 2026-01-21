#include <cassert>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/analysis/types/context.h"
#include "cursive0/analysis/resolve/scopes.h"
#include "cursive0/analysis/resolve/scopes_lookup.h"
#include "cursive0/analysis/types/types.h"
#include "cursive0/eval/eval.h"
#include "cursive0/eval/state.h"
#include "cursive0/eval/value.h"
#include "cursive0/syntax/ast.h"
#include "cursive0/syntax/token.h"

namespace {

using cursive0::analysis::ExprTypeMap;
using cursive0::analysis::ModuleNames;
using cursive0::analysis::NameMapTable;
using cursive0::analysis::ScopeContext;
using cursive0::analysis::TypeRef;
using cursive0::eval::BindInfo;
using cursive0::eval::BindVal;
using cursive0::eval::Control;
using cursive0::eval::MakeCtrlOut;
using cursive0::eval::Movability;
using cursive0::eval::PushScope;
using cursive0::eval::Responsibility;
using cursive0::eval::SemanticsContext;
using cursive0::eval::Sigma;
using cursive0::eval::StmtOut;
using cursive0::eval::Value;

using cursive0::syntax::Expr;
using cursive0::syntax::ExprNode;
using cursive0::syntax::ExprPtr;
using cursive0::syntax::Token;
using cursive0::syntax::TokenKind;

Token MakeToken(TokenKind kind, std::string lexeme) {
  Token tok;
  tok.kind = kind;
  tok.lexeme = std::move(lexeme);
  return tok;
}

ExprPtr MakeExpr(ExprNode node) {
  auto expr = std::make_shared<Expr>();
  expr->node = std::move(node);
  return expr;
}

ExprPtr MakeLiteralExpr(TokenKind kind, std::string lexeme) {
  cursive0::syntax::LiteralExpr lit;
  lit.literal = MakeToken(kind, std::move(lexeme));
  return MakeExpr(std::move(lit));
}

ExprPtr MakeIntLiteral(std::string lexeme) {
  return MakeLiteralExpr(TokenKind::IntLiteral, std::move(lexeme));
}

ExprPtr MakeIdentifierExpr(std::string name) {
  return MakeExpr(cursive0::syntax::IdentifierExpr{std::move(name)});
}

ExprPtr MakeFieldAccessExpr(ExprPtr base, std::string name) {
  cursive0::syntax::FieldAccessExpr node;
  node.base = std::move(base);
  node.name = std::move(name);
  return MakeExpr(std::move(node));
}

ExprPtr MakeTupleAccessExpr(ExprPtr base, std::string index) {
  cursive0::syntax::TupleAccessExpr node;
  node.base = std::move(base);
  node.index = MakeToken(TokenKind::IntLiteral, std::move(index));
  return MakeExpr(std::move(node));
}

ExprPtr MakeIndexAccessExpr(const ExprPtr& base, const ExprPtr& index) {
  cursive0::syntax::IndexAccessExpr node;
  node.base = base;
  node.index = index;
  return MakeExpr(std::move(node));
}

ExprPtr MakeRangeExpr(cursive0::syntax::RangeKind kind, const ExprPtr& lo, const ExprPtr& hi) {
  cursive0::syntax::RangeExpr node;
  node.kind = kind;
  node.lhs = lo;
  node.rhs = hi;
  return MakeExpr(std::move(node));
}

ExprPtr MakeDerefExpr(ExprPtr value) {
  cursive0::syntax::DerefExpr node;
  node.value = std::move(value);
  return MakeExpr(std::move(node));
}

Value MakeIntValue(std::string type, std::uint64_t value) {
  cursive0::eval::IntVal v;
  v.type = std::move(type);
  v.negative = false;
  v.magnitude = cursive0::core::UInt128FromU64(value);
  return Value{v};
}

void SetExprType(ExprTypeMap& map, const ExprPtr& expr, TypeRef type) {
  map[expr.get()] = std::move(type);
}

void BindValue(Sigma& sigma, std::string name, const Value& value) {
  BindInfo info;
  info.mov = Movability::Mov;
  info.resp = Responsibility::Resp;
  BindVal(sigma, std::move(name), value, info, nullptr);
}

void EnsureScopes(ScopeContext& ctx) {
  if (ctx.scopes.empty()) {
    ctx.scopes = {cursive0::analysis::Scope{}, cursive0::analysis::Scope{},
                  cursive0::analysis::UniverseBindings()};
  }
}

void AddScopeEntity(ScopeContext& ctx,
                    std::string name,
                    cursive0::analysis::EntityKind kind,
                    cursive0::syntax::ModulePath origin) {
  EnsureScopes(ctx);
  cursive0::analysis::Entity ent{kind, std::move(origin), std::nullopt,
                             cursive0::analysis::EntitySource::Decl};
  ctx.scopes[0][cursive0::analysis::IdKeyOf(name)] = ent;
}

void AddNameMap(NameMapTable& maps,
                const cursive0::syntax::ModulePath& module,
                std::string name,
                cursive0::analysis::EntityKind kind) {
  auto& map = maps[cursive0::analysis::PathKeyOf(module)];
  cursive0::analysis::Entity ent{kind, module, std::nullopt,
                             cursive0::analysis::EntitySource::Decl};
  map.emplace(cursive0::analysis::IdKeyOf(name), ent);
}

SemanticsContext MakeContext(ScopeContext& sema_ctx,
                             NameMapTable& name_maps,
                             ModuleNames& module_names,
                             ExprTypeMap& expr_types) {
  sema_ctx.expr_types = &expr_types;
  EnsureScopes(sema_ctx);
  SemanticsContext ctx;
  ctx.sema = &sema_ctx;
  ctx.name_maps = &name_maps;
  ctx.module_names = &module_names;
  return ctx;
}

}  // namespace

int main() {
  SPEC_COV("WritePlace-Deref");
  SPEC_COV("WritePlace-Field");
  SPEC_COV("WritePlace-Ident");
  SPEC_COV("WritePlace-Ident-Path");
  SPEC_COV("WritePlace-Ident-Path-Poison");
  SPEC_COV("WritePlace-Index");
  SPEC_COV("WritePlace-Index-OOB");
  SPEC_COV("WritePlace-Index-Range");
  SPEC_COV("WritePlace-Index-Range-Len");
  SPEC_COV("WritePlace-Index-Range-OOB");
  SPEC_COV("WritePlace-Tuple");
  SPEC_COV("WritePtr-Expired");
  SPEC_COV("WritePtr-Null");
  SPEC_COV("WritePtr-Raw");
  SPEC_COV("WritePtr-Safe");
  SPEC_COV("WriteSub-Deref");
  SPEC_COV("WriteSub-Field");
  SPEC_COV("WriteSub-Ident");
  SPEC_COV("WriteSub-Ident-Path");
  SPEC_COV("WriteSub-Ident-Path-Poison");
  SPEC_COV("WriteSub-Index");
  SPEC_COV("WriteSub-Index-OOB");
  SPEC_COV("WriteSub-Index-Range");
  SPEC_COV("WriteSub-Index-Range-Len");
  SPEC_COV("WriteSub-Index-Range-OOB");
  SPEC_COV("WriteSub-Tuple");

  ScopeContext sema_ctx;
  NameMapTable name_maps;
  ModuleNames module_names;
  ExprTypeMap expr_types;

  module_names.push_back("M");
  const cursive0::syntax::ModulePath module = {"M"};
  AddScopeEntity(sema_ctx, "STATIC", cursive0::analysis::EntityKind::Value, module);
  AddNameMap(name_maps, module, "STATIC", cursive0::analysis::EntityKind::Value);

  SemanticsContext ctx = MakeContext(sema_ctx, name_maps, module_names, expr_types);

  Sigma sigma;
  PushScope(sigma, nullptr);

  const auto static_addr = cursive0::eval::AllocAddr(sigma);
  sigma.static_addrs[{cursive0::analysis::PathKeyOf(module), "STATIC"}] = static_addr;
  cursive0::eval::WriteAddr(sigma, static_addr, MakeIntValue("i32", 1));

  BindValue(sigma, "x", MakeIntValue("i32", 1));
  BindValue(sigma, "rec",
            Value{cursive0::eval::RecordVal{
                cursive0::analysis::MakeTypePath({"Rec"}),
                {{"a", MakeIntValue("i32", 5)}}}});
  BindValue(sigma, "tup",
            Value{cursive0::eval::TupleVal{
                {MakeIntValue("i32", 10), MakeIntValue("i32", 11)}}});
  BindValue(sigma, "arr",
            Value{cursive0::eval::ArrayVal{
                {MakeIntValue("i32", 2), MakeIntValue("i32", 3), MakeIntValue("i32", 4)}}});

  const auto pointee_addr = cursive0::eval::AllocAddr(sigma);
  cursive0::eval::WriteAddr(sigma, pointee_addr, MakeIntValue("i32", 9));
  cursive0::eval::PtrVal ptr;
  ptr.state = cursive0::analysis::PtrState::Valid;
  ptr.addr = pointee_addr;
  BindValue(sigma, "ptr", Value{ptr});

  auto idx_one = MakeIntLiteral("1");
  auto idx_big = MakeIntLiteral("9");
  auto idx_range_lo = MakeIntLiteral("0");
  auto idx_range_hi = MakeIntLiteral("2");
  auto idx_range_oob = MakeIntLiteral("9");
  SetExprType(expr_types, idx_one, cursive0::analysis::MakeTypePrim("usize"));
  SetExprType(expr_types, idx_big, cursive0::analysis::MakeTypePrim("usize"));
  SetExprType(expr_types, idx_range_lo, cursive0::analysis::MakeTypePrim("usize"));
  SetExprType(expr_types, idx_range_hi, cursive0::analysis::MakeTypePrim("usize"));
  SetExprType(expr_types, idx_range_oob, cursive0::analysis::MakeTypePrim("usize"));

  {
    auto out = cursive0::eval::WritePtrSigma(Value{ptr}, MakeIntValue("i32", 7), sigma);
    assert(!std::holds_alternative<Control>(out.node));
  }

  {
    cursive0::eval::PtrVal null_ptr;
    null_ptr.state = cursive0::analysis::PtrState::Null;
    auto out = cursive0::eval::WritePtrSigma(Value{null_ptr}, MakeIntValue("i32", 1), sigma);
    assert(std::holds_alternative<Control>(out.node));
  }

  {
    cursive0::eval::PtrVal expired_ptr;
    expired_ptr.state = cursive0::analysis::PtrState::Expired;
    expired_ptr.addr = 1;
    auto out = cursive0::eval::WritePtrSigma(Value{expired_ptr}, MakeIntValue("i32", 1), sigma);
    assert(std::holds_alternative<Control>(out.node));
  }

  {
    const auto raw_addr = cursive0::eval::AllocAddr(sigma);
    cursive0::eval::RawPtrVal raw;
    raw.qual = cursive0::analysis::RawPtrQual::Mut;
    raw.addr = raw_addr;
    auto out = cursive0::eval::WritePtrSigma(Value{raw}, MakeIntValue("i32", 5), sigma);
    assert(!std::holds_alternative<Control>(out.node));
  }

  {
    auto out = cursive0::eval::WritePlaceSigma(ctx, *MakeIdentifierExpr("x"), MakeIntValue("i32", 2), sigma);
    assert(!std::holds_alternative<Control>(out.node));
  }

  {
    auto out = cursive0::eval::WritePlaceSigma(ctx, *MakeIdentifierExpr("STATIC"), MakeIntValue("i32", 3), sigma);
    assert(!std::holds_alternative<Control>(out.node));
  }

  {
    const auto poison_addr = cursive0::eval::AllocAddr(sigma);
    sigma.poison_flags[cursive0::analysis::PathKeyOf(module)] = poison_addr;
    cursive0::eval::BoolVal poison;
    poison.value = true;
    cursive0::eval::WriteAddr(sigma, poison_addr, Value{poison});
    auto out = cursive0::eval::WritePlaceSigma(ctx, *MakeIdentifierExpr("STATIC"), MakeIntValue("i32", 4), sigma);
    assert(std::holds_alternative<Control>(out.node));
  }

  {
    auto place = MakeFieldAccessExpr(MakeIdentifierExpr("rec"), "a");
    auto out = cursive0::eval::WritePlaceSigma(ctx, *place, MakeIntValue("i32", 8), sigma);
    assert(!std::holds_alternative<Control>(out.node));
  }

  {
    auto place = MakeTupleAccessExpr(MakeIdentifierExpr("tup"), "1");
    auto out = cursive0::eval::WritePlaceSigma(ctx, *place, MakeIntValue("i32", 12), sigma);
    assert(!std::holds_alternative<Control>(out.node));
  }

  {
    auto place = MakeIndexAccessExpr(MakeIdentifierExpr("arr"), idx_one);
    auto out = cursive0::eval::WritePlaceSigma(ctx, *place, MakeIntValue("i32", 99), sigma);
    assert(!std::holds_alternative<Control>(out.node));
  }

  {
    auto place = MakeIndexAccessExpr(MakeIdentifierExpr("arr"), idx_big);
    auto out = cursive0::eval::WritePlaceSigma(ctx, *place, MakeIntValue("i32", 99), sigma);
    assert(std::holds_alternative<Control>(out.node));
  }

  {
    auto range = MakeRangeExpr(cursive0::syntax::RangeKind::Exclusive, idx_range_lo, idx_range_hi);
    auto place = MakeIndexAccessExpr(MakeIdentifierExpr("arr"), range);
    cursive0::eval::ArrayVal slice_val;
    slice_val.elements = {MakeIntValue("i32", 1), MakeIntValue("i32", 2)};
    auto out = cursive0::eval::WritePlaceSigma(ctx, *place, Value{slice_val}, sigma);
    assert(!std::holds_alternative<Control>(out.node));
  }

  {
    auto range = MakeRangeExpr(cursive0::syntax::RangeKind::Exclusive, idx_range_lo, idx_range_hi);
    auto place = MakeIndexAccessExpr(MakeIdentifierExpr("arr"), range);
    cursive0::eval::ArrayVal slice_val;
    slice_val.elements = {MakeIntValue("i32", 1)};
    auto out = cursive0::eval::WritePlaceSigma(ctx, *place, Value{slice_val}, sigma);
    assert(std::holds_alternative<Control>(out.node));
  }

  {
    auto range = MakeRangeExpr(cursive0::syntax::RangeKind::Exclusive, idx_range_lo, idx_range_oob);
    auto place = MakeIndexAccessExpr(MakeIdentifierExpr("arr"), range);
    cursive0::eval::ArrayVal slice_val;
    slice_val.elements = {MakeIntValue("i32", 1), MakeIntValue("i32", 2)};
    auto out = cursive0::eval::WritePlaceSigma(ctx, *place, Value{slice_val}, sigma);
    assert(std::holds_alternative<Control>(out.node));
  }

  {
    auto place = MakeDerefExpr(MakeIdentifierExpr("ptr"));
    auto out = cursive0::eval::WritePlaceSigma(ctx, *place, MakeIntValue("i32", 55), sigma);
    assert(!std::holds_alternative<Control>(out.node));
  }

  {
    Sigma sigma_sub;
    PushScope(sigma_sub, nullptr);

    const auto static_addr_sub = cursive0::eval::AllocAddr(sigma_sub);
    sigma_sub.static_addrs[{cursive0::analysis::PathKeyOf(module), "STATIC"}] = static_addr_sub;
    cursive0::eval::WriteAddr(sigma_sub, static_addr_sub, MakeIntValue("i32", 1));

    BindValue(sigma_sub, "x", MakeIntValue("i32", 1));
    BindValue(sigma_sub, "rec",
              Value{cursive0::eval::RecordVal{
                  cursive0::analysis::MakeTypePath({"Rec"}),
                  {{"a", MakeIntValue("i32", 5)}}}});
    BindValue(sigma_sub, "tup",
              Value{cursive0::eval::TupleVal{
                  {MakeIntValue("i32", 10), MakeIntValue("i32", 11)}}});
    BindValue(sigma_sub, "arr",
              Value{cursive0::eval::ArrayVal{
                  {MakeIntValue("i32", 2), MakeIntValue("i32", 3), MakeIntValue("i32", 4)}}});

    const auto sub_pointee = cursive0::eval::AllocAddr(sigma_sub);
    cursive0::eval::WriteAddr(sigma_sub, sub_pointee, MakeIntValue("i32", 9));
    cursive0::eval::PtrVal sub_ptr;
    sub_ptr.state = cursive0::analysis::PtrState::Valid;
    sub_ptr.addr = sub_pointee;
    BindValue(sigma_sub, "ptr", Value{sub_ptr});

    {
      auto out = cursive0::eval::WritePlaceSubSigma(ctx, *MakeIdentifierExpr("x"), MakeIntValue("i32", 6), sigma_sub);
      assert(!std::holds_alternative<Control>(out.node));
    }

    {
      auto out = cursive0::eval::WritePlaceSubSigma(ctx, *MakeIdentifierExpr("STATIC"), MakeIntValue("i32", 7), sigma_sub);
      assert(!std::holds_alternative<Control>(out.node));
    }

    {
      const auto poison_addr = cursive0::eval::AllocAddr(sigma_sub);
      sigma_sub.poison_flags[cursive0::analysis::PathKeyOf(module)] = poison_addr;
      cursive0::eval::BoolVal poison;
      poison.value = true;
      cursive0::eval::WriteAddr(sigma_sub, poison_addr, Value{poison});
      auto out = cursive0::eval::WritePlaceSubSigma(ctx, *MakeIdentifierExpr("STATIC"), MakeIntValue("i32", 8), sigma_sub);
      assert(std::holds_alternative<Control>(out.node));
    }

    {
      auto place = MakeFieldAccessExpr(MakeIdentifierExpr("rec"), "a");
      auto out = cursive0::eval::WritePlaceSubSigma(ctx, *place, MakeIntValue("i32", 13), sigma_sub);
      assert(!std::holds_alternative<Control>(out.node));
    }

    {
      auto place = MakeTupleAccessExpr(MakeIdentifierExpr("tup"), "0");
      auto out = cursive0::eval::WritePlaceSubSigma(ctx, *place, MakeIntValue("i32", 14), sigma_sub);
      assert(!std::holds_alternative<Control>(out.node));
    }

    {
      auto place = MakeIndexAccessExpr(MakeIdentifierExpr("arr"), idx_one);
      auto out = cursive0::eval::WritePlaceSubSigma(ctx, *place, MakeIntValue("i32", 15), sigma_sub);
      assert(!std::holds_alternative<Control>(out.node));
    }

    {
      auto place = MakeIndexAccessExpr(MakeIdentifierExpr("arr"), idx_big);
      auto out = cursive0::eval::WritePlaceSubSigma(ctx, *place, MakeIntValue("i32", 16), sigma_sub);
      assert(std::holds_alternative<Control>(out.node));
    }

    {
      auto range = MakeRangeExpr(cursive0::syntax::RangeKind::Exclusive, idx_range_lo, idx_range_hi);
      auto place = MakeIndexAccessExpr(MakeIdentifierExpr("arr"), range);
      cursive0::eval::ArrayVal slice_val;
      slice_val.elements = {MakeIntValue("i32", 1), MakeIntValue("i32", 2)};
      auto out = cursive0::eval::WritePlaceSubSigma(ctx, *place, Value{slice_val}, sigma_sub);
      assert(!std::holds_alternative<Control>(out.node));
    }

    {
      auto range = MakeRangeExpr(cursive0::syntax::RangeKind::Exclusive, idx_range_lo, idx_range_hi);
      auto place = MakeIndexAccessExpr(MakeIdentifierExpr("arr"), range);
      cursive0::eval::ArrayVal slice_val;
      slice_val.elements = {MakeIntValue("i32", 1)};
      auto out = cursive0::eval::WritePlaceSubSigma(ctx, *place, Value{slice_val}, sigma_sub);
      assert(std::holds_alternative<Control>(out.node));
    }

    {
      auto range = MakeRangeExpr(cursive0::syntax::RangeKind::Exclusive, idx_range_lo, idx_range_oob);
      auto place = MakeIndexAccessExpr(MakeIdentifierExpr("arr"), range);
      cursive0::eval::ArrayVal slice_val;
      slice_val.elements = {MakeIntValue("i32", 1), MakeIntValue("i32", 2)};
      auto out = cursive0::eval::WritePlaceSubSigma(ctx, *place, Value{slice_val}, sigma_sub);
      assert(std::holds_alternative<Control>(out.node));
    }

    {
      auto place = MakeDerefExpr(MakeIdentifierExpr("ptr"));
      auto out = cursive0::eval::WritePlaceSubSigma(ctx, *place, MakeIntValue("i32", 77), sigma_sub);
      assert(!std::holds_alternative<Control>(out.node));
    }
  }
  return 0;
}
