#include "cursive0/semantics/match.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/int128.h"
#include "cursive0/sema/scopes.h"
#include "cursive0/sema/type_equiv.h"
#include "cursive0/sema/types.h"

namespace cursive0::semantics {

namespace {

static constexpr std::array<std::string_view, 12> kIntSuffixes = {
    "i128", "u128", "isize", "usize", "i64", "u64",
    "i32",  "u32",  "i16",  "u16",  "i8",  "u8"};

static constexpr std::array<std::string_view, 3> kFloatSuffixes = {
    "f16", "f32", "f64"};

static constexpr std::array<std::string_view, 12> kIntTypes = {
    "i8", "i16", "i32", "i64", "i128", "isize",
    "u8", "u16", "u32", "u64", "u128", "usize"};

static constexpr std::array<std::string_view, 3> kFloatTypes = {
    "f16", "f32", "f64"};

bool EndsWith(std::string_view value, std::string_view suffix) {
  if (suffix.size() > value.size()) {
    return false;
  }
  return value.substr(value.size() - suffix.size()) == suffix;
}

std::optional<std::string_view> MatchSuffix(
    std::string_view lexeme,
    std::span<const std::string_view> suffixes) {
  for (const auto suffix : suffixes) {
    if (EndsWith(lexeme, suffix)) {
      const std::size_t core_len = lexeme.size() - suffix.size();
      if (core_len == 0) {
        continue;
      }
      return suffix;
    }
  }
  return std::nullopt;
}

bool IsIntTypeName(std::string_view name) {
  for (const auto& t : kIntTypes) {
    if (name == t) {
      return true;
    }
  }
  return false;
}

bool IsFloatTypeName(std::string_view name) {
  for (const auto& t : kFloatTypes) {
    if (name == t) {
      return true;
    }
  }
  return false;
}

bool IsUnsignedIntType(std::string_view name) {
  return name == "u8" || name == "u16" || name == "u32" ||
         name == "u64" || name == "u128" || name == "usize";
}

bool IsSignedIntType(std::string_view name) {
  return name == "i8" || name == "i16" || name == "i32" ||
         name == "i64" || name == "i128" || name == "isize";
}

std::optional<std::string_view> IntSuffix(const syntax::Token& lit) {
  if (lit.kind != syntax::TokenKind::IntLiteral) {
    return std::nullopt;
  }
  return MatchSuffix(lit.lexeme, kIntSuffixes);
}

std::optional<std::string_view> FloatSuffix(const syntax::Token& lit) {
  if (lit.kind != syntax::TokenKind::FloatLiteral) {
    return std::nullopt;
  }
  return MatchSuffix(lit.lexeme, kFloatSuffixes);
}

std::string_view StripIntSuffix(std::string_view lexeme) {
  for (const auto suffix : kIntSuffixes) {
    if (EndsWith(lexeme, suffix)) {
      const std::size_t core_len = lexeme.size() - suffix.size();
      if (core_len == 0) {
        continue;
      }
      return lexeme.substr(0, core_len);
    }
  }
  return lexeme;
}

std::string_view StripFloatSuffix(std::string_view lexeme) {
  for (const auto suffix : kFloatSuffixes) {
    if (EndsWith(lexeme, suffix)) {
      const std::size_t core_len = lexeme.size() - suffix.size();
      if (core_len == 0) {
        continue;
      }
      return lexeme.substr(0, core_len);
    }
  }
  return lexeme;
}

bool DigitValue(char c, unsigned base, unsigned* out) {
  if (c >= '0' && c <= '9') {
    unsigned digit = static_cast<unsigned>(c - '0');
    if (digit < base) {
      *out = digit;
      return true;
    }
    return false;
  }
  if (base <= 10) {
    return false;
  }
  if (c >= 'a' && c <= 'f') {
    unsigned digit = static_cast<unsigned>(10 + (c - 'a'));
    if (digit < base) {
      *out = digit;
      return true;
    }
    return false;
  }
  if (c >= 'A' && c <= 'F') {
    unsigned digit = static_cast<unsigned>(10 + (c - 'A'));
    if (digit < base) {
      *out = digit;
      return true;
    }
    return false;
  }
  return false;
}

bool ParseIntCore(std::string_view core, core::UInt128& value_out) {
  unsigned base = 10;
  std::string_view digits = core;
  if (core.size() >= 2 && core[0] == '0') {
    const char prefix = core[1];
    if (prefix == 'x' || prefix == 'X') {
      base = 16;
      digits = core.substr(2);
    } else if (prefix == 'o' || prefix == 'O') {
      base = 8;
      digits = core.substr(2);
    } else if (prefix == 'b' || prefix == 'B') {
      base = 2;
      digits = core.substr(2);
    }
  }
  if (digits.empty()) {
    return false;
  }
  core::UInt128 value = core::UInt128FromU64(0);
  const core::UInt128 max = core::UInt128Max();
  const core::UInt128 base128 = core::UInt128FromU64(base);
  bool saw_digit = false;
  for (char c : digits) {
    if (c == '_') {
      continue;
    }
    unsigned digit = 0;
    if (!DigitValue(c, base, &digit)) {
      return false;
    }
    saw_digit = true;
    const core::UInt128 digit128 = core::UInt128FromU64(digit);
    const core::UInt128 max_minus_digit = core::UInt128Sub(max, digit128);
    const core::UInt128 threshold = core::UInt128Div(max_minus_digit, base128);
    if (core::UInt128Greater(value, threshold)) {
      return false;
    }
    value = core::UInt128Add(core::UInt128Mul(value, base128), digit128);
  }
  if (!saw_digit) {
    return false;
  }
  value_out = value;
  return true;
}

std::optional<core::UInt128> ParseIntLiteralValue(std::string_view lexeme) {
  if (lexeme.empty() || lexeme[0] == '-') {
    return std::nullopt;
  }
  const std::string_view core = StripIntSuffix(lexeme);
  if (core.empty()) {
    return std::nullopt;
  }
  core::UInt128 value = core::UInt128FromU64(0);
  if (!ParseIntCore(core, value)) {
    return std::nullopt;
  }
  return value;
}

std::optional<double> ParseFloatLiteralValue(std::string_view lexeme) {
  if (lexeme.empty() || lexeme[0] == '-') {
    return std::nullopt;
  }
  const std::string_view core = StripFloatSuffix(lexeme);
  if (core.empty()) {
    return std::nullopt;
  }
  std::string cleaned;
  cleaned.reserve(core.size());
  for (char c : core) {
    if (c != '_') {
      cleaned.push_back(c);
    }
  }
  if (cleaned.empty()) {
    return std::nullopt;
  }
  char* end = nullptr;
  const double value = std::strtod(cleaned.c_str(), &end);
  if (!end || *end != '\0') {
    return std::nullopt;
  }
  return value;
}

int CompareMagnitude(core::UInt128 lhs, core::UInt128 rhs) {
  if (core::UInt128Greater(lhs, rhs)) {
    return 1;
  }
  if (core::UInt128Greater(rhs, lhs)) {
    return -1;
  }
  return 0;
}

std::optional<int> CompareIntVal(const IntVal& lhs, const IntVal& rhs) {
  if (lhs.type != rhs.type) {
    return std::nullopt;
  }
  if (IsUnsignedIntType(lhs.type)) {
    if (lhs.negative || rhs.negative) {
      return std::nullopt;
    }
    return CompareMagnitude(lhs.magnitude, rhs.magnitude);
  }
  if (IsSignedIntType(lhs.type)) {
    if (lhs.negative != rhs.negative) {
      return lhs.negative ? -1 : 1;
    }
    if (lhs.negative) {
      return CompareMagnitude(rhs.magnitude, lhs.magnitude);
    }
    return CompareMagnitude(lhs.magnitude, rhs.magnitude);
  }
  return std::nullopt;
}

std::optional<int> CompareFloatVal(const FloatVal& lhs, const FloatVal& rhs) {
  if (lhs.type != rhs.type) {
    return std::nullopt;
  }
  if (std::isnan(lhs.value) || std::isnan(rhs.value)) {
    return std::nullopt;
  }
  if (lhs.value < rhs.value) {
    return -1;
  }
  if (lhs.value > rhs.value) {
    return 1;
  }
  return 0;
}

std::optional<int> CompareCharVal(const CharVal& lhs, const CharVal& rhs) {
  if (lhs.value < rhs.value) {
    return -1;
  }
  if (lhs.value > rhs.value) {
    return 1;
  }
  return 0;
}

std::optional<int> CompareValues(const Value& lhs, const Value& rhs) {
  if (const auto* li = std::get_if<IntVal>(&lhs.node)) {
    if (const auto* ri = std::get_if<IntVal>(&rhs.node)) {
      return CompareIntVal(*li, *ri);
    }
  }
  if (const auto* lf = std::get_if<FloatVal>(&lhs.node)) {
    if (const auto* rf = std::get_if<FloatVal>(&rhs.node)) {
      return CompareFloatVal(*lf, *rf);
    }
  }
  if (const auto* lc = std::get_if<CharVal>(&lhs.node)) {
    if (const auto* rc = std::get_if<CharVal>(&rhs.node)) {
      return CompareCharVal(*lc, *rc);
    }
  }
  return std::nullopt;
}

bool IsUnicodeScalar(std::uint32_t value) {
  if (value > 0x10FFFFu) {
    return false;
  }
  if (value >= 0xD800u && value <= 0xDFFFu) {
    return false;
  }
  return true;
}

std::optional<std::pair<std::uint32_t, std::size_t>> DecodeUtf8One(
    const unsigned char* data,
    std::size_t size,
    std::size_t offset) {
  if (offset >= size) {
    return std::nullopt;
  }
  const std::uint8_t b0 = data[offset];
  if (b0 < 0x80u) {
    return std::make_pair(static_cast<std::uint32_t>(b0), 1u);
  }
  if ((b0 & 0xE0u) == 0xC0u) {
    if (offset + 1 >= size) {
      return std::nullopt;
    }
    const std::uint8_t b1 = data[offset + 1];
    if ((b1 & 0xC0u) != 0x80u) {
      return std::nullopt;
    }
    const std::uint32_t cp =
        ((static_cast<std::uint32_t>(b0) & 0x1Fu) << 6) |
        (static_cast<std::uint32_t>(b1) & 0x3Fu);
    if (cp < 0x80u) {
      return std::nullopt;
    }
    if (!IsUnicodeScalar(cp)) {
      return std::nullopt;
    }
    return std::make_pair(cp, 2u);
  }
  if ((b0 & 0xF0u) == 0xE0u) {
    if (offset + 2 >= size) {
      return std::nullopt;
    }
    const std::uint8_t b1 = data[offset + 1];
    const std::uint8_t b2 = data[offset + 2];
    if ((b1 & 0xC0u) != 0x80u || (b2 & 0xC0u) != 0x80u) {
      return std::nullopt;
    }
    const std::uint32_t cp =
        ((static_cast<std::uint32_t>(b0) & 0x0Fu) << 12) |
        ((static_cast<std::uint32_t>(b1) & 0x3Fu) << 6) |
        (static_cast<std::uint32_t>(b2) & 0x3Fu);
    if (cp < 0x800u) {
      return std::nullopt;
    }
    if (!IsUnicodeScalar(cp)) {
      return std::nullopt;
    }
    return std::make_pair(cp, 3u);
  }
  if ((b0 & 0xF8u) == 0xF0u) {
    if (offset + 3 >= size) {
      return std::nullopt;
    }
    const std::uint8_t b1 = data[offset + 1];
    const std::uint8_t b2 = data[offset + 2];
    const std::uint8_t b3 = data[offset + 3];
    if ((b1 & 0xC0u) != 0x80u || (b2 & 0xC0u) != 0x80u ||
        (b3 & 0xC0u) != 0x80u) {
      return std::nullopt;
    }
    const std::uint32_t cp =
        ((static_cast<std::uint32_t>(b0) & 0x07u) << 18) |
        ((static_cast<std::uint32_t>(b1) & 0x3Fu) << 12) |
        ((static_cast<std::uint32_t>(b2) & 0x3Fu) << 6) |
        (static_cast<std::uint32_t>(b3) & 0x3Fu);
    if (cp < 0x10000u || cp > 0x10FFFFu) {
      return std::nullopt;
    }
    if (!IsUnicodeScalar(cp)) {
      return std::nullopt;
    }
    return std::make_pair(cp, 4u);
  }
  return std::nullopt;
}

std::vector<std::uint8_t> EncodeUtf8(std::uint32_t value) {
  std::vector<std::uint8_t> out;
  if (value <= 0x7Fu) {
    out.push_back(static_cast<std::uint8_t>(value));
  } else if (value <= 0x7FFu) {
    out.push_back(static_cast<std::uint8_t>(0xC0u | (value >> 6)));
    out.push_back(static_cast<std::uint8_t>(0x80u | (value & 0x3Fu)));
  } else if (value <= 0xFFFFu) {
    out.push_back(static_cast<std::uint8_t>(0xE0u | (value >> 12)));
    out.push_back(static_cast<std::uint8_t>(0x80u | ((value >> 6) & 0x3Fu)));
    out.push_back(static_cast<std::uint8_t>(0x80u | (value & 0x3Fu)));
  } else {
    out.push_back(static_cast<std::uint8_t>(0xF0u | (value >> 18)));
    out.push_back(static_cast<std::uint8_t>(0x80u | ((value >> 12) & 0x3Fu)));
    out.push_back(static_cast<std::uint8_t>(0x80u | ((value >> 6) & 0x3Fu)));
    out.push_back(static_cast<std::uint8_t>(0x80u | (value & 0x3Fu)));
  }
  return out;
}

std::optional<std::uint32_t> ParseHexScalar(std::string_view digits) {
  if (digits.empty()) {
    return std::nullopt;
  }
  std::uint32_t value = 0;
  for (char c : digits) {
    unsigned digit = 0;
    if (!DigitValue(c, 16, &digit)) {
      return std::nullopt;
    }
    if (value > (0x10FFFFu - digit) / 16u) {
      return std::nullopt;
    }
    value = static_cast<std::uint32_t>(value * 16u + digit);
  }
  if (!IsUnicodeScalar(value)) {
    return std::nullopt;
  }
  return value;
}

std::optional<std::vector<std::uint8_t>> DecodeStringLiteralBytes(
    std::string_view lexeme) {
  if (lexeme.size() < 2 || lexeme.front() != '"' || lexeme.back() != '"') {
    return std::nullopt;
  }
  const std::string_view inner = lexeme.substr(1, lexeme.size() - 2);
  std::vector<std::uint8_t> bytes;
  const auto* data = reinterpret_cast<const unsigned char*>(inner.data());
  std::size_t i = 0;
  while (i < inner.size()) {
    const unsigned char c = data[i];
    if (c == static_cast<unsigned char>('\\')) {
      if (i + 1 >= inner.size()) {
        return std::nullopt;
      }
      const char esc = inner[i + 1];
      switch (esc) {
        case '\\':
          bytes.push_back(0x5Cu);
          i += 2;
          break;
        case '"':
          bytes.push_back(0x22u);
          i += 2;
          break;
        case '\'':
          bytes.push_back(0x27u);
          i += 2;
          break;
        case 'n':
          bytes.push_back(0x0Au);
          i += 2;
          break;
        case 'r':
          bytes.push_back(0x0Du);
          i += 2;
          break;
        case 't':
          bytes.push_back(0x09u);
          i += 2;
          break;
        case '0':
          bytes.push_back(0x00u);
          i += 2;
          break;
        case 'x': {
          if (i + 3 >= inner.size()) {
            return std::nullopt;
          }
          unsigned d1 = 0;
          unsigned d2 = 0;
          if (!DigitValue(inner[i + 2], 16, &d1) ||
              !DigitValue(inner[i + 3], 16, &d2)) {
            return std::nullopt;
          }
          bytes.push_back(static_cast<std::uint8_t>((d1 << 4) | d2));
          i += 4;
          break;
        }
        case 'u': {
          if (i + 2 >= inner.size() || inner[i + 2] != '{') {
            return std::nullopt;
          }
          const std::size_t start = i + 3;
          const std::size_t close = inner.find('}', start);
          if (close == std::string_view::npos) {
            return std::nullopt;
          }
          const auto digits = inner.substr(start, close - start);
          const auto value = ParseHexScalar(digits);
          if (!value.has_value()) {
            return std::nullopt;
          }
          const auto utf8 = EncodeUtf8(*value);
          bytes.insert(bytes.end(), utf8.begin(), utf8.end());
          i = close + 1;
          break;
        }
        default:
          return std::nullopt;
      }
      continue;
    }
    const auto decoded = DecodeUtf8One(data, inner.size(), i);
    if (!decoded.has_value()) {
      return std::nullopt;
    }
    const std::size_t len = decoded->second;
    bytes.insert(bytes.end(), inner.begin() + i, inner.begin() + i + len);
    i += len;
  }
  return bytes;
}

std::optional<std::uint32_t> DecodeCharLiteral(std::string_view lexeme) {
  if (lexeme.size() < 2 || lexeme.front() != '\'' || lexeme.back() != '\'') {
    return std::nullopt;
  }
  const std::string_view inner = lexeme.substr(1, lexeme.size() - 2);
  std::vector<std::uint8_t> bytes;
  const auto* data = reinterpret_cast<const unsigned char*>(inner.data());
  std::size_t i = 0;
  while (i < inner.size()) {
    const unsigned char c = data[i];
    if (c == static_cast<unsigned char>('\\')) {
      if (i + 1 >= inner.size()) {
        return std::nullopt;
      }
      const char esc = inner[i + 1];
      switch (esc) {
        case '\\':
          bytes.push_back(0x5Cu);
          i += 2;
          break;
        case '"':
          bytes.push_back(0x22u);
          i += 2;
          break;
        case '\'':
          bytes.push_back(0x27u);
          i += 2;
          break;
        case 'n':
          bytes.push_back(0x0Au);
          i += 2;
          break;
        case 'r':
          bytes.push_back(0x0Du);
          i += 2;
          break;
        case 't':
          bytes.push_back(0x09u);
          i += 2;
          break;
        case '0':
          bytes.push_back(0x00u);
          i += 2;
          break;
        case 'x': {
          if (i + 3 >= inner.size()) {
            return std::nullopt;
          }
          unsigned d1 = 0;
          unsigned d2 = 0;
          if (!DigitValue(inner[i + 2], 16, &d1) ||
              !DigitValue(inner[i + 3], 16, &d2)) {
            return std::nullopt;
          }
          bytes.push_back(static_cast<std::uint8_t>((d1 << 4) | d2));
          i += 4;
          break;
        }
        case 'u': {
          if (i + 2 >= inner.size() || inner[i + 2] != '{') {
            return std::nullopt;
          }
          const std::size_t start = i + 3;
          const std::size_t close = inner.find('}', start);
          if (close == std::string_view::npos) {
            return std::nullopt;
          }
          const auto digits = inner.substr(start, close - start);
          const auto value = ParseHexScalar(digits);
          if (!value.has_value()) {
            return std::nullopt;
          }
          const auto utf8 = EncodeUtf8(*value);
          bytes.insert(bytes.end(), utf8.begin(), utf8.end());
          i = close + 1;
          break;
        }
        default:
          return std::nullopt;
      }
      continue;
    }
    const auto decoded = DecodeUtf8One(data, inner.size(), i);
    if (!decoded.has_value()) {
      return std::nullopt;
    }
    const std::size_t len = decoded->second;
    bytes.insert(bytes.end(), inner.begin() + i, inner.begin() + i + len);
    i += len;
  }
  if (bytes.empty()) {
    return std::nullopt;
  }
  const auto decoded = DecodeUtf8One(bytes.data(), bytes.size(), 0);
  if (!decoded.has_value() || decoded->second != bytes.size()) {
    return std::nullopt;
  }
  return decoded->first;
}

std::optional<sema::TypeRef> PatType(const syntax::LiteralPattern& lit) {
  const auto& tok = lit.literal;
  switch (tok.kind) {
    case syntax::TokenKind::IntLiteral: {
      if (const auto suffix = IntSuffix(tok)) {
        return sema::MakeTypePrim(std::string(*suffix));
      }
      return sema::MakeTypePrim("i32");
    }
    case syntax::TokenKind::FloatLiteral: {
      if (const auto suffix = FloatSuffix(tok)) {
        return sema::MakeTypePrim(std::string(*suffix));
      }
      return sema::MakeTypePrim("f64");
    }
    case syntax::TokenKind::BoolLiteral:
      return sema::MakeTypePrim("bool");
    case syntax::TokenKind::CharLiteral:
      return sema::MakeTypePrim("char");
    case syntax::TokenKind::StringLiteral:
      return sema::MakeTypeString(sema::StringState::View);
    case syntax::TokenKind::NullLiteral:
    case syntax::TokenKind::Identifier:
    case syntax::TokenKind::Keyword:
    case syntax::TokenKind::Operator:
    case syntax::TokenKind::Punctuator:
    case syntax::TokenKind::Newline:
    case syntax::TokenKind::Unknown:
      break;
  }
  return std::nullopt;
}

std::optional<Value> LiteralValue(const syntax::Token& lit,
                                  const sema::TypeRef& type) {
  if (!type) {
    return std::nullopt;
  }
  if (const auto* prim = std::get_if<sema::TypePrim>(&type->node)) {
    if (prim->name == "bool" && lit.kind == syntax::TokenKind::BoolLiteral) {
      BoolVal v;
      v.value = lit.lexeme == "true";
      return Value{v};
    }
    if (prim->name == "char" && lit.kind == syntax::TokenKind::CharLiteral) {
      const auto decoded = DecodeCharLiteral(lit.lexeme);
      if (!decoded.has_value()) {
        return std::nullopt;
      }
      CharVal v;
      v.value = *decoded;
      return Value{v};
    }
    if (IsIntTypeName(prim->name) && lit.kind == syntax::TokenKind::IntLiteral) {
      const auto value = ParseIntLiteralValue(lit.lexeme);
      if (!value.has_value()) {
        return std::nullopt;
      }
      IntVal v;
      v.type = prim->name;
      v.negative = false;
      v.magnitude = *value;
      return Value{v};
    }
    if (IsFloatTypeName(prim->name) && lit.kind == syntax::TokenKind::FloatLiteral) {
      const auto value = ParseFloatLiteralValue(lit.lexeme);
      if (!value.has_value()) {
        return std::nullopt;
      }
      FloatVal v;
      v.type = prim->name;
      v.value = *value;
      return Value{v};
    }
  }
  if (const auto* str = std::get_if<sema::TypeString>(&type->node)) {
    if (lit.kind == syntax::TokenKind::StringLiteral &&
        str->state.has_value() &&
        *str->state == sema::StringState::View) {
      const auto bytes = DecodeStringLiteralBytes(lit.lexeme);
      if (!bytes.has_value()) {
        return std::nullopt;
      }
      StringVal v;
      v.state = sema::StringState::View;
      v.bytes = *bytes;
      return Value{v};
    }
  }
  if (const auto* raw = std::get_if<sema::TypeRawPtr>(&type->node)) {
    if (lit.kind == syntax::TokenKind::NullLiteral) {
      RawPtrVal v;
      v.qual = raw->qual;
      v.addr = 0;
      return Value{v};
    }
  }
  return std::nullopt;
}

std::optional<Value> ConstPat(const syntax::Pattern& pattern) {
  const auto* lit = std::get_if<syntax::LiteralPattern>(&pattern.node);
  if (!lit) {
    return std::nullopt;
  }
  const auto type = PatType(*lit);
  if (!type.has_value()) {
    return std::nullopt;
  }
  return LiteralValue(lit->literal, *type);
}

std::optional<BindEnv> MergeBindings(const BindEnv& lhs,
                                     const BindEnv& rhs) {
  BindEnv out = lhs;
  for (const auto& [name, value] : rhs) {
    if (out.find(name) != out.end()) {
      return std::nullopt;
    }
    out.emplace(name, value);
  }
  return out;
}

std::optional<Value> FieldValueFromList(
    const std::vector<std::pair<std::string, Value>>& fields,
    std::string_view name) {
  for (const auto& [field_name, value] : fields) {
    if (sema::IdEq(field_name, name)) {
      return value;
    }
  }
  return std::nullopt;
}

bool PathEq(const sema::TypePath& lhs, const syntax::TypePath& rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }
  for (std::size_t i = 0; i < lhs.size(); ++i) {
    if (!sema::IdEq(lhs[i], rhs[i])) {
      return false;
    }
  }
  return true;
}

std::optional<sema::TypePath> EnumPathOf(const sema::TypePath& path) {
  if (path.empty()) {
    return std::nullopt;
  }
  sema::TypePath out(path.begin(), path.end() - 1);
  return out;
}

std::optional<std::string> VariantNameOf(const sema::TypePath& path) {
  if (path.empty()) {
    return std::nullopt;
  }
  return path.back();
}

std::optional<sema::Permission> LowerPermission(syntax::TypePerm perm) {
  switch (perm) {
    case syntax::TypePerm::Const:
      return sema::Permission::Const;
    case syntax::TypePerm::Unique:
      return sema::Permission::Unique;
    case syntax::TypePerm::Shared:
      return sema::Permission::Shared;
  }
  return std::nullopt;
}

std::optional<sema::RawPtrQual> LowerRawPtrQual(syntax::RawPtrQual qual) {
  switch (qual) {
    case syntax::RawPtrQual::Imm:
      return sema::RawPtrQual::Imm;
    case syntax::RawPtrQual::Mut:
      return sema::RawPtrQual::Mut;
  }
  return std::nullopt;
}

std::optional<sema::StringState> LowerStringState(
    const std::optional<syntax::StringState>& state) {
  if (!state.has_value()) {
    return std::nullopt;
  }
  switch (*state) {
    case syntax::StringState::Managed:
      return sema::StringState::Managed;
    case syntax::StringState::View:
      return sema::StringState::View;
  }
  return std::nullopt;
}

std::optional<sema::BytesState> LowerBytesState(
    const std::optional<syntax::BytesState>& state) {
  if (!state.has_value()) {
    return std::nullopt;
  }
  switch (*state) {
    case syntax::BytesState::Managed:
      return sema::BytesState::Managed;
    case syntax::BytesState::View:
      return sema::BytesState::View;
  }
  return std::nullopt;
}

std::optional<sema::PtrState> LowerPtrState(
    const std::optional<syntax::PtrState>& state) {
  if (!state.has_value()) {
    return std::nullopt;
  }
  switch (*state) {
    case syntax::PtrState::Valid:
      return sema::PtrState::Valid;
    case syntax::PtrState::Null:
      return sema::PtrState::Null;
    case syntax::PtrState::Expired:
      return sema::PtrState::Expired;
  }
  return std::nullopt;
}

std::optional<sema::TypeRef> LowerSyntaxType(const SemanticsContext& ctx,
                                             const std::shared_ptr<syntax::Type>& type) {
  if (!type) {
    return std::nullopt;
  }
  return std::visit(
      [&](const auto& node) -> std::optional<sema::TypeRef> {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::TypePrim>) {
          return sema::MakeTypePrim(node.name);
        } else if constexpr (std::is_same_v<T, syntax::TypePermType>) {
          const auto perm = LowerPermission(node.perm);
          const auto base = LowerSyntaxType(ctx, node.base);
          if (!perm.has_value() || !base.has_value()) {
            return std::nullopt;
          }
          return sema::MakeTypePerm(*perm, *base);
        } else if constexpr (std::is_same_v<T, syntax::TypeUnion>) {
          std::vector<sema::TypeRef> members;
          members.reserve(node.types.size());
          for (const auto& member : node.types) {
            const auto lowered = LowerSyntaxType(ctx, member);
            if (!lowered.has_value()) {
              return std::nullopt;
            }
            members.push_back(*lowered);
          }
          return sema::MakeTypeUnion(std::move(members));
        } else if constexpr (std::is_same_v<T, syntax::TypeFunc>) {
          std::vector<sema::TypeFuncParam> params;
          params.reserve(node.params.size());
          for (const auto& param : node.params) {
            sema::TypeFuncParam out_param;
            if (param.mode.has_value()) {
              out_param.mode = sema::ParamMode::Move;
            }
            const auto lowered = LowerSyntaxType(ctx, param.type);
            if (!lowered.has_value()) {
              return std::nullopt;
            }
            out_param.type = *lowered;
            params.push_back(std::move(out_param));
          }
          const auto ret = LowerSyntaxType(ctx, node.ret);
          if (!ret.has_value()) {
            return std::nullopt;
          }
          return sema::MakeTypeFunc(std::move(params), *ret);
        } else if constexpr (std::is_same_v<T, syntax::TypeTuple>) {
          std::vector<sema::TypeRef> elements;
          elements.reserve(node.elements.size());
          for (const auto& elem : node.elements) {
            const auto lowered = LowerSyntaxType(ctx, elem);
            if (!lowered.has_value()) {
              return std::nullopt;
            }
            elements.push_back(*lowered);
          }
          return sema::MakeTypeTuple(std::move(elements));
        } else if constexpr (std::is_same_v<T, syntax::TypeArray>) {
          const auto elem = LowerSyntaxType(ctx, node.element);
          if (!elem.has_value() || !ctx.sema) {
            return std::nullopt;
          }
          const auto len = sema::ConstLen(*ctx.sema, node.length);
          if (!len.ok || !len.value.has_value()) {
            return std::nullopt;
          }
          return sema::MakeTypeArray(*elem, *len.value);
        } else if constexpr (std::is_same_v<T, syntax::TypeSlice>) {
          const auto elem = LowerSyntaxType(ctx, node.element);
          if (!elem.has_value()) {
            return std::nullopt;
          }
          return sema::MakeTypeSlice(*elem);
        } else if constexpr (std::is_same_v<T, syntax::TypePtr>) {
          const auto elem = LowerSyntaxType(ctx, node.element);
          if (!elem.has_value()) {
            return std::nullopt;
          }
          return sema::MakeTypePtr(*elem, LowerPtrState(node.state));
        } else if constexpr (std::is_same_v<T, syntax::TypeRawPtr>) {
          const auto elem = LowerSyntaxType(ctx, node.element);
          const auto qual = LowerRawPtrQual(node.qual);
          if (!elem.has_value() || !qual.has_value()) {
            return std::nullopt;
          }
          return sema::MakeTypeRawPtr(*qual, *elem);
        } else if constexpr (std::is_same_v<T, syntax::TypeString>) {
          return sema::MakeTypeString(LowerStringState(node.state));
        } else if constexpr (std::is_same_v<T, syntax::TypeBytes>) {
          return sema::MakeTypeBytes(LowerBytesState(node.state));
        } else if constexpr (std::is_same_v<T, syntax::TypeDynamic>) {
          sema::TypePath path(node.path.begin(), node.path.end());
          return sema::MakeTypeDynamic(std::move(path));
        } else if constexpr (std::is_same_v<T, syntax::TypeModalState>) {
          sema::TypePath path(node.path.begin(), node.path.end());
          return sema::MakeTypeModalState(std::move(path), node.state);
        } else if constexpr (std::is_same_v<T, syntax::TypePathType>) {
          sema::TypePath path(node.path.begin(), node.path.end());
          return sema::MakeTypePath(std::move(path));
        }
        return std::nullopt;
      },
      type->node);
}

}  // namespace

std::optional<BindEnv> MatchModal(const SemanticsContext& ctx,
                                  const syntax::ModalPattern& pattern,
                                  const Value& value) {
  (void)ctx;
  if (!pattern.fields_opt.has_value()) {
    SPEC_RULE("Match-Modal-Empty");
    return BindEnv{};
  }
  auto result = MatchRecord(ctx, pattern.fields_opt->fields, value);
  if (!result.has_value()) {
    return std::nullopt;
  }
  SPEC_RULE("Match-Modal-Record");
  return result;
}

std::optional<BindEnv> MatchRecord(const SemanticsContext& ctx,
                                   const std::vector<syntax::FieldPattern>& fields,
                                   const Value& value) {
  (void)ctx;
  if (fields.empty()) {
    SPEC_RULE("MatchRecord-Empty");
    return BindEnv{};
  }

  const auto& head = fields.front();
  std::vector<syntax::FieldPattern> tail(fields.begin() + 1, fields.end());

  std::optional<Value> field_value;
  if (const auto* record = std::get_if<RecordVal>(&value.node)) {
    field_value = FieldValueFromList(record->fields, head.name);
  }

  if (!field_value.has_value()) {
    return std::nullopt;
  }

  if (!head.pattern_opt) {
    auto rest = MatchRecord(ctx, tail, value);
    if (!rest.has_value()) {
      return std::nullopt;
    }
    BindEnv binding;
    binding.emplace(head.name, *field_value);
    auto merged = MergeBindings(*rest, binding);
    if (!merged.has_value()) {
      return std::nullopt;
    }
    SPEC_RULE("MatchRecord-Cons-Implicit");
    return merged;
  }

  auto sub = MatchPattern(ctx, *head.pattern_opt, *field_value);
  if (!sub.has_value()) {
    return std::nullopt;
  }
  auto rest = MatchRecord(ctx, tail, value);
  if (!rest.has_value()) {
    return std::nullopt;
  }
  auto merged = MergeBindings(*sub, *rest);
  if (!merged.has_value()) {
    return std::nullopt;
  }
  SPEC_RULE("MatchRecord-Cons-Explicit");
  return merged;
}

std::optional<BindEnv> MatchPattern(const SemanticsContext& ctx,
                                    const syntax::Pattern& pattern,
                                    const Value& value) {
  return std::visit(
      [&](const auto& node) -> std::optional<BindEnv> {
        using T = std::decay_t<decltype(node)>;

        if constexpr (std::is_same_v<T, syntax::WildcardPattern>) {
          SPEC_RULE("Match-Wildcard");
          return BindEnv{};
        } else if constexpr (std::is_same_v<T, syntax::IdentifierPattern>) {
          SPEC_RULE("Match-Ident");
          BindEnv env;
          env.emplace(node.name, value);
          return env;
        } else if constexpr (std::is_same_v<T, syntax::TypedPattern>) {
          const auto lowered = LowerSyntaxType(ctx, node.type);
          if (!lowered.has_value()) {
            return std::nullopt;
          }
          const auto* uni = std::get_if<UnionVal>(&value.node);
          if (!uni || !uni->member || !uni->value) {
            return std::nullopt;
          }
          const auto equiv = sema::TypeEquiv(uni->member, *lowered);
          if (!equiv.ok || !equiv.equiv) {
            return std::nullopt;
          }
          SPEC_RULE("Match-Typed");
          BindEnv env;
          env.emplace(node.name, *uni->value);
          return env;
        } else if constexpr (std::is_same_v<T, syntax::LiteralPattern>) {
          const auto type = PatType(node);
          if (!type.has_value()) {
            return std::nullopt;
          }
          const auto lit_value = LiteralValue(node.literal, *type);
          if (!lit_value.has_value()) {
            return std::nullopt;
          }
          if (!ValueEqual(*lit_value, value)) {
            return std::nullopt;
          }
          SPEC_RULE("Match-Literal");
          return BindEnv{};
        } else if constexpr (std::is_same_v<T, syntax::TuplePattern>) {
          if (node.elements.empty()) {
            if (std::holds_alternative<UnitVal>(value.node)) {
              SPEC_RULE("Match-Tuple");
              return BindEnv{};
            }
            if (const auto* tuple = std::get_if<TupleVal>(&value.node)) {
              if (tuple->elements.empty()) {
                SPEC_RULE("Match-Tuple");
                return BindEnv{};
              }
            }
            return std::nullopt;
          }
          const auto* tuple = std::get_if<TupleVal>(&value.node);
          if (!tuple || tuple->elements.size() != node.elements.size()) {
            return std::nullopt;
          }
          BindEnv env;
          for (std::size_t i = 0; i < node.elements.size(); ++i) {
            const auto sub = MatchPattern(ctx, *node.elements[i], tuple->elements[i]);
            if (!sub.has_value()) {
              return std::nullopt;
            }
            const auto merged = MergeBindings(env, *sub);
            if (!merged.has_value()) {
              return std::nullopt;
            }
            env = *merged;
          }
          SPEC_RULE("Match-Tuple");
          return env;
        } else if constexpr (std::is_same_v<T, syntax::RecordPattern>) {
          const auto res = MatchRecord(ctx, node.fields, value);
          if (!res.has_value()) {
            return std::nullopt;
          }
          SPEC_RULE("Match-Record");
          return res;
        } else if constexpr (std::is_same_v<T, syntax::EnumPattern>) {
          const auto* enum_val = std::get_if<EnumVal>(&value.node);
          if (!enum_val) {
            return std::nullopt;
          }
          const auto enum_path = EnumPathOf(enum_val->path);
          const auto variant_name = VariantNameOf(enum_val->path);
          if (!enum_path.has_value() || !variant_name.has_value()) {
            return std::nullopt;
          }
          if (!PathEq(*enum_path, node.path) ||
              !sema::IdEq(*variant_name, node.name)) {
            return std::nullopt;
          }
          if (!node.payload_opt.has_value()) {
            if (enum_val->payload.has_value()) {
              return std::nullopt;
            }
            SPEC_RULE("Match-Enum-Unit");
            return BindEnv{};
          }
          if (!enum_val->payload.has_value()) {
            return std::nullopt;
          }
          return std::visit(
              [&](const auto& payload_pattern) -> std::optional<BindEnv> {
                using P = std::decay_t<decltype(payload_pattern)>;
                if constexpr (std::is_same_v<P, syntax::TuplePayloadPattern>) {
                  const auto* tuple_payload =
                      std::get_if<EnumPayloadTupleVal>(&*enum_val->payload);
                  if (!tuple_payload ||
                      tuple_payload->elements.size() != payload_pattern.elements.size()) {
                    return std::nullopt;
                  }
                  BindEnv env;
                  for (std::size_t i = 0; i < payload_pattern.elements.size(); ++i) {
                    const auto sub = MatchPattern(ctx,
                                                  *payload_pattern.elements[i],
                                                  tuple_payload->elements[i]);
                    if (!sub.has_value()) {
                      return std::nullopt;
                    }
                    const auto merged = MergeBindings(env, *sub);
                    if (!merged.has_value()) {
                      return std::nullopt;
                    }
                    env = *merged;
                  }
                  SPEC_RULE("Match-Enum-Tuple");
                  return env;
                } else if constexpr (std::is_same_v<P, syntax::RecordPayloadPattern>) {
                  const auto* record_payload =
                      std::get_if<EnumPayloadRecordVal>(&*enum_val->payload);
                  if (!record_payload) {
                    return std::nullopt;
                  }
                  Value record_value;
                  RecordVal record;
                  record.fields = record_payload->fields;
                  record_value.node = std::move(record);
                  const auto res = MatchRecord(ctx, payload_pattern.fields, record_value);
                  if (!res.has_value()) {
                    return std::nullopt;
                  }
                  SPEC_RULE("Match-Enum-Record");
                  return res;
                }
                return std::nullopt;
              },
              *node.payload_opt);
        } else if constexpr (std::is_same_v<T, syntax::ModalPattern>) {
          if (const auto* modal_val = std::get_if<ModalVal>(&value.node)) {
            if (!sema::IdEq(modal_val->state, node.state)) {
              return std::nullopt;
            }
            if (!modal_val->payload) {
              return std::nullopt;
            }
            const auto res = MatchModal(ctx, node, *modal_val->payload);
            if (!res.has_value()) {
              return std::nullopt;
            }
            SPEC_RULE("Match-Modal-General");
            return res;
          }
          if (!node.fields_opt.has_value()) {
            return std::nullopt;
          }
          const auto res = MatchRecord(ctx, node.fields_opt->fields, value);
          if (!res.has_value()) {
            return std::nullopt;
          }
          SPEC_RULE("Match-Modal-State");
          return res;
        } else if constexpr (std::is_same_v<T, syntax::RangePattern>) {
          if (!node.lo || !node.hi) {
            return std::nullopt;
          }
          const auto lo = ConstPat(*node.lo);
          const auto hi = ConstPat(*node.hi);
          if (!lo.has_value() || !hi.has_value()) {
            return std::nullopt;
          }
          const auto lo_cmp = CompareValues(*lo, value);
          if (!lo_cmp.has_value() || *lo_cmp > 0) {
            return std::nullopt;
          }
          const auto hi_cmp = CompareValues(value, *hi);
          if (!hi_cmp.has_value()) {
            return std::nullopt;
          }
          if (node.kind == syntax::RangeKind::Inclusive) {
            if (*hi_cmp <= 0) {
              SPEC_RULE("Match-Range");
              return BindEnv{};
            }
            return std::nullopt;
          }
          if (node.kind == syntax::RangeKind::Exclusive) {
            if (*hi_cmp < 0) {
              SPEC_RULE("Match-Range");
              return BindEnv{};
            }
            return std::nullopt;
          }
          return std::nullopt;
        }
        return std::nullopt;
      },
      pattern.node);
}

}  // namespace cursive0::semantics
