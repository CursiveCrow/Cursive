# Cursive Language Specification

## Clause 11 — Contracts

**Clause**: 11 — Contracts
**File**: 12-3_Grants.md
**Section**: §11.3 Grants (within Sequent)
**Stable label**: [contract.grant]  
**Forward references**: §12.2 [contract.sequent], §12.7 [contract.checking], Clause 5 §5.9 [decl.grant], Clause 10 §10.2.5 [generic.parameter.grant], Annex A §A.9 [grammar.grant]

---

### §11.3 Grants (within Sequent) [contract.grant]

#### §11.3.1 Overview

[1] _Grants_ are capability tokens that represent permissions to perform observable computational effects. The grant clause of a contractual sequent lists the grants required for a procedure to execute. Grant checking ensures that procedures declare all effects they may perform, enabling static tracking of capabilities and side effects.

[2] Cursive provides two categories of grants:

- **Built-in grants** (§12.3.3): Predefined capability tokens for standard effects (I/O, allocation, networking, etc.)
- **User-defined grants** (§5.9): Module-specific capability tokens for domain-specific effects

[3] Grants are compile-time annotations with zero runtime representation. All grant checking occurs statically; there is no runtime grant validation or capability system.

#### §11.3.2 Syntax [contract.grant.syntax]

[4] Grant references in sequent grant clauses follow:

```ebnf
grant_clause
    ::= grant_reference ("," grant_reference)*
     | ε

grant_reference
    ::= qualified_grant_path
     | grant_parameter_reference

qualified_grant_path
    ::= identifier ("::" identifier)*

grant_parameter_reference
    ::= identifier
```

[ Note: See Annex A §A.9 [grammar.grant] for complete grant grammar.
— end note ]

[5] Grant paths are qualified with `::` separators. Built-in grants use reserved namespaces (`alloc::heap`, `fs::read`). User-defined grants use module paths (`database::query`, `auth::validate`).

[6] Grant parameters (§10.2.5) appear as identifiers in grant clauses and are substituted during monomorphization with concrete grant sets.

**Example 12.3.2.1 (Grant syntax forms)**:

```cursive
// Built-in grants
procedure allocate_buffer()
    [[ alloc::heap |- true => true ]]
{ }

// User-defined grant
public grant query

procedure execute_query(sql: string@View): [Row]
    [[ query |- sql.len() > 0 => true ]]
{ }

// Multiple grants
procedure process_file(path: string@View)
    [[ fs::read, fs::write, alloc::heap |- path.len() > 0 => true ]]
{ }

// Grant parameter
procedure apply<T, U, ε>(f: (T) -> U ! ε): U
    [[ ε |- true => true ]]
{ result f(value) }
```

#### §11.3.3 Built-In Grant Catalog [contract.grant.builtin]

[7] Implementations shall provide at least the following built-in grants:

##### §11.3.3.1 Allocation Grants

**alloc namespace**:

| Grant           | Capability                    |
| --------------- | ----------------------------- |
| `alloc::heap`   | Heap memory allocation        |
| `alloc::region` | Region-scoped allocation      |
| `alloc::global` | Global allocator access (FFI) |

##### §11.3.3.2 File System Grants

**fs namespace**:

| Grant          | Capability                       |
| -------------- | -------------------------------- |
| `fs::read`     | Read file contents               |
| `fs::write`    | Write file contents              |
| `fs::delete`   | Delete files                     |
| `fs::metadata` | Query file metadata (size, etc.) |
| `fs::create`   | Create new files/directories     |

##### §11.3.3.3 Network Grants

**net namespace**:

| Grant          | Capability             |
| -------------- | ---------------------- |
| `net::connect` | Establish connections  |
| `net::listen`  | Listen for connections |
| `net::send`    | Send data over network |
| `net::receive` | Receive network data   |
| `net::dns`     | DNS resolution         |

##### §11.3.3.4 I/O Grants

**io namespace**:

| Grant       | Capability                     |
| ----------- | ------------------------------ |
| `io::read`  | Read from stdin or streams     |
| `io::write` | Write to stdout/stderr/streams |

##### §11.3.3.5 Concurrency Grants

**thread namespace**:

| Grant           | Capability           |
| --------------- | -------------------- |
| `thread::spawn` | Spawn new threads    |
| `thread::join`  | Join threads         |
| `thread::sleep` | Sleep current thread |

**sync namespace**:

| Grant          | Capability            |
| -------------- | --------------------- |
| `sync::atomic` | Atomic operations     |
| `sync::lock`   | Acquire locks/mutexes |

##### §11.3.3.6 System Grants

**sys namespace**:

| Grant       | Capability                  |
| ----------- | --------------------------- |
| `sys::env`  | Environment variable access |
| `sys::time` | System time queries         |
| `sys::exit` | Process termination         |

##### §11.3.3.7 Unsafe and FFI Grants

**unsafe namespace**:

| Grant               | Capability             |
| ------------------- | ---------------------- |
| `unsafe::ptr`       | Raw pointer operations |
| `unsafe::transmute` | Type transmutation     |
| `unsafe::asm`       | Inline assembly        |

**ffi namespace**:

| Grant       | Capability             |
| ----------- | ---------------------- |
| `ffi::call` | Foreign function calls |

##### §11.3.3.8 Panic Grant

**panic** (top-level):

| Grant   | Capability            |
| ------- | --------------------- |
| `panic` | Panic/abort execution |

##### §11.3.3.9 Compile-Time Grants

[8] Compile-time execution contexts (§2.2.4.2) support comptime-specific grants:

**comptime namespace**:

| Grant               | Capability              |
| ------------------- | ----------------------- |
| `comptime::alloc`   | Compile-time allocation |
| `comptime::codegen` | Code generation         |
| `comptime::config`  | Configuration queries   |
| `comptime::diag`    | Diagnostic emission     |

[9] Comptime grants are available only within `comptime` blocks and `comptime procedure` bodies. Using comptime grants in runtime procedures produces diagnostic E12-020.

#### §11.3.4 User-Defined Grants [contract.grant.user]

[10] User-defined grants are declared at module scope using the `grant` keyword (§5.9 [decl.grant]). Grant declarations introduce capability tokens into the module's namespace with visibility controlled by modifiers (`public`, `internal`, `private`).

[11] User-defined grant paths are module-qualified. For grant `g` in module `M`, the fully qualified path is `M::g`.

**Example 12.3.4.1 (User-defined grants)**:

```cursive
// Module: database
public grant query
public grant write
internal grant admin

procedure execute_query(sql: string@View): [Row]
    [[ query |- sql.len() > 0 => true ]]
{ }

// Module: application
import database

procedure fetch_users(): [User]
    [[ database::query, alloc::heap |- true => true ]]
{
    result database::execute_query("SELECT * FROM users")
}
```

#### §11.3.5 Grant Subsumption [contract.grant.subsumption]

[12] Grant sets are ordered by subset inclusion. A procedure with grant set $G_1$ may be used where grant set $G_2$ is required if $G_1 \subseteq G_2$ (fewer grants is stronger/more specific).

[13] **Subsumption rule**:

$$
\frac{G_1 \subseteq G_2}
     {(\tau_1, \ldots, \tau_n) \to \tau_r \; ! G_1 \; \texttt{[[} \cdots \texttt{]]} \;<: (\tau_1, \ldots, \tau_n) \to \tau_r \; ! G_2 \; \texttt{[[} \cdots \texttt{]]}}
\tag{Sub-Grant}
$$

[14] This allows pure procedures (grant set $\emptyset$) to substitute for procedures requiring grants, and procedures with specific grants to substitute for procedures accepting broader grant sets.

**Example 12.3.5.1 (Grant subsumption)**:

```cursive
// Pure procedure
procedure pure_compute(x: i32): i32
    [[ |- true => true ]]
{ result x * 2 }

// Procedure accepting grants
procedure with_logging(f: (i32) -> i32 ! { io::write })
    [[ io::write |- true => true ]]
{
    println("Computing...")
    result f(42)
}

// Valid: ∅ ⊆ {io::write}
with_logging(pure_compute)
```

#### §11.3.6 Grant Composition [contract.grant.composition]

[15] When a procedure calls other procedures, the caller's grant set must include all grants required by callees. Grant accumulation follows set union:

$$
\frac{\Gamma \vdash f : (\ldots) \to \tau_f \; ! G_f \quad \Gamma \vdash g : (\ldots) \to \tau_g \; ! G_g}
     {\Gamma \vdash \texttt{procedure } h \{ f(); g(); \} \text{ requires grants } G_f \cup G_g}
\tag{Grant-Union}
$$

[16] Implementations shall verify that every procedure's grant clause includes the union of all grants required by called procedures, used operations, and intrinsic effects.

**Example 12.3.6.1 (Grant composition)**:

```cursive
procedure read_config(path: string@View): Config
    [[ fs::read, alloc::heap |- path.len() > 0 => true ]]
{ }

procedure write_log(message: string@View)
    [[ fs::write |- true => true ]]
{ }

procedure process_config(path: string@View)
    [[ fs::read, fs::write, alloc::heap |- path.len() > 0 => true ]]
    //  ^^^^^^^^  ^^^^^^^^  ^^^^^^^^^^^  Union of callee grants
{
    let config = read_config(path)  // Requires: fs::read, alloc::heap
    write_log("Config loaded")       // Requires: fs::write
    // Caller must declare all grants used
}
```

#### §11.3.7 Grant Polymorphism [contract.grant.polymorphism]

[17] Grant parameters (§10.2.5) enable procedures polymorphic over grant sets. A grant parameter represents a variable grant set determined at instantiation:

```cursive
procedure map<T, U, ε>(values: [T], f: (T) -> U ! ε): [U]
    [[ alloc::heap, ε |- values.len() > 0 => result.len() == values.len() ]]
{
    let result = array::with_capacity(values.len())
    loop item: T in values {
        result.push(f(item))
    }
    result result
}
```

[18] At instantiation, the grant parameter $\varepsilon$ is substituted with the concrete grant set from the callable argument. The caller's grant clause must include both the explicit grants (`alloc::heap`) and the parameter grants ($\varepsilon$).

[19] Grant parameter bounds constrain which grants may be substituted (§10.3.7):

```cursive
procedure controlled<T, ε>(f: () -> T ! ε): T
    [[ ε |- true => true ]]
    where ε ⊆ {io::write, alloc::heap}
{
    result f()
}
```

#### §11.3.8 Grant Checking [contract.grant.checking]

[20] Grant checking occurs at call sites. For a call `f(args)` where $f$ has grant set $G_f$, the compiler verifies that the calling procedure's grant clause includes $G_f$:

$$
\frac{\text{procedure } caller \; \texttt{[[} G_{caller} \; \texttt{|-} \cdots \texttt{]]} \quad \Gamma \vdash f : \ldots \; ! G_f \quad G_f \subseteq G_{caller}}
     {\text{call } f(args) \text{ permitted}}
\tag{WF-Grant-Call}
$$

[21] If $G_f \not\subseteq G_{caller}$, the call is ill-formed and produces diagnostic E12-030 listing the missing grants.

**Example 12.3.8.1 (Grant checking)**:

```cursive
procedure helper()
    [[ io::write |- true => true ]]
{
    println("Helper")
}

procedure caller()
    [[ io::write |- true => true ]]
{
    helper()  // ✅ OK: {io::write} ⊆ {io::write}
}

procedure invalid_caller()
    [[ |- true => true ]]
{
    helper()  // ❌ ERROR E12-030: missing grant io::write
}
```

#### §11.3.9 Constraints [contract.grant.constraints]

[1] _Visibility._ User-defined grants must be visible in the scope where they are referenced. Cross-module grant usage requires the grant to be `public` in the defining module and the module to be imported. Private grants are accessible only within their defining module. Violations produce diagnostic E12-031.

[2] _No wildcards._ Grant clauses shall list grants explicitly. Wildcard expansion syntax (e.g., `fs::*`) is not supported. All grants must be explicitly enumerated. This ensures maximum explicitness and prevents ambiguity about which capabilities are required.

[3] _Grant set size._ Implementations shall support grant sets with at least 256 grant references. Exceeding this limit may produce diagnostic E12-032.

[4] _Reserved namespaces._ User-defined grants shall not use reserved top-level namespaces: `alloc`, `fs`, `net`, `io`, `thread`, `sync`, `sys`, `unsafe`, `ffi`, `panic`, `comptime`. Violations produce diagnostic E05-901 (defined in §5.9).

[5] _Grant path validity._ Every component of a qualified grant path shall be a valid identifier. Grant paths shall resolve to grant declarations visible in the current scope. Unresolved grants produce diagnostic E12-006.

#### §11.3.10 Semantics [contract.grant.semantics]

##### §11.3.10.1 Grant Availability

[6] At any point in a procedure body, the available grant set is the set declared in the procedure's sequent grant clause. Grant availability is lexically scoped: inner procedures have their own grant clauses and do not inherit grants from enclosing procedures.

[7] **Grant availability rule**:

[ Given: Procedure with grant clause $G$ ]

$$
\frac{\text{procedure } f \; \texttt{[[} G \; \texttt{|-} \cdots \texttt{]]} \{ \text{body} \}}
     {\text{In body: grants available} = G}
\tag{P-Grant-Available}
$$

##### §11.3.10.2 Grant Propagation

[8] Grants propagate through expressions according to the grant accumulation rule (§8.1.5). Each expression has a required grant set; compound expressions union the grant sets of their subexpressions:

$$
\frac{\Gamma \vdash e_1 : \tau_1 \; ! G_1 \quad \Gamma \vdash e_2 : \tau_2 \; ! G_2}
     {\Gamma \vdash (e_1; e_2) : \tau_2 \; ! (G_1 \cup G_2)}
\tag{Grant-Compose}
$$

[9] The accumulated grant set for the entire procedure body must be a subset of the declared grant clause. Violations indicate missing grants in the sequent and produce diagnostic E12-030.

##### §11.3.10.3 Grant Parameter Substitution

[10] When a procedure with grant parameters is instantiated, grant parameters are replaced with concrete grant sets:

$$
\frac{\text{procedure } f\langle\varepsilon\rangle \; \texttt{[[} \varepsilon \; \texttt{|-} \cdots \texttt{]]} \quad \text{instantiated with } G}
     {f\langle G \rangle \text{ has grants } G}
\tag{Grant-Param-Subst}
$$

[11] Grant parameter bounds (§10.3.7) constrain which grant sets may be substituted. The bound $\varepsilon \subseteq G_{bound}$ requires that the instantiated grant set satisfies $G_{concrete} \subseteq G_{bound}$.

#### §11.3.11 Intrinsic Operation Grants [contract.grant.intrinsic]

[12] Certain language constructs intrinsically require grants without explicit procedure calls:

**Table 12.2 — Intrinsic grant requirements**

| Operation                   | Required Grant  | Clause Reference |
| --------------------------- | --------------- | ---------------- |
| Region allocation `^expr`   | `alloc::region` | §11.3.2          |
| Heap conversion `to_heap()` | `alloc::heap`   | §11.3.3.5        |
| Raw pointer dereference     | `unsafe::ptr`   | §7.5.5.2         |
| Panic `panic(msg)`          | `panic`         | §9.x             |
| FFI call                    | `ffi::call`     | Clause 15        |

[13] Implementations shall accumulate intrinsic grants during expression typing and verify they are included in the procedure's grant clause.

#### §11.3.12 Examples [contract.grant.examples]

**Example 12.3.12.1 (Grant composition in complex procedure)**:

```cursive
public grant database_write

procedure save_user(user: User, path: string@View)
    [[
        database_write,      // User-defined grant
        fs::write,           // Built-in grant
        alloc::heap          // Built-in grant
    |-
        user.id > 0,
        path.len() > 0
    =>
        true
    ]]
{
    let serialized = user.serialize()     // Requires: alloc::heap
    write_to_database(user)                // Requires: database_write
    write_to_file(path, serialized)        // Requires: fs::write
}
```

**Example 12.3.12.2 (Grant parameter polymorphism)**:

```cursive
procedure twice<T, ε>(action: () -> T ! ε): (T, T)
    [[ ε |- true => true ]]
{
    result (action(), action())
}

// Usage with pure function
let pure_fn = || [[ |- true => true ]] { result 42 }
let pair1 = twice(pure_fn)  // ε = ∅

// Usage with grant-requiring function
let writer = || [[ io::write |- true => true ]] { println("hi") }
let pair2 = twice(writer)    // ε = {io::write}
```

**Example 12.3.12.3 (Intrinsic grants)**:

```cursive
procedure allocate_in_region(): Buffer
    [[ alloc::region, alloc::heap |- true => true ]]
{
    region temp {
        let buffer = ^Buffer::new()  // Intrinsic: requires alloc::region
        result buffer.to_heap()       // Intrinsic: requires alloc::heap
    }
}
```

#### §11.3.13 Diagnostics [contract.grant.diagnostics]

[14] Grant-related diagnostics:

| Code    | Condition                                     | Constraint |
| ------- | --------------------------------------------- | ---------- |
| E12-006 | Undefined grant in grant clause               | [5]        |
| E12-020 | Comptime grant used in runtime procedure      | [9]        |
| E12-030 | Call site missing required grants             | [21]       |
| E12-031 | Grant visibility violation (non-public grant) | [1]        |
| E12-032 | Grant set exceeds implementation limit        | [3]        |

#### §11.3.14 Conformance Requirements [contract.grant.requirements]

[15] Implementations shall:

1. Provide all built-in grants enumerated in §12.3.3
2. Support user-defined grants declared via §5.9 with module-qualified paths
3. Enforce grant visibility rules (public/internal/private)
4. Reject wildcard grant syntax; require explicit grant enumeration
5. Perform grant checking at call sites using subset inclusion
6. Accumulate grant sets through expressions via union
7. Support grant parameters with substitution and bounds checking
8. Track intrinsic operation grants and verify against grant clauses
9. Distinguish comptime grants from runtime grants
10. Emit diagnostics E12-006, E12-020, E12-030, E12-031, E12-032 for violations
11. Maintain grant metadata for reflection and callable type signatures

---

**Previous**: §12.2 Sequent: Syntax and Structure (§12.2 [contract.sequent]) | **Next**: §12.4 Preconditions: must (§12.4 [contract.precondition])
