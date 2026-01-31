/*
 * =============================================================================
 * MIGRATION MAPPING: modal_transitions.cpp
 * =============================================================================
 *
 * SPEC REFERENCE:
 *   - C0updated.md, Section 5.4.5 "Transitions" (lines 12760-12810)
 *   - C0updated.md, Section 5.4.6 "State Methods" (lines 12820-12900)
 *   - C0updated.md, Section 8.7 "E-MOD Errors" (lines 21600-21700)
 *
 * SOURCE FILE:
 *   - cursive-bootstrap/src/03_analysis/modal/modal_transitions.cpp (lines 1-80)
 *
 * FUNCTIONS TO MIGRATE:
 *   - LookupStateMethodDecl(ModalState* state, Name method) -> ProcDecl* [lines 15-35]
 *       Find method declaration within a specific modal state
 *   - LookupTransitionDecl(ModalState* state, Name trans) -> TransitionDecl* [lines 37-55]
 *       Find transition declaration (state -> state)
 *   - StateMemberVisible(ModalState* state, Decl* member) -> bool       [lines 57-70]
 *       Check if method/transition is accessible from current state
 *   - GetTransitionTarget(TransitionDecl* trans) -> ModalState*         [lines 72-80]
 *       Get the target state of a transition
 *
 * DEPENDENCIES:
 *   - ModalState, TransitionDecl, ProcDecl from AST
 *   - Method resolution infrastructure
 *   - State tracking context
 *
 * REFACTORING NOTES:
 *   1. Transitions use `transition` keyword and return a different state
 *   2. Transition return type must be @StateName (specific state)
 *   3. Regular methods in a state can only be called when in that state
 *   4. Transitions consume self (take ownership of the modal)
 *   5. Transition bodies must construct and return the target state
 *   6. Consider validating transition graph for reachability
 *
 * DIAGNOSTIC CODES:
 *   - E-MOD-0020: Method not accessible in current state
 *   - E-MOD-0021: Transition target state invalid
 *   - E-MOD-0022: Transition does not return state type
 *   - E-MOD-0023: Unreachable state in modal
 *
 * =============================================================================
 */
