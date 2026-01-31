// =============================================================================
// MIGRATION MAPPING: dispatch_expr.cpp
// =============================================================================
// This file should contain parsing logic for dispatch expressions, which
// implement data-parallel iteration over ranges with optional key-based
// synchronization and reduction operations.
//
// SPEC REFERENCE: C0updated.md, Section 3.3.8.6, Lines 5279-5282
// HELPER RULES: Section 3.3.8.7, Lines 5583-5753
// =============================================================================
//
// FORMAL RULE - Parse-Dispatch-Expr (Lines 5279-5282):
// -----------------------------------------------------------------------------
// IsKw(Tok(P), `dispatch`)
// Gamma |- ParsePattern(Advance(P)) => (P_1, pat)
// IsIdent(Tok(P_1))    Lexeme(Tok(P_1)) = `in`
// Gamma |- ParseRange(Advance(P_1)) => (P_2, range)
// Gamma |- ParseKeyClauseOpt(P_2) => (P_3, key_clause_opt)
// Gamma |- ParseDispatchOptsOpt(P_3) => (P_4, opts)
// Gamma |- ParseBlock(P_4) => (P_5, body)
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParsePrimary(P) => (P_5, DispatchExpr(pat, range, key_clause_opt, opts, body))
//
// HELPER RULES - Key Clause (Lines 5583-5592):
// -----------------------------------------------------------------------------
//
// **(Parse-KeyClauseOpt-None)** Lines 5583-5586
// NOT Ctx(Tok(P), `key`)
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseKeyClauseOpt(P) => (P, bottom)  // None
//
// **(Parse-KeyClauseOpt-Yes)** Lines 5588-5592
// Ctx(Tok(P), `key`)
// Gamma |- ParseKeyPathExpr(Advance(P)) => (P_1, path)
// Gamma |- ParseKeyMode(P_1) => (P_2, mode)
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseKeyClauseOpt(P) => (P_2, <path, mode>)
//
// HELPER RULES - Dispatch Options (Lines 5695-5753):
// -----------------------------------------------------------------------------
//
// **(Parse-DispatchOptsOpt-None)** Lines 5695-5698
// NOT IsPunc(Tok(P), "[")
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseDispatchOptsOpt(P) => (P, [])
//
// **(Parse-DispatchOptsOpt-Yes)** Lines 5700-5703
// IsPunc(Tok(P), "[")
// Gamma |- ParseDispatchOptList(Advance(P)) => (P_1, opts)
// IsPunc(Tok(P_1), "]")
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseDispatchOptsOpt(P) => (Advance(P_1), opts)
//
// **(Parse-DispatchOptList-Empty)** Lines 5705-5708
// IsPunc(Tok(P), "]")    c = Code(Parse-Syntax-Err)    Emit(c, Tok(P).span)
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseDispatchOptList(P) => (P, [])
//
// **(Parse-DispatchOptList-Cons)** Lines 5710-5713
// Gamma |- ParseDispatchOpt(P) => (P_1, opt)
// Gamma |- ParseDispatchOptListTail(P_1, [opt]) => (P_2, opts)
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseDispatchOptList(P) => (P_2, opts)
//
// **(Parse-DispatchOptListTail-End)** Lines 5715-5718
// IsPunc(Tok(P), "]")
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseDispatchOptListTail(P, xs) => (P, xs)
//
// **(Parse-DispatchOptListTail-TrailingComma)** Lines 5720-5723
// IsPunc(Tok(P), ",")    IsPunc(Tok(Advance(P)), "]")
// TrailingCommaAllowed(P_0, P, {Punctuator("]")})
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseDispatchOptListTail(P, xs) => (Advance(P), xs)
//
// **(Parse-DispatchOptListTail-Comma)** Lines 5725-5728
// IsPunc(Tok(P), ",")
// Gamma |- ParseDispatchOpt(Advance(P)) => (P_1, opt)
// Gamma |- ParseDispatchOptListTail(P_1, xs ++ [opt]) => (P_2, ys)
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseDispatchOptListTail(P, xs) => (P_2, ys)
//
// **(Parse-ReduceOp-Op)** Lines 5730-5733
// IsOp(Tok(P), op)    op in {"+", "*"}
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseReduceOp(P) => (Advance(P), op)
//
// **(Parse-ReduceOp-Ident)** Lines 5735-5738
// IsIdent(Tok(P))
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseReduceOp(P) => (Advance(P), Lexeme(Tok(P)))
//
// **(Parse-DispatchOpt-Reduce)** Lines 5740-5743
// IsIdent(Tok(P))    Lexeme(Tok(P)) = `reduce`
// IsPunc(Tok(Advance(P)), ":")
// Gamma |- ParseReduceOp(Advance(Advance(P))) => (P_1, op)
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseDispatchOpt(P) => (P_1, Reduce(op))
//
// **(Parse-DispatchOpt-Ordered)** Lines 5745-5748
// IsIdent(Tok(P))    Lexeme(Tok(P)) = `ordered`
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseDispatchOpt(P) => (Advance(P), Ordered)
//
// **(Parse-DispatchOpt-Chunk)** Lines 5750-5753
// IsIdent(Tok(P))    Lexeme(Tok(P)) = `chunk`
// IsPunc(Tok(Advance(P)), ":")
// Gamma |- ParseExpr(Advance(Advance(P))) => (P_1, e)
// ────────────────────────────────────────────────────────────────────────────
// Gamma |- ParseDispatchOpt(P) => (P_1, Chunk(e))
//
// SEMANTICS:
// - `dispatch pattern in range_expr key_clause? [options]? { body }`
// - Executes body in parallel for each value in range
// - Key clause: `key path_expr mode` for synchronized access
//   * path_expr: Expression like `data[i]` determining synchronization key
//   * mode: `read` or `write`
// - Options: `reduce: op`, `ordered`, `chunk: expr`
//   * reduce: Combine iteration results (+, *, min, max, and, or, custom)
//   * ordered: Preserve iteration order for side effects
//   * chunk: Set chunk size for work distribution
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_expr.cpp
// =============================================================================
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
//
// 1. ParseDispatchExpr (Lines 1624-1798)
//    ─────────────────────────────────────────────────────────────────────────
//    Lines 1624-1627: Keyword check and initialization
//      - Check for `dispatch` keyword
//      - SPEC_RULE("Parse-Dispatch-Expr")
//      - Advance past keyword
//
//    Lines 1628-1642: Parse pattern and 'in' keyword
//      - ParsePattern(next) for loop variable pattern
//      - Expect "in" contextual keyword
//      - Error recovery if "in" missing
//      - Advance past "in"
//
//    Lines 1640-1643: Parse range expression
//      - ParseExprNoBraceNoBracket(after_in) for range
//      - Must not contain braces/brackets (options follow)
//
//    Lines 1644-1713: Parse optional key clause
//      - Lines 1646-1647: Check for "key" contextual keyword
//      - Lines 1651-1661: Parse key path root
//        * Expect identifier for root variable
//        * Create KeyPathExpr with root name
//      - Lines 1662-1697: Parse key path segments
//        * Handle "." for field access
//        * Handle "[" for index access
//        * Support "#" marker for key boundaries
//        * Nested loop for segment parsing
//      - Lines 1699-1707: Parse key mode
//        * "read" -> KeyMode::Read
//        * "write" -> KeyMode::Write
//        * Default: KeyMode::Read
//      - Lines 1708-1712: Construct DispatchKeyClause
//
//    Lines 1714-1787: Parse optional [options]
//      - Check for "[" to start options
//      - Loop parsing options until "]"
//
//      Lines 1727-1760: Handle reduce option
//        * Parse "reduce:"
//        * Parse reduce operator ("+", "*", or identifier)
//        * Map to ReduceOp enum: Add, Mul, Min, Max, And, Or, Custom
//
//      Lines 1761-1763: Handle ordered option
//        * Just keyword "ordered", no value
//        * DispatchOptionKind::Ordered
//
//      Lines 1764-1774: Handle chunk option
//        * Parse "chunk:"
//        * Parse expression for chunk size
//        * DispatchOptionKind::Chunk
//
//      Lines 1781-1786: Consume comma and closing "]"
//
//    Lines 1789-1797: Parse body and construct result
//      - ParseBlock(after_range) for body
//      - Construct DispatchExpr{pat, range, key_clause, opts, body}
//      - Return with span covering entire expression
//
// =============================================================================
// DEPENDENCIES:
// =============================================================================
// - ParsePattern: Pattern parser
// - ParseExprNoBraceNoBracket: Expression parser variant
// - ParseBlock: Block statement parser
// - ParseExpr: General expression parser
// - DispatchExpr AST node (from ast.hpp)
// - DispatchKeyClause struct with key_path, mode, span
// - KeyPathExpr struct with root, segs (KeySegField | KeySegIndex)
// - KeyMode enum: Read, Write
// - DispatchOption struct with kind enum
// - DispatchOptionKind enum: Reduce, Ordered, Chunk
// - ReduceOp enum: Add, Mul, Min, Max, And, Or, Custom
// - MakeExpr, SpanBetween, SpanCover helpers
// - Token predicates: IsKwTok, IsPunc, IsIdentTok, IsOpTok
// - EmitParseSyntaxErr, SyncStmt for error recovery
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
// 1. Key path parsing is complex - consider extracting ParseKeyPathExpr as
//    a separate function matching the spec structure.
//
// 2. The key path segment parsing handles both field and index access with
//    optional "#" markers - this could be modularized.
//
// 3. Consider extracting ParseDispatchOptsOpt and ParseDispatchOpt as
//    separate functions.
//
// 4. The reduce operator parsing maps identifiers to enum values inline;
//    consider a lookup table approach.
//
// 5. Error recovery could be improved - currently exits loop on first error.
//
// 6. The spec uses ParseRange but source uses ParseExprNoBraceNoBracket;
//    verify if there's a semantic difference.
//
// 7. Trailing comma handling should follow TrailingCommaAllowed predicate.
//
// =============================================================================
