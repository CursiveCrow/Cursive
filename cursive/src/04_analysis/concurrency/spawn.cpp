/*
 * =============================================================================
 * MIGRATION MAPPING: spawn.cpp
 * =============================================================================
 *
 * SPEC REFERENCE:
 *   - C0updated.md, Section 16.3 "Spawn Expression" (lines 24050-24100)
 *   - C0updated.md, Section 16.3.1 "Spawn Options" (lines 24060-24080)
 *   - C0updated.md, Section 16.5 "Capture Semantics" (lines 24050-24150)
 *
 * SOURCE FILE:
 *   - cursive-bootstrap/src/03_analysis/caps/cap_concurrency.cpp (lines 1-717)
 *
 * FUNCTIONS TO MIGRATE:
 *   - ValidateSpawn(Spawn* spawn) -> bool
 *       Validate spawn expression structure
 *   - AnalyzeSpawnCapture(Spawn* spawn) -> CaptureSet
 *       Analyze variables captured by spawn block
 *   - ValidateCapturePermissions(CaptureSet caps) -> bool
 *       Validate captured permissions are safe
 *   - CheckUniqueCapture(Spawn* spawn) -> bool
 *       Ensure unique captures use explicit move
 *   - ComputeSpawnOptions(Spawn* spawn) -> SpawnOpts
 *       Parse and validate spawn options
 *   - InferSpawnReturnType(Spawn* spawn) -> TypeRef
 *       Infer Spawned<T> return type from body
 *
 * DEPENDENCIES:
 *   - Spawned<T> modal type
 *   - Capture analysis
 *   - Permission system
 *
 * REFACTORING NOTES:
 *   1. Spawn creates a parallel task returning Spawned<T>
 *   2. Options: name (string), affinity (CpuSet), priority (Priority)
 *   3. const data: Captured by reference, unlimited sharing
 *   4. shared data: Captured by reference, needs key
 *   5. unique data: MUST use explicit move
 *   6. Spawned<T> is modal: @Pending -> @Ready
 *
 * SPAWN SYNTAX:
 *   let handle: Spawned<i32> = spawn { compute() }
 *
 *   let handle: Spawned<i32> = spawn [name: "worker", priority: High] {
 *       work()
 *   }
 *
 * SPAWN OPTIONS:
 *   | Option   | Type     | Default   | Effect                  |
 *   |----------|----------|-----------|-------------------------|
 *   | name     | string   | Anonymous | Debug label             |
 *   | affinity | CpuSet   | Default   | CPU core affinity       |
 *   | priority | Priority | Normal    | Scheduling priority     |
 *
 * CAPTURE SEMANTICS:
 *   | Permission | Capture          | Notes                    |
 *   |------------|------------------|--------------------------|
 *   | const      | By reference     | Unlimited sharing        |
 *   | shared     | By reference     | Needs key synchronization|
 *   | unique     | MUST use move    | Single owner             |
 *
 * DIAGNOSTIC CODES:
 *   - E-CON-0020: Spawn outside parallel block
 *   - E-CON-0021: Unique capture without move
 *   - E-CON-0022: Invalid spawn option
 *   - E-CON-0023: Conflicting captures
 *
 * =============================================================================
 */
