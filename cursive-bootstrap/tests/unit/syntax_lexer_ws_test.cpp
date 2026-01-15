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
using cursive0::syntax::CommentScanResult;
using cursive0::syntax::DocKind;
using cursive0::syntax::LexNewlines;
using cursive0::syntax::ScalarRange;
using cursive0::syntax::ScanBlockComment;
using cursive0::syntax::ScanDocComment;
using cursive0::syntax::ScanLineComment;
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
  SPEC_COV("Scan-Line-Comment");
  SPEC_COV("Doc-Comment");
  SPEC_COV("Block-Start");
  SPEC_COV("Block-Step");
  SPEC_COV("Block-End");
  SPEC_COV("Block-Done");
  SPEC_COV("Block-Comment-Unterminated");

  {
    const auto scalars = AsScalars("//a\nb");
    const auto source = MakeSourceFile("line_comment.cursive", scalars);
    CommentScanResult res = ScanLineComment(source, 0);
    assert(res.ok);
    assert(!res.doc.has_value());
    assert(res.next == 3);
    assert(res.range.start == 0);
    assert(res.range.end == 3);
  }

  {
    const auto scalars = AsScalars("/// foo\n");
    const auto source = MakeSourceFile("doc_line.cursive", scalars);
    CommentScanResult res = ScanDocComment(source, 0);
    assert(res.ok);
    assert(res.doc.has_value());
    assert(res.doc->kind == DocKind::LineDoc);
    assert(res.doc->text == "foo");
    const Span expect = SpanOf(source, 0, 7);
    ExpectSpanEq(res.doc->span, expect);
  }

  {
    const auto scalars = AsScalars("//!bar\n");
    const auto source = MakeSourceFile("doc_module.cursive", scalars);
    CommentScanResult res = ScanDocComment(source, 0);
    assert(res.ok);
    assert(res.doc.has_value());
    assert(res.doc->kind == DocKind::ModuleDoc);
    assert(res.doc->text == "bar");
    const Span expect = SpanOf(source, 0, 6);
    ExpectSpanEq(res.doc->span, expect);
  }

  {
    const auto scalars = AsScalars("/* a /* b */ c */");
    const auto source = MakeSourceFile("block_nested.cursive", scalars);
    CommentScanResult res = ScanBlockComment(source, 0);
    assert(res.ok);
    assert(res.next == scalars.size());
    assert(res.range.start == 0);
    assert(res.range.end == scalars.size());
  }

  {
    const auto scalars = AsScalars("/* a");
    const auto source = MakeSourceFile("block_unterminated.cursive", scalars);
    CommentScanResult res = ScanBlockComment(source, 0);
    assert(!res.ok);
    assert(res.diags.size() == 1);
    assert(res.diags[0].code == "E-SRC-0306");
    assert(res.diags[0].span.has_value());
    const Span expect = SpanOf(source, 0, 2);
    ExpectSpanEq(*res.diags[0].span, expect);
  }

  {
    const auto scalars = AsScalars("a/*\n*/\n");
    const auto source = MakeSourceFile("newline_block.cursive", scalars);
    CommentScanResult res = ScanBlockComment(source, 1);
    assert(res.ok);
    const std::vector<ScalarRange> suppressed = {res.range};
    const auto tokens = LexNewlines(source, suppressed);
    assert(tokens.size() == 1);
    assert(tokens[0].kind == TokenKind::Newline);
    ExpectSpanEq(tokens[0].span, SpanOf(source, 6, 7));
  }

  {
    const auto scalars = AsScalars("a//x\nb");
    const auto source = MakeSourceFile("newline_line.cursive", scalars);
    CommentScanResult res = ScanLineComment(source, 1);
    assert(res.ok);
    const std::vector<ScalarRange> suppressed = {res.range};
    const auto tokens = LexNewlines(source, suppressed);
    assert(tokens.size() == 1);
    assert(tokens[0].kind == TokenKind::Newline);
    ExpectSpanEq(tokens[0].span, SpanOf(source, 4, 5));
  }

  return 0;
}
