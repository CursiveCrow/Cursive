#include "cursive0/core/source_load.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/diagnostic_messages.h"
#include "cursive0/core/diagnostics.h"
#include "cursive0/core/span.h"
#include "cursive0/core/source_text.h"
#include "cursive0/core/unicode.h"

namespace cursive0::core {

namespace {

bool IsContinuation(std::uint8_t byte) {
  return (byte & 0xC0) == 0x80;
}

bool IsSurrogate(UnicodeScalar u) {
  return u >= 0xD800 && u <= 0xDFFF;
}

std::optional<std::vector<UnicodeScalar>> DecodeUtf8Internal(
    const std::vector<std::uint8_t>& bytes) {
  std::vector<UnicodeScalar> out;
  out.reserve(bytes.size());

  std::size_t i = 0;
  while (i < bytes.size()) {
    const std::uint8_t b0 = bytes[i];
    if (b0 <= 0x7F) {
      out.push_back(b0);
      ++i;
      continue;
    }

    if ((b0 & 0xE0) == 0xC0) {
      if (i + 1 >= bytes.size()) {
        return std::nullopt;
      }
      const std::uint8_t b1 = bytes[i + 1];
      if (!IsContinuation(b1)) {
        return std::nullopt;
      }
      const UnicodeScalar u =
          static_cast<UnicodeScalar>(((b0 & 0x1F) << 6) | (b1 & 0x3F));
      if (u < 0x80) {
        return std::nullopt;
      }
      out.push_back(u);
      i += 2;
      continue;
    }

    if ((b0 & 0xF0) == 0xE0) {
      if (i + 2 >= bytes.size()) {
        return std::nullopt;
      }
      const std::uint8_t b1 = bytes[i + 1];
      const std::uint8_t b2 = bytes[i + 2];
      if (!IsContinuation(b1) || !IsContinuation(b2)) {
        return std::nullopt;
      }
      const UnicodeScalar u =
          static_cast<UnicodeScalar>(((b0 & 0x0F) << 12) | ((b1 & 0x3F) << 6) | (b2 & 0x3F));
      if (u < 0x800 || IsSurrogate(u)) {
        return std::nullopt;
      }
      out.push_back(u);
      i += 3;
      continue;
    }

    if ((b0 & 0xF8) == 0xF0) {
      if (i + 3 >= bytes.size()) {
        return std::nullopt;
      }
      const std::uint8_t b1 = bytes[i + 1];
      const std::uint8_t b2 = bytes[i + 2];
      const std::uint8_t b3 = bytes[i + 3];
      if (!IsContinuation(b1) || !IsContinuation(b2) || !IsContinuation(b3)) {
        return std::nullopt;
      }
      const UnicodeScalar u = static_cast<UnicodeScalar>(((b0 & 0x07) << 18)
                                                         | ((b1 & 0x3F) << 12)
                                                         | ((b2 & 0x3F) << 6)
                                                         | (b3 & 0x3F));
      if (u < 0x10000 || u > 0x10FFFF) {
        return std::nullopt;
      }
      out.push_back(u);
      i += 4;
      continue;
    }

    return std::nullopt;
  }

  return out;
}


SourceFile BuildSpanSource(std::string_view path,
                           const std::vector<std::uint8_t>& bytes,
                           std::vector<UnicodeScalar> scalars) {
  SourceFile source;
  source.path = std::string(path);
  source.bytes = bytes;
  source.scalars = std::move(scalars);
  source.text = EncodeUtf8(source.scalars);
  source.byte_len = source.text.size();
  source.line_starts = LineStarts(source.scalars);
  source.line_count = source.line_starts.size();
  return source;
}

std::optional<std::size_t> FirstBomIndex(
    const std::vector<UnicodeScalar>& scalars) {
  for (std::size_t i = 0; i < scalars.size(); ++i) {
    if (scalars[i] == kBOM) {
      return i;
    }
  }
  return std::nullopt;
}

Span SpanAtIndex(const SourceFile& source,
                 const std::vector<std::size_t>& offsets,
                 std::size_t index) {
  const std::size_t start = offsets[index];
  const std::size_t end = offsets[index + 1];
  return SpanOf(source, start, end);
}

}  // namespace

DecodeResult Decode(const std::vector<std::uint8_t>& bytes) {
  DecodeResult result;
  auto scalars = DecodeUtf8Internal(bytes);
  if (!scalars) {
    SPEC_RULE("Decode-Err");
    return result;
  }
  SPEC_RULE("Decode-Ok");
  result.scalars = std::move(*scalars);
  result.ok = true;
  return result;
}

StripBOMResult StripBOM(const std::vector<UnicodeScalar>& scalars) {
  StripBOMResult result;
  if (scalars.empty()) {
    SPEC_RULE("StripBOM-Empty");
    return result;
  }

  const bool has_lead = scalars.front() == kBOM;
  result.had_bom = has_lead;
  if (has_lead) {
    result.scalars.assign(scalars.begin() + 1, scalars.end());
  } else {
    result.scalars = scalars;
  }

  for (std::size_t i = 0; i < result.scalars.size(); ++i) {
    if (result.scalars[i] == kBOM) {
      result.embedded_index = i;
      SPEC_RULE("StripBOM-Embedded");
      return result;
    }
  }

  if (has_lead) {
    SPEC_RULE("StripBOM-Start");
  } else {
    SPEC_RULE("StripBOM-None");
  }
  return result;
}

std::vector<UnicodeScalar> NormalizeLF(const std::vector<UnicodeScalar>& scalars) {
  if (scalars.empty()) {
    SPEC_RULE("Norm-Empty");
    return {};
  }

  std::vector<UnicodeScalar> out;
  out.reserve(scalars.size());

  std::size_t i = 0;
  while (i < scalars.size()) {
    const UnicodeScalar c = scalars[i];
    if (c == kCR) {
      if (i + 1 < scalars.size() && scalars[i + 1] == kLF) {
        SPEC_RULE("Norm-CRLF");
        out.push_back(kLF);
        i += 2;
      } else {
        SPEC_RULE("Norm-CR");
        out.push_back(kLF);
        ++i;
      }
      continue;
    }
    if (c == kLF) {
      SPEC_RULE("Norm-LF");
      out.push_back(kLF);
      ++i;
      continue;
    }
    SPEC_RULE("Norm-Other");
    out.push_back(c);
    ++i;
  }

  return out;
}

SourceLoadResult LoadSource(std::string_view path,
                            const std::vector<std::uint8_t>& bytes) {
  SourceLoadResult result;
  SPEC_RULE("Step-Size");

  const DecodeResult decoded = Decode(bytes);
  if (!decoded.ok) {
    SPEC_RULE("Step-Decode-Err");
    SPEC_RULE("NoSpan-Decode");
    if (auto diag = MakeDiagnostic("E-SRC-0101")) {
      result.diags = Emit(result.diags, *diag);
    }
    SPEC_RULE("LoadSource-Err");
    return result;
  }
  SPEC_RULE("Step-Decode");

  const StripBOMResult stripped = StripBOM(decoded.scalars);
  SPEC_RULE("Step-BOM");

  std::vector<UnicodeScalar> normalized = NormalizeLF(stripped.scalars);
  SPEC_RULE("Step-Norm");

  SourceFile source = BuildSpanSource(path, bytes, std::move(normalized));
  const std::vector<std::size_t> offsets = Utf8Offsets(source.scalars);

  if (stripped.had_bom) {
    SPEC_RULE("Span-BOM-Warn");
    const std::size_t end = std::min<std::size_t>(1, source.byte_len);
    if (auto diag = MakeDiagnostic("W-SRC-0101", SpanOf(source, 0, end))) {
      result.diags = Emit(result.diags, *diag);
    }
  }

  if (stripped.embedded_index.has_value()) {
    SPEC_RULE("Step-EmbeddedBOM-Err");
    std::size_t bom_index = 0;
    if (auto index = FirstBomIndex(source.scalars)) {
      bom_index = *index;
    }
    SPEC_RULE("Span-BOM-Embedded");
    if (auto diag = MakeDiagnostic("E-SRC-0103", SpanAtIndex(source, offsets, bom_index))) {
      result.diags = Emit(result.diags, *diag);
    }
    SPEC_RULE("LoadSource-Err");
    return result;
  }

  SPEC_RULE("Step-LineMap");

  if (!NoProhibited(source.scalars)) {
    SPEC_RULE("Step-Prohibited-Err");
    std::size_t prohibited_index = 0;
    if (auto index = FirstProhibitedOutsideLiteral(source.scalars)) {
      prohibited_index = *index;
    }
    SPEC_RULE("Span-Prohibited");
    if (auto diag = MakeDiagnostic("E-SRC-0104", SpanAtIndex(source, offsets, prohibited_index))) {
      result.diags = Emit(result.diags, *diag);
    }
    SPEC_RULE("LoadSource-Err");
    return result;
  }
  SPEC_RULE("Step-Prohibited");

  result.source = std::move(source);
  SPEC_RULE("LoadSource-Ok");
  return result;
}

}  // namespace cursive0::core
