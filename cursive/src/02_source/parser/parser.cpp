// =============================================================================
// parser.cpp - Top-Level Parsing Entry Points
// =============================================================================
//
// SPEC REFERENCE: CursiveSpecification.md Section 3.3.1, 3.3.3, 3.3.6
//
// This file implements top-level parsing operations:
//   - AdvanceOrEOF: Advance if not at EOF
//   - Clone: Clone parser state with cleared diagnostics
//   - MergeDiag: Merge diagnostics from multiple parsers
//   - PStateOk: Validate parser index invariant
//   - SpanBetween: Compute span between two parser states
//   - ParseItems: Parse sequence of items
//   - ParseFile: Parse complete source file
//   - ParseFileBestEffort: Check if file was parsed
//   - ParseFileOk: Check if file parsed without errors
//
// =============================================================================

#include "02_source/parser/parser.h"

#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string_view>

#include "00_core/assert_spec.h"
#include "00_core/diagnostic_messages.h"
#include "00_core/process_config.h"
#include "00_core/diagnostics.h"
#include "00_core/span.h"
#include "02_source/lexer/lexer.h"

namespace cursive::ast {

// Use lexer types
using cursive::lexer::DocComment;
using cursive::lexer::FilterNewlines;
using cursive::lexer::Token;
using cursive::lexer::TokenKind;
using cursive::lexer::Tokenize;
using cursive::lexer::TokenizeResult;
using cursive::lexer::UnsafeSpans;

// Forward declarations for doc handling (implemented in parser_docs.cpp)
std::vector<DocComment> ModuleDocs(const std::vector<DocComment>* docs);
void AttachLineDocs(std::vector<ASTItem>& items,
                    const std::vector<DocComment>* docs);

namespace {

// =============================================================================
// AppendDiags - Merge diagnostic streams
// =============================================================================

void AppendDiags(core::DiagnosticStream& out,
                 const core::DiagnosticStream& add) {
  for (const auto& diag : add) {
    core::Emit(out, diag);
  }
}

// =============================================================================
// SpanFrom - Create span from start/end tokens
// =============================================================================

core::Span SpanFrom(const Token& start, const Token& end) {
  core::Span span = start.span;
  span.end_offset = end.span.end_offset;
  span.end_line = end.span.end_line;
  span.end_col = end.span.end_col;
  return span;
}

}  // namespace

// =============================================================================
// AdvanceOrEOF - Advance if not at EOF
// =============================================================================
//
// SPEC: Section 3.3.3 lines 2995-2997
//   AdvanceOrEOF(P) = Advance(P) if i < |K|, else P

Parser AdvanceOrEOF(const Parser& parser) {
  if (AtEof(parser)) {
    return parser;
  }
  Parser next = parser;
  next.index += 1;
  return next;
}

// =============================================================================
// Clone - Clone parser state with cleared diagnostics
// =============================================================================
//
// SPEC: Section 3.3.3 line 2989
//   Clone(P) = <K, i, D, j, d, []>

Parser Clone(const Parser& parser) {
  Parser out = parser;
  out.diags.clear();
  return out;
}

// =============================================================================
// MergeDiag - Merge diagnostics from multiple parsers
// =============================================================================
//
// SPEC: Section 3.3.3 line 2990
//   MergeDiag(P_b, P_d, P_s): Merges diagnostics from base and diag into src

Parser MergeDiag(const Parser& base, const Parser& diag, const Parser& src) {
  Parser out = src;
  out.diags = base.diags;
  AppendDiags(out.diags, diag.diags);
  return out;
}

// =============================================================================
// PStateOk - Parser index invariant validation
// =============================================================================
//
// SPEC: Section 3.3.3 line 2993
//   PStateOk(P) <=> 0 <= i <= |K|

bool PStateOk(const Parser& parser) {
  if (!parser.tokens) {
    return parser.index == 0;
  }
  return parser.index <= parser.tokens->size();
}

// =============================================================================
// SpanBetween - Compute span between two parser states
// =============================================================================
//
// SPEC: Section 3.3.3 line 3002
//   LastConsumed(P, P') = K[i'-1] if i' > i, else Tok(P)
//   SpanBetween(P, P') = SpanFrom(Tok(P), LastConsumed(P, P'))

core::Span SpanBetween(const Parser& start, const Parser& end) {
  Token start_tok = *Tok(start);
  const std::vector<Token>* tokens =
      end.tokens ? end.tokens : start.tokens;
  Token end_tok = start_tok;
  if (tokens && end.index > start.index && end.index - 1 < tokens->size()) {
    end_tok = (*tokens)[end.index - 1];
  }
  return SpanFrom(start_tok, end_tok);
}

// =============================================================================
// ParseItemsInternal - Internal recursive item parsing
// =============================================================================
//
// SPEC: Section 3.3.6 lines 3454-3464
//   - ParseItems-Empty: Empty item list at EOF
//   - ParseItems-Cons: Recursive item list construction

static ParseItemsResult ParseItemsInternal(
    Parser parser,
    const std::vector<DocComment>& module_docs) {
  ParseItemsResult result;
  result.module_doc = module_docs;

  Parser cur = parser;
  for (;;) {
    while (!AtEof(cur)) {
      const Token* tok = Tok(cur);
      if (!tok || tok->kind != TokenKind::Newline) {
        break;
      }
      Advance(cur);
    }

    if (AtEof(cur)) {
      SPEC_RULE("ParseItems-Empty");
      result.parser = cur;
      return result;
    }

    SPEC_RULE("ParseItems-Cons");
    if (core::IsDebugEnabled("parse")) {
      const Token* tok = Tok(cur);
      std::string_view lex = tok ? tok->lexeme : "<eof>";
      std::uint8_t b0 = 0;
      std::uint8_t b1 = 0;
      if (tok && !tok->lexeme.empty()) {
        b0 = static_cast<std::uint8_t>(tok->lexeme[0]);
        if (tok->lexeme.size() > 1) {
          b1 = static_cast<std::uint8_t>(tok->lexeme[1]);
        }
      }
      std::cerr << "[cursive] parse-items: index=" << cur.index
                << " tok=" << lex << " kind="
                << (tok ? static_cast<int>(tok->kind) : -1)
                << " b0=0x" << std::hex << std::uppercase << std::setw(2)
                << std::setfill('0') << static_cast<int>(b0)
                << " b1=0x" << std::setw(2) << static_cast<int>(b1)
                << std::dec << "\n";
    }

    ParseItemResult item = ParseItem(cur);
    if (core::IsDebugEnabled("parse")) {
      std::cerr << "[cursive] parse-items: next_index=" << item.parser.index
                << " advanced="
                << (item.parser.index > cur.index ? "yes" : "no")
                << " diags=" << item.parser.diags.size() << "\n";
    }

    if (item.parser.tokens == cur.tokens && item.parser.index == cur.index) {
      EmitParseSyntaxErr(cur, TokSpan(cur));
      Parser next = AdvanceOrEOF(cur);
      result.items.push_back(ErrorItem{SpanBetween(cur, next), {}});
      cur = next;
      continue;
    }

    result.items.push_back(std::move(item.item));
    cur = item.parser;
  }
}

// =============================================================================
// ParseItems - Parse sequence of items
// =============================================================================
//
// SPEC: Section 3.3.6 lines 3454-3464
// Public entry point for item sequence parsing.
// Extracts module docs before parsing.

ParseItemsResult ParseItems(Parser parser) {
  return ParseItemsInternal(parser, ModuleDocs(parser.docs));
}

// =============================================================================
// ParseFile - Parse complete source file
// =============================================================================
//
// SPEC: Section 3.3.6 lines 3442-3450 (ParseFile-Ok)
// Complete file parsing entry point.
// Orchestrates: Tokenize -> FilterNewlines -> ParseItems -> AttachLineDocs
// Returns ASTFile with items, module docs, unsafe spans.

ParseFileResult ParseFile(const core::SourceFile& source) {
  ParseFileResult result;
  const bool debug_phases = core::IsDebugEnabled("phases");
  if (debug_phases) {
    std::cerr << "[cursive] parsefile: tokenize " << source.path << "\n";
  }
  TokenizeResult tok = Tokenize(source);
  result.diags = tok.diags;

  if (!tok.output.has_value()) {
    return result;
  }

  if (debug_phases) {
    std::cerr << "[cursive] parsefile: filter-newlines " << source.path << "\n";
  }
  std::vector<Token> filtered = FilterNewlines(tok.output->tokens);
  if (debug_phases) {
    std::cerr << "[cursive] parsefile: parse-items " << source.path << "\n";
  }
  std::vector<core::Span> unsafe_spans = UnsafeSpans(filtered);
  Parser parser = MakeParser(filtered, tok.output->docs, source);
  ParseItemsResult items = ParseItems(parser);
  if (debug_phases) {
    std::cerr << "[cursive] parsefile: attach-docs " << source.path << "\n";
  }
  AttachLineDocs(items.items, parser.docs);
  SPEC_RULE("ParseFile-Ok");

  AppendDiags(result.diags, items.parser.diags);

  ASTFile file;
  file.path.clear();
  file.path.push_back(source.path);
  file.items = std::move(items.items);
  file.module_doc = std::move(items.module_doc);
  file.unsafe_spans = std::move(unsafe_spans);
  result.file = std::move(file);
  return result;
}

// =============================================================================
// ParseFileBestEffort - Check if file was parsed
// =============================================================================
//
// Returns true if file was parsed (even with errors)

bool ParseFileBestEffort(const ParseFileResult& result) {
  return result.file.has_value();
}

// =============================================================================
// ParseFileOk - Check if file parsed without errors
// =============================================================================
//
// Returns true if file parsed without errors

bool ParseFileOk(const ParseFileResult& result) {
  return result.file.has_value() && !core::HasError(result.diags);
}

}  // namespace cursive::ast
