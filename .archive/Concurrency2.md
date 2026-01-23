# Cursive Concurrency Model Specification

## Part 1: Foundations

### 1.1 Design Principles

The Cursive concurrency model is built on five principles:

1. **Shared data is locked by default.** A `shared` value is opaque—you cannot access its contents without a key.

2. **Keys unlock, not lock.** The mental model is a locked box requiring a key to open, not a guard blocking access.

3. **Keys are implicit and fine-grained.** Accessing a path implicitly acquires a key to exactly that path. Maximum parallelism emerges naturally.

4. **No ambient concurrency.** Spawning tasks requires an explicit `parallel` block. There is no global thread pool or hidden parallelism.

5. **Structured concurrency by default.** Tasks are scoped to `parallel` blocks and complete before the block exits.

---

### 1.2 The Permission System

Three permissions govern data access:

```
    unique       Exclusive ownership — no synchronization needed
       │
    shared       Aliased access — locked by default, key required
       │
     const       Immutable access — no synchronization needed
```

| Permission | Aliased | Readable | Writable | Key Required |
| :--------- | :------ | :------- | :------- | :----------- |
| `unique`   | No      | Yes      | Yes      | No           |
| `shared`   | Yes     | Yes      | Yes      | Yes          |
| `const`    | Yes     | Yes      | No       | No           |

**Why no key for `unique` and `const`?**

- `unique`: Exclusive ownership. No other path to this data exists. No synchronization needed.
- `const`: Data cannot change. Any number of readers is safe. No synchronization needed.
- `shared`: Multiple tasks may read and write. Keys mediate access.

**Subtyping:**

```
unique <: shared <: const
```

A `unique` value can be viewed as `shared` (now requires keys). A `shared` value can be viewed as `const` (drops write capability, no keys needed for read-only access).

---

### 1.3 Shared Data is Opaque

A `shared` value cannot be accessed directly:

```cursive
let player: shared Player = ...

player.health           // ERROR E-KEY-0001: shared path requires key
player.health = 100     // ERROR E-KEY-0001: shared path requires key
```

Access requires a key. Keys are acquired **implicitly by accessing the data**:

```cursive
let player: shared Player = ...

player.health = 100     // OK: key to player.health implicitly acquired and released
```

The access itself is the key acquisition. No ceremony required.

---

### 1.4 Keys

A **key** is a runtime proof of access rights to a specific path within `shared` data.

**Key properties:**

- **Path-specific:** A key to `player.health` does not unlock `player.mana`
- **Implicitly acquired:** Accessing a `shared` path acquires the key automatically
- **Scoped:** Keys are held for the minimal necessary duration (single statement by default)
- **Reentrant:** If a key covering a path is already held, nested access succeeds without additional acquisition

**Key modes:**

| Mode  | Grants       | Multiple Holders | Acquired By  |
| :---- | :----------- | :--------------- | :----------- |
| Read  | Read         | Yes              | Read access  |
| Write | Read + Write | No               | Write access |

---

### 1.5 Implicit Key Acquisition

Every access to a `shared` path implicitly acquires a key:

```cursive
let player: shared Player = ...

// Read: acquire read key, read, release
let hp = player.health

// Write: acquire write key, write, release
player.health = 100

// Read-modify-write: acquire write key for entire statement
player.health += 10
```

**Key Scope:**

Keys are held for the **minimal scope** that preserves correctness:

```cursive
// Single statement: key acquired and released within statement
player.health = 100

// Read-modify-write: write key held for entire operation
player.health += 10

// Independent accesses: separate key acquisitions
let hp = player.health    // Key acquired, read, released
let mp = player.mana      // Key acquired, read, released
```

---

### 1.6 Deadlock Prevention

Keys are acquired in **canonical order** (lexicographic by path, depth-first) when multiple keys are needed in a single expression:

```cursive
// Acquires keys in canonical order: health, then mana
player.health = player.mana * 2
```

**Dynamic Index Restriction:**

When array indices cannot be statically proven disjoint within a statement, explicit coarsening is required:

```cursive
procedure swap(arr: shared [i32; N], i: usize, j: usize) {
    // ERROR E-KEY-0010: Potential key conflict on dynamic indices arr[i], arr[j]
    let temp = arr[i]
    arr[i] = arr[j]
    arr[j] = temp
}

// Correct: explicit coarsening
procedure swap(arr: shared [i32; N], i: usize, j: usize) {
    #arr                      // Acquire key to entire array
    let temp = arr[i]
    arr[i] = arr[j]
    arr[j] = temp
}
```

---

### 1.7 Key Reentry

Keys are **reentrant**. When a task holds a key covering a path, nested access to that path or sub-paths succeeds without additional acquisition:

```cursive
procedure outer(p: shared Player) {
    #p                        // Acquire coarse key to p
    inner(p)                  // inner's accesses are covered
}

procedure inner(p: shared Player) {
    p.health = 100            // Covered by outer's key, no new acquisition
    p.mana = 50               // Covered by outer's key, no new acquisition
}
```

The compiler statically elides redundant acquisitions where provable. Runtime tracking handles remaining cases.

---

### 1.8 The `#` Annotation

By default, keys are acquired at the **finest grain**—each distinct path gets its own key.

The `#` annotation **coarsens** the key acquisition point.

**On direct field access:**

```cursive
player.health = 100           // Key: player.health (fine-grained)
#player.health = 100          // Key: player (coarse)
#player.stats.health = 100    // Key: player.stats
```

**Standalone (block-scoped):**

```cursive
{
    #player                   // Acquire key to player for this block
    player.health = 100       // Uses held key
    player.mana = 50          // Uses held key
    player.position = origin  // Uses held key
}                             // Key released
```

**On type definitions (permanent key boundary):**

```cursive
record Player {
    health: i32,
    mana: i32,
    position: Vector3,
    #inventory: [Item; 20],   // Key boundary: access always keys at inventory level
    #equipment: Equipment,    // Key boundary: access always keys at equipment level
}
```

Effect:
```cursive
player.health                 // Key: player.health (fine-grained)
player.inventory[0]           // Key: player.inventory (boundary)
player.inventory[5]           // Key: player.inventory (same boundary)
player.equipment.weapon       // Key: player.equipment (boundary)
```

**Restriction:** `#` is permitted only on **direct field access expressions**. Method calls cannot be annotated with `#`:

```cursive
#player.health = 100          // OK: direct field access
#player.restore()             // ERROR E-KEY-0020: # cannot annotate method calls
```

Method authors control key granularity internally via `#self`:

```cursive
impl Player {
    procedure restore_all(~%) {
        #self                 // Coarse key for entire method
        self.health = 100
        self.mana = 100
    }
}
```

---

### 1.9 Concurrent Access

When multiple tasks access `shared` data:

| Task A Key | Task B Key | Same Path? | Behavior |
| :--------- | :--------- | :--------- | :------- |
| Read       | Read       | Yes        | Parallel |
| Read       | Write      | Yes        | B waits  |
| Write      | Read       | Yes        | B waits  |
| Write      | Write      | Yes        | B waits  |
| Any        | Any        | No         | Parallel |

**Key insight:** Disjoint paths never wait for each other.

```cursive
parallel {
    spawn { player.health = 100 }     // Key: health
    spawn { player.position = pos }   // Key: position — parallel
    spawn { player.health = 50 }      // Key: health — waits for first
}
```

---

## Part 2: Structured Concurrency

### 2.1 Separation of Concerns

Cursive separates memory management from task execution:

| Construct  | Responsibility                    |
| :--------- | :-------------------------------- |
| `region`   | Memory arena lifetime             |
| `parallel` | Task grouping + executor          |
| `spawn`    | Create task (requires `parallel`) |

```cursive
region as r {
    let data = r^Data::new()          // Memory allocation in region
    
    parallel pool(4) {                // Execution with 4 workers
        spawn { process(data) }
        spawn { analyze(data) }
    }                                 // Tasks complete here
    
}                                     // Memory freed here
```

---

### 2.2 The `parallel` Block

The `parallel` block is the structured concurrency primitive. It groups tasks and ensures all complete before continuing.

**Syntax:**

```ebnf
parallel_block   ::= "parallel" [executor] [block_options] "{" parallel_body "}"

executor         ::= "pool" "(" expr ")"
                   | "async" "(" expr ")"

block_options    ::= block_option*
block_option     ::= "." "collect_errors" "()"
                   | "." "name" "(" expr ")"

parallel_body    ::= statement*
```

**Executor types:**

| Executor         | Description                        |
| :--------------- | :--------------------------------- |
| (none)           | System default pool                |
| `pool(n)`        | Thread pool with n workers         |
| `async(runtime)` | Async executor for I/O concurrency |

**Default executor:**

Bare `parallel` uses the system default thread pool:

```cursive
parallel {
    spawn { task_a() }
    spawn { task_b() }
}

// Equivalent to:
parallel pool(System.cpu_count()) {
    spawn { task_a() }
    spawn { task_b() }
}
```

---

### 2.3 Spawning Tasks

The `spawn` expression creates a new task within a `parallel` block:

```cursive
parallel {
    spawn { task_body() }
}
```

`spawn` returns a `Task<T>` handle:

```cursive
parallel {
    let task: Task<i32> = spawn { compute_value() }
    let result = task.join()      // Wait and get result
}
```

**Task Handle API:**

```cursive
modal Task<T> {
    @Running { }
    @Completed { value: T }
    @Panicked { error: PanicInfo }
}

impl<T> Task<T> {
    // Block until complete, return result
    procedure join(~!) -> T
    
    // Check completion without blocking
    procedure is_complete(~) -> bool
    
    // Non-blocking attempt to get result
    procedure try_join(~!) -> Option<T>
}
```

**Restrictions:**

- `spawn` is only valid inside a `parallel` block (error `E-SPAWN-0001` otherwise)
- `detach()` is prohibited inside `parallel` blocks—structured concurrency is enforced

---

### 2.4 Implicit Joining

All tasks spawned within a `parallel` block are implicitly joined when the block exits:

```cursive
parallel {
    let task_a = spawn { compute_a() }
    let task_b = spawn { compute_b() }
    
    let result_a = task_a.join()      // Join a early
    use(result_a)
    
    // task_b implicitly joined here
}
```

---

### 2.5 Nesting Parallel Blocks

`parallel` blocks nest naturally. Inner blocks complete before the outer block continues:

```cursive
parallel pool(4) {
    spawn { cpu_work_1() }
    
    parallel async(runtime) {
        spawn { fetch(url_a).await }
        spawn { fetch(url_b).await }
    }
    // Async tasks complete here
    
    spawn { cpu_work_2() }
}
// All tasks complete here
```

This enables mixed workloads without special rules:

```cursive
parallel pool(4) {
    spawn {
        // Fetch data (I/O), then process (CPU)
        let data = parallel async(runtime) {
            spawn { fetch(url).await }
        }
        process(data)
    }
}
```

---

### 2.6 Panic Propagation

**Default behavior:** All tasks run to completion (or their own panic). After all tasks finish, the first panic propagates:

```cursive
parallel {
    spawn { panic("A") }          // Panics first
    spawn { task_b() }            // Completes normally
    spawn { panic("C") }          // Also panics
}
// All tasks done, then panic("A") propagates
```

**Opt-in error collection:**

```cursive
parallel .collect_errors() {
    spawn { may_panic_a() }
    spawn { may_panic_b() }
}
// Returns Result with all collected panics
```

---

## Part 3: Capture Semantics

### 3.1 Capture Rules

Tasks capture bindings from their environment according to their permission:

| Source Permission | Capture Mode | In Task  |
| :---------------- | :----------- | :------- |
| `const`           | Reference    | `const`  |
| `shared`          | Reference    | `shared` |
| `unique`          | Must `move`  | `unique` |

---

### 3.2 Const Capture

`const` data is freely capturable—it cannot change, so sharing is safe:

```cursive
let config: const Config = ...

parallel {
    spawn { use(config.timeout) }     // OK: const, no key needed
    spawn { use(config.retries) }     // OK: const, no key needed
}
```

---

### 3.3 Shared Capture

`shared` data is capturable within structured concurrency:

```cursive
let player: shared Player = ...

parallel {
    spawn { player.health = 100 }     // OK: shared, key acquired on access
    spawn { player.mana = 50 }        // OK: different path, parallel
}
```

---

### 3.4 Unique Capture

`unique` data cannot be captured by reference—it must be moved:

```cursive
let buffer: unique Buffer = ...

parallel {
    spawn { buffer.write(data) }      // ERROR E-KEY-0001: cannot capture unique
}

parallel {
    spawn { 
        move buffer                   // Ownership moves into task
        buffer.write(data)            // OK: buffer is unique within task
    }
}
// buffer no longer accessible here
```

---

### 3.5 Implicit Downgrade Capture

When `unique` data is captured as `shared` (permission downgrade), this is only permitted within structured concurrency:

```cursive
let player: unique Player = Player::new()

parallel {
    // player implicitly downgraded to shared for these captures
    spawn { player.health = 100 }
    spawn { player.mana = 50 }
}
// parallel block complete, player accessible as unique again

player.health = 200   // OK: unique access restored
```

The original `unique` binding is **inactive** for the duration of the `parallel` block.

---

### 3.6 Unstructured Concurrency Escape Hatch

For detached tasks or unstructured concurrency patterns, use explicit `make_shared`:

```cursive
let player: unique Player = Player::new()

// Convert to explicitly shared (consumes unique)
let shared_player: shared Player = make_shared(move player)

// Now can use outside structured concurrency
let handle = shared_player.spawn_detached { 
    shared_player.health = 100 
}
handle.detach()       // OK: explicitly unstructured

// player no longer exists; shared_player is the only reference
```

---

## Part 4: Async

### 4.1 Futures

A **Future** represents a value that may not yet exist:

```cursive
trait Future {
    type Output
    procedure poll(~!, cx: unique WakerContext) -> Poll<Self::Output>
}

enum Poll<T> {
    Ready { value: T },
    Pending,
}
```

---

### 4.2 Async Procedures

An `async` procedure returns a `Future` instead of executing immediately:

```cursive
async procedure fetch(url: Url) -> Response {
    let conn = connect(url).await
    conn.read_response().await
}

// Calling creates a future
let future = fetch(url)

// Awaiting drives it to completion
let response = future.await
```

---

### 4.3 The `.await` Operator

`.await` suspends the current task until a future completes:

```cursive
let response = http_get(url).await
```

**Critical property:** Keys are never held across `.await`.

Each access is self-contained:

```cursive
let player: shared Player = ...

parallel async(runtime) {
    spawn {
        player.health += 1            // Acquire key, modify, release
        sleep(1.second).await         // Suspended — no keys held
        player.health += 1            // Acquire key, modify, release
    }
}
```

**TOCTOU consideration:**

```cursive
let hp = player.health                // Read, release
network_call().await                  // Suspended
player.health = hp + 10               // hp may be stale!
```

This is explicit and visible. For atomic updates, don't split across await:

```cursive
player.health += 10                   // Atomic: write key for entire expression
```

---

### 4.4 Async with Parallel

Async tasks use `parallel async(runtime)`:

```cursive
parallel async(runtime) {
    spawn { fetch(url_a).await }
    spawn { fetch(url_b).await }
}
```

Concurrent async operations:

```cursive
parallel async(runtime) {
    let task_a = spawn { fetch(url_a).await }
    let task_b = spawn { fetch(url_b).await }
    
    let a = task_a.join()
    let b = task_b.join()
    process(a, b)
}
```

---

### 4.5 Select

Wait on multiple futures, proceed with first to complete:

```cursive
select {
    response = fetch(url).await => handle(response),
    _ = timeout(5.seconds).await => handle_timeout(),
}
```

**Biased (check in order):**

```cursive
select biased {
    urgent = urgent_chan.recv().await => process_urgent(urgent),
    normal = normal_chan.recv().await => process_normal(normal),
}
```

---

## Part 5: Data Parallelism

### 5.1 The `dispatch` Primitive

`dispatch` expresses data-parallel operations over a range:

```cursive
let data: shared [i32; 1000] = ...

dispatch i in 0..data.len() {
    data[i] = data[i] * 2
}
```

The runtime:
1. Partitions the iteration space
2. Assigns partitions to available workers
3. Executes in parallel
4. Joins when complete

---

### 5.2 Automatic Key Elision

`dispatch` automatically proves disjointness for iteration-indexed access and elides keys:

```cursive
dispatch i in 0..data.len() {
    data[i] *= 2              // No key: proven disjoint by dispatch semantics
}
```

**Cross-iteration dependency detection:**

```cursive
dispatch i in 0..data.len() {
    data[i] = data[i + 1]     // ERROR E-DISPATCH-0001: cross-iteration dependency
}
```

---

### 5.3 Dispatch Variants

**With explicit pool:**

```cursive
dispatch on pool(8) i in 0..n {
    process(data[i])
}
```

**Reduction:**

```cursive
let sum = dispatch i in 0..data.len()
    reduce(0, |acc, x| acc + x)
{
    data[i]
}
```

**Map-collect:**

```cursive
let doubled: [i32] = dispatch i in data collect {
    data[i] * 2
}
```

---

### 5.4 Nested Dispatch

```cursive
dispatch i in 0..rows {
    dispatch j in 0..cols {
        matrix[i][j] = compute(i, j)
    }
}
```

The runtime flattens or nests based on workload characteristics.

---

## Part 6: Channels

### 6.1 Purpose

Channels provide **ownership transfer** between tasks—a different paradigm from shared access:

- **Keys:** Multiple tasks access shared data with synchronized access
- **Channels:** Data moves from producer to consumer with no sharing

```cursive
let (tx, rx) = channel::<Data>()

parallel {
    spawn {
        let data: unique Data = produce()
        tx.send(move data)            // Ownership into channel
    }
    
    spawn {
        let data: unique Data = rx.recv()   // Ownership out
        consume(data)
    }
}
```

The data is `unique` throughout—never shared, never needs keys.

---

### 6.2 Channel Types

**Unbounded:**

```cursive
let (tx, rx) = channel::<T>()
// Grows as needed, send never blocks
```

**Bounded:**

```cursive
let (tx, rx) = channel::<T>(capacity)
// send blocks when full
```

**Oneshot:**

```cursive
let (tx, rx) = oneshot::<T>()
// Single value, single send
```

**Broadcast:**

```cursive
let tx = broadcast::<T>(capacity)
let rx1 = tx.subscribe()
let rx2 = tx.subscribe()
// All receivers get each message
```

---

### 6.3 Channel Operations

**Synchronous:**

```cursive
tx.send(move value)           // Block until sent (bounded) or enqueue (unbounded)
let val = rx.recv()           // Block until received

tx.try_send(move value)       // Non-blocking: Ok(()) or Err(Full)
let opt = rx.try_recv()       // Non-blocking: Some(val) or None
```

**Async:**

```cursive
tx.send(move value).await     // Suspend until sent
let val = rx.recv().await     // Suspend until received
```

**Closing:**

```cursive
tx.close()                    // Signal no more values
// rx.recv() returns None after channel drained
```

---

### 6.4 Channel Safety

No additional type bounds are required on channel element types. The structured concurrency model guarantees:

- All tasks complete before their region exits
- Region-allocated data remains valid for all tasks in that region
- Data sent through channels is always valid when received

---

## Part 7: Waiting

### 7.1 The `wait` Expression

`wait` efficiently blocks until a condition becomes true, then executes an action atomically:

```cursive
wait <condition> then {
    <body>
}
```

**Semantics:**

1. Acquire keys to paths in condition
2. Evaluate condition
3. If true: execute body with keys held, release keys, done
4. If false:
   - Register as waiter for these keys
   - Release keys
   - Sleep until any registered key is released by another task
   - Goto 1

**Example:**

```cursive
let queue: shared Queue<T> = ...

// Consumer waits for non-empty, then pops atomically
wait !queue.items.is_empty() then {
    let item = queue.items.pop()      // Guaranteed non-empty
    process(item)
}
```

---

### 7.2 Wake Trigger

Waiters are triggered by **key release**, not modification notification:

- When a task releases a key, all tasks waiting on that key are woken
- Woken tasks re-acquire keys and re-evaluate their condition
- If condition is still false, they return to waiting

This integrates naturally with the key system—no separate notification mechanism.

---

### 7.3 Plain `wait`

`wait <condition>` without a body is sugar for `wait <condition> then { }`:

```cursive
wait shutdown_requested       // Wait for flag, then continue

// Equivalent to:
wait shutdown_requested then { }
```

Use plain `wait` for simple notifications where no atomic action is needed.

---

### 7.4 Wait with Timeout

```cursive
if wait(timeout: 5.seconds) !queue.is_empty() then {
    process(queue.pop())
} else {
    handle_timeout()
}
```

---

## Part 8: Memory Ordering

### 8.1 Default: Sequential Consistency

Key acquisition uses **acquire** semantics. Key release uses **release** semantics. Memory accesses default to **sequentially consistent** ordering.

This provides the strongest guarantees—all tasks observe operations in a consistent global order.

For most code, this is correct and sufficient.

---

### 8.2 Relaxed Ordering

For performance-critical code, weaker orderings are available via annotation:

```cursive
#[relaxed]
counter += 1

#[acquire]
let ready = flag

#[release]
flag = true

#[acq_rel]
counter += 1
```

**Ordering levels:**

| Ordering  | Guarantee                             |
| :-------- | :------------------------------------ |
| `relaxed` | Atomicity only—no ordering            |
| `acquire` | Subsequent reads see prior writes     |
| `release` | Prior writes visible to acquire reads |
| `acq_rel` | Both acquire and release              |
| `seq_cst` | Total global order (default)          |

The annotation affects the memory operation, not the key synchronization. Keys always use acquire/release semantics.

---

### 8.3 Fence Operations

For explicit memory barriers:

```cursive
fence(acquire)       // Acquire fence
fence(release)       // Release fence
fence(seq_cst)       // Full barrier
```

---

## Part 9: Regions and Memory

### 9.1 Regions

Regions provide **memory arena lifetime**—data allocated in a region lives until the region exits:

```cursive
region as r {
    let data = r^[0u8; 1024]      // Allocate in region
    process(data)
}                                 // All region memory freed
```

Regions are **orthogonal to concurrency**—they manage memory lifetime, not task execution.

---

### 9.2 Regions with Concurrency

Region-allocated data can be shared across tasks because structured concurrency guarantees tasks complete before the region exits:

```cursive
region as r {
    let buffer: shared Buffer = r^Buffer::new()
    
    parallel {
        spawn { write_to(buffer) }
        spawn { read_from(buffer) }
    }
    // Tasks complete here
}   
// Region freed here—safe because tasks already completed
```

---

### 9.3 Parallel Regions

Combine arena allocation with structured concurrency:

```cursive
region as r {
    parallel pool(4) {
        let buffer: shared Buffer = r^Buffer::new()
        
        spawn { writer(buffer) }
        spawn { reader(buffer) }
    }
}
// Tasks complete, then region freed
```

---

## Part 10: Low-Level Primitives

### 10.1 Volatile Access

For memory-mapped I/O:

```cursive
let reg: Volatile<u32> = unsafe { Volatile::from_addr(0x4000_0000) }

reg.read()
reg.write(0x01)
```

Volatile operations:
- Not reordered by compiler
- Not elided
- Bypass the key system (hardware access, not shared memory)

---

### 10.2 Interrupt Context

For interrupt service routines:

```cursive
#[interrupt_safe]
procedure timer_isr(counter: shared i64) {
    #[relaxed]
    counter += 1
}
```

Interrupt context restrictions:
- No blocking (`wait`, channel recv)
- No allocation
- Only small atomic operations on `shared` data

---

## Part 11: Implementation Model

### 11.1 Zero-Cost Abstractions

Keys are a compile-time concept. Runtime cost occurs only when necessary:

| Condition                              | Cost                          |
| :------------------------------------- | :---------------------------- |
| No concurrent access possible          | Zero—elided                   |
| Single-threaded context                | Zero—elided                   |
| Provably disjoint access               | Zero—parallel without sync    |
| `const` permission                     | Zero—immutable                |
| `unique` permission                    | Zero—exclusive                |
| `dispatch` iteration-indexed access    | Zero—proven disjoint          |
| Small primitive + potential contention | Atomic operation              |
| Compound type + potential contention   | Reader-writer synchronization |

---

### 11.2 Key Implementation Strategy

When runtime synchronization is needed:

| Data Type                   | Implementation               |
| :-------------------------- | :--------------------------- |
| Small primitive (≤ 8 bytes) | Atomic CAS / fetch-and-op    |
| Struct field                | Per-field or single RwLock   |
| Array (static index)        | Per-element or segment locks |
| Array (dynamic index)       | Lock keyed by address        |
| `#`-annotated boundary      | Single lock for subtree      |

**Key state storage is implementation-defined.** Implementations document their strategy in the Conformance Dossier.

---

### 11.3 Compiler Transformations

**Source:**
```cursive
let player: shared Player = get_player()
player.health = 100
```

**Intermediate representation:**
```cursive
let player: SharedHandle<Player> = get_player()

// Acquire write key
let key: Key<i32> = acquire_write_key(player, path(".health"))

// Access through key
write(key.ptr, 100)

// Release key
release_key(key)
```

**Optimized (when elision applies):**
```cursive
// Direct memory access, no key operations
write(player.health_ptr, 100)
```

---

### 11.4 Key Elision Analysis

The compiler uses several analyses to prove key elision is safe:

**Escape Analysis:**
```cursive
procedure local_only() {
    let data: shared Data = Data { value: 0 }
    data.value = 100
}
// data never escapes—elide all key operations
```

**Sequential Region Analysis:**
```cursive
region as r {
    // No parallel block in this region
    let data: shared Data = r^Data::new()
    data.value = 100
}
// No concurrency possible—elide all key operations
```

**Disjoint Path Analysis:**
```cursive
parallel {
    spawn { player.health = 100 }
    spawn { player.mana = 50 }
}
// Paths are disjoint—elide key operations for both
```

---

## Part 12: Diagnostics

### 12.1 Diagnostic Codes

| Code              | Severity | Condition                                                          |
| :---------------- | :------- | :----------------------------------------------------------------- |
| `E-KEY-0001`      | Error    | Access to shared path without key context                          |
| `E-KEY-0010`      | Error    | Potential key conflict on dynamic indices                          |
| `E-KEY-0020`      | Error    | `#` annotation on method call                                      |
| `E-KEY-0030`      | Error    | Redundant `#` (already at boundary)                                |
| `E-SPAWN-0001`    | Error    | `spawn` outside `parallel` block                                   |
| `E-SPAWN-0002`    | Error    | Captured reference outlives task                                   |
| `E-SPAWN-0003`    | Error    | Cannot capture downgraded reference outside structured concurrency |
| `E-SPAWN-0004`    | Error    | `detach` prohibited inside `parallel` block                        |
| `E-ASYNC-0001`    | Error    | `.await` outside async context                                     |
| `E-ASYNC-0002`    | Error    | Blocking operation in async context                                |
| `E-DISPATCH-0001` | Error    | Cross-iteration dependency in dispatch                             |
| `E-WAIT-0001`     | Error    | `wait` on non-shared data                                          |
| `W-KEY-0001`      | Warning  | Fine-grained keys in tight loop                                    |
| `W-KEY-0002`      | Warning  | Redundant key acquisition (covered by held key)                    |

---

## Part 13: Summary

### 13.1 Concurrency Primitives

| Concept                  | Mechanism                             |
| :----------------------- | :------------------------------------ |
| Shared mutable state     | `shared` permission + implicit keys   |
| Fine-grained parallelism | Keys per path, disjoint = parallel    |
| Coarse-grained locking   | `#` annotation                        |
| Reader-writer semantics  | Read vs write keys (automatic)        |
| Atomic operations        | Small `shared` primitives (automatic) |
| Ownership transfer       | Channels                              |
| Blocking on conditions   | `wait` expression                     |
| Task parallelism         | `parallel` + `spawn`                  |
| Data parallelism         | `dispatch`                            |
| Async I/O                | `parallel async` + `.await`           |
| Memory ordering          | Annotations (default: seq_cst)        |

### 13.2 The Mental Model

- **`unique`**: I own it exclusively—direct access, no synchronization
- **`shared`**: Others may access—key required for any access
- **`const`**: Cannot change—direct read, no synchronization

Keys unlock. Access acquires keys. Disjoint paths parallelize.

### 13.3 Structured Concurrency

- `region` provides memory lifetime
- `parallel` provides task grouping and execution
- `spawn` creates tasks within `parallel`
- All tasks complete before `parallel` exits
- Nesting composes naturally

---

## Appendix A: Formal Rules

### A.1 Key Acquisition Rules

**K-Read:** Reading `shared` path P acquires read key to P
```
read(P) → acquire(P, Read)
```

**K-Write:** Writing `shared` path P acquires write key to P
```
write(P) → acquire(P, Write)
```

**K-ReadModifyWrite:** Read + write same path acquires write key for statement
```
read(P) ∧ write(P) in S → acquire(P, Write, scope=S)
```

**K-Coarsen:** `#` moves key point up the path
```
#prefix.rest → acquire(prefix) instead of acquire(prefix.rest)
```

**K-Block:** Standalone `#path` acquires for enclosing block
```
{ #P; ... } → acquire(P, scope=block)
```

**K-Reentry:** If key covering P is held, access to P succeeds without acquisition
```
held(Q) ∧ covers(Q, P) → access(P) succeeds
```

---

### A.2 Key Compatibility

Two keys are compatible (can be held simultaneously by different tasks):

```
Compatible(K1, K2) ⟺ 
    Disjoint(K1.path, K2.path) ∨
    (K1.mode = Read ∧ K2.mode = Read)
```

---

### A.3 Key Ordering

Keys acquired in canonical order within a single expression:

```
acquire_all(paths) = for P in sort(paths, canonical): acquire(P)
```

Canonical order: lexicographic by path components, depth-first.

---

### A.4 Wait Semantics

```
wait C then { B }:
    loop:
        keys = acquire_keys(paths(C))
        if eval(C):
            eval(B)
            release(keys)
            break
        else:
            register_waiter(keys)
            release(keys)
            sleep_until_key_released()
```

---

## Appendix B: Complete Example

### B.1 Game Entity System

```cursive
record Entity {
    position: Vector3,
    velocity: Vector3,
    health: i32,
    #components: HashMap<TypeId, Component>,   // Key boundary
}

record World {
    entities: [Entity; MAX_ENTITIES],
    time: f64,
}

procedure update(world: shared World, dt: f32) {
    parallel pool(System.cpu_count()) {
        spawn { update_physics(world, dt) }
        spawn { update_ai(world, dt) }
    }
}

procedure update_physics(world: shared World, dt: f32) {
    dispatch i in 0..world.entities.len() {
        // Keys elided: dispatch proves disjoint access
        let e = world.entities[i]
        e.position += e.velocity * dt
    }
}

procedure update_ai(world: shared World, dt: f32) {
    dispatch i in 0..world.entities.len() {
        // Key: entities[i].components (boundary)
        let e = world.entities[i]
        if let Some(ai) = e.components.get(AI_TYPE) {
            ai.think(e, world, dt)
        }
    }
}
```

Physics and AI run in parallel. Within each, entities are processed in parallel via `dispatch`.

---

### B.2 Async HTTP Server

```cursive
async procedure serve(listener: TcpListener, state: shared AppState) {
    parallel async(runtime) {
        loop {
            let conn = listener.accept().await
            
            spawn {
                handle_connection(conn, state).await
            }
        }
    }
}

async procedure handle_connection(conn: unique TcpStream, state: shared AppState) {
    let request = parse_request(conn).await
    
    let response = match (request.method, request.path) {
        (GET, "/data") => {
            let data = state.data            // Read key acquired
            Response::json(data)
        }
        (PUT, "/data") => {
            state.data = request.body.parse()  // Write key acquired
            Response::ok()
        }
        _ => Response::not_found()
    }
    
    conn.write(response.serialize()).await
}
```

---

### B.3 Producer-Consumer with Wait

```cursive
record Queue<T> {
    items: Vec<T>,
    closed: bool,
}

procedure producer(queue: shared Queue<Data>, items: const [Data]) {
    for item in items {
        wait queue.items.len() < MAX_CAPACITY then {
            queue.items.push(item.clone())
        }
    }
    queue.closed = true
}

procedure consumer(queue: shared Queue<Data>) {
    loop {
        wait !queue.items.is_empty() || queue.closed then {
            if queue.items.is_empty() && queue.closed {
                break
            }
            let item = queue.items.pop()
            process(item)
        }
    }
}

procedure run_pipeline(items: const [Data]) {
    let queue: shared Queue<Data> = Queue { items: Vec::new(), closed: false }
    
    parallel {
        spawn { producer(queue, items) }
        spawn { consumer(queue) }
        spawn { consumer(queue) }
    }
}
```