// =============================================================================
// MIGRATION MAPPING: layout_ranges.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   - Section 6.1.6 Aggregate Layouts - Ranges (lines 15015-15048)
//   - RangeTag definitions (lines 15017-15022)
//   - RangeFields = [{kind, u8}, {lo, usize}, {hi, usize}] (line 15023)
//   - OptVal helper (lines 15024-15026)
//   - RangeValFields (line 15027)
//   - Layout-Range rule (lines 15030-15033)
//   - Size-Range, Align-Range rules (lines 15035-15043)
//   - Layout-Range-SizeAlign rule (lines 15045-15048)
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/layout/layout_aggregates.cpp
//   - RangeLayoutInternal function (lines 47-55)
//   - RangeLayoutOf function (lines 59-62)
//
// DEPENDENCIES:
//   - cursive/include/05_codegen/layout/layout.h (RecordLayout struct)
//   - cursive/include/04_analysis/types/types.h (MakeTypePrim)
//   - RecordLayoutOf for field layout computation
//
// REFACTORING NOTES:
//   1. Range is represented as a 3-field record:
//      - kind: u8 (range variant tag)
//      - lo: usize (lower bound)
//      - hi: usize (upper bound)
//   2. RangeTag values:
//      - To = 0 (..hi)
//      - ToInclusive = 1 (..=hi)
//      - Full = 2 (..)
//      - From = 3 (lo..)
//      - Exclusive = 4 (lo..hi)
//      - Inclusive = 5 (lo..=hi)
//   3. Layout computed using RecordLayoutOf with synthetic fields
//   4. Absent bounds stored as 0 per OptVal
//   5. Size depends on platform pointer size (2*PtrSize + padding + u8)
//
// RANGE LAYOUT (64-bit):
//   - kind at offset 0 (1 byte)
//   - 7 bytes padding
//   - lo at offset 8 (8 bytes)
//   - hi at offset 16 (8 bytes)
//   - Total size: 24 bytes, alignment: 8 bytes
// =============================================================================

#include "cursive/05_codegen/layout/layout.h"

// TODO: Migrate implementation from cursive-bootstrap/src/04_codegen/layout/layout_aggregates.cpp
