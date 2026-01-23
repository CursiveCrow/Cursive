# Cursive Language Specification

## Clause 10 — Memory Model, Regions, and Permissions

**Part**: X — Memory Model, Regions, and Permissions
**File**: 11-7_Aliasing-and-Uniqueness.md  
**Section**: §10.7 Aliasing and Uniqueness Rules  
**Stable label**: [memory.aliasing]  
**Forward references**: §11.2 [memory.object], §11.4 [memory.permission], Clause 7 §7.5 [type.pointer]

---

### §10.7 Aliasing and Uniqueness Rules [memory.aliasing]

#### §10.7.1 Overview

[1] This section specifies when multiple bindings may reference the same memory location (aliasing), how the `unique` permission prevents aliasing, and how field-level partitioning enables safe `shared` usage.

[2] Aliasing is controlled through permissions: `const` allows unlimited aliasing, `shared` allows coordinated aliasing, and `unique` prevents aliasing through compile-time enforcement.

#### §10.7.2 Aliasing Definition [memory.aliasing.definition]

##### §10.7.2.1 Formal Definition

[3] Two bindings $x$ and $y$ _alias_ when they reference overlapping memory locations:

$$
\text{alias}(x, y) \iff \text{location}(x) \cap \text{location}(y) \ne \emptyset
$$

[4] For scalar types (§7.2), bindings alias when they reference the exact same location. For composite types (§7.3), bindings alias when they reference overlapping fields or elements.

##### §10.7.2.2 Aliasing Examples

**Example 11.7.2.1 (Aliasing through non-responsible bindings):**

```cursive
let data = Buffer::new()      // data is responsible
let ref1 <- data              // ref1 aliases data
let ref2 <- data              // ref2 aliases data and ref1

// All three bindings reference the same Buffer object
// Only data has cleanup responsibility
```

##### §10.7.2.3 Field Aliasing

[5] Field access creates aliases:

**Example 11.7.2.2 (Field aliasing):**

```cursive
record Point { x: f64, y: f64 }

let point = Point { x: 3.0, y: 4.0 }
let x_ref <- point.x          // Aliases point's x field
let y_ref <- point.y          // Aliases point's y field (different location)

// x_ref and y_ref do NOT alias each other (different fields)
```

#### §10.7.3 Unique Permission Enforcement [memory.aliasing.unique]

##### §10.7.3.1 Overview

[6] The `unique` permission guarantees exclusive access: no other `unique` or `shared` bindings to the same object can coexist. The compiler enforces this through active reference tracking. Temporary downgrades to `const` are permitted.

##### §10.7.3.2 Unique Exclusivity Rule

[7] When a `unique` binding is active, creating another `unique` or `shared` binding to the same object is forbidden:

[ Given: Active `unique` binding $x$ to object $o$ ]

$$
\frac{x : \text{unique } \tau \text{ active} \quad y \text{ attempts binding to } o \quad y : (\text{unique} \mid \text{shared}) \tau}{\text{ERROR E11-302: unique permission already active}}
\tag{WF-Unique-No-Alias}
$$

[8] The exclusivity holds until $x$ goes out of scope or is moved.

##### §10.7.3.3 Temporary Const Allowed

[9] Creating `const` bindings from `unique` is allowed (permission downgrade):

**Example 11.7.3.1 (Unique with const downgrade):**

```cursive
let data: unique Buffer = Buffer::new()

{
    let reader: const <- data     // OK: downgrade to const
    println("{}", reader.size())
    // data.modify()               // error: cannot use unique while const active
}

data.modify()                      // OK: unique restored after const out of scope
```

#### §10.7.4 Shared Permission Coordination [memory.aliasing.shared]

##### §10.7.4.1 Overview

[10] The `shared` permission allows multiple mutable bindings. The compiler does NOT prevent aliasing for `shared`; programmers coordinate access to prevent conflicts.

##### §10.7.4.2 Field-Level Protection

[11] Field-level partitioning (§11.4.5) prevents accessing the same field multiple times through `shared`, while allowing different-field access:

**Example 11.7.4.1 (Shared with partition protection):**

```cursive
record World {
    <<Physics>>
    positions: [Position],
    velocities: [Velocity],

    <<Rendering>>
    sprites: [Sprite],
}

let world: shared World = World::new()

let pos: shared <- world.positions      // Access Physics partition
let spr: shared <- world.sprites        // Access Rendering partition (OK: different)

// let vel: shared <- world.velocities  // error[E11-310]: Physics partition already active
```

#### §10.7.5 Active Reference Tracking [memory.aliasing.tracking]

##### §10.7.5.1 Tracking Algorithm

[12] The compiler tracks active non-`const` bindings to enforce uniqueness and partition rules:

**Tracking state per object:**

- **Unique active**: One `unique` binding exists; no other `unique` or `shared` allowed
- **Shared active**: One or more `shared` bindings exist; tracks which partitions are active
- **Const only**: Zero or more `const` bindings; any permission can be added

[13] **State transitions**:

$$
\begin{align*}
\text{None} &\xrightarrow{\text{create unique}} \text{Unique}(x) \\
\text{None} &\xrightarrow{\text{create shared}} \text{Shared}(\{p_1, \ldots\}) \\
\text{Unique}(x) &\xrightarrow{x \text{ out of scope}} \text{None} \\
\text{Shared}(P) &\xrightarrow{\text{all out of scope}} \text{None}
\end{align*}
$$

#### §10.7.6 Diagnostics

[14] Aliasing diagnostics:

| Code    | Condition                                 |
| ------- | ----------------------------------------- |
| E11-701 | Multiple `unique` bindings to same object |
| E11-702 | `shared` binding while `unique` active    |
| E11-703 | `unique` binding while `shared` active    |

[ Note: E11-302 (duplicate unique) is defined in §11.4. E11-310 (partition conflict) is defined in §11.4.5.
— end note ]

#### §10.7.7 Conformance Requirements

[15] Implementations shall:

1. Track active bindings with `unique` and `shared` permissions
2. Enforce unique exclusivity: reject multiple `unique` or `shared` while `unique` active
3. Allow unlimited `const` aliasing
4. Implement partition tracking for `shared` field access
5. Provide compile-time aliasing checks with zero runtime cost
6. Integrate with escape analysis (§11.3) for pointer aliasing
7. Maintain aliasing rules across all binding forms (let/var, =/←)

---

**Previous**: §11.6 Layout and Alignment (§11.6 [memory.layout]) | **Next**: §11.8 Unsafe Blocks and Safety Obligations (§11.8 [memory.unsafe])
