// =============================================================================
// MIGRATION MAPPING: stmt/error_stmt.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Lines 16701-16703 (Lower-Stmt-Error)
//   - LowerPanic(ErrorStmt(span))
//   - Error statements trigger panic IR
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/lower/lower_stmt.cpp
//   - Lines 626-628: ErrorStmt case in LowerStmt dispatch
//   - Calls LowerPanic with ErrorStmt reason
//
// DEPENDENCIES:
//   - cursive/src/05_codegen/checks.h (LowerPanic, PanicReason::ErrorStmt)
//
// =============================================================================

#include "cursive/05_codegen/lower/stmt/error_stmt.h"

namespace cursive::codegen {

// TODO: Migrate implementation from cursive-bootstrap

}  // namespace cursive::codegen
