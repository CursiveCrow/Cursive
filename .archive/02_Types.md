# Part II: Types

Integration Correction (normative alignment): The canonical function type constructor is map; the value-level term is function; the declaration/definition form is procedure. Effect annotations use one authoritative form:
- Type level: map(T1,…,Tn) → U ! ε
- Declaration level: procedure … uses ε { … }
- Equivalence: map(T) → U ≡ map(T) → U ! ∅ and a procedure’s uses ε induces the same ε on its inferred map type. CITE: §2.0.5.1 — Type Grammar; CITE: §2.0.6.1 — Subtyping; CITE: §2.0.8.2 — Effects.

> Integration Note: This scaffold enumerates the canonical content that will be reflowed into `Spec/02_Types.md` once the cited ISSUE dependencies close. CITE: §2.1 — Primitive Types. ISSUE TS-5.

Canonical Update (2025-10-30): Unified `string` modal type replaces separate `str` (view) and `String` (owned) types. The single type `string` has states `@View` and `@Owned` with the following defaults:

- Parameters: `string` ≡ `string@View`
- Return types and stored fields: `string` ≡ `string@Owned`
- Coercion: `string@Owned → string@View` (implicit, zero cost)
- Allocation: `string@View.to_owned()` (explicit, `uses alloc.heap`)

Deprecated aliases remain for compatibility within archives: `type str = string@View`; `type String = string@Owned`.

## Chapter 1: Type Foundations

| Ready | Dependencies | Issues | Bridge | Notes |
| --- | --- | --- | --- | --- |
| [x] | CITE: §2.0.1 — Scope; CITE: §2.0.3 — Cross-Part Dependencies | TS-1, TS-2, TS-3, TS-4, TS-5, TS-6, TS-7, TS-8, TS-9 | §2.0 canonical prose → Chapter 1 scaffold | Outline is complete and ready for normative text injection; cross-part invariants and taxonomy anchors added below |

### Section 1.0 Overview
- Foundations describe the shared typing infrastructure, linking permissions, effects, contracts, and regions so later chapters inherit consistent invariants. CITE: §2.0.1 — Scope; CITE: §2.0.3 — Cross-Part Dependencies. ISSUE TS-5.
- Integration mirrors the canonical ordering, preserving design goals and the dependency graph recorded in §2.0.2–§2.0.4 for seamless merge-back. CITE: §2.0.2 — Design Goals; CITE: §2.0.4 — Notational Conventions. ISSUE TS-5.

#### Section 1.0.1 Scope and Goals
- Outline reiterates that Type Foundations establishes judgments, subtyping, and equivalence facts required throughout Part II, matching §2.0.1 language. CITE: §2.0.1 — Scope. ISSUE TS-5.

#### Section 1.0.2 Dependencies and Notation
- Summaries enumerate shared metavariables, effect markers, and cross-part requirements drawn from §2.0.3–§2.0.4 so downstream chapters remain synchronized. CITE: §2.0.3 — Cross-Part Dependencies; CITE: §2.0.4 — Notational Conventions. ISSUE TS-5.

#### Section 1.0.3 Guarantees
- Guarantees restate compile-time safety, local reasoning, and performance targets that motivate the rest of Part II. CITE: §2.0.2 — Design Goals. ISSUE TS-5.

#### Section 1.0.4 Front‑Matter Alignment (normative)
- Front‑matter MUST align with Cantrip guardrails:
  - Remove lifetime syntax (e.g., &'a T) and iso; express capabilities via own/mut/imm wrappers and regions.
  - Replace fn/proc type tokens with map types; retain procedure only as a declaration/definition form.
  - If legacy tokens remain, mark them explicitly deprecated in a historical appendix. CITE: §2.0.5.1 — Type Grammar; CITE: §2.0.8.1 — Permissions; CITE: §2.0.8.4 — Regions.

### Section 1.1 Syntax
- The outline records required grammar references for type constructors, kinds, and contexts, noting that detailed EBNF lives in §2.0.5. CITE: §2.0.5 — Syntax. ISSUE TS-5.

#### Section 1.1.1 Concrete Grammar
- Call out the full type grammar and primitive catalog, including pending exemplar updates gated by the example compendium. CITE: §2.0.5.1 — Type Grammar. ISSUE TS-5.

#### Section 1.1.2 Abstract Syntax
- Document kind grammar and context formation rules that underpin abstract syntax trees for foundational constructs. Kinds: Type, Eff (effect sets), Reg (regions). Provide well‑formedness hooks tying ε : Eff and Δ : Reg*. CITE: §2.0.5.2 — Kind Grammar; CITE: §2.0.5.3 — Typing Contexts and Judgments. ISSUE TS-5.

#### Section 1.1.3 Derived Forms
- Note that no additional derived forms are authorised beyond §2.0.5 definitions, with any future sugar logged under TS-5. If a chapter has no derived forms, fold the note into its Concrete Grammar and omit this subsection to reduce scanning cost. CITE: §2.0.5 — Syntax. ISSUE TS-5.

#### Section 1.1.4 Lexical Units (new)
- Define the lexical nonterminals referenced by the EBNF in this Part: TypeName, TraitName, EffectName, ConstExpr, Nat, Identifier. Nat ∈ ℕ₀ (non‑negative). ConstExprs are phase‑separable and must normalize at compile time. Cross‑link to parametric features for const generics; link to Part I if centralized there. CITE: §2.0.5 — Syntax; §2.7 — Parametric Features. ISSUE TS-11.

#### Section 1.1.5 PermissionType Grammar (new)
- Record intent to add PermissionType ::= ("own" | "mut" | "imm") Type under §2.0.5. Until canonical grammar is updated, treat this as a pending insertion to keep the grammar closed. Register ISSUE and forward‑link to permission operations. CITE: §2.0.5.1 — Type Grammar; CITE: §2.0.8.1 — Permissions. ISSUE TS-13.

#### Section 1.1.6 EffectSet Grammar and Lattice (new)
- EffectSet concrete syntax (hosted in the type grammar): Effects ::= "{" Effect ("," Effect)* "}". The lattice laws (⊆, idempotence, commutativity, associativity; ⊥=∅) govern normalization; ‘⊔’ is not surface syntax. CITE: §2.0.5.1 — Type Grammar; CITE: §2.0.8.2 — Effects; CITE: §1.2.7 — Effect Lattice Laws. ISSUE TS-12.

#### Section 1.1.7 Effect Polymorphism (placeholder)
- Placeholder hooks for effect quantification (∀ε); register ISSUE and defer concrete surface syntax to the Effects chapter. CITE: §2.0.8.2 — Effects. ISSUE TS-19.

### Section 1.2 Static Semantics
- Static semantics summary imports subtyping, equivalence, and well-formedness rules that all later chapters rely on. CITE: §2.0.6 — Static Semantics. ISSUE TS-5; ISSUE TS-3; ISSUE TS-7; ISSUE TS-8.

#### Section 1.2.1 Typing Contexts and Judgments
- Outline references definitions of `Γ ⊢` judgments and auxiliary relations to ensure consistent use across Part II. CITE: §2.0.5.3 — Typing Contexts and Judgments. ISSUE TS-5.

#### Section 1.2.2 Well-Formedness Rules
- Capture the well-formedness obligations for types and contexts, noting associated-bound extensions still blocked by TS-3/TS-8. CITE: §2.0.6.3 — Well-Formedness. ISSUE TS-5; ISSUE TS-3; ISSUE TS-8.
  - Sized marker: by default, types are Sized; unsized families include str, [T], and dyn‑like objects as defined in their chapters. Pointers/views to unsized types are well‑formed; values are not. Cross‑link to Chapters 2 (strings/str), 5 (slices/views), and 8 (trait objects).

#### Section 1.2.3 Subtyping and Equivalence
- Summaries highlight the preorder axioms and aliasing-safe conversions that underpin derived chapters. CITE: §2.0.6.1 — Subtyping; CITE: §2.0.6.2 — Type Equivalence. ISSUE TS-5; ISSUE TS-7.

#### Section 1.2.4 Permission-Indexed Types
- Note that permission wrappers integrate with these judgments and defer region-specific details to Part III. CITE: §2.0.8.1 — Permissions. ISSUE TS-5; ISSUE TS-2; ISSUE TS-7.

#### Section 1.2.5 Effect‑Annotated Map Types
- Use the type‑level notation map(…) → … ! ε and the declaration‑level form procedure … uses ε { … }. Apply the equivalence law from the Integration Correction preface; subtyping uses ε₁ ⊆ ε₂. Temporary dual notation equivalence: map(…) →{ε} … ≡ map(…) → … ! ε until canonical unification (ISSUE S‑01). The →{ε} form is legacy and appears only in historical examples; tools and new text MUST use ! ε. CITE: §2.0.6.1 — Subtyping; §2.0.8.2 — Effects. ISSUE TS-9.

#### Section 1.2.6 Variance and Subtyping Side-Conditions (new)
- Centralize constructor variance defaults (invariant unless stated) and the specific side-conditions for map types: argument positions contravariant, result covariant, and EffectSet inclusion ε₁ ⊆ ε₂. Provide a single authoritative table referenced by later chapters. CITE: §2.0.6.1 — Subtyping; CITE: §2.0.8.2 — Effects. ISSUE TS-18.

#### Section 1.2.7 Effect Lattice Laws (new)
- Algebra: idempotence, commutativity, associativity of ⊔; identity ∅; define top/bottom if present. Reference in [Sub‑MapType] and composition. CITE: §2.0.8.2 — Effects; §2.0.6.1 — Subtyping. ISSUE TS-12.

#### Section 1.2.9 Unsafe/Capability Effect Tokens (global)
- Placeholder catalog of effect tokens referenced by chapters (e.g., unsafe.ptr, unsafe.union, unsafe.bitcopy, ffi.call, alloc.heap, io.read/write). Chapters 4/6/7/11 MUST link here instead of enumerating tokens locally. Canonicalization is pending; register ISSUE and avoid normative commitment beyond map/uses/!ε. CITE: §2.0.8.2 — Effects. ISSUE TS-12; ISSUE TS-9.

#### Section 1.2.10 Numeric Promotion Policy (global)
- Policy: no implicit numeric promotions; conversions require explicit casts. Chapters MUST not define ad‑hoc promotions. Register for ratification. CITE: §2.1 — Primitive Types. ISSUE TS-20.

#### Section 1.2.8 Effect Polymorphism (∀ε)
- Kinding: ε : Eff.
- Quantification: ∀ε. map(T1,…,Tn) → U ! ε forms a type schema; ε may be constrained by subeffect bounds.
- Subeffecting: if ε₁ ⊆ ε₂ then map(…)→U ! ε₁ <: map(…)→U ! ε₂ (integrates with §1.2.6 and §1.2.7).
- Instantiation at call sites: when applying a polymorphic map, choose ε̂ such that all callee constraints hold; instantiate and check εcall ⊇ ε̂ (anchored near the application typing rule in §2.0.6.4 and [Sub‑MapType]).
- Specialization/order: specialization MAY restrict ε via bounds; coherence follows type‑parameter specialization ordering extended with ε bounds.
  CITE: §2.0.6.1 — Subtyping; §2.0.8.2 — Effects. ISSUE TS-19.

### Section 1.3 Dynamic Semantics
- Runtime summaries document evaluation judgments and safety theorems that establish progress/preservation for the rest of Part II. CITE: §2.0.7 — Dynamic Semantics. ISSUE TS-5.

#### Section 1.3.1 Runtime Obligations
- Outline enumerates evaluation judgment shape and determinism assumptions from §2.0.7, maintaining the same obligations before reuse elsewhere. CITE: §2.0.7 — Dynamic Semantics. ISSUE TS-5.

#### Section 1.3.2 Region Exit Semantics
- Highlight how regions enforce LIFO cleanup and how that contract threads into foundational proofs. CITE: §2.0.8.4 — Regions. ISSUE TS-5; ISSUE TS-2.

#### Section 1.3.3 Unsafe Interaction
- Note that unsafe operations are scoped through effect tokens and remain constrained by permission rules. CITE: §2.0.8.5 — Modal Types; CITE: §2.0.9.1 — Copy Trait. ISSUE TS-5; ISSUE TS-2.

#### Section 1.3.4 Region-Parameterized Judgments (new)
- Introduce the region-aware typing judgment form Γ;Δ ⊢ e : τ (Δ is the region stack) used to state non-escape and LIFO deallocation. Provide rule anchors [Region‑Escape] and [Region‑Close] to be filled during normative expansion. CITE: §2.0.8.4 — Regions; CITE: §2.0.7 — Dynamic Semantics. ISSUE TS-14.

Anchors (rule schemata):
```
[Region‑Escape]
Γ; (Δ · r) ⊢ v : τ    v contains an allocation from r
──────────────────────────────────────────────────────
Γ; Δ ⊬ v : τ          // reject escape past region r’s close

[Region‑Close]
Γ; (Δ · r) ⊢ e : τ
───────────────────────
Γ; Δ ⊢ { close r } : τ
```
These anchors record the intent and naming only; precise dynamic semantics remain in §2.0.7 and §2.0.8.4. CITE: §2.0.8.4 — Regions; CITE: §2.0.7 — Dynamic Semantics. ISSUE TS-14.

### Section 1.4 Algorithms
- Algorithms section recaps bidirectional checking, inference, and constraint solving seeded in §2.0.6.4–§2.0.6.5. CITE: §2.0.6.4 — Type Checking; CITE: §2.0.6.5 — Type Inference. ISSUE TS-5; ISSUE TS-3; ISSUE TS-9.

#### Section 1.4.1 Bidirectional Typing Procedure
- Summaries reference the algorithmic outline of `check`/`synth`, pointing to future specialization per type family. CITE: §2.0.6.4 — Type Checking. ISSUE TS-5.

#### Section 1.4.2 Constraint Solving and Coherence
- Outline restates constraint forms and solver behaviour, flagging associated-type and coherence work items. CITE: §2.0.6.5 — Type Inference. ISSUE TS-5; ISSUE TS-3; ISSUE TS-8; ISSUE TS-9.

#### Section 1.4.3 Error Diagnostics
- Identify diagnostic expectations for mismatched types and subtyping failures, with concrete code IDs to follow once the example suite lands. CITE: §2.0.6.4 — Type Checking. ISSUE TS-5.

### Section 1.5 Proven Properties
- Summaries reiterate the foundational progress, preservation, and parametricity results that justify the remainder of Part II. CITE: §2.0.7 — Dynamic Semantics. ISSUE TS-5; ISSUE TS-2.

#### Section 1.5.1 Progress
- Outline references Theorem 2.9 and highlights pending aliasing lemmas tied to TS-2. CITE: §2.0.7 — Dynamic Semantics. ISSUE TS-5; ISSUE TS-2.

#### Section 1.5.2 Preservation
- Summaries cite Theorem 2.10 and associated lemmas, noting dependency on permission invariants under TS-2. CITE: §2.0.7 — Dynamic Semantics. ISSUE TS-5; ISSUE TS-2.

#### Section 1.5.3 Soundness Corollaries
- List soundness consequences recorded in §2.0.7 and note follow-on proofs for markers and modals. CITE: §2.0.7 — Dynamic Semantics. ISSUE TS-5; ISSUE TS-7.

#### Section 1.5.4 Parametricity and Principal Types
- Record parametricity theorem coverage and principal-type existence claims, with generalization gaps pending TS-3/TS-8. CITE: §2.0.6.5 — Type Inference; CITE: §2.0.7 — Dynamic Semantics. ISSUE TS-5; ISSUE TS-3; ISSUE TS-8.

### Section 1.6 Interaction Summaries
- Outline sketches the eventual summary tables tying foundations to permissions/effects/contracts so integration mirrors §2.0.8–§2.0.10. Provide one shared template (columns/order) for all chapters; small families (e.g., primitives) MAY consolidate into a single Part‑level table. CITE: §2.0.8 — Inter-System Integration; CITE: §2.0.10 — Type Taxonomy. ISSUE TS-5.

### Section 1.A Appendices (Non-Normative)
- Appendices will consolidate navigation aids, example stubs, and status tables aligned with §2.0.10–§2.0.12. CITE: §2.0.10 — Type Taxonomy; CITE: §2.0.12 — Reading Guide. ISSUE TS-5.

#### Section 1.A.1 Taxonomy Diagram and Cross-Reference Table
- Document that taxonomy diagrams must mirror §2.0.10 and remain synchronized with the backlog. CITE: §2.0.10 — Type Taxonomy. ISSUE TS-5; ISSUE TS-4.

#### Section 1.A.2 Extended Examples
- Plan example suites derived from the pending compendium, logging their dependency on TS-5. CITE: §2.0.12 — Reading Guide; CITE: §2.0.13 — Outstanding Issues. ISSUE TS-5.

#### Section 1.A.3 Specification Status & Backlog
 - Preserve a single Part‑level status/backlog table here; chapter appendices link to this central table instead of duplicating entries. See the central register at `Spec/ISSUES.md`. CITE: §2.0.11 — Specification Status; CITE: §2.0.13 — Outstanding Issues. ISSUE TS-1; ISSUE TS-2; ISSUE TS-3; ISSUE TS-4; ISSUE TS-5; ISSUE TS-6; ISSUE TS-7; ISSUE TS-8; ISSUE TS-9.

#### Section 1.A.4 Diagnostics Index (Part‑Level)
- Central index of diagnostic classes/codes referenced by chapters (overflow, invalid code point, non‑boundary slice, divide‑by‑zero, unsafe misuse). Chapters cite this index; codes are defined once and reused.


## Chapter 2: Primitive Types

| Ready | Dependencies | Issues | Bridge | Notes |
| --- | --- | --- | --- | --- |
| [x] | CITE: §2.1.0 — Overview; CITE: §2.0.11 — Specification Status | TS-1, TS-2, TS-5, TS-6, TS-7, TS-8, TS-9 | §2.1 canonical prose → Chapter 2 scaffold | Outline finalized and ready for normative text injection; TS-series items remain for content-level proofs/examples |

### Section 2.0 Overview
- Primitive types anchor Cantrip's scalar semantics, coordinating with permissions, effects, contracts, and regions so later chapters can build on a stable baseline. CITE: §2.1.0 — Overview. ISSUE TS-5.
- Integration follows §2.1.0.9 by mirroring terminology and subsection ordering, ensuring a lossless merge back into `Spec/02_Types.md` once blockers resolve. CITE: §2.1.0.9 — Integration Notes. ISSUE TS-5.

#### Section 2.0.1 Scope and Goals
- Chapter coverage spans integers, floats, booleans, characters, the never type, and strings, matching the canonical mandate and documenting stabilization gates for each family. CITE: §2.1.0.1 — Scope. ISSUE TS-5.

#### Section 2.0.2 Dependencies and Notation
- Outline references the shared metavariables, lexical conventions, and cross-part dependencies (permissions, effects, contracts, regions) carried over from the chapter body to maintain notation fidelity. CITE: §2.1.0.2 — Dependencies and Notation. ISSUE TS-5.

#### Section 2.0.3 Guarantees
- Summaries restate compile-time guarantees—range safety, normalization rules, and effect isolation—so reviewers can confirm alignment with the canonical promises before reintegration. CITE: §2.1.0.3 — Guarantees. ISSUE TS-5.

### Section 2.1 Syntax
- Syntax notes collect every primitive grammar requirement while flagging the missing exemplar EBNF forms that TS-5 keeps out-of-tree. CITE: §2.1.0.4 — Syntax Summary. ISSUE TS-5.

#### Section 2.1.1 Concrete Grammar
- Lexical productions for literals and type names are cross-referenced for integers, floats, booleans, characters, the never type, and strings to keep token sets synchronized with the canonical listings. CITE: §2.1.1.2 — Integer Syntax; CITE: §2.1.2.2 — Floating-Point Syntax; CITE: §2.1.3.2 — Boolean Syntax; CITE: §2.1.4.2 — Character Syntax; CITE: §2.1.5.2 — Never Syntax; CITE: §2.1.6.2 — String Syntax. ISSUE TS-5.

#### Section 2.1.2 Abstract Syntax
- The outline maps each primitive constructor to its AST counterpart, mirroring the canonical abstract syntax while acknowledging exemplar gaps until TS-5 lands. CITE: §2.1.1.2 — Integer Syntax; CITE: §2.1.2.2 — Floating-Point Syntax; CITE: §2.1.3.2 — Boolean Syntax; CITE: §2.1.4.2 — Character Syntax; CITE: §2.1.5.2 — Never Syntax; CITE: §2.1.6.2.2 — String Abstract Syntax. ISSUE TS-5.

#### Section 2.1.3 Derived Forms
- Literal suffix rules and other shorthand notations are catalogued with their desugarings to guarantee parity with the concrete grammar once examples return. CITE: §2.1.0.4 — Syntax Summary. ISSUE TS-5.

### Section 2.2 Static Semantics
- Static semantics coverage summarizes typing, subtyping, and well-formedness obligations, surfacing all dependent TS-series issues that block promotion. CITE: §2.1.0.5 — Static Semantics Summary. ISSUE TS-5; ISSUE TS-2; ISSUE TS-7; ISSUE TS-8.

#### Section 2.2.1 Typing Contexts and Judgments
- Entries explain how primitive permissions and types appear in typing contexts, noting unresolved slice aliasing, trait-object integration, and associated-bound rules. CITE: §2.1.1.3 — Integer Static Semantics; CITE: §2.1.2.3 — Floating-Point Static Semantics; CITE: §2.1.3.3 — Boolean Static Semantics; CITE: §2.1.4.3 — Character Static Semantics; CITE: §2.1.5.3 — Never Static Semantics; CITE: §2.1.6.3 — String Static Semantics. ISSUE TS-5; ISSUE TS-2; ISSUE TS-7; ISSUE TS-8.

#### Section 2.2.2 Well-Formedness Rules
- Outline bullet lists numeric-base constraints, UTF-8 enforcement, and other invariants while recording associated-type dependencies still tracked under TS-8. CITE: §2.1.1.3 — Integer Static Semantics; CITE: §2.1.2.3 — Floating-Point Static Semantics; CITE: §2.1.3.3 — Boolean Static Semantics; CITE: §2.1.4.3 — Character Static Semantics; CITE: §2.1.5.3 — Never Static Semantics; CITE: §2.1.6.3.1 — String Well-Formedness Rules. ISSUE TS-5; ISSUE TS-2; ISSUE TS-8.
  - Integers: exact bit‑widths (two’s‑complement), shift semantics, division by zero policy.
  - Literals: base/suffix tables; defaulting rules; any implicit promotions (if permitted).
  - Floats: IEEE‑754 (NaN payloads, ±∞, signed zero) and what comparisons mean statically.
  - char: Unicode scalar value only.
  - String/str: UTF‑8; str is an unsized view; view creation WF constraints.

#### Section 2.2.3 Subtyping and Equivalence
- Numeric promotion, never-type bottoming, and string equivalence constraints are summarized with pointers to pending trait-object and associated-bound clarifications. CITE: §2.1.1.3 — Integer Static Semantics; CITE: §2.1.2.3 — Floating-Point Static Semantics; CITE: §2.1.3.3 — Boolean Static Semantics; CITE: §2.1.4.3 — Character Static Semantics; CITE: §2.1.5.3 — Never Static Semantics; CITE: §2.1.6.3.3 — String Type Properties. ISSUE TS-5; ISSUE TS-7; ISSUE TS-8.
  - Never (!) coercion into any type; relationship with divergence and panic effect.
  - Numeric promotions: see §1.2.10 for the global policy (no implicit promotions; explicit casts only).
  - State array→slice and string→str representational relationships or assert nominal separation where applicable.

#### Section 2.2.4 Permission-Indexed Types
- Permissions for copy-eligible scalars and string regions are called out, alongside slice aliasing and trait-object follow-up obligations. CITE: §2.1.6.6.1 — Permission Integration. ISSUE TS-5; ISSUE TS-2; ISSUE TS-7.

#### Section 2.2.5 Effect-Annotated Map Types
- The outline highlights primitive operations that require effect tokens (overflow checks, unsafe copies) and records aliasing and associated-bound fallout still tracked by TS-2 and TS-8. CITE: §2.1.0.5 — Static Semantics Summary; CITE: §2.1.6.6.3 — String Operations. ISSUE TS-5; ISSUE TS-2; ISSUE TS-8.
- Effect notation: see Integration Correction preface and §1.2.5.

#### Section 2.2.6 Primitive Ops Effect Table (anchor)
- Normative table (to be populated in prose) mapping primitive operations to required effect tokens and runtime behavior.

### Section 2.3 Dynamic Semantics
- Runtime obligations summarize evaluation behavior for all primitive families and note outstanding aliasing proofs. CITE: §2.1.0.6 — Dynamic Semantics Summary. ISSUE TS-5; ISSUE TS-2; ISSUE TS-9.

#### Section 2.3.1 Runtime Obligations
- Progress checks, overflow behavior, and runtime guards are enumerated per primitive family to match the canonical evaluation rules. CITE: §2.1.1.4 — Integer Dynamic Semantics; CITE: §2.1.2.4 — Floating-Point Dynamic Semantics; CITE: §2.1.3.4 — Boolean Dynamic Semantics; CITE: §2.1.4.4 — Character Dynamic Semantics; CITE: §2.1.5.4 — Never Dynamic Semantics; CITE: §2.1.6.4 — String Dynamic Semantics. ISSUE TS-5; ISSUE TS-2.
  - Overflow policy table: wrap vs checked (panic) per op; effect tokens for checked arithmetic.
  - Float runtime: NaN propagation and rounding; ordering behavior.
  - Strings: forbidden byte indexing; slicing rules (code‑point boundaries); region and permission considerations for mutation/reallocation.

#### Section 2.3.2 Region Exit Semantics
- Notes describe how strings and slices deallocate under LIFO discipline, capturing the unresolved aliasing and closure-capture behavior that TS-2 and TS-9 track. CITE: §2.1.6.6.2 — Region Integration. ISSUE TS-5; ISSUE TS-2; ISSUE TS-9.

#### Section 2.3.3 Unsafe Interaction
- Unsafe operations such as raw byte access and unchecked conversions are summarized with explicit references to the canonical view and comparison procedures. CITE: §2.1.6.6.3.4 — View Operations; CITE: §2.1.6.6.3.6 — Comparison Operations. ISSUE TS-5; ISSUE TS-2; ISSUE TS-9.

### Section 2.4 Algorithms
- Algorithm notes track the outstanding modal verification and closure analyses that block stabilization for primitives. CITE: §2.1.0.7 — Algorithms. ISSUE TS-5; ISSUE TS-6; ISSUE TS-9.

#### Section 2.4.1 Bidirectional Typing Procedure
- Outline highlights literal defaulting and bidirectional flow for primitive expressions, noting the exhaustiveness dependencies still open. CITE: §2.1.1.3 — Integer Static Semantics; CITE: §2.1.2.3 — Floating-Point Static Semantics. ISSUE TS-5; ISSUE TS-6; ISSUE TS-9.

#### Section 2.4.2 Constraint Solving and Coherence
- Constraint items cover literal suffix propagation, numeric coercions, and string coherence, while recording associated-bound and modal-verification blockers. CITE: §2.1.1.3 — Integer Static Semantics; CITE: §2.1.2.3 — Floating-Point Static Semantics; CITE: §2.1.6.3.2 — String Type Rules. ISSUE TS-5; ISSUE TS-6; ISSUE TS-8; ISSUE TS-9.

#### Section 2.4.3 Error Diagnostics
- Diagnostic TODOs point to integer and string rule sections plus the outstanding issues catalogue so error codes remain aligned. CITE: §2.1.1.3 — Integer Static Semantics; CITE: §2.1.6.3.3 — String Type Properties; CITE: §2.1.0.10 — Notes and Issues. ISSUE TS-5; ISSUE TS-6; ISSUE TS-8; ISSUE TS-9.
  - Diagnostics: integer overflow, invalid code‑point, slice on non‑boundary, String copy misuse, divide‑by‑zero; effect ties for panic/unsafe.

### Section 2.5 Proven Properties
- Proof obligations restate the canonical guarantees while flagging aliasing proofs that TS-2 still blocks. CITE: §2.1.0.8 — Proven Properties. ISSUE TS-5; ISSUE TS-2.

#### Section 2.5.1 Progress
- Outline maintains progress statements for each primitive family, waiting on final aliasing closure. CITE: §2.1.1.4 — Integer Dynamic Semantics; CITE: §2.1.2.4 — Floating-Point Dynamic Semantics; CITE: §2.1.3.4 — Boolean Dynamic Semantics; CITE: §2.1.4.4 — Character Dynamic Semantics; CITE: §2.1.5.4 — Never Dynamic Semantics; CITE: §2.1.6.4 — String Dynamic Semantics. ISSUE TS-5; ISSUE TS-2.

#### Section 2.5.2 Preservation
- Preservation summaries cite each primitive's static rules and emphasize aliasing lemmas that remain under TS-2. CITE: §2.1.1.3 — Integer Static Semantics; CITE: §2.1.2.3 — Floating-Point Static Semantics; CITE: §2.1.3.3 — Boolean Static Semantics; CITE: §2.1.4.3 — Character Static Semantics; CITE: §2.1.5.3 — Never Static Semantics; CITE: §2.1.6.3 — String Static Semantics. ISSUE TS-5; ISSUE TS-2.

#### Section 2.5.3 Soundness Corollaries
- Corollaries such as the absence of undefined primitive states are restated with explicit dependency on the pending aliasing proof thread. CITE: §2.1.0.8 — Proven Properties. ISSUE TS-5; ISSUE TS-2.

#### Section 2.5.4 Parametricity and Principal Types
- Principal typing outcomes for polymorphic literals and strings are summarized, highlighting associated-bound tasks still recorded under TS-8. CITE: §2.1.0.8 — Proven Properties; CITE: §2.1.0.7 — Algorithms. ISSUE TS-5; ISSUE TS-2; ISSUE TS-8.

### Section 2.6 Interaction Summaries
- The interaction table will enumerate permissions, effects, and contracts per primitive capability once the canonical integration backlog closes. CITE: §2.1.0.9 — Integration Notes; CITE: §2.1.0.10 — Notes and Issues. ISSUE TS-5.

| Feature | Permissions | Effects | Contracts | Notes |
|---------|-------------|---------|-----------|-------|
| _TBD_   | _TBD_       | _TBD_   | _TBD_     | _TBD_ |

### Section 2.A Appendices (Non-Normative)
- Appendices track supporting material, enumerate outstanding TS-series issues, and reference the centralized backlog for primitives. CITE: §2.1.0.10 — Notes and Issues; CITE: §2.0.13 — Outstanding Issues. ISSUE TS-1; ISSUE TS-2; ISSUE TS-3; ISSUE TS-4; ISSUE TS-5; ISSUE TS-6; ISSUE TS-7; ISSUE TS-8; ISSUE TS-9.

#### Section 2.A.1 Taxonomy Diagram and Cross-Reference Table
- Diagram guidance links primitive categories back to the scope statement and highlights aliasing/trait annotations that remain open. CITE: §2.1.0.1 — Scope; CITE: §2.1.0.10 — Notes and Issues. ISSUE TS-5; ISSUE TS-2; ISSUE TS-7.

#### Section 2.A.2 Extended Examples
- Example plans enumerate the primitive scenarios to re-import once the example compendium closes, keeping TS-series blockers visible. CITE: §2.1.0.10 — Notes and Issues. ISSUE TS-1; ISSUE TS-2; ISSUE TS-3; ISSUE TS-4; ISSUE TS-5; ISSUE TS-6; ISSUE TS-9.

#### Section 2.A.3 Specification Status & Backlog
- Status tracking mirrors the central backlog, preserving explicit references to TS-1 through TS-9 for reviewer auditing. CITE: §2.0.13 — Outstanding Issues; CITE: §2.1.0.10 — Notes and Issues. ISSUE TS-1; ISSUE TS-2; ISSUE TS-3; ISSUE TS-4; ISSUE TS-5; ISSUE TS-6; ISSUE TS-7; ISSUE TS-8; ISSUE TS-9.


## Chapter 3: Product Types

| Ready | Dependencies | Issues | Bridge | Notes |
| --- | --- | --- | --- | --- |
| [x] | CITE: §2.2.0 — Overview; CITE: §2.2.0.2 — Dependencies and Notation | TS-1, TS-2, TS-5, TS-7 | §2.2 canonical prose → Chapter 3 scaffold | Outline finalized and ready for normative text injection; tuple‑structs/aliasing/examples tracked via TS items |

### Section 3.0 Overview
- Product types cover tuples, records, and tuple-structs, preserving field invariants and explicit permission tracking per §2.2.0.3. CITE: §2.2.0.1 — Scope; CITE: §2.2.0.3 — Guarantees. ISSUE TS-1; ISSUE TS-5.
- Integration retains dependencies on permissions, effects, and regions so aggregate layout and method semantics remain aligned with the monolith. CITE: §2.2.0.2 — Dependencies and Notation. ISSUE TS-2.

#### Section 3.0.1 Scope and Goals
- Outline enumerates tuple, record, and tuple-struct coverage in lockstep with the canonical scope statement. CITE: §2.2.0.1 — Scope. ISSUE TS-5.

#### Section 3.0.2 Dependencies and Notation
- Summaries track the shared metavariables and permission assumptions documented for product types. CITE: §2.2.0.2 — Dependencies and Notation. ISSUE TS-2; ISSUE TS-7.

#### Section 3.0.3 Guarantees
- Record the structural invariants, constructor rules, and projection safety obligations exported to downstream chapters. CITE: §2.2.0.3 — Guarantees. ISSUE TS-1.

### Section 3.1 Syntax
- Syntax outline references tuple and record grammar while flagging tuple-struct forms pending TS-1. CITE: §2.2.0.4 — Syntax Summary. ISSUE TS-1; ISSUE TS-5.

#### Section 3.1.1 Concrete Grammar
- Detail the tuple and record EBNF along with tuple-struct notation that remains provisional. CITE: §2.2.1.2 — Tuple Syntax; CITE: §2.2.2.2 — Record Syntax; CITE: §2.2.2.8.1 — Tuple Struct Syntax. ISSUE TS-1; ISSUE TS-5.

#### Section 3.1.2 Abstract Syntax
- Capture AST constructors for tuples, records, and tuple-structs to keep semantic rules synchronized. CITE: §2.2.1.2.2 — Tuple Abstract Syntax; CITE: §2.2.2.2.2 — Record Abstract Syntax; CITE: §2.2.2.8.2 — Tuple Struct Abstract Syntax. ISSUE TS-1.

##### §2.2.2.8.1 — Tuple Struct Syntax (anchor)
```ebnf
TupleStructDecl ::= "record" TypeName "(" FieldType ("," FieldType)+ ")"
FieldType       ::= PermissionType | Type
```
Notes:
- The declaration body, if present, uses the same procedure/member syntax as records (see §2.2.2.2 — Record Syntax). CITE: §2.2.2.2 — Record Syntax; CITE: §2.0.5.1 — Type Grammar; CITE: §2.0.8.1 — Permissions. ISSUE TS-1; ISSUE TS-13.

##### §2.2.2.8.2 — Tuple Struct Abstract Syntax (anchor)
Abstract types:
```
τ ::= N(τ₁,…,τₙ)      where n ≥ 1
```
Constructors are nominal; positions are significant; no field names appear in the type. CITE: §2.2.2.8.2 — Tuple Struct Abstract Syntax.

##### §2.2.2.8.3 — Tuple Struct Well‑Formedness (anchor)
Well‑formedness obligations:
- n ≥ 1 and each Γ ⊢ τᵢ : Type.
- Positions are significant; arity and order MUST match between declaration and use.
- Permission wrappers propagate componentwise over fields (own/mut/imm). CITE: §2.2.2.8.3 — Tuple Struct Well‑Formedness; CITE: §2.0.8.1 — Permissions; CITE: §2.0.6.3 — Well‑Formedness. ISSUE TS-1.

##### §2.2.2.8.4 — Tuple Struct Typing (anchor)
Typing rule (constructor formation):
```
[T-TupleStruct-Ctor]
Γ ⊢ e₁ : τ₁   …   Γ ⊢ eₙ : τₙ
────────────────────────────────
Γ ⊢ N(e₁,…,eₙ) : N(τ₁,…,τₙ)
```
Effects: constructor is pure unless it performs allocation; such allocation MUST be declared in uses ε at the declaration site. CITE: §2.2.2.8.4 — Tuple Struct Typing; CITE: §2.0.8.2 — Effects. ISSUE TS-1.

#### Section 3.1.3 Derived Forms
- Document shorthand patterns (field shorthand, tuple projection) that desugar to canonical forms. CITE: §2.2.1.3 — Tuple Static Semantics; CITE: §2.2.2.3 — Record Static Semantics. ISSUE TS-5.

### Section 3.2 Static Semantics
- Static semantics summary lists construction, projection, and destructuring rules plus open questions about trait/object integration. CITE: §2.2.0.5 — Static Semantics Summary. ISSUE TS-1; ISSUE TS-7.

#### Section 3.2.1 Typing Contexts and Judgments
- Outline references tuple/record typing rules that manage permissions for fields and self receivers. CITE: §2.2.1.3 — Tuple Static Semantics; CITE: §2.2.2.3 — Record Static Semantics. ISSUE TS-7.

#### Section 3.2.2 Well-Formedness Rules
- Highlight field uniqueness, visibility, and tuple-struct arity constraints, noting constructors still blocked by TS-1. CITE: §2.2.1.3.1 — Tuple Well-Formedness; CITE: §2.2.2.3.1 — Record Well-Formedness; CITE: §2.2.2.8.3 — Tuple Struct Well-Formedness. ISSUE TS-1.

#### Section 3.2.3 Subtyping and Equivalence
- Summaries record nominal equality for records and invariant tuple positions, with object-safety follow-up tracked separately. CITE: §2.2.1.3.3 — Tuple Type Properties; CITE: §2.2.2.3.3 — Record Type Properties. ISSUE TS-7.

#### Section 3.2.4 Permission-Indexed Types
- Document how owning, mutating, and immutable permissions flow through tuple/record fields and tuple-struct constructors. CITE: §2.2.0.3 — Guarantees; CITE: §2.2.2.4 — Record Permissions. ISSUE TS-2; ISSUE TS-7.

#### Section 3.2.5 Effect-Annotated Map Types
 - Outline notes method effects on records and tuple-structs, noting that side-effect summaries rely on Part IV tokens. CITE: §2.2.0.2 — Dependencies and Notation; CITE: §2.2.2.5 — Record Methods. See §1.2.5 for effect notation. ISSUE TS-5.
 - Effect notation: see Integration Correction preface and §1.2.5.

### Section 3.3 Dynamic Semantics
- Runtime summaries follow the left-to-right evaluation model and destructor behaviour recorded in §2.2.0.6. CITE: §2.2.0.6 — Dynamic Semantics Summary. ISSUE TS-5; ISSUE TS-2.

#### Section 3.3.1 Runtime Obligations
- Document initialization completeness and projection safety, referencing the tuple and record evaluation rules. CITE: §2.2.1.4 — Tuple Dynamic Semantics; CITE: §2.2.2.4 — Record Dynamic Semantics. ISSUE TS-5.

#### Section 3.3.2 Region Exit Semantics
- Summaries cover aggregate teardown within regions and note any pending aliasing guarantees for embedded views. CITE: §2.2.0.2 — Dependencies and Notation. ISSUE TS-2.

#### Section 3.3.3 Unsafe Interaction
- Record the limited unsafe hooks (repr overrides, raw projections) and their relationship to permission-enforced invariants. CITE: §2.2.2.6 — Record Representation Overrides. ISSUE TS-7.

### Section 3.4 Algorithms
- Algorithm notes track bidirectional checking reuse and the unmet tuple-struct constructor elaboration. CITE: §2.2.0.7 — Algorithms. ISSUE TS-1; ISSUE TS-6.

#### Section 3.4.1 Bidirectional Typing Procedure
- Outline inherits tuple/record checking paths from §2.2.1.3–§2.2.2.3, flagging tuple-struct creation as pending. CITE: §2.2.1.3 — Tuple Static Semantics; CITE: §2.2.2.3 — Record Static Semantics. ISSUE TS-1.

#### Section 3.4.2 Constraint Solving and Coherence
- Note constraints ensuring field types align and generics instantiate correctly, with associated-type gaps under TS-8. CITE: §2.2.2.7 — Record Generics. ISSUE TS-8; ISSUE TS-1.

#### Section 3.4.3 Error Diagnostics
- Summaries capture diagnostic expectations for missing fields, mismatched arity, and visibility violations once examples ship. CITE: §2.2.0.10 — Notes and Issues. ISSUE TS-5.

### Section 3.5 Proven Properties
- Outline restates that global progress/preservation already cover product types while pending tuple-struct proofs remain blocked. CITE: §2.2.0.8 — Proven Properties; CITE: §2.0.7 — Dynamic Semantics. ISSUE TS-1.

#### Section 3.5.1 Progress
- Call out tuple/record progress lemmas with tuple-struct additions deferred to TS-1. CITE: §2.2.1.4 — Tuple Dynamic Semantics; CITE: §2.2.2.4 — Record Dynamic Semantics. ISSUE TS-1.

#### Section 3.5.2 Preservation
- Summaries track preservation proofs for field updates and destructuring patterns while tuple-struct cases remain pending. CITE: §2.2.1.3 — Tuple Static Semantics; CITE: §2.2.2.3 — Record Static Semantics. ISSUE TS-1.

#### Section 3.5.3 Soundness Corollaries
- Document corollaries like field access safety and destructuring soundness, awaiting tuple-struct confirmation. CITE: §2.2.0.3 — Guarantees. ISSUE TS-1.

#### Section 3.5.4 Parametricity and Principal Types
- Note obligations for polymorphic records/tuples alongside associated-bound dependencies. CITE: §2.2.2.7 — Record Generics. ISSUE TS-8.

### Section 3.6 Interaction Summaries
- Interaction tables will chart permission and effect requirements for constructors, projections, and methods. CITE: §2.2.0.9 — Integration Notes; CITE: §2.2.0.10 — Notes and Issues. ISSUE TS-5.
#### [FFI‑Repr‑Table] FFI Representation Table (anchor)
- repr(packed), repr(C), transparent—centralized here; Chapters 6/7 reference this table. CITE: §2.2.2.6 — Record Representation Overrides.

### Section 3.A Appendices (Non-Normative)
- Appendices will bundle diagrams, examples, and backlog hooks once tuple-struct work closes. CITE: §2.2.0.9 — Integration Notes; CITE: §2.2.0.10 — Notes and Issues. ISSUE TS-1; ISSUE TS-5.

#### Section 3.A.1 Taxonomy Diagram and Cross-Reference Table
- Plan to show relationships between tuples, records, and tuple-structs, consistent with the canonical taxonomy. CITE: §2.2.0.1 — Scope; CITE: §2.0.10 — Type Taxonomy. ISSUE TS-1; ISSUE TS-5.

#### Section 3.A.2 Extended Examples
- Outline example programs covering construction, pattern matching, and method dispatch, pending the example compendium. CITE: §2.2.0.10 — Notes and Issues. ISSUE TS-5.

#### Section 3.A.3 Specification Status & Backlog
- Maintain references to the central backlog so tuple-struct progress and related TS issues stay synchronized. CITE: §2.0.11 — Specification Status; CITE: §2.0.13 — Outstanding Issues. ISSUE TS-1; ISSUE TS-2; ISSUE TS-5; ISSUE TS-7.


## Chapter 4: Sum and Modal Types

| Ready | Dependencies | Issues | Bridge | Notes |
| --- | --- | --- | --- | --- |
| [x] | CITE: §2.3.0 — Overview; CITE: §2.3.0.2 — Dependencies and Notation | TS-5, TS-6, TS-2, TS-7 | §2.3 canonical prose → Chapter 4 scaffold | Outline finalized and ready for normative text injection; exhaustiveness/modal verification remain as TS items |

### Section 4.0 Overview
- Sum types cover enums, unsafe unions, and modal state machines, guaranteeing discriminant fidelity and state-safe transitions. CITE: §2.3.0.1 — Scope; CITE: §2.3.0.3 — Guarantees. ISSUE TS-6.
- Integration aligns dependencies on permissions, effects, pattern matching, and unsafe hooks. CITE: §2.3.0.2 — Dependencies and Notation. ISSUE TS-2; ISSUE TS-7.

#### Section 4.0.1 Scope and Goals
- Outline repeats the canonical scope emphasising exhaustive matches, union safety, and modal verification goals. CITE: §2.3.0.1 — Scope. ISSUE TS-6.

#### Section 4.0.2 Dependencies and Notation
- Document reliance on permission transfers, effect tokens like `unsafe.union`, and pattern matching conventions. CITE: §2.3.0.2 — Dependencies and Notation. ISSUE TS-5; ISSUE TS-6.

#### Section 4.0.3 Guarantees
- Restate normative guarantees for discriminants, unsafe access, and modal transition enforcement. CITE: §2.3.0.3 — Guarantees. ISSUE TS-6.

### Section 4.1 Syntax
- Syntax overview collects enum, union, and modal grammars, highlighting pending figures in the example compendium. CITE: §2.3.0.4 — Syntax Summary. ISSUE TS-5.

#### Section 4.1.1 Concrete Grammar
- Note constructors for enums/variants, union field syntax, and modal state declarations. CITE: §2.3.1.2 — Enum Syntax; CITE: §2.3.2.2 — Union Syntax; CITE: §2.3.3.2 — Modal Syntax. ISSUE TS-5; ISSUE TS-6.

#### Section 4.1.2 Abstract Syntax
- Summaries capture AST nodes for variants, union fields, and modal transitions to align with typing rules. CITE: §2.3.1.2.2 — Enum Abstract Syntax; CITE: §2.3.2.2.2 — Union Abstract Syntax; CITE: §2.3.3.2.2 — Modal Abstract Syntax. ISSUE TS-6.

#### Section 4.1.3 Derived Forms
- Document shorthand pattern constructs and modal transition sugar with references to their desugaring rules. CITE: §2.3.1.3 — Enum Static Semantics; CITE: §2.3.3.3 — Modal Static Semantics. ISSUE TS-6.

### Section 4.2 Static Semantics
- Static semantics summary imports variant construction, pattern exhaustiveness, union safety, and modal transition typing. CITE: §2.3.0.5 — Static Semantics Summary. ISSUE TS-6; ISSUE TS-2.
 - Layout & ABI Notes (anchor): discriminant size/alignment, union payload layout, modal state representation.

#### Section 4.2.1 Typing Contexts and Judgments
- Outline references judgment forms for match arms, union permissions, and modal state ownership. CITE: §2.3.1.3 — Enum Static Semantics; CITE: §2.3.2.3 — Union Static Semantics; CITE: §2.3.3.3 — Modal Static Semantics. ISSUE TS-6.

#### Section 4.2.2 Well-Formedness Rules
- Capture invariants on discriminant ordering, union field completeness, and modal state connectivity. CITE: §2.3.1.3.1 — Enum Well-Formedness; CITE: §2.3.2.3.1 — Union Well-Formedness; CITE: §2.3.3.3.1 — Modal Well-Formedness. ISSUE TS-6.

#### Section 4.2.3 Subtyping and Equivalence
- Note that enums and modals are nominal while unions rely on explicit permissions, reinforcing object-safety dependencies. CITE: §2.3.1.3.3 — Enum Type Properties; CITE: §2.3.3.3.3 — Modal Type Properties. ISSUE TS-7.

#### Section 4.2.4 Permission-Indexed Types
- Summaries explain how permissions control variant construction, union access, and modal transitions. CITE: §2.3.0.3 — Guarantees; CITE: §2.3.3.4 — Modal Permissions. ISSUE TS-2; ISSUE TS-6.

#### Section 4.2.5 Effect-Annotated Map Types
 - Record effect requirements for unions and modals (see §1.2.9 for token names). CITE: §2.3.2.3.2 — Union Effects; CITE: §2.3.3.3.4 — Modal Effects. See §1.2.5 for effect notation. ISSUE TS-6.
 - Effect notation: see Integration Correction preface and §1.2.5.

### Section 4.3 Dynamic Semantics
- Runtime section recaps discriminant evaluation, union mutation, and modal state evolution. CITE: §2.3.0.6 — Dynamic Semantics Summary. ISSUE TS-6.

#### Section 4.3.1 Runtime Obligations
- Highlight obligations for match exhaustiveness, union validity checks, and modal transition side effects. CITE: §2.3.1.4 — Enum Dynamic Semantics; CITE: §2.3.2.4 — Union Dynamic Semantics; CITE: §2.3.3.4 — Modal Dynamic Semantics. ISSUE TS-6; ISSUE TS-2.

#### Section 4.3.2 Region Exit Semantics
- Note how modal permissions and union payloads interact with region teardown. CITE: §2.3.3.4.2 — Modal Region Semantics. ISSUE TS-2; ISSUE TS-6.

#### Section 4.3.3 Unsafe Interaction
- Document unsafe union reads/writes and modal escape hatches that require explicit effect tokens. CITE: §2.3.2.3.2 — Union Effects; CITE: §2.3.0.9 — Integration Notes. ISSUE TS-6.

### Section 4.4 Algorithms
- Outline acknowledges missing exhaustiveness and modal verification algorithms, tracking their delivery under TS-6. CITE: §2.3.0.7 — Algorithms. ISSUE TS-6.

#### Section 4.4.1 Bidirectional Typing Procedure
- Summaries explain how match checking feeds into bidirectional typing once algorithms land. CITE: §2.3.1.3 — Enum Static Semantics. ISSUE TS-6.
  - Placeholder: Exhaustiveness Check — Input: enum + patterns; Output: success or uncovered set; Pre: enum WF; Post: coverage proven or finite witness; Complexity: worst‑case exponential with practical heuristics.

#### Section 4.4.2 Constraint Solving and Coherence
- Document constraints ensuring match arms cover variants and modal transitions respect state graphs. CITE: §2.3.3.3 — Modal Static Semantics. ISSUE TS-6.

#### Section 4.4.3 Error Diagnostics
- Note diagnostic expectations for non-exhaustive matches, illegal union access, and invalid modal transitions pending example updates. CITE: §2.3.0.10 — Notes and Issues. ISSUE TS-5; ISSUE TS-6.

#### Section 4.4.4 Modal Verification Algorithms
- Reserve space for the state-machine verification workflow still tracked by TS-6. CITE: §2.3.3.5 — Modal Verification Notes. ISSUE TS-6.
  - Placeholder: Modal Verification — Input: modal graph + transitions; Output: verified obligations or counterexample; Pre: modal WF; Post: transitions respect permissions/effects/contracts; Complexity: polynomial in states+edges with bounded path exploration.

### Section 4.5 Proven Properties
- Outline restates that global safety theorems extend to sums and modals while specialized proofs await algorithm updates. CITE: §2.3.0.8 — Proven Properties; CITE: §2.0.7 — Dynamic Semantics. ISSUE TS-6.

#### Section 4.5.1 Progress
- Document progress obligations for matches, unions, and modals referencing canonical dynamic rules. CITE: §2.3.1.4 — Enum Dynamic Semantics; CITE: §2.3.3.4 — Modal Dynamic Semantics. ISSUE TS-6.

#### Section 4.5.2 Preservation
- Summaries highlight preservation lemmas for variant and modal transitions with pending automation under TS-6. CITE: §2.3.1.3 — Enum Static Semantics; CITE: §2.3.3.3 — Modal Static Semantics. ISSUE TS-6.

#### Section 4.5.3 Soundness Corollaries
- Note corollaries around discriminant integrity and modal state safety, awaiting modal verification proofs. CITE: §2.3.0.3 — Guarantees. ISSUE TS-6.

#### Section 4.5.4 Parametricity and Principal Types
- Outline references polymorphic enums/modals and associated constraints, with associated-type bounds tracked in TS-8. CITE: §2.3.1.3 — Enum Static Semantics. ISSUE TS-8; ISSUE TS-6.

### Section 4.6 Interaction Summaries
- Interaction tables will compile permission, effect, and contract requirements once algorithms finalize. CITE: §2.3.0.9 — Integration Notes; CITE: §2.3.0.10 — Notes and Issues. ISSUE TS-6; ISSUE TS-5.

#### Section 4.6.1 Modal Verification Summary
- Reserve space for modal verification metrics and obligations coordinated with the algorithm rollout. CITE: §2.3.3.5 — Modal Verification Notes. ISSUE TS-6.

### Section 4.A Appendices (Non-Normative)
- Appendices will contain variant tables, modal diagrams, and backlog tracking once the outstanding work lands. CITE: §2.3.0.9 — Integration Notes; CITE: §2.3.0.10 — Notes and Issues. ISSUE TS-6; ISSUE TS-5.

#### Section 4.A.1 Taxonomy Diagram and Cross-Reference Table
- Plan diagrams connecting enums, unions, and modals to the type taxonomy and permission system. CITE: §2.0.10 — Type Taxonomy; CITE: §2.3.0.1 — Scope. ISSUE TS-6; ISSUE TS-5.

#### Section 4.A.2 Extended Examples
- Outline example suites for exhaustive matches, union wrappers, and modal workflows pending the compendium. CITE: §2.3.0.10 — Notes and Issues. ISSUE TS-5; ISSUE TS-6.

#### Section 4.A.3 Specification Status & Backlog
- Keep modality-related blockers tied into the central backlog for visibility. CITE: §2.0.11 — Specification Status; CITE: §2.0.13 — Outstanding Issues. ISSUE TS-6; ISSUE TS-2; ISSUE TS-7.


## Chapter 5: Collections and Views

| Ready | Dependencies | Issues | Bridge | Notes |
| --- | --- | --- | --- | --- |
| [x] | CITE: §2.4.0 — Overview; CITE: §2.4.0.2 — Dependencies and Notation | TS-2, TS-5, TS-9 | §2.4 canonical prose → Chapter 5 scaffold | Outline finalized and ready for normative text injection; aliasing/diagnostics/examples tracked via TS items |

### Section 5.0 Overview
- Collections cover arrays, slices, and view types, guaranteeing bounds safety and explicit aliasing contracts. CITE: §2.4.0.1 — Scope; CITE: §2.4.0.3 — Guarantees. ISSUE TS-2.
- Integration preserves dependencies on pointer semantics, regions, and iteration constructs documented in §2.4.0.2. CITE: §2.4.0.2 — Dependencies and Notation. ISSUE TS-9.

#### Section 5.0.1 Scope and Goals
- Outline mirrors the canonical scope, emphasizing fixed-size arrays, dynamic slices, and view abstractions. CITE: §2.4.0.1 — Scope. ISSUE TS-2.

#### Section 5.0.2 Dependencies and Notation
- Summaries track pointer, region, and iterator notation inherited from prior chapters. CITE: §2.4.0.2 — Dependencies and Notation. ISSUE TS-2; ISSUE TS-9.

#### Section 5.0.3 Guarantees
- Restate guarantees around bounds checks, aliasing invariants, and region interactions. CITE: §2.4.0.3 — Guarantees. ISSUE TS-2.

### Section 5.1 Syntax
- Syntax outline references array and slice grammars together with forthcoming view constructors. CITE: §2.4.0.4 — Syntax Summary. ISSUE TS-5.

#### Section 5.1.1 Concrete Grammar
- Detail literals, indexing syntax, and view constructors as defined in §2.4.2. CITE: §2.4.2.1 — Concrete Syntax. ISSUE TS-5.

#### Section 5.1.2 Abstract Syntax
- Capture AST forms for array literals, slice expressions, and view descriptors. CITE: §2.4.2.2 — Abstract Syntax. ISSUE TS-2.

#### Section 5.1.3 Derived Forms
- Note syntactic sugar such as range literals and shorthand iteration, ensuring desugarings link back to core forms. CITE: §2.4.2 — Syntax. ISSUE TS-5.

### Section 5.2 Static Semantics
- Static semantics summarise construction, indexing, borrowing, and iteration typing rules with aliasing proofs outstanding. CITE: §2.4.0.5 — Static Semantics Summary. ISSUE TS-2; ISSUE TS-9.
 - Layout & ABI Notes (anchor): array element alignment/stride; slice fat‑pointer layout.

#### Section 5.2.1 Typing Contexts and Judgments
- Outline references typing judgments for array lengths, slice lifetimes, and view borrowing. CITE: §2.4.3 — Static Semantics. ISSUE TS-2.

#### Section 5.2.2 Well-Formedness Rules
- Document requirements on array lengths, slice bounds, and view invariants. CITE: §2.4.3.1 — Well-Formedness Rules. ISSUE TS-2.

#### Section 5.2.3 Subtyping and Equivalence
 - See §1.2.6 for variance table; arrays and slices are invariant. Note permitted coercions captured in §2.4.3.3. CITE: §2.4.3.3 — Type Properties. ISSUE TS-2.
#### [Coercions‑Ch5] Coercions Table (anchor)
- Explicit array → slice and String → str coercions with representation, variance, and effect/permission constraints.

#### Section 5.2.4 Permission-Indexed Types
- Summaries explain permission propagation for mutable and immutable slices/views, with aliasing still under TS-2. CITE: §2.4.0.3 — Guarantees; CITE: §2.4.3.2 — Type Rules. ISSUE TS-2.
  - See [Coercions‑Ch5] for which coercions require mut, which are imm‑safe, and any allocation effects.

#### Section 5.2.5 Effect-Annotated Map Types
 - Outline references effects required for allocation, copying, and unsafe conversions. CITE: §2.4.3.2 — Type Rules; CITE: §2.4.0.2 — Dependencies and Notation. See §1.2.5 for effect notation and §1.2.9 for token names. ISSUE TS-9.
 - Effect notation: see Integration Correction preface and §1.2.5.

### Section 5.3 Dynamic Semantics
- Runtime summaries highlight bounds checking, iterator validity, and region exit behaviour. CITE: §2.4.0.6 — Dynamic Semantics Summary. ISSUE TS-2.

#### Section 5.3.1 Runtime Obligations
- Enumerate dynamic checks for indexing, mutation, and iteration. CITE: §2.4.4 — Dynamic Semantics. ISSUE TS-2.

#### Section 5.3.2 Region Exit Semantics
- Note deallocation and aliasing requirements when collections live in regions. CITE: §2.4.4.2 — Region Semantics. ISSUE TS-2.

#### Section 5.3.3 Unsafe Interaction
- Summaries outline unsafe slice/view construction and raw pointer conversions, all guarded by explicit effects. CITE: §2.4.4.3 — Unsafe Interaction. ISSUE TS-9.

### Section 5.4 Algorithms
- Algorithm section captures planned analyses for aliasing verification and bounds diagnostics. CITE: §2.4.0.7 — Algorithms. ISSUE TS-2; ISSUE TS-9.

#### Section 5.4.1 Bidirectional Typing Procedure
- Outline covers how collection expressions integrate into bidirectional typing once aliasing rules are final. CITE: §2.4.3.2 — Type Rules. ISSUE TS-2.

#### Section 5.4.2 Constraint Solving and Coherence
- Document constraints tying length metrics to iteration invariants and region scopes. CITE: §2.4.3.2 — Type Rules; CITE: §2.4.5 — Verification Notes. ISSUE TS-2; ISSUE TS-9.

#### Section 5.4.3 Error Diagnostics
- Summaries note diagnostics for out-of-bounds access and aliasing violations as captured in the pending notes. CITE: §2.4.0.10 — Notes and Issues. ISSUE TS-5; ISSUE TS-2.

### Section 5.5 Proven Properties
- Outline reiterates the need for aliasing safety proofs and iteration invariants to align with global theorems. CITE: §2.4.0.8 — Proven Properties; CITE: §2.0.7 — Dynamic Semantics. ISSUE TS-2.
  - Conditional stability: coercions relying on aliasing proofs (TS‑2) are marked conditionally stable until proofs land.

#### Section 5.5.1 Progress
- Indicate progress obligations for indexed operations referencing canonical dynamic rules. CITE: §2.4.4 — Dynamic Semantics. ISSUE TS-2.

#### Section 5.5.2 Preservation
- Track preservation results for mutation and iteration, pending aliasing lemmas under TS-2. CITE: §2.4.3 — Static Semantics. ISSUE TS-2.

#### Section 5.5.3 Soundness Corollaries
- Summaries mention corollaries around bounds and aliasing safety awaiting completion of TS-2 deliverables. CITE: §2.4.0.3 — Guarantees. ISSUE TS-2.

#### Section 5.5.4 Parametricity and Principal Types
- Outline references generic collection behaviour with associated-type dependencies logged under TS-8. CITE: §2.4.3.2 — Type Rules. ISSUE TS-8; ISSUE TS-2.

### Section 5.6 Interaction Summaries
- Interaction tables will summarise permissions, effects, and contracts once aliasing work stabilizes. CITE: §2.4.0.9 — Integration Notes; CITE: §2.4.0.10 — Notes and Issues. ISSUE TS-2; ISSUE TS-5.

### Section 5.A Appendices (Non-Normative)
- Appendices will host diagrams, extended examples, and backlog hooks for collections and views. CITE: §2.4.0.9 — Integration Notes; CITE: §2.4.0.10 — Notes and Issues. ISSUE TS-2; ISSUE TS-5.

#### Section 5.A.1 Taxonomy Diagram and Cross-Reference Table
- Plan diagrams connecting arrays/slices/views to the type taxonomy and pointer chapter. CITE: §2.0.10 — Type Taxonomy; CITE: §2.4.0.1 — Scope. ISSUE TS-2.

#### Section 5.A.2 Extended Examples
- Outline example scenarios for bounds-safe iteration and alias-aware mutation once the compendium is ready. CITE: §2.4.0.10 — Notes and Issues. ISSUE TS-5; ISSUE TS-2.

#### Section 5.A.3 Specification Status & Backlog
- Keep collection-related blockers synchronised with the global backlog entries for TS-2 and TS-9. CITE: §2.0.11 — Specification Status; CITE: §2.0.13 — Outstanding Issues. ISSUE TS-2; ISSUE TS-9; ISSUE TS-5.


## Chapter 6: Reference and Pointer Types

| Ready | Dependencies | Issues | Bridge | Notes |
| --- | --- | --- | --- | --- |
| [x] | CITE: §2.5.0 — Overview; CITE: §2.5.0.2 — Dependencies and Notation | TS-5, TS-7, TS-9 | §2.5 canonical prose → Chapter 6 scaffold | Outline finalized and ready for normative text injection; unsafe guidance/object safety/examples tracked via TS items |

### Section 6.0 Overview
- Reference and pointer types distinguish safe `Ptr<T>@State` handles from raw pointers while preserving LPS guarantees. CITE: §2.5.0.1 — Scope; CITE: §2.5.0.3 — Guarantees. ISSUE TS-7.
- Integration references permissions, modal states, and region behaviour outlined in §2.5.0.2. CITE: §2.5.0.2 — Dependencies and Notation. ISSUE TS-9.

#### Section 6.0.1 Scope and Goals
- Outline mirrors canonical scope covering safe pointers, raw pointers, and their intended use cases. CITE: §2.5.0.1 — Scope. ISSUE TS-7.

#### Section 6.0.2 Dependencies and Notation
- Summaries document notation for pointer qualifiers, modal state annotations, and unsafe effect tokens. CITE: §2.5.0.2 — Dependencies and Notation. ISSUE TS-9.

#### Section 6.0.3 Guarantees
- Restate safety model separating capability-checked pointers from raw operations that require `unsafe`. CITE: §2.5.0.3 — Guarantees. ISSUE TS-7.

### Section 6.1 Syntax
- Syntax outline collects pointer declaration forms, dereference expressions, and capability annotations. CITE: §2.5.0.4 — Syntax Summary. ISSUE TS-5.

#### Section 6.1.1 Concrete Grammar
- Detail concrete syntax for pointer types, dereference, address-of, and pointer casts. CITE: §2.5.2.1 — Concrete Syntax. ISSUE TS-5.

#### Section 6.1.2 Abstract Syntax
- Capture AST nodes for pointer forms and dereference expressions to keep static rules aligned. CITE: §2.5.2.2 — Abstract Syntax. ISSUE TS-7.

#### Section 6.1.3 Derived Forms
- Note optional sugar such as safe pointer constructors or helper macros (none today) and log pending examples. CITE: §2.5.0.4 — Syntax Summary. ISSUE TS-5.

### Section 6.2 Static Semantics
- Static semantics summary covers pointer capability checks, permission conversions, and unsafe preconditions. CITE: §2.5.0.5 — Static Semantics Summary. ISSUE TS-7.
 - Layout & ABI Notes (anchor): pointer size/alignment; safe handle representation.

#### Section 6.2.1 Typing Contexts and Judgments
- Outline references judgments for pointer creation, capability checks, and state-indexed types. CITE: §2.5.3 — Static Semantics. ISSUE TS-7.

#### Section 6.2.2 Well-Formedness Rules
- Document conditions on pointer kinds, state annotations, and raw pointer usage. CITE: §2.5.3.1 — Well-Formedness Rules. ISSUE TS-7.

#### Section 6.2.3 Subtyping and Equivalence
- See §1.2.6 for variance table; summarise pointer invariance and capability-based equivalence. CITE: §2.5.3.3 — Type Properties. ISSUE TS-7.

#### Section 6.2.4 Permission-Indexed Types
- Note how LPS permissions combine with pointer capabilities and how transitions consume/produce states. CITE: §2.5.0.3 — Guarantees; CITE: §2.5.4 — Permission Integration. ISSUE TS-7.

#### Section 6.2.5 Effect-Annotated Map Types
 - Record effects required for raw pointer dereference and capability upgrades. CITE: §2.5.3.2 — Type Rules; CITE: §2.5.4.2 — Unsafe Effects. ISSUE TS-9.
 - Effect notation: see Integration Correction preface and §1.2.5.

### Section 6.3 Dynamic Semantics
- Runtime outline captures dereference behaviour, safety checks, and state transitions. CITE: §2.5.0.6 — Dynamic Semantics Summary. ISSUE TS-7.

#### Section 6.3.1 Runtime Obligations
- Highlight obligations for runtime capability checks, null handling, and aliasing compliance. CITE: §2.5.4.3 — Runtime Semantics. ISSUE TS-7.

#### Section 6.3.2 Region Exit Semantics
- Note how pointers interact with region teardown and lifetimes, emphasising non-escape requirements. CITE: §2.5.4.4 — Region Semantics. ISSUE TS-2; ISSUE TS-7.

#### Section 6.3.3 Unsafe Interaction
- Summaries document raw pointer operations, required effect tokens, and verification hooks. CITE: §2.5.4.2 — Unsafe Effects. ISSUE TS-9.

### Section 6.4 Algorithms
- Algorithm notes track capability checking, state transition verification, and pointer alias analysis. CITE: §2.5.0.7 — Algorithms. ISSUE TS-7; ISSUE TS-9.

#### Section 6.4.1 Bidirectional Typing Procedure
- Outline references bidirectional rules for pointer expressions, ensuring capability checks integrate with synth/check. CITE: §2.5.3.2 — Type Rules. ISSUE TS-7.

#### Section 6.4.2 Constraint Solving and Coherence
- Document constraints on state transitions and capability upgrades with any unsolved coherence items recorded. CITE: §2.5.4 — Permission Integration. ISSUE TS-7; ISSUE TS-9.

#### Section 6.4.3 Error Diagnostics
- Summaries flag diagnostics for invalid dereference, missing effects, and capability mismatches once examples land. CITE: §2.5.0.10 — Notes and Issues. ISSUE TS-5.

### Section 6.5 Proven Properties
- Outline restates that pointer safety builds on global theorems plus capability-specific lemmas. CITE: §2.5.0.8 — Proven Properties; CITE: §2.0.7 — Dynamic Semantics. ISSUE TS-7.

#### Section 6.5.1 Progress
- Document progress obligations for pointer evaluation referencing canonical rules. CITE: §2.5.4.3 — Runtime Semantics. ISSUE TS-7.

#### Section 6.5.2 Preservation
- Summaries note preservation lemmas for capability transitions and pointer mutation. CITE: §2.5.3 — Static Semantics. ISSUE TS-7.

#### Section 6.5.3 Soundness Corollaries
- Capture soundness corollaries ensuring safe pointer dereference and modal state alignment. CITE: §2.5.0.3 — Guarantees. ISSUE TS-7.

#### Section 6.5.4 Parametricity and Principal Types
- Outline references pointer generics and capability polymorphism with outstanding object-safety work tracked by TS-7. CITE: §2.5.3.2 — Type Rules. ISSUE TS-7.

### Section 6.6 Interaction Summaries
- Interaction tables will present permission, effect, and contract requirements for pointer APIs. CITE: §2.5.0.9 — Integration Notes; CITE: §2.5.0.10 — Notes and Issues. ISSUE TS-5.
#### [Caps‑Ch6] Capability Matrix (anchor)
- Operation × required permission/effect × safety obligations, adjacent to typing rules for auditability.

### Section 6.A Appendices (Non-Normative)
- Appendices will collate pointer safety checklists, extended examples, and backlog hooks. CITE: §2.5.0.9 — Integration Notes; CITE: §2.5.0.10 — Notes and Issues. ISSUE TS-5; ISSUE TS-7.

#### Section 6.A.1 Taxonomy Diagram and Cross-Reference Table
- Plan diagrams linking pointers to modal states and permission wrappers in the global taxonomy. CITE: §2.0.10 — Type Taxonomy; CITE: §2.5.0.1 — Scope. ISSUE TS-7.

#### Section 6.A.2 Extended Examples
- Outline example suites for safe pointer transitions, raw interop, and capability upgrades once available. CITE: §2.5.0.10 — Notes and Issues. ISSUE TS-5; ISSUE TS-7.

#### Section 6.A.3 Specification Status & Backlog
- Keep pointer-related blockers tied into the global backlog so unsafe guidance and trait hooks stay visible. CITE: §2.0.11 — Specification Status; CITE: §2.0.13 — Outstanding Issues. ISSUE TS-7; ISSUE TS-9; ISSUE TS-5.


## Chapter 7: Raw and Unsafe Pointers

| Ready | Dependencies | Issues | Bridge | Notes |
| --- | --- | --- | --- | --- |
| [x] | CITE: §2.5.0.2 — Dependencies and Notation; CITE: §2.5.4 — Permission Integration | TS-5, TS-7, TS-9, TS-2 | §2.5 raw pointer prose → Chapter 7 scaffold | Outline finalized and ready for normative text injection; unsafe/diagnostics/aliasing proofs remain TS items |

### Section 7.0 Overview
- Raw pointers provide explicit escape hatches from LPS guarantees, requiring `unsafe` effects and manual validation. CITE: §2.5.0.1 — Scope; CITE: §2.5.0.3 — Guarantees. ISSUE TS-7.
- Integration tracks dependencies on effect tokens, FFI conventions, and region constraints for raw pointers. CITE: §2.5.0.2 — Dependencies and Notation. ISSUE TS-9; ISSUE TS-2.

#### Section 7.0.1 Scope and Goals
- Outline mirrors canonical scope describing raw pointer capabilities, motivations, and caveats. CITE: §2.5.0.1 — Scope. ISSUE TS-7.

#### Section 7.0.2 Dependencies and Notation
- Summaries note effect tokens (`unsafe.ptr`, `unsafe.union`), modal states, and FFI hooks relevant to raw pointers. CITE: §2.5.0.2 — Dependencies and Notation; CITE: §2.5.4.2 — Unsafe Effects. ISSUE TS-9.

#### Section 7.0.3 Guarantees
- Restate minimal guarantees: type compatibility only, with all safety delegated to caller responsibility. CITE: §2.5.0.3 — Guarantees. ISSUE TS-7.

### Section 7.1 Syntax
- Syntax section catalogs raw pointer type forms, dereference expressions, and `unsafe` blocks. CITE: §2.5.2.1 — Concrete Syntax. ISSUE TS-5.

#### Section 7.1.1 Concrete Grammar
- Detail grammar for `*T`, `*mut T`, address-of, casts, and unsafe blocks. CITE: §2.5.2.1 — Concrete Syntax. ISSUE TS-5.

#### Section 7.1.2 Abstract Syntax
- Capture AST nodes for raw pointer types and dereference expressions. CITE: §2.5.2.2 — Abstract Syntax. ISSUE TS-7.

#### Section 7.1.3 Derived Forms
- Summaries document any sugar (e.g., `unsafe { *ptr }`) and link to desugaring rules. CITE: §2.5.2.1 — Concrete Syntax. ISSUE TS-5.

### Section 7.2 Static Semantics
- Static semantics highlight effect requirements, cast rules, and interactions with safe pointers. CITE: §2.5.3 — Static Semantics; CITE: §2.5.4.2 — Unsafe Effects. ISSUE TS-7; ISSUE TS-9.

#### Section 7.2.1 Typing Contexts and Judgments
- Outline references typing rules for raw pointer expressions and their effect prerequisites. CITE: §2.5.3.2 — Type Rules. ISSUE TS-7.

#### Section 7.2.2 Well-Formedness Rules
- Document requirements for raw pointer initialization, null allowances, and cast validity. CITE: §2.5.3.1 — Well-Formedness Rules. ISSUE TS-7.

#### Section 7.2.3 Subtyping and Equivalence
- See §1.2.6 for variance table; summarise conversions between safe and raw pointers. CITE: §2.5.3.3 — Type Properties. ISSUE TS-7.

#### Section 7.2.4 Permission-Indexed Types
- Note how permissions affect conversion to/from raw pointers and which guarantees are forfeited. CITE: §2.5.4 — Permission Integration. ISSUE TS-7; ISSUE TS-2.

#### Section 7.2.5 Effect-Annotated Map Types
 - Record effects required for raw pointer operations (see §1.2.9). CITE: §2.5.4.2 — Unsafe Effects. ISSUE TS-9.
 - Effect notation: see Integration Correction preface and §1.2.5.
 - Raw function pointers vs map: link to Chapter 11 for callable representations and to Chapter 3 [FFI‑Repr‑Table] for ABI.

### Section 7.3 Dynamic Semantics
- Runtime summaries emphasise programmer responsibilities: manual null checks, aliasing control, and lifetime management. CITE: §2.5.4.3 — Runtime Semantics. ISSUE TS-7; ISSUE TS-2.

#### Section 7.3.1 Runtime Obligations
- Outline obligations for validating pointer provenance, alignment, and lifetime before dereference. CITE: §2.5.4.3 — Runtime Semantics. ISSUE TS-7.

#### Section 7.3.2 Region Exit Semantics
- Note that raw pointers can escape regions, leaving cleanup to programmer discipline. CITE: §2.5.4.4 — Region Semantics. ISSUE TS-2.

#### Section 7.3.3 Unsafe Interaction
- Summaries document interactions with unions, transmute, and FFI, all requiring explicit effects. CITE: §2.5.4.2 — Unsafe Effects. ISSUE TS-9.

### Section 7.4 Algorithms
- Algorithm placeholders collect linting, capability audits, and static analyses still to be specified for unsafe code. CITE: §2.5.0.7 — Algorithms. ISSUE TS-9.

#### Section 7.4.1 Bidirectional Typing Procedure
- Outline references how raw pointer expressions integrate with synth/check under declared effects. CITE: §2.5.3.2 — Type Rules. ISSUE TS-7.

#### Section 7.4.2 Constraint Solving and Coherence
- Document constraints ensuring casts and pointer arithmetic remain coherent with type intents. CITE: §2.5.4 — Permission Integration. ISSUE TS-7; ISSUE TS-9.

#### Section 7.4.3 Error Diagnostics
- Summaries highlight diagnostic requirements for unsafe misuse pending example coverage. CITE: §2.5.0.10 — Notes and Issues. ISSUE TS-5.

### Section 7.5 Proven Properties
- Outline indicates that only limited safety results apply to raw pointers, referencing canonical warnings. CITE: §2.5.0.8 — Proven Properties; CITE: §2.5.4.3 — Runtime Semantics. ISSUE TS-7.

#### Section 7.5.1 Progress
- Document progress assumptions for unsafe code when effect obligations are met. CITE: §2.5.4.3 — Runtime Semantics. ISSUE TS-7.

#### Section 7.5.2 Preservation
- Note preservation conditions contingent on programmer-maintained invariants. CITE: §2.5.3 — Static Semantics. ISSUE TS-7.

#### Section 7.5.3 Soundness Corollaries
- Summaries reiterate that raw pointer usage can subvert soundness without manual discipline. CITE: §2.5.0.3 — Guarantees. ISSUE TS-7.

#### Section 7.5.4 Parametricity and Principal Types
- Outline references limitations on parametric reasoning once raw pointers are introduced. CITE: §2.5.3.3 — Type Properties. ISSUE TS-7; ISSUE TS-8.

### Section 7.6 Interaction Summaries
- Interaction tables will track effects, permissions, and contracts required to compartmentalize unsafe code. CITE: §2.5.0.9 — Integration Notes; CITE: §2.5.0.10 — Notes and Issues. ISSUE TS-7; ISSUE TS-5.
#### [Caps‑Ch7] Capability Matrix (anchor)
- Unsafe read/write, cast, address arithmetic, interop calls with explicit effect tokens and pre/post safety obligations.

### Section 7.A Appendices (Non-Normative)
- Appendices will hold unsafe coding guidelines, FFI patterns, and backlog references. CITE: §2.5.0.9 — Integration Notes; CITE: §2.5.0.10 — Notes and Issues. ISSUE TS-7; ISSUE TS-5.

#### Section 7.A.1 Taxonomy Diagram and Cross-Reference Table
- Plan diagrams linking raw pointers to safe pointer wrappers, FFI modules, and permission rules. CITE: §2.0.10 — Type Taxonomy; CITE: §2.5.0.2 — Dependencies and Notation. ISSUE TS-7.

#### Section 7.A.2 Extended Examples
- Outline example unsafe blocks, FFI shims, and manual lifetime management scenarios pending the compendium. CITE: §2.5.0.10 — Notes and Issues. ISSUE TS-5; ISSUE TS-7.

#### Section 7.A.3 Specification Status & Backlog
- Keep unsafe-pointer work items visible alongside TS-7 and TS-9 in the global backlog. CITE: §2.0.11 — Specification Status; CITE: §2.0.13 — Outstanding Issues. ISSUE TS-7; ISSUE TS-9; ISSUE TS-5.


## Chapter 8: Traits and Code Reuse

| Ready | Dependencies | Issues | Bridge | Notes |
| --- | --- | --- | --- | --- |
| [x] | CITE: §2.6.0 — Overview; CITE: §2.6.0.2 — Dependencies and Notation | TS-5, TS-7, TS-8 | §2.6 canonical prose → Chapter 8 scaffold | Outline finalized and ready for normative text injection; object‑safety/bounds/examples tracked via TS items |

### Section 8.0 Overview
- Traits provide reusable implementations and behavioural abstractions distinct from contracts, with explicit permission-aware receivers. CITE: §2.6.0.1 — Scope; CITE: §2.6.0.3 — Guarantees. ISSUE TS-7.
- Integration references dependencies on generics, permissions, and contracts enumerated in §2.6.0.2. CITE: §2.6.0.2 — Dependencies and Notation. ISSUE TS-8.

#### Section 8.0.1 Scope and Goals
- Outline mirrors canonical coverage: trait declarations, impl blocks, trait objects, and default methods. CITE: §2.6.0.1 — Scope. ISSUE TS-7.

#### Section 8.0.2 Dependencies and Notation
- Summaries reiterate shared metavariables, `$` receiver notation, and interplay with Part III–V subsystems. CITE: §2.6.0.2 — Dependencies and Notation. ISSUE TS-8.

#### Section 8.0.3 Guarantees
- Restate guarantees on method resolution, coherence, and required object-safety checks. CITE: §2.6.0.3 — Guarantees. ISSUE TS-7.

### Section 8.1 Syntax
- Syntax outline catalogues trait declarations, impl blocks, trait objects, and `$` receivers. CITE: §2.6.0.4 — Syntax Summary. ISSUE TS-5.

#### Section 8.1.1 Concrete Grammar
- Detail EBNF for `trait`, `impl trait for`, default methods, and trait object usage. CITE: §2.6.2.1 — Concrete Syntax. ISSUE TS-5.

#### Section 8.1.2 Abstract Syntax
- Capture AST nodes for trait definitions, method signatures, and impls to align with typing rules. CITE: §2.6.2.2 — Abstract Syntax. ISSUE TS-7.

#### Section 8.1.3 Derived Forms
- Summaries note shorthand such as `$` self receivers and blanket impl sugar, referencing desugarings. CITE: §2.6.2.3 — Basic Examples. ISSUE TS-5.

### Section 8.2 Static Semantics
- Static semantics overview enumerates trait well-formedness, method type checking, and coherence constraints. CITE: §2.6.0.5 — Static Semantics Summary. ISSUE TS-7; ISSUE TS-8.

#### Section 8.2.1 Typing Contexts and Judgments
- Outline references contexts for trait declarations, impl blocks, and trait objects. CITE: §2.6.3.2 — Type Rules. ISSUE TS-7.

#### Section 8.2.2 Well-Formedness Rules
 - Document requirements for trait signatures, method bodies, and associated type completeness. CITE: §2.6.3.1 — Well-Formedness Rules. ISSUE TS-8.
#### [ObjSafety‑Checklist] Object‑Safety Checklist (anchor)
- Minimal checklist to be completed with normative text; §8.3.1 cites this anchor. ISSUE TS-7.

#### Section 8.2.3 Subtyping and Equivalence
- Summaries highlight trait object subtyping, auto-trait interactions, and equivalence of impls. CITE: §2.6.3.3 — Type Properties. ISSUE TS-7.

#### Section 8.2.4 Permission-Indexed Types
- Record how receiver permissions (`own`, `mut`, `imm`) influence trait method availability. CITE: §2.6.0.3 — Guarantees; CITE: §2.6.2.3 — Basic Examples. ISSUE TS-7.

#### Section 8.2.5 Effect-Annotated Map Types
 - Outline effect annotations on trait methods and how impls must satisfy declared `uses`. CITE: §2.6.3.2 — Type Rules. ISSUE TS-5.
 - Effect notation: see Integration Correction preface and §1.2.5.

### Section 8.3 Dynamic Semantics
- Runtime summary covers vtable-free dispatch, trait object layout, and receiver state transitions. CITE: §2.6.0.6 — Dynamic Semantics Summary. ISSUE TS-7.

#### Section 8.3.1 Runtime Obligations
 - Highlight obligations for method dispatch, including receiver permission checks and effect respect. Cite [ObjSafety‑Checklist] in §8.2.2. CITE: §2.6.4 — Dynamic Semantics. ISSUE TS-7.

#### Section 8.3.2 Region Exit Semantics
- Note how trait objects and impls interact with region teardown and borrowed receivers. CITE: §2.6.4.2 — Region Semantics. ISSUE TS-2; ISSUE TS-7.

#### Section 8.3.3 Unsafe Interaction
- Summaries document unsafe hooks (trait object downcasts, raw vtable access) that require explicit effects. CITE: §2.6.4.3 — Unsafe Interaction. ISSUE TS-7; ISSUE TS-9.

### Section 8.4 Algorithms
- Algorithm outline references coherence checking, method resolution, and future object-safety validation. CITE: §2.6.0.7 — Algorithms. ISSUE TS-7; ISSUE TS-8.

#### Section 8.4.1 Bidirectional Typing Procedure
- Describe how trait methods integrate with bidirectional typing, including `$` receiver synthesis. CITE: §2.6.3.2 — Type Rules. ISSUE TS-7.

#### Section 8.4.2 Constraint Solving and Coherence
 - Summaries capture constraints for overlapping impls, specialization, and associated type bounds. CITE: §2.6.3.2 — Type Rules; CITE: §2.6.5 — Trait Patterns; CITE: Property 6.3 (Coherence). ISSUE TS-7; ISSUE TS-8.

#### Section 8.4.3 Error Diagnostics
- Outline diagnostics for missing impls, conflicting methods, and object-safety violations. CITE: §2.6.0.10 — Notes and Issues. ISSUE TS-5; ISSUE TS-7.

### Section 8.5 Proven Properties
- Outline restates trait soundness obligations and pending object-safety proofs. CITE: §2.6.0.8 — Proven Properties; CITE: §2.0.7 — Dynamic Semantics. ISSUE TS-7.
 - Conditional stability: trait objects are conditionally stable until object‑safety rules (TS‑7) land.

#### Section 8.5.1 Progress
- Document progress lemmas for trait method dispatch. CITE: §2.6.4 — Dynamic Semantics. ISSUE TS-7.

#### Section 8.5.2 Preservation
- Summaries highlight preservation under trait method calls and impl instantiation. CITE: §2.6.3 — Static Semantics. ISSUE TS-7.

#### Section 8.5.3 Soundness Corollaries
- Note corollaries ensuring trait coherence and no missing method dispatch at runtime. CITE: §2.6.0.3 — Guarantees. ISSUE TS-7.

#### Section 8.5.4 Parametricity and Principal Types
- Outline references trait generics and associated type uniqueness, pending TS-8 work. CITE: §2.6.3.2 — Type Rules. ISSUE TS-8.

### Section 8.6 Interaction Summaries
- Interaction tables will consolidate permissions, effects, and contracts per trait pattern. CITE: §2.6.0.9 — Integration Notes; CITE: §2.6.0.10 — Notes and Issues. ISSUE TS-5; ISSUE TS-7.

### Section 8.A Appendices (Non-Normative)
- Appendices plan to deliver trait diagrams, example suites, and backlog tie-ins. CITE: §2.6.0.9 — Integration Notes; CITE: §2.6.0.10 — Notes and Issues. ISSUE TS-5; ISSUE TS-7.

#### Section 8.A.1 Taxonomy Diagram and Cross-Reference Table
- Diagram notes link traits to contracts, generics, and map types following the global taxonomy. CITE: §2.0.10 — Type Taxonomy; CITE: §2.6.0.1 — Scope. ISSUE TS-7.

#### Section 8.A.2 Extended Examples
- Outline example impls, default methods, and trait objects pending the example compendium. CITE: §2.6.0.10 — Notes and Issues. ISSUE TS-5; ISSUE TS-7.

#### Section 8.A.3 Specification Status & Backlog
- Maintain trait-related blockers (object safety, associated bounds) in sync with the global backlog. CITE: §2.0.11 — Specification Status; CITE: §2.0.13 — Outstanding Issues. ISSUE TS-7; ISSUE TS-8; ISSUE TS-5.


## Chapter 9: Marker Traits and Utilities

| Ready | Dependencies | Issues | Bridge | Notes |
| --- | --- | --- | --- | --- |
| [x] | CITE: §2.0.10 — Type Taxonomy; CITE: §2.0.11 — Specification Status | TS-4, TS-5 | §2.8 marker entries → Chapter 9 scaffold | Outline finalized and ready for normative text injection; marker semantics/examples tracked via TS items |

### Section 9.0 Overview
- Marker traits (Send, Sync, Sized, Copy extensions) remain placeholders awaiting normative text. CITE: §2.0.11 — Specification Status; CITE: §2.0.13 — Outstanding Issues. ISSUE TS-4.
- Integration plan keeps markers aligned with the taxonomy row for utility markers until canonical content lands. CITE: §2.0.10 — Type Taxonomy. ISSUE TS-5.

#### Section 9.0.1 Scope and Goals
- Outline records intended coverage (auto-traits, opt-in markers, FFI adapters) per backlog notes. CITE: §2.0.11 — Specification Status. ISSUE TS-4.

#### Section 9.0.2 Dependencies and Notation
- Summaries list dependencies on permissions, threading model, and generics referenced in backlog descriptions. CITE: §2.0.13 — Outstanding Issues. ISSUE TS-4.

#### Section 9.0.3 Guarantees
- Future guarantees (thread-safety, auto-implementation) remain unspecified until normative text lands. CITE: §2.0.13 — Outstanding Issues. ISSUE TS-4.

### Section 9.1 Syntax
- Record that syntax for marker declarations and annotations is pending specification. CITE: §2.0.11 — Specification Status. ISSUE TS-4; ISSUE TS-5.

#### Section 9.1.1 Concrete Grammar
- Identify forthcoming grammar for declaring markers and applying them to types. CITE: §2.0.13 — Outstanding Issues. ISSUE TS-4.

#### Section 9.1.2 Abstract Syntax
- Note abstract syntax will mirror trait declarations once markers are defined. CITE: §2.0.10 — Type Taxonomy. ISSUE TS-4.

#### Section 9.1.3 Derived Forms
- Shorthand or auto-implementation syntax is not yet specified; this outline tracks the pending work. CITE: §2.0.13 — Outstanding Issues. ISSUE TS-4.

### Section 9.2 Static Semantics
- Document that typing, auto-implementation rules, and coherence checks remain under TS-4. CITE: §2.0.11 — Specification Status. ISSUE TS-4.

#### Section 9.2.1 Typing Contexts and Judgments
- Track required judgments for marker attachment once rules exist. CITE: §2.0.13 — Outstanding Issues. ISSUE TS-4.

#### Section 9.2.2 Well-Formedness Rules
- Well-formedness rules for marker declarations and usage remain to be written; the backlog reference keeps them visible. CITE: §2.0.13 — Outstanding Issues. ISSUE TS-4.

#### Section 9.2.3 Subtyping and Equivalence
- Record that marker-induced subtyping/equivalence remains undefined. CITE: §2.0.13 — Outstanding Issues. ISSUE TS-4.

#### Section 9.2.4 Permission-Indexed Types
- Note the planned interaction between markers and permissions reference the backlog. CITE: §2.0.13 — Outstanding Issues. ISSUE TS-4.

#### Section 9.2.5 Effect-Annotated Map Types
 - Effect interactions will be defined alongside marker semantics once TS-4 closes. CITE: §2.0.13 — Outstanding Issues. ISSUE TS-4.
 - Effect notation: see Integration Correction preface and §1.2.5.

### Section 9.3 Dynamic Semantics
- State that runtime implications for markers are pending canonical text. CITE: §2.0.11 — Specification Status. ISSUE TS-4.

#### Section 9.3.1 Runtime Obligations
- Runtime guarantees (for example thread-safety) await the future normative text. CITE: §2.0.13 — Outstanding Issues. ISSUE TS-4.

#### Section 9.3.2 Region Exit Semantics
- Note interactions with regions will be documented in the future specification. CITE: §2.0.13 — Outstanding Issues. ISSUE TS-4.

#### Section 9.3.3 Unsafe Interaction
- Unsafe requirements will be documented when marker semantics are delivered. CITE: §2.0.13 — Outstanding Issues. ISSUE TS-4.

### Section 9.4 Algorithms
- Record the need for auto-trait resolution and diagnostic algorithms tracked by TS-4. CITE: §2.0.13 — Outstanding Issues. ISSUE TS-4.

#### Section 9.4.1 Bidirectional Typing Procedure
- Integration into type checking is deferred to the marker semantics update. CITE: §2.0.13 — Outstanding Issues. ISSUE TS-4.

#### Section 9.4.2 Constraint Solving and Coherence
- Note future coherence checks for marker propagation. CITE: §2.0.13 — Outstanding Issues. ISSUE TS-4.

#### Section 9.4.3 Error Diagnostics
- Document diagnostic plans to surface missing or conflicting markers once available. CITE: §2.0.13 — Outstanding Issues. ISSUE TS-4.

### Section 9.5 Proven Properties
- Formal proofs will follow once marker semantics exist; this outline reserves the section. CITE: §2.0.11 — Specification Status. ISSUE TS-4.

#### Section 9.5.1 Progress
- Note anticipated progress proofs for marker-enforced behaviours. CITE: §2.0.13 — Outstanding Issues. ISSUE TS-4.

#### Section 9.5.2 Preservation
- Preservation results will be supplied with the marker specification. CITE: §2.0.13 — Outstanding Issues. ISSUE TS-4.

#### Section 9.5.3 Soundness Corollaries
- Note corollaries (thread-safety, layout) will be added with the normative content. CITE: §2.0.11 — Specification Status. ISSUE TS-4.

#### Section 9.5.4 Parametricity and Principal Types
- Parametric guarantees tied to marker propagation remain pending future work. CITE: §2.0.13 — Outstanding Issues. ISSUE TS-4.

### Section 9.6 Interaction Summaries
- Interaction tables await marker semantics; backlog will supply data once TS-4 closes. CITE: §2.0.11 — Specification Status. ISSUE TS-4; ISSUE TS-5.

### Section 9.A Appendices (Non-Normative)
- Appendices will carry diagrams, examples, and backlog updates when markers are specified. CITE: §2.0.10 — Type Taxonomy; CITE: §2.0.13 — Outstanding Issues. ISSUE TS-4; ISSUE TS-5.

#### Section 9.A.1 Taxonomy Diagram and Cross-Reference Table
- Note requirement to align future diagrams with the taxonomy row for utility markers. CITE: §2.0.10 — Type Taxonomy. ISSUE TS-4.

#### Section 9.A.2 Extended Examples
- Example suites (thread-safe types, sized constraints) will be added when the compendium lands. CITE: §2.0.13 — Outstanding Issues. ISSUE TS-5; ISSUE TS-4.

#### Section 9.A.3 Specification Status & Backlog
- Keep marker trait backlog entries visible for coordination. CITE: §2.0.11 — Specification Status; CITE: §2.0.13 — Outstanding Issues. ISSUE TS-4; ISSUE TS-5.


## Chapter 10: Parametric Features

| Ready | Dependencies | Issues | Bridge | Notes |
| --- | --- | --- | --- | --- |
| [x] | CITE: §2.7.0 — Overview; CITE: §2.7.0.2 — Dependencies and Notation | TS-3, TS-5, TS-8 | §2.7 canonical prose → Chapter 10 scaffold | Outline finalized and ready for normative text injection; region params/associated bounds/examples remain TS items |

### Section 10.0 Overview
- Parametric features cover generics, where clauses, default parameters, and specialization hooks. CITE: §2.7.0.1 — Scope; CITE: §2.7.0.3 — Guarantees. ISSUE TS-3; ISSUE TS-8.
- Integration retains dependencies on permissions, effects, and alias transparency noted in §2.7.0.2. CITE: §2.7.0.2 — Dependencies and Notation. ISSUE TS-8.

#### Section 10.0.1 Scope and Goals
- Outline mirrors canonical goals: parametric types, generic procedures, and specialization boundaries. CITE: §2.7.0.1 — Scope. ISSUE TS-3.

#### Section 10.0.2 Dependencies and Notation
- Summaries capture metavariables and substitution notation reused across Part II. CITE: §2.7.0.2 — Dependencies and Notation. ISSUE TS-8.

#### Section 10.0.3 Guarantees
- Restate guarantees for monomorphization, zero-cost abstraction, and uniform parametric behaviour. CITE: §2.7.0.3 — Guarantees. ISSUE TS-3.

### Section 10.1 Syntax
- Syntax outline collects grammar for generic declarations, where clauses, and specialization keywords. CITE: §2.7.0.4 — Syntax Summary. ISSUE TS-5.

#### Section 10.1.1 Concrete Grammar
- Detail EBNF for generic parameters, const generics, where clauses, and default values. CITE: §2.7.2.1 — Concrete Syntax. ISSUE TS-5.

#### Section 10.1.2 Abstract Syntax
- Capture AST nodes for generic definitions and instantiations. CITE: §2.7.2.2 — Abstract Syntax. ISSUE TS-8.

#### Section 10.1.3 Derived Forms
- Summaries note shorthand for inference and default parameter applications. CITE: §2.7.2.3 — Basic Examples. ISSUE TS-5.

### Section 10.2 Static Semantics
- Static semantics highlight constraint solving, generalization, and specialization coherence. CITE: §2.7.0.5 — Static Semantics Summary. ISSUE TS-3; ISSUE TS-8.

#### Section 10.2.1 Typing Contexts and Judgments
- Outline references contexts tracking type parameters, const parameters, and bounds. CITE: §2.7.3.2 — Type Rules. ISSUE TS-3; ISSUE TS-8.

#### Section 10.2.2 Well-Formedness Rules
- Document requirements for parameter scopes and bound satisfaction; defer variance to §1.2.6. CITE: §2.7.3.1 — Well-Formedness Rules. ISSUE TS-8.

#### Section 10.2.3 Subtyping and Equivalence
- Summaries record equivalence modulo substitution and monomorphization behaviour. CITE: §2.7.3.3 — Type Properties. ISSUE TS-8.

#### Section 10.2.4 Permission-Indexed Types
- Note how permissions propagate through generic parameters and instantiated types. CITE: §2.7.0.3 — Guarantees. ISSUE TS-3.

#### Section 10.2.5 Effect-Annotated Map Types
 - Outline ensures generic functions propagate `uses` clauses through instantiations. CITE: §2.7.3.2 — Type Rules. ISSUE TS-5.
 - Effect Polymorphism note: cross‑reference §1.2.8 (ISSUE TS‑19) for ∀ε interactions with generics and constraints.
 - Effect notation: see Integration Correction preface and §1.2.5.

### Section 10.3 Dynamic Semantics
- Runtime summary emphasises monomorphized code generation and erased type parameters. CITE: §2.7.0.6 — Dynamic Semantics Summary. ISSUE TS-3.

#### Section 10.3.1 Runtime Obligations
- Highlight obligations for specialization selection and instantiation of const generics. CITE: §2.7.4 — Dynamic Semantics. ISSUE TS-3.

#### Section 10.3.2 Region Exit Semantics
- Note how region parameters (once specified) constrain generic instantiations. CITE: §2.7.0.2 — Dependencies and Notation. ISSUE TS-3.

#### Section 10.3.3 Unsafe Interaction
- Summaries mention unsafe hooks such as `unsafe` specialization blocks pending further detail. CITE: §2.7.0.10 — Notes and Issues. ISSUE TS-8.

### Section 10.4 Algorithms
- Algorithm section references generalization, constraint solving, and specialization ordering. CITE: §2.7.0.7 — Algorithms. ISSUE TS-3; ISSUE TS-8; ISSUE TS-9.

#### Section 10.4.1 Bidirectional Typing Procedure
- Outline describes how inference integrates with bidirectional typing for generics. CITE: §2.7.3.2 — Type Rules. ISSUE TS-3.

#### Section 10.4.2 Constraint Solving and Coherence
- Summaries capture constraint sets and coherence requirements for overlapping impls. CITE: §2.7.3.2 — Type Rules. ISSUE TS-8; ISSUE TS-9.

#### Section 10.4.3 Error Diagnostics
- Document diagnostic expectations for unsatisfied bounds, inference failure, and specialization conflicts. CITE: §2.7.0.10 — Notes and Issues. ISSUE TS-5; ISSUE TS-8.

### Section 10.5 Proven Properties
- Outline restates parametricity, principal types, and monomorphization soundness per canonical text. CITE: §2.7.0.8 — Proven Properties; CITE: §2.0.7 — Dynamic Semantics. ISSUE TS-3; ISSUE TS-8.

#### Section 10.5.1 Progress
- Document progress proofs for generic instantiations. CITE: §2.7.4 — Dynamic Semantics. ISSUE TS-3.

#### Section 10.5.2 Preservation
- Summaries highlight preservation under substitution and specialization. CITE: §2.7.3 — Static Semantics. ISSUE TS-8.

#### Section 10.5.3 Soundness Corollaries
- Note corollaries around uniform behaviour and zero-cost abstraction. CITE: §2.7.0.3 — Guarantees. ISSUE TS-3.

#### Section 10.5.4 Parametricity and Principal Types
- Outline references the canonical parametricity theorem and principal type guarantees. CITE: §2.7.0.8 — Proven Properties. ISSUE TS-3; ISSUE TS-8.

### Section 10.6 Interaction Summaries
- Interaction tables will capture permissions, effects, and contracts across generic instantiations. CITE: §2.7.0.9 — Integration Notes; CITE: §2.7.0.10 — Notes and Issues. ISSUE TS-5; ISSUE TS-8.

### Section 10.A Appendices (Non-Normative)
- Appendices will host diagrams, inference walkthroughs, and backlog coordination. CITE: §2.7.0.9 — Integration Notes; CITE: §2.7.0.10 — Notes and Issues. ISSUE TS-5; ISSUE TS-8.

#### Section 10.A.1 Taxonomy Diagram and Cross-Reference Table
- Plan diagrams linking generics to traits, map types, and aliases per the global taxonomy. CITE: §2.0.10 — Type Taxonomy; CITE: §2.7.0.1 — Scope. ISSUE TS-8.

#### Section 10.A.2 Extended Examples
- Outline example suites for generic APIs, specialization, and const generics pending the compendium. CITE: §2.7.0.10 — Notes and Issues. ISSUE TS-5; ISSUE TS-3.

#### Section 10.A.3 Specification Status & Backlog
- Keep TS-3 and TS-8 progress visible through shared backlog linkage. CITE: §2.0.11 — Specification Status; CITE: §2.0.13 — Outstanding Issues. ISSUE TS-3; ISSUE TS-8; ISSUE TS-5.


## Chapter 11: Map Types and Higher-Order Constructs

| Ready | Dependencies | Issues | Bridge | Notes |
| --- | --- | --- | --- | --- |
| [x] | CITE: §2.8.0 — Overview; CITE: §2.8.0.2 — Dependencies and Notation | TS-5, TS-9, TS-8 | §2.8 canonical prose → Chapter 11 scaffold | Outline finalized and ready for normative text injection; capture proofs/diagnostics/examples tracked via TS items |

### Section 11.0 Overview
- Map types provide first-class callable values with explicit effect signatures and capture semantics. CITE: §2.8.0.1 — Scope; CITE: §2.8.0.3 — Guarantees. ISSUE TS-9.
- Integration reiterates dependencies on permissions, effects, traits, and self-parameter sugar. CITE: §2.8.0.2 — Dependencies and Notation. ISSUE TS-8.

#### Section 11.0.1 Scope and Goals
- Outline mirrors canonical coverage: map types, closures, method sugar, and pipeline integration. CITE: §2.8.0.1 — Scope. ISSUE TS-9.

#### Section 11.0.2 Dependencies and Notation
- Summaries capture notation for map signatures, closure capture environments, and `$` parameters. CITE: §2.8.0.2 — Dependencies and Notation. ISSUE TS-9.

#### Section 11.0.3 Guarantees
- Restate guarantees for zero-cost higher-order programming, explicit effects, and capture safety. CITE: §2.8.0.3 — Guarantees. ISSUE TS-9.

### Section 11.1 Syntax
- Syntax outline documents map type grammar, closure expressions, and procedure declarations. CITE: §2.8.0.4 — Syntax Summary. ISSUE TS-5.

#### Section 11.1.1 Concrete Grammar
- Provide EBNF for map types, closures, and method declarations referencing canonical grammar. CITE: §2.8.2.1 — Concrete Syntax. ISSUE TS-5.

#### Section 11.1.2 Abstract Syntax
- Capture AST nodes for function values, closures, and method sugar expansions. CITE: §2.8.2.2 — Abstract Syntax. ISSUE TS-9.

#### Section 11.1.3 Derived Forms
- Summaries note pipeline sugar, partial application, and `$` receiver shorthand. CITE: §2.8.2.3 — Derived Forms. ISSUE TS-5.

### Section 11.2 Static Semantics
- Static semantics enumerate typing rules for function values, closure capture, and map composition. CITE: §2.8.0.5 — Static Semantics Summary. ISSUE TS-9; ISSUE TS-8.

#### Section 11.2.1 Typing Contexts and Judgments
- Outline references contexts for captured variables, effect annotations, and parameter permissions. CITE: §2.8.3.2 — Type Rules. ISSUE TS-9.

#### Section 11.2.2 Well-Formedness Rules
- Document requirements for capture eligibility, effect declarations, and result types. CITE: §2.8.3.1 — Well-Formedness Rules. ISSUE TS-9.

#### Section 11.2.3 Subtyping and Equivalence
- Summaries highlight map type equivalence and restrictions on effect variance; see §1.2.6 and §1.2.7. CITE: §2.8.3.3 — Type Properties. ISSUE TS-8.
 - Effect Polymorphism note: cross‑reference §1.2.8 (ISSUE TS‑19) for ∀ε interactions with higher‑order subtyping.

#### Section 11.2.4 Permission-Indexed Types
- Note how permissions flow through captured variables and method receivers. CITE: §2.8.4 — Permission Integration. ISSUE TS-9.

#### Section 11.2.5 Effect-Annotated Map Types
- Outline emphasises explicit `uses` clauses and propagation through higher-order calls. CITE: §2.8.3.2 — Type Rules. ISSUE TS-9.
 - Effect notation: see Integration Correction preface and §1.2.5.

### Section 11.3 Dynamic Semantics
- Runtime summary covers closure capture, invocation, and pipeline execution order. CITE: §2.8.0.6 — Dynamic Semantics Summary. ISSUE TS-9.

#### Section 11.3.1 Runtime Obligations
- Highlight obligations for capture lifetime, effect availability, and evaluation order. CITE: §2.8.6 — Dynamic Semantics. ISSUE TS-9.

#### Section 11.3.2 Region Exit Semantics
- Note restrictions on captured values and region-scoped closures. CITE: §2.8.6.3 — Region Semantics. ISSUE TS-2; ISSUE TS-9.

#### Section 11.3.3 Unsafe Interaction
- Summaries document unsafe operations (transmute of closures, raw function pointers) requiring explicit effects. CITE: §2.8.6.4 — Unsafe Interaction. ISSUE TS-9.

### Section 11.4 Algorithms
- Algorithm outline references pending closure capture analysis and effect checking. CITE: §2.8.0.7 — Algorithms. ISSUE TS-9.

#### Section 11.4.1 Bidirectional Typing Procedure
- Describe how closure expressions integrate with bidirectional typing and effect inference. CITE: §2.8.3.2 — Type Rules. ISSUE TS-9.

#### Section 11.4.2 Constraint Solving and Coherence
 - Summaries capture constraints for capture sets, effect inclusion, and specialization compatibility. CITE: §2.8.3.2 — Type Rules; CITE: §2.8.5.2 — Environment Capture. ISSUE TS-9; ISSUE TS-8.

#### Section 11.4.3 Error Diagnostics
- Document diagnostic expectations for capture violations, effect mismatches, and invalid pipelines. CITE: §2.8.0.10 — Notes and Issues. ISSUE TS-5; ISSUE TS-9.

### Section 11.5 Proven Properties
- Outline restates soundness obligations for higher-order functions referencing global theorems. CITE: §2.8.0.8 — Proven Properties; CITE: §2.0.7 — Dynamic Semantics. ISSUE TS-9.

#### Section 11.5.1 Progress
- Document progress results for function invocation and closure evaluation. CITE: §2.8.6 — Dynamic Semantics. ISSUE TS-9.

#### Section 11.5.2 Preservation
- Summaries highlight preservation of effects, capture environments, and result types. CITE: §2.8.3 — Static Semantics. ISSUE TS-9.

#### Section 11.5.3 Soundness Corollaries
- Note corollaries ensuring closure captures respect LPS invariants. CITE: §2.8.0.3 — Guarantees. ISSUE TS-9.

#### Section 11.5.4 Parametricity and Principal Types
- Outline references higher-order parametricity and inference results. CITE: §2.8.3.3 — Type Properties. ISSUE TS-8; ISSUE TS-9.

### Section 11.6 Interaction Summaries
- Interaction tables will summarise permissions, effects, and contracts for higher-order constructs. CITE: §2.8.0.9 — Integration Notes; CITE: §2.8.0.10 — Notes and Issues. ISSUE TS-9; ISSUE TS-5.

### Section 11.A Appendices (Non-Normative)
- Appendices will host capture diagrams, pipeline examples, and backlog integration items. CITE: §2.8.0.9 — Integration Notes; CITE: §2.8.0.10 — Notes and Issues. ISSUE TS-9; ISSUE TS-5.

#### Section 11.A.1 Taxonomy Diagram and Cross-Reference Table
- Plan diagrams linking map types to traits, generics, and the self type. CITE: §2.0.10 — Type Taxonomy; CITE: §2.8.0.1 — Scope. ISSUE TS-9.

#### Section 11.A.2 Extended Examples
- Outline example suites for closures, pipelines, and higher-order utilities pending the compendium. CITE: §2.8.0.10 — Notes and Issues. ISSUE TS-5; ISSUE TS-9.

#### Section 11.A.3 Specification Status & Backlog
- Keep TS-9 progress visible via shared backlog entries. CITE: §2.0.11 — Specification Status; CITE: §2.0.13 — Outstanding Issues. ISSUE TS-9; ISSUE TS-5.


## Chapter 12: Type Aliases and Utility Forms

| Ready | Dependencies | Issues | Bridge | Notes |
| --- | --- | --- | --- | --- |
| [x] | CITE: §2.9.0 — Overview; CITE: §2.9.0.2 — Dependencies and Notation | TS-5, TS-8 | §2.9 canonical prose → Chapter 12 scaffold | Outline finalized and ready for normative text injection; alias transparency/examples tracked via TS items |

### Section 12.0 Overview
- Type aliases provide transparent renamings and utility wrappers while preserving representation. CITE: §2.9.0.1 — Scope; CITE: §2.9.0.3 — Guarantees. ISSUE TS-5.
- Integration keeps aliases synchronized with permissions, generics, and substitution rules. CITE: §2.9.0.2 — Dependencies and Notation. ISSUE TS-8.

#### Section 12.0.1 Scope and Goals
- Outline mirrors canonical scope: transparent aliases, where clauses, and newtype promotion guidance. CITE: §2.9.0.1 — Scope. ISSUE TS-5.

#### Section 12.0.2 Dependencies and Notation
- Summaries restate substitution notation and its interactions with generics and permissions. CITE: §2.9.0.2 — Dependencies and Notation. ISSUE TS-8.

#### Section 12.0.3 Guarantees
- Restate equivalence guarantees and requirements for alias transparency. CITE: §2.9.0.3 — Guarantees. ISSUE TS-5.

### Section 12.1 Syntax
- Syntax outline documents alias declaration grammar and where clause usage. CITE: §2.9.0.4 — Syntax Summary. ISSUE TS-5.

#### Section 12.1.1 Concrete Grammar
- Detail EBNF for `type` aliases, generic parameters, and bounds. CITE: §2.9.2.1 — Concrete Syntax. ISSUE TS-5.

#### Section 12.1.2 Abstract Syntax
- Capture AST nodes for alias definitions and instantiations. CITE: §2.9.2.2 — Abstract Syntax. ISSUE TS-8.

#### Section 12.1.3 Derived Forms
- Summaries note shorthand (e.g., using alias in `use` statements) and default parameter behaviours. CITE: §2.9.2.3 — Derived Forms. ISSUE TS-5.

### Section 12.2 Static Semantics
- Static semantics outline equality, visibility, and substitution rules for aliases. CITE: §2.9.0.5 — Static Semantics Summary. ISSUE TS-8.

#### Section 12.2.1 Typing Contexts and Judgments
- Outline references contexts recording alias definitions for lookup during checking. CITE: §2.9.3.2 — Type Rules. ISSUE TS-8.

#### Section 12.2.2 Well-Formedness Rules
- Document conditions for alias declarations, including bound satisfaction. CITE: §2.9.3.1 — Well-Formedness Rules. ISSUE TS-8.

#### Section 12.2.3 Subtyping and Equivalence
- Summaries restate alias transparency and equivalence expansions. CITE: §2.9.3.3 — Type Properties. ISSUE TS-5.

#### Section 12.2.4 Permission-Indexed Types
- Note how permissions propagate through aliases and when newtypes are required instead. CITE: §2.9.0.3 — Guarantees. ISSUE TS-5.

#### Section 12.2.5 Effect-Annotated Map Types
 - Outline references aliasing of map types while maintaining effects. CITE: §2.9.3.2 — Type Rules. See §1.2.5 for effect notation. ISSUE TS-5.

### Section 12.3 Dynamic Semantics
- Runtime summary stresses that aliases erase to underlying representations. CITE: §2.9.0.6 — Dynamic Semantics Summary. ISSUE TS-5.

#### Section 12.3.1 Runtime Obligations
- Note there are no extra runtime checks; obligations mirror the underlying type. CITE: §2.9.0.6 — Dynamic Semantics Summary. ISSUE TS-5.

#### Section 12.3.2 Region Exit Semantics
- Alias behaviour at region exit matches the target type; outline this mapping. CITE: §2.9.0.6 — Dynamic Semantics Summary. ISSUE TS-5.

#### Section 12.3.3 Unsafe Interaction
- Summaries highlight that unsafe behaviour only arises from the aliased type. CITE: §2.9.0.6 — Dynamic Semantics Summary. ISSUE TS-5.

### Section 12.4 Algorithms
- Algorithm section references alias expansion in type checking and substitution. CITE: §2.9.0.7 — Algorithms. ISSUE TS-8.

#### Section 12.4.1 Bidirectional Typing Procedure
- Outline describes how alias expansion integrates into synthesis/check steps. CITE: §2.9.3.2 — Type Rules. ISSUE TS-8.

#### Section 12.4.2 Constraint Solving and Coherence
- Summaries capture constraint replacement and avoidance of infinite expansions. CITE: §2.9.3.2 — Type Rules. ISSUE TS-8.

#### Section 12.4.3 Error Diagnostics
- Document diagnostic needs for alias cycles, visibility, and misuse. CITE: §2.9.0.10 — Notes and Issues. ISSUE TS-5.

### Section 12.5 Proven Properties
- Outline restates alias transparency results and their link to equivalence theorems. CITE: §2.9.0.8 — Proven Properties; CITE: §2.0.6.2 — Type Equivalence. ISSUE TS-5.

#### Section 12.5.1 Progress
- Note progress properties are inherited from the target type; placeholder references canonical text. CITE: §2.9.0.6 — Dynamic Semantics Summary. ISSUE TS-5.

#### Section 12.5.2 Preservation
- Summaries emphasise preservation under alias expansion. CITE: §2.9.3 — Static Semantics. ISSUE TS-5.

#### Section 12.5.3 Soundness Corollaries
- Outline references alias soundness corollaries derived from equivalence rules. CITE: §2.9.0.3 — Guarantees. ISSUE TS-5.

#### Section 12.5.4 Parametricity and Principal Types
- Note alias interactions with generics and principal type derivation. CITE: §2.9.3.2 — Type Rules. ISSUE TS-8.

### Section 12.6 Interaction Summaries
- Interaction tables will map alias usage to permissions, effects, and contracts. CITE: §2.9.0.9 — Integration Notes; CITE: §2.9.0.10 — Notes and Issues. ISSUE TS-5.

### Section 12.A Appendices (Non-Normative)
- Appendices will host alias catalogues, migration guides, and backlog detail. CITE: §2.9.0.9 — Integration Notes; CITE: §2.9.0.10 — Notes and Issues. ISSUE TS-5.
 - Part‑level status: chapters link to a single consolidated Part II status/backlog appendix to avoid duplication. CITE: §2.0.11 — Specification Status.

#### Section 12.A.1 Taxonomy Diagram and Cross-Reference Table
- Plan diagrams linking aliases to generics, traits, and modules. CITE: §2.0.10 — Type Taxonomy; CITE: §2.9.0.1 — Scope. ISSUE TS-5.

#### Section 12.A.2 Extended Examples
- Outline example use cases for aliasing complex signatures pending the compendium. CITE: §2.9.0.10 — Notes and Issues. ISSUE TS-5.

#### Section 12.A.3 Specification Status & Backlog
- Keep alias-related updates visible alongside TS-5 and TS-8 in the central backlog. CITE: §2.0.11 — Specification Status; CITE: §2.0.13 — Outstanding Issues. ISSUE TS-5; ISSUE TS-8.


## Chapter 13: Self Type and Intrinsics

| Ready | Dependencies | Issues | Bridge | Notes |
| --- | --- | --- | --- | --- |
| [x] | CITE: §2.10.0 — Overview; CITE: §2.10.0.2 — Dependencies and Notation | TS-5, TS-7, TS-9 | §2.10 canonical prose → Chapter 13 scaffold | Outline finalized and ready for normative text injection; self-type diagnostics/intrinsics/examples tracked via TS items |

### Section 13.0 Overview
- The self type links methods to their implementing types, enabling `$` receivers, intrinsic access, and modal-state awareness. CITE: §2.10.0.1 — Scope; CITE: §2.10.0.3 — Guarantees. ISSUE TS-7.
- Integration references dependency on traits, map types, and permission systems per §2.10.0.2. CITE: §2.10.0.2 — Dependencies and Notation. ISSUE TS-9.

#### Section 13.0.1 Scope and Goals
- Outline mirrors canonical scope: self parameters, intrinsic invocation, and pipeline receivers. CITE: §2.10.0.1 — Scope. ISSUE TS-5.

#### Section 13.0.2 Dependencies and Notation
- Summaries repeat shared notation for `$`, implicit self, and trait integrations. CITE: §2.10.0.2 — Dependencies and Notation. ISSUE TS-7.

#### Section 13.0.3 Guarantees
- Restate guarantees around receiver permissions, intrinsic safety, and dispatch consistency. CITE: §2.10.0.3 — Guarantees. ISSUE TS-7.

### Section 13.1 Syntax
- Syntax outline documents self-parameter grammar, intrinsic references, and pipeline invocation forms. CITE: §2.10.0.4 — Syntax Summary. ISSUE TS-5.

#### Section 13.1.1 Concrete Grammar
- Provide EBNF for `$` receivers, implicit self usage, and intrinsic declarations. CITE: §2.10.2.1 — Concrete Syntax. ISSUE TS-5.

#### Section 13.1.2 Abstract Syntax
- Capture AST nodes for self-annotated methods and intrinsic calls. CITE: §2.10.2.2 — Abstract Syntax. ISSUE TS-7.

#### Section 13.1.3 Derived Forms
- Summaries note derived forms like pipeline `.|` usage and sugar for intrinsic calls. CITE: §2.10.2.3 — Derived Forms. ISSUE TS-5.

### Section 13.2 Static Semantics
- Static semantics highlight typing of self receivers, intrinsic contracts, and permission requirements. CITE: §2.10.0.5 — Static Semantics Summary. ISSUE TS-7; ISSUE TS-9.

#### Section 13.2.1 Typing Contexts and Judgments
- Outline references contexts tracking self permissions and trait/object associations. CITE: §2.10.3.2 — Type Rules. ISSUE TS-7.

#### Section 13.2.2 Well-Formedness Rules
- Document requirements for intrinsic declarations, receiver alignment, and permission compatibility. CITE: §2.10.3.1 — Well-Formedness Rules. ISSUE TS-7.

#### Section 13.2.3 Subtyping and Equivalence
- Summaries cover relationships between self type instantiations, trait objects, and monomorphized receivers. CITE: §2.10.3.3 — Type Properties. ISSUE TS-7.

#### Section 13.2.4 Permission-Indexed Types
- Note how self permissions (`own`, `mut`, `imm`) integrate with LPS and modal transitions. CITE: §2.10.0.3 — Guarantees; CITE: §2.10.3.2 — Type Rules. ISSUE TS-7.

#### Section 13.2.5 Effect-Annotated Map Types
- Outline effect propagation for methods and intrinsics using self receivers. CITE: §2.10.3.2 — Type Rules. ISSUE TS-9.

### Section 13.3 Dynamic Semantics
- Runtime summary covers method dispatch, intrinsic evaluation, and pipeline execution semantics. CITE: §2.10.0.6 — Dynamic Semantics Summary. ISSUE TS-7.

#### Section 13.3.1 Runtime Obligations
- Highlight obligations for permission consumption, state transitions, and intrinsic preconditions. CITE: §2.10.4.1 — Evaluation Rules. ISSUE TS-7.

#### Section 13.3.2 Region Exit Semantics
- Note region behaviour for self-owned resources and intrinsic side effects. CITE: §2.10.4.3 — Operational Behavior. ISSUE TS-2.

#### Section 13.3.3 Unsafe Interaction
- Summaries document unsafe intrinsic hooks and manual dispatch that require explicit effects. CITE: §2.10.4.4 — Unsafe Interaction. ISSUE TS-9.

### Section 13.4 Algorithms
- Algorithm outline references method dispatch resolution, intrinsic lookup, and pipeline lowering. CITE: §2.10.0.7 — Algorithms. ISSUE TS-7; ISSUE TS-9.

#### Section 13.4.1 Bidirectional Typing Procedure
- Describe how `$` receivers integrate with synth/check via implicit self binding. CITE: §2.10.3.2 — Type Rules. ISSUE TS-7.

#### Section 13.4.2 Constraint Solving and Coherence
- Summaries cover constraints ensuring self type coherence across generics and traits. CITE: §2.10.3.2 — Type Rules. ISSUE TS-7; ISSUE TS-8.

#### Section 13.4.3 Error Diagnostics
- Outline diagnostics for missing self bindings, intrinsic misuse, and permission mismatches. CITE: §2.10.0.10 — Notes and Issues. ISSUE TS-5.

### Section 13.5 Proven Properties
- Outline restates soundness requirements for self-aware dispatch and intrinsic calls. CITE: §2.10.0.8 — Proven Properties; CITE: §2.0.7 — Dynamic Semantics. ISSUE TS-7.

#### Section 13.5.1 Progress
- Document progress lemmas for self-method invocation and pipelines. CITE: §2.10.4 — Dynamic Semantics. ISSUE TS-7.

#### Section 13.5.2 Preservation
- Summaries highlight preservation of receiver permissions and state after method calls. CITE: §2.10.3 — Static Semantics. ISSUE TS-7.

#### Section 13.5.3 Soundness Corollaries
- Note corollaries ensuring dispatch coherence and safe intrinsic integration. CITE: §2.10.0.3 — Guarantees. ISSUE TS-7.

#### Section 13.5.4 Parametricity and Principal Types
- Outline references self-generic behaviour and principal method typings. CITE: §2.10.3.2 — Type Rules. ISSUE TS-8.

### Section 13.6 Interaction Summaries
- Interaction tables will compile permissions, effects, and contracts for self methods and intrinsics. CITE: §2.10.0.9 — Integration Notes; CITE: §2.10.0.10 — Notes and Issues. ISSUE TS-5; ISSUE TS-7.

### Section 13.A Appendices (Non-Normative)
- Appendices will cover self-type diagrams, intrinsic catalogues, and backlog updates. CITE: §2.10.0.9 — Integration Notes; CITE: §2.10.0.10 — Notes and Issues. ISSUE TS-5; ISSUE TS-7.

#### Section 13.A.1 Taxonomy Diagram and Cross-Reference Table
- Plan diagrams connecting self type usage to traits, map types, and modality. CITE: §2.0.10 — Type Taxonomy; CITE: §2.10.0.1 — Scope. ISSUE TS-7.

#### Section 13.A.2 Extended Examples
- Outline example suites for self receivers, pipelines, and intrinsic calls pending the compendium. CITE: §2.10.0.10 — Notes and Issues. ISSUE TS-5; ISSUE TS-7.

#### Section 13.A.3 Specification Status & Backlog
- Keep receiver/intrinsic backlog items surfaced alongside TS-series issues. CITE: §2.0.11 — Specification Status; CITE: §2.0.13 — Outstanding Issues. ISSUE TS-7; ISSUE TS-9; ISSUE TS-5.
