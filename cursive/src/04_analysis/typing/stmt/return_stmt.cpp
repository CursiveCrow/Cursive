// =============================================================================
// MIGRATION MAPPING: stmt/return_stmt.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.2.11: Statement Typing (search for return rules)
//   - return expr: Return with value
//   - return: Return unit (procedure with () return)
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_stmt.cpp
//   Return statement typing
//
// KEY CONTENT TO MIGRATE:
//   RETURN WITH VALUE (return expr):
//   1. Type the return expression
//   2. Check expr type against procedure return type R
//   3. Record return type in Res collection
//   4. Statement diverges (subsequent code unreachable)
//
//   RETURN UNIT (return):
//   1. Procedure return type must be ()
//   2. Record unit in Res collection
//   3. Statement diverges
//
//   PROCEDURE CONTEXT:
//   - R context parameter holds expected return type
//   - Res accumulates all return types (for consistency check)
//   - All returns must be compatible with declared type
//
//   DIVERGENCE:
//   - Return statement is divergent
//   - Following statements are unreachable
//   - Block type becomes ! if only path is return
//
// DEPENDENCIES:
//   - ExprTypeFn for return value typing
//   - CheckExpr() for type checking against R
//   - R context (expected return type)
//   - Res accumulator
//
// SPEC RULES:
//   - T-Return-Value: Return with expression
//   - T-Return-Unit: Return without expression
//   - Return-Type-Err: Type mismatch
//
// RESULT TYPE:
//   StmtResult { Gamma (unchanged), Res=[T], Brk=[], BrkVoid=false }
//   Where T is the return type (or () for bare return)
//
// =============================================================================

