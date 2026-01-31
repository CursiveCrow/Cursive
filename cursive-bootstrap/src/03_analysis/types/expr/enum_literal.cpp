// =================================================================
// File: 03_analysis/types/expr/enum_literal.cpp
// Construct: Enum Literal Expression Type Checking
// Spec Section: 5.2.12
// Spec Rules: T-Enum-Lit-Unit, T-Enum-Lit-Tuple, T-Enum-Lit-Record
// =================================================================
#include "cursive0/03_analysis/types/expr/enum_literal.h"

#include <unordered_map>
#include <unordered_set>

#include "cursive0/00_core/assert_spec.h"
#include "cursive0/03_analysis/composite/enums.h"
#include "cursive0/03_analysis/resolve/scopes.h"
#include "cursive0/03_analysis/types/expr/path.h"
#include "cursive0/03_analysis/types/type_expr.h"
#include "cursive0/03_analysis/types/type_lookup.h"
#include "cursive0/03_analysis/types/type_lower.h"
#include "cursive0/03_analysis/types/type_match.h"

namespace cursive0::analysis::expr {

namespace {

static inline void SpecDefsEnumLiteral() {
  SPEC_DEF("T-Enum-Lit-Unit", "5.2.12");
  SPEC_DEF("T-Enum-Lit-Tuple", "5.2.12");
  SPEC_DEF("T-Enum-Lit-Record", "5.2.12");
}

}  // namespace

ExprTypeResult TypeEnumLiteralExprImpl(const ScopeContext& ctx,
                                       const StmtTypeContext& type_ctx,
                                       const syntax::EnumLiteralExpr& expr,
                                       const TypeEnv& env) {
  SpecDefsEnumLiteral();
  ExprTypeResult result;

  if (expr.path.empty()) {
    return result;
  }

  TypePath enum_path;
  std::string variant_name;
  if (expr.path.size() == 1) {
    return result;
  }
  enum_path.assign(expr.path.begin(), expr.path.end() - 1);
  variant_name = expr.path.back();

  const auto* enum_decl = LookupEnumDecl(ctx, enum_path);
  if (!enum_decl) {
    return result;
  }

  const syntax::VariantDecl* variant = nullptr;
  for (const auto& v : enum_decl->variants) {
    if (IdEq(v.name, variant_name)) {
      variant = &v;
      break;
    }
  }
  if (!variant) {
    return result;
  }

  auto type_expr_fn = [&](const syntax::ExprPtr& inner) {
    return TypeExpr(ctx, type_ctx, inner, env);
  };
  auto type_ident = [&](std::string_view name) -> ExprTypeResult {
    return TypeIdentifierExprImpl(ctx, syntax::IdentifierExpr{std::string(name)},
                                  env);
  };
  PlaceTypeFn type_place = [&](const syntax::ExprPtr& inner) {
    return TypePlace(ctx, type_ctx, inner, env);
  };
  auto match_check = [&](const syntax::MatchExpr& match,
                         const TypeRef& expected) -> CheckResult {
    return CheckMatchExpr(ctx, type_ctx, match, env, expected);
  };

  if (!variant->payload_opt.has_value()) {
    if (expr.payload_opt.has_value()) {
      return result;
    }
    SPEC_RULE("T-Enum-Lit-Unit");
  } else {
    if (!expr.payload_opt.has_value()) {
      return result;
    }

    const auto& decl_payload = *variant->payload_opt;
    const auto& expr_payload = *expr.payload_opt;

    if (std::holds_alternative<syntax::VariantPayloadTuple>(decl_payload)) {
      const auto& tuple_payload =
          std::get<syntax::VariantPayloadTuple>(decl_payload);
      if (!std::holds_alternative<syntax::EnumPayloadParen>(expr_payload)) {
        return result;
      }
      const auto& paren = std::get<syntax::EnumPayloadParen>(expr_payload);

      if (paren.elements.size() != tuple_payload.elements.size()) {
        return result;
      }

      for (std::size_t i = 0; i < paren.elements.size(); ++i) {
        const auto elem_type_lowered =
            LowerType(ctx, tuple_payload.elements[i]);
        if (!elem_type_lowered.ok) {
          result.diag_id = elem_type_lowered.diag_id;
          return result;
        }
        const auto check = CheckExpr(
            ctx, paren.elements[i], elem_type_lowered.type, type_expr_fn,
            type_place, type_ident, match_check);
        if (!check.ok) {
          result.diag_id = check.diag_id;
          return result;
        }
      }
      SPEC_RULE("T-Enum-Lit-Tuple");
    } else {
      const auto& record_payload =
          std::get<syntax::VariantPayloadRecord>(decl_payload);
      if (!std::holds_alternative<syntax::EnumPayloadBrace>(expr_payload)) {
        return result;
      }
      const auto& brace = std::get<syntax::EnumPayloadBrace>(expr_payload);

      std::unordered_set<IdKey> seen;
      for (const auto& field_init : brace.fields) {
        const auto key = IdKeyOf(field_init.name);
        if (!seen.insert(key).second) {
          return result;
        }
      }

      std::unordered_map<IdKey, TypeRef> field_types;
      for (const auto& field_decl : record_payload.fields) {
        const auto lowered = LowerType(ctx, field_decl.type);
        if (!lowered.ok) {
          result.diag_id = lowered.diag_id;
          return result;
        }
        field_types.emplace(IdKeyOf(field_decl.name), lowered.type);
      }

      if (field_types.size() != seen.size()) {
        return result;
      }

      for (const auto& field_init : brace.fields) {
        const auto it = field_types.find(IdKeyOf(field_init.name));
        if (it == field_types.end()) {
          return result;
        }
        const auto check = CheckExpr(
            ctx, field_init.value, it->second, type_expr_fn, type_place, type_ident,
            match_check);
        if (!check.ok) {
          result.diag_id = check.diag_id;
          return result;
        }
      }
      SPEC_RULE("T-Enum-Lit-Record");
    }
  }

  result.ok = true;
  result.type = MakeTypePath(enum_path);
  return result;
}

}  // namespace cursive0::analysis::expr
