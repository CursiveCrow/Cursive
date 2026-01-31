// =============================================================================
// MIGRATION MAPPING: all_expr.cpp
// =============================================================================
// This file should contain parsing logic for all expressions, which run
// multiple async operations concurrently and wait for all to complete.
//
// SPEC REFERENCE: C0updated.md, Section 3.3.8.6, Lines 5304-5307
// HELPER RULES: Section 3.3.8.7, Lines 5797-5822
// =============================================================================
//
// FORMAL RULE - Parse-All-Expr (Lines 5304-5307):
// -----------------------------------------------------------------------------
// IsKw(Tok(P), `all`)
// IsPunc(Tok(Advance(P)), "{")
// Gamma |- ParseAllExprList(Advance(Advance(P))) => (P_1, es)
// IsPunc(Tok(P_1), "}")
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParsePrimary(P) => (Advance(P_1), AllExpr(es))
//
// HELPER RULES - All Expression Lists (Lines 5799-5822):
// -----------------------------------------------------------------------------
//
// **(Parse-AllExprList-Cons)** Lines 5799-5802
// Gamma |- ParseExpr(P) => (P_1, e)
// Gamma |- ParseAllExprListTail(P_1, [e]) => (P_2, es)
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseAllExprList(P) => (P_2, es)
//
// **(Parse-AllExprList-Empty)** Lines 5804-5807
// Tok(P) = Punctuator("}")    c = Code(Parse-Syntax-Err)    Emit(c, Tok(P).span)
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseAllExprList(P) => (P, [])
//
// **(Parse-AllExprListTail-End)** Lines 5809-5812
// Tok(P) = Punctuator("}")
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseAllExprListTail(P, xs) => (P, xs)
//
// **(Parse-AllExprListTail-TrailingComma)** Lines 5814-5817
// IsPunc(Tok(P), ",")    IsPunc(Tok(Advance(P)), "}")
// TrailingCommaAllowed(P_0, P, {Punctuator("}")})
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseAllExprListTail(P, xs) => (Advance(P), xs)
//
// **(Parse-AllExprListTail-Comma)** Lines 5819-5822
// IsPunc(Tok(P), ",")
// Gamma |- ParseExpr(Advance(P)) => (P_1, e)
// Gamma |- ParseAllExprListTail(P_1, xs ++ [e]) => (P_2, ys)
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseAllExprListTail(P, xs) => (P_2, ys)
//
// SEMANTICS:
// - `all { expr, expr, ... }` - Wait for all async operations
// - All expressions are started concurrently
// - Returns a tuple of results: (T1, T2, ...) | E1 | E2 | ...
// - If any operation fails, remaining operations are cancelled
// - First error (by completion order) is propagated
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_expr.cpp
// =============================================================================
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
//
// 1. ParseAllExpr (Lines 977-1001)
//    ─────────────────────────────────────────────────────────────────────────
//    Lines 977-980: Keyword check and initialization
//      - Check for `all` keyword (IsKwTok)
//      - SPEC_RULE("Parse-All-Expr")
//      - Advance past keyword
//
//    Lines 981-988: Expect and consume opening brace
//      - Check for "{" punctuator
//      - Error recovery if missing (SyncStmt)
//      - Advance past "{"
//
//    Lines 989-995: Parse expression list
//      - ParseAllExprList(after_l) for list of expressions
//      - Check for "}" closing brace
//      - Error recovery if missing
//
//    Lines 996-1000: Consume closing brace and construct result
//      - Advance past "}"
//      - Construct AllExpr{exprs}
//      - Return with span
//
// 2. ParseAllExprList (Lines 2134-2145)
//    ─────────────────────────────────────────────────────────────────────────
//    Lines 2135-2138: Handle empty case (with error)
//      - SkipNewlines for formatting flexibility
//      - If immediately at "}", return empty
//      - SPEC_RULE("Parse-AllExprList-Empty")
//      - NOTE: Empty list emits error per spec
//
//    Lines 2140-2144: Parse first expression and continue
//      - SPEC_RULE("Parse-AllExprList-Cons")
//      - ParseExpr for first expression
//      - Add to vector
//      - Continue with ParseAllExprListTail
//
// 3. ParseAllExprListTail (Lines 2147-2173)
//    ─────────────────────────────────────────────────────────────────────────
//    Lines 2150-2153: Handle end of list
//      - SkipNewlines for formatting flexibility
//      - If at "}", return accumulated expressions
//      - SPEC_RULE("Parse-AllExprListTail-End")
//
//    Lines 2155-2169: Handle comma continuation
//      - Check for ","
//      - Handle trailing comma case (with error diagnostic)
//        * SPEC_RULE("Parse-AllExprListTail-TrailingComma")
//        * EmitTrailingCommaErr
//        * Still return successfully
//      - Otherwise parse next expression
//        * SPEC_RULE("Parse-AllExprListTail-Comma")
//        * ParseExpr for element
//        * Add to vector
//        * Recursively continue with tail
//
//    Lines 2171-2172: Error handling
//      - If not at "}" and not at ",", emit syntax error
//      - Return accumulated expressions
//
// =============================================================================
// DEPENDENCIES:
// =============================================================================
// - ParseExpr: General expression parser
// - AllExpr AST node (from ast.hpp)
// - MakeExpr, SpanBetween helpers
// - Token predicates: IsKwTok, IsPunc
// - EmitParseSyntaxErr, SyncStmt, SkipNewlines for error recovery
// - EmitTrailingCommaErr, MatchPunct for trailing comma handling
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
// 1. The ParseAllExprList and ParseAllExprListTail functions follow the same
//    pattern as race arms - consider a generic comma-list parser.
//
// 2. The source uses SkipNewlines which the spec doesn't explicitly mention;
//    verify this matches intended formatting flexibility.
//
// 3. Trailing comma handling emits an error but continues parsing - this
//    provides better error recovery while still flagging the issue.
//
// 4. The spec shows empty list as an error (Parse-AllExprList-Empty emits
//    Parse-Syntax-Err), but the source returns empty without always emitting.
//    Verify the error emission behavior matches spec.
//
// 5. Consider whether the list parsing could share infrastructure with
//    tuple element parsing or other comma-separated lists.
//
// 6. The minimum element count (semantic requirement) is not enforced at
//    parse time - this would be a type/semantic check.
//
// =============================================================================
