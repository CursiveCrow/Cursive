// =============================================================================
// MIGRATION MAPPING: scopes_lookup.cpp
// =============================================================================
//
// SPEC REFERENCE:
//   C0updated.md §5.1.3 "Lookup" (Lines 6822-6899)
//   C0updated.md §5.1.5 "Top-Level Name Collection" (Lines 6956-7309)
//
// SOURCE FILE:
//   cursive-bootstrap/src/03_analysis/resolve/scopes_lookup.cpp (Lines 1-240)
//
// DEPENDENCIES:
//   - cursive/include/04_analysis/resolve/scopes_lookup.h (header)
//   - cursive/src/04_analysis/resolve/scopes.h (IdKeyOf, etc.)
//   - cursive/src/00_core/symbols.h (StringOfPath)
//
// =============================================================================
// CONTENT TO MIGRATE:
// =============================================================================
//
// 1. AliasMap Functions (Source Lines 28-71)
//    - AliasExpand(path, alias) -> ModulePath: Expand first segment via alias
//    - AliasMapOf(names) -> AliasMap: Extract module aliases from NameMap
//    - AliasMapForCurrentModule(ctx, name_maps) -> AliasMap: Get current alias
//
// 2. ModuleNames Function (Source Lines 73-81)
//    - ModuleNamesOf(project) -> ModuleNames: Collect all module path strings
//
// 3. Entity Kind Predicates (Source Lines 83-106)
//    - ValueKind(entity) -> bool
//    - TypeKind(entity) -> bool
//    - ClassKind(entity) -> bool
//    - ModuleKind(entity) -> bool
//    - RegionAlias(entity) -> bool
//
// 4. Unqualified Lookup (Source Lines 108-121)
//    - Lookup(ctx, name) -> optional<Entity>
//    - Implements (Lookup-Unqualified), (Lookup-Unqualified-None)
//    - Linear search through scope list, return first match
//
// 5. Kind-Filtered Lookup (Source Lines 123-185)
//    - ResolveValueName(ctx, name) -> optional<Entity>
//    - ResolveTypeName(ctx, name) -> optional<Entity>
//    - ResolveClassName(ctx, name) -> optional<Entity>
//    - ResolveModuleName(ctx, name) -> optional<Entity>
//    - RegionAliasName(ctx, name) -> bool
//    - Note: ResolveValueName maps "~" to "self"
//
// 6. Module Path Resolution (Source Lines 187-201)
//    - ResolveModulePath(ctx, path, alias, module_names) -> Result
//    - Implements (Resolve-ModulePath), (ResolveModulePath-Err)
//    - Expands aliases, checks against visible module names
//
// 7. Qualified Lookup (Source Lines 203-237)
//    - ResolveQualified(ctx, name_maps, module_names, path, name, kind, can_access)
//    - Implements (Resolve-Qualified)
//    - Resolves module path, looks up name in module's NameMap
//    - Checks kind filter and accessibility
//
// =============================================================================
// SPEC DEFINITIONS TO IMPLEMENT:
// =============================================================================
//
// From §5.1.3:
//   Scopes(Γ) = [S_1, …, S_n]
//   i = min{j | IdKey(x) ∈ dom(S_j)}
//
//   (Lookup-Unqualified):
//     i defined → Γ ⊢ Lookup(x) ⇓ S_i[IdKey(x)]
//
//   (Lookup-Unqualified-None):
//     i undefined → Γ ⊢ Lookup(x) ↑
//
//   ValueKind(ent) ⇔ ent.kind = Value
//   TypeKind(ent) ⇔ ent.kind = Type
//   ClassKind(ent) ⇔ ent.kind = Class
//   ModuleKind(ent) ⇔ ent.kind = ModuleAlias
//   RegionAlias(ent) ⇔ ent.source = RegionAlias
//
//   (Resolve-Value-Name):
//     Γ ⊢ Lookup(x) ⇓ ent ∧ ValueKind(ent)
//     → Γ ⊢ ResolveValueName(x) ⇓ ent
//
//   (Resolve-ModulePath):
//     Γ ⊢ AliasExpand(path, Alias) ⇓ path' ∧ StringOfPath(path') ∈ ModuleNames
//     → Γ ⊢ ResolveModulePath(path, Alias, ModuleNames) ⇓ path'
//
//   (Resolve-Qualified):
//     Γ ⊢ ResolveModulePath(path, Alias, ModuleNames) ⇓ mp ∧
//     NameMap(P, mp)[IdKey(name)] = ent ∧
//     Γ ⊢ CanAccess(m, DeclOf(mp, name)) ⇓ ok ∧ K(ent)
//     → Γ ⊢ ResolveQualified(path, name, K) ⇓ ent
//
// From §5.1.5:
//   AliasMap(m) = { n ↦ origin | NameMap(P, m)[n].kind = ModuleAlias }
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
//
// 1. ResolveModulePathResult: { ok, diag_id, path: optional<ModulePath> }
// 2. ResolveQualifiedResult: { ok, diag_id, entity: optional<Entity> }
// 3. CanAccessFn type: function<AccessResult(ScopeContext&, ModulePath, name)>
// 4. ContainsName helper uses linear search on ModuleNames vector
// 5. ~ -> self mapping is specific to receiver self-reference syntax
//
// =============================================================================

