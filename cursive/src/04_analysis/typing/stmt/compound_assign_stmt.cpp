// =============================================================================
// compound_assign_stmt.cpp - Compound assignment statement typing
// =============================================================================
//
// SPEC REFERENCE: CursiveSpecification.md
//   Section 5.2.9: Assignment Statements
//   - Compound assignment operators: +=, -=, *=, /=, %=, &=, |:, ^=, <<=, >>=
//   - T-CompoundAssign (lines 9462-9465): Compound assignment
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_stmt.cpp
//
// =============================================================================

#include "04_analysis/typing/type_stmt.h"

#include <array>
#include <optional>
#include <string_view>

#include "00_core/assert_spec.h"
#include "04_analysis/composite/function_types.h"
#include "04_analysis/typing/subtyping.h"
#include "02_source/ast/ast.h"

namespace cursive::analysis
{

  ExprTypeResult TypeExpr(const ScopeContext &ctx,
                          const StmtTypeContext &type_ctx,
                          const ast::ExprPtr &expr,
                          const TypeEnv &env);

  namespace
  {
    struct RootMutabilityResult
    {
      bool ok = true;
      std::optional<std::string_view> diag_id;
      std::optional<ast::Mutability> mut;
    };

    static inline void SpecDefsCompoundAssignStmt()
    {
      SPEC_DEF("T-CompoundAssign", "5.2.11");
      SPEC_DEF("Assign-NotPlace", "5.2.11");
      SPEC_DEF("Assign-Immutable-Err", "5.2.11");
      SPEC_DEF("Assign-Type-Err", "5.2.11");
      SPEC_DEF("Assign-Const-Err", "5.2.11");
    }

    static std::optional<std::string_view> PlaceRootName(const ast::ExprPtr &expr)
    {
      if (!expr)
      {
        return std::nullopt;
      }
      if (const auto *ident = std::get_if<ast::IdentifierExpr>(&expr->node))
      {
        return ident->name;
      }
      if (const auto *field = std::get_if<ast::FieldAccessExpr>(&expr->node))
      {
        return PlaceRootName(field->base);
      }
      if (const auto *tup = std::get_if<ast::TupleAccessExpr>(&expr->node))
      {
        return PlaceRootName(tup->base);
      }
      if (const auto *idx = std::get_if<ast::IndexAccessExpr>(&expr->node))
      {
        return PlaceRootName(idx->base);
      }
      if (const auto *deref = std::get_if<ast::DerefExpr>(&expr->node))
      {
        return PlaceRootName(deref->value);
      }
      return std::nullopt;
    }

    static bool IsPlaceExprNode(const ast::ExprNode &node)
    {
      if (std::holds_alternative<ast::IdentifierExpr>(node))
      {
        return true;
      }
      if (const auto *field = std::get_if<ast::FieldAccessExpr>(&node))
      {
        return field->base && IsPlaceExprNode(field->base->node);
      }
      if (const auto *tup = std::get_if<ast::TupleAccessExpr>(&node))
      {
        return tup->base && IsPlaceExprNode(tup->base->node);
      }
      if (const auto *idx = std::get_if<ast::IndexAccessExpr>(&node))
      {
        return idx->base && IsPlaceExprNode(idx->base->node);
      }
      if (std::holds_alternative<ast::DerefExpr>(node))
      {
        return true;
      }
      return false;
    }

    static bool IsPlaceExpr(const ast::ExprPtr &expr)
    {
      if (!expr)
      {
        return false;
      }
      return IsPlaceExprNode(expr->node);
    }

    static RootMutabilityResult LookupRootMutability(const ScopeContext &ctx,
                                                     const TypeEnv &env,
                                                     std::string_view name)
    {
      if (const auto local_mut = MutOf(env, name))
      {
        return {true, std::nullopt, local_mut};
      }

      const auto static_lookup = LookupModuleStatic(ctx, ctx.current_module, name);
      if (!static_lookup.ok)
      {
        return {false, static_lookup.diag_id, std::nullopt};
      }
      if (static_lookup.type)
      {
        return {true, std::nullopt,
                static_lookup.is_mutable
                    ? std::optional<ast::Mutability>{ast::Mutability::Var}
                    : std::optional<ast::Mutability>{ast::Mutability::Let}};
      }

      return {true, std::nullopt, std::nullopt};
    }

    static TypeRef StripPermOnce(const TypeRef &type)
    {
      if (!type)
      {
        return type;
      }
      if (const auto *perm = std::get_if<TypePerm>(&type->node))
      {
        return perm->base;
      }
      return type;
    }

    static constexpr std::array<std::string_view, 12> kIntTypes = {
        "i8", "i16", "i32", "i64", "i128", "isize",
        "u8", "u16", "u32", "u64", "u128", "usize"};
    static constexpr std::array<std::string_view, 3> kFloatTypes = {"f16", "f32",
                                                                    "f64"};

    static bool IsNumericType(const TypeRef &type)
    {
      const auto stripped = StripPermOnce(type);
      if (!stripped)
      {
        return false;
      }
      const auto *prim = std::get_if<TypePrim>(&stripped->node);
      if (!prim)
      {
        return false;
      }
      for (const auto &t : kIntTypes)
      {
        if (prim->name == t)
        {
          return true;
        }
      }
      for (const auto &t : kFloatTypes)
      {
        if (prim->name == t)
        {
          return true;
        }
      }
      return false;
    }

    static ExprTypeResult TypeExprWithCurrentEnv(const ScopeContext &ctx,
                                                 const StmtTypeContext &type_ctx,
                                                 const TypeEnv &env,
                                                 const ExprTypeFn &type_expr,
                                                 const ast::ExprPtr &expr)
    {
      if (!expr)
      {
        return {};
      }
      const auto via_callback = type_expr(expr);
      if (via_callback.ok)
      {
        return via_callback;
      }
      const auto via_env = TypeExpr(ctx, type_ctx, expr, env);
      if (via_env.ok || via_env.diag_id.has_value())
      {
        return via_env;
      }
      return via_callback;
    }

  } // namespace

  StmtTypeResult TypeCompoundAssignStmt(const ScopeContext &ctx,
                                        const StmtTypeContext &type_ctx,
                                        const ast::CompoundAssignStmt &node,
                                        const TypeEnv &env,
                                        const ExprTypeFn &type_expr,
                                        const IdentTypeFn &type_ident,
                                        const PlaceTypeFn &type_place)
  {
    (void)type_ident; // Reserved for future use
    SpecDefsCompoundAssignStmt();

    // Check that the target is a place expression (lvalue)
    if (!IsPlaceExpr(node.place))
    {
      SPEC_RULE("Assign-NotPlace");
      return {false, "Assign-NotPlace", {}, {}};
    }

    // Type the place expression
    const auto place_type = type_place(node.place);
    if (!place_type.ok)
    {
      return {false, place_type.diag_id, {}, {}};
    }

    // Check for const permission
    if (const auto *perm = std::get_if<TypePerm>(&place_type.type->node))
    {
      if (perm->perm == Permission::Const)
      {
        SPEC_RULE("Assign-Const-Err");
        return {false, "Assign-Const-Err", {}, {}};
      }
    }

    TypeRef assign_target_type = place_type.type;
    bool shared_write_with_key = false;
    if (const auto *perm = std::get_if<TypePerm>(&place_type.type->node))
    {
      if (perm->perm == Permission::Shared)
      {
        const bool has_write_key =
            type_ctx.keys_held &&
            type_ctx.key_mode.has_value() &&
            *type_ctx.key_mode == ast::KeyMode::Write;
        if (!has_write_key)
        {
          return {false, "E-TYP-1604", {}, {}};
        }
        shared_write_with_key = true;
      }
      assign_target_type = perm->base;
    }

    // Find the root of the place and check mutability
    const auto root = PlaceRootName(node.place);
    if (root.has_value())
    {
      const auto root_mut = LookupRootMutability(ctx, env, *root);
      if (!root_mut.ok)
      {
        return {false, root_mut.diag_id, {}, {}};
      }
      if (!shared_write_with_key &&
          root_mut.mut.has_value() &&
          *root_mut.mut == ast::Mutability::Let)
      {
        SPEC_RULE("Assign-Immutable-Err");
        return {false, "Assign-Immutable-Err", {}, {}};
      }
    }

    // Compound assignment requires numeric type
    if (!IsNumericType(assign_target_type))
    {
      SPEC_RULE("Assign-Type-Err");
      return {false, "Assign-Type-Err", {}, {}};
    }

    // Type the RHS expression
    const auto rhs_type =
        TypeExprWithCurrentEnv(ctx, type_ctx, env, type_expr, node.value);
    if (!rhs_type.ok)
    {
      return {false, rhs_type.diag_id, {}, {}};
    }

    // Check that RHS is subtype of place's base type
    const auto sub = Subtyping(ctx, rhs_type.type, assign_target_type);
    if (!sub.ok)
    {
      return {false, sub.diag_id, {}, {}};
    }
    if (!sub.subtype)
    {
      SPEC_RULE("Assign-Type-Err");
      return {false, "Assign-Type-Err", {}, {}};
    }

    SPEC_RULE("T-CompoundAssign");
    return {true, std::nullopt, env, {}};
  }

} // namespace cursive::analysis
