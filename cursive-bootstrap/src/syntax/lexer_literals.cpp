#include "cursive0/syntax/lexer.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/diagnostic_messages.h"
#include "cursive0/core/source_text.h"
#include "cursive0/core/span.h"

namespace cursive0::syntax {

namespace {

using core::UnicodeScalar;

struct DigitScanResult {
  bool ok = false;
  bool malformed = false;
  std::size_t next = 0;
};

struct TerminatorResult {
  std::size_t index = 0;
  bool closed = false;
};

bool IsDecDigit(UnicodeScalar c) {
  return c >= '0' && c <= '9';
}

bool IsHexDigit(UnicodeScalar c) {
  return IsDecDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

bool IsOctDigit(UnicodeScalar c) {
  return c >= '0' && c <= '7';
}

bool IsBinDigit(UnicodeScalar c) {
  return c == '0' || c == '1';
}

bool IsUnicodeScalarValue(std::uint32_t value) {
  if (value > 0x10FFFF) {
    return false;
  }
  return !(value >= 0xD800 && value <= 0xDFFF);
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

bool StartsWith(std::string_view s, std::string_view prefix) {
  return s.size() >= prefix.size() && s.substr(0, prefix.size()) == prefix;
}

bool EndsWith(std::string_view s, std::string_view suffix) {
  return s.size() >= suffix.size() &&
         s.substr(s.size() - suffix.size()) == suffix;
}

bool NumericUnderscoreOk(std::string_view s) {
  if (s.empty()) {
    return true;
  }
  if (s.front() == '_' || s.back() == '_') {
    return false;
  }
  if (StartsWith(s, "0x_") || StartsWith(s, "0o_") || StartsWith(s, "0b_")) {
    return false;
  }

  for (std::size_t i = 0; i < s.size(); ++i) {
    if (s[i] != '_') {
      continue;
    }
    if (i > 0 && (s[i - 1] == 'e' || s[i - 1] == 'E')) {
      return false;
    }
    if (i + 1 < s.size() && (s[i + 1] == 'e' || s[i + 1] == 'E')) {
      return false;
    }
  }

  static constexpr std::string_view kIntSuffixes[] = {
      "i8",   "i16",  "i32",  "i64",  "i128",
      "u8",   "u16",  "u32",  "u64",  "u128",
      "isize", "usize",
  };
  static constexpr std::string_view kFloatSuffixes[] = {
      "f16",
      "f32",
      "f64",
  };

  for (std::string_view suf : kIntSuffixes) {
    if (s.size() > suf.size() + 1 &&
        s[s.size() - suf.size() - 1] == '_' && EndsWith(s, suf)) {
      return false;
    }
  }
  for (std::string_view suf : kFloatSuffixes) {
    if (s.size() > suf.size() + 1 &&
        s[s.size() - suf.size() - 1] == '_' && EndsWith(s, suf)) {
      return false;
    }
  }

  return true;
}

bool DecimalLeadingZero(std::string_view s) {
  std::string digits;
  digits.reserve(s.size());
  for (char c : s) {
    if (c != '_') {
      digits.push_back(c);
    }
  }
  return digits.size() > 1 && digits.front() == '0';
}

DigitScanResult ScanDigits(const std::vector<UnicodeScalar>& scalars,
                           std::size_t start,
                           bool (*pred)(UnicodeScalar)) {
  DigitScanResult result;
  const std::size_t n = scalars.size();
  if (start >= n || !pred(scalars[start])) {
    result.ok = false;
    result.next = start;
    return result;
  }
  result.ok = true;
  std::size_t p = start + 1;
  while (p < n) {
    if (pred(scalars[p])) {
      ++p;
      continue;
    }
    if (scalars[p] == '_') {
      while (p < n && scalars[p] == '_') {
        ++p;
      }
      if (p < n && pred(scalars[p])) {
        ++p;
        continue;
      }
      result.malformed = true;
      result.next = p;
      return result;
    }
    break;
  }
  result.next = p;
  return result;
}

std::optional<std::size_t> ScanEscapeMatch(
    const std::vector<UnicodeScalar>& scalars,
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

std::optional<std::size_t> FirstBadEscape(
    const std::vector<UnicodeScalar>& scalars,
    std::size_t start,
    std::size_t terminator) {
  std::size_t p = start + 1;
  while (p < terminator) {
    if (scalars[p] != '\\') {
      ++p;
      continue;
    }
    const auto match = ScanEscapeMatch(scalars, p);
    if (!match.has_value() || *match > terminator) {
      return p;
    }
    p = *match;
  }
  return std::nullopt;
}

std::size_t CharScalarCount(const std::vector<UnicodeScalar>& scalars,
                            std::size_t start,
                            std::size_t terminator) {
  std::size_t count = 0;
  std::size_t p = start + 1;
  while (p < terminator) {
    if (scalars[p] != '\\') {
      ++count;
      ++p;
      continue;
    }
    const auto match = ScanEscapeMatch(scalars, p);
    if (match.has_value() && *match <= terminator) {
      ++count;
      p = *match;
    } else {
      ++count;
      ++p;
    }
  }
  return count;
}

TerminatorResult FindTerminator(const std::vector<UnicodeScalar>& scalars,
                                std::size_t start,
                                UnicodeScalar quote) {
  TerminatorResult result;
  const std::size_t n = scalars.size();
  std::size_t backslashes = 0;
  for (std::size_t p = start + 1; p < n; ++p) {
    const UnicodeScalar c = scalars[p];
    if (c == core::kLF) {
      result.index = p;
      result.closed = false;
      return result;
    }
    if (c == '\\') {
      ++backslashes;
      continue;
    }
    if (c == quote && (backslashes % 2 == 0)) {
      result.index = p;
      result.closed = true;
      return result;
    }
    backslashes = 0;
  }
  result.index = n;
  result.closed = false;
  return result;
}

std::size_t MatchSuffix(const std::vector<UnicodeScalar>& scalars,
                        std::size_t start,
                        std::string_view suffix) {
  if (start + suffix.size() > scalars.size()) {
    return 0;
  }
  for (std::size_t i = 0; i < suffix.size(); ++i) {
    if (scalars[start + i] != static_cast<UnicodeScalar>(suffix[i])) {
      return 0;
    }
  }
  return suffix.size();
}

std::size_t MatchIntSuffix(const std::vector<UnicodeScalar>& scalars,
                           std::size_t start) {
  static constexpr std::string_view kIntSuffixes[] = {
      "i8",   "i16",  "i32",  "i64",  "i128",
      "u8",   "u16",  "u32",  "u64",  "u128",
      "isize", "usize",
  };
  for (std::string_view suf : kIntSuffixes) {
    const std::size_t len = MatchSuffix(scalars, start, suf);
    if (len > 0) {
      return len;
    }
  }
  return 0;
}

std::size_t MatchFloatSuffix(const std::vector<UnicodeScalar>& scalars,
                             std::size_t start) {
  static constexpr std::string_view kFloatSuffixes[] = {
      "f16",
      "f32",
      "f64",
  };
  for (std::string_view suf : kFloatSuffixes) {
    const std::size_t len = MatchSuffix(scalars, start, suf);
    if (len > 0) {
      return len;
    }
  }
  return 0;
}

core::Span SpanOfText(const core::SourceFile& source,
                      const std::vector<std::size_t>& offsets,
                      std::size_t i,
                      std::size_t j) {
  const std::size_t start = offsets[i];
  const std::size_t end = offsets[j];
  return core::SpanOf(source, start, end);
}

std::string LexemeSlice(const core::SourceFile& source,
                        const std::vector<std::size_t>& offsets,
                        std::size_t i,
                        std::size_t j) {
  const std::size_t start = offsets[i];
  const std::size_t end = offsets[j];
  if (start >= source.text.size() || end < start) {
    return std::string();
  }
  return source.text.substr(start, end - start);
}

void EmitDiag(core::DiagnosticStream& diags,
              std::string_view code,
              const core::Span& span) {
  const auto diag = core::MakeDiagnostic(code, span);
  if (!diag.has_value()) {
    return;
  }
  diags = core::Emit(diags, *diag);
}

}  // namespace

LiteralScanResult ScanIntLiteral(const core::SourceFile& source,
                                 std::size_t start) {
  SPEC_RULE("Lex-Int");
  SPEC_RULE("Lex-Numeric-Err");
  LiteralScanResult result;
  result.ok = false;
  result.next = start;

  const auto& scalars = source.scalars;
  const std::size_t n = scalars.size();
  if (start >= n || !IsDecDigit(scalars[start])) {
    return result;
  }

  const auto offsets = core::Utf8Offsets(scalars);
  std::size_t p = start;
  bool is_based = false;

  if (scalars[start] == '0' && start + 1 < n) {
    const UnicodeScalar next = scalars[start + 1];
    if (next == 'x' || next == 'o' || next == 'b') {
      is_based = true;
      p = start + 2;
      DigitScanResult digits;
      if (next == 'x') {
        digits = ScanDigits(scalars, p, IsHexDigit);
      } else if (next == 'o') {
        digits = ScanDigits(scalars, p, IsOctDigit);
      } else {
        digits = ScanDigits(scalars, p, IsBinDigit);
      }
      if (!digits.ok) {
        EmitDiag(result.diags, "E-SRC-0304", SpanOfText(source, offsets, start, p));
        result.next = p;
        return result;
      }
      if (digits.malformed) {
        EmitDiag(result.diags, "E-SRC-0304",
                 SpanOfText(source, offsets, start, digits.next));
        result.next = digits.next;
        return result;
      }
      p = digits.next;
    }
  }

  if (!is_based) {
    const DigitScanResult digits = ScanDigits(scalars, p, IsDecDigit);
    if (!digits.ok) {
      return result;
    }
    if (digits.malformed) {
      EmitDiag(result.diags, "E-SRC-0304",
               SpanOfText(source, offsets, start, digits.next));
      result.next = digits.next;
      return result;
    }
    p = digits.next;
  }

  const std::size_t suffix_len = MatchIntSuffix(scalars, p);
  const std::size_t j = p + suffix_len;

  result.ok = true;
  result.next = j;

  const std::string lexeme = LexemeSlice(source, offsets, start, j);
  if (!NumericUnderscoreOk(lexeme)) {
    EmitDiag(result.diags, "E-SRC-0304", SpanOfText(source, offsets, start, j));
  }

  if (!is_based) {
    const std::string digits_lexeme = LexemeSlice(source, offsets, start, p);
    if (DecimalLeadingZero(digits_lexeme)) {
      EmitDiag(result.diags, "W-SRC-0301", SpanOfText(source, offsets, start, j));
    }
  }

  return result;
}

LiteralScanResult ScanFloatLiteral(const core::SourceFile& source,
                                   std::size_t start) {
  SPEC_RULE("Lex-Float");
  SPEC_RULE("Lex-Numeric-Err");
  LiteralScanResult result;
  result.ok = false;
  result.next = start;

  const auto& scalars = source.scalars;
  const std::size_t n = scalars.size();
  if (start >= n || !IsDecDigit(scalars[start])) {
    return result;
  }

  const auto offsets = core::Utf8Offsets(scalars);
  DigitScanResult int_digits = ScanDigits(scalars, start, IsDecDigit);
  if (!int_digits.ok) {
    return result;
  }
  if (int_digits.malformed) {
    EmitDiag(result.diags, "E-SRC-0304",
             SpanOfText(source, offsets, start, int_digits.next));
    result.next = int_digits.next;
    return result;
  }

  std::size_t p = int_digits.next;
  if (p >= n || scalars[p] != '.') {
    return result;
  }
  if (p + 1 < n && scalars[p + 1] == '.') {
    return result;
  }
  ++p;

  if (p < n && scalars[p] == '_') {
    std::size_t q = p;
    while (q < n && scalars[q] == '_') {
      ++q;
    }
    EmitDiag(result.diags, "E-SRC-0304", SpanOfText(source, offsets, start, q));
    result.next = q;
    return result;
  }

  if (p < n && IsDecDigit(scalars[p])) {
    DigitScanResult frac_digits = ScanDigits(scalars, p, IsDecDigit);
    if (frac_digits.malformed) {
      EmitDiag(result.diags, "E-SRC-0304",
               SpanOfText(source, offsets, start, frac_digits.next));
      result.next = frac_digits.next;
      return result;
    }
    p = frac_digits.next;
  }

  if (p < n && (scalars[p] == 'e' || scalars[p] == 'E')) {
    std::size_t exp_pos = p + 1;
    if (exp_pos < n && (scalars[exp_pos] == '+' || scalars[exp_pos] == '-')) {
      ++exp_pos;
    }
    if (exp_pos >= n || !IsDecDigit(scalars[exp_pos])) {
      EmitDiag(result.diags, "E-SRC-0304",
               SpanOfText(source, offsets, start, exp_pos));
      result.next = exp_pos;
      return result;
    }
    DigitScanResult exp_digits = ScanDigits(scalars, exp_pos, IsDecDigit);
    if (exp_digits.malformed) {
      EmitDiag(result.diags, "E-SRC-0304",
               SpanOfText(source, offsets, start, exp_digits.next));
      result.next = exp_digits.next;
      return result;
    }
    p = exp_digits.next;
  }

  const std::size_t suffix_len = MatchFloatSuffix(scalars, p);
  const std::size_t j = p + suffix_len;

  result.ok = true;
  result.next = j;

  const std::string lexeme = LexemeSlice(source, offsets, start, j);
  if (!NumericUnderscoreOk(lexeme)) {
    EmitDiag(result.diags, "E-SRC-0304", SpanOfText(source, offsets, start, j));
  }

  return result;
}

LiteralScanResult ScanStringLiteral(const core::SourceFile& source,
                                    std::size_t start) {
  SPEC_RULE("Lex-String");
  SPEC_RULE("Lex-String-Unterminated");
  SPEC_RULE("Lex-String-BadEscape");
  LiteralScanResult result;
  result.ok = false;
  result.next = start;

  const auto& scalars = source.scalars;
  const std::size_t n = scalars.size();
  if (start >= n || scalars[start] != '"') {
    return result;
  }

  const auto offsets = core::Utf8Offsets(scalars);
  const TerminatorResult term = FindTerminator(scalars, start, '"');
  if (!term.closed) {
    EmitDiag(result.diags, "E-SRC-0301",
             SpanOfText(source, offsets, start, start + 1));
    result.next = term.index;
    return result;
  }

  const std::size_t j = term.index + 1;
  result.ok = true;
  result.next = j;
  result.range = ScalarRange{start, j};

  const auto bad = FirstBadEscape(scalars, start, term.index);
  if (bad.has_value()) {
    EmitDiag(result.diags, "E-SRC-0302",
             SpanOfText(source, offsets, *bad, *bad + 1));
  }

  return result;
}

LiteralScanResult ScanCharLiteral(const core::SourceFile& source,
                                  std::size_t start) {
  SPEC_RULE("Lex-Char");
  SPEC_RULE("Lex-Char-Unterminated");
  SPEC_RULE("Lex-Char-BadEscape");
  SPEC_RULE("Lex-Char-Invalid");
  LiteralScanResult result;
  result.ok = false;
  result.next = start;

  const auto& scalars = source.scalars;
  const std::size_t n = scalars.size();
  if (start >= n || scalars[start] != '\'') {
    return result;
  }

  const auto offsets = core::Utf8Offsets(scalars);
  const TerminatorResult term = FindTerminator(scalars, start, '\'');
  if (!term.closed) {
    EmitDiag(result.diags, "E-SRC-0303",
             SpanOfText(source, offsets, start, start + 1));
    result.next = term.index;
    return result;
  }

  const std::size_t j = term.index + 1;
  result.ok = true;
  result.next = j;
  result.range = ScalarRange{start, j};

  const auto bad = FirstBadEscape(scalars, start, term.index);
  if (bad.has_value()) {
    EmitDiag(result.diags, "E-SRC-0302",
             SpanOfText(source, offsets, *bad, *bad + 1));
  }

  const std::size_t count = CharScalarCount(scalars, start, term.index);
  if (count != 1) {
    EmitDiag(result.diags, "E-SRC-0303",
             SpanOfText(source, offsets, start, start + 1));
  }

  return result;
}

}  // namespace cursive0::syntax
