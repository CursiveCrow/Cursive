// =============================================================================
// MIGRATION MAPPING: bytes_type.cpp
// =============================================================================
// This file should contain parsing logic for bytes types: bytes@Managed
// and bytes@View representing owned and borrowed byte sequences.
//
// SPEC REFERENCE: C0updated.md, Section 3.3.7, Lines 4793-4796
// Section 3.3.7.1 (Bytes State), Lines 4898-4913
// =============================================================================
//
// FORMAL RULES FROM SPEC:
// -----------------------------------------------------------------------------
//
// **(Parse-Bytes-Type)** Lines 4793-4796
// IsIdent(Tok(P))    Lexeme(Tok(P)) = `bytes`
// Γ ⊢ ParseBytesState(Advance(P)) ⇓ (P_1, st_opt)
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseNonPermType(P) ⇓ (P_1, TypeBytes(st_opt))
//
// BYTES STATE RULES (Lines 4900-4913):
// -----------------------------------------------------------------------------
//
// **(Parse-BytesState-None)** Lines 4900-4903
// ¬ IsOp(Tok(P), "@")
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseBytesState(P) ⇓ (P, ⊥)
//
// **(Parse-BytesState-Managed)** Lines 4905-4908
// IsOp(Tok(P), "@")    IsIdent(Tok(Advance(P)))    Lexeme(Tok(Advance(P))) = `Managed`
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseBytesState(P) ⇓ (Advance(Advance(P)), `Managed`)
//
// **(Parse-BytesState-View)** Lines 4910-4913
// IsOp(Tok(P), "@")    IsIdent(Tok(Advance(P)))    Lexeme(Tok(Advance(P))) = `View`
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseBytesState(P) ⇓ (Advance(Advance(P)), `View`)
//
// SEMANTICS:
// - bytes@Managed: heap-allocated, owned byte sequence
// - bytes@View: borrowed view into byte data
// - bytes (no annotation): type depends on context
// - Similar to string but without UTF-8 guarantee
// - Examples: bytes@Managed, bytes@View
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_types.cpp
// =============================================================================
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
//
// 1. ParseBytesState function (Lines 236-256)
//    ─────────────────────────────────────────────────────────────────────────
//    ParseElemResult<std::optional<BytesState>> ParseBytesState(Parser parser) {
//      if (!IsOp(parser, "@")) {
//        SPEC_RULE("Parse-BytesState-None");
//        return {parser, std::nullopt};
//      }
//      Parser next = parser;
//      Advance(next);  // consume @
//      const Token* tok = Tok(next);
//      if (tok && IsIdentTok(*tok) && tok->lexeme == "Managed") {
//        SPEC_RULE("Parse-BytesState-Managed");
//        Advance(next);
//        return {next, BytesState::Managed};
//      }
//      if (tok && IsIdentTok(*tok) && tok->lexeme == "View") {
//        SPEC_RULE("Parse-BytesState-View");
//        Advance(next);
//        return {next, BytesState::View};
//      }
//      EmitParseSyntaxErr(next, TokSpan(next));
//      return {next, std::nullopt};
//    }
//
// 2. Bytes type parsing in ParseNonPermType (Lines 575-584)
//    ─────────────────────────────────────────────────────────────────────────
//    if (lexeme == "bytes") {
//      SPEC_RULE("Parse-Bytes-Type");
//      Parser start = parser;
//      Parser next = parser;
//      Advance(next);  // consume bytes
//      ParseElemResult<std::optional<BytesState>> st = ParseBytesState(next);
//      TypeBytes bytes;
//      bytes.state = st.elem;
//      return {st.parser, MakeTypeNode(SpanBetween(start, st.parser), bytes)};
//    }
//
// PARSING FLOW:
// -----------------------------------------------------------------------------
//   1. Match "bytes" identifier
//   2. Advance past "bytes"
//   3. Call ParseBytesState to check for @State
//   4. If "@" found:
//      a. Advance past "@"
//      b. Expect "Managed" or "View"
//      c. If found, set state accordingly
//      d. If not found, emit error
//   5. Construct TypeBytes node
//
// ERROR HANDLING:
// -----------------------------------------------------------------------------
//   - If "@" is followed by neither "Managed" nor "View":
//     - Emit syntax error diagnostic
//     - Return state as nullopt
//
// DEPENDENCIES:
// =============================================================================
// - MakeTypeNode helper (type_common.cpp)
// - TypeBytes AST node type
// - BytesState enum (Managed, View)
// - Token predicates: IsOp, IsIdentTok
// - Parser utilities: Tok, Advance, SpanBetween, TokSpan
// - EmitParseSyntaxErr diagnostic helper
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
// - "bytes" is NOT a keyword, it's a special type identifier
// - Nearly identical structure to string_type.cpp
// - Consider extracting common @State parsing pattern
// - bytes is for raw binary data, string is for UTF-8 text
// - Both types share the Managed/View ownership model
// - FFI note: bytes types are NOT FfiSafe
// =============================================================================
