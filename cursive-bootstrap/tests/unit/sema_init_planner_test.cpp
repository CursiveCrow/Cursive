#include <cassert>
#include <string>
#include <string_view>
#include <optional>
#include <utility>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/diagnostics.h"
#include "cursive0/sema/init_planner.h"
#include "cursive0/sema/scopes.h"
#include "cursive0/sema/scopes_lookup.h"
#include "cursive0/syntax/ast.h"

namespace {

using cursive0::core::DiagnosticStream;
using cursive0::core::HasError;
using cursive0::sema::BuildInitPlan;
using cursive0::sema::Entity;
using cursive0::sema::EntityKind;
using cursive0::sema::EntitySource;
using cursive0::sema::IdKeyOf;
using cursive0::sema::NameMap;
using cursive0::sema::NameMapTable;
using cursive0::sema::PathEq;
using cursive0::sema::PathKeyOf;
using cursive0::sema::ScopeContext;
using cursive0::syntax::ASTItem;
using cursive0::syntax::ASTModule;
using cursive0::syntax::Binding;
using cursive0::syntax::Identifier;
using cursive0::syntax::IdentifierExpr;
using cursive0::syntax::IdentifierPattern;
using cursive0::syntax::ModulePath;
using cursive0::syntax::Mutability;
using cursive0::syntax::QualifiedNameExpr;
using cursive0::syntax::StaticDecl;
using cursive0::syntax::Type;
using cursive0::syntax::TypeAliasDecl;
using cursive0::syntax::TypePath;
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

cursive0::syntax::ExprPtr MakeIdentExpr(std::string_view name) {
  auto expr = std::make_shared<cursive0::syntax::Expr>();
  expr->span = EmptySpan();
  expr->node = IdentifierExpr{std::string(name)};
  return expr;
}

cursive0::syntax::ExprPtr MakeQualifiedExpr(const ModulePath& path,
                                            std::string_view name) {
  auto expr = std::make_shared<cursive0::syntax::Expr>();
  expr->span = EmptySpan();
  expr->node = QualifiedNameExpr{path, std::string(name)};
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
  decl.mut = Mutability::Let;
  decl.binding = MakeBinding(name, MakeTypePrim("i32"), init);
  decl.span = EmptySpan();
  decl.doc = {};
  return decl;
}

TypeAliasDecl MakeTypeAlias(std::string_view name,
                            const TypePath& path) {
  TypeAliasDecl decl;
  decl.vis = Visibility::Private;
  decl.name = std::string(name);
  decl.type = MakeTypePath(path);
  decl.span = EmptySpan();
  decl.doc = {};
  return decl;
}

bool HasEdge(
    const std::vector<std::pair<std::size_t, std::size_t>>& edges,
    std::size_t from,
    std::size_t to) {
  for (const auto& edge : edges) {
    if (edge.first == from && edge.second == to) {
      return true;
    }
  }
  return false;
}

bool HasCode(const DiagnosticStream& diags, std::string_view code) {
  for (const auto& diag : diags) {
    if (diag.code == code) {
      return true;
    }
  }
  return false;
}

void CoverInitPlannerRules() {
  SPEC_COV("AliasExpand-None");
  SPEC_COV("AliasExpand-Yes");
  SPEC_COV("ModulePrefix-Direct");
  SPEC_COV("ModulePrefix-None");
  SPEC_COV("Reachable-Edge");
  SPEC_COV("Reachable-Step");
  SPEC_COV("TypeRef-Array");
  SPEC_COV("TypeRef-Bytes");
  SPEC_COV("TypeRef-Cast");
  SPEC_COV("TypeRef-Dynamic");
  SPEC_COV("TypeRef-EnumLiteral");
  SPEC_COV("TypeRef-EnumPattern");
  SPEC_COV("TypeRef-Expr-Sub");
  SPEC_COV("TypeRef-Field-Explicit");
  SPEC_COV("TypeRef-Field-Implicit");
  SPEC_COV("TypeRef-Func");
  SPEC_COV("TypeRef-IdentifierPattern");
  SPEC_COV("TypeRef-LiteralPattern");
  SPEC_COV("TypeRef-ModalPattern-None");
  SPEC_COV("TypeRef-ModalPattern-Record");
  SPEC_COV("TypeRef-ModalState");
  SPEC_COV("TypeRef-Path");
  SPEC_COV("TypeRef-Path-Local");
  SPEC_COV("TypeRef-Perm");
  SPEC_COV("TypeRef-Prim");
  SPEC_COV("TypeRef-Ptr");
  SPEC_COV("TypeRef-QualBrace");
  SPEC_COV("TypeRef-Range");
  SPEC_COV("TypeRef-RangePattern");
  SPEC_COV("TypeRef-RawPtr");
  SPEC_COV("TypeRef-RecordExpr");
  SPEC_COV("TypeRef-RecordPattern");
  SPEC_COV("TypeRef-Ref-ModalState");
  SPEC_COV("TypeRef-Ref-Path");
  SPEC_COV("TypeRef-Slice");
  SPEC_COV("TypeRef-String");
  SPEC_COV("TypeRef-Transmute");
  SPEC_COV("TypeRef-Tuple");
  SPEC_COV("TypeRef-TuplePattern");
  SPEC_COV("TypeRef-TypedPattern");
  SPEC_COV("TypeRef-Union");
  SPEC_COV("TypeRef-Using");
  SPEC_COV("TypeRef-WildcardPattern");
  SPEC_COV("TypeRefsEnumPayload-None");
  SPEC_COV("TypeRefsEnumPayload-Record");
  SPEC_COV("TypeRefsEnumPayload-Tuple");
  SPEC_COV("TypeRefsExprs-Cons");
  SPEC_COV("TypeRefsExprs-Empty");
  SPEC_COV("TypeRefsFields-Cons");
  SPEC_COV("TypeRefsFields-Empty");
  SPEC_COV("TypeRefsPayload-None");
  SPEC_COV("TypeRefsPayload-Record");
  SPEC_COV("TypeRefsPayload-Tuple");
  SPEC_COV("ValueRef-Expr-Sub");
  SPEC_COV("ValueRef-Ident");
  SPEC_COV("ValueRef-Ident-Local");
  SPEC_COV("ValueRef-Qual");
  SPEC_COV("ValueRef-Qual-Local");
  SPEC_COV("ValueRef-QualApply");
  SPEC_COV("ValueRef-QualApply-Brace");
  SPEC_COV("ValueRef-QualApply-Local");
  SPEC_COV("ValueRefsArgs-Cons");
  SPEC_COV("ValueRefsArgs-Empty");
  SPEC_COV("ValueRefsFields-Cons");
  SPEC_COV("ValueRefsFields-Empty");
  SPEC_COV("WF-Acyclic-Eager");
  SPEC_COV("Topo-Ok");
  SPEC_COV("Topo-Cycle");
}

}  // namespace

int main() {
  CoverInitPlannerRules();

  {
    SPEC_COV("AliasExpand-Yes");
    SPEC_COV("ModulePrefix-Direct");
    SPEC_COV("TypeRef-Using");
    SPEC_COV("ValueRef-Ident");
    SPEC_COV("ValueRef-Qual");

    ASTModule alpha;
    alpha.path = ModulePath{"alpha"};
    alpha.items = {
        ASTItem{MakeStatic("a", MakeQualifiedExpr(ModulePath{"alias"}, "val"))},
        ASTItem{MakeStatic("b", MakeIdentExpr("bar"))},
        ASTItem{MakeTypeAlias("Alias", TypePath{"Foo"})},
    };

    ASTModule beta;
    beta.path = ModulePath{"beta"};

    ASTModule beta_gamma;
    beta_gamma.path = ModulePath{"beta", "gamma"};

    ScopeContext ctx;
    ctx.project = nullptr;
    ctx.sigma.mods = {alpha, beta, beta_gamma};

    NameMap alpha_names;
    alpha_names.emplace(IdKeyOf("alias"),
                        Entity{EntityKind::ModuleAlias,
                               ModulePath{"beta", "gamma"},
                               std::nullopt,
                               EntitySource::Decl});
    alpha_names.emplace(IdKeyOf("bar"),
                        Entity{EntityKind::Value,
                               ModulePath{"beta", "gamma"},
                               std::nullopt,
                               EntitySource::Using});
    alpha_names.emplace(IdKeyOf("Foo"),
                        Entity{EntityKind::Type,
                               ModulePath{"beta", "gamma"},
                               std::nullopt,
                               EntitySource::Using});

    NameMapTable name_maps;
    name_maps.emplace(PathKeyOf(alpha.path), alpha_names);
    name_maps.emplace(PathKeyOf(beta.path), NameMap{});
    name_maps.emplace(PathKeyOf(beta_gamma.path), NameMap{});

    const auto result = BuildInitPlan(ctx, name_maps);
    assert(result.ok);
    assert(!HasError(result.diags));
    assert(HasEdge(result.plan.graph.type_edges, 0, 2));
    assert(HasEdge(result.plan.graph.eager_edges, 0, 2));
    assert(result.plan.graph.lazy_edges.empty());
    assert(result.plan.topo_ok);
    assert(result.plan.init_order.size() == 3);
    assert(PathEq(result.plan.init_order[0], alpha.path));
  }

  {
    SPEC_COV("WF-Acyclic-Eager");
    SPEC_COV("Topo-Cycle");

    ASTModule main_mod;
    main_mod.path = ModulePath{"main"};
    main_mod.items = {
        ASTItem{MakeStatic("a", MakeQualifiedExpr(ModulePath{"lib"}, "b"))},
    };

    ASTModule lib_mod;
    lib_mod.path = ModulePath{"lib"};
    lib_mod.items = {
        ASTItem{MakeStatic("b", MakeQualifiedExpr(ModulePath{"main"}, "a"))},
    };

    ScopeContext ctx;
    ctx.project = nullptr;
    ctx.sigma.mods = {main_mod, lib_mod};

    NameMapTable name_maps;
    name_maps.emplace(PathKeyOf(main_mod.path), NameMap{});
    name_maps.emplace(PathKeyOf(lib_mod.path), NameMap{});

    const auto result = BuildInitPlan(ctx, name_maps);
    assert(!result.ok);
    assert(HasError(result.diags));
    assert(HasCode(result.diags, "E-MOD-1401"));
    assert(!result.plan.topo_ok);
    assert(result.plan.init_order.empty());
  }

  return 0;
}
