// =============================================================================
// MIGRATION MAPPING: expr/cast.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Section 6.4 (Expression Lowering)
//   - Lines 16178-16181: (Lower-Expr-Cast)
//   - Lines 16338-16346: (Lower-Cast) and (Lower-Cast-Panic)
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/lower/lower_expr_core.cpp
//   - CastExpr visitor with type conversion
//   - Panic for invalid casts (e.g., truncation overflow)
//
// DEPENDENCIES:
//   - cursive/src/05_codegen/ir_model.h (IRCast, IRValue)
//   - cursive/src/05_codegen/checks.h (LowerPanic, PanicReason::Cast)
//
// CAST KINDS:
//   - Numeric widening (i32 -> i64)
//   - Numeric narrowing (i64 -> i32) - may panic
//   - Pointer casts
//   - Bool to int, int to bool
//
// =============================================================================

#include "cursive/05_codegen/lower/expr/cast.h"

namespace cursive::codegen {

// TODO: Migrate implementation from cursive-bootstrap

}  // namespace cursive::codegen
