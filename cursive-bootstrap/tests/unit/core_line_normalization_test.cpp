#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/span.h"
#include "cursive0/core/source_text.h"

namespace {

using cursive0::core::EncodeUtf8;
using cursive0::core::LineStarts;
using cursive0::core::Locate;
using cursive0::core::NormalizeLF;
using cursive0::core::SourceFile;
using cursive0::core::UnicodeScalar;
using cursive0::core::kCR;
using cursive0::core::kLF;

void ExpectNormalize(const std::vector<UnicodeScalar>& input,
                     const std::vector<UnicodeScalar>& expected) {
  const auto out = NormalizeLF(input);
  assert(out == expected);
}

static SourceFile MakeSourceFile(const std::string& path,
                                 const std::vector<UnicodeScalar>& scalars) {
  SourceFile s;
  s.path = path;
  s.scalars = scalars;
  s.text = EncodeUtf8(scalars);
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
  SPEC_COV("Norm-Empty");
  SPEC_COV("Norm-CRLF");
  SPEC_COV("Norm-CR");
  SPEC_COV("Norm-LF");
  SPEC_COV("Norm-Other");

  {
    ExpectNormalize({}, {});
  }

  {
    const std::vector<UnicodeScalar> input = {kCR, kLF};
    const std::vector<UnicodeScalar> expected = {kLF};
    ExpectNormalize(input, expected);
  }

  {
    const std::vector<UnicodeScalar> input = {kCR};
    const std::vector<UnicodeScalar> expected = {kLF};
    ExpectNormalize(input, expected);
  }

  {
    const std::vector<UnicodeScalar> input = {kLF};
    const std::vector<UnicodeScalar> expected = {kLF};
    ExpectNormalize(input, expected);
  }

  {
    const std::vector<UnicodeScalar> input = {'x', kCR, 'y'};
    const std::vector<UnicodeScalar> expected = {'x', kLF, 'y'};
    ExpectNormalize(input, expected);
  }

  {
    const std::vector<UnicodeScalar> input = {
        'a', kCR, kLF, 'b', kCR, 'c', kLF, 'd', 0x00E9, kCR,
    };
    const std::vector<UnicodeScalar> expected = {
        'a', kLF, 'b', kLF, 'c', kLF, 'd', 0x00E9, kLF,
    };
    ExpectNormalize(input, expected);

    const SourceFile s = MakeSourceFile("lines.cursive", expected);
    const std::vector<std::size_t> expected_starts = {0, 2, 4, 6, 10};
    assert(s.line_starts == expected_starts);
    assert(s.line_count == expected_starts.size());
    assert(s.byte_len == 10);

    ExpectLocate(s, 0, 1, 1, 0);
    ExpectLocate(s, 1, 1, 2, 1);
    ExpectLocate(s, 2, 2, 1, 2);
    ExpectLocate(s, 3, 2, 2, 3);
    ExpectLocate(s, 4, 3, 1, 4);
    ExpectLocate(s, 6, 4, 1, 6);
    ExpectLocate(s, 7, 4, 2, 7);
    ExpectLocate(s, 8, 4, 3, 8);
    ExpectLocate(s, 9, 4, 4, 9);
    ExpectLocate(s, 10, 5, 1, 10);
  }

  return 0;
}
