#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/span.h"
#include "cursive0/core/source_text.h"

namespace {

using cursive0::core::LineStarts;
using cursive0::core::Locate;
using cursive0::core::SourceFile;
using cursive0::core::Span;
using cursive0::core::SpanOf;
using cursive0::core::SpanRange;
using cursive0::core::UnicodeScalar;

static SourceFile MakeSourceFile(const std::string& path, const std::vector<UnicodeScalar>& scalars) {
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

static void ExpectLocate(const SourceFile& s,
                         std::size_t offset,
                         std::size_t exp_line,
                         std::size_t exp_col,
                         std::size_t exp_off) {
  const auto loc = Locate(s, offset);
  assert(loc.offset == exp_off);
  assert(loc.line == exp_line);
  assert(loc.column == exp_col);
}

}  // namespace

int main() {
  SPEC_COV("WF-Location");
  SPEC_COV("WF-Span");
  SPEC_COV("Span-Of");

  {
    const std::vector<UnicodeScalar> scalars = {'a', 'b', 'c'};
    const auto s = MakeSourceFile("single.cursive", scalars);

    ExpectLocate(s, 0, 1, 1, 0);
    ExpectLocate(s, 2, 1, 3, 2);
    ExpectLocate(s, 5, 1, 4, 3);
  }

  {
    const std::vector<UnicodeScalar> scalars = {'a', 'b', 0x0A, 'c', 'd', 0x0A};
    const auto s = MakeSourceFile("multi.cursive", scalars);

    ExpectLocate(s, 0, 1, 1, 0);
    ExpectLocate(s, 3, 2, 1, 3);
    ExpectLocate(s, 6, 3, 1, 6);

    const Span sp = SpanOf(s, 1, 4);
    assert(sp.start_offset == 1);
    assert(sp.end_offset == 4);
    assert(sp.start_line == 1);
    assert(sp.start_col == 2);
    assert(sp.end_line == 2);
    assert(sp.end_col == 2);

    const auto range = SpanRange(sp);
    assert(range.first == 1);
    assert(range.second == 4);
  }

  {
    const std::vector<UnicodeScalar> scalars = {'a', 0x00E9, 'b'};
    const auto s = MakeSourceFile("utf8.cursive", scalars);

    ExpectLocate(s, 1, 1, 2, 1);
    ExpectLocate(s, 2, 1, 3, 2);
    ExpectLocate(s, 3, 1, 4, 3);
  }

  {
    const std::vector<UnicodeScalar> scalars = {'a', 'b', 0x0A, 'c', 'd', 0x0A};
    const auto s = MakeSourceFile("clamp.cursive", scalars);

    const Span sp = SpanOf(s, 10, 2);
    assert(sp.start_offset == 6);
    assert(sp.end_offset == 6);
    assert(sp.start_line == 3);
    assert(sp.start_col == 1);
    assert(sp.end_line == 3);
    assert(sp.end_col == 1);
  }

  return 0;
}
