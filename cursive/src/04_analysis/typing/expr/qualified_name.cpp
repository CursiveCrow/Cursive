// =============================================================================
// MIGRATION MAPPING: expr/qualified_name.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.1: Name Resolution (for qualified name lookup)
//   Section 5.2.3: Function Types (T-Proc-As-Value)
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/resolve/scopes_lookup.cpp
//   ResolveQualified() function
//
// KEY CONTENT TO MIGRATE:
//   QUALIFIED NAME RESOLUTION:
//   - module::name - value in module
//   - module::Type::name - associated value in type
//   - Enum::Variant - enum unit variant as value
//
//   PATH RESOLUTION STEPS:
//   1. Resolve module path prefix
//   2. Look up name in target module/type
//   3. Check visibility (public/internal/private/protected)
//   4. Return entity with resolved path and type
//
//   VALUE KINDS:
//   - Procedure: Function type
//   - Static binding: Static's type
//   - Enum unit variant: Enum type
//   - Record associated procedure: Method type
//
//   GENERIC HANDLING:
//   - If generic args present, perform substitution
//   - Check generic arg count matches parameter count
//   - Apply type bounds checking
//
// DEPENDENCIES:
//   - ScopeContext for resolution context
//   - ModuleNames for module lookup
//   - CollectNameMaps() for name tables
//   - EntityKind (Value, Type, Module)
//   - CanAccess() visibility predicate
//   - Monomorphize() for generic substitution
//
// SPEC RULES IMPLEMENTED:
//   - T-Proc-As-Value: Procedure reference yields function type
//   - Path resolution rules from Section 5.1
//
// RESULT TYPE:
//   ExprTypeResult { bool ok; optional<string_view> diag_id; TypeRef type; }
//
// ERROR CASES:
//   - Module not found
//   - Name not found in module
//   - Name is not a value (is a type or module)
//   - Visibility violation
//   - Generic argument count mismatch
//   - Generic bound violation
//
// =============================================================================

