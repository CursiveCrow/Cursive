#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "cursive0/04_codegen/ir_model.h"
#include "cursive0/04_codegen/lower/lower_expr.h"
#include "cursive0/02_syntax/ast.h"

namespace cursive0::codegen {

// ============================================================================
// §6.6 Pattern Lowering Judgments
// ============================================================================

// §6.6 LowerBindPattern - bind a value to a pattern
IRPtr LowerBindPattern(const syntax::Pattern& pattern,
                       const IRValue& value,
                       LowerCtx& ctx);

// §6.6 LowerBindList - bind values to a list of patterns
IRPtr LowerBindList(const std::vector<std::shared_ptr<syntax::Pattern>>& patterns,
                    const std::vector<IRValue>& values,
                    LowerCtx& ctx);

// Register bindings introduced by a pattern with optional type hints
void RegisterPatternBindings(const syntax::Pattern& pattern,
                             const analysis::TypeRef& type_hint,
                             LowerCtx& ctx,
                             bool is_immovable = false,
                             analysis::ProvenanceKind prov = analysis::ProvenanceKind::Bottom,
                             std::optional<std::string> prov_region = std::nullopt);

// §6.6 TagOf - get the discriminant tag
enum class TagOfKind { Enum, Modal };
IRValue TagOf(const IRValue& value, TagOfKind kind, LowerCtx& ctx);

// ============================================================================
// §6.6 Match Expression Lowering
// ============================================================================

// §6.6 LowerMatch - lower match expression with arms
LowerResult LowerMatch(const syntax::Expr& scrutinee,
                       const std::vector<syntax::MatchArm>& arms,
                       LowerCtx& ctx);

// §6.6 LowerMatchArm - lower a single match arm
LowerResult LowerMatchArm(const syntax::MatchArm& arm,
                          const IRValue& scrutinee,
                          const analysis::TypeRef& scrutinee_type,
                          analysis::ProvenanceKind scrutinee_prov,
                          std::optional<std::string> scrutinee_region,
                          LowerCtx& ctx);

// §6.6 PatternCheck - check if a value matches a pattern
IRValue PatternCheck(const syntax::Pattern& pattern,
                     const IRValue& value,
                     LowerCtx& ctx);

// Emit SPEC_RULE anchors for all §6.6 rules
void AnchorPatternRules();

}  // namespace cursive0::codegen
