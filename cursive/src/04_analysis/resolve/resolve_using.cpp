// =============================================================================
// MIGRATION MAPPING: resolve_using.cpp
// =============================================================================
//
// SPEC REFERENCE:
//   C0updated.md §5.1.5 "Top-Level Name Collection" (Lines 6956-7309)
//   C0updated.md §5.1.4 "Visibility and Accessibility" (Lines 6900-6955)
//
// SOURCE FILE:
//   cursive-bootstrap/src/03_analysis/resolve/collect_toplevel.cpp (Lines 900-1064)
//
// DEPENDENCIES:
//   - cursive/include/04_analysis/resolve/resolve_using.h (header)
//   - cursive/src/04_analysis/resolve/scopes.h (ScopeContext, IdKeyOf)
//   - cursive/src/04_analysis/resolve/scopes_lookup.h (ResolveModulePath, ResolveQualified)
//   - cursive/src/04_analysis/resolve/visibility.h (CanAccess)
//
// =============================================================================
// CONTENT TO MIGRATE:
// =============================================================================
//
// 1. ResolveUsing (Source Lines 900-960)
//    - ResolveUsing(ctx, using_decl, name_maps) -> UsingResult
//    - Resolves using declaration to imported names
//    - Handles: single item, multiple items, glob (*)
//    - Implements (Resolve-Using)
//
// 2. ResolveUsingSingle (Source Lines 920-940)
//    - ResolveUsingSingle(ctx, path, name, alias) -> Entity
//    - Resolves single using declaration
//    - Applies alias if provided
//    - Checks accessibility of imported name
//    - Implements (Resolve-Using-Single)
//
// 3. ResolveUsingMultiple (Source Lines 940-960)
//    - ResolveUsingMultiple(ctx, path, names) -> NameMap
//    - Resolves using with multiple names: using mod::{a, b, c}
//    - Each name resolved independently
//    - Implements (Resolve-Using-Multiple)
//
// 4. ResolveUsingGlob (Source Lines 960-1000)
//    - ResolveUsingGlob(ctx, path) -> NameMap
//    - Resolves using mod::*
//    - Imports all public names from module
//    - Implements (Resolve-Using-Glob)
//
// 5. UsingNames (Source Lines 1000-1064)
//    - UsingNames(using_decl, resolved) -> NameMap
//    - Converts resolved entities to NameMap entries
//    - Handles aliasing and entity kind preservation
//
// =============================================================================
// SPEC DEFINITIONS TO IMPLEMENT:
// =============================================================================
//
// From §5.1.5:
//   (Resolve-Using-Single):
//     Γ ⊢ ResolveModulePath(path, Alias, ModuleNames) ⇓ mp ∧
//     Γ ⊢ ResolveQualified(mp, name, K, CanAccess) ⇓ ent ∧
//     alias_key = (alias ≠ ⊥ ? IdKey(alias) : IdKey(name))
//     → Γ ⊢ ResolveUsingSingle(path, name, alias) ⇓ (alias_key ↦ ent')
//
//   (Resolve-Using-Multiple):
//     ∀ (n, a) ∈ names. Γ ⊢ ResolveUsingSingle(path, n, a) ⇓ entry
//     → Γ ⊢ ResolveUsingMultiple(path, names) ⇓ ⋃ entries
//
//   (Resolve-Using-Glob):
//     Γ ⊢ ResolveModulePath(path, Alias, ModuleNames) ⇓ mp ∧
//     names = { n ↦ ent | (n, ent) ∈ NameMap(mp) ∧ IsPublic(ent) }
//     → Γ ⊢ ResolveUsingGlob(path) ⇓ names
//
// From §5.1.4:
//   Using declarations check accessibility of imported names.
//   Glob imports only public names.
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
//
// 1. Using declarations resolved during top-level collection
// 2. Using creates aliases with EntitySource::Using
// 3. Glob imports may introduce many names - check for conflicts
// 4. Alias allows renaming: using mod::foo as bar
// 5. Multiple import syntax: using mod::{a, b, c}
// 6. Using declarations don't create new entities, just aliases
// 7. Consider lazy resolution for glob imports
//
// =============================================================================

