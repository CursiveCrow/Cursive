# Cursive Language Specification

## Clause 3 — Modules

**Clause**: 3 — Modules
**File**: 04-3_Module-Scope.md
**Section**: §3.3 Module Scope Formation
**Stable label**: [module.scope]  
**Forward references**: §2.5 [lex.units], Clause 4 [decl], Clause 5 [name], Annex E §E.2.1 [implementation.algorithms]

---

### §3.3 Module Scope Formation [module.scope]

#### §3.3.1 Overview

[1] Module scope formation defines how bindings introduced by `import`, `use`, and `public use` participate in the namespace of the enclosing module.

[2] The rules in this subclause ensure that imported modules are available for qualified references, that `use` declarations introduce explicit aliases, and that re-exports (`public use`) become part of the module’s outward interface.

[3] Scope formation interacts closely with Clause 5 (declaration binding) and Clause 6 (name resolution). Forward references are restricted to those clauses and Annex E §E.2.1.

#### §4.3.2 Constraints

[1] **Import availability.** For each `import module_path`, the referenced module shall exist and be reachable via the manifest-declared source roots (§4.1.3). Missing modules trigger diagnostic E04-205 (§4.6). When an alias is supplied, it becomes the preferred head component for subsequent qualified references.

[2] **Import idempotence.** Multiple `import` declarations referring to the same module path are permitted and shall not change semantics. Implementations may diagnose redundant imports as a quality-of-implementation feature but shall not reject them.

[3] **Alias uniqueness.** A `use` declaration shall not introduce a binding whose identifier matches an existing binding in the module scope. To avoid conflicts, use the `as` clause to provide a distinct alias. Violations raise diagnostic E04-200.

[4] **Wildcard determinism.** Expansion of `use module::path::*` shall be deterministic. If two exports from the target module map to the same identifier (after considering aliasing), diagnostic E04-201 shall be emitted and the importing module is ill-formed.

[5] **Public exposure.** `public use` shall only reference public items from the target module. Attempting to re-export an internal or private item raises diagnostic E04-204 (§4.4).

[6] **Evaluation order.** Scope formation shall treat all `use` declarations as if they were evaluated after parsing but before name resolution. No `use` may depend on bindings introduced later in the same module unless Clause 5 explicitly allows forward references.

[7] **Use prerequisites.** A `use` declaration is well-formed only if its referenced module path (or alias) resolves to an imported module in the current compilation unit. Violations emit diagnostic E04-202. This requirement prevents `use` from implicitly importing modules.

(7.1) The two-phase compilation model (§2.2 [lex.phases]) permits declarations to appear in any textual order. The prerequisite is semantic (the module must be imported) not temporal (the import need not precede the use textually). Implementations **shall** collect all `import` declarations during parsing before validating `use` declarations.

[ Note: This ensures that the complete set of imports is known before use-declaration validation proceeds, enabling forward references while maintaining deterministic resolution.
— end note ]

#### §4.3.3 Formal Rules

[1] The following inference rules capture the well-formedness of module imports and use bindings. Paragraph numbering resets at each rule for traceability.

[2]
[ Given: Module table $\mathbb{M}$ ]

$$
\frac{m \in \operatorname{dom}(\mathbb{M})}{\mathbb{M} \vdash \text{Import}(m)}\tag{WF-Import}
$$

[3] Rule WF-Import requires the referenced module path `m` to exist in the module table derived from manifest roots.

[4]
[ Given: Scope $S$, identifier $a$, binding $b$ ]

$$
\frac{a \notin S \qquad \text{binding}(b)}{S \vdash \text{UseBind}(a, b)}\tag{WF-Use-Binding}
$$

[5] Rule WF-Use-Binding requires `a` to be fresh within scope `S` and the referenced entity to be a valid binding. When freshness fails, diagnostic E04-200 applies.

[6]
[ Given: Scope $S$, identifier $a$, binding $b$ ]

$$
\frac{\text{binding}(b) \qquad \text{visibility}(b) = public}{S \vdash \text{PublicUse}(a, b)}\tag{WF-Public-Use}
$$

[7] Rule WF-Public-Use enforces that re-exported bindings originate from public items before entering the exporting scope.

[8]
[ Given: Imported module set $\mathbb{I}$ ]

$$
\frac{p \in \mathbb{I}}{\mathbb{I} \vdash \text{UsePrereq}(p)}\tag{WF-Use-Prerequisite}
$$

[9] Rule WF-Use-Prerequisite formalises constraint [7]; the module path (or alias) `p` drawn from the `use` statement must appear in the imported module set `𝕀`. When an alias is introduced, both the canonical module path and alias are inserted into `𝕀`.

#### §4.3.4 Semantics

##### §4.3.4.1 Import Semantics

[1] An `import` declaration records a dependency between the current module and the referenced module. Implementations shall ensure the referenced module is compiled (or otherwise available) before name resolution proceeds. The import table records both the canonical module path and any alias supplied by the programmer.

[2] Each import contributes entries to the module's **alias map**, a deterministic mapping from identifier → module path. The canonical module name is always inserted; when an alias is present, the alias is inserted as a distinct key that resolves to the same module path. Clause 6 consumes the alias map when performing qualified lookup so that constructs such as `stats::mean` resolve predictably.

[3] Alias-map identifiers do not introduce standalone bindings. They may only appear as the leading component of a qualified name (`alias::item`). Using an alias without a following `::` is ill-formed and should be diagnosed via the lookup rules in Clause 6.

[4] `import` does not introduce a new binding. Qualified references use the module path directly (e.g., `math::geometry::area`).

##### §4.3.4.2 Use Semantics

[3] A `use` declaration introduces bindings into the module scope according to the forms specified in §4.2.2. Each binding records: (a) the originating module path, (b) the imported identifier, and (c) any alias applied via `as`.

[4] Wildcard imports expand to the set of public items exported by the target module at scope-formation time. Expansion observes aliases declared in the exporting module; each resulting identifier behaves as if introduced by an explicit `use` of the same form.

[5] When expansion encounters an identifier already present in scope, the importer shall issue diagnostic E04-201 and treat the module as ill-formed. Implementations may suggest explicit aliases to disambiguate. `use` declarations that introduce aliases shall also update the alias map so that Clause 6 can disambiguate between module heads and value bindings.

[6] If the wildcard head component is an import alias, expansion is evaluated against the aliased module.

##### §4.3.4.3 Re-export Semantics

[1] A `public use` re-export introduces bindings into the current module scope (for local use) and simultaneously marks them for inclusion in the module’s export set. Re-exported aliases are exported under the alias name; the original name is not automatically re-exported unless an explicit binding is introduced.

[2] Re-exported bindings participate in visibility checks during downstream imports exactly as if they had been declared `public` within the module. If the referenced item is not public in the source module, diagnostic E04-204 shall be emitted at scope-formation time.

#### §4.3.5 Examples

**Example 4.3.5.1 (Alias and re-export):**

```cursive
import math::geometry as geometry
use geometry::area_of_circle as area
public use geometry::{circumference as circle_circumference}
```

[1] The identifier `area` is available within the module. `circle_circumference` is both locally accessible and exported for downstream modules.

**Example 4.3.5.2 - invalid (Alias conflict):**

```cursive
import math::geometry
use math::geometry::area_of_circle
use math::geometry::area_of_circle as area_of_circle  // error[E04-200]
```

[2] The second `use` attempts to introduce `area_of_circle` again without using `as` to provide a distinct alias, provoking diagnostic E04-200.

**Example 4.3.5.3 - invalid (Wildcard conflict):**

```cursive
import math::geometry
import math::statistics
public use math::geometry::*
public use math::statistics::*  // error[E04-201] if both export `mean`
```

[3] If both modules export an identifier named `mean`, the wildcard expansion conflicts, making the importing module ill-formed.

#### §4.3.6 Integration

[1] Clause 5 shall treat bindings introduced by `use` as declarations for the purposes of visibility checks. Clause 6 leverages the recorded origin information when resolving qualified and unqualified names.

[2] Annex E §E.2.1 extends the module resolution algorithm to include scope-formation diagnostics (E04-200, E04-201) so tooling can surface conflicts consistently.
