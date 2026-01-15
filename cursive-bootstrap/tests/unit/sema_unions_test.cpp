#include <cassert>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include "cursive0/core/assert_spec.h"
#include "cursive0/sema/unions.h"
#include "cursive0/sema/types.h"
#include "cursive0/syntax/ast.h"

namespace {

using cursive0::sema::CheckUnionDirectAccess;
using cursive0::sema::ExprTypeFn;
using cursive0::sema::ExprTypeResult;
using cursive0::sema::MakeTypePerm;
using cursive0::sema::MakeTypePrim;
using cursive0::sema::MakeTypeUnion;
using cursive0::sema::Permission;
using cursive0::sema::ScopeContext;
using cursive0::sema::TypePerm;
using cursive0::sema::TypePrim;
using cursive0::sema::TypeRef;
using cursive0::sema::TypeUnion;
using cursive0::sema::TypeUnionIntro;
using cursive0::syntax::ExprPtr;
using cursive0::syntax::FieldAccessExpr;
using cursive0::syntax::IdentifierExpr;

TypeRef Prim(const char* name) { return MakeTypePrim(name); }

ExprPtr MakeExpr(cursive0::syntax::ExprNode node) {
  auto expr = std::make_shared<cursive0::syntax::Expr>();
  expr->node = std::move(node);
  return expr;
}

ExprPtr MakeIdent(const char* name) {
  return MakeExpr(IdentifierExpr{std::string(name)});
}

FieldAccessExpr MakeFieldAccess(const ExprPtr& base, const char* name) {
  FieldAccessExpr access;
  access.base = base;
  access.name = std::string(name);
  return access;
}

struct TypeEnv {
  std::unordered_map<std::string, TypeRef> types;

  ExprTypeResult TypeOf(const ExprPtr& expr) const {
    if (!expr) {
      return {false, std::nullopt, {}};
    }
    if (const auto* ident = std::get_if<IdentifierExpr>(&expr->node)) {
      const auto it = types.find(ident->name);
      if (it == types.end()) {
        return {false, "ResolveExpr-Ident-Err", {}};
      }
      return {true, std::nullopt, it->second};
    }
    return {false, "ResolveExpr-Ident-Err", {}};
  }
};

}  // namespace

int main() {
  ScopeContext ctx;

  {
    SPEC_COV("T-Union-Intro");
    auto uni = MakeTypeUnion({Prim("i32"), Prim("bool")});
    const auto result = TypeUnionIntro(ctx, Prim("i32"), uni);
    assert(result.ok);
    const auto* uni_node = std::get_if<TypeUnion>(&result.type->node);
    assert(uni_node && uni_node->members.size() == 2);
  }

  {
    auto uni = MakeTypeUnion({Prim("i32"), Prim("bool")});
    auto perm_union = MakeTypePerm(Permission::Const, uni);
    auto perm_value = MakeTypePerm(Permission::Const, Prim("i32"));
    const auto result = TypeUnionIntro(ctx, perm_value, perm_union);
    assert(result.ok);
    const auto* perm = std::get_if<TypePerm>(&result.type->node);
    assert(perm && perm->perm == Permission::Const);
  }

  {
    auto uni = MakeTypeUnion({Prim("i32"), Prim("bool")});
    const auto result = TypeUnionIntro(ctx, Prim("u8"), uni);
    assert(!result.ok);
    assert(!result.diag_id.has_value());
  }

  {
    SPEC_COV("Union-DirectAccess-Err");
    TypeEnv env;
    env.types["u"] = MakeTypeUnion({Prim("i32"), Prim("bool")});
    ExprTypeFn type_expr = [&](const auto& node) { return env.TypeOf(node); };
    auto access = MakeFieldAccess(MakeIdent("u"), "x");
    const auto result = CheckUnionDirectAccess(ctx, access, type_expr);
    assert(!result.ok);
    assert(result.diag_id ==
           std::optional<std::string_view>("Union-DirectAccess-Err"));
  }

  {
    TypeEnv env;
    env.types["u"] = MakeTypePerm(
        Permission::Const,
        MakeTypeUnion({Prim("i32"), Prim("bool")}));
    ExprTypeFn type_expr = [&](const auto& node) { return env.TypeOf(node); };
    auto access = MakeFieldAccess(MakeIdent("u"), "x");
    const auto result = CheckUnionDirectAccess(ctx, access, type_expr);
    assert(!result.ok);
    assert(result.diag_id ==
           std::optional<std::string_view>("Union-DirectAccess-Err"));
  }

  {
    TypeEnv env;
    env.types["r"] = cursive0::sema::MakeTypePath({"Record"});
    ExprTypeFn type_expr = [&](const auto& node) { return env.TypeOf(node); };
    auto access = MakeFieldAccess(MakeIdent("r"), "x");
    const auto result = CheckUnionDirectAccess(ctx, access, type_expr);
    assert(result.ok);
    assert(!result.diag_id.has_value());
  }

  return 0;
}
