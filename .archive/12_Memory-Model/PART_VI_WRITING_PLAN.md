# PART VI: MEMORY MODEL AND PERMISSIONS - Writing Plan

**Version**: 1.0
**Date**: 2025-11-03
**Status**: Approved Plan
**Critical Change**: Uses System 3 (Two-Axis Model) as canonical, superseding System 1 (imm/mut/own)
**Primary Source**: `CURSIVE_MEMORY_MODEL_SPECIFICATION.md` (v3.0, canonical)
**Validation**: Checked against `Proposed_Organization.md` and `Content_Location_Mapping.md`

---

## Mission

Write a complete, correct, ISO/ECMA-compliant formal specification for PART VI, using the **System 3 two-axis model** (binding categories × permissions) as the canonical memory model. Transform and formalize content from CURSIVE_MEMORY_MODEL_SPECIFICATION.md into publication-ready normative specification.

---

## Critical Decision: System 3 as Canonical

**IMPORTANT**: The Proposed_Organization.md references System 1 (imm/mut/own), but the recently completed CURSIVE_MEMORY_MODEL_SPECIFICATION.md uses **System 3** (const/unique/shared + let/var/ref binding categories) and declares itself canonical.

**Adopted approach**: Part VI will specify **System 3** as the normative memory model.

**Permission System Change**:
- ❌ **System 1 (deprecated)**: `imm < mut < own` (single axis)
- ✅ **System 3 (canonical)**: Two orthogonal axes:
  - **Binding Categories**: `let` / `var` / `ref` (cleanup responsibility)
  - **Permissions**: `const` / `unique` / `shared` (access control + aliasing)

**Section structure adaptation**: Sections 6.2-6.3 restructured to reflect two-axis model.

---

## Core Writing Principles

1. **TRANSFORM from CURSIVE_MEMORY_MODEL_SPECIFICATION.md** - Formalize comprehensive spec into ISO format
2. **ONE canonical example per concept** - Publication-quality demonstrations
3. **Complete normative precision** - Every requirement with shall/should/may
4. **Implementer-ready** - Compiler implementation guidance included
5. **Zero ambiguity** - Formal semantics for all memory operations
6. **Completeness over brevity** - Specification complete enough to build conforming implementation

---

## File Structure

All files in `Spec/06_Memory-Model-and-Permissions/` directory:

| File | Type | Subsections | Primary Source |
|------|------|-------------|----------------|
| `06-0_Memory-Model-Overview.md` | 🔄 TRANSFORM | 6.0.1-6.0.5 (5) | CURSIVE_MEMORY_MODEL_SPECIFICATION.md Part I |
| `06-1_Objects-Memory.md` | 🔄 SYNTHESIZE | 6.1.1-6.1.5 (5) | Part I + scattered refs + Part II §2.8 |
| `06-2_Binding-Categories.md` | ✨ NEW STRUCTURE | 6.2.1-6.2.4 (4) | CURSIVE_MEMORY_MODEL_SPECIFICATION.md Part II |
| `06-3_Permission-System.md` | ✨ NEW STRUCTURE | 6.3.1-6.3.7 (7) | CURSIVE_MEMORY_MODEL_SPECIFICATION.md Part III |
| `06-4_Transfer-Semantics.md` | 🔄 TRANSFORM | 6.4.1-6.4.6 (6) | CURSIVE_MEMORY_MODEL_SPECIFICATION.md Part V |
| `06-5_Regions-Lifetimes.md` | 🔄 TRANSFORM | 6.5.1-6.5.6 (6) | Part IX + 04_Lexical-Permissions.md |
| `06-6_Storage-Duration.md` | 🔄 EXPAND | 6.6.1-6.6.4 (4) | Part II §2.8 + CURSIVE_MEMORY_MODEL_SPECIFICATION.md |
| `06-7_Memory-Layout.md` | ⭐ NEW | 6.7.1-6.7.5 (5) | Design decisions + industry standards |
| `06-8_Aliasing-Uniqueness.md` | 🔄 FORMALIZE | 6.8.1-6.8.3 (3) | CURSIVE_MEMORY_MODEL_SPECIFICATION.md Part III + VIII |
| `06-9_Unsafe-Operations.md` | 🔄 TRANSFORM | 6.9.1-6.9.5 (5) | 04_Lexical-Permissions.md + Part VI considerations |

**Total**: 10 sections, 50 subsections

**Structural changes from Proposed_Organization.md**:
- Section 6.2 split into 6.2 (Binding Categories) + 6.3 (Permissions) to reflect two-axis model
- Section 6.3 (Move Semantics) becomes 6.4 (Transfer Semantics) with updated terminology
- Section 6.4 (Regions) becomes 6.5
- Sections 6.5-6.8 renumbered accordingly
- Section 6.9 expanded for Unsafe (previously 6.8)

---

## Section 6.0: Memory Model Overview 🔄 TRANSFORM

### Source
- CURSIVE_MEMORY_MODEL_SPECIFICATION.md Part I (Overview & Philosophy)
- `04_Lexical-Permissions.md` lines 9-50 (System 1 overview - adapt)

### Strategy
Transform comprehensive overview into formal ISO specification, emphasizing two-axis orthogonal model and design philosophy.

### Subsections

#### 6.0.1 Memory Safety Goals
- Memory safety without garbage collection
- Zero-cost abstractions principle
- Compile-time enforcement guarantees
- No runtime overhead from safety mechanisms
- **Formal property**: Type safety theorem (reference to formal semantics)
- Deterministic performance characteristics
- Predictable resource management

**Normative statements**:
- "A conforming implementation shall provide memory safety guarantees without runtime garbage collection."
- "Memory safety shall be enforced at compile time through the permission system and binding categories."

#### 6.0.2 Permission-Based Memory Management Philosophy
- **Core principle**: Two orthogonal axes (binding + permission)
- Explicit over implicit design
- Local reasoning principle
- Separation of concerns (cleanup vs access control)
- Reference semantics (all bindings are references)
- Immutability by default (`const` is default permission)
- No implicit copying (all copy operations explicit)

**Two-axis model formal definition**:
```
Memory Model = Binding Categories × Permissions

Binding Categories = {let, var, ref}
Permissions = {const, unique, shared}

Valid Combinations = 9 (all combinations valid)
```

**Design rationale section**:
- Why orthogonal axes improve expressiveness
- Comparison with single-axis models
- Game development pattern support

#### 6.0.3 No Garbage Collection
- Deterministic cleanup (RAII)
- Predictable performance characteristics
- LIFO destruction order guarantee
- Contrast with GC languages (Java, Go, Python)
- Performance comparison table (GC vs RAII)

**Normative statement**:
> "Cursive implementations shall not employ garbage collection. Resource cleanup is deterministic and occurs at statically determined program points."

**Cleanup timing guarantees**:
- Scope-based: destructors called at scope exit
- Order: LIFO (Last In, First Out)
- Region-based: O(1) bulk deallocation
- No non-deterministic pauses

#### 6.0.4 No Borrow Checker
- **Key distinction**: Not Rust
- No lifetime parameters (`'a` annotations)
- Region-based scoping instead of lifetime tracking
- Permission system vs borrow checker comparison
- Shared mutable access support (via `shared` permission)

**Comparison table**:

| Feature | Rust Borrow Checker | Cursive Permission System |
|---------|---------------------|---------------------------|
| Lifetime annotations | Required (`'a`) | Not needed (regions) |
| Shared mutable | Forbidden (XOR) | Allowed (`shared`) |
| Move keyword | Implicit | Explicit |
| Aliasing control | Enforced (exclusive) | Controlled (unique vs shared) |
| Game dev patterns | Difficult | Natural |

**Why no borrow checker**:
- Regions provide lexical scoping alternative
- `shared` permission enables intentional aliasing
- Simpler mental model for complex graphs
- LLM-friendly (fewer lifetime annotations)

#### 6.0.5 Relationship to Part II §2.8
- §2.8 provides conceptual overview
- Part VI provides formal normative specification
- Cross-reference table mapping concepts
- Forward references from Part II resolved here

**Cross-reference mapping table**:

| Part II Concept | Part VI Specification |
|-----------------|----------------------|
| Object Model (§2.8.1) | Objects and Memory (§6.1) |
| Memory Locations (§2.8.2) | Memory Locations (§6.1.2) |
| Object Lifetime (§2.8.3) | Lifetime Rules (§6.1.5) |
| Storage Duration (§2.8.4) | Storage Duration (§6.6) |
| Alignment (§2.8.5) | Memory Layout (§6.7) |

### Formal Elements Required
- Memory safety properties (formal statements)
- Two-axis model definition (orthogonal decomposition)
- Comparison table (GC vs Borrow Checker vs Cursive)
- Design principles enumeration
- Formal type safety guarantee

### Canonical Example
Single program demonstrating:
- Memory safety without GC
- Two-axis model in action (binding categories × permissions)
- Deterministic cleanup
- No lifetime annotations needed
- Multiple memory safety patterns (let/var/ref + const/unique/shared)

---

## Section 6.1: Objects and Memory Locations 🔄 SYNTHESIZE

### Source
- `02_Type-System.md` lines 974-1027 (regions), 1076-1154 (object lifetime basics)
- Part II §2.8.1-2.8.2 (conceptual overview)
- Industry standard object model definitions (C++, Rust)

### Strategy
Create comprehensive formal object model, synthesizing scattered content and adding missing formal definitions.

### Subsections

#### 6.1.1 Object Definition (Formal)
**Formal Definition 6.1**: An object is a region of storage with:
- Associated type T
- Lifetime L (from creation to destruction)
- Memory location M (address)
- Size S (in bytes, type-dependent)
- Alignment A (address constraint)

**Object as tuple**:
```
Object = (Type, Lifetime, Location, Size, Alignment)
```

**Properties**:
- Objects are created by declarations, allocations, or temporaries
- Objects have exactly one responsible binding OR are in a region
- Objects are destroyed deterministically
- Object state: initialized, moved-from, or destroyed

**Well-formedness**:
- Object must be initialized before use
- Object cannot be used after destruction
- Object cannot be used after transfer (move)

**Normative rules**:
- "An object shall be initialized before any operation is performed on it."
- "Use of uninitialized memory is undefined behavior [UB: B.2 #N]."
- "Use of an object after its lifetime has ended is undefined behavior [UB: B.2 #N]."

#### 6.1.2 Memory Location (Formal)
**Formal Definition 6.2**: A memory location is a sequence of one or more bytes with a unique address.

**Properties**:
- Scalar types occupy exactly one memory location
- Aggregate types may occupy multiple memory locations (one per field)
- Adjacent bit-fields may share memory locations (implementation-defined)
- Zero-sized types occupy no memory locations

**Memory location disjointness**:
- Two memory locations are either disjoint or identical
- Partial overlap is not permitted (except bit-fields)

**Aliasing definition**:
> "Two references alias if they refer to overlapping memory locations."

**Aliasing categories**:
- **Disjoint**: No overlap (safe concurrent access)
- **Identical**: Complete overlap (aliasing rules apply)
- **Partial**: Not permitted (except bit-fields)

**Normative rules**:
- "Two distinct objects shall occupy disjoint memory locations, except for subobjects and bit-fields."
- "Access to disjoint memory locations may occur concurrently without data races."

#### 6.1.3 Object Creation
**Creation mechanisms**:
1. **Variable declarations**: `let x = value` or `var x = value`
2. **Temporary expressions**: Intermediate values in expressions
3. **Region allocation**: `alloc_in<r>(value)`
4. **Heap allocation**: Via Box, Rc, Arc (library-provided)

**Formal semantics**:
```
[Object-Create-Let]
Γ ⊢ e : T
storage allocated for T
initialization of storage from e
──────────────────────────────────
Γ, x : let T ⊢ let x = e : ()
object created with responsible binding x

[Object-Create-Var]
Γ ⊢ e : T
storage allocated for T
initialization of storage from e
──────────────────────────────────
Γ, x : var T ⊢ var x = e : ()
object created with responsible binding x

[Object-Create-Region]
Γ; Δ ⊢ r active region
Γ ⊢ e : T
storage allocated in region r for T
initialization of storage from e
──────────────────────────────────
alloc_in<r>(e) creates object in region r

[Object-Create-Temporary]
Γ ⊢ e : T
e is not a variable
──────────────────────────────────
Temporary object created
Lifetime: until end of full expression (default)
Lifetime may be extended (see §6.1.5)
```

**Initialization requirement**:
- All objects must be fully initialized
- Partial initialization not permitted
- Initialization order: field declaration order for aggregates

**Normative rules**:
- "All objects shall be initialized before use."
- "Reading uninitialized memory is undefined behavior [UB]."
- "Initialization of aggregate types shall proceed in field declaration order."

#### 6.1.4 Object Destruction
**Destruction mechanisms**:
1. **Scope exit** (automatic objects - RAII)
2. **Explicit drop** (early destruction via `drop` function)
3. **Region end** (bulk deallocation)
4. **Rebinding** (`var` bindings destroy old value on reassignment)

**Destructor call semantics**:
- **Compiler privilege**: Compiler can call destructor regardless of user-visible permission
- **User code permission**: Determined by binding permission for explicit drop
- **LIFO order**: Destructors called in reverse declaration order within scope

**Formal semantics**:
```
[Object-Destroy-Scope-Exit]
scope S exits
object o with responsible binding b in S
b has not been moved
T = type(o), T has destructor drop
──────────────────────────────────────
T::drop(o) called automatically

[Destroy-Order-LIFO]
scope S contains responsible bindings b₁, b₂, ..., bₙ (declaration order)
S exits
──────────────────────────────────────
Destruction order: bₙ.drop(), bₙ₋₁.drop(), ..., b₂.drop(), b₁.drop()

[Object-Destroy-Region-End]
region r ends
objects O = {o₁, o₂, ..., oₘ} allocated in r
for each oᵢ: type(oᵢ) has destructor
──────────────────────────────────────
All destructors called (order implementation-defined within region)
Region memory deallocated in O(1)

[Object-Destroy-Rebind]
Γ ⊢ x : var T
Γ ⊢ e : T'
x currently holds value v
──────────────────────────────────────
T::drop(v) called before x = e
x then bound to new value
```

**Compiler privilege rationale**:
> "The compiler has the privilege to call destructors at scope exit regardless of the user-visible permission on the binding. This ensures RAII semantics. User code must have appropriate permission to call drop() explicitly."

**Normative rules**:
- "Objects with responsible bindings shall have their destructors called at scope exit."
- "Destruction order within a scope shall be LIFO (reverse declaration order)."
- "Region deallocation shall occur in O(1) time (bulk deallocation)."

#### 6.1.5 Object Lifetime
**Formal Definition 6.3**: Object lifetime is the period from object creation to object destruction.

**Lifetime categories**:
- **Lexical lifetime**: Bound to lexical scope (most common)
- **Region lifetime**: Bound to region extent
- **Temporary lifetime**: Expression evaluation duration
- **Static lifetime**: Entire program execution

**Lifetime rules**:
1. Object accessible only within its lifetime
2. Use after lifetime end is undefined behavior [UB]
3. Lifetime determined statically (lexical scoping)
4. No lifetime annotations required (regions provide scoping)

**Temporary lifetime rules**:

**Default**: Temporary destroyed at end of full expression
```cursive
f(g(x))  // g(x) temporary destroyed after f call completes
```

**Lifetime extension**: Extended when bound to `let` or const `ref`
```cursive
let x = Resource::new()  // Temporary extended to x's lifetime

ref y: const = get_resource()  // Extended to y's lifetime
```

**No extension**: `ref` to mutable does NOT extend
```cursive
ref z: unique = get_resource()  // Temporary NOT extended - ERROR
```

**Formal semantics**:
```
[Lifetime-Temporary-Default]
expression e produces temporary t
t not bound to variable
──────────────────────────────────────
lifetime(t) = end of full expression containing e

[Lifetime-Extension-Let]
let x = e
e produces temporary t
──────────────────────────────────────
lifetime(t) extended to lifetime(x)

[Lifetime-Extension-Const-Ref]
ref x: const = e
e produces temporary t
──────────────────────────────────────
lifetime(t) extended to lifetime(x)

[Lifetime-No-Extension-Mut-Ref]
ref x: unique = e  or  ref x: shared = e
e produces temporary t
──────────────────────────────────────
ERROR: temporary lifetime not extended for mutable ref
```

**Lifetime and transfer**:
```
[Lifetime-After-Transfer]
object o has lifetime L
responsible binding b transferred via move
──────────────────────────────────────
L continues (transferred to new responsible binding)
Original binding b invalid
ref bindings to o remain valid
```

**Normative rules**:
- "An object shall not be accessed outside its lifetime."
- "Use of an object after its lifetime has ended is undefined behavior [UB]."
- "Temporary object lifetimes shall be extended when bound to let declarations or const ref bindings."
- "Temporary object lifetimes shall not be extended for mutable ref bindings."

### Formal Elements Required
- Definition 6.1: Object
- Definition 6.2: Memory location
- Definition 6.3: Object lifetime
- Inference rules: Object creation, destruction, lifetime
- Algorithms: LIFO destruction order, temporary lifetime extension

### Canonical Example
Complete program demonstrating all object lifecycle concepts:
- Object creation (let, var, region, temporary)
- Object destruction (scope exit, LIFO order, region end)
- Object lifetime (lexical, temporary extension)
- Compiler privilege (destructor calls)
- Transfer and lifetime continuation

---

## Section 6.2: Binding Categories ✨ NEW STRUCTURE

### Source
- CURSIVE_MEMORY_MODEL_SPECIFICATION.md Part II (Binding Categories)

### Strategy
Write comprehensive formal specification of binding categories (let/var/ref) - first axis of two-axis model.

### Subsections

#### 6.2.1 Binding Category Concept
**Formal Definition 6.4**: A binding category determines:
1. **Cleanup responsibility**: Whether the binding calls the destructor
2. **Rebindability**: Whether the binding can be reassigned
3. **Transferability**: Whether cleanup responsibility can be transferred via `move`

**Three binding categories**:

| Category | Responsible | Rebindable | Transferable | Use Case |
|----------|-------------|------------|--------------|----------|
| `let` | ✅ YES | ❌ NO | ✅ YES | Most locals |
| `var` | ✅ YES | ✅ YES | ❌ NO | Loop counters, state machines |
| `ref` | ❌ NO | ❌ NO | ❌ NO | Parameters, temporary views |

**Orthogonality principle**:
> "Binding category is independent from permission. A binding can be responsible (`let`/`var`) with any permission (`const`/`unique`/`shared`)."

**Formal binding type**:
```
Binding = (Category, Permission, Type)

where:
  Category ∈ {let, var, ref}
  Permission ∈ {const, unique, shared}
  Type = any valid type
```

**Normative statements**:
- "Every binding shall have exactly one binding category."
- "Binding category determines cleanup responsibility, independent of permission."
- "The nine combinations of binding categories and permissions are all valid."

#### 6.2.2 `let` Binding Category
**Definition**: Responsible, non-rebindable, transferable binding.

**Formal semantics**:
```
[Let-Create]
Γ ⊢ e : T
──────────────────────────────────
Γ, x : let T ⊢ let x = e : ()
x is responsible for cleanup
x cannot be rebound
x can be transferred via move

[Let-Scope-Exit]
Γ ⊢ x : let T
scope containing x exits
x has not been transferred
T has destructor drop
──────────────────────────────────
T::drop(x) called
x removed from Γ

[Let-No-Rebind]
Γ ⊢ x : let T
Γ ⊢ e : T'
──────────────────────────────────
x = e → ERROR E6001: Cannot reassign let binding

[Let-Transferable]
Γ ⊢ x : let T
──────────────────────────────────
move x → valid (transfers responsibility)
x becomes invalid after transfer
```

**Properties**:
- Most common binding category for local variables
- Used for owned local variables with RAII
- Cleanup guaranteed at scope exit (unless transferred)
- Can transfer responsibility via `move` keyword
- Cannot be reassigned (immutable binding, not immutable value)

**With different permissions**:
```cursive
let x: const = Data::new()     // Responsible, immutable access
let y: unique = Data::new()    // Responsible, exclusive mutable
let z: shared = Data::new()    // Responsible, shared mutable
```

**Normative rules**:
- "A let binding shall call the destructor of its value at scope exit, unless the value has been transferred."
- "A let binding shall not be reassigned."
- "A let binding may be transferred via the move keyword."

#### 6.2.3 `var` Binding Category
**Definition**: Responsible, rebindable, non-transferable binding.

**Formal semantics**:
```
[Var-Create]
Γ ⊢ e : T
──────────────────────────────────
Γ, x : var T ⊢ var x = e : ()
x is responsible for cleanup
x can be rebound
x cannot be transferred

[Var-Rebind]
Γ ⊢ x : var T
Γ ⊢ e : T'
T' coercible to T
x currently holds value v_old
──────────────────────────────────
Γ ⊢ x = e : ()
Side effect: T::drop(v_old) called
x then bound to value of e

[Var-No-Transfer]
Γ ⊢ x : var T
──────────────────────────────────
move x → ERROR E6002: Cannot move from var binding

[Var-Scope-Exit]
Γ ⊢ x : var T
scope containing x exits
x currently holds value v
T has destructor drop
──────────────────────────────────
T::drop(v) called
x removed from Γ
```

**Rebinding semantics**:
> "When a `var` binding is rebound, the destructor of the old value is called before the binding refers to the new value."

**Rebinding sequence**:
1. Evaluate new expression
2. Call destructor on old value
3. Bind variable to new value

**Rationale for non-transferability**:
> "`var` bindings are always valid (never in moved-from state). Allowing transfer would create invalid bindings still in scope, violating this invariant."

**Use cases**:
- Loop counters
- Accumulators
- State machines that transition between states
- Any variable that needs rebinding

**With different permissions**:
```cursive
var counter: const = 0          // Rebindable, immutable access
var buffer: unique = Buffer::new()  // Rebindable, exclusive mutable
var cache: shared = Cache::new()    // Rebindable, shared mutable
```

**Normative rules**:
- "A var binding shall call the destructor of its current value when rebound."
- "A var binding shall call the destructor of its final value at scope exit."
- "A var binding shall not be transferred via the move keyword."
- "Rebinding shall occur in the order: evaluate new expression, destroy old value, bind to new value."

#### 6.2.4 `ref` Binding Category
**Definition**: Non-responsible, non-rebindable, non-transferable binding.

**Formal semantics**:
```
[Ref-Create]
Γ ⊢ x : let T  or  Γ ⊢ x : var T
──────────────────────────────────────
Γ, y : ref T ⊢ ref y = x : ()
y is NOT responsible for cleanup
y is a view into x's data
y cannot be rebound
y cannot be transferred

[Ref-No-Destructor]
Γ ⊢ y : ref T
scope containing y exits
──────────────────────────────────────
No destructor called for y
y simply removed from Γ

[Ref-No-Rebind]
Γ ⊢ y : ref T
Γ ⊢ e : T
──────────────────────────────────────
y = e → ERROR E6003: Cannot reassign ref binding

[Ref-No-Transfer]
Γ ⊢ y : ref T
──────────────────────────────────────
move y → ERROR E6002: Cannot move from ref binding
Rationale: ref is non-responsible, no responsibility to transfer

[Ref-Parameter-Default]
function f(param: T)
──────────────────────────────────────
Equivalent to: function f(ref param: T)
ref is implicit default for parameters

[Ref-Valid-After-Transfer]
Γ ⊢ owner : let T
Γ ⊢ viewer : ref T = owner
transfer owner to new_owner
──────────────────────────────────────
owner becomes INVALID
viewer remains VALID
new_owner becomes responsible
Data not destroyed (responsibility transferred)
```

**Key property**:
> "`ref` bindings remain valid after the responsible binding is transferred. The data is not destroyed until the new responsible binding goes out of scope."

**Safety guarantee**:
- Data not destroyed by transfer (only responsibility moved)
- `ref` is view, not ownership claim
- At most one responsible binding exists at any time
- Multiple `ref` bindings to same data permitted (permission rules apply)

**Use cases**:
- Function parameters (default)
- Temporary views into data owned elsewhere
- Multiple references to same data
- When cleanup responsibility not desired

**Parameter default rationale**:
> "Most function parameters are non-responsible views. Making `ref` the default reduces annotation noise."

**With different permissions**:
```cursive
ref viewer: const = data        // Non-responsible, immutable access (DEFAULT)
ref editor: unique = data       // Non-responsible, exclusive mutable
ref accessor: shared = world    // Non-responsible, shared mutable
```

**Normative rules**:
- "A ref binding shall not call any destructor."
- "A ref binding shall not be reassigned."
- "A ref binding shall not be transferred."
- "A ref binding shall remain valid after the original responsible binding is transferred."
- "Function parameters shall default to ref binding category."

### Formal Elements Required
- Definition 6.4: Binding category
- Definition 6.5: Cleanup responsibility
- Definition 6.6: Rebindability
- Definition 6.7: Transferability
- Inference rules for each binding category
- Rebinding algorithm (for `var`)
- Transfer validity rules

### Canonical Example
Comprehensive demonstration of all three binding categories with all properties:
- `let` binding with transfer
- `var` binding with rebinding and destructor calls
- `ref` binding valid after transfer
- All three permissions (const/unique/shared) shown
- Interaction patterns demonstrated

---

## Section 6.3: Permission System ✨ NEW STRUCTURE

### Source
- CURSIVE_MEMORY_MODEL_SPECIFICATION.md Part III (Permissions)
- `04_Lexical-Permissions.md` (System 1 - adapt to System 3)

### Strategy
Write comprehensive formal specification of permissions (const/unique/shared) - second axis of two-axis model.

### Subsections

#### 6.3.1 Permission Concept
**Formal Definition 6.8**: A permission determines:
1. **Access rights**: What operations can user code perform on the referenced value
2. **Aliasing control**: Whether multiple references with this permission can coexist

**Three permissions**:

| Permission | Mutability | Aliasing | Enforcement | Use Case |
|------------|-----------|----------|-------------|----------|
| `const` | Immutable | Allowed | Type system | Default, safe sharing |
| `unique` | Mutable | Forbidden | Compiler enforced | Safe mutation |
| `shared` | Mutable | Allowed | Programmer coordinated | Complex graphs, ECS |

**Orthogonality principle**:
> "Permission is independent from binding category. Any binding category (`let`/`var`/`ref`) can have any permission (`const`/`unique`/`shared`)."

**Permission lattice**:
```
unique ─────┐
            ├──→ const
shared ─────┘
```

**Weakening-only principle**:
> "Permissions can be weakened (downgraded), never strengthened (upgraded)."

**Normative statements**:
- "Every binding shall have exactly one permission."
- "Permission determines access rights and aliasing control, independent of binding category."
- "Permission coercion shall only weaken permissions, never strengthen them."

#### 6.3.2 `const` Permission
**Definition**: Immutable access, unlimited aliasing permitted.

**Formal semantics**:
```
[Const-Read]
Γ ⊢ x : const T
T has field f : T'
──────────────────────────────────
Γ ⊢ x.f : const T'
Field access preserves const permission

[Const-No-Mutate]
Γ ⊢ x : const T
op is mutable operation on T (requires unique or shared)
──────────────────────────────────
Γ ⊢ x.op() → ERROR E6010: Cannot mutate through const permission

[Const-Unlimited-Alias]
Γ ⊢ x : const T
Γ ⊢ y : const T = x
Γ ⊢ z : const T = x
... (unlimited)
──────────────────────────────────
All valid simultaneously
No aliasing restrictions
```

**Properties**:
- Default permission (immutability by default)
- Safe sharing (no coordination needed)
- Unlimited aliasing permitted
- Cannot perform mutable operations
- Cannot rebind through const (but can rebind const binding if it's `var`)

**Const binding vs const value**:
```cursive
let x: const = Data::new()  // Const permission: cannot mutate via x
var y: const = Data::new()  // Can rebind y, but cannot mutate through y
```

**Read-only operations**:
- Field access (read-only)
- Method calls (if method requires const or weaker)
- Pattern matching
- Comparison operations

**Default rule**:
> "`const` is the default permission for all bindings and parameters when no permission is explicitly specified."

**Normative rules**:
- "Operations through const permission shall not modify the referenced value."
- "Multiple const references to the same data may exist simultaneously without restriction."
- "const shall be the default permission when no permission is explicitly specified."

#### 6.3.3 `unique` Permission
**Definition**: Exclusive mutable access, compiler-enforced uniqueness.

**Formal semantics**:
```
[Unique-Mutate]
Γ ⊢ x : unique T
op is mutable operation on T
──────────────────────────────────
Γ ⊢ x.op() → valid
Mutable operations permitted

[Unique-Exclusivity]
Γ ⊢ x : unique T active
no other active bindings with unique/shared permission to same data
──────────────────────────────────
x has exclusive mutable access
Compiler enforces this statically

[Unique-Alias-Error]
Γ ⊢ x : unique T active
attempt to create y : unique T referencing same data
──────────────────────────────────
ERROR E4020: unique permission violation: active unique access exists

[Unique-Shared-Conflict]
Γ ⊢ x : unique T active
attempt to create y : shared T referencing same data
──────────────────────────────────
ERROR E4020: unique permission violation: active unique access exists

[Unique-Const-Downgrade]
Γ ⊢ x : unique T
Γ ⊢ y : const T = x
──────────────────────────────────
While y active:
  - x cannot perform mutable operations
  - Temporary downgrade to const
After y goes out of scope:
  - x regains unique access
```

**Compiler enforcement algorithm**:
```
For each scope S:
  unique_active = {}  // Map: data source → active unique binding
  shared_active = {}  // Map: data source → set of active shared bindings

  For each binding b in S:
    source = resolve_source(b)

    if permission(b) == unique:
      if unique_active.contains(source):
        ERROR E4020 at b
      if shared_active.contains(source):
        ERROR E4020 at b
      unique_active[source] = b

    if permission(b) == shared:
      if unique_active.contains(source):
        ERROR E4020 at b
      shared_active[source].add(b)

  On scope exit:
    Remove entries for bindings in S
```

**Properties**:
- Exclusive mutable access
- Compiler prevents aliasing bugs at compile time
- Can temporarily downgrade to `const`
- Safe default for mutation
- Prevents iterator invalidation

**Mutable operations**:
- Field mutation
- Method calls (if method requires unique or shared)
- In-place modifications
- Swap operations

**Iterator invalidation prevention**:
```cursive
let vec: unique = Vec::new()
ref iter = vec.iter()      // Creates const ref
vec.push(42)               // ERROR: Cannot mutate while iter active
```

**Normative rules**:
- "At most one active unique reference to a given piece of data shall exist at any time."
- "A unique reference shall not coexist with any shared reference to the same data."
- "The compiler shall enforce unique permission violations at compile time."
- "A unique reference may be temporarily downgraded to const when a const reference is created."

#### 6.3.4 `shared` Permission
**Definition**: Shared mutable access, programmer-coordinated safety.

**Formal semantics**:
```
[Shared-Mutate]
Γ ⊢ x : shared T
op is mutable operation on T
──────────────────────────────────
Γ ⊢ x.op() → valid
Mutable operations permitted

[Shared-Alias-Allowed]
Γ ⊢ x : shared T
Γ ⊢ y : shared T = x
Γ ⊢ z : shared T = x
... (unlimited)
──────────────────────────────────
All valid simultaneously
Compiler does NOT prevent aliasing
Type system DOCUMENTS aliasing
Programmer responsible for coordination

[Shared-No-Compiler-Enforcement]
Multiple active shared T references to same data
──────────────────────────────────
No compile error
Aliasing permitted and expected
Safety is programmer's responsibility
```

**General-purpose aliasing**:
> "`shared` works for ANY type, not just synchronized types. It is a general-purpose permission for intentional aliasing with programmer coordination."

**NOT restricted to**:
- Atomics
- Mutexes
- Synchronized types

**Works with**:
- Plain types (Vec, arrays, records)
- Custom types
- Any type where aliasing needed

**Properties**:
- Shared mutable access
- Compiler does NOT prevent aliasing
- Type-level documentation of aliasing intent
- Programmer coordinates safety
- Used when `unique` is too restrictive

**Coordination strategies**:
- Index-based access (different indices = safe)
- Runtime checks
- Spatial partitioning
- Temporal coordination (sequential phases)
- Domain knowledge (programmer knows pattern is safe)

**Use cases**:
- Game development (ECS, World systems)
- Complex entity graphs
- Multiple systems accessing shared data structure
- Index-based architectures
- When aliasing is intentional and coordinated

**Type-level documentation**:
> "The `shared` permission documents that aliasing is expected. Code reviewers and maintainers can see the aliasing intent from the type signature."

**Normative rules**:
- "Multiple shared references to the same data may exist simultaneously."
- "Operations through shared permission may modify the referenced value."
- "The compiler shall not prevent aliasing of shared references."
- "Safety of shared references is the programmer's responsibility."
- "The shared permission shall work with any type, not only synchronized types."

#### 6.3.5 Permission Lattice and Coercion
**Permission lattice definition**:
```
        unique ─────┐
                    ├──→ const
        shared ─────┘

Partial order:
  unique ⊑ const
  shared ⊑ const
  unique ⊥ shared (incomparable)
```

**Weakening-only principle**:
> "Permissions can be weakened (downgraded to a position higher in the lattice), never strengthened (upgraded to a position lower in the lattice)."

**Valid coercions**:
```
[Coerce-Unique-Const]
Γ ⊢ e : unique T
──────────────────────────────────
Γ ⊢ e : const T
Automatic coercion (weakening)

[Coerce-Shared-Const]
Γ ⊢ e : shared T
──────────────────────────────────
Γ ⊢ e : const T
Automatic coercion (weakening)

[No-Strengthen-Const-Unique]
Γ ⊢ e : const T
──────────────────────────────────
Γ ⊢ e : unique T → ERROR E6011: Cannot strengthen const to unique

[No-Strengthen-Const-Shared]
Γ ⊢ e : const T
──────────────────────────────────
Γ ⊢ e : shared T → ERROR E6011: Cannot strengthen const to shared

[No-Unique-To-Shared]
Γ ⊢ e : unique T
──────────────────────────────────
Γ ⊢ e : shared T → ERROR E6012: Cannot coerce unique to shared
Rationale: Would violate exclusivity guarantee
```

**Coercion sites**:
- Assignment to variable with weaker permission
- Function argument passing
- Return value
- Field access (combined with field permission)

**Normative rules**:
- "Permission coercion shall only occur in the direction of weakening."
- "Automatic coercion from unique to const shall be permitted."
- "Automatic coercion from shared to const shall be permitted."
- "Coercion from unique to shared shall not be permitted."
- "Strengthening permissions (const to unique/shared) shall not be permitted."

#### 6.3.6 Permission Propagation (Field Access)
**Field permission interaction**:

When accessing a field, the effective permission is determined by BOTH:
1. Container permission
2. Field's declared permission

**Effective permission algorithm**:
```
effective_permission(container_perm, field_perm) -> Permission | ERROR:
    if field_perm == const:
        // const field accessible through any container
        return const

    else if field_perm == unique:
        match container_perm:
            const:
                // Can only read unique field through const container
                return const
            unique:
                // Can mutate unique field through unique container
                return unique
            shared:
                // CONFLICT: Cannot access unique field through shared
                ERROR E6013: Cannot access unique field through shared container

    else if field_perm == shared:
        match container_perm:
            const:
                return const
            unique:
                // Can access shared field through unique container
                return shared
            shared:
                return shared
```

**Formal rule**:
```
[Field-Access-Permission]
Γ ⊢ container : P_container Record
Record.field has declared permission P_field
P_eff = effective_permission(P_container, P_field)
──────────────────────────────────────────────────
Γ ⊢ container.field : P_eff FieldType
```

**Permission propagation table**:

| Container ↓ / Field → | const Field | unique Field | shared Field |
|----------------------|-------------|--------------|--------------|
| **const Container** | const | const | const |
| **unique Container** | const | unique | shared |
| **shared Container** | const | ERROR | shared |

**Rationale for unique field through shared container error**:
> "A unique field requires exclusive access. A shared container permits aliasing. These requirements conflict, hence the access is forbidden."

**Normative rules**:
- "Field access permission shall be determined by both container permission and field permission."
- "const field shall be accessible through any container permission."
- "unique field shall require unique container for mutable access."
- "unique field shall not be accessible for mutation through shared container."
- "shared field shall be accessible through unique or shared container."

#### 6.3.7 Context-Sensitive Defaults
**Default permission rules**:

**Parameters**: Default to `const`
```cursive
function f(param: T)
≡
function f(ref param: const T)
```

**Local bindings**: Default to `const`
```cursive
let x = value
≡
let x: const T = value
```

**Fields**: Default to `const`
```cursive
record R { field: T }
≡
record R { field: const T }
```

**Return types**: Default to `const`
```cursive
function f() -> T
≡
function f() -> const T
```

**Complete defaults table**:

| Context | Binding Category Default | Permission Default | Rationale |
|---------|-------------------------|-------------------|-----------|
| Parameter | `ref` | `const` | Non-responsible, immutable view (most common) |
| Local `let` | **Required** (no default) | `const` | Immutability by default |
| Local `var` | **Required** (no default) | `const` | Immutability by default |
| Field | N/A (record owns) | `const` | Immutability by default |
| Return type | N/A (no binding) | `const` | Immutability by default |

**Explicit annotation overrides defaults**:
```cursive
function modify(data: unique Data)           // Override: unique
procedure update(world: shared World)        // Override: shared
function consume(let data: Data)             // Override: let binding
```

**Rationale for const default**:
> "Immutability by default promotes safer code. Mutation must be explicitly requested, making mutable access visible and intentional."

**Normative rules**:
- "When no permission is explicitly specified, const shall be the default."
- "When no binding category is explicitly specified for parameters, ref shall be the default."
- "Local bindings (let/var) shall require explicit binding category specification."

### Formal Elements Required
- Definition 6.8: Permission
- Definition 6.9: Aliasing
- Definition 6.10: Exclusive access
- Definition 6.11: Permission lattice
- Permission coercion rules (formal)
- Field permission propagation algorithm
- Aliasing tracking algorithm (compiler implementation)
- Default permission resolution rules

### Canonical Example
Comprehensive demonstration of all three permissions:
- const with unlimited aliasing
- unique with exclusivity enforcement
- shared with coordinated aliasing
- Permission coercions (weakening)
- Field permission propagation
- All nine combinations (3 bindings × 3 permissions)

---

## Section 6.4: Transfer Semantics 🔄 TRANSFORM

### Source
- CURSIVE_MEMORY_MODEL_SPECIFICATION.md Part V (Transfer Semantics)
- `04_Lexical-Permissions.md` lines 269-314 (move operations)

### Strategy
Transform comprehensive transfer semantics into formal specification, using correct terminology (transfer in prose, `move` keyword in code).

### Subsections

#### 6.4.1 Transfer Concept and `move` Keyword
#### 6.4.2 Transfer Rules and Validity
#### 6.4.3 Transfer in Different Contexts
#### 6.4.4 Ref Bindings After Transfer
#### 6.4.5 Partial Transfer Prohibition
#### 6.4.6 Transfer and Copy Types

### Formal Elements Required
- Definition 6.12: Transfer
- Transfer validity rules
- Invalidation tracking algorithm
- Context-specific transfer rules
- Copy predicate interaction

### Canonical Example
Comprehensive transfer demonstration across all contexts with ref validity after transfer.

---

## Section 6.5: Regions and Lifetimes 🔄 TRANSFORM

### Source
- CURSIVE_MEMORY_MODEL_SPECIFICATION.md Part IX (Region Integration)
- `04_Lexical-Permissions.md` lines 2293-2380
- `02_Type-System.md` lines 974-1027

### Strategy
Transform region content into formal specification, emphasizing lexical scoping alternative to lifetime parameters.

### Subsections

#### 6.5.1 Region Concept
#### 6.5.2 Region Syntax and Allocation
#### 6.5.3 Region Lifetime and Scope
#### 6.5.4 Region Escape Analysis
#### 6.5.5 Region Stack and Nesting
#### 6.5.6 Region Cleanup and Performance

### Formal Elements Required
- Definition 6.13: Region
- Region allocation rules
- Escape analysis algorithm
- O(1) bulk deallocation guarantee

### Canonical Example
Nested regions with escape analysis errors and valid patterns.

---

## Section 6.6: Storage Duration 🔄 EXPAND

### Source
- Part II §2.8.4 (conceptual overview)
- CURSIVE_MEMORY_MODEL_SPECIFICATION.md scattered references
- `03_Declarations-and-Scope.md` lines 1027-1159

### Strategy
Expand conceptual overview into complete formal specification of all storage duration categories.

### Subsections

#### 6.6.1 Storage Duration Categories
#### 6.6.2 Static Storage Duration
#### 6.6.3 Automatic Storage Duration
#### 6.6.4 Region and Heap Storage Duration

### Formal Elements Required
- Definition 6.14: Storage duration
- Static initialization order algorithm
- Storage duration determination rules

### Canonical Example
All storage durations in single program with initialization/deinitialization order.

---

## Section 6.7: Memory Layout ⭐ NEW

### Source
- Industry standards (C++, Rust)
- Design decisions for Cursive
- Implementation considerations

### Strategy
Write comprehensive memory layout specification from scratch, covering sizes, alignment, padding, and layout attributes.

### Subsections

#### 6.7.1 Type Sizes and Alignment
#### 6.7.2 Record Layout
#### 6.7.3 Enum Layout (Tagged Union)
#### 6.7.4 Layout Attributes
#### 6.7.5 Padding and Packing

### Formal Elements Required
- Definition 6.15: Alignment
- Definition 6.16: Padding
- Layout calculation algorithms
- repr attribute specifications

### Canonical Example
Complex type with explicit layout control and padding visualization.

---

## Section 6.8: Aliasing and Uniqueness 🔄 FORMALIZE

### Source
- CURSIVE_MEMORY_MODEL_SPECIFICATION.md Part III + VIII
- `04_Lexical-Permissions.md` lines 34-35, 78-85

### Strategy
Formalize aliasing rules implicit in permission system into explicit normative specification.

### Subsections

#### 6.8.1 Aliasing Rules and Definitions
#### 6.8.2 Uniqueness Guarantees
#### 6.8.3 Aliasing Violations and Detection

### Formal Elements Required
- Definition 6.17: Aliasing
- Definition 6.18: Uniqueness
- Aliasing violation detection algorithm

### Canonical Example
Aliasing patterns with unique violations and shared coordination.

---

## Section 6.9: Unsafe Operations 🔄 TRANSFORM

### Source
- `04_Lexical-Permissions.md` lines 2343-2380
- CURSIVE_MEMORY_MODEL_SPECIFICATION.md unsafe considerations
- Design decisions

### Strategy
Transform unsafe block semantics into comprehensive formal specification with safety obligations.

### Subsections

#### 6.9.1 Unsafe Block Semantics
#### 6.9.2 Safety Obligations
#### 6.9.3 Unsafe Capabilities
#### 6.9.4 When Unsafe is Required
#### 6.9.5 Safe Abstractions Over Unsafe

### Formal Elements Required
- Definition 6.19: Unsafe block
- Definition 6.20: Safety obligation
- Unsafe capability enumeration
- Safety proof obligations

### Canonical Example
Unsafe block with raw pointer manipulation and safety invariant documentation.

---

## Writing Standards (Apply to All Sections)

### Normative Language
- **shall / shall not** = mandatory requirements (ISO/IEC Directives Part 2)
- **should / should not** = recommendations
- **may / may not** = permissions
- **can / cannot** = capability (informative)

### Cross-References
- Use `§X.Y.Z` format for internal references
- Use `[REF_TBD]` for references to other parts
- Use `Part [REF_TBD]` for references to other parts
- All references shall be validated before publication

### Formal Notation
- **Judgment forms**: `Γ ⊢ e : T` per §[REF_TBD]
- **Inference rules**: `[Rule-Name]` format per §[REF_TBD]
- **Grammar snippets**: EBNF notation per §[REF_TBD]
- **Algorithms**: Structured pseudocode
- **Note**: "For complete grammar, see Annex A"

### Example Requirements
- ONE canonical example per major concept
- Example is comprehensive (demonstrates all aspects)
- Example is minimal yet complete
- Both valid (✅) and invalid (❌) cases shown where appropriate
- Example is self-contained
- Example quality: publication-ready
- Examples demonstrate real-world patterns

### Error Codes
- Part VI range: E4xxx, E6xxx
- Each error code documented with:
  - Brief description
  - Context where it occurs
  - Example triggering the error
  - Suggested fix

### ISO/ECMA Section Structure
Each section follows:
1. Section number + title
2. Introduction paragraph (overview)
3. Numbered subsections (X.Y.Z format)
4. Formal definitions (Definition X.Y format)
5. Normative rules (using shall/should/may)
6. Inference rules and algorithms (formal notation)
7. Canonical example (comprehensive demonstration)
8. Cross-references ([REF_TBD] placeholders)
9. Notes (informative clarifications)

### Consistency Requirements
- Terminology consistent with glossary
- Judgment form notation consistent
- Example formatting consistent
- Section numbering follows Proposed_Organization.md (with adjustments)
- Cross-references validated

---

## Validation Checklist (For Each Section)

Before considering a section complete, verify:

- ✅ All subsections from plan present (or deviation justified)
- ✅ Formal definitions for key concepts (Definition X.Y format)
- ✅ Normative language used correctly (shall/should/may)
- ✅ Inference rules for semantic specifications
- ✅ Algorithms for procedures (where applicable)
- ✅ Grammar productions (inline + Annex A reference where applicable)
- ✅ ONE canonical example per major concept
- ✅ Example is comprehensive and publication-quality
- ✅ Cross-references use appropriate format
- ✅ Error codes defined and documented
- ✅ Implementation-defined aspects marked
- ✅ Undefined behavior tagged [UB: B.2 #N]
- ✅ Section structure follows ISO/ECMA format
- ✅ Content is transformed, not copied
- ✅ Sufficient detail for implementer
- ✅ Two-axis model consistently applied
- ✅ System 3 terminology throughout

---

## Success Criteria

PART VI is complete when:

✅ All 10 sections written to specification
✅ Every concept has formal definition
✅ Every requirement is normative (shall/should/may)
✅ ONE canonical example per concept
✅ All sections self-contained within Part VI scope
✅ All forward references use appropriate format
✅ ISO/ECMA formatting throughout
✅ Zero ambiguity - normative precision achieved
✅ Implementer-ready specification (can build memory model from Part VI)
✅ LLM-friendly clarity maintained
✅ Publication-quality technical writing
✅ Complete transformation (no copy-paste)
✅ System 3 two-axis model fully specified
✅ All nine combinations (3 bindings × 3 permissions) documented
✅ Compiler implementation guidance included
✅ All safety guarantees formally specified

---

## Key Distinguishing Features of Part VI

This specification is distinguished by:

1. **Two-Axis Orthogonal Model**: First specification to formally document binding categories as independent from permissions
2. **Nine Valid Combinations**: Complete coverage of all binding × permission combinations
3. **Explicit Transfer**: Move keyword required at call site (not implicit like Rust)
4. **Shared Permission**: Enables game development patterns impossible in Rust
5. **No Lifetime Annotations**: Region-based scoping eliminates need for `'a` parameters
6. **Ref After Transfer**: Unique property that ref bindings remain valid after responsible binding transferred
7. **Compiler Privilege**: Formal specification of compiler's ability to call destructors regardless of permission
8. **Comprehensive Examples**: Each example demonstrates all aspects of concept

---

## Estimated Scope

**Sections**: 10
**Subsections**: 50
**Completeness criterion**: Publication-ready ISO/ECMA specification

This plan serves as the authoritative guide for writing **PART VI: MEMORY MODEL AND PERMISSIONS** of the Cursive Language Specification using the System 3 two-axis model as canonical.