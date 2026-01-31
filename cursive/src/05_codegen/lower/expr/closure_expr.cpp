// =============================================================================
// MIGRATION MAPPING: expr/closure_expr.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Lines 16258-16286 (Closure Lowering)
//   - Lower-Expr-Closure-NonCapturing: ClosureVal(null, sym)
//   - Lower-Expr-Closure-Capturing: Environment allocation + ClosureVal
//   - Lower-Closure-Call: IndirectCall with env_ptr
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/lower/lower_expr_core.cpp
//   - [TODO: Find exact lines for ClosureExpr visitor]
//   - Creates environment record for captures
//   - Generates closure procedure symbol
//
// DEPENDENCIES:
//   - cursive/src/05_codegen/ir_model.h (IRClosure, ClosureVal)
//   - cursive/src/04_analysis/caps/cap_capture.h (Capture analysis)
//
// NOTE: Closures may be unsupported in Cursive0 per CLAUDE.md
//
// =============================================================================

#include "cursive/05_codegen/lower/expr/closure_expr.h"

namespace cursive::codegen {

// TODO: Migrate implementation from cursive-bootstrap

}  // namespace cursive::codegen
