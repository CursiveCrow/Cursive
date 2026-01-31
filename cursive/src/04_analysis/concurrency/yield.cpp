/*
 * =============================================================================
 * MIGRATION MAPPING: yield.cpp
 * =============================================================================
 *
 * SPEC REFERENCE:
 *   - C0updated.md, Section 15.2 "Yield Expression" (lines 23820-23900)
 *   - C0updated.md, Section 15.2.1 "Yield Forms" (lines 23830-23870)
 *   - C0updated.md, Section 16.8 "Async-Key Integration" (lines 24150-24200)
 *
 * SOURCE FILE:
 *   - cursive-bootstrap/src/03_analysis/caps/cap_concurrency.cpp (lines 1-717)
 *
 * FUNCTIONS TO MIGRATE:
 *   - ValidateYield(Yield* yield) -> bool
 *       Validate yield expression structure
 *   - CheckYieldContext(Yield* yield) -> bool
 *       Ensure yield is in async procedure
 *   - ValidateYieldRelease(Yield* yield, KeyContext* ctx) -> bool
 *       Validate yield release with held keys
 *   - CheckYieldType(Yield* yield, TypeRef expected) -> bool
 *       Validate yielded value matches Out type
 *   - AnalyzeYieldSuspension(Yield* yield) -> SuspensionInfo
 *       Analyze suspension point characteristics
 *
 * DEPENDENCIES:
 *   - Async type parameters (Out, In, Result, E)
 *   - Key context tracking
 *   - Async procedure detection
 *
 * REFACTORING NOTES:
 *   1. yield suspends async procedure and outputs value
 *   2. yield release releases held keys before suspend
 *   3. CRITICAL: Keys MUST NOT be held across yield without release
 *   4. After yield, procedure may receive input via resume
 *   5. Yield type must match Async<Out, ...> parameter
 *   6. Consider warn on stale bindings after yield release
 *
 * YIELD FORMS:
 *   yield value                  // Yield and suspend
 *   yield release value          // Release keys, then yield
 *
 * YIELD SEMANTICS:
 *   - Outputs: value of type Out
 *   - Suspends: procedure execution
 *   - Resumes: when caller calls resume(input)
 *   - Input: value of type In available after resume
 *
 * KEY INTEGRATION:
 *   // WRONG - key held across yield
 *   #shared_data {
 *       yield value        // ERROR: E-CON-0213
 *   }
 *
 *   // CORRECT - yield release
 *   #shared_data {
 *       yield release value  // OK: keys released before suspend
 *   }
 *
 * STALENESS WARNING:
 *   After yield release, bindings derived from shared data are stale.
 *   Must re-acquire keys and re-read after resume.
 *
 * DIAGNOSTIC CODES:
 *   - E-CON-0040: Yield outside async procedure
 *   - E-CON-0041: Yield type mismatch
 *   - E-CON-0213: Key held across yield (spec code)
 *   - W-CON-0001: Data may be stale after yield release
 *
 * =============================================================================
 */
