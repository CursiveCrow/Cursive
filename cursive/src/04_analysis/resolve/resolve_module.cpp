// =============================================================================
// MIGRATION MAPPING: resolve_module.cpp
// =============================================================================
//
// SPEC REFERENCE:
//   C0updated.md §5.1.7 "Resolution Pass" (Lines 7430-7549)
//   C0updated.md §5.1.5 "Top-Level Name Collection" (Lines 6956-7309)
//
// SOURCE FILE:
//   cursive-bootstrap/src/03_analysis/resolve/resolver_modules.cpp (Lines 1-498)
//
// DEPENDENCIES:
//   - cursive/include/04_analysis/resolve/resolve_module.h (header)
//   - cursive/src/04_analysis/resolve/scopes.h (ScopeContext, Scopes)
//   - cursive/src/04_analysis/resolve/scopes_lookup.h (NameMapTable, ModuleNames)
//   - cursive/src/04_analysis/resolve/resolve_items.h (ResolveItem)
//   - cursive/src/04_analysis/resolve/visibility.h (CanAccess)
//   - cursive/src/00_core/project.h (Project, Module)
//
// =============================================================================
// CONTENT TO MIGRATE:
// =============================================================================
//
// 1. ResolveModules Entry Point (Source Lines 440-498)
//    - ResolveModules(project, diag) -> ResolveResult
//    - Collects top-level names for all modules
//    - Builds NameMapTable and ModuleNames
//    - Resolves each module in sequence
//    - Implements (Resolve-Pass) from §5.1.7
//
// 2. ResolveModule Function (Source Lines 380-438)
//    - ResolveModule(ctx, module, name_maps, module_names) -> void
//    - Creates module-level ScopeContext
//    - Calls ResolveItems for all items in module
//    - Implements (Module-Scope-Enter), (Module-Scope-Exit)
//
// 3. PopulateSigma Function (Source Lines 330-378)
//    - PopulateSigma(ctx, names) -> Scope
//    - Converts NameMap entries to Scope bindings
//    - Handles visibility filtering for current module
//    - Used to initialize module scope before resolution
//
// 4. CollectTopLevelNames Function (Source Lines 180-328)
//    - CollectTopLevelNames(project) -> (NameMapTable, errors)
//    - Implements (Top-Level-Names-Pass) from §5.1.5
//    - Collects names from all items in all modules
//    - Handles imports and using declarations
//    - Validates no reserved names at module level
//
// 5. BuildModuleScope Function (Source Lines 120-178)
//    - BuildModuleScope(universe, module_names, local_names) -> Scope
//    - Constructs the module-level scope
//    - Merges universe bindings with module-local names
//    - Implements (Scope-Module-Init)
//
// =============================================================================
// SPEC DEFINITIONS TO IMPLEMENT:
// =============================================================================
//
// From §5.1.7:
//   (Resolve-Pass):
//     ∀ m ∈ Modules(P). Γ_m = ⟨Scopes_m, m, NameMaps, ModuleNames⟩
//     → ∀ item ∈ Items(m). Γ_m ⊢ ResolveItem(item) ⇓ ok
//     → P ⊢ ResolvePass ⇓ ok
//
//   (Module-Scope-Enter):
//     Scopes(Γ) = [S_proc, S_module, S_universe]
//     → Γ ⊢ EnterModule(m) ⇓ Γ'
//
// From §5.1.5:
//   (Top-Level-Names-Pass):
//     ∀ m ∈ Modules(P). CollectNames(P, m) ⇓ NameMap(m)
//     → P ⊢ TopLevelNamesPass ⇓ NameMaps
//
//   NameMaps : PathKey → NameMap
//   ModuleNames = [StringOfPath(m.path) | m ∈ Modules(P)]
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
//
// 1. Consider parallel collection of top-level names (modules are independent)
// 2. ResolveResult should include collected diagnostics and resolved AST
// 3. ScopeContext construction should validate universe scope is present
// 4. Error accumulation strategy: continue on non-fatal errors
// 5. Module ordering: process in dependency order if imports create deps
// 6. NameMapTable should use PathKey for efficient lookup
//
// =============================================================================

