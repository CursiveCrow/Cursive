// =============================================================================
// MIGRATION MAPPING: class_linearization.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
// - Section 5.3.1 "Classes (Cursive0)" - Superclass Linearization (C3) (lines 11834-11882)
//   - Supers(Cl) (line 11836)
//   - Lin-Base (lines 11838-11841)
//   - Head, Tail (lines 11843-11845)
//   - HeadOk (line 11846)
//   - SelectHead (lines 11847-11848)
//   - PopHead (lines 11849-11850)
//   - PopAll (line 11851)
//   - Merge-Empty (lines 11853-11856)
//   - Merge-Step (lines 11858-11861)
//   - Merge-Fail (lines 11863-11866)
//   - Lin-Ok (lines 11868-11871)
//   - Lin-Fail (lines 11873-11876)
//   - Superclass-Cycle (lines 11878-11881)
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/composite/class_linearization.cpp
// - Lines 1-190 (entire file)
//
// Key source functions to migrate:
// - LinearizeClass (lines 182-187): Main entry point for C3 linearization
//
// Supporting helpers:
// - LookupClassDecl (lines 27-34): Look up class declaration
// - ClassList type alias (line 36): Vector of class paths
// - ClassLists type alias (line 37): Vector of class lists
// - HeadOk (lines 39-51): Check if head is ok (not in tail of any list)
// - SelectHead (lines 53-64): Select first ok head from lists
// - PopAll (lines 66-79): Remove head from all lists
// - AllEmpty (lines 81-88): Check if all lists are empty
// - Merge (lines 90-110): C3 merge algorithm
// - LinearizeState struct (lines 112-115): Memoization state
// - LinearizeImpl (lines 117-178): Recursive linearization implementation
//
// DEPENDENCIES:
// - cursive/src/04_analysis/resolve/scopes.h (ScopeContext, PathKeyOf, PathEq)
// - cursive/src/00_core/assert_spec.h (SPEC_DEF, SPEC_RULE)
//
// Diagnostic rules implemented:
// - Merge-Empty (line 97): All lists empty - successful merge
// - Merge-Step (line 106): Found valid head - continue merge
// - Merge-Fail (line 103): No valid head found - linearization fails
// - Lin-Base (line 140): Base case - class with no supers
// - Lin-Fail (line 163): Merge failed
// - Lin-Ok (line 169): Linearization succeeded
// - Superclass-Undefined (line 133): Referenced superclass not found
//
// REFACTORING NOTES:
// 1. C3 linearization (MRO - Method Resolution Order) from Python
// 2. Memoization prevents recomputation for diamond inheritance
// 3. Active set prevents infinite recursion on cycles
// 4. Merge algorithm selects heads that don't appear in tails
// 5. Result includes the class itself as first element
// 6. Cycle detection via active set returns empty result
// =============================================================================

