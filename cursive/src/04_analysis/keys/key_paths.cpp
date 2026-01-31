/*
 * =============================================================================
 * MIGRATION MAPPING: key_paths.cpp
 * =============================================================================
 *
 * SPEC REFERENCE:
 *   - C0updated.md, Section 17.8 "Key Paths" (lines 24410-24500)
 *   - C0updated.md, Section 17.9 "Path Construction" (lines 24510-24600)
 *   - C0updated.md, Section 17.10 "Key Boundaries" (lines 24610-24700)
 *
 * SOURCE FILE:
 *   - cursive-bootstrap/src/03_analysis/keys/key_context.cpp (lines 1-192)
 *     (path handling integrated into context)
 *
 * FUNCTIONS TO MIGRATE:
 *   - BuildKeyPath(Expr* expr) -> KeyPath
 *       Construct key path from expression
 *   - ParseKeyPathSpec(KeySpec* spec) -> KeyPath
 *       Parse key path from #block specification
 *   - NormalizePath(KeyPath path) -> KeyPath
 *       Normalize path for comparison
 *   - PathPrefix(KeyPath a, KeyPath b) -> bool
 *       Check if a is prefix of b
 *   - FindKeyBoundary(KeyPath path) -> Option<KeyPath>
 *       Find # boundary in path
 *   - TruncateAtBoundary(KeyPath path) -> KeyPath
 *       Truncate path at key boundary
 *
 * DEPENDENCIES:
 *   - Expression AST for path construction
 *   - Field declarations for # boundaries
 *   - Path representation
 *
 * REFACTORING NOTES:
 *   1. Key paths represent memory locations for synchronization
 *   2. Paths are built from field access chains
 *   3. Array indexing creates indexed paths
 *   4. # prefix on fields creates key boundaries
 *   5. Boundaries stop conflict propagation inward
 *   6. Paths must be normalized for comparison
 *
 * PATH CONSTRUCTION:
 *   - x         -> [x]
 *   - x.field   -> [x, .field]
 *   - x[i]      -> [x, [i]]
 *   - x.#bound  -> [x, #bound] (boundary)
 *
 * KEY BOUNDARIES:
 *   record Container {
 *       #data: shared DataStore,  // # creates boundary
 *       metadata: i32
 *   }
 *   - #container.data acquires key for data only
 *   - Nested paths beyond boundary need separate acquisition
 *
 * DIAGNOSTIC CODES:
 *   - E-KEY-0030: Invalid key path expression
 *   - E-KEY-0031: Path not rooted at binding
 *   - E-KEY-0032: Dynamic index in static key path
 *
 * =============================================================================
 */
