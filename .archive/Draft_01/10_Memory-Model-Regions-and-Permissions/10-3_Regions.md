# Cursive Language Specification

## Clause 10 — Memory Model, Regions, and Permissions

**Part**: X — Memory Model, Regions, and Permissions
**File**: 11-3_Regions.md  
**Section**: §10.3 Regions  
**Stable label**: [memory.region]  
**Forward references**: §11.2 [memory.object], §11.5 [memory.move], §11.7 [memory.aliasing], §6.2 [name.scope], Clause 7 §7.5 [type.pointer]

---

### §10.3 Regions [memory.region]

#### §10.3.1 Overview

[1] Regions provide lexically-scoped, stack-like allocation arenas with O(1) bulk deallocation. They enable efficient temporary allocation without garbage collection or manual memory management.

[2] Cursive implements regions through the `Arena` modal type - a built-in modal type with states representing the arena lifecycle (@Active, @Frozen, @Freed). The `region` keyword is syntactic sugar for creating a scoped `Arena@Active` with automatic cleanup.

[3] This section specifies the Arena modal type, region block syntax, allocation semantics, escape analysis, and arena state transitions.

#### §10.3.2 Arena Modal Type [memory.region.arena]

##### §10.3.2.1 Arena States and Structure

[4] The `Arena` type is a built-in modal type with three states representing the arena lifecycle:

```cursive
modal Arena {
    @Active {
        ptr: Ptr<u8>,
        capacity: usize,
        allocated: usize,
    }

    @Active::alloc<T>(~!, value: T) -> @Active
    @Active::alloc_array<T>(~!, count: usize) -> @Active
    @Active::reset(~!) -> @Active
    @Active::freeze(~) -> @Frozen
    @Active::free(~!) -> @Freed

    @Frozen {
        ptr: Ptr<u8>,
        allocated: usize,
    }

    @Frozen::thaw(~!) -> @Active
    @Frozen::free(~!) -> @Freed

    @Freed { }
}
```

[5] **State semantics**:

- **@Active**: Arena is active and accepting allocations. Supports allocation, reset (bulk free + reuse), freeze (prevent further allocation), and terminal free.
- **@Frozen**: Arena is read-only; allocated data remains valid but no new allocations permitted. Supports thaw (resume allocation) and terminal free.
- **@Freed**: Terminal state after arena is deallocated. No operations available.

[6] **State transition diagram**:

```
@Active ──reset()──> @Active    (bulk free, reuse memory)
   │
   │ freeze()
   ↓
@Frozen ──thaw()───> @Active    (resume allocation)
   │              │
   │ free()   free()
   ↓              ↓
@Freed <────────────            (terminal state)
```

##### §10.3.2.2 Arena Allocation Methods

[7] The `alloc` method allocates a value in the arena and returns the arena in @Active state:

```cursive
procedure Arena.alloc<T>(~!, value: T): Self@Active
    [[ alloc::region |- true => true ]]
{
    // Implementation: bump-allocate space for T, write value
    result self  // Returns same arena instance, still @Active
}
```

[8] Type rule for allocation:

$$\frac{\Gamma \vdash arena : \text{Arena}@\text{Active} \quad \Gamma \vdash v : \tau}{\Gamma \vdash arena\texttt{.alloc}(v) : \text{Arena}@\text{Active}}$$

[9] The allocated value is stored in the arena's memory; the arena itself is returned to enable method chaining and continued allocation.

##### §10.3.2.3 Arena Reset and Lifecycle

[10] The `reset` method bulk-frees all allocations and returns the arena to empty @Active state:

```cursive
procedure Arena.reset(~!): Self@Active
{
    // Reset allocation pointer to beginning
    // All previously allocated values become invalid
    result self
}
```

[11] After `reset()`, all pointers previously allocated in the arena become invalid. Accessing such pointers produces undefined behavior [UB-ID: B.2.01].

[12] The `free` method transitions the arena to @Freed state, deallocating the arena's backing memory:

```cursive
procedure Arena.free(~!): Self@Freed
{
    // Deallocate arena backing memory
    result Arena@Freed { }
}
```

#### §10.3.3 Region Block Syntax [memory.region.syntax]

##### §10.3.3.1 Region Blocks

[13] Region block syntax:

**Region blocks** match the pattern:
```
"region" <identifier> [ "as" <identifier> ] <block_stmt>
```

[ Note: See Annex A §A.5 [grammar.statement] for complete region grammar.
— end note ]

[14] The first identifier names the region for parent-region allocation (caret stacking, §11.3.5). The optional `as identifier` clause binds the arena to the specified name, creating a non-responsible binding to the `Arena@Active` object.

[15] **Desugaring**: Region blocks desugar to arena creation with automatic cleanup:

```cursive
// Source syntax:
region temp as workspace {
    let x = ^Value::new()
}

// Desugars to:
{
    let workspace = Arena::new_scoped()  // Arena@Active, non-responsible binding
    defer { workspace.free() }
    let x = workspace.alloc(Value::new())
}
```

[16] When the `as identifier` clause is omitted, an implicit arena binding is created but not accessible by name. The `^` operator allocates through this implicit arena.

**Example 11.3.2.1 (Region blocks):**

```cursive
region temp {
    let data = ^Buffer::new(1024)
    process(data)
}
// All allocations in temp freed here (O(1))

region outer {
    region inner {
        let a = ^Data::new()   // Allocated in inner
    }
    // inner region freed
    let b = ^Data::new()       // Allocated in outer
}
// outer region freed
```

##### §10.3.3.2 The Caret Operator (Syntactic Sugar)

[17] The `^` prefix operator is syntactic sugar for arena allocation:

**Region allocation expressions** take one of the following forms:
```
"^" <expression>
"^" "^" ... <expression>    // Multiple carets for parent arenas
```

[18] **Desugaring rule**: Within a region block, `^expr` desugars to `arena.alloc(expr)` where `arena` is the active arena binding (named or implicit):

```cursive
region temp as workspace {
    let x = ^Value::new()
    let y = workspace.alloc(Value::new())
    // Equivalent: both desugar to workspace.alloc()
}

region temp {
    let x = ^Value::new()
    // Desugars to: __implicit_arena.alloc(Value::new())
    // where __implicit_arena is the compiler-generated arena binding
}
```

[19] **Type preservation**: The caret operator (via `arena.alloc()`) evaluates the expression and allocates the resulting value in the arena. The type remains `T`:

$$
\frac{\Gamma \vdash e : \tau \quad arena : \text{Arena}@\text{Active}}{\Gamma \vdash \texttt{^} e : \tau}
\tag{T-Region-Alloc}
$$

The return type is the allocated value's type `τ`, not a pointer type. The `alloc` method stores the value in arena memory and returns the arena itself (type `Arena@Active`), but the `^` operator discards the returned arena and yields the allocated value.

[20] **Provenance assignment**: Values allocated via `^` or `arena.alloc()` receive provenance `Region(r)` where `r` is the region identifier. This provenance is compiler metadata (not part of the type system) used for escape analysis (§11.3.4).

[ Note: The `^` operator is purely syntactic convenience. It desugars to `arena.alloc(expression)` during parsing. The allocation semantics, escape analysis, and bulk deallocation are all properties of the underlying `Arena@Active` modal value, not special behaviors of the caret operator.
— end note ]

**Example 11.3.3.2 (Caret operator and explicit allocation equivalence):**

```cursive
region batch as workspace {
    // All four allocations are equivalent:
    let buf1 = ^Buffer::new()
    let buf2 = workspace.alloc(Buffer::new())

    // Both have:
    // - Type: Buffer
    // - Provenance: Region(batch)
    // - Allocated in: batch arena

    // To get pointers:
    let ptr1: Ptr<Buffer>@Valid = &buf1
    let ptr2: Ptr<Buffer>@Valid = &buf2
}
```

#### §10.3.4 Escape Analysis [memory.region.escape]

##### §10.3.4.1 Overview

[21] Escape analysis prevents region-allocated values from outliving their region. The compiler statically verifies that region-allocated data does not escape the region block through provenance tracking and escape checking at region boundaries.

[22] Escape analysis operates on compile-time **provenance metadata** associated with each value. Provenance tracks the allocation source (stack, region, or heap) and is propagated through operations. Complete algorithm specifications appear in Annex E §E.2.6 [implementation.algorithms.region].

##### §10.3.4.2 Provenance Metadata [memory.region.provenance]

[23] Every value has associated provenance metadata indicating its allocation source:

**Provenance Type:**

$$
\text{Provenance} ::= \text{Stack} \mid \text{Region}(r) \mid \text{Heap}
$$

where $r$ is a region identifier bound to a lexical region scope.

[24] **Provenance assignment rules**:

- Values allocated via `^` or `arena.alloc()` in region $r$ have provenance $\text{Region}(r)$
- Local variables (without `^` or `arena.alloc()`) have provenance $\text{Stack}$
- Values converted via `.to_heap()` have provenance $\text{Heap}$
- Module-scope values have provenance $\text{Static}$ (treated as Heap for escape purposes)

[25] **Provenance is metadata**: Provenance information is tracked by the compiler but is not part of the type system. A value of type `T` has the same type regardless of whether it has provenance Stack, Region(r), or Heap. Provenance affects only escape analysis, not type checking.

[ Note: The `Arena` modal type itself does not carry provenance - it is a heap-allocated object managing an arena. Values allocated THROUGH the arena (via `alloc()` or `^`) receive Region(r) provenance based on which region block created the arena.
— end note ]

##### §10.3.3.3 Provenance Propagation [memory.region.propagation]

[15] Provenance propagates through operations according to these rules:

**Address-of propagation:**

[ Given: Value $v$ with provenance $\pi$ ]

$$
\frac{\text{prov}(v) = \pi}{\text{prov}(\&v) = \pi}
\tag{Prov-Addr}
$$

Taking the address of a value produces a pointer with the same provenance as the value.

**Field access propagation:**

[ Given: Structure $s$ with provenance $\pi$, field $f$ ]

$$
\frac{\text{prov}(s) = \pi}{\text{prov}(s.f) = \pi}
\tag{Prov-Field}
$$

Accessing a field inherits the structure's provenance.

**Procedure return propagation:**

[ Given: Procedure call $p(\text{args})$ with conservative provenance ]

$$
\frac{}{\text{prov}(p(\text{args})) = \text{Heap}}
\tag{Prov-Call-Conservative}
$$

Procedure calls conservatively assume Heap provenance unless the procedure's signature specifies otherwise (future extension). Implementations may refine this with interprocedural analysis.

[16] Complete provenance propagation rules for all expression forms are specified in Annex E §E.2.6 [implementation.algorithms.region].

##### §10.3.3.4 Escape Checking [memory.region.escape.checking]

[17] **Escape prohibition**: Values with region provenance shall not escape their region. Implementations shall check escape at:

1. **Return statements**: Returning a Region(r)-provenance value when r is local
2. **Region block exit**: Storing region-allocated values in outer scopes
3. **Closure capture**: Capturing region values in closures that outlive the region
4. **Field assignment**: Assigning region values to fields of longer-lived structures

[18] **Formal escape constraint**:

[ Given: Expression $e$ with provenance $\pi$, target scope $s$ ]

$$
\frac{\text{prov}(e) = \text{Region}(r) \quad r \text{ is lexically nested in } s}
     {\text{escape}_s(e) \Rightarrow \text{ERROR E11-101}}
\tag{WF-No-Region-Escape}
$$

The rule states: if a value has region provenance and the region is local to the current analysis scope, escaping that value to the target scope is forbidden.

[19] **Diagnostic E11-101**: "Cannot escape region-allocated value from region `{region_id}`"

The diagnostic shall include:

- Region identifier
- Allocation site (where `^` occurred)
- Escape site (where escape was attempted)
- Provenance path (how provenance propagated to escape site)

##### §10.3.3.5 Permitted Escape via Heap Conversion [memory.region.escape.heap]

[20] Region-allocated values may escape only after explicit conversion to heap allocation:

**Heap conversion rule:**

[ Given: Value $v$ with provenance $\text{Region}(r)$ ]

$$
\frac{\text{prov}(v) = \text{Region}(r)}{\text{prov}(v\texttt{.to\_heap()}) = \text{Heap}}
\tag{Prov-Heap-Convert}
$$

[21] The `.to_heap()` operation (standard library) copies the value to heap storage and returns a heap-allocated value with provenance Heap. The original region-allocated value remains valid until the region exits.

[22] Heap conversion requires the `alloc::heap` grant in the procedure's contractual sequent. Attempting to call `.to_heap()` without the grant produces diagnostic E11-102 (heap allocation requires alloc::heap grant).

##### §10.3.3.6 Examples [memory.region.escape.examples]

**Example 11.3.3.1 - invalid (Region escape):**

```cursive
procedure create_data(): Buffer
    [[ alloc::region |- true => true ]]
{
    region temp {
        let data = ^Buffer::new()
        result data  // error[E11-101]: region-allocated value cannot escape
    }
}
```

**Example 11.3.3.2 (Heap conversion allows escape):**

```cursive
procedure create_data(): Buffer
    [[ alloc::region, alloc::heap |- true => true ]]
{
    region temp {
        let data = ^Buffer::new()
        data.prepare()
        result data.to_heap()  // ✅ OK: explicit heap conversion
    }
}
```

**Example 11.3.3.3 (Pointer escape prevention):**

```cursive
procedure get_pointer(): Ptr<i32>@Valid
    [[ alloc::region |- true => true ]]
{
    region r {
        let value = ^42
        let ptr = &value           // ptr has provenance Region(r)
        result ptr                 // error[E11-101]: pointer provenance is Region(r)
    }
}
```

**Example 11.3.3.4 (Stack values can escape):**

```cursive
procedure create_value(): i32
{
    region r {
        let stack_value = 42       // Provenance: Stack (no ^)
        result stack_value         // ✅ OK: stack values can escape
    }
}
```

##### §10.3.3.7 Algorithm Reference [memory.region.escape.algorithm]

[23] The complete escape analysis algorithm, including:

- Provenance propagation for all expression forms
- Interprocedural provenance tracking
- Escape checking at all control-flow points
- Conservative analysis for complex cases

is specified in Annex E §E.2.6 [implementation.algorithms.region]. Implementations shall follow the algorithm or a refinement that rejects strictly more programs (i.e., more conservative).

#### §10.3.5 Caret Stacking for Parent Arenas [memory.region.stacking]

##### §10.3.5.1 Parent Arena Allocation

[26] Multiple caret operators allocate in parent arenas:

- `^expr` — Allocate in innermost region
- `^^expr` — Allocate in parent region (one level up)
- `^^^expr` — Allocate in grandparent region (two levels up)

[16] This enables allocating data with longer lifetimes without escaping to the heap.

**Example 11.3.4.1 (Caret stacking):**

```cursive
region outer {
    let outer_data = ^Data::new()  // Allocated in outer

    region inner {
        let inner_data = ^Data::new()   // Allocated in inner
        let parent_data = ^^Data::new()  // Allocated in outer (parent)
    }
    // inner region freed (inner_data destroyed)
    // parent_data still alive (allocated in outer)

    use(parent_data)
}
// outer region freed (outer_data and parent_data destroyed)
```

[17] Caret stacking requires named regions in future versions for explicit parent references. The current version uses implicit parent resolution based on lexical nesting.

[ Note: Region parameters and explicit parent naming are deferred to a future version of this specification.
— end note ]

#### §10.3.6 Bulk Deallocation [memory.region.bulk]

##### §10.3.6.1 O(1) Deallocation

[27] When a region exits, all allocations within that region are freed simultaneously in O(1) constant time. This is achieved through the `Arena@Active` implementation which maintains a single allocation pointer.

[28] **Performance characteristic**:

- **Allocation**: O(1) pointer bump via `arena.alloc()`
- **Individual deallocation**: Not supported (bulk only)
- **Bulk deallocation**: O(1) via `arena.reset()` or automatic at region exit

[29] This makes arenas ideal for temporary allocations in tight loops or recursive procedures.

[30] **Automatic vs explicit reset**: Region blocks automatically call `arena.free()` at scope exit (via desugared `defer` block, §11.3.3.1[15]). Named arena bindings also support explicit `reset()` for reusing arena memory mid-scope:

```cursive
region batch as workspace {
    // Phase 1: Allocate and process
    loop item in phase1_items {
        let data = workspace.alloc(process(item))
        output(data)
    }

    workspace.reset()  // Bulk free, keep arena

    // Phase 2: Reuse same memory
    loop item in phase2_items {
        let data = workspace.alloc(process(item))
        output(data)
    }
}
// workspace.free() called automatically
```

**Example 11.3.5.1 (Efficient batch processing):**

```cursive
procedure process_items(items: [Item])
    [[ alloc::region, io::write |- items.len() > 0 => true ]]
{
    loop item: Item in items {
        region iteration {
            let buffer = ^Buffer::new(4096)
            let processed = ^transform(item)
            output(buffer, processed)
        }
        // O(1) bulk free - no accumulation across iterations
    }
}
```

#### §10.3.7 Arena Passing and Pools [memory.region.pools]

##### §10.3.7.1 Passing Arenas to Procedures

[31] Named arena bindings can be passed to procedures as parameters of type `Arena@Active`:

```cursive
procedure fill_arena(workspace: Arena@Active, items: [Item])
    [[ alloc::region |- items.len() > 0 => true ]]
{
    loop item in items {
        let allocated = workspace.alloc(transform(item))
        store(allocated)
    }
}

region batch as arena {
    fill_arena(arena, data)
    // All allocations performed in batch arena
}
```

[32] **Parameter semantics**: Arena parameters are non-responsible bindings (§11.2.2.3). The procedure receives access to the arena but does not control its lifetime. The arena is destroyed when the originating `region` block exits, not when the parameter goes out of scope.

[33] **Escape prevention**: Arena bindings cannot escape their defining region block. Attempting to return or store an arena parameter produces diagnostic E11-110 (arena binding escapes region).

##### §10.3.7.2 Multiple Independent Arenas

[34] Multiple arenas with independent lifetimes use nested region blocks:

```cursive
region long_lived as persist {
    region short_lived as temp {
        persist.alloc(permanent_data())
        temp.alloc(temporary_data())
    }
    // temp freed, persist still active
}
// persist freed
```

[35] Nested arenas have LIFO lifetimes: inner arenas are freed before outer arenas.

##### §10.3.7.3 Arena Collections

[36] For multiple arenas with identical lifetimes (freed together), arenas can be collected in arrays or records. Since arena bindings from `region` blocks are non-responsible and cannot escape, arena collections require explicit arena creation (standard library feature):

```cursive
// Conceptual arena pool (standard library, not core language):
record ArenaPool {
    arenas: [Arena@Active],

    procedure with_arenas<T>(count: usize, action: ([Arena@Active]) -> T): T
        [[ alloc::region |- count > 0 => true ]]
}
```

[37] Arena collections and advanced arena management patterns are library features built on the core `Arena` modal type and `region` syntax defined in this clause. Complete specifications appear in Clause 17 (Standard Library).

#### §10.3.8 Diagnostics

[38] Region and arena diagnostics:

| Code    | Condition                                     |
| ------- | --------------------------------------------- |
| E11-101 | Region-allocated value escapes region         |
| E11-102 | Invalid caret stacking (too many ^ for depth) |
| E11-103 | Region allocation outside region block        |
| E11-110 | Arena binding escapes region                  |

#### §10.3.9 Conformance Requirements

[39] Implementations shall:

1. Implement `Arena` as a built-in modal type with @Active, @Frozen, and @Freed states per §11.3.2
2. Provide arena allocation methods: `alloc<T>`, `alloc_array<T>`, `reset`, `freeze`, `thaw`, `free`
3. Support region blocks with optional arena binding syntax (`region id as arena`)
4. Desugar region blocks to arena creation with automatic cleanup via `defer`
5. Desugar `^` operator to `arena.alloc()` calls on the active (named or implicit) arena
6. Support caret stacking (`^^`, `^^^`) for parent arena allocation
7. Perform static escape analysis rejecting region-allocated value escapes with E11-101
8. Prevent arena bindings from escaping their region blocks with E11-110
9. Provide O(1) bulk deallocation when arenas are freed or reset
10. Track region provenance as compiler metadata for escape checking (§7.5.3)
11. Integrate arena lifecycle with binding responsibility and RAII cleanup
12. Support passing arenas as `Arena@Active` parameters to procedures (non-responsible)

---

**Previous**: §11.2 Objects and Storage Duration (§11.2 [memory.object]) | **Next**: §11.4 Permissions (§11.4 [memory.permission])
