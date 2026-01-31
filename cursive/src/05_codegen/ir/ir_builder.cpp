// =============================================================================
// MIGRATION MAPPING: ir_builder.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   - Section 6.0 Codegen Model and Judgments (lines 14196-14347)
//   - ProcIR structure (line 14234)
//   - CodegenParams (line 14237)
//   - MethodParams, ClassMethodParams (lines 14239-14243)
//   - StateMethodParams, TransitionParams (lines 14245-14246)
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/ir_model.cpp
//   - SeqIR builder functions (lines 379-413)
//   - Additional builder patterns may be in lower/*.cpp files
//
// RELATED SOURCE FILES:
//   - cursive-bootstrap/src/04_codegen/lower/lower_expr_core.cpp
//   - cursive-bootstrap/src/04_codegen/lower/lower_stmt.cpp
//   - cursive-bootstrap/src/04_codegen/lower/lower_proc.cpp
//
// DEPENDENCIES:
//   - cursive/include/05_codegen/ir/ir_model.h (IR node types)
//   - cursive/include/05_codegen/ir/ir_builder.h (builder interface)
//   - analysis types for parameter handling
//
// REFACTORING NOTES:
//   1. Implement fluent builder API for IR construction
//   2. Provide helpers for common patterns:
//      - SeqIR for sequencing
//      - BlockIR for scoped blocks
//      - IfIR for conditionals
//      - LoopIR for all loop types
//      - MatchIR for pattern matching
//   3. Handle PanicOutParam insertion per §6.2.3
//   4. Support method receiver transformations (self parameter injection)
//   5. Builder should ensure IR validation invariants
//
// BUILDER METHODS TO IMPLEMENT:
//   - makeSeq(items...) -> IRPtr
//   - makeCall(callee, args, result) -> IRPtr
//   - makeBindVar(name, value) -> IRPtr
//   - makeIf(cond, then_ir, else_ir, result) -> IRPtr
//   - makeLoop(kind, body, result) -> IRPtr
//   - makeMatch(scrutinee, arms, result) -> IRPtr
//   - makeReturn(value) -> IRPtr
//   - makeBlock(setup, body, value) -> IRPtr
//   - makeRegion(owner, body, value) -> IRPtr
//   - makeParallel(domain, body, result) -> IRPtr (§19.1)
//   - makeSpawn(body, result) -> IRPtr (§19.1.1)
//   - makeDispatch(range, body, result) -> IRPtr (§19.1.3)
// =============================================================================

#include "cursive/05_codegen/ir/ir_builder.h"

// TODO: Implement IR builder API
