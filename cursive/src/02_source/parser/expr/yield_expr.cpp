// =============================================================================
// MIGRATION MAPPING: yield_expr.cpp
// =============================================================================
// This file should contain parsing logic for yield expressions, which suspend
// an async procedure and produce a value for the caller.
//
// SPEC REFERENCE: C0updated.md, Section 3.3.8.6, Lines 5289-5292
// =============================================================================
//
// FORMAL RULE - Parse-Yield-Expr (Lines 5289-5292):
// -----------------------------------------------------------------------------
// IsKw(Tok(P), `yield`)
// P_1 = Advance(P)
// (IsIdent(Tok(P_1)) AND Lexeme(Tok(P_1)) = `release`
//     => release_opt = Release AND P_2 = Advance(P_1))
// (NOT (IsIdent(Tok(P_1)) AND Lexeme(Tok(P_1)) = `release`)
//     => release_opt = bottom AND P_2 = P_1)
// NOT IsKw(Tok(P_2), `from`)  // Distinguishes from yield-from
// Gamma |- ParseExpr(P_2) => (P_3, e)
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParsePrimary(P) => (P_3, YieldExpr(release_opt, e))
//
// NOTE: `yield` is a RESERVED KEYWORD.
// NOTE: `release` is a CONTEXTUAL KEYWORD (identifier with special meaning).
//
// SEMANTICS:
// - `yield expr` - Suspend and produce value
// - `yield release expr` - Release held keys, then suspend and produce value
// - CRITICAL: Plain yield MUST NOT occur while keys are held (E-CON-0213)
// - The `release` modifier allows yielding while keys are held by releasing
//   them before the suspension point
//
// DISTINCTION FROM YIELD-FROM:
// - yield expr       -> YieldExpr (produces single value)
// - yield from expr  -> YieldFromExpr (delegates to another async)
// - Parser checks `from` keyword AFTER optional `release` to distinguish
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_expr.cpp
// =============================================================================
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
//
// 1. ParseYieldExpr (Lines 911-937, shared with yield-from)
//    ─────────────────────────────────────────────────────────────────────────
//    Lines 911-914: Keyword check and initialization
//      - Check for `yield` keyword (IsKwTok)
//      - Save start position for span
//      - Advance past "yield"
//
//    Lines 915-921: Parse optional `release` modifier
//      - Check if next token is identifier "release"
//      - If so, set release = true and advance
//      - Otherwise, release = false
//
//    Lines 922-931: Check for `from` and dispatch to yield-from
//      - If next token is `from` keyword, parse as YieldFromExpr (separate file)
//      - This branch handled in yield_from_expr.cpp
//
//    Lines 932-937: Parse yield value and construct result
//      - SPEC_RULE("Parse-Yield-Expr")
//      - ParseExpr(next) for value expression
//      - Construct YieldExpr{release, value}
//      - Return with span covering entire expression
//
// =============================================================================
// DEPENDENCIES:
// =============================================================================
// - ParseExpr: General expression parser
// - YieldExpr AST node (from ast.hpp)
// - MakeExpr, SpanBetween helpers
// - Token predicates: IsKwTok, IsIdentTok, IsKw
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
// 1. The source code handles both yield and yield-from in the same block.
//    For this refactor, yield_expr.cpp should handle only the YieldExpr case.
//    The common prefix (yield + optional release) could be shared.
//
// 2. Consider extracting the release modifier parsing as a helper since it's
//    shared between yield and yield-from:
//    ParseOptionalRelease(parser) -> (parser, bool release)
//
// 3. The spec uses conditional notation for release_opt; in implementation
//    this is a simple boolean flag check.
//
// 4. The distinction between yield and yield-from depends on checking for
//    the `from` keyword AFTER consuming the optional release modifier.
//
// 5. Semantic analysis must verify that plain yield (without release) does
//    not occur while keys are held - this is not a parsing concern.
//
// =============================================================================
