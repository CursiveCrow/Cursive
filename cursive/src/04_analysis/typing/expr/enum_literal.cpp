// =================================================================
// File: 04_analysis/typing/expr/enum_literal.cpp
// Construct: Enum Literal Expression Type Checking
// Spec Section: 5.2.12
// Spec Rules: T-Enum-Lit-Unit, T-Enum-Lit-Tuple, T-Enum-Lit-Record
// =================================================================

#include "04_analysis/typing/expr/enum_literal.h"

#include <unordered_map>
#include <unordered_set>

#include "00_core/assert_spec.h"
#include "04_analysis/composite/enums.h"
#include "04_analysis/resolve/scopes.h"
#include "04_analysis/typing/expr/path.h"
#include "04_analysis/typing/if_case_check.h"
#include "04_analysis/typing/type_expr.h"
#include "04_analysis/typing/type_infer.h"
#include "04_analysis/typing/deprecation_warnings.h"
#include "04_analysis/typing/type_lookup.h"
#include "04_analysis/typing/type_lower.h"
#include "04_analysis/typing/typecheck.h"

namespace cursive::analysis::expr {

namespace {

static inline void SpecDefsEnumLiteral() {
  SPEC_DEF("T-Enum-Lit-Unit", "5.2.12");
  SPEC_DEF("T-Enum-Lit-Tuple", "5.2.12");
  SPEC_DEF("T-Enum-Lit-Record", "5.2.12");
}

}  // namespace

ExprTypeResult TypeEnumLiteralExprImpl(const ScopeContext& ctx,
                                       const StmtTypeContext& type_ctx,
                                       const ast::EnumLiteralExpr& expr,
                                       const TypeEnv& env) {
  SpecDefsEnumLiteral();
  ExprTypeResult result;

  if (expr.path.empty()) {
    return result;
  }

  // Parse the path to get enum type and variant name
  TypePath enum_path;
  std::string variant_name;
  if (expr.path.size() == 1) {
    // Single name - needs type context to resolve
    return result;
  }
  enum_path.assign(expr.path.begin(), expr.path.end() - 1);
  variant_name = expr.path.back();

  // Lookup the enum declaration
  const auto* enum_decl = LookupEnumDecl(ctx, enum_path);
  if (!enum_decl) {
    return result;
  }
  std::optional<core::Span> ref_span = std::nullopt;
  if (expr.payload_opt.has_value()) {
    if (std::holds_alternative<ast::EnumPayloadParen>(*expr.payload_opt)) {
      const auto& payload = std::get<ast::EnumPayloadParen>(*expr.payload_opt);
      if (!payload.elements.empty() && payload.elements.front()) {
        ref_span = payload.elements.front()->span;
      }
    } else {
      const auto& payload = std::get<ast::EnumPayloadBrace>(*expr.payload_opt);
      if (!payload.fields.empty() && payload.fields.front().value) {
        ref_span = payload.fields.front().value->span;
      }
    }
  }
  EmitDeprecatedReferenceWarningFromAttrs(enum_decl->attrs, type_ctx, ref_span);

  // Find the variant
  const ast::VariantDecl* variant = nullptr;
  for (const auto& v : enum_decl->variants) {
    if (IdEq(v.name, variant_name)) {
      variant = &v;
      break;
    }
  }
  if (!variant) {
    return result;
  }

  // Unit variant: no payload
  if (!variant->payload_opt.has_value()) {
    if (expr.payload_opt.has_value()) {
      return result;  // Unit variant with payload provided
    }
    SPEC_RULE("T-Enum-Lit-Unit");
  } else {
    if (!expr.payload_opt.has_value()) {
      return result;  // Non-unit variant without payload
    }

    const auto& decl_payload = *variant->payload_opt;
    const auto& expr_payload = *expr.payload_opt;

    // Tuple variant: Direction::North(1, 2)
    if (std::holds_alternative<ast::VariantPayloadTuple>(decl_payload)) {
      const auto& tuple_payload =
          std::get<ast::VariantPayloadTuple>(decl_payload);
      if (!std::holds_alternative<ast::EnumPayloadParen>(expr_payload)) {
        return result;
      }
      const auto& paren = std::get<ast::EnumPayloadParen>(expr_payload);

      // Check element count
      if (paren.elements.size() != tuple_payload.elements.size()) {
        return result;
      }

      // Check each element type
      for (std::size_t i = 0; i < paren.elements.size(); ++i) {
        const auto elem_type_lowered =
            LowerType(ctx, tuple_payload.elements[i]);
        if (!elem_type_lowered.ok) {
          result.diag_id = elem_type_lowered.diag_id;
          return result;
        }
        const auto check = CheckExprAgainst(ctx, type_ctx, paren.elements[i],
                                            elem_type_lowered.type, env);
        if (!check.ok) {
          result.diag_id = check.diag_id;
          return result;
        }
      }
      SPEC_RULE("T-Enum-Lit-Tuple");
    } else {
      // Record variant: Result::Ok{ value: 42 }
      const auto& record_payload =
          std::get<ast::VariantPayloadRecord>(decl_payload);
      if (!std::holds_alternative<ast::EnumPayloadBrace>(expr_payload)) {
        return result;
      }
      const auto& brace = std::get<ast::EnumPayloadBrace>(expr_payload);

      // Check for duplicate field initializers
      std::unordered_set<IdKey> seen;
      for (const auto& field_init : brace.fields) {
        const auto key = IdKeyOf(field_init.name);
        if (!seen.insert(key).second) {
          return result;  // Duplicate field
        }
      }

      // Build field type map
      std::unordered_map<IdKey, TypeRef> field_types;
      for (const auto& field_decl : record_payload.fields) {
        const auto lowered = LowerType(ctx, field_decl.type);
        if (!lowered.ok) {
          result.diag_id = lowered.diag_id;
          return result;
        }
        field_types.emplace(IdKeyOf(field_decl.name), lowered.type);
      }

      // All fields must be initialized
      if (field_types.size() != seen.size()) {
        return result;
      }

      // Type check each field initializer
      for (const auto& field_init : brace.fields) {
        const auto it = field_types.find(IdKeyOf(field_init.name));
        if (it == field_types.end()) {
          return result;  // Unknown field
        }
        const auto check = CheckExprAgainst(ctx, type_ctx, field_init.value,
                                            it->second, env);
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

}  // namespace cursive::analysis::expr
