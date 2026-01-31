// =============================================================================
// MIGRATION MAPPING: classes.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
// - Section 5.3.1 "Classes (Cursive0)" (lines 11762-12155)
//   - Common Method-Signature Definitions (lines 11764-11811)
//     - ReturnType (lines 11769-11771)
//     - SelfVar (line 11773)
//     - SubstSelf (lines 11775-11789)
//     - RecvType (lines 11791-11794)
//     - RecvMode (lines 11796-11797)
//     - PermOf (lines 11799-11800)
//     - RecvPerm (line 11802)
//     - ParamSig_T (line 11804)
//     - ParamBinds_T (line 11805)
//     - ReturnType_T (line 11806)
//     - Sig_T (line 11808)
//     - SigSelf (line 11810)
//     - SigMatch (line 11812)
//   - Class Declarations (lines 11816-11820)
//     - ClassItems, ClassMethods, ClassFields (lines 11816-11818)
//     - MethodNames, FieldNames (lines 11819-11820)
//   - WF-ClassPath (lines 11824-11832)
//   - Superclass Linearization C3 (lines 11834-11876)
//   - Effective Method Set (lines 11883-11900)
//   - Effective Field Set (lines 11902-11919)
//   - Dispatchability (lines 11921-11943)
//     - SelfOccurs (lines 11923-11939)
//     - vtable_eligible (line 11941)
//     - dispatchable (line 11943)
//   - Class Method Well-Formedness (lines 11945-11988)
//   - Class Implementation (lines 11989-12091)
//   - Dynamic Class Types (lines 12092-12102)
//   - Method Lookup (lines 12104-12154)
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/composite/classes.cpp
// - Lines 1-899 (entire file)
//
// Key source functions to migrate:
// - ClassMethodTable (lines 538-579): Build effective method table
// - ClassFieldTable (lines 581-626): Build effective field table
// - LookupClassMethod (lines 628-642): Look up class method by name
// - ClassDispatchable (lines 644-656): Check if class is dispatchable
// - ClassSubtypes (lines 658-675): Check class subtype relationship
// - TypeImplementsClass (lines 677-726): Check if type implements class
// - ClassAssociatedTypes (lines 730-739): C0X extension for associated types
// - ClassAbstractStates (lines 743-752): C0X extension for modal classes
// - IsModalClass (lines 755-758): C0X extension check
// - VTableEligible (lines 765-797): Check vtable eligibility
// - Dispatchable (lines 803-816): Check class dispatchability
// - CheckImplCompleteness (lines 821-870): Check implementation completeness
// - CheckOrphanRule (lines 874-896): C0X orphan rule check
//
// Supporting helpers:
// - LookupClassDecl (lines 46-53): Look up class declaration
// - ClassMethods (lines 55-64): Extract methods from class
// - ClassFields (lines 66-75): Extract fields from class
// - TypeLowerResult struct (lines 77-81): Type lowering result
// - LowerType (lines 157-294): Full type lowering
// - LowerReturnType (lines 296-303): Lower return type with unit default
// - SubstSelfType (lines 305-373): Substitute Self in types
// - StripPerm (lines 375-387): Strip permission recursively
// - LowerReceiverPerm (lines 390-400): Convert receiver permission
// - MethodSig struct (lines 402-407): Method signature
// - MethodSigSelf (lines 409-442): Compute method signature with Self
// - SigEqual (lines 444-469): Check signature equality
// - SelfOccurs (type) (lines 471-518): Check if Self appears in type
// - SelfOccurs (method) (lines 520-530): Check if Self appears in method
// - VtableEligible (internal) (lines 532-534): Internal vtable check
//
// DEPENDENCIES:
// - cursive/src/04_analysis/composite/class_linearization.h (LinearizeClass)
// - cursive/src/04_analysis/resolve/scopes.h (ScopeContext)
// - cursive/src/04_analysis/types/type_equiv.h (TypeEquiv)
// - cursive/src/00_core/assert_spec.h (SPEC_DEF, SPEC_RULE)
//
// REFACTORING NOTES:
// 1. LowerType is duplicated across multiple files - must consolidate
// 2. SubstSelfType handles Self replacement throughout type structure
// 3. C3 linearization is in separate file (class_linearization.cpp)
// 4. Effective method/field sets use FirstByName with signature conflict detection
// 5. dispatchable requires all methods to be vtable_eligible
// 6. vtable_eligible checks that Self does not appear by value in signature
// 7. C0X extensions for associated types, abstract states, orphan rule
// =============================================================================

