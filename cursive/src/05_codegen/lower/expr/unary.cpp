// =============================================================================
// MIGRATION MAPPING: expr/unary.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Section 6.4 (Expression Lowering)
//   - Lines 16158-16161: (Lower-Expr-Unary)
//   - Lines 16318-16326: (Lower-UnOp-Ok) and (Lower-UnOp-Panic)
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/lower/lower_expr_core.cpp
//   - UnaryExpr visitor with operator dispatch
//   - Panic for undefined operations (e.g., negation overflow)
//
// DEPENDENCIES:
//   - cursive/src/05_codegen/ir_model.h (IRUnaryOp, IRCheckOp)
//   - cursive/src/05_codegen/checks.h (OpPanicReason)
//
// UNARY OPERATORS: \! (not), - (negate)
//
// =============================================================================

#include "cursive/05_codegen/lower/expr/unary.h"

namespace cursive::codegen {

// TODO: Migrate implementation from cursive-bootstrap

}  // namespace cursive::codegen
