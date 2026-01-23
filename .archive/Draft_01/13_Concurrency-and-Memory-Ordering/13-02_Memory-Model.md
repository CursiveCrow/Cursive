# Cursive Language Specification

## Clause 13 — Concurrency and Memory Ordering

**Part**: XIII — Concurrency and Memory Ordering  
**File**: 14-2_Memory-Model.md  
**Section**: §13.2 Memory Model  
**Stable label**: [concurrency.memory]  
**Forward references**: §14.1 [concurrency.model], §14.3 [concurrency.atomic], §14.4 [concurrency.synchronization], Clause 11 [memory]

---

### §13.2 Memory Model [concurrency.memory]

#### §13.2.1 Overview

[1] The concurrent memory model defines the visibility and ordering of memory operations across threads. It specifies when writes from one thread become visible to reads in another thread, establishes the happens-before relation for program ordering, and guarantees data-race freedom for well-typed programs.

[2] This subclause integrates with the sequential memory model (Clause 11) to provide a complete specification of memory semantics in both single-threaded and multi-threaded contexts.

#### §13.2.2 Sequential Consistency for Data-Race-Free Programs

[3] Cursive guarantees **sequential consistency** for data-race-free (DRF) programs: programs execute as if operations from all threads were interleaved in some sequential order respecting the program order within each thread.

[4] **Data-race freedom**: A program is data-race-free when all pairs of concurrent operations accessing the same memory location satisfy at least one of:
1. Both operations are reads
2. Operations are ordered by synchronization (happens-before)
3. At least one operation is atomic (§14.3)

[5] **Guarantee**: Well-typed Cursive programs without `unsafe` blocks are data-race-free by construction. The permission system ensures this property.

#### §13.2.3 Happens-Before Relation

[6] The _happens-before_ relation orders operations across threads. If operation A happens-before operation B, then A's effects are visible to B.

[7] **Happens-before rules**:

**Intra-thread ordering**:
$$
\frac{\text{Op}_1 \text{ precedes } \text{Op}_2 \text{ in program order within thread}}
     {\text{Op}_1 \text{ happens-before } \text{Op}_2}
\tag{HB-Sequential}
$$

**Thread spawn**:
$$
\frac{\text{thread } t \text{ spawned at point } p}
     {\forall \text{ ops before } p \text{ happen-before all ops in } t}
\tag{HB-Spawn}
$$

**Thread join**:
$$
\frac{\text{thread } t \text{ joined at point } j}
     {\forall \text{ ops in } t \text{ happen-before all ops after } j}
\tag{HB-Join}
$$

**Synchronization**:
$$
\frac{\text{Release op } r \text{ on location } \ell \quad \text{Acquire op } a \text{ on } \ell \quad r \text{ synchronizes-with } a}
     {r \text{ happens-before } a}
\tag{HB-Sync}
$$

**Transitivity**:
$$
\frac{A \text{ happens-before } B \quad B \text{ happens-before } C}
     {A \text{ happens-before } C}
\tag{HB-Trans}
$$

#### §13.2.4 Synchronizes-With Relation

[8] The _synchronizes-with_ relation establishes ordering between release and acquire operations on the same location:

**Mutex unlock/lock**:
[9] Unlocking a mutex synchronizes-with the next lock acquisition on that mutex:

$$
\frac{\text{Thread}_1 \text{ unlocks mutex } m \quad \text{Thread}_2 \text{ locks mutex } m}
     {\text{unlock } m \text{ synchronizes-with lock } m}
\tag{SW-Mutex}
$$

**Channel send/receive**:
[10] Sending a value on a channel synchronizes-with receiving that value:

$$
\frac{\text{Thread}_1 \text{ sends } v \text{ on channel } c \quad \text{Thread}_2 \text{ receives from } c}
     {\text{send}(v) \text{ synchronizes-with receive}()}
\tag{SW-Channel}
$$

**Atomic release/acquire**:
[11] Atomic store with Release ordering synchronizes-with atomic load with Acquire ordering:

$$
\frac{\text{Atomic store}_{\text{Release}}(\ell, v) \quad \text{Atomic load}_{\text{Acquire}}(\ell) = v}
     {\text{store}_{\text{Release}} \text{ synchronizes-with load}_{\text{Acquire}}}
\tag{SW-Atomic}
$$

#### §13.2.5 Permission-Based Data Race Prevention

[12] The permission system prevents data races at compile time:

**const permission**:
[13] Multiple threads may read `const` data concurrently. Since `const` prohibits mutation (§11.4.2.1), no write/write or read/write races can occur.

[ Given: Value $v : \text{const } T$ shared across threads ]

$$
\frac{\text{All operations on } v \text{ are reads}}
     {\text{No data race on } v}
\tag{P-Const-NoRace}
$$

**unique permission**:
[14] `unique` data cannot be aliased (§11.7.3). When transferred to another thread via `move`, the source thread loses all access. Exclusive access in destination thread prevents races.

[ Given: Value $v : \text{unique } T$ transferred to thread ]

$$
\frac{\text{Source thread: } v \text{ becomes invalid} \quad \text{Dest thread: exclusive access}}
     {\text{No concurrent access} \Rightarrow \text{No data race}}
\tag{P-Unique-NoRace}
$$

**shared permission with synchronization**:
[15] `shared` data requires synchronization primitives (§14.4). Mutex, RwLock, and atomic operations provide the necessary happens-before edges to prevent races.

#### §13.2.6 Memory Operation Ordering

[16] Within a single thread, operations execute in program order (sequential consistency per thread). Across threads, only operations ordered by happens-before have guaranteed ordering.

[17] **Example of unordered operations**:

```cursive
// Thread 1:
x = 1
y = 2

// Thread 2:
let a = y  // Might see 0, 2, or intermediate state
let b = x  // Might see 0, 1, or intermediate state
```

Without synchronization, Thread 2 may observe operations from Thread 1 in any order.

[18] **Example with synchronization**:

```cursive
let lock = Mutex::new(())

// Thread 1:
x = 1
y = 2
lock.unlock()  // Release

// Thread 2:
lock.lock()    // Acquire
let a = y      // Guaranteed to see 2
let b = x      // Guaranteed to see 1
```

The mutex unlock synchronizes-with lock, establishing happens-before from all operations before unlock to all operations after lock.

#### §13.2.7 Conformance Requirements

[19] Implementations shall:

1. Guarantee sequential consistency for data-race-free programs
2. Implement happens-before relation per rules in §14.2.3
3. Implement synchronizes-with relation for all synchronization primitives
4. Ensure permission-based race prevention: const allows concurrent reads, unique prevents aliasing, shared requires synchronization
5. Prohibit data races in well-typed programs (races are undefined behavior: UB-ID: B.2.50)
6. Support memory orderings for atomic operations (§14.3): Relaxed, Acquire, Release, AcqRel, SeqCst
7. Document platform-specific memory model details (cache coherency, memory barriers)

---

**Previous**: §14.1 Concurrency Model (§14.1 [concurrency.model]) | **Next**: §14.3 Atomic Operations and Orderings (§14.3 [concurrency.atomic])

