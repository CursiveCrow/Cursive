// =============================================================================
// MIGRATION MAPPING: pattern/record_pattern.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.4: Pattern Matching
//   - Pat-Record (lines 9757-9766): Record pattern typing
//   - RecordPattern-UnknownField-Bind (lines 9768-9771): Unknown field
//   - Field visibility in patterns
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_pattern.cpp
//   Record pattern typing
//
// KEY CONTENT TO MIGRATE:
//   RECORD PATTERN (RecordName{ field1, field2: pat }):
//   1. Resolve record type
//   2. Check scrutinee type matches record
//   3. For each field pattern:
//      a. Verify field exists in record
//      b. Check field is visible
//      c. Type the field's sub-pattern
//   4. Collect bindings from all field patterns
//   5. Irrefutable (record type matches exactly)
//
//   TYPING RULE (Pat-Record):
//   T = TypePath(p)
//   RecordDecl(p) = R
//   For each fp in field_patterns:
//     FieldType(R, fp.name) = T_f
//     FieldVisible(context, R, fp.name)
//     Gamma |- fp.pattern <= T_f => B_f
//   B = union of all B_f
//   --------------------------------------------------
//   Gamma |- RecordName{ fields } <= T => ok, bindings = B
//
//   FIELD PATTERN FORMS:
//   - { x }: shorthand for { x: x }
//   - { x: pat }: field with explicit pattern
//   - { x: _ }: field ignored
//
//   UNKNOWN FIELD ERROR:
//   RecordPattern-UnknownField-Bind
//   - Error if field name not in record
//   - Check against record declaration
//
//   VISIBILITY:
//   - Private fields not accessible outside module
//   - Check FieldVisible for each field
//
//   IRREFUTABILITY:
//   - Record patterns are irrefutable
//   - Can be used in let/var bindings
//   - Type must match exactly (structural)
//
//   EXAMPLE:
//   let Point{ x, y } = point    // destructure all
//   let Point{ x, y: _ } = point // ignore y
//   let Point{ x: a, y: b } = pt // rename bindings
//
// DEPENDENCIES:
//   - RecordLookup for field resolution
//   - FieldVisible for visibility check
//   - PatternTypeFn for field patterns
//   - BindingMerge for collecting bindings
//
// SPEC RULES:
//   - Pat-Record (lines 9757-9766)
//   - RecordPattern-UnknownField-Bind (lines 9768-9771)
//
// RESULT TYPE:
//   PatternResult { ok: true, bindings: from_fields }
//
// NOTES:
//   - Irrefutable - can use in let/var
//   - Fields must be visible
//   - Shorthand { x } binds x to field x
//
// =============================================================================

