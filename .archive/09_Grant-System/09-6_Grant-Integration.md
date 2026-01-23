# Part IX: Grant System
## Section 9.6: Grant Integration

**File**: `09-6_Grant-Integration.md`
**Version**: 1.0
**Status**: Normative
**Previous**: [09-5_User-Defined-Grants.md](09-5_User-Defined-Grants.md) | **Next**: [Part X]

---

## 9.6.1 Integration Overview

**Purpose**: This section specifies how the grant system integrates with other language features including contracts, permissions, modal types, and unsafe code.

**Principle**: Grants are orthogonal to most language features and compose independently.

---

## 9.6.2 Grants and Contracts

### 9.6.2.1 Contract Clause Ordering

**Normative Statement 9.6.1**: In procedure signatures, grants shall be specified as the context portion of the sequent clause, before the turnstile `|-`.

**Syntax Order**:
```
procedure name(parameters): return_type
    sequent { [grant_set] |- precondition => postcondition }
{
    body
}
```

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

### 9.6.2.2 Grants in Contract Predicates

**Normative Statement 9.6.2**: Contract predicates (preconditions, postconditions) shall be evaluated without requiring grants. Predicates must be pure.

**Rationale**: Contract predicates are logical assertions that should not perform side-effecting operations.

**Example (informative - INVALID)**:
```cursive
// ERROR: Precondition cannot require grants
procedure bad_example(path: string)
    sequent { [fs::write] |- file_exists(path) => true }  // ERROR if file_exists requires fs::read
{
    write_file(path, [0; 10])
}
```

### 9.6.2.3 Behavioral Contracts

**Integration**: Grant clauses compose with behavioral contract declarations.

**Example (informative)**:
```cursive
contract Serializable {
    procedure serialize(self: const Self): string
        sequent { [alloc::heap] |- true => true }
}

record Data: Serializable {
    value: i32

    procedure serialize(self: const Self): string
        sequent { [alloc::heap] |- true => true }
    {
        result format_number(self.value)
    }
}
```

**Normative Statement 9.6.3**: A procedure implementing a contract procedure shall declare at least the grants required by the contract signature.

**Forward Reference**: See Part X (Contract System) for complete behavioral contract specification.

---

## 9.6.3 Grants and Permissions

### 9.6.3.1 Orthogonality

**Normative Statement 9.6.4**: Grants (capability tracking) and permissions (aliasing control) are orthogonal systems that compose independently.

**Independence**:
- Grants control which operations a procedure can perform
- Permissions control how values can be accessed and modified

**Example (informative)**:
```cursive
procedure process_data(
    buffer: unique [u8],     // Permission: unique (exclusive access)
    path: readonly string    // Permission: readonly (immutable)
)
    sequent { [fs::write, alloc::heap] |- true => true }  // Grants: file write and heap allocation
{
    let expanded = heap_allocate_array<u8>(buffer.length * 2)
    write_file_bytes(path, expanded)
}
```

### 9.6.3.2 No Grant-Permission Interaction

**Normative Statement 9.6.5**: Permissions do not affect grant requirements, and grants do not affect permission checking.

**Example (informative)**:
```cursive
// Permission and grant are independent
procedure example(data: const Data)  // const permission
    sequent { [alloc::heap] |- true => true }                 // heap grant
{
    // const permission: cannot mutate data
    // alloc::heap grant: can allocate memory
    let copy = heap_allocate_array<Data>(1)
}
```

**Forward Reference**: See Part VI (Memory Model and Permissions) for complete permission specification.

---

## 9.6.4 Grants and Modal Types

### 9.6.4.1 State Transitions

**Normative Statement 9.6.6**: Procedures that perform modal state transitions may require grants.

**Example (informative)**:
```cursive
modal File {
    @Closed {
        path: string
    }

    @Open {
        path: string
        handle: FileHandle
    }

    procedure @Closed -> @Open open(self): File@Open
        sequent { [fs::read] |- true => true }
    {
        let handle = open_file(self.path)
        result File@Open { path: self.path, handle }
    }
}
```

### 9.6.4.2 Modal Coercions

**Normative Statement 9.6.7**: Modal coercion declarations may specify required grants using the `grants` field in coercion constraints.

**Syntax**:
```ebnf
CoercionConstraint ::= "{"
                       ("cost" ":" Cost ","?)?
                       ("requires" ":" Permission ","?)?
                       ("grants" ":" GrantSet ","?)?
                       "}"
```

**Example (informative)**:
```cursive
modal Connection {
    @Connecting { address: string }
    @Connected { socket: Socket }

    coerce @Connecting <: @Connected {
        cost: O(n),
        grants: net::connect
    }
}
```

**Forward Reference**: See Part XI (Modal Types) for complete modal type specification.

---

## 9.6.5 Grants and Unsafe Code

### 9.6.5.1 Unsafe Blocks

**Normative Statement 9.6.8**: Unsafe blocks may bypass normal safety checks but do NOT bypass grant requirements.

**Rationale**: Grants track capabilities at the procedure level, not the expression level. Unsafe blocks cannot acquire additional capabilities.

**Example (informative)**:
```cursive
procedure safe_wrapper()
    sequent { [unsafe::ptr] |- true => true }  // Grant required even for unsafe block
{
    unsafe {
        let ptr: *i32 = get_raw_pointer()
        let value = *ptr  // Requires unsafe::ptr grant
    }
}
```

### 9.6.5.2 Grant Requirements for Unsafe Operations

**Normative Statement 9.6.9**: Unsafe operations shall require the appropriate `unsafe::*` grants.

**Required Grants**:
- Raw pointer dereferencing: `unsafe::ptr`
- Type transmutation: `unsafe::transmute`
- FFI calls: `ffi::call`

**Example (informative)**:
```cursive
procedure transmute_example(x: u32): f32
    sequent { [unsafe::transmute] |- true => true }
{
    unsafe {
        result transmute<u32, f32>(x)
    }
}
```

**Forward Reference**: See Part VI §6.8 (Unsafe Operations) for complete unsafe semantics.

---

## 9.6.6 Pure and Effectful Procedures

### 9.6.6.1 Pure Procedure Restrictions

**Normative Statement 9.6.10**: Procedures whose contractual sequents declare an empty grant set are pure. Pure procedures shall not perform operations that require grants, nor may they call procedures whose grant sets are non-empty.

**Example (informative)**:
```cursive
// Valid: Pure procedure
procedure sum(data: [i32; 100]): i32
    {| |- true => true |}
{
    let total = 0
    for i in 0..100 {
        total += data[i]
    }
    result total
}

// INVALID: Pure procedure attempting heap allocation
procedure bad(): i32
    {| |- true => true |}
{
    let buffer = allocate_buffer(16)  // Requires alloc::heap → ERROR
    result buffer[0]
}
```

### 9.6.6.2 Invoking Pure Procedures from Effectful Ones

**Normative Statement 9.6.11**: Procedures may call pure procedures without inheriting additional grants because pure procedures require none.

**Example (informative)**:
```cursive
procedure pure_computation(x: i32): i32
    {| |- true => true |}
{
    result x * x
}

procedure caller()
    sequent { [fs::write] |- true => true }
{
    let value = pure_computation(42)  // No grants needed
    write_result(value)                // Requires fs::write
}
```

---

## 9.6.7 Grants and Closures

### 9.6.7.1 Closure Grant Requirements

**Normative Statement 9.6.12**: Closures inherit the grant set of their enclosing procedure. Closures cannot declare additional grants beyond those available in the enclosing scope.

**Example (informative)**:
```cursive
procedure outer()
    sequent { [fs::write, alloc::heap] |- true => true }
{
    let closure = || {
        // Closure can use fs::write and alloc::heap (from outer)
        write_file("log.txt", [0; 10])
        let buffer = heap_allocate_array<u8>(100)
    }

    closure()
}
```

### 9.6.7.2 Higher-Order Procedure Grants

**Normative Statement 9.6.13**: A procedure that accepts a closure as a parameter shall declare sufficient grants to execute that closure.

**Example (informative)**:
```cursive
procedure for_each<T>(
    items: [T; 100],
    f: (T) -> () sequent { [alloc::heap] |- true => true }
)
    sequent { [alloc::heap] |- true => true }  // Must have grants to call f
{
    for i in 0..100 {
        f(items[i])
    }
}
```

---

## 9.6.8 Grants and Generics

### 9.6.8.1 Type Parameters

**Normative Statement 9.6.14**: Type parameters and grant parameters are independent. A procedure may be generic over both types and grants.

**Example (informative)**:
```cursive
procedure process<T, grants G>(data: T): T
    sequent { [grants(G), alloc::heap] |- true => true }
{
    // Generic over type T and grant set G
    result transform(data)
}
```

### 9.6.8.2 Const Parameters

**Normative Statement 9.6.15**: Const parameters (compile-time constant values) do not interact with grant parameters.

**Example (informative)**:
```cursive
procedure process_array<const N: usize, grants G>(data: [i32; N])
    sequent { [grants(G)] |- true => true }
{
    // Generic over array size N and grant set G
    loop i in 0..N {
        process_element(data[i])
    }
}
```

---

## 9.6.9 Grants and Predicates

### 9.6.9.1 Predicate Method Grants

**Normative Statement 9.6.16**: Predicate method signatures may specify required grants. Implementations must declare at least the grants required by the predicate.

**Example (informative)**:
```cursive
predicate Loadable {
    procedure load(path: string): Self
        sequent { [fs::read, alloc::heap] |- true => true }
}

record Config: Loadable {
    data: [u8; 1024]

    procedure load(path: string): Config
        sequent { [fs::read, alloc::heap] |- true => true }  // Matches predicate requirements
    {
        let bytes = read_file_bytes(path)
        result Config::from_bytes(bytes)
    }
}
```

### 9.6.9.2 Marker Predicates

**Normative Statement 9.6.17**: Marker predicates (predicates with no methods) do not interact with grants.

**Example (informative)**:
```cursive
predicate Copy { }  // Marker predicate, no grant interaction

record Point: Copy {
    x: f64,
    y: f64,
}
```

**Forward Reference**: See Part VIII (Predicates and Dispatch) for complete predicate specification.

---

## 9.6.10 Grants and Attributes

### 9.6.10.1 Grant-Related Attributes

**Informative Note**: Implementations may provide attributes that affect grant checking behavior.

**Example Attributes** (informative, non-normative):
- `[[trusted]]` - Disable grant checking for a procedure (unsafe)
- `[[grant_infer]]` - Enable grant inference for a procedure
- `[[no_grants]]` - Assert that a procedure requires no grants

**Example (informative)**:
```cursive
[[trusted]]
procedure trusted_operation()
    // Grant checking disabled (use with extreme caution)
{
    // Can perform any operation without declaring grants
}
```

**Non-Normative**: Attribute behavior is implementation-defined.

---

## 9.6.11 Grants and Modules

### 9.6.11.1 Module-Level Grant Declarations

**Informative Note**: Grants are declared at the procedure level, not the module level. There is no module-wide grant declaration.

**Rationale**: Procedure-level granularity provides precise capability control.

### 9.6.11.2 Import and Export

**Normative Statement 9.6.18**: Grant requirements are part of procedure signatures and are preserved across module boundaries.

**Example (informative)**:
```cursive
// module: file_utils
public procedure load_config(path: string): [u8]
    sequent { [fs::read] |- true => true }
{
    result read_file_bytes(path)
}

// module: main
import file_utils

procedure main()
    sequent { [fs::read] |- true => true }
{
    let config = file_utils::load_config("config.txt")  // Requires fs::read
}
```

---

## 9.6.12 Grants and Metaprogramming

### 9.6.12.1 Compile-Time Evaluation

**Normative Statement 9.6.19**: Compile-time evaluated code (comptime blocks) does not require runtime grants. Compile-time operations are unrestricted.

**Rationale**: Compile-time code runs in the compiler, not in the generated program.

**Example (informative)**:
```cursive
procedure example() {
    comptime {
        // No grants required for compile-time operations
        let x = read_file("config.txt")  // OK at compile time
    }
}
```

### 9.6.12.2 Macros and Code Generation

**Informative Note**: Generated code must respect grant requirements. Macro-generated procedures must declare appropriate grants.

**Forward Reference**: See Part XII (Metaprogramming) for complete metaprogramming specification.

---

## 9.6.13 Grant Composition Patterns

### 9.6.13.1 Capability Layering

**Pattern**: Building procedures with progressively restricted grant sets.

**Example (informative)**:
```cursive
// Layer 1: Full capabilities
procedure unrestricted()
    sequent { [fs::*, net::*, alloc::*] |- true => true }
{
    // Can do anything
}

// Layer 2: Restricted to file system
procedure fs_only()
    sequent { [fs::read, fs::write] |- true => true }
{
    // Can only access file system
}

// Layer 3: Read-only
procedure read_only()
    sequent { [fs::read] |- true => true }
{
    // Can only read files
}
```

### 9.6.13.2 Sandbox Pattern

**Pattern**: Creating capability-restricted execution contexts.

**Example (informative)**:
```cursive
procedure sandbox_execute<T, grants G>(
    operation: () -> T sequent { [grants(G)] |- true => true }
): T
    sequent { [grants(G)] |- true => true }
    where G <: {alloc::heap}  // Restrict to heap allocation only
{
    // operation can only allocate, cannot perform I/O
    result operation()
}
```

### 9.6.13.3 Capability Injection

**Pattern**: Providing capabilities through parameters.

**Example (informative)**:
```cursive
record FileSystem {
    root: string
}

procedure FileSystem.read_file(self: const Self, path: string): [u8]
    sequent { [fs::read] |- true => true }
{
    let full_path = self.root + "/" + path
    result read_file_bytes(full_path)
}

procedure process_with_fs(fs: const FileSystem)
    sequent { [fs::read, alloc::heap] |- true => true }
{
    // Capabilities provided through fs parameter and grants
    let data = fs::read_file("config.txt")
}
```

---

## 9.6.14 Conformance Requirements

A conforming implementation shall:

1. **Compose with contracts** (§9.6.2): Support grant clauses in procedures with contract clauses
2. **Maintain orthogonality** (§9.6.3): Verify grants and permissions independently
3. **Integrate with modals** (§9.6.4): Support grants in modal transition procedures
4. **Enforce in unsafe** (§9.6.5): Require appropriate grants even in unsafe blocks
5. **Enforce pure procedure rules** (§9.6.6): Detect pure procedures performing grant-requiring operations or calling effectful procedures without declaring the requisite grants
6. **Support closures** (§9.6.7): Propagate grants to closures correctly
7. **Preserve across modules** (§9.6.11): Maintain grant requirements across module boundaries

---

**Previous**: [09-5_User-Defined-Grants.md](09-5_User-Defined-Grants.md) | **Next**: [Part X]
