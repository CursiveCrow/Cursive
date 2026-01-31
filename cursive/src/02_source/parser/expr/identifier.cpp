// =============================================================================
// MIGRATION MAPPING: identifier.cpp
// =============================================================================
// This file implements parsing for simple identifier expressions (variable
// references, function names, etc. that are NOT qualified paths).
//
// =============================================================================
// SPEC REFERENCE: C0updated.md
// =============================================================================
// Lines 5184-5187:
//
// **(Parse-Identifier-Expr)**
// IsIdent(Tok(P))
//     ¬ IsOp(Tok(Advance(P)), "::")
//     ¬ IsOp(Tok(Advance(P)), "@")
//     ¬ IsPunc(Tok(Advance(P)), "{")
// ─────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParsePrimary(P) ⇓ (Advance(P), Identifier(Lexeme(Tok(P))))
//
// SEMANTICS:
// - Current token must be an identifier
// - Next token must NOT be "::" (would be qualified name/apply)
// - Next token must NOT be "@" (would be modal state reference)
// - Next token must NOT be "{" (would be record literal) - controlled by allow_brace
// - Returns IdentifierExpr containing the identifier name
// - Parser advances past the identifier token
//
// NOTE: The spec rule shows ¬ IsPunc(Tok(Advance(P)), "{") but the
// implementation also checks for "<" (generics) which is an extension.
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_expr.cpp
// =============================================================================
//
// 1. IDENTIFIER EXPRESSION PARSING
//    Source lines: 1117-1140
//    -------------------------------------------------------------------------
//    if (tok && IsIdentTok(*tok)) {
//      Parser next = parser;
//      Advance(next);
//      const Token* look = Tok(next);
//      const bool is_qual = look && IsOpTok(*look, "::");
//      const bool is_modal = look && IsOpTok(*look, "@");
//      const bool is_record = look &&
//          look->kind == TokenKind::Punctuator && look->lexeme == "{";
//      const bool is_generic = look && IsOpTok(*look, "<");
//      if (!look || (!is_qual && (!allow_brace || !is_modal) &&
//                    (!allow_brace || !is_record) &&
//                    (!allow_brace || !is_generic))) {
//        SPEC_RULE("Parse-Identifier-Expr");
//        // Check for unsupported lexemes used as identifiers
//        if (core::IsUnsupportedLexeme(tok->lexeme)) {
//          EmitUnsupportedConstruct(parser);
//          SyncStmt(next);
//          return {next, MakeExpr(tok->span, ErrorExpr{})};
//        }
//        IdentifierExpr ident;
//        ident.name = tok->lexeme;
//        return {next, MakeExpr(tok->span, ident)};
//      }
//    }
//
// 2. RECEIVER REFERENCE (~) - Related but separate
//    Source lines: 1094-1102
//    -------------------------------------------------------------------------
//    // Receiver reference: ~ is used inside method bodies to access the receiver
//    if (tok && IsOpTok(*tok, "~")) {
//      SPEC_RULE("Parse-Receiver-Ref");
//      Parser next = parser;
//      Advance(next);
//      IdentifierExpr ident;
//      ident.name = "~";
//      return {next, MakeExpr(tok->span, ident)};
//    }
//
//    NOTE: The receiver reference creates an IdentifierExpr with name "~",
//    which is semantically the `self` receiver in method context. This should
//    probably be moved to a separate receiver.cpp or kept here as a special case.
//
// =============================================================================
// DEPENDENCIES
// =============================================================================
// FROM expr_common.cpp:
// - MakeExpr(Span, ExprNode) -> ExprPtr
//
// FROM parser_common.cpp:
// - Parser state type
// - Advance(Parser&) -> void
// - Tok(Parser) -> const Token*
// - SyncStmt(Parser&) -> void
//
// FROM lexer/token helpers:
// - IsIdentTok(Token) -> bool
// - IsOpTok(Token, const char*) -> bool
// - TokenKind::Punctuator
//
// FROM core utilities:
// - core::IsUnsupportedLexeme(string_view) -> bool
//
// FROM error emission:
// - EmitUnsupportedConstruct(Parser) -> void
//
// FROM AST types:
// - IdentifierExpr struct { std::string name; }
// - ErrorExpr struct { }
// - ExprPtr = std::shared_ptr<Expr>
//
// =============================================================================
// REFACTORING NOTES
// =============================================================================
// 1. Export a try-parse function:
//    std::optional<ParseElemResult<ExprPtr>> TryParseIdentifierExpr(
//        Parser parser, bool allow_brace);
//    Returns std::nullopt if:
//    - Current token is not an identifier, OR
//    - Next token indicates a qualified/modal/record/generic expression
//
// 2. The allow_brace parameter controls whether lookahead for modal (@),
//    record ({), and generic (<) prevents identifier parsing:
//    - allow_brace=true: these lookaheads block identifier parsing
//    - allow_brace=false: these lookaheads are ignored (identifier allowed)
//
// 3. Guard against unsupported lexemes (e.g., reserved future keywords).
//    This emits a diagnostic and returns ErrorExpr.
//
// 4. The receiver reference (~) case might be:
//    a) Kept here as a special identifier case
//    b) Moved to receiver.cpp as a separate primary expression parser
//    c) Handled inline in the main ParsePrimary dispatcher
//    Current recommendation: Keep it separate but in this file since it
//    produces an IdentifierExpr.
//
// 5. The lookahead logic is somewhat complex due to the interaction between
//    is_qual, is_modal, is_record, is_generic, and allow_brace. Consider
//    simplifying with a helper: ShouldParseAsSimpleIdent(parser, allow_brace).
// =============================================================================
