/*
 * =============================================================================
 * MIGRATION MAPPING: attribute_validate.cpp
 * =============================================================================
 *
 * SPEC REFERENCE:
 *   - C0updated.md, Section 5.13.5 "Attribute Validation" (lines 14290-14400)
 *   - C0updated.md, Section 5.13.6 "Attribute Effects" (lines 14410-14500)
 *   - C0updated.md, Section 8.12 "E-ATTR Errors" (lines 22100-22200)
 *
 * SOURCE FILE:
 *   - cursive-bootstrap/src/03_analysis/attributes/attribute_registry.cpp (lines 1-456)
 *     (validation integrated into registry)
 *
 * FUNCTIONS TO MIGRATE:
 *   - ValidateAttributes(Decl* decl) -> bool
 *       Validate all attributes on a declaration
 *   - ValidateInlineAttribute(Attr* attr) -> bool
 *       Validate [[inline]] variants
 *   - ValidateLayoutAttribute(Attr* attr, TypeDecl* ty) -> bool
 *       Validate [[layout(C)]] requirements
 *   - ValidateExportAttribute(Attr* attr, ProcDecl* proc) -> bool
 *       Validate [[export]] on procedure
 *   - ValidateDynamicAttribute(Attr* attr, ProcDecl* proc) -> bool
 *       Validate [[dynamic]] with contract presence
 *   - ValidateFfiAttribute(Attr* attr, TypeDecl* ty) -> bool
 *       Validate [[ffi_pass_by_value]] requirements
 *   - CheckAttributeConflicts(Vec<Attr*> attrs) -> bool
 *       Check for conflicting attributes on same target
 *   - ApplyAttributeEffects(Decl* decl) -> void
 *       Apply semantic effects of attributes
 *
 * DEPENDENCIES:
 *   - Attribute AST nodes
 *   - Type declarations
 *   - FFI safety checking
 *   - Contract detection
 *
 * REFACTORING NOTES:
 *   1. [[layout(C)]]: All fields must be FfiSafe
 *   2. [[ffi_pass_by_value]]: Required for Drop types in FFI
 *   3. [[export]]: Procedure must have FfiSafe signature
 *   4. [[dynamic]]: Procedure must have contracts
 *   5. [[inline(always)]]/[[inline(never)]] conflict
 *   6. Validate attribute arguments match expected types
 *
 * VALIDATION RULES:
 *   [[inline]]:
 *     - Must be on procedure
 *     - Argument must be: (none), always, never, hint
 *
 *   [[layout(C)]]:
 *     - Must be on record or enum
 *     - All fields must be FfiSafe types
 *     - No permission qualifiers on fields
 *
 *   [[export]]:
 *     - Must be on procedure
 *     - Signature must be FfiSafe
 *     - Cannot take capability parameters
 *
 *   [[dynamic]]:
 *     - Must be on procedure
 *     - Procedure must have |= contract
 *
 * DIAGNOSTIC CODES:
 *   - E-ATTR-0020: Invalid inline argument
 *   - E-ATTR-0021: [[layout(C)]] with non-FfiSafe field
 *   - E-ATTR-0022: [[export]] with non-FfiSafe signature
 *   - E-ATTR-0023: [[dynamic]] without contract
 *   - E-ATTR-0024: Conflicting inline attributes
 *
 * =============================================================================
 */
