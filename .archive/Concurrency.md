## Clause 12: Concurrency

This clause defines the Cursive concurrency model. It specifies **Execution Regions**—a unified abstraction combining memory lifetime with task lifetime—and the constructs for creating, synchronizing, and coordinating shared tasks.

---

### 13.1 Concurrency Model Foundations

##### Definition

The **Cursive Concurrency Model** is a structured concurrency system in which all shared execution occurs within lexically-scoped **Execution Regions**. The model enforces three invariants:

1. **Structured Lifetime**: All tasks spawned within a region MUST complete before the region exits.
2. **Permission-Based Safety**: Data race freedom is guaranteed by the Permission System (§4.5).
3. **Explicit Authority**: shared execution requires explicit capabilities; there is no ambient concurrency.

**Formal Definition**

Let $\mathcal{R}$ denote the set of **Region Kinds**:

$$\mathcal{R} = \{\texttt{Sequential}, \texttt{Pool}, \texttt{Async}, \texttt{GPU}, \texttt{Interrupt}\}$$

A **Region Instance** is a tuple $(k, \alpha, \tau)$ where:
- $k \in \mathcal{R}$ is the region kind
- $\alpha$ is the region's **allocation arena** (per §3.4)
- $\tau$ is the set of **active tasks** within the region

The **Structured Concurrency Invariant** states that for any region instance $(k, \alpha, \tau)$:

$$\text{RegionExit}(k, \alpha, \tau) \implies \tau = \emptyset$$

That is, a region cannot exit while tasks remain active.

**Data Race Freedom**

A **data race** occurs when two shared operations access the same memory location, at least one is a write, and there is no synchronization ordering between them.

**Theorem (Data Race Freedom)**: A well-formed Cursive program that does not use `unsafe` blocks cannot exhibit data races.

*Proof sketch*: The Permission System (§4.5) ensures that:
1. `unique` permission grants exclusive access; no shared aliases exist
2. `shared` permission requires synchronized mutation through `~%` methods
3. `const` permission forbids mutation entirely

Since all mutable access requires either exclusivity (`unique`) or synchronization (`shared` with `~%` methods), and the type system statically enforces these requirements, no unsynchronized shared writes can occur. ∎

**Thread Safety via Permissions**

Thread safety in Cursive is a consequence of the permission system rather than a separate marker trait mechanism:

| Permission | Share Across Tasks | Transfer to Task | Rationale                                        |
| :--------- | :----------------- | :--------------- | :----------------------------------------------- |
| `const`    | Safe               | Safe             | Immutable data has no write conflicts            |
| `unique`   | Forbidden          | Safe (via move)  | Exclusive ownership transfers atomically         |
| `shared`   | Safe               | Safe             | Can only be accessed through `~%` or `~` methods |

A type's thread-safety characteristics are determined by:
1. **Which permissions the type supports** (what receivers its methods accept)
2. **Whether `~%` methods provide actual synchronization** (author responsibility)

Types that provide `~%` methods declare their intent to support shared access. Types without `~%` methods are effectively read-only when accessed through `shared` paths (per §4.5.4), which is inherently safe.

##### Syntax & Declaration

Region blocks are the sole mechanism for introducing concurrency:

**Grammar**

```ebnf
region_block      ::= "region" region_kind region_config* "as" IDENTIFIER block

region_kind       ::= (* empty - sequential default *)
                    | "pool" "(" expression ")"
                    | "async" "(" expression ")"
                    | "gpu" "(" expression ")"
                    | "interrupt"

region_config     ::= "." IDENTIFIER "(" expression ")"
```

##### Static Semantics

**Region Context Extension**

Entering a region extends the type context $\Gamma$ with a region binding:

$$\frac{\Gamma \vdash k : \text{RegionKind} \quad \Gamma, r : \text{Region}(k) \vdash_{(k,r)} e : T}{\Gamma \vdash \texttt{region}\ k\ \texttt{as}\ r\ \{ e \} : T} \quad \text{(T-Region)}$$

**Parallel Region Predicate**

Define the predicate $\text{IsParallel}(k)$ over region kinds:

$$\text{IsParallel}(k) \iff k \in \{\texttt{Pool}, \texttt{Async}, \texttt{GPU}\}$$

This predicate determines whether `spawn` is permitted within a region (§13.3).

##### Dynamic Semantics

**Region Lifecycle**

The evaluation of a region block proceeds as follows:

1. **Arena Allocation**: A fresh allocation arena $\alpha$ is created (per §3.4).
2. **Task Set Initialization**: The active task set $\tau$ is initialized to $\{t_{\text{main}}\}$, containing only the primary task.
3. **Body Evaluation**: The region body $e$ is evaluated in context extended with region binding $r$.
4. **Task Synchronization**: Evaluation blocks until $|\tau| = 1$ (only the primary task remains).
5. **Arena Deallocation**: All memory allocated via $\alpha$ is deallocated.
6. **Region Exit**: The region completes, yielding the result of $e$.

**Operational Semantics**

Let $\sigma = (H, \rho, \tau)$ denote a shared state where $H$ is the heap, $\rho$ is the region stack, and $\tau$ is the task set.

$$\frac{\alpha = \text{fresh\_arena}() \quad \sigma' = (H, \rho \cdot (k, \alpha, \{t_0\}), \{t_0\}) \quad \sigma', e \Downarrow \sigma'', v}{\sigma, \texttt{region}\ k\ \texttt{as}\ r\ \{ e \} \Downarrow (H'', \rho, \emptyset), v} \quad \text{(E-Region)}$$

where $\Downarrow$ denotes big-step evaluation and the task set in the final state is empty (excluding the continuation).

##### Memory & Layout

**Region Descriptor**

A region instance is represented at runtime by an **implementation-defined** descriptor. The descriptor MUST contain sufficient information to:
1. Track the allocation arena for region-allocated memory
2. Track active tasks for synchronization at region exit
3. Provide region identity for capability checking

**ABI Guarantee**: The size and layout of region descriptors are **implementation-defined (IDB)**. Implementations MUST document descriptor layouts in the Conformance Dossier.

##### Constraints & Legality

**Negative Constraints**

1. A region block MUST NOT exit while tasks remain active (enforced by runtime barrier).
2. A region's arena MUST NOT be used outside the region's lexical scope (enforced by lifetime checking per §3.5).
3. Nested regions MUST have lifetimes strictly nested (inner region fully contained within outer).

**Diagnostic Table**

| Code         | Severity | Condition                                                | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------------------- | :----------- | :-------- |
| `E-CON-3200` | Error    | Region kind requires capability not in scope             | Compile-time | Rejection |
| `E-CON-3201` | Error    | Region arena escapes lexical scope                       | Compile-time | Rejection |
| `P-CON-3202` | Panic    | Region exit with active tasks (unreachable in safe code) | Runtime      | Panic     |

##### Examples

**Valid: Basic Pool Region**

```cursive
procedure parallel_sum(data: const [i32], ctx: Context) -> i64 {
    region pool(ctx.sys.cpu_count()) as r {
        let results: [Atomic<i64>; 4] = r^[Atomic::new(0); 4]
        
        for chunk_idx in 0..4 {
            let chunk = data.slice_chunk(chunk_idx, 4)
            let target = results[chunk_idx]
            spawn {
                let sum: i64 = 0
                for elem in chunk {
                    sum = sum + elem as i64
                }
                target~>fetch_add(sum, Ordering::Relaxed)
            }
        }
    }
    
    let total: i64 = 0
    for r in results {
        total = total + r~>load(Ordering::Relaxed)
    }
    total
}
```

**Invalid: Arena Escape**

```cursive
procedure bad_escape(ctx: Context) -> const [i32] {
    let escaped: const [i32]
    region pool(4) as r {
        escaped = r^[1, 2, 3, 4]  // E-CON-3201: arena reference escapes
    }
    escaped
}
```

---

### 13.2 Execution Regions

##### Definition

An **Execution Region** is a lexically-scoped block that combines:
1. A **memory arena** for deterministic allocation (per §3.4)
2. An **execution context** determining what shared operations are permitted
3. A **task set** tracking spawned shared work

Each region kind provides different execution semantics while sharing the common structured concurrency guarantees defined in §13.1.

**Formal Definition**

An Execution Region is characterized by the tuple:

$$\text{Region} = (k, \alpha, \tau, \mathcal{C})$$

where:
- $k \in \mathcal{R}$ is the region kind
- $\alpha$ is the allocation arena
- $\tau$ is the active task set
- $\mathcal{C}$ is the set of **capabilities** available within the region

The capabilities $\mathcal{C}$ determine which operations are permitted:

| Region Kind | $\mathcal{C}$ includes               | $\mathcal{C}$ excludes                            |
| :---------- | :----------------------------------- | :------------------------------------------------ |
| Sequential  | Arena allocation                     | `Spawn`, `Suspend`                                |
| `Pool`      | Arena allocation, `Spawn`            | `Suspend`                                         |
| `Async`     | Arena allocation, `Spawn`, `Suspend` | —                                                 |
| `GPU`       | Arena allocation, `Dispatch`         | `Spawn`, `Suspend`, heap allocation               |
| `Interrupt` | Stack allocation, atomic operations  | `Spawn`, `Suspend`, `Sync`, arena/heap allocation |

##### Syntax & Declaration

**Grammar**

```ebnf
region_block       ::= "region" region_specifier? "as" IDENTIFIER block

region_specifier   ::= pool_spec  
                     | async_spec
                     | gpu_spec
                     | "interrupt"

pool_spec          ::= "pool" "(" expression ")" pool_config*

pool_config        ::= "." "stack_size" "(" expression ")"
                     | "." "affinity" "(" expression ")"
                     | "." "priority" "(" expression ")"
                     | "." "name" "(" expression ")"

async_spec         ::= "async" "(" expression ")"

gpu_spec           ::= "gpu" "(" expression ")"
```

**Desugaring**

A bare `region as r { ... }` without a specifier is a sequential region:

```cursive
region as r { e }
```

---

#### 13.2.1 Sequential Regions

##### Definition

A **Sequential Region** provides arena allocation without shared execution. It is the default region kind when no specifier is provided.

Sequential regions serve two purposes:
1. Deterministic memory management via arena allocation
2. Establishing a scope for region-allocated data

**Formal Definition**

$$\text{Sequential} = (k_{\text{seq}}, \alpha, \{t_{\text{main}}\}, \{\texttt{Allocate}\})$$

The task set contains exactly one task (the main execution thread) and never changes.

##### Syntax & Declaration

**Grammar**

```ebnf
sequential_region  ::= "region" "as" IDENTIFIER block
```

**Examples**

```cursive
region as r {
    let data = r^[0u8; 1024]
    process(data)
}
```

##### Static Semantics

**Typing Rule**

$$\frac{\Gamma, r : \text{Region}(\texttt{Sequential}) \vdash_{(\texttt{seq}, r)} e : T}{\Gamma \vdash \texttt{region as}\ r\ \{ e \} : T} \quad \text{(T-Sequential)}$$

**Forbidden Operations**

The following operations are ill-formed within a sequential region:

| Operation       | Diagnostic   |
| :-------------- | :----------- |
| `spawn { ... }` | `E-CON-3210` |
| `expr.await`    | `E-CON-3211` |

##### Dynamic Semantics

**Evaluation**

Sequential region evaluation is synchronous:

1. Allocate arena $\alpha$
2. Bind $r$ to region descriptor
3. Evaluate body $e$ to value $v$
4. Deallocate arena $\alpha$ (in reverse allocation order)
5. Return $v$

**Operational Rule**

$$\frac{\alpha = \text{alloc\_arena}() \quad (H[\alpha \mapsto \emptyset], e[r := \alpha]) \Downarrow (H', v)}{(H, \texttt{region as}\ r\ \{ e \}) \Downarrow (H' \setminus \alpha, v)} \quad \text{(E-Sequential)}$$

##### Memory & Layout

The arena $\alpha$ uses bump allocation (per §3.4). Memory is deallocated in LIFO order when the region exits.

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                    | Detection    | Effect    |
| :----------- | :------- | :--------------------------- | :----------- | :-------- |
| `E-CON-3210` | Error    | `spawn` in sequential region | Compile-time | Rejection |
| `E-CON-3211` | Error    | `await` in sequential region | Compile-time | Rejection |

##### Examples

**Valid: Arena-Scoped Processing**

```cursive
procedure process_file(path: const str, fs: witness FileSystem) -> Result<Stats, IoError> {
    region as r {
        let contents: unique [u8] = r^fs~>read_all(path)?
        let parsed: unique Document = r^Document::parse(contents)?
        parsed~>compute_stats()
    }
}
```

**Invalid: Spawn in Sequential Region**

```cursive
procedure bad_spawn() {
    region as r {
        spawn { work() }  // E-CON-3210: spawn requires parallel region
    }
}
```

---

#### 13.2.2 Pool Regions

##### Definition

A **Pool Region** provides parallel execution via a thread pool. Tasks spawned within a pool region execute sharedly on worker threads, with all tasks guaranteed to complete before the region exits.

**Formal Definition**

$$\text{Pool}(n) = (k_{\text{pool}}, \alpha, \tau, \{\texttt{Allocate}, \texttt{Spawn}\})$$

where $n \in \mathbb{N}^+$ is the **worker count**—the maximum number of shared worker threads.

The task set $\tau$ is dynamic: $|\tau| \geq 1$ during execution, and $|\tau| = 1$ (only the main task) at region exit.

##### Syntax & Declaration

**Grammar**

```ebnf
pool_region        ::= "region" "pool" "(" worker_count ")" pool_config* "as" IDENTIFIER block

worker_count       ::= expression                    (* type: usize, must be > 0 *)

pool_config        ::= "." "stack_size" "(" expression ")"
                     | "." "affinity" "(" expression ")"
                     | "." "priority" "(" expression ")"
                     | "." "name" "(" expression ")"
```

**Configuration Options**

| Method       | Parameter Type   | Description                           | Default                  |
| :----------- | :--------------- | :------------------------------------ | :----------------------- |
| `stack_size` | `usize`          | Stack size per worker thread in bytes | IDB                      |
| `affinity`   | `const [CpuId]`  | CPU cores workers may execute on      | All cores                |
| `priority`   | `ThreadPriority` | OS scheduling priority                | `ThreadPriority::Normal` |
| `name`       | `const str`      | Prefix for worker thread names        | `"pool"`                 |

**ThreadPriority Type**

The `ThreadPriority` type is defined in the standard library module `std::thread`:

```cursive
enum ThreadPriority {
    Low,
    Normal,
    High,
}
```

**CpuId Type**

```cursive
record CpuId {
    id: usize
}
```

##### Static Semantics

**Typing Rules**

**(T-Pool-Region)**
$$\frac{
    \Gamma \vdash n : \texttt{usize} \quad
    n > 0 \quad
    \Gamma, r : \text{Region}(\texttt{Pool}, n) \vdash_{(\texttt{pool}, r)} e : T
}{
    \Gamma \vdash \texttt{region pool}(n)\ \texttt{as}\ r\ \{ e \} : T
}$$

**Capability Provision**

Within a pool region, the `Spawn` capability is available, permitting the `spawn` expression (§13.3).

##### Dynamic Semantics

**Pool Lifecycle**

1. **Initialization**: 
   - Allocate arena $\alpha$
   - Create thread pool with $n$ workers (threads created lazily or eagerly; IDB)
   - Initialize task queue $Q = \emptyset$
   - Initialize task set $\tau = \{t_{\text{main}}\}$

2. **Execution**:
   - Main task evaluates region body
   - `spawn` expressions enqueue tasks to $Q$ and add to $\tau$
   - Worker threads dequeue and execute tasks
   - Completed tasks are removed from $\tau$

3. **Synchronization Barrier**:
   - When main task reaches region end, it waits until $\tau = \{t_{\text{main}}\}$
   - All spawned tasks must complete (or panic)

4. **Cleanup**:
   - Destroy thread pool
   - Deallocate arena $\alpha$
   - Return result of body expression

**Task Scheduling**

The order in which tasks are scheduled to workers is **unspecified behavior (USB)**. Implementations MAY use work-stealing, FIFO queues, or other scheduling strategies.

**Panic Propagation**

If a spawned task panics:
1. The panic is captured and stored
2. Other tasks continue executing
3. At region exit, if any task panicked, the region propagates one panic (which one is USB)

##### Memory & Layout

**Pool Descriptor**

The runtime representation of a pool region includes:

| Field          | Description                        |
| :------------- | :--------------------------------- |
| Arena pointer  | Reference to allocation arena      |
| Worker threads | Array of OS thread handles         |
| Task queue     | shared queue of pending tasks      |
| Active count   | Atomic counter of incomplete tasks |
| Panic slot     | Storage for captured panics        |

The exact layout is **implementation-defined (IDB)**.

##### Constraints & Legality

**Negative Constraints**

1. Worker count MUST be a positive integer ($n > 0$).
2. Worker count MUST be evaluable at region entry.
3. The `await` expression is forbidden in pool regions (use async regions for suspension).

**Diagnostic Table**

| Code         | Severity | Condition                                   | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------ | :----------- | :-------- |
| `E-CON-3220` | Error    | Pool worker count is zero                   | Compile-time | Rejection |
| `E-CON-3221` | Error    | Pool worker count expression has wrong type | Compile-time | Rejection |
| `E-CON-3222` | Error    | `await` in pool region                      | Compile-time | Rejection |
| `E-CON-3223` | Error    | Pool configuration type mismatch            | Compile-time | Rejection |
| `P-CON-3224` | Panic    | Worker thread stack overflow                | Runtime      | Panic     |
| `P-CON-3225` | Panic    | Spawned task panicked                       | Runtime      | Panic     |

##### Examples

**Valid: Parallel Processing with Configuration**

```cursive
procedure parallel_transform(
    input: const [Data],
    output: unique [Result],
    ctx: Context,
) {
    let cpu_count = ctx.sys~>cpu_count()
    
    region pool(cpu_count)
        .stack_size(2 * 1024 * 1024)
        .name("transform")
    as r {
        let chunk_size = input~>len() / cpu_count + 1
        
        for i in 0..cpu_count {
            let start = i * chunk_size
            let end = min(start + chunk_size, input~>len())
            let in_slice: const [Data] = input[start..end]
            let out_slice: unique [Result] = output[start..end]
            
            spawn {
                for j in 0..in_slice~>len() {
                    out_slice[j] = transform(in_slice[j])
                }
            }
        }
    }
}
```

**Invalid: Zero Workers**

```cursive
procedure bad_pool() {
    region pool(0) as r {  // E-CON-3220: worker count must be positive
        spawn { work() }
    }
}
```

---

#### 13.2.3 Async Regions

##### Definition

An **Async Region** provides cooperative multitasking via an executor that drives **futures** to completion. Tasks within an async region may **suspend** at explicit yield points (`.await`), allowing other tasks to progress without blocking OS threads.

**Formal Definition**

$$\text{Async}(R) = (k_{\text{async}}, \alpha, \tau, \{\texttt{Allocate}, \texttt{Spawn}, \texttt{Suspend}\})$$

where $R$ is a value whose type implements the `AsyncRuntime` trait (§13.2.3.1).

The `Suspend` capability enables the `.await` operator, which is forbidden in other region kinds.

##### Syntax & Declaration

**Grammar**

```ebnf
async_region       ::= "region" "async" "(" runtime_expr ")" "as" IDENTIFIER block

runtime_expr       ::= expression                    (* type must implement AsyncRuntime *)
```

**Example**

```cursive
procedure fetch_all(urls: const [Url], ctx: Context) -> Vec<Response> {
    region async(ctx.async_runtime) as r {
        let tasks: Vec<Task<Response>> = Vec::new()
        
        for url in urls {
            let u = url
            tasks~>push(spawn { http_get(u).await })
        }
        
        let results: Vec<Response> = Vec::new()
        for task in tasks {
            results~>push(task~>join().await)
        }
        results
    }
}
```

---

##### 13.2.3.1 The AsyncRuntime Trait

##### Definition

The `AsyncRuntime` trait defines the interface for async executors. Implementations provide the machinery to poll futures, schedule tasks, and integrate with I/O event sources.

```cursive
trait AsyncRuntime {
    procedure spawn<T>(~%, future: Future<T>) -> Task<T>;
    procedure block_on<T>(~%, future: Future<T>) -> T;
}
```

> **Note**: The `~%` receiver indicates `shared` permission, as runtimes are shared across tasks.

**Capability Injection**

The `Context` type (§8.9) provides an `async_runtime` field:

```cursive
record Context {
    // ... other capabilities ...
    async_runtime: witness AsyncRuntime,
}
```

The concrete runtime implementation is **implementation-defined (IDB)**. Conforming implementations MUST provide at least one `AsyncRuntime` implementation.

---

##### 13.2.3.2 Futures and the Suspend Capability

##### Definition

A **Future** represents a computation that may not yet be complete. The `Future` trait is defined as:

```cursive
trait Future {
    type Output;
    
    procedure poll(~!, cx: unique WakerContext) -> Poll<Self::Output>;
}

enum Poll<T> {
    Ready { value: T },
    Pending,
}
```

**The Suspend Capability**

The `Suspend` capability authorizes suspension points within async regions. It is a compile-time capability tracked by the region system, not a user-visible type.

$$\texttt{Suspend} \in \text{Capabilities}(R) \iff R = \text{Async}(\_)$$

##### Syntax & Declaration

**The `.await` Operator**

```ebnf
await_expr         ::= expression "." "await"
```

The `.await` operator suspends the current task until the future completes.

**Desugaring**

The `.await` operator desugars to a loop that polls the future:

```cursive
future.await

// Desugars to (conceptual; actual implementation is intrinsic):
loop {
    match future~>poll(waker_context) {
        Poll::Ready { value } => break value,
        Poll::Pending => __suspend(),
    }
}
```

The `__suspend()` intrinsic is not user-callable; it is inserted by the compiler within async contexts.

##### Static Semantics

**Typing Rules**

**(T-Await)**
$$\frac{
    \Gamma \vdash_R e : T \quad
    T <: \texttt{Future} \quad
    \texttt{Suspend} \in \text{Capabilities}(R)
}{
    \Gamma \vdash_R e\texttt{.await} : T\texttt{::Output}
}$$

**(T-Await-Forbidden)**
If `Suspend` is not available, `.await` is ill-formed:
$$\frac{
    \Gamma \vdash_R e : T \quad
    T <: \texttt{Future} \quad
    \texttt{Suspend} \notin \text{Capabilities}(R)
}{
    \Gamma \vdash_R e\texttt{.await} : \bot \quad [\texttt{E-CON-3230}]
}$$

---

##### 13.2.3.3 Async Procedures

##### Definition

An **async procedure** is a procedure that returns a `Future` and may contain `.await` expressions. The `async` keyword transforms a procedure's body into a state machine implementing `Future`.

##### Syntax & Declaration

**Grammar**

```ebnf
async_procedure    ::= "async" "procedure" IDENTIFIER generic_params? 
                       "(" param_list ")" "->" return_type block
```

**Desugaring**

An async procedure desugars to a regular procedure returning an anonymous future type:

```cursive
async procedure fetch(url: Url) -> Response {
    let conn = connect(url).await
    conn~>read_response().await
}

// Desugars to:
procedure fetch(url: Url) -> opaque Future { 
    // Compiler-generated state machine capturing `url`
    __FetchStateMachine { url: url, state: 0 }
}
```

**Witness Propagation**

Within an async region, async procedures receive the `Suspend` capability implicitly from the region context:

```cursive
region async(ctx.async_runtime) as r {
    let response = fetch(url).await  // Suspend capability from region
}
```

Outside an async region, the future can be constructed but not awaited:

```cursive
procedure prepare_fetch(url: Url) -> opaque Future {
    fetch(url)  // Returns future, does not execute
}
```

##### Static Semantics

**Typing Rules**

**(T-Async-Proc)**
$$\frac{
    \Gamma, \bar{x} : \bar{T}, \texttt{Suspend} \vdash e : U
}{
    \Gamma \vdash \texttt{async procedure } f(\bar{x}: \bar{T}) \to U\ \{ e \} : (\bar{T}) \to \texttt{opaque Future}
}$$

The `Suspend` capability is added to the context for the body.

##### Dynamic Semantics

**State Machine Generation**

The compiler transforms an async procedure body into a state machine:

1. Each `.await` point becomes a **yield state**
2. Local variables that live across yield points are captured in the state machine
3. The `poll` method resumes from the last yield state

##### Memory & Layout

**Future Size**

The size of an async procedure's future type equals the maximum size needed across all states, plus discriminant and alignment padding:

$$\text{sizeof}(\text{AsyncFuture}) = \max_{s \in \text{States}}(\text{sizeof}(s)) + \text{sizeof}(\text{discriminant}) + \text{padding}$$

The exact layout is **implementation-defined (IDB)**.

##### Constraints & Legality

**Negative Constraints**

1. `.await` MUST only appear within async regions or async procedure bodies.
2. Async procedures MUST NOT be declared with `extern` linkage.

**Diagnostic Table**

| Code         | Severity | Condition                           | Detection    | Effect    |
| :----------- | :------- | :---------------------------------- | :----------- | :-------- |
| `E-CON-3230` | Error    | `.await` outside async context      | Compile-time | Rejection |
| `E-CON-3231` | Error    | Async procedure with extern linkage | Compile-time | Rejection |
| `E-CON-3232` | Error    | Recursive async without indirection | Compile-time | Rejection |

##### Examples

**Valid: shared HTTP Fetches**

```cursive
async procedure fetch_both(a: Url, b: Url, rt: witness AsyncRuntime) -> (Response, Response) {
    region async(rt) as r {
        let task_a = spawn { http_get(a).await }
        let task_b = spawn { http_get(b).await }
        
        (task_a~>join().await, task_b~>join().await)
    }
}
```

**Invalid: Await Outside Async Context**

```cursive
procedure bad_await(url: Url) -> Response {
    http_get(url).await  // E-CON-3230: await requires async context
}
```

---

#### 13.2.4 GPU Regions

##### Definition

A **GPU Region** targets compute devices (GPUs, accelerators) for data-parallel execution. Code within a GPU region is compiled to device-specific instructions and executed on the specified device.

**Formal Definition**

$$\text{GPU}(D) = (k_{\text{gpu}}, \alpha_{\text{device}}, \tau, \{\texttt{DeviceAllocate}, \texttt{Dispatch}\})$$

where:
- $D$ is a device handle whose type implements `GpuDevice`
- $\alpha_{\text{device}}$ is device memory (distinct from host memory)
- `Dispatch` enables the `dispatch` expression for data-parallel iteration (§13.10)

GPU regions notably **exclude**:
- `Spawn` (use `dispatch` instead)
- `Suspend` (no async on GPU)
- Host heap allocation

##### Syntax & Declaration

**Grammar**

```ebnf
gpu_region         ::= "region" "gpu" "(" device_expr ")" "as" IDENTIFIER block

device_expr        ::= expression                    (* type must implement GpuDevice *)
```

**The GpuDevice Trait**

```cursive
trait GpuDevice {
    procedure allocate<T>(~%, count: usize) -> DeviceBuffer<T>;
    procedure copy_to_device<T <: Copy>(~%, dst: unique DeviceBuffer<T>, src: const [T]);
    procedure copy_to_host<T <: Copy>(~%, dst: unique [T], src: const DeviceBuffer<T>);
}
```

##### Static Semantics

**Device Code Restrictions**

Within the body of a `dispatch` inside a GPU region, the following restrictions apply:

| Construct                    | Status    | Rationale                 |
| :--------------------------- | :-------- | :------------------------ |
| Stack allocation             | Allowed   | Device has local memory   |
| Device buffer access         | Allowed   | Primary purpose           |
| Recursion                    | Forbidden | Most GPUs lack call stack |
| Dynamic dispatch (`witness`) | Forbidden | No vtables on device      |
| Host memory access           | Forbidden | Separate address spaces   |
| Allocation (`^`)             | Forbidden | No dynamic allocation     |
| Procedure calls              | Allowed   | Must be `#[gpu_callable]` |

##### Dynamic Semantics

**Memory Model**

Device memory and host memory are **disjoint address spaces**. Pointers cannot be directly shared; data must be explicitly copied via `copy_to_device` and `copy_to_host`.

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                            | Detection    | Effect    |
| :----------- | :------- | :----------------------------------- | :----------- | :-------- |
| `E-CON-3240` | Error    | `spawn` in GPU region                | Compile-time | Rejection |
| `E-CON-3241` | Error    | `.await` in GPU region               | Compile-time | Rejection |
| `E-CON-3242` | Error    | Host allocation in GPU kernel        | Compile-time | Rejection |
| `E-CON-3243` | Error    | Recursion in GPU kernel              | Compile-time | Rejection |
| `E-CON-3244` | Error    | Dynamic dispatch in GPU kernel       | Compile-time | Rejection |
| `E-CON-3245` | Error    | Non-`#[gpu_callable]` call in kernel | Compile-time | Rejection |
| `P-CON-3246` | Panic    | Device memory allocation failed      | Runtime      | Panic     |

##### Examples

**Valid: Vector Addition**

```cursive
procedure vector_add(
    a: const [f32],
    b: const [f32],
    result: unique [f32],
    ctx: Context,
) {
    assert(a~>len() == b~>len() && b~>len() == result~>len())
    let n = a~>len()
    
    region gpu(ctx.gpu_device) as g {
        let dev_a: DeviceBuffer<f32> = g~>allocate(n)
        let dev_b: DeviceBuffer<f32> = g~>allocate(n)
        let dev_result: DeviceBuffer<f32> = g~>allocate(n)
        
        g~>copy_to_device(dev_a, a)
        g~>copy_to_device(dev_b, b)
        
        dispatch idx in 0..n {
            dev_result[idx] = dev_a[idx] + dev_b[idx]
        }
        
        g~>copy_to_host(result, dev_result)
    }
}
```

---

#### 13.2.5 Interrupt Regions

##### Definition

An **Interrupt Region** enforces constraints appropriate for Interrupt Service Routines (ISRs). Code within an interrupt region is restricted to operations safe in interrupt context: no blocking, no allocation, no unbounded execution.

**Formal Definition**

$$\text{Interrupt} = (k_{\text{int}}, \emptyset, \{t_{\text{isr}}\}, \{\texttt{AtomicOps}, \texttt{VolatileAccess}\})$$

Interrupt regions have:
- No allocation arena ($\alpha = \emptyset$)
- Single task (the ISR itself)
- Only atomic operations and volatile memory access permitted for mutation

##### Syntax & Declaration

**Grammar**

```ebnf
interrupt_region   ::= "region" "interrupt" block
```

> **Note**: Interrupt regions do not bind a region name because no arena allocation is available.

**Example**

```cursive
#[interrupt_handler(IRQ_TIMER)]
procedure timer_isr(ctrl: Volatile<u32>, counter: shared Atomic<u64>) {
    region interrupt {
        ctrl~>write(EOI_BIT)
        counter~>fetch_add(1, Ordering::Relaxed)
    }
}
```

##### Static Semantics

**Forbidden Operations**

The following operations are statically forbidden within interrupt regions:

| Operation                   | Diagnostic   | Rationale                |
| :-------------------------- | :----------- | :----------------------- |
| `spawn { ... }`             | `E-CON-3250` | No threading in ISR      |
| `expr.await`                | `E-CON-3251` | No suspension in ISR     |
| `sync (x = m) { ... }`      | `E-CON-3252` | Blocking forbidden       |
| `r^expr` (arena allocation) | `E-CON-3253` | No allocator available   |
| Heap allocation             | `E-CON-3254` | May block or fail        |
| Non-interrupt-safe calls    | `E-CON-3255` | Transitivity requirement |

**The `#[interrupt_safe]` Attribute**

Procedures callable from interrupt context MUST be marked with `#[interrupt_safe]`:

```cursive
#[interrupt_safe]
procedure acknowledge(ctrl: Volatile<u32>) {
    ctrl~>write(EOI_BIT)
}
```

**Transitive Verification**: A procedure marked `#[interrupt_safe]` MUST NOT call any procedure that is not also `#[interrupt_safe]`. This is verified transitively at compile time.

##### Dynamic Semantics

Interrupt regions execute with:
- **Interrupts disabled** (on the current core): IDB whether nested interrupts are permitted
- **Bounded stack**: ISRs execute on a separate, limited stack
- **No preemption**: The ISR runs to completion

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                    | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------- | :----------- | :-------- |
| `E-CON-3250` | Error    | `spawn` in interrupt region                  | Compile-time | Rejection |
| `E-CON-3251` | Error    | `.await` in interrupt region                 | Compile-time | Rejection |
| `E-CON-3252` | Error    | `sync` block in interrupt region             | Compile-time | Rejection |
| `E-CON-3253` | Error    | Arena allocation in interrupt region         | Compile-time | Rejection |
| `E-CON-3254` | Error    | Heap allocation in interrupt region          | Compile-time | Rejection |
| `E-CON-3255` | Error    | Call to non-`#[interrupt_safe]` procedure    | Compile-time | Rejection |
| `E-CON-3256` | Error    | `#[interrupt_safe]` calls non-safe procedure | Compile-time | Rejection |

##### Examples

**Valid: Minimal ISR**

```cursive
#[interrupt_safe]
procedure uart_rx_handler(
    uart: Volatile<UartRegs>,
    buffer: shared Atomic<u64>,
) {
    region interrupt {
        let byte = uart~>read()
        buffer~>fetch_or(byte as u64, Ordering::Relaxed)
    }
}
```

**Invalid: Allocation in ISR**

```cursive
procedure bad_isr(heap: witness HeapAllocator) {
    region interrupt {
        let data = heap~>allocate::<[u8; 64]>()  // E-CON-3254
    }
}
```



### 13.4 Mutex<T> and Synchronized Access

##### Definition

`Mutex<T>` is a modal type providing synchronized mutable access to shared data.
Access to mutex-protected data occurs **exclusively** through `sync` blocks.

##### Modal States

| State       | Description                   |
| ----------- | ----------------------------- |
| `@Unlocked` | Mutex free; data inaccessible |
| `@Locked`   | Mutex held; data accessible   |

##### Ranked Mutexes

Mutexes may optionally have compile-time ranks for static deadlock prevention:

```cursive
// Unranked (dynamic ordering)
type DynMutex<T> = Mutex<T>

// Ranked (static deadlock prevention)
const RANK_DB: u32 = 100
const RANK_CACHE: u32 = 50
let db: Mutex<Db, RANK_DB> = ...
let cache: Mutex<Cache, RANK_CACHE> = ...
```

See §13.8.4 for lock rank rules.

---

### 13.5 Atomic<T> and Lock-Free Operations

##### Definition

`Atomic<T>` provides lock-free operations for types satisfying `AtomicCompatible`.

##### AtomicCompatible Trait

```cursive
trait AtomicCompatible {
    const SIZE: usize
    const ALIGNMENT: usize
    const LOCK_FREE: bool
}
```

**Architecture Limits:**

| Architecture        | Native Size | Lock-Free Types      |
| ------------------- | ----------- | -------------------- |
| 32-bit              | 32 bits     | bool, i8-i32, u8-u32 |
| 64-bit              | 64 bits     | + i64, u64, usize    |
| 64-bit + CMPXCHG16B | 128 bits    | + i128, u128         |

Types exceeding native size produce `W-ATM-4010` (lock-based fallback) or
`E-ATM-4005` (no fallback available).

---

### 13.6 Linear Lenses

##### Definition

A **Linear Lens** splits a `unique` resource into multiple disjoint `unique` views,
using a **linear ticket** to track reconstruction rights.

##### Core Mechanism

```cursive
// Split consumes 'data', returns views AND a linear ticket
let (left, right, ticket) = data~>split_at(500)

// Use views freely
region pool(2) as r {
    spawn { process(move left) }
    spawn { process(move right) }
}

// Reconstruct OR discard
let data = ticket~>rejoin(left, right)
// OR: ticket~>discard()
```

##### Lens Traits

```cursive
modal LensTicket<Parent, const N: usize> {
    @Valid { }
    @Consumed { }
}

trait SplitAt {
    type View
    procedure split_at(self: unique Self, index: usize)
        -> (unique Self::View, unique Self::View, linear LensTicket<Self, 2>)
}

trait Chunked {
    procedure chunks(self: unique Self, size: usize)
        -> (ChunkIter<Self::View>, linear LensTicket<Self, Dynamic>)
}

trait Strided {
    procedure stride_split(self: unique Self, stride: usize, offsets: [usize])
        -> ([unique StridedView<Self::Item>], linear LensTicket<Self, Dynamic>)
}
```

##### Ticket Operations

```cursive
impl<P, const N: usize> LensTicket<P, N> {
    transition rejoin(self: linear Self@Valid, views: ...) -> unique P
    to Self@Consumed

    transition discard(self: linear Self@Valid)
    to Self@Consumed
}
```

##### Constraints

| Code         | Severity | Condition                 |
| ------------ | -------- | ------------------------- |
| `E-LEN-2001` | Error    | Ticket not consumed       |
| `E-LEN-2002` | Error    | View used after rejoin    |
| `E-LEN-2003` | Error    | Rejoin with missing views |
| `E-LEN-2004` | Error    | Ticket consumed twice     |

---

### 13.7 Static Field Lenses

##### Definition

Fields of a `unique` record are implicitly lensable. Moving a field into a
parallel task creates a **static lens**—the compiler tracks which fields are
"out" and restores the parent when all fields return.

##### Syntax

No new syntax required. Behavior emerges from structured regions:

```cursive
record Player {
    pos: Vector3,
    health: f32,
    inventory: Inventory,
}

procedure update(p: unique Player) {
    region pool(2) as r {
        spawn { physics_step(move p.pos) }
        spawn { sort_items(move p.inventory) }

        // p.health still accessible (not moved)
        p.health = 100.0
    }
    // p fully restored
    log(p.pos)  // OK
}
```

##### Partial Move Tracking

| State                     | Accessible Fields | Parent Accessible |
| ------------------------- | ----------------- | ----------------- |
| `@Whole`                  | All               | Yes               |
| `@PartiallyMoved{f1, f2}` | All except f1, f2 | No                |

##### Constraints

| Code         | Severity | Condition                               |
| ------------ | -------- | --------------------------------------- |
| `E-FLD-3001` | Error    | Union field projection (aliased memory) |
| `E-FLD-3002` | Error    | Packed record field projection          |
| `E-FLD-3003` | Error    | shared record field split               |

---

### 13.8 Sync Blocks

##### Definition

A **sync block** provides temporary `unique` access to data protected by a `Mutex`,
transforming `shared Mutex<T>` into `unique T`.

##### 13.8.1 Basic Syntax

```cursive
sync (binding = mutex_expr) {
    // binding has type `unique T`
}
```

##### 13.8.2 Multiple Locks

```cursive
sync (a = mutex_a, b = mutex_b) {
    transfer(a, b, amount)
}
```

##### 13.8.3 Lock Ordering Options

**Declaration Order (default):**
```cursive
sync (a = m1, b = m2) { ... }  // Acquires m1, then m2
```

**Explicit Ordering:**
```cursive
sync(ordered: a.id < b.id) (a = acc_a, b = acc_b) { ... }
```

**Address-Based Sorting:**
```cursive
sync(address_sorted) (a = m1, b = m2) { ... }  // Runtime sorts by address
```

##### 13.8.4 Lock Ranks

Ranked mutexes enable static deadlock prevention:

```cursive
const RANK_DB: u32 = 100
const RANK_CACHE: u32 = 50

let db: Mutex<Db, RANK_DB> = ...
let cache: Mutex<Cache, RANK_CACHE> = ...

// Valid: higher rank first
sync (d = db) {
    sync (c = cache) { ... }  // OK: 50 < 100
}

// Invalid: rank violation
sync (c = cache) {
    sync (d = db) { ... }  // ERROR E-SYN-3010
}
```

**Rules:**
1. Can only acquire locks with rank **lower** than currently held
2. Same-rank locks must be acquired together, not nested

##### 13.8.5 Try-Sync

```cursive
if sync? (map = shared) {
    map.insert(key, value)
} else {
    handle_contention()
}

if sync?(timeout: 100.ms) (map = shared) { ... }
if sync?(spin: 1000) (map = shared) { ... }
```

##### 13.8.6 Condition Variables

```cursive
sync (queue = mutex, wait cond) {
    cond~>wait_while(|| queue.is_empty())
    let item = queue.pop()
}

sync (queue = mutex, notify cond) {
    queue.push(item)
    cond~>notify_one()
}
```

**Permission Flow During Wait:**
1. Before wait: `unique` permission held
2. During wait: Permission suspended, lock released
3. After wait: Lock re-acquired, permission restored

##### Constraints

| Code         | Severity | Condition                  |
| ------------ | -------- | -------------------------- |
| `E-SYN-3004` | Error    | Nested sync on same mutex  |
| `W-SYN-3003` | Warning  | Inconsistent lock ordering |
| `E-SYN-3010` | Error    | Lock rank violation        |
| `E-SYN-3011` | Error    | Same-rank nested sync      |

---

### 13.9 Atomic Matching

##### Definition

**Atomic matching** combines pattern matching with compare-and-swap, expressing
lock-free state machines declaratively.

##### Syntax

```cursive
atomic match status {
    @Idle => set @Running { started: now() },
    @Running { started } => { log("Running since {started}") },
    @Error { code } => set @Idle,
}
```

##### Compiled Form

Transforms to CAS loop:
```cursive
loop {
    let current = status.load(Acquire)
    match current {
        @Idle => {
            if status.cas(current, @Running{...}, AcqRel).is_ok() { break }
        }
        ...
    }
}
```

##### Memory Ordering

```cursive
atomic(acquire_release) match counter { n => set n + 1 }
```

| Ordering               | Load    | Store   | CAS     |
| ---------------------- | ------- | ------- | ------- |
| `relaxed`              | Relaxed | Relaxed | Relaxed |
| `acquire`              | Acquire | Relaxed | Acquire |
| `release`              | Relaxed | Release | Release |
| `acquire_release`      | Acquire | Release | AcqRel  |
| `sequential` (default) | SeqCst  | SeqCst  | SeqCst  |

##### Control Flow

**Read-only arms:** May use any control flow.
**Arms with `set`:** Early exit forbidden. Use `then` for post-CAS logic:

```cursive
atomic match status {
    @Idle => set @Running then { return true },
    _ => return false
}
```

##### Constraints

| Code         | Severity | Condition                        |
| ------------ | -------- | -------------------------------- |
| `E-ATM-4001` | Error    | Non-exhaustive atomic match      |
| `E-ATM-4006` | Error    | Early exit from `set` arm        |
| `W-ATM-4003` | Warning  | Side effects in `set` expression |

---

### 13.10 Dispatch

##### Definition

**Dispatch** is a high-level parallel iteration primitive.

##### Syntax

```cursive
region pool(8) as r {
    dispatch elem in data {
        elem.process()
    }
}
```

##### Variants

**Indexed:**
```cursive
dispatch (i, elem) in data.enumerate() {
    output[i] = transform(elem)
}
```

**Reduction:**
```cursive
let sum = dispatch elem in data reduce(0, |acc, x| acc + x)
```

**Filter:**
```cursive
let valid = dispatch elem in data filter(|e| e.is_valid())
```

##### Constraints

| Code         | Severity | Condition                          |
| ------------ | -------- | ---------------------------------- |
| `E-DSP-6001` | Error    | `dispatch` outside parallel region |
| `E-DSP-6002` | Error    | Cross-iteration dependency         |

---

### 13.11 Select

##### Definition

**Select** waits on multiple shared events.

##### Async Select

```cursive
region async(runtime) as r {
    select {
        val = t1.join() => handle(val),
        _ = timeout(5.seconds) => handle_timeout(),
    }
}
```

##### Blocking Select

```cursive
select_blocking {
    msg = chan_a.recv_blocking() => handle_a(msg),
    msg = chan_b.recv_blocking() => handle_b(msg),
}
```

##### Biased Select

```cursive
select_biased {
    urgent = high_priority.recv() => ...,
    normal = regular.recv() => ...,
}
```

##### Default Arm

```cursive
select {
    msg = channel.recv() => handle(msg),
    default => no_message(),
}
```

---

### 13.12 Using Declarations

##### Definition

**Using** creates lexically-scoped aliases for capabilities.

##### Syntax

```cursive
using <operator> = <expr> { <block> }
```

---

### 13.13 Diagnostics Summary

| Code         | Severity | Description                        |
| ------------ | -------- | ---------------------------------- |
| `E-REG-1201` | Error    | `spawn` outside parallel region    |
| `E-REG-1205` | Error    | `^` with no named region           |
| `E-REG-1210` | Error    | `sync` in interrupt region         |
| `E-REG-1211` | Error    | `^` in interrupt region            |
| `E-REG-1212` | Error    | `await` in interrupt region        |
| `E-REG-1213` | Error    | `spawn` in interrupt region        |
| `E-REG-1214` | Error    | Heap allocation in interrupt       |
| `E-REG-1215` | Error    | Non-interrupt-safe call            |
| `E-LEN-2001` | Error    | Ticket not consumed                |
| `E-LEN-2002` | Error    | View used after rejoin             |
| `E-LEN-2003` | Error    | Rejoin with missing views          |
| `E-LEN-2004` | Error    | Ticket consumed twice              |
| `E-FLD-3001` | Error    | Union field projection             |
| `E-FLD-3002` | Error    | Packed record projection           |
| `E-FLD-3003` | Error    | shared record split                |
| `E-SYN-3004` | Error    | Nested sync same mutex             |
| `W-SYN-3003` | Warning  | Inconsistent lock order            |
| `E-SYN-3010` | Error    | Lock rank violation                |
| `E-SYN-3011` | Error    | Same-rank nested sync              |
| `E-ATM-4001` | Error    | Non-exhaustive atomic match        |
| `E-ATM-4006` | Error    | Early exit from `set` arm          |
| `W-ATM-4003` | Warning  | Side effects in `set`              |
| `E-DSP-6001` | Error    | `dispatch` outside parallel region |
| `E-DSP-6002` | Error    | Cross-iteration dependency         |
| `E-CON-3203` | Error    | Invalid capture in spawn           |
| `E-ASY-5001` | Error    | `await` outside async context      |
| `E-VOL-6001` | Error    | Volatile type not Copy             |
| `W-VOL-6002` | Warning  | Volatile for thread sync           |

---

### Clause 12 Cross-Reference Notes

**Terms defined in Clause 12:**

| Term              | Section | Description                       |
| ----------------- | ------- | --------------------------------- |
| Execution Region  | §13.2   | Unified memory + execution scope  |
| Pool Region       | §13.2.2 | Thread pool execution             |
| Async Region      | §13.2.3 | Async executor execution          |
| Interrupt Region  | §13.2.5 | ISR-safe execution                |
| Spawn             | §13.3   | Task creation within region       |
| Linear Lens       | §13.6   | Ticket-based view splitting       |
| Static Field Lens | §13.7   | Automatic record field projection |
| Sync Block        | §13.8   | Permission elevation for mutexes  |
| Lock Rank         | §13.8.4 | Static deadlock prevention        |
| Atomic Match      | §13.9   | Lock-free state machines          |
| Dispatch          | §13.10  | High-level data parallelism       |
| Select            | §13.11  | shared event handling             |

**Terms removed from Clause 12:**

| Term              | Replacement      |
| ----------------- | ---------------- |
| Parallel Epoch    | Execution Region |
| Fork Expression   | Spawn            |
| JobHandle<T>      | TaskHandle<T>    |
| CREW Path         | Pool Region      |
| Coordination Path | Sync Blocks      |

---
