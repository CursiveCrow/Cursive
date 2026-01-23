# Cursive Language Specification

## Clause 13 — Concurrency and Memory Ordering

**Part**: XIII — Concurrency and Memory Ordering  
**File**: 14-01_Concurrency-Model.md  
**Section**: §13.1 Concurrency Model  
**Stable label**: [concurrency.model]  
**Forward references**: §13.2 [concurrency.memory], §13.3 [concurrency.atomic], §13.4 [concurrency.synchronization], Clause 9 §9.4 [memory.permission], Clause 10 [contract]

---

### §13.1 Concurrency Model [concurrency.model]

#### §13.1.1 Overview

[1] Cursive provides structured concurrency through thread spawning, synchronization primitives, and permission-based thread safety. The concurrency model integrates with the permission system (§9.4), modal types (§5.6), and grant tracking (Clause 10) to ensure data-race freedom without requiring separate thread-safety markers.

[2] **Design principles**:

- **Permission-based thread safety**: The existing permission system (`const`/`unique`/`shared`) encodes thread safety without additional markers
- **Modal types for lifecycle**: Threads, mutexes, channels, and other concurrent primitives are modal types tracking their lifecycle states
- **Explicit grants**: Concurrent operations require explicit capability grants (`thread::spawn`, `sync::lock`)
- **Data-race freedom**: Well-typed programs without unsafe code cannot produce data races

#### §13.1.2 Thread Safety Through Permissions

[4] Thread safety is encoded through the permission system (§10.4). The permission lattice directly determines thread safety properties as shown in Table 13.1.

**Table 13.1 — Permission-based thread safety**

| Permission | Thread Safety Meaning | Can Cross Thread Boundary | Synchronization Required |
|------------|----------------------|---------------------------|-------------------------|
| `const` | Immutable, unlimited aliasing | YES (safe to share) | NO (reads are safe) |
| `unique` | Exclusive ownership | YES (via transfer) | NO (exclusive access) |
| `shared` | Coordinated mutable access | YES (with sync primitive) | YES (Mutex, RwLock, etc.) |

[5] Complete permission semantics and enforcement rules are specified in §10.4 [memory.permission].

#### §13.1.3 Thread as Modal Type

[8] Threads are represented by the `Thread` built-in modal type:

```cursive
modal Thread<T> {
    @Spawned {
        id: ThreadId,
    }
    
    @Spawned::join(~!) -> @Joined
    @Spawned::detach(~!) -> @Detached
    
    @Joined {
        result: T,
    }
    
    @Detached { }
}
```

[9] **State semantics**:

- **@Spawned**: Thread is running. Supports join (wait for completion) or detach (run independently).
- **@Joined**: Thread has completed; result is available. Terminal state.
- **@Detached**: Thread runs independently; no result retrieval possible. Terminal state.

[10] **State transition diagram**:

```
@Spawned ──join()──> @Joined (result available)
   │
   │ detach()
   ↓
@Detached (independent execution)
```

#### §13.1.4 Thread Spawning

[11] Threads are spawned via the `spawn` procedure:

```cursive
procedure spawn<T, ε>(action: () -> T ! ε): Thread<T>@Spawned
    [[ thread::spawn, ε |- true => true ]]
    where T: const
{
    // Implementation creates OS thread executing action
}
```

[12] **Type constraint**: The return type `T` shall satisfy `T: const` (immutable permission). This ensures thread-safe return values without requiring separate Send markers.

[13] **Grant propagation**: The spawned closure's grant set `ε` is propagated to the spawning procedure's grant clause. The spawning procedure must have all grants required by the closure.

[14] **Closure constraints**: The spawned closure may capture:
- `const` bindings (safe to share across threads)
- Moved `unique` bindings (exclusive ownership transferred)
- No `shared` bindings without synchronization primitives

**Example 13.1.4.1 (Thread spawning with const data)**:

```cursive
procedure parallel_compute(config: const Config): (i32, i32)
    [[ thread::spawn, thread::join |- true => true ]]
{
    let thread1 = spawn(|| [[ |- true => true ]] {
        compute_first(config)  // OK: config is const (shareable)
    })
    
    let thread2 = spawn(|| [[ |- true => true ]] {
        compute_second(config)  // OK: multiple threads can read const
    })
    
    let result1 = thread1.join().result
    let result2 = thread2.join().result
    
    result (result1, result2)
}
```

**Example 13.1.4.2 (Thread spawning with unique transfer)**:

```cursive
procedure async_process(data: unique Buffer): i32
    [[ thread::spawn, thread::join, io::write |- true => true ]]
{
    let thread = spawn(move || [[ io::write |- true => true ]] {
        process_buffer(data)  // OK: unique ownership transferred
    })
    
    // data no longer accessible here (moved to thread)
    
    result thread.join().result
}
```

**Example 13.1.4.3 - invalid (Shared without synchronization)**:

```cursive
procedure invalid_parallel(data: shared Buffer)
    [[ thread::spawn |- true => true ]]
{
    let thread = spawn(|| {
        data.modify()  // error[E13-101]: shared data requires synchronization
    })
}
```

#### §13.1.5 Thread Joining and Detaching

##### §13.1.5.1 Join Operation

[15] The `join` method waits for thread completion and retrieves the result:

```cursive
procedure Thread.join<T>(self: unique Self@Spawned): Self@Joined
    [[ thread::join |- true => true ]]
{
    // Block until thread completes
    // Transition to @Joined with result available
}
```

[16] **Happens-before**: The join operation establishes a happens-before relationship (§13.2): all operations in the joined thread happen-before operations after the join in the joining thread.

[17] **Type rule**:

$$
\frac{\Gamma \vdash thread : \text{Thread}\langle T \rangle@\text{Spawned}}
     {\Gamma \vdash thread\texttt{.join()} : \text{Thread}\langle T \rangle@\text{Joined}}
\tag{T-Thread-Join}
$$

##### §13.1.5.2 Detach Operation

[18] The `detach` method allows a thread to run independently:

```cursive
procedure Thread.detach<T>(self: unique Self@Spawned): Self@Detached
    [[ thread::detach |- true => true ]]
{
    // Thread continues independently
    // Result is discarded
}
```

[19] **No result retrieval**: Detached threads cannot be rejoined. The result value is lost. Detach is appropriate for fire-and-forget tasks.

**Example 13.1.5.1 (Join and detach)**:

```cursive
procedure example()
    [[ thread::spawn, thread::join |- true => true ]]
{
    // Join pattern:
    let worker = spawn(|| [[ io::write ]] { compute_result() })
    let result = worker.join().result
    process(result)
    
    // Detach pattern:
    let logger = spawn(|| [[ io::write ]] { background_logging() })
    logger.detach()  // Runs independently, no result needed
}
```

#### §14.1.6 Thread-Local Storage

[20] Thread-local storage is managed through per-thread arena allocation (§10.3):

```cursive
procedure worker_thread(thread_id: usize)
    [[ alloc::region, io::write |- true => true ]]
{
    region thread_local as workspace {
        loop work_item in fetch_work(thread_id) {
            let data = workspace.alloc(Data::new(work_item))
            process(data)
        }
    }
    // Thread-local arena freed when thread exits
}
```

[21] Each thread has independent region stack. Arena allocations in one thread do not affect other threads.

#### §14.1.7 Constraints

[1] _Thread return type constraint._ The return type `T` of a spawned closure shall satisfy `T: const`. Non-const return types produce diagnostic E13-100.

[2] _Closure capture constraints._ Closures passed to `spawn` may capture:
- `const` bindings (immutable sharing)
- Moved `unique` bindings (ownership transfer via `move` keyword)

Capturing `shared` bindings without wrapping in synchronization primitives produces diagnostic E13-101.

[3] _Grant requirements._ Spawning threads requires the `thread::spawn` grant. Joining threads requires the `thread::join` grant. Detaching threads requires the `thread::detach` grant.

[4] _Thread lifecycle._ Threads shall either be joined or detached before the `Thread` value goes out of scope. Dropping a `Thread@Spawned` without join or detach produces diagnostic E13-102.

#### §14.1.8 Semantics

##### §14.1.8.1 Thread Creation

[5] The `spawn` procedure creates a new OS thread executing the provided closure:

[ Given: Closure `action : () -> T ! ε` ]

$$
\frac{\Gamma \vdash action : () \to T \; ! \varepsilon \quad T : \text{const}}
     {\Gamma \vdash \texttt{spawn}(action) : \text{Thread}\langle T \rangle@\text{Spawned}}
\tag{T-Spawn}
$$

[6] The spawned thread executes concurrently with the spawning thread. Execution order between threads is unspecified; synchronization is required for deterministic ordering.

##### §14.1.8.2 Thread Completion

[7] When a thread's closure returns, the thread transitions internally from running to completed state. The `join` method retrieves this completion:

$$
\frac{\langle thread, \sigma \rangle \Downarrow_{\text{join}} \langle \text{completed}, \sigma' \rangle}
     {\langle thread\texttt{.join()}, \sigma \rangle \Downarrow \langle \text{Thread}@\text{Joined}\{\text{result}: v\}, \sigma' \rangle}
\tag{E-Thread-Join}
$$

where $v$ is the value returned by the thread's closure.

##### §14.1.8.3 Grant Accumulation

[8] Grant sets accumulate across thread boundaries. The spawning procedure's grant clause shall include:
- `thread::spawn` (for the spawn operation)
- All grants `ε` required by the spawned closure

$$
\frac{\text{closure requires grants } \varepsilon}
     {\text{spawning procedure requires grants } \{\texttt{thread::spawn}\} \cup \varepsilon}
\tag{Grant-Thread-Accumulate}
$$

#### §13.1.9 Diagnostics

[9] [Note: Diagnostics defined in this subsection are cataloged in Annex E §E.5.1.13. — end note]

#### §14.1.10 Examples

**Example 14.1.10.1 (Parallel computation with const sharing)**:

```cursive
procedure parallel_sum(numbers: const [i32]): i32
    [[ thread::spawn, thread::join |- numbers.len() >= 2 => true ]]
{
    let mid = numbers.len() / 2
    
    let left_thread = spawn(|| [[ |- true => true ]] {
        sum_range(numbers, 0, mid)
    })
    
    let right_thread = spawn(|| [[ |- true => true ]] {
        sum_range(numbers, mid, numbers.len())
    })
    
    let left_sum = left_thread.join().result
    let right_sum = right_thread.join().result
    
    result left_sum + right_sum
}

procedure sum_range(numbers: const [i32], start: usize, end: usize): i32
    [[ |- start <= end && end <= numbers.len() => true ]]
{
    var total: i32 = 0
    loop i in start..end {
        total = total + numbers[i]
    }
    result total
}
```

**Example 14.1.10.2 (Background task with detach)**:

```cursive
procedure start_background_logger()
    [[ thread::spawn, thread::detach, io::write |- true => true ]]
{
    let logger = spawn(|| [[ io::write |- true => true ]] {
        loop {
            sleep(1000)
            log_status()
        }
    })
    
    logger.detach()  // Runs independently
    // Returns immediately; logging continues in background
}
```

**Example 14.1.10.3 (Transfer unique ownership to thread)**:

```cursive
procedure process_in_background(data: unique Buffer)
    [[ thread::spawn, thread::detach |- true => true ]]
{
    let processor = spawn(move || [[ alloc::heap |- true => true ]] {
        expensive_computation(data)
    })
    
    // data moved to thread (no longer accessible here)
    
    processor.detach()
}
```

**Example 14.1.10.4 - invalid (Non-const return type)**:

```cursive
procedure invalid_spawn()
    [[ thread::spawn |- true => true ]]
{
    let buffer: unique Buffer = Buffer::new()
    
    let thread = spawn(move || {
        result buffer  // error[E13-100]: return type 'unique Buffer' not const
    })
}
```

#### §14.1.11 Integration with Permission System

[10] The permission system (§11.4) provides thread safety guarantees:

**const types are thread-safe**:
[11] Types with `const` permission are inherently thread-safe because they are immutable. The compiler guarantees no mutable access exists, preventing data races.

$$
\frac{v : \text{const } T}
     {\text{Sharing } v \text{ across threads is safe}}
\tag{P-Const-ThreadSafe}
$$

**unique types are transferable**:
[12] Types with `unique` permission can be transferred between threads via `move`. The source thread loses access; the destination thread gains exclusive ownership. No aliasing exists, preventing data races.

$$
\frac{v : \text{unique } T \quad \text{move } v \text{ to thread}}
     {\text{Transfer is safe (exclusive ownership)}}
\tag{P-Unique-Transfer}
$$

**shared types require synchronization**:
[13] Types with `shared` permission require explicit synchronization when accessed from multiple threads. The programmer must wrap `shared` data in Mutex, RwLock, or other synchronization primitives (§14.4).

#### §13.1.12 No Send/Sync Markers

[14] Thread safety uses permissions, not markers (§1.8.3).

#### §13.1.13 Conformance Requirements

[16] Implementations shall establish happens-before relationships at spawn and join points (§13.2) and use permissions (const/unique/shared) for all thread safety checking without providing separate Send/Sync marker behaviors.

---

**Previous**: Clause 12 — Witness System (§12.8 [witness.diagnostics]) | **Next**: §13.2 Memory Model (§13.2 [concurrency.memory])

