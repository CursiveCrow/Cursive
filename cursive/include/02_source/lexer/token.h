#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "00_core/span.h"

namespace cursive::lexer
{

  // EOF is not a TokenKind in the C0 spec; the parser uses a separate sentinel.
  enum class TokenKind
  {
    Identifier,
    Keyword,
    IntLiteral,
    FloatLiteral,
    StringLiteral,
    CharLiteral,
    BoolLiteral,
    NullLiteral,
    Operator,
    Punctuator,
    Newline,
    Unknown,
  };

  // UTF-8 bytes corresponding to Lexeme(T,i,j) from the spec's scalar slice.
  using Lexeme = std::string;

  struct RawToken
  {
    TokenKind kind = TokenKind::Unknown;
    Lexeme lexeme;
    std::size_t start_offset = 0;
    std::size_t end_offset = 0;
  };

  struct Token
  {
    TokenKind kind = TokenKind::Unknown;
    Lexeme lexeme;
    core::Span span;
  };

  enum class DocKind
  {
    LineDoc,
    ModuleDoc,
  };

  struct DocComment
  {
    DocKind kind = DocKind::LineDoc;
    std::string text;
    core::Span span;
  };

  bool NoUnknownOk(const std::vector<Token> &tokens);

  Token AttachSpan(const core::SourceFile &source, const RawToken &raw);

  std::vector<Token> AttachSpans(const core::SourceFile &source,
                                 const std::vector<RawToken> &raws);

  Token MakeEofToken(const core::SourceFile &source);

} // namespace cursive::lexer
