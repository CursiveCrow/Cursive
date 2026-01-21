#include <cassert>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/analysis/composite/records.h"
#include "cursive0/analysis/resolve/scopes.h"
#include "cursive0/analysis/types/types.h"
#include "cursive0/syntax/ast.h"

namespace {

using cursive0::analysis::CheckRecordWf;
using cursive0::analysis::ExprTypeFn;
using cursive0::analysis::ExprTypeResult;
using cursive0::analysis::MakeTypePrim;
using cursive0::analysis::MakeTypePath;
using cursive0::analysis::RecordWfResult;
using cursive0::analysis::ScopeContext;
using cursive0::analysis::TypePath;
using cursive0::analysis::TypePathType;
using cursive0::analysis::TypePrim;
using cursive0::analysis::TypeRef;
using cursive0::analysis::TypeRecordDefaultCall;
using cursive0::analysis::Entity;
using cursive0::analysis::EntityKind;
using cursive0::analysis::EntitySource;
using cursive0::analysis::IdKeyOf;
using cursive0::analysis::PathKeyOf;
using cursive0::analysis::Scope;
using cursive0::syntax::ExprPtr;
using cursive0::syntax::FieldDecl;
using cursive0::syntax::IdentifierExpr;
using cursive0::syntax::LiteralExpr;
using cursive0::syntax::RecordDecl;
using cursive0::syntax::Token;
using cursive0::syntax::TokenKind;
using cursive0::syntax::Type;
using SyntaxTypePrim = cursive0::syntax::TypePrim;
using cursive0::syntax::Visibility;
using cursive0::syntax::Path;

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

std::shared_ptr<Type> MakeTypePrimAst(const char* name) {
  Type out;
  out.node = SyntaxTypePrim{std::string(name)};
  return std::make_shared<Type>(std::move(out));
}

FieldDecl MakeField(std::string name,
                    std::shared_ptr<Type> type,
                    ExprPtr init) {
  FieldDecl field;
  field.vis = Visibility::Public;
  field.name = std::move(name);
  field.type = std::move(type);
  field.init_opt = std::move(init);
  return field;
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
            if (node.literal.kind == TokenKind::IntLiteral) {
              return {true, std::nullopt, Prim("i32")};
            }
            return {false, "ResolveExpr-Ident-Err", {}};
          }
          return {false, "ResolveExpr-Ident-Err", {}};
        },
        expr->node);
  }
};

ScopeContext MakeRecordContext(const RecordDecl& record) {
  ScopeContext ctx;
  ctx.scopes = {Scope{}, Scope{}, Scope{}};
  ctx.current_module = {"main"};

  Entity ent;
  ent.kind = EntityKind::Type;
  ent.origin_opt = cursive0::syntax::ModulePath{"main"};
  ent.source = EntitySource::Decl;
  ctx.scopes[0][IdKeyOf(record.name)] = ent;

  Path rec_path = {"main", record.name};
  ctx.sigma.types[PathKeyOf(rec_path)] = record;
  return ctx;
}

}  // namespace

int main() {
  {
    SPEC_COV("WF-Record");
    SPEC_COV("T-Record-Default");
    RecordDecl rec;
    rec.vis = Visibility::Public;
    rec.name = "Rec";
    rec.members.push_back(
        MakeField("a", MakeTypePrimAst("i32"), MakeIntLiteral("1")));
    rec.members.push_back(
        MakeField("b", MakeTypePrimAst("i32"), MakeIntLiteral("2")));

    auto ctx = MakeRecordContext(rec);
    TypeEnv env;
    ExprTypeFn type_expr = [&](const auto& expr) { return env.TypeOf(expr); };

    const auto wf = CheckRecordWf(ctx, rec, type_expr);
    assert(wf.ok);

    const auto result = TypeRecordDefaultCall(ctx, MakeIdent("Rec"), {},
                                              type_expr);
    assert(result.ok);
    const auto* path = std::get_if<TypePathType>(&result.type->node);
    assert(path && path->path.size() == 2);
    assert(path->path[0] == "main");
    assert(path->path[1] == "Rec");
  }

  {
    SPEC_COV("Record-Default-Init-Err");
    RecordDecl rec;
    rec.vis = Visibility::Public;
    rec.name = "Rec";
    rec.members.push_back(
        MakeField("a", MakeTypePrimAst("i32"), nullptr));

    auto ctx = MakeRecordContext(rec);
    TypeEnv env;
    ExprTypeFn type_expr = [&](const auto& expr) { return env.TypeOf(expr); };

    const auto result = TypeRecordDefaultCall(ctx, MakeIdent("Rec"), {},
                                              type_expr);
    assert(!result.ok);
    assert(result.diag_id ==
           std::optional<std::string_view>("Record-Default-Init-Err"));
  }

  {
    SPEC_COV("WF-Record-DupField");
    RecordDecl rec;
    rec.vis = Visibility::Public;
    rec.name = "Rec";
    rec.members.push_back(
        MakeField("a", MakeTypePrimAst("i32"), MakeIntLiteral("1")));
    rec.members.push_back(
        MakeField("a", MakeTypePrimAst("i32"), MakeIntLiteral("2")));

    auto ctx = MakeRecordContext(rec);
    TypeEnv env;
    ExprTypeFn type_expr = [&](const auto& expr) { return env.TypeOf(expr); };

    const auto wf = CheckRecordWf(ctx, rec, type_expr);
    assert(!wf.ok);
    assert(wf.diag_id ==
           std::optional<std::string_view>("WF-Record-DupField"));
  }

  return 0;
}
