// =============================================================================
// MIGRATION MAPPING: layout_dispatch.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   - Section 6.1 Layout and Representation (lines 14349-15153)
//   - LayoutJudg = {sizeof, alignof, layout} (line 14397)
//   - All layout rules from sections 6.1.1 through 6.1.8
//   - Permission layout pass-through (lines 14479-14492)
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/layout/layout_dispatch.cpp
//   - SizeOf dispatch function
//   - AlignOf dispatch function
//   - LayoutOf dispatch function
//   - LowerTypeForLayout for type alias resolution
//
// DEPENDENCIES:
//   - cursive/include/05_codegen/layout/layout.h (Layout struct)
//   - cursive/include/04_analysis/types/types.h (all type variants)
//   - All layout_*.cpp files for specific type handling
//
// REFACTORING NOTES:
//   1. Central dispatch for sizeof, alignof, layout queries
//   2. Type visitor pattern over TypeRef variants:
//      - TypePrim -> layout_primitives.cpp
//      - TypePerm -> unwrap permission, recurse
//      - TypePtr, TypeRawPtr -> PtrSize, PtrAlign
//      - TypeFunc -> PtrSize, PtrAlign (function pointer)
//      - TypePathType -> lookup record/enum/alias
//      - TypeTuple -> layout_tuples.cpp
//      - TypeArray -> element_size * length
//      - TypeSlice -> 2 * PtrSize
//      - TypeUnion -> layout_unions.cpp
//      - TypeString, TypeBytes -> layout by state
//      - TypeModalState -> layout_modal.cpp
//      - TypeDynamic -> layout_dynobj.cpp
//      - TypeRange -> layout_ranges.cpp
//   3. LowerTypeForLayout resolves type aliases
//   4. Returns std::optional to handle unknown/error types
//
// DISPATCH TABLE:
//   SizeOf(T) -> specific layout function based on T variant
//   AlignOf(T) -> specific layout function based on T variant
//   LayoutOf(T) -> {size, align} pair
// =============================================================================

#include "cursive/05_codegen/layout/layout.h"

// TODO: Implement central layout dispatch functions
