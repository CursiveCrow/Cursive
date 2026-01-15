#include <cassert>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/sema/scopes.h"
#include "cursive0/sema/type_infer.h"
#include "cursive0/sema/types.h"
#include "cursive0/syntax/ast.h"

namespace {

using cursive0::sema::CheckExpr;
using cursive0::sema::ExprTypeFn;
using cursive0::sema::ExprTypeResult;
using cursive0::sema::IdentTypeFn;
using cursive0::sema::InferExpr;
using cursive0::sema::PlaceTypeFn;
using cursive0::sema::PlaceTypeResult;
using cursive0::sema::MakeTypeFunc;
using cursive0::sema::MakeTypeModalState;
using cursive0::sema::MakeTypePerm;
using cursive0::sema::MakeTypePrim;
using cursive0::sema::MakeTypePtr;
using cursive0::sema::MakeTypePath;
using cursive0::sema::PathKeyOf;
using cursive0::sema::Permission;
using cursive0::sema::PtrState;
using cursive0::sema::Scope;
using cursive0::sema::ScopeContext;
using cursive0::sema::TypeFuncParam;
using cursive0::sema::TypePrim;
using cursive0::sema::TypeRef;
using cursive0::sema::TypeTuple;
using cursive0::syntax::Arg;
using cursive0::syntax::ExprPtr;
using cursive0::syntax::IdentifierExpr;
using cursive0::syntax::ModalDecl;
using cursive0::syntax::Path;
using cursive0::syntax::PathExpr;
using cursive0::syntax::PtrNullExpr;
using cursive0::syntax::StateBlock;
using cursive0::syntax::TupleExpr;
using cursive0::syntax::Visibility;

TypeRef Prim(const char* name) { return MakeTypePrim(name); }

ExprPtr MakeExpr(cursive0::syntax::ExprNode node) {
  auto expr = std::make_shared<cursive0::syntax::Expr>();
  expr->node = std::move(node);
  return expr;
}

ExprPtr MakeIdent(const char* name) {
  return MakeExpr(IdentifierExpr{std::string(name)});
}

ExprPtr MakePath(const char* name) {
  PathExpr expr;
  expr.path = Path{"main"};
  expr.name = name;
  return MakeExpr(expr);
}

ExprPtr MakeTuple(const std::vector<ExprPtr>& elements) {
  TupleExpr expr;
  expr.elements = elements;
  return MakeExpr(expr);
}

ExprPtr MakePtrNull() { return MakeExpr(PtrNullExpr{}); }

Arg MakeArg(const ExprPtr& value) {
  Arg arg;
  arg.value = value;
  return arg;
}

struct TypeEnv {
  std::unordered_map<std::string, TypeRef> idents;
  std::unordered_map<std::string, TypeRef> paths;

  ExprTypeResult TypeOf(const ExprPtr& expr) const {
    if (!expr) {
      return {false, std::nullopt, {}};
    }
    return std::visit(
        [&](const auto& node) -> ExprTypeResult {
          using T = std::decay_t<decltype(node)>;
          if constexpr (std::is_same_v<T, IdentifierExpr>) {
            const auto it = idents.find(node.name);
            if (it == idents.end()) {
              return {false, "ResolveExpr-Ident-Err", {}};
            }
            return {true, std::nullopt, it->second};
          } else if constexpr (std::is_same_v<T, PathExpr>) {
            const auto key = node.name;
            const auto it = paths.find(key);
            if (it == paths.end()) {
              return {false, "ResolveExpr-Ident-Err", {}};
            }
            return {true, std::nullopt, it->second};
          }
          return {false, std::nullopt, {}};
        },
        expr->node);
  }
};

}  // namespace

int main() {
  {
    SPEC_COV("Syn-Ident");
    TypeEnv env;
    env.idents["x"] = MakeTypePerm(Permission::Unique, Prim("i32"));
    ExprTypeFn type_expr = [&](const auto&) -> ExprTypeResult {
      return {false, std::nullopt, {}};
    };
    IdentTypeFn type_ident = [&](std::string_view name) {
      const auto it = env.idents.find(std::string(name));
      if (it == env.idents.end()) {
        return ExprTypeResult{false, "ResolveExpr-Ident-Err", {}};
      }
      return ExprTypeResult{true, std::nullopt, it->second};
    };
    PlaceTypeFn type_place = [&](const auto& expr) -> PlaceTypeResult {
      const auto res = env.TypeOf(expr);
      return {res.ok, res.diag_id, res.type};
    };

    const auto result = InferExpr(ScopeContext{}, MakeIdent("x"), type_expr,
                                  type_place, type_ident);
    assert(result.ok);
    const auto* perm = std::get_if<cursive0::sema::TypePerm>(&result.type->node);
    assert(perm && perm->perm == Permission::Unique);
  }

  {
    SPEC_COV("Syn-Unit");
    TypeEnv env;
    ExprTypeFn type_expr = [&](const auto& expr) { return env.TypeOf(expr); };
    IdentTypeFn type_ident = [&](std::string_view) {
      return ExprTypeResult{false, std::nullopt, {}};
    };
    PlaceTypeFn type_place = [&](const auto& expr) -> PlaceTypeResult {
      const auto res = env.TypeOf(expr);
      return {res.ok, res.diag_id, res.type};
    };

    const auto result =
        InferExpr(ScopeContext{}, MakeTuple({}), type_expr, type_place, type_ident);
    assert(result.ok);
    const auto* prim = std::get_if<TypePrim>(&result.type->node);
    assert(prim && prim->name == "()");
  }

  {
    SPEC_COV("Syn-Tuple");
    SPEC_COV("Syn-Expr");
    TypeEnv env;
    env.paths["a"] = Prim("i32");
    env.paths["b"] = Prim("bool");
    ExprTypeFn type_expr = [&](const auto& expr) { return env.TypeOf(expr); };
    IdentTypeFn type_ident = [&](std::string_view) {
      return ExprTypeResult{false, std::nullopt, {}};
    };
    PlaceTypeFn type_place = [&](const auto& expr) -> PlaceTypeResult {
      const auto res = env.TypeOf(expr);
      return {res.ok, res.diag_id, res.type};
    };

    const auto tuple = MakeTuple({MakePath("a"), MakePath("b")});
    const auto result = InferExpr(ScopeContext{}, tuple, type_expr, type_place, type_ident);
    assert(result.ok);
    const auto* tup = std::get_if<TypeTuple>(&result.type->node);
    assert(tup && tup->elements.size() == 2);
  }

  {
    SPEC_COV("Syn-Call");
    TypeEnv env;
    env.idents["f"] =
        MakeTypeFunc({TypeFuncParam{std::nullopt, Prim("i32")},
                      TypeFuncParam{std::nullopt, Prim("bool")}},
                     Prim("bool"));
    env.idents["a"] = Prim("i32");
    env.idents["b"] = Prim("bool");
    ExprTypeFn type_expr = [&](const auto& expr) { return env.TypeOf(expr); };
    IdentTypeFn type_ident = [&](std::string_view name) {
      const auto it = env.idents.find(std::string(name));
      if (it == env.idents.end()) {
        return ExprTypeResult{false, "ResolveExpr-Ident-Err", {}};
      }
      return ExprTypeResult{true, std::nullopt, it->second};
    };
    PlaceTypeFn type_place = [&](const auto& expr) -> PlaceTypeResult {
      const auto res = env.TypeOf(expr);
      return {res.ok, res.diag_id, res.type};
    };

    cursive0::syntax::CallExpr call;
    call.callee = MakeIdent("f");
    call.args = {MakeArg(MakeIdent("a")), MakeArg(MakeIdent("b"))};

    const auto result =
        InferExpr(ScopeContext{}, MakeExpr(call), type_expr, type_place, type_ident);
    assert(result.ok);
    const auto* prim = std::get_if<TypePrim>(&result.type->node);
    assert(prim && prim->name == "bool");
  }

  {
    SPEC_COV("Syn-Call-Err");
    TypeEnv env;
    env.idents["f"] =
        MakeTypeFunc({TypeFuncParam{std::nullopt, Prim("i32")}}, Prim("i32"));
    env.idents["b"] = Prim("bool");
    ExprTypeFn type_expr = [&](const auto& expr) { return env.TypeOf(expr); };
    IdentTypeFn type_ident = [&](std::string_view name) {
      const auto it = env.idents.find(std::string(name));
      if (it == env.idents.end()) {
        return ExprTypeResult{false, "ResolveExpr-Ident-Err", {}};
      }
      return ExprTypeResult{true, std::nullopt, it->second};
    };
    PlaceTypeFn type_place = [&](const auto& expr) -> PlaceTypeResult {
      const auto res = env.TypeOf(expr);
      return {res.ok, res.diag_id, res.type};
    };

    cursive0::syntax::CallExpr call;
    call.callee = MakeIdent("f");
    call.args = {MakeArg(MakeIdent("b"))};

    const auto result =
        InferExpr(ScopeContext{}, MakeExpr(call), type_expr, type_place, type_ident);
    assert(!result.ok);
    assert(result.diag_id ==
           std::optional<std::string_view>("Call-ArgType-Err"));
  }

  {
    SPEC_COV("Syn-PtrNull-Err");
    TypeEnv env;
    ExprTypeFn type_expr = [&](const auto& expr) { return env.TypeOf(expr); };
    IdentTypeFn type_ident = [&](std::string_view) {
      return ExprTypeResult{false, std::nullopt, {}};
    };
    PlaceTypeFn type_place = [&](const auto& expr) -> PlaceTypeResult {
      const auto res = env.TypeOf(expr);
      return {res.ok, res.diag_id, res.type};
    };

    const auto result =
        InferExpr(ScopeContext{}, MakePtrNull(), type_expr, type_place, type_ident);
    assert(!result.ok);
    assert(result.diag_id ==
           std::optional<std::string_view>("PtrNull-Infer-Err"));
  }

  {
    SPEC_COV("Chk-Null-Ptr");
    TypeEnv env;
    ExprTypeFn type_expr = [&](const auto& expr) { return env.TypeOf(expr); };
    IdentTypeFn type_ident = [&](std::string_view) {
      return ExprTypeResult{false, std::nullopt, {}};
    };
    PlaceTypeFn type_place = [&](const auto& expr) -> PlaceTypeResult {
      const auto res = env.TypeOf(expr);
      return {res.ok, res.diag_id, res.type};
    };

    const auto expected = MakeTypePtr(Prim("u8"), PtrState::Null);
    const auto result = CheckExpr(ScopeContext{}, MakePtrNull(), expected,
                                  type_expr, type_place, type_ident);
    assert(result.ok);
  }

  {
    SPEC_COV("Chk-PtrNull-Err");
    TypeEnv env;
    ExprTypeFn type_expr = [&](const auto& expr) { return env.TypeOf(expr); };
    IdentTypeFn type_ident = [&](std::string_view) {
      return ExprTypeResult{false, std::nullopt, {}};
    };
    PlaceTypeFn type_place = [&](const auto& expr) -> PlaceTypeResult {
      const auto res = env.TypeOf(expr);
      return {res.ok, res.diag_id, res.type};
    };

    const auto result = CheckExpr(ScopeContext{}, MakePtrNull(), Prim("i32"),
                                  type_expr, type_place, type_ident);
    assert(!result.ok);
    assert(result.diag_id ==
           std::optional<std::string_view>("PtrNull-Infer-Err"));
  }

  {
    SPEC_COV("Chk-Subsumption");
    TypeEnv env;
    env.idents["x"] = Prim("i32");
    ExprTypeFn type_expr = [&](const auto& expr) { return env.TypeOf(expr); };
    IdentTypeFn type_ident = [&](std::string_view name) {
      const auto it = env.idents.find(std::string(name));
      if (it == env.idents.end()) {
        return ExprTypeResult{false, "ResolveExpr-Ident-Err", {}};
      }
      return ExprTypeResult{true, std::nullopt, it->second};
    };
    PlaceTypeFn type_place = [&](const auto& expr) -> PlaceTypeResult {
      const auto res = env.TypeOf(expr);
      return {res.ok, res.diag_id, res.type};
    };

    const auto result =
        CheckExpr(ScopeContext{}, MakeIdent("x"), Prim("i32"), type_expr,
                  type_place, type_ident);
    assert(result.ok);
  }

  {
    SPEC_COV("Chk-Subsumption-Modal-NonNiche");
    ScopeContext ctx;
    ctx.scopes = {Scope{}, Scope{}, Scope{}};
    ctx.current_module = {"main"};

    ModalDecl modal;
    modal.vis = Visibility::Public;
    modal.name = "M";
    modal.doc = {};
    StateBlock state;
    state.name = "S";
    modal.states.push_back(state);

    Path modal_path = {"main", "M"};
    ctx.sigma.types[PathKeyOf(modal_path)] = modal;

    TypeEnv env;
    env.idents["m"] = MakeTypeModalState({"main", "M"}, "S");
    ExprTypeFn type_expr = [&](const auto& expr) { return env.TypeOf(expr); };
    IdentTypeFn type_ident = [&](std::string_view name) {
      const auto it = env.idents.find(std::string(name));
      if (it == env.idents.end()) {
        return ExprTypeResult{false, "ResolveExpr-Ident-Err", {}};
      }
      return ExprTypeResult{true, std::nullopt, it->second};
    };
    PlaceTypeFn type_place = [&](const auto& expr) -> PlaceTypeResult {
      const auto res = env.TypeOf(expr);
      return {res.ok, res.diag_id, res.type};
    };

    const auto expected = MakeTypePath({"main", "M"});
    const auto result =
        CheckExpr(ctx, MakeIdent("m"), expected, type_expr, type_place, type_ident);
    assert(!result.ok);
    assert(result.diag_id ==
           std::optional<std::string_view>("Chk-Subsumption-Modal-NonNiche"));
  }

  return 0;
}
