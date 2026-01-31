// =============================================================================
// MIGRATION MAPPING: opaque_type.cpp
// =============================================================================
// This file should contain parsing logic for opaque types: opaque TypePath
// representing types whose internal structure is hidden.
//
// SPEC REFERENCE: C0updated.md, Section 3.3.7, Lines 4808-4811
// =============================================================================
//
// FORMAL RULES FROM SPEC:
// -----------------------------------------------------------------------------
//
// **(Parse-Opaque-Type)** Lines 4808-4811
// IsIdent(Tok(P))    Lexeme(Tok(P)) = `opaque`
// Γ ⊢ ParseTypePath(Advance(P)) ⇓ (P_1, path)
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseNonPermType(P) ⇓ (P_1, TypeOpaque(path))
//
// SEMANTICS:
// - Opaque types hide internal representation
// - Used for abstract data types and module encapsulation
// - Syntax: opaque TypePath
// - The "opaque" keyword indicates the type's structure is not exposed
// - Examples: opaque FileHandle, opaque crypto::Key
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_types.cpp
// =============================================================================
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
//
// 1. OpaqueTypeTok predicate (in keyword_policy.h or similar):
//    ─────────────────────────────────────────────────────────────────────────
//    bool OpaqueTypeTok(const Token& tok) {
//      return tok.kind == TokenKind::Identifier && tok.lexeme == "opaque";
//    }
//
// 2. Opaque type parsing in ParseNonPermType (Lines 545-555)
//    ─────────────────────────────────────────────────────────────────────────
//    if (OpaqueTypeTok(*tok)) {
//      Parser start = parser;
//      Parser after_kw = parser;
//      Advance(after_kw);  // consume opaque
//      ParseElemResult<TypePath> opaque_path = ParseTypePath(after_kw);
//      SPEC_RULE("Parse-Opaque-Type");
//      TypeOpaque opaque;
//      opaque.path = std::move(opaque_path.elem);
//      return {opaque_path.parser,
//              MakeTypeNode(SpanBetween(start, opaque_path.parser), opaque)};
//    }
//
// PARSING FLOW:
// -----------------------------------------------------------------------------
//   1. Check for "opaque" identifier (contextual keyword)
//   2. Advance past "opaque"
//   3. Parse type path via ParseTypePath
//   4. Construct TypeOpaque node
//
// ERROR HANDLING:
// -----------------------------------------------------------------------------
//   - Errors in type path parsing are handled by ParseTypePath
//   - "opaque" must be followed by a valid type path
//
// DEPENDENCIES:
// =============================================================================
// - ParseTypePath function (type_path.cpp)
// - OpaqueTypeTok predicate (keyword_policy.h)
// - MakeTypeNode helper (type_common.cpp)
// - TypeOpaque AST node type
// - TypePath type
// - Parser utilities: Tok, Advance, SpanBetween
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
// - "opaque" is a contextual keyword (identifier in most contexts)
// - OpaqueTypeTok checks for the specific identifier string
// - Opaque types are useful for FFI and module boundaries
// - The underlying type may be known inside the defining module
// - Callers can only use the opaque type through its defined interface
// - Consider if generic opaque types are needed: opaque Container<T>
// - Span covers from "opaque" to end of type path
// =============================================================================
