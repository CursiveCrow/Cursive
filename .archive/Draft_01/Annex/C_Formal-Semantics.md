# Cursive Language Specification

## Annex C — Formal Semantics (Informative)

**Part**: Annex
**Section**: Annex C - Formal Semantics
**Stable label**: [formal]
**Forward references**: None

---

### §C.1 Type System Rules [formal.type]

[1] This annex shall provide a complete formalization of the type system using mathematical notation and inference rules. It will consolidate all typing judgments from Clause 7 and prove key meta-theoretic properties.

[ Note: This section is reserved for the complete formal type system specification.

Planned content includes:
- Typing judgments for all expression forms
- Type equivalence and subtyping relations  
- Well-formedness rules for all type constructors
- Kinding rules for higher-level types
- Proof of type system soundness
— end note ]

---

### §C.2 Operational Semantics [formal.operational]

[2] This annex shall provide small-step and big-step operational semantics for expression and statement evaluation, formalizing the execution model described informally in Clauses 8 and 9.

[ Note: This section is reserved for formal operational semantics.

Planned content includes:
- Small-step reduction rules ($\langle e, \sigma \rangle \to \langle e', \sigma' \rangle$)
- Big-step evaluation rules ($\langle e, \sigma \rangle \Downarrow \langle v, \sigma' \rangle$)
- Store and environment models
- Control outcome propagation
- Proof of deterministic evaluation
— end note ]

---

### §C.3 Denotational Semantics [formal.denotational]

[3] This annex shall provide denotational semantics mapping Cursive programs to mathematical domains, enabling equational reasoning about program behavior.

[ Note: This section is reserved for denotational semantics.
— end note ]

---

### §C.4 Permission and Move Semantics [formal.permission]

[4] This annex shall formalize the permission system, move semantics, and binding responsibility tracking described in Clause 11.

[ Note: This section is reserved for formal permission semantics.

Planned content includes:
- Permission lattice formalization
- Cleanup responsibility tracking
- Move semantics and invalidation rules
- Non-responsible binding validity conditions
- Proof of memory safety properties
— end note ]

---

### §C.5 Witness Semantics [formal.witness]

[5] This annex shall formalize the witness system's dynamic dispatch semantics, vtable construction, and type erasure properties described in Clause 13.

[ Note: This section is reserved for formal witness semantics.
— end note ]

---

### §C.6 Contract and Sequent Semantics [formal.contract]

[6] This annex shall formalize the sequent calculus model, grant propagation, and contract verification described in Clause 12.

[ Note: This section is reserved for formal contract semantics.

Planned content includes:
- Sequent calculus formalization
- Grant subsumption and composition rules
- Precondition and postcondition entailment
- Invariant desugaring formalization
- Verification soundness proofs
— end note ]

---

### §C.7 Meta-Theory [formal.metatheory]

[7] This annex shall prove key meta-theoretic properties of the Cursive language: progress (well-typed programs don't get stuck), preservation (evaluation preserves types), and soundness (well-typed programs don't exhibit undefined behavior).

[ Note: This section is reserved for meta-theoretic proofs.

Planned theorems:
- **Progress**: If $\Gamma \vdash e : \tau$, then either $e$ is a value or there exists $e'$ such that $\langle e, \sigma \rangle \to \langle e', \sigma' \rangle$
- **Preservation**: If $\Gamma \vdash e : \tau$ and $\langle e, \sigma \rangle \to \langle e', \sigma' \rangle$, then $\Gamma \vdash e' : \tau$
- **Soundness**: Well-typed programs that do not use unsafe code cannot produce undefined behaviors enumerated in Annex B §B.2
- **Memory Safety**: Well-typed programs cannot produce use-after-free, double-free, or null-pointer dereference errors
- **Data Race Freedom**: Well-typed concurrent programs without unsafe code are data-race-free
— end note ]

---

**Previous**: Annex B — Behavior Classification ([behavior]) | **Next**: Annex D — Implementation Limits ([limits])

