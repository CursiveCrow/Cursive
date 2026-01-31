// =============================================================================
// MIGRATION MAPPING: layout_unions.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   - Section 6.1.4 Union Layout and Discriminants (lines 14632-14835)
//   - Section 6.1.4.1 Niche Optimization (Cursive0) (lines 14634-14827)
//   - NicheSet definitions (lines 14638-14641)
//   - Valid pointer non-zero invariant (lines 14647-14654)
//   - Type Ordering (Cursive0) (lines 14660-14719)
//   - MemberList, MemberIndex, UnionDiscValue (lines 14721-14723)
//   - PayloadMember, NicheApplies predicates (lines 14724-14728)
//   - ValueBits specifications (lines 14729-14770)
//   - Union Layout rules (lines 14773-14815)
//   - Layout-Union-Niche rule (lines 14783-14786)
//   - Layout-Union-Tagged rule (lines 14788-14791)
//   - Size-Union, Align-Union, Layout-Union rules (lines 14793-14806)
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/layout/layout_unions.cpp
//   - AlignUp helper (lines 14-23)
//   - DiscTypeName helper (lines 25-36)
//   - DiscTypeLayout helper (lines 38-49)
//   - IsUnitType helper (lines 51-59)
//   - NicheCount function (lines 61-96)
//   - UnionLayoutOf function (lines 100-191)
//
// DEPENDENCIES:
//   - cursive/include/05_codegen/layout/layout.h (UnionLayout struct)
//   - cursive/include/04_analysis/types/type_equiv.h (SortUnionMembers)
//   - cursive/include/04_analysis/types/types.h (TypeRef, TypePerm, TypePtr)
//   - LowerTypeForLayout for type alias resolution
//
// REFACTORING NOTES:
//   1. Union members are sorted using TypeKey ordering (see spec lines 14660-14719)
//   2. Niche optimization applies when:
//      - Exactly one non-unit member with niches (PayloadMember)
//      - All other members are unit type ()
//      - Niche count >= number of members - 1
//   3. NicheSet(Ptr<T>@Valid) = {null pointer bit pattern}
//   4. NicheSet for other types = empty
//   5. Tagged union layout: discriminant + payload
//   6. Niche union layout: payload only (empty members use niche values)
//   7. UnionDiscValue(U, T) = sorted index of T in U
//
// NICHE OPTIMIZATION EXAMPLE:
//   Ptr<i32>@Valid | () uses niche optimization:
//   - Ptr<i32>@Valid stored directly (must be non-null)
//   - () represented as null pointer (0x0)
//   - No discriminant needed
// =============================================================================

#include "cursive/05_codegen/layout/layout.h"

// TODO: Migrate implementation from cursive-bootstrap/src/04_codegen/layout/layout_unions.cpp
