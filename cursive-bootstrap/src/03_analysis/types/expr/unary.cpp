// =================================================================
// File: 03_analysis/types/expr/unary.cpp
// Construct: Unary Expression Type Checking
// Spec Section: 5.2.12
// Spec Rules: T-Not-Bool, T-Not-Int, T-Neg, T-Modal-Widen
// =================================================================
#include "cursive0/03_analysis/types/expr/unary.h"

#include "cursive0/00_core/assert_spec.h"
#include "cursive0/03_analysis/types/expr/common.h"
#include "cursive0/03_analysis/types/type_expr.h"
#include "cursive0/03_analysis/modal/modal.h"
#include "cursive0/03_analysis/modal/modal_widen.h"

namespace cursive0::analysis::expr {

// (T-Not-Bool), (T-Not-Int), (T-Neg)
ExprTypeResult TypeUnaryExprImpl(const ScopeContext& ctx,
                                 const StmtTypeContext& type_ctx,
                                 const syntax::UnaryExpr& expr,
                                 const TypeEnv& env,
                                 const core::Span& span) {
  ExprTypeResult result;

  const auto operand = TypeExpr(ctx, type_ctx, expr.value, env);
  if (!operand.ok) {
    result.diag_id = operand.diag_id;
    return result;
  }

  const std::string_view op = expr.op;
  if (op == "widen") {
    const auto stripped = StripPerm(operand.type);
    if (!stripped) {
      return result;
    }
    if (const auto* modal = std::get_if<TypeModalState>(&stripped->node)) {
      const auto* decl = LookupModalDecl(ctx, modal->path);
      if (!decl || !HasState(*decl, modal->state)) {
        return result;
      }
      WarnWidenLargePayload(ctx, type_ctx, span, modal->path, modal->state);
      if (const auto* perm = std::get_if<TypePerm>(&operand.type->node)) {
        SPEC_RULE("T-Modal-Widen-Perm");
        result.ok = true;
        result.type = MakeTypePerm(perm->perm, MakeTypePath(modal->path));
        return result;
      }
      SPEC_RULE("T-Modal-Widen");
      result.ok = true;
      result.type = MakeTypePath(modal->path);
      return result;
    }
    if (const auto* path = std::get_if<TypePathType>(&stripped->node)) {
      if (LookupModalDecl(ctx, path->path)) {
        SPEC_RULE("Widen-AlreadyGeneral");
        result.diag_id = "Widen-AlreadyGeneral";
        return result;
      }
    }
    SPEC_RULE("Widen-NonModal");
    result.diag_id = "Widen-NonModal";
    return result;
  }

  const auto* prim = std::get_if<TypePrim>(&operand.type->node);
  if (!prim) {
    return result;
  }
  const std::string_view name = prim->name;
  if (op == "!") {
    if (name == "bool") {
      SPEC_RULE("T-Not-Bool");
      result.ok = true;
      result.type = MakeTypePrim("bool");
      return result;
    }
    if (IsIntType(name)) {
      SPEC_RULE("T-Not-Int");
      result.ok = true;
      result.type = MakeTypePrim(std::string(name));
      return result;
    }
    return result;
  }

  if (op == "-") {
    if (IsSignedIntType(name) || IsFloatType(name)) {
      SPEC_RULE("T-Neg");
      result.ok = true;
      result.type = MakeTypePrim(std::string(name));
      return result;
    }
    return result;
  }

  return result;
}

}  // namespace cursive0::analysis::expr
