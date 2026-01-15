#include <cassert>
#include <cstddef>
#include <string>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/source_text.h"
#include "cursive0/core/span.h"
#include "cursive0/syntax/token.h"

namespace {

using cursive0::core::LineStarts;
using cursive0::core::SourceFile;
using cursive0::core::Span;
using cursive0::core::SpanOf;
using cursive0::core::UnicodeScalar;
using cursive0::syntax::AttachSpan;
using cursive0::syntax::AttachSpans;
using cursive0::syntax::EofToken;
using cursive0::syntax::MakeEofToken;
using cursive0::syntax::NoUnknownOk;
using cursive0::syntax::RawToken;
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

static RawToken MakeRaw(TokenKind kind,
                        const std::string& lexeme,
                        std::size_t start,
                        std::size_t end) {
  RawToken raw;
  raw.kind = kind;
  raw.lexeme = lexeme;
  raw.start_offset = start;
  raw.end_offset = end;
  return raw;
}

}  // namespace

int main() {
  SPEC_COV("No-Unknown-Ok");
  SPEC_COV("Attach-Token-Ok");
  SPEC_COV("Attach-Tokens-Ok");

  const std::vector<UnicodeScalar> scalars = {'a', 'b', 'c', 0x0A, 'd'};
  const auto source = MakeSourceFile("tokens.cursive", scalars);

  {
    const std::vector<Token> tokens = {
        Token{TokenKind::Identifier, "a", SpanOf(source, 0, 1)},
        Token{TokenKind::Operator, "+", SpanOf(source, 1, 2)},
    };
    assert(NoUnknownOk(tokens));

    std::vector<Token> with_unknown = tokens;
    with_unknown.push_back(Token{TokenKind::Unknown, "?", SpanOf(source, 2, 3)});
    assert(!NoUnknownOk(with_unknown));
  }

  {
    const RawToken raw = MakeRaw(TokenKind::Identifier, "ab", 0, 2);
    const Token tok = AttachSpan(source, raw);
    const Span expected = SpanOf(source, 0, 2);
    ExpectSpanEq(tok.span, expected);
    assert(tok.kind == TokenKind::Identifier);
    assert(tok.lexeme == "ab");
  }

  {
    const RawToken raw = MakeRaw(TokenKind::Identifier, "clamp", 99, 1);
    const Token tok = AttachSpan(source, raw);
    const Span expected = SpanOf(source, 99, 1);
    ExpectSpanEq(tok.span, expected);
  }

  {
    const std::vector<RawToken> raws = {
        MakeRaw(TokenKind::Identifier, "a", 0, 1),
        MakeRaw(TokenKind::Operator, "+", 1, 2),
        MakeRaw(TokenKind::Identifier, "b", 2, 3),
    };
    const auto toks = AttachSpans(source, raws);
    assert(toks.size() == raws.size());

    for (std::size_t i = 0; i < raws.size(); ++i) {
      const Span expected = SpanOf(source, raws[i].start_offset, raws[i].end_offset);
      ExpectSpanEq(toks[i].span, expected);
      assert(toks[i].kind == raws[i].kind);
      assert(toks[i].lexeme == raws[i].lexeme);
    }

    const std::vector<RawToken> empty;
    const auto empty_out = AttachSpans(source, empty);
    assert(empty_out.empty());
  }

  {
    const EofToken eof = MakeEofToken(source);
    const Span expected = SpanOf(source, source.byte_len, source.byte_len);
    ExpectSpanEq(eof.span, expected);
  }

  return 0;
}
