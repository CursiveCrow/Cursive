// =============================================================================
// MIGRATION MAPPING: block_expr.cpp
// =============================================================================
// This file implements parsing for block expressions ({ stmt* expr? }).
//
// =============================================================================
// SPEC REFERENCE: C0updated.md
// =============================================================================
//
// **(Parse-Block-Expr)** Lines 5369-5372:
// IsPunc(Tok(P), "{")    Γ ⊢ ParseBlock(P) ⇓ (P_1, b)
// ──────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParsePrimary(P) ⇓ (P_1, b)
//
// **(Parse-Block)** Lines 6352-6355:
// IsPunc(Tok(P), "{")    Γ ⊢ ParseStmtSeq(Advance(P)) ⇓ (P_1, stmts, tail)
// IsPunc(Tok(P_1), "}")
// ──────────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseBlock(P) ⇓ (Advance(P_1), BlockExpr(stmts, tail))
//
// **(ParseStmtSeq-End)** (implicit, in parser_stmt.cpp):
// IsPunc(Tok(P), "}")
// ──────────────────────────────────────────────
// Γ ⊢ ParseStmtSeq(P) ⇓ (P, [], ⊥)
//
// **(ParseStmtSeq-TailExpr)** (implicit, in parser_stmt.cpp):
// Γ ⊢ ParseExpr(P) ⇓ (P_1, e)    IsPunc(Tok(P_1), "}")
// ────────────────────────────────────────────────────────────────
// Γ ⊢ ParseStmtSeq(P) ⇓ (P_1, [], e)
//
// **(ParseStmtSeq-Cons)** (implicit, in parser_stmt.cpp):
// Γ ⊢ ParseStmt(P) ⇓ (P_1, s)    Γ ⊢ ParseStmtSeq(P_1) ⇓ (P_2, ss, tail)
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseStmtSeq(P) ⇓ (P_2, [s] ++ ss, tail)
//
// SEMANTICS:
// - Block expression: `{ stmt* expr? }`
// - Zero or more statements followed by optional tail expression
// - Tail expression (if present) is the block's value
// - Statements require terminators (semicolon or newline)
// - Blocks create a new scope for local bindings
// - Empty block `{}` has unit type ()
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_stmt.cpp
// =============================================================================
// NOTE: ParseBlock is in parser_stmt.cpp, not parser_expr.cpp
//
// 1. RESULT TYPE: ParseStmtSeqResult
//    Source lines: 330-334
//    -------------------------------------------------------------------------
//    struct ParseStmtSeqResult {
//      Parser parser;
//      std::vector<Stmt> stmts;
//      ExprPtr tail_opt;
//    };
//
// 2. ParseStmtSeq IMPLEMENTATION
//    Source lines: 735-781
//    -------------------------------------------------------------------------
//    ParseStmtSeqResult ParseStmtSeq(Parser parser) {
//      // Skip leading newlines
//      while (Tok(parser) && Tok(parser)->kind == TokenKind::Newline) {
//        Advance(parser);
//      }
//
//      if (IsPunc(parser, "}")) {
//        SPEC_RULE("ParseStmtSeq-End");
//        return {parser, {}, nullptr};
//      }
//
//      // Try to parse as tail expression
//      Parser probe = Clone(parser);
//      ParseElemResult<ExprPtr> tail = ParseExpr(probe);
//      Parser tail_end = Clone(tail.parser);
//      while (Tok(tail_end) && Tok(tail_end)->kind == TokenKind::Newline) {
//        Advance(tail_end);
//      }
//      if (tail.parser.diags.empty() && IsPunc(tail_end, "}")) {
//        SPEC_RULE("ParseStmtSeq-TailExpr");
//        Parser merged = MergeDiag(parser, tail.parser, tail_end);
//        return {merged, {}, tail.elem};
//      }
//
//      // Otherwise, parse as statement and recurse
//      SPEC_RULE("ParseStmtSeq-Cons");
//      ParseElemResult<Stmt> head = ParseStmt(parser);
//      ParseStmtSeqResult tail_seq = ParseStmtSeq(head.parser);
//      std::vector<Stmt> stmts;
//      stmts.reserve(1 + tail_seq.stmts.size());
//      stmts.push_back(std::move(head.elem));
//      stmts.insert(stmts.end(),
//                   std::make_move_iterator(tail_seq.stmts.begin()),
//                   std::make_move_iterator(tail_seq.stmts.end()));
//      tail_seq.stmts = std::move(stmts);
//      return tail_seq;
//    }
//
// 3. ParseBlock IMPLEMENTATION
//    Source lines: 905-928
//    -------------------------------------------------------------------------
//    ParseElemResult<std::shared_ptr<Block>> ParseBlock(Parser parser) {
//      // Skip leading newlines
//      while (Tok(parser) && Tok(parser)->kind == TokenKind::Newline) {
//        Advance(parser);
//      }
//      Parser start = parser;
//      if (!IsPunc(parser, "{")) {
//        EmitParseSyntaxErr(parser, TokSpan(parser));
//        Parser next = AdvanceOrEOF(parser);
//        core::Span span = SpanBetween(start, next);
//        return {next, MakeBlockNode(span, {}, nullptr)};
//      }
//
//      SPEC_RULE("Parse-Block");
//      Parser next = parser;
//      Advance(next);
//      ParseStmtSeqResult seq = ParseStmtSeq(next);
//      if (!IsPunc(seq.parser, "}")) {
//        EmitParseSyntaxErr(seq.parser, TokSpan(seq.parser));
//        core::Span span = SpanBetween(start, seq.parser);
//        return {seq.parser, MakeBlockNode(span, std::move(seq.stmts), seq.tail_opt)};
//      }
//      Parser after = seq.parser;
//      Advance(after);
//      core::Span span = SpanBetween(start, after);
//      return {after, MakeBlockNode(span, std::move(seq.stmts), seq.tail_opt)};
//    }
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_expr.cpp
// =============================================================================
//
// 4. BLOCK EXPRESSION PARSING (within ParsePrimary)
//    Source: Referenced in spec but implementation reuses ParseBlock
//    When ParsePrimary sees "{", it delegates to ParseBlock and wraps
//    the result in a BlockExpr node.
//
//    NOTE: The bootstrap may directly return the Block as an expression
//    or wrap it in BlockExpr. Check actual implementation for details.
//
// =============================================================================
// DEPENDENCIES
// =============================================================================
// FROM expr_common.cpp:
// - MakeExpr(Span, ExprNode) -> ExprPtr
// - SpanBetween(Parser, Parser) -> Span
//
// FROM parser_common.cpp:
// - Parser state type
// - Advance(Parser&) -> void
// - AdvanceOrEOF(Parser) -> Parser
// - Tok(Parser) -> const Token*
// - Clone(Parser) -> Parser
// - MergeDiag(Parser, Parser, Parser) -> Parser
// - IsPunc(Parser, string_view) -> bool
// - EmitParseSyntaxErr(Parser&, Span) -> void
//
// FROM stmt/*.cpp:
// - ParseStmt(Parser) -> ParseElemResult<Stmt>
//
// FROM binary.cpp (or expr entry):
// - ParseExpr(Parser) -> ParseElemResult<ExprPtr>
//
// FROM AST types:
// - Block struct {
//     std::vector<Stmt> stmts;
//     ExprPtr tail_opt;        // Optional tail expression (block value)
//     Span span;
//   }
// - BlockExpr struct { std::shared_ptr<Block> block; }
// - Stmt (various statement types)
// - ExprPtr = std::shared_ptr<Expr>
//
// HELPER FUNCTION:
// - MakeBlockNode(Span, vector<Stmt>, ExprPtr) -> std::shared_ptr<Block>
//   (Factory function for creating Block nodes)
//
// =============================================================================
// REFACTORING NOTES
// =============================================================================
// 1. ParseBlock is currently in parser_stmt.cpp. Consider whether to:
//    - Keep it there (blocks contain statements)
//    - Move to this file (blocks are expressions)
//    - Create shared block.cpp with both perspectives
//
// 2. ParseStmtSeq uses speculative parsing to detect tail expressions.
//    The logic is:
//    - Clone parser
//    - Try ParseExpr
//    - If succeeds without errors and followed by "}", it's a tail expr
//    - Otherwise, parse as statement and recurse
//
// 3. The tail expression detection uses MergeDiag to preserve diagnostics
//    from the successful speculative parse.
//
// 4. Empty blocks {} return empty stmts vector and nullptr tail_opt.
//
// 5. Export standalone functions:
//    ParseElemResult<std::shared_ptr<Block>> ParseBlock(Parser parser);
//    - This is the main export, used by many other parsers
//
//    ParseStmtSeqResult ParseStmtSeq(Parser parser);
//    - Internal helper, may not need export
//
// 6. Block vs BlockExpr distinction:
//    - Block is the AST node with stmts and tail
//    - BlockExpr is an Expr variant that wraps a Block
//    - Some places return BlockExpr, others return Block directly
//    - Decide on consistent representation in new design
//
// 7. Consider whether ParseBlock should skip leading newlines or require
//    the caller to do so. Currently it skips them.
//
// 8. Error recovery: If missing "}", the parser still returns a Block
//    with whatever was parsed, along with a diagnostic.
// =============================================================================
