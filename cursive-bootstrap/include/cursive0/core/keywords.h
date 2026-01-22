#pragma once

#include <array>
#include <string_view>

namespace cursive0::core {

inline constexpr std::array<std::string_view, 47> kCursive0Keywords = {
    "as",
    "break",
    "class",
    "continue",
    "dynamic",     // C0X Extension: key system
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
    "read",        // C0X Extension: key system
    "record",
    "region",
    "release",     // C0X Extension: key system
    "result",
    "return",
    "shadow",
    "shared",
    "speculative", // C0X Extension: key system
    "transition",
    "transmute",
    "true",
    "type",
    "unique",
    "unsafe",
    "var",
    "where",       // C0X Extension: generics
    "widen",
    "write",       // C0X Extension: key system
    "using",
    "const",
    "override",
};

inline constexpr std::array<std::string_view, 46> kCursive0Operators = {
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
    "~%",
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
