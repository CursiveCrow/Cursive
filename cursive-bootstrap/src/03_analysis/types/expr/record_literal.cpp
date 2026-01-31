// =================================================================
// File: 03_analysis/types/expr/record_literal.cpp
// Construct: Record Literal Expression Type Checking
// Spec Section: 5.2.12
// Spec Rules: T-Record-Literal, T-Modal-State-Intro, Record-FieldInit-Dup,
//             Record-Field-Unknown, Record-Field-NotVisible,
//             Record-FieldInit-Missing, Record-Field-NonBitcopy-Move
// =================================================================
#include "cursive0/03_analysis/types/expr/record_literal.h"

#include <unordered_map>
#include <unordered_set>

#include "cursive0/00_core/assert_spec.h"
#include "cursive0/03_analysis/generics/monomorphize.h"
#include "cursive0/03_analysis/modal/modal.h"
#include "cursive0/03_analysis/resolve/scopes.h"
#include "cursive0/03_analysis/types/type_equiv.h"
#include "cursive0/03_analysis/types/type_expr.h"
#include "cursive0/03_analysis/types/type_lookup.h"
#include "cursive0/03_analysis/types/type_lower.h"
#include "cursive0/03_analysis/types/type_match.h"

namespace cursive0::analysis::expr {

using namespace cursive0::analysis;

ExprTypeResult TypeRecordExprImpl(const ScopeContext& ctx,
                                  const StmtTypeContext& type_ctx,
                                  const syntax::RecordExpr& expr,
                                  const TypeEnv& env) {
  ExprTypeResult result;

  if (const auto* modal = std::get_if<syntax::ModalStateRef>(&expr.target)) {
    if (modal->path.size() == 1 &&
        (modal->path[0] == "File" || modal->path[0] == "DirIter")) {
      SPEC_RULE("Record-FileDir-Err");
      result.diag_id = "Record-FileDir-Err";
      return result;
    }
    const auto* decl = LookupModalDecl(ctx, modal->path);
    if (!decl) {
      return result;
    }
    const auto* state = LookupModalState(*decl, modal->state);
    if (!state) {
      return result;
    }

    SPEC_RULE("WF-ModalState");
    SPEC_RULE("State-Specific-WF");

    std::unordered_set<IdKey> seen;
    for (const auto& field_init : expr.fields) {
      const auto key = IdKeyOf(field_init.name);
      if (!seen.insert(key).second) {
        SPEC_RULE("Record-FieldInit-Dup");
        result.diag_id = "Record-FieldInit-Dup";
        return result;
      }
    }

    std::unordered_map<IdKey, const syntax::StateFieldDecl*> payload_fields;
    for (const auto& member : state->members) {
      if (const auto* field = std::get_if<syntax::StateFieldDecl>(&member)) {
        payload_fields.emplace(IdKeyOf(field->name), field);
      }
    }

    for (const auto& field_init : expr.fields) {
      if (payload_fields.find(IdKeyOf(field_init.name)) == payload_fields.end()) {
        SPEC_RULE("Record-Field-Unknown");
        result.diag_id = "Record-Field-Unknown";
        return result;
      }
    }

    for (const auto& member : state->members) {
      if (const auto* field = std::get_if<syntax::StateFieldDecl>(&member)) {
        const auto key = IdKeyOf(field->name);
        if (seen.find(key) == seen.end()) {
          SPEC_RULE("Record-FieldInit-Missing");
          result.diag_id = "Record-FieldInit-Missing";
          return result;
        }
      }
    }

    auto type_expr_fn = [&](const syntax::ExprPtr& inner) {
      return TypeExpr(ctx, type_ctx, inner, env);
    };
    auto type_ident = [&](std::string_view name) -> ExprTypeResult {
      return TypeIdentifierExpr(ctx, syntax::IdentifierExpr{std::string(name)},
                                env);
    };
    PlaceTypeFn type_place = [&](const syntax::ExprPtr& inner) {
      return TypePlace(ctx, type_ctx, inner, env);
    };
    auto match_check = [&](const syntax::MatchExpr& match,
                           const TypeRef& expected) -> CheckResult {
      return CheckMatchExpr(ctx, type_ctx, match, env, expected);
    };

    for (const auto& field_init : expr.fields) {
      const auto it = payload_fields.find(IdKeyOf(field_init.name));
      if (it == payload_fields.end() || !it->second) {
        return result;
      }
      const auto lowered = LowerType(ctx, it->second->type);
      if (!lowered.ok) {
        result.diag_id = lowered.diag_id;
        return result;
      }
      const auto check = CheckExpr(
          ctx, field_init.value, lowered.type, type_expr_fn, type_place, type_ident,
          match_check);
      if (!check.ok) {
        result.diag_id = check.diag_id;
        return result;
      }
    }

    SPEC_RULE("T-Modal-State-Intro");

    // Lower generic args from syntax types to TypeRefs
    std::vector<TypeRef> lowered_args;
    for (const auto& arg : modal->generic_args) {
      const auto lowered = LowerType(ctx, arg);
      if (!lowered.ok) {
        result.diag_id = lowered.diag_id;
        return result;
      }
      lowered_args.push_back(lowered.type);
    }

    result.ok = true;
    result.type = MakeTypeModalState(modal->path, modal->state, std::move(lowered_args));
    return result;
  }

  // Handle TypePath and GenericTypeRef cases
  TypePath type_path;
  std::vector<std::shared_ptr<syntax::Type>> syntax_generic_args;

  if (const auto* path = std::get_if<syntax::TypePath>(&expr.target)) {
    type_path = *path;
  } else if (const auto* gen_ref = std::get_if<syntax::GenericTypeRef>(&expr.target)) {
    type_path = gen_ref->path;
    syntax_generic_args = gen_ref->generic_args;
  } else {
    return result;
  }

  const auto* record = LookupRecordDecl(ctx, type_path);
  if (!record) {
    return result;
  }

  std::unordered_set<IdKey> seen;
  for (const auto& field_init : expr.fields) {
    const auto key = IdKeyOf(field_init.name);
    if (!seen.insert(key).second) {
      SPEC_RULE("Record-FieldInit-Dup");
      result.diag_id = "Record-FieldInit-Dup";
      return result;
    }
  }

  for (const auto& field_init : expr.fields) {
    if (!FieldExists(*record, field_init.name)) {
      SPEC_RULE("Record-Field-Unknown");
      result.diag_id = "Record-Field-Unknown";
      return result;
    }
    if (!FieldVisible(ctx, *record, field_init.name, type_path)) {
      SPEC_RULE("Record-Field-NotVisible");
      result.diag_id = "Record-Field-NotVisible";
      return result;
    }
  }

  std::unordered_set<IdKey> provided;
  for (const auto& field_init : expr.fields) {
    provided.insert(IdKeyOf(field_init.name));
  }
  for (const auto& member : record->members) {
    if (const auto* field = std::get_if<syntax::FieldDecl>(&member)) {
      if (provided.find(IdKeyOf(field->name)) == provided.end()) {
        SPEC_RULE("Record-FieldInit-Missing");
        result.diag_id = "Record-FieldInit-Missing";
        return result;
      }
    }
  }

  // §13.1 WF-Apply: Lower generic args BEFORE field type checking so we can
  // build a substitution map for type parameter instantiation
  std::vector<TypeRef> lowered_record_args;
  for (const auto& arg : syntax_generic_args) {
    const auto lowered = LowerType(ctx, arg);
    if (!lowered.ok) {
      result.diag_id = lowered.diag_id;
      return result;
    }
    lowered_record_args.push_back(lowered.type);
  }

  // §13.1: Build type substitution map for generic records
  // Maps type parameter names (e.g., "T") to concrete types (e.g., i32)
  TypeSubst type_subst;
  if (record->generic_params.has_value() && !lowered_record_args.empty()) {
    type_subst = BuildSubstitution(record->generic_params->params, lowered_record_args);
  }

  auto type_expr_fn = [&](const syntax::ExprPtr& inner) {
    return TypeExpr(ctx, type_ctx, inner, env);
  };
  auto type_ident = [&](std::string_view name) -> ExprTypeResult {
    return TypeIdentifierExpr(ctx, syntax::IdentifierExpr{std::string(name)},
                              env);
  };
  PlaceTypeFn type_place = [&](const syntax::ExprPtr& inner) {
    return TypePlace(ctx, type_ctx, inner, env);
  };
  auto match_check = [&](const syntax::MatchExpr& match,
                         const TypeRef& expected) -> CheckResult {
    return CheckMatchExpr(ctx, type_ctx, match, env, expected);
  };

  for (const auto& field_init : expr.fields) {
    auto field_type_opt = FieldType(*record, field_init.name, ctx);
    if (!field_type_opt.has_value()) {
      return result;
    }

    // §13.1 Instantiate: Substitute type parameters in field type
    // e.g., for Box<i32>, substitute T -> i32 in field type "T"
    if (!type_subst.empty()) {
      field_type_opt = InstantiateType(*field_type_opt, type_subst);
    }

    if (!BitcopyType(ctx, *field_type_opt) && IsPlaceExpr(field_init.value) &&
        (!field_init.value ||
         !std::holds_alternative<syntax::MoveExpr>(field_init.value->node))) {
      SPEC_RULE("Record-Field-NonBitcopy-Move");
      result.diag_id = "Record-Field-NonBitcopy-Move";
      return result;
    }

    const auto check = CheckExpr(
        ctx, field_init.value, *field_type_opt, type_expr_fn, type_place, type_ident,
        match_check);
    if (!check.ok) {
      result.diag_id = check.diag_id;
      return result;
    }
  }

  SPEC_RULE("T-Record-Literal");

  result.ok = true;
  result.type = MakeTypePath(type_path, std::move(lowered_record_args));
  return result;
}

}  // namespace cursive0::analysis::expr
