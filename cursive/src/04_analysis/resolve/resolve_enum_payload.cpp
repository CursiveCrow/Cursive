// =============================================================================
// MIGRATION MAPPING: resolve_enum_payload.cpp
// =============================================================================
//
// SPEC REFERENCE:
//   C0updated.md §5.1.6 "Qualified Disambiguation" (Lines 7310-7429)
//   C0updated.md §5.1.7 "Resolution Pass" (Lines 7430-7549)
//
// SOURCE FILE:
//   cursive-bootstrap/src/03_analysis/resolve/resolve_qual.cpp (Lines 120-240)
//   cursive-bootstrap/src/03_analysis/resolve/resolver_items.cpp (Lines 320-418)
//
// DEPENDENCIES:
//   - cursive/include/04_analysis/resolve/resolve_enum_payload.h (header)
//   - cursive/src/04_analysis/resolve/scopes.h (ScopeContext)
//   - cursive/src/04_analysis/resolve/resolve_types.h (ResolveType)
//   - cursive/src/04_analysis/resolve/resolve_expr.h (ResolveExpr)
//   - cursive/src/02_source/ast/types.h (EnumVariant, TuplePayload, RecordPayload)
//
// =============================================================================
// CONTENT TO MIGRATE:
// =============================================================================
//
// 1. ResolveEnumVariant (Source resolve_qual.cpp Lines 120-240)
//    - ResolveEnumVariant(ctx, enum_entity, variant_name) -> Entity
//    - Validates variant exists in enum definition
//    - Determines variant kind (unit, tuple, record)
//    - Implements (Resolve-Enum-Variant)
//
// 2. ResolveTuplePayload (Source resolver_items.cpp Lines 350-380)
//    - ResolveTuplePayload(ctx, payload) -> void
//    - Resolves each field type in tuple variant
//    - Implements (Resolve-Tuple-Payload)
//
// 3. ResolveRecordPayload (Source resolver_items.cpp Lines 380-418)
//    - ResolveRecordPayload(ctx, payload) -> void
//    - Resolves each field name and type in record variant
//    - Validates no duplicate field names
//    - Implements (Resolve-Record-Payload)
//
// 4. ResolveEnumDiscriminant (Source resolver_items.cpp Lines 340-350)
//    - ResolveEnumDiscriminant(ctx, discrim) -> void
//    - Resolves discriminant expression
//    - Expression must be constant
//
// =============================================================================
// SPEC DEFINITIONS TO IMPLEMENT:
// =============================================================================
//
// From §5.1.6:
//   (Resolve-Enum-Variant):
//     Γ ⊢ ResolveTypePath(enum_path) ⇓ ent ∧
//     TypeKind(ent) ∧ IsEnum(ent.target) ∧
//     variant ∈ Variants(ent.target)
//     → Γ ⊢ ResolveEnumVariant(enum_path, variant) ⇓ variant_ent
//
// From §5.1.7:
//   (Resolve-Tuple-Payload):
//     ∀ ty ∈ fields. Γ ⊢ ResolveType(ty) ⇓ ok
//     → Γ ⊢ ResolveTuplePayload(payload) ⇓ ok
//
//   (Resolve-Record-Payload):
//     NoDuplicates(field_names) ∧
//     ∀ (n, ty) ∈ fields. Γ ⊢ ResolveType(ty) ⇓ ok
//     → Γ ⊢ ResolveRecordPayload(payload) ⇓ ok
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
//
// 1. Enum variant lookup requires enum definition access
// 2. Tuple payload is positional - no name resolution
// 3. Record payload has named fields - validate uniqueness
// 4. Discriminant expressions must be compile-time constants
// 5. Consider caching variant info for repeated lookups
// 6. Error messages should include enum and variant names
//
// =============================================================================

