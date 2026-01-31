// =============================================================================
// MIGRATION MAPPING: binding_storage.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   - Section 6.4 Expression Lowering (lines 15665-15992)
//   - IRBindVar, IRStoreVar, IRStoreVarNoDrop (IR nodes for binding)
//   - ExecIR-BindVar rule (lines 15831-15833)
//   - ExecIR-StoreVar rule (lines 15821-15823)
//   - ExecIR-StoreVarNoDrop rule (lines 15825-15828)
//   - Binding state tracking (Alive, Moved, PartiallyMoved, Poisoned)
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/binding_storage.cpp
//   - Local variable allocation
//   - Binding state management
//   - Stack frame layout
//
// DEPENDENCIES:
//   - cursive/include/05_codegen/globals/binding_storage.h
//   - cursive/include/05_codegen/layout/layout.h (SizeOf, AlignOf)
//   - cursive/include/04_analysis/types/types.h (TypeRef)
//
// REFACTORING NOTES:
//   1. Bindings track local variables within a procedure
//   2. Each binding has:
//      - Name (identifier)
//      - Type
//      - Storage location (stack slot)
//      - State (Alive, Moved, etc.)
//      - Mutability (let vs var)
//      - Movability (= vs :=)
//   3. BindVar: create new binding with initial value
//   4. StoreVar: update existing binding (with drop of old value)
//   5. StoreVarNoDrop: update without dropping old (for subfields)
//   6. Stack layout computed for procedure prologue
//
// BINDING OPERATIONS:
//   - allocate(name, type) -> storage location
//   - bind(name, value) -> void
//   - store(name, value) -> void (drops old)
//   - load(name) -> value
//   - move(name) -> value (marks as Moved)
//   - drop(name) -> void (cleanup)
//
// STATE TRANSITIONS:
//   Alive -> Moved (after move)
//   Alive -> PartiallyMoved (after field move)
//   * -> Poisoned (on error)
// =============================================================================

#include "cursive/05_codegen/globals/binding_storage.h"

// TODO: Migrate implementation from cursive-bootstrap/src/04_codegen/binding_storage.cpp
