// =============================================================================
// MIGRATION MAPPING: stmt/region_stmt.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.5: Memory and Pointers - Region statements
//   - region { body } creates arena scope
//   - region (options) { body } with configuration
//   - region as r { body } with named region binding
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/memory/regions.cpp
//   Region statement typing
//
// KEY CONTENT TO MIGRATE:
//   BASIC REGION (region { body }):
//   1. Push region scope
//   2. Type body statements
//   3. Pop region scope
//   4. All allocations in region freed at exit
//
//   REGION WITH OPTIONS (region (options) { body }):
//   1. Type options expression (must be RegionOptions)
//   2. Same as basic region
//   3. Options affect allocation behavior
//
//   NAMED REGION (region as r { body }):
//   1. Create region binding r: Region@Active
//   2. r available in body for explicit allocation
//   3. Can use r ^ value syntax for allocation
//   4. Region freed when scope exits
//
//   ALLOCATION WITHIN REGION:
//   - ^value allocates in current region
//   - r ^ value allocates in named region r
//   - Ptr<T>@Valid produced for allocation
//   - Ptr becomes @Expired when region exits
//
// DEPENDENCIES:
//   - ExprTypeFn for options typing
//   - Block typing for body
//   - Region modal type (Region@Active, Region@Frozen, Region@Freed)
//   - RegionOptions record type
//   - PushRegionScope/PopRegionScope
//
// SPEC RULES:
//   - T-Region: Basic region
//   - T-Region-Options: With configuration
//   - T-Region-Named: With binding
//
// RESULT TYPE:
//   StmtResult { Gamma (unchanged), Res (from body), Brk (from body), BrkVoid }
//
// NOTES:
//   - Pointers allocated in region must not escape
//   - Region lifetime tracked for pointer validity
//
// =============================================================================

