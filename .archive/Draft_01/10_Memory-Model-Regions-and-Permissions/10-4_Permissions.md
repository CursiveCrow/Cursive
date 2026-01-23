# Cursive Language Specification

## Clause 10 — Memory Model, Regions, and Permissions

**Part**: X — Memory Model, Regions, and Permissions
**File**: 11-4_Permissions.md  
**Section**: §10.4 Permissions  
**Stable label**: [memory.permission]  
**Forward references**: §9.2 [memory.object], §9.5 [memory.move], §9.7 [memory.aliasing], Clause 4 §4.5 [decl.type], Clause 6 §6.1.2 [type.overview.permissions]

---

### §10.4 Permissions [memory.permission]

#### §10.4.1 Overview

[14] The memory model embodies the "explicit over implicit" design principle:

(14.1) **Explicit responsibility transfer**: The `move` keyword is required at call sites to transfer cleanup responsibility. Implicit transfers are forbidden.

(14.2) **Explicit permissions**: Type annotations include permission qualifiers (`const`, `unique`, `shared`) when needed. `const` is the default permission in all contexts (bindings, parameters, return types, fields); mutability requires explicit `unique` or `shared` qualifiers. The `const` qualifier may be stated explicitly for documentation but is redundant.

(14.3) **Explicit allocation**: Region allocation requires the `^` operator; there are no hidden heap allocations.

(14.4) **Explicit coordination**: The `shared` permission requires programmers to coordinate access; the compiler does not hide synchronization.

(14.5) **Explicit escape**: Escaping region-allocated data to the heap requires explicit `.to_heap()` calls.

[15] These explicit markers make memory behavior locally visible without global analysis.

[1] Permissions control what operations are allowed on values. Cursive provides three permissions: `const` (read-only), `unique` (exclusive mutable access, no aliasing), and `shared` (shared mutable with programmer coordination).

[2] Permissions are orthogonal to cleanup responsibility (§9.2). A binding can be responsible yet immutable, or non-responsible yet mutable. Permissions attach to types at binding sites as specified in §6.1.2.

[ Note: Permissions are type qualifiers, not binding modifiers. The syntax `let x: const Type` means the binding `x` has type `const Type`, where `const` is part of the type itself. Similarly, `let x: unique Type` means `x` has type `unique Type`. The permission qualifier (`const`, `unique`, or `shared`) is part of the type system, not a separate binding attribute. This distinction is important: permissions control what operations are allowed on values, while binding responsibility (determined by `=` vs `<-`) controls cleanup semantics.
— end note ]

[3] This section specifies permission semantics, the permission lattice, compile-time enforcement rules, and field-level partitioning for safe `shared` usage.

#### §10.4.2 Permission Definitions [memory.permission.definitions]

##### §10.4.2.1 Const Permission

[4] The `const` permission provides read-only access with unlimited aliasing. Multiple `const` bindings can reference the same object safely.

**Semantics:**

- ✅ Read object fields and call const methods
- ❌ Mutate object fields
- ❌ Call methods requiring unique or shared
- ✅ Create multiple `const` bindings to same object (unlimited aliasing)

[5] Permission qualifiers may be omitted; the compiler infers permissions from context. Complete inference rules are specified in §9.4.2.5 [memory.permission.inference].

[5.1] **Summary**: When permission annotation is omitted:

- If initializer has permission-qualified type: permission is inherited from initializer
- Otherwise: permission defaults to `const`
- Explicit annotations override inference

Mutability is strictly opt-in: to enable mutation, `unique` or `shared` must be explicitly specified. This ensures that all code is immutable by default, making mutation explicit and visible.

**Example 11.4.2.1 (Const permission):**

```cursive
let data: const Buffer = Buffer::new()
let ref1: const <- data
let ref2: const <- data
let ref3: const <- data      // Multiple const bindings allowed

println("{}", data.size())   // Read-only access OK
// data.modify()             // error[E10-301]: cannot mutate through const
```

##### §10.4.2.2 Unique Permission

[6] **Semantics:**

- ✅ Read and mutate object fields
- ✅ Call any methods (const, unique, shared)
- ❌ Create second `unique` binding while first is active
- ❌ Create `shared` binding while `unique` is active
- ✅ Create temporary `const` binding (downgrade)

[6.1] **Exclusivity clarification**: `unique` exclusivity means no other `unique` OR `shared` bindings to the same object can coexist. Temporary downgrades to `const` are permitted; during the downgrade, the `unique` binding becomes inactive and regains exclusive access when the `const` binding exits scope.

[6.2] **Formal constraint:**

[ Given: Active `unique` binding $x$ to object $o$ ]

$$
\frac{x : \text{unique } \tau \text{ active} \quad y \text{ attempts binding to } o}
     {\text{Permission}(y) \in \{\text{unique}, \text{shared}\} \Rightarrow \text{ERROR E10-302}}
\tag{WF-Unique-Exclusive}
$$

Creating `const` bindings is permitted (downgrade), but `unique` becomes temporarily inactive.

**Example 11.4.2.2 (Unique permission):**

```cursive
let data: unique Buffer = Buffer::new()

data.append(42)              // Mutation allowed

let second: unique <- data   // error[E10-302]: unique already active
```

##### §10.4.2.3 Shared Permission

[7] The `shared` permission provides shared mutable access. Multiple `shared` bindings can coexist; programmers coordinate to prevent conflicts.

**Semantics:**

- ✅ Read and mutate object fields
- ✅ Create multiple `shared` bindings
- ⚠️ Programmer responsible for coordination
- ✅ Field-level partitioning prevents same-field multi-access (§11.4.6)

**Example 11.4.2.3 (Shared permission):**

```cursive
let data: shared Buffer = Buffer::new()
let ref1: shared <- data
let ref2: shared <- data     // Multiple shared bindings allowed

ref1.set_flag(true)          // Both can mutate
ref2.set_counter(10)         // Programmer ensures no conflicts
```

##### §10.4.2.4 No Interior Mutability

[8] **Design principle**: `const` means const. There is no interior mutability: if a value has `const` permission, it is immutable. No hidden mutation through `const` is permitted.

[ Rationale: Some languages provide interior mutability mechanisms that allow mutation through immutable references. Cursive explicitly rejects this pattern to maintain the "explicit over implicit" design principle. If mutation is needed, use `unique` or `shared` permission explicitly.
— end rationale ]

##### §10.4.2.5 Permission Inference Rules [memory.permission.inference]

[9] **THIS SECTION IS AUTHORITATIVE** for permission inference. When a binding omits explicit permission annotation, the compiler infers permission using the following precedence:

**Rule 1: Explicit annotation** (highest precedence)

If permission is explicitly specified, use it: `let x: unique T = value`

**Rule 2: Inheritance from initializer**

If initializer has permission-qualified type, inherit that permission:

```cursive
let unique_data: unique Buffer = Buffer::new()
let inherits = unique_data       // Inferred: unique Buffer
```

**Rule 3: Default to const** (fallback)

Otherwise, default to `const`:

```cursive
let x: i32 = 42                  // Inferred: const i32
procedure f(p: Buffer)           // Inferred: const Buffer
procedure make(): Buffer         // Inferred: const Buffer
```

**Formal Inference Rule:**

[ Given: Binding declaration $x$ with optional annotation $A$ and initializer $e$ of type $\tau_e$ ]

$$
\frac{\Gamma \vdash e : \text{perm}_e\, \tau_e \quad A = \text{none}}{\Gamma \vdash x : \text{perm}_e\, \tau_e}
\tag{Infer-Perm-Inherit}
$$

$$
\frac{\Gamma \vdash e : \tau_e \quad A = \text{none}}{\Gamma \vdash x : \text{const}\, \tau_e}
\tag{Infer-Perm-Default}
$$

**Permission Inference Examples:**

**Example 11.4.2.5 (Permission inference):**

```cursive
let explicit: unique Buffer = Buffer::new()    // Explicit unique
let inherits = explicit                        // Inferred: unique (inherits from explicit)
let downgrades: const = explicit               // Explicit: const (downgrade)

procedure process(data: Buffer)                // Inferred: const Buffer
procedure modify(data: unique Buffer)          // Explicit: unique Buffer

let param_const = process(explicit)            // OK: unique → const coercion at call
```

**Diagnostic E10-320: Ambiguous permission inference**

[9.1] When permission cannot be uniquely inferred (for example, in complex generic contexts), the compiler shall emit diagnostic E10-320 requesting an explicit annotation:

```cursive
// Ambiguous: Cannot infer permission from context
// let ambig = if cond { unique_val } else { shared_val }  // error[E10-320]

// Resolved: Explicit annotation
let resolved: const = if cond { unique_val } else { shared_val }  // OK: downgrade both
```

#### §10.4.3 Permission Lattice [memory.permission.lattice]

##### §10.4.3.1 Subtyping Relations

[10] Permissions form a lattice with subtyping relations:

$$
\text{unique} <: \text{shared} <: \text{const}
$$

[11] **Downgrade rules**: Permissions can downgrade (stronger → weaker) implicitly:

$$
\frac{\Gamma \vdash e : \text{unique } \tau}{\Gamma \vdash e : \text{shared } \tau}
\tag{Coerce-Unique-Shared}
$$

$$
\frac{\Gamma \vdash e : \text{unique } \tau}{\Gamma \vdash e : \text{const } \tau}
\tag{Coerce-Unique-Const}
$$

$$
\frac{\Gamma \vdash e : \text{shared } \tau}{\Gamma \vdash e : \text{const } \tau}
\tag{Coerce-Shared-Const}
$$

[12] **No upgrades**: Permissions cannot upgrade (weaker → stronger). Attempting to treat `const` as `unique` produces diagnostic E10-303.

##### §10.4.3.2 Permission Lattice Diagram

[13] Visual representation:

```
        unique (most restrictive)
         /  \
        /    \
    shared   (middle)
        \    /
         \  /
        const (least restrictive)
```

#### §10.4.4 Permission Checking [memory.permission.checking]

##### §10.4.4.1 Active Reference Tracking

[14] The compiler tracks active bindings with non-`const` permissions to enforce uniqueness and detect conflicts:

(14.1) When a `unique` binding is created, the compiler records it as active.

(14.2) While a `unique` binding is active, creating another `unique` or `shared` binding to the same object is forbidden.

(14.3) When the `unique` binding goes out of scope, it becomes inactive.

[15] **Unique enforcement rule**:

[ Given: Object $o$, active `unique` binding $x$ ]

$$
\frac{x : \text{unique } \tau \text{ active} \quad y \text{ attempts } \text{unique or shared binding to same object}}{\text{ERROR E10-302: unique permission already active}}
\tag{WF-Unique-Exclusive}
$$

##### §10.4.4.2 Temporary Downgrade

[16] A `unique` binding can temporarily downgrade to `const`:

**Example 11.4.4.1 (Temporary const downgrade):**

```cursive
let data: unique Buffer = Buffer::new()

{
    let reader: const <- data   // Temporarily downgrade to const
    println("{}", reader.size())
    // data.modify()             // error: cannot use unique while const active
}

// Const binding out of scope
data.modify()                    // OK: unique restored
```

#### §10.4.5 Field-Level Partitioning [memory.permission.partitioning]

##### §10.4.5.1 Overview

[17] Field-level partitioning prevents accessing the same field multiple times through `shared` bindings, eliminating iterator invalidation and aliasing hazards while preserving the ability to access different fields simultaneously.

##### §10.4.5.2 Partition Directive Syntax

[18] Partition directives appear in record bodies:

**Partition directives** take one of the following forms:
```
"<<" <identifier> ">>"
"<<" "_" ">>"    // Reset to default
```

**Record bodies** match the pattern:
```
"{" [ <partition_directive> | <record_field> ... ] "}"
```

[ Note: See Annex A §A.6 [grammar.declaration] for complete record grammar with partition directives.
— end note ]

[19] **Default behavior**: Each field is its own partition unless a directive groups them.

**Example 11.4.5.1 (Partition directives):**

```cursive
record World {
    entities: [Entity],      // Default: World::entities partition

    <<Physics>>
    positions: [Position],   // Physics partition
    velocities: [Velocity],  // Physics partition

    <<Combat>>
    healths: [Health],       // Combat partition
}
```

##### §10.4.5.3 Partition Access Rules

[20] **Single-access rule**: Through a `shared` binding, each partition can be accessed at most once in a scope.

[ Given: Shared binding $s$, field $f$ in partition $p$ ]

$$
\frac{s : \text{shared } R \quad f \in \text{partition } p \quad \text{active access to partition } p}{\text{ERROR E10-310: partition already accessed}}
\tag{WF-Partition-Single}
$$

[21] **Different-partition freedom**: Accessing fields in different partitions is allowed:

**Example 11.4.5.2 (Partition access):**

```cursive
let world: shared World = World::new()

let pos_ref: shared <- world.positions     // Access Physics partition
let health_ref: shared <- world.healths    // Access Combat partition (OK: different)

// let vel_ref: shared <- world.velocities  // error[E10-310]: Physics partition already active
```

[22] The `<<_>>` directive resets to default partitioning (each field separate).

##### §10.4.5.4 Nested Field Access and Partitions [memory.permission.partitioning.nested]

[23] **Nested field access behavior.** Partition access tracking applies only to direct field access on the `shared` binding itself. Once a field reference is obtained, further nested field access through that reference does not count as re-accessing the partition.

[ Given: Shared binding $s$ with field $f$ in partition $p$, resulting reference $r$ ]

$$
\frac{s: \text{shared } R \quad r \gets s.f \quad f \in \text{partition } p}{\text{nested accesses through } r \text{ do not re-check partition } p}
\tag{P-Partition-Nested}
$$

[24] The partition constraint is enforced at the initial field access point. Subsequent operations through the obtained reference (further field access, method calls, indexing) are unrestricted by partition rules.

**Example 11.4.5.3 (Nested access through partition):**

```cursive
record Inner { x: i32, y: i32 }
record Outer {
    <<PartA>>
    field1: Inner,
    field2: Inner,
}

let obj: shared Outer = Outer::new()
let ref1: shared <- obj.field1       // Accesses PartA partition
let x_coord = ref1.x                 // OK: accessing through ref1, not obj
let y_coord = ref1.y                 // OK: nested access doesn't re-check partition

// let ref2: shared <- obj.field2    // error[E10-310]: PartA partition already accessed
```

[25] This design allows normal operation on obtained references while preventing multiple direct accesses to the same partition, balancing safety with usability.

#### §10.4.6 Diagnostics

[26] [Note: Diagnostics defined in this subsection are cataloged in Annex E §E.5.1.10. — end note]

#### §10.4.7 Conformance Requirements

[27] Implementations shall provide compile-time permission checking with zero runtime overhead.

---

**Previous**: §11.3 Regions (§11.3 [memory.region]) | **Next**: §11.5 Move/Copy/Clone Semantics (§11.5 [memory.move])
