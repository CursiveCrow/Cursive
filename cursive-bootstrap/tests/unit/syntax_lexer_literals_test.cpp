#include <cassert>
#include <cstddef>
#include <optional>
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
using cursive0::syntax::LexNewlines;
using cursive0::syntax::LiteralScanResult;
using cursive0::syntax::ScalarRange;
using cursive0::syntax::ScanCharLiteral;
using cursive0::syntax::ScanFloatLiteral;
using cursive0::syntax::ScanIntLiteral;
using cursive0::syntax::ScanStringLiteral;
using cursive0::syntax::Token;
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
  SPEC_COV("Lex-Int");
  SPEC_COV("Lex-Float");
  SPEC_COV("Lex-Numeric-Err");
  SPEC_COV("Lex-String");
  SPEC_COV("Lex-String-Unterminated");
  SPEC_COV("Lex-String-BadEscape");
  SPEC_COV("Lex-Char");
  SPEC_COV("Lex-Char-Unterminated");
  SPEC_COV("Lex-Char-BadEscape");
  SPEC_COV("Lex-Char-Invalid");

  {
    const auto source = MakeSourceFile("int_dec.cursive", AsScalars("123"));
    LiteralScanResult res = ScanIntLiteral(source, 0);
    assert(res.ok);
    assert(res.next == 3);
    assert(res.diags.empty());
  }

  {
    const auto source = MakeSourceFile("int_hex.cursive", AsScalars("0x1f"));
    LiteralScanResult res = ScanIntLiteral(source, 0);
    assert(res.ok);
    assert(res.next == 4);
    assert(res.diags.empty());
  }

  {
    const auto source = MakeSourceFile("int_suffix.cursive", AsScalars("42u8"));
    LiteralScanResult res = ScanIntLiteral(source, 0);
    assert(res.ok);
    assert(res.next == 4);
    assert(res.diags.empty());
  }

  {
    const auto source = MakeSourceFile("int_leading_zero.cursive", AsScalars("0123"));
    LiteralScanResult res = ScanIntLiteral(source, 0);
    assert(res.ok);
    assert(res.diags.size() == 1);
    assert(res.diags[0].code == "W-SRC-0301");
    const Span expect = SpanOf(source, 0, 4);
    assert(res.diags[0].span.has_value());
    ExpectSpanEq(*res.diags[0].span, expect);
  }

  {
    const auto source = MakeSourceFile("int_bad_underscore.cursive", AsScalars("1_"));
    LiteralScanResult res = ScanIntLiteral(source, 0);
    assert(!res.ok);
    assert(res.next == 2);
    assert(res.diags.size() == 1);
    assert(res.diags[0].code == "E-SRC-0304");
    const Span expect = SpanOf(source, 0, 2);
    assert(res.diags[0].span.has_value());
    ExpectSpanEq(*res.diags[0].span, expect);
  }

  {
    const auto source = MakeSourceFile("float_full.cursive", AsScalars("1.25e-3f32"));
    LiteralScanResult res = ScanFloatLiteral(source, 0);
    assert(res.ok);
    assert(res.next == 10);
    assert(res.diags.empty());
  }

  {
    const auto source = MakeSourceFile("string_ok.cursive", AsScalars("\"hi\""));
    LiteralScanResult res = ScanStringLiteral(source, 0);
    assert(res.ok);
    assert(res.next == 4);
    assert(res.range.has_value());
    assert(res.range->start == 0);
    assert(res.range->end == 4);
    assert(res.diags.empty());
  }

  {
    const auto source = MakeSourceFile("string_bad_escape.cursive", AsScalars("\"\\q\""));
    LiteralScanResult res = ScanStringLiteral(source, 0);
    assert(res.ok);
    assert(res.diags.size() == 1);
    assert(res.diags[0].code == "E-SRC-0302");
    const Span expect = SpanOf(source, 1, 2);
    assert(res.diags[0].span.has_value());
    ExpectSpanEq(*res.diags[0].span, expect);
  }

  {
    const auto source = MakeSourceFile("string_unterminated.cursive", AsScalars("\"hi\n"));
    LiteralScanResult res = ScanStringLiteral(source, 0);
    assert(!res.ok);
    assert(res.range == std::nullopt);
    assert(res.next == 3);
    assert(res.diags.size() == 1);
    assert(res.diags[0].code == "E-SRC-0301");
    const Span expect = SpanOf(source, 0, 1);
    assert(res.diags[0].span.has_value());
    ExpectSpanEq(*res.diags[0].span, expect);

    const std::vector<ScalarRange> suppressed;
    const auto tokens = LexNewlines(source, suppressed);
    assert(tokens.size() == 1);
    assert(tokens[0].kind == TokenKind::Newline);
    ExpectSpanEq(tokens[0].span, SpanOf(source, 3, 4));
  }

  {
    const auto source = MakeSourceFile("char_ok.cursive", AsScalars("'a'"));
    LiteralScanResult res = ScanCharLiteral(source, 0);
    assert(res.ok);
    assert(res.next == 3);
    assert(res.range.has_value());
    assert(res.range->start == 0);
    assert(res.range->end == 3);
    assert(res.diags.empty());
  }

  {
    const auto source = MakeSourceFile("char_bad_escape.cursive", AsScalars("'\\q'"));
    LiteralScanResult res = ScanCharLiteral(source, 0);
    assert(res.ok);
    assert(res.diags.size() == 2);
    assert(res.diags[0].code == "E-SRC-0302");
    const Span expect_escape = SpanOf(source, 1, 2);
    assert(res.diags[0].span.has_value());
    ExpectSpanEq(*res.diags[0].span, expect_escape);
    assert(res.diags[1].code == "E-SRC-0303");
    const Span expect_invalid = SpanOf(source, 0, 1);
    assert(res.diags[1].span.has_value());
    ExpectSpanEq(*res.diags[1].span, expect_invalid);
  }

  {
    const auto source = MakeSourceFile("char_empty.cursive", AsScalars("''"));
    LiteralScanResult res = ScanCharLiteral(source, 0);
    assert(res.ok);
    assert(res.diags.size() == 1);
    assert(res.diags[0].code == "E-SRC-0303");
    const Span expect = SpanOf(source, 0, 1);
    assert(res.diags[0].span.has_value());
    ExpectSpanEq(*res.diags[0].span, expect);
  }

  {
    const auto source = MakeSourceFile("char_unterminated.cursive", AsScalars("'a\n"));
    LiteralScanResult res = ScanCharLiteral(source, 0);
    assert(!res.ok);
    assert(res.next == 2);
    assert(res.diags.size() == 1);
    assert(res.diags[0].code == "E-SRC-0303");
    const Span expect = SpanOf(source, 0, 1);
    assert(res.diags[0].span.has_value());
    ExpectSpanEq(*res.diags[0].span, expect);
  }

  return 0;
}
