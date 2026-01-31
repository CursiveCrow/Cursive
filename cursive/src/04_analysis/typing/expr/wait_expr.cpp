// =============================================================================
// MIGRATION MAPPING: expr/wait_expr.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 17.2.4: Wait Expression (lines 25099+)
//   - T-Wait-Spawn (line 25099): Wait on Spawned<T>
//   - T-Wait-Future (line 25104): Wait on Future<T, E>
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_infer.cpp
//   Wait expression typing portions
//
// KEY CONTENT TO MIGRATE:
//   WAIT EXPRESSION (wait handle):
//   1. Type handle expression
//   2. Check handle is waitable (Spawned<T> or Future<T, E>)
//   3. Extract result type from handle type
//   4. Result is the extracted type
//
//   TYPING RULE (T-Wait-Spawn):
//   Gamma |- handle : Spawned<T>
//   --------------------------------------------------
//   Gamma |- wait handle : T
//
//   TYPING RULE (T-Wait-Future):
//   Gamma |- handle : Future<T, E>
//   --------------------------------------------------
//   Gamma |- wait handle : T | E
//
//   WAITABLE TYPES:
//   - Spawned<T>: parallel task result
//   - Future<T, E>: async computation result
//   - Tracked<T, E>: tracked async result (internal)
//
//   BLOCKING SEMANTICS:
//   - wait blocks until result is available
//   - For Spawned<T>: waits for parallel task completion
//   - For Future<T, E>: waits for async completion
//
//   KEY CONSTRAINT:
//   CRITICAL: Keys MUST NOT be held across wait suspension
//   - wait is a suspension point
//   - Holding keys across wait causes deadlock risk
//
// DEPENDENCIES:
//   - ExprTypeFn for handle typing
//   - Spawned<T> type extraction
//   - Future<T, E> type extraction
//   - Key state verification (no held keys)
//
// SPEC RULES:
//   - T-Wait-Spawn (line 25099)
//   - T-Wait-Future (line 25104)
//
// RESULT TYPE:
//   T for Spawned<T>, T | E for Future<T, E>
//
// NOTES:
//   - wait consumes the handle (moves ownership)
//   - Cannot wait on same handle twice
//   - Key constraint critical for correctness
//
// =============================================================================

