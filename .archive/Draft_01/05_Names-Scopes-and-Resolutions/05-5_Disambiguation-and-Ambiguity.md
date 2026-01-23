# Cursive Language Specification

## Clause 5 — Names, Scopes, and Resolution

**Clause**: 5 — Names, Scopes, and Resolution
**File**: 06-5_Disambiguation-and-Ambiguity.md
**Section**: §5.5 Disambiguation and Ambiguity
**Stable label**: [name.ambiguity]  
**Forward references**: §3.2 [module.syntax], §3.4 [module.export], §4.6 [decl.visibility], §5.4 [name.lookup], Annex E §E.5 [implementation.diagnostics]

---

### §5.5 Disambiguation and Ambiguity [name.ambiguity]

#### §5.5.1 Overview [name.ambiguity.overview]

[1] This subclause defines how Cursive detects, diagnoses, and resolves ambiguous bindings uncovered by the lookup rules in §6.4. Ambiguity arises when multiple bindings with the same identifier are simultaneously visible (typically via imports or wildcard expansions). Rather than silently choosing one, Cursive requires explicit disambiguation to preserve predictability.

[2] The rules below specify when ambiguity is reported, how programmers can disambiguate (qualification, aliasing, or scope restriction), and which diagnostics implementations must emit.

#### §6.5.2 Ambiguity Detection [name.ambiguity.detection]

[3] Ambiguity occurs exactly when Step 4 of the unqualified lookup algorithm (§6.4.4) yields more than one candidate binding for the same identifier. The lookup procedure shall emit diagnostic **E06-400** with payload enumerating every contributing module or alias.

[4] Ambiguity is never resolved by declaration order, import order, or visibility precedence. If two candidates survive Steps 1–3 and differ only in origin, the diagnostic is mandatory; the program is ill-formed until disambiguated.

[5] Qualified lookup (`Prefix::Name`) is never ambiguous because the prefix identifies a single namespace. If the prefix itself is ambiguous, it triggers E06-400 during prefix resolution prior to the qualified lookup stage.

#### §6.5.3 Disambiguation Techniques [name.ambiguity.resolution]

[6] Cursive provides three orthogonal strategies for eliminating ambiguity:

- **Explicit qualification**: Write `module::Name` (or `Type::Name`) to select a particular binding. Any entity that triggered the ambiguity may be referenced this way.
- **Alias introduction**: Attach `as alias` to one or more `use` declarations so subsequent unqualified references resolve uniquely.
- **Scope restriction**: Remove or narrow the `use` declaration supplying the unwanted binding (for example, replace wildcard imports with explicit lists).

[7] Implementations shall accept the program once all ambiguous references have been rewritten using one of the strategies above and re-run lookup accordingly.

[8] Disambiguation does not alter shadowing semantics. A `shadow` declaration can mask an imported binding, but it does not resolve the ambiguity among imports; the ambiguity must be addressed before shadowing rules are considered.

#### §6.5.4 Diagnostics [name.ambiguity.diagnostics]

[9] Diagnostic **E06-400** shall include:

- The ambiguous identifier.
- The set of modules/aliases contributing candidates.
- Suggested fixes (qualification or alias introduction) as informative notes.
- Structured payload as defined in Annex E §E.5 for tooling consumption.

[10] Additional errors may cascade (e.g., “missing `use` alias” suggestions) but shall not suppress E06-400.

#### §6.5.5 Examples (Informative) [name.ambiguity.examples]

**Example 6.5.5.1 (Ambiguous imports requiring qualification):**

```cursive
use lib_a::process
use lib_b::process  // Same identifier imported twice

procedure run()
    [[ |- true => true ]]
{
    // ERROR E06-400: ambiguous identifier 'process'
    // Candidates: lib_a::process, lib_b::process
    // Suggested fixes:
    //   - qualify as lib_a::process(...)
    //   - alias: use lib_b::process as process_b
    let value = process()
}
```

[1] The ambiguity arises from importing the same identifier from two different modules. Disambiguation requires explicit qualification or aliasing.

**Example 6.5.5.2 (Resolving ambiguity via aliasing):**

```cursive
use math::geometry::*
use math::analytics::*
use math::analytics::area as analytics_area

let a = area(5.0)             // OK: geometry::area
let b = analytics_area(5.0)   // OK: analytics::area via alias
```

#### §6.5.6 Conformance Requirements [name.ambiguity.requirements]

[11] Implementations shall detect ambiguous unqualified references per §6.5.2 and emit E06-400 with the payload described above.

[12] Implementations shall honour programmer disambiguation by recomputing lookup after qualification, aliasing, or scope changes and shall not cache the prior ambiguous state.

[13] Implementations shall not attempt heuristic tie-breaking; if multiple candidates remain after all deterministic steps, the program is ill-formed until the programmer resolves the ambiguity.
