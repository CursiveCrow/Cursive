/*
 * =============================================================================
 * MIGRATION MAPPING: attribute_targets.cpp
 * =============================================================================
 *
 * SPEC REFERENCE:
 *   - C0updated.md, Section 5.13.3 "Attribute Targets" (lines 14110-14200)
 *   - C0updated.md, Section 5.13.4 "Target Validation" (lines 14210-14280)
 *
 * SOURCE FILE:
 *   - cursive-bootstrap/src/03_analysis/attributes/attribute_registry.cpp (lines 1-456)
 *     (target handling integrated into registry)
 *
 * FUNCTIONS TO MIGRATE:
 *   - GetDeclTargetKind(Decl* decl) -> TargetKind
 *       Determine target kind from declaration
 *   - IsValidTarget(AttrDef* def, TargetKind kind) -> bool
 *       Check if attribute can be applied to target kind
 *   - GetTargetKindName(TargetKind kind) -> string
 *       Get human-readable name for target kind
 *   - ValidateProcedureAttribute(Attr* attr, ProcDecl* proc) -> bool
 *       Procedure-specific attribute validation
 *   - ValidateTypeAttribute(Attr* attr, TypeDecl* ty) -> bool
 *       Type-specific attribute validation
 *   - ValidateFieldAttribute(Attr* attr, FieldDecl* field) -> bool
 *       Field-specific attribute validation
 *
 * DEPENDENCIES:
 *   - Declaration AST nodes
 *   - TargetKind enumeration
 *   - Attribute definitions
 *
 * REFACTORING NOTES:
 *   1. Each attribute has a set of valid targets
 *   2. Some attributes have target-specific requirements
 *   3. [[layout(C)]] only valid on records/enums
 *   4. [[inline]] only valid on procedures
 *   5. [[dynamic]] only valid on procedures with contracts
 *   6. Consider attribute scopes (file, module, declaration)
 *
 * TARGET KINDS:
 *   - Procedure: procedure declarations
 *   - Record: record type declarations
 *   - Enum: enum type declarations
 *   - Modal: modal type declarations
 *   - Class: class declarations
 *   - Field: record/modal fields
 *   - Static: static declarations
 *   - Module: module-level attributes
 *
 * DIAGNOSTIC CODES:
 *   - E-ATTR-0010: Attribute not valid on procedure
 *   - E-ATTR-0011: Attribute not valid on type
 *   - E-ATTR-0012: Attribute not valid on field
 *   - E-ATTR-0013: Attribute requires contract
 *
 * =============================================================================
 */
