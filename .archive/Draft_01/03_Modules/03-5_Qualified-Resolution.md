# Cursive Language Specification

## Clause 3 — Modules

**Clause**: 3 — Modules
**File**: 04-5_Qualified-Resolution.md
**Section**: §3.5 Qualified Names and Resolution
**Stable label**: [module.qualified]  
**Forward references**: §2.3 [lex.tokens], §3.3 [module.scope], Clause 4 [decl], Clause 5 [name], Annex A §A.7 [grammar.declaration], Annex E §E.2.1 [implementation.algorithms]

---

### §3.5 Qualified Names and Resolution [module.qualified]

#### §3.5.1 Overview

[1] This subclause defines how qualified names are formed (`module::identifier`, `alias::identifier`) and how the resolver locates declarations across module boundaries.

[2] Resolution depends on the module table (§3.1), scope bindings (§3.3), and export sets (§3.4).

[3] Forward references: §2.3 (identifiers), Clause 5 (declaration categories), Clause 6 (unqualified lookup), Annex A §A.7 (grammar), Annex E §E.2.1 (resolution algorithm).

#### §4.5.2 Syntax

[1] Qualified names use the scope operator `::` and match one of the following patterns:

```
<module_path> "::" <identifier>
<identifier> "::" <identifier>
```

where the first form uses a full module path, and the second form uses an alias (an identifier introduced by `import module as alias`) in place of the module path.

[ Note: The normative grammar in Annex A §A.6 represents both forms as `module_path '::' ident | ident '::' ident`, treating aliases as identifiers that semantically resolve to module paths.
— end note ]

[2] `module_path` expands per §4.1.

#### §4.5.3 Constraints

[1] **Canonical head.** The head component of a qualified name shall match an imported module path or alias recorded under §4.3.4.1. Otherwise diagnostic E04-400 is issued.

[2] **Export availability.** The identifier following the head component shall exist in the export set of the head module. Missing identifiers raise diagnostic E04-404.

[3] **Ambiguity avoidance.** The resolver shall not disambiguate identically named modules. Programmers shall introduce distinct aliases; failure yields diagnostic E04-401.

[4] **Stable ordering.** When a qualified name contains multiple components (e.g., `pkg::subpkg::item`), resolution shall verify each prefix against the module table from left to right. A prefix that does not resolve emits diagnostic E04-402.

[5] **Alias immutability.** Once an alias is bound, subsequent `import ... as ...` statements shall not rebind the same alias to a different module. Violations produce diagnostic E04-403.

[6] **No implicit traversal.** Resolution stops at the first module boundary that exports the identifier. Private or internal bindings are never traversed implicitly.

#### §4.5.4 Formal Judgments

[ Given: Module table $\mathbb{M}$, export map $\mathbb{E}$ ]

$$
\frac{m \in \operatorname{dom}(\mathbb{M}) \qquad x \in \mathbb{E}(m)}{\mathbb{M}, \mathbb{E} \vdash m::x \Rightarrow \text{Binding}(m,x)}\tag{QR-Resolve}
$$

[1] Rule QR-Resolve succeeds when both the module path `m` exists and the identifier `x` is exported from `m`.

[ Given: Alias map $\mathbb{A}$ ]

$$
\frac{a \in \operatorname{dom}(\mathbb{A}) \qquad \mathbb{M}, \mathbb{E} \vdash \mathbb{A}(a)::x \Rightarrow b}{\mathbb{A}, \mathbb{M}, \mathbb{E} \vdash a::x \Rightarrow b}\tag{QR-Alias}
$$

[2] Rule QR-Alias substitutes the alias head with its canonical module path before delegating to QR-Resolve.

[ Given: Alias map $\mathbb{A}$ ]

$$
\frac{a \in \operatorname{dom}(\mathbb{A}) \qquad \mathbb{A}(a) = m}{\mathbb{A} \vdash \text{AliasStable}(a,m)}\tag{QR-Alias-Stability}
$$

[3] Rule QR-Alias-Stability ensures aliases remain bound to their original module.

#### §4.5.5 Semantics

##### §4.5.5.1 Resolution Pipeline

[1] Resolution proceeds in three stages: (a) head verification (module or alias), (b) export lookup, and (c) binding retrieval.

[2] Implementations shall surface diagnostics at the earliest failing stage. A missing head triggers E04-400 or E04-402; a missing export triggers E04-404.

##### §4.5.5.2 Diagnostics

[3] Diagnostics shall include contextual information and, where possible, heuristic suggestions:

- **E04-400** — Unrecognised module or alias head `<head>`.
- **E04-401** — Ambiguous qualified head `<component>`; add explicit aliases.
- **E04-402** — Partial module path `<prefix>` does not resolve to a module.
- **E04-403** — Alias `<alias>` already bound to `<module>`; cannot rebind.
- **E04-404** — Item `<identifier>` not exported from `<module>` (with notes indicating traversal history).

[4] Heuristic suggestions may include near-miss module names, possible aliases, or exported identifiers differing by edit distance, provided the suggestions are marked informative.

##### §4.5.5.3 Integration with Clause 6

[5] Clause 6 resolves unqualified identifiers using the bindings introduced by `use`. When source code explicitly employs `::`, resolution follows the rules in this subclause. There is no automatic fallback from unqualified names to qualified lookup.

#### §4.5.6 Examples

**Example 4.5.6.1 (Alias resolution):**

```cursive
import analytics::statistics as stats

procedure compute(metrics: Metrics): Analysis
    [[ |- true => true ]]
{
    result Analysis {
        spread: stats::variance(metrics.values),
        center: stats::mean(metrics.values),
    }
}
```

[1] `stats::variance` resolves by substituting the alias `stats` with `analytics::statistics` and applying QR-Resolve.

**Example 4.5.6.2 - invalid (Missing export):**

```cursive
import analytics::statistics

let bucket = analytics::statistics::histogram  // error[E04-404] if `histogram` is not public
```

[2] The identifier `histogram` fails the export lookup stage.

**Example 4.5.6.3 - invalid (Partial path):**

```cursive
import analytics::statistics
let m = analytics::stats::mean  // error[E04-402]; `analytics::stats` is not a module
```

[3] The resolver stops at `analytics::stats`, which does not correspond to a module path.

#### §4.5.7 Integration

[1] Annex E §E.2.1 shall expose the module table, alias map, and export sets for tooling so diagnostics can include trace data.

[2] Clause 6 diagnostic reporting shall suggest creating aliases when E04-401 occurs.

**Previous**: §4.4 Export, Import, and Re-export Interactions (§4.4 [module.export]) | **Next**: §4.6 Cycles, Initialization Order, and Diagnostics (§4.6 [module.initialization])
