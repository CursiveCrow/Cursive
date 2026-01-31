// =============================================================================
// MIGRATION MAPPING: expr/if_expr.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Section 6.4 (Expression Lowering)
//   - Lines 16203-16206: (Lower-Expr-If)
//     Gamma |- LowerExpr(cond) => <IR_c, v_c>
//     Gamma |- LowerBlock(b_1) => <IR_1, v_1>
//     Gamma |- LowerBlock(b_2) => <IR_2, v_2>
//     ---
//     Gamma |- LowerExpr(IfExpr(cond, b_1, b_2)) => <SeqIR(IR_c, IfIR(...)), v_if>
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/lower/lower_expr_core.cpp
//   - IfExpr visitor produces IRIf
//
// DEPENDENCIES:
//   - cursive/src/05_codegen/ir_model.h (IRIf, IRValue)
//
// =============================================================================

#include "cursive/05_codegen/lower/expr/if_expr.h"

namespace cursive::codegen {

// TODO: Migrate implementation from cursive-bootstrap

}  // namespace cursive::codegen
