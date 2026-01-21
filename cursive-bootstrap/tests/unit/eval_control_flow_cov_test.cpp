#include <cassert>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/analysis/types/context.h"
#include "cursive0/analysis/resolve/scopes.h"
#include "cursive0/analysis/types/types.h"
#include "cursive0/eval/eval.h"
#include "cursive0/eval/state.h"
#include "cursive0/eval/value.h"
#include "cursive0/syntax/ast.h"
#include "cursive0/syntax/token.h"

namespace {

using cursive0::analysis::ExprTypeMap;
using cursive0::analysis::ScopeContext;
using cursive0::analysis::TypeRef;
using cursive0::eval::BindInfo;
using cursive0::eval::BindVal;
using cursive0::eval::Control;
using cursive0::eval::ControlKind;
using cursive0::eval::EvalSigma;
using cursive0::eval::IsCtrl;
using cursive0::eval::IsVal;
using cursive0::eval::MakeCtrl;
using cursive0::eval::MakeVal;
using cursive0::eval::Movability;
using cursive0::eval::Outcome;
using cursive0::eval::PushScope;
using cursive0::eval::RegionEntry;
using cursive0::eval::RegionTag;
using cursive0::eval::RegionTarget;
using cursive0::eval::Responsibility;
using cursive0::eval::SemanticsContext;
using cursive0::eval::Sigma;
using cursive0::eval::Value;
using cursive0::eval::ValueEqual;

using cursive0::syntax::Arg;
using cursive0::syntax::AssignStmt;
using cursive0::syntax::Block;
using cursive0::syntax::BreakStmt;
using cursive0::syntax::ContinueStmt;
using cursive0::syntax::Expr;
using cursive0::syntax::ExprNode;
using cursive0::syntax::ExprPtr;
using cursive0::syntax::IdentifierPattern;
using cursive0::syntax::IfExpr;
using cursive0::syntax::LiteralExpr;
using cursive0::syntax::LoopConditionalExpr;
using cursive0::syntax::LoopInfiniteExpr;
using cursive0::syntax::LoopIterExpr;
using cursive0::syntax::MatchArm;
using cursive0::syntax::MatchExpr;
using cursive0::syntax::Pattern;
using cursive0::syntax::PatternNode;
using cursive0::syntax::Stmt;
using cursive0::syntax::Token;
using cursive0::syntax::TokenKind;
using cursive0::syntax::UnaryExpr;

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
  LiteralExpr lit;
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

ExprPtr MakeUnaryExpr(std::string op, ExprPtr value) {
  UnaryExpr node;
  node.op = std::move(op);
  node.value = std::move(value);
  return MakeExpr(std::move(node));
}

ExprPtr MakeIfExpr(const ExprPtr& cond, const ExprPtr& then_expr, const ExprPtr& else_expr) {
  IfExpr node;
  node.cond = cond;
  node.then_expr = then_expr;
  node.else_expr = else_expr;
  return MakeExpr(std::move(node));
}

std::shared_ptr<Pattern> MakePattern(PatternNode node) {
  auto pat = std::make_shared<Pattern>();
  pat->node = std::move(node);
  return pat;
}

std::shared_ptr<Pattern> MakeWildcardPattern() {
  return MakePattern(cursive0::syntax::WildcardPattern{});
}

std::shared_ptr<Pattern> MakeLiteralPatternBool(bool value) {
  cursive0::syntax::LiteralPattern pat;
  pat.literal = MakeToken(TokenKind::BoolLiteral, value ? "true" : "false");
  return MakePattern(std::move(pat));
}

std::shared_ptr<Pattern> MakeLiteralPatternInt(std::string lexeme) {
  cursive0::syntax::LiteralPattern pat;
  pat.literal = MakeToken(TokenKind::IntLiteral, std::move(lexeme));
  return MakePattern(std::move(pat));
}

std::shared_ptr<Pattern> MakeIdentPattern(std::string name) {
  return MakePattern(IdentifierPattern{std::move(name)});
}

MatchArm MakeMatchArm(std::shared_ptr<Pattern> pattern,
                      ExprPtr guard_opt,
                      ExprPtr body) {
  MatchArm arm;
  arm.pattern = std::move(pattern);
  arm.guard_opt = std::move(guard_opt);
  arm.body = std::move(body);
  return arm;
}

ExprPtr MakeMatchExpr(ExprPtr value, std::vector<MatchArm> arms) {
  MatchExpr node;
  node.value = std::move(value);
  node.arms = std::move(arms);
  return MakeExpr(std::move(node));
}

std::shared_ptr<Block> MakeBlock(std::vector<Stmt> stmts, ExprPtr tail = nullptr) {
  auto block = std::make_shared<Block>();
  block->stmts = std::move(stmts);
  block->tail_opt = std::move(tail);
  return block;
}

ExprPtr MakeBlockExpr(std::shared_ptr<Block> block) {
  cursive0::syntax::BlockExpr node;
  node.block = std::move(block);
  return MakeExpr(std::move(node));
}

ExprPtr MakeUnsafeBlockExpr(std::shared_ptr<Block> block) {
  cursive0::syntax::UnsafeBlockExpr node;
  node.block = std::move(block);
  return MakeExpr(std::move(node));
}

Stmt MakeAssignStmt(ExprPtr place, ExprPtr value) {
  AssignStmt stmt;
  stmt.place = std::move(place);
  stmt.value = std::move(value);
  return stmt;
}

Stmt MakeContinueStmt() {
  return ContinueStmt{};
}

Stmt MakeBreakStmt(ExprPtr value_opt = nullptr) {
  BreakStmt stmt;
  stmt.value_opt = std::move(value_opt);
  return stmt;
}

Stmt MakeExprStmt(ExprPtr value) {
  cursive0::syntax::ExprStmt stmt;
  stmt.value = std::move(value);
  return stmt;
}

ExprPtr MakeLoopInfiniteExpr(std::shared_ptr<Block> body) {
  LoopInfiniteExpr node;
  node.body = std::move(body);
  return MakeExpr(std::move(node));
}

ExprPtr MakeLoopConditionalExpr(ExprPtr cond, std::shared_ptr<Block> body) {
  LoopConditionalExpr node;
  node.cond = std::move(cond);
  node.body = std::move(body);
  return MakeExpr(std::move(node));
}

ExprPtr MakeLoopIterExpr(std::shared_ptr<Pattern> pattern,
                         ExprPtr iter,
                         std::shared_ptr<Block> body) {
  LoopIterExpr node;
  node.pattern = std::move(pattern);
  node.type_opt = nullptr;
  node.iter = std::move(iter);
  node.body = std::move(body);
  return MakeExpr(std::move(node));
}

ExprPtr MakeMethodCallExpr(ExprPtr receiver,
                           std::string name,
                           std::vector<Arg> args) {
  cursive0::syntax::MethodCallExpr node;
  node.receiver = std::move(receiver);
  node.name = std::move(name);
  node.args = std::move(args);
  return MakeExpr(std::move(node));
}

ExprPtr MakeAllocExpr(std::optional<std::string> region_opt, ExprPtr value) {
  cursive0::syntax::AllocExpr node;
  if (region_opt.has_value()) {
    node.region_opt = *region_opt;
  }
  node.value = std::move(value);
  return MakeExpr(std::move(node));
}

void SetExprType(ExprTypeMap& map, const ExprPtr& expr, TypeRef type) {
  map[expr.get()] = std::move(type);
}

Value MakeBoolValue(bool value) {
  cursive0::eval::BoolVal v;
  v.value = value;
  return Value{v};
}

Value MakeIntValue(std::string type, std::uint64_t value) {
  cursive0::eval::IntVal v;
  v.type = std::move(type);
  v.negative = false;
  v.magnitude = cursive0::core::UInt128FromU64(value);
  return Value{v};
}

Value MakeStringValue(std::string bytes) {
  cursive0::eval::StringVal v;
  v.state = cursive0::analysis::StringState::View;
  v.bytes.assign(bytes.begin(), bytes.end());
  return Value{v};
}

Value MakeRegionHandleValue(RegionTarget target) {
  cursive0::eval::RecordVal record;
  record.record_type = cursive0::analysis::MakeTypeModalState({"Region"}, "@Active");
  record.fields = {{"handle", MakeIntValue("usize", target)}};
  return Value{record};
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
    sema_ctx.scopes = {cursive0::analysis::Scope{}, cursive0::analysis::Scope{},
                       cursive0::analysis::UniverseBindings()};
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
  entry.scope = cursive0::eval::CurrentScopeId(sigma);
  entry.mark = std::nullopt;
  sigma.region_stack.push_back(entry);
}

void TestIfExprs() {
  ScopeContext sema_ctx;
  ExprTypeMap expr_types;
  SemanticsContext ctx = MakeContext(sema_ctx, expr_types);

  Sigma sigma;
  PushScope(sigma, nullptr);

  auto cond_true = MakeBoolLiteral(true);
  auto cond_false = MakeBoolLiteral(false);
  auto value_one = MakeIntLiteral("1");
  auto value_two = MakeIntLiteral("2");

  SetExprType(expr_types, cond_true, cursive0::analysis::MakeTypePrim("bool"));
  SetExprType(expr_types, cond_false, cursive0::analysis::MakeTypePrim("bool"));
  SetExprType(expr_types, value_one, cursive0::analysis::MakeTypePrim("i32"));
  SetExprType(expr_types, value_two, cursive0::analysis::MakeTypePrim("i32"));

  auto if_true = MakeIfExpr(cond_true, value_one, value_two);
  Outcome out_true = EvalSigma(ctx, *if_true, sigma);
  assert(IsVal(out_true));

  auto if_false_none = MakeIfExpr(cond_false, value_one, nullptr);
  Outcome out_false_none = EvalSigma(ctx, *if_false_none, sigma);
  assert(IsVal(out_false_none));

  auto if_false_some = MakeIfExpr(cond_false, value_one, value_two);
  Outcome out_false_some = EvalSigma(ctx, *if_false_some, sigma);
  assert(IsVal(out_false_some));

  auto if_ctrl = MakeIfExpr(MakeErrorExpr(), value_one, value_two);
  Outcome out_ctrl = EvalSigma(ctx, *if_ctrl, sigma);
  assert(IsCtrl(out_ctrl));
}

void TestMatchExprs() {
  ScopeContext sema_ctx;
  ExprTypeMap expr_types;
  SemanticsContext ctx = MakeContext(sema_ctx, expr_types);

  Sigma sigma;
  PushScope(sigma, nullptr);
  BindValue(sigma, "v", MakeIntValue("i32", 1));

  auto scrutinee = MakeIdentifierExpr("v");
  auto zero = MakeIntLiteral("0");
  auto one = MakeIntLiteral("1");
  auto two = MakeIntLiteral("2");
  SetExprType(expr_types, zero, cursive0::analysis::MakeTypePrim("i32"));
  SetExprType(expr_types, one, cursive0::analysis::MakeTypePrim("i32"));
  SetExprType(expr_types, two, cursive0::analysis::MakeTypePrim("i32"));

  std::vector<MatchArm> arms_fail_then_match;
  arms_fail_then_match.push_back(MakeMatchArm(MakeLiteralPatternInt("2"), nullptr, zero));
  arms_fail_then_match.push_back(MakeMatchArm(MakeWildcardPattern(), nullptr, one));
  auto match_expr = MakeMatchExpr(scrutinee, arms_fail_then_match);
  Outcome out_match = EvalSigma(ctx, *match_expr, sigma);
  assert(IsVal(out_match));

  auto guard_true = MakeBoolLiteral(true);
  auto guard_false = MakeBoolLiteral(false);
  SetExprType(expr_types, guard_true, cursive0::analysis::MakeTypePrim("bool"));
  SetExprType(expr_types, guard_false, cursive0::analysis::MakeTypePrim("bool"));

  std::vector<MatchArm> arms_guard_true;
  arms_guard_true.push_back(MakeMatchArm(MakeWildcardPattern(), guard_true, two));
  auto match_guard_true_scrut = MakeIntLiteral("1");
  SetExprType(expr_types, match_guard_true_scrut, cursive0::analysis::MakeTypePrim("i32"));
  auto match_guard_true = MakeMatchExpr(match_guard_true_scrut, arms_guard_true);
  Outcome out_guard_true = EvalSigma(ctx, *match_guard_true, sigma);
  assert(IsVal(out_guard_true));

  std::vector<MatchArm> arms_guard_false;
  arms_guard_false.push_back(MakeMatchArm(MakeWildcardPattern(), guard_false, two));
  arms_guard_false.push_back(MakeMatchArm(MakeWildcardPattern(), nullptr, one));
  auto match_guard_false_scrut = MakeIntLiteral("1");
  SetExprType(expr_types, match_guard_false_scrut, cursive0::analysis::MakeTypePrim("i32"));
  auto match_guard_false = MakeMatchExpr(match_guard_false_scrut, arms_guard_false);
  Outcome out_guard_false = EvalSigma(ctx, *match_guard_false, sigma);
  assert(IsVal(out_guard_false));

  std::vector<MatchArm> arms_guard_ctrl;
  arms_guard_ctrl.push_back(MakeMatchArm(MakeWildcardPattern(), MakeErrorExpr(), two));
  auto match_guard_ctrl_scrut = MakeIntLiteral("1");
  SetExprType(expr_types, match_guard_ctrl_scrut, cursive0::analysis::MakeTypePrim("i32"));
  auto match_guard_ctrl = MakeMatchExpr(match_guard_ctrl_scrut, arms_guard_ctrl);
  Outcome out_guard_ctrl = EvalSigma(ctx, *match_guard_ctrl, sigma);
  assert(IsCtrl(out_guard_ctrl));

  auto match_ctrl = MakeMatchExpr(MakeErrorExpr(), arms_fail_then_match);
  Outcome out_match_ctrl = EvalSigma(ctx, *match_ctrl, sigma);
  assert(IsCtrl(out_match_ctrl));
}

void TestLoopConditional() {
  ScopeContext sema_ctx;
  ExprTypeMap expr_types;
  SemanticsContext ctx = MakeContext(sema_ctx, expr_types);

  {
    Sigma sigma;
    PushScope(sigma, nullptr);
    BindValue(sigma, "flag", MakeBoolValue(false));
    auto cond = MakeIdentifierExpr("flag");
    auto body = MakeBlock({});
    auto loop = MakeLoopConditionalExpr(cond, body);
    Outcome out = EvalSigma(ctx, *loop, sigma);
    assert(IsVal(out));
  }

  {
    Sigma sigma;
    PushScope(sigma, nullptr);
    auto loop = MakeLoopConditionalExpr(MakeErrorExpr(), MakeBlock({}));
    Outcome out = EvalSigma(ctx, *loop, sigma);
    assert(IsCtrl(out));
  }

  {
    Sigma sigma;
    PushScope(sigma, nullptr);
    BindValue(sigma, "flag", MakeBoolValue(true));

    auto false_expr = MakeBoolLiteral(false);
    SetExprType(expr_types, false_expr, cursive0::analysis::MakeTypePrim("bool"));
    auto assign_false = MakeAssignStmt(MakeIdentifierExpr("flag"), false_expr);
    auto body = MakeBlock({assign_false});
    auto loop = MakeLoopConditionalExpr(MakeIdentifierExpr("flag"), body);
    Outcome out = EvalSigma(ctx, *loop, sigma);
    assert(IsVal(out));
  }

  {
    Sigma sigma;
    PushScope(sigma, nullptr);
    BindValue(sigma, "flag", MakeBoolValue(true));
    auto false_expr = MakeBoolLiteral(false);
    SetExprType(expr_types, false_expr, cursive0::analysis::MakeTypePrim("bool"));
    auto assign_false = MakeAssignStmt(MakeIdentifierExpr("flag"), false_expr);
    auto cont_stmt = MakeContinueStmt();
    auto body = MakeBlock({assign_false, cont_stmt});
    auto loop = MakeLoopConditionalExpr(MakeIdentifierExpr("flag"), body);
    Outcome out = EvalSigma(ctx, *loop, sigma);
    assert(IsVal(out));
  }

  {
    Sigma sigma;
    PushScope(sigma, nullptr);
    BindValue(sigma, "flag", MakeBoolValue(true));
    auto break_val = MakeIntLiteral("9");
    SetExprType(expr_types, break_val, cursive0::analysis::MakeTypePrim("i32"));
    auto break_stmt = MakeBreakStmt(break_val);
    auto body = MakeBlock({break_stmt});
    auto loop = MakeLoopConditionalExpr(MakeIdentifierExpr("flag"), body);
    Outcome out = EvalSigma(ctx, *loop, sigma);
    assert(IsVal(out));
  }

  {
    Sigma sigma;
    PushScope(sigma, nullptr);
    BindValue(sigma, "flag", MakeBoolValue(true));
    auto body = MakeBlock({cursive0::syntax::ErrorStmt{}});
    auto loop = MakeLoopConditionalExpr(MakeIdentifierExpr("flag"), body);
    Outcome out = EvalSigma(ctx, *loop, sigma);
    assert(IsCtrl(out));
  }
}

void TestLoopInfinite() {
  ScopeContext sema_ctx;
  ExprTypeMap expr_types;
  SemanticsContext ctx = MakeContext(sema_ctx, expr_types);

  {
    Sigma sigma;
    PushScope(sigma, nullptr);
    auto body = MakeBlock({MakeBreakStmt()});
    auto loop = MakeLoopInfiniteExpr(body);
    Outcome out = EvalSigma(ctx, *loop, sigma);
    assert(IsVal(out));
  }

  {
    Sigma sigma;
    PushScope(sigma, nullptr);
    auto body = MakeBlock({cursive0::syntax::ErrorStmt{}});
    auto loop = MakeLoopInfiniteExpr(body);
    Outcome out = EvalSigma(ctx, *loop, sigma);
    assert(IsCtrl(out));
  }

  {
    Sigma sigma;
    PushScope(sigma, nullptr);
    BindValue(sigma, "flag", MakeBoolValue(true));

    auto false_expr = MakeBoolLiteral(false);
    SetExprType(expr_types, false_expr, cursive0::analysis::MakeTypePrim("bool"));
    auto assign_false = MakeAssignStmt(MakeIdentifierExpr("flag"), false_expr);
    auto cont_block = MakeBlock({assign_false, MakeContinueStmt()});
    auto break_block = MakeBlock({MakeBreakStmt()});

    std::vector<MatchArm> arms;
    arms.push_back(MakeMatchArm(MakeLiteralPatternBool(true), nullptr, MakeBlockExpr(cont_block)));
    arms.push_back(MakeMatchArm(MakeLiteralPatternBool(false), nullptr, MakeBlockExpr(break_block)));

    auto match_expr = MakeMatchExpr(MakeIdentifierExpr("flag"), arms);
    auto body = MakeBlock({MakeExprStmt(match_expr)});
    auto loop = MakeLoopInfiniteExpr(body);
    Outcome out = EvalSigma(ctx, *loop, sigma);
    assert(IsVal(out));
  }

  {
    Sigma sigma;
    PushScope(sigma, nullptr);
    BindValue(sigma, "flag", MakeBoolValue(true));

    auto false_expr = MakeBoolLiteral(false);
    SetExprType(expr_types, false_expr, cursive0::analysis::MakeTypePrim("bool"));
    auto assign_false = MakeAssignStmt(MakeIdentifierExpr("flag"), false_expr);
    auto ok_block = MakeBlock({assign_false});
    auto break_block = MakeBlock({MakeBreakStmt()});

    std::vector<MatchArm> arms;
    arms.push_back(MakeMatchArm(MakeLiteralPatternBool(true), nullptr, MakeBlockExpr(ok_block)));
    arms.push_back(MakeMatchArm(MakeLiteralPatternBool(false), nullptr, MakeBlockExpr(break_block)));

    auto match_expr = MakeMatchExpr(MakeIdentifierExpr("flag"), arms);
    auto body = MakeBlock({MakeExprStmt(match_expr)});
    auto loop = MakeLoopInfiniteExpr(body);
    Outcome out = EvalSigma(ctx, *loop, sigma);
    assert(IsVal(out));
  }
}

void TestLoopIter() {
  ScopeContext sema_ctx;
  ExprTypeMap expr_types;
  SemanticsContext ctx = MakeContext(sema_ctx, expr_types);

  {
    Sigma sigma;
    PushScope(sigma, nullptr);
    cursive0::eval::ArrayVal array;
    BindValue(sigma, "arr", Value{array});
    auto loop = MakeLoopIterExpr(MakeWildcardPattern(), MakeIdentifierExpr("arr"), MakeBlock({}));
    Outcome out = EvalSigma(ctx, *loop, sigma);
    assert(IsVal(out));
  }

  {
    Sigma sigma;
    PushScope(sigma, nullptr);
    cursive0::eval::ArrayVal array;
    array.elements.push_back(MakeIntValue("i32", 1));
    BindValue(sigma, "arr", Value{array});

    auto body = MakeBlock({});
    auto loop = MakeLoopIterExpr(MakeWildcardPattern(), MakeIdentifierExpr("arr"), body);
    Outcome out = EvalSigma(ctx, *loop, sigma);
    assert(IsVal(out));
  }

  {
    Sigma sigma;
    PushScope(sigma, nullptr);
    cursive0::eval::ArrayVal array;
    array.elements.push_back(MakeIntValue("i32", 1));
    BindValue(sigma, "arr", Value{array});

    auto body = MakeBlock({MakeContinueStmt()});
    auto loop = MakeLoopIterExpr(MakeWildcardPattern(), MakeIdentifierExpr("arr"), body);
    Outcome out = EvalSigma(ctx, *loop, sigma);
    assert(IsVal(out));
  }

  {
    Sigma sigma;
    PushScope(sigma, nullptr);
    cursive0::eval::ArrayVal array;
    array.elements.push_back(MakeIntValue("i32", 1));
    BindValue(sigma, "arr", Value{array});

    auto body = MakeBlock({MakeBreakStmt()});
    auto loop = MakeLoopIterExpr(MakeWildcardPattern(), MakeIdentifierExpr("arr"), body);
    Outcome out = EvalSigma(ctx, *loop, sigma);
    assert(IsVal(out));
  }

  {
    Sigma sigma;
    PushScope(sigma, nullptr);
    cursive0::eval::ArrayVal array;
    array.elements.push_back(MakeIntValue("i32", 1));
    BindValue(sigma, "arr", Value{array});

    auto body = MakeBlock({cursive0::syntax::ErrorStmt{}});
    auto loop = MakeLoopIterExpr(MakeWildcardPattern(), MakeIdentifierExpr("arr"), body);
    Outcome out = EvalSigma(ctx, *loop, sigma);
    assert(IsCtrl(out));
  }

  {
    Sigma sigma;
    PushScope(sigma, nullptr);
    auto loop = MakeLoopIterExpr(MakeWildcardPattern(), MakeErrorExpr(), MakeBlock({}));
    Outcome out = EvalSigma(ctx, *loop, sigma);
    assert(IsCtrl(out));
  }
}

void TestMethodCall() {
  ScopeContext sema_ctx;
  ExprTypeMap expr_types;
  SemanticsContext ctx = MakeContext(sema_ctx, expr_types);

  Sigma sigma;
  PushScope(sigma, nullptr);

  const auto scope_id = cursive0::eval::CurrentScopeId(sigma);
  const auto fs_addr = cursive0::eval::AllocAddr(sigma);
  cursive0::eval::WriteAddr(sigma, fs_addr, Value{cursive0::eval::UnitVal{}});
  sigma.addr_tags[fs_addr] = cursive0::eval::RuntimeTag{cursive0::eval::RuntimeTagKind::ScopeTag, scope_id};
  sigma.fs_handles[fs_addr] = cursive0::eval::FileSystemHandle{std::nullopt, ""};

  cursive0::eval::RawPtrVal raw;
  raw.qual = cursive0::analysis::RawPtrQual::Imm;
  raw.addr = fs_addr;
  cursive0::eval::DynamicVal dyn;
  dyn.class_path = {"FileSystem"};
  dyn.data = raw;
  dyn.concrete_type = nullptr;

  BindValue(sigma, "fs", Value{dyn});
  BindValue(sigma, "path", MakeStringValue("foo"));

  Arg arg_ok;
  arg_ok.value = MakeIdentifierExpr("path");
  auto call_ok = MakeMethodCallExpr(MakeIdentifierExpr("fs"), "exists", {arg_ok});
  Outcome out_ok = EvalSigma(ctx, *call_ok, sigma);
  assert(IsVal(out_ok));

  Arg arg_ctrl;
  arg_ctrl.value = MakeErrorExpr();
  auto call_ctrl_args = MakeMethodCallExpr(MakeIdentifierExpr("fs"), "exists", {arg_ctrl});
  Outcome out_ctrl_args = EvalSigma(ctx, *call_ctrl_args, sigma);
  assert(IsCtrl(out_ctrl_args));

  auto call_ctrl_recv = MakeMethodCallExpr(MakeErrorExpr(), "exists", {arg_ok});
  Outcome out_ctrl_recv = EvalSigma(ctx, *call_ctrl_recv, sigma);
  assert(IsCtrl(out_ctrl_recv));
}

void TestStepSigmaNodes() {
  ScopeContext sema_ctx;
  ExprTypeMap expr_types;
  SemanticsContext ctx = MakeContext(sema_ctx, expr_types);

  Sigma sigma;
  PushScope(sigma, nullptr);

  auto lit = MakeIntLiteral("4");
  SetExprType(expr_types, lit, cursive0::analysis::MakeTypePrim("i32"));
  Outcome out_lit = EvalSigma(ctx, *lit, sigma);
  assert(IsVal(out_lit));

  BindValue(sigma, "b", MakeBoolValue(true));
  auto unary = MakeUnaryExpr("!", MakeIdentifierExpr("b"));
  Outcome out_unary = EvalSigma(ctx, *unary, sigma);
  assert(IsVal(out_unary));

  auto block = MakeBlock({});
  auto block_expr = MakeBlockExpr(block);
  Outcome out_block = EvalSigma(ctx, *block_expr, sigma);
  assert(IsVal(out_block));

  auto unsafe_block = MakeUnsafeBlockExpr(MakeBlock({}));
  Outcome out_unsafe = EvalSigma(ctx, *unsafe_block, sigma);
  assert(IsVal(out_unsafe));

  BindValue(sigma, "val", MakeIntValue("i32", 3));
  const RegionTarget target = 1;
  AddRegionEntry(sigma, target, 1);

  auto alloc_implicit = MakeAllocExpr(std::nullopt, MakeIdentifierExpr("val"));
  Outcome out_alloc_implicit = EvalSigma(ctx, *alloc_implicit, sigma);
  assert(IsVal(out_alloc_implicit));

  BindValue(sigma, "r", MakeRegionHandleValue(target));
  auto alloc_explicit = MakeAllocExpr(std::optional<std::string>("r"), MakeIdentifierExpr("val"));
  Outcome out_alloc_explicit = EvalSigma(ctx, *alloc_explicit, sigma);
  assert(IsVal(out_alloc_explicit));

  auto alloc_implicit_ctrl = MakeAllocExpr(std::nullopt, MakeErrorExpr());
  Outcome out_alloc_implicit_ctrl = EvalSigma(ctx, *alloc_implicit_ctrl, sigma);
  assert(IsCtrl(out_alloc_implicit_ctrl));

  auto alloc_explicit_ctrl = MakeAllocExpr(std::optional<std::string>("r"), MakeErrorExpr());
  Outcome out_alloc_explicit_ctrl = EvalSigma(ctx, *alloc_explicit_ctrl, sigma);
  assert(IsCtrl(out_alloc_explicit_ctrl));
}

}  // namespace

int main() {
  SPEC_COV("EvalSigma-If-Ctrl");
  SPEC_COV("EvalSigma-If-False-None");
  SPEC_COV("EvalSigma-If-False-Some");
  SPEC_COV("EvalSigma-If-True");
  SPEC_COV("EvalSigma-Loop-Cond-Body-Ctrl");
  SPEC_COV("EvalSigma-Loop-Cond-Break");
  SPEC_COV("EvalSigma-Loop-Cond-Continue");
  SPEC_COV("EvalSigma-Loop-Cond-Ctrl");
  SPEC_COV("EvalSigma-Loop-Cond-False");
  SPEC_COV("EvalSigma-Loop-Cond-True-Step");
  SPEC_COV("EvalSigma-Loop-Infinite-Break");
  SPEC_COV("EvalSigma-Loop-Infinite-Continue");
  SPEC_COV("EvalSigma-Loop-Infinite-Ctrl");
  SPEC_COV("EvalSigma-Loop-Infinite-Step");
  SPEC_COV("EvalSigma-Loop-Iter");
  SPEC_COV("EvalSigma-Loop-Iter-Ctrl");
  SPEC_COV("EvalSigma-Match");
  SPEC_COV("EvalSigma-Match-Ctrl");
  SPEC_COV("EvalSigma-MethodCall");
  SPEC_COV("EvalSigma-MethodCall-Ctrl");
  SPEC_COV("EvalSigma-MethodCall-Ctrl-Args");
  SPEC_COV("EvalSigma-UnsafeBlock");
  SPEC_COV("LoopIter-Done");
  SPEC_COV("LoopIter-Step-Break");
  SPEC_COV("LoopIter-Step-Continue");
  SPEC_COV("LoopIter-Step-Ctrl");
  SPEC_COV("LoopIter-Step-Val");
  SPEC_COV("MatchArm-Ctrl");
  SPEC_COV("MatchArm-Fail");
  SPEC_COV("MatchArm-Guard-False");
  SPEC_COV("MatchArm-Guard-True");
  SPEC_COV("MatchArm-NoGuard");
  SPEC_COV("MatchArms-Head");
  SPEC_COV("MatchArms-Tail");
  SPEC_COV("StepSigma-Alloc-Explicit");
  SPEC_COV("StepSigma-Alloc-Explicit-Ctrl");
  SPEC_COV("StepSigma-Alloc-Implicit");
  SPEC_COV("StepSigma-Alloc-Implicit-Ctrl");
  SPEC_COV("StepSigma-Block");
  SPEC_COV("StepSigma-Loop");
  SPEC_COV("StepSigma-Pure");
  SPEC_COV("StepSigma-Stateful-Other");
  SPEC_COV("StepSigma-UnsafeBlock");

  TestIfExprs();
  TestMatchExprs();
  TestLoopConditional();
  TestLoopInfinite();
  TestLoopIter();
  TestMethodCall();
  TestStepSigmaNodes();
  return 0;
}
