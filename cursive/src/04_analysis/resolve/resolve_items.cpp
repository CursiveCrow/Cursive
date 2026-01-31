// =============================================================================
// MIGRATION MAPPING: resolve_items.cpp
// =============================================================================
//
// SPEC REFERENCE:
//   C0updated.md §5.1.7 "Resolution Pass" (Lines 7430-7549)
//   C0updated.md §5.1.2 "Name Introduction and Shadowing" (Lines 6718-6821)
//
// SOURCE FILE:
//   cursive-bootstrap/src/03_analysis/resolve/resolver_items.cpp (Lines 1-868)
//
// DEPENDENCIES:
//   - cursive/include/04_analysis/resolve/resolve_items.h (header)
//   - cursive/src/04_analysis/resolve/scopes.h (ScopeContext, PushScope, PopScope)
//   - cursive/src/04_analysis/resolve/scopes_intro.h (Intro, ShadowIntro)
//   - cursive/src/04_analysis/resolve/resolve_types.h (ResolveType)
//   - cursive/src/04_analysis/resolve/resolve_expr.h (ResolveExpr, ResolveBlock)
//   - cursive/src/04_analysis/resolve/resolve_pattern.h (ResolvePattern)
//   - cursive/src/04_analysis/resolve/resolve_generics.h (ResolveGenerics)
//
// =============================================================================
// CONTENT TO MIGRATE:
// =============================================================================
//
// 1. ResolveItem Dispatcher (Source Lines 780-868)
//    - ResolveItem(ctx, item) -> void
//    - Dispatches to specific item resolvers based on item kind
//    - Handles: Procedure, Record, Enum, Modal, Class, TypeAlias, Static, Using
//
// 2. ResolveProcedure (Source Lines 580-680)
//    - ResolveProcedure(ctx, proc) -> void
//    - Introduces generic parameters into scope
//    - Resolves parameter types and patterns
//    - Resolves return type
//    - Resolves procedure body in new scope
//    - Implements (Resolve-Procedure)
//
// 3. ResolveRecord (Source Lines 420-520)
//    - ResolveRecord(ctx, record) -> void
//    - Introduces Self into scope
//    - Resolves field types
//    - Resolves class implementations
//    - Resolves member procedures
//    - Implements (Resolve-Record)
//
// 4. ResolveEnum (Source Lines 320-418)
//    - ResolveEnum(ctx, enum_item) -> void
//    - Resolves variant payloads (tuple or record)
//    - Handles discriminant expressions
//    - Implements (Resolve-Enum)
//
// 5. ResolveModal (Source Lines 220-318)
//    - ResolveModal(ctx, modal) -> void
//    - Resolves each state block
//    - Resolves state-specific fields and methods
//    - Resolves transitions
//    - Implements (Resolve-Modal)
//
// 6. ResolveClass (Source Lines 140-218)
//    - ResolveClass(ctx, class_item) -> void
//    - Introduces generic parameters
//    - Resolves associated type declarations
//    - Resolves method signatures (no bodies)
//    - Implements (Resolve-Class)
//
// 7. ResolveTypeAlias (Source Lines 100-138)
//    - ResolveTypeAlias(ctx, alias) -> void
//    - Resolves the aliased type
//    - Handles generic parameters in alias
//    - Implements (Resolve-TypeAlias)
//
// 8. ResolveStatic (Source Lines 60-98)
//    - ResolveStatic(ctx, static_item) -> void
//    - Resolves type annotation
//    - Resolves initializer expression
//    - Implements (Resolve-Static)
//
// 9. ResolveUsing (Source Lines 30-58)
//    - ResolveUsing(ctx, using_item) -> void
//    - Already handled in top-level collection
//    - Validates imported names are accessible
//
// =============================================================================
// SPEC DEFINITIONS TO IMPLEMENT:
// =============================================================================
//
// From §5.1.7:
//   (Resolve-Procedure):
//     Γ ⊢ PushScope(S_params) ⇓ Γ₁ ∧
//     Γ₁ ⊢ ResolveParams(params) ⇓ ok ∧
//     Γ₁ ⊢ ResolveType(ret_ty) ⇓ ok ∧
//     Γ₁ ⊢ ResolveBlock(body) ⇓ ok
//     → Γ ⊢ ResolveProcedure(proc) ⇓ ok
//
//   (Resolve-Record):
//     Γ ⊢ Intro(`Self`, ⟨Type, ⊥, ⊥, Decl⟩) ⇓ Γ₁ ∧
//     ∀ f ∈ fields. Γ₁ ⊢ ResolveType(f.ty) ⇓ ok ∧
//     ∀ m ∈ methods. Γ₁ ⊢ ResolveProcedure(m) ⇓ ok
//     → Γ ⊢ ResolveRecord(rec) ⇓ ok
//
//   (Resolve-Enum):
//     ∀ v ∈ variants. Γ ⊢ ResolveVariant(v) ⇓ ok
//     → Γ ⊢ ResolveEnum(enum) ⇓ ok
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
//
// 1. Consider extracting ResolveParams as standalone helper
// 2. Self introduction should use consistent entity construction
// 3. Generic parameter scope should be pushed before Self for records
// 4. Member procedure resolution needs receiver type binding
// 5. Modal state resolution requires per-state scope contexts
// 6. Class method signatures don't have bodies - skip body resolution
// 7. Error recovery: continue resolving other items on failure
//
// =============================================================================

