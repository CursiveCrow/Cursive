# Cursive Language Specification

## Clause 9 — Generics and Behaviors

**Clause**: 9 — Generics and Behaviors
**File**: 10-4_Behavior-Declarations.md
**Section**: §9.4 Behaviors: Declarations and Items
**Stable label**: [generic.behavior]  
**Forward references**: §10.2 [generic.parameter], §10.3 [generic.bounds], §10.5 [generic.implementation], §10.6 [generic.resolution], Clause 5 §5.5 [decl.type], Clause 7 [type], Clause 11 [memory], Clause 12 [contract], Clause 13 [witness]

---

### §9.4 Behaviors: Declarations and Items [generic.behavior]

#### §9.4.1 Overview [generic.behavior.overview]

##### §9.4.1.1 Behavior Purpose

[1] A _behavior_ is a declaration that provides concrete, reusable implementations of procedures and associated types. Behaviors enable code sharing across multiple types without inheritance: types attach behaviors to gain functionality through monomorphized dispatch.

[2] Behaviors are Cursive's mechanism for concrete code reuse. They provide default implementations that types may use directly or override. Every procedure in a behavior **shall** have a concrete body; behaviors with abstract (body-less) procedures are ill-formed.

[3] **Critical distinction**: Behaviors provide concrete code; contracts (Clause 12) specify abstract requirements. The separation ensures that interface obligations (contracts) remain distinct from implementation sharing (behaviors). The term "behavior" describes reusable type functionality; "predicate expression" refers to boolean-valued logical expressions used in contractual sequent clauses.

##### §9.4.1.2 Behaviors vs Contracts

[4] Table 10.4.1 summarizes the distinction:

| Aspect              | Behaviors                                     | Contracts (Clause 12)                     |
| ------------------- | --------------------------------------------- | ----------------------------------------- |
| Purpose             | Concrete code reuse                           | Abstract interface specification          |
| Procedure bodies    | ALL procedures MUST have bodies               | NO procedures may have bodies             |
| Associated types    | Concrete defaults (may be overridden)         | Abstract requirements (must be specified) |
| Dispatch            | Static (monomorphization)                     | Static or dynamic (witnesses)             |
| Attachment syntax   | `with Behavior` or standalone implementation  | `: Contract` in type declaration          |
| Implementation site | Inline in type body or standalone declaration | Inline in type body only                  |
| Use case            | Sharing default implementations, marker types | Defining behavioral requirements          |

[5] Types may attach both behaviors and contracts:

```cursive
record Account: Ledgered with Serializable, Display {
    // Ledgered is a contract (abstract requirement, defined in Clause 12)
    // Serializable and Display are behaviors (concrete code reuse)
}
```

#### §9.4.2 Syntax [generic.behavior.syntax]

[6] Behavior declaration syntax:

**Behavior declarations** match the pattern:
```
[ <visibility_modifier> ] "behavior" <identifier> [ <generic_params> ]
    [ <behavior_extension> ] <behavior_body>
```

**Behavior extension clauses** match the pattern:
```
"with" <behavior_reference> [ "," <behavior_reference> ... ]
```

where each **behavior reference** is a `<type_expression>` that must resolve to a behavior name.

**Behavior bodies** match the pattern:
```
"{" [ <behavior_item> ... ] "}"
```

where each **behavior item** takes one of the following forms:
```
<associated_type_decl>
<procedure_declaration>
```

**Associated type declarations** match the pattern:
```
"type" <identifier> [ "=" <type_expression> ] [ ":" <behavior_bounds> ]
```

[ Note: See Annex A §A.7 [grammar.declaration] for complete behavior grammar.
— end note ]

[7] Procedure declarations within behaviors follow §5.4 [decl.function] syntax with the receiver parameter using `Self` to denote the implementing type. Receiver shorthand (`~`, `~%`, `~!`) desugars to `self: const/shared/unique Self`.

#### §9.4.3 Constraints [generic.behavior.constraints]

##### §9.4.3.1 Mandatory Procedure Bodies

[8] **Normative requirement**: Every procedure declared in a behavior **shall** include a concrete body. Behaviors declaring procedures without bodies are ill-formed.

$$
\frac{\text{behavior } P \{ \text{procedure } m : \text{Signature} \} \quad m \text{ has no body}}{\text{ERROR E10-401: behavior procedure must have body}}
\tag{WF-Pred-Body-Required}
$$

[9] This constraint distinguishes behaviors from contracts. Contracts declare procedures without bodies (abstract interfaces, Clause 12); behaviors provide bodies (concrete implementations).

##### §9.4.3.2 Visibility and Scope

[10] Behaviors declared at module scope default to `internal` visibility. The modifiers `public`, `internal`, `private`, and `protected` apply per §5.6 [decl.visibility].

[11] Behaviors introduce type bindings into the module scope (§6.2). The behavior identifier enters the unified namespace and may not collide with other bindings.

##### §9.4.3.3 Behavior Extension

[12] Behaviors may extend other behaviors using the `with` clause:

```cursive
behavior Debug with Display {
    procedure debug(~): string@View
    {
        result "Debug: " ++ self.show()
    }

    // Inherits show() from Display behavior
}
```

[13] Extension requires that all extended behaviors are visible and that no name conflicts arise. When multiple extended behaviors declare procedures with the same name:

- If signatures are identical (same parameters and return type), one implementation satisfies all
- If signatures differ, the extending behavior must provide an explicit implementation resolving the conflict
- If no explicit implementation is provided for conflicting signatures, diagnostic E10-413 is emitted

Extended behaviors contribute their procedures to the extending behavior; implementations must provide bodies for inherited procedures or use the inherited defaults.

[14] Cyclic extension (a behavior extending itself directly or transitively) is ill-formed (diagnostic E10-402).

##### §9.4.3.4 Generic Behaviors

[15] Behaviors may be generic over type, const, and grant parameters:

```cursive
behavior Converter<T, U> {
    procedure convert(~): U
    {
        // Default implementation
        panic("Converter not implemented")
    }
}
```

[16] Generic behaviors follow the same parameter rules as generic types (§10.2). Bounds on behavior parameters constrain which types may satisfy the behavior.

##### §9.4.3.5 Self Type

[17] Within a behavior body, the keyword `Self` refers to the implementing type. It is available in:

- Procedure signatures (`procedure method(~): Self`)
- Procedure bodies (`result Self { ... }`)
- Associated type bounds (`where Self: OtherBehavior`)
- Type expressions (`Ptr<Self>`, `[Self]`)

[18] `Self` is a type variable that resolves to the concrete implementing type during monomorphization (§10.6).

#### §9.4.4 Associated Types in Behaviors [generic.behavior.assoc]

##### §9.4.4.1 Overview

[19] Behaviors may declare associated types: type members that implementing types specify. Associated types in behaviors may have default values; implementing types may override the default.

##### §9.4.4.2 Syntax

[20] Associated type declarations in behaviors:

```cursive
behavior Iterator {
    type Item = ()    // Default value

    procedure next(~%): Item
    {
        result ()
    }
}
```

[21] The syntax `type Item = ()` provides a concrete default. Implementing types may override:

```cursive
record Range with Iterator {
    type Item = i32   // Override default

    start: i32,
    end: i32,

    procedure next(~%): i32
    {
        // Custom implementation
    }
}
```

##### §9.4.4.3 Associated Type Bounds

[22] Associated types may have bounds constraining valid overrides:

```cursive
behavior Container {
    type Element: Copy = u8    // Bound: Element must satisfy Copy

    procedure get(~, index: usize): Element
    {
        // Default implementation
        panic("Container::get not implemented")
    }
}
```

[23] Implementing types must provide associated types satisfying the bounds:

$$
\frac{\text{behavior } P \{ \text{type } A : B = \tau_{\text{default}} \} \quad \text{type } R \text{ with } P \{ \text{type } A = \upsilon \}}{\Gamma \vdash \upsilon : B}
\tag{WF-Assoc-Bound}
$$

[24] Violating associated type bounds produces diagnostic E10-403.

##### §9.4.4.4 Associated Type Projection

[25] Associated types are accessed using projection syntax `T::AssocType` as specified in §10.3.6. Within generic contexts, projections enable algorithms polymorphic over the associated type:

```cursive
procedure sum_container<C>(container: C): i32
    [[ alloc::heap |- true => true ]]
    where C: Container,
          C::Element = i32
{
    var total: i32 = 0
    var i: usize = 0
    loop i < container.len() {
        total = total + container.get(i)
        i = i + 1
    }
    result total
}
```

##### §9.4.4.5 Examples

**Example 10.4.4.1 (Associated type with default):**

```cursive
behavior Producer {
    type Output = ()

    procedure produce(~%): Output
    {
        result ()
    }
}

record IntProducer with Producer {
    type Output = i32    // Override default

    counter: i32,

    procedure produce(~%): i32
    {
        result self.counter
    }
}

record UnitProducer with Producer {
    // Uses default: type Output = ()

    procedure produce(~%): ()
    {
        result ()  // Inherits default associated type
    }
}
```

#### §9.4.5 Marker Behaviors [generic.behavior.marker]

##### §9.4.5.1 Overview

[26] Marker behaviors are special built-in behaviors with no procedures and no associated types. They express compile-time properties about types for use in bounds and optimization.

[27] Cursive provides three marker behaviors:

- **`Copy`**: Type values can be duplicated bitwise
- **`Sized`**: Type has statically known size
- **`Drop`**: Type has custom destructor

[28] Marker behaviors are automatically derived when structural requirements are met. They may be explicitly attached using the behavior clause syntax but cannot be manually implemented with procedure bodies.

##### §9.4.5.2 Copy Behavior [generic.behavior.marker.copy]

###### Definition

[29] The `Copy` behavior indicates that values of a type can be duplicated bitwise without violating safety invariants.

```cursive
behavior Copy {
    // No procedures, no associated types
    // Marker only
}
```

###### Auto-Derivation Rules

[30] A type automatically satisfies `Copy` when:

1. All fields satisfy `Copy` (for records and tuples)
2. All variants satisfy `Copy` (for enums)
3. The type does not satisfy the `Drop` behavior (§10.4.5.4)
4. The type does not contain non-`Copy` fields

[31] **Structural copy rule**:

[ Given: Record `R { f₁: T₁, ..., fₙ: Tₙ }` ]

$$
\frac{\forall i. \; \Gamma \vdash T_i : \text{Copy} \quad R \text{ does not satisfy } \text{Drop}}{\ Gamma \vdash R : \text{Copy}}
\tag{Prop-Copy-Struct}
$$

###### Primitive Types

[32] All primitive types satisfy `Copy` (§7.2):

$$
\forall T \in \text{PrimitiveTypes}. \; T : \text{Copy}
\tag{Prop-Copy-Prim}
$$

where $\text{PrimitiveTypes} = \{i8, i16, i32, i64, i128, isize, u8, u16, u32, u64, u128, usize, f32, f64, bool, char, (), !\}$.

###### Copy Semantics

[33] Types satisfying `Copy` may be duplicated using explicit `copy` keyword (§11.5.3):

```cursive
let a: i32 = 42
let b = copy a    // Bitwise duplication
```

[34] `Copy` types do not transfer cleanup responsibility when copied; each copy is an independent value with its own cleanup obligations.

###### Explicit Attachment

[35] Types explicitly attach `Copy` using the behavior clause:

```cursive
record Point with Copy {
    x: f64,
    y: f64,
}
```

[36] The attachment is validated: if structural requirements are not met, diagnostic E10-411 is emitted.

###### Examples

**Example 10.4.5.1 (Copy for simple record):**

```cursive
record Coordinate with Copy {
    x: f64,
    y: f64,
    z: f64,
}

let p1 = Coordinate { x: 1.0, y: 2.0, z: 3.0 }
let p2 = copy p1    // OK: Coordinate satisfies Copy
```

**Example 10.4.5.2 - invalid (Copy with non-Copy field):**

```cursive
record Container with Copy {
    data: Buffer,    // Buffer does not satisfy Copy
}
// error[E10-411]: cannot derive Copy, field `data` is not Copy
```

[ Note: **Thread safety through permissions, not markers**. Cursive does not provide Send and Sync marker behaviors. Thread safety is expressed through the permission system (§11.4):

- **`const` permission**: Safe to share across threads (immutable, unlimited aliasing)
- **`unique` permission**: Safe to transfer across threads (exclusive ownership via move)
- **`shared` permission**: Requires synchronization primitives for thread safety (Mutex, Channel, etc.)

This design avoids redundancy between permissions and marker behaviors. Complete concurrency semantics are specified in Clause 14 [concurrency].
— end note ]

##### §9.4.5.3 Sized Behavior [generic.behavior.marker.sized]

###### Definition

[47] The `Sized` behavior indicates that a type has statically known size and alignment.

```cursive
behavior Sized {
    // Marker only
}
```

###### Auto-Derivation Rules

[48] All types satisfy `Sized` except:

- Slice types `[T]` (size depends on runtime length)
- Future unsized types (trait objects in later editions)

[49] **Default sized**:

$$
\frac{\tau \notin \{\text{slice types}\}}{\Gamma \vdash \tau : \text{Sized}}
\tag{Prop-Sized-Default}
$$

###### Semantics

[50] `Sized` types may be stored in bindings directly, used as procedure parameters by value, and allocated on the stack. Unsized types must appear behind pointers:

```cursive
let array: [i32; 10] = [0; 10]    // OK: Sized
let slice: [i32] = array[..]      // error: unsized type in binding

let slice_ptr: Ptr<[i32]>@Valid = &array[..]  // OK: behind pointer
```

[51] Generic type parameters have an implicit `Sized` bound unless explicitly relaxed with `?Sized` (future extension, not current edition).

###### Examples

**Example 10.4.5.5 (Sized constraint):**

```cursive
procedure store<T: Sized>(value: T): T
{
    let stored = value
    result stored
}

store(42)              // OK: i32 is Sized
// store(array[..])    // error: [i32] is not Sized
```

##### §9.4.5.4 Drop Behavior [generic.behavior.marker.drop]

###### Definition

[37] The `Drop` behavior indicates that a type has a custom destructor. It provides the procedure invoked when a responsible binding goes out of scope (§11.2.5).

```cursive
behavior Drop {
    procedure drop(~!)
}
```

[ Note: Unlike other marker behaviors, `Drop` has a procedure signature. However, it remains a marker in the sense that it is automatically invoked by the runtime rather than called explicitly by user code.
— end note ]

###### Manual Implementation Only

[53] `Drop` is never auto-derived. Types must explicitly implement `Drop` by providing a `drop` procedure:

```cursive
record FileHandle {
    path: string@Managed,
    descriptor: i32,
}

behavior Drop for FileHandle {
    procedure drop(~!)
        [[ fs::close |- true => true ]]
    {
        os::close_fd(self.descriptor)
    }
}
```

[54] The `drop` procedure receives `unique Self` (exclusive access) and has no return value (implicitly returns `()`).

###### Drop and Copy Mutual Exclusion

[55] Types implementing `Drop` cannot satisfy `Copy`:

$$
\frac{\Gamma \vdash T : \text{Drop}}{\Gamma \vdash T \not: \text{Copy}}
\tag{Prop-Drop-Not-Copy}
$$

[56] Attempting to attach both `Copy` and `Drop` to the same type produces diagnostic E10-412.

###### Automatic Field Destruction

[57] After a type's `drop` procedure completes, the compiler automatically destroys fields in reverse declaration order (§11.2.5.3). Custom `drop` procedures need only perform type-specific cleanup; field cleanup is automatic.

###### Examples

**Example 10.4.5.6 (Drop for resource cleanup):**

```cursive
record Connection {
    address: string@Managed,
    socket: i32,
}

behavior Drop for Connection {
    procedure drop(~!)
        [[ net::close |- true => true ]]
    {
        net::close_socket(self.socket)
        // `address` automatically destroyed after drop() completes
    }
}
```

##### §9.4.5.5 Marker Behavior Summary

[38] Table 10.4.2 summarizes marker behavior properties:

| Behavior | Purpose               | Auto-Derived           | Has Procedures | Can Override |
| -------- | --------------------- | ---------------------- | -------------- | ------------ |
| `Copy`   | Bitwise copyable      | Yes (structural)       | No             | No           |
| `Sized`  | Statically known size | Yes (default for most) | No             | No           |
| `Drop`   | Custom destructor     | No (manual only)       | Yes (`drop`)   | Yes          |

[39] Marker behaviors with no procedures (Copy, Sized) are pure type-level properties with no runtime representation or procedure dispatch. Drop has a procedure but is considered a marker because it is automatically invoked by the runtime.

#### §9.4.6 Behavior Items [generic.behavior.items]

##### §9.4.6.1 Procedure Declarations

[60] Behavior procedures declare:

- Receiver parameter (`~`, `~%`, `~!`, or explicit `self: perm Self`)
- Additional parameters
- Return type
- Contractual sequent
- Procedure body (mandatory)

[61] All behavior procedures must include bodies. The body provides the default implementation available to all types attaching the behavior.

##### §9.4.6.2 Receiver Permissions

[62] Behavior procedures may use any receiver permission:

- `~` (`const Self`): Read-only access
- `~%` (`shared Self`): Shared mutable access
- `~!` (`unique Self`): Exclusive mutable access

[63] The receiver permission determines what operations the procedure may perform on `self` and constrains which bindings may invoke the procedure.

##### §9.4.6.3 Procedure Bodies and Self

[64] Within procedure bodies, `self` refers to the receiver and has type `perm Self` where `perm` matches the receiver permission. Operations on `self` are constrained by the permission:

```cursive
behavior Resettable {
    procedure reset(~!)
    {
        self = Self::default()  // OK: unique permission allows assignment
    }
}
```

##### §9.4.6.4 Examples

**Example 10.4.6.1 (Behavior with multiple procedures):**

```cursive
behavior Display {
    procedure show(~): string@View
    {
        result type_name::<Self>()  // Default: use type name
    }

    procedure show_verbose(~): string@Managed
        [[ alloc::heap |- true => true ]]
    {
        result self.show().to_managed()
    }
}
```

**Example 10.4.6.2 (Behavior procedures calling each other):**

```cursive
behavior Stringifiable {
    procedure to_string(~): string@Managed
        [[ alloc::heap |- true => true ]]
    {
        result string.from(self.to_str())
    }

    procedure to_str(~): string@View
    {
        result "Stringifiable"
    }
}
```

#### §9.4.7 Integration with Witnesses [generic.behavior.witness]

[66] Behaviors participate in the witness system for runtime polymorphism (Clause 13). A witness packages a value and behavior implementation for dynamic dispatch:

```cursive
// Forward reference to Clause 13
let drawable: witness<Display> = witness::heap(shape)
drawable.show()  // Dynamic dispatch to shape's show() implementation
```

[67] Witness construction, dispatch semantics, storage strategies, and modal witness integration are fully specified in Clause 13 [witness]. This clause establishes only that behaviors provide the compile-time structure that witnesses reify at runtime.

#### §9.4.8 Diagnostics [generic.behavior.diagnostics]

[68] Behavior declaration diagnostics:

| Code    | Condition                                                       |
| ------- | --------------------------------------------------------------- |
| E10-401 | Behavior procedure declared without body                        |
| E10-402 | Cyclic behavior extension (behavior extends itself)             |
| E10-403 | Associated type does not satisfy bound                          |
| E10-404 | Behavior extends non-behavior (e.g., contract)                  |
| E10-405 | Behavior name collision with existing binding                   |
| E10-406 | Duplicate associated type name in behavior                      |
| E10-407 | Behavior procedure has invalid receiver                         |
| E10-411 | Type claims Copy but does not satisfy structural requirements   |
| E10-412 | Type implements both Copy and Drop (mutual exclusion violation) |
| E10-413 | Conflicting procedures from extended behaviors                  |

#### §9.4.9 Examples [generic.behavior.examples]

**Example 10.4.9.1 (Complete behavior declaration):**

```cursive
behavior Drawable {
    type Canvas = DefaultCanvas

    procedure draw(~, canvas: Self::Canvas)
        [[ io::write |- true => true ]]
    {
        println("Drawing on canvas")
    }

    procedure draw_at(~, canvas: Self::Canvas, x: i32, y: i32)
        [[ io::write |- true => true ]]
    {
        println("Drawing at ({}, {})", x, y)
        self.draw(canvas)
    }
}
```

**Example 10.4.9.2 (Behavior extending another behavior):**

```cursive
behavior Display {
    procedure show(~): string@View
    {
        result "Display"
    }
}

behavior Debug with Display {
    procedure debug(~): string@View
    {
        result "Debug: " ++ self.show()
    }

    // Inherits show() from Display
    // Types may override show() or use inherited default
}
```

**Example 10.4.9.3 (Generic behavior):**

```cursive
behavior Converter<U> {
    procedure convert(~): U
    {
        panic("Converter::convert not implemented for this type")
    }
}

record IntWrapper with Converter<string@Managed> {
    value: i32,

    procedure convert(~): string@Managed
        [[ alloc::heap |- true => true ]]
    {
        result string.from_int(self.value)
    }
}
```

**Example 10.4.9.4 - invalid (Behavior with body-less procedure):**

```cursive
behavior Invalid {
    procedure abstract_method(~): i32
    // error[E10-401]: behavior procedure must have body
}
```

**Example 10.4.9.5 (Marker behaviors on type):**

```cursive
record Token with Copy {
    id: u64,
    timestamp: i64,
}

// Marker behaviors satisfied:
// - Copy: all fields are Copy (explicit attachment)
// - Sized: has known size (implicit for all non-slice types)
//
// Thread safety via permissions (not marker behaviors):
// - const Token: safe to share across threads (immutable)
// - unique Token: safe to transfer via move (exclusive ownership)
```

#### §9.4.10 Conformance Requirements [generic.behavior.requirements]

[69] Implementations shall:

1. Support behavior declarations with visibility modifiers per §5.6
2. Require all behavior procedures to have concrete bodies (E10-401)
3. Support behavior extension with `with` clause
4. Detect and reject cyclic behavior extensions (E10-402)
5. Support associated types in behaviors with optional defaults
6. Validate associated type bounds at implementation site (E10-403)
7. Support generic behaviors with type, const, and grant parameters
8. Provide `Self` type binding within behavior bodies
9. Implement marker behaviors (Copy, Sized, Drop) per §10.4.5
10. Auto-derive Copy and Sized when structural requirements met
11. Validate explicit marker attachments match structural requirements (E10-411)
12. Enforce Copy/Drop mutual exclusion (E10-412)
13. Not provide Send/Sync marker behaviors (thread safety via permissions, Clause 14)
14. Integrate behaviors with witness system (forward to Clause 13)
15. Emit structured diagnostics for behavior violations

---

**Previous**: §10.3 Bounds and Where-Constraints (§10.3 [generic.bounds]) | **Next**: §10.5 Behavior Implementations and Coherence (§10.5 [generic.implementation])
