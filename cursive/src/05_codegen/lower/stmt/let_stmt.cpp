// =============================================================================
// MIGRATION MAPPING: stmt/let_stmt.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Lines 16619-16622 (Lower-Stmt-Let)
//   - LowerExpr(init) produces IR_i, v
//   - LowerBindPattern(pattern, v) produces IR_b
//   - Result: SeqIR(IR_i, IR_b)
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/lower/lower_stmt.cpp
//   - Lines 268-311: LowerLetStmt function
//   - Handles immovable bindings (:= operator)
//   - Pattern binding with RegisterPatternBindings
//   - Provenance tracking for region safety
//
// DEPENDENCIES:
//   - cursive/src/05_codegen/ir_model.h (IRBindVar)
//   - cursive/src/05_codegen/lower/lower_ctx.h (LowerCtx)
//   - cursive/src/05_codegen/lower/expr/expr_common.h (LowerExpr)
//   - cursive/src/05_codegen/lower/pattern/pattern_common.h (LowerBindPattern)
//
// =============================================================================

#include "cursive/05_codegen/lower/stmt/let_stmt.h"

namespace cursive::codegen {

// TODO: Migrate implementation from cursive-bootstrap

}  // namespace cursive::codegen
