// =============================================================================
// MIGRATION MAPPING: dyn_dispatch.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   - Section 6.10 Dynamic Dispatch (lines 17163-17200)
//   - VTableEligible(Cl) definition (line 17165)
//   - vtable_eligible(m) predicate (line 11941)
//   - SelfOccurs predicate (§5.4.1)
//   - dispatchable(Cl) predicate (line 11943)
//   - VTable-Order rule (lines 17182-17185)
//   - VSlot-Entry rule (lines 17188-17190)
//   - DynPack rule (lines 17193-17195)
//   - LowerDynCall rule (lines 17198-17200)
//   - DispatchSym-Impl, DispatchSym-Default-None rules
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/dyn_dispatch.cpp
//   - Lines 1-100: SelfOccurs implementation
//   - Lines 104-120: SelfOccursInMethod
//   - Lines 122-166: IsVTableEligible, VTableEligible
//   - Lines 168-209: DispatchSym implementation
//   - Lines 211-251: VTable generation
//   - Lines 253-270: EmitVTable
//   - Lines 272-294: VSlot lookup
//   - Lines 296+: DynPack (fat pointer creation)
//
// DEPENDENCIES:
//   - cursive/include/05_codegen/dyn_dispatch/dyn_dispatch.h
//   - cursive/include/05_codegen/ir/ir_model.h (GlobalVTable, CallVTable)
//   - cursive/include/05_codegen/symbols/mangle.h (MangleVTable, MangleMethod)
//   - cursive/include/05_codegen/layout/layout.h (SizeOf, AlignOf)
//   - cursive/include/05_codegen/cleanup/cleanup.h (DropGlueSym)
//   - cursive/include/04_analysis/composite/classes.h
//   - cursive/include/04_analysis/composite/record_methods.h
//
// REFACTORING NOTES:
//   1. VTable eligibility: HasReceiver(m) AND NOT SelfOccurs(m)
//   2. SelfOccurs checks if Self type appears in params/return
//   3. VTableEligible returns ordered list of eligible methods
//   4. VSlot returns 0-based index into vtable slots
//   5. DynPack creates fat pointer: (data_ptr, vtable_ptr)
//   6. DispatchSym resolves actual implementation symbol
//
// VTABLE LAYOUT:
//   VTable = {
//     size: usize,      // SizeOf(T)
//     align: usize,     // AlignOf(T)
//     drop: *ptr,       // DropGlueSym(T)
//     slots: [*ptr]     // Method pointers in VTableEligible order
//   }
//
// DYNAMIC CALL LOWERING:
//   1. Extract vtable pointer from fat pointer
//   2. Load slot pointer at VSlot(Cl, method) index
//   3. Call slot pointer with (data_ptr, args...)
//   4. Check panic out-parameter
//
// SELFOCCURS RECURSION:
//   - TypePath(["Self"]) -> true
//   - TypePerm(p, ty) -> SelfOccurs(ty)
//   - TypeTuple/Union -> any element
//   - TypeArray/Slice/Ptr/RawPtr -> element type
//   - TypeFunc -> params or return
//   - Primitives -> false
// =============================================================================

#include "cursive/05_codegen/dyn_dispatch/dyn_dispatch.h"

// TODO: Migrate implementation from cursive-bootstrap/src/04_codegen/dyn_dispatch.cpp

