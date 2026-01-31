// =============================================================================
// MIGRATION MAPPING: stmt/frame_stmt.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Lines 16664-16672
//   - Lower-Stmt-Frame-Implicit: FrameIR(none, IR_b, v_b)
//   - Lower-Stmt-Frame-Explicit: SeqIR(IR_r, FrameIR(v_r, IR_b, v_b))
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/lower/lower_stmt.cpp
//   - Lines 747-854: FrameStmt case in LowerStmt dispatch
//   - Creates mark/reset calls for region subdivision
//   - Uses Region::mark and Region::reset_to runtime symbols
//   - Registers defer for reset to ensure cleanup
//
// DEPENDENCIES:
//   - cursive/src/05_codegen/ir_model.h (IRFrame, IRCall)
//   - cursive/src/05_codegen/runtime/runtime_interface.h (RegionSymMark, RegionSymResetTo)
//   - cursive/src/05_codegen/cleanup.h (cleanup plan computation)
//
// =============================================================================

#include "cursive/05_codegen/lower/stmt/frame_stmt.h"

namespace cursive::codegen {

// TODO: Migrate implementation from cursive-bootstrap

}  // namespace cursive::codegen
