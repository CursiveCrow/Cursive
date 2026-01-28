#pragma once

#include <array>
#include <string_view>

namespace cursive0::core {

inline constexpr std::array<std::string_view, 50> kCursive0Keywords = {
    "all",
    "as",
    "break",
    "class",
    "continue",
    "dispatch",    // C0X Extension: structured concurrency
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
    "record",
    "region",
    "return",
    "shadow",
    "shared",
    "spawn",       // C0X Extension: structured concurrency
    "sync",
    "transition",
    "transmute",
    "true",
    "type",
    "unique",
    "unsafe",
    "var",
    "where",       // C0X Extension: generics
    "widen",
    "using",
    "yield",
    "const",
    "override",
};

// §3.3.4 Fixed identifiers - these are identifiers, not keywords
// They have special meaning in specific contexts only
inline constexpr std::array<std::string_view, 5> kCursive0FixedIdentifiers = {
    "dynamic",     // C0X Extension: key system
    "read",        // C0X Extension: key system
    "release",     // C0X Extension: key system
    "speculative", // C0X Extension: key system
    "write",       // C0X Extension: key system
};

inline bool IsFixedIdentifier(std::string_view s) {
  for (std::string_view ident : kCursive0FixedIdentifiers) {
    if (s == ident) {
      return true;
    }
  }
  return false;
}

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
