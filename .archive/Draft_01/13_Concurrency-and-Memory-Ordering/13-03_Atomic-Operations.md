# Cursive Language Specification

## Clause 13 — Concurrency and Memory Ordering

**Part**: XIII — Concurrency and Memory Ordering  
**File**: 14-3_Atomic-Operations.md  
**Section**: §13.3 Atomic Operations and Orderings  
**Stable label**: [concurrency.atomic]  
**Forward references**: §14.2 [concurrency.memory], §14.4 [concurrency.synchronization], Clause 7 §7.2 [type.primitive], Clause 11 §11.4 [memory.permission]

---

### §13.3 Atomic Operations and Orderings [concurrency.atomic]

#### §13.3.1 Overview

[1] Atomic operations provide indivisible read-modify-write operations on primitive types without requiring locks. They enable lock-free algorithms, synchronization primitives, and fine-grained concurrent data structures.

[2] Cursive provides atomic operations as built-in procedures operating on `shared` primitive types. No wrapper types (`AtomicI32`, etc.) are needed—atomic operations work directly on shared integer and pointer types.

[3] This subclause specifies atomic operation syntax, memory ordering semantics, available operations, and integration with the happens-before memory model (§14.2).

#### §13.3.2 Memory Orderings

[4] Atomic operations accept a memory ordering parameter controlling synchronization strength:

```cursive
enum Ordering {
    Relaxed,   // No synchronization
    Acquire,   // Synchronize with Release stores
    Release,   // Synchronize with Acquire loads
    AcqRel,    // Both Acquire and Release
    SeqCst,    // Sequentially consistent
}
```

[5] **Ordering semantics**:

**Relaxed**:
[6] No synchronization guarantees. The operation is atomic (indivisible) but establishes no happens-before edges. Appropriate for counters where ordering doesn't matter.

**Acquire**:
[7] Used for loads. Synchronizes-with Release stores on the same location. All operations after the Acquire load in program order cannot be reordered before it.

**Release**:
[8] Used for stores. Synchronizes-with Acquire loads on the same location. All operations before the Release store in program order cannot be reordered after it.

**AcqRel**:
[9] Combines Acquire and Release. Used for read-modify-write operations. Acts as both Acquire (for the read) and Release (for the write).

**SeqCst**:
[10] Sequential consistency. Strongest ordering. All SeqCst operations have a single total order across all threads. Default when ordering is omitted.

#### §13.3.3 Atomic Operations

[11] Atomic operations are provided as built-in procedures in the `atomic` namespace:

##### §13.3.3.1 Atomic Load

```cursive
procedure atomic::load<T>(location: const T, ordering: Ordering): T
    [[ sync::atomic |- ordering in {Acquire, SeqCst, Relaxed} => true ]]
    where T: primitive integer or pointer type
```

[12] Atomically reads the value at `location` with specified memory ordering.

**Type rule**:

$$
\frac{\Gamma \vdash \ell : \text{const } T \quad T \in \text{AtomicTypes} \quad ord \in \{\text{Acquire}, \text{SeqCst}, \text{Relaxed}\}}
     {\Gamma \vdash \texttt{atomic::load}(\ell, ord) : T}
\tag{T-Atomic-Load}
$$

where $\text{AtomicTypes} = \{i8, i16, i32, i64, i128, isize, u8, u16, u32, u64, u128, usize, \text{Ptr}\langle T \rangle\}$.

##### §13.3.3.2 Atomic Store

```cursive
procedure atomic::store<T>(location: shared T, value: T, ordering: Ordering)
    [[ sync::atomic |- ordering in {Release, SeqCst, Relaxed} => true ]]
    where T: primitive integer or pointer type
```

[13] Atomically writes `value` to `location` with specified memory ordering.

**Type rule**:

$$
\frac{\Gamma \vdash \ell : \text{shared } T \quad \Gamma \vdash v : T \quad ord \in \{\text{Release}, \text{SeqCst}, \text{Relaxed}\}}
     {\Gamma \vdash \texttt{atomic::store}(\ell, v, ord) : ()}
\tag{T-Atomic-Store}
$$

##### §13.3.3.3 Atomic Compare-Exchange

```cursive
procedure atomic::compare_exchange<T>(
    location: shared T,
    expected: T,
    desired: T,
    success_order: Ordering,
    failure_order: Ordering
): bool
    [[ sync::atomic |- true => true ]]
    where T: primitive integer or pointer type
```

[14] Atomically compares `location` with `expected`. If equal, stores `desired` and returns `true`. If not equal, returns `false` without modification.

[15] **Success ordering**: Used when comparison succeeds. May be any ordering.

[16] **Failure ordering**: Used when comparison fails. Shall not be stronger than success ordering. Shall not be Release or AcqRel (loads cannot release).

##### §13.3.3.4 Atomic Fetch-And-Op

```cursive
procedure atomic::fetch_add<T>(location: shared T, value: T, ordering: Ordering): T
    [[ sync::atomic |- true => true ]]
    where T: integer type

procedure atomic::fetch_sub<T>(location: shared T, value: T, ordering: Ordering): T
    [[ sync::atomic |- true => true ]]
    where T: integer type

procedure atomic::fetch_and<T>(location: shared T, value: T, ordering: Ordering): T
    [[ sync::atomic |- true => true ]]
    where T: integer type

procedure atomic::fetch_or<T>(location: shared T, value: T, ordering: Ordering): T
    [[ sync::atomic |- true => true ]]
    where T: integer type

procedure atomic::fetch_xor<T>(location: shared T, value: T, ordering: Ordering): T
    [[ sync::atomic |- true => true ]]
    where T: integer type
```

[17] Atomically performs the operation and returns the previous value.

#### §13.3.4 Constraints

[1] _Atomic type requirement._ Atomic operations shall operate only on primitive integer types and pointer types. Attempting atomic operations on composite types (records, tuples, arrays) produces diagnostic E13-200.

[2] _Permission requirement._ Atomic loads require `const` permission. Atomic stores and RMW operations require `shared` permission. Using atomic operations on `unique` produces diagnostic E13-201.

[3] _Ordering validity._ Memory orderings shall be valid for the operation:
- Load: Acquire, SeqCst, Relaxed
- Store: Release, SeqCst, Relaxed  
- RMW: Any ordering

Invalid orderings produce diagnostic E13-202.

[4] _Alignment requirement._ Atomic operations require natural alignment for the type. Misaligned atomic access produces undefined behavior [UB-ID: B.2.51].

[5] _Size limits._ Atomic operations are guaranteed for types up to pointer width (32-bit or 64-bit). Larger types (i128, u128) may not support atomic operations on all platforms. Using atomic operations on unsupported types produces diagnostic E13-203.

#### §13.3.5 Semantics

##### §13.3.5.1 Atomicity Guarantee

[6] Atomic operations are indivisible: no thread can observe partial completion. For operation $\text{atomic\_op}(\ell)$:

$$
\frac{\langle \text{atomic\_op}(\ell), \sigma \rangle \Downarrow \langle v, \sigma' \rangle}
     {\text{No intermediate state observable}}
\tag{E-Atomic-Indivisible}
$$

##### §13.3.5.2 Ordering Semantics

[7] **Relaxed**: No synchronization. Only atomicity guaranteed:

```cursive
var counter: shared i32 = 0

// Thread 1:
atomic::store(counter, 1, Ordering::Relaxed)

// Thread 2:
let value = atomic::load(counter, Ordering::Relaxed)
// Might see 0 or 1, no synchronization guarantee
```

[8] **Acquire/Release**: Establishes synchronization:

```cursive
var flag: shared i32 = 0
var data: shared i32 = 0

// Thread 1 (producer):
atomic::store(data, 42, Ordering::Relaxed)
atomic::store(flag, 1, Ordering::Release)  // Synchronizes

// Thread 2 (consumer):
loop {
    if atomic::load(flag, Ordering::Acquire) == 1 {  // Synchronizes
        let value = atomic::load(data, Ordering::Relaxed)
        // Guaranteed to see 42 (happens-before from Release to Acquire)
        break
    }
}
```

[9] **SeqCst**: Total order across all threads:

```cursive
var x: shared i32 = 0
var y: shared i32 = 0

// Thread 1:
atomic::store(x, 1, Ordering::SeqCst)

// Thread 2:
atomic::store(y, 1, Ordering::SeqCst)

// Thread 3:
let r1 = atomic::load(x, Ordering::SeqCst)
let r2 = atomic::load(y, Ordering::SeqCst)

// Thread 4:
let r3 = atomic::load(y, Ordering::SeqCst)
let r4 = atomic::load(x, Ordering::SeqCst)

// SeqCst guarantees: NOT possible for r1=r2=0 AND r3=r4=0
// (There exists a total order consistent with all observations)
```

#### §13.3.6 Examples

**Example 14.3.6.1 (Atomic counter)**:

```cursive
var global_counter: shared i32 = 0

procedure increment_counter()
    [[ sync::atomic |- true => true ]]
{
    atomic::fetch_add(global_counter, 1, Ordering::Relaxed)
}

procedure parallel_count(threads: usize)
    [[ thread::spawn, thread::join, sync::atomic |- threads > 0 => true ]]
{
    var handles: [Thread<()>@Spawned] = []
    
    loop i in 0..threads {
        let thread = spawn(|| [[ sync::atomic ]] {
            loop j in 0..1000 {
                increment_counter()
            }
        })
        handles.push(thread)
    }
    
    loop thread in handles {
        thread.join()
    }
    
    let total = atomic::load(global_counter, Ordering::Relaxed)
    println("Total: {}", total)  // Guaranteed: threads * 1000
}
```

**Example 14.3.6.2 (Lock-free stack)**:

```cursive
record Node<T> {
    value: T,
    next: shared Ptr<Node<T>>@Valid,
}

record LockFreeStack<T> {
    head: shared Ptr<Node<T>>@Valid,
}

procedure LockFreeStack.push<T>(~%, value: T)
    [[ sync::atomic, alloc::heap |- true => true ]]
{
    let node = Node {
        value,
        next: Ptr::null(),
    }
    
    loop {
        let old_head = atomic::load(self.head, Ordering::Acquire)
        node.next = old_head
        
        if atomic::compare_exchange(
            self.head,
            old_head,
            &node,
            Ordering::Release,
            Ordering::Relaxed
        ) {
            break  // Successfully pushed
        }
        // CAS failed, retry
    }
}
```

#### §13.3.7 Diagnostics

[10] Atomic operation diagnostics:

| Code    | Condition                                      | Constraint |
| ------- | ---------------------------------------------- | ---------- |
| E13-200 | Atomic operation on non-atomic type            | [1]        |
| E13-201 | Atomic operation with incorrect permission     | [2]        |
| E13-202 | Invalid memory ordering for operation          | [3]        |
| E13-203 | Atomic operation on unsupported type (e.g. i128) | [5]      |
| E13-204 | Misaligned atomic access                       | [4]        |

#### §13.3.8 Conformance Requirements

[11] Implementations shall:

1. Provide atomic operations for all primitive integer types up to pointer width
2. Support atomic operations on pointer types (Ptr<T>)
3. Implement all five memory orderings (Relaxed, Acquire, Release, AcqRel, SeqCst)
4. Enforce permission requirements: const for loads, shared for stores/RMW
5. Validate memory orderings for each operation type
6. Require natural alignment for atomic operations
7. Document platform support for i128/u128 atomics
8. Guarantee atomicity (indivisibility) for all atomic operations
9. Implement synchronizes-with relation for Acquire/Release orderings
10. Integrate atomic operations with happens-before memory model
11. Emit diagnostics E14-200 through E14-204 for violations

---

**Previous**: §14.2 Memory Model (§14.2 [concurrency.memory]) | **Next**: §14.4 Synchronization Primitives (§14.4 [concurrency.synchronization])

