#pragma once

#include <array>
#include <string_view>

namespace cursive0::core {

inline constexpr std::array<std::string_view, 55> kCursive0Keywords = {
    "all",
    "as",
    "break",
    "class",
    "continue",
    "dispatch",    // C0X Extension: structured concurrency
    "dynamic",     // C0X Extension: key system
    "else",
    "enum",
    "false",
    "defer",
    "frame",
    "from",
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
    "parallel",    // C0X Extension: structured concurrency
    "private",
    "procedure",
    "protected",
    "public",
    "race",
    "read",        // C0X Extension: key system
    "record",
    "region",
    "release",     // C0X Extension: key system
    "result",
    "return",
    "shadow",
    "shared",
    "spawn",       // C0X Extension: structured concurrency
    "sync",
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
    "yield",
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

inline constexpr std::array<std::string_view, 12> kCursive0Punctuators = {
    "(",
    ")",
    "[",
    "]",
    "[[",
    "]]",
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

// Reserved identifiers that are unsupported in Cursive0 subset
// These trigger E-UNS-0101 when used as identifiers
inline constexpr std::array<std::string_view, 24> kUnsupportedLexemes = {
    "derive",
    "attribute",
    "opaque_type",
    "refinement_type",
    "closure",
    "pipeline",
    "async",
    "metaprogramming",
    "Network",
    "Reactor",
    "GPUFactory",
    "CPUFactory",
    "AsyncRuntime",
    "key_system",
    "extern_block",
    "foreign_decl",
    "class_generics",
    "class_where_clause",
    "associated_type",
    "modal_class",
    "class_contract",
    "type_item",
    "abstract_state",
    "override_in_class",
};

inline bool IsUnsupportedLexeme(std::string_view s) {
  for (std::string_view lex : kUnsupportedLexemes) {
    if (s == lex) {
      return true;
    }
  }
  return false;
}

}  // namespace cursive0::core
