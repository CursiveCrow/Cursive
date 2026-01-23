# Cursive Language Specification

## Clause 12 — Witness System

**Clause**: 12 — Witness System
**File**: 13-01_Overview-and-Purpose.md
**Section**: §12.1 Overview and Purpose
**Stable label**: [witness.overview]  
**Forward references**: §12.2 [witness.kind], §12.3 [witness.formation], §12.4 [witness.representation], §12.5 [witness.dispatch], §12.6 [witness.memory], §12.7 [witness.grammar], §12.8 [witness.diagnostics], Clause 9 [generic], Clause 11 [contract]

---

### §12.1 Overview and Purpose [witness.overview]

#### §12.1.1 Overview

[1] The _witness system_ provides runtime evidence of type-level properties, enabling dynamic polymorphism and runtime verification as an explicit opt-in feature. A witness is a dense pointer (multi-component pointer) that packages a value with proof of a property: behavior implementation, contract satisfaction, or modal state membership. The witness system integrates behaviors (Clause 9), contracts (Clause 11), and modal types (§6.6) into a unified runtime evidence mechanism. Static dispatch through monomorphization remains the zero-cost default; witnesses are an explicit opt-in for scenarios requiring runtime flexibility.

#### §12.1.2 Static vs Dynamic Verification [witness.overview.static]

[2] Cursive provides two verification strategies for type-level properties:

**Static verification** (default, zero-cost):

- Behavior implementation: Generic bounds `<T: Display>`
- Contract satisfaction: Generic bounds `<T: Serializable>`
- Modal states: Type system tracking `FileHandle@Open`
- Compile-time proof, monomorphization, no runtime overhead

**Dynamic verification** (opt-in, explicit cost):

- Behavior implementation: Witness type `witness<Display>`
- Contract satisfaction: Witness type `witness<Serializable>`
- Modal states: Witness type `witness<FileHandle@Open>`
- Runtime evidence via dense pointers, vtable dispatch, small overhead

[2] Table 12.1 compares the two strategies:

**Table 12.1 — Static vs dynamic verification**

| Aspect      | Static (Monomorphization)   | Dynamic (Witnesses)                  |
| ----------- | --------------------------- | ------------------------------------ |
| Syntax      | Generic bounds `<T: B>`     | Witness type `witness<B>`            |
| Type check  | Compile-time                | Runtime                              |
| Dispatch    | Direct call (inlined)       | Vtable indirect call                 |
| Memory      | Zero overhead               | 16 bytes per witness (dense pointer) |
| Flexibility | Homogeneous collections     | Heterogeneous collections            |
| Cost        | Zero                        | Small (one indirection)              |
| Use when    | Types known at compile time | Types determined at runtime          |

[3] The witness system makes the cost of dynamic dispatch **explicit**: the `witness<B>` type in signatures clearly indicates runtime polymorphism.

#### §12.1.3 Witness as Modal Type [witness.overview.modal]

[4] The `witness` type is a built-in modal type with three allocation states:

```cursive
modal witness<B> {
    @Stack   // Non-responsible reference to stack value
    @Region  // Responsible region-allocated value
    @Heap    // Responsible heap-allocated value
}
```

[5] Default: `witness<B>` without state annotation means `witness<B>@Stack` (conservative, local reference). Allocation states determine cleanup responsibility:

- `@Stack`: Non-responsible binding (like `let x <- value`), does not call destructor
- `@Region`: Responsible binding (like `let x = move value`), calls destructor at region exit
- `@Heap`: Responsible binding (like `let x = move value`), calls destructor at scope exit

[6] Transitions between states follow the chain: `@Stack → @Region → @Heap`, each transition requiring explicit method calls and appropriate grants.

#### §12.1.4 Dense Pointer Representation [witness.overview.representation]

[7] Witnesses use _dense pointer_ representation: a multi-component pointer containing both data and metadata. Dense pointers contrast with _sparse pointers_ (single-component pointers like `Ptr<T>`). A witness value contains:

```
witness<B> = {
    data_ptr: Ptr<T>,            // Pointer to concrete value (8 bytes)
    metadata: Ptr<WitnessTable>  // Pointer to witness metadata (8 bytes)
}

WitnessTable = {
    type_id: TypeId,                // Runtime type identification
    state_tag: Option<StateTag>,    // For modal witnesses
    vtable: [fn(*const ()) -> ...], // Method dispatch table
    drop: Option<fn(*mut ())>       // Destructor function
}
```

[8] Total size: 16 bytes on 64-bit platforms. The dense pointer representation enables type-safe dynamic dispatch with minimal overhead.

#### §12.1.5 Unified Witness Concept [witness.overview.unified]

[9] The witness system provides a **unified mechanism** for runtime evidence of different type-level properties:

**Behavior witnesses** (`witness<Display>`):

- Runtime evidence that a value satisfies behavior Display
- VTable contains pointers to behavior procedure implementations
- Enables heterogeneous collections of different types with common behavior

**Contract witnesses** (`witness<Serializable>`):

- Runtime evidence that a value satisfies contract Serializable
- VTable contains pointers to contract procedure implementations
- Same representation as behavior witnesses (behaviors and contracts distinguished at compile time)

**Modal state witnesses** (`witness<FileHandle@Open>`):

- Runtime evidence that a modal value is in state @Open
- State tag stored in witness metadata
- Enables dynamic state tracking when compile-time tracking insufficient

[10] All witness kinds use the same dense pointer representation and dispatch mechanism. The property in angle brackets (`<Display>`, `<Serializable>`, `<FileHandle@Open>`) determines what is being witnessed.

#### §12.1.6 Design Philosophy [witness.overview.philosophy]

[11] The witness system embodies Cursive's design principles (§1.9): explicit over implicit (dynamic dispatch is opt-in via explicit `witness<B>` types), zero-cost abstraction default (static dispatch is default), and local reasoning (witness types show complete dispatch strategy).

#### §12.1.7 Conformance Requirements [witness.overview.requirements]

[12] Implementations shall maintain witness as opt-in feature with zero cost when unused, and integrate witness cleanup responsibility with RAII (§10.2).

---

**Previous**: Clause 11 — Contracts (§11.10 [contract.diagnostics]) | **Next**: §12.2 Witness Kinds (§12.2 [witness.kind])
