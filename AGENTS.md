The cursive spec is the source of truth. You will NOT make changes that cause the compiler to deviate from the specification. You will NOT make changes to the spec with express user approval.

# **CRITICAL UTF-8 SPEC WARNING: DO NOT IGNORE**

# **`docs/CursiveSpecification.md` INTENTIONALLY USES UTF-8 UNICODE JUDGEMENT / INFERENCE / TYPE-THEORY SYMBOLS AS NORMATIVE CONTENT.**

# **DO NOT ASCII-NORMALIZE IT. DO NOT BLINDLY RE-ENCODE IT. DO NOT "CLEAN UP" OR "FIX" UNICODE MATHEMATICAL SYMBOLS.**

# **PRESERVE SYMBOLS SUCH AS `Γ`, `⊢`, `⇔`, `⇓`, `∈`, `∧`, `⊥`, `⟨ ... ⟩`, `≤`, AND `≠` EXACTLY.**

# **ANY EDIT TO THE SPEC MUST PRESERVE VALID UTF-8 AND MUST NOT INTRODUCE MOJIBAKE OR REPLACEMENT CHARACTERS.**


## Code Quality
- Prefer correct, complete implementations over minimal ones.
- Use appropriate data structures and algorithms — don't brute-force what has a known better solution.
- When fixing a bug, fix the root cause, not the symptom.
- If something I asked for requires error handling or validation to work reliably, include it without asking.

Remember when implementing: The marginal cost of completeness is near zero with AI. Do the whole thing. Do it right. Do it with tests. Do it with documentation. Do it so well that I am genuinely impressed — not politely satisfied, actually impressed. Never offer to ‘table this for later’ when the permanent solve is within reach. Never leave a dangling thread when tying it off takes five more minutes. Never present a workaround when the real fix exists. The standard isn’t ‘good enough’ — it’s ‘holy shit, that’s done.’ Search before building. Test before shipping. Ship the complete thing. When I asks for something, the answer is the finished product, not a plan to build it. Time is not an excuse. Fatigue is not an excuse. Complexity is not an excuse. Boil the ocean.

---

# Cursive User Guide

This guide is the programmer-facing companion to `docs/CursiveSpecification.md`. It is organized as a handbook rather than as a conformance document: start with a working mental model, then the everyday syntax, then the advanced surfaces that make Cursive distinct.

Every claim in this guide is intended to match the specification. When this guide and the specification differ, the specification wins.

Unless a snippet is marked **Illustrative only**, it is written to be copyable as-is within the section's assumptions.

## Contents

1. [How To Read This Guide](#how-to-read-this-guide)
2. [What Counts As Built-In](#what-counts-as-built-in)
3. [Cursive For LLMs](#cursive-for-llms)
4. [Project Structure And Modules](#project-structure-and-modules)
5. [A First Program](#a-first-program)
6. [The Core Mental Model](#the-core-mental-model)
7. [Syntax And Everyday Rules](#syntax-and-everyday-rules)
8. [Types And Data](#types-and-data)
9. [Bindings, Mutation, Responsibility, And Moves](#bindings-mutation-responsibility-and-moves)
10. [Procedures, Methods, And Receivers](#procedures-methods-and-receivers)
11. [Control Flow And Patterns](#control-flow-and-patterns)
12. [Error Handling And Union Propagation](#error-handling-and-union-propagation)
13. [Capabilities And No Ambient Authority](#capabilities-and-no-ambient-authority)
14. [Permissions And Shared State](#permissions-and-shared-state)
15. [Regions And Frames](#regions-and-frames)
16. [Modal Types And Typestate](#modal-types-and-typestate)
17. [Shared State And The Key System](#shared-state-and-the-key-system)
18. [Structured Parallelism](#structured-parallelism)
19. [Async And Resumable State Machines](#async-and-resumable-state-machines)
20. [Classes, Implementations, Dynamic Objects, And Opaque Interfaces](#classes-implementations-dynamic-objects-and-opaque-interfaces)
21. [Contracts, Invariants, And Refinement Types](#contracts-invariants-and-refinement-types)
22. [Compile-Time Execution And Metaprogramming](#compile-time-execution-and-metaprogramming)
23. [Unsafe Code And FFI](#unsafe-code-and-ffi)
24. [Practical Guidance For Programmers Coming From Other Languages](#practical-guidance-for-programmers-coming-from-other-languages)
25. [How To Use This Guide With The Specification](#how-to-use-this-guide-with-the-specification)
26. [Guide-To-Spec Map](#guide-to-spec-map)

## How To Read This Guide

Read sections 4 through 12 first if you are new to Cursive. That sequence covers project layout, the entry point, the type surface, method syntax, control flow, and the ordinary error-handling style.

Then read sections 13 through 19. Those chapters cover the language surfaces that most strongly distinguish Cursive from Rust, Go, JavaScript, and C#: explicit capabilities, distinct permission regimes, regions, modal types, the key system, structured parallelism, and async as an explicit state machine.

Sections 20 through 23 are the advanced systems-programming surface: class-based abstraction, verification, compile-time execution, and FFI.

## What Counts As Built-In

Cursive has a substantial built-in surface. The specification explicitly defines names such as:

- `Context`
- `System`
- `FileSystem`
- `Network`
- `HeapAllocator`
- `ExecutionDomain`
- `Reactor`
- `Region`
- `RegionOptions`
- `CancelToken`
- `File`
- `DirIter`
- `DirEntry`
- `FileKind`
- `IoError`
- `Async`
- `Future`
- `Sequence`
- `Stream`
- `Pipe`
- `Exchange`
- `Spawned`
- `Tracked`

Everything else in this guide is either defined inside the example itself or should be read as user-defined.

That distinction matters for both people and models. If you see a name like `Point`, `Door`, `Counter`, or `Printable`, it is an example type, not a hidden standard-library type.

## Cursive For LLMs

This section compresses the highest-value correctness constraints for code generation.

### Generate code in the language that exists

Do not translate Rust, Go, JavaScript, or C# syntax into Cursive.

Do not generate:

- `fn`, `impl`, `struct`, `trait`, `pub`, `mut`, `&`, `&mut`
- `Option`, `Result`, `Box`, `Vec`, `Rc`, `Arc`
- goroutine or promise idioms as if they were built into the language
- detached implementation blocks

Cursive implementations attach to the declaration that owns them.

### Conservative generation rules

Use these defaults unless you know the surrounding codebase requires something more specific:

- Give every procedure an explicit return type.
- Prefer explicit `return` in non-unit procedures.
- Use `~>` for method calls.
- Use `.` for field and tuple access.
- Put module-scope bindings at top level with explicit type annotations.
- Use `let` unless mutation is required.
- Use `=` for movable bindings and `:=` only when you intentionally want an immovable binding.
- Treat anything not defined by the spec or by the current file as user-defined.
- Keep examples self-contained unless they are explicitly marked **Illustrative only**.

### Syntax rules that commonly trip generators

- Statements end with a newline or `;`.
- Single-line trailing commas are not allowed.
- Generic parameter lists use semicolons: `<T; U>`.
- Generic argument lists use commas: `<T, U>`.
- `if ... is { ... }` case clauses use pattern blocks, not `case:` labels.
- `extern` declarations live inside `extern "ABI" { ... }` blocks.
- Top-level mutable storage is written as `var`; there is no separate `static` keyword.

### Semantic rules that commonly trip generators

- `move` transfers responsibility, not permission.
- `const`, `shared`, and `unique` are distinct permission regimes. Do not assume implicit coercions between them.
- `shared` access is governed by the key system, including implicit acquisition and explicit `#` blocks.
- `~%` receiver access is a write-context for the key system.
- `[[dynamic]]` is an explicit fallback. It is not the default verification mode.
- Union types are unordered. `A | B` and `B | A` are the same type.
- There is no numeric subtyping. `i32` is not a subtype of `i64`.
- `?` is not a generic "unwrap result" operator. It works by comparing the operand union against the enclosing return type or async error type.
- `race` arms must all use return handlers or all use yield handlers. Do not mix the two.
- `sync` is only valid outside async-returning procedures and only for async values with `Out = ()` and `In = ()`.
- `shared $Class` is only valid when every dynamically dispatchable method on the class uses `~`.

### Checklist before you emit code

- Does every method call use `~>`?
- Is every unknown type or function either defined locally or clearly intended to be user-defined?
- Are contracts and refinements using the correct syntax?
- Did you avoid single-line trailing commas?
- If you used `?`, does the enclosing return type actually make that propagation legal?
- If you used `shared`, did you account for key acquisition, `~%`, and possible staleness after `release` or `yield release`?

## Project Structure And Modules

Cursive programs live inside a project rooted by `Cursive.toml`. There is no single-file fallback mode in the specification.

### The manifest

At minimum, a project declares an assembly.

**Copyable example**

```toml
assembly = { name = "app", kind = "executable", root = "src" }
```

The top-level manifest keys are:

- `assembly`
- `toolchain`
- `build`

`assembly` may be a single table or an array of tables. Each assembly requires:

- `name`
- `kind`
- `root`

`kind` must be one of:

- `executable`
- `library`
- `dependency`

`link_kind` is only valid when `kind = "library"`, and may be `shared` or `static`.

### Assemblies and source roots

An assembly's `root` points to its source root. The specification discovers modules from directories under that root that contain one or more `.cursive` files.

That means module structure is directory-based, not file-name-based.

- A directory with `.cursive` files is a module directory.
- All `.cursive` files in the same module directory belong to the same module.
- Subdirectories introduce submodules.

If the assembly is named `app` and its source root is `src`, then:

- `src` is the root-module directory for module `app`
- `src/io` is the module directory for `app::io`
- `src/io/http` is the module directory for `app::io::http`

### A concrete multi-file layout

**Copyable example**

```text
Cursive.toml
src/
  main.cursive
  startup.cursive
  io/
    print.cursive
```

With `assembly = { name = "app", kind = "executable", root = "src" }`, the module paths are:

- `src/main.cursive` and `src/startup.cursive` belong to module `app`
- `src/io/print.cursive` belongs to module `app::io`

This is different from languages where each file is its own module. In Cursive, the directory defines the module, and the files in that directory are compiled together for that module directory.

### Imports and `using`

`import` brings a module path into scope. `using` brings selected members or all visible members into scope.

**Copyable example**

```cursive
import app::io
using app::io::banner
using app::io::{print_banner, print_error as fail}
```

The three `using` forms are:

- `using module::name`
- `using module::{name, other as alias}`
- `using module::*`

### Visibility

The visibility keywords are:

- `public`
- `internal`
- `private`

If you omit visibility, it defaults to `internal`.

Use them like this:

- `public`: visible everywhere
- `internal`: visible within the same assembly
- `private`: visible within the same module

### Module-scope bindings

Top-level `let` and `var` declarations are module-scope storage. There is no separate `static` keyword in the language surface.

At module scope:

- type annotations are required
- `public var` is illegal
- initialization and destruction follow the module lifecycle rules from the specification

**Copyable example**

```cursive
internal let version_text: string@View = "1.0.0\n"
private var boot_count: i32 = 0
```

Use module-scope `var` sparingly. Public mutable globals are intentionally rejected.

### A small module example

**Copyable example**

`src/io/print.cursive`

```cursive
public procedure banner() -> string@View {
    return "Hello from app::io\n"
}

public procedure print_banner(move ctx: Context) -> () | IoError {
    return ctx.fs~>write_stdout(banner())
}
```

`src/main.cursive`

```cursive
using app::io::{banner, print_banner}

public procedure main(move ctx: Context) -> i32 {
    let _ = banner()

    return if print_banner(move ctx) is {
        _: () { 0 }
        _: IoError { 1 }
    }
}
```

That example demonstrates several important facts at once:

- `main` lives in the root module of the executable assembly
- cross-module references use `import` or `using`
- module-scope organization is directory-based
- ordinary error handling uses unions, not hidden exceptions

## A First Program

Cursive executable projects must provide exactly one `main` procedure.

A valid `main`:

- is `public`
- is non-generic
- takes exactly one parameter
- uses either omitted parameter mode or `move`
- accepts a `Context` bundle type
- returns `i32`

**Copyable example**

```cursive
public procedure main(move ctx: Context) -> i32 {
    return if ctx.fs~>write_stdout("Hello, Cursive\n") is {
        _: () { 0 }
        _: IoError { 1 }
    }
}
```

This small program already shows the core shape of the language:

- side effects are explicit because `main` receives `Context`
- file-system output is a capability method call
- the call result is a union, not an implicit exception
- the program returns an explicit exit code

## The Core Mental Model

If you keep the following points straight, most of the language surface becomes much easier to read.

### 1. Effects require capabilities

Cursive does not grant ambient access to the file system, network, heap, reactor, or execution domains. Effectful code receives authority explicitly, usually through `Context` or a restricted capability value.

### 2. Responsibility and permission are separate axes

Cursive separates:

- responsibility: who must eventually clean up the value
- permission: how the value may be accessed

`move` changes responsibility. It does not turn `const` into `unique`, and it does not bypass the key system.

### 3. Shared mutation is a language feature

`shared` is not "mutable aliasing if you are careful." It is a dedicated permission regime with compiler-tracked key acquisition, release, conflict analysis, and optional dynamic fallback under `[[dynamic]]`.

### 4. Stateful protocols are first-class

Modal types let you represent protocol states directly in the type system. That makes state-specific fields, methods, and transitions part of the language rather than comments or naming conventions.

### 5. Allocation can be scoped directly

Regions and frames make scoped allocation explicit. You do not have to emulate arena discipline through library conventions alone.

### 6. Parallelism is structured

`parallel`, `spawn`, and `dispatch` are part of the language. They compose with execution domains, cancellation, keys, async, and deterministic lowering rules.

### 7. Verification is built in

Contracts, invariants, and refinement types are statically checked by default. `[[dynamic]]` exists when the specification explicitly allows runtime fallback.

## Syntax And Everyday Rules

### Statements end with a newline or `;`

These are equivalent:

```cursive
let x: i32 = 1
let y: i32 = 2;
```

Use newlines by default. Use `;` when you need multiple small statements on one line or when the surrounding style requires it.

### Comments are ordinary comments

```cursive
// line comment

/* block comment */

/* nested
   /* block */
   comment */
```

Documentation comments also exist:

- `///` for item docs
- `//!` for module docs

### Method calls use `~>`

Field access and tuple access use `.`. Method calls use `~>`.

**Copyable example**

```cursive
record Point {
    x: i32,
    y: i32,

    procedure sum(~) -> i32 {
        return self.x + self.y
    },
}

procedure demo() -> i32 {
    let point: Point = Point{ x: 3, y: 4 }
    return point~>sum()
}
```

This is one of the most important syntax differences from other languages. If you generate `point.sum()`, that is wrong.

### Single-line trailing commas are not allowed

This is legal:

```cursive
Point{
    x: 1,
    y: 2,
}
```

This is not:

```cursive
Point{ x: 1, y: 2, }
```

If you are generating code automatically, the simplest safe rule is: do not emit trailing commas on single-line lists.

### Generic parameter lists and generic argument lists use different separators

Generic parameters use semicolons.

```cursive
record Pair<T; U> {
    first: T,
    second: U,
}
```

Generic arguments use commas.

```cursive
let pair: Pair<i32, bool> = Pair<i32, bool>{ first: 1, second: true }
```

### Attributes use `[[...]]`

**Copyable example**

```cursive
[[dynamic]]
public procedure checked_divide(x: i32, y: i32) -> i32
    |: y != 0 {
    return x / y
}
```

Common attributes you will see in user code include:

- `[[dynamic]]`
- `[[stale_ok]]`
- `[[emit]]`
- `[[files]]`
- `[[derive(... )]]`
- `[[export(... )]]`
- `[[host_export(... )]]`

### Reserved names and prefixes matter

The specification reserves important type and namespace names such as `Context`, `Region`, `Async`, `FileSystem`, `Reactor`, `string`, and `bytes`.

Do not shadow them in user code.

## Types And Data

### Primitive types

Cursive includes the usual scalar primitives:

- signed integers: `i8`, `i16`, `i32`, `i64`, `i128`, `isize`
- unsigned integers: `u8`, `u16`, `u32`, `u64`, `u128`, `usize`
- floating point: `f16`, `f32`, `f64`
- `bool`
- `char`
- unit: `()`
- never: `!`

There is no numeric subtyping between these primitives. `i32` and `i64` are distinct types.

### Tuples

**Copyable example**

```cursive
let pair: (i32, bool) = (42, true)
let single: (i32;) = (42;)
let unit: () = ()
```

The single-element tuple uses `;`, not a dangling comma.

### Arrays, slices, and ranges

**Copyable example**

```cursive
let values: [i32; 3] = [10, 20, 30]
let slice: [i32] = values[0..2]
let full: .. = ..
let inclusive: 0..=10 = 0..=10
```

Ranges are ordinary expression forms and are used throughout the iteration and dispatch surface.

### Records

Records are the ordinary nominal product type. They can contain fields, methods, and, when implementing classes, associated-type bindings.

**Copyable example**

```cursive
record Point {
    x: i32,
    y: i32,

    procedure sum(~) -> i32 {
        return self.x + self.y
    },
}
```

Construct records with record literals:

```cursive
let point: Point = Point{ x: 3, y: 4 }
```

### Enums

Enums are tagged sum types with named variants.

**Copyable example**

```cursive
enum Token {
    Ident(string@Managed)
    Number(i32)
    End
}
```

Unlike records, enum variants are separated by terminators, so newlines are the natural style.

### Unions

Unions are structural and unordered. `A | B` is the same type as `B | A`.

They are heavily used for error handling and protocol branching.

**Copyable example**

```cursive
let value: i32 | bool = 42

let text: string@View = if value is {
    _: i32 { "int" }
    _: bool { "bool" }
}
```

Do not treat unions as if they were named `Result` or `Option` types. The typing rules operate on the actual union members, not on a privileged library wrapper.

### Strings and bytes

The built-in string and byte families distinguish view and managed forms:

- `string@View`
- `string@Managed`
- `bytes@View`
- `bytes@Managed`

Managed construction and growth use explicit heap capability methods, for example `string::from(..., heap)` or `bytes::from_slice(..., heap)`.

That matters when you design APIs:

- read-only borrowed-style text should usually be `string@View`
- owned heap-backed text should usually be `string@Managed`
- the same distinction exists for bytes

### Safe pointers and raw pointers

Safe pointers carry state in the type:

- `Ptr<T>@Valid`
- `Ptr<T>@Null`
- `Ptr<T>@Expired`

Raw pointers are spelled:

- `*imm T`
- `*mut T`

**Copyable example**

```cursive
procedure pointer_demo() -> i32 {
    let value: i32 = 10
    let ptr: Ptr<i32>@Valid = &value
    let _ = ptr
    return value
}
```

Raw-pointer dereference requires `unsafe`.

**Copyable example**

```cursive
procedure raw_read(ptr: *imm i32) -> i32 {
    unsafe {
        return *ptr
    }
}
```

Use raw pointers only when you need raw FFI-style or manually managed memory behavior. Prefer safe pointers or structured region allocation when possible.

## Bindings, Mutation, Responsibility, And Moves

### `let` and `var`

`let` introduces an immutable binding. `var` introduces a mutable binding.

**Copyable example**

```cursive
let x: i32 = 1
var y: i32 = 2
y = y + x
```

This answers only the question "may this binding be reassigned?" It does not answer the permission question for the value itself.

### `=` and `:=`

Binding operators control movability of the binding itself:

- `=` creates a movable binding
- `:=` creates an immovable binding

**Copyable example**

```cursive
let movable: i32 = 1
let anchored: i32 := 2
var changing: i32 = 3
var pinned: i32 := 4
```

The most important use of `:=` is when the binding identity itself must remain fixed.

### `shadow`

Use `shadow` when you want a new binding with the same name instead of mutating the existing binding.

**Copyable example**

```cursive
procedure absolute(x: i32) -> i32 {
    if x < 0 {
        shadow let x: i32 = -x
        return x
    }

    return x
}
```

This preserves the "one binding, one meaning" style inside each lexical scope.

### `move` transfers responsibility

`move` passes cleanup responsibility from one binding to another or from caller to callee.

That is not the same as granting mutable access.

**Copyable example**

```cursive
procedure consume(move file: File@Read) -> File@Closed {
    return file~>close()
}
```

Read this as:

- the caller gives the callee responsibility for `file`
- the permission of `file` is determined by its type, not by `move`

### Responsibility is orthogonal to permission

A value can be:

- responsibility-bearing but accessed with `const`
- not responsibility-bearing but accessed with `unique`
- `shared` and still not be the binding responsible for cleanup

If you remember only one thing here, remember this:

`move` is about responsibility. `const`, `shared`, and `unique` are about access.

## Procedures, Methods, And Receivers

### Procedures declare a return type

Use explicit return types consistently.

**Copyable example**

```cursive
procedure add(x: i32, y: i32) -> i32 {
    return x + y
}
```

This guide uses explicit `return` in non-unit procedures as the conservative style for both humans and generators.

### Parameters can be consuming or non-consuming

A parameter without `move` leaves responsibility with the caller. A parameter with `move` transfers responsibility to the callee.

**Copyable example**

```cursive
procedure print_name(name: string@View) -> () {
    let _ = name
    return
}

procedure close_file(move file: File@Read) -> File@Closed {
    return file~>close()
}
```

### Method receivers spell the permission story

Receiver shorthand is:

- `~` for const receiver access
- `~!` for unique receiver access
- `~%` for shared receiver access

**Copyable example**

```cursive
record Counter {
    value: i32,

    procedure read(~) -> i32 {
        return self.value
    },

    procedure set(~!, next: i32) -> () {
        self.value = next
        return
    },
}
```

An explicit receiver form also exists:

```cursive
procedure keep(self: Point, other: Point) -> Point {
    let _ = other
    return self
}
```

Use explicit receivers when you need to spell the receiver type directly. Use shorthand receivers when you want the common permission-centric form.

### Caller permission compatibility

The specification defines receiver-call compatibility separately from general permission coercion. The practical rule is:

- `const` callers may call `~`
- `shared` callers may call `~` and `~%`
- `unique` callers may call `~`, `~%`, and `~!`

Do not turn that matrix into a story about implicit permission subtyping. The language does not treat the permission regimes as a general implicit coercion ladder.

### Overloading exists, but stays static

Cursive supports overloaded procedures and methods, but overload resolution is static. The compiler picks a target from visible declarations and types. There is no JavaScript-style runtime overload behavior.

## Control Flow And Patterns

### `if`, `if ... is`, and case forms

The ordinary `if` is familiar.

**Copyable example**

```cursive
procedure clamp_zero(x: i32) -> i32 {
    if x < 0 {
        return 0
    }

    return x
}
```

`if ... is` handles pattern refinement.

**Copyable example**

```cursive
procedure describe(value: i32 | bool) -> string@View {
    return if value is {
        _: i32 { "integer" }
        _: bool { "boolean" }
    }
}
```

This is the normal way to refine a union or other pattern-matchable value in local control flow.

### Loops

Cursive supports loop forms for:

- indefinite loops
- condition-based loops
- iteration over ranges or iterables

**Copyable example**

```cursive
procedure sum_to(limit: i32) -> i32 {
    var acc: i32 = 0

    loop i in 0..limit {
        acc = acc + i
    }

    return acc
}
```

Loop invariants use `|: { ... }` and are covered in the contracts section.

### `defer`

`defer` registers cleanup code that runs on scope exit.

**Copyable example**

```cursive
procedure demo_defer() -> () {
    var x: i32 = 0

    defer {
        x = x + 1
        let _ = x
    }

    x = x + 10
    return
}
```

Use `defer` for local structured cleanup. Use responsibility transfer when the cleanup owner should change entirely.

## Error Handling And Union Propagation

This is one of the most important day-to-day chapters in the language.

### The ordinary result surface is a union

Cursive does not force a special library wrapper for fallible operations. The common shape is:

- success type `T`
- error type `E`
- return type `T | E`

The built-in file-system methods follow this pattern.

### `?` works by comparing against the enclosing return type

`?` is legal on a union-typed expression when the compiler can identify exactly one union member that is not compatible with the enclosing return type.

That member becomes the local success value. The remaining member or members are propagated.

**Copyable example**

```cursive
public procedure read_config(move ctx: Context) -> string@Managed | IoError {
    let text = ctx.fs~>read_file("config.txt")?
    return text
}
```

Why this works:

- `ctx.fs~>read_file("config.txt")` has type `string@Managed | IoError`
- the enclosing return type is `string@Managed | IoError`
- `IoError` is compatible with the enclosing return type
- `string@Managed` is the unique non-error member, so `text` has type `string@Managed`

### `?` is not tied to a named `Result`

Do not read `?` as "unwrap the `Ok` variant". It is not variant-name driven. It is type-driven.

That means the legality of `?` depends on the actual union members and the actual enclosing return type.

### A direct pattern-matching alternative

If you want to handle the union locally instead of propagating it, use `if ... is`.

**Copyable example**

```cursive
public procedure print_config(move ctx: Context) -> i32 {
    return if ctx.fs~>read_file("config.txt") is {
        _: string@Managed { 0 }
        _: IoError { 1 }
    }
}
```

### Async propagation is different

Inside an async-returning procedure, `?` compares the union against the async error type `E`, not against the whole `Async<...>` type.

**Copyable example**

```cursive
public procedure load_text(move ctx: Context) -> Future<string@Managed, IoError> {
    let text = ctx.fs~>read_file("config.txt")?
    return text
}
```

Here:

- the async type is `Future<string@Managed, IoError>`
- its error type is `IoError`
- if the file read fails, the procedure produces `Async@Failed { error = e }`

### `?` in infallible async procedures

If the async error type is `!`, then propagation of a fallible union is rejected. There is nowhere for the error to go.

That is a useful design check. If you want fallible async behavior, make the async error type explicit.

### Practical guidance

Use `?` when:

- the callee's error type already belongs in your return type
- the function is mostly plumbing success values through a fallible chain

Use explicit pattern matching when:

- you need to convert or classify errors
- you want to map multiple union members to a new shape
- you need user-visible branching rather than propagation

## Capabilities And No Ambient Authority

### `Context` is the normal root capability bundle

The built-in `Context` bundle exposes fields such as:

- `fs`
- `net`
- `heap`
- `sys`
- `reactor`

The specification also defines execution-domain access through the `Context` surface for:

- CPU
- GPU
- inline execution

### Authority is visible in the signature

If a procedure performs I/O, networking, allocation, or reactor interaction, the capability path should be visible in its parameters.

**Copyable example**

```cursive
public procedure write_banner(move ctx: Context) -> () | IoError {
    return ctx.fs~>write_stdout("ready\n")
}
```

A reader can tell from the signature that the procedure has authority because `Context` is present.

### Capabilities can be attenuated

Cursive supports attenuation rather than ambient authority leakage.

**Copyable example**

```cursive
public procedure restricted_fs(move ctx: Context) -> $FileSystem {
    return ctx.fs~>restrict("/srv/app")
}
```

That is the preferred way to narrow authority for helper APIs or hosted boundaries.

### Custom context bundles

The `main` parameter may be `Context` or another type that satisfies the specification's context-bundle rules.

If you design your own bundle, keep it explicit and narrow. The guide's default recommendation is still to start with `Context` unless you have a concrete need to project a smaller bundle.

## Permissions And Shared State

### The three permission regimes

The language surface uses three permission qualifiers:

- `const`
- `shared`
- `unique`

They describe access mode, not lifetime or ownership.

### `const` means read-only access

**Copyable example**

```cursive
procedure read_only(value: const i32) -> i32 {
    return value
}
```

### `unique` means exclusive mutable access

**Copyable example**

```cursive
record Boxed {
    value: i32,

    procedure bump(~!) -> () {
        self.value = self.value + 1
        return
    },
}
```

### `shared` means synchronized shared access

`shared` values participate in the key system. Reads and writes on shared data are not ordinary unsynchronized field accesses.

This is not a library convention. It is part of the core language semantics.

### Default permission

When no permission qualifier is written, the effective default is `const`.

That default applies in many places, including explicit receiver and parameter types.

### Do not turn permission rules into ownership rules

These statements are all different:

- "this binding is responsible for cleanup"
- "this code has exclusive mutable access"
- "this access is synchronized shared access"

Cursive intentionally keeps them different so the compiler can reason locally and precisely.

## Regions And Frames

Regions are scoped allocation arenas. Frames are nested reset points inside an active region.

### A region introduces an active arena

**Copyable example**

```cursive
procedure region_demo() -> i32 {
    region as arena {
        let first = arena~>alloc(10)
        let second = arena~>alloc(20)
        let _ = first
        let _ = second
    }

    return 0
}
```

Inside `region as arena { ... }`, `arena` has type `unique Region@Active`.

If you omit `as arena`, the compiler still introduces an active region, but the binding is synthetic and cannot be named by user code.

### Region options are explicit

`region` accepts an optional options expression.

**Copyable example**

```cursive
procedure tuned_region() -> i32 {
    region(RegionOptions()) as arena {
        let value = arena~>alloc(1)
        let _ = value
    }

    return 0
}
```

### `frame` creates a nested reset scope

Use `frame` when you want nested scoped allocation inside the current active region.

**Copyable example**

```cursive
procedure frame_demo() -> i32 {
    region as arena {
        let outside = arena~>alloc(1)

        frame {
            let inside = arena~>alloc(2)
            let _ = inside
        }

        let after = arena~>alloc(3)
        let _ = outside
        let _ = after
    }

    return 0
}
```

An explicit target form also exists:

```cursive
arena.frame {
    let value = arena~>alloc(4)
    let _ = value
}
```

### When to use regions

Use regions when:

- values have a natural lexical lifetime
- you want deterministic reclamation without a general heap
- you want allocation policy visible in local code

Use the heap capability when you need heap-backed values whose lifetime is not confined to a region scope.

## Modal Types And Typestate

Modal types are first-class state machines. This is one of the most distinctive parts of the language.

### Declaring a modal type

**Copyable example**

```cursive
modal Door {
    @Closed {
        transition open(code: i32) -> @Open {
            return Door@Open{ last_code: code }
        }
    }

    @Open {
        last_code: i32

        transition close() -> @Closed {
            return Door@Closed{}
        }
    }
}
```

Each state may define:

- state-specific fields
- state-specific methods
- transitions to other states

### State-specific types are real types

`Door@Closed` and `Door@Open` are distinct types. That lets you make illegal states unrepresentable at the type level instead of by convention.

### State-specific methods

Methods can live inside a state block and only exist on that state.

**Copyable example**

```cursive
modal Light {
    @Off {
        transition turn_on() -> @On {
            return Light@On{ level: 1 }
        }
    }

    @On {
        level: i32

        procedure brightness(~) -> i32 {
            return self.level
        }

        transition turn_off() -> @Off {
            return Light@Off{}
        }
    }
}
```

### `widen` converts a specific state to the general modal family

Use `widen` when you want to forget the specific state and treat the value as the modal family type.

**Copyable example**

```cursive
procedure hide_state() -> Light {
    let on: Light@On = Light@On{ level: 3 }
    return widen on
}
```

### Why modals matter

Many languages encode protocol state using:

- enums plus conventions
- booleans plus comments
- interfaces plus runtime errors

Cursive gives you a dedicated surface. That makes state legality visible in types, method sets, and control flow.

## Shared State And The Key System

The key system is how Cursive makes `shared` access analyzable and safe.

### Most shared access acquires keys implicitly

If you read or write through a `shared` path, the compiler computes the key path and required mode and inserts the corresponding acquisition and release behavior.

In practice:

- reading shared data requires a read key
- writing shared data requires a write key
- calling a `~%` method is a write-context
- passing a `shared` value as an argument does not itself acquire a key; the callee acquires keys when it actually accesses shared data

### The key path is rooted in the accessed path

The compiler reasons about a shared access by computing its key path from the root of the place expression.

Important consequences:

- `counter.value` is keyed from `counter`
- `items[i]` is keyed from `items`
- pointer dereference is a key boundary
- for `shared $Class`, dynamic method calls use the root of the receiver as the key path

### Explicit key blocks

When you want to spell the synchronization boundary directly, use `#`.

**Illustrative only.** These examples assume `shared`-typed bindings already exist in scope.

```cursive
# counter write {
    counter.value = counter.value + 1
}
```

The general shape is:

- one or more paths
- optional modifiers: `dynamic`, `speculative`, `ordered`
- mode: `read`, `write`, `release read`, or `release write`

### `ordered` is specialized

Use `ordered` only when the paths are same-base indexed paths whose relative order matters.

**Illustrative only**

```cursive
# items[i], items[j] ordered write {
    items[i] = items[i] + 1
    items[j] = items[j] + 1
}
```

Two important rules follow from the specification:

- `ordered` is rejected for different array bases
- if the indices are already statically comparable, the compiler warns that `ordered` is redundant

### `release` temporarily gives up held keys

`release` is for the cases where you must step outside the current synchronized region and then resume it.

**Illustrative only**

```cursive
# state write {
    [[stale_ok]]
    let cached = state.value

    # state release read {
        let _ = cached
    }

    state.value = state.value + 1
}
```

Operationally, `release` does four things:

- releases the outer key set
- acquires the inner target mode
- executes the inner block
- releases the inner keys and reacquires the outer keys

Other tasks may interleave while the outer keys are released.

### Stale values after `release`

Bindings derived from shared data before a `release` block may be stale afterward.

That is a warning, not a type error. `[[stale_ok]]` suppresses the warning only. It does not make the value fresh and it does not change runtime behavior.

The same staleness rule applies across `yield release` in async code.

### Conservative callee summaries

If the compiler cannot determine a callee's shared-access summary, it warns and conservatively treats the callee as potentially writing broad subpaths under the shared argument root.

This matters most at:

- unresolved calls
- bodyless declarations
- dynamic calls with unknown bodies
- recursive situations whose access summary does not close precisely

### `[[dynamic]]` is an explicit fallback

Outside `[[dynamic]]`, the compiler must be able to prove the key discipline statically. If it cannot, the program is ill-formed.

Inside `[[dynamic]]`, the owning key rules may permit runtime synchronization instead.

Two subtle but important consequences follow:

- `[[dynamic]]` does not force runtime synchronization when static proof already succeeds
- `[[dynamic]]` is not a general opt-out from key correctness; it is a targeted fallback when the specification allows dynamic enforcement

### Dynamic classes and `shared $Class`

`shared $Class` is intentionally restricted.

If any dynamically dispatchable class method requires `~%` or `~!`, then `shared $Class` is ill-formed. A shared dynamic object is only valid when every vtable-eligible method is compatible with shared dynamic use.

That rule prevents the language from hiding unsound shared dynamic dispatch behind runtime library behavior.

## Structured Parallelism

Cursive parallelism is expression-oriented and structured.

### `parallel`

Use `parallel` to enter an execution domain and wait for the enclosed spawned work before leaving the block.

**Copyable example**

```cursive
procedure cpu_sum(move ctx: Context) -> i32 {
    return parallel ctx~>cpu() {
        let left = spawn { 1 + 2 }
        let right = spawn { 3 + 4 }
        wait left + wait right
    }
}
```

The block result becomes the result of the `parallel` expression.

### Execution domains

The built-in `Context` surface provides domain selection for:

- CPU
- GPU
- inline execution

Use them as explicit domain expressions:

- `ctx~>cpu()`
- `ctx~>gpu()`
- `ctx~>inline()`

The domain is not ambient scheduler state. It is part of the source program.

### `spawn`

`spawn` starts a child task and returns a handle.

**Copyable example**

```cursive
procedure spawn_demo(move ctx: Context) -> i32 {
    return parallel ctx~>cpu() {
        let handle = spawn { 40 + 2 }
        wait handle
    }
}
```

A `Spawned<T>` handle is observed with `wait`. Panic behavior is handled by the structured parallel semantics of the enclosing region.

### `dispatch`

`dispatch` is the data-parallel surface.

**Copyable example**

```cursive
procedure sum_range(count: i32) -> i32 {
    return dispatch i in 0..count [reduce: +] {
        i
    }
}
```

Without a reduction, a `dispatch` body is used for parallel side effects. With `[reduce: op]`, the body contributes values to a reduction result.

### Key-aware dispatch

`dispatch` can declare keyed access directly.

**Illustrative only**

```cursive
dispatch i in 0..count key items[i] write {
    items[i] = items[i] + 1
}
```

This ties data-parallel iteration to the same key reasoning model used by ordinary shared code.

### Ordered reductions and GPU execution

`dispatch [ordered]` forces ordered behavior where the specification requires it, especially for reduction semantics.

GPU execution is not a separate ad hoc subsystem. It is the same structured parallel surface routed through the GPU execution domain and the GPU-specific safety rules.

## Async And Resumable State Machines

Async is a built-in modal family, not a hidden promise transform.

### The core async type

The primary form is:

- `Async<Out, In, Result, E>`

Read the parameters as:

- `Out`: value produced when the async value suspends
- `In`: value accepted when it resumes
- `Result`: final success value
- `E`: final failure value

The built-in states are:

- `@Suspended { output }`
- `@Completed { value }`
- `@Failed { error }`

When `E = !`, `@Failed` is uninhabited.

### The built-in aliases

The specification defines these aliases:

- `Sequence<T> = Async<T, (), (), !>`
- `Future<T, E> = Async<(), (), T, E>`
- `Stream<T, E> = Async<T, (), (), E>`
- `Pipe<In, Out> = Async<Out, In, (), !>`
- `Exchange<T> = Async<T, T, T, !>`

These are not library conventions. They are built-in aliases.

### Async procedures

An async-returning procedure is still an ordinary procedure declaration whose return type is an async type.

**Copyable example**

```cursive
public procedure load_text(move ctx: Context) -> Future<string@Managed, IoError> {
    let text = ctx.fs~>read_file("config.txt")?
    return text
}
```

That declaration means:

- suspension output is `()`
- resume input is `()`
- success result is `string@Managed`
- failure result is `IoError`

### `yield`

`yield e` suspends and produces an `Out` value. The expression itself evaluates to an `In` value when the async computation resumes.

**Copyable example**

```cursive
procedure countdown() -> Sequence<i32> {
    yield 3
    yield 2
    yield 1
    return
}
```

For a `Sequence<T>`, `In = ()`, so each `yield` resumes with unit.

### `yield from`

`yield from` delegates suspension behavior to another async source.

Use it when you want the current async procedure to forward the suspended outputs of another async value instead of manually matching its states.

### Manual stepping with `resume`

When `In ≠ ()`, or when you want direct control, inspect the modal state and call `resume` on `@Suspended`.

**Illustrative only**

```cursive
if task is {
    @Suspended { output } { task = task~>resume(()) }
    @Completed { value } { value }
    @Failed { error } { error }
}
```

This is the right mental model:

- async values are stateful modal values
- suspension is visible
- resumption is explicit
- completion and failure are explicit

### Built-in async combinators

The built-in async family also includes combinator members such as:

- `map`
- `filter`
- `take`
- `fold`
- `chain`

Treat them as methods on the built-in async surface, not as an external promise library.

### `wait`

`wait` joins a spawned or tracked async computation.

The important result types are:

- `wait` on `Spawned<T>` gives `T`
- `wait` on `Tracked<T, E>` gives `T | E`

You must not `wait` while keys are held.

### `sync`

`sync` is the bridge from async back into synchronous code.

It is only valid:

- outside async-returning procedures
- when the async value has `Out = ()`
- when the async value has `In = ()`

**Copyable example**

```cursive
public procedure load_text_sync(move ctx: Context) -> string@Managed | IoError {
    return sync load_text(move ctx)
}
```

Because `load_text` returns `Future<string@Managed, IoError>`, `sync` produces `string@Managed | IoError`.

### `race`

`race` waits for the first arm to become ready and then applies a handler.

Two modes exist:

- return-handlers, which produce a union of handler result and arm error types
- yield-handlers, which produce a `Stream`

Do not mix them in one `race`.

**Illustrative only**

```cursive
let fastest: i32 | IoError = race {
    query_a() -> |value| value,
    query_b() -> |value| value,
}
```

### `all`

`all` waits for all async operands to complete. Each operand must have `Out = ()` and `In = ()`.

The result is:

- a tuple of the success results
- or the union of the error types

**Illustrative only**

```cursive
let both: (i32, i32) | IoError = all {
    read_a(),
    read_b(),
}
```

### Async and keys

Two rules matter immediately in real code:

- `yield` and `yield from` while holding keys require `release`
- `yield release` releases held keys at suspension and reacquires them on resume

That means async code and shared-state code are not separate subsystems. The language specifies exactly how they interact.

## Classes, Implementations, Dynamic Objects, And Opaque Interfaces

Classes are Cursive's abstraction surface for shared interfaces, associated types, default behavior, dynamic dispatch, and opaque returns.

If you come from Rust, do not read `class` as `trait`. If you come from C# or Java, do not read it as a heap-allocated nominal object declaration. A Cursive class defines an interface shape. Records, enums, and modals implement that shape with `<:`.

### Defining a class

**Copyable example**

```cursive
public class Measure {
    procedure width(~) -> i32

    procedure is_empty(~) -> bool {
        return self~>width() == 0
    }
}
```

This shows the two basic forms:

- an abstract procedure, which ends at the signature
- a concrete default procedure, which includes a body

Implementations may use the default body or replace it with `override`.

### Implementing a class on a record

**Copyable example**

```cursive
public class Measure {
    procedure width(~) -> i32
}

public record FixedWidth <: Measure {
    columns: i32

    procedure width(~) -> i32 {
        return self.columns
    }
}
```

The implementing declaration names the class in its `record ... <: ...` header. The implementation body is part of the record body itself.

### Associated types

Associated types let a class describe a family of related types without forcing every use site to pass them as ordinary generic arguments.

**Copyable example**

```cursive
public class Source {
    type Item

    procedure next(~!) -> Self::Item | ()
}

public record OneShot <: Source {
    value: i32
    done: bool = false

    type Item = i32

    procedure next(~!) -> i32 | () {
        if self.done {
            return ()
        }

        self.done = true
        return self.value
    }
}
```

Rules that matter in practice:

- abstract associated types in a class must be bound by each implementation
- default associated types may be inherited unchanged
- associated types participate in class contracts and dispatchability

### `override` and default methods

If a class provides a concrete default procedure, an implementation that replaces it must use `override`.

If the class procedure is abstract, using `override` is wrong. If the class procedure is concrete and you replace it without `override`, that is also wrong. Cursive makes the distinction explicit so generated code does not silently drift between "satisfy a requirement" and "replace inherited behavior."

### Modal classes

A class itself may be declared `modal`. That lets a class interface describe state-sensitive behavior in the same way modal concrete types do.

This is an advanced surface, but the practical rule is simple: if the class is modal, implementations must satisfy the class's modal requirements rather than treating it as an ordinary always-available method set.

### Dynamic class objects

Dynamic class object types are written as `$ClassName`.

Creation uses an ordinary cast:

**Illustrative only**

```cursive
let printer = banner as $Printable
let text = printer~>render()
```

The important constraints are:

- only dispatchable classes may be used as `$Class`
- methods called through `$Class` must be vtable-eligible
- generic class procedures are not vtable-eligible
- procedures whose signatures use `Self` in forbidden positions are not vtable-eligible

The runtime model is explicit in the specification: a dynamic class object is a pair of pointers, one to data and one to a vtable. The language surface still treats it as an ordinary value with `~>` method calls.

### `shared $Class` is more restricted than plain `$Class`

This is a distinctive rule worth calling out because both humans and LLMs tend to miss it.

`shared $Class` is only permitted when every vtable-eligible procedure on that class has a `const` receiver. If the class exposes `~%` or `~!` through dynamic dispatch, then `shared $Class` is rejected.

That rule prevents the language from promising keyed shared access through an erased interface that cannot safely represent the required receiver behavior.

### Opaque interfaces

Opaque types let a procedure return "some concrete type implementing this class" without exposing the concrete type to the caller.

**Copyable example**

```cursive
public class Printable {
    procedure render(~) -> string@View
}

public record Banner <: Printable {
    text: string@View

    procedure render(~) -> string@View {
        return self.text
    }
}

public procedure make_banner() -> opaque Printable {
    return Banner{ text: "hello" }
}
```

At the call site, the result exposes only the `Printable` interface:

**Illustrative only**

```cursive
let value = make_banner()
let rendered = value~>render()
```

Two operational facts matter:

- opaque values are restricted statically to the named class interface
- opaque returns add no extra runtime wrapper

Use `opaque ClassName` when you want interface-based abstraction without committing the caller to a particular implementing type.

## Contracts, Invariants, And Refinement Types

Verification is a first-class language surface in Cursive. It is not bolted on as a linter and it is not a library convention.

The default rule is critical: proof obligations are static by default. `[[dynamic]]` only enables runtime checking where the owning rules permit it.

### Procedure contracts

Procedure contracts use `|:` and can express:

- preconditions
- postconditions
- both at once with `=>`

**Copyable example**

```cursive
public procedure halve_even(x: i32) -> i32 |: x % 2 == 0 => @result * 2 == x {
    return x / 2
}
```

Read that as:

- before the body runs, `x % 2 == 0` must hold
- on success, the returned value named by `@result` must satisfy `@result * 2 == x`

### `@result` and `@entry`

Two built-in contract forms matter most:

- `@result` refers to the returned value in a postcondition
- `@entry(expr)` captures a bitcopy expression at procedure entry so you can compare entry and exit state

**Illustrative only**

```cursive
procedure consume_one(~!) -> () |: => self.count == @entry(self.count) - 1 {
    self.count -= 1
    return
}
```

`@entry` is intentionally restricted. It must be pure, capability-free, and bitcopy-typed. Do not generate arbitrary entry snapshots of managed resources or effectful expressions.

### Type invariants

Records, enums, and modals may carry invariants after the declaration body.

**Copyable example**

```cursive
public record Range {
    start: i32
    end: i32
} |: { self.start <= self.end }
```

That invariant becomes part of the type's validity story. Constructors, mutation paths, and other introduction points must preserve it.

### Loop invariants

Loop invariants use the same `|: { ... }` surface.

**Copyable example**

```cursive
procedure sum_to(limit: i32) -> i32 {
    var i = 0
    var total = 0

    loop i < limit |: { i >= 0 && total >= 0 } {
        total += i
        i += 1
    }

    return total
}
```

In practice, you add loop invariants when the verifier needs a stable fact that remains true across iterations.

### Refinement types

Refinement types constrain an existing type with a predicate.

**Copyable example**

```cursive
type Port = u16 |: { self > 0 }
```

This does not create a new runtime representation. It creates a stronger static statement about the set of valid `u16` values.

Use refinement types when:

- a plain primitive is too weak
- the constraint is local and value-based
- you want the type checker and verifier to carry that fact forward

### `[[dynamic]]` is a fallback, not the normal path

When the specification says a contract or refinement may lower to a runtime check under `[[dynamic]]`, that does not change the default model. The default model is still compile-time proof.

This matters for both people and LLMs:

- do not assume contracts are runtime assertions by default
- do not add `[[dynamic]]` casually just to make type-checking friction disappear
- use `[[dynamic]]` only when you explicitly want the specification's runtime fallback behavior

## Compile-Time Execution And Metaprogramming

Compile-time execution is a language phase, not a macro preprocessor bolted onto the side.

The high-level model is:

- Phase 2 runs compile-time code
- compile-time code may inspect types, issue diagnostics, read project files when explicitly allowed, and emit declarations
- the expanded program then continues through ordinary typing and lowering

### The core forms

The main surface forms are:

- `comptime { ... }` for a compile-time statement block
- `comptime { expression }` for a compile-time expression
- `comptime if ...`
- `comptime loop ... in ...`
- `comptime procedure ...`
- `Type::<T>` for a compile-time type value

**Illustrative only**

```cursive
comptime procedure type_name_of(target: Type) -> string@Managed {
    return introspect~>type_name(target)
}
```

### Built-in compile-time capabilities

Every compile-time context gets:

- `introspect`
- `diagnostics`

Two more capabilities are opt-in:

- `emitter`, granted by `[[emit]]`
- `files`, granted by `[[files]]`

That split is important. Reflection is always available in compile-time code, but emission and project-file access are explicitly capability-gated.

### Reflection

The `introspect` capability lets compile-time code ask structural questions about types.

The most important members are:

- `category`
- `fields`
- `variants`
- `states`
- `implements_form`
- `type_name`
- `module_path`

**Illustrative only**

```cursive
comptime {
    let type_name = introspect~>type_name(Type::<Config>)
}
```

The field-, variant-, and state-level queries are only valid for the matching category. The specification rejects applying them to the wrong kind of type.

### Quotes and splices

Quoted syntax creates compile-time AST values.

The three quote forms are:

- `quote { ... }`
- `quote type { ... }`
- `quote pattern { ... }`

Two splice rules matter most in practice:

- `$name` is only for identifier-position splicing
- `$(expr)` is the general splice form in other quoted positions

For LLMs, the main trap is trying to splice identifiers into structural positions where the grammar does not permit identifier splices, such as path segments, field labels, variant names, type parameter names, or item declaration names. The specification is strict here.

### Emission

`[[emit]]` enables the `emitter` capability inside a compile-time form.

`emitter~>emit(ast)` inserts generated declarations back into the program. The emitted value must be an item AST.

**Illustrative only**

```cursive
[[emit]]
comptime {
    let item = quote {
        public procedure generated() -> i32 {
            return 42
        }
    }

    emitter~>emit(item)
}
```

Emission order is deterministic. Items emitted by an earlier compile-time site in a module become visible to later compile-time sites in that same module.

### Project-file access

`[[files]]` enables the `files` capability.

The built-in operations are:

- `files~>read(path)`
- `files~>read_bytes(path)`
- `files~>exists(path)`
- `files~>list_dir(path)`
- `files~>project_root()`

These paths are project-root relative. Absolute paths are rejected, and normalized paths must not escape the project root.

### Derive targets

Derive targets are named compile-time transforms that can be attached with `[[derive(... )]]`.

**Illustrative only**

```cursive
derive target Announce(target: Type) {
    let type_name = introspect~>type_name(target)
    diagnostics~>warning("derive target executed")
    return
}
```

Derive contracts let a target declare ordering and validation constraints:

- `requires Name`
- `emits Name`

Those clauses do not themselves add implementations. They tell the compiler what must already be true, and what the derive target is expected to produce.

### Compile-time restrictions

Compile-time code does not get the whole runtime language unchanged.

In particular, compile-time contexts reject runtime-only constructs such as:

- capability-bearing types
- pointers and provenance-bearing values
- raw `Context`
- dynamic class objects
- ordinary runtime side effects

And the broader compile-time execution rules reject runtime constructs that do not make sense during Phase 2, such as raw FFI interaction and structured runtime concurrency. When in doubt, treat compile-time code as a separate execution domain with its own admissible value set.

## Unsafe Code And FFI

Cursive allows unsafe and foreign interaction, but it keeps the boundary explicit.

The rule is simple:

- safe code cannot perform raw-pointer operations, unchecked reinterpretation, or foreign calls
- `unsafe` marks the exact block where those operations are allowed

### `unsafe`

Use `unsafe` to perform operations whose safety cannot be proven by the ordinary language rules.

Common examples include:

- dereferencing raw pointers
- `transmute`
- calling `extern` procedures

Keep unsafe regions narrow. The design intent is that the surrounding API remains as statically checked as possible.

### Extern blocks and foreign procedures

Extern declarations live inside `extern` blocks and may carry an ABI string.

**Illustrative only**

```cursive
[[library(name: "c")]]
extern "C" {
    procedure puts(text: *imm u8) -> i32;
}

procedure print_line(text: *imm u8) -> i32 {
    unsafe {
        return puts(text)
    }
}
```

Library names are target-specific, so treat the linked library spelling as an example even though the FFI surface itself is exact.

Operational rules that matter immediately:

- extern signatures must be FFI-safe
- calling an extern procedure requires `unsafe`
- ABI strings are validated by target profile
- `[[unwind]]` controls whether boundary unwinding aborts or is caught

If `[[unwind]]` is omitted, `"abort"` is the default.

### Foreign contracts

Foreign procedures may carry foreign boundary contracts:

- `|: @foreign_assumes(...)`
- `|: @foreign_ensures(...)`

Use them to describe what the caller must guarantee and what foreign code is assumed to guarantee on return.

These are not proofs about foreign code. They are explicit foreign-contract boundaries.

### Exports and hosted exports

Cursive distinguishes between:

- raw exports via `[[export("abi")]]`
- hosted exports via `[[host_export("abi")]]`

Hosted exports are not just raw symbol exports with different spelling. They derive a foreign-visible signature from the source procedure plus a hosted-library session handle.

For handbook purposes, remember these rules:

- `[[host_export]]` requires `assembly.kind = "library"`
- a hosted export must begin with an explicit projected `Context` bundle parameter
- that leading context-bundle parameter must not be `move`
- raw `Context` is not permitted there

### FFI-facing attributes

The main FFI attributes are:

- `[[library(name: "...", kind: "...")]]`
- `[[mangle(none)]]`
- `[[mangle("name")]]`
- `[[export("abi")]]`
- `[[host_export("abi")]]`
- `[[unwind("abort")]]`
- `[[unwind("catch")]]`
- `[[ffi_pass_by_value]]`

`[[ffi_pass_by_value]]` matters when a type is both FFI-safe and a drop type but still needs to cross the boundary by value.

### Capability isolation at the FFI boundary

Raw FFI signatures must not expose capability-bearing language internals.

In particular, raw FFI and hosted-export visible signatures reject:

- `Context`
- capability classes
- dynamic class objects such as `$FileSystem`

That rule preserves the language's no-ambient-authority and capability-isolation model across the foreign boundary.

## Practical Guidance For Programmers Coming From Other Languages

### If you know Rust

- `move` transfers cleanup responsibility, not permission.
- `const`, `shared`, and `unique` are not Rust borrow forms and should not be explained with borrow-checker terminology.
- classes are closer to interface specifications than Rust traits, but `$Class` and `opaque Class` have their own rules and should not be translated directly into trait objects or `impl Trait`.
- regions and frames are explicit allocation surfaces; they are not lifetimes.

### If you know Go

- capabilities replace ambient package-level effect access.
- `spawn`, `parallel`, and `dispatch` are structured language forms, not free-form goroutine spawning.
- shared-state coordination is expressed through keys and receiver permissions, not by assuming channels or mutex libraries are the primary concurrency story.

### If you know JavaScript

- async is not a hidden promise transform.
- suspension, resumption, completion, and failure are explicit modal states.
- compile-time execution is not a build-script escape hatch; it is part of the language.
- unions are typed data alternatives, not loose dynamic values.

### If you know C#

- `class` in Cursive defines an interface and implementation contract, not an ordinary heap object declaration.
- dynamic class objects use `$Class` and explicit dispatchability rules.
- contracts and refinements are part of the type-and-verification surface, not just runtime assertions.
- capability passing is explicit rather than implicit through ambient services or globals.

## How To Use This Guide With The Specification

Use this handbook for the programmer mental model and the specification for exact legality.

A practical reading order is:

1. Read this guide through async if you are learning the language surface.
2. Jump to the matching spec chapter when you need exact syntax, typing, evaluation order, or diagnostics.
3. Use Appendix B of the specification when you need grammar-level confirmation.

When there is tension between "what seems natural" and "what the spec says," follow the spec.

For LLM-targeted workflows, the safest pattern is:

1. Generate code from the handbook's surface rules.
2. Validate unusual constructs against the mapped spec chapter.
3. Prefer explicitness over inference when receiver permissions, key acquisition, modal states, or contracts are involved.

## Guide-To-Spec Map

This guide is organized by developer workflow. The specification is organized by normative ownership. Use the following map when you need to drill down.

| Handbook Section                                                 | Primary Spec Chapters                                    |
| ---------------------------------------------------------------- | -------------------------------------------------------- |
| Project Structure And Modules                                    | Chapter 3, Chapter 7, Chapter 11                         |
| A First Program                                                  | Chapter 3, Chapter 6, Chapter 15                         |
| Syntax And Everyday Rules                                        | Chapter 4, Chapter 5, Chapter 16, Chapter 17, Chapter 18 |
| Types And Data                                                   | Chapter 8, Chapter 12, Chapter 13, Chapter 14.1-14.2     |
| Bindings, Mutation, Responsibility, And Moves                    | Chapter 6, Chapter 10, Chapter 18                        |
| Procedures, Methods, And Receivers                               | Chapter 15, Chapter 16.3                                 |
| Error Handling And Union Propagation                             | Chapter 12.8, Chapter 15, Chapter 16, Chapter 21         |
| Capabilities And No Ambient Authority                            | Chapter 6.1-6.2, Chapter 14.9                            |
| Permissions And Shared State                                     | Chapter 10, Chapter 19                                   |
| Regions And Frames                                               | Chapter 6.4-6.5, Chapter 18                              |
| Modal Types And Typestate                                        | Chapter 13.1-13.5                                        |
| Shared State And The Key System                                  | Chapter 19                                               |
| Structured Parallelism                                           | Chapter 20                                               |
| Async And Resumable State Machines                               | Chapter 21                                               |
| Classes, Implementations, Dynamic Objects, And Opaque Interfaces | Chapter 14.3-14.7                                        |
| Contracts, Invariants, And Refinement Types                      | Chapter 14.8, Chapter 15                                 |
| Compile-Time Execution And Metaprogramming                       | Chapter 22                                               |
| Unsafe Code And FFI                                              | Chapter 13.9, Chapter 16.5, Chapter 18.10, Chapter 23    |

If you are debugging a compiler error, also consult:

- Chapter 2 for diagnostic ordering and rendering
- Appendix A for the diagnostic index
- Appendix B for the complete grammar
- Appendix C for AST form names used throughout the spec
