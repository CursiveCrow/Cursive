// =============================================================================
// MIGRATION MAPPING: expr/tuple_access.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Section 6.4 (Expression Lowering)
//   - Lines 16109-16112: (Lower-Expr-TupleAccess)
//     Gamma |- LowerExpr(base) => <IR_b, v_b>    TupleValue(v_b, i) = v_i
//     Gamma |- LowerExpr(TupleAccess(base, i)) => <IR_b, v_i>
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/lower/lower_expr_places.cpp
//   - Lines 392-404: LowerReadPlace for TupleAccessExpr
//
// DEPENDENCIES:
//   - cursive/src/05_codegen/ir_model.h (DerivedValueInfo::Kind::Tuple)
//
// =============================================================================

#include "cursive/05_codegen/lower/expr/tuple_access.h"

namespace cursive::codegen {

// TODO: Migrate implementation from cursive-bootstrap

}  // namespace cursive::codegen
