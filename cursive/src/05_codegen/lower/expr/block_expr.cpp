// =============================================================================
// MIGRATION MAPPING: expr/block_expr.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Section 6.5 (Statement and Block Lowering)
//   - Lines 16228-16231: (Lower-Expr-Block)
//   - Lines 16731-16739: (Lower-Block-Tail) and (Lower-Block-Unit)
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/lower/lower_stmt.cpp
//   - Lines 211-262: LowerBlock
//
// DEPENDENCIES:
//   - cursive/src/05_codegen/ir_model.h (IRBlock, IRValue)
//   - cursive/src/05_codegen/cleanup.h (ComputeCleanupPlanForCurrentScope)
//
// =============================================================================

#include "cursive/05_codegen/lower/expr/block_expr.h"

namespace cursive::codegen {

// TODO: Migrate implementation from cursive-bootstrap

}  // namespace cursive::codegen
