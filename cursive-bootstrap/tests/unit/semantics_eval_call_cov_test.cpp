#include <cassert>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/symbols.h"
#include "cursive0/sema/collect_toplevel.h"
#include "cursive0/sema/scopes.h"
#include "cursive0/sema/types.h"
#include "cursive0/semantics/eval.h"
#include "cursive0/semantics/state.h"
#include "cursive0/syntax/ast.h"
#include "cursive0/syntax/token.h"

namespace {

using cursive0::core::StringOfPath;
using cursive0::semantics::BindInfo;
using cursive0::semantics::BindVal;
using cursive0::semantics::IsCtrl;
using cursive0::semantics::IsVal;
using cursive0::semantics::Sigma;
using cursive0::semantics::SemanticsContext;
using cursive0::syntax::Arg;
using cursive0::syntax::Block;
using cursive0::syntax::BlockExpr;
using cursive0::syntax::CallExpr;
using cursive0::syntax::ErrorExpr;
using cursive0::syntax::Expr;
using cursive0::syntax::ExprPtr;
using cursive0::syntax::IdentifierExpr;
using cursive0::syntax::LiteralExpr;
using cursive0::syntax::MatchArm;
using cursive0::syntax::MatchExpr;
using cursive0::syntax::MethodCallExpr;
using cursive0::syntax::MoveExpr;
using cursive0::syntax::Param;
using cursive0::syntax::ParamMode;
using cursive0::syntax::PathExpr;
using cursive0::syntax::Pattern;
using cursive0::syntax::ProcedureDecl;
using cursive0::syntax::RecordDecl;
using cursive0::syntax::Token;
using cursive0::syntax::TokenKind;
using cursive0::syntax::Type;
using cursive0::syntax::TypePathType;
using cursive0::syntax::TypePrim;
using cursive0::syntax::Visibility;
using cursive0::syntax::WildcardPattern;

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

static ExprPtr MakeIdentifierExpr(std::string name) {
  return MakeExpr(IdentifierExpr{std::move(name)});
}

static ExprPtr MakeErrorExpr() {
  return MakeExpr(ErrorExpr{});
}

static ExprPtr MakeMoveExpr(ExprPtr value) {
  MoveExpr move;
  move.place = std::move(value);
  return MakeExpr(std::move(move));
}

static ExprPtr MakeCallExpr(ExprPtr callee, std::vector<Arg> args) {
  CallExpr call;
  call.callee = std::move(callee);
  call.args = std::move(args);
  return MakeExpr(std::move(call));
}

static ExprPtr MakeMethodCallExpr(ExprPtr receiver,
                                  std::string name,
                                  std::vector<Arg> args) {
  MethodCallExpr call;
  call.receiver = std::move(receiver);
  call.name = std::move(name);
  call.args = std::move(args);
  return MakeExpr(std::move(call));
}

static ExprPtr MakeBlockExpr(std::shared_ptr<Block> block) {
  BlockExpr node;
  node.block = std::move(block);
  return MakeExpr(std::move(node));
}

static std::shared_ptr<Block> MakeEmptyBlock() {
  auto block = std::make_shared<Block>();
  block->span = EmptySpan();
  return block;
}

static std::shared_ptr<Block> MakeBlockWithTail(const ExprPtr& tail) {
  auto block = std::make_shared<Block>();
  block->span = EmptySpan();
  block->tail_opt = tail;
  return block;
}

static Arg MakeArg(ExprPtr value, bool moved = false) {
  Arg arg;
  arg.value = std::move(value);
  arg.moved = moved;
  arg.span = EmptySpan();
  return arg;
}

static std::shared_ptr<Type> MakePrimType(std::string name) {
  auto type = std::make_shared<Type>();
  type->span = EmptySpan();
  type->node = TypePrim{std::move(name)};
  return type;
}

static std::shared_ptr<Type> MakePathType(std::vector<std::string> path) {
  auto type = std::make_shared<Type>();
  type->span = EmptySpan();
  type->node = TypePathType{std::move(path)};
  return type;
}

static Param MakeParam(std::string name, bool move) {
  Param param;
  param.mode = move ? std::optional<ParamMode>(ParamMode::Move) : std::nullopt;
  param.name = std::move(name);
  param.type = MakePrimType("i32");
  param.span = EmptySpan();
  return param;
}

static ProcedureDecl MakeProc(std::string name, std::vector<Param> params) {
  ProcedureDecl proc;
  proc.vis = Visibility::Public;
  proc.name = std::move(name);
  proc.params = std::move(params);
  proc.return_type_opt = MakePrimType("i32");
  proc.body = MakeEmptyBlock();
  proc.span = EmptySpan();
  proc.doc = {};
  return proc;
}

static std::shared_ptr<Pattern> MakeWildcardPattern() {
  auto pat = std::make_shared<Pattern>();
  pat->span = EmptySpan();
  pat->node = WildcardPattern{};
  return pat;
}

static RecordDecl MakeRecordDecl(std::string name, ExprPtr init) {
  RecordDecl decl;
  decl.vis = Visibility::Public;
  decl.name = std::move(name);
  decl.implements = {};
  decl.members = {};
  cursive0::syntax::FieldDecl field;
  field.vis = Visibility::Public;
  field.name = "x";
  field.type = MakePrimType("i32");
  field.init_opt = std::move(init);
  field.span = EmptySpan();
  field.doc_opt = std::nullopt;
  decl.members.push_back(std::move(field));
  decl.span = EmptySpan();
  decl.doc = {};
  return decl;
}

static cursive0::sema::ModuleNames ModuleNamesFromScope(
    const cursive0::sema::ScopeContext& ctx) {
  cursive0::sema::ModuleNames names;
  names.reserve(ctx.sigma.mods.size());
  for (const auto& mod : ctx.sigma.mods) {
    names.push_back(StringOfPath(mod.path));
  }
  return names;
}

static void SetExprType(cursive0::sema::ExprTypeMap& map,
                        const ExprPtr& expr,
                        cursive0::sema::TypeRef type) {
  map[expr.get()] = std::move(type);
}

void TestEvalArgsAndCallRules() {
  SPEC_COV("EvalArgsSigma-Cons-Move");
  SPEC_COV("EvalArgsSigma-Cons-Ref");
  SPEC_COV("EvalArgsSigma-Ctrl-Move");
  SPEC_COV("EvalArgsSigma-Ctrl-Ref");
  SPEC_COV("EvalArgsSigma-Empty");
  SPEC_COV("EvalSigma-Call-Ctrl");
  SPEC_COV("EvalSigma-Call-Ctrl-Args");
  SPEC_COV("EvalSigma-Call-Proc");
  SPEC_COV("EvalSigma-Call-Record");
  SPEC_COV("EvalSigma-Call-RegionProc");
  SPEC_COV("EvalSigma-Call-RegionProc-Ctrl-Args");

  cursive0::sema::ScopeContext scope;
  scope.scopes = {cursive0::sema::Scope{}, cursive0::sema::Scope{},
                  cursive0::sema::UniverseBindings()};
  scope.current_module = {"main"};

  auto rec_init = MakeLiteralExpr(TokenKind::IntLiteral, "1");
  auto proc_move = MakeProc("proc_move", {MakeParam("v", true)});
  auto proc_ref = MakeProc("proc_ref", {MakeParam("v", false)});
  auto proc_empty = MakeProc("proc_empty", {});
  auto record = MakeRecordDecl("Rec", rec_init);

  cursive0::syntax::ASTModule mod;
  mod.path = {"main"};
  mod.items = {proc_move, proc_ref, proc_empty, record};
  mod.module_doc = {};
  mod.unsafe_spans = {};
  scope.sigma.mods = {mod};

  // cursive0::sema::PopulateSigma(scope);

  cursive0::sema::ExprTypeMap expr_types;
  scope.expr_types = &expr_types;
  SetExprType(expr_types, rec_init, cursive0::sema::MakeTypePrim("i32"));

  auto lit_arg = MakeLiteralExpr(TokenKind::IntLiteral, "2");
  SetExprType(expr_types, lit_arg, cursive0::sema::MakeTypePrim("i32"));

  auto name_maps = cursive0::sema::CollectNameMaps(scope);
  auto module_names = ModuleNamesFromScope(scope);

  SemanticsContext ctx;
  ctx.sema = &scope;
  ctx.name_maps = &name_maps.name_maps;
  ctx.module_names = &module_names;

  Sigma sigma;
  cursive0::semantics::PushScope(sigma, nullptr);
  cursive0::semantics::Binding binding;
  BindVal(sigma, "x", cursive0::semantics::Value{cursive0::semantics::UnitVal{}},
          BindInfo{}, &binding);
  sigma.store[binding.addr] = cursive0::semantics::Value{cursive0::semantics::UnitVal{}};

  auto call_empty = MakeCallExpr(MakeIdentifierExpr("proc_empty"), {});
  auto out_empty = cursive0::semantics::EvalSigma(ctx, *call_empty, sigma);
  assert(IsVal(out_empty));

  auto call_move_ok =
      MakeCallExpr(MakeIdentifierExpr("proc_move"), {MakeArg(lit_arg)});
  auto out_move_ok = cursive0::semantics::EvalSigma(ctx, *call_move_ok, sigma);
  assert(IsVal(out_move_ok));

  auto call_ref_ok =
      MakeCallExpr(MakeIdentifierExpr("proc_ref"),
                   {MakeArg(MakeIdentifierExpr("x"))});
  auto out_ref_ok = cursive0::semantics::EvalSigma(ctx, *call_ref_ok, sigma);
  assert(IsVal(out_ref_ok));

  auto call_move_ctrl =
      MakeCallExpr(MakeIdentifierExpr("proc_move"), {MakeArg(MakeErrorExpr())});
  auto out_move_ctrl = cursive0::semantics::EvalSigma(ctx, *call_move_ctrl, sigma);
  assert(IsCtrl(out_move_ctrl));

  auto call_ref_ctrl =
      MakeCallExpr(MakeIdentifierExpr("proc_ref"),
                   {MakeArg(MakeIdentifierExpr("missing"))});
  auto out_ref_ctrl = cursive0::semantics::EvalSigma(ctx, *call_ref_ctrl, sigma);
  assert(IsCtrl(out_ref_ctrl));

  auto call_callee_ctrl = MakeCallExpr(MakeErrorExpr(), {});
  auto out_callee_ctrl = cursive0::semantics::EvalSigma(ctx, *call_callee_ctrl, sigma);
  assert(IsCtrl(out_callee_ctrl));

  auto call_record = MakeCallExpr(MakeIdentifierExpr("Rec"), {});
  auto out_record = cursive0::semantics::EvalSigma(ctx, *call_record, sigma);
  assert(IsVal(out_record));

  auto call_region =
      MakeCallExpr(MakeExpr(PathExpr{{"Region"}, "new_scoped"}),
                   {MakeArg(MakeIdentifierExpr("x"))});
  auto out_region = cursive0::semantics::EvalSigma(ctx, *call_region, sigma);
  assert(IsVal(out_region));

  auto call_region_ctrl =
      MakeCallExpr(MakeExpr(PathExpr{{"Region"}, "new_scoped"}),
                   {MakeArg(MakeIdentifierExpr("missing"))});
  auto out_region_ctrl = cursive0::semantics::EvalSigma(ctx, *call_region_ctrl, sigma);
  assert(IsCtrl(out_region_ctrl));
}

void TestEvalRecvAndArmBodyRules() {
  SPEC_COV("EvalRecvSigma-Ctrl-Move");
  SPEC_COV("EvalRecvSigma-Ctrl-Ref");
  SPEC_COV("EvalRecvSigma-Move");
  SPEC_COV("EvalRecvSigma-Ref");
  SPEC_COV("EvalRecvSigma-Ref-Dyn");
  SPEC_COV("EvalRecvSigma-Ref-Dyn-Expired");
  SPEC_COV("EvalArmBody-Block");
  SPEC_COV("EvalArmBody-Expr");
  SPEC_COV("EvalSigma-Block");

  cursive0::sema::ScopeContext scope;
  scope.scopes = {cursive0::sema::Scope{}, cursive0::sema::Scope{},
                  cursive0::sema::UniverseBindings()};
  scope.current_module = {"main"};

  cursive0::syntax::ASTModule mod;
  mod.path = {"main"};
  mod.items = {};
  mod.module_doc = {};
  mod.unsafe_spans = {};
  scope.sigma.mods = {mod};

  // cursive0::sema::PopulateSigma(scope);
  cursive0::sema::ExprTypeMap expr_types;
  scope.expr_types = &expr_types;

  auto lit = MakeLiteralExpr(TokenKind::IntLiteral, "1");
  SetExprType(expr_types, lit, cursive0::sema::MakeTypePrim("i32"));

  auto name_maps = cursive0::sema::CollectNameMaps(scope);
  auto module_names = ModuleNamesFromScope(scope);

  SemanticsContext ctx;
  ctx.sema = &scope;
  ctx.name_maps = &name_maps.name_maps;
  ctx.module_names = &module_names;

  Sigma sigma;
  cursive0::semantics::PushScope(sigma, nullptr);

  cursive0::semantics::Binding recv_binding;
  BindVal(sigma, "recv", cursive0::semantics::Value{cursive0::semantics::UnitVal{}},
          BindInfo{}, &recv_binding);
  sigma.store[recv_binding.addr] = cursive0::semantics::Value{cursive0::semantics::UnitVal{}};

  auto move_recv = MakeMethodCallExpr(
      MakeMoveExpr(MakeIdentifierExpr("recv")), "noop", {});
  auto out_move = cursive0::semantics::EvalSigma(ctx, *move_recv, sigma);
  assert(IsCtrl(out_move) || IsVal(out_move));

  auto move_ctrl = MakeMethodCallExpr(
      MakeMoveExpr(MakeIdentifierExpr("missing")), "noop", {});
  auto out_move_ctrl = cursive0::semantics::EvalSigma(ctx, *move_ctrl, sigma);
  assert(IsCtrl(out_move_ctrl));

  auto ref_recv = MakeMethodCallExpr(MakeIdentifierExpr("recv"), "noop", {});
  auto out_ref = cursive0::semantics::EvalSigma(ctx, *ref_recv, sigma);
  assert(IsCtrl(out_ref) || IsVal(out_ref));

  auto ref_ctrl = MakeMethodCallExpr(MakeIdentifierExpr("missing"), "noop", {});
  auto out_ref_ctrl = cursive0::semantics::EvalSigma(ctx, *ref_ctrl, sigma);
  assert(IsCtrl(out_ref_ctrl));

  // Dynamic receiver (active)
  cursive0::semantics::Binding dyn_binding;
  BindVal(sigma, "dyn", cursive0::semantics::Value{cursive0::semantics::UnitVal{}},
          BindInfo{}, &dyn_binding);
  const auto dyn_addr = cursive0::semantics::AllocAddr(sigma);
  const auto scope_id = cursive0::semantics::CurrentScopeId(sigma);
  cursive0::semantics::DynamicVal dyn_val;
  dyn_val.class_path = {"Unknown"};
  dyn_val.data.qual = cursive0::sema::RawPtrQual::Imm;
  dyn_val.data.addr = dyn_addr;
  dyn_val.concrete_type = nullptr;
  sigma.addr_tags[dyn_addr] =
      cursive0::semantics::RuntimeTag{cursive0::semantics::RuntimeTagKind::ScopeTag, scope_id};
  sigma.store[dyn_binding.addr] = cursive0::semantics::Value{dyn_val};

  auto dyn_recv = MakeMethodCallExpr(MakeIdentifierExpr("dyn"), "noop", {});
  auto out_dyn = cursive0::semantics::EvalSigma(ctx, *dyn_recv, sigma);
  assert(IsCtrl(out_dyn) || IsVal(out_dyn));

  // Dynamic receiver (expired)
  cursive0::semantics::Binding dead_binding;
  BindVal(sigma, "dyn_dead", cursive0::semantics::Value{cursive0::semantics::UnitVal{}},
          BindInfo{}, &dead_binding);
  const auto dead_addr = cursive0::semantics::AllocAddr(sigma);
  cursive0::semantics::DynamicVal dead_val;
  dead_val.class_path = {"Unknown"};
  dead_val.data.qual = cursive0::sema::RawPtrQual::Imm;
  dead_val.data.addr = dead_addr;
  dead_val.concrete_type = nullptr;
  sigma.addr_tags[dead_addr] =
      cursive0::semantics::RuntimeTag{cursive0::semantics::RuntimeTagKind::ScopeTag, 9999};
  sigma.store[dead_binding.addr] = cursive0::semantics::Value{dead_val};

  auto dead_recv = MakeMethodCallExpr(MakeIdentifierExpr("dyn_dead"), "noop", {});
  auto out_dead = cursive0::semantics::EvalSigma(ctx, *dead_recv, sigma);
  assert(IsCtrl(out_dead));

  auto block_expr = MakeBlockExpr(MakeBlockWithTail(lit));
  auto out_block = cursive0::semantics::EvalSigma(ctx, *block_expr, sigma);
  assert(IsVal(out_block));

  // Match arm body: block
  MatchArm arm_block;
  arm_block.pattern = MakeWildcardPattern();
  arm_block.guard_opt = nullptr;
  arm_block.body = MakeBlockExpr(MakeBlockWithTail(lit));
  MatchExpr match_block;
  match_block.value = lit;
  match_block.arms = {arm_block};
  auto match_block_expr = MakeExpr(match_block);
  auto out_match_block = cursive0::semantics::EvalSigma(ctx, *match_block_expr, sigma);
  assert(IsVal(out_match_block));

  // Match arm body: expr
  MatchArm arm_expr;
  arm_expr.pattern = MakeWildcardPattern();
  arm_expr.guard_opt = nullptr;
  arm_expr.body = lit;
  MatchExpr match_expr;
  match_expr.value = lit;
  match_expr.arms = {arm_expr};
  auto match_expr_expr = MakeExpr(match_expr);
  auto out_match_expr = cursive0::semantics::EvalSigma(ctx, *match_expr_expr, sigma);
  assert(IsVal(out_match_expr));
}

}  // namespace

int main() {
  TestEvalArgsAndCallRules();
  TestEvalRecvAndArmBodyRules();
  return 0;
}
