#include <cassert>
#include <string>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/source_text.h"
#include "cursive0/core/span.h"
#include "cursive0/syntax/ast.h"
#include "cursive0/syntax/parser.h"

namespace {

using cursive0::core::LineStarts;
using cursive0::core::SourceFile;
using cursive0::core::Span;
using cursive0::core::SpanOf;
using cursive0::core::UnicodeScalar;
using cursive0::syntax::ASTFile;
using cursive0::syntax::ErrorItem;
using cursive0::syntax::ParseFile;

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
  SPEC_COV("ParseFile-Ok");
  SPEC_COV("ParseItems-Empty");
  SPEC_COV("ParseItems-Cons");
  SPEC_COV("Parse-Item-Err");

  {
    const SourceFile source = MakeSourceFile("parse_empty.cursive", {});
    auto result = ParseFile(source);
    assert(result.file.has_value());
    const ASTFile& file = *result.file;
    assert(file.items.empty());
    assert(file.module_doc.empty());
    assert(result.diags.empty());
  }

  {
    const std::vector<UnicodeScalar> scalars = {'}', '}'};
    const SourceFile source = MakeSourceFile("parse_err.cursive", scalars);
    auto result = ParseFile(source);
    assert(result.file.has_value());
    const ASTFile& file = *result.file;
    assert(file.items.size() == 2);
    assert(result.diags.size() == 2);
    assert(result.diags[0].code == "E-SRC-0520");
    assert(result.diags[1].code == "E-SRC-0520");
    assert(result.diags[0].span.has_value());
    assert(result.diags[1].span.has_value());

    const auto& item0 = std::get<ErrorItem>(file.items[0]);
    const auto& item1 = std::get<ErrorItem>(file.items[1]);
    const Span span0 = SpanOf(source, 0, 1);
    const Span span1 = SpanOf(source, 1, 2);
    ExpectSpanEq(item0.span, span0);
    ExpectSpanEq(item1.span, span1);
    ExpectSpanEq(*result.diags[0].span, span0);
    ExpectSpanEq(*result.diags[1].span, span1);
  }

  return 0;
}
