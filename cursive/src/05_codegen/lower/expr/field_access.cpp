// =============================================================================
// MIGRATION MAPPING: expr/field_access.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Section 6.4 (Expression Lowering)
//   - Lines 16104-16107: (Lower-Expr-FieldAccess)
//     Gamma |- LowerExpr(base) => <IR_b, v_b>    FieldValue(v_b, f) = v_f
//     Gamma |- LowerExpr(FieldAccess(base, f)) => <IR_b, v_f>
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/lower/lower_expr_places.cpp
//   - Lines 379-391: LowerReadPlace for FieldAccessExpr
//
// DEPENDENCIES:
//   - cursive/src/05_codegen/ir_model.h (DerivedValueInfo::Kind::Field)
//   - cursive/src/05_codegen/layout/layout.h (field offset calculation)
//
// =============================================================================

#include "cursive/05_codegen/lower/expr/field_access.h"

namespace cursive::codegen {

// TODO: Migrate implementation from cursive-bootstrap

}  // namespace cursive::codegen
