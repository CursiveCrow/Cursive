/*
 * =============================================================================
 * MIGRATION MAPPING: attribute_registry.cpp
 * =============================================================================
 *
 * SPEC REFERENCE:
 *   - C0updated.md, Section 5.13 "Attributes" (line 13848)
 *   - C0updated.md, Section 5.13.1 "Attribute Syntax" (lines 13860-13920)
 *   - C0updated.md, Section 5.13.2 "Built-in Attributes" (lines 13930-14100)
 *
 * SOURCE FILE:
 *   - cursive-bootstrap/src/03_analysis/attributes/attribute_registry.cpp (lines 1-456)
 *
 * FUNCTIONS TO MIGRATE:
 *   - RegisterBuiltinAttributes() -> void                          [lines 30-150]
 *       Register all built-in attributes in registry
 *   - LookupAttribute(Name name) -> AttributeDef*                  [lines 155-180]
 *       Find attribute definition by name
 *   - ParseAttributeArgs(Attr* attr, AttrDef* def) -> AttrArgs     [lines 185-280]
 *       Parse and validate attribute arguments
 *   - RegisterCustomAttribute(AttrDef* def) -> void                [lines 285-320]
 *       Register user-defined attribute
 *   - ValidateAttributeUsage(Attr* attr, Decl* target) -> bool     [lines 325-400]
 *       Validate attribute is used on valid target
 *   - GetAttributeTargets(AttrDef* def) -> TargetSet               [lines 405-440]
 *       Get valid targets for an attribute
 *   - AttributeConflicts(Attr* a, Attr* b) -> bool                 [lines 445-456]
 *       Check if two attributes conflict
 *
 * DEPENDENCIES:
 *   - Attribute AST nodes
 *   - Declaration targets
 *   - Argument parsing
 *
 * REFACTORING NOTES:
 *   1. Attributes use [[name]] or [[name(args)]] syntax
 *   2. Multiple attributes: [[attr1, attr2]] or [[attr1]][[attr2]]
 *   3. Each attribute has specific valid targets (procedures, types, etc.)
 *   4. Some attributes conflict with each other
 *   5. Consider attribute inheritance for nested declarations
 *
 * BUILT-IN ATTRIBUTES:
 *   | Attribute              | Valid Targets              |
 *   |------------------------|----------------------------|
 *   | [[inline]]             | procedure                  |
 *   | [[inline(always)]]     | procedure                  |
 *   | [[inline(never)]]      | procedure                  |
 *   | [[inline(hint)]]       | procedure                  |
 *   | [[cold]]               | procedure                  |
 *   | [[export]]             | procedure                  |
 *   | [[no_mangle]]          | procedure                  |
 *   | [[symbol("name")]]     | procedure                  |
 *   | [[unwind]]             | procedure                  |
 *   | [[layout(C)]]          | record, enum               |
 *   | [[ffi_pass_by_value]]  | record, enum               |
 *   | [[weak]]               | procedure, static          |
 *   | [[dynamic]]            | procedure (with contract)  |
 *   | [[static_dispatch_only]]| procedure (generic)       |
 *
 * DIAGNOSTIC CODES:
 *   - E-ATTR-0001: Unknown attribute
 *   - E-ATTR-0002: Invalid attribute target
 *   - E-ATTR-0003: Invalid attribute arguments
 *   - E-ATTR-0004: Conflicting attributes
 *
 * =============================================================================
 */
