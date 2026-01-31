// =============================================================================
// MIGRATION MAPPING: expr/error_expr.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Section 6.4 (Expression Lowering)
//   - Line 16071-16073: (Lower-Expr-Error)
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/lower/lower_expr_core.cpp
//   - ErrorExpr visitor produces panic IR
//
// DEPENDENCIES:
//   - cursive/src/05_codegen/checks.h (LowerPanic, PanicReason::ErrorExpr)
//
// =============================================================================

#include "cursive/05_codegen/lower/expr/error_expr.h"

namespace cursive::codegen {

// TODO: Migrate implementation from cursive-bootstrap

}  // namespace cursive::codegen
