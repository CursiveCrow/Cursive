// =============================================================================
// MIGRATION MAPPING: resolve_path.cpp
// =============================================================================
//
// SPEC REFERENCE:
//   C0updated.md §5.1.3 "Lookup" (Lines 6822-6899)
//   C0updated.md §5.1.6 "Qualified Disambiguation" (Lines 7310-7429)
//
// SOURCE FILE:
//   cursive-bootstrap/src/03_analysis/resolve/scopes_lookup.cpp (Lines 187-237)
//   cursive-bootstrap/src/03_analysis/resolve/resolve_qual.cpp (Lines 1-100)
//
// DEPENDENCIES:
//   - cursive/include/04_analysis/resolve/resolve_path.h (header)
//   - cursive/src/04_analysis/resolve/scopes.h (ScopeContext)
//   - cursive/src/04_analysis/resolve/scopes_lookup.h (ResolveModulePath, AliasMap)
//   - cursive/src/00_core/symbols.h (StringOfPath, PathKeyOf)
//
// =============================================================================
// CONTENT TO MIGRATE:
// =============================================================================
//
// 1. ResolveModulePath (Source scopes_lookup.cpp Lines 187-201)
//    - ResolveModulePath(ctx, path, alias, module_names) -> ResolveModulePathResult
//    - Expands first segment via alias map
//    - Validates expanded path is in module_names
//    - Returns expanded path or error
//    - Implements (Resolve-ModulePath), (ResolveModulePath-Err)
//
// 2. AliasExpand (Source scopes_lookup.cpp Lines 28-44)
//    - AliasExpand(path, alias) -> ModulePath
//    - If first segment is in alias map, replace with target
//    - Otherwise return path unchanged
//    - Implements (AliasExpand-Yes), (AliasExpand-None)
//
// 3. ResolveItemPath (Source resolve_qual.cpp Lines 40-80)
//    - ResolveItemPath(ctx, path) -> optional<Entity>
//    - Determines if path is unqualified or qualified
//    - Routes to appropriate lookup function
//    - Implements (Resolve-Item-Path)
//
// 4. PathIsQualified (Source resolve_qual.cpp Lines 20-38)
//    - PathIsQualified(path) -> bool
//    - Returns true if path has more than one segment
//    - Used to determine lookup strategy
//
// =============================================================================
// SPEC DEFINITIONS TO IMPLEMENT:
// =============================================================================
//
// From §5.1.3:
//   AliasExpand(path, Alias) =
//     let n = path[0] in
//     if IdKey(n) ∈ dom(Alias) then Alias[IdKey(n)] ++ path[1:]
//     else path
//
//   (Resolve-ModulePath):
//     Γ ⊢ AliasExpand(path, Alias) ⇓ path' ∧ StringOfPath(path') ∈ ModuleNames
//     → Γ ⊢ ResolveModulePath(path, Alias, ModuleNames) ⇓ path'
//
//   (ResolveModulePath-Err):
//     Γ ⊢ AliasExpand(path, Alias) ⇓ path' ∧ StringOfPath(path') ∉ ModuleNames
//     → Γ ⊢ ResolveModulePath(path, Alias, ModuleNames) ⇑
//
// From §5.1.6:
//   (Path-Unqualified):
//     |path| = 1 → PathIsQualified(path) = false
//
//   (Path-Qualified):
//     |path| > 1 → PathIsQualified(path) = true
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
//
// 1. Path resolution is a foundation for qualified lookups
// 2. Alias expansion happens before module existence check
// 3. ModuleNames should be pre-computed once per resolution pass
// 4. Consider PathKey caching for repeated lookups
// 5. Error messages should include both original and expanded paths
// 6. Import resolution uses same path resolution logic
//
// =============================================================================

