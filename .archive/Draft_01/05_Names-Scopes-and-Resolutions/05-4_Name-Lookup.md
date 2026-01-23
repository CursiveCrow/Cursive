# Cursive Language Specification

## Clause 5 — Names, Scopes, and Resolution

**Clause**: 5 — Names, Scopes, and Resolution
**File**: 06-4_Name-Lookup.md
**Section**: §5.4 Name Lookup
**Stable label**: [name.lookup]  
**Forward references**: §3.4 [module.export], §4.6 [decl.visibility], §5.5 [name.ambiguity], §5.6 [name.predeclared], Annex A.7 [grammar.declaration]

---

### §5.4 Name Lookup [name.lookup]

#### §5.4.1 Overview [name.lookup.overview]

[1] This subclause specifies the algorithms that map identifier occurrences to their bindings: unqualified lookup (single identifiers) and qualified lookup (`Prefix::Name`). The rules honour the scope tree defined in §6.2, the shadowing semantics of §6.3, and the visibility/export rules in Clause 4.

[2] Lookup is context-agnostic: it produces a binding independent of whether the identifier is used in a value or type position. Category checks occur after lookup (for example, using a type name in an expression is diagnosed by Clause 8).

#### §6.4.2 Syntax [name.lookup.syntax]

[3] Lookup operates on identifiers and qualified names:

**Unqualified names** are:
```
<identifier>
```

**Qualified names** match the pattern:
```
<identifier> "::" <identifier> [ "::" <identifier> ... ]
```

[ Note: See Annex A §A.6 [grammar.declaration] for the normative `qualified_name` production.
— end note ]

[4] Longer chains (e.g., `pkg::module::Type::member`) are therefore part of the grammar itself; prefix resolution applies repeatedly to each `::` component (§6.4.4).

#### §6.4.3 Constraints [name.lookup.constraints]

[5] Unqualified lookup proceeds in a fixed order: current block scope → lexical ancestors → module scope → imported bindings → universe scope. Ambiguity or absence at any stage produces the diagnostics defined below.

[6] Qualified lookup requires the prefix to resolve to a module or type binding. Prefixes that resolve to other entities are rejected (diagnostic E06-402). Visibility of the suffix is enforced by the export rules in Clause 4; attempting to access a non-exported item produces E06-403.

[7] `use` bindings introduced in module scope participate in Step 4. Resolved names inherit any alias applied with `as`. Wildcard imports contribute all exported names from the target module; collisions are handled by §6.5.

[8] Universe-scope bindings (primitive types, constants, built-ins) are considered only after imported names. Implementations shall not treat predeclared names as candidates if they are explicitly listed as non-overridable in §6.6; user declarations that would mask such names shall be rejected per §6.6 (diagnostic E06-302).

#### §6.4.4 Semantics [name.lookup.semantics]

##### Unqualified Lookup

[9] The lookup algorithm searches scopes from innermost to outermost using the following procedure:

```
resolve_unqualified(name):
    // Step 1: current block scope
    if name ∈ bindings(current_scope):
        return bindings(current_scope)[name]

    // Step 2: lexical ancestors
    for scope in ancestors(current_scope):
        if name ∈ bindings(scope):
            return bindings(scope)[name]

    // Step 3: module scope
    if name ∈ bindings(module_scope):
        return bindings(module_scope)[name]

    // Step 4: imported bindings (module-level `use`)
    candidates := { b ∈ use_bindings(module_scope) | b.identifier == name }
    if |candidates| == 1:
        return candidates[0]
    if |candidates| > 1:
        error E06-400 (ambiguous identifier)  // see §6.5 for resolution strategies

    // Step 5: universe scope (predeclared identifiers)
    if name ∈ universe_scope:
        return universe_scope[name]

    error E06-401 (undefined identifier)
```

[10] Shadowed bindings in inner scopes mask outer bindings automatically because Step 1 and Step 2 encounter them first. Import aliases populate the alias map described in §4.3.4.1; they participate only when resolving qualified prefixes (Step 1 of §6.4.4) rather than as standalone bindings. The algorithm is deterministic and requires at most the lexical depth d of the reference, giving O(d) complexity.

##### Qualified Lookup

[11] A qualified reference `Prefix::Name` is resolved in two stages:

1. Resolve `Prefix` using `resolve_unqualified`.
2. Depending on the entity kind of `Prefix`:
   - **Module**: look up `Name` in the module’s export table; enforce visibility.
   - **Type**: look up associated items (methods, constants) exposed by the type; enforce access rules defined by the type’s module and visibility modifiers.

[12] Qualified chains longer than two components (e.g., `pkg::module::Type::method`) are resolved left-to-right, applying step 1 and step 2 repeatedly.

(12.1) _Intermediate failure handling._ When resolving a qualified chain `A::B::C::D`, if any intermediate component fails to resolve (e.g., `A::B` does not exist), the lookup terminates immediately and emits the appropriate diagnostic (E06-404 for missing module components, E06-405 for missing type components). Subsequent components in the chain are not evaluated. This ensures that diagnostics point to the first failing component rather than cascading errors.

(12.2) _Maximum chain length._ Implementations shall support qualified name chains of at least 32 components. Chains exceeding this limit may be rejected with diagnostic E06-406 (qualified name chain too long). This limit prevents pathological cases while accommodating deep module hierarchies.

[13] If the prefix is not a module or type, diagnostic E06-402 is emitted. If the suffix exists but is not exported, E06-403 is emitted; if it does not exist, E06-404 (module case) or E06-405 (type case) is emitted.

(13.1) _Empty module path components._ A qualified name may not contain empty components (e.g., `::Name` or `A::::B`). Such constructs are ill-formed and emit diagnostic E06-407 (empty qualified name component) during parsing, before lookup proceeds.

#### §6.4.5 Diagnostics [name.lookup.diagnostics]

[14] Lookup-related diagnostics defined in this subclause reference the structured payloads in Annex E §E.5:

| Code    | Condition                                         | Constraint |
| ------- | ------------------------------------------------- | ---------- |
| E06-400 | Ambiguous identifier (multiple imported)          | [6]        |
| E06-401 | Undefined identifier                              | [6]        |
| E06-402 | Prefix not module or type                         | [6]        |
| E06-403 | Item not exported                                 | [6]        |
| E06-404 | Module doesn't contain item                       | [13]       |
| E06-405 | Type doesn't expose associated item               | [13]       |
| E06-406 | Qualified name chain too long                     | [12.2]     |
| E06-407 | Empty qualified name component                    | [13.1]     |

[14.1] See §6.5 for disambiguation strategies for E06-400.

#### §6.4.6 Examples (Informative) [name.lookup.examples]

**Example 6.4.6.1 (Lookup order and qualification):**

```cursive
use math::geometry::{area, circumference as circle_circumference}

procedure demo(radius: f64): f64
    [[ |- true => true ]]
{
    let area = area(radius)                         // Steps 1–3 resolve local `area`
    let perimeter = circle_circumference(radius)    // Step 4 uses the alias
    let precise = math::geometry::area(radius)      // Qualified lookup
    let whole = i32(area)                           // Step 5: universe-scope primitive
    result perimeter + precise + whole
}
```

**Example 6.4.6.2 (Ambiguous import diagnosis):**

```cursive
use lib_a::process
use lib_b::process

procedure run()
    [[ |- true => true ]]
{
    // ERROR E06-400: ambiguous identifier 'process'
    let value = process()
}
```

#### §6.4.7 Conformance Requirements [name.lookup.requirements]

[15] Implementations shall implement the five-step unqualified lookup algorithm and emit diagnostics E06-400 (ambiguity) and E06-401 (undefined) as appropriate.

[16] Implementations shall enforce qualified lookup rules, including prefix validation (E06-402), visibility (E06-403), existence checks (E06-404/E06-405), chain length limits (E06-406), and empty component detection (E06-407).

[17] Implementations shall integrate module exports and `use` bindings into Step 4 without creating additional lexical scopes (§6.2). Module re-exports (`public use`) must appear exactly as exported in import tables.

[18] Implementations shall expose lookup traces to tooling (for example, IDEs) showing the scope path and final binding, to assist diagnostics and AI feedback.
