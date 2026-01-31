// =============================================================================
// MIGRATION MAPPING: match_expr.cpp
// =============================================================================
// This file implements parsing for match expressions, including match arms,
// patterns, guards, and arm bodies.
//
// =============================================================================
// SPEC REFERENCE: C0updated.md
// =============================================================================
//
// **(Parse-Match-Expr)** Lines 5254-5257:
// IsKw(Tok(P), `match`)    Γ ⊢ ParseExpr_NoBrace(Advance(P)) ⇓ (P_1, e)
// IsPunc(Tok(P_1), "{")    Γ ⊢ ParseMatchArms(Advance(P_1)) ⇓ (P_2, arms)
// IsPunc(Tok(P_2), "}")
// ──────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParsePrimary(P) ⇓ (Advance(P_2), MatchExpr(e, arms))
//
// **(Parse-MatchArms-Cons)** Lines 5903-5906:
// Γ ⊢ ParseMatchArm(P) ⇓ (P_1, a)    Γ ⊢ ParseMatchArmsTail(P_1, [a]) ⇓ (P_2, arms)
// ────────────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseMatchArms(P) ⇓ (P_2, arms)
//
// **(Parse-MatchArm)** Lines 5908-5911:
// Γ ⊢ ParsePattern(P) ⇓ (P_1, pat)    Γ ⊢ ParseGuardOpt(P_1) ⇓ (P_2, guard_opt)
// IsOp(Tok(P_2), "=>")    Γ ⊢ ParseArmBody(Advance(P_2)) ⇓ (P_3, body)
// ────────────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseMatchArm(P) ⇓ (P_3, ⟨pat, guard_opt, body⟩)
//
// **(Parse-MatchArmsTail-End)** Lines 5913-5916:
// Tok(P) = Punctuator("}")
// ──────────────────────────────────────────────────────────────
// Γ ⊢ ParseMatchArmsTail(P, xs) ⇓ (P, xs)
//
// **(Parse-MatchArmsTail-Comma)** Lines 5918-5921:
// IsPunc(Tok(P), ",")    Γ ⊢ ParseMatchArm(Advance(P)) ⇓ (P_1, a)
// Γ ⊢ ParseMatchArmsTail(P_1, xs ++ [a]) ⇓ (P_2, ys)
// ────────────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseMatchArmsTail(P, xs) ⇓ (P_2, ys)
//
// **(Parse-GuardOpt-None)** Lines 5923-5926:
// ¬ IsKw(Tok(P), `if`)
// ──────────────────────────────────────────────
// Γ ⊢ ParseGuardOpt(P) ⇓ (P, ⊥)
//
// **(Parse-GuardOpt-Yes)** Lines 5928-5931:
// IsKw(Tok(P), `if`)    Γ ⊢ ParseExpr(Advance(P)) ⇓ (P_1, e)
// ────────────────────────────────────────────────────────────────
// Γ ⊢ ParseGuardOpt(P) ⇓ (P_1, e)
//
// **(Parse-ArmBody-Block)** Lines 5933-5936:
// IsPunc(Tok(P), "{")    Γ ⊢ ParseBlock(P) ⇓ (P_1, b)
// ──────────────────────────────────────────────────────────────
// Γ ⊢ ParseArmBody(P) ⇓ (P_1, b)
//
// **(Parse-ArmBody-Expr)** Lines 5938-5941:
// Γ ⊢ ParseExpr(P) ⇓ (P_1, e)
// ──────────────────────────────────────────────
// Γ ⊢ ParseArmBody(P) ⇓ (P_1, e)
//
// SEMANTICS:
// - Match expression: scrutinee expression followed by braced arms
// - Arms are comma-separated (not semicolon or newline alone)
// - Each arm: pattern => body with optional guard (if condition)
// - Arm body can be block { ... } or expression
// - Trailing comma allowed only when } is on different line
// - Newlines can also separate arms (bootstrap extension)
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_expr.cpp
// =============================================================================
//
// 1. RESULT TYPES
//    Source lines: 402-422
//    -------------------------------------------------------------------------
//    struct MatchArmsResult {
//      Parser parser;
//      std::vector<MatchArm> arms;
//    };
//
//    struct GuardOptResult {
//      Parser parser;
//      ExprPtr guard_opt;
//    };
//
//    struct ArmBodyResult {
//      Parser parser;
//      ExprPtr body;
//    };
//
// 2. FORWARD DECLARATIONS
//    Source lines: 407-408, 415, 422
//    -------------------------------------------------------------------------
//    MatchArmsResult ParseMatchArms(Parser parser);
//    MatchArmsResult ParseMatchArmsTail(Parser parser, std::vector<MatchArm> xs);
//    GuardOptResult ParseGuardOpt(Parser parser);
//    ArmBodyResult ParseArmBody(Parser parser);
//
// 3. MATCH EXPRESSION PARSING (within ParsePrimary)
//    Source lines: 1456-1481
//    -------------------------------------------------------------------------
//    if (tok && IsKwTok(*tok, "match")) {
//      SPEC_RULE("Parse-Match-Expr");
//      Parser next = parser;
//      Advance(next);
//      ParseElemResult<ExprPtr> value = ParseExprNoBrace(next);
//      if (!IsPunc(value.parser, "{")) {
//        EmitParseSyntaxErr(value.parser, TokSpan(value.parser));
//        Parser sync = value.parser;
//        SyncStmt(sync);
//        return {sync, MakeExpr(SpanBetween(parser, sync), ErrorExpr{})};
//      }
//      Parser after_l = value.parser;
//      Advance(after_l);
//      MatchArmsResult arms = ParseMatchArms(after_l);
//      if (!IsPunc(arms.parser, "}")) {
//        EmitParseSyntaxErr(arms.parser, TokSpan(arms.parser));
//        Parser sync = arms.parser;
//        SyncStmt(sync);
//        return {sync, MakeExpr(SpanBetween(parser, sync), ErrorExpr{})};
//      }
//      Parser after = arms.parser;
//      Advance(after);
//      MatchExpr match;
//      match.value = value.elem;
//      match.arms = std::move(arms.arms);
//      return {after, MakeExpr(SpanBetween(parser, after), match)};
//    }
//
// 4. ParseMatchArms IMPLEMENTATION
//    Source lines: 2286-2312
//    -------------------------------------------------------------------------
//    MatchArmsResult ParseMatchArms(Parser parser) {
//      // Skip leading newlines
//      while (Tok(parser) && Tok(parser)->kind == TokenKind::Newline) {
//        Advance(parser);
//      }
//      SPEC_RULE("Parse-MatchArms-Cons");
//      SPEC_RULE("Parse-MatchArm");
//      ParseElemResult<std::shared_ptr<Pattern>> pat = ParsePattern(parser);
//      GuardOptResult guard = ParseGuardOpt(pat.parser);
//      if (!IsOp(guard.parser, "=>")) {
//        EmitParseSyntaxErr(guard.parser, TokSpan(guard.parser));
//        Parser sync = guard.parser;
//        SyncStmt(sync);
//        return {sync, {}};
//      }
//      Parser after_arrow = guard.parser;
//      Advance(after_arrow);
//      ArmBodyResult body = ParseArmBody(after_arrow);
//      MatchArm arm;
//      arm.pattern = pat.elem;
//      arm.guard_opt = guard.guard_opt;
//      arm.body = body.body;
//      arm.span = SpanBetween(parser, body.parser);
//      std::vector<MatchArm> arms;
//      arms.push_back(arm);
//      return ParseMatchArmsTail(body.parser, std::move(arms));
//    }
//
// 5. ParseMatchArmsTail IMPLEMENTATION
//    Source lines: 2314-2391
//    -------------------------------------------------------------------------
//    MatchArmsResult ParseMatchArmsTail(Parser parser, std::vector<MatchArm> xs) {
//      // Skip newlines (they act as arm separators)
//      while (Tok(parser) && Tok(parser)->kind == TokenKind::Newline) {
//        Advance(parser);
//      }
//
//      if (IsPunc(parser, "}")) {
//        SPEC_RULE("Parse-MatchArmsTail-End");
//        return {parser, xs};
//      }
//      if (IsPunc(parser, ",")) {
//        const TokenKindMatch end_set[] = {MatchPunct("}")};
//        Parser after = parser;
//        Advance(after);
//        // Skip newlines after comma
//        while (Tok(after) && Tok(after)->kind == TokenKind::Newline) {
//          Advance(after);
//        }
//        if (IsPunc(after, "}")) {
//          SPEC_RULE("Parse-MatchArmsTail-TrailingComma");
//          EmitTrailingCommaErr(parser, end_set);
//          after.diags = parser.diags;
//          return {after, xs};
//        }
//        SPEC_RULE("Parse-MatchArmsTail-Comma");
//        SPEC_RULE("Parse-MatchArm");
//        // ... parse pattern, guard, arrow, body ...
//        return ParseMatchArmsTail(body.parser, std::move(xs));
//      }
//      // No comma - check if next token starts a new arm (newline-separated)
//      // Patterns start with: identifier, @, literal, (, [, _
//      const Token* tok = Tok(parser);
//      if (tok && (tok->kind == TokenKind::Identifier || ...)) {
//        SPEC_RULE("Parse-MatchArmsTail-Newline");
//        SPEC_RULE("Parse-MatchArm");
//        // ... parse arm ...
//        return ParseMatchArmsTail(body.parser, std::move(xs));
//      }
//      EmitParseSyntaxErr(parser, TokSpan(parser));
//      return {parser, xs};
//    }
//
// 6. ParseGuardOpt IMPLEMENTATION
//    Source lines: 2393-2403
//    -------------------------------------------------------------------------
//    GuardOptResult ParseGuardOpt(Parser parser) {
//      if (!IsKw(parser, "if")) {
//        SPEC_RULE("Parse-GuardOpt-None");
//        return {parser, nullptr};
//      }
//      SPEC_RULE("Parse-GuardOpt-Yes");
//      Parser next = parser;
//      Advance(next);
//      ParseElemResult<ExprPtr> guard = ParseExpr(next);
//      return {guard.parser, guard.elem};
//    }
//
// 7. ParseArmBody IMPLEMENTATION
//    Source lines: 2405-2416
//    -------------------------------------------------------------------------
//    ArmBodyResult ParseArmBody(Parser parser) {
//      if (IsPunc(parser, "{")) {
//        SPEC_RULE("Parse-ArmBody-Block");
//        ParseElemResult<std::shared_ptr<Block>> block = ParseBlock(parser);
//        BlockExpr blk;
//        blk.block = block.elem;
//        return {block.parser, MakeExpr(SpanBetween(parser, block.parser), blk)};
//      }
//      SPEC_RULE("Parse-ArmBody-Expr");
//      ParseElemResult<ExprPtr> expr = ParseExpr(parser);
//      return {expr.parser, expr.elem};
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
// - IsOp(Parser, string_view) -> bool
// - SyncStmt(Parser&) -> void
// - EmitParseSyntaxErr(Parser&, Span) -> void
// - EmitTrailingCommaErr(Parser&, span-of<TokenKindMatch>) -> void
//
// FROM block_expr.cpp (or parser_stmt.cpp):
// - ParseBlock(Parser) -> ParseElemResult<std::shared_ptr<Block>>
//
// FROM binary.cpp (or expr entry):
// - ParseExpr(Parser) -> ParseElemResult<ExprPtr>
// - ParseExprNoBrace(Parser) -> ParseElemResult<ExprPtr>
//
// FROM pattern/*.cpp:
// - ParsePattern(Parser) -> ParseElemResult<std::shared_ptr<Pattern>>
//
// FROM AST types:
// - MatchExpr struct { ExprPtr value; std::vector<MatchArm> arms; }
// - MatchArm struct { PatternPtr pattern; ExprPtr guard_opt; ExprPtr body; Span span; }
// - BlockExpr struct { std::shared_ptr<Block> block; }
// - ErrorExpr struct {}
// - ExprPtr = std::shared_ptr<Expr>
// - PatternPtr = std::shared_ptr<Pattern>
//
// =============================================================================
// REFACTORING NOTES
// =============================================================================
// 1. The bootstrap supports newline-separated arms as an extension. The spec
//    requires comma separation. Decide which behavior to preserve.
//
// 2. Trailing comma detection uses EmitTrailingCommaErr which checks line
//    differences. This logic should be preserved for spec compliance.
//
// 3. The arm body wraps block in BlockExpr node for uniform AST representation.
//
// 4. Consider whether to keep the newline-as-separator logic or strictly
//    require commas per spec.
//
// 5. Export standalone functions:
//    ParseElemResult<ExprPtr> ParseMatchExpr(Parser parser);
//    MatchArmsResult ParseMatchArms(Parser parser);
//    GuardOptResult ParseGuardOpt(Parser parser);
//    ArmBodyResult ParseArmBody(Parser parser);
//
// 6. The pattern can start with: identifier, @, _, (, [, or literal tokens.
//    This lookahead logic enables newline-separated arm parsing.
// =============================================================================
