// =================================================================
// File: 03_analysis/types/expr/binary.cpp
// Construct: Binary Expression Type Checking
// Spec Section: 5.2.12
// Spec Rules: T-Arith, T-Bitwise, T-Shift, T-Compare-Eq, T-Compare-Ord, T-Logical
// =================================================================
#include "cursive0/03_analysis/types/expr/binary.h"

#include "cursive0/00_core/assert_spec.h"
#include "cursive0/03_analysis/types/expr/common.h"
#include "cursive0/03_analysis/types/type_equiv.h"
#include "cursive0/03_analysis/types/type_expr.h"

namespace cursive0::analysis::expr {

// (T-Arith), (T-Bitwise), (T-Shift), (T-Compare-Eq), (T-Compare-Ord),
// (T-Logical)
ExprTypeResult TypeBinaryExprImpl(const ScopeContext& ctx,
                                  const StmtTypeContext& type_ctx,
                                  const syntax::BinaryExpr& expr,
                                  const TypeEnv& env) {
  ExprTypeResult result;

  const auto lhs = TypeExpr(ctx, type_ctx, expr.lhs, env);
  if (!lhs.ok) {
    result.diag_id = lhs.diag_id;
    return result;
  }

  const auto rhs = TypeExpr(ctx, type_ctx, expr.rhs, env);
  if (!rhs.ok) {
    result.diag_id = rhs.diag_id;
    return result;
  }

  const auto lhs_name = GetPrimName(lhs.type);
  const auto rhs_name = GetPrimName(rhs.type);
  const std::string_view op = expr.op;

  if (IsArithOp(op)) {
    if (!lhs_name.has_value() || !rhs_name.has_value()) {
      return result;
    }
    const auto equiv = TypeEquiv(lhs.type, rhs.type);
    if (!equiv.ok) {
      result.diag_id = equiv.diag_id;
      return result;
    }
    if (!equiv.equiv) {
      return result;
    }
    if (!IsNumericType(*lhs_name)) {
      return result;
    }
    SPEC_RULE("T-Arith");
    result.ok = true;
    result.type = MakeTypePrim(std::string(*lhs_name));
    return result;
  }

  if (IsBitOp(op)) {
    if (!lhs_name.has_value() || !rhs_name.has_value()) {
      return result;
    }
    const auto equiv = TypeEquiv(lhs.type, rhs.type);
    if (!equiv.ok) {
      result.diag_id = equiv.diag_id;
      return result;
    }
    if (!equiv.equiv) {
      return result;
    }
    if (!IsIntType(*lhs_name)) {
      return result;
    }
    SPEC_RULE("T-Bitwise");
    result.ok = true;
    result.type = MakeTypePrim(std::string(*lhs_name));
    return result;
  }

  if (IsShiftOp(op)) {
    if (!lhs_name.has_value()) {
      return result;
    }
    if (!IsIntType(*lhs_name)) {
      return result;
    }
    if (!IsPrimType(rhs.type, "u32")) {
      return result;
    }
    SPEC_RULE("T-Shift");
    result.ok = true;
    result.type = MakeTypePrim(std::string(*lhs_name));
    return result;
  }

  if (IsEqOp(op)) {
    if (!EqType(lhs.type)) {
      return result;
    }
    const auto equiv = TypeEquiv(lhs.type, rhs.type);
    if (!equiv.ok) {
      result.diag_id = equiv.diag_id;
      return result;
    }
    if (!equiv.equiv) {
      return result;
    }
    SPEC_RULE("T-Compare-Eq");
    result.ok = true;
    result.type = MakeTypePrim("bool");
    return result;
  }

  if (IsOrdOp(op)) {
    if (!OrdType(lhs.type)) {
      return result;
    }
    const auto equiv = TypeEquiv(lhs.type, rhs.type);
    if (!equiv.ok) {
      result.diag_id = equiv.diag_id;
      return result;
    }
    if (!equiv.equiv) {
      return result;
    }
    SPEC_RULE("T-Compare-Ord");
    result.ok = true;
    result.type = MakeTypePrim("bool");
    return result;
  }

  if (IsLogicOp(op)) {
    if (!IsPrimType(lhs.type, "bool") || !IsPrimType(rhs.type, "bool")) {
      return result;
    }
    SPEC_RULE("T-Logical");
    result.ok = true;
    result.type = MakeTypePrim("bool");
    return result;
  }

  return result;
}

}  // namespace cursive0::analysis::expr
