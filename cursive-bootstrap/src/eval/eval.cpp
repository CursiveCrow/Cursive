
#include "cursive0/semantics/eval.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <initializer_list>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/int128.h"
#include "cursive0/sema/cap_system.h"
#include "cursive0/sema/cap_heap.h"
#include "cursive0/sema/cap_filesystem.h"
#include "cursive0/sema/classes.h"
#include "cursive0/sema/collect_toplevel.h"
#include "cursive0/sema/modal.h"
#include "cursive0/sema/modal_transitions.h"
#include "cursive0/sema/record_methods.h"
#include "cursive0/sema/resolve_qual.h"
#include "cursive0/sema/resolver.h"
#include "cursive0/sema/scopes.h"
#include "cursive0/sema/scopes_lookup.h"
#include "cursive0/sema/string_bytes.h"
#include "cursive0/sema/subtyping.h"
#include "cursive0/sema/type_equiv.h"
#include "cursive0/sema/type_expr.h"
#include "cursive0/sema/types.h"
#include "cursive0/sema/visibility.h"
#include "cursive0/semantics/cleanup.h"
#include "cursive0/semantics/apply.h"
#include "cursive0/semantics/exec.h"
#include "cursive0/semantics/builtins.h"
#include "cursive0/semantics/match.h"
#include "cursive0/syntax/token.h"

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

static constexpr std::array<std::string_view, 6> kSignedIntTypes = {
    "i8", "i16", "i32", "i64", "i128", "isize"};

static constexpr std::array<std::string_view, 6> kUnsignedIntTypes = {
    "u8", "u16", "u32", "u64", "u128", "usize"};

static constexpr unsigned kPointerSizeBits = 64;

bool EndsWith(std::string_view value, std::string_view suffix) {
  if (suffix.size() > value.size()) {
    return false;
  }
  return value.substr(value.size() - suffix.size()) == suffix;
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

bool IsSignedIntTypeName(std::string_view name) {
  for (const auto& t : kSignedIntTypes) {
    if (name == t) {
      return true;
    }
  }
  return false;
}

bool IsUnsignedIntTypeName(std::string_view name) {
  for (const auto& t : kUnsignedIntTypes) {
    if (name == t) {
      return true;
    }
  }
  return false;
}

std::optional<unsigned> IntWidthBits(std::string_view name) {
  if (name == "i8" || name == "u8") {
    return 8;
  }
  if (name == "i16" || name == "u16") {
    return 16;
  }
  if (name == "i32" || name == "u32") {
    return 32;
  }
  if (name == "i64" || name == "u64") {
    return 64;
  }
  if (name == "i128" || name == "u128") {
    return 128;
  }
  if (name == "isize" || name == "usize") {
    return kPointerSizeBits;
  }
  return std::nullopt;
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

std::optional<std::string_view> IntSuffix(const syntax::Token& lit) {
  if (lit.kind != syntax::TokenKind::IntLiteral) {
    return std::nullopt;
  }
  for (const auto suffix : kIntSuffixes) {
    if (EndsWith(lit.lexeme, suffix)) {
      const std::size_t core_len = lit.lexeme.size() - suffix.size();
      if (core_len == 0) {
        continue;
      }
      return suffix;
    }
  }
  return std::nullopt;
}

std::optional<std::string_view> FloatSuffix(const syntax::Token& lit) {
  if (lit.kind != syntax::TokenKind::FloatLiteral) {
    return std::nullopt;
  }
  for (const auto suffix : kFloatSuffixes) {
    if (EndsWith(lit.lexeme, suffix)) {
      const std::size_t core_len = lit.lexeme.size() - suffix.size();
      if (core_len == 0) {
        continue;
      }
      return suffix;
    }
  }
  return std::nullopt;
}

std::optional<sema::TypeRef> InferLiteralType(const syntax::Token& lit) {
  switch (lit.kind) {
    case syntax::TokenKind::IntLiteral: {
      if (const auto suffix = IntSuffix(lit)) {
        return sema::MakeTypePrim(std::string(*suffix));
      }
      return sema::MakeTypePrim("i32");
    }
    case syntax::TokenKind::FloatLiteral: {
      if (const auto suffix = FloatSuffix(lit)) {
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
    default:
      break;
  }
  return std::nullopt;
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
  core::UInt128 value = core::UInt128FromU64(0);
  const auto core = StripIntSuffix(lexeme);
  if (!ParseIntCore(core, value)) {
    return std::nullopt;
  }
  return value;
}

std::optional<double> ParseFloatLiteralValue(std::string_view lexeme) {
  const auto core = StripFloatSuffix(lexeme);
  if (core.empty()) {
    return std::nullopt;
  }
  char* end = nullptr;
  const std::string temp(core);
  const double value = std::strtod(temp.c_str(), &end);
  if (!end || *end != '\0') {
    return std::nullopt;
  }
  return value;
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
    if (IsFloatTypeName(prim->name) &&
        lit.kind == syntax::TokenKind::FloatLiteral) {
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
        (!str->state.has_value() || *str->state == sema::StringState::View)) {
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

Control PanicCtrl() {
  Control ctrl;
  ctrl.kind = ControlKind::Panic;
  ctrl.value = std::nullopt;
  return ctrl;
}

Outcome PanicOutcome() {
  return MakeCtrl(PanicCtrl());
}

bool UInt128Equal(core::UInt128 lhs, core::UInt128 rhs) {
  return core::UInt128High(lhs) == core::UInt128High(rhs) &&
         core::UInt128Low(lhs) == core::UInt128Low(rhs);
}

bool UInt128IsZero(core::UInt128 value) {
  return core::UInt128High(value) == 0 && core::UInt128Low(value) == 0;
}

bool UInt128Less(core::UInt128 lhs, core::UInt128 rhs) {
  return core::UInt128Greater(rhs, lhs);
}

int CompareUInt128(core::UInt128 lhs, core::UInt128 rhs) {
  if (core::UInt128Greater(lhs, rhs)) {
    return 1;
  }
  if (core::UInt128Greater(rhs, lhs)) {
    return -1;
  }
  return 0;
}

core::UInt128 UInt128Mod(core::UInt128 value, core::UInt128 mod) {
  if (UInt128IsZero(mod)) {
    return core::UInt128FromU64(0);
  }
  const core::UInt128 div = core::UInt128Div(value, mod);
  const core::UInt128 prod = core::UInt128Mul(div, mod);
  return core::UInt128Sub(value, prod);
}

bool UInt128AddOverflow(core::UInt128 lhs,
                        core::UInt128 rhs,
                        core::UInt128* out) {
  const core::UInt128 sum = core::UInt128Add(lhs, rhs);
  if (out) {
    *out = sum;
  }
  return UInt128Less(sum, lhs);
}

bool UInt128MulOverflow(core::UInt128 lhs,
                        core::UInt128 rhs,
                        core::UInt128* out) {
  if (UInt128IsZero(lhs) || UInt128IsZero(rhs)) {
    if (out) {
      *out = core::UInt128FromU64(0);
    }
    return false;
  }
  const core::UInt128 max = core::UInt128Max();
  const core::UInt128 limit = core::UInt128Div(max, rhs);
  if (core::UInt128Greater(lhs, limit)) {
    return true;
  }
  if (out) {
    *out = core::UInt128Mul(lhs, rhs);
  }
  return false;
}

core::UInt128 UInt128And(core::UInt128 lhs, core::UInt128 rhs) {
  return core::MakeUInt128(core::UInt128High(lhs) & core::UInt128High(rhs),
                           core::UInt128Low(lhs) & core::UInt128Low(rhs));
}

core::UInt128 UInt128Or(core::UInt128 lhs, core::UInt128 rhs) {
  return core::MakeUInt128(core::UInt128High(lhs) | core::UInt128High(rhs),
                           core::UInt128Low(lhs) | core::UInt128Low(rhs));
}

core::UInt128 UInt128Xor(core::UInt128 lhs, core::UInt128 rhs) {
  return core::MakeUInt128(core::UInt128High(lhs) ^ core::UInt128High(rhs),
                           core::UInt128Low(lhs) ^ core::UInt128Low(rhs));
}

struct SignedMag {
  bool negative = false;
  core::UInt128 magnitude{};
};

SignedMag NormalizeSigned(SignedMag value) {
  if (UInt128IsZero(value.magnitude)) {
    value.negative = false;
  }
  return value;
}

int CompareSigned(const SignedMag& lhs, const SignedMag& rhs) {
  if (lhs.negative != rhs.negative) {
    return lhs.negative ? -1 : 1;
  }
  const int mag_cmp = CompareUInt128(lhs.magnitude, rhs.magnitude);
  if (lhs.negative) {
    return -mag_cmp;
  }
  return mag_cmp;
}

std::optional<SignedMag> AddSigned(const SignedMag& lhs,
                                   const SignedMag& rhs) {
  SignedMag out;
  if (lhs.negative == rhs.negative) {
    core::UInt128 sum{};
    if (UInt128AddOverflow(lhs.magnitude, rhs.magnitude, &sum)) {
      return std::nullopt;
    }
    out.negative = lhs.negative;
    out.magnitude = sum;
    return NormalizeSigned(out);
  }
  const int cmp = CompareUInt128(lhs.magnitude, rhs.magnitude);
  if (cmp == 0) {
    out.negative = false;
    out.magnitude = core::UInt128FromU64(0);
    return out;
  }
  if (cmp > 0) {
    out.negative = lhs.negative;
    out.magnitude = core::UInt128Sub(lhs.magnitude, rhs.magnitude);
    return NormalizeSigned(out);
  }
  out.negative = rhs.negative;
  out.magnitude = core::UInt128Sub(rhs.magnitude, lhs.magnitude);
  return NormalizeSigned(out);
}

std::optional<SignedMag> SubSigned(const SignedMag& lhs,
                                   const SignedMag& rhs) {
  SignedMag neg = rhs;
  neg.negative = !neg.negative;
  return AddSigned(lhs, neg);
}

std::optional<SignedMag> MulSigned(const SignedMag& lhs,
                                   const SignedMag& rhs) {
  core::UInt128 prod{};
  if (UInt128MulOverflow(lhs.magnitude, rhs.magnitude, &prod)) {
    return std::nullopt;
  }
  SignedMag out;
  out.negative = lhs.negative != rhs.negative;
  out.magnitude = prod;
  return NormalizeSigned(out);
}

std::optional<SignedMag> DivSigned(const SignedMag& lhs,
                                   const SignedMag& rhs) {
  if (UInt128IsZero(rhs.magnitude)) {
    return std::nullopt;
  }
  SignedMag out;
  out.negative = lhs.negative != rhs.negative;
  out.magnitude = core::UInt128Div(lhs.magnitude, rhs.magnitude);
  return NormalizeSigned(out);
}

std::optional<SignedMag> ModSigned(const SignedMag& lhs,
                                   const SignedMag& rhs) {
  if (UInt128IsZero(rhs.magnitude)) {
    return std::nullopt;
  }
  const core::UInt128 div = core::UInt128Div(lhs.magnitude, rhs.magnitude);
  const core::UInt128 prod = core::UInt128Mul(div, rhs.magnitude);
  SignedMag out;
  out.negative = lhs.negative;
  out.magnitude = core::UInt128Sub(lhs.magnitude, prod);
  return NormalizeSigned(out);
}

core::UInt128 Pow2(unsigned width) {
  if (width == 0) {
    return core::UInt128FromU64(1);
  }
  if (width >= 128) {
    return core::UInt128FromU64(0);
  }
  return core::UInt128ShiftLeft(core::UInt128FromU64(1), width);
}

core::UInt128 UnsignedMax(unsigned width) {
  if (width >= 128) {
    return core::UInt128Max();
  }
  const core::UInt128 pow = Pow2(width);
  return core::UInt128Sub(pow, core::UInt128FromU64(1));
}

core::UInt128 SignedMax(unsigned width) {
  const core::UInt128 pow = Pow2(width - 1);
  return core::UInt128Sub(pow, core::UInt128FromU64(1));
}

core::UInt128 SignedMinMag(unsigned width) {
  return Pow2(width - 1);
}

bool InRange(const SignedMag& value, std::string_view type_name) {
  const auto width_opt = IntWidthBits(type_name);
  if (!width_opt.has_value()) {
    return false;
  }
  const unsigned width = *width_opt;
  SignedMag normalized = NormalizeSigned(value);
  if (IsUnsignedIntTypeName(type_name)) {
    if (normalized.negative) {
      return false;
    }
    const core::UInt128 max = UnsignedMax(width);
    return CompareUInt128(normalized.magnitude, max) <= 0;
  }
  if (IsSignedIntTypeName(type_name)) {
    if (!normalized.negative) {
      const core::UInt128 max = SignedMax(width);
      return CompareUInt128(normalized.magnitude, max) <= 0;
    }
    const core::UInt128 min_mag = SignedMinMag(width);
    return CompareUInt128(normalized.magnitude, min_mag) <= 0;
  }
  return false;
}

core::UInt128 ToUnsigned(unsigned width, const SignedMag& value) {
  SignedMag normalized = NormalizeSigned(value);
  if (width >= 128) {
    if (!normalized.negative) {
      return normalized.magnitude;
    }
    return core::UInt128Sub(core::UInt128FromU64(0), normalized.magnitude);
  }
  const core::UInt128 modulus = Pow2(width);
  const core::UInt128 mod = UInt128Mod(normalized.magnitude, modulus);
  if (!normalized.negative) {
    return mod;
  }
  if (UInt128IsZero(mod)) {
    return core::UInt128FromU64(0);
  }
  return core::UInt128Sub(modulus, mod);
}

SignedMag ToSigned(unsigned width, core::UInt128 value) {
  if (width >= 128) {
    const core::UInt128 sign_bit = Pow2(127);
    if (core::UInt128Greater(value, sign_bit) ||
        UInt128Equal(value, sign_bit)) {
      SignedMag out;
      out.negative = true;
      out.magnitude = core::UInt128Sub(core::UInt128FromU64(0), value);
      return NormalizeSigned(out);
    }
    return SignedMag{false, value};
  }
  const core::UInt128 sign_bit = Pow2(width - 1);
  if (core::UInt128Greater(value, sign_bit) ||
      UInt128Equal(value, sign_bit)) {
    const core::UInt128 modulus = Pow2(width);
    SignedMag out;
    out.negative = true;
    out.magnitude = core::UInt128Sub(modulus, value);
    return NormalizeSigned(out);
  }
  return SignedMag{false, value};
}

SignedMag SignedMagOf(const IntVal& value) {
  SignedMag out;
  out.negative = value.negative;
  out.magnitude = value.magnitude;
  return NormalizeSigned(out);
}

IntVal IntValOf(std::string_view type_name, const SignedMag& value) {
  IntVal out;
  out.type = std::string(type_name);
  SignedMag normalized = NormalizeSigned(value);
  out.negative = normalized.negative;
  out.magnitude = normalized.magnitude;
  return out;
}

std::optional<SignedMag> PowSigned(const SignedMag& base,
                                   const SignedMag& exponent) {
  SignedMag exp = NormalizeSigned(exponent);
  if (exp.negative) {
    return std::nullopt;
  }
  if (!core::UInt128FitsU64(exp.magnitude)) {
    return std::nullopt;
  }
  std::uint64_t power = core::UInt128ToU64(exp.magnitude);
  SignedMag result;
  result.negative = false;
  result.magnitude = core::UInt128FromU64(1);
  SignedMag factor = NormalizeSigned(base);
  while (power > 0) {
    if (power & 1u) {
      const auto mul = MulSigned(result, factor);
      if (!mul.has_value()) {
        return std::nullopt;
      }
      result = *mul;
    }
    power >>= 1u;
    if (power == 0) {
      break;
    }
    const auto mul = MulSigned(factor, factor);
    if (!mul.has_value()) {
      return std::nullopt;
    }
    factor = *mul;
  }
  return NormalizeSigned(result);
}

long double SignedToLongDouble(const SignedMag& value) {
  const long double high =
      static_cast<long double>(core::UInt128High(value.magnitude));
  const long double low =
      static_cast<long double>(core::UInt128Low(value.magnitude));
  long double out = high * std::pow(2.0L, 64) + low;
  if (value.negative) {
    out = -out;
  }
  return out;
}

double CastFloatValue(std::string_view type_name, double value) {
  if (type_name == "f32" || type_name == "f16") {
    return static_cast<double>(static_cast<float>(value));
  }
  return value;
}

std::optional<SignedMag> TruncToSignedMag(double value) {
  if (!std::isfinite(value)) {
    return std::nullopt;
  }
  const long double truncated = std::trunc(static_cast<long double>(value));
  const long double abs_val = std::fabsl(truncated);
  if (abs_val > static_cast<long double>(
                    std::numeric_limits<std::uint64_t>::max())) {
    return std::nullopt;
  }
  SignedMag out;
  out.negative = truncated < 0;
  const auto mag = static_cast<std::uint64_t>(abs_val);
  out.magnitude = core::UInt128FromU64(mag);
  return NormalizeSigned(out);
}

std::optional<Value> CmpValues(std::string_view op,
                               const Value& lhs,
                               const Value& rhs) {
  if (op == "==" || op == "!=") {
    if (lhs.node.index() != rhs.node.index()) {
      return std::nullopt;
    }
    bool eq = ValueEqual(lhs, rhs);
    if (const auto* lf = std::get_if<FloatVal>(&lhs.node)) {
      const auto* rf = std::get_if<FloatVal>(&rhs.node);
      if (rf && lf->type == rf->type &&
          (std::isnan(lf->value) || std::isnan(rf->value))) {
        eq = false;
      }
    }
    const bool result = (op == "==") ? eq : !eq;
    return Value{BoolVal{result}};
  }

  if (op == "<" || op == "<=" || op == ">" || op == ">=") {
    if (const auto* li = std::get_if<IntVal>(&lhs.node)) {
      if (const auto* ri = std::get_if<IntVal>(&rhs.node)) {
        if (li->type != ri->type) {
          return std::nullopt;
        }
        const int cmp = CompareSigned(SignedMagOf(*li), SignedMagOf(*ri));
        bool result = false;
        if (op == "<") {
          result = cmp < 0;
        } else if (op == "<=") {
          result = cmp <= 0;
        } else if (op == ">") {
          result = cmp > 0;
        } else if (op == ">=") {
          result = cmp >= 0;
        }
        return Value{BoolVal{result}};
      }
    }
    if (const auto* lf = std::get_if<FloatVal>(&lhs.node)) {
      if (const auto* rf = std::get_if<FloatVal>(&rhs.node)) {
        if (lf->type != rf->type) {
          return std::nullopt;
        }
        if (std::isnan(lf->value) || std::isnan(rf->value)) {
          return Value{BoolVal{false}};
        }
        bool result = false;
        if (op == "<") {
          result = lf->value < rf->value;
        } else if (op == "<=") {
          result = lf->value <= rf->value;
        } else if (op == ">") {
          result = lf->value > rf->value;
        } else if (op == ">=") {
          result = lf->value >= rf->value;
        }
        return Value{BoolVal{result}};
      }
    }
    if (const auto* lc = std::get_if<CharVal>(&lhs.node)) {
      if (const auto* rc = std::get_if<CharVal>(&rhs.node)) {
        bool result = false;
        if (op == "<") {
          result = lc->value < rc->value;
        } else if (op == "<=") {
          result = lc->value <= rc->value;
        } else if (op == ">") {
          result = lc->value > rc->value;
        } else if (op == ">=") {
          result = lc->value >= rc->value;
        }
        return Value{BoolVal{result}};
      }
    }
  }
  return std::nullopt;
}

std::optional<Value> EvalUnOp(std::string_view op, const Value& value) {
  if (op == "!") {
    if (const auto* b = std::get_if<BoolVal>(&value.node)) {
      return Value{BoolVal{!b->value}};
    }
    if (const auto* i = std::get_if<IntVal>(&value.node)) {
      const auto width = IntWidthBits(i->type);
      if (!width.has_value()) {
        return std::nullopt;
      }
      const core::UInt128 u = ToUnsigned(*width, SignedMagOf(*i));
      const core::UInt128 mask = UnsignedMax(*width);
      const core::UInt128 u_not = core::UInt128Sub(mask, u);
      const SignedMag out_signed =
          IsSignedIntTypeName(i->type) ? ToSigned(*width, u_not)
                                       : SignedMag{false, u_not};
      return Value{IntValOf(i->type, out_signed)};
    }
    return std::nullopt;
  }
  if (op == "-") {
    if (const auto* i = std::get_if<IntVal>(&value.node)) {
      if (!IsSignedIntTypeName(i->type)) {
        return std::nullopt;
      }
      SignedMag x = SignedMagOf(*i);
      x.negative = !x.negative;
      if (!InRange(x, i->type)) {
        return std::nullopt;
      }
      return Value{IntValOf(i->type, x)};
    }
    if (const auto* f = std::get_if<FloatVal>(&value.node)) {
      FloatVal out;
      out.type = f->type;
      out.value = CastFloatValue(out.type, -f->value);
      return Value{out};
    }
    return std::nullopt;
  }
  if (op == "widen") {
    return value;
  }
  return std::nullopt;
}

bool IsZeroIntVal(const IntVal& value) {
  return !value.negative && UInt128IsZero(value.magnitude);
}

bool IsMinusOneIntVal(const IntVal& value) {
  if (!value.negative) {
    return false;
  }
  return UInt128Equal(value.magnitude, core::UInt128FromU64(1));
}

bool IsMinSignedIntVal(const IntVal& value) {
  if (!value.negative) {
    return false;
  }
  if (!IsSignedIntTypeName(value.type)) {
    return false;
  }
  const auto width = IntWidthBits(value.type);
  if (!width.has_value()) {
    return false;
  }
  const core::UInt128 sign_bit = Pow2(*width - 1);
  return UInt128Equal(value.magnitude, sign_bit);
}

PanicReason UnOpPanicReason(std::string_view op, const Value& value) {
  if (op == "-") {
    const auto* int_val = std::get_if<IntVal>(&value.node);
    if (int_val && IsSignedIntTypeName(int_val->type)) {
      return PanicReason::Overflow;
    }
  }
  return PanicReason::Other;
}

PanicReason BinOpPanicReason(std::string_view op,
                             const Value& lhs,
                             const Value& rhs) {
  if (op == "<<" || op == ">>") {
    return PanicReason::Shift;
  }
  const auto* li = std::get_if<IntVal>(&lhs.node);
  const auto* ri = std::get_if<IntVal>(&rhs.node);
  if ((op == "/" || op == "%") && li && ri && li->type == ri->type &&
      IsIntTypeName(li->type)) {
    if (IsZeroIntVal(*ri)) {
      return PanicReason::DivZero;
    }
    if (IsMinSignedIntVal(*li) && IsMinusOneIntVal(*ri)) {
      return PanicReason::Overflow;
    }
    return PanicReason::Overflow;
  }
  if (op == "+" || op == "-" || op == "*" || op == "**") {
    return PanicReason::Overflow;
  }
  return PanicReason::Other;
}

std::optional<Value> EvalBinOpImpl(std::string_view op,
                                   const Value& lhs,
                                   const Value& rhs) {
  if (op == "==" || op == "!=" || op == "<" || op == "<=" ||
      op == ">" || op == ">=") {
    return CmpValues(op, lhs, rhs);
  }

  if (op == "&" || op == "|" || op == "^") {
    const auto* li = std::get_if<IntVal>(&lhs.node);
    const auto* ri = std::get_if<IntVal>(&rhs.node);
    if (!li || !ri || li->type != ri->type) {
      return std::nullopt;
    }
    const auto width = IntWidthBits(li->type);
    if (!width.has_value()) {
      return std::nullopt;
    }
    const core::UInt128 u1 = ToUnsigned(*width, SignedMagOf(*li));
    const core::UInt128 u2 = ToUnsigned(*width, SignedMagOf(*ri));
    core::UInt128 u = core::UInt128FromU64(0);
    if (op == "&") {
      u = UInt128And(u1, u2);
    } else if (op == "|") {
      u = UInt128Or(u1, u2);
    } else {
      u = UInt128Xor(u1, u2);
    }
    const SignedMag out =
        IsSignedIntTypeName(li->type) ? ToSigned(*width, u)
                                      : SignedMag{false, u};
    return Value{IntValOf(li->type, out)};
  }

  if (op == "<<" || op == ">>") {
    const auto* li = std::get_if<IntVal>(&lhs.node);
    const auto* ri = std::get_if<IntVal>(&rhs.node);
    if (!li || !ri || ri->type != "u32") {
      return std::nullopt;
    }
    const auto width = IntWidthBits(li->type);
    if (!width.has_value()) {
      return std::nullopt;
    }
    if (ri->negative || !core::UInt128FitsU64(ri->magnitude)) {
      return std::nullopt;
    }
    const std::uint64_t shift =
        core::UInt128ToU64(ri->magnitude);
    if (shift >= *width) {
      return std::nullopt;
    }
    core::UInt128 u = ToUnsigned(*width, SignedMagOf(*li));
    if (op == "<<") {
      u = core::UInt128ShiftLeft(u, static_cast<unsigned>(shift));
      if (*width < 128) {
        const core::UInt128 mask = UnsignedMax(*width);
        u = UInt128And(u, mask);
      }
    } else {
      u = core::UInt128ShiftRight(u, static_cast<unsigned>(shift));
    }
    const SignedMag out =
        IsSignedIntTypeName(li->type) ? ToSigned(*width, u)
                                      : SignedMag{false, u};
    return Value{IntValOf(li->type, out)};
  }

  const auto* li = std::get_if<IntVal>(&lhs.node);
  const auto* ri = std::get_if<IntVal>(&rhs.node);
  if (li && ri && li->type == ri->type && IsIntTypeName(li->type)) {
    SignedMag x1 = SignedMagOf(*li);
    SignedMag x2 = SignedMagOf(*ri);
    if (IsUnsignedIntTypeName(li->type) &&
        (x1.negative || x2.negative)) {
      return std::nullopt;
    }
    std::optional<SignedMag> result;
    if (op == "+") {
      result = AddSigned(x1, x2);
    } else if (op == "-") {
      result = SubSigned(x1, x2);
    } else if (op == "*") {
      result = MulSigned(x1, x2);
    } else if (op == "/") {
      result = DivSigned(x1, x2);
    } else if (op == "%") {
      result = ModSigned(x1, x2);
    } else if (op == "**") {
      result = PowSigned(x1, x2);
    }
    if (!result.has_value()) {
      return std::nullopt;
    }
    if (!InRange(*result, li->type)) {
      return std::nullopt;
    }
    return Value{IntValOf(li->type, *result)};
  }

  const auto* lf = std::get_if<FloatVal>(&lhs.node);
  const auto* rf = std::get_if<FloatVal>(&rhs.node);
  if (lf && rf && lf->type == rf->type && IsFloatTypeName(lf->type)) {
    double out = 0.0;
    if (op == "+") {
      out = lf->value + rf->value;
    } else if (op == "-") {
      out = lf->value - rf->value;
    } else if (op == "*") {
      out = lf->value * rf->value;
    } else if (op == "/") {
      out = lf->value / rf->value;
    } else if (op == "%") {
      out = std::fmod(lf->value, rf->value);
    } else if (op == "**") {
      out = std::pow(lf->value, rf->value);
    } else {
      return std::nullopt;
    }
    FloatVal v;
    v.type = lf->type;
    v.value = CastFloatValue(v.type, out);
    return Value{v};
  }
  return std::nullopt;
}

std::optional<Value> EvalCastVal(const sema::TypeRef& source,
                                 const sema::TypeRef& target,
                                 const Value& value) {
  if (!source || !target) {
    return std::nullopt;
  }
  const auto src = sema::StripPerm(source);
  const auto dst = sema::StripPerm(target);
  if (!src || !dst) {
    return std::nullopt;
  }
  const auto equiv = sema::TypeEquiv(src, dst);
  if (!equiv.ok) {
    return std::nullopt;
  }
  if (equiv.equiv) {
    SPEC_RULE("CastVal-Id");
    return value;
  }
  const auto* src_prim = std::get_if<sema::TypePrim>(&src->node);
  const auto* dst_prim = std::get_if<sema::TypePrim>(&dst->node);
  if (!src_prim || !dst_prim) {
    return std::nullopt;
  }

  const std::string& s = src_prim->name;
  const std::string& t = dst_prim->name;

  if (IsIntTypeName(s) && IsIntTypeName(t)) {
    const auto* iv = std::get_if<IntVal>(&value.node);
    if (!iv || iv->type != s) {
      return std::nullopt;
    }
    const auto width = IntWidthBits(t);
    if (!width.has_value()) {
      return std::nullopt;
    }
    const core::UInt128 u = ToUnsigned(*width, SignedMagOf(*iv));
    SignedMag out = IsSignedIntTypeName(t) ? ToSigned(*width, u)
                                           : SignedMag{false, u};
    if (IsSignedIntTypeName(t)) {
      SPEC_RULE("CastVal-Int-Int-Signed");
    } else {
      SPEC_RULE("CastVal-Int-Int-Unsigned");
    }
    return Value{IntValOf(t, out)};
  }

  if (IsIntTypeName(s) && IsFloatTypeName(t)) {
    const auto* iv = std::get_if<IntVal>(&value.node);
    if (!iv || iv->type != s) {
      return std::nullopt;
    }
    const long double as_ld = SignedToLongDouble(SignedMagOf(*iv));
    FloatVal out;
    out.type = t;
    out.value = CastFloatValue(out.type, static_cast<double>(as_ld));
    SPEC_RULE("CastVal-Int-Float");
    return Value{out};
  }

  if (IsFloatTypeName(s) && IsFloatTypeName(t)) {
    const auto* fv = std::get_if<FloatVal>(&value.node);
    if (!fv || fv->type != s) {
      return std::nullopt;
    }
    FloatVal out;
    out.type = t;
    out.value = CastFloatValue(out.type, fv->value);
    SPEC_RULE("CastVal-Float-Float");
    return Value{out};
  }

  if (IsFloatTypeName(s) && IsIntTypeName(t)) {
    const auto* fv = std::get_if<FloatVal>(&value.node);
    if (!fv || fv->type != s) {
      return std::nullopt;
    }
    auto trunc = TruncToSignedMag(fv->value);
    if (!trunc.has_value()) {
      return std::nullopt;
    }
    if (!InRange(*trunc, t)) {
      return std::nullopt;
    }
    SPEC_RULE("CastVal-Float-Int");
    return Value{IntValOf(t, *trunc)};
  }

  if (s == "bool" && IsIntTypeName(t)) {
    const auto* bv = std::get_if<BoolVal>(&value.node);
    if (!bv) {
      return std::nullopt;
    }
    SignedMag out;
    out.negative = false;
    out.magnitude = core::UInt128FromU64(bv->value ? 1 : 0);
    SPEC_RULE("CastVal-Bool-Int");
    return Value{IntValOf(t, out)};
  }

  if (IsIntTypeName(s) && t == "bool") {
    const auto* iv = std::get_if<IntVal>(&value.node);
    if (!iv || iv->type != s) {
      return std::nullopt;
    }
    const bool nonzero = iv->negative ||
                         !UInt128IsZero(iv->magnitude);
    SPEC_RULE("CastVal-Int-Bool");
    return Value{BoolVal{nonzero}};
  }

  if (s == "char" && t == "u32") {
    const auto* cv = std::get_if<CharVal>(&value.node);
    if (!cv) {
      return std::nullopt;
    }
    SignedMag out;
    out.negative = false;
    out.magnitude = core::UInt128FromU64(cv->value);
    SPEC_RULE("CastVal-Char-U32");
    return Value{IntValOf("u32", out)};
  }

  if (s == "u32" && t == "char") {
    const auto* iv = std::get_if<IntVal>(&value.node);
    if (!iv || iv->type != "u32" || iv->negative ||
        !core::UInt128FitsU64(iv->magnitude)) {
      return std::nullopt;
    }
    const std::uint64_t code = core::UInt128ToU64(iv->magnitude);
    if (code > 0x10FFFFu || (code >= 0xD800u && code <= 0xDFFFu)) {
      return std::nullopt;
    }
    CharVal out;
    out.value = static_cast<std::uint32_t>(code);
    SPEC_RULE("CastVal-U32-Char");
    return Value{out};
  }

  return std::nullopt;
}

std::optional<Value> EvalTransmuteVal(const sema::TypeRef& source,
                                      const sema::TypeRef& target,
                                      const Value& value) {
  if (!source || !target) {
    return std::nullopt;
  }
  const auto src = sema::StripPerm(source);
  const auto dst = sema::StripPerm(target);
  if (!src || !dst) {
    return std::nullopt;
  }
  const auto equiv = sema::TypeEquiv(src, dst);
  if (!equiv.ok) {
    return std::nullopt;
  }
  if (equiv.equiv) {
    return value;
  }
  const auto* src_prim = std::get_if<sema::TypePrim>(&src->node);
  const auto* dst_prim = std::get_if<sema::TypePrim>(&dst->node);
  if (!src_prim || !dst_prim) {
    return std::nullopt;
  }
  const std::string& s = src_prim->name;
  const std::string& t = dst_prim->name;
  if (IsIntTypeName(s) && IsIntTypeName(t)) {
    const auto* iv = std::get_if<IntVal>(&value.node);
    if (!iv || iv->type != s) {
      return std::nullopt;
    }
    const auto w_s = IntWidthBits(s);
    const auto w_t = IntWidthBits(t);
    if (!w_s.has_value() || !w_t.has_value() || *w_s != *w_t) {
      return std::nullopt;
    }
    const core::UInt128 u = ToUnsigned(*w_s, SignedMagOf(*iv));
    SignedMag out = IsSignedIntTypeName(t) ? ToSigned(*w_s, u)
                                           : SignedMag{false, u};
    return Value{IntValOf(t, out)};
  }
  if (IsFloatTypeName(s) && IsFloatTypeName(t)) {
    const auto* fv = std::get_if<FloatVal>(&value.node);
    if (!fv || fv->type != s) {
      return std::nullopt;
    }
    const auto w_s = IntWidthBits(s == "f16" ? "u16" : (s == "f32" ? "u32" : "u64"));
    const auto w_t = IntWidthBits(t == "f16" ? "u16" : (t == "f32" ? "u32" : "u64"));
    if (!w_s.has_value() || !w_t.has_value() || *w_s != *w_t) {
      return std::nullopt;
    }
    FloatVal out;
    out.type = t;
    out.value = CastFloatValue(out.type, fv->value);
    return Value{out};
  }
  return std::nullopt;
}

std::optional<sema::TypeRef> SuccessMemberType(const SemanticsContext& ctx,
                                               const sema::TypeRef& ret_type,
                                               const sema::TypeRef& expr_type) {
  if (!ctx.sema || !ret_type || !expr_type) {
    return std::nullopt;
  }
  const auto stripped = sema::StripPerm(expr_type);
  if (!stripped) {
    return std::nullopt;
  }
  const auto* uni = std::get_if<sema::TypeUnion>(&stripped->node);
  if (!uni) {
    return std::nullopt;
  }
  std::optional<sema::TypeRef> success;
  for (const auto& member : uni->members) {
    const auto sub = sema::Subtyping(*ctx.sema, member, ret_type);
    if (!sub.ok) {
      return std::nullopt;
    }
    if (!sub.subtype) {
      if (success.has_value()) {
        return std::nullopt;
      }
      success = member;
    }
  }
  if (!success.has_value()) {
    return std::nullopt;
  }
  for (const auto& member : uni->members) {
    if (member == *success) {
      continue;
    }
    const auto sub = sema::Subtyping(*ctx.sema, member, ret_type);
    if (!sub.ok || !sub.subtype) {
      return std::nullopt;
    }
  }
  return success;
}

std::optional<std::pair<sema::TypeRef, Value>> UnionCaseOf(const Value& value) {
  const auto* uni = std::get_if<UnionVal>(&value.node);
  if (!uni || !uni->value) {
    return std::nullopt;
  }
  return std::make_pair(uni->member, *uni->value);
}

RangeKind RangeKindOf(syntax::RangeKind kind) {
  switch (kind) {
    case syntax::RangeKind::To:
      return RangeKind::To;
    case syntax::RangeKind::ToInclusive:
      return RangeKind::ToInclusive;
    case syntax::RangeKind::Full:
      return RangeKind::Full;
    case syntax::RangeKind::From:
      return RangeKind::From;
    case syntax::RangeKind::Exclusive:
      return RangeKind::Exclusive;
    case syntax::RangeKind::Inclusive:
      return RangeKind::Inclusive;
  }
  return RangeKind::Full;
}

RangeVal MakeRangeVal(RangeKind kind,
                      const std::optional<Value>& lo,
                      const std::optional<Value>& hi) {
  RangeVal r;
  r.kind = kind;
  if (lo.has_value()) {
    r.lo = std::make_shared<Value>(*lo);
  }
  if (hi.has_value()) {
    r.hi = std::make_shared<Value>(*hi);
  }
  return r;
}

std::optional<std::size_t> IndexNum(const Value& value) {
  const auto* int_val = std::get_if<IntVal>(&value.node);
  if (!int_val) {
    return std::nullopt;
  }
  if (int_val->type != "usize" || int_val->negative) {
    return std::nullopt;
  }
  if (!core::UInt128FitsU64(int_val->magnitude)) {
    return std::nullopt;
  }
  return static_cast<std::size_t>(core::UInt128ToU64(int_val->magnitude));
}

Value MakeUsizeValue(std::size_t value) {
  IntVal v;
  v.type = "usize";
  v.negative = false;
  v.magnitude = core::UInt128FromU64(static_cast<std::uint64_t>(value));
  return Value{v};
}

std::optional<std::size_t> IncIndex(const Value& value) {
  const auto index = IndexNum(value);
  if (!index.has_value()) {
    return std::nullopt;
  }
  if (*index == static_cast<std::size_t>(-1)) {
    return std::nullopt;
  }
  return *index + 1;
}

std::optional<std::pair<std::size_t, std::size_t>> SliceBounds(
    const RangeVal& range,
    std::size_t len) {
  std::optional<std::size_t> start;
  std::optional<std::size_t> end;
  switch (range.kind) {
    case RangeKind::Exclusive: {
      if (!range.lo || !range.hi) {
        return std::nullopt;
      }
      start = IndexNum(*range.lo);
      end = IndexNum(*range.hi);
      break;
    }
    case RangeKind::Inclusive: {
      if (!range.lo || !range.hi) {
        return std::nullopt;
      }
      start = IndexNum(*range.lo);
      const auto inc = IncIndex(*range.hi);
      if (!inc.has_value()) {
        return std::nullopt;
      }
      end = *inc;
      break;
    }
    case RangeKind::From: {
      if (!range.lo) {
        return std::nullopt;
      }
      start = IndexNum(*range.lo);
      end = len;
      break;
    }
    case RangeKind::To: {
      if (!range.hi) {
        return std::nullopt;
      }
      start = 0;
      end = IndexNum(*range.hi);
      break;
    }
    case RangeKind::ToInclusive: {
      if (!range.hi) {
        return std::nullopt;
      }
      start = 0;
      const auto inc = IncIndex(*range.hi);
      if (!inc.has_value()) {
        return std::nullopt;
      }
      end = *inc;
      break;
    }
    case RangeKind::Full: {
      start = 0;
      end = len;
      break;
    }
  }
  if (!start.has_value() || !end.has_value()) {
    return std::nullopt;
  }
  if (*start > *end || *end > len) {
    return std::nullopt;
  }
  return std::make_pair(*start, *end);
}

std::optional<std::size_t> Len(const Value& value) {
  if (const auto* array = std::get_if<ArrayVal>(&value.node)) {
    return array->elements.size();
  }
  if (const auto* slice = std::get_if<SliceVal>(&value.node)) {
    const auto bounds = SliceBounds(slice->range, slice->base.size());
    if (!bounds.has_value()) {
      return std::nullopt;
    }
    return bounds->second - bounds->first;
  }
  if (const auto* str = std::get_if<StringVal>(&value.node)) {
    return str->bytes.size();
  }
  if (const auto* bytes = std::get_if<BytesVal>(&value.node)) {
    return bytes->bytes.size();
  }
  return std::nullopt;
}

std::optional<Value> FieldValue(const Value& value, std::string_view name) {
  const auto* record = std::get_if<RecordVal>(&value.node);
  if (!record) {
    return std::nullopt;
  }
  for (const auto& [field, field_value] : record->fields) {
    if (field == name) {
      return field_value;
    }
  }
  return std::nullopt;
}

std::optional<Value> TupleValue(const Value& value, std::size_t index) {
  const auto* tuple = std::get_if<TupleVal>(&value.node);
  if (!tuple) {
    return std::nullopt;
  }
  if (index >= tuple->elements.size()) {
    return std::nullopt;
  }
  return tuple->elements[index];
}

std::optional<Value> IndexValue(const Value& value, std::size_t index) {
  if (const auto* array = std::get_if<ArrayVal>(&value.node)) {
    if (index >= array->elements.size()) {
      return std::nullopt;
    }
    return array->elements[index];
  }
  if (const auto* slice = std::get_if<SliceVal>(&value.node)) {
    const auto bounds = SliceBounds(slice->range, slice->base.size());
    if (!bounds.has_value()) {
      return std::nullopt;
    }
    const auto start = bounds->first;
    const auto end = bounds->second;
    if (index >= end - start) {
      return std::nullopt;
    }
    return slice->base[start + index];
  }
  return std::nullopt;
}

std::optional<Value> IndexValue(const Value& value, const Value& index_value) {
  const auto index = IndexNum(index_value);
  if (!index.has_value()) {
    return std::nullopt;
  }
  return IndexValue(value, *index);
}

bool UpdateField(Value& value,
                 std::string_view name,
                 const Value& field_value) {
  auto* record = std::get_if<RecordVal>(&value.node);
  if (!record) {
    return false;
  }
  for (auto& [field, elem] : record->fields) {
    if (field == name) {
      elem = field_value;
      return true;
    }
  }
  return false;
}

bool UpdateTuple(Value& value, std::size_t index, const Value& elem) {
  auto* tuple = std::get_if<TupleVal>(&value.node);
  if (!tuple || index >= tuple->elements.size()) {
    return false;
  }
  tuple->elements[index] = elem;
  return true;
}

bool UpdateIndex(Value& value, std::size_t index, const Value& elem) {
  if (auto* array = std::get_if<ArrayVal>(&value.node)) {
    if (index >= array->elements.size()) {
      return false;
    }
    array->elements[index] = elem;
    return true;
  }
  if (auto* slice = std::get_if<SliceVal>(&value.node)) {
    const auto bounds = SliceBounds(slice->range, slice->base.size());
    if (!bounds.has_value()) {
      return false;
    }
    const auto start = bounds->first;
    const auto end = bounds->second;
    if (index >= end - start) {
      return false;
    }
    slice->base[start + index] = elem;
    return true;
  }
  return false;
}

std::optional<Value> FieldUpdate(const Value& value,
                                 std::string_view name,
                                 const Value& field_value) {
  Value updated = value;
  if (!UpdateField(updated, name, field_value)) {
    return std::nullopt;
  }
  return updated;
}

std::optional<Value> TupleUpdate(const Value& value,
                                 std::size_t index,
                                 const Value& elem) {
  Value updated = value;
  if (!UpdateTuple(updated, index, elem)) {
    return std::nullopt;
  }
  return updated;
}

std::optional<Value> IndexUpdate(const Value& value,
                                 std::size_t index,
                                 const Value& elem) {
  Value updated = value;
  if (!UpdateIndex(updated, index, elem)) {
    return std::nullopt;
  }
  return updated;
}

std::optional<Value> IndexUpdate(const Value& value,
                                 const Value& index_value,
                                 const Value& elem) {
  const auto index = IndexNum(index_value);
  if (!index.has_value()) {
    return std::nullopt;
  }
  return IndexUpdate(value, *index, elem);
}

std::optional<std::size_t> SliceLen(const Value& value) {
  if (const auto* array = std::get_if<ArrayVal>(&value.node)) {
    return array->elements.size();
  }
  if (const auto* slice = std::get_if<SliceVal>(&value.node)) {
    const auto bounds = SliceBounds(slice->range, slice->base.size());
    if (!bounds.has_value()) {
      return std::nullopt;
    }
    return bounds->second - bounds->first;
  }
  return std::nullopt;
}

std::optional<Value> SliceElem(const Value& value, std::size_t index) {
  return IndexValue(value, index);
}

std::optional<std::vector<Value>> SliceElements(const Value& value) {
  if (const auto* array = std::get_if<ArrayVal>(&value.node)) {
    return array->elements;
  }
  if (const auto* slice = std::get_if<SliceVal>(&value.node)) {
    const auto bounds = SliceBounds(slice->range, slice->base.size());
    if (!bounds.has_value()) {
      return std::nullopt;
    }
    const auto start = bounds->first;
    const auto end = bounds->second;
    if (end < start || end > slice->base.size()) {
      return std::nullopt;
    }
    std::vector<Value> out;
    out.reserve(end - start);
    for (std::size_t i = start; i < end; ++i) {
      out.push_back(slice->base[i]);
    }
    return out;
  }
  return std::nullopt;
}

std::optional<Value> SliceUpdate(const Value& base,
                                 std::size_t start,
                                 const Value& rhs) {
  const auto elems = SliceElements(rhs);
  if (!elems.has_value()) {
    return std::nullopt;
  }
  Value current = base;
  for (std::size_t i = 0; i < elems->size(); ++i) {
    const auto updated = IndexUpdate(current, start + i, (*elems)[i]);
    if (!updated.has_value()) {
      return std::nullopt;
    }
    current = *updated;
  }
  return current;
}

RangeVal AbsoluteRange(std::size_t start, std::size_t end) {
  RangeVal range;
  range.kind = RangeKind::Exclusive;
  range.lo = std::make_shared<Value>(MakeUsizeValue(start));
  range.hi = std::make_shared<Value>(MakeUsizeValue(end));
  return range;
}

std::optional<Value> SliceValue(const Value& value, const RangeVal& range) {
  if (const auto* array = std::get_if<ArrayVal>(&value.node)) {
    const auto bounds = SliceBounds(range, array->elements.size());
    if (!bounds.has_value()) {
      return std::nullopt;
    }
    SliceVal slice;
    slice.base = array->elements;
    slice.range = range;
    return Value{slice};
  }
  if (const auto* slice_val = std::get_if<SliceVal>(&value.node)) {
    const auto base_bounds =
        SliceBounds(slice_val->range, slice_val->base.size());
    if (!base_bounds.has_value()) {
      return std::nullopt;
    }
    const auto len = base_bounds->second - base_bounds->first;
    const auto bounds = SliceBounds(range, len);
    if (!bounds.has_value()) {
      return std::nullopt;
    }
    const std::size_t abs_start = base_bounds->first + bounds->first;
    const std::size_t abs_end = base_bounds->first + bounds->second;
    SliceVal slice;
    slice.base = slice_val->base;
    slice.range = AbsoluteRange(abs_start, abs_end);
    return Value{slice};
  }
  return std::nullopt;
}

sema::TypePath TypePathOf(const syntax::TypePath& path) {
  sema::TypePath out;
  out.reserve(path.size());
  for (const auto& part : path) {
    out.push_back(part);
  }
  return out;
}

sema::ResolveTypePathResult ResolveTypePathAdapter(
    const sema::ScopeContext& ctx,
    const sema::NameMapTable& name_maps,
    const sema::ModuleNames& module_names,
    const syntax::TypePath& path) {
  sema::ResolveContext resolve_ctx;
  resolve_ctx.ctx = const_cast<sema::ScopeContext*>(&ctx);
  resolve_ctx.name_maps = &name_maps;
  resolve_ctx.module_names = &module_names;
  resolve_ctx.can_access = sema::CanAccess;
  const auto resolved = sema::ResolveTypePath(resolve_ctx, path);
  if (!resolved.ok) {
    return {false, resolved.diag_id, {}};
  }
  return {true, std::nullopt, resolved.value};
}

sema::TypeRef ExprTypeOf(const SemanticsContext& ctx,
                         const syntax::Expr& expr) {
  if (!ctx.sema || !ctx.sema->expr_types) {
    return nullptr;
  }
  const auto it = ctx.sema->expr_types->find(&expr);
  if (it == ctx.sema->expr_types->end()) {
    return nullptr;
  }
  return it->second;
}

bool PoisonedModulePath(const Sigma& sigma,
                        const std::optional<syntax::ModulePath>& path_opt) {
  if (!path_opt.has_value()) {
    return false;
  }
  const auto key = sema::PathKeyOf(*path_opt);
  return PoisonedModule(sigma, key);
}

Outcome EvalPathPoison(const SemanticsContext& ctx,
                       const Sigma& sigma,
                       const syntax::ModulePath& path,
                       std::string_view name) {
  if (!ctx.sema || !ctx.name_maps || !ctx.module_names) {
    return MakeVal(Value{UnitVal{}});
  }
  const auto resolved =
      sema::ResolveQualified(*ctx.sema, *ctx.name_maps, *ctx.module_names,
                             path, name, sema::EntityKind::Value,
                             sema::CanAccess);
  if (resolved.ok && resolved.entity.has_value() &&
      resolved.entity->origin_opt.has_value()) {
    if (PoisonedModulePath(sigma, resolved.entity->origin_opt)) {
      SPEC_RULE("EvalSigma-Path-Poison");
      return PanicOutcome();
    }
  } else {
    sema::ResolveQualContext qual_ctx;
    qual_ctx.ctx = ctx.sema;
    qual_ctx.name_maps = ctx.name_maps;
    qual_ctx.module_names = ctx.module_names;
    qual_ctx.resolve_type_path = ResolveTypePathAdapter;
    qual_ctx.can_access = sema::CanAccess;
    const auto record = sema::ResolveRecordPath(qual_ctx, path, name);
    if (record.ok) {
      const auto key = sema::PathKeyOf(record.path);
      if (!key.empty()) {
        auto module_key = key;
        module_key.pop_back();
        if (PoisonedModule(sigma, module_key)) {
          SPEC_RULE("EvalSigma-Path-Poison-RecordCtor");
          return PanicOutcome();
        }
      }
    }
  }
  return MakeVal(Value{UnitVal{}});
}

std::optional<std::string> PlaceRootImpl(const syntax::Expr& expr) {
  return std::visit(
      [&](const auto& node) -> std::optional<std::string> {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
          return node.name;
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
          return node.base ? PlaceRootImpl(*node.base) : std::nullopt;
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
          return node.base ? PlaceRootImpl(*node.base) : std::nullopt;
        } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
          return node.base ? PlaceRootImpl(*node.base) : std::nullopt;
        } else if constexpr (std::is_same_v<T, syntax::DerefExpr>) {
          return node.value ? PlaceRootImpl(*node.value) : std::nullopt;
        }
        return std::nullopt;
      },
      expr.node);
}

std::optional<std::string> FieldHeadImpl(const syntax::Expr& expr) {
  return std::visit(
      [&](const auto& node) -> std::optional<std::string> {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
          return std::nullopt;
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
          if (!node.base) {
            return std::nullopt;
          }
          auto head = FieldHeadImpl(*node.base);
          if (head.has_value()) {
            return head;
          }
          return node.name;
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
          return node.base ? FieldHeadImpl(*node.base) : std::nullopt;
        } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
          return node.base ? FieldHeadImpl(*node.base) : std::nullopt;
        } else if constexpr (std::is_same_v<T, syntax::DerefExpr>) {
          return std::nullopt;
        }
        return std::nullopt;
      },
      expr.node);
}

Outcome ReadPtrSigmaImpl(const Value& ptr, Sigma& sigma) {
  if (const auto* p = std::get_if<PtrVal>(&ptr.node)) {
    switch (p->state) {
      case sema::PtrState::Valid: {
        const auto value = ReadAddr(sigma, p->addr);
        if (!value.has_value()) {
          return PanicOutcome();
        }
        SPEC_RULE("ReadPtr-Safe");
        return MakeVal(*value);
      }
      case sema::PtrState::Null:
        SPEC_RULE("ReadPtr-Null");
        SetPanicReason(sigma, PanicReason::NullDeref);
        return PanicOutcome();
      case sema::PtrState::Expired:
        SPEC_RULE("ReadPtr-Expired");
        SetPanicReason(sigma, PanicReason::ExpiredDeref);
        return PanicOutcome();
    }
  }
  if (const auto* raw = std::get_if<RawPtrVal>(&ptr.node)) {
    const auto value = ReadAddr(sigma, raw->addr);
    if (!value.has_value()) {
      return PanicOutcome();
    }
    SPEC_RULE("ReadPtr-Raw");
    return MakeVal(*value);
  }
  return PanicOutcome();
}

Outcome ReadPlaceSigmaImpl(const SemanticsContext& ctx,
                           const syntax::Expr& expr,
                           Sigma& sigma) {
  return std::visit(
      [&](const auto& node) -> Outcome {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
          const auto binding = LookupBind(sigma, node.name);
          if (!binding.has_value() && ctx.sema) {
            const auto ent = sema::ResolveValueName(*ctx.sema, node.name);
            if (ent.has_value() && ent->origin_opt.has_value()) {
              if (PoisonedModulePath(sigma, ent->origin_opt)) {
                SPEC_RULE("ReadPlace-Ident-Poison");
                return PanicOutcome();
              }
            }
          }
          const auto value = LookupVal(ctx, sigma, node.name);
          if (!value.has_value()) {
            return PanicOutcome();
          }
          SPEC_RULE("ReadPlace-Ident");
          return MakeVal(*value);
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
          Outcome base_out = ReadPlaceSigmaImpl(ctx, *node.base, sigma);
          if (IsCtrl(base_out)) {
            return base_out;
          }
          const auto field =
              FieldValue(std::get<Value>(base_out.node), node.name);
          if (!field.has_value()) {
            return PanicOutcome();
          }
          SPEC_RULE("ReadPlace-Field");
          return MakeVal(*field);
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
          Outcome base_out = ReadPlaceSigmaImpl(ctx, *node.base, sigma);
          if (IsCtrl(base_out)) {
            return base_out;
          }
          auto index_value = ParseIntLiteralValue(node.index.lexeme);
          if (!index_value.has_value() ||
              !core::UInt128FitsU64(*index_value)) {
            return PanicOutcome();
          }
          const std::size_t index =
              static_cast<std::size_t>(core::UInt128ToU64(*index_value));
          const auto elem =
              TupleValue(std::get<Value>(base_out.node), index);
          if (!elem.has_value()) {
            return PanicOutcome();
          }
          SPEC_RULE("ReadPlace-Tuple");
          return MakeVal(*elem);
        } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
          Outcome base_out = ReadPlaceSigmaImpl(ctx, *node.base, sigma);
          if (IsCtrl(base_out)) {
            return base_out;
          }
          Outcome idx_out = EvalSigma(ctx, *node.index, sigma);
          if (IsCtrl(idx_out)) {
            return idx_out;
          }
          const auto& base_val = std::get<Value>(base_out.node);
          const auto& idx_val = std::get<Value>(idx_out.node);
          if (const auto* range = std::get_if<RangeVal>(&idx_val.node)) {
            const auto slice = SliceValue(base_val, *range);
            if (!slice.has_value()) {
              SPEC_RULE("ReadPlace-Index-Range-OOB");
              SetPanicReason(sigma, PanicReason::Bounds);
              return PanicOutcome();
            }
            SPEC_RULE("ReadPlace-Index-Range");
            return MakeVal(*slice);
          }
          const auto elem = IndexValue(base_val, idx_val);
          if (!elem.has_value()) {
            SPEC_RULE("ReadPlace-Index-OOB");
            SetPanicReason(sigma, PanicReason::Bounds);
            return PanicOutcome();
          }
          SPEC_RULE("ReadPlace-Index");
          return MakeVal(*elem);
        } else if constexpr (std::is_same_v<T, syntax::DerefExpr>) {
          Outcome base_out = ReadPlaceSigmaImpl(ctx, *node.value, sigma);
          if (IsCtrl(base_out)) {
            return base_out;
          }
          const auto ptr = std::get<Value>(base_out.node);
          Outcome out = ReadPtrSigmaImpl(ptr, sigma);
          SPEC_RULE("ReadPlace-Deref");
          return out;
        }
        return PanicOutcome();
      },
      expr.node);
}

using AddrResultImpl = std::variant<Addr, Control>;

bool IsAddrCtrlImpl(const AddrResultImpl& out) {
  return std::holds_alternative<Control>(out);
}

AddrResultImpl AddrOfSigmaImpl(const SemanticsContext& ctx,
                               const syntax::Expr& expr,
                               Sigma& sigma) {
  return std::visit(
      [&](const auto& node) -> AddrResult {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
          if (const auto binding = LookupBind(sigma, node.name)) {
            const auto addr = AddrOfBind(sigma, node.name);
            if (addr.has_value()) {
              SPEC_RULE("AddrOf-Ident");
              return *addr;
            }
          }
          if (ctx.sema) {
            const auto ent = sema::ResolveValueName(*ctx.sema, node.name);
            if (ent.has_value() && ent->origin_opt.has_value()) {
              if (PoisonedModulePath(sigma, ent->origin_opt)) {
                SPEC_RULE("AddrOf-Ident-Path-Poison");
                return PanicCtrl();
              }
              const auto path_key = sema::PathKeyOf(*ent->origin_opt);
              const std::string resolved_name =
                  ent->target_opt.has_value() ? *ent->target_opt : node.name;
              const auto it =
                  sigma.static_addrs.find({path_key, resolved_name});
              if (it != sigma.static_addrs.end()) {
                SPEC_RULE("AddrOf-Ident-Path");
                return it->second;
              }
            }
          }
          return PanicCtrl();
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
          AddrResult base_out = AddrOfSigmaImpl(ctx, *node.base, sigma);
          if (IsAddrCtrl(base_out)) {
            SPEC_RULE("AddrOf-Field-Ctrl");
            return std::get<Control>(base_out);
          }
          const Addr base = std::get<Addr>(base_out);
          const Addr addr = AllocAddr(sigma);
          sigma.addr_views[addr] =
              AddrView{base, AddrViewKind::Field, node.name, 0};
          if (const auto tag = AddrTag(sigma, base)) {
            sigma.addr_tags[addr] = *tag;
          }
          SPEC_RULE("AddrOf-Field");
          return addr;
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
          AddrResult base_out = AddrOfSigmaImpl(ctx, *node.base, sigma);
          if (IsAddrCtrl(base_out)) {
            SPEC_RULE("AddrOf-Tuple-Ctrl");
            return std::get<Control>(base_out);
          }
          auto index_value = ParseIntLiteralValue(node.index.lexeme);
          if (!index_value.has_value() ||
              !core::UInt128FitsU64(*index_value)) {
            return PanicCtrl();
          }
          const std::size_t index =
              static_cast<std::size_t>(core::UInt128ToU64(*index_value));
          const Addr base = std::get<Addr>(base_out);
          const Addr addr = AllocAddr(sigma);
          sigma.addr_views[addr] =
              AddrView{base, AddrViewKind::Tuple, "", index};
          if (const auto tag = AddrTag(sigma, base)) {
            sigma.addr_tags[addr] = *tag;
          }
          SPEC_RULE("AddrOf-Tuple");
          return addr;
        } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
          SPEC_RULE("AddrOf-Index");
          AddrResult base_out = AddrOfSigmaImpl(ctx, *node.base, sigma);
          if (IsAddrCtrl(base_out)) {
            SPEC_RULE("AddrOf-Index-Ctrl-Base");
            return std::get<Control>(base_out);
          }
          Outcome idx_out = EvalSigma(ctx, *node.index, sigma);
          if (IsCtrl(idx_out)) {
            SPEC_RULE("AddrOf-Index-Ctrl-Idx");
            return std::get<Control>(idx_out.node);
          }
          const auto addr = std::get<Addr>(base_out);
          const auto value = ReadAddr(sigma, addr);
          if (!value.has_value()) {
            return PanicCtrl();
          }
          const auto len = Len(*value);
          if (!len.has_value()) {
            return PanicCtrl();
          }
          const auto idx_num = IndexNum(std::get<Value>(idx_out.node));
          if (!idx_num.has_value() || *idx_num >= *len) {
            SPEC_RULE("AddrOfSigma-Index-OOB");
            SetPanicReason(sigma, PanicReason::Bounds);
            return PanicCtrl();
          }
          const Addr view = AllocAddr(sigma);
          sigma.addr_views[view] =
              AddrView{addr, AddrViewKind::Index, "", *idx_num};
          if (const auto tag = AddrTag(sigma, addr)) {
            sigma.addr_tags[view] = *tag;
          }
          SPEC_RULE("AddrOfSigma-Index-Ok");
          return view;
        } else if constexpr (std::is_same_v<T, syntax::DerefExpr>) {
          Outcome base_out = ReadPlaceSigmaImpl(ctx, *node.value, sigma);
          if (IsCtrl(base_out)) {
            SPEC_RULE("AddrOf-Deref-Ctrl");
            return std::get<Control>(base_out.node);
          }
          const auto ptr = std::get<Value>(base_out.node);
          if (const auto* p = std::get_if<PtrVal>(&ptr.node)) {
            switch (p->state) {
              case sema::PtrState::Valid:
                SPEC_RULE("AddrOf-Deref-Safe");
                return p->addr;
              case sema::PtrState::Null:
                SPEC_RULE("AddrOf-Deref-Null");
                SetPanicReason(sigma, PanicReason::NullDeref);
                return PanicCtrl();
              case sema::PtrState::Expired:
                SPEC_RULE("AddrOf-Deref-Expired");
                SetPanicReason(sigma, PanicReason::ExpiredDeref);
                return PanicCtrl();
            }
          }
          if (const auto* raw = std::get_if<RawPtrVal>(&ptr.node)) {
            SPEC_RULE("AddrOf-Deref-Raw");
            return raw->addr;
          }
          return PanicCtrl();
        }
        return PanicCtrl();
      },
      expr.node);
}

Outcome MovePlaceSigmaImpl(const SemanticsContext& ctx,
                           const syntax::Expr& expr,
                           Sigma& sigma) {
  Outcome out = ReadPlaceSigmaImpl(ctx, expr, sigma);
  if (IsCtrl(out)) {
    SPEC_RULE("MovePlace-Ctrl");
    return out;
  }
  auto root = PlaceRootImpl(expr);
  if (!root.has_value()) {
    return PanicOutcome();
  }
  const auto binding = LookupBind(sigma, *root);
  if (!binding.has_value()) {
    return PanicOutcome();
  }
  const auto info = BindInfoOf(sigma, *binding);
  if (!info.has_value() || info->mov != Movability::Mov) {
    return PanicOutcome();
  }
  const auto head = FieldHeadImpl(expr);
  if (!head.has_value()) {
    BindState state;
    state.kind = BindState::Kind::Moved;
    if (!SetState(sigma, *binding, state)) {
      return PanicOutcome();
    }
    SPEC_RULE("MovePlace-Whole");
    return out;
  }
  const auto state = BindStateOf(sigma, *binding);
  if (!state.has_value()) {
    return PanicOutcome();
  }
  BindState next = *state;
  switch (state->kind) {
    case BindState::Kind::Valid:
      next.kind = BindState::Kind::PartiallyMoved;
      next.fields.clear();
      next.fields.insert(*head);
      break;
    case BindState::Kind::PartiallyMoved:
      next.fields.insert(*head);
      break;
    case BindState::Kind::Moved:
      return PanicOutcome();
  }
  if (!SetState(sigma, *binding, next)) {
    return PanicOutcome();
  }
  SPEC_RULE("MovePlace-Field");
  return out;
}

std::optional<RegionTarget> ActiveTargetImpl(const Sigma& sigma) {
  if (sigma.region_stack.empty()) {
    return std::nullopt;
  }
  return sigma.region_stack.back().target;
}

std::optional<RegionTarget> ResolveTargetImpl(const Sigma& sigma,
                                              RegionTarget target) {
  for (auto it = sigma.region_stack.rbegin();
       it != sigma.region_stack.rend();
       ++it) {
    if (it->target == target) {
      return it->target;
    }
  }
  return std::nullopt;
}

std::optional<RegionTag> ResolveTagImpl(const Sigma& sigma,
                                        RegionTarget target) {
  for (auto it = sigma.region_stack.rbegin();
       it != sigma.region_stack.rend();
       ++it) {
    if (it->target == target) {
      return it->tag;
    }
  }
  return std::nullopt;
}

std::optional<RegionTarget> RegionTargetOfImpl(const Value& value) {
  const auto* record = std::get_if<RecordVal>(&value.node);
  if (!record || !record->record_type) {
    return std::nullopt;
  }
  const auto* modal = std::get_if<sema::TypeModalState>(&record->record_type->node);
  if (!modal || modal->path.size() != 1 || !sema::IdEq(modal->path[0], "Region")) {
    return std::nullopt;
  }
  for (const auto& [field_name, field_value] : record->fields) {
    if (!sema::IdEq(field_name, "handle")) {
      continue;
    }
    const auto* iv = std::get_if<IntVal>(&field_value.node);
    if (!iv || iv->negative || !sema::IdEq(iv->type, "usize") ||
        !core::UInt128FitsU64(iv->magnitude)) {
      return std::nullopt;
    }
    return static_cast<RegionTarget>(core::UInt128ToU64(iv->magnitude));
  }
  return std::nullopt;
}

std::optional<Value> RegionAllocImpl(Sigma& sigma,
                                     RegionTarget target,
                                     const Value& value) {
  const auto tag = ResolveTagImpl(sigma, target);
  if (tag.has_value()) {
    if (const auto* dyn = std::get_if<DynamicVal>(&value.node)) {
      sigma.addr_tags[dyn->data.addr] =
          RuntimeTag{RuntimeTagKind::RegionTag, *tag};
    }
  }
  return value;
}

}  // namespace

bool IsAddrCtrl(const AddrResult& out) {
  return std::holds_alternative<Control>(out);
}

Outcome ReadPtrSigma(const Value& ptr, Sigma& sigma) {
  return ReadPtrSigmaImpl(ptr, sigma);
}

Outcome ReadPlaceSigma(const SemanticsContext& ctx,
                       const syntax::Expr& expr,
                       Sigma& sigma) {
  return ReadPlaceSigmaImpl(ctx, expr, sigma);
}

Outcome MovePlaceSigma(const SemanticsContext& ctx,
                       const syntax::Expr& expr,
                       Sigma& sigma) {
  return MovePlaceSigmaImpl(ctx, expr, sigma);
}

AddrResult AddrOfSigma(const SemanticsContext& ctx,
                       const syntax::Expr& expr,
                       Sigma& sigma) {
  const auto out = AddrOfSigmaImpl(ctx, expr, sigma);
  if (IsAddrCtrlImpl(out)) {
    return std::get<Control>(out);
  }
  return std::get<Addr>(out);
}

std::optional<std::string> PlaceRoot(const syntax::Expr& expr) {
  return PlaceRootImpl(expr);
}

std::optional<std::string> FieldHead(const syntax::Expr& expr) {
  return FieldHeadImpl(expr);
}

std::optional<Value> EvalBinOp(std::string_view op,
                               const Value& lhs,
                               const Value& rhs) {
  return EvalBinOpImpl(op, lhs, rhs);
}

std::optional<RegionTarget> ActiveTarget(const Sigma& sigma) {
  return ActiveTargetImpl(sigma);
}

std::optional<RegionTarget> ResolveTarget(const Sigma& sigma,
                                          RegionTarget target) {
  return ResolveTargetImpl(sigma, target);
}

std::optional<RegionTag> ResolveTag(const Sigma& sigma, RegionTarget target) {
  return ResolveTagImpl(sigma, target);
}

std::optional<RegionTarget> RegionTargetOf(const Value& value) {
  return RegionTargetOfImpl(value);
}

std::optional<Value> RegionAlloc(Sigma& sigma,
                                 RegionTarget target,
                                 const Value& value) {
  return RegionAllocImpl(sigma, target, value);
}

namespace {

StmtOut PanicStmtOut() {
  Control ctrl;
  ctrl.kind = ControlKind::Panic;
  return MakeCtrlOut(ctrl);
}

bool DropOnAssign(const Sigma& sigma, const Binding& binding) {
  const auto info = BindInfoOf(sigma, binding);
  if (!info.has_value()) {
    return false;
  }
  return info->mov == Movability::Immov &&
         info->resp == Responsibility::Resp;
}

bool IsMoveExpr(const syntax::Expr& expr) {
  return std::holds_alternative<syntax::MoveExpr>(expr.node);
}

bool IsPlaceExpr(const syntax::Expr& expr) {
  return std::visit(
      [&](const auto& node) -> bool {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
          return true;
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
          return node.base ? IsPlaceExpr(*node.base) : false;
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
          return node.base ? IsPlaceExpr(*node.base) : false;
        } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
          return node.base ? IsPlaceExpr(*node.base) : false;
        } else if constexpr (std::is_same_v<T, syntax::DerefExpr>) {
          return node.value ? IsPlaceExpr(*node.value) : false;
        }
        return false;
      },
      expr.node);
}

Responsibility RespOfInit(const syntax::Expr& expr) {
  if (!IsPlaceExpr(expr)) {
    return Responsibility::Resp;
  }
  if (IsMoveExpr(expr)) {
    return Responsibility::Resp;
  }
  return Responsibility::Alias;
}

std::optional<BindInfo> StaticBindInfo(const SemanticsContext& ctx,
                                       const sema::PathKey& path,
                                       std::string_view name) {
  if (!ctx.sema) {
    return std::nullopt;
  }
  for (const auto& mod : ctx.sema->sigma.mods) {
    if (sema::PathKeyOf(mod.path) != path) {
      continue;
    }
    for (const auto& item : mod.items) {
      const auto* decl = std::get_if<syntax::StaticDecl>(&item);
      if (!decl) {
        continue;
      }
      const auto names = sema::PatNames(*decl->binding.pat);
      bool matches = false;
      for (const auto& n : names) {
        if (n == name) {
          matches = true;
          break;
        }
      }
      if (!matches) {
        continue;
      }
      BindInfo info;
      if (decl->binding.op.lexeme == ":=") {
        info.mov = Movability::Immov;
      }
      if (decl->binding.init) {
        info.resp = RespOfInit(*decl->binding.init);
      }
      return info;
    }
  }
  return std::nullopt;
}

bool DropOnAssignStatic(const SemanticsContext& ctx,
                        const sema::PathKey& path,
                        std::string_view name) {
  const auto info = StaticBindInfo(ctx, path, name);
  if (!info.has_value()) {
    return false;
  }
  return info->mov == Movability::Immov &&
         info->resp == Responsibility::Resp;
}

}  // namespace

StmtOut WritePtrSigma(const Value& ptr, const Value& value, Sigma& sigma) {
  if (const auto* p = std::get_if<PtrVal>(&ptr.node)) {
    switch (p->state) {
      case sema::PtrState::Valid:
        if (!WriteAddr(sigma, p->addr, value)) {
          return PanicStmtOut();
        }
        SPEC_RULE("WritePtr-Safe");
        return MakeOk();
      case sema::PtrState::Null:
        SPEC_RULE("WritePtr-Null");
        SetPanicReason(sigma, PanicReason::NullDeref);
        return PanicStmtOut();
      case sema::PtrState::Expired:
        SPEC_RULE("WritePtr-Expired");
        SetPanicReason(sigma, PanicReason::ExpiredDeref);
        return PanicStmtOut();
    }
  }
  if (const auto* raw = std::get_if<RawPtrVal>(&ptr.node)) {
    if (raw->qual != sema::RawPtrQual::Mut) {
      return PanicStmtOut();
    }
    if (!WriteAddr(sigma, raw->addr, value)) {
      return PanicStmtOut();
    }
    SPEC_RULE("WritePtr-Raw");
    return MakeOk();
  }
  return PanicStmtOut();
}

StmtOut WritePlaceSigma(const SemanticsContext& ctx,
                        const syntax::Expr& expr,
                        const Value& value,
                        Sigma& sigma) {
  return std::visit(
      [&](const auto& node) -> StmtOut {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
          const auto binding = LookupBind(sigma, node.name);
          if (binding.has_value()) {
            if (DropOnAssign(sigma, *binding)) {
              const auto dropped =
                  DropActionOut(ctx, *binding, sigma);
              if (dropped == DropStatus::Panic) {
                return PanicStmtOut();
              }
            }
            if (!UpdateVal(sigma, *binding, value)) {
              return PanicStmtOut();
            }
            BindState state;
            state.kind = BindState::Kind::Valid;
            if (!SetState(sigma, *binding, state)) {
              return PanicStmtOut();
            }
            SPEC_RULE("WritePlace-Ident");
            return MakeOk();
          }
          if (!ctx.sema) {
            return PanicStmtOut();
          }
          const auto ent = sema::ResolveValueName(*ctx.sema, node.name);
          if (!ent.has_value() || !ent->origin_opt.has_value()) {
            return PanicStmtOut();
          }
          const auto module_path = *ent->origin_opt;
          if (PoisonedModulePath(sigma, ent->origin_opt)) {
            SPEC_RULE("WritePlace-Ident-Path-Poison");
            return PanicStmtOut();
          }
          const std::string resolved_name =
              ent->target_opt.has_value() ? *ent->target_opt : node.name;
          const auto key = sema::PathKeyOf(module_path);
          const auto it = sigma.static_addrs.find({key, resolved_name});
          if (it == sigma.static_addrs.end()) {
            return PanicStmtOut();
          }
          if (DropOnAssignStatic(ctx, key, resolved_name)) {
            const auto dropped =
                DropStaticActionOut(ctx, key, resolved_name, sigma);
            if (dropped == DropStatus::Panic) {
              return PanicStmtOut();
            }
          }
          if (!WriteAddr(sigma, it->second, value)) {
            return PanicStmtOut();
          }
          SPEC_RULE("WritePlace-Ident-Path");
          return MakeOk();
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
          Outcome base_out = ReadPlaceSigmaImpl(ctx, *node.base, sigma);
          if (IsCtrl(base_out)) {
            return MakeCtrlOut(std::get<Control>(base_out.node));
          }
          const auto field_value =
              FieldValue(std::get<Value>(base_out.node), node.name);
          if (!field_value.has_value()) {
            return PanicStmtOut();
          }
          const auto field_type = ExprTypeOf(ctx, expr);
          const auto drop_out =
              DropSubvalue(ctx, *node.base, field_type, *field_value, sigma);
          if (drop_out == DropStatus::Panic) {
            return PanicStmtOut();
          }
          const auto updated =
              FieldUpdate(std::get<Value>(base_out.node), node.name, value);
          if (!updated.has_value()) {
            return PanicStmtOut();
          }
          const auto sout =
              WritePlaceSubSigma(ctx, *node.base, *updated, sigma);
          SPEC_RULE("WritePlace-Field");
          return sout;
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
          Outcome base_out = ReadPlaceSigmaImpl(ctx, *node.base, sigma);
          if (IsCtrl(base_out)) {
            return MakeCtrlOut(std::get<Control>(base_out.node));
          }
          auto index_value = ParseIntLiteralValue(node.index.lexeme);
          if (!index_value.has_value() ||
              !core::UInt128FitsU64(*index_value)) {
            return PanicStmtOut();
          }
          const std::size_t index =
              static_cast<std::size_t>(core::UInt128ToU64(*index_value));
          const auto elem =
              TupleValue(std::get<Value>(base_out.node), index);
          if (!elem.has_value()) {
            return PanicStmtOut();
          }
          const auto elem_type = ExprTypeOf(ctx, expr);
          const auto drop_out =
              DropSubvalue(ctx, *node.base, elem_type, *elem, sigma);
          if (drop_out == DropStatus::Panic) {
            return PanicStmtOut();
          }
          const auto updated =
              TupleUpdate(std::get<Value>(base_out.node), index, value);
          if (!updated.has_value()) {
            return PanicStmtOut();
          }
          const auto sout =
              WritePlaceSubSigma(ctx, *node.base, *updated, sigma);
          SPEC_RULE("WritePlace-Tuple");
          return sout;
        } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
          Outcome base_out = ReadPlaceSigmaImpl(ctx, *node.base, sigma);
          if (IsCtrl(base_out)) {
            return MakeCtrlOut(std::get<Control>(base_out.node));
          }
          Outcome idx_out = EvalSigma(ctx, *node.index, sigma);
          if (IsCtrl(idx_out)) {
            return MakeCtrlOut(std::get<Control>(idx_out.node));
          }
          const auto& base_val = std::get<Value>(base_out.node);
          const auto& idx_val = std::get<Value>(idx_out.node);
          if (const auto* range = std::get_if<RangeVal>(&idx_val.node)) {
            const auto len = Len(base_val);
            if (!len.has_value()) {
              return PanicStmtOut();
            }
            const auto bounds = SliceBounds(*range, *len);
            if (!bounds.has_value()) {
              SPEC_RULE("WritePlace-Index-Range-OOB");
              SetPanicReason(sigma, PanicReason::Bounds);
              return PanicStmtOut();
            }
            const auto slice_len = SliceLen(value);
            if (!slice_len.has_value()) {
              return PanicStmtOut();
            }
            const auto start = bounds->first;
            const auto end = bounds->second;
            if (*slice_len != end - start) {
              SPEC_RULE("WritePlace-Index-Range-Len");
              SetPanicReason(sigma, PanicReason::Bounds);
              return PanicStmtOut();
            }
            const auto updated = SliceUpdate(base_val, start, value);
            if (!updated.has_value()) {
              return PanicStmtOut();
            }
            const auto sout = WritePlaceSubSigma(ctx, *node.base, *updated, sigma);
            SPEC_RULE("WritePlace-Index-Range");
            return sout;
          }
          const auto idx_num = IndexNum(idx_val);
          if (!idx_num.has_value()) {
            return PanicStmtOut();
          }
          const auto len = Len(base_val);
          if (!len.has_value()) {
            return PanicStmtOut();
          }
          if (*idx_num >= *len) {
            SPEC_RULE("WritePlace-Index-OOB");
            SetPanicReason(sigma, PanicReason::Bounds);
            return PanicStmtOut();
          }
          const auto elem = IndexValue(base_val, *idx_num);
          if (!elem.has_value()) {
            return PanicStmtOut();
          }
          const auto elem_type = ExprTypeOf(ctx, expr);
          const auto drop_out =
              DropSubvalue(ctx, *node.base, elem_type, *elem, sigma);
          if (drop_out == DropStatus::Panic) {
            return PanicStmtOut();
          }
          const auto updated = IndexUpdate(base_val, *idx_num, value);
          if (!updated.has_value()) {
            return PanicStmtOut();
          }
          const auto sout =
              WritePlaceSubSigma(ctx, *node.base, *updated, sigma);
          SPEC_RULE("WritePlace-Index");
          return sout;
        } else if constexpr (std::is_same_v<T, syntax::DerefExpr>) {
          Outcome ptr_out = ReadPlaceSigmaImpl(ctx, *node.value, sigma);
          if (IsCtrl(ptr_out)) {
            return MakeCtrlOut(std::get<Control>(ptr_out.node));
          }
          const auto sout =
              WritePtrSigma(std::get<Value>(ptr_out.node), value, sigma);
          SPEC_RULE("WritePlace-Deref");
          return sout;
        }
        return PanicStmtOut();
      },
      expr.node);
}

StmtOut WritePlaceSubSigma(const SemanticsContext& ctx,
                           const syntax::Expr& expr,
                           const Value& value,
                           Sigma& sigma) {
  return std::visit(
      [&](const auto& node) -> StmtOut {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
          const auto binding = LookupBind(sigma, node.name);
          if (binding.has_value()) {
            if (!UpdateVal(sigma, *binding, value)) {
              return PanicStmtOut();
            }
            SPEC_RULE("WritePlaceSub-Ident");
            SPEC_RULE("WriteSub-Ident");
            return MakeOk();
          }
          if (!ctx.sema) {
            return PanicStmtOut();
          }
          const auto ent = sema::ResolveValueName(*ctx.sema, node.name);
          if (!ent.has_value() || !ent->origin_opt.has_value()) {
            return PanicStmtOut();
          }
          if (PoisonedModulePath(sigma, ent->origin_opt)) {
            SPEC_RULE("WritePlaceSub-Ident-Path-Poison");
            SPEC_RULE("WriteSub-Ident-Path-Poison");
            return PanicStmtOut();
          }
          const std::string resolved_name =
              ent->target_opt.has_value() ? *ent->target_opt : node.name;
          const auto key = sema::PathKeyOf(*ent->origin_opt);
          const auto it = sigma.static_addrs.find({key, resolved_name});
          if (it == sigma.static_addrs.end()) {
            return PanicStmtOut();
          }
          if (!WriteAddr(sigma, it->second, value)) {
            return PanicStmtOut();
          }
          SPEC_RULE("WritePlaceSub-Ident-Path");
          SPEC_RULE("WriteSub-Ident-Path");
          return MakeOk();
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
          Outcome base_out = ReadPlaceSigmaImpl(ctx, *node.base, sigma);
          if (IsCtrl(base_out)) {
            return MakeCtrlOut(std::get<Control>(base_out.node));
          }
          const auto updated =
              FieldUpdate(std::get<Value>(base_out.node), node.name, value);
          if (!updated.has_value()) {
            return PanicStmtOut();
          }
          const auto sout =
              WritePlaceSubSigma(ctx, *node.base, *updated, sigma);
          SPEC_RULE("WritePlaceSub-Field");
          SPEC_RULE("WriteSub-Field");
          return sout;
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
          Outcome base_out = ReadPlaceSigmaImpl(ctx, *node.base, sigma);
          if (IsCtrl(base_out)) {
            return MakeCtrlOut(std::get<Control>(base_out.node));
          }
          auto index_value = ParseIntLiteralValue(node.index.lexeme);
          if (!index_value.has_value() ||
              !core::UInt128FitsU64(*index_value)) {
            return PanicStmtOut();
          }
          const std::size_t index =
              static_cast<std::size_t>(core::UInt128ToU64(*index_value));
          const auto updated =
              TupleUpdate(std::get<Value>(base_out.node), index, value);
          if (!updated.has_value()) {
            return PanicStmtOut();
          }
          const auto sout =
              WritePlaceSubSigma(ctx, *node.base, *updated, sigma);
          SPEC_RULE("WritePlaceSub-Tuple");
          SPEC_RULE("WriteSub-Tuple");
          return sout;
        } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
          Outcome base_out = ReadPlaceSigmaImpl(ctx, *node.base, sigma);
          if (IsCtrl(base_out)) {
            return MakeCtrlOut(std::get<Control>(base_out.node));
          }
          Outcome idx_out = EvalSigma(ctx, *node.index, sigma);
          if (IsCtrl(idx_out)) {
            return MakeCtrlOut(std::get<Control>(idx_out.node));
          }
          const auto& base_val = std::get<Value>(base_out.node);
          const auto& idx_val = std::get<Value>(idx_out.node);
          if (const auto* range = std::get_if<RangeVal>(&idx_val.node)) {
            const auto len = Len(base_val);
            if (!len.has_value()) {
              return PanicStmtOut();
            }
            const auto bounds = SliceBounds(*range, *len);
            if (!bounds.has_value()) {
              SPEC_RULE("WritePlaceSub-Index-Range-OOB");
              SPEC_RULE("WriteSub-Index-Range-OOB");
              SetPanicReason(sigma, PanicReason::Bounds);
              return PanicStmtOut();
            }
            const auto slice_len = SliceLen(value);
            if (!slice_len.has_value()) {
              return PanicStmtOut();
            }
            const auto start = bounds->first;
            const auto end = bounds->second;
            if (*slice_len != end - start) {
              SPEC_RULE("WritePlaceSub-Index-Range-Len");
              SPEC_RULE("WriteSub-Index-Range-Len");
              SetPanicReason(sigma, PanicReason::Bounds);
              return PanicStmtOut();
            }
            const auto updated = SliceUpdate(base_val, start, value);
            if (!updated.has_value()) {
              return PanicStmtOut();
            }
            const auto sout =
                WritePlaceSubSigma(ctx, *node.base, *updated, sigma);
            SPEC_RULE("WritePlaceSub-Index-Range");
            SPEC_RULE("WriteSub-Index-Range");
            return sout;
          }
          const auto idx_num = IndexNum(idx_val);
          if (!idx_num.has_value()) {
            return PanicStmtOut();
          }
          const auto len = Len(base_val);
          if (!len.has_value()) {
            return PanicStmtOut();
          }
          if (*idx_num >= *len) {
            SPEC_RULE("WritePlaceSub-Index-OOB");
            SPEC_RULE("WriteSub-Index-OOB");
            SetPanicReason(sigma, PanicReason::Bounds);
            return PanicStmtOut();
          }
          const auto updated = IndexUpdate(base_val, *idx_num, value);
          if (!updated.has_value()) {
            return PanicStmtOut();
          }
          const auto sout =
              WritePlaceSubSigma(ctx, *node.base, *updated, sigma);
          SPEC_RULE("WritePlaceSub-Index");
          SPEC_RULE("WriteSub-Index");
          return sout;
        } else if constexpr (std::is_same_v<T, syntax::DerefExpr>) {
          Outcome ptr_out = ReadPlaceSigmaImpl(ctx, *node.value, sigma);
          if (IsCtrl(ptr_out)) {
            return MakeCtrlOut(std::get<Control>(ptr_out.node));
          }
          const auto sout =
              WritePtrSigma(std::get<Value>(ptr_out.node), value, sigma);
          SPEC_RULE("WritePlaceSub-Deref");
          SPEC_RULE("WriteSub-Deref");
          return sout;
        }
        return PanicStmtOut();
      },
      expr.node);
}

EvalOptResult EvalOptSigma(const SemanticsContext& ctx,
                           const syntax::ExprPtr& expr_opt,
                           Sigma& sigma) {
  if (!expr_opt) {
    SPEC_RULE("EvalOptSigma-None");
    return std::optional<Value>{};
  }
  Outcome out = EvalSigma(ctx, *expr_opt, sigma);
  if (IsCtrl(out)) {
    SPEC_RULE("EvalOptSigma-Ctrl");
    return std::get<Control>(out.node);
  }
  SPEC_RULE("EvalOptSigma-Some");
  return std::optional<Value>{std::get<Value>(out.node)};
}

EvalListResult EvalListSigma(const SemanticsContext& ctx,
                             const std::vector<syntax::ExprPtr>& exprs,
                             Sigma& sigma) {
  if (exprs.empty()) {
    SPEC_RULE("EvalListSigma-Empty");
    return std::vector<Value>{};
  }
  std::vector<Value> values;
  values.reserve(exprs.size());
  for (const auto& expr : exprs) {
    Outcome out = EvalSigma(ctx, *expr, sigma);
    if (IsCtrl(out)) {
      SPEC_RULE("EvalListSigma-Ctrl");
      return std::get<Control>(out.node);
    }
    values.push_back(std::get<Value>(out.node));
    SPEC_RULE("EvalListSigma-Cons");
  }
  return values;
}

EvalFieldInitsResult EvalFieldInitsSigma(
    const SemanticsContext& ctx,
    const std::vector<syntax::FieldInit>& fields,
    Sigma& sigma) {
  if (fields.empty()) {
    SPEC_RULE("EvalFieldInitsSigma-Empty");
    return std::vector<std::pair<std::string, Value>>{};
  }
  std::vector<std::pair<std::string, Value>> out;
  out.reserve(fields.size());
  for (const auto& field : fields) {
    Outcome value_out = EvalSigma(ctx, *field.value, sigma);
    if (IsCtrl(value_out)) {
      SPEC_RULE("EvalFieldInitsSigma-Ctrl");
      return std::get<Control>(value_out.node);
    }
    out.emplace_back(field.name, std::get<Value>(value_out.node));
    SPEC_RULE("EvalFieldInitsSigma-Cons");
  }
  return out;
}



using EvalArgsResult = std::variant<std::vector<BindingValue>, Control>;
using EvalRecvResult = std::variant<std::pair<Value, BindingValue>, Control>;

Outcome ExitOutcome(const Outcome& out, CleanupStatus status) {
  if (status == CleanupStatus::Abort) {
    Control ctrl;
    ctrl.kind = ControlKind::Abort;
    return MakeCtrl(ctrl);
  }
  if (status == CleanupStatus::Ok) {
    return out;
  }
  if (IsCtrl(out)) {
    const auto& ctrl = std::get<Control>(out.node);
    if (ctrl.kind == ControlKind::Abort) {
      return out;
    }
    if (ctrl.kind == ControlKind::Panic) {
      Control abort_ctrl;
      abort_ctrl.kind = ControlKind::Abort;
      return MakeCtrl(abort_ctrl);
    }
  }
  Control panic_ctrl;
  panic_ctrl.kind = ControlKind::Panic;
  return MakeCtrl(panic_ctrl);
}

std::optional<ScopeEntry*> MutableScopeById(Sigma& sigma, ScopeId sid) {
  for (auto& scope : sigma.scope_stack) {
    if (scope.id == sid) {
      return &scope;
    }
  }
  return std::nullopt;
}

Outcome BlockExit(const SemanticsContext& ctx,
                  ScopeId scope_id,
                  const Outcome& out,
                  Sigma& sigma) {
  auto scope_ptr = MutableScopeById(sigma, scope_id);
  if (!scope_ptr.has_value()) {
    return PanicOutcome();
  }
  const CleanupStatus cleanup = CleanupScope(ctx, **scope_ptr, sigma);
  const Outcome out2 = ExitOutcome(out, cleanup);
  if (IsCtrl(out2)) {
    const auto& ctrl = std::get<Control>(out2.node);
    if (ctrl.kind == ControlKind::Abort) {
      return out2;
    }
  }
  ScopeEntry popped;
  if (!PopScope(sigma, &popped)) {
    return PanicOutcome();
  }
  return out2;
}

Outcome ReturnOut(const Outcome& out) {
  if (!IsCtrl(out)) {
    return out;
  }
  const auto& ctrl = std::get<Control>(out.node);
  if (ctrl.kind == ControlKind::Return) {
    if (ctrl.value.has_value()) {
      return MakeVal(*ctrl.value);
    }
    return MakeVal(Value{UnitVal{}});
  }
  if (ctrl.kind == ControlKind::Panic || ctrl.kind == ControlKind::Abort) {
    return out;
  }
  return PanicOutcome();
}

BindInfo BindInfoForParam(const syntax::Param& param) {
  BindInfo info;
  info.mov = Movability::Mov;
  info.resp = param.mode.has_value() ? Responsibility::Resp
                                     : Responsibility::Alias;
  return info;
}

BindInfo BindInfoForRecv(bool move_mode) {
  BindInfo info;
  info.mov = Movability::Mov;
  info.resp = move_mode ? Responsibility::Resp
                        : Responsibility::Alias;
  return info;
}


sema::TypeRef TypeOfValue(const Value& value) {
  return std::visit(
      [&](const auto& node) -> sema::TypeRef {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, BoolVal>) {
          return sema::MakeTypePrim("bool");
        } else if constexpr (std::is_same_v<T, CharVal>) {
          return sema::MakeTypePrim("char");
        } else if constexpr (std::is_same_v<T, UnitVal>) {
          return sema::MakeTypePrim("()");
        } else if constexpr (std::is_same_v<T, IntVal>) {
          return sema::MakeTypePrim(node.type);
        } else if constexpr (std::is_same_v<T, FloatVal>) {
          return sema::MakeTypePrim(node.type);
        } else if constexpr (std::is_same_v<T, PtrVal>) {
          return sema::MakeTypePtr(nullptr, node.state);
        } else if constexpr (std::is_same_v<T, RawPtrVal>) {
          return sema::MakeTypeRawPtr(node.qual, nullptr);
        } else if constexpr (std::is_same_v<T, TupleVal>) {
          std::vector<sema::TypeRef> elems;
          elems.reserve(node.elements.size());
          for (const auto& elem : node.elements) {
            elems.push_back(TypeOfValue(elem));
          }
          return sema::MakeTypeTuple(std::move(elems));
        } else if constexpr (std::is_same_v<T, ArrayVal>) {
          sema::TypeRef elem_ty = nullptr;
          if (!node.elements.empty()) {
            elem_ty = TypeOfValue(node.elements.front());
          }
          return sema::MakeTypeArray(elem_ty, node.elements.size());
        } else if constexpr (std::is_same_v<T, RecordVal>) {
          return node.record_type;
        } else if constexpr (std::is_same_v<T, EnumVal>) {
          return sema::MakeTypePath(node.path);
        } else if constexpr (std::is_same_v<T, ModalVal>) {
          return nullptr;
        } else if constexpr (std::is_same_v<T, UnionVal>) {
          return node.member;
        } else if constexpr (std::is_same_v<T, DynamicVal>) {
          return sema::MakeTypeDynamic(node.class_path);
        } else if constexpr (std::is_same_v<T, StringVal>) {
          return sema::MakeTypeString(node.state);
        } else if constexpr (std::is_same_v<T, BytesVal>) {
          return sema::MakeTypeBytes(node.state);
        } else if constexpr (std::is_same_v<T, ProcRefVal>) {
          return nullptr;
        } else if constexpr (std::is_same_v<T, RecordCtorVal>) {
          return nullptr;
        } else if constexpr (std::is_same_v<T, RangeVal>) {
          return sema::MakeTypeRange();
        }
        return nullptr;
      },
      value.node);
}

struct MatchArmResult {
  bool matched = false;
  Outcome out;
};

struct IterState {
  Value value;
  std::size_t index = 0;
};

struct MethodCallInfo {
  enum class Kind {
    Method,
    Transition,
  };
  Kind kind = Kind::Method;
  MethodTarget target;
  std::vector<syntax::Param> params;
  const syntax::TransitionDecl* transition = nullptr;
  sema::TypePath modal_path;
};

const syntax::ProcedureDecl* FindProcedureDecl(const sema::ScopeContext& ctx,
                                               const sema::TypePath& module,
                                               std::string_view name) {
  for (const auto& mod : ctx.sigma.mods) {
    if (sema::PathKeyOf(mod.path) != module) {
      continue;
    }
    for (const auto& item : mod.items) {
      const auto* proc = std::get_if<syntax::ProcedureDecl>(&item);
      if (!proc) {
        continue;
      }
      if (sema::IdEq(proc->name, name)) {
        return proc;
      }
    }
  }
  return nullptr;
}

EvalArgsResult EvalArgsSigma(const SemanticsContext& ctx,
                             const std::vector<syntax::Param>& params,
                             const std::vector<syntax::Arg>& args,
                             Sigma& sigma) {
  if (params.size() != args.size()) {
    return PanicCtrl();
  }
  if (params.empty()) {
    SPEC_RULE("EvalArgsSigma-Empty");
    return std::vector<BindingValue>{};
  }
  std::vector<BindingValue> values;
  values.reserve(args.size());
  for (std::size_t i = 0; i < params.size(); ++i) {
    const auto& param = params[i];
    const auto& arg = args[i];
    if (param.mode.has_value()) {
      Outcome out;
      if (arg.moved && arg.value && IsPlaceExpr(*arg.value)) {
        out = MovePlaceSigmaImpl(ctx, *arg.value, sigma);
      } else if (arg.value) {
        out = EvalSigma(ctx, *arg.value, sigma);
      } else {
        return PanicCtrl();
      }
      if (IsCtrl(out)) {
        SPEC_RULE("EvalArgsSigma-Ctrl-Move");
        return std::get<Control>(out.node);
      }
      values.push_back(std::get<Value>(out.node));
      SPEC_RULE("EvalArgsSigma-Cons-Move");
      continue;
    }
    if (!arg.value) {
      return PanicCtrl();
    }
    AddrResult addr_out = AddrOfSigmaImpl(ctx, *arg.value, sigma);
    if (IsAddrCtrl(addr_out)) {
      SPEC_RULE("EvalArgsSigma-Ctrl-Ref");
      return std::get<Control>(addr_out);
    }
    values.push_back(Alias{std::get<Addr>(addr_out)});
    SPEC_RULE("EvalArgsSigma-Cons-Ref");
  }
  return values;
}

EvalRecvResult EvalRecvSigma(const SemanticsContext& ctx,
                             const syntax::Expr& base,
                             bool move_mode,
                             Sigma& sigma) {
  if (move_mode) {
    Outcome out = EvalSigma(ctx, base, sigma);
    if (IsCtrl(out)) {
      SPEC_RULE("EvalRecvSigma-Ctrl-Move");
      return std::get<Control>(out.node);
    }
    const auto value = std::get<Value>(out.node);
    SPEC_RULE("EvalRecvSigma-Move");
    return std::make_pair(value, BindingValue{value});
  }

  AddrResult addr_out = AddrOfSigmaImpl(ctx, base, sigma);
  if (IsAddrCtrl(addr_out)) {
    SPEC_RULE("EvalRecvSigma-Ctrl-Ref");
    return std::get<Control>(addr_out);
  }
  const Addr addr = std::get<Addr>(addr_out);
  const auto value = ReadAddr(sigma, addr);
  if (!value.has_value()) {
    return PanicCtrl();
  }
  if (const auto* dyn = std::get_if<DynamicVal>(&value->node)) {
    const auto tag = AddrTag(sigma, dyn->data.addr);
    if (!tag.has_value() || !TagActive(sigma, *tag)) {
      SPEC_RULE("EvalRecvSigma-Ref-Dyn-Expired");
      return PanicCtrl();
    }
    SPEC_RULE("EvalRecvSigma-Ref-Dyn");
    return std::make_pair(*value, BindingValue{Alias{dyn->data.addr}});
  }
  SPEC_RULE("EvalRecvSigma-Ref");
  return std::make_pair(*value, BindingValue{Alias{addr}});
}

Outcome EvalArmBodySigma(const SemanticsContext& ctx,
                         const syntax::Expr& body,
                         Sigma& sigma) {
  if (const auto* block = std::get_if<syntax::BlockExpr>(&body.node)) {
    SemanticsContext inner_ctx = ctx;
    inner_ctx.temp_ctx = nullptr;
    Outcome out = EvalBlockSigma(inner_ctx, *block->block, sigma);
    SPEC_RULE("EvalArmBody-Block");
    return out;
  }
  Outcome out = EvalSigma(ctx, body, sigma);
  SPEC_RULE("EvalArmBody-Expr");
  return out;
}

MatchArmResult MatchArmSigma(const SemanticsContext& ctx,
                             const syntax::MatchArm& arm,
                             const Value& value,
                             Sigma& sigma) {
  MatchArmResult result;
  const auto env = MatchPattern(ctx, *arm.pattern, value);
  if (!env.has_value()) {
    SPEC_RULE("MatchArm-Fail");
    result.matched = false;
    return result;
  }
  const auto binds = BindOrder(*arm.pattern, *env);
  ScopeId scope = 0;
  if (!BlockEnter(sigma, binds, BindInfo{}, &scope)) {
    result.matched = true;
    result.out = PanicOutcome();
    return result;
  }
  if (arm.guard_opt) {
    Outcome guard_out = EvalSigma(ctx, *arm.guard_opt, sigma);
    if (IsCtrl(guard_out)) {
      Outcome out = BlockExit(ctx, scope, guard_out, sigma);
      SPEC_RULE("MatchArm-Ctrl");
      result.matched = true;
      result.out = out;
      return result;
    }
    const auto guard_val = std::get<Value>(guard_out.node);
    const auto* guard_bool = std::get_if<BoolVal>(&guard_val.node);
    if (!guard_bool) {
      Outcome out = BlockExit(ctx, scope, PanicOutcome(), sigma);
      result.matched = true;
      result.out = out;
      return result;
    }
    if (!guard_bool->value) {
      (void)BlockExit(ctx, scope, MakeVal(Value{UnitVal{}}), sigma);
      SPEC_RULE("MatchArm-Guard-False");
      result.matched = false;
      return result;
    }
    Outcome body_out = EvalArmBodySigma(ctx, *arm.body, sigma);
    Outcome out = BlockExit(ctx, scope, body_out, sigma);
    SPEC_RULE("MatchArm-Guard-True");
    result.matched = true;
    result.out = out;
    return result;
  }
  Outcome body_out = EvalArmBodySigma(ctx, *arm.body, sigma);
  Outcome out = BlockExit(ctx, scope, body_out, sigma);
  SPEC_RULE("MatchArm-NoGuard");
  result.matched = true;
  result.out = out;
  return result;
}

Outcome MatchArmsSigma(const SemanticsContext& ctx,
                       const std::vector<syntax::MatchArm>& arms,
                       const Value& value,
                       Sigma& sigma) {
  for (const auto& arm : arms) {
    const auto res = MatchArmSigma(ctx, arm, value, sigma);
    if (res.matched) {
      SPEC_RULE("MatchArms-Head");
      return res.out;
    }
    SPEC_RULE("MatchArms-Tail");
  }
  return PanicOutcome();
}

std::optional<IterState> IterInit(const Value& value) {
  const auto len = Len(value);
  if (!len.has_value()) {
    return std::nullopt;
  }
  IterState out;
  out.value = value;
  out.index = 0;
  return out;
}

std::pair<std::optional<Value>, IterState> IterNext(const IterState& it) {
  const auto len = Len(it.value);
  if (!len.has_value() || it.index >= *len) {
    return {std::nullopt, it};
  }
  const auto elem = IndexValue(it.value, it.index);
  if (!elem.has_value()) {
    return {std::nullopt, it};
  }
  IterState next = it;
  next.index = it.index + 1;
  return {elem, next};
}

Outcome LoopIterExecSigma(const SemanticsContext& ctx,
                          const syntax::Pattern& pattern,
                          const syntax::Block& body,
                          const IterState& it,
                          Sigma& sigma) {
  const auto next = IterNext(it);
  if (!next.first.has_value()) {
    SPEC_RULE("LoopIter-Done");
    return MakeVal(Value{UnitVal{}});
  }
  SemanticsContext inner_ctx = ctx;
  inner_ctx.temp_ctx = nullptr;
  Outcome out = EvalBlockBindSigma(inner_ctx, pattern, *next.first, body,
                                   BindInfo{}, sigma);
  if (IsCtrl(out)) {
    const auto& ctrl = std::get<Control>(out.node);
    if (ctrl.kind == ControlKind::Continue) {
      SPEC_RULE("LoopIter-Step-Continue");
      return LoopIterExecSigma(ctx, pattern, body, next.second, sigma);
    }
    if (ctrl.kind == ControlKind::Break) {
      SPEC_RULE("LoopIter-Step-Break");
      return MakeVal(BreakVal(ctrl.value));
    }
    if (ctrl.kind == ControlKind::Return ||
        ctrl.kind == ControlKind::Panic ||
        ctrl.kind == ControlKind::Abort) {
      SPEC_RULE("LoopIter-Step-Ctrl");
      return out;
    }
    return PanicOutcome();
  }
  SPEC_RULE("LoopIter-Step-Val");
  return LoopIterExecSigma(ctx, pattern, body, next.second, sigma);
}

Outcome ApplyTransitionSigma(const SemanticsContext& ctx,
                             const sema::TypePath& modal_path,
                             const syntax::TransitionDecl& transition,
                             const Value& self_value,
                             const BindingValue& self_arg,
                             const std::vector<BindingValue>& args,
                             Sigma& sigma) {
  if (sema::IdEq(transition.name, "close") &&
      modal_path.size() == 1 &&
      (modal_path[0] == "File" || modal_path[0] == "DirIter")) {
    const auto prim = PrimCall(modal_path, transition.name, self_value, {}, sigma);
    if (!prim.has_value()) {
      return PanicOutcome();
    }
    return MakeVal(*prim);
  }

  if (transition.params.size() != args.size()) {
    return PanicOutcome();
  }
  ScopeEntry scope;
  if (!PushScope(sigma, &scope)) {
    return PanicOutcome();
  }
  const ScopeId scope_id = scope.id;
  const BindInfo self_info = BindInfoForRecv(true);
  if (!BindVal(sigma, "self", self_arg, self_info, nullptr)) {
    return BlockExit(ctx, scope_id, PanicOutcome(), sigma);
  }
  for (std::size_t i = 0; i < transition.params.size(); ++i) {
    const auto& param = transition.params[i];
    const BindInfo info = BindInfoForParam(param);
    if (!BindVal(sigma, param.name, args[i], info, nullptr)) {
      return BlockExit(ctx, scope_id, PanicOutcome(), sigma);
    }
  }

  SemanticsContext inner = ctx;
  inner.ret_type = sema::MakeTypeModalState(modal_path,
                                            transition.target_state);
  Outcome out = EvalBlockBodySigma(inner, *transition.body, sigma);
  Outcome out2 = BlockExit(inner, scope_id, out, sigma);
  return ReturnOut(out2);
}

std::optional<MethodCallInfo> ResolveMethodCallInfo(
    const SemanticsContext& ctx,
    const sema::TypeRef& recv_type,
    const Value& self_value,
    std::string_view name) {
  if (!ctx.sema) {
    return std::nullopt;
  }
  MethodCallInfo info;
  
  if (const auto* dyn = std::get_if<DynamicVal>(&self_value.node)) {
    const auto* class_method =
        sema::LookupClassMethod(*ctx.sema, dyn->class_path, name);
    if (!class_method) {
      if (dyn->class_path.size() == 1 && sema::IdEq(dyn->class_path[0], "FileSystem")) {
        const auto sig = sema::LookupFileSystemMethodSig(name);
        if (!sig.has_value()) {
          return std::nullopt;
        }
        info.kind = MethodCallInfo::Kind::Method;
        info.target.kind = MethodTarget::Kind::Class;
        info.target.owner_class = dyn->class_path;
        info.params = sig->params;
        return info;
      }
      if (dyn->class_path.size() == 1 && sema::IdEq(dyn->class_path[0], "HeapAllocator")) {
        const auto sig = sema::LookupHeapAllocatorMethodSig(name);
        if (!sig.has_value()) {
          return std::nullopt;
        }
        info.kind = MethodCallInfo::Kind::Method;
        info.target.kind = MethodTarget::Kind::Class;
        info.target.owner_class = dyn->class_path;
        info.params = sig->params;
        return info;
      }
      return std::nullopt;
    }
    const auto stripped = sema::StripPerm(dyn->concrete_type);
    const auto lookup =
        stripped ? sema::LookupMethodStatic(*ctx.sema, stripped, name)
                 : sema::StaticMethodLookup{};
    if (lookup.record_method) {
      info.kind = MethodCallInfo::Kind::Method;
      info.target.kind = MethodTarget::Kind::Record;
      info.target.record_method = lookup.record_method;
      info.params = lookup.record_method->params;
      return info;
    }
    info.kind = MethodCallInfo::Kind::Method;
    info.target.kind = MethodTarget::Kind::Class;
    info.target.class_method = class_method;
    info.target.owner_class = dyn->class_path;
    info.params = class_method->params;
    return info;
  }

  sema::TypeRef base_type = recv_type;
  if (!base_type) {
    base_type = TypeOfValue(self_value);
  }
  const auto stripped = sema::StripPerm(base_type);
  if (!stripped) {
    return std::nullopt;
  }

  if (const auto* modal = std::get_if<sema::TypeModalState>(&stripped->node)) {
    const auto* decl = sema::LookupModalDecl(*ctx.sema, modal->path);
    if (!decl) {
      return std::nullopt;
    }
    if (const auto* transition =
            sema::LookupTransitionDecl(*decl, modal->state, name)) {
      info.kind = MethodCallInfo::Kind::Transition;
      info.transition = transition;
      info.params = transition->params;
      info.modal_path = modal->path;
      return info;
    }
    if (const auto* method =
            sema::LookupStateMethodDecl(*decl, modal->state, name)) {
      info.kind = MethodCallInfo::Kind::Method;
      info.target.kind = MethodTarget::Kind::State;
      info.target.state_method = method;
      info.params = method->params;
      return info;
    }
    return std::nullopt;
  }


  if (const auto* path = std::get_if<sema::TypePathType>(&stripped->node)) {
    if (path->path.size() == 1 && sema::IdEq(path->path[0], "System")) {
      const auto sig = sema::LookupSystemMethodSig(name);
      if (!sig.has_value()) {
        return std::nullopt;
      }
      info.kind = MethodCallInfo::Kind::Method;
      info.target.kind = MethodTarget::Kind::Class;
      info.target.owner_class = {"System"};
      info.params = sig->params;
      return info;
    }
  }

  const auto lookup = sema::LookupMethodStatic(*ctx.sema, stripped, name);
  if (!lookup.ok) {
    return std::nullopt;
  }
  if (lookup.record_method) {
    info.kind = MethodCallInfo::Kind::Method;
    info.target.kind = MethodTarget::Kind::Record;
    info.target.record_method = lookup.record_method;
    info.params = lookup.record_method->params;
    return info;
  }
  if (lookup.class_method) {
    info.kind = MethodCallInfo::Kind::Method;
    info.target.kind = MethodTarget::Kind::Class;
    info.target.class_method = lookup.class_method;
    info.target.owner_class = lookup.owner_class;
    info.params = lookup.class_method->params;
    return info;
  }
  return std::nullopt;
}

std::optional<std::vector<syntax::Param>> BuiltinParams(
    const sema::TypePath& module_path,
    std::string_view name) {
  auto make_param = [](bool move) {
    syntax::Param param{};
    param.mode = move ? std::optional<syntax::ParamMode>(syntax::ParamMode::Move)
                      : std::nullopt;
    param.name = "";
    param.type = nullptr;
    return param;
  };
  auto make_params = [&](std::initializer_list<bool> moves) {
    std::vector<syntax::Param> params;
    params.reserve(moves.size());
    for (const bool move : moves) {
      params.push_back(make_param(move));
    }
    return params;
  };
  if (module_path.size() == 1 &&
      sema::IdEq(module_path[0], "string") &&
      sema::IsStringBuiltinName(name)) {
    if (name == "from") return make_params({true, true});
    if (name == "as_view") return make_params({true});
    if (name == "to_managed") return make_params({true, true});
    if (name == "clone_with") return make_params({true, true});
    if (name == "append") return make_params({false, true, true});
    if (name == "length") return make_params({true});
    if (name == "is_empty") return make_params({true});
  }
  if (module_path.size() == 1 &&
      sema::IdEq(module_path[0], "bytes") &&
      sema::IsBytesBuiltinName(name)) {
    if (name == "with_capacity") return make_params({true, true});
    if (name == "from_slice") return make_params({true, true});
    if (name == "as_view") return make_params({true});
    if (name == "to_managed") return make_params({true, true});
    if (name == "view") return make_params({true});
    if (name == "view_string") return make_params({true});
    if (name == "append") return make_params({false, true, true});
    if (name == "length") return make_params({true});
    if (name == "is_empty") return make_params({true});
  }
  if (module_path.size() == 1 &&
      sema::IdEq(module_path[0], "Region") &&
      IsRegionProcName(name)) {
    if (name == "new_scoped") return make_params({false});
    if (name == "alloc") return make_params({false, false});
    if (name == "reset_unchecked") return make_params({false});
    if (name == "freeze") return make_params({false});
    if (name == "thaw") return make_params({false});
    if (name == "free_unchecked") return make_params({false});
  }
  return std::nullopt;
}

Outcome EvalCallSigma(const SemanticsContext& ctx,
                      const syntax::Expr& callee,
                      const std::vector<syntax::Arg>& args,
                      Sigma& sigma) {
  Outcome callee_out = EvalSigma(ctx, callee, sigma);
  if (IsCtrl(callee_out)) {
    SPEC_RULE("EvalSigma-Call-Ctrl");
    return callee_out;
  }
  const auto callee_val = std::get<Value>(callee_out.node);
  if (const auto* proc_ref = std::get_if<ProcRefVal>(&callee_val.node)) {
    if (!ctx.sema) {
      return PanicOutcome();
    }
    const auto builtin_params = BuiltinParams(proc_ref->module_path, proc_ref->name);
    if (builtin_params.has_value()) {
      const bool is_region_proc =
          proc_ref->module_path.size() == 1 &&
          sema::IdEq(proc_ref->module_path[0], "Region") &&
          IsRegionProcName(proc_ref->name);
      auto args_out = EvalArgsSigma(ctx, *builtin_params, args, sigma);
      if (std::holds_alternative<Control>(args_out)) {
        if (is_region_proc) {
          SPEC_RULE("EvalSigma-Call-RegionProc-Ctrl-Args");
        } else {
          SPEC_RULE("EvalSigma-Call-Ctrl-Args");
        }
        return MakeCtrl(std::get<Control>(args_out));
      }
      const auto& arg_values = std::get<std::vector<BindingValue>>(args_out);
      if (is_region_proc) {
        Outcome out = ApplyRegionProc(ctx, proc_ref->name, arg_values, sigma);
        SPEC_RULE("EvalSigma-Call-RegionProc");
        return out;
      }
      const auto result = BuiltinCall(proc_ref->module_path, proc_ref->name, arg_values, sigma);
      if (!result.has_value()) {
        return PanicOutcome();
      }
      SPEC_RULE("EvalSigma-Call-Proc");
      return MakeVal(*result);
    }
    const auto* proc = FindProcedureDecl(*ctx.sema,
                                         proc_ref->module_path,
                                         proc_ref->name);
    if (!proc) {
      return PanicOutcome();
    }
    auto args_out = EvalArgsSigma(ctx, proc->params, args, sigma);
    if (std::holds_alternative<Control>(args_out)) {
      SPEC_RULE("EvalSigma-Call-Ctrl-Args");
      return MakeCtrl(std::get<Control>(args_out));
    }
    const auto& arg_values = std::get<std::vector<BindingValue>>(args_out);
    Outcome out = ApplyProcSigma(ctx, *proc, arg_values, sigma);
    SPEC_RULE("EvalSigma-Call-Proc");
    return out;
  }
  if (const auto* ctor = std::get_if<RecordCtorVal>(&callee_val.node)) {
    auto args_out = EvalArgsSigma(ctx, {}, args, sigma);
    if (std::holds_alternative<Control>(args_out)) {
      SPEC_RULE("EvalSigma-Call-Ctrl-Args");
      return MakeCtrl(std::get<Control>(args_out));
    }
    if (!std::get<std::vector<BindingValue>>(args_out).empty()) {
      return PanicOutcome();
    }
    Outcome out = ApplyRecordCtorSigma(ctx, ctor->path, sigma);
    SPEC_RULE("EvalSigma-Call-Record");
    return out;
  }
  return PanicOutcome();
}

Outcome EvalMethodCallSigma(const SemanticsContext& ctx,
                            const syntax::Expr& base,
                            std::string_view name,
                            const std::vector<syntax::Arg>& args,
                            Sigma& sigma) {
  const bool move_mode = std::holds_alternative<syntax::MoveExpr>(base.node);
  auto recv_out = EvalRecvSigma(ctx, base, move_mode, sigma);
  if (std::holds_alternative<Control>(recv_out)) {
    SPEC_RULE("EvalSigma-MethodCall-Ctrl");
    return MakeCtrl(std::get<Control>(recv_out));
  }
  const auto recv_pair = std::get<std::pair<Value, BindingValue>>(recv_out);
  const auto self_value = recv_pair.first;
  const auto self_arg = recv_pair.second;

  const auto recv_type = ExprTypeOf(ctx, base);
  const auto info = ResolveMethodCallInfo(ctx, recv_type, self_value, name);
  if (!info.has_value()) {
    return PanicOutcome();
  }

  auto args_out = EvalArgsSigma(ctx, info->params, args, sigma);
  if (std::holds_alternative<Control>(args_out)) {
    SPEC_RULE("EvalSigma-MethodCall-Ctrl-Args");
    return MakeCtrl(std::get<Control>(args_out));
  }
  const auto& arg_values = std::get<std::vector<BindingValue>>(args_out);

  Outcome out = PanicOutcome();
  if (info->kind == MethodCallInfo::Kind::Transition) {
    out = ApplyTransitionSigma(ctx, info->modal_path,
                               *info->transition, self_value,
                               self_arg, arg_values, sigma);
  } else {
    out = ApplyMethodSigma(ctx, base, name, self_value, self_arg,
                           arg_values, info->target, sigma);
  }
  SPEC_RULE("EvalSigma-MethodCall");
  return out;
}


Outcome EvalIfSigma(const SemanticsContext& ctx,
                    const syntax::Expr& cond,
                    const syntax::Expr& then_expr,
                    const syntax::ExprPtr& else_expr,
                    Sigma& sigma) {
  Outcome cond_out = EvalSigma(ctx, cond, sigma);
  if (IsCtrl(cond_out)) {
    SPEC_RULE("EvalSigma-If-Ctrl");
    return cond_out;
  }
  const auto cond_val = std::get<Value>(cond_out.node);
  const auto* cond_bool = std::get_if<BoolVal>(&cond_val.node);
  if (!cond_bool) {
    return PanicOutcome();
  }
  if (cond_bool->value) {
    Outcome out = EvalSigma(ctx, then_expr, sigma);
    SPEC_RULE("EvalSigma-If-True");
    return out;
  }
  if (!else_expr) {
    SPEC_RULE("EvalSigma-If-False-None");
    return MakeVal(Value{UnitVal{}});
  }
  Outcome out = EvalSigma(ctx, *else_expr, sigma);
  SPEC_RULE("EvalSigma-If-False-Some");
  return out;
}

Outcome EvalMatchSigma(const SemanticsContext& ctx,
                       const syntax::Expr& scrutinee,
                       const std::vector<syntax::MatchArm>& arms,
                       Sigma& sigma) {
  Outcome scrut_out = EvalSigma(ctx, scrutinee, sigma);
  if (IsCtrl(scrut_out)) {
    SPEC_RULE("EvalSigma-Match-Ctrl");
    return scrut_out;
  }
  const auto scrut_val = std::get<Value>(scrut_out.node);
  Outcome out = MatchArmsSigma(ctx, arms, scrut_val, sigma);
  SPEC_RULE("EvalSigma-Match");
  return out;
}

Outcome EvalLoopInfiniteSigma(const SemanticsContext& ctx,
                              const syntax::Block& body,
                              Sigma& sigma) {
  SemanticsContext inner_ctx = ctx;
  inner_ctx.temp_ctx = nullptr;
  Outcome body_out = EvalBlockSigma(inner_ctx, body, sigma);
  if (IsCtrl(body_out)) {
    const auto& ctrl = std::get<Control>(body_out.node);
    if (ctrl.kind == ControlKind::Continue) {
      SPEC_RULE("EvalSigma-Loop-Infinite-Continue");
      return EvalLoopInfiniteSigma(ctx, body, sigma);
    }
    if (ctrl.kind == ControlKind::Break) {
      SPEC_RULE("EvalSigma-Loop-Infinite-Break");
      return MakeVal(BreakVal(ctrl.value));
    }
    if (ctrl.kind == ControlKind::Return ||
        ctrl.kind == ControlKind::Panic ||
        ctrl.kind == ControlKind::Abort) {
      SPEC_RULE("EvalSigma-Loop-Infinite-Ctrl");
      return body_out;
    }
    return PanicOutcome();
  }
  SPEC_RULE("EvalSigma-Loop-Infinite-Step");
  return EvalLoopInfiniteSigma(ctx, body, sigma);
}

Outcome EvalLoopConditionalSigma(const SemanticsContext& ctx,
                                 const syntax::Expr& cond,
                                 const syntax::Block& body,
                                 Sigma& sigma) {
  Outcome cond_out = EvalSigma(ctx, cond, sigma);
  if (IsCtrl(cond_out)) {
    SPEC_RULE("EvalSigma-Loop-Cond-Ctrl");
    return cond_out;
  }
  const auto cond_val = std::get<Value>(cond_out.node);
  const auto* cond_bool = std::get_if<BoolVal>(&cond_val.node);
  if (!cond_bool) {
    return PanicOutcome();
  }
  if (!cond_bool->value) {
    SPEC_RULE("EvalSigma-Loop-Cond-False");
    return MakeVal(Value{UnitVal{}});
  }
  SemanticsContext inner_ctx = ctx;
  inner_ctx.temp_ctx = nullptr;
  Outcome body_out = EvalBlockSigma(inner_ctx, body, sigma);
  if (IsCtrl(body_out)) {
    const auto& ctrl = std::get<Control>(body_out.node);
    if (ctrl.kind == ControlKind::Continue) {
      SPEC_RULE("EvalSigma-Loop-Cond-Continue");
      return EvalLoopConditionalSigma(ctx, cond, body, sigma);
    }
    if (ctrl.kind == ControlKind::Break) {
      SPEC_RULE("EvalSigma-Loop-Cond-Break");
      return MakeVal(BreakVal(ctrl.value));
    }
    if (ctrl.kind == ControlKind::Return ||
        ctrl.kind == ControlKind::Panic ||
        ctrl.kind == ControlKind::Abort) {
      SPEC_RULE("EvalSigma-Loop-Cond-Body-Ctrl");
      return body_out;
    }
    return PanicOutcome();
  }
  SPEC_RULE("EvalSigma-Loop-Cond-True-Step");
  return EvalLoopConditionalSigma(ctx, cond, body, sigma);
}

Outcome EvalLoopIterSigma(const SemanticsContext& ctx,
                          const syntax::Pattern& pattern,
                          const syntax::Expr& iter,
                          const syntax::Block& body,
                          Sigma& sigma) {
  Outcome iter_out = EvalSigma(ctx, iter, sigma);
  if (IsCtrl(iter_out)) {
    SPEC_RULE("EvalSigma-Loop-Iter-Ctrl");
    return iter_out;
  }
  const auto iter_val = std::get<Value>(iter_out.node);
  const auto init = IterInit(iter_val);
  if (!init.has_value()) {
    return PanicOutcome();
  }
  Outcome out = LoopIterExecSigma(ctx, pattern, body, *init, sigma);
  SPEC_RULE("EvalSigma-Loop-Iter");
  return out;
}
Outcome EvalSigma(const SemanticsContext& ctx,
                  const syntax::Expr& expr,
                  Sigma& sigma) {
  TempContext* temp_ctx = ctx.temp_ctx;
  bool suppress = false;
  if (temp_ctx) {
    temp_ctx->depth += 1;
    const auto depth = temp_ctx->depth;
    suppress = temp_ctx->suppress_depth.has_value() &&
               *temp_ctx->suppress_depth == depth;
    if (suppress) {
      temp_ctx->suppress_depth.reset();
    }
  }

  const auto expr_type = ExprTypeOf(ctx, expr);
  Outcome out = std::visit(
      [&](const auto& node) -> Outcome {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::LiteralExpr>) {
          auto value = LiteralValue(node.literal, expr_type);
          if (!value.has_value() && !expr_type) {
            if (const auto inferred = InferLiteralType(node.literal)) {
              value = LiteralValue(node.literal, *inferred);
            }
          }
          if (!value.has_value()) {
            return PanicOutcome();
          }
          SPEC_RULE("EvalSigma-Literal");
          SPEC_RULE("StepSigma-Pure");
          return MakeVal(*value);
        } else if constexpr (std::is_same_v<T, syntax::PtrNullExpr>) {
          PtrVal ptr;
          ptr.state = sema::PtrState::Null;
          ptr.addr = 0;
          SPEC_RULE("EvalSigma-PtrNull");
          return MakeVal(Value{ptr});
        } else if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
          const auto binding = LookupBind(sigma, node.name);
          if (!binding.has_value() && ctx.sema) {
            const auto ent = sema::ResolveValueName(*ctx.sema, node.name);
            if (ent.has_value() && ent->origin_opt.has_value()) {
              if (PoisonedModulePath(sigma, ent->origin_opt)) {
                SPEC_RULE("EvalSigma-Ident-Poison");
                return PanicOutcome();
              }
            } else {
              const auto type_ent = sema::ResolveTypeName(*ctx.sema, node.name);
              if (type_ent.has_value() && type_ent->origin_opt.has_value() &&
                  PoisonedModulePath(sigma, type_ent->origin_opt)) {
                SPEC_RULE("EvalSigma-Ident-Poison-RecordCtor");
                return PanicOutcome();
              }
            }
          }
          const auto value = LookupVal(ctx, sigma, node.name);
          if (value.has_value()) {
            SPEC_RULE("EvalSigma-Ident");
            return MakeVal(*value);
          }
          return PanicOutcome();
        } else if constexpr (std::is_same_v<T, syntax::PathExpr>) {
          const auto poison_out =
              EvalPathPoison(ctx, sigma, node.path, node.name);
          if (IsCtrl(poison_out)) {
            return poison_out;
          }
          const auto path_key = sema::PathKeyOf(node.path);
          const auto value = LookupValPath(ctx, sigma, path_key, node.name);
          if (value.has_value()) {
            SPEC_RULE("EvalSigma-Path");
            return MakeVal(*value);
          }
          return PanicOutcome();
        } else if constexpr (std::is_same_v<T, syntax::ErrorExpr>) {
          SPEC_RULE("EvalSigma-ErrorExpr");
          SetPanicReason(sigma, PanicReason::ErrorExpr);
          return PanicOutcome();
        } else if constexpr (std::is_same_v<T, syntax::UnaryExpr>) {
          Outcome value_out = EvalSigma(ctx, *node.value, sigma);
          if (IsCtrl(value_out)) {
            SPEC_RULE("EvalSigma-Unary-Ctrl");
            return value_out;
          }
          const auto value =
              EvalUnOp(node.op, std::get<Value>(value_out.node));
          if (!value.has_value()) {
            SPEC_RULE("EvalSigma-Unary-Panic");
            SetPanicReason(sigma, UnOpPanicReason(node.op,
                                                 std::get<Value>(value_out.node)));
            return PanicOutcome();
          }
          SPEC_RULE("EvalSigma-Unary");
          SPEC_RULE("StepSigma-Stateful-Other");
          return MakeVal(*value);
        } else if constexpr (std::is_same_v<T, syntax::BinaryExpr>) {
          const std::string_view op = node.op;
          Outcome lhs_out = EvalSigma(ctx, *node.lhs, sigma);
          if (IsCtrl(lhs_out)) {
            SPEC_RULE("EvalSigma-Bin-Ctrl-L");
            return lhs_out;
          }
          const auto& lhs_val = std::get<Value>(lhs_out.node);
          if (op == "&&") {
            const auto* lhs_bool = std::get_if<BoolVal>(&lhs_val.node);
            if (!lhs_bool) {
              return PanicOutcome();
            }
            if (!lhs_bool->value) {
              SPEC_RULE("EvalSigma-Bin-And-False");
              return MakeVal(Value{BoolVal{false}});
            }
            Outcome rhs_out = EvalSigma(ctx, *node.rhs, sigma);
            SPEC_RULE("EvalSigma-Bin-And-True");
            return rhs_out;
          }
          if (op == "||") {
            const auto* lhs_bool = std::get_if<BoolVal>(&lhs_val.node);
            if (!lhs_bool) {
              return PanicOutcome();
            }
            if (lhs_bool->value) {
              SPEC_RULE("EvalSigma-Bin-Or-True");
              return MakeVal(Value{BoolVal{true}});
            }
            Outcome rhs_out = EvalSigma(ctx, *node.rhs, sigma);
            SPEC_RULE("EvalSigma-Bin-Or-False");
            return rhs_out;
          }
          Outcome rhs_out = EvalSigma(ctx, *node.rhs, sigma);
          if (IsCtrl(rhs_out)) {
            SPEC_RULE("EvalSigma-Bin-Ctrl-R");
            return rhs_out;
          }
          const auto& rhs_val = std::get<Value>(rhs_out.node);
          const auto value = EvalBinOpImpl(op, lhs_val, rhs_val);
          if (!value.has_value()) {
            SPEC_RULE("EvalSigma-Binary-Panic");
            SetPanicReason(sigma, BinOpPanicReason(op, lhs_val, rhs_val));
            return PanicOutcome();
          }
          SPEC_RULE("EvalSigma-Binary");
          return MakeVal(*value);
        
        } else if constexpr (std::is_same_v<T, syntax::CastExpr>) {
          if (expr_type) {
            if (const auto* dyn = std::get_if<sema::TypeDynamic>(&expr_type->node)) {
              if (node.value && IsPlaceExpr(*node.value)) {
                AddrResult addr_out = AddrOfSigmaImpl(ctx, *node.value, sigma);
                if (IsAddrCtrl(addr_out)) {
                  SPEC_RULE("EvalSigma-Dynamic-Form-Ctrl");
                  return MakeCtrl(std::get<Control>(addr_out));
                }
                const auto source_type = ExprTypeOf(ctx, *node.value);
                const auto stripped = sema::StripPerm(source_type);
                if (!stripped) {
                  return PanicOutcome();
                }
                RawPtrVal raw;
                raw.qual = sema::RawPtrQual::Imm;
                raw.addr = std::get<Addr>(addr_out);
                DynamicVal dyn_val;
                dyn_val.class_path = dyn->path;
                dyn_val.data = raw;
                dyn_val.concrete_type = stripped;
                SPEC_RULE("EvalSigma-Dynamic-Form");
                return MakeVal(Value{dyn_val});
              }
            }
          }
          Outcome value_out = EvalSigma(ctx, *node.value, sigma);
          if (IsCtrl(value_out)) {
            SPEC_RULE("EvalSigma-Cast-Ctrl");
            return value_out;
          }
          const auto source_type = ExprTypeOf(ctx, *node.value);
          const auto value =
              EvalCastVal(source_type, expr_type, std::get<Value>(value_out.node));
          if (!value.has_value()) {
            SPEC_RULE("EvalSigma-Cast-Panic");
            SetPanicReason(sigma, PanicReason::Cast);
            return PanicOutcome();
          }
          SPEC_RULE("EvalSigma-Cast");
          return MakeVal(*value);
        } else if constexpr (std::is_same_v<T, syntax::TransmuteExpr>) {
          Outcome value_out = EvalSigma(ctx, *node.value, sigma);
          if (IsCtrl(value_out)) {
            SPEC_RULE("EvalSigma-Transmute-Ctrl");
            return value_out;
          }
          const auto source_type = ExprTypeOf(ctx, *node.value);
          const auto value =
              EvalTransmuteVal(source_type, expr_type,
                               std::get<Value>(value_out.node));
          if (!value.has_value()) {
            return PanicOutcome();
          }
          SPEC_RULE("EvalSigma-Transmute");
          return MakeVal(*value);
        } else if constexpr (std::is_same_v<T, syntax::PropagateExpr>) {
          Outcome value_out = EvalSigma(ctx, *node.value, sigma);
          if (IsCtrl(value_out)) {
            SPEC_RULE("EvalSigma-Propagate-Ctrl");
            return value_out;
          }
          const auto expr_ty = ExprTypeOf(ctx, *node.value);
          const auto success =
              SuccessMemberType(ctx, ctx.ret_type, expr_ty);
          const auto uni_case = UnionCaseOf(std::get<Value>(value_out.node));
          if (!success.has_value() || !uni_case.has_value()) {
            return PanicOutcome();
          }
          const auto equiv = sema::TypeEquiv(*success, uni_case->first);
          if (!equiv.ok) {
            return PanicOutcome();
          }
          if (equiv.equiv) {
            SPEC_RULE("EvalSigma-Propagate-Success");
            return MakeVal(uni_case->second);
          }
          Control ctrl;
          ctrl.kind = ControlKind::Return;
          ctrl.value = uni_case->second;
          SPEC_RULE("EvalSigma-Propagate-Error");
          return MakeCtrl(ctrl);
        } else if constexpr (std::is_same_v<T, syntax::AddressOfExpr>) {
          AddrResult addr_out = AddrOfSigmaImpl(ctx, *node.place, sigma);
          if (IsAddrCtrl(addr_out)) {
            SPEC_RULE("EvalSigma-AddressOf-Ctrl");
            return MakeCtrl(std::get<Control>(addr_out));
          }
          PtrVal ptr;
          ptr.state = sema::PtrState::Valid;
          ptr.addr = std::get<Addr>(addr_out);
          SPEC_RULE("EvalSigma-AddressOf");
          return MakeVal(Value{ptr});
        } else if constexpr (std::is_same_v<T, syntax::DerefExpr>) {
          Outcome ptr_out = EvalSigma(ctx, *node.value, sigma);
          if (IsCtrl(ptr_out)) {
            SPEC_RULE("EvalSigma-Deref-Ctrl");
            return ptr_out;
          }
          Outcome out = ReadPtrSigmaImpl(std::get<Value>(ptr_out.node), sigma);
          SPEC_RULE("EvalSigma-Deref");
          return out;
        } else if constexpr (std::is_same_v<T, syntax::MoveExpr>) {
          Outcome out = MovePlaceSigmaImpl(ctx, *node.place, sigma);
          SPEC_RULE("EvalSigma-Move");
          return out;
        } else if constexpr (std::is_same_v<T, syntax::AllocExpr>) {
          Outcome value_out = EvalSigma(ctx, *node.value, sigma);
          if (IsCtrl(value_out)) {
            if (node.region_opt.has_value()) {
              SPEC_RULE("EvalSigma-Alloc-Explicit-Ctrl");
              SPEC_RULE("StepSigma-Alloc-Explicit-Ctrl");
            } else {
              SPEC_RULE("EvalSigma-Alloc-Implicit-Ctrl");
              SPEC_RULE("StepSigma-Alloc-Implicit-Ctrl");
            }
            return value_out;
          }
          const auto value = std::get<Value>(value_out.node);
          if (node.region_opt.has_value()) {
            const auto handle = LookupVal(ctx, sigma, *node.region_opt);
            if (!handle.has_value()) {
              return PanicOutcome();
            }
            const auto target = RegionTargetOfImpl(*handle);
            if (!target.has_value()) {
              return PanicOutcome();
            }
            const auto resolved = ResolveTargetImpl(sigma, *target);
            if (!resolved.has_value()) {
              return PanicOutcome();
            }
            const auto alloc_val = RegionAllocImpl(sigma, *resolved, value);
            if (!alloc_val.has_value()) {
              return PanicOutcome();
            }
            SPEC_RULE("EvalSigma-Alloc-Explicit");
            SPEC_RULE("StepSigma-Alloc-Explicit");
            return MakeVal(*alloc_val);
          }
          const auto target = ActiveTargetImpl(sigma);
          if (!target.has_value()) {
            return PanicOutcome();
          }
          const auto alloc_val = RegionAllocImpl(sigma, *target, value);
          if (!alloc_val.has_value()) {
            return PanicOutcome();
          }
          SPEC_RULE("EvalSigma-Alloc-Implicit");
          SPEC_RULE("StepSigma-Alloc-Implicit");
          return MakeVal(*alloc_val);
        } else if constexpr (std::is_same_v<T, syntax::RangeExpr>) {
          auto lo_out = EvalOptSigma(ctx, node.lhs, sigma);
          if (std::holds_alternative<Control>(lo_out)) {
            SPEC_RULE("EvalSigma-Range-Ctrl");
            return MakeCtrl(std::get<Control>(lo_out));
          }
          auto hi_out = EvalOptSigma(ctx, node.rhs, sigma);
          if (std::holds_alternative<Control>(hi_out)) {
            SPEC_RULE("EvalSigma-Range-Ctrl-Hi");
            return MakeCtrl(std::get<Control>(hi_out));
          }
          const auto lo_val = std::get<std::optional<Value>>(lo_out);
          const auto hi_val = std::get<std::optional<Value>>(hi_out);
          RangeVal range = MakeRangeVal(RangeKindOf(node.kind), lo_val, hi_val);
          SPEC_RULE("EvalSigma-Range");
          return MakeVal(Value{range});
        } else if constexpr (std::is_same_v<T, syntax::TupleExpr>) {
          auto values_out = EvalListSigma(ctx, node.elements, sigma);
          if (std::holds_alternative<Control>(values_out)) {
            SPEC_RULE("EvalSigma-Tuple-Ctrl");
            return MakeCtrl(std::get<Control>(values_out));
          }
          TupleVal tuple;
          tuple.elements = std::move(std::get<std::vector<Value>>(values_out));
          SPEC_RULE("EvalSigma-Tuple");
          return MakeVal(Value{tuple});
        } else if constexpr (std::is_same_v<T, syntax::ArrayExpr>) {
          auto values_out = EvalListSigma(ctx, node.elements, sigma);
          if (std::holds_alternative<Control>(values_out)) {
            SPEC_RULE("EvalSigma-Array-Ctrl");
            return MakeCtrl(std::get<Control>(values_out));
          }
          ArrayVal array;
          array.elements = std::move(std::get<std::vector<Value>>(values_out));
          SPEC_RULE("EvalSigma-Array");
          return MakeVal(Value{array});
        } else if constexpr (std::is_same_v<T, syntax::RecordExpr>) {
          auto fields_out = EvalFieldInitsSigma(ctx, node.fields, sigma);
          if (std::holds_alternative<Control>(fields_out)) {
            SPEC_RULE("EvalSigma-Record-Ctrl");
            return MakeCtrl(std::get<Control>(fields_out));
          }
          RecordVal record;
          if (expr_type) {
            record.record_type = expr_type;
          } else if (const auto* path =
                         std::get_if<syntax::TypePath>(&node.target)) {
            record.record_type = sema::MakeTypePath(TypePathOf(*path));
          } else if (const auto* modal =
                         std::get_if<syntax::ModalStateRef>(&node.target)) {
            record.record_type =
                sema::MakeTypeModalState(TypePathOf(modal->path), modal->state);
          }
          record.fields = std::move(
              std::get<std::vector<std::pair<std::string, Value>>>(fields_out));
          SPEC_RULE("EvalSigma-Record");
          return MakeVal(Value{record});
        } else if constexpr (std::is_same_v<T, syntax::EnumLiteralExpr>) {
          EnumVal out;
          out.path = TypePathOf(node.path);
          if (!node.payload_opt.has_value()) {
            SPEC_RULE("EvalSigma-Enum-Unit");
            return MakeVal(Value{out});
          }
          if (const auto* payload =
                  std::get_if<syntax::EnumPayloadParen>(&*node.payload_opt)) {
            auto values_out = EvalListSigma(ctx, payload->elements, sigma);
            if (std::holds_alternative<Control>(values_out)) {
              SPEC_RULE("EvalSigma-Enum-Tuple-Ctrl");
              return MakeCtrl(std::get<Control>(values_out));
            }
            EnumPayloadTupleVal tuple;
            tuple.elements = std::move(std::get<std::vector<Value>>(values_out));
            out.payload = std::move(tuple);
            SPEC_RULE("EvalSigma-Enum-Tuple");
            return MakeVal(Value{out});
          }
          const auto& payload =
              std::get<syntax::EnumPayloadBrace>(*node.payload_opt);
          auto fields_out = EvalFieldInitsSigma(ctx, payload.fields, sigma);
          if (std::holds_alternative<Control>(fields_out)) {
            SPEC_RULE("EvalSigma-Enum-Record-Ctrl");
            return MakeCtrl(std::get<Control>(fields_out));
          }
          EnumPayloadRecordVal record;
          record.fields = std::move(
              std::get<std::vector<std::pair<std::string, Value>>>(fields_out));
          out.payload = std::move(record);
          SPEC_RULE("EvalSigma-Enum-Record");
          return MakeVal(Value{out});
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
          Outcome base_out = EvalSigma(ctx, *node.base, sigma);
          if (IsCtrl(base_out)) {
            SPEC_RULE("EvalSigma-FieldAccess-Ctrl");
            return base_out;
          }
          const auto field =
              FieldValue(std::get<Value>(base_out.node), node.name);
          if (!field.has_value()) {
            return PanicOutcome();
          }
          SPEC_RULE("EvalSigma-FieldAccess");
          return MakeVal(*field);
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
          Outcome base_out = EvalSigma(ctx, *node.base, sigma);
          if (IsCtrl(base_out)) {
            SPEC_RULE("EvalSigma-TupleAccess-Ctrl");
            return base_out;
          }
          auto index_value = ParseIntLiteralValue(node.index.lexeme);
          if (!index_value.has_value() ||
              !core::UInt128FitsU64(*index_value)) {
            return PanicOutcome();
          }
          const std::size_t index =
              static_cast<std::size_t>(core::UInt128ToU64(*index_value));
          const auto elem =
              TupleValue(std::get<Value>(base_out.node), index);
          if (!elem.has_value()) {
            return PanicOutcome();
          }
          SPEC_RULE("EvalSigma-TupleAccess");
          return MakeVal(*elem);
        } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
          Outcome base_out = EvalSigma(ctx, *node.base, sigma);
          if (IsCtrl(base_out)) {
            SPEC_RULE("EvalSigma-Index-Ctrl-Base");
            return base_out;
          }
          Outcome idx_out = EvalSigma(ctx, *node.index, sigma);
          if (IsCtrl(idx_out)) {
            SPEC_RULE("EvalSigma-Index-Ctrl-Idx");
            return idx_out;
          }
          const auto& base_val = std::get<Value>(base_out.node);
          const auto& idx_val = std::get<Value>(idx_out.node);
          if (const auto* range = std::get_if<RangeVal>(&idx_val.node)) {
            const auto len = Len(base_val);
            if (!len.has_value()) {
              return PanicOutcome();
            }
            const auto bounds = SliceBounds(*range, *len);
            if (!bounds.has_value()) {
              SPEC_RULE("EvalSigma-Index-Range-OOB");
              SetPanicReason(sigma, PanicReason::Bounds);
              return PanicOutcome();
            }
            const auto slice = SliceValue(base_val, *range);
            if (!slice.has_value()) {
              SPEC_RULE("EvalSigma-Index-Range-OOB");
              SetPanicReason(sigma, PanicReason::Bounds);
              return PanicOutcome();
            }
            SPEC_RULE("EvalSigma-Index-Range");
            return MakeVal(*slice);
          }
          const auto idx_num = IndexNum(idx_val);
          if (!idx_num.has_value()) {
            return PanicOutcome();
          }
          const auto len = Len(base_val);
          if (!len.has_value()) {
            return PanicOutcome();
          }
          if (*idx_num >= *len) {
            SPEC_RULE("EvalSigma-Index-OOB");
            SetPanicReason(sigma, PanicReason::Bounds);
            return PanicOutcome();
          }
          const auto elem = IndexValue(base_val, *idx_num);
          if (!elem.has_value()) {
            return PanicOutcome();
          }
          SPEC_RULE("EvalSigma-Index");
          return MakeVal(*elem);
        } else if constexpr (std::is_same_v<T, syntax::IfExpr>) {
          if (!node.cond || !node.then_expr) {
            return PanicOutcome();
          }
          return EvalIfSigma(ctx, *node.cond, *node.then_expr,
                             node.else_expr, sigma);
        } else if constexpr (std::is_same_v<T, syntax::MatchExpr>) {
          if (!node.value) {
            return PanicOutcome();
          }
          return EvalMatchSigma(ctx, *node.value, node.arms, sigma);
        } else if constexpr (std::is_same_v<T, syntax::CallExpr>) {
          if (!node.callee) {
            return PanicOutcome();
          }
          return EvalCallSigma(ctx, *node.callee, node.args, sigma);
        } else if constexpr (std::is_same_v<T, syntax::MethodCallExpr>) {
          if (!node.receiver) {
            return PanicOutcome();
          }
          return EvalMethodCallSigma(ctx, *node.receiver, node.name,
                                     node.args, sigma);
        } else if constexpr (std::is_same_v<T, syntax::BlockExpr>) {
          if (!node.block) {
            return PanicOutcome();
          }
          SemanticsContext inner_ctx = ctx;
          inner_ctx.temp_ctx = nullptr;
          Outcome out = EvalBlockSigma(inner_ctx, *node.block, sigma);
          SPEC_RULE("EvalSigma-Block");
          SPEC_RULE("StepSigma-Block");
          return out;
        } else if constexpr (std::is_same_v<T, syntax::UnsafeBlockExpr>) {
          if (!node.block) {
            return PanicOutcome();
          }
          SemanticsContext inner_ctx = ctx;
          inner_ctx.temp_ctx = nullptr;
          Outcome out = EvalBlockSigma(inner_ctx, *node.block, sigma);
          SPEC_RULE("EvalSigma-UnsafeBlock");
          SPEC_RULE("StepSigma-UnsafeBlock");
          return out;
        } else if constexpr (std::is_same_v<T, syntax::LoopInfiniteExpr>) {
          if (!node.body) {
            return PanicOutcome();
          }
          SPEC_RULE("StepSigma-Loop");
          return EvalLoopInfiniteSigma(ctx, *node.body, sigma);
        } else if constexpr (std::is_same_v<T, syntax::LoopConditionalExpr>) {
          if (!node.cond || !node.body) {
            return PanicOutcome();
          }
          SPEC_RULE("StepSigma-Loop");
          return EvalLoopConditionalSigma(ctx, *node.cond, *node.body, sigma);
        } else if constexpr (std::is_same_v<T, syntax::LoopIterExpr>) {
          if (!node.pattern || !node.iter || !node.body) {
            return PanicOutcome();
          }
          return EvalLoopIterSigma(ctx, *node.pattern, *node.iter,
                                   *node.body, sigma);
        } else {
          return PanicOutcome();
        }
      },
      expr.node);

  if (temp_ctx) {
    if (!suppress && temp_ctx->sink && IsVal(out) && !IsPlaceExpr(expr)) {
      const auto value = std::get<Value>(out.node);
      sema::TypeRef temp_type = ExprTypeOf(ctx, expr);
      if (!temp_type) {
        temp_type = TypeOfValue(value);
      }
      temp_ctx->sink->push_back(TempValue{temp_type, value});
    }
    temp_ctx->depth -= 1;
  }

  return out;
}

}  // namespace cursive0::semantics



