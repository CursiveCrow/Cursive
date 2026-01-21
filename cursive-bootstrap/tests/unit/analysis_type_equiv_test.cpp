#include <cassert>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/analysis/types/type_equiv.h"
#include "cursive0/analysis/types/types.h"

namespace {

using cursive0::core::Span;
using cursive0::analysis::BytesState;
using cursive0::analysis::ConstLen;
using cursive0::analysis::ConstLenResult;
using cursive0::analysis::MakeTypeArray;
using cursive0::analysis::MakeTypeBytes;
using cursive0::analysis::MakeTypeDynamic;
using cursive0::analysis::MakeTypeFunc;
using cursive0::analysis::MakeTypeModalState;
using cursive0::analysis::MakeTypePath;
using cursive0::analysis::MakeTypePerm;
using cursive0::analysis::MakeTypePrim;
using cursive0::analysis::MakeTypePtr;
using cursive0::analysis::MakeTypeRange;
using cursive0::analysis::MakeTypeRawPtr;
using cursive0::analysis::MakeTypeSlice;
using cursive0::analysis::MakeTypeString;
using cursive0::analysis::MakeTypeTuple;
using cursive0::analysis::MakeTypeUnion;
using cursive0::analysis::ParamMode;
using cursive0::analysis::Permission;
using cursive0::analysis::PtrState;
using cursive0::analysis::RawPtrQual;
using cursive0::analysis::ScopeContext;
using cursive0::analysis::StringState;
using cursive0::analysis::TypeEquiv;
using cursive0::analysis::TypeFuncParam;
using cursive0::analysis::TypePath;
using cursive0::analysis::TypeRef;
using cursive0::syntax::Expr;
using cursive0::syntax::ExprPtr;
using cursive0::syntax::IdentifierPattern;
using cursive0::syntax::LiteralExpr;
using cursive0::syntax::ModulePath;
using cursive0::syntax::PathExpr;
using cursive0::syntax::Pattern;
using cursive0::syntax::PatternPtr;
using cursive0::syntax::StaticDecl;
using cursive0::syntax::Token;
using cursive0::syntax::TokenKind;

TypeRef Prim(const char* name) { return MakeTypePrim(name); }

ExprPtr MakeIntLit(const char* text) {
  Expr expr;
  expr.span = Span{};
  LiteralExpr lit;
  lit.literal = Token{TokenKind::IntLiteral, text, Span{}};
  expr.node = lit;
  return std::make_shared<Expr>(std::move(expr));
}

ExprPtr MakePathExpr(const ModulePath& path, const char* name) {
  Expr expr;
  expr.span = Span{};
  PathExpr path_expr;
  path_expr.path = path;
  path_expr.name = name;
  expr.node = path_expr;
  return std::make_shared<Expr>(std::move(expr));
}

PatternPtr MakeIdentPattern(const char* name) {
  Pattern pattern;
  pattern.span = Span{};
  IdentifierPattern ident;
  ident.name = name;
  pattern.node = ident;
  return std::make_shared<Pattern>(std::move(pattern));
}

StaticDecl MakeStatic(const char* name, ExprPtr init) {
  StaticDecl decl;
  decl.vis = cursive0::syntax::Visibility::Private;
  decl.mut = cursive0::syntax::Mutability::Let;
  decl.binding.pat = MakeIdentPattern(name);
  decl.binding.type_opt = nullptr;
  decl.binding.op = Token{TokenKind::Operator, "=", Span{}};
  decl.binding.init = std::move(init);
  decl.binding.span = Span{};
  decl.span = Span{};
  return decl;
}

}  // namespace

int main() {
  {
    SPEC_COV("T-Equiv-Prim");
    const auto res = TypeEquiv(Prim("i32"), Prim("i32"));
    assert(res.ok && res.equiv);
  }
  {
    SPEC_COV("T-Equiv-Prim");
    const auto res = TypeEquiv(Prim("i32"), Prim("u32"));
    assert(res.ok && !res.equiv);
  }
  {
    SPEC_COV("T-Equiv-Perm");
    const auto lhs = MakeTypePerm(Permission::Const, Prim("i32"));
    const auto rhs = MakeTypePerm(Permission::Unique, Prim("i32"));
    const auto res = TypeEquiv(lhs, rhs);
    assert(res.ok && !res.equiv);
  }
  {
    SPEC_COV("T-Equiv-Tuple");
    const auto lhs = MakeTypeTuple({Prim("i32"), Prim("bool")});
    const auto rhs = MakeTypeTuple({Prim("i32"), Prim("bool")});
    const auto res = TypeEquiv(lhs, rhs);
    assert(res.ok && res.equiv);
  }
  {
    SPEC_COV("T-Equiv-Array");
    const auto lhs = MakeTypeArray(Prim("u8"), 4);
    const auto rhs = MakeTypeArray(Prim("u8"), 8);
    const auto res = TypeEquiv(lhs, rhs);
    assert(res.ok && !res.equiv);
  }
  {
    SPEC_COV("T-Equiv-Slice");
    const auto lhs = MakeTypeSlice(Prim("u8"));
    const auto rhs = MakeTypeSlice(Prim("u8"));
    const auto res = TypeEquiv(lhs, rhs);
    assert(res.ok && res.equiv);
  }
  {
    SPEC_COV("T-Equiv-Func");
    std::vector<TypeFuncParam> params;
    params.push_back(TypeFuncParam{ParamMode::Move, Prim("i32")});
    const auto lhs = MakeTypeFunc(std::move(params), Prim("()"));
    std::vector<TypeFuncParam> params2;
    params2.push_back(TypeFuncParam{ParamMode::Move, Prim("i32")});
    const auto rhs = MakeTypeFunc(std::move(params2), Prim("()"));
    const auto res = TypeEquiv(lhs, rhs);
    assert(res.ok && res.equiv);
  }
  {
    SPEC_COV("T-Equiv-Union");
    const auto lhs = MakeTypeUnion({Prim("i32"), Prim("bool")});
    const auto rhs = MakeTypeUnion({Prim("bool"), Prim("i32")});
    const auto res = TypeEquiv(lhs, rhs);
    assert(res.ok && res.equiv);
  }
  {
    SPEC_COV("T-Equiv-Path");
    TypePath path = {"Foo"};
    const auto lhs = MakeTypePath(path);
    const auto rhs = MakeTypePath(path);
    const auto res = TypeEquiv(lhs, rhs);
    assert(res.ok && res.equiv);
  }
  {
    SPEC_COV("T-Equiv-ModalState");
    TypePath path = {"Stateful"};
    const auto lhs = MakeTypeModalState(path, "Ready");
    const auto rhs = MakeTypeModalState(path, "Ready");
    const auto res = TypeEquiv(lhs, rhs);
    assert(res.ok && res.equiv);
  }
  {
    SPEC_COV("T-Equiv-String");
    const auto lhs = MakeTypeString(StringState::View);
    const auto rhs = MakeTypeString(StringState::View);
    const auto res = TypeEquiv(lhs, rhs);
    assert(res.ok && res.equiv);
  }
  {
    SPEC_COV("T-Equiv-Bytes");
    const auto lhs = MakeTypeBytes(BytesState::Managed);
    const auto rhs = MakeTypeBytes(BytesState::Managed);
    const auto res = TypeEquiv(lhs, rhs);
    assert(res.ok && res.equiv);
  }
  {
    SPEC_COV("T-Equiv-Range");
    const auto lhs = MakeTypeRange();
    const auto rhs = MakeTypeRange();
    const auto res = TypeEquiv(lhs, rhs);
    assert(res.ok && res.equiv);
  }
  {
    SPEC_COV("T-Equiv-Ptr");
    const auto lhs = MakeTypePtr(Prim("i32"), PtrState::Valid);
    const auto rhs = MakeTypePtr(Prim("i32"), PtrState::Null);
    const auto res = TypeEquiv(lhs, rhs);
    assert(res.ok && !res.equiv);
  }
  {
    SPEC_COV("T-Equiv-RawPtr");
    const auto lhs = MakeTypeRawPtr(RawPtrQual::Imm, Prim("i32"));
    const auto rhs = MakeTypeRawPtr(RawPtrQual::Imm, Prim("i32"));
    const auto res = TypeEquiv(lhs, rhs);
    assert(res.ok && res.equiv);
  }
  {
    SPEC_COV("T-Equiv-Dynamic");
    TypePath path = {"Dyn"};
    const auto lhs = MakeTypeDynamic(path);
    const auto rhs = MakeTypeDynamic(path);
    const auto res = TypeEquiv(lhs, rhs);
    assert(res.ok && res.equiv);
  }

  {
    ScopeContext ctx;
    cursive0::syntax::ASTModule mod;
    mod.path = {"main"};
    mod.items.push_back(MakeStatic("LEN", MakeIntLit("4")));
    ctx.sigma.mods.push_back(mod);
    ctx.current_module = {"main"};

    SPEC_COV("ConstLen-Lit");
    ConstLenResult lit = ConstLen(ctx, MakeIntLit("12"));
    assert(lit.ok && lit.value.has_value() && *lit.value == 12);

    SPEC_COV("ConstLen-Path");
    ConstLenResult path = ConstLen(ctx, MakePathExpr({"main"}, "LEN"));
    assert(path.ok && path.value.has_value() && *path.value == 4);

    SPEC_COV("ConstLen-Err");
    Expr expr;
    expr.span = Span{};
    expr.node = cursive0::syntax::IdentifierExpr{"x"};
    ConstLenResult err = ConstLen(ctx, std::make_shared<Expr>(std::move(expr)));
    assert(!err.ok && err.diag_id.has_value());
  }

  return 0;
}
