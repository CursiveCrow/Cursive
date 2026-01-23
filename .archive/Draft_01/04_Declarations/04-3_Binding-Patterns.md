# Cursive Language Specification

## Clause 4 — Declarations

**Clause**: 4 — Declarations
**File**: 05-3_Binding-Patterns.md
**Section**: §4.3 Binding Patterns
**Stable label**: [decl.pattern]  
**Forward references**: §4.2 [decl.variable], Clause 6 §6.3 [type.composite], Clause 7 §7.4 [expr.structured]

---

### §4.3 Binding Patterns [decl.pattern]

#### §4.3.1 Overview

[1] Binding patterns provide declarative destructuring for variable bindings. They allow a single declaration to introduce multiple identifiers by projecting fields or positional elements from a composite value.

[2] Patterns currently apply only to variable bindings (§4.2). Other constructs (parameters, match expressions) reference their own pattern grammars when available.

[3] Patterns preserve the visibility, storage duration, and shadowing behaviour of the enclosing binding.

#### §5.3.2 Syntax

**Binding patterns** take one of the following forms:
```
<identifier>
"{" <record_field_list> "}"
"(" <pattern_list> ")"
```

**Record field lists** match the pattern:
```
<record_field> [ "," <record_field> [ "," <record_field> ... ] ]
```

**Record fields** take one of the following forms:
```
<identifier>
<identifier> "as" <identifier>
<identifier> ":" <binding_pattern>
```

**Pattern lists** match the pattern:
```
<binding_pattern> [ "," <binding_pattern> [ "," <binding_pattern> ... ] ]
```

[ Note: See Annex A §A.3 [grammar.pattern] for the normative pattern grammar.
— end note ]

[1] `record_pattern` binds named fields; an optional `as` clause renames the bound field while preserving the source field name. `tuple_pattern` binds positional elements in order.

[2] Patterns appear within a variable binding as `let pattern: Type = initializer` or `var pattern: Type = initializer`. The type annotation applies to the whole pattern when it contains more than one identifier.

#### §5.3.3 Constraints

[1] _Type annotation required._ Any pattern containing more than one identifier shall include a type annotation of the form `:{Type}` in the enclosing binding. The annotated type must be compatible with the pattern shape (record or tuple). Missing annotations yield diagnostic E05-304.

[2] _Field matching._ Each `record_field` must correspond to a field in the annotated record type. Unknown fields raise diagnostic E05-301. When `field as binding` is used, `field` references the record field name and `binding` names the introduced identifier.

(2.1) _Partial matching._ Pattern matching is all-or-nothing: if any field in a record pattern fails to match (unknown field name) or any element in a tuple pattern has incorrect arity, the entire pattern binding is ill-formed and no bindings are introduced. Implementations shall emit a single diagnostic (E05-301 for record fields, E05-302 for tuple arity) and treat the binding as failed. Partial success is not permitted.

[3] _Tuple arity._ Tuple patterns shall list exactly the number of elements present in the annotated tuple type. Mismatches produce diagnostic E05-302.

[4] _Distinct identifiers._ Identifiers introduced by a pattern shall be unique within that pattern. Duplicates cause diagnostic E05-303.

[5] _Nested patterns._ Patterns may nest arbitrarily: record and tuple patterns may contain nested record, tuple, or identifier patterns. Each level of nesting follows the same typing rules recursively. The pattern as a whole shall type-check against the initializer expression's type.

[6] _Initialiser evaluation._ The initializer expression is evaluated once. Implementations may behave as if a temporary value were created and then projected recursively into individual bindings according to the pattern structure.

[7] _Visibility inheritance._ Identifiers introduced by the pattern inherit the visibility modifier and scope of the enclosing binding.

#### §5.3.4 Semantics

[1] Record pattern `let {field as name}: RecordType = expr` binds `name` to `expr.field`. Absent `as`, the binding identifier matches the field name.

[2] Tuple pattern `let (left, right): (T0, T1) = expr` binds `left` to the tuple’s element at index 0 and `right` to index 1.

[3] Patterns do not retain a reference to the composite value after destructuring; only the introduced identifiers remain in scope.

#### §5.3.5 Examples (Informative)

**Example 5.3.5.1 (Record pattern with renaming):**

```cursive
let {x, y as vertical}: Point2 = current_position()
// Binds identifiers `x` and `vertical` of type Point2
```

**Example 5.3.5.2 - invalid (Missing annotation):**

```cursive
let {real, imag} = complex_value  // error[E05-304]
```

**Example 5.3.5.3 (Tuple pattern):**

```cursive
let (min, max): (i32, i32) = bounds()
```

**Example 5.3.5.4 - invalid (Unknown record field):**

```cursive
let {width}: Point2 = current_position()  // error[E05-301]
```

### §5.3.6 Conformance Requirements [decl.pattern.requirements]

[1] Implementations shall require type annotations for multi-identifier patterns (E05-304) and ensure that record and tuple patterns match the annotated type's shape (E05-301–E05-302).

[2] Compilers shall reject patterns that introduce duplicate identifiers within the same binding (E05-303) and enforce the single-evaluation semantics described in §5.3.4.
