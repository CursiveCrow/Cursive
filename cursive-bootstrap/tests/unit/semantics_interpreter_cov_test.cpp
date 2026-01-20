#include <cassert>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "cursive0/project/project.h"
#include "cursive0/sema/collect_toplevel.h"
#include "cursive0/sema/resolver.h"
#include "cursive0/sema/scopes.h"
#include "cursive0/semantics/interpreter.h"
#include "cursive0/semantics/value.h"
#include "cursive0/syntax/ast.h"

namespace {

using cursive0::sema::ScopeContext;
using cursive0::syntax::ASTItem;
using cursive0::syntax::ASTModule;
using cursive0::syntax::Binding;
using cursive0::syntax::Block;
using cursive0::syntax::ErrorStmt;
using cursive0::syntax::IdentifierPattern;
using cursive0::syntax::LiteralExpr;
using cursive0::syntax::ModulePath;
using cursive0::syntax::Param;
using cursive0::syntax::ParamMode;
using cursive0::syntax::ProcedureDecl;
using cursive0::syntax::StaticDecl;
using cursive0::syntax::Token;
using cursive0::syntax::Type;
using cursive0::syntax::TypePathType;
using cursive0::syntax::TypePrim;
using cursive0::syntax::Visibility;

cursive0::core::Span EmptySpan() {
  return {};
}

std::shared_ptr<Type> MakeTypePrim(std::string_view name) {
  auto type = std::make_shared<Type>();
  type->span = EmptySpan();
  type->node = TypePrim{std::string(name)};
  return type;
}

std::shared_ptr<Type> MakeTypePath(ModulePath path) {
  auto type = std::make_shared<Type>();
  type->span = EmptySpan();
  type->node = TypePathType{std::move(path)};
  return type;
}

std::shared_ptr<cursive0::syntax::Pattern> MakeIdentPattern(
    std::string_view name) {
  auto pat = std::make_shared<cursive0::syntax::Pattern>();
  pat->span = EmptySpan();
  pat->node = IdentifierPattern{std::string(name)};
  return pat;
}

cursive0::syntax::ExprPtr MakeLiteralExpr(std::string_view lexeme) {
  auto expr = std::make_shared<cursive0::syntax::Expr>();
  expr->span = EmptySpan();
  Token tok;
  tok.kind = cursive0::syntax::TokenKind::IntLiteral;
  tok.lexeme = std::string(lexeme);
  expr->node = LiteralExpr{tok};
  return expr;
}

cursive0::syntax::ExprPtr MakeErrorExpr() {
  auto expr = std::make_shared<cursive0::syntax::Expr>();
  expr->span = EmptySpan();
  expr->node = cursive0::syntax::ErrorExpr{};
  return expr;
}

std::shared_ptr<Block> MakeBlockWithTail(const cursive0::syntax::ExprPtr& tail) {
  auto block = std::make_shared<Block>();
  block->span = EmptySpan();
  block->tail_opt = tail;
  return block;
}

std::shared_ptr<Block> MakeErrorBlock() {
  auto block = std::make_shared<Block>();
  block->span = EmptySpan();
  block->stmts.push_back(ErrorStmt{});
  return block;
}

Binding MakeBinding(std::string_view name,
                    const std::shared_ptr<Type>& type,
                    const cursive0::syntax::ExprPtr& init) {
  Binding binding;
  binding.pat = MakeIdentPattern(name);
  binding.type_opt = type;
  binding.op = {};
  binding.init = init;
  binding.span = EmptySpan();
  return binding;
}

StaticDecl MakeStatic(std::string_view name,
                      const cursive0::syntax::ExprPtr& init) {
  StaticDecl decl;
  decl.vis = Visibility::Private;
  decl.mut = cursive0::syntax::Mutability::Let;
  decl.binding = MakeBinding(name, MakeTypePrim("i32"), init);
  decl.span = EmptySpan();
  decl.doc = {};
  return decl;
}

StaticDecl MakeBadStatic(std::string_view name,
                          const cursive0::syntax::ExprPtr& init) {
  StaticDecl decl;
  decl.vis = Visibility::Private;
  decl.mut = cursive0::syntax::Mutability::Let;
  decl.binding = MakeBinding(name, MakeTypePath({"UnknownType"}), init);
  decl.span = EmptySpan();
  decl.doc = {};
  return decl;
}


ProcedureDecl MakeMainProc(std::shared_ptr<Block> body) {
  ProcedureDecl proc;
  proc.vis = Visibility::Public;
  proc.name = "main";
  Param param;
  param.mode = ParamMode::Move;
  param.name = "ctx";
  param.type = MakeTypePath({"Context"});
  param.span = EmptySpan();
  proc.params = {param};
  proc.return_type_opt = MakeTypePrim("i32");
  proc.body = std::move(body);
  proc.span = EmptySpan();
  proc.doc = {};
  return proc;
}

ASTModule MakeModule(const ModulePath& path, std::vector<ASTItem> items) {
  ASTModule module;
  module.path = path;
  module.items = std::move(items);
  module.module_doc = {};
  module.unsafe_spans = {};
  return module;
}

cursive0::project::Project MakeProject() {
  cursive0::project::Project project;
  project.root = ".";
  project.source_root = ".";
  project.modules = {cursive0::project::ModuleInfo{"main", {}}};
  project.assembly.name = "main";
  return project;
}

ScopeContext MakeScope(const ASTModule& mod,
                       cursive0::sema::ExprTypeMap* expr_types) {
  ScopeContext scope;
  scope.scopes = {cursive0::sema::Scope{}, cursive0::sema::Scope{},
                  cursive0::sema::UniverseBindings()};
  scope.sigma.mods = {mod};
  scope.current_module = mod.path;
  cursive0::sema::PopulateSigma(scope);
  scope.expr_types = expr_types;
  return scope;
}

void TestInterpretProjectInitPanic() {
  SPEC_COV("ContextInitSigma");
  SPEC_COV("Interpret-Project-Init-Panic");

  auto main_expr = MakeLiteralExpr("0");
  auto main_proc = MakeMainProc(MakeBlockWithTail(main_expr));
  auto bad_static = MakeStatic("x", MakeErrorExpr());

  ASTModule mod = MakeModule({"main"}, {main_proc, bad_static});

  cursive0::sema::ExprTypeMap expr_types;
  expr_types[main_expr.get()] = cursive0::sema::MakeTypePrim("i32");
  ScopeContext scope = MakeScope(mod, &expr_types);

  cursive0::semantics::Sigma sigma;
  const auto project = MakeProject();
  auto result = cursive0::semantics::InterpretProject(project, sigma, scope);
  assert(result.has_control);
  assert(result.control.kind == cursive0::semantics::ControlKind::Panic);
}

void TestInterpretProjectOk() {
  SPEC_COV("Interpret-Project-Ok");

  auto main_expr = MakeLiteralExpr("7");
  auto main_proc = MakeMainProc(MakeBlockWithTail(main_expr));
  ASTModule mod = MakeModule({"main"}, {main_proc});

  cursive0::sema::ExprTypeMap expr_types;
  expr_types[main_expr.get()] = cursive0::sema::MakeTypePrim("i32");
  ScopeContext scope = MakeScope(mod, &expr_types);

  cursive0::semantics::Sigma sigma;
  const auto project = MakeProject();
  auto result = cursive0::semantics::InterpretProject(project, sigma, scope);
  assert(result.has_value);
  assert(!result.has_control);
}

void TestInterpretProjectMainCtrl() {
  SPEC_COV("Interpret-Project-Main-Ctrl");

  auto main_proc = MakeMainProc(MakeErrorBlock());
  ASTModule mod = MakeModule({"main"}, {main_proc});

  cursive0::sema::ExprTypeMap expr_types;
  ScopeContext scope = MakeScope(mod, &expr_types);

  cursive0::semantics::Sigma sigma;
  const auto project = MakeProject();
  auto result = cursive0::semantics::InterpretProject(project, sigma, scope);
  assert(result.has_control);
}

void TestInterpretProjectDeinitPanic() {
  SPEC_COV("Interpret-Project-Deinit-Panic");

  auto main_expr = MakeLiteralExpr("0");
  auto main_proc = MakeMainProc(MakeBlockWithTail(main_expr));
  auto bad_static = MakeBadStatic("bad", MakeLiteralExpr("1"));

  ASTModule mod = MakeModule({"main"}, {main_proc, bad_static});

  cursive0::sema::ExprTypeMap expr_types;
  expr_types[main_expr.get()] = cursive0::sema::MakeTypePrim("i32");
  expr_types[bad_static.binding.init.get()] = cursive0::sema::MakeTypePrim("i32");
  ScopeContext scope = MakeScope(mod, &expr_types);

  cursive0::semantics::Sigma sigma;
  const auto project = MakeProject();
  auto result = cursive0::semantics::InterpretProject(project, sigma, scope);
  assert(result.has_control);
  assert(result.control.kind == cursive0::semantics::ControlKind::Panic);
}


}  // namespace

int main() {
  TestInterpretProjectInitPanic();
  TestInterpretProjectOk();
  TestInterpretProjectMainCtrl();
  TestInterpretProjectDeinitPanic();
  return 0;
}
