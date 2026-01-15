#include <cassert>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/hash.h"

namespace {

using cursive0::core::FNV1a64;
using cursive0::core::Hex2;
using cursive0::core::Hex64;
using cursive0::core::kFNVOffset64;
using cursive0::core::kFNVPrime64;
using cursive0::core::LiteralID;

// FNV-1a test vectors
// Reference: http://www.isthe.com/chongo/tech/comp/fnv/

void TestFNV1aConstants() {
  SPEC_COV("FNVOffset64");
  SPEC_COV("FNVPrime64");

  // Verify the constants match the spec
  assert(kFNVOffset64 == 14695981039346656037ULL);
  assert(kFNVPrime64 == 1099511628211ULL);
}

void TestFNV1aEmpty() {
  SPEC_COV("FNV1a64");

  // FNV1a64([]) = FNVOffset64
  const std::vector<std::uint8_t> empty;
  assert(FNV1a64(std::span<const std::uint8_t>(empty)) == kFNVOffset64);
  assert(FNV1a64("") == kFNVOffset64);
}

void TestFNV1aKnownVectors() {
  SPEC_COV("FNV1a64");

  // FNV-1a test vectors for 64-bit
  // These are well-known test cases for FNV-1a 64-bit

  // "a" -> 0xaf63dc4c8601ec8c
  assert(FNV1a64("a") == 0xaf63dc4c8601ec8cULL);

  // "b" -> 0xaf63df4c8601f1a5
  assert(FNV1a64("b") == 0xaf63df4c8601f1a5ULL);

  // "ab" -> 0x089c4407b545986a
  assert(FNV1a64("ab") == 0x089c4407b545986aULL);

  // "abc" -> 0xe71fa2190541574b
  assert(FNV1a64("abc") == 0xe71fa2190541574bULL);

  // "foobar" -> 0x85944171f73967e8
  assert(FNV1a64("foobar") == 0x85944171f73967e8ULL);
}

void TestHex2() {
  SPEC_COV("Hex2");
  SPEC_COV("HexDigit");

  // Test all nibble values
  assert(Hex2(0x00) == "00");
  assert(Hex2(0x0F) == "0F");
  assert(Hex2(0x10) == "10");
  assert(Hex2(0x1F) == "1F");
  assert(Hex2(0xAB) == "AB");
  assert(Hex2(0xFF) == "FF");
  assert(Hex2(0x42) == "42");
  assert(Hex2(0xDE) == "DE");
}

void TestHex64() {
  SPEC_COV("Hex64");
  SPEC_COV("LEBytes");

  // Hex64(h) = Join("", [Hex2(b1),...,Hex2(b8)]) where rev(LEBytes(h, 8)) = [b1,...,b8]
  // This means MSB first (big-endian representation in the string)

  // Zero
  assert(Hex64(0x0000000000000000ULL) == "0000000000000000");

  // All F's
  assert(Hex64(0xFFFFFFFFFFFFFFFFULL) == "FFFFFFFFFFFFFFFF");

  // Sequential bytes (MSB = 01, LSB = 08)
  assert(Hex64(0x0102030405060708ULL) == "0102030405060708");

  // The FNV offset (verify it's displayed correctly)
  // 14695981039346656037 = 0xCBF29CE484222325
  assert(Hex64(kFNVOffset64) == "CBF29CE484222325");

  // Simple value
  assert(Hex64(0x123456789ABCDEF0ULL) == "123456789ABCDEF0");
}

void TestLiteralID() {
  SPEC_COV("LiteralID");

  // LiteralID(kind, contents) = mangle(kind) ++ "_" ++ Hex64(FNV1a64(contents))

  // Empty contents should give FNVOffset64 hash
  const std::vector<std::uint8_t> empty;
  const std::string literal_id_empty = LiteralID("string", std::span<const std::uint8_t>(empty));

  // "string" mangled + "_" + Hex64(FNVOffset64)
  // FNVOffset64 = 0xCBF29CE484222325
  assert(literal_id_empty == "string_CBF29CE484222325");

  // Test with some actual content
  const std::vector<std::uint8_t> hello = {'h', 'e', 'l', 'l', 'o'};
  const std::string literal_id_hello = LiteralID("bytes", std::span<const std::uint8_t>(hello));

  // "bytes" mangled + "_" + Hex64(FNV1a64("hello"))
  // FNV1a64("hello") needs to be computed
  const std::uint64_t hello_hash = FNV1a64("hello");
  assert(literal_id_hello == "bytes_" + Hex64(hello_hash));
}

void TestFNV1aStringViewEquivalence() {
  SPEC_COV("FNV1a64");

  // Verify that string_view and span<uint8_t> produce same results
  const std::string test_str = "test string for hashing";
  const std::vector<std::uint8_t> test_bytes(test_str.begin(), test_str.end());

  assert(FNV1a64(test_str) == FNV1a64(std::span<const std::uint8_t>(test_bytes)));
}

}  // namespace

int main() {
  TestFNV1aConstants();
  TestFNV1aEmpty();
  TestFNV1aKnownVectors();
  TestHex2();
  TestHex64();
  TestLiteralID();
  TestFNV1aStringViewEquivalence();

  return 0;
}
