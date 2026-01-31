// =============================================================================
// MIGRATION MAPPING: expr/move_expr.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.6: Ownership and Move Semantics
//   - move place transfers ownership
//   - Source becomes invalid after move
//   - Result is the moved value
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/memory/ownership.cpp
//   Move expression typing
//
// KEY CONTENT TO MIGRATE:
//   MOVE EXPRESSION (move place):
//   1. Type the place expression
//   2. Place must be a movable binding
//   3. Place must have Alive state
//   4. Result type is the place's type
//   5. Side effect: Place state becomes Moved
//
//   MOVABILITY RULES:
//   - let x = v: movable (can use move x)
//   - let x := v: immovable (cannot move)
//   - var x = v: movable
//   - var x := v: immovable
//
//   BINDING STATE:
//   - Alive: valid, can be used
//   - Moved: ownership transferred, cannot use
//   - PartiallyMoved: some fields moved
//   - Poisoned: error state
//
//   MOVE PLACES:
//   - Local variable: entire binding moved
//   - Field access: field moved (partial move)
//   - Tuple element: element moved (partial move)
//   - Array element: element moved (partial move)
//
// DEPENDENCIES:
//   - PlaceTypeFn for place typing
//   - BindingState tracking
//   - IsMoveableBinding() predicate
//   - UpdateBindingState() for state tracking
//
// SPEC RULES:
//   - T-Move: Move expression typing
//   - Move-Immovable-Err: Trying to move immovable binding
//   - Move-AlreadyMoved-Err: Double move
//   - Move-NotPlace-Err: Move of non-place
//
// RESULT TYPE:
//   ExprTypeResult { bool ok; optional<string_view> diag_id; TypeRef type; }
//
// STATE EFFECTS:
//   This expression has side effects on binding state tracking.
//   The liveness analysis must be updated after move.
//
// =============================================================================

