// =============================================================================
// MIGRATION MAPPING: expr/wait_expr.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Section 10.4 (Wait Expression)
//   - wait handle blocks until spawned task completes
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/lower/lower_expr_core.cpp
//   - WaitExpr visitor produces IRWait
//
// DEPENDENCIES:
//   - cursive/src/05_codegen/ir_model.h (IRWait)
//
// =============================================================================

#include "cursive/05_codegen/lower/expr/wait_expr.h"

namespace cursive::codegen {

// TODO: Migrate implementation from cursive-bootstrap

}  // namespace cursive::codegen
