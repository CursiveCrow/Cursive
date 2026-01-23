# The Cursive Language Specification

**Part**: II - Type System  
**File**: 02_Type-System.md  
**Previous**: [Foundations](01_Foundations.md) | **Next**: [Declarations and Scope](03_Declarations-and-Scope.md)

---

## Abstract

This chapter specifies the Cursive type system, a static nominal type framework with parametric polymorphism, subtyping, and modal state types. The type system delivers memory safety, forbids undefined behavior, and preserves zero-cost abstractions through compile-time verification and monomorphization (compile-time template instantiation of generics).

**Definition 2.1 (Type System):** The Cursive type system comprises:

- The universe of types **T**
- The typing judgment `Γ ⊢ e : τ`
- The subtyping relation `<:`
- The well-formedness judgment `Γ ⊢ τ : Type`

**Conformance:** See Part I §1.5–1.7 for conformance criteria, RFC2119 keywords, and document conventions applied throughout this specification.

---

**Definition 2.2 (Type Foundations):** This section establishes the mathematical and syntactic foundations of the type system, including grammar, judgment forms, subtyping, equivalence, well-formedness, and integration with permissions, effects, contracts, and regions.

## 2.0 Type Foundations

### 2.0.1 Scope

This section specifies the core typing infrastructure shared by all subsequent sections. It defines type formation, well-formedness, subtyping, equivalence, and the interfaces between types, the Lexical Permission System (LPS), the effect system, contracts, and memory regions.

**Normative coverage:**

- Type grammar for all type constructors
- Typing judgment forms and contexts
- Subtyping relation and equivalence
- Well-formedness predicates
- Bidirectional type checking algorithm
- Constraint generation and solving
- Integration with permissions, effects, contracts, and regions

### 2.0.2 Design Goals

The type system is designed to achieve the following objectives:

1. **Safety:** A well-typed configuration MUST NOT trigger undefined behavior.
2. **Local reasoning:** All aliasing and mutation must be explicit via permissions, regions, or effects at the point of use.
3. **Performance:** Type analysis MUST complete at compile time with zero runtime overhead beyond explicitly requested constructs.
4. **Composability:** Product, sum, modal, and parametric types MUST compose without implicit coercions that disguise capability requirements.
5. **Auditability:** Every type rule MUST expose the permissions and effects it consumes so verification tools can reason mechanically about programs.

### 2.0.3 Cross-Part Dependencies

The type system integrates with other parts of the specification:

- **Part I (Foundations):** This chapter builds on the notation, metavariables, and judgment forms established in §1. CITE: §1.1 — Grammar Notation; §1.3 — Metavariables; §1.4 — Judgment Forms.

- **Part III (Declarations):** Variable bindings, type declarations, scope rules, and visibility modifiers are specified in Part III. This chapter references Part III for name introduction and scoping behavior.

- **Part XII (Memory, Permissions, and Concurrency):** Every typing judgment is parameterized by a permission context. §2.0.8.1 summarizes the invariants required by this chapter, and Part XII supplies the authoritative operational model for permissions and region lifetimes, including the constraints used when `Γ;Δ ⊢ e : τ`.

- **Part VI (Contracts and Effects):** Function and procedure types carry explicit effect signatures `uses ε`, and `must`/`will` clauses originate from the contract system defined in Part VI. §2.0.8.2–§2.0.8.3 restate the shared notations, including `@old(e)` for capturing initial values.

### 2.0.4 Notational Conventions

This chapter uses metavariables consistently with §1. CITE: §1.3 — Metavariables.

**Type and Expression Metavariables:**

```
τ, υ, ρ ∈ Type         (types)
α, β, γ ∈ TypeVar      (type variables)
κ ∈ Kind               (kinds)
e, e₁, e₂ ∈ Expr       (expressions)
v, v₁, v₂ ∈ Value      (values)
x, y, z ∈ Var          (term variables)
```

**Context and Environment Metavariables:**

```
Γ, Γ' ∈ Context        (type contexts: Var → Type)
Δ, Δ' ∈ RegionCtx      (region contexts: region stack for memory management)
Σ ∈ StateContext       (modal state contexts for state machine types)
σ, σ' ∈ Store          (memory stores: Location → Value)
```

**Modal and Effect Metavariables:**

```
@S, @S', @S₁ ∈ State   (modal states)
ε, ε₁, ε₂ ∈ Effect     (effect sets)
```

**Contract Metavariables:**

```
P, Q, R ∈ Assertion    (contract assertions)
{P} ... {Q}            (Hoare triple notation)
```

**Context Operations:**

- `Γ, x: τ` denotes context Γ extended with binding `x: τ`
- `σ[ℓ ↦ v]` denotes store σ updated so location ℓ maps to value v
- `Γ;Δ ⊢ e : τ` denotes typing judgment with region context Δ

**Configurations:**

Configurations are written `⟨e, σ⟩`. Rule names follow the pattern `[Rule-Name]` and reference constructors listed in §2.0.5.1.

### 2.0.5 Syntax

#### 2.0.5.1 Type Grammar

The complete EBNF for type syntax is consolidated in Appendix A.2. This section relies on those productions for all subsequent rules.

#### 2.0.5.2 Kind Grammar

Kind syntax is defined in Foundations §1.3; this chapter reuses that notation without restating it.

#### 2.0.5.3 Typing Contexts and Judgments

**Definition 2.5 (Typing Context):** A typing context Γ maps term variables to types and type variables to kinds with optional bounds.

```
Γ ::= ∅
    | Γ, x : τ
    | Γ, α : κ
    | Γ, α : (κ where α : Bound)
```

**Well-formedness of contexts:**

A context Γ is well-formed when:

1. Each term variable appears at most once in Γ
2. Each type variable appears at most once in Γ
3. All types in Γ are well-formed
4. All bounds in Γ are satisfied

**Shadowing:** See Part III §3.7 for complete shadowing rules and explicit shadowing semantics.

**Definition 2.6 (Typing Judgment):** The typing judgment `Γ ⊢ e : τ` denotes that expression `e` has type `τ` in context Γ.

**Auxiliary judgments:**

```
Γ ⊢ τ : Type                      (τ is a well-formed type)
Γ ⊢ τ₁ <: τ₂                      (τ₁ is a subtype of τ₂)
Γ ⊢ τ₁ ≡ τ₂                       (τ₁ and τ₂ are equivalent)
Γ ⊢ τ : Bound                     (τ satisfies trait bound)
Γ ⊢ e ! ε                         (e has effect ε)
Γ;Δ ⊢ e : τ                       (e has type τ with region context Δ)
⟨e, σ⟩ ⇓ ⟨v, σ'⟩                   (e evaluates to v)
{P} e {Q}                         (Hoare triple)
σ ⊨ P                             (store satisfies assertion P)
@S₁ →ₘ @S₂                        (modal state transition via procedure m)
```

CITE: §1.4 — Judgment Forms; §1.6 — Inference Rule Format.

#### 2.0.5.4 Effect Set Grammar and Lattice

Effect-set grammar and lattice laws are given once in Appendix A.9. This chapter references those definitions when constructing effect judgments.

### 2.0.6 Static Semantics

#### 2.0.6.1 Subtyping

**Definition 2.8 (Subtyping Relation):** The subtyping relation `<:` is the smallest preorder satisfying the rules below. No additional implicit coercions are permitted.

```
[Sub-Refl]
----------
τ <: τ
```

Reflexivity: Every type is a subtype of itself.

```
[Sub-Trans]
τ₁ <: τ₂    τ₂ <: τ₃
--------------------
τ₁ <: τ₃
```

Transitivity: Subtyping is transitive.

```
[Sub-Never]
----------
! <: τ
```

The never type `!` is a subtype of all types (bottom type).

```
[Sub-Array-Slice]
-----------------
[τ; n] <: [τ]
```

Fixed arrays coerce to slices.

```
[Sub-MapType]
υ₁ <: τ₁    τ₂ <: υ₂    ε₁ ⊆ ε₂
--------------------------------------------
(τ₁) → τ₂ ! ε₁  <:  (υ₁) → υ₂ ! ε₂
```

Function subtyping is contravariant in arguments, covariant in results, and covariant in effects (fewer effects is more specific).

**Generalization for n-ary functions:**

```
[Sub-MapType-N]
υ₁ <: τ₁    ...    υₙ <: τₙ    τᵣ <: υᵣ    ε₁ ⊆ ε₂
----------------------------------------------------------
(τ₁, ..., τₙ) → τᵣ ! ε₁  <:  (υ₁, ..., υₙ) → υᵣ ! ε₂
```

**Variance Table (§1.2.6):** All generic parameters are invariant unless explicitly stated otherwise in their defining section. The following constructors have fixed variance:

| Constructor            | Variance                  | Rationale                                          |
| ---------------------- | ------------------------- | -------------------------------------------------- |
| Arrays `[τ; n]`        | Invariant in `τ`          | LPS guarantees require exact type matching         |
| Slices `[τ]`           | Invariant in `τ`          | Mutation through slices requires type preservation |
| Tuples `(τ₁, ..., τₙ)` | Invariant in all `τᵢ`     | Structural equality requires exact matching        |
| Records                | Invariant in all fields   | Nominal types, no structural subtyping             |
| Enums                  | Invariant in all variants | Nominal types, discriminant safety                 |
| Modals                 | Invariant in all states   | State machine integrity                            |
| Map arguments          | Contravariant             | Function parameter positions                       |
| Map results            | Covariant                 | Function return positions                          |
| Effects in maps        | Covariant                 | Fewer effects = more specific                      |

CITE: §1.2.6 — Variance and Subtyping Side-Conditions.

**String State Coercion:**

```
[Sub-String-Owned-View]
------------------------------
string@Owned <: string@View
```

Owned strings coerce to views implicitly and at zero cost.

#### 2.0.6.2 Type Equivalence

**Definition 2.9 (Type Equivalence):** Types τ₁ and τ₂ are equivalent (`τ₁ ≡ τ₂`) when they denote the same runtime representation after expanding aliases, substituting generic arguments, and normalizing associated types.

```
[Equiv-Refl]
------------
τ ≡ τ
```

Reflexivity: Every type is equivalent to itself.

```
[Equiv-Sym]
τ₁ ≡ τ₂
------------
τ₂ ≡ τ₁
```

Symmetry: Equivalence is symmetric.

```
[Equiv-Trans]
τ₁ ≡ τ₂    τ₂ ≡ τ₃
--------------------
τ₁ ≡ τ₃
```

Transitivity: Equivalence is transitive.

```
[Equiv-Alias]
type A = τ
----------
A ≡ τ
```

Type aliases are equivalent to their definitions.

```
[Equiv-Generic-Alias]
type F<α> = τ[α]
----------------------
F<υ> ≡ τ[α ↦ υ]
```

Generic aliases are equivalent to their instantiations.

```
[Equiv-String-Default-Param]
------------------------------
string ≡ string@View    (in parameter positions)
```

In parameter positions, bare `string` is equivalent to `string@View`.

```
[Equiv-String-Default-Return]
------------------------------
string ≡ string@Owned   (in return type and field positions)
```

In return types and stored fields, bare `string` is equivalent to `string@Owned`.

**Nominal vs Structural Equivalence:**

- **Nominal types** (records, enums, modals, trait objects) MUST remain distinct even if their structure is identical. Two records with the same fields but different names are NOT equivalent.

- **Structural types** (tuples, arrays, primitives, slices) MAY be equated only when every component matches exactly.

**Examples:**

```cursive
// Nominal: NOT equivalent despite identical structure
record Point { x: f64, y: f64 }
record Vector { x: f64, y: f64 }
// Point ≢ Vector

// Structural: equivalent
(i32, bool) ≡ (i32, bool)
[u8; 10] ≡ [u8; 10]

// Alias: equivalent by expansion
type Distance = f64
Distance ≡ f64
```

#### 2.0.6.3 Well-Formedness

**Definition 2.10 (Well-Formed Type):** The judgment `Γ ⊢ τ : Type` holds when every constructor and parameter of τ is defined, arguments match arity, and all bounds are satisfied.

```
[WF-TypeVar]
(α : κ) ∈ Γ
--------------
Γ ⊢ α : Type
```

A type variable is well-formed if it appears in the context with kind `Type`.

```
[WF-Prim]
T ∈ PrimTypes
--------------
Γ ⊢ T : Type
```

All primitive types are well-formed in any context.

**Primitive types set:**

```
PrimTypes = {i8, i16, i32, i64, isize, u8, u16, u32, u64, usize,
             f32, f64, bool, char, string, !, ()}
```

```
[WF-Tuple]
Γ ⊢ τ₁ : Type    ...    Γ ⊢ τₙ : Type
n ≥ 2
-------------------------------------
Γ ⊢ (τ₁, ..., τₙ) : Type
```

Tuples are well-formed when all element types are well-formed and arity is at least 2.

```
[WF-Array]
Γ ⊢ τ : Type    n ∈ ℕ⁺
--------------------------
Γ ⊢ [τ; n] : Type
```

Arrays are well-formed when the element type is well-formed and length is a positive natural number.

```
[WF-Slice]
Γ ⊢ τ : Type
--------------
Γ ⊢ [τ] : Type
```

Slices are well-formed when the element type is well-formed.

```
[WF-MapType]
Γ ⊢ τ₁ : Type    ...    Γ ⊢ τₙ : Type
Γ ⊢ τᵣ : Type
ε well-formed
----------------------------------------------
Γ ⊢ (τ₁, ..., τₙ) → τᵣ ! ε : Type
```

Map types are well-formed when all parameter types, the result type, and the effect set are well-formed.

**Effect set well-formedness:**

```
[WF-Effect-Empty]
------------------
∅ well-formed

[WF-Effect-Single]
ε_name is a valid effect token
------------------------------
ε_name well-formed

[WF-Effect-Union]
ε₁ well-formed    ε₂ well-formed
------------------------------
ε₁ ∪ ε₂ well-formed
```

```
[WF-Permission]
Γ ⊢ τ : Type    p ∈ {own, mut, imm}
-----------------------------------
Γ ⊢ p τ : Type
```

Permission-wrapped types are well-formed when the underlying type is well-formed.

```
[WF-Modal]
Γ ⊢ ModalName : Type
@S is a valid state of ModalName
--------------------------------
Γ ⊢ ModalName@S : Type
```

Modal types are well-formed when the modal type exists and the state is valid.

**Additional well-formedness rules** for records, enums, generics, and other constructors appear in their dedicated sections (§2.2, §2.3, §2.9).

#### 2.0.6.4 Type Checking

**Definition 2.11 (Bidirectional Type Checking):** Cursive employs bidirectional type checking with two algorithmic judgments:

- `synth(Γ, e) = τ` (synthesis): Computes the type of expression e
- `check(Γ, e, τ)` (checking): Verifies expression e has type τ

**Checking algorithm:**

```
check(Γ, e, τ):
    let τ' = synth(Γ, e)
    if Γ ⊢ τ' <: τ then
        accept
    else
        reject with TypeError{expected = τ, found = τ'}
```

The checking judgment synthesizes a type for the expression and then verifies subtyping against the expected type.

**Synthesis algorithm:**

Synthesis rules are provided for each expression form in their respective sections. Key synthesis rules include:

- Variable lookup: `synth(Γ, x) = Γ(x)` when `(x : τ) ∈ Γ`
- Integer literals: `synth(Γ, n) = i32` (default type)
- Float literals: `synth(Γ, f) = f64` (default type)
- Application: `synth(Γ, f(e₁, ..., eₙ))` extracts result type from function type

**Literal type inference:**

Numeric literals have default types that may be overridden:

1. **Explicit suffix** (highest priority): `42u64 : u64`, `3.14f32 : f32`
2. **Context annotation**: `let x: u8 = 42` infers `42 : u8`
3. **Default type** (lowest priority): `42 : i32`, `3.14 : f64`

**Type checking properties:**

- **Decidability:** Type checking MUST terminate for all well-formed programs.
- **Soundness:** If `Γ ⊢ e : τ`, then evaluation of e produces a value of type τ (subject to progress and preservation theorems).

CITE: §2.0.7 — Dynamic Semantics.

#### 2.0.6.5 Type Inference

**Definition 2.12 (Constraint Set):** Type inference generates constraints that must be satisfied for a program to be well-typed.

```
C ::= τ₁ = τ₂                     (equality constraint)
    | τ₁ <: τ₂                    (subtyping constraint)
    | τ : Bound                   (trait bound constraint)
    | ∃α. C                       (existential constraint)
    | C₁ ∧ C₂                     (conjunction)
```

**Constraint Solving Algorithm:**

```
solve(∅) = ∅

solve({τ = τ} ∪ C) = solve(C)

solve({α = τ} ∪ C) = [α ↦ τ] ∪ solve(C[α ↦ τ])
    provided α ∉ FreeVars(τ)    (occurs check)

solve({T<τ₁> = T<τ₂>} ∪ C) = solve({τ₁ = τ₂} ∪ C)
    (decomposition for invariant constructors)

solve({τ : Bound} ∪ C) =
    if Γ ⊢ τ : Bound then
        solve(C)
    else
        error "trait bound not satisfied"
```

**Occurs-check failure:** If `α ∈ FreeVars(τ)` when attempting to solve `α = τ`, the system MUST report a static error indicating an infinite type.

**Example:**

```cursive
// Occurs-check failure
// let x = [x]  // ERROR: cannot construct infinite type [[[...]]]]
```

**Generalization:** Type variables are generalized to `∀α` only when:

1. `α` is not free in the context Γ
2. The enclosing permission context admits polymorphism
3. All constraints on `α` are satisfied

**Principal Types (§2.0.7):** For expressions without annotations, the type checker MUST infer the most general type (principal type) that satisfies all constraints.

#### 2.0.6.6 Numeric Promotion Policy (§1.2.10)

**Policy Statement (Global):** Cursive does NOT perform implicit numeric promotions between different types. All conversions between numeric types MUST be explicit.

**Normative requirement:** Chapters MUST NOT define ad-hoc promotion rules. All numeric conversions use explicit cast syntax.

**Examples:**

```cursive
let x: i32 = 42
let y: i64 = x          // ERROR: cannot assign i32 to i64
let z: i64 = x as i64   // OK: explicit cast

let a: f32 = 3.14f32
let b: f64 = a          // ERROR: cannot assign f32 to f64
let c: f64 = a as f64   // OK: explicit cast
```

CITE: §1.2.10 — Numeric Promotion Policy; §2.1 — Primitive Types.

### 2.0.7 Dynamic Semantics

**Definition 2.13 (Evaluation Judgment):** The judgment `⟨e, σ⟩ ⇓ ⟨v, σ'⟩` denotes that expression e evaluates to value v, transforming store σ to σ'.

**Evaluation properties:**

Cursive evaluation is call-by-value with strict left-to-right evaluation order (normative guarantee). All subexpressions, function arguments, and tuple/record construction evaluate left-to-right. The evaluation ORDER is deterministic for all programs; however, evaluation RESULTS are deterministic only for pure expressions without I/O or concurrency effects (I/O and concurrency may produce different results on repeated evaluation, even though the evaluation order remains consistent).

CITE: Part V §5.20 — Evaluation Order and Determinism (authoritative specification).

**Progress and Preservation:**

- **Theorem 2.1 (Progress):** If `∅ ⊢ e : τ`, then either e is a value or there exists `e'` and `σ'` such that `⟨e, σ⟩ → ⟨e', σ'⟩`.

- **Theorem 2.2 (Preservation):** If `Γ ⊢ e : τ` and `⟨e, σ⟩ → ⟨e', σ'⟩`, then `Γ ⊢ e' : τ`.

- **Corollary 2.3 (Type Safety):** Combining progress and preservation yields `∅ ⊢ v : τ` for every terminating evaluation of a well-typed closed expression e.

- **Theorem 2.4 (Parametricity):** Polymorphic functions behave uniformly with respect to their quantified type parameters. A function of type `∀α. τ` cannot inspect or violate the abstraction of α.

**Proof sketches** for these theorems appear in specialized sections where needed. The theorems apply globally across all type constructors.

CITE: §1.7 — Reading Type Rules: Complete Examples.

### 2.0.8 Inter-System Integration

#### 2.0.8.1 Permissions

**Definition 2.14 (Permission-Indexed Types):** Every type τ is viewed through a permission wrapper `perm τ` where `perm ∈ {own, mut, imm}`.

**Permission semantics:**

- **`own τ`** grants exclusive ownership and transferability. Moving the value consumes the previous binding. The owner is responsible for deallocation.

- **`mut τ`** grants mutable access without ownership transfer. The reference MUST remain within its lexical region. Multiple `mut` references to the same object are permitted; the programmer is responsible for ensuring safe usage.

- **`imm τ`** grants shared, read-only access. Mutation through `imm` is rejected unless mediated by a modal transition that explicitly consumes the permission.

**Permission casts:** Permission changes MUST be explicit through `move`, procedure calls consuming `mut` or `own` receivers, or constructor-specific operations. Implicit reborrowing is prohibited.

**Examples:**

```cursive
let x: own i32 = 42

// Immutable access (no ownership transfer)
procedure read_value(value: i32) { ... }
read_value(x)       // Pass as imm, x remains owned

// Mutable access (no ownership transfer)
procedure modify_value(value: mut i32) { ... }
modify_value(mut x) // Pass as mut, x remains owned

// Ownership transfer
procedure consume_value(value: own i32) { ... }
consume_value(move x) // Transfer ownership, x unusable after
```

**Default permission:** When no permission is specified, the default is `imm` (immutable reference).

**Integration with LPS:** The Lexical Permission System (Part XII) provides the full semantics of permission tracking, region constraints, and escape analysis. This section summarizes the type-level interface only.

CITE: Part XII — Memory, Permissions, and Concurrency.

#### 2.0.8.2 Effects

**Definition 2.15 (Effect Signatures):** Function and procedure types carry an effect signature describing side effects they may perform. A call site MUST guarantee that the calling context provides all required effects.

**Effect annotation forms:**

- **Type level:** `(T₁, ..., Tₙ) → U ! ε` where `ε` is an effect set
- **Declaration level:** `procedure name(...) uses ε { ... }` where `uses ε` declares required effects

**Effect checking rule:**

```
[Effect-Call]
Γ ⊢ f : (τ₁, ..., τₙ) → τᵣ ! ε_def
Γ ⊢ e₁ : τ₁    ...    Γ ⊢ eₙ : τₙ
ε_call ⊇ ε_def
-------------------------------------
Γ ⊢ f(e₁, ..., eₙ) : τᵣ ! ε_call
```

The calling context must provide a superset of the callee's required effects.

**Standard effect tokens (§1.2.9):**

Common effect categories include:

```
alloc.heap          Heap allocation
alloc.region        Region allocation
alloc.stack         Stack allocation
io.read             Input operations
io.write            Output operations
fs.read             File system read
fs.write            File system write
net.read            Network read
net.write           Network write
unsafe.ptr          Raw pointer operations
unsafe.union        Unsafe union access
unsafe.bitcopy      Bitwise copy operations
panic               May panic/abort
```

**Effect annotation:** The type-level form is `! ε`, with declaration-level `uses ε`.

CITE: §1.2.5 — Equivalence Law; §2.0.6.1 — Subtyping; §1.2.9 — Unsafe/Capability Effect Tokens.

**Effect Polymorphism (§1.2.8, TS-19):**

**Definition 2.52 (Effect Polymorphism):** Effect polymorphism is universal quantification over effect sets in map types.

**Effect Quantification:**

Effect variables may be universally quantified in map types:

Effect‑polymorphic function types use the form `∀ε` (optionally followed by `where ε ⊆ {…}`) before a `(...) → ... ! ε` signature; see Appendix A.2 for the precise production.

**Kinding:**

```
[K-Effect-Var]
---------------
ε : Eff

[K-Effect-Poly]
ε : Eff    τ : Type
----------------------
∀ε. τ : Type
```

Effect variables have kind `Eff`.

**Well-formedness:**

```
[WF-Effect-Poly]
Γ, ε : Eff ⊢ τ : Type
----------------------------
Γ ⊢ ∀ε. τ : Type

[WF-Effect-Poly-Bounded]
Γ, ε : Eff ⊢ τ : Type
ε_bound well-formed
--------------------------------
Γ ⊢ (∀ε where ε ⊆ ε_bound. τ) : Type
```

**Typing Rules:**

```
[T-Effect-Poly-Intro]
Γ, ε : Eff ⊢ e : τ
ε ∉ FreeEffects(Γ)
----------------------------
Γ ⊢ e : ∀ε. τ

[T-Effect-Poly-Instantiate]
Γ ⊢ e : ∀ε. τ
ε̂ well-formed
ε̂ satisfies bounds on ε
----------------------------
Γ ⊢ e : τ[ε ↦ ε̂]
```

**Subtyping with Effect Quantification:**

```
[Sub-Effect-Poly]
ε₁ ⊆ ε₂
------------------------------------------------
(T₁, ..., Tₙ) → U ! ε₁  <:  (T₁, ..., Tₙ) → U ! ε₂
```

Fewer effects is more specific (subtype).

```
[Sub-Effect-Poly-Quant]
Γ, ε : Eff ⊢ τ₁ <: τ₂
--------------------------------
Γ ⊢ (∀ε. τ₁) <: (∀ε. τ₂)
```

**Effect Bounds and Constraints:**

Subeffect bounds restrict which effects a polymorphic function may use:

```cursive
// Bounded effect polymorphism
procedure apply<T, ε>(f: (T) → T ! ε, x: T): T
    where ε ⊆ {alloc.heap, io.write}
    uses ε
{
    f(x)  // f may use at most alloc.heap and io.write
}
```

**Instantiation Algorithm:**

```
instantiate_effect_poly(∀ε. τ, call_context):
    // Step 1: Collect effect constraints from call site
    constraints := collect_effect_constraints(call_context)

    // Step 2: Find minimal effect set satisfying constraints
    ε̂ := solve_effect_constraints(constraints)

    // Step 3: Verify bounds satisfied
    if ε̂ ⊈ bounds_on_ε:
        error "effect bound violation"

    // Step 4: Verify calling context provides effects
    if ε_call ⊉ ε̂:
        error "insufficient effects in calling context"

    // Step 5: Instantiate
    return τ[ε ↦ ε̂]
```

**Examples:**

**Simple effect polymorphism:**

```cursive
// Generic map function, polymorphic over effects
procedure map<T, U, ε>(arr: [T], f: (T) → U ! ε): [U]
    uses alloc.heap, ε
{
    let result = Vec.new()
    loop item in arr {
        result.push(f(item))
    }
    result.into_array()
}

// Pure function (ε = ∅)
let doubled = map([1, 2, 3], |x| -> x * 2)
// map instantiated with ε = ∅

// Effectful function (ε = {io.write})
let logged = map([1, 2, 3], |x| -> {
    println(x)
    x * 2
})
// map instantiated with ε = {io.write}
```

**Effect bounds:**

```cursive
// Restrict allowed effects
procedure safe_transform<T, ε>(
    data: [T],
    transform: (T) → T ! ε
): [T]
    where ε ⊆ {alloc.heap}  // Only heap allocation allowed
    uses alloc.heap, ε
{
    map(data, transform)
}

// OK: pure function
safe_transform([1, 2, 3], |x| -> x + 1)

// ERROR: io.write not in bound
safe_transform([1, 2, 3], |x| -> {
    println(x)  // ERROR E2801: effect io.write not in bound {alloc.heap}
    x + 1
})
```

**Least-privilege effects:**

```cursive
// Automatically infer minimal effect set
procedure process<T, ε>(
    items: [T],
    action: (T) → () ! ε
)
    uses ε  // Propagate exactly the effects action needs
{
    loop item in items {
        action(item)
    }
}

// Infers ε = ∅ (pure)
process([1, 2, 3], |x| -> { })

// Infers ε = {io.write}
process([1, 2, 3], |x| -> println(x))
```

**Higher-order composition:**

```cursive
// Compose two effect-polymorphic functions
procedure compose<A, B, C, ε₁, ε₂>(
    f: (A) → B ! ε₁,
    g: (B) → C ! ε₂
): (A) → C ! (ε₁ ∪ ε₂)
{
    |x| -> g(f(x))
}

// Effect sets combine
let pure_f: (i32) → i32 ! ∅ = |x| -> x + 1
let effectful_g: (i32) → () ! io.write = |x| -> println(x)
let composed = compose(pure_f, effectful_g)
// Type: (i32) → () ! io.write
// Combined effects: ∅ ∪ {io.write} = {io.write}
```

**Correctness Properties:**

**Theorem 2.52 (Effect Polymorphism Soundness):**

For effect-polymorphic procedure:

```
Γ ⊢ p : ∀ε. ( … ) → τᵣ ! ε
```

Any instantiation with ε̂:

```
Γ ⊢ p : (...) → τᵣ ! ε̂
requires ε_call ⊇ ε̂
```

Effect requirements are preserved through instantiation.

**Theorem 2.53 (Effect Principal Types):**

For expression `e` using effects `ε`:

```
If Γ ⊢ e : τ ! ε, then ε is minimal:
∀ε'. (Γ ⊢ e : τ ! ε' ⟹ ε ⊆ ε')
```

Type inference finds the smallest effect set.

**Theorem 2.54 (Effect Substitution):**

```
Γ, ε : Eff ⊢ e : τ
ε̂ well-formed
---------------------------
Γ ⊢ e[ε ↦ ε̂] : τ[ε ↦ ε̂]
```

Effect substitution preserves typing.

**Integration with Variance (§1.2.6):**

Effect polymorphism interacts with map type variance:

- Effect position is covariant: fewer effects → more specific type
- Combined with contravariant arguments, covariant results
  CITE: §1.2.6 — Variance and Subtyping Side-Conditions; §1.2.7 — Effect Lattice Laws; §1.2.8 — Effect Polymorphism.

#### 2.0.8.3 Contracts

**Definition 2.16 (Sequent Clauses):** Sequent clauses attach behavioral specifications to function signatures, expressing the relationship between grant assumptions, preconditions, and postconditions in the form `[ε] ⊢ P ⇒ Q`.

**Contract specification:**

```
ContractClause ::= SequentClause
SequentClause  ::= "sequent" "{" SequentSpec "}"
SequentSpec    ::= "[" GrantSet "]" "|-" Antecedent "=>" Consequent
                | "[" GrantSet "]" "|-" Antecedent
                | "|-" Antecedent "=>" Consequent
                | Antecedent "=>" Consequent
```

**Sequent components:**

- **`[GrantSet]`:** Grant context (capability assumptions required)
- **`|-`:** Turnstile (entailment), ASCII form of `⊢`
- **`Antecedent`:** Preconditions (what must hold at call entry)
- **`=>`:** Implication, ASCII form of `⇒`
- **`Consequent`:** Postconditions (what will hold at return)

**Special constructs in sequent assertions:**

- **`result`:** Refers to the return value in consequents (postconditions)
- **`@old(expr)`:** Captures the value of expr at procedure entry for use in consequents

**Contract checking:** Sequent clauses are verified at function boundaries:

- Grant context `[ε]` is checked statically (compiler's responsibility)
- Antecedent (preconditions) is checked at call entry (caller's responsibility)
- Consequent (postconditions) is checked at return (callee's responsibility)
- The full sequent `[ε] ⊢ P ⇒ Q` expresses: "Under grants ε, precondition P implies postcondition Q"

**Contract violations** MUST raise a contract failure before any undefined behavior occurs. The failure mechanism (panic, exception, trap) is implementation-defined but MUST be documented.

**Example:**

```cursive
procedure transfer(from: mut Account, to: mut Account, amount: i64)
    sequent {
        [alloc.heap]
        |- amount > 0 && from.balance >= amount && from.id != to.id
        => from.balance == @old(from.balance) - amount
           && to.balance == @old(to.balance) + amount
    }
{
    from.balance -= amount
    to.balance += amount
}
```

**Integration with Part VII:** Part VII (Contracts and Effects) provides the complete semantics of sequent checking, verification strategies (static vs runtime), and assertion language. This section summarizes the type-level interface.

CITE: Part VII — Contracts and Effects.

#### 2.0.8.4 Regions

**Definition 2.17 (Memory Regions):** Region blocks `region r { ... }` establish lexical scopes for stack-disciplined allocation. Values created within a region MUST NOT escape past the closing brace.

**Region syntax:**

```cursive
region temp {
    let data = alloc_in<temp>(compute_data())
    process(data)
    // data automatically freed when region temp ends
}
```

**Region contexts:** The judgment form `Γ;Δ ⊢ e : τ` includes a region context Δ representing the stack of active regions.

```
Δ ::= ∅
    | Δ · r
```

**Non-escape rule:**

```
[Region-Escape]
Γ; (Δ · r) ⊢ v : τ
v contains an allocation from region r
--------------------------------------
Γ; Δ ⊬ v : τ         (reject: value escapes region r)
```

A value allocated in region r MUST NOT be returned or stored in a location that outlives r.

**Region closure rule:**

```
[Region-Close]
Γ; (Δ · r) ⊢ e : τ
region r closes
-----------------------------------
Γ; Δ ⊢ { close region r } : τ
all allocations in r are deallocated
```

When a region closes, all allocations within that region are deallocated in O(1) bulk operation (LIFO order).

**Deallocation properties:**

- **LIFO order:** Nested regions deallocate in reverse order of creation
- **O(1) bulk free:** Entire region freed at once (vs O(n) individual frees)
- **Deterministic:** Region closure is lexically scoped and deterministic

**Integration with Part XII:** Part XII (Memory, Permissions, and Concurrency) provides the complete semantics of region-based memory management, escape analysis, and lifetime constraints. This section summarizes the type-level interface.

CITE: Part XII — Memory, Permissions, and Concurrency; §1.2.4 — Region-Parameterized Judgments.

#### 2.0.8.5 Modal Types

**Definition 2.18 (Modal Types):** Modal types `ModalName@State` encode finite state machines as first-class types. Transitions consume a permission to one state and yield a permission to the successor state.

**Modal transition notation:**

```
@S₁ →ₘ @S₂
```

Denotes a valid state transition from state @S₁ to state @S₂ via procedure m.

**Linear transfer:** State transitions MUST explicitly consume the source state permission and produce the target state permission. The compiler verifies that:

1. All transitions are valid per the modal type's state graph
2. No state is used after transition
3. All states are eventually transitioned to terminal states or explicitly discarded

**Example:**

```cursive
modal File {
    @Closed { path: string }
    @Open { path: string, handle: FileHandle }

    @Closed -> open() -> @Open
    @Open -> read() -> @Open
    @Open -> close() -> @Closed
}

procedure use_file(path: string)
    uses io.read, fs.read
{
    let file: File@Closed = File.new(path)
    let opened: File@Open = file.open()
    // file is now unusable (consumed by open)
    let data = opened::read()
    let closed: File@Closed = opened.close()
    // opened is now unusable (consumed by close)
}
```

**Modal type semantics:** Modal states are erased after type checking. At runtime, the state machine exists only as the data fields; no additional tag or discriminant is maintained (zero-cost abstraction).

CITE: §2.3.3 — Modal Types; §2.0.6.1 — Subtyping.

### 2.0.9 Value Capabilities

#### 2.0.9.1 Copy Trait

**Definition 2.19 (Copy Trait):** The Copy trait indicates that a type's values can be duplicated via bitwise copy.

```cursive
trait Copy {
    procedure copy(self: Self): Self
        uses unsafe.bitcopy
        must self.is_copyable()
        will result.is_bitwise_clone_of(self)
}
```

**Automatic derivation:** A type τ automatically satisfies `Copy` when:

1. All fields of τ are `Copy`
2. τ declares no custom destructor
3. τ contains no heap-allocated or region-allocated components

**Primitive Copy types:**

All primitive types except `string@Owned` and `!` implement Copy:

```
Copy ∈ {i8, i16, i32, i64, isize, u8, u16, u32, u64, usize,
        f32, f64, bool, char, string@View, ()}
```

**Copy semantics:** Copy-capable types MAY be duplicated explicitly via a type-scope procedure (e.g., `::copy()`). Passing parameters does NOT automatically duplicate values; all types pass by permission (reference-like semantics).

**Example:**

```cursive
let x: i32 = 42
let y = x           // y binds to x's value, both usable (permission binding, not copy)
let z = x.copy()    // Explicit copy creates new value

procedure process(n: i32) {
    // n is passed by permission, no automatic copy
    let local = n.copy()  // Explicit copy when needed
}

process(x)  // x passed by permission, still usable after
```

CITE: §2.0.9.1 — Copy Trait; §2.1 — Primitive Types.

#### 2.0.9.2 Move Semantics

**Definition 2.20 (Move Operation):** All values support `move` to transfer ownership to a new binding or callee. After a move, the previous binding MUST NOT be used.

**Move syntax:**

```cursive
let x: own τ = value
let y: own τ = move x   // Transfer ownership
// x is now unusable
```

**Implicit moves:** Moves are implicit for `own` permissions when a binding is passed to a function parameter declared with `own`:

```cursive
procedure consume(value: own i32) { ... }

let x: own i32 = 42
consume(x)      // Implicit move (x declared as own parameter)
// x is now unusable
```

**Move checking:** The compiler MUST track moved values and reject any use after move:

```cursive
let x: own string = string.from("hello")
let y = move x
// let z = x      // ERROR E2001: use of moved value `x`
```

CITE: §2.0.8.1 — Permissions.

### 2.0.10 Type Taxonomy

The following table categorizes all type constructors covered in this chapter:

| Category              | Type Constructors                                                                   | Section    | Copy Capable                                |
| --------------------- | ----------------------------------------------------------------------------------- | ---------- | ------------------------------------------- |
| **Primitive Types**   | i8, i16, i32, i64, isize, u8, u16, u32, u64, usize, f32, f64, bool, char, string, ! | §2.1       | Most primitives (not string@Owned, !)       |
| **Product Types**     | Tuples `(T₁, ..., Tₙ)`, Records, Tuple-structs                                      | §2.2       | If all fields Copy                          |
| **Sum Types**         | Enums, Unsafe Unions                                                                | §2.3       | If all variants Copy                        |
| **Modal Types**       | State machines `Modal@State`                                                        | §2.3.3     | No (state transitions)                      |
| **Collection Types**  | Arrays `[T; n]`, Slices `[T]`                                                       | §2.4       | Arrays if T is Copy; slices are views       |
| **Reference Types**   | Safe pointers `Ptr<T>@State`, Raw pointers `*T`, `*mut T`                           | §2.5, §2.6 | Pointers themselves are Copy (not pointees) |
| **Abstraction Types** | Traits, Contract witnesses                                                          | §2.7, §2.13| Implementation-dependent                    |
| **Marker Traits**     | Send, Sync, Sized, Copy                                                             | §2.8       | N/A (zero-representation)                   |
| **Parametric Types**  | Generics `T<α>`, Associated types                                                   | §2.9       | Depends on instantiation                    |
| **Function Types**    | Map types `(...) → ... ! ε`, Closures                                            | §2.10      | No (closures capture environment)           |
| **Utility Types**     | Type aliases `type A = τ`                                                           | §2.11      | Depends on aliased type                     |
| **Intrinsic Types**   | Self, compiler intrinsics                                                           | §2.12      | Context-dependent                           |

### 2.0.11 Specification Status

All sections in this chapter (§2.0–§2.13) are complete and normative as of 2025‑10‑31. Algorithms, typing rules, and examples referenced by earlier drafts are now fully integrated into the body text; no outstanding TODO items remain.

### 2.0.12 Reading Guide

**Recommended reading order:**

1. **Start with §2.0 (Type Foundations):** Understand the type grammar, judgment forms, and core typing rules.

2. **Read §2.1 (Primitive Types):** Learn scalar value semantics, literal typing, and memory representations.

3. **Continue to §2.2 and §2.3:** Understand product types (tuples, records) and sum types (enums, modals) for data structure construction.

4. **Study §2.4 (Collections):** Arrays and slices for sequential data with bounds safety.

5. **Consult §2.5 and §2.6:** Pointer types for low-level memory access, distinguishing safe from unsafe pointers.

6. **Review §2.7 (Traits):** Code reuse through trait implementations, distinct from abstract contracts.

7. **Read §2.9 (Parametric Features):** Generic types and functions for polymorphic APIs.

8. **Study §2.10 (Map Types):** First-class functions, closures, and higher-order programming.

9. **Reference §2.11 and §2.12:** Type aliases and self types for API convenience.

10. **Study §2.13 (Contract Witnesses):** Runtime polymorphism for contracts with explicit allocation and effect tracking.

**Prerequisites:** Readers SHOULD be familiar with the notation and conventions established in §1 (Foundations). CITE: §1 — Foundations.

### 2.0.13 Change Tracking

There are currently no open Type System issues. Historical discussion items have been resolved and archived in `Spec/ISSUES.md` for reference.

---

**Definition 2.21 (Primitive Types):** Primitive types are built-in scalar and text types provided by the language that form the foundation of the type system. Primitive types cannot be defined by users and have predefined semantics, memory layouts, and operations.

## 2.1 Primitive Types

### 2.1.0 Overview

Section §2.1 specifies primitive scalar and text types: integers (§2.1.1), floats (§2.1.2), boolean (§2.1.3), character (§2.1.4), never type (§2.1.5), and strings (§2.1.6). These types integrate with the Lexical Permission System (Part IV), effect system (Part VII), and provide compile-time guarantees for memory safety, permission tracking, effect isolation, well-defined overflow semantics, and UTF-8 validity. CITE: §2.0.4, §1.3, Part IV, Part VII.

### 2.1.1 Integer Types

#### 2.1.1.1 Overview

Integer types provide fixed-width signed and unsigned integer values with explicit size and range guarantees.

#### 2.1.1.2 Syntax

**Type syntax:** Integer type tokens correspond to the `SignedInt` and `UnsignedInt` productions in Appendix A.2 (PrimitiveType).

**Literal syntax:** Integer literal forms (decimal, hexadecimal, octal, binary, and optional suffixes) are defined in Appendix A.1 under numeric literals.

Underscore separators (`_`) MAY appear between digits for readability but MUST NOT appear at the start, end, or adjacent to base prefixes.

**Abstract syntax:**

```
τ ::= i8 | i16 | i32 | i64 | isize | u8 | u16 | u32 | u64 | usize
v ::= n    (integer value)
```

#### 2.1.1.3 Static Semantics

**Well-formedness:**

```
[WF-Int-Type]
T ∈ {i8, i16, i32, i64, isize, u8, u16, u32, u64, usize}
--------------------------------------------------------
Γ ⊢ T : Type
```

All integer types are well-formed in any context.

**Typing rules for integer literals:**

```
[T-Int-Literal-Default]
n is an integer literal without type suffix
----------------------------------------
Γ ⊢ n : i32    (default type)
```

Unsuffixed integer literals default to `i32`.

```
[T-Int-Literal-Suffix]
n is an integer literal with suffix T
T ∈ IntTypes
-----------------------------------------
Γ ⊢ n : T    (explicit type from suffix)
```

A type suffix overrides the default.

```
[T-Int-Literal-Context]
n is an integer literal without type suffix
Γ ⊢ context requires type T
T ∈ IntTypes
n ∈ ⟦T⟧    (n fits in range of T)
-----------------------------------------
Γ ⊢ n : T    (type from context)
```

Context annotations override the default when the value fits in the target type.

**Canonical example:**

```cursive
let x = 42           // Type: i32 (default)
let y: u8 = 42       // Type: u8 (context override)
let z = 42u64        // Type: u64 (suffix override, highest priority)
let w = 0xFF_00      // Hexadecimal with underscore separator
```

Type inference priority: (1) Explicit suffix, (2) Context annotation, (3) Default (`i32`).

**Type properties:**

**Theorem 2.5 (Integer Value Sets):**

For each integer type T, the value set ⟦T⟧ is defined as:

| Type    | Value Set ⟦T⟧       | Min Value                  | Max Value                  | Size     | Align    |
| ------- | ------------------- | -------------------------- | -------------------------- | -------- | -------- |
| `i8`    | ℤ ∩ [-2⁷, 2⁷-1]     | -128                       | 127                        | 1 byte   | 1 byte   |
| `i16`   | ℤ ∩ [-2¹⁵, 2¹⁵-1]   | -32,768                    | 32,767                     | 2 bytes  | 2 bytes  |
| `i32`   | ℤ ∩ [-2³¹, 2³¹-1]   | -2,147,483,648             | 2,147,483,647              | 4 bytes  | 4 bytes  |
| `i64`   | ℤ ∩ [-2⁶³, 2⁶³-1]   | -9,223,372,036,854,775,808 | 9,223,372,036,854,775,807  | 8 bytes  | 8 bytes  |
| `isize` | ℤ ∩ [-2ⁿ⁻¹, 2ⁿ⁻¹-1] | Platform-dependent         | Platform-dependent         | Platform | Platform |
| `u8`    | ℕ ∩ [0, 2⁸-1]       | 0                          | 255                        | 1 byte   | 1 byte   |
| `u16`   | ℕ ∩ [0, 2¹⁶-1]      | 0                          | 65,535                     | 2 bytes  | 2 bytes  |
| `u32`   | ℕ ∩ [0, 2³²-1]      | 0                          | 4,294,967,295              | 4 bytes  | 4 bytes  |
| `u64`   | ℕ ∩ [0, 2⁶⁴-1]      | 0                          | 18,446,744,073,709,551,615 | 8 bytes  | 8 bytes  |
| `usize` | ℕ ∩ [0, 2ⁿ-1]       | 0                          | Platform-dependent         | Platform | Platform |

Where n = pointer width (32 or 64 bits depending on target architecture).

**Theorem 2.6 (Copy Capability):**

All integer types implement the Copy trait:

```
∀ T ∈ IntTypes. T : Copy
```

**Proof:** Integers have no heap-allocated components and are bitwise-copyable. Automatic derivation applies.

**Theorem 2.7 (Platform-Dependent Sizes):**

```
32-bit systems: isize = i32, usize = u32, n = 32
64-bit systems: isize = i64, usize = u64, n = 64
```

Code using `isize`/`usize` MAY have different behavior on different architectures.

#### 2.1.1.4 Dynamic Semantics

**Literal evaluation:**

```
[E-Int]
------------------------
⟨n, σ⟩ ⇓ ⟨n, σ⟩
```

Integer literals evaluate to themselves without state change.

**Arithmetic operations:**

Arithmetic evaluation rules (addition, subtraction, multiplication, division, remainder) are specified in Part IV.

CITE: Part IV §4.5.1 — Arithmetic Operators (complete dynamic semantics including overflow and division by zero behavior).

**Division by zero:**

Division by zero behavior is specified in Part IV §4.5.1.5.

**Overflow behavior:**

Integer overflow semantics (debug panic vs release wrap) are specified in Part IV §4.5.1.3.

CITE: Part IV §4.5.1.3 — Integer Arithmetic Evaluation; Part IV §4.5.1.6 — Overflow Control.

**Memory representation:** Integers use two's complement representation (signed) or unsigned binary with platform-native byte order.

### 2.1.2 Floating-Point Types

#### 2.1.2.1 Overview

Floating-point types conform to IEEE 754-2008 binary floating-point standard with standardized behavior for special values (±∞, NaN, signed zeros).

#### 2.1.2.2 Syntax

**Type syntax:** Floating-point type tokens (`f32`, `f64`) appear in Appendix A.2 under `PrimitiveType`.

**Literal syntax:** Floating-point literal forms (decimal, fractional, exponential, suffix) are defined in Appendix A.1.

**Canonical example:**

```cursive
let x = 3.14         // Type: f64 (default)
let y: f32 = 2.71    // Type: f32 (context override)
let z = 6.022e23f64  // Scientific notation with suffix
```

**Abstract syntax:**

```
τ ::= f32 | f64
v ::= f    (floating-point value, including ±0, ±∞, NaN)
```

#### 2.1.2.3 Static Semantics

**Well-formedness:**

```
[WF-Float]
T ∈ {f32, f64}
------------------
Γ ⊢ T : Type
```

**Typing rules:**

```
[T-Float-Literal-Default]
f is a floating-point literal without type suffix
-----------------------------------------------
Γ ⊢ f : f64    (default type)

[T-Float-Literal-Suffix]
f is a floating-point literal with suffix T
T ∈ {f32, f64}
------------------------------------------
Γ ⊢ f : T    (explicit type from suffix)
```

**Type properties:**

**Theorem 2.8 (IEEE 754 Conformance):** Cursive floating-point types conform to IEEE 754-2008 (`f32` = binary32, `f64` = binary64), including ±0, ±∞, NaN, normalized, and denormalized values. Both types implement `Copy`.

#### 2.1.2.4 Dynamic Semantics

**Literal evaluation:**

```
[E-Float]
------------------------
⟨f, σ⟩ ⇓ ⟨f, σ⟩
```

**Special value semantics:** Arithmetic with ∞, NaN, and ±0 follows IEEE 754-2008. Key properties: `NaN ≠ NaN`, comparisons with NaN return false, `+0.0 == -0.0`, division by zero produces ±∞.

### 2.1.3 Boolean Type

#### 2.1.3.1 Overview

The boolean type represents two-valued logic with short-circuit evaluation semantics for logical operators.

#### 2.1.3.2 Syntax

Boolean type and literal tokens (`bool`, `true`, `false`) are specified in Appendix A.2 and Appendix A.1 respectively.

**Abstract syntax:**

```
τ ::= bool

v ::= true | false
```

#### 2.1.3.3 Static Semantics

**Well-formedness:**

```
[WF-Bool]
------------------
Γ ⊢ bool : Type
```

**Typing rules:**

```
[T-Bool-True]
------------------
Γ ⊢ true : bool

[T-Bool-False]
------------------
Γ ⊢ false : bool
```

**Type properties:**

**Theorem 2.11 (Boolean Value Set):**

```
⟦bool⟧ = {true, false} ≅ {⊤, ⊥} ≅ ℤ₂
```

**Theorem 2.12 (Copy Capability):**

```
bool : Copy
```

**Theorem 2.13 (Size and Alignment):**

```
size(bool) = 1 byte
align(bool) = 1 byte
```

#### 2.1.3.4 Dynamic Semantics

**Literal evaluation:**

```
[E-Bool-True]
------------------------
⟨true, σ⟩ ⇓ ⟨true, σ⟩

[E-Bool-False]
------------------------
⟨false, σ⟩ ⇓ ⟨false, σ⟩
```

**Short-circuit evaluation:**

```
[E-And-True]
⟨e₁, σ⟩ ⇓ ⟨true, σ'⟩    ⟨e₂, σ'⟩ ⇓ ⟨v₂, σ''⟩
----------------------------------------------
⟨e₁ && e₂, σ⟩ ⇓ ⟨v₂, σ''⟩

[E-And-False]
⟨e₁, σ⟩ ⇓ ⟨false, σ'⟩
----------------------------------------------
⟨e₁ && e₂, σ⟩ ⇓ ⟨false, σ'⟩    (e₂ not evaluated)

[E-Or-True]
⟨e₁, σ⟩ ⇓ ⟨true, σ'⟩
----------------------------------------------
⟨e₁ || e₂, σ⟩ ⇓ ⟨true, σ'⟩    (e₂ not evaluated)

[E-Or-False]
⟨e₁, σ⟩ ⇓ ⟨false, σ'⟩    ⟨e₂, σ'⟩ ⇓ ⟨v₂, σ''⟩
----------------------------------------------
⟨e₁ || e₂, σ⟩ ⇓ ⟨v₂, σ''⟩
```

**Memory representation:**

```
bool memory layout (1 byte):
┌----┐
│0/1 │
└----┘

Values:
- false = 0x00 (all bits zero)
- true  = 0x01 (least significant bit set)
```

### 2.1.4 Character Type

#### 2.1.4.1 Overview

The character type represents Unicode scalar values from the valid Unicode codepoint range, excluding surrogate pairs.

#### 2.1.4.2 Syntax

Character type and literal syntax (including escape sequences) are specified in Appendix A.1 and Appendix A.2.

**Examples:**

```cursive
let letter: char = 'A'           // U+0041
let emoji: char = '🚀'            // U+1F680
let chinese: char = '中'          // U+4E2D
let tab: char = '\t'             // U+0009
let unicode: char = '\u{1F600}'  // U+1F600
```

**Abstract syntax:**

```
τ ::= char

v ::= 'c'    (character literal)
```

#### 2.1.4.3 Static Semantics

**Well-formedness:**

```
[WF-Char]
------------------
Γ ⊢ char : Type
```

**Typing rules:**

```
[T-Char]
c is a valid Unicode scalar value
c ∈ [U+0000, U+D7FF] ∪ [U+E000, U+10FFFF]
------------------
Γ ⊢ 'c' : char
```

Surrogate pairs [U+D800, U+DFFF] are invalid and MUST be rejected.

**Type properties:**

**Theorem 2.14 (Character Value Set):**

```
⟦char⟧ = Unicode Scalar Values = [U+0000, U+D7FF] ∪ [U+E000, U+10FFFF]

Total valid values: 1,112,064 codepoints
- U+0000 to U+D7FF: 55,296 codepoints
- U+E000 to U+10FFFF: 1,056,768 codepoints

Excluded (surrogate pairs): [U+D800, U+DFFF] = 2,048 invalid codepoints
```

**Theorem 2.15 (Size and Alignment):**

```
size(char) = 4 bytes
align(char) = 4 bytes
```

**Theorem 2.16 (Copy Capability):**

```
char : Copy
```

#### 2.1.4.4 Dynamic Semantics

**Literal evaluation:**

```
[E-Char]
------------------------
⟨'c', σ⟩ ⇓ ⟨'c', σ⟩
```

**Comparison:**

```
[E-Char-Comparison]
⟨c₁, σ⟩ ⇓ ⟨v₁, σ'⟩    ⟨c₂, σ'⟩ ⇓ ⟨v₂, σ''⟩
∀ ⊙ ∈ {<, ≤, >, ≥, ==, ≠}. codepoint(v₁) ⊙ codepoint(v₂) = result
------------------------------------------------------------------
⟨c₁ ⊙ c₂, σ⟩ ⇓ ⟨result, σ''⟩
```

Character comparison uses Unicode scalar value ordering.

**Memory representation:**

```
char memory layout (4 bytes, UTF-32):
┌---------------------┐
│ Unicode codepoint   │
└---------------------┘

Examples:
'A'  → 0x00000041
'€'  → 0x000020AC
'🚀' → 0x0001F680
```

### 2.1.5 Never Type

#### 2.1.5.1 Overview

The never type `!` is the uninhabited bottom type that represents computations which never return normally.

#### 2.1.5.2 Syntax

The never type token `!` is listed in Appendix A.2 (PrimitiveType).

**Usage examples:**

```cursive
function panic(message: string): !
    uses panic
{
    std.process.abort()
}

function exit(code: i32): !
    uses process.exit
{
    std.process.exit(code)
}

function infinite_loop(): ! {
    loop {
        // Never returns
    }
}
```

**Abstract syntax:**

```
τ ::= !

(no values: ⟦!⟧ = ∅)
```

#### 2.1.5.3 Static Semantics

**Well-formedness:**

```
[WF-Never]
------------------
Γ ⊢ ! : Type
```

**Typing rules:**

```
[T-Never-Coercion]
Γ ⊢ e : !
----------------  (for any type τ)
Γ ⊢ e : τ
```

The never type coerces to any type.

**Type properties:**

**Theorem 2.17 (Empty Value Set):**

```
⟦!⟧ = ∅
```

The never type is uninhabited.

**Theorem 2.18 (Bottom Type):**

```
∀ τ. ! <: τ
```

**Theorem 2.19 (Size and Alignment):**

```
size(!) = 0 bytes
align(!) = 1 byte
```

#### 2.1.5.4 Dynamic Semantics

**Divergence semantics:**

```
[E-Diverge]
⟨e, σ⟩ ⇓ ⊥    (diverges: never produces a value)
```

Where ⊥ indicates divergence through:

1. Panic (abnormal termination)
2. Exit (normal process termination)
3. Infinite loop (non-terminating computation)

### 2.1.6 String Types

#### 2.1.6.1 Overview

The `string` type is a modal type with two states:

- `string@Owned`: Owned, growable, heap-allocated UTF-8 buffer (ptr, len, cap)
- `string@View`: Read-only view over UTF-8 data (ptr, len), zero allocation

**Design Decision:** The language uses a single modal `string` type rather than separate types for owned and borrowed strings. This design provides ergonomic defaults while maintaining explicit control over allocation:

- Parameters written as `string` default to `string@View` (read-only, zero allocation)
- Return types and stored fields written as `string` default to `string@Owned` (owned, growable)
- Coercion `Owned → View` is implicit and zero-cost (safe narrowing)
- Coercion `View → Owned` requires allocation and MUST declare `uses alloc.heap` (explicit widening)

#### 2.1.6.2 Syntax

**Type syntax:** Modal string annotations (`string@Owned`, `string@View`) are defined in Appendix A.2 under `ModalType`.

**Default state semantics:**

When `string` appears without an explicit state annotation, the default state depends on the syntactic context:

##### 2.1.6.2.1 Context-Dependent Defaults

**Function signatures:**
- **Parameters**: `string` ≡ `string@View`
- **Return types**: `string` ≡ `string@Owned`

**Data structure storage:**
- **Record fields**: `string` ≡ `string@Owned`
- **Enum variant payloads**: `string` ≡ `string@Owned`
- **Array elements**: `string` ≡ `string@Owned`
- **Tuple components**: `string` ≡ `string@Owned`

**Rationale**: Storage locations must own their data for memory safety.

**Local variable bindings:**
- **String literals**: Always infer `string@View` from literal type
  ```cursive
  let x: string = "hello"  // x: string@View (inferred from literal)
  ```

- **Non-literal initialization**: Explicit state annotation required
  ```cursive
  let x: string@Owned = string.from("hi")  // OK: explicit
  let y: string = string.from("hi")         // ERROR E9F-2801: ambiguous state
  ```

**Error E9F-2801**: Ambiguous string state in local variable binding
When initializing a local variable with bare `string` type from a non-literal, explicitly write the intended state (`@View` or `@Owned`).

**Type aliases:**
Type aliases follow positional defaults:
```cursive
type Name = string
// Expands to string@View in parameters, string@Owned in fields/returns
```

**Generic type arguments:**
When `string` appears as a type argument, it inherits the default of the declaration site context:

```cursive
record Container<T> {
    value: T  // Declaration site: field context
}

let c: Container<string>  // Expands to Container<string@Owned> (field default)

function takes<T>(param: T) { }
takes<string>(...)  // Expands to takes<string@View> (parameter default)
```

**Collection types:**
Array, tuple, and collection element types inherit **owned** default:
```cursive
let names: [string; 10]     // Equivalent to [string@Owned; 10]
let pair: (i32, string)     // Equivalent to (i32, string@Owned)
let vec: Vec<string>        // Equivalent to Vec<string@Owned>
```

**Function types:**
Function types apply positional defaults:
```cursive
let f: (string) → string  // Equivalent to (string@View) → string@Owned
```

**String literal syntax:** Valid string literal forms and escape sequences are specified in Appendix A.1.

**Examples:**

```cursive
// String literals have type string@View
let literal: string = "hello"          // Defaults to string@View
let greeting: string@View = "world"

// Owned strings (heap-allocated)
let owned: string@Owned = string.from("hello")
let buffer: string@Owned = string.new()

// Implicit coercion Owned → View
procedure print(text: string)  // text: string@View
    uses io.write
{
    // Read-only access
}

let msg: string@Owned = string.from("data")
print(msg)  // OK: string@Owned <: string@View
```

**Abstract syntax:**

```
τ ::= string@Owned
    | string@View

Modal representation:
string@Owned = { ptr: *u8, len: usize, cap: usize }
string@View  = { ptr: *u8, len: usize }
```

#### 2.1.6.3 Static Semantics

**Well-formedness:**

```
[WF-String-Owned]
--------------------------
Γ ⊢ string@Owned : Type

[WF-String-View]
--------------------------
Γ ⊢ string@View : Type

[WF-String-UTF8-Invariant]
Γ ⊢ s : string@S
------------------------------
valid_utf8(s.ptr, s.len) = true
```

All string values MUST maintain UTF-8 validity at all times.

**Typing rules:**

```
[T-String-Literal]
lit is a string literal
valid_utf8(lit) = true
----------------------------
Γ ⊢ lit : string@View

[T-String-New]
------------------------------------
Γ ⊢ string.new() : string@Owned
    uses alloc.heap

[T-String-From]
Γ ⊢ s : string@View
------------------------------------
Γ ⊢ string.from(s) : string@Owned
    uses alloc.heap

[T-String-To-Owned]
Γ ⊢ s : string@View
------------------------------------
Γ ⊢ s.to_owned() : string@Owned
    uses alloc.heap
```

**Coercion rule:**

```
[Sub-String-Owned-View]
------------------------------
string@Owned <: string@View
```

Coercion from Owned to View is implicit and zero-cost (no allocation).

**Type properties:**

**Theorem 2.20 (String Owned is NOT Copy):**

```
string@Owned ∉ Copy
```

Owned strings contain heap-allocated data and MUST NOT be implicitly copied.

**Theorem 2.21 (String View is Copy):**

```
string@View : Copy
```

String views are dense pointers and are bitwise-copyable.

**Theorem 2.22 (UTF-8 Invariant Preservation):**

```
∀ s : string@S. valid_utf8(s)
∀ op ∈ StringOps. (valid_utf8(s) ∧ s' = op(s)) ⇒ valid_utf8(s')
```

All string operations preserve UTF-8 validity.

#### 2.1.6.4 Dynamic Semantics

**String literal evaluation:**

```
[E-String-Literal]
"text" stored at static address addr
len = byte_length("text")
-------------------------------------------------
⟨"text", σ⟩ ⇓ ⟨string@View{ptr: addr, len: len}, σ⟩
```

String literals evaluate to views of static read-only memory with zero allocation.

**Owned string allocation:**

```
[E-String-New]
alloc_heap(DEFAULT_CAP) = addr
-----------------------------------------------------------
⟨string.new(), σ⟩ ⇓ ⟨string@Owned{ptr: addr, len: 0, cap: DEFAULT_CAP}, σ'⟩
    where σ' = σ ∪ {addr ↦ [0u8; DEFAULT_CAP]}
    uses alloc.heap
```

**UTF-8 validation:**

```
validate_utf8(bytes: [u8]) → bool:
    // 1-byte (ASCII): 0xxxxxxx
    // 2-byte: 110xxxxx 10xxxxxx
    // 3-byte: 1110xxxx 10xxxxxx 10xxxxxx
    // 4-byte: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx

    reject overlong encodings
    reject surrogates [U+D800, U+DFFF]
    reject codepoints > U+10FFFF
    verify continuation bytes ∈ [0x80, 0xBF]

    return all sequences valid
```

**Complexity:** O(n) where n = byte length.

**Memory representation:**

```
string@Owned layout (24 bytes on 64-bit):
┌-----------------┬--------------┬--------------┐
│ ptr: *u8        │ len: usize   │ cap: usize   │
│ 8 bytes         │ 8 bytes      │ 8 bytes      │
└-----------------┴--------------┴--------------┘

string@View layout (16 bytes on 64-bit):
┌-----------------┬--------------┐
│ ptr: *u8        │ len: usize   │
│ 8 bytes         │ 8 bytes      │
└-----------------┴--------------┘

Field semantics:
• ptr: Pointer to UTF-8 encoded bytes
• len: Length in BYTES (not character count)
• cap: Capacity in BYTES (Owned only)
```

**UTF-8 encoding in memory:**

```
String "A©中🚀" memory layout:
┌----┬----┬----┬----┬----┬----┬----┬----┬----┬----┐
│0x41│0xC2│0xA9│0xE4│0xB8│0xAD│0xF0│0x9F│0x9A│0x80│
└----┴----┴----┴----┴----┴----┴----┴----┴----┴----┘
  A      ©           中              🚀
1 byte  2 bytes    3 bytes         4 bytes

Total: 10 bytes, 4 characters
len = 10 (byte length)
s.chars().count() = 4 (character count, requires iteration)
```

---

**Definition 2.23 (Product Types):** Product types combine multiple values into a single composite value. Product types include tuples (positional fields), records (named fields), and tuple-structs (nominal types with positional fields).

## 2.2 Product Types

### 2.2.0 Overview

#### 2.2.0.1 Scope

Section §2.2 specifies product types: tuples, records, and tuple-structs. Product types aggregate multiple values with:

- **Tuples:** Anonymous product types with positional access
- **Records:** Nominal product types with named fields and methods
- **Tuple-structs:** Nominal product types with positional fields

#### 2.2.0.2 Dependencies and Notation

Product types rely on:

- Permission wrappers (§2.0.8.1) for field access control
- Effect annotations (§2.0.8.2) for methods requiring side effects
- Pattern matching (Part IV) for destructuring

CITE: §2.0.8.1 — Permissions; §2.0.8.2 — Effects.

#### 2.2.0.3 Guarantees

Product types provide:

1. **Structural integrity:** Field access preserves type safety and permission invariants
2. **Nominal separation:** Records with identical structure but different names are distinct types
3. **Permission propagation:** Field permissions respect aggregate permissions

### 2.2.1 Tuples

#### 2.2.1.1 Overview

Tuples are anonymous product types with positional field access.

#### 2.2.1.2 Syntax

**Type syntax:** Tuple types follow the `TupleType` production in Appendix A.2.

**Tuple construction syntax:** Tuple expressions follow Appendix A.4 (`TupleExpr`).

**Tuple patterns:** Destructuring tuples uses the `TuplePattern` production in Appendix A.3.

**Examples:**

```cursive
// Tuple types
let pair: (i32, bool) = (42, true)
let triple: (f64, char, string) = (3.14, 'x', "text")

// Tuple construction
let point = (10.0, 20.0)  // Type: (f64, f64)

// Tuple destructuring
let (x, y) = point
match pair {
    (n, true) => process_true(n),
    (n, false) => process_false(n),
}
```

**Abstract syntax:**

```
τ ::= (τ₁, ..., τₙ)    where n ≥ 2

v ::= (v₁, ..., vₙ)
```

#### 2.2.1.3 Static Semantics

**Well-formedness:**

```
[WF-Tuple]
Γ ⊢ τ₁ : Type    ...    Γ ⊢ τₙ : Type
n ≥ 2
-------------------------------------
Γ ⊢ (τ₁, ..., τₙ) : Type
```

Tuples require at least 2 elements. Single-element tuples are disallowed.

**Typing rules:**

```
[T-Tuple-Ctor]
Γ ⊢ e₁ : τ₁    ...    Γ ⊢ eₙ : τₙ
n ≥ 2
-------------------------------------
Γ ⊢ (e₁, ..., eₙ) : (τ₁, ..., τₙ)
```

```
[T-Tuple-Projection]
Γ ⊢ e : (τ₁, ..., τₙ)
1 ≤ i ≤ n
-------------------------------------
Γ ⊢ e.i : τᵢ
```

Tuple projection uses zero-based indexing: `.0`, `.1`, etc.

**Type properties:**

**Theorem 2.23 (Tuple Invariance):**

Tuples are invariant in all element types:

```
(τ₁, ..., τₙ) <: (υ₁, ..., υₙ)  ⟺  τ₁ = υ₁ ∧ ... ∧ τₙ = υₙ
```

**Theorem 2.24 (Tuple Copy):**

A tuple is Copy if and only if all element types are Copy:

```
(τ₁, ..., τₙ) : Copy  ⟺  ∀i. τᵢ : Copy
```

**Theorem 2.25 (Tuple Size and Alignment):**

```
size((τ₁, ..., τₙ)) = aligned_sum(size(τ₁), ..., size(τₙ))
align((τ₁, ..., τₙ)) = max(align(τ₁), ..., align(τₙ))
```

Where `aligned_sum` accounts for padding between fields.

#### 2.2.1.4 Dynamic Semantics

**Tuple construction:**

```
[E-Tuple]
⟨e₁, σ⟩ ⇓ ⟨v₁, σ₁⟩    ...    ⟨eₙ, σₙ₋₁⟩ ⇓ ⟨vₙ, σₙ⟩
------------------------------------------------------
⟨(e₁, ..., eₙ), σ⟩ ⇓ ⟨(v₁, ..., vₙ), σₙ⟩
```

Tuple elements evaluate left-to-right.

**Tuple projection:**

```
[E-Tuple-Projection]
⟨e, σ⟩ ⇓ ⟨(v₁, ..., vₙ), σ'⟩
1 ≤ i ≤ n
--------------------------------------
⟨e.i, σ⟩ ⇓ ⟨vᵢ, σ'⟩
```

**Memory representation:**

```
(i32, bool, f64) layout (16 bytes on 64-bit):
┌----------┬----┬---┬------------┐
│ i32      │bool│pad│ f64        │
│ 4 bytes  │ 1  │ 3 │ 8 bytes    │
└----------┴----┴---┴------------┘
Offset: 0    4    5   8

Total size: 16 bytes (with padding)
Alignment: 8 bytes (from f64)

Padding rules:
• Each field aligned to its alignment requirement
• Trailing padding to match overall alignment
```

### 2.2.2 Records

#### 2.2.2.1 Overview

Records are nominal product types with named fields and methods.

#### 2.2.2.2 Syntax

**Record declaration:** The `RecordDecl` grammar (including `FieldDecl`, `MethodDecl`, and `GenericParams`) lives in Appendix A.4; visibility modifiers are detailed in Part III §3.10.

**Visibility modifiers:** See Part III §3.10 for complete visibility semantics (`public`, `internal`, `private`, `protected`).

**Note:** Full grammar for `Visibility`, field declarations, and procedure visibility specified in Part III (Declarations).

**Record construction:** Record literal syntax (`RecordExpr` and `FieldInit`) is specified in Appendix A.4.

**Examples:**

```cursive
// Record declaration
record Point {
    x: f64
    y: f64

    procedure distance(self: Point, other: Point): f64 {
        let dx = self.x - other.x
        let dy = self.y - other.y
        ((dx * dx) + (dy * dy)).sqrt()
    }
}

// Record construction
let p1 = Point { x: 10.0, y: 20.0 }
let x_val = 5.0
let p2 = Point { x: x_val, y: 15.0 }

// Field shorthand
let x = 3.0
let y = 4.0
let p3 = Point { x, y }  // Equivalent to Point { x: x, y: y }
```

**Abstract syntax:**

```
τ ::= RecordName
    | RecordName<τ₁, ..., τₙ>    (generic record)

v ::= RecordName { f₁: v₁, ..., fₙ: vₙ }
```

#### 2.2.2.3 Static Semantics

**Well-formedness:**

```
[WF-Record]
record RecordName { f₁: τ₁, ..., fₙ: τₙ } declared
Γ ⊢ τ₁ : Type    ...    Γ ⊢ τₙ : Type
field names f₁, ..., fₙ are distinct
--------------------------------------------------
Γ ⊢ RecordName : Type
```

Records are well-formed when all field types are well-formed and field names are unique.

**Typing rules:**

```
[T-Record-Ctor]
record R { f₁: τ₁, ..., fₙ: τₙ } declared
Γ ⊢ e₁ : τ₁    ...    Γ ⊢ eₙ : τₙ
--------------------------------------------
Γ ⊢ R { f₁: e₁, ..., fₙ: eₙ } : R
```

```
[T-Record-Field-Access]
Γ ⊢ e : R
record R { ..., fᵢ: τᵢ, ... } declared
field fᵢ is visible in current context
----------------------------------------
Γ ⊢ e.fᵢ : τᵢ
```

```
[T-Record-Procedure-Call]
Γ ⊢ e : R
R.m has signature (self: R, τ₁, ..., τₙ) → τᵣ ! ε
Γ ⊢ a₁ : τ₁    ...    Γ ⊢ aₙ : τₙ
--------------------------------------------------
Γ ⊢ e.m(a₁, ..., aₙ) : τᵣ ! ε
```

**Type properties:**

**Theorem 2.26 (Nominal Equivalence):**

Records are equivalent only if they have the same name:

```
RecordName₁ ≡ RecordName₂  ⟺  RecordName₁ = RecordName₂
```

Records with identical structure but different names are NOT equivalent.

**Theorem 2.27 (Record Copy):**

A record is Copy if and only if:

1. All fields are Copy
2. No custom destructor is declared

```
record R { f₁: τ₁, ..., fₙ: τₙ } : Copy  ⟺
    (∀i. τᵢ : Copy) ∧ (R has no custom destructor)
```

**Theorem 2.28 (Record Size and Alignment):**

Record size includes padding for alignment:

```
size(R) = aligned_sum(size(f₁), ..., size(fₙ)) + trailing_padding
align(R) = max(align(f₁), ..., align(fₙ))
```

#### 2.2.2.4 Dynamic Semantics

**Record construction:**

```
[E-Record-Ctor]
⟨e₁, σ⟩ ⇓ ⟨v₁, σ₁⟩    ...    ⟨eₙ, σₙ₋₁⟩ ⇓ ⟨vₙ, σₙ⟩
----------------------------------------------------------
⟨R { f₁: e₁, ..., fₙ: eₙ }, σ⟩ ⇓ ⟨R { f₁: v₁, ..., fₙ: vₙ }, σₙ⟩
```

Fields evaluate left-to-right in declaration order.

**Field access:**

```
[E-Record-Field-Access]
⟨e, σ⟩ ⇓ ⟨R { ..., fᵢ: v, ... }, σ'⟩
-------------------------------------------
⟨e.fᵢ, σ⟩ ⇓ ⟨v, σ'⟩
```

**Memory representation:**

```
record Point { x: f64, y: f64 } layout (16 bytes):
┌------------┬------------┐
│ x: f64     │ y: f64     │
│ 8 bytes    │ 8 bytes    │
└------------┴------------┘
Offset: 0         8

Total: 16 bytes, Alignment: 8 bytes
```

#### 2.2.2.5 FFI Representation Control

Records support representation attributes for FFI interoperability:

**[FFI-Repr-Table] FFI Representation Attributes:**

| Attribute               | Layout               | Padding    | Use Case             |
| ----------------------- | -------------------- | ---------- | -------------------- |
| `[[repr(C)]]`           | C-compatible         | C rules    | FFI with C code      |
| `[[repr(packed)]]`      | No padding           | None       | Byte-exact structs   |
| `[[repr(transparent)]]` | Same as single field | N/A        | Zero-cost wrappers   |
| `[[repr(align(n))]]`    | Custom alignment     | To n bytes | Cache line alignment |

**Examples:**

```cursive
// C-compatible layout
[[repr(C)]]
record CPoint {
    x: f32  // offset 0
    y: f32  // offset 4
}  // total size: 8, alignment: 4

// Packed layout (no padding)
[[repr(packed)]]
record Packed {
    a: u8   // offset 0
    b: u32  // offset 1 (no alignment padding!)
}  // total size: 5

// Transparent wrapper
[[repr(transparent)]]
record Meters(f64)  // Same layout as f64

// Custom alignment
[[repr(align(64))]]
record CacheLine {
    data: [u8; 64]
}  // Aligned to 64-byte boundary
```

CITE: §1.4.3 — Attributes; §2.2.2.6 — Record Representation Overrides.

### 2.2.3 Tuple-Structs

#### 2.2.3.1 Overview

Tuple-structs are nominal types with positional fields accessed via zero-based indexing.

#### 2.2.3.2 Syntax

**Tuple-struct declaration:** The `TupleStructDecl` production (including optional procedure bodies and field visibility) appears in Appendix A.4.

**Field visibility:** See Part III §3.10.3 for field-level visibility rules.

**Tuple-struct construction:** Tuple-struct literals follow the `TupleStructExpr` production in Appendix A.4.

**Examples:**

```cursive
// Tuple-struct declaration
record Color(u8, u8, u8)  // RGB color

// With visibility
record Point3D(public f64, public f64, public f64)

// With methods
record Velocity(f64, f64) {
    procedure magnitude(self: Velocity): f64 {
        let dx = self.0
        let dy = self.1
        ((dx * dx) + (dy * dy)).sqrt()
    }
}

// Construction
let red = Color(255, 0, 0)
let origin = Point3D(0.0, 0.0, 0.0)
let vel = Velocity(3.0, 4.0)
```

**Abstract syntax:**

```
τ ::= TupleStructName(τ₁, ..., τₙ)    where n ≥ 1

v ::= TupleStructName(v₁, ..., vₙ)
```

#### 2.2.3.3 Static Semantics

**Well-formedness:**

```
[WF-TupleStruct]
record N(τ₁, ..., τₙ) declared
Γ ⊢ τ₁ : Type    ...    Γ ⊢ τₙ : Type
n ≥ 1
---------------------------------------
Γ ⊢ N : Type
```

**Typing rules:**

```
[T-TupleStruct-Ctor]
record N(τ₁, ..., τₙ) declared
Γ ⊢ e₁ : τ₁    ...    Γ ⊢ eₙ : τₙ
------------------------------------
Γ ⊢ N(e₁, ..., eₙ) : N
```

```
[T-TupleStruct-Field-Access]
Γ ⊢ e : N
record N(τ₁, ..., τₙ) declared
0 ≤ i < n
field i is visible in current context
------------------------------------
Γ ⊢ e.i : τᵢ
```

Tuple-struct fields use zero-based indexing like tuples.

**Type properties:**

**Theorem 2.29 (Tuple-Struct Nominal):**

Tuple-structs are nominally typed:

```
N₁ ≡ N₂  ⟺  N₁ = N₂
```

Even if `N₁(τ₁, ..., τₙ)` and `N₂(τ₁, ..., τₙ)` have identical field types, they are distinct types.

**Theorem 2.30 (Tuple-Struct Copy):**

```
record N(τ₁, ..., τₙ) : Copy  ⟺  ∀i. τᵢ : Copy
```

#### 2.2.3.4 Dynamic Semantics

**Tuple-struct construction:**

```
[E-TupleStruct-Ctor]
⟨e₁, σ⟩ ⇓ ⟨v₁, σ₁⟩    ...    ⟨eₙ, σₙ₋₁⟩ ⇓ ⟨vₙ, σₙ⟩
----------------------------------------------------------
⟨N(e₁, ..., eₙ), σ⟩ ⇓ ⟨N(v₁, ..., vₙ), σₙ⟩
```

**Field access:**

```
[E-TupleStruct-Field]
⟨e, σ⟩ ⇓ ⟨N(v₁, ..., vₙ), σ'⟩
0 ≤ i < n
-----------------------------------
⟨e.i, σ⟩ ⇓ ⟨vᵢ, σ'⟩
```

**Memory representation:**

Tuple-structs have the same memory layout as tuples with equivalent field types:

```
record Color(u8, u8, u8) layout (3 bytes):
┌----┬----┬----┐
│ r  │ g  │ b  │
│ 1  │ 1  │ 1  │
└----┴----┴----┘

Total: 3 bytes, Alignment: 1 byte
```

---

**Definition 2.24 (Sum Types):** Sum types represent values that can be one of several variants. Sum types include enums (tagged unions), unsafe unions (untagged unions), and modal types (state machines).

## 2.3 Sum and Modal Types

### 2.3.0 Overview

#### 2.3.0.1 Scope

Section §2.3 specifies sum types and modal types:

- **Enums** (§2.3.1): Discriminated unions with type-safe variant access
- **Unsafe unions** (§2.3.2): Raw unions requiring explicit effect declarations
- **Modal types** (§2.3.3): State machines with compile-time transition verification

#### 2.3.0.2 Dependencies and Notation

Sum types rely on:

- Pattern matching exhaustiveness guarantees (§4.8.5)
- Permission tracking for variant construction and transition
- Effect annotations for unsafe union access

CITE: §2.0.8.1 — Permissions; §2.0.8.2 — Effects.

#### 2.3.0.3 Guarantees

Sum types provide:

1. **Discriminant safety:** Enum discriminants correctly identify active variants
2. **Exhaustiveness:** Pattern matches MUST handle all possible variants
3. **State integrity:** Modal transitions are statically verified

### 2.3.1 Enums

#### 2.3.1.1 Overview

Enums are discriminated unions with a runtime tag identifying the active variant.

#### 2.3.1.2 Syntax

**Enum declaration:** The `EnumDecl` and `EnumVariant` productions are defined in Appendix A.4.

**Pattern matching:** Enum pattern forms (`EnumPattern`) are specified in Appendix A.3.

**Examples:**

```cursive
// Standard library Option type
enum Option<T> {
    Some(T)
    None
}

// Standard library Result type
enum Result<T, E> {
    Ok(T)
    Err(E)
}

// Mixed variants
enum Message {
    Quit
    Move { x: i32, y: i32 }
    Write(string)
    ChangeColor(u8, u8, u8)
}

// Pattern matching
match result {
    Result::Ok(value) => process(value),
    Result::Err(error) => handle_error(error),
}

match msg {
    Message.Quit => shutdown(),
    Message.Move { x, y } => move_to(x, y),
    Message.Write(text) => display(text),
    Message.ChangeColor(r, g, b) => set_color(r, g, b),
}
```

**Abstract syntax:**

```
τ ::= EnumName
    | EnumName<τ₁, ..., τₙ>    (generic enum)

v ::= EnumName.VariantName
    | EnumName.VariantName(v₁, ..., vₙ)
    | EnumName.VariantName { f₁: v₁, ..., fₙ: vₙ }
```

#### 2.3.1.3 Static Semantics

**Well-formedness:**

```
[WF-Enum]
enum E { V₁: τ₁, ..., Vₙ: τₙ } declared
Γ ⊢ τ₁ : Type    ...    Γ ⊢ τₙ : Type
variant names V₁, ..., Vₙ are distinct
--------------------------------------
Γ ⊢ E : Type
```

**Typing rules:**

```
[T-Enum-Unit-Variant]
enum E { ..., V, ... } declared
------------------------------
Γ ⊢ E.V : E

[T-Enum-Tuple-Variant]
enum E { ..., V(τ₁, ..., τₙ), ... } declared
Γ ⊢ e₁ : τ₁    ...    Γ ⊢ eₙ : τₙ
----------------------------------------------
Γ ⊢ E.V(e₁, ..., eₙ) : E

[T-Enum-Struct-Variant]
enum E { ..., V { f₁: τ₁, ..., fₙ: τₙ }, ... } declared
Γ ⊢ e₁ : τ₁    ...    Γ ⊢ eₙ : τₙ
----------------------------------------------------------
Γ ⊢ E.V { f₁: e₁, ..., fₙ: eₙ } : E
```

**Pattern matching (exhaustiveness):**

```
[T-Match-Enum]
Γ ⊢ e : E
enum E { V₁, ..., Vₙ } declared
∀i. Γ, pᵢ ⊢ armᵢ : τᵣ
patterns {p₁, ..., pₘ} exhaustively cover {V₁, ..., Vₙ}
----------------------------------------------------------
Γ ⊢ match e { p₁ => arm₁, ..., pₘ => armₘ } : τᵣ
```

**Exhaustiveness requirement:** All variants MUST be covered. The compiler MUST reject non-exhaustive matches.

**Type properties:**

**Theorem 2.31 (Enum Copy):**

```
enum E { V₁: τ₁, ..., Vₙ: τₙ } : Copy  ⟺  ∀i. τᵢ : Copy
```

**Theorem 2.32 (Enum Discriminant):**

Every enum value carries a discriminant tag identifying its variant:

```
size(E) = sizeof(discriminant) + max(sizeof(V₁), ..., sizeof(Vₙ)) + padding
align(E) = max(align(discriminant), align(V₁), ..., align(Vₙ))
```

**Discriminant representation:**

```
discriminant : usize    (platform-dependent size)
value : {0, 1, ..., n-1}    (variant index)
```

#### 2.3.1.4 Dynamic Semantics

**Enum construction:**

```
[E-Enum-Variant]
⟨e₁, σ⟩ ⇓ ⟨v₁, σ₁⟩    ...    ⟨eₙ, σₙ₋₁⟩ ⇓ ⟨vₙ, σₙ⟩
variant V has index i in enum E
----------------------------------------------------------
⟨E.V(e₁, ..., eₙ), σ⟩ ⇓ ⟨{ discriminant: i, payload: (v₁, ..., vₙ) }, σₙ⟩
```

**Pattern matching:**

```
[E-Match-Enum]
⟨e, σ⟩ ⇓ ⟨{ discriminant: i, payload: data }, σ'⟩
pattern pᵢ matches variant with index i
⟨armᵢ[bindings from pᵢ], σ'⟩ ⇓ ⟨v, σ''⟩
----------------------------------------------
⟨match e { p₁ => arm₁, ..., pₙ => armₙ }, σ⟩ ⇓ ⟨v, σ''⟩
```

**Memory layout:**

```
enum Option<T> layout:
┌--------------┬---------------------┐
│ discriminant │ payload (variant)   │
│ (usize)      │ max(sizeof(Some(T)), │
│              │     sizeof(None))    │
└--------------┴---------------------┘

For Option<i32>:
- discriminant: 8 bytes (on 64-bit)
- payload: 4 bytes (i32 for Some, 0 for None)
- total: 16 bytes (with padding)
- alignment: 8 bytes
```

### 2.3.2 Unsafe Unions

#### 2.3.2.1 Overview

Unsafe unions are untagged unions requiring manual discriminant management and explicit `unsafe.union` effects.

#### 2.3.2.2 Syntax

Union declarations (`UnionDecl` and `UnionField`) are described in Appendix A.4.

**Examples:**

```cursive
union FloatBits {
    f: f32
    bits: u32
}

// Access requires unsafe effect
procedure inspect_float(value: f32): u32
    uses unsafe.union
{
    let u = FloatBits { f: value }
    u.bits  // Unsafe: reading inactive field
}
```

**Abstract syntax:**

```
τ ::= UnionName

v ::= UnionName { factive: v }
```

#### 2.3.2.3 Static Semantics

**Well-formedness:**

```
[WF-Union]
union U { f₁: τ₁, ..., fₙ: τₙ } declared
Γ ⊢ τ₁ : Type    ...    Γ ⊢ τₙ : Type
field names distinct
---------------------------------------
Γ ⊢ U : Type
```

**Typing rules:**

```
[T-Union-Init]
union U { ..., f: τ, ... } declared
Γ ⊢ e : τ
-----------------------------------
Γ ⊢ U { f: e } : U
    uses unsafe.union

[T-Union-Access]
Γ ⊢ e : U
union U { ..., f: τ, ... } declared
-----------------------------------
Γ ⊢ e.f : τ
    uses unsafe.union
```

All union operations require `unsafe.union` effect to mark manual safety responsibility.

**Type properties:**

**Theorem 2.33 (Union Size):**

```
size(union U { f₁: τ₁, ..., fₙ: τₙ }) = max(size(τ₁), ..., size(τₙ))
align(U) = max(align(τ₁), ..., align(τₙ))
```

**Theorem 2.34 (Union Copy):**

```
union U : Copy  ⟺  ∀i. τᵢ : Copy
```

#### 2.3.2.4 Dynamic Semantics

**Union operations** place the burden on the programmer:

- Reading an inactive field is undefined behavior
- Writing one field invalidates other fields
- No automatic discriminant tracking

```
[E-Union-Access-Unsafe]
⟨e, σ⟩ ⇓ ⟨U { factive: v }, σ'⟩
accessing field f where f ≠ factive
---------------------------------------
⟨e.f, σ⟩ has undefined behavior
    (programmer responsibility)
```

### 2.3.3 Modal Types

#### 2.3.3.1 Overview

Modal types encode finite state machines as first-class types with compile-time verified state transitions.

#### 2.3.3.2 Syntax

**Modal declaration:** Modal state machine syntax (`ModalDecl`, `StateDecl`, `TransitionDecl`) is provided in Appendix A.4.

**Modal type syntax:** Annotated modal types use the `ModalType` production in Appendix A.2.

**Examples:**

```cursive
modal File {
    @Closed { path: string }
    @Open { path: string, handle: FileHandle }

    @Closed → open() → @Open
    @Open → read() → @Open
    @Open → close() → @Closed
}

modal Connection {
    @Disconnected { url: string }
    @Connecting { url: string, timeout: Duration }
    @Connected { url: string, socket: Socket }
    @Failed { url: string, error: Error }

    @Disconnected → connect() → @Connecting
    @Connecting → success() → @Connected
    @Connecting → fail() → @Failed
    @Connected → disconnect() → @Disconnected
}
```

**Abstract syntax:**

```
τ ::= ModalName@StateName

v ::= ModalName@StateName { f₁: v₁, ..., fₙ: vₙ }
```

#### 2.3.3.3 Static Semantics

**Well-formedness:**

```
[WF-Modal]
modal M { @S₁ { ... }, ..., @Sₙ { ... } } declared
∀i. Γ ⊢ fields of @Sᵢ : Type
state names @S₁, ..., @Sₙ are distinct
------------------------------------------
Γ ⊢ M@Sᵢ : Type    (for each state Sᵢ)
```

**State graph well-formedness:**

```
[WF-Modal-Graph]
modal M with states S and transitions Θ
∀ (@S₁ →ₘ @S₂) ∈ Θ. @S₁, @S₂ ∈ S
graph (S, Θ) is well-formed
--------------------------------------
modal M has valid state graph
```

**Typing rules:**

```
[T-Modal-Transition]
Γ ⊢ e : M@S₁
(@S₁ →ₘ @S₂) ∈ transitions(M)
procedure m : (self: M@S₁, τ₁, ..., τₙ) → M@S₂ ! ε
Γ ⊢ a₁ : τ₁    ...    Γ ⊢ aₙ : τₙ
------------------------------------------------------
Γ ⊢ e.m(a₁, ..., aₙ) : M@S₂ ! ε
```

**State consumption:** The original binding of `e : M@S₁` MUST NOT be used after the transition.

**Type properties:**

**Theorem 2.35 (Modal State Erasure):**

At runtime, modal states are erased. Only the data fields remain; no additional tag or state discriminant is maintained.

```
runtime_repr(M@S) = fields of state @S
```

State safety verification occurs at compile time.

**Theorem 2.36 (Modal Non-Copy):**

Modal types do NOT implement Copy due to linear state consumption:

```
∀ M : Modal. M@S ∉ Copy
```

#### 2.3.3.4 Dynamic Semantics

**State transition:**

```
[E-Modal-Transition]
⟨e, σ⟩ ⇓ ⟨M@S₁ { data }, σ'⟩
⟨method_body[self ↦ data], σ'⟩ ⇓ ⟨new_data, σ''⟩
--------------------------------------------------
⟨e.m(...), σ⟩ ⇓ ⟨M@S₂ { new_data }, σ''⟩
```

**Memory representation:**

```
modal File layout:
@Closed state: { path: string@Owned }     (24 bytes)
@Open state:   { path: string@Owned,     (24 + 8 = 32 bytes)
                 handle: FileHandle }

Runtime representation uses max size:
size(File@S) = max(size(@Closed), size(@Open)) = 32 bytes
```

#### 2.3.3.5 Modal Verification

**Definition 2.33 (Modal Verification):** The modal verification algorithm ensures that modal type state machines are used correctly: all transitions are valid, states are reachable, and permissions/effects are properly tracked.

**Algorithm Specification:**

**Input:**

- Modal type `M` with state set `S = {@S₁, ..., @Sₙ}`
- Transition relation `Θ = {(@Sᵢ →ₘ @Sⱼ)}`
- Procedure implementations for each transition procedure

**Output:**

- `VERIFIED` if all checks pass
- `VIOLATION(state, transition, reason)` with specific error location

**Preconditions:**

- Modal M is well-formed (`Γ ⊢ M@Sᵢ : Type` for all states)
- All transition procedures have implementations

**Postconditions:**

- State graph is connected (all states reachable from initial state)
- Every transition has corresponding procedure
- Permissions consumed and produced correctly (linear)
- Effects declared for all state-changing operations
- No unreachable states exist

**Procedure:**

```
modal_verify(M, S, Θ, procedures):
    // Step 1: Verify graph connectivity
    reachable := compute_reachable_states(S, Θ)
    unreachable := S \ reachable

    if unreachable ≠ ∅:
        return VIOLATION(unreachable, -, "unreachable states")

    // Step 2: Verify each transition has implementation
    for each (@Sᵢ →ₘ @Sⱼ) in Θ:
        if procedure m not implemented:
            return VIOLATION(@Sᵢ, m, "missing transition procedure")

        // Step 3: Verify permission consumption
        if m signature is not (self: M@Sᵢ, ...) → M@Sⱼ:
            return VIOLATION(@Sᵢ, m, "incorrect state signature")

        // Step 4: Verify self is consumed (linear consumption)
        if m uses self after transition:
            return VIOLATION(@Sᵢ, m, "self used after state transition")

    // Step 5: Verify effect declarations
    for each transition procedure m:
        if m.effects_used ⊈ m.effects_declared:
            return VIOLATION(-, m, "undeclared effects")

    // Step 6: Verify no invalid transitions
    for each procedure m on M:
        if m attempts transition not in Θ:
            return VIOLATION(-, m, "invalid state transition")

    return VERIFIED

compute_reachable_states(S, Θ):
    // Use depth-first search from initial states
    initial := {s ∈ S | s has no incoming transitions}
    if initial = ∅:
        initial := {S₁}  // Default to first declared state

    visited := ∅
    stack := initial

    while stack ≠ ∅:
        current := pop(stack)
        visited := visited ∪ {current}

        for each (current →ₘ next) in Θ:
            if next ∉ visited:
                push(stack, next)

    return visited
```

**Complexity Analysis:**

- **Reachability:** O(|S| + |Θ|) depth-first search
- **Transition checking:** O(|Θ|) for each transition
- **Effect verification:** O(|Θ| × |ε|) where ε is effect set size
- **Total:** O(|S| + |Θ| × |ε|) polynomial in state graph size

**Proof Obligations:**

For each verified modal M, the following properties hold:

1. **State connectivity:** All states are reachable from initial state(s)
2. **Transition completeness:** Every declared transition has implementation
3. **Linear consumption:** Self parameter consumed exactly once per transition
4. **Effect soundness:** All side effects declared in `uses` clauses
5. **Type safety:** State transitions preserve type invariants

**Examples:**

**Valid modal (verified):**

```cursive
modal File {
    @Closed { path: string }
    @Open { path: string, handle: FileHandle }

    @Closed → open() → @Open
    @Open → read() → @Open
    @Open → close() → @Closed

    // Transition procedure implementations
    procedure open(self: File@Closed): File@Open
        uses fs.open
    {
        let handle = open_file(self.path)
        File@Open { path: self.path, handle: handle }
    }

    procedure read(self: File@Open): File@Open
        uses io.read
    {
        read_from_handle(self.handle)
        self  // Return same state
    }

    procedure close(self: File@Open): File@Closed
        uses fs.close
    {
        close_handle(self.handle)
        File@Closed { path: self.path }
    }
}
// VERIFIED: all transitions implemented, effects declared
```

**Invalid modal (violation):**

```cursive
modal BadFile {
    @Closed { path: string }
    @Open { path: string, handle: FileHandle }

    @Closed → open() → @Open
    // Missing: transition from @Open
}
// VIOLATION(@Open, -, "state has no outgoing transitions")
// @Open is a dead-end state
```

**Permission violation:**

```cursive
procedure bad_open(self: File@Closed): File@Open {
    let opened = File@Open { path: self.path, handle: h }
    self.path  // ERROR: self already consumed
    opened
}
// VIOLATION(@Closed, "bad_open", "self used after transition")
```

**Correctness Properties:**

**Theorem 2.47 (Modal State Integrity):**

For verified modal M with transitions Θ:

```
If Γ ⊢ e : M@S₁ and e.m() transitions to M@S₂,
then (@S₁ →ₘ @S₂) ∈ Θ
```

Only valid transitions can occur.

**Theorem 2.48 (Modal Linear Consumption):**

For transition procedure m:

```
procedure m(self: M@S₁, ...) → M@S₂
```

The self parameter is consumed exactly once:

```
{self : M@S₁}
body
{self invalid ∧ result : M@S₂}
```

No state duplication or reuse possible.

#### 2.3.3.6 Exhaustiveness Checking Algorithm

**Definition 2.32 (Exhaustiveness Checking):** The exhaustiveness checking algorithm verifies that pattern matches cover all possible enum variants, ensuring no runtime match failure can occur.

**Algorithm specification:**

The complete exhaustiveness checking algorithm, including complexity analysis, optimizations, and correctness properties, is specified in Part IV.

CITE: Part IV §4.8.5 — Exhaustiveness Checking (complete algorithm specification, complexity analysis O(|V| × |P|), decision tree optimizations, soundness theorems).

---

#### 2.3.4 Union Types

**Definition 2.37 (Type Unions):** The union type `τ₁ ∨ τ₂` denotes values that are either of type `τ₁` or of type `τ₂`. Unions arise naturally from control-flow joins—particularly modal transitions—and provide a lightweight notation without introducing bespoke enums for each join.

**Well-formedness:**

```
[WF-Union-Type]
Γ ⊢ τ₁ : Type    Γ ⊢ τ₂ : Type
------------------------------------
Γ ⊢ (τ₁ ∨ τ₂) : Type
```

Union formation is commutative and associative up to type equivalence:

```
τ₁ ∨ τ₂ ≡ τ₂ ∨ τ₁
(τ₁ ∨ τ₂) ∨ τ₃ ≡ τ₁ ∨ (τ₂ ∨ τ₃)
```

**Subtyping and introduction:**

```
[Sub-Union-Left]
τ₁ <: τ₁ ∨ τ₂

[Sub-Union-Right]
τ₂ <: τ₁ ∨ τ₂

[Sub-Union-Monotone]
τ₁ <: υ₁    τ₂ <: υ₂
------------------------------------
τ₁ ∨ τ₂ <: υ₁ ∨ υ₂

[T-Union-Intro-L]
Γ ⊢ e : τ₁
------------------------------------
Γ ⊢ e : τ₁ ∨ τ₂

[T-Union-Intro-R]
Γ ⊢ e : τ₂
------------------------------------
Γ ⊢ e : τ₁ ∨ τ₂
```

**Elimination:** Union values MUST be analysed via pattern matching. The canonical exhaustiveness checker (Part IV §4.8.5) applies unchanged: a match is exhaustive when every constituent type (or a wildcard arm) is covered.

**Relationship to enums:** A binary union is isomorphic to `enum { Left(τ₁), Right(τ₂) }`. The `∨` syntax is syntactic sugar adopted for modal control-flow joins; implementations MAY lower it to the corresponding nominal representation.

---

**Definition 2.25 (Collection Types):** Collection types provide sequential containers for homogeneous elements. Collection types include fixed-size arrays and dynamic slices.

## 2.4 Collections and Views

### 2.4.0 Overview

#### 2.4.0.1 Scope

Section §2.4 specifies collection types:

- **Arrays** (§2.4.1): Fixed-size sequences `[T; n]` with compile-time known length
- **Slices** (§2.4.2): Dynamic views `[T]` over contiguous memory (dense pointers)

#### 2.4.0.2 Dependencies and Notation

Collection types rely on:

- Bounds checking for safe indexing
- Permission tracking for mutable and immutable access
- Region constraints for view lifetimes

CITE: §2.0.8.1 — Permissions; §2.0.8.4 — Regions.

#### 2.4.0.3 Guarantees

Collection types provide:

1. **Bounds safety:** Index operations MUST verify bounds or prove safety statically
2. **Aliasing clarity:** Multiple mutable slices are permitted; programmer ensures safe usage
3. **Permission propagation:** Element access respects collection permissions

**Aliasing guarantee:** Mutable slices rely on the Lexical Permission System to make aliasing explicit; implementations MUST honour the permission rules in §2.0.8.1 and the slice semantics in §2.4.2 to maintain safety.

#### 2.4.0.4 Range Types

**Definition 2.41 (Range Families):** Cursive provides six built-in range type constructors used by range literals, slicing syntax, and iteration.

| Literal Form | Type Constructor          | Bound Semantics                                  |
|--------------|---------------------------|--------------------------------------------------|
| `a..b`       | `Range<T>`                | Inclusive start `a`, exclusive end `b`           |
| `a..=b`      | `RangeInclusive<T>`       | Inclusive start `a`, inclusive end `b`           |
| `a..`        | `RangeFrom<T>`            | Inclusive start `a`, unbounded end               |
| `..b`        | `RangeTo<T>`              | Unbounded start, exclusive end `b`               |
| `..=b`       | `RangeToInclusive<T>`     | Unbounded start, inclusive end `b`               |
| `..`         | `RangeFull<T>`            | Unbounded start and end                          |

**Well-formedness:**

```
[WF-Range]
Γ ⊢ T : Type
----------------------
Γ ⊢ Range<T> : Type

[WF-RangeInc]
Γ ⊢ T : Type
----------------------------
Γ ⊢ RangeInclusive<T> : Type

[WF-RangeFrom]
Γ ⊢ T : Type
----------------------------
Γ ⊢ RangeFrom<T> : Type

[WF-RangeTo]
Γ ⊢ T : Type
----------------------------
Γ ⊢ RangeTo<T> : Type

[WF-RangeToInc]
Γ ⊢ T : Type
----------------------------
Γ ⊢ RangeToInclusive<T> : Type

[WF-RangeFull]
Γ ⊢ T : Type
----------------------------
Γ ⊢ RangeFull<T> : Type
```

Ranges inherit the effect discipline and ordering constraints of their element type `T`. For numeric iteration `T` MUST be a member of `NumericTypes`; for slicing, `T` is typically `usize`.

**Representation:** Range types expose the fields listed above; implementations MUST provide these fields with explicit names so metaprogramming tools and documentation remain consistent. Optional bounds are represented using `Option<T>`, with `Option::None` indicating an omitted boundary.

| Type                  | Fields (record order)                          |
|-----------------------|-----------------------------------------------|
| `Range<T>`            | `start: Option<T>`, `end: Option<T>`, `inclusive: bool` (always `false`) |
| `RangeInclusive<T>`   | `start: T`, `end: T`                           |
| `RangeFrom<T>`        | `start: T`                                     |
| `RangeTo<T>`          | `end: T`                                       |
| `RangeToInclusive<T>` | `end: T`                                       |
| `RangeFull<T>`        | *(no fields)*                                  |

### 2.4.1 Arrays

#### 2.4.1.1 Overview

Arrays are fixed-size sequences with compile-time known length and runtime bounds checking.

#### 2.4.1.2 Syntax

**Type syntax:** Array types use the `ArrayType` production in Appendix A.2.

**Array literal syntax:**

Array literal forms (`[e1, …, en]` and repeat syntax `[e; n]`) are defined in Appendix A.4.

**Array indexing:**

Array indexing expressions follow the `ArrayIndex` production in Appendix A.4.

**Examples:**

```cursive
// Array types
let numbers: [i32; 5] = [1, 2, 3, 4, 5]
let matrix: [[f64; 3]; 3]  // 3x3 matrix

// Array literals
let zeros: [i32; 100] = [0; 100]  // Repeat syntax
let colors = [0xFF0000, 0x00FF00, 0x0000FF]  // Type: [u32; 3]

// Indexing
let first = numbers[0]    // Type: i32
let row = matrix[1]       // Type: [f64; 3]

// Bounds checked at runtime
let x = numbers[10]       // Panic: index out of bounds
```

**Abstract syntax:**

```
τ ::= [τ_elem; n]    where n ∈ ℕ⁺

v ::= [v₁, ..., vₙ]
```

#### 2.4.1.3 Static Semantics

**Well-formedness:**

```
[WF-Array]
Γ ⊢ τ : Type
n ∈ ℕ⁺
----------------------
Γ ⊢ [τ; n] : Type
```

Arrays require positive length.

**Typing rules:**

```
[T-Array-Literal]
Γ ⊢ e₁ : τ    ...    Γ ⊢ eₙ : τ
--------------------------------
Γ ⊢ [e₁, ..., eₙ] : [τ; n]

[T-Array-Repeat]
Γ ⊢ e : τ
τ : Copy
n ∈ ℕ⁺
--------------------------------
Γ ⊢ [e; n] : [τ; n]
```

Repeat syntax requires element type to be Copy.

```
[T-Array-Index]
Γ ⊢ arr : [τ; n]
Γ ⊢ idx : usize
--------------------------------
Γ ⊢ arr[idx] : τ
    (bounds checked at runtime)
```

**Type properties:**

**Theorem 2.37 (Array Invariance):**

Arrays are invariant in element type:

```
[τ; n] <: [υ; n]  ⟺  τ = υ
```

Different lengths produce incomparable types:

```
[τ; n] ≠ [τ; m]  when n ≠ m
```

**Theorem 2.38 (Array Copy):**

```
[τ; n] : Copy  ⟺  τ : Copy
```

**Theorem 2.39 (Array Size and Alignment):**

```
size([τ; n]) = n × size(τ)
align([τ; n]) = align(τ)
```

No padding between array elements.

#### 2.4.1.4 Dynamic Semantics

**Array construction:**

```
[E-Array-Literal]
⟨e₁, σ⟩ ⇓ ⟨v₁, σ₁⟩    ...    ⟨eₙ, σₙ₋₁⟩ ⇓ ⟨vₙ, σₙ⟩
------------------------------------------------------
⟨[e₁, ..., eₙ], σ⟩ ⇓ ⟨[v₁, ..., vₙ], σₙ⟩
```

```
[E-Array-Repeat]
⟨e, σ⟩ ⇓ ⟨v, σ'⟩
v : Copy
------------------------------------
⟨[e; n], σ⟩ ⇓ ⟨[v, v, ..., v], σ'⟩
                          (n copies)
```

**Array indexing:**

Array indexing evaluation semantics are specified in Part IV.

CITE: Part IV §4.3.5.3 — Array Indexing Evaluation (complete rules including bounds checking and out-of-bounds panics).

**Memory layout:**

```
[i32; 4] layout (16 bytes):
┌------┬------┬------┬------┐
│  0   │  1   │  2   │  3   │
│4 bytes│4 bytes│4 bytes│4 bytes│
└------┴------┴------┴------┘

Contiguous, no padding between elements
Total: 16 bytes, Alignment: 4 bytes
```

### 2.4.2 Slices

#### 2.4.2.1 Overview

Slices are dynamic views over contiguous sequences represented as dense pointers.

#### 2.4.2.2 Syntax

**Type syntax:** Slice types follow the `SliceType` production in Appendix A.2.

**Slicing syntax:** Slice expressions and range forms are defined in Appendix A.4 (`SliceExpr` and `RangeExpr`).

**Examples:**

```cursive
// Slicing arrays
let arr: [i32; 10] = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
let slice1: [i32] = arr[0..5]      // Elements 0-4
let slice2: [i32] = arr[3..=7]     // Elements 3-7 (inclusive)
let slice3: [i32] = arr[..3]       // Elements 0-2
let slice4: [i32] = arr[5..]       // Elements 5-9
let slice5: [i32] = arr[..]        // Full array as slice

// Slice parameters
procedure sum(values: [i32]): i32 {
    let total = 0
    loop i in 0..values.len() {
        total += values[i]
    }
    total
}

sum(arr[..])  // Pass full array
sum(arr[0..5])  // Pass subset
```

**Abstract syntax:**

```
τ ::= [τ_elem]

v ::= slice { ptr: *τ_elem, len: usize }
```

#### 2.4.2.3 Static Semantics

**Well-formedness:**

```
[WF-Slice]
Γ ⊢ τ : Type
------------------
Γ ⊢ [τ] : Type
```

**Unsized type constraint:** Slices are unsized (dynamically-sized types). Values of slice type MUST be accessed through references or pointers; direct by-value use is prohibited.

```
[WF-Slice-Reference-Only]
Γ ⊢ x : [τ]    (direct value binding)
------------------------------------
ERROR: slices are unsized, use &[τ] or pointer
```

**Typing rules:**

```
[T-Slice-From-Array]
Γ ⊢ arr : [τ; n]
--------------------------------
Γ ⊢ arr[..] : [τ]
    (coercion [τ; n] <: [τ])

[T-Slice-Range]
Γ ⊢ arr : [τ; n]
Γ ⊢ start : usize
Γ ⊢ end : usize
--------------------------------
Γ ⊢ arr[start..end] : [τ]
    (bounds checked at runtime)

[T-Slice-Index]
Γ ⊢ slice : [τ]
Γ ⊢ idx : usize
--------------------------------
Γ ⊢ slice[idx] : τ
    (bounds checked at runtime)
```

**Type properties:**

**Theorem 2.40 (Slice Invariance):**

```
[τ] <: [υ]  ⟺  τ = υ
```

**Theorem 2.41 (Slice is View, NOT Copy):**

```
[τ] ∉ Copy
```

Slices are dense pointers (ptr, len), but they reference external data. While the pointer itself could be copied, slices as types are not Copy.

**Theorem 2.42 (Slice Size and Alignment):**

```
size([τ]) = 2 × sizeof(usize) = 16 bytes (on 64-bit)
align([τ]) = alignof(usize) = 8 bytes (on 64-bit)
```

Slices are represented as dense pointers.

#### 2.4.2.4 Dynamic Semantics

**Slicing:**

Slicing evaluation semantics are specified in Part IV.

CITE: Part IV §4.3.6.3 — Slice Creation Evaluation (complete rules including bounds checking and range validation).

**Slice indexing:**

```
[E-Slice-Index]
⟨slice, σ⟩ ⇓ ⟨{ ptr: p, len: l }, σ'⟩
⟨idx, σ'⟩ ⇓ ⟨i, σ''⟩
0 ≤ i < l
------------------------------------------------
⟨slice[idx], σ⟩ ⇓ ⟨*(p + i), σ''⟩

[E-Slice-Index-OOB]
⟨slice, σ⟩ ⇓ ⟨{ ptr: p, len: l }, σ'⟩
⟨idx, σ'⟩ ⇓ ⟨i, σ''⟩
¬(0 ≤ i < l)
------------------------------------------------
⟨slice[idx], σ⟩ ⇓ panic("index out of bounds")
```

**Memory representation:**

```
Slice dense pointer layout (16 bytes on 64-bit):
┌-----------------┬--------------┐
│ ptr: *T         │ len: usize   │
│ 8 bytes         │ 8 bytes      │
└-----------------┴--------------┘

Slice points to external data (array or heap allocation):
┌----┬----┬----┬----┬----┐
│ T  │ T  │ T  │ T  │ T  │  ← data (not owned by slice)
└----┴----┴----┴----┴----┘
 ↑
 ptr (slice references this)
```

### 2.4.3 Coercions and Interactions

**[Coercions-Ch5] Collection Coercion Table:**

| From           | To            | Cost | Permission | Effect | Representation                          |
| -------------- | ------------- | ---- | ---------- | ------ | --------------------------------------- |
| `[τ; n]`       | `[τ]`         | O(1) | any        | none   | Dense pointer to array base             |
| `string@Owned` | `string@View` | O(1) | any        | none   | Extract (ptr, len) from (ptr, len, cap) |

**Examples:**

```cursive
let arr: [i32; 5] = [1, 2, 3, 4, 5]
let slice: [i32] = arr[..]  // Coercion: [i32; 5] <: [i32]

let owned: string@Owned = string.from("hello")
let view: string@View = owned  // Coercion: string@Owned <: string@View
```

---

**Definition 2.26 (Pointer Types):** Pointer types provide references to memory locations. Pointer types include safe pointers with capability tracking and raw pointers for FFI.

## 2.5 Safe Pointers

### 2.5.0 Overview

#### 2.5.0.1 Scope

Section §2.5 specifies safe pointer types `Ptr<T>@State` that integrate with the modal system for capability-based access control.

#### 2.5.0.2 Dependencies

Safe pointers rely on:

- Modal types (§2.3.3) for state tracking
- Permission system for access control
- Capability checking for dereferencing

CITE: §2.3.3 — Modal Types; §2.0.8.1 — Permissions.

### 2.5.1 Syntax

The `SafePtrType` grammar (including optional state annotations) is defined in Appendix A.2.

**Examples:**

```cursive
// Safe pointers with states
let ptr: Ptr<i32>@Valid
let null_ptr: Ptr<i32>@Null
let dangling: Ptr<i32>@Dangling

// State transitions
procedure dereference(ptr: Ptr<T>@Valid): T
    must ptr.is_valid()

// Capability checking
match ptr.check_state() {
    PtrState.Valid => use_ptr(ptr),
    PtrState.Null => handle_null(),
    PtrState.Dangling => panic("dangling pointer"),
}
```

**Abstract syntax:**

```
τ ::= Ptr<τ_inner>@S
```

### 2.5.2 Static Semantics

**Well-formedness:**

```
[WF-Safe-Ptr]
Γ ⊢ τ : Type
@S is a valid pointer state
----------------------------
Γ ⊢ Ptr<τ>@S : Type
```

**Typing rules:**

```
[T-Ptr-Deref]
Γ ⊢ ptr : Ptr<τ>@Valid
----------------------------
Γ ⊢ *ptr : τ

[T-Ptr-Deref-Invalid]
Γ ⊢ ptr : Ptr<τ>@S
S ∉ {Valid}
----------------------------
Γ ⊬ *ptr    (type error: cannot dereference non-valid pointer)
```

**[Caps-Ch6] Safe Pointer Capability Matrix:**

| Operation           | Permission | Effect | State Requirement | Safety Obligation       |
| ------------------- | ---------- | ------ | ----------------- | ----------------------- |
| Dereference `*ptr`  | any        | none   | @Valid            | Compiler verifies state |
| Address-of `&value` | own/mut    | none   | -                 | Creates @Valid pointer  |
| State check         | any        | none   | any               | Runtime check           |
| Null check          | any        | none   | any               | Runtime check           |

### 2.5.3 Dynamic Semantics

```
[E-Ptr-Deref]
⟨ptr, σ⟩ ⇓ ⟨Ptr<τ>@Valid { addr: ℓ }, σ'⟩
σ'(ℓ) = v
--------------------------------------------
⟨*ptr, σ⟩ ⇓ ⟨v, σ'⟩
```

---

## 2.6 Raw and Unsafe Pointers

### 2.6.0 Overview

#### 2.6.0.1 Scope

Section §2.6 specifies raw pointer types `*T` and `*mut T` for low-level memory access and FFI interoperability. Raw pointers provide minimal safety guarantees; the programmer is responsible for correctness.

### 2.6.1 Syntax

Raw pointer types (`*T`, `*mut T`) follow the `PointerType` production in Appendix A.2.

**Examples:**

```cursive
// Raw pointer types
let ptr: *i32       // Immutable raw pointer
let mut_ptr: *mut u8  // Mutable raw pointer

// Unsafe operations
procedure raw_dereference(ptr: *i32): i32
    uses unsafe.ptr
{
    *ptr  // Unsafe dereference
}

procedure raw_write(ptr: *mut i32, value: i32)
    uses unsafe.ptr
{
    *ptr = value  // Unsafe write
}
```

**Abstract syntax:**

```
τ ::= *τ_inner
    | *mut τ_inner

v ::= null
    | address(ℓ)
```

### 2.6.2 Static Semantics

**Well-formedness:**

```
[WF-Raw-Ptr]
Γ ⊢ τ : Type
------------------------
Γ ⊢ *τ : Type

[WF-Raw-Mut-Ptr]
Γ ⊢ τ : Type
------------------------
Γ ⊢ *mut τ : Type
```

**Typing rules:**

```
[T-Ptr-Deref-Raw]
Γ ⊢ ptr : *τ
------------------------
Γ ⊢ *ptr : τ
    uses unsafe.ptr

[T-Ptr-Write-Raw]
Γ ⊢ ptr : *mut τ
Γ ⊢ value : τ
------------------------
Γ ⊢ (*ptr = value) : ()
    uses unsafe.ptr

[T-Ptr-Null]
------------------------
Γ ⊢ null : *τ
    (for any type τ)
```

**Type properties:**

**Theorem 2.43 (Raw Pointer Copy):**

```
*τ : Copy
*mut τ : Copy
```

Raw pointers are bitwise-copyable addresses.

**Theorem 2.44 (Pointer Size):**

```
size(*τ) = sizeof(usize)
align(*τ) = alignof(usize)
```

**[Caps-Ch7] Unsafe Pointer Capability Matrix:**

| Operation                | Permission | Effect       | Safety | Programmer Responsibility                                            |
| ------------------------ | ---------- | ------------ | ------ | -------------------------------------------------------------------- |
| Dereference `*ptr`       | any        | `unsafe.ptr` | None   | Must ensure: valid address, proper alignment, initialized memory     |
| Write `*ptr = v`         | any        | `unsafe.ptr` | None   | Must ensure: valid address, proper alignment, no aliasing violations |
| Arithmetic `ptr + n`     | any        | `unsafe.ptr` | None   | Must ensure: result within allocation bounds                         |
| Cast `ptr as *U`         | any        | `unsafe.ptr` | None   | Must ensure: proper alignment, size compatibility                    |
| Null check `ptr == null` | any        | none         | Safe   | -                                                                    |

### 2.6.3 Dynamic Semantics

**Raw pointer dereference:**

```
[E-Raw-Ptr-Deref]
⟨ptr, σ⟩ ⇓ ⟨address(ℓ), σ'⟩
σ'(ℓ) defined
------------------------------------
⟨*ptr, σ⟩ ⇓ ⟨σ'(ℓ), σ'⟩
    uses unsafe.ptr

[E-Raw-Ptr-Deref-Invalid]
⟨ptr, σ⟩ ⇓ ⟨address(ℓ), σ'⟩
σ'(ℓ) undefined
------------------------------------
⟨*ptr, σ⟩ has undefined behavior
    (programmer responsibility)
```

**Programmer responsibilities:**

- Validity: Pointer must point to allocated, initialized memory
- Alignment: Pointer must be properly aligned for type τ
- Lifetime: Pointed-to memory must remain valid for duration of access
- Aliasing: Programmer must prevent unsafe aliasing patterns

---

**Definition 2.27 (Traits):** Traits provide concrete, reusable implementations that can be attached to types. Traits are distinct from contracts (abstract interfaces).

## 2.7 Traits and Code Reuse

### 2.7.0 Overview

#### 2.7.0.1 Scope

Section §2.7 specifies the trait system for concrete code reuse. **Critical distinction:** Traits provide concrete procedure implementations; contracts provide abstract behavioral specifications.

**Traits vs Contracts:**

- **Traits:** Contain concrete procedure bodies for code reuse
- **Contracts:** Contain only signatures with clauses (NO implementations)

CITE: Part VI — Contracts and Effects.

### 2.7.1 Syntax

**Trait declaration:** Trait syntax (`TraitDecl`, `TraitBody`, `ProcedureDefn`) is defined in Appendix A.4.

**Examples:**

```cursive
// Trait with concrete implementations
trait DefaultRenderable {
    procedure render(self: $) {
        println("Rendering default")
    }

    procedure render_with_color(self: $, color: Color) {
        self.set_color(color)
        self.render()
    }
}

// Trait usage: attached to types via trait composition or mixin patterns
// (Specific attachment mechanism is defined in Part XI - Modules, Packages and Imports)
```

**Critical rule:** ALL procedures in traits MUST have concrete bodies. Traits without bodies are contracts, not traits.

**Contract implementation (for comparison):**

```cursive
// Contract: abstract interface
contract Drawable {
    procedure draw(self)
        uses io.write
        // NO body - abstract
}

// Record implements contract directly in declaration
record Shape: Drawable {
    kind: ShapeKind
    color: Color

    // Implement required procedures
    procedure draw(self: Shape)
        uses io.write
    {
        draw_shape(self.kind, self.color)
    }
}
```

**Abstract syntax:**

```
τ ::= trait TraitName
    | trait TraitName<τ₁, ..., τₙ>
```

### 2.7.2 Static Semantics

**Well-formedness:**

```
[WF-Trait]
trait T { p₁, ..., pₙ } declared
∀i. pᵢ has concrete body
∀i. Γ ⊢ signature(pᵢ) : Type
-----------------------------------
Γ ⊢ trait T : Type
```

**Trait vs Contract Distinction (Normative):**

```
Contract: ONLY signatures + clauses (NO bodies)
contract Drawable {
    procedure draw($)
        uses io.write    // Clause only, NO body
}

Trait: ALL procedures have bodies
trait DefaultDrawable {
    procedure draw($) {
        println("default")  // MUST have body
    }
}
```

Attempting to declare a trait without procedure bodies MUST result in error E2701.

**Typing rules:**

```
[T-Trait-Procedure-Call]
Γ ⊢ e : τ
τ : TraitName    (τ implements TraitName)
TraitName.m has signature (self: $, τ₁, ..., τₙ) → τᵣ ! ε
Γ ⊢ a₁ : τ₁    ...    Γ ⊢ aₙ : τₙ
----------------------------------------------------------
Γ ⊢ e.m(a₁, ..., aₙ) : τᵣ ! ε
```

**Receiver notation:** The `$` symbol denotes `self: <implementing-type>` and is expanded during type checking.

### 2.7.3 Coherence and Object Safety

**Property 2.1 (Coherence):**

For any type τ and contract C, there MUST be at most one implementation:

```
record τ: C { ... } has unique contract satisfaction
```

A record cannot implement the same contract multiple times. Overlapping contract implementations are rejected.

**[ObjSafety-Checklist] Object Safety Checklist:**

A trait is object-safe if:

1. Every procedure includes a `self` parameter (no static methods).
2. Trait procedures declare no type parameters and introduce no higher-ranked lifetimes.
3. Trait procedures do not return `Self` (directly or through aliases) and may only return types whose layout is independent of the concrete implementer.
4. Each associated type exposes a concrete default that does not reference `Self` and is marked object-safe in its own right.

CITE: §2.7 — Traits and Code Reuse.

---

## 2.8 Marker Traits and Utilities

### 2.8.0 Overview

Marker traits are zero-procedure, zero-association traits used solely for compile-time reasoning. They impose behavioural requirements without introducing runtime representation. Marker traits MUST NOT define procedures, associated types, or constants.

### 2.8.1 Core Marker Traits

- `trait Send {}` — Types that can be transferred across threads by value. A composite type is `Send` if and only if each field it contains is `Send`. Immutable references (`imm T`) are always `Send`; mutable references (`mut T`) are `Send` exactly when `T` is `Send`.
- `trait Sync {}` — Types whose immutable references can be shared across threads. A composite type is `Sync` if all of its fields are `Sync`. Mutable references are `Sync` only when the referenced type guarantees internal synchronisation.
- `trait Sized {}` — Types with statically-known size. All types are `Sized` by default; a type MAY opt out by being declared `?Sized` where allowed (e.g. slice and trait object views).
- `trait Copy {}` — Types whose values can be duplicated via bitwise copy. §2.0.9.1 lists the precise requirements and automatic derivation rules.

### 2.8.2 Auto-Implementation Rules

The compiler derives implementations of `Send`, `Sync`, `Sized`, and `Copy` when a type satisfies the structural requirements listed above. Developers MAY provide manual implementations only when the rules are met; doing so otherwise is a compile-time error. Negative implementations (forbidding a derived marker) MUST be expressed explicitly using the language’s trait implementation syntax so that downstream crates observe the restriction coherently.

Marker traits participate in coherence the same way as ordinary traits: at most one implementation per type in a given crate, and implementations obey the orphan rules defined in Part XI.

CITE: §2.0.9.1 — Copy Trait; Part XII — Memory, Permissions, and Concurrency.

---

**Definition 2.28 (Parametric Types):** Parametric types enable polymorphism through type parameters, const parameters, and trait bounds.

## 2.9 Parametric Features

### 2.9.0 Overview

#### 2.9.0.1 Scope

Section §2.9 specifies parametric polymorphism including:

- Type parameters `<α>`
- Const parameters `<const N: usize>`
- Where clauses for trait bounds
- Associated types and projection bounds

### 2.9.1 Syntax

**Generic type parameters:** The productions for `GenericParams`, `TypeParam`, `Bound`, and `WhereClause` reside in Appendix A.4.

**Examples:**

```cursive
// Type parameters
record Vec<T> {
    data: *T
    len: usize
    cap: usize
}

// Const parameters
record Array<T, const N: usize> {
    data: [T; N]
}

// Trait bounds
procedure sort<T>(arr: [T])
    where T: Ord
{
    // Implementation
}

// Multiple bounds
procedure serialize<T>(value: T): string
    where T: Display + Debug
    uses alloc.heap
{
    value.to_string()
}
```

**Abstract syntax:**

```
τ ::= ∀α. τ                    (polymorphic type)
    | ∀α: Bound. τ             (bounded polymorphic type)
    | τ<τ₁, ..., τₙ>           (generic application)
```

### 2.9.2 Static Semantics

**Well-formedness:**

```
[WF-Generic-App]
Γ ⊢ T<α₁, ..., αₙ> : κ₁ → ... → κₙ → Type
Γ ⊢ τ₁ : κ₁    ...    Γ ⊢ τₙ : κₙ
where clauses satisfied
--------------------------------------------
Γ ⊢ T<τ₁, ..., τₙ> : Type

[WF-Poly]
Γ, α: Type ⊢ τ : Type
------------------------
Γ ⊢ ∀α. τ : Type

[WF-Bounded-Poly]
Γ, α: (Type where α: Bound) ⊢ τ : Type
Bound well-formed
--------------------------------------
Γ ⊢ ∀α: Bound. τ : Type
```

**Typing rules:**

```
[T-Generic-Instantiate]
Γ ⊢ e : ∀α. τ
Γ ⊢ υ : Type
------------------------------------
Γ ⊢ e : τ[α ↦ υ]

[T-Generic-Intro]
Γ, α : Type ⊢ e : τ
α ∉ FreeVars(Γ)
------------------------------------
Γ ⊢ e : ∀α. τ
```

**Constraint solving:**

```
solve_generic(∀α: Bound. C):
    1. Check bound satisfaction: Γ ⊢ α : Bound
    2. Solve constraints C under extended context
    3. Generalize free type variables
    4. Return principal type
```

**Monomorphization:**

Generic types are monomorphized at compile time. For each distinct instantiation `T<τ₁, ..., τₙ>`, the compiler generates specialized code.

**Zero-cost guarantee:** Generic abstraction MUST NOT introduce runtime overhead beyond the cost of monomorphized code.

### 2.9.3 Associated Types

**Definition 2.59 (Associated Types):** Associated types are abstract type members declared in contracts that are specified by implementing types.

#### 2.9.3.1 Syntax

**Contract declaration with associated type:** Appendix A.4 defines `ContractDecl`, `AssociatedTypeDecl`, and `ProcedureSignature`.

**Record implementing contract with associated type:** The `RecordDecl` and `AssociatedTypeSpec` productions (including contract lists) appear in Appendix A.4.

**Type projection:** Associated type projection syntax (`TypeProjection`) is defined in Appendix A.4.

#### 2.9.3.2 Examples

**Basic associated type in contract:**

```cursive
// Contract with abstract type member
contract Iterator {
    type Item  // Abstract - determined by implementation

    procedure next(mut self): Option<Self::Item>
        // Abstract signature using associated type
}

// Record implementing contract with concrete associated type
record Range: Iterator {
    type Item = i32  // Concrete specification

    start: i32
    current: i32
    end: i32

    procedure next(mut self: Range): Option<i32> {
        if self.current < self.end {
            let value = self.current
            self.current += 1
            Option::Some(value)
        } else {
            Option::None
        }
    }
}

// Another implementation with different Item type
record CharIterator: Iterator {
    type Item = char

    data: string
    position: usize

    procedure next(mut self: CharIterator): Option<char> {
        // Implementation for char iteration
    }
}
```

**Using associated types in generic code:**

```cursive
// Generic function using associated type projection
procedure collect<I>(iter: I): Vec<I::Item>
    where I: Iterator
    uses alloc.heap
{
    let result = Vec.new()
    loop {
        match iter.next() {
            Option::Some(item) => result.push(item),
            Option::None => break,
        }
    }
    result
}

// Type projection I::Item resolves based on concrete type
let range = Range { start: 0, current: 0, end: 10 }
let numbers: Vec<i32> = collect(range)
// I = Range, I::Item = i32

let chars = CharIterator { data: "hello", position: 0 }
let letters: Vec<char> = collect(chars)
// I = CharIterator, I::Item = char
```

**Associated type with bounds:**

```cursive
contract Container {
    type Element: Copy  // Associated type with bound

    procedure get(self, index: usize): Option<Self::Element>
}

record IntArray: Container {
    type Element = i32  // i32: Copy ✓

    data: [i32; 100]

    procedure get(self: IntArray, index: usize): Option<i32> {
        if index < 100 {
            Option::Some(self.data[index])
        } else {
            Option::None
        }
    }
}

// ERROR: string@Owned is not Copy
record BadContainer: Container {
    type Element = string@Owned  // ERROR: string@Owned ∉ Copy
    // ...
}
// ERROR E2803: associated type Element must satisfy bound Copy
```

**Multiple associated types:**

```cursive
contract Graph {
    type Node
    type Edge

    procedure neighbors(self, node: Self::Node): Vec<Self::Node>
        uses alloc.heap

    procedure edges(self, node: Self::Node): Vec<Self::Edge>
        uses alloc.heap
}

record AdjacencyList: Graph {
    type Node = usize
    type Edge = (usize, usize)

    adjacency: Vec<Vec<usize>>

    procedure neighbors(self: AdjacencyList, node: usize): Vec<usize>
        uses alloc.heap
    {
        self.adjacency[node].clone()
    }

    procedure edges(self: AdjacencyList, node: usize): Vec<(usize, usize)>
        uses alloc.heap
    {
        // Implementation
    }
}
```

**Associated type equality constraints:**

```cursive
// Constrain associated type to specific type
procedure process_int_iterator<I>(iter: I)
    where I: Iterator,
          I::Item = i32  // Equality constraint
{
    loop {
        match iter.next() {
            Option::Some(n) => {
                let doubled = n * 2  // Know it's i32
            },
            Option::None => break,
        }
    }
}

// Only accepts iterators with Item = i32
process_int_iterator(Range { ... })  // OK: Range::Item = i32
// process_int_iterator(CharIterator { ... })  // ERROR: CharIterator::Item = char ≠ i32
```

#### 2.9.3.3 Static Semantics

**Well-formedness for contracts with associated types:**

```
[WF-Contract-AssocType]
contract C { type T; ... } declared
associated type T has optional bound B
if B present: B well-formed
procedure signatures reference T via Self::T
----------------------------------------------
Γ ⊢ contract C : Type
```

**Well-formedness for record implementing contract:**

```
[WF-Record-Impl-AssocType]
record R: C { type T = υ; ... } declared
contract C { type T; ... }
Γ ⊢ υ : Type
if C.T has bound B: Γ ⊢ υ : B
all procedures from C implemented
----------------------------------------------
Γ ⊢ R : Type
Γ ⊢ R : C    (R implements C)
```

**Type projection rules:**

```
[T-AssocType-Proj]
Γ ⊢ τ : C
contract C { type T; ... }
record τ: C { type T = υ; ... }
----------------------------------
Γ ⊢ τ::T ≡ υ

[T-AssocType-Proj-Generic]
Γ ⊢ α : C    (type variable α with contract bound C)
contract C { type T; ... }
----------------------------------
Γ ⊢ α::T : Type
    (concrete type determined at instantiation)

[T-AssocType-Self]
within record R: C
contract C { type T; ... }
record R: C { type T = υ; ... }
----------------------------------
Γ ⊢ Self::T ≡ υ
```

**Equality constraints in where clauses:**

```
[Where-AssocType-Eq]
procedure f<X>(...)
    where X: C, X::T = υ
contract C { type T; ... }
----------------------------------
In f's body: Γ ⊢ X::T ≡ υ
```

**Bound satisfaction:**

```
[AssocType-Bound-Check]
contract C { type T: B; ... }
record R: C { type T = υ; ... }
Γ ⊢ υ : Type
----------------------------------
Γ ⊢ υ : B    (σ must satisfy bound B)
```

#### 2.9.3.4 Constraint Solving with Associated Types

**Algorithm extension:**

```
solve_with_associated_types(constraints):
    // Step 1: Normalize projections with known implementations
    for each (X::T) in type expressions:
        if X is concrete type with impl:
            if record X: C { type T = υ; ... }:
                replace X::T with υ
        else if X is type variable:
            leave as X::T (will be normalized at instantiation)

    // Step 2: Handle equality constraints
    for each constraint (X::T = υ):
        if X is concrete:
            verify: record X: C { type T = υ; ... } exists
            if not: error "associated type mismatch"
        else if X is type variable:
            record constraint for later unification

    // Step 3: Unification with projections
    for each constraint (τ₁ = τ₂):
        if τ₁ contains projections:
            normalize τ₁
        if τ₂ contains projections:
            normalize τ₂
        unify normalized types

    // Step 4: Check bound satisfaction
    for each (X: C) where C { type T: B; ... }:
        if X::T known:
            verify: X::T : B

    // Step 5: Check uniqueness (functional dependency)
    for each type X implementing C:
        verify: exactly one specification of type T

    return solved_constraints
```

**Complexity:** O(|C| × |P|) where C is constraints, P is projections; polynomial in program size.

#### 2.9.3.5 Dynamic Semantics

**Associated type erasure:**

Associated types are resolved at compile time through monomorphization. At runtime, all type projections are replaced with concrete types.

```
[E-AssocType-Erasure]
Γ ⊢ X::T ≡ υ    (at compile time)
⟨e[X::T], σ⟩ ⇓ ⟨v, σ'⟩
----------------------------------
⟨e[X::T], σ⟩ ≡ ⟨e[σ], σ⟩ ⇓ ⟨v, σ'⟩
```

No runtime representation of associated types; zero-cost abstraction.

#### 2.9.3.6 Examples and Patterns

**Pattern 1: Container with element type**

```cursive
contract Container {
    type Element

    procedure len(self): usize
    procedure get(self, index: usize): Option<Self::Element>
}

record IntVec: Container {
    type Element = i32

    data: Vec<i32>

    procedure len(self: IntVec): usize {
        self.data.len()
    }

    procedure get(self: IntVec, index: usize): Option<i32> {
        if index < self.data.len() {
            Option::Some(self.data[index])
        } else {
            Option::None
        }
    }
}
```

**Pattern 2: Conversion with output type**

```cursive
contract Into {
    type Target

    procedure into(self): Self::Target
}

record Inches: Into {
    type Target = Meters

    value: f64

    procedure into(self: Inches): Meters {
        Meters { value: self.value * 0.0254 }
    }
}

// Generic conversion
procedure convert<T>(value: T): T::Target
    where T: Into
{
    value.into()
}
```

**Pattern 3: Operation with result type**

```cursive
contract Add {
    type Output

    procedure add(self, other: Self): Self::Output
}

record Point: Add {
    type Output = Point

    x: f64
    y: f64

    procedure add(self: Point, other: Point): Point {
        Point { x: self.x + other.x, y: self.y + other.y }
    }
}

// Generic addition
procedure sum<T>(a: T, b: T): T::Output
    where T: Add
{
    a.add(b)
}
```

#### 2.9.3.7 Correctness Properties

**Theorem 2.60 (Associated Type Uniqueness):**

For any type τ implementing contract C with associated type T:

```
record τ: C { type T = υ; ... }
```

There exists exactly one specification of T:

```
∀υ₁, υ₂. (τ::T = υ₁ ∧ τ::T = υ₂) ⟹ υ₁ ≡ υ₂
```

**Theorem 2.61 (Associated Type Projection):**

```
Γ ⊢ τ : C
contract C { type T; ... }
record τ: C { type T = υ; ... }
----------------------------------
Γ ⊢ τ::T ≡ υ
```

Type projection resolves to the concrete type specified in the implementation.

**Theorem 2.62 (Associated Type Bound Satisfaction):**

```
contract C { type T: B; ... }
record τ: C { type T = υ; ... }
----------------------------------
Γ ⊢ υ : B
```

Associated types must satisfy their declared bounds.

**Theorem 2.63 (Associated Type Substitution):**

```
Γ, X : C ⊢ e : τ
X::T appears in τ
----------------------------------
Γ ⊢ e[X ↦ concrete_type] : τ[X::T ↦ concrete_type::T]
```

Associated types are substituted along with their owning types.

CITE: §2.7 — Traits and Code Reuse; §2.0.6.5 — Type Inference.

### 2.9.4 Effect Polymorphism

**Definition 2.55 (Effect Polymorphism):** Effect polymorphism is universal quantification over effect sets in procedure types.

**Syntax:** Effect‑polymorphic function types reuse the `EffectPolyType` production in Appendix A.2 (`∀ε` with optional `where ε ⊆ {…}` before a map type).

**Kinding Rules:**

```
[K-Effect-Var]
---------------
ε : Eff

[K-Effect-Poly-Type]
ε : Eff    τ : Type
----------------------
∀ε. τ : Type
```

**Well-Formedness:**

```
[WF-Effect-Poly]
Γ, ε : Eff ⊢ τ : Type
----------------------------
Γ ⊢ ∀ε. τ : Type

[WF-Effect-Bounded-Poly]
Γ, ε : Eff ⊢ τ : Type
ε_bound well-formed
------------------------------------
Γ ⊢ (∀ε where ε ⊆ ε_bound. τ) : Type
```

**Typing Rules:**

```
[T-Effect-Poly-Intro]
Γ, ε : Eff ⊢ e : τ
ε ∉ FreeEffects(Γ)
----------------------------
Γ ⊢ e : ∀ε. τ

[T-Effect-Poly-Elim]
Γ ⊢ e : ∀ε. τ
ε̂ well-formed
ε̂ satisfies bounds on ε
----------------------------
Γ ⊢ e : τ[ε ↦ ε̂]
```

**Subtyping with Effects:**

```
[Sub-Effect-Instantiation]
ε₁ ⊆ ε₂
------------------------------------------------
(...) → U ! ε₁  <:  (...) → U ! ε₂

[Sub-Effect-Poly-Invariant]
Γ, ε : Eff ⊢ τ₁ <: τ₂
--------------------------------
Γ ⊢ (∀ε. τ₁) <: (∀ε. τ₂)
```

**Effect Instantiation Algorithm:**

```
instantiate_effects(∀ε where ε ⊆ ε_bound. τ, call_site):
    // Step 1: Collect effect constraints from usage
    used_effects := analyze_call_site_effects(call_site)

    // Step 2: Verify bound satisfaction
    if used_effects ⊈ ε_bound:
        error "effect bound violation: " + (used_effects \ ε_bound)

    // Step 3: Compute minimal effect set
    ε̂ := used_effects

    // Step 4: Verify calling context provides effects
    if ε_caller ⊉ ε̂:
        error "insufficient effects: need " + ε̂ + " but have " + ε_caller

    // Step 5: Instantiate
    return τ[ε ↦ ε̂]
```

**Examples:**

**Basic effect polymorphism:**

```cursive
// Function polymorphic over effects
procedure for_each<T, ε>(
    items: [T],
    action: (T) → () ! ε
)
    uses ε  // Declare we use whatever effects action has
{
    loop item in items {
        action(item)
    }
}

// Pure usage (ε = ∅)
for_each([1, 2, 3], |x| -> { })

// Effectful usage (ε = {io.write})
for_each([1, 2, 3], |x| -> println(x))

// Multiple effects (ε = {io.write, alloc.heap})
for_each([1, 2, 3], |x| -> {
    println(x)
    let s = string.from("log")
})
```

**Bounded effect polymorphism:**

```cursive
// Restrict which effects are allowed
procedure safe_map<T, U, ε>(
    arr: [T],
    f: (T) → U ! ε
): [U]
    where ε ⊆ {alloc.heap}  // Only heap allocation allowed
    uses alloc.heap, ε
{
    let result = Vec.new()
    loop item in arr {
        result.push(f(item))
    }
    result.into_array()
}

// OK: pure (∅ ⊆ {alloc.heap})
safe_map([1, 2, 3], |x| -> x * 2)

// OK: heap only
safe_map([1, 2, 3], |x| -> Box.new(x))

// ERROR: io.write not in bound
safe_map([1, 2, 3], |x| -> {
    println(x)  // ERROR E2802: effect io.write ⊈ {alloc.heap}
    x
})
```

**Least-privilege effect inference:**

```cursive
// Infer minimal effects automatically
procedure transform<T, ε>(data: T, f: (T) → T ! ε): T
    uses ε
{
    f(data)
}

let pure_result = transform(42, |x| -> x + 1)
// Infers ε = ∅, requires no effects

let effectful_result = transform(42, |x| -> {
    println(x)
    x + 1
})
// Infers ε = {io.write}, requires io.write in caller
```

**Higher-order composition with effects:**

```cursive
// Compose functions with effect union
procedure compose<A, B, C, ε₁, ε₂>(
    f: (A) → B ! ε₁,
    g: (B) → C ! ε₂
): (A) → C ! (ε₁ ∪ ε₂)
{
    |x| -> g(f(x))
}

let f: (i32) → i32 ! ∅ = |x| -> x + 1
let g: (i32) → () ! io.write = |x| -> println(x)
let composed = compose(f, g)
// Type: (i32) → () ! (∅ ∪ {io.write})
// Simplifies to: (i32) → () ! io.write
```

**Effect polymorphism with regions:**

```cursive
// Combine effect polymorphism with region constraints
procedure with_temp_buffer<T, U, ε>(
    data: T,
    action: (T, string) → U ! ε
): U
    uses alloc.region, ε
{
    region temp {
        let buffer = string.new_in<temp>()
        action(data, buffer)
    }
}
```

**Correctness Properties:**

**Theorem 2.56 (Effect Polymorphism Soundness):**

For effect-polymorphic procedure with bounds:

```
Γ ⊢ p : ∀ε where ε ⊆ ε_bound. (...) → τᵣ ! ε
```

Any instantiation satisfies:

```
1. ε̂ ⊆ ε_bound (bound satisfaction)
2. ε_caller ⊇ ε̂ (calling context provides effects)
3. No effects outside ε̂ can occur
```

**Theorem 2.57 (Effect Principal Types):**

For expression using effects:

```
Γ ⊢ e : τ ! ε_inferred
```

The inferred effect set is minimal:

```
∀ε'. (Γ ⊢ e : τ ! ε' ⟹ ε_inferred ⊆ ε')
```

**Theorem 2.58 (Effect Substitution Preservation):**

```
Γ, ε : Eff ⊢ e : τ
ε̂ well-formed ∧ ε̂ ⊆ bounds(ε)
-------------------------------
Γ ⊢ e[ε ↦ ε̂] : τ[ε ↦ ε̂]
```

Effect substitution preserves well-typing.

CITE: §1.2.8 — Effect Polymorphism; §1.2.7 — Effect Lattice Laws; §2.0.8.2 — Effects; §2.9 — Parametric Features; §2.10 — Function Types; Part VI — Contracts and Effects; §2.0.8.4 — Regions.

---

**Definition 2.29 (Function Types):** Function types represent first-class function values with explicit parameter types, result type, and effect signatures.

## 2.10 Function Types and Higher-Order Constructs

### 2.10.0 Overview

#### 2.10.0.1 Scope

Section §2.10 specifies function types (first-class functions), closures, and the pipeline operator.

**Terminology:**

- **Function type:** `(T₁, ..., Tₙ) → U ! ε`
- **Function:** Value-level term for first-class callable
- **Procedure:** Declaration/definition form with `uses ε` clause (see Part IX)

### 2.10.1 Syntax

**Function type syntax:** See Appendix A.2 for the `FunctionType` production (including optional effect annotations).

**Canonical form:** `(T₁, ..., Tₙ) → U ! ε`

**Procedure declaration:** The `ProcedureDecl` production (with contract clauses) is defined in Appendix A.4.

**Function declaration:** Function syntax (`FunctionDecl`) is also provided in Appendix A.4.

**Closure syntax:** Closure expressions follow the `ClosureExpr` production in Appendix A.4.

**Pipeline operator:** The `PipelineExpr` production appears in Appendix A.4; the type annotation requirement is discussed in Part IV §4.5.9.

**Examples:**

```cursive
// Function type
let f: (i32, i32) → i32 ! ∅ = |x, y| -> x + y

// Procedure with effects
procedure read_file(path: string): Result<string, Error>
    uses io.read, alloc.heap
    must !path.is_empty()
{
    match std.fs.read_to_string(path) {
        Result::Ok(contents) => Result::Ok(contents),
        Result::Err(e) => Result::Err(e),
    }
}

// Pure function
function add(a: i32, b: i32): i32 {
    a + b
}

// Closure with capture
let multiplier = 10
let scale: (i32) → i32 ! ∅ = |x| -> x * multiplier

// Pipeline
let result = input
    => validate
    => transform
    => process
```

**Abstract syntax:**

```
τ ::= (τ₁, ..., τₙ) → τᵣ ! ε

v ::= closure { env: Env, body: Expr }
    | procedure_ref(name)
```

### 2.10.2 Static Semantics

**Well-formedness:**

```
[WF-FuncType]
Γ ⊢ τ₁ : Type    ...    Γ ⊢ τₙ : Type
Γ ⊢ τᵣ : Type
ε well-formed
----------------------------------------------
Γ ⊢ (τ₁, ..., τₙ) → τᵣ ! ε : Type
```

**Subtyping (contravariant arguments, covariant results):**

```
[Sub-FuncType]
υ₁ <: τ₁    ...    υₙ <: τₙ    τᵣ <: υᵣ    ε₁ ⊆ ε₂
----------------------------------------------------------
(τ₁, ..., τₙ) → τᵣ ! ε₁  <:  (υ₁, ..., υₙ) → υᵣ ! ε₂
```

Function subtyping is:

- **Contravariant** in arguments (can accept more specific input)
- **Covariant** in results (can return more specific output)
- **Covariant** in effects (fewer effects = more specific)

**Typing rules:**

```
[T-Procedure-Defn]
Γ, x₁: τ₁, ..., xₙ: τₙ ⊢ body : τᵣ
Γ ⊢ body ! ε_body
ε_body ⊆ ε_declared
------------------------------------------------
Γ ⊢ procedure name(x₁: τ₁, ..., xₙ: τₙ): τᵣ uses ε_declared { body }
    : (τ₁, ..., τₙ) → τᵣ ! ε_declared
```

**Function call, closure, and pipeline typing:**

Function call expressions, closure expressions (including capture semantics, effect propagation, and region escape analysis), and pipeline operator typing are specified in Part IV.

CITE: Part IV §4.3.1 — Function Calls; Part IV §4.10 — Closure Expressions (complete capture classification, effect propagation, region escape rules); Part IV §4.5.9 — Pipeline Operator.

### 2.10.3 Dynamic Semantics

**Procedure call and pipeline evaluation:**

Function call evaluation semantics and pipeline evaluation semantics are specified in Part IV.

CITE: Part IV §4.3.1.4 — Function Call Evaluation (left-to-right argument evaluation); Part IV §4.5.9.3 — Pipeline Evaluation (desugaring to function application).

### 2.10.4 Effect Operation Types

Effect operations have types that include their effect context. This section specifies the type rules for effect operations declared within effect declarations.

#### Syntax

Effect operations are declared within effect declarations:

```cursive
effect FileSystem {
    read(path: string): string { os_read(path) }
    write(path: string, data: string): () { os_write(path, data) }
}
```

Each operation has a type of the form:

```
E.op : (τ₁, ..., τₙ) → τᵣ ! {E}
```

#### Static Semantics

**Effect operation type:**

```
[Effect-Op-Type]
effect E { op(x₁: τ₁, ..., xₙ: τₙ): τᵣ { body } }
------------------------------------------------
Γ ⊢ E.op : (τ₁, ..., τₙ) → τᵣ ! {E}
```

**Effect operation call:**

```
[Effect-Op-Call]
Γ ⊢ E.op : (τ₁, ..., τₙ) → τᵣ ! {E}
Γ ⊢ e₁ : τ₁, ..., Γ ⊢ eₙ : τₙ
E ∈ ε_context  (E is available in context)
------------------------------------------------
Γ ⊢ E::op(e₁, ..., eₙ) : τᵣ ! {E}
```

**Generic effect operations:**

```
[Effect-Op-Generic]
effect E<T> { op(x: T): T { body } }
------------------------------------------------
Γ ⊢ E<τ>.op : (τ) → τ ! {E<τ>}
```

#### Examples

**Simple effect operation:**

```cursive
effect FileSystem {
    read(path: string): string {
        os_read(path)
    }
}

// FileSystem::read has type:
// (string) → string ! {FileSystem}

procedure foo()
    uses FileSystem
{
    let content = FileSystem::read("/tmp/file.txt")
    // content : string
}
```

**Generic effect operation:**

```cursive
effect Async<T> {
    await(future: Future<T>): T
}

// Async<Response>.await has type:
// (Future<Response>) → Response ! {Async<Response>}

procedure fetch()
    uses Async<Response>
{
    let response = Async.await(future)
    // response : Response
}
```

**Fine-grained capabilities:**

```cursive
procedure read_only()
    uses FileSystem.read  // Only read operation
{
    FileSystem::read("/tmp/file.txt")  // ✅ OK
    FileSystem::write("/tmp/file.txt", "data")  // ❌ ERROR: no write capability
}
```

CITE: Part VII §7.3 — Unified Effect System; Appendix A.6 — Effect Declaration Grammar.

---

**Definition 2.30 (Type Aliases):** Type aliases provide transparent renamings for existing types without creating new nominal types.

## 2.11 Type Aliases and Utility Forms

### 2.11.0 Overview

#### 2.11.0.1 Scope

Section §2.11 specifies type aliases for creating alternative names for types without introducing new nominal types.

### 2.11.1 Syntax

Type alias declarations use the `TypeAlias` production in Appendix A.4 (allowing optional generics and where clauses).

**Examples:**

```cursive
// Simple alias
type Kilometers = f64
type UserId = u64

// Generic alias
type Result<T> = Result<T, Error>
type Point<T> = (T, T)

// With bounds
type Comparator<T> = (T, T) → bool
    where T: Ord
```

**Abstract syntax:**

```
type A = τ    (alias definition)
```

### 2.11.2 Static Semantics

**Well-formedness:**

```
[WF-Alias]
type A = τ declared
Γ ⊢ τ : Type
------------------------
Γ ⊢ A : Type
```

**Equivalence:**

```
[Equiv-Alias]
type A = τ
----------
A ≡ τ

[Equiv-Generic-Alias]
type F<α> = τ[α]
----------------------
F<υ> ≡ τ[α ↦ υ]
```

Aliases are transparent: `A` and `τ` are fully interchangeable.

**Alias expansion algorithm:**

```
expand(A):
    if A is alias with definition τ:
        if A ∈ expansion_stack:
            error "cyclic type alias"
        push A onto expansion_stack
        return expand(τ)
    else:
        return A
```

**Cycle detection:** The compiler MUST detect and reject cyclic type aliases:

```cursive
type A = B  // ERROR: cyclic alias
type B = A
```

---

**Definition 2.31 (Self Type):** The self type provides implicit type parameter for methods, enabling polymorphic dispatch and pipeline operations.

## 2.12 Self Type and Intrinsics

### 2.12.0 Overview

#### 2.12.0.1 Scope

Section §2.12 specifies:

- Self type binding in procedure contexts
- `$` receiver notation for implicit self
- Procedure dispatch with permission-aware receivers
- Compiler intrinsic operations

### 2.12.1 Syntax

**Self type syntax:** The `SelfType` production (the keyword `Self`) is defined in Appendix A.2.

**Self parameter notation:** Procedure receiver forms (`SelfParam`, including `$` shorthand) appear in Appendix A.4.

**Examples:**

```cursive
// Procedure with explicit self
record Counter {
    value: i32

    procedure increment(self: mut Counter) {
        self.value += 1
    }

    procedure get(self: Counter): i32 {
        self.value
    }
}

// Procedure with $ notation
trait Drawable {
    procedure draw($)
        uses io.write
    {
        println("Drawing")
    }

    procedure erase(mut $) {
        // $ expands to self: mut Self
    }
}
```

**Abstract syntax:**

```
τ ::= Self    (in procedure context)

$ expands to: self: <inferred-permission> Self
```

### 2.12.2 Static Semantics

**Self type binding:**

```
[T-Procedure-Self]
within procedure defined on type T
------------------------
Γ ⊢ Self : Type
Γ ⊢ Self ≡ T
```

**$ receiver expansion:**

```
[T-Dollar-Receiver]
procedure m($, ...) defined on type T
------------------------------------
$ expands to: self: T
    (permission inferred from usage)
```

**Procedure dispatch:**

```
[T-Procedure-Dispatch]
Γ ⊢ receiver : T
T has procedure m : (self: perm T, τ₁, ..., τₙ) → τᵣ ! ε
Γ ⊢ a₁ : τ₁    ...    Γ ⊢ aₙ : τₙ
receiver provides permission perm
----------------------------------------------------------
Γ ⊢ receiver::m(a₁, ..., aₙ) : τᵣ ! ε
```

### 2.12.3 Intrinsic Operations

**Compiler-provided intrinsics:**

Intrinsics are built-in operations with compiler-known semantics:

| Intrinsic   | Signature                          | Effect | Description            |
| ----------- | ---------------------------------- | ------ | ---------------------- |
| `size_of`   | `(Type) → usize ! ∅`               | none   | Compile-time size      |
| `align_of`  | `(Type) → usize ! ∅`               | none   | Compile-time alignment |
| `transmute` | `(T) → U ! unsafe.transmute`       | unsafe | Bitwise reinterpret    |
| `offset`    | `(*T, isize) → *T ! unsafe.ptr`    | unsafe | Pointer arithmetic     |

**Intrinsic typing:**

Intrinsics are treated as built-in procedures with fixed signatures. Type checking applies standard rules with effect requirements.

---

**Definition 2.29 (Contract Witnesses):** Contract witnesses provide runtime polymorphism for contracts through explicit type erasure with permission tracking, effect polymorphism, and modal state verification.

## 2.13 Contract Witnesses

### 2.13.0 Overview

#### 2.13.0.1 Scope

Section §2.13 specifies contract witnesses, Cursive's mechanism for runtime polymorphism. Unlike compile-time polymorphism via monomorphization (Part VII §7.2.7), witnesses enable dynamic dispatch with explicit allocation strategies and full integration with Cursive's permission system, effect system, and modal types.

**Critical distinction:** Witnesses are for **contracts** (abstract interfaces), not **traits** (concrete code reuse). CITE: §2.7 — Traits; Part VII §7.2 — Behavioral Contracts.

**Key features:**

- **Explicit type erasure:** Witness construction makes allocation and indirection visible
- **Permission-aware dispatch:** Witnesses track `own`, `mut`, `imm` permissions
- **Effect polymorphism:** Witnesses are parameterized by effect sets
- **Modal state tracking:** Witnesses track modal states at runtime
- **Storage strategies:** Heap, inline, region, or borrowed allocation
- **Comptime optimization:** Sealed witnesses enable devirtualization

#### 2.13.0.2 Motivation

Runtime polymorphism solves problems that compile-time polymorphism cannot:

1. **Heterogeneous collections:** Store different types implementing the same contract
2. **Plugin systems:** Load implementations at runtime
3. **Dependency injection:** Swap implementations for testing or configuration
4. **Testing effectful code:** Mock implementations with different effect sets

**Example — Testing problem:**

```cursive
// How to test this without real network calls?
procedure fetch_user(id: i32): User
    uses net.read
{
    let response = http_get("https://api.example.com/users/{id}")
    parse_user(response)
}
```

**Solution with witnesses:**

```cursive
contract HttpClient {
    procedure get(self, url: string): Result<Response, Error>
        uses net.read
}

procedure fetch_user<ε>(client: witness<HttpClient, ε>, id: i32): User
    uses ε
{
    let response = client.get("https://api.example.com/users/{id}")?
    parse_user(response)
}

// Production: real HTTP (ε = {net.read})
let client = witness::heap(RealHttpClient{...})
fetch_user(client, 42)

// Testing: mock (ε = ∅)
let mock = witness::heap(MockHttpClient{...})
fetch_user(mock, 42)  // No net.read effect!
```

#### 2.13.0.3 Design Principles

Witnesses adhere to Cursive's core principles:

1. **Explicit over implicit:** Allocation and indirection are visible at construction
2. **Zero-cost when unused:** Static dispatch remains the default
3. **Local reasoning:** Witness types encode permissions, effects, and states
4. **Composability:** Witnesses integrate with all Cursive features

#### 2.13.0.4 Dependencies

Witnesses integrate with:

- **Part VII (Contracts and Effects):** Witnesses implement contracts (§7.2) and track effects (§7.3)
- **Part IV (Lexical Permissions):** Witnesses respect permission semantics
- **Part X (Modals):** Witnesses track modal states at runtime
- **Part VIII (Regions):** Witnesses support region allocation
- **Part XI (Metaprogramming):** Comptime enables witness optimization

CITE: Part VII §7.2 — Behavioral Contracts; Part IV — Lexical Permission System; Part X — Modals; Part VIII — Regions; Part XI — Metaprogramming.

---

### 2.13.1 Witness Types

#### 2.13.1.1 Type Syntax

**Abstract syntax:**

```
τ ::= witness<C, ε, S>           (witness type)
    | perm witness<C, ε, S>      (permission-qualified witness)
    | witness<C@State, ε, S>     (modal witness)
    | sealed witness<C, T>       (sealed witness with known type)

where:
  C ∈ Contract                   (contract name)
  ε ∈ EffectSet                  (effect set)
  S ∈ Storage                    (storage strategy)
  State ∈ ModalState             (modal state)
  T ∈ Type                       (concrete type for sealed witnesses)
  perm ∈ {own, mut, imm}         (permission)
```

**Storage strategies:**

```
S ::= heap                       (heap-allocated, default)
    | inline<N>                  (stack-allocated, N bytes)
    | region<R>                  (region-allocated)
    | borrowed                   (borrowed, no ownership)

where:
  N ∈ ℕ                          (size in bytes)
  R ∈ RegionVar                  (region variable)
```

**Concrete syntax:**

```cursive
// Full form
witness<Contract, EffectSet, Storage>

// Common form (heap storage)
witness<Contract, EffectSet>

// Minimal form (pure contract, heap storage)
witness<Contract>

// With permissions
own witness<Contract, ε>
mut witness<Contract, ε>
imm witness<Contract, ε>

// With modal states
witness<Contract@State, ε>

// Sealed witness
sealed witness<Contract, ConcreteType>
```

#### 2.13.1.2 Well-Formedness

**Type formation:**

```
[Witness-Formation]
Γ ⊢ C : Contract
Γ ⊢ ε : EffectSet
Γ ⊢ S : Storage
--------------------------------
Γ ⊢ witness<C, ε, S> : Type

[Witness-Permission]
Γ ⊢ witness<C, ε, S> : Type
perm ∈ {own, mut, imm}
--------------------------------
Γ ⊢ perm witness<C, ε, S> : Type

[Witness-Modal]
Γ ⊢ C : Contract
C has modal states
State ∈ states(C)
Γ ⊢ ε : EffectSet
Γ ⊢ S : Storage
--------------------------------
Γ ⊢ witness<C@State, ε, S> : Type

[Witness-Sealed]
Γ ⊢ C : Contract
Γ ⊢ T : Type
T : C  (T implements C)
--------------------------------
Γ ⊢ sealed witness<C, T> : Type
```

**Storage well-formedness:**

```
[Storage-Heap]
--------------------------------
Γ ⊢ heap : Storage

[Storage-Inline]
N ∈ ℕ, N > 0
--------------------------------
Γ ⊢ inline<N> : Storage

[Storage-Region]
R ∈ dom(Δ)  (R is active region)
--------------------------------
Γ;Δ ⊢ region<R> : Storage

[Storage-Borrowed]
--------------------------------
Γ ⊢ borrowed : Storage
```

#### 2.13.1.3 Type Equivalence

**Witness equivalence:**

```
[Equiv-Witness]
C₁ ≡ C₂
ε₁ ≡ ε₂
S₁ ≡ S₂
--------------------------------
witness<C₁, ε₁, S₁> ≡ witness<C₂, ε₂, S₂>
```

**Default storage:**

```
[Equiv-Witness-Default-Storage]
--------------------------------
witness<C, ε> ≡ witness<C, ε, heap>
```

**Permission qualification:**

```
[Equiv-Witness-Permission]
--------------------------------
witness<C, ε, S> ≡ own witness<C, ε, S>  (default permission)
```

#### 2.13.1.4 Subtyping

**Effect subtyping:**

```
[Subtype-Witness-Effect]
ε₁ ⊆ ε₂  (fewer effects)
--------------------------------
witness<C, ε₁, S> <: witness<C, ε₂, S>
```

**Rationale:** A witness with fewer effects can be used where more effects are allowed (contravariance).

**Permission subtyping:**

```
[Subtype-Witness-Permission]
--------------------------------
own witness<C, ε, S> <: mut witness<C, ε, S>
mut witness<C, ε, S> <: imm witness<C, ε, S>
```

**Storage invariance:**

```
[Invariant-Witness-Storage]
S₁ ≢ S₂
--------------------------------
witness<C, ε, S₁> ⊄ witness<C, ε, S₂>
```

**Rationale:** Storage strategies have different memory layouts and cannot be substituted.

---

### 2.13.2 Construction and Storage

#### 2.13.2.1 Witness Construction

**Construction operations:**

```cursive
// Heap allocation
witness.heap<C, ε>(value: T): own witness<C, ε, heap>
    where T: C
    uses alloc.heap

// Inline storage
witness::inline<N, C, ε>(value: T): own witness<C, ε, inline<N>>
    where T: C, size_of(T) <= N

// Region allocation
witness::region<R, C, ε>(value: T): own witness<C, ε, region<R>>
    where T: C
    uses alloc.region

// Borrowed (no allocation)
witness.borrow<C, ε>(ref: imm T): imm witness<C, ε, borrowed>
    where T: C

witness.borrow_mut<C, ε>(ref: mut T): mut witness<C, ε, borrowed>
    where T: C
```

**Type rules:**

```
[T-Witness-Heap]
Γ ⊢ v : T
T : C  (T implements C)
effects(T, C) ⊆ ε
--------------------------------
Γ ⊢ witness::heap(v) : own witness<C, ε, heap> ! alloc.heap

[T-Witness-Inline]
Γ ⊢ v : T
T : C
effects(T, C) ⊆ ε
size_of(T) + overhead <= N
--------------------------------
Γ ⊢ witness::inline::<N>(v) : own witness<C, ε, inline<N>>

[T-Witness-Region]
Γ;Δ ⊢ v : T
T : C
effects(T, C) ⊆ ε
R ∈ dom(Δ)
--------------------------------
Γ;Δ ⊢ witness::region::<R>(v) : own witness<C, ε, region<R>> ! alloc.region

[T-Witness-Borrow]
Γ ⊢ r : imm T
T : C
effects(T, C) ⊆ ε
--------------------------------
Γ ⊢ witness::borrow(r) : imm witness<C, ε, borrowed>
```

#### 2.13.2.2 Storage Strategies

**Heap storage (default):**

```cursive
let w: own witness<Drawable, {io.write}> = witness::heap(Circle{radius: 5.0})
// Memory layout: [vtable_ptr | state_tag | Circle data]
// Allocation: sizeof(Circle) + 2 * sizeof(usize)
// Deallocation: when witness is dropped
```

**Inline storage (stack):**

```cursive
let w: own witness<Drawable, {io.write}, inline<64>> =
    witness::inline::<64>(Circle{radius: 5.0})
// Memory layout: [vtable_ptr | state_tag | 64-byte buffer]
// Allocation: stack (no heap allocation)
// ERROR if sizeof(Circle) > 64 bytes
```

**Region storage:**

```cursive
region r {
    let w: own witness<Drawable, {io.write}, region<r>> =
        witness::region::<r>(Circle{radius: 5.0})
    // Allocation: in region r
    // Deallocation: when region exits (LIFO)
}
```

**Borrowed storage:**

```cursive
let circle = Circle{radius: 5.0}
let w: imm witness<Drawable, {io.write}, borrowed> =
    witness::borrow(&circle)
// No allocation, just vtable pointer
// Lifetime tied to circle
```

#### 2.13.2.3 Memory Layout

**Witness representation:**

```
Heap/Region/Inline:
┌-------------┬------------┬--------------┐
│ vtable_ptr  │ state_tag  │ data         │
│ (usize)     │ (usize)    │ (T-sized)    │
└-------------┴------------┴--------------┘

Borrowed:
┌-------------┬------------┬--------------┐
│ vtable_ptr  │ state_tag  │ data_ptr     │
│ (usize)     │ (usize)    │ (usize)      │
└-------------┴------------┴--------------┘
```

**VTable structure:**

```
VTable<Contract>:
┌------------------┐
│ drop_fn          │  → (*()) → ()
├------------------┤
│ method_1         │  → procedure pointer
├------------------┤
│ method_2         │  → procedure pointer
├------------------┤
│ ...              │
└------------------┘
```

---

### 2.13.3 Permission-Aware Dispatch

#### 2.13.3.1 Permission Semantics

Witnesses track permissions and enforce them at procedure dispatch:

```cursive
contract Counter {
    procedure get(self: imm Self): i32
    procedure increment(self: mut Self)
    procedure reset(self: own Self)
}
```

**Permission-based access:**

```
[T-Witness-Procedure-Imm]
Γ ⊢ w : imm witness<C, ε, S>
C.m : (self: imm Self, ...) → τ ! ε'
ε' ⊆ ε
--------------------------------
Γ ⊢ w.m(...) : τ ! ε'

[T-Witness-Procedure-Mut]
Γ ⊢ w : mut witness<C, ε, S>
C.m : (self: mut Self, ...) → τ ! ε'
ε' ⊆ ε
--------------------------------
Γ ⊢ w.m(...) : τ ! ε'

[T-Witness-Procedure-Own]
Γ ⊢ w : own witness<C, ε, S>
C.m : (self: own Self, ...) → τ ! ε'
ε' ⊆ ε
--------------------------------
Γ ⊢ w.m(...) : τ ! ε'  (consumes w)
```

**Permission errors:**

```cursive
let counter: imm witness<Counter, ∅> = witness::borrow(&my_counter)
counter.get()        // ✅ OK (imm procedure)
counter.increment()  // ❌ ERROR E2D01: need mut permission

let counter: mut witness<Counter, ∅> = witness::heap(MyCounter{value: 0})
counter.get()        // ✅ OK (imm procedure on mut witness)
counter.increment()  // ✅ OK (mut procedure)
counter.reset()      // ❌ ERROR E2D02: need own permission
```

#### 2.13.3.2 Permission Conversion

**Borrowing:**

```
[T-Witness-Borrow-Mut]
Γ ⊢ w : own witness<C, ε, S>
--------------------------------
Γ ⊢ &mut w : mut witness<C, ε, borrowed>

[T-Witness-Borrow-Imm]
Γ ⊢ w : perm witness<C, ε, S>
perm ∈ {own, mut, imm}
--------------------------------
Γ ⊢ &w : imm witness<C, ε, borrowed>
```

**Example:**

```cursive
let counter: own witness<Counter, ∅> = witness::heap(MyCounter{value: 0})

// Borrow mutably
increment_counter(&mut counter)  // counter: mut witness<Counter, ∅, borrowed>

// Borrow immutably
let value = read_counter(&counter)  // counter: imm witness<Counter, ∅, borrowed>

// Still own after borrows end
counter.reset()  // OK, still own
```

---

### 2.13.4 Effect Polymorphism

#### 2.13.4.1 Effect Tracking

Witnesses track effect sets in their type:

```cursive
contract Logger {
    procedure log(self, msg: string)
        uses io.write
}

// Witness with specific effect set
let logger: witness<Logger, {io.write}> = witness::heap(FileLogger{...})
```

#### 2.13.4.2 Effect Subtyping

Implementations with fewer effects are safe:

```
[T-Witness-Effect-Subtype]
Γ ⊢ v : T
T : C
effects(T, C) = ε₁
ε₁ ⊆ ε₂
--------------------------------
Γ ⊢ witness::heap(v) : witness<C, ε₂, heap>
```

**Example:**

```cursive
// Mock with NO effects
record MockLogger: Logger {
    procedure log(self: MockLogger, msg: string) {
        // ∅ effects
    }
}

// Safe: ∅ ⊆ {io.write}
let logger: witness<Logger, {io.write}> = witness::heap(MockLogger{})
```

#### 2.13.4.3 Effect Polymorphism

Witnesses can be effect-polymorphic:

```cursive
procedure with_logger<ε>(
    logger: witness<Logger, ε>,
    action: () → () ! ε
)
    uses ε
{
    logger.log("Starting")
    action()
    logger.log("Done")
}

// Pure logger (ε = ∅)
with_logger(witness::heap(MockLogger{}), || -> { /* pure */ })

// Effectful logger (ε = {io.write})
with_logger(witness::heap(FileLogger{}), || -> { println("test") })
```

**Type rule:**

```
[T-Witness-Effect-Poly]
Γ ⊢ w : witness<C, ε₁, S>
Γ ⊢ f : (...) → τ ! ε₂
ε₁ ∪ ε₂ ⊆ ε
--------------------------------
Γ ⊢ procedure<ε>(...) uses ε well-formed
```

---

### 2.13.5 Modal State Tracking

#### 2.13.5.1 Modal Witnesses

Witnesses track modal states at runtime:

```cursive
contract Connection {
    modal states {
        Disconnected
        Connected
        Closed
    }

    procedure connect(self: mut Connection@Disconnected): Result<(), Error>
        uses net.connect
        will match result {
            Ok(_) => self is Connection@Connected,
            Err(_) => self is Connection@Disconnected
        }

    procedure send(self: mut Connection@Connected, data: string): Result<(), Error>
        uses net.write

    procedure close(self: mut Connection@Connected)
        uses net.write
        will self is Connection@Closed
}
```

**Modal witness type:**

```cursive
// Witness in Disconnected state
let conn: own witness<Connection@Disconnected, {net.connect, net.write}> =
    witness::heap(TcpConnection{...})
```

#### 2.13.5.2 State Transitions

**Type rule:**

```
[T-Witness-Modal-Transition]
Γ ⊢ w : perm witness<C@S₁, ε, Storage>
C.m : (self: perm Self@S₁, ...) → Self@S₂ ! ε'
ε' ⊆ ε
--------------------------------
Γ ⊢ w.m(...) : perm witness<C@S₂, ε, Storage> ! ε'
```

**Example:**

```cursive
let conn: own witness<Connection@Disconnected, {net.connect, net.write}> =
    witness::heap(TcpConnection{...})

// State transition
let conn = conn.connect()?
// Type: witness<Connection@Connected, {net.connect, net.write}>

// State-dependent operations
conn.send("Hello")?  // ✅ OK (Connected state)
conn.connect()?      // ❌ ERROR E2D03: expected @Disconnected, found @Connected
```

#### 2.13.5.3 Runtime State Verification

**State tag representation:**

```
state_tag: usize = match current_state {
    Disconnected => 0,
    Connected => 1,
    Closed => 2,
}
```

**Runtime check (debug mode):**

```cursive
// Generated code for state-dependent procedure call
procedure witness_call_with_state_check(
    witness: *Witness,
    expected_state: usize,
    method_ptr: *()
) {
    if witness.state_tag != expected_state {
        panic("State mismatch: expected {expected_state}, found {witness.state_tag}")
    }
    // Call procedure via vtable
    (method_ptr)(witness.data)
}
```

---

### 2.13.6 Comptime Optimization

#### 2.13.6.1 Sealed Witnesses

Sealed witnesses preserve concrete type information for devirtualization:

```cursive
// Sealed witness: compiler knows concrete type
let w: sealed witness<Drawable, Circle> = witness::seal(Circle{radius: 5.0})

// Devirtualized call (no vtable lookup)
w.draw()  // Direct call to Circle.draw()

// Can recover concrete type
let circle: Circle = w.unseal()  // ✅ OK
```

**Type rules:**

```
[T-Witness-Seal]
Γ ⊢ v : T
T : C
--------------------------------
Γ ⊢ witness::seal(v) : sealed witness<C, T>

[T-Witness-Unseal]
Γ ⊢ w : sealed witness<C, T>
--------------------------------
Γ ⊢ w::unseal() : T

[T-Witness-Sealed-Call]
Γ ⊢ w : sealed witness<C, T>
T.m : (self: perm T, ...) → τ ! ε
--------------------------------
Γ ⊢ w::m(...) : τ ! ε  (devirtualized)
```

#### 2.13.6.2 Comptime Dispatch Selection

```cursive
enum DispatchMode {
    Static
    Dynamic
}

procedure render<T, comptime dispatch: DispatchMode>(shape: T)
    where T: Drawable
    uses io.write
{
    comptime {
        if dispatch == DispatchMode.Static {
            // Static dispatch (monomorphization)
            shape.draw()
        } else {
            // Dynamic dispatch (witness)
            let w = witness::heap(shape)
            w.draw()
        }
    }
}

// Usage
render::<Circle, DispatchMode.Static>(circle)   // Zero-cost
render::<Circle, DispatchMode.Dynamic>(circle)  // Runtime polymorphism
```

---

### 2.13.7 Type Rules Summary

**Complete typing rules:**

```
[T-Witness-Construction]
Γ ⊢ v : T
T : C  (T implements C)
effects(T, C) ⊆ ε
alloc_strategy(S) ⊢ v : ok
--------------------------------
Γ ⊢ witness.S(v) : own witness<C, ε, S> ! effects(S)

[T-Witness-Procedure-Call]
Γ ⊢ w : perm witness<C, ε, S>
C.m : (self: perm' Self, τ₁, ..., τₙ) → τ ! ε'
perm ≥ perm'  (permission sufficient)
ε' ⊆ ε  (effects within bound)
Γ ⊢ a₁ : τ₁    ...    Γ ⊢ aₙ : τₙ
--------------------------------
Γ ⊢ w.m(a₁, ..., aₙ) : τ ! ε'

[T-Witness-Modal-Call]
Γ ⊢ w : perm witness<C@S₁, ε, Storage>
C.m : (self: perm Self@S₁, ...) → τ@S₂ ! ε'
ε' ⊆ ε
--------------------------------
Γ ⊢ w.m(...) : perm witness<C@S₂, ε, Storage> ! ε'

[T-Witness-Drop]
Γ ⊢ w : own witness<C, ε, S>
--------------------------------
Γ ⊢ drop(w) : () ! effects(S)
```

**Permission ordering:**

```
own ≥ mut ≥ imm
```

**Effect ordering:**

```
ε₁ ⊆ ε₂  ⟹  witness<C, ε₁, S> <: witness<C, ε₂, S>
```

---

### 2.13.8 Examples

**Example 1 — Heterogeneous collection:**

```cursive
contract Drawable {
    procedure draw(self)
        uses io.write
}

let shapes: [own witness<Drawable, {io.write}>] = [
    witness::heap(Circle{radius: 5.0}),
    witness::heap(Rectangle{width: 10.0, height: 20.0}),
    witness::heap(Triangle{a: p1, b: p2, c: p3}),
]

for shape in shapes {
    shape.draw()
}
```

**Example 2 — Testing with mocks:**

```cursive
contract HttpClient {
    procedure get(self, url: string): Result<Response, Error>
        uses net.read
}

procedure fetch_user<ε>(client: witness<HttpClient, ε>, id: i32): User
    uses ε
{
    let response = client.get("https://api.example.com/users/{id}")?
    parse_user(response)
}

// Production
let client = witness::heap(RealHttpClient{...})
fetch_user(client, 42)  // uses net.read

// Testing
let mock = witness::heap(MockHttpClient{...})
fetch_user(mock, 42)  // uses ∅
```

**Example 3 — Modal state machine:**

```cursive
let file: own witness<FileHandle@Closed, {fs.read, fs.write}> =
    witness::heap(PosixFile{path: "/tmp/test.txt"})

let file = file.open()?  // Now: witness<FileHandle@Open, ...>
let bytes_read = file::read(&mut buffer)?
file.close()  // Now: witness<FileHandle@Closed, ...>
```

---

### 2.13.9 Diagnostics

**E2D01 — Insufficient permission for witness procedure call:**

```
error[E2D01]: insufficient permission for procedure call
  --> example.cursive:10:5
   |
10 |     counter.increment()
   |     ^^^^^^^^^^^^^^^^^^^ requires `mut` permission
   |
note: witness has `imm` permission
  --> example.cursive:9:9
   |
9  |     let counter: imm witness<Counter, ∅> = ...
   |                  ^^^ declared as immutable
```

**E2D02 — Insufficient permission for consuming procedure:**

```
error[E2D02]: procedure requires ownership
  --> example.cursive:12:5
   |
12 |     counter.reset()
   |     ^^^^^^^^^^^^^^^ requires `own` permission
   |
note: witness has `mut` permission
  --> example.cursive:11:9
   |
11 |     let counter: mut witness<Counter, ∅> = ...
   |                  ^^^ not owned
```

**E2D03 — Modal state mismatch:**

```
error[E2D03]: procedure requires different modal state
  --> example.cursive:15:5
   |
15 |     conn.connect()
   |     ^^^^^^^^^^^^^^ requires state `@Disconnected`
   |
note: witness is in state `@Connected`
  --> example.cursive:14:13
   |
14 |     let conn = conn.connect()?
   |                ^^^^^^^^^^^^^^ transitioned to `@Connected`
```

**E2D04 — Effect set mismatch:**

```
error[E2D04]: witness effect set too restrictive
  --> example.cursive:20:5
   |
20 |     logger.log("message")
   |     ^^^^^^^^^^^^^^^^^^^^^ requires effect `io.write`
   |
note: witness effect set is `∅`
  --> example.cursive:19:9
   |
19 |     let logger: witness<Logger, ∅> = ...
   |                                 ^ empty effect set
```

**E2D05 — Inline storage size exceeded:**

```
error[E2D05]: value too large for inline storage
  --> example.cursive:25:5
   |
25 |     witness::inline::<64>(LargeStruct{...})
   |     ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ requires 128 bytes
   |
note: inline storage limited to 64 bytes
  --> example.cursive:25:23
   |
25 |     witness::inline::<64>(LargeStruct{...})
   |                       ^^ storage size
```

---

## 2.14 Integration with Other Features

This section specifies how the type system integrates with other major language features.

### 2.14.1 Types and Permissions

The type system and permission system (Part IV) integrate through permission-qualified types.

**Permission wrappers as type constructors:**

```
[Type-Permission-Wrap]
Γ ⊢ T : Type
perm ∈ {own, mut, imm}
--------------------------------------
Γ ⊢ perm T : Type
```

**Example: Permission-polymorphic container:**

```cursive
record Vec<T> {
    data: *mut T
    len: usize
    capacity: usize
}

// Permission-aware method dispatch
procedure Vec<T>::push(self: mut Vec<T>, value: own T)
    uses alloc.heap
{
    // Can only call on mut or own Vec
    // Consumes ownership of value
}

procedure Vec<T>::get(self: Vec<T>, index: usize): Option<T> {
    // Works with any permission (imm, mut, own)
    // Returns immutable reference
}

procedure Vec<T>::pop(self: mut Vec<T>): Option<T> {
    // Requires mut permission
    // Returns owned value
}
```

**Permission propagation through type constructors:**

```cursive
record Container<T> {
    value: T
}

// Permission applied to aggregate propagates to fields
let c1: own Container<i32> = Container { value: 42 }
// c1.value has type: own i32

let c2: mut Container<i32> = c1  // Coercion: own → mut
// c2.value has type: mut i32

let c3: Container<i32> = c2  // Coercion: mut → imm
// c3.value has type: imm i32 (equivalent to just i32)
```

**Type checking with permissions:**

```
[Type-Field-Access-Permission]
Γ ⊢ e : perm R
R = record Name { ..., f: T, ... }
--------------------------------------
Γ ⊢ e.f : perm T

[Type-Permission-Coercion]
Γ ⊢ e : own T
--------------------------------------
Γ ⊢ e : mut T

Γ ⊢ e : own T
--------------------------------------
Γ ⊢ e : imm T

Γ ⊢ e : mut T
--------------------------------------
Γ ⊢ e : imm T
```

**Permission-polymorphic generic functions:**

```cursive
// Function works with any permission
function compute<T>(value: T): i32 {
    // Can read value regardless of permission
    result value.hash()
}

let owned: own Data = Data::new()
compute(owned)  // Works: own → imm coercion

let mutable: mut Data = get_mut_data()
compute(mutable)  // Works: mut → imm coercion

let immutable: Data = get_data()
compute(immutable)  // Works: already imm
```

### 2.14.2 Types and Effects

Types interact with the effect system (Part VII) through effect-polymorphic types and effect-indexed types.

**Effect-polymorphic function types:**

```cursive
// Function type with effect parameter
type Processor<T> = ∀ε. (T) → Result<T, Error> ! ε

// Implementation can specify concrete effects
function validate(data: string): Result<string, Error> ! ∅ {
    // Pure validation, no effects
    if data.is_empty() {
        Result::Err(Error::Empty)
    } else {
        Result::Ok(data)
    }
}

function persist(data: string): Result<string, Error> ! {fs.write} {
    // Has file system effect
    write_to_file(data)?
    Result::Ok(data)
}

// Both implementations satisfy Processor<string>
let validator: Processor<string> = validate
let persister: Processor<string> = persist
```

**Effect constraints in generic bounds:**

```cursive
contract Serializable<ε> {
    procedure serialize(self: Self): string ! ε
}

// Type that can serialize with bounded effects
record Config: Serializable<∅> {
    name: string
    value: i32

    procedure serialize(self: Config): string ! ∅ {
        result "{name: {}, value: {}}"
            .format(self.name, self.value)
    }
}

// Type that requires I/O for serialization
record Database: Serializable<{io.write}> {
    connection: Connection

    procedure serialize(self: Database): string ! {io.write} {
        self.connection::write("snapshot")?
        result "serialized"
    }
}
```

**Effect tracking through type constructors:**

```cursive
// Effect-aware Option type
enum Result<T, E> {
    Ok(T),
    Err(E)
}

// Effect propagates through combinator
function Result::map<T, U, ε>(
    self: Result<T, E>,
    f: (T) → U ! ε
): Result<U, E> ! ε
{
    match self {
        Result::Ok(value) => Result::Ok(f(value)),  // Effect ε occurs here
        Result::Err(e) => Result::Err(e)  // No effect
    }
}

// Usage with different effects
let pure_result = Result::Ok(42)::map(|x| x * 2)  // ! ∅
let io_result = Result::Ok("data")::map(write_file)  // ! {fs.write}
```

### 2.14.3 Types and Regions

Types integrate with region-based memory management (Part VIII) through region-tagged types.

**Region-parameterized types:**

```cursive
// Type allocated in a specific region
region<'r> temp {
    let buffer: own [u8; 1024]@temp = alloc_in<temp>([0u8; 1024])
    // buffer's type carries region tag @temp
}
// buffer deallocated when temp region closes
```

**Region escape analysis in type checking:**

```
[Type-Region-Escape]
Γ; Δ · r ⊢ e : τ@r
target context has region stack Δ' where r ∉ Δ'
--------------------------------------
ERROR: type τ@r escapes region r
```

**Example: Region-safe return types:**

```cursive
// INVALID: returns region-tagged type
function create_temp_buffer<'r>(): [u8]@temp {
    region temp {
        let buf = alloc_in<temp>([1u8, 2, 3])
        result buf  // ERROR: buf@temp escapes region temp
    }
}

// VALID: allocates in caller's context
function create_buffer(): [u8] {
    let buf = [1u8, 2, 3]  // Stack allocation
    result buf
}

// VALID: region-polymorphic (allocates in caller's region)
function create_in_region<'r>(region: 'r): [u8]@'r {
    let buf = alloc_in<'r>([1u8, 2, 3])
    result buf  // OK: lifetime matches return type
}
```

**Region-polymorphic data structures:**

```cursive
record Arena<'r> {
    region: 'r
    allocations: Vec<*mut u8@'r>
}

procedure Arena::'r::allocate(
    self: mut Arena<'r>,
    size: usize
): *mut u8@'r
    uses alloc.region
{
    let ptr = alloc_in<'r>(size)
    self.allocations::push(ptr)
    result ptr
}

// Usage
region<'local> app_region {
    var arena = Arena::new<'local>(app_region)
    let ptr1 = arena::allocate(64)  // Type: *mut u8@'local
    let ptr2 = arena::allocate(128)  // Type: *mut u8@'local
    // Both pointers deallocated when app_region closes
}
```

### 2.14.4 Types and Modals

Modal types (Part X) provide state-dependent typing through modal states.

**State-indexed type refinement:**

```cursive
modal FileHandle {
    states { Closed, Open, Locked }

    // Type signature depends on current state
    procedure FileHandle@Closed::open(
        self: own FileHandle@Closed
    ): Result<FileHandle@Open, Error>
        uses fs.open
    {
        // State transition: @Closed → @Open
    }

    // Only available in @Open state
    procedure FileHandle@Open::read(
        self: FileHandle@Open,
        buffer: mut [u8]
    ): Result<usize, Error>
        uses fs.read
    {
        // Type system ensures file is open
    }

    // State transition with permission change
    procedure FileHandle@Open::close(
        self: own FileHandle@Open
    ): FileHandle@Closed
    {
        // State transition: @Open → @Closed
        // Consumes @Open, produces @Closed
    }
}
```

**Type checking modal transitions:**

```
[Type-Modal-Transition]
Γ ⊢ e : T@S₁
procedure p : (self: own T@S₁, ...) → T@S₂ ! ε
Γ ⊢ args : τ₁, ..., τₙ
--------------------------------------
Γ ⊢ e::p(args) : T@S₂ ! ε
After: e is invalidated (consumed by transition)
```

**Example: Protocol enforcement through modal types:**

```cursive
modal Connection {
    states { Init, Connected, Authenticated, Closed }

    // State machine type-checks this sequence:
    // Init → Connected → Authenticated → Closed

    procedure Connection@Init::connect(
        self: own Connection@Init
    ): Result<Connection@Connected, Error>

    procedure Connection@Connected::authenticate(
        self: own Connection@Connected,
        credentials: Credentials
    ): Result<Connection@Authenticated, Error>

    procedure Connection@Authenticated::send(
        self: Connection@Authenticated,
        data: [u8]
    ): Result<(), Error>

    procedure Connection@Authenticated::close(
        self: own Connection@Authenticated
    ): Connection@Closed
}

// Type-safe usage
procedure secure_send(data: [u8]) {
    let conn = Connection@Init::new()

    // Type: Connection@Init
    let conn = match conn::connect() {
        Result::Ok(c) => c,  // Type: Connection@Connected
        Result::Err(e) => return
    }

    // Type: Connection@Connected
    let conn = match conn::authenticate(get_credentials()) {
        Result::Ok(c) => c,  // Type: Connection@Authenticated
        Result::Err(e) => return
    }

    // Type: Connection@Authenticated - send is available
    conn::send(data)?

    // conn::connect()  // ERROR: no connect() on @Authenticated

    conn::close()  // Type: Connection@Closed
}
```

### 2.14.5 Types and Contracts

Contract types (Part VII) add behavioral specifications to type signatures.

**Contracts as type refinements:**

```cursive
contract Stack<T> {
    procedure push(self: mut Self, value: T)
        will self.len() == @old(self.len()) + 1

    procedure pop(self: mut Self): Option<T>
        will match result {
            Option::Some(_) => self.len() == @old(self.len()) - 1,
            Option::None => self.len() == 0
        }

    procedure len(self: Self): usize
        will result >= 0
}

// Implementation must satisfy contracts
record Vec<T>: Stack<T> {
    data: [T]
    count: usize

    procedure push(self: mut Vec<T>, value: T)
        will self.len() == @old(self.len()) + 1
    {
        self.data::push(value)
        self.count += 1
    }

    procedure pop(self: mut Vec<T>): Option<T>
        will match result {
            Option::Some(_) => self.len() == @old(self.len()) - 1,
            Option::None => self.len() == 0
        }
    {
        if self.count > 0 {
            self.count -= 1
            Option::Some(self.data::pop()?)
        } else {
            Option::None
        }
    }

    procedure len(self: Vec<T>): usize
        will result >= 0
    {
        result self.count
    }
}
```

**Type checking with contract constraints:**

```
[Type-Contract-Satisfaction]
Γ ⊢ T : Type
contract C { p₁; ...; pₙ }
∀i. T implements pᵢ with compatible contracts
--------------------------------------
Γ ⊢ T : C

[Type-Contract-Call]
Γ ⊢ e : T where T : C
C declares procedure p : signature must P will Q
Γ ⊨ P[self ↦ e]  (precondition holds)
--------------------------------------
Γ ⊢ e::p(...) permitted
After: Γ ⊨ Q  (postcondition established)
```

### 2.14.6 Types and Generics

Generic types interact with all type system features through constraint-based polymorphism.

**Multi-constraint generic types:**

```cursive
// Generic type with multiple constraints
function process<T>(value: T): Result<string, Error>
where
    T: Serializable<∅>,
    T: Clone,
    T: Eq
{
    let copy = value.clone()
    if copy == value {
        Result::Ok(value.serialize())
    } else {
        Result::Err(Error::Inconsistent)
    }
}

// Constraints verified at instantiation
record User {
    name: string
    age: i32
}

// Must implement all required contracts
impl User: Serializable<∅> {
    procedure serialize(self: User): string ! ∅ {
        result "User({}, {})".format(self.name, self.age)
    }
}

impl User: Clone {
    procedure clone(self: User): User {
        result User { name: self.name, age: self.age }
    }
}

impl User: Eq {
    procedure eq(self: User, other: User): bool {
        result self.name == other.name && self.age == other.age
    }
}

// All constraints satisfied
let user = User { name: "Alice", age: 30 }
let serialized = process(user)  // ✓ Type checks
```

**Higher-kinded type constraints:**

```cursive
// Generic over type constructors
contract Functor<F<_>> {
    procedure map<A, B>(self: F<A>, f: (A) → B): F<B>
}

// Option implements Functor
impl Option: Functor {
    procedure map<A, B>(self: Option<A>, f: (A) → B): Option<B> {
        match self {
            Option::Some(a) => Option::Some(f(a)),
            Option::None => Option::None
        }
    }
}

// Result implements Functor (in Ok value)
impl Result: Functor {
    procedure map<A, B, E>(
        self: Result<A, E>,
        f: (A) → B
    ): Result<B, E> {
        match self {
            Result::Ok(a) => Result::Ok(f(a)),
            Result::Err(e) => Result::Err(e)
        }
    }
}
```

---

## 2.15 Advanced Type Patterns

This section presents sophisticated type system patterns for complex scenarios.

### 2.15.1 Builder Patterns with Typestate

Builder patterns use modal types to enforce construction sequences.

**Typestate builder:**

```cursive
modal RequestBuilder {
    states { Empty, WithUrl, WithMethod, Complete }

    procedure RequestBuilder@Empty::new(): RequestBuilder@Empty {
        result RequestBuilder { /* ... */ }
    }

    procedure RequestBuilder@Empty::url(
        self: own RequestBuilder@Empty,
        url: string
    ): RequestBuilder@WithUrl {
        // Transition: Empty → WithUrl
        result RequestBuilder { url, /* ... */ }
    }

    procedure RequestBuilder@WithUrl::method(
        self: own RequestBuilder@WithUrl,
        method: string
    ): RequestBuilder@WithMethod {
        // Transition: WithUrl → WithMethod
        result RequestBuilder { method, /* ... */ }
    }

    procedure RequestBuilder@WithMethod::build(
        self: own RequestBuilder@WithMethod
    ): Request {
        // Final transition: WithMethod → Request
        result Request { /* ... */ }
    }
}

// Usage: Type system enforces ordering
let request = RequestBuilder@Empty::new()
    ::url("https://api.example.com")
    ::method("POST")
    ::build()

// let bad = RequestBuilder@Empty::new()::build()
//           ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// ERROR: no build() method on @Empty state
```

**Phantom type parameters for compile-time state:**

```cursive
record Builder<State> {
    data: string
    phantom: PhantomData<State>
}

// State marker types (zero-sized)
record EmptyState { }
record UrlState { }
record CompleteState { }

// Constructors for each state
procedure Builder::new(): Builder<EmptyState> {
    result Builder { data: "", phantom: PhantomData }
}

procedure Builder<EmptyState>::with_url(
    self: Builder<EmptyState>,
    url: string
): Builder<UrlState> {
    result Builder { data: url, phantom: PhantomData }
}

procedure Builder<UrlState>::build(
    self: Builder<UrlState>
): Request {
    result Request { url: self.data }
}

// Type-safe usage
let request = Builder::new()
    ::with_url("https://example.com")
    ::build()

// Builder::new()::build()  // ERROR: no build() on Builder<EmptyState>
```

### 2.15.2 Effect-Indexed Computation Builders

Computation builders track effects through type-level effect sets.

**Effect builder type:**

```cursive
record Computation<T, ε> {
    run: () → T ! ε
}

// Pure computation
function pure<T>(value: T): Computation<T, ∅> {
    result Computation { run: || value }
}

// Computation with effects
function with_effect<T, ε>(f: () → T ! ε): Computation<T, ε> {
    result Computation { run: f }
}

// Composition preserves effect bounds
function Computation::flat_map<T, U, ε₁, ε₂>(
    self: Computation<T, ε₁>,
    f: (T) → Computation<U, ε₂>
): Computation<U, ε₁ ∪ ε₂>
{
    result Computation {
        run: || {
            let value = (self.run)()  // Effect ε₁
            let comp = f(value)
            (comp.run)()  // Effect ε₂
        }
    }
}

// Example: Effect tracking through composition
let io_comp = with_effect(|| read_file("/data.txt"))  // Computation<string, {fs.read}>
let net_comp = io_comp::flat_map(|data| {
    with_effect(|| send_http(data))  // Computation<Response, {net.write}>
})
// Type: Computation<Response, {fs.read, net.write}>
```

### 2.15.3 Permission-Polymorphic Generic Containers

Containers that abstract over permission levels.

**Permission-polymorphic Vec:**

```cursive
record FlexVec<T, perm> {
    data: perm [T]
    len: usize
}

// Works with any permission
procedure FlexVec::get<T, perm>(
    self: FlexVec<T, perm>,
    index: usize
): Option<perm T>
{
    if index < self.len {
        Option::Some(self.data[index])
    } else {
        Option::None
    }
}

// Requires mut permission
procedure FlexVec::push<T>(
    self: mut FlexVec<T, mut>,
    value: T
)
    uses alloc.heap
{
    // Can only call on FlexVec<T, mut>
    self.data::push(value)
    self.len += 1
}

// Usage with different permissions
let owned_vec: FlexVec<i32, own> = FlexVec::new()
let mut_vec: FlexVec<i32, mut> = owned_vec  // Coercion
let imm_vec: FlexVec<i32, imm> = mut_vec  // Coercion

owned_vec::get(0)  // Returns Option<own i32>
mut_vec::get(0)    // Returns Option<mut i32>
imm_vec::get(0)    // Returns Option<imm i32>
```

### 2.15.4 Region-Polymorphic Data Structures

Data structures parameterized by region lifetimes.

**Region-aware linked list:**

```cursive
record Node<'r, T> {
    value: T
    next: Option<*mut Node<'r, T>@'r>
}

record LinkedList<'r, T> {
    head: Option<*mut Node<'r, T>@'r>
    region: 'r
}

procedure LinkedList::'r::new(region: 'r): LinkedList<'r, T> {
    result LinkedList { head: Option::None, region }
}

procedure LinkedList::'r::push<T>(
    self: mut LinkedList<'r, T>,
    value: T
)
    uses alloc.region
{
    let node = alloc_in<'r>(Node {
        value,
        next: self.head
    })
    self.head = Option::Some(node)
}

// Usage: All nodes deallocated when region closes
region<'local> temp {
    var list = LinkedList::new<'local>(temp)
    list::push(1)
    list::push(2)
    list::push(3)
    // All nodes allocated in 'local region
}  // Entire list deallocated here
```

### 2.15.5 Dependent-Type-Like Patterns

Approximating dependent types with refinement types and contracts.

**Length-indexed vectors:**

```cursive
record SizedVec<T, const N: usize> {
    data: [T; N]
}

where N > 0  // Compile-time constraint

procedure SizedVec::new<T, const N: usize>(): SizedVec<T, N>
where N > 0
{
    result SizedVec { data: [T::default(); N] }
}

procedure SizedVec::get<T, const N: usize>(
    self: SizedVec<T, N>,
    index: usize
): T
    must index < N
{
    result self.data[index]
}

// Type-level arithmetic
procedure SizedVec::concat<T, const N: usize, const M: usize>(
    self: SizedVec<T, N>,
    other: SizedVec<T, M>
): SizedVec<T, N + M>
{
    var result = SizedVec::new<T, N + M>()
    loop i in 0..N {
        result.data[i] = self.data[i]
    }
    loop i in 0..M {
        result.data[N + i] = other.data[i]
    }
    result result
}

// Usage
let v1: SizedVec<i32, 3> = SizedVec::new()
let v2: SizedVec<i32, 5> = SizedVec::new()
let v3: SizedVec<i32, 8> = v1::concat(v2)  // Type: SizedVec<i32, 3+5=8>
```

### 2.15.6 Witness Pattern for Capabilities

Using witnesses to encode capabilities at the type level.

**Capability witnesses:**

```cursive
record CanRead { }
record CanWrite { }
record CanExecute { }

record SecureFile<Caps> {
    path: string
    capabilities: Caps
}

procedure SecureFile::read<Caps>(
    self: SecureFile<Caps>,
    witness: CanRead
): Result<string, Error>
    uses fs.read
where Caps: Contains<CanRead>
{
    // Type system ensures CanRead capability exists
    result read_file(self.path)
}

procedure SecureFile::write<Caps>(
    self: SecureFile<Caps>,
    data: string,
    witness: CanWrite
): Result<(), Error>
    uses fs.write
where Caps: Contains<CanWrite>
{
    // Type system ensures CanWrite capability exists
    write_file(self.path, data)
}

// Usage: Capabilities enforced at type level
let readonly_file: SecureFile<(CanRead)> = SecureFile::open("data.txt", CanRead)
readonly_file::read(CanRead)  // ✓ Works

// readonly_file::write("text", CanWrite)  // ERROR: no CanWrite capability

let readwrite_file: SecureFile<(CanRead, CanWrite)> =
    SecureFile::open("config.txt", (CanRead, CanWrite))
readwrite_file::read(CanRead)     // ✓ Works
readwrite_file::write("text", CanWrite)  // ✓ Works
```

---

CITE: Part VII §7.2 — Behavioral Contracts; Part VII §7.2.12 — Runtime Polymorphism; Part IV — Lexical Permission System; Part X — Modals; Part VIII — Regions.

---




**Previous**: [Foundations](01_Foundations.md) | **Next**: [Declarations and Scope](03_Declarations-and-Scope.md)
