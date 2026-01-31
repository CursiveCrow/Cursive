// =============================================================================
// MIGRATION MAPPING: expr/range.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Section 6.4 (Expression Lowering)
//   - Lines 16198-16201: (Lower-Expr-Range)
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/lower/lower_expr_core.cpp
//   - RangeExpr visitor produces IRRange value
//   - Handles all range kinds: Full, From, To, ToInclusive, Exclusive, Inclusive
//
// DEPENDENCIES:
//   - cursive/src/05_codegen/ir_model.h (IRRange, RangeKind)
//
// =============================================================================

#include "cursive/05_codegen/lower/expr/range.h"

namespace cursive::codegen {

// TODO: Migrate implementation from cursive-bootstrap

}  // namespace cursive::codegen
