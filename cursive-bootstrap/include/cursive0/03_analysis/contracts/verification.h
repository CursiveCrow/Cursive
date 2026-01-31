#pragma once

#include <optional>
#include <string>
#include <vector>

#include "cursive0/03_analysis/types/types.h"
#include "cursive0/00_core/span.h"
#include "cursive0/02_syntax/ast.h"

namespace cursive0::analysis {

// C0X Extension: Contract System - Static Verification Logic

// Verification fact: F(P, L, S)
// P = predicate, L = location, S = scope
struct VerificationFact {
  syntax::ExprPtr predicate;
  core::Span location;
  std::size_t scope_id;
};

// Static proof context
struct StaticProofContext {
  std::vector<VerificationFact> facts;
  std::size_t current_scope = 0;
};

// Static proof result
struct StaticProofResult {
  bool provable = false;
  std::optional<std::string_view> diag_id;
  std::string explanation;  // For diagnostic
};

// Structural equality of expressions (syntax-only)
bool ExprStructEqual(const syntax::ExprPtr& a, const syntax::ExprPtr& b);

// StaticProof(Γ_S, P): decidable predicate provability
StaticProofResult StaticProof(
    const StaticProofContext& ctx,
    const syntax::ExprPtr& predicate);

// Entailment rules

// Ent-True: true is always entailed
bool EntTrue(const syntax::ExprPtr& expr);

// Ent-Fact: predicate in facts
bool EntFact(const StaticProofContext& ctx, const syntax::ExprPtr& expr);

// Ent-And: P && Q entailed if P and Q entailed
bool EntAnd(const StaticProofContext& ctx,
            const syntax::ExprPtr& left,
            const syntax::ExprPtr& right);

// Ent-Or-L/R: P || Q entailed if P or Q entailed
bool EntOr(const StaticProofContext& ctx,
           const syntax::ExprPtr& left,
           const syntax::ExprPtr& right);

// Ent-Linear: Linear integer reasoning
// Proves relations like x + 1 > x, a < b && b < c => a < c
bool EntLinear(const StaticProofContext& ctx, const syntax::ExprPtr& expr);

// Mandatory proof techniques

// Constant propagation
struct ConstValue {
  bool known = false;
  std::int64_t value = 0;
  bool bool_value = false;
  bool is_bool = false;
};

ConstValue EvaluateConstant(const syntax::ExprPtr& expr);

// Boolean algebra simplification
syntax::ExprPtr SimplifyBoolean(const syntax::ExprPtr& expr);

// Control flow analysis for reachability
bool IsReachable(const StaticProofContext& ctx, const core::Span& location);

// Type-derived bounds
struct TypeBounds {
  bool has_min = false;
  bool has_max = false;
  std::int64_t min = 0;
  std::int64_t max = 0;
};

TypeBounds GetTypeBounds(const TypeRef& type);

// Generate verification fact
void AddFact(StaticProofContext& ctx,
             const syntax::ExprPtr& predicate,
             const core::Span& location);

// Check dominance (fact valid at location)
bool FactDominates(const VerificationFact& fact, const core::Span& location);

}  // namespace cursive0::analysis
