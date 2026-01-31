// =============================================================================
// MIGRATION MAPPING: stmt/expr_stmt.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Lines 16650-16653 (Lower-Stmt-Expr)
//   - LowerExpr(expr) produces IR_e
//   - Result value discarded (side effects only)
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/lower/lower_stmt.cpp
//   - Lines 436-456: LowerExprStmt function
//   - Handles parallel_collect for spawn/dispatch expressions
//   - Returns only the IR, discards value
//
// DEPENDENCIES:
//   - cursive/src/05_codegen/lower/expr/expr_common.h (LowerExpr)
//   - cursive/src/05_codegen/lower/lower_ctx.h (parallel_collect)
//
// =============================================================================

#include "cursive/05_codegen/lower/stmt/expr_stmt.h"

namespace cursive::codegen {

// TODO: Migrate implementation from cursive-bootstrap

}  // namespace cursive::codegen
