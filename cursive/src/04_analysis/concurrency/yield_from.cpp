/*
 * =============================================================================
 * MIGRATION MAPPING: yield_from.cpp
 * =============================================================================
 *
 * SPEC REFERENCE:
 *   - C0updated.md, Section 15.2.2 "Yield From" (lines 23880-23920)
 *   - C0updated.md, Section 15.2.3 "Delegation Semantics" (lines 23930-23960)
 *
 * SOURCE FILE:
 *   - cursive-bootstrap/src/03_analysis/caps/cap_concurrency.cpp (lines 1-717)
 *
 * FUNCTIONS TO MIGRATE:
 *   - ValidateYieldFrom(YieldFrom* yf) -> bool
 *       Validate yield from expression structure
 *   - CheckYieldFromTarget(YieldFrom* yf) -> bool
 *       Ensure target is compatible async type
 *   - ValidateYieldFromRelease(YieldFrom* yf, KeyContext* ctx) -> bool
 *       Validate yield release from with held keys
 *   - CheckTypeCompatibility(YieldFrom* yf, AsyncType* outer) -> bool
 *       Ensure inner async matches outer's types
 *   - InferYieldFromType(YieldFrom* yf) -> TypeRef
 *       Infer result type after delegation completes
 *
 * DEPENDENCIES:
 *   - Async type parameters
 *   - Key context tracking
 *   - Type compatibility checking
 *
 * REFACTORING NOTES:
 *   1. yield from delegates to another async
 *   2. Yields all values from inner async
 *   3. Returns inner async's result when complete
 *   4. yield release from releases keys before delegation
 *   5. Type parameters must be compatible between outer and inner
 *   6. Consider flattening for nested async
 *
 * YIELD FROM FORMS:
 *   yield from other_async      // Delegate iteration
 *   yield release from async    // Release keys, then delegate
 *
 * DELEGATION SEMANTICS:
 *   procedure outer() -> Sequence<i32> {
 *       yield 1
 *       yield from inner()    // Yields all of inner's values
 *       yield 3
 *   }
 *   // If inner() yields [10, 20], outer yields [1, 10, 20, 3]
 *
 * TYPE COMPATIBILITY:
 *   - Outer: Async<Out, In, Result, E>
 *   - Inner: Async<Out', In', Result', E'>
 *   - Requirements:
 *     - Out' must be assignable to Out
 *     - In must be assignable to In'
 *     - Result' returned when inner completes
 *
 * DIAGNOSTIC CODES:
 *   - E-CON-0060: Yield from outside async
 *   - E-CON-0061: Yield from type mismatch
 *   - E-CON-0062: Keys held across yield from
 *   - E-CON-0063: Incompatible async types
 *
 * =============================================================================
 */
