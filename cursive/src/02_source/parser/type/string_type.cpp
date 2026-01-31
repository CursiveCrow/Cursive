// =============================================================================
// MIGRATION MAPPING: string_type.cpp
// =============================================================================
// This file should contain parsing logic for string types: string@Managed
// and string@View representing owned and borrowed UTF-8 strings.
//
// SPEC REFERENCE: C0updated.md, Section 3.3.7, Lines 4788-4791
// Section 3.3.7.1 (String State), Lines 4881-4896
// =============================================================================
//
// FORMAL RULES FROM SPEC:
// -----------------------------------------------------------------------------
//
// **(Parse-String-Type)** Lines 4788-4791
// IsIdent(Tok(P))    Lexeme(Tok(P)) = `string`
// Γ ⊢ ParseStringState(Advance(P)) ⇓ (P_1, st_opt)
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseNonPermType(P) ⇓ (P_1, TypeString(st_opt))
//
// STRING STATE RULES (Lines 4883-4896):
// -----------------------------------------------------------------------------
//
// **(Parse-StringState-None)** Lines 4883-4886
// ¬ IsOp(Tok(P), "@")
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseStringState(P) ⇓ (P, ⊥)
//
// **(Parse-StringState-Managed)** Lines 4888-4891
// IsOp(Tok(P), "@")    IsIdent(Tok(Advance(P)))    Lexeme(Tok(Advance(P))) = `Managed`
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseStringState(P) ⇓ (Advance(Advance(P)), `Managed`)
//
// **(Parse-StringState-View)** Lines 4893-4896
// IsOp(Tok(P), "@")    IsIdent(Tok(Advance(P)))    Lexeme(Tok(Advance(P))) = `View`
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseStringState(P) ⇓ (Advance(Advance(P)), `View`)
//
// SEMANTICS:
// - string@Managed: heap-allocated, owned UTF-8 string
// - string@View: borrowed view into string data (like &str)
// - string (no annotation): type depends on context
// - String literals produce string@View
// - Examples: string@Managed, string@View
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_types.cpp
// =============================================================================
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
//
// 1. ParseStringState function (Lines 214-234)
//    ─────────────────────────────────────────────────────────────────────────
//    ParseElemResult<std::optional<StringState>> ParseStringState(Parser parser) {
//      if (!IsOp(parser, "@")) {
//        SPEC_RULE("Parse-StringState-None");
//        return {parser, std::nullopt};
//      }
//      Parser next = parser;
//      Advance(next);  // consume @
//      const Token* tok = Tok(next);
//      if (tok && IsIdentTok(*tok) && tok->lexeme == "Managed") {
//        SPEC_RULE("Parse-StringState-Managed");
//        Advance(next);
//        return {next, StringState::Managed};
//      }
//      if (tok && IsIdentTok(*tok) && tok->lexeme == "View") {
//        SPEC_RULE("Parse-StringState-View");
//        Advance(next);
//        return {next, StringState::View};
//      }
//      EmitParseSyntaxErr(next, TokSpan(next));
//      return {next, std::nullopt};
//    }
//
// 2. String type parsing in ParseNonPermType (Lines 565-574)
//    ─────────────────────────────────────────────────────────────────────────
//    if (lexeme == "string") {
//      SPEC_RULE("Parse-String-Type");
//      Parser start = parser;
//      Parser next = parser;
//      Advance(next);  // consume string
//      ParseElemResult<std::optional<StringState>> st = ParseStringState(next);
//      TypeString str;
//      str.state = st.elem;
//      return {st.parser, MakeTypeNode(SpanBetween(start, st.parser), str)};
//    }
//
// PARSING FLOW:
// -----------------------------------------------------------------------------
//   1. Match "string" identifier
//   2. Advance past "string"
//   3. Call ParseStringState to check for @State
//   4. If "@" found:
//      a. Advance past "@"
//      b. Expect "Managed" or "View"
//      c. If found, set state accordingly
//      d. If not found, emit error
//   5. Construct TypeString node
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
// - TypeString AST node type
// - StringState enum (Managed, View)
// - Token predicates: IsOp, IsIdentTok
// - Parser utilities: Tok, Advance, SpanBetween, TokSpan
// - EmitParseSyntaxErr diagnostic helper
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
// - "string" is NOT a keyword, it's a special type identifier
// - State annotation pattern (@State) is shared with bytes and Ptr types
// - Consider extracting common state parsing logic
// - Without @State, the type needs inference or explicit context
// - View strings are borrowed and don't own their data
// - Managed strings require heap allocation capability
// =============================================================================
