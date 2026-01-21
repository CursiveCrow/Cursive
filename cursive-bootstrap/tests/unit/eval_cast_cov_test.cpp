#include <cassert>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/int128.h"
#include "cursive0/analysis/types/context.h"
#include "cursive0/analysis/types/types.h"
#include "cursive0/eval/control.h"
#include "cursive0/eval/eval.h"
#include "cursive0/eval/value.h"
#include "cursive0/syntax/ast.h"
#include "cursive0/syntax/token.h"

namespace {

using cursive0::core::UInt128FromU64;
using cursive0::eval::FloatVal;
using cursive0::eval::IntVal;
using cursive0::eval::IsVal;
using cursive0::eval::Outcome;
using cursive0::eval::Sigma;
using cursive0::eval::Value;
using cursive0::eval::ValueEqual;
using cursive0::syntax::CastExpr;
using cursive0::syntax::Expr;
using cursive0::syntax::ExprPtr;
using cursive0::syntax::LiteralExpr;
using cursive0::syntax::Token;
using cursive0::syntax::TokenKind;
using cursive0::syntax::Type;
using cursive0::syntax::TypePrim;

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

static ExprPtr MakeCastExpr(ExprPtr value, std::shared_ptr<Type> type) {
  CastExpr cast;
  cast.value = std::move(value);
  cast.type = std::move(type);
  return MakeExpr(std::move(cast));
}

static std::shared_ptr<Type> MakePrimType(std::string name) {
  auto type = std::make_shared<Type>();
  type->span = EmptySpan();
  type->node = TypePrim{std::move(name)};
  return type;
}

static Value MakeInt(std::string type, std::uint64_t value) {
  IntVal v;
  v.type = std::move(type);
  v.negative = false;
  v.magnitude = UInt128FromU64(value);
  return Value{v};
}

static Value MakeBool(bool value) {
  return Value{cursive0::eval::BoolVal{value}};
}

static void SetExprType(cursive0::analysis::ExprTypeMap& map,
                        const ExprPtr& expr,
                        cursive0::analysis::TypeRef type) {
  map[expr.get()] = std::move(type);
}

void TestCastValCoverage() {
  SPEC_COV("CastVal-Id");
  SPEC_COV("CastVal-Int-Int-Signed");
  SPEC_COV("CastVal-Int-Int-Unsigned");
  SPEC_COV("CastVal-Int-Float");
  SPEC_COV("CastVal-Float-Float");
  SPEC_COV("CastVal-Float-Int");
  SPEC_COV("CastVal-Bool-Int");
  SPEC_COV("CastVal-Int-Bool");
  SPEC_COV("CastVal-Char-U32");
  SPEC_COV("CastVal-U32-Char");

  Sigma sigma;
  cursive0::analysis::ScopeContext sema_ctx;
  cursive0::analysis::ExprTypeMap expr_types;
  sema_ctx.expr_types = &expr_types;

  cursive0::eval::SemanticsContext ctx;
  ctx.sema = &sema_ctx;

  // CastVal-Id: i32 -> i32
  auto lit_id = MakeLiteralExpr(TokenKind::IntLiteral, "3");
  auto cast_id = MakeCastExpr(lit_id, MakePrimType("i32"));
  SetExprType(expr_types, lit_id, cursive0::analysis::MakeTypePrim("i32"));
  SetExprType(expr_types, cast_id, cursive0::analysis::MakeTypePrim("i32"));
  Outcome id_out = cursive0::eval::EvalSigma(ctx, *cast_id, sigma);
  assert(IsVal(id_out));
  assert(ValueEqual(std::get<Value>(id_out.node), MakeInt("i32", 3)));

  // CastVal-Int-Int-Signed: i32 -> i64
  auto lit_signed = MakeLiteralExpr(TokenKind::IntLiteral, "4");
  auto cast_signed = MakeCastExpr(lit_signed, MakePrimType("i64"));
  SetExprType(expr_types, lit_signed, cursive0::analysis::MakeTypePrim("i32"));
  SetExprType(expr_types, cast_signed, cursive0::analysis::MakeTypePrim("i64"));
  Outcome signed_out = cursive0::eval::EvalSigma(ctx, *cast_signed, sigma);
  assert(IsVal(signed_out));
  assert(ValueEqual(std::get<Value>(signed_out.node), MakeInt("i64", 4)));

  // CastVal-Int-Int-Unsigned: i32 -> u32
  auto lit_unsigned = MakeLiteralExpr(TokenKind::IntLiteral, "5");
  auto cast_unsigned = MakeCastExpr(lit_unsigned, MakePrimType("u32"));
  SetExprType(expr_types, lit_unsigned, cursive0::analysis::MakeTypePrim("i32"));
  SetExprType(expr_types, cast_unsigned, cursive0::analysis::MakeTypePrim("u32"));
  Outcome unsigned_out = cursive0::eval::EvalSigma(ctx, *cast_unsigned, sigma);
  assert(IsVal(unsigned_out));
  assert(ValueEqual(std::get<Value>(unsigned_out.node), MakeInt("u32", 5)));

  // CastVal-Int-Float: i32 -> f32
  auto lit_int_float = MakeLiteralExpr(TokenKind::IntLiteral, "6");
  auto cast_int_float = MakeCastExpr(lit_int_float, MakePrimType("f32"));
  SetExprType(expr_types, lit_int_float, cursive0::analysis::MakeTypePrim("i32"));
  SetExprType(expr_types, cast_int_float, cursive0::analysis::MakeTypePrim("f32"));
  Outcome int_float_out = cursive0::eval::EvalSigma(ctx, *cast_int_float, sigma);
  assert(IsVal(int_float_out));
  const auto* int_float_val = std::get_if<FloatVal>(&std::get<Value>(int_float_out.node).node);
  assert(int_float_val && int_float_val->type == "f32");

  // CastVal-Float-Float: f32 -> f64
  auto lit_float = MakeLiteralExpr(TokenKind::FloatLiteral, "1.5");
  auto cast_float = MakeCastExpr(lit_float, MakePrimType("f64"));
  SetExprType(expr_types, lit_float, cursive0::analysis::MakeTypePrim("f32"));
  SetExprType(expr_types, cast_float, cursive0::analysis::MakeTypePrim("f64"));
  Outcome float_out = cursive0::eval::EvalSigma(ctx, *cast_float, sigma);
  assert(IsVal(float_out));
  const auto* float_val = std::get_if<FloatVal>(&std::get<Value>(float_out.node).node);
  assert(float_val && float_val->type == "f64");

  // CastVal-Float-Int: f32 -> i32
  auto lit_float_int = MakeLiteralExpr(TokenKind::FloatLiteral, "2.0");
  auto cast_float_int = MakeCastExpr(lit_float_int, MakePrimType("i32"));
  SetExprType(expr_types, lit_float_int, cursive0::analysis::MakeTypePrim("f32"));
  SetExprType(expr_types, cast_float_int, cursive0::analysis::MakeTypePrim("i32"));
  Outcome float_int_out = cursive0::eval::EvalSigma(ctx, *cast_float_int, sigma);
  assert(IsVal(float_int_out));
  assert(ValueEqual(std::get<Value>(float_int_out.node), MakeInt("i32", 2)));

  // CastVal-Bool-Int: bool -> i32
  auto lit_bool = MakeLiteralExpr(TokenKind::BoolLiteral, "true");
  auto cast_bool_int = MakeCastExpr(lit_bool, MakePrimType("i32"));
  SetExprType(expr_types, lit_bool, cursive0::analysis::MakeTypePrim("bool"));
  SetExprType(expr_types, cast_bool_int, cursive0::analysis::MakeTypePrim("i32"));
  Outcome bool_int_out = cursive0::eval::EvalSigma(ctx, *cast_bool_int, sigma);
  assert(IsVal(bool_int_out));
  assert(ValueEqual(std::get<Value>(bool_int_out.node), MakeInt("i32", 1)));

  // CastVal-Int-Bool: i32 -> bool
  auto lit_int_bool = MakeLiteralExpr(TokenKind::IntLiteral, "1");
  auto cast_int_bool = MakeCastExpr(lit_int_bool, MakePrimType("bool"));
  SetExprType(expr_types, lit_int_bool, cursive0::analysis::MakeTypePrim("i32"));
  SetExprType(expr_types, cast_int_bool, cursive0::analysis::MakeTypePrim("bool"));
  Outcome int_bool_out = cursive0::eval::EvalSigma(ctx, *cast_int_bool, sigma);
  assert(IsVal(int_bool_out));
  assert(ValueEqual(std::get<Value>(int_bool_out.node), MakeBool(true)));

  // CastVal-Char-U32: char -> u32
  auto lit_char = MakeLiteralExpr(TokenKind::CharLiteral, "'A'");
  auto cast_char_u32 = MakeCastExpr(lit_char, MakePrimType("u32"));
  SetExprType(expr_types, lit_char, cursive0::analysis::MakeTypePrim("char"));
  SetExprType(expr_types, cast_char_u32, cursive0::analysis::MakeTypePrim("u32"));
  Outcome char_u32_out = cursive0::eval::EvalSigma(ctx, *cast_char_u32, sigma);
  assert(IsVal(char_u32_out));
  assert(ValueEqual(std::get<Value>(char_u32_out.node), MakeInt("u32", 65)));

  // CastVal-U32-Char: u32 -> char
  auto lit_u32 = MakeLiteralExpr(TokenKind::IntLiteral, "66");
  auto cast_u32_char = MakeCastExpr(lit_u32, MakePrimType("char"));
  SetExprType(expr_types, lit_u32, cursive0::analysis::MakeTypePrim("u32"));
  SetExprType(expr_types, cast_u32_char, cursive0::analysis::MakeTypePrim("char"));
  Outcome u32_char_out = cursive0::eval::EvalSigma(ctx, *cast_u32_char, sigma);
  assert(IsVal(u32_char_out));
  const auto* char_val = std::get_if<cursive0::eval::CharVal>(
      &std::get<Value>(u32_char_out.node).node);
  assert(char_val && char_val->value == 66);
}

}  // namespace

int main() {
  TestCastValCoverage();
  return 0;
}
