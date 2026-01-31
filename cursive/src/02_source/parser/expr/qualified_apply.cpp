// =============================================================================
// MIGRATION MAPPING: qualified_apply.cpp
// =============================================================================
// This file implements parsing for qualified apply expressions
// (module::path::name followed by call parens or record braces).
// These represent function calls or record construction with qualified paths.
//
// =============================================================================
// SPEC REFERENCE: C0updated.md
// =============================================================================
// Lines 5194-5197:
//
// **(Parse-Qualified-Apply-Paren)**
// Γ ⊢ ParseQualifiedHead(P) ⇓ (P_1, path, name)
//     IsPunc(Tok(P_1), "(")
//     Γ ⊢ ParseArgList(Advance(P_1)) ⇓ (P_2, args)
//     IsPunc(Tok(P_2), ")")
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParsePrimary(P) ⇓ (Advance(P_2), QualifiedApply(path, name, Paren(args)))
//
// SEMANTICS (Paren):
// - Parse qualified path via ParseQualifiedHead
// - Token after head must be "("
// - Parse argument list inside parentheses via ParseArgList
// - Closing ")" required
// - Returns QualifiedApplyExpr with ParenArgs
//
// Lines 5199-5202:
//
// **(Parse-Qualified-Apply-Brace)**
// Γ ⊢ ParseQualifiedHead(P) ⇓ (P_1, path, name)
//     IsPunc(Tok(P_1), "{")
//     Γ ⊢ ParseFieldInitList(Advance(P_1)) ⇓ (P_2, fields)
//     IsPunc(Tok(P_2), "}")
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParsePrimary(P) ⇓ (Advance(P_2), QualifiedApply(path, name, Brace(fields)))
//
// SEMANTICS (Brace):
// - Parse qualified path via ParseQualifiedHead
// - Token after head must be "{"
// - Parse field initializer list inside braces via ParseFieldInitList
// - Closing "}" required
// - Returns QualifiedApplyExpr with BraceArgs
// - NOTE: Only allowed when allow_brace=true (see RuleSet definitions line 5230)
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_expr.cpp
// =============================================================================
//
// 1. QUALIFIED APPLY WITH PARENTHESES
//    Source lines: 1156-1173
//    -------------------------------------------------------------------------
//       if (after_head && IsPuncTok(*after_head, "(")) {
//         SPEC_RULE("Parse-Qualified-Apply-Paren");
//         Parser after_l = head.parser;
//         Advance(after_l);
//         ParseElemResult<std::vector<Arg>> args = ParseArgList(after_l);
//         if (!IsPunc(args.parser, ")")) {
//           EmitParseSyntaxErr(args.parser, TokSpan(args.parser));
//           Parser sync = args.parser;
//           SyncStmt(sync);
//           return {sync, MakeExpr(SpanBetween(parser, sync), ErrorExpr{})};
//         }
//         Parser after = args.parser;
//         Advance(after);
//         QualifiedApplyExpr app;
//         app.path = head.module_path;
//         app.name = head.name;
//         app.args = ParenArgs{std::move(args.elem)};
//         return {after, MakeExpr(SpanBetween(parser, after), app)};
//       }
//
// 2. QUALIFIED APPLY WITH BRACES
//    Source lines: 1175-1193
//    -------------------------------------------------------------------------
//       if (allow_brace && after_head && IsPuncTok(*after_head, "{")) {
//         SPEC_RULE("Parse-Qualified-Apply-Brace");
//         Parser after_l = head.parser;
//         Advance(after_l);
//         ParseElemResult<std::vector<FieldInit>> fields = ParseFieldInitList(after_l);
//         if (!IsPunc(fields.parser, "}")) {
//           EmitParseSyntaxErr(fields.parser, TokSpan(fields.parser));
//           Parser sync = fields.parser;
//           SyncStmt(sync);
//           return {sync, MakeExpr(SpanBetween(parser, sync), ErrorExpr{})};
//         }
//         Parser after = fields.parser;
//         Advance(after);
//         QualifiedApplyExpr app;
//         app.path = head.module_path;
//         app.name = head.name;
//         app.args = BraceArgs{std::move(fields.elem)};
//         return {after, MakeExpr(SpanBetween(parser, after), app)};
//       }
//
// =============================================================================
// DEPENDENCIES
// =============================================================================
// FROM expr_common.cpp:
// - MakeExpr(Span, ExprNode) -> ExprPtr
// - SpanBetween(Parser, Parser) -> Span
//
// FROM parser_paths.cpp:
// - ParseQualifiedHead(Parser) -> ParseQualifiedHeadResult
// - ParseQualifiedHeadResult struct { Parser parser; ModulePath module_path; Identifier name; }
//
// FROM call.cpp (or argument parsing):
// - ParseArgList(Parser) -> ParseElemResult<std::vector<Arg>>
// - Arg struct (positional or named argument)
//
// FROM record_literal.cpp (or field init parsing):
// - ParseFieldInitList(Parser) -> ParseElemResult<std::vector<FieldInit>>
// - FieldInit struct { Identifier name; ExprPtr value; }
//
// FROM parser_common.cpp:
// - Parser state type
// - Advance(Parser&) -> void
// - Tok(Parser) -> const Token*
// - IsPunc(Parser, const char*) -> bool
// - SyncStmt(Parser&) -> void
// - TokSpan(Parser) -> Span
//
// FROM lexer/token helpers:
// - IsPuncTok(Token, const char*) -> bool
//
// FROM error emission:
// - EmitParseSyntaxErr(Parser, Span) -> void
//
// FROM AST types:
// - QualifiedApplyExpr struct { ModulePath path; Identifier name; ApplyArgs args; }
// - ApplyArgs = std::variant<ParenArgs, BraceArgs>
// - ParenArgs struct { std::vector<Arg> args; }
// - BraceArgs struct { std::vector<FieldInit> fields; }
// - ErrorExpr struct { }
// - ModulePath = std::vector<Identifier>
// - Identifier = std::string
// - ExprPtr = std::shared_ptr<Expr>
//
// =============================================================================
// REFACTORING NOTES
// =============================================================================
// 1. Export functions for use after ParseQualifiedHead has been called:
//
//    a) For paren case (always allowed):
//       ParseElemResult<ExprPtr> ParseQualifiedApplyParen(
//           Parser original_parser,
//           const ParseQualifiedHeadResult& head);
//       Assumes IsPuncTok(*Tok(head.parser), "(") is true.
//
//    b) For brace case (controlled by allow_brace):
//       ParseElemResult<ExprPtr> ParseQualifiedApplyBrace(
//           Parser original_parser,
//           const ParseQualifiedHeadResult& head);
//       Assumes IsPuncTok(*Tok(head.parser), "{") is true.
//
//    c) Combined try-parse after head:
//       std::optional<ParseElemResult<ExprPtr>> TryBuildQualifiedApplyExpr(
//           Parser original_parser,
//           const ParseQualifiedHeadResult& head,
//           bool allow_brace);
//       Returns std::nullopt if neither "(" nor "{" follows.
//
// 2. Error handling pattern:
//    - If closing paren/brace is missing, emit syntax error
//    - Call SyncStmt to skip to a safe recovery point
//    - Return ErrorExpr
//
// 3. The allow_brace parameter is checked ONLY for the brace case.
//    The paren case is always allowed regardless of allow_brace.
//
// 4. Consider: ParseArgList and ParseFieldInitList might need to be
//    accessible from this file. Check if they should be in shared
//    utilities or imported as dependencies.
//
// 5. The span for the resulting expression spans from the original
//    parser position (before qualified head) to after the closing
//    delimiter, ensuring the full expression is covered.
// =============================================================================
