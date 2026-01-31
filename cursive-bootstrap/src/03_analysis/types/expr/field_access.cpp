// =================================================================
// File: 03_analysis/types/expr/field_access.cpp
// Construct: Field Access Expression Type Checking
// Spec Section: 5.2.12
// Spec Rules: T-Field-Record, T-Field-Record-Perm, P-Field-Record,
//             P-Field-Record-Perm, FieldAccess-Unknown, FieldAccess-NotVisible,
//             Modal-Field-Missing, Modal-Field-NotVisible, Modal-Field-General-Err
// =================================================================
#include "cursive0/03_analysis/types/expr/field_access.h"

#include "cursive0/00_core/assert_spec.h"
#include "cursive0/03_analysis/generics/monomorphize.h"
#include "cursive0/03_analysis/modal/modal.h"
#include "cursive0/03_analysis/modal/modal_fields.h"
#include "cursive0/03_analysis/types/type_expr.h"
#include "cursive0/03_analysis/types/type_lookup.h"
#include "cursive0/03_analysis/types/type_lower.h"

namespace cursive0::analysis::expr {

using namespace cursive0::analysis;

// (T-Field-Record), (T-Field-Record-Perm), (FieldAccess-*)
ExprTypeResult TypeFieldAccessExprImpl(const ScopeContext& ctx,
                                       const StmtTypeContext& type_ctx,
                                       const syntax::FieldAccessExpr& expr,
                                       const TypeEnv& env) {
  ExprTypeResult result;

  // Field access uses place typing for the base expression, since accessing
  // a field doesn't consume the base - it just reads through it. This allows
  // field access on non-Bitcopy types (e.g., self.field in methods).
  const auto place_result = TypePlace(ctx, type_ctx, expr.base, env);
  TypeRef base_type;
  if (place_result.ok) {
    base_type = place_result.type;
  } else {
    // Fall back to value typing for non-place expressions
    const auto base_result = TypeExpr(ctx, type_ctx, expr.base, env);
    if (!base_result.ok) {
      result.diag_id = base_result.diag_id;
      return result;
    }
    base_type = base_result.type;
  }

  const auto stripped = StripPerm(base_type);
  if (!stripped) {
    return result;
  }

  if (std::holds_alternative<TypeUnion>(stripped->node)) {
    SPEC_RULE("Union-DirectAccess-Err");
    result.diag_id = "Union-DirectAccess-Err";
    return result;
  }
  if (std::holds_alternative<TypeOpaque>(stripped->node)) {
    SPEC_RULE("T-Opaque-Project");
    result.diag_id = "E-TYP-2510";
    return result;
  }

  if (const auto* modal_state = std::get_if<TypeModalState>(&stripped->node)) {
    const auto* modal = LookupModalDecl(ctx, modal_state->path);
    if (!modal) {
      return result;
    }
    const auto* field_decl =
        LookupModalFieldDecl(*modal, modal_state->state, expr.name);
    if (!field_decl) {
      SPEC_RULE("Modal-Field-Missing");
      result.diag_id = "Modal-Field-Missing";
      return result;
    }
    if (!ModalFieldVisible(ctx, modal_state->path)) {
      SPEC_RULE("Modal-Field-NotVisible");
      result.diag_id = "Modal-Field-NotVisible";
      return result;
    }
    const auto lowered = LowerType(ctx, field_decl->type);
    if (!lowered.ok) {
      return result;
    }
    TypeRef field_type = lowered.type;
    if (const auto* perm_type = std::get_if<TypePerm>(&base_type->node)) {
      field_type = MakeTypePerm(perm_type->perm, field_type);
      if (BitcopyType(ctx, field_type)) {
        SPEC_RULE("T-Modal-Field-Perm");
        result.ok = true;
        result.type = field_type;
        return result;
      }
      SPEC_RULE("ValueUse-NonBitcopyPlace");
      result.diag_id = "ValueUse-NonBitcopyPlace";
      return result;
    }

    if (BitcopyType(ctx, field_type)) {
      SPEC_RULE("T-Modal-Field");
      result.ok = true;
      result.type = field_type;
      return result;
    }

    SPEC_RULE("ValueUse-NonBitcopyPlace");
    result.diag_id = "ValueUse-NonBitcopyPlace";
    return result;
  }

  const auto* path_type = std::get_if<TypePathType>(&stripped->node);
  if (!path_type) {
    return result;
  }

  if (LookupModalDecl(ctx, path_type->path)) {
    SPEC_RULE("Modal-Field-General-Err");
    result.diag_id = "Modal-Field-General-Err";
    return result;
  }

  if (LookupEnumDecl(ctx, path_type->path)) {
    SPEC_RULE("FieldAccess-Enum");
    result.diag_id = "FieldAccess-Unknown";
    return result;
  }

  const auto* record = LookupRecordDecl(ctx, path_type->path);
  if (!record) {
    return result;
  }

  if (!FieldExists(*record, expr.name)) {
    SPEC_RULE("FieldAccess-Unknown");
    result.diag_id = "FieldAccess-Unknown";
    return result;
  }

  if (!FieldVisible(ctx, *record, expr.name, path_type->path)) {
    SPEC_RULE("FieldAccess-NotVisible");
    result.diag_id = "FieldAccess-NotVisible";
    return result;
  }

  const auto field_type_opt = FieldType(*record, expr.name, ctx);
  if (!field_type_opt.has_value()) {
    return result;
  }

  TypeRef field_type = *field_type_opt;

  // §13.1 Instantiate: Substitute type parameters in field type
  // e.g., for Container<i32>, substitute T -> i32 in field type "T"
  if (record->generic_params.has_value() && !path_type->generic_args.empty()) {
    TypeSubst type_subst = BuildSubstitution(record->generic_params->params,
                                             path_type->generic_args);
    field_type = InstantiateType(field_type, type_subst);
  }

  if (const auto* perm_type = std::get_if<TypePerm>(&base_type->node)) {
    field_type = MakeTypePerm(perm_type->perm, field_type);
    if (BitcopyType(ctx, field_type)) {
      SPEC_RULE("T-Field-Record-Perm");
      result.ok = true;
      result.type = field_type;
      return result;
    }
    SPEC_RULE("ValueUse-NonBitcopyPlace");
    result.diag_id = "ValueUse-NonBitcopyPlace";
    return result;
  }

  if (BitcopyType(ctx, field_type)) {
    SPEC_RULE("T-Field-Record");
    result.ok = true;
    result.type = field_type;
    return result;
  }

  SPEC_RULE("ValueUse-NonBitcopyPlace");
  result.diag_id = "ValueUse-NonBitcopyPlace";
  return result;
}


PlaceTypeResult TypeFieldAccessPlaceImpl(const ScopeContext& ctx,
                                         const StmtTypeContext& type_ctx,
                                         const syntax::FieldAccessExpr& expr,
                                         const TypeEnv& env) {
  PlaceTypeResult result;

  const auto base_result = TypePlace(ctx, type_ctx, expr.base, env);
  if (!base_result.ok) {
    result.diag_id = base_result.diag_id;
    return result;
  }

  const auto stripped = StripPerm(base_result.type);
  if (!stripped) {
    return result;
  }

  if (std::holds_alternative<TypeUnion>(stripped->node)) {
    SPEC_RULE("Union-DirectAccess-Err");
    result.diag_id = "Union-DirectAccess-Err";
    return result;
  }

  if (const auto* modal_state = std::get_if<TypeModalState>(&stripped->node)) {
    const auto* modal = LookupModalDecl(ctx, modal_state->path);
    if (!modal) {
      return result;
    }
    const auto* field_decl =
        LookupModalFieldDecl(*modal, modal_state->state, expr.name);
    if (!field_decl) {
      SPEC_RULE("Modal-Field-Missing");
      result.diag_id = "Modal-Field-Missing";
      return result;
    }
    if (!ModalFieldVisible(ctx, modal_state->path)) {
      SPEC_RULE("Modal-Field-NotVisible");
      result.diag_id = "Modal-Field-NotVisible";
      return result;
    }
    const auto lowered = LowerType(ctx, field_decl->type);
    if (!lowered.ok) {
      return result;
    }
    TypeRef field_type = lowered.type;
    if (const auto* perm_type = std::get_if<TypePerm>(&base_result.type->node)) {
      field_type = MakeTypePerm(perm_type->perm, field_type);
    }
    result.ok = true;
    result.type = field_type;
    return result;
  }

  const auto* path_type = std::get_if<TypePathType>(&stripped->node);
  if (!path_type) {
    return result;
  }

  if (LookupModalDecl(ctx, path_type->path)) {
    SPEC_RULE("Modal-Field-General-Err");
    result.diag_id = "Modal-Field-General-Err";
    return result;
  }

  if (LookupEnumDecl(ctx, path_type->path)) {
    SPEC_RULE("FieldAccess-Enum");
    result.diag_id = "FieldAccess-Unknown";
    return result;
  }

  const auto* record = LookupRecordDecl(ctx, path_type->path);
  if (!record) {
    return result;
  }

  if (!FieldExists(*record, expr.name)) {
    SPEC_RULE("FieldAccess-Unknown");
    result.diag_id = "FieldAccess-Unknown";
    return result;
  }

  if (!FieldVisible(ctx, *record, expr.name, path_type->path)) {
    SPEC_RULE("FieldAccess-NotVisible");
    result.diag_id = "FieldAccess-NotVisible";
    return result;
  }

  const auto field_type_opt = FieldType(*record, expr.name, ctx);
  if (!field_type_opt.has_value()) {
    return result;
  }

  TypeRef field_type = *field_type_opt;

  // §13.1 Instantiate: Substitute type parameters in field type
  // e.g., for Container<i32>, substitute T -> i32 in field type "T"
  if (record->generic_params.has_value() && !path_type->generic_args.empty()) {
    TypeSubst type_subst = BuildSubstitution(record->generic_params->params,
                                             path_type->generic_args);
    field_type = InstantiateType(field_type, type_subst);
  }

  if (const auto* perm_type = std::get_if<TypePerm>(&base_result.type->node)) {
    field_type = MakeTypePerm(perm_type->perm, field_type);
    SPEC_RULE("P-Field-Record-Perm");
  } else {
    SPEC_RULE("P-Field-Record");
  }

  result.ok = true;
  result.type = field_type;
  return result;
}

}  // namespace cursive0::analysis::expr
