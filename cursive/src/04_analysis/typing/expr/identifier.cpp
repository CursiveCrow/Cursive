// =============================================================================
// MIGRATION MAPPING: expr/identifier.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.2.9: Type Inference (lines 9107-9173)
//   - Syn-Ident (lines 9123-9126): (x : T) in Gamma => Identifier(x) => T
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_infer.cpp
//   InferExprImpl() - IdentifierExpr case (lines 362-372)
//
// KEY CONTENT TO MIGRATE:
//   IDENTIFIER LOOKUP:
//   1. Look up identifier in current scope context
//   2. Return the bound type if found
//   3. Error if not found
//
//   IMPLEMENTATION:
//   - Use type_ident callback to resolve identifier
//   - type_ident performs scope lookup via ResolveValueName
//   - Returns the type from the binding
//
//   ERROR CASES:
//   - Identifier not found in scope
//   - Identifier refers to type, not value
//   - Identifier refers to module, not value
//
// DEPENDENCIES:
//   - IdentTypeFn callback type
//   - ScopeContext for lookup context
//   - ResolveValueName() from resolve/scopes_lookup.h
//
// SPEC RULES IMPLEMENTED:
//   - Syn-Ident: Identifier synthesis from binding
//
// RESULT TYPE:
//   ExprTypeResult { bool ok; optional<string_view> diag_id; TypeRef type; }
//
// FUNCTION SIGNATURE:
//   ExprTypeResult TypeIdentifierExpr(
//       const ScopeContext& ctx,
//       const syntax::IdentifierExpr& expr,
//       const IdentTypeFn& type_ident);
//
// =============================================================================

