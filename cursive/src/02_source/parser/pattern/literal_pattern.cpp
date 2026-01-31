// =============================================================================
// MIGRATION MAPPING: literal_pattern.cpp
// =============================================================================
// This file should contain parsing logic for literal patterns, which match
// constant values (integers, floats, strings, chars, bools, null).
//
// SPEC REFERENCE: C0updated.md, Section 3.3.9, Lines 6087-6090
// =============================================================================
//
// FORMAL RULE:
// -----------------------------------------------------------------------------
// **(Parse-Pattern-Literal)** Lines 6087-6090
// Tok(P).kind in {IntLiteral, FloatLiteral, StringLiteral, CharLiteral,
//                 BoolLiteral, NullLiteral}
// --------------------------------------------------------------------------
// Gamma |- ParsePatternAtom(P) => (Advance(P), LiteralPattern(Tok(P)))
//
// SEMANTICS:
// - Recognizes literal tokens as the start of a literal pattern
// - Consumes the literal token and produces a LiteralPattern node
// - The pattern stores the original token for value extraction later
// - Used in match arms to match against constant values:
//   - match x { 0 => ..., 1 => ..., _ => ... }
//   - match c { 'a' => ..., '\n' => ... }
//   - match s { "hello" => ... }
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_patterns.cpp
// =============================================================================
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
//
// 1. IsLiteralToken helper (Lines 33-40)
//    -------------------------------------------------------------------------
//    bool IsLiteralToken(const Token& tok) {
//      return tok.kind == TokenKind::IntLiteral ||
//             tok.kind == TokenKind::FloatLiteral ||
//             tok.kind == TokenKind::StringLiteral ||
//             tok.kind == TokenKind::CharLiteral ||
//             tok.kind == TokenKind::BoolLiteral ||
//             tok.kind == TokenKind::NullLiteral;
//    }
//
//    NOTE: This helper may belong in pattern_common.cpp or parser utilities.
//
// 2. Literal pattern branch in ParsePatternAtom (Lines 302-309)
//    -------------------------------------------------------------------------
//    if (IsLiteralToken(*tok)) {
//      SPEC_RULE("Parse-Pattern-Literal");
//      Parser next = parser;
//      Advance(next);
//      LiteralPattern lit;
//      lit.literal = *tok;
//      return {next, MakePattern(tok->span, lit)};
//    }
//
// DEPENDENCIES:
// =============================================================================
// - TokenKind enum with literal kinds
// - Token structure with kind and lexeme fields
// - LiteralPattern AST node type (contains Token field)
// - MakePattern helper (pattern_common.cpp)
// - IsLiteralToken helper (pattern_common.cpp or this file)
// - Advance, Tok parser utilities
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
// - This is the simplest pattern type - just wraps a literal token
// - Should be the first check in ParsePatternAtom due to priority ordering
// - The stored token contains both the lexeme and token kind for later use
// - Span is directly taken from the token's span
// - No recursion or lookahead needed
// =============================================================================
