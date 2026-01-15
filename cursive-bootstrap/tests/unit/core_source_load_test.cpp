#include <cassert>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/source_load.h"
#include "cursive0/core/source_text.h"

namespace {

using cursive0::core::Decode;
using cursive0::core::Diagnostic;
using cursive0::core::DecodeResult;
using cursive0::core::LoadSource;
using cursive0::core::SourceLoadResult;
using cursive0::core::SourceFile;
using cursive0::core::Span;
using cursive0::core::StripBOM;
using cursive0::core::StripBOMResult;
using cursive0::core::UnicodeScalar;
using cursive0::core::EncodeUtf8;
using cursive0::core::kBOM;
using cursive0::core::kLF;

void ExpectDecodeOk(const std::vector<std::uint8_t>& bytes,
                    const std::vector<UnicodeScalar>& expected) {
  const DecodeResult result = Decode(bytes);
  assert(result.ok);
  assert(result.scalars == expected);
}

void ExpectDecodeErr(const std::vector<std::uint8_t>& bytes) {
  const DecodeResult result = Decode(bytes);
  assert(!result.ok);
}

void ExpectStripBOM(const std::vector<UnicodeScalar>& scalars,
                    const std::vector<UnicodeScalar>& expected,
                    bool had_bom,
                    const std::optional<std::size_t>& embedded) {
  const StripBOMResult result = StripBOM(scalars);
  assert(result.scalars == expected);
  assert(result.had_bom == had_bom);
  assert(result.embedded_index == embedded);
}

std::vector<std::uint8_t> BytesFromScalars(
    const std::vector<UnicodeScalar>& scalars) {
  const std::string text = EncodeUtf8(scalars);
  return std::vector<std::uint8_t>(text.begin(), text.end());
}

const Diagnostic& ExpectSingleDiag(const SourceLoadResult& result,
                                  std::string_view code) {
  assert(result.diags.size() == 1);
  assert(result.diags[0].code == code);
  return result.diags[0];
}

const Diagnostic& ExpectErrorDiag(const SourceLoadResult& result,
                                 std::string_view code) {
  assert(!result.source.has_value());
  return ExpectSingleDiag(result, code);
}

void ExpectDiagNoSpan(const Diagnostic& diag) {
  assert(!diag.span.has_value());
}

void ExpectDiagSpan(const Diagnostic& diag,
                    std::size_t start,
                    std::size_t end,
                    std::size_t start_line,
                    std::size_t start_col,
                    std::size_t end_line,
                    std::size_t end_col) {
  assert(diag.span.has_value());
  const Span& span = *diag.span;
  assert(span.start_offset == start);
  assert(span.end_offset == end);
  assert(span.start_line == start_line);
  assert(span.start_col == start_col);
  assert(span.end_line == end_line);
  assert(span.end_col == end_col);
}

}  // namespace

int main() {
  SPEC_COV("Decode-Ok");
  SPEC_COV("Decode-Err");
  SPEC_COV("StripBOM-Empty");
  SPEC_COV("StripBOM-None");
  SPEC_COV("StripBOM-Start");
  SPEC_COV("StripBOM-Embedded");
  SPEC_COV("Step-Size");
  SPEC_COV("Step-Decode");
  SPEC_COV("Step-Decode-Err");
  SPEC_COV("NoSpan-Decode");
  SPEC_COV("Step-BOM");
  SPEC_COV("Span-BOM-Warn");
  SPEC_COV("Span-BOM-Embedded");
  SPEC_COV("Step-Norm");
  SPEC_COV("Step-EmbeddedBOM-Err");
  SPEC_COV("Step-LineMap");
  SPEC_COV("Step-Prohibited");
  SPEC_COV("Step-Prohibited-Err");
  SPEC_COV("Span-Prohibited");
  SPEC_COV("LoadSource-Ok");
  SPEC_COV("LoadSource-Err");

  {
    const std::vector<std::uint8_t> bytes = {
        0x61,
        0xC3, 0xA9,
        0xE2, 0x82, 0xAC,
        0xF0, 0x9F, 0x98, 0x80,
    };
    const std::vector<UnicodeScalar> expected = {
        0x61,
        0x00E9,
        0x20AC,
        0x1F600,
    };
    ExpectDecodeOk(bytes, expected);
  }

  {
    const std::vector<std::uint8_t> bytes = {0xEF, 0xBB, 0xBF};
    const std::vector<UnicodeScalar> expected = {kBOM};
    ExpectDecodeOk(bytes, expected);
  }

  {
    const std::vector<std::uint8_t> bytes = {0xC0, 0xAF};
    ExpectDecodeErr(bytes);
  }

  {
    const std::vector<std::uint8_t> bytes = {0xE2, 0x28, 0xA1};
    ExpectDecodeErr(bytes);
  }

  {
    const std::vector<std::uint8_t> bytes = {0xF0, 0x9F, 0x98};
    ExpectDecodeErr(bytes);
  }

  {
    const std::vector<std::uint8_t> bytes = {0xED, 0xA0, 0x80};
    ExpectDecodeErr(bytes);
  }

  {
    const std::vector<std::uint8_t> bytes = {0xF4, 0x90, 0x80, 0x80};
    ExpectDecodeErr(bytes);
  }

  {
    const std::vector<UnicodeScalar> scalars = {};
    ExpectStripBOM(scalars, {}, false, std::nullopt);
  }

  {
    const std::vector<UnicodeScalar> scalars = {'a', 'b'};
    ExpectStripBOM(scalars, scalars, false, std::nullopt);
  }

  {
    const std::vector<UnicodeScalar> scalars = {kBOM};
    ExpectStripBOM(scalars, {}, true, std::nullopt);
  }

  {
    const std::vector<UnicodeScalar> scalars = {kBOM, 'a', 'b'};
    const std::vector<UnicodeScalar> expected = {'a', 'b'};
    ExpectStripBOM(scalars, expected, true, std::nullopt);
  }

  {
    const std::vector<UnicodeScalar> scalars = {'a', kBOM, 'b'};
    const std::vector<UnicodeScalar> expected = {'a', kBOM, 'b'};
    ExpectStripBOM(scalars, expected, false, 1);
  }

  {
    const std::vector<UnicodeScalar> scalars = {kBOM, 'a', kBOM, 'b'};
    const std::vector<UnicodeScalar> expected = {'a', kBOM, 'b'};
    ExpectStripBOM(scalars, expected, true, 1);
  }

  {
    const std::string input = "a\r\nb\rc\n";
    const std::vector<std::uint8_t> bytes(input.begin(), input.end());
    const SourceLoadResult result = LoadSource("ok.cursive", bytes);
    assert(result.source.has_value());
    assert(result.diags.empty());

    const SourceFile& source = *result.source;
    const std::vector<UnicodeScalar> expected = {
        'a', kLF, 'b', kLF, 'c', kLF,
    };
    const std::vector<std::size_t> expected_lines = {0, 2, 4, 6};
    assert(source.bytes == bytes);
    assert(source.scalars == expected);
    assert(source.text == "a\nb\nc\n");
    assert(source.byte_len == 6);
    assert(source.line_starts == expected_lines);
    assert(source.line_count == expected_lines.size());
  }

  {
    const std::vector<std::uint8_t> bytes = {0xC0, 0xAF};
    const SourceLoadResult result = LoadSource("bad_utf8.cursive", bytes);
    const Diagnostic& diag = ExpectErrorDiag(result, "E-SRC-0101");
    ExpectDiagNoSpan(diag);
  }

  {
    const std::vector<UnicodeScalar> scalars = {kBOM, 'a'};
    const std::vector<std::uint8_t> bytes = BytesFromScalars(scalars);
    const SourceLoadResult result = LoadSource("bom_warn.cursive", bytes);
    assert(result.source.has_value());
    const Diagnostic& diag = ExpectSingleDiag(result, "W-SRC-0101");
    ExpectDiagSpan(diag, 0, 1, 1, 1, 1, 2);
  }

  {
    const std::vector<UnicodeScalar> scalars = {'a', kBOM, 'b'};
    const std::vector<std::uint8_t> bytes = BytesFromScalars(scalars);
    const SourceLoadResult result = LoadSource("bom.cursive", bytes);
    const Diagnostic& diag = ExpectErrorDiag(result, "E-SRC-0103");
    ExpectDiagSpan(diag, 1, 4, 1, 2, 1, 5);
  }

  {
    const std::vector<UnicodeScalar> scalars = {'a', 0x00, 'b'};
    const std::vector<std::uint8_t> bytes = BytesFromScalars(scalars);
    const SourceLoadResult result = LoadSource("prohibited.cursive", bytes);
    const Diagnostic& diag = ExpectErrorDiag(result, "E-SRC-0104");
    ExpectDiagSpan(diag, 1, 2, 1, 2, 1, 3);
  }

  return 0;
}
