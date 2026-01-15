#include <cassert>
#include <cstddef>
#include <string>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/source_text.h"
#include "cursive0/core/span.h"
#include "cursive0/syntax/lexer.h"
#include "cursive0/syntax/token.h"

namespace {

using cursive0::core::LineStarts;
using cursive0::core::SourceFile;
using cursive0::core::Span;
using cursive0::core::SpanOf;
using cursive0::core::UnicodeScalar;
using cursive0::syntax::NextToken;
using cursive0::syntax::NextTokenResult;
using cursive0::syntax::TokenKind;

static std::vector<UnicodeScalar> AsScalars(const std::string& text) {
  std::vector<UnicodeScalar> out;
  out.reserve(text.size());
  for (unsigned char c : text) {
    out.push_back(static_cast<UnicodeScalar>(c));
  }
  return out;
}

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
  SPEC_COV("Max-Munch");
  SPEC_COV("Max-Munch-Err");

  {
    const auto source = MakeSourceFile("op_shift_assign.cursive",
                                       AsScalars(">>="));
    NextTokenResult res = NextToken(source, 0);
    assert(res.ok);
    assert(res.kind == TokenKind::Operator);
    assert(res.next == 3);
    assert(res.diags.empty());
  }

  {
    const auto source = MakeSourceFile("punct_colon.cursive",
                                       AsScalars(":"));
    NextTokenResult res = NextToken(source, 0);
    assert(res.ok);
    assert(res.kind == TokenKind::Punctuator);
    assert(res.next == 1);
  }

  {
    const auto source = MakeSourceFile("op_scope.cursive",
                                       AsScalars("::"));
    NextTokenResult res = NextToken(source, 0);
    assert(res.ok);
    assert(res.kind == TokenKind::Operator);
    assert(res.next == 2);
  }

  {
    const auto source = MakeSourceFile("op_range_inclusive.cursive",
                                       AsScalars("..="));
    NextTokenResult res = NextToken(source, 0);
    assert(res.ok);
    assert(res.kind == TokenKind::Operator);
    assert(res.next == 3);
  }

  {
    const auto source = MakeSourceFile("float_vs_int.cursive",
                                       AsScalars("1.0"));
    NextTokenResult res = NextToken(source, 0);
    assert(res.ok);
    assert(res.kind == TokenKind::FloatLiteral);
    assert(res.next == 3);
  }

  {
    const auto source = MakeSourceFile("numeric_malformed.cursive",
                                       AsScalars("1_"));
    NextTokenResult res = NextToken(source, 0);
    assert(res.ok);
    assert(res.kind == TokenKind::IntLiteral);
    assert(res.next == 2);
    assert(res.diags.size() == 1);
    assert(res.diags[0].code == "E-SRC-0304");
    const Span expect = SpanOf(source, 0, 2);
    assert(res.diags[0].span.has_value());
    ExpectSpanEq(*res.diags[0].span, expect);
  }

  {
    const auto source = MakeSourceFile("op_shift.cursive",
                                       AsScalars(">>"));
    NextTokenResult res = NextToken(source, 0);
    assert(res.ok);
    assert(res.kind == TokenKind::Operator);
    assert(res.next == 2);
  }

  {
    const auto source = MakeSourceFile("max_munch_err.cursive",
                                       AsScalars("`"));
    NextTokenResult res = NextToken(source, 0);
    assert(!res.ok);
    assert(res.diags.size() == 1);
    assert(res.diags[0].code == "E-SRC-0309");
    const Span expect = SpanOf(source, 0, 1);
    assert(res.diags[0].span.has_value());
    ExpectSpanEq(*res.diags[0].span, expect);
  }

  return 0;
}
