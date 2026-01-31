#pragma once

#include <string_view>

#include "cursive0/00_core/diagnostics.h"
#include "cursive0/02_syntax/ast.h"
#include "cursive0/02_syntax/token.h"

namespace cursive0::syntax {

bool IsIdentTok(const Token& tok);
bool IsKwTok(const Token& tok, std::string_view s);
bool IsOpTok(const Token& tok, std::string_view s);
bool IsPuncTok(const Token& tok, std::string_view s);
std::string_view LexemeText(const Token& tok);

bool IsKeyword(std::string_view s);

// §3.3.4 Fixed identifiers - identifiers with special meaning in context
bool IsFixedIdentifier(std::string_view s);
bool IsFixedIdentTok(const Token& tok, std::string_view s);

bool IsCtxKeyword(std::string_view s);
bool Ctx(const Token& tok, std::string_view s);

constexpr std::string_view UsingKeyword() { return "using"; }

bool UnionPropTok(const Token& tok);
bool TypeWhereTok(const Token& tok);
bool OpaqueTypeTok(const Token& tok);

void CheckMethodContext(const ASTFile& file, core::DiagnosticStream& diags);

}  // namespace cursive0::syntax
