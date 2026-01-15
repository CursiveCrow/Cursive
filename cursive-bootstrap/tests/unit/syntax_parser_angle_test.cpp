#include <cassert>
#include <string>
#include <vector>

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
using cursive0::syntax::AngleDelta;
using cursive0::syntax::AngleScan;
using cursive0::syntax::MakeParser;
using cursive0::syntax::Parser;
using cursive0::syntax::SkipAngles;
using cursive0::syntax::SplitShiftR;
using cursive0::syntax::SplitSpan2;
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

}  // namespace

int main() {
  const std::vector<UnicodeScalar> ptr_scalars =
      {'P', 't', 'r', '<', 'T', '>', '>', 'x'};
  const SourceFile ptr_source = MakeSourceFile("angle_ptr.cursive", ptr_scalars);

  {
    const Span full = SpanOf(ptr_source, 5, 7);
    const auto split = SplitSpan2(full);
    const Span expected_left = SpanOf(ptr_source, 5, 6);
    const Span expected_right = SpanOf(ptr_source, 6, 7);
    ExpectSpanEq(split.first, expected_left);
    ExpectSpanEq(split.second, expected_right);
  }

  {
    std::vector<Token> tokens = {
        Token{TokenKind::Identifier, "Ptr", SpanOf(ptr_source, 0, 3)},
        Token{TokenKind::Operator, "<", SpanOf(ptr_source, 3, 4)},
        Token{TokenKind::Identifier, "T", SpanOf(ptr_source, 4, 5)},
        Token{TokenKind::Operator, ">>", SpanOf(ptr_source, 5, 7)},
        Token{TokenKind::Identifier, "x", SpanOf(ptr_source, 7, 8)},
    };

    Parser parser = MakeParser(tokens, ptr_source);
    parser.index = 3;
    Parser split = SplitShiftR(parser);

    assert(split.tokens);
    assert(split.tokens->size() == 6);
    assert(split.index == 3);

    const Token& first = (*split.tokens)[3];
    const Token& second = (*split.tokens)[4];
    assert(first.kind == TokenKind::Operator && first.lexeme == ">");
    assert(second.kind == TokenKind::Operator && second.lexeme == ">");

    ExpectSpanEq(first.span, SpanOf(ptr_source, 5, 6));
    ExpectSpanEq(second.span, SpanOf(ptr_source, 6, 7));

    assert((*split.tokens)[0].lexeme == "Ptr");
    assert((*split.tokens)[5].lexeme == "x");
  }

  {
    const Token lt = Token{TokenKind::Operator, "<", SpanOf(ptr_source, 3, 4)};
    const Token gt = Token{TokenKind::Operator, ">", SpanOf(ptr_source, 5, 6)};
    const Token shr = Token{TokenKind::Operator, ">>", SpanOf(ptr_source, 5, 7)};
    const Token id = Token{TokenKind::Identifier, "T", SpanOf(ptr_source, 4, 5)};
    assert(AngleDelta(lt) == 1);
    assert(AngleDelta(gt) == -1);
    assert(AngleDelta(shr) == -2);
    assert(AngleDelta(id) == 0);
  }

  {
    const std::vector<UnicodeScalar> nested_scalars =
        {'<', 'A', '<', 'B', '>', '>', 'C'};
    const SourceFile nested_source =
        MakeSourceFile("angle_nested.cursive", nested_scalars);
    std::vector<Token> tokens = {
        Token{TokenKind::Operator, "<", SpanOf(nested_source, 0, 1)},
        Token{TokenKind::Identifier, "A", SpanOf(nested_source, 1, 2)},
        Token{TokenKind::Operator, "<", SpanOf(nested_source, 2, 3)},
        Token{TokenKind::Identifier, "B", SpanOf(nested_source, 3, 4)},
        Token{TokenKind::Operator, ">>", SpanOf(nested_source, 4, 6)},
        Token{TokenKind::Identifier, "C", SpanOf(nested_source, 6, 7)},
    };

    Parser parser = MakeParser(tokens, nested_source);
    Parser out = SkipAngles(parser);
    assert(out.index == 5);
  }

  {
    const std::vector<UnicodeScalar> eof_scalars = {'<', 'A'};
    const SourceFile eof_source = MakeSourceFile("angle_eof.cursive", eof_scalars);
    std::vector<Token> tokens = {
        Token{TokenKind::Operator, "<", SpanOf(eof_source, 0, 1)},
        Token{TokenKind::Identifier, "A", SpanOf(eof_source, 1, 2)},
    };

    Parser parser = MakeParser(tokens, eof_source);
    Parser out = AngleScan(parser, parser, 0);
    assert(out.index == 0);
  }

  return 0;
}
