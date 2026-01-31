// =============================================================================
// MIGRATION MAPPING: scopes_intro.cpp
// =============================================================================
//
// SPEC REFERENCE:
//   C0updated.md §5.1.2 "Name Introduction and Shadowing" (Lines 6718-6821)
//
// SOURCE FILE:
//   cursive-bootstrap/src/03_analysis/resolve/scopes_intro.cpp (Lines 1-183)
//
// DEPENDENCIES:
//   - cursive/include/04_analysis/resolve/scopes_intro.h (header)
//   - cursive/src/04_analysis/resolve/scopes.h (IdKeyOf, ReservedGen, etc.)
//
// =============================================================================
// CONTENT TO MIGRATE:
// =============================================================================
//
// 1. Scope Predicates (Source Lines 39-56)
//    - InScope(scope, name) -> bool: Check if name in current scope
//    - InOuter(ctx, name) -> bool: Check if name in any outer scope
//
// 2. Intro Function (Source Lines 58-91)
//    - Intro(ctx, name, entity) -> IntroResult
//    - Implements (Intro-Ok), (Intro-Dup), (Intro-Shadow-Required)
//    - Implements (Intro-Reserved-Gen-Err), (Intro-Reserved-Cursive-Err)
//    - Priority order: Gen > Cursive > Dup > Shadow-Required > Ok
//
// 3. ShadowIntro Function (Source Lines 93-127)
//    - ShadowIntro(ctx, name, entity) -> IntroResult
//    - Implements (Shadow-Ok), (Shadow-Unnecessary)
//    - Implements (Shadow-Reserved-Gen-Err), (Shadow-Reserved-Cursive-Err)
//    - Priority order: Gen > Cursive > InScope > Unnecessary > Ok
//
// 4. ValidateModuleNames Function (Source Lines 129-180)
//    - ValidateModuleNames(names) -> ValidateModuleNamesResult
//    - Implements (Validate-Module-Ok)
//    - Implements (Validate-Module-Keyword-Err)
//    - Implements (Validate-Module-Prim-Shadow-Err)
//    - Implements (Validate-Module-Special-Shadow-Err)
//    - Implements (Validate-Module-Async-Shadow-Err)
//
// =============================================================================
// SPEC DEFINITIONS TO IMPLEMENT:
// =============================================================================
//
// From §5.1.2:
//   dom(S) = keys(S)
//   Scopes(Γ) = [S_cur] ++ Γ_out
//   InScope(S, x) ⇔ IdKey(x) ∈ dom(S)
//   InOuter(Γ, x) ⇔ ∃ S ∈ Γ_out. InScope(S, x)
//
//   (Intro-Ok):
//     ¬ InScope(S_cur, x) ∧ ¬ InOuter(Γ, x) ∧ ¬ ReservedId(x) ∧
//     (S_cur ≠ S_module ∨ x ∉ UniverseProtected)
//     → Γ ⊢ Intro(x, ent) ⇓ Γ'
//
//   (Intro-Dup):
//     InScope(S_cur, x) → Γ ⊢ Intro(x, ent) ⇑
//
//   (Intro-Shadow-Required):
//     ¬ InScope(S_cur, x) ∧ InOuter(Γ, x)
//     → Γ ⊢ Intro(x, ent) ⇑ Code(Intro-Shadow-Required)
//
//   (Shadow-Ok):
//     ¬ InScope(S_cur, x) ∧ InOuter(Γ, x) ∧ ¬ ReservedId(x) ∧
//     (S_cur ≠ S_module ∨ x ∉ UniverseProtected)
//     → Γ ⊢ ShadowIntro(x, ent) ⇓ Γ'
//
//   (Shadow-Unnecessary):
//     ¬ InScope(S_cur, x) ∧ ¬ InOuter(Γ, x)
//     → Γ ⊢ ShadowIntro(x, ent) ⇑ Code(Shadow-Unnecessary)
//
//   Intro Priority: Gen-Err > Cursive-Err > Dup > Shadow-Required > Ok
//   ShadowIntro Priority: Gen-Err > Cursive-Err > InScope > Unnecessary > Ok
//
//   (Validate-Module-Ok):
//     ∀ n ∈ Names(N). ¬ KeywordKey(n) ∧ n ∉ PrimTypeKeys ∧
//     n ∉ SpecialTypeKeys ∧ n ∉ AsyncTypeKeys
//
// =============================================================================
// HELPER FUNCTIONS (Source Lines 20-35):
// =============================================================================
//
// - ContainsKey(vector<IdKey>, key) -> bool: Linear search helper
// - IsUniverseProtectedKey(key) -> bool: Check Prim|Special|Async keys
// - IsModuleScopeCurrent(scopes) -> bool: Check if current scope is module
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
//
// 1. IntroResult struct: { ok: bool, diag_id: optional<string_view> }
// 2. ValidateModuleNamesResult struct: same shape as IntroResult
// 3. Consider using std::unordered_set for faster key membership checks
// 4. Module scope detection relies on scope list structure [cur, mod, universe]
// 5. All diagnostic IDs should match spec rule names exactly
//
// =============================================================================

