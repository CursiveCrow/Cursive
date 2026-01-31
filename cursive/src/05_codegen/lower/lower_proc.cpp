// =============================================================================
// MIGRATION MAPPING: lower_proc.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Section 6 (Code Generation)
//   - Lines 16586-16756: Statement and Block Lowering (for block handling)
//   - Lines 17028-17160: Cleanup, Drop, and Unwinding
//   - Procedure lowering handles function body generation and async procedures
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/lower/lower_proc.cpp
//   - Lines 1-18: Includes and namespace
//   - Lines 22-66: Helper functions (IsUnitType, IsAsyncProc, BlockEndsWithReturn, etc.)
//   - Lines 67-148: AsyncIRInfo struct and CollectAsyncIR - async state machine analysis
//   - Lines 150-186: Contract check emission (EmitPreconditionCheck, EmitPostconditionCheck)
//   - Lines 190-430: LowerProc - main procedure lowering function
//   - Lines 432-434: AnchorProcRules
//
// DEPENDENCIES:
//   - cursive/src/05_codegen/lower/lower_expr.h (LowerExpr)
//   - cursive/src/05_codegen/lower/lower_stmt.h (LowerBlock)
//   - cursive/src/05_codegen/abi/abi.h (NeedsPanicOut, PanicOutType, kPanicOutName)
//   - cursive/src/05_codegen/cleanup.h (ComputeCleanupPlanForCurrentScope, EmitCleanup)
//   - cursive/src/05_codegen/checks.h (LowerPanic, PanicReason)
//   - cursive/src/05_codegen/globals.h (EmitInitPlan)
//   - cursive/src/05_codegen/async_frame.h (kAsyncFrameHeaderSize, kAsyncFrameHeaderAlign)
//   - cursive/src/05_codegen/mangle.h (MangleProc)
//   - cursive/src/05_codegen/layout/layout.h (LowerTypeForLayout, SizeOf, AlignOf)
//   - cursive/src/04_analysis/types/types.h (TypeRef, IsAsyncType, GetAsyncSig)
//   - cursive/src/04_analysis/memory/regions.h (ComputeExprProvenanceMap)
//
// REFACTORING NOTES:
//   1. LowerProc handles both synchronous and asynchronous procedures
//   2. For async procedures (return type is Async<...>):
//      - CollectAsyncIR gathers yield points and local variable slots
//      - Generates wrapper function that allocates frame and initial state
//      - Generates resume function that handles state machine dispatch
//   3. Contract checking (preconditions/postconditions) is inserted based on [[dynamic]] attr
//   4. Cleanup plan computed for parameters to handle fallthrough returns
//   5. Special handling for 'main' procedure to emit initialization plan
//   6. Consider extracting async procedure lowering into separate file
//
// SPEC RULES IMPLEMENTED:
//   - Lower-Proc: Main procedure lowering judgment
//   - Lower-Proc-ContractPre: Precondition check insertion
//   - Lower-Proc-ContractPost: Postcondition check insertion
//   - Contract-Pre-Check: Runtime precondition check
//   - Contract-Post-Check: Runtime postcondition check
//   - Lower-AsyncReturn: Async procedure return wrapping
//
// ASYNC PROCEDURE STRUCTURE:
//   - Wrapper: Allocates frame, copies params to slots, returns @Suspended
//   - Resume: Dispatches on state, executes body, updates state on yield
//   - Frame layout: [header(state, resume_ptr) | slot_0 | slot_1 | ...]
//
// =============================================================================

#include "cursive/05_codegen/lower/lower_proc.h"

namespace cursive::codegen {

// TODO: Migrate implementation from cursive-bootstrap

}  // namespace cursive::codegen
