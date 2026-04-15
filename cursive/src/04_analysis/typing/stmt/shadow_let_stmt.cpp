// =============================================================================
// shadow_let_stmt.cpp - Shadow let statement typing
// =============================================================================
//
// SPEC REFERENCE: CursiveSpecification.md
//   Section 5.2.11: Statement Typing (lines 9409-9427)
//   - T-ShadowLetStmt-Ann (line 9409): With type annotation
//   - T-ShadowLetStmt-Ann-Mismatch (line 9414): Annotation mismatch
//   - T-ShadowLetStmt-Infer (line 9419): Type inference
//   - T-ShadowLetStmt-Infer-Err (line 9424): Inference failure
//   - shadow_binding grammar (line 3153)
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_stmt.cpp
//
// =============================================================================

#include "04_analysis/typing/type_stmt.h"

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "00_core/assert_spec.h"
#include "02_source/ast/ast.h"
#include "04_analysis/attributes/attribute_registry.h"
#include "04_analysis/memory/regions.h"
#include "04_analysis/resolve/scopes.h"
#include "04_analysis/typing/type_expr.h"
#include "04_analysis/typing/type_equiv.h"
#include "04_analysis/typing/type_infer.h"
#include "04_analysis/typing/type_lower.h"

namespace cursive::analysis {

// IntroResult and IntroAll are declared in type_stmt.h

namespace {

static inline void SpecDefsShadowLetStmt() {
  SPEC_DEF("T-ShadowLetStmt-Ann", "5.2.11");
  SPEC_DEF("T-ShadowLetStmt-Ann-Mismatch", "5.2.11");
  SPEC_DEF("T-ShadowLetStmt-Infer", "5.2.11");
  SPEC_DEF("T-ShadowLetStmt-Infer-Err", "5.2.11");
}

bool IsUniqueMoveInitCompatible(const TypeRef& annotated,
                                const ast::ExprPtr& init,
                                const PlaceTypeFn& type_place) {
  if (!init || PermOfType(annotated) != Permission::Unique) {
    return false;
  }
  const auto* move_expr = std::get_if<ast::MoveExpr>(&init->node);
  if (!move_expr || !move_expr->place) {
    return false;
  }
  const auto place = type_place(move_expr->place);
  if (!place.ok) {
    return false;
  }
  const auto eq = TypeEquiv(StripPerm(place.type), StripPerm(annotated));
  return eq.ok && eq.equiv;
}

std::optional<std::string> NormalizeDeprecatedMessage(
    const ast::AttributeList& attrs) {
  auto message =
      GetAttributeValue(attrs, ::cursive::analysis::attrs::kDeprecated);
  if (!message.has_value()) {
    return std::nullopt;
  }
  if (message->size() >= 2 &&
      ((message->front() == '"' && message->back() == '"') ||
       (message->front() == '\'' && message->back() == '\''))) {
    return message->substr(1, message->size() - 2);
  }
  return message;
}

void ApplyShadowBindingMetadata(
    TypeEnv& env,
    std::string_view name,
    const std::optional<TypeBinding::ClosureCaptureInfo>& closure_info,
    const std::optional<ProvExprTrackResult>& provenance,
    bool deprecated,
    const std::optional<std::string>& deprecated_message) {
  if (env.scopes.empty()) {
    return;
  }
  const auto key = IdKeyOf(name);
  const auto it = env.scopes.back().find(key);
  if (it == env.scopes.back().end()) {
    return;
  }
  if (closure_info.has_value()) {
    it->second.closure_capture_info = *closure_info;
  }
  if (provenance.has_value()) {
    ApplyBindingProvenanceSeed(it->second, provenance->kind,
                               provenance->region);
  }
  it->second.deprecated = deprecated;
  it->second.deprecated_message = deprecated_message;
}

}  // namespace

StmtTypeResult TypeShadowLetStmt(const ScopeContext& ctx,
                                 const StmtTypeContext& type_ctx,
                                 const ast::ShadowLetStmt& node,
                                 const TypeEnv& env,
                                 const ExprTypeFn& type_expr,
                                 const IdentTypeFn& type_ident,
                                 const PlaceTypeFn& type_place) {
  SpecDefsShadowLetStmt();
  const auto attr_validation =
      ValidateAttributes(node.attrs, AttributeTarget::Binding);
  if (!attr_validation.ok) {
    return {false, attr_validation.diag_id, {}, {}, attr_validation.message};
  }
  const bool binding_deprecated =
      HasAttribute(node.attrs, ::cursive::analysis::attrs::kDeprecated);
  const auto deprecated_message = NormalizeDeprecatedMessage(node.attrs);

  // Case 1: Type annotation provided
  if (node.type_opt) {
    // Lower the type annotation
    const auto ann = LowerType(ctx, node.type_opt);
    if (!ann.ok) {
      return {false, ann.diag_id, {}, {}};
    }

    // Check init expression against annotated type
    const auto check =
        CheckExprAgainst(ctx, type_ctx, node.init, ann.type, env);
    const bool unique_move_ok =
        IsUniqueMoveInitCompatible(ann.type, node.init, type_place);
    if (!check.ok && !unique_move_ok) {
      if (!check.diag_id.has_value()) {
        SPEC_RULE("T-ShadowLetStmt-Ann-Mismatch");
        return {false, "T-ShadowLetStmt-Ann-Mismatch", {}, {}};
      }
      return {false, check.diag_id, {}, {}};
    }

    if (const auto log_diag =
            ValidateLogAttributesForObservedType(ctx, node.attrs, ann.type, env)) {
      return {false, log_diag, {}, {}};
    }

    // Introduce shadowing binding with 'let' mutability
    std::vector<std::pair<std::string, TypeRef>> binds;
    binds.emplace_back(node.name, ann.type);
    const auto intro = IntroAll(env, binds, ast::Mutability::Let, true);
    if (!intro.ok) {
      return {false, intro.diag_id, {}, {}};
    }

    TypeEnv out_env = std::move(intro.env);
    std::optional<ProvExprTrackResult> provenance;
    const auto tracked = TrackExprProvenance(ctx, node.init, env);
    if (tracked.ok) {
      provenance = tracked;
    }
    ApplyShadowBindingMetadata(
        out_env, node.name, AnalyzeClosureCaptureInfo(node.init, env, ann.type),
        provenance, binding_deprecated, deprecated_message);

    SPEC_RULE("T-ShadowLetStmt-Ann");
    return {true, std::nullopt, std::move(out_env), {}};
  }

  // Case 2: Type inference
  ConstraintSet constraints;
  const auto inferred = InferExpr(ctx, node.init, type_expr, type_place,
                                  type_ident, &constraints);
  if (!inferred.ok) {
    if (inferred.diag_id.has_value()) {
      return {false, inferred.diag_id, {}, {}};
    }
    SPEC_RULE("T-ShadowLetStmt-Infer-Err");
    return {false, "T-ShadowLetStmt-Infer-Err", {}, {}};
  }
  const auto solved = Solve(ctx, constraints);
  if (!solved.ok) {
    if (solved.diag_id.has_value()) {
      return {false, solved.diag_id, {}, {}};
    }
    SPEC_RULE("T-ShadowLetStmt-Infer-Err");
    return {false, "T-ShadowLetStmt-Infer-Err", {}, {}};
  }
  const auto inferred_type = ApplySubstitution(inferred.type, solved.subst);
  if (!inferred_type) {
    SPEC_RULE("T-ShadowLetStmt-Infer-Err");
    return {false, "T-ShadowLetStmt-Infer-Err", {}, {}};
  }

  if (const auto log_diag =
          ValidateLogAttributesForObservedType(ctx, node.attrs, inferred_type,
                                               env)) {
    return {false, log_diag, {}, {}};
  }

  // Introduce shadowing binding with 'let' mutability
  std::vector<std::pair<std::string, TypeRef>> binds;
  binds.emplace_back(node.name, inferred_type);
  const auto intro = IntroAll(env, binds, ast::Mutability::Let, true);
  if (!intro.ok) {
    return {false, intro.diag_id, {}, {}};
  }

  TypeEnv out_env = std::move(intro.env);
  std::optional<ProvExprTrackResult> provenance;
  const auto tracked = TrackExprProvenance(ctx, node.init, env);
  if (tracked.ok) {
    provenance = tracked;
  }
  ApplyShadowBindingMetadata(out_env,
                             node.name,
                             AnalyzeClosureCaptureInfo(node.init, env,
                                                       inferred_type),
                             provenance,
                             binding_deprecated,
                             deprecated_message);

  SPEC_RULE("T-ShadowLetStmt-Infer");
  return {true, std::nullopt, std::move(out_env), {}};
}

}  // namespace cursive::analysis
