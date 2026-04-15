// =============================================================================
// MIGRATION MAPPING: layout_ranges.cpp
// =============================================================================
//
// SPEC REFERENCE: CursiveSpecification.md
//   - Section 6.1.6 Aggregate Layouts - Ranges (lines 15015-15048)
//   - Typed range family layouts:
//     * Range<T>, RangeInclusive<T> => [T, T]
//     * RangeFrom<T>, RangeTo<T>, RangeToInclusive<T> => [T]
//     * RangeFull => []
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
//   1. Layout is computed from the concrete range type constructor.
//   2. Range<T>/RangeInclusive<T> use two fields of T.
//   3. RangeFrom<T>/RangeTo<T>/RangeToInclusive<T> use one field of T.
//   4. RangeFull is empty (size 0, align 1).
// =============================================================================

#include "05_codegen/layout/layout.h"

#include "00_core/assert_spec.h"

// The RangeLayoutOf implementation is in layout_aggregates.cpp.
// This file exists to document the range layout specification.
//
// RangeLayoutOf builds synthetic field lists from concrete range constructors and
// applies RecordLayoutOf to compute each layout.

namespace cursive::codegen {

// RangeLayoutOf is implemented in layout_aggregates.cpp

}  // namespace cursive::codegen
