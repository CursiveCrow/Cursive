// =============================================================================
// MIGRATION MAPPING: Visibility Parsing
// =============================================================================
// Destination: cursive/src/02_source/parser/item/visibility.cpp
// Source:      cursive-bootstrap/src/02_syntax/parser_items.cpp
//
// PURPOSE:
// Parse visibility modifiers: public, private, internal, protected
// Default visibility is "internal" when no modifier present.
// =============================================================================

// =============================================================================
// SPEC RULES IMPLEMENTED
// =============================================================================
// From C0updated.md Section 3.3.6.2:
//
// (Parse-Vis-Public)
// IsKw(Tok(P), `public`)
// ──────────────────────────────────────────
// Γ ⊢ ParseVis(P) ⇓ (Advance(P), `public`)
//
// (Parse-Vis-Private)
// IsKw(Tok(P), `private`)
// ──────────────────────────────────────────
// Γ ⊢ ParseVis(P) ⇓ (Advance(P), `private`)
//
// (Parse-Vis-Internal)
// IsKw(Tok(P), `internal`)
// ──────────────────────────────────────────
// Γ ⊢ ParseVis(P) ⇓ (Advance(P), `internal`)
//
// (Parse-Vis-Protected)
// IsKw(Tok(P), `protected`)
// ──────────────────────────────────────────
// Γ ⊢ ParseVis(P) ⇓ (Advance(P), `protected`)
//
// (Parse-Vis-Default)
// ¬ IsKw(Tok(P), v)  [where v ∈ {public, private, internal, protected}]
// ──────────────────────────────────────────
// Γ ⊢ ParseVis(P) ⇓ (P, `internal`)

// =============================================================================
// SOURCE CODE MAPPING
// =============================================================================

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser.cpp (visibility parsing)
NOTE: ParseVis is defined in parser.cpp, not parser_items.cpp
FUNCTION: ParseVis

The implementation checks for each visibility keyword in order:
1. public -> Visibility::Public
2. private -> Visibility::Private
3. internal -> Visibility::Internal
4. protected -> Visibility::Protected
5. Default (no keyword) -> Visibility::Internal

IMPLEMENTATION NOTES:
- Visibility keywords: public, private, internal, protected
- Default is "internal" (not public) - different from many languages
- Must consume the keyword token if matched
- Return parser position unchanged if no visibility keyword
*/

// =============================================================================
// TYPE DEFINITIONS USED
// =============================================================================

/*
ENUM: Visibility
enum class Visibility {
  Public,
  Private,
  Internal,   // Default visibility
  Protected
};

RESULT TYPE:
ParseElemResult<Visibility> { Parser parser; Visibility elem; }
*/

// =============================================================================
// GRAMMAR
// =============================================================================

/*
Visibility ::= 'public' | 'private' | 'internal' | 'protected' | ε

When ε (empty), defaults to Internal.
*/

// =============================================================================
// IMPLEMENTATION CHECKLIST
// =============================================================================
// [ ] ParseVis function
//     - Check for 'public' keyword -> Visibility::Public
//     - Check for 'private' keyword -> Visibility::Private
//     - Check for 'internal' keyword -> Visibility::Internal
//     - Check for 'protected' keyword -> Visibility::Protected
//     - Default (no match) -> Visibility::Internal (do not advance parser)

// =============================================================================
// PSEUDO-IMPLEMENTATION
// =============================================================================

/*
ParseElemResult<Visibility> ParseVis(Parser parser) {
  if (IsKw(parser, "public")) {
    Advance(parser);
    return {parser, Visibility::Public};
  }
  if (IsKw(parser, "private")) {
    Advance(parser);
    return {parser, Visibility::Private};
  }
  if (IsKw(parser, "internal")) {
    Advance(parser);
    return {parser, Visibility::Internal};
  }
  if (IsKw(parser, "protected")) {
    Advance(parser);
    return {parser, Visibility::Protected};
  }
  // Default: internal visibility, do not consume any token
  return {parser, Visibility::Internal};
}
*/
