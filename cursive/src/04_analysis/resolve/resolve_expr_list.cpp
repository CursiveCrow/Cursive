// =============================================================================
// MIGRATION MAPPING: resolve_expr_list.cpp
// =============================================================================
//
// SPEC REFERENCE:
//   C0updated.md §5.1.7 "Resolution Pass" (Lines 7430-7549)
//
// SOURCE FILE:
//   cursive-bootstrap/src/03_analysis/resolve/resolver_expr.cpp (Lines 400-560)
//
// DEPENDENCIES:
//   - cursive/include/04_analysis/resolve/resolve_expr_list.h (header)
//   - cursive/src/04_analysis/resolve/scopes.h (ScopeContext)
//   - cursive/src/04_analysis/resolve/resolve_expr.h (ResolveExpr)
//
// =============================================================================
// CONTENT TO MIGRATE:
// =============================================================================
//
// 1. ResolveExprList (Source Lines 400-430)
//    - ResolveExprList(ctx, exprs) -> void
//    - Resolves a sequence of expressions
//    - Used for function arguments, array elements, tuple elements
//    - Implements (Resolve-Expr-List)
//
// 2. ResolveArgList (Source Lines 430-480)
//    - ResolveArgList(ctx, args) -> void
//    - Resolves function call arguments
//    - Handles move arguments specially
//    - Validates argument count (deferred to type checking)
//    - Implements (Resolve-Arg-List)
//
// 3. ResolveMoveArg (Source Lines 460-480)
//    - ResolveMoveArg(ctx, arg) -> void
//    - Resolves moved argument expression
//    - Validates argument is a place expression
//    - Move semantics checked in later pass
//
// 4. ResolveNamedArgList (Source Lines 480-520)
//    - ResolveNamedArgList(ctx, named_args) -> void
//    - Resolves named arguments (for record construction)
//    - Validates no duplicate names
//    - Implements (Resolve-Named-Arg-List)
//
// 5. ResolveFieldInit (Source Lines 520-560)
//    - ResolveFieldInit(ctx, field) -> void
//    - Resolves field name and value expression
//    - Field existence checked in type checking
//
// =============================================================================
// SPEC DEFINITIONS TO IMPLEMENT:
// =============================================================================
//
// From §5.1.7:
//   (Resolve-Expr-List):
//     ∀ e ∈ exprs. Γ ⊢ ResolveExpr(e) ⇓ ok
//     → Γ ⊢ ResolveExprList(exprs) ⇓ ok
//
//   (Resolve-Arg-List):
//     ∀ arg ∈ args.
//       (arg.is_move → Γ ⊢ ResolveMoveArg(arg.expr) ⇓ ok) ∧
//       (¬arg.is_move → Γ ⊢ ResolveExpr(arg.expr) ⇓ ok)
//     → Γ ⊢ ResolveArgList(args) ⇓ ok
//
//   (Resolve-Named-Arg-List):
//     NoDuplicates(arg_names) ∧
//     ∀ (n, e) ∈ args. Γ ⊢ ResolveExpr(e) ⇓ ok
//     → Γ ⊢ ResolveNamedArgList(args) ⇓ ok
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
//
// 1. Expression list resolution is purely structural
// 2. Move argument validation may need place expression check
// 3. Named argument duplicate check is O(n^2) - consider set
// 4. Argument evaluation order: left-to-right
// 5. Consider parallel expression list for independent args
// 6. Empty lists are valid
//
// =============================================================================

