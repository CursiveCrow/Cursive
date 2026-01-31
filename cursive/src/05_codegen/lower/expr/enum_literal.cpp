// =============================================================================
// MIGRATION MAPPING: expr/enum_literal.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Section 6.4 (Expression Lowering)
//   - Lines 16090-16102: (Lower-Expr-Enum-Unit/Tuple/Record)
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/lower/lower_expr_core.cpp
//   - EnumLiteralExpr visitor handles all payload kinds
//
// DEPENDENCIES:
//   - cursive/src/05_codegen/ir_model.h (IREnumLiteral, IRValue)
//   - cursive/src/05_codegen/layout/layout.h (EnumLayout, VariantLayout)
//
// =============================================================================

#include "cursive/05_codegen/lower/expr/enum_literal.h"

namespace cursive::codegen {

// TODO: Migrate implementation from cursive-bootstrap

}  // namespace cursive::codegen
