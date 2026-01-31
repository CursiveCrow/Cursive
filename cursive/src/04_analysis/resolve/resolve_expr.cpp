// =============================================================================
// MIGRATION MAPPING: resolve_expr.cpp
// =============================================================================
//
// SPEC REFERENCE:
//   C0updated.md §5.1.3 "Lookup" (Lines 6822-6899)
//   C0updated.md §5.1.7 "Resolution Pass" (Lines 7430-7549)
//
// SOURCE FILE:
//   cursive-bootstrap/src/03_analysis/resolve/resolver_expr.cpp (Lines 1-1349)
//
// DEPENDENCIES:
//   - cursive/include/04_analysis/resolve/resolve_expr.h (header)
//   - cursive/src/04_analysis/resolve/scopes.h (ScopeContext, PushScope, PopScope)
//   - cursive/src/04_analysis/resolve/scopes_lookup.h (ResolveValueName)
//   - cursive/src/04_analysis/resolve/resolve_types.h (ResolveType)
//   - cursive/src/04_analysis/resolve/resolve_pattern.h (ResolvePattern)
//   - cursive/src/04_analysis/resolve/resolve_qual.h (ResolveQualifiedForm)
//   - cursive/src/02_source/ast/expr.h (Expr AST nodes)
//
// =============================================================================
// CONTENT TO MIGRATE:
// =============================================================================
//
// 1. ResolveExpr Dispatcher (Source Lines 1200-1349)
//    - ResolveExpr(ctx, expr) -> void
//    - Main dispatch for all expression kinds
//    - Handles ~40 expression types
//
// 2. ResolveIdentifier (Source Lines 180-220)
//    - ResolveIdentifier(ctx, ident) -> void
//    - Uses ResolveValueName for unqualified lookup
//    - Maps ~ to self for receiver reference
//    - Implements (Resolve-Ident-Expr)
//
// 3. ResolveQualifiedName (Source Lines 222-280)
//    - ResolveQualifiedName(ctx, qname) -> void
//    - Resolves module path
//    - Looks up name in target module's NameMap
//    - Implements (Resolve-Qualified-Expr)
//
// 4. ResolveCall (Source Lines 400-480)
//    - ResolveCall(ctx, call) -> void
//    - Resolves callee expression
//    - Resolves argument expressions
//    - Handles move arguments
//    - Implements (Resolve-Call-Expr)
//
// 5. ResolveMethodCall (Source Lines 482-560)
//    - ResolveMethodCall(ctx, method) -> void
//    - Resolves receiver expression
//    - Method name deferred to type checking
//    - Resolves argument expressions
//    - Implements (Resolve-Method-Expr)
//
// 6. ResolveBinary (Source Lines 280-320)
//    - ResolveBinary(ctx, binary) -> void
//    - Resolves left and right operands
//    - Operator semantics deferred to type checking
//
// 7. ResolveUnary (Source Lines 320-360)
//    - ResolveUnary(ctx, unary) -> void
//    - Resolves operand expression
//    - Handles address-of, deref, negate, not
//
// 8. ResolveIf (Source Lines 560-620)
//    - ResolveIf(ctx, if_expr) -> void
//    - Resolves condition
//    - Resolves then/else branches in new scopes
//    - Implements (Resolve-If-Expr)
//
// 9. ResolveMatch (Source Lines 620-720)
//    - ResolveMatch(ctx, match_expr) -> void
//    - Resolves scrutinee
//    - Resolves each arm (pattern + body)
//    - Implements (Resolve-Match-Expr)
//
// 10. ResolveLoop (Source Lines 720-820)
//     - ResolveLoop(ctx, loop_expr) -> void
//     - Handles infinite, conditional, and iterator loops
//     - Introduces loop variable for iterator form
//     - Resolves loop body in new scope
//     - Implements (Resolve-Loop-Expr)
//
// 11. ResolveBlock (Source Lines 820-880)
//     - ResolveBlock(ctx, block) -> void
//     - Pushes new scope
//     - Resolves statement sequence
//     - Pops scope
//     - Implements (Resolve-Block-Expr)
//
// 12. ResolveLiteral (Source Lines 100-140)
//     - ResolveLiteral(ctx, lit) -> void
//     - No resolution needed for most literals
//     - String literals may need interning
//
// 13. ResolveAlloc (Source Lines 880-920)
//     - ResolveAlloc(ctx, alloc) -> void
//     - Resolves allocated value expression
//     - Region context tracked separately
//
// 14. ResolveCast (Source Lines 360-400)
//     - ResolveCast(ctx, cast) -> void
//     - Resolves operand expression
//     - Resolves target type
//
// 15. ResolveMove (Source Lines 920-960)
//     - ResolveMove(ctx, move_expr) -> void
//     - Resolves operand (must be a place)
//     - Move semantics handled in later pass
//
// 16. ResolveWiden (Source Lines 960-1000)
//     - ResolveWiden(ctx, widen) -> void
//     - Resolves modal expression
//     - State erasure checked in type checking
//
// 17. ResolveTuple (Source Lines 140-180)
//     - ResolveTuple(ctx, tuple) -> void
//     - Resolves each element expression
//
// 18. ResolveArray (Source Lines 1000-1040)
//     - ResolveArray(ctx, array) -> void
//     - Resolves each element expression
//
// 19. ResolveRecord (Source Lines 1040-1100)
//     - ResolveRecord(ctx, record_expr) -> void
//     - Resolves record path
//     - Resolves field value expressions
//
// 20. ResolveField (Source Lines 1100-1140)
//     - ResolveField(ctx, field) -> void
//     - Resolves base expression
//     - Field name resolved in type checking
//
// 21. ResolveIndex (Source Lines 1140-1180)
//     - ResolveIndex(ctx, index) -> void
//     - Resolves base expression
//     - Resolves index expression
//
// 22. ResolveRange (Source Lines 1180-1200)
//     - ResolveRange(ctx, range) -> void
//     - Resolves start/end expressions if present
//
// =============================================================================
// SPEC DEFINITIONS TO IMPLEMENT:
// =============================================================================
//
// From §5.1.7:
//   (Resolve-Ident-Expr):
//     Γ ⊢ ResolveValueName(x) ⇓ ent
//     → Γ ⊢ ResolveExpr(Ident(x)) ⇓ ok
//
//   (Resolve-Call-Expr):
//     Γ ⊢ ResolveExpr(callee) ⇓ ok ∧
//     ∀ arg ∈ args. Γ ⊢ ResolveExpr(arg) ⇓ ok
//     → Γ ⊢ ResolveExpr(Call(callee, args)) ⇓ ok
//
//   (Resolve-Block-Expr):
//     Γ ⊢ PushScope(S_new) ⇓ Γ₁ ∧
//     Γ₁ ⊢ ResolveStmtSeq(stmts) ⇓ ok ∧
//     Γ₁ ⊢ PopScope ⇓ Γ
//     → Γ ⊢ ResolveExpr(Block(stmts)) ⇓ ok
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
//
// 1. Consider splitting into resolve_expr_simple.cpp and resolve_expr_compound.cpp
// 2. Expression resolution is purely structural - no type info needed
// 3. Place expressions (lvalues) may need separate tracking
// 4. Move expression validation should check operand is movable
// 5. Region/frame expressions need region context tracking
// 6. Async expressions (spawn, wait, yield) need special scope handling
// 7. Key block expressions need key path resolution
//
// =============================================================================

