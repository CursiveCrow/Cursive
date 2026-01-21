#include <cassert>
#include <optional>
#include <string>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/diagnostics.h"
#include "cursive0/analysis/types/type_stmt.h"
#include "cursive0/analysis/resolve/scopes.h"
#include "cursive0/analysis/types/types.h"
#include "cursive0/syntax/ast.h"

namespace {

using cursive0::core::DiagnosticStream;
using cursive0::analysis::BindOf;
using cursive0::analysis::CheckBlock;
using cursive0::analysis::CheckResult;
using cursive0::analysis::ExprTypeFn;
using cursive0::analysis::ExprTypeResult;
using cursive0::analysis::IdentTypeFn;
using cursive0::analysis::LoopFlag;
using cursive0::analysis::MakeTypePerm;
using cursive0::analysis::MakeTypePrim;
using cursive0::analysis::MutOf;
using cursive0::analysis::Permission;
using cursive0::analysis::PlaceTypeFn;
using cursive0::analysis::PlaceTypeResult;
using cursive0::analysis::ScopeContext;
using cursive0::analysis::StmtTypeContext;
using cursive0::analysis::TypePattern;
using cursive0::analysis::TypeEnv;
using cursive0::analysis::TypeRef;
using cursive0::analysis::MakeTypeTuple;
using cursive0::analysis::MakeTypePath;
using cursive0::analysis::TypeStmt;
using cursive0::analysis::TypeStmtSeq;
using cursive0::syntax::AssignStmt;
using cursive0::syntax::CompoundAssignStmt;
using cursive0::syntax::Binding;
using cursive0::syntax::Block;
using cursive0::syntax::BreakStmt;
using cursive0::syntax::ContinueStmt;
using cursive0::syntax::Expr;
using cursive0::syntax::ExprPtr;
using cursive0::syntax::ExprStmt;
using cursive0::syntax::IdentifierExpr;
using cursive0::syntax::LetStmt;
using cursive0::syntax::LiteralExpr;
using cursive0::syntax::ReturnStmt;
using cursive0::syntax::ResultStmt;
using cursive0::syntax::Token;
using cursive0::syntax::TokenKind;
using cursive0::syntax::VarStmt;
using cursive0::syntax::Pattern;
using cursive0::syntax::PatternPtr;
using cursive0::syntax::IdentifierPattern;
using cursive0::syntax::TuplePattern;
using cursive0::syntax::RecordPattern;
using cursive0::syntax::FieldPattern;
using cursive0::syntax::Stmt;
using cursive0::syntax::DeferStmt;

using cursive0::analysis::PathKeyOf;
using cursive0::analysis::TypePrim;
using cursive0::analysis::TypePerm;
using cursive0::syntax::FieldDecl;
using cursive0::syntax::RecordDecl;
using cursive0::syntax::Visibility;

ExprPtr MakeExpr(cursive0::syntax::ExprNode node) {
  auto expr = std::make_shared<Expr>();
  expr->node = std::move(node);
  return expr;
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

ExprPtr MakeIdent(const char* name) {
  return MakeExpr(IdentifierExpr{std::string(name)});
}

PatternPtr MakeIdentPattern(const char* name) {
  auto pat = std::make_shared<Pattern>();
  pat->node = IdentifierPattern{std::string(name)};
  return pat;
}

PatternPtr MakeTuplePattern(std::vector<PatternPtr> elements) {
  auto pat = std::make_shared<Pattern>();
  TuplePattern tuple{};
  tuple.elements = std::move(elements);
  pat->node = std::move(tuple);
  return pat;
}

PatternPtr MakeRecordPattern(const char* type_name, const char* field_name) {
  auto pat = std::make_shared<Pattern>();
  RecordPattern record{};
  record.path = {std::string(type_name)};
  FieldPattern field{};
  field.name = std::string(field_name);
  record.fields.push_back(field);
  pat->node = std::move(record);
  return pat;
}

std::shared_ptr<cursive0::syntax::Type> MakeTypePrimNode(const char* name) {
  auto type = std::make_shared<cursive0::syntax::Type>();
  type->node = cursive0::syntax::TypePrim{std::string(name)};
  return type;
}

TypeEnv EmptyEnv() {
  TypeEnv env;
  env.scopes.emplace_back();
  return env;
}

void AddBinding(TypeEnv& env, const char* name, cursive0::syntax::Mutability mut,
                const TypeRef& type) {
  env.scopes.back()[cursive0::analysis::IdKeyOf(name)] = {mut, type};
}

ExprTypeFn MakeTypeExpr(const TypeEnv* env) {
  return [env](const ExprPtr& expr) -> ExprTypeResult {
    if (!expr) {
      return {false, std::nullopt, {}};
    }
    if (const auto* lit = std::get_if<LiteralExpr>(&expr->node)) {
      if (lit->literal.kind == TokenKind::BoolLiteral) {
        return {true, std::nullopt, MakeTypePrim("bool")};
      }
      return {true, std::nullopt, MakeTypePrim("i32")};
    }
    if (const auto* ident = std::get_if<IdentifierExpr>(&expr->node)) {
      if (env) {
        const auto bind = BindOf(*env, ident->name);
        if (bind.has_value()) {
          return {true, std::nullopt, bind->type};
        }
      }
      return {false, "ResolveExpr-Ident-Err", {}};
    }
    return {false, "ResolveExpr-Ident-Err", {}};
  };
}

IdentTypeFn MakeTypeIdent(const TypeEnv* env) {
  return [env](std::string_view name) -> ExprTypeResult {
    if (env) {
      const auto bind = BindOf(*env, name);
      if (bind.has_value()) {
        return {true, std::nullopt, bind->type};
      }
    }
    return {false, "ResolveExpr-Ident-Err", {}};
  };
}

PlaceTypeFn MakePlaceType(const TypeEnv* env) {
  return [env](const ExprPtr& expr) -> PlaceTypeResult {
    if (!expr) {
      return {false, std::nullopt, {}};
    }
    if (const auto* ident = std::get_if<IdentifierExpr>(&expr->node)) {
      if (env) {
        const auto bind = BindOf(*env, ident->name);
        if (bind.has_value()) {
          return {true, std::nullopt, bind->type};
        }
      }
      return {false, "ResolveExpr-Ident-Err", {}};
    }
    return {false, "ResolveExpr-Ident-Err", {}};
  };
}

void SpecCoverage() {
  SPEC_COV("IntroAll-Empty");
  SPEC_COV("IntroAll-Cons");
  SPEC_COV("IntroAllVar-Empty");
  SPEC_COV("IntroAllVar-Cons");
  SPEC_COV("ShadowAll-Empty");
  SPEC_COV("ShadowAll-Cons");
  SPEC_COV("ShadowAllVar-Empty");
  SPEC_COV("ShadowAllVar-Cons");
  SPEC_COV("StmtSeq-Empty");
  SPEC_COV("StmtSeq-Cons");
  SPEC_COV("T-LetStmt-Ann");
  SPEC_COV("T-LetStmt-Ann-Mismatch");
  SPEC_COV("T-LetStmt-Infer");
  SPEC_COV("T-LetStmt-Infer-Err");
  SPEC_COV("T-VarStmt-Ann");
  SPEC_COV("T-VarStmt-Ann-Mismatch");
  SPEC_COV("T-VarStmt-Infer");
  SPEC_COV("T-VarStmt-Infer-Err");
  SPEC_COV("T-ShadowLetStmt-Ann");
  SPEC_COV("T-ShadowLetStmt-Ann-Mismatch");
  SPEC_COV("T-ShadowLetStmt-Infer");
  SPEC_COV("T-ShadowLetStmt-Infer-Err");
  SPEC_COV("T-ShadowVarStmt-Ann");
  SPEC_COV("T-ShadowVarStmt-Ann-Mismatch");
  SPEC_COV("T-ShadowVarStmt-Infer");
  SPEC_COV("T-ShadowVarStmt-Infer-Err");
  SPEC_COV("T-Assign");
  SPEC_COV("T-CompoundAssign");
  SPEC_COV("Assign-NotPlace");
  SPEC_COV("Assign-Immutable-Err");
  SPEC_COV("Assign-Type-Err");
  SPEC_COV("Assign-Const-Err");
  SPEC_COV("T-ExprStmt");
  SPEC_COV("T-UnsafeStmt");
  SPEC_COV("T-DeferStmt");
  SPEC_COV("Defer-NonUnit-Err");
  SPEC_COV("Defer-NonLocal-Err");
  SPEC_COV("HasNonLocalCtrl-Return");
  SPEC_COV("HasNonLocalCtrl-Result");
  SPEC_COV("HasNonLocalCtrl-Break");
  SPEC_COV("HasNonLocalCtrl-Continue");
  SPEC_COV("HasNonLocalCtrl-LoopInfinite");
  SPEC_COV("HasNonLocalCtrl-LoopConditional");
  SPEC_COV("HasNonLocalCtrl-LoopIter");
  SPEC_COV("HasNonLocalCtrl-Child");
  SPEC_COV("T-RegionStmt");
  SPEC_COV("T-FrameStmt-Implicit");
  SPEC_COV("T-FrameStmt-Explicit");
  SPEC_COV("Frame-NoActiveRegion-Err");
  SPEC_COV("Frame-Target-NotActive-Err");
  SPEC_COV("T-Return-Value");
  SPEC_COV("T-Return-Unit");
  SPEC_COV("Return-Type-Err");
  SPEC_COV("Return-Unit-Err");
  SPEC_COV("T-ResultStmt");
  SPEC_COV("Warn-Result-Unreachable");
  SPEC_COV("Warn-Result-Ok");
  SPEC_COV("T-Break-Value");
  SPEC_COV("T-Break-Unit");
  SPEC_COV("Break-Outside-Loop");
  SPEC_COV("T-Continue");
  SPEC_COV("Continue-Outside-Loop");
  SPEC_COV("T-ErrorStmt");
  SPEC_COV("BlockInfo-Res");
  SPEC_COV("BlockInfo-Res-Err");
  SPEC_COV("BlockInfo-Tail");
  SPEC_COV("BlockInfo-ReturnTail");
  SPEC_COV("BlockInfo-Unit");
  SPEC_COV("T-Block");
  SPEC_COV("Chk-Block-Tail");
  SPEC_COV("Chk-Block-Return");
  SPEC_COV("Chk-Block-Unit");
  SPEC_COV("T-Unsafe-Expr");
  SPEC_COV("Chk-Unsafe-Expr");
  SPEC_COV("T-Loop-Infinite");
  SPEC_COV("T-Loop-Conditional");
  SPEC_COV("T-Loop-Iter");
  SPEC_COV("Pat-Dup-Err");
  SPEC_COV("Pat-Wildcard");
  SPEC_COV("Pat-Ident");
  SPEC_COV("Pat-Unit");
  SPEC_COV("Pat-Tuple-Arity-Err");
  SPEC_COV("Pat-Tuple");
  SPEC_COV("Pat-Record");
  SPEC_COV("RecordPattern-UnknownField-Bind");
  SPEC_COV("Let-Refutable-Pattern-Err");
}

}  // namespace

int main() {
  SpecCoverage();

  ScopeContext scope;
  TypeEnv env = EmptyEnv();
  DiagnosticStream diags;
  StmtTypeContext type_ctx;
  type_ctx.return_type = MakeTypePrim("()");
  type_ctx.loop_flag = LoopFlag::None;
  type_ctx.in_unsafe = false;
  type_ctx.diags = &diags;
  auto type_expr = MakeTypeExpr(&env);
  auto type_ident = MakeTypeIdent(&env);
  auto type_place = MakePlaceType(&env);

  {
    SPEC_COV("T-LetStmt-Ann");
    Binding binding{};
    binding.pat = MakeIdentPattern("x");
    binding.type_opt = MakeTypePrimNode("i32");
    binding.init = MakeIntLiteral("1");
    LetStmt stmt{binding};
    auto res = TypeStmt(scope, type_ctx, stmt, env, type_expr, type_ident, type_place);
    assert(res.ok);
    const auto bound = BindOf(res.env, "x");
    assert(bound.has_value());
  }

  {
    SPEC_COV("T-LetStmt-Ann-Mismatch");
    Binding binding{};
    binding.pat = MakeIdentPattern("x");
    binding.type_opt = MakeTypePrimNode("i32");
    binding.init = MakeBoolLiteral(true);
    LetStmt stmt{binding};
    auto res = TypeStmt(scope, type_ctx, stmt, env, type_expr, type_ident, type_place);
    assert(!res.ok);
    assert(res.diag_id == std::optional<std::string_view>("T-LetStmt-Ann-Mismatch"));
  }

  {
    SPEC_COV("T-VarStmt-Infer");
    Binding binding{};
    binding.pat = MakeIdentPattern("y");
    binding.init = MakeIntLiteral("2");
    VarStmt stmt{binding};
    auto res = TypeStmt(scope, type_ctx, stmt, env, type_expr, type_ident, type_place);
    assert(res.ok);
    const auto bound = BindOf(res.env, "y");
    assert(bound.has_value());
  }

  {
    SPEC_COV("Assign-Immutable-Err");
    TypeEnv local = env;
    AddBinding(local, "z", cursive0::syntax::Mutability::Let, MakeTypePrim("i32"));
    AssignStmt stmt{MakeIdent("z"), MakeIntLiteral("3")};
    auto res = TypeStmt(scope, type_ctx, stmt, local, MakeTypeExpr(&local),
                        MakeTypeIdent(&local), MakePlaceType(&local));
    assert(!res.ok);
    assert(res.diag_id == std::optional<std::string_view>("Assign-Immutable-Err"));
  }

  {
    SPEC_COV("Assign-Type-Err");
    TypeEnv local = env;
    AddBinding(local, "w", cursive0::syntax::Mutability::Var, MakeTypePrim("i32"));
    AssignStmt stmt{MakeIdent("w"), MakeBoolLiteral(true)};
    auto res = TypeStmt(scope, type_ctx, stmt, local, MakeTypeExpr(&local),
                        MakeTypeIdent(&local), MakePlaceType(&local));
    assert(!res.ok);
    assert(res.diag_id == std::optional<std::string_view>("Assign-Type-Err"));
  }

  {
    SPEC_COV("Assign-Const-Err");
    TypeEnv local = env;
    AddBinding(local, "c", cursive0::syntax::Mutability::Var,
               MakeTypePerm(Permission::Const, MakeTypePrim("i32")));
    AssignStmt stmt{MakeIdent("c"), MakeIntLiteral("4")};
    auto res = TypeStmt(scope, type_ctx, stmt, local, MakeTypeExpr(&local),
                        MakeTypeIdent(&local), MakePlaceType(&local));
    assert(!res.ok);
    assert(res.diag_id == std::optional<std::string_view>("Assign-Const-Err"));
  }

  {
    SPEC_COV("Assign-Type-Err");
    TypeEnv local = env;
    AddBinding(local, "n", cursive0::syntax::Mutability::Var,
               MakeTypePrim("bool"));
    CompoundAssignStmt stmt{MakeIdent("n"), "+=", MakeBoolLiteral(true)};
    auto res = TypeStmt(scope, type_ctx, stmt, local, MakeTypeExpr(&local),
                        MakeTypeIdent(&local), MakePlaceType(&local));
    assert(!res.ok);
    assert(res.diag_id == std::optional<std::string_view>("Assign-Type-Err"));
  }

  {
    SPEC_COV("Break-Outside-Loop");
    BreakStmt stmt{MakeIntLiteral("1")};
    auto res = TypeStmt(scope, type_ctx, stmt, env, type_expr, type_ident, type_place);
    assert(!res.ok);
    assert(res.diag_id == std::optional<std::string_view>("Break-Outside-Loop"));
  }

  {
    SPEC_COV("T-Break-Value");
    StmtTypeContext loop_ctx;
    loop_ctx.return_type = MakeTypePrim("()");
    loop_ctx.loop_flag = LoopFlag::Loop;
    loop_ctx.in_unsafe = false;
    loop_ctx.diags = nullptr;
    BreakStmt stmt{MakeIntLiteral("1")};
    auto res = TypeStmt(scope, loop_ctx, stmt, env, type_expr, type_ident, type_place);
    assert(res.ok);
    assert(res.flow.breaks.size() == 1);
  }

  {
    SPEC_COV("Continue-Outside-Loop");
    ContinueStmt stmt{};
    auto res = TypeStmt(scope, type_ctx, stmt, env, type_expr, type_ident, type_place);
    assert(!res.ok);
    assert(res.diag_id == std::optional<std::string_view>("Continue-Outside-Loop"));
  }

  {
    SPEC_COV("Defer-NonLocal-Err");
    Block block{};
    block.stmts.push_back(Stmt{ReturnStmt{}});
    DeferStmt stmt{std::make_shared<Block>(block)};
    auto res = TypeStmt(scope, type_ctx, stmt, env, type_expr, type_ident, type_place);
    assert(!res.ok);
    assert(res.diag_id == std::optional<std::string_view>("Defer-NonLocal-Err"));
  }

  {
    auto pat = MakeTuplePattern({MakeIdentPattern("a")});
    auto expected = MakeTypePerm(Permission::Const,
                                 MakeTypeTuple({MakeTypePrim("i32")}));
    auto res = TypePattern(scope, pat, expected);
    assert(res.ok);
    assert(res.bindings.size() == 1);
    assert(res.bindings[0].first == "a");
    const auto* perm = std::get_if<TypePerm>(&res.bindings[0].second->node);
    assert(perm && perm->perm == Permission::Const);
    const auto* prim = std::get_if<TypePrim>(&perm->base->node);
    assert(prim && prim->name == "i32");
  }

  {
    RecordDecl rec;
    rec.vis = Visibility::Public;
    rec.name = "Rec";
    rec.implements = {};
    FieldDecl field;
    field.vis = Visibility::Public;
    field.name = "field";
    field.type = MakeTypePrimNode("i32");
    field.init_opt = nullptr;
    field.span = {};
    field.doc_opt = std::nullopt;
    rec.members = {field};
    rec.span = {};
    rec.doc = {};
    scope.sigma.types.emplace(PathKeyOf(cursive0::syntax::Path{"Rec"}), rec);

    auto pat = MakeRecordPattern("Rec", "field");
    auto expected =
        MakeTypePerm(Permission::Const, MakeTypePath({"Rec"}));
    auto res = TypePattern(scope, pat, expected);
    assert(res.ok);
    assert(res.bindings.size() == 1);
    assert(res.bindings[0].first == "field");
    const auto* perm = std::get_if<TypePerm>(&res.bindings[0].second->node);
    assert(perm && perm->perm == Permission::Const);
    const auto* prim = std::get_if<TypePrim>(&perm->base->node);
    assert(prim && prim->name == "i32");
  }

  return 0;
}
