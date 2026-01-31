// =============================================================================
// MIGRATION MAPPING: layout_value_bits.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   - Section 6.1.4.1 ValueBits specifications (lines 14729-14770)
//   - LEBytes definition (line 14416)
//   - FloatBits_t for IEEE754 encoding (line 14417)
//   - StructBits for record encoding (line 14732)
//   - PadBytes for padding (line 14733)
//   - ValueBits for each type variant (lines 14735-14770)
//   - UnionBits, UnionNicheBits, UnionTaggedBits (lines 14771-14814)
//   - ModalBits, ModalNicheBits, ModalTaggedBits (lines 14816-14834)
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/layout/layout_value_bits.cpp
//   - ValueBitsOf functions for constant encoding
//   - StructBits helper for record field packing
//   - Union/Modal encoding helpers
//
// DEPENDENCIES:
//   - cursive/include/05_codegen/layout/layout.h (Layout structs)
//   - cursive/include/04_analysis/types/types.h (TypeRef, value types)
//   - IEEE 754 bit manipulation for floats
//   - Little-endian byte encoding
//
// REFACTORING NOTES:
//   1. LEBytes(v, n) encodes value v as n little-endian bytes
//   2. BoolByte: false=0x00, true=0x01
//   3. Char: UTF-32 codepoint as 4 LE bytes
//   4. Integers: LE encoding at type width
//   5. Floats: IEEE 754 bit pattern, LE encoded
//   6. Unit: empty byte array []
//   7. Never: empty byte array (unreachable)
//   8. Pointers: address as PtrSize LE bytes
//   9. Records: StructBits packs fields at offsets
//   10. Unions: niche or tagged encoding
//   11. Modals: niche or tagged encoding per state
//
// ENCODING FUNCTIONS:
//   - LEBytes(value, size) -> bytes
//   - StructBits(types, values, offsets, size) -> bytes
//   - PadBytes(content, total_size) -> bytes
//   - ValueBits(type, value) -> bytes
// =============================================================================

#include "cursive/05_codegen/layout/layout.h"

// TODO: Migrate implementation from cursive-bootstrap/src/04_codegen/layout/layout_value_bits.cpp
