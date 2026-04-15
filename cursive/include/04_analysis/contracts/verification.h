#pragma once

#include <optional>
#include <string>
#include <vector>

#include "04_analysis/typing/types.h"
#include "00_core/span.h"
#include "02_source/ast/ast.h"

namespace cursive::analysis {

// C0X Extension: Contract System - Static Verification Logic

// Verification fact: F(P, L, S)
// P = predicate, L = location, S = scope
struct VerificationFact {
  ast::ExprPtr predicate;
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
bool ExprStructEqual(const ast::ExprPtr& a, const ast::ExprPtr& b);

// StaticProof(Γ_S, P): decidable predicate provability
StaticProofResult StaticProof(
    const StaticProofContext& ctx,
    const ast::ExprPtr& predicate);

// Entailment rules

// Ent-True: true is always entailed
bool EntTrue(const ast::ExprPtr& expr);

// Ent-Fact: predicate in facts
bool EntFact(const StaticProofContext& ctx, const ast::ExprPtr& expr);

// Ent-And: P && Q entailed if P and Q entailed
bool EntAnd(const StaticProofContext& ctx,
            const ast::ExprPtr& left,
            const ast::ExprPtr& right);

// Ent-Or-L/R: P || Q entailed if P or Q entailed
bool EntOr(const StaticProofContext& ctx,
           const ast::ExprPtr& left,
           const ast::ExprPtr& right);

// Ent-Linear: Linear integer reasoning
// Proves relations like x + 1 > x, a < b && b < c => a < c
bool EntLinear(const StaticProofContext& ctx, const ast::ExprPtr& expr);

// Mandatory proof techniques

// Constant propagation
struct ConstValue {
  bool known = false;
  std::int64_t value = 0;
  bool bool_value = false;
  bool is_bool = false;
};

ConstValue EvaluateConstant(const ast::ExprPtr& expr);

// Boolean algebra simplification
ast::ExprPtr SimplifyBoolean(const ast::ExprPtr& expr);

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
             const ast::ExprPtr& predicate,
             const core::Span& location);

// Check dominance (fact valid at location)
bool FactDominates(const VerificationFact& fact, const core::Span& location);

}  // namespace cursive::analysis
