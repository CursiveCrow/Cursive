/*
 * =============================================================================
 * MIGRATION MAPPING: borrow_bind.cpp
 * =============================================================================
 *
 * SPEC REFERENCE:
 *   - C0updated.md, Section 10.1 "Binding Operators" (lines 22110-22200)
 *   - C0updated.md, Section 10.2 "Binding States" (lines 22210-22300)
 *   - C0updated.md, Section 10.3 "Move Semantics" (lines 22310-22400)
 *   - C0updated.md, Section 10.4 "Permission System" (lines 22410-22500)
 *   - C0updated.md, Section 8.9 "E-OWN Errors" (lines 21800-21900)
 *
 * SOURCE FILE:
 *   - cursive-bootstrap/src/03_analysis/memory/borrow_bind.cpp (LARGE FILE ~29000+ tokens)
 *     NOTE: File is very large, read with offset/limit
 *
 * FUNCTIONS TO MIGRATE:
 *   - TrackBindingState(Binding* bind) -> BindingState             [estimated lines 50-300]
 *       Track Alive/Moved/PartiallyMoved/Poisoned states
 *   - CheckMove(Expr* expr) -> bool                                [estimated lines 305-500]
 *       Validate move expression is legal
 *   - AnalyzeOwnership(Stmt* stmt) -> OwnershipInfo                [estimated lines 505-800]
 *       Analyze ownership flow through statement
 *   - ValidateUniqueAccess(Expr* expr) -> bool                     [estimated lines 805-1000]
 *       Ensure unique permission has no aliases
 *   - CheckPermissionCompatibility(Perm required, Perm actual) -> bool [estimated lines 1005-1200]
 *       Validate permission satisfies requirement
 *   - TrackPartialMove(Expr* field_access) -> void                 [estimated lines 1205-1400]
 *       Track partial moves of record fields
 *   - ValidateConsumingParam(Param* param) -> bool                 [estimated lines 1405-1600]
 *       Validate move parameter in procedure signature
 *   - AnalyzeBindingLifetime(Binding* bind) -> Lifetime            [estimated lines 1605-1800]
 *       Compute lifetime of a binding
 *   - CheckUseAfterMove(Expr* expr) -> bool                        [estimated lines 1805-2000]
 *       Detect use of moved binding
 *   - ValidateDropOrder(Block* block) -> Vec<Binding*>             [estimated lines 2005-2200]
 *       Determine deterministic drop order (reverse declaration)
 *
 * DEPENDENCIES:
 *   - Binding, BindingState enum
 *   - Permission enum (const, unique, shared)
 *   - Move semantics tracking
 *   - Drop trait detection
 *
 * REFACTORING NOTES:
 *   1. CURSIVE IS NOT RUST - no borrow checker, but ownership tracking
 *   2. Binding operators: let/let:=/var/var:= (movable vs immovable)
 *   3. States: Alive, Moved, PartiallyMoved, Poisoned
 *   4. move keyword transfers responsibility (not permission)
 *   5. Permissions: const (read), unique (exclusive write), shared (synchronized)
 *   6. Drop::drop called in reverse declaration order
 *   7. This is a VERY large source file - consider major refactoring:
 *      - ownership.cpp: Ownership tracking
 *      - permissions.cpp: Permission validation
 *      - move_check.cpp: Move validation
 *      - drop_order.cpp: Drop ordering
 *      - binding_state.cpp: State machine for bindings
 *
 * DIAGNOSTIC CODES:
 *   - E-OWN-0001: Use after move
 *   - E-OWN-0002: Move of immovable binding
 *   - E-OWN-0003: Unique permission violated (alias exists)
 *   - E-OWN-0004: Permission insufficient
 *   - E-OWN-0005: Partial move leaves binding unusable
 *   - E-OWN-0006: Drop of moved binding
 *
 * =============================================================================
 */
