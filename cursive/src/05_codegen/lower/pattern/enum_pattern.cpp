// =============================================================================
// MIGRATION MAPPING: pattern/enum_pattern.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Lines 16772-16780
//   - TagOf-Enum: Get enum discriminant for variant
//   - EnumValuePath, VariantIndex, EnumDisc
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/lower/lower_pat.cpp
//   - Lines 295-331: EnumPattern in RegisterPatternBindings
//   - Looks up EnumDecl and VariantDecl
//   - Handles TuplePayloadPattern and RecordPayloadPattern
//   - Recursively registers payload element/field bindings
//   - LowerBindPattern: tag comparison + payload extraction
//
// DEPENDENCIES:
//   - cursive/src/05_codegen/ir_model.h (IRTagOf, comparison IR)
//   - Helper functions: LookupEnumDecl, FindVariant
//
// =============================================================================

#include "cursive/05_codegen/lower/pattern/enum_pattern.h"

namespace cursive::codegen {

// TODO: Migrate implementation from cursive-bootstrap

}  // namespace cursive::codegen
