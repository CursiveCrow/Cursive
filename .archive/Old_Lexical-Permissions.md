# The Cursive Language Specification

**Part**: IV — Lexical Permission System  
**File**: 04_Lexical-Permissions.md  
**Previous**: [Declarations and Scope](03_Declarations-and-Scope.md) | **Next**: [Expressions and Operators](05_Expressions-and-Operators.md)

---

## Abstract

The Lexical Permission System (LPS) is the compile-time framework that delivers Cursive’s ownership discipline without a borrow checker. Part IV is the single source of truth for permission wrappers, ownership transfer, permission-aware typing, and the cross-cutting invariants that bind declarations, expressions, statements, effects, and regions together. Wherever other parts of the specification provide grammar or operational rules, this part supplies the normative permission model those rules rely upon.
This chapter depends on Part VIII (Regions) for runtime semantics of region stacks and deallocation.

**Definition 4.1 (Lexical Permission System):** The LPS comprises

1. the permission universe {`owned`, `unique`, `shared`, `readonly`} and their attachment to types,
2. the typing and coercion judgments that track permissions,
3. the ownership transfer semantics for moves and copies,
4. the interaction between permissions, control-flow constructs, data structures, and regions, and
5. the diagnostic catalogue that detects LPS violations.

### 4.0 Scope and Authority

- **Authoritative coverage** – This part governs permission wrappers, aliasing guarantees, ownership transfer, permission-directed typing judgments, and the interoperability of permissions with effects, regions, and diagnostics. It supersedes incidental descriptions scattered across earlier drafts.
- **Delegated authority** – Grammar for bindings, parameters, and expressions remains in Part III and Part V. Statement execution rules continue to live in Part VI. Where a referenced chapter is deferred, the rules stated here are normative and MUST be implemented until superseded.
- **Cross-part references** – Citations follow the form “Part N §x.y”. When a referenced part is not yet published, this chapter restates the applicable constraint explicitly so that implementers require no external draft.

---

## 4.1 Conceptual Overview

### 4.1.1 Motivating Goals

1. **Memory safety without a borrow checker** – Permissions encode aliasing constraints directly in type syntax, yielding deterministic reasoning and avoiding lifetime parameters.
2. **Explicit aliasing discipline** – All mutable or owning access is spelled out at call sites and bindings; the compiler never infers hidden aliasing transformations.
3. **Predictable AI generation** – Required annotations (e.g., pipeline stage types, loop iterator types, block `result`) ensure that models and humans can reason locally about permissions and effects.

### 4.1.2 Glossary of Core Terms (Normative)

| Term | Definition |
|------|------------|
| **Permission wrapper** | A type of the form `owned T`, `unique T`, `shared T`, or `readonly T`, denoting ownership, exclusive mutable access, shared mutable access, or immutable access to a value of type `T`. |
| **Place** | A value category representing a memory location paired with its permission (Part V §5.0.4). |
| **Ownership transfer** | An operation that transfers `owned` permission from one binding to another, invalidating the source binding. Occurs when passing to `owned` parameter, assignment, or return. |
| **Use-after-transfer** | Any attempted read or write of a binding invalidated by ownership transfer; diagnosed as E4006. |
| **Permission coercion** | Implicit conversion from a stronger permission to a weaker one (`owned → unique → shared → readonly`) or explicit operations for the reverse direction. |
| **Permission judgment** | A typing form `Γ ⊢ e : perm τ` recording that expression `e` is accessed through permission `perm`. |
| **Permission lattice** | The partial order `owned ≥ unique ≥ shared ≥ readonly`, defining allowable coercions and implicit conversions. |
| **Region value** | A value allocated inside a lexical region (`region r { ... }`) that must not escape past the region boundary. |

---

## 4.2 Permission Universe

### 4.2.1 Syntax and Formation

Every type constructor `T` admits four permission wrappers: `owned T`, `unique T`, `shared T`, and `readonly T`. The canonical grammar is established in Part II §2.0.5.1; this section supplies the semantics.

```
[Perm-Owned]
Γ ⊢ T : Type
----------------------
Γ ⊢ owned T : Type

[Perm-Unique]
Γ ⊢ T : Type
----------------------
Γ ⊢ unique T : Type

[Perm-Shared]
Γ ⊢ T : Type
----------------------
Γ ⊢ shared T : Type

[Perm-Readonly]
Γ ⊢ T : Type
----------------------
Γ ⊢ readonly T : Type
```

**Context-sensitive defaults:**
- **Bindings** (`let`, `var`): Bare type `T` means `owned T` (implied)
- **Parameters**: Bare type `T` means `readonly T` (implied)
- **Returns**: Bare type `T` means `readonly T` (implied)
- **Fields**: Bare type `T` means `owned T` (implied, and ONLY `owned` is valid)


### 4.2.2 Guarantees and Responsibilities

| Permission | Guarantees | Responsibilities |
|------------|------------|------------------|
| `owned T` | Exactly one owning access exists; exclusive mutable access; can transfer ownership; destruction drops the value; implicit coercion to `unique T`/`shared T`/`readonly T` permitted. | Caller must not use binding after ownership transfer; must release resources (RAII). |
| `unique T` | Exclusive mutable access (no other `unique` or `shared` aliases); cannot transfer ownership (temporary view only); compiler-verified exclusivity; coerces to `readonly T`. | Caller must not create `unique` or `shared` aliases while this binding is active. |
| `shared T` | Non-exclusive mutable access; multiple `shared` references may coexist; cannot transfer ownership; **no exclusivity guarantee** (explicit opt-in to aliasing). | Programmer must manually coordinate access across aliases to avoid data races and maintain invariants. |
| `readonly T` | Immutable read-only access; unlimited sharing; coercible from any permission. | No mutation through this binding; operations requiring mutation must obtain `unique`, `shared`, or `owned`. |

**Design rationale:** The four-tier permission system provides graduated control over aliasing:
- `owned` enables resource management through ownership transfer and RAII
- `unique` prevents aliasing bugs while allowing mutation (compiler-verified safety)
- `shared` explicitly opts into aliasing when coordination is necessary (type-documented flexibility)
- `readonly` allows unrestricted sharing for immutable access (safe default)

Permissions are part of the type identity. Subtyping and equivalence rules are defined in Part II §2.0.6.1; this chapter provides the coercion lattice in §4.6.

### 4.2.3 Wrapper Nesting

The grammar permits at most one permission wrapper per type. Any attempt to wrap an already wrapped type (for example `own mut T`) is ill-formed and rejected with E4003.

---

## 4.3 Binding Mutability vs Value Mutability

1. **Binding mutability** is controlled solely by the declaration keyword (`let` or `var`) and never inferred (Part III §§3.2.1–3.2.4).
2. **Value mutability** is governed by permission wrappers. A `let` binding can wrap a `mut T` and permit in-place mutation without rebinding; a `var` binding can rebind itself.
3. **Diagnostics** – Assignment to a `let` binding (regardless of permission) triggers E3D10. Attempting to mutate a value through an `imm` permission triggers E4003 (§4.5.3).

**Rule 4.3.1 (Single-Binding):** A name may be introduced only once per scope unless shadowed explicitly (Part III §3.2.2). Shadowing does not change the permission of the outer binding; it introduces a new binding with its own permissions.

---

## 4.4 Permission Wrappers in the Type System

### 4.4.1 Well-Formedness and Variance

- Permission wrappers are invariant in their payload type unless the underlying constructor states otherwise (Part II §2.0.6.1). For example, `own Vec<T>` is invariant in `T`.
- Composite types carry permissions component-wise: a record field declared as `own T` inherits owning semantics when the record is owned; a slice `[T]` is always a view and thus `imm` unless wrapped explicitly.

### 4.4.2 Interaction with Products, Sums, and Modals

- Product types (tuples, records) wrap each field with the same permission as the aggregate. An `own` record owns its fields; a `mut` record grants mutable access to its fields.
- Sum types (enums) apply the wrapper to the entire payload. An `own` enum variant owns whichever payload is active.
- Modal types treat permissions as orthogonal to state; state transitions consume and produce permissions according to modal procedure signatures (Part II §2.3.3).

### 4.4.3 Type Formation Examples

```cursive
// Record owning its resources
let point: own Point = Point { x: 0.0, y: 0.0 }

// Mutable view into owned record
procedure translate(p: mut Point, dx: f64, dy: f64) {
    p.x += dx
    p.y += dy
}
```

---

## 4.5 Permission Judgments

### 4.5.1 Judgement Forms

```
Γ ⊢ e : perm τ
```

denotes that expression `e` is viewed with permission `perm`. The following rules are normative:

```
[Perm-Var]
(x : perm τ) ∈ Γ
----------------------
Γ ⊢ x : perm τ

[Perm-Field]
Γ ⊢ e : perm R
field f : U in R
----------------------
Γ ⊢ e.f : perm U

[Perm-Index]
Γ ⊢ a : perm [U]
Γ ⊢ i : usize
----------------------
Γ ⊢ a[i] : perm U

[Perm-Deref-Raw]
Γ ⊢ p : perm (* τ)
----------------------
Γ ⊢ *p : imm τ

[Perm-Deref-RawMut]
Γ ⊢ p : perm (*mut τ)
----------------------
Γ ⊢ *p : mut τ

[Perm-Deref-Safe]
Γ ⊢ p : perm Ptr<τ>
----------------------
Γ ⊢ *p : perm τ

[Perm-Deref-Safe-State]
Γ ⊢ p : perm Ptr<τ>@State
------------------------
Γ ⊢ *p : perm τ
```

Raw pointer dereference requires the `unsafe.ptr` effect (Part V §4.4.3). Safe pointers follow the managed-pointer rules in Part II §2.5.

### 4.5.2 Assignment and Update

**Rule 4.5.2 (Assignment Permission):** The target of an assignment must be an lvalue with permission `mut` or `own` (Part VI §6.4.2). Violation produces E4003.

```
[Perm-Assign]
Γ ⊢ target : perm τ
perm ∈ {mut, own}
Γ ⊢ rhs : τ
----------------------
Γ ⊢ (target = rhs) : ()

[Perm-Assign-Compound]
Γ ⊢ target : perm τ
perm ∈ {mut, own}
Γ ⊢ rhs : τ
operator ⊕ valid on τ
--------------------------------
Γ ⊢ (target ⊕= rhs) : ()
```

### 4.5.3 Procedure Dispatch

Techniques:
- Procedure signatures declare their required `self` permission (`self: T`, `self: mut T`, or `self: own T`). Call sites must supply a receiver of at least that strength (Part V §4.3.2).
- If the receiver is a `let` binding, only procedures requiring `imm` are available unless the binding wraps a `mut` or `own` permission.

```
[Perm-Procedure]
Γ ⊢ receiver : perm₀ T
procedure m : (self: perm₁ T, τ₁, …, τₙ) → τᵣ ! ε
Γ ⊢ aᵢ : τᵢ  (∀i)
perm₀ ≥ perm₁
----------------------------------------
Γ ⊢ receiver::m(a₁, …, aₙ) : τᵣ
```

### 4.5.4 Indexing and Slicing

Array and slice indexing follow the same permission as the aggregate. Mutable indexing requires the aggregate to be `mut`. Attempting to mutate through `imm` slices raises E4003 (Part V §4.3.4).

---

## 4.6 Permission Coercions

### 4.6.1 Implicit Lattice

```
owned T  →  unique T  →  shared T  →  readonly T
```

Implicit coercions are allowed only in the direction of the arrows. The compiler inserts them automatically at call sites, assignments, and pattern matches where a weaker permission is expected.

```
[Coerce-Owned-Unique]
Γ ⊢ e : owned τ
----------------------
Γ ⊢ e : unique τ

[Coerce-Owned-Shared]
Γ ⊢ e : owned τ
----------------------
Γ ⊢ e : shared τ

[Coerce-Owned-Readonly]
Γ ⊢ e : owned τ
----------------------
Γ ⊢ e : readonly τ

[Coerce-Unique-Readonly]
Γ ⊢ e : unique τ
----------------------
Γ ⊢ e : readonly τ

[Coerce-Shared-Readonly]
Γ ⊢ e : shared τ
----------------------
Γ ⊢ e : readonly τ
```

**Note:** `unique` → `shared` is **NOT** allowed implicitly, as it would violate the exclusivity guarantee of `unique`.

### 4.6.2 Explicit Reversal

The reverse direction (`readonly` → `unique`/`shared`/`owned`) requires explicit operations:

- **Constructors or cloning** – Creating a new owned value from immutable data (`string.to_owned()`).
- **Ownership transfer** – Receiving ownership from an `owned` binding (via function parameter, return, or assignment).
- **Unsafe operations** – `unsafe` code may reinterpret handles but MUST re-establish invariants before returning a value.

There is no implicit upgrade from weaker to stronger permissions. Any code that needs a stronger permission MUST:
1. Receive it from an existing `owned` binding (via coercion), OR
2. Construct/clone a fresh value with the desired permission

### 4.6.3 Context-Sensitive Defaults

Permission annotations may be omitted in certain contexts, with the following defaults:

| Context | Omitted Permission | Default Meaning |
|---------|-------------------|-----------------| | `let x = expr` | Always `owned` | Binding owns the value |
| `var x = expr` | Always `owned` | Binding owns the value |
| `f(x: T)` | `readonly T` | Parameter is readonly (safe default) |
| `f(): T` | `readonly T` | Return is readonly (safe default) |
| `record R { f: T }` | `owned T` | Field owns its data (only valid permission for fields) |

**Explicit annotations required for:**
- Parameters needing `owned` (ownership transfer): `f(x: owned T)`
- Parameters needing `unique` (exclusive mutation): `f(x: unique T)`
- Parameters needing `shared` (coordinated mutation): `f(x: shared T)`
- Returns transferring ownership: `f(): owned T`

**Field restrictions:**
Record fields MUST be `owned` (implied by default). Other permissions are invalid:
```cursive
record Invalid {
    data: unique Data,    // ERROR E4022: fields cannot be 'unique'
    ref: readonly Data,   // ERROR E4022: fields cannot be 'readonly'
    cache: shared Cache   // ERROR E4022: fields cannot be 'shared'
}
```

### 4.6.4 Ownership Transfer Semantics

Ownership transfer occurs automatically (no explicit `transfer` keyword) when:

1. **Passing to `owned` parameter:**
   ```cursive
   function consume(f: owned File) { ... }
   let file = File::open("data.txt")
   consume(file)  // Ownership automatically transferred
   // file is now INVALID
   ```

2. **Assignment between bindings:**
   ```cursive
   let file1 = File::open("a.txt")
   let file2 = file1  // Ownership transferred
   // file1 is now INVALID
   ```

3. **Returning `owned` from function:**
   ```cursive
   function create(): owned File {
       let f = File::open("data.txt")
       result f  // Ownership transferred to caller
   }
   ```

4. **Constructing record with `owned` fields:**
   ```cursive
   record Container {
       file: File  // Owned (implied)
   }
   let f = File::open("data.txt")
   let c = Container { file: f }  // f becomes INVALID
   ```

**No transfer occurs for other permissions:**
- `readonly` parameters/returns: Caller retains ownership
- `unique` parameters: Caller retains ownership (exclusive view only)
- `shared` parameters: Caller retains ownership (shared view only)

### 4.6.5 Permission Checking and Diagnostics

#### Unique Exclusivity Checking

The compiler MUST enforce that `unique` permissions are exclusive:

```
[Unique-Exclusive]
Γ ⊢ x : unique τ active in scope S
Γ ⊢ attempt to create y : unique τ from x in S
Γ ⊢ attempt to create y : shared τ from x in S
───────────────────────────────────────────────
ERROR E4020 (unique permission violation)
```

**Algorithm**: For each scope, track active `unique` bindings. When a new `unique` or `shared` binding is created from the same source, emit E4020.

#### Ownership Transfer Tracking

The compiler MUST track ownership transfer:

```
[Transfer-Tracking]
Γ ⊢ x : owned τ
x passed to owned parameter / assigned to owned target / returned as owned
───────────────────────────────────────────────
x marked as INVALID in Γ

[Use-After-Transfer]
Γ ⊢ x : INVALID
Γ ⊢ use of x
───────────────────────────────────────────────
ERROR E4006 (use after transfer)
```

#### Field Permission Validation

```
[Field-Permission-Check]
Γ ⊢ record R { f: perm T }
perm ∉ {owned, (omitted)}
───────────────────────────────────────────────
ERROR E4022 (invalid field permission)
```

Fields MUST be `owned` (or omit annotation, which defaults to `owned`).


## 4.7 Ownership Transfer and Move Semantics

### 4.7.1 Move Rules

```
[Move-Expr]
Γ ⊢ e : own τ
----------------------
Γ ⊢ move e : own τ
```

After `move e`, any binding providing the original `own` permission becomes invalid (E4006).

### 4.7.2 Implicit Moves

1. Passing an `own` argument to a parameter declared `own` moves the value.
2. Assigning from an `own` lvalue to another `own` lvalue moves the value unless `.copy()` is invoked on a type implementing `Copy` (Part II §2.0.9).

### 4.7.3 Decision Procedure

**Algorithm 4.7.1 (Move/Copy Decision).**

Inputs: source binding `b : perm₀ τ`, destination requirement `perm_dest`, operation kind (`move`, `assign`, `call`).

1. **Check copy capability.**
   - If `τ : Copy`, reading `b` produces an immutable view; an explicit duplicate MUST call `b.copy()` and record any effects declared on `Copy::copy`.
   - If `τ ∉ Copy`, proceed to step 2.
2. **Compute required permission.**
   - If `perm_dest = own`, require `perm₀ = own`.
   - If `perm_dest = mut`, require `perm₀ ∈ {own, mut}`.
   - If `perm_dest = imm`, any permission suffices.
   If no requirement is met, emit E4003.
3. **Detect transfer.**
   - If the operation is `move` or implicitly consumes an `own` parameter, mark binding `b` as moved.
   - Otherwise, record the permission view (`mut` or `imm`) held for the duration of the operation.
4. **Enforce post-move usage.**
   - If `b` is marked moved and later appears in the CFG, emit E4006.
   - When a permission view ends after the operation, release it (no diagnostics).
5. **Validate region lifetime.**
   - If `b` refers to a region value allocated in `r` and the destination outlives `r`, emit E4014 (or E3D09 when escaping via binding).
6. **Insert coercion.**
   - Apply the weakest implicit coercion from §4.6 that satisfies `perm_dest`.
   - If no implicit coercion exists (e.g., `imm` → `mut`), require explicit code (constructor, `to_owned`, or move into a new owner).

Outputs: updated binding states and the permission supplied to the destination.

### 4.7.4 Move Semantics in Collections

Collections like `Vec<T>` and arrays exhibit ownership semantics that depend on whether the element type is `Copy`.

#### Owned Vectors

```cursive
function create_vector(): own Vec<i32>
    uses alloc.heap
{
    let vec: own Vec<i32> = Vec::new()
    vec::push(1)
    vec::push(2)
    vec::push(3)
    result vec
}

function consume_vector(vec: own Vec<i32>): i32
    uses alloc.heap
{
    let sum = 0
    loop item in vec {
        sum = sum + item
    }
    result sum
}

function move_vector_example()
    uses alloc.heap
{
    let vec = create_vector()
    let total = consume_vector(move vec)
    // vec is now invalid - moved
    println("Total: {}", total)
}
```

#### Permission Coercion with Vectors

```cursive
function read_vector(vec: imm Vec<i32>): i32 {
    result vec::len() as i32
}

function modify_vector(vec: mut Vec<i32>)
    uses alloc.heap
{
    vec::push(42)
}

function vector_permission_example()
    uses alloc.heap
{
    let vec: own Vec<i32> = Vec::new()

    // Coerce own → imm for read
    let len = read_vector(vec)

    // Coerce own → mut for modification
    modify_vector(mut vec)

    // vec still owned
    let final_len = vec::len()
}
```

#### Moving from Collections

```cursive
record Data {
    value: i32,
    name: string
}

function extract_first(vec: own Vec<Data>): Option<Data>
    uses alloc.heap
{
    if vec::len() > 0 {
        result Option::Some(vec::remove(0))
    } else {
        result Option::None
    }
}
```

### 4.7.5 Partial Moves and Aggregates

Partial moves extract individual fields from a structure, invalidating the original binding.

#### Partial Move from Record

```cursive
record Container {
    data: Vec<i32>,
    metadata: string,
    count: i32
}

function extract_data_field(c: own Container): own Vec<i32>
    uses alloc.heap
{
    result move c.data
    // c is now partially moved
    // c.metadata and c.count are dropped
}

function partial_move_example()
    uses alloc.heap
{
    let container = Container {
        data: Vec::from([1, 2, 3]),
        metadata: string::from("example"),
        count: 3
    }

    let extracted = move container.data

    // ERROR: cannot use container as whole
    // process(container)

    // Can still access Copy fields
    println("Count was: {}", container.count)
}
```

#### Partial Moves in Tuples

```cursive
function tuple_partial_move()
    uses alloc.heap
{
    let tuple: (Vec<i32>, string, i32) = (
        Vec::from([1, 2]),
        string::from("hello"),
        42
    )

    let vec = move tuple.0
    // tuple.1 and tuple.2 dropped

    // Can still access Copy fields before move
    println("Third element: {}", tuple.2)
}
```

#### Enum Partial Moves

```cursive
enum Container {
    Single(i32),
    Pair(string, Vec<i32>),
    Triple { x: i32, y: string, z: Vec<i32> }
}

function extract_from_enum(c: own Container): Option<Vec<i32>>
    uses alloc.heap
{
    c match {
        Container::Pair(s, vec) => {
            result Option::Some(vec)
        },
        Container::Triple { x, y, z } => {
            result Option::Some(z)
        },
        _ => {
            result Option::None
        }
    }
}
```

### 4.7.6 Move in Pattern Matching

Pattern matching can move values out of the scrutinee.

#### Basic Pattern Moves

```cursive
enum Message {
    Text(string),
    Number(i32),
    Data(Vec<u8>)
}

function process_message(msg: own Message): string
    uses alloc.heap
{
    msg match {
        Message::Text(s) => {
            // s is owned, moved from msg
            result s
        },
        Message::Number(n) => {
            result string::from(n)
        },
        Message::Data(d) => {
            result string::from("Data with {} bytes", d::len())
        }
    }
}
```

#### Pattern Matching with Permission Coercion

```cursive
function examine_message(msg: imm Message): i32 {
    msg match {
        Message::Text(s) => {
            // s has imm permission (coerced from msg)
            result s::len() as i32
        },
        Message::Number(n) => {
            result n
        },
        Message::Data(d) => {
            result d::len() as i32
        }
    }
}
```

#### Nested Pattern Moves

```cursive
enum Outer {
    Inner(Option<Vec<i32>>),
    Empty
}

function nested_pattern_move(outer: own Outer): Option<Vec<i32>>
    uses alloc.heap
{
    outer match {
        Outer::Inner(Option::Some(vec)) => {
            // vec moved out of nested structure
            result Option::Some(vec)
        },
        _ => {
            result Option::None
        }
    }
}
```

### 4.7.7 Common Move Patterns

#### Builder Pattern with Ownership Transfer

```cursive
record Builder {
    name: string,
    value: i32
}

function Builder::new(): own Builder
    uses alloc.heap
{
    result Builder {
        name: string::from("default"),
        value: 0
    }
}

function Builder::with_name(self: own Builder, name: string): own Builder {
    result Builder {
        name: name,
        value: self.value
    }
}

function Builder::with_value(self: own Builder, value: i32): own Builder {
    result Builder {
        name: self.name,
        value: value
    }
}

function builder_pattern_example()
    uses alloc.heap
{
    let builder = Builder::new()
        => (|b| -> b::with_name(string::from("example")))
        => (|b| -> b::with_value(42))

    println("Built: {}, {}", builder.name, builder.value)
}
```

#### Swap Pattern

```cursive
function swap<T>(a: own T, b: own T): (own T, own T) {
    result (b, a)
}

function swap_example()
    uses alloc.heap
{
    let x = string::from("first")
    let y = string::from("second")

    let (new_x, new_y) = swap(move x, move y)

    println("x is now: {}", new_x)
    println("y is now: {}", new_y)
}
```

#### Resource Transfer Pattern

```cursive
record File {
    handle: i32,
    path: string
}

function File::open(path: string): own File
    uses fs.open, alloc.heap
{
    let handle = fs::open_file(path)
    result File { handle, path }
}

procedure File::close(self: own File)
    uses fs.close
{
    fs::close_file(self.handle)
}

function transfer_ownership(file: own File): own File {
    // Ownership transferred through function
    result file
}

function resource_pattern()
    uses fs.open, fs.close, alloc.heap
{
    let file = File::open(string::from("/tmp/test.txt"))
    let file2 = transfer_ownership(move file)
    file2::close()
}
```

## 4.8 Places and Value Categories

### 4.8.1 Lvalues, Rvalues, and Places

Part V §5.0.4 defines the categories. This chapter enforces the following:

- Only lvalues with `mut` or `own` permissions may appear on the left-hand side of an assignment.
- A place inherits the strongest permission offered by its provenance (e.g., a field of a `mut` record is `mut` unless the field is wrapped explicitly).

### 4.8.2 Composition Rules

1. **Field access** – `perm(R).f` yields `perm` applied to the field type.
2. **Tuple projection** – Maintains the permission of the tuple.
3. **Array indexing** – Requires the array to be at least `mut` for mutation.
4. **Pointer dereference** – `*p` inherits the permission implied by the pointer type (`*mut T` grants `mut`, `*T` is `imm`).

---

## 4.9 Parameters and Self Bindings

### 4.9.1 Default Semantics

- Parameters without an explicit wrapper default to `imm` (Part III §3.5.2).
- `self`/`$` adopts the permission declared in the procedure signature (Part V §4.3.2.3).

### 4.9.2 Conversion at Call Sites

- Arguments of type `own` can satisfy parameters of type `mut` or `imm` via implicit coercion.
- Passing an argument to an `own` parameter consumes an `own` binding via move; supplying only a `mut` handle is a static error.
- The compiler rejects mismatched permissions with E4003 when coercion cannot satisfy the callee.

```cursive
procedure read_only(value: string) { }
procedure mutate(value: mut string) { }
procedure consume(value: own string) { }

let text: own string = string.from("hi")
read_only(text)          // Implicit own → imm, `text` still owned
mutate(mut text)         // Explicit mut view, `text` still owned
consume(move text)       // Ownership transferred; `text` invalid afterward
```

### 4.9.3 Variadic Effects and Overloads

When a procedure is effect-polymorphic, the effect variable must include any implicit coercions that allocate or copy to adjust permissions. Effect checking remains in Part VII (deferred); until that chapter is complete, this part requires effect-polymorphic procedures to document the propagated permission changes in prose.

---

## 4.10 Closures and Captures

### 4.10.1 Classification

| Capture Form | Requirement | Post-capture Availability |
|--------------|-------------|---------------------------|
| Owned capture (`own`) | Captured value is moved into closure; binding outside becomes invalid. | Closure exclusively owns the value. |
| Mutable capture (`mut`) | Closure must not escape the region or scope that owns the captured mutable access. | Source binding remains usable; closure and binding share mutable access. |
| Immutable capture (`imm`) | No restrictions; read-only sharing. | Source binding remains usable. |

### 4.10.2 Region Escape Rules

```
[Closure-Region-Escape]
Γ; (Δ · r) ⊢ x : own τ allocated in r
closure escapes r
----------------------------------------
ERROR E4015
```

Closures that capture region-allocated values by `own` or `mut` must not outlive the region unless Part VIII introduces explicit escape-analysis exemptions. For example:

```cursive
region temp {
    let buffer = alloc_in<temp>(Buffer.new())
    let f = || {
        result move buffer
    }
    // ERROR E4015: closure `f` escapes `temp` while owning `buffer`
}
```

### 4.10.3 Effect Accounting

**Algorithm 4.10.1 (Closure Effect Aggregation).**

Inputs: capture environment `env = {x₁, …, xₖ}`, closure body `body`.

1. Compute `ε_body ← effects(body)`.
2. For each captured value `xᵢ`:
   - If `xᵢ` is owned and its destructor requires `ε_dtor`, set `ε_body ← ε_body ∪ ε_dtor`.
   - If `xᵢ` is mutable and the closure mutates it, include the effects of those mutations.
   - If `xᵢ` is immutable and pure, no additional effects are introduced.
3. Return `ε_total = ε_body`.  
The closure type MUST declare `uses ε_total`. Calls made in a context whose effect set does not cover `ε_total` are rejected with E4004.




### 4.10.4 Capture Pattern Examples

Closures can capture values with different permissions, affecting both the closure's capabilities and the outer scope's continued access.

#### Immutable Captures

```cursive
function immutable_capture_example()
    uses alloc.heap
{
    let data: own Vec<i32> = Vec::from([1, 2, 3, 4, 5])

    let processor = || -> {
        let sum = 0
        loop item in data {
            sum = sum + item
        }
        result sum
    }

    // data is captured with imm permission
    // Original binding retains ownership
    let result = processor()
    println("Sum: {}, Length: {}", result, data::len())
}
```

#### Mutable Captures

```cursive
function mutable_capture_example()
    uses alloc.heap
{
    var counter: mut i32 = 0

    let incrementer = || -> {
        counter = counter + 1
        result counter
    }

    let val1 = incrementer()
    let val2 = incrementer()

    println("Counter: {}", counter)
}
```

#### Owned Captures

```cursive
function owned_capture_example()
    uses alloc.heap
{
    let data: own Vec<i32> = Vec::from([1, 2, 3])

    let processor = move || -> {
        let sum = 0
        loop item in data {
            sum = sum + item
        }
        result sum
    }

    // ERROR: data has been moved into closure
    // println("Length: {}", data::len())

    let result = processor()
    println("Sum: {}", result)
}
```

#### Mixed Permission Captures

```cursive
function mixed_capture_example()
    uses alloc.heap
{
    let config: imm string = string::from("config")
    var count: mut i32 = 0
    let data: own Vec<i32> = Vec::from([1, 2, 3])

    let processor = move || -> {
        // config: captured imm
        // count: captured mut
        // data: moved (owned)
        count = count + 1
        let sum = 0
        loop item in data {
            sum = sum + item
        }
        println("Config: {}, Count: {}", config, count)
        result sum
    }

    // config still accessible
    // count still accessible
    // data is moved

    let result = processor()
}
```

### 4.10.5 Closure Composition

Closures can be composed, returned, and passed as parameters to create higher-order abstractions.

#### Returning Closures

```cursive
function create_adder(n: i32): (i32) → i32 {
    move |x| -> x + n
}

function closure_factory_example() {
    let add_five = create_adder(5)
    let result = add_five(10)
    println("Result: {}", result)
}
```

#### Closures as Parameters

```cursive
function apply<T, U>(value: T, f: (T) → U) → U {
    result f(value)
}

function closure_parameter_example() {
    let doubled = apply(5, |x| -> x * 2)
    println("Doubled: {}", doubled)
}
```

#### Closure Chaining

```cursive
function chain_example()
    uses alloc.heap
{
    let numbers = [1, 2, 3, 4, 5]

    let result = numbers
        => (|ns| -> {
            let sum = 0
            loop n in ns {
                sum = sum + n
            }
            result sum
        })
        => (|sum| -> sum * 2)

    println("Result: {}", result)
}
```

#### Higher-Order Function Composition

```cursive
function compose<A, B, C>(
    f: (B) → C,
    g: (A) → B
): (A) → C {
    move |x| -> f(g(x))
}

function composition_example() {
    let add_one = |x: i32| -> x + 1
    let double = |x: i32| -> x * 2

    let add_then_double = compose(double, add_one)

    let result = add_then_double(5)
    println("Result: {}", result)
}
```

### 4.10.6 Closure Lifetime Scenarios

Closures capturing values from outer scopes must respect region and permission constraints.

#### Region-Constrained Closures

```cursive
function region_closure_example()
    uses alloc.region, alloc.heap
{
    let outer_data = Vec::from([1, 2, 3])

    region {
        let region_data = string::from_region("temporary")

        // ERROR: closure would capture region_data beyond region
        // let bad_closure = || -> region_data::len()

        // OK: closure used within region
        let good_closure = || -> region_data::len()
        let len = good_closure()
        println("Length: {}", len)
    }

    // outer_data still accessible
    println("Outer: {}", outer_data::len())
}
```

#### Closure with Permission Constraints

```cursive
function permission_lifetime_example()
    uses alloc.heap
{
    let data: own Vec<i32> = Vec::from([1, 2, 3])

    let reader = || -> {
        result data::len()
    }

    // Both reader and data accessible
    let len1 = reader()
    let len2 = data::len()

    println("Lengths: {}, {}", len1, len2)
}
```

#### Nested Closures

```cursive
function nested_closure_example()
    uses alloc.heap
{
    let outer = 10

    let make_inner = || -> {
        let inner = outer * 2
        result move |x: i32| -> x + inner
    }

    let inner_closure = make_inner()
    let result = inner_closure(5)

    println("Result: {}", result)
}
```

### 4.10.7 Higher-Order Functions with Captures

Combining closures with higher-order functions enables functional programming patterns.

#### Map Pattern

```cursive
function map<T, U>(items: [T], f: (T) → U): [U]
    uses alloc.heap
{
    let result = Vec::new()
    loop item in items {
        result::push(f(item))
    }
    result result::into_array()
}

function map_example()
    uses alloc.heap
{
    let numbers = [1, 2, 3, 4, 5]
    let squared = map(numbers, |x| -> x * x)
    println("Squared: {:?}", squared)
}
```

#### Filter Pattern

```cursive
function filter<T>(items: [T], predicate: (T) → bool): [T]
    uses alloc.heap
{
    let result = Vec::new()
    loop item in items {
        if predicate(item) {
            result::push(item)
        }
    }
    result result::into_array()
}

function filter_example()
    uses alloc.heap
{
    let numbers = [1, 2, 3, 4, 5, 6]
    let evens = filter(numbers, |x| -> x % 2 == 0)
    println("Evens: {:?}", evens)
}
```

#### Fold Pattern

```cursive
function fold<T, Acc>(items: [T], initial: Acc, f: (Acc, T) → Acc): Acc {
    var accumulator = initial
    loop item in items {
        accumulator = f(accumulator, item)
    }
    result accumulator
}

function fold_example()
    uses alloc.heap
{
    let numbers = [1, 2, 3, 4, 5]
    let sum = fold(numbers, 0, |acc, x| -> acc + x)
    let product = fold(numbers, 1, |acc, x| -> acc * x)
    println("Sum: {}, Product: {}", sum, product)
}
```

#### Closure Factory Pattern

```cursive
function create_multiplier(factor: i32): (i32) → i32 {
    move |x| -> x * factor
}

function create_filter(threshold: i32): (i32) → bool {
    move |x| -> x > threshold
}

function factory_example()
    uses alloc.heap
{
    let times_three = create_multiplier(3)
    let above_ten = create_filter(10)

    let numbers = [5, 10, 15, 20]
    let multiplied = map(numbers, times_three)
    let filtered = filter(multiplied, above_ten)

    println("Result: {:?}", filtered)
}
```
---

## 4.11 Control Flow Interactions

### 4.11.1 Loops and Break Values

1. A loop with `break value` transfers ownership of the value out of the loop. The loop’s result type is the join of all break values (Part VI §6.11.3).
2. Owned values moved out by `break` invalidate their in-loop bindings.

```
[Loop-Result-From-Break]
loop contains breaks producing values v₁: τ, …, vₙ: τ
----------------------------------------
Γ ⊢ loop { … } : τ
```

### 4.11.2 Labeled Blocks

`break 'label value` is permitted only when the block’s type unifies across all exits. If the value is owned, the binding within the block becomes invalid.

### 4.11.3 No-Panic and Effect-Gated Branching

- `[[no_panic]]` regions forbid operations that might panic, including permission violations that would otherwise be diagnosed as E4003 (Part VI §6.14).
- `comptime if effects_include(...)` must type-check both branches; permissions in the inactive branch still need to be valid. The syntax of effect predicates follows `EffectGatedBranch` in Appendix A.

### 4.11.4 Loop Permission Patterns

Loops interact with the permission system through iterators, break values, and per-iteration ownership.

#### Basic Loop Iteration

```cursive
function sum_array(numbers: [i32]): i32 {
    var total = 0
    loop n in numbers {
        total = total + n
    }
    result total
}

function sum_vector(vec: imm Vec<i32>): i32 {
    var sum = 0
    loop item in vec {
        sum = sum + item
    }
    result sum
}
```

#### Loops with Owned Values

```cursive
record Item {
    value: i32,
    name: string
}

function process_items(items: own Vec<Item>)
    uses alloc.heap
{
    loop item in items {
        // Each item moved from vector
        println("Item: {}, Value: {}", item.name, item.value)
        // item dropped at end of iteration
    }
    // items fully consumed
}
```

#### Loops with Mutable Access

```cursive
function increment_all(numbers: mut Vec<i32]) {
    loop mut n in numbers {
        n = n + 1
    }
}

function modify_items(items: mut Vec<Item>)
    uses alloc.heap
{
    loop mut item in items {
        item.value = item.value * 2
    }
}
```

#### Break with Owned Values

```cursive
function find_first_match(items: own Vec<Item>): Option<Item>
    uses alloc.heap
{
    loop item in items {
        if item.value > 100 {
            result Option::Some(item)
        }
    }
    result Option::None
}

function search_until_condition(data: own Vec<i32>): Option<i32>
    uses alloc.heap
{
    var found = Option::None

    loop value in data {
        if value > 50 {
            found = Option::Some(value)
            break
        }
    }

    result found
}
```

#### Labeled Breaks with Ownership

```cursive
function nested_search(matrix: [[i32]]): Option<(i32, i32)>
    uses alloc.heap
{
    'outer: loop row in matrix {
        loop col in row {
            if col == 42 {
                result Option::Some((row, col))
            }
        }
    }
    result Option::None
}

function find_in_nested(items: Vec<Vec<Item>>): Option<Item>
    uses alloc.heap
{
    'search: loop group in items {
        loop item in group {
            if item.value > 100 {
                break 'search Option::Some(item)
            }
        }
    }
    result Option::None
}
```

#### While Loops with Permissions

```cursive
function drain_until_empty(queue: mut Vec<i32>): i32
    uses alloc.heap
{
    var sum = 0

    while queue::len() > 0 {
        let value = queue::pop()
        value match {
            Option::Some(v) => {
                sum = sum + v
            },
            Option::None => {
                break
            }
        }
    }

    result sum
}
```

#### Range Iteration

```cursive
function fill_array(arr: mut [i32; 10]) {
    loop i in 0..10 {
        arr[i] = i * i
    }
}

function sum_range(start: i32, end: i32): i32 {
    var total = 0
    loop i in start..end {
        total = total + i
    }
    result total
}
```

### 4.11.5 Match Expression Ownership

Pattern matching transfers ownership according to the scrutinee's permission and the pattern binding.

#### Basic Match with Ownership Transfer

```cursive
enum Message {
    Text(string),
    Number(i32),
    Data(Vec<u8>)
}

function process_owned_message(msg: own Message): string
    uses alloc.heap
{
    msg match {
        Message::Text(s) => {
            // s is owned, moved from msg
            result s
        },
        Message::Number(n) => {
            result string::from(n)
        },
        Message::Data(d) => {
            let len = d::len()
            result string::from("Data: {} bytes", len)
        }
    }
}
```

#### Match with Immutable Access

```cursive
function examine_message(msg: imm Message): i32 {
    msg match {
        Message::Text(s) => {
            // s has imm permission
            result s::len() as i32
        },
        Message::Number(n) => {
            result n
        },
        Message::Data(d) => {
            result d::len() as i32
        }
    }
}
```

#### Match with Mutable Access

```cursive
procedure modify_message(msg: mut Message)
    uses alloc.heap
{
    msg match {
        Message::Text(mut s) => {
            s = string::from("Modified")
        },
        Message::Number(mut n) => {
            n = n * 2
        },
        Message::Data(mut d) => {
            d::push(0)
        }
    }
}
```

#### Nested Pattern Matching

```cursive
enum Outer {
    Single(i32),
    Nested(Option<Vec<i32>>),
    Double(Result<string, i32>)
}

function extract_nested(outer: own Outer): Option<Vec<i32>>
    uses alloc.heap
{
    outer match {
        Outer::Nested(Option::Some(vec)) => {
            // vec moved out of nested structure
            result Option::Some(vec)
        },
        _ => {
            result Option::None
        }
    }
}
```

#### Match with Partial Moves

```cursive
record Container {
    id: i32,
    data: Vec<i32>,
    name: string
}

function extract_data_from_match(c: own Container): Vec<i32>
    uses alloc.heap
{
    // Move out data field
    let data = move c.data

    // Can still access Copy field
    println("ID: {}", c.id)

    result data
}
```

#### Option and Result Matching

```cursive
function unwrap_or_default<T>(opt: own Option<T>, default: T): T {
    opt match {
        Option::Some(value) => {
            result value
        },
        Option::None => {
            result default
        }
    }
}

function handle_result<T, E>(res: own Result<T, E>): T
    uses panic
{
    res match {
        Result::Ok(value) => {
            result value
        },
        Result::Err(error) => {
            panic("Error occurred")
        }
    }
}
```

#### Guard Clauses with Permissions

```cursive
function filter_positive(opt: own Option<i32>): Option<i32> {
    opt match {
        Option::Some(n) if n > 0 => {
            result Option::Some(n)
        },
        _ => {
            result Option::None
        }
    }
}

function match_with_complex_guard(msg: own Message): bool
    uses alloc.heap
{
    msg match {
        Message::Text(s) if s::len() > 10 => {
            result true
        },
        Message::Data(d) if d::len() > 100 => {
            result true
        },
        _ => {
            result false
        }
    }
}
```

### 4.11.6 Early Return with Owned Values

Early returns from functions must respect ownership and permission constraints.

#### Simple Early Return

```cursive
function validate_and_process(data: own Vec<i32>): Result<i32, string>
    uses alloc.heap
{
    if data::len() == 0 {
        result Result::Err(string::from("Empty data"))
    }

    var sum = 0
    loop item in data {
        sum = sum + item
    }

    result Result::Ok(sum)
}
```

#### Early Return with Permission Transfer

```cursive
function extract_if_valid(container: own Container): Option<Vec<i32>>
    uses alloc.heap
{
    if container.id < 0 {
        result Option::None
    }

    if container::data::len() == 0 {
        result Option::None
    }

    result Option::Some(move container.data)
}
```

#### Multiple Return Paths with Ownership

```cursive
function process_message_early(msg: own Message): string
    uses alloc.heap
{
    msg match {
        Message::Text(s) => {
            if s::len() == 0 {
                result string::from("Empty text")
            }
            result s
        },
        Message::Number(n) => {
            if n == 0 {
                result string::from("Zero")
            }
            result string::from(n)
        },
        Message::Data(d) => {
            if d::len() == 0 {
                result string::from("No data")
            }
            result string::from("Data: {} bytes", d::len())
        }
    }
}
```

#### Early Return in Loops

```cursive
function find_first_positive(numbers: own Vec<i32>): Option<i32>
    uses alloc.heap
{
    loop n in numbers {
        if n > 0 {
            result Option::Some(n)
        }
    }
    result Option::None
}

function search_nested(matrix: Vec<Vec<i32>>): Option<i32>
    uses alloc.heap
{
    loop row in matrix {
        loop col in row {
            if col > 100 {
                result Option::Some(col)
            }
        }
    }
    result Option::None
}
```

#### Conditional Ownership Transfer

```cursive
function transfer_if_valid(data: own Vec<i32>, threshold: i32): Result<Vec<i32>, string>
    uses alloc.heap
{
    if data::len() < threshold as usize {
        result Result::Err(string::from("Insufficient data"))
    }

    var valid = true
    loop item in data {
        if item < 0 {
            valid = false
            break
        }
    }

    if !valid {
        result Result::Err(string::from("Invalid data"))
    }

    result Result::Ok(data)
}
```

#### Early Return with Resource Cleanup

```cursive
record Resource {
    handle: i32,
    name: string
}

procedure Resource::close(self: own Resource)
    uses fs.close
{
    fs::close_handle(self.handle)
}

function use_resource(res: own Resource): Result<i32, string>
    uses fs.read, fs.close, alloc.heap
{
    if res.handle < 0 {
        res::close()
        result Result::Err(string::from("Invalid handle"))
    }

    let data = fs::read_from_handle(res.handle)

    data match {
        Option::Some(value) => {
            res::close()
            result Result::Ok(value)
        },
        Option::None => {
            res::close()
            result Result::Err(string::from("Read failed"))
        }
    }
}
```

---

## 4.12 Permissions and Data Structures

### 4.12.1 Structural Types

| Structure | Permission Propagation |
|-----------|------------------------|
| Tuples `(T₁, …, Tₙ)` | Wrapper distributes to all elements. |
| Records | Wrapper applies to every field; field-level wrappers override. |
| Arrays `[T; n]` | `own` array owns its elements; `mut` allows mutation in place; slices derived from arrays are `imm` views unless explicitly wrapped. |
| Slices `[T]` | Always views; wrapping with `own`/`mut` is ill-formed (see rule below). |
| Enums | Wrapper applies to active payload. |

```
[WF-Slice-Permission]
Γ ⊢ [T] : Type
------------------------
Γ ⊬ own [T] : Type  (slices are views)
```
Attempts to allocate `own [T]` are rejected with E4003.

### 4.12.2 Strings

- `string@Owned` is non-`Copy`; moving transfers ownership.
- `string@View` is `Copy` and can be implicitly coerced from `string@Owned`.

### 4.12.3 Standard Library Types

- `Vec<T>` is owned; `mut Vec<T>` allows in-place mutation of the buffer; slicing yields views with `imm` by default.
- `Option<T>` and `Result<T, E>` adopt the permission of their payloads automatically.


### 4.12.4 Nested Structure Permissions

Permission wrappers propagate through nested structures, following the permission of the outermost container.

#### Records with Nested Fields

```cursive
record Point {
    x: f64,
    y: f64
}

record Shape {
    center: Point,
    radius: f64,
    name: string
}

function access_nested_field(shape: imm Shape): f64 {
    // Immutable access to nested field
    result shape.center.x
}

procedure modify_nested_field(shape: mut Shape) {
    // Mutable access to nested field
    shape.center.x = 10.0
    shape.center.y = 20.0
}

function move_nested_field(shape: own Shape): Point
    uses alloc.heap
{
    // Move nested field out
    result move shape.center
    // shape is now partially moved
}
```

#### Nested Collections

```cursive
function process_nested_vectors(data: own Vec<Vec<i32>>): i32
    uses alloc.heap
{
    var sum = 0

    loop row in data {
        loop value in row {
            sum = sum + value
        }
    }

    result sum
}

function modify_nested_vectors(data: mut Vec<Vec<i32>>)
    uses alloc.heap
{
    loop mut row in data {
        loop mut value in row {
            value = value * 2
        }
    }
}

function extract_row(data: own Vec<Vec<i32>>, index: usize): Option<Vec<i32>>
    uses alloc.heap
{
    if index < data::len() {
        result Option::Some(data::remove(index))
    } else {
        result Option::None
    }
}
```

#### Records Containing Collections

```cursive
record Database {
    users: Vec<string>,
    metadata: Vec<(string, string)>,
    count: i32
}

function add_user(db: mut Database, name: string)
    uses alloc.heap
{
    db.users::push(name)
    db.count = db.count + 1
}

function extract_users(db: own Database): Vec<string>
    uses alloc.heap
{
    result move db.users
    // db is now partially moved
}

function clone_users(db: imm Database): Vec<string>
    uses alloc.heap
{
    let result = Vec::new()
    loop user in db.users {
        result::push(user::to_owned())
    }
    result result
}
```

#### Tuples with Mixed Permissions

```cursive
function tuple_nested_access()
    uses alloc.heap
{
    let data: (Vec<i32>, string, (i32, i32)) = (
        Vec::from([1, 2, 3]),
        string::from("example"),
        (42, 100)
    )

    // Access Copy field
    let first_coord = data.2.0

    // Access non-Copy field (coercion to imm)
    let len = data.0::len()

    // Cannot move individual tuple elements without destructuring
    let (vec, str, coords) = data
}
```

#### Enums with Nested Structures

```cursive
enum TreeNode {
    Leaf(i32),
    Branch {
        left: Box<TreeNode>,
        right: Box<TreeNode>,
        value: i32
    }
}

function tree_depth(node: imm TreeNode): i32 {
    node match {
        TreeNode::Leaf(_) => {
            result 1
        },
        TreeNode::Branch { left, right, value } => {
            let left_depth = tree_depth(left)
            let right_depth = tree_depth(right)

            if left_depth > right_depth {
                result left_depth + 1
            } else {
                result right_depth + 1
            }
        }
    }
}

function extract_value(node: own TreeNode): Option<i32> {
    node match {
        TreeNode::Leaf(v) => {
            result Option::Some(v)
        },
        TreeNode::Branch { left, right, value } => {
            result Option::Some(value)
        }
    }
}
```

### 4.12.5 Generic Container Permissions

Generic containers like `Vec<T>`, `Option<T>`, and `Result<T, E>` interact with permissions through their type parameters.

#### Vector Ownership Patterns

```cursive
function create_owned_vector(): own Vec<i32>
    uses alloc.heap
{
    let vec: own Vec<i32> = Vec::new()
    vec::push(1)
    vec::push(2)
    vec::push(3)
    result vec
}

function vector_of_strings(): own Vec<string>
    uses alloc.heap
{
    let vec = Vec::new()
    vec::push(string::from("hello"))
    vec::push(string::from("world"))
    result vec
}

function vector_permission_demo()
    uses alloc.heap
{
    let numbers: own Vec<i32> = create_owned_vector()

    // Coerce own → imm for reading
    let len = numbers::len()

    // Coerce own → mut for modification
    numbers::push(4)

    // Move ownership
    let moved = move numbers
    // numbers is now invalid
}
```

#### Vectors of Owned Values

```cursive
record Resource {
    handle: i32,
    name: string
}

function collect_resources(): own Vec<Resource>
    uses alloc.heap
{
    let resources = Vec::new()

    resources::push(Resource { handle: 1, name: string::from("first") })
    resources::push(Resource { handle: 2, name: string::from("second") })

    result resources
}

function process_all_resources(resources: own Vec<Resource>)
    uses alloc.heap
{
    loop resource in resources {
        // Each resource is owned (moved from vector)
        println("Resource: {}, Handle: {}", resource.name, resource.handle)
        // resource dropped at end of iteration
    }
}
```

#### Nested Generic Containers

```cursive
function create_matrix(): own Vec<Vec<i32>>
    uses alloc.heap
{
    let matrix = Vec::new()

    let row1 = Vec::from([1, 2, 3])
    let row2 = Vec::from([4, 5, 6])

    matrix::push(row1)
    matrix::push(row2)

    result matrix
}

function sum_matrix(matrix: imm Vec<Vec<i32>>): i32 {
    var total = 0

    loop row in matrix {
        loop value in row {
            total = total + value
        }
    }

    result total
}

function transpose(matrix: own Vec<Vec<i32>>): own Vec<Vec<i32>>
    uses alloc.heap
{
    if matrix::len() == 0 {
        result matrix
    }

    let rows = matrix::len()
    let cols = matrix[0]::len()

    let result = Vec::new()

    loop col_idx in 0..cols {
        let column = Vec::new()

        loop row_idx in 0..rows {
            column::push(matrix[row_idx][col_idx])
        }

        result::push(column)
    }

    result result
}
```

#### Generic Structs with Permissions

```cursive
record Container<T> {
    value: T,
    id: i32
}

function Container::new<T>(value: T, id: i32): own Container<T> {
    result Container { value, id }
}

function Container::into_value<T>(self: own Container<T>): T {
    result move self.value
}

function Container::get_value<T>(self: imm Container<T>): imm T {
    result self.value
}

function generic_container_example()
    uses alloc.heap
{
    let container = Container::new(Vec::from([1, 2, 3]), 42)

    // Access value with imm permission
    let len = container::get_value()::len()

    // Extract owned value
    let vec = container::into_value()
}
```

#### Maps and Hash Tables

```cursive
function create_map(): own Map<string, i32>
    uses alloc.heap
{
    let map = Map::new()

    map::insert(string::from("one"), 1)
    map::insert(string::from("two"), 2)
    map::insert(string::from("three"), 3)

    result map
}

function lookup_value(map: imm Map<string, i32>, key: string): Option<i32> {
    result map::get(key)
}

function modify_map(map: mut Map<string, i32>)
    uses alloc.heap
{
    map::insert(string::from("four"), 4)
    map::remove(string::from("one"))
}

function consume_map(map: own Map<string, i32>): i32
    uses alloc.heap
{
    var sum = 0

    loop (key, value) in map {
        sum = sum + value
    }

    result sum
}
```

### 4.12.6 Option and Result Ownership

The `Option<T>` and `Result<T, E>` types adopt permissions from their payloads, enabling ownership transfer through success/failure paths.

#### Option Ownership Transfer

```cursive
function wrap_value(value: own Vec<i32>): own Option<Vec<i32>> {
    result Option::Some(value)
}

function unwrap_or_empty(opt: own Option<Vec<i32>>): own Vec<i32>
    uses alloc.heap
{
    opt match {
        Option::Some(vec) => {
            result vec
        },
        Option::None => {
            result Vec::new()
        }
    }
}

function map_option<T, U>(opt: own Option<T>, f: (T) → U): own Option<U> {
    opt match {
        Option::Some(value) => {
            result Option::Some(f(value))
        },
        Option::None => {
            result Option::None
        }
    }
}
```

#### Option with Immutable Access

```cursive
function option_length(opt: imm Option<Vec<i32>>): i32 {
    opt match {
        Option::Some(vec) => {
            result vec::len() as i32
        },
        Option::None => {
            result 0
        }
    }
}

function option_contains(opt: imm Option<string>, target: string): bool
    uses alloc.heap
{
    opt match {
        Option::Some(s) => {
            result s == target
        },
        Option::None => {
            result false
        }
    }
}
```

#### Option with Mutable Access

```cursive
procedure option_modify(opt: mut Option<Vec<i32>>)
    uses alloc.heap
{
    opt match {
        Option::Some(mut vec) => {
            vec::push(42)
        },
        Option::None => {
            // Nothing to modify
        }
    }
}

procedure option_replace<T>(opt: mut Option<T>, new_value: T) {
    opt match {
        Option::Some(old_value) => {
            opt = Option::Some(new_value)
        },
        Option::None => {
            opt = Option::Some(new_value)
        }
    }
}
```

#### Result Ownership Transfer

```cursive
function safe_divide(numerator: i32, denominator: i32): Result<i32, string>
    uses alloc.heap
{
    if denominator == 0 {
        result Result::Err(string::from("Division by zero"))
    } else {
        result Result::Ok(numerator / denominator)
    }
}

function process_result(res: own Result<Vec<i32>, string>): i32
    uses alloc.heap
{
    res match {
        Result::Ok(vec) => {
            var sum = 0
            loop item in vec {
                sum = sum + item
            }
            result sum
        },
        Result::Err(err) => {
            println("Error: {}", err)
            result 0
        }
    }
}
```

#### Result with Error Propagation

```cursive
function read_and_parse(filename: string): Result<i32, string>
    uses fs.read, alloc.heap
{
    let contents = fs::read_file(filename)

    contents match {
        Result::Ok(data) => {
            let parsed = string::parse_int(data)
            parsed match {
                Option::Some(value) => {
                    result Result::Ok(value)
                },
                Option::None => {
                    result Result::Err(string::from("Parse error"))
                }
            }
        },
        Result::Err(err) => {
            result Result::Err(err)
        }
    }
}

function chain_results(a: Result<i32, string>, b: Result<i32, string>): Result<i32, string> {
    a match {
        Result::Ok(val_a) => {
            b match {
                Result::Ok(val_b) => {
                    result Result::Ok(val_a + val_b)
                },
                Result::Err(err) => {
                    result Result::Err(err)
                }
            }
        },
        Result::Err(err) => {
            result Result::Err(err)
        }
    }
}
```

#### Nested Option and Result

```cursive
function nested_option(): Option<Option<i32>> {
    result Option::Some(Option::Some(42))
}

function flatten_option<T>(opt: own Option<Option<T>>): own Option<T> {
    opt match {
        Option::Some(inner) => {
            result inner
        },
        Option::None => {
            result Option::None
        }
    }
}

function result_of_option(value: i32): Result<Option<i32>, string> {
    if value > 0 {
        result Result::Ok(Option::Some(value))
    } else if value == 0 {
        result Result::Ok(Option::None)
    } else {
        result Result::Err(string::from("Negative value"))
    }
}

function extract_nested(res: own Result<Option<Vec<i32>>, string>): own Vec<i32>
    uses alloc.heap, panic
{
    res match {
        Result::Ok(opt) => {
            opt match {
                Option::Some(vec) => {
                    result vec
                },
                Option::None => {
                    result Vec::new()
                }
            }
        },
        Result::Err(err) => {
            panic("Error: {}", err)
        }
    }
}
```

#### Combining Option and Result

```cursive
function option_to_result<T>(opt: own Option<T>, error_msg: string): Result<T, string> {
    opt match {
        Option::Some(value) => {
            result Result::Ok(value)
        },
        Option::None => {
            result Result::Err(error_msg)
        }
    }
}

function result_to_option<T, E>(res: own Result<T, E>): Option<T> {
    res match {
        Result::Ok(value) => {
            result Option::Some(value)
        },
        Result::Err(_) => {
            result Option::None
        }
    }
}

function safe_get(vec: imm Vec<i32>, index: usize): Option<i32> {
    if index < vec::len() {
        result Option::Some(vec[index])
    } else {
        result Option::None
    }
}

function validated_get(vec: imm Vec<i32>, index: usize): Result<i32, string>
    uses alloc.heap
{
    if index < vec::len() {
        result Result::Ok(vec[index])
    } else {
        result Result::Err(string::from("Index out of bounds"))
    }
}
```

---

## 4.13 Regions and Permissions

Region judgments extend the typing context with a region stack `Δ`:

```
Δ ::= ∅ | (Δ · r)
```

Each frame `r` denotes an active region in LIFO order.

**Definition 4.13.1 (Outlives Relation).** Let `Δ` be the region stack at a use site. A construct outlives region `r` precisely when `r ∉ Δ`.

### 4.13.1 Allocation Rules

```
[Alloc-In-Region]
Γ; Δ · r ⊢ v : τ
------------------------------
Γ; Δ · r ⊢ alloc_in<r>(v) : own τ@r
```

Values allocated in region `r` carry an implicit region tag. They must not be returned or stored beyond the region boundary (diagnostics E4014/E3D09; Part III §3.9.2, Part V §4.11.3).

```
[Region-Escape]
Γ; Δ · r ⊢ v : own τ@r
target stack Δ' with r ∉ Δ'
------------------------------
ERROR E4014

In `[Region-Escape]`, `Δ'` denotes the region stack at the destination context.
```

```
[Region-Close]
Γ; Δ · r ⊢ body : τ
------------------------------
Γ; Δ ⊢ region r { body } : τ
```

### 4.13.2 Iteration Discipline

Loop bodies that allocate per-iteration regions must drop owned values before the iteration ends (Part VI §6.12). This part restates: **Rule 4.13.2** – All owned region values must be consumed or dropped before the enclosing `region` block closes.

### 4.13.3 Deferred Dependencies

Part VIII (deferred) will provide runtime semantics for regions, concurrency, and memory reclamation. Until published, the operational rules above are normative and MUST be implemented exactly as stated.

---

## 4.14 Effects, Unsafe Operations, and Permissions

### 4.14.1 Effect Requirements

- Raw pointer operations (`*T`, `*mut T`) require `unsafe.ptr` or related effects and do not relax permission rules (Part V §4.4.3).
- Procedures annotated `[[no_panic]]` implicitly forbid the `panic` effect; permission errors must therefore be resolved statically (§4.11.3).

```
[Effect-Inclusion]
Γ ⊢ e : τ ! ε
ε ⊆ ε_available
----------------------
Γ ⊢ e permitted

[Effect-Exclusion]
Γ ⊢ e : τ ! ε
panic ∈ ε
[[no_panic]] active
----------------------
ERROR E6009
```

### 4.14.2 Effect-Polymorphic Code

Effect polymorphism propagates permission costs: if a polymorphic function must clone an argument to satisfy `own`, it must declare `alloc.heap` or whichever effect performs the copy (Part II §2.9.4).

```
[Effect-Poly]
Γ ⊢ f : ∀ε. (τ₁, …, τₙ) → τᵣ ! ε
Γ ⊢ args : τ₁ … τₙ
ε_instantiated = resolve_effects(f, args)
ε_instantiated ⊆ ε_available
----------------------------------------
Γ ⊢ f(args) : τᵣ ! ε_instantiated
```

`resolve_effects` MUST include any effects introduced by permission adjustments (e.g., cloning, allocation, or dropping captured owners). Part VII (deferred) will provide the complete effect lattice once published.


## 4.Z Permission Attributes

Permission attributes provide fine-grained control over permission behavior, enable compiler optimizations, and document permission requirements for maintainers and tools.

### 4.Z.1 Overview

Permission attributes modify how the compiler treats permissions, capturing patterns, and ownership transfer. They are applied using the attribute syntax `[[attribute_name]]` or `[[attribute_name(args)]]`.

### 4.Z.2 Standard Permission Attributes

| Attribute | Applies To | Purpose |
|-----------|------------|---------|
| `[[must_use]]` | Functions, types | Warns if owned result is ignored |
| `[[no_implicit_copy]]` | Types | Prevents implicit `Copy` even if eligible |
| `[[permission(perm)]]` | Parameters, fields | Documents required permission |
| `[[consume]]` | Parameters | Marks parameter as consumed (ownership transferred) |
| `[[shared]]` | Types | Indicates type supports shared access |
| `[[exclusive]]` | Procedures | Requires exclusive access to `self` |

### 4.Z.3 Must-Use Attribute

The `[[must_use]]` attribute warns when an owned value is created but not used, preventing resource leaks.

#### On Functions

```cursive
[[must_use]]
function create_resource(): own Resource
    uses alloc.heap
{
    result Resource::new()
}

function example()
    uses alloc.heap
{
    // WARNING: unused owned value from [[must_use]] function
    create_resource()

    // OK: value is bound
    let res = create_resource()

    // OK: value is consumed
    res::close()
}
```

#### On Types

```cursive
[[must_use]]
record Handle {
    id: i32,
    name: string
}

function create_handle(): own Handle
    uses alloc.heap
{
    result Handle { id: 42, name: string::from("example") }
}

function use_handle()
    uses alloc.heap
{
    // WARNING: unused value of [[must_use]] type
    create_handle()

    // OK: value is used
    let handle = create_handle()
    println("Handle: {}", handle.id)
}
```

#### With Custom Messages

```cursive
[[must_use = "Resource must be explicitly closed"]]
function open_file(path: string): own File
    uses fs.open, alloc.heap
{
    result File::open(path)
}
```

### 4.Z.4 No Implicit Copy Attribute

Prevents a type from being `Copy` even if all fields are `Copy`, forcing explicit cloning.

```cursive
[[no_implicit_copy]]
record SensitiveData {
    value: i32,
    checksum: i32
}

function use_sensitive(data: SensitiveData) {
    // data is passed, ownership transfers
    println("Value: {}", data.value)
}

function example() {
    let data = SensitiveData { value: 42, checksum: 42 }

    // ERROR: cannot copy SensitiveData
    // use_sensitive(data)
    // use_sensitive(data)

    // OK: explicit move
    use_sensitive(move data)
}
```

### 4.Z.5 Permission Documentation Attributes

Document required permissions for parameters and fields to improve code clarity.

#### Parameter Permission Annotations

```cursive
procedure process_data(
    [[permission(own)]] data: Vec<i32>,
    [[permission(mut)]] output: Vec<i32>,
    [[permission(imm)]] config: Config
)
    uses alloc.heap
{
    // Clear documentation of permission requirements
    loop item in data {
        output::push(item * config.multiplier)
    }
}
```

#### Field Permission Annotations

```cursive
record Database {
    [[permission(own)]]
    connection: Connection,

    [[permission(mut)]]
    cache: Vec<Entry>,

    [[permission(imm)]]
    config: Config
}

// Documents that connection will be consumed
procedure Database::close(self: own Database)
    uses network.close
{
    self.connection::close()
}
```

### 4.Z.6 Consume Attribute

Marks parameters that are consumed by the function, clarifying ownership transfer.

```cursive
procedure log_and_drop(
    [[consume]]
    message: own string
)
    uses io.write, alloc.heap
{
    println("Message: {}", message)
    // message is consumed (dropped at end)
}

function transfer_ownership(
    [[consume]]
    data: own Vec<i32>
): own Result<(), string>
    uses alloc.heap
{
    if data::len() == 0 {
        result Result::Err(string::from("Empty data"))
    }

    // Process and consume data
    loop item in data {
        println("{}", item)
    }

    result Result::Ok(())
}
```

### 4.Z.7 Shared Access Attribute

Indicates that a type is designed for shared (immutable) access patterns.

```cursive
[[shared]]
record Config {
    host: string,
    port: i32,
    timeout: i32
}

function Config::new(host: string, port: i32): own Config
    uses alloc.heap
{
    result Config {
        host,
        port,
        timeout: 30
    }
}

// Config is typically passed as imm
function connect(config: imm Config): Result<Connection, string>
    uses network.connect, alloc.heap
{
    // Use config without modification
    Connection::connect(config.host, config.port)
}
```

### 4.Z.8 Exclusive Access Attribute

Requires exclusive access for procedures, documenting mutation requirements.

```cursive
record Counter {
    value: i32
}

[[exclusive]]
procedure Counter::increment(self: mut Counter) {
    self.value = self.value + 1
}

[[exclusive]]
procedure Counter::reset(self: mut Counter) {
    self.value = 0
}

// Non-exclusive procedure for reading
function Counter::get(self: imm Counter): i32 {
    result self.value
}
```

### 4.Z.9 Combining Attributes

Multiple attributes can be combined to provide comprehensive documentation and enforcement.

```cursive
[[must_use = "Handle must be explicitly closed"]]
[[shared]]
record FileHandle {
    [[permission(own)]]
    descriptor: i32,

    [[permission(imm)]]
    path: string
}

[[must_use]]
[[exclusive]]
procedure FileHandle::write(
    self: mut FileHandle,
    [[consume]] data: own Vec<u8>
): Result<usize, string>
    uses fs.write, alloc.heap
{
    fs::write_to_descriptor(self.descriptor, data)
}

[[consume]]
procedure FileHandle::close(self: own FileHandle)
    uses fs.close
{
    fs::close_descriptor(self.descriptor)
}
```

### 4.Z.10 Attribute Verification

The compiler enforces attribute constraints statically:

#### Must-Use Violations

```cursive
[[must_use]]
function allocate_buffer(): own Vec<u8>
    uses alloc.heap
{
    result Vec::with_capacity(1024)
}

function bad_example()
    uses alloc.heap
{
    allocate_buffer()
    // WARNING W4011: unused result from [[must_use]] function `allocate_buffer`
    // help: bind the result to a variable or explicitly drop it
}

function good_example()
    uses alloc.heap
{
    let buffer = allocate_buffer()
    // Use buffer...
    buffer::push(1)
}
```

#### Permission Attribute Mismatches

```cursive
procedure process(
    [[permission(mut)]] data: Vec<i32>
)
    uses alloc.heap
{
    data::push(42)
}

function example()
    uses alloc.heap
{
    let data: imm Vec<i32> = Vec::new()

    // ERROR E4003: permission mismatch
    // note: parameter `data` requires `mut` permission (from [[permission(mut)]])
    // note: argument has `imm` permission
    process(data)
}
```

### 4.Z.11 Custom Attributes

User-defined attributes for documentation purposes (not enforced by compiler):

```cursive
[[doc_permission(own, "Consumes the data structure")]]
[[performance("O(n) where n is the number of elements")]]
function flatten<T>(nested: own Vec<Vec<T>>): own Vec<T>
    uses alloc.heap
{
    let result = Vec::new()

    loop inner in nested {
        loop item in inner {
            result::push(item)
        }
    }

    result result
}
```

### 4.Z.12 Interaction with Effects

Permission attributes interact with effect annotations to provide complete function signatures.

```cursive
[[must_use]]
[[exclusive]]
function critical_section<T>(
    [[consume]] lock: own Mutex<T>,
    operation: (mut T) → Result<(), string>
): Result<(), string>
    uses alloc.heap, panic
{
    let guard = lock::acquire()

    let result = operation(guard::get_mut())

    guard::release()

    result result
}
```

### 4.Z.13 Diagnostics for Attributes

| Code | Description | Example |
|------|-------------|---------|
| W4011 | Unused [[must_use]] value | Ignoring owned result |
| E4021 | [[permission]] attribute mismatch | Passing wrong permission |
| W4012 | Redundant [[permission]] attribute | Attribute matches default |
| E4022 | [[exclusive]] requires mut/own | Calling on imm receiver |
| W4013 | [[consume]] on Copy type | Attribute ineffective |


---

## 4.15 Diagnostics Catalogue

This section catalogs errors and warnings related to the lexical permission system, including permission violations, move semantics, region escapes, and permission annotations.

### 4.15.1 Permission Violation Errors (E4003-xx)

| Code | Description | Example |
|------|-------------|---------|
| E4003-01 | Insufficient permission for mutation | Writing to `imm T` binding |
| E4003-02 | Insufficient permission for procedure call | Calling `mut self` procedure on `imm` receiver |
| E4003-03 | Insufficient permission for field mutation | Mutating field through `imm` permission |
| E4003-04 | Insufficient permission for move | Moving from `imm` or `mut` without ownership |
| E4003-05 | Permission mismatch in assignment | Assigning `imm T` to `mut T` binding |
| E4003-06 | Permission downgrade not allowed | Attempting `imm → mut` coercion |
| E4003-07 | Cannot coerce immutable to mutable permission | Passing `imm T` where `mut T` required |
| E4003-08 | Cannot move without ownership | Moving from `imm T` or `mut T` |
| E4003-09 | Insufficient permission for closure capture | Capturing `own` where only `imm` available |
| E4003-10 | Permission required for dereference | Dereferencing pointer with insufficient permission |
| E4003-11 | Field permission exceeds container permission | `imm` container with `mut` field access |
| E4003-12 | Index mutation requires mutable permission | `arr[i] = val` where `arr: imm [T]` |
| E4003-13 | Slice mutation requires mutable source | Creating `mut` slice from `imm` container |
| E4003-14 | Cannot obtain exclusive permission | Multiple active mut permissions attempted |
| E4003-15 | Permission conflict in pattern binding | Pattern requires permission not available |

### 4.15.2 Move Semantic Errors (E4006-xx)

| Code | Description | Example |
|------|-------------|---------|
| E4006-01 | Use of moved value | Using variable after `move` |
| E4006-02 | Value used after partial move | Using struct after field moved |
| E4006-03 | Cannot move without ownership | Moving from imm or mut without ownership |
| E4006-04 | Cannot move value with active mut access | Moving with insufficient permission |
| E4006-05 | Move in loop body | Moving in loop iteration (non-Copy) |
| E4006-06 | Moved value in closure capture | Closure captures moved value |
| E4006-07 | Cannot move out of array/slice element | `move arr[0]` without destructuring |
| E4006-08 | Partial move prevents full move | Cannot move whole after partial move |
| E4006-09 | Cannot move from indexing operation | `let x = move vec[i]` |
| E4006-10 | Move out of match scrutinee | Moving field in match guard |
| E4006-11 | Cannot move without ownership transfer | Returning moved value without ownership |
| E4006-12 | Double move in conditional | Moving in both if/else branches |
| E4006-13 | Move invalidates later usage | Using moved value in deferred block |
| E4006-14 | Cannot move Copy type explicitly | `move x` where `x: i32` (Copy) |
| E4006-15 | Move in pattern binding unavailable | `move` pattern on non-moveable value |

### 4.15.3 Region Escape Errors (E4014-xx, E4015-xx)

| Code | Description | Example |
|------|-------------|---------|
| E4014-01 | Region-allocated value escapes via return | Returning region-allocated value |
| E4014-02 | Region value stored in outer scope | Storing region value in outer binding |
| E4014-03 | Region value escapes via closure | Closure captures and outlives region |
| E4014-04 | Region lifetime insufficient for capture | Captured region value outlives region |
| E4014-05 | Region mismatch in assignment | Assigning inner region to outer binding |
| E4014-06 | Cannot return region-scoped value | Function returns region-bound value |
| E4014-07 | Region value passed to non-region parameter | Passing region value to heap-expecting fn |
| E4015-01 | Closure captures escaping region value | Closure outlives captured region binding |
| E4015-02 | Closure moves region value out of scope | Closure takes ownership beyond region |
| E4015-03 | Returned closure captures region | Function returns closure capturing region |
| E4015-04 | Closure capture exceeds region lifetime | Region ends before closure invocation |
| E4015-05 | Multiple region captures conflict | Closure captures from incompatible regions |

### 4.15.4 Immutability and Binding Errors (E3D10-xx, E4401-xx)

| Code | Description | Example |
|------|-------------|---------|
| E3D10-01 | Assignment to immutable binding | `x = 5` where `let x = 3` |
| E3D10-02 | Reassignment to `let` binding | Mutating non-`mut` binding |
| E3D10-03 | Cannot mutate through immutable binding | `x.field = val` where `x` is `imm` |
| E3D10-04 | Immutable binding in mutable context | Pattern requires `mut` but binding is `let` |
| E4401-01 | Missing `result` in block expression | Block-bodied function without `result` |
| E4401-02 | Missing `result` in conditional block | If-expression block missing `result` |
| E4401-03 | Missing `result` in match arm | Match arm block without `result` |
| E4401-04 | Ambiguous block value without `result` | Block with multiple expressions, no `result` |

### 4.15.5 Type Annotation Errors (E4402-xx, E4403-xx, E4404-xx)

| Code | Description | Example |
|------|-------------|---------|
| E4402-01 | Missing type annotation on match binding | `Some(x) => ...` without type for `x` |
| E4402-02 | Cannot infer permission from match pattern | Pattern binding needs explicit permission |
| E4402-03 | Ambiguous permission in destructuring | `let (a, b) = tuple` with unclear permissions |
| E4403-01 | Missing type annotation on pipeline stage | `x => f` where `f` result type unclear |
| E4403-02 | Pipeline stage permission ambiguous | Cannot infer permission through pipeline |
| E4403-03 | Pipeline chain breaks permission inference | Middle stage needs explicit annotation |
| E4404-01 | Missing type annotation on loop iterator | `loop x in iter` without type for `x` |
| E4404-02 | Cannot infer iterator permission | Loop binding needs explicit permission |
| E4404-03 | Range iterator permission unclear | `loop i in 0..n` with custom iterator type |

### 4.15.6 Permission Warnings (W4xxx)

| Code | Description | Example |
|------|-------------|---------|
| W4001 | Unnecessary `own` annotation | `own i32` where `i32` is Copy |
| W4002 | Permission unnecessarily restrictive | Function requires `own` but only reads |
| W4003 | Value could use weaker permission | Parameter is `mut` but never mutated |
| W4004 | Unused moved value | Moving value that's never used |
| W4005 | Redundant permission coercion | Explicit coercion where implicit works |
| W4006 | Permission violation suppressed | `@allow(permission)` hiding real error |
| W4007 | Closure captures more than needed | Capturing `own` when `imm` sufficient |
| W4008 | Immutable binding never read | Unused `imm` binding |
| W4009 | Move could be avoided with weaker permission | Using `move` where permission coercion works |
| W4010 | Region allocation could be heap | Using region where heap more appropriate |

### 4.15.7 Advanced Permission Errors (E4020-xx)

| Code | Description | Example |
|------|-------------|---------|
| E4020-01 | Permission requirement in generic constraint | Generic bound `T: Mut` unsatisfied |
| E4020-02 | Modal state permission mismatch | Wrong permission for modal state field |
| E4020-03 | Contract precondition permission violation | Caller lacks permission for `must` clause |
| E4020-04 | Contract postcondition permission conflict | `will` clause requires unavailable permission |
| E4020-05 | Effect requires permission not available | Effect needs `mut` but has `imm` |
| E4020-06 | Unsafe block permission bypass detected | `unsafe` circumventing permission check |
| E4020-07 | Permission attribute conflict | `[[own]]` and `[[imm]]` both specified |
| E4020-08 | Variance violation in permission | Invariant type with incompatible permission |
| E4020-09 | Permission hole cannot be inferred | Hole `?perm` in context without constraints |
| E4020-10 | Cyclic permission dependency | Circular permission requirements |

### 4.15.8 Selected Diagnostic Examples

This subsection provides detailed diagnostic examples with complete error messages, notes, and help text.

**E4003-01: Insufficient Permission for Mutation**

```cursive
let x: imm i32 = 10
x = 20
// ERROR E4003-01: cannot assign to immutable binding `x`
// note: binding `x` has `imm` permission
// note: mutation requires `mut` or `own` permission
// help: declare binding as mutable: `let mut x: mut i32 = 10`
```

**E4003-02: Insufficient Permission for Procedure Call**

```cursive
record Counter { count: i32 }

procedure Counter::increment(self: mut Counter) {
    self.count = self.count + 1
}

function example() {
    let counter: imm Counter = Counter { count: 0 }
    counter::increment()
    // ERROR E4003-02: insufficient permission for procedure call
    // note: procedure `increment` requires `mut Counter` receiver
    // note: receiver `counter` has `imm Counter` permission
    // help: declare binding as mutable: `let mut counter: mut Counter = ...`
}
```

**E4003-04: Insufficient Permission for Move**

```cursive
record Data { value: i32 }

function take_ownership(d: own Data) { }

function example() {
    let data: imm Data = Data { value: 42 }
    take_ownership(move data)
    // ERROR E4003-04: cannot move value with `imm` permission
    // note: `move` requires `own` permission
    // note: binding `data` has `imm` permission
    // help: declare binding with `own` permission: `let data: own Data = ...`
}
```

**E4003-06: Permission Downgrade Not Allowed**

```cursive
function example() {
    let x: imm i32 = 10
    let y: mut i32 = x
    // ERROR E4003-06: cannot coerce `imm i32` to `mut i32`
    // note: permission lattice only allows weakening: `own → mut → imm`
    // note: upgrading `imm` to `mut` would violate immutability guarantee
    // help: if you need mutation, declare the original binding as `mut`
}
```

**E4006-01: Use of Moved Value**

```cursive
record Resource { handle: i32 }

function example()
    uses alloc.heap
{
    let resource = Resource { handle: 42 }
    let taken = move resource
    println("Handle: {}", resource.handle)
    // ERROR E4006-01: use of moved value `resource`
    // note: value moved here: `let taken = move resource`
    // note: move transfers ownership, invalidating original binding
    // help: use `taken.handle` instead, or avoid moving `resource`
}
```

**E4006-02: Value Used After Partial Move**

```cursive
record Container {
    data: Vec<i32>,
    metadata: string,
    count: i32
}

function example()
    uses alloc.heap
{
    let container = Container {
        data: Vec::from([1, 2, 3]),
        metadata: string::from("example"),
        count: 3
    }

    let extracted = move container.data

    println("Container: {:?}", container)
    // ERROR E4006-02: value used after partial move
    // note: field `data` was moved here: `move container.data`
    // note: cannot use `container` as a whole after partial move
    // help: access non-moved fields individually: `container.count`, `container.metadata`
}
```

**E4006-05: Move in Loop Body**

```cursive
record Item { value: i32 }

function example()
    uses alloc.heap
{
    let items = [Item { value: 1 }, Item { value: 2 }, Item { value: 3 }]

    loop item in items {
        let owned = move item
        process(owned)
    }
    // ERROR E4006-05: cannot move value in loop iteration
    // note: loop attempts to move `item` on each iteration
    // note: after first iteration, `item` would be moved and unavailable
    // help: if `Item` should be Copy, derive Copy trait
    // help: or use Copy type for elements to allow iteration without consumption
    // help: or consume array: use iterator that takes ownership
}
```

**E4014-01: Region-Allocated Value Escapes via Return**

```cursive
function create_region_string(): string
    uses alloc.region
{
    region {
        let s = string::from_region("Hello, region!")
        result s
        // ERROR E4014-01: region-allocated value escapes region
        // note: `s` is allocated in region scope
        // note: region deallocates when scope exits
        // note: returning `s` would create dangling value
        // help: allocate on heap instead: `string::from("Hello, heap!")`
        // help: or return value before region ends
    }
}
```

**E4015-01: Closure Captures Escaping Region Value**

```cursive
function create_closure(): () → string
    uses alloc.region, alloc.heap
{
    region {
        let s = string::from_region("Hello")
        result || -> s
        // ERROR E4015-01: closure captures region value that escapes scope
        // note: closure captures `s` which is region-allocated
        // note: region ends but closure could be invoked later
        // note: this would access deallocated memory
        // help: allocate `s` on heap: `string::from("Hello")`
        // help: or use closure within region scope only
    }
}
```

**E3D10-01: Assignment to Immutable Binding**

```cursive
function example() {
    let x = 10
    x = 20
    // ERROR E3D10-01: cannot assign to immutable binding `x`
    // note: binding `x` declared with `let` (immutable)
    // note: reassignment requires `mut` binding
    // help: declare as mutable: `let mut x = 10`
}
```

**E4401-01: Missing `result` Keyword in Block Expression**

```cursive
function compute(x: i32): i32 {
    if x > 0 {
        x * 2
    } else {
        0
    }
    // ERROR E4401-01: missing `result` keyword in block expression
    // note: block-valued expressions require explicit `result`
    // note: this ensures ownership transfer is explicit
    // help: add `result` to final expression in each block:
    //       if x > 0 { result x * 2 } else { result 0 }
}
```

**E4402-01: Missing Type Annotation on Match Binding**

```cursive
function process(opt: Option<Vec<i32>>)
    uses alloc.heap
{
    opt match {
        Option::Some(vec) => {
            // ERROR E4402-01: missing type annotation on match binding `vec`
            // note: cannot infer permission for binding `vec`
            // note: `vec` could be `own Vec<i32>`, `mut Vec<i32>`, or `imm Vec<i32>`
            // help: add explicit type: `Option::Some(vec: own Vec<i32>) => ...`
            // help: or use permission annotation: `Option::Some(own vec) => ...`
            vec::push(42)
        },
        Option::None => { }
    }
}
```

**E4403-01: Missing Type Annotation on Pipeline Stage**

```cursive
function process(values: [i32]): [i32]
    uses alloc.heap
{
    result values
        => (|v| -> filter_positive(v))
        => (|v| -> double(v))
        // ERROR E4403-01: missing type annotation on pipeline stage
        // note: cannot infer return type of `double` function
        // note: permission context unclear without annotation
        // help: add explicit type annotation: `=> (|v: [i32]| -> double(v))`
        // help: or annotate intermediate binding
}
```

**W4002: Permission Unnecessarily Restrictive**

```cursive
function calculate_sum(data: own Vec<i32>): i32 {
    var sum = 0
    loop item in data {
        sum = sum + item
    }
    result sum
    // WARNING W4002: permission unnecessarily restrictive
    // note: parameter `data` requires `own Vec<i32>`
    // note: function only reads from `data`, never mutates or moves
    // help: consider using `imm Vec<i32>` instead
    // help: this allows callers to pass with weaker permissions
}
```

**W4007: Closure Captures More Than Needed**

```cursive
function example()
    uses alloc.heap
{
    var counter = 0

    let incrementer = move || -> {
        counter = counter + 1
        // WARNING W4007: closure captures `counter` by move unnecessarily
        // note: closure moves `counter` but could capture with `mut` permission
        // note: move prevents outer scope from accessing `counter`
        // help: remove `move` keyword to capture with imm permission
    }

    incrementer()
}
```

**E4020-03: Contract Precondition Permission Violation**

```cursive
procedure Buffer::write(self: mut Buffer, data: [u8])
    uses alloc.heap
    must self::capacity() >= data::len()
{
    // ...
}

function example() {
    let buf: imm Buffer = Buffer::with_capacity(10)
    buf::write([1, 2, 3])
    // ERROR E4020-03: contract precondition requires unavailable permission
    // note: precondition `must self::capacity() >= data::len()` cannot be checked
    // note: procedure `write` requires `mut Buffer` receiver
    // note: receiver `buf` has `imm Buffer` permission
    // help: declare buffer as mutable: `let mut buf: mut Buffer = ...`
}
```

---

### 4.15.9 Cross-Reference to Other Chapters

| Error Code | Primary Definition | Related Sections |
|------------|-------------------|------------------|
| **E3D09** | Region value escapes via binding/return | Part III §3.9.2; §4.13 |
| **E3D10** | Assignment to immutable binding | Part III §3.2.8; §4.3 |
| **E4003** | Permission violation (insufficient permission) | §4.3, §4.5, §4.9 |
| **E4006** | Use of moved value | §4.4, §4.7, §4.11 |
| **E4014** | Region allocation escapes region | §4.13; Part III §3.9 |
| **E4015** | Closure captures region value that escapes | §4.10, §4.13 |
| **E4401** | Missing `result` keyword in block expression | §4.2.4; Part VI §6.6 |
| **E4402 / E6402** | Missing type annotation on match binding | §4.8; Part VI §6.8 |
| **E4403** | Missing type annotation on pipeline stage | §4.5.9; Part V §5.7 |
| **E4404 / E6404** | Missing type annotation on loop iterator binding | §4.9.3; Part VI §6.7 |
| **E4020** | Advanced permission errors (contracts, modals, effects) | §4.Y; Parts VII, X |
| **E6009** | Panic forbidden inside `[[no_panic]]` block/function | Part VI §6.14 |
| **W4xxx** | Permission warnings | All sections |

Each diagnostic entry references its authoritative rule; this chapter provides the permission-specific context and rationale.

---

## 4.16 Inter-Part Cross-Reference Table

| Topic | This Part | Supporting Parts |
|-------|-----------|------------------|
| Permission wrappers & syntax | §4.2 | Part II §2.0.5 |
| Binding vs value mutability | §4.3 | Part III §§3.2.2–3.2.4 |
| Permission judgments | §4.5 | Part V §§4.3–4.4 |
| Coercion lattice | §4.6 | Part II §2.1.6 |
| Move semantics | §4.7 | Part V §4.4.5 |
| Value categories | §4.8 | Part V §5.0.4 |
| Parameters & self | §4.9 | Part III §3.5.2; Part V §4.3.2 |
| Closures & captures | §4.10 | Part V §4.10 |
| Control-flow integration | §4.11 | Part V §4.9; Part VI §§6.6–6.11 |
| Data structures | §4.12 | Part II §§2.2–2.4 |
| Region rules | §4.13 | Part III §3.9; Part VIII (Regions) |
| Effects & unsafe | §4.14 | Part V §4.4.3; Part VII (Contracts and Effects) |
| Diagnostics | §4.15 | Foundations §8; Parts III–VII |

---

## Appendix 4.A – Reference Tables (Informative)

### 4.A.1 Permission Coercion Summary

| From | To | Implicit? | Notes |
|------|----|-----------|-------|
| `own T` | `mut T` | Yes | Drops exclusive ownership but retains mutability. |
| `own T` | `imm T` | Yes | Produces read-only view. |
| `mut T` | `imm T` | Yes | Drops mutation capability. |
| `imm T` | `mut T` | No | Requires an API that produces a mutable handle (e.g., procedure returning `mut`). |
| `mut T` | `own T` | No | Requires move or reconstruction. |

### 4.A.2 Standard Library Permission Notes (Informative)

| Type | Default Permission | Notable Operations |
|------|--------------------|--------------------|
| `Vec<T>` | `own` | `mut Vec<T>` enables push/pop; slicing yields `imm` views. |
| `Option<T>` | Mirrors payload | `Option::Some` consumes payload permissions. |
| `Result<T, E>` | Mirrors active payload | Pattern matches must respect payload permissions. |
| `string` | Parameters: `string@View`; returns/fields: `string@Owned` | `.to_owned()` performs explicit widening requiring `alloc.heap`. |

---

**Document Status:** Normative. Further clarifications introduced as Parts VI and XII are published must be coordinated through this file to preserve the single source of truth for permissions.
