/*
 * =============================================================================
 * MIGRATION MAPPING: all.cpp
 * =============================================================================
 *
 * SPEC REFERENCE:
 *   - C0updated.md, Section 15.6 "All Expression" (lines 24090-24130)
 *   - C0updated.md, Section 15.6.1 "All Semantics" (lines 24100-24130)
 *
 * SOURCE FILE:
 *   - cursive-bootstrap/src/03_analysis/caps/cap_concurrency.cpp (lines 1-717)
 *
 * FUNCTIONS TO MIGRATE:
 *   - ValidateAll(All* all) -> bool
 *       Validate all expression structure
 *   - CheckAllArmTypes(All* all) -> bool
 *       Validate all arms are async types
 *   - InferAllResultType(All* all) -> TypeRef
 *       Infer tuple result type from arms
 *   - AnalyzeAllCancellation(All* all) -> CancelInfo
 *       Analyze cancellation on failure
 *   - ValidateAllErrorHandling(All* all) -> bool
 *       Validate error type compatibility
 *
 * DEPENDENCIES:
 *   - Async types for all arms
 *   - Tuple type construction
 *   - Error handling
 *
 * REFACTORING NOTES:
 *   1. all waits for all async operations to complete
 *   2. Returns tuple of results when all succeed
 *   3. If any fails, cancels remaining and propagates first error
 *   4. Result type is tuple of individual result types
 *   5. Error types must be compatible (unified)
 *   6. Consider progress tracking for long-running operations
 *
 * ALL SYNTAX:
 *   let results: (i32, i32) = all {
 *       compute_a(),
 *       compute_b()
 *   }
 *
 * ALL SEMANTICS:
 *   - Runs all arms concurrently
 *   - Waits for all to complete
 *   - Success: Returns tuple of all results
 *   - Failure: First error cancels others, propagates error
 *
 * RESULT TYPE:
 *   all {
 *       a: Future<A, E>,
 *       b: Future<B, E>,
 *       c: Future<C, E>
 *   }
 *   => (A, B, C) | E
 *
 * ERROR HANDLING:
 *   - All arms must have compatible error types
 *   - First error is propagated
 *   - Remaining arms receive cancellation
 *   - Cleanup runs for all arms
 *
 * DIAGNOSTIC CODES:
 *   - E-CON-0080: All arm is not async
 *   - E-CON-0081: Incompatible error types in all
 *   - E-CON-0082: Empty all expression
 *
 * =============================================================================
 */
