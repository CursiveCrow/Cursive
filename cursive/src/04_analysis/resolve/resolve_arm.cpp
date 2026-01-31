// =============================================================================
// MIGRATION MAPPING: resolve_arm.cpp
// =============================================================================
//
// SPEC REFERENCE:
//   C0updated.md §5.1.2 "Name Introduction and Shadowing" (Lines 6718-6821)
//   C0updated.md §5.1.7 "Resolution Pass" (Lines 7430-7549)
//
// SOURCE FILE:
//   cursive-bootstrap/src/03_analysis/resolve/resolver_expr.cpp (Lines 620-720)
//
// DEPENDENCIES:
//   - cursive/include/04_analysis/resolve/resolve_arm.h (header)
//   - cursive/src/04_analysis/resolve/scopes.h (ScopeContext, PushScope, PopScope)
//   - cursive/src/04_analysis/resolve/resolve_pattern.h (ResolvePattern)
//   - cursive/src/04_analysis/resolve/resolve_expr.h (ResolveExpr)
//
// =============================================================================
// CONTENT TO MIGRATE:
// =============================================================================
//
// 1. ResolveMatchArm (Source Lines 650-720)
//    - ResolveMatchArm(ctx, arm) -> void
//    - Pushes new scope for arm
//    - Resolves pattern (introduces bindings)
//    - Resolves guard expression if present
//    - Resolves body expression
//    - Pops arm scope
//    - Implements (Resolve-Match-Arm)
//
// 2. ResolveArmPattern (Source Lines 660-680)
//    - ResolveArmPattern(ctx, pattern) -> void
//    - Resolves refutable pattern
//    - Introduces bindings into arm scope
//    - Pattern is never shadow in match arms
//
// 3. ResolveArmGuard (Source Lines 680-700)
//    - ResolveArmGuard(ctx, guard) -> void
//    - Resolves guard condition expression
//    - Guard can use pattern bindings
//    - Guard must be boolean (checked in type checking)
//
// 4. ResolveArmBody (Source Lines 700-720)
//    - ResolveArmBody(ctx, body) -> void
//    - Resolves arm body expression or block
//    - Body can use pattern bindings
//
// =============================================================================
// SPEC DEFINITIONS TO IMPLEMENT:
// =============================================================================
//
// From §5.1.7:
//   (Resolve-Match-Arm):
//     Γ ⊢ PushScope(S_arm) ⇓ Γ₁ ∧
//     Γ₁ ⊢ ResolvePattern(pat, shadow=false) ⇓ ok ∧
//     (guard ≠ ⊥ → Γ₁ ⊢ ResolveExpr(guard) ⇓ ok) ∧
//     Γ₁ ⊢ ResolveExpr(body) ⇓ ok ∧
//     Γ₁ ⊢ PopScope ⇓ Γ
//     → Γ ⊢ ResolveMatchArm(arm) ⇓ ok
//
// From §5.1.2:
//   Match arm patterns use Intro (not ShadowIntro) for bindings.
//   Shadowing is not allowed within match patterns.
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
//
// 1. Each match arm gets its own scope
// 2. Pattern bindings are local to the arm
// 3. Guard expression evaluated after pattern match
// 4. Guard can reference pattern-bound variables
// 5. Body expression can reference pattern-bound variables
// 6. Match exhaustiveness checked in later pass
// 7. Consider arm scope as child of match scope
//
// =============================================================================

