// =============================================================================
// MIGRATION MAPPING: expr/all_expr.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 17.3.7: All Expression (lines 25739+)
//   - all_expr grammar (line 3239)
//   - All expression semantics (line 25728)
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_infer.cpp
//   All expression typing portions
//
// KEY CONTENT TO MIGRATE:
//   ALL EXPRESSION (all { async1, async2, ... }):
//   1. Type each async expression
//   2. Initiate all concurrently
//   3. Wait for all to complete
//   4. If any fails, cancel remaining and propagate first error
//   5. Result is tuple of all results
//
//   TYPING:
//   Gamma |- async_1 : Future<T_1, E_1>
//   Gamma |- async_2 : Future<T_2, E_2>
//   ...
//   --------------------------------------------------
//   Gamma |- all {async_1, async_2, ...} : (T_1, T_2, ...) | E_1 | E_2 | ...
//
//   SEMANTICS (line 25728):
//   1. Initiate all async expressions concurrently
//   2. Wait for all to complete or first to fail
//   3. If any fails:
//      a. Cancel remaining asyncs
//      b. Return first error (by completion order)
//   4. If all succeed:
//      a. Return tuple of results
//
//   ERROR HANDLING:
//   - First failure triggers cancellation
//   - Error type is union of all possible errors
//   - Error returned is first to complete (not first in list)
//
//   RESULT ORDERING:
//   - Success tuple preserves declaration order
//   - all { a(), b() } returns (a_result, b_result)
//
// DEPENDENCIES:
//   - ExprTypeFn for async typing
//   - Future<T, E> type extraction
//   - Tuple type construction
//   - Union type construction for errors
//
// SPEC RULES:
//   - all expression semantics (lines 25728+)
//
// RESULT TYPE:
//   (T_1, T_2, ...) | E_1 | E_2 | ... (tuple of successes or any error)
//
// NOTES:
//   - all enables parallel async execution
//   - Fail-fast semantics on any error
//   - Complements race for concurrent patterns
//
// =============================================================================

