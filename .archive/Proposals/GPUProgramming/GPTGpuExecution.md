## Clause 14.6.2 (Revised): GPU Domain — Native Heterogeneous Execution

### 14.6.2.0 Goals and Non-Goals

**Goals**

1. **One parallel model**: GPU execution is expressed using existing structured concurrency (`parallel`, `dispatch`), not a separate sublanguage.
2. **No ambient authority**: GPU resource use must be authorized by an execution-domain capability, consistent with `Context` capability passing.
3. **Race freedom by construction**: GPU global/shared memory access is statically rejected unless proven disjoint or explicitly synchronized via GPU primitives (barriers/atomics/reductions).
4. **Local reasoning**: A `dispatch` block is type-checked under a precise “Device context” rule-set; host code remains host.

**Non-goals (for this clause)**

* Cross-device multi-GPU collectives, async streams/events, unified memory/page migration. These can be layered later without changing the core safety model.

---

## 14.6.2.1 Domain Access and Availability

### Access

The GPU domain is accessed through the `Context` record’s `gpu` field, which is `GpuDomainFactory | None`.

```cursive
parallel ctx.gpu(device: d, queue: q) { ... }
```

### Availability Rule (Dynamic)

If, at runtime, `ctx.gpu` evaluates to `None`, entering a GPU parallel block MUST raise a panic (implementation-defined diagnostic code; normative behavior is that execution does not continue in the block). This matches the “capability must exist to use authority” principle while preserving the ergonomics of `parallel ctx.gpu() { ... }`.

*(Rationale: the language already uses union types for optionality; this provides a language-level, structured failure mode for “GPU absent” without introducing ambient global queries.)*

---

## 14.6.2.2 Two Contexts: Host Commander vs Device Kernel

A GPU `parallel` block establishes two distinct static contexts:

1. **Host context**: code in the `parallel ctx.gpu { ... }` body **outside** any `dispatch` executes on the CPU (the “commander”). It may perform ordinary host computation, allocate host memory, call I/O, and so on—subject to normal language rules.
2. **Device context**: code inside a `dispatch ... { ... }` nested within a GPU parallel block is executed on the GPU as a kernel.

This distinction is required for correct capture and legality rules, and avoids “GPU sublanguage leakage” into the host portion of the block.

### Nesting

* CPU↔GPU nesting is permitted.
* GPU-in-GPU nesting is prohibited (existing `E-PAR-0052`).

---

## 14.6.2.3 Authority: GPU Intrinsics are Domain-Bound, Not Ambient

The spec currently lists `gpu::alloc`, `gpu::upload`, `gpu::download`, and indexing/atomic/barrier intrinsics as “available within GPU parallel blocks”.

This clause tightens their authority semantics:

> **Rule (Domain Binding):** A `gpu::...` intrinsic is well-formed **iff** it appears within the dynamic extent of a GPU `parallel` block, and it is interpreted as using the *active GPU domain capability* established by that block.

Equivalently (desugaring model), inside:

```cursive
parallel D { body }
```

each `gpu::foo(args...)` is sugar for:

```cursive
D.gpu_foo(args...)
```

where `D` is the domain expression provided to `parallel`. This preserves “no ambient authority” while keeping surface syntax minimal.

**New diagnostic (recommended):**

* `E-PAR-0053`: `gpu::...` intrinsic used outside a GPU parallel block (compile-time rejection).

---

## 14.6.2.4 Device Memory: `GpuBuffer<T>` as the Fundamental Storage Type

### Definition (Library/Abstract)

`GpuBuffer<T>` is an owned handle to a contiguous device allocation. It is device-addressable storage; it is **not** host-dereferenceable.

### Host vs Device Dereference Rule

* Dereferencing or indexing a `GpuBuffer<T>` in a **Host context** is ill-formed (recommended diagnostic: `E-PAR-0054` “device memory access in host code”).
* Dereferencing or indexing a host pointer / host array in a **Device context** remains ill-formed (`E-PAR-0050`). 

This makes the address-space boundary symmetric and explicit (closing an ambiguity in the current text, which only names host→device misuse). 

### Allocation and Transfer

The “explicit transfer” model is retained, but its contracts are made explicit:

```cursive
parallel ctx.gpu() {
    let d_input:  GpuBuffer<f32> = gpu::alloc(n)
    let d_output: GpuBuffer<f32> = gpu::alloc(n)

    gpu::upload(host_input, d_input)
    dispatch i in 0..n { d_output[i] = d_input[i] * 2.0 }
    gpu::download(d_output, host_output)
}
```

This baseline remains the default mental model: separate memory spaces, explicit copies.

### Sequencing Semantics (Determinism + Local Reasoning)

Within a single GPU `parallel` block, GPU operations (`gpu::upload`, kernel submissions via `dispatch`, `gpu::download`) MUST be **observationally sequenced** in lexical order: effects of earlier operations are visible to later operations in the same block.

Implementations MAY pipeline/overlap internally, but must preserve this sequential semantics. This is crucial for writing correct host commander code without an additional async/event system.

At block exit, all queued GPU work in that block MUST be complete (consistent with structured concurrency’s “work settles before scope exit”).

---

## 14.6.2.5 GPU `dispatch`: Dimensions, Workgroups, and Intrinsics

### Dispatch Dimensions

Existing syntax is preserved.

```cursive
dispatch i in 0..n { ... }
dispatch (x, y) in (0..w, 0..h) { ... }
dispatch (x, y, z) in (0..w, 0..h, 0..d) { ... }
dispatch i in 0..n [workgroup: 256] { ... }
```

### Workgroup Attribute Generalization (Refinement)

To support multi-dimensional dispatch consistently:

* `[workgroup: N]` is valid for 1D dispatch.
* For 2D/3D dispatch, `[workgroup: (wx, wy)]` / `[workgroup: (wx, wy, wz)]` SHOULD be accepted (recommended) as syntactic sugar.
* If omitted, workgroup sizing is implementation-defined.

### Intrinsics Shape Rule (Refinement)

The current spec types `gpu::global_id()` / `local_id()` / `workgroup_id()` as `usize`. 
To match multi-dimensional dispatch, refine them as:

* `gpu::global_id()` returns the same “index shape” as the dispatch binder:

  * `usize` for `dispatch i ...`
  * `(usize, usize)` for `dispatch (x, y) ...`
  * `(usize, usize, usize)` for `dispatch (x, y, z) ...`

Same for `local_id()` and `workgroup_id()`.

*(This avoids forcing programmers into manual linearization and removes a semantic mismatch.)*

---

## 14.6.2.6 Device Context Constraints (Kernel Subset)

The spec already restricts captures (next section); it does not fully spell out the device-language subset. This clause makes it normative:

In **Device context**, the following are ill-formed (compile-time rejection):

1. **Host capabilities and I/O** (`ctx.fs`, `ctx.net`, filesystem/network use) — host authority is not available on device.
2. **Heap allocation** via `ctx.heap` or other host allocators (device has no host heap).
3. **`wait`** and async suspension (no device scheduler model is defined for suspension; also keys cannot be held across waits generally).
4. **Dynamic dispatch** (`dyn ...`) and other constructs requiring vtables/reflection at runtime (unless an implementation defines a device ABI for them; default is forbidden).
5. **Unbounded recursion / cyclic call graphs** (implementation-defined limits are unacceptable here; require static acyclicity for device-callable call graphs).

Additionally:

* Any operation that would panic (e.g., bounds check) in Device context MUST be treated as a kernel fault that is surfaced as a panic at the enclosing GPU parallel block boundary, consistent with parallel panic propagation semantics.

---

## 14.6.2.7 Capture Rules (Tightened)

Existing capture rules are retained: device code may capture only `GpuBuffer<T>`, primitive constants, and `const` small records; `shared` is forbidden because keys are unavailable on GPU (`E-PAR-0051`), host pointers/heap values forbidden, etc. 

### “Small record” must be specified

To make this implementable and portable:

* Define a constant limit `GPU_MAX_UNIFORM_CAPTURE_BYTES` (implementation-defined, but MUST be documented in the conformance dossier).
* Capturing a `const` record larger than this limit is ill-formed (recommended diagnostic `E-PAR-0055`).

*(This removes a major ambiguity: what counts as “small”.)*

---

## 14.6.2.8 Race Freedom on the GPU: Static Disjointness or Explicit Synchronization

GPU code cannot rely on the Key System (`shared` capture is forbidden).
Therefore, **race freedom** for device/global/shared memory is enforced in the GPU type-checker via a stricter rule:

### Rule (GPU Memory Access Safety)

Within a GPU `dispatch`:

* Writes to `GpuBuffer` (and GPU-shared allocations defined below) MUST be proven **disjoint across invocations**, or the write MUST occur through an **atomic intrinsic**, or via a structured **reduction**.
* If the compiler cannot prove disjointness and the access is not atomic/reduction, reject with `E-PAR-0042` (cross-iteration dependency). 

This leverages the existing “dispatch-indexed is statically safe” concept: accesses indexed by the dispatch iteration variable are recognized as disjoint.

### Implication: `[[dynamic]]` does not rescue GPU races

`[[dynamic]]` enables runtime synchronization for keys.
Because the GPU does not implement the key runtime, **`[[dynamic]]` MUST NOT permit overlapping non-atomic device writes** in GPU dispatch. Applying `[[dynamic]]` to a GPU dispatch may still be meaningful for *other* checks (e.g., contracts), but not for device race-freedom.

---

## 14.6.2.9 Barriers and Atomics (Normative Semantics)

The spec lists `gpu::barrier()` and atomic intrinsics, but not their memory semantics. 
This clause defines them:

### `gpu::barrier()`

* Synchronizes all invocations in the **same workgroup**.
* Acts as a memory fence for **workgroup-shared memory** and for workgroup-visible ordering of device memory operations (exact fence strength is implementation-defined but MUST be at least acquire+release for workgroup scope).

### Atomics

`gpu::atomic_add/min/max/cas`:

* Operate atomically on the addressed location.
* Default ordering is sequential consistency with respect to other GPU atomics on the same address (matching the language’s general “seq_cst by default” philosophy).
* Are treated by the static race checker as *explicit synchronization*, permitting otherwise-overlapping updates.

*(Optional future extension: allow ordering attributes analogous to `[[relaxed]]`, etc., but scoped to GPU atomics rather than `shared` keys.)*

---

## 14.6.2.10 Workgroup Shared Memory via Regions (GPU-Scoped Region Option)

Cursive already has a `region ... as r { ... }` construct and `^` allocator for region-local storage.
We extend **region options** with a GPU-only variant:

```ebnf
<region_option> ::= ... | "." "gpu_shared" "(" <const_expr_bytes> ")"
```

### Semantics

Inside a **GPU dispatch** only:

```cursive
dispatch i in 0..n [workgroup: 256] {
    region .gpu_shared(1024) as smem {
        let tile: [f32; 256] = smem^zeroed()
        // ...
        gpu::barrier()
        // ...
    }
}
```

* `.gpu_shared(N)` creates a workgroup-shared allocation arena of exactly `N` bytes for that workgroup.
* `N` MUST be a compile-time constant expression.
* The implementation MUST reject launches where `N` exceeds the device’s per-workgroup shared-memory limit (panic at launch boundary; IDB diagnostic code).
* Values allocated in a `.gpu_shared` region MUST NOT escape the dispatch body (compile-time rejection).

This preserves the language’s existing region vocabulary while making GPU shared memory explicit and statically bounded.

---

## 14.6.2.11 Performance Tuning Without New Grammar: Comptime Kernel Generation

To avoid introducing a new `schedule` keyword/DSL, scheduling/tuning is expressed via **compile-time generation and attributes** (consistent with the attribute system and metaprogramming philosophy).

### Pattern: Generate Variants at Comptime

```cursive
comptime {
    for wg in [64, 128, 256] {
        emit procedure vec_scale_wg_${wg}(
            d_in:  GpuBuffer<f32>,
            d_out: GpuBuffer<f32>,
            a: f32,
            n: usize,
        ) {
            parallel ctx.gpu() {
                dispatch i in 0..n [workgroup: wg] {
                    d_out[i] = d_in[i] * a
                }
            }
        }
    }
}
```

A separate (ordinary) host function selects among generated variants by device limits, problem size, or a cached tuning table.

### Optional Attribute: `[[gpu::autotune(... )]]`

If desired, autotuning can be specified as an attribute recognized by tooling (not new grammar). Unknown attributes must be diagnosed per the attribute registry rules. 

---

## 14.6.2.12 Worked Examples

### A. Minimal “change CPU→GPU” shape

```cursive
procedure saxpy(ctx: Context, x: const [f32], y: const [f32], a: f32) -> [f32] {
    let n = x.len()
    let out: [f32; n]

    parallel ctx.gpu() {
        let dx = gpu::alloc<f32>(n)
        let dy = gpu::alloc<f32>(n)
        let dout = gpu::alloc<f32>(n)

        gpu::upload(x, dx)
        gpu::upload(y, dy)

        dispatch i in 0..n {
            dout[i] = a * dx[i] + dy[i]
        }

        gpu::download(dout, out)
    }

    result out
}
```

### B. Tiled access with `.gpu_shared`

```cursive
parallel ctx.gpu() {
    // d_in, d_out already allocated and filled...
    dispatch i in 0..n [workgroup: 256] {
        region .gpu_shared(256 * size_of::<f32>()) as smem {
            let tile: [f32; 256] = smem^zeroed()

            let t = gpu::local_id()          // 0..255
            let g = gpu::global_id()         // global index

            tile[t] = d_in[g]
            gpu::barrier()

            // safe: disjoint write to d_out[g]
            d_out[g] = tile[(t + 1) % 256]   // read-only reuse
        }
    }
}
```

---

## Summary of What This Rewrite Fixes

* **Authority**: `gpu::` operations are explicitly *domain-bound sugar*, satisfying “no ambient authority” instead of relying on an implicit global GPU context.
* **Host/device boundary**: makes dereference legality symmetric (host can’t read device, device can’t read host) and clarifies the two contexts inside a GPU parallel block. 
* **Kernel subset**: device restrictions are normative (no `wait`, no host capabilities, no heap, no dyn dispatch, no recursion cycles).
* **Race model**: replaces “keys on GPU” with a strict, static “disjoint or atomic/reduce” rule; rejects ambiguous overlap with `E-PAR-0042`.
* **Shared memory**: integrates GPU shared memory via a *region option* consistent with existing region syntax, with compile-time sizing and launch-time limit checks.
* **Scheduling/tuning**: removes the need for a new scheduling DSL by using comptime variant generation and attributes. 

If you want, I can now take this and rewrite the existing **§14.6.2** section directly as a patch (diff-style) against your `CursiveLanguageSpecification.md`, keeping numbering/diagnostic tables consistent.
