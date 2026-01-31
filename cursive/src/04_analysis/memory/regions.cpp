/*
 * =============================================================================
 * MIGRATION MAPPING: regions.cpp
 * =============================================================================
 *
 * SPEC REFERENCE:
 *   - C0updated.md, Section 21.1 "Region Model" (line 24725)
 *   - C0updated.md, Section 21.2 "Region Statements" (lines 24800-24900)
 *   - C0updated.md, Section 21.3 "Frame Statements" (lines 24910-25000)
 *   - C0updated.md, Section 21.4 "Provenance Tracking" (lines 25010-25200)
 *   - C0updated.md, Section 8.8 "E-REG Errors" (lines 21700-21800)
 *
 * SOURCE FILE:
 *   - cursive-bootstrap/src/03_analysis/memory/regions.cpp (lines 1-2036)
 *
 * FUNCTIONS TO MIGRATE:
 *   - TrackProvenance(Expr* expr) -> Provenance                    [lines 50-250]
 *       Track memory provenance for an expression
 *   - CheckRegionEscape(Ptr* ptr, Region* region) -> bool          [lines 255-400]
 *       Verify pointer does not escape its region
 *   - AnalyzeRegionBlock(Block* block) -> RegionInfo               [lines 405-600]
 *       Analyze region block for safety
 *   - AnalyzeFrameBlock(Block* block) -> FrameInfo                 [lines 605-800]
 *       Analyze frame subdivision within region
 *   - ValidateAllocation(Expr* alloc, Region* region) -> bool      [lines 805-1000]
 *       Validate ^ allocation is within valid region
 *   - TrackRegionLifetime(Region* region) -> Lifetime              [lines 1005-1200]
 *       Track region lifetime for escape analysis
 *   - CheckDanglingPtr(Ptr* ptr) -> bool                           [lines 1205-1400]
 *       Check for use of expired pointer
 *   - PropagateProvenance(Expr* from, Expr* to) -> void            [lines 1405-1600]
 *       Propagate provenance through assignments
 *   - AnalyzeDefer(Defer* defer, Region* region) -> void           [lines 1605-1800]
 *       Analyze defer statements for region cleanup
 *   - ComputeRegionNesting(Block* block) -> NestingInfo            [lines 1805-2000]
 *       Compute nested region/frame structure
 *   - ValidateNamedRegion(Ident name, Expr* alloc) -> bool         [lines 2005-2036]
 *       Validate allocation targets named region
 *
 * DEPENDENCIES:
 *   - Region modal type (@Active, @Frozen, @Freed)
 *   - Provenance tracking infrastructure
 *   - Lifetime analysis
 *   - Pointer types (Ptr<T>@Valid, @Null, @Expired)
 *
 * REFACTORING NOTES:
 *   1. Region creates a memory arena with deterministic cleanup
 *   2. Frame subdivides a region for partial deallocation
 *   3. Provenance tracks which region owns each pointer
 *   4. Pointers become @Expired when their region exits
 *   5. Named regions allow explicit allocation targeting (r^value)
 *   6. ^ allocation is ONLY valid inside region blocks
 *   7. This is a large file - consider splitting:
 *      - provenance.cpp: Provenance tracking
 *      - lifetime.cpp: Lifetime analysis
 *      - escape.cpp: Escape analysis
 *      - region_validate.cpp: Region block validation
 *
 * DIAGNOSTIC CODES:
 *   - E-REG-0001: Allocation outside region
 *   - E-REG-0002: Pointer escapes region
 *   - E-REG-0003: Use of expired pointer
 *   - E-REG-0004: Invalid region nesting
 *   - E-REG-0005: Named region not found
 *   - W-REG-0001: Pointer may escape region
 *
 * =============================================================================
 */
