# Cursive Language Specification

## Clause 11 — Contracts

**Clause**: 11 — Contracts
**File**: 12-01_Overview-and-Model.md
**Section**: §11.1 Overview and Model
**Stable label**: [contract.overview]  
**Forward references**: §11.2 [contract.sequent], §11.3 [contract.grant], §11.4 [contract.precondition], §11.5 [contract.postcondition], §11.6 [contract.invariant], §11.7 [contract.checking], §11.8 [contract.verification], Clause 4 §4.4 [decl.function], Clause 4 §4.9 [decl.grant], Clause 9 [generic], Clause 12 [witness]

---

### §11.1 Overview and Model [contract.overview]

#### §11.1.1 Overview

[1] The contract system provides mechanisms for specifying and verifying behavioral requirements on procedures and types through _sequents_. A sequent is a formal specification attached to procedure declarations that expresses required capabilities (grants), caller obligations (preconditions), and callee guarantees (postconditions).

[2] Sequents enable grant tracking, interface specification, static and dynamic verification, documentation, and optimization based on proven contract guarantees.

[3] Cursive's contract system uses sequent calculus notation: ⟦ G ⊢ P ⇒ Q ⟧ where G represents capability requirements (grants), P represents preconditions, and Q represents postconditions. Complete syntax and semantics are specified in §11.2 [contract.sequent].

#### §11.1.2 Contracts vs Behaviors [contract.overview.distinction]

[4] Cursive employs two distinct polymorphism mechanisms that serve complementary purposes:

- **Contracts** (this clause): Abstract interface specifications with no implementations
- **Behaviors** (Clause 9 §9.4): Concrete code reuse with default implementations

[5] **For complete comparison of contracts vs behaviors, including syntax, dispatch mechanisms, and use cases, see Table 9.4.1 in §9.4.1.2 [generic.behavior.overview].**

[6] The key distinction: Contracts specify requirements (procedures have NO bodies), while behaviors provide implementations (procedures MUST have bodies). Types may attach both:

```cursive
record Account: Ledgered, Auditable with Serializable, Display {
    // Ledgered, Auditable: contracts (abstract requirements)
    // Serializable, Display: behaviors (concrete code reuse)
}
```

[7] The separation ensures that interface obligations remain distinct from implementation sharing.

#### §11.1.3 Sequent Structure and Components [contract.overview.structure]

[8] A complete sequent has three components:

**Grant component** (before turnstile):
[9] The grant component lists capability tokens that must be available for the procedure to execute. Grants represent permissions to perform observable effects: file I/O, network access, heap allocation, etc. The grant system is specified in detail in §11.3.

**Preconditions** (must clause):
[10] Preconditions specify conditions that must hold when the procedure is called. They represent _caller obligations_: the caller must ensure preconditions are satisfied before invoking the procedure. Preconditions are specified in §11.4.

**Postconditions** (will clause):
[11] Postconditions specify conditions that will hold when the procedure returns normally. They represent _callee guarantees_: the procedure implementation must ensure postconditions are satisfied on all non-diverging exit paths. Postconditions are specified in §11.5.

[12] All three components are optional with smart defaulting (§11.2.2.2):

- Omitted grant component: Empty grant set `∅` (pure procedure)
- Omitted preconditions: `true` (no requirements)
- Omitted postconditions: `true` (no guarantees)
- Entirely omitted sequent: `[[ |- true => true ]]`

[13] Cursive provides `where` clauses for declaring _invariants_: conditions that must always hold at observable program points. Invariants desugar to conjunctions in sequent postconditions.

**Type invariants**:
[14] Type declarations may include `where` clauses specifying type-level invariants. These desugar to implicit postcondition conjunctions on all constructors and mutating methods:

```cursive
record BankAccount {
    balance: i64,

    where {
        balance >= 0
    }

    procedure deposit(~!, amount: i64)
        [[ ledger::post |- amount > 0 => self.balance >= amount ]]
    {
        self.balance += amount
    }
}
```

[15] The `where { balance >= 0 }` invariant automatically extends the postcondition of `deposit` to:

$$
\texttt{will} \; (\texttt{self.balance} \ge \texttt{amount}) \land (\texttt{self.balance} \ge 0)
$$

The compiler inserts the invariant check automatically; programmers need not write it explicitly.

**Loop invariants**:
[16] Loop statements may include `where` clauses specifying loop invariants. These conditions must hold on loop entry, at the start of each iteration, and on loop exit:

```cursive
loop i in 0..array.len() where { i >= 0, i <= array.len() } {
    // Invariant checked: entry, each iteration, exit
}
```

[17] Complete invariant semantics and desugaring rules are specified in §11.6.

#### §11.1.4 Verification and Contract Declarations [contract.overview.verification]

[18] Sequents are verified through a combination of static proof and dynamic checking:

**Static verification** (compile-time):
[19] The compiler attempts to prove contracts using dataflow analysis, symbolic execution, and optional SMT solvers. Provable contracts are verified at compile time with zero runtime overhead.

**Dynamic verification** (runtime):
[20] Contracts that cannot be proven statically are converted to runtime assertions. The verification mode (§11.8) controls when dynamic checks are inserted: always, debug-only, release-proven-only, or never.

**Verification modes**:
[21] Procedures may specify verification requirements via attributes:

- `[[verify(static)]]` — Must prove statically or compilation fails
- `[[verify(dynamic)]]` — Always insert runtime checks
- `[[verify(trusted)]]` — Trust contract, no checks (programmer responsibility)
- No attribute — Build-mode default (debug inserts checks, release only proven)

[22] Verification modes are metadata (not part of sequent syntax) and do not affect procedure type signatures. Two procedures with identical sequents but different verification modes have the same type.

[23] In addition to sequents on procedures, Cursive supports _contract declarations_: abstract interfaces that types may implement for polymorphism. Contract declarations define procedure signatures without implementations:

```cursive
public contract Serializable {
    type Format = string@View

    procedure serialize(~): Self::Format
        [[ alloc::heap |- true => result.len() > 0 ]]

    procedure deserialize(data: Self::Format): Self \/ ParseError
        [[ alloc::heap |- data.len() > 0 => true ]]
}
```

[24] Types attach contracts using `:` syntax in the type declaration header:

```cursive
record User: Serializable {
    id: u64,
    name: string@Managed,

    type Format = string@Managed

    procedure serialize(~): string@Managed
        [[ alloc::heap |- true => result.len() > 0 ]]
    {
        // Implementation must satisfy contract
    }

    procedure deserialize(data: string@Managed): Self \/ ParseError
        [[ alloc::heap |- data.len() > 0 => true ]]
    {
        // Implementation
    }
}
```

[25] Contract declarations, satisfaction rules, and coherence are integrated with the behavior system defined in Clause 9. This clause focuses on the sequent notation and verification; polymorphic dispatch is specified in Clause 12 (Witness System).

#### §11.1.5 Conformance Requirements [contract.overview.requirements]

[26] Implementations shall maintain contract metadata through compilation phases for reflection and tooling, and integrate contracts with the type system, generics, and witness system.

---

**Previous**: Clause 10 — Memory Model, Regions, and Permissions (§10.8 [memory.unsafe]) | **Next**: §11.2 Sequent: Syntax and Structure (§11.2 [contract.sequent])
