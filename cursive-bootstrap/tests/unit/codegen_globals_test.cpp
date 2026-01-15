#include <cassert>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "cursive0/codegen/globals.h"
#include "cursive0/core/assert_spec.h"
#include "cursive0/core/symbols.h"
#include "cursive0/sema/types.h"
#include "cursive0/syntax/ast.h"
#include "cursive0/syntax/token.h"

namespace {

using cursive0::codegen::EmitGlobal;
using cursive0::codegen::EmitGlobalResult;
using cursive0::codegen::GlobalConst;
using cursive0::codegen::GlobalZero;
using cursive0::codegen::InitSym;
using cursive0::codegen::DeinitSym;
using cursive0::codegen::LowerCtx;
using cursive0::core::Mangle;
using cursive0::core::StringOfPath;
using cursive0::sema::MakeTypePrim;
using cursive0::syntax::Binding;
using cursive0::syntax::BinaryExpr;
using cursive0::syntax::Expr;
using cursive0::syntax::IdentifierPattern;
using cursive0::syntax::LiteralExpr;
using cursive0::syntax::Mutability;
using cursive0::syntax::Pattern;
using cursive0::syntax::StaticDecl;
using cursive0::syntax::Token;
using cursive0::syntax::TokenKind;
using cursive0::syntax::TuplePattern;
using cursive0::syntax::Type;
using cursive0::syntax::TypePrim;
using cursive0::syntax::TypeTuple;
using cursive0::syntax::Visibility;

Token MakeToken(TokenKind kind, const char* lexeme) {
  Token tok;
  tok.kind = kind;
  tok.lexeme = std::string(lexeme);
  return tok;
}

std::shared_ptr<Expr> MakeLiteralExpr(TokenKind kind, const char* lexeme) {
  auto expr = std::make_shared<Expr>();
  LiteralExpr lit;
  lit.literal = MakeToken(kind, lexeme);
  expr->node = lit;
  return expr;
}

std::shared_ptr<Expr> MakeBinaryExpr(const char* op,
                                     std::shared_ptr<Expr> lhs,
                                     std::shared_ptr<Expr> rhs) {
  auto expr = std::make_shared<Expr>();
  BinaryExpr bin;
  bin.op = std::string(op);
  bin.lhs = std::move(lhs);
  bin.rhs = std::move(rhs);
  expr->node = std::move(bin);
  return expr;
}

std::shared_ptr<Pattern> MakeIdentPattern(const char* name) {
  auto pat = std::make_shared<Pattern>();
  IdentifierPattern ident;
  ident.name = std::string(name);
  pat->node = std::move(ident);
  return pat;
}

std::shared_ptr<Pattern> MakeTuplePattern(
    std::vector<std::shared_ptr<Pattern>> elements) {
  auto pat = std::make_shared<Pattern>();
  TuplePattern tuple;
  tuple.elements = std::move(elements);
  pat->node = std::move(tuple);
  return pat;
}

std::shared_ptr<Type> MakePrimType(const char* name) {
  auto type = std::make_shared<Type>();
  TypePrim prim;
  prim.name = std::string(name);
  type->node = std::move(prim);
  return type;
}

std::shared_ptr<Type> MakeTupleType(std::vector<std::shared_ptr<Type>> elems) {
  auto type = std::make_shared<Type>();
  TypeTuple tuple;
  tuple.elements = std::move(elems);
  type->node = std::move(tuple);
  return type;
}

StaticDecl MakeStatic(const char* name,
                      std::shared_ptr<Type> type_opt,
                      std::shared_ptr<Expr> init) {
  Binding binding;
  binding.pat = MakeIdentPattern(name);
  binding.type_opt = std::move(type_opt);
  binding.op = MakeToken(TokenKind::Operator, "=");
  binding.init = std::move(init);

  StaticDecl decl;
  decl.vis = Visibility::Internal;
  decl.mut = Mutability::Let;
  decl.binding = std::move(binding);
  return decl;
}

}  // namespace

int main() {
  SPEC_COV("Emit-Static-Const");
  SPEC_COV("Emit-Static-Init");
  SPEC_COV("Emit-Static-Multi");
  SPEC_COV("ConstInit");
  SPEC_COV("InitFn");
  SPEC_COV("DeinitFn");

  cursive0::sema::Sigma sigma;
  LowerCtx ctx;
  ctx.sigma = &sigma;

  {
    auto lit = MakeLiteralExpr(TokenKind::IntLiteral, "42");
    StaticDecl decl = MakeStatic("x", MakePrimType("i32"), lit);
    ctx.expr_type = [](const Expr&) { return MakeTypePrim("i32"); };

    EmitGlobalResult result = EmitGlobal(decl, {"m"}, ctx);
    assert(result.decls.size() == 1);
    assert(std::holds_alternative<GlobalConst>(result.decls[0]));
    const auto& gc = std::get<GlobalConst>(result.decls[0]);
    assert(gc.bytes.size() == 4);
    assert(gc.bytes[0] == 0x2A);
    assert(!result.needs_runtime_init);
  }

  {
    auto lhs = MakeLiteralExpr(TokenKind::IntLiteral, "1");
    auto rhs = MakeLiteralExpr(TokenKind::IntLiteral, "2");
    auto bin = MakeBinaryExpr("+", lhs, rhs);
    StaticDecl decl = MakeStatic("y", MakePrimType("i32"), bin);
    ctx.expr_type = [](const Expr&) { return MakeTypePrim("i32"); };

    EmitGlobalResult result = EmitGlobal(decl, {"m"}, ctx);
    assert(result.decls.size() == 1);
    assert(std::holds_alternative<GlobalZero>(result.decls[0]));
    const auto& gz = std::get<GlobalZero>(result.decls[0]);
    assert(gz.size == 4);
    assert(result.needs_runtime_init);
  }

  {
    Binding binding;
    binding.pat = MakeTuplePattern({MakeIdentPattern("a"),
                                    MakeIdentPattern("b")});
    binding.type_opt = MakeTupleType({MakePrimType("i32"),
                                      MakePrimType("u8")});
    binding.op = MakeToken(TokenKind::Operator, "=");
    binding.init = nullptr;

    StaticDecl decl;
    decl.vis = Visibility::Internal;
    decl.mut = Mutability::Let;
    decl.binding = std::move(binding);

    ctx.expr_type = {};
    EmitGlobalResult result = EmitGlobal(decl, {"m"}, ctx);
    assert(result.decls.size() == 2);
    assert(std::holds_alternative<GlobalZero>(result.decls[0]));
    assert(std::holds_alternative<GlobalZero>(result.decls[1]));
    const auto& g0 = std::get<GlobalZero>(result.decls[0]);
    const auto& g1 = std::get<GlobalZero>(result.decls[1]);
    assert(g0.size == 4);
    assert(g1.size == 1);
    assert(result.needs_runtime_init);
  }

  {
    cursive0::syntax::ModulePath module_path = {"foo", "bar"};
    std::vector<std::string> init_path = {"cursive", "runtime", "init", "foo", "bar"};
    std::vector<std::string> deinit_path = {"cursive", "runtime", "deinit", "foo", "bar"};
    assert(InitSym(module_path) == Mangle(StringOfPath(init_path)));
    assert(DeinitSym(module_path) == Mangle(StringOfPath(deinit_path)));
  }

  return 0;
}
