# Cursive Language Specification

## Clause 11 — Contracts

**Clause**: 11 — Contracts
**File**: 12-10_Conformance-Diagnostics.md
**Section**: §11.10 Conformance and Diagnostics
**Stable label**: [contract.diagnostics]  
**Forward references**: §11.1-11.9 (all contract subclauses), Annex E §E.5 [implementation.diagnostics]

---

### §11.10 Conformance and Diagnostics [contract.diagnostics]

#### §11.10.1 Overview

[1] This subclause consolidates conformance requirements for the contract system, provides the complete diagnostic code catalog for Clause 11, and specifies integration requirements with other language subsystems.

[2] Conforming implementations shall satisfy all requirements enumerated in §§11.1-11.9 and shall emit the diagnostics defined in this subclause when contract violations occur.

#### §11.10.2 Consolidated Conformance Requirements [contract.diagnostics.conformance]

[3] A conforming implementation of the contract system shall:

**Sequent Support** (§11.2):

1. Parse contractual sequents with semantic brackets `⟦ ⟧` or `[[ ]]`
2. Support complete sequent form: `[[ grants |- must => will ]]`
3. Apply smart defaulting rules for abbreviated forms
4. Recognize turnstile `⊢`/`|-` and implication `⇒`/`=>`
5. Desugar abbreviated sequents deterministically to canonical form

**Grant System** (§11.3): 6. Provide all built-in grants enumerated in §11.3.3 7. Support user-defined grants declared via §4.9 8. Enforce grant visibility rules (public/internal/private) 9. Perform grant checking at call sites using subset inclusion 10. Reject wildcard grant syntax; require explicit grant lists 11. Support grant parameters with substitution and bounds

**Preconditions** (§11.4): 12. Parse must clauses as boolean predicates 13. Enforce caller obligations at call sites 14. Perform static verification when provable 15. Insert dynamic checks when verification mode requires 16. Support flow-sensitive precondition proof

**Postconditions** (§11.5): 17. Parse will clauses with `result` and `@old` support 18. Implement `@old(expression)` operator capturing pre-state 19. Verify postconditions on all non-diverging return paths 20. Insert dynamic checks per verification mode 21. Support multiple independent @old captures

**Invariants** (§11.6): 22. Parse `where` clauses on types and loops 23. Desugar type invariants to postcondition conjunctions 24. Desugar loop invariants to verification points 25. Automatically check invariants at construction and mutation 26. Maintain unified `where` terminology across all contexts

**Checking Flow** (§11.7): 27. Implement grant availability checking algorithm 28. Implement precondition obligation verification 29. Implement postcondition guarantee checking 30. Support compositional verification using callee contracts 31. Integrate with definite assignment analysis

**Verification Modes** (§11.8): 32. Support static, dynamic, trusted modes via attributes 33. Provide build-mode-dependent defaults 34. Enforce static verification requirements (prove or reject) 35. Insert runtime checks in dynamic mode 36. Skip checks in trusted mode 37. Generate witness hooks for dynamic verification

**Grammar** (§11.9): 38. Implement sequent, grant, and predicate grammars per Annex A 39. Support contract declarations with no procedure bodies 40. Integrate contracts with behavior system (Clause 9)

#### §11.10.3 Complete Diagnostic Catalog [contract.diagnostics.catalog]

[4] [Note: All diagnostic codes defined in Clause 11 are cataloged in Annex E §E.5.1.11. — end note]

#### §11.10.4 Diagnostic Payload Requirements [contract.diagnostics.payloads]

[5] Contract diagnostics shall follow payload schemas in Annex E §E.5.3, with the following required fields:

- **Grant diagnostics**: grant identifier, required/available sets
- **Precondition diagnostics**: expression, substituted arguments, violation context
- **Postcondition diagnostics**: expression, result value, @old captures
- **Invariant diagnostics**: expression, violated field or state, construction/mutation site

#### §11.10.5 Integration Summary [contract.diagnostics.integration]

[6] The contract system integrates with the following language subsystems:

**Declarations (Clause 5)**:

- Contractual sequents attach to procedures (§5.4)
- Grant declarations introduce capability tokens (§5.9)
- Contract declarations are type-like bindings

**Type System (Clause 7)**:

- Callable types include grant sets and sequents (§7.4)
- Contract bounds on generic parameters (§7.x)
- Subtyping respects sequent variance

**Expressions (Clause 8)**:

- Grant accumulation through expressions (§8.1.5)
- Predicate expressions type-checked as bool
- result and @old special identifiers

**Generics (Clause 10)**:

- Grant parameters for grant polymorphism (§10.2.5)
- Grant bounds in where clauses (§10.3.7)
- Contract vs behavior distinction (§10.4.1.2)

**Memory Model (Clause 11)**:

- Grant tracking for allocation operations
- Integration with RAII and cleanup
- Permissions orthogonal to contracts

**Witness System (Clause 13)**:

- Dynamic verification generates witnesses
- Polymorphic dispatch via contract witnesses
- Runtime evidence of contract satisfaction

#### §11.10.6 Complete Example [contract.diagnostics.example]

**Example 12.10.6.1 (Comprehensive contract usage)**:

```cursive
// User-defined grants
public grant ledger_read
public grant ledger_write

// Contract declaration
public contract Auditable {
    type AuditLog = string@View

    procedure audit_read(~): Self::AuditLog
        [[ ledger_read |- true => result.len() > 0 ]]

    procedure audit_write(~, entry: Self::AuditLog)
        [[ ledger_write |- entry.len() > 0 => true ]]
}

// Type with invariant implementing contract
record BankAccount: Auditable {
    balance: i64,
    transactions: [Transaction],

    where {
        balance >= 0,
        transactions.len() <= 10_000
    }

    type AuditLog = string@Managed

    procedure audit_read(~): string@Managed
        [[ ledger_read, alloc::heap |- true => result.len() > 0 ]]
    {
        result string.from("audit log")
    }

    procedure audit_write(~, entry: string@Managed)
        [[ ledger_write |- entry.len() > 0 => true ]]
    {
        // Log to audit system
    }

    [[verify(static)]]
    procedure deposit(~!, amount: i64)
        [[
            ledger_write
            |-
            amount > 0
            =>
            self.balance == @old(self.balance) + amount,
            self.balance >= amount
        ]]
    {
        self.balance += amount
        self.audit_write(string.from("deposit"))
        // Invariant automatically checked: balance >= 0, transactions.len() <= 10_000
    }

    [[verify(dynamic)]]
    procedure withdraw(~!, amount: i64): () \/ InsufficientFunds
        [[
            ledger_write
            |-
            amount > 0
            =>
            match result {
                _: () => self.balance == @old(self.balance) - amount,
                _: InsufficientFunds => self.balance == @old(self.balance)
            }
        ]]
    {
        if self.balance >= amount {
            self.balance -= amount
            self.audit_write(string.from("withdrawal"))
            result ()
        } else {
            result InsufficientFunds { requested: amount, available: self.balance }
        }
    }
}

// Grant-polymorphic procedure using contract bound
procedure process_auditable<T, ε>(item: unique T)
    [[
        ε,
        alloc::heap
        |-
        true
        =>
        true
    ]]
    where T: Auditable,
          ε ⊆ {ledger_read, ledger_write}
{
    let log = item.audit_read()
    println("Audit: {}", log)
}
```

#### §11.10.7 Diagnostic Code Summary [contract.diagnostics.summary]

[4] Clause 11 defines diagnostic codes E11-001 through E11-091 covering:

- **Syntax** (001-010): Sequent structure and formatting
- **Grants** (020-032): Grant availability and visibility
- **Preconditions** (040-044): Must clause requirements
- **Postconditions** (050-056): Will clause and @old operator
- **Invariants** (060-069): Where clauses and checking
- **Flow/Verification** (070-091): Checking and verification modes

[5] All diagnostics shall be registered in Annex E §E.5 with:

- Canonical code (E11-[NNN])
- Section reference
- Description
- Severity (all E for Error in current version)
- Structured payload schema

#### §11.10.8 Future Extensions [contract.diagnostics.future]

[6] Future editions of this specification may introduce:

- Quantified predicates (`forall`, `exists`) with verification support
- Contract derivation (automatic contract implementation)
- Refinement types (contracts in type expressions)
- Temporal operators beyond `@old` (e.g., `@eventually`, `@always`)
- Frame conditions (explicit specification of unchanged state)

[7] Such extensions shall be designed to maintain backward compatibility with existing contracts or shall be introduced in new major versions per §1.7 [intro.versioning].

#### §11.10.9 Conformance Testing [contract.diagnostics.testing]

[8] Conformance test suites should verify:

- All smart defaulting forms parse correctly
- Grant checking rejects missing grants
- Precondition violations are detected (statically or dynamically)
- Postcondition violations are detected
- Invariants are checked at all required points
- Verification modes behave as specified
- Contract/behavior distinction is enforced
- All diagnostic codes are emitted for corresponding violations

[9] Test coverage shall include positive cases (valid contracts) and negative cases (each diagnostic code triggered).

#### §11.10.10 Conformance Requirements [contract.diagnostics.requirements]

[10] Implementations claiming conformance to Clause 11 shall:

1. Satisfy all requirements in §11.10.2
2. Emit all diagnostics defined in §11.10.3
3. Provide structured diagnostic payloads per Annex E §E.5.3
4. Integrate contracts with all systems listed in §11.10.5
5. Support the complete grammar in §11.9 and Annex A
6. Document build flags and verification behavior
7. Maintain contract metadata for reflection and tooling
8. Pass conformance tests per §11.10.8

---

**Previous**: §11.9 Grammar Reference (§11.9 [contract.grammar]) | **Next**: Clause 12 — Witness System (§12 [witness])
