# The Cursive Language Specification

**Part**: VI — Memory Model and Permissions
**File**: Field-Level-Partitioning.md
**Section**: Field-Level Partitioning for Shared Permission Safety
**Related**: [Memory Model Specification](CURSIVE_MEMORY_MODEL_SPECIFICATION.md) | [Lexical Permissions](../04_Lexical-Permissions.md)

---

# Field-Level Partitioning for Shared Permission Safety

## Table of Contents

1. [Introduction](#1-introduction)
2. [Partition Directive Syntax](#2-partition-directive-syntax)
3. [Default Partitioning Behavior](#3-default-partitioning-behavior)
4. [Partition Scoping Rules](#4-partition-scoping-rules)
5. [Access Rules and Enforcement](#5-access-rules-and-enforcement)
6. [Formal Semantics](#6-formal-semantics)
7. [Examples and Patterns](#7-examples-and-patterns)
8. [Error Codes and Diagnostics](#8-error-codes-and-diagnostics)
9. [Implementation Guidance](#9-implementation-guidance)
10. [Conformance Requirements](#10-conformance-requirements)

---

## 1. Introduction

### 1.1 Motivation

The `shared` permission in Cursive allows multiple mutable references to the same data, with the programmer responsible for coordinating access. While this flexibility is essential for patterns like Entity Component Systems (ECS), it introduces potential safety hazards:

**Primary Safety Concern:**

```cursive
record World {
    positions: shared Vec<Position>,
}

procedure unsafe_example(world: shared World) {
    ref p1: shared = world.positions;
    ref p2: shared = world.positions;  // ⚠️ Two mutable aliases to same Vec

    p1.push(Position::new());          // May invalidate p2's iterators
    for pos in p2.iter() { ... }       // ⚠️ Iterator invalidation bug
}
```

**The Problem:**

Accessing the same field multiple times through `shared` references can lead to:
- Iterator invalidation
- Data race vulnerabilities (when concurrency is added)
- Violated invariants across aliases

**Traditional Solutions (Rejected for Cursive):**

1. **Borrowing/Borrow Checker**: Incompatible with Cursive's "no borrow checker" design goal
2. **Lifetime Annotations**: Explicitly rejected (no `'a` style lifetimes)
3. **Runtime Tracking**: Violates "zero runtime overhead" principle

**Field-Level Partitioning Solution:**

This specification introduces **field-level partitioning**, a compile-time mechanism that:
- ✅ Prevents accessing the same field multiple times through `shared` references
- ✅ Allows accessing different fields simultaneously (the safe, common case)
- ✅ Requires zero runtime overhead (pure compile-time checking)
- ✅ Supports explicit grouping when mutual exclusion is desired
- ✅ Maintains Cursive's design principles (no borrowing, local reasoning, explicit syntax)

### 1.2 Design Goals

**Alignment with Cursive Principles:**

1. **Explicit over implicit**: Partition boundaries are visible in code via `<<Partition>>` directives
2. **Local reasoning**: Partition assignments visible in record definition
3. **Zero runtime overhead**: All checking at compile-time, no runtime state
4. **No borrow checker**: Partitions are not borrows; just compile-time access tracking
5. **Practical power**: Prevents real bugs without excessive restrictions
6. **Progressive enhancement**: Works without annotations (safe default), opt-in grouping

**Safety Properties:**

1. **Same-field protection**: Cannot access the same field twice through `shared` in the same scope
2. **Different-field freedom**: Can access different fields simultaneously (safe pattern)
3. **Explicit grouping**: Can declare that multiple fields must not be accessed together
4. **Lexical scoping**: Partition locks released when `ref` binding goes out of scope

### 1.3 Non-Goals

**What Field-Level Partitioning Does NOT Provide:**

1. **Element-level partitioning**: Does not prevent accessing `positions[0]` twice (field-level granularity only)
2. **Borrowing semantics**: Partitions are not temporary "borrows" that return access
3. **Dynamic partitioning**: All partitions determined at compile-time
4. **Thread safety**: Partitioning is orthogonal to concurrency (see Part XIII)
5. **Invariant enforcement**: Does not verify programmer's coordination logic

**Rationale:**

Field-level granularity matches the actual safety boundary in the primary use case (ECS component arrays). Element-level tracking would require borrow checker complexity, which is explicitly rejected.

---

## 2. Partition Directive Syntax

### 2.1 Lexical Syntax

**Partition Directive:**

A partition directive consists of the double angle bracket delimiters `<<` and `>>` surrounding an identifier.

```ebnf
PartitionDirective ::= "<<" Identifier ">>"
```

**Default Partition Directive:**

The special directive `<<_>>` resets to default partitioning behavior (each field is its own partition).

```ebnf
DefaultPartitionDirective ::= "<<" "_" ">>"
```

**Lexical Tokens:**

- `<<`: Left double angle bracket (partition directive open)
- `>>`: Right double angle bracket (partition directive close)

**Note:** The tokens `<<` and `>>` are distinct from left shift (`<<`) and right shift (`>>`) operators due to context. In record field declarations, they are parsed as partition directive delimiters.

### 2.2 Syntactic Placement

**Record Field Context:**

Partition directives appear within record bodies, before field declarations:

```ebnf
RecordBody ::= "{" RecordItem* "}"

RecordItem ::= PartitionDirective
             | RecordField

PartitionDirective ::= "<<" (Identifier | "_") ">>"

RecordField ::= Identifier ":" Permission? Type ","?
```

**Example:**

```cursive
record World {
    entities: Vec<Entity>,      // Before any directive: default partition

    <<Physics>>                 // Partition directive
    velocities: Vec<Velocity>,  // In Physics partition
    forces: Vec<Force>,         // In Physics partition

    <<Combat>>                  // New partition directive
    healths: Vec<Health>,       // In Combat partition
}
```

### 2.3 Partition Identifier Requirements

**Valid Partition Identifiers:**

Partition identifiers follow standard identifier rules:
- Must conform to UAX#31 (XID_Start and XID_Continue)
- Case-sensitive
- Cannot be reserved keywords (except `_` for default directive)

**Examples:**

```cursive
// ✅ Valid partition identifiers
<<Physics>>
<<Transform>>
<<PlayerInput>>
<<RenderingSubsystem>>
<<AI_Behavior>>
<<μετάβλητη>>  // Unicode identifiers allowed

// ❌ Invalid partition identifiers
<<>>           // ERROR: Empty identifier
<<let>>        // ERROR: Reserved keyword
<<123Start>>   // ERROR: Starts with digit
```

### 2.4 Special Partition Identifiers

**Default Partition Reset: `<<_>>`**

The underscore identifier `_` has special meaning: it resets partition assignment to default behavior.

```cursive
record Example {
    <<Physics>>
    velocities: Vec<Velocity>,  // Physics partition

    <<_>>                       // Reset to default
    entities: Vec<Entity>,      // Own partition (Example::entities)
    time: f64,                  // Own partition (Example::time)
}
```

**Rationale:** The underscore follows Rust convention for "default" or "ignored" values.

---

## 3. Default Partitioning Behavior

### 3.1 Default Rule

**NORMATIVE REQUIREMENT:**

> "In the absence of explicit partition directives, each field of a record is assigned its own distinct partition. The partition identifier is the qualified field name: `TypeName::field_name`."

**Formal Specification:**

```
[Partition-Default]
Record type R with field f
No partition directive precedes f
──────────────────────────────
f is assigned partition "R::f"
```

**Example:**

```cursive
record Point {
    x: f64,  // Partition: Point::x
    y: f64,  // Partition: Point::y
}

procedure distance(p: shared Point) -> f64 {
    let dx: f64 = p.x;  // ✅ OK: Access Point::x partition
    let dy: f64 = p.y;  // ✅ OK: Access Point::y partition (different from Point::x)

    result ((dx * dx) + (dy * dy)).sqrt()
}
```

### 3.2 Rationale for Default Behavior

**Why Each Field Is Separate:**

1. **Prevents the actual bug**: Cannot access same field twice (iterator invalidation)
2. **Allows safe patterns**: Different fields CAN be accessed (the common, safe case)
3. **Low annotation burden**: Simple types need no partition directives
4. **ECS pattern fit**: Accessing different component arrays is the primary use case

**Comparison to Alternative Default:**

If all fields shared a type-level partition by default:

```cursive
// ❌ Alternative (rejected): All fields in "Point" partition
record Point {
    x: f64,  // Partition: Point
    y: f64,  // Partition: Point
}

procedure distance(p: shared Point) -> f64 {
    let dx = p.x;  // Locks "Point" partition
    let dy = p.y;  // ❌ ERROR: Point partition already active
    // ...
}
```

This would be overly restrictive and require explicit partitioning for every struct.

### 3.3 Default Partition Identity

**Partition Identity Properties:**

Each default partition is:
- **Unique**: `Record1::field` ≠ `Record2::field` (even if field names match)
- **Stable**: Partition identity does not change across program execution
- **Nominal**: Based on declared type and field name, not structural properties

**Example:**

```cursive
record Position { x: f64, y: f64 }
record Velocity { x: f64, y: f64 }

// Position::x and Velocity::x are DIFFERENT partitions
// (Different types, even though field names match)

procedure update(pos: shared Position, vel: shared Velocity) {
    ref px: shared = pos.x;  // Locks Position::x
    ref vx: shared = vel.x;  // ✅ OK: Velocity::x is different partition
}
```

---

## 4. Partition Scoping Rules

### 4.1 Directive Scope

**NORMATIVE REQUIREMENT:**

> "A partition directive applies to all field declarations that follow it in lexical order, until either (1) another partition directive is encountered, or (2) the record body ends."

**Formal Specification:**

```
[Partition-Directive-Scope]
Partition directive "<<P>>" at position i in record R
Field f declared at position j > i
No partition directive between positions i and j
──────────────────────────────────────────────
Field f is assigned partition P
```

**Example:**

```cursive
record World {
    // No directive: default partitions
    entities: Vec<Entity>,     // Partition: World::entities
    time: f64,                 // Partition: World::time

    // Physics directive starts here
    <<Physics>>
    positions: Vec<Position>,  // Partition: Physics
    velocities: Vec<Velocity>, // Partition: Physics
    forces: Vec<Force>,        // Partition: Physics

    // Combat directive starts here (Physics ends implicitly)
    <<Combat>>
    healths: Vec<Health>,      // Partition: Combat
    damages: Vec<Damage>,      // Partition: Combat
}
```

### 4.2 Multiple Directives with Same Identifier

**Rule: Partition Merging**

If multiple partition directives with the same identifier appear in a record, all fields under those directives belong to the same partition.

```cursive
record World {
    <<Physics>>
    velocities: Vec<Velocity>,  // Partition: Physics

    <<Combat>>
    healths: Vec<Health>,       // Partition: Combat

    <<Physics>>  // Returns to Physics partition
    forces: Vec<Force>,         // Partition: Physics (same as velocities)
}

procedure update(world: shared World) {
    ref vel: shared = world.velocities;  // Locks Physics partition
    ref force: shared = world.forces;    // ❌ ERROR: Physics already locked
}
```

**Formal Specification:**

```
[Partition-Merge]
Partition directive "<<P>>" at position i₁
Partition directive "<<P>>" at position i₂ (i₂ > i₁)
Fields f₁ follows i₁, f₂ follows i₂
──────────────────────────────────────
f₁ and f₂ are in the same partition P
```

**Rationale:**

Allows organizing large structs with partitions grouped logically throughout the definition, rather than requiring contiguous blocks.

### 4.3 Default Partition Reset

**Rule: `<<_>>` Directive**

The special directive `<<_>>` resets to default partitioning behavior. Fields following `<<_>>` are each assigned their own partition.

```cursive
record World {
    <<Physics>>
    velocities: Vec<Velocity>,  // Partition: Physics
    forces: Vec<Force>,         // Partition: Physics

    <<_>>                       // Reset to default
    entities: Vec<Entity>,      // Partition: World::entities
    time: f64,                  // Partition: World::time

    <<Combat>>
    healths: Vec<Health>,       // Partition: Combat
}
```

**Formal Specification:**

```
[Partition-Reset]
Partition directive "<<_>>" at position i in record R
Field f declared at position j > i
No partition directive between positions i and j
──────────────────────────────────────────────
Field f assigned default partition R::f
```

### 4.4 Empty Records

**Rule: No Partition State**

An empty record (no fields) has no partitions.

```cursive
record Empty {
    // No fields, no partitions
}
```

This is trivially safe; no partition checking occurs.

---

## 5. Access Rules and Enforcement

### 5.1 Partition Activation

**NORMATIVE REQUIREMENT:**

> "Accessing a field through a `shared` reference activates that field's partition for the duration of the reference binding's lexical scope."

**Formal Specification:**

```
[Partition-Activate]
Expression e: ref binding to field f through shared reference
Field f in partition P
Scope S of ref binding
───────────────────────────────────────
Partition P is ACTIVE in scope S
```

**Example:**

```cursive
record World {
    positions: Vec<Position>,  // Partition: World::positions
}

procedure update(world: shared World) {
    {
        ref pos: shared = world.positions;  // Activates World::positions
        // World::positions is ACTIVE in this scope
    }  // pos goes out of scope, World::positions becomes INACTIVE

    ref pos2: shared = world.positions;  // ✅ OK: Not active anymore
}
```

### 5.2 Partition Conflict Detection

**NORMATIVE REQUIREMENT:**

> "Attempting to access a field whose partition is already active in the current scope is a compile-time error."

**Formal Specification:**

```
[Partition-Conflict]
Expression e attempts to access field f through shared reference
Field f in partition P
Partition P already ACTIVE in current scope S
───────────────────────────────────────────────
COMPILE ERROR E4024: Partition already active
```

**Example:**

```cursive
record World {
    <<Physics>>
    positions: Vec<Position>,
    velocities: Vec<Velocity>,
}

procedure update(world: shared World) {
    ref pos: shared = world.positions;  // Activates Physics partition
    ref vel: shared = world.velocities; // ❌ ERROR E4024: Physics partition already active
}
```

### 5.3 Partition Independence

**Rule: Different Partitions Are Independent**

Accessing fields from different partitions is permitted; partitions are independent.

```cursive
record World {
    <<Physics>>
    positions: Vec<Position>,   // Partition: Physics

    <<Combat>>
    healths: Vec<Health>,       // Partition: Combat
}

procedure update(world: shared World) {
    ref pos: shared = world.positions;  // Activates Physics
    ref hp: shared = world.healths;     // ✅ OK: Combat is separate partition
}
```

**Formal Specification:**

```
[Partition-Independence]
Partition P₁ active in scope S
Expression e accesses field f in partition P₂
P₁ ≠ P₂
───────────────────────────────────────
Access to f is PERMITTED
```

### 5.4 Lexical Scope Lifetime

**Rule: Partition Activation Lifetime**

A partition remains active for the **lexical scope** of the `ref` binding that activated it.

```cursive
procedure example(world: shared World) {
    {
        ref pos: shared = world.positions;  // Activates World::positions
        // World::positions is ACTIVE here
    }  // pos out of scope -> World::positions becomes INACTIVE

    ref pos2: shared = world.positions;  // ✅ OK: New scope, partition not active
}
```

**Formal Specification:**

```
[Partition-Lifetime]
Ref binding r to field f in partition P
Lexical scope S_r of binding r
───────────────────────────────────────
P is ACTIVE in S_r
P is INACTIVE outside S_r
```

### 5.5 Interaction with Other Permissions

**Rule: Partitioning Only Applies to `shared` Permission**

Partition checking only applies when accessing fields through `shared` references. Access through `unique` or `const` references does not activate partitions.

```cursive
record World {
    <<Physics>>
    positions: Vec<Position>,
    velocities: Vec<Velocity>,
}

procedure with_unique(world: unique World) {
    ref pos: unique = world.positions;  // No partition activation
    ref vel: unique = world.velocities; // ✅ OK: unique has exclusive access already
}

procedure with_const(world: const World) {
    let pos_val: const Vec<Position> = world.positions;  // No partition activation
    let vel_val: const Vec<Velocity> = world.velocities; // ✅ OK: const is read-only
}
```

**Rationale:**

- **`unique`**: Already provides exclusive access; cannot have multiple references
- **`const`**: Read-only access; no mutation safety concerns

**Formal Specification:**

```
[Partition-Shared-Only]
Access to field f through reference r
Permission of r is unique or const
───────────────────────────────────
No partition activation or checking occurs
```

### 5.6 Nested Scopes

**Rule: Partition State Is Scope-Local**

Each nested scope has its own partition activation state. Inner scopes inherit active partitions from outer scopes.

```cursive
procedure nested(world: shared World) {
    ref pos: shared = world.positions;  // Activates World::positions in outer scope

    {
        // Inner scope: World::positions is STILL active (inherited)
        ref pos2: shared = world.positions;  // ❌ ERROR: Partition already active
    }

    // World::positions still active in outer scope
}
```

**Formal Specification:**

```
[Partition-Scope-Inheritance]
Scope S_inner nested in scope S_outer
Partition P active in S_outer
───────────────────────────────────
P is active in S_inner (inherited)
```

---

## 6. Formal Semantics

### 6.1 Partition Assignment

**Partition Assignment Function:**

Define `partition_of(R, f)` as the partition assignment function:

```
partition_of(R, f) = P
```

Where:
- `R` is a record type
- `f` is a field name in `R`
- `P` is the partition identifier assigned to field `f`

**Default Assignment:**

```
[Partition-Assign-Default]
Record R with field f
No directive precedes f in R's definition
─────────────────────────────────────
partition_of(R, f) = "R::f"
```

**Directive Assignment:**

```
[Partition-Assign-Directive]
Record R with field f
Partition directive "<<P>>" precedes f
No intervening directive or <<_>> between directive and f
────────────────────────────────────────────────────
partition_of(R, f) = P
```

**Reset Assignment:**

```
[Partition-Assign-Reset]
Record R with field f
Partition directive "<<_>>" precedes f
No intervening directive between <<_>> and f
────────────────────────────────────────
partition_of(R, f) = "R::f"
```

### 6.2 Partition Activation Semantics

**Active Partition Set:**

Each scope `S` has an associated set `ActivePartitions(S)` containing partitions active in that scope.

```
ActivePartitions(S) <: Partitions
```

**Initial State:**

```
[Active-Initial]
Scope S begins
──────────────────────────────
ActivePartitions(S) = ∅
```

**Activation on Field Access:**

```
[Active-Access]
Scope S
Expression: ref r: shared = container.field
partition_of(typeof(container), field) = P
P ∉ ActivePartitions(S)
───────────────────────────────────────────
ActivePartitions(S) := ActivePartitions(S) ∪ {P}
```

**Conflict Detection:**

```
[Active-Conflict]
Scope S
Expression: ref r: shared = container.field
partition_of(typeof(container), field) = P
P ∈ ActivePartitions(S)
───────────────────────────────────────────
ERROR E4024: Partition P already active in scope S
```

### 6.3 Scope Inheritance

**Nested Scope Inheritance:**

```
[Scope-Inherit]
Scope S_inner nested in S_outer
───────────────────────────────────────────────
ActivePartitions(S_inner) ⊇ ActivePartitions(S_outer)
```

Initially, `ActivePartitions(S_inner) = ActivePartitions(S_outer)` (inherited), then new activations in `S_inner` add to this set.

### 6.4 Type System Integration

**Partition Checking as Compiler Phase:**

Partition checking occurs **after** type checking and permission checking:

```
1. Lexical Analysis
2. Parsing
3. Type Checking
4. Permission Checking (unique/shared/const rules)
5. Partition Checking ← New phase
6. Code Generation
```

**Permission Check First:**

```
[Check-Order]
Expression: ref r: perm = container.field
───────────────────────────────────────────
1. Check: perm is compatible with container's permission (existing rules)
2. If perm == shared: Check partition activation (new rule)
```

**Example:**

```cursive
record World {
    positions: unique Vec<Position>,
}

let world: shared = get_world();
ref pos: shared = world.positions;  // ❌ ERROR E4022: Cannot access unique field through shared container

// Partition checking is not reached; permission check fails first
```

---

## 7. Examples and Patterns

### 7.1 Entity Component System (Primary Use Case)

**Typical ECS World:**

```cursive
record World {
    // Default partitions (separate)
    entities: unique Vec<Entity>,
    entity_count: shared usize,

    // Transform components
    <<Transform>>
    positions: shared Vec<Position>,
    rotations: shared Vec<Rotation>,
    scales: shared Vec<Scale>,

    // Physics components
    <<Physics>>
    velocities: shared Vec<Velocity>,
    accelerations: shared Vec<Acceleration>,
    masses: shared Vec<Mass>,

    // Rendering components
    <<Rendering>>
    meshes: shared Vec<Mesh>,
    materials: shared Vec<Material>,
    visible_flags: shared Vec<bool>,

    // Gameplay components
    <<Gameplay>>
    healths: shared Vec<Health>,
    inventories: shared Vec<Inventory>,
    ai_states: shared Vec<AIState>,
}
```

**Physics System:**

```cursive
procedure physics_system(world: shared World, dt: f32) {
    // ✅ Can access Transform and Physics partitions simultaneously
    ref pos: shared = world.positions;      // Activates Transform
    ref vel: shared = world.velocities;     // Activates Physics
    ref accel: shared = world.accelerations; // ✅ OK: Same Physics partition

    // Update positions based on velocities
    for i in 0..pos.len() {
        pos[i] = pos[i] + vel[i] * dt + 0.5 * accel[i] * dt * dt;
    }
}
```

**Rendering System:**

```cursive
procedure render_system(world: shared World, camera: Camera) {
    // ✅ Can access Transform and Rendering partitions
    ref pos: shared = world.positions;      // Activates Transform
    ref rot: shared = world.rotations;      // ❌ ERROR: Transform already active
}

// Fixed version:
procedure render_system_fixed(world: shared World, camera: Camera) {
    // Access Transform fields in separate scopes or re-architect
    {
        ref pos: shared = world.positions;
        // Use positions here
    }
    {
        ref rot: shared = world.rotations;
        // Use rotations here
    }

    // Or: redesign to use single Transform component containing all data
}
```

### 7.2 Configuration with Derived Fields

**Configuration with Cached Values:**

```cursive
record AppConfig {
    // Source data (separate partition)
    source_file: shared String,
    raw_data: shared Vec<u8>,

    // Derived/cached values (grouped partition)
    <<Derived>>
    cached_sum: shared i64,
    cached_product: shared i64,
    cached_hash: shared u64,

    // These must be recomputed together if source_data changes
}

procedure update_config(config: shared AppConfig, new_data: Vec<u8>) {
    ref raw: shared = config.raw_data;  // ✅ OK: raw_data is separate
    *raw = new_data;

    // ❌ ERROR: Cannot access both cached values simultaneously
    // This is GOOD: forces you to recompute all derived values together
    ref sum: shared = config.cached_sum;
    ref prod: shared = config.cached_product;  // ❌ ERROR: Derived partition active
}

procedure recompute_derived(config: shared AppConfig) {
    // Access source data
    ref raw: shared = config.raw_data;

    // Compute new values
    let new_sum = compute_sum(raw);
    let new_product = compute_product(raw);
    let new_hash = compute_hash(raw);

    // Update derived values (in separate scopes to avoid partition conflict)
    {
        ref sum: shared = config.cached_sum;
        *sum = new_sum;
    }
    {
        ref prod: shared = config.cached_product;
        *prod = new_product;
    }
    {
        ref hash: shared = config.cached_hash;
        *hash = new_hash;
    }
}
```

### 7.3 Simple Types (No Partitions Needed)

**Point Type:**

```cursive
record Point {
    x: f64,  // Partition: Point::x
    y: f64,  // Partition: Point::y
}

procedure distance(p1: shared Point, p2: shared Point) -> f64 {
    // ✅ Can access x and y of each point
    let dx = p2.x - p1.x;  // Activates Point::x for p2
    let dy = p2.y - p1.y;  // Activates Point::y for p2 (different partition)

    result ((dx * dx) + (dy * dy)).sqrt()
}
```

**No annotations needed!** Default partitioning works perfectly for simple types.

### 7.4 Graph Structures

**Graph with Adjacency Lists:**

```cursive
record Graph {
    // Each node's data is separate partition
    node_data: shared Vec<NodeData>,

    // Edge lists grouped if mutual exclusion desired
    <<Edges>>
    adjacency_out: shared Vec<Vec<NodeId>>,
    adjacency_in: shared Vec<Vec<NodeId>>,
}

procedure add_edge(graph: shared Graph, from: NodeId, to: NodeId) {
    ref out: shared = graph.adjacency_out;  // Activates Edges
    ref in_adj: shared = graph.adjacency_in; // ❌ ERROR: Edges already active

    // Must access in separate scopes or refactor
}

// Alternative: Separate partitions if independent access needed
record GraphSeparate {
    node_data: shared Vec<NodeData>,

    <<OutEdges>>
    adjacency_out: shared Vec<Vec<NodeId>>,

    <<InEdges>>
    adjacency_in: shared Vec<Vec<NodeId>>,
}

procedure add_edge_separate(graph: shared GraphSeparate, from: NodeId, to: NodeId) {
    ref out: shared = graph.adjacency_out;   // Activates OutEdges
    ref in_adj: shared = graph.adjacency_in; // ✅ OK: InEdges is separate
}
```

### 7.5 Partition Merging Example

**Non-Contiguous Partitions:**

```cursive
record GameState {
    // Player input partition
    <<Input>>
    keyboard_state: shared KeyboardState,
    mouse_state: shared MouseState,

    // Rendering state
    <<Rendering>>
    frame_buffer: shared FrameBuffer,

    // More input (merged with first Input block)
    <<Input>>
    gamepad_state: shared GamepadState,
    touch_state: shared TouchState,
}

procedure process_input(state: shared GameState) {
    ref kb: shared = state.keyboard_state;   // Activates Input
    ref gp: shared = state.gamepad_state;    // ❌ ERROR: Input already active
    // Both are in same Input partition
}
```

---

## 8. Error Codes and Diagnostics

### 8.1 Error Code Catalog

#### E4024: Partition Already Active

**Description:** Attempted to access a field whose partition is already active in the current scope.

**Example:**

```cursive
record World {
    <<Physics>>
    positions: Vec<Position>,
    velocities: Vec<Velocity>,
}

procedure update(world: shared World) {
    ref pos: shared = world.positions;
    ref vel: shared = world.velocities;  // ❌ ERROR E4024
}
```

**Diagnostic:**

```
error[E4024]: partition 'Physics' already active in this scope
  --> src/game.rs:42:15
   |
40 |     ref pos: shared = world.positions;
   |                       --------------- partition 'Physics' first accessed here
41 |
42 |     ref vel: shared = world.velocities;
   |                       ^^^^^^^^^^^^^^^^ cannot access field in active partition
   |
   = note: fields 'positions' and 'velocities' are both in partition 'Physics'
   = help: access these fields in separate scopes or redesign to avoid simultaneous access
   = help: see: Field-Level-Partitioning.md section 5.2
```

**Resolution:**

1. **Separate scopes**: Access fields in different lexical scopes
2. **Redesign**: Put fields in different partitions if independent access is needed
3. **Sequential access**: Use one field, finish, then use the other

#### E4025: Invalid Partition Directive

**Description:** Partition directive syntax error or invalid partition identifier.

**Example:**

```cursive
record World {
    <<>>  // ❌ ERROR E4025: Empty partition identifier
    positions: Vec<Position>,
}
```

**Diagnostic:**

```
error[E4025]: invalid partition directive
  --> src/world.rs:12:5
   |
12 |     <<>>
   |     ^^^^ partition identifier cannot be empty
   |
   = help: use a valid identifier: <<PartitionName>>
   = help: or use <<_>> to reset to default partitioning
```

**Common Causes:**

- Empty identifier: `<<>>`
- Reserved keyword: `<<let>>`
- Invalid characters: `<<123Invalid>>`
- Missing delimiters: `<Partition>` (single angle brackets)

#### E4026: Partition Directive Outside Record

**Description:** Partition directive used in invalid context (only valid within record bodies).

**Example:**

```cursive
<<Invalid>>  // ❌ ERROR E4026: Not inside record
function foo() {}
```

**Diagnostic:**

```
error[E4026]: partition directive used outside record body
  --> src/example.rs:5:1
   |
5 | <<Invalid>>
  | ^^^^^^^^^^^ partition directives are only valid inside record definitions
  |
   = note: partition directives control field grouping within records
   = help: move this directive into a record body
```

### 8.2 Warning Diagnostics

#### W4001: Unused Partition

**Description:** A partition directive is declared but no fields follow it before the record ends or another directive appears.

**Example:**

```cursive
record World {
    positions: Vec<Position>,

    <<Physics>>
    // No fields follow before record ends
}
```

**Diagnostic:**

```
warning[W4001]: unused partition directive 'Physics'
  --> src/world.rs:15:5
   |
15 |     <<Physics>>
   |     ^^^^^^^^^^^ partition directive has no effect
   |
   = note: no fields are assigned to this partition
   = help: remove this directive or add fields after it
```

#### W4002: Single-Field Partition

**Description:** A partition contains only one field (partitioning has no effect).

**Example:**

```cursive
record World {
    <<Physics>>
    positions: Vec<Position>,  // Only field in Physics partition

    <<Combat>>
    healths: Vec<Health>,
}
```

**Diagnostic:**

```
warning[W4002]: partition 'Physics' contains only one field
  --> src/world.rs:12:5
   |
12 |     <<Physics>>
   |     ^^^^^^^^^^^ this partition contains only 'positions'
   |
   = note: partitioning a single field has no effect
   = help: remove this directive to use default partitioning
   = help: or add more fields to this partition if grouping is intended
```

---

## 9. Implementation Guidance

### 9.1 Compiler Phases

**Partition Checking Phase:**

Partition checking is implemented as a separate compiler phase after permission checking:

```
┌─────────────────────────────────────────────┐
│ 1. Lexical Analysis                         │
│    - Tokenize <<, >>, identifiers           │
└─────────────────────────────────────────────┘
                   ↓
┌─────────────────────────────────────────────┐
│ 2. Parsing                                  │
│    - Parse partition directives in records  │
│    - Build AST with partition annotations   │
└─────────────────────────────────────────────┘
                   ↓
┌─────────────────────────────────────────────┐
│ 3. Type Checking                            │
│    - Resolve types                          │
│    - Build type system representation       │
└─────────────────────────────────────────────┘
                   ↓
┌─────────────────────────────────────────────┐
│ 4. Permission Checking                      │
│    - Check unique/shared/const rules        │
│    - Verify field access permissions        │
└─────────────────────────────────────────────┘
                   ↓
┌─────────────────────────────────────────────┐
│ 5. Partition Assignment                     │
│    - Assign partition IDs to fields         │
│    - Build partition metadata               │
└─────────────────────────────────────────────┘
                   ↓
┌─────────────────────────────────────────────┐
│ 6. Partition Checking                       │
│    - Track active partitions per scope      │
│    - Detect partition conflicts             │
│    - Emit errors for violations             │
└─────────────────────────────────────────────┘
                   ↓
┌─────────────────────────────────────────────┐
│ 7. Code Generation                          │
│    - No runtime partition checks            │
│    - Zero runtime overhead                  │
└─────────────────────────────────────────────┘
```

### 9.2 Data Structures

**Partition Assignment Map:**

```rust
// Pseudo-code representation
struct RecordMetadata {
    type_name: String,
    fields: Vec<FieldMetadata>,
}

struct FieldMetadata {
    field_name: String,
    partition_id: PartitionId,
    // ... other metadata
}

enum PartitionId {
    Default(QualifiedFieldName),  // e.g., "World::positions"
    Explicit(String),               // e.g., "Physics"
}
```

**Active Partition Set Per Scope:**

```rust
struct ScopeState {
    active_partitions: HashSet<PartitionId>,
    parent_scope: Option<Box<ScopeState>>,
}

impl ScopeState {
    fn is_partition_active(&self, partition: &PartitionId) -> bool {
        self.active_partitions.contains(partition)
            || self.parent_scope
                .as_ref()
                .map(|parent| parent.is_partition_active(partition))
                .unwrap_or(false)
    }

    fn activate_partition(&mut self, partition: PartitionId) {
        self.active_partitions.insert(partition);
    }
}
```

### 9.3 Partition Assignment Algorithm

**Algorithm: Assign Partitions to Record Fields**

```
Input: Record definition R with fields F₁, F₂, ..., Fₙ and directives D₁, D₂, ..., Dₘ
Output: Map from field name to partition ID

1. Initialize: current_partition = None
2. For each item in R (in lexical order):
   a. If item is partition directive <<P>>:
      - Set current_partition = P
   b. If item is partition directive <<_>>:
      - Set current_partition = None
   c. If item is field declaration f:
      - If current_partition is None:
          - Assign partition_of(R, f) = "R::f"  (default)
      - Else:
          - Assign partition_of(R, f) = current_partition
3. Return partition assignment map
```

**Example Execution:**

```cursive
record World {
    entities: Vec<Entity>,     // current_partition = None → World::entities
    time: f64,                  // current_partition = None → World::time

    <<Physics>>                 // current_partition = "Physics"
    positions: Vec<Position>,   // current_partition = "Physics" → Physics
    velocities: Vec<Velocity>,  // current_partition = "Physics" → Physics

    <<_>>                       // current_partition = None
    debug: String,              // current_partition = None → World::debug
}
```

Result:
```
entities   → World::entities
time       → World::time
positions  → Physics
velocities → Physics
debug      → World::debug
```

### 9.4 Partition Checking Algorithm

**Algorithm: Check Partition Conflicts in Function Body**

```
Input: Function body AST, record metadata
Output: List of errors (if any)

1. Initialize: scope_stack = [empty ScopeState]
2. For each statement S in function body:
   a. If S introduces new scope (block, loop, etc.):
      - Push new ScopeState inheriting from current scope
   b. If S exits scope:
      - Pop ScopeState from stack
   c. If S is field access through shared reference:
      - Let f be the field being accessed
      - Let P = partition_of(typeof(container), f)
      - Let current_scope = scope_stack.top()
      - If current_scope.is_partition_active(P):
          - Emit ERROR E4024 (partition already active)
      - Else:
          - current_scope.activate_partition(P)
   d. Recursively process nested expressions
3. Return collected errors
```

### 9.5 Runtime Overhead Analysis

**Compile-Time Only:**

- ✅ Partition assignment: Compile-time only (stored in metadata)
- ✅ Partition checking: Compile-time only (no runtime checks)
- ✅ No runtime data structures: No `ActivePartitions` at runtime
- ✅ No runtime overhead: Zero cost abstraction

**Code Generation:**

Generated code is **identical** to code without partition checking:

```cursive
// Source code
ref pos: shared = world.positions;
```

```asm
; Generated assembly (no difference with or without partitions)
mov rax, [world_ptr + offset_positions]
; No partition checks in generated code
```

**Memory Layout:**

Record layout is **unaffected** by partitions:

```cursive
record World {
    <<Physics>>
    positions: Vec<Position>,   // Offset: 0
    velocities: Vec<Velocity>,  // Offset: 24 (assuming Vec is 24 bytes)
}

// Memory layout is same as:
record WorldNoPartitions {
    positions: Vec<Position>,   // Offset: 0
    velocities: Vec<Velocity>,  // Offset: 24
}
```

---

## 10. Conformance Requirements

### 10.1 Normative Requirements

A conforming implementation **MUST**:

1. **Recognize partition directives**: Support `<<Identifier>>` syntax in record bodies
2. **Recognize default partition reset**: Support `<<_>>` directive
3. **Assign default partitions**: Each field without directive gets own partition `RecordType::field_name`
4. **Assign explicit partitions**: Fields following `<<P>>` are assigned partition `P`
5. **Detect partition conflicts**: Emit error when accessing same partition twice through `shared` in same scope
6. **Respect lexical scoping**: Partition activation limited to ref binding's lexical scope
7. **Inherit partition state**: Nested scopes inherit active partitions from parent scopes
8. **Allow partition merging**: Multiple `<<P>>` directives assign fields to same partition `P`
9. **Apply only to `shared`**: Partition checking only for shared permission, not unique or const
10. **Zero runtime overhead**: No runtime partition checks or data structures

### 10.2 Recommended Practices

A conforming implementation **SHOULD**:

1. **Emit clear diagnostics**: Error messages should identify both conflicting accesses with source locations
2. **Suggest resolutions**: Help messages should suggest scope separation or redesign
3. **Warn for unused partitions**: Emit warning for partition directives with no following fields (W4001)
4. **Warn for single-field partitions**: Emit warning for partitions containing only one field (W4002)
5. **Provide partition metadata**: Make partition assignments available for tooling (IDE, docs)
6. **Optimize partition checking**: Use efficient data structures for partition conflict detection

### 10.3 Optional Extensions

A conforming implementation **MAY**:

1. **Hierarchical partitions**: Support nested partition identifiers (e.g., `<<Physics::Kinematics>>`)
2. **Partition parameters**: Support parameterized partitions for generic types
3. **Cross-module partitions**: Allow partition names to span module boundaries
4. **Partition inference**: Infer partition groupings based on access patterns (diagnostic mode only)
5. **Visualization tools**: Provide tools to visualize partition structure in large records

### 10.4 Non-Conformance

A conforming implementation **MUST NOT**:

1. **Add runtime overhead**: No runtime checks, data structures, or performance cost
2. **Change memory layout**: Partitions must not affect record field layout
3. **Require annotations**: Default partitioning must work without any directives
4. **Break lexical scoping**: Partition state must respect lexical scope boundaries
5. **Affect code generation**: Generated code must be identical with or without partition checking (except for rejected programs)

---

## 11. Related Work and Alternatives

### 11.1 Comparison to Rust Borrowing

**Rust's Approach:**

```rust
// Rust: Borrow checker prevents simultaneous mutable references
let mut world = World::new();
let pos = &mut world.positions;   // Mutable borrow of whole `world`
let vel = &mut world.velocities;  // ❌ ERROR: world already borrowed
```

Rust's borrow checker is more powerful but requires:
- Lifetime annotations (`'a`)
- Borrow checker compiler phase
- Complex error messages
- Non-local reasoning in many cases

**Cursive's Approach:**

```cursive
// Cursive: Field-level partitions allow finer granularity
let world: shared = World::new()
ref pos: shared = world.positions   // Locks positions partition only
ref vel: shared = world.velocities  // ✅ OK: Different partition
```

Cursive's partition system is:
- Simpler (no lifetimes, no borrow checker)
- More flexible for ECS patterns (field-level granularity)
- Less powerful (no borrowing semantics, no lifetime tracking)

### 11.2 Comparison to Disjoint Borrows

Some borrow checkers support "disjoint borrows" of different fields:

```rust
// Rust (with disjoint borrows)
let pos = &mut world.positions;
let vel = &mut world.velocities;  // ✅ OK: Different fields
```

**Cursive's advantage:** No borrow checker needed; partition checking is simpler and fits Cursive's design goals.

### 11.3 Alternatives Considered and Rejected

**1. Fractional Permissions:**

"Split `unique` permission into multiple `const` views."

Rejected: This is borrowing with different terminology. Requires borrow checker.

**2. Runtime Tracking:**

Add runtime checks for partition conflicts.

Rejected: Violates "zero runtime overhead" principle.

**3. Type-Level Partition by Default:**

All fields share the record's partition unless explicitly separated.

Rejected: Too restrictive; prevents safe patterns like accessing different ECS component arrays.

**4. Element-Level Partitioning:**

Track which elements (e.g., `positions[0]` vs `positions[1]`) are accessed.

Rejected: Requires dependent types or borrow checker complexity. Field-level suffices for ECS use case.

---

## 12. Future Directions

### 12.1 Potential Extensions

**Hierarchical Partitions:**

```cursive
record World {
    <<Physics::Kinematics>>
    positions: Vec<Position>,
    velocities: Vec<Velocity>,

    <<Physics::Dynamics>>
    forces: Vec<Force>,
    masses: Vec<Mass>,
}

// Could access Physics::Kinematics and Physics::Dynamics simultaneously
// but not two fields from same leaf partition
```

**Partition Parameters for Generics:**

```cursive
record Container<T, <<P>>> {
    data: Vec<T> #P,
}
```

**Partition Inference (Diagnostic Mode):**

Compiler suggests partition groupings based on observed access patterns.

### 12.2 Research Questions

1. **Concurrency Integration**: How do partitions interact with thread-safety predicates (Send/Sync)?
2. **Partition Algebra**: Can partition union/intersection operations be useful?
3. **Automatic Partitioning**: Can the compiler automatically infer optimal partitions?
4. **Partition Contracts**: Can functions specify partition requirements in signatures?

---

## 13. Acknowledgments

This specification addresses the safety concerns with the `shared` permission documented in `DESIGN_DECISIONS_2025_11.md` (line 433: "Iterator invalidation possible with shared").

The design was influenced by:
- Rust's disjoint field borrows
- C++ access specifiers (directive-based scoping)
- ECS architecture patterns in game development
- Cursive's core design principles (explicit, local reasoning, zero overhead)

---

## 14. References

- **[Memory Model Specification](CURSIVE_MEMORY_MODEL_SPECIFICATION.md)**: Section 3.3 (Shared Permission)
- **[Design Decisions](../DESIGN_DECISIONS_2025_11.md)**: Lines 432-434 (Shared permission safety concerns)
- **[Lexical Permissions](../04_Lexical-Permissions.md)**: Permission system overview
- **[Lexical Elements](../02_Basics-and-Program-Model/02-2_Lexical-Elements.md)**: Section 2.2.8 (Operators and Punctuators)

---

**END OF SPECIFICATION: Field-Level Partitioning**

**Version**: 1.0
**Status**: Draft
**Last Updated**: 2025-11-03
