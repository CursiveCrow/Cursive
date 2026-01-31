// =============================================================================
// MIGRATION MAPPING: expr/sync_expr.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 17.3.5: Sync Expression (lines 25606+)
//   - T-Sync (line 25624): Sync typing rule
//   - sync_expr grammar (line 3235)
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_infer.cpp
//   Sync expression typing portions
//
// KEY CONTENT TO MIGRATE:
//   SYNC EXPRESSION (sync async_expr):
//   1. Type the async expression
//   2. Check async has Out = () and In = ()
//   3. Run async to completion synchronously
//   4. Result is the async's Result type (possibly union with E)
//
//   TYPING RULE (T-Sync):
//   Gamma |- expr : Async<(), (), Result, E>
//   --------------------------------------------------
//   Gamma |- sync expr : Result | E
//
//   CONSTRAINTS:
//   - Only valid in NON-async context
//   - Async must have Out = () (no yields)
//   - Async must have In = () (no inputs)
//   - Blocking operation - runs async to completion
//
//   ERRORS:
//   - E-CON-0212: yield inside sync expression
//   - E-CON-0223: yield from inside sync expression
//   - Sync in async context is disallowed
//
//   USE CASES:
//   - Converting Future<T, E> to T | E synchronously
//   - Running async initialization in non-async main
//   - Testing async code synchronously
//
// DEPENDENCIES:
//   - ExprTypeFn for async expression typing
//   - AsyncSig extraction
//   - Non-async context verification
//   - Type construction for Result | E
//
// SPEC RULES:
//   - T-Sync (line 25624)
//   - E-CON-0212, E-CON-0223 constraints
//
// RESULT TYPE:
//   Result | E (union of success and error types)
//   Or just Result if E = !
//
// NOTES:
//   - sync is blocking - use sparingly
//   - Cannot be nested in async procedures
//   - Converts async to synchronous execution
//
// =============================================================================

