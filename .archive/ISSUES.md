# Cantrip Specification Issues (Central Register)

This register implements outstanding Issues for Part II (Types).

## Key

- TS-#: Type System work items
  - Scope: Substantive, normative tasks for Part II (semantics, typing/eval rules, algorithms, proofs, global policies like variance/promotion).
  - Canonical impact: MAY require canonical spec updates beyond the scaffold.

- S-#: Structural / Scaffold issues
  - Scope: Consistency, anchors, citations, notation, or organizational fixes within the scaffold and cross‑references.
  - Canonical impact: MAY require small anchor/title syncs; MUST NOT introduce new language features.

- PV-#: Provenance & Canonical Alignment issues
  - Scope: Missing/ambiguous canonical §/anchor, conflicting clauses, or provenance gaps that prevent introducing/altering rules.
  - Policy: Highest severity; STOP adding semantics until resolved per “Canonical provenance check”.

- CN-#: Canonical Normative Change requests
  - Scope: Proposed patches to canonical grammar/rules/tables outside this scaffold (e.g., unify effect annotation, add PermissionType nonterminal).
  - Policy: Track until merged; cross‑link affected §§ and PRs.

- ED-#: Editorial / Style / Copy issues
  - Scope: Typos, grammar, formatting, figure/table captions, cross‑reference text alignment; non‑normative wording fixes.
  - Policy: Batch and resolve frequently; MUST NOT change semantics.

- DX-#: Diagnostics & Tooling issues
  - Scope: Error code taxonomy, diagnostic messages, acceptance lints, CI hooks, doc build/toolchain items.
  - Policy: Gate “Ready” flags; align codes with the Part‑level Diagnostics Index.

- EX-#: Examples & Figures issues
  - Scope: Example compendium gaps, diagrams (taxonomy, modal graphs), memory layout tables awaiting art/text.
  - Policy: Non‑blocking for outline; required before final publication.

- INT-#: Cross‑Part Integration issues
  - Scope: Linkage with permissions/effects/regions/contracts across Parts; taxonomy placement; inter‑chapter anchor consistency.
  - Policy: May depend on other Parts; keep explicit cross‑links.

- COMP-#: Compatibility & FFI issues
  - Scope: ABI/representation promises (repr(C/packed/transparent)), raw function pointer forms vs map types, external toolchain constraints.
  - Policy: Coordinate with products (repr) and pointers/map chapters; reference [FFI‑Repr‑Table].

ID format: ISSUE[<CAT>-<N>], where <CAT> ∈ {TS,S,PV,CN,ED,DX,EX,INT,COMP} and N is a monotonically increasing integer per category.
Status values: open | in_progress | resolved | deferred.


Each entry follows the required format:

ISSUE[ID]: <short-title>
Context: <where the gap arose>
Proposal: <minimal change or “unspecified”>
Impact: <why this blocks fidelity>
Conflicts: <CITE(s) and anchors>
Status: <open|in_progress|resolved|deferred>
Last-Updated: <YYYY-MM-DD>

---

### ISSUE[TS-1]: Tuple-struct constructors & examples
Context: Tuple‑struct declarations and constructors lacked explicit grammar anchors and typing rule in the scaffold.
Proposal: Add canonical anchors and minimal rules: §2.2.2.8.1 — Tuple Struct Syntax; §2.2.2.8.2 — Tuple Struct Abstract Syntax; §2.2.2.8.3 — Tuple Struct Well‑Formedness; §2.2.2.8.4 — Tuple Struct Typing; constructor examples using explicit match; effects declared via uses ε when allocation occurs.
Impact: Closes grammar and static semantics; unblocks examples in Part II, Chapter 3.
Conflicts: CITE: §2.2.2.8.1 — Tuple Struct Syntax; CITE: §2.2.2.8.2 — Tuple Struct Abstract Syntax; CITE: §2.2.2.8.3 — Tuple Struct Well‑Formedness; CITE: §2.2.2.8.4 — Tuple Struct Typing; CITE: §2.0.8.1 — Permissions; CITE: §2.0.8.2 — Effects.
Status: in_progress
Last-Updated: 2025-10-29

### ISSUE[TS-2]: Aliasing proofs for collections/views
Context: Aliasing guarantees for product types and embedded views were referenced but not proven.
Proposal: Add lemmas for field‑access safety under own/mut/imm; non‑escape of region‑allocated values; parameterize proofs by Γ;Δ and LIFO.
Impact: Satisfies Quality Gates for Proven Properties without importing Rust exclusivity.
Conflicts: CITE: §2.2.0.3 — Guarantees; CITE: §2.0.8.4 — Regions; CITE: §2.0.7 — Dynamic Semantics.
Status: open
Last-Updated: 2025-10-29

### ISSUE[TS-3]: Associated-type bounds & inference
Context: Solver hooks for associated types were not recorded in the scaffold.
Proposal: Clarify use of Trait::Item in type positions under in‑scope bounds; normalize associated items during constraint solving.
Impact: Predictable inference with nominal traits.
Conflicts: CITE: §2.6 — Traits; CITE: §2.0.6.5 — Type Inference; CITE: §2.0.5.1 — Type Grammar.
Status: open
Last-Updated: 2025-10-29

### ISSUE[TS-4]: Marker traits semantics
Context: Marker traits needed an explicit normative role and WF constraints.
Proposal: Define marker traits as zero‑procedure, zero‑representation bounds‑only traits; forbid layout‑affecting associated items.
Impact: Clarifies compile‑time role without runtime promises.
Conflicts: CITE: §2.6 — Traits.
Status: open
Last-Updated: 2025-10-29

### ISSUE[TS-5]: Example compendium integration
Context: Examples tracked out‑of‑tree; no Ready gating.
Proposal: Add Ready flags and build checks; require examples to use ! ε and explicit match.
Impact: Prevents drift; improves reviewability.
Conflicts: CITE: §2.0.11 — Specification Status; CITE: §2.0.13 — Outstanding Issues; CITE: §1.2.5 — Equivalence Law.
Status: open
Last-Updated: 2025-10-29

### ISSUE[TS-6]: Sums/Modals: exhaustiveness & verification
Context: Exhaustiveness and modal transition verification were referenced but not stated.
Proposal: Add theorem and diagnostics for exhaustive match; require transition coverage and flag unreachable/missing modal states.
Impact: Strengthens safety of pattern‑matching and modals.
Conflicts: CITE: §2.4 — Sums; CITE: §2.0.7 — Dynamic Semantics.
Status: open
Last-Updated: 2025-10-29

### ISSUE[TS-7]: Trait objects & object safety
Context: Object safety and dynamic dispatch conditions were not captured.
Proposal: Add object‑safety checklist and dyn‑like nominal object type; dispatch side‑conditions reference subtyping.
Impact: Stabilizes use of trait objects.
Conflicts: CITE: §2.6 — Traits; CITE: §2.6.3 — Object Safety; CITE: §2.0.6.1 — Subtyping.
Status: open
Last-Updated: 2025-10-29

### ISSUE[TS-8]: Associated types in generics
Context: Referencing Trait::Item in signatures lacked WF rules.
Proposal: Permit Trait::Item only under in‑scope trait bounds; record WF and inference requirements.
Impact: Removes ambiguity in generic signatures.
Conflicts: CITE: §2.0.5.1 — Type Grammar; CITE: §2.6 — Traits; CITE: §2.0.6.5 — Type Inference.
Status: open
Last-Updated: 2025-10-29

### ISSUE[TS-9]: Higher‑order effects & capture diagnostics
Context: Closure capture effects were not enforced; higher‑order subeffect polymorphism not summarized.
Proposal: Require ε_capture ⊆ ε_decl; add diagnostics; summarize ∀ε propagation for higher‑order map types.
Impact: Prevents unsound effect elision in higher‑order code.
Conflicts: CITE: §1.2.8 — Effect Polymorphism; CITE: §2.0.8.2 — Effects; CITE: §2.0.6.1 — Subtyping.
Status: open
Last-Updated: 2025-10-29
## Index

| ID | Short Title | Status | Last-Updated |
|----|-------------|--------|--------------|
| TS-1  | Tuple-struct constructors & examples          | in_progress  | 2025-10-29 |
| TS-2  | Aliasing proofs for collections/views          | open         | 2025-10-29 |
| TS-3  | Associated-type bounds & inference             | open         | 2025-10-29 |
| TS-4  | Marker traits semantics                        | open         | 2025-10-29 |
| TS-5  | Example compendium integration                 | open         | 2025-10-29 |
| TS-6  | Sums/Modals: exhaustiveness & verification     | open         | 2025-10-29 |
| TS-7  | Trait objects & object safety                  | open         | 2025-10-29 |
| TS-8  | Associated types in generics                   | open         | 2025-10-29 |
| TS-9  | Higher‑order effects & capture diagnostics     | open         | 2025-10-29 |
| TS-10 | Map notation consolidation                     | in_progress  | 2025-10-29 |
| TS-11 | Lexical nonterminals (TypeName/…/Nat/ConstExpr)| resolved     | 2025-10-29 |
| TS-12 | EffectSet grammar & lattice                    | in_progress  | 2025-10-29 |
| TS-13 | Permission wrapper EBNF                        | open         | 2025-10-29 |
| TS-14 | Region‑parameterized judgments                 | in_progress  | 2025-10-29 |
| TS-15 | Copy trait ordering                            | resolved     | 2025-10-29 |
| TS-16 | Quick Reference “?” removal                    | in_progress  | 2025-10-29 |
| TS-17 | Ready‑flag gating/checks                       | resolved     | 2025-10-29 |
| TS-18 | Variance centralization                        | resolved     | 2025-10-29 |
| TS-19 | Effect polymorphism (∀ε)                       | in_progress  | 2025-10-29 |
| TS-20 | Numeric promotion policy                       | in_progress  | 2025-10-29 |
| S-01  | Effect notation mismatch (→{ε} vs ! ε)         | in_progress  | 2025-10-29 |
| S-02  | Unique anchors for FFI/ObjSafety tables        | resolved     | 2025-10-29 |
| S-03  | Anchor name mismatch (Sub‑MapType/T‑App)       | resolved     | 2025-10-29 |
| S-04  | Trait citation title mismatch                  | resolved     | 2025-10-29 |
| S-05  | Nonexistent map/higher‑order citation          | resolved     | 2025-10-29 |
| S-06  | Ambiguous coercion/capability anchors          | resolved     | 2025-10-29 |
| S-07  | Token catalog scope (placeholder)              | in_progress  | 2025-10-29 |
| S-08  | FFI repr cross‑chapter anchor                  | resolved     | 2025-10-29 |
| S-09  | PermissionType nonterminal insertion           | open         | 2025-10-29 |

| PV-1  | Effect notation canonical sync (→{ε} vs ! ε)   | in_progress  | 2025-10-29 |
| CN-1  | Add PermissionType to canonical §2.0.5         | open         | 2025-10-29 |

---

### ISSUE[TS-10]: Map vs function type notation conflict
Context: Canonical §§2.0.5.1/2.0.6.1 show EffectAnnotation braces; scaffold standardizes map(T) → U ! ε.
Proposal: Maintain a single equivalence in §1.2.5; unify canonical grammar to ! ε or explicitly permit both until the unification patch lands.
Impact: Prevents split parsers and reviewer confusion.
Conflicts: CITE: §2.0.5.1; §2.0.6.1; §1.2.5 (scaffold law).
Status: in_progress
Last-Updated: 2025-10-29

### ISSUE[S-01]: Effect notation mismatch (→{ε} vs ! ε)
Context: Codex structural review identified mismatch.
Proposal: Keep temporary equivalence; file canonical unification PR; update examples to one form post‑merge.
Impact: Blocks final canonical EBNF; acceptable during outline phase.
Conflicts: CITE: §2.0.5.1 — Type Grammar; CITE: §2.0.6.1 — Subtyping; CITE: §1.2.5 — Equivalence Law.
Status: in_progress
Last-Updated: 2025-10-29

### ISSUE[PV-1]: Effect notation canonical sync (→{ε} vs ! ε)
Context: Canonical §2.0.5.1 and §2.0.6.1 present EffectAnnotation via braces and examples; scaffold and Foundations standardize type-level `! ε` with a declaration-level `uses ε` and an equivalence law.
Proposal: Update canonical grammar/examples to adopt `! ε` as the single type-level annotation and keep `uses ε` on declarations; add an explicit equivalence note for legacy →{ε} forms during deprecation.
Impact: Removes dual-surface ambiguity; aligns tooling, examples, and subtyping side-conditions; improves clarity for LLMs and readers.
Conflicts: CITE: §2.0.5.1 — Type Grammar; CITE: §2.0.6.1 — Subtyping; CITE: §1.2.5 — Equivalence Law.
Status: in_progress
Last-Updated: 2025-10-29

### ISSUE[CN-1]: Add PermissionType nonterminal to canonical §2.0.5
Context: The scaffold requires Permission wrappers (own/mut/imm) as first-class; canonical §2.0.5 currently lacks a PermissionType production.
Proposal: Insert `PermissionType ::= ("own" | "mut" | "imm") Type` in canonical §2.0.5.1 with links to permission operations in §2.0.8.1; update WF rules accordingly.
Impact: Closes the grammar; enables consistent referencing across Parts; formalizes permission‑indexed types.
Conflicts: CITE: §2.0.5.1 — Type Grammar; §2.0.8.1 — Permissions.
Status: open
Last-Updated: 2025-10-29

### ISSUE[S-02]: Unique anchors for FFI/ObjSafety tables
Context: FFI repr/table and object‑safety checklist lacked anchors.
Proposal: Add [FFI‑Repr‑Table] and [ObjSafety‑Checklist] anchors.
Impact: Fixes cross‑chapter link resolution.
Conflicts: CITE: §2.2.2.6; §2.6.
Status: resolved
Last-Updated: 2025-10-29

### ISSUE[S-03]: Anchor name mismatch (Sub‑MapType/T‑App)
Context: Scaffold referenced [Sub‑Map]/[T‑App]; canonical uses [Sub‑MapType], no [T‑App].
Proposal: Align scaffold to [Sub‑MapType] and cite §2.0.6.4 for application typing.
Impact: Eliminates anchor drift.
Conflicts: CITE: §2.0.6.1; §2.0.6.4.
Status: resolved
Last-Updated: 2025-10-29

### ISSUE[S-04]: Trait citation title mismatch
Context: Cited “Coherence Notes” vs canonical “Trait Patterns” + Property 6.3.
Proposal: Cite §2.6.5 Trait Patterns and Property 6.3.
Impact: Corrects citation.
Conflicts: CITE: §2.6.5; Property 6.3.
Status: resolved
Last-Updated: 2025-10-29

### ISSUE[S-05]: Nonexistent map/higher‑order citation
Context: Referenced “§2.8.5 Constraint Notes,” which doesn’t exist.
Proposal: Cite §2.8.3.2 Type Rules; §2.8.5.2 Environment Capture.
Impact: Corrects citation.
Conflicts: CITE: §2.8.3.2; §2.8.5.2.
Status: resolved
Last-Updated: 2025-10-29

### ISSUE[S-06]: Ambiguous coercion/capability anchors
Context: Multiple chapters used generic table names.
Proposal: Use [Coercions‑Ch5], [Caps‑Ch6], [Caps‑Ch7].
Impact: Unique targets for references.
Conflicts: CITE: §2.4; §2.5.
Status: resolved
Last-Updated: 2025-10-29

### ISSUE[S-07]: Token catalog scope
Context: §1.2.9 lists tokens not yet canonically defined in Part II.
Proposal: Mark as placeholder catalog with explicit ISSUE; canonicalize tokens in effects chapter.
Impact: Avoids premature normative commitment.
Conflicts: CITE: §1.2.9; §2.0.8.2.
Status: in_progress
Last-Updated: 2025-10-29

### ISSUE[S-08]: FFI repr anchor resolution
Context: Chapters 6/7/11 referenced FFI table without anchor.
Proposal: Add [FFI‑Repr‑Table] and update references.
Impact: Resolves cross‑chapter links.
Conflicts: CITE: §2.2.2.6.
Status: resolved
Last-Updated: 2025-10-29

### ISSUE[S-09]: PermissionType nonterminal insertion
Context: Scaffold records intent to add PermissionType to §2.0.5; canonical grammar not yet updated.
Proposal: Add nonterminal under §2.0.5.* with Permission operations, or keep as pending insertion until canonical patch lands.
Impact: Keeps grammar closed and auditable.
Conflicts: CITE: §2.0.5.1 — Type Grammar; CITE: §2.0.8.1 — Permissions.
Status: open
Last-Updated: 2025-10-29

### ISSUE[TS-11]: Lexical nonterminals
Context: Undefined nonterminals in EBNF.
Proposal: Add §1.1.4 Lexical Units (done).
Impact: Fixes Quality Gate.
Conflicts: CITE: §2.0.5.
Status: resolved
Last-Updated: 2025-10-29

### ISSUE[TS-12]: EffectSet grammar & lattice
Context: Missing grammar and lattice laws for effects.
Proposal: Add §1.1.6 (grammar) and §1.2.7 (laws); tie to subtyping.
Impact: Enables function subtyping and effect normalization.
Conflicts: CITE: §2.0.5.1 — Type Grammar; CITE: §2.0.8.2 — Effects; CITE: §1.2.7 — Effect Lattice Laws; CITE: §2.0.6.1 — Subtyping.
Status: in_progress
Last-Updated: 2025-10-29

### ISSUE[TS-13]: Permission wrapper EBNF
Context: own/mut/imm used widely without grammar site.
Proposal: Add PermissionType nonterminal under §2.0.5.* (pending canonical update).
Impact: Closes grammar; keeps permissions first‑class.
Conflicts: CITE: §2.0.5.1; §2.0.8.1.
Status: open
Last-Updated: 2025-10-29

### ISSUE[TS-14]: Region constraints in judgments
Context: Regions discussed but not reflected in judgments.
Proposal: Add Γ;Δ ⊢ e : τ and anchors [Region‑Escape]/[Region‑Close] (done).
Impact: Enables non‑escape/LIFO theorems.
Conflicts: CITE: §2.0.8.4; §2.0.7.
Status: in_progress
Last-Updated: 2025-10-29

### ISSUE[TS-15]: Copy trait ordering
Context: Copy described before traits.
Proposal: Move trait to §2.6; keep structural note in §2.0.9.1.
Impact: Removes cyclic dependency.
Conflicts: CITE: §2.0.9.1; §2.6.
Status: resolved
Last-Updated: 2025-10-29

### ISSUE[TS-16]: Quick Reference “?” operator
Context: Quick Reference example used `?` which is banned.
Proposal: Replace with explicit match; add note clarifying that “?” appears only as EBNF optionality; cite sums/exhaustiveness and effects for explicit error handling.
Impact: Aligns with guardrails.
Conflicts: CITE: §2.4 — Sums; CITE: §2.0.8.2 — Effects.
Status: in_progress
Last-Updated: 2025-10-29

### ISSUE[TS-17]: Ready‑flag gating
Context: Chapters lacked gating criteria.
Proposal: Add acceptance checklist and lints; flip Ready only when passing.
Impact: Controls merge quality.
Conflicts: §2.0.11.
Status: resolved
Last-Updated: 2025-10-29

### ISSUE[TS-18]: Variance centralization
Context: Variance scattered across chapters.
Proposal: Centralize in §1.2.6; chapters cite it (done).
Impact: Prevents drift.
Conflicts: §2.0.6.1.
Status: resolved
Last-Updated: 2025-10-29

### ISSUE[TS-19]: Effect polymorphism (∀ε)
Context: Higher‑order compositionality and least‑privilege effects.
Proposal: Add §1.2.8 rules (done), integrate with §1.2.6/§1.2.7; defer surface syntax.
Impact: Precision, reuse, least‑privilege.
Conflicts: §2.0.8.2; §2.0.6.1.
Status: in_progress
Last-Updated: 2025-10-29

### ISSUE[TS-20]: Numeric promotion policy
Context: Avoid family‑specific divergence.
Proposal: Global policy in §1.2.10 (no implicit promotions; explicit casts); update chapters to cite (§2.2.3 done).
Impact: Clarity; soundness.
Conflicts: §2.1; §1.2.10.
Status: in_progress
Last-Updated: 2025-10-29

---

Maintenance:
- Update this register when an ISSUE is opened/closed or its proposal changes.
- Chapters MUST reference this file (Spec/ISSUES.md) as the source of truth for Issues.
