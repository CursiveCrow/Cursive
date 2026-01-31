/*
 * =============================================================================
 * MIGRATION MAPPING: modal_fields.cpp
 * =============================================================================
 *
 * SPEC REFERENCE:
 *   - C0updated.md, Section 5.4.3 "State-Specific Fields" (lines 12560-12650)
 *   - C0updated.md, Section 5.4.4 "Field Visibility" (lines 12660-12750)
 *   - C0updated.md, Section 8.7 "E-MOD Errors" (lines 21600-21700)
 *
 * SOURCE FILE:
 *   - cursive-bootstrap/src/03_analysis/modal/modal_fields.cpp (lines 1-43)
 *
 * FUNCTIONS TO MIGRATE:
 *   - LookupModalFieldDecl(ModalState* state, Name field) -> FieldDecl* [lines 12-28]
 *       Find field declaration within a specific modal state
 *   - ModalFieldVisible(ModalState* state, FieldDecl* field) -> bool    [lines 30-43]
 *       Check if field is accessible from current state context
 *
 * DEPENDENCIES:
 *   - ModalState, FieldDecl from AST
 *   - Visibility checking utilities
 *   - Current state context tracking
 *
 * REFACTORING NOTES:
 *   1. Fields declared in a state block are ONLY accessible when in that state
 *   2. Common fields (outside state blocks) are accessible in all states
 *   3. Field access on widened modal type is ill-formed for state-specific fields
 *   4. Pattern matching on modal state unlocks access to state fields
 *   5. Consider tracking field provenance (which state defines it)
 *
 * DIAGNOSTIC CODES:
 *   - E-MOD-0010: Field not accessible in current state
 *   - E-MOD-0011: Field access on widened modal type
 *   - E-MOD-0012: Unknown field in modal state
 *
 * =============================================================================
 */
