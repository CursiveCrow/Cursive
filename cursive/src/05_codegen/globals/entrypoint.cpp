// =============================================================================
// MIGRATION MAPPING: entrypoint.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   - Section 6.0 CG-Item-Procedure-Main rule (lines 14262-14265)
//   - MainSigOk predicate (line 14263)
//   - EmitInitPlan(P) for program initialization (line 14263)
//   - EntrySym linkage (lines 15660-15663)
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/entrypoint.cpp
//   - Entry point wrapper generation
//   - Context initialization
//   - Main procedure invocation
//
// DEPENDENCIES:
//   - cursive/include/05_codegen/globals/entrypoint.h
//   - cursive/include/05_codegen/ir/ir_model.h (ProcIR)
//   - cursive/include/05_codegen/symbols/linkage.h (LinkageOfEntrySym)
//
// REFACTORING NOTES:
//   1. Entry point wraps user's main procedure
//   2. MainSigOk: main(move ctx: Context) -> i32
//   3. Entry performs:
//      - Runtime initialization
//      - Context creation
//      - Module init calls (EmitInitPlan)
//      - main() invocation
//      - Module deinit calls
//      - Exit code return
//   4. EntrySym has External linkage
//   5. Panic handling at entry level
//
// ENTRY POINT STRUCTURE:
//   proc main() -> i32 {  // C-compatible entry
//     // Runtime init
//     let ctx = Context::new(...)
//     // Module init plan
//     EmitInitPlan(...)
//     // Call user main
//     let result = user_main(move ctx)
//     // Module deinit
//     // Return exit code
//     return result
//   }
//
// MAIN SIGNATURE CHECK:
//   - Name must be "main"
//   - Single parameter: move ctx: Context
//   - Return type: i32
//   - Must be in executable project
// =============================================================================

#include "cursive/05_codegen/globals/entrypoint.h"

// TODO: Migrate implementation from cursive-bootstrap/src/04_codegen/entrypoint.cpp
