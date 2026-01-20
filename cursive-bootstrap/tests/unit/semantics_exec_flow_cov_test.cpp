#include <cassert>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/sema/context.h"
#include "cursive0/sema/scopes.h"
#include "cursive0/sema/types.h"
#include "cursive0/semantics/exec.h"
#include "cursive0/semantics/eval.h"
#include "cursive0/semantics/state.h"
#include "cursive0/semantics/value.h"
#include "cursive0/syntax/ast.h"
#include "cursive0/syntax/token.h"

namespace {

using cursive0::sema::ExprTypeMap;
using cursive0::sema::ScopeContext;
using cursive0::sema::TypeRef;
using cursive0::semantics::BindInfo;
using cursive0::semantics::BindVal;
using cursive0::semantics::Control;
using cursive0::semantics::ControlKind;
using cursive0::semantics::ExecSeqSigma;
using cursive0::semantics::ExecSigma;
using cursive0::semantics::IsCtrl;
using cursive0::semantics::IsVal;
using cursive0::semantics::MakeCtrl;
using cursive0::semantics::MakeVal;
using cursive0::semantics::Movability;
using cursive0::semantics::Outcome;
using cursive0::semantics::PushScope;
using cursive0::semantics::RegionEntry;
using cursive0::semantics::RegionTag;
using cursive0::semantics::RegionTarget;
using cursive0::semantics::Responsibility;
using cursive0::semantics::SemanticsContext;
using cursive0::semantics::Sigma;
using cursive0::semantics::StmtOut;
using cursive0::semantics::Value;

using cursive0::syntax::AssignStmt;
using cursive0::syntax::Binding;
using cursive0::syntax::Block;
using cursive0::syntax::BreakStmt;
using cursive0::syntax::CompoundAssignStmt;
using cursive0::syntax::ContinueStmt;
using cursive0::syntax::DeferStmt;
using cursive0::syntax::ErrorStmt;
using cursive0::syntax::Expr;
using cursive0::syntax::ExprNode;
using cursive0::syntax::ExprPtr;
using cursive0::syntax::ExprStmt;
using cursive0::syntax::FrameStmt;
using cursive0::syntax::IdentifierPattern;
using cursive0::syntax::LetStmt;
using cursive0::syntax::Pattern;
using cursive0::syntax::PatternNode;
using cursive0::syntax::RegionStmt;
using cursive0::syntax::ResultStmt;
using cursive0::syntax::ReturnStmt;
using cursive0::syntax::ShadowLetStmt;
using cursive0::syntax::ShadowVarStmt;
using cursive0::syntax::Stmt;
using cursive0::syntax::Token;
using cursive0::syntax::TokenKind;
using cursive0::syntax::UnsafeBlockStmt;
using cursive0::syntax::VarStmt;

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

ExprPtr MakeBoolLiteral(bool value) {
  return MakeLiteralExpr(TokenKind::BoolLiteral, value ? "true" : "false");
}

ExprPtr MakeIntLiteral(std::string lexeme) {
  return MakeLiteralExpr(TokenKind::IntLiteral, std::move(lexeme));
}

ExprPtr MakeIdentifierExpr(std::string name) {
  return MakeExpr(cursive0::syntax::IdentifierExpr{std::move(name)});
}

ExprPtr MakeErrorExpr() {
  return MakeExpr(cursive0::syntax::ErrorExpr{});
}

std::shared_ptr<Pattern> MakePattern(PatternNode node) {
  auto pat = std::make_shared<Pattern>();
  pat->node = std::move(node);
  return pat;
}

std::shared_ptr<Pattern> MakeIdentPattern(std::string name) {
  return MakePattern(IdentifierPattern{std::move(name)});
}

std::shared_ptr<Block> MakeBlock(std::vector<Stmt> stmts, ExprPtr tail = nullptr) {
  auto block = std::make_shared<Block>();
  block->stmts = std::move(stmts);
  block->tail_opt = std::move(tail);
  return block;
}

Binding MakeBinding(std::string name, ExprPtr init) {
  Binding binding;
  binding.pat = MakeIdentPattern(std::move(name));
  binding.type_opt = nullptr;
  binding.op = MakeToken(TokenKind::Operator, "=");
  binding.init = std::move(init);
  return binding;
}

LetStmt MakeLetStmt(std::string name, ExprPtr init) {
  LetStmt stmt;
  stmt.binding = MakeBinding(std::move(name), std::move(init));
  return stmt;
}

VarStmt MakeVarStmt(std::string name, ExprPtr init) {
  VarStmt stmt;
  stmt.binding = MakeBinding(std::move(name), std::move(init));
  return stmt;
}

ShadowLetStmt MakeShadowLetStmt(std::string name, ExprPtr init) {
  ShadowLetStmt stmt;
  stmt.name = std::move(name);
  stmt.type_opt = nullptr;
  stmt.init = std::move(init);
  return stmt;
}

ShadowVarStmt MakeShadowVarStmt(std::string name, ExprPtr init) {
  ShadowVarStmt stmt;
  stmt.name = std::move(name);
  stmt.type_opt = nullptr;
  stmt.init = std::move(init);
  return stmt;
}

AssignStmt MakeAssignStmt(ExprPtr place, ExprPtr value) {
  AssignStmt stmt;
  stmt.place = std::move(place);
  stmt.value = std::move(value);
  return stmt;
}

CompoundAssignStmt MakeCompoundAssignStmt(ExprPtr place, std::string op, ExprPtr value) {
  CompoundAssignStmt stmt;
  stmt.place = std::move(place);
  stmt.op = std::move(op);
  stmt.value = std::move(value);
  return stmt;
}

ExprStmt MakeExprStmt(const ExprPtr& value) {
  ExprStmt stmt;
  stmt.value = value;
  return stmt;
}

DeferStmt MakeDeferStmt(std::shared_ptr<Block> body) {
  DeferStmt stmt;
  stmt.body = std::move(body);
  return stmt;
}

RegionStmt MakeRegionStmt(ExprPtr opts, std::shared_ptr<Block> body, std::optional<std::string> alias_opt = std::nullopt) {
  RegionStmt stmt;
  stmt.opts_opt = std::move(opts);
  stmt.alias_opt = std::move(alias_opt);
  stmt.body = std::move(body);
  return stmt;
}

FrameStmt MakeFrameStmt(std::shared_ptr<Block> body, std::optional<std::string> target_opt = std::nullopt) {
  FrameStmt stmt;
  stmt.target_opt = std::move(target_opt);
  stmt.body = std::move(body);
  return stmt;
}

ReturnStmt MakeReturnStmt(ExprPtr value_opt = nullptr) {
  ReturnStmt stmt;
  stmt.value_opt = std::move(value_opt);
  return stmt;
}

ResultStmt MakeResultStmt(ExprPtr value) {
  ResultStmt stmt;
  stmt.value = std::move(value);
  return stmt;
}

BreakStmt MakeBreakStmt(ExprPtr value_opt = nullptr) {
  BreakStmt stmt;
  stmt.value_opt = std::move(value_opt);
  return stmt;
}

ContinueStmt MakeContinueStmt() {
  return ContinueStmt{};
}

UnsafeBlockStmt MakeUnsafeBlockStmt(std::shared_ptr<Block> body) {
  UnsafeBlockStmt stmt;
  stmt.body = std::move(body);
  return stmt;
}

Value MakeIntValue(std::string type, std::uint64_t value) {
  cursive0::semantics::IntVal v;
  v.type = std::move(type);
  v.negative = false;
  v.magnitude = cursive0::core::UInt128FromU64(value);
  return Value{v};
}

Value MakeRegionHandleValue(RegionTarget target) {
  cursive0::semantics::RecordVal record;
  record.record_type = cursive0::sema::MakeTypeModalState({"Region"}, "@Active");
  record.fields = {{"handle", MakeIntValue("usize", target)}};
  return Value{record};
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

SemanticsContext MakeContext(ScopeContext& sema_ctx, ExprTypeMap& expr_types) {
  sema_ctx.expr_types = &expr_types;
  if (sema_ctx.scopes.empty()) {
    sema_ctx.scopes = {cursive0::sema::Scope{}, cursive0::sema::Scope{},
                       cursive0::sema::UniverseBindings()};
  }
  SemanticsContext ctx;
  ctx.sema = &sema_ctx;
  ctx.name_maps = nullptr;
  ctx.module_names = nullptr;
  return ctx;
}

void AddRegionEntry(Sigma& sigma, RegionTarget target, RegionTag tag) {
  RegionEntry entry;
  entry.tag = tag;
  entry.target = target;
  entry.scope = cursive0::semantics::CurrentScopeId(sigma);
  entry.mark = std::nullopt;
  sigma.region_stack.push_back(entry);
}

void TestExecSeq() {
  ScopeContext sema_ctx;
  ExprTypeMap expr_types;
  SemanticsContext ctx = MakeContext(sema_ctx, expr_types);

  {
    Sigma sigma;
    PushScope(sigma, nullptr);
    auto out = ExecSeqSigma(ctx, {}, sigma);
    assert(!std::holds_alternative<Control>(out.node));
  }

  {
    Sigma sigma;
    PushScope(sigma, nullptr);
    auto expr = MakeIntLiteral("1");
    SetExprType(expr_types, expr, cursive0::sema::MakeTypePrim("i32"));
    std::vector<Stmt> stmts;
    stmts.push_back(MakeExprStmt(expr));
    stmts.push_back(MakeExprStmt(expr));
    auto out = ExecSeqSigma(ctx, stmts, sigma);
    assert(!std::holds_alternative<Control>(out.node));
  }

  {
    Sigma sigma;
    PushScope(sigma, nullptr);
    auto expr = MakeIntLiteral("1");
    SetExprType(expr_types, expr, cursive0::sema::MakeTypePrim("i32"));
    std::vector<Stmt> stmts;
    stmts.push_back(MakeExprStmt(expr));
    stmts.push_back(MakeBreakStmt());
    auto out = ExecSeqSigma(ctx, stmts, sigma);
    assert(std::holds_alternative<Control>(out.node));
  }
}

void TestExecBindings() {
  ScopeContext sema_ctx;
  ExprTypeMap expr_types;
  SemanticsContext ctx = MakeContext(sema_ctx, expr_types);

  {
    Sigma sigma;
    PushScope(sigma, nullptr);
    auto init = MakeIntLiteral("4");
    SetExprType(expr_types, init, cursive0::sema::MakeTypePrim("i32"));
    auto out = ExecSigma(ctx, MakeLetStmt("x", init), sigma);
    assert(!std::holds_alternative<Control>(out.node));
  }

  {
    Sigma sigma;
    PushScope(sigma, nullptr);
    auto out = ExecSigma(ctx, MakeLetStmt("x", MakeErrorExpr()), sigma);
    assert(std::holds_alternative<Control>(out.node));
  }

  {
    Sigma sigma;
    PushScope(sigma, nullptr);
    auto init = MakeIntLiteral("5");
    SetExprType(expr_types, init, cursive0::sema::MakeTypePrim("i32"));
    auto out = ExecSigma(ctx, MakeVarStmt("y", init), sigma);
    assert(!std::holds_alternative<Control>(out.node));
  }

  {
    Sigma sigma;
    PushScope(sigma, nullptr);
    auto out = ExecSigma(ctx, MakeVarStmt("y", MakeErrorExpr()), sigma);
    assert(std::holds_alternative<Control>(out.node));
  }

  {
    Sigma sigma;
    PushScope(sigma, nullptr);
    auto init = MakeIntLiteral("6");
    SetExprType(expr_types, init, cursive0::sema::MakeTypePrim("i32"));
    auto out = ExecSigma(ctx, MakeShadowLetStmt("s", init), sigma);
    assert(!std::holds_alternative<Control>(out.node));
  }

  {
    Sigma sigma;
    PushScope(sigma, nullptr);
    auto out = ExecSigma(ctx, MakeShadowLetStmt("s", MakeErrorExpr()), sigma);
    assert(std::holds_alternative<Control>(out.node));
  }

  {
    Sigma sigma;
    PushScope(sigma, nullptr);
    auto init = MakeIntLiteral("7");
    SetExprType(expr_types, init, cursive0::sema::MakeTypePrim("i32"));
    auto out = ExecSigma(ctx, MakeShadowVarStmt("v", init), sigma);
    assert(!std::holds_alternative<Control>(out.node));
  }

  {
    Sigma sigma;
    PushScope(sigma, nullptr);
    auto out = ExecSigma(ctx, MakeShadowVarStmt("v", MakeErrorExpr()), sigma);
    assert(std::holds_alternative<Control>(out.node));
  }
}

void TestExecAssign() {
  ScopeContext sema_ctx;
  ExprTypeMap expr_types;
  SemanticsContext ctx = MakeContext(sema_ctx, expr_types);

  {
    Sigma sigma;
    PushScope(sigma, nullptr);
    BindValue(sigma, "x", MakeIntValue("i32", 1));
    auto val = MakeIntLiteral("2");
    SetExprType(expr_types, val, cursive0::sema::MakeTypePrim("i32"));
    AssignStmt stmt = MakeAssignStmt(MakeIdentifierExpr("x"), val);
    auto out = ExecSigma(ctx, stmt, sigma);
    assert(!std::holds_alternative<Control>(out.node));
  }

  {
    Sigma sigma;
    PushScope(sigma, nullptr);
    BindValue(sigma, "x", MakeIntValue("i32", 1));
    AssignStmt stmt = MakeAssignStmt(MakeIdentifierExpr("x"), MakeErrorExpr());
    auto out = ExecSigma(ctx, stmt, sigma);
    assert(std::holds_alternative<Control>(out.node));
  }

  {
    Sigma sigma;
    PushScope(sigma, nullptr);
    BindValue(sigma, "x", MakeIntValue("i32", 2));
    auto rhs = MakeIntLiteral("3");
    SetExprType(expr_types, rhs, cursive0::sema::MakeTypePrim("i32"));
    CompoundAssignStmt stmt = MakeCompoundAssignStmt(MakeIdentifierExpr("x"), "+=", rhs);
    auto out = ExecSigma(ctx, stmt, sigma);
    assert(!std::holds_alternative<Control>(out.node));
  }

  {
    Sigma sigma;
    PushScope(sigma, nullptr);
    auto rhs = MakeIntLiteral("3");
    SetExprType(expr_types, rhs, cursive0::sema::MakeTypePrim("i32"));
    CompoundAssignStmt stmt = MakeCompoundAssignStmt(MakeErrorExpr(), "+=", rhs);
    auto out = ExecSigma(ctx, stmt, sigma);
    assert(std::holds_alternative<Control>(out.node));
  }

  {
    Sigma sigma;
    PushScope(sigma, nullptr);
    BindValue(sigma, "x", MakeIntValue("i32", 2));
    CompoundAssignStmt stmt = MakeCompoundAssignStmt(MakeIdentifierExpr("x"), "+=", MakeErrorExpr());
    auto out = ExecSigma(ctx, stmt, sigma);
    assert(std::holds_alternative<Control>(out.node));
  }
}

void TestExecOtherStmts() {
  ScopeContext sema_ctx;
  ExprTypeMap expr_types;
  SemanticsContext ctx = MakeContext(sema_ctx, expr_types);

  {
    Sigma sigma;
    PushScope(sigma, nullptr);
    auto expr = MakeIntLiteral("1");
    SetExprType(expr_types, expr, cursive0::sema::MakeTypePrim("i32"));
    auto out = ExecSigma(ctx, MakeExprStmt(expr), sigma);
    assert(!std::holds_alternative<Control>(out.node));
  }

  {
    Sigma sigma;
    PushScope(sigma, nullptr);
    auto body = MakeBlock({});
    auto out = ExecSigma(ctx, MakeUnsafeBlockStmt(body), sigma);
    assert(!std::holds_alternative<Control>(out.node));
  }

  {
    Sigma sigma;
    PushScope(sigma, nullptr);
    auto body = MakeBlock({});
    auto out = ExecSigma(ctx, MakeDeferStmt(body), sigma);
    assert(!std::holds_alternative<Control>(out.node));
  }

  {
    Sigma sigma;
    PushScope(sigma, nullptr);
    auto opts = MakeIntLiteral("1");
    SetExprType(expr_types, opts, cursive0::sema::MakeTypePrim("i32"));
    auto body = MakeBlock({});
    auto out = ExecSigma(ctx, MakeRegionStmt(opts, body, std::nullopt), sigma);
    assert(!std::holds_alternative<Control>(out.node));
  }

  {
    Sigma sigma;
    PushScope(sigma, nullptr);
    auto out = ExecSigma(ctx, MakeRegionStmt(MakeErrorExpr(), MakeBlock({}), std::nullopt), sigma);
    assert(std::holds_alternative<Control>(out.node));
  }

  {
    Sigma sigma;
    PushScope(sigma, nullptr);
    const RegionTarget target = 1;
    AddRegionEntry(sigma, target, 1);
    BindValue(sigma, "r", MakeRegionHandleValue(target));
    auto body = MakeBlock({});
    auto out = ExecSigma(ctx, MakeFrameStmt(body, std::optional<std::string>("r")), sigma);
    assert(!std::holds_alternative<Control>(out.node));
  }

  {
    Sigma sigma;
    PushScope(sigma, nullptr);
    const RegionTarget target = 2;
    AddRegionEntry(sigma, target, 2);
    auto body = MakeBlock({cursive0::syntax::ErrorStmt{}});
    auto out = ExecSigma(ctx, MakeFrameStmt(body, std::nullopt), sigma);
    assert(std::holds_alternative<Control>(out.node));
  }

  {
    Sigma sigma;
    PushScope(sigma, nullptr);
    auto out = ExecSigma(ctx, MakeReturnStmt(nullptr), sigma);
    assert(std::holds_alternative<Control>(out.node));
  }

  {
    Sigma sigma;
    PushScope(sigma, nullptr);
    auto value = MakeIntLiteral("2");
    SetExprType(expr_types, value, cursive0::sema::MakeTypePrim("i32"));
    auto out = ExecSigma(ctx, MakeReturnStmt(value), sigma);
    assert(std::holds_alternative<Control>(out.node));
  }

  {
    Sigma sigma;
    PushScope(sigma, nullptr);
    auto out = ExecSigma(ctx, MakeReturnStmt(MakeErrorExpr()), sigma);
    assert(std::holds_alternative<Control>(out.node));
  }

  {
    Sigma sigma;
    PushScope(sigma, nullptr);
    auto value = MakeIntLiteral("3");
    SetExprType(expr_types, value, cursive0::sema::MakeTypePrim("i32"));
    auto out = ExecSigma(ctx, MakeResultStmt(value), sigma);
    assert(std::holds_alternative<Control>(out.node));
  }

  {
    Sigma sigma;
    PushScope(sigma, nullptr);
    auto out = ExecSigma(ctx, MakeResultStmt(MakeErrorExpr()), sigma);
    assert(std::holds_alternative<Control>(out.node));
  }

  {
    Sigma sigma;
    PushScope(sigma, nullptr);
    auto out = ExecSigma(ctx, MakeBreakStmt(nullptr), sigma);
    assert(std::holds_alternative<Control>(out.node));
  }

  {
    Sigma sigma;
    PushScope(sigma, nullptr);
    auto value = MakeIntLiteral("4");
    SetExprType(expr_types, value, cursive0::sema::MakeTypePrim("i32"));
    auto out = ExecSigma(ctx, MakeBreakStmt(value), sigma);
    assert(std::holds_alternative<Control>(out.node));
  }

  {
    Sigma sigma;
    PushScope(sigma, nullptr);
    auto out = ExecSigma(ctx, MakeBreakStmt(MakeErrorExpr()), sigma);
    assert(std::holds_alternative<Control>(out.node));
  }

  {
    Sigma sigma;
    PushScope(sigma, nullptr);
    auto out = ExecSigma(ctx, MakeContinueStmt(), sigma);
    assert(std::holds_alternative<Control>(out.node));
  }

  {
    Sigma sigma;
    PushScope(sigma, nullptr);
    auto out = ExecSigma(ctx, ErrorStmt{}, sigma);
    assert(std::holds_alternative<Control>(out.node));
  }
}

}  // namespace

int main() {
  SPEC_COV("ExecSeq-Cons-Ctrl");
  SPEC_COV("ExecSeq-Cons-Ok");
  SPEC_COV("ExecSeq-Empty");
  SPEC_COV("ExecSigma-Assign-Ctrl");
  SPEC_COV("ExecSigma-Break-Ctrl");
  SPEC_COV("ExecSigma-Break-Unit");
  SPEC_COV("ExecSigma-CompoundAssign");
  SPEC_COV("ExecSigma-CompoundAssign-Left-Ctrl");
  SPEC_COV("ExecSigma-CompoundAssign-Right-Ctrl");
  SPEC_COV("ExecSigma-Defer");
  SPEC_COV("ExecSigma-Error");
  SPEC_COV("ExecSigma-ExprStmt");
  SPEC_COV("ExecSigma-Frame-Explicit");
  SPEC_COV("ExecSigma-Frame-Implicit");
  SPEC_COV("ExecSigma-Let");
  SPEC_COV("ExecSigma-Let-Ctrl");
  SPEC_COV("ExecSigma-Region");
  SPEC_COV("ExecSigma-Region-Ctrl");
  SPEC_COV("ExecSigma-Result");
  SPEC_COV("ExecSigma-Result-Ctrl");
  SPEC_COV("ExecSigma-Return");
  SPEC_COV("ExecSigma-Return-Ctrl");
  SPEC_COV("ExecSigma-Return-Unit");
  SPEC_COV("ExecSigma-ShadowLet");
  SPEC_COV("ExecSigma-ShadowLet-Ctrl");
  SPEC_COV("ExecSigma-ShadowVar");
  SPEC_COV("ExecSigma-ShadowVar-Ctrl");
  SPEC_COV("ExecSigma-UnsafeStmt");
  SPEC_COV("ExecSigma-Var");
  SPEC_COV("ExecSigma-Var-Ctrl");
  SPEC_COV("Step-Exec-Defer");
  SPEC_COV("Step-Exec-Frame-Body");
  SPEC_COV("Step-Exec-Frame-Enter-Explicit");
  SPEC_COV("Step-Exec-Frame-Enter-Implicit");
  SPEC_COV("Step-Exec-Frame-Exit-Ctrl");
  SPEC_COV("Step-Exec-Frame-Exit-Ok");
  SPEC_COV("Step-Exec-Other-Ctrl");
  SPEC_COV("Step-Exec-Other-Ok");
  SPEC_COV("Step-Exec-Region-Body");
  SPEC_COV("Step-Exec-Region-Enter");
  SPEC_COV("Step-Exec-Region-Enter-Ctrl");
  SPEC_COV("Step-Exec-Region-Exit-Ctrl");
  SPEC_COV("Step-Exec-Region-Exit-Ok");
  SPEC_COV("Step-ExecSeq-Ctrl");
  SPEC_COV("Step-ExecSeq-Ok");

  TestExecSeq();
  TestExecBindings();
  TestExecAssign();
  TestExecOtherStmts();
  return 0;
}
