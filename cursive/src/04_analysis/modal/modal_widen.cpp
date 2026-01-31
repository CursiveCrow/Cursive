/*
 * =============================================================================
 * MIGRATION MAPPING: modal_widen.cpp
 * =============================================================================
 *
 * SPEC REFERENCE:
 *   - C0updated.md, Section 5.7 "Modal Widening" (line 12814)
 *   - C0updated.md, Section 5.7.1 "widen Expression" (lines 12820-12900)
 *   - C0updated.md, Section 7.2 "Modal Layout" (line 18619)
 *   - C0updated.md, Section 7.3 "Niche Optimization" (line 18635)
 *   - C0updated.md, Section 8.7 "E-MOD Errors" (lines 21600-21700)
 *
 * SOURCE FILE:
 *   - cursive-bootstrap/src/03_analysis/modal/modal_widen.cpp (lines 1-413)
 *
 * FUNCTIONS TO MIGRATE:
 *   - PayloadState(ModalDecl* decl) -> ModalState*                [lines 25-60]
 *       Find state with payload fields (for niche optimization)
 *   - NicheApplies(ModalDecl* decl) -> bool                       [lines 65-120]
 *       Check if niche optimization can be applied to modal
 *   - NicheCompatible(TypeRef ty) -> bool                         [lines 125-180]
 *       Check if type has a niche (unused bit patterns)
 *   - WidenWarnCond(TypeRef specific, TypeRef widened) -> Warning* [lines 185-250]
 *       Generate warning conditions for implicit widening
 *   - CanWiden(TypeRef state_specific) -> bool                    [lines 255-300]
 *       Check if state-specific type can be widened
 *   - WidenType(TypeRef state_specific) -> TypeRef                [lines 305-350]
 *       Perform modal widening transformation
 *   - NicheValue(TypeRef ty) -> Value                             [lines 355-400]
 *       Get the niche value for a type (for discriminant-free enums)
 *   - ComputeModalLayout(ModalDecl* decl) -> Layout               [lines 405-413]
 *       Compute memory layout for modal type
 *
 * DEPENDENCIES:
 *   - ModalDecl, ModalState from AST
 *   - TypeRef for type manipulation
 *   - Layout computation infrastructure
 *   - Niche detection for Ptr, bool, char types
 *
 * REFACTORING NOTES:
 *   1. `widen` converts Modal@State -> Modal (erases state information)
 *   2. After widening, state-specific fields/methods are inaccessible
 *   3. Niche optimization: Ptr<T>@Valid uses null as @Null discriminant
 *   4. Bool niche: values other than 0/1 can represent other states
 *   5. Char niche: invalid UTF-8 sequences can be used
 *   6. Consider warning when widening loses static state information
 *   7. Layout must account for discriminant tag when no niche available
 *
 * DIAGNOSTIC CODES:
 *   - E-MOD-0030: Cannot widen non-modal type
 *   - E-MOD-0031: Already widened
 *   - W-MOD-0001: State information lost by widening
 *   - W-MOD-0002: Niche optimization not applied
 *
 * =============================================================================
 */
