/*
 * =============================================================================
 * MIGRATION MAPPING: callgraph_caps.cpp
 * =============================================================================
 *
 * SPEC REFERENCE:
 *   - C0updated.md, Section 19.1 "Capability Flow Analysis" (lines 24210-24300)
 *   - C0updated.md, Section 19.2 "Call Graph Construction" (lines 24310-24400)
 *
 * SOURCE FILE:
 *   - cursive-bootstrap/src/03_analysis/caps/cap_concurrency.cpp (lines 1-717)
 *     (contains call graph analysis for parallel execution)
 *
 * FUNCTIONS TO MIGRATE:
 *   - BuildCallGraph(Module* mod) -> CallGraph
 *       Construct call graph for module
 *   - AnnotateCapabilityFlow(CallGraph* cg) -> void
 *       Annotate edges with capability flow information
 *   - DetectCapabilityLeaks(CallGraph* cg) -> Vec<Leak>
 *       Find capabilities escaping to unauthorized code
 *   - ValidateCapabilityChain(CallGraph* cg, Proc* entry) -> bool
 *       Verify capability flow from entry point
 *   - ComputeTransitiveClosure(CallGraph* cg) -> ReachabilityMatrix
 *       Compute transitive call relationships
 *
 * DEPENDENCIES:
 *   - Call graph data structure
 *   - Capability annotations
 *   - Module/procedure declarations
 *
 * REFACTORING NOTES:
 *   1. Call graph enables whole-program capability analysis
 *   2. Capabilities must flow through explicit parameters
 *   3. Dynamic dispatch complicates capability tracking
 *   4. Extern procedures are capability "sinks" (cannot receive)
 *   5. Consider separate graphs for each capability type
 *   6. Recursive procedures need fixed-point analysis
 *
 * CALL GRAPH ANNOTATIONS:
 *   - Edge: caller -> callee with capability set
 *   - Node: procedure with required/provided capabilities
 *   - Sink: extern procedure (no capabilities allowed)
 *
 * DIAGNOSTIC CODES:
 *   - E-CAP-0030: Capability leak detected
 *   - E-CAP-0031: Unreachable capability requirement
 *   - E-CAP-0032: Capability cycle detected
 *
 * =============================================================================
 */
