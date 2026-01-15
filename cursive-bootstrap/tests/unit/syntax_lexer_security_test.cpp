#include <cassert>
#include <cstddef>
#include <string>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/source_text.h"
#include "cursive0/core/span.h"
#include "cursive0/syntax/lexer.h"

namespace {

using cursive0::core::LineStarts;
using cursive0::core::SourceFile;
using cursive0::core::Span;
using cursive0::core::SpanOf;
using cursive0::core::UnicodeScalar;
using cursive0::syntax::LexSecure;
using cursive0::syntax::LexSecureResult;
using cursive0::syntax::LexSmallStep;
using cursive0::syntax::LexSmallStepResult;

SourceFile MakeSourceFile(const std::string& path,
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

LexSecureResult RunLexSecure(const SourceFile& source) {
  LexSmallStepResult lexed = LexSmallStep(source);
  assert(lexed.ok);
  return LexSecure(source, lexed.output.tokens, lexed.sensitive);
}

void ExpectSpanEq(const Span& a, const Span& b) {
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
  SPEC_COV("LexSecure-Err");
  SPEC_COV("LexSecure-Warn");

  {
    const std::vector<UnicodeScalar> scalars = {'a', 0x202Eu, 'b'};
    const SourceFile source = MakeSourceFile("err_outside.cursive", scalars);
    LexSecureResult result = RunLexSecure(source);
    assert(!result.ok);
    assert(result.diags.size() == 1);
    assert(result.diags[0].code == "E-SRC-0308");
    const auto offsets = cursive0::core::Utf8Offsets(scalars);
    const Span expect = SpanOf(source, offsets[1], offsets[2]);
    assert(result.diags[0].span.has_value());
    ExpectSpanEq(*result.diags[0].span, expect);
  }

  {
    const std::vector<UnicodeScalar> scalars = {
        'u', 'n', 's', 'a', 'f', 'e', ' ', '{', ' ', 0x200Du, ' ', '}'};
    const SourceFile source = MakeSourceFile("warn_unsafe.cursive", scalars);
    LexSecureResult result = RunLexSecure(source);
    assert(result.ok);
    assert(result.diags.size() == 1);
    assert(result.diags[0].code == "W-SRC-0308");
    const auto offsets = cursive0::core::Utf8Offsets(scalars);
    const Span expect = SpanOf(source, offsets[9], offsets[10]);
    assert(result.diags[0].span.has_value());
    ExpectSpanEq(*result.diags[0].span, expect);
  }

  {
    const std::vector<UnicodeScalar> scalars = {'/', '/', 0x202Au, '\n'};
    const SourceFile source = MakeSourceFile("comment_ok.cursive", scalars);
    LexSecureResult result = RunLexSecure(source);
    assert(result.ok);
    assert(result.diags.empty());
  }

  {
    const std::vector<UnicodeScalar> scalars = {'"', 0x202Au, '"'};
    const SourceFile source = MakeSourceFile("string_ok.cursive", scalars);
    LexSecureResult result = RunLexSecure(source);
    assert(result.ok);
    assert(result.diags.empty());
  }

  {
    const std::vector<UnicodeScalar> scalars = {
        'u', 'n', 's', 'a', 'f', 'e', ' ', '{', ' ', 0x202Eu};
    const SourceFile source = MakeSourceFile("missing_brace.cursive", scalars);
    LexSecureResult result = RunLexSecure(source);
    assert(!result.ok);
    assert(result.diags.size() == 1);
    assert(result.diags[0].code == "E-SRC-0308");
  }

    {
    const std::vector<UnicodeScalar> scalars = {'u', 'n', 's', 'a', 'f', 'e',
                                                '\n', '{', '\n', 0x200Cu,
                                                '\n', '}'};
    const SourceFile source =
        MakeSourceFile("newline_unsafe.cursive", scalars);
    LexSecureResult result = RunLexSecure(source);
    assert(result.ok);
    assert(result.diags.size() == 1);
    assert(result.diags[0].code == "W-SRC-0308");
  }

  return 0;
}
