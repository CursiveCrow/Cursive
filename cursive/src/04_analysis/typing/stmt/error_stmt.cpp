// =============================================================================
// MIGRATION MAPPING: stmt/error_stmt.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Error recovery in statement parsing
//   - Error statements from parse failures
//   - Propagation of syntax errors
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_infer.cpp
//   Error statement handling
//
// KEY CONTENT TO MIGRATE:
//   ERROR STATEMENT (ErrorStmt):
//   1. Represents a parse error that produced invalid AST
//   2. Propagate error during typechecking
//   3. May attempt partial recovery
//   4. Accumulate diagnostics
//
//   TYPING:
//   ErrorStmt has associated parse error
//   --------------------------------------------------
//   Gamma |- ErrorStmt : !
//   Emit diagnostic for parse error
//
//   ERROR RECOVERY:
//   - Parse errors produce ErrorStmt nodes
//   - Typechecker propagates these errors
//   - Allows continued checking of remaining code
//   - Multiple errors collected for reporting
//
//   RESULT TYPE:
//   - ! (never) or ErrorType
//   - Enables typing to continue past errors
//
// DEPENDENCIES:
//   - Diagnostic system for error reporting
//   - Error recovery state
//
// SPEC RULES:
//   - Error recovery semantics
//
// RESULT TYPE:
//   ! or ErrorType
//
// NOTES:
//   - ErrorStmt is internal representation
//   - Not directly written by users
//   - Enables better error messages and recovery
//
// =============================================================================

