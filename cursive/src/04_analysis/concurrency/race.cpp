/*
 * =============================================================================
 * MIGRATION MAPPING: race.cpp
 * =============================================================================
 *
 * SPEC REFERENCE:
 *   - C0updated.md, Section 15.5 "Race Expression" (lines 24010-24080)
 *   - C0updated.md, Section 15.5.1 "Race Handlers" (lines 24020-24050)
 *   - C0updated.md, Section 15.5.2 "Race Semantics" (lines 24060-24080)
 *
 * SOURCE FILE:
 *   - cursive-bootstrap/src/03_analysis/caps/cap_concurrency.cpp (lines 1-717)
 *
 * FUNCTIONS TO MIGRATE:
 *   - ValidateRace(Race* race) -> bool
 *       Validate race expression structure
 *   - CheckRaceArmCount(Race* race) -> bool
 *       Ensure at least 2 arms
 *   - ValidateRaceHandlers(Race* race) -> bool
 *       Ensure all handlers are same type (return or yield)
 *   - CheckHandlerTypes(Race* race) -> bool
 *       Validate handler result types are compatible
 *   - InferRaceResultType(Race* race) -> TypeRef
 *       Infer result type from handlers
 *   - AnalyzeRaceCancellation(Race* race) -> CancelInfo
 *       Analyze cancellation behavior for losers
 *
 * DEPENDENCIES:
 *   - Async types for race arms
 *   - Handler type checking
 *   - Cancellation semantics
 *
 * REFACTORING NOTES:
 *   1. race returns first completed result
 *   2. At least 2 arms required
 *   3. All handlers must be same type (all return OR all yield)
 *   4. RaceReturn: Returns value from first completer
 *   5. RaceYield: Yields values as they arrive (streaming)
 *   6. Losing arms are cancelled when winner completes
 *
 * RACE SYNTAX:
 *   let result: i32 = race {
 *       fetch_primary() -> |v: i32| v,
 *       fetch_backup() -> |v: i32| v
 *   }
 *
 *   race {
 *       source_a() -> |v| yield v,
 *       source_b() -> |v| yield v
 *   }
 *
 * HANDLER TYPES:
 *   - RaceReturn: -> |v| v
 *     Returns value, race expression completes
 *   - RaceYield: -> |v| yield v
 *     Yields value, continues racing
 *
 * CANCELLATION:
 *   When one arm completes (RaceReturn):
 *   - Winning arm's result is returned
 *   - Losing arms receive cancellation signal
 *   - Resources cleaned up
 *
 * DIAGNOSTIC CODES:
 *   - E-CON-0070: Race with fewer than 2 arms
 *   - E-CON-0071: Mixed handler types in race
 *   - E-CON-0072: Incompatible handler result types
 *   - E-CON-0073: Race arm is not async
 *
 * =============================================================================
 */
