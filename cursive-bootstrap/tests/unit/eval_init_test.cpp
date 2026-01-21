#include <cassert>
#include <string>
#include <string_view>
#include <vector>

#include "cursive0/core/symbols.h"
#include "cursive0/analysis/resolve/collect_toplevel.h"
#include "cursive0/analysis/resolve/resolver.h"
#include "cursive0/analysis/resolve/scopes.h"
#include "cursive0/analysis/resolve/scopes_lookup.h"
#include "cursive0/eval/init.h"
#include "cursive0/eval/state.h"
#include "cursive0/syntax/ast.h"

namespace {

using cursive0::analysis::ModuleNames;
using cursive0::analysis::PathKeyOf;
using cursive0::analysis::ScopeContext;
using cursive0::syntax::ASTItem;
using cursive0::syntax::ASTModule;
using cursive0::syntax::Binding;
using cursive0::syntax::IdentifierExpr;
using cursive0::syntax::IdentifierPattern;
using cursive0::syntax::LiteralExpr;
using cursive0::syntax::ModulePath;
using cursive0::syntax::StaticDecl;
using cursive0::syntax::Type;
using cursive0::syntax::TypePrim;
using cursive0::syntax::TypePath;
using cursive0::syntax::TypePathType;
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

std::shared_ptr<Type> MakeTypePath(const TypePath& path) {
  auto type = std::make_shared<Type>();
  type->span = EmptySpan();
  type->node = TypePathType{path};
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
  cursive0::syntax::Token tok;
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

cursive0::syntax::ExprPtr MakeQualifiedExpr(const ModulePath& path,
                                            std::string_view name) {
  auto expr = std::make_shared<cursive0::syntax::Expr>();
  expr->span = EmptySpan();
  expr->node = cursive0::syntax::PathExpr{path, std::string(name)};
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

ModuleNames ModuleNamesFromScope(const ScopeContext& ctx) {
  ModuleNames names;
  names.reserve(ctx.sigma.mods.size());
  for (const auto& mod : ctx.sigma.mods) {
    names.push_back(cursive0::core::StringOfPath(mod.path));
  }
  return names;
}

}  // namespace

int main() {
  using cursive0::eval::CleanupStatus;
  using cursive0::eval::Init;
  using cursive0::eval::Deinit;
  using cursive0::eval::PoisonedModule;
  using cursive0::eval::Sigma;
  using cursive0::eval::SemanticsContext;

  {
    ScopeContext scope;
    scope.scopes = {cursive0::analysis::Scope{}, cursive0::analysis::Scope{},
                    cursive0::analysis::UniverseBindings()};
    ModulePath main_path{"main"};
    auto init_expr = MakeLiteralExpr("1");
    auto mod = MakeModule(main_path, {MakeStatic("x", init_expr)});
    scope.sigma.mods = {mod};
    scope.current_module = main_path;
    cursive0::analysis::PopulateSigma(scope);

    auto name_maps = cursive0::analysis::CollectNameMaps(scope);
    auto module_names = ModuleNamesFromScope(scope);
    cursive0::analysis::ExprTypeMap expr_types;
    expr_types[init_expr.get()] = cursive0::analysis::MakeTypePrim("i32");
    scope.expr_types = &expr_types;

    SemanticsContext ctx;
    ctx.sema = &scope;
    ctx.name_maps = &name_maps.name_maps;
    ctx.module_names = &module_names;

    Sigma sigma;
    const auto init_result = Init(ctx, sigma);
    assert(init_result.ok);
    assert(!PoisonedModule(sigma, PathKeyOf(main_path)));

    const auto status = Deinit(ctx, sigma);
    assert(status == CleanupStatus::Ok);
  }

  {
    ScopeContext scope;
    scope.scopes = {cursive0::analysis::Scope{}, cursive0::analysis::Scope{},
                    cursive0::analysis::UniverseBindings()};
    ModulePath main_path{"main"};
    ModulePath dep_path{"dep"};

    auto dep_mod = MakeModule(dep_path, {MakeStatic("b", MakeErrorExpr())});
    auto main_mod = MakeModule(main_path,
                               {MakeStatic("a", MakeQualifiedExpr(dep_path, "b"))});

    scope.sigma.mods = {main_mod, dep_mod};
    scope.current_module = main_path;
    cursive0::analysis::PopulateSigma(scope);

    auto name_maps = cursive0::analysis::CollectNameMaps(scope);
    auto module_names = ModuleNamesFromScope(scope);
    cursive0::analysis::ExprTypeMap expr_types;
    scope.expr_types = &expr_types;

    SemanticsContext ctx;
    ctx.sema = &scope;
    ctx.name_maps = &name_maps.name_maps;
    ctx.module_names = &module_names;

    Sigma sigma;
    const auto init_result = Init(ctx, sigma);
    assert(!init_result.ok);

    assert(PoisonedModule(sigma, PathKeyOf(dep_path)));
    assert(PoisonedModule(sigma, PathKeyOf(main_path)));
  }

  return 0;
}
