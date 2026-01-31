/*
 * =============================================================================
 * MIGRATION MAPPING: parallel.cpp
 * =============================================================================
 *
 * SPEC REFERENCE:
 *   - C0updated.md, Section 16 "Structured Parallelism" (lines 23900-24100)
 *   - C0updated.md, Section 16.1 "Parallel Blocks" (lines 23910-24000)
 *   - C0updated.md, Section 16.2 "Execution Domains" (lines 24010-24050)
 *   - C0updated.md, Section 8.9 "E-CON Errors" (line 21584)
 *
 * SOURCE FILE:
 *   - cursive-bootstrap/src/03_analysis/caps/cap_concurrency.cpp (lines 1-717)
 *
 * FUNCTIONS TO MIGRATE:
 *   - ValidateParallelBlock(Parallel* block) -> bool               [lines 100-200]
 *       Validate parallel block structure and requirements
 *   - CheckExecutionDomain(Parallel* block) -> bool                [lines 205-280]
 *       Validate execution domain is valid ($ExecutionDomain)
 *   - AnalyzeParallelCapture(Parallel* block) -> CaptureSet        [lines 285-400]
 *       Analyze captured variables from surrounding scope
 *   - ValidateForkJoin(Parallel* block) -> bool                    [lines 405-500]
 *       Ensure fork-join guarantee is maintained
 *   - CheckCancelToken(Parallel* block) -> bool                    [lines 505-600]
 *       Validate cancel token option if present
 *   - ComputeParallelOptions(Parallel* block) -> ParallelOpts      [lines 605-700]
 *       Parse and validate parallel block options
 *
 * DEPENDENCIES:
 *   - ExecutionDomain modal type
 *   - Context for domain access
 *   - CancelToken modal type
 *   - Capture analysis
 *
 * REFACTORING NOTES:
 *   1. Parallel blocks require $ExecutionDomain (ctx.cpu(), ctx.gpu(), ctx.inline())
 *   2. Fork-join guarantee: All spawned work completes before block exits
 *   3. Options: name (string), cancel (CancelToken)
 *   4. Capture semantics differ by permission (const/shared/unique)
 *   5. Panic handling: First panic raised at block boundary
 *   6. Consider checking for empty parallel blocks
 *
 * PARALLEL SYNTAX:
 *   parallel ctx.cpu() {
 *       spawn { ... }
 *       spawn { ... }
 *   }
 *
 *   parallel ctx.cpu() [name: "work", cancel: token] {
 *       ...
 *   }
 *
 * EXECUTION DOMAINS:
 *   - ctx.cpu(): Parallel on OS threads
 *   - ctx.gpu(): Compute shader execution
 *   - ctx.inline(): Sequential (for testing)
 *
 * DIAGNOSTIC CODES:
 *   - E-CON-0001: Invalid execution domain
 *   - E-CON-0002: Parallel block without spawn/dispatch
 *   - E-CON-0003: Fork-join violation
 *   - E-CON-0004: Invalid parallel option
 *
 * =============================================================================
 */
