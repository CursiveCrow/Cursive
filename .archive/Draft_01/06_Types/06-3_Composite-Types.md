# Cursive Language Specification

## Clause 6 — Types

**Part**: VI — Type System  
**File**: 07-3_Composite-Types.md
**Section**: §6.3 Composite Types  
**Stable label**: [type.composite]  
**Forward references**: §6.4 [type.function], §6.5 [type.pointer], §6.7 [type.relation], §7.4 [expr.structured], §7.3 [expr.operator], Clause 8 [stmt], Clause 8 [generic], Clause 9 [memory]

---

### §6.3 Composite Types [type.composite]

[1] Composite types combine or select among other types. They cover structural products (tuples), nominal products (records), sequential collections (arrays, slices, strings, ranges), discriminated sums (enums), and safe unions (`τ₁ \/ τ₂`).

[2] Composite types integrate with permissions (§9.4), regions (§9.3), contracts (Clause 10), and generics (Clause 8). Each constructor follows the standard template: syntax, formation constraints, semantics, and canonical examples. Where relevant, Copy behaviors and size/alignment rules are stated explicitly.

[3] This subclause is organized as follows:

- §7.3.2 Tuples (structural products)
- §7.3.3 Records (nominal products with dual access)
- §7.3.4 Collections (arrays, slices, strings, ranges)
- §7.3.5 Enums (discriminated sums)
- §7.3.6 Union types (safe structural unions)
- §7.3.7 Conformance requirements

---

#### §7.3.2 Tuples [type.composite.tuple]

##### §7.3.2.1 Overview [type.composite.tuple.overview]

[4] Tuple types are anonymous structural products. Two tuple types are equivalent when they have the same arity and pairwise equivalent element types. Tuple components inherit the permissions of the aggregate.

##### §7.3.2.2 Syntax [type.composite.tuple.syntax]

[5] Tuple types require at least two elements. Single-element tuple syntax is prohibited to avoid ambiguity with parenthesized expressions. The zero-element product is the unit type (§7.2.6).

[ Note: See Annex A §A.3 [grammar.type] for the normative `TupleType` production. — end note ]

##### §7.3.2.3 Constraints [type.composite.tuple.constraints]

[7] Tuple well-formedness requires each component type to be well-formed:

$$
\dfrac{\Gamma \vdash \tau_1 : \text{Type} \quad \cdots \quad \Gamma \vdash \tau_n : \text{Type} \quad n \ge 2}{\Gamma \vdash (\tau_1, \ldots, \tau_n) : \text{Type}}
\tag{WF-Tuple}
$$

[8] Construction requires matching types for each element:

$$
\dfrac{\Gamma \vdash e_1 : \tau_1 \quad \cdots \quad \Gamma \vdash e_n : \tau_n}{\Gamma \vdash (e_1, \ldots, e_n) : (\tau_1, \ldots, \tau_n)}
\tag{T-Tuple-Ctor}
$$

[9] Projection is zero-based:

$$
\dfrac{\Gamma \vdash e : (\tau_1, \ldots, \tau_n) \quad 0 \le i < n}{\Gamma \vdash e.i : \tau_{i+1}}
\tag{T-Tuple-Proj}
$$

##### §7.3.2.4 Semantics [type.composite.tuple.semantics]

[10] Evaluation proceeds left-to-right; side effects in elements are sequenced accordingly. Runtime representation stores elements contiguously with padding inserted to satisfy each element’s alignment. Tuple alignment equals the maximum alignment of its elements; tuple size equals the padded sum of element sizes.

[11] Copy behavior and variance:

- $(\tau_1, \ldots, \tau_n)$ satisfies `Copy` iff every $\tau_i$ does.
- Tuples are invariant in each parameter position because elements may be both read and written.

##### §7.3.2.5 Examples (Informative) [type.composite.tuple.examples]

```cursive
let point: const (f64, f64) = (3.0, 4.0)
let (x, y) = point                 // Destructuring
let next = (x + 1.0, y + 1.0)      // New tuple, same structure
let dx = point.0                   // Positional access (0-based)
```

---

#### §7.3.3 Records [type.composite.record]

##### §7.3.3.1 Overview [type.composite.record.overview]

[12] Record types are nominal products with named fields and optional methods. They support dual access: named (`value.field`) and positional (`value.0`). Positional indices mirror declaration order and enforce the same visibility as named access.

##### §7.3.3.2 Syntax [type.composite.record.syntax]

[13] Record declarations consist of a record keyword, identifier, optional generic parameters, and a body containing field and method declarations. Visibility modifiers are `public`, `internal` (default), or `private` (§5.6).

[ Note: See Annex A §A.6 [grammar.declaration] for the normative `RecordDecl` and `FieldDecl` productions. — end note ]

##### §7.3.3.3 Constraints [type.composite.record.constraints]

[14] Field names shall be unique. Each field's type shall be well-formed. Formally:

$$
\dfrac{\text{record } R \{ f_1 : \tau_1, \ldots, f_n : \tau_n \} \text{ declared} \quad \Gamma \vdash \tau_i : \text{Type}}{\Gamma \vdash R : \text{Type}}
\tag{WF-Record}
$$

[15] Construction and field access rules:

$$
\dfrac{\Gamma \vdash e_i : \tau_i}{\Gamma \vdash R \{ f_1 : e_1, \ldots, f_n : e_n \} : R}
\tag{T-Record-Ctor}
$$

$$
\dfrac{\Gamma \vdash e : R \quad f_k \text{ visible}}{\Gamma \vdash e.f_k : \tau_k}
\tag{T-Record-Field}
$$

$$
\dfrac{\Gamma \vdash e : R \quad 0 \le i < n \quad \text{field } i \text{ visible}}{\Gamma \vdash e.i : \tau_{i+1}}
\tag{T-Record-Pos}
$$

##### §7.3.3.4 Semantics [type.composite.record.semantics]

[16] Records are nominal: two records are equivalent iff they share the same declaration. Fields are laid out in declaration order with padding inserted as required. Dual access refers to the same storage location. Struct update syntax (`R { field: value, ..expr }`) copies unspecified fields from `expr`.

[17] Copy behavior: a record satisfies `Copy` iff every field does and no user-defined destructor is attached. Variance is invariant per field for the same reason as tuples.

##### §7.3.3.5 Examples (Informative) [type.composite.record.examples]

```cursive
record Point { x: f64, y: f64 }
let origin = Point { x: 0.0, y: 0.0 }
let translated = Point { x: 5.0, ..origin }
let x_named = translated.x         // Named access
let x_pos = translated.0           // Positional access (same field)
```

```cursive
record BankAccount {
    public number: u64,
    private balance: f64,
}

let acct = BankAccount { number: 10_001, balance: 250.0 }
let seen = acct.number             // OK (public)
// let hidden = acct.1            // ERROR: index 1 refers to private field
```

---

#### §7.3.4 Collections [type.composite.collection]

[18] Collections are sequential composites. They include fixed-size arrays, dynamic slices, modal strings, and range types used for iteration and slicing.

##### §7.3.4.1 Arrays [type.composite.collection.array]

###### Overview

[19] Arrays `[T; n]` store exactly `n` elements of type `T`. Length `n` is a compile-time constant. Arrays support indexing, slicing, and iteration.

###### Syntax

```
ArrayType ::= '[' Type ';' ConstExpr ']'
ArrayLiteral ::= '[' Expr (',' Expr)* ']' | '[' Expr ';' ConstExpr ']'
ArrayIndex ::= Expr '[' Expr ']'
```

###### Constraints

[20] Array formation:

$$
\dfrac{\Gamma \vdash \tau : \text{Type} \quad n \in \mathbb{N}^+}{\Gamma \vdash [\tau; n] : \text{Type}}
\tag{WF-Array}
$$

[21] Array literals require each element to type-check as `τ`. Repeat literals `[e; n]` require `e : τ` and `τ : Copy`.

###### Semantics

[22] Elements are stored contiguously with no padding between them. Array alignment equals `align(τ)`; size equals `n × size(τ)`. Indexing evaluates the index expression, converts it to `usize`, and performs bounds checking (§8.4.7). Out-of-bounds indices raise a runtime diagnostic unless proven safe at compile time.

[23] `Copy` holds for `[τ; n]` iff `τ : Copy`.

###### Examples (Informative)

```cursive
let numbers: [i32; 5] = [1, 2, 3, 4, 5]
let zeros: [i32; 8] = [0; 8]         // repeat syntax
let first = numbers[0]
let row: [f64; 3] = [[1.0, 0.0, 0.0],
                     [0.0, 1.0, 0.0],
                     [0.0, 0.0, 1.0]][1]
```

---

##### §7.3.4.2 Slices [type.composite.collection.slice]

###### Overview

[24] Slice types `[T]` are dynamic views over contiguous sequences of `T`. A slice stores a pointer and length; it does not own the underlying data. Permissions on the slice govern aliasing.

###### Syntax

```
SliceType ::= '[' Type ']'
SliceExpr ::= Expr '[' RangeExpr? ']'
```

###### Constraints

[25] Slice formation:

$$
\dfrac{\Gamma \vdash \tau : \text{Type}}{\Gamma \vdash [\tau] : \text{Type}}
\tag{WF-Slice}
$$

Slicing requires the source to be an array, owned string, or pointer supporting the slicing operation, and the range to be within bounds.

###### Semantics

[26] A slice value is `{ ptr: Ptr<τ>, len: usize }`. Copy behavior: a slice satisfies `Copy` because the view is immutable with respect to ownership; mutability is represented through permissions on the pointer. Indexing a slice is identical to array indexing with bounds checking.

###### Examples (Informative)

```cursive
let data: [u8; 10] = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
let head: [u8] = data[..4]     // elements 0..3
let tail: [u8] = data[6..]     // elements 6..9
procedure checksum(bytes: [u8]): u8 { bytes.iter().fold(0, add) }
checksum(data[..])
```

---

##### §7.3.4.3 Strings [type.composite.collection.string]

###### Overview

[27] Strings are UTF-8 sequences with two modal states (defined in §7.6):

- `string@Managed`: growable, heap-allocated buffer (`ptr`, `len`, `cap`), not `Copy`.
- `string@View`: read-only view over UTF-8 data (`ptr`, `len`), `Copy`.

(27.1) **String type defaulting**: A bare `string` identifier in a type annotation defaults to `string@View`. The `string@Managed` state shall be explicitly specified. This default aligns with the common use case of string parameters and return types, which typically use read-only views. There is an implicit coercion `string@Managed <: string@View` established via the modal subtyping rule (`Sub-Modal-Widen`).

###### Syntax

```
StringType ::= 'string'              // Defaults to string@View in annotations
             | 'string' '@Managed'
             | 'string' '@View'
StringLiteral ::= '"' UTF8Text '"'
```

[ Note: See Annex A §A.2 for the authoritative grammar. — end note ]

(27.2) `string` is a built-in modal type (§7.6) with two states:

- `string@Managed` — manages heap-allocated UTF-8 storage and may reallocate or mutate.
- `string@View` — an immutable span consisting of pointer + length.
  Transition signatures `string@Managed::view(~) -> @View` and `string@View::to_managed(~) -> @Managed` define the canonical conversions.

###### Constraints

[28] Well-formedness follows the modal formation rules (§7.6). String literals have type `string@View`. Conversions:

- `string.from(view: string@View) : string@Managed`
- `view.to_managed() : string@Managed`
- Implicit coercion `string@Managed` → `string@View`

###### Semantics

[29] Layout on 64-bit targets:

- `string@Managed`: `{ ptr: Ptr<u8>, len: usize, cap: usize }` (24 bytes)
- `string@View`: `{ ptr: Ptr<u8>, len: usize }` (16 bytes)

`string@View : Copy`; `string@Managed` is movable but not `Copy`. All string forms guarantee UTF-8 validity, and `string@Managed` values implicitly coerce to `string@View` by invoking the modal transition above.

**Example 7.3.4.3.1 (Managed/View transitions):**

```cursive
let greeting: string@Managed = string.from("hello")
let view: string@View = greeting.view()    // implicit when needed
print_line(view)

let roundtrip: string@Managed = view.to_managed()
print_line(roundtrip.view())
```

[ Note: The procedures `view()` and `to_managed()` are mnemonics for the modal transitions declared in §7.6; concrete names will follow the eventual Clause 10/standard library surface when specified. — end note ]

###### Examples (Informative)

**Example 7.3.4.3.2 (String type defaulting):**

```cursive
// Bare 'string' defaults to string@View
procedure print_line(text: string) {      // text: string@View
    io::write_all(text)
}

// Explicit states must be specified for managed strings
let managed: string@Managed = string.from("hello")
let view: string@View = managed           // implicit coercion
let also_view: string = managed           // bare 'string' = string@View

print_line(view)
print_line(also_view)
print_line("literal")                     // literal : string@View
```

---

##### §7.3.4.4 Range Types [type.composite.collection.range]

###### Overview

[30] Range types underpin slicing and iteration. Cursive provides six constructors: `Range<T>`, `RangeInclusive<T>`, `RangeFrom<T>`, `RangeTo<T>`, `RangeToInclusive<T>`, and `RangeFull<T>`.

###### Syntax

Range literals map to constructors:

| Literal | Type constructor      | Bounds                         |
| ------- | --------------------- | ------------------------------ |
| `a..b`  | `Range<T>`            | inclusive start, exclusive end |
| `a..=b` | `RangeInclusive<T>`   | inclusive start and end        |
| `a..`   | `RangeFrom<T>`        | inclusive start, unbounded end |
| `..b`   | `RangeTo<T>`          | unbounded start, exclusive end |
| `..=b`  | `RangeToInclusive<T>` | unbounded start, inclusive end |
| `..`    | `RangeFull<T>`        | unbounded start and end        |

###### Constraints

[31] Each constructor is well-formed when `Γ ⊢ T : Type`:

$$
\dfrac{\Gamma \vdash T : \text{Type}}{\Gamma \vdash Range<T> : \text{Type}}
\quad
\cdots
\quad
\dfrac{\Gamma \vdash T : \text{Type}}{\Gamma \vdash RangeFull<T> : \text{Type}}
\tag{WF-Range}
$$

###### Semantics

[32] Ranges carry optional bounds. `Range<T>` stores `{ start_present: bool, start: T, end_present: bool, end: T, inclusive: bool }` with `inclusive = false`. `RangeInclusive<T>` stores `{ start: T, end: T }` (bounds always present). `RangeFull<T>` has no fields (unbounded). Bounds are evaluated eagerly; illegal constructions (e.g., `a..b` with `b < a` for numeric ranges) are diagnosed by the consuming API.

###### Examples (Informative)

```cursive
let data: [u8; 8] = [10, 11, 12, 13, 14, 15, 16, 17]
let prefix = data[..4]
let suffix = data[4..]
loop i in 0..data.len() { println("{}", data[i]) }
```

---

#### §7.3.5 Enums [type.composite.enum]

##### §7.3.5.1 Overview [type.composite.enum.overview]

[33] Enums are nominal discriminated sums. Variants may be unit-like, tuple-like, or struct-like. Exhaustive pattern matching is required to ensure all variants are handled.

##### §7.3.5.2 Syntax [type.composite.enum.syntax]

[33] Enum declarations consist of an enum keyword, identifier, optional generic parameters, and a body containing variant declarations. Variants may be unit-like (no payload), tuple-like (parenthesized type list), or record-like (braced field list).

[ Note: See Annex A §A.6 [grammar.declaration] for the normative `EnumDecl` and `VariantDecl` productions. — end note ]

##### §7.3.5.3 Constraints [type.composite.enum.constraints]

[34] Variants shall have unique names. Payload types shall be well-formed under the enum's generic parameters:

$$
\dfrac{\Gamma, \vec{\alpha} \vdash \tau_i : \text{Type}}{\Gamma \vdash enum\;E\langle \vec{\alpha} \rangle : \text{Type}}
\tag{WF-Enum}
$$

[35] Variant construction typing rules follow the payload form (unit, tuple, struct). Generic instantiation requires well-formed arguments.

##### §7.3.5.4 Semantics [type.composite.enum.semantics]

[36] Each enum value stores a discriminant plus payload. Layout is implementation-defined but shall allocate enough space for the largest payload and align to the maximum payload alignment. `Copy` holds iff every variant payload satisfies `Copy` and no destructor is defined.

[37] Pattern matching shall be exhaustive. The compiler emits a compile-time error when variants remain unmatched. Wildcards (`_`) count as covering remaining cases.

##### §7.3.5.5 Examples (Informative) [type.composite.enum.examples]

```cursive
enum Status {
    Pending,
    Running,
    Completed,
}

match Status::Running {
    Status::Pending => println("Not started"),
    Status::Running => println("In progress"),
    Status::Completed => println("Done"),
}
```

```cursive
enum Shape {
    Point,
    Circle(f64),
    Rectangle { width: f64, height: f64 },
}

let circle = Shape::Circle(2.0)
match circle {
    Shape::Point => println("point"),
    Shape::Circle(r) => println("radius {}", r),
    Shape::Rectangle { width, height } => println("{} × {}", width, height),
}
```

---

#### §7.3.6 Union Types [type.composite.union]

##### §7.3.6.1 Overview [type.composite.union.overview]

[38] Safe union types use the **union operator `\/` (reads as "or")** to combine types structurally. They are discriminated unions with automatically inserted runtime tags that track which component type is active. Unlike enums (which are nominal and require explicit declaration), unions are structural and may be formed inline without prior declaration.

[39] Union types enable error handling, optional values, and control-flow merging without manual wrapper types. The type system automatically widens component types to the union and enforces exhaustive pattern matching.

##### §7.3.6.2 Syntax [type.composite.union.syntax]

[40] Union type syntax:

```
UnionType ::= Type '\/' Type ('\/' Type)*
```

[ Note: See Annex A §A.3 [grammar.type] for complete union type grammar.
— end note ]

[41] **Union properties**:

- **Binary operator**: `\/` is a type-level binary operator
- **Associative**: `(τ₁ \/ τ₂) \/ τ₃ ≡ τ₁ \/ (τ₂ \/ τ₃)`
- **Commutative**: `τ₁ \/ τ₂ ≡ τ₂ \/ τ₁`
- **Idempotent**: `τ \/ τ ≡ τ`
- **Flattening**: Nested unions automatically flatten: `(τ₁ \/ τ₂) \/ τ₃ ≡ τ₁ \/ τ₂ \/ τ₃`

[42] Component ordering is semantically immaterial; `i32 \/ string` and `string \/ i32` denote the same type after normalization.

##### §7.3.6.3 Constraints [type.composite.union.constraints]

##### §7.3.6.3.1 Formation

[43] A union is well-formed when each component type is well-formed:

**Binary union formation:**

$$
\dfrac{\Gamma \vdash \tau_1 : \text{Type} \quad \Gamma \vdash \tau_2 : \text{Type}}{\Gamma \vdash \tau_1 \/ \tau_2 : \text{Type}}
\tag{WF-Union-Binary}
$$

**N-ary union formation:**

$$
\dfrac{\Gamma \vdash \tau_1 : \text{Type} \quad \cdots \quad \Gamma \vdash \tau_n : \text{Type} \quad n \geq 2}{\Gamma \vdash \tau_1 \/ \cdots \/ \tau_n : \text{Type}}
\tag{WF-Union-Nary}
$$

[44] **Normalization**: Implementations shall normalize unions by:

1. Flattening nested unions: `(τ₁ \/ τ₂) \/ τ₃` → `τ₁ \/ τ₂ \/ τ₃`
2. Removing duplicates: `τ \/ τ \/ υ` → `τ \/ υ`
3. Sorting components by canonical order (implementation-defined but stable)

After normalization, union types are compared component-wise for equivalence.

##### §7.3.6.3.2 Subtyping

[45] Each component type is automatically a subtype of the union:

**Component introduction:**

$$
\dfrac{i \in [1..n]}{\tau_i <: \tau_1 \/ \cdots \/ \tau_n}
\tag{Sub-Union-Intro}
$$

**Union subsumption:**

$$
\dfrac{\forall i.\, \tau_i <: \upsilon_1 \/ \cdots \/ \upsilon_m}{\tau_1 \/ \cdots \/ \tau_n <: \upsilon_1 \/ \cdots \/ \upsilon_m}
\tag{Sub-Union-Subsumption}
$$

If every component of the left union is a subtype of the right union (or one of its components), then the left union is a subtype of the right union.

[46] **Automatic widening**: Expressions of type `τᵢ` automatically coerce to type `τ₁ \/ ... \/ τₙ` when required by context:

$$
\dfrac{\Gamma \vdash e : \tau_i \quad \tau_i <: \tau_1 \/ \cdots \/ \tau_n}{\Gamma \vdash e : \tau_1 \/ \cdots \/ \tau_n}
\tag{Coerce-Union-Widen}
$$

This enables returning different component types from the same procedure without explicit wrapper construction.

##### §7.3.6.3.3 Pattern Matching and Exhaustiveness

[47] Pattern matching on unions requires exhaustive coverage of all component types:

**Exhaustiveness rule:**

[ Given: Union type $\tau_1 \/ \cdots \/ \tau_n$, match patterns $p_1, \ldots, p_m$ ]

$$
\dfrac{\bigcup_{j=1}^{m} \text{cover}(p_j) \supseteq \{\tau_1, \ldots, \tau_n\}}{\text{match}(\tau_1 \/ \cdots \/ \tau_n, [p_1, \ldots, p_m]) \text{ is exhaustive}}
\tag{WF-Union-Exhaustive}
$$

[48] Incomplete matches produce diagnostic E08-450 (non-exhaustive union match). Wildcard patterns (`_`) satisfy exhaustiveness for all remaining component types.

[49] **Type refinement in patterns**: Within a match arm matching component type `τᵢ`, the matched value is refined to type `τᵢ`:

```cursive
let result: i32 \/ Error = compute()

match result {
    value: i32 => {
        // value has type i32 here (refined)
        println("Success: {}", value)
    }
    error: Error => {
        // error has type Error here (refined)
        println("Failed: {}", error.message)
    }
}
```

##### §7.3.6.4 Semantics [type.composite.union.semantics]

##### §7.3.6.4.1 Runtime Representation

[50] Union types use **tagged union** representation:

**Layout:**

```
struct Union<T₁, ..., Tₙ> {
    discriminant: usize,           // Which component is active
    payload: max_size(T₁, ..., Tₙ) // Storage for largest component
}
```

[51] **Discriminant**: The discriminant is an unsigned integer requiring minimum bits to represent $n$ component types:

- 2-3 components: 2 bits (u8)
- 4-255 components: 8 bits (u8)
- 256+ components: 16 bits (u16)

Implementations may use larger discriminants for alignment purposes.

[52] **Size and alignment**:

$$
\text{sizeof}(\tau_1 \/ \cdots \/ \tau_n) = \text{sizeof(discriminant)} + \max_i(\text{sizeof}(\tau_i)) + \text{padding}
$$

$$
\text{alignof}(\tau_1 \/ \cdots \/ \tau_n) = \max(\text{alignof(discriminant)}, \max_i(\text{alignof}(\tau_i)))
$$

Padding is inserted to satisfy alignment requirements.

##### §7.3.6.4.2 Construction and Widening

[53] Union values are constructed by automatic widening:

**Typing rule:**

$$
\dfrac{\Gamma \vdash e : \tau_i \quad \tau_i <: \tau_1 \/ \cdots \/ \tau_n}{\Gamma \vdash e : \tau_1 \/ \cdots \/ \tau_n}
\tag{T-Union-Widen}
$$

The discriminant is set to indicate component `τᵢ`, and the payload stores the value.

[54] **No explicit constructor**: Union construction is automatic through subtyping. Expressions of component type automatically widen to union type when expected by context.

##### §7.3.6.4.3 Pattern Matching Semantics

[55] Pattern matching inspects the discriminant and extracts the payload:

**Match evaluation:**

[ Given: Union value $v : \tau_1 \/ \cdots \/ \tau_n$ with active component $\tau_k$ ]

$$
\dfrac{v \text{ has discriminant } k \quad p_j \text{ matches } \tau_k}{\text{match } v \{ \ldots, p_j \Rightarrow e_j, \ldots \} \text{ selects branch } j}
\tag{E-Union-Match}
$$

The pattern matching extracts the payload as type `τₖ` and evaluates the corresponding arm.

##### §7.3.6.4.4 Copy Behavior

[56] A union type satisfies `Copy` if and only if all component types satisfy `Copy`:

$$
\dfrac{\forall i.\, \tau_i : \text{Copy}}{\tau_1 \/ \cdots \/ \tau_n : \text{Copy}}
\tag{Prop-Union-Copy}
$$

If any component is move-only, the entire union is move-only.

##### §7.3.6.4.5 Never Type Absorption

[57] Union with never type `!` simplifies to the other type:

$$
\tau \/ ! \equiv \tau
\tag{Union-Never-Absorb}
$$

Since `!` is uninhabited, a union containing `!` can never hold a `!` value, so the union is equivalent to the union of all other components.

##### §7.3.6.5 Examples (Informative) [type.composite.union.examples]

**Example 7.3.6.1 (Error handling with unions):**

```cursive
procedure parse(input: string@View): i32 \/ parse::Error
{
    if input.is_empty() {
        result parse::Error::invalid_data("empty")  // Widens to union
    }
    result input.to_i32()  // Widens to union
}

let result = parse("42")
match result {
    value: i32 => println("Parsed: {}", value),
    err: parse::Error => println("Error: {}", err.message()),
}
```

[1] Both return paths produce union type `i32 \/ parse::Error` through automatic widening. The match expression exhaustively handles both components.

**Example 7.3.6.4 (Optional values via union):**

```cursive
type Option<T> = T \/ None

record None { }  // Unit-like type for "no value"

procedure find(items: [i32], target: i32): i32 \/ None
{
    loop i in 0..items.len() {
        if items[i] == target {
            result items[i]  // Widens to i32 \/ None
        }
    }
    result None { }  // Widens to i32 \/ None
}
```

**Example 7.3.6.6 - invalid (Non-exhaustive match):**

```cursive
let result: i32 \/ Error = compute()

match result {
    value: i32 => println("Got {}", value)
    // error[E08-450]: non-exhaustive match, missing Error case
}
```

##### §7.3.6.6 Integration with Type Inference [type.composite.union.inference]

[58] Union types participate in bidirectional type inference:

**Context-driven inference:**

```cursive
procedure example(): i32 \/ Error {
    if condition {
        result 42               // Infers i32, widens to i32 \/ Error
    } else {
        result Error::failed()  // Infers Error, widens to i32 \/ Error
    }
}
```

**Inference with multiple branches:**

```cursive
let value = if condition {
    42
} else {
    Error::failed()
}
// Infers type: i32 \/ Error (union of branch types)
```

[59] When branches produce different types, the type checker automatically forms a union of those types. If a return type annotation is provided, branches must be subtypes of that annotation.

##### §7.3.6.7 Diagnostic Requirements [type.composite.union.diagnostics]

[60] Union-related diagnostics:

| Code    | Condition                                         | Section |
| ------- | ------------------------------------------------- | ------- |
| E08-450 | Non-exhaustive match on union type                | §8.5    |
| E07-710 | Union component type not well-formed              | §7.3.6  |
| E07-711 | Type mismatch: cannot widen to union              | §7.3.6  |
| E07-712 | Single-component union (should use type directly) | §7.3.6  |

[61] Implementations shall provide clear diagnostics showing which union components are missing from match expressions and suggest adding the missing patterns.

---

#### §7.3.7 Conformance Requirements [type.composite.requirements]

[43] A conforming implementation shall:

- Support tuples, records, arrays, slices, strings, ranges, enums, and safe unions as specified.
- Enforce dual record access while respecting visibility modifiers.
- Uphold the `string@Managed`/`string@View` split, including defaulting rules and implicit coercion.
- Guarantee bounds checking semantics for arrays and slices, with diagnostics that match Clause 7 listings and Annex E §E.5 payload requirements.
- Provide exhaustive pattern diagnostics for enums and unions.
- Preserve the stated size/alignment rules and Copy behaviors.
- **Union types**: Implement automatic widening (Coerce-Union-Widen), normalization (flattening, duplicate removal), exhaustiveness checking (WF-Union-Exhaustive), and runtime tagging with minimal discriminant size.
- **Union type equivalence**: Normalize unions before comparison (associativity, commutativity, idempotence).

[44] Deviations from layout, semantics, or diagnostic obligations render the implementation non-conforming unless explicitly categorized as implementation-defined elsewhere.

---

**Previous**: §7.2 Primitive Types (§7.2 [type.primitive]) | **Next**: §7.4 Function and Procedure Types (§7.4 [type.function])
