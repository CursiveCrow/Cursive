// =============================================================================
// MIGRATION MAPPING: expr/qualified_apply.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.3.2: Record Expressions (for record construction)
//   Section 5.3.3: Enum Expressions (for enum variant construction)
//   Section 5.7: Modal Types (for modal state construction)
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/composite/records.cpp
//   (QualifiedApply handles Path::Constructor{ fields } syntax)
//
// KEY CONTENT TO MIGRATE:
//   QUALIFIED APPLY FORMS:
//   1. Record construction: RecordPath{ field: value, ... }
//   2. Enum variant construction: EnumPath::Variant(args) or EnumPath::Variant{ fields }
//   3. Modal state construction: ModalPath@State{ fields }
//
//   RECORD CONSTRUCTION:
//   - Resolve path to record declaration
//   - Check all required fields are provided
//   - Check no duplicate fields
//   - Check no extra fields
//   - Type-check each field value against field type
//   - Handle default field values
//   - Apply generic arguments if present
//
//   ENUM VARIANT CONSTRUCTION:
//   - Resolve path to enum declaration
//   - Find variant by name
//   - Check payload matches variant definition:
//     * Unit variant: no payload
//     * Tuple variant: positional args
//     * Record variant: named fields
//
//   MODAL STATE CONSTRUCTION:
//   - Resolve path to modal declaration
//   - Find state by name
//   - Check all state fields are provided
//   - Type is TypeModalState(path, state_name, generic_args)
//
// DEPENDENCIES:
//   - ResolveTypeName() for path resolution
//   - LookupRecordDecl(), LookupEnumDecl(), LookupModalDecl()
//   - Fields() for record field list
//   - Variants() for enum variant list
//   - States() for modal state list
//   - Subtyping() for field type compatibility
//   - Generic argument substitution
//
// RESULT TYPE:
//   ExprTypeResult { bool ok; optional<string_view> diag_id; TypeRef type; }
//
// ERROR CASES:
//   - Path does not resolve to type
//   - Missing required field
//   - Duplicate field
//   - Unknown field
//   - Field type mismatch
//   - Unknown enum variant
//   - Variant payload mismatch
//   - Unknown modal state
//
// =============================================================================

