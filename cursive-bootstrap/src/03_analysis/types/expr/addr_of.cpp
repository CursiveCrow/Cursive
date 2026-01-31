// =================================================================
// File: 03_analysis/types/expr/addr_of.cpp
// Construct: Address-Of Expression Type Checking
// Spec Section: 5.2.12
// Spec Rules: T-AddrOf, AddrOf-NonPlace, AddrOf-Index-Array-NonUsize,
//             AddrOf-Index-Slice-NonUsize
// =================================================================
#include "cursive0/03_analysis/types/expr/addr_of.h"

#include "cursive0/00_core/assert_spec.h"
#include "cursive0/03_analysis/types/type_equiv.h"
#include "cursive0/03_analysis/types/type_expr.h"

namespace cursive0::analysis::expr {

// (T-AddrOf)
ExprTypeResult TypeAddressOfExprImpl(const ScopeContext& ctx,
                                     const StmtTypeContext& type_ctx,
                                     const syntax::AddressOfExpr& expr,
                                     const TypeEnv& env) {
  ExprTypeResult result;

  // Must be a place expression
  if (!IsPlaceExpr(expr.place)) {
    SPEC_RULE("AddrOf-NonPlace");
    result.diag_id = "AddrOf-NonPlace";
    return result;
  }

  if (expr.place) {
    if (const auto* index = std::get_if<syntax::IndexAccessExpr>(&expr.place->node)) {
      const auto idx_type = TypeExpr(ctx, type_ctx, index->index, env);
      if (!idx_type.ok) {
        result.diag_id = idx_type.diag_id;
        return result;
      }
      const auto idx_stripped = StripPerm(idx_type.type);
      if (!IsPrimType(idx_stripped, "usize")) {
        const auto base_type = TypePlace(ctx, type_ctx, index->base, env);
        if (!base_type.ok) {
          result.diag_id = base_type.diag_id;
          return result;
        }
        const auto stripped = StripPerm(base_type.type);
        if (stripped && std::holds_alternative<TypeArray>(stripped->node)) {
          SPEC_RULE("AddrOf-Index-Array-NonUsize");
          result.diag_id = "Index-Array-NonUsize";
          return result;
        }
        if (stripped && std::holds_alternative<TypeSlice>(stripped->node)) {
          SPEC_RULE("AddrOf-Index-Slice-NonUsize");
          result.diag_id = "Index-Slice-NonUsize";
          return result;
        }
        result.diag_id = "Index-NonIndexable";
        return result;
      }

      if (!index->index ||
          !std::holds_alternative<syntax::RangeExpr>(index->index->node)) {
        const auto base_place = TypePlace(ctx, type_ctx, index->base, env);
        if (!base_place.ok) {
          result.diag_id = base_place.diag_id;
          return result;
        }
        const auto stripped = StripPerm(base_place.type);
        if (const auto* array = stripped ? std::get_if<TypeArray>(&stripped->node)
                                         : nullptr) {
          const auto index_const = ConstLen(ctx, index->index);
          const bool has_const_index = index_const.ok && index_const.value.has_value();
          if (!has_const_index) {
            if (!type_ctx.contract_dynamic) {
              result.diag_id = "Index-Array-NonConst-Err";
              return result;
            }
          } else if (*index_const.value >= array->length) {
            result.diag_id = "Index-Array-OOB-Err";
            return result;
          }
          TypeRef out_type = array->element;
          if (const auto* perm = std::get_if<TypePerm>(&base_place.type->node)) {
            out_type = MakeTypePerm(perm->perm, out_type);
          }
          SPEC_RULE("T-AddrOf");
          result.ok = true;
          result.type = MakeTypePtr(out_type, PtrState::Valid);
          return result;
        }
      }
    }
  }

  const auto place = TypePlace(ctx, type_ctx, expr.place, env);
  if (!place.ok) {
    result.diag_id = place.diag_id;
    return result;
  }

  SPEC_RULE("T-AddrOf");
  result.ok = true;
  result.type = MakeTypePtr(place.type, PtrState::Valid);
  return result;
}

}  // namespace cursive0::analysis::expr
