// =============================================================================
// MIGRATION MAPPING: expr/sync_expr.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Section 11.3 (Sync Expression)
//   - sync e runs async to completion synchronously
//   - Constraints: Out = (), In = ()
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/lower/lower_expr_core.cpp
//   - Lines 2629-2650: SyncExpr visitor
//   - SPEC_RULE(Lower-Expr-Sync)
//   - Creates IRSync with async value and result type extraction
//
// DEPENDENCIES:
//   - cursive/src/05_codegen/ir_model.h (IRSync)
//   - cursive/src/04_analysis/types.h (GetAsyncSig)
//
// =============================================================================

#include "cursive/05_codegen/lower/expr/sync_expr.h"

namespace cursive::codegen {

// TODO: Migrate implementation from cursive-bootstrap

}  // namespace cursive::codegen
