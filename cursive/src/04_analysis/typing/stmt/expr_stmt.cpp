// =============================================================================
// MIGRATION MAPPING: stmt/expr_stmt.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.2: Static Semantics
//   - Expression statements (expr;)
//   - S_ExprStmt set (line 377)
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_infer.cpp
//   Expression statement typing
//
// KEY CONTENT TO MIGRATE:
//   EXPRESSION STATEMENT (expr;):
//   1. Type the expression
//   2. Discard the result value
//   3. Execute for side effects only
//
//   TYPING:
//   Gamma; R; L |- expr : T
//   --------------------------------------------------
//   Gamma |- expr; : ()
//
//   VALID EXPRESSION STATEMENTS:
//   From S_ExprStmt (line 377):
//   - Method calls: obj~>method(args)
//   - Function calls: f(args)
//   - Assignments: x = value
//   - Control flow: break, continue, return
//   - Memory operations: defer, region
//   - Parallel operations: spawn, dispatch, wait
//   - Async operations: yield, sync
//
//   DISCARDED VALUES:
//   - Result value is discarded (not used)
//   - May warn for unused non-unit values
//   - Side effects are the purpose
//
//   EXAMPLES:
//   print("hello")       // method call for side effect
//   x = x + 1            // assignment as statement
//   list~>push(item)     // mutation method call
//
// DEPENDENCIES:
//   - ExprTypeFn for expression typing
//   - S_ExprStmt set for validation
//
// SPEC RULES:
//   - Expression statement semantics
//   - S_ExprStmt valid expression kinds
//
// RESULT:
//   Statement result ()
//   Expression typed but value discarded
//
// NOTES:
//   - Used for side-effecting expressions
//   - Non-unit results may generate warnings
//   - Terminator (;) or newline required
//
// =============================================================================

