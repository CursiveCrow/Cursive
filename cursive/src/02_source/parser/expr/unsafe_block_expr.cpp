// =============================================================================
// MIGRATION MAPPING: unsafe_block_expr.cpp
// =============================================================================
// This file should contain parsing logic for unsafe block expressions.
//
// SPEC REFERENCE: C0updated.md, Section 3.3.8.6, Lines 5374-5377
// =============================================================================
//
// FORMAL RULES FROM SPEC:
// -----------------------------------------------------------------------------
// **(Parse-Unsafe-Expr)** Lines 5374-5377
// IsKw(Tok(P), `unsafe`)    G |- ParseBlock(Advance(P)) => (P_1, b)
// -----------------------------------------------------------------------
// G |- ParsePrimary(P) => (P_1, UnsafeBlockExpr(b))
//
// GRAMMAR:
// -----------------------------------------------------------------------------
// UnsafeBlockExpr = "unsafe" Block
//
// SEMANTICS:
// - The `unsafe` keyword precedes a block expression
// - The block is parsed using standard block parsing (ParseBlock)
// - The resulting AST node is UnsafeBlockExpr containing the parsed block
// - Unsafe blocks permit operations not otherwise allowed:
//   - Raw pointer dereferencing
//   - Transmute operations
//   - FFI calls
//   - Access to mutable statics
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_expr.cpp
// =============================================================================
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
//
// 1. Unsafe Block Expression Parsing (Lines 1811-1818)
//    ---------------------------------------------------------------------------
//    Line 1811: Condition check
//      - Check: tok && IsKwTok(*tok, "unsafe")
//      - This appears within ParsePrimary function
//
//    Line 1812: Spec rule annotation
//      - SPEC_RULE("Parse-Unsafe-Expr");
//
//    Lines 1813-1814: Advance past keyword
//      - Parser next = parser;
//      - Advance(next);
//
//    Line 1815: Parse block
//      - ParseElemResult<std::shared_ptr<Block>> block = ParseBlock(next);
//      - Delegates to standard block parsing
//
//    Lines 1816-1817: Construct AST node
//      - UnsafeBlockExpr unsafe;
//      - unsafe.block = block.elem;
//
//    Line 1818: Return result
//      - return {block.parser, MakeExpr(SpanBetween(parser, block.parser), unsafe)};
//      - Span covers from 'unsafe' keyword to closing brace
//
// COMPLETE SOURCE CODE (Lines 1811-1818):
// -----------------------------------------------------------------------------
//   if (tok && IsKwTok(*tok, "unsafe")) {
//     SPEC_RULE("Parse-Unsafe-Expr");
//     Parser next = parser;
//     Advance(next);
//     ParseElemResult<std::shared_ptr<Block>> block = ParseBlock(next);
//     UnsafeBlockExpr unsafe;
//     unsafe.block = block.elem;
//     return {block.parser, MakeExpr(SpanBetween(parser, block.parser), unsafe)};
//   }
//
// =============================================================================
// RELATED SPEC CONTENT: Statement Form
// =============================================================================
// Note: There is also a statement form (Parse-Unsafe-Block) at Lines 6295-6298
// which produces UnsafeBlockStmt. This file covers the EXPRESSION form only.
//
// **(Parse-Unsafe-Block)** Lines 6295-6298 (for reference)
// IsKw(Tok(P), `unsafe`)    G |- ParseBlock(Advance(P)) => (P_1, b)
// -----------------------------------------------------------------------
// G |- ParseStmtCore(P) => (P_1, UnsafeBlockStmt(b))
//
// The expression form is used in expression context (e.g., as value of a let).
// The statement form is used in statement context.
// Both parse identically but produce different AST node types.
//
// =============================================================================
// DEPENDENCIES:
// =============================================================================
// - ParseBlock(Parser) -> ParseElemResult<std::shared_ptr<Block>>
//   Parses a braced block: '{' Stmt* '}'
// - MakeExpr(Span, ExprNode) -> ExprPtr
//   Constructs an expression node with the given span
// - SpanBetween(Parser start, Parser end) -> Span
//   Creates span covering from start position to end position
// - IsKwTok(Token, string_view) -> bool
//   Checks if token is a keyword with given lexeme
// - Advance(Parser&) -> void
//   Moves parser forward one token
// - UnsafeBlockExpr AST node type
//   Fields: block (std::shared_ptr<Block>)
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
// - This is a simple primary expression form
// - Integrates with ParsePrimary as one case in the keyword dispatch
// - No error recovery needed beyond what ParseBlock provides
// - The span starts at 'unsafe' keyword and ends at the block's closing brace
// - Consider extracting to standalone function for modularity:
//     ParseElemResult<ExprPtr> ParseUnsafeBlockExpr(Parser parser);
// - Would be called from ParsePrimary when 'unsafe' keyword detected
// =============================================================================
