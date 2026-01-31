// =============================================================================
// MIGRATION MAPPING: parallel_expr.cpp
// =============================================================================
// This file should contain parsing logic for parallel expressions, which create
// structured parallel execution blocks with an execution domain and options.
//
// SPEC REFERENCE: C0updated.md, Section 3.3.8.6, Lines 5264-5267
// HELPER RULES: Section 3.3.8.7, Lines 5594-5639
// =============================================================================
//
// FORMAL RULE - Parse-Parallel-Expr (Lines 5264-5267):
// -----------------------------------------------------------------------------
// IsKw(Tok(P), `parallel`)
// Gamma |- ParseExpr_NoBrace(Advance(P)) => (P_1, domain)
// Gamma |- ParseParallelOptsOpt(P_1) => (P_2, opts)
// Gamma |- ParseBlock(P_2) => (P_3, body)
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParsePrimary(P) => (P_3, ParallelExpr(domain, opts, body))
//
// HELPER RULES - Parallel Options (Lines 5596-5639):
// -----------------------------------------------------------------------------
//
// **(Parse-ParallelOptsOpt-None)** Lines 5596-5599
// NOT IsPunc(Tok(P), "[")
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseParallelOptsOpt(P) => (P, [])
//
// **(Parse-ParallelOptsOpt-Yes)** Lines 5601-5604
// IsPunc(Tok(P), "[")
// Gamma |- ParseParallelOptList(Advance(P)) => (P_1, opts)
// IsPunc(Tok(P_1), "]")
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseParallelOptsOpt(P) => (Advance(P_1), opts)
//
// **(Parse-ParallelOptList-Empty)** Lines 5606-5609
// IsPunc(Tok(P), "]")    c = Code(Parse-Syntax-Err)    Emit(c, Tok(P).span)
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseParallelOptList(P) => (P, [])
//
// **(Parse-ParallelOptList-Cons)** Lines 5611-5614
// Gamma |- ParseParallelOpt(P) => (P_1, opt)
// Gamma |- ParseParallelOptListTail(P_1, [opt]) => (P_2, opts)
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseParallelOptList(P) => (P_2, opts)
//
// **(Parse-ParallelOptListTail-End)** Lines 5616-5619
// IsPunc(Tok(P), "]")
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseParallelOptListTail(P, xs) => (P, xs)
//
// **(Parse-ParallelOptListTail-TrailingComma)** Lines 5621-5624
// IsPunc(Tok(P), ",")    IsPunc(Tok(Advance(P)), "]")
// TrailingCommaAllowed(P_0, P, {Punctuator("]")})
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseParallelOptListTail(P, xs) => (Advance(P), xs)
//
// **(Parse-ParallelOptListTail-Comma)** Lines 5626-5629
// IsPunc(Tok(P), ",")
// Gamma |- ParseParallelOpt(Advance(P)) => (P_1, opt)
// Gamma |- ParseParallelOptListTail(P_1, xs ++ [opt]) => (P_2, ys)
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseParallelOptListTail(P, xs) => (P_2, ys)
//
// **(Parse-ParallelOpt-Cancel)** Lines 5631-5634
// IsIdent(Tok(P))    Lexeme(Tok(P)) = `cancel`
// IsPunc(Tok(Advance(P)), ":")
// Gamma |- ParseExpr(Advance(Advance(P))) => (P_1, e)
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseParallelOpt(P) => (P_1, Cancel(e))
//
// **(Parse-ParallelOpt-Name)** Lines 5636-5639
// IsIdent(Tok(P))    Lexeme(Tok(P)) = `name`
// IsPunc(Tok(Advance(P)), ":")
// Tok(Advance(Advance(P))).kind = StringLiteral
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseParallelOpt(P) => (Advance(Advance(Advance(P))), Name(Tok(...)))
//
// SEMANTICS:
// - `parallel domain_expr [options]? { body }`
// - domain_expr: Expression yielding an ExecutionDomain ($cpu(), $gpu(), etc.)
// - Options: `cancel: token_expr`, `name: "string"`
// - Body: Block containing spawn/dispatch statements
// - All spawned work must complete before parallel block exits (fork-join)
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_expr.cpp
// =============================================================================
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
//
// 1. ParseParallelExpr (Lines 1494-1549)
//    ─────────────────────────────────────────────────────────────────────────
//    Lines 1494-1497: Keyword check and initialization
//      - Check for `parallel` keyword
//      - SPEC_RULE("Parse-Parallel-Expr")
//      - Advance past keyword
//
//    Lines 1498-1499: Parse domain expression
//      - ParseExprNoBraceNoBracket(next) for domain
//      - Must not contain braces or brackets (options follow)
//
//    Lines 1501-1541: Parse optional [options]
//      - Check for "[" to start options
//      - Loop parsing options until "]"
//      - Lines 1514-1522: Option kind dispatch
//        * "cancel" -> ParallelOptionKind::Cancel
//        * "name" -> ParallelOptionKind::Name
//        * Other -> EmitParseSyntaxErr
//      - Lines 1523-1536: Parse option value
//        * Expect ":" after option name
//        * Parse expression value
//        * Handle comma separator
//      - Lines 1538-1540: Consume closing "]"
//
//    Lines 1542-1548: Parse body and construct result
//      - ParseBlock(after_opts) for body
//      - Construct ParallelExpr{domain, opts, body}
//      - Return with span covering entire expression
//
// =============================================================================
// DEPENDENCIES:
// =============================================================================
// - ParseExprNoBraceNoBracket: Expression parser variant (no brace/bracket)
// - ParseBlock: Block statement parser
// - ParallelExpr AST node (from ast.hpp)
// - ParallelOption struct with kind enum and value
// - ParallelOptionKind enum: Cancel, Name
// - MakeExpr, SpanBetween helpers
// - Token predicates: IsKwTok, IsPunc, IsIdentTok
// - EmitParseSyntaxErr for error recovery
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
// 1. Consider extracting ParseParallelOptsOpt as a separate function matching
//    the spec structure rather than inline parsing.
//
// 2. The source uses a single loop with switch-like dispatch; consider making
//    the option parsing more modular via ParseParallelOpt helper.
//
// 3. Trailing comma handling should follow TrailingCommaAllowed predicate from
//    spec (only allowed when closing delimiter on different line).
//
// 4. Error recovery could be improved - currently breaks on first error but
//    could attempt to sync to next option or closing bracket.
//
// 5. The spec shows Name option takes a StringLiteral directly, but the source
//    parses it as an expression - verify which is correct.
//
// =============================================================================
