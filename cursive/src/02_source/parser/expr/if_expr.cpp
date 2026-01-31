// =============================================================================
// MIGRATION MAPPING: if_expr.cpp
// =============================================================================
// This file implements parsing for if-expressions, including the else clause
// and else-if chains.
//
// =============================================================================
// SPEC REFERENCE: C0updated.md
// =============================================================================
//
// **(Parse-If-Expr)** Lines 5249-5252:
// IsKw(Tok(P), `if`)    Γ ⊢ ParseExpr_NoBrace(Advance(P)) ⇓ (P_1, c)
// Γ ⊢ ParseBlock(P_1) ⇓ (P_2, b1)    Γ ⊢ ParseElseOpt(P_2) ⇓ (P_3, b2)
// ──────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParsePrimary(P) ⇓ (P_3, IfExpr(c, b1, b2))
//
// **(Parse-ElseOpt-None)** Lines 6018-6021:
// ¬ IsKw(Tok(P), `else`)
// ──────────────────────────────────────────────
// Γ ⊢ ParseElseOpt(P) ⇓ (P, ⊥)
//
// **(Parse-ElseOpt-If)** Lines 6023-6026:
// IsKw(Tok(P), `else`)    IsKw(Tok(Advance(P)), `if`)
// Γ ⊢ ParsePrimary(Advance(P)) ⇓ (P_1, e)
// ──────────────────────────────────────────────
// Γ ⊢ ParseElseOpt(P) ⇓ (P_1, e)
//
// **(Parse-ElseOpt-Block)** Lines 6028-6031:
// IsKw(Tok(P), `else`)    Γ ⊢ ParseBlock(Advance(P)) ⇓ (P_1, b)
// ────────────────────────────────────────────────────────────────
// Γ ⊢ ParseElseOpt(P) ⇓ (P_1, b)
//
// SEMANTICS:
// - If expression consists of: condition, then-block, optional else-expr
// - Condition is parsed with ParseExpr_NoBrace (no brace allowed in condition)
// - Then-block is parsed with ParseBlock (requires braces)
// - Else clause is optional and can be:
//   - Nothing (else_opt = nullptr)
//   - else if ... (recursive if expression via ParsePrimary)
//   - else { ... } (block expression)
// - Chained else-if creates nested IfExpr in else_expr field
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_expr.cpp
// =============================================================================
//
// 1. RESULT TYPE: ElseOptResult
//    Source lines: 439-442
//    -------------------------------------------------------------------------
//    struct ElseOptResult {
//      Parser parser;
//      ExprPtr else_opt;
//    };
//
// 2. FORWARD DECLARATION
//    Source line: 444
//    -------------------------------------------------------------------------
//    ElseOptResult ParseElseOpt(Parser parser);
//
// 3. IF EXPRESSION PARSING (within ParsePrimary)
//    Source lines: 1438-1454
//    -------------------------------------------------------------------------
//    if (tok && IsKwTok(*tok, "if")) {
//      SPEC_RULE("Parse-If-Expr");
//      Parser next = parser;
//      Advance(next);
//      ParseElemResult<ExprPtr> cond = ParseExprNoBrace(next);
//      ParseElemResult<std::shared_ptr<Block>> then_block = ParseBlock(cond.parser);
//      BlockExpr then_expr;
//      then_expr.block = then_block.elem;
//      ExprPtr then_node = MakeExpr(SpanBetween(cond.parser, then_block.parser),
//                                   then_expr);
//      ElseOptResult else_opt = ParseElseOpt(then_block.parser);
//      IfExpr ifexpr;
//      ifexpr.cond = cond.elem;
//      ifexpr.then_expr = then_node;
//      ifexpr.else_expr = else_opt.else_opt;
//      return {else_opt.parser, MakeExpr(SpanBetween(parser, else_opt.parser), ifexpr)};
//    }
//
// 4. ParseElseOpt IMPLEMENTATION
//    Source lines: 2521-2538
//    -------------------------------------------------------------------------
//    ElseOptResult ParseElseOpt(Parser parser) {
//      if (!IsKw(parser, "else")) {
//        SPEC_RULE("Parse-ElseOpt-None");
//        return {parser, nullptr};
//      }
//      Parser next = parser;
//      Advance(next);
//      if (IsKw(next, "if")) {
//        SPEC_RULE("Parse-ElseOpt-If");
//        ParseElemResult<ExprPtr> expr = ParsePrimary(next, true);
//        return {expr.parser, expr.elem};
//      }
//      SPEC_RULE("Parse-ElseOpt-Block");
//      ParseElemResult<std::shared_ptr<Block>> block = ParseBlock(next);
//      BlockExpr blk;
//      blk.block = block.elem;
//      return {block.parser, MakeExpr(SpanBetween(parser, block.parser), blk)};
//    }
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
// - Tok(Parser) -> const Token*
// - IsKw(Parser, string_view) -> bool
// - IsKwTok(Token, string_view) -> bool
//
// FROM block_expr.cpp (or parser_stmt.cpp):
// - ParseBlock(Parser) -> ParseElemResult<std::shared_ptr<Block>>
//
// FROM binary.cpp (or expr entry):
// - ParseExprNoBrace(Parser) -> ParseElemResult<ExprPtr>
//
// FROM primary.cpp (circular for else-if):
// - ParsePrimary(Parser, bool) -> ParseElemResult<ExprPtr>
//
// FROM AST types:
// - IfExpr struct { ExprPtr cond; ExprPtr then_expr; ExprPtr else_expr; }
// - BlockExpr struct { std::shared_ptr<Block> block; }
// - Block struct { std::vector<Stmt> stmts; ExprPtr tail_opt; Span span; }
// - ExprPtr = std::shared_ptr<Expr>
//
// =============================================================================
// REFACTORING NOTES
// =============================================================================
// 1. Extract ParseElseOpt as a separate helper function in this file.
//
// 2. The then-block is wrapped in a BlockExpr before being stored in IfExpr.
//    This creates a uniform AST where then_expr and else_expr are both ExprPtr.
//
// 3. For else-if chains, ParsePrimary is called recursively with allow_brace=true,
//    which allows the recursive if-expression to contain block expressions.
//
// 4. The bootstrap wraps the then-block in a BlockExpr node. Consider whether
//    the new AST design should do the same or store Block directly.
//
// 5. Export a standalone function:
//    ParseElemResult<ExprPtr> ParseIfExpr(Parser parser);
//    - Assumes "if" keyword already checked by caller
//    - Returns complete IfExpr node
//
// 6. Alternative: Export TryParseIfExpr that returns std::optional if not at "if".
// =============================================================================
