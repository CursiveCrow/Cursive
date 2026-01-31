// =============================================================================
// MIGRATION MAPPING: init.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   - Section 6.0 CG-Module rule (lines 14228-14231)
//   - InitFn(m), DeinitFn(m) (line 14229)
//   - Lower-StaticInit, Lower-StaticDeinit (line 14229)
//   - Module initialization ordering
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/init.cpp
//   - Module init/deinit function generation
//   - Static initialization code emission
//
// DEPENDENCIES:
//   - cursive/include/05_codegen/globals/init.h
//   - cursive/include/05_codegen/ir/ir_model.h (ProcIR)
//   - cursive/include/05_codegen/symbols/mangle.h (module symbols)
//
// REFACTORING NOTES:
//   1. Each module has init and deinit functions
//   2. InitFn handles:
//      - Static variable initialization
//      - Module-level setup
//   3. DeinitFn handles:
//      - Static variable cleanup (Drop)
//      - Module-level teardown
//   4. Functions take PanicOutParam for error handling
//   5. Called by runtime in dependency order
//   6. EmitInitPlan generates full program init sequence
//
// MODULE LIFECYCLE:
//   1. Runtime loads all modules
//   2. Calls InitFn for each in dependency order
//   3. ... program execution ...
//   4. Calls DeinitFn in reverse order
//
// INIT FUNCTION STRUCTURE:
//   proc __module_init(__panic: *mut PanicRecord) -> () {
//     // Initialize statics in declaration order
//     // Call dependency module inits
//   }
// =============================================================================

#include "cursive/05_codegen/globals/init.h"

// TODO: Migrate implementation from cursive-bootstrap/src/04_codegen/init.cpp
