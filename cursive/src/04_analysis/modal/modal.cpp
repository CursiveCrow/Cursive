/*
 * =============================================================================
 * MIGRATION MAPPING: modal.cpp
 * =============================================================================
 *
 * SPEC REFERENCE:
 *   - C0updated.md, Section 5.4 "Modal Types" (line 12261)
 *   - C0updated.md, Section 5.4.1 "Modal Declaration" (lines 12270-12400)
 *   - C0updated.md, Section 5.4.2 "Modal States" (lines 12410-12550)
 *   - C0updated.md, Section 8.7 "E-MOD Errors" (lines 21600-21700)
 *
 * SOURCE FILE:
 *   - cursive-bootstrap/src/03_analysis/modal/modal.cpp (lines 1-60)
 *
 * FUNCTIONS TO MIGRATE:
 *   - LookupModalDecl(Name name) -> ModalDecl*                    [lines 15-30]
 *       Find modal declaration by name in current scope
 *   - LookupModalState(ModalDecl* decl, Name state) -> ModalState* [lines 32-45]
 *       Find specific state within a modal declaration
 *   - HasState(ModalDecl* decl, Name state) -> bool               [lines 47-52]
 *       Check if modal has a given state
 *   - StateNameSet(ModalDecl* decl) -> Set<Name>                  [lines 54-60]
 *       Get all state names for a modal type
 *
 * DEPENDENCIES:
 *   - ModalDecl, ModalState from AST
 *   - Name resolution infrastructure
 *   - Scope lookup utilities
 *
 * REFACTORING NOTES:
 *   1. Modal types have states prefixed with @ (e.g., @Connected, @Disconnected)
 *   2. Each modal declaration has exactly one state active at any time
 *   3. States contain state-specific fields and methods
 *   4. Built-in modals: Region (@Active, @Frozen, @Freed), CancelToken, Spawned, etc.
 *   5. Consider caching state lookups for frequently accessed modals
 *   6. Validate state names are unique within a modal declaration
 *
 * DIAGNOSTIC CODES:
 *   - E-MOD-0001: Unknown modal type
 *   - E-MOD-0002: Unknown modal state
 *   - E-MOD-0003: Duplicate state name
 *
 * =============================================================================
 */
