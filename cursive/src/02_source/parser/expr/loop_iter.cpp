// =============================================================================
// MIGRATION MAPPING: loop_iter.cpp
// =============================================================================
// This file implements parsing for iterator loop expressions
// (loop pattern in iterable { body } - equivalent to for-each loops).
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
// **(Parse-LoopTail-Iter)** Lines 5996-5999:
// Γ ⊢ TryParsePatternIn(P) ⇓ (P_1, pat)
// Γ ⊢ ParseTypeAnnotOpt(P_1) ⇓ (P_2, ty_opt)
// IsIdent(Tok(P_2))    Lexeme(Tok(P_2)) = `in`
// Γ ⊢ ParseExpr_NoBrace(Advance(P_2)) ⇓ (P_3, iter)
// Γ ⊢ ParseLoopInvariantOpt(P_3) ⇓ (P_4, inv_opt)
// Γ ⊢ ParseBlock(P_4) ⇓ (P_5, body)
// ────────────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseLoopTail(P) ⇓ (P_5, LoopIter(pat, ty_opt, iter, inv_opt, body))
//
// **(TryParsePatternIn-Ok)** Lines 6006-6009:
// P_c = Clone(P)    Γ ⊢ ParsePattern(P_c) ⇓ (P_1, pat)
// Γ ⊢ ParseTypeAnnotOpt(P_1) ⇓ (P_2, ty_opt)
// IsIdent(Tok(P_2))    Lexeme(Tok(P_2)) = `in`
// P' = MergeDiag(P, P_2, P_1)
// ────────────────────────────────────────────────────────────────────────────────────
// Γ ⊢ TryParsePatternIn(P) ⇓ (P', pat)
//
// **(TryParsePatternIn-Fail)** Lines 6011-6014:
// P_c = Clone(P)
// ¬ (Γ ⊢ ParsePattern(P_c) ⇓ (P_1, pat) ∧ ... ∧ Lexeme(Tok(P_2)) = `in`)
// ────────────────────────────────────────────────────────────────────────────────────
// Γ ⊢ TryParsePatternIn(P) ↑
//
// SEMANTICS:
// - Iterator loop: `loop pattern in iterable { body }`
// - Pattern binds loop variable(s) for each iteration
// - Optional type annotation: `loop x: i32 in range { ... }`
// - `in` is a contextual keyword (valid identifier elsewhere)
// - Iterable is parsed with ParseExpr_NoBrace (no brace in iterable expr)
// - Optional invariant clause: `where { predicate }`
// - Body is a block expression (braces required)
//
// DISAMBIGUATION (TryParsePatternIn):
// - Uses speculative parsing (Clone) to check for pattern + "in"
// - If pattern followed by "in" keyword -> iterator loop
// - Otherwise -> fall back to conditional loop
// - MergeDiag preserves diagnostics from successful speculative parse
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_expr.cpp
// =============================================================================
//
// 1. RESULT TYPE: TryPatternInResult
//    Source lines: 431-436
//    -------------------------------------------------------------------------
//    struct TryPatternInResult {
//      Parser parser;
//      std::shared_ptr<Pattern> pattern;
//      bool ok = false;
//    };
//
// 2. FORWARD DECLARATION
//    Source line: 437
//    -------------------------------------------------------------------------
//    TryPatternInResult TryParsePatternIn(Parser parser);
//
// 3. TryParsePatternIn IMPLEMENTATION
//    Source lines: 2418-2438
//    -------------------------------------------------------------------------
//    TryPatternInResult TryParsePatternIn(Parser parser) {
//      Parser clone = Clone(parser);
//      ParseElemResult<std::shared_ptr<Pattern>> pat = ParsePattern(clone);
//      ParseElemResult<std::shared_ptr<Type>> ty = ParseTypeAnnotOpt(pat.parser);
//      const Token* tok = Tok(ty.parser);
//      if (tok && IsIdentTok(*tok) && tok->lexeme == "in") {
//        SPEC_RULE("TryParsePatternIn-Ok");
//        TryPatternInResult out;
//        out.parser = MergeDiag(parser, ty.parser, pat.parser);
//        out.pattern = pat.elem;
//        out.ok = true;
//        return out;
//      }
//      SPEC_RULE("TryParsePatternIn-Fail");
//      TryPatternInResult out;
//      out.parser = parser;
//      out.pattern = nullptr;
//      out.ok = false;
//      return out;
//    }
//
// 4. ParseLoopTail - ITERATOR LOOP CASE
//    Source lines: 2483-2509
//    -------------------------------------------------------------------------
//    TryPatternInResult try_in = TryParsePatternIn(parser);
//    if (try_in.ok) {
//      SPEC_RULE("Parse-LoopTail-Iter");
//      ParseElemResult<std::shared_ptr<Type>> ty = ParseTypeAnnotOpt(try_in.parser);
//      const Token* tok = Tok(ty.parser);
//      if (!tok || !IsIdentTok(*tok) || tok->lexeme != "in") {
//        EmitParseSyntaxErr(ty.parser, TokSpan(ty.parser));
//        Parser sync = ty.parser;
//        SyncStmt(sync);
//        return {sync, MakeExpr(SpanBetween(parser, sync), ErrorExpr{})};
//      }
//      Parser after_in = ty.parser;
//      Advance(after_in);
//      ParseElemResult<ExprPtr> iter = ParseExprNoBrace(after_in);
//      ParseElemResult<std::optional<LoopInvariant>> inv = ParseLoopInvariantOpt(iter.parser);
//      ParseElemResult<std::shared_ptr<Block>> body = ParseBlock(inv.parser);
//      auto pattern = try_in.pattern;
//      auto type_opt = ty.elem;
//      NormalizeBindingPattern(pattern, type_opt);
//      LoopIterExpr loop;
//      loop.pattern = pattern;
//      loop.type_opt = type_opt;
//      loop.iter = iter.elem;
//      loop.invariant_opt = inv.elem;
//      loop.body = body.elem;
//      return {body.parser, MakeExpr(SpanBetween(parser, body.parser), loop)};
//    }
//
// 5. FULL ParseLoopTail IMPLEMENTATION (for context)
//    Source lines: 2473-2519
//    -------------------------------------------------------------------------
//    LoopTailResult ParseLoopTail(Parser parser) {
//      // Case 1: Infinite loop (starts with { or where)
//      if (IsPunc(parser, "{") || IsKw(parser, "where")) {
//        SPEC_RULE("Parse-LoopTail-Infinite");
//        // ... infinite loop handling ...
//      }
//
//      // Case 2: Iterator loop (pattern ... in iterable)
//      TryPatternInResult try_in = TryParsePatternIn(parser);
//      if (try_in.ok) {
//        SPEC_RULE("Parse-LoopTail-Iter");
//        // ... iterator loop handling (this file) ...
//      }
//
//      // Case 3: Conditional loop (fallback)
//      SPEC_RULE("Parse-LoopTail-Cond");
//      // ... conditional loop handling ...
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
// - Clone(Parser) -> Parser
// - MergeDiag(Parser, Parser, Parser) -> Parser
// - IsIdentTok(Token) -> bool
// - SyncStmt(Parser&) -> void
// - EmitParseSyntaxErr(Parser&, Span) -> void
//
// FROM loop_infinite.cpp (or loop_common.cpp):
// - ParseLoopInvariantOpt(Parser) -> ParseElemResult<std::optional<LoopInvariant>>
//
// FROM block_expr.cpp (or parser_stmt.cpp):
// - ParseBlock(Parser) -> ParseElemResult<std::shared_ptr<Block>>
//
// FROM binary.cpp (or expr entry):
// - ParseExprNoBrace(Parser) -> ParseElemResult<ExprPtr>
//
// FROM pattern/*.cpp:
// - ParsePattern(Parser) -> ParseElemResult<std::shared_ptr<Pattern>>
//
// FROM type/*.cpp:
// - ParseTypeAnnotOpt(Parser) -> ParseElemResult<std::shared_ptr<Type>>
//
// FROM pattern/normalize.cpp (or similar):
// - NormalizeBindingPattern(Pattern&, Type&) -> void
//   (Moves type annotation from pattern into separate type_opt field)
//
// FROM AST types:
// - LoopIterExpr struct {
//     std::shared_ptr<Pattern> pattern;
//     std::shared_ptr<Type> type_opt;
//     ExprPtr iter;
//     std::optional<LoopInvariant> invariant_opt;
//     std::shared_ptr<Block> body;
//   }
// - LoopInvariant struct { ExprPtr predicate; Span span; }
// - Block struct { std::vector<Stmt> stmts; ExprPtr tail_opt; Span span; }
// - Pattern (various pattern types)
// - Type (various type AST nodes)
// - ErrorExpr struct {}
// - ExprPtr = std::shared_ptr<Expr>
//
// =============================================================================
// REFACTORING NOTES
// =============================================================================
// 1. TryParsePatternIn uses speculative parsing via Clone(). This is critical
//    for disambiguation between iterator and conditional loops.
//
// 2. The MergeDiag function preserves diagnostics from the speculative parse.
//    This ensures errors found during pattern parsing are not lost.
//
// 3. "in" is a contextual keyword - it's a valid identifier in other contexts.
//    The check uses IsIdentTok() && lexeme == "in", not IsKw().
//
// 4. NormalizeBindingPattern moves type annotation from pattern to type_opt.
//    This handles cases like `loop x: i32 in range` where the type annotation
//    might initially be parsed as part of the pattern.
//
// 5. Export standalone functions:
//    TryPatternInResult TryParsePatternIn(Parser parser);
//    ParseElemResult<ExprPtr> ParseLoopIterExpr(Parser parser, TryPatternInResult try_in);
//    - Second function assumes TryParsePatternIn already succeeded
//
// 6. The type annotation is parsed AFTER TryParsePatternIn succeeds but before
//    the "in" keyword is consumed. This allows both:
//    - `loop x in range { ... }`
//    - `loop x: i32 in range { ... }`
//
// 7. Consider whether TryParsePatternIn should be in this file or a shared
//    loop_common.cpp since it's the disambiguation mechanism.
// =============================================================================
