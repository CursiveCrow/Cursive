// =================================================================
// File: 03_analysis/types/expr/deref.cpp
// Construct: Dereference Expression Type Checking
// Spec Section: 5.2.12
// Spec Rules: T-Deref-Ptr, T-Deref-Raw, P-Deref-Ptr, P-Deref-Raw-Imm,
//             P-Deref-Raw-Mut, Deref-Null, Deref-Expired, Deref-Raw-Unsafe
// =================================================================
#include "cursive0/03_analysis/types/expr/deref.h"

#include "cursive0/00_core/assert_spec.h"
#include "cursive0/03_analysis/types/type_expr.h"

namespace cursive0::analysis::expr {

// (T-Deref-Ptr), (T-Deref-Raw)
ExprTypeResult TypeDerefExprImpl(const ScopeContext& ctx,
                                 const StmtTypeContext& type_ctx,
                                 const syntax::DerefExpr& expr,
                                 const TypeEnv& env,
                                 const core::Span& span) {
  ExprTypeResult result;

  const auto ptr = TypeExpr(ctx, type_ctx, expr.value, env);
  if (!ptr.ok) {
    result.diag_id = ptr.diag_id;
    return result;
  }

  const auto stripped = StripPerm(ptr.type);
  if (!stripped) {
    return result;
  }

  if (const auto* type_ptr = std::get_if<TypePtr>(&stripped->node)) {
    if (type_ptr->state == PtrState::Null) {
      SPEC_RULE("Deref-Null");
      result.diag_id = "Deref-Null";
      return result;
    }
    if (type_ptr->state == PtrState::Expired) {
      SPEC_RULE("Deref-Expired");
      result.diag_id = "Deref-Expired";
      return result;
    }
    if (!BitcopyType(ctx, type_ptr->element)) {
      SPEC_RULE("ValueUse-NonBitcopyPlace");
      result.diag_id = "ValueUse-NonBitcopyPlace";
      return result;
    }
    SPEC_RULE("T-Deref-Ptr");
    result.ok = true;
    result.type = type_ptr->element;
    return result;
  }

  if (const auto* raw_ptr = std::get_if<TypeRawPtr>(&stripped->node)) {
    if (!IsInUnsafeSpan(ctx, span)) {
      SPEC_RULE("Deref-Raw-Unsafe");
      result.diag_id = "Deref-Raw-Unsafe";
      return result;
    }
    if (!BitcopyType(ctx, raw_ptr->element)) {
      SPEC_RULE("ValueUse-NonBitcopyPlace");
      result.diag_id = "ValueUse-NonBitcopyPlace";
      return result;
    }
    SPEC_RULE("T-Deref-Raw");
    result.ok = true;
    result.type = raw_ptr->element;
    return result;
  }

  return result;
}

// (P-Deref-Ptr), (P-Deref-Raw-Imm), (P-Deref-Raw-Mut)
PlaceTypeResult TypeDerefPlaceImpl(const ScopeContext& ctx,
                                   const StmtTypeContext& type_ctx,
                                   const syntax::DerefExpr& expr,
                                   const TypeEnv& env,
                                   const core::Span& span) {
  PlaceTypeResult result;

  const auto ptr = TypeExpr(ctx, type_ctx, expr.value, env);
  if (!ptr.ok) {
    result.diag_id = ptr.diag_id;
    return result;
  }

  const auto stripped = StripPerm(ptr.type);
  if (!stripped) {
    return result;
  }

  if (const auto* type_ptr = std::get_if<TypePtr>(&stripped->node)) {
    if (type_ptr->state == PtrState::Null) {
      SPEC_RULE("Deref-Null");
      result.diag_id = "Deref-Null";
      return result;
    }
    if (type_ptr->state == PtrState::Expired) {
      SPEC_RULE("Deref-Expired");
      result.diag_id = "Deref-Expired";
      return result;
    }
    SPEC_RULE("P-Deref-Ptr");
    result.ok = true;
    result.type = type_ptr->element;
    return result;
  }

  if (const auto* raw_ptr = std::get_if<TypeRawPtr>(&stripped->node)) {
    if (!IsInUnsafeSpan(ctx, span)) {
      SPEC_RULE("Deref-Raw-Unsafe");
      result.diag_id = "Deref-Raw-Unsafe";
      return result;
    }
    if (raw_ptr->qual == RawPtrQual::Imm) {
      SPEC_RULE("P-Deref-Raw-Imm");
      result.ok = true;
      result.type = MakeTypePerm(Permission::Const, raw_ptr->element);
      return result;
    }
    SPEC_RULE("P-Deref-Raw-Mut");
    result.ok = true;
    result.type = MakeTypePerm(Permission::Unique, raw_ptr->element);
    return result;
  }

  return result;
}

}  // namespace cursive0::analysis::expr
