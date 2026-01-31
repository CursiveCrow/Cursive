// =================================================================
// File: 03_analysis/types/expr/range.cpp
// Construct: Range Expression Type Checking
// Spec Section: 5.2.12
// Spec Rules: Range-Full, Range-To, Range-ToInclusive, Range-From,
//             Range-Exclusive, Range-Inclusive
// =================================================================
#include "cursive0/03_analysis/types/expr/range.h"

#include "cursive0/00_core/assert_spec.h"
#include "cursive0/03_analysis/types/expr/common.h"
#include "cursive0/03_analysis/types/type_expr.h"

namespace cursive0::analysis::expr {

namespace {

bool IsIntegerType(const TypeRef& type) {
  if (!type) return false;
  const auto* prim = std::get_if<TypePrim>(&type->node);
  if (!prim) return false;
  return prim->name == "usize" || prim->name == "isize" ||
         prim->name == "i8" || prim->name == "i16" ||
         prim->name == "i32" || prim->name == "i64" ||
         prim->name == "u8" || prim->name == "u16" ||
         prim->name == "u32" || prim->name == "u64";
}

}  // namespace

// Range expressions (Range-Full, Range-To, etc.)
ExprTypeResult TypeRangeExprImpl(const ScopeContext& ctx,
                                 const StmtTypeContext& type_ctx,
                                 const syntax::RangeExpr& expr,
                                 const TypeEnv& env) {
  ExprTypeResult result;

  // Check bounds are usize (or integer types in parallel context for C0X)
  if (expr.lhs) {
    const auto start = TypeExpr(ctx, type_ctx, expr.lhs, env);
    if (!start.ok) {
      result.diag_id = start.diag_id;
      return result;
    }
    // C0X Extension: Allow integer types in parallel context for dispatch
    if (!IsPrimType(start.type, "usize") &&
        !(type_ctx.in_parallel && IsIntegerType(start.type))) {
      SPEC_RULE("Range-NonIndex-Err");
      result.diag_id = "Range-NonIndex-Err";
      return result;
    }
  }

  if (expr.rhs) {
    const auto end = TypeExpr(ctx, type_ctx, expr.rhs, env);
    if (!end.ok) {
      result.diag_id = end.diag_id;
      return result;
    }
    // C0X Extension: Allow integer types in parallel context for dispatch
    if (!IsPrimType(end.type, "usize") &&
        !(type_ctx.in_parallel && IsIntegerType(end.type))) {
      SPEC_RULE("Range-NonIndex-Err");
      result.diag_id = "Range-NonIndex-Err";
      return result;
    }
  }

  // Determine range kind and apply appropriate rule
  switch (expr.kind) {
    case syntax::RangeKind::Full:
      SPEC_RULE("Range-Full");
      break;
    case syntax::RangeKind::To:
      SPEC_RULE("Range-To");
      break;
    case syntax::RangeKind::ToInclusive:
      SPEC_RULE("Range-ToInclusive");
      break;
    case syntax::RangeKind::From:
      SPEC_RULE("Range-From");
      break;
    case syntax::RangeKind::Exclusive:
      SPEC_RULE("Range-Exclusive");
      break;
    case syntax::RangeKind::Inclusive:
      SPEC_RULE("Range-Inclusive");
      break;
  }

  result.ok = true;
  result.type = MakeTypeRange();
  return result;
}

}  // namespace cursive0::analysis::expr
