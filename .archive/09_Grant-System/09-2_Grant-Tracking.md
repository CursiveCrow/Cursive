# Part IX: Grant System
## Section 9.2: Grant Tracking and Propagation

**File**: `09-2_Grant-Tracking.md`
**Version**: 1.0
**Status**: Normative
**Previous**: [09-1_Grant-Clauses.md](09-1_Grant-Clauses.md) | **Next**: [09-3_Grant-Polymorphism.md](09-3_Grant-Polymorphism.md)

---

## 9.2.1 Grant Propagation Fundamentals

### 9.2.1.1 Subset Rule

**Normative Statement 9.2.1 (Subset Rule)**: For a call from procedure A to procedure B to be valid, the grant set of B shall be a subset of the grant set of A.

**Formal Definition**:
```
∀ call A → B : grants(B) <: grants(A)
```

Where:
- `grants(P)` denotes the grant set declared in procedure P's signature
- `<:` denotes the subset relation on grant sets

**Interpretation**: The callee cannot require capabilities that the caller does not possess.

**Example (informative)**:
```cursive
procedure helper()
    sequent { [fs::read] |- true => true }
{
    result read_file_bytes("config.txt")
}

procedure caller()
    sequent { [fs::read, fs::write] |- true => true }
{
    // Valid: {fs::read} <: {fs::read, fs::write}
    let data = helper()
    result data
}
```

### 9.2.1.2 Empty Grant Set

**Normative Statement 9.2.2**: A procedure with an empty grant set (no grant clause) may call only procedures that also have empty grant sets.

**Formal Rule**:
```
grants(A) = ∅  ⟹  ∀ call A → B : grants(B) = ∅
```

**Example (informative)**:
```cursive
procedure pure_computation(x: i32): i32
    {| |- true => true |}
{
    result x * 2
}

procedure caller(): i32
    {| |- true => true |}
{
    // Valid: Both have empty grant sets
    result pure_computation(42)
}

// INVALID:
procedure bad_caller(): i32
    {| |- true => true |}
{
    // ERROR: {fs::read} ⊄ ∅
    // result load_file("data.txt")  // ERROR
}
```

### 9.2.1.3 Wildcard Expansion

**Normative Statement 9.2.3**: For purposes of subset checking, wildcard grants shall be expanded to their full grant sets before comparison.

**Formal Rule**:
```
category::* = {category::op₁, category::op₂, ..., category::opₙ}
```

**Example (informative)**:
```cursive
procedure caller()
    sequent { [fs::*] |- true => true }  // Expands to {fs::read, fs::write, fs::delete, fs::metadata}
{
    // Valid: {fs::read} <: {fs::read, fs::write, fs::delete, fs::metadata}
    read_file("data.txt")

    // Valid: {fs::write} <: {fs::read, fs::write, fs::delete, fs::metadata}
    write_file("output.txt", [0; 100])
}
```

---

## 9.2.2 Transitive Grant Propagation

### 9.2.2.1 Transitive Closure

**Normative Statement 9.2.4**: The grant requirements for a procedure shall include the union of grants required by all procedures it calls, directly or indirectly (transitive closure).

**Formal Definition**:

Let `calls(P)` be the set of procedures directly called by procedure P. The transitive grant requirements for P are:

```
required_grants(P) = ⋃{grants(Q) | Q ∈ calls(P)} ∪ ⋃{required_grants(Q) | Q ∈ calls(P)}
```

**Implementation Requirement**: A conforming implementation shall compute the transitive closure of grant requirements.

**Example (informative)**:
```cursive
procedure level3()
    sequent { [fs::read] |- true => true }
{
    result read_file_bytes("data.txt")
}

procedure level2()
    sequent { [fs::read, alloc::heap] |- true => true }
{
    let data = level3()         // Requires fs::read
    let buffer = allocate(1024) // Requires alloc::heap
    result buffer
}

procedure level1()
    sequent { [fs::read, fs::write, alloc::heap] |- true => true }
{
    let buffer = level2()      // Requires {fs::read, alloc::heap}
    write_file("out.txt", buffer)  // Requires fs::write
}

// level1 must declare: {fs::read, alloc::heap} ∪ {fs::write} = {fs::read, fs::write, alloc::heap}
```

### 9.2.2.2 Direct Operation Requirements

**Normative Statement 9.2.5**: A procedure shall declare grants for all operations it performs directly, in addition to grants required by called procedures.

**Example (informative)**:
```cursive
procedure process()
    sequent { [fs::read, fs::write, alloc::heap] |- true => true }
{
    // Direct operation: requires fs::read
    let data = read_file_bytes("input.txt")

    // Direct operation: requires alloc::heap
    let buffer = heap_allocate_array<u8>(1024)

    // Call: requires {fs::write}
    save_to_file("output.txt", buffer)
}
```

### 9.2.2.3 Minimal Grant Sets

**Recommendation**: Procedures should declare minimal grant sets sufficient for their operations.

**Example (informative)**:
```cursive
// Good: Minimal grant set
procedure read_config(): [u8]
    sequent { [fs::read] |- true => true }
{
    result read_file_bytes("config.txt")
}

// Discouraged: Over-privileged
procedure read_config_bad(): [u8]
    sequent { [fs::read, fs::write, fs::delete, net::*, alloc::heap] |- true => true }  // Excessive
{
    result read_file_bytes("config.txt")
}
```

---

## 9.2.3 Grant Verification Algorithm

### 9.2.3.1 Verification Phases

**Normative Statement 9.2.6**: A conforming implementation shall verify grant requirements in the following phases:

1. **Grant Set Construction**: Parse and construct grant sets for each procedure
2. **Wildcard Expansion**: Expand wildcard grants to full sets
3. **Call Graph Analysis**: Build procedure call graph
4. **Transitive Closure**: Compute transitive grant requirements
5. **Subset Verification**: Verify subset relations at all call sites
6. **Diagnostic Generation**: Issue diagnostics for violations

### 9.2.3.2 Grant Set Construction

**Algorithm** (informative):

```
For each procedure P:
  1. Parse grant clause
  2. Construct set grants(P) from grant paths
  3. Validate all grant paths are known (§9.4 for built-in, §9.5 for user-defined)
  4. Check for duplicates (ill-formed if duplicates found)
```

**Example (informative)**:
```cursive
procedure example()
    sequent { [fs::read, alloc::heap, fs::write] |- true => true }
{
    // grants(example) = {fs::read, alloc::heap, fs::write}
}
```

### 9.2.3.3 Wildcard Expansion

**Algorithm** (informative):

```
For each grant g in grants(P):
  If g matches pattern "category::*":
    Replace g with {category::op₁, ..., category::opₙ}
    where category::opᵢ are all known grants with prefix "category::"
```

**Example (informative)**:
```cursive
// Before expansion:
sequent { [fs::*, alloc::heap] |- true => true }

// After expansion:
sequent { [fs::read, fs::write, fs::delete, fs::metadata, alloc::heap] |- true => true }
```

### 9.2.3.4 Call Graph Analysis

**Algorithm** (informative):

```
1. For each procedure P:
   a. Identify all procedure calls in P's body
   b. Record edges (P, Q) for each call from P to Q
2. Detect cycles (recursive calls)
3. Build topological ordering (for non-recursive portion)
```

**Recursive Procedures**: Recursive procedures require special handling (see §9.2.4.1).

### 9.2.3.5 Transitive Closure Computation

**Algorithm** (informative):

```
For each procedure P (in reverse topological order):
  required_grants(P) = ∅
  For each direct call P → Q:
    required_grants(P) = required_grants(P) ∪ grants(Q)
```

**Implementation Note**: Memoization may be used to avoid recomputing transitive closures.

### 9.2.3.6 Subset Verification

**Algorithm** (informative):

```
For each procedure call A → B:
  If not (grants(B) <: grants(A)):
    Issue diagnostic: "Insufficient grants for call to B"
    Report missing grants: grants(B) \ grants(A)
```

**Example Diagnostic** (informative):
```
Error: Insufficient grants for call to `write_log`
  --> procedure caller at line 42
  |
  | write_log("message")
  | ^^^^^^^^^ requires grant `fs::write` which is not available
  |
  = note: caller grants: {alloc::heap}
  = note: callee requires: {fs::write}
  = help: add `grants fs::write` to procedure signature
```

---

## 9.2.4 Special Cases

### 9.2.4.1 Recursive Procedures

**Normative Statement 9.2.7**: For recursive procedures, the grant set declared in the signature shall be sufficient for all operations performed during any level of recursion.

**Verification**: The grant set is checked against the procedure's own body, including recursive calls.

**Example (informative)**:
```cursive
procedure recursive_process(data: [i32], depth: i32)
    sequent { [fs::write, alloc::heap] |- true => true }
{
    if depth > 0 {
        let buffer = allocate(depth)  // Requires alloc::heap
        write_file("log.txt", buffer) // Requires fs::write
        recursive_process(data, depth - 1)  // Valid: same grant set
    }
}
```

### 9.2.4.2 Mutually Recursive Procedures

**Normative Statement 9.2.8**: For mutually recursive procedures, each procedure's grant set shall be sufficient for all procedures in the mutually recursive group.

**Example (informative)**:
```cursive
procedure even_process(n: i32)
    sequent { [fs::read] |- true => true }
{
    if n > 0 {
        let data = read_file_bytes("even.txt")
        odd_process(n - 1)
    }
}

procedure odd_process(n: i32)
    sequent { [fs::read] |- true => true }
{
    if n > 0 {
        let data = read_file_bytes("odd.txt")
        even_process(n - 1)
    }
}
```

### 9.2.4.3 Callable Pointers and Closures

**Normative Statement 9.2.9**: When calling through a callable pointer or closure, the caller shall possess all grants that could possibly be required by any procedure that matches the callable type.

**Conservative Analysis**: Implementations may require callers to possess all grants declared in the callable’s FunctionType.

**Example (informative)**:
```cursive
type FileProcessor = (string) -> () sequent { [fs::read, fs::write] |- true => true }

procedure apply_processor(f: FileProcessor, path: string)
    sequent { [fs::read, fs::write] |- true => true }  // Must have all grants from FileProcessor
{
    f(path)
}
```

**Forward Reference**: See §7.4 [type.function] for the complete callable type specification.

### 9.2.4.4 Higher-Order Procedures

**Normative Statement 9.2.10**: A procedure that accepts another procedure as a parameter shall declare sufficient grants to call that procedure.

**Example (informative)**:
```cursive
procedure for_each<T>(
    items: [T],
    f: (T) -> () sequent { [alloc::heap] |- true => true }
)
    sequent { [alloc::heap] |- true => true }  // Must have grants to call f
{
    for i in 0..items.length {
        f(items[i])
    }
}
```

---

## 9.2.5 Compile-Time Erasure

### 9.2.5.1 Zero Runtime Cost

**Normative Statement 9.2.11**: A conforming implementation shall erase all grant information before code generation. Grant verification shall impose zero runtime overhead.

**No Runtime Checks**: Generated code shall not contain any grant checking logic.

**No Runtime Representation**: Grant sets shall have no runtime representation.

### 9.2.5.2 Code Generation

**Normative Statement 9.2.12**: After grant verification, code generation shall proceed as if sequent clauses with grants were not present.

**Example (informative)**:
```cursive
// Source:
procedure process()
    sequent { [fs::read] |- true => true }
{
    let data = read_file_bytes("data.txt")
    result data
}

// Generated code (conceptual):
// - No grant checking code
// - Direct call to read_file_bytes
// - Same as procedure without sequent clause
```

---

## 9.2.6 Grant Checking for Operations

### 9.2.6.1 Built-In Operations

**Normative Statement 9.2.13**: Certain built-in operations require specific grants. A conforming implementation shall verify that procedures performing these operations declare the required grants.

**Grant-Requiring Operations**:

| Operation | Required Grant | Example |
|-----------|---------------|---------|
| Heap allocation | `alloc::heap` | `heap_allocate_array<T>(n)` |
| Region allocation | `alloc::region` | `alloc_in<r>(value)` |
| File read | `fs::read` | `read_file_bytes(path)` |
| File write | `fs::write` | `write_file_bytes(path, data)` |
| File delete | `fs::delete` | `delete_file(path)` |
| Network read | `net::read` | `socket_recv(socket, buffer)` |
| Network write | `net::write` | `socket_send(socket, data)` |
| Thread spawn | `thread::spawn` | `thread::spawn(closure)` |
| FFI call | `ffi::call` | `external_c_function()` |
| Unsafe operation | `unsafe::*` | `unsafe { ... }` |
| Panic | `panic` | `panic("error")` |

**Forward Reference**: Complete built-in grant catalog in §9.4 (Built-In Grants).

### 9.2.6.2 Operation-Grant Mapping

**Normative Statement 9.2.14**: The mapping from operations to required grants is defined by the implementation for built-in operations and by procedure signatures for user-defined procedures.

**Example (informative)**:
```cursive
procedure allocate_buffer(size: usize): [u8]
    sequent { [alloc::heap] |- true => true }
{
    // Operation: heap_allocate_array
    // Required grant: alloc::heap
    result heap_allocate_array<u8>(size)
}

procedure caller()
    sequent { [alloc::heap] |- true => true }
{
    // Valid: caller has alloc::heap
    let buffer = allocate_buffer(1024)
}
```

---

## 9.2.7 Grant Violations and Diagnostics

### 9.2.7.1 Insufficient Grants

**Diagnostic Requirement 9.2.1**: When a procedure calls another procedure with insufficient grants, the implementation shall issue a compile-time diagnostic.

**Required Information**:
1. Location of the call
2. Called procedure name
3. Required grants (callee's grant set)
4. Available grants (caller's grant set)
5. Missing grants (required ∖ available)

**Example Diagnostic** (informative):
```
error[E0412]: insufficient grants for call to `write_file`
  --> src/main.cursive:42:5
   |
42 |     write_file("output.txt", data)
   |     ^^^^^^^^^^ call requires grant `fs::write`
   |
   = note: procedure grants: {fs::read, alloc::heap}
   = note: call requires: {fs::write}
   = help: add `fs::write` to grant context in sequent clause
```

### 9.2.7.2 Undeclared Operations

**Diagnostic Requirement 9.2.2**: When a procedure performs an operation directly without declaring the required grant, the implementation shall issue a compile-time diagnostic.

**Example Diagnostic** (informative):
```
error[E0413]: operation requires undeclared grant
  --> src/main.cursive:15:9
   |
15 |         let data = read_file_bytes("config.txt")
   |                    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^ requires grant `fs::read`
   |
   = note: procedure grants: {alloc::heap}
   = help: add `fs::read` to grant context in sequent clause
```

### 9.2.7.3 Transitive Violations

**Diagnostic Requirement 9.2.3**: When a procedure's grant set is insufficient due to transitive requirements, the diagnostic should indicate the call chain.

**Example Diagnostic** (informative):
```
error[E0414]: insufficient grants for transitive call
  --> src/main.cursive:30:5
   |
30 |     process_file(path)
   |     ^^^^^^^^^^^^^^^^^^ call requires grant `fs::write`
   |
   = note: call chain:
     caller (grants: {fs::read})
       -> process_file (grants: {fs::read, fs::write})
         -> write_output (grants: {fs::write})
   |
   = help: add `fs::write` to grant context in procedure `caller` sequent clause
```

---

## 9.2.8 Grant Inference

### 9.2.8.1 Inference Capability

**Informative Note**: Implementations may optionally provide grant inference, automatically determining required grants from procedure bodies.

**Non-Normative**: Grant inference is not required for conformance.

**Benefits**:
- Reduces annotation burden
- Ensures completeness of grant declarations
- Assists migration from unannotated code

### 9.2.8.2 Inference Hole Syntax

**Syntax**: `[_?]` in the grant context indicates that grants should be inferred.

**Example (informative)**:
```cursive
procedure example()
    sequent { [_?] |- true => true }  // Infer grants from body
{
    let data = read_file_bytes("config.txt")  // Infers fs::read
    let buffer = allocate(1024)               // Infers alloc::heap
    write_file("output.txt", buffer)          // Infers fs::write
}

// Inferred: sequent { [fs::read, alloc::heap, fs::write] |- true => true }
```

**Forward Reference**: Complete inference specification in Part VIII (Type Inference and Holes).

---

## 9.2.9 Conformance Requirements

A conforming implementation shall:

1. **Compute transitive closure** (§9.2.2): Determine all grant requirements transitively
2. **Verify subset relation** (§9.2.1): Check `grants(B) <: grants(A)` at all call sites
3. **Expand wildcards** (§9.2.1.3): Expand wildcard grants before verification
4. **Erase at compile time** (§9.2.5): Generate code with zero grant checking overhead
5. **Issue diagnostics** (§9.2.7): Provide clear error messages for grant violations
6. **Handle recursion** (§9.2.4): Verify recursive and mutually recursive procedures correctly

---

**Previous**: [09-1_Grant-Clauses.md](09-1_Grant-Clauses.md) | **Next**: [09-3_Grant-Polymorphism.md](09-3_Grant-Polymorphism.md)
