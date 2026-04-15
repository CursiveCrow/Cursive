// =================================================================
// File: 04_analysis/typing/expr/propagate_expr.cpp
// Construct: Propagate Expression Type Checking (?)
// Spec Section: 5.2.12
// Spec Rules: T-Propagate
// =================================================================
#include "04_analysis/typing/expr/propagate_expr.h"

#include "00_core/assert_spec.h"
#include "04_analysis/typing/subtyping.h"
#include "04_analysis/typing/type_expr.h"
#include "04_analysis/typing/type_stmt.h"

namespace cursive::analysis::expr {

namespace {

static inline void SpecDefsPropagate() {
  SPEC_DEF("T-Propagate", "5.2.12");
  SPEC_DEF("T-Async-Try", "5.2.12");
  SPEC_DEF("Async-Try-Infallible-Err", "5.2.12");
}

}  // namespace

// (T-Propagate)
// The propagate operator (?) extracts the success type from a union,
// propagating failure cases to the caller via early return.
//
// Typing rule:
// Gamma |- expr : Success | Failure
// Return type includes Failure
// --------------------------------------------------
// Gamma |- expr? : Success
// (early returns Failure if not success)
ExprTypeResult TypePropagateExprImpl(const ScopeContext& ctx,
                                     const StmtTypeContext& type_ctx,
                                     const ast::PropagateExpr& expr,
                                     const TypeEnv& env) {
  ExprTypeResult result;

  // Type the inner expression
  const auto inner = TypeExpr(ctx, type_ctx, expr.value, env);
  if (!inner.ok) {
    result.diag_id = inner.diag_id;
    return result;
  }

  // Strip permission to get the underlying type
  const auto stripped = StripPerm(inner.type);
  if (!stripped) {
    return result;
  }

  // The inner type must be a union type
  const auto* union_type = std::get_if<TypeUnion>(&stripped->node);
  if (!union_type) {
    return result;
  }

  if (union_type->members.empty()) {
    return result;
  }

  // Need a return type context to check what can be propagated.
  // For async procedures, propagate (?) routes through the async error channel.
  if (!type_ctx.return_type) {
    return result;
  }

  TypeRef propagate_target = type_ctx.return_type;
  bool async_try = false;
  if (const auto async_sig = AsyncSigOf(ctx, type_ctx.return_type);
      async_sig.has_value() && IsPrimType(async_sig->err, "!")) {
    SPEC_RULE("Async-Try-Infallible-Err");
    result.diag_id = "E-CON-0230";
    return result;
  } else if (async_sig.has_value()) {
    async_try = true;
    propagate_target = async_sig->err;
  }

  if (!propagate_target) {
    return result;
  }

  // Find the success type: the member that is NOT a subtype of the propagate
  // target. All other members (failures) must be subtypes of that target.
  std::optional<TypeRef> success;
  for (const auto& member : union_type->members) {
    const auto sub = Subtyping(ctx, member, propagate_target);
    if (!sub.ok) {
      result.diag_id = sub.diag_id;
      return result;
    }
    if (sub.subtype) {
      // This member can be propagated to the return type (failure case)
      continue;
    }
    // This member cannot be propagated - it's the success type
    if (success.has_value()) {
      // Multiple success types - cannot determine which to propagate
      return result;
    }
    success = member;
  }

  if (!success.has_value()) {
    // All members are subtypes of return - no success type to extract
    return result;
  }

  if (async_try) {
    SPEC_RULE("T-Async-Try");
  } else {
    SPEC_RULE("T-Propagate");
  }
  result.ok = true;
  result.type = *success;
  return result;
}

}  // namespace cursive::analysis::expr
