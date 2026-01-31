// =============================================================================
// MIGRATION MAPPING: wait_expr.cpp
// =============================================================================
// This file should contain parsing logic for wait expressions, which block
// until a spawned task completes and retrieve its result.
//
// SPEC REFERENCE: C0updated.md, Section 3.3.8.6, Lines 5274-5277
// =============================================================================
//
// FORMAL RULE - Parse-Wait-Expr (Lines 5274-5277):
// -----------------------------------------------------------------------------
// IsIdent(Tok(P))    Lexeme(Tok(P)) = `wait`
// Gamma |- ParseExpr(Advance(P)) => (P_1, handle)
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParsePrimary(P) => (P_1, WaitExpr(handle))
//
// NOTE: `wait` is a CONTEXTUAL KEYWORD, not a reserved keyword.
// This means it can be used as an identifier in other contexts.
// The parser must check IsIdent && Lexeme == "wait" (not IsKw).
//
// SEMANTICS:
// - `wait handle_expr`
// - Blocks until the Spawned<T> handle completes
// - Returns the value of type T from the spawned task
// - Must appear within a parallel block context
// - CRITICAL: Keys MUST NOT be held across wait suspension points (E-CON-0206)
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_expr.cpp
// =============================================================================
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
//
// 1. ParseWaitExpr (Lines 1104-1115)
//    ─────────────────────────────────────────────────────────────────────────
//    Lines 1104-1106: Contextual keyword check
//      - NOTE: Appears BEFORE general identifier parsing in ParsePrimary
//      - This is critical to prevent "wait" being consumed as simple identifier
//      - Check: tok->kind == TokenKind::Identifier && tok->lexeme == "wait"
//      - NOT IsKwTok (wait is contextual, not reserved)
//
//    Lines 1107-1108: Rule annotation and initialization
//      - SPEC_RULE("Parse-Wait-Expr")
//      - Advance past "wait" contextual keyword
//
//    Lines 1110-1114: Parse handle expression and construct result
//      - ParseExpr(next) for handle expression
//      - Construct WaitExpr{handle}
//      - Return with span covering entire expression
//
// =============================================================================
// DEPENDENCIES:
// =============================================================================
// - ParseExpr: General expression parser
// - WaitExpr AST node (from ast.hpp)
// - MakeExpr, SpanBetween helpers
// - Token kind checks (NOT IsKwTok for contextual keyword)
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
// 1. CRITICAL: The wait expression check MUST appear before general identifier
//    parsing to prevent "wait" from being consumed as a simple IdentifierExpr.
//    This ordering dependency should be documented in the new code.
//
// 2. Consider adding a helper like IsContextualKeyword("wait") to make the
//    intent clearer than raw kind/lexeme checks.
//
// 3. The source comment on line 1104-1105 explains this ordering requirement;
//    preserve this documentation in the refactored code.
//
// 4. Semantic analysis must verify wait appears within parallel context and
//    that no keys are held - this is not a parsing concern but should be
//    noted for downstream analysis.
//
// =============================================================================
