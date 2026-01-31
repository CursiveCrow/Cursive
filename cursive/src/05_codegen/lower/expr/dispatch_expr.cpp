// =============================================================================
// MIGRATION MAPPING: expr/dispatch_expr.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Section 10.5 (Dispatch - Data Parallelism)
//   - dispatch i in range { body } with optional reduce/ordered/chunk
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/lower/lower_expr_core.cpp
//   - DispatchExpr visitor produces IRDispatch
//   - Reduction handling for parallel aggregation
//
// DEPENDENCIES:
//   - cursive/src/05_codegen/ir_model.h (IRDispatch)
//
// =============================================================================

#include "cursive/05_codegen/lower/expr/dispatch_expr.h"

namespace cursive::codegen {

// TODO: Migrate implementation from cursive-bootstrap

}  // namespace cursive::codegen
