#include <array>
#include <cassert>
#include <string>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/source_text.h"
#include "cursive0/core/span.h"
#include "cursive0/syntax/parser.h"
#include "cursive0/syntax/token.h"

namespace {

using cursive0::core::LineStarts;
using cursive0::core::SourceFile;
using cursive0::core::Span;
using cursive0::core::SpanOf;
using cursive0::core::UnicodeScalar;
using cursive0::syntax::ConsumeKind;
using cursive0::syntax::ConsumeKeyword;
using cursive0::syntax::ConsumeOperator;
using cursive0::syntax::ConsumePunct;
using cursive0::syntax::EmitTrailingCommaErr;
using cursive0::syntax::ListCons;
using cursive0::syntax::ListDone;
using cursive0::syntax::ListStart;
using cursive0::syntax::MatchPunct;
using cursive0::syntax::MatchKind;
using cursive0::syntax::ParseElemResult;
using cursive0::syntax::Parser;
using cursive0::syntax::Token;
using cursive0::syntax::TokenKind;
using cursive0::syntax::TrailingComma;

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

}  // namespace

int main() {
  SPEC_COV("Tok-Consume-Kind");
  SPEC_COV("Tok-Consume-Keyword");
  SPEC_COV("Tok-Consume-Operator");
  SPEC_COV("Tok-Consume-Punct");
  SPEC_COV("List-Start");
  SPEC_COV("List-Cons");
  SPEC_COV("List-Done");
  SPEC_COV("Trailing-Comma-Err");

  const std::vector<UnicodeScalar> scalars = {
      'a', 'b', 'c', 'd', 'e', 'f', ',', ')'};
  const SourceFile source = MakeSourceFile("consume.cursive", scalars);

  {
    std::vector<Token> tokens = {
        Token{TokenKind::Identifier, "a", SpanOf(source, 0, 1)},
        Token{TokenKind::Operator, "+", SpanOf(source, 1, 2)},
        Token{TokenKind::Punctuator, ";", SpanOf(source, 2, 3)},
    };
    Parser parser = cursive0::syntax::MakeParser(tokens, source);

    assert(!ConsumeKind(parser, TokenKind::Operator));
    assert(parser.index == 0);
    assert(parser.diags.empty());

    assert(ConsumeKind(parser, TokenKind::Identifier));
    assert(parser.index == 1);
    assert(parser.diags.empty());

    assert(!ConsumeKeyword(parser, "using"));
    assert(parser.index == 1);
  }

  {
    std::vector<Token> tokens = {
        Token{TokenKind::Keyword, "using", SpanOf(source, 0, 1)},
        Token{TokenKind::Operator, "->", SpanOf(source, 1, 2)},
        Token{TokenKind::Punctuator, "(", SpanOf(source, 2, 3)},
    };
    Parser parser = cursive0::syntax::MakeParser(tokens, source);

    assert(ConsumeKeyword(parser, "using"));
    assert(parser.index == 1);

    assert(ConsumeOperator(parser, "->"));
    assert(parser.index == 2);

    assert(ConsumePunct(parser, "("));
    assert(parser.index == 3);
  }

  {
    std::vector<Token> tokens = {
        Token{TokenKind::Identifier, "a", SpanOf(source, 0, 1)},
        Token{TokenKind::Identifier, "b", SpanOf(source, 1, 2)},
        Token{TokenKind::Punctuator, ")", SpanOf(source, 2, 3)},
    };
    Parser parser = cursive0::syntax::MakeParser(tokens, source);

    auto parse_elem = [](Parser p) -> ParseElemResult<int> {
      const bool ok = ConsumeKind(p, TokenKind::Identifier);
      assert(ok);
      return {p, 1};
    };

    auto state = ListStart<int>(parser);
    state = ListCons<int>(state, parse_elem);
    assert(state.parser.index == 1);
    assert(state.elems.size() == 1);

    state = ListCons<int>(state, parse_elem);
    assert(state.parser.index == 2);
    assert(state.elems.size() == 2);

    const std::array end_paren = {MatchPunct(")")};
    const std::array end_bracket = {MatchPunct("]")};
    assert(ListDone<int>(state, end_paren));
    assert(!ListDone<int>(state, end_bracket));
  }

  {
    std::vector<Token> tokens = {
        Token{TokenKind::Punctuator, ",", SpanOf(source, 6, 7)},
        Token{TokenKind::Punctuator, ")", SpanOf(source, 7, 8)},
    };
    Parser parser = cursive0::syntax::MakeParser(tokens, source);

    const std::array end_paren = {MatchPunct(")")};
    assert(TrailingComma(parser, end_paren));
    assert(parser.diags.empty());

    const Span comma_span = SpanOf(source, 6, 7);
    assert(EmitTrailingCommaErr(parser, end_paren));
    assert(parser.index == 0);
    assert(parser.diags.size() == 1);
    assert(parser.diags[0].code == "E-UNS-0101");
    assert(parser.diags[0].span.has_value());
    ExpectSpanEq(*parser.diags[0].span, comma_span);
  }

  {
    std::vector<Token> tokens = {
        Token{TokenKind::Punctuator, ",", SpanOf(source, 6, 7)},
        Token{TokenKind::Identifier, "x", SpanOf(source, 7, 8)},
    };
    Parser parser = cursive0::syntax::MakeParser(tokens, source);

    const std::array end_paren = {MatchPunct(")")};
    assert(!TrailingComma(parser, end_paren));
    assert(!EmitTrailingCommaErr(parser, end_paren));
    assert(parser.diags.empty());
  }

  {
    std::vector<Token> tokens = {
        Token{TokenKind::Newline, "\n", SpanOf(source, 0, 1)},
    };
    Parser parser = cursive0::syntax::MakeParser(tokens, source);
    const std::array end_newline = {MatchKind(TokenKind::Newline)};
    assert(ListDone<int>(ListStart<int>(parser), end_newline));
  }

  return 0;
}
