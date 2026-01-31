/*
 * =============================================================================
 * MIGRATION MAPPING: sync.cpp
 * =============================================================================
 *
 * SPEC REFERENCE:
 *   - C0updated.md, Section 15.4 "Sync Expression" (lines 23940-24000)
 *   - C0updated.md, Section 15.4.1 "Sync Constraints" (lines 23950-23980)
 *
 * SOURCE FILE:
 *   - cursive-bootstrap/src/03_analysis/caps/cap_concurrency.cpp (lines 1-717)
 *
 * FUNCTIONS TO MIGRATE:
 *   - ValidateSync(Sync* sync) -> bool
 *       Validate sync expression structure
 *   - CheckSyncTarget(Sync* sync) -> bool
 *       Ensure target is compatible async type
 *   - ValidateSyncConstraints(Sync* sync) -> bool
 *       Check Out = () and In = () constraints
 *   - CheckNonAsyncContext(Sync* sync) -> bool
 *       Ensure sync is in non-async context
 *   - InferSyncResultType(Sync* sync) -> TypeRef
 *       Infer result type from Future<T, E>
 *
 * DEPENDENCIES:
 *   - Async type and aliases (Future, etc.)
 *   - Context tracking (async vs non-async)
 *   - Type constraint checking
 *
 * REFACTORING NOTES:
 *   1. sync runs async to completion synchronously (blocking)
 *   2. Only allowed in non-async context
 *   3. Async must have Out = () and In = ()
 *   4. Typically used with Future<T, E> alias
 *   5. Result type is T | E from Future<T, E>
 *   6. Consider timeout variants
 *
 * SYNC SYNTAX:
 *   let result: Data | IOError = sync fetch_data()
 *
 * SYNC CONSTRAINTS:
 *   - Target must be Async<Out, In, Result, E>
 *   - Out MUST be ()
 *   - In MUST be ()
 *   - Returns Result | E
 *
 * VALID TARGETS:
 *   - Future<T, E> = Async<(), (), T, E>
 *   - Stream<T, E> = Async<T, (), (), E> (drains all values)
 *
 * CONTEXT RESTRICTION:
 *   // WRONG - sync in async context
 *   procedure async_proc() -> Future<i32, Error> {
 *       let x: i32 = sync other_async()  // ERROR
 *   }
 *
 *   // CORRECT - sync in non-async
 *   procedure main(move ctx: Context) -> i32 {
 *       let x: i32 | Error = sync async_proc()  // OK
 *   }
 *
 * DIAGNOSTIC CODES:
 *   - E-CON-0050: Sync in async context
 *   - E-CON-0051: Sync target has non-unit Out
 *   - E-CON-0052: Sync target has non-unit In
 *   - E-CON-0053: Sync on non-async type
 *
 * =============================================================================
 */
