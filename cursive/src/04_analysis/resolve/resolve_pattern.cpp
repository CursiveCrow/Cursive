// =============================================================================
// MIGRATION MAPPING: resolve_pattern.cpp
// =============================================================================
//
// SPEC REFERENCE:
//   C0updated.md §5.1.2 "Name Introduction and Shadowing" (Lines 6718-6821)
//   C0updated.md §5.1.7 "Resolution Pass" (Lines 7430-7549)
//
// SOURCE FILE:
//   cursive-bootstrap/src/03_analysis/resolve/resolver_pat.cpp (Lines 1-283)
//
// DEPENDENCIES:
//   - cursive/include/04_analysis/resolve/resolve_pattern.h (header)
//   - cursive/src/04_analysis/resolve/scopes.h (ScopeContext)
//   - cursive/src/04_analysis/resolve/scopes_intro.h (Intro, ShadowIntro)
//   - cursive/src/04_analysis/resolve/resolve_types.h (ResolveType)
//   - cursive/src/02_source/ast/pattern.h (Pattern AST nodes)
//
// =============================================================================
// CONTENT TO MIGRATE:
// =============================================================================
//
// 1. ResolvePattern Dispatcher (Source Lines 220-283)
//    - ResolvePattern(ctx, pattern, is_shadow) -> void
//    - Dispatches to specific pattern resolvers
//    - Handles: Wildcard, Ident, Tuple, Record, Enum, Literal, Range, Modal
//
// 2. ResolveIdentPattern (Source Lines 60-100)
//    - ResolveIdentPattern(ctx, ident, is_shadow) -> void
//    - Introduces binding via Intro or ShadowIntro based on flag
//    - Resolves type annotation if present
//    - Implements (Resolve-Ident-Pattern)
//
// 3. ResolveTuplePattern (Source Lines 100-130)
//    - ResolveTuplePattern(ctx, tuple, is_shadow) -> void
//    - Resolves each element pattern
//    - Implements (Resolve-Tuple-Pattern)
//
// 4. ResolveRecordPattern (Source Lines 130-170)
//    - ResolveRecordPattern(ctx, record, is_shadow) -> void
//    - Resolves record type path
//    - Resolves field patterns
//    - Implements (Resolve-Record-Pattern)
//
// 5. ResolveEnumPattern (Source Lines 170-210)
//    - ResolveEnumPattern(ctx, enum_pat, is_shadow) -> void
//    - Resolves enum path (qualified or unqualified)
//    - Resolves payload pattern (tuple or record)
//    - Implements (Resolve-Enum-Pattern)
//
// 6. ResolveLiteralPattern (Source Lines 40-58)
//    - ResolveLiteralPattern(ctx, lit) -> void
//    - No bindings introduced
//    - Literal value validated in type checking
//
// 7. ResolveRangePattern (Source Lines 200-218)
//    - ResolveRangePattern(ctx, range) -> void
//    - Resolves start and end expressions
//    - Range bounds validated in type checking
//
// 8. ResolveModalPattern (Source Lines 185-200)
//    - ResolveModalPattern(ctx, modal, is_shadow) -> void
//    - Resolves modal type
//    - Resolves state-specific field patterns
//    - Implements (Resolve-Modal-Pattern)
//
// =============================================================================
// SPEC DEFINITIONS TO IMPLEMENT:
// =============================================================================
//
// From §5.1.7:
//   (Resolve-Ident-Pattern):
//     ¬shadow ∧ Γ ⊢ Intro(x, ⟨Value, ⊥, ⊥, Decl⟩) ⇓ Γ₁ ∧
//     Γ₁ ⊢ ResolveType(ty_opt) ⇓ ok
//     → Γ ⊢ ResolvePattern(Ident(x, ty_opt), shadow=false) ⇓ ok
//
//   (Resolve-Ident-Pattern-Shadow):
//     shadow ∧ Γ ⊢ ShadowIntro(x, ⟨Value, ⊥, ⊥, Decl⟩) ⇓ Γ₁ ∧
//     Γ₁ ⊢ ResolveType(ty_opt) ⇓ ok
//     → Γ ⊢ ResolvePattern(Ident(x, ty_opt), shadow=true) ⇓ ok
//
//   (Resolve-Tuple-Pattern):
//     ∀ p ∈ patterns. Γ ⊢ ResolvePattern(p, shadow) ⇓ ok
//     → Γ ⊢ ResolvePattern(Tuple(patterns), shadow) ⇓ ok
//
//   (Resolve-Record-Pattern):
//     Γ ⊢ ResolveTypePath(path) ⇓ ent ∧
//     ∀ (f, p) ∈ fields. Γ ⊢ ResolvePattern(p, shadow) ⇓ ok
//     → Γ ⊢ ResolvePattern(Record(path, fields), shadow) ⇓ ok
//
// From §5.1.2:
//   Pattern binding uses Intro for normal patterns, ShadowIntro when
//   shadow keyword present.
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
//
// 1. is_shadow parameter propagates through nested patterns
// 2. Type annotations in patterns need ResolveType call
// 3. Record/Enum path resolution uses ResolveTypePath
// 4. Wildcard pattern (_) introduces no bindings
// 5. Guard expressions in match arms resolved separately
// 6. Pattern bindings are always Value kind entities
// 7. Consider PatternBindings helper to extract all introduced names
//
// =============================================================================

