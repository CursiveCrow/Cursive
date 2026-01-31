// =============================================================================
// MIGRATION MAPPING: expr/field_access.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.3.2: Record Field Access (search for T-FieldAccess rules)
//   Section 5.2.7: Union Types - Union-DirectAccess-Err (line 9067-9070)
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/composite/records.cpp
//   Field access typing
//
// KEY CONTENT TO MIGRATE:
//   FIELD ACCESS TYPING (record.field):
//   1. Type the base expression
//   2. Strip permission qualifier to find record type
//   3. Look up field in record declaration
//   4. Check field visibility
//   5. Result type is field type (with base permission applied)
//
//   PERMISSION PROPAGATION:
//   - const record => const field
//   - unique record => unique field (if field doesn't have permission)
//   - shared record => shared field
//
//   UNION FIELD ACCESS:
//   - Direct field access on union is an error
//   - Must use match to narrow union first
//   - Error: Union-DirectAccess-Err
//
//   MODAL STATE FIELDS:
//   - Modal in specific state can access that state's fields
//   - E.g., Connection@Connected.socket is valid
//   - E.g., Connection.socket is error (state unknown)
//
// DEPENDENCIES:
//   - ExprTypeFn for base expression
//   - LookupRecordDecl() for record lookup
//   - Fields() for field list
//   - FieldType() for field type lookup
//   - CanAccessField() for visibility
//   - StripPerm() for base type extraction
//   - MakeTypePerm() for permission wrapping
//
// SPEC RULES:
//   - T-FieldAccess: Basic field access
//   - T-FieldAccess-Perm: With permission propagation
//   - Union-DirectAccess-Err: Union field access error
//
// RESULT TYPE:
//   ExprTypeResult { bool ok; optional<string_view> diag_id; TypeRef type; }
//
// ERROR CASES:
//   - Base is not a record
//   - Field not found
//   - Field not visible
//   - Direct access on union type
//   - Access on general modal (not state-specific)
//
// =============================================================================

