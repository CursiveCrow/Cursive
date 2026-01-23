# Cursive Language Specification

## Clause 3 — Modules

**Clause**: 3 — Modules
**File**: 04-1_Module-Overview.md
**Section**: §3.1 Module Overview and File-Based Organization
**Stable label**: [module.overview]  
**Forward references**: §2.5 [lex.units], Clause 4 [decl], Clause 5 [name], Annex A §A.7 [grammar.declaration], Annex E §E.2.1 [implementation.algorithms]

---

### §3.1 Module Overview and File-Based Organization [module.overview]

#### §3.1.1 Overview

[1] A _module_ is the fundamental namespace unit in Cursive. Each compilation unit described in §2.5 defines exactly one module without additional syntax. Module boundaries determine which declarations are visible outside the file, where documentation attaches, and how Clause 5 resolves qualified names. Filesystem layout fixes the module's fully-qualified path; this clause specifies path derivation, export boundaries, and required diagnostics.

#### §3.1.2 Syntax

[1] Module paths consist of one or more identifiers separated by `::`. Examples: `main`, `utilities::math`, `core::io::file`.

[ Note: See Annex A §A.6 [grammar.declaration] for the normative `module_path` production. — end note ]

[3] Each compilation unit contributes one `module_path`; mapping rules appear in §3.1.3.

#### §3.1.3 Constraints

[1] **Single-file modules.** Each compilation unit shall define exactly one module. Attempts to amalgamate multiple files into a single module are ill-formed (diagnostic E03-001).

[2] **Manifest-defined roots.** The workspace manifest `Cursive.toml` (§1.7.6 [intro.versioning.manifest]) shall exist and shall provide a `[cursive.source]` table whose `roots` array lists every source root. Missing manifests or malformed tables provoke diagnostic E03-006. When the manifest is well-formed, every source file shall reside beneath exactly one declared root; files outside the declared roots provoke diagnostic E03-002.

[3] **Component derivation.** The relative path from the selected root to the file, minus extension, is segmented by the platform path separator. Each segment must satisfy the identifier rules of §2.3. Segments that fail this requirement elicit diagnostic E03-003.

[4] **Extension.** The canonical extension for module files is `.cursive`. Implementations may document additional extensions, but such extensions shall not alter module semantics.

[5] **Case collisions.** When two files differ only by case under a case-insensitive filesystem, the build is ill-formed (diagnostic E03-004). Implementations shall detect this condition before semantic analysis.

(5.1) **Case sensitivity error handling.** When a case-insensitive filesystem is detected and multiple files would map to the same module path when case is ignored, implementations shall:

- Emit diagnostic E03-004 listing all conflicting file paths
- Reject the compilation unit containing the ambiguous reference
- Provide suggestions for resolving the conflict (e.g., standardizing on a single case convention)

(5.2) **Cross-platform consistency.** Implementations shall warn (diagnostic E03-007, severity warning) when module paths that are distinct on case-sensitive filesystems would collide on case-insensitive filesystems. This helps developers maintain portability across platforms.

[6] **Reserved components.** Module components shall not coincide with reserved keywords (§2.3[4]). Violations produce diagnostic E03-005.

#### §3.1.4 Semantics

[1] The derived path establishes the stable label namespace `[module.<component>*]`.

[2] Items lacking `public` visibility remain confined to the defining module unless re-exported (§3.4).

[3] Documentation comments not otherwise attached to declarations bind to the enclosing module.

[4] Manifest metadata beyond the declared roots is informative only; normative behaviour flows solely from the filesystem-derived identity. Implementations shall ignore unspecified tables while still enforcing the `language.version` and `source.roots` requirements inherited from §1.7.6.

[5] Implementations shall canonicalise absolute paths after root resolution to guarantee diagnostics and tooling can identify modules uniquely.

#### §3.1.5 Examples

**Example 3.1.5.1 (Canonical layout):**

```
workspace/
├── Cursive.toml           # [cursive.source].roots = ["source"]
└── source/
    ├── main.cursive       → module path: main
    ├── utilities.cursive  → module path: utilities
    └── math/
        ├── geometry.cursive → module path: math::geometry
        └── report.cursive   → module path: math::report
```

[1] Manifest declares `source` as the root; each segment is a valid identifier.

**Example 3.1.5.2 - invalid (Reserved component):**

```
workspace/
└── source/
    └── module/
        └── type.cursive   → module component `type` is reserved
```

[2] Diagnostic E03-005 is required.

#### §3.1.6 Integration

[1] Clause 4 scopes declarations by module identity; Clause 5 performs lookup using the same paths. Annex E §E.2.1 mandates tooling support for enumerating module dependencies.

[2] Build caches may persist manifest resolution results provided they do not modify the normative mapping.

---

**Previous**: Clause 2 — Lexical Structure and Translation | **Next**: §3.2 Module Syntax (§3.2 [module.syntax])
