The cursive spec is the source of truth. You will NOT make changes that cause the compiler to deviate from the specification. You will NOT make changes to the spec with express user approval.

## Behavior and Expectations

- You will NOT continuously provide negative framing. When explaining or planning, avoid negative framing by default. Do not repeatedly say what a concept is not, what alternatives are rejected, or what should not be done unless the distinction is necessary to prevent a likely mistake. Prefer direct affirmative descriptions of the intended design, behavior, and next action. If a constraint matters, state it once in the relevant decision record or plan section, then proceed using the approved design without re-litigating rejected options.
- Do NOT use "stage#" or "phase#" as a naming convention; ever.
- Do NOT use vanity prefixes or suffixes unless there is an explicit need to prevent potential naming collisions, such as public facing API/ABIs.
- Spec-valid source is authoritative evidence. When the current compiler rejects, misparses, mischecks, mislowers, or miscompiles source that conforms to the authoritative specification, repair the canonical compiler implementation: parser, resolver, typechecker, lowering, runtime, diagnostics, or whichever path owns the defect. Preserve the source form unless the source itself violates the specification, has an independent code-quality defect, or the user explicitly requests a source rewrite.

# **CRITICAL UTF-8 SPEC WARNING: DO NOT IGNORE**

# **`docs/CursiveSpecification.md` INTENTIONALLY USES UTF-8 UNICODE JUDGEMENT / INFERENCE / TYPE-THEORY SYMBOLS AS NORMATIVE CONTENT.**

# **DO NOT ASCII-NORMALIZE IT. DO NOT BLINDLY RE-ENCODE IT. DO NOT "CLEAN UP" OR "FIX" UNICODE MATHEMATICAL SYMBOLS.**

# **PRESERVE SYMBOLS SUCH AS `Γ`, `⊢`, `⇔`, `⇓`, `∈`, `∧`, `⊥`, `⟨ ... ⟩`, `≤`, AND `≠` EXACTLY.**

# **ANY EDIT TO THE SPEC MUST PRESERVE VALID UTF-8 AND MUST NOT INTRODUCE MOJIBAKE OR REPLACEMENT CHARACTERS.**

## Cursive Design Contract

Cursive is a general-purpose systems programming language optimized for
machine-generated source code and human review.

A conforming design change SHOULD preserve the following principles:

1. One Correct Way.
   Where possible, each semantic operation in Cursive MUST have exactly one accepted source form.
   
   A language feature SHOULD NOT introduce aliases, shorthand forms, optional equivalent spellings, or syntactic sugar that lower to the same AST form and have identical static and dynamic semantics.
   
   An alternate source form is permitted only when it changes at least one of:
   
   1. static semantics;
   2. dynamic semantics;
   3. authority or capability requirements;
   4. ownership, movement, copying, or responsibility;
   5. synchronization behavior;
   6. suspension behavior;
   7. ABI, layout, or foreign-boundary behavior;
   8. diagnostic behavior in a way that is part of the language contract.
   
   Formatting whitespace and comments are not semantic source forms for this principle, but grammar-level alternatives are.

1. Local Reasoning.
   A reader SHOULD be able to determine the authority, mutability, ownership, copy/move/reference behavior, synchronization behavior, suspension behavior, and dynamic-check behavior of a construct from its local syntactic context and the directly referenced type/procedure signature.

2. Explicit over Implicit.
   Source constructs MUST NOT hide externally observable effects, synchronization, allocation, copying, dynamic verification, suspension, unsafe behavior, or authority acquisition.

3. Static by Default.
   Where both static and runtime mechanisms are possible, the static mechanism is the default. Runtime checks, runtime synchronization, dynamic dispatch, heap allocation, copying, and foreign trust boundaries require explicit source opt-in.

## Code Quality

- Implementations must be correct, complete, and accurate to the specification in full detail.
- Prefer correct, complete implementations over minimal ones.
- Use appropriate data structures and algorithms; do not brute-force behavior that has a known better solution.
- When fixing a bug, fix the root cause, not the symptom.
- If reliable behavior requires error handling, validation, diagnostics, or edge-case handling, include it without asking.
- Do not weaken tests, delete assertions, or alter expected behavior to match an incomplete implementation.

## Spec-Valid Source Preservation

Spec-valid source is authoritative evidence. When the current compiler rejects,
misparses, mischecks, mislowers, or miscompiles source that conforms to the
authoritative specification, repair the canonical compiler implementation:
parser, resolver, typechecker, lowering, runtime, diagnostics, or whichever path
owns the defect. Preserve the source form unless the source itself violates the
specification, has an independent code-quality defect, or the user explicitly
requests a source rewrite.

## No Shortcut Implementations

Implement requested behavior in the existing source of truth for that behavior.

Do not satisfy a task by adding a shim, adapter, wrapper, redirect, proxy, compatibility layer, duplicate implementation, special-case branch, or parallel code path unless the specification explicitly calls for that architectural boundary.

A change is incomplete or incorrect if it:

- bypasses existing incomplete or incorrect logic instead of fixing it;
- adds a new implementation of behavior that already exists elsewhere without replacing or reconciling the existing implementation;
- special-cases the observed failing input, test, fixture, command, filename, or call path instead of implementing the general rule;
- fixes downstream symptoms while leaving the upstream representation, parser, model, lowering, runtime, or API contract incorrect;
- leaves two reachable implementations of the same behavior with divergent semantics;
- preserves legacy behavior only because changing it would require touching more files;
- adds redirects, aliases, or compatibility layers to avoid updating real call sites;
- adds TODOs, placeholders, partial branches, temporary fallbacks, or dead paths instead of completing the implementation.

Wrappers, adapters, facades, redirects, and compatibility layers are allowed only when they are the correct long-term architecture, required by an external boundary, or explicitly requested. If used, they must delegate to the canonical implementation and must not hide an incomplete implementation underneath.

## Root-Cause Change Procedure

Before editing, identify:

- the canonical module, type, function, parser, lowering pass, runtime path, command, API, or data model responsible for the behavior;
- the call sites and tests that currently exercise it;
- the reason the existing implementation does not satisfy the specification.

Make the change at the source of truth. Update existing callers to use the corrected behavior naturally. Do not route around the source of truth to make only the current task pass.

If the existing abstraction is wrong, update or replace that abstraction. Do not layer a workaround on top of it.

If multiple implementations already exist, reconcile them. Prefer one canonical implementation over duplicated behavior.

## Completeness Standard

Completeness means fully satisfying the requested specification within the real constraints of the repository.

Do not intentionally scope down implementation detail, semantic coverage, diagnostics, validation, runtime behavior, verification, or documentation to reduce diff size, avoid difficult files, or finish faster.

Do not expand into unrelated work merely to appear thorough. Complete the requested behavior and the directly necessary supporting work: tests, validation, error handling, diagnostics, documentation, and integration updates.

When existing code is incomplete, legacy, approximate, or incorrect relative to the current authoritative specification, update or replace it with the spec-correct implementation. Do not preserve older behavior merely to avoid touching dependent code.

Preserve legacy behavior only when explicitly required by the current specification, compatibility contract, migration plan, or tests that encode intentional behavior. If legacy behavior must remain, isolate it clearly and route shared semantics through the canonical implementation.

## Generality Requirement

Implement the general rule described by the specification, not only the currently visible examples or tests.

Do not:

- hard-code expected outputs;
- special-case known fixtures;
- branch on test names, filenames, exact strings, or incidental input shapes;
- implement only the smallest subset needed by the current failing case;
- add test-only code paths;
- weaken assertions, delete tests, or change expected results to match incorrect behavior.

When adding tests, include cases that would fail against a wrapper-only, redirect-only, special-case, or partial implementation.

## Integration Requirement

New behavior must be integrated into the existing architecture, not attached beside it.

Prefer modifying the existing module, pass, API, command, or runtime path over creating a new one. Creating a new module is appropriate only when the existing architecture clearly lacks the required responsibility and the new module becomes the canonical owner of that responsibility.

After the change:

- existing public entry points should exercise the new behavior without special routing;
- old incorrect paths should be removed, replaced, or made to delegate to the canonical implementation;
- there should not be separate old and new behavior paths unless the specification requires versioned behavior;
- documentation and tests should describe the canonical path, not a workaround path.

## Workflow Expectations

- Think through structural work before editing. Build an internal model of the intended system, its likely future needs, and its stable end-state shape before committing to module, API, or folder changes.
- Search authoritative docs before planning or editing.
- Prefer updating or reconciling docs when a task exposes contradiction or drift.
- Do not introduce temporary architecture, placeholder layering, or "for now" structure that will predictably need to be renamed, collapsed, or moved in the next pass.
- Prefer the durable design directly, when it is consistent with current authoritative docs and real repo constraints.
- Avoid churn. Do not create intermediate organization or naming that you already know is wrong.
- For design work, preserve the pure-Cursive direction even if older docs suggest a dependency-based path.
- For implementation work, follow the Cursive module/directory model from the current specs, not older C++ module layouts.
- Do not invent commands or claim support for tools that are not present in the repo.
- Do not silently work around blocked requirements; classify and report the block.
- Finish tasks end-to-end when the repo state supports it. If the repo does not support full execution or verification, say so directly.

## Verification Requirement

Before considering the task complete:

- run the relevant tests, build, type checks, linters, or verification commands that exist in the repo;
- add or update tests for the new behavior and relevant edge cases;
- verify that existing entry points exercise the corrected implementation;
- verify that no stale duplicate implementation, fallback path, temporary shim, test-only special case, or redirect-only fix remains.

In the final response, summarize:

- the canonical implementation path that was changed;
- why the change fixes the root cause rather than a symptom;
- what tests or checks were run;
- any remaining blockers, if applicable.

## Priority Order

When instructions appear to conflict, use this priority order:

1. Current authoritative specification and user request.
2. Correctness, semantic completeness, and root-cause implementation.
3. Integration with the existing canonical architecture.
4. Tests, validation, diagnostics, and documentation required to support the behavior.
5. Minimal unnecessary churn.

A small diff is valuable only when it is also correct, complete, integrated, and durable. Do not choose a smaller diff by adding indirection, preserving incorrect behavior, or bypassing the existing implementation.

---

# Cursive User Guide

This guide is the programmer-facing companion to `docs/CursiveSpecification.md`. It is organized as a handbook rather than as a conformance document: start with a working mental model, then the everyday syntax, then the advanced surfaces that make Cursive distinct.

Every claim in this guide is intended to match the specification. When this guide and the specification differ, the specification wins.

Unless a snippet is marked **Illustrative only**, it is written to be copyable as-is within the section's assumptions.

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

## Cursive Style Guide

- Express correctness in the code, not in comments.
- Use the type system, `modal` types, contracts, invariants, and narrow
  capabilities before reaching for weaker runtime-only validation.
- Keep authority narrow. Pass only the capabilities and data that are actually used.
- Prefer safe language patterns even when they require more code.
- Treat `unsafe` and `[[dynamic]]` as deliberate boundary tools, not convenience
  escapes.
- Keep APIs small, explicit, and stable.
- Optimize for legibility during review over terseness while avoiding unnecessary
  ceremony.

## Naming

### General Rules

- Use descriptive names. Do not abbreviate unless the abbreviation is
  well-established in the problem domain.
- Preserve established acronyms and initialisms in their conventional form.
- Do not encode type information in variable names.
- Do not use name churn to simulate shadowing or ownership changes. Alias only
  with `using ... as ...` where aliasing is genuinely needed.

### Naming Matrix

| Category                                                 | Style                         | Examples                                                                                 |
| -------------------------------------------------------- | ----------------------------- | ---------------------------------------------------------------------------------------- |
| Assemblies                                               | `PascalCase`                  | `Grimoire`, `Vellum`, `Generated`, `GrimDemo`                                            |
| Modules and submodules                                   | `PascalCase` per path segment | `Grimoire::Behavior::Compiler`, `Grimoire::Frame::Loop`, `Grimoire::Inkwell::FrameGraph` |
| Directories                                              | `PascalCase`                  | `Behavior`, `Frame`, `FrameGraph`                                                        |
| Files                                                    | `PascalCase.cursive`          | `SessionConfig.cursive`, `Loop.cursive`, `FrameGraph.cursive`                            |
| Types (`record`, `class`, `modal`, `enum`, type aliases) | `PascalCase`                  | `SessionContext`, `AssetManifest`, `PlaybackState`                                       |
| Procedures and methods                                   | `camelCase`                   | `bootSession`, `buildPackage`, `extractFrame`                                            |
| Transitions                                              | `camelCase`                   | `beginPlayback`, `finishImport`, `enterEditor`                                           |
| Local variables                                          | `snake_case`                  | `frame_index`, `asset_id`, `package_root`                                                |
| Parameters                                               | `snake_case`                  | `config_path`, `frame_delta`, `device_handle`                                            |
| Public/internal instance fields                          | `snake_case`                  | `package_id`, `world_id`                                                                 |
| Private instance fields                                  | `_snake_case`                 | `_device`, `_frame_index`, `_package_cache`                                              |
| Constants and static values                              | `SCREAMING_SNAKE`             | `MAX_SUBTICKS`, `DEFAULT_TIMEOUT_MS`                                                     |
| Private static fields                                    | `_SCREAMING_SNAKE`            | `_FRAME_POOL_SIZE`, `_DEFAULT_STAGE_MASK`                                                |
| Enum variants                                            | `PascalCase`                  | `Windowed`, `BorderlessFullscreen`, `Cooked`                                             |
| Boolean variables and fields                             | predicate `snake_case`        | `is_ready`, `has_focus`, `can_present`, `should_reload`                                  |
| Boolean procedures and methods                           | predicate `camelCase`         | `isReady`, `hasFocus`, `canPresent`, `shouldReload`                                      |
| Generic type parameters                                  | `PascalCase` with `T` prefix  | `TValue`, `TState`, `TResource`                                                          |

### Acronyms and Initialisms

- Preserve well-known acronyms in their established form.
- Preferred: `SDL3Bridge`, `D3D12Device`, `UUID`, `RGBA8Texture`, `CPUTime`.
- Do not normalize established acronyms into mixed-case words such as
  `Sdl3Bridge`, `D3d12Device`, `Uuid`, or `CpuTime`.

### Naming Exceptions

- Language-mandated names may break local convention.
- The executable entry point remains `main` when required by the language.
- Foreign ABI names, serialized schema keys, file-format field names, and other
  externally defined identifiers may preserve external casing where compatibility
  requires it.
- Generated code may use narrower machine-oriented naming if required for stable,
  deterministic generation, but should still stay close to this guide when practical.

## Module, Directory, and File Organization

### Module Structure

- In Cursive, directories define modules. Every intended public or internal
  submodule must have its own directory.
- Do not treat file names as the module boundary. Multiple `.cursive` files in the
  same directory belong to the same module.
- Keep public API roots stable. Reorganize internals freely, but do not rename
  public module roots casually.

### File and Module Size

- Keep files around `~400` lines or less.
- Split earlier when a file mixes multiple responsibilities, mixes large public API
  surfaces with implementation detail, or becomes difficult to review.
- Prefer splitting by responsibility, lifecycle phase, or subsystem boundary rather
  than by arbitrary size alone.
- If a directory accumulates unrelated concepts, introduce submodules instead of
  continuing to grow a flat module.

### Special Files

- Use `Main.cursive` for executable-root source files when the file name is
  project-controlled, but the entry procedure inside remains `main`.
- Use `Api.cursive` only for thin facade or root export surfaces.
- Keep facade files small. They should coordinate exports, not accumulate deep logic.

## Formatting

### Layout

- Use `4` spaces for indentation.
- Target `100` columns maximum.
- Use same-line C/K&R braces.

```cursive
procedure buildFrame(request: FrameRequest) -> FrameReply {
    if should_skip
        return FrameReply.Skip

    let frame_reply: FrameReply = runFrame(request)
    return frame_reply
}
```

- Control-flow braces may be omitted for a single-statement body when the result is
  still immediately legible.
- Use braces when the body is multiline, nested, or likely to grow.
- Do not use alignment-based formatting that depends on manual column spacing.

### Line Breaking

- Use newlines as the default statement terminator.
- Use `;` only when multiple small statements on one line are clearly justified or
  surrounding syntax requires it.
- When a signature, argument list, type parameter list, or initializer exceeds the
  line limit, wrap to one item per line.

```cursive
procedure buildSession(
    session_context: SessionContext,
    package_registry: PackageRegistry,
    graph_registry: GraphRegistry,
    frame_config: FrameConfig
) -> Session
```

```cursive
let session: Session = buildSession(
    session_context,
    package_registry,
    graph_registry,
    frame_config
)
```

### Spacing and Blank Lines

- Put one blank line between top-level declaration groups.
- Use blank lines to separate logical phases inside longer procedures.
- Avoid vertical whitespace that does not communicate structure.
- Keep related declarations visually grouped.

## Imports and Visibility

### Import Ordering

- Order imports from most foundational to most specific.
- Put foundational and built-in imports first.
- Put engine and project imports next.
- Put aliases last.
- If an implementation module uses `using module::*`, keep it after regular imports
  and regular `using` declarations.

### `using` Rules

- `using module::*` is allowed only in internal or implementation modules.
- Never use wildcard `using` in public API modules.
- Prefer importing exact names or explicit aliases in public-facing code.
- Use `using ... as ...` only when the alias meaningfully improves clarity or avoids
  a real collision.

### Visibility

- Always write visibility explicitly where the language allows it.
- Do not rely on omitted visibility defaults for project code.
- Treat visibility as part of the API contract, not as an optional decoration.

## Type Design

### `record`, `class`, and `modal`

- Use `record` for plain value data, descriptors, configuration, snapshots, and
  other data-first structures.
- Use `class` only when shared identity, polymorphism, or reference-oriented
  behavior is actually required.
- Use `modal` for state-based code. If behavior, available fields, or allowed
  operations differ by lifecycle state, model that with `modal` types rather than
  booleans, comments, or informal conventions.
- Modal types and contracts are the preferred way to model protocols, resource
  states, runtime sessions, imports, cooking phases, and other lifecycle-heavy flows.

### Member Ordering

- Inside a type, order members from highest-level and most stable to most local:
  constants and static values, fields, invariants/contracts, factories/lifecycle,
  public API, then private helpers.
- In `modal` types, order states in lifecycle order.
- Within a state, keep transitions and state-specific public behavior near the
  state fields they govern.

## Contracts, Invariants, and Safety Semantics

### Contracts Are Mandatory Where Expressible

- If a rule about safety, range, state, ownership, lifetime, authority, or valid
  sequencing can be expressed with contracts or invariants, express it in code.
- Do not leave machine-checkable rules as comments alone.
- Prefer precise contracts over broad defensive code where the language can state
  the constraint directly.
- Public APIs, cross-module APIs, lifecycle transitions, and FFI wrappers should
  be especially strict about contracts.

### Capability Passing

- Do not pass large context bundles through ordinary code.
- Pass only the exact capabilities a procedure or method uses.
- If several capabilities repeatedly travel together at a real subsystem boundary,
  define a narrow projected context type for that boundary.
- Do not thread through broad "god context" objects for convenience.
- Capability narrowing is part of API design, not an optional cleanup pass.

### State and Validation

- Prefer state encoded in types over state encoded in booleans.
- Prefer contracts over ad hoc runtime checks when the language can express the rule.
- Prefer invariants over duplicated validation logic.
- Prefer compile-time safety and structural constraints over convention-based usage.

## `unsafe`, `[[dynamic]]`, and FFI

### `unsafe`

- `unsafe` is permitted only when safe language patterns genuinely cannot replicate
  the required behavior.
- More code or more effort is not a justification for `unsafe`.
- Keep `unsafe` blocks as small and local as possible.
- Wrap unsafe operations in safe APIs that re-establish project invariants.
- Every unsafe boundary must document ownership, lifetime, thread affinity, and
  caller obligations.

### `[[dynamic]]`

- Use `[[dynamic]]` only when the intended semantics are truly dynamic.
- Do not use `[[dynamic]]` to bypass correct static conformance.
- Do not use `[[dynamic]]` to compensate for poor API design, weak type modeling,
  or missing contracts.
- If a static formulation is possible and matches the intended behavior, use it.

### FFI Boundaries

- Isolate foreign interaction to dedicated boundary modules.
- Keep ABI-facing code thin and explicit.
- Do not let FFI concerns leak into ordinary gameplay, tooling, or simulation code.
- Prefer safe wrappers that expose project-level types and contracts instead of raw
  foreign handles or pointers.

## Procedures and API Design

### Procedure Style

- Use `camelCase` for procedures, methods, and transitions.
- Write explicit `return` statements in non-`unit` procedures.
- Keep procedures focused on one operation or one cohesive phase.
- Prefer small helper procedures over large deeply nested bodies.

### API Surface

- Prefer narrow, specific APIs over broad convenience APIs.
- Avoid parameter lists that mix unrelated concerns.
- Avoid wrappers or indirection that add no clarity, safety, or ownership boundary.
- Prefer a small number of strong, composable types over many weak convenience
  helpers.

## Module-Scope State

- Prefer immutable module-scope declarations.
- Avoid mutable module-scope state except for carefully justified runtime services
  or boundary objects.
- Name module-scope and static values with `SCREAMING_SNAKE`.
- Name private module-scope or private static values with `_SCREAMING_SNAKE`.
- Public mutable module-scope state is forbidden.

## Comments and Documentation

### Comments

- Use comments to explain why, constraints, ownership, or non-obvious intent.
- Do not narrate code that is already clear from the implementation.
- Keep comments factual and durable.
- Delete comments that become stale.

### Documentation Comments

- All public modules must have `//!` module documentation.
- All public types, procedures, methods, transitions, and exported constants must
  have `///` documentation.
- Public documentation must cover purpose, important preconditions, important
  postconditions, ownership or capability expectations, and notable failure modes.

## Review Expectations

- Code should be understandable without relying on hidden context.
- Reviewers should be able to see authority boundaries, state transitions, and
  safety constraints directly in the code.
- Prefer code that is easy to verify over code that is merely short.
- If a design relies on a rule that the language can express, the rule belongs in
  the code.
