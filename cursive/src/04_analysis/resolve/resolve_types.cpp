// =============================================================================
// MIGRATION MAPPING: resolve_types.cpp
// =============================================================================
//
// SPEC REFERENCE:
//   C0updated.md §5.1.3 "Lookup" (Lines 6822-6899)
//   C0updated.md §5.1.6 "Qualified Disambiguation" (Lines 7310-7429)
//   C0updated.md §5.1.7 "Resolution Pass" (Lines 7430-7549)
//
// SOURCE FILE:
//   cursive-bootstrap/src/03_analysis/resolve/resolver_types.cpp (Lines 1-311)
//
// DEPENDENCIES:
//   - cursive/include/04_analysis/resolve/resolve_types.h (header)
//   - cursive/src/04_analysis/resolve/scopes.h (ScopeContext)
//   - cursive/src/04_analysis/resolve/scopes_lookup.h (ResolveTypeName, ResolveClassName)
//   - cursive/src/04_analysis/resolve/resolve_qual.h (ResolveQualified)
//   - cursive/src/02_source/ast/types.h (Type AST nodes)
//
// =============================================================================
// CONTENT TO MIGRATE:
// =============================================================================
//
// 1. ResolveType Dispatcher (Source Lines 220-311)
//    - ResolveType(ctx, type) -> void
//    - Dispatches to specific type resolvers
//    - Handles: Named, Tuple, Array, Slice, Ptr, Union, Function, Modal, etc.
//
// 2. ResolveTypePath (Source Lines 140-180)
//    - ResolveTypePath(ctx, path) -> optional<Entity>
//    - Resolves unqualified type name via ResolveTypeName
//    - Resolves qualified type path via ResolveQualified
//    - Validates entity is TypeKind
//    - Implements (Resolve-Type-Path)
//
// 3. ResolveClassPath (Source Lines 100-138)
//    - ResolveClassPath(ctx, path) -> optional<Entity>
//    - Similar to ResolveTypePath but for class constraints
//    - Validates entity is ClassKind
//    - Implements (Resolve-Class-Path)
//
// 4. ResolveNamedType (Source Lines 60-98)
//    - ResolveNamedType(ctx, named) -> void
//    - Resolves the type path
//    - Resolves generic arguments
//    - Implements (Resolve-Named-Type)
//
// 5. ResolveTupleType (Source Lines 40-58)
//    - ResolveTupleType(ctx, tuple) -> void
//    - Resolves each element type
//    - Handles single-element tuple syntax
//
// 6. ResolveArrayType (Source Lines 25-38)
//    - ResolveArrayType(ctx, array) -> void
//    - Resolves element type
//    - Resolves size expression (if present)
//
// 7. ResolveUnionType (Source Lines 182-218)
//    - ResolveUnionType(ctx, union_ty) -> void
//    - Resolves each variant type
//    - Validates no duplicate types in union
//
// 8. ResolveFunctionType (Source Lines 85-99)
//    - ResolveFunctionType(ctx, fn_ty) -> void
//    - Resolves parameter types
//    - Resolves return type
//
// 9. ResolveModalType (Source Lines 160-180)
//    - ResolveModalType(ctx, modal_ty) -> void
//    - Resolves base modal type
//    - Validates state annotation if present
//
// =============================================================================
// SPEC DEFINITIONS TO IMPLEMENT:
// =============================================================================
//
// From §5.1.7:
//   (Resolve-Type-Path):
//     path = [n] ∧ Γ ⊢ ResolveTypeName(n) ⇓ ent
//     → Γ ⊢ ResolveTypePath(path) ⇓ ent
//
//   (Resolve-Type-Path-Qualified):
//     path = mp::n ∧ Γ ⊢ ResolveQualified(mp, n, Type) ⇓ ent
//     → Γ ⊢ ResolveTypePath(path) ⇓ ent
//
//   (Resolve-Named-Type):
//     Γ ⊢ ResolveTypePath(path) ⇓ ent ∧
//     ∀ arg ∈ args. Γ ⊢ ResolveType(arg) ⇓ ok
//     → Γ ⊢ ResolveType(Named(path, args)) ⇓ ok
//
// From §5.1.6:
//   (Qualified-Type-Lookup):
//     Γ ⊢ ResolveQualified(path, name, Type, CanAccess) ⇓ ent
//     → Γ ⊢ QualifiedTypeLookup(path::name) ⇓ ent
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
//
// 1. Consider caching resolved type paths for efficiency
// 2. Generic argument resolution should track arity mismatches
// 3. Modal type state validation requires modal definition lookup
// 4. Union type normalization: sort variants for canonical form
// 5. Function type resolution may need to handle move annotations
// 6. Ptr type resolution needs permission validation
// 7. Consider splitting into resolve_type_simple.cpp and resolve_type_compound.cpp
//
// =============================================================================

