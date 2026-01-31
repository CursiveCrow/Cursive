// =============================================================================
// MIGRATION MAPPING: identifier_pattern.cpp
// =============================================================================
// This file should contain parsing logic for identifier patterns, which bind
// a matched value to a name without requiring a type annotation.
//
// SPEC REFERENCE: C0updated.md, Section 3.3.9, Lines 6102-6105
// =============================================================================
//
// FORMAL RULE:
// -----------------------------------------------------------------------------
// **(Parse-Pattern-Identifier)** Lines 6102-6105
// IsIdent(Tok(P))
// --------------------------------------------------------------------------
// Gamma |- ParsePatternAtom(P) => (Advance(P), IdentifierPattern(Lexeme(Tok(P))))
//
// SEMANTICS:
// - Matches any value and binds it to the identifier
// - Used in let bindings and match arms:
//   - let x = 42
//   - match opt { Some(v) => v, None => 0 }
// - The identifier becomes available in the arm's body/expression
//
// DISAMBIGUATION (priority order in ParsePatternAtom):
// 1. Literal patterns (if token is a literal)
// 2. TypedPattern (identifier followed by `:`)
// 3. WildcardPattern (identifier is `_`)
// 4. EnumPattern (identifier followed by `::`)
// 5. RecordPattern (path followed by `{`)
// 6. IdentifierPattern (fallback for plain identifiers)
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_patterns.cpp
// =============================================================================
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
//
// 1. Identifier pattern branch in ParsePatternAtom (Lines 402-409)
//    -------------------------------------------------------------------------
//    if (IsIdentTok(*tok)) {
//      SPEC_RULE("Parse-Pattern-Identifier");
//      Parser next = parser;
//      Advance(next);
//      IdentifierPattern pat;
//      pat.name = tok->lexeme;
//      return {next, MakePattern(tok->span, pat)};
//    }
//
//    NOTE: This is the last identifier-based pattern check, serving as a
//    fallback when no other pattern type matches.
//
// POSITION IN ParsePatternAtom (Lines 295-413):
// -----------------------------------------------------------------------------
// The identifier pattern check comes AFTER all other identifier-based checks:
// - Line 302: LiteralPattern check
// - Line 313: TypedPattern check (identifier + ":")
// - Line 329: WildcardPattern check (identifier = "_")
// - Line 336: EnumPattern check (identifier + "::")
// - Line 351: TuplePattern check
// - Line 368: RecordPattern check (path + "{")
// - Line 390: ModalPattern check ("@" + identifier)
// - Line 402: IdentifierPattern check (fallback)
//
// DEPENDENCIES:
// =============================================================================
// - IdentifierPattern AST node type (contains name: string field)
// - MakePattern helper (pattern_common.cpp)
// - IsIdentTok, Tok, Advance parser utilities
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
// - IdentifierPattern is a simple binding pattern
// - Must be checked LAST among identifier-based patterns
// - All the precedence/priority logic is handled by check ordering
// - Span is taken directly from the identifier token
// - The name field stores the lexeme (not the token itself)
// =============================================================================
