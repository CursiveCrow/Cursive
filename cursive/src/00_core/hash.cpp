// =============================================================================
// MIGRATION MAPPING: hash.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   - Section 6.3.1 (referenced in source, covers symbol generation)
//     - FNV1a64: FNV-1a hash algorithm for 64-bit hashes
//     - FNVOffset64 = 14695981039346656037 (0xcbf29ce484222325)
//     - FNVPrime64 = 1099511628211 (0x100000001b3)
//     - Hex2: converts byte to 2-digit hex string
//     - Hex64: converts 64-bit hash to 16-digit hex string (MSB first)
//     - LEBytes: extracts bytes in little-endian order
//     - LiteralID(kind, contents) = mangle(kind) ++ "_" ++ Hex64(FNV1a64(contents))
//
// SOURCE FILE: cursive-bootstrap/src/00_core/hash.cpp
//   - Lines 1-73 (entire file)
//
// CONTENT TO MIGRATE:
//   - FNV1a64(bytes) -> uint64_t (lines 8-19)
//     Implements FNV-1a hash: hash = offset; for each byte: hash ^= byte; hash *= prime
//   - FNV1a64(string_view) -> uint64_t (lines 21-24)
//     Convenience overload converting string to byte span
//   - Hex2(byte) -> string (lines 26-36)
//     Converts single byte to 2-char uppercase hex string
//     Uses HexDigit lookup: "0123456789ABCDEF"
//   - Hex64(hash) -> string (lines 38-59)
//     Converts 64-bit hash to 16-char hex string
//     Per spec: rev(LEBytes(h, 8)) -> outputs MSB first (big-endian string)
//   - LiteralID(kind, contents) -> string (lines 61-71)
//     Generates unique ID for literals: mangle(kind) + "_" + Hex64(FNV1a64(contents))
//
// DEPENDENCIES:
//   - cursive/include/00_core/hash.h (header)
//     - kFNVOffset64 constant
//     - kFNVPrime64 constant
//   - cursive/include/00_core/assert_spec.h
//     - SPEC_DEF macro
//   - cursive/include/00_core/symbols.h
//     - Mangle() function for PathToPrefix encoding
//   - <span> for std::span<const uint8_t>
//   - <cstdint> for uint64_t, uint8_t
//
// REFACTORING NOTES:
//   1. FNV-1a algorithm: XOR then multiply (not multiply then XOR)
//   2. Hex64 outputs big-endian (MSB first) per spec's rev(LEBytes(...))
//   3. Hex2 uses uppercase hex digits - maintain consistency
//   4. SPEC_DEF traces to "6.3.1" for all functions
//   5. LiteralID used for string literal symbol generation
//   6. Consider constexpr for Hex2 if C++20 string is available
//   7. span<const uint8_t> matches spec's Bytes type
//
// =============================================================================
