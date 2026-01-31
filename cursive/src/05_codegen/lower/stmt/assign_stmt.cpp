// =============================================================================
// MIGRATION MAPPING: stmt/assign_stmt.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Lines 16639-16642 (Lower-Stmt-Assign)
//   - LowerExpr(expr) produces IR_e, v
//   - LowerWritePlace(place, v) produces IR_w
//   - Result: SeqIR(IR_e, IR_w)
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/lower/lower_stmt.cpp
//   - Lines 366-376: LowerAssignStmt function
//   - Simple RHS lowering then write to place
//
// DEPENDENCIES:
//   - cursive/src/05_codegen/ir_model.h (IRWriteVar, IRWriteField, etc.)
//   - cursive/src/05_codegen/lower/expr/expr_common.h (LowerWritePlace)
//
// =============================================================================

#include "cursive/05_codegen/lower/stmt/assign_stmt.h"

namespace cursive::codegen {

// TODO: Migrate implementation from cursive-bootstrap

}  // namespace cursive::codegen
