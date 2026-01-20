#include "cursive0/codegen/layout.h"

#include <algorithm>
#include <array>
#include <bit>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <cstring>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/int128.h"
#include "cursive0/sema/scopes.h"
#include "cursive0/sema/modal_widen.h"
#include "cursive0/sema/type_equiv.h"

namespace cursive0::codegen {

bool ValidValue(const cursive0::sema::ScopeContext& ctx,
                const cursive0::sema::TypeRef& type,
                const std::vector<std::uint8_t>& bits);

namespace {

std::uint64_t AlignUp(std::uint64_t value, std::uint64_t align) {
  if (align == 0) {
    return value;
  }
  const std::uint64_t rem = value % align;
  if (rem == 0) {
    return value;
  }
  return value + (align - rem);
}

std::vector<std::uint8_t> LEBytes(core::UInt128 value, std::size_t n) {
  std::vector<std::uint8_t> out(n);
  core::UInt128 tmp = value;
  for (std::size_t i = 0; i < n; ++i) {
    const std::uint64_t low = core::UInt128ToU64(tmp);
    out[i] = static_cast<std::uint8_t>(low & 0xFFu);
    tmp = core::UInt128ShiftRight(tmp, 8);
  }
  return out;
}

std::vector<std::uint8_t> LEBytesU64(std::uint64_t value, std::size_t n) {
  return LEBytes(core::UInt128FromU64(value), n);
}

std::optional<std::uint64_t> BitsToUInt(const std::vector<std::uint8_t>& bits) {
  if (bits.size() > 8) {
    return std::nullopt;
  }
  std::uint64_t out = 0;
  for (std::size_t i = 0; i < bits.size(); ++i) {
    out |= (static_cast<std::uint64_t>(bits[i]) << (8 * i));
  }
  return out;
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

static constexpr std::array<std::string_view, 12> kIntTypes = {
    "i8", "i16", "i32", "i64", "i128", "isize",
    "u8", "u16", "u32", "u64", "u128", "usize"};

static constexpr std::array<std::string_view, 3> kFloatTypes = {
    "f16", "f32", "f64"};

static bool IsIntTypeName(std::string_view name) {
  for (const auto& t : kIntTypes) {
    if (name == t) {
      return true;
    }
  }
  return false;
}

static bool IsFloatTypeName(std::string_view name) {
  for (const auto& t : kFloatTypes) {
    if (name == t) {
      return true;
    }
  }
  return false;
}

static constexpr std::array<std::string_view, 12> kIntSuffixes = {
    "i128", "u128", "isize", "usize", "i64", "u64",
    "i32",  "u32",  "i16",  "u16",  "i8",  "u8"};

static constexpr std::array<std::string_view, 3> kFloatSuffixes = {
    "f16", "f32", "f64"};

static bool EndsWith(std::string_view value, std::string_view suffix) {
  if (suffix.size() > value.size()) {
    return false;
  }
  return value.substr(value.size() - suffix.size()) == suffix;
}

static std::string_view StripIntSuffix(std::string_view lexeme) {
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

static std::string_view StripFloatSuffix(std::string_view lexeme) {
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

static bool DigitValue(char c, unsigned base, unsigned* out) {
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

static std::optional<std::pair<std::uint32_t, std::size_t>> DecodeUtf8One(
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

static std::vector<std::uint8_t> EncodeUtf8(std::uint32_t value) {
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

static std::optional<std::uint32_t> ParseHexScalar(std::string_view digits) {
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

static std::optional<std::uint32_t> DecodeCharLiteral(std::string_view lexeme) {
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
        case '\"':
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

}  // namespace

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

namespace {

static bool ParseIntCore(std::string_view core, core::UInt128& value_out) {
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

static std::optional<core::UInt128> ParseIntLiteralValue(
    std::string_view lexeme) {
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

static std::optional<double> ParseFloatLiteralValue(
    std::string_view lexeme) {
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

std::uint16_t FloatToHalf(float value) {
  const std::uint32_t bits = std::bit_cast<std::uint32_t>(value);
  const std::uint32_t sign = (bits >> 16) & 0x8000u;
  const std::uint32_t exp = (bits >> 23) & 0xFFu;
  const std::uint32_t mantissa = bits & 0x7FFFFFu;

  if (exp == 0xFFu) {
    if (mantissa == 0) {
      return static_cast<std::uint16_t>(sign | 0x7C00u);
    }
    const std::uint16_t payload = static_cast<std::uint16_t>(mantissa >> 13);
    return static_cast<std::uint16_t>(sign | 0x7C00u | payload | 0x1u);
  }

  if (exp == 0) {
    return static_cast<std::uint16_t>(sign);
  }

  const int exp_adjusted = static_cast<int>(exp) - 127 + 15;
  if (exp_adjusted >= 31) {
    return static_cast<std::uint16_t>(sign | 0x7C00u);
  }
  if (exp_adjusted <= 0) {
    if (exp_adjusted < -10) {
      return static_cast<std::uint16_t>(sign);
    }
    std::uint32_t mant = mantissa | 0x800000u;
    const std::uint32_t shift = static_cast<std::uint32_t>(14 - exp_adjusted);
    const std::uint32_t rounded =
        (mant + (1u << (shift - 1)) + ((mant >> shift) & 1u)) >> shift;
    return static_cast<std::uint16_t>(sign | rounded);
  }

  const std::uint32_t rounded =
      (mantissa + 0xFFFu + ((mantissa >> 13) & 1u)) >> 13;
  if (rounded == 0x400u) {
    return static_cast<std::uint16_t>(sign | ((exp_adjusted + 1) << 10));
  }
  return static_cast<std::uint16_t>(sign | (exp_adjusted << 10) |
                                    (rounded & 0x3FFu));
}

std::optional<std::vector<std::uint8_t>> StructBits(
    const std::vector<std::vector<std::uint8_t>>& field_bits,
    const std::vector<std::uint64_t>& offsets,
    std::uint64_t size) {
  if (field_bits.size() != offsets.size()) {
    return std::nullopt;
  }
  std::vector<std::uint8_t> bits(size, 0);
  for (std::size_t i = 0; i < field_bits.size(); ++i) {
    const auto& fb = field_bits[i];
    const std::uint64_t off = offsets[i];
    if (off + fb.size() > bits.size()) {
      return std::nullopt;
    }
    std::copy(fb.begin(), fb.end(), bits.begin() + off);
  }
  return bits;
}

std::vector<std::uint8_t> PadBytes(const std::vector<std::uint8_t>& bytes,
                                   std::uint64_t size) {
  std::vector<std::uint8_t> out(size, 0);
  const std::size_t copy = std::min<std::size_t>(bytes.size(), out.size());
  std::copy(bytes.begin(), bytes.begin() + copy, out.begin());
  return out;
}

std::optional<std::vector<std::uint8_t>> TaggedBits(
    const std::vector<std::uint8_t>& disc_bits,
    const std::vector<std::uint8_t>& payload_bits,
    std::uint64_t disc_size,
    std::uint64_t payload_size,
    std::uint64_t payload_align,
    std::uint64_t size) {
  const std::uint64_t payload_off = AlignUp(disc_size, payload_align);
  if (size < payload_off + payload_size) {
    return std::nullopt;
  }
  std::vector<std::uint8_t> out(size, 0);
  if (disc_bits.size() != disc_size || payload_bits.size() != payload_size) {
    return std::nullopt;
  }
  std::copy(disc_bits.begin(), disc_bits.end(), out.begin());
  std::copy(payload_bits.begin(), payload_bits.end(), out.begin() + payload_off);
  return out;
}

bool IsUnitType(const cursive0::sema::TypeRef& type) {
  if (!type) {
    return false;
  }
  if (const auto* prim = std::get_if<cursive0::sema::TypePrim>(&type->node)) {
    return prim->name == "()";
  }
  return false;
}

bool IsNicheType(const cursive0::sema::ScopeContext& ctx,
                 const cursive0::sema::TypeRef& type) {
  (void)ctx;
  if (!type) {
    return false;
  }
  if (const auto* ptr = std::get_if<cursive0::sema::TypePtr>(&type->node)) {
    return ptr->state == cursive0::sema::PtrState::Valid;
  }
  return false;
}

std::uint64_t NicheCount(const cursive0::sema::ScopeContext& ctx,
                         const cursive0::sema::TypeRef& type) {
  return IsNicheType(ctx, type) ? 1 : 0;
}

std::optional<std::vector<std::uint8_t>> NicheBitsForIndex(
    const cursive0::sema::ScopeContext& ctx,
    const cursive0::sema::TypeRef& type,
    std::size_t index) {
  if (!IsNicheType(ctx, type)) {
    return std::nullopt;
  }
  if (index != 0) {
    return std::nullopt;
  }
  const auto size = SizeOf(ctx, type);
  if (!size.has_value()) {
    return std::nullopt;
  }
  return LEBytesU64(0, *size);
}

bool IsNicheBits(const cursive0::sema::ScopeContext& ctx,
                 const cursive0::sema::TypeRef& type,
                 const std::vector<std::uint8_t>& bits) {
  const auto niche = NicheBitsForIndex(ctx, type, 0);
  return niche.has_value() && *niche == bits;
}

const Value* FindFieldValue(const RecordVal& val, std::string_view name) {
  for (const auto& entry : val.fields) {
    if (cursive0::sema::IdEq(entry.first, name)) {
      return &entry.second;
    }
  }
  return nullptr;
}

const cursive0::syntax::StateBlock* FindState(
    const cursive0::syntax::ModalDecl& decl,
    std::string_view name) {
  for (const auto& state : decl.states) {
    if (cursive0::sema::IdEq(state.name, name)) {
      return &state;
    }
  }
  return nullptr;
}

bool IsEmptyState(const cursive0::syntax::StateBlock& state) {
  for (const auto& member : state.members) {
    if (std::holds_alternative<cursive0::syntax::StateFieldDecl>(member)) {
      return false;
    }
  }
  return true;
}

std::optional<std::pair<std::string, cursive0::sema::TypeRef>>
SingleFieldPayloadType(const cursive0::sema::ScopeContext& ctx,
                       const cursive0::syntax::StateBlock& state) {
  const cursive0::syntax::StateFieldDecl* field = nullptr;
  for (const auto& member : state.members) {
    const auto* payload =
        std::get_if<cursive0::syntax::StateFieldDecl>(&member);
    if (!payload) {
      continue;
    }
    if (field) {
      return std::nullopt;
    }
    field = payload;
  }
  if (!field) {
    return std::nullopt;
  }
  const auto lowered = LowerTypeForLayout(ctx, field->type);
  if (!lowered.has_value()) {
    return std::nullopt;
  }
  return std::make_pair(field->name, *lowered);
}

std::optional<std::vector<std::pair<std::string, cursive0::sema::TypeRef>>> CollectStateFields(
    const cursive0::sema::ScopeContext& ctx,
    const cursive0::syntax::StateBlock& state) {
  std::vector<std::pair<std::string, cursive0::sema::TypeRef>> fields;
  for (const auto& member : state.members) {
    if (const auto* field =
            std::get_if<cursive0::syntax::StateFieldDecl>(&member)) {
      const auto lowered = LowerTypeForLayout(ctx, field->type);
      if (!lowered.has_value()) {
        return std::nullopt;
      }
      fields.emplace_back(field->name, *lowered);
    }
  }
  return fields;
}

std::optional<std::vector<std::uint8_t>> ValueBitsForRecord(
    const cursive0::sema::ScopeContext& ctx,
    const std::vector<std::pair<std::string, cursive0::sema::TypeRef>>& fields,
    const RecordVal& val) {
  std::vector<std::vector<std::uint8_t>> bits;
  std::vector<cursive0::sema::TypeRef> types;
  bits.reserve(fields.size());
  types.reserve(fields.size());

  for (const auto& field : fields) {
    const auto it = std::find_if(val.fields.begin(), val.fields.end(),
                                [&](const auto& entry) {
                                  return cursive0::sema::IdEq(entry.first,
                                                             field.first);
                                });
    if (it == val.fields.end()) {
      return std::nullopt;
    }
    const auto field_bits = ValueBits(ctx, field.second, it->second);
    if (!field_bits.has_value()) {
      return std::nullopt;
    }
    bits.push_back(*field_bits);
    types.push_back(field.second);
  }

  const auto layout = RecordLayoutOf(ctx, types);
  if (!layout.has_value()) {
    return std::nullopt;
  }

  return StructBits(bits, layout->offsets, layout->layout.size);
}

std::optional<std::vector<std::uint8_t>> EnumPayloadBits(
    const cursive0::sema::ScopeContext& ctx,
    const cursive0::syntax::EnumDecl& decl,
    const cursive0::syntax::VariantDecl& variant,
    const std::optional<EnumPayloadVal>& payload,
    std::uint64_t payload_size) {
  if (!variant.payload_opt.has_value()) {
    return PadBytes({}, payload_size);
  }
  if (!payload.has_value()) {
    return std::nullopt;
  }
  if (const auto* tuple =
          std::get_if<cursive0::syntax::VariantPayloadTuple>(
              &*variant.payload_opt)) {
    const auto* tuple_val = std::get_if<EnumPayloadTupleVal>(&*payload);
    if (!tuple_val || tuple->elements.size() != tuple_val->elements.size()) {
      return std::nullopt;
    }
    std::vector<cursive0::sema::TypeRef> elems;
    std::vector<std::vector<std::uint8_t>> bits;
    elems.reserve(tuple->elements.size());
    bits.reserve(tuple->elements.size());
    for (std::size_t i = 0; i < tuple->elements.size(); ++i) {
      const auto lowered = LowerTypeForLayout(ctx, tuple->elements[i]);
      if (!lowered.has_value()) {
        return std::nullopt;
      }
      const auto elem_bits = ValueBits(ctx, *lowered, tuple_val->elements[i]);
      if (!elem_bits.has_value()) {
        return std::nullopt;
      }
      elems.push_back(*lowered);
      bits.push_back(*elem_bits);
    }
    const auto layout = RecordLayoutOf(ctx, elems);
    if (!layout.has_value()) {
      return std::nullopt;
    }
    const auto struct_bits =
        StructBits(bits, layout->offsets, layout->layout.size);
    if (!struct_bits.has_value()) {
      return std::nullopt;
    }
    return PadBytes(*struct_bits, payload_size);
  }
  if (const auto* record =
          std::get_if<cursive0::syntax::VariantPayloadRecord>(
              &*variant.payload_opt)) {
    const auto* record_val = std::get_if<EnumPayloadRecordVal>(&*payload);
    if (!record_val) {
      return std::nullopt;
    }
    std::vector<std::pair<std::string, cursive0::sema::TypeRef>> fields;
    fields.reserve(record->fields.size());
    for (const auto& field : record->fields) {
      const auto lowered = LowerTypeForLayout(ctx, field.type);
      if (!lowered.has_value()) {
        return std::nullopt;
      }
      fields.emplace_back(field.name, *lowered);
    }
    const auto struct_bits = ValueBitsForRecord(ctx, fields,
                                                RecordVal{record_val->fields});
    if (!struct_bits.has_value()) {
      return std::nullopt;
    }
    return PadBytes(*struct_bits, payload_size);
  }
  return std::nullopt;
}

std::optional<std::vector<std::uint8_t>> ModalTaggedBits(
    const cursive0::sema::ScopeContext& ctx,
    const cursive0::syntax::ModalDecl& decl,
    const std::string& state,
    const Value& payload) {
  const auto layout = ModalLayoutOf(ctx, decl);
  if (!layout.has_value() || layout->niche || !layout->disc_type.has_value()) {
    return std::nullopt;
  }
  std::uint64_t payload_align = 1;
  std::uint64_t payload_size = 0;
  for (const auto& st : decl.states) {
    std::vector<cursive0::sema::TypeRef> fields;
    for (const auto& member : st.members) {
      if (const auto* field =
              std::get_if<cursive0::syntax::StateFieldDecl>(&member)) {
        const auto lowered = LowerTypeForLayout(ctx, field->type);
        if (!lowered.has_value()) {
          return std::nullopt;
        }
        fields.push_back(*lowered);
      }
    }
    const auto st_layout = RecordLayoutOf(ctx, fields);
    if (!st_layout.has_value()) {
      return std::nullopt;
    }
    payload_size = std::max(payload_size, st_layout->layout.size);
    payload_align = std::max(payload_align, st_layout->layout.align);
  }

  std::optional<std::vector<std::uint8_t>> payload_bits;
  std::size_t state_index = 0;
  bool found = false;
  for (std::size_t i = 0; i < decl.states.size(); ++i) {
    const auto& st = decl.states[i];
    if (cursive0::sema::IdEq(st.name, state)) {
      found = true;
      state_index = i;
      const auto fields = CollectStateFields(ctx, st);
      if (!fields.has_value()) {
        return std::nullopt;
      }
      if (fields->empty()) {
        if (std::holds_alternative<UnitVal>(payload.node)) {
          payload_bits = PadBytes({}, payload_size);
        } else if (const auto* record = std::get_if<RecordVal>(&payload.node)) {
          if (!record->fields.empty()) {
            return std::nullopt;
          }
          payload_bits = PadBytes({}, payload_size);
        } else {
          return std::nullopt;
        }
      } else {
        const auto* record_val = std::get_if<RecordVal>(&payload.node);
        if (!record_val) {
          return std::nullopt;
        }
        const auto record_bits = ValueBitsForRecord(ctx, *fields, *record_val);
        if (!record_bits.has_value()) {
          return std::nullopt;
        }
        payload_bits = PadBytes(*record_bits, payload_size);
      }
      break;
    }
  }
  if (!found || !payload_bits.has_value()) {
    return std::nullopt;
  }

  const auto disc_layout = PrimSize(*layout->disc_type);
  const auto disc_align = PrimAlign(*layout->disc_type);
  if (!disc_layout.has_value() || !disc_align.has_value()) {
    return std::nullopt;
  }

  const auto disc_bits = LEBytesU64(state_index, *disc_layout);
  return TaggedBits(disc_bits, *payload_bits, *disc_layout, payload_size,
                    payload_align, layout->layout.size);
}

std::optional<std::vector<std::uint8_t>> ModalNicheBits(
    const cursive0::sema::ScopeContext& ctx,
    const cursive0::syntax::ModalDecl& decl,
    const std::string& state,
    const Value& payload) {
  const auto payload_state = cursive0::sema::PayloadState(ctx, decl);
  if (!payload_state.has_value()) {
    return std::nullopt;
  }
  const auto* payload_block = FindState(decl, *payload_state);
  if (!payload_block) {
    return std::nullopt;
  }
  const auto payload_field = SingleFieldPayloadType(ctx, *payload_block);
  if (!payload_field.has_value()) {
    return std::nullopt;
  }

  const auto& field_name = payload_field->first;
  const auto& field_type = payload_field->second;

  if (cursive0::sema::IdEq(*payload_state, state)) {
    const Value* payload_value = &payload;
    if (const auto* record = std::get_if<RecordVal>(&payload.node)) {
      const Value* field_value = FindFieldValue(*record, field_name);
      if (!field_value) {
        return std::nullopt;
      }
      payload_value = field_value;
    }
    const auto bits = ValueBits(ctx, field_type, *payload_value);
    if (!bits.has_value()) {
      return std::nullopt;
    }
    if (IsNicheBits(ctx, field_type, *bits)) {
      return std::nullopt;
    }
    return bits;
  }

  const auto* state_block = FindState(decl, state);
  if (!state_block || !IsEmptyState(*state_block)) {
    return std::nullopt;
  }

  if (!std::holds_alternative<UnitVal>(payload.node)) {
    if (const auto* record = std::get_if<RecordVal>(&payload.node)) {
      if (!record->fields.empty()) {
        return std::nullopt;
      }
    } else {
      return std::nullopt;
    }
  }

  std::size_t empty_index = 0;
  bool found = false;
  for (const auto& st : decl.states) {
    if (!IsEmptyState(st)) {
      continue;
    }
    if (cursive0::sema::IdEq(st.name, state)) {
      found = true;
      break;
    }
    ++empty_index;
  }
  if (!found) {
    return std::nullopt;
  }
  return NicheBitsForIndex(ctx, field_type, empty_index);
}


std::optional<std::vector<std::uint8_t>> SliceBytes(
    const std::vector<std::uint8_t>& bits,
    std::size_t offset,
    std::size_t size) {
  if (offset > bits.size() || size > bits.size() - offset) {
    return std::nullopt;
  }
  return std::vector<std::uint8_t>(bits.begin() + offset,
                                   bits.begin() + offset + size);
}

bool AllZero(const std::vector<std::uint8_t>& bits,
             std::size_t offset,
             std::size_t size) {
  if (offset > bits.size() || size > bits.size() - offset) {
    return false;
  }
  for (std::size_t i = offset; i < offset + size; ++i) {
    if (bits[i] != 0) {
      return false;
    }
  }
  return true;
}

bool CheckPaddingZero(
    const std::vector<std::uint8_t>& bits,
    const std::vector<std::pair<std::size_t, std::size_t>>& ranges) {
  std::vector<bool> covered(bits.size(), false);
  for (const auto& range : ranges) {
    if (range.first > range.second || range.second > bits.size()) {
      return false;
    }
    for (std::size_t i = range.first; i < range.second; ++i) {
      covered[i] = true;
    }
  }
  for (std::size_t i = 0; i < bits.size(); ++i) {
    if (!covered[i] && bits[i] != 0) {
      return false;
    }
  }
  return true;
}

bool ValidStructBits(const cursive0::sema::ScopeContext& ctx,
                     const std::vector<cursive0::sema::TypeRef>& types,
                     const std::vector<std::uint64_t>& offsets,
                     std::uint64_t size,
                     const std::vector<std::uint8_t>& bits) {
  if (types.size() != offsets.size()) {
    return false;
  }
  if (bits.size() != size) {
    return false;
  }
  std::vector<std::pair<std::size_t, std::size_t>> ranges;
  ranges.reserve(types.size());
  for (std::size_t i = 0; i < types.size(); ++i) {
    const auto elem_size = SizeOf(ctx, types[i]);
    if (!elem_size.has_value()) {
      return false;
    }
    const std::size_t off = static_cast<std::size_t>(offsets[i]);
    const std::size_t sz = static_cast<std::size_t>(*elem_size);
    if (off > bits.size() || sz > bits.size() - off) {
      return false;
    }
    const auto slice = SliceBytes(bits, off, sz);
    if (!slice.has_value()) {
      return false;
    }
    if (!ValidValue(ctx, types[i], *slice)) {
      return false;
    }
    ranges.emplace_back(off, off + sz);
  }
  return CheckPaddingZero(bits, ranges);
}

bool ValidPaddedStructBits(const cursive0::sema::ScopeContext& ctx,
                           const std::vector<cursive0::sema::TypeRef>& types,
                           const std::vector<std::uint64_t>& offsets,
                           std::uint64_t struct_size,
                           const std::vector<std::uint8_t>& bits) {
  if (bits.size() < struct_size) {
    return false;
  }
  const auto prefix =
      SliceBytes(bits, 0, static_cast<std::size_t>(struct_size));
  if (!prefix.has_value()) {
    return false;
  }
  if (!ValidStructBits(ctx, types, offsets, struct_size, *prefix)) {
    return false;
  }
  return AllZero(bits, static_cast<std::size_t>(struct_size),
                 bits.size() - static_cast<std::size_t>(struct_size));
}

bool ValidTaggedPayloadBits(const cursive0::sema::ScopeContext& ctx,
                            const cursive0::sema::TypeRef& type,
                            const std::vector<std::uint8_t>& payload_bits) {
  const auto size = SizeOf(ctx, type);
  if (!size.has_value()) {
    return false;
  }
  const std::size_t sz = static_cast<std::size_t>(*size);
  if (sz > payload_bits.size()) {
    return false;
  }
  const auto prefix = SliceBytes(payload_bits, 0, sz);
  if (!prefix.has_value()) {
    return false;
  }
  return ValidValue(ctx, type, *prefix);
}

bool ValidEnumBits(const cursive0::sema::ScopeContext& ctx,
                   const cursive0::syntax::EnumDecl& decl,
                   const std::vector<std::uint8_t>& bits) {
  const auto layout = EnumLayoutOf(ctx, decl);
  if (!layout.has_value()) {
    return false;
  }
  if (bits.size() != layout->layout.size) {
    return false;
  }
  const auto disc_size_opt = PrimSize(layout->disc_type);
  const auto disc_align_opt = PrimAlign(layout->disc_type);
  if (!disc_size_opt.has_value() || !disc_align_opt.has_value()) {
    return false;
  }
  const std::uint64_t disc_size = *disc_size_opt;
  const std::uint64_t payload_off = AlignUp(disc_size, layout->payload_align);
  if (payload_off + layout->payload_size > bits.size()) {
    return false;
  }
  const auto disc_bits =
      SliceBytes(bits, 0, static_cast<std::size_t>(disc_size));
  if (!disc_bits.has_value()) {
    return false;
  }
  const auto disc_value = BitsToUInt(*disc_bits);
  if (!disc_value.has_value()) {
    return false;
  }
  const auto discs = cursive0::sema::EnumDiscriminants(decl);
  if (!discs.ok || discs.discs.size() != decl.variants.size()) {
    return false;
  }
  std::optional<std::size_t> index;
  for (std::size_t i = 0; i < discs.discs.size(); ++i) {
    if (discs.discs[i] == *disc_value) {
      index = i;
      break;
    }
  }
  if (!index.has_value()) {
    return false;
  }
  const std::size_t payload_start = static_cast<std::size_t>(payload_off);
  const std::size_t payload_size =
      static_cast<std::size_t>(layout->payload_size);
  const auto payload_bits = SliceBytes(bits, payload_start, payload_size);
  if (!payload_bits.has_value()) {
    return false;
  }
  const auto& variant = decl.variants[*index];
  if (!variant.payload_opt.has_value()) {
    return AllZero(*payload_bits, 0, payload_bits->size());
  }
  if (const auto* tuple =
          std::get_if<cursive0::syntax::VariantPayloadTuple>(
              &*variant.payload_opt)) {
    std::vector<cursive0::sema::TypeRef> elems;
    elems.reserve(tuple->elements.size());
    for (const auto& elem : tuple->elements) {
      const auto lowered = LowerTypeForLayout(ctx, elem);
      if (!lowered.has_value()) {
        return false;
      }
      elems.push_back(*lowered);
    }
    const auto tuple_layout = RecordLayoutOf(ctx, elems);
    if (!tuple_layout.has_value()) {
      return false;
    }
    return ValidPaddedStructBits(ctx, elems, tuple_layout->offsets,
                                 tuple_layout->layout.size, *payload_bits);
  }
  if (const auto* record =
          std::get_if<cursive0::syntax::VariantPayloadRecord>(
              &*variant.payload_opt)) {
    std::vector<cursive0::sema::TypeRef> types;
    types.reserve(record->fields.size());
    for (const auto& field : record->fields) {
      const auto lowered = LowerTypeForLayout(ctx, field.type);
      if (!lowered.has_value()) {
        return false;
      }
      types.push_back(*lowered);
    }
    const auto record_layout = RecordLayoutOf(ctx, types);
    if (!record_layout.has_value()) {
      return false;
    }
    return ValidPaddedStructBits(ctx, types, record_layout->offsets,
                                 record_layout->layout.size, *payload_bits);
  }
  return false;
}

bool ValidModalTaggedBits(const cursive0::sema::ScopeContext& ctx,
                          const cursive0::syntax::ModalDecl& decl,
                          const std::vector<std::uint8_t>& bits) {
  const auto layout = ModalLayoutOf(ctx, decl);
  if (!layout.has_value() || layout->niche || !layout->disc_type.has_value()) {
    return false;
  }
  if (bits.size() != layout->layout.size) {
    return false;
  }
  std::uint64_t payload_size = 0;
  std::uint64_t payload_align = 1;
  for (const auto& st : decl.states) {
    const auto fields = CollectStateFields(ctx, st);
    if (!fields.has_value()) {
      return false;
    }
    std::vector<cursive0::sema::TypeRef> types;
    types.reserve(fields->size());
    for (const auto& field : *fields) {
      types.push_back(field.second);
    }
    const auto st_layout = RecordLayoutOf(ctx, types);
    if (!st_layout.has_value()) {
      return false;
    }
    payload_size = std::max(payload_size, st_layout->layout.size);
    payload_align = std::max(payload_align, st_layout->layout.align);
  }
  const auto disc_size_opt = PrimSize(*layout->disc_type);
  const auto disc_align_opt = PrimAlign(*layout->disc_type);
  if (!disc_size_opt.has_value() || !disc_align_opt.has_value()) {
    return false;
  }
  const std::uint64_t disc_size = *disc_size_opt;
  const std::uint64_t payload_off = AlignUp(disc_size, payload_align);
  if (payload_off + payload_size > bits.size()) {
    return false;
  }
  const auto disc_bits =
      SliceBytes(bits, 0, static_cast<std::size_t>(disc_size));
  if (!disc_bits.has_value()) {
    return false;
  }
  const auto disc_value = BitsToUInt(*disc_bits);
  if (!disc_value.has_value()) {
    return false;
  }
  if (*disc_value >= decl.states.size()) {
    return false;
  }
  const std::size_t payload_start = static_cast<std::size_t>(payload_off);
  const std::size_t payload_size_sz = static_cast<std::size_t>(payload_size);
  if (!CheckPaddingZero(bits,
                        {{0, static_cast<std::size_t>(disc_size)},
                         {payload_start, payload_start + payload_size_sz}})) {
    return false;
  }
  const auto payload_bits =
      SliceBytes(bits, payload_start, payload_size_sz);
  if (!payload_bits.has_value()) {
    return false;
  }
  const auto& state = decl.states[*disc_value];
  const auto fields = CollectStateFields(ctx, state);
  if (!fields.has_value()) {
    return false;
  }
  if (fields->empty()) {
    return AllZero(*payload_bits, 0, payload_bits->size());
  }
  std::vector<cursive0::sema::TypeRef> types;
  types.reserve(fields->size());
  for (const auto& field : *fields) {
    types.push_back(field.second);
  }
  const auto st_layout = RecordLayoutOf(ctx, types);
  if (!st_layout.has_value()) {
    return false;
  }
  return ValidPaddedStructBits(ctx, types, st_layout->offsets,
                               st_layout->layout.size, *payload_bits);
}

bool ValidModalNicheBits(const cursive0::sema::ScopeContext& ctx,
                         const cursive0::syntax::ModalDecl& decl,
                         const std::vector<std::uint8_t>& bits) {
  const auto layout = ModalLayoutOf(ctx, decl);
  if (!layout.has_value() || !layout->niche) {
    return false;
  }
  if (bits.size() != layout->layout.size) {
    return false;
  }
  const auto payload_state = cursive0::sema::PayloadState(ctx, decl);
  if (!payload_state.has_value()) {
    return false;
  }
  const auto* payload_block = FindState(decl, *payload_state);
  if (!payload_block) {
    return false;
  }
  const auto payload_field = SingleFieldPayloadType(ctx, *payload_block);
  if (!payload_field.has_value()) {
    return false;
  }
  const auto& field_type = payload_field->second;
  std::size_t empty_states = 0;
  for (const auto& st : decl.states) {
    if (IsEmptyState(st)) {
      ++empty_states;
    }
  }
  const auto niche_bits = NicheBitsForIndex(ctx, field_type, 0);
  if (niche_bits.has_value() && *niche_bits == bits) {
    return empty_states > 0;
  }
  return ValidValue(ctx, field_type, bits);
}

bool ValidModalBits(const cursive0::sema::ScopeContext& ctx,
                    const cursive0::syntax::ModalDecl& decl,
                    const std::vector<std::uint8_t>& bits) {
  const auto layout = ModalLayoutOf(ctx, decl);
  if (!layout.has_value()) {
    return false;
  }
  if (layout->niche) {
    return ValidModalNicheBits(ctx, decl, bits);
  }
  return ValidModalTaggedBits(ctx, decl, bits);
}

bool ValidUnionBits(const cursive0::sema::ScopeContext& ctx,
                    const cursive0::sema::TypeUnion& uni,
                    const std::vector<std::uint8_t>& bits) {
  const auto layout = UnionLayoutOf(ctx, uni);
  if (!layout.has_value()) {
    return false;
  }
  if (bits.size() != layout->layout.size) {
    return false;
  }
  if (layout->niche) {
    const auto& members = layout->member_list;
    if (members.empty()) {
      return false;
    }
    std::optional<std::size_t> payload_index;
    std::uint64_t niche_count = 0;
    for (std::size_t i = 0; i < members.size(); ++i) {
      if (IsUnitType(members[i])) {
        continue;
      }
      const auto count = NicheCount(ctx, members[i]);
      if (count == 0) {
        continue;
      }
      if (payload_index.has_value()) {
        payload_index.reset();
        break;
      }
      payload_index = i;
      niche_count = count;
    }
    if (!payload_index.has_value()) {
      return false;
    }
    bool niche_applies = true;
    for (std::size_t i = 0; i < members.size(); ++i) {
      if (i == *payload_index) {
        continue;
      }
      if (!IsUnitType(members[i])) {
        niche_applies = false;
        break;
      }
    }
    if (niche_count < members.size() - 1) {
      niche_applies = false;
    }
    if (!niche_applies) {
      return false;
    }
    const auto& payload_type = members[*payload_index];
    std::size_t empty_states = 0;
    for (std::size_t i = 0; i < members.size(); ++i) {
      if (i == *payload_index) {
        continue;
      }
      if (IsUnitType(members[i])) {
        ++empty_states;
      }
    }
    const auto niche_bits = NicheBitsForIndex(ctx, payload_type, 0);
    if (niche_bits.has_value() && *niche_bits == bits) {
      return empty_states > 0;
    }
    return ValidValue(ctx, payload_type, bits);
  }

  if (!layout->disc_type.has_value()) {
    return false;
  }
  const auto disc_size_opt = PrimSize(*layout->disc_type);
  const auto disc_align_opt = PrimAlign(*layout->disc_type);
  if (!disc_size_opt.has_value() || !disc_align_opt.has_value()) {
    return false;
  }
  const std::uint64_t disc_size = *disc_size_opt;
  const std::uint64_t payload_off = AlignUp(disc_size, layout->payload_align);
  if (payload_off + layout->payload_size > bits.size()) {
    return false;
  }
  const auto disc_bits =
      SliceBytes(bits, 0, static_cast<std::size_t>(disc_size));
  if (!disc_bits.has_value()) {
    return false;
  }
  const auto disc_value = BitsToUInt(*disc_bits);
  if (!disc_value.has_value()) {
    return false;
  }
  if (*disc_value >= layout->member_list.size()) {
    return false;
  }
  const std::size_t payload_start = static_cast<std::size_t>(payload_off);
  const std::size_t payload_size =
      static_cast<std::size_t>(layout->payload_size);
  const auto payload_bits = SliceBytes(bits, payload_start, payload_size);
  if (!payload_bits.has_value()) {
    return false;
  }
  return ValidTaggedPayloadBits(ctx, layout->member_list[*disc_value],
                                *payload_bits);
}

}  // namespace

std::optional<std::vector<std::uint8_t>> EncodeConst(
    const cursive0::sema::TypeRef& type,
    const cursive0::syntax::Token& lit) {
  if (!type) {
    return std::nullopt;
  }
  const auto* prim = std::get_if<cursive0::sema::TypePrim>(&type->node);
  if (!prim) {
    if (std::holds_alternative<cursive0::sema::TypeRawPtr>(type->node) &&
        lit.kind == cursive0::syntax::TokenKind::NullLiteral) {
      SPEC_RULE("Encode-RawPtr-Null");
      return LEBytesU64(0, kPtrSize);
    }
    return std::nullopt;
  }

  if (prim->name == "bool") {
    if (lit.kind != cursive0::syntax::TokenKind::BoolLiteral) {
      return std::nullopt;
    }
    SPEC_RULE("Encode-Bool");
    const bool value = lit.lexeme == "true";
    return LEBytesU64(value ? 1 : 0, 1);
  }
  if (prim->name == "char") {
    if (lit.kind != cursive0::syntax::TokenKind::CharLiteral) {
      return std::nullopt;
    }
    SPEC_RULE("Encode-Char");
    const auto codepoint = DecodeCharLiteral(lit.lexeme);
    if (!codepoint.has_value()) {
      return std::nullopt;
    }
    return LEBytesU64(*codepoint, 4);
  }

  if (prim->name == "()") {
    SPEC_RULE("Encode-Unit");
    return std::vector<std::uint8_t>{};
  }
  if (prim->name == "!") {
    SPEC_RULE("Encode-Never");
    return std::vector<std::uint8_t>{};
  }

  if (lit.kind == cursive0::syntax::TokenKind::IntLiteral) {
    if (!IsIntTypeName(prim->name)) {
      return std::nullopt;
    }
    SPEC_RULE("Encode-Int");
    const auto value = ParseIntLiteralValue(lit.lexeme);
    if (!value.has_value()) {
      return std::nullopt;
    }
    const auto size = PrimSize(prim->name);
    if (!size.has_value()) {
      return std::nullopt;
    }
    return LEBytes(*value, *size);
  }

  if (lit.kind == cursive0::syntax::TokenKind::FloatLiteral) {
    if (!IsFloatTypeName(prim->name)) {
      return std::nullopt;
    }
    SPEC_RULE("Encode-Float");
    const auto value = ParseFloatLiteralValue(lit.lexeme);
    if (!value.has_value()) {
      return std::nullopt;
    }
    if (prim->name == "f64") {
      const double v = static_cast<double>(*value);
      const std::uint64_t bits = std::bit_cast<std::uint64_t>(v);
      return LEBytesU64(bits, 8);
    }
    if (prim->name == "f32") {
      const float v = static_cast<float>(*value);
      const std::uint32_t bits = std::bit_cast<std::uint32_t>(v);
      return LEBytesU64(bits, 4);
    }
    if (prim->name == "f16") {
      const float v = static_cast<float>(*value);
      const std::uint16_t bits = FloatToHalf(v);
      return LEBytesU64(bits, 2);
    }
  }

  return std::nullopt;
}

bool ValidValue(const cursive0::sema::ScopeContext& ctx,
                const cursive0::sema::TypeRef& type,
                const std::vector<std::uint8_t>& bits) {
  if (!type) {
    return false;
  }
  if (const auto* prim = std::get_if<cursive0::sema::TypePrim>(&type->node)) {
    if (prim->name == "bool") {
      SPEC_RULE("Valid-Bool");
      return bits == std::vector<std::uint8_t>{0x00} ||
             bits == std::vector<std::uint8_t>{0x01};
    }
    if (prim->name == "char") {
      SPEC_RULE("Valid-Char");
      if (bits.size() != 4) {
        return false;
      }
      const auto value = BitsToUInt(bits);
      if (!value.has_value()) {
        return false;
      }
      return IsUnicodeScalar(static_cast<std::uint32_t>(*value));
    }
    if (prim->name == "()") {
      SPEC_RULE("Valid-Unit");
      return bits.empty();
    }
    if (prim->name == "!") {
      SPEC_RULE("Valid-Never");
      return false;
    }
    SPEC_RULE("Valid-Scalar");
    const auto size = PrimSize(prim->name);
    return size.has_value() && bits.size() == *size;
  }
  if (const auto* perm = std::get_if<cursive0::sema::TypePerm>(&type->node)) {
    return ValidValue(ctx, perm->base, bits);
  }
  if (const auto* ptr = std::get_if<cursive0::sema::TypePtr>(&type->node)) {
    const auto zero = LEBytesU64(0, kPtrSize);
    if (ptr->state == cursive0::sema::PtrState::Valid) {
      return bits.size() == kPtrSize && bits != zero;
    }
    if (ptr->state == cursive0::sema::PtrState::Null) {
      return bits == zero;
    }
    return bits.size() == kPtrSize;
  }
  if (std::holds_alternative<cursive0::sema::TypeRawPtr>(type->node)) {
    return bits.size() == kPtrSize;
  }
  if (const auto* tuple = std::get_if<cursive0::sema::TypeTuple>(&type->node)) {
    const auto layout = RecordLayoutOf(ctx, tuple->elements);
    if (!layout.has_value()) {
      return false;
    }
    return ValidStructBits(ctx, tuple->elements, layout->offsets,
                           layout->layout.size, bits);
  }
  if (const auto* array = std::get_if<cursive0::sema::TypeArray>(&type->node)) {
    const auto elem_size = SizeOf(ctx, array->element);
    if (!elem_size.has_value()) {
      return false;
    }
    const std::size_t elem_sz = static_cast<std::size_t>(*elem_size);
    if (bits.size() != elem_sz * array->length) {
      return false;
    }
    for (std::size_t i = 0; i < array->length; ++i) {
      const auto slice = SliceBytes(bits, i * elem_sz, elem_sz);
      if (!slice.has_value()) {
        return false;
      }
      if (!ValidValue(ctx, array->element, *slice)) {
        return false;
      }
    }
    return true;
  }
  if (std::holds_alternative<cursive0::sema::TypeSlice>(type->node)) {
    return bits.size() == 2 * kPtrSize;
  }
  if (std::holds_alternative<cursive0::sema::TypeRange>(type->node)) {
    const auto layout = RangeLayoutOf(ctx);
    if (!layout.has_value()) {
      return false;
    }
    std::vector<cursive0::sema::TypeRef> types;
    types.push_back(cursive0::sema::MakeTypePrim("u8"));
    types.push_back(cursive0::sema::MakeTypePrim("usize"));
    types.push_back(cursive0::sema::MakeTypePrim("usize"));
    if (!ValidStructBits(ctx, types, layout->offsets, layout->layout.size,
                         bits)) {
      return false;
    }
    const auto kind_size = PrimSize("u8");
    if (!kind_size.has_value()) {
      return false;
    }
    const auto kind_bits =
        SliceBytes(bits, static_cast<std::size_t>(layout->offsets[0]),
                   static_cast<std::size_t>(*kind_size));
    if (!kind_bits.has_value()) {
      return false;
    }
    const auto kind_value = BitsToUInt(*kind_bits);
    if (!kind_value.has_value()) {
      return false;
    }
    return *kind_value <= 5;
  }
  if (const auto* modal_state =
          std::get_if<cursive0::sema::TypeModalState>(&type->node)) {
    cursive0::syntax::Path syntax_path;
    syntax_path.reserve(modal_state->path.size());
    for (const auto& comp : modal_state->path) {
      syntax_path.push_back(comp);
    }
    const auto it = ctx.sigma.types.find(cursive0::sema::PathKeyOf(syntax_path));
    if (it == ctx.sigma.types.end()) {
      return false;
    }
    const auto* decl = std::get_if<cursive0::syntax::ModalDecl>(&it->second);
    if (!decl) {
      return false;
    }
    const auto* state_block = FindState(*decl, modal_state->state);
    if (!state_block) {
      return false;
    }
    const auto fields = CollectStateFields(ctx, *state_block);
    if (!fields.has_value()) {
      return false;
    }
    std::vector<cursive0::sema::TypeRef> types;
    types.reserve(fields->size());
    for (const auto& field : *fields) {
      types.push_back(field.second);
    }
    const auto layout = RecordLayoutOf(ctx, types);
    if (!layout.has_value()) {
      return false;
    }
    return ValidStructBits(ctx, types, layout->offsets, layout->layout.size,
                           bits);
  }
  if (const auto* path =
          std::get_if<cursive0::sema::TypePathType>(&type->node)) {
    cursive0::syntax::Path syntax_path;
    syntax_path.reserve(path->path.size());
    for (const auto& comp : path->path) {
      syntax_path.push_back(comp);
    }
    const auto it = ctx.sigma.types.find(cursive0::sema::PathKeyOf(syntax_path));
    if (it == ctx.sigma.types.end()) {
      return false;
    }
    if (const auto* alias =
            std::get_if<cursive0::syntax::TypeAliasDecl>(&it->second)) {
      const auto lowered = LowerTypeForLayout(ctx, alias->type);
      if (!lowered.has_value()) {
        return false;
      }
      return ValidValue(ctx, *lowered, bits);
    }
    if (const auto* record =
            std::get_if<cursive0::syntax::RecordDecl>(&it->second)) {
      std::vector<cursive0::sema::TypeRef> types;
      for (const auto& member : record->members) {
        if (const auto* field =
                std::get_if<cursive0::syntax::FieldDecl>(&member)) {
          const auto lowered = LowerTypeForLayout(ctx, field->type);
          if (!lowered.has_value()) {
            return false;
          }
          types.push_back(*lowered);
        }
      }
      const auto layout = RecordLayoutOf(ctx, types);
      if (!layout.has_value()) {
        return false;
      }
      return ValidStructBits(ctx, types, layout->offsets, layout->layout.size,
                             bits);
    }
    if (const auto* enum_decl =
            std::get_if<cursive0::syntax::EnumDecl>(&it->second)) {
      return ValidEnumBits(ctx, *enum_decl, bits);
    }
    if (const auto* modal =
            std::get_if<cursive0::syntax::ModalDecl>(&it->second)) {
      return ValidModalBits(ctx, *modal, bits);
    }
    return false;
  }
  if (const auto* uni = std::get_if<cursive0::sema::TypeUnion>(&type->node)) {
    return ValidUnionBits(ctx, *uni, bits);
  }
  if (std::holds_alternative<cursive0::sema::TypeDynamic>(type->node)) {
    const auto dyn = DynLayoutOf();
    return bits.size() == dyn.layout.size;
  }
  if (std::holds_alternative<cursive0::sema::TypeString>(type->node)) {
    const auto size = SizeOf(ctx, type);
    return size.has_value() && bits.size() == *size;
  }
  if (std::holds_alternative<cursive0::sema::TypeBytes>(type->node)) {
    const auto size = SizeOf(ctx, type);
    return size.has_value() && bits.size() == *size;
  }
  return false;
}

std::optional<std::vector<std::uint8_t>> ValueBits(
    const cursive0::sema::ScopeContext& ctx,
    const cursive0::sema::TypeRef& type,
    const Value& value) {
  if (!type) {
    return std::nullopt;
  }

  if (const auto* prim = std::get_if<cursive0::sema::TypePrim>(&type->node)) {
    if (prim->name == "bool") {
      if (const auto* v = std::get_if<BoolVal>(&value.node)) {
        return LEBytesU64(v->value ? 1 : 0, 1);
      }
    }
    if (prim->name == "char") {
      if (const auto* v = std::get_if<CharVal>(&value.node)) {
        return LEBytesU64(v->value, 4);
      }
    }
    if (prim->name == "()") {
      if (std::holds_alternative<UnitVal>(value.node)) {
        return std::vector<std::uint8_t>{};
      }
    }
    if (prim->name == "!") {
      return std::nullopt;
    }
    if (const auto* v = std::get_if<IntVal>(&value.node)) {
      if (v->type == prim->name) {
        const auto size = PrimSize(prim->name);
        if (!size.has_value()) {
          return std::nullopt;
        }
        return LEBytes(v->value, *size);
      }
    }
    if (const auto* v = std::get_if<FloatVal>(&value.node)) {
      if (v->type == prim->name) {
        const auto size = PrimSize(prim->name);
        if (!size.has_value()) {
          return std::nullopt;
        }
        return LEBytesU64(v->bits, *size);
      }
    }
    return std::nullopt;
  }

  if (const auto* perm = std::get_if<cursive0::sema::TypePerm>(&type->node)) {
    return ValueBits(ctx, perm->base, value);
  }

  if (const auto* ptr = std::get_if<cursive0::sema::TypePtr>(&type->node)) {
    if (const auto* v = std::get_if<PtrVal>(&value.node)) {
      if (ptr->state.has_value() && v->state != *ptr->state) {
        return std::nullopt;
      }
      if (v->state == cursive0::sema::PtrState::Valid && v->addr == 0) {
        return std::nullopt;
      }
      if (v->state == cursive0::sema::PtrState::Null && v->addr != 0) {
        return std::nullopt;
      }
      return LEBytesU64(v->addr, kPtrSize);
    }
    return std::nullopt;
  }

  if (const auto* raw = std::get_if<cursive0::sema::TypeRawPtr>(&type->node)) {
    if (const auto* v = std::get_if<RawPtrVal>(&value.node)) {
      if (v->qual != raw->qual) {
        return std::nullopt;
      }
      return LEBytesU64(v->addr, kPtrSize);
    }
    return std::nullopt;
  }

  if (const auto* tuple = std::get_if<cursive0::sema::TypeTuple>(&type->node)) {
    const auto* v = std::get_if<TupleVal>(&value.node);
    if (!v || v->elements.size() != tuple->elements.size()) {
      return std::nullopt;
    }
    std::vector<std::vector<std::uint8_t>> bits;
    bits.reserve(tuple->elements.size());
    for (std::size_t i = 0; i < tuple->elements.size(); ++i) {
      const auto elem_bits = ValueBits(ctx, tuple->elements[i], v->elements[i]);
      if (!elem_bits.has_value()) {
        return std::nullopt;
      }
      bits.push_back(*elem_bits);
    }
    const auto layout = RecordLayoutOf(ctx, tuple->elements);
    if (!layout.has_value()) {
      return std::nullopt;
    }
    return StructBits(bits, layout->offsets, layout->layout.size);
  }

  if (const auto* array = std::get_if<cursive0::sema::TypeArray>(&type->node)) {
    const auto* v = std::get_if<ArrayVal>(&value.node);
    if (!v || v->elements.size() != array->length) {
      return std::nullopt;
    }
    const auto elem_size = SizeOf(ctx, array->element);
    if (!elem_size.has_value()) {
      return std::nullopt;
    }
    std::vector<std::uint8_t> out;
    out.reserve(static_cast<std::size_t>(*elem_size * array->length));
    for (const auto& elem : v->elements) {
      const auto elem_bits = ValueBits(ctx, array->element, elem);
      if (!elem_bits.has_value()) {
        return std::nullopt;
      }
      out.insert(out.end(), elem_bits->begin(), elem_bits->end());
    }
    return out;
  }

  if (std::holds_alternative<cursive0::sema::TypeSlice>(type->node)) {
    const auto* v = std::get_if<SliceVal>(&value.node);
    if (!v) {
      return std::nullopt;
    }
    if (v->ptr.qual != cursive0::sema::RawPtrQual::Imm) {
      return std::nullopt;
    }
    std::vector<std::uint8_t> bits;
    const auto ptr_bits = LEBytesU64(v->ptr.addr, kPtrSize);
    const auto len_bits = LEBytesU64(v->length, kPtrSize);
    bits.insert(bits.end(), ptr_bits.begin(), ptr_bits.end());
    bits.insert(bits.end(), len_bits.begin(), len_bits.end());
    return bits;
  }

  if (std::holds_alternative<cursive0::sema::TypeRange>(type->node)) {
    const auto* v = std::get_if<ValueRangeVal>(&value.node);
    if (!v) {
      return std::nullopt;
    }
    std::uint8_t tag = 2;
    switch (v->kind) {
      case ValueRangeKind::To:
        tag = 0;
        break;
      case ValueRangeKind::ToInclusive:
        tag = 1;
        break;
      case ValueRangeKind::Full:
        tag = 2;
        break;
      case ValueRangeKind::From:
        tag = 3;
        break;
      case ValueRangeKind::Exclusive:
        tag = 4;
        break;
      case ValueRangeKind::Inclusive:
        tag = 5;
        break;
    }
    const std::uint64_t lo = v->lo.value_or(0);
    const std::uint64_t hi = v->hi.value_or(0);
    std::vector<std::pair<std::string, cursive0::sema::TypeRef>> fields;
    fields.emplace_back("kind", cursive0::sema::MakeTypePrim("u8"));
    fields.emplace_back("lo", cursive0::sema::MakeTypePrim("usize"));
    fields.emplace_back("hi", cursive0::sema::MakeTypePrim("usize"));
    RecordVal rv;
    rv.fields.push_back({"kind",
                         Value{IntVal{"u8", core::UInt128FromU64(tag)}}});
    rv.fields.push_back(
        {"lo", Value{IntVal{"usize", core::UInt128FromU64(lo)}}});
    rv.fields.push_back(
        {"hi", Value{IntVal{"usize", core::UInt128FromU64(hi)}}});
    return ValueBitsForRecord(ctx, fields, rv);
  }

  if (const auto* modal_state =
          std::get_if<cursive0::sema::TypeModalState>(&type->node)) {
    const auto* record_val = std::get_if<RecordVal>(&value.node);
    if (!record_val) {
      return std::nullopt;
    }
    cursive0::syntax::Path syntax_path;
    syntax_path.reserve(modal_state->path.size());
    for (const auto& comp : modal_state->path) {
      syntax_path.push_back(comp);
    }
    const auto it = ctx.sigma.types.find(cursive0::sema::PathKeyOf(syntax_path));
    if (it == ctx.sigma.types.end()) {
      return std::nullopt;
    }
    const auto* decl = std::get_if<cursive0::syntax::ModalDecl>(&it->second);
    if (!decl) {
      return std::nullopt;
    }
    const auto* state_block = FindState(*decl, modal_state->state);
    if (!state_block) {
      return std::nullopt;
    }
    const auto fields = CollectStateFields(ctx, *state_block);
    if (!fields.has_value()) {
      return std::nullopt;
    }
    return ValueBitsForRecord(ctx, *fields, *record_val);
  }

  if (const auto* path =
          std::get_if<cursive0::sema::TypePathType>(&type->node)) {
    cursive0::syntax::Path syntax_path;
    syntax_path.reserve(path->path.size());
    for (const auto& comp : path->path) {
      syntax_path.push_back(comp);
    }
    const auto it = ctx.sigma.types.find(cursive0::sema::PathKeyOf(syntax_path));
    if (it == ctx.sigma.types.end()) {
      return std::nullopt;
    }
    if (const auto* alias =
            std::get_if<cursive0::syntax::TypeAliasDecl>(&it->second)) {
      const auto lowered = LowerTypeForLayout(ctx, alias->type);
      if (!lowered.has_value()) {
        return std::nullopt;
      }
      return ValueBits(ctx, *lowered, value);
    }
    if (const auto* record =
            std::get_if<cursive0::syntax::RecordDecl>(&it->second)) {
      const auto* v = std::get_if<RecordVal>(&value.node);
      if (!v) {
        return std::nullopt;
      }
      std::vector<std::pair<std::string, cursive0::sema::TypeRef>> fields;
      for (const auto& member : record->members) {
        if (const auto* field =
                std::get_if<cursive0::syntax::FieldDecl>(&member)) {
          const auto lowered = LowerTypeForLayout(ctx, field->type);
          if (!lowered.has_value()) {
            return std::nullopt;
          }
          fields.emplace_back(field->name, *lowered);
        }
      }
      return ValueBitsForRecord(ctx, fields, *v);
    }
    if (const auto* enum_decl =
            std::get_if<cursive0::syntax::EnumDecl>(&it->second)) {
      const auto* v = std::get_if<EnumVal>(&value.node);
      if (!v) {
        return std::nullopt;
      }
      const auto layout = EnumLayoutOf(ctx, *enum_decl);
      if (!layout.has_value()) {
        return std::nullopt;
      }
      std::size_t index = 0;
      bool found = false;
      for (std::size_t i = 0; i < enum_decl->variants.size(); ++i) {
        if (cursive0::sema::IdEq(enum_decl->variants[i].name, v->variant)) {
          index = i;
          found = true;
          break;
        }
      }
      if (!found) {
        return std::nullopt;
      }
      const auto discs = cursive0::sema::EnumDiscriminants(*enum_decl);
      if (!discs.ok || discs.discs.size() <= index) {
        return std::nullopt;
      }
      const std::uint64_t disc_value = discs.discs[index];
      const auto disc_bits =
          LEBytesU64(disc_value, PrimSize(layout->disc_type).value_or(1));
      const auto payload_bits = EnumPayloadBits(ctx, *enum_decl,
                                                enum_decl->variants[index],
                                                v->payload,
                                                layout->payload_size);
      if (!payload_bits.has_value()) {
        return std::nullopt;
      }
      const auto disc_size = PrimSize(layout->disc_type);
      const auto disc_align = PrimAlign(layout->disc_type);
      if (!disc_size.has_value() || !disc_align.has_value()) {
        return std::nullopt;
      }
      return TaggedBits(disc_bits, *payload_bits, *disc_size,
                        layout->payload_size, layout->payload_align,
                        layout->layout.size);
    }
    if (const auto* modal =
            std::get_if<cursive0::syntax::ModalDecl>(&it->second)) {
      const auto* v = std::get_if<ModalVal>(&value.node);
      if (!v || !v->payload) {
        return std::nullopt;
      }
      const Value& modal_payload = *v->payload;
      if (cursive0::sema::NicheApplies(ctx, *modal)) {
        const auto niche_bits =
            ModalNicheBits(ctx, *modal, v->state, modal_payload);
        if (niche_bits.has_value()) {
          return niche_bits;
        }
      }
      return ModalTaggedBits(ctx, *modal, v->state, modal_payload);
    }
  }

  if (const auto* uni = std::get_if<cursive0::sema::TypeUnion>(&type->node)) {
    const auto* v = std::get_if<UnionVal>(&value.node);
    if (!v || !v->value) {
      return std::nullopt;
    }
    const Value& union_value = *v->value;
    const auto layout = UnionLayoutOf(ctx, *uni);
    if (!layout.has_value()) {
      return std::nullopt;
    }
    if (layout->niche) {
      const auto& members = layout->member_list;
      if (members.empty()) {
        return std::nullopt;
      }
      std::optional<std::size_t> payload_index;
      std::uint64_t niche_count = 0;
      for (std::size_t i = 0; i < members.size(); ++i) {
        if (IsUnitType(members[i])) {
          continue;
        }
        const auto count = NicheCount(ctx, members[i]);
        if (count == 0) {
          continue;
        }
        if (payload_index.has_value()) {
          payload_index.reset();
          break;
        }
        payload_index = i;
        niche_count = count;
      }
      if (!payload_index.has_value()) {
        return std::nullopt;
      }
      bool niche_applies = true;
      for (std::size_t i = 0; i < members.size(); ++i) {
        if (i == *payload_index) {
          continue;
        }
        if (!IsUnitType(members[i])) {
          niche_applies = false;
          break;
        }
      }
      if (niche_count < members.size() - 1) {
        niche_applies = false;
      }
      if (!niche_applies) {
        return std::nullopt;
      }

      std::optional<std::size_t> member_index;
      for (std::size_t i = 0; i < members.size(); ++i) {
        if (cursive0::sema::TypeEquiv(v->member, members[i]).equiv) {
          member_index = i;
          break;
        }
      }
      if (!member_index.has_value()) {
        return std::nullopt;
      }

      if (*member_index == *payload_index) {
        const auto payload_bits = ValueBits(ctx, v->member, union_value);
        if (!payload_bits.has_value()) {
          return std::nullopt;
        }
        if (IsNicheBits(ctx, members[*payload_index], *payload_bits)) {
          return std::nullopt;
        }
        return payload_bits;
      }

      if (!IsUnitType(members[*member_index])) {
        return std::nullopt;
      }
      if (!std::holds_alternative<UnitVal>(union_value.node)) {
        if (const auto* record = std::get_if<RecordVal>(&union_value.node)) {
          if (!record->fields.empty()) {
            return std::nullopt;
          }
        } else {
          return std::nullopt;
        }
      }

      std::size_t empty_index = 0;
      bool found_empty = false;
      for (std::size_t i = 0; i < members.size(); ++i) {
        if (i == *payload_index) {
          continue;
        }
        if (!IsUnitType(members[i])) {
          continue;
        }
        if (i == *member_index) {
          found_empty = true;
          break;
        }
        ++empty_index;
      }
      if (!found_empty) {
        return std::nullopt;
      }
      return NicheBitsForIndex(ctx, members[*payload_index], empty_index);
    }
    std::size_t index = 0;
    bool found = false;
    for (std::size_t i = 0; i < layout->member_list.size(); ++i) {
      if (cursive0::sema::TypeEquiv(v->member, layout->member_list[i]).equiv) {
        index = i;
        found = true;
        break;
      }
    }
    if (!found || !layout->disc_type.has_value()) {
      return std::nullopt;
    }
    const auto payload_bits = ValueBits(ctx, v->member, union_value);
    if (!payload_bits.has_value()) {
      return std::nullopt;
    }
    const auto padded = PadBytes(*payload_bits, layout->payload_size);
    const auto disc_size = PrimSize(*layout->disc_type);
    const auto disc_align = PrimAlign(*layout->disc_type);
    if (!disc_size.has_value() || !disc_align.has_value()) {
      return std::nullopt;
    }
    const auto disc_bits = LEBytesU64(index, *disc_size);
    return TaggedBits(disc_bits, padded, *disc_size, layout->payload_size,
                      layout->payload_align, layout->layout.size);
  }

  if (std::holds_alternative<cursive0::sema::TypeDynamic>(type->node)) {
    const auto* v = std::get_if<DynamicVal>(&value.node);
    if (!v) {
      return std::nullopt;
    }
    const auto dyn = DynLayoutOf();
    std::vector<std::vector<std::uint8_t>> bits;
    bits.push_back(LEBytesU64(v->data, kPtrSize));
    bits.push_back(LEBytesU64(v->vtable, kPtrSize));
    std::vector<std::uint64_t> offsets = {0, kPtrSize};
    return StructBits(bits, offsets, dyn.layout.size);
  }

  if (std::holds_alternative<cursive0::sema::TypeString>(type->node)) {
    if (!std::holds_alternative<StringVal>(value.node)) {
      return std::nullopt;
    }
    const auto size = SizeOf(ctx, type);
    if (!size.has_value()) {
      return std::nullopt;
    }
    return std::vector<std::uint8_t>(*size, 0);
  }

  if (std::holds_alternative<cursive0::sema::TypeBytes>(type->node)) {
    if (!std::holds_alternative<BytesVal>(value.node)) {
      return std::nullopt;
    }
    const auto size = SizeOf(ctx, type);
    if (!size.has_value()) {
      return std::nullopt;
    }
    return std::vector<std::uint8_t>(*size, 0);
  }

  return std::nullopt;
}

}  // namespace cursive0::codegen
