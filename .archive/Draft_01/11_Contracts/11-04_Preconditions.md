# Cursive Language Specification

## Clause 11 — Contracts

**Clause**: 11 — Contracts
**File**: 12-4_Preconditions.md
**Section**: §11.4 Preconditions: must
**Stable label**: [contract.precondition]  
**Forward references**: §12.2 [contract.sequent], §12.5 [contract.postcondition], §12.7 [contract.checking], §12.8 [contract.verification], Clause 5 §5.7 [decl.initialization], Clause 8 [expr]

---

### §11.4 Preconditions: must [contract.precondition]

#### §11.4.1 Overview

[1] _Preconditions_ are boolean predicates that must hold when a procedure is called. They appear in the must clause of contractual sequents and specify _caller obligations_: conditions the caller must ensure before invoking the procedure.

[2] Preconditions enable:

- **Input validation**: Specify valid parameter ranges without defensive checks in procedure bodies
- **Contract-based programming**: Explicit division of responsibilities between caller and callee
- **Static verification**: Provable preconditions eliminate runtime checks
- **Safe assumptions**: Procedure implementations may assume preconditions hold
- **Documentation**: Machine-checkable specification of valid inputs

[3] This subclause specifies precondition syntax, well-formedness constraints, verification strategies, and integration with the broader contract system.

#### §11.4.2 Syntax [contract.precondition.syntax]

[4] Precondition syntax appears in the must clause of sequents:

```ebnf
must_clause
    ::= predicate_expression
     | predicate_expression ("," predicate_expression)*
     | "true"

predicate_expression
    ::= expression
```

[ Note: See Annex A §A.7 [grammar.sequent] for complete sequent grammar.
— end note ]

[5] Predicate expressions are boolean-valued expressions (type `bool`) that may reference:

- Procedure parameters (names and fields)
- Pure procedure calls
- Logical operators (`&&`, `||`, `!`, `=>`)
- Comparison operators (`==`, `!=`, `<`, `<=`, `>`, `>=`)
- Arithmetic expressions (for numeric comparisons)
- Quantifiers `forall` and `exists` (optional extension)

[6] Preconditions shall not reference:

- The `result` identifier (use will clause for result properties)
- The `@old` operator (use will clause for temporal properties)
- Mutable operations or side effects
- Procedure-local bindings (parameters only)

**Example 12.4.2.1 (Precondition forms)**:

```cursive
// Simple numeric constraint
procedure sqrt(x: f64): f64
    [[ |- x >= 0.0 => true ]]
{ }

// Multiple preconditions
procedure divide(numerator: i32, denominator: i32): i32
    [[ |- numerator != i32::MIN, denominator != 0 => true ]]
{ result numerator / denominator }

// Relationship between parameters
procedure transfer(from: unique Account, to: unique Account, amount: i64)
    [[
        ledger::post
        |-
        amount > 0,
        from.balance >= amount,
        from.id != to.id
        =>
        true
    ]]
{ }

// Field access in precondition
procedure read_buffer(buffer: Buffer)
    [[ |- buffer.data.len() > 0 => true ]]
{ }
```

#### §11.4.3 Constraints [contract.precondition.constraints]

[1] _Type requirement._ Predicate expressions in must clauses shall have type `bool`. Non-boolean predicates produce diagnostic E12-040.

[2] _Purity requirement._ Preconditions shall be pure expressions: they shall not perform I/O, mutate state, allocate memory, or invoke procedures with non-empty grant sets. Effectful preconditions produce diagnostic E12-041.

[3] _Parameter scope._ Preconditions may reference procedure parameters and their fields but shall not reference procedure-local bindings, the result identifier, or `@old` expressions. Violations produce diagnostics E12-007 (result), E12-008 (@old), or E12-042 (local bindings).

[4] _Well-formedness._ All identifiers and procedure calls in preconditions shall resolve via name lookup (Clause 6). Undefined identifiers produce standard name resolution diagnostics (E06-401).

[5] _Logical form._ Preconditions shall be evaluable as boolean expressions. Implementations may restrict preconditions to decidable subsets for static verification but shall not reject well-typed boolean predicates.

#### §11.4.4 Semantics [contract.precondition.semantics]

##### §11.4.4.1 Caller Obligation

[6] Preconditions represent _caller obligations_. When procedure $f$ has precondition $P$, every call site `f(args)` must ensure $P[params \mapsto args]$ holds.

[7] **Precondition obligation rule**:

[ Given: Procedure $f$ with precondition $P$, call site `f(args)` ]

$$
\frac{\text{procedure } f(x_1: \tau_1, \ldots) \; \texttt{[[} \cdots \texttt{|-} P(x_1, \ldots) \texttt{=>} \cdots \texttt{]]}}
     {\text{At call } f(e_1, \ldots): \text{ must prove } P[x_1 \mapsto e_1, \ldots]}
\tag{WF-Must-Obligation}
$$

[8] The caller must either:

- Prove $P$ statically via compile-time analysis, OR
- Establish $P$ through control flow (conditional guards), OR
- Accept dynamic check insertion (verification mode dependent, §12.8)

##### §11.4.4.2 Static Verification

[9] The compiler attempts to prove preconditions statically using:

- Constant propagation for literal arguments
- Range analysis for bounded integer types
- Control-flow analysis for conditional contexts
- Definite assignment analysis integration
- Optional SMT solver queries

[10] **Static proof rule**:

[ Given: Call `f(args)` with precondition $P$ ]

$$
\frac{\Gamma \vdash P[params \mapsto args] = \text{true}}
     {\text{Precondition statically verified}}
\tag{Verify-Must-Static}
$$

[11] When static proof succeeds, no runtime check is inserted. The call proceeds directly to the procedure body.

**Example 12.4.4.1 (Static verification)**:

```cursive
procedure needs_positive(x: i32)
    [[ |- x > 0 => true ]]
{ }

procedure caller()
{
    needs_positive(42)      // ✅ Proven: 42 > 0

    let y: i32 = 10
    needs_positive(y)       // ✅ Proven: 10 > 0

    let z: i32 = compute()
    needs_positive(z)       // ⚠️ Cannot prove: inserts check
}
```

##### §11.4.4.3 Dynamic Checking

[12] When static proof fails, the compiler inserts a runtime precondition check according to the verification mode (§12.8):

$$
\text{Dynamic check:} \quad \texttt{if } \lnot P[params \mapsto args] \{ \texttt{panic}(\text{"precondition violated"}) \}
$$

[13] The check executes immediately before the procedure call. If the precondition evaluates to `false`, execution panics with a diagnostic message identifying the violated condition.

**Example 12.4.4.2 (Dynamic checking)**:

```cursive
procedure sqrt(x: f64): f64
    [[ |- x >= 0.0 => true ]]
{ }

procedure compute(input: f64)
{
    let result = sqrt(input)
    // Runtime check inserted:
    // if !(input >= 0.0) {
    //     panic("precondition violated: x >= 0.0")
    // }
}
```

##### §11.4.4.4 Control-Flow Sensitive Verification

[14] The compiler performs flow-sensitive analysis to prove preconditions within conditional contexts:

**Example 12.4.4.3 (Flow-sensitive proof)**:

```cursive
procedure divide(a: i32, b: i32): i32
    [[ |- b != 0 => true ]]
{ result a / b }

procedure safe_divide(x: i32, y: i32): i32 \/ string@View
{
    if y != 0 {
        result divide(x, y)  // ✅ Proven in this branch: y != 0
    } else {
        result "division by zero"
    }
}
```

[15] Within the `if y != 0` branch, the compiler knows `y != 0` holds, satisfying the precondition statically.

#### §11.4.5 Multiple Preconditions [contract.precondition.multiple]

[16] Multiple preconditions specified with commas are conjoined. All conditions must hold simultaneously:

$$
\texttt{must} \; P_1, P_2, \ldots, P_n \equiv \texttt{must} \; (P_1 \land P_2 \land \cdots \land P_n)
$$

[17] **Example 12.4.5.1 (Multiple preconditions)**:

```cursive
procedure array_get<T>(array: [T], index: usize): T
    [[
        |-
        index < array.len(),     // Precondition 1
        array.len() > 0,         // Precondition 2 (implied by 1, but explicit)
        T: Copy                  // Precondition 3 (type requirement)
        =>
        true
    ]]
{
    result array[index]
}
```

#### §11.4.6 Preconditions in Contracts [contract.precondition.contracts]

[18] Contract declarations may specify preconditions on abstract procedure signatures. Implementing types must provide implementations whose preconditions are _as weak or weaker_ than the contract's preconditions (Liskov substitution principle):

[19] **Precondition weakening rule**:

$$
\frac{\text{contract } C \{ \texttt{procedure } m \; \texttt{[[} \cdots \texttt{|-} P_{contract} \texttt{=>} \cdots \texttt{]]} \} \quad \text{type } T: C \{ \texttt{procedure } m \; \texttt{[[} \cdots \texttt{|-} P_{impl} \texttt{=>} \cdots \texttt{]]} \}}
     {P_{impl} \Rightarrow P_{contract}}
\tag{WF-Must-Weaken}
$$

[20] The implementation's precondition must imply the contract's precondition. Implementations may accept more inputs (weaker preconditions) but not fewer (stronger preconditions). Violations produce diagnostic E12-043.

**Example 12.4.6.1 (Precondition weakening in contract implementation)**:

```cursive
public contract Validator {
    procedure validate(~, value: i32): bool
        [[ |- value > 0 => true ]]
}

// Valid: Weaker precondition (accepts more inputs)
record LenientValidator: Validator {
    procedure validate(~, value: i32): bool
        [[ |- value >= 0 => true ]]  // ✅ OK: value >= 0 ⇒ value > 0
    {
        result value >= 0
    }
}

// Invalid: Stronger precondition (accepts fewer inputs)
record StrictValidator: Validator {
    procedure validate(~, value: i32): bool
        [[ |- value > 10 => true ]]  // ❌ ERROR: value > 10 does NOT imply value > 0
    {
        result value > 10
    }
}
```

#### §11.4.7 Diagnostics [contract.precondition.diagnostics]

[21] Precondition diagnostics:

| Code    | Condition                                          | Constraint |
| ------- | -------------------------------------------------- | ---------- |
| E12-007 | `result` identifier used in must clause            | [6]        |
| E12-008 | `@old` operator used in must clause                | [6]        |
| E12-040 | Precondition expression has non-bool type          | [1]        |
| E12-041 | Precondition performs side effects (not pure)      | [2]        |
| E12-042 | Precondition references local binding (not param)  | [3]        |
| E12-043 | Contract implementation has stronger precondition  | [20]       |
| E12-044 | Precondition cannot be evaluated (ill-formed expr) | [4]        |

#### §11.4.8 Conformance Requirements [contract.precondition.requirements]

[22] Implementations shall:

1. Parse must clauses as boolean predicate expressions
2. Enforce type requirement: predicates must have type `bool`
3. Enforce purity: reject effectful predicates in must clauses
4. Scope checking: parameters accessible, result/`@old`/locals inaccessible
5. Perform static verification using dataflow analysis and optional SMT solvers
6. Insert dynamic checks when static proof fails (per verification mode §12.8)
7. Support flow-sensitive analysis to prove preconditions in conditional contexts
8. Handle multiple preconditions via conjunction
9. Enforce Liskov substitution: implementation preconditions must weaken contract preconditions
10. Emit diagnostics E12-007, E12-008, E12-040 through E12-044 for violations
11. Provide diagnostic messages showing violated precondition and call site

---

**Previous**: §12.3 Grants (within Sequent) (§12.3 [contract.grant]) | **Next**: §12.5 Postconditions: will (§12.5 [contract.postcondition])
