// =============================================================================
// MIGRATION MAPPING: subtyping.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.2.2: Subtyping (lines 8594-8696)
//   - SubtypingJudg = {<:}
//   - PermSubJudg = {PermSub}
//   - Rules: Perm-Const, Perm-Shared, Perm-Unique, Perm-Unique-Shared,
//            Perm-Shared-Const, Perm-Unique-Const
//   - Rules: Sub-Perm, Sub-Never, Sub-Tuple, Sub-Array, Sub-Slice, Sub-Range,
//            Sub-Ptr-State, Sub-Modal-Niche, Sub-Func, Sub-Closure, Sub-Async,
//            Sub-Member-Union, Sub-Union-Width
//   - Member(T, U) predicate defined at line 8684
//   - NicheCompatible predicate referenced from Section 5.7
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/subtyping.cpp
//   Lines 1-461 (complete file)
//
// KEY CONTENT TO MIGRATE:
//   - SpecDefsSubtyping() helper (lines 23-28)
//   - kIntTypes and kFloatTypes arrays (lines 30-36)
//   - IsIntType(), IsFloatType() helpers (lines 38-54)
//   - IsNumericMismatch() - prevents subtyping between different numeric types (lines 56-71)
//   - IsNeverType() helper (lines 73-76)
//   - PermSub() - permission subtyping lattice (lines 84-115)
//     * Permission lattice: unique <: shared <: const
//     * Reflexive rules: Perm-Const, Perm-Unique, Perm-Shared
//     * Transitive rules: Perm-Unique-Const, Perm-Unique-Shared, Perm-Shared-Const
//   - Member() helper for union membership (lines 129-141)
//   - Subtyping() main function (lines 145-458)
//     * Type equivalence check first
//     * Refinement type subtyping (Sub-Refine, Sub-Refine-Elim)
//     * Opaque type subtyping (Sub-Opaque)
//     * Numeric type mismatch check
//     * Never type subtyping (Sub-Never)
//     * Async type subtyping (Sub-Async)
//     * Dynamic type subtyping for ExecutionDomain
//     * String/Bytes modal widening (Sub-String-Modal, Sub-Bytes-Modal)
//     * Permission subtyping (Sub-Perm)
//     * Tuple subtyping (Sub-Tuple)
//     * Array subtyping (Sub-Array)
//     * Slice subtyping (Sub-Slice)
//     * Range subtyping (Sub-Range)
//     * Pointer state subtyping (Sub-Ptr-State)
//     * Modal state incomparability (Modal-Incomparable)
//     * Modal niche subtyping (Sub-Modal-Niche)
//     * Function subtyping with contravariant params (Sub-Func)
//     * Union width subtyping (Sub-Union-Width)
//     * Member-union subtyping (Sub-Member-Union)
//
// DEPENDENCIES:
//   - TypeRef, Type definitions from types.h
//   - TypeEquiv() from type_equiv.h
//   - StripPerm() from type_stmt.h
//   - NicheCompatible() from modal/modal_widen.h
//   - AsyncSigOf() for async type signatures
//   - IsExecutionDomainTypePath() from caps/cap_concurrency.h
//   - StaticProofContext, AddFact, StaticProof from contracts/verification.h
//   - IdKeyOf() from resolve/scopes.h
//
// REFACTORING NOTES:
//   1. Consider separating PermSub into its own inline function for reuse
//   2. The numeric mismatch check could be a shared utility
//   3. AsyncSigOf dependency suggests async type helpers should be accessible
//   4. Modal niche checking requires modal declaration lookup
//   5. The function is large (~300 lines); consider breaking into helper functions:
//      - SubtypingPrim() for primitive cases
//      - SubtypingComposite() for tuple/array/slice/func
//      - SubtypingModal() for modal-specific cases
//      - SubtypingUnion() for union cases
//
// SPEC RULES IMPLEMENTED:
//   - Perm-Const, Perm-Shared, Perm-Unique (reflexive)
//   - Perm-Unique-Shared, Perm-Shared-Const, Perm-Unique-Const (transitive)
//   - Sub-Perm, Sub-Never, Sub-Tuple, Sub-Array, Sub-Slice, Sub-Range
//   - Sub-Ptr-State, Sub-Modal-Niche, Sub-Func, Sub-Async
//   - Sub-Member-Union, Sub-Union-Width
//   - Sub-Refine, Sub-Refine-Elim (refinement types)
//   - Sub-Opaque (opaque types)
//   - Sub-String-Modal, Sub-Bytes-Modal, Sub-String-Widen, Sub-Bytes-Widen
//   - Modal-Incomparable (modal state comparison)
//
// =============================================================================

