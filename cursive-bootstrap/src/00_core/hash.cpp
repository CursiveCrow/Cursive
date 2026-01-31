#include "cursive0/00_core/hash.h"

#include "cursive0/00_core/assert_spec.h"
#include "cursive0/00_core/symbols.h"

namespace cursive0::core {

std::uint64_t FNV1a64(std::span<const std::uint8_t> bytes) {
  SPEC_DEF("FNV1a64", "6.3.1");
  SPEC_DEF("FNVOffset64", "6.3.1");
  SPEC_DEF("FNVPrime64", "6.3.1");

  std::uint64_t hash = kFNVOffset64;
  for (const std::uint8_t byte : bytes) {
    hash ^= byte;
    hash *= kFNVPrime64;
  }
  return hash;
}

std::uint64_t FNV1a64(std::string_view s) {
  return FNV1a64(std::span<const std::uint8_t>(
      reinterpret_cast<const std::uint8_t*>(s.data()), s.size()));
}

std::string Hex2(std::uint8_t byte) {
  SPEC_DEF("Hex2", "6.3.1");
  SPEC_DEF("HexDigit", "6.3.1");

  static constexpr char kHexDigits[] = "0123456789ABCDEF";
  std::string result;
  result.reserve(2);
  result.push_back(kHexDigits[(byte >> 4) & 0x0F]);
  result.push_back(kHexDigits[byte & 0x0F]);
  return result;
}

std::string Hex64(std::uint64_t hash) {
  SPEC_DEF("Hex64", "6.3.1");
  SPEC_DEF("LEBytes", "6.3.1");

  // Per §6.3.1:
  //   Hex64(h) = Join("", [Hex2(b1),...,Hex2(b8)]) where rev(LEBytes(h, 8)) = [b1,...,b8]
  //
  // LEBytes extracts bytes in little-endian order (LSB first).
  // rev() reverses that to get MSB first (big-endian order in the string).
  // So we print the most significant byte first.

  std::string result;
  result.reserve(16);

  // Extract bytes in big-endian order (MSB first)
  for (int i = 7; i >= 0; --i) {
    const std::uint8_t byte = static_cast<std::uint8_t>((hash >> (i * 8)) & 0xFF);
    result += Hex2(byte);
  }

  return result;
}

std::string LiteralID(std::string_view kind, std::span<const std::uint8_t> contents) {
  SPEC_DEF("LiteralID", "6.3.1");

  // Per §6.3.1:
  //   LiteralID(kind, contents) = mangle(kind) ++ "_" ++ Hex64(FNV1a64(contents))

  std::string result = Mangle(kind);
  result += '_';
  result += Hex64(FNV1a64(contents));
  return result;
}

}  // namespace cursive0::core
