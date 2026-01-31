#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "cursive0/03_analysis/types/types.h"
#include "cursive0/00_core/span.h"
#include "cursive0/02_syntax/ast.h"

namespace cursive0::analysis {

// C0X Extension: Contract System - Contract Type Checking

// Contract checking result
struct ContractCheckResult {
  bool ok = true;
  std::optional<std::string_view> diag_id;
  std::optional<core::Span> span;
};

// Contract context
struct ContractContext {
  // Available bindings
  std::map<std::string, TypeRef> params;
  TypeRef receiver_type;
  TypeRef return_type;
  
  // Context state
  bool is_postcondition = false;
  bool in_type_invariant = false;
  bool in_loop_invariant = false;
};

// Check well-formedness of contract clause
// WF-Contract: pure predicates, correct typing
ContractCheckResult CheckContractWellFormed(
    const ContractContext& ctx,
    const syntax::ContractClause& contract);

// Check precondition expression
// Γ_pre: parameters, receiver, enclosing scope (no @result, @entry)
ContractCheckResult CheckPrecondition(
    const ContractContext& ctx,
    const syntax::ExprPtr& expr);

// Check postcondition expression
// Γ_post: add @result, @entry(), mutable params at post-state
ContractCheckResult CheckPostcondition(
    const ContractContext& ctx,
    const syntax::ExprPtr& expr);

// Check purity of expression
// Pure: no capability params, no observable mutation
ContractCheckResult CheckPurity(const syntax::ExprPtr& expr);

// Check type invariant
struct TypeInvariantResult {
  bool ok = true;
  std::optional<std::string_view> diag_id;
  bool has_public_mutable_fields = false;
};

TypeInvariantResult CheckTypeInvariant(
    const ContractContext& ctx,
    const syntax::TypeInvariant& invariant);

// Check loop invariant
ContractCheckResult CheckLoopInvariant(
    const ContractContext& ctx,
    const syntax::LoopInvariant& invariant);

// Behavioral subtyping check (LSP)
// impl pre >= class pre, impl post <= class post
struct BehavioralSubtypingResult {
  bool ok = true;
  std::optional<std::string_view> diag_id;
  bool precondition_weaker = true;   // impl pre is weaker/equal
  bool postcondition_stronger = true; // impl post is stronger/equal
};

BehavioralSubtypingResult CheckBehavioralSubtyping(
    const syntax::ContractClause& class_contract,
    const syntax::ContractClause& impl_contract);

}  // namespace cursive0::analysis
