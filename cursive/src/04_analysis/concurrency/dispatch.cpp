/*
 * =============================================================================
 * MIGRATION MAPPING: dispatch.cpp
 * =============================================================================
 *
 * SPEC REFERENCE:
 *   - C0updated.md, Section 16.4 "Dispatch" (lines 24100-24200)
 *   - C0updated.md, Section 16.4.1 "Dispatch Options" (lines 24110-24150)
 *   - C0updated.md, Section 16.4.2 "Reduction" (lines 24160-24200)
 *   - C0updated.md, Section 17.7 "Dispatch Key Clause" (lines 24310-24400)
 *
 * SOURCE FILE:
 *   - cursive-bootstrap/src/03_analysis/caps/cap_concurrency.cpp (lines 1-717)
 *
 * FUNCTIONS TO MIGRATE:
 *   - ValidateDispatch(Dispatch* disp) -> bool
 *       Validate dispatch expression structure
 *   - CheckDispatchRange(Dispatch* disp) -> bool
 *       Validate range expression is valid iterator
 *   - ValidateReduction(Dispatch* disp) -> bool
 *       Validate reduction operator and identity
 *   - AnalyzeDispatchBody(Dispatch* disp) -> BodyInfo
 *       Analyze dispatch body for parallelism safety
 *   - ComputeDispatchOptions(Dispatch* disp) -> DispatchOpts
 *       Parse and validate dispatch options
 *   - ValidateDispatchKeyClause(Dispatch* disp) -> bool
 *       Validate key clause for data-parallel access
 *   - InferDispatchParallelism(Dispatch* disp) -> ParallelismInfo
 *       Infer parallelism from key patterns
 *
 * DEPENDENCIES:
 *   - Range/iterator types
 *   - Reduction operators
 *   - Key system for dispatch keys
 *
 * REFACTORING NOTES:
 *   1. Dispatch is data-parallel iteration
 *   2. Options: reduce (operator), ordered, chunk (size)
 *   3. Reduction operators: +, *, min, max, and, or
 *   4. Key clause enables parallel access to shared data
 *   5. Key patterns determine parallelism (data[i] vs data[i/2])
 *   6. Consider auto-vectorization hints
 *
 * DISPATCH SYNTAX:
 *   dispatch i in 0..1000 {
 *       process(data[i])
 *   }
 *
 *   let sum: i32 = dispatch i in 0..100 [reduce: +] {
 *       arr[i]
 *   }
 *
 *   dispatch i in 0..100 key data[i] write {
 *       data[i] = compute(i)
 *   }
 *
 * DISPATCH OPTIONS:
 *   | Option        | Type     | Effect                           |
 *   |---------------|----------|----------------------------------|
 *   | reduce: op    | Operator | Reduction with op                |
 *   | ordered       | Flag     | Preserve iteration order         |
 *   | chunk: expr   | usize    | Set chunk size                   |
 *
 * DIAGNOSTIC CODES:
 *   - E-CON-0010: Invalid dispatch range
 *   - E-CON-0011: Invalid reduction operator
 *   - E-CON-0012: Dispatch body not parallelizable
 *   - E-CON-0013: Invalid key pattern
 *
 * =============================================================================
 */
