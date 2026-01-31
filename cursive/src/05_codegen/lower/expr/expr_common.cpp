// =============================================================================
// MIGRATION MAPPING: expr/expr_common.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Section 6 (Code Generation)
//   - LowerExpr judgment (Lines 16048+)
//   - Cleanup plan computation (Lines 16586+)
//   - Move state tracking and merging
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/lower/lower_expr_core.cpp
//   - Lines 2799-2825: LowerExpr wrapper function
//   - Lines 2827+: LowerCtx scope tracking methods
//   - Temporary value registration and suppression
//   - Type propagation and value type tracking
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/lower/lower_expr_places.cpp
//   - Place lowering helpers (LowerPlace, LowerReadPlace, LowerAddrOf)
//   - Cleanup plan computation (ComputeCleanupPlanForCurrentScope)
//   - EmitCleanupWithRemainder
//
// DEPENDENCIES:
//   - cursive/src/05_codegen/ir_model.h (All IR types)
//   - cursive/src/05_codegen/lower/lower_ctx.h (LowerCtx, LowerResult)
//   - cursive/src/04_analysis/types.h (TypeRef)
//
// REFACTORING NOTES:
//   - Core LowerExpr dispatch should be in this file
//   - Cleanup utilities shared across all expression lowering
//   - Move state merging for branching constructs
//
// =============================================================================

#include "cursive/05_codegen/lower/expr/expr_common.h"

namespace cursive::codegen {

// TODO: Migrate implementation from cursive-bootstrap

}  // namespace cursive::codegen
