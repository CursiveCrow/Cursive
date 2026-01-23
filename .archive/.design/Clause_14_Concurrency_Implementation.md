# Clause 14: Concurrency and Memory Ordering - Implementation Summary

**Date**: November 8, 2025  
**Status**: ✅ Complete  
**Scope**: Full specification of Cursive's concurrency model

---

## Executive Summary

Successfully designed and implemented Clause 14 (Concurrency and Memory Ordering) following SpecificationOnboarding.md standards. The design uses **permissions instead of Send/Sync markers**, **modal types for synchronization primitives**, and **explicit grants for concurrent operations**.

**Key innovation**: Thread safety through existing permission system, not separate marker behaviors.

---

## Files Created (4 new specification files)

### 1. §14.1 Concurrency Model (14-1_Concurrency-Model.md)

**Content**:
- Thread as modal type (Thread<T> with @Spawned/@Joined/@Detached states)
- Permission-based thread safety (const/unique/shared)
- Thread spawning with permission constraints
- Join and detach operations
- Grant propagation across thread boundaries
- Removal of Send/Sync markers (integrated with permissions)

**Key specifications**:
- `spawn<T>(action: () -> T ! ε): Thread<T>@Spawned where T: const`
- Thread return types must be const (immutable, safe to share)
- Closure captures: const (share), unique (move), no shared without sync

**Lines**: ~280 lines  
**Examples**: 4 canonical examples + 1 invalid example  
**Diagnostics**: E14-100 through E14-104

---

### 2. §14.2 Memory Model (14-2_Memory-Model.md)

**Content**:
- Sequential consistency for data-race-free programs
- Happens-before relation (intra-thread, spawn, join, sync)
- Synchronizes-with relation (mutex, channel, atomics)
- Permission-based data race prevention
- Memory operation ordering

**Key specifications**:
- Formal happens-before rules with mathematical notation
- Permission-based race prevention proofs
- Integration with single-threaded memory model (Clause 11)

**Lines**: ~200 lines  
**Formal rules**: 8 happens-before/synchronizes-with rules  
**Conformance**: 7 requirements

---

### 3. §14.3 Atomic Operations (14-3_Atomic-Operations.md)

**Content**:
- Memory orderings (Relaxed, Acquire, Release, AcqRel, SeqCst)
- Atomic operations on shared primitives (no wrapper types)
- Atomic load, store, compare-exchange, fetch-and-op
- Integration with happens-before memory model

**Key specifications**:
- `atomic::load<T>(location: const T, ordering: Ordering): T`
- `atomic::store<T>(location: shared T, value: T, ordering: Ordering)`
- `atomic::compare_exchange<T>(...) -> bool`
- Fetch-add, fetch-sub, fetch-and, fetch-or, fetch-xor

**Lines**: ~230 lines  
**Examples**: 2 (atomic counter, lock-free stack)  
**Diagnostics**: E14-200 through E14-204

---

### 4. §14.4 Synchronization Primitives (14-4_Synchronization-Primitives.md)

**Content**:
- Mutex as modal type (@Unlocked/@Locked)
- Channel as modal type (@ Open/@SenderClosed/@ReceiverClosed/@Closed)
- RwLock as modal type (@Unlocked/@ReadLocked/@WriteLocked)
- RAII guard pattern (MutexGuard with Drop)
- Synchronizes-with semantics for each primitive

**Key specifications**:
- Modal state machines for all sync primitives
- Permission integration (const for read locks, unique for write locks)
- Channel type constraint: `T: const` for messages

**Lines**: ~270 lines  
**Examples**: 3 (mutex, producer-consumer, RwLock)  
**Diagnostics**: E14-300, E14-301

---

## Files Modified (2 existing files)

### 5. Clause 10-4: Predicate Declarations

**Changes**:
- ❌ **Removed** §10.4.5.3 Send Behavior (~100 lines deleted)
- ❌ **Removed** §10.4.5.4 Sync Behavior (~100 lines deleted)
- ✅ **Added** note explaining thread safety via permissions
- ✅ **Updated** Table 10.4.2 (marker behaviors: 5 → 3)
- ✅ **Updated** conformance requirements

**Rationale added**:
> Thread safety in Cursive is provided through the permission system (§11.4) rather than separate marker behaviors. The `const` permission indicates thread-safe sharing (immutable), `unique` indicates transferability across threads (exclusive ownership), and `shared` requires explicit synchronization primitives (Mutex, Channel).

---

### 6. Annex E: Implementation Diagnostics

**Changes**:
- ✅ **Added** E14-xxx diagnostic code range (12 new codes)
- ✅ **Added** §E.5.1.10 Clause 14 diagnostic registry
- ✅ **Organized** by subsection (14.1, 14.3, 14.4)

---

## Design Principles

The concurrency specification embodies Cursive's core philosophy:

### 1. Permissions Instead of Markers

**Decision**: No Send/Sync behaviors

**Rationale**:
- Permissions (`const`/`unique`/`shared`) already encode thread safety
- `const` = safe to share (like Sync)
- `unique` = safe to transfer (like Send)  
- `shared` = requires synchronization
- Avoids redundancy between permission system and marker system
- Simpler mental model (one concept instead of two)

### 2. Modal Types for Lifecycle

**Thread lifecycle**: @Spawned → @Joined or @Detached  
**Mutex lifecycle**: @Unlocked ⇄ @Locked  
**Channel lifecycle**: @Open → @SenderClosed / @ReceiverClosed → @Closed

**Benefits**:
- State transitions enforced by type system
- Invalid operations prevented at compile time
- Lifecycle explicit in type signatures

### 3. Explicit Grants

**Required grants**:
- `thread::spawn` - create threads
- `thread::join` - wait for threads
- `sync::lock` - acquire locks
- `sync::channel` - message passing
- `sync::atomic` - atomic operations

**Benefits**:
- Concurrency visible in procedure signatures
- Grant propagation tracks effects
- Easy to audit concurrent operations

### 4. No Wrapper Types for Atomics

**Design**: Atomic operations on `shared` primitives directly

```cursive
var counter: shared i32 = 0
atomic::fetch_add(counter, 1, Ordering::Relaxed)
```

**Not**:
```rust
let counter: AtomicI32 = AtomicI32::new(0);  // Rust-style wrapper
counter.fetch_add(1, Ordering::Relaxed);
```

**Benefits**:
- Simpler (no wrapper types)
- Permission-based (shared indicates concurrent access)
- Explicit (atomic:: namespace shows atomic operation)

---

## Comparison: Cursive vs Rust Concurrency

| Feature | Rust | Cursive |
|---------|------|---------|
| **Thread safety markers** | Send/Sync traits | Permissions (const/unique/shared) |
| **Thread creation** | `std::thread::spawn` | `spawn` with permission constraints |
| **Data sharing** | Arc<T> reference counting | const permission (no ref counting) |
| **Mutual exclusion** | Mutex<T> wrapper type | Mutex<T> modal type (@Unlocked/@Locked) |
| **Atomics** | AtomicI32 wrapper types | atomic::ops on shared i32 directly |
| **Message passing** | mpsc::channel() | Channel<T> modal type |
| **Memory model** | Happens-before (same) | Happens-before (same) |
| **Lock guards** | MutexGuard<T> RAII | MutexGuard<T> RAII (same pattern) |

**Similarities**: Happens-before memory model, RAII guards, atomic orderings

**Key differences**: Permissions not markers, modal types for state, no wrapper types for atomics

---

## Technical Specifications

### Thread Safety Rules

**const types are thread-safe** (P-Const-ThreadSafe):

$$\frac{v : \text{const } T}{\text{Sharing } v \text{ across threads is safe}}$$

**unique types are transferable** (P-Unique-Transfer):

$$\frac{v : \text{unique } T \quad \text{move } v \text{ to thread}}{\text{Transfer is safe (exclusive ownership)}}$$

**shared types need synchronization**:

Accessing `shared` data from multiple threads requires Mutex, RwLock, or atomic operations.

### Happens-Before Relation

**Thread spawn** (HB-Spawn):

$$\frac{\text{thread } t \text{ spawned at point } p}{\forall \text{ ops before } p \text{ happen-before all ops in } t}$$

**Thread join** (HB-Join):

$$\frac{\text{thread } t \text{ joined at point } j}{\forall \text{ ops in } t \text{ happen-before all ops after } j}$$

**Synchronization** (HB-Sync):

$$\frac{\text{Release op } r \text{ synchronizes-with Acquire op } a}{r \text{ happens-before } a}$$

### Memory Orderings

1. **Relaxed**: Atomic only, no synchronization
2. **Acquire**: Loads synchronize with Release stores
3. **Release**: Stores synchronize with Acquire loads
4. **AcqRel**: Both Acquire and Release (for RMW ops)
5. **SeqCst**: Sequential consistency (total order)

---

## Complete Examples

### Example 1: Parallel Computation with Const Sharing

```cursive
procedure parallel_sum(numbers: const [i32]): i32
    [[ thread::spawn, thread::join |- numbers.len() >= 2 => true ]]
{
    let mid = numbers.len() / 2
    
    let left = spawn(|| [[ |- true => true ]] {
        sum_range(numbers, 0, mid)
    })
    
    let right = spawn(|| [[ |- true => true ]] {
        sum_range(numbers, mid, numbers.len())
    })
    
    result left.join().result + right.join().result
}
```

### Example 2: Producer-Consumer with Channel

```cursive
procedure pipeline()
    [[ thread::spawn, thread::join, sync::channel |- true => true ]]
{
    let channel: Channel<i32>@Open = Channel::new()
    
    let producer = spawn(move || [[ sync::channel ]] {
        loop i in 0..100 {
            channel.send(i)
        }
    })
    
    let consumer = spawn(move || [[ sync::channel, io::write ]] {
        loop {
            match channel.receive() {
                value: i32 => println("{}", value),
                err: ChannelError => break,
            }
        }
    })
    
    producer.join()
    consumer.join()
}
```

### Example 3: Mutex-Protected Counter

```cursive
var counter: Mutex<i32>@Unlocked = Mutex::new(0)

procedure increment()
    [[ sync::lock |- true => true ]]
{
    let locked = counter.lock()
    locked.data = locked.data + 1
    locked.unlock()
}
```

### Example 4: Lock-Free with Atomics

```cursive
var counter: shared i32 = 0

procedure atomic_increment()
    [[ sync::atomic |- true => true ]]
{
    atomic::fetch_add(counter, 1, Ordering::Relaxed)
}
```

---

## Integration with Existing Clauses

### Clause 7: Type System
- Thread, Mutex, Channel, RwLock are modal types (§7.6)
- Follow same state transition semantics as FileHandle, Arena

### Clause 10: Generics and Behaviors
- **Removed** Send and Sync marker behaviors
- Thread safety expressed through permissions instead
- Simplified marker behavior system to Copy, Sized, Drop

### Clause 11: Memory Model
- Permissions (const/unique/shared) encode thread safety
- const = safe to share
- unique = safe to transfer
- shared = requires synchronization

### Clause 12: Contracts
- Grants track concurrent operations (thread::spawn, sync::lock, sync::atomic)
- Grant propagation across thread boundaries
- Sequents specify concurrency requirements

---

## Conformance Requirements Summary

Implementations shall:

1. **Thread Modal Type** (§14.1):
   - Provide Thread<T> with @Spawned/@Joined/@Detached states
   - Enforce const constraint on return types
   - Support spawn, join, detach operations
   - Establish happens-before at spawn/join boundaries

2. **Memory Model** (§14.2):
   - Guarantee sequential consistency for DRF programs
   - Implement happens-before relation
   - Implement synchronizes-with for all sync primitives
   - Prevent data races via permission checking

3. **Atomic Operations** (§14.3):
   - Provide atomic ops for integers and pointers
   - Support all five memory orderings
   - Operate on shared primitives directly (no wrappers)
   - Enforce alignment and permission requirements

4. **Synchronization** (§14.4):
   - Provide Mutex, Channel, RwLock as modal types
   - Implement state transitions (lock/unlock, send/receive)
   - Establish synchronizes-with relations
   - Integrate permissions with lock states

5. **No Send/Sync** (§10.4.5):
   - Removed Send and Sync marker behaviors
   - Thread safety via permissions exclusively
   - Simpler marker behavior system (3 instead of 5)

---

## Diagnostic Codes Added

### Concurrency Model (E14-100 to E14-104)
- E14-100: Thread return type not const
- E14-101: Shared data captured without synchronization
- E14-102: Thread dropped without join/detach
- E14-103: Missing thread::spawn grant
- E14-104: Missing thread::join grant

### Atomic Operations (E14-200 to E14-204)
- E14-200: Atomic operation on non-atomic type
- E14-201: Atomic operation with incorrect permission
- E14-202: Invalid memory ordering
- E14-203: Atomic op on unsupported type
- E14-204: Misaligned atomic access

### Synchronization (E14-300 to E14-301)
- E14-300: Type constraint violation for sync primitive
- E14-301: Missing grant for synchronization operation

**Total**: 12 new diagnostic codes registered in Annex E

---

## Why Permissions Replace Send/Sync

### The Problem with Send/Sync Markers

Rust uses Send/Sync because:
1. &T and &mut T don't encode thread safety
2. Interior mutability requires separate checking
3. Need to distinguish "safe to transfer" from "safe to share"

### Why Cursive Doesn't Need Them

Cursive's permission system already provides:

| Permission | Thread Safety Property | Equivalent To |
|------------|------------------------|---------------|
| `const` | Safe to share immutably | Sync |
| `unique` | Safe to transfer exclusively | Send |
| `shared` | Requires synchronization | (neither - needs Mutex) |

**Key insight**: Permissions are more expressive than Send/Sync because they also control mutation, not just thread safety.

### Benefits of Permission-Based Approach

1. **No redundancy**: One system (permissions) instead of two (permissions + markers)
2. **Explicit**: Permissions visible in type annotations
3. **Simpler**: Fewer concepts to learn
4. **Consistent**: Same permission system for single-threaded and multi-threaded code
5. **No interior mutability confusion**: Cursive prohibits interior mutability, eliminating need for Cell/RefCell distinctions

---

## Design Validation

### Use Cases Covered

✅ **Parallel computation**: const data sharing across threads  
✅ **Background tasks**: spawn + detach for fire-and-forget  
✅ **Producer-consumer**: Channel message passing  
✅ **Shared mutable state**: Mutex protecting shared data  
✅ **Read-heavy workloads**: RwLock for multiple readers  
✅ **Lock-free algorithms**: Atomic operations with ordering control  
✅ **Thread pools**: Spawn multiple threads, join all  
✅ **Thread-local storage**: Per-thread arena allocation (§11.3)

### Safety Guarantees

✅ **Data-race freedom**: Permission system prevents races at compile time  
✅ **No use-after-free**: Const immutability or unique transfer prevents dangling  
✅ **Type-safe channels**: Channel<T> enforces T: const  
✅ **Deadlock detection**: Runtime detection in debug builds (recommended)  
✅ **Memory ordering**: Explicit ordering parameters prevent subtle bugs

---

## Comparison to Prior Art

### Go
- **Similarity**: Channels for communication
- **Difference**: Cursive uses modal types for channel state, Go uses unbuffered/buffered channels

### Rust
- **Similarity**: Happens-before memory model, atomic orderings, RAII guards
- **Difference**: Permissions not Send/Sync, modal types not plain types, no Arc<T>

### Java
- **Similarity**: Mutex/lock abstractions
- **Difference**: No garbage collection, compile-time race prevention, explicit grants

### C++
- **Similarity**: Atomic operations with memory orderings, std::mutex
- **Difference**: Memory-safe (no raw pointers), modal types for lifecycle, permission checking

---

## Specification Quality Metrics

### ISO/IEC Compliance ✅

- Numbered paragraphs with resets
- Stable labels ([concurrency.model], [concurrency.memory], etc.)
- Formal rules with mathematical notation
- Canonical examples for each major concept
- Forward references documented
- Cross-references to other clauses

### Completeness ✅

- Syntax specified with Annex A references
- Constraints enumerated with diagnostics
- Semantics with formal rules
- Examples showing valid and invalid usage
- Conformance requirements listed
- Integration with other clauses documented

### Consistency ✅

- Follows SpecificationOnboarding.md template
- Matches existing clause structure
- Uses established notation and conventions
- Integrates with permission system (Clause 11)
- Aligns with modal type system (§7.6)

---

## Future Extensions

Clause 14 provides the foundation. Future work could add:

1. **Async/await** (future Clause 14.5):
   - Task modal type (@Pending/@Ready/@Complete)
   - Async procedures and await expressions
   - Future<T> as modal type

2. **Advanced synchronization** (standard library, Clause 17):
   - Semaphore, Barrier, CondVar
   - RwLock upgrades (read → write)
   - Concurrent collections (lock-free queues, maps)

3. **Thread pools** (standard library, Clause 17):
   - WorkerPool modal type
   - Task scheduling strategies
   - Work stealing

4. **Memory ordering relaxations** (implementation guidance):
   - Platform-specific orderings
   - Optimization opportunities
   - Fence operations

---

## Conclusion

Clause 14 (Concurrency and Memory Ordering) is **complete and ready for use**. The design successfully:

✅ Integrates with Cursive's existing permission system  
✅ Uses modal types for synchronization primitive lifecycle  
✅ Avoids Rust-specific concepts (Send/Sync markers, Arc<T>)  
✅ Provides explicit, understandable concurrency model  
✅ Maintains "explicit over implicit" philosophy  
✅ Follows ISO/specification standards throughout

**Total specification text**: ~980 lines across 4 files  
**Formal rules**: 15+ typing and semantics rules  
**Examples**: 10 canonical examples  
**Diagnostics**: 12 error codes

**Status**: Production-ready for Cursive v1.0 specification.

---

**Related documents**:
- Comprehensive specification review: `Reviews/Comprehensive_Review_2025-11-08_Complete.md`
- Arena modal type implementation: `IMPLEMENTATION_SUMMARY.md`

