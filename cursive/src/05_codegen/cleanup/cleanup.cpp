// =============================================================================
// MIGRATION MAPPING: cleanup.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   - Section 6.4 Expression Lowering - Drop and Cleanup (lines 15831-15992)
//   - ExecIR-Drop rule (lines 15837-15840)
//   - ExecIR-DropFields rule (lines 15842-15845)
//   - ExecIR-DropGlue rule (lines 15847-15850)
//   - Cleanup-* rules for control flow (lines 15880-15992)
//   - CleanupPlan computation (lines 15880-15900)
//   - EmitCleanup judgment (lines 15901-15920)
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/cleanup.cpp
//   - Lines 1-200: CleanupPlan struct and computation
//   - Lines 201-400: EmitCleanup implementation
//   - Lines 401-600: EmitDrop for various types
//   - Lines 601-800: EmitDropFields for records
//   - Lines 801-1000: DropGlueSym generation
//   - Lines 1001-1200: DropOnAssign handling
//   - Lines 1201-1400: Binding validity tracking
//   - Lines 1401-1599: Control flow cleanup integration
//
// DEPENDENCIES:
//   - cursive/include/05_codegen/cleanup/cleanup.h
//   - cursive/include/05_codegen/ir/ir_model.h (IRDrop, IRDropFields)
//   - cursive/include/05_codegen/layout/layout.h (SizeOf, AlignOf)
//   - cursive/include/04_analysis/types/types.h (TypeRef, DropType predicate)
//   - cursive/include/05_codegen/symbols/mangle.h (DropGlueSym)
//
// REFACTORING NOTES:
//   1. CleanupPlan tracks which bindings need cleanup at scope exit
//   2. Cleanup is deterministic and follows reverse declaration order
//   3. Drop::drop method called for types with Drop
//   4. Bitcopy types have no cleanup required
//   5. Records drop fields in reverse declaration order
//   6. Enums/unions drop active variant only
//   7. Modal types drop current state's fields
//   8. Arrays drop elements in reverse index order
//   9. Control flow (return, break, continue) triggers cleanup
//   10. Panic paths also execute cleanup (unwind)
//
// CLEANUP COMPUTATION:
//   CleanupPlan = {
//     bindings: Vec<BindingCleanup>,  // In reverse declaration order
//     scope_depth: u32
//   }
//
//   BindingCleanup = {
//     name: string,
//     type: TypeRef,
//     state: BindingState,  // Alive, Moved, PartiallyMoved, Poisoned
//     needs_drop: bool
//   }
//
// DROP EXECUTION ORDER:
//   1. Check binding state (skip if Moved)
//   2. If PartiallyMoved, drop only non-moved fields
//   3. Call Drop::drop if type has Drop
//   4. Recursively drop fields/elements
//   5. Deallocate storage (if heap)
//
// EMITCLEANUP JUDGMENT:
//   EmitCleanup(plan, label) |- IRSeq
//   - Generates IR sequence for cleanup at label
//   - Handles both normal and panic paths
// =============================================================================

#include "cursive/05_codegen/cleanup/cleanup.h"

// TODO: Migrate implementation from cursive-bootstrap/src/04_codegen/cleanup.cpp

