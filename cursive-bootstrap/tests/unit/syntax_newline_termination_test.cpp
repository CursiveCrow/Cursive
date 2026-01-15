#include <cassert>
#include <cstddef>
#include <string>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/source_text.h"
#include "cursive0/core/span.h"
#include "cursive0/syntax/lexer.h"
#include "cursive0/syntax/parser.h"
#include "cursive0/syntax/token.h"

namespace {

using cursive0::core::LineStarts;
using cursive0::core::SourceFile;
using cursive0::core::Span;
using cursive0::core::SpanOf;
using cursive0::core::UnicodeScalar;
using cursive0::syntax::FilterNewlines;
using cursive0::syntax::LexNewlines;
using cursive0::syntax::ConsumeTerminatorOpt;
using cursive0::syntax::MakeParser;
using cursive0::syntax::Parser;
using cursive0::syntax::ScalarRange;
using cursive0::syntax::TerminatorPolicy;
using cursive0::syntax::Token;
using cursive0::syntax::TokenKind;

static SourceFile MakeSourceFile(const std::string& path,
                                 const std::vector<UnicodeScalar>& scalars) {
  SourceFile s;
  s.path = path;
  s.scalars = scalars;
  s.text = cursive0::core::EncodeUtf8(scalars);
  s.bytes.assign(s.text.begin(), s.text.end());
  s.byte_len = s.text.size();
  s.line_starts = LineStarts(scalars);
  s.line_count = s.line_starts.size();
  return s;
}

static void ExpectSpanEq(const Span& a, const Span& b) {
  assert(a.file == b.file);
  assert(a.start_offset == b.start_offset);
  assert(a.end_offset == b.end_offset);
  assert(a.start_line == b.start_line);
  assert(a.start_col == b.start_col);
  assert(a.end_line == b.end_line);
  assert(a.end_col == b.end_col);
}

static Token MakeTok(TokenKind kind,
                     const std::string& lexeme,
                     const SourceFile& source,
                     std::size_t start,
                     std::size_t end) {
  Token tok;
  tok.kind = kind;
  tok.lexeme = lexeme;
  tok.span = SpanOf(source, start, end);
  return tok;
}

}  // namespace

int main() {
  SPEC_COV("Lex-Newline");
  SPEC_COV("Missing-Terminator-Err");

  {
    const std::vector<UnicodeScalar> scalars = {'a', 0x0A, 'b', 0x0A, 'c'};
    const auto source = MakeSourceFile("lex_newlines.cursive", scalars);
    const auto tokens = LexNewlines(source, {});
    assert(tokens.size() == 2);
    assert(tokens[0].kind == TokenKind::Newline);
    assert(tokens[0].lexeme == "\n");
    assert(tokens[1].kind == TokenKind::Newline);
    assert(tokens[1].lexeme == "\n");
    ExpectSpanEq(tokens[0].span, SpanOf(source, 1, 2));
    ExpectSpanEq(tokens[1].span, SpanOf(source, 3, 4));
  }

  {
    const std::vector<UnicodeScalar> scalars = {'a', 0x0A, 'b'};
    const auto source = MakeSourceFile("lex_newlines_suppressed.cursive", scalars);
    const std::vector<ScalarRange> suppressed = {{1, 2}};
    const auto tokens = LexNewlines(source, suppressed);
    assert(tokens.empty());
  }

  {
    const std::vector<UnicodeScalar> scalars = {'(', 0x0A, ')', 0x0A};
    const auto source = MakeSourceFile("filter_newlines.cursive", scalars);
    const std::vector<Token> tokens = {
        MakeTok(TokenKind::Punctuator, "(", source, 0, 1),
        MakeTok(TokenKind::Newline, "\n", source, 1, 2),
        MakeTok(TokenKind::Punctuator, ")", source, 2, 3),
        MakeTok(TokenKind::Newline, "\n", source, 3, 4),
    };
    const auto filtered = FilterNewlines(tokens);
    std::size_t newline_count = 0;
    for (const auto& tok : filtered) {
      if (tok.kind == TokenKind::Newline) {
        newline_count += 1;
      }
    }
    assert(newline_count == 1);
  }

  {
    const std::vector<UnicodeScalar> scalars = {'a', ',', 0x0A, 'b'};
    const auto source = MakeSourceFile("filter_trailing.cursive", scalars);
    const std::vector<Token> tokens = {
        MakeTok(TokenKind::Identifier, "a", source, 0, 1),
        MakeTok(TokenKind::Punctuator, ",", source, 1, 2),
        MakeTok(TokenKind::Newline, "\n", source, 2, 3),
        MakeTok(TokenKind::Identifier, "b", source, 3, 4),
    };
    const auto filtered = FilterNewlines(tokens);
    bool has_newline = false;
    for (const auto& tok : filtered) {
      if (tok.kind == TokenKind::Newline) {
        has_newline = true;
      }
    }
    assert(!has_newline);
  }

  {
    const std::vector<UnicodeScalar> scalars = {'a', '+', 0x0A, 'b'};
    const auto source = MakeSourceFile("filter_ambig.cursive", scalars);
    const std::vector<Token> tokens = {
        MakeTok(TokenKind::Identifier, "a", source, 0, 1),
        MakeTok(TokenKind::Operator, "+", source, 1, 2),
        MakeTok(TokenKind::Newline, "\n", source, 2, 3),
        MakeTok(TokenKind::Identifier, "b", source, 3, 4),
    };
    const auto filtered = FilterNewlines(tokens);
    bool has_newline = false;
    for (const auto& tok : filtered) {
      if (tok.kind == TokenKind::Newline) {
        has_newline = true;
      }
    }
    assert(!has_newline);
  }

  {
    const std::vector<UnicodeScalar> scalars = {'a', '+', 0x0A, '}'};
    const auto source = MakeSourceFile("filter_no_operand.cursive", scalars);
    const std::vector<Token> tokens = {
        MakeTok(TokenKind::Identifier, "a", source, 0, 1),
        MakeTok(TokenKind::Operator, "+", source, 1, 2),
        MakeTok(TokenKind::Newline, "\n", source, 2, 3),
        MakeTok(TokenKind::Punctuator, "}", source, 3, 4),
    };
    const auto filtered = FilterNewlines(tokens);
    bool has_newline = false;
    for (const auto& tok : filtered) {
      if (tok.kind == TokenKind::Newline) {
        has_newline = true;
      }
    }
    assert(has_newline);
  }

  {
    const std::vector<UnicodeScalar> scalars = {'!', 0x0A, 'a'};
    const auto source = MakeSourceFile("filter_unary_only.cursive", scalars);
    const std::vector<Token> tokens = {
        MakeTok(TokenKind::Operator, "!", source, 0, 1),
        MakeTok(TokenKind::Newline, "\n", source, 1, 2),
        MakeTok(TokenKind::Identifier, "a", source, 2, 3),
    };
    const auto filtered = FilterNewlines(tokens);
    bool has_newline = false;
    for (const auto& tok : filtered) {
      if (tok.kind == TokenKind::Newline) {
        has_newline = true;
      }
    }
    assert(has_newline);
  }

  {
    const std::vector<UnicodeScalar> scalars = {'a', 0x0A, '.', 'b'};
    const auto source = MakeSourceFile("filter_leading.cursive", scalars);
    const std::vector<Token> tokens = {
        MakeTok(TokenKind::Identifier, "a", source, 0, 1),
        MakeTok(TokenKind::Newline, "\n", source, 1, 2),
        MakeTok(TokenKind::Punctuator, ".", source, 2, 3),
        MakeTok(TokenKind::Identifier, "b", source, 3, 4),
    };
    const auto filtered = FilterNewlines(tokens);
    bool has_newline = false;
    for (const auto& tok : filtered) {
      if (tok.kind == TokenKind::Newline) {
        has_newline = true;
      }
    }
    assert(!has_newline);
  }

  {
    const std::vector<UnicodeScalar> scalars = {'a', ';', 'b'};
    const auto source = MakeSourceFile("parser_term.cursive", scalars);
    const std::vector<Token> tokens = {
        MakeTok(TokenKind::Identifier, "a", source, 0, 1),
        MakeTok(TokenKind::Identifier, "b", source, 1, 2),
        MakeTok(TokenKind::Punctuator, ";", source, 2, 3),
    };
    Parser parser = MakeParser(tokens, source);
    ConsumeTerminatorOpt(parser, TerminatorPolicy::Required);
    assert(parser.diags.size() == 1);
    assert(parser.diags[0].code == "E-SRC-0510");
    assert(parser.diags[0].span.has_value());
    ExpectSpanEq(*parser.diags[0].span, tokens[0].span);
    assert(parser.index == 3);
  }

  {
    const std::vector<UnicodeScalar> scalars = {'\n', 'a'};
    const auto source = MakeSourceFile("parser_term_newline.cursive", scalars);
    const std::vector<Token> tokens = {
        MakeTok(TokenKind::Newline, "\n", source, 0, 1),
        MakeTok(TokenKind::Identifier, "a", source, 1, 2),
    };
    Parser parser = MakeParser(tokens, source);
    ConsumeTerminatorOpt(parser, TerminatorPolicy::Required);
    assert(parser.diags.empty());
    assert(parser.index == 1);
  }

  {
    const std::vector<UnicodeScalar> scalars = {'a', 'b'};
    const auto source = MakeSourceFile("parser_term_optional.cursive", scalars);
    const std::vector<Token> tokens = {
        MakeTok(TokenKind::Identifier, "a", source, 0, 1),
        MakeTok(TokenKind::Identifier, "b", source, 1, 2),
    };
    Parser parser = MakeParser(tokens, source);
    ConsumeTerminatorOpt(parser, TerminatorPolicy::Optional);
    assert(parser.diags.empty());
    assert(parser.index == 0);
  }

  return 0;
}
