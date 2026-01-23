# Cursive Language Specification

## Annex D — Implementation Limits (Informative)

**Part**: Annex
**Section**: Annex D - Implementation Limits
**Stable label**: [limits]
**Forward references**: None

---

### §D.1 Recommended Minimum Limits [limits.minimum]

[1] This annex specifies recommended minimum limits that conforming implementations should support. These limits provide guidance for implementers and help programmers write portable code. Implementations may exceed these minimums and shall document their actual limits.

[2] **Normative requirement**: Implementations shall support at least the minimum limits specified below. Exceeding the limits may produce implementation-defined diagnostics or may be unsupported (IFNDR).

#### §D.1.1 Source Text Limits

**D.1.01 — Source File Size**

- **Minimum**: 1 MiB (1,048,576 bytes)
- **Reference**: §2.1.3[5]
- **Diagnostic**: E02-002 if exceeded
- **Rationale**: Supports large generated files and comprehensive modules

**D.1.02 — Identifier Length**

- **Minimum**: 1023 characters
- **Reference**: §2.3, §6.1
- **Diagnostic**: E02-201 if exceeded
- **Rationale**: Accommodates descriptive names and generated identifiers

**D.1.03 — String Literal Length**

- **Minimum**: 65,535 characters (64 KiB)
- **Reference**: §2.3.3[6]
- **Diagnostic**: E02-200 if exceeded
- **Rationale**: Supports embedded documentation and data

#### §D.1.2 Syntactic Nesting Limits

**D.1.04 — Block Nesting Depth**

- **Minimum**: 256 levels
- **Reference**: §6.2, §9.1
- **Diagnostic**: Implementation-defined
- **Rationale**: Supports deeply nested control flow

**D.1.05 — Expression Nesting Depth**

- **Minimum**: 256 levels
- **Reference**: Clause 8
- **Diagnostic**: Implementation-defined
- **Rationale**: Accommodates complex expressions

**D.1.06 — Type Nesting Depth**

- **Minimum**: 128 levels
- **Reference**: Clause 7
- **Diagnostic**: Implementation-defined
- **Rationale**: Supports nested generic types

**D.1.07 — Delimiter Nesting Depth**

- **Minimum**: 256 levels
- **Reference**: §2.4.3[3.1]
- **Diagnostic**: E02-300
- **Rationale**: Prevents stack overflow in pathological cases

#### §D.1.3 Declaration Limits

**D.1.08 — Procedure Parameters**

- **Minimum**: 255 parameters
- **Reference**: §5.4 [decl.function]
- **Diagnostic**: Implementation-defined
- **Rationale**: Accommodates complex APIs

**D.1.09 — Record Fields**

- **Minimum**: 1024 fields
- **Reference**: §5.5 [decl.type]
- **Diagnostic**: Implementation-defined
- **Rationale**: Supports large data structures

**D.1.10 — Enum Variants**

- **Minimum**: 1024 variants
- **Reference**: §5.5 [decl.type], §7.3.5
- **Diagnostic**: Implementation-defined
- **Rationale**: Accommodates extensive state machines

**D.1.11 — Modal States**

- **Minimum**: 64 states per modal type
- **Reference**: §7.6 [type.modal]
- **Diagnostic**: Implementation-defined
- **Rationale**: Supports complex state machines

**D.1.12 — Generic Parameters**

- **Minimum**: 256 generic parameters per entity
- **Reference**: §10.2 [generic.parameter]
- **Diagnostic**: Implementation-defined
- **Rationale**: Supports highly generic code

#### §D.1.4 Compile-Time Evaluation Limits

**D.1.13 — Comptime Recursion Depth**

- **Minimum**: 256 stack frames
- **Reference**: §2.2.3[3], §16.2.3.3[13]
- **Diagnostic**: E02-101, E16-003
- **Rationale**: Supports recursive comptime algorithms

**D.1.14 — Comptime Evaluation Steps**

- **Minimum**: 1,000,000 evaluation steps per comptime block
- **Reference**: §2.2.3[3], §16.2.3.3[13]
- **Diagnostic**: E02-102, E16-004
- **Rationale**: Prevents infinite loops while allowing complex generation

**D.1.15 — Comptime Memory Allocation**

- **Minimum**: 64 MiB per compilation unit
- **Reference**: §2.2.3[3], §16.2.3.3[13]
- **Diagnostic**: E02-103, E16-005
- **Rationale**: Supports large generated data structures

**D.1.16 — Comptime String Size**

- **Minimum**: 1 MiB per string value
- **Reference**: §2.2.3[3], §16.2.3.3[13]
- **Diagnostic**: E02-104, E16-006
- **Rationale**: Accommodates code generation templates

**D.1.17 — Comptime Collection Cardinality**

- **Minimum**: 10,000 elements per array/collection
- **Reference**: §2.2.3[3], §16.2.3.3[13]
- **Diagnostic**: E02-105, E16-008
- **Rationale**: Supports bulk code generation

#### §D.1.5 Name Resolution Limits

**D.1.18 — Qualified Name Chain Length**

- **Minimum**: 32 components
- **Reference**: §6.4.4[12.2]
- **Diagnostic**: E06-406
- **Rationale**: Supports deep module hierarchies

**D.1.19 — Scope Depth**

- **Minimum**: 256 nested scopes
- **Reference**: §6.2 [name.scope]
- **Diagnostic**: Implementation-defined
- **Rationale**: Matches block nesting limit

#### §D.1.6 Generic and Contract Limits

**D.1.20 — Monomorphization Instantiations**

- **Minimum**: 1024 distinct instantiations per generic entity
- **Reference**: §10.6.9.3[58]
- **Diagnostic**: E10-612
- **Rationale**: Supports extensive generic usage

**D.1.21 — Generic Instantiation Nesting**

- **Minimum**: 32 levels of nested generic instantiation
- **Reference**: §10.6.9.3[58]
- **Diagnostic**: E10-612
- **Rationale**: Supports recursive generic types with indirection

**D.1.22 — Grant Set Size**

- **Minimum**: 256 grants per sequent clause
- **Reference**: §12.3.9[3]
- **Diagnostic**: E12-032
- **Rationale**: Accommodates complex capability requirements

**D.1.23 — Where Clause Predicates**

- **Minimum**: 64 predicates per where clause
- **Reference**: §10.3 [generic.bounds], §12.6 [contract.invariant]
- **Diagnostic**: Implementation-defined
- **Rationale**: Supports complex constraints

#### §D.1.7 Runtime Limits (Implementation-Defined)

**D.1.24 — Stack Size**

- **Minimum**: Implementation-defined (typically 1-8 MiB)
- **Reference**: §11.2.3 [memory.object.storage], B.1.08
- **Documentation Required**: Implementations shall document stack size and whether configurable
- **Rationale**: Platform-dependent

**D.1.25 — Maximum Heap Allocation**

- **Minimum**: Platform-dependent (limited by available memory)
- **Reference**: §11.2.3 [memory.object.storage]
- **Documentation Required**: Implementations shall document maximum single allocation and total heap size
- **Rationale**: Platform and configuration dependent

**D.1.26 — Concurrent Threads**

- **Minimum**: Platform-dependent (typically at least number of CPU cores)
- **Reference**: §14.1 [concurrency.model]
- **Documentation Required**: Implementations shall document thread limits
- **Rationale**: Platform and runtime dependent

[3] **Conformance Note**: Implementations exceeding these limits provide better support for complex programs but are not required to do so. Programs relying on limits beyond those specified here are not portable.

---

### §D.2 Translation Limits [limits.translation]

[4] This section specifies limits applicable during specific compilation phases. These limits are distinct from the general limits in §D.1.

#### §D.2.1 Parsing Phase Limits

**D.2.01 — Parse Tree Depth**

- **Minimum**: Same as block nesting (256 levels)
- **Reference**: §2.2.4.1
- **Diagnostic**: Implementation-defined (may cause parser stack overflow)
- **Rationale**: Parser must handle deeply nested structures

**D.2.02 — Declaration Count per Module**

- **Minimum**: 65,535 top-level declarations
- **Reference**: §2.5, Clause 4
- **Diagnostic**: Implementation-defined
- **Rationale**: Supports generated code and large APIs

#### §D.2.2 Type Checking Phase Limits

**D.2.03 — Type Unification Complexity**

- **Minimum**: 1024 unification variables per type inference problem
- **Reference**: §7.1, §10.6.8 [generic.resolution.constraints]
- **Diagnostic**: E10-610 (no principal type)
- **Rationale**: Supports complex generic inference

**D.2.04 — Constraint Solving Iterations**

- **Minimum**: 10,000 constraint solving iterations for generic bounds
- **Reference**: §10.6.8
- **Diagnostic**: E10-610
- **Rationale**: Prevents infinite constraint solving loops

#### §D.2.3 Code Generation Phase Limits

**D.2.05 — Generated Declarations**

- **Minimum**: 100,000 declarations generated via comptime/codegen
- **Reference**: §16.8 [comptime.codegen.api]
- **Diagnostic**: E16-210/E16-211/E16-212
- **Rationale**: Supports extensive metaprogramming

**D.2.06 — Witness Table Entries**

- **Minimum**: 256 procedures per behavior/contract vtable
- **Reference**: §13.4 [witness.representation]
- **Diagnostic**: Implementation-defined
- **Rationale**: Accommodates large interfaces

[5] **Quality of Implementation**: Implementations should provide diagnostic warnings when approaching limits (e.g., at 50%, 75%, 90% of capacity) to help programmers avoid hitting hard limits during development.

[6] **Conformance Testing**: Conformance test suites should include tests exercising limits at or near the minimums specified here to verify implementations meet requirements.

---

### §D.3 Recommended Practices [limits.practices]

[7] Programmers writing portable code should:

1. **Stay well below limits**: Target 50% of minimum limits for maximum portability
2. **Test on multiple implementations**: Verify code works across different compilers
3. **Avoid extreme nesting**: Refactor deeply nested code for readability and portability
4. **Monitor comptime resource usage**: Use comptime_note to track generation progress
5. **Partition large modules**: Split very large files across multiple modules

[8] Implementations providing limit diagnostics should emit warnings when programs approach limits, enabling proactive refactoring before hard limits are reached.

---

**Previous**: Annex C — Formal Semantics ([formal]) | **Next**: Annex E — Implementation Guidance ([implementation])
