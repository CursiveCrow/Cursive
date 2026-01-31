// =============================================================================
// MIGRATION MAPPING: pattern/range_pattern.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Section 6.6 (Pattern Matching Lowering)
//   - Range patterns match values within a range
//   - lo..hi (exclusive) or lo..=hi (inclusive)
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/lower/lower_pat.cpp
//   - Lines 356-364: RangePattern in RegisterPatternBindings
//   - Recursively processes lo and hi patterns (usually literals)
//   - LowerBindPattern: range bounds checking IR
//
// DEPENDENCIES:
//   - cursive/src/05_codegen/ir_model.h (comparison IR)
//
// =============================================================================

#include "cursive/05_codegen/lower/pattern/range_pattern.h"

namespace cursive::codegen {

// TODO: Migrate implementation from cursive-bootstrap

}  // namespace cursive::codegen
