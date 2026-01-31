// =============================================================================
// MIGRATION MAPPING: stmt/defer_stmt.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.2.11: Statement Typing (search for defer rules)
//   - defer { block } executes block at scope exit
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_stmt.cpp
//   Defer statement typing
//
// KEY CONTENT TO MIGRATE:
//   DEFER TYPING:
//   1. Type the deferred block
//   2. Block should have unit type (side-effects only)
//   3. Register deferred action for scope
//   4. No immediate execution
//
//   DEFER SEMANTICS:
//   - Deferred blocks run in LIFO order at scope exit
//   - Run on normal exit, break, return, panic
//   - Cannot return/break from deferred block
//   - Cannot access outer loop targets
//
//   TYPING CONSTRAINTS:
//   - Deferred block type should be ()
//   - Cannot capture outer return type
//   - Cannot use outer break targets
//   - Can access scope bindings (by reference)
//
//   CAPTURE SEMANTICS:
//   - Bindings captured by reference at defer point
//   - If binding moved before scope exit, undefined behavior
//   - Compiler may warn about moved captures
//
// DEPENDENCIES:
//   - Block typing (block_expr.cpp)
//   - Scope tracking for defer registration
//   - Capture analysis
//
// SPEC RULES:
//   - T-Defer: Defer statement typing
//   - Defer-NonUnit-Warn: Non-unit deferred block (warning)
//   - Defer-Return-Err: Return in defer
//   - Defer-Break-Err: Break in defer
//
// RESULT TYPE:
//   StmtResult { Gamma (unchanged), Res=[], Brk=[], BrkVoid=false }
//
// NOTES:
//   - Deferred actions are tracked separately from statement typing
//   - Execution ordering is determined at codegen
//
// =============================================================================

