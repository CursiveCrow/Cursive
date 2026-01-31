// =============================================================================
// MIGRATION MAPPING: expr/race_expr.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 17.3.6: Race Expression (lines 25660+)
//   - T-Race (line 25671): Race with return handlers
//   - T-Race-Stream (line 25676): Race with yield handlers
//   - race_arm grammar (line 3237)
//   - race_handler grammar (line 3238)
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_infer.cpp
//   Race expression typing portions
//
// KEY CONTENT TO MIGRATE:
//   RACE EXPRESSION (race { arm1, arm2, ... }):
//   1. Type each arm's async expression
//   2. Type each arm's handler
//   3. Verify all handlers are same kind (return or yield)
//   4. Initiate all asyncs concurrently
//   5. First to complete triggers its handler
//
//   TYPING RULE (T-Race):
//   All arms have return handlers (-> |v| expr)
//   Gamma |- async_i : Async<T_i, (), R_i, E_i>
//   All handlers return type T
//   --------------------------------------------------
//   Gamma |- race {...} : T
//
//   TYPING RULE (T-Race-Stream):
//   All arms have yield handlers (-> |v| yield expr)
//   Inside async context
//   --------------------------------------------------
//   Gamma |- race {...} : ()
//
//   RACE ARM SYNTAX:
//   async_expr -> |pattern| handler
//   - pattern binds the yielded/completed value
//   - handler is either expression (return) or yield expression
//
//   HANDLER TYPES:
//   - Return handler: -> |v| expr - returns value
//   - Yield handler: -> |v| yield expr - yields value
//
//   REQUIREMENTS:
//   - At least 2 arms required
//   - All handlers must be same type (all return OR all yield)
//   - Asyncs must have compatible signatures
//
//   SEMANTICS (line 25720):
//   1. Initiate all async expressions concurrently
//   2. When first produces value:
//      a. Cancel remaining asyncs
//      b. Invoke corresponding handler
//      c. Return/yield handler result
//
// DEPENDENCIES:
//   - ExprTypeFn for async and handler typing
//   - AsyncSig extraction
//   - Handler kind validation
//   - Type unification for return handlers
//   - AsyncContext for yield handlers
//
// SPEC RULES:
//   - T-Race (line 25671)
//   - T-Race-Stream (line 25676)
//
// RESULT TYPE:
//   T for return handlers, () for yield handlers
//
// NOTES:
//   - race enables first-completed-wins patterns
//   - Remaining asyncs cancelled on first completion
//   - Useful for timeouts and redundant requests
//
// =============================================================================

