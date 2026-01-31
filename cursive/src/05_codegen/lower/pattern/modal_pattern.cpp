// =============================================================================
// MIGRATION MAPPING: pattern/modal_pattern.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Lines 16782-16785
//   - TagOf-Modal: Get modal state index
//   - StateIndex(M, S) = i
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/lower/lower_pat.cpp
//   - Lines 332-356: ModalPattern in RegisterPatternBindings
//   - Extracts modal path from type hint
//   - Looks up ModalDecl and state-specific field types
//   - LowerBindPattern: state tag comparison + field extraction
//
// DEPENDENCIES:
//   - cursive/src/05_codegen/ir_model.h (IRTagOf)
//   - Helper functions: LookupModalDecl, FindState, ModalFieldType
//
// =============================================================================

#include "cursive/05_codegen/lower/pattern/modal_pattern.h"

namespace cursive::codegen {

// TODO: Migrate implementation from cursive-bootstrap

}  // namespace cursive::codegen
