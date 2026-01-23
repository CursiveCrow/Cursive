# Cursive Language Specification
## Clause 4 — Declarations

**Clause**: 4 — Declarations
**File**: 05-8_Program-Entry.md
**Section**: §4.8 Program Entry and Execution Model
**Stable label**: [decl.entry]  
**Forward references**: §3.6 [module.initialization], Clause 8 [stmt], Clause 10 [memory], Clause 13 [concurrency], Clause 14 [interop]

---

### §4.8 Program Entry and Execution Model [decl.entry]

#### §4.8.1 Overview

[1] This subclause defines how an executable Cursive program begins execution, the required form of the entry point, and how exit status is determined.

[2] Library modules omit an entry point and instead expose public APIs.

#### §5.8.2 Entry Point Requirements

[1] An executable module shall define exactly one top-level `procedure main`. Multiple definitions or the absence of `main` produce diagnostic E05-801.

[2] The entry point must be declared as:
```
public procedure main(): i32
```
or
```
public procedure main(args: [string]): i32
```
Return type `i32` expresses the process exit status.

[3] `main` shall be `public`. Other visibility modifiers are rejected (E05-802).

[4] `main` shall declare a contractual sequent that explicitly lists any grants required for execution. When `main` uses operations that require grants (such as I/O, allocation, file system access, etc.), the sequent clause is **required** and must explicitly declare those grants. If `main` performs no grant-requiring operations, the sequent clause may be omitted and defaults to `[[ ∅ |- true => true ]]` (empty grant set, canonical: `[[ |- true => true ]]`). `comptime` entry points and asynchronous entry points are not permitted (E05-803).

[5] Attributes such as `[[test]]`, `[[bench]]`, or other non-standard annotations shall not decorate `main` (E05-804).

#### §5.8.3 Execution Order

[1] Before `main` executes, all module-scope initializers in the root module and its dependencies run according to §4.6 and §5.7.

[2] `main` executes on the initial thread. Additional concurrency is implementation-defined and initiated explicitly by user code.

[3] If `main` returns normally, its `i32` result is mapped to the platform’s exit status conventions.

[4] If `main` panics or aborts, the program terminates with a non-zero exit status chosen by the implementation. Clause 11 describes destructor semantics in such cases.

#### §5.8.4 Argument Handling

[1] When the signature includes `args: [string]`, the implementation supplies command-line arguments normalized to UTF-8. Invalid data may be replaced or rejected; behavior is documented per implementation.

[2] The first argument (`args[0]`) may contain the executable path; subsequent positions carry user-provided parameters.

#### §5.8.5 Examples (Informative)

**Example 5.8.5.1 (Basic entry point):**
```cursive
public procedure main(): i32
    [[ io::write |- true => true ]]
{
    println("Hello, Cursive!")
    result 0
}
```

[1] The example shows `main` with an explicit grant declaration for I/O operations, demonstrating the standard pattern for executable programs.

**Example 5.8.5.2 (Entry with arguments):**
```cursive
public procedure main(args: [string]): i32
    [[ io::write |- args.len() > 0 => true ]]
{
    if args.len() < 2 {
        println("Usage: tool <path>")
        result 1
    } else {
        run_tool(args[1])
        result 0
    }
}
```

**Example 5.8.5.3 - invalid (Non-public main):**
```cursive
internal procedure main(): i32
{  // error[E05-802]
    result 0
}
```

### §5.8.6 Conformance Requirements [decl.entry.requirements]

[1] Implementations shall require exactly one `public procedure main` per executable program, diagnose missing or duplicate definitions (E05-801), and reject non-public entry points (E05-802). The contractual sequent follows standard optionality rules per §5.4 [decl.function].

[2] Compilers shall forbid entry points that are `comptime` procedures or asynchronous procedures (E05-803) and reject disallowed attributes on `main` (E05-804). `main` may declare any grants necessary for program execution.

[3] Before executing `main`, runtime systems shall evaluate module initialisers according to §4.6 and honour the exit-status mapping described in §5.8.3.
