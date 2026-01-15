#include <cassert>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/sema/tuples.h"
#include "cursive0/sema/types.h"
#include "cursive0/syntax/ast.h"

namespace {

using cursive0::sema::ExprTypeFn;
using cursive0::sema::ExprTypeResult;
using cursive0::sema::MakeTypePerm;
using cursive0::sema::MakeTypePrim;
using cursive0::sema::MakeTypeTuple;
using cursive0::sema::Permission;
using cursive0::sema::PlaceTypeFn;
using cursive0::sema::PlaceTypeResult;
using cursive0::sema::ScopeContext;
using cursive0::sema::TypeRef;
using cursive0::sema::TypeTuple;
using cursive0::sema::TypeTupleAccessPlace;
using cursive0::sema::TypeTupleAccessValue;
using cursive0::sema::TypeTupleExpr;
using cursive0::syntax::IdentifierExpr;
using cursive0::syntax::LiteralExpr;
using cursive0::syntax::Token;
using cursive0::syntax::TokenKind;
using cursive0::syntax::TupleAccessExpr;
using cursive0::syntax::TupleExpr;

TypeRef Prim(const char* name) { return MakeTypePrim(name); }

cursive0::syntax::ExprPtr MakeExpr(cursive0::syntax::ExprNode node) {
  auto expr = std::make_shared<cursive0::syntax::Expr>();
  expr->node = std::move(node);
  return expr;
}

cursive0::syntax::ExprPtr MakeIdent(const char* name) {
  return MakeExpr(IdentifierExpr{std::string(name)});
}

cursive0::syntax::ExprPtr MakeIntLiteral(const char* lexeme) {
  Token tok;
  tok.kind = TokenKind::IntLiteral;
  tok.lexeme = lexeme;
  return MakeExpr(LiteralExpr{tok});
}

cursive0::syntax::ExprPtr MakeBoolLiteral(bool value) {
  Token tok;
  tok.kind = TokenKind::BoolLiteral;
  tok.lexeme = value ? "true" : "false";
  return MakeExpr(LiteralExpr{tok});
}

TupleAccessExpr MakeTupleAccess(const cursive0::syntax::ExprPtr& base,
                                TokenKind kind,
                                const char* lexeme) {
  Token tok;
  tok.kind = kind;
  tok.lexeme = lexeme;
  return TupleAccessExpr{base, tok};
}

struct TypeEnv {
  std::unordered_map<std::string, TypeRef> types;

  ExprTypeResult TypeOf(const cursive0::syntax::ExprPtr& expr) const {
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
            return {true, std::nullopt, Prim("i32")};
          }
          return {false, "ResolveExpr-Ident-Err", {}};
        },
        expr->node);
  }

  PlaceTypeResult PlaceOf(const cursive0::syntax::ExprPtr& expr) const {
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
    SPEC_COV("T-Unit-Literal");
    TypeEnv env;
    TupleExpr expr;
    ExprTypeFn type_expr = [&](const auto& node) { return env.TypeOf(node); };
    const auto result = TypeTupleExpr(ctx, expr, type_expr);
    assert(result.ok);
    assert(result.type);
    const auto* prim = std::get_if<cursive0::sema::TypePrim>(&result.type->node);
    assert(prim && prim->name == "()");
  }

  {
    SPEC_COV("T-Tuple-Literal");
    TypeEnv env;
    TupleExpr expr;
    expr.elements.push_back(MakeIntLiteral("1"));
    expr.elements.push_back(MakeBoolLiteral(true));
    ExprTypeFn type_expr = [&](const auto& node) { return env.TypeOf(node); };
    const auto result = TypeTupleExpr(ctx, expr, type_expr);
    assert(result.ok);
    const auto* tuple = std::get_if<TypeTuple>(&result.type->node);
    assert(tuple && tuple->elements.size() == 2);
  }

  {
    SPEC_COV("T-Tuple-Index");
    TypeEnv env;
    env.types["t"] = MakeTypeTuple({Prim("i32"), Prim("bool")});
    ExprTypeFn type_expr = [&](const auto& node) { return env.TypeOf(node); };
    auto access = MakeTupleAccess(MakeIdent("t"), TokenKind::IntLiteral, "0");
    const auto result = TypeTupleAccessValue(ctx, access, type_expr);
    assert(result.ok);
    const auto* prim = std::get_if<cursive0::sema::TypePrim>(&result.type->node);
    assert(prim && prim->name == "i32");
  }

  {
    SPEC_COV("T-Tuple-Index-Perm");
    TypeEnv env;
    const auto tuple = MakeTypeTuple({Prim("i32"), Prim("bool")});
    env.types["p"] = MakeTypePerm(Permission::Const, tuple);
    ExprTypeFn type_expr = [&](const auto& node) { return env.TypeOf(node); };
    auto access = MakeTupleAccess(MakeIdent("p"), TokenKind::IntLiteral, "1");
    const auto result = TypeTupleAccessValue(ctx, access, type_expr);
    assert(result.ok);
    const auto* perm = std::get_if<cursive0::sema::TypePerm>(&result.type->node);
    assert(perm && perm->perm == Permission::Const);
    const auto* prim = std::get_if<cursive0::sema::TypePrim>(&perm->base->node);
    assert(prim && prim->name == "bool");
  }

  {
    SPEC_COV("P-Tuple-Index");
    TypeEnv env;
    env.types["t"] = MakeTypeTuple({Prim("i32"), Prim("bool")});
    PlaceTypeFn type_place = [&](const auto& node) { return env.PlaceOf(node); };
    auto access = MakeTupleAccess(MakeIdent("t"), TokenKind::IntLiteral, "0");
    const auto result = TypeTupleAccessPlace(ctx, access, type_place);
    assert(result.ok);
    const auto* prim = std::get_if<cursive0::sema::TypePrim>(&result.type->node);
    assert(prim && prim->name == "i32");
  }

  {
    SPEC_COV("P-Tuple-Index-Perm");
    TypeEnv env;
    const auto tuple = MakeTypeTuple({Prim("i32"), Prim("bool")});
    env.types["p"] = MakeTypePerm(Permission::Const, tuple);
    PlaceTypeFn type_place = [&](const auto& node) { return env.PlaceOf(node); };
    auto access = MakeTupleAccess(MakeIdent("p"), TokenKind::IntLiteral, "1");
    const auto result = TypeTupleAccessPlace(ctx, access, type_place);
    assert(result.ok);
    const auto* perm = std::get_if<cursive0::sema::TypePerm>(&result.type->node);
    assert(perm && perm->perm == Permission::Const);
    const auto* prim = std::get_if<cursive0::sema::TypePrim>(&perm->base->node);
    assert(prim && prim->name == "bool");
  }

  {
    SPEC_COV("TupleIndex-NonConst");
    TypeEnv env;
    env.types["t"] = MakeTypeTuple({Prim("i32"), Prim("bool")});
    ExprTypeFn type_expr = [&](const auto& node) { return env.TypeOf(node); };
    auto access = MakeTupleAccess(MakeIdent("t"), TokenKind::Identifier, "x");
    const auto result = TypeTupleAccessValue(ctx, access, type_expr);
    assert(!result.ok);
    assert(result.diag_id == std::optional<std::string_view>("TupleIndex-NonConst"));
  }

  {
    SPEC_COV("TupleIndex-OOB");
    TypeEnv env;
    env.types["t"] = MakeTypeTuple({Prim("i32"), Prim("bool")});
    ExprTypeFn type_expr = [&](const auto& node) { return env.TypeOf(node); };
    auto access = MakeTupleAccess(MakeIdent("t"), TokenKind::IntLiteral, "2");
    const auto result = TypeTupleAccessValue(ctx, access, type_expr);
    assert(!result.ok);
    assert(result.diag_id == std::optional<std::string_view>("TupleIndex-OOB"));
  }

  {
    SPEC_COV("TupleAccess-NotTuple");
    TypeEnv env;
    env.types["x"] = Prim("i32");
    ExprTypeFn type_expr = [&](const auto& node) { return env.TypeOf(node); };
    auto access = MakeTupleAccess(MakeIdent("x"), TokenKind::IntLiteral, "0");
    const auto result = TypeTupleAccessValue(ctx, access, type_expr);
    assert(!result.ok);
    assert(result.diag_id == std::optional<std::string_view>("TupleAccess-NotTuple"));
  }

  return 0;
}
