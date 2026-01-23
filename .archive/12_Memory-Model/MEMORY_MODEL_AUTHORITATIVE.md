# CURSIVE MEMORY MODEL SPECIFICATION
## Authoritative Design Document (Single-Threaded)

**Version**: 1.0 Draft
**Date**: 2025-11-03
**Status**: Design Authority - To Be Formalized
**Scope**: Single-threaded memory model (excludes concurrency)

---

## EXECUTIVE SUMMARY

This document is the authoritative specification for Cursive's single-threaded memory model. It defines how memory is managed, how ownership works, how permissions control access, and how the language achieves memory safety without garbage collection or a borrow checker.

**Core Achievement**: Memory safety through explicit ownership, orthogonal permissions, and compile-time verification with zero runtime overhead.

**Key Design Decisions**:
1. Two-axis orthogonal design: 3 binding categories × 3 permissions = 9 combinations
2. Explicit transfer semantics via `move` keyword
3. Optional copy semantics via `Copy` predicate
4. Region allocation via `^` operator with lexical scope inference
5. Field-level partitioning for safe `shared` permission usage
6. No interior mutability (const means const)
7. No borrow checker (no lifetime annotations)
8. Deterministic RAII-based cleanup

---

## PART I: FOUNDATIONAL CONCEPTS

### 1. Design Philosophy

Cursive's memory model is built on six core principles:

#### 1.1 Explicit Over Implicit

**Principle**: All ownership transfers, permission changes, and resource management decisions must be explicitly visible in code.

**Manifestations**:
- Transfer requires `move` keyword at call site
- Copy semantics provided via `Copy` predicate
- Region allocation requires `^` operator at allocation site
- Permissions must be declared (defaults to const if omitted)
- Partition directives explicit in record definitions
- Binding categories required in local declarations

**Anti-patterns avoided**:
- No implicit copying of expensive types
- No hidden ownership transfer
- No automatic reference counting
- No invisible memory allocations

**Examples**:
```cursive
// Explicit transfer
let x = Data::new();
consume(move x);  // Transfer is visible

// ERROR: Implicit transfer forbidden
consume(x);  // Must use 'move'

// Explicit copy
let a: i32 = 42;
let b = copy a;  // Copy is visible

// Explicit permission
let data: unique = Data::new();  // Permission declared
```

#### 1.2 Local Reasoning

**Principle**: Understanding code behavior should require minimal global context.

**Manifestations**:
- Function signature reveals ownership behavior
- Type annotation reveals permission level
- `let`/`var`/`ref` reveals cleanup responsibility
- No non-local lifetime analysis needed
- Partition assignments visible in record definition

**Anti-patterns avoided**:
- No flow-dependent move validity (rejected: move from var when safe)
- No hidden aliasing
- No distant effects on validity
- No non-local borrowing relationships

**Design Decision**: We explicitly rejected allowing `move` from `var` bindings even when provably safe after the move, because determining safety requires whole-function analysis. This violates local reasoning - the legality of `move x` should not depend on code 100 lines below.

#### 1.3 Zero Runtime Overhead

**Principle**: Memory safety mechanisms have no runtime cost.

**Manifestations**:
- All permission checking at compile-time
- Field-level partitioning compiled away (zero runtime state)
- Move tracking is static analysis only
- No reference counting
- No garbage collection
- No runtime type tags for modals (zero-cost abstraction)
- Sum types have discriminants only when needed for pattern matching

**Verification**:
Generated assembly for safe code is identical to unsafe code performing the same operations. The compiler adds no checks, metadata, or instrumentation.

#### 1.4 No Borrow Checker

**Principle**: Achieve memory safety without borrow checking or lifetime annotations.

**Explicitly Rejected**:
- Lifetime parameters (`'a` style annotations)
- Borrowing semantics (lending and returning)
- Fractional permissions (splitting unique into multiple const)
- Borrow tokens
- Disjoint borrows
- Any form of temporary lending

**Alternatives Used**:
- Explicit `move` for ownership transfer
- Permissions (const/unique/shared) for aliasing control
- Regions for scoped allocation
- Partitioning for shared safety
- `<-` operator for non-owning bindings (reference of value assignment)

**Rationale**: Borrow checking requires complex lifetime tracking and non-local reasoning. Cursive achieves memory safety through simpler mechanisms that maintain local reasoning.

#### 1.5 Deterministic Cleanup

**Principle**: Resources are released at predictable, statically determined points.

**Manifestations**:
- RAII: destructors run at scope exit
- LIFO order: reverse declaration order
- Lexical scoping determines lifetime
- No deferred cleanup
- No finalization
- No garbage collection pauses

**Contrast with GC**: Unlike garbage-collected languages where cleanup timing is unpredictable, Cursive's cleanup points are statically determined by program structure.

#### 1.6 Orthogonal Design

**Principle**: Separate concerns should be independently controllable.

**Manifestations**:
- Binding categories (who cleans up) independent of permissions (what access allowed)
- 3 × 3 design: no special cases, all combinations valid
- Pointer types orthogonal to binding categories
- Permissions orthogonal to storage duration
- No fourth binding category (rejected proposal)

**Design Decision**: We explicitly rejected adding a fourth binding category (`ptr` for non-responsible + rebindable) because it would break the clean 3×3 orthogonality. Existing mechanisms suffice.

---

### 2. Two-Axis Orthogonal Design

Cursive's memory model separates two independent concerns:

**AXIS 1: Binding Categories** - Who is responsible for calling the destructor?
- `let`: Responsible, can transfer via move
- `var`: Responsible, cannot transfer

**AXIS 2: Permissions** - What operations are allowed?
- `const`: Read-only, unlimited aliasing
- `unique`: Exclusive mutable, no aliasing
- `shared`: Shared mutable, programmer coordinates

These axes are **completely orthogonal**: every combination of binding category and permission is valid and meaningful.

**Result**: 2 × 3 = **6 valid combinations**, each with distinct semantics.

```
               const          unique         shared
         ┌─────────────┬──────────────┬──────────────┐
   let   │ Immutable   │ Exclusive    │ Shared       │
         │ owned       │ mutable      │ mutable      │
         │             │ owned        │ owned        │
         ├─────────────┼──────────────┼──────────────┤
   var   │ Immutable   │ Exclusive    │ Shared       │
         │ owned       │ mutable      │ mutable      │
         │ rebindable  │ rebindable   │ rebindable   │
         └─────────────┴──────────────┴──────────────┘
```

#### 2.1 Why Two Axes?

**Historical Problem**: In C++, ownership and mutability are conflated. A `const&` doesn't own but can't mutate. A value owns and can mutate. No way to express "owns but immutable" or "doesn't own but can mutate" cleanly.

**Rust's Problem**: Ownership and borrowing create complex interactions. `&mut T` means "temporary exclusive borrow" conflating permission with lifetime.

**Cursive's Solution**: Separate "who owns" (binding category) from "how accessed" (permission).

**Examples of Independence**:
```cursive
// Owning binding (=), Permission: const (immutable)
let x: const = Data::new();
// Will call destructor, but cannot mutate through x

// Reference of value assignment (<-), Permission: unique (mutable)
let y: unique <- some_data;
// Non-owning binding via <-, can mutate, won't call destructor

// Owning binding (=), Rebindable (var), Permission: const (immutable)
var state: const = State::Initial;
// Will call destructor, can rebind, but cannot mutate
```

These are genuinely independent concerns that deserve separate expression.

---

### 2.5. Orthogonality Clarification: Pointer Types vs Binding System

**Important Distinction**: The binding system (rebindability + ownership) is NOT part of the type system. Bindings are orthogonal to types.

#### 2.5.1 Binding System Is NOT Part of Types

The binding system controls **binding behavior** (ownership, transferability, rebindability), not the **type** of the value:

```cursive
// These all have type i32
let x: i32 = 42;     // Owning binding of i32
var y: i32 = 42;     // Rebindable owning binding of i32
let z: i32 <- x;     // Non-owning binding via <- (reference of value)
var w: i32 <- x;     // Rebindable non-owning binding via <-
```

The binding form (`let`/`var` + `=`/`<-`) is part of the **declaration syntax**, not part of the **type**.

**Note**: `<-` is the "reference of value" assignment operator. It creates a non-owning binding to an existing value.

#### 2.5.2 Pointer Types ARE Types

Cursive has separate pointer TYPES that are part of the type system:

```cursive
// Pointer TYPES:
Ptr<T>@State     // Modal state-tracked pointer
*T               // Raw immutable pointer (unsafe)
*mut T           // Raw mutable pointer (unsafe)
```

These are actual types that can be stored in fields, passed as parameters, and returned from functions.

#### 2.5.3 Orthogonal Composition

Binding categories and pointer types compose orthogonally:

```cursive
// Binding form × Pointer type:
let p: Ptr<i32>@Valid = Ptr::new(&x);      // Owning binding of Ptr type
var p: Ptr<i32>@Valid = Ptr::new(&x);      // Rebindable owning binding of Ptr type
let p: Ptr<i32>@Valid <- some_pointer;     // Reference binding of Ptr type

// Binding form × Permission × Pointer type:
let p: const Ptr<i32>@Valid = ...;         // Immutable pointer (can't reassign)
let p: unique Ptr<i32>@Valid = ...;        // Exclusive pointer
let p: const Ptr<i32>@Valid <- ...;        // Non-owning reference to pointer
```

#### 2.5.4 Common Confusion: Reference Bindings vs Pointers

**Reference bindings (`<-`) are NOT pointer types**. They're a binding form:

```cursive
// Reference binding - NOT a pointer type
let x: i32 <- some_value;   // x is a non-owning binding to an i32
                            // Type is still i32, not a pointer

// Pointer type - actual pointer value
let p: Ptr<i32>@Valid = Ptr::new(&value);  // p's type is Ptr<i32>@Valid
```

**Key difference**:
- `let x: i32 <- value` - x IS an i32 (accessed through a non-owning binding)
- `let p: Ptr<i32>@Valid = ...` - p IS a pointer (a value containing an address)

#### 2.5.5 Why This Matters

This orthogonality enables precise control:

```cursive
// Scenario: Temporary reference to a pointer owned elsewhere
record Connection {
    socket_ptr: Ptr<Socket>@Connected,  // Field owns the pointer
}

procedure use_socket(conn: const Connection) {
    // ref binding to access field without ownership
    ref ptr: const Ptr<Socket>@Connected = conn.socket_ptr;
    // ptr is a non-owning const binding to a Ptr<Socket>@Connected value
    //
    // Binding: ref (doesn't own)
    // Permission: const (can't modify the pointer itself)
    // Type: Ptr<Socket>@Connected (state-tracked pointer)
}
```

**Summary Table**:

| Concept | Category | Examples | What It Controls |
|---------|----------|----------|------------------|
| **Binding Categories** | Declaration syntax | `let`, `var` | Ownership, transferability, rebindability |
| **Assignment Operators** | Syntax | `=`, `<-` | Value assignment vs reference of value |
| **Permissions** | Type qualifier | `const`, `unique`, `shared` | Mutability, aliasing rules |
| **Pointer Types** | Actual types | `Ptr<T>@S`, `*T`, `*mut T` | Memory indirection, dereferencing |

**Takeaway**: `let` and `var` are binding categories that control rebindability and transferability. Combined with assignment operators (`=` or `<-`), they control whether a binding owns a value or creates a non-owning reference.

---

### 3. Memory Safety Without GC or Borrow Checker

**How Cursive achieves memory safety**:

#### 3.1 Use-After-Free Prevention

**Mechanism 1: Move Invalidation**
```cursive
let x = Data::new();
consume(move x);
// x is now invalid - compiler prevents any use
x.method();  // ERROR: use of moved value
```

**Mechanism 2: RAII Cleanup**
```cursive
{
    let x = Data::new();
    // Use x...
}  // x.drop() called automatically
// x no longer in scope, cannot be accessed
```

**Mechanism 3: Region Escape Prevention**
```cursive
region r {
    let data = ^Data::new();
    result data;  // ERROR: Cannot escape region
}
```

#### 3.2 Double-Free Prevention

**Mechanism 1: Single Owner (let)**
```cursive
let x = Data::new();  // x owns the data
let y = move x;  // Ownership transferred to y, x invalidated
// Only y will call destructor (exactly once)
```

**Mechanism 2: Non-Transferable var**
```cursive
var x = Data::new();
// Cannot move from var, so cannot create multiple owners
let y = move x;  // ERROR: cannot transfer from var
```

**Mechanism 3: Non-Owning Reference Binding**
```cursive
let owner = Data::new();
let viewer <- owner;
// viewer doesn't own, won't call destructor
// Only owner calls destructor
```

#### 3.3 Dangling Reference Prevention

**Mechanism: Lexical Regions, Not Lifetimes**

Region-allocated data cannot escape its region:
```cursive
region r {
    let data = ^Data::new();
    result data;  // ERROR: Cannot escape region
}
```

Region references have lexical lifetime:
```cursive
region r {
    let data = ^Data::new();
    let ptr: &r Data = data;
    // ptr valid only within region r
}
// ptr no longer accessible here
```

No lifetime annotations needed - lexical scoping is sufficient.

##### 3.3.4 Escape Analysis for First-Class Pointers

**Mechanism**: `Ptr<T>` escape analysis with provenance tracking.

Pointers created from region-allocated data are tracked at compile-time:

```cursive
region r {
    let data = ^Data::new();           // Region allocation
    let ptr: Ptr<Data> = &data;        // Create pointer

    // Compiler tracks: ptr -> region r

    result ptr;  // ERROR E4027: Pointer provenance is region r, cannot escape
}
```

**Provenance Tracking**:

Every `Ptr<T>` has compile-time provenance metadata:

```
Provenance ::= Stack | Region(RegionId) | Heap
```

**Escape Analysis Rules**:

1. **Provenance Tracking**: Each `Ptr<T>` tracks where its target is allocated
2. **Region Boundary**: Pointers to region-allocated data cannot escape region
3. **Heap Conversion**: Use `.to_heap()` to convert before creating escapable pointer
4. **No Runtime Overhead**: Pure static analysis, zero runtime cost

**Valid Escape via Heap**:

```cursive
region r {
    let data = ^Data::new();
    let heap_data = data.to_heap();     // Convert to heap first
    let ptr: Ptr<Data> = &heap_data;    // Pointer now heap-backed
    result ptr;  // ✅ OK: heap-backed pointer can escape
}
```

**Provenance Propagation**:

```
[Address-Of-Provenance]
Γ ⊢ value: T
storage_of(value) = provenance
────────────────────────────────────
provenance(&value) = provenance
```

**Escape Check**:

```
[Ptr-Escape-Check]
Γ ⊢ result ptr: Ptr<T>
provenance(ptr) = Region(r)
r is local to current function
────────────────────────────────────
ERROR E4027: Cannot escape region-backed pointer
```

**Implementation Requirement**:

Conforming implementations MUST:
- Track pointer provenance at compile-time
- Reject programs where region-backed pointers escape
- Provide clear diagnostics identifying escape violations
- Implement analysis with zero runtime overhead

#### 3.4 Aliasing Hazard Prevention

**Mechanism 1: Unique Permission (Compile-Time)**
```cursive
let data: unique = Data::new();
let a: unique <- data;  // Locks unique access
let b: unique <- data;  // ERROR: unique already accessed
```

**Mechanism 2: Field-Level Partitioning (Compile-Time)**
```cursive
record World {
    positions: [Position; 1000],  // Fixed-size array
}

let world: shared = World::new();
let p1: shared <- world.positions;
let p2: shared <- world.positions;  // ERROR: Partition already active
```

Prevents accessing same field twice through `shared`, avoiding iterator invalidation bugs.

---

### 4. Terminology and Conventions

#### 4.1 Key Terms

**Binding**: Association between a name and a value
**Owner**: Binding responsible for calling destructor
**Permission**: Level of access (const/unique/shared)
**Transfer**: Passing ownership via `move`
**Consume**: Taking ownership (invalidating source)
**Partition**: Grouping of fields for access control
**Coercion**: Automatic permission downgrade (unique→const, shared→const)
**RAII**: Resource Acquisition Is Initialization (automatic cleanup)
**LIFO**: Last In, First Out (destruction order)

#### 4.1.5 Important: Bindings ARE References, Values MOVE

**Fundamental Distinction**: In Cursive, ALL bindings are references to objects. However, ownership of values transfers by default (move semantics).

**Bindings are references** means:
```cursive
let x: i32 = 42;
// x is a binding (a name) that refers to an i32 object
// The binding itself is conceptually a reference to the object
// But you access the value directly through x
```

**Values move by default** means:
```cursive
let x = Data::new();
let y = move x;  // Ownership transfers from x to y
// x is now invalid (moved-from state)
```

**Why this matters - avoiding confusion**:

❌ **Common Misconception**: "Bindings are references" does NOT mean:
- Rust-style `&T` references
- Values are copied by default
- Only reference bindings (`<-`) refer to data

✅ **Correct Understanding**: "Bindings are references" means:
- The binding mechanism: names refer to objects
- But ownership semantics: values transfer via move
- The binding IS how you access the object

**Diagram**:
```
Memory:        [Data object at address 0x1000]
                      ↑
Binding:          x ──┘
                (x "refers to" or "binds to" the object)

After move:
let y = move x;

Memory:        [Data object at address 0x1000]
                      ↑
Binding:          y ──┘
                (ownership transferred, x is invalid)
```

**Key Takeaway**: Don't confuse the binding mechanism (bindings refer to objects) with the transfer semantics (values move by default). These are orthogonal concepts:
- **Binding mechanism**: How names relate to objects
- **Transfer semantics**: How ownership changes

#### 4.2 Syntax Conventions

**Binding System** (orthogonal axes):
```cursive
// Rebindability × Ownership
let x = ...     // Non-rebindable owning: owns, can transfer
var x = ...     // Rebindable owning: owns, can rebind
let x <- ...    // Non-rebindable reference: doesn't own
var x <- ...    // Rebindable reference: doesn't own, can rebind
```

**Permissions** (optional, defaults to const):
```cursive
let x: const = ...    // Explicit const
let x: unique = ...   // Explicit unique
let x: shared = ...   // Explicit shared
let x = ...           // Defaults to const
```

**Transfer** (explicit move keyword required):
```cursive
consume(move x)       // Explicit transfer
consume(x)            // ERROR: must use move
```

**Copy** (explicit copy keyword):
```cursive
let y = copy x        // Explicit copy (requires Copy predicate)
let y = x.clone()     // Explicit clone (requires Clone predicate)
```

**Partitions** (in record definitions):
```cursive
record World {
    entities: [Entity; 1000],  // Default: World::entities partition

    <<Physics>>
    positions: [Position; 1000],  // Physics partition
    velocities: [Velocity; 1000], // Physics partition
}
```

---

## PART II: BINDING CATEGORIES (AXIS 1)

### 5. Overview of Binding Categories

Binding categories answer the question: **Who is responsible for calling the destructor?**

Two binding categories exist (`let` and `var`), combined with two assignment operators (`=` for owning, `<-` for reference binding):

| Binding Form | Ownership | Rebindable | Transferable via move |
|--------------|-----------|------------|----------------------|
| `let x = ...`    | YES (owns)  | NO         | YES                  |
| `var x = ...`    | YES (owns)  | YES        | NO                   |
| `let x <- ...`   | NO (reference)    | NO         | NO                   |
| `var x <- ...`   | NO (reference)    | YES        | NO                   |

**Key Insight**: Responsibility for cleanup is independent from mutability and aliasing (which are controlled by permissions). The `<-` operator creates a reference binding (non-owning), not a third binding category.

---

### 6. `let` - Responsible, Non-Rebindable, Transferable

**Definition**: A `let` binding creates an owned value. The binding is responsible for calling the destructor at scope exit. The binding cannot be reassigned but can transfer ownership via `move`.

#### 6.1 Syntax

```cursive
let identifier: permission Type = expression
```

Permission is optional and defaults to `const`.

#### 6.2 Semantics

**Cleanup Responsibility**: At scope exit, the destructor is called:
```cursive
{
    let x = Resource::new();
    // Use x...
}  // x.drop() called automatically
```

**Non-Rebindable**: Cannot reassign to let binding:
```cursive
let x = Data::new();
x = Data::new();  // ERROR: cannot reassign to let binding
```

**Transferable**: Can move ownership to another binding:
```cursive
let x = Data::new();
let y = move x;  // Ownership transferred to y
// x is now invalid
// y is now responsible for cleanup
```

#### 6.3 Use Cases

**Most Common**: Majority of local bindings should use `let`:
```cursive
let file = File::open("data.txt");
let data = file.read();
let processed = process(data);
```

**Resource Management**: Ideal for RAII resources:
```cursive
let socket = Socket::connect(addr);
let lock = mutex.lock();
let allocation = allocate(1024);
// All automatically cleaned up at scope exit
```

**Owned Values**: When you want exclusive ownership:
```cursive
let config = Config::load();
let state = State::new();
```

---

### 7. `var` - Responsible, Rebindable, Non-Transferable

**Definition**: A `var` binding creates an owned value that can be reassigned. The binding is responsible for cleanup but cannot transfer ownership.

#### 7.1 Syntax

```cursive
var identifier: permission Type = expression
```

#### 7.2 Semantics

**Cleanup Responsibility**: Same as `let`, destructor called at scope exit:
```cursive
{
    var x = Resource::new();
    // Use x...
}  // x.drop() called automatically
```

**Rebindable**: Can reassign to var binding:
```cursive
var x = Data::new();
x = Data::new();  // OK: rebinding allowed
// Old value dropped, x now owns new value
```

**Rebinding Sequence**:
1. Call destructor on old value
2. Assign new value to binding
3. Binding now owns new value

**Non-Transferable**: Cannot move from var:
```cursive
var x = Data::new();
consume(move x);  // ERROR: cannot transfer from var
```

**Rationale for Non-Transferability**: Allowing move from `var` would require whole-function analysis to ensure the binding isn't used after the move. This violates local reasoning. We explicitly rejected this complexity.

#### 7.3 Use Cases

**Loop Counters and Accumulators**:
```cursive
var sum = 0;
var i = 0;

loop i < 10 {
    sum = sum + i;
    i = i + 1;
}
```

**State Machines**:
```cursive
var state = ConnectionState::Disconnected;

loop {
    match state {
        ConnectionState::Disconnected => {
            state = ConnectionState::Connecting;
        },
        ConnectionState::Connecting => {
            state = try_connect();
        },
        ConnectionState::Connected => {
            break;
        }
    }
}
```

**Rebinding Values**:
```cursive
var current_config = Config::default();

if user_config_exists() {
    current_config = Config::load_user();  // Rebind
}
```

#### 7.4 Pattern: When You Need Both Rebinding and Transfer

**Problem**: `var` can rebind but can't transfer. `let` can transfer but can't rebind.

**Solution**: Rebind to `let` first:
```cursive
var builder = Builder::new();

loop item in items {
    builder.add(item);
}

// Want to consume builder, but var can't move
// Solution: Create a let binding
let final_builder = builder;  // Move to let binding
result final_builder.build();  // Now can consume
```

Wait - this doesn't work! `var` is non-transferable.

**Actual Solution**: Use `let` with unique permission:
```cursive
let builder: unique = Builder::new();
// Can mutate via unique permission
// Can transfer when done

loop item in items {
    builder.add(item);
}

result builder.build();  // Consumes builder
```

**Or**: Design API to not require consumption:
```cursive
var builder = Builder::new();

loop item in items {
    builder.add(item);
}

result builder.build_in_place();  // Takes unique ref, not ownership
```

---

### 8. Reference Bindings (`<-`) - Non-Owning, Non-Rebindable (when `let`), Non-Transferable

**Definition**: A reference binding (`let x <- value` or `var x <- value`) creates a non-owning reference to a value. The binding is not responsible for cleanup and cannot be transferred. Rebindability depends on using `let` (non-rebindable) or `var` (rebindable).

#### 8.1 Syntax

```cursive
let identifier: permission Type <- expression   // Non-rebindable reference
var identifier: permission Type <- expression   // Rebindable reference
```

#### 8.2 Semantics

**Non-Owning**: Does NOT call destructor:
```cursive
let owner = Resource::new();
{
    let viewer <- owner;
    // Use viewer...
}  // viewer.drop() NOT called
// owner still valid
```

**Stays Valid After Owner Moves**:
```cursive
let owner = Data::new();
let viewer <- owner;

consume(move owner);  // owner moved

// viewer is STILL VALID!
viewer.read();  // OK: viewer still references the data
```

This is safe because:
1. Data is NOT destroyed (consume() now owns it)
2. viewer is just a reference, not an ownership claim
3. At most one responsible binding exists at any time

**Non-Rebindable (when using `let`)**: Cannot reassign:
```cursive
let x <- data1;
x = data2;  // ERROR: cannot reassign to let binding
```

**Rebindable (when using `var`)**: Can reassign:
```cursive
var x <- data1;
x = data2;  // OK: var allows rebinding to different reference
```

**Non-Transferable**: Cannot move:
```cursive
let x <- data;
consume(move x);  // ERROR: cannot transfer from reference binding
```

#### 8.3 Function Parameters and Ownership

Function parameters are non-owning by default (similar to reference bindings):
```cursive
// Default: non-owning parameter
procedure process(data: const Data) { }

// Explicit ownership transfer with `move`
procedure consume(move data: const Data) { }
```

This makes sense: most functions don't want to take ownership.

**Note**: Parameter passing semantics are conceptually similar to reference bindings but use distinct syntax. See Part IV for complete parameter passing specification.

#### 8.4 Use Cases

**Local Reference Bindings** (most common):
```cursive
procedure inspect(data: const Data) {
    // Create local reference to parameter
    let local_ref <- data;
    // Read-only access, no ownership
}
```

**Temporary Views**:
```cursive
let collection = load_data();

{
    let item <- collection.items[0];
    analyze(item);
}

// collection still valid, can continue using
```

**Multiple References to Same Data**:
```cursive
let data = load_data();

let view1 <- data;
let view2 <- data;
let view3 <- data;

// All three references valid simultaneously
// None will call destructor
```

---

### 9. Default Binding Categories

#### 9.1 Local Declarations: REQUIRED

Binding form is **required** for local variable declarations:
```cursive
x = Data::new();  // ERROR: must specify binding form

let x = Data::new();   // OK: owning binding
var x = Data::new();   // OK: rebindable owning binding
let x <- data;         // OK: reference binding
```

**Rationale**: Explicit ownership clarity. No ambiguity about who cleans up.

#### 9.2 Function Parameters: Non-Owning by Default

```cursive
// Non-owning parameter (default):
procedure process(data: Data) { }

// To take ownership, use move:
procedure consume(move data: Data) { }
```

**Note**: See Section 8.3 for details on function parameter semantics.

#### 9.3 Record Fields: N/A

Record fields don't have binding categories. The record owns all its fields.

```cursive
record Container {
    data: Data,  // Container owns this
}

// When Container drops, it drops all fields
```

---

## PART III: PERMISSIONS (AXIS 2)

### 10. Overview of Permissions

Permissions answer the question: **What operations are allowed, and how is aliasing controlled?**

Three permissions exist:

| Permission | Mutability | Aliasing | Enforcement |
|------------|-----------|----------|-------------|
| `const`    | Immutable | Allowed  | Type system |
| `unique`   | Mutable   | Forbidden | Compiler |
| `shared`   | Mutable   | Allowed  | Programmer |

**Permission Lattice** (coercion allowed downward only):
```
unique ──┐
         ├──→ const
shared ──┘
```

**Key Principle**: Permissions control aliasing and mutability, NOT ownership.

---

### 11. `const` - Immutable, Unlimited Aliasing

**Definition**: `const` permission provides read-only access. Multiple `const` references can coexist. No mutation is allowed.

#### 11.1 Semantics

**Immutable Access**:
```cursive
let x: const = Data::new();
x.mutate();  // ERROR: cannot mutate through const
```

**Unlimited Aliasing**:
```cursive
let data: const = Data::new();

let a: const <- data;  // OK
let b: const <- data;  // OK
let c: const <- data;  // OK

// All can read simultaneously
a.read();
b.read();
c.read();
```

**No Coordination Needed**: Multiple const aliases are always safe - no mutation means no conflicts.

#### 11.2 Default Permission

`const` is the **default permission** in all contexts:
```cursive
let x = Data::new();           // Defaults to const
procedure process(d: Data) {}  // Defaults to const
```

**Rationale**: Immutability by default is safer. Mutation must be explicit.

#### 11.3 Const Means Const

**Design Decision**: We explicitly rejected interior mutability. If something is `const`, it is **actually immutable**.

**No Exceptions**: No `Cell<T>`, no `RefCell<T>`, no magic mutation through const.

**Rationale**: Interior mutability violates "explicit over implicit". If a type says const but mutates internally, the signature lies.

**Alternative for Statistics/Caching**:
```cursive
// Don't hide mutation in const
// Instead: separate mutable state
record Cache {
    data: [Data; 100],   // Fixed-size array
    valid: [bool; 100],  // Which entries are valid
}

record CacheStats {
    hit_count: i32,
    miss_count: i32,
}

procedure lookup(
    cache: const Cache,
    stats: unique CacheStats,  // Explicit mutable stats
    index: usize
): bool {  // Returns true if found
    if index < 100 && cache.valid[index] {
        stats.hit_count = stats.hit_count + 1;  // Explicit mutation
        result true
    } else {
        stats.miss_count = stats.miss_count + 1;
        result false
    }
}
```

#### 11.4 Readers/Writer Pattern Limitation

**Design Note**: Cursive's permission system does NOT directly support the readers/writer pattern (multiple readers OR one writer) at the language level.

**The Problem**: Traditional readers/writer locks allow:
- Multiple concurrent readers (const-like access), OR
- One exclusive writer (unique-like access)
- But NOT both simultaneously

**Why Not Built-In?**: Implementing this at the language level would require:
1. Runtime tracking of reader/writer counts (violates zero-overhead principle)
2. Complex borrow-checker-style lifetime analysis (explicitly rejected)
3. Potential deadlocks requiring language-level prevention

**Chosen Solution (Option D)**: Accept this limitation as inherent to the zero-overhead permission model. Provide readers/writer functionality through explicit stdlib primitives:

```cursive
// Standard library types (out of scope for this memory model spec)
// RwLock<T> - readers/writer lock with runtime checking
// Mutex<T>  - exclusive mutual exclusion
// Atomic<T> - lock-free atomic operations
```

**Usage Pattern**:
```cursive
// Instead of language-level readers/writer:
record SharedData {
    cache: unique RwLock<Cache>,  // Stdlib type
}

procedure read_cache(data: const SharedData): CacheEntry {
    let guard = data.cache.read_lock();   // Blocks if writer active
    result guard.lookup(key)
    // Lock released when guard drops
}

procedure write_cache(data: const SharedData, entry: CacheEntry) {
    let guard = data.cache.write_lock();  // Blocks if readers/writer active
    guard.insert(entry);
    // Lock released when guard drops
}
```

**Rationale**: This approach:
- Keeps the core permission system simple and zero-overhead
- Makes synchronization costs explicit (lock/unlock operations are visible)
- Delegates complex concurrency to stdlib where runtime overhead is expected
- Maintains "explicit over implicit" principle

**Note**: Concurrent programming and stdlib synchronization primitives are out of scope for this single-threaded memory model specification. See the full language specification for concurrency details.

---

### 12. `unique` - Exclusive Mutable, Compiler Enforced

**Definition**: `unique` permission provides exclusive mutable access. Only one `unique` reference can exist at a time. The compiler enforces this exclusivity.

#### 12.1 Semantics

**Exclusive Mutable Access**:
```cursive
let data: unique = Data::new();

data.mutate();  // OK: Can mutate

let second: unique <- data;  // ERROR: Already have unique access
```

**Compiler Enforcement**: The compiler tracks active unique references and prevents creating multiple:
```cursive
let data: unique = Data::new();

{
    let writer: unique <- data;  // Locks unique access
    writer.mutate();
}  // Lock released

// Now can access again
data.mutate();  // OK
```

**Prevents Iterator Invalidation**:
```cursive
record Collection {
    items: [i32; 100],
    count: usize,
}

let coll: unique = Collection::new();
coll.add(1);
coll.add(2);

let iter <- coll.items;     // Creates const reference to array
coll.add(3);                // ERROR: Cannot mutate while iter active
```

#### 12.2 Temporary Downgrade to Const

Can temporarily create const references from unique:
```cursive
let data: unique = Data::new();

{
    let reader: const <- data;  // Downgrade to const

    reader.read();   // OK
    data.mutate();   // ERROR: Cannot mutate while const reference active
}

// After const reference goes out of scope
data.mutate();  // OK: unique access restored
```

#### 12.3 Use Cases

**Most Mutable Operations**: Default choice for mutation:
```cursive
record Buffer {
    data: [i32; 100],
    len: usize,
}

let buffer: unique = Buffer::new();
buffer.append(1);
buffer.append(2);
```

**Preventing Aliasing Bugs**:
```cursive
let config: unique = Config::new();

// Cannot accidentally create multiple mutable refs
// Compiler enforces single mutable access
```

**Safe Default for Mutation**: When in doubt, use `unique` over `shared`.

---

### 13. `shared` - Shared Mutable, Programmer Coordinated

**Definition**: `shared` permission allows multiple mutable references to coexist. The programmer is responsible for coordinating access to prevent conflicts. The compiler does NOT prevent aliasing with shared.

#### 13.1 Semantics

**Shared Mutable Access**:
```cursive
let data: shared = Data::new();

let a: shared <- data;  // OK
let b: shared <- data;  // OK

a.mutate();  // OK: Programmer coordinates
b.mutate();  // OK: Programmer coordinates
```

**Works for ANY Type** (not just synchronized types):
```cursive
// Regular types:
record List {
    items: [i32; 100],
    count: usize,
}

let list: shared = List::new();
let a <- list;
let b <- list;  // OK with shared permission

// Custom types:
record World {
    entities: [Entity; 1000],
}

let world: shared = World::new();
// Multiple systems can access with shared permission
```

**NO Compiler Enforcement**: Unlike `unique`, the compiler does NOT prevent creating multiple `shared` aliases.

#### 13.2 Programmer Coordination

**Responsibility**: The programmer must ensure aliased `shared` bindings don't cause conflicts.

**Safe Patterns**:
```cursive
// Pattern 1: Different indices
let world: shared = World::new();

procedure update_entity(world: shared World, entity_id: EntityId) {
    world.entities[entity_id].update();
}

// Multiple systems accessing different entities: SAFE
update_entity(world, 0);
update_entity(world, 1);  // Different entity, safe
```

**Unsafe Patterns**:
```cursive
// Pattern 2: Same index (programmer must prevent)
update_entity(world, 0);
update_entity(world, 0);  // Same entity, PROGRAMMER must prevent
```

#### 13.3 Field-Level Partitioning Safety

Field-level partitioning prevents accessing the **same field** twice, but doesn't prevent **element-level** conflicts:

**Prevents**:
```cursive
record World {
    positions: [Position; 1000],
}

let world: shared = World::new();
let p1: shared <- world.positions;
let p2: shared <- world.positions;  // ERROR: Partition active
```

**Doesn't Prevent** (programmer responsibility):
```cursive
let positions: shared <- world.positions;

let pos1 <- positions[0];
let pos2 <- positions[0];  // Same element - programmer must coordinate
```

**Design Decision**: Field-level partitioning is sufficient. Element-level tracking would require borrow checker complexity, which we explicitly reject.

#### 13.4 Use Cases

**Game Development (ECS)**:
```cursive
record World {
    <<Physics>>
    velocities: shared [Velocity; 1000],
    forces: shared [Force; 1000],

    <<Rendering>>
    meshes: shared [Mesh; 1000],
    materials: shared [Material; 1000],
}

// Multiple systems access shared world
// Different partitions = safe
// Same partition = programmer coordinates via entity IDs
```

**Complex Graphs**:
```cursive
record Graph {
    nodes: shared [Node; 500],
    edges: shared [Edge; 1000],
}

// Multiple traversal algorithms can run
// Programmer ensures no conflicts via node/edge IDs
```

**When to Use**: Only when you genuinely need multiple mutable references. Default to `unique` when possible.

---

### 14. Default Permissions

**All Contexts**: Permission defaults to `const`:
```cursive
let x = Data::new();           // x: const Data
procedure process(d: Data) {}  // d: const Data
record Container { d: Data }   // d: const Data
```

**Rationale**: Immutability by default. Mutation must be explicit.

---

### 15. Permission Coercion

#### 15.1 Permission Lattice

```
unique ──┐
         ├──→ const
shared ──┘
```

**Weakening Only**: Permissions can be downgraded (unique→const, shared→const), never upgraded.

#### 15.2 Automatic Coercion

**Function Arguments**:
```cursive
procedure read_data(data: const Data) { }

let data: unique = Data::new();
read_data(data);  // Automatic coercion unique → const
```

**Reference Binding**:
```cursive
let data: unique = Data::new();
let reader: const <- data;  // Automatic coercion unique → const
```

#### 15.3 Forbidden Coercions

**Cannot Upgrade**:
```cursive
let data: const = Data::new();
let writer: unique <- data;  // ERROR: Cannot upgrade const → unique
```

**Cannot Cross Between unique and shared**:
```cursive
let data: unique = Data::new();
let shared_alias: shared <- data;  // ERROR: Cannot coerce unique → shared

let data: shared = Data::new();
let exclusive: unique <- data;  // ERROR: Cannot coerce shared → unique
```

#### 15.4 Complete Coercion Rules

**Valid**:
- `unique` → `const` (automatic)
- `shared` → `const` (automatic)

**Invalid**:
- `const` → `unique` (never)
- `const` → `shared` (never)
- `unique` → `shared` (never)
- `shared` → `unique` (never)
- `const` → anything (never)

To get stronger permission: create new value with desired permission.

---

## PART IV: THE NINE COMBINATIONS

### 16. Complete Matrix

Every combination of binding category × permission is valid:

| Binding | const | unique | shared |
|---------|-------|--------|--------|
| **let** | Immutable owned value | Exclusive mutable owned value | Shared mutable owned value |
| **var** | Immutable rebindable owned value | Exclusive mutable rebindable owned value | Shared mutable rebindable owned value |
| **ref** | Immutable reference | Exclusive mutable reference | Shared mutable reference |

### 17. Use Cases and Patterns

#### 17.1 `let + const`
**Most Common Pattern**: Immutable owned value
```cursive
let config = Config::load();
let data = file.read();
```

#### 17.2 `let + unique`
**Mutable Owned Value**: Exclusive mutable access, can transfer
```cursive
record Buffer {
    data: [i32; 100],
    len: usize,
}

let buffer: unique = Buffer::new();
buffer.append(1);
buffer.append(2);
consume(move buffer);  // Transfer ownership
```

#### 17.3 `let + shared`
**Shared Mutable Owned**: Multiple mutable refs, owned
```cursive
let world: shared = World::new();
system1.run(world);
system2.run(world);
// world still owned by this binding
```

#### 17.4 `var + const`
**Rebindable Immutable**: State machine with immutable states
```cursive
var state: const = State::Initial;

loop {
    match state {
        State::Initial => state = State::Processing,
        State::Processing => state = State::Complete,
        State::Complete => break,
    }
}
```

#### 17.5 `var + unique`
**Rebindable Mutable**: Accumulator that can be reset
```cursive
var buffer: unique = Buffer::new();

loop chunk in chunks {
    buffer.write(chunk);

    if buffer.full() {
        flush(buffer);
        buffer = Buffer::new();  // Rebind
    }
}
```

#### 17.6 `var + shared`
**Rebindable Shared Mutable**: Shared state that gets swapped
```cursive
var active_world: shared = World::new();

loop {
    update_systems(active_world);

    if should_swap() {
        active_world = World::new();  // Swap to new world
    }
}
```

#### 17.7 `ref + const`
**Immutable Reference** (default for parameters):
```cursive
procedure inspect(data: Data) {  // ref data: const Data
    println("{}", data);
}
```

#### 17.8 `ref + unique`
**Exclusive Mutable Reference**: Temporary mutation without ownership
```cursive
procedure increment(ref counter: unique Counter) {
    counter.value = counter.value + 1;
}

let counter: unique = Counter { value: 0 };
increment(counter);  // counter still valid after call
```

#### 17.9 `ref + shared`
**Shared Mutable Reference**: Game dev, ECS patterns
```cursive
procedure physics_system(ref world: shared World) {
    ref velocities = world.velocities;
    ref positions = world.positions;

    // Update physics...
}
```

---

## PART V: TRANSFER SEMANTICS

### 18. The `move` Keyword

**Purpose**: Explicit ownership transfer from one binding to another.

**Syntax**: `move <binding>`

**Requirement**: Only valid for `let` bindings (responsible + transferable).

#### 18.1 Call Site Requirement

Transfer MUST be explicit at call site:
```cursive
procedure consume(let data: Data) { }

let x = Data::new();
consume(move x);  // OK: Explicit move

consume(x);  // ERROR: Missing 'move' keyword
```

**Rationale**: Explicit transfer makes ownership flow visible.

---

### 19. Transfer Rules

#### 19.1 Only `let` Can Transfer

```cursive
let x = Data::new();
consume(move x);  // OK: let is transferable

var y = Data::new();
consume(move y);  // ERROR: var is not transferable

let z = Data::new();
let w <- z;
consume(move w);  // ERROR: reference binding is not transferable
```

#### 19.2 Moved Bindings Become Invalid

```cursive
let x = Data::new();
consume(move x);  // x transferred

x.method();  // ERROR: use of moved value
```

#### 19.3 Reference Bindings Stay Valid After Owner Moves

```cursive
let owner = Data::new();
let viewer <- owner;

consume(move owner);  // owner moved

viewer.read();  // OK: viewer still valid!
```

This is safe because:
- `viewer` doesn't own, won't call destructor
- Data still exists (now owned by `consume`)
- No double-free possible

---

### 20. Move with Control Flow

**Design Decision**: We choose **Simple Rule** over flow-sensitive analysis.

#### 20.1 Simple Rule (CHOSEN)

**Rule**: Once `move` appears for a binding, that binding is invalid for the remainder of the enclosing function, regardless of control flow.

```cursive
let x = Data::new();

if condition {
    consume(move x);
}

x.use();  // ERROR: x may have been moved
```

Even though `x` is only moved in one branch, the compiler considers it potentially moved and rejects all subsequent uses.

**Rationale**: Maintains local reasoning. Don't need to trace all control flow paths to determine if a move happened.

**Alternative Patterns**:

**Pattern 1: Early Return**
```cursive
let x = Data::new();

if condition {
    consume(move x);
    return;  // Exit function
}

x.use();  // OK: Only reached if not moved
```

**Pattern 2: Handle Both Branches**
```cursive
let final_value = if condition {
    let x = Data::new();
    process(move x)
} else {
    let x = Data::new();
    x
};

// Use final_value
```

**Pattern 3: Result Type**
```cursive
enum DataState {
    Valid { data: Data },
    Consumed,
}

var state = DataState::Valid { data: Data::new() };

if condition {
    match state {
        DataState::Valid { data } => {
            consume(move data);
            state = DataState::Consumed;
        },
        DataState::Consumed => { }
    }
}

// Later: pattern match on state
match state {
    DataState::Valid { data } => data.use(),
    DataState::Consumed => println("Already consumed")
}
```

#### 20.2 Rejected: Flow-Sensitive Analysis

We explicitly **rejected** flow-sensitive move tracking where:
```cursive
let x = Data::new();
if condition {
    consume(move x);
    return;  // Compiler knows this path exits
}
x.use();  // Would be OK in flow-sensitive analysis
```

**Rejection Rationale**:
- Requires whole-function control-flow analysis
- Violates local reasoning principle
- Complex error messages ("may be moved on some paths")
- Inconsistent with design goals

---

### 21. Definite Initialization

**Design Decision**: Support definite initialization analysis (distinct from move tracking).

#### 21.1 Uninitialized Declarations

Can declare variables without immediate initialization:
```cursive
let x: Data;  // Declared but uninitialized

if condition {
    x = Data::new();
} else {
    x = other_data();
}

x.use();  // OK: Definitely initialized on all paths
```

#### 21.2 Rules

**Must Initialize Before Use**:
```cursive
let x: Data;

if condition {
    x = Data::new();
}
// x NOT initialized on else path

x.use();  // ERROR: Not definitely initialized
```

**All Paths Must Initialize**:
```cursive
let x: Data;

if condition {
    x = Data::new();
} else {
    x = other_data();
}
// Both paths initialize x

x.use();  // OK
```

#### 21.3 Distinction from Move Tracking

**Definite initialization** (allowed): Tracking whether value has been assigned
**Flow-sensitive moves** (rejected): Tracking whether value has been moved

These are different analyses. We support the former, reject the latter.

---

### 22. Moved-From State

#### 22.1 After Move: Invalid

Once a binding is moved, it cannot be used:
```cursive
let x = Data::new();
consume(move x);

// x is in "moved-from" state: completely invalid
x.method();  // ERROR
```

#### 22.2 Cannot Reinitialize `let`

`let` bindings cannot be reassigned:
```cursive
let x = Data::new();
consume(move x);
x = Data::new();  // ERROR: cannot reassign to let
```

#### 22.3 Can Reinitialize `var`

Wait - `var` cannot be moved in the first place!

```cursive
var x = Data::new();
consume(move x);  // ERROR: cannot move from var
```

So this is moot.

---

## PART V.5: DESTRUCTURING AND PATTERN-BASED TRANSFER

### 22.5. Destructuring Semantics

**Principle**: Destructuring a value via pattern matching moves all fields atomically.

#### 22.5.1 Atomic Field Movement

When destructuring a record or tuple, ALL fields are moved together in a single atomic operation:

```cursive
record Pair {
    first: Data,
    second: Data,
}

let pair = Pair { first: Data::new(), second: Data::new() };

// Destructuring moves BOTH fields atomically
let Pair { first, second } = pair;
// pair is now invalid (moved-from state)
// first and second are valid let bindings
```

**Key Property**: The original binding (`pair`) becomes invalid immediately, and all destructured bindings (`first`, `second`) become valid simultaneously.

#### 22.5.2 Partial Destructuring Not Allowed

Cursive does NOT support partial moves via destructuring:

```cursive
record Triple {
    a: Data,
    b: Data,
    c: Data,
}

let triple = Triple { a: Data::new(), b: Data::new(), c: Data::new() };

// ERROR: Cannot partially destructure
let Triple { a, b, .. } = triple;  // Not supported
```

**Rationale**: Partial destructuring would create partially-moved values, complicating the ownership model. All-or-nothing semantics are simpler and more predictable.

#### 22.5.3 Destructuring with match

Pattern matching in `match` expressions follows the same atomic semantics:

```cursive
enum State {
    Active { data: Data, id: i32 },
    Inactive,
}

let state = State::Active { data: Data::new(), id: 42 };

match state {
    State::Active { data, id } => {
        // data and id are moved here
        // state is now invalid
        use_data(data);
        use_id(id);
    },
    State::Inactive => { }
}
// state cannot be used here (moved in Active branch)
```

#### 22.5.4 Destructuring in Function Parameters

Function parameters can destructure arguments:

```cursive
record Point {
    x: f64,
    y: f64,
}

procedure process_point(Point { x, y }: Point) {
    // x and y are available as separate bindings
    println("x: {}, y: {}", x, y);
}

let p = Point { x: 1.0, y: 2.0 };
process_point(move p);  // p is moved and destructured
// p is now invalid
```

#### 22.5.5 Destructuring Does Not Bypass Move

Destructuring still requires `move` for transfer contexts:

```cursive
let pair = Pair { first: Data::new(), second: Data::new() };

procedure consume_pair(Pair { first, second }: Pair) {
    // ...
}

consume_pair(move pair);  // Still need explicit move
// consume_pair(pair);     // ERROR: must use move
```

**Principle**: Destructuring changes HOW the value is bound, but does not change WHEN ownership transfers.

---

## PART VI: COPY SEMANTICS

### 23. The `Copy` Predicate

**Purpose**: Opt-in marker predicate indicating that bitwise copying creates a valid, independent value.

**Definition**:
```cursive
predicate Copy { }
```

`Copy` is a marker predicate with no methods. It signals to the compiler that implicit copying is safe.

#### 23.1 Relationship to Clone

`Copy` requires `Clone`:
```cursive
predicate Copy: Clone { }

predicate Clone {
    procedure clone(self: const Self): Self;
}
```

If a type is `Copy`, it must also implement `Clone`. The `clone()` method performs the same operation as implicit copy (bitwise copy).

---

### 24. The `copy` Keyword

**Syntax**: `copy <expression>`

**Semantics**: Creates a bitwise copy of the value. Requires that the type implements `Copy`.

```cursive
let x: i32 = 42;
let y = copy x;  // Creates copy, both x and y valid

// ERROR if type doesn't implement Copy:
let data = Data::new();
let copy_data = copy data;  // ERROR: Data doesn't implement Copy
```

**Explicit Requirement**: Copy must be explicit via `copy` keyword.

---

### 25. Copy vs Move

**Default Behavior**: All types use **move semantics** by default.

```cursive
let x: i32 = 42;
let y = x;  // MOVE: x becomes invalid (even for i32)

// To copy, use copy keyword:
let x: i32 = 42;
let y = copy x;  // COPY: both valid
```

**Design Decision**: Consistent behavior - all types move by default. Copy must be explicit.

---

### 26. Which Types Can Be Copy

#### 26.1 Primitive Types

All primitive types implement `Copy`:
- `i8`, `i16`, `i32`, `i64`, `i128`
- `u8`, `u16`, `u32`, `u64`, `u128`
- `f32`, `f64`
- `bool`
- `char`

#### 26.2 Rules for User Types

**Can Be Copy If**:
1. All fields are `Copy`
2. No custom `drop` procedure
3. Not a modal type
4. Type explicitly opts-in via `impl Copy`

**Cannot Be Copy If**:
1. Any field is not `Copy`
2. Has custom `drop` procedure
3. Is a modal type (modals are never Copy)
4. Explicitly marked `!Copy`

#### 26.3 Examples

**Can Be Copy**:
```cursive
record Point {
    x: f64,
    y: f64,
}

impl Copy for Point { }
impl Clone for Point {
    procedure clone(self: const Self): Point {
        result Point { x: self.x, y: self.y }
    }
}
```

**Cannot Be Copy**:
```cursive
record File {
    handle: FileHandle,
}

// ERROR: Cannot implement Copy
// Reason: File owns a resource (file handle)
// Copying would create two owners → double-close
```

---

### 27. Derive and Implementation

#### 27.1 Automatic Derivation

Can derive `Copy` if all fields are `Copy`:
```cursive
derive Copy, Clone
record Point {
    x: f64,
    y: f64,
}

// Compiler automatically generates:
// impl Copy for Point { }
// impl Clone for Point { ... }
```

#### 27.2 Manual Implementation

Can manually implement if needed:
```cursive
impl Copy for MyType { }

impl Clone for MyType {
    procedure clone(self: const Self): MyType {
        // Custom clone logic
    }
}
```

---

## PART VII: FIELD-LEVEL PARTITIONING

### 28. Motivation and Problem

**Problem**: The `shared` permission allows multiple mutable references, which can cause bugs:

```cursive
record World {
    positions: shared [Position; 1000],
}

let world: shared = World::new();
let p1: shared <- world.positions;
let p2: shared <- world.positions;  // Two mutable references!

// If positions were a growable container, modifications through p1
// could invalidate p2 - this is the iterator invalidation problem
```

**Traditional Solutions** (all rejected):
- Borrow checker (rejected: no borrow checker)
- Runtime tracking (rejected: zero overhead)
- Lifetime annotations (rejected: no lifetimes)

**Cursive Solution**: Field-level partitioning with compile-time enforcement.

---

### 29. Partition Directives

**Syntax**: `<<PartitionName>>`

**Placement**: Inside record body, before field declarations

```cursive
record World {
    // Default partitions
    entities: [Entity; 1000],     // Partition: World::entities
    time: f64,                     // Partition: World::time

    // Explicit partition
    <<Physics>>
    positions: [Position; 1000],   // Partition: Physics
    velocities: [Velocity; 1000],  // Partition: Physics

    <<Rendering>>
    meshes: [Mesh; 1000],          // Partition: Rendering
    materials: [Material; 1000],   // Partition: Rendering
}
```

**Reset to Default**: Use `<<_>>` to reset:
```cursive
record Example {
    <<Group>>
    field1: i32,
    field2: i32,

    <<_>>  // Reset to default
    field3: i32,  // Partition: Example::field3
}
```

---

### 30. Default Partitioning Behavior

**Rule**: In the absence of explicit partition directives, each field gets its own distinct partition.

**Partition Identifier**: `TypeName::field_name`

```cursive
record Point {
    x: f64,  // Partition: Point::x
    y: f64,  // Partition: Point::y
}

let p: shared = Point::new();
let px <- p.x;  // Activates Point::x partition
let py <- p.y;  // OK: Point::y is different partition
```

**Prevents Same-Field Double-Access**:
```cursive
let px1 <- p.x;  // Activates Point::x
let px2 <- p.x;  // ERROR: Point::x partition already active
```

---

### 31. Partition Scoping Rules

**Scope**: A partition directive applies to all fields following it, until:
1. Another partition directive is encountered, OR
2. The record body ends

```cursive
record World {
    entities: [Entity; 1000],  // Default: World::entities

    <<Physics>>                // Physics partition starts
    positions: [Position; 1000],  // Physics
    velocities: [Velocity; 1000], // Physics

    <<Combat>>                 // Combat partition starts (Physics ends)
    healths: [Health; 1000],    // Combat
}
```

**Merging**: Multiple directives with same name merge into one partition:
```cursive
record World {
    <<Input>>
    keyboard: KeyboardState,

    <<Rendering>>
    framebuffer: FrameBuffer,

    <<Input>>  // Returns to Input partition
    mouse: MouseState,  // Same partition as keyboard
}
```

---

### 32. Access Rules and Enforcement

#### 32.1 Partition Activation

Accessing a field through a `shared` reference activates that field's partition for the duration of the reference's lexical scope:

```cursive
record World {
    positions: [Position; 1000],  // Partition: World::positions
}

let world: shared = World::new();

{
    let pos: shared <- world.positions;  // Activates World::positions
    // World::positions partition ACTIVE in this scope
}  // pos out of scope, partition deactivated

let pos2: shared <- world.positions;  // OK: Not active anymore
```

#### 32.2 Partition Conflict Detection

Attempting to access a field whose partition is already active is a compile-time error:

```cursive
record World {
    <<Physics>>
    positions: [Position; 1000],
    velocities: [Velocity; 1000],
}

let world: shared = World::new();
let pos: shared <- world.positions;  // Activates Physics
let vel: shared <- world.velocities; // ERROR: Physics already active
```

#### 32.3 Partition Independence

Different partitions are independent - can access simultaneously:

```cursive
record World {
    <<Physics>>
    positions: [Position; 1000],

    <<Combat>>
    healths: [Health; 1000],
}

let world: shared = World::new();
let pos: shared <- world.positions;  // Activates Physics
let hp: shared <- world.healths;     // OK: Combat is separate
```

---

### 33. Integration with Permissions

**Only Applies to `shared`**: Partition checking only occurs for `shared` permission.

**`unique` Permission**: Already enforces exclusivity, no partitioning needed:
```cursive
let world: unique = World::new();
let pos: unique <- world.positions;  // Locks unique access
let vel: unique <- world.velocities; // ERROR: unique already accessed
// This is unique permission enforcement, not partitioning
```

**`const` Permission**: Read-only, no mutation concerns, no partitioning needed:
```cursive
let world: const = World::new();
let p1 = world.positions;  // OK
let p2 = world.positions;  // OK: const allows unlimited aliasing
```

**Summary**: Partitioning is specifically for preventing unsafe `shared` patterns.

---

## PART VIII: TYPE SYSTEM INTEGRATION

### 34. Permission Propagation

Permissions propagate through field access:

```cursive
let file: unique = File { path: "/data.txt", handle: h };
let path: unique String = file.path;  // unique propagates

let file: const = File { path: "/data.txt", handle: h };
let path: const String = file.path;  // const propagates
```

---

### 35. Field Access

Standard field access syntax:
```cursive
let p = Point { x: 1.0, y: 2.0 };
let x = p.x;  // Access field
```

With permissions:
```cursive
let p: unique = Point { x: 1.0, y: 2.0 };
p.x = 3.0;  // Mutate field (unique allows)

let p: const = Point { x: 1.0, y: 2.0 };
p.x = 3.0;  // ERROR: cannot mutate through const
```

---

### 36. Method Dispatch

Methods called using `::` syntax:
```cursive
record Buffer {
    data: [i32; 100],
    len: usize,
}

let buffer: unique = Buffer::new();
buffer::append(42);  // Call method

let text: string@Owned = string::from("hello");
text::append(" world");
```

---

### 37. Permission Coercion Rules (Complete)

**Automatic Coercions**:
1. `unique` → `const` in function arguments
2. `shared` → `const` in function arguments
3. `unique` → `const` in ref bindings
4. `shared` → `const` in ref bindings

**Forbidden Coercions**:
1. `const` → anything (never strengthen)
2. `unique` ↔ `shared` (never cross)

---

### 38. Type Formation

Types are formed with permission annotations:
```cursive
Data                      // Defaults to const
const Data                // Explicit const
unique Data               // Unique permission
shared Data               // Shared permission

unique [const Data; 100]  // Unique array of const items
const [unique Data; 100]  // Const array of unique items (each element exclusive)
```

---

## PART IX: OBJECT LIFETIME AND STORAGE

### 39. Storage Duration Categories

Cursive defines **four storage duration categories**:

| Category | Allocated | Deallocated | Use Case |
|----------|-----------|-------------|----------|
| **Static** | Before `main` | After `main` returns | Globals, constants |
| **Automatic** | On scope entry | On scope exit | Local variables |
| **Region** | At `^` operator | At region end (O(1) bulk) | Temporary dynamic allocations |
| **Heap** | Via `.to_heap()` | When owner drops | Data escaping regions |

**Primary vs Secondary**: Regions are the **primary** mechanism for dynamic allocation. Heap is **secondary** for data that must outlive its creating scope.

---

### 40. RAII (Resource Acquisition Is Initialization)

**Principle**: Resources are automatically released when values go out of scope.

```cursive
{
    let file = File::open("data.txt");  // Resource acquired
    // Use file...
}  // file.drop() called automatically - file closed
```

**No Manual Management**: No need for explicit `close()`, `free()`, or `delete`.

---

### 41. Destruction Order

**LIFO (Last In, First Out)**: Objects destroyed in reverse declaration order.

```cursive
{
    let a = Resource::new("A");  // Declared first
    let b = Resource::new("B");  // Declared second
    let c = Resource::new("C");  // Declared third
}
// Destruction order: c, then b, then a
```

**Rationale**: Ensures dependencies are respected (later declarations may depend on earlier ones).

---

### 42. Regions and Arena Allocation

**Purpose**: Regions provide arena-style allocation for temporary dynamic data with O(1) bulk deallocation.

#### 42.1 Basic Region Allocation

**Syntax**: Use the `^` (caret) unary operator to allocate in the lexically enclosing region:

```cursive
region r {
    let data = ^Data::new();  // Allocates in region r
    // Use data...
}
// All region allocations freed at once (O(1))
```

**Key Properties**:
- `^` is a unary prefix operator
- Allocates in the innermost lexically enclosing `region` scope
- No explicit region name needed (inferred from lexical scope)
- Allocation point is explicit and visible

#### 42.2 Stack Allocation (Default)

**Without region scope**: Values allocate on the stack by default (no operator needed):

```cursive
let x = Data::new();  // Stack allocation (automatic storage)
```

**With region scope**: Must use `^` to allocate in region:

```cursive
region r {
    let stack_data = Data::new();   // Still on stack
    let region_data = ^Data::new(); // In region r
}
```

#### 42.3 Nested Regions and Stacking

**Nested regions** allow hierarchical allocation:

```cursive
region outer {
    let a = ^Data::new();  // Allocates in 'outer'

    region inner {
        let b = ^Data::new();   // Allocates in 'inner'
        let c = ^^Data::new();  // Allocates in 'outer' (one level up)
    }
    // inner region deallocated here
}
// outer region deallocated here
```

**Stacking Property**: Multiple `^` operators specify allocation in outer regions:
- `^` - innermost region
- `^^` - one region level up
- `^^^` - two region levels up
- And so on...

**Desugaring**: Stacking is syntactic sugar:
```cursive
^^Data::new()
// Desugars to:
ralloc<outer_region>(Data::new())
```

**Error**: Stack count exceeds nesting depth:
```cursive
region outer {
    let x = ^^^Data::new();  // ERROR: Only one region in scope
}
```

#### 42.4 Escape Prevention

Region-allocated data **cannot escape** its creating region:

```cursive
region r {
    let data = ^Data::new();
    result data;  // ERROR: Cannot escape region r
}
```

**Lexical Scoping Guarantee**: The compiler enforces that region-allocated values do not outlive their region through lexical scope analysis. No lifetime annotations required.

**Why This Matters**: Prevents use-after-free when region is deallocated.

#### 42.5 Benefits

**Performance**:
- O(1) bulk deallocation (entire region freed at once)
- Cache-friendly allocation (sequential memory)
- No per-object deallocation overhead

**Safety**:
- Lexical scoping prevents escapes
- Compile-time enforcement
- Zero runtime overhead

**Use Cases**:
- Temporary data structures during computation
- Builder patterns
- Parsing and compilation phases
- Frame-based allocations (game loops, request handling)

```cursive
// Example: Parser temporary data
procedure parse_expression(source: const String): Expr
    sequent { [alloc::region] |- true => true }
{
    region parse {
        let tokens = ^tokenize(source);
        let ast = ^build_ast(tokens);
        result ast.to_heap()  // Escape via heap
    }
}
```

---

#### 42.6 Modal Pointers (`Ptr<T>`)

**Purpose**: Cursive provides `Ptr<T>` as a first-class, nullable, modal pointer type that integrates with the region system and permission model.

##### 42.6.1 Design Decisions

**Critical Distinction**: `^expr` allocates in region and returns the VALUE type (not a pointer):

```cursive
region r {
    let data: Data = ^Data::new();  // data is Data, not Ptr<Data>
    // data is allocated in region r but accessed as a value
}
```

**Key Properties**:
- `^expr` returns `T`, not `Ptr<T>`
- Region-allocated values accessed directly through bindings
- `Ptr<T>` is for explicit first-class pointers
- `Ptr::new(&expr)` creates first-class pointers from values

##### 42.6.2 Modal States

Cursive's `Ptr<T>` uses a modal type system with four states:

```cursive
modal Ptr<T> {
    @Null      // Null pointer (no valid address)
    @Valid     // Strong reference, points to valid memory
    @Weak      // Weak reference, doesn't prevent destruction
    @Expired   // Weak reference after target was destroyed
}
```

**State Semantics**:

| State | Meaning | Dereferenceable? | Reference Strength |
|-------|---------|------------------|-------------------|
| `@Null` | Explicit null pointer | ❌ No (compile error) | N/A |
| `@Valid` | Points to valid memory | ✅ Yes | Strong (keeps target alive) |
| `@Weak` | Weak reference | ❌ Must upgrade first | Weak (doesn't prevent destruction) |
| `@Expired` | Target was destroyed | ❌ No (target gone) | N/A |

##### 42.6.3 Creating Pointers

**Null Pointer**:
```cursive
let null_ptr: Ptr<i32> = Ptr::null();  // Ptr<i32>@Null
```

**Valid Pointer** (from address-of):
```cursive
let value: i32 = 42;
let valid_ptr: Ptr<i32> = &value;      // Ptr<i32>@Valid
```

**Key design**: The address-of operator `&expr` directly produces `Ptr<T>` (not an intermediate address value). Type declaration is required.

**Weak Reference** (from strong):
```cursive
let strong: Ptr<i32> = &value;         // Ptr<i32>@Valid
let weak: Ptr<i32>@Weak = strong.downgrade();  // Ptr<i32>@Weak
```

**Upgrade Weak to Strong**:
```cursive
match weak.upgrade() {
    @Valid => {
        let value = *weak;  // OK: refined to @Valid
    }
    @Null => {
        // Target was destroyed
    }
}
```

##### 42.6.4 Escape Analysis for Pointers

**Rule**: Region-backed pointers cannot escape their region.

```cursive
// ❌ ERROR: Cannot escape region
procedure returns_dangling(): Ptr<Data> {
    region r {
        let data = ^Data::new();
        let ptr: Ptr<Data> = &data;
        result ptr;  // ERROR E4027: Cannot escape region r
    }
}
```

**Enforcement**:
- Compile-time tracking of pointer provenance
- Lexical region scoping determines validity
- No runtime overhead (pure static analysis)

**Valid Pattern - Pointer to Heap Data**:
```cursive
procedure returns_valid(): Ptr<Data>
    sequent { [alloc::region, alloc::heap] |- true => true }
{
    region r {
        let data = ^Data::new();
        let heap_data = data.to_heap();  // Escape to heap first
        result &heap_data                // OK: heap-backed pointer
    }
}
```

##### 42.6.5 Why Four States?

**@Null and @Valid** are fundamental for nullable pointers.

**@Weak and @Expired** enable essential patterns impossible with only @Null/@Valid:

**Use Case 1: Parent-Child Trees**
```cursive
record TreeNode {
    value: i32,
    parent: Ptr<TreeNode>@Weak,        // Weak: breaks reference cycle
    children: Vec<Ptr<TreeNode>>,       // Strong: owns children
}
```

**Use Case 2: Observer Pattern**
```cursive
record Subject {
    value: i32,
    observers: Vec<Ptr<Observer>@Weak>,  // Weak: doesn't prevent observer destruction
}

procedure Subject.attach(self: unique Self, obs: Ptr<Observer>) {
    self.observers.push(obs.downgrade());  // Store weak reference
}
```

**Use Case 3: Cache**
```cursive
record Cache<K, V> {
    entries: HashMap<K, Ptr<V>@Weak>,  // Weak: doesn't prevent eviction
}
```

**Without weak references**, these patterns require:
- Unsafe manual lifecycle management
- Reference counting with cycle detection overhead
- External bookkeeping structures

##### 42.6.6 Pattern Matching and State Refinement

Pattern matching refines pointer state within branches:

```cursive
let ptr: Ptr<i32> = get_ptr();

match ptr {
    @Null => {
        println("Pointer is null");
    }
    @Valid => {
        let value = *ptr;  // ✅ Safe: refined to @Valid
        println("Value: {}", value);
    }
    @Weak => {
        // Must upgrade before use
        match ptr.upgrade() {
            @Valid => { /* use it */ }
            @Null => { /* expired */ }
        }
    }
    @Expired => {
        println("Weak reference expired");
    }
}
```

##### 42.6.7 Integration with Permissions

Pointers work with Cursive's permission system:

```cursive
// Immutable pointer
let ptr: Ptr<const Data> = &data;

// Unique mutable pointer
let ptr: Ptr<unique Data> = &data;

// Shared mutable pointer
let ptr: Ptr<shared Data> = &data;
```

**Combined with states**:
```cursive
// Nullable unique pointer
let ptr: Ptr<unique Data> = get_maybe_null();

match ptr {
    @Null => { /* handle null */ }
    @Valid => {
        (*ptr).mutate();  // ✅ OK: unique permission allows mutation
    }
}
```

##### 42.6.8 Complete Example: Region-Based Linked List

```cursive
record Node {
    value: i32,
    next: Ptr<Node>,
}

procedure build_list(): Ptr<Node>
    sequent { [alloc::region, alloc::heap] |- true => true }
{
    region temp {
        var node1 = ^Node { value: 1, next: Ptr::null() };
        var node2 = ^Node { value: 2, next: Ptr::null() };
        var node3 = ^Node { value: 3, next: Ptr::null() };

        // Link them
        node3.next = &node2;
        node2.next = &node1;

        let head <- node3;

        // Escape to heap before region ends
        let heap_list = head.to_heap();
        result &heap_list
    }
}
```

---

### 43. Heap Allocation (Escape Pattern)

**Philosophy**: Heap allocation is **secondary** to region allocation. Heap is used ONLY when data must outlive its creating scope.

#### 43.1 The Escape Pattern: `.to_heap()`

**When Data Must Outlive Creating Scope**: Use `.to_heap()` to move region data to heap:

```cursive
record Collection {
    items: [Item; 100],
    count: usize,
}

procedure build_collection(): Collection
    sequent { [alloc::region, alloc::heap] |- true => true }
{
    region temp {
        let collection = ^Collection::new();  // Allocate in region

        loop i in 0..100 {
            collection::add(Item::new(i));
        }

        // Escape to heap before region ends
        result collection.to_heap()
    }
    // temp region deallocated, but collection is now on heap
}
```

**Key Properties**:
- `.to_heap()` transfers ownership from region to heap
- Explicit escape point visible in code
- Requires `alloc::heap` grant in sequent context
- Original region binding becomes invalid

#### 43.2 No Direct Heap Allocation

**Design Decision**: Cursive does NOT provide direct heap allocation in safe code.

❌ **Not Available**:
```cursive
let data = alloc_heap(Data::new());  // Does not exist
let data = Box::new(Data::new());    // Does not exist
```

✅ **Use Region Then Escape**:
```cursive
region temp {
    let data = ^Data::new();
    result data.to_heap()
}
```

**Rationale**:
- Encourages region-first design
- Makes heap allocation explicit and intentional
- Heap allocation is a "last resort" for escaping data
- Maintains "explicit over implicit" principle

#### 43.3 One-Way Flow: Region → Heap Only

**Supported: Region to Heap**:
```cursive
region r {
    let data = ^Data::new();
    result data.to_heap()  // ✅ OK: Region → Heap
}
```

**Not Supported: Heap to Region**:
```cursive
let heap_data = create_heap_data();

region r {
    let moved = heap_data.to_region();  // ❌ Does not exist
}
```

**Rationale for One-Way Restriction**:
1. **Lifetime Mismatch**: Heap data has indefinite lifetime; region has lexical lifetime
2. **Allocator Mismatch**: Heap and region use different allocation strategies
3. **O(1) Guarantee**: Moving heap data to region would require individual tracking, breaking the O(1) bulk deallocation guarantee
4. **Unnecessary**: Can construct new data in region instead

#### 43.4 Heap Cleanup: RAII Ownership

**Question**: When does heap-allocated data get freed?

**Answer**: When the owning binding goes out of scope (standard RAII):

```cursive
{
    let data = {
        region temp {
            let temp_data = ^Data::new();
            result temp_data.to_heap()
        }
        // temp region deallocated
    };
    // data is now heap-allocated, owned by 'data' binding

    // Use data...
}
// data goes out of scope → data.drop() called → heap memory freed
```

**Ownership Semantics**:
- `.to_heap()` returns a heap-allocated value
- Ownership transfers to receiving binding (standard move semantics)
- Destructor (`drop`) called when owner goes out of scope
- Destructor implementation deallocates heap memory

**No Special Tracking**: Heap values use the same ownership model as stack values. The only difference is WHERE they're stored (heap vs stack).

#### 43.5 When to Use Heap vs Region

**Use Region When**:
- Data lifetime is bounded by a known scope
- Building temporary data structures
- Performance-critical paths (faster allocation/deallocation)
- Multiple allocations with bulk cleanup

**Use Heap (via escape) When**:
- Data must outlive creating scope
- Returning dynamically-allocated data from functions
- Unknown lifetime at allocation time
- Data shared across disparate scopes

**Example Comparison**:
```cursive
// REGION: Temporary builder pattern
procedure process_items(items: [Item; 100]): Summary
    sequent { [alloc::region] |- true => true }
{
    region work {
        let builder = ^SummaryBuilder::new();
        loop item in items {
            builder::add(item);
        }
        result builder::finalize()  // Returns stack value
    }
}

// HEAP: Data must escape
procedure load_config(): Config
    sequent { [alloc::region, alloc::heap] |- true => true }
{
    region temp {
        let config = ^parse_config_file();
        result config.to_heap()  // Must escape scope
    }
}
```

#### 43.6 Grant System Integration

Heap allocation requires explicit permission via grant system:

```cursive
procedure build_data(): Data
    sequent { [alloc::region, alloc::heap] |- true => true }  // Both grants required
{
    region temp {
        let data = ^Data::new();    // Requires: alloc::region
        result data.to_heap()       // Requires: alloc::heap
    }
}
```

**Grant Permissions**:
- `alloc.region` - Permission to use `^` operator
- `alloc.heap` - Permission to use `.to_heap()`
- Both are capabilities that must be explicitly declared

**Safety**: Grants make allocation visible in function signatures, maintaining "explicit over implicit" principle.

#### 43.7 Grammar Specification

**Unary `^` Operator** (region allocation):

```ebnf
UnaryExpr ::= ("^")+ PrimaryExpr
            | UnaryOp UnaryExpr
            | PostfixExpr

PrimaryExpr ::= Literal
              | Identifier
              | FunctionCall
              | "(" Expr ")"
              | ...
```

**Binary `^` Operator** (bitwise XOR):

```ebnf
BitwiseXorExpr ::= BitwiseAndExpr ("^" BitwiseAndExpr)*
```

**Disambiguation**: Position-based parsing:
- **Prefix position**: `^` is region allocation (unary)
- **Infix position**: `^` is bitwise XOR (binary)

**Precedence**:
- Unary `^` has high precedence (same as other unary operators: `!`, `-`, `~`)
- Binary `^` has lower precedence (bitwise operator level)

**Examples**:
```cursive
^Data::new()        // Unary: region allocation
a ^ b               // Binary: XOR
^(a ^ b)            // Both: region-allocate result of XOR
^^Data::new()       // Unary: allocate two levels up
a ^ ^Data::new()    // Syntax error: unexpected unary ^ after binary ^
```

**Stacking**: Multiple `^` operators in prefix position stack:
```cursive
^x      // Allocate in innermost region
^^x     // Allocate one level up
^^^x    // Allocate two levels up
```

**Formal Stacking Semantics**:
```
^^expr  ⟹  ralloc<parent_region>(expr)
^^^expr ⟹  ralloc<grandparent_region>(expr)
```

Where `parent_region` and `grandparent_region` are determined by lexical nesting analysis during compilation.

---

### 44. Static Storage

**Module-Level Declarations**: Have static storage duration.

```cursive
// Allocated before main, deallocated after main
let MODULE_CONSTANT: i32 = 100;
var GLOBAL_COUNTER: i32 = 0;
```

**Initialization Order**: Dependency order (topological sort).

---

## PART X: MEMORY LAYOUT

### 45. Object Representation

Objects stored as contiguous sequences of bytes containing field data.

**No Hidden Metadata**: No type tags, vtables, or other metadata (except for sum types that need discriminants for pattern matching).

---

### 46. Alignment and Padding

**Alignment**: Objects aligned to natural boundaries.

**Padding**: Inserted between fields to satisfy alignment requirements.

**Example**:
```cursive
record Example {
    a: i8,   // 1 byte
    b: i64,  // 8 bytes, requires 8-byte alignment
}

// Memory layout:
// [a: 1 byte][padding: 7 bytes][b: 8 bytes]
// Total size: 16 bytes
```

---

### 47. Size Calculation

**Size**: Sum of field sizes plus padding.

```cursive
sizeof(Example) = sizeof(i8) + padding + sizeof(i64)
                = 1 + 7 + 8
                = 16 bytes
```

---

### 48. Field Offsets

**Offset**: Distance from start of record to field.

```cursive
record Point {
    x: f64,  // Offset 0
    y: f64,  // Offset 8
}

// sizeof(Point) = 16 bytes
```

---

## PART XI: PREDICATE SYSTEM INTEGRATION

### 49. Drop Predicate

**Purpose**: Custom cleanup logic.

```cursive
// Note: In Cursive, predicates are called "predicates" for concrete code reuse
predicate Drop {
    procedure drop(self);
}
```

**Implementation**:
```cursive
record Resource {
    handle: FileHandle,
}

impl Drop for Resource {
    procedure drop(self) {
        close_handle(self.handle);
    }
}
```

**Automatic Call**: Called automatically at scope exit.

**Rules**:
- Cannot implement both `Copy` and `Drop`
- Drop called in LIFO order

---

### 50. Copy Predicate (Detailed)

**Definition**:
```cursive
predicate Copy: Clone { }
```

**Semantics**: Marker predicate - bitwise copy is valid.

**Requirements**:
- All fields must be `Copy`
- No custom `Drop`
- Not a modal type

---

### 51. Clone Predicate

**Definition**:
```cursive
predicate Clone {
    procedure clone(self: const Self): Self;
}
```

**Purpose**: Explicit copying.

**Implementation**:
```cursive
impl Clone for Point {
    procedure clone(self: const Self): Point {
        result Point { x: self.x, y: self.y }
    }
}
```

---

### 52. Default Predicate

**Definition**:
```cursive
predicate Default {
    procedure default(): Self;
}
```

**Purpose**: Default value construction.

**Example**:
```cursive
impl Default for i32 {
    procedure default(): i32 {
        result 0
    }
}
```

---

### 53. Other Core Predicates

**Eq** (Equality):
```cursive
predicate Eq {
    procedure eq(self: const Self, other: const Self): bool;
}
```

**Ord** (Ordering):
```cursive
predicate Ord: Eq {
    procedure cmp(self: const Self, other: const Self): Ordering;
}
```

**Hash** (Hashing):
```cursive
predicate Hash {
    procedure hash(self: const Self, hasher: unique Hasher);
}
```

---

## CONCLUSION

This document specifies Cursive's complete single-threaded memory model. Key achievements:

1. ✅ **Memory safety** without garbage collection
2. ✅ **Memory safety** without borrow checker
3. ✅ **Zero runtime overhead** - all checking at compile-time
4. ✅ **Explicit ownership** - clear who cleans up
5. ✅ **Orthogonal design** - 3×3 binding × permission matrix
6. ✅ **Local reasoning** - minimal global context needed
7. ✅ **Deterministic cleanup** - RAII with lexical scoping

**Next Steps**: Extend with concurrency model (threads, atomics, Send/Sync predicates).

---

**END OF MEMORY MODEL SPECIFICATION**
