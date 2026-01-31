// =============================================================================
// MIGRATION MAPPING: expr/parallel_expr.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 17.2: Structured Parallelism (lines 24779+)
//   - T-Parallel (line 24811): Parallel block typing
//   - T-Parallel-Empty (line 24867): Empty parallel block
//   - T-Parallel-Single (line 24871): Single spawn
//   - T-Parallel-Multi (line 24875): Multiple spawns
//   - Parallel block grammar (line 24779)
//   - Structured concurrency invariant (line 24829)
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_infer.cpp
//   Parallel expression typing portions
//
// KEY CONTENT TO MIGRATE:
//   PARALLEL BLOCK (parallel domain opts { body }):
//   1. Type domain expression (must be ExecutionDomain)
//   2. Process block options (name, cancel)
//   3. Type body statements
//   4. Collect spawn/dispatch results
//   5. Compute result type based on contents
//
//   TYPING RULE (T-Parallel):
//   Gamma |- D : $ExecutionDomain
//   opts well-formed
//   B contains spawn/dispatch statements
//   --------------------------------------------------
//   Gamma |- parallel D opts {B} : T
//
//   RESULT TYPE RULES:
//   - Empty block: ()
//   - Single spawn: T (the spawn's result)
//   - Multiple spawns: (T_1, ..., T_n) tuple
//   - Dispatch: depends on reduce option
//
//   BLOCK OPTIONS:
//   - name: string - debugging label
//   - cancel: CancelToken - cooperative cancellation
//
//   CAPTURE SEMANTICS:
//   - const: captured by reference, unlimited sharing
//   - shared: captured by reference, synchronized via keys
//   - unique: MUST use explicit move, at most one work item
//
//   STRUCTURED CONCURRENCY:
//   - All spawned work completes before block exits
//   - Fork-join semantics guaranteed
//   - No escaping of spawned tasks
//
// DEPENDENCIES:
//   - ExprTypeFn for domain typing
//   - SpawnTypeFn for spawn expressions
//   - DispatchTypeFn for dispatch expressions
//   - ExecutionDomain type ($ExecutionDomain)
//   - CancelToken type
//   - Capture analysis
//
// SPEC RULES:
//   - T-Parallel (line 24811)
//   - T-Parallel-Empty (line 24867)
//   - T-Parallel-Single (line 24871)
//   - T-Parallel-Multi (line 24875)
//
// RESULT TYPE:
//   () | T | (T_1, ..., T_n) depending on block contents
//
// NOTES:
//   - spawn/dispatch only valid inside parallel blocks
//   - Captures must respect permission rules
//   - Keys cannot be held across parallel boundaries
//
// =============================================================================

