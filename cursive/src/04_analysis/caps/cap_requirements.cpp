/*
 * =============================================================================
 * MIGRATION MAPPING: cap_requirements.cpp
 * =============================================================================
 *
 * SPEC REFERENCE:
 *   - C0updated.md, Section 5.9.3 "Capability Requirements" (lines 13210-13300)
 *   - C0updated.md, Section 5.9.5 "Capability Classes" (lines 13400-13600)
 *   - C0updated.md, Section 8.10 "E-CAP Errors" (lines 21900-22000)
 *
 * SOURCE FILE:
 *   - cursive-bootstrap/src/03_analysis/caps/cap_filesystem.cpp (lines 1-497)
 *   - cursive-bootstrap/src/03_analysis/caps/cap_heap.cpp (lines 1-132)
 *   - cursive-bootstrap/src/03_analysis/caps/cap_concurrency.cpp (lines 1-717)
 *
 * FUNCTIONS TO MIGRATE:
 *   - InferCapabilityRequirements(Proc* proc) -> CapSet
 *       Infer which capabilities a procedure requires
 *   - ValidateCapabilitySatisfied(Call* call, CapSet required) -> bool
 *       Verify caller provides required capabilities
 *   - BuildCapabilitySignature(Proc* proc) -> CapSig
 *       Build capability requirements signature for procedure
 *   - CheckCapabilitySubset(CapSet provided, CapSet required) -> bool
 *       Verify provided capabilities satisfy requirements
 *   - PropagateCapabilityRequirements(CallGraph* cg) -> void
 *       Propagate requirements through call graph
 *
 * DEPENDENCIES:
 *   - Capability class definitions
 *   - Call graph infrastructure
 *   - Procedure signatures
 *
 * REFACTORING NOTES:
 *   1. Each capability type has specific operations it enables
 *   2. Requirements propagate transitively through calls
 *   3. Procedure signature should declare capability parameters
 *   4. Consider effect system for capability tracking
 *   5. Generic procedures may have parameterized capability requirements
 *
 * CAPABILITY OPERATIONS:
 *   $FileSystem:
 *     - read_file, write_file, exists, remove, create_dir, etc.
 *   $HeapAllocator:
 *     - string::from, bytes::with_capacity, etc.
 *   $ExecutionDomain:
 *     - parallel block execution
 *
 * DIAGNOSTIC CODES:
 *   - E-CAP-0010: Missing required capability
 *   - E-CAP-0011: Capability requirement not satisfiable
 *   - E-CAP-0012: Incompatible capability type
 *
 * =============================================================================
 */
