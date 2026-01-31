/*
 * =============================================================================
 * MIGRATION MAPPING: prov_args.cpp
 * =============================================================================
 *
 * SPEC REFERENCE:
 *   - C0updated.md, Section 21.4.4 "Argument Provenance" (lines 25270-25350)
 *   - C0updated.md, Section 6.8 "Procedure Calls" (lines 16500-16700)
 *   - C0updated.md, Section 10.3 "Move Semantics" (lines 22310-22400)
 *
 * SOURCE FILE:
 *   - cursive-bootstrap/src/03_analysis/memory/regions.cpp (lines 1-2036)
 *     (provenance tracking integrated into region analysis)
 *
 * FUNCTIONS TO MIGRATE:
 *   - AnalyzeCallProvenance(Call* call) -> ProvenanceEffect
 *       Analyze provenance effects of procedure call
 *   - TrackArgumentProvenance(Arg* arg, Param* param) -> void
 *       Track provenance from argument to parameter
 *   - TrackMoveArgProvenance(Arg* arg) -> void
 *       Track provenance for move arguments
 *   - ValidateProvenanceLifetime(Arg* arg, Call* call) -> bool
 *       Ensure argument provenance outlives call
 *   - InferReturnProvenance(Call* call) -> Provenance
 *       Infer provenance of call result
 *   - CheckProvenanceEscape(Call* call) -> bool
 *       Check if call may cause provenance escape
 *
 * DEPENDENCIES:
 *   - Provenance tracking
 *   - Call/argument AST
 *   - Parameter signatures
 *
 * REFACTORING NOTES:
 *   1. Non-move args: Provenance borrowed for call duration
 *   2. Move args: Provenance transferred to callee
 *   3. Return value may have provenance from:
 *      - One of the arguments
 *      - Callee-allocated memory
 *      - Static/global
 *   4. Provenance must outlive call (no dangling pointers)
 *   5. Consider provenance annotations for polymorphic calls
 *
 * ARGUMENT PROVENANCE:
 *   procedure foo(x: T):
 *     - x borrows provenance from caller's argument
 *     - Caller retains provenance
 *
 *   procedure bar(move x: T):
 *     - x takes provenance from caller's argument
 *     - Caller loses provenance
 *
 * RETURN PROVENANCE:
 *   - May come from arguments (explicitly tracked)
 *   - May be new allocation (callee's region/heap)
 *   - May be static (global data)
 *
 * LIFETIME REQUIREMENTS:
 *   - Borrowed provenance must be valid for call duration
 *   - Returned provenance must outlive call site scope
 *   - Move provenance may have any lifetime
 *
 * DIAGNOSTIC CODES:
 *   - E-PROV-0030: Argument provenance expired
 *   - E-PROV-0031: Return provenance escapes
 *   - E-PROV-0032: Move of borrowed provenance
 *
 * =============================================================================
 */
