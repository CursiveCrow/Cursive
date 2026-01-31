// =============================================================================
// MIGRATION MAPPING: call.cpp
// =============================================================================
// This file contains parsing logic for function call expressions: expr(args).
// Method calls and calls with type arguments are in separate files.
//
// SPEC REFERENCE: C0updated.md, Lines 5428-5431, 5448-5473, 5943-5958
// -----------------------------------------------------------------------------
// **(Postfix-Call)** Lines 5428-5431
// IsPunc(Tok(P), "(")    Γ ⊢ ParseArgList(Advance(P)) ⇓ (P_1, args)    IsPunc(Tok(P_1), ")")
// ────────────────────────────────────────────────────────────────────────────────────────────
// Γ ⊢ PostfixStep(P, e) ⇓ (Advance(P_1), Call(e, args))
//
// ARGUMENT LIST RULES (Lines 5448-5473):
// **(Parse-ArgList-Empty)** Lines 5450-5453
// IsPunc(Tok(P), ")")
// ──────────────────────────────────────────────
// Γ ⊢ ParseArgList(P) ⇓ (P, [])
//
// **(Parse-ArgList-Cons)** Lines 5455-5458
// Γ ⊢ ParseArg(P) ⇓ (P_1, a)    Γ ⊢ ParseArgTail(P_1, [a]) ⇓ (P_2, args)
// ────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseArgList(P) ⇓ (P_2, args)
//
// **(Parse-Arg)** Lines 5460-5463
// Γ ⊢ ParseArgMoveOpt(P) ⇓ (P_1, moved)    Γ ⊢ ParseExpr(P_1) ⇓ (P_2, e)
// ──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseArg(P) ⇓ (P_2, ⟨moved, e, SpanBetween(P, P_2)⟩)
//
// **(Parse-ArgMoveOpt-None)** Lines 5465-5468
// ¬ IsKw(Tok(P), `move`)
// ──────────────────────────────────────────────────────────────
// Γ ⊢ ParseArgMoveOpt(P) ⇓ (P, false)
//
// **(Parse-ArgMoveOpt-Yes)** Lines 5470-5473
// IsKw(Tok(P), `move`)
// ──────────────────────────────────────────────────────
// Γ ⊢ ParseArgMoveOpt(P) ⇓ (Advance(P), true)
//
// ARG TAIL RULES (Lines 5943-5958):
// **(Parse-ArgTail-End)** Lines 5945-5948
// IsPunc(Tok(P), ")")
// ───────────────────────────────────────────
// Γ ⊢ ParseArgTail(P, xs) ⇓ (P, xs)
//
// **(Parse-ArgTail-TrailingComma)** Lines 5950-5953
// IsPunc(Tok(P), ",")    IsPunc(Tok(Advance(P)), ")")    TrailingCommaAllowed(P_0, P, {Punctuator(")")})
// ────────────────────────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseArgTail(P, xs) ⇓ (Advance(P), xs)
//
// **(Parse-ArgTail-Comma)** Lines 5955-5958
// IsPunc(Tok(P), ",")    Γ ⊢ ParseArg(Advance(P)) ⇓ (P_1, a)    Γ ⊢ ParseArgTail(P_1, xs ++ [a]) ⇓ (P_2, ys)
// ────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseArgTail(P, xs) ⇓ (P_2, ys)
//
// SEMANTICS:
// - Function calls: expr(arg1, arg2, ...)
// - Callee expression is evaluated first, then arguments left-to-right
// - Arguments are comma-separated, optionally prefixed with "move"
// - Trailing commas allowed ONLY when closing ")" is on different line
// - Empty argument lists are valid: expr()
//
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_expr.cpp
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
// 1. Postfix Call Branch (in PostfixStep function)
//    Source: parser_expr.cpp, lines 787-805
//    ```cpp
//    if (IsPunc(parser, "(")) {
//      SPEC_RULE("Postfix-Call");
//      Parser next = parser;
//      Advance(next);
//      ParseElemResult<std::vector<Arg>> args = ParseArgList(next);
//      if (!IsPunc(args.parser, ")")) {
//        EmitParseSyntaxErr(args.parser, TokSpan(args.parser));
//        Parser sync = args.parser;
//        SyncStmt(sync);
//        return {sync, MakeExpr(SpanBetween(parser, sync), ErrorExpr{})};
//      }
//      core::Span end_span = TokSpan(args.parser);
//      Parser after = args.parser;
//      Advance(after);
//      CallExpr call;
//      call.callee = expr;
//      call.args = std::move(args.elem);
//      return {after, MakeExpr(SpanCover(expr->span, end_span), call)};
//    }
//    ```
//
// 2. ParseArgList function
//    Source: parser_expr.cpp, lines 1926-1937
//    ```cpp
//    ParseElemResult<std::vector<Arg>> ParseArgList(Parser parser) {
//      SkipNewlines(parser);
//      if (IsPunc(parser, ")")) {
//        SPEC_RULE("Parse-ArgList-Empty");
//        return {parser, {}};
//      }
//      SPEC_RULE("Parse-ArgList-Cons");
//      ParseElemResult<Arg> first = ParseArg(parser);
//      std::vector<Arg> args;
//      args.push_back(first.elem);
//      return ParseArgTail(first.parser, std::move(args));
//    }
//    ```
//
// 3. ParseArg function
//    Source: parser_expr.cpp, lines 1939-1949
//    ```cpp
//    ParseElemResult<Arg> ParseArg(Parser parser) {
//      SPEC_RULE("Parse-Arg");
//      Parser start = parser;
//      ParseElemResult<bool> moved = ParseArgMoveOpt(parser);
//      ParseElemResult<ExprPtr> expr = ParseExpr(moved.parser);
//      Arg arg;
//      arg.moved = moved.elem;
//      arg.value = expr.elem;
//      arg.span = SpanBetween(start, expr.parser);
//      return {expr.parser, arg};
//    }
//    ```
//
// 4. ParseArgMoveOpt function
//    Source: parser_expr.cpp, lines 1951-1960
//    ```cpp
//    ParseElemResult<bool> ParseArgMoveOpt(Parser parser) {
//      if (!IsKw(parser, "move")) {
//        SPEC_RULE("Parse-ArgMoveOpt-None");
//        return {parser, false};
//      }
//      SPEC_RULE("Parse-ArgMoveOpt-Yes");
//      Parser next = parser;
//      Advance(next);
//      return {next, true};
//    }
//    ```
//
// 5. ParseArgTail function
//    Source: parser_expr.cpp, lines 1962-1987
//    ```cpp
//    ParseElemResult<std::vector<Arg>> ParseArgTail(Parser parser,
//                                                   std::vector<Arg> xs) {
//      SkipNewlines(parser);
//      if (IsPunc(parser, ")")) {
//        SPEC_RULE("Parse-ArgTail-End");
//        return {parser, xs};
//      }
//      if (IsPunc(parser, ",")) {
//        const std::array<TokenKindMatch, 1> end_set = {MatchPunct(")")};
//        Parser after = parser;
//        Advance(after);
//        SkipNewlines(after);
//        if (IsPunc(after, ")")) {
//          SPEC_RULE("Parse-ArgTail-TrailingComma");
//          EmitTrailingCommaErr(parser, end_set);
//          after.diags = parser.diags;
//          return {after, xs};
//        }
//        SPEC_RULE("Parse-ArgTail-Comma");
//        ParseElemResult<Arg> arg = ParseArg(after);
//        xs.push_back(arg.elem);
//        return ParseArgTail(arg.parser, std::move(xs));
//      }
//      EmitParseSyntaxErr(parser, TokSpan(parser));
//      return {parser, xs};
//    }
//    ```
//
// AST DEFINITIONS (from ast.h):
// ```cpp
// struct Arg {           // lines 194-198
//   bool moved = false;
//   ExprPtr value;
//   core::Span span;
// };
//
// struct CallExpr {      // lines 484-488
//   ExprPtr callee;
//   std::vector<std::shared_ptr<Type>> generic_args;  // empty for simple calls
//   std::vector<Arg> args;
// };
// ```
//
// DEPENDENCIES:
// - Requires: ParseExpr (for argument expression parsing)
// - Requires: Arg, CallExpr AST node types
// - Requires: MakeExpr, SpanCover, SpanBetween helpers
// - Requires: IsPunc, IsKw, Tok, TokSpan, Advance helpers
// - Requires: SkipNewlines (for multi-line arg lists)
// - Requires: EmitParseSyntaxErr, EmitTrailingCommaErr, SyncStmt
// - Requires: TokenKindMatch, MatchPunct for trailing comma detection
//
// REFACTORING NOTES:
// - SkipNewlines is called at start of ParseArgList AND ParseArgTail
// - Trailing comma handling: EmitTrailingCommaErr is called but parsing continues
// - Note: after.diags = parser.diags propagates diagnostic after trailing comma
// - Error recovery uses SyncStmt to skip to statement boundary on missing ")"
// - Span for each Arg covers from move keyword (if present) to expression end
// - Span for full CallExpr covers from callee start to closing ")"
// - The generic_args field on CallExpr is empty for simple calls (no type args)
// =============================================================================
