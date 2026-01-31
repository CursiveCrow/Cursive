// =============================================================================
// MIGRATION MAPPING: expr/transmute_expr.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Section 6.4 (Expression Lowering)
//   - Lines 16183-16186: (Lower-Expr-Transmute)
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/lower/lower_expr_core.cpp
//   - TransmuteExpr visitor produces IRTransmute
//
// DEPENDENCIES:
//   - cursive/src/05_codegen/ir_model.h (IRTransmute, IRValue)
//
// NOTES:
//   - Transmute requires unsafe block
//   - Source and target types must have same size
//
// =============================================================================

#include "cursive/05_codegen/lower/expr/transmute_expr.h"

namespace cursive::codegen {

// TODO: Migrate implementation from cursive-bootstrap

}  // namespace cursive::codegen
