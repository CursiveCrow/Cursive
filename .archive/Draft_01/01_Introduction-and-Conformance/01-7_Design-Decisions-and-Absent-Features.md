# Cursive Language Specification

## Clause 1 — Introduction and Conformance

**Part**: I — Introduction and Conformance
**File**: 01-7_Design-Decisions-and-Absent-Features.md  
**Section**: §1.8 Design Decisions and Absent Features  
**Stable label**: [intro.absent]  
**Forward references**: Clause 10 [memory], Clause 13 [concurrency]

---

### §1.8 Design Decisions and Absent Features [intro.absent]

[1] Cursive achieves memory safety through explicit mechanisms that differ from other systems programming languages. Understanding what Cursive does NOT provide clarifies the deliberate design choices.

#### §1.8.1 No Garbage Collection [intro.absent.gc]

[2] Cursive does not employ garbage collection. Resource cleanup is deterministic via RAII (§10.2.4), occurring at statically determined program points. Complete memory management model is specified in Clause 10.

[3] Cleanup follows RAII (Resource Acquisition Is Initialization): objects are automatically destroyed at scope exit in LIFO (last-in, first-out) order. Destructors execute deterministically, releasing resources without programmer intervention.

[4] Comparison with garbage-collected languages:

**Table 1.1 — GC vs RAII comparison**

| Aspect           | Garbage Collection (Java, Go, Python) | Cursive RAII                   |
| ---------------- | ------------------------------------- | ------------------------------ |
| Cleanup timing   | Non-deterministic (GC decides)        | Deterministic (scope exit)     |
| Performance      | Unpredictable pauses                  | Bounded, predictable           |
| Resource release | Delayed (finalization queues)         | Immediate (LIFO at scope exit) |
| Real-time        | Difficult (GC pauses)                 | Natural (no pauses)            |
| Memory overhead  | GC metadata, heap pressure            | Minimal (stack/region only)    |

#### §1.8.2 No Borrow Checker [intro.absent.borrow]

[5] Cursive achieves memory safety without borrow checking or lifetime annotations. The design uses regions (§10.3), permissions (§10.4), and explicit move semantics (§10.5) instead of lifetime parameters and exclusive borrowing.

[ Rationale: Regions are concrete allocation arenas, not abstract lifetime bounds. Permissions control access without exclusive XOR mutable borrowing. Explicit move makes responsibility transfer visible. This approach provides memory safety with simpler mental model and better support for complex data structures. — end rationale ]

#### §1.8.3 No Send/Sync Markers [intro.absent.markers]

[6] Thread safety is encoded through the permission system (§10.4), not separate marker behaviors. The permission lattice (unique <: shared <: const) directly encodes thread safety properties (§13.1.2).

[7] **Permission-based thread safety:**

- **const types** are safe to share across threads (immutable, no races possible)
- **unique types** can be transferred between threads via `move` (exclusive ownership prevents races)
- **shared types** require explicit synchronization (programmer ensures coordination via Mutex, RwLock, etc.)

[ Rationale: The permission system already controls access and provides thread safety guarantees without introducing redundant markers. See §10.4 for complete permission semantics. — end rationale ]

---

**Previous**: §1.7 Versioning and Evolution (§1.7 [intro.versioning]) | **Next**: §1.9 Design Principles (§1.9 [intro.principles])

