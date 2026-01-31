// =============================================================================
// MIGRATION MAPPING: resolve_generics.cpp
// =============================================================================
//
// SPEC REFERENCE:
//   C0updated.md §5.1.2 "Name Introduction and Shadowing" (Lines 6718-6821)
//   C0updated.md §5.1.7 "Resolution Pass" (Lines 7430-7549)
//   C0updated.md §7 "Generics and Type Parameters"
//
// SOURCE FILE:
//   cursive-bootstrap/src/03_analysis/resolve/resolver_items.cpp (Lines 60-140)
//   cursive-bootstrap/src/03_analysis/resolve/resolver_types.cpp (Lines 60-100)
//
// DEPENDENCIES:
//   - cursive/include/04_analysis/resolve/resolve_generics.h (header)
//   - cursive/src/04_analysis/resolve/scopes.h (ScopeContext, PushScope)
//   - cursive/src/04_analysis/resolve/scopes_intro.h (Intro)
//   - cursive/src/04_analysis/resolve/resolve_types.h (ResolveType, ResolveClassPath)
//
// =============================================================================
// CONTENT TO MIGRATE:
// =============================================================================
//
// 1. ResolveGenericParams (Source resolver_items.cpp Lines 60-100)
//    - ResolveGenericParams(ctx, params) -> void
//    - Introduces type parameters into scope
//    - Resolves bounds (class constraints)
//    - Resolves where clause predicates
//    - Implements (Resolve-Generic-Params)
//
// 2. ResolveTypeParam (Source Lines 70-85)
//    - ResolveTypeParam(ctx, param) -> void
//    - Introduces single type parameter
//    - Resolves default type if present
//    - Implements (Resolve-Type-Param)
//
// 3. ResolveTypeBound (Source Lines 85-100)
//    - ResolveTypeBound(ctx, bound) -> void
//    - Resolves T <: ClassName bound
//    - Class path resolved via ResolveClassPath
//    - Implements (Resolve-Type-Bound)
//
// 4. ResolveWhereClause (Source Lines 100-140)
//    - ResolveWhereClause(ctx, where_clause) -> void
//    - Resolves each predicate in where clause
//    - Handles: Bitcopy, Clone, Drop, FfiSafe
//    - Handles: class constraints (T <: Class)
//    - Implements (Resolve-Where-Clause)
//
// 5. ResolveGenericArgs (Source resolver_types.cpp Lines 60-100)
//    - ResolveGenericArgs(ctx, args) -> void
//    - Resolves type arguments at instantiation site
//    - Validates arity (deferred to type checking)
//    - Implements (Resolve-Generic-Args)
//
// =============================================================================
// SPEC DEFINITIONS TO IMPLEMENT:
// =============================================================================
//
// From §5.1.7 / Generics section:
//   (Resolve-Generic-Params):
//     ∀ p ∈ params.
//       Γ ⊢ Intro(p.name, ⟨Type, ⊥, ⊥, Decl⟩) ⇓ Γ' ∧
//       (p.bound ≠ ⊥ → Γ' ⊢ ResolveTypeBound(p.bound) ⇓ ok) ∧
//       (p.default ≠ ⊥ → Γ' ⊢ ResolveType(p.default) ⇓ ok)
//     → Γ ⊢ ResolveGenericParams(params) ⇓ ok
//
//   (Resolve-Type-Bound):
//     Γ ⊢ ResolveClassPath(class_path) ⇓ ent ∧ ClassKind(ent)
//     → Γ ⊢ ResolveTypeBound(T <: class_path) ⇓ ok
//
//   (Resolve-Where-Clause):
//     ∀ pred ∈ predicates.
//       pred = Bitcopy(T) → T ∈ dom(Γ)
//       pred = Clone(T) → T ∈ dom(Γ)
//       pred = Drop(T) → T ∈ dom(Γ)
//       pred = T <: C → Γ ⊢ ResolveClassPath(C) ⇓ ok
//     → Γ ⊢ ResolveWhereClause(predicates) ⇓ ok
//
//   (Resolve-Generic-Args):
//     ∀ arg ∈ args. Γ ⊢ ResolveType(arg) ⇓ ok
//     → Γ ⊢ ResolveGenericArgs(args) ⇓ ok
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
//
// 1. Generic params introduced before other item content
// 2. Type parameters are Type kind entities
// 3. Bounds checked after parameter introduction
// 4. Where clause predicates use special syntax
// 5. Default type resolution may reference earlier params
// 6. Arity mismatch detected in type checking
// 7. Consider GenericScope as specialized scope type
//
// =============================================================================

