// =============================================================================
// MIGRATION MAPPING: expr/yield_expr.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Section 11.2 (Yield Expression)
//   - yield value suspends async and produces output
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/lower/lower_expr_core.cpp
//   - YieldExpr visitor produces IRYield with state index
//
// DEPENDENCIES:
//   - cursive/src/05_codegen/ir_model.h (IRYield)
//
// =============================================================================

#include "cursive/05_codegen/lower/expr/yield_expr.h"

namespace cursive::codegen {

// TODO: Migrate implementation from cursive-bootstrap

}  // namespace cursive::codegen
