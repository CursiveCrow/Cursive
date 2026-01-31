// =============================================================================
// MIGRATION MAPPING: expr/call.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Section 6.4 (Expression Lowering)
//   - Lines 16143-16151: (Lower-Expr-Call-PanicOut) and (Lower-Expr-Call-NoPanicOut)
//     With panic out: appends PanicOutName to args, adds PanicCheck after
//     Without panic out: direct call
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/lower/lower_expr_calls.cpp
//   - CallExpr lowering with argument evaluation
//   - NeedsPanicOut check determines if panic out parameter needed
//
// DEPENDENCIES:
//   - cursive/src/05_codegen/ir_model.h (IRCall, IRValue)
//   - cursive/src/05_codegen/abi/abi.h (NeedsPanicOut, kPanicOutName)
//   - cursive/src/05_codegen/checks.h (PanicCheck)
//
// =============================================================================

#include "cursive/05_codegen/lower/expr/call.h"

namespace cursive::codegen {

// TODO: Migrate implementation from cursive-bootstrap

}  // namespace cursive::codegen
