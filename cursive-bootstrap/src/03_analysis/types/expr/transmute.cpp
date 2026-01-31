// =================================================================
// File: 03_analysis/types/expr/transmute.cpp
// Construct: Transmute Expression Type Checking
// Spec Section: 5.2.12
// Spec Rules: T-Transmute, T-Transmute-SizeEq, T-Transmute-AlignEq,
//             Transmute-Unsafe-Err
// =================================================================
#include "cursive0/03_analysis/types/expr/transmute.h"

#include "cursive0/00_core/assert_spec.h"
#include "cursive0/03_analysis/types/type_equiv.h"
#include "cursive0/03_analysis/types/type_expr.h"
#include "cursive0/03_analysis/types/type_lower.h"
#include "cursive0/04_codegen/layout/layout.h"

namespace cursive0::analysis::expr {

namespace {

std::optional<std::uint64_t> SizeOfInternal(const ScopeContext& ctx,
                                            const TypeRef& type) {
  const auto layout = codegen::LayoutOf(ctx, type);
  if (!layout.has_value()) {
    return std::nullopt;
  }
  return layout->size;
}

std::optional<std::uint64_t> AlignOfInternal(const ScopeContext& ctx,
                                             const TypeRef& type) {
  const auto layout = codegen::LayoutOf(ctx, type);
  if (!layout.has_value()) {
    return std::nullopt;
  }
  return layout->align;
}

}  // namespace

ExprTypeResult TypeTransmuteExprImpl(const ScopeContext& ctx,
                                     const StmtTypeContext& type_ctx,
                                     const syntax::TransmuteExpr& expr,
                                     const TypeEnv& env,
                                     const core::Span& span) {
  ExprTypeResult result;
  if (type_ctx.require_pure) {
    result.diag_id = "E-SEM-2802";
    return result;
  }
  if (!IsInUnsafeSpan(ctx, span)) {
    SPEC_RULE("Transmute-Unsafe-Err");
    result.diag_id = "Transmute-Unsafe-Err";
    return result;
  }
  if (!expr.from || !expr.to || !expr.value) {
    return result;
  }

  const auto from = LowerType(ctx, expr.from);
  if (!from.ok) {
    result.diag_id = from.diag_id;
    return result;
  }
  const auto from_wf = TypeWF(ctx, from.type);
  if (!from_wf.ok) {
    result.diag_id = from_wf.diag_id;
    return result;
  }
  const auto to = LowerType(ctx, expr.to);
  if (!to.ok) {
    result.diag_id = to.diag_id;
    return result;
  }
  const auto to_wf = TypeWF(ctx, to.type);
  if (!to_wf.ok) {
    result.diag_id = to_wf.diag_id;
    return result;
  }

  const auto from_size = SizeOfInternal(ctx, from.type);
  const auto to_size = SizeOfInternal(ctx, to.type);
  if (!from_size.has_value() || !to_size.has_value()) {
    return result;
  }
  if (*from_size != *to_size) {
    SPEC_RULE("T-Transmute-SizeEq");
    result.diag_id = "T-Transmute-SizeEq";
    return result;
  }
  SPEC_RULE("T-Transmute-SizeEq");

  const auto from_align = AlignOfInternal(ctx, from.type);
  const auto to_align = AlignOfInternal(ctx, to.type);
  if (!from_align.has_value() || !to_align.has_value()) {
    return result;
  }
  if (*from_align != *to_align) {
    SPEC_RULE("T-Transmute-AlignEq");
    result.diag_id = "T-Transmute-AlignEq";
    return result;
  }
  SPEC_RULE("T-Transmute-AlignEq");

  const auto value = TypeExpr(ctx, type_ctx, expr.value, env);
  if (!value.ok) {
    result.diag_id = value.diag_id;
    return result;
  }
  const auto equiv = TypeEquiv(value.type, from.type);
  if (!equiv.ok) {
    result.diag_id = equiv.diag_id;
    return result;
  }
  if (!equiv.equiv) {
    return result;
  }

  SPEC_RULE("T-Transmute");
  result.ok = true;
  result.type = to.type;
  return result;
}

}  // namespace cursive0::analysis::expr
