# Cursive Language Specification

## Clause 3 — Modules

**Clause**: 3 — Modules
**File**: 04-4_Export-Import.md
**Section**: §3.4 Export, Import, and Re-export Interactions
**Stable label**: [module.export]  
**Forward references**: §2.5 [lex.units], §3.3 [module.scope], Clause 4 [decl], Clause 5 [name], Annex A §A.7 [grammar.declaration], Annex E §E.2.1 [implementation.algorithms]

---

### §3.4 Export, Import, and Re-export Interactions [module.export]

#### §3.4.1 Overview

[1] This subclause specifies how modules expose declarations to other modules, how `use` bindings participate in the export set, and which diagnostics govern attempts to re-export inaccessible items.

[2] Clause 4 governs the declarations themselves; this clause focuses on how visibility modifiers and `public use` statements determine the observable surface of a module.

[3] Forward references: §2.5 (compilation units), §4.3 (scope formation), Clause 5 (declaration visibility), Clause 6 (name resolution), Annex A §A.7 (grammar), Annex E §E.2.1 (module resolution algorithm).

#### §4.4.2 Constraints

[1] **Visibility gate.** A declaration contributes to the module’s export set if and only if it is declared `public` or is re-exported via `public use`. All other declarations are confined to the defining module (§5.6).

[2] **Re-export access.** `public use module::item` is well-formed only when `item` is exported by `module`. Violations emit diagnostic E04-204 and the re-export is rejected.

[3] **Alias stability.** When `public use` renames an item, the alias becomes the exported identifier. The original name is exported only if another binding (local or re-export) supplies it.

[4] **Conflict detection.** Two exported bindings in the same module shall not share the same identifier. To avoid conflicts, use the `as` clause in `public use` declarations to provide distinct aliases. Re-export collisions (including wildcard expansion) provoke diagnostic E04-203.

[5] **Wildcard exports.** `public use module::*` exports every public item that survives wildcard expansion after applying §4.3.4.2. The importing module is responsible for resolving conflicts introduced by wildcards.

[6] **Import alias exports.** Import aliases created with `import module as alias` do not enter the export set automatically. To expose the alias, the module shall declare a `public use` that binds the alias explicitly.

[7] **Metadata propagation.** Re-exported declarations retain documentation comments, diagnostics metadata, and stability annotations from their original definition unless supplemented with informative local annotations that do not contradict the source.

[8] **Visibility chain.** If module `A` re-exports an item from module `B`, and `B` re-exported it from module `C`, visibility is determined by the final `public` status in `C` and by every intermediate `public use`. Any non-public link diagnoses E04-204 at the point of failure.

[9] **Modules with no public exports.** A module may have an empty export set (no `public` declarations and no `public use` re-exports). Such modules are well-formed and may be imported for their side effects (e.g., module-level initialisation) or internal use, but contribute no bindings to importing modules' namespaces. No diagnostic is emitted for empty export sets.

#### §4.4.3 Formal Judgments

[ Given: Module export set $\mathbb{E}$, declaration $d$ ]

$$
\frac{\text{visibility}(d) = public}{\mathbb{E} \vdash \text{AddExport}(d)}\tag{WF-Export}
$$

[1] Rule WF-Export adds a declaration to the export set when the declaration itself is `public`.

[ Given: Scope $S$, binding $b$, alias $a$ ]

$$
\frac{S \vdash \text{UseBind}(a,b) \qquad \text{visibility}(b) = public}{\mathbb{E} \vdash \text{AddReexport}(a,b)}\tag{WF-Reexport}
$$

[2] Rule WF-Reexport promotes a `public use` binding into the export set under alias `a`.

[ Given: Export set $\mathbb{E}$, identifier $a$ ]

$$
\frac{a \in \mathbb{E}}{\mathbb{E} \vdash \text{RejectDup}(a)}\tag{WF-Reexport-Conflict}
$$

[3] Rule WF-Reexport-Conflict captures constraint [4]; duplicate exported identifiers trigger diagnostic E04-203 unless resolved via `as` aliases.

#### §4.4.4 Semantics

##### §4.4.4.1 Export Set Construction

[1] Implementations build a module’s export set after scope formation (§4.3). Each `public` declaration in the module adds itself (WF-Export). Re-exported items contribute according to WF-Reexport.

[2] Export sets are ordered only for presentation; semantically they behave as maps from identifiers to bindings.

[3] When two bindings map to the same exported identifier, the module is ill-formed (WF-Reexport-Conflict). Implementations shall report E04-203 and may offer suggestions such as introducing aliases.

##### §4.4.4.2 Qualified Access

[4] A module may access its own exported items via unqualified names and may access other modules’ exports via qualified names (`module::item`), subject to Clause 6 lookup rules.

[5] Import aliases do not alter the exported identifier; they merely shorten the qualified path within the importing module.

##### §4.4.4.3 Re-export Metadata

[6] Re-exported bindings retain diagnostic obligations (for example, required contracts) and stability annotations. Tooling shall attribute documentation to both the originating module and the re-exporting module.

[7] Informative annotations attached locally (such as additional documentation comments) may supplement but shall not contradict the originating documentation. When conflicts arise, implementations shall prefer the originating declaration for normative content while retaining local annotations as informative notes.

#### §4.4.5 Examples

**Example 4.4.5.1 (Selective re-export):**

```cursive
// module analytics::reporting
import analytics::statistics as stats
public use stats::{mean, median as middle_value}

public record Report { values: [f64] }

public procedure summarize(report: Report): ReportSummary
    [[ |- true => true ]]
{
    result ReportSummary {
        mean: mean(report.values),
        median: middle_value(report.values),
    }
}
```

[1] `mean` is exported under its original name; `median` is exported as `middle_value`. Both originate from `analytics::statistics`.

**Example 4.4.5.2 - invalid (Private re-export):**

```cursive
// module data::store
import data::backend
public use backend::connection_pool  // error[E04-204] if `connection_pool` is not public
```

[2] The re-export fails because `connection_pool` is not public in `data::backend`.

**Example 4.4.5.3 - invalid (Duplicate export):**

```cursive
// module math::geometry
public procedure area(circle: Circle): f64
    [[ |- true => true ]]
{ ... }

import math::legacy as legacy
public use legacy::{area as legacy_area}

// accidental duplicate
public use legacy::{area}  // error[E04-203]
```

[3] The final `public use` re-exports `area` again, colliding with the existing `public procedure area`.

#### §4.4.6 Integration

[1] Clause 6 relies on the export set to answer qualified lookups. Annex E §E.2.1 shall record export-set construction so tooling exposes re-export origins.

[2] Diagnostics introduced here: E04-203 ("duplicate exported identifier") supplements E04-204 ("non-public re-export") and the wildcard conflict diagnostics defined earlier in this clause.
