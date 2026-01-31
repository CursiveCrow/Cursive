// =============================================================================
// MIGRATION MAPPING: expr/propagate_expr.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Section 6.4 (Expression Lowering)
//   - Lines 16188-16196: (Lower-Expr-Propagate-Success) and (Lower-Expr-Propagate-Return)
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/lower/lower_expr_core.cpp
//   - PropagateExpr (?) visitor with union type handling
//
// DEPENDENCIES:
//   - cursive/src/05_codegen/ir_model.h (IRMatch, IRReturn, IRValue)
//   - cursive/src/04_analysis/types/types.h (TypeUnion, SuccessMember)
//
// NOTES:
//   - Propagate (?) returns early for non-success union members
//   - Success member extracted and returned as expression value
//
// =============================================================================

#include "cursive/05_codegen/lower/expr/propagate_expr.h"

namespace cursive::codegen {

// TODO: Migrate implementation from cursive-bootstrap

}  // namespace cursive::codegen
