
#include <cassert>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/int128.h"
#include "cursive0/core/symbols.h"
#include "cursive0/analysis/types/context.h"
#include "cursive0/analysis/resolve/scopes.h"
#include "cursive0/analysis/types/types.h"
#include "cursive0/eval/eval.h"
#include "cursive0/eval/state.h"
#include "cursive0/eval/value.h"
#include "cursive0/syntax/ast.h"
#include "cursive0/syntax/token.h"

namespace {

using cursive0::eval::BindInfo;
using cursive0::eval::BindVal;
using cursive0::eval::BreakVal;
using cursive0::eval::EvalFieldInitsSigma;
using cursive0::eval::EvalListSigma;
using cursive0::eval::EvalOptSigma;
using cursive0::eval::EvalSigma;
using cursive0::eval::IsCtrl;
using cursive0::eval::IsVal;
using cursive0::eval::MakeCtrl;
using cursive0::eval::MakeVal;
using cursive0::eval::Movability;
using cursive0::eval::Outcome;
using cursive0::eval::PushScope;
using cursive0::eval::Responsibility;
using cursive0::eval::SemanticsContext;
using cursive0::eval::Sigma;
using cursive0::eval::Value;
using cursive0::eval::ValueEqual;

using cursive0::syntax::Expr;
using cursive0::syntax::ExprPtr;
using cursive0::syntax::ExprNode;

using FieldValues = std::vector<std::pair<std::string, Value>>;

static cursive0::syntax::Token MakeToken(cursive0::syntax::TokenKind kind,
                                         std::string lexeme) {
  cursive0::syntax::Token tok;
  tok.kind = kind;
  tok.lexeme = std::move(lexeme);
  return tok;
}

static ExprPtr MakeExpr(ExprNode node) {
  auto expr = std::make_shared<Expr>();
  expr->node = std::move(node);
  return expr;
}

static ExprPtr MakeLiteralExpr(cursive0::syntax::TokenKind kind,
                               std::string lexeme) {
  cursive0::syntax::LiteralExpr lit;
  lit.literal = MakeToken(kind, std::move(lexeme));
  return MakeExpr(std::move(lit));
}

static ExprPtr MakeIdentifierExpr(std::string name) {
  return MakeExpr(cursive0::syntax::IdentifierExpr{std::move(name)});
}

static ExprPtr MakePathExpr(cursive0::syntax::ModulePath path,
                            std::string name) {
  cursive0::syntax::PathExpr node;
  node.path = std::move(path);
  node.name = std::move(name);
  return MakeExpr(std::move(node));
}

static ExprPtr MakeErrorExpr() {
  return MakeExpr(cursive0::syntax::ErrorExpr{});
}

static ExprPtr MakeRangeExpr(cursive0::syntax::RangeKind kind,
                             ExprPtr lo,
                             ExprPtr hi) {
  cursive0::syntax::RangeExpr node;
  node.kind = kind;
  node.lhs = std::move(lo);
  node.rhs = std::move(hi);
  return MakeExpr(std::move(node));
}

static ExprPtr MakeTupleExpr(std::vector<ExprPtr> elements) {
  cursive0::syntax::TupleExpr node;
  node.elements = std::move(elements);
  return MakeExpr(std::move(node));
}

static ExprPtr MakeArrayExpr(std::vector<ExprPtr> elements) {
  cursive0::syntax::ArrayExpr node;
  node.elements = std::move(elements);
  return MakeExpr(std::move(node));
}

static cursive0::syntax::FieldInit MakeFieldInit(std::string name,
                                                 ExprPtr value) {
  cursive0::syntax::FieldInit field;
  field.name = std::move(name);
  field.value = std::move(value);
  return field;
}

static std::shared_ptr<cursive0::syntax::Type> MakePrimType(std::string name) {
  auto type = std::make_shared<cursive0::syntax::Type>();
  type->node = cursive0::syntax::TypePrim{std::move(name)};
  return type;
}

static ExprPtr MakeRecordExpr(cursive0::syntax::TypePath path,
                              std::vector<cursive0::syntax::FieldInit> fields) {
  cursive0::syntax::RecordExpr node;
  node.target = std::move(path);
  node.fields = std::move(fields);
  return MakeExpr(std::move(node));
}

static ExprPtr MakeEnumUnitExpr(cursive0::syntax::Path path) {
  cursive0::syntax::EnumLiteralExpr node;
  node.path = std::move(path);
  node.payload_opt = std::nullopt;
  return MakeExpr(std::move(node));
}

static ExprPtr MakeEnumTupleExpr(cursive0::syntax::Path path,
                                 std::vector<ExprPtr> elements) {
  cursive0::syntax::EnumPayloadParen payload;
  payload.elements = std::move(elements);
  cursive0::syntax::EnumLiteralExpr node;
  node.path = std::move(path);
  node.payload_opt = cursive0::syntax::EnumPayload{std::move(payload)};
  return MakeExpr(std::move(node));
}

static ExprPtr MakeEnumRecordExpr(cursive0::syntax::Path path,
                                  std::vector<cursive0::syntax::FieldInit> fields) {
  cursive0::syntax::EnumPayloadBrace payload;
  payload.fields = std::move(fields);
  cursive0::syntax::EnumLiteralExpr node;
  node.path = std::move(path);
  node.payload_opt = cursive0::syntax::EnumPayload{std::move(payload)};
  return MakeExpr(std::move(node));
}

static ExprPtr MakeFieldAccessExpr(ExprPtr base, std::string name) {
  cursive0::syntax::FieldAccessExpr node;
  node.base = std::move(base);
  node.name = std::move(name);
  return MakeExpr(std::move(node));
}

static ExprPtr MakeTupleAccessExpr(ExprPtr base, std::string index) {
  cursive0::syntax::TupleAccessExpr node;
  node.base = std::move(base);
  node.index = MakeToken(cursive0::syntax::TokenKind::IntLiteral,
                         std::move(index));
  return MakeExpr(std::move(node));
}

static ExprPtr MakeIndexAccessExpr(ExprPtr base, ExprPtr index) {
  cursive0::syntax::IndexAccessExpr node;
  node.base = std::move(base);
  node.index = std::move(index);
  return MakeExpr(std::move(node));
}

static void AddName(cursive0::analysis::NameMapTable& maps,
                    const cursive0::syntax::ModulePath& module,
                    std::string_view name,
                    cursive0::analysis::EntityKind kind,
                    std::optional<cursive0::syntax::ModulePath> origin = std::nullopt) {
  auto& map = maps[cursive0::analysis::PathKeyOf(module)];
  map.emplace(
      cursive0::analysis::IdKeyOf(name),
      cursive0::analysis::Entity{kind, std::move(origin), std::nullopt,
                             cursive0::analysis::EntitySource::Decl});
}

static cursive0::syntax::RecordDecl MakeRecordDecl(std::string_view name) {
  cursive0::syntax::RecordDecl decl;
  decl.vis = cursive0::syntax::Visibility::Public;
  decl.name = std::string(name);
  decl.implements = {};
  decl.members = {};
  decl.span = {};
  decl.doc = {};
  return decl;
}

static Value MakeInt(std::string type, std::uint64_t value) {
  cursive0::eval::IntVal v;
  v.type = std::move(type);
  v.negative = false;
  v.magnitude = cursive0::core::UInt128FromU64(value);
  return Value{v};
}

static Value MakeBool(bool value) {
  return Value{cursive0::eval::BoolVal{value}};
}

static void SetExprType(cursive0::analysis::ExprTypeMap& map,
                        const ExprPtr& expr,
                        cursive0::analysis::TypeRef type) {
  map[expr.get()] = std::move(type);
}

}  // namespace
int main() {
  SPEC_COV("EvalOptSigma-None");
  SPEC_COV("EvalOptSigma-Some");
  SPEC_COV("EvalOptSigma-Ctrl");
  SPEC_COV("EvalListSigma-Empty");
  SPEC_COV("EvalListSigma-Cons");
  SPEC_COV("EvalListSigma-Ctrl");
  SPEC_COV("EvalFieldInitsSigma-Empty");
  SPEC_COV("EvalFieldInitsSigma-Cons");
  SPEC_COV("EvalFieldInitsSigma-Ctrl");
  SPEC_COV("EvalSigma-Range");
  SPEC_COV("EvalSigma-Range-Ctrl");
  SPEC_COV("EvalSigma-Range-Ctrl-Hi");
  SPEC_COV("EvalSigma-Literal");
  SPEC_COV("EvalSigma-PtrNull");
  SPEC_COV("EvalSigma-Ident");
  SPEC_COV("EvalSigma-Ident-Poison");
  SPEC_COV("EvalSigma-Ident-Poison-RecordCtor");
  SPEC_COV("EvalSigma-Path");
  SPEC_COV("EvalSigma-Path-Poison");
  SPEC_COV("EvalSigma-Path-Poison-RecordCtor");
  SPEC_COV("EvalSigma-ErrorExpr");
  SPEC_COV("EvalSigma-Tuple");
  SPEC_COV("EvalSigma-Tuple-Ctrl");
  SPEC_COV("EvalSigma-Array");
  SPEC_COV("EvalSigma-Array-Ctrl");
  SPEC_COV("EvalSigma-Record");
  SPEC_COV("EvalSigma-Record-Ctrl");
  SPEC_COV("EvalSigma-Enum-Unit");
  SPEC_COV("EvalSigma-Enum-Tuple");
  SPEC_COV("EvalSigma-Enum-Tuple-Ctrl");
  SPEC_COV("EvalSigma-Enum-Record");
  SPEC_COV("EvalSigma-Enum-Record-Ctrl");
  SPEC_COV("EvalSigma-FieldAccess");
  SPEC_COV("EvalSigma-FieldAccess-Ctrl");
  SPEC_COV("EvalSigma-TupleAccess");
  SPEC_COV("EvalSigma-TupleAccess-Ctrl");
  SPEC_COV("EvalSigma-Index");
  SPEC_COV("EvalSigma-Index-OOB");
  SPEC_COV("EvalSigma-Index-Range");
  SPEC_COV("EvalSigma-Index-Range-OOB");
  SPEC_COV("EvalSigma-Index-Ctrl-Base");
  SPEC_COV("EvalSigma-Index-Ctrl-Idx");
  SPEC_COV("EvalSigma-Unary");
  SPEC_COV("EvalSigma-Unary-Panic");
  SPEC_COV("EvalSigma-Unary-Ctrl");
  SPEC_COV("EvalSigma-Bin-And-False");
  SPEC_COV("EvalSigma-Bin-And-True");
  SPEC_COV("EvalSigma-Bin-Or-True");
  SPEC_COV("EvalSigma-Bin-Or-False");
  SPEC_COV("EvalSigma-Binary");
  SPEC_COV("EvalSigma-Binary-Panic");
  SPEC_COV("EvalSigma-Bin-Ctrl-L");
  SPEC_COV("EvalSigma-Bin-Ctrl-R");
  SPEC_COV("EvalSigma-Cast");
  SPEC_COV("EvalSigma-Cast-Panic");
  SPEC_COV("EvalSigma-Cast-Ctrl");
  SPEC_COV("EvalSigma-Transmute");
  SPEC_COV("EvalSigma-Transmute-Ctrl");
  SPEC_COV("EvalSigma-Propagate-Success");
  SPEC_COV("EvalSigma-Propagate-Error");
  SPEC_COV("EvalSigma-Propagate-Ctrl");
  SPEC_COV("EvalSigma-AddressOf");
  SPEC_COV("EvalSigma-AddressOf-Ctrl");
  SPEC_COV("EvalSigma-Deref");
  SPEC_COV("EvalSigma-Deref-Ctrl");
  SPEC_COV("EvalSigma-Move");
  SPEC_COV("EvalSigma-Alloc-Implicit");
  SPEC_COV("EvalSigma-Alloc-Implicit-Ctrl");
  SPEC_COV("EvalSigma-Alloc-Explicit");
  SPEC_COV("EvalSigma-Alloc-Explicit-Ctrl");
  SPEC_COV("ReadPlace-Ident");
  SPEC_COV("ReadPlace-Ident-Poison");
  SPEC_COV("ReadPlace-Field");
  SPEC_COV("ReadPlace-Tuple");
  SPEC_COV("ReadPlace-Index");
  SPEC_COV("ReadPlace-Index-OOB");
  SPEC_COV("ReadPlace-Index-Range");
  SPEC_COV("ReadPlace-Index-Range-OOB");
  SPEC_COV("ReadPlace-Deref");
  SPEC_COV("MovePlace-Whole");
  SPEC_COV("MovePlace-Field");
  SPEC_COV("MovePlace-Ctrl");
  SPEC_COV("ReadPtr-Safe");
  SPEC_COV("ReadPtr-Null");
  SPEC_COV("ReadPtr-Expired");
  SPEC_COV("ReadPtr-Raw");
  SPEC_COV("AddrOf-Ident");
  SPEC_COV("AddrOf-Ident-Path");
  SPEC_COV("AddrOf-Ident-Path-Poison");
  SPEC_COV("AddrOf-Field");
  SPEC_COV("AddrOf-Field-Ctrl");
  SPEC_COV("AddrOf-Tuple");
  SPEC_COV("AddrOf-Tuple-Ctrl");
  SPEC_COV("AddrOf-Index");
  SPEC_COV("AddrOfSigma-Index-Ok");
  SPEC_COV("AddrOfSigma-Index-OOB");
  SPEC_COV("AddrOf-Index-Ctrl-Base");
  SPEC_COV("AddrOf-Index-Ctrl-Idx");
  SPEC_COV("AddrOf-Deref-Safe");
  SPEC_COV("AddrOf-Deref-Null");
  SPEC_COV("AddrOf-Deref-Expired");
  SPEC_COV("AddrOf-Deref-Raw");
  SPEC_COV("AddrOf-Deref-Ctrl");

  Sigma sigma;
  PushScope(sigma, nullptr);

  cursive0::analysis::ScopeContext sema_ctx;
  cursive0::analysis::ExprTypeMap expr_types;
  sema_ctx.expr_types = &expr_types;

  SemanticsContext ctx;
  ctx.sema = &sema_ctx;

  auto lit_i32 = MakeLiteralExpr(cursive0::syntax::TokenKind::IntLiteral, "42");
  SetExprType(expr_types, lit_i32, cursive0::analysis::MakeTypePrim("i32"));
  Outcome lit_out = EvalSigma(ctx, *lit_i32, sigma);
  assert(IsVal(lit_out));
  assert(ValueEqual(std::get<Value>(lit_out.node), MakeInt("i32", 42)));

  auto opt_none = EvalOptSigma(ctx, nullptr, sigma);
  assert(std::holds_alternative<std::optional<Value>>(opt_none));
  assert(!std::get<std::optional<Value>>(opt_none).has_value());

  auto opt_some = EvalOptSigma(ctx, lit_i32, sigma);
  assert(std::holds_alternative<std::optional<Value>>(opt_some));
  assert(std::get<std::optional<Value>>(opt_some).has_value());

  auto err_expr = MakeErrorExpr();
  auto opt_ctrl = EvalOptSigma(ctx, err_expr, sigma);
  assert(std::holds_alternative<cursive0::eval::Control>(opt_ctrl));

  auto list_empty = EvalListSigma(ctx, {}, sigma);
  assert(std::holds_alternative<std::vector<Value>>(list_empty));
  assert(std::get<std::vector<Value>>(list_empty).empty());

  auto lit_i32_b = MakeLiteralExpr(cursive0::syntax::TokenKind::IntLiteral, "7");
  SetExprType(expr_types, lit_i32_b, cursive0::analysis::MakeTypePrim("i32"));
  std::vector<ExprPtr> list_exprs{lit_i32, lit_i32_b};
  auto list_out = EvalListSigma(ctx, list_exprs, sigma);
  assert(std::holds_alternative<std::vector<Value>>(list_out));
  assert(std::get<std::vector<Value>>(list_out).size() == 2);

  std::vector<cursive0::syntax::FieldInit> fields_empty;
  auto fields_out_empty = EvalFieldInitsSigma(ctx, fields_empty, sigma);
  assert(std::holds_alternative<FieldValues>(fields_out_empty));
  assert(std::get<FieldValues>(fields_out_empty).empty());

  std::vector<cursive0::syntax::FieldInit> fields = {
      MakeFieldInit("x", lit_i32),
  };
  auto fields_out = EvalFieldInitsSigma(ctx, fields, sigma);
  assert(std::holds_alternative<FieldValues>(fields_out));
  assert(std::get<FieldValues>(fields_out).size() == 1);

  auto lit_usize_lo = MakeLiteralExpr(cursive0::syntax::TokenKind::IntLiteral, "1");
  auto lit_usize_hi = MakeLiteralExpr(cursive0::syntax::TokenKind::IntLiteral, "2");
  SetExprType(expr_types, lit_usize_lo, cursive0::analysis::MakeTypePrim("usize"));
  SetExprType(expr_types, lit_usize_hi, cursive0::analysis::MakeTypePrim("usize"));
  auto range_expr = MakeRangeExpr(cursive0::syntax::RangeKind::Exclusive,
                                  lit_usize_lo, lit_usize_hi);
  Outcome range_out = EvalSigma(ctx, *range_expr, sigma);
  assert(IsVal(range_out));
  assert(std::holds_alternative<cursive0::eval::RangeVal>(std::get<Value>(range_out.node).node));

  auto range_ctrl = MakeRangeExpr(cursive0::syntax::RangeKind::Exclusive,
                                  err_expr, lit_usize_hi);
  Outcome range_ctrl_out = EvalSigma(ctx, *range_ctrl, sigma);
  assert(IsCtrl(range_ctrl_out));

  auto range_ctrl_hi = MakeRangeExpr(cursive0::syntax::RangeKind::Exclusive,
                                     lit_usize_lo, err_expr);
  Outcome range_ctrl_hi_out = EvalSigma(ctx, *range_ctrl_hi, sigma);
  assert(IsCtrl(range_ctrl_hi_out));

  auto ptr_null = MakeExpr(cursive0::syntax::PtrNullExpr{});
  Outcome ptr_out = EvalSigma(ctx, *ptr_null, sigma);
  assert(IsVal(ptr_out));
  const auto* ptr_val = std::get_if<cursive0::eval::PtrVal>(&std::get<Value>(ptr_out.node).node);
  assert(ptr_val && ptr_val->state == cursive0::analysis::PtrState::Null && ptr_val->addr == 0);

  auto tuple_expr = MakeTupleExpr({lit_i32, lit_i32_b});
  Outcome tuple_out = EvalSigma(ctx, *tuple_expr, sigma);
  assert(IsVal(tuple_out));

  auto array_expr = MakeArrayExpr({lit_i32, lit_i32_b});
  Outcome array_out = EvalSigma(ctx, *array_expr, sigma);
  assert(IsVal(array_out));

  auto record_expr = MakeRecordExpr({"Point"}, {MakeFieldInit("x", lit_i32)});
  SetExprType(expr_types, record_expr, cursive0::analysis::MakeTypePath({"Point"}));
  Outcome record_out = EvalSigma(ctx, *record_expr, sigma);
  assert(IsVal(record_out));

  auto enum_unit = MakeEnumUnitExpr({"Option", "None"});
  Outcome enum_unit_out = EvalSigma(ctx, *enum_unit, sigma);
  assert(IsVal(enum_unit_out));

  auto enum_tuple = MakeEnumTupleExpr({"Option", "Some"}, {lit_i32});
  Outcome enum_tuple_out = EvalSigma(ctx, *enum_tuple, sigma);
  assert(IsVal(enum_tuple_out));

  auto enum_record = MakeEnumRecordExpr({"Msg", "Data"}, {MakeFieldInit("value", lit_i32)});
  Outcome enum_record_out = EvalSigma(ctx, *enum_record, sigma);
  assert(IsVal(enum_record_out));

  auto field_access = MakeFieldAccessExpr(record_expr, "x");
  Outcome field_out = EvalSigma(ctx, *field_access, sigma);
  assert(IsVal(field_out));
  assert(ValueEqual(std::get<Value>(field_out.node), MakeInt("i32", 42)));

  auto tuple_access = MakeTupleAccessExpr(tuple_expr, "1");
  Outcome tuple_access_out = EvalSigma(ctx, *tuple_access, sigma);
  assert(IsVal(tuple_access_out));
  assert(ValueEqual(std::get<Value>(tuple_access_out.node), MakeInt("i32", 7)));

  auto tuple_access_ctrl = MakeTupleAccessExpr(err_expr, "0");
  Outcome tuple_access_ctrl_out = EvalSigma(ctx, *tuple_access_ctrl, sigma);
  assert(IsCtrl(tuple_access_ctrl_out));

  auto idx_expr = MakeIndexAccessExpr(array_expr, lit_usize_lo);
  Outcome idx_out = EvalSigma(ctx, *idx_expr, sigma);
  assert(IsVal(idx_out));
  assert(ValueEqual(std::get<Value>(idx_out.node), MakeInt("i32", 7)));

  auto idx_oob_lit = MakeLiteralExpr(cursive0::syntax::TokenKind::IntLiteral, "9");
  SetExprType(expr_types, idx_oob_lit, cursive0::analysis::MakeTypePrim("usize"));
  auto idx_oob = MakeIndexAccessExpr(array_expr, idx_oob_lit);
  Outcome idx_oob_out = EvalSigma(ctx, *idx_oob, sigma);
  assert(IsCtrl(idx_oob_out));
  auto idx_range = MakeIndexAccessExpr(array_expr, range_expr);
  Outcome idx_range_out = EvalSigma(ctx, *idx_range, sigma);
  assert(IsVal(idx_range_out));
  assert(std::holds_alternative<cursive0::eval::SliceVal>(std::get<Value>(idx_range_out.node).node));

  auto lit_usize_oob = MakeLiteralExpr(cursive0::syntax::TokenKind::IntLiteral, "10");
  SetExprType(expr_types, lit_usize_oob, cursive0::analysis::MakeTypePrim("usize"));
  auto range_oob = MakeRangeExpr(cursive0::syntax::RangeKind::Exclusive,
                                 lit_usize_lo, lit_usize_oob);
  auto idx_range_oob = MakeIndexAccessExpr(array_expr, range_oob);
  Outcome idx_range_oob_out = EvalSigma(ctx, *idx_range_oob, sigma);
  assert(IsCtrl(idx_range_oob_out));

  auto idx_ctrl_base = MakeIndexAccessExpr(err_expr, lit_usize_lo);
  Outcome idx_ctrl_base_out = EvalSigma(ctx, *idx_ctrl_base, sigma);
  assert(IsCtrl(idx_ctrl_base_out));

  auto idx_ctrl_idx = MakeIndexAccessExpr(array_expr, err_expr);
  Outcome idx_ctrl_idx_out = EvalSigma(ctx, *idx_ctrl_idx, sigma);
  assert(IsCtrl(idx_ctrl_idx_out));

  BindInfo bind_info;
  bind_info.mov = Movability::Mov;
  bind_info.resp = Responsibility::Alias;
  BindVal(sigma, "x", MakeInt("i32", 5), bind_info, nullptr);
  auto ident_expr = MakeIdentifierExpr("x");
  Outcome ident_out = EvalSigma(ctx, *ident_expr, sigma);
  assert(IsVal(ident_out));
  assert(ValueEqual(std::get<Value>(ident_out.node), MakeInt("i32", 5)));

  Outcome err_out = EvalSigma(ctx, *err_expr, sigma);
  assert(IsCtrl(err_out));

  auto tuple_ctrl = MakeTupleExpr({err_expr});
  Outcome tuple_ctrl_out = EvalSigma(ctx, *tuple_ctrl, sigma);
  assert(IsCtrl(tuple_ctrl_out));

  auto array_ctrl = MakeArrayExpr({err_expr});
  Outcome array_ctrl_out = EvalSigma(ctx, *array_ctrl, sigma);
  assert(IsCtrl(array_ctrl_out));

  auto record_ctrl = MakeRecordExpr({"Point"}, {MakeFieldInit("x", err_expr)});
  Outcome record_ctrl_out = EvalSigma(ctx, *record_ctrl, sigma);
  assert(IsCtrl(record_ctrl_out));

  auto enum_tuple_ctrl = MakeEnumTupleExpr({"Option", "Some"}, {err_expr});
  Outcome enum_tuple_ctrl_out = EvalSigma(ctx, *enum_tuple_ctrl, sigma);
  assert(IsCtrl(enum_tuple_ctrl_out));

  auto enum_record_ctrl =
      MakeEnumRecordExpr({"Msg", "Data"}, {MakeFieldInit("value", err_expr)});
  Outcome enum_record_ctrl_out = EvalSigma(ctx, *enum_record_ctrl, sigma);
  assert(IsCtrl(enum_record_ctrl_out));

  auto field_ctrl = MakeFieldAccessExpr(err_expr, "x");
  Outcome field_ctrl_out = EvalSigma(ctx, *field_ctrl, sigma);
  assert(IsCtrl(field_ctrl_out));

  {
    Sigma sigma_poison;
    cursive0::syntax::ModulePath mod_path = {"m"};
    const auto mod_key = cursive0::analysis::PathKeyOf(mod_path);
    const auto poison_addr = cursive0::eval::AllocAddr(sigma_poison);
    sigma_poison.store[poison_addr] = MakeBool(true);
    sigma_poison.poison_flags[mod_key] = poison_addr;

    cursive0::analysis::ScopeContext sema_poison;
    cursive0::analysis::Entity ent;
    ent.kind = cursive0::analysis::EntityKind::Value;
    ent.origin_opt = mod_path;
    ent.source = cursive0::analysis::EntitySource::Decl;
    cursive0::analysis::Scope scope;
    scope.emplace(cursive0::analysis::IdKeyOf("pval"), ent);
    sema_poison.scopes = {scope};

    SemanticsContext ctx_poison;
    ctx_poison.sema = &sema_poison;

    auto poisoned_ident = MakeIdentifierExpr("pval");
    Outcome poisoned_out = EvalSigma(ctx_poison, *poisoned_ident, sigma_poison);
    assert(IsCtrl(poisoned_out));
  }

  {
    Sigma sigma_poison;
    cursive0::syntax::ModulePath mod_path = {"m"};
    const auto mod_key = cursive0::analysis::PathKeyOf(mod_path);
    const auto poison_addr = cursive0::eval::AllocAddr(sigma_poison);
    sigma_poison.store[poison_addr] = MakeBool(true);
    sigma_poison.poison_flags[mod_key] = poison_addr;

    cursive0::analysis::ScopeContext sema_poison;
    cursive0::analysis::Entity ent;
    ent.kind = cursive0::analysis::EntityKind::Type;
    ent.origin_opt = mod_path;
    ent.source = cursive0::analysis::EntitySource::Decl;
    cursive0::analysis::Scope scope;
    scope.emplace(cursive0::analysis::IdKeyOf("Rec"), ent);
    sema_poison.scopes = {scope};

    SemanticsContext ctx_poison;
    ctx_poison.sema = &sema_poison;

    auto poisoned_ident = MakeIdentifierExpr("Rec");
    Outcome poisoned_out = EvalSigma(ctx_poison, *poisoned_ident, sigma_poison);
    assert(IsCtrl(poisoned_out));
  }

  {
    Sigma sigma_path;
    const cursive0::syntax::ModulePath current = {"main"};
    const cursive0::syntax::ModulePath mod_path = {"m"};
    const auto mod_key = cursive0::analysis::PathKeyOf(mod_path);
    const auto addr = cursive0::eval::AllocAddr(sigma_path);
    sigma_path.store[addr] = MakeInt("i32", 99);
    sigma_path.static_addrs[{mod_key, "pval"}] = addr;

    cursive0::analysis::ScopeContext sema_path;
    sema_path.current_module = current;

    cursive0::analysis::NameMapTable name_maps;
    name_maps.emplace(cursive0::analysis::PathKeyOf(current),
                      cursive0::analysis::NameMap{});
    name_maps.emplace(mod_key, cursive0::analysis::NameMap{});
    AddName(name_maps, mod_path, "pval", cursive0::analysis::EntityKind::Value,
            mod_path);

    cursive0::analysis::ModuleNames module_names = {
        cursive0::core::StringOfPath(current),
        cursive0::core::StringOfPath(mod_path),
    };

    SemanticsContext ctx_path;
    ctx_path.sema = &sema_path;
    ctx_path.name_maps = &name_maps;
    ctx_path.module_names = &module_names;

    auto path_expr = MakePathExpr(mod_path, "pval");
    Outcome path_out = EvalSigma(ctx_path, *path_expr, sigma_path);
    assert(IsVal(path_out));
    assert(ValueEqual(std::get<Value>(path_out.node), MakeInt("i32", 99)));
  }

  {
    Sigma sigma_poison;
    const cursive0::syntax::ModulePath current = {"main"};
    const cursive0::syntax::ModulePath mod_path = {"m"};
    const auto mod_key = cursive0::analysis::PathKeyOf(mod_path);
    const auto poison_addr = cursive0::eval::AllocAddr(sigma_poison);
    sigma_poison.store[poison_addr] = MakeBool(true);
    sigma_poison.poison_flags[mod_key] = poison_addr;

    cursive0::analysis::ScopeContext sema_path;
    sema_path.current_module = current;

    cursive0::analysis::NameMapTable name_maps;
    name_maps.emplace(cursive0::analysis::PathKeyOf(current),
                      cursive0::analysis::NameMap{});
    name_maps.emplace(mod_key, cursive0::analysis::NameMap{});
    AddName(name_maps, mod_path, "pval", cursive0::analysis::EntityKind::Value,
            mod_path);

    cursive0::analysis::ModuleNames module_names = {
        cursive0::core::StringOfPath(current),
        cursive0::core::StringOfPath(mod_path),
    };

    SemanticsContext ctx_path;
    ctx_path.sema = &sema_path;
    ctx_path.name_maps = &name_maps;
    ctx_path.module_names = &module_names;

    auto path_expr = MakePathExpr(mod_path, "pval");
    Outcome path_out = EvalSigma(ctx_path, *path_expr, sigma_poison);
    assert(IsCtrl(path_out));
  }

  {
    Sigma sigma_poison;
    const cursive0::syntax::ModulePath current = {"main"};
    const cursive0::syntax::ModulePath mod_path = {"m"};
    const auto mod_key = cursive0::analysis::PathKeyOf(mod_path);
    const auto poison_addr = cursive0::eval::AllocAddr(sigma_poison);
    sigma_poison.store[poison_addr] = MakeBool(true);
    sigma_poison.poison_flags[mod_key] = poison_addr;

    cursive0::analysis::ScopeContext sema_path;
    sema_path.current_module = current;
    const cursive0::syntax::TypePath type_path = {"m", "Rec"};
    sema_path.sigma.types.emplace(cursive0::analysis::PathKeyOf(type_path),
                                  MakeRecordDecl("Rec"));

    cursive0::analysis::NameMapTable name_maps;
    name_maps.emplace(cursive0::analysis::PathKeyOf(current),
                      cursive0::analysis::NameMap{});
    name_maps.emplace(mod_key, cursive0::analysis::NameMap{});
    AddName(name_maps, mod_path, "Rec", cursive0::analysis::EntityKind::Type,
            mod_path);

    cursive0::analysis::ModuleNames module_names = {
        cursive0::core::StringOfPath(current),
        cursive0::core::StringOfPath(mod_path),
    };

    SemanticsContext ctx_path;
    ctx_path.sema = &sema_path;
    ctx_path.name_maps = &name_maps;
    ctx_path.module_names = &module_names;

    auto path_expr = MakePathExpr(mod_path, "Rec");
    Outcome path_out = EvalSigma(ctx_path, *path_expr, sigma_poison);
    assert(IsCtrl(path_out));
  }

  auto bool_true = MakeLiteralExpr(cursive0::syntax::TokenKind::BoolLiteral, "true");
  auto bool_false = MakeLiteralExpr(cursive0::syntax::TokenKind::BoolLiteral, "false");
  SetExprType(expr_types, bool_true, cursive0::analysis::MakeTypePrim("bool"));
  SetExprType(expr_types, bool_false, cursive0::analysis::MakeTypePrim("bool"));

  {
    cursive0::syntax::UnaryExpr un_not;
    un_not.op = "!";
    un_not.value = bool_true;
    auto un_not_expr = MakeExpr(std::move(un_not));
    Outcome un_not_out = EvalSigma(ctx, *un_not_expr, sigma);
    assert(IsVal(un_not_out));
    assert(ValueEqual(std::get<Value>(un_not_out.node), MakeBool(false)));
  }

  {
    cursive0::syntax::UnaryExpr un_neg;
    un_neg.op = "-";
    un_neg.value = lit_i32;
    auto un_neg_expr = MakeExpr(std::move(un_neg));
    Outcome un_neg_out = EvalSigma(ctx, *un_neg_expr, sigma);
    assert(IsVal(un_neg_out));
    cursive0::eval::IntVal expected;
    expected.type = "i32";
    expected.negative = true;
    expected.magnitude = cursive0::core::UInt128FromU64(42);
    assert(ValueEqual(std::get<Value>(un_neg_out.node), Value{expected}));
  }

  {
    auto lit_f32 = MakeLiteralExpr(cursive0::syntax::TokenKind::FloatLiteral, "1.0");
    SetExprType(expr_types, lit_f32, cursive0::analysis::MakeTypePrim("f32"));
    cursive0::syntax::UnaryExpr un_bad;
    un_bad.op = "!";
    un_bad.value = lit_f32;
    auto un_bad_expr = MakeExpr(std::move(un_bad));
    Outcome un_bad_out = EvalSigma(ctx, *un_bad_expr, sigma);
    assert(IsCtrl(un_bad_out));
  }

  {
    cursive0::syntax::UnaryExpr un_ctrl;
    un_ctrl.op = "!";
    un_ctrl.value = err_expr;
    auto un_ctrl_expr = MakeExpr(std::move(un_ctrl));
    Outcome un_ctrl_out = EvalSigma(ctx, *un_ctrl_expr, sigma);
    assert(IsCtrl(un_ctrl_out));
  }

  {
    cursive0::syntax::BinaryExpr bin_and;
    bin_and.op = "&&";
    bin_and.lhs = bool_false;
    bin_and.rhs = err_expr;
    auto bin_and_expr = MakeExpr(std::move(bin_and));
    Outcome bin_and_out = EvalSigma(ctx, *bin_and_expr, sigma);
    assert(IsVal(bin_and_out));
    assert(ValueEqual(std::get<Value>(bin_and_out.node), MakeBool(false)));
  }

  {
    cursive0::syntax::BinaryExpr bin_and;
    bin_and.op = "&&";
    bin_and.lhs = bool_true;
    bin_and.rhs = bool_false;
    auto bin_and_expr = MakeExpr(std::move(bin_and));
    Outcome bin_and_out = EvalSigma(ctx, *bin_and_expr, sigma);
    assert(IsVal(bin_and_out));
    assert(ValueEqual(std::get<Value>(bin_and_out.node), MakeBool(false)));
  }

  {
    cursive0::syntax::BinaryExpr bin_or;
    bin_or.op = "||";
    bin_or.lhs = bool_true;
    bin_or.rhs = err_expr;
    auto bin_or_expr = MakeExpr(std::move(bin_or));
    Outcome bin_or_out = EvalSigma(ctx, *bin_or_expr, sigma);
    assert(IsVal(bin_or_out));
    assert(ValueEqual(std::get<Value>(bin_or_out.node), MakeBool(true)));
  }

  {
    cursive0::syntax::BinaryExpr bin_or;
    bin_or.op = "||";
    bin_or.lhs = bool_false;
    bin_or.rhs = bool_true;
    auto bin_or_expr = MakeExpr(std::move(bin_or));
    Outcome bin_or_out = EvalSigma(ctx, *bin_or_expr, sigma);
    assert(IsVal(bin_or_out));
    assert(ValueEqual(std::get<Value>(bin_or_out.node), MakeBool(true)));
  }

  {
    cursive0::syntax::BinaryExpr bin_add;
    bin_add.op = "+";
    bin_add.lhs = lit_i32;
    bin_add.rhs = lit_i32_b;
    auto bin_add_expr = MakeExpr(std::move(bin_add));
    Outcome bin_add_out = EvalSigma(ctx, *bin_add_expr, sigma);
    assert(IsVal(bin_add_out));
  }

  {
    auto lit_zero = MakeLiteralExpr(cursive0::syntax::TokenKind::IntLiteral, "0");
    SetExprType(expr_types, lit_zero, cursive0::analysis::MakeTypePrim("i32"));
    cursive0::syntax::BinaryExpr bin_div;
    bin_div.op = "/";
    bin_div.lhs = lit_i32;
    bin_div.rhs = lit_zero;
    auto bin_div_expr = MakeExpr(std::move(bin_div));
    Outcome bin_div_out = EvalSigma(ctx, *bin_div_expr, sigma);
    assert(IsCtrl(bin_div_out));
  }

  {
    cursive0::syntax::BinaryExpr bin_ctrl_l;
    bin_ctrl_l.op = "+";
    bin_ctrl_l.lhs = err_expr;
    bin_ctrl_l.rhs = lit_i32;
    auto bin_ctrl_expr = MakeExpr(std::move(bin_ctrl_l));
    Outcome bin_ctrl_out = EvalSigma(ctx, *bin_ctrl_expr, sigma);
    assert(IsCtrl(bin_ctrl_out));
  }

  {
    cursive0::syntax::BinaryExpr bin_ctrl_r;
    bin_ctrl_r.op = "+";
    bin_ctrl_r.lhs = lit_i32;
    bin_ctrl_r.rhs = err_expr;
    auto bin_ctrl_expr = MakeExpr(std::move(bin_ctrl_r));
    Outcome bin_ctrl_out = EvalSigma(ctx, *bin_ctrl_expr, sigma);
    assert(IsCtrl(bin_ctrl_out));
  }

  {
    cursive0::syntax::CastExpr cast;
    cast.value = bool_true;
    cast.type = MakePrimType("i32");
    auto cast_expr = MakeExpr(std::move(cast));
    SetExprType(expr_types, cast_expr, cursive0::analysis::MakeTypePrim("i32"));
    Outcome cast_out = EvalSigma(ctx, *cast_expr, sigma);
    assert(IsVal(cast_out));
    assert(ValueEqual(std::get<Value>(cast_out.node), MakeInt("i32", 1)));
  }

  {
    cursive0::syntax::CastExpr cast;
    cast.value = bool_true;
    cast.type = MakePrimType("char");
    auto cast_expr = MakeExpr(std::move(cast));
    SetExprType(expr_types, cast_expr, cursive0::analysis::MakeTypePrim("char"));
    Outcome cast_out = EvalSigma(ctx, *cast_expr, sigma);
    assert(IsCtrl(cast_out));
  }

  {
    cursive0::syntax::CastExpr cast;
    cast.value = err_expr;
    cast.type = MakePrimType("i32");
    auto cast_expr = MakeExpr(std::move(cast));
    SetExprType(expr_types, cast_expr, cursive0::analysis::MakeTypePrim("i32"));
    Outcome cast_out = EvalSigma(ctx, *cast_expr, sigma);
    assert(IsCtrl(cast_out));
  }

  {
    cursive0::syntax::TransmuteExpr trans;
    trans.from = MakePrimType("i32");
    trans.to = MakePrimType("u32");
    trans.value = lit_i32;
    auto trans_expr = MakeExpr(std::move(trans));
    SetExprType(expr_types, trans_expr, cursive0::analysis::MakeTypePrim("u32"));
    Outcome trans_out = EvalSigma(ctx, *trans_expr, sigma);
    assert(IsVal(trans_out));
  }

  {
    cursive0::syntax::TransmuteExpr trans;
    trans.from = MakePrimType("i32");
    trans.to = MakePrimType("u32");
    trans.value = err_expr;
    auto trans_expr = MakeExpr(std::move(trans));
    SetExprType(expr_types, trans_expr, cursive0::analysis::MakeTypePrim("u32"));
    Outcome trans_out = EvalSigma(ctx, *trans_expr, sigma);
    assert(IsCtrl(trans_out));
  }

  {
    auto type_i32 = cursive0::analysis::MakeTypePrim("i32");
    auto type_bool = cursive0::analysis::MakeTypePrim("bool");
    auto union_type = cursive0::analysis::MakeTypeUnion({type_i32, type_bool});
    ctx.ret_type = type_i32;

    Value union_ok;
    cursive0::eval::UnionVal uni_ok;
    uni_ok.member = type_bool;
    uni_ok.value = std::make_shared<Value>(MakeBool(true));
    union_ok.node = uni_ok;
    BindVal(sigma, "u_ok", union_ok, bind_info, nullptr);

    auto union_ok_expr = MakeIdentifierExpr("u_ok");
    SetExprType(expr_types, union_ok_expr, union_type);
    cursive0::syntax::PropagateExpr prop;
    prop.value = union_ok_expr;
    auto prop_expr = MakeExpr(std::move(prop));
    Outcome prop_out = EvalSigma(ctx, *prop_expr, sigma);
    assert(IsVal(prop_out));
    assert(ValueEqual(std::get<Value>(prop_out.node), MakeBool(true)));

    Value union_err;
    cursive0::eval::UnionVal uni_err;
    uni_err.member = type_i32;
    uni_err.value = std::make_shared<Value>(MakeInt("i32", 13));
    union_err.node = uni_err;
    BindVal(sigma, "u_err", union_err, bind_info, nullptr);

    auto union_err_expr = MakeIdentifierExpr("u_err");
    SetExprType(expr_types, union_err_expr, union_type);
    cursive0::syntax::PropagateExpr prop_err;
    prop_err.value = union_err_expr;
    auto prop_err_expr = MakeExpr(std::move(prop_err));
    Outcome prop_err_out = EvalSigma(ctx, *prop_err_expr, sigma);
    assert(IsCtrl(prop_err_out));
    const auto ctrl = std::get<cursive0::eval::Control>(prop_err_out.node);
    assert(ctrl.kind == cursive0::eval::ControlKind::Return);
    assert(ctrl.value.has_value());
    assert(ValueEqual(*ctrl.value, MakeInt("i32", 13)));
  }

  {
    auto addr_expr = MakeExpr(cursive0::syntax::AddressOfExpr{MakeIdentifierExpr("x")});
    Outcome addr_out = EvalSigma(ctx, *addr_expr, sigma);
    assert(IsVal(addr_out));
    const auto* ptr =
        std::get_if<cursive0::eval::PtrVal>(&std::get<Value>(addr_out.node).node);
    assert(ptr && ptr->state == cursive0::analysis::PtrState::Valid);

    auto deref_expr = MakeExpr(cursive0::syntax::DerefExpr{addr_expr});
    Outcome deref_out = EvalSigma(ctx, *deref_expr, sigma);
    assert(IsVal(deref_out));
    assert(ValueEqual(std::get<Value>(deref_out.node), MakeInt("i32", 5)));
  }

  {
    cursive0::eval::PtrVal null_ptr;
    null_ptr.state = cursive0::analysis::PtrState::Null;
    null_ptr.addr = 0;
    BindVal(sigma, "pnull", Value{null_ptr}, bind_info, nullptr);
    auto deref_expr = MakeExpr(cursive0::syntax::DerefExpr{MakeIdentifierExpr("pnull")});
    Outcome deref_out = EvalSigma(ctx, *deref_expr, sigma);
    assert(IsCtrl(deref_out));

    auto addr_deref_expr = MakeExpr(
        cursive0::syntax::AddressOfExpr{
            MakeExpr(cursive0::syntax::DerefExpr{MakeIdentifierExpr("pnull")})});
    Outcome addr_deref_out = EvalSigma(ctx, *addr_deref_expr, sigma);
    assert(IsCtrl(addr_deref_out));
  }

  {
    const auto raw_addr = cursive0::eval::AllocAddr(sigma);
    sigma.store[raw_addr] = MakeInt("i32", 77);
    cursive0::eval::RawPtrVal raw_ptr;
    raw_ptr.qual = cursive0::analysis::RawPtrQual::Imm;
    raw_ptr.addr = raw_addr;
    BindVal(sigma, "rawptr", Value{raw_ptr}, bind_info, nullptr);
    auto deref_expr = MakeExpr(cursive0::syntax::DerefExpr{MakeIdentifierExpr("rawptr")});
    Outcome deref_out = EvalSigma(ctx, *deref_expr, sigma);
    assert(IsVal(deref_out));
    assert(ValueEqual(std::get<Value>(deref_out.node), MakeInt("i32", 77)));
  }

  {
    cursive0::eval::ArrayVal arr;
    arr.elements = {MakeInt("i32", 1), MakeInt("i32", 2)};
    BindVal(sigma, "arr", Value{arr}, bind_info, nullptr);

    auto idx_one = MakeLiteralExpr(cursive0::syntax::TokenKind::IntLiteral, "1");
    SetExprType(expr_types, idx_one, cursive0::analysis::MakeTypePrim("usize"));
    auto index_expr = MakeIndexAccessExpr(MakeIdentifierExpr("arr"), idx_one);
    auto addr_expr = MakeExpr(cursive0::syntax::AddressOfExpr{index_expr});
    Outcome addr_out = EvalSigma(ctx, *addr_expr, sigma);
    assert(IsVal(addr_out));
    auto deref_expr = MakeExpr(cursive0::syntax::DerefExpr{addr_expr});
    Outcome deref_out = EvalSigma(ctx, *deref_expr, sigma);
    assert(IsVal(deref_out));
    assert(ValueEqual(std::get<Value>(deref_out.node), MakeInt("i32", 2)));

    auto idx_oob = MakeLiteralExpr(cursive0::syntax::TokenKind::IntLiteral, "9");
    SetExprType(expr_types, idx_oob, cursive0::analysis::MakeTypePrim("usize"));
    auto addr_oob_expr =
        MakeExpr(cursive0::syntax::AddressOfExpr{
            MakeIndexAccessExpr(MakeIdentifierExpr("arr"), idx_oob)});
    Outcome addr_oob_out = EvalSigma(ctx, *addr_oob_expr, sigma);
    assert(IsCtrl(addr_oob_out));

    auto addr_ctrl_idx =
        MakeExpr(cursive0::syntax::AddressOfExpr{
            MakeIndexAccessExpr(MakeIdentifierExpr("arr"), err_expr)});
    Outcome addr_ctrl_idx_out = EvalSigma(ctx, *addr_ctrl_idx, sigma);
    assert(IsCtrl(addr_ctrl_idx_out));

    auto addr_ctrl_base =
        MakeExpr(cursive0::syntax::AddressOfExpr{
            MakeIndexAccessExpr(
                MakeExpr(cursive0::syntax::DerefExpr{MakeIdentifierExpr("pnull")}),
                idx_one)});
    Outcome addr_ctrl_base_out = EvalSigma(ctx, *addr_ctrl_base, sigma);
    assert(IsCtrl(addr_ctrl_base_out));
  }

  {
    auto move_expr = MakeExpr(cursive0::syntax::MoveExpr{MakeIdentifierExpr("x")});
    Outcome move_out = EvalSigma(ctx, *move_expr, sigma);
    assert(IsVal(move_out));
    const auto binding = cursive0::eval::LookupBind(sigma, "x");
    assert(binding.has_value());
    const auto state = cursive0::eval::BindStateOf(sigma, *binding);
    assert(state.has_value());
    assert(state->kind == cursive0::eval::BindState::Kind::Moved);
  }

  {
    cursive0::eval::RecordVal rec;
    rec.record_type = cursive0::analysis::MakeTypePath({"Point"});
    rec.fields = {{"x", MakeInt("i32", 8)}, {"y", MakeInt("i32", 9)}};
    cursive0::eval::Binding rec_binding;
    BindVal(sigma, "rec_move", Value{rec}, bind_info, &rec_binding);
    auto field_expr = MakeFieldAccessExpr(MakeIdentifierExpr("rec_move"), "x");
    auto move_expr = MakeExpr(cursive0::syntax::MoveExpr{field_expr});
    Outcome move_out = EvalSigma(ctx, *move_expr, sigma);
    assert(IsVal(move_out));
    const auto state = cursive0::eval::BindStateOf(sigma, rec_binding);
    assert(state.has_value());
    assert(state->kind == cursive0::eval::BindState::Kind::PartiallyMoved);
    assert(state->fields.count("x") == 1);
  }

  {
    cursive0::eval::RegionEntry region;
    region.tag = 1;
    region.target = 1;
    region.scope = cursive0::eval::CurrentScopeId(sigma);
    region.mark = std::nullopt;
    sigma.region_stack.push_back(region);

    auto alloc_expr = MakeExpr(cursive0::syntax::AllocExpr{std::nullopt, lit_i32});
    Outcome alloc_out = EvalSigma(ctx, *alloc_expr, sigma);
    assert(IsVal(alloc_out));

    cursive0::eval::RecordVal region_val;
    region_val.record_type = cursive0::analysis::MakeTypeModalState({"Region"}, "@Active");
    region_val.fields = {{"handle", MakeInt("usize", 1)}};
    BindVal(sigma, "r", Value{region_val}, bind_info, nullptr);
    cursive0::syntax::AllocExpr alloc_explicit;
    alloc_explicit.region_opt = std::string("r");
    alloc_explicit.value = lit_i32_b;
    auto alloc_expr_explicit = MakeExpr(std::move(alloc_explicit));
    Outcome alloc_out_explicit = EvalSigma(ctx, *alloc_expr_explicit, sigma);
    assert(IsVal(alloc_out_explicit));

    cursive0::syntax::AllocExpr alloc_ctrl;
    alloc_ctrl.region_opt = std::nullopt;
    alloc_ctrl.value = err_expr;
    auto alloc_expr_ctrl = MakeExpr(std::move(alloc_ctrl));
    Outcome alloc_ctrl_out = EvalSigma(ctx, *alloc_expr_ctrl, sigma);
    assert(IsCtrl(alloc_ctrl_out));
  }

  return 0;
}

