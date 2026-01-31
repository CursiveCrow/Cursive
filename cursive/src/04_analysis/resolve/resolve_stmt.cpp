// =============================================================================
// MIGRATION MAPPING: resolve_stmt.cpp
// =============================================================================
//
// SPEC REFERENCE:
//   C0updated.md §5.1.2 "Name Introduction and Shadowing" (Lines 6718-6821)
//   C0updated.md §5.1.7 "Resolution Pass" (Lines 7430-7549)
//
// SOURCE FILE:
//   cursive-bootstrap/src/03_analysis/resolve/resolver_expr.cpp (Lines 1000-1200)
//   (Statement resolution is embedded in resolver_expr.cpp)
//
// DEPENDENCIES:
//   - cursive/include/04_analysis/resolve/resolve_stmt.h (header)
//   - cursive/src/04_analysis/resolve/scopes.h (ScopeContext, PushScope, PopScope)
//   - cursive/src/04_analysis/resolve/scopes_intro.h (Intro, ShadowIntro)
//   - cursive/src/04_analysis/resolve/resolve_expr.h (ResolveExpr)
//   - cursive/src/04_analysis/resolve/resolve_pattern.h (ResolvePattern)
//   - cursive/src/04_analysis/resolve/resolve_types.h (ResolveType)
//
// =============================================================================
// CONTENT TO MIGRATE:
// =============================================================================
//
// 1. ResolveStmt Dispatcher (Source Lines 1100-1150)
//    - ResolveStmt(ctx, stmt) -> void
//    - Dispatches to specific statement resolvers
//    - Handles: Let, Var, Assign, Return, Break, Continue, Expr, Defer
//
// 2. ResolveLetStmt (Source Lines 1000-1040)
//    - ResolveLetStmt(ctx, let_stmt) -> void
//    - Resolves type annotation if present
//    - Resolves initializer expression
//    - Introduces binding via pattern (Intro or ShadowIntro)
//    - Implements (Resolve-Let-Stmt)
//
// 3. ResolveVarStmt (Source Lines 1040-1080)
//    - ResolveVarStmt(ctx, var_stmt) -> void
//    - Similar to Let but creates mutable binding
//    - Implements (Resolve-Var-Stmt)
//
// 4. ResolveAssignStmt (Source Lines 1080-1100)
//    - ResolveAssignStmt(ctx, assign) -> void
//    - Resolves target (place expression)
//    - Resolves value expression
//    - Assignment target validation in later pass
//
// 5. ResolveReturnStmt (Source Lines 1120-1140)
//    - ResolveReturnStmt(ctx, return_stmt) -> void
//    - Resolves return expression if present
//    - Return context tracked for flow analysis
//
// 6. ResolveBreakStmt (Source Lines 1140-1160)
//    - ResolveBreakStmt(ctx, break_stmt) -> void
//    - Resolves break value expression if present
//    - Loop context tracked for flow analysis
//
// 7. ResolveContinueStmt (Source Lines 1160-1170)
//    - ResolveContinueStmt(ctx, continue_stmt) -> void
//    - No expressions to resolve
//    - Loop context tracked for flow analysis
//
// 8. ResolveDeferStmt (Source Lines 1170-1200)
//    - ResolveDeferStmt(ctx, defer) -> void
//    - Resolves deferred block
//    - Defer registration handled in codegen
//
// =============================================================================
// SPEC DEFINITIONS TO IMPLEMENT:
// =============================================================================
//
// From §5.1.7:
//   (Resolve-Let-Stmt):
//     Γ ⊢ ResolveType(ty_opt) ⇓ ok ∧
//     Γ ⊢ ResolveExpr(init) ⇓ ok ∧
//     Γ ⊢ ResolvePattern(pat, shadow) ⇓ ok
//     → Γ ⊢ ResolveStmt(Let(pat, ty_opt, init, shadow)) ⇓ ok
//
//   (Resolve-Var-Stmt):
//     Same as Let but binding is mutable
//
//   (Resolve-Assign-Stmt):
//     Γ ⊢ ResolveExpr(target) ⇓ ok ∧
//     Γ ⊢ ResolveExpr(value) ⇓ ok
//     → Γ ⊢ ResolveStmt(Assign(target, value)) ⇓ ok
//
// From §5.1.2:
//   Let/Var statements use shadow parameter to determine
//   whether Intro or ShadowIntro is used for bindings.
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
//
// 1. Statement resolution modifies context (adds bindings)
// 2. Expression statement just resolves the expression
// 3. Shadow detection based on 'shadow' keyword in source
// 4. Consider extracting binding creation from pattern resolution
// 5. Defer blocks may need special scope handling
// 6. Return/break/continue need control flow context
// 7. Compound assignment (+=, etc.) desugared before resolution?
//
// =============================================================================

