// =============================================================================
// MIGRATION MAPPING: pattern/pattern_common.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Lines 16757-16813 (Pattern Matching Lowering)
//   - PatternLowerJudg = {LowerBindPattern, LowerBindList, LowerMatch, TagOf}
//   - Lower-Pat-General: MatchPattern + BindOrder + LowerBindList
//   - Lower-Match: LowerExpr(scrut) then MatchIR
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/lower/lower_pat.cpp
//   - Lines 20-28: BoolImmediate helper
//   - Lines 34-42: TagOf function (TagOf-Enum, TagOf-Modal)
//   - Lines 184-236: MergeFailures, MergeMoveStates helpers
//   - Lines 240-370: RegisterPatternBindings (full dispatch)
//   - Lines 376-803: LowerBindPattern (full dispatch)
//
// DEPENDENCIES:
//   - cursive/src/05_codegen/ir_model.h (All IR types)
//   - cursive/src/05_codegen/lower/lower_ctx.h (LowerCtx)
//   - cursive/src/05_codegen/cleanup.h (cleanup helpers)
//   - All individual pattern/*.h headers
//
// REFACTORING NOTES:
//   - Main dispatch functions should delegate to individual pattern files
//   - RegisterPatternBindings and LowerBindPattern are parallel structures
//   - TagOf is shared utility for enum/modal discrimination
//
// =============================================================================

#include "cursive/05_codegen/lower/pattern/pattern_common.h"

namespace cursive::codegen {

// TODO: Migrate implementation from cursive-bootstrap

}  // namespace cursive::codegen
