# Cursive Language Specification

## Clause 12 — Witness System

**Clause**: 12 — Witness System
**File**: 13-6_Memory-Interplay.md
**Section**: §12.6 Regions, Permissions, and Grants Interplay
**Stable label**: [witness.memory]  
**Forward references**: §13.3 [witness.formation], §13.5 [witness.dispatch], Clause 11 §11.2 [memory.object], Clause 11 §11.3 [memory.region], Clause 11 §11.4 [memory.permission], Clause 11 §11.5 [memory.move]

---

### §12.6 Regions, Permissions, and Grants Interplay [witness.memory]

#### §12.6.1 Overview

[1] This subclause specifies how witnesses integrate with Cursive's memory model: cleanup responsibility (§11.2), region allocation (§11.3), permissions (§11.4), and move semantics (§11.5). Witnesses follow the same memory safety rules as all Cursive values.

[2] The three witness allocation states (@Stack, @Region, @Heap) map directly to cleanup responsibility and region integration, maintaining consistency with the broader memory model.

#### §12.6.2 Cleanup Responsibility [witness.memory.responsibility]

[3] Witness cleanup responsibility is determined by allocation state:

**Table 13.3 — Witness cleanup responsibility**

| Allocation State | Cleanup Responsibility | Destructor Called    | Move Required | Binding Analogy      |
| ---------------- | ---------------------- | -------------------- | ------------- | -------------------- |
| @Stack           | Non-responsible        | NO                   | NO            | `let w <- value`     |
| @Region          | Responsible            | YES (at region exit) | YES           | `let w = move value` |
| @Heap            | Responsible            | YES (at scope exit)  | YES           | `let w = move value` |

[4] **RAII integration**:

$$
\frac{w : \texttt{witness}\langle B \rangle@\texttt{Heap} \text{ responsible}}
     {\text{At scope exit: call } w\text{.drop()}}
\tag{P-Witness-RAII}
$$

[5] Responsible witnesses (@Heap, @Region) invoke the underlying value's destructor when they are destroyed. Non-responsible witnesses (@Stack) do not invoke destructors.

**Example 13.6.2.1 (Cleanup responsibility)**:

```cursive
procedure demo_cleanup()
    [[ alloc::heap, io::write |- true => true ]]
{
    let data = Resource::new()

    // Stack witness: Non-responsible
    let stack_w: witness<Display>@Stack = data
    // data retains cleanup responsibility

    {
        let shape = Shape::new()
        let heap_w: witness<Display>@Heap = move shape
        // heap_w is responsible
        // shape is invalid (moved)
    }
    // Scope exit: heap_w's destructor called (Shape::drop)

    // Outer scope exit: data's destructor called
    //                   stack_w does NOT call destructor (non-responsible)
}
```

#### §12.6.3 Region Integration [witness.memory.regions]

##### §12.6.3.1 Region Witness Lifetime

[6] Region witnesses (`@Region`) are allocated in the active region and destroyed when the region exits:

**Example 13.6.3.1 (Region witness lifecycle)**:

```cursive
procedure process_batch(items: [Item])
    [[ alloc::region, io::write |- true => true ]]
{
    region batch {
        let processors: [witness<Processor>@Region] = []

        loop item: Item in items {
            let processor = create_processor(item)
            let w: witness<Processor>@Region = move processor
            processors.push(w)
        }

        // Use processors
        loop p: witness<Processor>@Region in processors {
            p::process()
        }
    }
    // Region exit: All region witnesses destroyed in O(1)
}
```

##### §12.6.3.2 Region Escape Prevention

[7] Region witnesses cannot escape their region, consistent with region escape analysis (§11.3.3):

$$
\frac{w : \texttt{witness}\langle B \rangle@\texttt{Region} \quad \text{region } r \text{ active}}
     {\text{Escaping } w \text{ from } r \Rightarrow \text{ERROR E13-011}}
\tag{WF-Witness-Region-NoEscape}
$$

[8] Attempting to return region witnesses or store them in outer scopes produces diagnostic E13-011.

**Example 13.6.3.2 (Region escape prevention)**:

```cursive
procedure invalid_escape(): witness<Display>@Region
    [[ alloc::region |- true => true ]]
{
    region temp {
        let shape = Circle { radius: 5.0 }
        let w: witness<Display>@Region = move shape
        result w  // ❌ E13-011: region witness escapes region
    }
}
```

##### §12.6.3.3 Heap Conversion

[9] Region witnesses may explicitly convert to heap witnesses via `to_heap()` transition:

```cursive
region temp {
    let shape = Circle { radius: 5.0 }
    let region_w: witness<Display>@Region = move shape

    let heap_w: witness<Display>@Heap = region_w::to_heap()  // Escape to heap
    result heap_w  // ✅ OK: heap witness can escape
}
```

[10] The `to_heap()` transition requires `alloc::heap` grant.

#### §12.6.4 Permission Inheritance [witness.memory.permissions]

[11] Witnesses inherit permissions from the source value at construction and maintain those permissions throughout their lifetime:

$$
\frac{v : \texttt{perm } T \quad w : \texttt{witness}\langle B \rangle \text{ from } v}
     {w : \texttt{perm witness}\langle B \rangle}
\tag{Witness-Perm-Inherit}
$$

[12] Permission qualifiers apply to the entire witness value:

**Example 13.6.4.1 (Permission on witnesses)**:

```cursive
// Unique witness
let point: unique Point = Point::new()
let w1: unique witness<Display> = point
w1::mutate()  // ✅ OK if Display has mutating methods

// Const witness
let shape: const Circle = get_circle()
let w2: const witness<Display> = shape
// w2::mutate()  // ❌ ERROR: const permission prohibits mutation

// Shared witness
let data: shared Buffer = get_shared_buffer()
let w3: shared witness<Processor> = data
w3::process()  // Shared access to process method
```

##### §12.6.4.1 Permission Downgrades

[13] Witnesses follow the permission lattice (§11.4.3):

$$
\texttt{unique witness}\langle B \rangle <: \texttt{shared witness}\langle B \rangle <: \texttt{const witness}\langle B \rangle
$$

[14] Permission downgrades are implicit; upgrades are forbidden.

**Example 13.6.4.2 (Permission downgrades)**:

```cursive
let unique_w: unique witness<Display> = shape
let shared_w: shared witness<Display> = unique_w  // ✅ Downgrade
let const_w: const witness<Display> = shared_w    // ✅ Downgrade

// let back_to_unique: unique witness<Display> = const_w  // ❌ Cannot upgrade
```

#### §12.6.5 Move Semantics [witness.memory.move]

[15] Heap and region witnesses require explicit `move` keyword at construction, following the move semantics in §11.5:

**Example 13.6.5.1 (Explicit move for responsible witnesses)**:

```cursive
let value = Type::new()

// Stack: No move (non-responsible)
let w1: witness<B>@Stack = value
// value still valid

// Heap: Requires move (responsible)
let value2 = Type::new()
let w2: witness<B>@Heap = move value2
// value2 invalid (moved)

// Region: Requires move (responsible)
region temp {
    let value3 = Type::new()
    let w3: witness<B>@Region = move value3
    // value3 invalid (moved)
}
```

[16] Omitting `move` for @Heap/@Region witnesses produces diagnostic E13-010.

#### §12.6.6 Grant Requirements [witness.memory.grants]

[17] Witness construction requires grants based on allocation state:

**Table 13.4 — Witness construction grants**

| Allocation State | Required Grant  | Reason                 |
| ---------------- | --------------- | ---------------------- |
| @Stack           | None            | Only creates reference |
| @Region          | `alloc::region` | Allocates in region    |
| @Heap            | `alloc::heap`   | Allocates on heap      |

[18] Missing grants produce diagnostic E13-012.

**Example 13.6.6.1 (Grant requirements)**:

```cursive
procedure make_stack_witness(shape: Shape): witness<Display>@Stack
    [[ |- true => true ]]  // No grants needed
{
    result shape  // Implicit: witness::stack coercion
}

procedure make_heap_witness(shape: Shape): witness<Display>@Heap
    [[ alloc::heap |- true => true ]]  // Requires alloc::heap
{
    result move shape  // Implicit: witness::heap coercion with allocation
}

procedure make_region_witness(shape: Shape): witness<Display>@Region
    [[ alloc::region |- true => true ]]  // Requires alloc::region
{
    result move shape  // Implicit: witness::region coercion with allocation
}
```

#### §12.6.7 Witness Transitions [witness.memory.transitions]

[19] Witnesses support state transitions following modal type semantics (§7.6):

**Transition methods**:

```cursive
// Built-in transitions
@Stack::to_region(~): witness<B>@Region      // Stack → Region
@Stack::to_heap(~): witness<B>@Heap          // Stack → Heap
@Region::to_heap(~): witness<B>@Heap         // Region → Heap
```

[20] Transitions create new witnesses with different allocation states. Each transition has grant requirements:

- `to_region()`: Requires `alloc::region`
- `to_heap()`: Requires `alloc::heap`

**Example 13.6.7.1 (Witness transitions)**:

```cursive
procedure extend_witness_lifetime(shape: Shape): witness<Display>@Heap
    [[ alloc::heap |- true => true ]]
{
    let stack_w: witness<Display>@Stack = shape

    let heap_w = stack_w::to_heap()  // Transition to heap
    result heap_w  // Can escape scope
}
```

#### §12.6.8 Conformance Requirements [witness.memory.requirements]

[21] Implementations shall:

1. Map witness allocation states to cleanup responsibility per Table 13.3
2. Invoke destructors for responsible witnesses (@Heap, @Region)
3. Skip destructors for non-responsible witnesses (@Stack)
4. Enforce explicit `move` for @Heap/@Region witness creation
5. Integrate region witnesses with region escape analysis
6. Prevent region witness escape with diagnostic E13-011
7. Inherit permissions from source values to witnesses
8. Support permission downgrades following permission lattice
9. Enforce grant requirements per Table 13.4
10. Support witness state transitions (@Stack→@Region→@Heap)
11. Require appropriate grants for transition methods

---

**Previous**: §13.5 Dispatch Semantics (§13.5 [witness.dispatch]) | **Next**: §13.7 Grammar Hooks and References (§13.7 [witness.grammar])
