#include "cursive0/syntax/parser.h"

#include <string_view>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/diagnostic_messages.h"
#include "cursive0/core/diagnostics.h"
#include "cursive0/syntax/keyword_policy.h"

namespace cursive0::syntax {

namespace {
bool IsVisKeyword(std::string_view lexeme) {
  return lexeme == "public" || lexeme == "internal" || lexeme == "private" ||
         lexeme == "protected";
}

std::optional<Visibility> VisibilityFromLexeme(std::string_view lexeme) {
  if (lexeme == "public") {
    return Visibility::Public;
  }
  if (lexeme == "internal") {
    return Visibility::Internal;
  }
  if (lexeme == "private") {
    return Visibility::Private;
  }
  if (lexeme == "protected") {
    return Visibility::Protected;
  }
  return std::nullopt;
}

ParseElemResult<ModulePath> ParseModulePathTail(Parser parser, ModulePath xs) {
  const Token* tok = Tok(parser);
  if (!tok || !IsOpTok(*tok, "::")) {
    SPEC_RULE("Parse-ModulePathTail-End");
    return {parser, xs};
  }

  SPEC_RULE("Parse-ModulePathTail-Cons");
  Parser next = parser;
  Advance(next);
  ParseElemResult<Identifier> id = ParseIdent(next);
  xs.push_back(id.elem);
  return ParseModulePathTail(id.parser, std::move(xs));
}

ParseElemResult<TypePath> ParseTypePathTail(Parser parser, TypePath xs) {
  const Token* tok = Tok(parser);
  if (!tok || !IsOpTok(*tok, "::")) {
    SPEC_RULE("Parse-TypePathTail-End");
    return {parser, xs};
  }

  SPEC_RULE("Parse-TypePathTail-Cons");
  Parser next = parser;
  Advance(next);
  ParseElemResult<Identifier> id = ParseIdent(next);
  xs.push_back(id.elem);
  return ParseTypePathTail(id.parser, std::move(xs));
}

}  // namespace

ParseElemResult<Identifier> ParseIdent(Parser parser) {
  const Token* tok = Tok(parser);
  if (tok && IsIdentTok(*tok)) {
    SPEC_RULE("Parse-Ident");
    Identifier name = tok->lexeme;
    Advance(parser);
    return {parser, name};
  }

  SPEC_RULE("Parse-Ident-Err");
  EmitParseSyntaxErr(parser, TokSpan(parser));
  return {parser, Identifier{"_"}};
}

ParseElemResult<ModulePath> ParseModulePath(Parser parser) {
  SPEC_RULE("Parse-ModulePath");
  ParseElemResult<Identifier> head = ParseIdent(parser);
  ModulePath path;
  path.push_back(head.elem);
  return ParseModulePathTail(head.parser, std::move(path));
}

ParseElemResult<TypePath> ParseTypePath(Parser parser) {
  SPEC_RULE("Parse-TypePath");
  ParseElemResult<Identifier> head = ParseIdent(parser);
  TypePath path;
  path.push_back(head.elem);
  return ParseTypePathTail(head.parser, std::move(path));
}

ParseElemResult<ClassPath> ParseClassPath(Parser parser) {
  SPEC_RULE("Parse-ClassPath");
  ParseElemResult<TypePath> path = ParseTypePath(parser);
  return {path.parser, path.elem};
}

ParseQualifiedHeadResult ParseQualifiedHead(Parser parser) {
  SPEC_RULE("Parse-QualifiedHead");
  ParseElemResult<Identifier> head = ParseIdent(parser);
  const Token* tok = Tok(head.parser);
  if (!tok || !IsOpTok(*tok, "::")) {
    EmitParseSyntaxErr(parser, TokSpan(parser));
    return {parser, {}, "_"};
  }

  ParseElemResult<ModulePath> rest = ParseModulePathTail(head.parser, {head.elem});
  ModulePath full = std::move(rest.elem);
  if (full.size() < 2) {
    EmitParseSyntaxErr(parser, TokSpan(parser));
    return {rest.parser, full, "_"};
  }
  Identifier name = full.back();
  full.pop_back();
  return {rest.parser, full, name};
}

ParseElemResult<Visibility> ParseVis(Parser parser) {
  const Token* tok = Tok(parser);
  if (tok && IsKwTok(*tok, tok->lexeme) && IsVisKeyword(tok->lexeme)) {
    SPEC_RULE("Parse-Vis-Opt");
    std::optional<Visibility> vis = VisibilityFromLexeme(tok->lexeme);
    Advance(parser);
    return {parser, vis.value_or(Visibility::Internal)};
  }

  SPEC_RULE("Parse-Vis-Default");
  return {parser, Visibility::Internal};
}

ParseElemResult<std::optional<Identifier>> ParseAliasOpt(Parser parser) {
  const Token* tok = Tok(parser);
  if (tok && IsKwTok(*tok, "as")) {
    SPEC_RULE("Parse-AliasOpt-Yes");
    Parser next = parser;
    Advance(next);
    ParseElemResult<Identifier> id = ParseIdent(next);
    return {id.parser, id.elem};
  }

  SPEC_RULE("Parse-AliasOpt-None");
  return {parser, std::nullopt};
}

}  // namespace cursive0::syntax
