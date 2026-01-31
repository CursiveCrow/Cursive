/*
 * =============================================================================
 * MIGRATION MAPPING: region_type.cpp
 * =============================================================================
 *
 * SPEC REFERENCE:
 *   - C0updated.md, Section 5.4.7 "Built-in Modals - Region" (lines 12910-13000)
 *   - C0updated.md, Section 21.1 "Region Model" (line 24725)
 *
 * SOURCE FILE:
 *   - cursive-bootstrap/src/03_analysis/memory/region_type.cpp (lines 1-53)
 *
 * FUNCTIONS TO MIGRATE:
 *   - BuildRegionModalDecl() -> ModalDecl*                         [lines 10-53]
 *       Construct the built-in Region modal type declaration
 *
 * DEPENDENCIES:
 *   - ModalDecl, ModalState from AST
 *   - TypeRef for field types
 *   - RegionOptions record type
 *
 * REFACTORING NOTES:
 *   1. Region is a built-in modal with states: @Active, @Frozen, @Freed
 *   2. @Active state: alloc() method, reset_unchecked(), freeze()
 *   3. @Frozen state: read-only access, thaw()
 *   4. @Freed state: no operations allowed
 *   5. new_scoped() creates region with automatic cleanup
 *   6. Region accepts RegionOptions for size hints
 *   7. Consider making Region modal definition data-driven
 *
 * MODAL STATES:
 *   @Active:
 *     - alloc<T>(value: T) -> Ptr<T>@Valid
 *     - reset_unchecked() -> @Active (unsafe)
 *     - freeze() -> @Frozen
 *     - free_unchecked() -> @Freed (unsafe)
 *   @Frozen:
 *     - thaw() -> @Active
 *     - free_unchecked() -> @Freed (unsafe)
 *   @Freed:
 *     (no methods)
 *
 * DIAGNOSTIC CODES:
 *   - E-REG-0010: Region operation in wrong state
 *   - E-REG-0011: Unsafe region operation outside unsafe block
 *
 * =============================================================================
 */
