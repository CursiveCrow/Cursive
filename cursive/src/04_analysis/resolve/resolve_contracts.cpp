// =============================================================================
// MIGRATION MAPPING: resolve_contracts.cpp
// =============================================================================
//
// SPEC REFERENCE:
//   C0updated.md §5.1.7 "Resolution Pass" (Lines 7430-7549)
//   C0updated.md §8 "Contracts" (Preconditions/Postconditions section)
//
// SOURCE FILE:
//   cursive-bootstrap/src/03_analysis/resolve/resolver_items.cpp (Lines 600-680)
//   [Contract resolution embedded in procedure resolution]
//
// DEPENDENCIES:
//   - cursive/include/04_analysis/resolve/resolve_contracts.h (header)
//   - cursive/src/04_analysis/resolve/scopes.h (ScopeContext)
//   - cursive/src/04_analysis/resolve/resolve_expr.h (ResolveExpr)
//   - cursive/src/02_source/ast/contracts.h (Contract, Precondition, Postcondition)
//
// =============================================================================
// CONTENT TO MIGRATE:
// =============================================================================
//
// 1. ResolveContract (Source Lines 600-640)
//    - ResolveContract(ctx, contract) -> void
//    - Resolves precondition and postcondition
//    - Introduces @result binding for postcondition
//    - Implements (Resolve-Contract)
//
// 2. ResolvePrecondition (Source Lines 610-625)
//    - ResolvePrecondition(ctx, pre) -> void
//    - Resolves precondition expression
//    - Expression must be pure (checked later)
//    - Can reference parameters
//
// 3. ResolvePostcondition (Source Lines 625-640)
//    - ResolvePostcondition(ctx, post) -> void
//    - Introduces @result binding
//    - Resolves postcondition expression
//    - Can reference @result and @entry(...)
//
// 4. ResolveEntryCapture (Source Lines 640-660)
//    - ResolveEntryCapture(ctx, entry_expr) -> void
//    - Resolves @entry(expr) intrinsic
//    - Expression evaluated at procedure entry
//    - Result must be Bitcopy or Clone
//
// 5. ResolveForeignContract (Source Lines 660-680)
//    - ResolveForeignContract(ctx, contract) -> void
//    - Resolves @foreign_assumes, @foreign_ensures
//    - Special syntax for extern procedure contracts
//    - Implements (Resolve-Foreign-Contract)
//
// =============================================================================
// SPEC DEFINITIONS TO IMPLEMENT:
// =============================================================================
//
// From Contracts section:
//   (Resolve-Contract):
//     Γ ⊢ ResolvePrecondition(pre) ⇓ ok ∧
//     Γ ⊢ Intro(`@result`, ⟨Value, ⊥, return_ty, Decl⟩) ⇓ Γ₁ ∧
//     Γ₁ ⊢ ResolvePostcondition(post) ⇓ ok
//     → Γ ⊢ ResolveContract(contract) ⇓ ok
//
//   (Resolve-Precondition):
//     Γ ⊢ ResolveExpr(pre_expr) ⇓ ok ∧ Pure(pre_expr)
//     → Γ ⊢ ResolvePrecondition(pre_expr) ⇓ ok
//
//   (Resolve-Postcondition):
//     Γ ⊢ ResolveExpr(post_expr) ⇓ ok ∧ Pure(post_expr)
//     → Γ ⊢ ResolvePostcondition(post_expr) ⇓ ok
//
//   (Resolve-Entry-Capture):
//     Γ ⊢ ResolveExpr(expr) ⇓ ok ∧
//     (Bitcopy(typeof(expr)) ∨ Clone(typeof(expr)))
//     → Γ ⊢ ResolveEntryCapture(@entry(expr)) ⇓ ok
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
//
// 1. Contracts use special scope with @result binding
// 2. @entry captures require type info for Bitcopy/Clone check
// 3. Purity checking deferred to later pass
// 4. Contract expressions cannot call capability-bearing procedures
// 5. Foreign contracts have different syntax
// 6. Consider ContractScope as specialized scope type
// 7. Contract verification mode affects codegen, not resolution
//
// =============================================================================

