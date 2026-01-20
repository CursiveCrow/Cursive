#include <cassert>
#include <string>
#include <vector>

#include "cursive0/core/int128.h"
#include "cursive0/core/symbols.h"
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
using cursive0::syntax::LiteralExpr;
using cursive0::syntax::ModulePath;
using cursive0::syntax::StaticDecl;
using cursive0::syntax::Type;
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

std::shared_ptr<cursive0::syntax::Pattern> MakeIdentPattern(
    std::string_view name) {
  auto pat = std::make_shared<cursive0::syntax::Pattern>();
  pat->span = EmptySpan();
  pat->node = cursive0::syntax::IdentifierPattern{std::string(name)};
  return pat;
}

cursive0::syntax::ExprPtr MakeLiteralExpr(std::string_view lexeme) {
  auto expr = std::make_shared<cursive0::syntax::Expr>();
  expr->span = EmptySpan();
  cursive0::syntax::Token tok;
  tok.kind = cursive0::syntax::TokenKind::IntLiteral;
  tok.lexeme = std::string(lexeme);
  expr->node = LiteralExpr{tok};
  return expr;
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

ASTModule MakeModule(const ModulePath& path, std::vector<ASTItem> items) {
  ASTModule module;
  module.path = path;
  module.items = std::move(items);
  module.module_doc = {};
  module.unsafe_spans = {};
  return module;
}

}  // namespace

int main() {
  using namespace cursive0::semantics;

  ScopeContext scope;
  scope.scopes = {cursive0::sema::Scope{}, cursive0::sema::Scope{},
                  cursive0::sema::UniverseBindings()};
  ModulePath main_path{"main"};
  auto init_expr = MakeLiteralExpr("1");
  auto mod = MakeModule(main_path, {MakeStatic("x", init_expr)});
  scope.sigma.mods = {mod};
  scope.current_module = main_path;
  cursive0::sema::PopulateSigma(scope);

  auto expr = MakeLiteralExpr("7");
  cursive0::sema::ExprTypeMap expr_types;
  expr_types[expr.get()] = cursive0::sema::MakeTypePrim("i32");
  expr_types[init_expr.get()] = cursive0::sema::MakeTypePrim("i32");
  scope.expr_types = &expr_types;

  Sigma sigma;
  auto expr_result = InterpretExpr(*expr, sigma, scope);
  assert(expr_result.has_value);
  assert(ValueToString(expr_result.value) == "i32(7)");

  Sigma sigma2;
  auto mod_result = InterpretModule(mod, sigma2, scope);
  assert(mod_result.has_value);
  assert(ValueToString(mod_result.value) == "()");

  return 0;
}
