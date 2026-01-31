// =============================================================================
// MIGRATION MAPPING: parser_consume.cpp
// =============================================================================
//
// DESTINATION: cursive/src/02_source/parser/parser_consume.cpp
// PURPOSE: Token consumption helpers (ConsumeKind, ConsumeKeyword, etc.)
//
// =============================================================================
// SPEC REFERENCES
// =============================================================================
//
// C0updated.md Section 3.3.5 (Lines 3385-3438): Token Consumption (Small-Step)
//
// **Consume State Machine (lines 3387-3388)**:
//   ConsumeState = {Consume(P, k), ConsumeDone(P)}
//
// **Tok-Consume-Kind (lines 3390-3393)**:
//   Tok(P).kind = k
//   ----------------------------------------
//   <Consume(P, k)> -> <ConsumeDone(Advance(P))>
//
// **Tok-Consume-Keyword (lines 3395-3398)**:
//   IsKw(Tok(P), s)
//   ----------------------------------------
//   <Consume(P, Keyword(s))> -> <ConsumeDone(Advance(P))>
//
// **Tok-Consume-Operator (lines 3400-3403)**:
//   IsOp(Tok(P), s)
//   ----------------------------------------
//   <Consume(P, Operator(s))> -> <ConsumeDone(Advance(P))>
//
// **Tok-Consume-Punct (lines 3405-3408)**:
//   IsPunc(Tok(P), s)
//   ----------------------------------------
//   <Consume(P, Punctuator(s))> -> <ConsumeDone(Advance(P))>
//
// **List Parsing (lines 3410-3427)**:
//   ListState = {ListStart(P), ListScan(P, xs), ListDone(P, xs)}
//   - List-Start: <ListStart(P)> -> <ListScan(P, [])>
//   - List-Cons: <ListScan(P, xs)> -> <ListScan(P', xs ++ [x])>
//   - List-Done: Tok(P) in EndSet => <ListScan(P, xs)> -> <ListDone(P, xs)>
//
// **Trailing Comma Rules (lines 3430-3438)**:
//   TrailingComma(P, EndSet) <=> IsPunc(Tok(P), ",") && Tok(Advance(P)) in EndSet
//   TrailingCommaAllowed(P_0, P, EndSet) <=> TrailingComma(P, EndSet) && TokLine(Tok(P)) < TokLine(Tok(Advance(P)))
//   Trailing-Comma-Err: TrailingComma && !TrailingCommaAllowed => Emit(Code(Trailing-Comma-Err))
//
// =============================================================================
// SOURCE FILES
// =============================================================================
//
// PRIMARY SOURCE: cursive-bootstrap/src/02_syntax/parser_consume.cpp
//   Lines 1-125: Complete file
//
// FUNCTIONS TO MIGRATE:
//
// 1. Anonymous namespace helper (lines 11-17):
//    MatchLexeme: Determines if token kind requires lexeme matching
//
//    Source:
//      bool MatchLexeme(TokenKind kind) {
//        return kind == TokenKind::Keyword ||
//               kind == TokenKind::Operator ||
//               kind == TokenKind::Punctuator;
//      }
//
// 2. TokenMatches (lines 20-28):
//    Checks if a token matches a TokenKindMatch specification
//
//    Source:
//      bool TokenMatches(const Token& tok, const TokenKindMatch& match) {
//        if (tok.kind != match.kind) { return false; }
//        if (MatchLexeme(match.kind)) { return tok.lexeme == match.lexeme; }
//        return true;
//      }
//
// 3. TokenInEndSet (lines 30-38):
//    Checks if token is in an end set for list parsing
//
//    Source:
//      bool TokenInEndSet(const Token& tok, std::span<const TokenKindMatch> end_set) {
//        for (const auto& match : end_set) {
//          if (TokenMatches(tok, match)) { return true; }
//        }
//        return false;
//      }
//
// 4. ConsumeKind (lines 40-48):
//    SPEC: Tok-Consume-Kind (lines 3390-3393)
//
//    Source:
//      bool ConsumeKind(Parser& parser, TokenKind kind) {
//        SPEC_RULE("Tok-Consume-Kind");
//        const Token* tok = Tok(parser);
//        if (!tok || tok->kind != kind) { return false; }
//        Advance(parser);
//        return true;
//      }
//
// 5. ConsumeKeyword (lines 50-58):
//    SPEC: Tok-Consume-Keyword (lines 3395-3398)
//
//    Source:
//      bool ConsumeKeyword(Parser& parser, std::string_view keyword) {
//        SPEC_RULE("Tok-Consume-Keyword");
//        const Token* tok = Tok(parser);
//        if (!tok || tok->kind != TokenKind::Keyword || tok->lexeme != keyword) { return false; }
//        Advance(parser);
//        return true;
//      }
//
// 6. ConsumeOperator (lines 60-68):
//    SPEC: Tok-Consume-Operator (lines 3400-3403)
//
//    Source:
//      bool ConsumeOperator(Parser& parser, std::string_view op) {
//        SPEC_RULE("Tok-Consume-Operator");
//        const Token* tok = Tok(parser);
//        if (!tok || tok->kind != TokenKind::Operator || tok->lexeme != op) { return false; }
//        Advance(parser);
//        return true;
//      }
//
// 7. ConsumePunct (lines 70-78):
//    SPEC: Tok-Consume-Punct (lines 3405-3408)
//
//    Source:
//      bool ConsumePunct(Parser& parser, std::string_view punct) {
//        SPEC_RULE("Tok-Consume-Punct");
//        const Token* tok = Tok(parser);
//        if (!tok || tok->kind != TokenKind::Punctuator || tok->lexeme != punct) { return false; }
//        Advance(parser);
//        return true;
//      }
//
// 8. TrailingComma (lines 80-92):
//    SPEC: TrailingComma predicate (line 3430)
//    Detects trailing comma before end delimiter
//
//    Source:
//      bool TrailingComma(const Parser& parser, std::span<const TokenKindMatch> end_set) {
//        const Token* tok = Tok(parser);
//        if (!tok || tok->kind != TokenKind::Punctuator || tok->lexeme != ",") { return false; }
//        const Parser next = AdvanceOrEOF(parser);
//        const Token* next_tok = Tok(next);
//        if (!next_tok) { return false; }
//        return TokenInEndSet(*next_tok, end_set);
//      }
//
// 9. TrailingCommaAllowed (lines 94-106):
//    SPEC: TrailingCommaAllowed predicate (line 3433)
//    Trailing comma allowed only when closing delimiter is on different line
//
//    Source:
//      bool TrailingCommaAllowed(const Parser& parser, std::span<const TokenKindMatch> end_set) {
//        if (!TrailingComma(parser, end_set)) { return false; }
//        const Token* comma = Tok(parser);
//        const Parser next = AdvanceOrEOF(parser);
//        const Token* end_tok = Tok(next);
//        if (!comma || !end_tok) { return false; }
//        return comma->span.start_line < end_tok->span.start_line;
//      }
//
// 10. EmitTrailingCommaErr (lines 108-123):
//     SPEC: Trailing-Comma-Err (lines 3435-3438)
//     Emits E-SRC-0521 for invalid trailing comma
//
//     Source:
//       bool EmitTrailingCommaErr(Parser& parser, std::span<const TokenKindMatch> end_set) {
//         SPEC_RULE("Trailing-Comma-Err");
//         if (!TrailingComma(parser, end_set)) { return false; }
//         if (TrailingCommaAllowed(parser, end_set)) { return false; }
//         auto diag = core::MakeDiagnostic("E-SRC-0521", TokSpan(parser));
//         if (!diag) { return true; }
//         parser.diags = core::Emit(parser.diags, *diag);
//         return true;
//       }
//
// =============================================================================
// ADDITIONAL SOURCE: Header definitions
// =============================================================================
//
// From cursive-bootstrap/include/cursive0/02_syntax/parser.h (lines 77-91):
//
// TokenKindMatch struct:
//   struct TokenKindMatch {
//     TokenKind kind = TokenKind::Unknown;
//     std::string_view lexeme;
//   };
//
// Factory functions:
//   inline TokenKindMatch MatchKind(TokenKind kind) { return {kind, {}}; }
//   inline TokenKindMatch MatchKeyword(std::string_view s) { return {TokenKind::Keyword, s}; }
//   inline TokenKindMatch MatchOperator(std::string_view s) { return {TokenKind::Operator, s}; }
//   inline TokenKindMatch MatchPunct(std::string_view s) { return {TokenKind::Punctuator, s}; }
//
// =============================================================================
// DEPENDENCIES
// =============================================================================
//
// REQUIRED HEADERS:
//   - cursive/include/02_source/parser/parser.hpp
//   - cursive/include/02_source/lexer/token.hpp
//   - cursive/include/00_core/diagnostics.hpp
//   - cursive/include/00_core/diagnostic_messages.hpp
//   - <string_view>
//   - <span>
//
// REQUIRED FUNCTIONS:
//   - Tok (parser_state.cpp)
//   - Advance (parser_state.cpp)
//   - AdvanceOrEOF (parser.cpp)
//   - TokSpan (parser_state.cpp)
//   - core::MakeDiagnostic (00_core/diagnostic_messages.cpp)
//   - core::Emit (00_core/diagnostics.cpp)
//
// TYPES REQUIRED:
//   - Parser
//   - Token, TokenKind
//   - TokenKindMatch
//   - core::DiagnosticStream, core::Span
//
// =============================================================================
// REFACTORING NOTES
// =============================================================================
//
// 1. SPEC RULE MARKERS:
//    All SPEC_RULE calls must be preserved:
//    - "Tok-Consume-Kind" (line 41)
//    - "Tok-Consume-Keyword" (line 51)
//    - "Tok-Consume-Operator" (line 61)
//    - "Tok-Consume-Punct" (line 71)
//    - "Trailing-Comma-Err" (line 110)
//
// 2. LIST PARSING TEMPLATES:
//    The spec defines list parsing operations (List-Start, List-Cons, List-Done).
//    These are implemented as templates in parser.h (lines 164-199):
//      - ListState<Elem>
//      - ListStart<Elem>(Parser)
//      - ListCons<Elem, ParseElemFn>(ListState, parse_elem)
//      - ListDone<Elem>(ListState, end_set)
//    Consider whether these should move to parser_consume.hpp.
//
// 3. TRAILING COMMA LOGIC:
//    The trailing comma rules are critical for Cursive syntax:
//    - Trailing commas ONLY allowed when closing delimiter is on different line
//    - Single-line trailing commas are errors (E-SRC-0521)
//    Ensure all list parsing call sites use EmitTrailingCommaErr.
//
// 4. TOKENKINDMATCH:
//    The TokenKindMatch struct and factory functions should be in a header
//    (parser.hpp or a separate token_match.hpp). The inline factories are
//    convenient for creating end sets.
//
// 5. STD::SPAN USAGE:
//    The code uses std::span<const TokenKindMatch> for end sets. Ensure
//    C++20 compatibility or provide a fallback for older standards.
//
// 6. NAMESPACE:
//    Source uses cursive0::syntax. Update to target namespace.
//
// =============================================================================
