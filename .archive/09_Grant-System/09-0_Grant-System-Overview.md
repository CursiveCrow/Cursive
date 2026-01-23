# The Cursive Language Specification

## Clause 9 — Grant System

**Clause**: 9 — Grant System
**File**: `09-0_Grant-System-Overview.md`
**Section**: §9.1 Grant System Overview
**Stable label**: [grant.overview]
**Version**: 1.0
**Status**: Normative
**Forward references**: Clause 5 §5.4 [decl.function], Clause 12 [memory], Clause 13 [contract]

---

## 9.0.1 Grant System Purpose

**Definition 9.1 (Grant)**: A grant is a compile-time capability token that authorizes a procedure to perform specific classes of operations.

The grant system provides compile-time verification that procedures declare and possess the necessary capabilities to perform operations such as heap allocation, file system access, network communication, and other side-effecting operations.

**Normative Statement 9.0.1**: A conforming implementation shall verify at compile time that for every procedure call, the callee's required grants are a subset of the caller's available grants.

**Formal Requirement**: For a call from procedure A to procedure B, the following must hold:

```
grants(B) <: grants(A)
```

Where `grants(P)` denotes the set of grants declared in procedure P's signature.

**Compile-Time Verification**: All grant checking shall be performed during compilation. A conforming implementation shall not generate runtime grant checking code.

**Diagnostic Requirement**: A conforming implementation shall issue a compile-time diagnostic if a procedure attempts to call another procedure whose grants are not a subset of the caller's grants.

---

## 9.0.2 Grant Tracking Goals

The grant system serves the following purposes:

### 9.0.2.1 Capability Restriction

**Purpose**: Enable procedures to declare and restrict the operations they may perform.

**Example (informative)**:
```cursive
// Pure computation - no grants required
procedure sum(data: [i32]): i32
    {| |- true => true |}
{
    let total = 0
    for i in 0..data.length {
        total += data[i]
    }
    result total
}

// File I/O - requires grant
procedure load_config(path: string): [u8]
    sequent { [fs::read] |- true => true }
{
    result read_file_bytes(path)
}
```

### 9.0.2.2 Transitive Capability Propagation

**Purpose**: Ensure that capability requirements propagate through call chains.

**Normative Statement 9.0.2**: If procedure A calls procedure B, and B requires grant set G, then A shall declare a grant set that is a superset of G.

**Example (informative)**:
```cursive
procedure process_file(path: string)
    sequent { [fs::read, fs::write, alloc::heap] |- true => true }
{
    let data = load_config(path)        // Requires fs::read
    let buffer = allocate_buffer(1024)  // Requires alloc::heap
    write_output("out.txt", buffer)     // Requires fs::write
}
```

### 9.0.2.3 Security and Sandboxing

**Purpose**: Enable construction of capability-restricted execution environments.

**Use Case**: Procedures with restricted grant sets cannot perform unauthorized operations, enabling sandboxing at the language level.

**Limitation**: Grants provide compile-time safety only. They do not prevent circumvention through `unsafe` blocks or FFI.

### 9.0.2.4 Optimization Opportunities

**Purpose**: Enable compiler optimizations based on declared capabilities.

**Informative Note**: A procedure with no grants may be:
- Evaluated at compile time
- Reordered freely
- Parallelized without synchronization

---

## 9.0.3 Grant System Model

### 9.0.3.1 NOT Algebraic Effects

**Critical Distinction**: The grant system is NOT an algebraic effect system.

| Aspect | Cursive Grants | Algebraic Effects |
|--------|---------------|-------------------|
| **Purpose** | Compile-time capability tracking | Runtime control-flow abstraction |
| **Verification** | Static (compile-time) | Dynamic (runtime handlers) |
| **Runtime Cost** | Zero (fully erased) | Non-zero (handler dispatch) |
| **Control Flow** | No resumption, no delimited continuations | Resumable, delimited continuations |
| **Handlers** | No runtime handlers | Runtime effect handlers required |

**Normative Statement 9.0.3**: The grant system shall impose zero runtime overhead. All grant verification shall be performed at compile time, and grant annotations shall be erased before code generation.

### 9.0.3.2 Compile-Time Capability Tokens

Grants are compile-time tokens that authorize operations. They are not:
- First-class values
- Runtime-checkable
- Dynamically manipulable

**Example (informative - INVALID)**:
```cursive
// INVALID: Grants are not values
// let g = alloc::heap  // ERROR

// INVALID: No runtime grant checking
// if has_grant(fs::write) {  // ERROR
//     write_file(...)
// }
```

### 9.0.3.3 Hierarchical Organization

**Normative Statement 9.0.4**: Grants shall be organized in hierarchical namespaces using double-colon notation.

**Grant Path Syntax**: `category::operation`

**Examples**:
- `alloc::heap` - Heap allocation
- `fs::read` - File system read
- `net::write` - Network write

**Wildcard Grant**: `category::*` grants all operations in a category.

**Forward Reference**: Complete grant hierarchy is specified in §9.4 (Built-In Grants).

### 9.0.3.4 Subset-Based Propagation

**Formal Rule (Subset Property)**: For a valid call from procedure A to procedure B:

```
∀ g ∈ grants(B) : g ∈ grants(A)
```

**Interpretation**: Every grant required by the callee must be present in the caller's grant set.

**Example (informative)**:
```cursive
procedure caller()
    sequent { [fs::read, fs::write, alloc::heap] |- true => true }
{
    // Valid: {fs::read} <: {fs::read, fs::write, alloc::heap}
    read_file("data.txt")

    // Valid: {alloc::heap} <: {fs::read, fs::write, alloc::heap}
    allocate_buffer(1024)

    // Invalid: {net::read} ⊄ {fs::read, fs::write, alloc::heap}
    // fetch_url("http://example.com")  // ERROR
}
```

### 9.0.3.5 Grant Polymorphism

**Purpose**: Enable procedures that are generic over grant sets.

**Syntax**: Grant parameters are declared using `grants` keyword in generic parameter lists and referenced using `grants(G)` syntax in grant contexts.

**Example (informative)**:
```cursive
procedure process<T, grants G>(data: T): T
    sequent { [grants(G), alloc::heap] |- true => true }
    where G <: {fs::read, net::read}
{
    // Can use any grants in G, plus alloc::heap
    result transform(data)
}
```

**Forward Reference**: Complete specification in §9.3 (Grant Polymorphism).

---

## 9.0.4 Terminology

**Normative Terminology**:

**Grant**: Compile-time capability token authorizing a class of operations

**Grant Context**: Grant set specified within sequent clause, enclosed in square brackets `[...]`

**Grant Set**: Collection of grants specified as comma-separated grant paths

**Grant Path**: Hierarchical identifier for a grant (e.g., `alloc::heap`, `fs::write`)

**Grant Parameter**: Generic parameter over grant sets, enabling grant polymorphism

**Grant Propagation**: Transitive flow of grant requirements through call chains

**Deprecated Terminology**:

The following terms are deprecated but retained for backward compatibility:

- **Effect** (deprecated) → **Grant** (canonical)
- **Uses clause** (deprecated) → **Grant context in sequent** (canonical)
- **Standalone grants clause** (deprecated) → **Grant context in sequent** (canonical)
- `uses` keyword (deprecated) → `sequent { [...] }` syntax (canonical)

**Migration Path**: Implementations may accept deprecated terminology but shall issue warnings. Future language versions may remove deprecated forms.

---

## 9.0.5 Relationship to Other Language Features

### 9.0.5.1 Functions vs Procedures

**Normative Statement 9.0.5**: Functions (pure, side-effect-free) shall have no grants. Only procedures (impure, side-effecting) may declare grants.

### 9.0.5.1 Pure vs Effectful Procedures

**Normative Statement 9.0.5**: Procedures whose contractual sequents declare an empty grant set are pure. Procedures with non-empty grant sets may exercise the listed capabilities. Pure procedures shall call only other procedures whose grant sets are empty.

**Example (informative)**:
```cursive
// Pure procedure: no grants
procedure add(a: i32, b: i32): i32
    {| |- true => true |}
{
    result a + b
}

// Effectful procedure: requires fs::write
procedure save_result(value: i32)
    sequent { [fs::write] |- true => true }
{
    write_to_file("result.txt", value)
}
```

**Forward Reference**: Grant-aware procedure declarations appear in Clause 5 §5.4 [decl.function].

### 9.0.5.2 Permissions

Grants (capability tracking) are orthogonal to permissions (aliasing control).

| Feature | Purpose | Domain |
|---------|---------|--------|
| **Permissions** (`owned`, `unique`, `shared`, `readonly`) | Aliasing and mutation control | Type system (Clause 12 [memory]) |
| **Grants** (`alloc::heap`, `fs::read`, etc.) | Capability tracking | Grant system (Clause 13 [contract.grant]) |

**Independence**: A procedure may have any combination of parameter permissions and grants.

**Example (informative)**:
```cursive
procedure write_buffer(buffer: unique [u8], path: readonly string)
    sequent { [fs::write] |- true => true }
{
    write_file_bytes(path, buffer)
}
```

**Forward Reference**: Complete permission specification in Clause 12 [memory].

### 9.0.5.3 Contracts

Grants integrate with contract clauses (preconditions, postconditions).

**Example (informative)**:
```cursive
procedure allocate_aligned(size: usize, align: usize): [u8]
    sequent {
        [alloc::heap]
        |- align.is_power_of_two() && size > 0
        => result.length == size
    }
{
    result heap_allocate_aligned<u8>(size, align)
}
```

**Forward Reference**: Complete contract integration in Clause 13 [contract].

### 9.0.5.4 Regions

Region allocation requires the `alloc::region` grant.

**Example (informative)**:
```cursive
procedure process_in_region<r>(data: [i32]): i32
    sequent { [alloc::region] |- true => true }
{
    region temp {
        let buffer = alloc_in<temp>([0; 1024])
        result compute(buffer, data)
    }
}
```

**Forward Reference**: Complete region specification in Clause 12 §12.3 [memory.region].

---

## 9.0.6 Conformance Requirements

A conforming implementation shall:

1. **Verify grant requirements** (§9.0.1): Reject programs where callee grants are not a subset of caller grants

2. **Enforce transitive propagation** (§9.0.2.2): Check grant requirements through entire call chains

3. **Erase at compile time** (§9.0.3.1): Generate code with zero runtime grant checking overhead

4. **Issue diagnostics** (§9.0.1): Provide compile-time error messages for grant violations

5. **Support built-in grants** (§9.0.3.3): Implement all grants specified in §9.4 (Built-In Grants)

6. **Support user-defined grants** (§9.5): Allow modules to declare custom grants (optional but recommended)

7. **Support grant polymorphism** (§9.0.3.5): Allow procedures generic over grant sets (optional but recommended)

---

## 9.0.7 Organization of Part IX

| Section | File | Content |
|---------|------|---------|
| **§9.0** | `09-0_Grant-System-Overview.md` | This section: purpose, goals, model |
| **§9.1** | `09-1_Grant-Clauses.md` | Syntax of grant declarations |
| **§9.2** | `09-2_Grant-Tracking.md` | Propagation rules, verification algorithm |
| **§9.3** | `09-3_Grant-Polymorphism.md` | Generic procedures over grant sets |
| **§9.4** | `09-4_Built-In-Grants.md` | Catalog of all built-in grants |
| **§9.5** | `09-5_User-Defined-Grants.md` | User-defined grant declarations |
| **§9.6** | `09-6_Grant-Integration.md` | Integration with other features |

**Note**: Section numbering adapted from Proposed_Organization.md §9 structure, with content updated for grant terminology.

---

**Previous**: [Part VIII] | **Next**: [09-1_Grant-Clauses.md](09-1_Grant-Clauses.md)
