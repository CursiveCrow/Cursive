// =============================================================================
// MIGRATION MAPPING: pattern_common.cpp
// =============================================================================
// This file should contain shared helpers and utilities used across all
// pattern parsing functions.
//
// SPEC REFERENCE: C0updated.md, Section 3.3.9, Lines 6065-6125 (main rules)
// HELPER RULES: Section 3.3.9.1, Lines 6127-6155 (pattern list rules)
// =============================================================================
//
// MAIN ENTRY POINT RULES:
// -----------------------------------------------------------------------------
// **(Parse-Pattern)** Lines 6067-6070
// Gamma |- ParsePatternRange(P) => (P_1, pat)
// --------------------------------------------------------------------------
// Gamma |- ParsePattern(P) => (P_1, pat)
//
// **(Parse-Pattern-Err)** Lines 6072-6075
// c = Code(Parse-Syntax-Err)    Gamma |- Emit(c, Tok(P).span)
// --------------------------------------------------------------------------
// Gamma |- ParsePattern(P) => (P, WildcardPattern)
//
// PATTERN LIST RULES (Lines 6131-6154):
// -----------------------------------------------------------------------------
// **(Parse-PatternList-Empty)** Lines 6131-6134
// IsPunc(Tok(P), ")")
// --------------------------------------------------------------------------
// Gamma |- ParsePatternList(P) => (P, [])
//
// **(Parse-PatternList-Cons)** Lines 6136-6139
// Gamma |- ParsePattern(P) => (P_1, p)
// Gamma |- ParsePatternListTail(P_1, [p]) => (P_2, ps)
// --------------------------------------------------------------------------
// Gamma |- ParsePatternList(P) => (P_2, ps)
//
// **(Parse-PatternListTail-End)** Lines 6141-6144
// IsPunc(Tok(P), ")")
// --------------------------------------------------------------------------
// Gamma |- ParsePatternListTail(P, xs) => (P, xs)
//
// **(Parse-PatternListTail-TrailingComma)** Lines 6146-6149
// IsPunc(Tok(P), ",")    IsPunc(Tok(Advance(P)), ")")
// TrailingCommaAllowed(P_0, P, {Punctuator(")")})
// --------------------------------------------------------------------------
// Gamma |- ParsePatternListTail(P, xs) => (Advance(P), xs)
//
// **(Parse-PatternListTail-Comma)** Lines 6151-6154
// IsPunc(Tok(P), ",")
// Gamma |- ParsePattern(Advance(P)) => (P_1, p)
// Gamma |- ParsePatternListTail(P_1, xs ++ [p]) => (P_2, ys)
// --------------------------------------------------------------------------
// Gamma |- ParsePatternListTail(P, xs) => (P_2, ys)
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_patterns.cpp
// =============================================================================
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
//
// 1. Helper functions and utilities (Lines 14-64)
//    -------------------------------------------------------------------------
//    [[maybe_unused]] void EmitUnsupportedConstruct(Parser& parser) { ... }
//
//    bool IsOp(const Parser& parser, std::string_view op) {
//      const Token* tok = Tok(parser);
//      return tok && IsOpTok(*tok, op);
//    }
//
//    bool IsPunc(const Parser& parser, std::string_view punc) {
//      const Token* tok = Tok(parser);
//      return tok && IsPuncTok(*tok, punc);
//    }
//
//    bool IsLiteralToken(const Token& tok) {
//      return tok.kind == TokenKind::IntLiteral ||
//             tok.kind == TokenKind::FloatLiteral ||
//             tok.kind == TokenKind::StringLiteral ||
//             tok.kind == TokenKind::CharLiteral ||
//             tok.kind == TokenKind::BoolLiteral ||
//             tok.kind == TokenKind::NullLiteral;
//    }
//
//    bool IsPatternStart(const Token& tok) {
//      if (IsLiteralToken(tok) || IsIdentTok(tok)) {
//        return true;
//      }
//      if (tok.kind == TokenKind::Punctuator) {
//        return tok.lexeme == "(";
//      }
//      return tok.kind == TokenKind::Operator && tok.lexeme == "@";
//    }
//
//    void SkipNewlines(Parser& parser) {
//      while (Tok(parser) && Tok(parser)->kind == TokenKind::Newline) {
//        Advance(parser);
//      }
//    }
//
//    PatternPtr MakePattern(const core::Span& span, PatternNode node) {
//      auto pat = std::make_shared<Pattern>();
//      pat->span = span;
//      pat->node = std::move(node);
//      return pat;
//    }
//
//    NOTE: IsOp, IsPunc, SkipNewlines may already exist in parser utilities.
//    MakePattern should be placed here or in a central pattern helper location.
//
// 2. ParsePatternList function (Lines 90-101)
//    -------------------------------------------------------------------------
//    ParseElemResult<std::vector<PatternPtr>> ParsePatternList(Parser parser) {
//      SkipNewlines(parser);
//      if (IsPunc(parser, ")")) {
//        SPEC_RULE("Parse-PatternList-Empty");
//        return {parser, {}};
//      }
//      SPEC_RULE("Parse-PatternList-Cons");
//      ParseElemResult<PatternPtr> first = ParsePattern(parser);
//      std::vector<PatternPtr> elems;
//      elems.push_back(first.elem);
//      return ParsePatternListTail(first.parser, std::move(elems));
//    }
//
// 3. ParsePatternListTail function (Lines 104-130)
//    -------------------------------------------------------------------------
//    ParseElemResult<std::vector<PatternPtr>> ParsePatternListTail(
//        Parser parser, std::vector<PatternPtr> xs) {
//      SkipNewlines(parser);
//      if (IsPunc(parser, ")")) {
//        SPEC_RULE("Parse-PatternListTail-End");
//        return {parser, xs};
//      }
//      if (IsPunc(parser, ",")) {
//        const TokenKindMatch end_set[] = {MatchPunct(")")};
//        Parser after = parser;
//        Advance(after);
//        SkipNewlines(after);
//        if (IsPunc(after, ")")) {
//          SPEC_RULE("Parse-PatternListTail-TrailingComma");
//          EmitTrailingCommaErr(parser, end_set);
//          after.diags = parser.diags;
//          return {after, xs};
//        }
//        SPEC_RULE("Parse-PatternListTail-Comma");
//        ParseElemResult<PatternPtr> elem = ParsePattern(after);
//        xs.push_back(elem.elem);
//        return ParsePatternListTail(elem.parser, std::move(xs));
//      }
//      EmitParseSyntaxErr(parser, TokSpan(parser));
//      return {parser, xs};
//    }
//
// 4. ParsePattern entry point (Lines 435-444)
//    -------------------------------------------------------------------------
//    ParseElemResult<PatternPtr> ParsePattern(Parser parser) {
//      const Token* tok = Tok(parser);
//      if (!tok || !IsPatternStart(*tok)) {
//        SPEC_RULE("Parse-Pattern-Err");
//        EmitParseSyntaxErr(parser, TokSpan(parser));
//        return {parser, MakePattern(TokSpan(parser), WildcardPattern{})};
//      }
//      SPEC_RULE("Parse-Pattern");
//      return ParsePatternRange(parser);
//    }
//
// 5. ParsePatternAtom dispatch function (Lines 295-413)
//    -------------------------------------------------------------------------
//    This function is the main pattern dispatch point. It checks token types
//    and delegates to specific pattern parsers. The order of checks is critical:
//
//    1. Literal patterns (Line 302)
//    2. TypedPattern (identifier + ":") (Lines 313-326)
//    3. WildcardPattern ("_" alone) (Lines 329-334)
//    4. EnumPattern (identifier + "::") (Lines 336-349)
//    5. TuplePattern ("(") (Lines 351-366)
//    6. RecordPattern (path + "{") (Lines 368-388)
//    7. ModalPattern ("@") (Lines 390-400)
//    8. IdentifierPattern (fallback) (Lines 402-409)
//
//    NOTE: This may be placed here or split across individual pattern files
//    with a central dispatch function.
//
// DEPENDENCIES:
// =============================================================================
// - Pattern AST node types (all pattern variants)
// - PatternNode variant type
// - PatternPtr = shared_ptr<Pattern>
// - Token, TokenKind types
// - Parser state type
// - ParseElemResult template
// - Span, SpanBetween utilities
// - Error emission functions (EmitParseSyntaxErr, EmitTrailingCommaErr)
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
// - MakePattern creates a Pattern with span and node
// - IsPatternStart determines if a token can begin a pattern
// - ParsePatternList/ListTail are reused by tuple pattern parsing (indirectly)
// - The dispatch order in ParsePatternAtom is critical for disambiguation
// - Consider whether to keep ParsePatternAtom here or in a separate file
// - Helper functions like IsOp, IsPunc may be shared with expr/stmt parsing
// - SkipNewlines is used throughout for multi-line pattern support
// =============================================================================
//
// CROSS-FILE DEPENDENCIES:
// =============================================================================
// Other pattern files depend on functions defined here:
// - literal_pattern.cpp: MakePattern, IsLiteralToken
// - wildcard_pattern.cpp: MakePattern
// - identifier_pattern.cpp: MakePattern
// - typed_pattern.cpp: MakePattern, SpanBetween, ParseType (external)
// - tuple_pattern.cpp: MakePattern, ParsePatternListTail, SkipNewlines
// - record_pattern.cpp: MakePattern, ParseFieldPatternList (local or record)
// - enum_pattern.cpp: MakePattern, ParseTuplePatternElems, ParseFieldPatternList
// - modal_pattern.cpp: MakePattern, ParseFieldPatternList
// - range_pattern.cpp: MakePattern, ParsePatternAtom
// =============================================================================
