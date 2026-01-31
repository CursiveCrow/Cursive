// =============================================================================
// MIGRATION MAPPING: record_methods.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
// - Section 5.3.2 "Record Methods" (lines 12156-12259)
//   - Fields(R), Methods(R), Self_R, SelfType (lines 12160-12163)
//   - Recv-Explicit (lines 12167-12170)
//   - Record-Method-RecvSelf-Err (lines 12172-12175)
//   - Recv-Const (lines 12177-12179)
//   - Recv-Unique (lines 12181-12183)
//   - Recv-Shared (lines 12185-12187)
//   - ParamNames (line 12189)
//   - WF-Record-Method (lines 12191-12194)
//   - T-Record-Method-Body (lines 12196-12199)
//   - T-Record-Method-Builtin (lines 12201-12204)
//   - MethodNames (line 12206)
//   - WF-Record-Methods (lines 12208-12211)
//   - Record-Method-Dup (lines 12213-12216)
//   - LookupMethodRules (line 12220)
//   - ArgsOkJudg (line 12224)
//   - RecvBaseType (line 12226)
//   - Args-Empty (lines 12228-12230)
//   - Args-Cons (lines 12232-12235)
//   - Args-Cons-Ref (lines 12237-12240)
//   - RecvArgOk (line 12242)
//   - T-Record-MethodCall (lines 12245-12248)
// - Section 5.3.1 "Classes" for common method definitions (lines 11764-12155)
//   - RecvType (lines 11791-11794)
//   - RecvMode (lines 11796-11797)
//   - PermOf (lines 11799-11800)
//   - RecvPerm (line 11802)
//   - SubstSelf (lines 11775-11789)
//   - Sig_T (line 11808)
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/composite/record_methods.cpp
// - Lines 1-440 (entire file)
//
// Key source functions to migrate:
// - RecvTypeForReceiver (lines 124-150): Compute receiver type
// - RecvModeOf (lines 152-164): Extract receiver mode
// - RecvBaseType (lines 166-195): Get base type from receiver expression
// - RecvArgOk (lines 197-219): Validate receiver argument
// - ArgsOk (lines 221-343): Validate all arguments
// - LookupMethodStatic (lines 345-437): Static method lookup
//
// Supporting helpers:
// - LowerReceiverPerm (lines 36-46): Convert receiver permission
// - AddrOfOkResult struct (lines 48-51): Address-of validation result
// - AddrOfOk (lines 53-85): Check if address-of is valid
// - MakeExpr (lines 87-92): Construct expression node
// - MovedArgExpr (lines 94-106): Create moved argument expression
// - FindRecordMethod (lines 108-120): Find method in record declaration
//
// DEPENDENCIES:
// - cursive/src/04_analysis/composite/classes.h (ClassMethodTable)
// - cursive/src/04_analysis/resolve/scopes.h (ScopeContext)
// - cursive/src/04_analysis/resolve/scopes_lookup.h (name lookup)
// - cursive/src/04_analysis/types/subtyping.h (Subtyping)
// - cursive/src/04_analysis/types/type_expr.h (type operations)
// - cursive/src/00_core/assert_spec.h (SPEC_DEF, SPEC_RULE)
//
// REFACTORING NOTES:
// 1. Receiver type computation handles both shorthand (~, ~!, ~%) and explicit forms
// 2. RecvArgOk validates that address-of is valid for reference params
// 3. ArgsOk validates argument count, move modifiers, types, and place expressions
// 4. Method lookup first checks record methods, then class default methods
// 5. LookupMethodStatic handles opaque type underlying lookups
// 6. Ambiguous default methods from multiple classes produce an error
// 7. SubstSelf is used to substitute Self type in method signatures
// =============================================================================

