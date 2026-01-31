// =================================================================
// File: 03_analysis/types/expr/path.cpp
// Construct: Path and Identifier Expression Type Checking
// Spec Section: 5.2.12
// Spec Rules: T-Ident, P-Ident, T-Path-Value, ValueUse-NonBitcopyPlace
// =================================================================
#include "cursive0/03_analysis/types/expr/path.h"

#include "cursive0/00_core/assert_spec.h"
#include "cursive0/03_analysis/composite/function_types.h"
#include "cursive0/03_analysis/resolve/scopes.h"
#include "cursive0/03_analysis/resolve/scopes_lookup.h"
#include "cursive0/03_analysis/types/type_expr.h"

namespace cursive0::analysis::expr {

namespace {

static inline void SpecDefsPath() {
  SPEC_DEF("T-Ident", "5.2.12");
  SPEC_DEF("P-Ident", "5.2.12");
  SPEC_DEF("T-Path-Value", "5.2.12");
  SPEC_DEF("ValueUse-NonBitcopyPlace", "5.2.12");
  SPEC_DEF("ResolveExpr-Ident-Err", "5.2.12");
}

}  // namespace

// (T-Ident) and (P-Ident)
ExprTypeResult TypeIdentifierExprImpl(const ScopeContext& ctx,
                                      const syntax::IdentifierExpr& expr,
                                      const TypeEnv& env) {
  SpecDefsPath();
  ExprTypeResult result;

  const auto binding = BindOf(env, expr.name);
  if (binding.has_value()) {
    if (BitcopyType(ctx, binding->type)) {
      SPEC_RULE("T-Ident");
      result.ok = true;
      result.type = binding->type;
      return result;
    }
    SPEC_RULE("ValueUse-NonBitcopyPlace");
    result.diag_id = "ValueUse-NonBitcopyPlace";
    return result;
  }

  const auto ent = ResolveValueName(ctx, expr.name);
  if (ent.has_value() && ent->origin_opt.has_value()) {
    const auto target_name = ent->target_opt.value_or(expr.name);
    const auto value_type =
        ValuePathType(ctx, *ent->origin_opt, target_name);
    if (!value_type.ok) {
      result.diag_id = value_type.diag_id;
      return result;
    }
    if (value_type.type) {
      if (BitcopyType(ctx, value_type.type)) {
        SPEC_RULE("T-Ident");
        result.ok = true;
        result.type = value_type.type;
        return result;
      }
      SPEC_RULE("ValueUse-NonBitcopyPlace");
      result.diag_id = "ValueUse-NonBitcopyPlace";
      return result;
    }
  }

  result.diag_id = "ResolveExpr-Ident-Err";
  return result;
}

PlaceTypeResult TypeIdentifierPlaceImpl(const ScopeContext& /*ctx*/,
                                        const syntax::IdentifierExpr& expr,
                                        const TypeEnv& env) {
  SpecDefsPath();
  PlaceTypeResult result;

  const auto binding = BindOf(env, expr.name);
  if (binding.has_value()) {
    SPEC_RULE("P-Ident");
    result.ok = true;
    result.type = binding->type;
    return result;
  }

  result.diag_id = "ResolveExpr-Ident-Err";
  return result;
}

// (T-Path-Value)
ExprTypeResult TypePathExprImpl(const ScopeContext& ctx,
                                const syntax::PathExpr& expr) {
  SpecDefsPath();
  ExprTypeResult result;

  const auto typed = ValuePathType(ctx, expr.path, expr.name);
  if (!typed.ok) {
    result.diag_id = typed.diag_id;
    return result;
  }
  if (!typed.type) {
    result.diag_id = "ResolveExpr-Ident-Err";
    return result;
  }

  SPEC_RULE("T-Path-Value");
  result.ok = true;
  result.type = typed.type;
  return result;
}

}  // namespace cursive0::analysis::expr
