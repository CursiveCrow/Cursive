// =============================================================================
// MIGRATION MAPPING: resolve_callee.cpp
// =============================================================================
//
// SPEC REFERENCE:
//   C0updated.md §5.1.3 "Lookup" (Lines 6822-6899)
//   C0updated.md §5.1.6 "Qualified Disambiguation" (Lines 7310-7429)
//
// SOURCE FILE:
//   cursive-bootstrap/src/03_analysis/resolve/resolver_expr.cpp (Lines 400-480)
//   cursive-bootstrap/src/03_analysis/resolve/resolve_qual.cpp (Lines 40-80)
//
// DEPENDENCIES:
//   - cursive/include/04_analysis/resolve/resolve_callee.h (header)
//   - cursive/src/04_analysis/resolve/scopes.h (ScopeContext)
//   - cursive/src/04_analysis/resolve/scopes_lookup.h (ResolveValueName)
//   - cursive/src/04_analysis/resolve/resolve_qual.h (ResolveQualified)
//
// =============================================================================
// CONTENT TO MIGRATE:
// =============================================================================
//
// 1. ResolveCallee (Source resolver_expr.cpp Lines 400-440)
//    - ResolveCallee(ctx, callee) -> void
//    - Resolves the callee expression of a function call
//    - Handles: identifier, qualified name, expression
//    - Implements (Resolve-Callee)
//
// 2. ResolveCalleeIdent (Source Lines 405-420)
//    - ResolveCalleeIdent(ctx, ident) -> void
//    - Looks up identifier as value name
//    - Entity must be callable (procedure or function type)
//    - Callability checked in type checking
//
// 3. ResolveCalleeQualified (Source Lines 420-440)
//    - ResolveCalleeQualified(ctx, qname) -> void
//    - Resolves qualified procedure path
//    - Uses ResolveQualified with Value kind
//    - Access checking integrated
//
// 4. ResolveCalleeExpr (Source Lines 440-460)
//    - ResolveCalleeExpr(ctx, expr) -> void
//    - Resolves arbitrary expression as callee
//    - Expression must have function type (checked later)
//    - Used for closures/function values (if supported)
//
// =============================================================================
// SPEC DEFINITIONS TO IMPLEMENT:
// =============================================================================
//
// From §5.1.3:
//   (Resolve-Callee-Ident):
//     Γ ⊢ ResolveValueName(x) ⇓ ent ∧ Callable(ent)
//     → Γ ⊢ ResolveCallee(Ident(x)) ⇓ ok
//
// From §5.1.6:
//   (Resolve-Callee-Qualified):
//     Γ ⊢ ResolveQualified(path, name, Value, CanAccess) ⇓ ent ∧
//     Callable(ent)
//     → Γ ⊢ ResolveCallee(Qualified(path, name)) ⇓ ok
//
//   Callable(ent) ⇔ ent.kind = Value ∧ IsProcedure(ent.target)
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
//
// 1. Callee resolution is value-name lookup
// 2. Callable validation deferred to type checking
// 3. Method calls have separate resolution (receiver + method name)
// 4. Generic instantiation of callee handled after type inference
// 5. Consider caching resolved procedure entities
// 6. Overload resolution not applicable (no overloading in Cursive)
//
// =============================================================================

