// =============================================================================
// MIGRATION MAPPING: expr/pipeline_expr.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Lines 16288-16302 (Pipeline Lowering)
//   - Lower-Expr-Pipeline: e1 |> e2 desugars to function/closure application
//   - IsFunc case: CallIR(v2, [v1])
//   - IsClosure case: IndirectCall(code, [env, v1])
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/lower/lower_expr_core.cpp
//   - [TODO: Find exact lines for PipelineExpr visitor]
//   - Determines if RHS is function or closure
//   - Produces appropriate call IR
//
// DEPENDENCIES:
//   - cursive/src/05_codegen/ir_model.h (CallIR, IndirectCall)
//   - Type checking helpers (IsFunc, IsClosure)
//
// NOTE: Pipelines may be unsupported in Cursive0 per CLAUDE.md
//
// =============================================================================

#include "cursive/05_codegen/lower/expr/pipeline_expr.h"

namespace cursive::codegen {

// TODO: Migrate implementation from cursive-bootstrap

}  // namespace cursive::codegen
