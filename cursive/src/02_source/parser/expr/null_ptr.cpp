// =============================================================================
// MIGRATION MAPPING: null_ptr.cpp
// =============================================================================
// This file implements parsing for the Ptr::null() expression, which creates
// a null safe pointer value.
//
// =============================================================================
// SPEC REFERENCE: C0updated.md
// =============================================================================
// Lines 5163-5166:
//
// **(Parse-Null-Ptr)**
// IsIdent(Tok(P))
//     Lexeme(Tok(P)) = `Ptr`
//     IsOp(Tok(Advance(P)), "::")
//     Tok(Advance(Advance(P))).kind = NullLiteral
//     IsPunc(Tok(Advance(Advance(Advance(P)))), "(")
//     IsPunc(Tok(Advance(Advance(Advance(Advance(P))))), ")")
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParsePrimary(P) ⇓ (Advance(Advance(Advance(Advance(Advance(P))))), PtrNullExpr)
//
// SEMANTICS:
// - Token sequence must be exactly: Ptr :: null ( )
// - First token: identifier with lexeme "Ptr"
// - Second token: operator "::"
// - Third token: NullLiteral (the `null` keyword)
// - Fourth token: punctuator "("
// - Fifth token: punctuator ")"
// - Returns PtrNullExpr node
// - Parser advances past all 5 tokens
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_expr.cpp
// =============================================================================
//
// Source lines: 1058-1081
// -------------------------------------------------------------------------
//   if (tok && IsIdentTok(*tok) && tok->lexeme == "Ptr") {
//     Parser next = parser;
//     Advance(next);
//     if (IsOp(next, "::")) {
//       Parser after_colon = next;
//       Advance(after_colon);
//       const Token* lit = Tok(after_colon);
//       Parser after_lit = after_colon;
//       if (lit && lit->kind == TokenKind::NullLiteral) {
//         Advance(after_lit);
//         if (IsPunc(after_lit, "(")) {
//           Parser after_l = after_lit;
//           Advance(after_l);
//           if (IsPunc(after_l, ")")) {
//             SPEC_RULE("Parse-Null-Ptr");
//             Parser after = after_l;
//             Advance(after);
//             PtrNullExpr ptr;
//             return {after, MakeExpr(SpanBetween(parser, after), ptr)};
//           }
//         }
//       }
//     }
//   }
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
// - IsOp(Parser, const char*) -> bool
// - IsPunc(Parser, const char*) -> bool
//
// FROM lexer/token helpers:
// - IsIdentTok(Token) -> bool
// - TokenKind::NullLiteral
//
// FROM AST types:
// - PtrNullExpr struct { } (empty - just a marker type)
// - ExprPtr = std::shared_ptr<Expr>
//
// =============================================================================
// REFACTORING NOTES
// =============================================================================
// 1. Export a try-parse function:
//    std::optional<ParseElemResult<ExprPtr>> TryParsePtrNullExpr(Parser parser);
//    Returns std::nullopt if current token is not "Ptr" or pattern doesn't match.
//
// 2. The function should bail out early at each stage if the pattern doesn't
//    match, returning std::nullopt to allow fallthrough to other parsers.
//
// 3. This is distinct from the bare `null` keyword handled in literal.cpp.
//    Ptr::null() creates a typed null pointer (Ptr<T>@Null).
//
// 4. The current implementation falls through (doesn't return) if the pattern
//    partially matches but fails (e.g., "Ptr::foo"). This allows the qualified
//    name parser to handle such cases. The refactored version should preserve
//    this behavior.
//
// 5. Consider: Should failed partial matches emit diagnostics or silently
//    fall through? Current behavior: silent fallthrough.
// =============================================================================
