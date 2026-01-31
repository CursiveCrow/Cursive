// =============================================================================
// MIGRATION MAPPING: sync_expr.cpp
// =============================================================================
// This file should contain parsing logic for sync expressions, which run an
// async operation synchronously to completion (blocking).
//
// SPEC REFERENCE: C0updated.md, Section 3.3.8.6, Lines 5294-5297
// =============================================================================
//
// FORMAL RULE - Parse-Sync-Expr (Lines 5294-5297):
// -----------------------------------------------------------------------------
// IsKw(Tok(P), `sync`)
// Gamma |- ParseExpr(Advance(P)) => (P_1, e)
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParsePrimary(P) => (P_1, SyncExpr(e))
//
// NOTE: `sync` is a RESERVED KEYWORD.
//
// SEMANTICS:
// - `sync async_expr` - Run async to completion synchronously
// - Blocks the current execution until the async completes
// - Only allowed in non-async context (cannot nest sync within async)
// - The async must have Out = () and In = () (no intermediate values)
// - Returns T | E where T is the result type and E is the error union
//
// CONSTRAINTS (from type checking):
// - E-CON-0218: sync not in async context
// - E-CON-0219: sync operand must have Out = ()
// - E-CON-0220: sync operand must have In = ()
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_expr.cpp
// =============================================================================
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
//
// 1. ParseSyncExpr (Lines 940-948)
//    ─────────────────────────────────────────────────────────────────────────
//    Lines 940-942: Keyword check and initialization
//      - Check for `sync` keyword (IsKwTok)
//      - SPEC_RULE("Parse-Sync-Expr")
//      - Advance past keyword
//
//    Lines 943-948: Parse operand and construct result
//      - Conditional: allow_brace ? ParseExpr(next) : ParseExprNoBrace(next)
//      - Construct SyncExpr{value}
//      - Return with span covering entire expression
//
// NOTE ON BRACE HANDLING:
// The source conditionally uses ParseExprNoBrace based on the allow_brace
// parameter. This is likely for disambiguation in certain contexts where
// a block would be ambiguous. The spec rule shows unconditional ParseExpr.
//
// =============================================================================
// DEPENDENCIES:
// =============================================================================
// - ParseExpr: General expression parser
// - ParseExprNoBrace: Expression parser without brace-block interpretation
// - SyncExpr AST node (from ast.hpp)
// - MakeExpr, SpanBetween helpers
// - Token predicates: IsKwTok
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
// 1. The source uses conditional brace handling (allow_brace parameter) that
//    isn't reflected in the spec rule. Verify if this is needed for
//    disambiguation or can be simplified.
//
// 2. This is one of the simpler expression parsers - just keyword + expression.
//
// 3. Type checking (not parsing) enforces the constraints about sync not
//    appearing in async context and operand type requirements.
//
// 4. Consider whether the NoBrace variant is actually needed - sync should
//    typically be followed by a function call or simple expression, not
//    a block literal.
//
// =============================================================================
