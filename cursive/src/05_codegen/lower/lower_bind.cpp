// =============================================================================
// MIGRATION MAPPING: lower_bind.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Section 6.5 (Statement Lowering)
//   - Binding initialization and pattern destructuring
//   - Lines 16619-16637 (Lower-Stmt-Let, Lower-Stmt-Var, Shadow variants)
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/lower/lower_stmt.cpp
//   - Lines 171-183: LowerBindingType helper
//   - Pattern binding type resolution
//   - Provenance tracking for region safety
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/lower/lower_pat.cpp
//   - Lines 240-370: RegisterPatternBindings
//   - Lines 376+: LowerBindPattern
//
// DEPENDENCIES:
//   - cursive/src/05_codegen/ir_model.h (IRBindVar)
//   - cursive/src/05_codegen/lower/lower_ctx.h (RegisterVar)
//   - cursive/src/05_codegen/lower/pattern/pattern_common.h
//   - cursive/src/05_codegen/layout/layout.h (LowerTypeForLayout)
//
// REFACTORING NOTES:
//   - This file bridges statement lowering and pattern matching
//   - ProvInfo struct should be part of lower_ctx
//   - Consider unifying binding registration across let/var/shadow
//
// =============================================================================

#include "cursive/05_codegen/lower/lower_bind.h"

namespace cursive::codegen {

// TODO: Migrate implementation from cursive-bootstrap

}  // namespace cursive::codegen
