## Clause 21: Heterogeneous Execution

### 21.1 The Abstract Device Machine

##### Definition

The **Abstract Device Machine (ADM)** is the target execution model for Cursive code running on throughput-oriented accelerators (GPUs). It defines a hierarchical Single-Instruction, Multiple-Thread (SIMT) architecture with a partitioned memory model.

Conforming implementations **MUST** map the ADM to the underlying hardware (e.g., CUDA, SPIR-V, HIP, Metal) such that the semantic guarantees of this clause are preserved.

#### 21.1.1 Execution Hierarchy

The ADM defines execution in terms of a four-level hierarchy:

1.  **Thread ($T$)**: The fundamental unit of sequential execution. A thread maintains private state (program counter, registers, stack) and executes a kernel procedure.
2.  **Warp ($W$)**: A set of threads that execute in lockstep. The size of a warp is a hardware-defined constant, `WARP_SIZE` (typically 32 or 64). Threads within a warp share an instruction stream; divergent control flow causes serialization (see §21.10).
3.  **Block ($B$)** (also known as Workgroup): A set of threads that are co-scheduled on a single Compute Unit. Threads within a block share a fast scratchpad memory (Shared Memory) and can synchronize via execution barriers.
4.  **Grid ($G$)**: The complete set of blocks launched for a specific kernel invocation. Blocks within a grid are execution-independent; the ADM provides no guarantee of concurrent execution or ordering between blocks.

**Addressing**

Every thread is uniquely identified by a composite coordinate system:
*   $\vec{t} = (t_x, t_y, t_z)$: The **Thread Index** relative to the block.
*   $\vec{b} = (b_x, b_y, b_z)$: The **Block Index** relative to the grid.
*   $\vec{g} = (g_x, g_y, g_z)$: The **Global Thread Index**, derived as $\vec{g} = \vec{b} \times \text{BlockDim} + \vec{t}$.

#### 21.1.2 Memory Spaces

The ADM defines five distinct memory spaces with specific visibility, lifetime, and coherency properties.

| Space        | Visibility  | Lifetime    | Access    | Cursive Representation       |
| :----------- | :---------- | :---------- | :-------- | :--------------------------- |
| **Global**   | Grid + Host | Application | R/W       | `Buffer<T>@Device`           |
| **Shared**   | Block       | Kernel      | R/W       | `region [[gpu::shared]]`     |
| **Local**    | Thread      | Kernel      | R/W       | Stack bindings (`var`/`let`) |
| **Constant** | Grid        | Application | Read-Only | `const Buffer<T>@Device`     |
| **Texture**  | Grid        | Application | Sampled   | `Texture<T>` (Opaque)        |

**Static Semantics**

1.  **Global Memory**: Large, high-latency, off-chip memory (VRAM). Accessible by all threads in the grid. Coherency is **weak**; updates by one thread are not visible to others without explicit synchronization (§21.6).
2.  **Shared Memory**: Small, low-latency, on-chip scratchpad. Visible only to threads within the same Block. Data lifetime is bound to the Block execution.
3.  **Local Memory**: Private memory for thread-local variables and register spills. Not visible to other threads.
4.  **Constant Memory**: Read-only segment of Global memory optimized for broadcast access (cached).
5.  **Texture Memory**: Read-only memory accessed via spatial sampling hardware, providing interpolation and boundary handling.

#### 21.1.3 Hardware Capabilities

Access to the ADM is mediated by the `GpuCapability` class. This capability encapsulates the static properties of the physical device.

**Definition**

```cursive
class GpuCapability {
    /// Returns the hardware warp size (e.g., 32 for NVidia, 64 for AMD).
    /// This value is a compile-time constant within a kernel but runtime-variable on the host.
    procedure warp_size(~) -> usize;

    /// Returns the maximum number of threads allowed in a single block.
    procedure max_threads_per_block(~) -> usize;

    /// Returns the maximum Shared Memory (bytes) available per block.
    procedure max_shared_memory_per_block(~) -> usize;

    /// Returns the alignment requirement for Global Memory allocations.
    procedure global_mem_alignment(~) -> usize;
}
```

**Capability Constraints**

1.  **Device Heterogeneity**: A system may contain multiple devices with differing capabilities (e.g., an integrated GPU and a discrete GPU). The `GpuCapability` instance bound to a `Context` represents a *specific* device.
2.  **Constant Folding**: Within a procedure marked `[[gpu::kernel]]` or `[[gpu::device]]`, calls to `warp_size()` **MUST** be foldable to a compile-time constant by the backend to enable register allocation and loop unrolling.

#### 21.1.4 Forward Progress Guarantees

The ADM provides the following progress guarantees to support synchronization primitives:

1.  **Block Progress**: Once a block begins execution, it guarantees eventual progress for all its constituent warps, subject to barrier constraints.
2.  **Grid Progress**: The ADM does **NOT** guarantee concurrent progress of all blocks in a grid. A grid may be larger than the physical capacity of the device; blocks may execute sequentially.
    *   *Constraint*: Global synchronization (barriers across the entire grid) is **Ill-Formed** and results in deadlock if the grid size exceeds GPU residency limits.
3.  **Warp Progress**: Threads within a warp execute in lockstep. Infinite loops in one execution path of a divergent branch will starve the other path indefinitely.

---

### 21.2 The Buffer Modal Type

##### Definition

The `Buffer<T>` type is the fundamental unit of memory ownership for heterogeneous execution. It represents a contiguous, linearly-addressed allocation of typed memory residing in a specific **Memory Space**.

The physical location of the data is encoded in the **Modal State** of the buffer type. Transitions between states represent physical data movement (DMA transfers) or logical mapping changes (Unified memory).

##### Syntax & Declaration

```cursive
modal Buffer<T> {
    /// Data resides in Host (System) RAM.
    /// Memory is pageable and managed by the Host OS.
    @Host {
        ptr: *mut T,
        len: usize,
        allocator_id: usize,
    }

    /// Data resides in Device (Video) RAM.
    /// Accessible only by the GPU execution context.
    @Device {
        ptr: *mut T, // Device Virtual Address (DVA)
        len: usize,
        device_id: u32,
    }

    /// Data resides in Unified Memory (Managed).
    /// Accessible by both Host and Device. The runtime manages page migration.
    @Unified {
        ptr: *mut T,
        len: usize,
    }

    /// Data resides in Pinned (Page-Locked) Host RAM.
    /// Eligible for asynchronous DMA transfers.
    @Staging {
        ptr: *mut T,
        len: usize,
    }
}
```

##### Static Semantics

**Type Constraints**

For `Buffer<T>` to be well-formed, `T` MUST satisfy the `FfiSafe` class constraint (§20.2).
1.  **Layout**: `T` must have a deterministic layout compatible with the device (typically `[[layout(C)]]`).
2.  **No References**: `T` MUST NOT contain pointers, references, or `Buffer` handles (deep copies are not implicit).
3.  **Trivial Copy**: `T` MUST be trivially copyable (bitwise copy valid).

**State Invariants and Access Control**

The compiler enforces access rights based on the current **Execution Context** ($\Gamma_{\text{ctx}}$):

| Buffer State | Host Access                   | Device Access                 |
| :----------- | :---------------------------- | :---------------------------- |
| `@Host`      | Allowed                       | **Ill-Formed** (`E-GPU-4001`) |
| `@Device`    | **Ill-Formed** (`E-GPU-4001`) | Allowed                       |
| `@Unified`   | Allowed (Implicit Page Fault) | Allowed                       |
| `@Staging`   | Allowed                       | **Ill-Formed** (DMA Only)     |

**(T-Device-Deref-Check)**
$$\frac{
    \Gamma \vdash b : \texttt{Buffer}\langle T \rangle\texttt{@Device} \quad
    \text{Context}(\Gamma) = \texttt{Host}
}{
    \text{deref}(b) \to \text{Error: E-GPU-4001}
}$$

**Transfer Semantics**

Data movement is modeled as a state transition method.

1.  **Move Semantics**: The `to_device` method consumes the host buffer (requires `unique` permission). This statically prevents "Use-After-Free" race conditions where the Host modifies a buffer while the DMA engine is reading it.
2.  **Copy Semantics**: To preserve the host buffer, the user must explicitly `clone()` it before transfer, or use `copy_to_device` (which accepts `const` permission).

**Signatures**:

```cursive
/// Consuming Transfer (Move)
/// Host buffer is invalidated. Implementation MAY reuse the underlying allocation
/// for the new state if backing store permits, or release it immediately.
procedure to_device(self: unique Buffer<T>@Host, gpu: dyn GpuCapability) -> unique Buffer<T>@Device;

/// Non-Consuming Transfer (Copy)
/// Allocates new device memory and copies data. Host buffer remains valid.
procedure copy_to_device(self: const Buffer<T>@Host, gpu: dyn GpuCapability) -> unique Buffer<T>@Device;

/// Retrieval
procedure to_host(self: unique Buffer<T>@Device) -> unique Buffer<T>@Host;
```

##### Dynamic Semantics

**Allocation**

Buffers are allocated via the `GpuCapability` factory methods:

*   `gpu.alloc_device<T>(count: usize) -> unique Buffer<T>@Device`
*   `gpu.alloc_host<T>(count: usize) -> unique Buffer<T>@Host`
*   `gpu.alloc_unified<T>(count: usize) -> unique Buffer<T>@Unified`

**Failure Model**: Allocation failure (OOM) on the Device results in a Host-side panic (`P-GPU-5002`).

**Transfer Execution**

1.  **Synchronous Default**: By default, `to_device` blocks the Host thread until the transfer is complete. This ensures the returned `Buffer@Device` is valid for immediate use in subsequent kernel launches.
2.  **Stream Ordering**: If the transfer is part of a command stream (see Clause 24), the Host does not block, but the returned handle carries a dependency token ensuring kernels using it do not launch until the transfer completes.

**Lifetime Management**

`Buffer<T>` implements the `Drop` form.
1.  **Host/Staging**: Deallocated via system allocator.
2.  **Device**: Deallocated via GPU driver API.
3.  **Scope Safety**: If a `Buffer@Device` is dropped while a kernel is still using it (e.g., via a view), the specific behavior is implementation-defined (usually the driver defers free until queue completion), but Cursive's **Structured Parallelism** (§14.1) guarantees that kernels typically complete before bindings in the enclosing scope are dropped.

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                                                  | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------------------------------------- | :----------- | :-------- |
| `E-GPU-4001` | Error    | Accessing Device memory from Host code (or vice versa).                    | Compile-time | Rejection |
| `E-GPU-4005` | Error    | Buffer element type `T` is not `FfiSafe` (contains pointers/padding).      | Compile-time | Rejection |
| `E-GPU-4006` | Error    | `Buffer@Staging` accessed by Device kernel directly (must transfer first). | Compile-time | Rejection |
| `P-GPU-5002` | Panic    | Device Out Of Memory (OOM) during allocation or transfer.                  | Runtime      | Panic     |

---

### 21.3 Device Procedures and Constraints

##### Definition

This clause defines the execution semantics for code running within the **Device Context**. It specifies how procedures are marked for device execution, the restrictions imposed by the hardware architecture (SIMT), and how the compiler validates these constraints.

**Procedure Classification**

1.  **Kernels (`[[gpu::kernel]]`)**: Entry points initiated by the Host. They represent the root of the call graph on the device.
2.  **Device Procedures (`[[gpu::device]]`)**: Helper procedures callable *only* from other Device or Kernel procedures.
3.  **Portable Procedures** (Unannotated): Procedures that contain no context-specific logic. They may be called from either Host or Device contexts, subject to instantiation checks.

##### Syntax & Declaration

**Attributes**

The `gpu` attribute namespace is reserved for device directives.

```ebnf
gpu_attribute ::= "[[" "gpu" "::" ("kernel" | "device") "]]"
```

**Usage**

```cursive
/// A kernel entry point. Callable via launch syntax.
[[gpu::kernel]]
procedure vector_add(a: const Buffer<f32>@Device, ...) { ... }

/// A device-only helper.
[[gpu::device]]
procedure sigmoid(x: f32) -> f32 { ... }

/// A portable helper (no attribute).
procedure clamp(x: f32, min: f32, max: f32) -> f32 { ... }
```

##### Static Semantics

**Context Propagation**

The compiler tracks the **Execution Context** $\mathcal{C} \in \{ \text{Host}, \text{Device} \}$.

1.  The context defaults to $\text{Host}$.
2.  Entering a `[[gpu::kernel]]` or `[[gpu::device]]` procedure transitions $\mathcal{C}$ to $\text{Device}$.
3.  Calling a Portable procedure inherits the current $\mathcal{C}$.

**Device Constraints (Normative)**

When $\mathcal{C} = \text{Device}$, the following constructs are **Ill-Formed**:

1.  **Dynamic Dispatch**: Expressions of type `dyn Class` or method calls on `dyn` objects.
    *   *Rationale*: VTable indirection causes control flow divergence and instruction cache thrashing; implementation is often unsupported on GPUs.
    *   *Diagnostic*: `E-GPU-4011`.
2.  **Heap Allocation**: Calls to `heap.alloc`, usage of `box`, or creation of types owning heap memory (e.g., `string@Managed`).
    *   *Rationale*: Device memory allocators are non-deterministic and expensive.
    *   *Diagnostic*: `E-GPU-4014`.
3.  **Recursion**: The call graph reachable from any `[[gpu::kernel]]` MUST form a Directed Acyclic Graph (DAG).
    *   *Rationale*: GPUs have small, fixed-size stacks (often < 4KB) without overflow protection. Recursion risks silent corruption or hard faults.
    *   *Diagnostic*: `E-GPU-4010`.
4.  **Exception Handling**: `panic`, `try/catch`, or unwinding logic.
    *   *Rationale*: No runtime support for stack unwinding.
    *   *Diagnostic*: `E-GPU-4016`.
5.  **Host Interactions**: Accessing `Context` capabilities (`fs`, `net`, `sys`) or dereferencing `@Host` buffers.
    *   *Diagnostic*: `E-GPU-4001`.

**Kernel Signature Constraints**

A procedure annotated with `[[gpu::kernel]]` MUST satisfy:

1.  **Return Type**: MUST be `()`. Kernels return data via side effects to output buffers.
2.  **Parameter Types**: MUST be **Kernel-Safe**. A type $T$ is Kernel-Safe iff:
    *   $T$ is a primitive scalar (integer, float, bool).
    *   $T$ is a `Buffer` or `View` in the `@Device` or `@Unified` state.
    *   $T$ is a `struct` or `tuple` where all fields are Kernel-Safe.
    *   $T$ implements `Copy` (pass-by-value).

**(T-Kernel-Sig)**
$$\frac{
    \text{HasAttribute}(P, \texttt{gpu::kernel}) \quad
    \text{ReturnType}(P) = () \quad
    \forall \text{arg} \in \text{Args}(P), \text{IsKernelSafe}(\text{Type}(\text{arg}))
}{
    P \text{ is Well-Formed}
}$$

**Portable Procedure Instantiation**

When a portable procedure $P$ is called from a Device context:
1.  The compiler generates a specialized instance $P_{\text{device}}$.
2.  The body of $P$ is checked against **Device Constraints**.
3.  If the body violates constraints (e.g., calls `println`), the *call site* in the Device code triggers the error `E-GPU-4017`.

##### Dynamic Semantics

**Stack Frame Analysis**

The implementation MUST perform **Static Stack Analysis** for all kernels.
1.  Calculate the maximum stack depth (bytes) required for the kernel execution along the deepmost path of the DAG.
2.  If `MaxStack` > `HardwareLimit` (IDB, e.g., 2KB), compilation fails with `E-GPU-4012`.

**Function Pointers**

Function pointers (`fn(T)->U`) are supported in Device code ONLY if:
1.  They point to `[[gpu::device]]` procedures.
2.  The target is known at compile time (direct calls), OR the architecture supports indirect branching.
    *   *Note*: Portable code relying on function pointers may fail to compile for GPU targets that lack indirect branch support.

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                         | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------------ | :----------- | :-------- |
| `E-GPU-4010` | Error    | Recursive call cycle detected in device code.     | Compile-time | Rejection |
| `E-GPU-4011` | Error    | Usage of `dyn` or virtual method in device code.  | Compile-time | Rejection |
| `E-GPU-4012` | Error    | Static stack analysis exceeds hardware limit.     | Compile-time | Rejection |
| `E-GPU-4013` | Error    | `[[gpu::kernel]]` has non-void return type.       | Compile-time | Rejection |
| `E-GPU-4014` | Error    | Heap allocation (or `box`) in device code.        | Compile-time | Rejection |
| `E-GPU-4016` | Error    | Panic or unwind logic in device code.             | Compile-time | Rejection |
| `E-GPU-4017` | Error    | Portable procedure contains device-illegal logic. | Compile-time | Rejection |

---

### 21.4 Structured Launch

##### Definition

A **Structured Launch** maps Cursive's `parallel` construct to the GPU's Grid/Block execution hierarchy. It serves as the bridge between Host control flow and Device execution.

The launch construct performs three atomic actions:
1.  **Configuration**: Resolves grid dimensions and resource requirements.
2.  **Marshaling**: Captures and formats arguments for the device ABI.
3.  **Synchronization**: Enforces the Structured Concurrency invariant—the Host execution logic waits at the block boundary until the Device Grid completes.

> **Note on Asynchrony:** To overlap GPU execution with CPU work (Async Compute), the programmer MUST wrap the `parallel` block in a Host-side `spawn`. This preserves lifetime safety (the Host stack remains valid while the GPU accesses it) while allowing concurrency.

##### Syntax & Declaration

**Grammar**

```ebnf
gpu_parallel    ::= "parallel" gpu_domain launch_config? block

gpu_domain      ::= expression (* Must evaluate to dyn GpuCapability *)

launch_config   ::= "(" config_arg ("," config_arg)* ")"

config_arg      ::= "grid" ":" dims
                  | "block" ":" dims
                  | "shared_mem" ":" expression

dims            ::= "[" expression ("," expression){0,2} "]"
                  | expression (* Scalar expands to [e, 1, 1] *)
```

**Dispatch Statement**

Inside a GPU `parallel` block, the `dispatch` statement identifies the kernel body.

```ebnf
gpu_dispatch    ::= "dispatch" index_pattern "in" range_expr block
```

The `range_expr` is semantic sugar; the actual execution bounds are determined by `grid` and `block` dimensions. The runtime executes the `dispatch` body once per active thread.

##### Static Semantics

**Configuration Types**

1.  **Grid Dimensions**: MUST evaluate to `[u32; 3]` or a scalar `u32`.
2.  **Block Dimensions**: MUST evaluate to `[u32; 3]` or a scalar `u32`.
3.  **Shared Memory**: MUST evaluate to `usize` (bytes). Defaults to 0 if omitted.

**Capture and Marshaling**

When the `dispatch` body captures bindings from the enclosing Host scope, the compiler generates a **Kernel Argument Buffer**.

1.  **Primitive Scalars** (`i32`, `f32`, etc.): Copied by value into the argument buffer.
2.  **Device Buffers** (`Buffer@Device`, `Buffer@Unified`): The 64-bit Device Virtual Address (DVA) is copied.
3.  **Views**: The struct content (pointer + metadata) is shallow-copied.
4.  **Host Buffers** (`Buffer@Host`): Capture is **Ill-Formed** (`E-GPU-4001`).
5.  **Unique Bindings**: Capturing a `unique Buffer` logically moves ownership to the kernel for the duration of the launch.

**(T-Launch-Capture)**
$$\frac{
    \text{Capture}(v) \land \text{Type}(v) = \texttt{Buffer@Host}
}{
    \text{Emit}(\texttt{E-GPU-4001})
}$$

**Index Intrinsics**

The following intrinsics are available only within a `dispatch` block targeted at a GPU domain:

| Intrinsic           | Type              | Semantics                       |
| :------------------ | :---------------- | :------------------------------ |
| `gpu::thread_idx()` | `(u32, u32, u32)` | Location of thread within block |
| `gpu::block_idx()`  | `(u32, u32, u32)` | Location of block within grid   |
| `gpu::block_dim()`  | `(u32, u32, u32)` | Size of block (from config)     |
| `gpu::grid_dim()`   | `(u32, u32, u32)` | Size of grid (from config)      |
| `gpu::warp_size()`  | `u32`             | Hardware warp size (constant)   |

##### Dynamic Semantics

**Launch Sequence**

Evaluation of `parallel gpu(grid: G, block: B, shared_mem: S) { ... }`:

1.  **Resource Validation**:
    *   Verify $B_x \times B_y \times B_z \leq \text{gpu.max\_threads\_per\_block()}$.
    *   Verify $S \leq \text{gpu.max\_shared\_memory()}$.
    *   If invalid, panic with `P-GPU-5003`.
2.  **Argument Setup**:
    *   Allocate kernel argument buffer (implementation-defined, typically mapped memory).
    *   Copy captured values and buffer pointers.
3.  **Enqueue**:
    *   Submit the kernel to the device command queue.
4.  **Wait**:
    *   The Host thread blocks on the completion signal of the command.
    *   If the kernel triggers a hardware fault (MMU fault, TDR), the wait returns an error, propagating as a Host-side panic (`P-GPU-5001`).

**Execution Model**

*   The Device spawns a grid of $(G_x, G_y, G_z)$ blocks.
*   Each block contains $(B_x, B_y, B_z)$ threads.
*   Threads execute the `dispatch` body.
*   The `dispatch` iterator variable (if present) is bound to a linearized global index:
    $$i = (\text{block\_idx}.x \times \text{block\_dim}.x) + \text{thread\_idx}.x$$
    *(Note: Multi-dimensional dispatch binds a tuple index).*

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                                     | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------------------------ | :----------- | :-------- |
| `E-GPU-4020` | Error    | Launch configuration dimensions non-integer.                  | Compile-time | Rejection |
| `E-GPU-4021` | Error    | Capture of Host-only type (e.g., `String`, `File`) in kernel. | Compile-time | Rejection |
| `E-GPU-4022` | Error    | Use of GPU intrinsics outside `parallel` block.               | Compile-time | Rejection |
| `P-GPU-5003` | Panic    | Launch configuration exceeds hardware limits (Runtime).       | Runtime      | Panic     |

---

### 21.5 Shared Memory Regions

##### Definition

**Shared Memory** is a fast, low-latency, programmable scratchpad memory located on the Compute Unit. It is visible to all threads within a single **Block** (Workgroup).

Cursive exposes Shared Memory via the `region` system (§3.7). A region annotated with `[[gpu::shared]]` allocates memory from the device's shared memory bank rather than the stack or heap.

**Key Characteristics:**
1.  **Block Scope**: The memory is shared by all threads in the block.
2.  **Lifetime**: The data persists for the duration of the Block's execution.
3.  **Manual Synchronization**: Changes made by one thread are not visible to others until a **Barrier** is executed.

##### Syntax & Declaration

**Grammar**

```ebnf
shared_region   ::= "region" shared_attr region_alias block

shared_attr     ::= "[[" "gpu" "::" "shared" "(" size_expr ")" "]]"

size_expr       ::= constant_expression
                  | "extern" (* Dynamic size determined at launch *)
```

**Usage**

```cursive
dispatch i in 0..n {
    // Allocate 4KB of static shared memory
    region [[gpu::shared(1024 * 4)]] as cache {
        // Pointer arithmetic / casting often required here for raw byte buffers
        // or typed allocations via ^cache
        let tile: [f32; 256] = ^cache;
        
        // ... usage ...
    }
}
```

##### Static Semantics

**Uniformity Constraint**

The `region` declaration MUST appear in a **Uniform Control Flow** context.
*   It MUST NOT appear inside a branch (`if/else`) or loop whose condition depends on `thread_idx`.
*   *Rationale*: All threads in the block must reach the region declaration to establish the memory mapping consistently.
*   *Diagnostic*: `E-GPU-4030`.

**Size Constraints**

1.  **Static Allocation**: If `size_expr` is a constant expression, the compiler reserves fixed storage.
2.  **Dynamic Allocation**: If `size_expr` is `extern`, the size is determined by the `shared_mem` parameter of the launch configuration (§21.4).
    *   Only **one** `extern` shared region is permitted per kernel.
    *   *Diagnostic*: `E-GPU-4031`.

**Key System Integration**

Shared memory is a `shared` resource in the Key System (§13).
1.  **Implicit Sharing**: The pointer returned by `^cache` references the same physical memory for all threads.
2.  **Disjointness**:
    *   Accessing `tile[thread_idx.x]` is **Statically Safe** (Disjoint Path).
    *   Accessing `tile[0]` from all threads requires a **Write Key** (which implies serialization/atomic) or explicit synchronization.

##### Dynamic Semantics

**Initialization**

Shared memory contents are **Uninitialized** (undefined values) upon region entry. The programmer MUST assume garbage data.

**Visibility and Barriers**

Memory consistency is **Weak**.
*   **Write Propagation**: A write by Thread A is NOT guaranteed to be visible to Thread B immediately.
*   **Synchronization**: The `gpu::barrier()` intrinsic MUST be issued to establish ordering.
    *   `gpu::barrier()` acts as a `Fence(Scope::Block, Order::AcqRel)` followed by an Execution Barrier.

**Safe Access Pattern**:
1.  Load Global $\to$ Shared.
2.  `gpu::barrier()`.
3.  Compute/Read Shared.
4.  `gpu::barrier()` (if writing again).
5.  Store Shared $\to$ Global.

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                                            | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------------------------------- | :----------- | :-------- |
| `E-GPU-4030` | Error    | Shared region declared in divergent control flow.                    | Compile-time | Rejection |
| `E-GPU-4031` | Error    | Multiple `extern` shared regions in one kernel.                      | Compile-time | Rejection |
| `E-GPU-4032` | Error    | Static shared size exceeds hardware limit (checked if target known). | Compile-time | Rejection |
| `W-GPU-4033` | Warning  | Shared memory read-after-write without intervening barrier.          | Compile-time | Warning   |

---

### 21.6 Memory Consistency Model

##### Definition

The Cursive Device Memory Model is a **scoped, weakly-ordered consistency model**. It is derived from the HRF-relaxed (Heterogeneous Race-Free) model. It defines the visibility of memory operations across different scopes (Thread, Block, Device) and the synchronization primitives required to establish ordering.

This model applies to all memory spaces defined in the Abstract Device Machine (§21.1) except **Local** memory (which is always thread-private).

##### Static Semantics

**Visibility Scopes**

The model defines four hierarchical visibility scopes, forming a lattice $\mathcal{S} = \{ \text{Thread} \sqsubset \text{Block} \sqsubset \text{Device} \sqsubset \text{System} \}$:

1.  **Thread**: Visibility restricted to the executing thread. (Registers, Local Memory).
2.  **Block**: Visibility extends to all threads in the same Block (Workgroup).
3.  **Device**: Visibility extends to all threads in the Grid.
4.  **System**: Visibility extends to the Host CPU and peer devices (via PCIe atomics/UVA).

**Access Types**

Memory operations are classified by their ordering semantics:

1.  **Non-Atomic (Relaxed)**: Standard loads/stores. No ordering guarantees relative to other threads. Data races on non-atomic accesses constitute **Undefined Behavior**.
2.  **Atomic Relaxed**: Atomic RMW (Read-Modify-Write). Guarantees atomicity and total order per address, but no synchronization with other addresses.
3.  **Atomic Acquire**: A load operation that performs an acquire on a specific Scope $S$.
4.  **Atomic Release**: A store operation that performs a release on a specific Scope $S$.
5.  **Atomic AcqRel**: An RMW operation performing both acquire and release.
6.  **Atomic SeqCst**: Sequentially Consistent ordering (rarely supported efficiently on GPUs; maps to AcqRel + heavy fencing).

**The Happens-Before Relation ($\xrightarrow{hb}$)**

Let $A$ and $B$ be memory operations. $A \xrightarrow{hb} B$ if:

1.  **Program Order**: $A$ is sequenced before $B$ by the thread's control flow.
2.  **Synchronizes-With**:
    *   $A$ is a Release operation on scope $S$.
    *   $B$ is an Acquire operation on scope $S$.
    *   $A$ and $B$ operate on the same synchronization variable (or barrier).
    *   $B$ observes the value written by $A$ (or a value later in the modification order).
3.  **Transitivity**: $A \xrightarrow{hb} X \land X \xrightarrow{hb} B \implies A \xrightarrow{hb} B$.

**Coherency Rule**: A read $R$ by thread $T_2$ is allowed to observe a write $W$ by thread $T_1$ if and only if:
1.  $W$ is visible to $T_2$'s scope.
2.  There is no intervening write $W'$ such that $W \xrightarrow{hb} W' \xrightarrow{hb} R$.

##### Dynamic Semantics

**Barrier Semantics**

The `gpu::barrier()` intrinsic is a **Block-Scope Execution and Memory Barrier**.

1.  **Execution**: No thread in the block may proceed past the barrier until all active threads in the block have reached it.
2.  **Memory**:
    *   All memory operations sequenced before the barrier in any block thread are **visible** to all other block threads after the barrier.
    *   Effectively: `Fence(Scope::Block, Order::AcqRel)`.

**Fence Semantics**

Explicit fences establish ordering without halting execution flow.

*   `gpu::thread_fence_block()`: Ensures that all prior writes by the calling thread are visible to block threads before any subsequent writes.
*   `gpu::thread_fence_device()`: Ensures that all prior writes are visible to the entire grid before subsequent writes.
*   `gpu::thread_fence_system()`: Ensures that all prior writes are visible to the host/peers before subsequent writes.

**Volatile Qualifiers**

Accesses to `volatile` marked pointers (e.g., MMIO registers, doorbell queues) MUST bypass L1/L2 caches.

*   **Syntax**: `Volatile<T>` wrapper type in `std::gpu`.
*   **Semantics**: Compiles to `ld.volatile` / `st.volatile` (PTX) or `GLC` (coherent) loads (AMD). Reads are not cached; writes are write-through.

##### Constraints & Legality

**Negative Constraints**

1.  **Scope Mismatch**: Performing a `Scope::Device` atomic on a pointer to `Shared` memory is **Ill-Formed** (Shared memory does not exist at Device scope).
2.  **Barrier Divergence**: Calling `gpu::barrier()` inside divergent control flow (where not all threads in the block execute the barrier) leads to **Deadlock** or **Undefined Behavior**.
    *   *Note*: The compiler SHOULD attempt to detect static control flow divergence reaching a barrier (`W-GPU-4040`).

**Diagnostic Table**

| Code         | Severity | Condition                                                        | Detection    | Effect    |
| :----------- | :------- | :--------------------------------------------------------------- | :----------- | :-------- |
| `E-GPU-4041` | Error    | Atomic operation on non-atomic type (must be 32/64-bit integer). | Compile-time | Rejection |
| `W-GPU-4040` | Warning  | Barrier detected in potentially divergent control flow.          | Compile-time | Warning   |

---

### 21.7 Warp Intrinsics

##### Definition

**Warp Intrinsics** provide intra-warp communication primitives, allowing threads within the same warp to exchange data via registers without passing through Shared or Global memory. These operations utilize the SIMD nature of the hardware.

**Terminology**

*   **Warp**: A coalesced group of threads (size $W$) executing in lockstep.
*   **Lane ID**: The index of a thread within its warp ($0 \dots W-1$).
*   **Active Thread**: A thread currently executing the instruction. Threads may be inactive due to:
    *   Running beyond the grid dimensions.
    *   Divergent control flow (e.g., inside an `if` branch not taken by the thread).
    *   Early exit (`return`).

##### Static Semantics

**Availability**

Warp intrinsics are available via the `std::gpu::warp` module. They MUST only be called from procedures within the **Device Context** ($\Gamma_{\text{ctx}} = \text{Device}$). Calling them from Host code triggers `E-GPU-4022`.

**Supported Types**

Shuffle operations support **Register-Sized Types**:
*   Primitive integers (`u8` through `u64`, `i8` through `i64`).
*   Floating point (`f16`, `f32`, `f64`).
*   Pointers (`*mut T`, `*imm T`).
*   `bool`.

Composite types (structs, arrays) must be destructured into register-sized components to be shuffled.

##### Dynamic Semantics

**Data Exchange (Shuffle)**

Shuffle operations allow threads to read a register value from another thread in the same warp.

```cursive
/// Broadcasts value from `src_lane` to all active threads.
/// Returns the value held by `src_lane`.
procedure shuffle<T>(val: T, src_lane: u32) -> T;

/// Shifts value down: Lane `i` receives from `i - delta`.
/// Returns own `val` if `i < delta`.
procedure shuffle_up<T>(val: T, delta: u32) -> T;

/// Shifts value up: Lane `i` receives from `i + delta`.
/// Returns own `val` if `i + delta >= WARP_SIZE`.
procedure shuffle_down<T>(val: T, delta: u32) -> T;

/// XOR Shuffle: Lane `i` receives from `i ^ mask`.
/// Used for butterfly reductions.
procedure shuffle_xor<T>(val: T, mask: u32) -> T;
```

**Inactive Source Lane Behavior**:
If the target `src_lane` is inactive (disabled due to divergence), the result of the shuffle is **Unspecified Value**. It is not Undefined Behavior (it will not crash), but the value read is garbage. The programmer MUST ensure `src_lane` is active using `warp::ballot()` or control flow logic.

**Voting and Masks**

Voting operations evaluate a predicate across the active warp.

```cursive
/// Returns true if `predicate` is true for ALL active threads.
procedure all(predicate: bool) -> bool;

/// Returns true if `predicate` is true for ANY active thread.
procedure any(predicate: bool) -> bool;

/// Returns a bitmask where bit `k` is set iff the thread at Lane `k`
/// is active AND `predicate` is true.
procedure ballot(predicate: bool) -> u64;

/// Returns a bitmask of all currently active threads in the warp.
procedure active_mask() -> u64;
```

**Lane Identification**

```cursive
/// Returns the thread's index within the warp (0..31 or 0..63).
procedure lane_id() -> u32;
```

##### Constraints & Legality

**Mask Convergence Requirement**

Modern GPU architectures (e.g., Nvidia Volta+) require explicit masks for warp-synchronous operations to handle independent thread scheduling.

The Cursive compiler **MUST** perform **Convergence Analysis** to generate the appropriate hardware mask (e.g., `member_mask` for `__shfl_sync`). The mask passed to the hardware instruction includes all threads statically reachable in the current control flow graph node.

**Diagnostic Table**

| Code         | Severity | Condition                                                               | Detection    | Effect    |
| :----------- | :------- | :---------------------------------------------------------------------- | :----------- | :-------- |
| `E-GPU-4050` | Error    | Shuffle on non-register type (e.g., large struct).                      | Compile-time | Rejection |
| `W-GPU-4051` | Warning  | Shuffle source lane depends on runtime value (potential inactive read). | Compile-time | Warning   |

---

### 21.8 Atomic Operations

##### Definition

**Atomic Operations** provide Read-Modify-Write (RMW) primitives that are indivisible with respect to other threads executing in the same scope. They allow safe concurrent modification of Shared and Global memory without explicit locks.

##### Static Semantics

**Supported Memory Spaces**

Atomic operations are valid on pointers deriving from:
1.  **Global Memory** (`Buffer@Device`, `Buffer@Unified`): Scoped to `Scope::Device`.
2.  **Shared Memory** (`region [[gpu::shared]]`): Scoped to `Scope::Block`.

Performing atomics on **Local** memory (stack) or **Constant** memory is **Ill-Formed** (`E-GPU-4060`).

**Supported Types**

The following types support atomic operations:
*   `u32`, `i32` (Universal support)
*   `u64`, `i64` (Universal support)
*   `f32` (Universal support for Add/Exch)
*   `f64` (Architecture-dependent support, typically `sm_60+`)

**Operation Classification**

Operations are provided via the `std::gpu::atomic` module. All operations return the **previous value** stored at the address.

| Operation | Semantics                             | Integer |    Float    |
| :-------- | :------------------------------------ | :-----: | :---------: |
| `load`    | Atomic Read                           |    ✓    |      ✓      |
| `store`   | Atomic Write (returns void)           |    ✓    |      ✓      |
| `exch`    | Swap value                            |    ✓    |      ✓      |
| `cas`     | Compare-and-Swap                      |    ✓    | *(bitwise)* |
| `add`     | $old + val$                           |    ✓    |      ✓      |
| `sub`     | $old - val$                           |    ✓    |      -      |
| `min`     | $\min(old, val)$                      |    ✓    |      -      |
| `max`     | $\max(old, val)$                      |    ✓    |      -      |
| `and`     | $old \ \& \ val$                      |    ✓    |      -      |
| `or`      | $old \                                | \ val$  |      ✓      | - |
| `xor`     | $old \ \text{\textasciicircum} \ val$ |    ✓    |      -      |

##### Dynamic Semantics

**Memory Ordering**

By default, atomic operations in Cursive use **Relaxed** ordering (`Ordering::Relaxed`).
*   *Rationale*: This matches the native behavior of GPU ISA instructions (e.g., PTX `atom.global.add`). Stronger ordering requires explicit fences.

**Scope Deduction**

The compiler infers the scope of the atomic operation based on the pointer's provenance:
*   Pointer to `Shared` region $\to$ `Scope::Block`.
*   Pointer to `Global` buffer $\to$ `Scope::Device`.

**Compare-and-Swap (CAS)**

```cursive
procedure cas<T>(ptr: *mut T, compare: T, new_val: T) -> T
```

*   If `*ptr == compare`, sets `*ptr = new_val`.
*   Returns the value of `*ptr` before the operation.
*   **Success**: `result == compare`.
*   **Failure**: `result != compare`.

**Floating Point Atomics**

1.  **Associativity**: Floating point `atomic_add` is **not associative**. The order of summation is non-deterministic due to thread scheduling. Results may vary bitwise between runs.
2.  **CAS on Float**: Performed bitwise. NaN values compare equal to themselves bitwise if the bit patterns are identical.

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                                 | Detection    | Effect    |
| :----------- | :------- | :-------------------------------------------------------- | :----------- | :-------- |
| `E-GPU-4060` | Error    | Atomic operation on Local or Constant memory.             | Compile-time | Rejection |
| `E-GPU-4061` | Error    | Atomic operation on unsupported type (e.g., `u8`, `u16`). | Compile-time | Rejection |
| `W-GPU-4062` | Warning  | System-scope atomic used (PCIe performance hazard).       | Compile-time | Warning   |

---

### 21.9 Host-Device Interaction

##### Definition

This clause defines the mechanisms for coordination, synchronization, and error propagation between the Host (CPU) and the Device (GPU). It specifies the **Stream** model for command ordering and the **Event** system for fine-grained dependencies.

##### Static Semantics

**The Command Stream**

The ADM executes commands (kernels, memory transfers) via **Streams** (Command Queues).

1.  **Ordering**: Commands issued to a single stream execute in issue-order (FIFO).
2.  **Concurrency**: Commands issued to distinct streams may execute concurrently or out-of-order relative to each other.
3.  **The Default Stream**: Every `Context` possesses a default stream. Operations not explicitly assigned to a stream use this default.
    *   *Constraint*: The default stream provides **Legacy Synchronization**: it synchronizes with all other streams on the device unless configured otherwise (Implementation-Defined, typically matches CUDA legacy stream behavior).

**GpuStream Type**

```cursive
class GpuStream {
    /// Inserts a synchronization marker.
    procedure record_event(~) -> GpuEvent;

    /// Makes this stream wait until the event has completed.
    /// Does NOT block the Host thread.
    procedure wait_event(~, event: GpuEvent);

    /// Blocks the Host thread until all commands in this stream complete.
    procedure synchronize(~);
}
```

**GpuEvent Type**

```cursive
class GpuEvent {
    /// Returns true if the event has passed.
    procedure is_complete(~) -> bool;

    /// Blocks the Host thread until the event occurs.
    procedure synchronize(~);

    /// Returns elapsed time between two events in milliseconds.
    static procedure elapsed(start: GpuEvent, end: GpuEvent) -> f32;
}
```

##### Dynamic Semantics

**Launch Ordering**

The `parallel` construct (§21.4) submits work to a stream.

1.  **Implicit Stream**: By default, `parallel ctx.gpu` submits to the default stream.
2.  **Explicit Stream**: `parallel stream` (where `stream` is a `GpuStream` capability) submits to the specified stream.

**Dependency Management**

Cross-stream dependencies are established via Events.

```cursive
let s1 = ctx.gpu.create_stream();
let s2 = ctx.gpu.create_stream();

// Launch on S1
parallel s1(grid: ...) { ... }
let e = s1.record_event();

// Make S2 wait for S1's work to finish (on GPU, no CPU block)
s2.wait_event(e);

// Launch on S2 (guaranteed to run after S1's kernel)
parallel s2(grid: ...) { ... }
```

**Asynchronous Error Propagation**

GPU execution is asynchronous relative to the Host. Errors (MMU faults, TDRs, illegal instructions) are **Sticky**.

1.  **Detection**: Hardware faults are recorded in the device context status register.
2.  **Reporting**: Any subsequent Host-Device interaction (launch, synchronize, transfer) checks this register.
3.  **Panic**: If the register indicates failure, the Host operation panics with `P-GPU-5001`.
    *   *Note*: The panic may occur on a *different* kernel launch than the one that caused the fault. To localize errors, use `stream.synchronize()` immediately after launch during debugging.

**TDR (Timeout Detection and Recovery)**

If a kernel exceeds the OS-defined execution time limit (Watchdog):
1.  The OS terminates the kernel.
2.  The Device Context becomes **Poisoned**.
3.  All subsequent operations on that context panic immediately.
4.  Recovery requires destroying and recreating the `GpuCapability` (usually restarting the process).

##### Constraints & Legality

**Stream Safety**

`GpuStream` and `GpuEvent` handles are **Not Thread Safe** (`!Sync`) by default unless the implementation guarantees thread-safe submission. Passing a stream to multiple Host threads requires external synchronization (e.g., `Mutex<GpuStream>`).

**Diagnostic Table**

| Code         | Severity | Condition                                          | Detection | Effect |
| :----------- | :------- | :------------------------------------------------- | :-------- | :----- |
| `P-GPU-5001` | Panic    | Asynchronous device error detected (sticky error). | Runtime   | Panic  |
| `P-GPU-5004` | Panic    | Event wait on different device context.            | Runtime   | Panic  |

---

### 21.10 Execution Semantics

##### Definition

This clause defines the operational semantics of the **Single Instruction, Multiple Thread (SIMT)** execution model. It specifies how threads within a warp execute instructions in lockstep, how control flow divergence is handled, and how the runtime manages resource limits like stack usage.

##### Static Semantics

**The Active Mask**

At every program point $P$, there exists an implicit **Active Mask** ($\mathcal{M}_P$) representing the set of threads in the warp currently enabled to execute the instruction at $P$.

**Uniformity Analysis**

The compiler tracks the **Uniformity** of values to optimize control flow and verify warp-intrinsic safety. A value is **Uniform** if it is provably identical for all active threads in the warp.

1.  **Sources of Uniformity**:
    *   Compile-time constants.
    *   `gpu::block_idx()`, `gpu::grid_dim()`, `gpu::block_dim()` (invariant across the block/warp).
    *   Arguments passed to the kernel (invariant across the grid).
    *   Results of `gpu::shuffle(val, constant_lane)` (broadcast).
2.  **Sources of Variation**:
    *   `gpu::thread_idx()`.
    *   Loads from Global/Shared memory (unless address is Uniform and memory is `const`).
    *   Atomic results.

**Branch Classification**:
*   **Uniform Branch**: `if (cond)`. If `cond` is Uniform, the warp does not diverge. All threads take the same path.
*   **Varying Branch**: `if (cond)`. If `cond` is Varying, the warp diverges.

##### Dynamic Semantics

**Control Flow Divergence**

When execution reaches a Varying Branch with condition $C$:

1.  The warp **Diverges**.
2.  The hardware evaluates $C$ for all active threads.
3.  **Mask Split**: The active mask $\mathcal{M}$ is split into $\mathcal{M}_{true} = \{t \in \mathcal{M} \mid C(t)\}$ and $\mathcal{M}_{false} = \{t \in \mathcal{M} \mid \neg C(t)\}$.
4.  **Serialization**: The hardware arbitrarily selects one path (e.g., True) to execute first with mask $\mathcal{M}_{true}$. Threads in $\mathcal{M}_{false}$ become inactive.
5.  **Path Execution**: Instructions in the True path are executed.
6.  **Switch**: Upon reaching the reconvergence point (or end of path), execution suspends for $\mathcal{M}_{true}$ and resumes for $\mathcal{M}_{false}$ executing the False path.

**Maximal Reconvergence Guarantee**

Cursive guarantees **Maximal Reconvergence**. Threads that diverged at a branch MUST reconverge at the **Immediate Post-Dominator** (IPDOM) of that branch.

*   **Definition**: The IPDOM of a block $B$ is the unique block that strictly post-dominates $B$ and does not strictly post-dominate any other node that strictly post-dominates $B$.
*   **Implication**: Threads wait for their siblings at the earliest possible merge point in the control flow graph.

**Loop Semantics**

For a loop `while (C) { ... }`:
1.  Threads execute the body as long as $C$ is true.
2.  Threads where $C$ becomes false **disable themselves** and wait at the loop's immediate post-dominator.
3.  The warp continues executing the loop body as long as $\exists t \in \mathcal{M}.\ C(t)$.
4.  **Starvation Hazard**: If a loop in a divergent path is infinite for any thread, the entire warp hangs indefinitely (other threads waiting at the post-dominator never resume).

**Stack Management**

Device execution does not support dynamic stack growth (paging).

1.  **Static Analysis**: The compiler MUST compute the maximum stack frame size required for the kernel (`MaxStack`).
2.  **Limit Check**: If `MaxStack` exceeds the hardware limit (Implementation-Defined, typically 1KB–4KB), compilation fails (`E-GPU-4012`).
3.  **Runtime behavior**: If a thread exceeds the stack bound at runtime (e.g., via variable-length array indexing not caught statically), the result is **Undefined Behavior** (typically memory corruption or a hard fault).

**Trap Handling**

If a thread triggers a hardware exception (MMU fault, Misaligned Address, Integer Divide-by-Zero):

1.  **Warp Abort**: The warp stops execution immediately.
2.  **Context Poisoning**: The device context sets a sticky error bit.
3.  **Grid Termination**: The hardware attempts to terminate all other warps in the grid (best effort).
4.  **Host Notification**: The `parallel` block on the Host detects the error status and Panics (`P-GPU-5001`).

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                                               | Detection    | Effect    |
| :----------- | :------- | :---------------------------------------------------------------------- | :----------- | :-------- |
| `E-GPU-4012` | Error    | Kernel stack usage exceeds hardware limit.                              | Compile-time | Rejection |
| `W-GPU-4070` | Warning  | Infinite loop detected in potentially divergent path (Warp Starvation). | Compile-time | Warning   |
| `P-GPU-5001` | Panic    | Device hardware fault (trap) detected.                                  | Runtime      | Panic     |

---