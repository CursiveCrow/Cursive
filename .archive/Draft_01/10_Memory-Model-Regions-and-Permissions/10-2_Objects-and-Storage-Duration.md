# Cursive Language Specification

## Clause 10 — Memory Model, Regions, and Permissions

**Part**: X — Memory Model, Regions, and Permissions
**File**: 11-2_Objects-and-Storage-Duration.md  
**Section**: §10.2 Objects and Storage Duration  
**Stable label**: [memory.object]  
**Forward references**: §11.3 [memory.region], §11.4 [memory.permission], §11.5 [memory.move], Clause 5 §5.7 [decl.initialization]

---

### §10.2 Objects and Storage Duration [memory.object]

#### §10.2.1 Overview

[1] This section specifies object properties, storage duration categories, memory locations, and the RAII-based cleanup mechanism that ensures deterministic resource release.

[2] Objects are runtime entities with type, storage, and bounded lifetimes. Bindings establish relationships to objects: responsible bindings call destructors at scope exit; non-responsible bindings provide access without cleanup obligations.

#### §10.2.2 Object Model [memory.object.model]

##### §10.2.2.1 Object Definition

[3] An _object_ is a region of storage with the following properties:

1. **Type**: Exactly one type governing representation, size, and operations
2. **Storage location**: Contiguous bytes at a specific memory address
3. **Lifetime**: Bounded interval from creation to destruction
4. **Alignment**: Address alignment constraint matching type requirements
5. **Cleanup responsibility**: Association with zero or more responsible bindings

[4] Objects are created by:

- Variable declarations with `=` operator (§5.2)
- Region allocation via `^` operator (§11.3)
- Procedure parameters receiving moved values
- Temporary expressions during evaluation

[5] Objects are destroyed when:

- A responsible binding goes out of scope (RAII)
- A region exits (bulk deallocation)
- A `var` binding is reassigned (old value destroyed first)

##### §10.2.2.2 Memory Locations

[6] A _memory location_ is an addressable storage unit. Objects occupy one or more contiguous memory locations determined by their type:

- **Scalar types** (§7.2): One memory location
- **Composite types** (§7.3): Multiple locations (one per field/element)
- **Pointer types** (§7.5): One memory location (size of address)

[7] Two bindings _alias_ when they refer to overlapping memory locations. Aliasing is permitted for `const` and `shared` permissions; `unique` permission prevents aliasing through compile-time enforcement (§11.7).

##### §10.2.2.3 Binding Responsibility

[8] Binding cleanup responsibility is determined by the assignment operator per §4.2.5 [decl.variable.operator].

#### §10.2.3 Storage Duration Categories [memory.object.storage]

##### §10.2.3.1 Overview

[11] Storage duration determines when object storage is allocated and deallocated. Cursive provides four categories: static, automatic, region, and heap.

##### §10.2.3.2 Static Storage Duration

[12] **Static storage duration**: Objects allocated before `main` executes and deallocated after `main` returns. Module-scope bindings (§2.5) have static storage duration.

[13] **Allocation timing**: Static objects are allocated during program initialization, before the entry point begins.

[14] **Initialization order**: Static bindings initialize according to module dependency order (§4.6).

[15] **Lifetime**: Static objects live for the entire program execution.

**Example 11.2.3.1 (Static storage):**

```cursive
// Module-scope: static storage duration
let MODULE_CONFIG: const Config = load_initial_config()
var GLOBAL_COUNTER: const i32 = 0

public procedure main(): i32
    [[ io::write |- true => true ]]
{
    println("Config version: {}", MODULE_CONFIG.version)
    result 0
}
// Static objects destroyed after main returns
```

##### §10.2.3.3 Automatic Storage Duration

[16] **Automatic storage duration**: Objects allocated when control reaches their declaration and deallocated when their scope exits.

[17] **Allocation timing**: Automatic objects are allocated on the call stack (or equivalent) when execution reaches the binding statement.

[18] **Deallocation timing**: At scope exit (block end, procedure return), automatic objects are destroyed in LIFO order.

**Example 11.2.3.2 (Automatic storage with LIFO):**

```cursive
procedure demo()
    [[ alloc::heap |- true => true ]]
{
    let first = Resource::new("A")    // Allocated first
    let second = Resource::new("B")   // Allocated second
    let third = Resource::new("C")    // Allocated third

    // Destruction order: third, second, first (LIFO)
}
```

##### §10.2.3.4 Region Storage Duration

[19] **Region storage duration**: Objects allocated in an explicit region block using the `^` operator. All region allocations are freed simultaneously when the region exits.

[20] **Allocation timing**: Objects are allocated in the active region when the `^` operator executes.

[21] **Deallocation timing**: When the region block exits, all allocations within that region are freed in O(1) bulk deallocation.

[22] Complete region semantics are specified in §11.3 [memory.region].

##### §10.2.3.5 Heap Storage Duration

[23] **Heap storage duration**: Objects explicitly escaped to the heap via `.to_heap()` methods. These objects remain allocated until the responsible binding holding them is destroyed.

[24] Heap allocation is a secondary mechanism used when data must outlive its creating scope. The primary mechanism is region allocation (§11.3).

#### §10.2.4 RAII and Deterministic Cleanup [memory.object.raii]

##### §10.2.4.1 RAII Principle

[25] Cursive employs RAII: when a responsible binding goes out of scope, the binding's destructor (if any) executes automatically before the binding is removed.

[26] **RAII guarantee**: Resources are acquired during object initialization and released during object destruction. No explicit cleanup calls are needed; the compiler inserts destructor calls automatically.

##### §10.2.4.2 LIFO Destruction Order

[27] Within a scope, responsible bindings are destroyed in reverse declaration order (LIFO):

[ Given: Scope with responsible bindings $x_1, \ldots, x_n$ in declaration order ]

$$
\frac{\text{scope exits}}{\text{destroy } x_n, \text{ then } x_{n-1}, \ldots, \text{ then } x_1}
\tag{E-LIFO-Cleanup}
$$

[28] This order ensures that dependencies between objects are respected: objects created later (which may depend on earlier objects) are destroyed first.

##### §10.2.4.3 Destructor Execution

[29] When a responsible binding's scope exits:

1. The binding's destructor (if any) executes
2. Subobjects are destroyed in reverse field declaration order
3. Storage is released according to storage duration category

[30] Destructor execution is guaranteed for normal scope exit and early exit via `return`, `break`, or `continue`. Defer statements (§8.2.6) execute before destructors.

##### §10.2.4.4 Non-Responsible Bindings

[31] Non-responsible bindings created with `<-` do not invoke destructors. The original responsible binding retains cleanup responsibility. Multiple non-responsible bindings can reference the same object without creating cleanup obligations. See §4.2.5 Example 4.2.5.2 for binding operator semantics.

#### §10.2.5 Destructor Protocol [memory.object.destructor]

##### §10.2.5.1 Overview

[33] Types may implement custom cleanup logic through the `Drop` behavior (Clause 9). When a responsible binding goes out of scope, the runtime invokes the type's `drop` method if present, then automatically destroys all fields.

[34] The destructor protocol ensures deterministic resource release while allowing types to perform custom cleanup (closing file handles, releasing locks, logging shutdown, etc.).

##### §10.2.5.2 Drop Predicate [memory.object.destructor.predicate]

[35] The `Drop` behavior is defined in Clause 9 §9.4.5.6 [generic.behavior.marker.drop] with the following signature:

```cursive
behavior Drop {
    procedure drop(~!)
}
```

[36] Types implement `Drop` by providing a `drop` method with receiver `~!` (shorthand for `self: unique Self`). The method receives exclusive access to the object and may perform any cleanup operations required. The contractual sequent may include grants necessary for cleanup (e.g., `[[ fs::close |- ... ]]`).

**Example 11.2.5.1 (Drop implementation):**

```cursive
record FileHandle {
    path: string@Managed,
    handle: os::Handle,
}

behavior Drop for FileHandle {
    procedure drop(~!)
        [[ fs::close |- true => true ]]
    {
        os::close(self.handle)
        // Fields (path, handle) automatically destroyed after drop() completes
    }
}
```

##### §10.2.5.3 Automatic Field Destruction [memory.object.destructor.fields]

[37] After a type's `drop` method completes (if present), the compiler automatically invokes destructors for each field in reverse declaration order. Field destruction occurs regardless of whether the parent type implements `Drop`.

[ Given: Record `R { f₁: T₁, ..., fₙ: Tₙ }` with responsible binding ]

$$
\frac{R.\text{drop}() \text{ completes (if present)}}{\text{destroy } f_n, \text{ then } f_{n-1}, \ldots, \text{ then } f_1}
\tag{E-Field-Cleanup}
$$

[38] For types without custom `drop` methods, only automatic field destruction occurs. This ensures all subobjects are properly cleaned up in all cases.

##### §10.2.5.4 Destruction Order in Scopes [memory.object.destructor.order]

[39] Within a scope containing multiple responsible bindings, destruction follows strict LIFO (last-in, first-out) order as specified in §11.2.4.2. For each binding:

1. The binding's custom `drop` method executes (if `Drop` is implemented)
2. Fields are destroyed in reverse declaration order (recursively applying this protocol)
3. Storage is released according to storage duration category

[40] This order ensures that dependencies between objects are respected: objects created later (which may depend on earlier objects) are destroyed first.

##### §10.2.5.5 Panic and Unwinding [memory.object.destructor.panic]

[41] When a panic occurs during normal execution, destructors execute during stack unwinding (if unwinding is enabled). This section specifies the complete unwinding algorithm, destructor execution order, and interaction with region cleanup.

**Unwinding Algorithm:**

[41.1] When a panic occurs at location $L$ in scope $S$:

1. **Unwind initialization**: Mark the panic site and begin unwinding from innermost scope to outermost
2. **For each scope** $S_i$ being unwound (from innermost to outermost):
   a. Execute all `defer` statements in reverse order of declaration (last-declared executes first)
   b. Execute destructors for all responsible bindings in reverse declaration order (LIFO)
   c. If the scope is a region block, invoke `arena.free()` for bulk deallocation
   d. Continue to parent scope
3. **Termination**: Unwinding stops when:
   - A scope catches the panic (future feature), OR
   - The entry point `main` is reached (process exits with non-zero status)

[41.2] **Destructor panic during unwinding.** If any destructor invoked during unwinding panics:

- The process **aborts immediately** without executing remaining destructors
- No further unwinding occurs
- The process exits with implementation-defined non-zero status
- This prevents double-panic scenarios and undefined stack states

[41.3] **Defer execution order.** `defer` statements execute **before** destructors in the same scope:

```
Scope exit order:
1. defer blocks (reverse declaration order)
2. responsible binding destructors (reverse declaration order)
```

This ensures cleanup code in `defer` can access still-valid objects.

[42] **Region cleanup during unwinding.** Region blocks being unwound are freed via `arena.free()` after all responsible bindings in that scope are destroyed. This ensures:

- Responsible bindings can access region-allocated data during cleanup
- Arena memory is reclaimed after all cleanup completes
- O(1) bulk deallocation occurs even during panic scenarios

[43] **Abort-on-panic mode.** Implementations may provide an abort-on-panic mode (compilation flag `--panic=abort`) where panics immediately terminate the process without unwinding. In this mode:

- Destructors are **not guaranteed to execute**
- `defer` blocks are **not executed**
- Process exits immediately with non-zero status
- Programs requiring guaranteed cleanup should design for graceful shutdown rather than relying on panic unwinding

[43.1] **Conformance note**: Implementations shall document whether default panic behavior is unwinding or abort. Both modes are conforming; the choice affects reliability guarantees for resource cleanup.

##### §10.2.5.6 Types Without Custom Destructors [memory.object.destructor.auto]

[44] Types that do not implement `Drop` still participate in RAII through automatic field destruction. Primitive types (§7.2) have trivial destructors (no-op). Composite types recursively destroy their components.

[45] Most types do not need custom destructors; automatic field destruction handles cleanup correctly. Custom `drop` methods are needed only when:

- Releasing external resources (files, sockets, locks)
- Logging or metrics on object lifetime
- Performing cleanup operations beyond simple deallocation

##### §10.2.5.7 Diagnostics [memory.object.destructor.diagnostics]

[Note: Diagnostics defined in this subsection are cataloged in Annex E §E.5.1.10. — end note]

##### §10.2.5.8 Examples [memory.object.destructor.examples]

**Example 11.2.5.2 (Automatic field destruction):**

```cursive
record Resource {
    name: string@Managed,
    data: Buffer,
}

// No custom Drop implementation needed
// Fields destroyed automatically in reverse order: data, then name
```

**Example 11.2.5.3 (Nested destruction):**

```cursive
procedure demo()
    [[ alloc::heap |- true => true ]]
{
    let outer = Resource {
        name: string.from("outer"),
        data: Buffer::new(),
    }
    // At scope exit:
    // 1. outer.drop() if implemented
    // 2. outer.data destroyed (Buffer::drop() called if implemented)
    // 3. outer.name destroyed (string@Managed::drop() called)
}
```

#### §10.2.6 Conformance Requirements

[52] Implementations shall handle panic during unwinding per §10.2.5.5, aborting on double-panic scenarios.

---

**Previous**: §11.1 Memory Model Overview (§11.1 [memory.overview]) | **Next**: §11.3 Regions (§11.3 [memory.region])
