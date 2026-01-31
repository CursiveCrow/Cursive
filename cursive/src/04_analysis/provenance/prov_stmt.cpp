/*
 * =============================================================================
 * MIGRATION MAPPING: prov_stmt.cpp
 * =============================================================================
 *
 * SPEC REFERENCE:
 *   - C0updated.md, Section 21.4.3 "Statement Provenance" (lines 25190-25260)
 *   - C0updated.md, Section 10.5 "Memory Provenance" (lines 22510-22600)
 *   - C0updated.md, Section 6.6 "Statement Execution" (lines 16410-16500)
 *
 * SOURCE FILE:
 *   - cursive-bootstrap/src/03_analysis/memory/regions.cpp (lines 1-2036)
 *     (provenance tracking integrated into region analysis)
 *
 * FUNCTIONS TO MIGRATE:
 *   - AnalyzeStmtProvenance(Stmt* stmt) -> ProvenanceState
 *       Analyze provenance effects of statement
 *   - TrackBindingProvenance(Let* let) -> void
 *       Track provenance for new binding
 *   - TrackAssignmentProvenance(Assign* assign) -> void
 *       Track provenance through assignment
 *   - UpdateProvenanceOnMove(Move* move) -> void
 *       Update provenance when binding is moved
 *   - InvalidateProvenance(Binding* bind) -> void
 *       Invalidate provenance when binding goes out of scope
 *   - TrackReturnProvenance(Return* ret) -> void
 *       Track provenance of returned values
 *
 * DEPENDENCIES:
 *   - Provenance state tracking
 *   - Statement AST
 *   - Binding state machine
 *
 * REFACTORING NOTES:
 *   1. Bindings acquire provenance at initialization
 *   2. Assignment updates target's provenance
 *   3. Move transfers provenance to new binding
 *   4. Scope exit invalidates local provenance
 *   5. Return may propagate provenance to caller
 *   6. Consider defer statements for cleanup
 *
 * STATEMENT EFFECTS:
 *   let x = expr:
 *     - x acquires provenance from expr
 *   var x = expr:
 *     - x acquires provenance from expr (mutable)
 *   place = expr:
 *     - place's provenance updated from expr
 *   move x:
 *     - x's provenance transferred, x invalidated
 *   return expr:
 *     - expr's provenance propagated to caller
 *   defer { ... }:
 *     - Cleanup code inherits current provenance state
 *
 * PROVENANCE INVALIDATION:
 *   - Binding goes out of scope
 *   - Binding is moved from
 *   - Region/frame exits (all region pointers)
 *
 * DIAGNOSTIC CODES:
 *   - E-PROV-0020: Provenance invalidated before use
 *   - E-PROV-0021: Return of local provenance
 *   - E-PROV-0022: Move of expired provenance
 *
 * =============================================================================
 */
