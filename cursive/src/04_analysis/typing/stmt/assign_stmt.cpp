// =============================================================================
// assign_stmt.cpp - Assignment statement typing
// =============================================================================
//
// SPEC REFERENCE: CursiveSpecification.md
//   Section 5.2.11: Statement Typing - Assignments (lines 9449-9488)
//   - PlaceRoot definition (lines 9451-9455)
//   - T-Assign (lines 9457-9460): Basic assignment
//   - Assign-NotPlace (lines 9467-9470): Not a place error
//   - Assign-Immutable-Err (lines 9472-9475): Immutable binding error
//   - Assign-Type-Err (lines 9477-9480): Type mismatch error
//   - Assign-Const-Err (lines 9484-9487): Const-qualified error
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_stmt.cpp
//
// =============================================================================

#include "04_analysis/typing/type_stmt.h"

#include <optional>
#include <string>
#include <string_view>
#include <cstdio>
#include <type_traits>

#include "00_core/assert_spec.h"
#include "04_analysis/composite/function_types.h"
#include "04_analysis/memory/regions.h"
#include "04_analysis/resolve/scopes.h"
#include "00_core/process_config.h"
#include "04_analysis/typing/subtyping.h"
#include "04_analysis/typing/type_expr.h"
#include "04_analysis/typing/type_infer.h"
#include "04_analysis/typing/if_case_check.h"
#include "02_source/ast/ast.h"

namespace cursive::analysis {

ExprTypeResult TypeExpr(const ScopeContext& ctx,
                        const StmtTypeContext& type_ctx,
                        const ast::ExprPtr& expr,
                        const TypeEnv& env);
PlaceTypeResult TypePlace(const ScopeContext& ctx,
                          const StmtTypeContext& type_ctx,
                          const ast::ExprPtr& expr,
                          const TypeEnv& env);

namespace {

static inline void SpecDefsAssignStmt() {
  SPEC_DEF("T-Assign", "5.2.11");
  SPEC_DEF("Assign-NotPlace", "5.2.11");
  SPEC_DEF("Assign-Immutable-Err", "5.2.11");
  SPEC_DEF("Assign-Type-Err", "5.2.11");
  SPEC_DEF("Assign-Const-Err", "5.2.11");
  SPEC_DEF("PlaceRoot", "5.2.11");
}

static std::optional<std::string_view> PlaceRootName(const ast::ExprPtr& expr) {
  if (!expr) {
    return std::nullopt;
  }
  if (const auto* ident = std::get_if<ast::IdentifierExpr>(&expr->node)) {
    return ident->name;
  }
  if (const auto* field = std::get_if<ast::FieldAccessExpr>(&expr->node)) {
    return PlaceRootName(field->base);
  }
  if (const auto* tup = std::get_if<ast::TupleAccessExpr>(&expr->node)) {
    return PlaceRootName(tup->base);
  }
  if (const auto* idx = std::get_if<ast::IndexAccessExpr>(&expr->node)) {
    return PlaceRootName(idx->base);
  }
  if (const auto* deref = std::get_if<ast::DerefExpr>(&expr->node)) {
    return PlaceRootName(deref->value);
  }
  return std::nullopt;
}

static void UpdateAssignedBindingProvenance(TypeEnv& env,
                                            std::string_view name,
                                            const ProvExprTrackResult& prov) {
  const auto key = IdKeyOf(name);
  for (auto it = env.scopes.rbegin(); it != env.scopes.rend(); ++it) {
    const auto found = it->find(key);
    if (found == it->end()) {
      continue;
    }
    ApplyBindingProvenanceSeed(found->second, prov.kind, prov.region);
    return;
  }
}

static bool IsPlaceExprNode(const ast::ExprNode& node) {
  if (std::holds_alternative<ast::IdentifierExpr>(node)) {
    return true;
  }
  if (const auto* field = std::get_if<ast::FieldAccessExpr>(&node)) {
    return field->base && IsPlaceExprNode(field->base->node);
  }
  if (const auto* tup = std::get_if<ast::TupleAccessExpr>(&node)) {
    return tup->base && IsPlaceExprNode(tup->base->node);
  }
  if (const auto* idx = std::get_if<ast::IndexAccessExpr>(&node)) {
    return idx->base && IsPlaceExprNode(idx->base->node);
  }
  if (std::holds_alternative<ast::DerefExpr>(node)) {
    return true;
  }
  return false;
}

static bool IsPlaceExprLocal(const ast::ExprPtr& expr) {
  if (!expr) {
    return false;
  }
  return IsPlaceExprNode(expr->node);
}

struct RootMutabilityResult {
  bool ok = true;
  std::optional<std::string_view> diag_id;
  std::optional<ast::Mutability> mut;
};

static RootMutabilityResult LookupRootMutability(const ScopeContext& ctx,
                                                 const TypeEnv& env,
                                                 std::string_view name) {
  if (const auto local_mut = MutOf(env, name)) {
    return {true, std::nullopt, local_mut};
  }

  const auto static_lookup = LookupModuleStatic(ctx, ctx.current_module, name);
  if (!static_lookup.ok) {
    return {false, static_lookup.diag_id, std::nullopt};
  }
  if (static_lookup.type) {
    return {true, std::nullopt,
            static_lookup.is_mutable
                ? std::optional<ast::Mutability>{ast::Mutability::Var}
                : std::optional<ast::Mutability>{ast::Mutability::Let}};
  }

  return {true, std::nullopt, std::nullopt};
}

static std::string ExprKindName(const ast::ExprPtr& expr) {
  if (!expr) {
    return "null";
  }
  return std::visit(
      [&](const auto& node) -> std::string {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, ast::IdentifierExpr>) {
          return "IdentifierExpr(" + node.name + ")";
        } else if constexpr (std::is_same_v<T, ast::DerefExpr>) {
          return "DerefExpr";
        } else if constexpr (std::is_same_v<T, ast::FieldAccessExpr>) {
          return "FieldAccessExpr";
        } else if constexpr (std::is_same_v<T, ast::TupleAccessExpr>) {
          return "TupleAccessExpr";
        } else if constexpr (std::is_same_v<T, ast::IndexAccessExpr>) {
          return "IndexAccessExpr";
        } else if constexpr (std::is_same_v<T, ast::AttributedExpr>) {
          return "AttributedExpr";
        } else {
          return "Expr";
        }
      },
      expr->node);
}

static ExprTypeResult TypeExprWithCurrentEnv(const ScopeContext& ctx,
                                             const StmtTypeContext& type_ctx,
                                             const TypeEnv& env,
                                             const ExprTypeFn& type_expr,
                                             const ast::ExprPtr& expr) {
  if (!expr) {
    return {};
  }
  const auto via_callback = type_expr(expr);
  if (via_callback.ok) {
    return via_callback;
  }
  const auto via_env = TypeExpr(ctx, type_ctx, expr, env);
  if (via_env.ok || via_env.diag_id.has_value()) {
    return via_env;
  }
  return via_callback;
}

static PlaceTypeResult TypePlaceWithCurrentEnv(const ScopeContext& ctx,
                                               const StmtTypeContext& type_ctx,
                                               const TypeEnv& env,
                                               const PlaceTypeFn& type_place,
                                               const ast::ExprPtr& expr) {
  if (!expr) {
    return {};
  }
  if (const auto* ident = std::get_if<ast::IdentifierExpr>(&expr->node)) {
    const auto binding = BindOf(env, ident->name);
    if (binding.has_value()) {
      return {true, std::nullopt, binding->type};
    }
  }
  const auto via_callback = type_place(expr);
  if (via_callback.ok) {
    return via_callback;
  }
  const auto via_env = TypePlace(ctx, type_ctx, expr, env);
  if (via_env.ok || via_env.diag_id.has_value()) {
    return via_env;
  }
  return via_callback;
}

static IdentTypeFn IdentTypeWithCurrentEnv(const ScopeContext& ctx,
                                           const TypeEnv& env,
                                           const IdentTypeFn& type_ident) {
  (void)ctx;
  return [&](std::string_view name) -> ExprTypeResult {
    const auto binding = BindOf(env, name);
    if (binding.has_value()) {
      ExprTypeResult local;
      local.ok = true;
      local.type = binding->type;
      return local;
    }
    return type_ident(name);
  };
}

}  // namespace

StmtTypeResult TypeAssignStmt(const ScopeContext& ctx,
                              const StmtTypeContext& type_ctx,
                              const ast::AssignStmt& node,
                              const TypeEnv& env,
                              const ExprTypeFn& type_expr,
                              const IdentTypeFn& type_ident,
                              const PlaceTypeFn& type_place) {
  SpecDefsAssignStmt();

  // Check that the target is a place expression (lvalue)
  if (!IsPlaceExprLocal(node.place)) {
    SPEC_RULE("Assign-NotPlace");
    return {false, "Assign-NotPlace", {}, {}};
  }

  // Type the place expression
  auto type_place_current = [&](const ast::ExprPtr& inner) {
    return TypePlaceWithCurrentEnv(ctx, type_ctx, env, type_place, inner);
  };
  const auto place_type = type_place_current(node.place);
  if (!place_type.ok) {
    std::string detail = place_type.diag_detail;
    if (detail.empty()) {
      detail = "assign-place-kind=" + ExprKindName(node.place);
    } else {
      detail += "; assign-place-kind=" + ExprKindName(node.place);
    }
    if (node.place) {
      if (const auto* deref = std::get_if<ast::DerefExpr>(&node.place->node)) {
        if (deref->value) {
          if (const auto* ident =
                  std::get_if<ast::IdentifierExpr>(&deref->value->node)) {
            const auto binding = BindOf(env, ident->name);
            detail += "; deref-ident=" + ident->name;
            detail += "; deref-ident-binding=";
            detail += binding.has_value() ? "present" : "absent";
          }
        }
      }
    }
    return {false, place_type.diag_id, {}, {}, detail};
  }

  // Check for const permission
  if (const auto* perm = std::get_if<TypePerm>(&place_type.type->node)) {
    if (perm->perm == Permission::Const) {
      SPEC_RULE("Assign-Const-Err");
      return {false, "Assign-Const-Err", {}, {}};
    }
  }

  bool shared_write_with_key = false;
  if (const auto* perm = std::get_if<TypePerm>(&place_type.type->node)) {
    if (perm->perm == Permission::Shared) {
      const bool has_write_key =
          type_ctx.keys_held &&
          type_ctx.key_mode.has_value() &&
          *type_ctx.key_mode == ast::KeyMode::Write;
      if (!has_write_key) {
        return {false, "E-TYP-1604", {}, {}};
      }
      shared_write_with_key = true;
    }
  }

  // Find the root of the place and check mutability
  const auto root = PlaceRootName(node.place);
  if (root.has_value()) {
    const auto root_mut = LookupRootMutability(ctx, env, *root);
    if (!root_mut.ok) {
      return {false, root_mut.diag_id, {}, {}};
    }
    if (!shared_write_with_key &&
        root_mut.mut.has_value() &&
        *root_mut.mut == ast::Mutability::Let) {
      SPEC_RULE("Assign-Immutable-Err");
      return {false, "Assign-Immutable-Err", {}, {}};
    }
  }

  // Assignment checks compare against the stored value type, not the access
  // permission wrapper.
  TypeRef assign_target_type = place_type.type;
  if (const auto* perm = std::get_if<TypePerm>(&place_type.type->node)) {
    assign_target_type = perm->base;
  }

  // Type check the value against the place type
  const auto check =
      CheckExprAgainst(ctx, type_ctx, node.value, assign_target_type, env);
  if (!check.ok) {
    if (core::IsDebugEnabled("sema") || core::IsDebugEnabled("pipeline")) {
      const auto inferred_dbg =
          InferExpr(ctx, node.value,
                    [&](const ast::ExprPtr& inner) {
                      return TypeExprWithCurrentEnv(ctx, type_ctx, env, type_expr,
                                                    inner);
                    },
                    type_place_current,
                    IdentTypeWithCurrentEnv(ctx, env, type_ident));
      if (inferred_dbg.ok) {
        const std::string expected_dbg = TypeToString(place_type.type);
        const std::string inferred_dbg_text = TypeToString(inferred_dbg.type);
        const std::string diag_dbg =
            check.diag_id.has_value() ? std::string(*check.diag_id) : "<none>";
        std::fprintf(stderr,
                     "[assign-check-fail] %s:%zu:%zu expected=%s inferred=%s diag=%s\n",
                     node.span.file.c_str(),
                     node.span.start_line,
                     node.span.start_col,
                     expected_dbg.c_str(),
                     inferred_dbg_text.c_str(),
                     diag_dbg.c_str());
      } else {
        const std::string expected_dbg = TypeToString(place_type.type);
        const std::string diag_dbg =
            check.diag_id.has_value() ? std::string(*check.diag_id) : "<none>";
        std::fprintf(stderr,
                     "[assign-check-fail] %s:%zu:%zu expected=%s inferred=<infer-failed> diag=%s\n",
                     node.span.file.c_str(),
                     node.span.start_line,
                     node.span.start_col,
                     expected_dbg.c_str(),
                     diag_dbg.c_str());
      }
    }
    if (!check.diag_id.has_value()) {
      SPEC_RULE("Assign-Type-Err");
      return {false, "Assign-Type-Err", {}, {}};
    }
    return {false, check.diag_id, {}, {}};
  }

  TypeEnv out_env = env;
  if (root.has_value()) {
    const auto value_prov = TrackExprProvenance(ctx, node.value, env);
    if (value_prov.ok) {
      UpdateAssignedBindingProvenance(out_env, *root, value_prov);
    }
  }

  SPEC_RULE("T-Assign");
  return {true, std::nullopt, std::move(out_env), {}};
}

}  // namespace cursive::analysis
