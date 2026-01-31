// =============================================================================
// MIGRATION MAPPING: yield_from_expr.cpp
// =============================================================================
// This file should contain parsing logic for yield-from expressions, which
// delegate to another async and yield all of its values.
//
// SPEC REFERENCE: C0updated.md, Section 3.3.8.6, Lines 5284-5287
// =============================================================================
//
// FORMAL RULE - Parse-Yield-From-Expr (Lines 5284-5287):
// -----------------------------------------------------------------------------
// IsKw(Tok(P), `yield`)
// P_1 = Advance(P)
// (IsIdent(Tok(P_1)) AND Lexeme(Tok(P_1)) = `release`
//     => release_opt = Release AND P_2 = Advance(P_1))
// (NOT (IsIdent(Tok(P_1)) AND Lexeme(Tok(P_1)) = `release`)
//     => release_opt = bottom AND P_2 = P_1)
// IsKw(Tok(P_2), `from`)  // This IS the yield-from case
// Gamma |- ParseExpr(Advance(P_2)) => (P_3, e)
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParsePrimary(P) => (P_3, YieldFromExpr(release_opt, e))
//
// NOTE: `yield` is a RESERVED KEYWORD.
// NOTE: `from` is a RESERVED KEYWORD.
// NOTE: `release` is a CONTEXTUAL KEYWORD (identifier with special meaning).
//
// SEMANTICS:
// - `yield from async_expr` - Delegate iteration to another async
// - `yield release from async_expr` - Release keys then delegate
// - The delegate async's yielded values become this async's yielded values
// - Commonly used to compose sequences and flatten nested async operations
// - CRITICAL: Plain yield-from MUST NOT occur while keys are held (E-CON-0213)
//
// DISTINCTION FROM YIELD:
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
// 1. ParseYieldFromExpr (Lines 911-931, shared with yield)
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
//    Lines 922-930: Parse yield-from expression
//      - Check for `from` keyword (IsKw)
//      - SPEC_RULE("Parse-Yield-From-Expr")
//      - Advance past "from"
//      - ParseExpr(after_from) for delegate expression
//      - Construct YieldFromExpr{release, value}
//      - Return with span covering entire expression
//
// =============================================================================
// DEPENDENCIES:
// =============================================================================
// - ParseExpr: General expression parser
// - YieldFromExpr AST node (from ast.hpp)
// - MakeExpr, SpanBetween helpers
// - Token predicates: IsKwTok, IsIdentTok, IsKw
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
// 1. The source code handles both yield and yield-from in the same block.
//    For this refactor, yield_from_expr.cpp should handle only YieldFromExpr.
//    The common prefix (yield + optional release) could be shared with yield.
//
// 2. Consider a shared helper for the yield prefix parsing:
//    ```
//    ParseYieldPrefix(parser) -> (parser, bool release, bool is_from)
//    ```
//    Then dispatch to yield or yield-from based on is_from.
//
// 3. Alternatively, keep the parsing logic together in one file with both
//    YieldExpr and YieldFromExpr, then export appropriate functions.
//
// 4. The spec rule numbering puts yield-from (5284) before yield (5289),
//    suggesting the from-check happens first in the parsing logic, but
//    the actual code checks `from` after the common prefix.
//
// 5. Semantic analysis must verify that plain yield-from (without release)
//    does not occur while keys are held - this is not a parsing concern.
//
// =============================================================================
