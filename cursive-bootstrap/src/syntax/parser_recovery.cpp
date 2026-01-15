#include "cursive0/syntax/parser.h"

#include <string_view>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/diagnostic_messages.h"
#include "cursive0/core/diagnostics.h"

namespace cursive0::syntax {

namespace {

bool IsSyncItemToken(const Token& tok) {
  if (tok.kind == TokenKind::Keyword) {
    const std::string_view lex = tok.lexeme;
    return lex == "procedure" || lex == "record" || lex == "enum" ||
           lex == "modal" || lex == "class" || lex == "type" ||
           lex == "using" || lex == "let" || lex == "var";
  }
  return tok.kind == TokenKind::Punctuator && tok.lexeme == "}";
}

bool IsTerminatorToken(const Token& tok) {
  return tok.kind == TokenKind::Newline ||
         (tok.kind == TokenKind::Punctuator && tok.lexeme == ";");
}

}  // namespace

void EmitParseSyntaxErr(Parser& parser, const core::Span& span) {
  SPEC_RULE("Parse-Syntax-Err");
  auto diag = core::MakeDiagnostic("E-SRC-0520", span);
  if (!diag) {
    return;
  }
  parser.diags = core::Emit(parser.diags, *diag);
}

void SyncStmt(Parser& parser) {
  for (;;) {
    if (AtEof(parser)) {
      SPEC_RULE("Sync-Stmt-Stop");
      return;
    }
    const Token* tok = Tok(parser);
    if (!tok) {
      SPEC_RULE("Sync-Stmt-Stop");
      return;
    }
    if (tok->kind == TokenKind::Punctuator && tok->lexeme == "}") {
      SPEC_RULE("Sync-Stmt-Stop");
      return;
    }
    if (IsTerminatorToken(*tok)) {
      SPEC_RULE("Sync-Stmt-Consume");
      Advance(parser);
      return;
    }
    SPEC_RULE("Sync-Stmt-Advance");
    Advance(parser);
  }
}

void SyncItem(Parser& parser) {
  for (;;) {
    if (AtEof(parser)) {
      SPEC_RULE("Sync-Item-Stop");
      return;
    }
    const Token* tok = Tok(parser);
    if (!tok) {
      SPEC_RULE("Sync-Item-Stop");
      return;
    }
    if (IsSyncItemToken(*tok)) {
      SPEC_RULE("Sync-Item-Stop");
      return;
    }
    SPEC_RULE("Sync-Item-Advance");
    Advance(parser);
  }
}

void SyncType(Parser& parser) {
  for (;;) {
    if (AtEof(parser)) {
      SPEC_RULE("Sync-Type-Stop");
      return;
    }
    const Token* tok = Tok(parser);
    if (!tok) {
      SPEC_RULE("Sync-Type-Stop");
      return;
    }
    if (tok->kind == TokenKind::Punctuator) {
      if (tok->lexeme == ")" || tok->lexeme == "]" || tok->lexeme == "}") {
        SPEC_RULE("Sync-Type-Stop");
        return;
      }
      if (tok->lexeme == "," || tok->lexeme == ";") {
        SPEC_RULE("Sync-Type-Consume");
        Advance(parser);
        return;
      }
    }
    if (tok->kind == TokenKind::Newline) {
      SPEC_RULE("Sync-Type-Consume");
      Advance(parser);
      return;
    }
    SPEC_RULE("Sync-Type-Advance");
    Advance(parser);
  }
}

}  // namespace cursive0::syntax
