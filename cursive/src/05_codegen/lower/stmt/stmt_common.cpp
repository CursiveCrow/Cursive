// =============================================================================
// MIGRATION MAPPING: stmt/stmt_common.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Lines 16586-16756 (Statement Lowering)
//   - LowerStmt judgment dispatch
//   - LowerStmtList, LowerBlock, LowerLoop
//   - Temporary cleanup handling (TempDropOrder, TempCleanupIR)
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/lower/lower_stmt.cpp
//   - Lines 1-90: Helper functions and namespace definitions
//   - Lines 191-205: LowerStmtList
//   - Lines 211-262: LowerBlock
//   - Lines 599-896: LowerStmt main dispatch
//   - Lines 902-1078: Loop lowering (Infinite, Conditional, Iter)
//
// DEPENDENCIES:
//   - cursive/src/05_codegen/ir_model.h (All IR types)
//   - cursive/src/05_codegen/lower/lower_ctx.h (LowerCtx, scopes)
//   - cursive/src/05_codegen/cleanup.h (CleanupPlan, EmitCleanup)
//   - All individual stmt/*.h headers
//
// REFACTORING NOTES:
//   - Main dispatch should delegate to individual stmt files
//   - LowerStmtList and LowerBlock are shared utilities
//   - Temp sink handling is cross-cutting concern
//
// =============================================================================

#include "cursive/05_codegen/lower/stmt/stmt_common.h"

namespace cursive::codegen {

// TODO: Migrate implementation from cursive-bootstrap

}  // namespace cursive::codegen
