// =============================================================================
// MIGRATION MAPPING: expr/record_literal.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.2: Static Semantics - Record Literals
//   - Record construction
//   - Field initialization
//   - Default construction
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_infer.cpp
//   Record literal typing
//
// KEY CONTENT TO MIGRATE:
//   RECORD LITERAL (RecordName{ field: value, ... }):
//   1. Resolve record type
//   2. For each field initializer:
//      a. Verify field exists
//      b. Check field is visible
//      c. Type value against field type
//   3. Verify all required fields initialized
//   4. Apply default values for omitted fields
//   5. Check type invariant (if any)
//   6. Result is record type
//
//   TYPING:
//   RecordDecl(path) = R
//   For each (f, v) in initializers:
//     FieldType(R, f) = T_f
//     FieldVisible(context, R, f)
//     Gamma |- v : T_v
//     T_v <: T_f
//   All required fields covered
//   TypeInvariant(R) holds
//   --------------------------------------------------
//   Gamma |- RecordName{ fields } : RecordType
//
//   FIELD FORMS:
//   - { x: expr }: explicit initialization
//   - { x }: shorthand for { x: x }
//
//   DEFAULT VALUES:
//   - Fields with defaults can be omitted
//   - Default expression evaluated at construction
//
//   DEFAULT CONSTRUCTION:
//   - RecordName() when all fields have defaults
//   - DefaultConstructible predicate
//
//   TYPE INVARIANT:
//   - where { predicate } on record
//   - Checked after all fields initialized
//   - Statically verified or [[dynamic]] runtime check
//
//   EXAMPLE:
//   let p: Point = Point{ x: 10, y: 20 }
//   let p2: Point = Point{ x, y }  // shorthand
//   let c: Counter = Counter()     // default construction
//
// DEPENDENCIES:
//   - RecordLookup for field resolution
//   - FieldVisible for visibility check
//   - ExprTypeFn for field value typing
//   - DefaultValueEval for defaults
//   - TypeInvariantCheck
//
// SPEC RULES:
//   - Record literal semantics
//   - T-Record-Default for default construction
//   - InitOk for field initialization
//
// RESULT TYPE:
//   Record type
//
// =============================================================================

