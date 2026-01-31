// =============================================================================
// MIGRATION MAPPING: expr/error_expr.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   (Error expressions are not in the spec - they are compiler-internal)
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_expr.cpp
//   ErrorExpr handling (minimal - just propagates error)
//
// KEY CONTENT TO MIGRATE:
//   ERROR EXPRESSION HANDLING:
//   - ErrorExpr nodes are produced by the parser for invalid syntax
//   - They should propagate as type errors
//   - No actual type can be assigned
//   - Error recovery may use a "poison" type
//
//   IMPLEMENTATION:
//   - Return error result immediately
//   - Preserve any diagnostic from parsing
//   - Avoid cascading errors by marking as "already reported"
//
// DEPENDENCIES:
//   - ExprTypeResult for result type
//
// REFACTORING NOTES:
//   1. Consider using a special "error" type for recovery
//   2. Error expressions should not cause additional diagnostics
//   3. The parser should have already reported the issue
//
// RESULT TYPE:
//   ExprTypeResult { ok: false, diag_id: nullopt (already reported), type: nullptr }
//
// =============================================================================

