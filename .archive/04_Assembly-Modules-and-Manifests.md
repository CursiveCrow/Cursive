# Cursive Language Specification

## Chapter 4 — Assemblies, Modules, and Manifests

**Part**: IV — Modules and Organization
**File**: 04_Assembly-Modules-and-Manifests.md
**Stable label**: [module]
**Forward references**: §2.3 [lex.phases], Chapter 5 [decl], Chapter 6 [name], Chapter 7 [type], Annex A [grammar], Annex E §E.2 [implementation.algorithms]

---

## 4.1 Orientation [module.orientation]

### 4.1.1 Purpose

Chapter 4 defines how Cursive source code is organized into projects, assemblies, modules, and manifests. The chapter also explains how modules expose declarations, how other modules reference them, and how initialization order remains deterministic. Every rule in this chapter exists to reinforce Cursive's design philosophy: explicit effects, visible structure, and predictable runtime behavior.

### 4.1.2 Entity Stack

| Entity | Role | Required Artifact |
| --- | --- | --- |
| Project | Optional umbrella that coordinates multiple assemblies, dependency policy, and shared tooling defaults. | `Project.toml` |
| Assembly | Minimal unit of compilation and distribution. Own manifest, source roots, dependency list, build artifacts. | `Cursive.toml` |
| Module | Fundamental namespace unit. Derived from folders or manifest declarations. Hosts declarations, import table, export set, initializer. | Directory containing `*.cursive` files or manifest entry |
| Manifest | Declarative description of identity, layout, grants, and dependencies. Two manifest flavors exist (project and assembly). | TOML file |

### 4.1.3 Design Axioms

1. **Folder truth** — Folder boundaries define module identity unless the manifest states otherwise. A file never belongs to multiple modules.
2. **Qualified-only access** — Cursive never performs implicit name injection. Modules must expose `public` items and consumers must reference them via `module::path::item` or a declared alias.
3. **Assembly boundary** — Modules inside the same assembly are inside the same trust bubble and may reference one another without imports. Anything outside the assembly boundary requires an explicit import that ties back to a manifest entry.
4. **Deterministic initialization** — Module initializers execute exactly once and only after their eager dependencies succeed. Cycles on eager edges are always rejected.
5. **Manifest authority** — Tooling, build systems, and language features treat manifests as the single source of truth for layout, dependencies, and tool metadata. Any derived information must be reproducible from the manifest and the file tree.

### 4.1.4 Specification Map

- **§4.2** introduces project manifests and cross-assembly coordination.
- **§4.3** specifies assembly manifests, source roots, dependency declarations, and optional metadata.
- **§4.4** defines how modules are discovered, validated, and merged across multiple files.
- **§4.5–§4.7** describe scope formation: exports, imports, aliases, and qualified name resolution.
- **§4.8** formalizes the initialization graph and associated diagnostics.
- **§4.9–§4.10** consolidate diagnostics and conformance requirements.

---

## 4.2 Projects [module.project]

### 4.2.1 Manifest Placement and Format

A project manifest must be named `Project.toml` and must live at the project root directory. The manifest uses TOML v1.0.0 syntax. Nested project manifests are forbidden: once a directory contains `Project.toml`, no descendant directory may contain another project manifest.

### 4.2.2 Required Fields

Projects must declare the following information.

**Project metadata.**

```toml
[project]
name = "analytics_platform"
```

`name` follows module-identifier lexical rules (§2.3 [lex.tokens]). Tooling uses the value only for display; the name does not enter code namespaces.

**Assembly membership.**

```toml
[project]
members = ["core", "cli", "tests"]
```

- Entries are relative paths from the project root to assembly directories.
- Each entry must contain exactly one `Cursive.toml`.
- Member paths must be pairwise disjoint; no member may be ancestor or descendant of another (diagnostic **E04-015**).
- Missing or malformed assembly manifests provoke **E04-012**.

### 4.2.3 Optional Sections

**Shared dependencies.**

```toml
[project.dependencies]
std = "1.1"
math = "2.3"
```

Project-level dependency entries provide default version constraints. An assembly may inherit a version by specifying `"*"` for the version field inside `[dependencies]`. Assemblies may also override the project version or declare dependencies not listed at the project level. Attempting to inherit a dependency that the project did not declare emits **E04-013**.

**Build defaults.**

```toml
[project.build]
output_dir = "target"
optimization = "release"
```

These values seed assembly defaults; assembly manifests may override them.

**Metadata.**

```toml
[project.metadata]
authors = ["Team"]
repository = "https://example.invalid/repo"
```

Metadata keys are tool-defined. The specification imposes no semantics beyond preserving the entries.

### 4.2.4 Cross-Assembly Dependencies

Assemblies inside the same project refer to one another via path dependencies listed in their own `Cursive.toml` files:

```toml
[dependencies]
core = { path = "../core" }
```

Constraints:

- The path must point to another member listed in `project.members`.
- The target assembly manifest must declare the same `version` that dependents expect. Version mismatches provoke **E04-014**.
- The directed graph formed by cross-assembly dependencies must be acyclic (**E04-014**).

### 4.2.5 Project Semantics

1. Implementations discover assemblies exclusively through `project.members`; there is no globbing or implicit detection.
2. All member assemblies compile in a single workspace context, enabling shared incremental caching and consistent dependency resolution.
3. Project-level dependency defaults are resolved before assembly-level overrides so that diagnostics can point to the precise location of conflicting requirements.
4. When any assembly changes, implementations must rebuild dependent assemblies following the dependency order derived from path dependencies.

### 4.2.6 Diagnostics

| Code | Summary |
| --- | --- |
| **E04-012** | Listed member path missing or lacking `Cursive.toml`. |
| **E04-013** | Assembly attempts to inherit a dependency that the project did not declare. |
| **E04-014** | Circular or version-inconsistent assembly dependency. |
| **E04-015** | Project member paths overlap (ancestor/descendant). |

Each diagnostic payload must include the offending path and the manifest span that introduced it (Annex E §E.5).

### 4.2.7 Example

```
analytics_project/
├── Project.toml
├── core/
│   └── Cursive.toml
├── cli/
│   └── Cursive.toml
└── tests/
    └── Cursive.toml
```

```toml
# Project.toml
[project]
name = "analytics_project"
members = ["core", "cli", "tests"]

[project.dependencies]
std = "1.0"
logging = "^2.4"

[project.build]
output_dir = "build"
optimization = "debug"
```

Every assembly may now inherit `std` or `logging` by using `"*"` inside `[dependencies]`.

---

## 4.3 Assembly Manifests [module.assembly]

### 4.3.1 Location and Format

Each assembly directory must contain exactly one `Cursive.toml` at its root. Missing or malformed manifests provoke **E04-006**. Assemblies do not nest; subdirectories cannot contain their own assembly manifest unless they are listed as independent project members.

### 4.3.2 Required Sections

**Assembly identity.**

```toml
[assembly]
name = "analytics_core"
version = "1.2.0"
```

- `name` obeys the identifier rules from §2.3.
- Names must be unique across the dependency graph. Collisions with dependency names emit **E04-009**.
- `version` uses semantic versioning `MAJOR.MINOR.PATCH`.

**Language requirement.**

```toml
[assembly.language]
version = "1.0"
```

The `language.version` entry states the minimum language version required by the assembly.

**Source roots.**

```toml
[source]
roots = ["source", "tests"]
```

- Every entry must exist and be a directory (**E04-007**).
- Roots must be disjoint (no ancestor/descendant relationship). Violations emit **E04-008**.
- All `*.cursive` files included in the build must lie under exactly one root; files outside any root provoke **E04-002**.

### 4.3.3 Dependencies

The `[dependencies]` table enumerates external assemblies.

```toml
[dependencies]
std = "1.0"
http = "^2.1"
math = { path = "../math" }
```

- Version dependencies refer to registry packages; implementations must verify that selected versions satisfy the constraints.
- Path dependencies refer to local assemblies (project members or standalone directories).
- The dependency graph formed by assemblies must be acyclic (**E04-014**).
- Unresolved dependencies emit **E04-600**.

### 4.3.4 Module Composition Overrides

By default, folders determine modules (see §4.4). Assemblies may override this using the `[modules]` table.

```toml
[modules]
core = { files = [
    "source/core/**/*.cursive",
    "source/shared/types.cursive"
] }

math = { files = ["source/math/**/*.cursive"],
         exclude = ["source/math/**/*_bench.cursive"] }
```

- Keys in `[modules]` become module paths.
- `files` accepts glob patterns evaluated relative to the assembly root.
- `exclude` removes glob-matching files after the `files` set is expanded.
- Referencing a file that does not exist or does not end with `.cursive` emits **E04-011**.
- Modules not mentioned in `[modules]` are derived from folders normally.

### 4.3.5 Optional Assembly Sections

```toml
[build]
output_dir = "target"
optimization = "release"

[metadata]
license = "MIT"
authors = ["Analytics Team"]
```

Build and metadata entries behave like their project counterparts and never affect language semantics.

### 4.3.6 Assembly Semantics

1. The manifest defines the assembly boundary. All modules under the listed roots belong to this assembly and are implicitly visible to each other through qualified names.
2. When computing default module namespaces, implementations prepend nothing; module paths are relative to the assembly root. External consumers refer to the assembly by the dependency name used in their manifest.
3. Assemblies establish the authority for grants (Chapter 5) and symbol export sets. Name resolution (§4.7) always respects assembly boundaries.

### 4.3.7 Diagnostics

| Code | Summary |
| --- | --- |
| **E04-002** | Source file is outside all declared roots. |
| **E04-003** | Module component violates identifier rules. |
| **E04-004** | Case-insensitive collision between module paths. |
| **E04-005** | Module component equals a reserved keyword. |
| **E04-006** | Assembly manifest missing or malformed. |
| **E04-007** | Root path does not exist or is not a directory. |
| **E04-008** | Source roots overlap. |
| **E04-009** | Assembly name collides with dependency name. |
| **E04-010** | Warning: module paths differ only in case (portability). |
| **E04-011** | Manifest references non-existent module file. |
| **E04-016** | File assigned to multiple modules (folder-derived or manifest-specified). |
| **E04-600** | Dependency unresolved or violates version constraint. |

### 4.3.8 Example Manifest

```toml
[assembly]
name = "analytics_engine"
version = "2.1.0"

[assembly.language]
version = "1.0"

[source]
roots = ["source", "tests"]

[modules]
http_client = {
    files = [
        "source/http/request.cursive",
        "source/http/response.cursive",
        "source/http/client.cursive"
    ]
}

[dependencies]
std = "1.0"
statistics = "3.2"
database = { path = "../database-layer" }

[build]
output_dir = "build"
optimization = "release"

[metadata]
authors = ["Analytics Team"]
license = "MIT"
```

---

## 4.4 Module Topology [module.topology]

### 4.4.1 Default Derivation Algorithm

For every source root `S`:

1. Recursively scan directories under `S`.
2. Each directory `D` containing at least one `*.cursive` file defines a module whose path is the relative directory path (replace separators with `::`).
3. Any `*.cursive` file that resides directly under `S` (or under a directory that otherwise contains no `*.cursive` siblings) defines a **single-file module**. Its module path is the relative file path without the `.cursive` suffix, with intermediate directories translated to `::`.
4. All files assigned to a derived module stay in that module unless overridden by `[modules]`.

Single-file modules therefore do not require a dedicated folder. A file such as `source/math.cursive` forms module `math`, while `source/tools/build.cursive` forms module `tools::build`.

### 4.4.2 Explicit Composition

When `[modules]` provides file lists, the compiler trusts the manifest:

- The module path equals the table key.
- Files listed may live anywhere under the assembly root.
- No file may belong to more than one module; implementations must emit **E04-016** if a folder-derived rule and an explicit `[modules]` entry (or two manifest entries) both claim the same file.
- Manifest entries always shadow folder-derived modules that would otherwise map to the same path. Implementations must diagnose the overlap with **E04-016** instead of silently merging the file sets.

### 4.4.3 Component Validity

Every path component must satisfy identifier rules from §2.3. Reserved keywords (e.g., `module`, `procedure`, `type`) are disallowed. Violations emit **E04-003** or **E04-005** depending on the failure mode. Components are case-sensitive; implementations must detect case collisions per §4.3.7.

### 4.4.4 Multi-File Modules

All files within the same module share a single namespace:

1. Declarations from every file are parsed, type-checked, and merged into one module scope.
2. Items marked `public` become part of the export set (§4.5).
3. Items without `public` remain module-private but are visible to every file in the same module.
4. There is no file-private isolation; modules, not files, are the visibility boundary.

**Example.**

```cursive
// source/http/request.cursive
public record Request {
    url: string@View,
    method: HttpMethod,
}

// source/http/response.cursive
public record Response {
    status: i32,
    body: string@Managed,
}

// source/http/client.cursive
public procedure send(req: Request): Response
    [[ net::send |- true => result.status >= 100 ]]
{
    let raw = send_internal(req)
    result internal_parse(raw)
}
```

All three files contribute to module `http`. `internal_parse` may stay module-private while `send` is exported.

### 4.4.5 Examples

```
source/
├── math/
│   ├── geometry/
│   │   ├── shapes.cursive
│   │   └── area.cursive
│   └── algebra/
│       └── solve.cursive
└── utils/
    └── helpers.cursive
```

Module paths: `math::geometry`, `math::algebra`, `utils`.

### 4.4.6 Diagnostics

All diagnostics listed in §4.3.7 apply to module topology. In addition, implementations must emit **E04-016** whenever a file appears in multiple module definitions (whether through overlapping folders or overlapping `[modules]` entries). Implementations should also provide case-insensitive collision warnings (**E04-010**) even on case-sensitive filesystems to aid portability.

---

## 4.5 Visibility and Export Sets [module.visibility]

### 4.5.1 Internal Scope Construction

A module scope includes:

1. All declarations made in files belonging to the module.
2. Import aliases declared in the module (see §4.6).
3. Qualified references to other modules inside the same assembly (always available without imports).
4. Qualified references to imported external modules.

Local (unqualified) identifiers always resolve to declarations inside the same module. Everything else requires a qualified path.

**Field vs. procedure syntax.** Field or tuple member access always uses `.`, while invoking module-level procedures or referencing exported symbols uses `::`. This distinction keeps value semantics (dot) separate from module navigation (double colon).

### 4.5.2 Export Set Formation

- Only declarations marked `public` appear in the export set.
- Exported identifiers must be unique; duplicates emit **E04-203** with both source locations.
- Module-private items remain available to every file within the module but never escape the module boundary.

### 4.5.3 Shadowing

Local declarations override qualified paths only for unqualified references. Qualified names always target the referenced module. Example:

```cursive
let PI = 3.14
let precise = math::constants::PI
```

### 4.5.4 Formal Rules

Let $S$ be the set of in-module declarations, $	ext{vis}(x)$ the visibility of declaration $x$, and $\mathbb{E}$ the export set.

$$
\frac{x \in S \quad \text{vis}(x) = public}{\mathbb{E} \vdash x}
\quad [Export]
$$

$$
\frac{x, y \in \mathbb{E} \quad name(x) = name(y)}{\text{Reject}(x, y)}
\quad [Export-Conflict]
$$

### 4.5.5 Example

```cursive
// source/data/processing/types.cursive
public record Dataset { values: [f64] }

// source/data/processing/ops.cursive
let THRESHOLD = 1e-6

public procedure filter(data: Dataset): Dataset
    [[ |- true => true ]]
{
    // THRESHOLD visible here because both files share the module
}
```

Export set: `{Dataset, filter}`.

---

## 4.6 Import and Alias Model [module.import]

### 4.6.1 Access Model Summary

- Intra-assembly modules require no import; imports may still provide aliases for readability.
- External assemblies must be declared in `[dependencies]` and imported before use.
- No unqualified imports exist; all references remain qualified.

### 4.6.2 Syntax

```
import module::path [as alias]
```

`module::path` must refer either to an imported assembly or to an intra-assembly module. `alias`, if present, must be a valid identifier and must not collide with an existing alias (**E04-200**).

### 4.6.3 External Modules

Importing an external module performs two checks:

1. The dependency name matches an entry in `[dependencies]`; otherwise emit **E04-205**.
2. The remainder of the module path resolves inside that dependency; otherwise emit **E04-202**.

After a successful import, both the canonical module path and any alias become valid qualified-name heads.

### 4.6.4 Intra-Assembly Aliases

Importing a module defined inside the same assembly is optional and only creates an alias. The module is already visible through its fully-qualified path. Aliases cannot be rebound (**E04-403**).

### 4.6.5 Evaluation Order

Implementations record import declarations after lexical analysis (Chapter 2) and before name resolution. Failed imports do not enter the module scope.

### 4.6.6 Diagnostics

| Code | Summary |
| --- | --- |
| **E04-200** | Alias collision inside a module. |
| **E04-202** | Referenced module path does not exist. |
| **E04-205** | External dependency missing from manifest. |
| **E04-403** | Alias rebinding attempt. |

### 4.6.7 Example

```cursive
import std::io
import std::collections::vector as vec
import analytics::data::processing as proc  // intra-assembly alias

public procedure summarize(path: string@View)
    [[ std::io::read, std::io::write |- true => true ]]
{
    let contents = std::io::read_to_string(path)
    let values = proc::parse(contents)
    let buffer: vec::Vector<f64> = vec::Vector::new()
    buffer::extend(values)
    std::io::println("count = {}", buffer::len())
}
```

---

## 4.7 Qualified Name Resolution [module.qualified]

### 4.7.1 Resolution Phases

1. **Head evaluation.** Determine whether the first component is:
   - An intra-assembly module.
   - An alias defined by an import declaration.
   - A dependency name imported from `[dependencies]`.
   Missing heads emit **E04-400**.
2. **Module walk.** Validate each successive component. Prefix failures emit **E04-402**.
3. **Export lookup.** Ensure the final identifier is present in the target module's export set and marked `public`. Failure emits **E04-404**.

### 4.7.2 Aliases

When an alias forms the head of the path, the resolver substitutes the canonical target module before continuing the walk. Aliases never change visibility.

### 4.7.3 Examples

```cursive
// Intra-assembly access without import
let area = math::geometry::area_of_circle(r)

// External access with alias
import std::collections::vector as vec
let buffer = vec::Vector::new()
```

### 4.7.4 Diagnostics

| Code | Summary |
| --- | --- |
| **E04-400** | Qualified path head is unknown (missing import). |
| **E04-402** | Some prefix of the module path does not resolve. |
| **E04-404** | Target identifier not exported from module. |

---

## 4.8 Module Initialization Graph [module.init]

### 4.8.1 Model

Each module owns a single initializer that evaluates its module-level `let`/`var` bindings (across all files) and executes module-scope `comptime` blocks. Implementations build a directed graph $G = (V, E)$ where vertices are modules and edges represent initialization dependencies.

### 4.8.2 Edge Construction

Create an edge $m \to n$ when any of the following occur in module $m`:

- A module-level initializer reads a binding exported by $n`.
- A `comptime` block in $m` queries bindings from $n`.
- Contracts, witnesses, or diagnostics evaluated at module scope mention bindings from $n`.

### 4.8.3 Eager vs. Lazy Edges

Edges are classified as **eager** when the referenced binding must be evaluated before $m`'s initializer can complete. Examples: reading constants, constructing data, or invoking functions at module scope. All other edges remain **lazy** (e.g., storing type names, referencing procedure signatures without calling them).

The eager subgraph must be acyclic (**E04-500**). Lazy edges may cycle freely.

### 4.8.4 Ordering and Execution

1. Perform a topological sort over the eager subgraph.
2. Initialize modules in that order. Modules connected only through lazy edges may initialize in any order once their eager predecessors finish.
3. Each module initializer executes at most once. If an initializer fails, dependent modules remain uninitialized and implementations must emit **E04-501**.
4. Accessing bindings from a module that has not finished initialization emits **E04-502**.

### 4.8.5 Diagnostics

| Code | Summary |
| --- | --- |
| **E04-500** | Eager dependency cycle. Payload lists cycle nodes and justification for each edge. |
| **E04-501** | Module initialization blocked by failed predecessor. |
| **E04-502** | Attempted access to bindings from an uninitialized module. |

### 4.8.6 Examples

**Valid ordering.**

```
config::loader  →  analytics::engine (eager)
math::constants → math::geometry (lazy)
```

A permissible initialization order is `config::loader`, `analytics::engine`, followed by `math::constants` and `math::geometry` in any order that respects their eager edges.

**Rejected cycle.**

```cursive
// input::parser
import output::formatter
let TABLE = output::formatter::build_table()

// output::formatter
import input::parser
let INVERSE = input::parser::create_inverse()
```

Both edges are eager, so the cycle triggers **E04-500**.

---

## 4.9 Diagnostic Index [module.diagnostics]

| Code | Section | Condition |
| --- | --- | --- |
| **E04-002** | §4.3 | Source file outside declared roots. |
| **E04-003** | §4.3 | Module component violates identifier rules. |
| **E04-004** | §4.3 | Case-insensitive module collision. |
| **E04-005** | §4.3 | Module component matches reserved keyword. |
| **E04-006** | §4.3 | Assembly manifest missing/malformed. |
| **E04-007** | §4.3 | Source root missing/invalid. |
| **E04-008** | §4.3 | Overlapping source roots. |
| **E04-009** | §4.3 | Assembly name collides with dependency name. |
| **E04-010** | §4.3 | Case-only distinction warning. |
| **E04-011** | §4.3 | Non-existent module file referenced. |
| **E04-016** | §4.4 | File assigned to multiple modules. |
| **E04-012** | §4.2 | Project member path invalid. |
| **E04-013** | §4.2 | Dependency inherited without project entry. |
| **E04-014** | §4.2–§4.3 | Assembly dependency cycle or version mismatch. |
| **E04-015** | §4.2 | Member paths overlap. |
| **E04-200** | §4.6 | Import alias collision. |
| **E04-202** | §4.6 | Module path unresolved. |
| **E04-203** | §4.5 | Duplicate exported identifier. |
| **E04-205** | §4.6 | External dependency missing from manifest. |
| **E04-400** | §4.7 | Unknown qualified path head. |
| **E04-402** | §4.7 | Qualified path prefix unresolved. |
| **E04-403** | §4.6 | Alias rebinding forbidden. |
| **E04-404** | §4.7 | Identifier not exported. |
| **E04-500** | §4.8 | Eager initialization cycle. |
| **E04-501** | §4.8 | Initialization blocked by failed predecessor. |
| **E04-502** | §4.8 | Accessed binding from uninitialized module. |
| **E04-600** | §4.3 | Dependency unresolved. |

---

## 4.10 Conformance Requirements [module.conformance]

An implementation conforms to Chapter 4 when it:

1. Parses and validates `Project.toml` and `Cursive.toml` files exactly as specified, enforcing placement, required fields, and constraints.
2. Derives modules from folder structure or manifest overrides while rejecting invalid identifiers, missing files, and duplicate ownership.
3. Treats each module as the atomic visibility boundary, honoring `public` exports and refusing to inject unqualified names.
4. Requires explicit imports for every external assembly while allowing optional aliases for intra-assembly modules.
5. Resolves qualified names using the three-phase algorithm and emits diagnostics for unresolved heads, prefixes, or exports.
6. Builds module initialization graphs, classifies eager edges, rejects eager cycles, and enforces deterministic initialization order.
7. Emits every diagnostic listed in §4.9 with the payload schema described in Annex E §E.5.

---

**Previous**: Chapter 3 — Basic Concepts (§3.6 [basic.binding]) | **Next**: Chapter 5 — Declarations (§5.1 [decl.overview])
