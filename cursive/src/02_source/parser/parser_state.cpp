// =============================================================================
// parser_state.cpp - Core Parser State Primitives
// =============================================================================
//
// SPEC REFERENCE: CursiveSpecification.md Section 3.3.3 (Lines 2966-3018)
//
// This file implements the fundamental parser state operations:
//   - MakeParser: Initialize parser state (PState = <K, 0, D, 0, 0, []>)
//   - AtEof: Check if at end of token stream
//   - Tok: Get current token (K[i] if i < |K|, else EOF)
//   - TokSpan: Get span of current token
//   - Advance: Move to next token (index += 1)
//
// =============================================================================

#include "02_source/parser/parser.h"

#include "02_source/lexer/token.h"

namespace cursive::ast {

// Use lexer types
using cursive::lexer::DocComment;
using cursive::lexer::MakeEofToken;
using cursive::lexer::Token;

// =============================================================================
// MakeParser - Initialize parser state
// =============================================================================
//
// SPEC: Section 3.3.3 - Initializes PState = <K, 0, D, 0, 0, []>
//   - K: Token stream
//   - i: Token index (0)
//   - D: Doc comment stream
//   - j: Doc index (0)
//   - d: Depth counter (0)
//   - Delta: Diagnostic stream (empty)

Parser MakeParser(const std::vector<Token>& tokens,
                  const std::vector<DocComment>& docs,
                  const core::SourceFile& source) {
  Parser parser;
  parser.tokens = &tokens;
  parser.index = 0;
  parser.docs = &docs;
  parser.doc_index = 0;
  parser.depth = 0;
  parser.eof = MakeEofToken(source);
  return parser;
}

Parser MakeParser(const std::vector<Token>& tokens,
                  const core::SourceFile& source) {
  static const std::vector<DocComment> kEmptyDocs;
  return MakeParser(tokens, kEmptyDocs, source);
}

// =============================================================================
// AtEof - End of file check
// =============================================================================
//
// SPEC: Section 3.3.3 line 2983 - EOF condition check
// Returns true if tokens is null OR index >= tokens->size()

bool AtEof(const Parser& parser) {
  if (!parser.tokens) {
    return true;
  }
  return parser.index >= parser.tokens->size();
}

// =============================================================================
// Tok - Get current token
// =============================================================================
//
// SPEC: Section 3.3.3 lines 2981-2983
// Returns K[i] if i < |K|, else EOF token
// Returns pointer to current token or nullptr if at EOF

const Token* Tok(const Parser& parser) {
  if (!parser.tokens || parser.index >= parser.tokens->size()) {
    return &parser.eof;
  }
  return &(*parser.tokens)[parser.index];
}

// =============================================================================
// TokSpan - Get span of current token
// =============================================================================
//
// Returns span of current token, or EOF span if at end

const core::Span& TokSpan(const Parser& parser) {
  return Tok(parser)->span;
}

// =============================================================================
// Advance - Move to next token
// =============================================================================
//
// SPEC: Section 3.3.3 line 2988
// Advance(P) = <K, i+1, D, j, d, Delta>
// Increments parser.index if not at EOF

void Advance(Parser& parser) {
  if (AtEof(parser)) {
    return;
  }
  parser.index += 1;
}

}  // namespace cursive::ast
