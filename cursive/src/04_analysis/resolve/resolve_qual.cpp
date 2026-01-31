// =============================================================================
// MIGRATION MAPPING: resolve_qual.cpp
// =============================================================================
//
// SPEC REFERENCE:
//   C0updated.md §5.1.6 "Qualified Disambiguation" (Lines 7310-7429)
//   C0updated.md §5.1.3 "Lookup" (Lines 6822-6899)
//
// SOURCE FILE:
//   cursive-bootstrap/src/03_analysis/resolve/resolve_qual.cpp (Lines 1-475)
//
// DEPENDENCIES:
//   - cursive/include/04_analysis/resolve/resolve_qual.h (header)
//   - cursive/src/04_analysis/resolve/scopes.h (ScopeContext)
//   - cursive/src/04_analysis/resolve/scopes_lookup.h (ResolveQualified)
//   - cursive/src/04_analysis/resolve/resolve_path.h (ResolveModulePath)
//   - cursive/src/04_analysis/resolve/visibility.h (CanAccess)
//   - cursive/src/04_analysis/resolve/resolve_types.h (ResolveType)
//
// =============================================================================
// CONTENT TO MIGRATE:
// =============================================================================
//
// 1. ResolveQualifiedForm (Source Lines 350-475)
//    - ResolveQualifiedForm(ctx, form) -> ResolveQualifiedResult
//    - Handles all qualified name forms: Type::Name, Module::Name, etc.
//    - Dispatches based on form structure
//    - Implements (Qualified-Form-Dispatch)
//
// 2. ResolveQualified (Source Lines 200-280)
//    - ResolveQualified(ctx, name_maps, module_names, path, name, kind, can_access)
//    - Core qualified lookup implementation
//    - Resolves module path, looks up name, checks access
//    - Implements (Resolve-Qualified)
//
// 3. ResolveRecordPath (Source Lines 280-320)
//    - ResolveRecordPath(ctx, path, args) -> Entity
//    - Resolves qualified record type
//    - Validates it's a record entity
//    - Implements (Qualified-Record-Path)
//
// 4. ResolveEnumUnit (Source Lines 120-160)
//    - ResolveEnumUnit(ctx, path) -> Entity
//    - Resolves unit enum variant (no payload)
//    - Validates variant exists
//    - Implements (Qualified-Enum-Unit)
//
// 5. ResolveEnumTuple (Source Lines 160-200)
//    - ResolveEnumTuple(ctx, path, args) -> Entity
//    - Resolves tuple enum variant
//    - Resolves argument expressions
//    - Implements (Qualified-Enum-Tuple)
//
// 6. ResolveEnumRecord (Source Lines 200-240)
//    - ResolveEnumRecord(ctx, path, fields) -> Entity
//    - Resolves record enum variant
//    - Resolves field expressions
//    - Implements (Qualified-Enum-Record)
//
// 7. ResolveArgs (Source Lines 80-120)
//    - ResolveArgs(ctx, args) -> void
//    - Resolves generic type arguments
//    - Used by all qualified forms with generics
//
// 8. ResolveAssociatedType (Source Lines 320-350)
//    - ResolveAssociatedType(ctx, base, name) -> Entity
//    - Resolves Type::AssociatedType syntax
//    - Validates base is a type with associated types
//    - Implements (Qualified-Associated-Type)
//
// =============================================================================
// SPEC DEFINITIONS TO IMPLEMENT:
// =============================================================================
//
// From §5.1.6:
//   (Qualified-Form-Dispatch):
//     Form = Module::Name → ResolveQualified
//     Form = Type::Variant → ResolveEnumVariant
//     Form = Type::Associated → ResolveAssociatedType
//
//   (Resolve-Qualified):
//     Γ ⊢ ResolveModulePath(path, Alias, ModuleNames) ⇓ mp ∧
//     NameMap(P, mp)[IdKey(name)] = ent ∧
//     Γ ⊢ CanAccess(m, DeclOf(mp, name)) ⇓ ok ∧ K(ent)
//     → Γ ⊢ ResolveQualified(path, name, K) ⇓ ent
//
//   (Qualified-Enum-Unit):
//     Γ ⊢ ResolveTypePath(enum_path) ⇓ ent ∧
//     TypeKind(ent) ∧ IsEnum(ent.target) ∧
//     variant ∈ Variants(ent.target) ∧ IsUnit(variant)
//     → Γ ⊢ ResolveEnumUnit(enum_path::variant) ⇓ variant_ent
//
//   (Qualified-Associated-Type):
//     Γ ⊢ ResolveTypePath(base) ⇓ ent ∧
//     name ∈ AssociatedTypes(ent.target)
//     → Γ ⊢ ResolveAssociatedType(base::name) ⇓ assoc_ent
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
//
// 1. Qualified disambiguation requires entity kind context
// 2. Enum variant resolution needs enum definition lookup
// 3. Associated type resolution needs class definition lookup
// 4. Consider caching resolved paths for repeated lookups
// 5. Error messages should show full qualification chain
// 6. Generic argument count validation happens here
// 7. Access checking integrated into qualified resolution
//
// =============================================================================

