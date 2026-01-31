#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "cursive0/00_core/span.h"

namespace cursive0::syntax {

// EOF is not a TokenKind in the C0 spec; the parser uses a separate sentinel.
enum class TokenKind {
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

struct RawToken {
  TokenKind kind = TokenKind::Unknown;
  Lexeme lexeme;
  std::size_t start_offset = 0;
  std::size_t end_offset = 0;
};

struct Token {
  TokenKind kind = TokenKind::Unknown;
  Lexeme lexeme;
  core::Span span;
};

struct EofToken {
  core::Span span;
};

bool NoUnknownOk(const std::vector<Token>& tokens);

Token AttachSpan(const core::SourceFile& source, const RawToken& raw);

std::vector<Token> AttachSpans(const core::SourceFile& source,
                               const std::vector<RawToken>& raws);

EofToken MakeEofToken(const core::SourceFile& source);

}  // namespace cursive0::syntax
