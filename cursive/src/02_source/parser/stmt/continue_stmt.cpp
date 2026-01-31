// =============================================================================
// MIGRATION MAPPING: continue_stmt.cpp
// =============================================================================
// This file should contain parsing logic for continue statements.
//
// SPEC REFERENCE: C0updated.md, Section 3.3.10, Lines 6290-6293
// =============================================================================
//
// FORMAL RULES FROM SPEC:
// -----------------------------------------------------------------------------
// **(Parse-Continue-Stmt)** Lines 6290-6293
// IsKw(Tok(P), `continue`)
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseStmtCore(P) => (Advance(P), ContinueStmt)
//
// TERMINATOR RULES:
// -----------------------------------------------------------------------------
// ContinueStmt does NOT require terminator (it's a control flow statement)
// - Terminator is optional for continue statement
// - If present, it is consumed; if absent, no error
//
// SEMANTICS:
// - `continue` jumps to the next iteration of the innermost loop
// - No value can be provided (unlike break)
// - Continue must appear within a loop context
// - For iterator loops, advances to next element
// - For conditional loops, re-evaluates condition
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_stmt.cpp
// =============================================================================
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
//
// 1. Continue statement parsing in ParseStmtCore (Lines 456-463)
//    ─────────────────────────────────────────────────────────────────────────
//    Lines 456-457: Check for continue keyword
//      - if (IsKw(parser, "continue"))
//
//    Lines 458-463: Parse continue statement
//      - SPEC_RULE("Parse-Continue-Stmt");
//        Parser next = parser;
//        Advance(next);  // consume `continue`
//        ContinueStmt stmt;
//        stmt.span = SpanBetween(start, next);
//        return {next, stmt, true};
//
// CONTINUE DATA STRUCTURE:
// =============================================================================
// struct ContinueStmt {
//   core::Span span;       // Source span
// };
//
// NOTE: ContinueStmt has NO value field (unlike BreakStmt)
// - Continue simply jumps to next iteration
// - No expression follows the keyword
//
// DEPENDENCIES:
// =============================================================================
// - IsKw helper function (parser utilities)
// - ContinueStmt AST node type
// - ConsumeTerminatorOpt function (stmt_common.cpp)
// - SpanBetween helper function
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
// - Continue is the simplest control flow statement
// - Just keyword, no expression
// - Span covers only the `continue` keyword
// - No attributes handling (nothing to attach to)
// - Semantic validation ensures continue appears within loop context
// - Continue does not require terminator but will consume one if present
// =============================================================================
