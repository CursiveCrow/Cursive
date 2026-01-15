#include <cassert>
#include <optional>
#include <string>
#include <string_view>

#include "cursive0/core/assert_spec.h"
#include "cursive0/sema/literals.h"
#include "cursive0/sema/type_infer.h"
#include "cursive0/sema/types.h"
#include "cursive0/syntax/ast.h"

namespace {

using cursive0::sema::CheckLiteralExpr;
using cursive0::sema::ExprTypeFn;
using cursive0::sema::ExprTypeResult;
using cursive0::sema::InferExpr;
using cursive0::sema::PlaceTypeFn;
using cursive0::sema::PlaceTypeResult;
using cursive0::sema::MakeTypePrim;
using cursive0::sema::MakeTypeRawPtr;
using cursive0::sema::RawPtrQual;
using cursive0::sema::ScopeContext;
using cursive0::sema::StringState;
using cursive0::sema::TypePrim;
using cursive0::sema::TypeRef;
using cursive0::sema::TypeString;
using cursive0::sema::TypeLiteralExpr;
using cursive0::syntax::ExprPtr;
using cursive0::syntax::LiteralExpr;
using cursive0::syntax::Token;
using cursive0::syntax::TokenKind;

TypeRef Prim(const char* name) { return MakeTypePrim(name); }

LiteralExpr MakeLiteral(TokenKind kind, const char* lexeme) {
  Token tok;
  tok.kind = kind;
  tok.lexeme = lexeme;
  return LiteralExpr{tok};
}

ExprPtr MakeExpr(cursive0::syntax::ExprNode node) {
  auto expr = std::make_shared<cursive0::syntax::Expr>();
  expr->node = std::move(node);
  return expr;
}

ExprPtr MakeLiteralExpr(TokenKind kind, const char* lexeme) {
  return MakeExpr(MakeLiteral(kind, lexeme));
}

}  // namespace

int main() {
  ScopeContext ctx;

  {
    SPEC_COV("T-Int-Literal-Suffix");
    auto res = TypeLiteralExpr(ctx, MakeLiteral(TokenKind::IntLiteral, "42u16"));
    assert(res.ok);
    const auto* prim = std::get_if<TypePrim>(&res.type->node);
    assert(prim && prim->name == "u16");
  }

  {
    SPEC_COV("T-Int-Literal-Default");
    auto res = TypeLiteralExpr(ctx, MakeLiteral(TokenKind::IntLiteral, "123"));
    assert(res.ok);
    const auto* prim = std::get_if<TypePrim>(&res.type->node);
    assert(prim && prim->name == "i32");
  }

  {
    SPEC_COV("T-Float-Literal-Suffix");
    auto res =
        TypeLiteralExpr(ctx, MakeLiteral(TokenKind::FloatLiteral, "1.5f32"));
    assert(res.ok);
    const auto* prim = std::get_if<TypePrim>(&res.type->node);
    assert(prim && prim->name == "f32");
  }

  {
    SPEC_COV("T-Float-Literal-Default");
    auto res =
        TypeLiteralExpr(ctx, MakeLiteral(TokenKind::FloatLiteral, "1.5"));
    assert(res.ok);
    const auto* prim = std::get_if<TypePrim>(&res.type->node);
    assert(prim && prim->name == "f64");
  }

  {
    SPEC_COV("T-Bool-Literal");
    auto res = TypeLiteralExpr(ctx, MakeLiteral(TokenKind::BoolLiteral, "true"));
    assert(res.ok);
    const auto* prim = std::get_if<TypePrim>(&res.type->node);
    assert(prim && prim->name == "bool");
  }

  {
    SPEC_COV("T-Char-Literal");
    auto res = TypeLiteralExpr(ctx, MakeLiteral(TokenKind::CharLiteral, "'a'"));
    assert(res.ok);
    const auto* prim = std::get_if<TypePrim>(&res.type->node);
    assert(prim && prim->name == "char");
  }

  {
    SPEC_COV("T-String-Literal");
    auto res =
        TypeLiteralExpr(ctx, MakeLiteral(TokenKind::StringLiteral, "\"hi\""));
    assert(res.ok);
    const auto* str = std::get_if<TypeString>(&res.type->node);
    assert(str && str->state.has_value() && *str->state == StringState::View);
  }

  {
    SPEC_COV("Syn-Literal");
    ExprTypeFn type_expr = [&](const auto&) -> ExprTypeResult {
      return {false, std::nullopt, {}};
    };
    cursive0::sema::IdentTypeFn type_ident =
        [&](std::string_view) -> ExprTypeResult {
      return {false, std::nullopt, {}};
    };
    PlaceTypeFn type_place = [&](const auto&) -> PlaceTypeResult {
      return {false, std::nullopt, {}};
    };
    auto expr = MakeLiteralExpr(TokenKind::IntLiteral, "7");
    const auto res = InferExpr(ctx, expr, type_expr, type_place, type_ident);
    assert(res.ok);
    const auto* prim = std::get_if<TypePrim>(&res.type->node);
    assert(prim && prim->name == "i32");
  }

  {
    SPEC_COV("Chk-Int-Literal");
    auto res = CheckLiteralExpr(ctx, MakeLiteral(TokenKind::IntLiteral, "255"),
                                Prim("u8"));
    assert(res.ok);
  }

  {
    SPEC_COV("Chk-Float-Literal");
    auto res =
        CheckLiteralExpr(ctx, MakeLiteral(TokenKind::FloatLiteral, "1.0"),
                         Prim("f32"));
    assert(res.ok);
  }

  {
    SPEC_COV("Chk-Null-Literal");
    auto expected = MakeTypeRawPtr(RawPtrQual::Imm, Prim("i32"));
    auto res =
        CheckLiteralExpr(ctx, MakeLiteral(TokenKind::NullLiteral, "null"),
                         expected);
    assert(res.ok);
  }

  {
    SPEC_COV("Syn-Null-Literal-Err");
    ExprTypeFn type_expr = [&](const auto&) -> ExprTypeResult {
      return {false, std::nullopt, {}};
    };
    cursive0::sema::IdentTypeFn type_ident =
        [&](std::string_view) -> ExprTypeResult {
      return {false, std::nullopt, {}};
    };
    PlaceTypeFn type_place = [&](const auto&) -> PlaceTypeResult {
      return {false, std::nullopt, {}};
    };
    auto expr = MakeLiteralExpr(TokenKind::NullLiteral, "null");
    const auto res = InferExpr(ctx, expr, type_expr, type_place, type_ident);
    assert(!res.ok);
    assert(res.diag_id ==
           std::optional<std::string_view>("NullLiteral-Infer-Err"));
  }

  {
    SPEC_COV("Chk-Null-Literal-Err");
    auto res = CheckLiteralExpr(ctx, MakeLiteral(TokenKind::NullLiteral, "null"),
                                Prim("i32"));
    assert(!res.ok);
    assert(res.diag_id ==
           std::optional<std::string_view>("NullLiteral-Infer-Err"));
  }

  return 0;
}
