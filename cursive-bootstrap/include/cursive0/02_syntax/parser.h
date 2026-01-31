#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include "cursive0/00_core/assert_spec.h"
#include "cursive0/00_core/diagnostics.h"
#include "cursive0/00_core/source_text.h"
#include "cursive0/02_syntax/ast.h"
#include "cursive0/02_syntax/lexer.h"
#include "cursive0/02_syntax/token.h"

namespace cursive0::syntax {

enum class TerminatorPolicy {
  Required,
  Optional,
};

struct Parser {
  const std::vector<Token>* tokens = nullptr;
  std::shared_ptr<std::vector<Token>> owned_tokens;
  std::size_t index = 0;
  const std::vector<DocComment>* docs = nullptr;
  std::size_t doc_index = 0;
  std::size_t depth = 0;
  core::DiagnosticStream diags;
  EofToken eof;
};

Parser MakeParser(const std::vector<Token>& tokens,
                  const std::vector<DocComment>& docs,
                  const core::SourceFile& source);

Parser MakeParser(const std::vector<Token>& tokens,
                  const core::SourceFile& source);

bool AtEof(const Parser& parser);
const Token* Tok(const Parser& parser);
const core::Span& TokSpan(const Parser& parser);
void Advance(Parser& parser);
Parser AdvanceOrEOF(const Parser& parser);

Parser Clone(const Parser& parser);
Parser MergeDiag(const Parser& base, const Parser& diag, const Parser& src);

bool PStateOk(const Parser& parser);
core::Span SpanBetween(const Parser& start, const Parser& end);

std::pair<core::Span, core::Span> SplitSpan2(const core::Span& sp);

Parser SplitShiftR(const Parser& parser);

int AngleDelta(const Token& tok);

struct AngleStepResult {
  Parser parser;
  int depth = 0;
};

AngleStepResult AngleStep(const Parser& parser, int depth);

Parser AngleScan(const Parser& start, const Parser& parser, int depth);

Parser SkipAngles(const Parser& parser);

void SyncStmt(Parser& parser);
void SyncItem(Parser& parser);
void SyncType(Parser& parser);
void EmitParseSyntaxErr(Parser& parser, const core::Span& span);

struct TokenKindMatch {
  TokenKind kind = TokenKind::Unknown;
  std::string_view lexeme;
};

inline TokenKindMatch MatchKind(TokenKind kind) { return {kind, {}}; }
inline TokenKindMatch MatchKeyword(std::string_view s) {
  return {TokenKind::Keyword, s};
}
inline TokenKindMatch MatchOperator(std::string_view s) {
  return {TokenKind::Operator, s};
}
inline TokenKindMatch MatchPunct(std::string_view s) {
  return {TokenKind::Punctuator, s};
}

bool TokenMatches(const Token& tok, const TokenKindMatch& match);
bool TokenInEndSet(const Token& tok, std::span<const TokenKindMatch> end_set);

bool ConsumeKind(Parser& parser, TokenKind kind);
bool ConsumeKeyword(Parser& parser, std::string_view keyword);
bool ConsumeOperator(Parser& parser, std::string_view op);
bool ConsumePunct(Parser& parser, std::string_view punct);

bool TrailingComma(const Parser& parser,
                   std::span<const TokenKindMatch> end_set);
bool EmitTrailingCommaErr(Parser& parser,
                          std::span<const TokenKindMatch> end_set);

void ConsumeTerminatorOpt(Parser& parser, TerminatorPolicy policy);
void ConsumeTerminatorReq(Parser& parser);

struct ParseItemResult {
  Parser parser;
  ASTItem item;
};

struct ParseItemsResult {
  Parser parser;
  std::vector<ASTItem> items;
  std::vector<DocComment> module_doc;
};

struct ParseFileResult {
  std::optional<ASTFile> file;
  core::DiagnosticStream diags;
};

ParseItemResult ParseItem(Parser parser);
ParseItemsResult ParseItems(Parser parser);
ParseFileResult ParseFile(const core::SourceFile& source);

bool ParseFileBestEffort(const ParseFileResult& result);
bool ParseFileOk(const ParseFileResult& result);

template <typename Elem>
struct ParseElemResult {
  Parser parser;
  Elem elem;
};

ParseElemResult<Identifier> ParseIdent(Parser parser);
ParseElemResult<ModulePath> ParseModulePath(Parser parser);
ParseElemResult<TypePath> ParseTypePath(Parser parser);
ParseElemResult<ClassPath> ParseClassPath(Parser parser);

struct ParseQualifiedHeadResult {
  Parser parser;
  ModulePath module_path;
  Identifier name;
};

ParseQualifiedHeadResult ParseQualifiedHead(Parser parser);

ParseElemResult<Visibility> ParseVis(Parser parser);
ParseElemResult<std::optional<Identifier>> ParseAliasOpt(Parser parser);

ParseElemResult<std::shared_ptr<Pattern>> ParsePattern(Parser parser);
ParseElemResult<std::shared_ptr<Type>> ParseType(Parser parser);
ParseElemResult<std::shared_ptr<Type>> ParseTypeAnnotOpt(Parser parser);
ParseElemResult<std::shared_ptr<Expr>> ParseExpr(Parser parser);
ParseElemResult<std::shared_ptr<Expr>> ParseExprOpt(Parser parser);
ParseElemResult<Binding> ParseBindingAfterLetVar(Parser parser);
ParseElemResult<std::shared_ptr<Block>> ParseBlock(Parser parser);
ParseElemResult<Stmt> ParseShadowBinding(Parser parser);
ParseElemResult<Stmt> ParseStmt(Parser parser);

template <typename Elem>
struct ListState {
  Parser parser;
  std::vector<Elem> elems;
};

template <typename Elem>
inline ListState<Elem> ListStart(Parser parser) {
  SPEC_RULE("List-Start");
  ListState<Elem> state;
  state.parser = parser;
  return state;
}

template <typename Elem, typename ParseElemFn>
inline ListState<Elem> ListCons(const ListState<Elem>& state,
                                ParseElemFn parse_elem) {
  SPEC_RULE("List-Cons");
  ParseElemResult<Elem> parsed = parse_elem(state.parser);
  ListState<Elem> out;
  out.parser = parsed.parser;
  out.elems = state.elems;
  out.elems.push_back(std::move(parsed.elem));
  return out;
}

template <typename Elem>
inline bool ListDone(const ListState<Elem>& state,
                     std::span<const TokenKindMatch> end_set) {
  SPEC_RULE("List-Done");
  const Token* tok = Tok(state.parser);
  if (!tok) {
    return false;
  }
  return TokenInEndSet(*tok, end_set);
}

}  // namespace cursive0::syntax
