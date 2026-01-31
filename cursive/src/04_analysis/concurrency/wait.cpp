/*
 * =============================================================================
 * MIGRATION MAPPING: wait.cpp
 * =============================================================================
 *
 * SPEC REFERENCE:
 *   - C0updated.md, Section 16.3.2 "Wait Expression" (lines 24080-24100)
 *   - C0updated.md, Section 16.6 "Suspension Points" (lines 24150-24180)
 *
 * SOURCE FILE:
 *   - cursive-bootstrap/src/03_analysis/caps/cap_concurrency.cpp (lines 1-717)
 *
 * FUNCTIONS TO MIGRATE:
 *   - ValidateWait(Wait* wait) -> bool
 *       Validate wait expression structure
 *   - CheckWaitTarget(Wait* wait) -> bool
 *       Ensure target is Spawned<T> type
 *   - ValidateNoKeysHeld(Wait* wait, KeyContext* ctx) -> bool
 *       CRITICAL: Ensure no keys held across wait
 *   - InferWaitResultType(Wait* wait) -> TypeRef
 *       Infer result type T from Spawned<T>
 *   - AnalyzeWaitSuspension(Wait* wait) -> SuspensionInfo
 *       Analyze suspension point characteristics
 *
 * DEPENDENCIES:
 *   - Spawned<T> modal type
 *   - Key context tracking
 *   - Suspension point analysis
 *
 * REFACTORING NOTES:
 *   1. wait blocks until Spawned<T> transitions to @Ready
 *   2. Returns the value T from the spawned task
 *   3. CRITICAL: Keys MUST NOT be held across wait suspension
 *   4. Wait is a suspension point (may block current thread)
 *   5. Consider timeout variants for bounded waiting
 *   6. Multiple waits can be combined with all/race
 *
 * WAIT SYNTAX:
 *   let handle: Spawned<i32> = spawn { compute() }
 *   let result: i32 = wait handle
 *
 * WAIT SEMANTICS:
 *   - Input: Spawned<T>@Pending or Spawned<T>@Ready
 *   - Blocks if @Pending, returns immediately if @Ready
 *   - Output: T (the spawned task's result)
 *   - Transitions: @Pending -> @Ready (internally)
 *
 * KEY RESTRICTION:
 *   // WRONG - key held across wait
 *   #shared_data {
 *       let result: i32 = wait handle  // ERROR
 *   }
 *
 *   // CORRECT - wait outside key block
 *   let result: i32 = wait handle
 *   #shared_data {
 *       use(result)
 *   }
 *
 * DIAGNOSTIC CODES:
 *   - E-CON-0030: Wait on non-Spawned type
 *   - E-CON-0031: Keys held across wait
 *   - E-CON-0032: Wait outside parallel context
 *
 * =============================================================================
 */
