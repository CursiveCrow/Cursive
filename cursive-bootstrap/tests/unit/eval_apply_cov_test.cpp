#include <cassert>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/int128.h"
#include "cursive0/analysis/types/context.h"
#include "cursive0/analysis/resolve/scopes.h"
#include "cursive0/analysis/types/types.h"
#include "cursive0/eval/apply.h"
#include "cursive0/eval/eval.h"
#include "cursive0/eval/state.h"
#include "cursive0/eval/value.h"
#include "cursive0/syntax/ast.h"
#include "cursive0/syntax/token.h"

namespace {

using cursive0::core::UInt128FromU64;
using cursive0::eval::BindingValue;
using cursive0::eval::Control;
using cursive0::eval::ControlKind;
using cursive0::eval::IntVal;
using cursive0::eval::IsCtrl;
using cursive0::eval::IsVal;
using cursive0::eval::MethodTarget;
using cursive0::eval::Outcome;
using cursive0::eval::Sigma;
using cursive0::eval::SemanticsContext;
using cursive0::eval::Value;
using cursive0::eval::ValueEqual;
using cursive0::syntax::Block;
using cursive0::syntax::Expr;
using cursive0::syntax::ExprPtr;
using cursive0::syntax::FieldDecl;
using cursive0::syntax::LiteralExpr;
using cursive0::syntax::ProcedureDecl;
using cursive0::syntax::RecordDecl;
using cursive0::syntax::ReceiverExplicit;
using cursive0::syntax::Token;
using cursive0::syntax::TokenKind;
using cursive0::syntax::Type;
using cursive0::syntax::TypePrim;
using cursive0::syntax::Visibility;

cursive0::core::Span EmptySpan() {
  return {};
}

static ExprPtr MakeExpr(cursive0::syntax::ExprNode node) {
  auto expr = std::make_shared<Expr>();
  expr->span = EmptySpan();
  expr->node = std::move(node);
  return expr;
}

static ExprPtr MakeLiteralExpr(TokenKind kind, std::string lexeme) {
  Token tok;
  tok.kind = kind;
  tok.lexeme = std::move(lexeme);
  LiteralExpr lit;
  lit.literal = tok;
  return MakeExpr(std::move(lit));
}

static ExprPtr MakeErrorExpr() {
  return MakeExpr(cursive0::syntax::ErrorExpr{});
}

static std::shared_ptr<Type> MakePrimType(std::string name) {
  auto type = std::make_shared<Type>();
  type->span = EmptySpan();
  type->node = TypePrim{std::move(name)};
  return type;
}

static std::shared_ptr<Block> MakeBlockWithTail(const ExprPtr& tail) {
  auto block = std::make_shared<Block>();
  block->span = EmptySpan();
  block->tail_opt = tail;
  return block;
}

static Value MakeInt(std::string type, std::uint64_t value) {
  IntVal v;
  v.type = std::move(type);
  v.negative = false;
  v.magnitude = UInt128FromU64(value);
  return Value{v};
}

static void SetExprType(cursive0::analysis::ExprTypeMap& map,
                        const ExprPtr& expr,
                        cursive0::analysis::TypeRef type) {
  map[expr.get()] = std::move(type);
}

static FieldDecl MakeField(std::string name, ExprPtr init) {
  FieldDecl field;
  field.vis = Visibility::Public;
  field.name = std::move(name);
  field.type = MakePrimType("i32");
  field.init_opt = std::move(init);
  field.span = EmptySpan();
  field.doc_opt = std::nullopt;
  return field;
}

void TestApplyProcSigma() {
  SPEC_COV("ApplyProcSigma");

  cursive0::analysis::ScopeContext sema_ctx;
  cursive0::analysis::ExprTypeMap expr_types;
  sema_ctx.expr_types = &expr_types;

  SemanticsContext ctx;
  ctx.sema = &sema_ctx;

  auto lit = MakeLiteralExpr(TokenKind::IntLiteral, "7");
  SetExprType(expr_types, lit, cursive0::analysis::MakeTypePrim("i32"));

  ProcedureDecl proc;
  proc.vis = Visibility::Public;
  proc.name = "answer";
  proc.params = {};
  proc.return_type_opt = MakePrimType("i32");
  proc.body = MakeBlockWithTail(lit);
  proc.span = EmptySpan();

  Sigma sigma;
  Outcome out = cursive0::eval::ApplyProcSigma(ctx, proc, {}, sigma);
  assert(IsVal(out));
  assert(ValueEqual(std::get<Value>(out.node), MakeInt("i32", 7)));
}

void TestApplyRecordCtorSigma() {
  SPEC_COV("ApplyRecordCtorSigma");

  cursive0::analysis::ScopeContext sema_ctx;
  cursive0::analysis::ExprTypeMap expr_types;
  sema_ctx.expr_types = &expr_types;

  auto lit_x = MakeLiteralExpr(TokenKind::IntLiteral, "1");
  auto lit_y = MakeLiteralExpr(TokenKind::IntLiteral, "2");
  SetExprType(expr_types, lit_x, cursive0::analysis::MakeTypePrim("i32"));
  SetExprType(expr_types, lit_y, cursive0::analysis::MakeTypePrim("i32"));

  RecordDecl record;
  record.vis = Visibility::Public;
  record.name = "Point";
  record.implements = {};
  record.members = {MakeField("x", lit_x), MakeField("y", lit_y)};
  record.span = EmptySpan();
  record.doc = {};

  sema_ctx.sigma.types[cursive0::analysis::PathKeyOf({"Point"})] = record;

  SemanticsContext ctx;
  ctx.sema = &sema_ctx;

  Sigma sigma;
  Outcome out =
      cursive0::eval::ApplyRecordCtorSigma(ctx, {"Point"}, sigma);
  assert(IsVal(out));
  const auto& value = std::get<Value>(out.node);
  const auto* record_val = std::get_if<cursive0::eval::RecordVal>(&value.node);
  assert(record_val);
  assert(record_val->fields.size() == 2);
  assert(record_val->fields[0].first == "x");
  assert(record_val->fields[1].first == "y");
}

void TestApplyRecordCtorSigmaCtrl() {
  SPEC_COV("ApplyRecordCtorSigma-Ctrl");

  cursive0::analysis::ScopeContext sema_ctx;
  RecordDecl record;
  record.vis = Visibility::Public;
  record.name = "Bad";
  record.implements = {};
  record.members = {MakeField("x", MakeErrorExpr())};
  record.span = EmptySpan();
  record.doc = {};
  sema_ctx.sigma.types[cursive0::analysis::PathKeyOf({"Bad"})] = record;

  SemanticsContext ctx;
  ctx.sema = &sema_ctx;

  Sigma sigma;
  Outcome out =
      cursive0::eval::ApplyRecordCtorSigma(ctx, {"Bad"}, sigma);
  assert(IsCtrl(out));
}

void TestApplyMethodSigma() {
  SPEC_COV("ApplyMethodSigma");

  cursive0::analysis::ScopeContext sema_ctx;
  cursive0::analysis::ExprTypeMap expr_types;
  sema_ctx.expr_types = &expr_types;

  auto lit = MakeLiteralExpr(TokenKind::IntLiteral, "5");
  SetExprType(expr_types, lit, cursive0::analysis::MakeTypePrim("i32"));

  auto body = MakeBlockWithTail(lit);

  cursive0::syntax::MethodDecl method;
  method.vis = Visibility::Public;
  method.name = "get";
  method.params = {};
  method.return_type_opt = MakePrimType("i32");
  ReceiverExplicit recv;
  recv.mode_opt = std::nullopt;
  recv.type = MakePrimType("i32");
  method.receiver = recv;
  method.body = body;
  method.span = EmptySpan();

  MethodTarget target;
  target.kind = MethodTarget::Kind::Record;
  target.record_method = &method;

  SemanticsContext ctx;
  ctx.sema = &sema_ctx;

  Expr base;
  base.span = EmptySpan();
  base.node = cursive0::syntax::ErrorExpr{};

  Value self_value{cursive0::eval::UnitVal{}};
  BindingValue self_arg = self_value;

  Sigma sigma;
  Outcome out = cursive0::eval::ApplyMethodSigma(
      ctx, base, "get", self_value, self_arg, {}, target, sigma);
  assert(IsVal(out));
  assert(ValueEqual(std::get<Value>(out.node), MakeInt("i32", 5)));
}

void TestApplyMethodPrim() {
  SPEC_COV("ApplyMethodSigma-Prim");
  SPEC_COV("ApplyMethod-Prim");
  SPEC_COV("ApplyMethod-Prim-Step");

  SemanticsContext ctx;
  Sigma sigma;

  MethodTarget target;
  target.kind = MethodTarget::Kind::Class;
  target.class_method = nullptr;
  target.owner_class = {"System"};

  Expr base;
  base.span = EmptySpan();
  base.node = cursive0::syntax::ErrorExpr{};

  Value self_value{cursive0::eval::UnitVal{}};
  BindingValue self_arg = self_value;

  Outcome out = cursive0::eval::ApplyMethodSigma(
      ctx, base, "exit", self_value, self_arg, {}, target, sigma);
  assert(IsCtrl(out));
  const auto& ctrl = std::get<Control>(out.node);
  assert(ctrl.kind == ControlKind::Abort);
}

void TestApplyRegionProc() {
  SPEC_COV("ApplyRegionProc-NewScoped");
  SPEC_COV("ApplyRegionProc-Alloc");
  SPEC_COV("ApplyRegionProc-Reset");
  SPEC_COV("ApplyRegionProc-Freeze");
  SPEC_COV("ApplyRegionProc-Thaw");
  SPEC_COV("ApplyRegionProc-Free");

  SemanticsContext ctx;
  Sigma sigma;
  cursive0::eval::PushScope(sigma, nullptr);

  BindingValue opts = Value{cursive0::eval::UnitVal{}};

  auto scoped_out = cursive0::eval::ApplyRegionProc(
      ctx, "new_scoped", {opts}, sigma);
  assert(IsVal(scoped_out));
  const auto region = std::get<Value>(scoped_out.node);

  auto alloc_out = cursive0::eval::ApplyRegionProc(
      ctx, "alloc", {region, MakeInt("i32", 1)}, sigma);
  assert(IsVal(alloc_out));

  const auto region_reset = std::get<Value>(
      cursive0::eval::ApplyRegionProc(ctx, "new_scoped", {opts}, sigma).node);
  auto reset_out = cursive0::eval::ApplyRegionProc(
      ctx, "reset_unchecked", {region_reset}, sigma);
  assert(IsVal(reset_out));

  const auto region_freeze = std::get<Value>(
      cursive0::eval::ApplyRegionProc(ctx, "new_scoped", {opts}, sigma).node);
  auto freeze_out = cursive0::eval::ApplyRegionProc(
      ctx, "freeze", {region_freeze}, sigma);
  assert(IsVal(freeze_out));

  const auto region_thaw = std::get<Value>(
      cursive0::eval::ApplyRegionProc(ctx, "new_scoped", {opts}, sigma).node);
  auto thaw_out = cursive0::eval::ApplyRegionProc(
      ctx, "thaw", {region_thaw}, sigma);
  assert(IsVal(thaw_out));

  const auto region_free = std::get<Value>(
      cursive0::eval::ApplyRegionProc(ctx, "new_scoped", {opts}, sigma).node);
  auto free_out = cursive0::eval::ApplyRegionProc(
      ctx, "free_unchecked", {region_free}, sigma);
  assert(IsVal(free_out));
}

}  // namespace

int main() {
  TestApplyProcSigma();
  TestApplyRecordCtorSigma();
  TestApplyRecordCtorSigmaCtrl();
  TestApplyMethodSigma();
  TestApplyMethodPrim();
  TestApplyRegionProc();
  return 0;
}
