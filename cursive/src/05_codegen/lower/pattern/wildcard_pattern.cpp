// =============================================================================
// MIGRATION MAPPING: pattern/wildcard_pattern.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Section 6.6 (Pattern Matching Lowering)
//   - Wildcard pattern (_) matches anything
//   - Creates no bindings
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/lower/lower_pat.cpp
//   - WildcardPattern case in RegisterPatternBindings (lines 251-252)
//   - Returns immediately, no action needed
//   - WildcardPattern case in LowerBindPattern: EmptyIR
//
// DEPENDENCIES:
//   - cursive/src/05_codegen/ir_model.h (EmptyIR)
//
// =============================================================================

#include "cursive/05_codegen/lower/pattern/wildcard_pattern.h"

namespace cursive::codegen {

// TODO: Migrate implementation from cursive-bootstrap

}  // namespace cursive::codegen
