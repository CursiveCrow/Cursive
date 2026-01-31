// =============================================================================
// MIGRATION MAPPING: resolve_extern.cpp
// =============================================================================
//
// SPEC REFERENCE:
//   C0updated.md §5.1.7 "Resolution Pass" (Lines 7430-7549)
//   C0updated.md §12 "Foreign Function Interface" (FFI section)
//
// SOURCE FILE:
//   cursive-bootstrap/src/03_analysis/resolve/resolver_items.cpp (Lines 680-780)
//
// DEPENDENCIES:
//   - cursive/include/04_analysis/resolve/resolve_extern.h (header)
//   - cursive/src/04_analysis/resolve/scopes.h (ScopeContext)
//   - cursive/src/04_analysis/resolve/scopes_intro.h (Intro)
//   - cursive/src/04_analysis/resolve/resolve_types.h (ResolveType)
//   - cursive/src/04_analysis/resolve/resolve_contracts.h (ResolveForeignContract)
//
// =============================================================================
// CONTENT TO MIGRATE:
// =============================================================================
//
// 1. ResolveExternBlock (Source Lines 680-720)
//    - ResolveExternBlock(ctx, extern_block) -> void
//    - Validates ABI string is known
//    - Resolves each extern procedure declaration
//    - Implements (Resolve-Extern-Block)
//
// 2. ResolveExternProcedure (Source Lines 720-760)
//    - ResolveExternProcedure(ctx, proc) -> void
//    - Introduces procedure name into module scope
//    - Resolves parameter types (must be FfiSafe)
//    - Resolves return type (must be FfiSafe)
//    - Resolves foreign contracts if present
//    - Implements (Resolve-Extern-Procedure)
//
// 3. ValidateAbiString (Source Lines 700-710)
//    - ValidateAbiString(abi) -> bool
//    - Checks ABI is in known set: "C", "C-unwind", "system", etc.
//
// 4. ValidateFfiSafe (Source Lines 760-780)
//    - ValidateFfiSafe(type) -> bool
//    - Checks type can cross FFI boundary
//    - Deferred to type checking (needs type info)
//
// =============================================================================
// SPEC DEFINITIONS TO IMPLEMENT:
// =============================================================================
//
// From §5.1.7 / FFI section:
//   (Resolve-Extern-Block):
//     ValidAbiString(abi) ∧
//     ∀ proc ∈ procs. Γ ⊢ ResolveExternProcedure(proc) ⇓ ok
//     → Γ ⊢ ResolveExternBlock(extern_block) ⇓ ok
//
//   (Resolve-Extern-Procedure):
//     Γ ⊢ Intro(name, ⟨Value, proc, ⊥, Decl⟩) ⇓ Γ₁ ∧
//     ∀ param ∈ params. Γ₁ ⊢ ResolveType(param.ty) ⇓ ok ∧
//     Γ₁ ⊢ ResolveType(ret_ty) ⇓ ok ∧
//     (contract ≠ ⊥ → Γ₁ ⊢ ResolveForeignContract(contract) ⇓ ok)
//     → Γ ⊢ ResolveExternProcedure(proc) ⇓ ok
//
//   KnownAbiStrings = { "C", "C-unwind", "system", "stdcall",
//                       "fastcall", "vectorcall" }
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
//
// 1. Extern procedures don't have bodies
// 2. FfiSafe validation requires type information
// 3. Foreign contracts have special syntax (@foreign_assumes, etc.)
// 4. Extern block items are module-level
// 5. ABI string validation is syntactic
// 6. Consider separate pass for FfiSafe checking
// 7. Extern procedure entities marked as extern
//
// =============================================================================

