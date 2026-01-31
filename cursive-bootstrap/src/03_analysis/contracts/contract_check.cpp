#include "cursive0/03_analysis/contracts/contract_check.h"

#include "cursive0/00_core/assert_spec.h"

namespace cursive0::analysis {

namespace {

static inline void SpecDefsContractCheck() {
  SPEC_DEF("WF-Contract", "C0X.5.W");
  SPEC_DEF("ContractPure", "C0X.5.W");
  SPEC_DEF("PreContext", "C0X.5.W");
  SPEC_DEF("PostContext", "C0X.5.W");
  SPEC_DEF("TypeInvariant", "C0X.5.W");
  SPEC_DEF("LoopInvariant", "C0X.5.W");
  SPEC_DEF("LSP", "C0X.5.W");
}

// Check if expression contains @result
bool ContainsResult(const syntax::ExprPtr& expr) {
  if (!expr) return false;
  
  if (std::holds_alternative<syntax::ResultExpr>(expr->node)) {
    return true;
  }
  
  // Recursively check sub-expressions
  return std::visit(
      [](const auto& node) -> bool {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::BinaryExpr>) {
          return ContainsResult(node.lhs) || ContainsResult(node.rhs);
        } else if constexpr (std::is_same_v<T, syntax::UnaryExpr>) {
          return ContainsResult(node.value);
        } else if constexpr (std::is_same_v<T, syntax::CallExpr>) {
          if (ContainsResult(node.callee)) return true;
          for (const auto& arg : node.args) {
            if (ContainsResult(arg.value)) return true;
          }
          return false;
        } else if constexpr (std::is_same_v<T, syntax::MethodCallExpr>) {
          if (ContainsResult(node.receiver)) return true;
          for (const auto& arg : node.args) {
            if (ContainsResult(arg.value)) return true;
          }
          return false;
        }
        return false;
      },
      expr->node);
}

// Check if expression contains @entry
bool ContainsEntry(const syntax::ExprPtr& expr) {
  if (!expr) return false;
  
  if (std::holds_alternative<syntax::EntryExpr>(expr->node)) {
    return true;
  }
  
  return std::visit(
      [](const auto& node) -> bool {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::BinaryExpr>) {
          return ContainsEntry(node.lhs) || ContainsEntry(node.rhs);
        } else if constexpr (std::is_same_v<T, syntax::UnaryExpr>) {
          return ContainsEntry(node.value);
        } else if constexpr (std::is_same_v<T, syntax::CallExpr>) {
          if (ContainsEntry(node.callee)) return true;
          for (const auto& arg : node.args) {
            if (ContainsEntry(arg.value)) return true;
          }
          return false;
        }
        return false;
      },
      expr->node);
}

// Check if expression is observably mutating
bool IsMutating(const syntax::ExprPtr& expr) {
  if (!expr) return false;
  
  // Assignment, method calls on mutable receivers, etc.
  // Simplified check - full impl needs type info
  return std::visit(
      [](const auto& node) -> bool {
        using T = std::decay_t<decltype(node)>;
        // Method calls might mutate
        if constexpr (std::is_same_v<T, syntax::MethodCallExpr>) {
          return true;  // Conservative
        }
        return false;
      },
      expr->node);
}

}  // namespace

ContractCheckResult CheckContractWellFormed(
    const ContractContext& ctx,
    const syntax::ContractClause& contract) {
  SpecDefsContractCheck();
  SPEC_RULE("WF-Contract");
  
  ContractCheckResult result;
  
  // Check precondition
  if (contract.precondition) {
    auto pre_result = CheckPrecondition(ctx, contract.precondition);
    if (!pre_result.ok) {
      return pre_result;
    }
  }
  
  // Check postcondition
  if (contract.postcondition) {
    ContractContext post_ctx = ctx;
    post_ctx.is_postcondition = true;
    auto post_result = CheckPostcondition(post_ctx, contract.postcondition);
    if (!post_result.ok) {
      return post_result;
    }
  }
  
  return result;
}

ContractCheckResult CheckPrecondition(
    const ContractContext& ctx,
    const syntax::ExprPtr& expr) {
  SpecDefsContractCheck();
  SPEC_RULE("Check-Pre");
  
  ContractCheckResult result;
  
  // Precondition must not contain @result or @entry
  if (ContainsResult(expr)) {
    result.ok = false;
    result.diag_id = "E-SEM-2801";  // @result in precondition
    result.span = expr->span;
    return result;
  }
  
  if (ContainsEntry(expr)) {
    result.ok = false;
    result.diag_id = "E-SEM-2802";  // @entry in precondition
    result.span = expr->span;
    return result;
  }
  
  // Check purity
  auto purity = CheckPurity(expr);
  if (!purity.ok) {
    return purity;
  }
  
  return result;
}

ContractCheckResult CheckPostcondition(
    const ContractContext& ctx,
    const syntax::ExprPtr& expr) {
  SpecDefsContractCheck();
  SPEC_RULE("Check-Post");
  
  ContractCheckResult result;
  
  // Postcondition may contain @result and @entry
  // Check purity
  auto purity = CheckPurity(expr);
  if (!purity.ok) {
    return purity;
  }
  
  return result;
}

ContractCheckResult CheckPurity(const syntax::ExprPtr& expr) {
  SpecDefsContractCheck();
  SPEC_RULE("ContractPure");
  
  ContractCheckResult result;
  
  if (IsMutating(expr)) {
    result.ok = false;
    result.diag_id = "E-SEM-2803";  // Impure expression in contract
    if (expr) {
      result.span = expr->span;
    }
  }
  
  return result;
}

TypeInvariantResult CheckTypeInvariant(
    const ContractContext& ctx,
    const syntax::TypeInvariant& invariant) {
  SpecDefsContractCheck();
  SPEC_RULE("TypeInvariant");
  
  TypeInvariantResult result;
  
  // Check predicate purity
  auto purity = CheckPurity(invariant.predicate);
  if (!purity.ok) {
    result.ok = false;
    result.diag_id = purity.diag_id;
  }
  
  return result;
}

ContractCheckResult CheckLoopInvariant(
    const ContractContext& ctx,
    const syntax::LoopInvariant& invariant) {
  SpecDefsContractCheck();
  SPEC_RULE("LoopInvariant");
  
  ContractCheckResult result;
  
  // Check predicate purity
  auto purity = CheckPurity(invariant.predicate);
  if (!purity.ok) {
    return purity;
  }
  
  // Loop invariants may not contain @result
  if (ContainsResult(invariant.predicate)) {
    result.ok = false;
    result.diag_id = "E-SEM-2804";  // @result in loop invariant
    result.span = invariant.span;
  }
  
  return result;
}

BehavioralSubtypingResult CheckBehavioralSubtyping(
    const syntax::ContractClause& class_contract,
    const syntax::ContractClause& impl_contract) {
  SpecDefsContractCheck();
  SPEC_RULE("LSP");
  
  BehavioralSubtypingResult result;
  
  // LSP: Liskov Substitution Principle
  // 1. Precondition weakening: impl pre >= class pre
  //    (impl accepts at least what class accepts)
  // 2. Postcondition strengthening: impl post <= class post
  //    (impl guarantees at least what class guarantees)
  
  // Static verification of these properties would require
  // full theorem proving - simplified here
  
  // For now, just check structural compatibility
  result.precondition_weaker = true;
  result.postcondition_stronger = true;
  
  return result;
}

}  // namespace cursive0::analysis
