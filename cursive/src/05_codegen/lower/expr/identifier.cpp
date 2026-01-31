// =============================================================================
// MIGRATION MAPPING: expr/identifier.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Section 6.4 (Expression Lowering)
//   - Lines 16057-16065: (Lower-Expr-Ident-Local) and (Lower-Expr-Ident-Path)
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/lower/lower_expr_places.cpp
//   - Lines 310-378: LowerReadPlace for IdentifierExpr
//
// DEPENDENCIES:
//   - cursive/src/05_codegen/ir_model.h (IRReadVar, IRReadPath, IRValue)
//   - cursive/src/05_codegen/lower/lower_ctx.h (LowerCtx)
//
// REFACTORING NOTES:
//   1. Local binding: IRReadVar
//   2. Capture: Load from capture environment
//   3. Path/global: IRReadPath with resolution
//
// =============================================================================

#include "cursive/05_codegen/lower/expr/identifier.h"

namespace cursive::codegen {

// TODO: Migrate implementation from cursive-bootstrap

}  // namespace cursive::codegen
