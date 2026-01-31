/*
 * =============================================================================
 * MIGRATION MAPPING: authority_model.cpp
 * =============================================================================
 *
 * SPEC REFERENCE:
 *   - C0updated.md, Section 5.9 "Capabilities" (line 13048)
 *   - C0updated.md, Section 5.9.1 "Authority Model" (lines 13060-13150)
 *   - C0updated.md, Section 5.9.2 "No Ambient Authority" (lines 13160-13200)
 *   - C0updated.md, Section 19 "Capability Safety" (lines 24200-24400)
 *
 * SOURCE FILE:
 *   - cursive-bootstrap/src/03_analysis/caps/cap_system.cpp (lines 1-178)
 *   - cursive-bootstrap/src/03_analysis/caps/cap_filesystem.cpp (lines 1-497)
 *   - cursive-bootstrap/src/03_analysis/caps/cap_heap.cpp (lines 1-132)
 *   - cursive-bootstrap/src/03_analysis/caps/cap_concurrency.cpp (lines 1-717)
 *
 * FUNCTIONS TO MIGRATE:
 *   - ValidateCapabilityAccess(Expr* expr) -> bool
 *       Ensure capability is accessed through valid path
 *   - CheckAmbientAuthority(Proc* proc) -> bool
 *       Verify procedure does not use ambient authority
 *   - TraceCapabilityFlow(Expr* expr) -> CapPath
 *       Track capability from Context to usage point
 *   - ValidateCapabilityPassing(Call* call) -> bool
 *       Ensure capabilities are passed explicitly
 *   - CheckCapabilityIsolation(ExternProc* proc) -> bool
 *       Verify extern procedures don't receive capabilities
 *
 * DEPENDENCIES:
 *   - Context record type
 *   - Capability class types ($FileSystem, $HeapAllocator, etc.)
 *   - Call graph for flow analysis
 *
 * REFACTORING NOTES:
 *   1. NO AMBIENT AUTHORITY: All effects require explicit capability parameters
 *   2. Capabilities flow from Context through explicit parameters only
 *   3. Extern procedures CANNOT receive capability types (E-CAP-0001)
 *   4. Dynamic class objects ($ClassName) are capability markers
 *   5. Consider taint analysis for capability tracking
 *   6. Main receives Context, which provides all root capabilities
 *
 * CAPABILITY TYPES:
 *   - $FileSystem: File system operations
 *   - $HeapAllocator: Heap memory allocation
 *   - $ExecutionDomain: Parallel execution domain
 *   - Context: Record containing all capabilities
 *
 * DIAGNOSTIC CODES:
 *   - E-CAP-0001: Capability passed to extern procedure
 *   - E-CAP-0002: Ambient authority detected
 *   - E-CAP-0003: Capability not from Context
 *   - E-CAP-0004: Missing capability parameter
 *
 * =============================================================================
 */
