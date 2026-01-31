/*
 * =============================================================================
 * MIGRATION MAPPING: conformance.cpp
 * =============================================================================
 *
 * SPEC REFERENCE:
 *   - C0updated.md, Section 1.1 "Conformance" (line 230)
 *   - C0updated.md, Section 8.5 "E-CNF Errors" (line 21534)
 *   - C0updated.md, Section 8.6 "W-CNF Warnings" (line 21545)
 *   - C0updated.md, Section 6.2 "Type Equivalence" (lines 15800-15900)
 *   - C0updated.md, Section 6.3 "Subtyping" (lines 15950-16100)
 *
 * SOURCE FILE:
 *   - cursive-bootstrap/src/03_analysis/types/conformance.cpp (lines 1-365)
 *
 * FUNCTIONS TO MIGRATE:
 *   - ConformsTo(TypeRef a, TypeRef b) -> bool                    [lines 45-120]
 *       Core type conformance check; handles primitives, records, enums,
 *       unions, tuples, arrays, slices, functions, modals, pointers
 *   - TypeEquals(TypeRef a, TypeRef b) -> bool                    [lines 125-180]
 *       Structural type equality check
 *   - UnionConformsTo(TypeRef union_ty, TypeRef target) -> bool   [lines 185-220]
 *       Union type conformance (unordered: A|B == B|A)
 *   - PermissionConformsTo(Permission a, Permission b) -> bool    [lines 225-250]
 *       Permission lattice: unique <: shared <: const
 *   - ModalStateConformsTo(TypeRef a, TypeRef b) -> bool          [lines 255-290]
 *       Modal type with specific state conformance
 *   - FunctionConformsTo(TypeRef a, TypeRef b) -> bool            [lines 295-340]
 *       Function type conformance (contravariant params, covariant return)
 *   - ArrayConformsTo(TypeRef a, TypeRef b) -> bool               [lines 345-365]
 *       Array conformance (element type + length must match)
 *
 * DEPENDENCIES:
 *   - TypeRef, TypeKind, Permission enums from type system
 *   - ModalDecl, ModalState from modal type declarations
 *   - DiagnosticEmitter for E-CNF-* and W-CNF-* diagnostics
 *   - UnionType::Normalize() for union canonicalization
 *
 * REFACTORING NOTES:
 *   1. Union types are UNORDERED per spec - ensure A|B == B|A normalization
 *   2. No numeric subtyping: i32 is NOT a subtype of i64 (requires explicit cast)
 *   3. Permission subtyping follows lattice: unique <: shared <: const
 *   4. Modal state types require exact state match for equivalence
 *   5. Consider caching conformance results for repeated type pairs
 *   6. Split large ConformsTo switch into per-kind helper functions
 *
 * DIAGNOSTIC CODES:
 *   - E-CNF-0001: Type mismatch
 *   - E-CNF-0002: Permission violation
 *   - E-CNF-0003: Modal state mismatch
 *   - W-CNF-0001: Implicit widening
 *
 * =============================================================================
 */
