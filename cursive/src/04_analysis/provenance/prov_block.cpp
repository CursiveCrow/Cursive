/*
 * =============================================================================
 * MIGRATION MAPPING: prov_block.cpp
 * =============================================================================
 *
 * SPEC REFERENCE:
 *   - C0updated.md, Section 21.4.5 "Block Provenance" (lines 25360-25450)
 *   - C0updated.md, Section 21.2 "Region Statements" (lines 24800-24900)
 *   - C0updated.md, Section 21.3 "Frame Statements" (lines 24910-25000)
 *
 * SOURCE FILE:
 *   - cursive-bootstrap/src/03_analysis/memory/regions.cpp (lines 1-2036)
 *     (provenance tracking integrated into region analysis)
 *
 * FUNCTIONS TO MIGRATE:
 *   - AnalyzeBlockProvenance(Block* block) -> ProvenanceState
 *       Analyze provenance through block execution
 *   - EnterRegionProvenance(Region* region) -> RegionProv
 *       Set up provenance state for region block
 *   - ExitRegionProvenance(Region* region) -> void
 *       Invalidate region provenance on exit
 *   - EnterFrameProvenance(Frame* frame) -> FrameProv
 *       Set up provenance state for frame block
 *   - ExitFrameProvenance(Frame* frame) -> void
 *       Invalidate frame provenance on exit
 *   - CheckEscapeOnExit(Block* block) -> Vec<EscapedPtr>
 *       Check for provenance escaping block
 *   - MergeControlFlow(Branch* branch) -> ProvenanceState
 *       Merge provenance from if/match branches
 *
 * DEPENDENCIES:
 *   - Region, Frame AST nodes
 *   - Provenance state tracking
 *   - Control flow analysis
 *
 * REFACTORING NOTES:
 *   1. Region block creates new provenance domain
 *   2. All ^alloc within region share region's provenance
 *   3. Region exit invalidates all region provenance
 *   4. Frame subdivides region, partial invalidation
 *   5. If/match branches require provenance merge
 *   6. Consider loop invariant provenance
 *
 * REGION PROVENANCE:
 *   region {
 *       let p: Ptr<i32>@Valid = ^42  // p has region provenance
 *   }
 *   // p now has @Expired provenance (invalid)
 *
 * FRAME PROVENANCE:
 *   region {
 *       let outer: Ptr<i32>@Valid = ^1
 *       frame {
 *           let inner: Ptr<i32>@Valid = ^2
 *       }
 *       // inner invalid, outer still valid
 *   }
 *
 * ESCAPE CHECKING:
 *   - Pointers with region provenance cannot escape region
 *   - Must not assign region ptr to outer scope binding
 *   - Must not return region ptr
 *   - Must not store region ptr in heap
 *
 * CONTROL FLOW MERGE:
 *   if cond {
 *       let p = ^x  // p1 provenance
 *   } else {
 *       let p = ^y  // p2 provenance
 *   }
 *   // p has merged provenance (both p1 and p2)
 *
 * DIAGNOSTIC CODES:
 *   - E-PROV-0040: Provenance escapes region
 *   - E-PROV-0041: Provenance escapes frame
 *   - E-PROV-0042: Incompatible provenance merge
 *   - E-PROV-0043: Return of region provenance
 *
 * =============================================================================
 */
