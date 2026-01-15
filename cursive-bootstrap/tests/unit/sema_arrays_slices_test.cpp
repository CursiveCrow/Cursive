#include <cassert>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/sema/arrays_slices.h"
#include "cursive0/sema/types.h"
#include "cursive0/syntax/ast.h"

namespace {

using cursive0::sema::CoerceArrayToSlice;
using cursive0::sema::ExprTypeFn;
using cursive0::sema::ExprTypeResult;
using cursive0::sema::MakeTypeArray;
using cursive0::sema::MakeTypePerm;
using cursive0::sema::MakeTypePrim;
using cursive0::sema::MakeTypeSlice;
using cursive0::sema::Permission;
using cursive0::sema::PlaceTypeFn;
using cursive0::sema::PlaceTypeResult;
using cursive0::sema::ScopeContext;
using cursive0::sema::TypeArray;
using cursive0::sema::TypePerm;
using cursive0::sema::TypePrim;
using cursive0::sema::TypeRef;
using cursive0::sema::TypeSlice;
using cursive0::sema::TypeIndexAccessPlace;
using cursive0::sema::TypeIndexAccessValue;
using cursive0::sema::TypeArrayExpr;
using cursive0::syntax::ArrayExpr;
using cursive0::syntax::ExprPtr;
using cursive0::syntax::IdentifierExpr;
using cursive0::syntax::IndexAccessExpr;
using cursive0::syntax::LiteralExpr;
using cursive0::syntax::RangeExpr;
using cursive0::syntax::RangeKind;
using cursive0::syntax::Token;
using cursive0::syntax::TokenKind;

TypeRef Prim(const char* name) { return MakeTypePrim(name); }

ExprPtr MakeExpr(cursive0::syntax::ExprNode node) {
  auto expr = std::make_shared<cursive0::syntax::Expr>();
  expr->node = std::move(node);
  return expr;
}

ExprPtr MakeIdent(const char* name) {
  return MakeExpr(IdentifierExpr{std::string(name)});
}

ExprPtr MakeIntLiteral(const char* lexeme) {
  Token tok;
  tok.kind = TokenKind::IntLiteral;
  tok.lexeme = lexeme;
  return MakeExpr(LiteralExpr{tok});
}

ExprPtr MakeBoolLiteral(bool value) {
  Token tok;
  tok.kind = TokenKind::BoolLiteral;
  tok.lexeme = value ? "true" : "false";
  return MakeExpr(LiteralExpr{tok});
}

ExprPtr MakeRange(RangeKind kind, ExprPtr lhs, ExprPtr rhs) {
  RangeExpr range;
  range.kind = kind;
  range.lhs = std::move(lhs);
  range.rhs = std::move(rhs);
  return MakeExpr(range);
}

IndexAccessExpr MakeIndexAccess(const ExprPtr& base, const ExprPtr& index) {
  IndexAccessExpr access;
  access.base = base;
  access.index = index;
  return access;
}

static bool EndsWith(std::string_view value, std::string_view suffix) {
  if (suffix.size() > value.size()) {
    return false;
  }
  return value.substr(value.size() - suffix.size()) == suffix;
}

struct TypeEnv {
  std::unordered_map<std::string, TypeRef> types;

  ExprTypeResult TypeOf(const ExprPtr& expr) const {
    if (!expr) {
      return {false, std::nullopt, {}};
    }
    return std::visit(
        [&](const auto& node) -> ExprTypeResult {
          using T = std::decay_t<decltype(node)>;
          if constexpr (std::is_same_v<T, IdentifierExpr>) {
            const auto it = types.find(node.name);
            if (it == types.end()) {
              return {false, "ResolveExpr-Ident-Err", {}};
            }
            return {true, std::nullopt, it->second};
          } else if constexpr (std::is_same_v<T, LiteralExpr>) {
            if (node.literal.kind == TokenKind::BoolLiteral) {
              return {true, std::nullopt, Prim("bool")};
            }
            if (node.literal.kind == TokenKind::IntLiteral) {
              if (EndsWith(node.literal.lexeme, "usize")) {
                return {true, std::nullopt, Prim("usize")};
              }
              return {true, std::nullopt, Prim("i32")};
            }
            return {false, "ResolveExpr-Ident-Err", {}};
          }
          return {false, "ResolveExpr-Ident-Err", {}};
        },
        expr->node);
  }

  PlaceTypeResult PlaceOf(const ExprPtr& expr) const {
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
    SPEC_COV("T-Array-Literal-List");
    TypeEnv env;
    ArrayExpr expr;
    expr.elements.push_back(MakeIntLiteral("1"));
    expr.elements.push_back(MakeIntLiteral("2"));
    ExprTypeFn type_expr = [&](const auto& node) { return env.TypeOf(node); };
    const auto result = TypeArrayExpr(ctx, expr, type_expr);
    assert(result.ok);
    const auto* array = std::get_if<TypeArray>(&result.type->node);
    assert(array && array->length == 2);
    const auto* prim = std::get_if<TypePrim>(&array->element->node);
    assert(prim && prim->name == "i32");
  }

  {
    SPEC_COV("T-Index-Array");
    TypeEnv env;
    env.types["a"] = MakeTypeArray(Prim("i32"), 4);
    ExprTypeFn type_expr = [&](const auto& node) { return env.TypeOf(node); };
    auto access = MakeIndexAccess(MakeIdent("a"), MakeIntLiteral("1usize"));
    const auto result = TypeIndexAccessValue(ctx, access, type_expr);
    assert(result.ok);
    const auto* prim = std::get_if<TypePrim>(&result.type->node);
    assert(prim && prim->name == "i32");
  }

  {
    SPEC_COV("T-Index-Array-Perm");
    TypeEnv env;
    env.types["a"] = MakeTypePerm(Permission::Const,
                                  MakeTypeArray(Prim("i32"), 4));
    ExprTypeFn type_expr = [&](const auto& node) { return env.TypeOf(node); };
    auto access = MakeIndexAccess(MakeIdent("a"), MakeIntLiteral("0usize"));
    const auto result = TypeIndexAccessValue(ctx, access, type_expr);
    assert(result.ok);
    const auto* perm = std::get_if<TypePerm>(&result.type->node);
    assert(perm && perm->perm == Permission::Const);
    const auto* prim = std::get_if<TypePrim>(&perm->base->node);
    assert(prim && prim->name == "i32");
  }

  {
    SPEC_COV("Index-Array-NonConst-Err");
    TypeEnv env;
    env.types["a"] = MakeTypeArray(Prim("i32"), 4);
    env.types["idx"] = Prim("usize");
    ExprTypeFn type_expr = [&](const auto& node) { return env.TypeOf(node); };
    auto access = MakeIndexAccess(MakeIdent("a"), MakeIdent("idx"));
    const auto result = TypeIndexAccessValue(ctx, access, type_expr);
    assert(!result.ok);
    assert(result.diag_id ==
           std::optional<std::string_view>("Index-Array-NonConst-Err"));
  }

  {
    SPEC_COV("Index-Array-OOB-Err");
    TypeEnv env;
    env.types["a"] = MakeTypeArray(Prim("i32"), 2);
    ExprTypeFn type_expr = [&](const auto& node) { return env.TypeOf(node); };
    auto access = MakeIndexAccess(MakeIdent("a"), MakeIntLiteral("2usize"));
    const auto result = TypeIndexAccessValue(ctx, access, type_expr);
    assert(!result.ok);
    assert(result.diag_id ==
           std::optional<std::string_view>("Index-Array-OOB-Err"));
  }

  {
    SPEC_COV("Index-Array-NonUsize");
    TypeEnv env;
    env.types["a"] = MakeTypeArray(Prim("i32"), 2);
    ExprTypeFn type_expr = [&](const auto& node) { return env.TypeOf(node); };
    auto access = MakeIndexAccess(MakeIdent("a"), MakeIntLiteral("1"));
    const auto result = TypeIndexAccessValue(ctx, access, type_expr);
    assert(!result.ok);
    assert(result.diag_id ==
           std::optional<std::string_view>("Index-Array-NonUsize"));
  }

  {
    SPEC_COV("Index-NonIndexable");
    TypeEnv env;
    env.types["x"] = Prim("i32");
    ExprTypeFn type_expr = [&](const auto& node) { return env.TypeOf(node); };
    auto access = MakeIndexAccess(MakeIdent("x"), MakeIntLiteral("0usize"));
    const auto result = TypeIndexAccessValue(ctx, access, type_expr);
    assert(!result.ok);
    assert(result.diag_id ==
           std::optional<std::string_view>("Index-NonIndexable"));
  }

  {
    SPEC_COV("Index-Slice-Direct-Err");
    TypeEnv env;
    env.types["s"] = MakeTypeSlice(Prim("i32"));
    ExprTypeFn type_expr = [&](const auto& node) { return env.TypeOf(node); };
    auto access = MakeIndexAccess(MakeIdent("s"), MakeIntLiteral("0usize"));
    const auto result = TypeIndexAccessValue(ctx, access, type_expr);
    assert(!result.ok);
    assert(result.diag_id ==
           std::optional<std::string_view>("Index-Slice-Direct-Err"));
  }

  {
    SPEC_COV("Index-Slice-Perm-Direct-Err");
    TypeEnv env;
    env.types["s"] = MakeTypePerm(Permission::Const,
                                  MakeTypeSlice(Prim("i32")));
    ExprTypeFn type_expr = [&](const auto& node) { return env.TypeOf(node); };
    auto access = MakeIndexAccess(MakeIdent("s"), MakeIntLiteral("0usize"));
    const auto result = TypeIndexAccessValue(ctx, access, type_expr);
    assert(!result.ok);
    assert(result.diag_id ==
           std::optional<std::string_view>("Index-Slice-Direct-Err"));
  }

  {
    SPEC_COV("T-Slice-From-Array");
    TypeEnv env;
    env.types["a"] = MakeTypeArray(Prim("i32"), 4);
    ExprTypeFn type_expr = [&](const auto& node) { return env.TypeOf(node); };
    auto range = MakeRange(RangeKind::Full, nullptr, nullptr);
    auto access = MakeIndexAccess(MakeIdent("a"), range);
    const auto result = TypeIndexAccessValue(ctx, access, type_expr);
    assert(result.ok);
    const auto* slice = std::get_if<TypeSlice>(&result.type->node);
    assert(slice && std::holds_alternative<TypePrim>(slice->element->node));
  }

  {
    SPEC_COV("T-Slice-From-Array-Perm");
    TypeEnv env;
    env.types["a"] = MakeTypePerm(Permission::Const,
                                  MakeTypeArray(Prim("i32"), 4));
    ExprTypeFn type_expr = [&](const auto& node) { return env.TypeOf(node); };
    auto range = MakeRange(RangeKind::Full, nullptr, nullptr);
    auto access = MakeIndexAccess(MakeIdent("a"), range);
    const auto result = TypeIndexAccessValue(ctx, access, type_expr);
    assert(result.ok);
    const auto* perm = std::get_if<TypePerm>(&result.type->node);
    assert(perm && perm->perm == Permission::Const);
    const auto* slice = std::get_if<TypeSlice>(&perm->base->node);
    assert(slice && std::holds_alternative<TypePrim>(slice->element->node));
  }

  {
    SPEC_COV("T-Slice-From-Slice");
    TypeEnv env;
    env.types["s"] = MakeTypeSlice(Prim("i32"));
    ExprTypeFn type_expr = [&](const auto& node) { return env.TypeOf(node); };
    auto range = MakeRange(RangeKind::Full, nullptr, nullptr);
    auto access = MakeIndexAccess(MakeIdent("s"), range);
    const auto result = TypeIndexAccessValue(ctx, access, type_expr);
    assert(result.ok);
    const auto* slice = std::get_if<TypeSlice>(&result.type->node);
    assert(slice && std::holds_alternative<TypePrim>(slice->element->node));
  }

  {
    SPEC_COV("T-Slice-From-Slice-Perm");
    TypeEnv env;
    env.types["s"] = MakeTypePerm(Permission::Const,
                                  MakeTypeSlice(Prim("i32")));
    ExprTypeFn type_expr = [&](const auto& node) { return env.TypeOf(node); };
    auto range = MakeRange(RangeKind::Full, nullptr, nullptr);
    auto access = MakeIndexAccess(MakeIdent("s"), range);
    const auto result = TypeIndexAccessValue(ctx, access, type_expr);
    assert(result.ok);
    const auto* perm = std::get_if<TypePerm>(&result.type->node);
    assert(perm && perm->perm == Permission::Const);
    const auto* slice = std::get_if<TypeSlice>(&perm->base->node);
    assert(slice && std::holds_alternative<TypePrim>(slice->element->node));
  }

  {
    SPEC_COV("P-Index-Array");
    TypeEnv env;
    env.types["a"] = MakeTypeArray(Prim("i32"), 4);
    ExprTypeFn type_expr = [&](const auto& node) { return env.TypeOf(node); };
    PlaceTypeFn type_place = [&](const auto& node) { return env.PlaceOf(node); };
    auto access = MakeIndexAccess(MakeIdent("a"), MakeIntLiteral("1usize"));
    const auto result = TypeIndexAccessPlace(ctx, access, type_place, type_expr);
    assert(result.ok);
    const auto* prim = std::get_if<TypePrim>(&result.type->node);
    assert(prim && prim->name == "i32");
  }

  {
    SPEC_COV("P-Index-Array-Perm");
    TypeEnv env;
    env.types["a"] = MakeTypePerm(Permission::Const,
                                  MakeTypeArray(Prim("i32"), 4));
    ExprTypeFn type_expr = [&](const auto& node) { return env.TypeOf(node); };
    PlaceTypeFn type_place = [&](const auto& node) { return env.PlaceOf(node); };
    auto access = MakeIndexAccess(MakeIdent("a"), MakeIntLiteral("1usize"));
    const auto result = TypeIndexAccessPlace(ctx, access, type_place, type_expr);
    assert(result.ok);
    const auto* perm = std::get_if<TypePerm>(&result.type->node);
    assert(perm && perm->perm == Permission::Const);
    const auto* prim = std::get_if<TypePrim>(&perm->base->node);
    assert(prim && prim->name == "i32");
  }

  {
    SPEC_COV("P-Slice-From-Array");
    TypeEnv env;
    env.types["a"] = MakeTypeArray(Prim("i32"), 4);
    ExprTypeFn type_expr = [&](const auto& node) { return env.TypeOf(node); };
    PlaceTypeFn type_place = [&](const auto& node) { return env.PlaceOf(node); };
    auto range = MakeRange(RangeKind::Full, nullptr, nullptr);
    auto access = MakeIndexAccess(MakeIdent("a"), range);
    const auto result = TypeIndexAccessPlace(ctx, access, type_place, type_expr);
    assert(result.ok);
    const auto* slice = std::get_if<TypeSlice>(&result.type->node);
    assert(slice && std::holds_alternative<TypePrim>(slice->element->node));
  }

  {
    SPEC_COV("P-Slice-From-Array-Perm");
    TypeEnv env;
    env.types["a"] = MakeTypePerm(Permission::Const,
                                  MakeTypeArray(Prim("i32"), 4));
    ExprTypeFn type_expr = [&](const auto& node) { return env.TypeOf(node); };
    PlaceTypeFn type_place = [&](const auto& node) { return env.PlaceOf(node); };
    auto range = MakeRange(RangeKind::Full, nullptr, nullptr);
    auto access = MakeIndexAccess(MakeIdent("a"), range);
    const auto result = TypeIndexAccessPlace(ctx, access, type_place, type_expr);
    assert(result.ok);
    const auto* perm = std::get_if<TypePerm>(&result.type->node);
    assert(perm && perm->perm == Permission::Const);
    const auto* slice = std::get_if<TypeSlice>(&perm->base->node);
    assert(slice && std::holds_alternative<TypePrim>(slice->element->node));
  }

  {
    SPEC_COV("P-Slice-From-Slice");
    TypeEnv env;
    env.types["s"] = MakeTypeSlice(Prim("i32"));
    ExprTypeFn type_expr = [&](const auto& node) { return env.TypeOf(node); };
    PlaceTypeFn type_place = [&](const auto& node) { return env.PlaceOf(node); };
    auto range = MakeRange(RangeKind::Full, nullptr, nullptr);
    auto access = MakeIndexAccess(MakeIdent("s"), range);
    const auto result = TypeIndexAccessPlace(ctx, access, type_place, type_expr);
    assert(result.ok);
    const auto* slice = std::get_if<TypeSlice>(&result.type->node);
    assert(slice && std::holds_alternative<TypePrim>(slice->element->node));
  }

  {
    SPEC_COV("P-Slice-From-Slice-Perm");
    TypeEnv env;
    env.types["s"] = MakeTypePerm(Permission::Const,
                                  MakeTypeSlice(Prim("i32")));
    ExprTypeFn type_expr = [&](const auto& node) { return env.TypeOf(node); };
    PlaceTypeFn type_place = [&](const auto& node) { return env.PlaceOf(node); };
    auto range = MakeRange(RangeKind::Full, nullptr, nullptr);
    auto access = MakeIndexAccess(MakeIdent("s"), range);
    const auto result = TypeIndexAccessPlace(ctx, access, type_place, type_expr);
    assert(result.ok);
    const auto* perm = std::get_if<TypePerm>(&result.type->node);
    assert(perm && perm->perm == Permission::Const);
    const auto* slice = std::get_if<TypeSlice>(&perm->base->node);
    assert(slice && std::holds_alternative<TypePrim>(slice->element->node));
  }

  {
    SPEC_COV("Coerce-Array-Slice");
    const auto array = MakeTypeArray(Prim("i32"), 4);
    const auto perm_array = MakeTypePerm(Permission::Const, array);
    const auto result = CoerceArrayToSlice(ctx, perm_array);
    assert(result.ok);
    const auto* perm = std::get_if<TypePerm>(&result.type->node);
    assert(perm && perm->perm == Permission::Const);
    const auto* slice = std::get_if<TypeSlice>(&perm->base->node);
    assert(slice && std::holds_alternative<TypePrim>(slice->element->node));
  }

  return 0;
}
