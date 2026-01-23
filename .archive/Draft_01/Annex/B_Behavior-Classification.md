# Cursive Language Specification

## Annex B — Behavior Classification (Normative)

**Part**: Annex
**Section**: Annex B - Behavior Classification
**Stable label**: [behavior]
**Forward references**: None

---

### §B.1 Implementation-Defined Behavior Catalog [behavior.implementation]

[1] This annex enumerates all behaviors for which this specification permits multiple valid implementations. Each entry identifies the clause where the behavior is described, enumerates the permitted choices, and specifies the documentation obligations imposed on conforming implementations.

[2] Conforming implementations shall document their choices for each implementation-defined behavior listed below.

#### §B.1.1 Numeric Representation

**B.1.01 — Integer Type Widths Beyond Mandated Minima**

- **Description**: While this specification mandates minimum sizes for integer types (Table 7.2.1), implementations may provide larger representations on platforms where doing so improves performance or matches platform conventions.
- **Permitted Choices**: Implementations may use wider representations provided the value sets remain compatible.
- **References**: §7.2.2 [type.primitive.int], §11.6 [memory.layout]
- **Documentation Required**: Actual sizes for `i128`/`u128` and pointer-width types (`isize`/`usize`) on each target platform.

**B.1.02 — 128-bit Integer Alignment**

- **Description**: Alignment for `i128` and `u128` types.
- **Permitted Choices**: 8 bytes or 16 bytes alignment.
- **References**: §7.2.2, Table 7.2.1
- **Documentation Required**: Whether implementation uses 8 or 16-byte alignment for 128-bit integers.

**B.1.03 — Floating-Point Subnormal Handling**

- **Description**: Treatment of subnormal (denormalized) floating-point numbers.
- **Permitted Choices**: Full IEEE 754 subnormal support, or flush-to-zero.
- **References**: §7.2.3 [type.primitive.float]
- **Documentation Required**: Whether subnormals are supported or flushed to zero.

**B.1.04 — Endianness**

- **Description**: Byte order for multi-byte values.
- **Permitted Choices**: Little-endian, big-endian, or bi-endian (runtime selectable).
- **References**: §7.2.2 [type.primitive.int], §15.6.4
- **Documentation Required**: Endianness for each target platform.

#### §B.1.2 Memory and Layout

**B.1.05 — Pointer Size and Alignment**

- **Description**: Size and alignment of pointer types and `usize`/`isize`.
- **Permitted Choices**: 4 bytes (32-bit), 8 bytes (64-bit), or other platform-specific widths.
- **References**: §7.5 [type.pointer], Table 11.6
- **Documentation Required**: Pointer size for each target architecture.

**B.1.06 — Native Type Layout**

- **Description**: Memory layout for types without `[[repr(C)]]` or other repr attributes.
- **Permitted Choices**: Implementations may reorder fields, insert padding, or optimize layout.
- **References**: §11.6 [memory.layout]
- **Documentation Required**: Whether implementation optimizes layout or preserves declaration order for native types.

**B.1.07 — Maximum Source File Size**

- **Description**: Maximum size of source files accepted by the compiler.
- **Permitted Choices**: At least 1 MiB; implementations may support larger files.
- **References**: §2.1.3[5]
- **Documentation Required**: Actual maximum file size supported.

**B.1.08 — Maximum Stack Size**

- **Description**: Size of procedure call stack.
- **Permitted Choices**: Platform-dependent, typically 1-8 MiB.
- **References**: §10.2.3 [memory.object.storage]
- **Documentation Required**: Stack size limits and whether they are configurable.

#### §B.1.3 Translation Limits

**B.1.09 — Comptime Resource Limits**

- **Description**: Resource limits for compile-time evaluation beyond mandated minima.
- **Permitted Choices**: May exceed minimums specified in §2.2.3 (256 recursion depth, 1M steps, 64 MiB memory).
- **References**: §2.2.3, §16.2.3.3
- **Documentation Required**: Actual resource limits for comptime evaluation.

**B.1.10 — Monomorphization Limits**

- **Description**: Maximum number of generic instantiations.
- **Permitted Choices**: At least 1024 instantiations per generic entity.
- **References**: §10.6.9.3
- **Documentation Required**: Actual monomorphization limits.

**B.1.11 — Delimiter Nesting Depth**

- **Description**: Maximum nesting level for delimiters (parentheses, brackets, braces).
- **Permitted Choices**: At least 256 levels.
- **References**: §2.4.3[3.1]
- **Documentation Required**: Actual maximum nesting depth.

#### §B.1.4 Diagnostic Formatting

**B.1.12 — Diagnostic Display Format**

- **Description**: Presentation style for diagnostic messages.
- **Permitted Choices**: Implementations may use color, Unicode box-drawing, ASCII-only, or custom formats.
- **References**: §1.5.5 [intro.conformance.diagnostics], Annex E §E.5.6
- **Documentation Required**: Supported output formats and default style.

**B.1.13 — Diagnostic Payload Schema**

- **Description**: Machine-readable diagnostic output schema details.
- **Permitted Choices**: JSON Lines (required minimum), plus optionally XML, Protocol Buffers, or custom formats.
- **References**: Annex E §E.5.6
- **Documentation Required**: Supported schema formats.

#### §B.1.5 ABI and Interoperability

**B.1.14 — Name Mangling Scheme**

- **Description**: Symbol name mangling for external linkage entities.
- **Permitted Choices**: Implementation-defined mangling scheme or no mangling.
- **References**: §15.5.6 [interop.linkage]
- **Documentation Required**: Complete mangling algorithm or statement of no-mangling policy.

**B.1.15 — Calling Convention Details**

- **Description**: Exact register allocation and stack frame layout for each supported calling convention.
- **Permitted Choices**: Must follow platform ABI specifications (System V AMD64, Microsoft x64, ARM AAPCS).
- **References**: §15.6.2 [interop.abi], §15.6.5
- **Documentation Required**: Calling convention details for each supported platform.

**B.1.16 — Binary Compatibility Policy**

- **Description**: Whether implementation provides binary compatibility across versions.
- **Permitted Choices**: No guarantee (source-level only) or explicit ABI stability guarantees.
- **References**: §15.7.2 [interop.compatibility]
- **Documentation Required**: ABI stability policy and versioning scheme if applicable.

#### §B.1.6 Platform-Specific

**B.1.17 — Unicode Normalization Form**

- **Description**: Which Unicode normalization form is used for source text.
- **Permitted Choices**: NFC, NFD, NFKC, or NFKD.
- **References**: §2.1.4[2]
- **Documentation Required**: Normalization form used and whether conversions are performed.

**B.1.18 — Panic Unwinding Support**

- **Description**: Whether panic unwinding is supported or abort-only.
- **Permitted Choices**: Full unwinding, abort-on-panic, or mode-selectable.
- **References**: §11.2.5.5 [memory.object.destructor.panic]
- **Documentation Required**: Unwinding support and configuration options.

**B.1.19 — Weak Symbol Support**

- **Description**: Whether weak symbols are supported on the target platform.
- **Permitted Choices**: Full support, partial support, or unsupported.
- **References**: §15.5.7 [interop.linkage]
- **Documentation Required**: Which platforms support weak symbols and any limitations.

**B.1.20 — Default Verification Mode**

- **Description**: Default contract verification strategy when no `[[verify(...)]]` attribute is present.
- **Permitted Choices**: Implementation-defined mapping of build modes to verification strategies.
- **References**: §12.8.7, Table 12.3
- **Documentation Required**: How `--build=debug` and `--build=release` map to verification modes.

[3] Implementations shall publish a conformance document listing their choices for each implementation-defined behavior above. The document shall be publicly available and included with compiler distribution.

---

### §B.2 Undefined Behavior Catalog [behavior.undefined]

[2] This annex provides the authoritative catalog of undefined behaviors (UB-IDs) referenced throughout the specification. Each entry includes the UB identifier, a complete description, the operations that trigger it, detection feasibility, and references to clauses where it is mentioned.

[3] **Warning**: Programs exhibiting undefined behavior cease to be conforming. Implementations may produce any result once undefined behavior is triggered, including but not limited to: program crashes, data corruption, security vulnerabilities, or apparently correct behavior. Programmers must avoid all undefined behavior; tools may assist but cannot guarantee detection in all cases.

#### §B.2.1 Memory Safety Violations

**B.2.01 — Access to Object Outside Its Lifetime**

- **Description**: Accessing an object through any means (dereferencing a dangling pointer, using a moved-from binding, reading destroyed storage, accessing region-allocated memory after region exit) after the object's lifetime has ended or before the object's lifetime has begun.
- **Triggering Operations**:
  - Dereferencing a pointer after the pointed-to object has been destroyed
  - Using a binding after it has been moved
  - Accessing region-allocated values after the region has exited
  - Reading from uninitialized storage (also covered by B.2.02)
- **Detection**: Prevented by region escape analysis (§11.3.3), move checking (§11.5), and definite assignment (§5.7) in safe code. May occur in unsafe code or through FFI.
- **References**: §11.2, §11.5, §11.3, Clause 15
- **Consequences**: Memory corruption, crashes, security vulnerabilities

**B.2.02 — Read of Uninitialized Memory**

- **Description**: Reading the value of an object that has allocated storage but has not been initialized.
- **Triggering Operations**:
  - Reading a variable before assignment
  - Dereferencing a pointer to uninitialized memory
  - Accessing uninitialized array elements
- **Detection**: Prevented by definite assignment analysis (§5.7) in safe code. May occur in unsafe code when reading raw pointers.
- **References**: §5.7 [decl.initialization]
- **Consequences**: Reading indeterminate values, potential security issues if uninitialized memory contains sensitive data

**B.2.03 — Double Free**

- **Description**: Destroying an object multiple times or freeing memory that has already been freed.
- **Triggering Operations**:
  - Multiple responsible bindings to the same object (prevented in safe code)
  - Manual free of region-allocated memory
  - FFI interaction where both Cursive and foreign code free the same memory
- **Detection**: Prevented by cleanup responsibility tracking in safe code. May occur in unsafe code or FFI.
- **References**: §11.2.2, §11.5, Clause 15
- **Consequences**: Heap corruption, crashes, security vulnerabilities

**B.2.04 — Null Pointer Dereference**

- **Description**: Dereferencing a raw null pointer.
- **Triggering Operations**:
  - Dereferencing `*const T` or `*mut T` when the pointer is null
  - Calling foreign functions with null pointers when they expect non-null
- **Detection**: Prevented for safe pointers (`Ptr<T>@Valid` cannot be null). May occur with raw pointers in unsafe code or FFI.
- **References**: §7.5.5 [type.pointer.raw], §15.2.5
- **Consequences**: Segmentation fault, crash

#### §B.2.2 Alignment and Layout Violations

**B.2.10 — Misaligned Object Access**

- **Description**: Accessing an object at an address that does not satisfy the type's alignment requirement on platforms with strict alignment enforcement.
- **Triggering Operations**:
  - Casting pointer to type with stricter alignment requirement
  - Using `[[repr(packed)]]` and accessing fields without unaligned-load intrinsics
  - FFI passing misaligned pointers to functions expecting alignment
- **Detection**: Platform-dependent. Some platforms (ARM) fault on misalignment; others (x86-64) tolerate with performance penalty.
- **References**: §11.6 [memory.layout], §15.2.2.2
- **Consequences**: Hardware fault, performance degradation, or silent data corruption depending on platform

**B.2.11 — Invalid Type Representation**

- **Description**: Reading a value whose bit pattern is not valid for its type.
- **Triggering Operations**:
  - Transmuting to type with invalid bit pattern
  - Reading uninitialized bool (value other than 0 or 1)
  - Reading invalid enum discriminant
  - Foreign code providing invalid values via FFI
- **Detection**: Compiler cannot detect; programmer responsibility in unsafe code.
- **References**: §11.2.2 [memory.object.model], §11.8.3.3
- **Consequences**: Logic errors, potential exploits, type confusion

#### §B.2.3 Type System Violations

**B.2.15 — Type Confusion**

- **Description**: Treating a value of one type as if it were another incompatible type.
- **Triggering Operations**:
  - Invalid transmute between incompatible types
  - FFI providing value of different type than declared
  - Unsafe pointer cast followed by dereference
- **Detection**: Type system prevents in safe code. May occur in unsafe code or FFI.
- **References**: §7.5.5, §11.8, Clause 15
- **Consequences**: Memory corruption, security vulnerabilities, crashes

#### §B.2.4 Unsafe Operation Violations

**B.2.25 — Invalid Transmute**

- **Description**: Using transmute with incompatible sizes or interpreting bit patterns that are invalid for the target type.
- **Triggering Operations**:
  - `transmute::<From, To>(value)` where `sizeof(From) != sizeof(To)`
  - Transmuting to type where bit pattern is invalid
- **Detection**: Size mismatch detected at compile time. Bit pattern validity is programmer responsibility.
- **References**: §11.8.3.3[10-11]
- **Consequences**: Memory corruption if sizes differ; logic errors if bit pattern invalid

**B.2.26 — Raw Pointer Arithmetic Overflow**

- **Description**: Pointer arithmetic producing address outside valid memory or wrapping around address space.
- **Triggering Operations**:
  - `ptr.offset(n)` where `n` moves pointer outside valid allocation
  - Pointer arithmetic wrapping address space
- **Detection**: Not detected; programmer responsibility in unsafe code.
- **References**: §7.5.5 [type.pointer.raw], §11.8
- **Consequences**: Accessing invalid memory, crashes, security issues

#### §B.2.5 Concurrency Violations

**B.2.50 — Data Race**

- **Description**: Concurrent access to the same memory location where at least one access is a write and the accesses are not ordered by synchronization (happens-before relationship).
- **Triggering Operations**:
  - Multiple threads accessing shared data without synchronization
  - Missing mutex protection for shared mutable data
  - Incorrect atomic ordering allowing races
- **Detection**: Prevented by permission system in safe code (`const` for sharing, `unique` for exclusive access, `shared` requires synchronization). May occur in unsafe code or with incorrect synchronization.
- **References**: §14.2.2[3-5], §14.2.5, §14.1.2
- **Consequences**: Non-deterministic behavior, data corruption, crashes

**B.2.51 — Misaligned Atomic Operation**

- **Description**: Performing atomic operation on a location that does not satisfy natural alignment for the type.
- **Triggering Operations**:
  - Atomic load/store on misaligned address
  - Using atomic operations on packed structures without alignment guarantee
- **Detection**: Platform-dependent. Compiler cannot verify alignment of all atomic operation targets.
- **References**: §14.3.4[4], §14.3.7
- **Consequences**: Undefined behavior on most platforms; may cause incorrect results or crashes

**B.2.52 — Dropping Locked Mutex**

- **Description**: Destroying a Mutex value while in @Locked state (mutex dropped without unlocking).
- **Triggering Operations**:
  - Scope exit while Mutex is @Locked
  - Explicit drop call on locked mutex
- **Detection**: Type system prevents in safe code (RAII guard pattern). Possible with unsafe state manipulation.
- **References**: §14.4.5[4], §14.4.2
- **Consequences**: Deadlock (other threads waiting for unlock), resource leak

#### §B.2.6 FFI and Interoperability Violations

**B.2.53 — Calling Convention Mismatch**

- **Description**: Calling a foreign function with a calling convention different from what the function expects.
- **Triggering Operations**:
  - Declaring extern procedure with wrong calling convention
  - Passing Cursive callback with wrong convention to C
- **Detection**: Compiler generates code for declared convention but cannot verify foreign code matches.
- **References**: §15.2.6, §15.6.2
- **Consequences**: Stack corruption, incorrect argument values, crashes

**B.2.54 — One Definition Rule Violation**

- **Description**: Multiple conflicting definitions of the same entity with external linkage.
- **Triggering Operations**:
  - Same type defined differently in multiple compilation units
  - Same procedure defined with different implementations across units
- **Detection**: May be caught at link time; undetected violations cause UB.
- **References**: §15.5.5 [interop.linkage]
- **Consequences**: Linking errors or runtime instability with inconsistent definitions

#### §B.2.7 Additional Violations

**B.2.60 — Arithmetic Overflow in Unchecked Mode**

- **Description**: Integer arithmetic overflow in release/unchecked build mode.
- **Triggering Operations**:
  - Addition, subtraction, multiplication producing value outside type's range
  - Negating minimum signed integer
- **Detection**: In checked/debug mode, panics. In release mode with wrapping semantics, wraps (defined behavior). In unchecked optimization mode, undefined.
- **References**: §7.2.2 [type.primitive.int], §8.3.4
- **Consequences**: Incorrect results, potential security issues

**B.2.61 — Stack Overflow**

- **Description**: Exhausting the call stack through excessive recursion or large stack allocations.
- **Triggering Operations**:
  - Deep recursion exceeding stack size
  - Large automatic storage allocations
- **Detection**: May be caught by guard pages (stack overflow signal). Not prevented at compile time.
- **References**: §11.2.3 [memory.object.storage], §10.6 (recursive procedures)
- **Consequences**: Crash, stack corruption

**B.2.62 — Foreign Function Safety Violation**

- **Description**: Foreign function violating Cursive safety invariants (modifying const data, creating data races, corrupting memory).
- **Triggering Operations**:
  - Calling foreign function that violates documented contract
  - Foreign function modifying data through const pointer
  - Foreign function creating race conditions
- **Detection**: Cannot be detected; trust placed in foreign code via contracts.
- **References**: Clause 15 [interop], §15.1.10
- **Consequences**: Violates Cursive safety guarantees; any consequence possible

[4] **Normative Requirement**: Conforming programs shall not rely on undefined behavior. Implementations are not required to diagnose undefined behavior but may provide optional detection mechanisms (sanitizers, runtime checks) as quality-of-implementation features.

[5] **Informative Note**: The list above enumerates known sources of undefined behavior in Cursive. Implementations discovering additional undefined behaviors shall report them to the working group for inclusion in future specification revisions.

---

### §B.3 Unspecified Behavior Catalog [behavior.unspecified]

[6] This annex enumerates behaviors for which this specification permits multiple outcomes without requiring implementation documentation. Unlike implementation-defined behavior (§B.1), implementations need not document which choice they make for unspecified behaviors.

[7] **Normative Requirement**: Each outcome permitted for an unspecified behavior shall be well-defined (not undefined behavior). Programs may observe different outcomes across implementations or executions, but all outcomes shall be valid according to this specification.

#### §B.3.1 Padding Byte Values

**B.3.01 — Structure Padding Byte Values**

- **Description**: The values of padding bytes inserted between fields or at the end of structures to satisfy alignment requirements.
- **Permitted Outcomes**: Padding bytes may have any value (zero, garbage, deterministic pattern, or implementation-defined initialization).
- **References**: §11.6.3 [memory.layout.padding]
- **Constraint**: Reading padding bytes directly is undefined behavior; only accessing via structure fields is defined.

#### §B.3.2 Optimization Ordering

**B.3.02 — Dead Code Elimination Order**

- **Description**: The order in which unreachable code is eliminated during optimization.
- **Permitted Outcomes**: Implementations may eliminate dead code in any order or not at all.
- **References**: §9.4.4 [stmt.order.divergence]
- **Constraint**: Observable behavior must not change; only removal of unobservable code permitted.

**B.3.03 — Inline Expansion Decisions**

- **Description**: Which procedure calls are inlined by the optimizer.
- **Permitted Outcomes**: Implementations may inline any procedure call or leave it as a call, provided observable behavior is preserved.
- **References**: Clause 10 §10.6 [generic.resolution.monomorphization]
- **Constraint**: Optimization must not alter observable behavior.

#### §B.3.3 Module Initialization

**B.3.04 — Lazy vs Eager Initialization**

- **Description**: When module-level bindings without eager dependencies are initialized.
- **Permitted Outcomes**: Implementations may initialize lazily (on first use) or eagerly (before main), provided observable behavior matches the eager semantics specified in §4.6.
- **References**: §4.6.2[7], §5.7.2
- **Constraint**: Each initializer runs exactly once; dependencies must be respected.

#### §B.3.4 Memory Layout

**B.3.05 — Field Reordering in Native Layout**

- **Description**: The order of fields in memory for types without `[[repr(C)]]` or explicit layout attributes.
- **Permitted Outcomes**: Implementations may reorder fields for optimization (size reduction, alignment efficiency) or preserve declaration order.
- **References**: §11.6.2, B.1.06
- **Constraint**: Field offsets determined by chosen ordering; must remain stable for the compilation.

[8] Unspecified behaviors enable implementation flexibility while maintaining portability: programs shall not rely on specific outcomes but may observe different outcomes across implementations without violating conformance.

---

**Previous**: Annex A — Grammar ([grammar]) | **Next**: Annex C — Formal Semantics ([formal])
