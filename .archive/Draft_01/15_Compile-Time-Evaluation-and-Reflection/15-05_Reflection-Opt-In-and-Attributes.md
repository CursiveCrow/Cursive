# Cursive Language Specification

## Clause 15 — Compile-Time Evaluation and Reflection

**Part**: XV — Compile-Time Evaluation and Reflection  
**File**: 16-5_Reflection-Opt-In-and-Attributes.md  
**Section**: §15.5 Reflection Opt-In and Attributes  
**Stable label**: [comptime.reflect.optin]  
**Forward references**: §16.6 [comptime.reflect.queries], Clause 5 §5.5 [decl.type], Clause 7 §7.8 [type.introspection], Annex A §A.9 [grammar.attribute]

---

### §15.5 Reflection Opt-In and Attributes [comptime.reflect.optin]

#### §15.5.1 Overview [comptime.reflect.optin.overview]

[1] Type reflection in Cursive is opt-in per type via the `[[reflect]]` attribute. This design ensures zero metadata overhead for types that do not use reflection while enabling comprehensive introspection for types that explicitly opt in.

[2] This subclause specifies the `[[reflect]]` attribute syntax, zero-cost guarantees, opt-in constraints, and the security/privacy implications of reflection visibility.

#### §15.5.2 Syntax [comptime.reflect.optin.syntax]

[3] The `[[reflect]]` attribute syntax:

**Reflect attributes** match the pattern:
```
"[[" "reflect" "]]"
```

**Reflected type declarations** match the pattern:
```
<reflect_attribute> [ <reflect_attribute> ... ] <type_declaration>
```

[ Note: See §1.4.3 [intro.attributes] for the general attribute grammar.
— end note ]

[4] The attribute appears immediately before the type declaration it modifies. Multiple attributes may precede a type; `[[reflect]]` may combine with other attributes (`[[repr(C)]]`, `[[repr(packed)]]`, etc.).

**Example 16.5.2.1** (Attribute placement):

```cursive
[[reflect]]
record Point {
    x: f64,
    y: f64,
}

[[repr(C)]]
[[reflect]]
record CCompatibleData {
    flags: u32,
    value: i64,
}
```

#### §15.5.3 Constraints [comptime.reflect.optin.constraints]

##### §15.5.3.1 Applicable Declarations

[5] _Type declarations only._ The `[[reflect]]` attribute shall apply exclusively to:

- Record declarations (`record Name { ... }`)
- Enum declarations (`enum Name { ... }`)
- Modal type declarations (`modal Name { ... }`)

[6] Applying `[[reflect]]` to other declarations (procedures, const bindings, primitive types, type aliases) produces diagnostic E16-100:

$$
\frac{\texttt{[[reflect]]} \text{ applied to non-type declaration}}{\text{ERROR E16-100: reflect attribute misplaced}}
\tag{WF-Reflect-Type-Only}
$$

##### §15.5.3.2 Reflection Permission

[7] _Opt-in requirement._ Reflection APIs (§16.6) shall operate only on types marked with `[[reflect]]`. Attempting to reflect on types without the attribute produces diagnostic E16-101:

$$
\frac{T \text{ not marked } \texttt{[[reflect]]} \quad \text{reflection query on } T}{\text{ERROR E16-101: type not marked for reflection}}
\tag{WF-Reflect-Opt-In}
$$

[8] This constraint ensures that reflection is explicit and prevents accidental exposure of type internals.

##### §15.5.3.3 Visibility Interaction

[9] _Reflection and visibility._ The `[[reflect]]` attribute does not override visibility modifiers (§5.6). Reflection queries from outside the defining module shall respect field and procedure visibility:

- Public fields/procedures: Full metadata available
- Internal/private fields/procedures: Names and types visible, offsets and details restricted

[10] Attempting to access private field offsets or detailed metadata from outside the defining module produces diagnostic E16-111 (specified in §16.6).

##### §15.5.3.4 Generic Type Reflection

[11] _Generic instantiation._ Generic types marked `[[reflect]]` may be reflected when instantiated with concrete type arguments:

```cursive
[[reflect]]
record Container<T> {
    items: [T],
}

comptime {
    let info <- reflect_type::<Container<i32>>()  // OK: concrete instantiation
    // info reflects Container<i32>, not Container<T>
}
```

[12] Attempting to reflect on uninstantiated generic types produces diagnostic E16-112.

#### §15.5.4 Semantics [comptime.reflect.optin.semantics]

##### §15.5.4.1 Zero-Cost Guarantee

[13] **Normative guarantee.** Types without the `[[reflect]]` attribute SHALL incur zero instance overhead:

$$
\frac{T \text{ not marked } \texttt{[[reflect]]}}{\text{sizeof}(T)_{\text{with\_reflection}} = \text{sizeof}(T)_{\text{without\_reflection}}}
\tag{P-Reflect-Zero-Instance-Cost}
$$

[14] Implementations shall not generate reflection metadata, increase type instance size, change alignment, or modify runtime representation for non-reflected types.

[15] **Metadata generation.** Types marked with `[[reflect]]` generate compile-time metadata structures. The metadata is embedded in the compilation unit's debug information or a separate reflection section, contributing to binary size but NOT increasing the size of type instances:

$$
\frac{T \text{ marked } \texttt{[[reflect]]}}{\text{sizeof}(T)_{\text{instance}} = \text{sizeof}(T)_{\text{without\_reflect}}}
\tag{P-Reflect-No-Instance-Overhead}
$$

[16] The metadata exists separately from instances; each instance has the same size regardless of reflection. Reflection metadata contributes to binary size (stored in debug information or dedicated sections) but does not affect runtime instance size or performance.

##### §15.5.4.2 Metadata Accessibility

[17] Reflection metadata is accessible only at compile time through reflection query procedures (§16.6). Runtime access to reflection metadata is an optional implementation feature outside the specification scope.

[18] **Compile-time only rule:**

$$
\frac{\text{reflection query on } T \text{ in runtime context}}{\text{ERROR E16-110: reflection requires comptime context}}
\tag{WF-Reflect-Comptime}
$$

#### §15.5.5 Security and Privacy Considerations [comptime.reflect.optin.security]

##### §15.5.5.1 Intentional Exposure

[19] The opt-in design prevents accidental exposure of internal type structure. Types containing sensitive data (cryptographic keys, passwords, internal caches) may omit `[[reflect]]` to prevent reflection:

**Example 16.5.5.1** (Sensitive type without reflection):

```cursive
record CryptoKey {
    private key_material: [u8; 32],
    private salt: [u8; 16],
}

// No [[reflect]] attribute
// Reflection queries on CryptoKey will fail with E16-101
```

##### §15.5.5.2 Selective Reflection

[20] Types may selectively opt into reflection while keeping related types opaque:

**Example 16.5.5.2** (Selective reflection):

```cursive
[[reflect]]
record PublicConfig {
    public host: string@View,
    public port: u16,
}

record InternalState {
    // No [[reflect]] - internal implementation details
    cache: [Entry; 1024],
    last_update: u64,
}
```

#### §15.5.6 Examples [comptime.reflect.optin.examples]

**Example 16.5.6.1** (Multiple reflected types):

```cursive
[[reflect]]
record Point {
    x: f64,
    y: f64,
}

[[reflect]]
record Circle {
    center: Point,
    radius: f64,
}

comptime {
    let point_info <- reflect_type::<Point>()
    let circle_info <- reflect_type::<Circle>()

    comptime_note(string_concat(
        "Point size: ",
        usize_to_string(point_info.size)
    ))

    comptime_note(string_concat(
        "Circle size: ",
        usize_to_string(circle_info.size)
    ))
}
```

**Example 16.5.6.2** (Reflection with repr attributes):

```cursive
[[repr(C)]]
[[repr(packed)]]
[[reflect]]
record PackedHeader {
    magic: u32,
    version: u16,
    flags: u16,
}

comptime {
    let info <- reflect_type::<PackedHeader>()

    comptime_assert(info.size == 8, "Packed header must be 8 bytes")
    comptime_assert(info.align == 1, "Packed alignment must be 1")
}
```

#### §15.5.7 Diagnostics [comptime.reflect.optin.diagnostics]

[21] Reflection opt-in diagnostics:

**Table 16.5 — Reflection opt-in diagnostics**

| Code    | Condition                                        | Constraint |
| ------- | ------------------------------------------------ | ---------- |
| E16-100 | [[reflect]] on non-type declaration              | [6]        |
| E16-101 | Reflection query on non-reflected type           | [7]        |
| E16-110 | Reflection API in runtime context                | [18]       |
| E16-111 | Private reflection details accessed cross-module | [10]       |
| E16-112 | Reflection on uninstantiated generic             | [12]       |

#### §15.5.8 Conformance Requirements [comptime.reflect.optin.requirements]

[22] A conforming implementation SHALL:

1. Support `[[reflect]]` attribute on record, enum, and modal type declarations
2. Reject `[[reflect]]` on non-type declarations with diagnostic E16-100
3. Enforce opt-in requirement: reject reflection queries on non-reflected types (E16-101)
4. Guarantee zero runtime overhead for types without `[[reflect]]`
5. Ensure reflected type instances have same size as non-reflected instances
6. Generate compile-time metadata only for types with `[[reflect]]`
7. Restrict reflection APIs to comptime contexts (E16-110)
8. Respect visibility modifiers during reflection queries (E16-111)
9. Support reflection on instantiated generic types
10. Reject reflection on uninstantiated generics (E16-112)

[23] A conforming implementation MAY:

1. Provide runtime reflection as an optional feature (not specified here)
2. Optimize metadata storage format
3. Share metadata across compilation units

[24] A conforming implementation SHALL NOT:

1. Generate metadata for non-reflected types
2. Increase instance size for reflected types
3. Allow reflection queries in runtime contexts
4. Automatically add `[[reflect]]` without programmer consent

---

**Previous**: §16.4 Comptime Intrinsics and Configuration (§16.4 [comptime.intrinsics]) | **Next**: §16.6 Type Reflection and Metadata Queries (§16.6 [comptime.reflect.queries])
