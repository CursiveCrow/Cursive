// =============================================================================
// MIGRATION MAPPING: function_types.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
// - Section 5.2.3 "Function Types" (lines 8698-8705)
//   - T-Proc-As-Value (lines 8700-8703)
//   - FuncTypeRules (line 8705)
// - Section 5.2.4 "Procedure Calls" (lines 8707-8776)
//   - ArgsOkTJudg (line 8709)
//   - ParamMode, ParamType (lines 8710-8711)
//   - ArgMoved, ArgExpr (lines 8712-8713)
//   - PlaceType, ArgType (lines 8714-8717)
//   - ArgsT-Empty, ArgsT-Cons, ArgsT-Cons-Ref (lines 8719-8731)
//   - T-Call (lines 8733-8736)
//   - Call error rules (lines 8738-8776)
// - Section 5.2.12 referenced for ValuePathType
// - Section 5.2.14 referenced for ProcReturn
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/composite/function_types.cpp
// - Lines 1-387 (entire file)
//
// Key source functions to migrate:
// - ProcType (lines 321-339): Compute procedure type as function value
// - ValuePathType (lines 341-384): Resolve qualified path to value type
//
// Supporting helpers:
// - TypeLowerResult struct (lines 22-26): Type lowering result
// - LowerPermission (lines 33-43): Convert syntax permission
// - LowerParamMode (lines 45-51): Convert syntax param mode
// - LowerRawPtrQual (lines 53-61): Convert syntax raw pointer qualifier
// - LowerStringState (lines 63-75): Convert syntax string state
// - LowerBytesState (lines 77-89): Convert syntax bytes state
// - LowerPtrState (lines 91-105): Convert syntax pointer state
// - LowerType (lines 107-245): Full type lowering
// - ProcReturn (lines 247-254): Lower procedure return type
// - FindModule (lines 256-264): Find module by path
// - ModuleNamesForContext (lines 266-276): Get module names from context
// - StaticTypeOf (lines 278-302): Get type of static declaration
// - FindProcedure (lines 304-317): Find procedure in module
//
// DEPENDENCIES:
// - cursive/src/04_analysis/resolve/collect_toplevel.h (CollectNameMaps)
// - cursive/src/04_analysis/resolve/scopes.h (ScopeContext)
// - cursive/src/04_analysis/resolve/scopes_lookup.h (ResolveQualified)
// - cursive/src/04_analysis/resolve/visibility.h (CanAccess)
// - cursive/src/04_analysis/memory/string_bytes.h (LookupStringBytesBuiltinType)
// - cursive/src/04_analysis/caps/cap_concurrency.h (MakeCancelTokenTypeWithState)
// - cursive/src/04_analysis/types/type_equiv.h (TypeEquiv)
// - cursive/src/00_core/symbols.h (symbol utilities)
// - cursive/src/00_core/assert_spec.h (SPEC_DEF, SPEC_RULE)
//
// REFACTORING NOTES:
// 1. LowerType is duplicated across multiple files - must consolidate
// 2. ProcReturn defaults to unit "()" when no return type specified
// 3. ValuePathType handles special cases:
//    - String/bytes builtin methods
//    - CancelToken::new constructor
//    - Static declarations
//    - Procedure declarations
// 4. Qualified name resolution respects visibility
// 5. T-Proc-As-Value: procedure names can be used as function values
// =============================================================================

