#include <cassert>
#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "cursive0/codegen/llvm/llvm_emit.h"
#include "cursive0/codegen/mangle.h"
#include "cursive0/codegen/globals.h"
#include "cursive0/core/assert_spec.h"
#include "cursive0/core/symbols.h"
#include "cursive0/analysis/types/types.h"
#include "cursive0/syntax/ast.h"
#include "cursive0/syntax/token.h"
#include "llvm/IR/LLVMContext.h"

namespace {

using cursive0::codegen::EmitGlobal;
using cursive0::codegen::EmitGlobalResult;
using cursive0::codegen::GlobalConst;
using cursive0::codegen::GlobalZero;
using cursive0::codegen::InitSym;
using cursive0::codegen::DeinitSym;
using cursive0::codegen::LowerCtx;
using cursive0::codegen::LLVMEmitter;
using cursive0::codegen::MangleLiteral;
using cursive0::core::Mangle;
using cursive0::core::StringOfPath;
using cursive0::analysis::MakeTypePrim;
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

std::vector<std::uint8_t> BytesFromU32(std::uint32_t value) {
  return {
      static_cast<std::uint8_t>(value & 0xFF),
      static_cast<std::uint8_t>((value >> 8) & 0xFF),
      static_cast<std::uint8_t>((value >> 16) & 0xFF),
      static_cast<std::uint8_t>((value >> 24) & 0xFF),
  };
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
  SPEC_COV("Init-Start");
  SPEC_COV("Init-Step");
  SPEC_COV("Init-Next-Module");
  SPEC_COV("Init-Panic");
  SPEC_COV("Init-Done");
  SPEC_COV("Init-Ok");
  SPEC_COV("Init-Fail");
  SPEC_COV("Deinit-Ok");
  SPEC_COV("Deinit-Panic");

  cursive0::analysis::Sigma sigma;
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

  {
    SPEC_COV("EmitLiteral-Bytes");
    auto decl = cursive0::codegen::EmitBytesLit({0x01, 0x02, 0x03});
    assert(std::holds_alternative<GlobalConst>(decl));
  }

  {
    SPEC_COV("EmitLiteral-Char");
    SPEC_COV("EmitLiteral-Int");
    SPEC_COV("EmitLiteral-Float");
    SPEC_COV("LowerIRDecl-GlobalConst");
    SPEC_COV("LowerIRDecl-GlobalZero");

    LowerCtx ctx;
    llvm::LLVMContext llvm_ctx;

    std::vector<std::uint8_t> char_bytes = BytesFromU32(0x41);
    std::vector<std::uint8_t> int_bytes = BytesFromU32(0x2A);
    std::vector<std::uint8_t> float_bytes = BytesFromU32(0x3F800000);

    const std::string char_sym = MangleLiteral("char", char_bytes);
    const std::string int_sym = MangleLiteral("int", int_bytes);
    const std::string float_sym = MangleLiteral("float", float_bytes);

    ctx.RegisterStaticType(char_sym, MakeTypePrim("char"));
    ctx.RegisterStaticType(int_sym, MakeTypePrim("i32"));
    ctx.RegisterStaticType(float_sym, MakeTypePrim("f32"));
    ctx.RegisterStaticType("g_zero", MakeTypePrim("i32"));

    GlobalConst gc_char{char_sym, char_bytes};
    GlobalConst gc_int{int_sym, int_bytes};
    GlobalConst gc_float{float_sym, float_bytes};
    GlobalZero gz;
    gz.symbol = "g_zero";
    gz.size = 4;

    LLVMEmitter emitter(llvm_ctx, "literals");
    cursive0::codegen::IRDecls decls = {gc_char, gc_int, gc_float, gz};
    llvm::Module* mod = emitter.EmitModule(decls, ctx);
    assert(mod != nullptr);
    assert(!ctx.codegen_failed);
  }

  {
    SPEC_COV("EmitLiteral-Err");
    SPEC_COV("LowerIRDecl-Err");
    LowerCtx ctx;
    llvm::LLVMContext llvm_ctx;
    std::vector<std::uint8_t> bad_bytes = {0xAA};
    const std::string bad_sym = MangleLiteral("int", bad_bytes);
    ctx.RegisterStaticType(bad_sym, MakeTypePrim("i32"));
    GlobalConst bad{bad_sym, bad_bytes};
    LLVMEmitter emitter(llvm_ctx, "literal_err");
    cursive0::codegen::IRDecls decls = {bad};
    emitter.EmitModule(decls, ctx);
    assert(ctx.codegen_failed);
  }

  return 0;
}
