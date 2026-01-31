// =============================================================================
// MIGRATION MAPPING: stmt/defer_stmt.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Lines 16655-16657 (Lower-Stmt-Defer)
//   - DeferIR(block) - deferred block stored for cleanup
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/lower/lower_stmt.cpp
//   - Lines 695-701: DeferStmt case in LowerStmt dispatch
//   - Creates IRDefer and registers with context
//   - Defers run during scope cleanup (reverse order)
//
// DEPENDENCIES:
//   - cursive/src/05_codegen/ir_model.h (IRDefer)
//   - cursive/src/05_codegen/lower/lower_ctx.h (LowerCtx::RegisterDefer)
//
// =============================================================================

#include "cursive/05_codegen/lower/stmt/defer_stmt.h"

namespace cursive::codegen {

// TODO: Migrate implementation from cursive-bootstrap

}  // namespace cursive::codegen
