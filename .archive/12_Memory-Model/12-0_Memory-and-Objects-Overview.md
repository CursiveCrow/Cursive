# 2.8 Memory and Objects Overview

## Overview

This section provides a conceptual overview of the memory and object model in Cursive. It introduces fundamental concepts including objects, memory locations, object lifetime, storage duration, and alignment. These concepts provide the foundation for understanding how values are represented in memory and how resources are managed.

**Important**: This section is an overview only. Complete formal specifications, including permission systems, move semantics, regions, and memory safety guarantees, are provided in Part VI (Memory and Resource Management).

---

## 2.8.1 Object Model (Conceptual)

### 2.8.1.1 Object Concept

**Definition 2.46 (Object - Conceptual)**: An object is a value with associated storage, a type, a lifetime, and a location in memory.

**Conceptual properties**:

1. **Type**: Determines the representation, operations, and interpretation of the bit pattern
2. **Size**: The amount of storage occupied by the object (measured in bytes)
3. **Alignment**: The address alignment requirement (e.g., 4-byte alignment for `i32`)
4. **Lifetime**: The duration from object creation to object destruction
5. **Location**: The memory address or region where the object resides

**Example (informative)**:

```cursive
let x: i32 = 42
// Object properties:
// - Type: i32 (32-bit signed integer)
// - Size: 4 bytes (typically)
// - Alignment: 4 bytes (typically)
// - Lifetime: From declaration to end of scope
// - Location: Stack address (automatic storage)
```

### 2.8.1.2 Object Creation and Destruction

**Creation**: Objects are created by:
- Variable declarations (`let x = value`)
- Temporary expressions (intermediate values)
- Explicit region allocation (`^value` within region scope)

**Destruction**: Objects are destroyed by:
- Scope exit (automatic objects follow RAII)
- Explicit deallocation (for manually managed objects)
- Region end (bulk deallocation for region-allocated objects)

**Forward reference**: See §[REF_TBD] (Part VI: Object Lifetime) for formal object creation and destruction semantics, including initialization order, destruction order, and temporary object lifetime rules.

---

## 2.8.2 Memory Locations (Conceptual)

### 2.8.2.1 Memory Location Concept

**Definition 2.47 (Memory Location - Conceptual)**: A memory location is a distinct addressable region of storage. Objects occupy one or more contiguous memory locations.

**Conceptual properties**:

1. **Identity**: Each memory location is distinct from other locations
2. **Byte sequence**: A memory location consists of one or more bytes
3. **Alignment**: Memory locations have alignment requirements
4. **Accessibility**: Determined by the permission system (§[REF_TBD])

**Scalar types**: Objects of scalar types (e.g., `i32`, `f64`, `bool`) occupy exactly one memory location.

**Aggregate types**: Objects of aggregate types (e.g., records, tuples, arrays) may occupy multiple memory locations (one per field or element).

### 2.8.2.2 Aliasing

**Aliasing concept**: Two references alias if they refer to overlapping memory locations.

**Example (informative)**:

```cursive
// Demonstrate aliasing
region frame {
    let data = ^[1, 2, 3]

    // Multiple reference bindings to the same element
    let a1 <- data[0]      // Reference binding to first element
    let a2 <- data[0]      // Another reference binding to same element
    // a1 and a2 alias (refer to the same memory location)

    println("a1: {}, a2: {}", a1, a2)
}
```

**Note**: Cursive does NOT use Rust-style borrowing (`&T`, `&mut T`). Aliasing is demonstrated through reference bindings (using the `<-` operator) or through the permission system (see Part VI).

**Forward reference**: See §[REF_TBD] (Part VI: Aliasing and Uniqueness) for formal aliasing rules, including aliasing restrictions and the uniqueness guarantees provided by the permission system.

---

## 2.8.3 Object Lifetime (Overview)

### 2.8.3.1 RAII (Resource Acquisition Is Initialization)

**Normative Statement 2.8.1**: Cursive employs RAII (Resource Acquisition Is Initialization) for deterministic resource management. Objects are automatically cleaned up at the end of their scope.

**RAII principle**: When an object goes out of scope, its destructor (if any) runs automatically, releasing any resources held by the object (memory, file handles, locks, etc.).

**Example (informative)**:

```cursive
{
    let file = File::open("data.txt")  // Resource acquired
    // Use file...
}
// file's destructor runs automatically here, closing the file
```

### 2.8.3.2 Destruction Order (LIFO)

**Requirement 2.8.1**: Objects are destroyed in reverse order of declaration (Last In, First Out - LIFO).

**LIFO principle**: The most recently declared object is destroyed first, ensuring that dependencies between objects are respected.

**Example (informative)**:

```cursive
{
    let a = Resource::new("A")  // Declared first
    let b = Resource::new("B")  // Declared second
    let c = Resource::new("C")  // Declared third
}
// Destruction order: c, then b, then a (reverse of declaration)
```

### 2.8.3.3 Determinism

**Normative Statement 2.8.2**: Cleanup points are statically determined. Destructors run at well-defined points in the program determined by lexical scope.

**Contrast with garbage collection**: Unlike garbage-collected languages where cleanup timing is non-deterministic, Cursive's RAII provides deterministic cleanup at scope exit.

**Forward reference**: See §[REF_TBD] (Part VI: Object Lifetime) for detailed lifetime rules, including:
- Temporary object lifetime and lifetime extension
- Early destruction via explicit `drop`
- Move semantics and moved-from objects
- Partial moves and field-wise destruction

---

## 2.8.4 Storage Duration (Overview)

### 2.8.4.1 Storage Duration Concept

**Definition 2.48 (Storage Duration)**: Storage duration determines when storage for an object is allocated and when it is deallocated.

**Storage duration categories**: Cursive defines four storage duration categories:

1. **Static storage**: Allocated before `main`, deallocated after `main`
2. **Automatic storage**: Allocated on scope entry, deallocated on scope exit
3. **Region storage**: Allocated in a region, deallocated when region ends (bulk deallocation)
4. **Heap storage**: Explicitly escaped to heap via `.to_heap()`, deallocated via RAII

**Primary vs secondary allocation**: Regions are the **primary** mechanism for dynamic allocation in safe code. Heap allocation is **secondary**, used when data must outlive its creating scope (escape pattern, see §2.8.4.6).

### 2.8.4.2 Storage Duration Table

The following table summarizes storage duration by declaration context:

| Context | Declaration | Duration | Allocation | Deallocation |
|---------|-------------|----------|------------|--------------|
| **Module scope** | `let x = ...` | Static (program lifetime) | Before `main` | After `main` returns |
| **Module scope** | `var x = ...` | Static (program lifetime) | Before `main` | After `main` returns |
| **Procedure scope** | `let x = ...` | Automatic (procedure lifetime) | On procedure entry | On procedure exit |
| **Block scope** | `let x = ...` | Automatic (block lifetime) | At declaration | At end of block |
| **Region block** | `^...` | Region lifetime | At `^` operator | At region end (O(1) bulk) |
| **Escape pattern** | `value.to_heap()` | Heap (until owner drops) | At `.to_heap()` call | When owning binding drops (RAII) |

### 2.8.4.3 Static Storage

**Static storage duration**: Objects with static storage duration are allocated before `main` begins execution and deallocated after `main` returns.

**Example (informative)**:

```cursive
// Module-level declarations have static storage
let MODULE_CONSTANT: i32 = 42         // Static storage, immutable binding
var GLOBAL_COUNTER: i32 = 0           // Static storage, mutable binding
```

**Initialization order**: Module-level variables are initialized in dependency order (§2.4.2.4).

### 2.8.4.4 Automatic Storage

**Automatic storage duration**: Objects with automatic storage duration are allocated when control reaches their declaration and deallocated when control exits their scope.

**Stack allocation**: Automatic objects are typically allocated on the call stack.

**Example (informative)**:

```cursive
procedure example()
    {| |- true => true |}
{
    let proc_var = 42  // Automatic storage, lives until end of procedure

    {
        let block_var = 10  // Automatic storage, lives until end of block
        println("{}", block_var)
    }
    // block_var destroyed here

    println("{}", proc_var)
}
// proc_var destroyed here
```

### 2.8.4.5 Region Storage

**Region storage duration**: Objects allocated within a region live until the region ends, at which point all region allocations are freed simultaneously in constant time.

**Bulk deallocation**: Regions enable O(1) bulk deallocation, improving performance for allocation-intensive code.

**Example (informative)**:

```cursive
region r {
    let value1 = ^Data::new(1)
    let value2 = ^Data::new(2)
    let value3 = ^Data::new(3)
    // All allocated in region r
}
// All region allocations freed at once (O(1) bulk free)
```

**Forward reference**: See §[REF_TBD] (Part VI: Regions and Lifetimes) for complete region specification, including escape analysis, region parameters, and region borrowing.

### 2.8.4.6 Escape-to-Heap Pattern

**Escape pattern**: When data must outlive its creating scope, Cursive provides an explicit escape-to-heap pattern via `.to_heap()` methods (or similar explicit conversion).

**Primary model**: Regions are the primary allocation mechanism. Heap allocation is used when:
1. Data must outlive the procedure that creates it
2. Data lifetime cannot be statically determined by regions

**Example (informative)**:

```cursive
// Build data in temporary region, then escape to heap
procedure build_collection(): Collection<Item>
    sequent { [alloc::region, alloc::heap] |- true => true }
{
    region temp {
        let items = ^Collection::new()
        // Build collection in temporary region...
        loop i in 0..100 {
            items::push(^Item::new(i))
        }
        // Explicit escape to heap before returning
        result items.to_heap()
    }
}
```

**Explicit vs implicit**: The escape-to-heap pattern is explicit (`to_heap()` call) rather than implicit, making allocation intent clear.

**Forward reference**: See §[REF_TBD] (Part VI: Storage Duration and Escape Analysis) for complete specification of region escape rules, heap allocation patterns, and the `.to_heap()` predicate.

---

## 2.8.5 Alignment (Brief Overview)

### 2.8.5.1 Alignment Concept

**Definition 2.49 (Alignment - Conceptual)**: Alignment is the address requirement for objects. An object with alignment N must be stored at an address that is a multiple of N.

**Natural alignment**: Most types have natural alignment determined by their size and composition. For example:
- `i8` typically has 1-byte alignment
- `i32` typically has 4-byte alignment
- `i64` typically has 8-byte alignment

**Implementation-defined**: Specific alignment values are implementation-defined and platform-dependent.

### 2.8.5.2 Alignment Requirements

**Requirement 2.8.2**: The implementation shall respect alignment requirements when allocating objects. Violating alignment requirements may result in:
- Performance degradation (unaligned access)
- Hardware faults (on architectures that do not support unaligned access)
- Undefined behavior

**Forward reference**: See §[REF_TBD] (Part VI: Memory Layout) for formal memory layout and alignment rules, including:
- Alignment calculation for aggregate types
- Alignment attributes
- Padding and structure layout
- Over-aligned types

---

## 2.8.6 Canonical Example: Storage Durations and RAII

The following comprehensive example demonstrates all storage duration categories and RAII cleanup:

```cursive
// ========== Static Storage (Module Level) ==========

let MODULE_CONSTANT: i32 = 100  // Static storage, immutable binding

var CALL_COUNT: i32 = 0  // Static storage, mutable binding

// ========== Resource Type with Destructor ==========

record Resource {
    name: string
}

procedure Resource.drop(self)
    sequent { [fs::write] |- true => true }
{
    println("Destroying resource: {}", self.name)
}

procedure Resource.new(name: string): Resource
    sequent { [fs::write] |- true => true }
{
    println("Creating resource: {}", name)
    CALL_COUNT += 1  // Increment mutable module-level variable
    result Resource { name }
}

// ========== Procedure Demonstrating All Storage Durations ==========

procedure demonstrate_storage()
    {| |- true => true |}
{
    // Automatic storage (procedure scope)
    let proc_var = Resource::new("proc_var")

    {
        // Automatic storage (block scope)
        let block_var = Resource::new("block_var")
        println("Inside block")
    }
    // block_var destroyed here (RAII)
    println("After block")

    // Region storage (bulk deallocation)
    region r {
        let r1 = ^Resource::new("region1")
        let r2 = ^Resource::new("region2")
        let r3 = ^Resource::new("region3")
        println("Inside region")
    }
    // All region allocations freed at once (O(1))
    println("After region")

    println("Procedure ending")
}
// Destruction order (LIFO): proc_var destroyed

// ========== Entry Point ==========

procedure main(): i32
    {| |- true => true |}
{
    println("Starting program")
    println("CALL_COUNT: {}", CALL_COUNT)

    demonstrate_storage()

    println("After demonstrate_storage()")
    println("CALL_COUNT: {}", CALL_COUNT)

    result 0
}
// Static destructors run after main returns (if any)

/* ========== Expected Execution Trace ==========

Starting program
CALL_COUNT: 0
Creating resource: func_var
Creating resource: block_var
Inside block
Destroying resource: block_var        // block_var destroyed (block exit)
After block
Creating resource: region1
Creating resource: region2
Creating resource: region3
Inside region
Destroying resource: region3
Destroying resource: region2
Destroying resource: region1          // All region objects destroyed (region exit)
After region
Function ending
Destroying resource: func_var         // func_var destroyed (LIFO)
After demonstrate_storage()
CALL_COUNT: 5
(Program exits, static destructors run if any)
*/
```

**Key observations**:
1. **Static storage**: `MODULE_CONSTANT` and `CALL_COUNT` live for the entire program
2. **Automatic storage**: `func_var` and `block_var` destroyed at scope exit (RAII)
3. **Region storage**: `r1`, `r2`, `r3` allocated in region `r`, freed in O(1) bulk deallocation
4. **LIFO destruction**: Objects destroyed in reverse declaration order
5. **Deterministic cleanup**: All destruction points are statically determined by scope structure

---

## 2.8.7 Cross-References to Part VI

Part VI (Memory and Resource Management) provides comprehensive formal specifications for the concepts introduced in this overview:

| Topic | Part VI Section |
|-------|-----------------|
| **Objects and Memory Locations** (formal definitions) | §[REF_TBD] |
| **Binding Categories** (let, var) | §[REF_TBD] |
| **Permission System** (const, unique, shared) | §[REF_TBD] |
| **Ownership Transfer and Move Semantics** (transfer rules, Copy types) | §[REF_TBD] |
| **Regions and Lifetimes** (escape analysis, region parameters) | §[REF_TBD] |
| **Storage Duration** (complete specification, initialization order) | §[REF_TBD] |
| **Memory Layout** (sizes, alignment, padding) | §[REF_TBD] |
| **Aliasing and Uniqueness** (formal aliasing rules) | §[REF_TBD] |
| **Unsafe Operations** (unsafe blocks, safety obligations) | §[REF_TBD] |

**Principle**: Part II provides conceptual foundations for understanding program structure. Part VI provides formal semantics for memory safety and resource management.

---

## 2.8.8 Conformance Requirements

A conforming implementation shall satisfy the following requirements with respect to memory and objects:

1. **RAII enforcement** (§2.8.3.1): The implementation shall automatically invoke destructors at scope exit.

2. **LIFO destruction** (§2.8.3.2): The implementation shall destroy objects in reverse declaration order within a scope.

3. **Deterministic cleanup** (§2.8.3.3): The implementation shall ensure cleanup points are statically determinable from the program's lexical structure.

4. **Storage duration** (§2.8.4): The implementation shall provide all four storage duration categories: static, automatic, region, and heap.

5. **Alignment** (§2.8.5.2): The implementation shall respect alignment requirements for all types.

---

## 2.8.9 Notes and Examples

### Informative Note 1: RAII and Exception Safety

RAII provides automatic cleanup even in the presence of early returns and error conditions (via `result`, `break`, `continue`, etc.). This ensures resources are always released, preventing leaks.

### Informative Note 2: Performance of Storage Durations

Different storage durations have different performance characteristics:
- **Static**: No allocation cost (allocated before `main`)
- **Automatic**: Very fast (stack pointer adjustment)
- **Region**: Fast allocation, extremely fast bulk deallocation (O(1))
- **Heap**: Slower allocation/deallocation (general-purpose allocator overhead)

### Informative Note 3: Destructors

Not all types have destructors. Types with no resources to clean up (e.g., `i32`, `bool`) have trivial destructors (no-ops). Types that manage resources (e.g., `File`, `Socket`, user-defined resource wrappers) have non-trivial destructors that release those resources.

Destructor implementation is specified in §[REF_TBD] (Part VI).

### Informative Note 4: Relationship to C++ RAII

Cursive's RAII is similar to C++'s RAII but with important differences:
- **Move by default**: Cursive uses move semantics by default (like Rust), not copy semantics (like C++)
- **No implicit copies**: Copying must be explicit via `Copy` predicate (§[REF_TBD])
- **Region support**: Cursive adds region-based memory management for bulk deallocation
- **Permission system**: Cursive's permission system (§[REF_TBD]) provides compile-time memory safety

---

**End of Section 2.8: Memory and Objects Overview**
