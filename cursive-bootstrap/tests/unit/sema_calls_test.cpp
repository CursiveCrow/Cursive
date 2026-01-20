#include <cassert>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/sema/calls.h"
#include "cursive0/sema/record_methods.h"
#include "cursive0/sema/scopes.h"
#include "cursive0/sema/scopes_lookup.h"
#include "cursive0/sema/types.h"
#include "cursive0/syntax/ast.h"

namespace {

using cursive0::sema::CallTypeResult;
using cursive0::sema::Entity;
using cursive0::sema::EntityKind;
using cursive0::sema::EntitySource;
using cursive0::sema::ExprTypeFn;
using cursive0::sema::ExprTypeResult;
using cursive0::sema::IdKeyOf;
using cursive0::sema::IsRecordCallee;
using cursive0::sema::ArgsOk;
using cursive0::sema::LowerTypeFn;
using cursive0::sema::LowerTypeResult;
using cursive0::sema::MakeTypeFunc;
using cursive0::sema::MakeTypePrim;
using cursive0::sema::ParamMode;
using cursive0::sema::PlaceTypeFn;
using cursive0::sema::PlaceTypeResult;
using cursive0::sema::PathKeyOf;
using cursive0::sema::Scope;
using cursive0::sema::ScopeContext;
using cursive0::sema::TypeCall;
using cursive0::sema::TypeFuncParam;
using cursive0::sema::TypeRef;
using cursive0::syntax::Arg;
using cursive0::syntax::IdentifierExpr;
using cursive0::syntax::LiteralExpr;
using cursive0::syntax::MoveExpr;
using cursive0::syntax::Param;
using cursive0::syntax::Path;
using cursive0::syntax::RecordDecl;
using cursive0::syntax::Token;
using cursive0::syntax::TokenKind;
using cursive0::syntax::Type;
using cursive0::syntax::TypePath;
using cursive0::syntax::TypePathType;

TypeRef Prim(const char* name) { return MakeTypePrim(name); }

cursive0::syntax::ExprPtr MakeExpr(cursive0::syntax::ExprNode node) {
  auto expr = std::make_shared<cursive0::syntax::Expr>();
  expr->node = std::move(node);
  return expr;
}

cursive0::syntax::ExprPtr MakeIdent(const char* name) {
  return MakeExpr(IdentifierExpr{std::string(name)});
}

std::shared_ptr<Type> MakeTypePathType(const char* name) {
  auto type = std::make_shared<Type>();
  type->span = {};
  TypePath path;
  path.emplace_back(std::string(name));
  type->node = TypePathType{std::move(path)};
  return type;
}

cursive0::syntax::ExprPtr MakeIntLiteral() {
  Token tok;
  tok.kind = TokenKind::IntLiteral;
  tok.lexeme = "0";
  return MakeExpr(LiteralExpr{tok});
}

Arg MakeArg(bool moved, const cursive0::syntax::ExprPtr& value) {
  Arg arg;
  arg.moved = moved;
  arg.value = value;
  return arg;
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
            return {true, std::nullopt, Prim("i32")};
          } else if constexpr (std::is_same_v<T, MoveExpr>) {
            return TypeOf(node.place);
          }
          return {false, "ResolveExpr-Ident-Err", {}};
        },
        expr->node);
  }
};

}  // namespace

int main() {
  {
    SPEC_COV("ArgsT-Empty");
    SPEC_COV("T-Call");
    TypeEnv env;
    std::vector<TypeFuncParam> params;
    env.types["f"] = MakeTypeFunc(params, Prim("bool"));

    const auto callee = MakeIdent("f");
    const auto result = TypeCall(ScopeContext{}, callee, {},
                                 [&](const auto& expr) { return env.TypeOf(expr); });
    assert(result.ok);
    assert(result.type);
    const auto* prim = std::get_if<cursive0::sema::TypePrim>(&result.type->node);
    assert(prim && prim->name == "bool");
  }

  {
    SPEC_COV("ArgsT-Cons");
    SPEC_COV("ArgsT-Cons-Ref");
    SPEC_COV("T-Call");

    bool saw_move = false;
    TypeEnv env;
    std::vector<TypeFuncParam> params;
    params.push_back(TypeFuncParam{ParamMode::Move, Prim("i32")});
    params.push_back(TypeFuncParam{std::nullopt, Prim("i32")});
    env.types["f"] = MakeTypeFunc(params, Prim("i32"));
    env.types["x"] = Prim("i32");
    env.types["y"] = Prim("i32");

    ExprTypeFn type_expr = [&](const auto& expr) -> ExprTypeResult {
      if (expr && std::holds_alternative<MoveExpr>(expr->node)) {
        saw_move = true;
      }
      return env.TypeOf(expr);
    };

    std::vector<Arg> args;
    args.push_back(MakeArg(true, MakeIdent("x")));
    args.push_back(MakeArg(false, MakeIdent("y")));

    const auto result = TypeCall(ScopeContext{}, MakeIdent("f"), args, type_expr);
    assert(result.ok);
    assert(saw_move);
  }

  {
    SPEC_COV("Call-Callee-NotFunc");
    TypeEnv env;
    env.types["x"] = Prim("i32");
    const auto result = TypeCall(ScopeContext{}, MakeIdent("x"), {},
                                 [&](const auto& expr) { return env.TypeOf(expr); });
    assert(!result.ok);
    assert(result.diag_id == std::optional<std::string_view>("Call-Callee-NotFunc"));
  }

  {
    SPEC_COV("Call-ArgCount-Err");
    TypeEnv env;
    std::vector<TypeFuncParam> params;
    params.push_back(TypeFuncParam{std::nullopt, Prim("i32")});
    env.types["f"] = MakeTypeFunc(params, Prim("i32"));
    const auto result = TypeCall(ScopeContext{}, MakeIdent("f"), {},
                                 [&](const auto& expr) { return env.TypeOf(expr); });
    assert(!result.ok);
    assert(result.diag_id == std::optional<std::string_view>("Call-ArgCount-Err"));
  }

  {
    SPEC_COV("Call-ArgType-Err");
    TypeEnv env;
    std::vector<TypeFuncParam> params;
    params.push_back(TypeFuncParam{std::nullopt, Prim("i32")});
    env.types["f"] = MakeTypeFunc(params, Prim("i32"));
    env.types["x"] = Prim("i64");
    std::vector<Arg> args;
    args.push_back(MakeArg(false, MakeIdent("x")));
    const auto result = TypeCall(ScopeContext{}, MakeIdent("f"), args,
                                 [&](const auto& expr) { return env.TypeOf(expr); });
    assert(!result.ok);
    assert(result.diag_id == std::optional<std::string_view>("Call-ArgType-Err"));
  }

  {
    SPEC_COV("Call-Move-Missing");
    TypeEnv env;
    std::vector<TypeFuncParam> params;
    params.push_back(TypeFuncParam{ParamMode::Move, Prim("i32")});
    env.types["f"] = MakeTypeFunc(params, Prim("i32"));
    env.types["x"] = Prim("i32");
    std::vector<Arg> args;
    args.push_back(MakeArg(false, MakeIdent("x")));
    const auto result = TypeCall(ScopeContext{}, MakeIdent("f"), args,
                                 [&](const auto& expr) { return env.TypeOf(expr); });
    assert(!result.ok);
    assert(result.diag_id == std::optional<std::string_view>("Call-Move-Missing"));
  }

  {
    SPEC_COV("Call-Move-Unexpected");
    TypeEnv env;
    std::vector<TypeFuncParam> params;
    params.push_back(TypeFuncParam{std::nullopt, Prim("i32")});
    env.types["f"] = MakeTypeFunc(params, Prim("i32"));
    env.types["x"] = Prim("i32");
    std::vector<Arg> args;
    args.push_back(MakeArg(true, MakeIdent("x")));
    const auto result = TypeCall(ScopeContext{}, MakeIdent("f"), args,
                                 [&](const auto& expr) { return env.TypeOf(expr); });
    assert(!result.ok);
    assert(result.diag_id == std::optional<std::string_view>("Call-Move-Unexpected"));
  }

  {
    SPEC_COV("Call-Arg-NotPlace");
    TypeEnv env;
    std::vector<TypeFuncParam> params;
    params.push_back(TypeFuncParam{std::nullopt, Prim("i32")});
    env.types["f"] = MakeTypeFunc(params, Prim("i32"));
    std::vector<Arg> args;
    args.push_back(MakeArg(false, MakeIntLiteral()));
    const auto result = TypeCall(ScopeContext{}, MakeIdent("f"), args,
                                 [&](const auto& expr) { return env.TypeOf(expr); });
    assert(!result.ok);
    assert(result.diag_id == std::optional<std::string_view>("Call-Arg-NotPlace"));
  }

  {
    ScopeContext ctx;
    ctx.scopes = {Scope{}, Scope{}, Scope{}};
    ctx.current_module = {"main"};

    Entity ent;
    ent.kind = EntityKind::Type;
    ent.origin_opt = cursive0::syntax::ModulePath{"main"};
    ent.source = EntitySource::Decl;
    ctx.scopes[0][IdKeyOf("Rec")] = ent;

    RecordDecl rec;
    rec.vis = cursive0::syntax::Visibility::Public;
    rec.name = "Rec";
    Path rec_path = {"main", "Rec"};
    ctx.sigma.types[PathKeyOf(rec_path)] = rec;

    TypeEnv env;
    env.types["Rec"] = Prim("i32");

    const auto callee = MakeIdent("Rec");
    assert(IsRecordCallee(ctx, callee, {}));
    const CallTypeResult result = TypeCall(ctx, callee, {},
                                           [&](const auto& expr) { return env.TypeOf(expr); });
    assert(!result.ok);
    assert(result.record_callee);
    assert(!result.diag_id.has_value());
  }

  {
    SPEC_COV("Args-Empty");
    ScopeContext ctx;
    TypeEnv env;
    LowerTypeFn lower_type = [&](const auto&) -> LowerTypeResult {
      return {true, std::nullopt, Prim("i32")};
    };
    PlaceTypeFn place_type = [&](const auto& expr) -> PlaceTypeResult {
      const auto typed = env.TypeOf(expr);
      if (!typed.ok) {
        return {false, typed.diag_id, {}};
      }
      return {true, std::nullopt, typed.type};
    };
    const auto result = ArgsOk(ctx, {}, {}, [&](const auto& expr) {
      return env.TypeOf(expr);
    }, &place_type, lower_type);
    assert(result.ok);
  }

  {
    SPEC_COV("Args-Cons");
    ScopeContext ctx;
    TypeEnv env;
    env.types["x"] = Prim("i32");

    Param param;
    param.mode = cursive0::syntax::ParamMode::Move;
    param.name = "x";
    param.type = MakeTypePathType("i32");
    param.span = {};

    Arg arg;
    arg.moved = true;
    arg.value = MakeIdent("x");
    arg.span = {};

    LowerTypeFn lower_type = [&](const auto&) -> LowerTypeResult {
      return {true, std::nullopt, Prim("i32")};
    };
    PlaceTypeFn place_type = [&](const auto& expr) -> PlaceTypeResult {
      const auto typed = env.TypeOf(expr);
      if (!typed.ok) {
        return {false, typed.diag_id, {}};
      }
      return {true, std::nullopt, typed.type};
    };

    const auto result = ArgsOk(ctx, {param}, {arg}, [&](const auto& expr) {
      return env.TypeOf(expr);
    }, &place_type, lower_type);
    assert(result.ok);
  }

  {
    SPEC_COV("Args-Cons-Ref");
    ScopeContext ctx;
    TypeEnv env;
    env.types["y"] = Prim("i32");

    Param param;
    param.mode = std::nullopt;
    param.name = "y";
    param.type = MakeTypePathType("i32");
    param.span = {};

    Arg arg;
    arg.moved = false;
    arg.value = MakeIdent("y");
    arg.span = {};

    LowerTypeFn lower_type = [&](const auto&) -> LowerTypeResult {
      return {true, std::nullopt, Prim("i32")};
    };
    PlaceTypeFn place_type = [&](const auto& expr) -> PlaceTypeResult {
      const auto typed = env.TypeOf(expr);
      if (!typed.ok) {
        return {false, typed.diag_id, {}};
      }
      return {true, std::nullopt, typed.type};
    };

    const auto result = ArgsOk(ctx, {param}, {arg}, [&](const auto& expr) {
      return env.TypeOf(expr);
    }, &place_type, lower_type);
    assert(result.ok);
  }

  return 0;
}
