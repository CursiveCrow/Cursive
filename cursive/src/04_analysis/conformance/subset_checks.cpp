/*
 * =============================================================================
 * MIGRATION MAPPING: subset_checks.cpp
 * =============================================================================
 *
 * SPEC REFERENCE:
 *   - C0updated.md, Section 6.3 "Subtyping" (lines 15950-16100)
 *   - C0updated.md, Section 6.4 "Type Coercion" (lines 16150-16300)
 *   - C0updated.md, Section 5.3.4 "Permission Lattice" (lines 11900-12000)
 *
 * SOURCE FILE:
 *   - cursive-bootstrap/src/03_analysis/types/conformance.cpp (lines 1-365)
 *     (subset checking logic is integrated into conformance.cpp)
 *
 * FUNCTIONS TO MIGRATE:
 *   - IsSubsetOf(TypeRef sub, TypeRef super) -> bool              [lines 50-80]
 *       Check if sub is a subtype of super
 *   - CanCoerceTo(TypeRef from, TypeRef to) -> bool               [lines 85-120]
 *       Check if implicit coercion is allowed
 *   - RequiresExplicitCast(TypeRef from, TypeRef to) -> bool      [lines 125-150]
 *       Determine if explicit `as` cast is required
 *   - PermissionSubset(Permission sub, Permission super) -> bool  [lines 155-170]
 *       Permission lattice subset check
 *   - UnionSubset(TypeRef union_sub, TypeRef union_super) -> bool [lines 175-200]
 *       Union type subset (all variants of sub must be in super)
 *
 * DEPENDENCIES:
 *   - TypeRef, Permission from type system
 *   - UnionType for variant enumeration
 *   - Conformance checks from conformance.cpp
 *
 * REFACTORING NOTES:
 *   1. CRITICAL: No implicit numeric coercion (i32 -> i64 requires explicit cast)
 *   2. Permission subset: unique <: shared <: const (not equality)
 *   3. Union subset: every variant of A|B must appear in C|D for subset
 *   4. Safe pointer states: @Valid <: @Null (valid can be used where null expected)
 *   5. Modal widened type is supertype of state-specific types
 *   6. Consider separate functions for each type category
 *
 * DIAGNOSTIC CODES:
 *   - E-CNF-0010: Subset check failed
 *   - E-CNF-0011: Coercion not allowed
 *   - E-CNF-0012: Explicit cast required
 *
 * =============================================================================
 */
