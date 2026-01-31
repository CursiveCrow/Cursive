// =============================================================================
// MIGRATION MAPPING: alloc_expr.cpp
// =============================================================================
// This file should contain parsing logic for allocation expressions.
//
// SPEC REFERENCE: C0updated.md, Section 3.3.8.6, Lines 5178-5181
// =============================================================================
//
// FORMAL RULES FROM SPEC:
// -----------------------------------------------------------------------------
// **(Parse-Alloc-Implicit)** Lines 5178-5181
// IsOp(Tok(P), "^")    G |- ParseExpr(Advance(P)) => (P_1, e)
// -----------------------------------------------------------------------
// G |- ParsePrimary(P) => (P_1, AllocExpr(bottom, e))
//
// where bottom = no region specified (implicit current region)
//
// GRAMMAR:
// -----------------------------------------------------------------------------
// AllocExpr = "^" Expr                  (implicit region)
//           | Ident "^" Expr            (explicit named region)
//
// The spec rule shows only the implicit form. Explicit region syntax is:
//   region as r { let p = r ^ value }
// where 'r' is the region name and '^' allocates in that region.
//
// SEMANTICS:
// - The "^" operator allocates a value in a region
// - Without explicit region prefix, uses the innermost enclosing region
// - Returns Ptr<T>@Valid pointing to the allocated value
// - The value is copied/moved into the region's memory
// - Allocation MUST occur inside a `region { }` block
// - Memory is freed when the region is dropped/exited
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_expr.cpp
// =============================================================================
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
//
// 1. Implicit Allocation Expression Parsing (Lines 1083-1091)
//    ---------------------------------------------------------------------------
//    Line 1083: Condition check
//      - Check: tok && IsOpTok(*tok, "^")
//      - This appears within ParsePrimary function
//
//    Line 1084: Spec rule annotation
//      - SPEC_RULE("Parse-Alloc-Implicit");
//
//    Lines 1085-1086: Advance past operator
//      - Parser next = parser;
//      - Advance(next);
//
//    Line 1087: Parse value expression
//      - ParseElemResult<ExprPtr> expr = ParseExpr(next);
//      - Full expression parsing for the value to allocate
//
//    Lines 1088-1090: Construct AST node
//      - AllocExpr alloc;
//      - alloc.region_opt = std::nullopt;   // Implicit region
//      - alloc.value = expr.elem;
//
//    Line 1091: Return result
//      - return {expr.parser, MakeExpr(SpanBetween(parser, expr.parser), alloc)};
//      - Span covers from '^' operator to end of value expression
//
// COMPLETE SOURCE CODE (Lines 1083-1091):
// -----------------------------------------------------------------------------
//   if (tok && IsOpTok(*tok, "^")) {
//     SPEC_RULE("Parse-Alloc-Implicit");
//     Parser next = parser;
//     Advance(next);
//     ParseElemResult<ExprPtr> expr = ParseExpr(next);
//     AllocExpr alloc;
//     alloc.region_opt = std::nullopt;
//     alloc.value = expr.elem;
//     return {expr.parser, MakeExpr(SpanBetween(parser, expr.parser), alloc)};
//   }
//
// 2. Explicit Region Allocation (if implemented)
//    ---------------------------------------------------------------------------
//    NOTE: The bootstrap code shown only implements implicit allocation.
//    Explicit region allocation (ident ^ expr) may be parsed differently:
//    - Could be handled in identifier parsing
//    - Could be a postfix operator on region identifier
//    - Requires additional investigation of bootstrap source
//
//    Expected syntax: `r ^ value` where r is a region name
//    Expected AST: AllocExpr{ region_opt: Some(r), value: expr }
//
// =============================================================================
// RELATED CONSTRUCTS:
// =============================================================================
// Region statements (for context where allocation is valid):
//
// **(Parse-Region-Stmt)** Lines 6325-6328
// IsKw(Tok(P), `region`)
// G |- ParseRegionOptsOpt(Advance(P)) => (P_1, opts_opt)
// G |- ParseRegionAliasOpt(P_1) => (P_2, alias_opt)
// G |- ParseBlock(P_2) => (P_3, b)
// -----------------------------------------------------------------------
// G |- ParseStmtCore(P) => (P_3, RegionStmt(opts_opt, alias_opt, b))
//
// The region alias (via `as name`) creates a binding that can be used
// with explicit allocation syntax.
//
// =============================================================================
// DEPENDENCIES:
// =============================================================================
// - ParseExpr(Parser) -> ParseElemResult<ExprPtr>
//   Parses any expression (the value to allocate)
// - MakeExpr(Span, ExprNode) -> ExprPtr
//   Constructs an expression node with the given span
// - SpanBetween(Parser start, Parser end) -> Span
//   Creates span covering from start position to end position
// - IsOpTok(Token, string_view) -> bool
//   Checks if token is operator with given lexeme
// - Advance(Parser&) -> void
//   Moves parser forward one token
// - AllocExpr AST node type
//   Fields:
//     - region_opt: std::optional<Identifier> (None for implicit)
//     - value: ExprPtr (expression producing value to allocate)
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
// - Simple primary expression form for implicit allocation
// - Explicit allocation parsing may need investigation
// - Consider extracting to standalone function:
//     ParseElemResult<ExprPtr> ParseAllocExpr(Parser parser);
// - Would be called from ParsePrimary when '^' operator detected
// - Semantic validation (must be in region) happens in later phases
// - The allocated type is inferred from the value expression
// - No error recovery needed beyond what ParseExpr provides
//
// OPEN QUESTIONS:
// - How is explicit region allocation (`r ^ value`) parsed?
// - Is it a separate parsing rule or combined with identifier handling?
// - The bootstrap may handle this in qualified expression parsing
// =============================================================================
