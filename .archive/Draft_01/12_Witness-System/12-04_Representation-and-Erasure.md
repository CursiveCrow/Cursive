# Cursive Language Specification

## Clause 12 — Witness System

**Clause**: 12 — Witness System
**File**: 13-4_Representation-and-Erasure.md
**Section**: §12.4 Representation and Erasure
**Stable label**: [witness.representation]  
**Forward references**: §13.5 [witness.dispatch], Clause 7 §7.8 [type.introspection], Clause 11 §11.6 [memory.layout]

---

### §12.4 Representation and Erasure [witness.representation]

#### §12.4.1 Overview

[1] This subclause specifies the memory representation of witness values, the structure of witness tables (vtables and metadata), size and alignment properties, and the conditions under which witnesses can be erased or optimized.

[2] Witnesses use _dense pointer_ representation: a pair of sparse pointers (data pointer and metadata pointer) occupying 16 bytes on 64-bit platforms. The dense pointer enables type-safe dynamic dispatch with minimal overhead.

#### §12.4.2 Dense Pointer Structure [witness.representation.dense]

##### §12.4.2.1 Memory Layout

[3] A witness value has the following memory layout:

```
witness<B>@State = {
    data_ptr: Ptr<T>@Valid,          // 8 bytes: pointer to concrete value
    metadata_ptr: Ptr<WitnessTable>, // 8 bytes: pointer to witness metadata
}
```

[4] **Size and alignment**:

$$
\text{sizeof}(\texttt{witness}\langle B \rangle@S) = 16 \text{ bytes (64-bit platforms)}
$$

$$
\text{alignof}(\texttt{witness}\langle B \rangle@S) = 8 \text{ bytes (pointer alignment)}
$$

[5] On 32-bit platforms: `sizeof(witness<B>) = 8 bytes`, `alignof = 4 bytes`.

[6] Implementations shall document platform-specific witness sizes and ensure they match two-pointer size for the target architecture.

##### §12.4.2.2 Data Pointer

[7] The data pointer references the concrete value. It has type `Ptr<T>@Valid` where `T` is the concrete implementing type (erased from the witness type signature).

[8] **Permission tracking**: The data pointer carries the permission inherited from the source value:

- const witness → `const Ptr<T>`
- unique witness → `unique Ptr<T>`
- shared witness → `shared Ptr<T>`

##### §12.4.2.3 Metadata Pointer

[9] The metadata pointer references a witness table containing type information and dispatch functions. The witness table structure is:

```
WitnessTable = {
    type_id: TypeId,                      // Runtime type identification
    size: usize,                          // Size of concrete type
    align: usize,                         // Alignment of concrete type
    state_tag: Option<StateDiscriminant>, // For modal witnesses
    drop: Option<fn(*mut ())>,           // Destructor (if needed)
    vtable: [ProcedurePtr],              // Method dispatch table
}
```

[10] **VTable entries**: One function pointer per behavior/contract procedure. For modal witnesses, vtable contains state-specific procedures.

[11] **State tag**: Modal witnesses (`witness<T@State>`) store the state discriminant in `state_tag`. Non-modal witnesses have `state_tag = None`.

#### §12.4.3 VTable Structure [witness.representation.vtable]

##### §12.4.3.1 Behavior/Contract VTable

[12] For behavior witness `witness<Display>` where Display has procedures `show` and `debug`:

```
VTable_Display = {
    show: fn(*const ()) -> string@View,
    debug: fn(*const ()) -> string@Managed,
}
```

[13] Each vtable entry is a function pointer with signature matching the behavior procedure's signature, with receiver replaced by sparse pointer `*const ()` (type-erased).

##### §12.4.3.2 Modal State VTable

[14] For modal witness `witness<FileHandle@Open>`:

```
VTable_FileHandle_Open = {
    read: fn(*shared (), buffer: [u8]) -> FileHandle@Open,
    close: fn(*unique ()) -> FileHandle@Closed,
    // Only @Open state procedures
}
```

[15] Modal witness vtables contain only procedures available in the specified state.

#### §12.4.4 Witness Table Sharing [witness.representation.sharing]

[16] Implementations may share witness tables across instances:

- All witnesses of the same concrete type implementing the same behavior share one witness table
- Witness tables are compile-time constants (allocated in static storage)
- Sharing reduces memory overhead for multiple witnesses

[17] **Sharing rule**:

$$
\frac{\texttt{witness}\langle B \rangle@S_1 \text{ from } T \quad \texttt{witness}\langle B \rangle@S_2 \text{ from } T}
     {\text{Both share } \texttt{WitnessTable}_B^T}
\tag{WitnessTable-Share}
$$

[18] Allocation states (@Stack/@Region/@Heap) do not affect witness table sharing. The vtable is the same regardless of allocation strategy.

#### §12.4.5 Type Erasure [witness.representation.erasure]

##### §12.4.5.1 Erasure Semantics

[19] Witnesses perform _type erasure_: the concrete type `T` is not part of the witness type signature. The witness type `witness<Display>` can hold values of any type implementing Display.

[20] **Type erasure rule**:

$$
\frac{T_1 : B \quad T_2 : B}
     {\texttt{witness}\langle B \rangle \text{ can hold both } T_1 \text{ and } T_2}
\tag{Witness-Erasure}
$$

[21] Heterogeneous collections use type erasure:

**Example 13.4.5.1 (Type erasure enabling heterogeneous collections)**:

```cursive
let point: witness<Display> = Point { x: 1.0, y: 2.0 }
let circle: witness<Display> = Circle { radius: 5.0 }
let rectangle: witness<Display> = Rectangle { width: 2.0, height: 3.0 }

// All have type witness<Display> despite different concrete types
let shapes: [witness<Display>] = [point, circle, rectangle]
```

##### §12.4.5.2 Type Recovery

[22] The concrete type can be recovered at runtime via pattern matching or type introspection (§7.8):

```cursive
match witness {
    w if w.type_id() == type_id<Point>() => {
        // Dynamically verified: witness contains Point
    }
    _ => { }
}
```

[23] Complete type recovery mechanisms and safe downcasting patterns are implementation-defined extensions. The core witness system provides type erasure; recovery is optional.

#### §12.4.6 Witness Optimization and Erasure [witness.representation.optimization]

##### §12.4.6.1 Monomorphization Erasure

[24] When a witness-using procedure is monomorphized with a known concrete type, implementations may erase the witness and use direct dispatch:

**Example 13.4.6.1 (Witness erasure optimization)**:

```cursive
procedure show_witness(w: witness<Display>)
    [[ io::write |- true => true ]]
{
    w::show()
}

// If called with only Point:
show_witness(point_witness)
show_witness(another_point_witness)

// Compiler MAY optimize to:
procedure show_witness_Point(p: Point) {
    p::show()  // Direct call, no vtable
}
```

[25] This optimization is permitted when all call sites use the same concrete type. It does not affect observable behavior (only performance).

##### §12.4.6.2 Erasure Limitations

[26] Witnesses stored in fields, returned from procedures, or appearing in truly heterogeneous collections cannot be erased. The witness representation must be preserved.

[27] Erasure is a quality-of-implementation optimization. Conforming programs shall not depend on whether erasure occurs; behavior must be identical with or without optimization.

#### §12.4.7 Size and Alignment [witness.representation.size]

[28] **Size properties**:

$$
\text{sizeof}(\texttt{witness}\langle B \rangle@S) = 2 \times \text{sizeof}(\texttt{Ptr}\langle T \rangle)
\tag{Witness-Size}
$$

[29] **Alignment properties**:

$$
\text{alignof}(\texttt{witness}\langle B \rangle@S) = \text{alignof}(\texttt{Ptr}\langle T \rangle)
\tag{Witness-Align}
$$

[30] Witnesses have uniform size regardless of the concrete type they contain. A `witness<Display>` holding a Point has the same size as a `witness<Display>` holding a Circle.

#### §12.4.8 Conformance Requirements [witness.representation.requirements]

[31] Implementations shall:

1. Represent witnesses as dense pointers (data pointer + metadata pointer)
2. Ensure witness size equals two sparse pointer sizes
3. Align witnesses to pointer alignment
4. Implement witness tables with type ID, size, alignment, optional state tag, drop function, and vtable
5. Share witness tables across instances of same type/behavior combination
6. Allocate witness tables in static storage
7. Support type erasure: witness type hides concrete type
8. May optimize witnesses to direct dispatch when monomorphizable
9. Preserve witness representation when needed (heterogeneous collections)
10. Document platform-specific sizes (32-bit vs 64-bit)

---

**Previous**: §13.3 Formation and Construction (§13.3 [witness.formation]) | **Next**: §13.5 Dispatch Semantics (§13.5 [witness.dispatch])
