#include "cursive0/00_core/unicode.h"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <string_view>
#include <vector>

#include <unicode/normalizer2.h>
#include <unicode/stringpiece.h>
#include <unicode/uchar.h>
#include <unicode/unistr.h>
#include <unicode/utypes.h>
#include <unicode/uversion.h>

#include "cursive0/00_core/assert_spec.h"

namespace cursive0::core {

static inline void SpecDefsUnicode() {
  SPEC_DEF("NFC", "3.1.6");
  SPEC_DEF("CaseFold", "3.1.6");
}

std::string NFC(std::string_view s) {
  SpecDefsUnicode();
  constexpr std::string_view kUnicodeVersion = U_UNICODE_VERSION;
  static_assert(kUnicodeVersion == "15.0",
                "ICU must target Unicode 15.0.x to match the Cursive0 spec.");
  UErrorCode status = U_ZERO_ERROR;
  const icu::Normalizer2* nfc = icu::Normalizer2::getNFCInstance(status);
  if (U_FAILURE(status) || nfc == nullptr) {
    std::abort();
  }
  const icu::UnicodeString input =
      icu::UnicodeString::fromUTF8(icu::StringPiece(s.data(), s.size()));
  icu::UnicodeString normalized = nfc->normalize(input, status);
  if (U_FAILURE(status)) {
    std::abort();
  }
  std::string out;
  normalized.toUTF8String(out);
  return out;
}

std::string CaseFold(std::string_view s) {
  SpecDefsUnicode();
  constexpr std::string_view kUnicodeVersion = U_UNICODE_VERSION;
  static_assert(kUnicodeVersion == "15.0",
                "ICU must target Unicode 15.0.x to match the Cursive0 spec.");
  const icu::UnicodeString input =
      icu::UnicodeString::fromUTF8(icu::StringPiece(s.data(), s.size()));
  icu::UnicodeString folded = input;
  folded.foldCase(U_FOLD_CASE_DEFAULT);
  std::string out;
  folded.toUTF8String(out);
  return out;
}

static inline void SpecDefsIdentChars() {
  SPEC_DEF("IdentStart", "3.2.2");
  SPEC_DEF("IdentContinue", "3.2.2");
  SPEC_DEF("XID_Start", "3.2.2");
  SPEC_DEF("XID_Continue", "3.2.2");
  SPEC_DEF("NonCharacter", "3.2.2");
  SPEC_DEF("Sensitive", "3.2.2");
}

bool IsXidStart(UnicodeScalar c) {
  SpecDefsIdentChars();
  if (c > 0x10FFFF) {
    return false;
  }
  return u_hasBinaryProperty(static_cast<UChar32>(c), UCHAR_XID_START);
}

bool IsXidContinue(UnicodeScalar c) {
  SpecDefsIdentChars();
  if (c > 0x10FFFF) {
    return false;
  }
  return u_hasBinaryProperty(static_cast<UChar32>(c), UCHAR_XID_CONTINUE);
}

bool IsNonCharacter(UnicodeScalar c) {
  SpecDefsIdentChars();
  if (c > 0x10FFFF) {
    return false;
  }
  if (c >= 0xFDD0 && c <= 0xFDEF) {
    return true;
  }
  const std::uint32_t low = static_cast<std::uint32_t>(c) & 0xFFFFu;
  return low == 0xFFFEu || low == 0xFFFFu;
}

bool IsIdentStart(UnicodeScalar c) {
  SpecDefsIdentChars();
  return c == '_' || IsXidStart(c);
}

bool IsIdentContinue(UnicodeScalar c) {
  SpecDefsIdentChars();
  return c == '_' || IsXidContinue(c);
}

bool IsSensitive(UnicodeScalar c) {
  SpecDefsIdentChars();
  return (c >= 0x202Au && c <= 0x202Eu) || (c >= 0x2066u && c <= 0x2069u) ||
         c == 0x200Cu || c == 0x200Du;
}

namespace {

struct ByteSpan {
  std::size_t start = 0;
  std::size_t end = 0;
};

bool IsHexDigit(UnicodeScalar c) {
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
         (c >= 'A' && c <= 'F');
}

unsigned int HexValue(UnicodeScalar c) {
  if (c >= '0' && c <= '9') {
    return static_cast<unsigned int>(c - '0');
  }
  if (c >= 'a' && c <= 'f') {
    return 10u + static_cast<unsigned int>(c - 'a');
  }
  return 10u + static_cast<unsigned int>(c - 'A');
}

bool IsUnicodeScalarValue(std::uint32_t value) {
  if (value > 0x10FFFF) {
    return false;
  }
  return !(value >= 0xD800 && value <= 0xDFFF);
}

bool IsStringChar(UnicodeScalar c) {
  return c != '"' && c != '\\' && c != kLF;
}

bool IsCharContent(UnicodeScalar c) {
  return c != '\'' && c != '\\' && c != kLF;
}

std::optional<std::size_t> ScanEscape(const std::vector<UnicodeScalar>& scalars,
                                      std::size_t start) {
  if (start + 1 >= scalars.size() || scalars[start] != '\\') {
    return std::nullopt;
  }
  const UnicodeScalar next = scalars[start + 1];
  switch (next) {
    case '\\':
    case '"':
    case '\'':
    case 'n':
    case 'r':
    case 't':
    case '0':
      return start + 2;
    case 'x': {
      if (start + 3 >= scalars.size()) {
        return std::nullopt;
      }
      if (!IsHexDigit(scalars[start + 2]) || !IsHexDigit(scalars[start + 3])) {
        return std::nullopt;
      }
      return start + 4;
    }
    case 'u': {
      if (start + 2 >= scalars.size() || scalars[start + 2] != '{') {
        return std::nullopt;
      }
      std::size_t p = start + 3;
      std::uint32_t value = 0;
      std::size_t digits = 0;
      while (p < scalars.size() && IsHexDigit(scalars[p])) {
        if (digits == 6) {
          return std::nullopt;
        }
        value = (value << 4) | HexValue(scalars[p]);
        ++digits;
        ++p;
      }
      if (digits == 0) {
        return std::nullopt;
      }
      if (p >= scalars.size() || scalars[p] != '}') {
        return std::nullopt;
      }
      if (!IsUnicodeScalarValue(value)) {
        return std::nullopt;
      }
      return p + 1;
    }
    default:
      return std::nullopt;
  }
}

std::optional<std::size_t> ScanStringLiteral(
    const std::vector<UnicodeScalar>& scalars,
    std::size_t start) {
  if (start >= scalars.size() || scalars[start] != '"') {
    return std::nullopt;
  }
  std::size_t i = start + 1;
  while (i < scalars.size()) {
    const UnicodeScalar c = scalars[i];
    if (c == '"') {
      return i + 1;
    }
    if (c == kLF) {
      return std::nullopt;
    }
    if (c == '\\') {
      const auto escaped = ScanEscape(scalars, i);
      if (!escaped.has_value()) {
        return std::nullopt;
      }
      i = *escaped;
      continue;
    }
    if (!IsStringChar(c)) {
      return std::nullopt;
    }
    ++i;
  }
  return std::nullopt;
}

std::optional<std::size_t> ScanCharLiteral(
    const std::vector<UnicodeScalar>& scalars,
    std::size_t start) {
  if (start >= scalars.size() || scalars[start] != '\'') {
    return std::nullopt;
  }
  if (start + 1 >= scalars.size()) {
    return std::nullopt;
  }
  if (scalars[start + 1] == kLF) {
    return std::nullopt;
  }
  std::size_t i = start + 1;
  if (scalars[i] == '\\') {
    const auto escaped = ScanEscape(scalars, i);
    if (!escaped.has_value()) {
      return std::nullopt;
    }
    i = *escaped;
  } else {
    if (!IsCharContent(scalars[i])) {
      return std::nullopt;
    }
    i += 1;
  }
  if (i >= scalars.size() || scalars[i] != '\'') {
    return std::nullopt;
  }
  return i + 1;
}

std::vector<ByteSpan> LiteralByteSpans(
    const std::vector<UnicodeScalar>& scalars) {
  const auto offsets = Utf8Offsets(scalars);
  std::vector<ByteSpan> spans;
  std::size_t i = 0;
  while (i < scalars.size()) {
    std::optional<std::size_t> end;
    if (scalars[i] == '"') {
      end = ScanStringLiteral(scalars, i);
    } else if (scalars[i] == '\'') {
      end = ScanCharLiteral(scalars, i);
    }
    if (end.has_value()) {
      spans.push_back(ByteSpan{offsets[i], offsets[*end]});
      i = *end;
      continue;
    }
    ++i;
  }
  return spans;
}

bool ByteInLiteralSpan(std::size_t offset,
                       const std::vector<ByteSpan>& spans,
                       std::size_t* span_index) {
  while (*span_index < spans.size() && offset >= spans[*span_index].end) {
    ++(*span_index);
  }
  if (*span_index >= spans.size()) {
    return false;
  }
  return offset >= spans[*span_index].start &&
         offset < spans[*span_index].end;
}

}  // namespace

bool IsProhibited(UnicodeScalar c) {
  const bool is_cc = (c <= 0x1F) || (c >= 0x7F && c <= 0x9F);
  if (!is_cc) {
    return false;
  }
  return c != 0x09 && c != 0x0A && c != 0x0C && c != 0x0D;
}

std::optional<std::size_t> FirstProhibitedOutsideLiteral(
    const std::vector<UnicodeScalar>& scalars) {
  const auto offsets = Utf8Offsets(scalars);
  const auto spans = LiteralByteSpans(scalars);
  std::size_t span_index = 0;
  for (std::size_t i = 0; i < scalars.size(); ++i) {
    if (!IsProhibited(scalars[i])) {
      continue;
    }
    const std::size_t offset = offsets[i];
    if (!ByteInLiteralSpan(offset, spans, &span_index)) {
      return i;
    }
  }
  return std::nullopt;
}

bool NoProhibited(const std::vector<UnicodeScalar>& scalars) {
  SPEC_RULE("WF-Prohibited");
  return !FirstProhibitedOutsideLiteral(scalars).has_value();
}

}  // namespace cursive0::core
