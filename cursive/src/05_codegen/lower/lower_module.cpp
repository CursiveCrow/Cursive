// =============================================================================
// MIGRATION MAPPING: lower_module.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Section 6 (Code Generation)
//   - Lines 14196-14349: Codegen Model and Judgments
//   - Lines 15394-15665: Symbols, Mangling, and Linkage (for symbol generation)
//   - Lines 17161-17202: Dynamic Dispatch (for vtable emission)
//   - Module lowering orchestrates item-level code generation
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/lower/lower_module.cpp
//   - Lines 1-20: Includes and namespace
//   - Lines 21-50: Helper functions (IsUnitType, BlockEndsWithReturn, BuildScope)
//   - Lines 52-128: Type lowering helpers (LowerTypeForMethod, SubstSelfType, LowerReturnType)
//   - Lines 130-152: Parameter lowering (LowerParam, HasDynamicAttr)
//   - Lines 154-207: LowerProcLike - core procedure lowering logic
//   - Lines 209-256: SortedImplementations, DefaultUserList - class method helpers
//   - Lines 258-339: Method lowering (LowerRecordMethod, LowerStateMethod, LowerTransition)
//   - Lines 341-382: LowerClassMethodBody - class default implementations
//   - Lines 386-549: LowerModule - main module lowering entry point
//
// DEPENDENCIES:
//   - cursive/src/05_codegen/lower/lower_proc.h (LowerProc)
//   - cursive/src/05_codegen/lower/lower_stmt.h (LowerBlock, LowerStmtList)
//   - cursive/src/05_codegen/abi/abi.h (NeedsPanicOut, PanicOutType)
//   - cursive/src/05_codegen/cleanup.h (ComputeCleanupPlanForCurrentScope, EmitCleanup)
//   - cursive/src/05_codegen/dyn_dispatch.h (EmitVTable)
//   - cursive/src/05_codegen/globals.h (EmitGlobal, EmitModuleInitFn, EmitModuleDeinitFn)
//   - cursive/src/05_codegen/mangle.h (MangleProc, MangleMethod, MangleStateMethod, etc.)
//   - cursive/src/05_codegen/layout/layout.h (LowerTypeForLayout)
//   - cursive/src/04_analysis/types/types.h (TypeRef, MakeTypePrim, etc.)
//   - cursive/src/04_analysis/composite/classes.h (TypeImplementsClass)
//   - cursive/src/04_analysis/composite/record_methods.h (RecvTypeForReceiver, RecvModeOf)
//
// REFACTORING NOTES:
//   1. Module lowering is the top-level entry point for code generation
//   2. Key function LowerModule iterates over all module items and dispatches to:
//      - LowerProc for procedures
//      - LowerRecordMethod for record methods
//      - LowerStateMethod for modal state methods
//      - LowerTransition for modal transitions
//      - LowerClassMethodBody for class default implementations
//      - EmitVTable for class vtables
//      - EmitGlobal for static declarations
//   3. Self-type substitution (SubstSelfType) handles replacing 'Self' in method signatures
//   4. Module init/deinit functions handle static initialization order
//   5. Consider splitting method lowering into separate files for maintainability
//
// SPEC RULES IMPLEMENTED:
//   - Lower-Module: Main module lowering judgment
//   - CG-Item-Procedure, CG-Item-Procedure-Main: Procedure items
//   - CG-Item-Record, CG-Item-Method: Record and method items
//   - CG-Item-Modal, CG-Item-StateMethod, CG-Item-Transition: Modal items
//   - CG-Item-Class, CG-Item-ClassMethod-Abstract, CG-Item-ClassMethod-Body: Class items
//   - CG-Item-Static: Static declarations
//   - CG-Item-ExternBlock, CG-Item-ExternProc: FFI declarations
//   - CG-Item-Using, CG-Item-TypeAlias, CG-Item-Import: Non-codegen items
//
// =============================================================================

#include "cursive/05_codegen/lower/lower_module.h"

namespace cursive::codegen {

// TODO: Migrate implementation from cursive-bootstrap

}  // namespace cursive::codegen
