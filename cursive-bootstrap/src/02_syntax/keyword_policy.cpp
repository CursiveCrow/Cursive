#include "cursive0/02_syntax/keyword_policy.h"

#include <string_view>

#include "cursive0/00_core/assert_spec.h"
#include "cursive0/00_core/diagnostic_messages.h"
#include "cursive0/00_core/keywords.h"

namespace cursive0::syntax {

namespace {

}  // namespace

bool IsIdentTok(const Token& tok) {
  return tok.kind == TokenKind::Identifier;
}

bool IsKwTok(const Token& tok, std::string_view s) {
  return tok.kind == TokenKind::Keyword && tok.lexeme == s;
}

bool IsOpTok(const Token& tok, std::string_view s) {
  return tok.kind == TokenKind::Operator && tok.lexeme == s;
}

bool IsPuncTok(const Token& tok, std::string_view s) {
  return tok.kind == TokenKind::Punctuator && tok.lexeme == s;
}

std::string_view LexemeText(const Token& tok) {
  return tok.lexeme;
}

bool IsKeyword(std::string_view s) {
  return core::IsKeyword(s);
}

bool IsFixedIdentifier(std::string_view s) {
  return core::IsFixedIdentifier(s);
}

bool IsFixedIdentTok(const Token& tok, std::string_view s) {
  return tok.kind == TokenKind::Identifier && tok.lexeme == s;
}

bool IsCtxKeyword(std::string_view s) {
  return s == "in" || s == "key" || s == "wait";
}

bool Ctx(const Token& tok, std::string_view s) {
  return IsIdentTok(tok) && tok.lexeme == s && IsCtxKeyword(s);
}

bool UnionPropTok(const Token& tok) {
  return IsOpTok(tok, "?");
}

bool TypeWhereTok(const Token& tok) {
  return IsIdentTok(tok) && tok.lexeme == "where";
}

bool OpaqueTypeTok(const Token& tok) {
  return IsIdentTok(tok) && tok.lexeme == "opaque";
}

void CheckMethodContext(const ASTFile& file, core::DiagnosticStream& diags) {
  SPEC_RULE("Method-Context-Err");
  bool ok = true;
  for (const auto& item : file.items) {
    if (const auto* record = std::get_if<RecordDecl>(&item)) {
      for (const auto& member : record->members) {
        if (std::holds_alternative<MethodDecl>(member) ||
            std::holds_alternative<FieldDecl>(member)) {
          continue;
        }
        ok = false;
      }
      continue;
    }
    if (const auto* cls = std::get_if<ClassDecl>(&item)) {
      for (const auto& member : cls->items) {
        if (std::holds_alternative<ClassMethodDecl>(member) ||
            std::holds_alternative<ClassFieldDecl>(member)) {
          continue;
        }
        ok = false;
      }
      continue;
    }
  }

  if (!ok) {
    if (auto diag = core::MakeDiagnostic("E-SEM-3011")) {
      diags = core::Emit(diags, *diag);
    }
  }
}

}  // namespace cursive0::syntax
