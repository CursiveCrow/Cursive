#include "cursive0/syntax/parser.h"

#include <cstdlib>
#include <iostream>
#include <string_view>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/diagnostic_messages.h"
#include "cursive0/core/diagnostics.h"
#include "cursive0/core/span.h"
#include "cursive0/syntax/lexer.h"

namespace cursive0::syntax {

std::vector<DocComment> ModuleDocs(const std::vector<DocComment>* docs);
void AttachLineDocs(std::vector<ASTItem>& items,
                    const std::vector<DocComment>* docs);

namespace {

void AppendDiags(core::DiagnosticStream& out,
                 const core::DiagnosticStream& add) {
  for (const auto& diag : add) {
    out = core::Emit(out, diag);
  }
}

core::Span SpanFrom(const Token& start, const Token& end) {
  core::Span span = start.span;
  span.end_offset = end.span.end_offset;
  span.end_line = end.span.end_line;
  span.end_col = end.span.end_col;
  return span;
}

Token EofAsToken(const Parser& parser) {
  Token tok;
  tok.kind = TokenKind::Unknown;
  tok.lexeme.clear();
  tok.span = parser.eof.span;
  return tok;
}
}  // namespace

Parser AdvanceOrEOF(const Parser& parser) {
  if (AtEof(parser)) {
    return parser;
  }
  Parser next = parser;
  next.index += 1;
  return next;
}

Parser Clone(const Parser& parser) {
  Parser out = parser;
  out.diags.clear();
  return out;
}

Parser MergeDiag(const Parser& base, const Parser& diag, const Parser& src) {
  Parser out = src;
  out.diags = base.diags;
  AppendDiags(out.diags, diag.diags);
  return out;
}

bool PStateOk(const Parser& parser) {
  if (!parser.tokens) {
    return parser.index == 0;
  }
  return parser.index <= parser.tokens->size();
}

core::Span SpanBetween(const Parser& start, const Parser& end) {
  Token start_tok = Tok(start) ? *Tok(start) : EofAsToken(start);
  const std::vector<Token>* tokens =
      end.tokens ? end.tokens : start.tokens;
  Token end_tok = start_tok;
  if (tokens && end.index > start.index && end.index - 1 < tokens->size()) {
    end_tok = (*tokens)[end.index - 1];
  }
  return SpanFrom(start_tok, end_tok);
}

static ParseItemsResult ParseItemsInternal(
    Parser parser,
    const std::vector<DocComment>& module_docs) {
  Parser cur = parser;
  while (!AtEof(cur)) {
    const Token* tok = Tok(cur);
    if (!tok || tok->kind != TokenKind::Newline) {
      break;
    }
    Advance(cur);
  }

  if (AtEof(cur)) {
    SPEC_RULE("ParseItems-Empty");
    ParseItemsResult result;
    result.parser = cur;
    result.module_doc = module_docs;
    return result;
  }

  SPEC_RULE("ParseItems-Cons");
  ParseItemResult item = ParseItem(cur);
  if (item.parser.tokens == cur.tokens && item.parser.index == cur.index) {
    EmitParseSyntaxErr(cur, TokSpan(cur));
    Parser next = AdvanceOrEOF(cur);
    ParseItemsResult tail = ParseItemsInternal(next, module_docs);
    std::vector<ASTItem> items;
    items.reserve(1 + tail.items.size());
    items.push_back(ErrorItem{SpanBetween(cur, next)});
    items.insert(items.end(), tail.items.begin(), tail.items.end());
    tail.items = std::move(items);
    return tail;
  }
  ParseItemsResult tail = ParseItemsInternal(item.parser, module_docs);

  std::vector<ASTItem> items;
  items.reserve(1 + tail.items.size());
  items.push_back(std::move(item.item));
  items.insert(items.end(), tail.items.begin(), tail.items.end());
  tail.items = std::move(items);
  return tail;
}

ParseItemsResult ParseItems(Parser parser) {
  return ParseItemsInternal(parser, ModuleDocs(parser.docs));
}

ParseFileResult ParseFile(const core::SourceFile& source) {
  ParseFileResult result;
  const bool debug_phases = std::getenv("CURSIVE0_DEBUG_PHASES") != nullptr;
  if (debug_phases) {
    std::cerr << "[cursivec0] parsefile: tokenize " << source.path << "\n";
  }
  TokenizeResult tok = Tokenize(source);
  result.diags = tok.diags;

  if (!tok.output.has_value()) {
    return result;
  }

  if (debug_phases) {
    std::cerr << "[cursivec0] parsefile: filter-newlines " << source.path << "\n";
  }
  std::vector<Token> filtered = FilterNewlines(tok.output->tokens);
  if (debug_phases) {
    std::cerr << "[cursivec0] parsefile: parse-items " << source.path << "\n";
  }
  std::vector<core::Span> unsafe_spans = UnsafeSpans(filtered);
  Parser parser = MakeParser(filtered, tok.output->docs, source);
  ParseItemsResult items = ParseItems(parser);
  if (debug_phases) {
    std::cerr << "[cursivec0] parsefile: attach-docs " << source.path << "\n";
  }
  AttachLineDocs(items.items, parser.docs);
  SPEC_RULE("ParseFile-Ok");

  AppendDiags(result.diags, items.parser.diags);

  ASTFile file;
  file.path = source.path;
  file.items = std::move(items.items);
  file.module_doc = std::move(items.module_doc);
  file.unsafe_spans = std::move(unsafe_spans);
  result.file = std::move(file);
  return result;
}

bool ParseFileBestEffort(const ParseFileResult& result) {
  return result.file.has_value();
}

bool ParseFileOk(const ParseFileResult& result) {
  return result.file.has_value() && !core::HasError(result.diags);
}

}  // namespace cursive0::syntax
