// =============================================================================
// MIGRATION MAPPING: parser_state.cpp
// =============================================================================
//
// DESTINATION: cursive/src/02_source/parser/parser_state.cpp
// PURPOSE: Core parser state primitives (MakeParser, Tok, Advance, AtEof, TokSpan)
//
// NOTE: Terminator handling has been split into parser_terminator.cpp
//
// =============================================================================
// SPEC REFERENCES
// =============================================================================
//
// C0updated.md Section 3.3.3 (Lines 2966-3018): Parser State and Judgments
//
// **Parser State Definition (lines 2968-2977)**:
//   PState = <K, i, D, j, d, Delta>
//   - K: Token stream (TokStream)
//   - i: Token index (TokIndex)
//   - D: Doc comment stream (DocStream)
//   - j: Doc index (DocIndex)
//   - d: Depth counter (Depth)
//   - Delta: Diagnostic stream (DiagStream)
//
// **Helper Functions (lines 2979-3002)**:
//   - Tok(P): Returns K[i] if i < |K|, else EOF token (lines 2981-2983)
//   - Advance(P): Returns <K, i+1, D, j, d, Delta> (line 2988)
//   - Clone(P): Returns <K, i, D, j, d, []> (line 2989)
//   - MergeDiag(P_b, P_d, P_s): Merges diagnostics (line 2990)
//
// **Parser Index Invariant (lines 2992-2997)**:
//   PStateOk(P) <=> 0 <= i <= |K|
//   AdvanceOrEOF(P) = Advance(P) if i < |K|, else P
//
// **Span Computation (lines 2999-3002)**:
//   LastConsumed(P, P') = K[i'-1] if i' > i, else Tok(P)
//   SpanBetween(P, P') = SpanFrom(Tok(P), LastConsumed(P, P'))
//
// =============================================================================
// SOURCE FILES
// =============================================================================
//
// PRIMARY SOURCE: cursive-bootstrap/src/02_syntax/parser_stmt.cpp
//   Lines 785-868: Core parser state functions
//
// FUNCTIONS TO MIGRATE:
//
// 1. MakeParser (2 overloads) (lines 785-802):
//    SPEC: Initializes PState = <K, 0, D, 0, 0, []>
//    - First overload: Takes tokens, docs, and source file
//    - Second overload: Takes tokens and source file (empty docs)
//    - Initializes parser.eof with MakeEofToken
//
//    Source lines 785-796 (with docs):
//      Parser MakeParser(const std::vector<Token>& tokens,
//                        const std::vector<DocComment>& docs,
//                        const core::SourceFile& source) {
//        Parser parser;
//        parser.tokens = &tokens;
//        parser.index = 0;
//        parser.docs = &docs;
//        parser.doc_index = 0;
//        parser.depth = 0;
//        parser.eof = MakeEofToken(source);
//        return parser;
//      }
//
//    Source lines 798-802 (without docs):
//      Parser MakeParser(const std::vector<Token>& tokens,
//                        const core::SourceFile& source) {
//        static const std::vector<DocComment> kEmptyDocs;
//        return MakeParser(tokens, kEmptyDocs, source);
//      }
//
// 2. AtEof (lines 804-809):
//    SPEC: Section 3.3.3 line 2983 - EOF condition check
//    - Returns true if tokens is null OR index >= tokens->size()
//
//    Source:
//      bool AtEof(const Parser& parser) {
//        if (!parser.tokens) { return true; }
//        return parser.index >= parser.tokens->size();
//      }
//
// 3. Tok (lines 811-819):
//    SPEC: Section 3.3.3 lines 2981-2983
//    - Returns pointer to current token or nullptr if at EOF
//
//    Source:
//      const Token* Tok(const Parser& parser) {
//        if (!parser.tokens) { return nullptr; }
//        if (parser.index >= parser.tokens->size()) { return nullptr; }
//        return &(*parser.tokens)[parser.index];
//      }
//
// 4. TokSpan (lines 821-826):
//    - Returns span of current token, or EOF span if at end
//
//    Source:
//      const core::Span& TokSpan(const Parser& parser) {
//        if (const auto* tok = Tok(parser)) { return tok->span; }
//        return parser.eof.span;
//      }
//
// 5. Advance (lines 828-833):
//    SPEC: Section 3.3.3 line 2988
//    - Increments parser.index if not at EOF
//
//    Source:
//      void Advance(Parser& parser) {
//        if (AtEof(parser)) { return; }
//        parser.index += 1;
//      }
//
// =============================================================================
// SPLIT FILES
// =============================================================================
//
// Terminator handling has been moved to parser_terminator.cpp:
//   - ConsumeTerminatorOpt (lines 834-856)
//   - ConsumeTerminatorReq (lines 858-868)
//
// See parser_terminator.cpp for full documentation of those functions.
//
// =============================================================================
// ADDITIONAL SOURCE: cursive-bootstrap/include/cursive0/02_syntax/parser.h
//   Lines 25-34: Parser struct definition
//
// Parser STRUCT (header lines 25-34):
//   struct Parser {
//     const std::vector<Token>* tokens = nullptr;
//     std::shared_ptr<std::vector<Token>> owned_tokens;
//     std::size_t index = 0;
//     const std::vector<DocComment>* docs = nullptr;
//     std::size_t doc_index = 0;
//     std::size_t depth = 0;
//     core::DiagnosticStream diags;
//     EofToken eof;
//   };
//
// NOTE: owned_tokens is used when tokens need to be modified (e.g., SplitShiftR)
//
// =============================================================================
// DEPENDENCIES
// =============================================================================
//
// REQUIRED HEADERS:
//   - cursive/include/02_source/parser/parser.hpp
//   - cursive/include/02_source/lexer/token.hpp
//   - cursive/include/02_source/ast/doc_comment.hpp
//   - cursive/include/00_core/span.hpp
//   - cursive/include/00_core/source_text.hpp
//
// REQUIRED FUNCTIONS:
//   - MakeEofToken (lexer.cpp or token.cpp)
//
// TYPES REQUIRED:
//   - Parser (struct)
//   - Token, TokenKind, EofToken
//   - DocComment
//   - core::Span, core::SourceFile
//   - core::DiagnosticStream
//
// =============================================================================
// REFACTORING NOTES
// =============================================================================
//
// 1. SPLIT FROM parser_stmt.cpp:
//    The source has these functions in parser_stmt.cpp (lines 785-868).
//    Core primitives are in this file; terminator handling is in
//    parser_terminator.cpp.
//
// 2. NO SPEC RULE MARKERS:
//    The core primitives in this file don't have SPEC_RULE markers.
//    They implement fundamental operations from the spec directly.
//    SPEC_RULE markers for terminator handling are in parser_terminator.cpp.
//
// 3. OWNED_TOKENS PATTERN:
//    The owned_tokens member enables token stream modification (used by
//    SplitShiftR in parser_angle.cpp). When tokens are modified, the new
//    vector is stored in owned_tokens and tokens points to it.
//
// 4. DEPTH TRACKING:
//    parser.depth is used for nested structure parsing but is currently
//    not utilized in the migrated functions. Ensure it's properly maintained
//    in block/brace parsing.
//
// 5. DOC_INDEX:
//    parser.doc_index tracks position in documentation comments. Ensure
//    this is used correctly in parser_docs.cpp for doc association.
//
// 6. PURE PRIMITIVES:
//    All functions in this file are pure primitives with no external
//    dependencies beyond MakeEofToken. They form the foundation that
//    all other parser functions build upon.
//
// =============================================================================
