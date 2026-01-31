// =============================================================================
// MIGRATION MAPPING: unions.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
// - Section 5.2.7 "Union Types" (lines 9056-9070)
//   - Members (line 9058)
//   - DistinctMembers (line 9059)
//   - SetMembers (line 9060)
//   - T-Union-Intro (lines 9062-9065)
//   - Union-DirectAccess-Err (lines 9067-9070)
// - Section 5.2.2 "Subtyping" (lines 8594-8696)
//   - Member (line 8684)
//   - Sub-Member-Union (lines 8686-8689)
//   - Sub-Union-Width (lines 8691-8694)
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/composite/unions.cpp
// - Lines 1-135 (entire file)
//
// Key source functions to migrate:
// - TypeUnionIntro (lines 54-108): Union introduction rule
// - CheckUnionDirectAccess (lines 110-132): Direct access error check
//
// Supporting helpers:
// - UnionMemberResult struct (lines 21-25): Result type for membership check
// - StripPerm (lines 27-35): Strip permission layer
// - UnionMember (lines 37-50): Check if type is a union member
//
// DEPENDENCIES:
// - cursive/src/04_analysis/types/type_equiv.h (TypeEquiv)
// - cursive/src/00_core/assert_spec.h (SPEC_DEF, SPEC_RULE)
//
// REFACTORING NOTES:
// 1. Union types are unordered: A|B is equivalent to B|A (spec line 8512 MembersEq)
// 2. The UnionMember check uses TypeEquiv for each member comparison
// 3. StripPerm is duplicated; consolidate with other files
// 4. Union-DirectAccess-Err prevents field access on union types directly
// 5. Consider caching union member equivalence checks for performance
// 6. The union introduction rule checks permission compatibility when both
//    the value and target have permissions
// =============================================================================

