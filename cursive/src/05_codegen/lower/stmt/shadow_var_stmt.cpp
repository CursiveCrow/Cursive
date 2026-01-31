// =============================================================================
// MIGRATION MAPPING: stmt/shadow_var_stmt.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Lines 16634-16637 (Lower-Stmt-ShadowVar)
//   - LowerExpr(init) produces IR_i, v
//   - SeqIR(IR_i, BindVarIR(x, v))
//   - Mutable shadow binding
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/lower/lower_stmt.cpp
//   - Lines 661-688: ShadowVarStmt case in LowerStmt dispatch
//   - Nearly identical to ShadowLetStmt
//   - Registers as mutable
//
// DEPENDENCIES:
//   - cursive/src/05_codegen/ir_model.h (IRBindVar)
//   - cursive/src/05_codegen/lower/lower_ctx.h (LowerCtx::RegisterVar)
//
// =============================================================================

#include "cursive/05_codegen/lower/stmt/shadow_var_stmt.h"

namespace cursive::codegen {

// TODO: Migrate implementation from cursive-bootstrap

}  // namespace cursive::codegen
