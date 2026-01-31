// =============================================================================
// MIGRATION MAPPING: resolve_stmt_seq.cpp
// =============================================================================
//
// SPEC REFERENCE:
//   C0updated.md §5.1.7 "Resolution Pass" (Lines 7430-7549)
//   C0updated.md §5.1.2 "Name Introduction and Shadowing" (Lines 6718-6821)
//
// SOURCE FILE:
//   cursive-bootstrap/src/03_analysis/resolve/resolver_expr.cpp (Lines 820-900)
//
// DEPENDENCIES:
//   - cursive/include/04_analysis/resolve/resolve_stmt_seq.h (header)
//   - cursive/src/04_analysis/resolve/scopes.h (ScopeContext)
//   - cursive/src/04_analysis/resolve/resolve_stmt.h (ResolveStmt)
//   - cursive/src/04_analysis/resolve/resolve_expr.h (ResolveExpr)
//
// =============================================================================
// CONTENT TO MIGRATE:
// =============================================================================
//
// 1. ResolveStmtSeq (Source Lines 840-880)
//    - ResolveStmtSeq(ctx, stmts) -> void
//    - Resolves a sequence of statements
//    - Statements resolved in order
//    - Bindings from earlier stmts visible to later stmts
//    - Implements (Resolve-Stmt-Seq)
//
// 2. ResolveBlockStmts (Source Lines 860-880)
//    - ResolveBlockStmts(ctx, stmts, tail_expr) -> void
//    - Resolves statement sequence plus optional tail expression
//    - Tail expression resolved after all statements
//    - Used for block expressions with final expression
//
// 3. ResolveBlockBody (Source Lines 820-860)
//    - ResolveBlockBody(ctx, body) -> void
//    - Handles both statement-only and expr-tail blocks
//    - Dispatches to appropriate resolver
//
// =============================================================================
// SPEC DEFINITIONS TO IMPLEMENT:
// =============================================================================
//
// From §5.1.7:
//   (Resolve-Stmt-Seq):
//     stmts = [s_1, ..., s_n] ∧
//     Γ_0 = Γ ∧
//     ∀ i ∈ 1..n. Γ_{i-1} ⊢ ResolveStmt(s_i) ⇓ Γ_i
//     → Γ ⊢ ResolveStmtSeq(stmts) ⇓ ok
//
//   (Resolve-Block-Stmts):
//     Γ ⊢ ResolveStmtSeq(stmts) ⇓ ok ∧
//     (tail ≠ ⊥ → Γ' ⊢ ResolveExpr(tail) ⇓ ok)
//     → Γ ⊢ ResolveBlockStmts(stmts, tail) ⇓ ok
//
// From §5.1.2:
//   Statement sequence resolution threads context through:
//   each statement may introduce bindings visible to subsequent statements.
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
//
// 1. Statement sequence is core to block resolution
// 2. Context threading: each stmt can modify scope
// 3. Order matters: bindings accumulate left-to-right
// 4. Tail expression uses final accumulated context
// 5. Error recovery: continue on non-fatal stmt errors
// 6. Consider iterator-based resolution for large blocks
// 7. Defer statements accumulated for later processing
//
// =============================================================================

