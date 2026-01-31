// =============================================================================
// MIGRATION MAPPING: pattern/identifier_pattern.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Section 6.6 (Pattern Matching Lowering)
//   - Identifier pattern binds the matched value to a name
//   - Lower-BindList-Cons: SeqIR(BindVarIR(x, v), IR_r)
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/lower/lower_pat.cpp
//   - Lines 255-258: IdentifierPattern in RegisterPatternBindings
//   - Calls ctx.RegisterVar with name, type hint, immovability, provenance
//   - LowerBindPattern: creates IRBindVar
//
// DEPENDENCIES:
//   - cursive/src/05_codegen/ir_model.h (IRBindVar)
//   - cursive/src/05_codegen/lower/lower_ctx.h (RegisterVar)
//
// =============================================================================

#include "cursive/05_codegen/lower/pattern/identifier_pattern.h"

namespace cursive::codegen {

// TODO: Migrate implementation from cursive-bootstrap

}  // namespace cursive::codegen
