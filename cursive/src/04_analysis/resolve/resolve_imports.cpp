// =============================================================================
// MIGRATION MAPPING: resolve_imports.cpp
// =============================================================================
//
// SPEC REFERENCE:
//   C0updated.md §5.1.5 "Top-Level Name Collection" (Lines 6956-7309)
//   C0updated.md §5.1.4 "Visibility and Accessibility" (Lines 6900-6955)
//
// SOURCE FILE:
//   cursive-bootstrap/src/03_analysis/resolve/imports.cpp (Lines 1-257)
//
// DEPENDENCIES:
//   - cursive/include/04_analysis/resolve/resolve_imports.h (header)
//   - cursive/src/04_analysis/resolve/scopes.h (ScopeContext, IdKeyOf)
//   - cursive/src/04_analysis/resolve/scopes_lookup.h (ResolveModulePath)
//   - cursive/src/04_analysis/resolve/visibility.h (CanAccess)
//   - cursive/src/00_core/symbols.h (PathKeyOf)
//
// =============================================================================
// CONTENT TO MIGRATE:
// =============================================================================
//
// 1. ImportResolver Class (Source Lines 20-80)
//    - ImportResolver: stateful resolver for import declarations
//    - Tracks current module and name maps
//    - Accumulates resolved imports and errors
//
// 2. ResolveImport (Source Lines 80-160)
//    - ResolveImport(ctx, import_decl) -> ImportResult
//    - Resolves module path in import declaration
//    - Creates ModuleAlias entity for the import
//    - Validates module exists
//    - Implements (Resolve-Import)
//
// 3. ValidateImport (Source Lines 160-200)
//    - ValidateImport(ctx, import) -> bool
//    - Checks import doesn't shadow existing names
//    - Checks imported module is accessible
//    - Implements (Validate-Import)
//
// 4. ResolveImportAlias (Source Lines 200-230)
//    - ResolveImportAlias(import) -> IdKey
//    - Determines alias name for import
//    - Uses explicit alias if provided
//    - Otherwise uses last segment of module path
//    - Implements (Import-Alias-Name)
//
// 5. CollectImportNames (Source Lines 230-257)
//    - CollectImportNames(imports) -> NameMap
//    - Aggregates all import bindings into NameMap
//    - Used during top-level name collection
//
// =============================================================================
// SPEC DEFINITIONS TO IMPLEMENT:
// =============================================================================
//
// From §5.1.5:
//   (Resolve-Import):
//     Γ ⊢ ResolveModulePath(path, {}, ModuleNames) ⇓ mp ∧
//     alias = ImportAliasName(import)
//     → Γ ⊢ ResolveImport(import) ⇓ (alias ↦ ⟨ModuleAlias, mp, ⊥, Decl⟩)
//
//   (Import-Alias-Name):
//     import.alias ≠ ⊥ → ImportAliasName(import) = import.alias
//     import.alias = ⊥ → ImportAliasName(import) = path[|path|-1]
//
//   (Validate-Import):
//     ¬InScope(S_module, alias) ∧
//     Γ ⊢ CanAccess(current_module, mp) ⇓ ok
//     → Γ ⊢ ValidateImport(import) ⇓ ok
//
// From §5.1.4:
//   Module accessibility checked during import validation.
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
//
// 1. Import resolution happens during top-level collection
// 2. Imports create ModuleAlias entities
// 3. Alias shadows the original module path segments
// 4. Import validation is separate from resolution
// 5. Cyclic import detection handled elsewhere
// 6. Consider ImportResult struct for success/error
// 7. Error messages should include both alias and path
//
// =============================================================================

