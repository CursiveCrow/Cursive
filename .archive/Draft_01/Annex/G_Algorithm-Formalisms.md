# Cursive Language Specification

## Annex G — Algorithm Formalisms [algorithms]

Forward references: §4 [module], §5.6 [decl.visibility], §6.4 [name.lookup], Annex A §A.6 [grammar], Annex E §E.5 [implementation.diagnostics]

---

### G.1 Assembly, module table, and external imports [algorithms.modules]

This annex specifies normative algorithms for building the module table within a single assembly (project) and for resolving external assemblies referenced by `import`. Within an assembly, every module is always available via qualified access without an `import`. `import` is required only for external assemblies/libraries.

#### G.1.1 Definitions

- Assembly. The build unit defined by the workspace manifest (`Cursive.toml`). The manifest declares one or more source roots (§4.1) and, implementation‑defined, may declare external dependencies.
- Module table 𝕄. A finite map from fully qualified module paths to module metadata constructed from the assembly’s source roots.
- External assembly table 𝔼𝔸. A finite map from external assembly identifiers to their exported module tables. Construction of 𝔼𝔸 is implementation‑defined but must be deterministic and documented.

#### G.1.2 Module table construction (intra‑assembly)

Input: manifest roots R = {r₁,…, rₙ}; file system F.

Output: module table 𝕄.

Algorithm (deterministic):

```
function build_module_table(R, F):
    M := {}                     // empty map
    for each root r in R in lexicographic order:
        for each file f under r where extension ∈ {".cursive"} sorted lexicographically:
            s := relative_path_without_extension(f, r)
            comps := split_on_path_sep(s)
            if not all(is_identifier(c) for c in comps):
                error E04-003 (invalid module path component)
            mpath := join_with("::", comps)
            if mpath ∈ M:
                error E04-004 (case/alias collision) and continue
            M[mpath] := metadata_for(f)
    return M
```

Side conditions:

- `is_identifier` must conform to Annex A §A.1 ASCII identifier rules (Option B from §2 vs Annex A mismatch resolution).
- Implementations must diagnose case‑insensitive collisions before semantic analysis (E04‑004).

#### G.1.3 External assembly import resolution

`import A::p::q [as alias]` refers to a module path under external assembly `A`. Resolution proceeds as:

1) Resolve `A` in 𝔼𝔸 to obtain an external module table 𝕄_A.
2) Verify that `p::q` ∈ dom(𝕄_A). If not, emit E04‑205 (unresolved external module).
3) Record an alias (if any) into the alias map 𝔄 as `alias ↦ (A::p::q)`.

Notes:

- `import` must not be used for intra‑assembly modules; qualified access is sufficient within the assembly.
- Alias identifiers introduced by `import` do not become unqualified bindings; they serve only as qualified heads (§G.3).

---

### G.2 Export‑set construction (no re‑exports) [algorithms.exports]

In the absence of `use`/`public use`, a module’s export set ℰ(m) contains exactly the declarations in module m that are marked `public` by §5.6.

Judgment:

$$
\frac{\text{decl}(d) \land \text{module}(d)=m \land \text{visibility}(d)=\text{public}}{d \in \mathcal{E}(m)}\quad[\text{WF-Export}]
$$

Diagnostics:

- Duplicate exported identifiers within a single module are ill‑formed: emit E04‑203.

---

### G.3 Qualified name resolution (intra‑ and inter‑assembly) [algorithms.qualified]

This section gives a single pipeline for qualified references that covers intra‑assembly access (no imports required) and inter‑assembly access (via `import`).

#### G.3.1 Alias map

The alias map 𝔄 only records aliases introduced by `import` of external assemblies; intra‑assembly access does not use aliases.

#### G.3.2 Resolution rules

Given module tables 𝕄 (assembly) and {𝕄_A} for external assemblies, and alias map 𝔄:

Module‑path head:

$$
\frac{m \in \operatorname{dom}(\mathbb{M}) \quad x \in \mathcal{E}(m)}{m::x \Rightarrow \text{Binding}(m,x)}\quad[\text{QR-Resolve-Intra}]
$$

Alias head (external):

$$
\frac{a \in \operatorname{dom}(\mathbb{A}) \quad \mathbb{A}(a)=m' \quad m'::x \Rightarrow b}{a::x \Rightarrow b}\quad[\text{QR-Alias-External}]
$$

Failure conditions and diagnostics:

- Invalid head (neither module path in 𝕄/𝕄_A nor alias in 𝔄): E04‑400.
- Partial module path not found (left‑to‑right prefix failure): E04‑402.
- Item not exported from resolved module: E04‑404.
- Alias rebound attempt: E04‑403.

---

### G.4 Initialization order and eager/lazy classification [algorithms.init]

Let G = (V, E) be the directed dependency graph whose vertices are modules and edges record initialization‑relevant dependencies.

#### G.4.1 Edge classification

An edge m → n is eager if and only if any module‑scope initializer or comptime block in m may read a binding in ℰ(n) that requires initialization. Otherwise the edge is lazy.

Deterministic classifier:

```
function classify_edges(M):
    for each module m in M:
        for each reference (m → n) discovered in module-scope initializers or comptime blocks:
            mark eager(m, n)
        for each reference to n that mentions only procedures, types, behaviors, or contracts:
            if not eager(m, n): mark lazy(m, n)
    return E_eager, E_lazy
```

#### G.4.2 Ordering and safety

Cycle rejection (eager subgraph):

$$
\frac{\exists C \subseteq V.\, \text{Cycle}(C, E_{eager})}{\text{RejectCycle}(C)}\quad[\text{WF-Cycle}]
$$

Safe initialization:

$$
\frac{\forall p \in \operatorname{Pred}_{eager}(m).\, \text{Initialized}(p)}{\text{SafeInit}(m)}\quad[\text{WF-SafeInit}]
$$

Scheduler (topological): any topological order over (V, E_{eager}). On failure, emit E04‑501 for blocked successors and E04‑502 on attempted reads before initialization completes.

---

### G.5 Conformance hooks and tooling [algorithms.conformance]

Implementations must expose the following artifacts to tooling:

- The intra‑assembly module table 𝕄 and per‑module export sets ℰ(m).
- The external alias map 𝔄 and the set of resolved external assemblies {𝕄_A}.
- The eager subgraph (V, E_{eager}) and its topological order.

All emitted diagnostics must follow Annex E §E.5 payload schemas.

