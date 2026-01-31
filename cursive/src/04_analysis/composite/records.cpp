// =============================================================================
// MIGRATION MAPPING: records.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
// - Section 5.2.8 "Default Record Construction" (lines 9072-9089)
//   - Fields (line 9074)
//   - InitOk (line 9076)
//   - WF-Record (lines 9078-9081)
//   - WF-Record-DupField (lines 9083-9086)
//   - DefaultConstructible (line 9088)
//   - BuiltinRecord (line 9089)
// - Section 5.3.2 "Record Methods" (lines 12156-12259)
//   - Fields(R) definition (line 12160)
//   - Methods(R) definition (line 12161)
//   - Self_R (line 12162)
//   - SelfType (line 12163)
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/composite/records.cpp
// - Lines 1-417 (entire file)
//
// Key source functions to migrate:
// - CheckRecordWf (lines 337-384): Record well-formedness check
// - TypeRecordDefaultCall (lines 386-414): Default record constructor call typing
//
// Supporting helpers:
// - TypeLowerResult struct (lines 19-23): Type lowering result
// - LowerPermission (lines 33-43): Convert syntax permission to analysis permission
// - LowerParamMode (lines 45-51): Convert syntax param mode to analysis mode
// - LowerRawPtrQual (lines 53-61): Convert syntax raw pointer qualifier
// - LowerStringState (lines 63-75): Convert syntax string state
// - LowerBytesState (lines 77-89): Convert syntax bytes state
// - LowerPtrState (lines 91-105): Convert syntax pointer state
// - LowerType (lines 107-245): Full type lowering from syntax to analysis
// - RecordFields (lines 247-256): Extract fields from record declaration
// - DefaultConstructible (lines 258-266): Check if all fields have initializers
// - FullPath (lines 268-273): Build full path from module path and name
// - LookupRecordDecl (lines 275-282): Look up record declaration by path
// - RecordCalleeResult struct (lines 284-287): Callee resolution result
// - ResolveRecordCallee (lines 289-333): Resolve call callee as record type
//
// DEPENDENCIES:
// - cursive/src/04_analysis/resolve/scopes.h (ScopeContext)
// - cursive/src/04_analysis/resolve/scopes_lookup.h (ResolveTypeName)
// - cursive/src/04_analysis/types/subtyping.h (Subtyping)
// - cursive/src/04_analysis/types/type_equiv.h (TypeEquiv)
// - cursive/src/00_core/assert_spec.h (SPEC_DEF, SPEC_RULE)
//
// REFACTORING NOTES:
// 1. LowerType is a large visitor that is duplicated in multiple files
//    (classes.cpp, function_types.cpp) - must consolidate
// 2. LowerPermission/LowerParamMode/etc are also duplicated
// 3. BuiltinRecord set includes: RegionOptions, DirEntry, Context, System
// 4. DefaultConstructible requires all fields to have initializers
// 5. Record well-formedness checks for duplicate field names
// 6. Type lowering handles generic type arguments per WF-Apply (5.2.3)
// =============================================================================

