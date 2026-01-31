// =============================================================================
// MIGRATION MAPPING: unwind.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   - Section 6.4 Expression Lowering - Panic and Unwind (lines 15950-15992)
//   - Panic out-parameter handling (lines 15950-15965)
//   - Unwind path generation (lines 15966-15980)
//   - Cleanup on panic (lines 15981-15992)
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/cleanup.cpp
//   - Lines 1401-1500: Unwind path generation
//   - Lines 1501-1599: Panic cleanup integration
//
// ADDITIONAL SOURCE: cursive-bootstrap/src/04_codegen/abi/abi_params.cpp
//   - Lines 200-284: PanicRecordType, NeedsPanicOut, PanicOutParams
//
// DEPENDENCIES:
//   - cursive/include/05_codegen/cleanup/unwind.h
//   - cursive/include/05_codegen/ir/ir_model.h (IRBranch, IRLabel)
//   - cursive/include/05_codegen/abi/abi_params.h (PanicOutParam)
//   - cursive/include/05_codegen/cleanup/cleanup.h (CleanupPlan)
//
// REFACTORING NOTES:
//   1. Cursive uses out-parameter panic handling, not exceptions
//   2. Procedures that can panic have hidden PanicRecord* parameter
//   3. On panic, set panic record and jump to cleanup
//   4. Cleanup runs in reverse scope order
//   5. After cleanup, propagate panic to caller
//   6. Unwind paths share cleanup code with normal paths where possible
//
// PANIC RECORD STRUCTURE:
//   PanicRecord = {
//     panicked: bool,
//     message: *const u8,
//     message_len: usize,
//     file: *const u8,
//     file_len: usize,
//     line: u32,
//     column: u32
//   }
//
// UNWIND PATH GENERATION:
//   1. At each call site that can panic:
//      - Check panic flag after call
//      - If set, branch to unwind label
//   2. Unwind label:
//      - Execute cleanup for current scope
//      - Jump to next outer unwind label or return
//   3. Entry point catches outermost panic
//
// CLEANUP-ON-PANIC RULES:
//   - Same cleanup as normal scope exit
//   - Must handle partially-constructed values
//   - Must not panic during cleanup (abort if so)
// =============================================================================

#include "cursive/05_codegen/cleanup/unwind.h"

// TODO: Migrate implementation from cursive-bootstrap/src/04_codegen/cleanup.cpp

