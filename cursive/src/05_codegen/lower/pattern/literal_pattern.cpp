// =============================================================================
// MIGRATION MAPPING: pattern/literal_pattern.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Section 6.6 (Pattern Matching Lowering)
//   - Literal patterns match constant values
//   - Used in match arms for equality checking
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/lower/lower_pat.cpp
//   - LiteralPattern case in RegisterPatternBindings (line 253-254)
//   - LiteralPattern case in LowerBindPattern visitor
//   - No bindings created, just value comparison
//
// DEPENDENCIES:
//   - cursive/src/05_codegen/ir_model.h (comparison IR)
//
// =============================================================================

#include "cursive/05_codegen/lower/pattern/literal_pattern.h"

namespace cursive::codegen {

// TODO: Migrate implementation from cursive-bootstrap

}  // namespace cursive::codegen
