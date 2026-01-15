#pragma once

#include <array>
#include <string_view>

namespace cursive0::core {

inline constexpr std::array<std::string_view, 41> kCursive0Keywords = {
    "as",
    "break",
    "class",
    "continue",
    "else",
    "enum",
    "false",
    "defer",
    "frame",
    "if",
    "imm",
    "import",
    "internal",
    "let",
    "loop",
    "match",
    "modal",
    "move",
    "mut",
    "null",
    "private",
    "procedure",
    "protected",
    "public",
    "record",
    "region",
    "result",
    "return",
    "shadow",
    "shared",
    "transition",
    "transmute",
    "true",
    "type",
    "unique",
    "unsafe",
    "var",
    "widen",
    "using",
    "const",
    "override",
};

inline constexpr std::array<std::string_view, 45> kCursive0Operators = {
    "+",
    "-",
    "*",
    "/",
    "%",
    "**",
    "==",
    "!=",
    "<",
    "<=",
    ">",
    ">=",
    "&&",
    "||",
    "!",
    "&",
    "|",
    "^",
    "<<",
    ">>",
    "=",
    "+=",
    "-=",
    "*=",
    "/=",
    "%=",
    "&=",
    "|=",
    "^=",
    "<<=",
    ">>=",
    ":=",
    "<:",
    "..",
    "..=",
    "=>",
    "->",
    "::",
    "~",
    "~>",
    "~!",
    "?",
    "#",
    "@",
    "$",
};

inline constexpr std::array<std::string_view, 10> kCursive0Punctuators = {
    "(",
    ")",
    "[",
    "]",
    "{",
    "}",
    ",",
    ":",
    ";",
    ".",
};

inline bool IsKeyword(std::string_view s) {
  for (std::string_view kw : kCursive0Keywords) {
    if (s == kw) {
      return true;
    }
  }
  return false;
}

}  // namespace cursive0::core
