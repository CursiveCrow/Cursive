// =============================================================================
// MIGRATION MAPPING: loop_infinite.cpp
// =============================================================================
// This file implements parsing for infinite loop expressions (loop { body }).
//
// =============================================================================
// SPEC REFERENCE: C0updated.md
// =============================================================================
//
// **(Parse-Loop-Expr)** Lines 5259-5262:
// IsKw(Tok(P), `loop`)    Γ ⊢ ParseLoopTail(Advance(P)) ⇓ (P_1, loop)
// ────────────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParsePrimary(P) ⇓ (P_1, loop)
//
// **(Parse-LoopTail-Infinite)** Lines 5991-5994:
// Γ ⊢ ParseLoopInvariantOpt(P) ⇓ (P_0, inv_opt)
// IsPunc(Tok(P_0), "{")    Γ ⊢ ParseBlock(P_0) ⇓ (P_1, b)
// ────────────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseLoopTail(P) ⇓ (P_1, LoopInfinite(inv_opt, b))
//
// **(Parse-LoopInvariantOpt-None)** (implicit in spec):
// ¬ IsKw(Tok(P), `where`)
// ──────────────────────────────────────────────
// Γ ⊢ ParseLoopInvariantOpt(P) ⇓ (P, ⊥)
//
// **(Parse-LoopInvariantOpt-Yes)** (implicit in spec):
// IsKw(Tok(P), `where`)    IsPunc(Tok(Advance(P)), "{")
// Γ ⊢ ParseExpr(Advance(Advance(P))) ⇓ (P_1, pred)    IsPunc(Tok(P_1), "}")
// ────────────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseLoopInvariantOpt(P) ⇓ (Advance(P_1), LoopInvariant(pred))
//
// SEMANTICS:
// - Infinite loop: `loop { body }` or `loop where { invariant } { body }`
// - Body is a block expression (braces required)
// - Optional invariant clause: `where { predicate }`
// - Infinite loops typically exit via `break` or `return`
// - Loop can produce a value via `break value`
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_expr.cpp
// =============================================================================
//
// 1. RESULT TYPE: LoopTailResult
//    Source lines: 424-428
//    -------------------------------------------------------------------------
//    struct LoopTailResult {
//      Parser parser;
//      ExprPtr loop_expr;
//    };
//
// 2. FORWARD DECLARATION
//    Source line: 429
//    -------------------------------------------------------------------------
//    LoopTailResult ParseLoopTail(Parser parser);
//
// 3. LOOP EXPRESSION PARSING (within ParsePrimary)
//    Source lines: 1484-1490
//    -------------------------------------------------------------------------
//    if (tok && IsKwTok(*tok, "loop")) {
//      SPEC_RULE("Parse-Loop-Expr");
//      Parser next = parser;
//      Advance(next);
//      LoopTailResult tail = ParseLoopTail(next);
//      return {tail.parser, tail.loop_expr};
//    }
//
// 4. ParseLoopTail - INFINITE LOOP CASE
//    Source lines: 2473-2482
//    -------------------------------------------------------------------------
//    LoopTailResult ParseLoopTail(Parser parser) {
//      if (IsPunc(parser, "{") || IsKw(parser, "where")) {
//        SPEC_RULE("Parse-LoopTail-Infinite");
//        ParseElemResult<std::optional<LoopInvariant>> inv = ParseLoopInvariantOpt(parser);
//        ParseElemResult<std::shared_ptr<Block>> body = ParseBlock(inv.parser);
//        LoopInfiniteExpr loop;
//        loop.invariant_opt = inv.elem;
//        loop.body = body.elem;
//        return {body.parser, MakeExpr(SpanBetween(parser, body.parser), loop)};
//      }
//      // ... other cases (iter, cond) ...
//    }
//
// 5. ParseLoopInvariantOpt IMPLEMENTATION
//    Source lines: 2440-2471
//    -------------------------------------------------------------------------
//    ParseElemResult<std::optional<LoopInvariant>> ParseLoopInvariantOpt(Parser parser) {
//      if (!IsKw(parser, "where")) {
//        SPEC_RULE("Parse-LoopInvariantOpt-None");
//        return {parser, std::nullopt};
//      }
//      SPEC_RULE("Parse-LoopInvariantOpt-Yes");
//      Parser start = parser;
//      Parser next = parser;
//      Advance(next);  // consume where
//      if (!IsPunc(next, "{")) {
//        EmitParseSyntaxErr(next, TokSpan(next));
//        Parser sync = next;
//        SyncStmt(sync);
//        return {sync, std::nullopt};
//      }
//      Parser after_l = next;
//      Advance(after_l);
//      ParseElemResult<ExprPtr> pred = ParseExpr(after_l);
//      Parser after_pred = pred.parser;
//      if (!IsPunc(after_pred, "}")) {
//        EmitParseSyntaxErr(after_pred, TokSpan(after_pred));
//        Parser sync = after_pred;
//        SyncStmt(sync);
//        return {sync, std::nullopt};
//      }
//      Parser after = after_pred;
//      Advance(after);
//      LoopInvariant inv;
//      inv.predicate = pred.elem;
//      inv.span = SpanBetween(start, after);
//      return {after, inv};
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
// - IsPunc(Parser, string_view) -> bool
// - SyncStmt(Parser&) -> void
// - EmitParseSyntaxErr(Parser&, Span) -> void
//
// FROM block_expr.cpp (or parser_stmt.cpp):
// - ParseBlock(Parser) -> ParseElemResult<std::shared_ptr<Block>>
//
// FROM binary.cpp (or expr entry):
// - ParseExpr(Parser) -> ParseElemResult<ExprPtr>
//
// FROM AST types:
// - LoopInfiniteExpr struct {
//     std::optional<LoopInvariant> invariant_opt;
//     std::shared_ptr<Block> body;
//   }
// - LoopInvariant struct { ExprPtr predicate; Span span; }
// - Block struct { std::vector<Stmt> stmts; ExprPtr tail_opt; Span span; }
// - ExprPtr = std::shared_ptr<Expr>
//
// =============================================================================
// REFACTORING NOTES
// =============================================================================
// 1. This file handles ONLY the infinite loop case from ParseLoopTail.
//    The conditional and iterator cases are in separate files.
//
// 2. ParseLoopInvariantOpt is shared by all three loop types and could be
//    placed in a shared helper file (loop_common.cpp or similar).
//
// 3. The lookahead logic: if current token is "{" or "where", it's an infinite
//    loop. Otherwise, try iterator pattern, then fall back to conditional.
//
// 4. Export standalone functions:
//    ParseElemResult<ExprPtr> ParseLoopInfiniteExpr(Parser parser);
//    - Assumes parser is positioned AFTER the "loop" keyword
//    - Checks for "{" or "where" before proceeding
//
// 5. Shared helper to extract:
//    ParseElemResult<std::optional<LoopInvariant>> ParseLoopInvariantOpt(Parser parser);
//    - Used by all three loop types
//    - Can be in a loop_common.cpp or this file with external linkage
//
// 6. The LoopInvariant struct holds both the predicate expression and span.
//    The invariant block is `where { predicate }` with braces required.
// =============================================================================
