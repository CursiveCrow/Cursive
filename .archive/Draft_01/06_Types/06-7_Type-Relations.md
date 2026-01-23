# Cursive Language Specification

## Clause 6 — Types

**Part**: VI — Type System  
**File**: 07-7_Type-Relations.md
**Section**: §6.7 Type Relations  
**Stable label**: [type.relation]  
**Forward references**: Clause 7 [expr], Clause 9 [generic], Clause 10 [memory], Clause 11 [contract]

---

### §6.7 Type Relations [type.relation]

[1] This subclause defines the core relations on types: type equivalence (`≡`), subtyping (`<:`), and compatibility. These relations govern implicit conversions, variance, and the conditions under which values may be substituted for one another.

[2] Type equivalence is an equivalence relation used to normalize types (e.g., expanding aliases). Subtyping is a preorder determining assignability. Compatibility captures auxiliary checks (e.g., state compatibility for modal types) used during type inference and diagnostics.

#### §7.7.1 Syntax [type.relation.syntax]

```
EquivalenceJudgment ::= TypeExpr "≡" TypeExpr
SubtypingJudgment   ::= TypeExpr "<:" TypeExpr
CompatibilityJudgment ::= TypeExpr "~" TypeExpr
```

A judgment is well-formed only if both operands are well-formed types in the current type environment Γ.

$$
\dfrac{\Gamma \vdash \tau : \text{Type} \quad \Gamma \vdash \upsilon : \text{Type}}{\Gamma \vdash (\tau \prec \upsilon) \text{ wf}}
\tag{WF-Relation}
$$

Where `τ ≺ υ` stands for any of the three relations.

#### §7.7.2 Type Equivalence [type.relation.equivalence]

[3] **Definition 7.7.1 (Equivalence).** Two types `τ₁` and `τ₂` are equivalent (`τ₁ ≡ τ₂`) when they denote the same type after expanding aliases, substituting generic arguments, normalizing associated types, and erasing syntactic sugar.

[4] Equivalence is reflexive, symmetric, and transitive:

$$
\dfrac{}{\tau \equiv \tau}
\tag{Equiv-Refl}
$$

$$
\dfrac{\tau_1 \equiv \tau_2}{\tau_2 \equiv \tau_1}
\tag{Equiv-Sym}
$$

$$
\dfrac{\tau_1 \equiv \tau_2 \quad \tau_2 \equiv \tau_3}{\tau_1 \equiv \tau_3}
\tag{Equiv-Trans}
$$

[5] Type aliases are transparent:

$$
\dfrac{\text{type } A = \tau}{A \equiv \tau}
\tag{Equiv-Alias}
$$

$$
\dfrac{\text{type } F\langle\alpha_1, \ldots, \alpha_n\rangle = \tau[\alpha]}{F\langle\upsilon_1, \ldots, \upsilon_n\rangle \equiv \tau[\alpha_i \mapsto \upsilon_i]}
\tag{Equiv-Generic-Alias}
$$

[6] Structural types compare element-wise:

$$
\dfrac{\forall i.\, \tau_i \equiv \upsilon_i}{(\tau_1, \ldots, \tau_n) \equiv (\upsilon_1, \ldots, \upsilon_n)}
\tag{Equiv-Tuple}
$$

$$
\dfrac{\tau \equiv \upsilon \quad n = m}{[\tau; n] \equiv [\upsilon; m]}
\tag{Equiv-Array}
$$

$$
\dfrac{\tau \equiv \upsilon}{[\tau] \equiv [\upsilon]}
\tag{Equiv-Slice}
$$

$$
\dfrac{\tau_1 \equiv \upsilon_1 \; \cdots \; \tau_n \equiv \upsilon_n \quad \tau_r \equiv \upsilon_r \quad \varepsilon = \varepsilon'}{(\tau_1, \ldots, \tau_n) \to \tau_r ! \varepsilon \; \texttt{[[} \; \texttt{|-} \; M \; \texttt{=>} \; W \; \texttt{]]} \equiv (\upsilon_1, \ldots, \upsilon_n) \to \upsilon_r ! \varepsilon' \; \texttt{[[} \; \texttt{|-} \; M \; \texttt{=>} \; W \; \texttt{]]}}
\tag{Equiv-Function}
$$

$$
\dfrac{\text{State sets identical}}{\text{Ptr}\langle\tau\rangle@S \equiv \text{Ptr}\langle\upsilon\rangle@S \quad \text{iff } \tau \equiv \upsilon}
\tag{Equiv-Ptr}
$$

$$
\dfrac{\text{Same state names and payloads}}{M@S \equiv M@S}
\tag{Equiv-Modal-State}
$$

[7] Nominal types (records, enums, modals, behaviors) are equivalent only if they reference the same declaration. Structural types (tuples, arrays, slices, unions, function types, pointers) use component-wise equivalence.

#### §7.7.3 Subtyping [type.relation.subtyping]

[8] **Definition 7.7.2 (Subtyping).** `τ <: υ` indicates that any value of type `τ` may substitute for `υ` without violating safety guarantees. Subtyping is reflexive and transitive:

$$
\dfrac{}{\tau <: \tau}
\tag{Sub-Refl}
$$

$$
\dfrac{\tau_1 <: \tau_2 \quad \tau_2 <: \tau_3}{\tau_1 <: \tau_3}
\tag{Sub-Trans}
$$

[9] Equivalence implies subtyping:

$$
\dfrac{\tau \equiv \upsilon}{\tau <: \upsilon}
\tag{Sub-Equiv}
$$

[10] Derived rules per type family:

- **Primitive:** Only equality; no widening beyond equivalence.
- **Tuples/Arrays/Slices:** Invariant (component-wise equivalence required).
- **Records/Enums/Modals:** Nominal, invariant.
- **Function types:** Contravariant in parameters, covariant in result, contravariant in grants and preconditions, covariant in postconditions (see §7.4.3.5).
- **Pointers:** State hierarchy `Ptr<T>@S <: Ptr<T>`; specific states do not subtype each other except through widening to the unconstrained type.
- **Unions:** `τ_i <: τ_1 \/ \cdots \/ τ_n`. Conversely, a union subtypes another union if every component of the source appears in the target (`{τ}` subset containment).
- **Never type:** `! <: τ` for all τ (§7.2.7).

Formal union subtyping:

$$
\dfrac{}{\tau_i <: \tau_1 \/ \cdots \/ \tau_n}
\tag{Sub-Union-Intro}
$$

$$
\dfrac{\forall i.\, \exists j.\, \tau_i \equiv \upsilon_j}{\tau_1 \/ \cdots \/ \tau_m <: \upsilon_1 \/ \cdots \/ \upsilon_n}
\tag{Sub-Union-Contain}
$$

Pointer widening:

$$
\dfrac{}{\text{Ptr}\langle\tau\rangle@S <: \text{Ptr}\langle\tau\rangle}
\tag{Sub-Ptr-Widen}
$$

Modal widening:

$$
\dfrac{}{M@S <: M}
\tag{Sub-Modal-Widen}
$$

#### §7.7.4 Variance Summary [type.relation.variance]

[11] Table 7.7.1 summarizes variance:

| Constructor               | Variance notes                                                                        |
| ------------------------- | ------------------------------------------------------------------------------------- |
| `τ` (primitive)           | invariant (only equality)                                                             |
| `(τ₁, …, τₙ)`             | invariant per component                                                               |
| `[τ; n]`, `[τ]`           | invariant                                                                             |
| `record`/`enum`/`modal`   | nominal, invariant                                                                    |
| `(τ₁,…,τₙ) -> τ_ret`      | contravariant parameters, covariant result, contravariant grants/must, covariant will |
| `Ptr<T>@State`            | invariant in `T`, but state-specific types widen to `Ptr<T>`                          |
| `τ₁ \/ … \/ τₙ`           | covariant in every component                                                          |
| `Option<T>` (example ADT) | follows enum rules (nominal)                                                          |

Manifesting a variance rule requires referencing the appropriate constructor rule (e.g., `Sub-Function`, `Sub-Union-Contain`).

#### §7.7.5 Compatibility [type.relation.compatibility]

[12] Some operations require compatibility checks beyond subtyping. Compatibility is denoted `τ ~ υ` and is used for:

- Matching pointer states (e.g., `Ptr<T>@Valid ~ Ptr<T>`).
- Matching modal states in pattern bindings.
- Ensuring binary operators operate on types with shared promotion rules (Clause 8).

Compatibility is reflexive and symmetric, but not necessarily transitive.

Example compatibility rules:

$$
\dfrac{}{\text{Ptr}\langle\tau\rangle@S ~ \text{Ptr}\langle\tau\rangle}
\tag{Compat-Ptr}
$$

$$
\dfrac{}{(\tau_1, \tau_2) ~ (\tau_1, \tau_2)}
\tag{Compat-Tuple}
$$

Compatibility failures produce diagnostics (Table 7.7.2).

#### §7.7.6 Diagnostics [type.relation.diagnostics]

[13] Required diagnostics:

| Code    | Condition                                                     |
| ------- | ------------------------------------------------------------- |
| E07-700 | Failed subtyping check (reports source/target, variance info) |
| E07-701 | Cyclic type alias detected                                    |
| E07-702 | Incompatible types in operation (compatibility failure)       |
| E07-703 | Pattern match missing modal state branch                      |
| E07-704 | Union pattern missing component type                          |

Each diagnostic SHALL provide the offending types, the rule violated, and suggestions (e.g., insert cast, add missing branch).

#### §7.7.7 Examples (Informative) [type.relation.examples]

```cursive
// Tuple invariance
let pair: (i32, string@Managed) = (1, string.from("hi"))
// let wider: (i32, string@View) = pair  // ERROR E07-700: tuple invariant

// Function subtyping
let pure: (i32) -> i32 = |x| [[ |- true => true ]] { result x }
let grant_fn: (i32) -> i32 ! { io::write } [[ |- true => true ]] = pure

// Union widening
let value: i32 \/ string@Managed = 42
let union: i32 \/ string@Managed \/ bool = value  // OK: subset containment

// Modal widening
procedure fetch(conn: Connection) {
    match conn {
        @Open => println("open"),
        @Closed => println("closed"),
    }
}

// Pointer state refinement
procedure use(ptr: Ptr<Data>) {
    match ptr {
        @Valid => println("value {}", (*ptr).id),
        @Null => println("missing"),
        _ => println("unusable"),
    }
}
```

#### §7.7.8 Conformance Requirements [type.relation.requirements]

[14] Implementations shall:

1. Implement equivalence, subtyping, and compatibility exactly as specified, maintaining reflexivity/transitivity properties.
2. Expand type aliases during equivalence checks and detect alias cycles (E07-701).
3. Enforce variance rules per constructor, rejecting invalid subtyping conversions with E07-700 diagnostics that include variance context.
4. Integrate pointer and modal state widening with provenance tracking to ensure state-sensitive operations are safe.
5. Report compatibility failures with E07-702/E07-703/E07-704, pinpointing missing cases.
6. Propagate relation results through the compiler (type inference, code generation, diagnostics) so downstream phases do not contradict type judgments.

[15] Any deviation from these requirements renders an implementation non-conforming unless explicitly noted elsewhere as implementation-defined.

---

**Previous**: §7.6 Modal Types (§7.6 [type.modal]) | **Next**: §7.8 Type Introspection (§7.8 [type.introspection])
