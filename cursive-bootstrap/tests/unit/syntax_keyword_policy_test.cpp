#include <cassert>
#include <string_view>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/diagnostics.h"
#include "cursive0/syntax/keyword_policy.h"
#include "cursive0/syntax/token.h"

namespace {

using cursive0::core::DiagnosticStream;
using cursive0::syntax::ASTFile;
using cursive0::syntax::CheckMethodContext;
using cursive0::syntax::Ctx;
using cursive0::syntax::IsCtxKeyword;
using cursive0::syntax::IsIdentTok;
using cursive0::syntax::IsKwTok;
using cursive0::syntax::IsOpTok;
using cursive0::syntax::IsPuncTok;
using cursive0::syntax::LexemeText;
using cursive0::syntax::OpaqueTypeTok;
using cursive0::syntax::Token;
using cursive0::syntax::TokenKind;
using cursive0::syntax::TypeWhereTok;
using cursive0::syntax::UnionPropTok;
using cursive0::syntax::UsingKeyword;

Token MakeTok(TokenKind kind, std::string_view lexeme) {
  Token tok;
  tok.kind = kind;
  tok.lexeme = std::string(lexeme);
  return tok;
}

}  // namespace

int main() {
  SPEC_COV("Method-Context-Err");

  {
    const Token tok = MakeTok(TokenKind::Keyword, "using");
    assert(IsKwTok(tok, "using"));
    assert(LexemeText(tok) == "using");
    assert(UsingKeyword() == "using");
  }

  {
    const Token tok = MakeTok(TokenKind::Identifier, "use");
    assert(IsIdentTok(tok));
    assert(!IsKwTok(tok, "use"));
  }

  {
    const Token tok = MakeTok(TokenKind::Identifier, "in");
    assert(IsCtxKeyword("in"));
    assert(!IsCtxKeyword("as"));
    assert(!IsCtxKeyword("move"));
    assert(Ctx(tok, "in"));
    assert(!Ctx(tok, "as"));
  }

  {
    const Token tok = MakeTok(TokenKind::Operator, "?");
    assert(IsOpTok(tok, "?"));
    assert(UnionPropTok(tok));
  }

  {
    const Token where_tok = MakeTok(TokenKind::Identifier, "where");
    const Token opaque_tok = MakeTok(TokenKind::Identifier, "opaque");
    assert(TypeWhereTok(where_tok));
    assert(OpaqueTypeTok(opaque_tok));
  }

  {
    const Token tok = MakeTok(TokenKind::Punctuator, "{");
    assert(IsPuncTok(tok, "{"));
  }

  {
    ASTFile file;
    file.path = "empty.cursive";
    DiagnosticStream diags;
    CheckMethodContext(file, diags);
    assert(diags.empty());
  }

  return 0;
}
