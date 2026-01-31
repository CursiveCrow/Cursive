// =============================================================================
// MIGRATION MAPPING: expr/alloc_expr.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Section 6.4 (Expression Lowering)
//   - Lines 16253-16256: (Lower-Expr-Alloc)
//     Gamma |- LowerExpr(e) => <IR_e, v>
//     Gamma |- LowerExpr(AllocExpr(r_opt, e)) => <SeqIR(IR_e, AllocIR(r_opt, v)), v_alloc>
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/lower/lower_expr_core.cpp
//   - AllocExpr visitor produces IRAlloc
//
// DEPENDENCIES:
//   - cursive/src/05_codegen/ir_model.h (IRAlloc, IRValue)
//   - cursive/src/05_codegen/layout/layout.h (SizeOf, AlignOf)
//
// NOTES:
//   - Alloc (^expr) allocates in active region
//   - Region target optional (default: innermost active region)
//
// =============================================================================

#include "cursive/05_codegen/lower/expr/alloc_expr.h"

namespace cursive::codegen {

// TODO: Migrate implementation from cursive-bootstrap

}  // namespace cursive::codegen
