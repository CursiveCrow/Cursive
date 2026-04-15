# Cursive

Cursive is a systems programming language designed for LLM writing and human
reading. The language focuses on explicit authority, explicit state, and
reviewable semantics so AI-assisted code generation produces code that is
easier for humans to inspect for safety and correctness.

This repository currently supports a private alpha release process. GitHub
prereleases are built as standalone Linux and Windows bundles that package the
compiler, runtime, required toolchain sidecars, and supporting metadata.

The canonical integration branch is `main`. Release artifacts are cut from tags
on `main`; release branches are not part of the steady-state workflow.

The build consumes vendored extern payloads under `extern/`. CI expects those
files to already be present in the repository checkout. The setup scripts are
for repopulating `extern/` when a local checkout is missing vendored content;
they are not part of the GitHub Actions build path.

Git clones and GitHub Actions checkouts that build from vendored extern content
must fetch Git LFS objects. Heavy vendored LLVM and ICU binaries and libraries
are tracked through LFS, so run `git lfs pull` after clone and before local or
CI-style validation.

Generated or bootstrap residue such as `tmp/`, `cursive-bootstrap/`,
`docker-data/`, `release-artifacts/`, and `extern/icu/install/` is intentionally
untracked local state and should not be committed.

## Alpha status

Cursive is in alpha and not spec-complete. Expect nonfunctional language constructs, constant breaks and incorrect behavior and frequent breaking changes.

## Local setup

Linux:

```bash
git lfs pull
test -f extern/tomlplusplus/include/toml++/toml.hpp || ./scripts/setup_extern_linux.sh
(
  cd cursive
  cmake --preset linux-release
  cmake --build --preset linux-release-package --parallel
)
python3 scripts/package_release.py bundle \
  --platform linux \
  --version v0.1.0-alpha.local \
  --staging-root cursive/build/linux/out \
  --output-dir release-artifacts
```

Windows:

```powershell
git lfs pull
if (-not (Test-Path 'extern\tomlplusplus\include\toml++\toml.hpp')) { ./scripts/setup_extern.ps1 -RepoRoot $PWD }
Push-Location cursive
cmake --preset windows-release
cmake --build --preset windows-release-package --parallel
Pop-Location
py -3 scripts/package_release.py bundle `
  --platform windows `
  --version v0.1.0-alpha.local `
  --staging-root cursive/build/windows/out `
  --output-dir release-artifacts
```

## Release model

- Distribution channel: private GitHub prereleases
- Platforms: Linux and Windows
- Bundle type: standalone toolchain archives
- Naming: `Cursive-X.Y.Z-alpha-{platform}.{tar.gz|zip}`

## Quick Start

### 1. Explicit Capability and Side-effects

- What the surface is in spec terms: Externally observable effects are permitted only through explicit capability values. Capability roots are introduced through `Context` or hosted-library session initialization, and effectful calls are constrained by capability-bearing parameter and receiver types.
- Why it is primary or distinctive: This is not a library convention. The authority model is part of the abstract machine and is enforced as a language-level rule for effectful behavior and call admissibility.
- Concrete use cases:
  - Building services where file, network, heap, reactor, and execution-domain access must be explicitly threaded through APIs.
  - Constraining AI-generated code so it cannot silently perform I/O or process effects without receiving the necessary capability.
  - Attenuating authority by handing a child capability only a restricted filesystem root, host restriction, quota, or execution domain.
- Key spec sections: 6.1, 14.9, 23.5, 24.4

### 2. Orthogonal Responsibility and Permission Model

- What the surface is in spec terms: Cursive separates binding responsibility and movability from access permission. The surface includes `move`, binding operators `=` and `:=`, `let`/`var`, binding state transitions such as `Moved` and `PartiallyMoved`, and the permission regimes `const`, `shared`, and `unique`.
- Why it is primary or distinctive: The spec treats responsibility transfer and aliasing control as independent axes. That is a stronger and more explicit model than mainstream ownership systems that bundle multiple concerns together.
- Concrete use cases:
  - Deterministic transfer of cleanup responsibility into functions or bindings without changing the aliasing regime of unrelated values.
  - Exclusive mutation through `unique` paths without implicit weakening to `shared` or `const`.
  - Preventing accidental use-after-move, partial-move misuse, and implicit moves from immovable bindings.
  - Writing AI-generated code where ownership transfer must be spelled out rather than guessed.
- Key spec sections: 6.3, 10.1 through 10.4, 18.2

### 3. Regions and Frames for Scoped Allocation and Reset

- What the surface is in spec terms: `region` introduces a scoped allocation arena, optionally named with `as`. `frame` creates a reset scope within an active region, either implicitly from the innermost active region or explicitly from a named region handle.
- Why it is primary or distinctive: Region-based lifetime control is part of the core statement surface, provenance model, and runtime semantics, not an afterthought. This gives Cursive a direct language-level answer to predictable allocation and reset.
- Concrete use cases:
  - Temporary allocations for parsers, frame-based game logic, or request-local scratch data.
  - Nested allocation phases where cleanup happens by region release or frame reset rather than per-object heap traffic.
  - Real-time or embedded code that needs bounded, explicit allocation behavior.
- Key spec sections: 6.4, 6.5, 18.7, 18.8, 24.6

### 4. Modal Types as First-Class State Machines

- What the surface is in spec terms: `modal` declarations define named states, state-specific fields, state-specific methods, and `transition` members. Concrete state types use `Modal@State`, and `widen` converts a concrete state value to its general modal type.
- Why it is primary or distinctive: Modal state is not encoded indirectly through enums plus ad hoc methods. The state-machine structure is a dedicated type-forming mechanism with specific typing, transition, and layout rules.
- Concrete use cases:
  - Modeling protocols and resource lifecycles where legal operations depend on the current state.
  - Encoding typestate for files, cancellation tokens, async values, regions, or other stateful resources.
  - Forcing AI-generated code to make state transitions explicit rather than relying on convention.
- Key spec sections: 13.1 through 13.5, 17.3, 24.6

### 5. Key System for Shared Access and Synchronization

- What the surface is in spec terms: `shared` permission is synchronized through the key system. The surface includes implicit keyed access, explicit `#` key blocks, read/write modes, `release`, `ordered`, `speculative`, dynamic verification, and memory-order attributes for keyed/shared accesses.
- Why it is primary or distinctive: Cursive does not treat shared mutation as an ordinary aliasing mode backed by user-space locks. The synchronization protocol is part of the language semantics and type checking for `shared`.
- Concrete use cases:
  - Coordinating safe access to shared graphs, arrays, or indexed structures without dropping to raw mutex APIs.
  - Partitioning keyed work by path or index so concurrency remains analyzable and deterministic.
  - Using speculative keyed execution for optimistic updates with required fallback semantics.
  - Making generated concurrent code follow one explicit synchronization model instead of inventing ad hoc locking.
- Key spec sections: 10.1, 10.4, 19.1 through 19.7, 21.5

### 6. Structured Parallelism and Explicit Execution Domains

- What the surface is in spec terms: `parallel` establishes an execution domain, `spawn` starts structured child work, and `dispatch` executes range-partitioned work with key-aware grouping, optional reductions, and deterministic scheduling constraints. Execution domains are explicit values such as `ctx.cpu()`, `ctx.gpu()`, and `ctx.inline()`.
- Why it is primary or distinctive: Parallelism is not just thread spawning. The language gives domains, partitioning, cancellation, dispatch determinism, and GPU topology first-class semantics.
- Concrete use cases:
  - CPU fork-join work where spawned tasks must settle at the enclosing block boundary.
  - GPU kernels or GPU-style workgroup execution expressed inside the main language rather than via a separate kernel language.
  - Deterministic keyed dispatch over index ranges that may run in parallel only when cross-iteration dependencies permit it.
  - AI-generated parallel code that must remain structured and analyzable instead of open-ended task spawning.
- Key spec sections: 20.1 through 20.8

### 7. Async as a Built-In Modal State Machine

- What the surface is in spec terms: `Async<Out, In, Result, E>` is a built-in modal type with states `@Suspended`, `@Completed`, and `@Failed`. The surface includes `yield`, `yield from`, `wait`, `resume`, async combinators, and explicit async frame lowering semantics.
- Why it is primary or distinctive: Async is not a hidden compiler transform with opaque futures. The spec exposes async as a stateful modal value with explicit suspension and resumption behavior.
- Concrete use cases:
  - Streaming and coroutine-style producers where `yield` exposes intermediate output and accepts resumed input.
  - Futures, streams, pipes, and exchanges built from one unified async shape.
  - Integrating async code with the key system through `yield release` and explicit stale-value handling.
  - Writing AI-generated async code where suspension points and resume contracts are visible in the type surface.
- Key spec sections: 21.1 through 21.5

### 8. Static-by-Default Contracts, Invariants, and Refinement Types

- What the surface is in spec terms: Procedures, types, and loops can carry contract clauses and invariants. Refinement types use `T |: { predicate }`. Verification is static by default; failed proofs outside `[[dynamic]]` are ill-formed, while failed proofs inside `[[dynamic]]` lower to runtime checks.
- Why it is primary or distinctive: This is a language-integrated verification surface, not just a runtime assertion API. The spec defines proof obligations, purity rules, generated facts, `@result`, `@entry`, and dynamic fallback.
- Concrete use cases:
  - Stating preconditions and postconditions that either prove statically or reject the program.
  - Encoding value constraints such as bounds or protocol facts directly in types.
  - Carrying loop invariants and branch-generated facts into later reasoning.
  - Constraining AI-generated code to emit machine-checkable safety conditions rather than comments or informal assumptions.
- Key spec sections: 14.8, 15.4 through 15.8

### 9. Compile-Time Execution, Reflection, and Emission

- What the surface is in spec terms: `comptime` statements, expressions, conditionals, loops, and procedures execute in Phase 2. The compile-time system includes reflection, quote/splice, `TypeEmitter`-based emission, file capabilities, and derive targets with explicit `requires` and `emits` contracts.
- Why it is primary or distinctive: Compile-time execution is a distinct phase with its own capabilities, restrictions, ordering, hygiene, and emitted-program visibility rules. It is closer to a constrained language subsystem than to a simple macro facility.
- Concrete use cases:
  - Generating declarations from reflected type shape while remaining inside a spec-defined compile-time environment.
  - Writing hygienic metaprograms that construct AST fragments rather than stringly source rewrites.
  - Implementing derivation workflows that depend on explicit ordering contracts between derive targets.
  - Enabling AI-generated boilerplate or wrappers to be emitted through typed compile-time code rather than duplicated manually.
- Key spec sections: 22.1 through 22.5

## Supporting Surface

These features are important parts of Cursive, but they are less differentiating than the surfaces above or mostly serve them:

- Classes, implementations, associated types, and dynamic class objects provide ordinary abstraction and dispatch machinery, including capability-class and execution-domain plumbing. Key sections: 14.3 through 14.7.
- Records, enums, unions, tuples, arrays, slices, ranges, patterns, and generics provide the ordinary data-model surface expected of a systems language. Key sections: 12, 14.1 through 14.2, 17.
- Safe pointers `Ptr<T>@Valid/@Null/@Expired` and raw pointers `*imm T` / `*mut T` provide two different pointer surfaces: tracked safe-pointer states and explicitly `unsafe` raw dereference. Key sections: 13.8 through 13.9.
- FFI is a major boundary feature, but its main role is preserving Cursive’s capability, contract, and unwind rules at foreign boundaries rather than defining the language’s everyday programming model. Key sections: 23.1 through 23.7.
- Foundational classes and predicates such as `Bitcopy`, `Clone`, `Drop`, `FfiSafe`, `Eq`, `Iterator`, and `Step` shape legality and lowering, but they are more support infrastructure than the primary differentiating surface. Key sections: 14.10, 24.5.
- Lexical security, diagnostic infrastructure, and strong conformance/lowering rules materially affect compiler and tool behavior, but they are not the primary user-facing programming surface. Key sections: 1, 2, 4.1 through 4.3, 24.
