# Cursive Language Specification

## Clause 6 — Types

**Part**: VI — Type System  
**File**: 07-8_Type-Introspection.md
**Section**: §6.8 Type Introspection  
**Stable label**: [type.introspection]  
**Forward references**: Clause 7 [expr], Clause 15 [comptime], Clause 9 [generic], Clause 10 [memory]

---

### §6.8 Type Introspection [type.introspection]

[1] Type introspection provides compile-time access to metadata about types. It enables generic algorithms, diagnostics, and metaprogramming to query type properties without runtime overhead. Introspection facilities operate in two modes:

- **Type-level operators** such as `typeof(expr)` usable in type positions.
- **Compile-time queries** (e.g., `type_name<T>()`) available in compile-time evaluation contexts (Clause 16).

[2] Introspection never alters type semantics; it only exposes metadata. All results are immutable and must be consumed in compile-time contexts unless a function explicitly returns a runtime value (e.g., `type_id`).

#### §7.8.1 Syntax [type.introspection.syntax]

```
TypeOfExpr     ::= 'typeof' '(' Expression ')'
TypeNameExpr   ::= 'type_name' '<' Type '>' '()'
TypeIdExpr     ::= 'type_id' '<' Type '>' '()'
TypeInfoExpr   ::= 'type_info' '<' Type '>' '()'
```

`TypeOfExpr` is a type-level operator; the others produce values that may be used in compile-time or runtime contexts depending on the API.

#### §7.8.2 `typeof` Operator [type.introspection.typeof]

[3] `typeof(expr)` yields the static type of `expr` after type inference. It is valid only in type positions (e.g., type annotations, alias definitions). Formation rule:

$$
\dfrac{\Gamma \vdash e : \tau}{\Gamma \vdash \text{typeof}(e) : \text{Type}}
\tag{T-TypeOf}
$$

[4] `typeof` is evaluated during type checking. It cannot appear in runtime expressions; doing so emits diagnostic E07-900 (“`typeof` is a type-level operator”).

**Example 7.8.2.1**

```cursive
let value = compute()
let clone: typeof(value) = value.copy()
```

#### §7.8.3 Type Name Query [type.introspection.typename]

[5] `type_name<T>()` returns a `string@View` containing the canonical, fully-qualified type name.

- Accessible in both compile-time and runtime contexts.
- Guaranteed to be UTF-8.
- Name format is stable within a specification version (implementation may add informative namespace qualifiers but must document them).

Typing rule:

$$
\dfrac{\Gamma \vdash \tau : \text{Type}}{\Gamma \vdash \text{type\_name}<\tau>() : \text{string@View}}
\tag{T-TypeName}
$$

**Example 7.8.3.1**

```cursive
let label: string@View = type_name<[i32]>()
println("Type: {}", label)
```

#### §7.8.4 Type Identity [type.introspection.typeid]

[6] `type_id<T>()` yields a `TypeId` value (opaque, comparable) used to test type equality at runtime. `TypeId` supports `==` and `!=` only.

$$
\dfrac{\Gamma \vdash \tau : \text{Type}}{\Gamma \vdash \text{type\_id}<\tau>() : \text{TypeId}}
\tag{T-TypeId}
$$

`TypeId` satisfies:

- `type_id<T>() == type_id<U>()` iff `T ≡ U`.
- `TypeId : Copy`.

`TypeId` values may be stored at runtime for type-erasure scenarios.

#### §7.8.5 Detailed Type Information [type.introspection.typeinfo]

[7] `type_info<T>()` returns a compile-time constant of type `TypeInfo`. `TypeInfo` is defined as:

```cursive
record TypeInfo {
    name: string@View,
    size: usize,
    alignment: usize,
    kind: TypeKind,
    fields: [FieldInfo@Owned],
    variants: [VariantInfo@Owned],
}

enum TypeKind {
    Primitive,
    Tuple,
    Record,
    Enum,
    Union,
    Pointer,
    Modal,
    Function,
    Array,
    Slice,
    Other,
}

record FieldInfo {
    name: string@View,
    ty: Type,
    visibility: Visibility,
}

record VariantInfo {
    name: string@View,
    payload: [FieldInfo@Owned],
}
```

`TypeInfo` is evaluated in compile-time contexts. Attempting to use `type_info<T>()` at runtime emits E07-901. Elements `fields` and `variants` are empty for kinds where they are inapplicable.

Typing rule:

$$
\dfrac{\Gamma \vdash \tau : \text{Type}}{\Gamma \vdash \text{type\_info}<\tau>() : \text{TypeInfo}}
\tag{T-TypeInfo}
$$

`FieldInfo.ty` has kind `Type` and may only be inspected inside compile-time contexts (Clause 16). Using it in runtime expressions is rejected (E07-901).

#### §7.8.6 Compile-Time Context Requirements [type.introspection.comptime]

[8] Calls to `type_name`, `type_id`, and `type_info` are permitted in runtime code, but `type_info` results must remain compile-time constants (e.g., assigned to `const` bindings, used inside `comptime` blocks). Using `type_info` in runtime expressions triggers E07-901. `type_name` and `type_id` may be evaluated at runtime; their cost is implementation-defined but expected to be O(1).

[9] Introspection functions inside `comptime` blocks may be used to generate code (e.g., auto-derive implementations). Combine with Clause 16 semantics for compile-time evaluation.

#### §7.8.7 Interaction with Generics [type.introspection.generics]

[10] For generic parameters:

- `typeof<T>(expr)` is illegal; `typeof` always inspects expressions.
- `type_name<T>()` and `type_id<T>()` accept generic arguments instantiated in the current monomorphization.
- `type_info<T>()` reflects the monomorphized instantiation (after substitutions and alias expansion).

#### §7.8.8 Diagnostics [type.introspection.diagnostics]

[11] Required diagnostics:

| Code    | Condition                                                     |
| ------- | ------------------------------------------------------------- |
| E07-900 | `typeof` used in value position                               |
| E07-901 | `type_info` result escapes runtime context                    |
| E07-902 | Introspection invoked on ill-formed type                      |
| E07-903 | Attempt to inspect private field without visibility (tooling) |

E07-903 is emitted when tooling attempts to access private fields via `type_info` outside the defining module. Public metadata remains accessible.

#### §7.8.9 Examples (Informative) [type.introspection.examples]

```cursive
// Example: auto-generate debug output
comptime {
    let info = type_info<Point>()
    assert(info.kind == TypeKind::Record)
    let debug_impl = format!(
        "procedure debug(self: const Point) {{ println(\"Point(x: {{}}, y: {{}})\", self.{}, self.{}) }}",
        info.fields[0].name,
        info.fields[1].name,
    )
    compiler::emit_impl(debug_impl)
}

// Runtime type comparisons
let a_id = type_id<[i32; 10]>()
let b_id = type_id<[i32; 10]>()
assert(a_id == b_id)

let label = type_name<string@Managed \/ io::Error>()
println("Type label: {}", label)
```

#### §7.8.10 Conformance Requirements [type.introspection.requirements]

[12] Implementations shall:

1. Evaluate `typeof` during type checking and forbid value-position use.
2. Provide stable, fully-qualified names for `type_name`, documented per implementation.
3. Generate unique, comparable `TypeId` values respecting type equivalence.
4. Implement `type_info` as a compile-time constant with accurate metadata (size, alignment, fields, variants, kind) and enforce compile-time-only usage.
5. Detect misuse via diagnostics E07-900–E07-903.
6. Ensure introspection results are unaffected by optimization levels; metadata is purely structural.

[13] Deviations render an implementation non-conforming unless explicitly marked as implementation-defined elsewhere.

---

**Previous**: §7.7 Type Relations (§7.7 [type.relation]) | **Next**: Clause 8 — Expressions (§8.1 [expr.fundamental])
