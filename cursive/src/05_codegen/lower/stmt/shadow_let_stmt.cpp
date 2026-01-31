// =============================================================================
// MIGRATION MAPPING: stmt/shadow_let_stmt.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Lines 16629-16632 (Lower-Stmt-ShadowLet)
//   - LowerExpr(init) produces IR_i, v
//   - SeqIR(IR_i, BindVarIR(x, v))
//   - Shadows previous binding of same name
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/lower/lower_stmt.cpp
//   - Lines 633-660: ShadowLetStmt case in LowerStmt dispatch
//   - Uses simple name binding (no pattern)
//   - Registers with RegisterVar, immutable
//
// DEPENDENCIES:
//   - cursive/src/05_codegen/ir_model.h (IRBindVar)
//   - cursive/src/05_codegen/lower/lower_ctx.h (LowerCtx::RegisterVar)
//
// =============================================================================

#include "cursive/05_codegen/lower/stmt/shadow_let_stmt.h"

namespace cursive::codegen {

// TODO: Migrate implementation from cursive-bootstrap

}  // namespace cursive::codegen
