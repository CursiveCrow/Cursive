# Part II: Type System - §2. Type Foundations

**Section**: §2 | **Part**: Type System (Part II)
**Next**: [§2.1 Primitive Types](#21-primitive-types)

---

Canonical Update (2025-10-30): The language now uses a single modal `string` type with states `string@Owned` (owned, growable) and `string@View` (read-only view). Legacy names remain as deprecated aliases:

- `type String = string@Owned`  // DEPRECATED alias
- `type str = string@View`      // DEPRECATED alias

Rules: parameters written as `string` mean `string@View`; return types and stored fields written as `string` mean `string@Owned`. Owned → View is an implicit, zero-cost coercion; View → Owned requires allocation and MUST be covered by `uses alloc.heap`.

**Definition 2.1 (Type System):** The Cantrip type system is a static, nominal framework with parametric polymorphism, subtyping, and modal state types. The system comprises the universe of types **T**, the typing judgment `Γ ⊢ e : τ`, the subtyping relation `<:`, and the well-formedness judgment `Γ ⊢ τ : Type`. The system MUST deliver memory safety, forbid undefined behavior, and preserve zero-cost abstractions through compile-time verification and monomorphization.

## 2.0 Type Foundations

### 2.0.1 Scope

This section specifies the core typing infrastructure shared by every later subsection. It defines type formation, well-formedness, subtyping, equivalence, and the interfaces between types, the Lexical Permission System (LPS), the effect system, and contracts.

### 2.0.2 Design Goals

1. **Safety:** A well-typed configuration MUST NOT trigger undefined behavior.
2. **Local reasoning:** Permissions, regions, and effects MUST make aliasing and mutation explicit at every use site.
3. **Performance:** Type analysis MUST complete at compile time and MUST NOT introduce runtime tagging or indirect dispatch beyond constructs the program explicitly uses.
4. **Composability:** Product, sum, modal, and parametric types MUST compose without implicit coercions that disguise capability requirements.
5. **Auditability:** Every type rule MUST expose the permissions and effects it consumes so that verification tools can reason mechanically about programs.

### 2.0.3 Cross-Part Dependencies

- **Part III (Permissions):** Every typing judgment is parameterized by a permission context. §2.0.8.1 summarizes the invariants required by Chapter 2.
- **Part IV (Effects):** Function and procedure types carry explicit `uses` clauses. §2.0.8.2 restates the linking requirements.
- **Part V (Contracts):** Pre- and postconditions appear in type signatures via `must` and `will`. §2.0.8.3 defines the shared notations, including `old(e)`.
- **Part VI (Regions):** Region blocks determine allocation and deallocation order. §2.0.8.4 records the constraints enforced at type-check time.

### 2.0.4 Notational Conventions

Metavariables remain consistent throughout Chapter 2.

```
τ, σ, ρ           Types
α, β, γ           Type variables
κ                 Kinds
e, e₁, e₂         Expressions
v, v₁, v₂         Values
x, y, z           Term variables
Γ                 Typing contexts
ε                 Effect sets
ϕ                 Contract predicates
σ_store           Stores
```

Configurations are written `⟨e, σ_store⟩`. Rule names follow the pattern `Rule-Name` and reference the constructors listed in §2.0.5.1.

### 2.0.5 Syntax

#### 2.0.5.1 Type Grammar

```ebnf
Type ::= PrimType
       | TupleType
       | ArrayType
       | SliceType
       | RecordName
       | EnumName
       | ModalName "@" StateName
       | GenericApp
       | PolyType
       | MapType
       | RawPtrType
       | TypeVar

PrimType   ::= "i8" | "i16" | "i32" | "i64" | "isize"
             | "u8" | "u16" | "u32" | "u64" | "usize"
             | "f32" | "f64" | "bool" | "char"
             | "str" | "String" | "!" | "()"

TupleType  ::= "(" Type ("," Type)* ")"               // arity ≥ 2
ArrayType  ::= "[" Type ";" Nat "]"
SliceType  ::= "[" Type "]"
GenericApp ::= TypeName "<" TypeArg ("," TypeArg)* ">"
PolyType   ::= "∀" TypeParam+ "." Type
MapType    ::= "map" "(" TypeList? ")" EffectAnnotation? "→" Type
RawPtrType ::= "*" "mut"? Type

TypeArg    ::= Type | ConstExpr
TypeParam  ::= TypeVar | (TypeVar ":" Bound)
TypeList   ::= Type ("," Type)*
EffectAnnotation ::= "{" EffectSet "}"

Bound      ::= TraitName | Bound "+" TraitName
EffectSet  ::= EffectName ("+" EffectName)*
```

Constructors such as `record`, `enum`, and `modal` are defined in the dedicated sections beginning at §2.1. This subsection references their names without re-specifying concrete syntax.

#### 2.0.5.2 Kind Grammar

```ebnf
Kind ::= "Type"
       | Kind "→" Kind
```

A constructor of arity _n_ has kind `Type → … → Type` (with _n_ occurrences of `Type`). Ground types carry kind `Type`.

#### 2.0.5.3 Typing Contexts and Judgments

**Definition 2.2 (Typing Context):**

```text
Γ ::= ∅
    | Γ, x : τ
    | Γ, α : κ
    | Γ, α : (κ where α : Bound)
```

`Γ` MUST assign each term variable at most one type and each type variable at most one kind-and-bound pair. Shadowing is the only supported form of rebinding.

**Definition 2.3 (Typing Judgment):** `Γ ⊢ e : τ` denotes that expression `e` has type `τ` in context `Γ`. Auxiliary judgments include:

```
Γ ⊢ τ : Type               τ is well-formed
Γ ⊢ τ₁ <: τ₂               τ₁ is a subtype of τ₂
Γ ⊢ τ₁ ≡ τ₂                τ₁ and τ₂ are equivalent
Γ ⊢ τ : Bound              τ satisfies every trait in Bound
⟨e, σ_store⟩ ⇓ ⟨v, σ_store'⟩   e evaluates to v
```

### 2.0.6 Static Semantics

#### 2.0.6.1 Subtyping

**Definition 2.4 (Subtyping):** The relation `<:` is the smallest preorder satisfying the rules below. No additional coercions are implied.

```
[Sub-Refl]
──────────
τ <: τ

[Sub-Trans]
τ₁ <: τ₂    τ₂ <: τ₃
────────────────────
τ₁ <: τ₃

[Sub-Never]
──────────
! <: τ

[Sub-Array-Slice]
───────────────
[T; n] <: [T]

[Sub-Fun]
σ₁ <: τ₁    τ₂ <: σ₂    ε₁ ⊆ ε₂
────────────────────────────────
(τ₁ →{ε₁} τ₂) <: (σ₁ →{ε₂} σ₂)
```

All generic parameters are invariant unless a later subsection states otherwise. Arrays, slices, tuples, records, enums, and modals MUST remain invariant in their element types to preserve LPS guarantees.

#### 2.0.6.2 Type Equivalence

**Definition 2.5 (Type Equivalence):** Types `τ₁` and `τ₂` are equivalent (`τ₁ ≡ τ₂`) when they denote the same runtime representation after expanding aliases, substituting generic arguments, and normalizing associated types.

```
[Equiv-Refl]
────────
τ ≡ τ

[Equiv-Sym]
τ₁ ≡ τ₂
────────
τ₂ ≡ τ₁

[Equiv-Trans]
τ₁ ≡ τ₂    τ₂ ≡ τ₃
──────────────────
τ₁ ≡ τ₃

[Equiv-Alias]
type A = τ
──────────
A ≡ τ

[Equiv-Generic-Alias]
type F<α> = τ[α]
──────────────────
F<σ> ≡ τ[α ↦ σ]
```

Nominal types (records, enums, modals, trait objects) MUST remain distinct even if their structure is identical. Structural types (tuples, arrays, primitives) MAY be equated only when every component matches exactly.

#### 2.0.6.3 Well-Formedness

**Definition 2.6 (Well-Formed Type):** `Γ ⊢ τ : Type` holds when every constructor and parameter of `τ` is defined, arguments match arity, and all bounds are satisfied.

```
[WF-TypeVar]
(α : κ) ∈ Γ
──────────────
Γ ⊢ α : Type

[WF-Prim]
──────────────
Γ ⊢ i32 : Type

[WF-Tuple]
Γ ⊢ τ₁ : Type … Γ ⊢ τₙ : Type
───────────────────────────────
Γ ⊢ (τ₁, …, τₙ) : Type

[WF-Array]
Γ ⊢ τ : Type    n ∈ ℕ⁺
────────────────────────
Γ ⊢ [τ; n] : Type

[WF-Slice]
Γ ⊢ τ : Type
──────────────
Γ ⊢ [τ] : Type

[WF-Fun]
Γ ⊢ τᵢ : Type    Γ ⊢ τ_r : Type    ε well-formed
─────────────────────────────────────────────────
Γ ⊢ fn(τ₁, …, τₙ) →{ε} τ_r : Type
```

Rules for records, enums, modals, generic parameters, and trait bounds appear in their dedicated sections and MUST be imported when introducing those constructors.

#### 2.0.6.4 Type Checking

Cantrip employs bidirectional checking. The algorithmic judgments are `synth(Γ, e) = τ` (synthesis) and `check(Γ, e, τ)` (checking). The checking judgment MUST first synthesize a type and then verify subtyping:

```text
check(Γ, e, τ):
    let τ' = synth(Γ, e)
    if Γ ⊢ τ' <: τ then
        accept
    else
        reject with TypeError{expected = τ, found = τ'}
```

Literal typing defaults are `i32` for integers and `f64` for floating-point values. A suffix or an explicit annotation overrides the default.

#### 2.0.6.5 Type Inference

**Definition 2.7 (Constraint Set):**

```
C ::= τ₁ = τ₂
    | τ₁ <: τ₂
    | τ : Bound
    | ∃α. C
    | C₁ ∧ C₂
```

Constraint solving proceeds by repeated normalization:

```
solve(∅) = ∅
solve({τ = τ} ∪ C) = solve(C)
solve({α = τ} ∪ C) = [α ↦ τ] ∪ solve(C[α ↦ τ])      provided α ∉ τ
solve({T<τ₁> = T<τ₂>} ∪ C) = solve({τ₁ = τ₂} ∪ C)
solve({τ : Bound} ∪ C) = (Γ ⊢ τ : Bound) ∧ solve(C)
```

Occurs-check failure results in a static error. Generalization introduces `∀α` only when `α` is not free in Γ and the enclosing permission context admits polymorphism.

### 2.0.7 Dynamic Semantics

**Definition 2.8 (Evaluation Judgment):** `⟨e, σ_store⟩ ⇓ ⟨v, σ_store'⟩` denotes multi-step evaluation. Cantrip leaves the determinism of evaluation unspecified, provided every reduction respects the typing rules.

- **Theorem 2.9 (Progress):** If `∅ ⊢ e : τ`, then `e` is a value or can reduce.
- **Theorem 2.10 (Preservation):** If `Γ ⊢ e : τ` and `⟨e, σ_store⟩ → ⟨e', σ_store'⟩`, then `Γ ⊢ e' : τ`.
- **Corollary 2.11 (Type Safety):** Combining progress and preservation yields `∅ ⊢ v : τ` for every terminating evaluation of `e`.
- **Theorem 2.12 (Parametricity):** Polymorphic functions behave uniformly with respect to their quantified parameters.

### 2.0.8 Inter-System Integration

#### 2.0.8.1 Permissions

Every type is viewed through a permission wrapper `perm T` with `perm ∈ {own, mut, imm}`.

- `own T` grants exclusive transferability; moving the value consumes the previous binding.
- `mut T` grants exclusive borrow without ownership transfer; the borrow MUST remain within its lexical region.
- `imm T` grants shared, read-only access; mutation through `imm` is rejected unless mediated by a modal transition that consumes the permission.

Permission casts MUST be explicit (`move`, `mut`, or constructor-specific operations). Implicit reborrowing is prohibited.

#### 2.0.8.2 Effects

Function and procedure types carry an effect signature `uses ε`. A call site MUST guarantee `ε_call ⊇ ε_def`. Effect tokens include capabilities such as `io.print` and `unsafe.bitcopy`.

Deprecation note (normative alignment): Historical examples may show map types annotated with `→{ε}`. The canonical type‑level form is `! ε`, with declaration‑level `uses ε`. Both are equivalent by law; new text and tools MUST use `! ε`. CITE: §1.2.5 — Equivalence Law; CITE: §2.0.6.1 — Subtyping.

#### 2.0.8.3 Contracts

Contracts attach `must` (preconditions) and `will` (postconditions) to function signatures. The operator `old(e)` captures the value of `e` at call entry. Contract checks occur at the boundary of the function; violations MUST raise a contract failure before any undefined behavior occurs.

#### 2.0.8.4 Regions

Region blocks `{ region r { ... } }` allocate stack disciplines. Values created within a region MUST NOT escape past the closing brace. Deallocation follows LIFO order.

#### 2.0.8.5 Modal Types

Modal types `ModalName@State` encode state machines. Transitions consume a permission to one state and yield a permission to the successor state. Linear transfer MUST be explicit and is verified at compile time.

### 2.0.9 Value Capabilities

#### 2.0.9.1 Copy Trait

**Definition 2.13 (Copy Trait):**

```cantrip
trait Copy {
    procedure copy(self: Self): Self
        uses unsafe.bitcopy;
        must self.is_copyable();
        will result.is_bitwise_clone_of(self);
}
```

`unsafe.bitcopy` is an effect token meaning “invoke the trusted bitwise copy primitive.” Only the standard library MAY implement this effect. A type T automatically satisfies `Copy` when all its fields are `Copy` and T declares no destructor. Automatic derivation MUST be stated explicitly for each constructor family in §2.1–§2.6.

Copy-eligible types MAY be duplicated explicitly via `.copy()`. Passing parameters does not duplicate values; permissions remain bound to the callee unless `.copy()` or another explicit duplicating operation is invoked.

#### 2.0.9.2 Move Semantics

All values support `move` to transfer ownership into a callee or a new binding. After a move, the previous binding MUST NOT be used. Moves are implicit for `own` permissions when a binding is passed by value.

### 2.0.10 Type Taxonomy

| Category                 | Examples                            | Section |
| ------------------------ | ----------------------------------- | ------- |
| Primitive types          | Integers, floats, bool, char, str   | §2.1    |
| Product types            | Tuples, records                     | §2.2    |
| Sum types                | Enums, modals                       | §2.3    |
| Collection types         | Arrays, slices                      | §2.4    |
| Reference types          | Safe pointers, raw pointers         | §2.5    |
| Abstraction types        | Traits, function types              | §2.6    |
| Parametric constructions | Generics, associated types, aliases | §2.7    |
| Utility markers          | Copy, Sized, Send, Sync             | §2.8    |

### 2.0.11 Specification Status

| Section | Topic                       | Status   | Notes                                                       |
| ------- | --------------------------- | -------- | ----------------------------------------------------------- |
| §2.1    | Primitive type forms        | Complete | Covers syntax, typing, layout.                              |
| §2.2    | Product types               | Partial  | Tuple-struct rules pending (ISSUE TS-1).                    |
| §2.3    | Sum and modal types         | Partial  | Modal constructors & algorithms pending (ISSUE TS-6).       |
| §2.4    | Collections                 | Partial  | Mutable slice invariants under review (ISSUE TS-2).         |
| §2.5    | Reference types             | Complete | Safe vs. raw pointers fully specified.                      |
| §2.6    | Abstraction types           | Partial  | Object safety rules outstanding (ISSUE TS-7).               |
| §2.7    | Parametric features         | Partial  | Lifetime & associated bounds unspecified (ISSUE TS-3/TS-8). |
| §2.8    | Marker traits and utilities | Missing  | Send/Sync/Sized notation to be added (ISSUE TS-4).          |

Percent complete: 37 % (fully specified subsections ÷ eight major categories). This value MUST be updated whenever a subsection changes status.

### 2.0.12 Reading Guide

- Start with §2.1 for primitive value semantics.
- Continue to §2.2 and §2.3 to understand product and sum construction.
- Consult §2.5 and §2.6 when reasoning about pointers, traits, and higher-order values.
- Refer to §2.7 before authoring generic APIs.

### 2.0.13 Outstanding Issues

**ISSUE TS-1:** Tuple-struct constructor rules in §2.2 remain unspecified. Authoritative rule text MUST define constructor syntax and initialization before stabilization.

**ISSUE TS-2:** Mutable slice aliasing guarantees in §2.4 require proofs integrated with the LPS. The chapter presently references a draft design; the final version MUST provide static and dynamic rules.

**ISSUE TS-3:** Lifetime-like parameters for generics (§2.7) are not codified. The specification MUST either formalize region parameters in type signatures or document why the feature is postponed.

**ISSUE TS-4:** Marker traits Send, Sync, and Sized (§2.8) are placeholders. Until specified, no code MAY assume auto-implementation rules for these markers.

**ISSUE TS-5:** Section-specific example suites referenced throughout §2.1–§2.10 are unavailable. The examples remain out-of-scope until the Example Compendium is authored.

**ISSUE TS-6:** Exhaustiveness-check and modal-verification algorithms (§2.3) remain unspecified. The specification MUST document these algorithms before sum types are declared stable.

**ISSUE TS-7:** Object safety rules for traits (§2.6) are not yet codified. The specification MUST define the admissible trait methods and receivers before trait objects are stabilized.

**ISSUE TS-8:** Associated type bounds for generics (§2.7) lack formal rules. The specification MUST provide syntax and typing rules before dependent code may rely on them.

**ISSUE TS-9:** Closure capture and effect-checking algorithms for map types (§2.8) remain undefined. The specification MUST describe capture typing and effect propagation prior to stabilization.

## 2.1 Primitive Types

This section specifies Cantrip's primitive types, which form the foundation of the type system. Primitive types are built into the language and cannot be defined by users.

### 2.1.0 Overview

#### 2.1.0.1 Scope

Section §2.1 governs the syntax, typing, and runtime behaviour of primitive scalar and text types. Detailed rules for each primitive family remain in §§2.1.1–2.1.6; the present subsection summarizes their shared guarantees.

#### 2.1.0.2 Dependencies and Notation

Primitive types rely on the Lexical Permission System (Part III) for move semantics and on the effect taxonomy of Part IV to describe operations such as `alloc.heap` used by owned strings. Metavariables follow §2.0.4.

#### 2.1.0.3 Guarantees

Primitive values MUST preserve memory safety, respect LPS permissions, and honour the overflow semantics codified in §§2.1.1–2.1.2. String and slice primitives MUST maintain UTF-8 validity and bounds invariants.

### 2.1.0.4 Syntax Summary

The grammar in §§2.1.1–2.1.6 defines concrete tokens (numeric bases, character literals, string delimiters). No additional derived forms exist beyond those sections; outstanding variants MUST be logged under ISSUE TS-5.

### 2.1.0.5 Static Semantics Summary

Typing rules for literals, conversion behaviour, and trait memberships are specified per primitive family. This chapter relies on those rules without alteration.

### 2.1.0.6 Dynamic Semantics Summary

Overflow checks, NaN propagation, and runtime panics are described in the family subsections. No extra dynamic rules are introduced here.

### 2.1.0.7 Algorithms

Out-of-scope. Literal inference algorithms are covered inline in §§2.1.1–2.1.2.

### 2.1.0.8 Proven Properties

Primitive types participate in the global Progress and Preservation results (§2.0.7). No additional metatheorems are required.

### 2.1.0.9 Integration Notes

Permissions default to `imm` bindings unless a procedure explicitly requests `mut` or `own`. String operations requiring `alloc.heap` MUST declare the effect tokens enumerated in §2.1.6.

### 2.1.0.10 Notes and Issues

Example suites for primitives remain tracked under ISSUE TS-5.

**Primitive type categories:**

- **Integer types** (§2.1.1): Fixed-width signed and unsigned integers
- **Floating-point types** (§2.1.2): IEEE 754 binary floating-point numbers
- **Boolean type** (§2.1.3): Two-valued logic (`true`, `false`)
- **Character type** (§2.1.4): Unicode scalar values
- **Never type** (§2.1.5): Bottom type for diverging computations
- **String types** (§2.1.6): UTF-8 encoded text

---

## 2.1.1 Integer Types

##### 2.1.1.1 Overview

**Key innovation/purpose:** Explicit integer sizes enable precise control over memory layout, numeric ranges, and FFI interoperability, essential for systems programming.

##### Guidance (non-normative)

- Prefer `usize`/`isize` for indexing and pointer offsets; rely on the overflow rules in §2.1.1.4 for arithmetic safety.
- Use explicit suffixes or contextual annotations when the default `i32` literal type is unsuitable.
- For fractional or arbitrary-precision needs, select the floating-point or big-integer facilities defined outside §2.1.

##### 2.1.1.2 Syntax

#### Concrete Syntax

**Type syntax:**

```ebnf
IntType     ::= SignedInt | UnsignedInt
SignedInt   ::= "i8" | "i16" | "i32" | "i64" | "isize"
UnsignedInt ::= "u8" | "u16" | "u32" | "u64" | "usize"
```

**Literal syntax:**

```ebnf
IntLiteral  ::= DecLiteral TypeSuffix?
              | HexLiteral TypeSuffix?
              | OctLiteral TypeSuffix?
              | BinLiteral TypeSuffix?

DecLiteral  ::= Digit (Digit | "_")*
HexLiteral  ::= "0x" HexDigit (HexDigit | "_")*
OctLiteral  ::= "0o" OctDigit (OctDigit | "_")*
BinLiteral  ::= "0b" BinDigit (BinDigit | "_")*

TypeSuffix  ::= "i8" | "i16" | "i32" | "i64" | "isize"
              | "u8" | "u16" | "u32" | "u64" | "usize"

Digit       ::= [0-9]
HexDigit    ::= [0-9a-fA-F]
OctDigit    ::= [0-7]
BinDigit    ::= [0-1]
```

**Examples:**

```cantrip
// Decimal literals
let a: i32 = 42
let b: u64 = 1_000_000    // Underscores for readability
let c = 42i8              // Type suffix

// Hexadecimal
let hex: u32 = 0xDEAD_BEEF

// Octal
let oct: u16 = 0o755

// Binary
let bin: u8 = 0b1010_1010
```

#### Abstract Syntax

**Types:**

```
τ ::= i8 | i16 | i32 | i64 | isize      (signed integers)
    | u8 | u16 | u32 | u64 | usize      (unsigned integers)
```

**Values:**

```
v ::= n    (integer value)
```

##### 2.1.1.3 Static Semantics

###### Well-Formedness Rules

```
[WF-Int-Type]
T ∈ {i8, i16, i32, i64, isize, u8, u16, u32, u64, usize}
────────────────────────────────────────────────────────
Γ ⊢ T : Type
```

#### Type Rules

**Integer literal typing:**

```
[T-Int-Literal-Default]
n is an integer literal without type suffix
────────────────────────────────────────
Γ ⊢ n : i32    (default type)

[T-Int-Literal-Suffix]
n is an integer literal with suffix T
T ∈ IntTypes
─────────────────────────────────────────
Γ ⊢ n : T    (explicit type from suffix)

[T-Int-Literal-Context]
n is an integer literal without type suffix
Γ ⊢ context must type T
T ∈ IntTypes
n ∈ ⟦T⟧    (n fits in range of T)
─────────────────────────────────────────
Γ ⊢ n : T    (type from context)
```

**Type inference priority:**

1. Explicit suffix (highest priority): `42u64` → `u64`
2. Context from annotation: `let x: u8 = 42` → `u8`
3. Default type: `let x = 42` → `i32` (lowest priority)

**Examples:**

```cantrip
let x = 42           // Type: i32 (default)
let y: u8 = 42       // Type: u8 (context)
let z = 42u64        // Type: u64 (suffix)
let w: i8 = 200      // ERROR E5101: 200 doesn't fit in i8 range [-128, 127]
```

#### Type Properties

**Theorem 5.1.1 (Value Sets):**

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

**Theorem 5.1.2 (Copy Capability):**

All integer types implement the `Copy` trait:

```
∀ T ∈ IntTypes. T : Copy
```

**Semantics:**

- Integers pass by permission (reference-like) by default
- The `.copy()` method creates an explicit duplicate when needed
- Small size (1-8 bytes) makes copying cheap when explicitly requested
- Assignment creates permission bindings, not copies

**Example:**

```cantrip
let x: i32 = 42
let y = x           // y binds to x's value, both usable (permission binding)
let z = x.copy()    // Explicit copy creates new value

procedure double(n: i32): i32 {
    n * 2  // Access through permission, no copy
}

let result = double(x)  // x passed by permission, still usable after
```

**Theorem 5.1.3 (Platform-Dependent Sizes):**

```
32-bit systems: isize = i32, usize = u32, n = 32
64-bit systems: isize = i64, usize = u64, n = 64
```

**Corollary:** Code using `isize`/`usize` may have different behavior on different architectures.

##### 2.1.1.4 Dynamic Semantics

#### Evaluation Rules

**Literal evaluation:**

```
[E-Int]
────────────────
⟨n, σ⟩ ⇓ ⟨n, σ⟩
```

Integer literals evaluate to themselves without state change.

**Arithmetic operations:**

```
[E-Add]
⟨e₁, σ⟩ ⇓ ⟨n₁, σ₁⟩    ⟨e₂, σ₁⟩ ⇓ ⟨n₂, σ₂⟩
─────────────────────────────────────────
⟨e₁ + e₂, σ⟩ ⇓ ⟨n₁ ⊕ n₂, σ₂⟩

where ⊕ is:
  - wrapping addition (modulo 2^bits) in release mode
  - checked addition (panics on overflow) in debug mode

[E-Sub]
⟨e₁, σ⟩ ⇓ ⟨n₁, σ₁⟩    ⟨e₂, σ₁⟩ ⇓ ⟨n₂, σ₂⟩
─────────────────────────────────────────
⟨e₁ - e₂, σ⟩ ⇓ ⟨n₁ ⊖ n₂, σ₂⟩

[E-Mul]
⟨e₁, σ⟩ ⇓ ⟨n₁, σ₁⟩    ⟨e₂, σ₁⟩ ⇓ ⟨n₂, σ₂⟩
─────────────────────────────────────────
⟨e₁ * e₂, σ⟩ ⇓ ⟨n₁ ⊗ n₂, σ₂⟩

[E-Div]
⟨e₁, σ⟩ ⇓ ⟨n₁, σ₁⟩    ⟨e₂, σ₁⟩ ⇓ ⟨n₂, σ₂⟩
n₂ ≠ 0
─────────────────────────────────────────
⟨e₁ / e₂, σ⟩ ⇓ ⟨n₁ ÷ n₂, σ₂⟩

[E-Div-Zero]
⟨e₁, σ⟩ ⇓ ⟨n₁, σ₁⟩    ⟨e₂, σ₁⟩ ⇓ ⟨0, σ₂⟩
─────────────────────────────────────────
⟨e₁ / e₂, σ⟩ ⇓ panic("division by zero")
```

#### Memory Representation

**Integer layout:**

Integers are stored in two's complement representation (signed) or unsigned binary (unsigned) with platform-native byte order (typically little-endian on x86-64, ARM64).

```
i8/u8 memory layout (1 byte):
┌────┐
│ b₀ │
└────┘
Alignment: 1 byte

i16/u16 memory layout (2 bytes):
┌────┬────┐
│ b₀ │ b₁ │
└────┴────┘
Alignment: 2 bytes

i32/u32 memory layout (4 bytes):
┌────┬────┬────┬────┐
│ b₀ │ b₁ │ b₂ │ b₃ │
└────┴────┴────┴────┘
Alignment: 4 bytes

i64/u64 memory layout (8 bytes):
┌────┬────┬────┬────┬────┬────┬────┬────┐
│ b₀ │ b₁ │ b₂ │ b₃ │ b₄ │ b₅ │ b₆ │ b₇ │
└────┴────┴────┴────┴────┴────┴────┴────┘
Alignment: 8 bytes
```

**Two's complement representation (signed):**

For signed integer n of bit-width w:

```
If n ≥ 0: binary representation of n
If n < 0: binary representation of 2^w + n

Example (i8):
  127 → 0x7F → 0111_1111
    0 → 0x00 → 0000_0000
   -1 → 0xFF → 1111_1111
 -128 → 0x80 → 1000_0000
```

**Byte order:**

Cantrip follows the platform's native byte order:

- **Little-endian** (x86, x86-64, ARM64): Least significant byte first
- **Big-endian** (some RISC architectures): Most significant byte first

```
Value: 0x12345678 (i32)

Little-endian memory layout:
Address: [0]  [1]  [2]  [3]
Value:   0x78 0x56 0x34 0x12

Big-endian memory layout:
Address: [0]  [1]  [2]  [3]
Value:   0x12 0x34 0x56 0x78
```

#### Operational Behavior

**Integer overflow:**

**Axiom 5.1.1 (Integer Arithmetic):**

For integer type T with min M and max N:

```
Debug mode (checked):
  x ⊕ y ∉ [M, N] ⟹ panic("arithmetic overflow")

Release mode (wrapping):
  result = (x ⊕ y) mod (N - M + 1)
  result ∈ [M, N]
```

**Examples:**

```cantrip
// u8: range [0, 255]
let x: u8 = 255
let y = x + 1
// Debug: panic!("attempt to add with overflow")
// Release: y = 0 (wrapping: 256 mod 256 = 0)

// i8: range [-128, 127]
let a: i8 = 127
let b = a + 1
// Debug: panic!
// Release: b = -128 (two's complement wrap)
```

**Attribute control:**

Override default overflow behavior with attributes:

```cantrip
#[overflow_checks(true)]
function critical_math(a: i32, b: i32): i32 {
    a * b  // Always checked, even in release mode
}

#[overflow_checks(false)]
function fast_hash(x: u64): u64 {
    x.wrapping_mul(0x517cc1b727220a95)  // Never checked
}
```

**Explicit overflow handling:**

Cantrip provides methods for explicit overflow control:

```cantrip
// Checked arithmetic (returns Option)
let x: u32 = 100
let y: u32 = 200
let sum = x.checked_add(y)?  // Some(300)

let max: u32 = u32::MAX
let overflow = max.checked_add(1)  // None

// Saturating arithmetic (clamps at limits)
let sat_sum = max.saturating_add(1)  // u32::MAX

// Wrapping arithmetic (explicit wrap-around)
let wrap_sum = max.wrapping_add(1)  // 0

// Overflowing arithmetic (returns tuple with overflow flag)
let (result, overflowed) = max.overflowing_add(1)
// result = 0, overflowed = true
```

**Division and modulo:**

```cantrip
// Division by zero: always panics (both debug and release)
let x = 10 / 0  // panic!("attempt to divide by zero")

// Integer division truncates toward zero
assert!(7 / 2 == 3)
assert!(-7 / 2 == -3)

// Modulo has sign of left operand
assert!(7 % 3 == 1)
assert!(-7 % 3 == -1)
assert!(7 % -3 == 1)
assert!(-7 % -3 == -1)

// Special case: MIN / -1 can overflow
let min: i8 = i8::MIN  // -128
let result = min / -1  // Would be 128, which overflows i8
// Debug: panic!("attempt to divide with overflow")
// Release: result = -128 (wraps)
```

---

**Parent**: [§2.1 Primitive Types](#21-primitive-types) | **Next**: [§2.1.2 Floating-Point Types](#212-floating-point-types)

---

## 2.1.2 Floating-Point Types

##### 2.1.2.1 Overview

**Key innovation/purpose:** IEEE 754-compliant floating-point provides standardized, hardware-accelerated real number approximations with well-defined behavior for special values (±∞, NaN).

##### Guidance (non-normative)

- Choose `f64` unless memory or performance requirements justify `f32`; both obey the IEEE 754 rules stated in §2.1.2.4.
- Avoid direct equality comparisons; rely on the tolerance patterns described in §2.1.2.4 for numerical stability.
- For exact decimal work or cryptography, select integer or fixed-point facilities defined outside §2.1.

##### 2.1.2.2 Syntax

#### Concrete Syntax

**Type syntax:**

```ebnf
FloatType ::= "f32" | "f64"
```

**Literal syntax:**

```ebnf
FloatLiteral ::= DecFloatLit TypeSuffix?
               | DecLiteral Exponent TypeSuffix?
               | DecLiteral "." DecLiteral? Exponent? TypeSuffix?
               | "." DecLiteral Exponent? TypeSuffix?

Exponent     ::= ("e" | "E") ("+" | "-")? DecLiteral
TypeSuffix   ::= "f32" | "f64"
```

**Examples:**

```cantrip
// Basic floating-point literals
let pi: f64 = 3.14159
let e: f32 = 2.71828f32

// Scientific notation
let avogadro: f64 = 6.022e23
let planck: f64 = 6.626e-34

// Suffix notation
let x = 1.0f32
let y = 2.5f64

// Underscores for readability
let large: f64 = 299_792_458.0  // Speed of light (m/s)
```

#### Abstract Syntax

**Types:**

```
τ ::= f32 | f64
```

**Values:**

```
v ::= f    (floating-point value, including ±0, ±∞, NaN)
```

##### 2.1.2.3 Static Semantics

#### Well-Formedness Rules

```
[WF-Float]
T ∈ {f32, f64}
──────────────────
Γ ⊢ T : Type
```

#### Type Rules

**Floating-point literal typing:**

```
[T-Float-Literal-Default]
f is a floating-point literal without type suffix
───────────────────────────────────────────────
Γ ⊢ f : f64    (default type)

[T-Float-Literal-Suffix]
f is a floating-point literal with suffix T
T ∈ {f32, f64}
──────────────────────────────────────────
Γ ⊢ f : T    (explicit type from suffix)

[T-Float-Literal-Context]
f is a floating-point literal without type suffix
Γ ⊢ context must type T
T ∈ {f32, f64}
──────────────────────────────────────────
Γ ⊢ f : T    (type from context)
```

**Examples:**

```cantrip
let x = 3.14         // Type: f64 (default)
let y: f32 = 3.14    // Type: f32 (context)
let z = 3.14f32      // Type: f32 (suffix)
```

#### Type Properties

**Theorem 5.2.1 (IEEE 754 Representation):**

Cantrip floating-point types conform to IEEE 754-2008:

| Type  | Format   | Sign bits | Exponent bits | Mantissa bits | Total bits |
| ----- | -------- | --------- | ------------- | ------------- | ---------- |
| `f32` | binary32 | 1         | 8             | 23            | 32         |
| `f64` | binary64 | 1         | 11            | 52            | 64         |

**Theorem 5.2.2 (Value Set):**

```
⟦f32⟧ = IEEE754_single = {
    ±0,                          (positive/negative zero)
    ±∞,                          (positive/negative infinity)
    NaN,                         (not-a-number, many representations)
    normalized values,           (standard representation)
    denormalized values          (gradual underflow)
}

⟦f64⟧ = IEEE754_double (same structure, different precision)
```

**Precision:**

- `f32`: ~7 decimal digits of precision
- `f64`: ~15-17 decimal digits of precision

**Range:**

- `f32`: ±1.18 × 10⁻³⁸ to ±3.40 × 10³⁸
- `f64`: ±2.23 × 10⁻³⁰⁸ to ±1.80 × 10³⁰⁸

##### 2.1.2.4 Dynamic Semantics

#### Evaluation Rules

**Literal evaluation:**

```
[E-Float]
────────────────
⟨f, σ⟩ ⇓ ⟨f, σ⟩
```

**Arithmetic operations:**

```
[E-Float-Add]
⟨e₁, σ⟩ ⇓ ⟨f₁, σ₁⟩    ⟨e₂, σ₁⟩ ⇓ ⟨f₂, σ₂⟩
─────────────────────────────────────────
⟨e₁ + e₂, σ⟩ ⇓ ⟨round(f₁ + f₂), σ₂⟩

where round applies IEEE 754 rounding mode (default: round-to-nearest-even)

[E-Float-Div-Zero]
⟨e₁, σ⟩ ⇓ ⟨f₁, σ₁⟩    ⟨e₂, σ₁⟩ ⇓ ⟨0.0, σ₂⟩
f₁ ≠ 0
─────────────────────────────────────────
⟨e₁ / e₂, σ⟩ ⇓ ⟨±∞, σ₂⟩    (±∞ depending on sign)

[E-Float-NaN]
⟨e₁, σ⟩ ⇓ ⟨NaN, σ₁⟩    ⟨e₂, σ₁⟩ ⇓ ⟨f₂, σ₂⟩
─────────────────────────────────────────
⟨e₁ ⊕ e₂, σ⟩ ⇓ ⟨NaN, σ₂⟩    (NaN propagation)
```

#### Memory Representation

**f32 memory layout (4 bytes):**

```
IEEE 754 single-precision:
┌─┬────────┬───────────────────────┐
│S│Exponent│      Mantissa         │
│ │(8 bits)│      (23 bits)        │
└─┴────────┴───────────────────────┘
Size: 4 bytes, Alignment: 4 bytes

Components:
- Sign bit (S): 1 bit
- Exponent: 8 bits (biased by 127)
- Mantissa: 23 bits (implicit leading 1)
```

**f64 memory layout (8 bytes):**

```
IEEE 754 double-precision:
┌─┬───────────┬────────────────────────────────────────────────┐
│S│ Exponent  │              Mantissa                          │
│ │ (11 bits) │              (52 bits)                         │
└─┴───────────┴────────────────────────────────────────────────┘
Size: 8 bytes, Alignment: 8 bytes

Components:
- Sign bit (S): 1 bit
- Exponent: 11 bits (biased by 1023)
- Mantissa: 52 bits (implicit leading 1)
```

#### Operational Behavior

**Formal properties:**

```
⟦f₁ + f₂⟧ = round(⟦f₁⟧ + ⟦f₂⟧)
⟦f₁ * f₂⟧ = round(⟦f₁⟧ × ⟦f₂⟧)
⟦f₁ / f₂⟧ = round(⟦f₁⟧ / ⟦f₂⟧)

where round is IEEE 754 round-to-nearest-even
```

**Special value arithmetic:**

```
x + ∞ = ∞    (for x ≠ -∞)
x - ∞ = -∞   (for x ≠ ∞)
∞ + ∞ = ∞
∞ - ∞ = NaN
x * ∞ = ∞    (for x > 0)
x * ∞ = -∞   (for x < 0)
0 * ∞ = NaN
x / 0 = ±∞   (for x ≠ 0)
0 / 0 = NaN
∞ / ∞ = NaN
```

**NaN properties:**

```
NaN ≠ NaN        (NaN is not equal to itself)
NaN < x = false  (all comparisons with NaN are false)
NaN ≤ x = false
NaN > x = false
NaN ≥ x = false
NaN == x = false
```

**Signed zeros:**

```
+0.0 == -0.0     (equal for comparison)
1.0 / +0.0 = +∞
1.0 / -0.0 = -∞
```

---

## 2.1.3 Boolean Type

##### 2.1.3.1 Overview

**Key innovation/purpose:** `bool` provides type-safe two-valued logic with short-circuit evaluation for conditional expressions, essential for control flow and predicate logic.

##### Guidance (non-normative)

- Use `bool` strictly for binary flow control (`if`, `loop`, `match` guards); encode multi-state logic with enums.
- Leverage the short-circuit operators from §2.1.3.4 for efficient conditionals.
- Treat boolean parameters as permissions-bound references—copying occurs only when `.copy()` is invoked.

##### 2.1.3.2 Syntax

#### Concrete Syntax

**Type and literal syntax:**

```ebnf
BoolType    ::= "bool"
BoolLiteral ::= "true" | "false"
```

**Examples:**

```cantrip
let flag: bool = true
let is_valid = false

if flag {
    println("Flag is set")
}
```

#### Abstract Syntax

**Type:**

```
τ ::= bool
```

**Values:**

```
v ::= true | false
```

##### 2.1.3.3 Static Semantics

#### Well-Formedness Rules

```
[WF-Bool]
──────────────────
Γ ⊢ bool : Type
```

#### Type Rules

**Boolean literal typing:**

```
[T-Bool-True]
──────────────────
Γ ⊢ true : bool

[T-Bool-False]
──────────────────
Γ ⊢ false : bool
```

**No type inference needed:** Boolean literals always have type `bool` with no context sensitivity or suffixes.

#### Type Properties

**Theorem 5.3.1 (Value Set):**

The boolean type has exactly two values:

```
⟦bool⟧ = {true, false} ≅ {⊤, ⊥} ≅ ℤ₂
```

**Theorem 5.3.2 (Copy Capability):**

```
bool : Copy
```

**Semantics:**

- Booleans pass by permission (reference-like) by default
- The `.copy()` method creates an explicit duplicate when needed
- Single-byte size makes copying trivial when explicitly requested

**Theorem 5.3.3 (Size and Alignment):**

```
size(bool) = 1 byte
align(bool) = 1 byte
```

##### 2.1.3.4 Dynamic Semantics

#### Evaluation Rules

**Literal evaluation:**

```
[E-Bool-True]
────────────────────────
⟨true, σ⟩ ⇓ ⟨true, σ⟩

[E-Bool-False]
────────────────────────
⟨false, σ⟩ ⇓ ⟨false, σ⟩
```

**Logical operations:**

```
[E-And-True]
⟨e₁, σ⟩ ⇓ ⟨true, σ₁⟩    ⟨e₂, σ₁⟩ ⇓ ⟨v₂, σ₂⟩
────────────────────────────────────────
⟨e₁ && e₂, σ⟩ ⇓ ⟨v₂, σ₂⟩

[E-And-False]
⟨e₁, σ⟩ ⇓ ⟨false, σ'⟩
────────────────────────────────────────
⟨e₁ && e₂, σ⟩ ⇓ ⟨false, σ'⟩    (short-circuit, e₂ not evaluated)

[E-Or-False]
⟨e₁, σ⟩ ⇓ ⟨false, σ₁⟩    ⟨e₂, σ₁⟩ ⇓ ⟨v₂, σ₂⟩
────────────────────────────────────────
⟨e₁ || e₂, σ⟩ ⇓ ⟨v₂, σ₂⟩

[E-Or-True]
⟨e₁, σ⟩ ⇓ ⟨true, σ'⟩
────────────────────────────────────────
⟨e₁ || e₂, σ⟩ ⇓ ⟨true, σ'⟩    (short-circuit, e₂ not evaluated)

[E-Not]
⟨e, σ⟩ ⇓ ⟨v, σ'⟩
────────────────────────────────────────
⟨!e, σ⟩ ⇓ ⟨¬v, σ'⟩    (logical negation)
```

#### Memory Representation

**Boolean layout:**

```
bool memory layout (1 byte):
┌────┐
│0/1 │
└────┘
Size: 1 byte
Alignment: 1 byte

Values:
- false = 0x00 (all bits zero)
- true  = 0x01 (least significant bit set, others zero)
```

**Representation guarantee:**

```
⟦false⟧ = 0x00
⟦true⟧  = 0x01
```

**Note:** While only values 0x00 and 0x01 are valid for `bool`, Cantrip guarantees all boolean values are normalized to these representations. Creating a `bool` with any other bit pattern (via unsafe code) is undefined behavior.

#### Operational Behavior

**Boolean algebra:**

Cantrip booleans follow standard Boolean algebra:

```
Logical AND (∧):
⊤ ∧ ⊤ = ⊤
⊤ ∧ ⊥ = ⊥
⊥ ∧ ⊤ = ⊥
⊥ ∧ ⊥ = ⊥

Logical OR (∨):
⊤ ∨ ⊤ = ⊤
⊤ ∨ ⊥ = ⊤
⊥ ∨ ⊤ = ⊤
⊥ ∨ ⊥ = ⊥

Logical NOT (¬):
¬⊤ = ⊥
¬⊥ = ⊤
```

**Short-circuit evaluation:**

```
⊥ ∧ _ = ⊥    (right side not evaluated)
⊤ ∨ _ = ⊤    (right side not evaluated)
```

**Examples:**

```cantrip
// Short-circuit prevents division by zero
let safe = (y != 0) && (x / y > 10)

// Short-circuit prevents null dereference
let result = (ptr != null) && (*ptr == target)
```

**Comparison:**

```
true == true   = true
false == false = true
true == false  = false
true != false  = true
```

---

## 2.1.4 Character Type

##### 2.1.4.1 Overview

**Key innovation/purpose:** `char` provides Unicode-correct character handling by representing Unicode scalar values (not bytes or code units), ensuring text processing correctness for international text.

##### Guidance (non-normative)

- Use `char` for single Unicode scalar values; handle grapheme clusters with the string operations detailed in §2.1.6.
- For raw bytes or ASCII-only data, rely on `u8` and convert explicitly when invoking the typing rules in §2.1.4.3.
- Surrogate ranges remain invalid; use `char::from_u32` and validation helpers to enforce the invariants stated in §2.1.4.3.

##### 2.1.4.2 Syntax

#### Concrete Syntax

**Type and literal syntax:**

```ebnf
CharType    ::= "char"
CharLiteral ::= "'" CharContent "'"

CharContent ::= AnyChar
              | EscapeSeq
              | UnicodeEscape

EscapeSeq   ::= "\\" | "\'" | "\"" | "\n" | "\r" | "\t" | "\0"
UnicodeEscape ::= "\\u{" HexDigit+ "}"
```

**Examples:**

```cantrip
let letter: char = 'A'           // U+0041 (Latin capital A)
let emoji: char = '🚀'            // U+1F680 (Rocket)
let chinese: char = '中'          // U+4E2D (Chinese character)
let null_char: char = '\0'       // U+0000 (null character, valid!)
let tab: char = '\t'             // U+0009 (tab)
let newline: char = '\n'         // U+000A (line feed)
let unicode: char = '\u{1F600}'  // U+1F600 (grinning face emoji)
```

#### Abstract Syntax

**Type:**

```
τ ::= char
```

**Values:**

```
v ::= 'c'    (character literal)
```

##### 2.1.4.3 Static Semantics

###### 2.1.4.3.1 Well-Formedness Rules

```
[WF-Char]
──────────────────
Γ ⊢ char : Type
```

###### 2.1.4.3.2 Type Rules

**Character literal typing:**

```
[T-Char]
c is a valid Unicode scalar value
c ∈ [U+0000, U+D7FF] ∪ [U+E000, U+10FFFF]
──────────────────
Γ ⊢ 'c' : char
```

**No type inference:** Character literals always have type `char` with no context sensitivity:

```cantrip
let c1 = 'A'           // Type: char (no inference needed)
let c2: char = '🚀'    // Type: char (explicit annotation)
// No suffixes or context variations like integers
```

**Invalid characters:**

```cantrip
// ERROR E5401: Invalid Unicode scalar value (surrogate pair)
// let invalid: char = '\u{D800}';  // U+D800 is in surrogate range
```

###### 2.1.4.3.3 Type Properties

**Theorem 5.4.1 (Value Set):**

```
⟦char⟧ = Unicode Scalar Values = [U+0000, U+D7FF] ∪ [U+E000, U+10FFFF]

Total valid values: 1,112,064 codepoints
- U+0000 to U+D7FF: 55,296 codepoints
- U+E000 to U+10FFFF: 1,056,768 codepoints
```

**Excluded range (surrogate pairs):**

```
[U+D800, U+DFFF] = 2,048 invalid codepoints
```

**Theorem 5.4.2 (Size and Alignment):**

```
size(char) = 4 bytes
align(char) = 4 bytes
```

**Theorem 5.4.3 (Copy Capability):**

```
char : Copy
```

**Semantics:**

- Characters pass by permission (reference-like) by default
- The `.copy()` method creates an explicit duplicate when needed
- Fixed 4-byte size makes copying inexpensive when explicitly requested

##### 2.1.4.4 Dynamic Semantics

#### Evaluation Rules

**Literal evaluation:**

```
[E-Char]
────────────────
⟨'c', σ⟩ ⇓ ⟨'c', σ⟩
```

**Character comparison:**

```
[E-Char-Comparison]
⟨c₁, σ⟩ ⇓ ⟨v₁, σ'⟩    ⟨c₂, σ'⟩ ⇓ ⟨v₂, σ''⟩
codepoint(v₁) ⊙ codepoint(v₂) = result
where ⊙ ∈ {<, ≤, >, ≥, ==, ≠}
─────────────────────────────────────────
⟨c₁ ⊙ c₂, σ⟩ ⇓ ⟨result, σ''⟩
```

#### Memory Representation

**Character layout:**

```
char memory layout (4 bytes):
┌─────────────────────┐
│ Unicode codepoint   │
│     (UTF-32)        │
└─────────────────────┘
Size: 4 bytes
Alignment: 4 bytes

Valid range: [U+0000, U+D7FF] ∪ [U+E000, U+10FFFF]

Examples:
'A'  → 0x00000041
'€'  → 0x000020AC
'🚀' → 0x0001F680
```

**Byte order:**

Characters follow platform byte order (typically little-endian):

```
'🚀' (U+1F680) on little-endian:
Address: [0]  [1]  [2]  [3]
Value:   0x80 0xF6 0x01 0x00
```

#### Operational Behavior

**Comparison semantics:**

Character comparison uses Unicode scalar value ordering:

```cantrip
'A' < 'Z'              // true (U+0041 < U+005A)
'a' > 'A'              // true (U+0061 > U+0041, lowercase > uppercase)
'0' < '9'              // true (U+0030 < U+0039)
'🚀' == '🚀'            // true (U+1F680 == U+1F680)
```

**Formal property:**

```
∀ c₁, c₂ : char. c₁ < c₂ ⟺ codepoint(c₁) < codepoint(c₂)
```

**Character conversions:**

Characters can be converted to/from `u32` representing the Unicode codepoint:

```cantrip
let c: char = 'A'
let code: u32 = c as u32                // 65 (U+0041)

// Converting u32 to char (validation required)
match char::from_u32(65) {
    Option.Some(valid_char) => valid_char,  // 'A'
    Option.None => panic("invalid codepoint"),
}

let invalid: Option<char> = char::from_u32(0xD800) // None (surrogate)
```

**Type rules for conversions:**

```
[T-Char-To-U32]
Γ ⊢ c : char
────────────────────────────
Γ ⊢ (c as u32) : u32

[T-U32-To-Char]
Γ ⊢ n : u32
────────────────────────────────────────
Γ ⊢ char::from_u32(n) : Option<char>    (runtime validation)
```

---

## 2.1.5 Never Type

##### 2.1.5.1 Overview

**Key innovation/purpose:** The never type enables the compiler to verify that certain code paths diverge (never return), supporting type-safe control flow analysis and exhaustiveness checking.

##### Guidance (non-normative)

- Use `!` to mark diverging functions, unreachable branches, and match arms that cannot complete; the coercion rule in §2.1.5.3 handles type compatibility.
- Prefer `Option`/`Result` or explicit enums when a function may sometimes return—`!` is reserved for true non-returning code.
- Treat expressions of type `!` as proof obligations: the compiler will consider subsequent code unreachable per the rules in §2.1.5.3.

##### 2.1.5.2 Syntax

#### Concrete Syntax

**Type syntax:**

```ebnf
NeverType ::= "!"
```

**Usage in function signatures:**

```cantrip
function panic(message: str): !
    uses panic
{
    std.process::abort()
}

function exit(code: i32): !
    uses process.exit
{
    std.process::exit(code)
}

function infinite_loop(): ! {
    loop {
        // Never returns
    }
}
```

#### Abstract Syntax

**Type:**

```
τ ::= !
```

**Value:**

```
(no values: ⟦!⟧ = ∅)
```

##### 2.1.5.3 Static Semantics

#### Well-Formedness Rules

```
[WF-Never]
──────────────────
Γ ⊢ ! : Type
```

#### Type Rules

**Never type as bottom:**

```
[T-Never]
Γ ⊢ e : !
────────────────  (for any type T)
Γ ⊢ e : T
```

The never type coerces to any type because it represents diverging computation—if the expression never produces a value, it's safe to claim it has any type.

**Typing diverging functions:**

```
[T-Diverge-Panic]
Γ ⊢ panic(...) : !

[T-Diverge-Exit]
Γ ⊢ exit(...) : !

[T-Diverge-Loop]
loop { ... } has no break
────────────────────────
Γ ⊢ loop { ... } : !
```

#### Type Properties

**Theorem 5.5.1 (Empty Value Set):**

The never type is uninhabited:

```
⟦!⟧ = ∅
```

There are no values of type `!`.

**Theorem 5.5.2 (Bottom Type):**

The never type is a subtype of all types:

```
∀ τ. ! <: τ
```

This allows expressions of type `!` to be used in contexts expecting any type.

**Theorem 5.5.3 (Size and Alignment):**

```
size(!) = 0 bytes
align(!) = 1 byte
```

The never type is zero-sized because it's never instantiated.

**Corollary:** The never type cannot be Copy (no values to copy), but this is irrelevant since no values exist.

##### 2.1.5.4 Dynamic Semantics

#### Evaluation Rules

There are no evaluation rules for the never type itself, as it represents diverging computation:

```
(no evaluation rules: ! values never exist)
```

**Diverging expression semantics:**

```
[E-Diverge]
⟨e, σ⟩ ⇓ ⊥    (diverges: never produces a value)
```

Where ⊥ indicates divergence (non-termination, panic, or exit).

#### Memory Representation

**Never type layout:**

```
! memory layout:
(no memory allocated, zero-sized type)
Size: 0 bytes
Alignment: 1 byte
```

The never type has no runtime representation since values of this type never exist.

#### Operational Behavior

**Divergence categories:**

1. **Panic**: Abnormal termination

```cantrip
function panic(msg: str): ! {
    std.process::abort()
}
```

2. **Exit**: Normal process termination

```cantrip
function exit(code: i32): ! {
    std.process::exit(code)
}
```

3. **Infinite loop**: Non-terminating computation

```cantrip
function serve_forever(): ! {
    loop {
        handle_request()
    }
}
```

## 2.1.6 String Types

##### 2.1.6.1 Overview

**Key innovation/purpose:** String provides a single, unified type for owned text data with automatic memory management and guaranteed UTF-8 validity, simplifying text handling compared to systems requiring explicit distinction between owned and borrowed string types.

##### Guidance (non-normative)

- Use `own String` when text must be growable or outlive its source scope; treat `str` slices as the borrowing counterpart described in §2.1.6.2.
- Store binary or non-UTF-8 data in `Vec<u8>` and convert explicitly—`String` enforces the UTF-8 invariant in §2.1.6.3.
- Prefer `char` or iterator-based facilities for single-scalar operations; rely on the permission forms (`mut`, `own`, `iso`) defined earlier for mutation and ownership.

##### 2.1.6.2 Syntax

###### 2.1.6.2.1 Concrete Syntax

**Type syntax:**

```ebnf
StringType ::= "String"
```

**String literal syntax (produces `str` type):**

```ebnf
StringLiteral  ::= '"' (EscapeSequence | ~["\\])* '"'
EscapeSequence ::= "\\" [nrt\\'"0]
                 | "\\x" HexDigit HexDigit
                 | "\\u{" HexDigit+ "}"
```

**Permission annotations:**

```ebnf
OwnedString    ::= "own" "String"
MutableString  ::= "mut" "String"
RefString      ::= "String"              # Immutable reference (default)
IsolatedString ::= "iso" "String"
```

**Examples:**

```cantrip
// Type annotations
let message: own String = String.new()
let greeting: String = "hello"          // ERROR: type mismatch (str vs String)

// String construction
let empty: own String = String.new()
let from_literal: own String = String.from("hello")

// Permission annotations
function read_only(text: String) { ... }         // Immutable reference
function modify(text: mut String) { ... }        // Mutable reference
function consume(own text: String) { ... }       // Takes ownership

// String literals (type: str, not String)
"hello world"
"line 1\nline 2"
"unicode: 🚀"
"quotes: \" "
"hex byte: \x41"
"unicode escape: \u{1F600}"
```

###### 2.1.6.2.2 Abstract Syntax

**Type representation:**

```
τ ::= String                                    (String type)
    | own String                                (owned String)
    | mut String                                (mutable reference to String)
    | iso String                                (isolated String)

String = record {
    data: own Vec<u8>                          (UTF-8 byte vector)
}
```

**Value set:**

```
⟦String⟧ = { s | s = (ptr, len, cap),
             ptr : *u8,
             len : usize,
             cap : usize,
             len ≤ cap,
             valid_utf8(ptr[0..len]) }

where valid_utf8(bytes) ⟹ bytes form valid UTF-8 sequence
```

**Components:**

- **ptr**: Pointer to heap-allocated UTF-8 byte data
- **len**: Current length in bytes (not characters)
- **cap**: Allocated capacity in bytes
- **Invariant**: `data` contains valid UTF-8 at all times

**UTF-8 invariant (formal):**

```
∀ s : String. valid_utf8(s.data[0..s.len])

valid_utf8(bytes) ⟺
  ∀ i. (bytes[i] ∈ [0x00, 0x7F]) ∨
       (bytes[i] ∈ [0xC2, 0xDF] ∧ continuation(bytes[i+1])) ∨
       (bytes[i] ∈ [0xE0, 0xEF] ∧ continuation(bytes[i+1..i+2])) ∨
       (bytes[i] ∈ [0xF0, 0xF4] ∧ continuation(bytes[i+1..i+3]))

where continuation(b) ⟺ b ∈ [0x80, 0xBF]
```

##### 2.1.6.3 Static Semantics

###### 2.1.6.3.1 Well-Formedness Rules

**Definition 2.1.6.2 (Well-Formed String):** The String type is well-formed in any context. String values must maintain the UTF-8 validity invariant.

**[WF-String]** - String type is well-formed:

```
────────────────
Γ ⊢ String : Type
```

**Explanation:** String is a built-in type available in all contexts without imports.

**[WF-String-Owned]** - Owned string is well-formed:

```
[WF-String-Owned]
Γ ⊢ String : Type
─────────────────────
Γ ⊢ own String : Type
```

**Explanation:** Owned String is the standard form for heap-allocated strings. The `own` permission indicates ownership of heap memory.

**[WF-String-Mut]** - Mutable string reference:

```
[WF-String-Mut]
Γ ⊢ String : Type
─────────────────────
Γ ⊢ mut String : Type
```

**Explanation:** Mutable references to String allow modification operations like `push_str`. Multiple mutable references are permitted in Cantrip.

**[WF-String-UTF8-Invariant]** - UTF-8 validity invariant:

```
[WF-String-UTF8-Invariant]
Γ ⊢ s : String
─────────────────────────────
valid_utf8(s.ptr, s.len) = true
```

**Explanation:** All String values satisfy UTF-8 validity. The String API will this invariant is never violated.

###### 2.1.6.3.2 Type Rules

**[T-String-Literal-Conv]** - String literal conversion:

```
[T-String-Literal-Conv]
Γ ⊢ lit : str
valid_utf8(lit) = true
────────────────────────────────
Γ ⊢ String.from(lit) : own String
    uses alloc.heap
```

**Explanation:** String literals have type `str` (immutable string slice). Converting to `String` allocates heap memory and copies the data.

**Example:**

```cantrip
function create_greeting(): own String
    uses alloc.heap
{
    String.from("Hello, World!")
    // Type: own String (heap-allocated copy)
}
```

**[T-String-New]** - Empty string construction:

```
[T-String-New]
────────────────────────────────
Γ ⊢ String.new() : own String
    uses alloc.heap
```

**Explanation:** Creating a new empty String allocates minimal heap storage and returns owned String.

**Example:**

```cantrip
function create_buffer(): own String
    uses alloc.heap
{
    String.new()  // Empty string, capacity may be 0 or small
}
```

**[T-String-From-Slice]** - String from slice:

```
[T-String-From-Slice]
Γ ⊢ data : str
valid_utf8(data) = true
────────────────────────────────
Γ ⊢ String.from(data) : own String
    uses alloc.heap
```

**Explanation:** Construct String from any string slice. Allocates heap memory and copies the UTF-8 data.

**Example:**

```cantrip
function duplicate(text: str): own String
    uses alloc.heap
{
    String.from(text)  // Heap-allocated copy
}
```

**[T-String-Method]** - String method invocation:

```
[T-String-Method]
Γ ⊢ s : String
method m has signature (String, T₁, ..., Tₙ) → U ! ε
Γ ⊢ arg₁ : T₁    ...    Γ ⊢ argₙ : Tₙ
─────────────────────────────────────────────────
Γ ⊢ s.m(arg₁, ..., argₙ) : U ! ε
```

**Explanation:** Method calls on String follow standard method resolution. The receiver permission (String, mut String, own String) must match method signature.

**Example:**

```cantrip
let s: own String = String.from("hello")
let n: usize = s.len()        // Method: len(self: String): usize
let empty: bool = s.is_empty() // Method: is_empty(self: String): bool
```

**[T-String-Mut-Method]** - Mutable string method:

```
[T-String-Mut-Method]
Γ ⊢ s : own String
method m has signature (mut String, T₁, ..., Tₙ) → U ! ε
Γ ⊢ arg₁ : T₁    ...    Γ ⊢ argₙ : Tₙ
─────────────────────────────────────────────────
Γ ⊢ s.m(arg₁, ..., argₙ) : U ! ε
```

**Explanation:** Mutable methods like `push_str` require mutable reference. Owned String can provide mutable access.

**Example:**

```cantrip
function append_text(text: str): own String
    uses alloc.heap
{
    var s = String.new()
    s.push_str(text)  // Mutates s (requires mut String)
    move s
}
```

**[T-String-Permission-Upcast]** - Permission compatibility:

```
[T-String-Permission-Upcast]
Γ ⊢ s : own String
────────────────────────────
Γ ⊢ s : String
```

**Explanation:** Owned String can be passed as immutable reference without transferring ownership. This enables read-only access while retaining ownership.

**Example:**

```cantrip
function print(text: String) {
    // Read-only access to text
}

function example()
    uses alloc.heap
{
    let msg: own String = String.from("data")
    print(msg)  // Passes immutable reference (own String <: String)
    print(msg)  // msg still owned here
    // msg destroyed at end of scope
}
```

**[T-String-Move]** - Ownership transfer:

```
[T-String-Move]
Γ ⊢ s : own String
Γ' = Γ \ {s}
────────────────────────────
Γ ⊢ move s : own String
Γ' ⊬ s
```

**Explanation:** Moving a String transfers ownership of heap buffer. Original binding becomes inaccessible after move.

**Example:**

```cantrip
function consume(own s: String) {
    // s destroyed here
}

function example()
    uses alloc.heap
{
    let s1: own String = String.from("text")
    let s2: own String = move s1  // Transfer ownership
    // s1 no longer accessible
    consume(move s2)  // Transfer to function
}
```

###### 2.1.6.3.3 Type Properties

**Theorem 5.6.1 (String is NOT Copy):**

String does NOT implement the Copy trait. Assignment and parameter passing transfer ownership via move semantics.

**Formal statement:**

```
∀s : String. s ∉ Copy
```

**Proof sketch:** String contains `own Vec<u8>` which owns heap memory. Copying would require deep copy of heap buffer, violating Copy trait semantics (bitwise copy). Therefore String is move-only.

**Example:**

```cantrip
let s1: own String = String.from("hello")
let s2: own String = move s1  // Explicit move required
// s1 no longer accessible

// Copying would require:
let s3: own String = s2.clone()  // Explicit clone for deep copy
```

**Theorem 5.6.2 (UTF-8 Invariant Preservation):**

All String values satisfy the UTF-8 validity invariant. All String operations preserve this invariant.

**Formal statement:**

```
∀s : String. valid_utf8(s.ptr, s.len) = true

∀op ∈ StringOps. (valid_utf8(s) ∧ s' = op(s)) ⇒ valid_utf8(s')
```

**Proof sketch:** String API only accepts UTF-8 validated input (`str` slices, which are validated). Internal mutations preserve byte boundaries. Therefore invariant is maintained.

**Property 5.6.1 (Memory Layout):**

String has the following memory characteristics:

```
sizeof(String) = 3 × sizeof(usize) = 24 bytes (on 64-bit systems)
alignof(String) = alignof(usize) = 8 bytes (on 64-bit systems)
```

**Memory representation:**

```
String {
    ptr: *u8,      // 8 bytes (64-bit pointer)
    len: usize,    // 8 bytes (current length)
    cap: usize,    // 8 bytes (allocated capacity)
}
```

**Property 5.6.2 (Permission Semantics):**

String interacts with the permission system as follows:

| Permission | Syntax       | Semantics          | Operations Allowed                             |
| ---------- | ------------ | ------------------ | ---------------------------------------------- |
| Immutable  | `String`     | Borrowed reference | Read-only methods (len, is_empty, chars, etc.) |
| Mutable    | `mut String` | Mutable reference  | Modification methods (push_str, clear, etc.)   |
| Owned      | `own String` | Full ownership     | All operations, destroyed at scope end         |

**Note:** Unlike Rust, Cantrip permits multiple `mut String` references simultaneously. Programmer is responsible for ensuring safe usage.

**Example:**

```cantrip
function permissions_demo()
    uses alloc.heap
{
    let s: own String = String.from("base")

    // Immutable reference
    let len: usize = s.len()  // OK: s implicitly borrowed as String

    // Mutable reference
    var s_mut = s  // s_mut has type own String
    s_mut.push_str(" text")  // OK: can mutate owned string

    // Multiple mut refs (allowed in Cantrip!)
    let ref1 = mut s_mut
    let ref2 = mut s_mut
    // Programmer responsible for safe usage
}
```

**Property 5.6.3 (Allocation Requirements):**

String operations requiring heap allocation must declare the `alloc.heap` effect:

```
String.new() : own String ! {alloc.heap}
String.from(str) : own String ! {alloc.heap}
String.push_str(mut String, str) ! {alloc.heap}
```

**Rationale:** Explicit effect tracking makes memory allocation visible in function signatures, enabling resource budget analysis and constraint enforcement.

**Property 5.6.4 (Slice Relationship):**

String and str are related but distinct types:

```
String ≠ str

String → str  (via deref coercion or explicit .as_str())
str → String  (via String.from(), must allocation)
```

**Relationship rules:**

```
[String-To-Slice]
Γ ⊢ s : String
────────────────────────────
Γ ⊢ s : str  (deref coercion)

[Slice-To-String]
Γ ⊢ s : str
────────────────────────────
Γ ⊢ String.from(s) : own String ! {alloc.heap}
```

**Example:**

```cantrip
function accepts_slice(text: str) {
    // Read-only access to text
}

function example()
    uses alloc.heap
{
    let owned: own String = String.from("data")

    // String can be passed where str expected (deref coercion)
    accepts_slice(owned)  // OK: String → str

    // str to String must allocation
    let slice: str = "literal"
    let owned2: own String = String.from(slice)  // Allocates
}
```

**Theorem 5.6.3 (String Destruction):**

When an owned String goes out of scope, its heap memory is automatically deallocated.

**Formal statement:**

```
{s : own String}
block { ... }
{deallocated(s.ptr)}
```

**Operational semantics:**

```
[E-String-Destroy]
⟨block, σ[s ↦ string_val(ptr, len, cap)]⟩ ⇓ ⟨v, σ'⟩
─────────────────────────────────────────────────────
σ' = σ \ {ptr}  ∧  free_heap(ptr, cap)
```

**Example:**

```cantrip
function automatic_cleanup()
    uses alloc.heap
{
    {
        let temp: own String = String.from("temporary")
        // Use temp
    } // temp.ptr deallocated here automatically

    // temp no longer accessible
}
```

##### 2.1.6.4 Dynamic Semantics

###### 2.1.6.4.1 Evaluation Rules

**String literal evaluation:**

String literals evaluate to String values pointing to static read-only memory (.rodata section). No heap allocation occurs during evaluation.

```
[E-String-Literal]
"text" stored at static address addr
len = byte_length("text")
─────────────────────────────────────────────────────────────
⟨"text", σ⟩ ⇓ ⟨String{ptr: addr, len: len, cap: len}, σ⟩
```

**Explanation:** String literals are embedded in the executable's read-only data section. The resulting String value is a dense pointer to this static memory. Since no heap allocation is performed, the store σ remains unchanged.

**String heap allocation:**

Creating a new owned string allocates an empty buffer on the heap with default capacity.

```
[E-String-New]
alloc_heap(DEFAULT_CAP) = addr
─────────────────────────────────────────────────────────────
⟨String.new(), σ⟩ ⇓ ⟨String{ptr: addr, len: 0, cap: DEFAULT_CAP}, σ'⟩
    where σ' = σ ∪ {addr ↦ [0u8; DEFAULT_CAP]}
```

**Explanation:** String.new() allocates a heap buffer and returns an owned String with length 0. The capacity (DEFAULT_CAP) is implementation-defined but typically 8-16 bytes. The store σ' includes the new allocation.

**String from UTF-8 validation:**

Constructing a string from untrusted bytes must validation to ensure UTF-8 correctness.

```
[E-String-From-UTF8-Valid]
⟨bytes, σ⟩ ⇓ ⟨data, σ'⟩
validate_utf8(data) = true
alloc_heap(data.len) = addr
memcpy(addr, data.ptr, data.len)
─────────────────────────────────────────────────────────────
⟨String.from_utf8(bytes), σ⟩ ⇓ ⟨Ok(String{ptr: addr, len: data.len, cap: data.len}), σ''⟩
    where σ'' = σ' ∪ {addr ↦ data}

[E-String-From-UTF8-Invalid]
⟨bytes, σ⟩ ⇓ ⟨data, σ'⟩
validate_utf8(data) = false
─────────────────────────────────────────────────────────────
⟨String.from_utf8(bytes), σ⟩ ⇓ ⟨Err(Utf8Error), σ'⟩
```

**Explanation:** UTF-8 validation is performed in O(n) time where n = byte length. Valid sequences result in heap allocation and Ok(String). Invalid sequences return Err without allocating.

**String concatenation:**

Concatenating two strings may require reallocation if capacity is exceeded.

```
[E-String-Concat-No-Realloc]
⟨s1, σ⟩ ⇓ ⟨String{ptr: p1, len: l1, cap: c1}, σ'⟩
⟨s2, σ'⟩ ⇓ ⟨String{ptr: p2, len: l2, cap: c2}, σ''⟩
l1 + l2 ≤ c1
memcpy(p1 + l1, p2, l2)
─────────────────────────────────────────────────────────────
⟨s1.push_str(s2), σ⟩ ⇓ ⟨(), σ'''⟩
    where σ'''(p1).len = l1 + l2

[E-String-Concat-Realloc]
⟨s1, σ⟩ ⇓ ⟨String{ptr: p1, len: l1, cap: c1}, σ'⟩
⟨s2, σ'⟩ ⇓ ⟨String{ptr: p2, len: l2, cap: c2}, σ''⟩
l1 + l2 > c1
new_cap = max(c1 × 2, l1 + l2)
alloc_heap(new_cap) = addr
memcpy(addr, p1, l1)
memcpy(addr + l1, p2, l2)
dealloc_heap(p1)
─────────────────────────────────────────────────────────────
⟨s1.push_str(s2), σ⟩ ⇓ ⟨(), σ'''⟩
    where σ''' = (σ'' \ {p1 ↦ _}) ∪ {addr ↦ [l1 + l2 bytes]}
          s1 = String{ptr: addr, len: l1 + l2, cap: new_cap}
```

**Explanation:** If the existing capacity is sufficient, bytes are appended in-place. Otherwise, a new buffer is allocated with doubled capacity (or exact size if doubling is insufficient), existing data is copied, new data is appended, and the old buffer is freed.

**String slicing:**

Slicing a string creates a new string view (borrows the underlying data).

```
[E-String-Slice]
⟨s, σ⟩ ⇓ ⟨String{ptr: p, len: l, cap: c}, σ'⟩
⟨start, σ'⟩ ⇓ ⟨a, σ''⟩
⟨end, σ''⟩ ⇓ ⟨b, σ'''⟩
a ≤ b ≤ l
is_char_boundary(p, a) ∧ is_char_boundary(p, b)
─────────────────────────────────────────────────────────────
⟨s[start..end], σ⟩ ⇓ ⟨String{ptr: p + a, len: b - a, cap: b - a}, σ'''⟩

[E-String-Slice-Invalid-Boundary]
⟨s, σ⟩ ⇓ ⟨String{ptr: p, len: l, cap: c}, σ'⟩
⟨start, σ'⟩ ⇓ ⟨a, σ''⟩
⟨end, σ''⟩ ⇓ ⟨b, σ'''⟩
a ≤ b ≤ l
¬(is_char_boundary(p, a) ∧ is_char_boundary(p, b))
─────────────────────────────────────────────────────────────
⟨s[start..end], σ⟩ ⇓ panic("byte index not at char boundary")
```

**Explanation:** String slicing must occur at UTF-8 character boundaries. A byte index is a character boundary if it is 0, equal to the string length, or points to a byte that is not a UTF-8 continuation byte (i.e., not in range 0x80-0xBF). Invalid boundaries cause runtime panic.

**Character boundary predicate:**

```
is_char_boundary(ptr, idx) ⟺
    idx = 0 ∨
    idx = len ∨
    (ptr[idx] & 0xC0) ≠ 0x80
```

###### 2.1.6.4.2 Memory Representation

**Record Layout:**

The String type is represented as a dense pointer with three fields:

```
String record layout (24 bytes on 64-bit):
┌─────────────────┬──────────────┬──────────────┐
│ ptr: *u8        │ len: usize   │ cap: usize   │
│ 8 bytes         │ 8 bytes      │ 8 bytes      │
└─────────────────┴──────────────┴──────────────┘
Offset:  0              8               16

Field Semantics:
• ptr:  Pointer to UTF-8 encoded byte array
• len:  Number of BYTES currently used (NOT character count)
• cap:  Total capacity in BYTES (for owned strings only)
```

**Size and alignment:**

```
sizeof(String)  = 24 bytes  (on 64-bit platforms)
                = 12 bytes  (on 32-bit platforms)
alignof(String) = 8 bytes   (on 64-bit platforms)
                = 4 bytes   (on 32-bit platforms)
```

**Important distinctions:**

- `len` represents byte length, not character count
- `cap` represents allocated byte capacity
- Character count must O(n) iteration over UTF-8 sequences

**String literal storage:**

String literals are stored in the executable's read-only data section:

```
.rodata section (static memory):
┌─────────────────────────────────┐
│ string_0: "Hello, World!\0"     │ ← addr_0
│ string_1: "Cantrip\0"           │ ← addr_1
│ string_2: "🦀 Rust inspired\0"  │ ← addr_2
└─────────────────────────────────┘

String literal expression "Hello, World!" evaluates to:
String {
    ptr: addr_0,
    len: 13,      // byte length (no null terminator in len)
    cap: 13,
}
```

**Properties of literal strings:**

- Immutable (stored in read-only memory)
- Zero allocation cost (no heap usage)
- Lifetime: 'static (valid for entire program execution)
- Null-terminated in .rodata for C FFI, but null not included in len

**Heap-allocated strings:**

Owned strings allocate a contiguous buffer on the heap:

```
Heap memory layout:
┌──────────────────────────────────────┐
│ UTF-8 byte array                     │
│ [capacity bytes allocated]           │
│ [first 'len' bytes used]             │
│ [remaining bytes unused/uninitialized]│
└──────────────────────────────────────┘
          ↑
          │ (points to start)
          │
String { ptr: ─┘, len: used_bytes, cap: total_allocated }
```

**Example:**

```cantrip
let mut s: own String = String.new()  // Allocate with DEFAULT_CAP
s.push_str("Hello")                   // Append 5 bytes

Heap state:
┌─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┐
│H│e│l│l│o│?│?│?│?│?│?│?│?│?│?│?│  (assuming DEFAULT_CAP = 16)
└─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┘
 ↑ len = 5              ↑ cap = 16
 ptr
```

**UTF-8 byte encoding in memory:**

Characters are stored as UTF-8 byte sequences:

```
Character: 'A'     → [0x41]                     (1 byte, ASCII)
Character: '©'     → [0xC2, 0xA9]               (2 bytes)
Character: '中'    → [0xE4, 0xB8, 0xAD]         (3 bytes)
Character: '🦀'    → [0xF0, 0x9F, 0xA6, 0x80]   (4 bytes, emoji)

String "A©中🦀" memory layout:
┌────┬────┬────┬────┬────┬────┬────┬────┬────┬────┐
│0x41│0xC2│0xA9│0xE4│0xB8│0xAD│0xF0│0x9F│0xA6│0x80│
└────┴────┴────┴────┴────┴────┴────┴────┴────┴────┘
  A      ©           中              🦀
 1 byte  2 bytes    3 bytes         4 bytes

Total: 10 bytes, 4 characters
len = 10 (byte length)
s.chars().count() = 4 (character count, must iteration)
```

**Memory layout theorem:**

**Theorem 5.6.4 (String Memory Invariants):**

For any String value `s = String{ptr: p, len: l, cap: c}`, the following invariants hold:

1. **Capacity constraint:** `0 ≤ l ≤ c`
2. **UTF-8 validity:** `validate_utf8(p[0..l]) = true`
3. **Allocation validity:** If `c > 0`, then `p` points to valid heap allocation of size `c`
4. **Null-pointer invariant:** If `c = 0`, then `p` may be null or dangling (zero-capacity strings need not allocate)
5. **Alignment:** `p` is aligned to 1-byte boundary (no alignment requirement for u8 arrays)

**Proof sketch:**

- Invariant (1) enforced by all String operations (construction, push, etc.)
- Invariant (2) enforced by UTF-8 validation in String.from_utf8() and maintained by all operations
- Invariants (3-4) enforced by allocator interface and String.new() semantics
- Invariant (5) follows from u8 alignment requirements (trivially satisfied)

###### 2.1.6.4.3 Operational Behavior

**Allocation strategy:**

**String literals (zero-cost):**

- Storage: Read-only data section (.rodata)
- Allocation: None (embedded in executable)
- Mutability: Immutable (any attempt to modify causes compiler error or runtime fault)
- Lifetime: 'static (valid for entire program execution)
- Cost: Zero runtime overhead

**Owned strings (heap-allocated):**

- Storage: Heap memory
- Allocation: Dynamic via allocator (uses `alloc.heap`)
- Mutability: Mutable if declared with `mut`
- Lifetime: Lexically scoped (freed when owner goes out of scope)
- Cost: Allocation/deallocation overhead, possible reallocation on growth

**Growth strategy (capacity management):**

When appending data exceeds current capacity, strings employ an amortized doubling strategy:

```
new_capacity = max(old_capacity × 2, required_size)
```

**Example growth sequence:**

```
Initial:     cap = 0,  len = 0
After "Hi":  cap = 8,  len = 2   (allocated with DEFAULT_CAP)
After 6 more bytes (len=8):  cap = 8,  len = 8   (fits, no realloc)
After 1 more byte (len=9):   cap = 16, len = 9   (realloc: 8 × 2 = 16)
After 7 more bytes (len=16): cap = 16, len = 16  (fits, no realloc)
After 1 more byte (len=17):  cap = 32, len = 17  (realloc: 16 × 2 = 32)
```

**Amortized analysis:**

- Individual push operations: O(1) amortized
- Worst case (reallocation): O(n) where n = current length
- Total cost of n pushes: O(n) amortized

**Deallocation behavior:**

Owned strings (`own String`) are automatically deallocated when their owner's scope ends:

```cantrip
function example()
    uses alloc.heap
{
    let s: own String = String.new()  // Allocate heap buffer
    s.push_str("data")                // Append data (may realloc)
    // Automatic deallocation occurs here
}
```

**Deallocation rule:**

```
[E-String-Dealloc]
⟨{ let s: own String = e; body }, σ⟩ ⇓ ⟨v, σ'⟩
s = String{ptr: p, len: l, cap: c}
c > 0
─────────────────────────────────────────────────────────────
final σ'' = σ' \ {p ↦ _}  (heap buffer freed)
```

**Exception:** Zero-capacity strings (empty string constants) may not allocate, hence no deallocation.

**Permission transitions:**

Unlike Rust, Cantrip allows multiple mutable references through region-based lifetime management:

```
Ownership transitions:
own String ────→ String      (pass as immutable reference)
own String ────→ mut String  (pass as mutable reference)

Region guarantees:
• Multiple `mut String` references can coexist
• Region system ensures lifetime safety through escape analysis
• All references must not outlive the region containing the owned data
```

**Example:**

```cantrip
function process(s1: mut String, s2: mut String)
    uses alloc.heap
{
    s1.push_str("A")
    s2.push_str("B")  // OK: multiple mutable references allowed
}

let mut owner: own String = String.new()
process(owner, owner)  // OK in Cantrip (unlike Rust)
                        // Regions ensure owner outlives both references
```

**Performance characteristics:**

```
Operation              Time Complexity    Space Complexity
──────────────────────────────────────────────────────────
String literal         O(1)               O(1) - static memory
String.new()           O(1)               O(cap) - heap allocation
push_str(s)            O(m) amortized     O(m) worst case (realloc)
                       where m = s.len
s[a..b]                O(1)               O(1) - dense pointer creation
s.chars()              O(n)               O(1) - iterator creation
s.chars().count()      O(n)               O(1)
validate_utf8(s)       O(n)               O(1)
──────────────────────────────────────────────────────────
where n = byte length of string
```

**Memory overhead:**

- String struct: 24 bytes (on 64-bit)
- Heap allocation: len + (cap - len) bytes
- Minimum overhead: 24 bytes + 0-16 bytes (DEFAULT_CAP)

**Cache efficiency:**

- UTF-8 data stored contiguously for excellent cache locality
- Sequential iteration exhibits near-optimal cache behavior
- Random character access must O(n) scan (UTF-8 is variable-width)

##### 2.1.6.5 Additional Properties

**Definition 2.1.6.3 (UTF-8 Encoding):** UTF-8 is a variable-width character encoding for Unicode, using 1-4 bytes per Unicode scalar value. All String values in Cantrip MUST contain valid UTF-8 byte sequences.

###### 2.1.6.5.1 Invariants

UTF-8 encodes Unicode scalar values (U+0000 to U+10FFFF, excluding surrogates U+D800-U+DFFF) using 1-4 bytes:

**1-byte sequence (ASCII range: U+0000 to U+007F):**

```
Byte pattern: 0xxxxxxx
Range:        U+0000 to U+007F (0 to 127 decimal)
Bits:         7 bits for codepoint
```

**Examples:**

```
'A'   (U+0041) → [01000001]        → 0x41
'0'   (U+0030) → [00110000]        → 0x30
'\n'  (U+000A) → [00001010]        → 0x0A
' '   (U+0020) → [00100000]        → 0x20
```

**2-byte sequence (U+0080 to U+07FF):**

```
Byte pattern: 110xxxxx 10xxxxxx
Range:        U+0080 to U+07FF (128 to 2,047 decimal)
Bits:         11 bits for codepoint (5 + 6)
```

**Examples:**

```
'©'   (U+00A9) → [11000010 10101001] → 0xC2 0xA9
'é'   (U+00E9) → [11000011 10101001] → 0xC3 0xA9
'Ω'   (U+03A9) → [11001110 10101001] → 0xCE 0xA9
```

**3-byte sequence (U+0800 to U+FFFF, excluding surrogates):**

```
Byte pattern: 1110xxxx 10xxxxxx 10xxxxxx
Range:        U+0800 to U+FFFF (2,048 to 65,535 decimal)
              EXCLUDING U+D800 to U+DFFF (surrogate pairs)
Bits:         16 bits for codepoint (4 + 6 + 6)
```

**Examples:**

```
'中'  (U+4E2D) → [11100100 10111000 10101101] → 0xE4 0xB8 0xAD
'€'   (U+20AC) → [11100010 10000010 10101100] → 0xE2 0x82 0xAC
'—'   (U+2014) → [11100010 10000000 10010100] → 0xE2 0x80 0x94
```

**4-byte sequence (U+10000 to U+10FFFF):**

```
Byte pattern: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
Range:        U+10000 to U+10FFFF (65,536 to 1,114,111 decimal)
Bits:         21 bits for codepoint (3 + 6 + 6 + 6)
```

**Examples:**

```
'🦀'  (U+1F980) → [11110000 10011111 10100110 10000000] → 0xF0 0x9F 0xA6 0x80
'🚀'  (U+1F680) → [11110000 10011111 10011010 10000000] → 0xF0 0x9F 0x9A 0x80
'😀'  (U+1F600) → [11110000 10011111 10011000 10000000] → 0xF0 0x9F 0x98 0x80
```

**Invariant 5.6.1 (UTF-8 Validity):** All String values MUST contain valid UTF-8 at all times.

**Formally:**

```
�^?s : String. validate_utf8(s.as_bytes()) = true
```

**Operations that maintain invariant:**

1. **String literals (compiler validated):**

   ```cantrip
   let s = "Hello, �,-�O"  // Compiler will UTF-8 validity
   ```

2. **String.new() (creates empty valid string):**

   ```cantrip
   let s = String.new()  // Empty string is trivially valid UTF-8
   ```

3. **String.from_utf8() (validates or errors):**

   ```cantrip
   let s = String.from_utf8(bytes)?  // Returns Err if invalid
   ```

4. **push_str(valid_string) (concatenation preserves validity):**

   ```cantrip
   s1.push_str(s2)  // If s1 and s2 valid, result is valid
   ```

5. **Slicing at character boundaries:**
   ```cantrip
   let slice = s[0..5]  // Panics if not at char boundary
   ```

**Theorem 5.6.5 (UTF-8 Invariant Preservation):**

If `s1` and `s2` are valid UTF-8 strings, then:

1. `s1.push_str(s2)` produces a valid UTF-8 string
2. `s1 + s2` (if concatenation operator exists) produces a valid UTF-8 string
3. `s[a..b]` produces a valid UTF-8 string if `a` and `b` are character boundaries

**Proof sketch:**

1. Valid UTF-8 sequences concatenated produce a valid UTF-8 sequence
2. Slicing at character boundaries preserves sequence validity
3. No operation modifies the byte content except by appending valid UTF-8

###### 2.1.6.5.2 Verification

Verification will that all byte sequences stored in String instances satisfy the UTF-8 invariant before being exposed to user code.

##### Invalid Sequences

**Normative requirement:** The following byte sequences are INVALID in UTF-8 and MUST be rejected by validation:

**1. Overlong encodings:**

Encoding a codepoint with more bytes than necessary.

```
Invalid: 'A' (U+0041) as [11000001 10000001] → 0xC1 0x81  (2-byte overlong)
Valid:   'A' (U+0041) as [01000001]           → 0x41      (1-byte correct)

Invalid: '©' (U+00A9) as [11100000 10000010 10101001] → 0xE0 0x82 0xA9  (3-byte overlong)
Valid:   '©' (U+00A9) as [11000010 10101001]           → 0xC2 0xA9      (2-byte correct)
```

**Detection rule:** A sequence is overlong if the codepoint value could be encoded in fewer bytes.

**2. UTF-16 surrogate halves (U+D800 to U+DFFF):**

These codepoints are reserved for UTF-16 encoding and are invalid in UTF-8.

```
Invalid: [11101101 10100000 10000000] → 0xED 0xA0 0x80  (U+D800, high surrogate)
Invalid: [11101101 10111111 10111111] → 0xED 0xBF 0xBF  (U+DFFF, low surrogate)
```

**Rationale:** UTF-8 encodes Unicode scalar values directly; surrogate pairs are a UTF-16 encoding artifact.

**3. Codepoints beyond U+10FFFF:**

Unicode defines codepoints only up to U+10FFFF.

```
Invalid: [11110100 10010000 10000000 10000000] → 0xF4 0x90 0x80 0x80  (U+110000)
Invalid: [11111000 10000000 10000000 10000000] → 0xF8 0x80 0x80 0x80  (5-byte sequence)
```

**Maximum valid codepoint:** U+10FFFF

**4. Incomplete sequences:**

Missing continuation bytes.

```
Invalid: [11000010]                 → 0xC2      (2-byte lead, missing continuation)
Invalid: [11100100 10111000]        → 0xE4 0xB8 (3-byte lead, only 1 continuation)
Invalid: [11110000 10011111 10100110] → 0xF0 0x9F 0xA6 (4-byte lead, only 2 continuations)
```

**5. Invalid continuation bytes:**

Continuation bytes outside the range 0x80-0xBF.

```
Invalid: [11000010 01000001]        → 0xC2 0x41 (second byte not in 0x80-0xBF)
Invalid: [11100100 11000000 10101101] → 0xE4 0xC0 0xAD (second byte not continuation)
```

**6. Unexpected continuation bytes:**

Continuation bytes without a leading byte.

```
Invalid: [10101001]                 → 0xA9      (continuation byte with no lead)
Invalid: [01000001 10101001]        → 0x41 0xA9 (continuation after ASCII)
```

**7. Invalid leading bytes:**

Bytes 0xC0, 0xC1, 0xF5-0xFF are never valid UTF-8 leading bytes.

```
Invalid: [11000000 10000000]        → 0xC0 0x80 (invalid leading byte 0xC0)
Invalid: [11111000 10000000 ...]    → 0xF8 ...  (5-byte sequence, invalid)
```

##### Validation Procedure

**Validation requirement:** All String construction from untrusted bytes MUST perform UTF-8 validation.

**Validation algorithm complexity:** O(n) where n = byte length

**Validation predicate:**

```
validate_utf8(bytes: [u8]) → bool:
    let i = 0
    while i < bytes.len:
        let b0 = bytes[i]

        // 1-byte sequence (ASCII)
        if b0 < 0x80:
            i += 1
            continue

        // Invalid leading bytes
        if b0 < 0xC2 || b0 > 0xF4:
            return false

        // 2-byte sequence
        if b0 < 0xE0:
            if i + 1 >= bytes.len:
                return false  // incomplete
            if !is_continuation(bytes[i + 1]):
                return false
            let cp = ((b0 & 0x1F) << 6) | (bytes[i + 1] & 0x3F)
            if cp < 0x80:
                return false  // overlong
            i += 2
            continue

        // 3-byte sequence
        if b0 < 0xF0:
            if i + 2 >= bytes.len:
                return false  // incomplete
            if !is_continuation(bytes[i + 1]) || !is_continuation(bytes[i + 2]):
                return false
            let cp = ((b0 & 0x0F) << 12) | ((bytes[i + 1] & 0x3F) << 6) | (bytes[i + 2] & 0x3F)
            if cp < 0x800:
                return false  // overlong
            if 0xD800 ≤ cp ≤ 0xDFFF:
                return false  // surrogate
            i += 3
            continue

        // 4-byte sequence
        if b0 < 0xF5:
            if i + 3 >= bytes.len:
                return false  // incomplete
            if !is_continuation(bytes[i + 1]) || !is_continuation(bytes[i + 2]) || !is_continuation(bytes[i + 3]):
                return false
            let cp = ((b0 & 0x07) << 18) | ((bytes[i + 1] & 0x3F) << 12) | ((bytes[i + 2] & 0x3F) << 6) | (bytes[i + 3] & 0x3F)
            if cp < 0x10000:
                return false  // overlong
            if cp > 0x10FFFF:
                return false  // out of range
            i += 4
            continue

        return false  // invalid leading byte

    return true

is_continuation(byte: u8) → bool:
    return (byte & 0xC0) == 0x80
```

**Safe construction API:**

```cantrip
function String.from_utf8(bytes: [u8]): Result<own String, Utf8Error>
    uses alloc.heap
{
    if validate_utf8(bytes) {
        let mut s = String.new()
        s.reserve(bytes.len)
        memcpy(s.ptr, bytes.ptr, bytes.len)
        s.len = bytes.len
        Ok(s)
    } else {
        Err(Utf8Error::InvalidSequence)
    }
}
```

**Unsafe operations (programmer responsibility):**

Unsafe operations bypass validation and place the burden on the programmer:

```cantrip
// UNSAFE: Programmer must ensure `bytes` contains valid UTF-8
function String.from_utf8_unchecked(bytes: [u8]): own String
    uses alloc.heap, unsafe.memory
{
    // No validation performed
    let mut s = String.new()
    s.reserve(bytes.len)
    memcpy(s.ptr, bytes.ptr, bytes.len)
    s.len = bytes.len
    s
}
```

###### 2.1.6.5.3 Algorithms

##### UTF-8 Encoding

**Encoding a Unicode scalar value to UTF-8:**

```
encode_utf8(codepoint: u32) → [u8]:
    if codepoint ≤ 0x7F:
        return [codepoint]
    else if codepoint ≤ 0x7FF:
        return [
            0xC0 | (codepoint >> 6),
            0x80 | (codepoint & 0x3F)
        ]
    else if codepoint ≤ 0xFFFF:
        if 0xD800 ≤ codepoint ≤ 0xDFFF:
            error("surrogate codepoint invalid")
        return [
            0xE0 | (codepoint >> 12),
            0x80 | ((codepoint >> 6) & 0x3F),
            0x80 | (codepoint & 0x3F)
        ]
    else if codepoint ≤ 0x10FFFF:
        return [
            0xF0 | (codepoint >> 18),
            0x80 | ((codepoint >> 12) & 0x3F),
            0x80 | ((codepoint >> 6) & 0x3F),
            0x80 | (codepoint & 0x3F)
        ]
    else:
        error("codepoint out of range")
```

##### Character Iteration

**Iterating over characters (not bytes):**

String provides an iterator that yields `char` values by decoding UTF-8:

```cantrip
function String.chars(self: String): CharIterator {
    CharIterator { ptr: self.ptr, end: self.ptr + self.len }
}

impl Iterator<char> for CharIterator {
    function next(mut self): Option<char> {
        if self.ptr >= self.end {
            return None
        }

        let (ch, byte_len) = decode_utf8_char(self.ptr)
        self.ptr += byte_len
        Some(ch)
    }
}
```

**UTF-8 decoding algorithm:**

```
decode_utf8_char(ptr: *u8) → (char, usize):
    let b0 = *ptr

    if b0 < 0x80:  // 1-byte (ASCII)
        return (b0 as char, 1)

    if b0 < 0xE0:  // 2-byte
        let b1 = *(ptr + 1)
        let cp = ((b0 & 0x1F) << 6) | (b1 & 0x3F)
        return (char::from_u32_unchecked(cp), 2)

    if b0 < 0xF0:  // 3-byte
        let b1 = *(ptr + 1)
        let b2 = *(ptr + 2)
        let cp = ((b0 & 0x0F) << 12) | ((b1 & 0x3F) << 6) | (b2 & 0x3F)
        return (char::from_u32_unchecked(cp), 3)

    // 4-byte
    let b1 = *(ptr + 1)
    let b2 = *(ptr + 2)
    let b3 = *(ptr + 3)
    let cp = ((b0 & 0x07) << 18) | ((b1 & 0x3F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F)
    return (char::from_u32_unchecked(cp), 4)
```

**Note:** `from_u32_unchecked` is safe here because the String invariant guarantees valid UTF-8.

**Example:**

```cantrip
let s = "A©中🦀"
for ch in s.chars() {
    // Yields: 'A', '©', '中', '🦀'
}

// Byte iteration (raw UTF-8 bytes)
for byte in s.as_bytes() {
    // Yields: 0x41, 0xC2, 0xA9, 0xE4, 0xB8, 0xAD, 0xF0, 0x9F, 0xA6, 0x80
}
```

##### 2.1.6.6 Advanced Features

###### 2.1.6.6.1 Permission Integration

**Definition 2.1.6.4 (String Permissions):** The `String` type interacts with Cantrip's Lexical Permission System through three permission modes: immutable reference (default), mutable reference (`mut`), and owned (`own`).

**Philosophy:** Cantrip follows the Cyclone model, not Rust. Permissions control _access rights_, while regions control _lifetimes_. Multiple mutable references are allowed—programmer's responsibility to prevent aliasing bugs.

####### 2.1.6.6.1.1 Immutable Reference (Default)

**Syntax:** `String` or `text: String`

**Type rule:**

```
[T-String-Ref]
Γ ⊢ s : String    immutable(s)
─────────────────────────────────
Γ ⊢ function f(x: String) accepts reference to s
```

**Semantics:**

- Can read string contents
- Cannot modify contents
- Cannot take ownership
- Many references can coexist (permission-based model)

**Operations allowed:**

```cantrip
function get_length(s: String): usize {
    s.len()              // OK: reading
}

function get_char(s: String, idx: usize): char {
    s.char_at(idx)       // OK: indexing
}

function compare(s1: String, s2: String): bool {
    s1 == s2             // OK: comparison
}
```

**Operations prohibited:**

```cantrip
function illegal_modify(s: String) {
    s.push_str("!")     // ERROR E3002: Cannot modify immutable reference
    s.clear()           // ERROR E3002: Cannot modify immutable reference
}
```

**Example - Multiple references allowed:**

```cantrip
function process_multiple(text: String)
    uses io.write
{
    let len1 = get_length(text)      // OK: first reference
    let len2 = get_length(text)      // OK: can pass again
    compare(text, text)              // OK: two references simultaneously

    // text still accessible here
    std.io.println(text)
}
```

**Formal semantics:**

```
⟦String⟧ = { reference to UTF-8 byte sequence }
Operations: Read-only (len, char_at, slice, ==, <, etc.)
Aliasing: Unlimited
```

####### 2.1.6.6.1.2 Mutable Reference

**Syntax:** `mut String` or `text: mut String`

**Type rule:**

```
[T-String-Mut]
Γ ⊢ s : String    mutable(s) = true
─────────────────────────────────────
Γ ⊢ s : mut String
```

**Semantics:**

- Can read and modify string contents
- Cannot take ownership (remains with caller)
- Multiple `mut` references CAN coexist (programmer's responsibility!)

**Operations allowed:**

```cantrip
function modify_string(s: mut String) {
    s.push_str("!")           // OK: can append
    s.clear()                 // OK: can clear
    s[0] = 'X'                // OK: can modify bytes (if valid UTF-8)
    let len = s.len()         // OK: can also read
}
```

**Example - Basic mutation:**

```cantrip
function append_exclamation(s: mut String) {
    s.push_str("!")
}

function example()
    uses alloc.heap
{
    var text: own String = String.from("Hello")
    append_exclamation(mut text)
    // text now contains "Hello!"
    std.io.println(text)  // Prints: Hello!
}
```

**CRITICAL DIFFERENCE FROM RUST:** Multiple mutable references allowed:

```cantrip
function modify1(s: mut String) {
    s.push_str(" world")
}

function modify2(s: mut String) {
    s.push_str("!")
}

function example()
    uses alloc.heap
{
    var text: own String = String.from("Hello")

    // Both allowed simultaneously - permission-based aliasing
    modify1(mut text)
    modify2(mut text)

    // Result: "Hello world!"
    // Programmer must ensure correct ordering!
}
```

**Aliasing responsibility:**

```cantrip
// UNSAFE PATTERN - Programmer's responsibility to avoid!
function dangerous_pattern()
    uses alloc.heap
{
    var text: own String = String.from("data")

    let ref1: mut String = mut text
    let ref2: mut String = mut text

    // Both ref1 and ref2 can modify - could cause bugs
    // Cantrip allows this, but programmer must be careful!
}
```

**Formal semantics:**

```
⟦mut String⟧ = { mutable reference to UTF-8 byte sequence }
Operations: Read, Write (push_str, clear, insert, remove, etc.)
Aliasing: Unlimited (programmer will safety)
Lifetime: Lexical scope
```

####### 2.1.6.6.1.3 Owned Permission

**Syntax:** `own String`

**Type rule:**

```
[T-String-Own]
Γ ⊢ e : String    copyable(String) = false
─────────────────────────────────────────────
Γ ⊢ e : own String
```

**Semantics:**

- Full ownership of the string
- Can read, modify, move, destroy
- Automatically destroyed when scope ends (no memory leaks)
- Exactly one owner (enforced by move semantics)

**Operations - Full control:**

```cantrip
function example()
    uses alloc.heap
{
    let s: own String = String.new()

    s.push_str("data")     // Can modify (owner has all rights)
    let len = s.len()      // Can read
    consume(move s)        // Can transfer ownership
    // s no longer accessible here
}
```

**Creating owned strings:**

```cantrip
function create_greeting(): own String
    uses alloc.heap
{
    var s: own String = String.new()
    s.push_str("Hello, ")
    s.push_str("World!")
    s  // Return ownership to caller
}
```

**Automatic destruction:**

```cantrip
function auto_cleanup()
    uses alloc.heap
{
    let temp: own String = String.from("temporary")
    temp.push_str(" data")
    // temp automatically freed here (no explicit drop needed)
}
```

**Move semantics:**

```cantrip
function consume_string(text: own String) {
    std.io.println(text)
    // text destroyed here
}

function example()
    uses alloc.heap
{
    let message: own String = String.from("Hello")

    consume_string(move message)  // Explicit transfer
    // message.len()  // ERROR E3004: Value moved
}
```

**Formal semantics:**

```
⟦own String⟧ = { unique pointer to UTF-8 byte sequence }
Operations: Read, Write, Move, Destroy
Aliasing: Exactly one owner
Lifetime: Lexical scope (automatic destruction)
Destruction: RAII-style cleanup
```

####### 2.1.6.6.1.4 Permission Transitions

**Subtyping hierarchy:**

```
[Sub-String-Permission]
own String <: mut String <: String
```

**Explicit transitions:**

```cantrip
let s: own String = String.from("data")

// 1. Pass as immutable reference (keep ownership)
read(s)           // s: String (reference)

// 2. Pass as mutable reference (keep ownership)
modify(mut s)     // s: mut String (mutable reference)

// 3. Transfer ownership (lose access)
consume(move s)   // s: own String (moved)
// s no longer accessible
```

**Complex example showing all transitions:**

```cantrip
function demonstrate_permissions()
    uses alloc.heap, io.write
{
    var text: own String = String.from("initial")

    // Transition 1: own → reference (temporary, keep ownership)
    let len1 = get_length(text)
    std.io.println(format!("Length: {}", len1))

    // Transition 2: own → mut reference (temporary, keep ownership)
    append_data(mut text)

    // Transition 3: own → reference again
    let len2 = get_length(text)

    // Transition 4: own → own (permanent move)
    send_to_logger(move text)

    // text no longer accessible
}
```

####### 2.1.6.6.1.5 Safety Guarantees

**What Cantrip PREVENTS:**

1. **Use-after-free** (regions enforce lifetime):

```cantrip
function safe_example(): String {
    region temp {
        let s = String.new_in<temp>()
        s.push_str("data")
        // return s  // ERROR E8301: Cannot escape region
    }
}
```

2. **Double-free** (one owner only):

```cantrip
function safe_example()
    uses alloc.heap
{
    let s = String.from("data")
    consume(move s)
    // consume(move s)  // ERROR E3004: Value already moved
}
```

3. **Memory leaks** (automatic destruction):

```cantrip
function safe_example()
    uses alloc.heap
{
    let s = String.from("data")
    // No need for explicit free - automatic cleanup
}  // s destroyed here, memory reclaimed
```

**What Cantrip does NOT prevent (programmer's responsibility):**

1. **Aliasing bugs** (multiple `mut` refs allowed):

```cantrip
// ALLOWED but potentially dangerous
var text = String.from("data")
modify1(mut text)
modify2(mut text)  // Could conflict with modify1
```

2. **Data races** (no automatic synchronization):

```cantrip
// Programmer must use proper synchronization for threads
thread1.spawn(|| modify(mut shared_string))
thread2.spawn(|| modify(mut shared_string))  // Race condition!
```

3. **Modification during iteration** (no iterator invalidation protection):

```cantrip
var text = String.from("data")
for c in text.chars() {
    text.push_str("x")  // ALLOWED but dangerous!
}
```

**Error codes:**

- `E3002` — Attempted modification of immutable reference
- `E3004` — Use of moved value
- `E8301` — Region-allocated value cannot escape scope

**Design philosophy:**

> Cantrip provides memory safety guarantees (no use-after-free, no leaks) through region-based escape analysis and permission tracking. Aliasing safety is managed through permissions and effects, trading compile-time restrictions for runtime flexibility.

###### 2.1.6.6.2 Region Integration

**Definition 2.1.6.5 (Region-Allocated String):** A `String` allocated within a memory region for fast bulk deallocation. Region-allocated strings cannot escape their region scope.

**Philosophy:** Regions control WHEN memory is freed (lexical scope), not WHO can access it. Perfect for temporary string processing.

####### 2.1.6.6.2.1 Region Allocation Syntax

**Basic syntax:**

```cantrip
region name {
    let s: own String = String.new_in<name>()
    // s allocated in region 'name'
}  // s freed here (O(1) bulk deallocation)
```

**Type rule:**

```
[T-String-Region]
Γ ⊢ region r active
─────────────────────────────────────
Γ ⊢ String.new_in<r>() : own String
    uses alloc.region
```

**Complete example:**

```cantrip
function parse_file(path: String): Result<Data, Error>
    uses alloc.region, io.read
{
    region temp {
        // Allocate parsing buffer in region
        let contents: own String = String.new_in<temp>()

        // Read file into region-allocated buffer
        std.fs.read_to_string(path, mut contents)?

        // Parse (may use more temp strings)
        let parsed = parse_json(contents)?

        // Must return heap-allocated data
        Ok(parsed.to_heap())
    }  // All temp strings freed in O(1)
}
```

####### 2.1.6.6.2.2 String Region Methods

**Standard allocation methods:**

| Method                           | Signature               | Allocation  |
| -------------------------------- | ----------------------- | ----------- |
| `String.new()`                   | `map(()) → own String`      | Heap        |
| `String.new_in<'r>()`            | `map(()) → own String`      | Region `'r` |
| `String.from(s)`                 | `map(str) → own String`     | Heap        |
| `String.from_in<'r>(s)`          | `map(str) → own String`     | Region `'r` |
| `String.with_capacity(n)`        | `map(usize) → own String`   | Heap        |
| `String.with_capacity_in<'r>(n)` | `map(usize) → own String`   | Region `'r` |

**Example usage:**

```cantrip
region temp {
    // Empty string in region
    let s1 = String.new_in<temp>()

    // From literal
    let s2 = String.from_in<temp>("hello")

    // With capacity
    let s3 = String.with_capacity_in<temp>(1024)
}
```

####### 2.1.6.6.2.3 Lifetime Enforcement

**Cannot escape region:**

```cantrip
function bad_example(): own String
    uses alloc.region
{
    region temp {
        let s = String.new_in<temp>()
        s.push_str("data")
        s  // ERROR E8301: Cannot return region-allocated value
    }
}
```

**Compiler error:**

```
Error E8301: Value allocated in region 'temp' cannot escape region scope
  --> example.ct:5:9
   |
3  |     region temp {
   |     ----------- region 'temp' declared here
4  |         let s = String.new_in<temp>()
   |                 --------------------- allocated in region 'temp'
5  |         s  // ERROR
   |         ^ cannot return region-allocated value
   |
   = note: Region-allocated values are freed when region ends
   = help: Convert to heap: `s.to_heap()` or `s.clone()`
```

**Correct pattern - convert to heap:**

```cantrip
function safe_example(): own String
    uses alloc.region, alloc.heap
{
    region temp {
        var s = String.new_in<temp>()
        s.push_str("data")
        s.to_heap()  // OK: Deep copy to heap
    }
}
```

**Type rule for escape prevention:**

```
[Region-Escape]
Γ ⊢ region r { e }    e : T    alloc(e, r)    e escapes r
──────────────────────────────────────────────────────────
ERROR E8301: Cannot return region-allocated value
```

####### 2.1.6.6.2.4 Performance Characteristics

**Allocation comparison:**

| Operation      | Heap          | Region     | Speedup   |
| -------------- | ------------- | ---------- | --------- |
| Allocation     | 50-100 cycles | 3-5 cycles | 10-20×    |
| Deallocation   | O(n) frees    | O(1) bulk  | 100-1000× |
| Cache locality | Scattered     | Contiguous | 2-4×      |

**Region allocation algorithm:**

```
1. Check: current_page has space?
2. Yes: bump pointer (3-5 cycles)
3. No: allocate new page, update list
4. Return: pointer to allocation
5. Region end: free entire page list (O(1))
```

**Memory layout:**

```
Region: temp
├── Page 1 (4 KB)    [String1][String2][String3]▒▒▒▒
├── Page 2 (8 KB)    [String4][String5]▒▒▒▒▒▒▒▒▒▒▒▒▒
└── Page 3 (16 KB)   [String6]▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
                      ↑
                      allocation_ptr
```

####### 2.1.6.6.2.5 Region vs Heap Decision

**Use heap allocation (`String.new()`)** when:

- String uses to outlive current function
- String lifetime is unpredictable
- Building long-lived data structures
- String uses to be stored in persistent collections

**Use region allocation (`String.new_in<r>()`)** when:

- String only needed temporarily (within function/phase)
- Batch processing many short-lived strings
- Performance-critical code with many allocations
- Parsing, lexing, or compilation phases

**Decision flowchart:**

```
Does string need to escape function?
├─ Yes → Use heap allocation
└─ No → Is this performance-critical?
    ├─ Yes → Use region allocation
    └─ No → Either (prefer heap for simplicity)
```

####### 2.1.6.6.2.6 Nested Regions

**Regions support nesting (LIFO order):**

```cantrip
function nested_example()
    uses alloc.region
{
    region outer {
        let s1 = String.from_in<outer>("outer")

        region inner {
            let s2 = String.from_in<inner>("inner")

            // Can access outer from inner
            s1.push_str(" modified")

            // Cannot return s2 (would escape inner)
        }  // s2 freed here

        // s1 still valid
        s1.push_str(" again")
    }  // s1 freed here
}
```

**Region hierarchy rules:**

1. Inner regions deallocate before outer (LIFO)
2. Inner regions CAN access outer region values
3. Outer regions CANNOT access inner region values
4. Cannot return inner-region values to outer scopes

**Formal semantics:**

```
[Region-LIFO]
r₂ nested in r₁
──────────────────────────────────────
dealloc(r₂) <ʜᴀᴘᴘᴇɴs-ʙᴇғᴏʀᴇ> dealloc(r₁)

[Region-Nesting]
Γ, r₁ : Region ⊢ region r₂ { e₂ }    parent(r₂) = r₁
──────────────────────────────────────────────────────
Γ ⊢ e₂ can reference values in r₁
```

####### 2.1.6.6.2.7 Common Patterns

**Pattern 1: Parsing with regions**

```cantrip
function tokenize(source: String): Vec<Token>
    uses alloc.heap, alloc.region
{
    var tokens = Vec.new()  // Heap (return value)

    region temp {
        for line in source.lines() {
            // Temporary string for processing
            var trimmed = String.new_in<temp>()
            trimmed.push_str(line.trim())

            let token = parse_token(trimmed)
            tokens.push(token)  // token copied to heap

            // trimmed stays in region (will be freed)
        }
    }  // All temp strings freed (O(1))

    tokens  // Return heap-allocated result
}
```

**Pattern 2: Request processing**

```cantrip
function handle_request(request: Request): Response
    uses alloc.region, alloc.heap
{
    region request_scope {
        // Parse request (many temp strings)
        var body = String.new_in<request_scope>()
        request.read_body(mut body)?

        var headers = parse_headers_in<request_scope>(request)
        let data = extract_data(body, headers)?

        // Return heap-allocated response
        Response.from_data(data)
    }  // All request temps freed instantly
}
```

**Pattern 3: Batch string processing**

```cantrip
function process_batch(items: Vec<String>): Vec<Result>
    uses alloc.region, alloc.heap
{
    var results = Vec.new()

    region batch {
        for item in items {
            // Temporary processing strings
            var normalized = String.new_in<batch>()
            normalized.push_str(item.to_lowercase())

            var cleaned = String.new_in<batch>()
            cleaned.push_str(normalized.trim())

            let result = process(cleaned)
            results.push(result)  // result copied to heap

            // normalized, cleaned stay in region
        }
    }  // O(1) bulk free - perfect for batch jobs

    results
}
```

**Pattern 4: Compilation phases**

```cantrip
function compile(source: String): Program
    uses alloc.region, alloc.heap
{
    region parsing {
        var tokens = lex_into_strings<parsing>(source)

        region analysis {
            let ast = parse_in<analysis>(tokens)
            let optimized = optimize(ast)

            codegen(optimized)  // Return heap-allocated Program
        }  // Analysis temps freed
    }  // Parsing temps freed
}
```

####### 2.1.6.6.2.8 Converting Between Heap and Region

**Methods:**

```cantrip
impl String {
    // Clone from heap to region
    function clone_in<'r>(self: String): own String
        uses alloc.region

    // Convert region → heap (deep copy)
    function to_heap(self: own String): own String
        uses alloc.heap
}
```

**Example:**

```cantrip
function example()
    uses alloc.heap, alloc.region
{
    let heap_str = String.from("data")

    region temp {
        // Clone heap string into region
        let region_str = heap_str.clone_in<temp>()

        // Process in region
        region_str.push_str(" processed")

        // Convert back to heap if needed
        let result = region_str.to_heap()

        // result escapes, region_str freed
    }
}
```

####### 2.1.6.6.2.9 Permissions Still Apply

**Regions control lifetime, not access:**

```cantrip
function example()
    uses alloc.region
{
    region temp {
        var text = String.new_in<temp>()

        // Permissions work as normal:
        read(text)          // Immutable reference
        modify(mut text)    // Mutable reference
        read(text)          // Can reference again

        // Multiple mut refs still allowed
        modify1(mut text)
        modify2(mut text)
    }  // text freed (region determines WHEN)
}
```

**Key insight:**

- **Permissions** control WHO can do WHAT (read/write/move)
- **Regions** control WHEN memory is freed (scope-based)
- These are orthogonal concerns in Cantrip

**Error codes:**

- `E8301` — Region-allocated value cannot escape region scope
- `E8302` — Region reference lifetime violation
- `E8303` — Invalid region nesting

###### 2.1.6.6.3 String Operations

String provides a comprehensive API for construction, access, modification, and manipulation of UTF-8 text. Operations are organized by category and permission requirements.

####### 2.1.6.6.3.1 Construction Operations

**Empty string creation:**

```cantrip
function String.new(): own String
    uses alloc.heap
```

Creates an empty String with zero length. May allocate minimal capacity.

**Example:**

```cantrip
function create_buffer(): own String
    uses alloc.heap
{
    String.new()  // Returns empty String
}
```

**From UTF-8 bytes (validated):**

```cantrip
function String.from_utf8(bytes: [u8]): Result<own String, Utf8Error>
    uses alloc.heap
```

Constructs a String from a byte array, validating UTF-8 encoding. Returns error if bytes are invalid UTF-8.

**Example:**

```cantrip
function parse_bytes(data: [u8]): Result<own String, Utf8Error>
    uses alloc.heap
{
    String.from_utf8(data)  // Validates UTF-8
}

let valid_bytes: [u8] = [72, 101, 108, 108, 111]  // "Hello"
let s = String.from_utf8(valid_bytes)?  // Ok(String)

let invalid_bytes: [u8] = [0xFF, 0xFE]  // Invalid UTF-8
let err = String.from_utf8(invalid_bytes)  // Err(Utf8Error)
```

**From UTF-8 bytes (unchecked):**

```cantrip
function String.from_utf8_unchecked(bytes: [u8]): own String
    uses alloc.heap
```

Constructs a String from bytes without validation. **Unsafe**: Violates UTF-8 invariant if bytes are invalid.

**Example:**

```cantrip
// ONLY use if you KNOW bytes are valid UTF-8
function trusted_conversion(validated_bytes: [u8]): own String
    uses alloc.heap
{
    String.from_utf8_unchecked(validated_bytes)  // No validation overhead
}
```

**Warning:** Using `from_utf8_unchecked` with invalid UTF-8 creates undefined behavior. Use `from_utf8` unless performance is critical and data is pre-validated.

**From string slice:**

```cantrip
function String.from(slice: str): own String
    uses alloc.heap
```

Constructs a String by copying data from a string slice. Allocation required for heap storage.

**Example:**

```cantrip
function duplicate_literal(): own String
    uses alloc.heap
{
    let literal: str = "Hello, world!"
    String.from(literal)  // Allocates and copies
}
```

**With initial capacity:**

```cantrip
function String.with_capacity(cap: usize): own String
    uses alloc.heap
```

Creates an empty String with pre-allocated capacity. Useful for avoiding reallocations when final size is known.

**Example:**

```cantrip
function build_large_string(parts: [str]): own String
    uses alloc.heap
{
    let total_len: usize = parts.map(|p| p.len()).sum()
    let mut result: own String = String.with_capacity(total_len)

    for part in parts {
        result.push_str(part)  // No reallocation needed
    }

    result
}
```

**Region-allocated string:**

```cantrip
function String.new_in<'r>(): own String
    uses alloc.region
```

Creates an empty String allocated in the specified region instead of the heap. String lifetime is bound to the region.

**Example:**

```cantrip
function temporary_strings(): own String
    uses alloc.heap, alloc.region
{
    region temp {
        let s: own String = String.new_in<temp>()
        s.push_str("temporary")

        // Cannot return s (region-bound)
        // Must convert to heap:
        String.from(s.as_str())  // Heap copy
    }  // s deallocated here
}
```

####### 2.1.6.6.3.2 Access Operations (Immutable)

**Get byte length:**

```cantrip
function len(self: String): usize
```

Returns the length of the String in bytes (NOT characters). O(1) operation.

**Example:**

```cantrip
let s: own String = String.from("Hello🦀")
assert!(s.len() == 10)  // 5 ASCII bytes + 4 byte emoji
```

**Check if empty:**

```cantrip
function is_empty(self: String): bool
```

Returns `true` if the String contains zero bytes. Equivalent to `self.len() == 0`.

**Example:**

```cantrip
let empty: own String = String.new()
assert!(empty.is_empty())

let non_empty: own String = String.from("x")
assert!(!non_empty.is_empty())
```

**Get capacity:**

```cantrip
function capacity(self: String): usize
```

Returns the allocated capacity in bytes. Capacity is always greater than or equal to length.

**Example:**

```cantrip
let s: own String = String.with_capacity(100)
assert!(s.capacity() >= 100)
assert!(s.len() == 0)
```

**Convert to str slice:**

```cantrip
function as_str(self: String): str
```

Returns a string slice view of the String's contents. Zero-copy operation.

**Example:**

```cantrip
function print_text(text: str) {
    println(text)
}

function example()
    uses alloc.heap
{
    let s: own String = String.from("data")
    print_text(s.as_str())  // Explicit conversion
    print_text(s)  // Or use deref coercion
}
```

**Get bytes:**

```cantrip
function as_bytes(self: String): [u8]
```

Returns a byte slice view of the String's UTF-8 data. Zero-copy operation.

**Example:**

```cantrip
let s: own String = String.from("ABC")
let bytes: [u8] = s.as_bytes()
assert!(bytes[0] == 65)  // 'A'
assert!(bytes[1] == 66)  // 'B'
assert!(bytes[2] == 67)  // 'C'
```

####### 2.1.6.6.3.3 Modification Operations (Requires own or mut)

**Append string:**

```cantrip
function push_str(self: mut String, s: str)
    uses alloc.heap
```

Appends a string slice to the end of the String. May reallocate if capacity is insufficient.

**Example:**

```cantrip
function concatenate(a: str, b: str): own String
    uses alloc.heap
{
    let mut result: own String = String.from(a)
    result.push_str(b)
    result
}
```

**Append character:**

```cantrip
function push(self: mut String, ch: char)
    uses alloc.heap
```

Appends a Unicode scalar value to the String. Character is encoded as UTF-8 (1-4 bytes).

**Example:**

```cantrip
let mut s: own String = String.from("Hello")
s.push(' ')
s.push('🦀')
assert!(s.as_str() == "Hello 🦀")
```

**Insert string at position:**

```cantrip
function insert_str(self: mut String, idx: usize, s: str)
    uses alloc.heap
    must idx <= self.len()
    must is_char_boundary(self, idx)
```

Inserts a string slice at the specified byte index. May reallocate. Index must be at a UTF-8 character boundary.

**Example:**

```cantrip
let mut s: own String = String.from("HelloWorld")
s.insert_str(5, ", ")  // Index 5 is after "Hello"
assert!(s.as_str() == "Hello, World")

// ERROR: Invalid boundary
// s.insert_str(1, "x")  // Panic if idx=1 is mid-character
```

**Remove range:**

```cantrip
function remove(self: mut String, start: usize, end: usize)
    must start <= end
    must end <= self.len()
    must is_char_boundary(self, start)
    must is_char_boundary(self, end)
```

Removes characters in the byte range [start, end). Indices must be at UTF-8 character boundaries.

**Example:**

```cantrip
let mut s: own String = String.from("Hello, World!")
s.remove(5, 7)  // Remove ", "
assert!(s.as_str() == "HelloWorld!")
```

**Clear all contents:**

```cantrip
function clear(self: mut String)
```

Removes all characters from the String, setting length to 0. Capacity is unchanged.

**Example:**

```cantrip
let mut s: own String = String.from("temporary")
let old_cap: usize = s.capacity()

s.clear()
assert!(s.is_empty())
assert!(s.capacity() == old_cap)  // Capacity preserved
```

**Truncate to length:**

```cantrip
function truncate(self: mut String, new_len: usize)
    must new_len <= self.len()
    must is_char_boundary(self, new_len)
```

Shortens the String to the specified byte length. `new_len` must be at a character boundary.

**Example:**

```cantrip
let mut s: own String = String.from("Hello, World!")
s.truncate(5)  // Keep only "Hello"
assert!(s.as_str() == "Hello")
```

**Reserve additional capacity:**

```cantrip
function reserve(self: mut String, additional: usize)
    uses alloc.heap
```

Reserves capacity for at least `additional` more bytes. May reallocate if current capacity is insufficient.

**Example:**

```cantrip
function build_string(parts: [str]): own String
    uses alloc.heap
{
    let mut s: own String = String.new()
    let total: usize = parts.map(|p| p.len()).sum()

    s.reserve(total)  // Pre-allocate

    for part in parts {
        s.push_str(part)  // No reallocation
    }

    s
}
```

####### 2.1.6.6.3.4 View Operations (Zero-Copy)

**Slice to str:**

```cantrip
function slice(self: String, start: usize, end: usize): str
    must start <= end
    must end <= self.len()
    must is_char_boundary(self, start)
    must is_char_boundary(self, end)
```

Returns a string slice view of the byte range [start, end). Zero-copy operation. Indices must be at UTF-8 character boundaries.

**Example:**

```cantrip
let s: own String = String.from("Hello, World!")
let hello: str = s.slice(0, 5)  // "Hello"
let world: str = s.slice(7, 12) // "World"
```

**Split by delimiter:**

```cantrip
function split(self: String, delimiter: str): Iterator<str>
    must !delimiter.is_empty()
```

Returns an iterator over substrings separated by the delimiter. Delimiter is not included in results.

**Example:**

```cantrip
let s: own String = String.from("one,two,three")
let parts: [str] = s.split(",").collect()
assert!(parts.len() == 3)
assert!(parts[0] == "one")
assert!(parts[1] == "two")
assert!(parts[2] == "three")
```

**Get lines:**

```cantrip
function lines(self: String): Iterator<str>
```

Returns an iterator over lines (substrings separated by `\n` or `\r\n`). Line terminators are not included in results.

**Example:**

```cantrip
let doc: own String = String.from("Line 1\nLine 2\nLine 3")
for line in doc.lines() {
    println(line)
}
// Output:
// Line 1
// Line 2
// Line 3
```

**Trim whitespace:**

```cantrip
function trim(self: String): str
```

Returns a string slice with leading and trailing whitespace removed. Zero-copy operation.

**Example:**

```cantrip
let s: own String = String.from("  hello  ")
let trimmed: str = s.trim()
assert!(trimmed == "hello")
```

####### 2.1.6.6.3.5 Iteration Operations

**Iterate bytes:**

```cantrip
function bytes(self: String): Iterator<u8>
```

Returns an iterator over the String's bytes. Each byte is yielded individually.

**Example:**

```cantrip
let s: own String = String.from("ABC")
for byte in s.bytes() {
    println("{byte}")  // 65, 66, 67
}
```

**Iterate characters:**

```cantrip
function chars(self: String): Iterator<char>
```

Returns an iterator over Unicode scalar values (characters). Multi-byte UTF-8 sequences are decoded.

**Example:**

```cantrip
let s: own String = String.from("Hello🦀")
for ch in s.chars() {
    println("{ch}")  // H, e, l, l, o, 🦀
}
```

####### 2.1.6.6.3.6 Comparison Operations

**Equality (bytes):**

```cantrip
function equals(self: String, other: String): bool
```

Returns `true` if both Strings contain identical byte sequences. Equivalent to `==` operator.

**Example:**

```cantrip
let a: own String = String.from("hello")
let b: own String = String.from("hello")
let c: own String = String.from("world")

assert!(a.equals(b))
assert!(!a.equals(c))
```

**Lexicographic comparison:**

```cantrip
function compare(self: String, other: String): Ordering
```

Compares Strings lexicographically by byte values. Returns `Less`, `Equal`, or `Greater`.

**Example:**

```cantrip
let a: own String = String.from("apple")
let b: own String = String.from("banana")

match a.compare(b) {
    Ordering::Less => println("apple < banana"),
    Ordering::Equal => println("equal"),
    Ordering::Greater => println("apple > banana"),
}
```

**Starts with prefix:**

```cantrip
function starts_with(self: String, prefix: str): bool
```

Returns `true` if the String begins with the specified prefix.

**Example:**

```cantrip
let s: own String = String.from("https://example.com")
assert!(s.starts_with("https://"))
assert!(!s.starts_with("http://"))
```

**Ends with suffix:**

```cantrip
function ends_with(self: String, suffix: str): bool
```

Returns `true` if the String ends with the specified suffix.

**Example:**

```cantrip
let filename: own String = String.from("document.txt")
assert!(filename.ends_with(".txt"))
assert!(!filename.ends_with(".pdf"))
```

###### 2.1.6.6.4 Indexing and Iteration

####### 2.1.6.6.4.1 Why No Direct Indexing

String does NOT support direct indexing via `s[i]` syntax. This is a deliberate design decision based on UTF-8's variable-width encoding.

**Problem with character indexing:**

UTF-8 encodes Unicode scalar values using 1-4 bytes per character. Direct indexing creates ambiguity:

```cantrip
let s: own String = String.from("Hello🦀World")  // Contains emoji (4 bytes)

// If s[6] were allowed, what would it return?
// Option 1: Byte at index 6? (middle of emoji - INVALID UTF-8!)
// Option 2: 6th character? (requires O(n) scan from start)

// ERROR: s[6] is not allowed
// let ch = s[6]  // Compile error: String does not support indexing
```

**Why byte indexing is unsafe:**

```cantrip
// UTF-8 encoding of "Hello🦀":
// H     e     l     l     o     🦀              (characters)
// 0x48  0x65  0x6C  0x6C  0x6F  0xF0 0x9F 0xA6 0x80  (bytes)
// 0     1     2     3     4     5    6    7    8     (indices)

// Accessing index 6 would return 0x9F (middle of 🦀)
// This is NOT a valid UTF-8 sequence!
```

**Why character indexing is avoided:**

Users typically expect `[i]` to be O(1) constant-time. However, UTF-8 character access must scanning from the start:

```cantrip
// To find the nth character, must scan from beginning
function nth_char_naive(s: String, n: usize): Option<char> {
    let mut count: usize = 0
    for ch in s.chars() {
        if count == n {
            return Some(ch)
        }
        count = count + 1
    }
    None
}
// O(n) operation!
```

**Rationale:**

By prohibiting `s[i]`, Cantrip makes performance characteristics explicit:

- Byte iteration: `s.bytes()` - clearly O(1) per byte
- Character iteration: `s.chars()` - clearly must decoding
- Slicing: `s.slice(start, end)` - must explicit boundary indices

####### 2.1.6.6.4.2 Byte Iteration

For byte-level access (protocols, parsing, low-level operations):

```cantrip
function process_bytes(s: String) {
    for byte in s.bytes() {
        // byte has type u8
        if byte == 0x0A {  // Newline
            println("Found newline")
        }
    }
}
```

**Performance:** O(1) per byte (direct array access).

**Use cases:**

- Binary protocol parsing
- ASCII-only processing
- Byte-level validation
- Low-level file I/O

####### 2.1.6.6.4.3 Character Iteration

For Unicode scalar value access:

```cantrip
function count_chars(s: String): usize {
    let mut count: usize = 0
    for ch in s.chars() {
        // ch has type char (Unicode scalar value)
        count = count + 1
    }
    count
}

let s: own String = String.from("Hello🦀")
assert!(count_chars(s) == 6)  // 5 ASCII + 1 emoji
```

**Performance:** O(n) to traverse entire string. Getting the nth character must O(n) scan.

**Example: Finding nth character:**

```cantrip
function nth_char(s: String, n: usize): Option<char> {
    let mut iter = s.chars()
    iter.skip(n).next()  // O(n) operation
}

let s: own String = String.from("Hello")
assert!(nth_char(s, 0) == Some('H'))
assert!(nth_char(s, 4) == Some('o'))
assert!(nth_char(s, 10) == None)
```

####### 2.1.6.6.4.4 Grapheme Clusters

**Important caveat:** `.chars()` returns Unicode scalar values (code points), NOT grapheme clusters (user-perceived characters).

```cantrip
// Example: é can be represented two ways
let composed: str = "é"     // U+00E9 (single code point)
let decomposed: str = "é"   // U+0065 U+0301 (e + combining accent)

let s1: own String = String.from(composed)
let s2: own String = String.from(decomposed)

// Character counts differ!
assert!(s1.chars().count() == 1)  // 1 code point
assert!(s2.chars().count() == 2)  // 2 code points (e + accent)

// But they look identical when displayed:
println("{}", s1)  // é
println("{}", s2)  // é
```

**For true grapheme iteration:**

Use the Unicode segmentation library (not in core):

```cantrip
// Hypothetical library usage:
// use std.unicode.graphemes
//
// for grapheme in s.graphemes() {
//     // grapheme is a user-perceived character
// }
```

####### 2.1.6.6.4.5 Safe Slicing

Slicing must respect UTF-8 character boundaries to preserve the UTF-8 invariant:

```cantrip
let s: own String = String.from("Hello🦀World")

// OK: Slices at character boundaries
let hello: str = s.slice(0, 5)  // "Hello"
let world: str = s.slice(9, 14) // "World"

// PANIC: Index 7 is mid-character (inside 🦀 emoji)
// let bad: str = s.slice(0, 7)  // Runtime panic!
// Error: "byte index 7 is not a char boundary"
```

**Character boundary rules:**

A byte index is a valid character boundary if:

1. It equals 0 or `s.len()`, OR
2. The byte at that index is NOT a UTF-8 continuation byte (0x80-0xBF)

```cantrip
// UTF-8 continuation bytes: 10xxxxxx (0x80-0xBF)
function is_char_boundary(s: String, idx: usize): bool {
    if idx > s.len() {
        return false
    }
    if idx == 0 || idx == s.len() {
        return true
    }

    let byte: u8 = s.as_bytes()[idx]
    (byte & 0xC0) != 0x80  // NOT a continuation byte
}
```

**Safe slicing pattern:**

When you need to slice at a specific character position:

```cantrip
function slice_first_n_chars(s: String, n: usize): str {
    let mut char_count: usize = 0
    let mut byte_idx: usize = 0

    for (idx, ch) in s.char_indices() {
        if char_count >= n {
            break
        }
        byte_idx = idx + ch.len_utf8()
        char_count = char_count + 1
    }

    s.slice(0, byte_idx)
}

let s: own String = String.from("Hello🦀World")
let first5: str = slice_first_n_chars(s, 5)
assert!(first5 == "Hello")
```

###### 2.1.6.6.5 Common Pitfalls

**Pitfall 1: UTF-8 Boundary Violation**

```cantrip
// WRONG: Slicing mid-character
function bad_slice()
    uses alloc.heap
{
    let s: own String = String.from("Hello🦀")

    // 🦀 is 4 bytes: 0xF0 0x9F 0xA6 0x80
    // Byte indices: H=0, e=1, l=2, l=3, o=4, 🦀=5-8

    let bad: str = s.slice(0, 7)  // PANIC at runtime!
    // Error: "byte index 7 is not a char boundary"
}

// CORRECT: Use character iteration or valid boundaries
function good_slice()
    uses alloc.heap
{
    let s: own String = String.from("Hello🦀")

    // Option 1: Slice at known character boundaries
    let hello: str = s.slice(0, 5)  // "Hello"

    // Option 2: Use character iteration
    let first5: own String = String.new()
    for (idx, ch) in s.chars().enumerate() {
        if idx < 5 {
            first5.push(ch)
        }
    }
}
```

**Explanation:** UTF-8 multi-byte characters cannot be split. Index 7 falls inside the 🦀 emoji (4 bytes at indices 5-8), causing a panic. Always slice at character boundaries or use character-based iteration.

**Pitfall 2: Multiple Mutable References (Use-After-Realloc)**

```cantrip
// DANGEROUS: Reference invalidation
function dangerous(s: mut String)
    uses alloc.heap
{
    let bytes: [u8] = s.as_bytes()  // Get reference to byte data
    let first: u8 = bytes[0]        // Read from reference

    s.push_str("x")  // Modifies String, MAY reallocate!

    // bytes pointer is now INVALID if reallocation occurred!
    // Accessing bytes[1] could read freed memory (undefined behavior)
    // let second: u8 = bytes[1]  // USE-AFTER-FREE!
}

// SAFE: Don't hold references across mutations
function safe(s: mut String)
    uses alloc.heap
{
    // Copy values, not references
    let len: usize = s.len()
    let first: u8 = s.as_bytes()[0]

    s.push_str("x")  // OK: no outstanding references

    // Re-acquire reference after mutation
    let new_len: usize = s.len()
    assert!(new_len == len + 1)
}
```

**Explanation:** Unlike Rust, Cantrip allows multiple mutable references and doesn't prevent invalidation. Programmer must ensure references aren't held across reallocating operations. Copy values instead of holding references.

**Pitfall 3: Assuming Character Count = Byte Count**

```cantrip
// WRONG: Confusing bytes and characters
function bad_count()
    uses alloc.heap
{
    let s: own String = String.from("Hello🦀")
    let char_count: usize = s.len()  // Returns 10 (bytes), not 6!
    println("Characters: {char_count}")  // Wrong: prints 10
}

// CORRECT: Use chars().count()
function good_count()
    uses alloc.heap
{
    let s: own String = String.from("Hello🦀")

    let byte_count: usize = s.len()           // 10 bytes
    let char_count: usize = s.chars().count() // 6 characters

    println("Bytes: {byte_count}")      // 10
    println("Characters: {char_count}") // 6
}
```

**Explanation:** `len()` returns byte length, not character count. Use `chars().count()` for character count, but note this is O(n) because it must decode the entire UTF-8 string.

**Pitfall 4: Region Escape Attempt**

```cantrip
// WRONG: Trying to return region-allocated string
function bad_region(): own String
    uses alloc.region
{
    region temp {
        let s: own String = String.new_in<temp>()
        s.push_str("data")
        s  // ERROR: cannot escape region (lifetime violation)
    }
}

// CORRECT: Convert to heap before returning
function good_region(): own String
    uses alloc.heap, alloc.region
{
    region temp {
        let s: own String = String.new_in<temp>()
        s.push_str("data")

        // Copy to heap before escaping region
        String.from(s.as_str())  // Heap allocation
    }  // s freed here (region ends)
}
```

**Explanation:** Region-allocated Strings cannot escape their region's scope. To return a region-built string, copy it to the heap using `String.from()` before the region ends.

**Pitfall 5: Grapheme vs. Character Count**

```cantrip
// WRONG: Assuming chars() gives grapheme count
function bad_grapheme()
    uses alloc.heap
{
    // "é" can be U+00E9 (composed) or U+0065 U+0301 (decomposed)
    let s1: own String = String.from("é")  // Composed (1 code point)
    let s2: own String = String.from("é")  // Decomposed (2 code points)

    // Character counts differ!
    assert!(s1.chars().count() == 1)  // Passes
    assert!(s2.chars().count() == 1)  // FAILS! (actually 2)
}

// CORRECT: Be aware of normalization
function good_grapheme()
    uses alloc.heap
{
    let s: own String = String.from("é")  // Could be 1 or 2 code points

    // For user-perceived character count, need Unicode library:
    // use std.unicode.graphemes
    // let grapheme_count = s.graphemes().count()

    // Or normalize before counting:
    // let normalized = s.nfc()  // NFC normalization
    // let char_count = normalized.chars().count()
}
```

**Explanation:** Unicode allows multiple representations of the same visual character. `chars()` counts code points, not graphemes (user-perceived characters). For accurate user-visible character counts, use Unicode segmentation libraries.

---

## 2.1.7 Built-In Sum Types

Cantrip provides two fundamental sum types that are so pervasive they are considered built-in primitives despite being implemented as enums. These types provide the foundation for Cantrip's error handling and null safety.

**Definition 2.1.7.1 (Built-In Sum Types):** The types `Option<T>` and `Result<T, E>` are built-in generic sum types that provide type-safe alternatives to null pointers and exception-based error handling. While structurally implemented as enums, they receive special treatment in the type system and integrate deeply with Cantrip's contract and effect systems.

### 2.1.7.1 Option<T>

#### Overview

**Key innovation/purpose:** `Option<T>` provides type-safe optional values, eliminating null pointer errors by making the absence of a value explicit in the type system.

##### Guidance (non-normative)

- Wrap values that may be absent in `Option`; rely on the matching rules in §2.1.7.1 for exhaustive handling.
- For richer error information, switch to `Result`; for booleans or empty collections, keep the simpler types.
- Remember that `Option<Ptr<T>>` and other niche-optimized cases follow the layout guarantees spelled out in §2.1.7.1.

#### Syntax

**Type definition (conceptual - actually built-in):**

```cantrip
enum Option<T> {
    Some(T),
    None,
}
```

**Construction:**

```cantrip
// Create Some variant
let some_value: Option<i32> = Option.Some(42)

// Create None variant
let no_value: Option<i32> = Option.None
```

**Pattern matching:**

```cantrip
match some_value {
    Option.Some(x) => println("Got value: {x}"),
    Option.None => println("No value"),
}
```

#### Static Semantics

**Type rules:**

```
[T-Option-Some]
Γ ⊢ e : T
──────────────────────────
Γ ⊢ Option.Some(e) : Option<T>

[T-Option-None]
──────────────────────────
Γ ⊢ Option.None : Option<T>
    (T inferred from context)
```

**Pattern matching rule:**

```
[T-Option-Match]
Γ ⊢ e : Option<T>
Γ, x : T ⊢ e_some : U
Γ ⊢ e_none : U
──────────────────────────────────────────────
Γ ⊢ match e {
      Option.Some(x) => e_some,
      Option.None => e_none
    } : U
```

#### Memory Representation

**Standard layout:**

```
Option<T>:
┌──────────────┬──────────────────────┐
│ Discriminant │    Payload (T)       │
│  (1-8 bytes) │  (sizeof(T) bytes)   │
└──────────────┴──────────────────────┘

Size: max(sizeof(T) + discriminant_size, pointer_size)
Alignment: max(alignof(T), alignof(discriminant))
```

**Niche optimization (for non-nullable pointers):**

```
Option<Ptr<T>@Exclusive>:
┌──────────────┐
│   Pointer    │  Some = valid pointer, None = null (0x0)
│   8 bytes    │  No discriminant needed!
└──────────────┘

Size: 8 bytes (same as Ptr<T>)
Alignment: 8 bytes
```

**Theorem 2.1.7.1 (Niche Optimization):** For types T with invalid bit patterns (e.g., non-nullable pointers), `sizeof(Option<T>) = sizeof(T)`. The compiler uses the invalid pattern to represent `None`.

#### Working with Option Values

**Primary mechanism:** Pattern matching (exhaustive checking enforced by compiler)

```cantrip
// Extract value with pattern matching
match optional_value {
    Option.Some(x) => {
        // Use x here
        process(x)
    },
    Option.None => {
        // Handle absence
        use_default()
    },
}

// Conditional binding
if let Option.Some(x) = optional_value {
    // x is bound here
    use_value(x)
}
```

**Note:** Standard library (Part XII) provides utility procedures for common operations (checking presence, extracting with defaults, etc.), but pattern matching is the idiomatic Cantrip approach for Option handling.

### 2.1.7.2 Result<T, E>

#### Overview

**Key innovation/purpose:** `Result<T, E>` provides type-safe error handling without exceptions, making errors explicit in function signatures and enabling exhaustive error checking through pattern matching.

##### Guidance (non-normative)

- Use `Result<T, E>` when failures carry data; follow the typing and matching rules in §2.1.7.2 to propagate errors explicitly.
- Reserve `Option` for presence/absence and `panic` for unrecoverable logic errors.
- Consider defining aliases for frequently reused error types to keep signatures concise.

#### Syntax

**Type definition (conceptual - actually built-in):**

```cantrip
enum Result<T, E> {
    Ok(T),
    Err(E),
}
```

**Construction:**

```cantrip
// Success case
let success: Result<i32, String> = Result.Ok(42)

// Error case
let failure: Result<i32, String> = Result.Err("computation failed")
```

**Pattern matching:**

```cantrip
match success {
    Result.Ok(value) => println("Success: {value}"),
    Result.Err(error) => println("Error: {error}"),
}
```

#### Static Semantics

**Type rules:**

```
[T-Result-Ok]
Γ ⊢ e : T
──────────────────────────
Γ ⊢ Result.Ok(e) : Result<T, E>
    (E inferred from context)

[T-Result-Err]
Γ ⊢ e : E
──────────────────────────
Γ ⊢ Result.Err(e) : Result<T, E>
    (T inferred from context)
```

**Pattern matching rule:**

```
[T-Result-Match]
Γ ⊢ e : Result<T, E>
Γ, x : T ⊢ e_ok : U
Γ, err : E ⊢ e_err : U
──────────────────────────────────────────────
Γ ⊢ match e {
      Result.Ok(x) => e_ok,
      Result.Err(err) => e_err
    } : U
```

**Effect integration:**

```
[Effect-Result]
procedure p(...): Result<T, E> uses effects
──────────────────────────────────────────────
Effects include all potential error-producing effects
```

#### Memory Representation

**Standard layout:**

```
Result<T, E>:
┌──────────────┬──────────────────────────────────┐
│ Discriminant │  Payload (max(T, E))             │
│  (1-8 bytes) │  (max(sizeof(T), sizeof(E)))     │
└──────────────┴──────────────────────────────────┘

Size: discriminant_size + max(sizeof(T), sizeof(E))
Alignment: max(alignof(T), alignof(E), alignof(discriminant))
```

**Example sizes:**

```
Result<i32, String>:
  - Discriminant: 1 byte
  - i32: 4 bytes
  - String: 24 bytes (ptr + len + cap)
  - Total: ~32 bytes (with padding)

Result<(), ErrorCode>:  // Common pattern for operations without meaningful return
  - Discriminant: 1 byte
  - (): 0 bytes
  - ErrorCode: depends on error type
```

#### Working with Result Values

**Primary mechanism:** Pattern matching for explicit error handling

```cantrip
// Handle both success and error cases
match operation_result {
    Result.Ok(value) => {
        // Use successful value
        continue_with(value)
    },
    Result.Err(error) => {
        // Handle error
        log_error(error)
        recover()
    },
}

// Propagate errors through explicit matching
procedure process_file(path: String): Result<Data, Error>
    uses io.read, alloc.heap
{
    match std.fs.read_to_string(path) {
        Result.Ok(contents) => {
            match parse_data(contents) {
                Result.Ok(data) => Result.Ok(data),
                Result.Err(e) => Result.Err(e),
            }
        },
        Result.Err(e) => Result.Err(e),
    }
}
```

**Note:** Standard library (Part XII) may provide utility procedures for result composition, but explicit pattern matching is the idiomatic Cantrip approach for error handling.

### 2.1.7.3 Design Rationale

**Why are Option and Result primitives?**

1. **Pervasive use**: Every Cantrip program uses these types extensively
2. **Compiler integration**: Niche optimization, pattern exhaustiveness checking
3. **Effect system**: Result integrates with Cantrip's effect tracking
4. **Error handling foundation**: Result is Cantrip's primary error mechanism
5. **Null safety**: Option eliminates entire class of null pointer bugs

**Why not exceptions?**

Cantrip uses Result<T, E> instead of exceptions because:

- **Explicit in types**: Function signatures show they can fail
- **Exhaustive handling**: Compiler enforces error checking
- **Composable**: Easy to chain fallible operations
- **Zero overhead**: No exception unwinding machinery
- **Permission-safe**: Works naturally with permission system

**Why not built-in `?` operator?**

Cantrip doesn't have Rust's `?` operator because:

- **Explicit over implicit**: Pattern matching makes control flow visible
- **Simpler language**: One less special case operator
- **Methods suffice**: `and_then` and `map` provide composition
- **Clarity**: Error propagation is explicit at each step

However, library or macro support for error propagation patterns is expected.

### 2.1.7.4 Comparison with Other Approaches

**Cantrip vs Rust:**

- **Same**: Option and Result as primary mechanisms
- **Different**: No `?` operator (yet), explicit pattern matching
- **Different**: Permission system with region-based escape analysis affects ownership transfer

**Cantrip vs Java/C++:**

- **Cantrip**: Type-safe Option/Result with exhaustiveness checking
- **Java/C++**: Exceptions with no compile-time checking, nullable types

**Cantrip vs Go:**

- **Cantrip**: Result<T, E> with type-safe errors
- **Go**: (T, error) tuple with convention-based checking

**Cantrip vs Haskell:**

- **Similar**: Maybe/Either monads map to Option/Result
- **Different**: Cantrip uses explicit match, Haskell uses `do` notation

---

## 2.1.9 Notes and Warnings

**Note 1 (Integer Overflow):** Integer overflow wraps in release mode (two's complement) but panics in debug mode. Use checked arithmetic methods (`checked_add`, `checked_mul`, etc.) for overflow detection in release builds.

**Note 2 (Default Integer Type):** Unsuffixed integer literals default to `i32`. Use type suffixes (`42u64`) or type annotations (`: u16`) for other integer types.

**Note 3 (Platform-Dependent Sizes):** Types `isize` and `usize` are pointer-sized (32-bit on 32-bit platforms, 64-bit on 64-bit platforms). Use these for array indexing and memory offsets.

**Note 4 (Floating-Point Precision):** `f32` provides ~7 decimal digits of precision; `f64` provides ~15 decimal digits. Use `f64` by default unless memory/performance constraints require `f32`.

**Note 5 (NaN Behavior):** Floating-point NaN values do not compare equal to themselves (`NaN != NaN`). Use `.is_nan()` method to check for NaN. NaN propagates through most arithmetic operations.

**Note 6 (Character vs String):** `char` represents a single Unicode scalar value (4 bytes), while `str` represents UTF-8 encoded text (variable bytes per character). A grapheme cluster (user-perceived character) may require multiple `char` values.

**Note 7 (String Heap Allocation):** `String` type requires heap allocation (effect `alloc.heap`). Use `str` slices when possible to avoid allocations. String literals have type `str` (borrowed string slice).

**Note 8 (UTF-8 Invariant):** All `String` and `str` values maintain UTF-8 validity as a type invariant. Invalid UTF-8 cannot be constructed through safe operations. Unsafe operations that violate this invariant cause undefined behavior.

**Note 9 (Never Type Divergence):** Functions returning `Never` (`!`) never return normally—they must loop forever, panic, or exit. The `Never` type can coerce to any other type, enabling uniform error handling in match arms.

**Warning 1 (Integer Division by Zero):** Division by zero panics at runtime for integer types. Use checked division (`checked_div`) or explicit zero checks for fallible division.

**Warning 2 (Floating-Point Equality):** Avoid direct equality comparisons with floating-point types due to rounding errors. Use epsilon-based comparisons or relative tolerance checks.

**Warning 3 (String Indexing):** Strings cannot be indexed by position (`s[i]` is invalid) because UTF-8 encoding has variable-width characters. Use `.chars()` iterator or byte slicing (unsafe for invalid boundaries).

**Warning 4 (Option/Result Unwrapping):** Methods like `Option.unwrap()` and `Result.unwrap()` panic if the value is `None` or `Err`. Use pattern matching or fallible unwrapping methods (`unwrap_or`, `unwrap_or_else`) for safer handling.

**Performance Note 1 (Copy Trait):** All primitive types except `String` implement the `Copy` trait and have trivial copy semantics. Copying is automatic when passing by value.

**Performance Note 2 (String Cloning):** Cloning `String` requires heap allocation and memory copying. Prefer passing `str` slices by reference when ownership transfer is not needed.

**Implementation Note 1 (Niche Optimization):** `Option<T>` uses niche optimization for types with unused bit patterns (e.g., `Option<&T>` has same size as `&T` using null for `None`). Similarly optimized for `Option<bool>`, non-zero integers, etc.

**Implementation Note 2 (Result Size):** `Result<T, E>` has size `max(sizeof(T), sizeof(E)) + discriminant`. Use small error types or boxed errors (`Result<T, Box<E>>`) to reduce size overhead.

---

## 2.2 Product Types

### 2.2.0 Overview

#### 2.2.0.1 Scope

This subsection summarises tuple, record, and tuple-struct constructs elaborated in §§2.2.1–2.2.3.

#### 2.2.0.2 Dependencies and Notation

Product types reuse metavariables from §2.0.4 and depend on Part III (Permissions) for move semantics, Part IV (Effects) for method side effects, and Part VI (Regions) for aggregate storage.

#### 2.2.0.3 Guarantees

Product types MUST preserve field invariants, maintain explicit permission tracking, and obey constructor and projection rules documented in §§2.2.1–2.2.2.

### 2.2.0.4 Syntax Summary

Concrete grammar for tuples and records is provided in §§2.2.1–2.2.2. No additional derived forms exist beyond those rules; tuple-struct syntax remains under ISSUE TS-1.

### 2.2.0.5 Static Semantics Summary

Typing rules for construction, projection, and destructuring are defined in §§2.2.1–2.2.2. This subsection introduces no new rules.

### 2.2.0.6 Dynamic Semantics Summary

Evaluation order is left-to-right and reuses layout descriptions from §§2.2.1–2.2.2.

### 2.2.0.7 Algorithms

Out-of-scope; bidirectional checking derives from the rules already published in §§2.2.1–2.2.2.

### 2.2.0.8 Proven Properties

No additional metatheorems beyond the global Progress/Preservation results (§2.0.7).

### 2.2.0.9 Integration Notes

Records with methods MUST annotate self permissions (`own`, `mut`, `imm`) as required by Part III. Pattern matching semantics defer to §13.

### 2.2.0.10 Notes and Issues

Open gaps: ISSUE TS-1 (tuple-struct constructors) and ISSUE TS-5 (example suites).

**Definition 2.2.1 (Tuple):** A tuple is an anonymous product type containing a fixed number of heterogeneous elements: `(T₁, ..., Tₙ) = T₁ × T₂ × ... × Tₙ`. Tuple types and literals use the same syntax `(T₁, ..., Tₙ)` and `(e₁, ..., eₙ)`, with the unit type `()` as the zero-element tuple.

## 2.2.1 Tuples

#### 2.2.1.1 Overview

**Key innovation/purpose:** Tuples provide lightweight, anonymous product types for temporary grouping of heterogeneous values without the ceremony of defining a record type. They are essential for multiple return values and destructuring patterns.

##### Guidance (non-normative)

- Use tuples for short-lived grouping or multiple return values; promote to a record when field names clarify intent.
- Remember the unit type `()` is the zero-element tuple and appears in procedure signatures per §2.2.1.2.
- Destructure via pattern matching; the projection rules in §2.2.1.3 define positional access.

#### 2.2.1.2 Syntax

##### 2.2.1.2.1 Concrete Syntax

**Concrete Syntax:**

```ebnf
TupleType    ::= "(" Type ("," Type)* ")"
               | "(" ")"                     // Unit type
TupleLiteral ::= "(" Expr ("," Expr)* ")"
               | "(" ")"                     // Unit value
TupleProj    ::= Expr "." IntLiteral
TuplePattern ::= "(" Pattern ("," Pattern)* ")"
```

**Examples:**

```cantrip
// Tuple types and literals
let pair: (i32, str) = (42, "answer")
let triple: (f64, f64, f64) = (1.0, 2.0, 3.0)
let nested: ((i32, i32), str) = ((10, 20), "point")

// Tuple projection (field access by index)
let first = pair.0          // 42
let second = pair.1         // "answer"

// Tuple destructuring
let (x, y) = pair
let ((a, b), label) = nested
```

**Unit type (0-tuple):**

```cantrip
let unit: () = ()  // Type and value both written as ()

function print_message(msg: str): () {
    std.io.println(msg)
    // Implicit return of ()
}
```

The unit type `()` is a special case of tuples with zero elements. It represents "no value" and is used for:

- Functions that perform side effects but return nothing meaningful
- Empty states in enums
- Placeholder in generic contexts

##### 2.2.1.2.2 Abstract Syntax

**Tuples:**

```
τ ::= (τ₁, ..., τₙ)  (tuple type)
    | ()             (unit type)

e ::= (e₁, ..., eₙ)  (tuple literal)
    | e.i            (tuple projection)
    | ()             (unit value)

p ::= (p₁, ..., pₙ)  (tuple pattern)
```

#### 2.2.1.3 Static Semantics

##### 2.2.1.3.1 Well-Formedness Rules

**Tuple type well-formedness:**

```
[WF-Tuple]
Γ ⊢ T₁ : Type    ...    Γ ⊢ Tₙ : Type
────────────────────────────────────────
Γ ⊢ (T₁, ..., Tₙ) : Type

[WF-Unit]
────────────────
Γ ⊢ () : Type
```

**Explanation:** A tuple type is well-formed if all its component types are well-formed. The unit type `()` is always well-formed.

##### 2.2.1.3.2 Type Rules

**Tuple literal typing:**

```
[T-Tuple]
Γ ⊢ e₁ : T₁    ...    Γ ⊢ eₙ : Tₙ
───────────────────────────────────
Γ ⊢ (e₁, ..., eₙ) : (T₁, ..., Tₙ)
```

**Explanation:** A tuple literal has tuple type when each element expression has the corresponding component type.

**Example:**

```cantrip
let x: i32 = 42
let y: str = "hello"
let pair = (x, y)  // Type: (i32, str)
```

**Tuple projection typing:**

```
[T-Tuple-Proj]
Γ ⊢ e : (T₁, ..., Tₙ)
i ∈ [0, n)
───────────────────────────────────────
Γ ⊢ e.i : Tᵢ
```

**Explanation:** Accessing the i-th field of a tuple (0-indexed) yields the type of that field.

**Example:**

```cantrip
let pair: (i32, str) = (42, "answer")
let first = pair.0   // Type: i32
let second = pair.1  // Type: str
```

**Unit type:**

```
[T-Unit]
────────────────
Γ ⊢ () : ()
```

**Explanation:** The unit value `()` has unit type `()`.

**Tuple pattern typing:**

```
[T-Tuple-Pattern]
Γ ⊢ p₁ : T₁ ⇝ Γ₁    ...    Γ ⊢ pₙ : Tₙ ⇝ Γₙ
────────────────────────────────────────────────
Γ ⊢ (p₁, ..., pₙ) : (T₁, ..., Tₙ) ⇝ Γ₁ ∪ ... ∪ Γₙ
```

**Explanation:** A tuple pattern matches a tuple type when each sub-pattern matches the corresponding component type. The resulting context is the union of all sub-pattern contexts.

**Example:**

```cantrip
let (x, y) = (42, "hello")
// Pattern (x, y) : (i32, str) ⇝ {x: i32, y: str}
```

##### 2.2.1.3.3 Type Properties

**Property 7.1 (Tuple Type Uniqueness):**

For any expression `(e₁, ..., eₙ)`, if it is well-typed, it has exactly one type:

```
∀e₁...eₙ. (∃T. Γ ⊢ (e₁, ..., eₙ) : T) ⇒ (∃!T₁...Tₙ. T = (T₁, ..., Tₙ))
```

**Property 7.2 (Tuple Type Decomposition):**

A tuple type determines the types of its components:

```
Γ ⊢ e : (T₁, ..., Tₙ) ⇒ ∀i ∈ [0, n). Γ ⊢ e.i : Tᵢ
```

**Type Inference:**

Tuple types are inferred component-wise without requiring unification. Each element can have a different type, determined independently:

```cantrip
let tuple = (42, "hello", 3.14)  // Inferred: (i32, str, f64)
let nested = ((1, 2), (3, 4))    // Inferred: ((i32, i32), (i32, i32))
```

**Contextual Type Inference:**

When a tuple appears in a context expecting a specific tuple type, that type information flows to the elements:

```cantrip
let pair: (u32, f32) = (10, 3.5) // 10 inferred as u32, 3.5 as f32

function process(data: (i64, bool)) { ... }
process((100, true))  // 100 inferred as i64
```

#### 2.2.1.4 Dynamic Semantics

##### 2.2.1.4.1 Evaluation Rules

**Tuple evaluation:**

```
[E-Tuple]
⟨e₁, σ⟩ ⇓ ⟨v₁, σ₁⟩    ...    ⟨eₙ, σₙ₋₁⟩ ⇓ ⟨vₙ, σₙ⟩
─────────────────────────────────────────────────────
⟨(e₁, ..., eₙ), σ⟩ ⇓ ⟨(v₁, ..., vₙ), σₙ⟩

[E-Unit]
────────────────
⟨(), σ⟩ ⇓ ⟨(), σ⟩
```

**Tuple projection evaluation:**

```
[E-Tuple-Proj]
⟨e, σ⟩ ⇓ ⟨(v₀, ..., vₙ₋₁), σ'⟩
i < n
─────────────────────────────────────
⟨e.i, σ⟩ ⇓ ⟨vᵢ, σ'⟩
```

**Formal semantics:**

```
⟦(v₁, ..., vₙ)⟧ = (⟦v₁⟧, ..., ⟦vₙ⟧)  (tuple value)
⟦(v₁, ..., vₙ).i⟧ = ⟦vᵢ⟧              (projection)
```

##### 2.2.1.4.2 Memory Representation

**Tuple layout:**

```
(T₁, T₂, T₃) memory layout:
┌────┬───┬────┬───┬────┐
│ T₁ │pad│ T₂ │pad│ T₃ │
└────┴───┴────┴───┴────┘
```

**Properties:**

- Each field aligned to its type's alignment
- Padding inserted between fields to maintain alignment
- Total size rounded up to tuple's alignment
- Fields ordered as declared (no reordering optimization)

**Alignment calculation:**

```
align((T₁, ..., Tₙ)) = max(align(T₁), ..., align(Tₙ))
size((T₁, ..., Tₙ)) = aligned_sum(size(T₁), ..., size(Tₙ))
```

**Examples:**

```cantrip
// (u8, u32, u16) on typical 64-bit system
// align(u8) = 1, align(u32) = 4, align(u16) = 2
// Layout:
// Offset:  0  1  2  3  4  5  6  7
//         ┌──┬──┬──┬──┬────┬────┬──┬──┐
//         │u8│p │p │p │u32 │u16 │p │p │
//         └──┴──┴──┴──┴────┴────┴──┴──┘
// Total: 8 bytes (rounded to alignment of 4)

// (f64, u8) on 64-bit system
// Offset:  0              8  9
//         ┌──────────────┬──┬──┬──┬──┬──┬──┬──┐
//         │     f64      │u8│p │p │p │p │p │p │p │
//         └──────────────┴──┴──┴──┴──┴──┴──┴──┘
// Total: 16 bytes (f64 must 8-byte alignment)
```

**Unit type:**

```
() memory layout:
(no memory allocated, zero-sized type)
```

The unit type has size 0 and alignment 1.

##### 2.2.1.4.3 Operational Behavior

**Construction and access:**

Tuple operations are zero-cost abstractions with predictable evaluation:

- **Construction**: Tuple elements are evaluated left-to-right, with each element fully evaluated before the next begins
- **Field access**: Access via `.i` is a compile-time offset calculation with zero runtime overhead
- **Pattern matching**: Tuple destructuring is optimized away by the compiler when possible

**Copy Capability:**

```
Tuples implement Copy ⟺ all fields implement Copy
```

**Parameter Passing:**

- All tuples pass by permission by default (like all types)
- `.copy()` available for Copy-capable tuples
- Non-Copy tuples require explicit `move` for ownership transfer
- **Partial moves**: Not allowed—tuples move as a complete unit

**Example:**

```cantrip
// Copy-capable tuple
let point: (i32, i32) = (10, 20)
procedure distance(p: (i32, i32)): f64 { ... }
distance(point)  // Permission, point still usable

let copy = point.copy()  // Explicit copy if needed

// Non-Copy tuple
let record: (String, i32) = (String.new("test"), 42)
process(record)  // Permission
consume(move record)  // Explicit move
// record no longer usable
```

**Pattern matching optimization:**

The compiler optimizes tuple construction and destructuring:

```cantrip
// Intermediate tuple construction may be eliminated
let (x, y) = compute_pair()  // Compiler may use two separate variables

// Swap without temporary allocation
let (a, b) = (b, a)  // Optimized to register swaps
```

**Performance characteristics:**

```
Tuple construction:  O(sum of field construction costs)
Field access:        O(1), compile-time offset
Pattern matching:    O(1), zero-cost destructuring
Memory overhead:     Padding for alignment (no discriminant tag)
```

#### 2.2.1.5 Additional Properties

##### 2.2.1.5.1 Parameter Passing and Copy Capability

**Tuples:**

- `(T₁, ..., Tₙ)` implements Copy if all Tᵢ implement Copy
- All tuples pass by permission regardless of Copy capability
- Explicit `.copy()` for Copy-capable tuples
- Explicit `move` for ownership transfer

```cantrip
let pair1: (i32, f64) = (42, 3.14)
procedure process(p: (i32, f64)) { ... }
process(pair1)  // Permission, pair1 still usable

let pair2 = pair1.copy()  // Explicit copy

let owned: (String, Vec<i32>) = (String.new(), Vec.new())
process_owned(owned)  // Permission, owned still usable
consume(move owned)  // Explicit move
// owned is now unusable
```

##### 2.2.1.5.2 Subtyping

Tuples are **invariant** in their type parameters:

```
(T₁, T₂) is NOT a subtype of (U₁, U₂) even if T₁ <: U₁, T₂ <: U₂
```

**Rationale:** Allowing covariance would violate type safety in mutable contexts.

---

**Definition 2.2.2 (Record):** A record is a labeled Cartesian product of its fields. Formally, if `R { f₁: T₁; …; fₙ: Tₙ }` then `⟦R⟧ ≅ T₁ × … × Tₙ` with named projections `fᵢ : R → Tᵢ`. Records provide procedures within their definition; inheritance is not part of the model—use traits and composition.

## 2.2.2 Records and Classes

#### 2.2.2.1 Overview

**Key innovation/purpose:** Records provide named product types with procedures, enabling data encapsulation and object-oriented programming without inheritance complexity.

##### Terminology Snapshot

| Construct    | Syntax example                  | Identity model         | Field access             | Typical use case                               |
| ------------ | ------------------------------- | ---------------------- | ------------------------ | ---------------------------------------------- |
| Tuple        | `(T₁, T₂)`                      | Structural (anonymous) | Positional (`.0`, `.1`)  | Lightweight, short-lived aggregation           |
| Tuple struct | `record Wrapper(T₁, T₂)`        | Nominal                | Positional (`.0`, `.1`)  | Newtype wrappers with distinct static identity |
| Named record | `record Point { x: T₁; y: T₂ }` | Nominal                | Field names (`.x`, `.y`) | Rich data shapes with semantic field labelling |

The remainder of §2.2.2 elaborates on tuple structs and named records, while §2.2.1 covers plain tuples.

##### Guidance (non-normative)

- Model structured data with records when field names improve clarity or methods are required; keep lightweight tuples for ad-hoc groupings.
- Choose enums for mutually exclusive variants and arrays/slices for homogeneous collections, as governed by §§2.3 and 2.4.
- Apply permission annotations per §2.2.2.3—records inherit the permission of the enclosing value unless fields introduce modal types.

#### 2.2.2.2 Syntax

##### 2.2.2.2.1 Concrete Syntax

**Record declaration:**

```ebnf
RecordDecl   ::= "record" Ident GenericParams? "{" FieldList "}"
FieldList    ::= Field (";" Field)* ";"?
Field        ::= Visibility? Ident ":" Type
Visibility   ::= "public" | "private"
GenericParams ::= "<" TypeParam ("," TypeParam)* ">"
```

**Record construction:**

```ebnf
RecordExpr   ::= Ident "{" FieldInit ("," FieldInit)* ","? "}"
FieldInit    ::= Ident ":" Expr
               | Ident              // Shorthand: field: field
```

**Field access:**

```ebnf
FieldAccess  ::= Expr "." Ident
```

**Examples:**

```cantrip
record Point {
    x: f64
    y: f64
}

public record User {
    public name: String
    public email: String
    private password_hash: String
}

let origin = Point { x: 0.0, y: 0.0 }
let x_coord = origin.x
```

##### 2.2.2.2.2 Abstract Syntax

**Record types:**

```
τ ::= record Name { f₁: τ₁; ...; fₙ: τₙ }    (record type)
    | Name                                     (record type reference)
```

**Record expressions:**

```
e ::= Name { f₁: e₁, ..., fₙ: eₙ }            (record literal)
    | e.f                                      (field access)
```

**Record patterns:**

```
p ::= Name { f₁: p₁, ..., fₙ: pₙ }            (record pattern)
    | Name { f₁, ..., fₙ }                     (shorthand pattern)
```

#### 2.2.2.3 Static Semantics

##### 2.2.2.3.1 Well-Formedness Rules

**Record declaration:**

```
[WF-Record]
Γ ⊢ T₁ : Type    ...    Γ ⊢ Tₙ : Type
fields f₁, ..., fₙ are distinct
──────────────────────────────────────────────
Γ ⊢ record Name { f₁: T₁; ...; fₙ: Tₙ } : Type
```

**Generic record:**

```
[WF-Generic-Record]
Γ, α₁ : Type, ..., αₘ : Type ⊢ T₁ : Type    ...    Γ ⊢ Tₙ : Type
fields f₁, ..., fₙ are distinct
────────────────────────────────────────────────────────────────────
Γ ⊢ record Name<α₁, ..., αₘ> { f₁: T₁; ...; fₙ: Tₙ } : Type
```

##### 2.2.2.3.2 Type Rules

**Record construction:**

```
[T-Record]
record Name { f₁: T₁; ...; fₙ: Tₙ } in Γ
Γ ⊢ e₁ : T₁    ...    Γ ⊢ eₙ : Tₙ
───────────────────────────────────────────
Γ ⊢ Name { f₁: e₁, ..., fₙ: eₙ } : Name
```

**Field access:**

```
[T-Field]
Γ ⊢ e : record S    field f : T in S
────────────────────────────────────
Γ ⊢ e.f : T
```

**Record pattern:**

```
[T-Record-Pattern]
record Name { f₁: T₁; ...; fₙ: Tₙ }
Γ ⊢ p₁ : T₁ ⇝ Γ₁    ...    Γ ⊢ pₙ : Tₙ ⇝ Γₙ
────────────────────────────────────────────────
Γ ⊢ Name { f₁: p₁, ..., fₙ: pₙ } : Name ⇝ Γ₁ ∪ ... ∪ Γₙ
```

##### 2.2.2.3.3 Type Properties

**Theorem 8.1 (Structural Typing):**

Records with identical field names and types are distinct types:

```
record A { x: i32; y: i32 } ≠ record B { x: i32; y: i32 }
```

Cantrip uses nominal typing for records, not structural typing.

**Theorem 8.2 (Copy Capability):**

A record implements `Copy` if and only if all its fields implement `Copy`:

```
record R { f₁: T₁; ...; fₙ: Tₙ } : Copy
⟺
T₁ : Copy ∧ ... ∧ Tₙ : Copy
```

**Parameter Passing:** Records pass by permission regardless of Copy capability.

**Theorem 8.3 (Alignment and Size):**

For record R with fields f₁: T₁, ..., fₙ: Tₙ:

```
align(R) = max(align(T₁), ..., align(Tₙ))
size(R) = aligned_sum(size(T₁), ..., size(Tₙ), align(R))
```

where aligned_sum accounts for padding between fields.

#### 2.2.2.4 Dynamic Semantics

##### 2.2.2.4.1 Evaluation Rules

**Record construction:**

```
[E-Record]
⟨e₁, σ⟩ ⇓ ⟨v₁, σ₁⟩    ...    ⟨eₙ, σₙ₋₁⟩ ⇓ ⟨vₙ, σₙ⟩
─────────────────────────────────────────────────────
⟨Name { f₁: e₁, ..., fₙ: eₙ }, σ⟩ ⇓ ⟨Name { f₁: v₁, ..., fₙ: vₙ }, σₙ⟩
```

**Field access:**

```
[E-Field]
⟨e, σ⟩ ⇓ ⟨Name { f₁: v₁, ..., fᵢ: vᵢ, ..., fₙ: vₙ }, σ'⟩
──────────────────────────────────────────────────────────
⟨e.fᵢ, σ⟩ ⇓ ⟨vᵢ, σ'⟩
```

##### 2.2.2.4.2 Memory Representation

**Default layout:**

Records are laid out in memory with fields in declaration order, with padding inserted for alignment:

```
record Example {
    a: u8      // offset 0
    b: u32     // offset 4 (3 bytes padding)
    c: u16     // offset 8
}

Memory layout:
┌──┬──┬──┬──┬────┬────┬──┐
│a │p │p │p │ b  │ c  │p │
└──┴──┴──┴──┴────┴────┴──┘
Total: 12 bytes (rounded to alignment of 4)
```

**Layout calculation:**

```
layout(record R) = {
    size = ∑ᵢ size(fᵢ) + padding
    align = max(align(f₁), ..., align(fₙ))
    offset(fᵢ) = aligned(∑ⱼ₍ⱼ₍ᵢ₎ size(fⱼ), align(fᵢ))
}
```

##### 2.2.2.4.3 Operational Behavior

**Copy Capability vs Parameter Passing:**

```cantrip
record Point {
    x: i32  // Copy-capable
    y: i32  // Copy-capable
}
// Point is Copy-capable (all fields are Copy)

let p1 = Point { x: 1, y: 2 }
procedure process(pt: Point) { ... }
process(p1)  // Permission, p1 still usable

let p2 = p1.copy()  // Explicit copy
assert(p1.x == p2.x)

record Entity {
    name: String  // Non-Copy
    id: i32       // Copy-capable
}
// Entity is NOT Copy (contains String)

let e1 = Entity { name: String.new("test"), id: 1 }
process_entity(e1)  // Permission, e1 still usable
consume_entity(move e1)  // Explicit move
// e1 no longer usable
```

#### 2.2.2.5 Procedures

**Definition 2.2.3 (Procedure):** A procedure is a method with an explicit `self` parameter defined within a record.

**Syntax:**

```cantrip
record TypeName {
    // fields

    procedure method_name(self: Permission TypeName, params): T
        must ...
        will ...
    {
        ...
    }
}
```

**Type rule:**

```
[T-Procedure]
procedure m : Self => T => U in record S
Γ ⊢ self : S    Γ ⊢ arg : T
───────────────────────────────────────
Γ ⊢ self.m(arg) : U
```

**Procedure call desugaring:**

```
obj.procedure(args) ≡ Type::procedure(obj, args)
```

**Example:**

```cantrip
record Point {
    x: f64
    y: f64

    procedure new(x: f64, y: f64): own Point {
        own Point { x, y }
    }

    procedure distance(self: Point, other: Point): f64 {
        let dx = self.x - other.x
        let dy = self.y - other.y
        (dx * dx + dy * dy).sqrt()
    }

    procedure move_by(self: mut Point, dx: f64, dy: f64) {
        self.x += dx
        self.y += dy
    }
}
```

#### 2.2.2.6 Advanced Features

##### 2.2.2.6.1 Memory Layout Control

**C-compatible layout:**

```cantrip
[[repr(C)]]
record Vector3 {
    x: f32  // offset 0
    y: f32  // offset 4
    z: f32  // offset 8
}
// Size: 12 bytes, Align: 4
```

**Formal semantics:**

```
[[repr(C)]] ⟹ {
    fields laid out in declaration order
    no reordering optimization
    matches C struct layout
    ABI-compatible with extern "C"
}
```

**Packed layout (no padding):**

```cantrip
[[repr(packed)]]
record NetworkHeader {
    magic: u32     // offset 0
    version: u16   // offset 4
    length: u16    // offset 6
}
// Size: 8 bytes, Align: 1
```

**Warning:** Packed layouts can cause unaligned access, which may be undefined behavior on some architectures.

**Explicit alignment:**

```cantrip
[[repr(align(64))]]
record CacheLine {
    data: [u8; 64]
}
// Aligned to 64-byte boundary for cache optimization
```

##### 2.2.2.6.2 Structure-of-Arrays

**Attribute:**

```cantrip
[[soa]]
record Particle {
    position: Vec3
    velocity: Vec3
    lifetime: f32
}
```

**Compiler transformation:**

```cantrip
// Generates:
record ParticleSOA {
    positions: Vec<Vec3>
    velocities: Vec<Vec3>
    lifetimes: Vec<f32>
    length: usize

    procedure get(self: ParticleSOA, index: usize): Particle
        must index < self.length
    {
        Particle {
            position: self.positions[index],
            velocity: self.velocities[index],
            lifetime: self.lifetimes[index],
        }
    }
}
```

**Benefits:**

- Better cache locality for iterating over specific fields
- SIMD optimization opportunities
- Useful for game engines and data-intensive applications

#### 2.2.2.8 Tuple Structs

**Definition 2.2.3.1 (Tuple Struct):** A tuple struct is a nominal record type with unnamed, positionally-accessed fields. Formally, `record Name(T₁, ..., Tₙ)` creates a distinct type with fields accessed as `.0`, `.1`, etc. Unlike type aliases, tuple structs are not transparent and provide nominal type safety.

##### 2.2.2.8.1 Syntax

**Grammar:**

```ebnf
TupleStruct     ::= "record" Ident GenericParams? "(" TypeList ")" ";"
TypeList        ::= Type ("," Type)* ","?
TupleStructExpr ::= Ident "(" ExprList ")"
TupleFieldAccess ::= Expr "." IntLiteral
```

**Examples:**

```cantrip
// Basic tuple structs
record Point2D(f64, f64)
record Color(u8, u8, u8)
record Newtype(i32)

// Generic tuple structs
record Wrapper<T>(T)
record Pair<A, B>(A, B)

// Construction
let p = Point2D(1.0, 2.0)
let c = Color(255, 128, 0)

// Field access by position
let x = p.0  // First field
let y = p.1  // Second field
```

##### 2.2.2.8.2 Abstract Syntax

**Tuple struct types:**

```
τ ::= record Name(τ₁, ..., τₙ)    (tuple struct type)
    | Name                         (tuple struct type reference)
```

**Tuple struct expressions:**

```
e ::= Name(e₁, ..., eₙ)            (tuple struct construction)
    | e.n                          (positional field access, n ∈ ℕ)
```

**Tuple struct patterns:**

```
p ::= Name(p₁, ..., pₙ)            (tuple struct pattern)
```

##### 2.2.2.8.3 Well-Formedness Rules

**[WF-TupleStruct] Tuple Struct Declaration:**

```
[WF-TupleStruct]
Γ ⊢ T₁ : Type    ...    Γ ⊢ Tₙ : Type
n ≥ 1    (at least one field)
────────────────────────────────────────────
Γ ⊢ record Name(T₁, ..., Tₙ) well-formed
```

**Explanation:** Tuple structs must have at least one field. Zero-field tuple structs are not allowed (use unit type `()` instead).

```cantrip
// ✓ Valid: at least one field
record UserId(u64)
record Point(f64, f64)

// ✗ Invalid: zero fields
// record Empty()  // Use () instead
```

**[WF-TupleStruct-Generic] Generic Tuple Struct:**

```
[WF-TupleStruct-Generic]
Γ, α₁ : Type, ..., αₖ : Type ⊢ T₁ : Type    ...    Γ, α₁ : Type, ..., αₖ : Type ⊢ Tₙ : Type
────────────────────────────────────────────
Γ ⊢ record Name<α₁, ..., αₖ>(T₁, ..., Tₙ) well-formed
```

**Explanation:** Generic tuple structs can be parameterized by type variables.

```cantrip
// Generic wrapper
record Wrapper<T>(T)

// Multiple type parameters
record Result<T, E>(bool, T, E)  // (success, value, error)

let w: Wrapper<i32> = Wrapper(42)
let r: Result<String, Error> = Result(true, "ok", error_val)
```

##### 2.2.2.8.4 Type Rules

**[T-TupleStruct-Cons] Tuple Struct Construction:**

```
[T-TupleStruct-Cons]
record Name(T₁, ..., Tₙ) declared
Γ ⊢ e₁ : T₁    ...    Γ ⊢ eₙ : Tₙ
────────────────────────────────────────────
Γ ⊢ Name(e₁, ..., eₙ) : Name
```

**Explanation:** Tuple struct construction must arguments matching field types in order.

```cantrip
record Point3D(f64, f64, f64)

let p = Point3D(1.0, 2.0, 3.0)  // ✓ Correct types
// let bad = Point3D(1, 2, 3)   // ✗ ERROR: expected f64, found i32
```

**[T-TupleStruct-Field] Positional Field Access:**

```
[T-TupleStruct-Field]
record Name(T₁, ..., Tₙ) declared
Γ ⊢ e : Name
0 ≤ i < n
────────────────────────────────────────────
Γ ⊢ e.i : Tᵢ₊₁    (0-indexed)
```

**Explanation:** Field access uses numeric literals: `.0` for first field, `.1` for second, etc.

```cantrip
record Color(u8, u8, u8)

let c = Color(255, 128, 0)
let red = c.0    // ✓ u8
let green = c.1  // ✓ u8
let blue = c.2   // ✓ u8
// let bad = c.3 // ✗ ERROR: no field 3
```

**[T-TupleStruct-Nominal] Nominal Typing:**

```
[T-TupleStruct-Nominal]
record A(T₁, ..., Tₙ) declared
record B(T₁, ..., Tₙ) declared
A ≠ B
────────────────────────────────────────────
A ≢ B    (not equivalent)
```

**Explanation:** Tuple structs with identical field types are distinct types (nominal typing).

```cantrip
record Meters(f64)
record Feet(f64)

let m = Meters(10.0)
let f = Feet(33.0)

// ✗ ERROR: cannot assign Feet to Meters (distinct types)
// let distance: Meters = f

// Must convert explicitly
function feet_to_meters(f: Feet) => Meters {
    Meters(f.0 * 0.3048)
}
```

**[T-TupleStruct-Pattern] Pattern Matching:**

```
[T-TupleStruct-Pattern]
record Name(T₁, ..., Tₙ) declared
Γ ⊢ p₁ : T₁    ...    Γ ⊢ pₙ : Tₙ
────────────────────────────────────────────
Γ ⊢ Name(p₁, ..., pₙ) : pattern for Name
```

**Explanation:** Tuple structs can be destructured in patterns.

```cantrip
record Point(i32, i32)

let p = Point(5, 10)

match p {
    Point(x, y) => {
        println("x: {}, y: {}", x, y)
    }
}

// Or using let binding
let Point(x, y) = p
assert(x == 5 && y == 10)
```

**[T-TupleStruct-Method] Methods on Tuple Structs:**

```
[T-TupleStruct-Method]
record Name(T₁, ..., Tₙ) {
    procedure f(self: Self, ...) : U { e }
}
────────────────────────────────────────────
Methods can access self.0, self.1, ..., self.n-1
```

**Explanation:** Tuple structs support methods within their definition.

```cantrip
record Vector2(f64, f64) {
    procedure length(self: Self): f64 {
        (self.0 * self.0 + self.1 * self.1).sqrt()
    }

    procedure dot(self: Self, other: Vector2): f64 {
        self.0 * other.0 + self.1 * other.1
    }
}

let v = Vector2(3.0, 4.0)
assert(v.length() == 5.0)
```

**[T-TupleStruct-Trait] Trait Implementation:**

```
[T-TupleStruct-Trait]
record Name(T₁, ..., Tₙ): Trait {
    // procedures implementing Trait
}
────────────────────────────────────────────
Name : Trait
```

**Explanation:** Tuple structs can implement traits.

```cantrip
record UserId(u64): Display {
    procedure fmt(self: Self): String {
        "User#" + self.0.to_string()
    }
}

let id = UserId(42)
println("{}", id)  // Output: User#42
```

**[T-TupleStruct-Generic-Inst] Generic Instantiation:**

```
[T-TupleStruct-Generic-Inst]
record Name<α₁, ..., αₖ>(T₁, ..., Tₙ) declared
Γ ⊢ U₁ : Type    ...    Γ ⊢ Uₖ : Type
────────────────────────────────────────────
Γ ⊢ Name<U₁, ..., Uₖ> : Type
```

**Explanation:** Generic tuple structs are instantiated by providing concrete types.

```cantrip
record Wrapper<T>(T)

let i: Wrapper<i32> = Wrapper(42)
let s: Wrapper<String> = Wrapper("hello")

// Generic with multiple parameters
record Pair<A, B>(A, B)
let p: Pair<i32, String> = Pair(1, "one")
```

**[T-TupleStruct-Mutability] Mutable Field Access:**

```
[T-TupleStruct-Mutability]
record Name(T₁, ..., Tₙ) declared
Γ ⊢ e : mut Name
0 ≤ i < n
────────────────────────────────────────────
Γ ⊢ e.i : mut Tᵢ₊₁    (mutable access)
```

**Explanation:** Mutable tuple struct instances allow mutable field access.

```cantrip
record Counter(i32)

var c = Counter(0)
c.0 += 1  // ✓ Mutable field access
assert(c.0 == 1)
```

##### 2.2.2.8.5 Memory Representation

Tuple structs have the same memory layout as tuples with the same field types:

```
record Name(T₁, T₂, ..., Tₙ)

Size: size(T₁) + size(T₂) + ... + size(Tₙ) + padding
Alignment: max(align(T₁), align(T₂), ..., align(Tₙ))
```

**Memory layout:**

```
Tuple Struct Name(T₁, T₂, T₃):
┌───────────┬───────────┬───────────┐
│  field 0  │  field 1  │  field 2  │
│  (T₁)     │  (T₂)     │  (T₃)     │
└───────────┴───────────┴───────────┘
```

**Example:**

```cantrip
record Point(f64, f64)
// Size: 16 bytes (2 × 8 bytes)
// Alignment: 8 bytes

record Color(u8, u8, u8)
// Size: 3 bytes (3 × 1 byte, no padding needed if standalone)
// Alignment: 1 byte
```

**Layout control:**
Tuple structs support the same layout attributes as named records:

```cantrip
[[repr(C)]]
record CPoint(f64, f64)  // C-compatible layout

[[repr(packed)]]
record Packed(u8, u16, u8)  // No padding

[[repr(align(64))]]
record CacheAligned(i32)  // 64-byte alignment
```

##### 2.2.2.8.6 Comparison: Tuple Structs vs. Tuples vs. Named Records

| Feature              | Tuple `(T₁, T₂)`  | Tuple Struct `record Name(T₁, T₂)` | Named Record `record Name { a: T₁; b: T₂ }` |
| -------------------- | ----------------- | ---------------------------------- | ------------------------------------------- |
| **Type identity**    | Structural        | Nominal                            | Nominal                                     |
| **Field access**     | `.0`, `.1`        | `.0`, `.1`                         | `.a`, `.b`                                  |
| **Type safety**      | Weak (structural) | Strong (nominal)                   | Strong (nominal)                            |
| **Semantic meaning** | Generic grouping  | Domain-specific type               | Domain-specific type                        |
| **Trait impls**      | Cannot add custom | Can add custom                     | Can add custom                              |
| **Methods**          | No                | Yes (inline)                       | Yes (inline)                                |
| **Readability**      | Low (positional)  | Medium (positional)                | High (named fields)                         |
| **Use case**         | Temporary data    | Newtype pattern, wrappers          | Complex data structures                     |

##### Guidance (non-normative)

- Prefer tuples for ad-hoc grouping, tuple structs for newtype safety, and records when field names or visibility control matter.

#### 2.2.2.9 Proven Properties

- **Theorem 2.2.4 (Tuple Struct Nominality).** Tuple structs with identical field types remain distinct nominal types (`record A(T)` ≢ `record B(T)` when `A ≠ B`).
- **Corollary 2.2.4.1.** Nominal distinction prevents accidental mixing of tuple structs that share layouts.
- **Theorem 2.2.5 (Memory Layout Equivalence).** Tuple structs share layout and alignment characteristics with their underlying tuples `(T₁, …, Tₙ)`.
- **Corollary 2.2.5.1.** Tuple structs introduce no runtime overhead compared to tuples.
- **Theorem 2.2.6 (Pattern Exhaustiveness).** Pattern matching on tuple structs is exhaustive precisely when every field pattern is exhaustive for its respective component type.

---

### 2.2.4 Notes and Warnings

**Note 2.2.1:** Tuples use structural typing (two tuples with the same component types are interchangeable), while records and tuple structs use nominal typing (distinct type identities).

**Note 2.2.2:** The unit type `()` is both a type and a value. It represents "no data" and is used for procedures that perform effects without returning meaningful values.

**Note 2.2.3:** All product types (tuples, records, tuple structs) implement Copy if and only if all their fields implement Copy. Regardless of Copy capability, all types pass by permission.

**Note 2.2.4:** Tuple structs provide a lightweight alternative to full records when you need nominal typing without named fields. They are ideal for the newtype pattern.

**Performance Note 2.2.1:** Product types have zero runtime overhead for field access. All projections (`.0`, `.field_name`) compile to direct offset calculations.

**Performance Note 2.2.2:** The compiler may optimize tuple construction and destructuring, especially in cases like `let (a, b) = (b, a)` which can become register swaps.

**Performance Note 2.2.3:** Records with #[repr(C)] guarantee C-compatible layout at the cost of disabling field reordering optimizations.

**Warning 2.2.1:** Tuples with more than 3-4 fields become hard to understand. Use records instead for clarity and maintainability.

**Warning 2.2.2:** Packed records (#[repr(packed)]) can cause unaligned memory access, which is undefined behavior on some architectures. Use with caution.

**Warning 2.2.3:** Partial moves are not allowed for product types. A tuple or record moves as a complete unit, not field-by-field.

**Warning 2.2.4:** Field order in tuples and records matters for memory layout. Reordering fields can change padding and total size.

**Implementation Note 2.2.1:** Pattern matching on product types is exhaustive checking. The compiler ensures all fields are accounted for in patterns.

**Implementation Note 2.2.2:** Structure-of-Arrays (#[soa]) is a compiler transformation that reorganizes data layout for cache efficiency in data-intensive applications.

---

## 2.3 Sum Types

### 2.3.0 Overview

#### 2.3.0.1 Scope

Section §2.3 summarises enums, unions, and modal types described in §§2.3.1–2.3.3.

#### 2.3.0.2 Dependencies and Notation

Modal semantics require Part III (Permissions) for linear transfers and Part IV (Effects) for transition capabilities. Pattern matching references §13; unsafe union access references §24.

#### 2.3.0.3 Guarantees

Enums MUST provide exhaustive pattern checking and maintain discriminant fidelity. Unions MUST only be accessed under `unsafe.union` effects. Modals MUST enforce state transitions declared in §12.

### 2.3.0.4 Syntax Summary

Concrete syntax for enums and unions is defined in §§2.3.1–2.3.2. Modal syntax is summarised in §2.3.3. No additional derived forms are introduced.

### 2.3.0.5 Static Semantics Summary

Typing rules for variant construction, pattern matching, and modal transitions reside in §§2.3.1–2.3.3.

### 2.3.0.6 Dynamic Semantics Summary

Discriminant evaluation, union field writes, and modal transition effects follow the rules in §§2.3.1–2.3.3.

### 2.3.0.7 Algorithms

Out-of-scope. Exhaustiveness checking and modal verification algorithms remain future work (ISSUE TS-6).

### 2.3.0.8 Proven Properties

Modals integrate with the global safety theorems (§2.0.7), ensuring state-sound transitions.

### 2.3.0.9 Integration Notes

Union accesses MUST declare `unsafe.union`. Modal transitions consume and produce permissions as specified in Part III.

### 2.3.0.10 Notes and Issues

Example suites remain pending (ISSUE TS-5). Algorithmic treatment for modals is tracked via ISSUE TS-6.

**Definition 2.3.1 (Enum):** An enum is a sum type (tagged union) with named variants. Formally, `enum E { V₁(T₁), ..., Vₙ(Tₙ) }` denotes the disjoint union `⟦E⟧ = T₁ + T₂ + ... + Tₙ`, where each variant Vᵢ is tagged with a discriminant to distinguish values at runtime.

## 2.3.1 Enums and Pattern Matching

### 2.3.1.1 Overview

**Key innovation/purpose:** Enums provide type-safe discriminated unions, enabling exhaustive pattern matching and making illegal states unrepresentable.

##### Guidance (non-normative)

- Model mutually exclusive states with enums and rely on the exhaustive matching rules in §2.3.1.3.
- Choose records when all variants share identical structure, or traits for open polymorphic hierarchies.
- Wrap optional or error cases in the built-in `Option` and `Result` types described in §2.1.7.

### 2.3.1.2 Syntax

#### 2.3.1.2.1 Concrete Syntax

**Enum declaration:**

```ebnf
EnumDecl     ::= "enum" Ident GenericParams? "{" VariantList "}"
VariantList  ::= Variant ("," Variant)* ","?
Variant      ::= Ident Discriminant?                // Unit variant
               | Ident "(" TupleFields ")"          // Tuple variant
               | Ident "{" RecordFields "}"         // Record variant
Discriminant ::= "=" IntLiteral                     // Explicit discriminant value
TupleFields  ::= Type ("," Type)*
RecordFields ::= Field ("," Field)*
```

**Pattern matching:**

```ebnf
Match        ::= "match" Expr "{" MatchArm ("," MatchArm)* ","? "}"
MatchArm     ::= Pattern ("if" Expr)? "->" Expr
Pattern      ::= Ident "." Ident                     // Unit pattern
               | Ident "." Ident "(" PatternList ")" // Tuple pattern
               | Ident "." Ident "{" FieldPat "}"    // Record pattern
               | "_"                                 // Wildcard
```

**Examples:**

```cantrip
enum Status {
    Active,
    Pending,
    Inactive,
}

enum Result<T, E> {
    Ok(T),
    Err(E),
}

enum Message {
    Quit,
    Move { x: i32, y: i32 },
    Write(String),
    ChangeColor(u8, u8, u8),
}
```

#### 2.3.1.2.2 Abstract Syntax

**Enum types:**

```
τ ::= enum E { V₁(τ₁), ..., Vₙ(τₙ) }    (enum type)
    | E                                   (enum type reference)
```

**Enum expressions:**

```
e ::= E.Vᵢ                               (unit variant)
    | E.Vᵢ(e)                            (tuple variant)
    | E.Vᵢ { f₁: e₁, ..., fₙ: eₙ }      (record variant)
```

**Pattern syntax:**

```
p ::= E.Vᵢ                               (unit pattern)
    | E.Vᵢ(p)                            (tuple pattern)
    | E.Vᵢ { f₁: p₁, ..., fₙ: pₙ }      (record pattern)
    | _                                  (wildcard)
```

### 2.3.1.3 Static Semantics

#### 2.3.1.3.1 Well-Formedness Rules

**Enum declaration:**

```
[WF-Enum]
Γ ⊢ T₁ : Type    ...    Γ ⊢ Tₙ : Type
variants V₁, ..., Vₙ are distinct
──────────────────────────────────────────────
Γ ⊢ enum E { V₁(T₁), ..., Vₙ(Tₙ) } : Type
```

**Generic enum:**

```
[WF-Generic-Enum]
Γ, α₁ : Type, ..., αₘ : Type ⊢ T₁ : Type    ...    Γ ⊢ Tₙ : Type
variants V₁, ..., Vₙ are distinct
────────────────────────────────────────────────────────────────────
Γ ⊢ enum E<α₁, ..., αₘ> { V₁(T₁), ..., Vₙ(Tₙ) } : Type
```

#### 2.3.1.3.2 Type Rules

**Enum variant construction:**

```
[T-Enum-Unit]
enum E { ..., V, ... } well-formed
──────────────────────────────────────────
Γ ⊢ E.V : E

[T-Enum-Tuple]
enum E { ..., V(T), ... } well-formed
Γ ⊢ e : T
──────────────────────────────────────────
Γ ⊢ E.V(e) : E

[T-Enum-Record]
enum E { ..., V { f₁: T₁, ..., fₙ: Tₙ }, ... } well-formed
Γ ⊢ e₁ : T₁    ...    Γ ⊢ eₙ : Tₙ
──────────────────────────────────────────
Γ ⊢ E.V { f₁: e₁, ..., fₙ: eₙ } : E
```

**Pattern matching:**

```
[T-Match]
Γ ⊢ e : E
enum E { V₁(T₁), ..., Vₙ(Tₙ) }
∀i. Γ, pᵢ ⊢ eᵢ : U
patterns {p₁, ..., pₙ} exhaustive for E
──────────────────────────────────────────────
Γ ⊢ match e { p₁ => e₁, ..., pₙ => eₙ } : U
```

**Pattern typing:**

```
[T-Pattern-Unit]
enum E { ..., V, ... }
────────────────────────────────────────────
Γ ⊢ E.V : E ⇝ Γ

[T-Pattern-Tuple]
enum E { ..., V(T), ... }
Γ ⊢ p : T ⇝ Γ'
────────────────────────────────────────────
Γ ⊢ E.V(p) : E ⇝ Γ'

[T-Pattern-Record]
enum E { ..., V { f₁: T₁, ..., fₙ: Tₙ }, ... }
Γ ⊢ p₁ : T₁ ⇝ Γ₁    ...    Γ ⊢ pₙ : Tₙ ⇝ Γₙ
────────────────────────────────────────────────
Γ ⊢ E.V { f₁: p₁, ..., fₙ: pₙ } : E ⇝ Γ₁ ∪ ... ∪ Γₙ
```

- **Definition 2.3.2 (Exhaustive Patterns).** A pattern family is exhaustive for `enum E { V₁, …, Vₙ }` when every constructor `Vᵢ` is matched and the payload patterns for each constructor are exhaustive in turn.

#### 2.3.1.3.3 Proven Properties

- **Theorem 2.3.1 (Exhaustiveness Checking).** The pattern set for `enum E` is exhaustive exactly when every constructor `Vᵢ` appears in the match (possibly via wildcard) and nested payload patterns are likewise exhaustive. The compiler enforces this via the recursive check defined in Definition 2.3.2.
- **Theorem 2.3.2 (Discriminant Uniqueness).** Distinct variants of an enum carry distinct discriminant values: `∀ i ≠ j. discriminant(Vᵢ) ≠ discriminant(Vⱼ)`.

#### 2.3.1.3.4 Pattern Forms

**Purpose:** Pattern matching supports multiple pattern forms for destructuring different data types. These forms are core to using enums effectively and enable expressive, type-safe data access.

**Literal patterns:**

Literal patterns match exact values:

```cantrip
match number {
    0 => "zero",
    1 => "one",
    2 => "two",
    _ => "many",  // Wildcard
}
```

**Type rule:**

```
[T-Pattern-Literal]
────────────────────────────────────────────
Γ ⊢ n : T ⇝ Γ
```

**Tuple patterns:**

Tuple patterns destructure tuple values and bind variables:

```cantrip
match point {
    (0, 0) => "origin",
    (0, y) => "on y-axis",  // Binds y
    (x, 0) => "on x-axis",  // Binds x
    (x, y) => "point",       // Binds both
}
```

**Type rule:**

```
[T-Pattern-Tuple]
Γ ⊢ p₁ : T₁ ⇝ Γ₁    ...    Γ ⊢ pₙ : Tₙ ⇝ Γₙ
────────────────────────────────────────────
Γ ⊢ (p₁, ..., pₙ) : (T₁, ..., Tₙ) ⇝ Γ₁ ∪ ... ∪ Γₙ
```

**Record patterns:**

Record patterns match record types and destructure fields:

```cantrip
match person {
    Person { name, age: 0..=17 } => "minor: {name}",
    Person { name, age: 18..=64 } => "adult: {name}",
    Person { name, age: 65.. } => "senior: {name}",
}
```

**Type rule:**

```
[T-Pattern-Record]
record R { f₁: T₁, ..., fₙ: Tₙ }
Γ ⊢ p₁ : T₁ ⇝ Γ₁    ...    Γ ⊢ pₙ : Tₙ ⇝ Γₙ
────────────────────────────────────────────
Γ ⊢ R { f₁: p₁, ..., fₙ: pₙ } : R ⇝ Γ₁ ∪ ... ∪ Γₙ
```

**Guard patterns:**

Guard patterns add boolean conditions to patterns:

```cantrip
match value {
    x if x > 0 => "positive",
    x if x < 0 => "negative",
    _ => "zero",
}
```

**Type rule:**

```
[T-Pattern-Guard]
Γ ⊢ p : T ⇝ Γ'
Γ' ⊢ e : bool
────────────────────────────────────────────
Γ ⊢ (p if e) : T ⇝ Γ'
```

**Wildcard pattern:**

The wildcard `_` matches any value without binding:

```
[T-Pattern-Wildcard]
────────────────────────────────────────────
Γ ⊢ _ : T ⇝ Γ
```

### 2.3.1.4 Dynamic Semantics

#### 2.3.1.4.1 Evaluation Rules

**Enum variant construction:**

```
[E-Enum-Unit]
────────────────────────────────────────────
⟨E.V, σ⟩ ⇓ ⟨E.V, σ⟩

[E-Enum-Tuple]
⟨e, σ⟩ ⇓ ⟨v, σ'⟩
────────────────────────────────────────────
⟨E.V(e), σ⟩ ⇓ ⟨E.V(v), σ'⟩

[E-Enum-Record]
⟨e₁, σ⟩ ⇓ ⟨v₁, σ₁⟩    ...    ⟨eₙ, σₙ₋₁⟩ ⇓ ⟨vₙ, σₙ⟩
────────────────────────────────────────────
⟨E.V { f₁: e₁, ..., fₙ: eₙ }, σ⟩ ⇓ ⟨E.V { f₁: v₁, ..., fₙ: vₙ }, σₙ⟩
```

**Pattern matching evaluation:**

```
[E-Match-First]
⟨e, σ⟩ ⇓ ⟨v, σ'⟩
match(p₁, v) = Some(θ)    // Pattern matches
⟨e₁[θ], σ'⟩ ⇓ ⟨v', σ''⟩
────────────────────────────────────────────
⟨match e { p₁ => e₁, ..., pₙ => eₙ }, σ⟩ ⇓ ⟨v', σ''⟩

[E-Match-Rest]
⟨e, σ⟩ ⇓ ⟨v, σ'⟩
match(p₁, v) = None    // Pattern doesn't match
⟨match e { p₂ => e₂, ..., pₙ => eₙ }, σ'⟩ ⇓ ⟨v', σ''⟩
────────────────────────────────────────────
⟨match e { p₁ => e₁, p₂ => e₂, ..., pₙ => eₙ }, σ⟩ ⇓ ⟨v', σ''⟩
```

#### 2.3.1.4.2 Memory Representation

**Enum layout:**

```
enum Layout = {
    discriminant: usize,    // Tag (variant index)
    payload: union {        // Largest variant
        variant₁: T₁,
        ...
        variantₙ: Tₙ,
    }
}
```

**Size calculation:**

```
size(enum E { V₁(T₁), ..., Vₙ(Tₙ) }) =
    size(discriminant) + max(size(T₁), ..., size(Tₙ)) + padding

align(enum E) = max(align(discriminant), align(T₁), ..., align(Tₙ))
```

**Example:**

```cantrip
enum Message {
    Quit,                      // size = 0
    Move { x: i32, y: i32 },   // size = 8
    Write(String),             // size = 24 (String = ptr + len + cap)
}

// Layout (64-bit system):
// ┌────────────┬──────────────────────────┐
// │discriminant│      payload (24 bytes)  │
// │  8 bytes   │      (largest variant)    │
// └────────────┴──────────────────────────┘
// Total: 32 bytes
```

#### 2.3.1.4.3 Operational Behavior

**Discriminant access:**

The discriminant value can be accessed for certain enums:

```cantrip
enum HttpStatus {
    Ok = 200,
    NotFound = 404,
    ServerError = 500,
}

let code: u16 = HttpStatus.Ok as u16  // 200
```

**Type rule for discriminant:**

```
[T-Discriminant]
enum E { V₁ = n₁, ..., Vₖ = nₖ }
───────────────────────────────────
E as IntType valid
```

#### 2.3.1.4.4 Discriminant Size and Optimization

**Discriminant sizing:** The compiler automatically selects the smallest integer type that can represent all variants.

**Default discriminant type selection:**

```
Number of variants  | Discriminant type | Size
--------------------|-------------------|------
1-256 variants      | u8                | 1 byte
257-65536 variants  | u16               | 2 bytes
65537-2³² variants  | u32               | 4 bytes
> 2³² variants      | u64/usize         | 8 bytes
```

**Explicit discriminant representation:**

Users can control discriminant size with `#[repr(...)]` attributes:

```cantrip
// Force specific discriminant size
#[repr(u8)]
enum SmallEnum {
    A,
    B,
    C,
}  // Discriminant guaranteed to be u8

#[repr(u16)]
enum MediumEnum {
    Variant1,
    Variant2,
    // ... up to 65536 variants
}

#[repr(u32)]
enum LargeEnum {
    // ... many variants
}
```

**Discriminant value constraints:**

When explicitly specifying discriminant values, they must:

1. Fit within the discriminant type
2. Be unique across all variants
3. Not exceed the maximum value for the chosen representation

```cantrip
#[repr(u8)]
enum HttpStatus {
    Ok = 200,
    Created = 201,
    NotFound = 404,
    ServerError = 500,
}

// ✗ ERROR: Discriminant value 500 doesn't fit in u8 (max 255)
```

**C-compatible layout:**

The `#[repr(C)]` attribute will C-compatible memory layout:

```cantrip
#[repr(C)]
enum Status {
    Active = 0,
    Inactive = 1,
    Pending = 2,
}

// Memory layout compatible with C enum:
// typedef enum {
//     Active = 0,
//     Inactive = 1,
//     Pending = 2
// } Status
```

**Combined with integer type:**

```cantrip
#[repr(C, u32)]
enum NetworkProtocol {
    TCP = 6,
    UDP = 17,
    ICMP = 1,
}

// C-compatible with explicit 32-bit discriminant
```

**Automatic discriminant assignment:**

If discriminants are not explicitly provided, they start at 0 and increment:

```cantrip
enum Color {
    Red,     // discriminant = 0
    Green,   // discriminant = 1
    Blue,    // discriminant = 2
}
```

**Mixed explicit and implicit:**

```cantrip
enum Priority {
    Low = 0,
    Medium,    // discriminant = 1 (auto-increment)
    High,      // discriminant = 2
    Critical = 100,
    Urgent,    // discriminant = 101
}
```

**Discriminant optimization (niche optimization):**

The compiler performs "niche optimization" for enums with specific patterns:

**Option<T> optimization:**

```cantrip
enum Option<T> {
    Some(T),
    None,
}

// If T is a non-nullable pointer (e.g., Ptr<i32>@Exclusive):
// Size(Option<Ptr<T>@Exclusive>) = Size(Ptr<T>@Exclusive)  // No extra discriminant!
// None represented by null pointer
```

**Memory layout comparison:**

```
Without optimization:
Option<Ptr<i32>@Exclusive>:
┌──────────────┬──────────────┐
│discriminant  │  pointer     │
│  8 bytes     │  8 bytes     │
└──────────────┴──────────────┘
Total: 16 bytes

With niche optimization:
Option<Ptr<i32>@Exclusive>:
┌──────────────┐
│  pointer     │  (None = null, Some(_) = valid ptr)
│  8 bytes     │
└──────────────┘
Total: 8 bytes (50% smaller!)
```

**Other niche-optimizable types:**

```cantrip
// NonZeroU32 has niche (0)
enum Option<NonZeroU32> {
    Some(NonZeroU32),
    None,  // Represented as 0
}

// Size(Option<NonZeroU32>) = Size(NonZeroU32) = 4 bytes

// Bool has two niches (2-255)
enum Option<bool> {
    Some(bool),   // 0 or 1
    None,         // 2 (or any value 2-255)
}
```

**Layout for zero-sized types:**

Enums containing only zero-sized types have minimal layout:

```cantrip
enum ZeroSized {
    A,
    B,
    C,
}

// Size: 1 byte (discriminant only, no payload)
// Alignment: 1 byte
```

**Complex variant layout:**

```cantrip
enum Message {
    Quit,                        // ZST, size = 0
    Move { x: i32, y: i32 },     // size = 8
    Write(String),               // size = 24 (on 64-bit)
    ChangeColor(u8, u8, u8),     // size = 3
}

// Layout calculation:
// Discriminant: u8 (4 variants < 256)
// Payload size: max(0, 8, 24, 3) = 24
// Alignment: max(1, 4, 8, 1) = 8
//
// Memory layout:
// ┌─────────────┬─────────────────────────────┐
// │ disc (1 byte)│  payload (24 bytes)        │
// │ + padding    │  (largest variant)         │
// └─────────────┴─────────────────────────────┘
// Total: 32 bytes (1 + 7 padding + 24)
```

**Discriminant size rule:**

```
[DiscriminantSize]
enum E { V₁, ..., Vₙ }
n = number of variants
─────────────────────────────────
size(discriminant) = ⌈log₂(n) / 8⌉ bytes

(rounded up to next integer size: u8, u16, u32, u64)
```

**Type rule for repr attribute:**

```
[T-ReprExplicit]
#[repr(IntType)]
enum E { V₁, ..., Vₙ }
all discriminants fit in IntType
─────────────────────────────────
discriminant type = IntType
```

**Theorem 2.3.3 (Discriminant Minimality):** Without explicit `#[repr(...)]`, the compiler selects the smallest integer type sufficient to represent all variants.

**Proof sketch:** The compiler counts the number of variants `n` and selects the smallest unsigned integer type `uₖ` such that `2^(8k) ≥ n`. This minimizes memory usage while ensuring all variants are representable. ∎

**Corollary 2.3.3.1:** Enums with ≤256 variants have 1-byte discriminants by default.

**Theorem 2.3.4 (Niche Optimization Soundness):** Niche optimization preserves type safety. A niche-optimized enum can represent all variants without ambiguity.

**Proof sketch:** Niche optimization uses invalid bit patterns (niches) in the payload type to represent discriminant values. Since these bit patterns are invalid for the payload type, they cannot be confused with valid payload values. The compiler will exhaustive case coverage to prevent accessing niches as valid payloads. ∎

**Corollary 2.3.4.1:** `Option<Ptr<T>@Exclusive>` has the same size as `Ptr<T>@Exclusive` for all pointer types.

**Layout attributes summary:**

| Attribute              | Effect                                       |
| ---------------------- | -------------------------------------------- |
| `#[repr(u8)]`          | Force u8 discriminant (max 256 variants)     |
| `#[repr(u16)]`         | Force u16 discriminant (max 65536 variants)  |
| `#[repr(u32)]`         | Force u32 discriminant                       |
| `#[repr(u64)]`         | Force u64 discriminant                       |
| `#[repr(C)]`           | C-compatible layout (discriminant + union)   |
| `#[repr(C, u32)]`      | C-compatible with explicit discriminant size |
| `#[repr(transparent)]` | Single-variant enum, same layout as payload  |

**Transparent representation:**

```cantrip
#[repr(transparent)]
enum Wrapper<T> {
    Value(T),
}

// Size(Wrapper<T>) = Size(T)
// No discriminant overhead for single-variant enum
// Useful for newtype pattern with enums
```

**FFI considerations:**

When interfacing with C code, use `#[repr(C)]` to ensure compatible layout:

```cantrip
// Cantrip
#[repr(C)]
enum FileType {
    Regular = 0,
    Directory = 1,
    Symlink = 2,
}

// Corresponding C enum:
// typedef enum {
//     Regular = 0,
//     Directory = 1,
//     Symlink = 2
// } FileType
```

**Examples:**

**Example 1: Minimal discriminant**

```cantrip
enum TinyEnum {
    A,
    B,
}

// 2 variants → u8 discriminant (1 byte)
// Size: 1 byte (discriminant only, no payload)
```

**Example 2: Niche optimization**

```cantrip
record NonNull<T>(*const T)  // Cannot be null

enum Option<NonNull<i32>> {
    Some(NonNull<i32>),  // Valid pointer
    None,                // Null pointer (niche)
}

// Size: 8 bytes (just the pointer)
// No separate discriminant needed
```

**Example 3: Large discriminant**

```cantrip
enum LargeEnum {
    V0, V1, V2, ..., V300,  // 301 variants
}

// 301 variants > 256 → u16 discriminant (2 bytes)
// Size: 2 bytes (discriminant only, no payload)
```

**Example 4: Explicit discriminant control**

```cantrip
#[repr(u32)]
enum Opcode {
    NOP = 0x00,
    LOAD = 0x01,
    STORE = 0x02,
    ADD = 0x10,
    SUB = 0x11,
    HALT = 0xFF,
}

// Discriminant: u32 (4 bytes)
// Explicit values for protocol compatibility
```

---

**Definition 2.3.3 (Union):** A union is an untagged sum type where all variants occupy the same memory location. Formally, if `union U { f₁: T₁, ..., fₙ: Tₙ }` then `⟦U⟧ = T₁ ⊔ ... ⊔ Tₙ` where ⊔ denotes overlapping union (not disjoint sum), with `size(U) = max(size(T₁), ..., size(Tₙ))` and no runtime discriminant. The active variant is tracked by the programmer, making unions inherently unsafe.

## 2.3.2 Unions

### 2.3.2.1 Overview

**Key innovation/purpose:** Unions provide C-compatible untagged sum types for FFI interoperability, enabling direct mapping to C union types without discriminant overhead while maintaining explicit safety boundaries through the effect system.

##### Guidance (non-normative)

- Restrict unions to FFI and low-level overlays where manual tag tracking is unavoidable; otherwise prefer enums or records.
- Declare the `unsafe.union` effect whenever accessing a union field and pair unions with explicit tag records to document the active variant.

### 2.3.2.2 Syntax

#### 2.3.2.2.1 Concrete Syntax

**Union declaration:**

```ebnf
UnionDecl    ::= "union" Ident GenericParams? "{" FieldList "}"
FieldList    ::= Field (Newline Field)* Newline?
Field        ::= Visibility? Ident ":" Type
Visibility   ::= "public" | "private"
GenericParams ::= "<" TypeParam ("," TypeParam)* ">"
```

**Union construction:**

```ebnf
UnionExpr    ::= Ident "{" FieldInit "}"
FieldInit    ::= Ident ":" Expr
```

**Variant access:**

```ebnf
VariantAccess ::= Expr "." Ident
```

**Examples:**

```cantrip
// Simple union
union Value {
    i: i32
    f: f32
}

// C-compatible union
[[repr(C)]]
union Data {
    integer: i32
    floating: f64
    bytes: [u8; 8]
}

// Generic union
union Either<T, U> {
    left: T
    right: U
}

// Construction (only one variant initialized)
let v = Value { i: 42 }
let d = Data { floating: 3.14 }

// Field access (requires unsafe.union effect)
procedure get_int(v: Value): i32
    uses unsafe.union
{
    v.i  // Unsafe: may read invalid data if f was active
}
```

#### 2.3.2.2.2 Abstract Syntax

**Union types:**

```
τ ::= union U { f₁: τ₁, ..., fₙ: τₙ }    (union type)
    | U                                   (union type reference)
```

**Union expressions:**

```
e ::= U { f: e }                          (union literal, single variant)
    | e.f                                 (variant access, unsafe)
```

**Union patterns:**

```
p ::= U { f: p }                          (union pattern, limited support)
```

**Memory representation:**

```
⟦union U { f₁: T₁, ..., fₙ: Tₙ }⟧ = {
    storage: max(⟦T₁⟧, ..., ⟦Tₙ⟧)      (all variants share storage)
    size: max(size(T₁), ..., size(Tₙ))
    align: max(align(T₁), ..., align(Tₙ))
    discriminant: none                    (no tag)
}
```

### 2.3.2.3 Static Semantics

#### 2.3.2.3.1 Well-Formedness Rules

**[WF-Union] Union Declaration:**

```
[WF-Union]
Γ ⊢ T₁ : Type    ...    Γ ⊢ Tₙ : Type
variants f₁, ..., fₙ are distinct
n ≥ 1    (at least one variant)
──────────────────────────────────────────────
Γ ⊢ union U { f₁: T₁, ..., fₙ: Tₙ } : Type
```

**Explanation:** Unions must have at least one variant with distinct names. All variant types must be well-formed. Unlike records, variants overlap in memory.

```cantrip
// ✓ Valid: at least one variant
union Data {
    i: i32
    f: f32
}

// ✗ Invalid: zero variants
// union Empty { }

// ✗ Invalid: duplicate variant names
// union Bad {
//     x: i32
//     x: f32
// }
```

**[WF-Union-Generic] Generic Union:**

```
[WF-Union-Generic]
Γ, α₁ : Type, ..., αₘ : Type ⊢ T₁ : Type    ...    Γ, α₁ : Type, ..., αₘ : Type ⊢ Tₙ : Type
variants f₁, ..., fₙ are distinct
────────────────────────────────────────────────────────────────────
Γ ⊢ union U<α₁, ..., αₘ> { f₁: T₁, ..., fₙ: Tₙ } : Type
```

**Explanation:** Unions can be generic over type parameters.

```cantrip
// Generic union
union Either<L, R> {
    left: L
    right: R
}

let e1: Either<i32, String> = Either { left: 42 }
let e2: Either<i32, String> = Either { right: String.from("hello") }
```

**[WF-Union-ZST-Restriction] Zero-Sized Type Restriction:**

```
[WF-Union-ZST-Restriction]
union U { f₁: T₁, ..., fₙ: Tₙ }
∃ i. size(Tᵢ) > 0    (at least one non-ZST variant)
────────────────────────────────────────────
Γ ⊢ U well-formed
```

**Explanation:** Unions with only zero-sized types are disallowed (use unit type `()` instead).

```cantrip
// ✗ Invalid: all variants are ZST
// union AllZero {
//     a: ()
//     b: ()
// }

// ✓ Valid: at least one non-ZST
union Mixed {
    nothing: ()
    something: i32
}
```

#### 2.3.2.3.2 Type Rules

**[T-Union-Construct] Union Construction:**

```
[T-Union-Construct]
union U { f₁: T₁, ..., fᵢ: Tᵢ, ..., fₙ: Tₙ } declared
Γ ⊢ e : Tᵢ
────────────────────────────────────────────
Γ ⊢ U { fᵢ: e } : U
```

**Explanation:** Union construction initializes exactly one variant. The expression type must match the variant's type.

```cantrip
union Data {
    i: i32
    f: f32
    s: String
}

let d1 = Data { i: 42 }                    // ✓ i32 matches i variant
let d2 = Data { f: 3.14 }                  // ✓ f32 matches f variant
let d3 = Data { s: String.from("hello") }  // ✓ String matches s variant

// ✗ ERROR: type mismatch
// let bad = Data { i: 3.14 }              // f32 doesn't match i32
```

**[T-Union-Single-Init] Single Variant Initialization:**

```
[T-Union-Single-Init]
union U { f₁: T₁, ..., fₙ: Tₙ }
U { fᵢ: e, fⱼ: e' } where i ≠ j
────────────────────────────────────────────
ERROR: Multiple variant initialization
```

**Explanation:** Only one variant can be initialized. Initializing multiple variants is an error.

```cantrip
union Data {
    i: i32
    f: f32
}

// ✗ ERROR: Cannot initialize multiple variants
// let bad = Data { i: 42, f: 3.14 }
```

**[T-Union-Access] Variant Access (Unsafe):**

```
[T-Union-Access]
union U { f₁: T₁, ..., fᵢ: Tᵢ, ..., fₙ: Tₙ }
Γ ⊢ e : U
unsafe.union ∈ Γ.effects    (requires unsafe.union effect)
────────────────────────────────────────────
Γ ⊢ e.fᵢ : Tᵢ
```

**Explanation:** Accessing union variants requires the `unsafe.union` effect. Reading the wrong variant is undefined behavior.

```cantrip
union Data {
    i: i32
    f: f32
}

// ✓ Correct: declares unsafe.union effect
procedure read_int(d: Data): i32
    uses unsafe.union
{
    d.i
}

// ✗ ERROR: Missing unsafe.union effect
// procedure bad(d: Data): i32 {
//     d.i  // E7001: Union field access requires unsafe.union effect
// }
```

**[T-Union-Safe-Access] Contract-Guarded Access:**

```
[T-Union-Safe-Access]
union U { f₁: T₁, ..., fᵢ: Tᵢ, ..., fₙ: Tₙ }
record R { tag: E, data: U }    (tagged wrapper)
Γ ⊢ self : R
Γ ⊢ self.tag == Eᵢ    (precondition: correct tag)
unsafe.union ∈ Γ.effects
────────────────────────────────────────────
Γ ⊢ self.data.fᵢ : Tᵢ    (safe access)
```

**Explanation:** Contracts can document which variant is active, making access safer (but still requires unsafe.union).

```cantrip
enum Tag { Int, Float }

union Data {
    i: i32
    f: f32
}

record Tagged {
    tag: Tag
    data: Data

    procedure get_int(self: Self): i32
        must self.tag == Tag.Int    // Precondition: tag must match
        uses unsafe.union
    {
        self.data.i  // Safe because precondition guarantees correct variant
    }
}
```

**[T-Union-Assign] Variant Assignment:**

```
[T-Union-Assign]
union U { f₁: T₁, ..., fᵢ: Tᵢ, ..., fₙ: Tₙ }
Γ ⊢ e₁ : mut U
Γ ⊢ e₂ : Tᵢ
unsafe.union ∈ Γ.effects
────────────────────────────────────────────
Γ ⊢ e₁.fᵢ = e₂ : ()    (overwrites all variants)
```

**Explanation:** Assigning to a union variant requires mutable access and overwrites the entire union.

```cantrip
union Data {
    i: i32
    f: f32
}

procedure update(d: mut Data)
    uses unsafe.union
{
    d.i = 42     // Overwrites entire union (including f)
    d.f = 3.14   // Overwrites i
}
```

**[T-Union-Permission] Permission Application:**

```
[T-Union-Permission]
union U { f₁: T₁, ..., fₙ: Tₙ }
π ∈ {own, mut, immutable, iso}
────────────────────────────────────────────
Γ ⊢ π U : Type
```

**Explanation:** Unions support all permission qualifiers.

```cantrip
union Data {
    i: i32
    f: f32
}

procedure consume(data: own Data) { }           // Owned
procedure modify(data: mut Data) { }            // Mutable
procedure read(data: Data) { }                  // Immutable
procedure isolate(data: iso Data) { }           // Isolated
```

**[T-Union-Copy] Copy Semantics:**

```
[T-Union-Copy]
union U { f₁: T₁, ..., fₙ: Tₙ }
T₁ : Copy ∧ ... ∧ Tₙ : Copy
────────────────────────────────────────────
U : Copy
```

**Explanation:** A union is `Copy` if and only if all variants are `Copy`.

```cantrip
// Copy union (all variants are Copy)
union CopyData {
    i: i32      // Copy
    f: f32      // Copy
    b: bool     // Copy
}

let d1 = CopyData { i: 42 }
let d2 = d1  // Copy (bitwise copy)
// d1 still usable

// Non-Copy union (String is not Copy)
union NonCopyData {
    i: i32
    s: String   // Not Copy
}

let d1 = NonCopyData { s: String.from("hello") }
let d2 = d1  // Move
// d1 no longer usable
```

**[T-Union-Move] Move Semantics:**

```
[T-Union-Move]
union U { f₁: T₁, ..., fₙ: Tₙ }
∃ i. Tᵢ : ¬Copy    (at least one non-Copy variant)
Γ ⊢ e₁ : U
────────────────────────────────────────────
Γ ⊢ let x = e₁ ⟹ e₁ moved, cannot be used
```

**Explanation:** Unions with any non-Copy variant are moved by default.

```cantrip
union Data {
    i: i32
    s: String  // Not Copy
}

let d1 = Data { s: String.from("hello") }
let d2 = d1  // Move entire union
// d1 is now invalid, even though we initialized with String

// This is safe because:
// - We don't know which variant is active
// - Must assume String could be active
// - Moving entire union is safe
```

**[T-Union-Size] Size Calculation:**

```
[T-Union-Size]
union U { f₁: T₁, ..., fₙ: Tₙ }
────────────────────────────────────────────
size(U) = max(size(T₁), ..., size(Tₙ))
align(U) = max(align(T₁), ..., align(Tₙ))
```

**Explanation:** Union size is the maximum size of any variant. All variants share the same storage.

```cantrip
union Data {
    i: i32       // size=4, align=4
    f: f64       // size=8, align=8
    bytes: [u8; 16]  // size=16, align=1
}

// size(Data) = max(4, 8, 16) = 16
// align(Data) = max(4, 8, 1) = 8
// Total: 16 bytes aligned to 8-byte boundary
```

**[T-Union-Repr-C] C-Compatible Layout:**

```
[T-Union-Repr-C]
[[repr(C)]]
union U { f₁: T₁, ..., fₙ: Tₙ }
────────────────────────────────────────────
layout(U) matches C union U { T₁ f₁; ...; Tₙ fₙ; }
```

**Explanation:** `[[repr(C)]]` ensures the union has the same layout as a C union.

```cantrip
// Cantrip
[[repr(C)]]
union Value {
    i: i32
    f: f32
}

// Equivalent C
// union Value {
//     int32_t i;
//     float f;
// }
```

**[T-Union-Nested] Nested Unions:**

```
[T-Union-Nested]
union U₁ { ... }
union U₂ { f: U₁, ... }
────────────────────────────────────────────
Γ ⊢ U₂ : Type    (unions can nest)
```

**Explanation:** Unions can contain other unions.

```cantrip
union Inner {
    i: i32
    f: f32
}

union Outer {
    inner: Inner
    s: String
}

let o = Outer { inner: Inner { i: 42 } }
```

**[T-Union-Generic-Inst] Generic Union Instantiation:**

```
[T-Union-Generic-Inst]
union U<α₁, ..., αₖ> { f₁: T₁, ..., fₙ: Tₙ } declared
Γ ⊢ S₁ : Type    ...    Γ ⊢ Sₖ : Type
────────────────────────────────────────────
Γ ⊢ U<S₁, ..., Sₖ> : Type
```

**Explanation:** Generic unions are instantiated by providing concrete types.

```cantrip
union Either<L, R> {
    left: L
    right: R
}

let e1: Either<i32, String> = Either { left: 42 }
let e2: Either<bool, f64> = Either { right: 3.14 }
```

**[T-Union-Subtyping] No Structural Subtyping:**

```
[T-Union-Subtyping]
union U₁ { f₁: T₁, ..., fₙ: Tₙ }
union U₂ { f₁: T₁, ..., fₙ: Tₙ }
U₁ ≠ U₂
────────────────────────────────────────────
U₁ ≢ U₂    (no structural equivalence)
```

**Explanation:** Unions use nominal typing. Identical structure doesn't imply type equivalence.

```cantrip
union A {
    i: i32
    f: f32
}

union B {
    i: i32
    f: f32
}

let a = A { i: 42 }
// let b: B = a  // ✗ ERROR: type mismatch (A ≠ B)
```

**[T-Union-Pattern] Limited Pattern Matching:**

```
[T-Union-Pattern]
union U { f₁: T₁, ..., fₙ: Tₙ }
────────────────────────────────────────────
Pattern matching on unions is restricted (see §8.5.3.3)
```

**Explanation:** Direct pattern matching on unions is limited because the active variant is unknown.

```cantrip
union Data {
    i: i32
    f: f32
}

// ✗ Invalid: Cannot pattern match directly on union
// match data {
//     Data { i } => ...,  // How do we know i is active?
//     Data { f } => ...,
// }

// ✓ Valid: Pattern match on tagged wrapper
enum Tag { Int, Float }
record Tagged { tag: Tag, data: Data }

match tagged {
    Tagged { tag: Tag.Int, data } => {
        // Safe to access data.i
    },
    Tagged { tag: Tag.Float, data } => {
        // Safe to access data.f
    },
}
```

#### 2.3.2.3.3 Type Properties

**Theorem 8.5.1 (Union Nominality):**

Unions with identical variant names and types are distinct types. For any unions `union A { f: T }` and `union B { f: T }`, `A ≠ B` implies `A ≢ B`.

**Proof sketch:** The type system assigns each union declaration a unique nominal identity. Even if variant types are structurally identical, the nominal identities differ. ∎

**Corollary 8.5.1:** Union types provide type safety through nominal distinction.

**Theorem 8.5.2 (Size Maximality):**

For union U with variants of types T₁, ..., Tₙ:

```
size(U) = max(size(T₁), ..., size(Tₙ))
```

**Proof sketch:** All variants occupy the same memory location. The union must be large enough to hold any variant. Therefore, size must be the maximum. ∎

**Corollary 8.5.2:** Unions are space-efficient when variants have similar sizes.

**Theorem 8.5.3 (Copy Capability):**

A union U implements Copy if and only if all variants implement Copy:

```
union U { f₁: T₁, ..., fₙ: Tₙ } : Copy
⟺
T₁ : Copy ∧ ... ∧ Tₙ : Copy
```

**Semantics:** Unions pass by permission by default. `.copy()` performs bitwise copy when all variants are Copy-capable.

**Proof sketch:**

- (⇒) If U is Copy, bitwise copy must be valid. Since any variant could be active, all must support bitwise copy, thus all Tᵢ : Copy.
- (⇐) If all Tᵢ : Copy, bitwise copy is safe regardless of active variant, thus U : Copy.
  ∎

**Theorem 8.5.4 (Unsafe Access):**

All union variant access is inherently unsafe without additional invariants:

```
∀ u : U, ∀ f : variant of U.
access(u.f) is safe ⟺ f is the active variant
```

**Proof sketch:** Reading a variant that is not active results in type confusion - interpreting memory of type Tᵢ as type Tⱼ where i ≠ j, which is undefined behavior in general. Only external invariants (tags, contracts) can establish safety. ∎

**Corollary 8.5.3:** Unions require `unsafe.union` effect for all field access.

**Theorem 8.5.5 (Layout Compatibility):**

For unions marked `[[repr(C)]]`, the memory layout matches the equivalent C union:

```
[[repr(C)]] union U { f₁: T₁, ..., fₙ: Tₙ } in Cantrip
⟺
union U { T₁ f₁; ...; Tₙ fₙ; } in C
```

**Proof sketch:** The `[[repr(C)]]` attribute instructs the compiler to follow C's union layout rules: overlapping storage, maximum size, maximum alignment, no padding between variants (only trailing padding for alignment). ∎

**Theorem 8.5.6 (Zero Runtime Overhead):**

Unions have zero abstraction cost compared to raw memory reinterpretation:

```
cost(union U) = cost(reinterpret_cast in C++)
```

**Proof sketch:** Unions are a compile-time construct. At runtime, they are just memory blocks with maximum size. Field access is a zero-cost offset calculation (always offset 0). No runtime checks, no discriminant, no indirection. ∎

### 2.3.2.4 Dynamic Semantics

#### 2.3.2.4.1 Evaluation Rules

**[E-Union-Construct] Union Construction:**

```
[E-Union-Construct]
⟨e, σ⟩ ⇓ ⟨v, σ'⟩
────────────────────────────────────────────
⟨U { f: e }, σ⟩ ⇓ ⟨U { f: v }, σ'⟩
```

**Explanation:** Union construction evaluates the initializer expression and stores it in the union's storage.

**[E-Union-Access] Variant Access:**

```
[E-Union-Access]
⟨e, σ⟩ ⇓ ⟨U { fᵢ: v }, σ'⟩
────────────────────────────────────────────
⟨e.fⱼ, σ'⟩ ⇓ ⟨interpret(storage, Tⱼ), σ'⟩
```

**Explanation:** Accessing variant fⱼ interprets the union's storage as type Tⱼ, regardless of which variant was initialized. If i ≠ j, this is undefined behavior.

**[E-Union-Assign] Variant Assignment:**

```
[E-Union-Assign]
⟨e₁, σ⟩ ⇓ ⟨u, σ₁⟩    u : mut U
⟨e₂, σ₁⟩ ⇓ ⟨v, σ₂⟩   v : Tᵢ
────────────────────────────────────────────
⟨e₁.fᵢ = e₂, σ⟩ ⇓ ⟨(), σ₂[u.storage := v]⟩
```

**Explanation:** Assigning to a variant overwrites the entire union's storage.

#### 2.3.2.4.2 Memory Representation

**Union memory layout:**

```
union U { f₁: T₁, f₂: T₂, ..., fₙ: Tₙ }

All variants share the same memory location:
┌────────────────────────────────┐
│  max(size(T₁), ..., size(Tₙ))  │  All variants overlap
└────────────────────────────────┘
offset 0: f₁ starts here
offset 0: f₂ starts here (overlaps f₁)
...
offset 0: fₙ starts here (overlaps all)

Size: max(size(T₁), ..., size(Tₙ))
Alignment: max(align(T₁), ..., align(Tₙ))
```

**Example: Different-sized variants:**

```cantrip
union Data {
    byte: u8       // size=1, align=1
    word: u32      // size=4, align=4
    dword: u64     // size=8, align=8
}

Memory layout:
┌──────────────────────────────┐
│         8 bytes              │  All variants overlap at offset 0
└──────────────────────────────┘
Size: max(1, 4, 8) = 8 bytes
Alignment: max(1, 4, 8) = 8 bytes
```

**C-compatible layout:**

```cantrip
[[repr(C)]]
union CData {
    i: i32
    f: f32
    d: f64
}

// Matches C: union CData { int32_t i; float f; double d; };
Size: 8 bytes, Alignment: 8 bytes
```

#### 2.3.2.4.3 Operational Behavior

**Initialization and overwrites:**

```cantrip
union Data {
    i: i32
    f: f32
}

let d1 = Data { i: 42 }
// Memory: [42, 0, 0, 0] (little-endian)

var d2 = Data { i: 42 }
d2.f = 3.14
// Memory: [0xC3, 0xF5, 0x48, 0x40] - integer overwritten!
```

**Undefined behavior:**

```cantrip
let d = Data { i: 42 }
let x = d.f  // UB! Reading float when int was stored
```

**Type punning (legal but unsafe):**

```cantrip
union FloatPun {
    f: f32
    bits: u32
}

procedure float_to_bits(f: f32): u32
    uses unsafe.union
{
    FloatPun { f: f }.bits
}
```

### 2.3.2.5 Safety and Integration

#### 2.3.2.5.1 Effect System Integration

**Effect: `unsafe.union`**

All union variant access requires the `unsafe.union` effect:

```cantrip
effect unsafe.union {
    doc = "Access to union variants without discriminant checking"
    category = "unsafe"
}

union Data {
    i: i32
    f: f32
}

// ✓ Correct: declares unsafe.union
procedure read_int(d: Data): i32
    uses unsafe.union
{
    d.i
}

// ✗ ERROR E7001: Missing unsafe.union effect
// procedure bad(d: Data): i32 {
//     d.i
// }
```

**Effect hierarchy:**

```
unsafe
├── unsafe.ptr        (raw pointer dereferencing)
├── unsafe.union      (union variant access)
├── unsafe.cast       (type punning casts)
├── unsafe.asm        (inline assembly)
└── unsafe.transmute  (memory reinterpretation)

ffi
├── ffi.call          (calling C functions)
└── ffi.union         (FFI union operations)
```

**Effect composition:**

```cantrip
// FFI procedure using unions
procedure call_c_function(): i32
    uses ffi.call, unsafe.union
{
    let result = external_c_func()
    result.data.value  // Requires both ffi.call and unsafe.union
}
```

#### 2.3.2.5.2 Contract-Based Safety

**Pattern: Tagged union wrapper:**

```cantrip
enum ValueKind { Integer, Floating }

union ValueData {
    i: i64
    f: f64
}

record Value {
    kind: ValueKind
    data: ValueData

    procedure new_int(value: i64): own Value {
        own Value { kind: ValueKind.Integer, data: ValueData { i: value } }
    }

    procedure get_int(self: Self): i64
        must self.kind == ValueKind.Integer
        uses unsafe.union
    {
        self.data.i  // Safe: precondition ensures i is active
    }

    procedure as_int(self: Self): Option<i64>
        uses unsafe.union
    {
        match self.kind {
            ValueKind.Integer => Option.Some(self.data.i),
            _ => Option.None,
        }
    }
}
```

#### 2.3.2.5.3 Permission System Integration

**Unions support all permissions:**

```cantrip
union Data {
    i: i32
    f: f32
}

procedure consume(data: own Data) { }        // Owned
procedure modify(data: mut Data) { }         // Mutable
procedure read(data: Data) { }               // Immutable
procedure isolate(data: iso Data) { }        // Isolated
```

**Copy semantics:**

```cantrip
// Copy union (all variants Copy)
union CopyData {
    i: i32
    f: f32
}

// Non-Copy union (String not Copy)
union NonCopyData {
    i: i32
    s: String
}
```

#### 2.3.2.5.4 Region Allocation Support

Unions support region allocation for temporary data:

```cantrip
union Data { i: i32, f: f64 }

procedure process_batch(items: [Item]): Result
    uses alloc.region
{
    region temp {
        var buffer = Data { i: 0 }
        for item in items {
            buffer.i = process(item)
        }
        // buffer automatically freed at region end
    }
}
```

### 2.3.2.6 Advanced Features

#### 2.3.2.6.1 FFI Integration

**C structure with embedded union:**

```cantrip
// C: struct Event { uint32_t type; union { ... } data; }

[[repr(C)]]
record Event {
    type_id: u32
    data: EventData
}

[[repr(C)]]
union EventData {
    mouse: MouseEvent
    keyboard: KeyboardEvent
}

[[repr(C)]]
record MouseEvent { x: i32, y: i32 }

[[repr(C)]]
record KeyboardEvent { key: u8 }

extern "C" {
    procedure poll_event(event: mut Event): bool
        uses ffi.call, unsafe.union
}

procedure handle_event(event: Event)
    uses unsafe.union
{
    match event.type_id {
        1 => println("Mouse: ({}, {})", event.data.mouse.x, event.data.mouse.y),
        2 => println("Key: {}", event.data.keyboard.key),
        _ => {},
    }
}
```

**Win32 LARGE_INTEGER:**

```cantrip
// C: typedef union { struct { DWORD LowPart; LONG HighPart; }; LONGLONG QuadPart; } LARGE_INTEGER;

[[repr(C)]]
union LargeInteger {
    parts: LargeIntegerParts
    quad: i64
}

[[repr(C)]]
record LargeIntegerParts { low: u32, high: i32 }

extern "C" {
    procedure QueryPerformanceCounter(counter: mut LargeInteger): bool
        uses ffi.call, unsafe.union
}

procedure get_performance_counter(): i64
    uses ffi.call, unsafe.union
{
    var counter = LargeInteger { quad: 0 }
    QueryPerformanceCounter(mut counter)
    counter.quad
}
```

#### 2.3.2.6.2 Hardware Register Access

**Memory-mapped I/O register:**

```cantrip
[[repr(C)]]
union ControlRegister {
    raw: u32
    bits: ControlBits
}

[[repr(C, packed)]]
record ControlBits { enable: u8, mode: u8, interrupt: u8 }

const CONTROL_REG: *mut ControlRegister = 0x4000_0000 as *mut ControlRegister

procedure enable_device()
    uses unsafe.ptr, unsafe.union
{
    var reg = *CONTROL_REG
    reg.bits.enable = 1
    reg.bits.mode = 3
    *CONTROL_REG = reg
}
```

#### 2.3.2.6.3 Type Punning and Bit Manipulation

**Float/int conversion:**

```cantrip
union FloatInt {
    f: f32
    i: u32
}

procedure float_bits(f: f32): u32
    uses unsafe.union
{
    FloatInt { f: f }.i
}

procedure is_nan(f: f32): bool
    uses unsafe.union
{
    let bits = float_bits(f)
    ((bits >> 23) & 0xFF) == 0xFF && (bits & 0x7F_FFFF) != 0
}
```

**Endianness detection:**

```cantrip
union EndianTest { value: u32, bytes: [u8; 4] }

procedure is_little_endian(): bool
    uses unsafe.union
{
    EndianTest { value: 1 }.bytes[0] == 1
}
```

### 2.3.2.7 Comparison to Alternatives

| Feature              | Union               | Enum                         | Record          | Type Alias      |
| -------------------- | ------------------- | ---------------------------- | --------------- | --------------- |
| **Memory layout**    | Overlapping         | Tagged + largest variant     | Sequential      | Transparent     |
| **Discriminant**     | None                | Automatic                    | N/A             | N/A             |
| **Type safety**      | Unsafe              | Safe                         | Safe            | Transparent     |
| **C FFI**            | Perfect match       | Tag overhead                 | Perfect match   | Depends         |
| **Pattern matching** | Limited             | Full support                 | Full support    | Depends         |
| **Space efficiency** | max(variants)       | discriminant + max(variants) | sum(fields)     | Underlying type |
| **Use case**         | C interop, hardware | General variants             | Structured data | Type branding   |

### 2.3.2.8 Proven Properties

- **Layout bound.** As defined in §2.3.2.2, unions share storage across variants; `size(U) = max(size(Tᵢ))` and `align(U) = max(align(Tᵢ))`.
- **Untagged semantics.** Unions carry no discriminant; the active variant is tracked externally by user code.
- **Non-ZST requirement.** At least one variant must have non-zero size (§2.3.2.3.1), preventing degenerate overlapping of zero-sized values.
- **Effect discipline.** Reading or writing a union field requires the `unsafe.union` effect, ensuring every access is opt-in and auditable (§2.3.2.4).

### 2.3.2.9 Safety Guidelines

**DO:**

- ✓ Always wrap unions in records with explicit tag fields
- ✓ Use contracts to document which variant is active
- ✓ Use `[[repr(C)]]` for FFI unions
- ✓ Prefer enums over unions in pure Cantrip code
- ✓ Use unions only when interfacing with C or hardware
- ✓ Document which variant is active in comments

**DON'T:**

- ✗ Access union variants without knowing which is active
- ✗ Use unions for general variant types (use enums)
- ✗ Assume unions are Copy unless all variants are Copy
- ✗ Forget to handle owned types (String, Vec) in unions properly
- ✗ Use unions without clear external invariants
- ✗ Pattern match directly on unions (match on tags instead)
- ✗ Ignore the `unsafe.union` effect requirement

**Common pitfalls:**

```cantrip
// ❌ BAD: No tag, no way to know active variant
union Bad {
    i: i32
    s: String
}

let x = Bad { s: String.from("hello") }
// Later... which variant is active? Can't tell!

// ✓ GOOD: Tagged wrapper
enum Kind { Int, Str }

union GoodData {
    i: i32
    s: String
}

record Good {
    kind: Kind
    data: GoodData
}
```

---

**Definition 2.3.4 (Modal):** A modal is a type with an explicit finite state machine, where each state @Sᵢ can have different data fields and the compiler enforces valid state transitions. Formally, a modal M = (S, Σ, δ, s₀) where S is a finite set of states, Σ is the alphabet of procedures, δ: S × Σ → S is the transition function, and s₀ ∈ S is the initial state. Modals bring state machine verification into the type system, ensuring compile-time correctness for stateful resources and business logic.

## 2.3.3 Modals: State Machines as Types

### 2.3.3.1 Overview

**Key innovation/purpose:** Modals make state machines first-class types with compile-time verification, ensuring that state-dependent operations are only called in valid states and all state transitions are explicit and type-checked.

##### Guidance (non-normative)

- Model resources whose valid operations depend on runtime state (files, protocols, transactions) as modals; the transition rules in §2.3.3.3 enforce linear hand-offs.
- Prefer enums or records when there is no stateful evolution between operations.
- Pair modal procedures with explicit `uses` effects and `must`/`will` contracts so callers can reason about required capabilities.

### 2.3.3.2 Syntax

#### 2.3.3.2.1 Concrete Syntax

**Modal declaration:**

```ebnf
ModalDecl    ::= "modal" Ident GenericParams? "{" ModalItem* "}"
ModalItem    ::= StateDecl | CommonDecl | ModalProcedure
StateDecl    ::= "state" Ident "{" FieldList "}"
CommonDecl   ::= "common" "{" FieldList "}"
ModalProcedure ::= "procedure" Ident "@" StatePattern "(" ParamList ")" "->" "@" Ident ContractClauses? Block
StatePattern ::= Ident
               | "(" Ident ("|" Ident)* ")"    // Union of states
```

**State annotations:**

```ebnf
StateAnnot   ::= Type "@" Ident
```

**Examples:**

```cantrip
modal File {
    state Closed {
        path: String
    }

    state Open {
        path: String
        handle: FileHandle
        position: usize
    }

    procedure open@Closed() => @Open
        uses fs.read
    {
        let handle = FileSystem.open(self.path)?
        Open {
            path: self.path,
            handle: handle,
            position: 0,
        }
    }

    procedure close@Open() => @Closed
        uses fs.close
    {
        self.handle.close()
        Closed { path: self.path }
    }
}
```

#### 2.3.3.2.2 Abstract Syntax

**Modal types:**

```
τ ::= modal M { @S₁, ..., @Sₙ }    (modal type with states)
    | M@S                           (modal in specific state)
```

**State machine:**

```
M = (S, Σ, δ, s₀, Fields)
where:
  S = {@S₁, ..., @Sₙ}              (finite set of states)
  Σ = {m₁, ..., mₖ}                (procedures)
  δ : S × Σ → S                     (transition function)
  s₀ ∈ S                            (initial state)
  Fields: S → (Name × Type)*        (state-dependent fields)
```

**Transition syntax:**

```
δ ::= m@Sᵢ(...) => @Sⱼ { e }      (transition from Sᵢ to Sⱼ via m)
```

### 2.3.3.3 Static Semantics

#### 2.3.3.3.1 Well-Formedness Rules

**Modal declaration:**

```
[WF-Modal]
∀i. state @Sᵢ { fields } well-formed
∀j. procedure mⱼ@S_src => @S_tgt well-formed
@S_src, @S_tgt ∈ S
────────────────────────────────────────────
modal M { @S₁, ..., @Sₙ; m₁, ..., mₖ } well-formed
```

**State declaration:**

```
[WF-State]
Γ ⊢ T₁ : Type    ...    Γ ⊢ Tₙ : Type
field names distinct
────────────────────────────────────────
state @S { f₁: T₁; ...; fₙ: Tₙ } well-formed
```

**Modal procedure:**

```
[WF-Modal-Procedure]
modal M { ..., state @S₁, ..., state @S₂, ... }
Γ, self: M@S₁ ⊢ body : M@S₂
────────────────────────────────────────────────
procedure m@S₁(...) => @S₂ { body } well-formed
```

**Reachability:**

```
[WF-Reachability]
modal M = (S, Σ, δ, s₀, Fields)
∀ @S ∈ S. ∃ path from s₀ to @S in transition graph
────────────────────────────────────────────────
M satisfies reachability
```

**Determinism:**

```
[WF-Determinism]
∀ @S ∈ S, m ∈ Σ. |{@S' | δ(@S, m) = @S'}| ≤ 1
────────────────────────────────────────────────
M is deterministic
```

#### 2.3.3.3.2 Type Rules

**State-annotated type:**

```
[T-State-Type]
Γ ⊢ M : modal
@S ∈ states(M)
────────────────────────────────
Γ ⊢ M@S : Type
```

**Procedure invocation:**

```
[T-Modal-Procedure]
Γ ⊢ self : M@S₁
procedure m@S₁(...) => @S₂ in M
Γ ⊢ args : T
────────────────────────────────────────
Γ ⊢ self.m(args) : M@S₂
```

**State transition:**

```
[T-State-Transition]
Γ, Σ ⊢ e : M@S₁
Σ ⊢ (@S₁, m, @S₂) ∈ δ
Γ ⊢ args : T
────────────────────────────────────────
Γ, Σ ⊢ e.m(args) : M@S₂
```

**Invalid transition:**

```
[T-Invalid-Transition]
Γ ⊢ self : M@S₁
procedure m@S₂(...) => @S₃ in M
S₁ ≠ S₂
────────────────────────────────────────
ERROR E10020: Procedure m not available in state @S₁
```

**Field access (state-dependent):**

```
[T-Modal-Field]
Γ ⊢ self : M@S
field f : T in Fields(@S)
────────────────────────────────────────
Γ ⊢ self.f : T

[T-Modal-Field-Invalid]
Γ ⊢ self : M@S
field f ∉ Fields(@S)
────────────────────────────────────────
ERROR E10021: Field f not available in state @S
```

#### 2.3.3.3.3 State Invariants

**State invariant declaration:**

```
[T-State-Invariant]
state @S {
    fields...
    invariant P(fields)
}
────────────────────────────────────────
invariant(@S) = P
```

**Invariant preservation:**

```
[T-Invariant-Preservation]
procedure m@S₁(...) => @S₂
    must P
    will Q
P ∧ invariant(@S₁) ⟹ Q ∧ invariant(@S₂)
────────────────────────────────────────
Transition preserves invariants
```

**Example:**

```cantrip
modal Account {
    state Active {
        balance: i64
        invariant self.balance >= 0
    }

    state Frozen {
        balance: i64
        reason: String
        invariant self.balance >= 0
    }

    procedure withdraw@Active(amount: i64) => @Active
        must amount > 0
        must self.balance >= amount  // Preserves invariant
        will self.balance == old(self.balance) - amount
    {
        Active { balance: self.balance - amount }
    }
}
```

### 2.3.3.4 Dynamic Semantics

#### 2.3.3.4.1 State Transitions

**Transition evaluation:**

```
[E-Modal-Transition]
⟨self, σ⟩ ⇓ ⟨v_self : M@S₁, σ₁⟩
procedure m@S₁(...) => @S₂ in M
⟨body[self ↦ v_self], σ₁⟩ ⇓ ⟨v_result : M@S₂, σ₂⟩
────────────────────────────────────────────────────
⟨self.m(...), σ⟩ ⇓ ⟨v_result, σ₂⟩
```

**State construction:**

```
[E-State-Construction]
⟨e₁, σ⟩ ⇓ ⟨v₁, σ₁⟩    ...    ⟨eₙ, σₙ₋₁⟩ ⇓ ⟨vₙ, σₙ⟩
────────────────────────────────────────────────────
⟨@S { f₁: e₁, ..., fₙ: eₙ }, σ⟩ ⇓ ⟨M@S { f₁: v₁, ..., fₙ: vₙ }, σₙ⟩
```

#### 2.3.3.4.2 Linear State Transfer

**Linearity rule:**

Modal values use linear semantics—each value must be used exactly once:

```
[Linear-Use]
Γ ⊢ x : M@S
x used exactly once in scope
────────────────────────────────
Well-formed

[Linear-Use-Error]
Γ ⊢ x : M@S
x used zero or multiple times
────────────────────────────────
ERROR E10022: Linear value must be used exactly once
```

**Example:**

```cantrip
let file = File.new("data.txt")  // File@Closed
let file2 = file   // Moves ownership
// let x = file.open()  // ERROR E10022: file already moved

let conn = Connection.new("localhost", 8080)
// conn must be used exactly once
let conn = conn.connect()?
```

**Rebinding pattern:**

The common pattern is to rebind the same variable name:

```cantrip
let file = File.new("data.txt")  // File@Closed
let file = file.open()?           // Rebind: now File@Open
let data = file.read(1024)?       // Still File@Open (returns (data, file))
let file = file.close()           // Back to File@Closed
```

#### 2.3.3.4.3 Memory Layout

**Modal memory representation:**

A modal value consists of:

1. **Discriminant** (which state)
2. **Payload** (fields for that state)

```
Modal layout (similar to enum):
┌──────────────┬─────────────────────────┐
│ discriminant │  state payload          │
│  (which @S)  │  (max size of all states)│
└──────────────┴─────────────────────────┘
```

**Size calculation:**

```
size(modal M { @S₁, ..., @Sₙ }) =
    size(discriminant) + max(size(@S₁), ..., size(@Sₙ)) + padding

align(M) = max(align(discriminant), align(@S₁), ..., align(@Sₙ))
```

**Example:**

```cantrip
modal Example {
    state Small { x: u8; }           // 1 byte
    state Large { data: [u8; 100]; } // 100 bytes
}
// Total size ≈ 104 bytes (discriminant + largest state)
```

### 2.3.3.5 Verification

#### 2.3.3.5.1 Verification Conditions

**State reachability:**

Compiler verifies all states are reachable from initial state:

```
VC-Reachability:
∀ @S ∈ states(M). ∃ path s₀ →* @S
```

**Diagnostic:** `E10023` — State @S is unreachable from initial state

**Transition validity:**

All transitions must target valid states:

```
VC-Valid-Target:
∀ (procedure m@S₁ => @S₂). @S₁, @S₂ ∈ states(M)
```

**Diagnostic:** `E10024` — Transition targets non-existent state @S

#### 2.3.3.5.2 Static Checking

**Exhaustiveness checking:**

When matching on modal states, all states must be covered:

```cantrip
function describe(conn: Connection) => str {
    match conn {
        Connection@Disconnected => "disconnected",
        Connection@Connected => "connected",
        // Compiler will exhaustiveness
    }
}
```

**Type rule:**

```
[T-Modal-Match]
Γ ⊢ e : M
patterns cover all states in M
∀i. Γ, pᵢ ⊢ eᵢ : T
────────────────────────────────────────
Γ ⊢ match e { p₁ => e₁, ..., pₙ => eₙ } : T
```

#### 2.3.3.5.3 Runtime Checks

**Invariant checking:**

When enabled, runtime checks verify state invariants:

```cantrip
#[runtime_checks(true)]
modal SafeAccount {
    state Active {
        balance: i64
        invariant self.balance >= 0
    }

    procedure withdraw@Active(amount: i64) => @Active {
        Active { balance: self.balance - amount }
        // Runtime check: balance >= 0
    }
}
```

### 2.3.3.6 Advanced Features

#### 2.3.3.6.1 State Unions

**Syntax:**

Procedures can accept multiple source states using union syntax:

```cantrip
modal OrderProcessor {
    state Draft { items: Vec<Item>; }
    state Submitted { order_id: u64; items: Vec<Item>; }
    state Cancelled { order_id: u64; reason: String; }

    // Can cancel from either Draft or Submitted
    procedure cancel@(Draft | Submitted)(reason: String) => @Cancelled {
        let order_id = match self {
            Draft { .. } => generate_id(),
            Submitted { order_id, .. } => order_id,
        }
        Cancelled { order_id, reason }
    }
}
```

**Type rule:**

```
[T-State-Union]
procedure m@(@S₁ | ... | @Sₙ)(...) => @S
Γ ⊢ self : M@Sᵢ    where Sᵢ ∈ {@S₁, ..., @Sₙ}
────────────────────────────────────────────────
Γ ⊢ self.m(...) : M@S
```

#### 2.3.3.6.2 Common Fields

**Purpose:** Fields that exist in ALL states can be declared once in a `common` block.

**Syntax:**

```cantrip
modal HttpRequest {
    common {
        url: String
        headers: HashMap<String, String>
    }

    state Building { }

    state Sent {
        response_code: u16
    }

    state Completed {
        response_code: u16
        body: Vec<u8>
    }

    procedure send@Building() => @Sent
        uses net.http
    {
        let code = http_send(self.url, self.headers)?
        Sent { response_code: code }
    }
}
```

**Access:**

Common fields accessible in all states:

```cantrip
function log_url(req: HttpRequest@Building) {
    println(req.url)  // OK: url is common
}

function log_url2(req: HttpRequest@Sent) {
    println(req.url)  // OK: url is common
}
```

**Formal semantics:**

```
common fields ⊆ Fields(@S) for all @S ∈ states(M)
```

#### 2.3.3.6.3 Reachability Analysis

**Algorithm:**

```
reachability(M = (S, Σ, δ, s₀)):
1. Initialize reached = {s₀}
2. Initialize worklist = {s₀}
3. While worklist ≠ ∅:
     a. Remove state @S from worklist
     b. For each procedure m where (@S, m, @S') ∈ δ:
        i.  If @S' ∉ reached:
            - Add @S' to reached
            - Add @S' to worklist
4. Return unreachable = S \ reached
```

**Example:**

```cantrip
modal Protocol {
    state Init { }
    state Ready { }
    state Working { }
    state Done { }
    state Error { }       // Unreachable!
    state Timeout { }     // Unreachable!

    procedure start@Init() => @Ready
    procedure process@Ready() => @Working
    procedure finish@Working() => @Done
    // No transitions to Error or Timeout
}
```

**Diagnostic:** `W10025` — States Error and Timeout are unreachable

---

### 2.3.3.5 Proven Properties

- **Linear ownership.** Modal values obey linear usage: each permission instance `M@S` must be consumed exactly once when transitioning (§2.3.3.4.2).
- **Invariant preservation.** Transition contracts (`must`/`will`) combined with state invariants guarantee that invariants on `@S₁` hold for successor states `@S₂` (§2.3.3.3.3).
- **State reachability diagnostics.** The reachability analysis reports unreachable states (`W10025`), ensuring the declared modal machine is strongly connected from the initial state (§2.3.3.4.3).

---

### 2.3.5 Notes and Warnings

**Note 2.3.1:** Enums are safe tagged unions with compile-time exhaustiveness checking. Unions are unsafe untagged unions requiring manual tag management.

**Note 2.3.2:** Modals extend enums with explicit state machines and compile-time transition verification. Use modals when state transitions matter for correctness.

**Note 2.3.3:** Enum discriminants are automatically assigned unless explicitly specified with `= value` syntax. Discriminant values can be accessed via `as` cast for unit-only enums.

**Note 2.3.4:** Modals use linear state transfer - each procedure consumes the current state and produces the next state. State cannot be copied or duplicated.

**Performance Note 2.3.1:** Enum size is `size(discriminant) + max(variant sizes)` plus padding for alignment. The compiler uses the smallest discriminant type that can represent all variants.

**Performance Note 2.3.2:** Unions have no discriminant overhead - size is `max(field sizes)`. Use unions only when external tags track the active variant.

**Performance Note 2.3.3:** Modal state transitions compile to zero-cost abstractions. State checking and transition verification happen entirely at compile time.

**Warning 2.3.1:** Union field access without knowing the active variant causes undefined behavior. Always wrap unions in records with explicit tag fields.

**Warning 2.3.2:** Modals enforce linear state transfer. Attempting to use a modal value after a state transition causes a compile error.

**Warning 2.3.3:** Pattern matching on enums must be exhaustive. Missing variants cause compile errors unless a wildcard `_` pattern is provided.

**Warning 2.3.4:** Unions containing non-Copy types (String, Vec) require careful manual management. Dropping the wrong variant causes undefined behavior.

**Warning 2.3.5:** Modal state invariants are only as strong as the contracts you write. The compiler verifies transitions exist but cannot verify business logic correctness.

**Implementation Note 2.3.1:** The exhaustiveness checker uses constructor-based analysis to ensure all enum variants are covered in pattern matches.

**Implementation Note 2.3.2:** Modal reachability analysis ensures all states can be reached from the initial state. Unreachable states generate warnings.

**Implementation Note 2.3.3:** Union safety requires the `unsafe.union` effect to be declared. This makes unsafe boundaries explicit and auditable.

---

**Definition 2.4.1 (Arrays and Slices):** Arrays are fixed-size, contiguous sequences of homogeneous elements: `[T; n] = { (v₁, ..., vₙ) | ∀i ∈ [0, n). vᵢ : T }`. Slices are dynamically-sized views into contiguous data, implemented as dense pointers: `[T] = { (ptr, len) | ptr : *T, len : usize }`.

## 2.4 Arrays and Slices

### 2.4.0 Overview

#### 2.4.0.1 Scope

Section §2.4 summarises fixed-size arrays and dynamically-sized slices, with detailed coverage in §§2.4.1–2.4.6.

#### 2.4.0.2 Dependencies and Notation

Arrays and slices share metavariables with §2.0.4, depend on Part III for permission handling, and on Part VI for region-based allocation rules.

#### 2.4.0.3 Guarantees

Bounds checking MUST occur for slice indexing. Mutable slices MUST respect exclusive-borrow rules; immutable slices MUST be shareable but read-only.

### 2.4.0.4 Syntax Summary

Concrete grammar for array literals and slice notation resides in §§2.4.1–2.4.2. No additional derived forms are declared here.

### 2.4.0.5 Static Semantics Summary

Typing rules for array literals, slice coercions, and borrow semantics exist in §§2.4.1–2.4.3. This subsection introduces no new rules.

### 2.4.0.6 Dynamic Semantics Summary

Runtime checks for bounds and aliasing follow §§2.4.4–2.4.5.

### 2.4.0.7 Algorithms

Out-of-scope; coercion and bounds-checking algorithms are defined inline in the detailed subsections.

### 2.4.0.8 Proven Properties

Arrays and slices participate in the global safety theorems (§2.0.7); no unique metatheorems are added here.

### 2.4.0.9 Integration Notes

Mutable slices require exclusive permissions. Array-to-slice coercions preserve effects and rely on the `Sub-Array-Slice` rule (§2.0.6.1).

### 2.4.0.10 Notes and Issues

Mutable slice aliasing proofs remain open (ISSUE TS-2); example suites remain pending (ISSUE TS-5).

### 2.4.1 Overview

**Key innovation/purpose:** Arrays and slices provide memory-efficient, predictable data layout for homogeneous collections. Arrays are stack-allocated with compile-time known size, while slices enable dynamic views without heap allocation.

##### Guidance (non-normative)

- Use arrays for fixed-size, stack-friendly data; coerce to slices for APIs that accept variable lengths per §2.4.3.
- Borrow immutable `[T]` or mutable `[mut T]` views when sharing data without copying; observe the aliasing rules in §2.4.5.
- Prefer higher-level collections (e.g., `Vec<T>`) when sizes must grow dynamically.

**Relationship to other features:**

- **Memory layout**: Contiguous, predictable layout essential for FFI and performance
- **Type inference**: Array and slice types can be inferred from literals and context
- **Pattern matching**: Arrays support destructuring patterns
- **Generics**: `Vec<T>` and other collections internally use slices
- **Bounds checking**: Compiler inserts runtime checks for dynamic indices

### 2.4.2 Syntax

#### 2.4.2.1 Concrete Syntax

**Array Syntax:**

```ebnf
ArrayType    ::= "[" Type ";" IntLiteral "]"
ArrayLiteral ::= "[" Expr ("," Expr)* "]"
               | "[" Expr ";" IntLiteral "]"    // Repeat syntax
ArrayIndex   ::= Expr "[" Expr "]"
```

**Slice Syntax:**

```ebnf
SliceType  ::= "[" Type "]"
SliceRange ::= Expr "[" Range "]"
Range      ::= Expr? ".." Expr?        // start..end (exclusive)
             | Expr? "..=" Expr?       // start..=end (inclusive)
```

**Range semantics:**

```
arr[a..b]   = { arr[i] | a ≤ i < b }
arr[a..=b]  = { arr[i] | a ≤ i ≤ b }
arr[..b]    = arr[0..b]
arr[a..]    = arr[a..len(arr)]
arr[..]     = arr[0..len(arr)]
```

#### 2.4.2.2 Abstract Syntax

**Arrays:**

```
τ ::= [τ; n]         (array type)
e ::= [e₁, ..., eₙ]  (array literal)
    | [e; n]         (repeat literal)
    | e[e']          (array indexing)
```

**Slices:**

```
τ ::= [τ]            (slice type)
e ::= e[e₁..e₂]      (slice range)
    | e[e₁..=e₂]     (inclusive range)
    | e[..e₂]        (from start)
    | e[e₁..]        (to end)
    | e[..]          (full slice)
```

### 2.4.3 Static Semantics

#### 2.4.3.1 Well-Formedness Rules

**Array well-formedness:**

```
[WF-Array]
Γ ⊢ T : Type    n > 0
────────────────────────
Γ ⊢ [T; n] : Type

[WF-Array-Lit]
Γ ⊢ e₁ : T    ...    Γ ⊢ eₙ : T
──────────────────────────────────
Γ ⊢ [e₁, ..., eₙ] : [T; n]

[WF-Array-Repeat]
Γ ⊢ e : T
──────────────────────
Γ ⊢ [e; n] : [T; n]
```

**Slice well-formedness:**

```
[WF-Slice]
Γ ⊢ T : Type
────────────────
Γ ⊢ [T] : Type

[WF-Slice-Range]
Γ ⊢ arr : [T; n]    Γ ⊢ start : usize    Γ ⊢ end : usize
start ≤ end ≤ n
────────────────────────────────────────────────────────────
Γ ⊢ arr[start..end] : [T]

[WF-Slice-Range-Inclusive]
Γ ⊢ arr : [T; n]    Γ ⊢ start : usize    Γ ⊢ end : usize
start ≤ end < n
────────────────────────────────────────────────────────────
Γ ⊢ arr[start..=end] : [T]
```

#### 2.4.3.2 Type Rules

**Array type rules:**

```
[T-Array-Index]
Γ ⊢ arr : [T; n]
Γ ⊢ i : usize
i < n
────────────────────────────────────────────
Γ ⊢ arr[i] : T

[T-Array-Lit]
Γ ⊢ e₁ : T    ...    Γ ⊢ eₙ : T
──────────────────────────────────
Γ ⊢ [e₁, ..., eₙ] : [T; n]

[T-Array-Repeat]
Γ ⊢ e : T
──────────────────────
Γ ⊢ [e; n] : [T; n]
```

**Slice type rules:**

```
[T-Slice]
Γ ⊢ arr : [T; n]
Γ ⊢ start : usize
Γ ⊢ end : usize
start ≤ end ≤ n
────────────────────────────────────────────────────────────
Γ ⊢ arr[start..end] : [T]

[T-Slice-Inclusive]
Γ ⊢ arr : [T; n]
Γ ⊢ start : usize
Γ ⊢ end : usize
start ≤ end < n
────────────────────────────────────────────────────────────
Γ ⊢ arr[start..=end] : [T]

[T-Slice-Index]
Γ ⊢ slice : [T]
Γ ⊢ i : usize
────────────────────────────────────────────
Γ ⊢ slice[i] : T    (checked at runtime: i < len(slice))

[T-Slice-Full]
Γ ⊢ arr : [T; n]
────────────────────────────────────────────
Γ ⊢ arr[..] : [T]
```

**Array type inference:**

```
[Infer-Array-Homogeneous]
Γ ⊢ e₁ : T₁    ...    Γ ⊢ eₙ : Tₙ
T = unify(T₁, ..., Tₙ)
──────────────────────────────────
Γ ⊢ [e₁, ..., eₙ] : [T; n]
```

**Example:**

```cantrip
let arr = [1, 2, 3]        // Inferred: [i32; 3]
let mixed = [1, 2u64, 3]   // ERROR E2301: type mismatch in array literal: expected i32, found u64
```

**Contextual type inference:**

```cantrip
let arr: [u8; 3] = [1, 2, 3]    // Literals inferred as u8 from context
function process(data: [f32; 4]) { ... }
process([1.0, 2.0, 3.0, 4.0])   // Literals inferred as f32
```

**Bounds checking:**

_Static bounds checking (when possible):_

```
[T-Array-Index-Static]
Γ ⊢ arr : [T; n]
i is a compile-time constant
0 ≤ i < n
────────────────────────────────────────────
Γ ⊢ arr[i] : T    (no runtime check needed)
```

**Runtime bounds checking:**

```
[T-Array-Index-Dynamic]
Γ ⊢ arr : [T; n]
Γ ⊢ i : usize
i is not a compile-time constant
────────────────────────────────────────────
Γ ⊢ arr[i] : T    (runtime check: panic if i ≥ n)
```

**Example:**

```cantrip
let arr = [1, 2, 3, 4, 5]
let x = arr[2]           // Static: guaranteed in bounds
let i: usize = get_index()
let y = arr[i]           // Dynamic: runtime check inserted
```

#### 2.4.3.3 Type Properties

**Theorem 6.1 (Array Layout and Size):**

Arrays have a predictable, contiguous memory layout:

```
size([T; n]) = n × size(T)
align([T; n]) = align(T)
layout = contiguous memory, no padding between elements
offset(element i) = i × size(T)
```

**Theorem 6.2 (Slice Representation):**

Slices are dense pointers with fixed size independent of the viewed data:

```
size([T]) = size(*T) + size(usize) = 2 × word_size
align([T]) = word_size
layout = { ptr: *T, len: usize }  (dense pointer)
```

The slice itself is always 16 bytes on 64-bit systems (8 bytes pointer + 8 bytes length), regardless of the data it views.

**Theorem 6.3 (Copy Capability):**

```
[T; n] implements Copy ⟺ T implements Copy
[T] always implements Copy
```

**Semantics:**

- Arrays pass by permission by default (like all types)
- `.copy()` method performs element-wise bitwise copy for Copy-capable arrays
- Slices always implement Copy (copying the dense pointer descriptor, not underlying data)
- Slice `.copy()` duplicates the (ptr, len) pair, creating another view of same data

**Example:**

```cantrip
// Array parameter - pass by permission
procedure sum(arr: [i32; 5]): i32 {
    let mut total = 0
    for x in arr {  // Iterates through permission to array
        total += x
    }
    total
}

let numbers = [1, 2, 3, 4, 5]
let result = sum(numbers)  // numbers passed by permission
// numbers still usable here

// Explicit copy when needed
let copy_of_numbers = numbers.copy()  // Element-wise copy
```

### 2.4.4 Dynamic Semantics

#### 2.4.4.1 Evaluation Rules

**Array literal evaluation:**

```
[E-Array]
⟨e₁, σ⟩ ⇓ ⟨v₁, σ₁⟩    ...    ⟨eₙ, σₙ₋₁⟩ ⇓ ⟨vₙ, σₙ⟩
─────────────────────────────────────────────────────
⟨[e₁, ..., eₙ], σ⟩ ⇓ ⟨[v₁, ..., vₙ], σₙ⟩

[E-Array-Repeat]
⟨e, σ⟩ ⇓ ⟨v, σ'⟩
─────────────────────────────────────────────────────
⟨[e; n], σ⟩ ⇓ ⟨[v, v, ..., v], σ'⟩    (n copies of v)
```

**Array indexing evaluation:**

```
[E-Array-Index]
⟨arr, σ⟩ ⇓ ⟨[v₀, ..., vₙ₋₁], σ'⟩
⟨i, σ'⟩ ⇓ ⟨k, σ''⟩
k < n
─────────────────────────────────────
⟨arr[i], σ⟩ ⇓ ⟨vₖ, σ''⟩

[E-Array-Index-OOB]
⟨arr, σ⟩ ⇓ ⟨[v₀, ..., vₙ₋₁], σ'⟩
⟨i, σ'⟩ ⇓ ⟨k, σ''⟩
k ≥ n
─────────────────────────────────────
⟨arr[i], σ⟩ ⇓ panic("index out of bounds: {k} >= {n}")
```

**Slice creation evaluation:**

```
[E-Slice]
⟨arr, σ⟩ ⇓ ⟨[v₀, ..., vₙ₋₁], σ'⟩
⟨start, σ'⟩ ⇓ ⟨a, σ''⟩
⟨end, σ''⟩ ⇓ ⟨b, σ'''⟩
a ≤ b ≤ n
─────────────────────────────────────
⟨arr[start..end], σ⟩ ⇓ ⟨slice(&arr[a], b - a), σ'''⟩

[E-Slice-Inclusive]
⟨arr, σ⟩ ⇓ ⟨[v₀, ..., vₙ₋₁], σ'⟩
⟨start, σ'⟩ ⇓ ⟨a, σ''⟩
⟨end, σ''⟩ ⇓ ⟨b, σ'''⟩
a ≤ b < n
─────────────────────────────────────
⟨arr[start..=end], σ⟩ ⇓ ⟨slice(&arr[a], b - a + 1), σ'''⟩

[E-Slice-OOB]
⟨arr, σ⟩ ⇓ ⟨[v₀, ..., vₙ₋₁], σ'⟩
⟨start, σ'⟩ ⇓ ⟨a, σ''⟩
⟨end, σ''⟩ ⇓ ⟨b, σ'''⟩
a > b ∨ b > n
─────────────────────────────────────
⟨arr[start..end], σ⟩ ⇓ panic("slice index out of bounds")
```

**Slice indexing evaluation:**

```
[E-Slice-Index]
⟨slice, σ⟩ ⇓ ⟨(ptr, len), σ'⟩
⟨i, σ'⟩ ⇓ ⟨k, σ''⟩
k < len
─────────────────────────────────────
⟨slice[i], σ⟩ ⇓ ⟨*(ptr + k), σ''⟩
```

#### 2.4.4.2 Memory Representation

**Array layout:**

```
[T; n] memory layout:
┌────┬────┬────┬────┬────┐
│ T₀ │ T₁ │ T₂ │... │Tₙ₋₁│
└────┴────┴────┴────┴────┘
↑ aligned to align(T)
```

**Properties:**

- Contiguous memory
- No padding between elements
- Element i at offset: `i × size(T)`
- Total size: `n × size(T)`
- Alignment: `align(T)`

**Example:**

```cantrip
// [i32; 4] layout (assuming size(i32) = 4, align(i32) = 4)
// Offset:  0    4    8    12
//         ┌────┬────┬────┬────┐
//         │ v₀ │ v₁ │ v₂ │ v₃ │
//         └────┴────┴────┴────┘
// Total: 16 bytes, align: 4
```

**Slice layout:**

```
[T] memory layout (dense pointer):
┌─────────────┬─────────────┐
│ ptr: *T     │ len: usize  │
└─────────────┴─────────────┘
```

**Properties:**

- Size: `2 × word_size` (typically 16 bytes on 64-bit)
- Alignment: `word_size`
- Does NOT contain the actual data
- Points to data owned by array or other container

**Example:**

```cantrip
// On 64-bit system:
// [i32] layout:
//         ┌────────────────┬────────────────┐
//         │ ptr: 8 bytes   │ len: 8 bytes   │
//         └────────────────┴────────────────┘
// Total: 16 bytes, align: 8
```

#### 2.4.4.3 Operational Behavior

**Bounds checking behavior:**

Arrays and slices provide memory-safe access through compile-time and runtime bounds checking:

- **Static indices** (compile-time constants): When the compiler can prove an index is in bounds, no runtime overhead is incurred
- **Dynamic indices**: Runtime checks inserted, panic on out-of-bounds access with error message indicating the index and length
- **Performance**: Modern CPUs can often predict bounds check branches, minimizing performance impact

**Parameter Passing:**

- **All arrays** pass by permission (reference-like) regardless of Copy capability
- **Copy-capable arrays** can be explicitly copied with `.copy()` method
- **Non-Copy arrays** require explicit `move` for ownership transfer
- **Slices** always Copy-capable (dense pointer), pass by permission by default

**Example:**

```cantrip
let arr: [i32; 3] = [1, 2, 3]
procedure process(a: [i32; 3]) { ... }
process(arr)  // Pass by permission, arr still usable

let explicit_copy = arr.copy()  // Explicit copy if needed
```

**Performance characteristics:**

```
Array access:        O(1) with potential bounds check (often elided)
Slice creation:      O(1) (pointer arithmetic only)
Array construction:  O(n) (initialize all elements)
Sequential access:   Cache-friendly due to contiguous layout
```

**Example - bounds check elimination:**

```cantrip
// Compiler can prove i < 5 for all iterations
for i in 0..5 {
    arr[i] = i  // Bounds check eliminated
}
```

### 2.4.5 Additional Properties

#### 2.4.5.1 Parameter Passing and Copying

**All arrays and slices pass by permission:**

```cantrip
// Small array - passes by permission
let arr1: [i32; 3] = [1, 2, 3]
procedure process(a: [i32; 3]) { ... }
process(arr1)  // Permission, not copy
// arr1 still usable

// Explicit copy when needed
let arr2 = arr1.copy()  // Element-wise copy

// Large array - passes by permission (efficient!)
let big: [i32; 1000] = initialize_big_array()
process_big(big)  // Permission only, zero cost
// No expensive copy unless explicitly requested

// Non-Copy array - passes by permission or moves
let strings: [String; 3] = [String.new("a"), String.new("b"), String.new("c")]
process_strings(strings)  // Permission
consume_strings(move strings)  // Explicit move
// strings no longer usable
```

**Slices - always Copy-capable:**

```cantrip
let arr = [1, 2, 3, 4, 5]
let slice: [i32] = arr[..]

// Slices pass by permission like everything else
procedure process(s: [i32]) { ... }
process(slice)  // Permission to dense pointer

// Explicit copy duplicates dense pointer (cheap)
let slice2 = slice.copy()  // Copies (ptr, len), NOT data
// Both slice and slice2 view same data

// Data copying requires explicit element iteration
let mut data_copy = Vec.new()
for x in slice {
    data_copy.push(x.copy())
}
```

#### 2.4.5.2 Subtyping

Arrays and slices are **invariant** in their type parameter:

```
[T; n] is NOT a subtype of [U; n] even if T <: U
[T] is NOT a subtype of [U] even if T <: U
```

**Rationale:** Allowing covariance would violate type safety in mutable contexts.

**Example:**

```cantrip
// This would be unsound if allowed:
let arr: [own String; 3] = [...]
let arr2: [mut String; 3] = arr  // ERROR: not a subtype
```

#### 2.4.5.3 Array-to-Slice Coercion

Arrays can be implicitly coerced to slices in certain contexts:

```
[T-Array-Coercion]
Γ ⊢ arr : [T; n]
────────────────────────────────────────────
Γ ⊢ arr : [T]    (implicit coercion)
```

**Example:**

```cantrip
function process(data: [i32]) {
    // ...
}

let arr: [i32; 5] = [1, 2, 3, 4, 5]
process(arr)  // OK: [i32; 5] coerces to [i32]
```

### 2.4.5 Mutable Slices

**Definition 2.4.2 (Mutable Slice):** A mutable slice `[mut T]` is a dynamically-sized view into contiguous mutable data, allowing in-place modification of elements. Like immutable slices, mutable slices are dense pointers `(ptr: *mut T, len: usize)`, but require `mut` permission to create and use.

#### 2.4.5.1 Syntax

**Grammar:**

```ebnf
MutSliceType ::= "[" "mut" Type "]"
MutSliceRange ::= MutExpr "[" Range "]"
MutSliceIndex ::= MutExpr "[" Expr "]"
```

**Examples:**

```cantrip
// Mutable slice types
let mut arr = [1, 2, 3, 4, 5]
let slice: [mut i32] = arr[1..4]  // Mutable view

// Mutable indexing
slice[0] = 10  // Modify through slice

// Mutable slice from mutable array
var numbers = [0; 100]
let view: [mut i32] = numbers[10..20]
```

#### 2.4.5.2 Type Rules

**[T-MutSlice] Mutable Slice Creation:**

```
[T-MutSlice]
Γ ⊢ arr : mut [T; n]    (mutable array)
Γ ⊢ start : usize
Γ ⊢ end : usize
start ≤ end ≤ n
────────────────────────────────────────────────────────────
Γ ⊢ arr[start..end] : [mut T]
```

**Explanation:** Only mutable arrays can produce mutable slices. The `mut` permission is required.

```cantrip
let arr = [1, 2, 3, 4, 5]           // Immutable array
// let s: [mut i32] = arr[1..3]     // ✗ ERROR: cannot create mutable slice from immutable array

var mut_arr = [1, 2, 3, 4, 5]       // Mutable array
let s: [mut i32] = mut_arr[1..3]   // ✓ Mutable slice
```

**[T-MutSliceIndex] Mutable Slice Indexing:**

```
[T-MutSliceIndex]
Γ ⊢ slice : [mut T]
Γ ⊢ i : usize
────────────────────────────────────────────
Γ ⊢ slice[i] : mut T    (checked at runtime: i < len(slice))
```

**Explanation:** Indexing a mutable slice yields a mutable reference to the element.

```cantrip
var arr = [10, 20, 30]
let slice: [mut i32] = arr[..]

// Read element
let value = slice[1]  // 20

// Modify element
slice[1] = 25         // ✓ Mutable access
assert(slice[1] == 25)
```

**[T-MutSliceAssign] Mutable Slice Assignment:**

```
[T-MutSliceAssign]
Γ ⊢ slice : [mut T]
Γ ⊢ i : usize
Γ ⊢ v : T
────────────────────────────────────────────
Γ ⊢ (slice[i] = v) : ()    (modifies slice[i])
```

**Explanation:** Elements can be assigned through mutable slices.

```cantrip
var data = [1, 2, 3, 4, 5]
let slice: [mut i32] = data[1..4]

slice[0] = 100  // Modifies data[1]
slice[1] = 200  // Modifies data[2]
slice[2] = 300  // Modifies data[3]

assert(data == [1, 100, 200, 300, 5])
```

**[T-MutSliceIter] Mutable Slice Iteration:**

```
[T-MutSliceIter]
Γ ⊢ slice : [mut T]
────────────────────────────────────────────
Γ ⊢ slice.iter_mut() : Iterator<Item = mut T>
```

**Explanation:** Mutable slices provide mutable iterators for in-place modification.

```cantrip
var numbers = [1, 2, 3, 4, 5]
let slice: [mut i32] = numbers[..]

// Mutable iteration
for x in slice.iter_mut() {
    *x *= 2  // Double each element in place
}

assert(numbers == [2, 4, 6, 8, 10])
```

**[T-SlicePermission] Permission Requirements:**

```
[T-SlicePermission]
Γ ⊢ arr : π [T; n]
────────────────────────────────────────────
Γ ⊢ arr[start..end] : [π T]

where π ∈ {own, mut, imm}
```

**Explanation:** Slice permission matches the permission of the source array.

```cantrip
// Immutable array → immutable slice
let arr: imm [i32; 5] = [1, 2, 3, 4, 5]
let slice: [imm i32] = arr[1..3]  // ✓ Immutable slice

// Mutable array → mutable slice
var mut_arr: mut [i32; 5] = [1, 2, 3, 4, 5]
let mut_slice: [mut i32] = mut_arr[1..3]  // ✓ Mutable slice

// Cannot create mutable slice from immutable array
// let bad: [mut i32] = arr[1..3]  // ✗ ERROR: permission mismatch
```

**[T-MutSliceSplit] Mutable Slice Splitting:**

```
[T-MutSliceSplit]
Γ ⊢ slice : [mut T]
Γ ⊢ mid : usize
────────────────────────────────────────────
Γ ⊢ slice.split_at_mut(mid) : ([mut T], [mut T])
```

**Explanation:** Mutable slices can be split into non-overlapping mutable sub-slices.

```cantrip
var arr = [1, 2, 3, 4, 5, 6]
let slice: [mut i32] = arr[..]

let (left, right) = slice.split_at_mut(3)
// left: [mut i32] = [1, 2, 3]
// right: [mut i32] = [4, 5, 6]

left[0] = 10
right[0] = 40

assert(arr == [10, 2, 3, 40, 5, 6])
```

**[T-MutSliceNonOverlap] Non-Overlapping Guarantee:**

```
[T-MutSliceNonOverlap]
Γ ⊢ arr : mut [T; n]
slice₁ = arr[a₁..b₁]
slice₂ = arr[a₂..b₂]
(a₁..b₁) ∩ (a₂..b₂) = ∅    (disjoint ranges)
────────────────────────────────────────────
slice₁ and slice₂ can coexist as [mut T]
```

**Explanation:** Multiple mutable slices of the same array are allowed only if they don't overlap.

```cantrip
var arr = [1, 2, 3, 4, 5, 6]

// ✓ Non-overlapping mutable slices
let slice1: [mut i32] = arr[0..3]
let slice2: [mut i32] = arr[3..6]
slice1[0] = 10
slice2[0] = 40

// ✗ ERROR: Overlapping mutable slices not allowed
// let bad1: [mut i32] = arr[0..4]
// let bad2: [mut i32] = arr[2..6]  // Overlaps with bad1
```

**[T-MutSliceCoercion] Array to Mutable Slice Coercion:**

```
[T-MutSliceCoercion]
Γ ⊢ arr : mut [T; n]
────────────────────────────────────────────
Γ ⊢ arr : [mut T]    (coerces to mutable slice)
```

**Explanation:** Mutable arrays automatically coerce to mutable slices in function calls.

```cantrip
function double_elements(data: [mut i32]) {
    for x in data.iter_mut() {
        *x *= 2
    }
}

var numbers = [1, 2, 3, 4, 5]
double_elements(numbers)  // ✓ Coerces [i32; 5] to [mut i32]
assert(numbers == [2, 4, 6, 8, 10])
```

#### 2.4.5.3 Memory Representation

Mutable slices have the same memory representation as immutable slices:

```
[mut T] = (ptr: *mut T, len: usize)

Size: 2 × usize (16 bytes on 64-bit)
Alignment: align(usize) = 8 bytes on 64-bit
```

**Key difference:** The pointer type is `*mut T` (mutable raw pointer) instead of `*const T`.

**Memory layout:**

```
Mutable Slice [mut T]:
┌─────────────────┬─────────────────┐
│  ptr: *mut T    │  len: usize     │
│  (8 bytes)      │  (8 bytes)      │
└─────────────────┴─────────────────┘
       ↓
       Points to mutable memory region:
       [T₀, T₁, T₂, ..., Tₙ₋₁]
```

#### 2.4.5.4 Operational Behavior

**In-place modification:**

```cantrip
var data = [1, 2, 3, 4, 5]
let slice: [mut i32] = data[1..4]

// Modify slice element
slice[1] = 100

// Original array is modified
assert(data == [1, 2, 100, 4, 5])
```

**Mutable vs. Immutable slices:**

| Feature          | Immutable Slice `[T]`             | Mutable Slice `[mut T]`                      |
| ---------------- | --------------------------------- | -------------------------------------------- |
| **Pointer type** | `*const T`                        | `*mut T`                                     |
| **Permission**   | `imm` or `own`                    | `mut` required                               |
| **Modification** | Read-only                         | Can modify elements                          |
| **Aliasing**     | Multiple immutable slices allowed | Only one mutable slice or multiple immutable |
| **Copy trait**   | ✓ Implements Copy                 | ✓ Implements Copy                            |
| **Size**         | 16 bytes (64-bit)                 | 16 bytes (64-bit)                            |

**Permission rules:**

```cantrip
var arr = [1, 2, 3, 4, 5]

// ✓ Multiple immutable slices allowed
let s1: [i32] = arr[0..2]
let s2: [i32] = arr[2..5]

// ✓ One mutable slice allowed
let ms: [mut i32] = arr[..]

// ✗ Cannot have mutable and immutable slices simultaneously
// let bad_imm: [i32] = arr[0..2]      // ✗ ERROR: already borrowed mutably
// let bad_mut: [mut i32] = arr[3..5]  // ✗ ERROR: already borrowed mutably
```

#### 2.4.5.5 Type Properties

**Theorem 6.1 (Mutable Slice Safety):** Mutable slices preserve memory safety through permission checking. At most one mutable slice to overlapping memory can exist at any time.

**Proof sketch:** The type system enforces that mutable slices require `mut` permission, and the permission system guarantees exclusive access to mutable data. ∎

**Corollary 6.1:** No data races can occur through mutable slices.

**Theorem 6.2 (Non-Overlapping Split):** Split operations like `split_at_mut` produce non-overlapping slices that can be safely used simultaneously.

**Proof sketch:** The split operation divides the slice into disjoint ranges `[0..mid)` and `[mid..len)`. Since these ranges don't overlap, multiple mutable accesses are safe. ∎

---

### 2.4.7 Notes and Warnings

**Note 2.4.1:** Array sizes must be compile-time constants. For runtime-sized collections, use Vec<T> from the standard library.

**Note 2.4.2:** Slices are views, not owned data. They cannot outlive the data they reference. The region system enforces this automatically.

**Performance Note 2.4.1:** Array indexing performs bounds checking by default. For performance-critical loops where bounds are statically known, the compiler can eliminate checks through loop analysis.

**Performance Note 2.4.2:** Slices have zero overhead compared to raw pointers with manual length tracking. The dense pointer representation (ptr + len) is the same size as separate variables.

**Warning 2.4.1:** Creating a slice from an array does NOT copy data - it creates a view. Modifying the array affects the slice and vice versa (for mutable slices).

**Implementation Note 2.4.1:** Array-to-slice coercion is a zero-cost conversion. No code is generated; the compiler simply adjusts the type.

---

---

**Definition 2.5.1 (Pointers):** Cantrip provides two pointer systems: (1) Safe pointers `Ptr<T>@S` using modal states and effect exclusion for compile-time aliasing safety without runtime overhead, and (2) raw pointers `*T` and `*mut T` for unsafe manual memory management and FFI interoperability. Safe pointers prevent aliasing bugs through the axiom ∀ T. ¬(read<T> ∧ write<T>), while raw pointers provide no safety guarantees and require `unsafe { }` blocks with the `unsafe.ptr` effect.

---

## 2.5 Pointers

### 2.5.0 Overview

#### 2.5.0.1 Scope

Section §2.5 summarises safe pointers (`Ptr<T>@State`) and raw pointers (`*T`, `*mut T`) detailed in §§2.5.1–2.5.7.

#### 2.5.0.2 Dependencies and Notation

Pointer semantics depend on Part III (Permissions) for alias control, Part IV (Effects) for unsafe capabilities, and Part VI (Regions) for allocation lifetimes.

#### 2.5.0.3 Guarantees

Safe pointers MUST respect modal-state transitions and permissions. Raw pointers MUST only be dereferenced inside `unsafe` blocks with declared `unsafe.ptr`/`unsafe.union` effects.

### 2.5.0.4 Syntax Summary

Pointer grammar is defined in §§2.5.1–2.5.2. No additional sugar is introduced here.

### 2.5.0.5 Static Semantics Summary

Typing rules for pointer creation, borrowing, and comparison appear in §§2.5.3–2.5.5.

### 2.5.0.6 Dynamic Semantics Summary

Runtime obligations (null checks, modal transitions) are described in §§2.5.6–2.5.7.

### 2.5.0.7 Algorithms

Out-of-scope; pointer verification algorithms align with modal reachability in §2.3.

### 2.5.0.8 Proven Properties

Safe pointer operations contribute to the global safety theorems (§2.0.7). No additional proofs are introduced here.

### 2.5.0.9 Integration Notes

Pointer effects MUST be declared (`unsafe.ptr`, `unsafe.union`). Region escape analysis from Part VI prevents pointer misuse.

### 2.5.0.10 Notes and Issues

Example suites are pending (ISSUE TS-5).

### 2.5.1 Overview

#### 2.5.1.1 Pointer Types in Cantrip

Cantrip provides two complementary pointer systems, each designed for specific use cases:

**Safe Pointers (`Ptr<T>@State`):**

- Modal state machines (@Exclusive, @Shared, @Frozen)
- Effect-based aliasing prevention (read<T> ⊥ write<T>)
- Compile-time safety guarantees
- Zero runtime overhead
- For application code, safe data structures

**Raw Pointers (`*T`, `*mut T`):**

- Direct memory addresses (machine words)
- No safety guarantees
- Requires `unsafe { }` blocks and `unsafe.ptr` effect
- Nullable, supports pointer arithmetic
- For FFI, manual memory management, unsafe code

**Key innovation:** Safe pointers use modal types + effect exclusion to achieve memory safety through compile-time state tracking and aliasing prevention, while raw pointers provide an explicit escape hatch for unsafe operations.

#### 2.5.1.2 When to Use Each Type

##### Guidance (non-normative)

Safe pointer families (`Ptr<T>@State`) from §2.5.1.1 SHOULD remain the default in application code because the guarantees enumerated in §2.5.1.3 compose with the cross-feature integrations catalogued in §2.5.1.4. Escalate to raw pointers (`*T`, `*mut T`) only inside `unsafe.ptr` effect contexts—FFI shims, allocator internals, or other intentionally unchecked boundaries—where those guarantees are knowingly suspended; outside those contexts the modal transitions and permission hooks already cover ownership transfer and temporary views.

#### 2.5.1.3 Safety Model

**Safe by default:**

```cantrip
// Normal code uses safe pointers
let ptr: Ptr<i32>@Exclusive = Ptr.new(&x)
let value = ptr.read()  // Compile-time safety checks
```

**Unsafe escape hatch:**

```cantrip
// Unsafe code explicitly marked
function use_raw_ptr(raw: *i32): i32
    uses unsafe.ptr  // Effect declaration required
{
    unsafe {  // Explicit unsafe block
        *raw  // Dereference only allowed here
    }
}
```

**Effect declarations:**

- Safe pointers: `read<T>`, `write<T>` (type-specific)
- Raw pointers: `unsafe.ptr` (blanket unsafe permission)
- FFI calls: `ffi.call` + `unsafe.ptr`

#### 2.5.1.4 Relationship to Other Features

**Cross-references:**

- **§1 (Mathematical Foundations):** Notation used throughout this section:

  - Type judgments: `Γ ⊢ e : T`
  - Effect judgments: `Γ ⊢ e ! ε`
  - Evaluation: `⟨e, σ⟩ ⇓ ⟨v, σ'⟩`
  - Substitution: `e[x ↦ v]`
  - See §1.2 for complete notation reference

- **§8 (Modals):** Safe pointers ARE modal types

  - `Ptr<T>@Exclusive`, `Ptr<T>@Shared`, `Ptr<T>@Frozen` are modal states
  - State transitions follow modal transition rules (§8.3.2)
  - Compile-time state verification (§8.5)

- **Part III (Permission System):** Pointers compose with permissions

  - `own Ptr<T>@Exclusive` — owned pointer (movable)
  - `mut Ptr<T>@Exclusive` — mutable reference to pointer
  - `Ptr<T>@Exclusive` — immutable reference to pointer (default)
  - Raw pointers: `*T` (immutable), `*mut T` (mutable)
  - See §26 for permission semantics

- **Part V (Contract System):**

  - Effect tracking for pointer operations:
    - `read<T>` — reading through pointer
    - `write<T>` — writing through pointer
    - `unsafe.ptr` — raw pointer operations
  - Effect exclusion axiom: `∀ T. ¬(read<T> ∧ write<T>)` (Definition 2.5.4)
  - Preconditions for validity: `must valid(ptr)`
  - See §21 for effect system details

- **Part VI (Memory Regions):**

  - Safe pointers integrate with region escape analysis
  - `Ptr.new_in<r>(&x)` binds pointer to region r
  - Compiler prevents region escape (§29.3)
  - Raw pointers bypass escape analysis in `unsafe { }` blocks

- **§17 (Contracts):**

  - Safe pointer validity enforced via contracts
  - Raw pointer validity is programmer responsibility
  - Pointer preconditions: `must ptr != null<T>()`

- **§24 (Unsafe):**

  - Raw pointer dereferencing must `unsafe { }` blocks
  - `unsafe.ptr` effect must be declared
  - Unsafe boundary for raw operations

- **§56-62 (FFI):**
  - Raw pointers provide C interoperability
  - Type mappings: `*T` ↔ `const T*`, `*mut T` ↔ `T*`
  - `null<T>()` ↔ `NULL`

---

### 2.5.2 Syntax

**Key innovation/purpose:** Unified syntax covering both safe and raw pointers, demonstrating the complementary nature of Cantrip's two-tier pointer system.

#### 2.5.2.1 Concrete Syntax

**EBNF Grammar:**

```ebnf
(* ========== Type Syntax ========== *)

PointerType ::= SafePointerType | RawPointerType

SafePointerType ::= "Ptr" "<" Type ">" "@" PointerState

PointerState ::= "Exclusive" | "Shared" | "Frozen"

RawPointerType ::= "*" Type                    (* Immutable raw pointer *)
                 | "*" "mut" Type              (* Mutable raw pointer *)

(* ========== Expression Syntax ========== *)

PointerExpr ::= SafePointerExpr | RawPointerExpr | ConversionExpr

SafePointerExpr ::= SafePointerCreation
                  | SafePointerOperation

SafePointerCreation ::= "Ptr" "." "new" "(" Expr ")"
                      | "Ptr" "." "new_in" "<" Ident ">" "(" Expr ")"

SafePointerOperation ::= Expr "." "share" "(" ")"
                       | Expr "." "freeze" "(" ")"
                       | Expr "." "read" "(" ")"
                       | Expr "." "write" "(" Expr ")"
                       | Expr "." "clone" "(" ")"
                       | Expr "." "as_raw" "(" ")"

RawPointerExpr ::= RawPointerCreation
                 | RawPointerOperation

RawPointerCreation ::= "&raw" Expr
                     | "&raw" "mut" Expr
                     | "null" "<" Type ">" "(" ")"

RawPointerOperation ::= "*" Expr                           (* Dereference *)
                      | Expr "." "offset" "(" Expr ")"
                      | Expr "." "add" "(" Expr ")"
                      | Expr "." "sub" "(" Expr ")"
                      | Expr "." "cast" "<" Type ">" "(" ")"

ConversionExpr ::= "Ptr" "." "from_raw" "(" Expr ")"

(* ========== Effect Syntax ========== *)

PointerEffect ::= "read" "<" Type ">"          (* Type-specific read *)
                | "write" "<" Type ">"         (* Type-specific write *)
                | "unsafe.ptr"                 (* Raw pointer operations *)
```

**Explanation:**

- **Safe pointer types** are parameterized by pointee type `T` and state `@S`
- **Raw pointer types** distinguish immutable (`*T`) and mutable (`*mut T`)
- **Safe operations** (`read`, `write`, `share`, `freeze`) are type-safe
- **Raw operations** (`*ptr`, `offset`, `cast`) require `unsafe { }` blocks
- **Effect annotations** are type-specific for safe pointers, blanket for raw

#### 2.5.2.2 Abstract Syntax

**Formal representation:**

```
(* ========== Types ========== *)

τ, σ, ρ ::= ...                              (types from previous sections)
          | Ptr<τ>@S                         (safe pointer in state S)
          | *τ                               (immutable raw pointer)
          | *mut τ                           (mutable raw pointer)

S ::= Exclusive                              (unique access)
    | Shared                                 (multiple read-only aliases)
    | Frozen                                 (permanently immutable)

(* ========== Expressions ========== *)

e ::= ...                                    (expressions from previous sections)
    (* Safe pointer operations *)
    | Ptr.new(e)                            (heap allocation)
    | Ptr.new_in<r>(e)                      (region allocation)
    | e.share()                             (create aliases)
    | e.freeze()                            (freeze forever)
    | e.read()                              (read value)
    | e.write(e')                           (write value)
    | e.clone()                             (clone pointer)
    | e.as_raw()                            (convert to raw)

    (* Raw pointer operations *)
    | &raw e                                (take raw address - immutable)
    | &raw mut e                            (take raw address - mutable)
    | *e                                    (dereference - UNSAFE)
    | e.offset(n)                           (pointer arithmetic - UNSAFE)
    | e.add(n)                              (pointer addition - UNSAFE)
    | e.sub(n)                              (pointer subtraction - UNSAFE)
    | e.cast<τ>()                           (type casting - UNSAFE)
    | null<τ>()                             (null pointer)

    (* Conversions *)
    | Ptr.from_raw(e)                       (raw to safe - UNSAFE)

(* ========== Values ========== *)

v ::= ...                                    (values from previous sections)
    | Ptr@S { addr: ℓ, region: r }          (safe pointer value - internal representation)
    | *τ { addr: ℓ }                        (raw pointer value)
    | *mut τ { addr: ℓ }                    (mutable raw pointer value)

(* ========== Effects ========== *)

ε ::= ...                                    (effects from previous sections)
    | read<τ>                               (reading type τ)
    | write<τ>                              (writing type τ)
    | unsafe.ptr                            (raw pointer operations)
    | alloc.heap                            (heap allocation)
    | alloc.region                          (region allocation)

(* ========== Effect Exclusion Axiom ========== *)

Axiom (Effect Exclusion):
  ∀ τ ∈ Type. ¬(read<τ> ∧ write<τ>)

  Reading: "For all types τ, read and write effects on τ cannot coexist."
```

**Components:**

**Safe Pointer Internal Representation:**

- `addr: ℓ` — raw memory address (pointer-sized)
- `region: r` — region binding (compile-time only, not stored)
- `state: S` — modal state (@Exclusive, @Shared, @Frozen) (compile-time only)

**Raw Pointer Representation:**

- `addr: ℓ` — raw memory address (pointer-sized)
- No state tracking, no safety guarantees

**Key Observations:**

- Safe pointers have richer type information but identical runtime representation
- Modal states compile away — zero runtime overhead
- Effect annotations enable compile-time safety checking
- Raw pointers provide escape hatch when safety checks not needed

---

### 2.5.3 Static Semantics

#### 2.5.3.1 Well-Formedness Rules

**Definition 2.5.2 (Pointer Well-Formedness):** A pointer type is well-formed if its pointee type is well-formed and, for safe pointers, the state annotation is valid.

**Safe Pointer Types:**

```
[WF-Ptr-Type]
Γ ⊢ T : Type
S ∈ {Exclusive, Shared, Frozen}
────────────────────────────────────
Γ ⊢ Ptr<T>@S : Type

[WF-Ptr-State-Exclusive]
Γ ⊢ Ptr<T>@Exclusive : Type
────────────────────────────────────
internal_fields: { addr: *T, region: RegionId }
(region binding tracked by compiler, not stored)

[WF-Ptr-State-Shared]
Γ ⊢ Ptr<T>@Shared : Type
────────────────────────────────────
internal_fields: { addr: *T, region: RegionId, count: StaticCount }
(count is static analysis only, not runtime)

[WF-Ptr-State-Frozen]
Γ ⊢ Ptr<T>@Frozen : Type
────────────────────────────────────
internal_fields: { addr: *T, region: RegionId }
```

**Raw Pointer Types:**

```
[WF-RawPtr-Immut]
Γ ⊢ T : Type
────────────────────────────────────
Γ ⊢ *T : Type

[WF-RawPtr-Mut]
Γ ⊢ T : Type
────────────────────────────────────
Γ ⊢ *mut T : Type

[WF-Null-Ptr]
Γ ⊢ T : Type
────────────────────────────────────
Γ ⊢ null<T>() : *T
(raw pointers can be null)
```

**Raw Pointer Operations (NEW - P1):**

```
[WF-RawPtr-Deref-Context]
Γ ⊢ ptr : *T
unsafe_context(Γ)
unsafe.ptr ∈ available_effects(Γ)
────────────────────────────────────
Γ ⊢ *ptr well-formed

[WF-RawPtr-Arithmetic-Context]
Γ ⊢ ptr : *T
Γ ⊢ offset : isize | usize
unsafe_context(Γ)
unsafe.ptr ∈ available_effects(Γ)
────────────────────────────────────
Γ ⊢ ptr.offset(offset) well-formed
Γ ⊢ ptr.add(offset) well-formed
Γ ⊢ ptr.sub(offset) well-formed

[WF-RawPtr-Cast-Context]
Γ ⊢ ptr : *T
Γ ⊢ U : Type
unsafe_context(Γ)
unsafe.ptr ∈ available_effects(Γ)
────────────────────────────────────
Γ ⊢ ptr.cast<U>() well-formed

[WF-Ptr-Effect-Context]
Γ ⊢ T : Type
ε ∈ {read<T>, write<T>}
ε ∈ available_effects(Γ)
────────────────────────────────────
effect context ε well-formed for T
```

**Definition 2.5.3 (Pointer Provenance):** Provenance tracks the origin and valid range of pointers for soundness. All pointers carry provenance metadata (conceptual, not stored at runtime).

**Provenance rules:**

1. Pointers derived from an allocation have provenance of that allocation
2. Pointer arithmetic preserves provenance
3. Casting does not change provenance
4. Accessing outside provenance bounds is undefined behavior

**Definition 2.5.4 (Effect Exclusion):** For any type T, the read and write effects on T are mutually exclusive:

```
Axiom (Effect Exclusion):
∀ T ∈ Type. ¬(read<T> ∧ write<T>)

Formally:
  If read<T> ∈ active_effects(Γ), then write<T> ∉ active_effects(Γ)
  If write<T> ∈ active_effects(Γ), then read<T> ∉ active_effects(Γ)

This axiom prevents aliasing bugs by making simultaneous reading and writing
to the same type impossible at compile time.
```

**Definition 2.5.5 (Null Pointer):** A null pointer is a pointer with address 0x0 that does not reference any valid memory location. Dereferencing a null pointer causes undefined behavior.

```
Formal definition:
  null<T>() = *T { addr: 0x0 }

Properties:
  ∀ T. null<T>() : *T
  ∀ T, ptr: *T. (ptr == null<T>()) ⟺ (ptr.addr == 0x0)
  ⟨*null<T>(), σ⟩ ⇒ UNDEFINED BEHAVIOR
```

**Definition 2.5.6 (Pointer Validity):** A pointer ptr is valid to dereference if and only if:

```
valid(ptr) ⟺
  ptr.addr ≠ 0x0                              ∧    (not null)
  ptr.addr % alignof(T) == 0                  ∧    (properly aligned)
  ptr.addr ∈ allocated_memory(σ)              ∧    (points to allocated memory)
  ¬freed(ptr.addr, σ)                         ∧    (not freed)
  ptr.addr ∈ provenance_bounds(ptr)                (within provenance)

Where:
  allocated_memory(σ) = set of currently allocated memory addresses
  freed(addr, σ) = true if address was allocated but has been freed
  provenance_bounds(ptr) = valid address range for pointer's provenance
```

#### 2.5.3.2 Type Rules

This section presents all typing rules for pointer operations, organized by category.

##### Safe Pointer Operations

**[T-Ptr-New-Heap] Heap pointer creation:**

```
[T-Ptr-New-Heap]
Γ ⊢ value : T
¬in_region_context(Γ)
────────────────────────────────────────────────────
Γ ⊢ Ptr.new(&value) : Ptr<T>@Exclusive ! alloc.heap
(region = heap)
```

**[T-Ptr-New-Region] Region pointer creation:**

```
[T-Ptr-New-Region]
Γ ⊢ value : T
region r ∈ Γ
value allocated in region r
────────────────────────────────────────────────────
Γ ⊢ Ptr.new_in<r>(&value) : Ptr<T>@Exclusive@r
(internally tagged as belonging to region r)
```

**[T-Ptr-Share] Sharing transition:**

```
[T-Ptr-Share]
Γ ⊢ ptr : Ptr<T>@Exclusive
────────────────────────────────────────────────────
Γ ⊢ ptr.share() : (Ptr<T>@Shared, Ptr<T>@Shared)
(consumes Exclusive, produces two Shared)
```

**[T-Ptr-Freeze] Freezing transition:**

```
[T-Ptr-Freeze]
Γ ⊢ ptr : Ptr<T>@Exclusive
────────────────────────────────────────────────────
Γ ⊢ ptr.freeze() : Ptr<T>@Frozen
(consumes Exclusive, produces Frozen)
```

**[T-Ptr-Clone-Shared] Clone shared pointer:**

```
[T-Ptr-Clone-Shared]
Γ ⊢ ptr : Ptr<T>@Shared
────────────────────────────────────────────────────
Γ ⊢ ptr.clone() : Ptr<T>@Shared
(increments static reference count)
```

**[T-Ptr-Clone-Frozen] Clone frozen pointer:**

```
[T-Ptr-Clone-Frozen]
Γ ⊢ ptr : Ptr<T>@Frozen
────────────────────────────────────────────────────
Γ ⊢ ptr.clone() : Ptr<T>@Frozen
(frozen pointers can be freely cloned)
```

**[T-Ptr-Read-Exclusive] Read through exclusive pointer:**

```
[T-Ptr-Read-Exclusive]
Γ ⊢ ptr : Ptr<T>@Exclusive
read<T> ∈ available_effects(Γ)
valid(ptr.addr)
────────────────────────────────────────────────────
Γ ⊢ ptr.read() : T ! read<T>
```

**[T-Ptr-Read-Shared] Read through shared pointer:**

```
[T-Ptr-Read-Shared]
Γ ⊢ ptr : Ptr<T>@Shared
read<T> ∈ available_effects(Γ)
valid(ptr.addr)
────────────────────────────────────────────────────
Γ ⊢ ptr.read() : T ! read<T>
```

**[T-Ptr-Read-Frozen] Read through frozen pointer:**

```
[T-Ptr-Read-Frozen]
Γ ⊢ ptr : Ptr<T>@Frozen
read<T> ∈ available_effects(Γ)
valid(ptr.addr)
────────────────────────────────────────────────────
Γ ⊢ ptr.read() : T ! read<T>
```

**[T-Ptr-Write] Write through exclusive pointer:**

```
[T-Ptr-Write]
Γ ⊢ ptr : Ptr<T>@Exclusive
Γ ⊢ value : T
write<T> ∈ available_effects(Γ)
¬(read<T> ∈ active_effects(Γ))    (exclusion check)
valid(ptr.addr)
────────────────────────────────────────────────────
Γ ⊢ ptr.write(value) : () ! write<T>
```

**[T-Ptr-As-Raw] Convert to raw pointer:**

```
[T-Ptr-As-Raw]
Γ ⊢ ptr : Ptr<T>@S
unsafe_context(Γ)
────────────────────────────────────────────────────
Γ ⊢ ptr.as_raw() : *T
(loses all safety guarantees)
```

**[T-Ptr-Escape-Check] Region escape prevention:**

```
[T-Ptr-Escape-Check]
Γ ⊢ ptr : Ptr<T>@S
ptr internally tagged with region r
ptr escapes region r
────────────────────────────────────────────────────
ERROR: Cannot return region-scoped pointer
(uses existing escape analysis from §29)
```

**[T-Ptr-With-Permission] Pointers with permissions:**

```
[T-Ptr-With-Permission]
Γ ⊢ ptr : Ptr<T>@S
π ∈ {own, mut, iso}
────────────────────────────────────────────────────
Γ ⊢ ptr : π Ptr<T>@S
(permissions apply to pointer itself, not pointee)
```

**[T-Ptr-Effect-Read] Read effect typing:**

```
[T-Ptr-Effect-Read]
Γ ⊢ e : Ptr<T>@S
S ∈ {Exclusive, Shared, Frozen}
────────────────────────────────────────────────────
effect(e.read()) = read<T>
```

**[T-Ptr-Effect-Write] Write effect typing:**

```
[T-Ptr-Effect-Write]
Γ ⊢ e : Ptr<T>@Exclusive
────────────────────────────────────────────────────
effect(e.write(v)) = write<T>
```

**[T-Ptr-Effect-Exclusion] Effect exclusion enforcement:**

```
[T-Ptr-Effect-Exclusion]
read<T> ∈ active_effects(Γ)
Γ ⊢ e : expr requiring write<T>
────────────────────────────────────────────────────
ERROR: Cannot acquire write<T> while read<T> active

[T-Ptr-Effect-Exclusion-Reverse]
write<T> ∈ active_effects(Γ)
Γ ⊢ e : expr requiring read<T>
────────────────────────────────────────────────────
ERROR: Cannot acquire read<T> while write<T> active
```

**[T-Ptr-Modal-Transition] State transition validity:**

```
[T-Ptr-Modal-Transition]
Γ ⊢ ptr : Ptr<T>@S₁
S₁ →[method] S₂ ∈ valid_transitions(Ptr)
────────────────────────────────────────────────────
Γ ⊢ ptr.method() : Ptr<T>@S₂
```

**[T-Ptr-Invalid-Transition] Invalid transition prevention:**

```
[T-Ptr-Invalid-Transition]
Γ ⊢ ptr : Ptr<T>@Shared
────────────────────────────────────────────────────
ERROR: Cannot call share() on @Shared pointer
(no transition from Shared to Shared via share())

[T-Ptr-Invalid-Transition-2]
Γ ⊢ ptr : Ptr<T>@Frozen
────────────────────────────────────────────────────
ERROR: Cannot call freeze() on @Frozen pointer
(already frozen)
```

##### Raw Pointer Operations

**[T-RawPtr-Addr] Taking raw address (SAFE):**

```
[T-RawPtr-Addr-Immut]
Γ ⊢ e : T
────────────────────────────────────
Γ ⊢ &raw e : *T

[T-RawPtr-Addr-Mut]
Γ ⊢ e : mut T
────────────────────────────────────
Γ ⊢ &raw mut e : *mut T
```

**[T-RawPtr-Deref] Dereferencing (UNSAFE):**

```
[T-RawPtr-Deref-Immut]
Γ ⊢ ptr : *T
unsafe_context(Γ)
unsafe.ptr ∈ available_effects(Γ)
────────────────────────────────────
Γ ⊢ *ptr : T ! unsafe.ptr

[T-RawPtr-Deref-Mut]
Γ ⊢ ptr : *mut T
unsafe_context(Γ)
unsafe.ptr ∈ available_effects(Γ)
────────────────────────────────────
Γ ⊢ *ptr : mut T ! unsafe.ptr
```

**[T-RawPtr-Write] Writing through pointer (UNSAFE):**

```
[T-RawPtr-Write]
Γ ⊢ ptr : *mut T
Γ ⊢ value : T
unsafe_context(Γ)
unsafe.ptr ∈ available_effects(Γ)
────────────────────────────────────
Γ ⊢ *ptr = value : () ! unsafe.ptr
```

**[T-RawPtr-Offset] Pointer arithmetic (UNSAFE):**

```
[T-RawPtr-Offset]
Γ ⊢ ptr : *T
Γ ⊢ offset : isize
unsafe_context(Γ)
unsafe.ptr ∈ available_effects(Γ)
────────────────────────────────────
Γ ⊢ ptr.offset(offset) : *T ! unsafe.ptr

[T-RawPtr-Add]
Γ ⊢ ptr : *T
Γ ⊢ count : usize
unsafe_context(Γ)
unsafe.ptr ∈ available_effects(Γ)
────────────────────────────────────
Γ ⊢ ptr.add(count) : *T ! unsafe.ptr

[T-RawPtr-Sub]
Γ ⊢ ptr : *T
Γ ⊢ count : usize
unsafe_context(Γ)
unsafe.ptr ∈ available_effects(Γ)
────────────────────────────────────
Γ ⊢ ptr.sub(count) : *T ! unsafe.ptr
```

**[T-RawPtr-Cast] Type casting (UNSAFE):**

```
[T-RawPtr-Cast]
Γ ⊢ ptr : *T
unsafe_context(Γ)
unsafe.ptr ∈ available_effects(Γ)
────────────────────────────────────
Γ ⊢ ptr.cast<U>() : *U ! unsafe.ptr
```

**[T-RawPtr-Null] Null pointer:**

```
[T-RawPtr-Null]
Γ ⊢ T : Type
────────────────────────────────────
Γ ⊢ null<T>() : *T
```

**[T-RawPtr-Compare] Pointer comparison:**

```
[T-RawPtr-Compare]
Γ ⊢ ptr1 : *T
Γ ⊢ ptr2 : *T
────────────────────────────────────
Γ ⊢ ptr1 == ptr2 : bool
Γ ⊢ ptr1 != ptr2 : bool
```

**[T-RawPtr-NoSafety] No safety checking:**

```
[T-RawPtr-NoSafety]
Raw pointers provide NO compile-time safety:
- No aliasing checks (multiple mutable pointers allowed)
- No lifetime checks (can outlive pointee)
- No null checks (dereferencing null is UB)
- No bounds checks (out-of-bounds is UB)
- Programmer responsible for all correctness
```

##### Pointer Conversions

**[T-Ptr-To-Raw] Safe to raw conversion:**

```
[T-Ptr-To-Raw]
Γ ⊢ ptr : Ptr<T>@S
unsafe_context(Γ)
────────────────────────────────────
Γ ⊢ ptr.as_raw() : *T
(loses all modal states and safety guarantees)
```

**[T-Raw-To-Ptr] Raw to safe conversion (UNSAFE):**

```
[T-Raw-To-Ptr]
Γ ⊢ raw : *T
unsafe_context(Γ)
unsafe.ptr ∈ available_effects(Γ)
programmer ensures: valid(raw) ∧ unique(raw) ∧ ¬escaped(raw)
────────────────────────────────────
Γ ⊢ Ptr.from_raw(raw) : Ptr<T>@Exclusive ! unsafe.ptr
```

**Preconditions (programmer responsibility):**

1. Pointer is valid (not null, properly aligned)
2. Pointer is unique (no other aliases exist)
3. Pointed-to memory has not escaped
4. Pointed-to memory is properly initialized

**[T-RawPtr-Cast-Types] Casting between raw pointer types:**

```
[T-RawPtr-Cast-Types]
Γ ⊢ ptr : *T
unsafe_context(Γ)
────────────────────────────────────
Γ ⊢ ptr.cast<U>() : *U ! unsafe.ptr
```

**Safety considerations:**

- Alignment must be respected (casting from more-aligned to less-aligned is safe)
- Size compatibility is programmer responsibility
- Type aliasing rules apply (accessing `T` through `*U` may violate type system)

#### 2.5.3.3 Type Properties

This section presents key theorems about the pointer type system and their proof sketches.

##### Safe Pointer Properties

**Theorem 2.5.1 (Effect Exclusion Safety):**

If `read<T>` is active in context Γ, then no expression requiring `write<T>` can be typed in Γ, and vice versa.

**Proof sketch:** By induction on typing derivations. The [T-Ptr-Effect-Exclusion] rules explicitly check for conflicting effects and reject programs that would violate exclusion. Effect activation and deactivation follow lexical scoping (effect released when variable goes out of scope).

**Corollary 2.5.1.1 (No Aliased Mutation):**

If multiple pointers (Shared or Frozen) point to the same type T, mutation of T through any pointer or direct access is prevented at compile time.

**Theorem 2.5.2 (State Machine Validity):**

All pointer state transitions follow the modal state machine, and no invalid states are reachable.

**Proof sketch:** Modal type system (§8) enforces valid transitions. Type rules [T-Ptr-Modal-Transition] and [T-Ptr-Invalid-Transition] reject invalid transitions at compile time.

**Theorem 2.5.3 (Region Escape Safety):**

Pointers created in a region cannot escape that region, preventing use-after-free.

**Proof sketch:** [T-Ptr-Escape-Check] rule integrates with existing region escape analysis (§29). Pointers are internally tagged with their creation region, and escape analysis treats them like any region-allocated value.

**Theorem 2.5.4 (Zero Runtime Overhead):**

Modal pointer states and effect checking compile to direct memory access with no runtime tracking.

**Proof sketch:**

- Modal states are compile-time only (§8 modal compilation)
- Effects are compile-time only (§21 effect erasure)
- Pointer fields (addr, region) compile to single raw pointer
- Reference counting in @Shared is static analysis only (no runtime counter)

##### Raw Pointer Properties

**Theorem 2.5.5 (No Safety Guarantees):**

Raw pointer operations provide no compile-time safety guarantees beyond basic type compatibility. All safety is programmer responsibility.

**Proof sketch:** Raw pointers bypass the type system's safety mechanisms:

- No modal states (unlike Ptr<T>)
- No effect exclusion (multiple `*mut T` allowed simultaneously)
- No escape analysis enforcement (can escape regions in unsafe blocks)
- All operations require `unsafe { }` blocks where normal safety rules suspended

**Corollary 2.5.5.1 (Unsafe Boundary):**

Safety properties only hold at the boundary where safe code calls unsafe code, not within unsafe blocks themselves.

**Theorem 2.5.6 (Nullability):**

Raw pointers can be null, unlike references and safe pointers.

**Proof sketch:** `null<T>() : *T` is well-typed. Dereferencing null is undefined behavior, not a type error.

**Theorem 2.5.7 (Uniform Representation):**

All raw pointer types have identical memory representation regardless of pointed-to type.

**Proof sketch:** `size(*T) = size(*mut T) = size(*U) = sizeof(usize)` for all types T, U. Only the address is stored.

##### Unification Properties

**Theorem 2.5.8 (Safe-Raw Distinction):**

Safe pointers (`Ptr<T>@S`) and raw pointers (`*T`, `*mut T`) are distinct types with incompatible operations, preventing accidental mixing.

**Proof sketch:** Type rules explicitly distinguish between safe and raw pointer operations. Conversions between them require unsafe blocks and explicit method calls (`.as_raw()`, `Ptr.from_raw()`), preventing implicit conversions.

**Theorem 2.5.9 (Permission Orthogonality):**

Pointer permissions (`own`, `mut`, `iso`) and pointer states (@Exclusive, @Shared, @Frozen) compose orthogonally without interference.

**Proof sketch:** Permissions apply to the pointer value itself (§11), while states govern aliasing and access (§8). The [T-Ptr-With-Permission] rule shows these are independent dimensions of the type system.

---

#### 2.5.4 Dynamic Semantics

This section presents the runtime behavior and operational semantics of pointer operations.

#### 2.5.4.1 Evaluation Rules

##### Safe Pointer Evaluation

**[E-Ptr-New-Heap] Heap pointer creation:**

```
[E-Ptr-New-Heap]
⟨value, σ⟩ ⇓ ⟨v@addr, σ₁⟩
────────────────────────────────────────────────────
⟨Ptr.new(&value), σ⟩ ⇓ ⟨Ptr@Exclusive { addr: addr, region: heap }, σ₁⟩
```

**[E-Ptr-New-Region] Region pointer creation:**

```
[E-Ptr-New-Region]
⟨value, σ⟩ ⇓ ⟨v@addr, σ₁⟩
addr ∈ region_bounds(r, σ₁)
────────────────────────────────────────────────────
⟨Ptr.new_in<r>(&value), σ⟩ ⇓ ⟨Ptr@Exclusive { addr: addr, region: r }, σ₁⟩
```

**[E-Ptr-Share] Sharing transition:**

```
[E-Ptr-Share]
⟨ptr, σ⟩ ⇓ ⟨Ptr@Exclusive { addr: a, region: r }, σ₁⟩
────────────────────────────────────────────────────
⟨ptr.share(), σ⟩ ⇓ ⟨(
    Ptr@Shared { addr: a, region: r, count: 1 },
    Ptr@Shared { addr: a, region: r, count: 1 }
  ), σ₁⟩
```

**[E-Ptr-Freeze] Freezing transition:**

```
[E-Ptr-Freeze]
⟨ptr, σ⟩ ⇓ ⟨Ptr@Exclusive { addr: a, region: r }, σ₁⟩
────────────────────────────────────────────────────
⟨ptr.freeze(), σ⟩ ⇓ ⟨Ptr@Frozen { addr: a, region: r }, σ₁⟩
```

**[E-Ptr-Read] Read through pointer:**

```
[E-Ptr-Read]
⟨ptr, σ⟩ ⇓ ⟨Ptr@S { addr: a, ... }, σ₁⟩
S ∈ {Exclusive, Shared, Frozen}
valid(a, σ₁)
σ₁(a) = v
────────────────────────────────────────────────────
⟨ptr.read(), σ⟩ ⇓ ⟨v, σ₁⟩
```

**[E-Ptr-Write] Write through pointer:**

```
[E-Ptr-Write]
⟨ptr, σ⟩ ⇓ ⟨Ptr@Exclusive { addr: a, region: r }, σ₁⟩
⟨value, σ₁⟩ ⇓ ⟨v, σ₂⟩
valid(a, σ₂)
────────────────────────────────────────────────────
⟨ptr.write(value), σ⟩ ⇓ ⟨(), σ₂[a ↦ v]⟩
```

##### Raw Pointer Evaluation

**[E-RawPtr-Addr] Taking address:**

```
[E-RawPtr-Addr]
⟨e, σ⟩ ⇓ ⟨v@addr, σ'⟩
────────────────────────────────────
⟨&raw e, σ⟩ ⇓ ⟨*T { addr: addr }, σ'⟩
```

**[E-RawPtr-Deref] Dereferencing:**

```
[E-RawPtr-Deref]
⟨ptr, σ⟩ ⇓ ⟨*T { addr: a }, σ₁⟩
valid(a, σ₁)    (undefined behavior if invalid)
σ₁(a) = v
────────────────────────────────────
⟨*ptr, σ⟩ ⇓ ⟨v, σ₁⟩
```

**[E-RawPtr-Write] Writing:**

```
[E-RawPtr-Write]
⟨ptr, σ⟩ ⇓ ⟨*mut T { addr: a }, σ₁⟩
⟨value, σ₁⟩ ⇓ ⟨v, σ₂⟩
valid(a, σ₂)    (undefined behavior if invalid)
────────────────────────────────────
⟨*ptr = value, σ⟩ ⇓ ⟨(), σ₂[a ↦ v]⟩
```

**[E-RawPtr-Offset] Pointer arithmetic:**

```
[E-RawPtr-Offset]
⟨ptr, σ⟩ ⇓ ⟨*T { addr: a }, σ₁⟩
⟨offset, σ₁⟩ ⇓ ⟨n, σ₂⟩
────────────────────────────────────
⟨ptr.offset(offset), σ⟩ ⇓ ⟨*T { addr: a + n × sizeof(T) }, σ₂⟩

[E-RawPtr-Add]
⟨ptr, σ⟩ ⇓ ⟨*T { addr: a }, σ₁⟩
⟨count, σ₁⟩ ⇓ ⟨n, σ₂⟩
────────────────────────────────────
⟨ptr.add(count), σ⟩ ⇓ ⟨*T { addr: a + n × sizeof(T) }, σ₂⟩

[E-RawPtr-Sub]
⟨ptr, σ⟩ ⇓ ⟨*T { addr: a }, σ₁⟩
⟨count, σ₁⟩ ⇓ ⟨n, σ₂⟩
────────────────────────────────────
⟨ptr.sub(count), σ⟩ ⇓ ⟨*T { addr: a - n × sizeof(T) }, σ₂⟩
```

**[E-RawPtr-Cast] Type casting:**

```
[E-RawPtr-Cast]
⟨ptr, σ⟩ ⇓ ⟨*T { addr: a }, σ'⟩
────────────────────────────────────
⟨ptr.cast<U>(), σ⟩ ⇓ ⟨*U { addr: a }, σ'⟩
```

**[E-RawPtr-Null] Null pointer:**

```
[E-RawPtr-Null]
────────────────────────────────────
⟨null<T>(), σ⟩ ⇓ ⟨*T { addr: 0x0 }, σ⟩
```

##### Conversion Evaluation

**[E-Ptr-To-Raw] Safe to raw conversion:**

```
[E-Ptr-To-Raw]
⟨ptr, σ⟩ ⇓ ⟨Ptr@S { addr: a, ... }, σ'⟩
────────────────────────────────────
⟨ptr.as_raw(), σ⟩ ⇓ ⟨*T { addr: a }, σ'⟩
```

**[E-Raw-To-Ptr] Raw to safe conversion:**

```
[E-Raw-To-Ptr]
⟨raw, σ⟩ ⇓ ⟨*T { addr: a }, σ'⟩
valid(a, σ') ∧ unique(a, σ') ∧ ¬escaped(a, σ')    (programmer responsibility)
────────────────────────────────────
⟨Ptr.from_raw(raw), σ⟩ ⇓ ⟨Ptr@Exclusive { addr: a, region: heap }, σ'⟩
```

#### 2.5.4.2 Memory Representation

##### Safe Pointer Layout

**Modal states compile away:**

```
Compile-time:
  Ptr<T>@Exclusive  →  1 pointer (8 bytes on 64-bit)
  Ptr<T>@Shared     →  1 pointer (8 bytes on 64-bit)
  Ptr<T>@Frozen     →  1 pointer (8 bytes on 64-bit)

Runtime layout (all states):
┌────────────────┐
│ addr: *T (8B)  │
└────────────────┘
Total: 8 bytes (64-bit)
Alignment: 8 bytes

Modal state: ❌ NOT stored (compile-time only)
Region tag:  ❌ NOT stored (compile-time only)
Count field: ❌ NOT stored (static analysis only)
```

**Key insight:** Despite having different states (@Exclusive, @Shared, @Frozen) with different fields in the type system, ALL pointer states compile to a single raw pointer at runtime. The state machine and reference counting are purely compile-time constructs.

##### Raw Pointer Layout

```
Raw pointer (all types):
┌────────────────┐
│ addr (word)    │  4 bytes (32-bit) or 8 bytes (64-bit)
└────────────────┘

Total: sizeof(usize)
Alignment: alignof(usize)
```

**Key properties:**

- Same size as machine word (platform-dependent)
- `*T` and `*mut T` have identical layout (mutability is compile-time only)
- No metadata (unlike dense pointers for slices)
- Direct memory address, no indirection

**Null representation:**

```
null pointer = { addr: 0x0 }
```

**Comparison with dense pointers:**

```
Sparse pointer (*T):   8 bytes (64-bit)
Dense pointer ([T]):   16 bytes (ptr + len)
```

#### 2.5.4.3 Operational Behavior

##### Pointer Validity Conditions

A pointer `ptr : *T` is valid for dereferencing if and only if:

```
valid(ptr.addr, σ) ⟺
  ptr.addr ≠ 0x0                              ∧    (not null)
  ptr.addr % alignof(T) == 0                  ∧    (properly aligned)
  ptr.addr ∈ allocated_memory(σ)              ∧    (points to allocated memory)
  ¬freed(ptr.addr, σ)                         ∧    (not freed)
  ptr.addr ∈ provenance_bounds(ptr)                (within provenance)
```

##### Undefined Behavior

**Operations that cause undefined behavior (UB):**

**1. Dereferencing null:**

```cantrip
unsafe {
    let ptr: *i32 = null<i32>()
    let x = *ptr  // ⚠️ UNDEFINED BEHAVIOR
}
```

**Formal specification:**

```
⟨*ptr, σ⟩ where ptr.addr = 0x0 ⟹ UB
```

**2. Out-of-bounds access:**

```cantrip
unsafe {
    let arr = [1, 2, 3]
    let ptr = &raw arr[0]
    let ptr2 = ptr.offset(10)  // Points outside array
    let x = *ptr2  // ⚠️ UNDEFINED BEHAVIOR
}
```

**Formal specification:**

```
⟨*ptr, σ⟩ where ptr.addr ∉ allocated_memory(σ) ⟹ UB
```

**3. Use-after-free:**

```cantrip
unsafe {
    let ptr = malloc(4)
    free(ptr)
    let x = *ptr  // ⚠️ UNDEFINED BEHAVIOR (dangling pointer)
}
```

**Formal specification:**

```
⟨*ptr, σ⟩ where freed(ptr.addr, σ) ⟹ UB
```

**4. Unaligned access:**

```cantrip
unsafe {
    let bytes: [u8; 8] = [0; 8]
    let ptr: *u64 = (&raw bytes[1]).cast<u64>()
    let x = *ptr  // ⚠️ UNDEFINED BEHAVIOR (misaligned)
}
```

**Formal specification:**

```
⟨*ptr, σ⟩ where ptr.addr % alignof(T) ≠ 0 ⟹ UB
```

**5. Data races:**

```cantrip
unsafe {
    // Thread 1: *ptr_mut = 42
    // Thread 2: *ptr_mut = 100
    // ⚠️ UNDEFINED BEHAVIOR (data race)
}
```

**Formal specification:**

```
concurrent_write(ptr.addr, σ₁) ∧ concurrent_access(ptr.addr, σ₂) ⟹ UB
```

**6. Invalid pointer arithmetic:**

```cantrip
unsafe {
    let ptr: *i32 = null<i32>()
    let ptr2 = ptr.add(1)  // ⚠️ UNDEFINED BEHAVIOR (arithmetic on null)
}
```

**Formal specification:**

```
⟨ptr.offset(n), σ⟩ where ptr.addr ∉ valid_provenance(ptr) ⟹ UB
```

##### Safe vs Raw Pointer Behavior

**Compile-time guarantees:**

| Operation     | Safe Pointer                 | Raw Pointer                  |
| ------------- | ---------------------------- | ---------------------------- |
| Null safety   | ✅ Never null                | ❌ Can be null               |
| Aliasing      | ✅ Effect exclusion enforced | ❌ No tracking               |
| Region escape | ✅ Prevented                 | ❌ Not checked in unsafe     |
| Alignment     | ✅ Guaranteed by type        | ⚠️ Programmer responsibility |
| Bounds        | ✅ Via effects + ownership   | ❌ Programmer responsibility |
| Lifetime      | ✅ Tracked via regions       | ❌ Programmer responsibility |

**Runtime representation:**

Both safe and raw pointers compile to identical machine code—a single pointer value. All safety is enforced statically.

---

### 2.5.5 Null Pointer Handling

#### 2.5.5.1 Overview

Cantrip provides two distinct approaches to handling potentially-absent pointers:

1. **Safe pointers with Option<Ptr<T>>**: Explicit nullability using sum types
2. **Nullable raw pointers**: C-compatible null pointers (`*T`, `*mut T`)

**Key difference:**

- Safe pointers (`Ptr<T>`) **cannot be null** - nullability must be explicit via `Option<Ptr<T>>`
- Raw pointers can be null and require manual null checking in unsafe code

#### 2.5.5.2 Safe Approach: Option<Ptr<T>>

**Type:** `Option<Ptr<T>@State>`

Safe pointers use Cantrip's `Option<T>` type to represent nullable pointers:

```cantrip
// Safe nullable pointer
function find_node(key: i32): Option<Ptr<Node>@Shared>
    uses read<HashMap>
{
    match self.lookup(key) {
        Some(node) => Some(node),
        None => None
    }
}

function use_node() {
    match find_node(42) {
        Some(node) => {
            // Statically guaranteed: node is not null
            let value = node.read().value
        }
        None => {
            // Explicitly handle absence
        }
    }
}
```

**Benefits:**

- ✅ Compile-time enforcement of null checks
- ✅ Pattern matching prevents null dereferences
- ✅ Type system tracks nullability
- ✅ Cannot forget to check for null

##### Guidance (non-normative)

- Prefer `Option<Ptr<T>>` in safe Cantrip code to force explicit handling of absence; the typing rules in §2.5.5.1 guarantee null safety.

#### 2.5.5.3 Unsafe Approach: Nullable Raw Pointers

**Type:** `*T` or `*mut T` with value `null<T>()`

Raw pointers can be null and require manual checking:

**Type rule (from §9.3.2):**

```
[T-RawPtr-Null]
Γ ⊢ T : Type
────────────────────────────────────
Γ ⊢ null<T>() : *T
```

**Evaluation rule (from §9.4.1):**

```
[E-RawPtr-Null]
────────────────────────────────────
⟨null<T>(), σ⟩ ⇓ ⟨*T { addr: 0x0 }, σ⟩
```

**Example:**

```cantrip
extern "C" {
    function malloc(size: usize): *mut u8
        uses ffi.call, unsafe.ptr
}

function allocate_buffer(size: usize): Option<*mut u8>
    uses ffi.call, unsafe.ptr
{
    unsafe {
        let ptr = malloc(size)

        // Manual null check required
        if ptr == null<u8>() {
            return None
        }

        Some(ptr)
    }
}
```

**Danger:**

- ⚠️ Dereferencing null causes undefined behavior
- ⚠️ Compiler cannot enforce null checks
- ⚠️ Easy to forget to check for null

##### Guidance (non-normative)

- Reserve nullable raw pointers for `unsafe` FFI boundaries or manual memory management, pairing each dereference with the checks described in §2.5.5.3.

#### 2.5.5.4 Comparison: Option<Ptr<T>> vs Nullable Raw Pointers

| Aspect                 | Option<Ptr<T>@S>                                 | \*T (nullable)                    |
| ---------------------- | ------------------------------------------------ | --------------------------------- |
| **Null safety**        | Compile-time enforced                            | Manual checking required          |
| **Type system**        | `Option` tracks nullability                      | Type doesn't indicate nullability |
| **Dereferencing null** | Impossible (compile error)                       | Undefined behavior                |
| **Pattern matching**   | ✅ Supported                                     | ❌ Not supported                  |
| **FFI compatibility**  | ❌ Not C-compatible                              | ✅ C-compatible                   |
| **Performance**        | Same (Option<Ptr> optimized to nullable pointer) | Zero overhead                     |
| **Context**            | Safe code                                        | `unsafe { }` blocks               |
| **Typical context**    | Safe Cantrip code                                | FFI, manual memory management     |

#### 2.5.5.5 Null Pointer Representation

**Memory layout:**

```
Null pointer:
┌────────────────┐
│ 0x00000000     │  (32-bit)
└────────────────┘

┌────────────────────────┐
│ 0x0000000000000000     │  (64-bit)
└────────────────────────────┘
```

**Option<Ptr<T>> optimization:**

```
Some(ptr):
┌────────────────┐
│ 0xNNNNNNNN     │  Non-zero address
└────────────────┘

None:
┌────────────────┐
│ 0x00000000     │  Null representation
└────────────────┘
```

Cantrip optimizes `Option<Ptr<T>>` to use the null representation for `None`, making it the same size as a raw pointer.

#### 2.5.5.6 Undefined Behavior: Dereferencing Null

**From §9.4.3 - Undefined Behavior:**

Dereferencing a null pointer causes undefined behavior:

```cantrip
unsafe {
    let ptr: *i32 = null<i32>()
    let x = *ptr  // ⚠️ UNDEFINED BEHAVIOR
}
```

**Safe alternative using Option:**

```cantrip
let ptr_opt: Option<Ptr<i32>@Shared> = None

match ptr_opt {
    Some(ptr) => {
        let x = ptr.read()  // ✅ Safe - cannot be null
    }
    None => {
        // Compile-time enforced handling
    }
}
```

#### 2.5.5.7 FFI Null Pointer Convention

**C NULL convention:**

```
C: NULL  →  Cantrip: null<T>()
```

**Example with C library:**

```cantrip
extern "C" {
    // Returns NULL on failure
    function fopen(path: *u8, mode: *u8): *mut File
        uses ffi.call, unsafe.ptr

    function fclose(file: *mut File)
        uses ffi.call, unsafe.ptr
}

function open_file(path: &str): Result<*mut File, Error>
    uses ffi.call, unsafe.ptr
{
    unsafe {
        let c_path = path.as_ptr()
        let c_mode = "r".as_ptr()

        let file = fopen(c_path, c_mode)

        if file == null<File>() {
            Err(Error::OpenFailed)
        } else {
            Ok(file)
        }
    }
}
```

#### 2.5.5.8 Best Practices

**DO:**

- ✅ Use `Option<Ptr<T>>` in safe code
- ✅ Always check raw pointers for null before dereferencing
- ✅ Convert raw pointers to `Option` at FFI boundaries
- ✅ Use pattern matching for `Option<Ptr<T>>`
- ✅ Document when functions can return null

**DON'T:**

- ❌ Dereference raw pointers without null check
- ❌ Assume non-null without verification
- ❌ Use raw pointers in safe code (use `Option<Ptr<T>>`)
- ❌ Forget to handle `None` case in pattern matching

---

### 2.5.6 Comparison Table

| Feature           | Ptr<T>@State                          | *T / *mut T                      |
| ----------------- | ------------------------------------- | -------------------------------- |
| **Safety**        | Modal states + effect exclusion       | No safety guarantees             |
| **Aliasing**      | Compile-time checks via effects       | No checks, any aliasing allowed  |
| **Null**          | Cannot be null                        | Can be null                      |
| **Context**       | Normal safe code                      | `unsafe { }` blocks only         |
| **Effects**       | `read<T>`, `write<T>` (type-specific) | `unsafe.ptr` (blanket)           |
| **Arithmetic**    | Not allowed                           | Allowed (`offset`, `add`, `sub`) |
| **Casting**       | Type-safe via `as_raw()`              | Arbitrary casting allowed        |
| **Regions**       | Escape analysis enforced              | No enforcement in unsafe         |
| **FFI**           | Via `as_raw()` conversion             | Direct C interop                 |
| **Performance**   | Zero overhead (compiles to raw)       | Zero overhead                    |
| **Dereferencing** | Type-safe `read()`/`write()`          | Unsafe `*ptr`                    |
| **Use case**      | Safe data structures                  | FFI, manual memory, unsafe code  |
| **Runtime cost**  | None (same as raw pointer)            | None                             |

---

### 2.5.7 Advanced Features

#### 2.5.7.1 Pointer Provenance

**Definition (from §2.5.3.1):** Provenance tracks the origin and valid range of pointers for soundness. All pointers carry provenance metadata (conceptual, not stored at runtime).

**Rules:**

1. Pointers derived from an allocation have provenance of that allocation
2. Pointer arithmetic preserves provenance
3. Casting does not change provenance
4. Accessing outside provenance bounds is undefined behavior

#### 2.5.7.2 FFI Attributes

**#[repr(C)]** - Ensures C-compatible struct layout for FFI interoperability. When applied to records containing raw pointers, guarantees field ordering and padding matches C conventions.

**#[null_terminated]** - Marker for C-style null-terminated strings (`*u8` pointers). Used in FFI contexts where strings follow C NULL-termination convention.

#### 2.5.7.3 Volatile Operations

**@volatile** attribute - Prevents compiler optimization of memory accesses for:

- Memory-mapped I/O registers
- Hardware device communication
- Concurrent access from multiple contexts

**Semantics:**

- Compiler must not eliminate, reorder, or cache volatile reads/writes
- Each volatile operation maps to exactly one memory access
- Used with raw pointers in unsafe contexts

#### 2.5.7.4 Alignment

**Alignment checking:**

```
is_aligned(ptr, T) ⟺ (ptr as usize) % align_of<T>() == 0
```

**#[repr(align(N))]** - Enforces minimum alignment for record types:

- Guarantees pointers to the type are N-byte aligned
- N must be power of 2
- Compiler inserts padding as needed
- Useful for cache-line optimization and SIMD requirements

---

### 2.5.9 Notes and Warnings

**Note 2.5.1:** Safe pointers (`Ptr<T>@S`) have zero runtime overhead compared to raw pointers. All modal states and effect tracking are compile-time only.

**Note 2.5.2:** Safe pointers cannot be null. Use `Option<Ptr<T>>` for nullable safe pointers. The compiler optimizes `Option<Ptr<T>>` to the same size as a raw pointer using null-pointer optimization.

**Note 2.5.3:** Effect exclusion (∀ T. ¬(read<T> ∧ write<T>)) is enforced per-type, not per-pointer. Multiple `Ptr<i32>@Shared` pointers can coexist, but acquiring `write<i32>` anywhere prevents all `read<i32>` operations.

**Note 2.5.4:** Raw pointers (`*T`, `*mut T`) are for FFI and manual memory management only. Prefer safe pointers in application code.

**Performance Note 2.5.1:** Safe pointer state transitions (`share()`, `freeze()`, `clone()`) compile to no-ops. The state machine is entirely compile-time.

**Performance Note 2.5.2:** Both safe and raw pointers compile to identical machine code - a single pointer-sized value (8 bytes on 64-bit systems).

**Warning 2.5.1:** Dereferencing a null raw pointer causes undefined behavior. Always check `ptr != null<T>()` before dereferencing raw pointers.

**Warning 2.5.2:** Pointer arithmetic (`offset`, `add`, `sub`) does not perform bounds checking. Out-of-bounds access causes undefined behavior.

**Warning 2.5.3:** Converting raw pointers to safe pointers (`Ptr.from_raw()`) is the programmer's responsibility to ensure validity, uniqueness, and proper initialization.

**Warning 2.5.4:** Raw pointers can escape regions in `unsafe { }` blocks. The region system cannot prevent use-after-free bugs when raw pointers are involved.

**Warning 2.5.5:** Data races with raw pointers cause undefined behavior. The effect system cannot prevent races in unsafe code.

**Implementation Note 2.5.1:** Provenance is conceptual, not stored at runtime. The compiler tracks provenance through static analysis to validate pointer arithmetic safety.

**Implementation Note 2.5.2:** The permission system and modal states are orthogonal: `own Ptr<T>@Exclusive` means the pointer value itself is owned, while @Exclusive controls aliasing of the pointee.

---

**Definition 2.6.1 (Trait):** A trait is a concrete, reusable implementation template that provides procedures for types. Traits are composition units (mixins) that types can **attach** to gain their implementations. Formally, a trait `T` defines procedures `{m₁, ..., mₙ}` with concrete implementations using `Self` to be generic over the attaching type.

## 2.6 Traits

### 2.6.0 Overview

#### 2.6.0.1 Scope

This subsection summarises trait definitions, implementations, and marker traits elaborated in §§2.6.1–2.6.9.

#### 2.6.0.2 Dependencies and Notation

Trait semantics depend on Part III (Permissions) for method receivers and Part IV (Effects) for trait methods with side effects. Self-type handling references §2.10.

#### 2.6.0.3 Guarantees

Trait obligations MUST be satisfied by implementations. Object-safety constraints remain pending (ISSUE TS-7) and MUST be formalised before stabilization.

### 2.6.0.4 Syntax Summary

Grammar for trait definitions and impl blocks appears in §§2.6.1–2.6.2.

### 2.6.0.5 Static Semantics Summary

Typing rules for trait bounds, implementations, and coherence are defined in §§2.6.3–2.6.5.

### 2.6.0.6 Dynamic Semantics Summary

Trait methods dispatch statically; dynamic semantics align with the underlying procedure rules.

### 2.6.0.7 Algorithms

Coherence and trait resolution algorithms remain TODO (ISSUE TS-7).

### 2.6.0.8 Proven Properties

No additional metatheorems beyond the global Progress/Preservation statements (§2.0.7).

### 2.6.0.9 Integration Notes

Trait bounds propagate through generics (§2.7); marker traits interact with permissions and concurrency semantics.

### 2.6.0.10 Notes and Issues

Outstanding: object safety (ISSUE TS-7), marker trait definitions, and example suites (ISSUE TS-5).

### 2.6.1 Overview

**Key innovation/purpose:** Traits enable code reuse through composition rather than inheritance. A trait provides concrete procedure implementations that can be attached to any compatible type, similar to mixins or default interface implementations. This promotes horizontal composition and reduces code duplication.

##### Guidance (non-normative)

- Attach traits when multiple types share concrete behavior; keep implementation-specific procedures on the type itself.
- Use traits as generic bounds (see §2.7) and rely on `Self` to reference the attaching type per §2.6.3.
- Protocols, once specified, will provide the abstract contract layer; until then, treat traits as mixins providing reusable code today.

### 2.6.2 Syntax

#### 2.6.2.1 Concrete Syntax

**Trait declaration:**

```ebnf
TraitDecl    ::= "public"? "trait" Ident GenericParams? RequiredMethods? "{" TraitProcedure* "}"
RequiredMethods ::= "requires" "{" ProcedureSignature+ "}"
TraitProcedure ::= "procedure" Ident "(" ParamList ")" (":" Type)? NeedsClause? ContractClauses? Block
```

**Trait attachment:**

```ebnf
RecordDecl   ::= "record" Ident GenericParams? AttachClause? "{" Fields "}"
AttachClause ::= "attach" TraitList
TraitList    ::= Ident ("," Ident)*
```

**Examples:**

```cantrip
// Trait providing concrete implementations
trait Debug {
    procedure debug($): String
        uses alloc.heap
    {
        format!("{}@{:p}", Self::type_name(), $)
    }

    procedure print_debug($)
        uses io.write, alloc.heap
    {
        println("{}", $.debug())
    }
}

// Trait with required procedure
trait Display
    must {
        procedure fmt($): String
    }
{
    procedure display($)
        uses io.write, alloc.heap
    {
        println("{}", $.fmt())
    }

    procedure to_string($): String
        uses alloc.heap
    {
        $.fmt()
    }
}

// Trait with generic Self
trait Clone {
    procedure clone($): own Self
        uses alloc.heap
    {
        // Generic clone implementation
        Self::deep_copy($)
    }
}

// Record attaching traits
record Point attach Debug, Clone {
    x: f64
    y: f64
}

// Point now has debug(), print_debug(), and clone() procedures
```

#### 2.6.2.2 Abstract Syntax

**Trait declaration:**

```
D ::= trait I must {sig₁, ..., sigₖ} { m₁ = e₁; ...; mₙ = eₙ }

where:
  I = trait identifier
  sig = required procedure signature (abstract)
  m = procedure name
  e = procedure implementation (concrete)
```

**Trait attachment:**

```
D ::= record T attach {I₁, ..., Iₙ} { fields }

Desugars to:
  record T { fields }
  + procedures from I₁
  + procedures from I₂
  + ...
  + procedures from Iₙ
```

**Trait composition:**

```
record T attach {I₁, I₂} { ... }

Procedures available on T:
  T_procedures ∪ I₁_procedures ∪ I₂_procedures

With `Self` in each trait substituted by `T`:
  Self ↦ T
```

#### 2.6.2.3 Basic Examples

**Example 1: Simple trait**

```cantrip
trait Identifiable {
    procedure id($): u64 {
        std.ptr.addr_of($) as u64
    }

    procedure is_same($, other: Self): bool {
        $.id() == other.id()
    }
}

record Counter attach Identifiable {
    value: i32
}

// Usage
let c1 = Counter { value: 0 }
let c2 = Counter { value: 0 }
println("{}", c1.id())              // 140736... (memory address)
println("{}", c1.is_same(c2))      // false (different objects)
```

**Example 2: Trait requiring procedures**

```cantrip
trait Comparable
    must {
        procedure compare($, other: Self): i32
    }
{
    procedure less_than($, other: Self): bool {
        $.compare(other) < 0
    }

    procedure greater_than($, other: Self): bool {
        $.compare(other) > 0
    }

    procedure equals($, other: Self): bool {
        $.compare(other) == 0
    }
}

record Point attach Comparable {
    x: i32
    y: i32

    // Must provide required procedure
    procedure compare($, other: Self): i32 {
        if $.x != other.x {
            $.x - other.x
        } else {
            $.y - other.y
        }
    }
}

// Usage
let p1 = Point { x: 1, y: 2 }
let p2 = Point { x: 3, y: 4 }
println("{}", p1.less_than(p2))     // true (from trait)
println("{}", p1.equals(p2))        // false (from trait)
```

**Example 3: Trait with effects**

```cantrip
trait Serializable {
    procedure save($, path: String): Result<(), Error>
        uses io.write, alloc.heap
    {
        let data = $.to_json()
        std.fs.write(path, data)
    }

    procedure load(path: String): Result<own Self, Error>
        uses io.read, alloc.heap
    {
        let data = std.fs.read(path)?
        Self::from_json(data)
    }
}

record Config attach Serializable {
    host: String
    port: u16

    procedure to_json($): String
        uses alloc.heap
    {
        format!("{{\"host\":\"{}\",\"port\":{}}}", $.host, $.port)
    }

    procedure from_json(json: String): own Self
        uses alloc.heap
    {
        // Parse JSON and return Config
        ...
    }
}
```

### 2.6.3 Static Semantics

#### 2.6.3.1 Well-Formedness Rules

**[WF-Trait] Well-formed trait:**

```
[WF-Trait]
procedures m₁, ..., mₙ well-formed
each procedure uses Self consistently
required signatures sig₁, ..., sigₖ well-formed
procedure names distinct
────────────────────────────────────────
trait I must {sig₁, ..., sigₖ} { m₁, ..., mₙ } well-formed
```

**Explanation:** A trait is well-formed if all its procedures type-check with `Self` as a type variable, all required signatures are valid, and there are no name conflicts.

**[WF-TraitAttach] Well-formed trait attachment:**

```
[WF-TraitAttach]
record T { fields } well-formed
traits I₁, ..., Iₙ well-formed
∀j ∈ {1..n}. T provides required procedures for Iⱼ
no conflicting procedure names across traits
────────────────────────────────────────────────
record T attach {I₁, ..., Iₙ} { fields } well-formed
```

**Explanation:** A type can attach traits if it provides all required procedures and there are no naming conflicts.

**[WF-RequiredProc] Required procedure satisfaction:**

```
[WF-RequiredProc]
trait I must { procedure m(Self, τ₁, ..., τₙ): τ }
record T attach I { ... }
T has procedure m(Self, τ₁, ..., τₙ): τ
────────────────────────────────────────────────
T satisfies requirement for I
```

**Explanation:** Types must provide procedures matching required signatures.

**[WF-NoConflict] No procedure name conflicts:**

```
[WF-NoConflict]
record T attach {I₁, I₂} { ... }
I₁ provides procedure m
I₂ provides procedure n
m ≠ n
────────────────────────────────────────────────
no conflict
```

**Explanation:** Attached traits cannot provide procedures with the same name unless explicitly resolved.

#### 2.6.3.2 Type Rules

**[T-TraitProc] Trait procedure typing:**

```
[T-TraitProc]
trait I { procedure m(Self, τ₁, ..., τₙ): τ { body } }
Γ, Self : Type ⊢ body : τ
────────────────────────────────────────────────
Γ ⊢ trait I well-typed
```

**Explanation:** Trait procedures type-check with `Self` as an abstract type parameter.

**[T-AttachProc] Attached procedure typing:**

```
[T-AttachProc]
trait I { procedure m($, params): τ { body } }
record T attach I { fields }
────────────────────────────────────────────────
Γ ⊢ T.m : (Self = T, params) → τ
```

**Explanation:** When a trait is attached, its procedures become available on the type with `Self` substituted.

**[T-ProcCall] Procedure call via trait:**

```
[T-ProcCall]
record T attach I { ... }
trait I { procedure m($): τ }
Γ ⊢ obj : T
────────────────────────────────────────────────
Γ ⊢ obj.m() : τ
```

**Explanation:** Procedures from attached traits are callable on values of the type.

**[T-RequiredProc] Required procedure call:**

```
[T-RequiredProc]
trait I must { procedure req($): τ }
  { procedure m($): U { ... $.req() ... } }
record T attach I { ... procedure req($): τ { ... } }
────────────────────────────────────────────────
Γ ⊢ T.m calls T.req
```

**Explanation:** Trait procedures can call required procedures, which are resolved to the type's implementation.

#### 2.6.3.3 Type Properties

**Theorem 6.1 (Trait Instantiation):**

When a trait is attached to a type, all trait procedures are instantiated with `Self` bound to the concrete type:

```
trait I { procedure m(Self, τ): U }
record T attach I { ... }
─────────────────────────────────
T.m : (T, τ) → U    (where Self = T)
```

**Proof sketch:** By trait attachment desugaring. The compiler instantiates each trait procedure with the concrete type substituted for `Self`.

**Theorem 6.2 (Required Procedure Satisfaction):**

A type can only attach a trait if it provides all required procedures:

```
trait I must { procedure req($): τ }
record T attach I { ... }
─────────────────────────────────
T must provide: procedure req($): τ
```

**Proof sketch:** By [WF-RequiredProc]. Type checker verifies all requirements before allowing attachment.

**Theorem 6.3 (Trait Composition):**

Multiple traits can be attached if their procedure names don't conflict:

```
trait I₁ { procedures M₁ }
trait I₂ { procedures M₂ }
M₁ ∩ M₂ = ∅
record T attach I₁, I₂ { ... }
─────────────────────────────────
T has procedures: M₁ ∪ M₂
```

**Proof sketch:** By [WF-NoConflict] and trait desugaring. Non-conflicting procedures are composed.

**Property 6.1 (Static Dispatch):**

All trait procedure calls use static dispatch (no vtables):

```cantrip
record T attach I { ... }
let obj = T { ... }
obj.m()  // Resolved at compile time to T's implementation
```

**Property 6.2 (Zero-Cost Abstraction):**

Trait attachment has zero runtime overhead. Procedures are monomorphized at compile time, identical to writing them directly in the type.

**Property 6.3 (Coherence):**

Each type has exactly one set of attached trait procedures. There is no ambiguity in procedure resolution.

### 2.6.4 Dynamic Semantics

#### 2.6.4.1 Evaluation Rules

**[E-TraitProc] Trait procedure call:**

```
[E-TraitProc]
record T attach I { ... }
trait I { procedure m($, params): τ { body } }
⟨obj, σ⟩ ⇓ ⟨v_obj, σ₁⟩
⟨body[Self ↦ T, $ ↦ v_obj, params ↦ args], σ₁⟩ ⇓ ⟨v_result, σ₂⟩
────────────────────────────────────────────────────────────────
⟨obj.m(args), σ⟩ ⇓ ⟨v_result, σ₂⟩
```

**Explanation:** Calling a trait procedure evaluates the trait's implementation with `Self` bound to the concrete type.

**[E-RequiredProc] Required procedure dispatch:**

```
[E-RequiredProc]
record T attach I { procedure req($): τ { body_T } }
trait I must { procedure req($): τ }
  { procedure m($): U { ... $.req() ... } }
⟨obj.m(), σ⟩
  ⇝ evaluate trait body with $.req() calling body_T
────────────────────────────────────────────────────
⟨obj.m(), σ⟩ ⇓ ⟨result, σ'⟩
```

**Explanation:** When trait procedures call required procedures, the call dispatches to the type's implementation.

#### 2.6.4.2 Memory Representation

**Trait procedures:**

Traits have **no runtime representation**. All trait procedures are monomorphized at compile time.

```
trait I { procedure m($): i32 { $.value + 1 } }

record T attach I {
    value: i32
}

Memory layout of T:
┌─────────────┐
│ value: i32  │  (4 bytes)
└─────────────┘

No trait metadata stored.
```

**Procedure resolution:**

All trait procedure calls are resolved statically:

```cantrip
// Source
let obj = T { value: 5 }
obj.m()

// After monomorphization (conceptual)
let obj = T { value: 5 }
T_m_from_I(obj)  // Direct function call

// Compiled (pseudo-assembly)
mov rdi, [obj]     ; load obj pointer
call T_m           ; direct call, no indirection
```

**Attachment compilation:**

```cantrip
// Source
record Point attach Debug, Clone {
    x: i32
    y: i32
}

// Conceptual desugaring
record Point {
    x: i32
    y: i32
}

// Debug trait procedures monomorphized for Point
procedure Point_debug(self: Point): String { ... }
procedure Point_print_debug(self: Point) { ... }

// Clone trait procedures monomorphized for Point
procedure Point_clone(self: Point): own Point { ... }
```

#### 2.6.4.3 Operational Behavior

**Example: Trait attachment and usage**

```cantrip
trait Incrementable {
    procedure increment(mut $) {
        $.value += 1
    }

    procedure add(mut $, n: i32) {
        for _ in 0..n {
            $.increment()
        }
    }
}

record Counter attach Incrementable {
    value: i32
}

// Evaluation trace
let mut c = Counter { value: 0 }
c.add(3)

⇝ Counter { value: 0 }
⇝ c.add(3) where add is from Incrementable
⇝ for _ in 0..3 { c.increment(); }
⇝ c.value += 1  (iteration 1)
⇝ Counter { value: 1 }
⇝ c.value += 1  (iteration 2)
⇝ Counter { value: 2 }
⇝ c.value += 1  (iteration 3)
⇝ Counter { value: 3 }
```

### 2.6.5 Trait Patterns

#### 2.6.5.1 Utility Traits

**Example: Debug trait**

```cantrip
trait Debug {
    procedure type_name(): String {
        // Compiler intrinsic
        intrinsic::type_name::<Self>()
    }

    procedure debug($): String
        uses alloc.heap
    {
        format!("{}{{ ... }}", Self::type_name())
    }

    procedure print_debug($)
        uses io.write, alloc.heap
    {
        println("{}", $.debug())
    }
}

// Attach to any type
record Config attach Debug {
    host: String
    port: u16
}

let cfg = Config { host: "localhost", port: 8080 }
cfg.print_debug()  // Output: Config{ ... }
```

#### 2.6.5.2 Builder Pattern Trait

```cantrip
trait Builder
    must {
        procedure new(): own Self
    }
{
    procedure with_default(): own Self {
        Self::new()
    }

    procedure build_multiple(count: usize): Vec<own Self>
        uses alloc.heap
    {
        var result = Vec::with_capacity(count)
        for _ in 0..count {
            result.push(Self::new())
        }
        result
    }
}

record Connection attach Builder {
    socket: Socket

    procedure new(): own Self {
        own Self { socket: Socket::new() }
    }
}

// Usage
let connections = Connection::build_multiple(10)
```

#### 2.6.5.3 Default Values Trait

```cantrip
trait Default
    must {
        procedure default(): own Self
    }
{
    procedure or_default(opt: Option<Self>): own Self {
        match opt {
            Some(val) => val,
            None => Self::default(),
        }
    }

    procedure array<const N: usize>(): [Self; N]
        where N > 0
    {
        [Self::default(); N]
    }
}

record Config attach Default {
    max_connections: i32

    procedure default(): own Self {
        own Self { max_connections: 100 }
    }
}

// Usage
let cfg = Config::or_default(None)
let cfgs: [Config; 5] = Config::array()
```

#### 2.6.5.4 Comparison Trait

```cantrip
trait Comparable
    must {
        procedure compare($, other: Self): i32
    }
{
    procedure less_than($, other: Self): bool {
        $.compare(other) < 0
    }

    procedure less_equal($, other: Self): bool {
        $.compare(other) <= 0
    }

    procedure greater_than($, other: Self): bool {
        $.compare(other) > 0
    }

    procedure greater_equal($, other: Self): bool {
        $.compare(other) >= 0
    }

    procedure equals($, other: Self): bool {
        $.compare(other) == 0
    }

    procedure clamp($, min: Self, max: Self): Self {
        if $.less_than(min) {
            min
        } else if $.greater_than(max) {
            max
        } else {
            $
        }
    }
}

record Score attach Comparable {
    value: i32

    procedure compare($, other: Self): i32 {
        $.value - other.value
    }
}
```

#### 2.6.5.5 Conversion Trait

```cantrip
trait ToBytes
    must {
        procedure to_bytes($): Vec<u8>
    }
{
    procedure to_hex($): String
        uses alloc.heap
    {
        let bytes = $.to_bytes()
        bytes.iter()
            .map(|b| format!("{:02x}", b))
            .collect()
    }

    procedure byte_count($): usize {
        $.to_bytes().len()
    }
}

record Message attach ToBytes {
    data: String

    procedure to_bytes($): Vec<u8>
        uses alloc.heap
    {
        $.data.as_bytes().to_vec()
    }
}
```

### 2.6.6 Advanced Features

#### 2.6.6.1 Trait Dependencies

Traits can use other traits:

```cantrip
trait Display {
    procedure to_string($): String
        uses alloc.heap
}

trait Loggable
    must {
        procedure level($): LogLevel
    }
{
    // Can require that Self has Display
    procedure log($)
        where Self: Display
        uses io.write, alloc.heap
    {
        println("[{}] {}", $.level(), $.to_string())
    }
}
```

#### 2.6.6.2 Parameterized Traits

Traits can be generic:

```cantrip
trait Convertible<T> {
    procedure convert($): T

    procedure try_convert($): Result<T, String> {
        Ok($.convert())
    }
}

record Number attach Convertible<String> {
    value: i32

    procedure convert($): String
        uses alloc.heap
    {
        $.value.to_string()
    }
}
```

#### 2.6.6.3 Conditional Trait Procedures

Procedures can have trait bounds:

```cantrip
trait Container {
    procedure print_all($)
        where Self: Iterator, Self::Item: Display
        uses io.write, alloc.heap
    {
        for item in $ {
            println("{}", item.to_string())
        }
    }
}
```

### 2.6.7 Trait vs Protocol Comparison

| Aspect             | Trait                           | Protocol                       |
| ------------------ | ------------------------------- | ------------------------------ |
| **Nature**         | Concrete implementation (mixin) | Abstract contract (interface)  |
| **Provides**       | Working code                    | Signatures only                |
| **Keyword**        | `trait` with implementations    | `protocol` (to be defined)     |
| **Relationship**   | "attach" to types               | "implement" by types           |
| **Purpose**        | Code reuse through composition  | Define behavioral requirements |
| **Required procs** | Can require some procedures     | All procedures are required    |
| **Examples**       | Out-of-scope (ISSUE TS-5)       | Out-of-scope (ISSUE TS-5)      |

**Example showing both:**

```cantrip
// Protocol (abstract contract)
protocol Drawable {
    procedure draw($, canvas: Canvas)
    procedure bounds($): Rectangle
}

// Trait (concrete implementation)
trait Cacheable
    must {
        procedure compute_hash($): u64
    }
{
    procedure with_cache($, cache: mut Cache): Result<T> {
        let hash = $.compute_hash()
        if let Some(cached) = cache.get(hash) {
            return Ok(cached)
        }
        // ... compute and cache
    }
}

// Type implements protocol, attaches trait
record Shape : Drawable attach Cacheable {
    ...
}
```

---

## 2.6.9 Notes and Warnings

**Note 1 (Trait as Mixin):** Traits provide concrete implementations (mixins), not abstract interfaces. For abstract contracts, use protocols (when available) or trait bounds on generic parameters.

**Note 2 (Self Type):** The `Self` keyword in trait definitions refers to the type that will attach the trait. It enables generic implementations that work across all attaching types.

**Note 3 (Trait Attachment):** Types attach traits using `attach TraitName` syntax. Multiple traits can be attached by separating with commas: `attach Trait1, Trait2, Trait3`.

**Note 4 (No Override):** When a type attaches a trait, it gains all trait procedures. Types cannot selectively override individual trait procedures - it's all or nothing.

**Note 5 (Trait Bounds):** Generic type parameters can require traits: `procedure foo<T: Display>(value: T)` requires T to have Display trait attached.

**Note 6 (Composition Over Inheritance):** Cantrip uses trait composition instead of class inheritance. Complex behaviors are built by composing multiple simple traits rather than creating deep inheritance hierarchies.

**Warning 1 (Name Conflicts):** If a type defines a procedure with the same name as an attached trait's procedure, there is a name conflict. Cantrip resolves this through explicit qualification or reports a compilation error.

**Warning 2 (Coherence):** Trait implementations must follow coherence rules - you can only attach a trait to a type if you defined either the trait or the type in your crate. This prevents conflicting attachments from multiple libraries.

**Warning 3 (No Partial Attachment):** You cannot attach only some procedures from a trait. Attachment is all-or-nothing - the entire trait implementation comes with attachment.

**Performance Note 1 (Static Dispatch):** Trait procedures attached to concrete types use static dispatch (direct function calls). No runtime overhead compared to regular procedures.

**Performance Note 2 (Monomorphization):** When traits are used with generic parameters, the compiler generates specialized code for each concrete type, enabling full optimization with zero abstraction cost.

**Implementation Note 1 (Trait Objects):** While this specification covers trait attachment to types, Cantrip may support trait objects (dynamic dispatch) through separate mechanisms. See implementation documentation.

**Implementation Note 2 (Standard Traits):** The standard library provides fundamental traits like `Copy`, `Clone`, `Debug`, `Display`, `Default` that types commonly attach for standard behaviors.

---

**Definition 2.7.1 (Parametric Polymorphism):** Generic functions and types abstract over type parameters. A generic function `∀α. e : τ` is universally quantified over type variable α, and can be instantiated at any concrete type. Generic types enable code reuse while maintaining type safety through parametric polymorphism.

## 2.7 Generics and Parametric Polymorphism

### 2.7.0 Overview

#### 2.7.0.1 Scope

Section §2.7 summarises type parameters, const generics, associated types, and lifetime placeholders described in §§2.7.1–2.7.8.

#### 2.7.0.2 Dependencies and Notation

Generics tie into Part III (Permissions) for capability bounds and Part IV (Effects) for effect-qualified functions. Metavariables follow §2.0.4.

#### 2.7.0.3 Guarantees

Generic abstractions MUST respect trait bounds and effect availability. Lifetime-esque parameters remain unspecified (ISSUE TS-3).

### 2.7.0.4 Syntax Summary

Generic grammar is defined in §§2.7.1–2.7.2; associated type syntax appears in §§2.7.3–2.7.4.

### 2.7.0.5 Static Semantics Summary

Constraint solving, instantiation, and generalisation rules reside in §§2.7.5–2.7.6.

### 2.7.0.6 Dynamic Semantics Summary

Generics monomorphise at compile time; no runtime cost is introduced.

### 2.7.0.7 Algorithms

Constraint solving and coherence algorithms are described in §2.0.6.5; additional work for lifetime parameters remains under ISSUE TS-3.

### 2.7.0.8 Proven Properties

Parametricity theorem (§2.0.7) applies; no new results presented here.

### 2.7.0.9 Integration Notes

Generics interact with map types (§2.8) and traits (§2.6). Associated types feed into the alias system (§2.9).

### 2.7.0.10 Notes and Issues

Outstanding work: lifetime parameters (ISSUE TS-3), associated type bounds (ISSUE TS-8), example suites (ISSUE TS-5).

### 2.7.1 Overview

**Key innovation/purpose:** Generics enable writing code once that works with multiple types, providing type-safe abstraction without runtime overhead through compile-time monomorphization.

##### Guidance (non-normative)

- Use generics when behavior is type-agnostic and can be expressed with the bounds in §§2.7.2–2.7.3; stick to concrete types when specialization is required.
- Monitor monomorphization cost—if many instantiations are expected, consider enums or trait objects once object safety (ISSUE TS-7) is defined.

### 2.7.2 Syntax

#### 2.7.2.1 Concrete Syntax

**Generic function:**

```ebnf
GenericFunction ::= "function" Ident GenericParams ParamList (":" Type)? WhereClauses? ContractClauses? Block
GenericParams   ::= "<" TypeParam ("," TypeParam)* ">"
TypeParam       ::= Ident (":" Bounds)?
                  | "const" Ident ":" ConstType
Bounds          ::= Bound ("+" Bound)*
Bound           ::= Ident GenericArgs?
WhereClauses    ::= "where" WhereClause ("," WhereClause)*
WhereClause     ::= Type ":" Bound ("+" Bound)*
```

**Generic record:**

```ebnf
GenericRecord   ::= "record" Ident GenericParams "{" FieldList "}"
```

**Generic enum:**

```ebnf
GenericEnum     ::= "enum" Ident GenericParams "{" VariantList "}"
```

**Examples:**

```cantrip
function identity<T>(x: T): T {
    x
}

function swap<T>(a: T, b: T): (T, T) {
    (b, a)
}

record Pair<T, U> {
    first: T
    second: U
}

enum Option<T> {
    Some(T),
    None,
}
```

#### 2.7.2.2 Abstract Syntax

**Generic function types:**

```
τ ::= ∀α. τ    (universal quantification)
```

**Generic type application:**

```
e ::= e[τ]     (type application)
τ ::= T⟨τ₁, ..., τₙ⟩    (generic type instantiation)
```

**Type schemes:**

```
σ ::= ∀α₁...αₙ. τ    (polytype/type scheme)
```

#### 2.7.2.3 Basic Examples

**Generic identity:**

```cantrip
function identity<T>(x: T): T {
    x
}

let a = identity(42)        // T = i32, returns 42
let b = identity("hello")   // T = str, returns "hello"
```

**Generic container:**

```cantrip
record Container<T> {
    value: T
}

let int_cont = Container { value: 42 }
let str_cont = Container { value: "hello" }
```

### 2.7.3 Static Semantics

#### 2.7.3.1 Well-Formedness Rules

**Generic function:**

```
[WF-Generic-Fun]
Γ, α₁ : Type, ..., αₙ : Type ⊢ body : τ
─────────────────────────────────────────────
Γ ⊢ function f<α₁, ..., αₙ>(...): ∀α₁...αₙ. τ
```

**Generic record:**

```
[WF-Generic-Record]
Γ, α₁ : Type, ..., αₘ : Type ⊢ fields well-formed
──────────────────────────────────────────────────────
Γ ⊢ record S<α₁, ..., αₘ> { ... } : Type → ... → Type
```

**Type parameter bounds:**

```
[WF-Bounded-Param]
Γ ⊢ B₁ : Trait    ...    Γ ⊢ Bₙ : Trait
──────────────────────────────────────────────────────
Γ ⊢ α : B₁ + ... + Bₙ
```

#### 2.7.3.2 Type Rules

**Type instantiation:**

```
[T-Inst]
Γ ⊢ e : ∀α. τ
Γ ⊢ U : Type
──────────────────────────────
Γ ⊢ e[U] : τ[α ↦ U]
```

**Bounded instantiation:**

```
[T-Bounded-Inst]
Γ ⊢ e : ∀α : B. τ
Γ ⊢ U : Type
U : B    (U satisfies bound B)
──────────────────────────────
Γ ⊢ e[U] : τ[α ↦ U]
```

**Generic type construction:**

```
[T-Generic-Type]
Γ ⊢ T : Type → ... → Type    (n parameters)
Γ ⊢ τ₁ : Type    ...    Γ ⊢ τₙ : Type
──────────────────────────────────────────
Γ ⊢ T⟨τ₁, ..., τₙ⟩ : Type
```

**Where clause checking:**

```
[T-Where]
Γ, T : Type, T : C₁, ..., T : Cₙ ⊢ body : U
────────────────────────────────────────────────
Γ ⊢ function f<T> where T: C₁, ..., Cₙ : U
```

#### 2.7.3.3 Type Properties

**Theorem 11.1 (Parametricity):**

Generic functions cannot inspect or depend on the specific type of their parameters:

```
∀α. f : α → β
```

The implementation of f must work uniformly for all types α.

**Theorem 11.2 (Monomorphization):**

Every generic instantiation produces a distinct specialized function:

```
f<i32> ≠ f<String>
```

Each instantiation is compiled separately with no shared code.

**Theorem 11.3 (Type Substitution):**

Type substitution preserves well-typedness:

```
If Γ, α : Type ⊢ e : τ and Γ ⊢ U : Type
Then Γ ⊢ e[α ↦ U] : τ[α ↦ U]
```

#### 2.7.3.4 Type Bounds

**Purpose:** Type bounds constrain generic type parameters to types that satisfy specific trait requirements.

**Syntax:**

```cantrip
function serialize<T>(value: T): String
    where T: Serialize + Clone
    uses alloc.heap
{
    value.clone().to_json()
}
```

**Type rule:**

```
[T-Bounds]
Γ ⊢ T : B₁    ...    Γ ⊢ T : Bₙ
──────────────────────────────────
Γ ⊢ T : B₁ + ... + Bₙ
```

**Example with multiple bounds:**

```cantrip
function process<T, U>(data: T): U
    where
        T: Clone + Serialize,
        U: Display + Debug
    uses alloc.heap
{
    let copy = data.clone()
    let json = copy.to_json()
    U.from_json(json)
}
```

### 2.7.4 Dynamic Semantics

#### 2.7.4.1 Evaluation Rules

**Monomorphization semantics:**

Generic functions are not evaluated directly. Instead, they are instantiated at compile time:

```
[E-Monomorphize]
f<T> where T = ConcreteType
────────────────────────────────────────────
Generates specialized function f_ConcreteType
```

**Generic value construction:**

```
[E-Generic-Record]
⟨e₁, σ⟩ ⇓ ⟨v₁, σ₁⟩    ...    ⟨eₙ, σₙ₋₁⟩ ⇓ ⟨vₙ, σₙ⟩
──────────────────────────────────────────────────────
⟨T⟨U⟩ { f₁: e₁, ..., fₙ: eₙ }, σ⟩ ⇓ ⟨T⟨U⟩ { f₁: v₁, ..., fₙ: vₙ }, σₙ⟩
```

#### 2.7.4.2 Memory Representation

**Generic types:**

Generic types have the same memory layout as their instantiated versions:

```
size(Container<i32>) = size(i32) = 4 bytes
size(Container<String>) = size(String) = 24 bytes (on 64-bit)
```

Each instantiation has its own distinct layout.

**Monomorphization:**

```cantrip
// Generic function
function max<T: Comparable>(a: T, b: T): T {
    if a.compare(b) > 0 { a } else { b }
}

// Compiler generates specialized versions:
function max_i32(a: i32, b: i32): i32 { ... }
function max_f64(a: f64, b: f64): f64 { ... }
function max_String(a: String, b: String): String { ... }
```

**No runtime overhead:**

- All type parameters resolved at compile time
- No type tags or vtables
- Direct function calls (no indirection)
- Same performance as hand-written specialized code

#### 2.7.4.3 Operational Behavior

**Code generation:**

Each unique instantiation generates separate machine code:

```cantrip
let a = identity(42)       // Calls identity_i32
let b = identity(3.14)     // Calls identity_f64
let c = identity("hello")  // Calls identity_str
```

**Trade-off:**

- **Benefit**: Zero runtime overhead, maximum performance
- **Cost**: Increased binary size (code bloat for many instantiations)

### 2.7.5 Advanced Features

#### 2.7.5.1 Const Generics

**Purpose:** Const generics allow types and functions to be parameterized by compile-time constants (e.g., array lengths, matrix dimensions).

**Syntax:**

```ebnf
ConstParam     ::= "const" Ident ":" ConstType
ConstType      ::= "usize" | "u8" | "u16" | "u32" | "u64" | "isize" | "i8" | "i16" | "i32" | "i64" | "bool"
ConstArg       ::= IntegerLiteral | BooleanLiteral | ConstExpr
ConstExpr      ::= literal | arithmetic | sizeof(T) | alignof(T)
```

**Type rules:**

```
[T-Const-Generic]
Γ, N : const usize ⊢ e : τ
───────────────────────────────────────────
Γ ⊢ function f<const N: usize>(...) : τ

[T-Const-Inst]
Γ ⊢ n : usize      n is a compile-time constant
Γ ⊢ f : ∀(const N). τ
───────────────────────────────────────────
Γ ⊢ f<n> : τ[N ↦ n]
```

**Examples:**

```cantrip
// Function parameterized by a const length N
function process<T, const N: usize>(data: [T; N]): T {
    data[0]
}

// Type aliases with const parameters
type Matrix<T, const ROWS: usize, const COLS: usize> = [[T; COLS]; ROWS]

// Records with const parameters
record RingBuffer<T, const CAP: usize> {
    data: [T; CAP]
    head: usize
    tail: usize

    procedure new(): own RingBuffer<T, CAP>
        must CAP > 0
    {
        own RingBuffer {
            data: [T::default(); CAP],
            head: 0,
            tail: 0,
        }
    }

    procedure capacity(self: RingBuffer<T, CAP>): usize {
        CAP
    }
}
```

**Const expression evaluation:**

Const arguments must be evaluable at compile time:

```cantrip
const SIZE: usize = 10

// OK: const value
let buf: RingBuffer<i32, SIZE>

// OK: literal
let buf2: RingBuffer<i32, 100>

// OK: const arithmetic
const DOUBLED: usize = SIZE * 2
let buf3: RingBuffer<i32, DOUBLED>

// ERROR E9310: not a compile-time constant
let n = read_input()
let buf4: RingBuffer<i32, n>  // ERROR
```

**Diagnostics:**

- `E9310` — Non-constant used where const generic argument required
- `E9311` — Const expression not evaluable at compile time

#### 2.7.5.2 Higher-Rank Types (Not Supported)

Cantrip does not support higher-rank polymorphism (rank-2 or higher types):

```cantrip
// NOT SUPPORTED:
type F = map<T>(map<U>(U) → U) → T
```

Generics are first-order only.

---

## 2.7.8 Notes and Warnings

**Note 1 (Monomorphization):** Generics are compiled through monomorphization - the compiler generates specialized code for each concrete type used. This provides zero-cost abstraction but increases binary size.

**Note 2 (Type Inference):** Generic type parameters are often inferred from usage context. Explicit turbofish syntax `function::<Type>()` can specify types when inference fails.

**Note 3 (Trait Bounds):** Generic parameters can be constrained with trait bounds: `<T: Display + Clone>` requires T to have both Display and Clone traits attached.

**Note 4 (Where Clauses):** Complex bounds use where clauses for readability: `where T: Display, U: Clone`. Where clauses appear after parameter list.

**Note 5 (Lifetime Parameters):** Generics can include lifetime parameters (Part IV) alongside type parameters: `<'a, T>` where 'a is a lifetime and T is a type.

**Note 6 (Const Generics):** Compile-time constant parameters enable array sizes and other compile-time values: `<T, const SIZE: usize>`.

**Note 7 (No Higher-Kinded Types):** Cantrip generics are first-order only. You cannot abstract over type constructors (no `F<T>` where F itself is a generic parameter).

**Note 8 (Associated Types):** Traits can define associated types that act as type members, providing an alternative to generic parameters in some cases.

**Warning 1 (Binary Bloat):** Excessive generic instantiations increase binary size. Each unique concrete type generates separate code. Consider using dynamic dispatch or enums for many variants.

**Warning 2 (Orphan Rules):** You can only implement traits for generic types if you defined either the trait or the generic type in your crate (coherence/orphan rules).

**Warning 3 (Recursive Bounds):** Avoid overly complex recursive trait bounds that make type checking difficult. Keep bounds simple and composable.

**Warning 4 (Type Inference Failures):** Complex generic code may require explicit type annotations when the compiler cannot infer types. Use turbofish syntax or variable type annotations.

**Performance Note 1 (Zero Cost):** Properly used generics have zero runtime cost compared to hand-written specialized code. The compiler generates the same machine code.

**Performance Note 2 (Inline Optimization):** Generic functions are excellent candidates for inlining across specializations, enabling aggressive optimization.

**Implementation Note 1 (Default Type Parameters):** Generic types may support default type parameters: `record Foo<T = i32>` where T defaults to i32 if not specified.

**Implementation Note 2 (Generic Limits):** Compilers may impose practical limits on generic parameter counts and nesting depth to ensure reasonable compilation times.

---

**Definition 2.8.1 (Map Type):** A map type `map(T₁, ..., Tₙ) → U` represents a callable function value with a known signature. Formally, it is the type of functions mapping n-tuples of argument types to a return type: `⟦map(T₁, ..., Tₙ) → U⟧ ≅ ⟦T₁ × ... × Tₙ⟧ → ⟦U⟧`. Map types are first-class values with static dispatch and zero runtime overhead.

## 2.8 Map Types

### 2.8.0 Overview

#### 2.8.0.1 Scope

This subsection summarises map types, function values, closures, and method sugar described in §§2.8.1–2.8.8.

#### 2.8.0.2 Dependencies and Notation

Callable types reference Part III (Permissions) for parameter passing, Part IV (Effects) for effectful functions, and §2.10 for self-parameter sugar.

#### 2.8.0.3 Guarantees

Map types MUST carry explicit effect signatures. Captured environments in closures MUST obey LPS rules.

### 2.8.0.4 Syntax Summary

Grammar for map types and procedure declarations is defined in §§2.8.1–2.8.2.

### 2.8.0.5 Static Semantics Summary

Typing rules for function values, closures, and map composition reside in §§2.8.3–2.8.5.

### 2.8.0.6 Dynamic Semantics Summary

Evaluation semantics for function calls, closure capture, and pipelines appear in §§2.8.6–2.8.7.

### 2.8.0.7 Algorithms

Out-of-scope; closure conversion and effect checking algorithms remain to be formalised (ISSUE TS-9).

### 2.8.0.8 Proven Properties

No additional theorems beyond the global Progress/Preservation statements (§2.0.7).

### 2.8.0.9 Integration Notes

Map types integrate with traits (§2.6) and generics (§2.7). Procedures with `$` parameters reference §2.10.

### 2.8.0.10 Notes and Issues

Outstanding: closure capture proof obligations (ISSUE TS-9) and example suites (ISSUE TS-5).

### 2.8.1 Overview

**Key innovation/purpose:** Map types provide first-class functions with static dispatch, enabling higher-order programming without dynamic allocation or runtime overhead. Cantrip distinguishes between **procedures** (what you declare), **functions** (values you can pass around), and **map types** (the type system representation).

**Terminology clarification:**

The distinction between procedures and functions is fundamental:

- **Procedure**: A declaration made with the `procedure` keyword. This is compile-time construct that defines named, executable code. Procedures exist at the **declaration site**.

- **Function**: A first-class value of map type. When you reference a procedure name without calling it, you get a function value that can be passed as an argument, stored in variables, or returned from other procedures. Functions exist at the **use site**.

- **Map type**: The type `map(T₁, ..., Tₙ) → U` describing the signature of a function value. This specifies the parameter types and return type.

- **Closure**: An anonymous function expression with environment capture (see §10.5).

- **Method**: A procedure with explicit `self` parameter using `$` syntax (see §10.6).

**Key insight:** One procedure declaration can produce many function values:

```cantrip
procedure increment(x: i32): i32 { x + 1 }  // ONE procedure declaration

let f = increment   // f binds to the function value
let g = increment   // g binds to the same function value (permission)
let h = increment   // h binds to the same function value

// All three (f, g, h) have type: map(i32) -> i32
// All three reference the same procedure code
```

Think of it this way:

- **Procedures** are like blueprints (what you declare)
- **Functions** are like instances (what you use)
- **Map types** are like type signatures (how you describe them)

##### Guidance (non-normative)

- Use map types when passing or storing callable values without captures; rely on closures (§2.8.5) when environment capture is required.
- Annotate effects on map types so call sites can satisfy the `uses` clauses described in §2.8.3.

### 2.8.2 Syntax

#### 2.8.2.1 Concrete Syntax

**Map type syntax:**

```ebnf
MapType ::= "map" "(" ParamTypes? ")" EffectAnnotation? "->" ReturnType

ParamTypes ::= Type ("," Type)*

ReturnType ::= Type
```

**Procedure declaration syntax:**

```ebnf
ProcedureDecl ::= "procedure" Ident GenericParams? "(" Params? ")" (":" ReturnType)? Block

Params ::= Param ("," Param)*
Param ::= Ident ":" Type
      | Permission? "$"                    // Self parameter shorthand

Permission ::= "own" | "mut"
```

**Function value creation:**

```ebnf
FunctionValue ::= Ident                    // Named procedure
                | ClosureExpr              // Anonymous function
```

**Examples:**

```cantrip
// Procedure declarations
procedure increment(x: i32): i32 { x + 1 }
procedure validate(s: String): bool { s.len() > 0 }
procedure action() { std.io.println("done"); }

// Map type annotations
let add: map(i32, i32) -> i32 = ...
let predicate: map(String) -> bool = ...
let callback: map() -> () = ...

// Named procedure as function value
procedure increment(x: i32): i32 { x + 1 }
let f: map(i32) -> i32 = increment

// Function values cannot be null (unlike raw pointers)
// let null_fn: map(i32) -> i32 = null  // ERROR: No null for map types
```

**Map types with effects:**

```ebnf
MapTypeWithEffects ::= "map" "(" ParamTypes? ")" "->" ReturnType "!" Effects

Effects ::= "{" Effect ("," Effect)* "}"
```

**Examples with effects:**

```cantrip
// Function that performs I/O
let logger: map(String) -> () ! {io.write} = log_message

// Function that allocates
let builder: map(i32) -> String ! {alloc.heap} = int_to_string

// Pure function (no effects)
let pure_math: map(f64, f64) -> f64 = multiply
```

**Method syntax (procedure with self):**

```cantrip
record Counter {
    value: i32

    // Immutable self
    procedure get($): i32 {
        $.value
    }

    // Mutable self
    procedure increment(mut $) {
        $.value += 1
    }

    // Owned self
    procedure consume(own $): i32 {
        $.value
    }
}
```

#### 2.8.2.2 Abstract Syntax

**Type representation:**

```
τ ::= map(τ₁, ..., τₙ) → τ                 (map type)
    | map(τ₁, ..., τₙ) → τ ! ε              (map type with effects)
    | π map(τ₁, ..., τₙ) → τ                (map type with permission)

where:
  τ₁, ..., τₙ are parameter types (contravariant)
  τ is return type (covariant)
  ε is effect set
  π ∈ {own, mut, [default]} is permission
```

**Declaration vs Value (Critical Distinction):**

Procedures exist in the **declaration space**:

```
decl ::= procedure f(x₁: τ₁, ..., xₙ: τₙ): τ { e }    (procedure declaration)
```

Functions exist in the **value space**:

```
value ::= f                                             (procedure name referenced as value)
        | |x₁, ..., xₙ| e                              (closure expression)
```

**The relationship:**

1. You **declare** a procedure with `procedure f(...) { ... }`
2. You **obtain** a function value by referencing `f` without calling it
3. The function value has type `map(T₁, ..., Tₙ) → U` matching the procedure's signature

**Value set (denotational semantics):**

```
⟦map(T₁, ..., Tₙ) → U⟧ = { v | v represents code at address addr,
                           code has signature (T₁, ..., Tₙ) → U,
                           v ≠ null }
```

**Key properties:**

- **Code address:** Function values reference executable code in text segment (NOT heap/stack data)
- **Signature:** Full type information captured in map type (arity, parameter types, return type)
- **Non-null:** Map types cannot be null (unlike raw pointers)
- **Static dispatch:** Call target resolved at compile time through monomorphization
- **Zero-size:** Function values are zero-sized at runtime (compile to direct calls)
- **Copy semantics:** Function values are Copy (no environment capture)
- **Declaration/value split:** One procedure declaration → many function values (all referencing same code)

#### 2.8.2.3 Basic Examples

**Example 1: Declaration Site vs Use Site**

```cantrip
// DECLARATION SITE: Define procedure
procedure add_one(x: i32): i32 { x + 1 }

// USE SITE: Reference procedure as function value
let f: map(i32) -> i32 = add_one   // 'add_one' becomes a function value
let g = add_one                     // Type inference: g has type map(i32) -> i32

// You can bind multiple names to the same function value
let h = add_one   // Another binding to the same function value

// All three can be called
f(5)   // 6
g(5)   // 6
h(5)   // 6

// Direct procedure call (not through function value)
add_one(5)   // 6
```

**Example 2: Multiple Procedures**

```cantrip
// Declare two procedures
procedure add_one(x: i32): i32 { x + 1 }
procedure multiply(x: i32, y: i32): i32 { x * y }

// Create function values
let f1: map(i32) -> i32 = add_one
let f2: map(i32, i32) -> i32 = multiply

// Call through function values
let result1 = f1(42)        // 43
let result2 = f2(6, 7)      // 42
```

**Example 3: Higher-Order Procedures (procedures that accept function values)**

```cantrip
// Procedure that takes a FUNCTION VALUE as parameter
procedure apply(f: map(i32) -> i32, x: i32): i32 {
    f(x)   // Call the function value
}

procedure twice(f: map(i32) -> i32, x: i32): i32 {
    f(f(x))   // Call the function value twice
}

// Pass 'add_one' procedure as a function value
let result1 = apply(add_one, 5)      // 6
let result2 = twice(add_one, 5)      // 7
```

**Example 4: Returning Function Values**

```cantrip
// Procedure that RETURNS a function value
procedure compose<A, B, C>(
    f: map(B) -> C,
    g: map(A) -> B
): map(A) -> C {
    |x| f(g(x))  // Returns closure (a function value)
}
```

**Example 5: What You CANNOT Do**

```cantrip
// ❌ Cannot call a map TYPE (types aren't callable)
let t: map(i32) -> i32 = ...
// map(i32) -> i32(42)  // ERROR: Types aren't values

// ❌ Cannot have null function values
// let null_fn: map(i32) -> i32 = null  // ERROR: No null for map types

// ❌ Cannot modify a procedure declaration
// add_one = something_else  // ERROR: Procedures aren't variables
```

### 2.8.3 Static Semantics

#### 2.8.3.1 Well-Formedness Rules

**Definition 2.8.2 (Well-Formed Map Type):** A map type is well-formed if all parameter types and the return type are well-formed in the given context.

**[WF-MapType] Map type well-formedness:**

```
[WF-MapType]
Γ ⊢ T₁ : Type    ...    Γ ⊢ Tₙ : Type    Γ ⊢ U : Type
─────────────────────────────────────────────────────
Γ ⊢ map(T₁, ..., Tₙ) → U : Type
```

**Explanation:** A map type is well-formed if all component types are well-formed.

**[WF-MapType-Effects] Map type with effects:**

```
[WF-MapType-Effects]
Γ ⊢ map(T₁, ..., Tₙ) → U : Type
ε ⊆ EffectSet
─────────────────────────────────────────────────────
Γ ⊢ map(T₁, ..., Tₙ) → U ! ε : Type
```

**Explanation:** Effects must be a valid subset of the available effect system.

**[WF-MapType-Recursive] Recursive map types:**

```
[WF-MapType-Recursive]
Γ ⊢ T₁ : Type    ...    Γ ⊢ Tₙ : Type
Γ ⊢ U : Type
map(T₁, ..., Tₙ) → U does NOT appear negatively in U
─────────────────────────────────────────────────────
Γ ⊢ map(T₁, ..., Tₙ) → U : Type
```

**Explanation:** Map types can appear recursively only in positive positions (return types, not as function parameters) to ensure well-foundedness.

**[WF-MapType-Permission] Map type with permission:**

```
[WF-MapType-Permission]
Γ ⊢ map(T₁, ..., Tₙ) → U : Type
π ∈ {own, mut, [default]}
─────────────────────────────────────────────────────
Γ ⊢ π map(T₁, ..., Tₙ) → U : Type
```

**Explanation:** Permissions apply to the function value itself, not to its behavior.

#### 2.8.3.2 Type Rules

##### Procedure Declaration Rules

**[T-Procedure] Procedure declaration:**

```
[T-Procedure]
Γ, x₁: T₁, ..., xₙ: Tₙ ⊢ e : U
─────────────────────────────────────────────────────
Γ ⊢ procedure f(x₁: T₁, ..., xₙ: Tₙ): U { e } well-typed
Γ, f : map(T₁, ..., Tₙ) → U
```

**Explanation:** A procedure declaration introduces a new procedure with the corresponding map type.

**[T-ProcedureAsFunction] Procedure name as function value:**

```
[T-ProcedureAsFunction]
procedure f(x₁: T₁, ..., xₙ: Tₙ): U { e } declared in Γ
─────────────────────────────────────────────────────
Γ ⊢ f : map(T₁, ..., Tₙ) → U
```

**Explanation:** When a procedure name appears in expression position (not being called), it is treated as a function value. The procedure's signature `(T₁, ..., Tₙ) → U` becomes the function's map type `map(T₁, ..., Tₙ) → U`.

**Key insight:** This rule bridges the declaration space and value space:

- **Input:** A procedure declaration (compile-time construct)
- **Output:** A function value (runtime construct with map type)

**Example:**

```cantrip
// DECLARATION SPACE: Procedure declaration
procedure square(x: i32): i32 { x * x }

// VALUE SPACE: Function value derived from procedure
let f: map(i32) -> i32 = square  // Type: map(i32) -> i32

// The rule allows 'square' (a procedure name) to appear as a value expression
```

**Distinction:**

```cantrip
square(5)   // Direct call: procedure invocation (not using this rule)
square      // Value reference: becomes function value (uses this rule)
```

##### Function Call Rules

**[T-FunctionCall] Function call through function value:**

```
[T-FunctionCall]
Γ ⊢ f : map(T₁, ..., Tₙ) → U
Γ ⊢ e₁ : T₁    ...    Γ ⊢ eₙ : Tₙ
─────────────────────────────────────────────────────
Γ ⊢ f(e₁, ..., eₙ) : U
```

**Explanation:** Calling a function value with arguments of the correct types produces a value of the return type.

**Example:**

```cantrip
let f: map(i32, i32) -> i32 = add
let result: i32 = f(10, 20)  // Type: i32
```

**[T-FunctionCall-Effects] Function call with effects:**

```
[T-FunctionCall-Effects]
Γ ⊢ f : map(T₁, ..., Tₙ) → U ! ε
Γ ⊢ e₁ : T₁    ...    Γ ⊢ eₙ : Tₙ
ε ⊆ available_effects(Γ)
─────────────────────────────────────────────────────
Γ ⊢ f(e₁, ..., eₙ) : U ! ε
```

**Explanation:** Calling a function with effects must those effects to be available in the calling context.

**Example:**

```cantrip
procedure process_data(f: map(String) -> () ! {io.write}, msg: String)
    uses io.write
{
    f(msg)  // OK: io.write available
}
```

##### Higher-Order Rules

**[T-HigherOrder] Higher-order procedure parameter:**

```
[T-HigherOrder]
Γ ⊢ T₁ : Type    ...    Γ ⊢ Tₙ : Type    Γ ⊢ U : Type
Γ, f : map(T₁, ..., Tₙ) → U ⊢ e : V
─────────────────────────────────────────────────────
Γ ⊢ procedure g(f: map(T₁, ..., Tₙ) → U, ...): V { e } well-typed
```

**Explanation:** Procedures can accept functions as parameters (higher-order procedures).

**Example:**

```cantrip
procedure map<T, U>(transform: map(T) -> U, items: [T]): Vec<U>
    uses alloc.heap
{
    var result = Vec.new()
    for item in items {
        result.push(transform(item))
    }
    result
}
```

**[T-ReturnFunction] Procedure returning function:**

```
[T-ReturnFunction]
Γ ⊢ e : map(T₁, ..., Tₙ) → U
─────────────────────────────────────────────────────
Γ ⊢ procedure g(...): map(T₁, ..., Tₙ) → U { ...; e } well-typed
```

**Explanation:** Procedures can return functions (currying, partial application).

**Example:**

```cantrip
procedure make_adder(x: i32): map(i32) -> i32 {
    |y| x + y  // Returns closure
}
```

##### Subtyping Rules

**[Sub-MapType] Map type subtyping (contravariant/covariant):**

```
[Sub-MapType]
T₂ <: T₁    ...    Tₙ' <: Tₙ    (contravariant in parameters)
U <: U'                         (covariant in return)
─────────────────────────────────────────────────────
map(T₁, ..., Tₙ) → U <: map(T₂, ..., Tₙ') → U'
```

**Explanation:** Map types are contravariant in parameters (can accept more general inputs) and covariant in return type (can return more specific outputs). This is the Liskov Substitution Principle for functions.

**Example:**

```cantrip
// If String <: Object and i32 <: f64:
// map(Object) -> i32 <: map(String) -> f64

procedure process_object(f: map(Object) -> i32, s: String) {
    let more_specific: map(String) -> f64 = ...
    // Can pass more_specific where map(Object) -> i32 expected
}
```

**[Sub-MapType-Effects] Effect subtyping:**

```
[Sub-MapType-Effects]
map(T₁, ..., Tₙ) → U <: map(T₁, ..., Tₙ) → U
ε₁ ⊆ ε₂
─────────────────────────────────────────────────────
map(T₁, ..., Tₙ) → U ! ε₁ <: map(T₁, ..., Tₙ) → U ! ε₂
```

**Explanation:** A function with fewer effects can substitute for one requiring more effects (contravariance on effects).

**[Sub-MapType-Never] Never type in return:**

```
[Sub-MapType-Never]
─────────────────────────────────────────────────────
map(T₁, ..., Tₙ) → ! <: map(T₁, ..., Tₙ) → U
```

**Explanation:** A function that never returns (returns `!`) can substitute for any function type with the same parameters.

##### Permission Integration Rules

**[T-MapType-Owned] Owned function value:**

```
[T-MapType-Owned]
Γ ⊢ f : map(T₁, ..., Tₙ) → U
─────────────────────────────────────────────────────
Γ ⊢ f : own map(T₁, ..., Tₙ) → U
```

**Explanation:** Function values can be owned (though they're typically zero-sized).

**[T-MapType-Move] Moving function values:**

```
[T-MapType-Move]
Γ ⊢ f : own map(T₁, ..., Tₙ) → U
Γ' = Γ \ {f}
─────────────────────────────────────────────────────
Γ ⊢ move f : own map(T₁, ..., Tₙ) → U
Γ' ⊬ f
```

**Explanation:** Function values can be moved, transferring ownership (though in practice they're often Copy).

**[T-MapType-Copy] Function values are Copy:**

```
[T-MapType-Copy]
─────────────────────────────────────────────────────
map(T₁, ..., Tₙ) → U : Copy
```

**Explanation:** Function values (without environment capture) are Copy because they're just code addresses.

##### Arity and Currying Rules

**[T-MapType-ZeroArity] Zero-argument functions:**

```
[T-MapType-ZeroArity]
Γ ⊢ U : Type
─────────────────────────────────────────────────────
Γ ⊢ map() → U : Type
```

**Explanation:** Functions can take zero arguments.

**Example:**

```cantrip
procedure get_random(): i32
    uses random
{
    rand.gen()
}

let f: map() -> i32 = get_random
```

**[T-MapType-Curry] Currying transformation:**

```
[T-MapType-Curry]
Γ ⊢ f : map(T₁, T₂) → U
─────────────────────────────────────────────────────
curry(f) : map(T₁) -> map(T₂) -> U
```

**Explanation:** Multi-parameter functions can be curried into nested single-parameter functions.

**Example:**

```cantrip
procedure curry<A, B, C>(f: map(A, B) -> C): map(A) -> map(B) -> C {
    |a| |b| f(a, b)
}
```

##### Generic Map Type Rules

**[T-MapType-Generic] Generic map types:**

```
[T-MapType-Generic]
Γ, α : Type ⊢ T₁ : Type    ...    Γ, α : Type ⊢ Tₙ : Type
Γ, α : Type ⊢ U : Type
─────────────────────────────────────────────────────
Γ ⊢ map<α>(T₁, ..., Tₙ) → U : Type
```

**Explanation:** Map types can be generic over type parameters.

**Example:**

```cantrip
procedure identity<T>(x: T): T { x }

let f: map<T>(T) -> T = identity
```

**[T-MapType-Generic-Call] Calling generic functions:**

```
[T-MapType-Generic-Call]
Γ ⊢ f : map<α>(T₁, ..., Tₙ) → U
Γ ⊢ V : Type
Γ ⊢ e₁ : T₁[α ↦ V]    ...    Γ ⊢ eₙ : Tₙ[α ↦ V]
─────────────────────────────────────────────────────
Γ ⊢ f<V>(e₁, ..., eₙ) : U[α ↦ V]
```

**Explanation:** Generic functions are instantiated with concrete types when called.

##### Effect Composition Rules

**[T-MapType-EffectComposition] Effect propagation:**

```
[T-MapType-EffectComposition]
Γ ⊢ f : map(T₁, ..., Tₙ) → U ! ε₁
Γ ⊢ g : map(U) → V ! ε₂
─────────────────────────────────────────────────────
Γ ⊢ (g ∘ f) : map(T₁, ..., Tₙ) → V ! (ε₁ ∪ ε₂)
```

**Explanation:** Composing functions with effects produces a function with the union of effects.

**[T-MapType-EffectRequired] Effect availability check:**

```
[T-MapType-EffectRequired]
Γ ⊢ f : map(T₁, ..., Tₙ) → U ! ε
ε ⊆ available_effects(Γ)
─────────────────────────────────────────────────────
Γ ⊢ f callable in context Γ
```

**Explanation:** A function can only be called if all its required effects are available.

##### Type Equivalence Rules

**[Equiv-MapType] Map type equivalence:**

```
[Equiv-MapType]
T₁ ≡ T₁'    ...    Tₙ ≡ Tₙ'    U ≡ U'
─────────────────────────────────────────────────────
map(T₁, ..., Tₙ) → U ≡ map(T₁', ..., Tₙ') → U'
```

**Explanation:** Map types are equivalent if their parameter types and return type are pairwise equivalent.

**[Equiv-MapType-Eta] Eta equivalence:**

```
[Equiv-MapType-Eta]
Γ ⊢ f : map(T₁, ..., Tₙ) → U
─────────────────────────────────────────────────────
f ≡ |x₁, ..., xₙ| f(x₁, ..., xₙ)
```

**Explanation:** A function is equivalent to a lambda that immediately calls it (eta equivalence).

#### 2.8.3.3 Type Properties

##### Safety Properties

**Theorem 10.1 (Type Safety):**

If `Γ ⊢ f : map(T₁, ..., Tₙ) → U` and `Γ ⊢ eᵢ : Tᵢ` for all `i`, then `⟨f(e₁, ..., eₙ), σ⟩` either:

1. Evaluates to a value of type `U`, or
2. Diverges (infinite loop or recursion), or
3. Panics (explicit failure)

**Proof sketch:** By Progress and Preservation lemmas. Well-typed function calls with well-typed arguments cannot get stuck (Progress). If the call reduces, the result maintains the expected type (Preservation).

**Corollary 10.1 (No Invalid Calls):**

Function calls with incorrect argument counts or types are rejected at compile time.

**Theorem 10.2 (Non-Nullability):**

Map types cannot be null. There is no value `null : map(T₁, ..., Tₙ) → U`.

**Proof sketch:** Map types inhabit a different value space than pointers. The type system provides no constructor for null function values, unlike raw pointers which have `null<T>()`.

**Theorem 10.3 (Static Dispatch):**

All map type calls are resolved at compile time with no runtime indirection.

**Proof sketch:** Map types are monomorphized during compilation. Each call site knows the exact target function, enabling direct call instructions without vtable lookup.

##### Subtyping Properties

**Theorem 10.4 (Contravariance in Parameters):**

If `T₂ <: T₁` then `map(T₁) → U <: map(T₂) → U`.

**Proof sketch:** A function accepting more general inputs (T₁) can safely substitute for one requiring more specific inputs (T₂), by Liskov Substitution Principle.

**Example:**

```cantrip
// String <: Object
// map(Object) -> bool <: map(String) -> bool

procedure accepts_string_predicate(f: map(String) -> bool, s: String): bool {
    f(s)
}

procedure general_predicate(obj: Object): bool { true }

// Can pass general_predicate where map(String) -> bool expected
accepts_string_predicate(general_predicate, "hello")  // OK
```

**Theorem 10.5 (Covariance in Return Type):**

If `U <: V` then `map(T₁, ..., Tₙ) → U <: map(T₁, ..., Tₙ) → V`.

**Proof sketch:** A function returning more specific outputs (U) can safely substitute for one returning more general outputs (V).

**Theorem 10.6 (Subtyping Transitivity):**

Map type subtyping is transitive: if `F <: G` and `G <: H`, then `F <: H`.

**Proof sketch:** Follows from transitivity of subtyping on parameter and return types.

##### Equivalence Properties

**Theorem 10.7 (Eta Equivalence):**

For any function `f : map(T₁, ..., Tₙ) → U`, we have:

```
f ≡ |x₁: T₁, ..., xₙ: Tₙ| f(x₁, ..., xₙ)
```

**Proof sketch:** Both expressions denote the same function value. Wrapping a function in a lambda that immediately calls it produces an equivalent function.

**Theorem 10.8 (Beta Equivalence):**

For any lambda `|x: T| e` and value `v : T`:

```
(|x: T| e)(v) ≡ e[x ↦ v]
```

**Proof sketch:** Function application is substitution. Calling a lambda with an argument replaces the parameter with the argument in the body.

##### Performance Properties

**Theorem 10.9 (Zero Runtime Overhead):**

Map types compile to direct function calls with no runtime indirection, vtable lookup, or allocation.

**Proof sketch:** Function values are zero-sized at runtime. All call targets are known at compile time through monomorphization, enabling direct call instructions.

**Property 10.1 (Memory Representation):**

Map types have zero size:

```
sizeof(map(T₁, ..., Tₙ) → U) = 0 bytes
```

**Rationale:** Function values are compile-time constructs that resolve to direct calls. No runtime storage is needed.

**Property 10.2 (Copy Semantics):**

Map types (without environment capture) are `Copy`:

```
∀ T₁, ..., Tₙ, U. map(T₁, ..., Tₙ) → U : Copy
```

**Proof sketch:** Function values are zero-sized, so copying them is trivial (no-op at runtime).

### 2.8.4 Dynamic Semantics

#### 2.8.4.1 Evaluation Rules

**[E-ProcedureValue] Procedure value evaluation:**

```
[E-ProcedureValue]
procedure f(...) { body } declared at address addr
─────────────────────────────────────────────────────
⟨f, σ⟩ ⇓ ⟨fn_val(addr), σ⟩
```

**Explanation:** A named procedure evaluates to a function value pointing to its code address. The store is unchanged because no allocation occurs.

**[E-FunctionCall] Function call evaluation:**

```
[E-FunctionCall]
⟨f, σ⟩ ⇓ ⟨fn_val(addr), σ₁⟩
⟨e₁, σ₁⟩ ⇓ ⟨v₁, σ₂⟩
...
⟨eₙ, σₙ⟩ ⇓ ⟨vₙ, σₙ₊₁⟩
⟨body[params ↦ v₁, ..., vₙ], σₙ₊₁⟩ ⇓ ⟨v, σ'⟩
─────────────────────────────────────────────────────
⟨f(e₁, ..., eₙ), σ⟩ ⇓ ⟨v, σ'⟩
```

**Explanation:** Function call evaluates the function expression, evaluates each argument left-to-right, then executes the function body with parameters substituted by argument values.

**[E-FunctionCall-Direct] Direct function call optimization:**

```
[E-FunctionCall-Direct]
procedure f(x₁, ..., xₙ) { body } known at compile time
⟨e₁, σ⟩ ⇓ ⟨v₁, σ₁⟩    ...    ⟨eₙ, σₙ₋₁⟩ ⇓ ⟨vₙ, σₙ⟩
⟨body[x₁ ↦ v₁, ..., xₙ ↦ vₙ], σₙ⟩ ⇓ ⟨v, σ'⟩
─────────────────────────────────────────────────────
⟨f(e₁, ..., eₙ), σ⟩ ⇓ ⟨v, σ'⟩
```

**Explanation:** When the function is known at compile time, the call can be directly compiled without indirection.

**[E-HigherOrder] Higher-order function call:**

```
[E-HigherOrder]
⟨f_arg, σ⟩ ⇓ ⟨fn_val(addr), σ₁⟩
⟨g, σ₁⟩ ⇓ ⟨fn_val(addr_g), σ₂⟩
⟨body_g[param ↦ fn_val(addr)], σ₂⟩ ⇓ ⟨v, σ'⟩
─────────────────────────────────────────────────────
⟨g(f_arg), σ⟩ ⇓ ⟨v, σ'⟩
```

**Explanation:** When passing a function as an argument, the function value is passed directly (zero-sized), then the receiving function can call it.

**[E-ReturnFunction] Returning function value:**

```
[E-ReturnFunction]
⟨e, σ⟩ ⇓ ⟨fn_val(addr), σ'⟩
─────────────────────────────────────────────────────
⟨return e, σ⟩ ⇓ ⟨fn_val(addr), σ'⟩
```

**Explanation:** Procedures can return function values. The returned value is the code address.

**[E-Compose] Function composition:**

```
[E-Compose]
⟨f, σ⟩ ⇓ ⟨fn_val(addr_f), σ₁⟩
⟨g, σ₁⟩ ⇓ ⟨fn_val(addr_g), σ₂⟩
compose_fn(addr_f, addr_g) = addr_composed
─────────────────────────────────────────────────────
⟨f ∘ g, σ⟩ ⇓ ⟨fn_val(addr_composed), σ₂⟩
```

**Explanation:** Function composition creates a new function that calls both functions in sequence.

**[E-PartialApply] Partial application:**

```
[E-PartialApply]
⟨f, σ⟩ ⇓ ⟨fn_val(addr), σ₁⟩
⟨e₁, σ₁⟩ ⇓ ⟨v₁, σ₂⟩
...
⟨eₖ, σₖ⟩ ⇓ ⟨vₖ, σₖ₊₁⟩
k < n
partial_apply(addr, v₁, ..., vₖ) = addr_partial
─────────────────────────────────────────────────────
⟨f(e₁, ..., eₖ), σ⟩ ⇓ ⟨fn_val(addr_partial), σₖ₊₁⟩
```

**Explanation:** Partially applying a function with fewer arguments than required creates a new function expecting the remaining arguments (requires closure conversion).

#### 2.8.4.2 Memory Representation

**Function value representation:**

Map types are **zero-sized** at runtime. They compile to direct calls with no runtime indirection.

```
Compile-time:
  map(T₁, ..., Tₙ) → U

Runtime representation:
┌─────────────────────────────┐
│ (nothing - zero bytes)      │
└─────────────────────────────┘
Size: 0 bytes
Alignment: 1 byte
```

**Code location:**

Procedures live in the **text segment** (executable code), not the data segment:

```
Memory Layout:
┌─────────────────────────────┐
│ Text Segment (Read-Execute) │
│ ┌─────────────────────────┐ │
│ │ procedure_1() { ... }   │ ← Code address
│ │ procedure_2() { ... }   │ ← Code address
│ │ procedure_3() { ... }   │ ← Code address
│ └─────────────────────────┘ │
└─────────────────────────────┘

┌─────────────────────────────┐
│ Data Segment (Read-Write)   │
│ (function values NOT here)  │
└─────────────────────────────┘
```

**Comparison with other callable types:**

```
Type                    Size (bytes)    Indirection    Allocation
─────────────────────────────────────────────────────────────────
map(i32) -> i32         0               None           None
Closure (no capture)    0               None           None
Closure (with capture)  varies          None           Possible
Raw fn pointer (*fn)    8               None           None (C FFI)
```

**Calling convention:**

Function calls use standard calling conventions (platform-dependent):

- **x86-64:** System V AMD64 ABI (Linux/Unix) or Microsoft x64 (Windows)
- **ARM64:** AAPCS64
- Parameters passed in registers first, then stack
- Return value in designated register

**Example compiled code:**

```cantrip
procedure add(x: i32, y: i32): i32 { x + y }

let f: map(i32, i32) -> i32 = add
let result = f(10, 20)
```

Compiles to (x86-64 assembly):

```asm
add:
    mov eax, edi        ; x in edi
    add eax, esi        ; y in esi
    ret                 ; result in eax

main:
    mov edi, 10         ; First argument
    mov esi, 20         ; Second argument
    call add            ; Direct call (no indirection!)
    ; result in eax
```

**Key insight:** No runtime representation for function values. All calls are direct (static dispatch).

#### 2.8.4.3 Operational Behavior

**Call-by-value semantics:**

Cantrip uses strict call-by-value evaluation:

1. Evaluate function expression to code address
2. Evaluate arguments left-to-right
3. Pass argument values to function
4. Execute function body
5. Return result value

**Example:**

```cantrip
procedure expensive_computation(): i32
    uses io.write
{
    std.io.println("Computing...")
    42
}

procedure use_twice(x: i32, y: i32): i32 {
    x + y
}

// Arguments evaluated exactly once before call
let result = use_twice(expensive_computation(), expensive_computation())
// Prints "Computing..." twice
```

**Evaluation order:**

Arguments are evaluated **left-to-right** before the function is called:

```cantrip
procedure side_effect(x: i32, msg: String): i32
    uses io.write
{
    std.io.println(msg)
    x
}

// Left-to-right evaluation
side_effect(
    side_effect(1, "first"),   // Executed first
    side_effect(2, "second")    // Executed second
)
// Output: "first" then "second"
```

**Tail call optimization:**

Cantrip implementations SHOULD optimize tail calls:

```cantrip
procedure factorial_tail(n: i32, acc: i32): i32 {
    if n <= 1 {
        acc
    } else {
        factorial_tail(n - 1, n * acc)  // Tail call - no stack growth
    }
}
```

**Compilation strategy:** Tail calls can be compiled to jumps instead of calls, preventing stack overflow in recursive functions.

**Function inlining:**

Small functions are typically inlined at call sites:

```cantrip
procedure add_one(x: i32): i32 { x + 1 }

let y = add_one(42)  // Compiled to: let y = 42 + 1
```

**Stack frame layout:**

Function calls create stack frames with:

```
┌─────────────────────────────┐
│ Return address              │
├─────────────────────────────┤
│ Saved registers             │
├─────────────────────────────┤
│ Local variables             │
├─────────────────────────────┤
│ Parameters (if stack-passed)│
└─────────────────────────────┘
```

**Performance characteristics:**

```
Operation                  Cost
──────────────────────────────────────────
Function value creation    0 (compile-time)
Function call (direct)     ~5 cycles (call + ret)
Function call (inline)     0 (inlined away)
Parameter passing          1-2 cycles per register
```

### 2.8.5 Closures

**Definition 2.8.3 (Closure):** A closure is an anonymous function with environment capture. Formally, a closure is a pair `⟨λ(x₁, ..., xₙ). e, ρ⟩` where `λ(x₁, ..., xₙ). e` is the function body and `ρ` is the captured environment.

#### 2.8.5.1 Closure Syntax

**Closure expression syntax:**

```ebnf
ClosureExpr ::= "|" ParamList? "|" Expr
              | "|" ParamList? "|" "->" Type Expr

ParamList ::= Param ("," Param)*
Param ::= Ident (":" Type)?
```

**Examples:**

```cantrip
// No parameters
let f = || 42

// With parameters (type inference)
let add = |x, y| x + y

// With explicit types
let multiply: map(i32, i32) -> i32 = |x: i32, y: i32| x * y

// With explicit return type
let divide = |x: i32, y: i32| -> f64 { (x as f64) / (y as f64) }

// Multi-line body
let complex = |x| {
    let temp = x * 2
    temp + 1
}
```

#### 2.8.5.2 Environment Capture

**Capture modes:**

Closures can capture variables from their environment in three ways:

1. **By immutable value** (default):

```cantrip
let x = 42
let f = || x + 1  // Captures x (Copy)
```

2. **By mutable capture** (if closure mutates):

```cantrip
var count = 0
let mut increment = || { count += 1; }  // Captures count mutably
```

3. **By move** (takes ownership):

```cantrip
let s = String.from("hello")
let f = move || s.len()  // Captures own String
// s no longer accessible
```

#### 2.8.5.3 Closure Types

Closures implement one of three traits based on their capture behavior:

**Fn trait (immutable access):**

```cantrip
trait Fn<Args> {
    type Output
    procedure call($, args: Args): Self.Output
}
```

Example:

```cantrip
let x = 10
let f = |y| x + y  // Closure capturing &x: inferred type <closure(i32) => i32>
```

**FnMut contract (mutable access):**

```cantrip
contract FnMut<Args> {
    type Output
    procedure call_mut(mut $, args: Args): Self.Output
}
```

Example:

```cantrip
var count = 0
let f = || { count += 1; count }  // Closure capturing count mutably
```

**FnOnce contract (consumes captured values):**

```cantrip
contract FnOnce<Args> {
    type Output
    procedure call_once(own $, args: Args): Self.Output
}
```

Example:

```cantrip
let s = String.from("data")
let f = move || s.len()  // Closure with move, consumes String
```

#### 2.8.5.4 Closure Type Rules

**[T-Closure-Simple] Closure without capture:**

```
[T-Closure-Simple]
Γ, x₁: T₁, ..., xₙ: Tₙ ⊢ e : U
no free variables in e
─────────────────────────────────────────────────────
Γ ⊢ |x₁: T₁, ..., xₙ: Tₙ| e : map(T₁, ..., Tₙ) → U
```

**Explanation:** Closures without environment capture have map type.

**[T-Closure-Capture-Immut] Closure with immutable capture:**

```
[T-Closure-Capture-Immut]
Γ, x₁: T₁, ..., xₙ: Tₙ ⊢ e : U
free_vars(e) = {y₁: S₁, ..., yₘ: Sₘ} ⊆ Γ
all yᵢ used immutably in e
─────────────────────────────────────────────────────
Γ ⊢ |x₁, ..., xₙ| e : <closure(T₁, ..., Tₙ) -> U> : Fn(T₁, ..., Tₙ) -> U
```

**Explanation:** Closures capturing immutable references have closure type implementing `Fn` contract.

**[T-Closure-Capture-Mut] Closure with mutable capture:**

```
[T-Closure-Capture-Mut]
Γ, x₁: T₁, ..., xₙ: Tₙ ⊢ e : U
free_vars(e) = {y₁: S₁, ..., yₘ: Sₘ} ⊆ Γ
some yᵢ used mutably in e
─────────────────────────────────────────────────────
Γ ⊢ |x₁, ..., xₙ| e : <closure(T₁, ..., Tₙ) -> U> : FnMut(T₁, ..., Tₙ) -> U
```

**Explanation:** Closures requiring mutable access to captured variables implement `FnMut` contract.

**[T-Closure-Move] Closure with move capture:**

```
[T-Closure-Move]
Γ, x₁: T₁, ..., xₙ: Tₙ ⊢ e : U
free_vars(e) = {y₁: S₁, ..., yₘ: Sₘ} ⊆ Γ
Γ' = Γ \ {y₁, ..., yₘ}
─────────────────────────────────────────────────────
Γ ⊢ move |x₁, ..., xₙ| e : <closure(T₁, ..., Tₙ) -> U> : FnOnce(T₁, ..., Tₙ) -> U
Γ' ⊬ y₁, ..., yₘ
```

**Explanation:** Move closures take ownership of captured variables, implementing `FnOnce` contract.

#### 2.8.5.5 Closure Memory Representation

**Without captures (zero-sized):**

```
Closure with no captures:
┌────────────────────┐
│ (empty - 0 bytes)  │
└────────────────────┘
```

**With captures:**

```
Closure with captures:
┌────────────────────────────┐
│ Captured variable 1        │
├────────────────────────────┤
│ Captured variable 2        │
├────────────────────────────┤
│ ...                        │
└────────────────────────────┘
Size: sum of captured variable sizes
Alignment: max alignment of captured variables
```

**Example:**

```cantrip
let x: i32 = 10
let y: f64 = 3.14
let closure = |z: i32| x + z + (y as i32)

// Closure representation:
// ┌──────────┬──────────┐
// │ x (4B)   │ y (8B)   │
// └──────────┴──────────┘
// Total: 16 bytes (8-byte aligned)
```

### 2.8.6 Methods (Procedures with Self)

**Definition 2.8.4 (Method):** A method is a procedure with an explicit self parameter, defined within a record or modal type definition. Methods use the `$` shorthand for `self: Permission Self`.

#### 2.8.6.1 Method Syntax

```cantrip
record TypeName {
    // fields...

    procedure method_name(Permission? $, params): ReturnType {
        body
    }
}
```

**Self parameter forms:**

- `$` — Immutable self (desugars to `self: Self`)
- `mut $` — Mutable self (desugars to `self: mut Self`)
- `own $` — Owned self (desugars to `self: own Self`)

**Examples:**

```cantrip
record Counter {
    value: i32

    // Constructor (no self parameter)
    procedure new(initial: i32): own Self {
        own Self { value: initial }
    }

    // Immutable method
    procedure get($): i32 {
        $.value
    }

    // Mutable method
    procedure increment(mut $) {
        $.value += 1
    }

    // Consuming method
    procedure consume(own $): i32 {
        $.value  // self destroyed after return
    }
}
```

#### 2.8.6.2 Method Type Rules

**[T-Method-Immut] Immutable method:**

```
[T-Method-Immut]
record T { procedure m($, x₁: T₁, ..., xₙ: Tₙ): U { e } }
Γ, self: T, x₁: T₁, ..., xₙ: Tₙ ⊢ e : U
─────────────────────────────────────────────────────
Γ ⊢ T::m : map(T, T₁, ..., Tₙ) → U
```

**[T-Method-Call] Method call:**

```
[T-Method-Call]
Γ ⊢ obj : T
Γ ⊢ T::m : map(T, T₁, ..., Tₙ) → U
Γ ⊢ e₁ : T₁    ...    Γ ⊢ eₙ : Tₙ
─────────────────────────────────────────────────────
Γ ⊢ obj.m(e₁, ..., eₙ) : U
```

**Explanation:** Method call syntax `obj.m(args)` desugars to `T::m(obj, args)`.

#### 2.8.6.3 UFCS (Universal Function Call Syntax)

Methods can be called using function syntax:

```cantrip
let counter = Counter.new(0)

// Method syntax
counter.increment()

// Function syntax (UFCS)
Counter::increment(counter)

// Both are equivalent
```

## 2.8.8 Notes and Warnings

**Note 1 (Terminology):** Cantrip distinguishes between _procedures_ (what you declare), _functions_ (first-class values), and _map types_ (the type system representation). This differs from languages that use "function" for all three concepts.

**Note 2 (Zero-Size Functions):** Function values without captured environment are zero-sized at runtime. They compile to direct calls with no allocation or indirection.

**Note 3 (Static Dispatch):** All map type calls use static dispatch (compile-time resolution). There is no dynamic dispatch or vtable lookup for plain function values.

**Note 4 (Non-Nullable):** Map types cannot be null, unlike raw function pointers in FFI contexts. Use Option<map(T) -> U> for nullable functions.

**Note 5 (Copy Semantics):** Function values (without environment capture) implement Copy trait. Passing a function value creates no copies at runtime.

**Note 6 (Closure Capture):** Closures can capture variables by immutable reference (Fn), mutable reference (FnMut), or by move (FnOnce). Capture mode is inferred from closure body.

**Note 7 (Method Sugar):** The `$` parameter is syntactic sugar for `self: Permission Self`. Methods can be called as `obj.method()` or `Type::method(obj)` (UFCS).

**Note 8 (Contravariance):** Map types are contravariant in parameters (accept more general inputs) and covariant in returns (return more specific outputs).

**Warning 1 (Closure Size):** Closures with captured environment have non-zero size equal to the sum of captured variable sizes. Large captures can impact performance.

**Warning 2 (Move Closures):** The `move` keyword forces ownership transfer of all captured variables. Original variables become inaccessible after closure creation.

**Warning 3 (Recursive Map Types):** Map types can appear recursively only in positive positions (return types, not parameters) to ensure well-foundedness.

**Warning 4 (Effect Requirements):** Functions with effects (`map(T) -> U ! {e}`) can only be called in contexts where those effects are available via `uses` declarations.

**Performance Note 1 (Inlining):** Small functions are typically inlined at call sites, eliminating call overhead entirely. Use `#[inline]` attributes to control inlining.

**Performance Note 2 (Tail Call Optimization):** Cantrip implementations should optimize tail calls to prevent stack overflow in recursive functions. Tail-recursive functions compile to loops.

**Performance Note 3 (Monomorphization):** Generic functions are monomorphized - separate code generated for each concrete type instantiation. This enables full optimization but increases binary size.

**Implementation Note 1 (Calling Conventions):** Function calls follow platform-specific calling conventions (System V AMD64, Microsoft x64, AAPCS64). First parameters pass in registers, remainder on stack.

**Implementation Note 2 (Function Pointers for FFI):** For C FFI, use raw function pointers (`*fn`) instead of map types. Map types are Cantrip-specific and don't match C ABI.

---

**Definition 2.9.1 (Type Alias):** A type alias introduces a new name for an existing type. Type aliases are transparent: they create a synonym for a type rather than a new distinct type. Formally, if `type A = τ`, then `A ≡ τ` (type equivalence).

## 2.9 Type Aliases

### 2.9.0 Overview

#### 2.9.0.1 Scope

Section §2.9 summarises transparent aliases, newtype patterns, and alias constraints detailed in §§2.9.1–2.9.8.

#### 2.9.0.2 Dependencies and Notation

Aliases rely on Part III (Permissions) for move semantics and §2.7 for generic propagation. Metavariables match §2.0.4.

#### 2.9.0.3 Guarantees

Aliases MUST remain transparent where specified and respect the Copy/Drop traits of their targets.

### 2.9.0.4 Syntax Summary

Alias declarations are described in §§2.9.1–2.9.2.

### 2.9.0.5 Static Semantics Summary

Equivalence rules and visibility constraints appear in §§2.9.3–2.9.5.

### 2.9.0.6 Dynamic Semantics Summary

Aliases introduce no runtime behaviour; semantics defer to the underlying types.

### 2.9.0.7 Algorithms

Out-of-scope; alias expansion is handled by the equivalence rules in §2.0.6.2.

### 2.9.0.8 Proven Properties

Alias transparency feeds into the equivalence theorem (§2.0.6.2). No additional properties are listed.

### 2.9.0.9 Integration Notes

Aliases interact with map types (§2.8) and generics (§2.7) through substitution.

### 2.9.0.10 Notes and Issues

Example suites pending (ISSUE TS-5).

### 2.9.1 Overview

**Key innovation/purpose:** Type aliases provide meaningful names for complex types, improving code readability and maintainability without introducing runtime overhead or new type distinctions.

##### Guidance (non-normative)

- Introduce aliases to shorten complex signatures or express domain terminology; the equivalence rules in §2.9.3 keep them transparent.
- When nominal distinction or separate trait implementations are required, promote the alias to a newtype (see §2.9.6).

### 2.9.2 Syntax

#### 2.9.2.1 Concrete Syntax

**Grammar:**

```ebnf
TypeAlias     ::= "type" Ident GenericParams? WhereClauses? "=" Type ";"
GenericParams ::= "<" TypeParam ("," TypeParam)* ">"
TypeParam     ::= Ident (":" Bounds)?
WhereClauses  ::= "where" WhereClause ("," WhereClause)*
WhereClause   ::= Type ":" Bound ("+" Bound)*
```

**Syntax:**

```cantrip
// Simple type alias
type Kilometers = f64

// Generic type alias
type Result<T> = Result<T, Error>

// Complex type alias (function pointer)
type Callback = map(String) → bool

// Type alias with bounds
type CompareFn<T> = map(T, T) → i32
    where T: Ord

// Type alias for nested generics
type StringMap<V> = HashMap<String, V>
```

#### 2.9.2.2 Abstract Syntax

**Formal representation:**

```
TypeDef ::= type A<α₁, ..., αₙ> = τ

where:
  A      = alias name (identifier)
  α₁...αₙ = type parameters (optional)
  τ      = target type expression
```

**Type equivalence rule:**

```
[Equiv-Alias]
type A<α₁, ..., αₙ> = τ
──────────────────────────────────────
A<τ₁, ..., τₙ> ≡ τ[α₁ ↦ τ₁, ..., αₙ ↦ τₙ]
```

**Components:**

- **Alias name:** Identifier introducing the new type name
- **Type parameters:** Optional generic parameters `<α₁, ..., αₙ>`
- **Target type:** The type being aliased (right-hand side)
- **Where clauses:** Optional trait bounds on type parameters

### 2.9.3 Static Semantics

#### 2.9.3.1 Well-Formedness Rules

**Definition 2.9.2 (Well-Formed Type Alias):** A type alias `type A<α₁, ..., αₙ> = τ` is well-formed if:

1. The target type τ is well-formed under the given type parameters
2. All type parameters appear in τ (no unused parameters)
3. All trait bounds in where clauses are satisfiable
4. The alias does not create cycles (no recursive aliases without indirection)

```
[WF-TypeAlias]
Γ, α₁ : Type, ..., αₙ : Type ⊢ τ : Type
FV(τ) ⊆ {α₁, ..., αₙ}    (no free variables)
No cycles: A ∉ FV(τ)      (non-recursive)
────────────────────────────────────────
Γ ⊢ type A<α₁, ..., αₙ> = τ  well-formed
```

**Well-formedness examples:**

**Valid:**

```cantrip
type Point2D = (f64, f64)                    // ✓ Well-formed
type Callback<T> = map(T) → Option<T>       // ✓ Generic parameter used
type Matrix<T> = Vec<Vec<T>> where T: Num   // ✓ Bounded parameter used
```

**Invalid:**

```cantrip
type Invalid<T> = String    // ✗ Unused type parameter T
type Cycle = Vec<Cycle>     // ✗ Recursive without indirection
type Bad<T> = map(U) → T    // ✗ Free variable U not in parameters
```

#### 2.9.3.2 Type Rules

**[T-TypeAlias] Type Alias Declaration:**

```
[T-TypeAlias]
Γ ⊢ τ : Type
type A = τ declared in scope
─────────────────────────────
Γ ⊢ A ≡ τ    (type equivalence)
```

**Explanation:** Type alias declarations establish type equivalence. The alias name A and its definition τ are fully interchangeable in all contexts.

```cantrip
type Distance = f64

let d: Distance = 5.5      // ✓ Distance = f64
let x: f64 = d             // ✓ Transparent, no conversion needed
```

**[T-TypeAliasInst] Generic Type Alias Instantiation:**

```
[T-TypeAliasInst]
type A<α₁, ..., αₙ> = τ declared
Γ ⊢ τ₁ : Type    ...    Γ ⊢ τₙ : Type
─────────────────────────────────────────
Γ ⊢ A<τ₁, ..., τₙ> ≡ τ[α₁ ↦ τ₁, ..., αₙ ↦ τₙ]
```

**Explanation:** Generic type aliases are instantiated by substituting concrete types for type parameters.

```cantrip
type Pair<T> = (T, T)

let p: Pair<i32> = (1, 2)        // ✓ Pair<i32> ≡ (i32, i32)
let q: (i32, i32) = p            // ✓ Transparent equivalence
```

**[T-AliasTransparency] Transparency Rule:**

```
[T-AliasTransparency]
type A = τ
Γ ⊢ e : τ
───────────
Γ ⊢ e : A

Γ ⊢ e : A
───────────
Γ ⊢ e : τ
```

**Explanation:** Expressions can be typed as either the alias or its definition interchangeably without conversion.

```cantrip
type Age = u32

function is_adult(age: Age) => bool {
    age >= 18     // ✓ Age is transparent to u32, >= works
}

let years: u32 = 25
is_adult(years)  // ✓ u32 can be passed where Age expected
```

**[T-AliasSubstitution] Substitution in Types:**

```
[T-AliasSubstitution]
type A = τ₁
τ₂ contains A
────────────────────────
τ₂[A ↦ τ₁] is valid type
```

**Explanation:** Aliases can be substituted in any type expression.

```cantrip
type Score = i32
type GameState = HashMap<String, Score>

// GameState ≡ HashMap<String, i32>
```

**[T-AliasBounds] Bounded Type Alias:**

```
[T-AliasBounds]
type A<α> = τ where α: Trait
Γ, α : Type, α : Trait ⊢ τ : Type
Γ ⊢ T : Trait    (bound satisfied)
──────────────────────────────────
Γ ⊢ A<T> ≡ τ[α ↦ T]
```

**Explanation:** Type aliases with where clauses require bounds to be satisfied at instantiation.

```cantrip
type Comparator<T> = map(T, T) → i32
    where T: Ord

// ✓ i32 implements Ord
let cmp: Comparator<i32> = |a, b| a - b

// ✗ Error: Vec<T> does not implement Ord
// let bad: Comparator<Vec<i32>> = ...
```

**[T-AliasNesting] Nested Type Aliases:**

```
[T-AliasNesting]
type A = τ₁
type B = τ₂[A]    (B contains A)
─────────────────────────────────
B ≡ τ₂[A ↦ τ₁]    (transitive)
```

**Explanation:** Type aliases compose transitively.

```cantrip
type Key = String
type Value = i32
type Cache = HashMap<Key, Value>

// Cache ≡ HashMap<String, i32>
```

**[T-AliasFunction] Map Type Aliases:**

```
[T-AliasFunction]
type F = map(τ₁, ..., τₙ) → τ
Γ ⊢ f : map(τ₁, ..., τₙ) → τ
───────────────────────────────
Γ ⊢ f : F
```

**Explanation:** Map (function value) types can be aliased for readability.

```cantrip
type BinaryOp = map(i32, i32) → i32

let add: BinaryOp = |a, b| a + b
let sub: BinaryOp = |a, b| a - b

function apply(op: BinaryOp, x: i32, y: i32) => i32 {
    op(x, y)
}
```

**[T-AliasInference] Type Inference with Aliases:**

```
[T-AliasInference]
type A = τ
Γ ⊢ e : _    (type variable)
e used in context expecting τ
─────────────────────────────
Γ ⊢ e : A    (inferred as alias)
```

**Explanation:** Type inference can infer aliases or their definitions.

```cantrip
type Index = usize

let indices = vec![1, 2, 3]  // Inferred as Vec<Index> or Vec<usize>
```

**[T-AliasError] Error Reporting with Aliases:**

```
[T-AliasError]
type A = τ
Γ ⊢ e : τ'    τ' ≠ τ
Expected type: A
─────────────────────────────
ERROR: Expected A (which is τ), found τ'
```

**Explanation:** Error messages display both the alias name and its definition for clarity.

```cantrip
type UserId = u64

function get_user(id: UserId) { ... }

get_user("invalid")
// ERROR: Expected UserId (which is u64), found &str
```

**[T-AliasVisibility] Visibility and Scope:**

```
[T-AliasVisibility]
type A = τ declared in module M
A visible in current scope
────────────────────────────
Can use A ≡ τ
```

**Explanation:** Type aliases respect module visibility rules.

```cantrip
module math {
    pub type Vector = (f64, f64, f64)  // Public alias
    type Internal = i32                 // Private alias
}

use math::Vector
let v: Vector = (1.0, 2.0, 3.0)  // ✓ Public alias accessible

// let x: math::Internal = 5     // ✗ Private alias not accessible
```

**[T-AliasRecursive] Recursive Types with Indirection:**

```
[T-AliasRecursive]
type List<T> = Option<Ptr<(T, List<T>)>@Exclusive>
Ptr provides heap indirection
─────────────────────────────────────────
Recursive type is well-formed
```

**Explanation:** Recursive type aliases require heap indirection (via Ptr) to have finite size.

```cantrip
// ✗ Invalid: Direct recursion
// type List<T> = (T, List<T>)  // Infinite size!

// ✓ Valid: Recursion through heap pointer
type List<T> = Option<Ptr<(T, List<T>)>@Exclusive>

let list: List<i32> = Some(Ptr.new(&(1, None)))
```

**[T-AliasAssociated] Associated Type Aliases:**

```
[T-AliasAssociated]
trait I { type A; }
impl I for T { type A = τ; }
─────────────────────────────
T : I  ∧  T::A ≡ τ
```

**Explanation:** Associated types in trait implementations are type aliases bound to the implementing type.

```cantrip
trait Container {
    type Item
    function get(self: Self, index: usize) => Option<Self::Item>
}

impl Container for Vec<i32> {
    type Item = i32  // Associated type alias

    function get(self: Self, index: usize) => Option<i32> {
        if index < self.len() {
            Some(self[index])
        } else {
            None
        }
    }
}
```

**[T-AliasEffect] Map Type Aliases with Effects:**

```
[T-AliasEffect]
type F = map(τ₁) → τ₂ ! ε
Γ ⊢ f : map(τ₁) → τ₂ ! ε
─────────────────────────
Γ ⊢ f : F
```

**Explanation:** Type aliases can include effect signatures for functions.

```cantrip
type Logger = map(String) → () ! {io.write}
type Reader = map() → String ! {io.read}

let log: Logger = |msg| println(msg)
```

**[T-AliasPermission] Type Aliases with Permissions:**

```
[T-AliasPermission]
type A = π τ    (alias includes permission)
Γ ⊢ e : π τ
─────────────
Γ ⊢ e : A
```

**Explanation:** Type aliases can encode permission qualifiers.

```cantrip
type OwnedString = own String
type SharedInt = imm i32
type MutableVec<T> = mut Vec<T>
```

**[T-AliasConversion] No Implicit Conversion:**

```
[T-AliasConversion]
type A = τ
A and τ are same type
────────────────────────
No conversion needed: A = τ (not A → τ)
```

**Explanation:** Type aliases are transparent; no conversion or coercion occurs.

```cantrip
type Meters = f64

let distance: Meters = 10.5
let value: f64 = distance    // ✓ No conversion, same type

// Newtype would require explicit conversion:
// record MetersNewtype(f64)
// let d = MetersNewtype(10.5)
// let v: f64 = d.0  // Must access field
```

**[T-AliasCopy] Trait Implementation Inheritance:**

```
[T-AliasCopy]
type A = τ
τ : Trait
─────────
A : Trait
```

**Explanation:** Type aliases inherit all trait implementations from their target type.

```cantrip
type Point = (i32, i32)

// (i32, i32) implements Copy, Clone, Debug, etc.
// Therefore Point also implements Copy, Clone, Debug, etc.

let p1: Point = (5, 10)
let p2 = p1  // ✓ Both bind to same value (permission), p1 still usable
let p3 = p1.copy()  // ✓ Explicit copy because (i32, i32) is Copy
```

#### 2.9.3.3 Type Properties

**Theorem 11.1 (Type Equivalence Transitivity):** If `type A = B` and `type B = C`, then `A ≡ C`.

**Proof sketch:** By definition of type alias, `A ≡ B` and `B ≡ C`. Type equivalence is transitive, so `A ≡ C` follows. ∎

**Corollary 11.1:** Type alias chains resolve to their final target type.

**Theorem 11.2 (Type Alias Transparency):** For any type alias `type A = τ`, expressions of type A and expressions of type τ are interchangeable in all contexts.

**Proof sketch:** By [T-AliasTransparency], `Γ ⊢ e : τ` implies `Γ ⊢ e : A` and vice versa. Therefore, A and τ can be used interchangeably. ∎

**Corollary 11.2:** Type aliases do not affect type checking, only improve readability.

**Theorem 11.3 (Well-Founded Recursion):** Recursive type aliases are well-formed if and only if they contain indirection (Box, reference, or pointer).

**Proof sketch:** Without indirection, the type has infinite size, violating well-formedness. With indirection, the size is finite (pointer size + heap allocation). ∎

**Theorem 11.4 (Monomorphization Erasure):** Type aliases are erased during monomorphization and do not affect generated code.

**Proof sketch:** Since type aliases are transparent equivalences, the compiler replaces all alias occurrences with their definitions before code generation. The resulting code is identical to code written without aliases. ∎

**Corollary 11.3:** Type aliases have zero runtime overhead.

### 2.9.4 Dynamic Semantics

#### 2.9.4.1 Evaluation Rules

Type aliases have no evaluation rules because they are purely compile-time constructs. They are erased before code generation.

**[E-AliasErased] Compile-Time Erasure:**

```
[E-AliasErased]
type A = τ
e contains type A
─────────────────────────
e[A ↦ τ] evaluated at runtime (A erased)
```

**Explanation:** At runtime, all type aliases are replaced by their definitions. Type aliases exist only in the source code and type system.

```cantrip
type Distance = f64

let d: Distance = 10.5
// At runtime: let d: f64 = 10.5;  (Distance erased)
```

#### 2.9.4.2 Memory Representation

**Layout:** Type aliases have the same memory layout as their target type.

```
Size(type A = τ) = Size(τ)
Align(type A = τ) = Align(τ)
```

**Examples:**

```cantrip
type Coordinate = (f32, f32)
// Size: 8 bytes (2 × f32)
// Align: 4 bytes (alignment of f32)

type Callback = map(i32) → bool
// Size: 0 bytes (function types are zero-sized)
// Align: 1 byte
```

**Representation invariant:** The memory representation of a value of type A is identical to the memory representation of the same value of type τ.

#### 2.9.4.3 Operational Behavior

Type aliases do not change operational behavior. All operations on aliased types are identical to operations on their target types.

```cantrip
type Index = usize

let i: Index = 5
let j: usize = 10

let sum = i + j  // Same as: 5_usize + 10_usize
// No conversion, same operation
```

**Performance characteristic:** Type aliases have zero runtime cost. They are purely a compile-time abstraction that improves code readability without affecting performance.

### 2.9.5 Newtype Pattern

The **newtype pattern** uses single-field record types to create distinct nominal types. Unlike type aliases, newtypes are not transparent and provide type safety through nominal distinctions.

**Syntax:**

```cantrip
record NewtypeName(InnerType)
```

**Comparison with Type Aliases:**

| Feature                   | Type Alias                | Newtype Pattern                 |
| ------------------------- | ------------------------- | ------------------------------- |
| **Type equivalence**      | Transparent (`A ≡ τ`)     | Distinct nominal type           |
| **Type safety**           | No protection             | Prevents mixing similar types   |
| **Trait implementations** | Inherited from target     | Custom implementations possible |
| **Conversion**            | Automatic (same type)     | Explicit field access           |
| **Use case**              | Readability, abbreviation | Type safety, abstraction        |

**Examples:**

**Type alias (transparent):**

```cantrip
type Meters = f64
type Feet = f64

let m: Meters = 10.0
let f: Feet = 33.0

// ✗ Oops! Can mix Meters and Feet (both are f64)
let sum = m + f  // ✓ Compiles but semantically wrong
```

**Newtype pattern (distinct types):**

```cantrip
record Meters(f64)
record Feet(f64)

let m = Meters(10.0)
let f = Feet(33.0)

// ✓ Cannot mix Meters and Feet (compiler error)
// let sum = m + f  // ✗ ERROR: cannot add Meters and Feet

// Must convert explicitly
function feet_to_meters(f: Feet) => Meters {
    Meters(f.0 * 0.3048)
}

let sum_meters = m.0 + feet_to_meters(f).0  // ✓ Explicit
```

##### Guidance (non-normative)

- Wrap primitives in single-field records when you need nominal distinction, custom trait implementations, or stable APIs with encapsulated representation.

**Example: Preventing ID confusion**

```cantrip
record UserId(u64)
record ProductId(u64)

function get_user(id: UserId) => User { ... }
function get_product(id: ProductId) => Product { ... }

let user_id = UserId(12345)
let product_id = ProductId(67890)

get_user(user_id)      // ✓ Correct
get_user(product_id)   // ✗ ERROR: Expected UserId, found ProductId
```

### 2.9.6 Generic Type Aliases

Generic type aliases allow parameterization by type variables, enabling reusable type abbreviations.

**Syntax:**

```cantrip
type AliasName<T1, T2, ...> = ComplexType<T1, T2, ...>
```

**Examples:**

**Standard library patterns:**

```cantrip
// Result type with fixed error type
type IoResult<T> = Result<T, IoError>
type ParseResult<T> = Result<T, ParseError>

function read_config(): IoResult<Config> {
    // Return type is Result<Config, IoError>
    ...
}
```

**Collection type aliases:**

```cantrip
type StringMap<V> = HashMap<String, V>
type IntSet = HashSet<i32>
type Matrix<T> = Vec<Vec<T>>

let scores: StringMap<i32> = HashMap::new()
let grid: Matrix<f64> = vec![vec![1.0, 2.0], vec![3.0, 4.0]]
```

**Function type aliases:**

```cantrip
type Predicate<T> = map(T) → bool
type Transform<T, U> = map(T) → U
type FoldFn<T, A> = map(A, T) → A

function filter<T>(items: Vec<T>, pred: Predicate<T>) => Vec<T> {
    items.into_iter().filter(pred).collect()
}
```

**Trait-bounded generic aliases:**

```cantrip
type Comparator<T> = map(T, T) → i32
    where T: Ord

type Serializer<T> = map(T) → String
    where T: Display

type Cloner<T> = map(T) → T
    where T: Clone
```

**Complex nested generics:**

```cantrip
// Simplify callback signatures
type AsyncCallback<T> = map(Result<T, Error>) → ()
type EventHandler<E> = map(E) → EventResult
type StateTransition<S> = map(S) → Option<S>

// Simplify data structure signatures
type Graph<N, E> = HashMap<N, Vec<(N, E)>>
type Cache<K, V> = Arc<Mutex<HashMap<K, V>>>
```

## 2.9.7 Notes and Warnings

**Note 1 (Transparency):** Type aliases are completely transparent - they create alternative names, not new types. `type A = B` means A and B are interchangeable everywhere.

**Note 2 (Zero Cost):** Type aliases have zero runtime cost. They exist only at compile time for improved readability and documentation.

**Note 3 (Generic Aliases):** Type aliases can have generic parameters, enabling parameterized abbreviations like `type Result<T> = Result<T, MyError>`.

**Note 4 (Recursive Aliases):** Type aliases can be recursive if they appear through indirection (pointer, Box, etc.): `type List<T> = Option<Box<(T, List<T>)>>`.

**Note 5 (No Separate Implementations):** You cannot implement traits specifically for a type alias. Trait implementations apply to the underlying type.

**Note 6 (Newtype for Safety):** For type safety (preventing misuse of similar types), use the newtype pattern (single-field records) instead of type aliases.

**Note 7 (Module Visibility):** Type aliases respect module visibility. Private aliases are only accessible within their defining module.

**Note 8 (Documentation):** Type aliases serve as inline documentation, making complex types self-explanatory. Use them to communicate intent.

**Warning 1 (No Type Safety):** Type aliases provide NO type safety. `type Meters = f64` and `type Feet = f64` are both `f64` and can be mixed accidentally.

**Warning 2 (Confusing Errors):** Compiler error messages may show either the alias name or the expanded type, potentially causing confusion.

**Warning 3 (Infinite Recursion):** Direct recursive aliases without indirection are invalid: `type Foo = (i32, Foo)` causes infinite expansion. Use `type Foo = (i32, Box<Foo>)`.

**Warning 4 (Cannot Override):** You cannot "override" or "refine" an alias. `type A = B` in one module and `type A = C` in another creates a conflict if both are imported.

**Performance Note 1:** Type aliases have literally zero runtime overhead. All alias resolution happens at compile time.

**Performance Note 2:** Using aliases for complex generic types (like `HashMap<K, V>`) doesn't affect performance - the generated code is identical to using the full type name.

**Implementation Note 1:** Compilers typically implement type aliases through simple substitution during type checking and name resolution.

**Implementation Note 2:** Debuggers may show aliased types by their underlying representation, not the alias name. This is implementation-dependent.

---

**Definition 2.10.1 (Self Type):** The `Self` type is a first-class type alias that refers to the enclosing type. Within procedures defined in a record or modal type `T`, `Self` is equivalent to `T`. The `Self` type enables generic and abstract references to the implementing type without repetition.

## 2.10 Self Type

### 2.10.0 Overview

#### 2.10.0.1 Scope

This subsection summarises the Self keyword and `$` shorthand described in §§2.10.1–2.10.6.

#### 2.10.0.2 Dependencies and Notation

Self relies on Part III (Permissions) for method receivers and §2.6 for trait usage.

#### 2.10.0.3 Guarantees

Self MUST resolve to the enclosing nominal type with all generic parameters instantiated. The `$` shorthand MUST desugar to the explicit self parameter.

### 2.10.0.4 Syntax Summary

Method definitions and `$` sugar appear in §§2.10.1–2.10.2.

### 2.10.0.5 Static Semantics Summary

Typing rules for Self in methods and traits are documented in §§2.10.3–2.10.4.

### 2.10.0.6 Dynamic Semantics Summary

Method calls reduce to procedure invocations with explicit receivers.

### 2.10.0.7 Algorithms

Out-of-scope; desugaring and monomorphisation algorithms follow from §2.8.

### 2.10.0.8 Proven Properties

Self participates in the global safety theorems (§2.0.7).

### 2.10.0.9 Integration Notes

Self interacts with trait implementations (§2.6) and map types (§2.8).

### 2.10.0.10 Notes and Issues

Example suites pending (ISSUE TS-5).

### 2.10.1 Overview

**Key innovation/purpose:** The `Self` type provides a first-class way to refer to "the type being implemented" without repeating complex type names. It reduces verbosity, improves maintainability, and enables cleaner generic implementations. The `$` parameter syntax provides syntactic sugar for `self: Self` with permission modifiers.

##### Guidance (non-normative)

- Use `Self` (and the `$` shorthand) inside record or modal definitions to avoid repeating complex type names; the desugaring in §2.10.2 keeps signatures explicit.
- Outside the defining type’s scope, spell the concrete type name explicitly.

### 2.10.2 Syntax

#### 2.10.2.1 Concrete Syntax

**Self type syntax:**

```ebnf
SelfType ::= "Self"
```

**Self parameter syntax (shorthand):**

```ebnf
SelfParam ::= Permission? "$"

Permission ::= "own" | "mut" | ε
```

**Examples:**

```cantrip
record Counter {
    value: i32

    // Self in return type
    procedure new(initial: i32): own Self {
        own Self { value: initial }
    }

    // Self parameter with $ syntax
    procedure get($): i32 {
        $.value
    }

    procedure increment(mut $) {
        $.value += 1
    }

    procedure consume(own $): i32 {
        $.value
    }

    // Self in parameter type
    procedure add($, other: Self): i32 {
        $.value + other.value
    }
}
```

**Desugaring:**

```cantrip
// $ desugars to self: Self with appropriate permission

procedure get($): i32
// desugars to:
procedure get(self: Self): i32

procedure increment(mut $)
// desugars to:
procedure increment(self: mut Self)

procedure consume(own $): i32
// desugars to:
procedure consume(self: own Self): i32
```

**Explicit type override:**

```cantrip
record Counter {
    value: i32

    // Override Self type when needed
    procedure from_immut($: Counter): i32 {
        $.value
    }

    procedure from_ptr($: Ptr<Counter>@Exclusive): i32
        uses read<Counter>
    {
        $.read().value
    }
}
```

#### 2.10.2.2 Abstract Syntax

**Type representation:**

```
τ ::= Self                           (self type reference)
    | ...

Within context record T { ... }:
  Self ≡ T

Within context record T<α₁, ..., αₙ> { ... }:
  Self ≡ T<α₁, ..., αₙ>
```

**Parameter representation:**

```
param ::= self : π Self              (explicit self parameter)
        | π $                        (self parameter shorthand)

where π ∈ {own, mut, [default]} is permission

Desugaring:
  $        ≡ self : Self
  mut $    ≡ self : mut Self
  own $    ≡ self : own Self
```

**Scope rules:**

```
Self is valid in context Γ iff:
  - Γ contains a type definition: record T { ... } or modal T { ... }
  - Γ contains a trait definition: trait T { ... }
  - Γ contains a contract definition: contract T { ... }

Outside these contexts, Self is undefined.
```

### 2.10.3 Static Semantics

#### 2.10.3.1 Well-Formedness Rules

**[WF-Self] Self type in type definition:**

```
[WF-Self]
record T { ... } in Γ  (or modal T { ... })
Γ ⊢ T : Type
─────────────────────────────────────────────────────
Γ ⊢ Self : Type    (where Self ≡ T)
```

**Explanation:** Within a record or modal definition for type T, `Self` is a well-formed type equivalent to T.

**[WF-Self-Generic] Self type in generic type definition:**

```
[WF-Self-Generic]
record T<α₁, ..., αₙ> { ... } in Γ
Γ, α₁ : Type, ..., αₙ : Type ⊢ T<α₁, ..., αₙ> : Type
─────────────────────────────────────────────────────
Γ ⊢ Self : Type    (where Self ≡ T<α₁, ..., αₙ>)
```

**Explanation:** In generic type definitions, `Self` includes all type parameters.

**[WF-Self-Invalid] Self outside type definition context:**

```
[WF-Self-Invalid]
no record, modal, trait, or contract definition in Γ
─────────────────────────────────────────────────────
Γ ⊬ Self : Type    (ERROR)
```

**Explanation:** `Self` is only valid within type definitions (records, modals, traits, contracts).

**[WF-SelfParam] Self parameter shorthand:**

```
[WF-SelfParam]
record T { procedure m(π $, ...) { ... } } in Γ
π ∈ {own, mut, [default]}
Γ ⊢ Self : Type    (where Self ≡ T)
─────────────────────────────────────────────────────
π $ ≡ self : π Self
Γ, self : π T ⊢ procedure m well-formed
```

**Explanation:** The `$` parameter desugars to `self: Permission Self`.

#### 2.10.3.2 Type Rules

**[T-Self] Self type equivalence:**

```
[T-Self]
record T { ... } in Γ  (or modal T { ... })
Γ ⊢ T : Type
─────────────────────────────────────────────────────
Γ ⊢ Self ≡ T
```

**Explanation:** `Self` is definitionally equal to the enclosing type.

**[T-SelfAccess] Self parameter access:**

```
[T-SelfAccess]
record T { procedure m(π $, ...) { ... body ... } }
Γ, self : π T ⊢ body : U
$ appears in body
─────────────────────────────────────────────────────
Γ ⊢ $ : π T    (where $ denotes self)
```

**Explanation:** Within method body, `$` refers to the self parameter with appropriate permission.

**[T-SelfField] Field access through $:**

```
[T-SelfField]
record T { procedure m($, ...) { ... $.field ... } }
Γ ⊢ T has field field : U
─────────────────────────────────────────────────────
Γ ⊢ $.field : U
```

**Explanation:** Field access through `$` works like any self parameter.

**[T-SelfReturn] Returning Self:**

```
[T-SelfReturn]
record T { procedure m(...): Self { ... return e ... } }
Γ ⊢ e : T
─────────────────────────────────────────────────────
Γ ⊢ return e : Self    (valid)
```

**Explanation:** Expressions of type T can be returned where Self is expected.

**[T-SelfParam-Permission] Permission on self parameter:**

```
[T-SelfParam-Permission]
record T { procedure m(π $, ...) { ... } }
π ∈ {own, mut, [default]}
─────────────────────────────────────────────────────
Γ ⊢ $ : π T
```

**Explanation:** Permission modifiers on `$` determine self's permission.

#### 2.10.3.3 Type Properties

**Theorem 12.1 (Self Type Soundness):**

Within a type definition for type T, all uses of `Self` are equivalent to T and maintain type safety.

**Proof sketch:** By definition, `Self` is a syntactic alias for T within the type definition scope. All type checking proceeds as if T were written directly.

**Theorem 12.2 (Self Parameter Equivalence):**

The `$` parameter syntax is exactly equivalent to explicit `self: Permission Self` syntax.

```
$ ≡ self : Self
mut $ ≡ self : mut Self
own $ ≡ self : own Self
```

**Proof sketch:** By desugaring definition. The compiler transforms `$` to the explicit form before type checking.

**Theorem 12.3 (Self Scope Restriction):**

`Self` is only valid within type definitions (records, modals, traits, contracts). Use outside these contexts is a compile-time error.

**Proof sketch:** The type checker maintains context information about the current type definition. `Self` resolution fails if no type definition context exists in the environment.

**Property 12.1 (Generic Preservation):**

In generic type definitions, `Self` carries all type parameters:

```
record Pair<T, U> { ... }
  Self ≡ Pair<T, U>
```

**Property 12.2 (Permission Independence):**

`Self` can be used with any permission:

```cantrip
own Self    // Owned self type
mut Self    // Mutable self type
Self        // Immutable self type (default)
```

### 2.10.4 Dynamic Semantics

#### 2.10.4.1 Evaluation Rules

**[E-SelfType] Self type resolution:**

```
[E-SelfType]
record T { ... } at compile time
Self appears in expression or type
─────────────────────────────────────────────────────
Self ⇝ T    (compile-time substitution)
```

**Explanation:** `Self` is resolved to the concrete type at compile time. No runtime representation.

**[E-SelfParam] Self parameter evaluation:**

```
[E-SelfParam]
record T { procedure m($, ...) { body } }
obj : T
obj.m(args)
─────────────────────────────────────────────────────
⟨body[$ ↦ obj], σ⟩ ⇓ ⟨v, σ'⟩
```

**Explanation:** The `$` parameter is bound to the receiver object at call time.

**[E-SelfField] Field access through $:**

```
[E-SelfField]
record T { procedure m($, ...) { ... $.field ... } }
obj : T with field : v
⟨$.field, σ[self ↦ obj]⟩
─────────────────────────────────────────────────────
⟨$.field, σ⟩ ⇓ ⟨v, σ⟩
```

**Explanation:** Field access through `$` evaluates to the field value of the self object.

**[E-SelfConstruct] Constructing Self:**

```
[E-SelfConstruct]
record T { procedure new(...): own Self { own Self { fields } } }
⟨own Self { fields }, σ⟩
─────────────────────────────────────────────────────
⟨own T { fields }, σ⟩ ⇓ ⟨T_value, σ'⟩
```

**Explanation:** Constructing `Self` evaluates identically to constructing the concrete type.

#### 2.10.4.2 Memory Representation

**Compile-time only:**

`Self` has **no runtime representation**. It is purely a compile-time alias.

```
Compile-time:
  Self (within record T)

Compile-time substitution:
  Self ⇝ T

Runtime:
  (nothing - Self is erased)
```

**Self parameter representation:**

The `$` parameter compiles to the same representation as any other self parameter:

```cantrip
// Source
record Counter {
    value: i32
    procedure get($): i32 { $.value }
}

// After desugaring
record Counter {
    value: i32
    procedure get(self: Counter): i32 { self.value }
}

// Compiled (pseudo-assembly)
Counter_get:
    mov rdi, [self_ptr]    ; self parameter in first register
    mov eax, [rdi]         ; load value field
    ret
```

**Memory layout (identical to explicit self):**

```
Stack frame for method call:
┌─────────────────────────────┐
│ Return address              │
├─────────────────────────────┤
│ self pointer (rdi/rcx)      │  ← $ points here
├─────────────────────────────┤
│ Other parameters            │
├─────────────────────────────┤
│ Local variables             │
└─────────────────────────────┘
```

#### 2.10.4.3 Operational Behavior

**Self type usage:**

```cantrip
record Vec<T> {
    data: *T
    len: usize
    cap: usize

    // Self ≡ Vec<T>
    procedure new(): own Self {
        own Self { data: null(), len: 0, cap: 0 }
    }

    procedure clone($): own Self {
        // $ : Vec<T>
        // return : own Vec<T>
        own Self {
            data: $.data.clone(),
            len: $.len,
            cap: $.cap,
        }
    }
}

// Usage
let v1 = Vec::<i32>.new()  // Returns own Vec<i32>
let v2 = v1.clone()         // Returns own Vec<i32>
```

**Evaluation trace:**

```
v1.clone()
⇝ Vec<i32>::clone(v1)
⇝ { $ ↦ v1, Self ↦ Vec<i32> }
⇝ own Vec<i32> { data: v1.data.clone(), len: v1.len, cap: v1.cap }
⇝ Vec<i32> { data: [...], len: 5, cap: 8 }
```

## 2.10.6 Notes and Warnings

**Note 1 (Context-Dependent):** Self always refers to the enclosing type definition context (record, enum, trait). It cannot be used outside these contexts.

**Note 2 ($ Shorthand):** The `$` symbol is syntactic sugar for `self: Permission Self`, providing concise method syntax. `$` expands to `self: Self`, `mut $` to `self: mut Self`, etc.

**Note 3 (Generic Self):** In generic types, Self includes all type parameters. In `Stack<T>`, Self means `Stack<T>`, not just `Stack`.

**Note 4 (Constructor Pattern):** Return type `own Self` is idiomatic for constructors, allowing type inference and easier refactoring.

**Note 5 (Trait Self):** In trait definitions, Self represents the type that will attach the trait, enabling generic trait implementations.

**Note 6 (Permission Flexibility):** Self can appear with any permission: `own Self`, `mut Self`, or plain `Self` (immutable).

**Note 7 (Return Self):** Methods returning Self enable method chaining (builder pattern): `obj.method1().method2().method3()`.

**Note 8 (Associated Types):** In traits with associated types, Self can interact with those types: `Self::AssociatedType`.

**Warning 1 (Not a Type Parameter):** Self is NOT a generic type parameter - it's a keyword that refers to the enclosing type. It cannot be constrained with trait bounds directly.

**Warning 2 (Context Required):** Using Self outside of record/enum/trait definitions is a compile error. Self requires an enclosing type context.

**Warning 3 (No Type Alias):** You cannot create type aliases for Self: `type Alias = Self` is invalid because Self's meaning changes based on context.

**Warning 4 (Method Calls):** When calling methods returning Self, the actual concrete type is returned, not some abstract Self type. Type inference resolves Self to the concrete type.

**Performance Note 1 (Zero Cost):** Self is resolved at compile time. There is no runtime overhead compared to using the explicit type name.

**Performance Note 2 (Monomorphization):** For generic types, Self participates in monomorphization - separate code generated for each concrete instantiation.

**Implementation Note 1 ($ Desugaring):** The `$` parameter is purely syntactic sugar. Compilers desugar it to `self: Permission Self` during parsing.

**Implementation Note 2 (IDE Support):** Code editors and IDEs should show the concrete type when hovering over Self for better developer experience.

---
