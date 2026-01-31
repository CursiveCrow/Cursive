// =============================================================================
// MIGRATION MAPPING: stmt/unsafe_block_stmt.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.5: Memory and Pointers - Unsafe blocks
//   - unsafe block as statement
//   - See also expr/unsafe_block_expr.cpp
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_infer.cpp
//   Unsafe block statement typing
//
// KEY CONTENT TO MIGRATE:
//   UNSAFE BLOCK STATEMENT (unsafe { stmts }):
//   1. Push unsafe context
//   2. Type statements in unsafe context
//   3. Pop unsafe context
//   4. Statement result is ()
//
//   TYPING:
//   InUnsafe' = true
//   Gamma; InUnsafe' |- stmts : ()
//   --------------------------------------------------
//   Gamma |- unsafe { stmts } : ()
//
//   NOTE: This is the statement form of unsafe block.
//   The expression form is in expr/unsafe_block_expr.cpp.
//
//   UNSAFE CONTEXT ENABLES:
//   - Raw pointer dereference
//   - Calls to extern procedures
//   - transmute expressions
//   - Manual memory operations
//
//   STATEMENT VS EXPRESSION:
//   - Statement form: unsafe { stmts } - result is ()
//   - Expression form: unsafe { expr } - result is expr type
//   - Both enable same unsafe operations
//
// DEPENDENCIES:
//   - StmtListTypeFn for body typing
//   - UnsafeContext tracking
//
// SPEC RULES:
//   - Unsafe block semantics (statement form)
//
// RESULT TYPE:
//   () (unit - statement form)
//
// NOTES:
//   - Statement form used when unsafe code has no result value
//   - Otherwise semantically identical to expression form
//
// =============================================================================

