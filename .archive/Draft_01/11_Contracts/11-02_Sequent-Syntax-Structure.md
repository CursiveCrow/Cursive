# Cursive Language Specification

## Clause 11 — Contracts

**Clause**: 11 — Contracts
**File**: 12-2_Sequent-Syntax-Structure.md
**Section**: §11.2 Sequent: Syntax and Structure
**Stable label**: [contract.sequent]  
**Forward references**: §12.3 [contract.grant], §12.4 [contract.precondition], §12.5 [contract.postcondition], §12.6 [contract.invariant], Clause 5 §5.4 [decl.function], Annex A §A.7 [grammar.sequent]

---

### §11.2 Sequent: Syntax and Structure [contract.sequent]

#### §11.2.1 Overview

[1] A _contractual sequent_ is the complete formal specification attached to a procedure declaration. It uses the form `[[ grants |- must => will ]]` where semantic brackets enclose a sequent calculus judgment expressing capability requirements, preconditions, and postconditions.

[2] This subclause specifies the concrete and abstract syntax of sequents, the smart defaulting rules that allow omitting components, the parsing and desugaring algorithms, and examples of all valid sequent forms.

#### §11.2.2 Syntax [contract.sequent.syntax]

##### §11.2.2.1 Complete Form

[3] The canonical contractual sequent syntax is:

```ebnf
sequent_clause
    ::= "[[" grant_clause "|-" must_clause "=>" will_clause "]]"

grant_clause
    ::= grant_reference ("," grant_reference)*
     | ε

must_clause
    ::= predicate_expression
     | "true"

will_clause
    ::= predicate_expression
     | "true"
```

[ Note: See Annex A §A.7 [grammar.sequent] for complete authoritative grammar.
— end note ]

[4] The semantic brackets `⟦ ⟧` (U+27E6, U+27E7) or their ASCII equivalent `[[ ]]` delimit formal specification content. The turnstile `⊢` (U+22A2) or ASCII `|-` separates the grant context from logical implications. Implication `⇒` (U+21D2) or ASCII `=>` separates preconditions from postconditions.

##### §11.2.2.2 Smart Defaulting Forms

[5] Components may be omitted with smart defaulting rules applied during parsing:

**Grant-only form**:

```ebnf
"[[" grant_clause "]]"
```

Expands to: `[[ grants |- true => true ]]`

**Precondition-only form**:

```ebnf
"[[" must_clause "]]"
```

Expands to: `[[ must => true ]]` (canonical form: `[[ |- must => true ]]`)

**Postcondition-only form**:

```ebnf
"[[" "=>" will_clause "]]"
```

Expands to: `[[ true => will ]]` (canonical form: `[[ |- true => will ]]`)

**Precondition and postcondition (no grants)**:

```ebnf
"[[" must_clause "=>" will_clause "]]"
```

Preferred form without turnstile. Canonical form: `[[ |- must => will ]]`

**Empty sequent**:
[6] Procedures without explicit sequent clauses default to `[[ ∅ |- true => true ]]` (empty grant set, pure procedure with no requirements or guarantees). The canonical internal form is `[[ |- true => true ]]` where the grant set is empty (∅).

##### §11.2.2.3 Examples of All Forms

**Example 12.2.2.1 (Complete form with all components)**:

```cursive
procedure transfer_funds(~!, to: unique Account, amount: i64)
    [[ ledger::post, ledger::validate |-
       amount > 0 && self.balance >= amount && self.id != to.id
       =>
       self.balance == @old(self.balance) - amount &&
       to.balance == @old(to.balance) + amount
    ]]
{
    self.balance -= amount
    to.balance += amount
}
```

**Example 12.2.2.2 (Grant-only form)**:

```cursive
procedure log_message(message: string@View)
    [[ io::write ]]
{
    println("{}", message)
}

// Desugars to: [[ io::write |- true => true ]]
```

**Example 12.2.2.3 (Precondition-only form)**:

```cursive
procedure divide(a: i32, b: i32): i32
    [[ b != 0 ]]
{
    result a / b
}

// Desugars to: [[ b != 0 => true ]]
```

**Example 12.2.2.4 (Postcondition-only form)**:

```cursive
procedure absolute_value(x: i32): i32
    [[ => result >= 0 ]]
{
    if x < 0 { result -x } else { result x }
}

// Desugars to: [[ true => result >= 0 ]]
```

**Example 12.2.2.5 (No turnstile when no grants)**:

```cursive
procedure clamp(value: i32, min: i32, max: i32): i32
    [[ min <= max => result >= min && result <= max ]]
{
    if value < min { result min }
    else if value > max { result max }
    else { result value }
}

// Desugars to: [[ min <= max => result >= min && result <= max ]] (canonical: `[[ |- min <= max => result >= min && result <= max ]]`)
```

**Example 12.2.2.6 (Omitted sequent - pure procedure)**:

```cursive
procedure add(a: i32, b: i32): i32
{
    result a + b
}

// Sequent omitted - defaults to [[ ∅ |- true => true ]] (canonical: `[[ |- true => true ]]` with empty grant set)
```

#### §11.2.3 Constraints [contract.sequent.constraints]

[1] _Semantic bracket requirement._ Contractual sequents shall use semantic brackets `⟦ ⟧` or ASCII `[[ ]]`. Other bracket forms (parentheses, square brackets, curly braces) are not valid for sequent delimiters and shall produce diagnostic E12-001.

[2] _Turnstile placement._ The turnstile `|-` shall separate the grant clause from the must clause. When grants are present, the turnstile is required. When no grants are present, the turnstile may be omitted, allowing the form `[[ must => will ]]` which desugars to `[[ |- must => will ]]`. The turnstile shall appear at most once per sequent. Violations produce diagnostic E12-002.

[3] _Implication placement._ The implication operator `=>` shall separate the must clause from the will clause. It shall appear exactly once per sequent. Violations produce diagnostic E12-003.

[4] _Component ordering._ Sequent components shall appear in the order: grants (optional), turnstile (required if grants present, optional if no grants), must, implication, will. When no grants are present, the form `[[ must => will ]]` is valid and desugars to `[[ |- must => will ]]`. Reordering components is ill-formed and produces diagnostic E12-004.

[5] _Predicate purity._ Expressions in must and will clauses shall be pure: they shall not perform I/O, mutate state, allocate memory, or have observable side effects. Effectful predicates produce diagnostic E12-005.

[6] _Grant reference validity._ All grants referenced in the grant clause shall be visible (§5.9 for user-defined grants, §12.3 for built-in grants). Undefined grants produce diagnostic E12-006.

[7] _Result identifier scope._ The `result` identifier is valid only in will clauses. Using `result` in must clauses produces diagnostic E12-007. The `result` identifier has the type of the procedure's declared return type.

[8] _@old scope._ The `@old(expression)` operator is valid only in will clauses. Using `@old` in must clauses produces diagnostic E12-008. The `@old` operator may not be nested: `@old(@old(x))` is ill-formed (diagnostic E12-009).

[9] _Smart defaulting determinism._ Smart defaulting rules shall be applied deterministically during parsing. The expanded form shall be unique and well-defined for every valid abbreviated sequent. Ambiguous forms are ill-formed (diagnostic E12-010).

#### §11.2.4 Semantics [contract.sequent.semantics]

##### §11.2.4.1 Sequent Interpretation

[10] A contractual sequent `[[ G |- P => Q ]]` has the following semantic interpretation:

$$
\frac{G \text{ available} \quad P \text{ holds at entry}}
     {Q \text{ holds at exit}}
\tag{Sequent-Interpretation}
$$

[11] In prose: "Given that grants $G$ are available in the calling context, if preconditions $P$ hold when the procedure is called, then postconditions $Q$ will hold when the procedure returns."

##### §11.2.4.2 Empty Components

[12] Empty or omitted components have standard interpretations:

- Empty grant clause: $G = \emptyset$ (no capabilities required)
- `must true`: No preconditions (always satisfied)
- `will true`: No postconditions (no guarantees)

[13] The trivial sequent `[[ ∅ |- true => true ]]` (canonical form: `[[ |- true => true ]]` with empty grant set) represents a pure procedure with no requirements and no guarantees. When sequents are omitted entirely, they default to this form. Such procedures may still perform local computation and return values; the sequent indicates only that there are no external requirements or observable guarantees beyond the type signature.

##### §11.2.4.3 Multiple Predicates

[14] Multiple predicates in must or will clauses are conjoined using the `&&` operator:

$$
\texttt{must} \; P_1 \;\&\&\; P_2 \;\&\&\; \cdots \;\&\&\; P_n \equiv \texttt{must} \; P_1 \land P_2 \land \cdots \land P_n
$$

$$
\texttt{will} \; Q_1 \;\&\&\; Q_2 \;\&\&\; \cdots \;\&\&\; Q_n \equiv \texttt{will} \; Q_1 \land Q_2 \land \cdots \land Q_n
$$

[15] The `&&` operator binds less tightly than comparison operators within predicates, so `must a > 0 && b > 0` means `(a > 0) ∧ (b > 0)`, not `a > (0 && b) > 0`.

##### §11.2.4.4 Desugaring Algorithm

[16] The desugaring algorithm for abbreviated sequents proceeds as follows:

```
desugar_sequent(input):
    grants = extract_grants(input) or ∅
    has_turnstile = contains(input, "|-")
    has_implication = contains(input, "=>")

    if has_turnstile:
        before_turnstile = extract_before(input, "|-")
        after_turnstile = extract_after(input, "|-")
        grants = before_turnstile or ∅

        if has_implication:
            before_arrow = extract_before(after_turnstile, "=>")
            after_arrow = extract_after(after_turnstile, "=>")
            must = before_arrow or "true"
            will = after_arrow or "true"
        else:
            must = after_turnstile or "true"
            will = "true"
    else if has_implication:
        before_arrow = extract_before(input, "=>")
        after_arrow = extract_after(input, "=>")
        must = before_arrow or "true"
        will = after_arrow or "true"
    else:
        // Only grants present
        grants = input
        must = "true"
        will = "true"

    return [[ grants |- must => will ]]
```

[17] This algorithm is deterministic and produces a unique canonical form for every valid abbreviated sequent.

#### §11.2.5 Examples [contract.sequent.examples]

**Example 12.2.5.1 (All smart defaulting forms)**:

```cursive
// Form 1: Complete sequent
procedure complete(x: i32)
    [[ io::write |- x > 0 => result >= 0 ]]
{ result x }

// Form 2: Grants only
procedure grants_only()
    [[ fs::read, fs::write ]]
{ }

// Form 3: Precondition only
procedure precond_only(b: i32)
    [[ b != 0 ]]
{ }

// Form 4: Postcondition only
procedure postcond_only(): i32
    [[ => result > 0 ]]
{ result 42 }

// Form 5: Must + will, no grants
procedure no_grants(x: i32): i32
    [[ x >= 0 => result >= x ]]
{ result x + 1 }

// Form 6: Implication without turnstile
procedure implicit_turnstile(n: i32): i32
    [[ n > 0 => result > n ]]
{ result n + 1 }

// Form 7: Omitted entirely (default)
procedure defaulted(a: i32, b: i32): i32
{ result a + b }
```

**Example 12.2.5.2 (Multi-predicate sequents)**:

```cursive
procedure validate_transfer(from: Account, to: Account, amount: i64): bool
    [[
        ledger::validate
        |-
        from.balance >= amount &&
        amount > 0 &&
        from.id != to.id
        =>
        result => from.balance >= amount &&
        result => to.id != from.id
    ]]
{
    result from.balance >= amount && amount > 0 && from.id != to.id
}
```

#### §11.2.6 Integration with Procedure Declarations [contract.sequent.integration]

[18] Contractual sequents attach to procedure declarations as specified in §5.4 [decl.function]. The sequent clause appears after the procedure signature, either on the same line (for simple sequents) or on the following line (recommended for complex sequents).

[19] Expression-bodied procedures (`= expression ;`) shall not include explicit sequents; they implicitly default to `[[ ∅ |- true => true ]]` (empty grant set, canonical: `[[ |- true => true ]]`). Including a sequent with expression bodies produces diagnostic E05-408 (defined in Clause 5).

[20] Extern procedures may include sequents specifying their contract. The sequent serves as documentation and verification obligation for callers; the implementation is external and assumed to satisfy the contract.

**Example 12.2.6.1 (Sequent placement)**:

```cursive
// Single-line sequent (simple)
procedure simple(x: i32): i32 [[ x > 0 => result > x ]]
{ result x + 1 }

// Multi-line sequent (recommended for readability)
procedure complex(~!, data: Buffer, size: usize)
    [[
        alloc::heap, io::write
        |-
        data.capacity() >= size &&
        size > 0
        =>
        self.data.len() >= size &&
        self.data.capacity() >= size
    ]]
{
    // Implementation
}

// Expression-bodied (no explicit sequent)
procedure double(x: i32): i32
= x * 2

// Extern procedure with contract
extern "C" procedure malloc(size: usize): Ptr<u8>@Valid
    [[ ffi::call, unsafe::ptr |- size > 0 => result.is_valid() ]]
```

#### §11.2.7 Formal Sequent Structure [contract.sequent.formal]

[21] The formal abstract syntax for sequents is:

$$
\text{Sequent} ::= G \vdash P \Rightarrow Q
$$

where:

- $G \in \mathcal{P}(\text{Grant})$ is a finite set of grant identifiers
- $P \in \text{Predicate}$ is a boolean-valued expression over parameters
- $Q \in \text{Predicate}$ is a boolean-valued expression over parameters, result, and `@old` expressions

$$
\text{Predicate} ::= \text{true} \mid \text{false} \mid e : \text{bool} \mid P_1 \land P_2 \mid P_1 \lor P_2 \mid \lnot P
$$

$$
\text{Grant} ::= \text{qualified\_grant\_path} \mid \text{grant\_parameter}
$$

[22] Sequent equivalence is defined up to logical equivalence of predicates and set equality of grants:

$$
G_1 \vdash P_1 \Rightarrow Q_1 \equiv G_2 \vdash P_2 \Rightarrow Q_2
$$

$$
\text{iff } G_1 = G_2 \land (P_1 \Leftrightarrow P_2) \land (Q_1 \Leftrightarrow Q_2)
$$

#### §11.2.8 Diagnostics [contract.sequent.diagnostics]

[23] Sequent syntax diagnostics:

| Code    | Condition                                                     | Constraint |
| ------- | ------------------------------------------------------------- | ---------- |
| E12-001 | Invalid sequent brackets (not [[]])                           | [1]        |
| E12-002 | Missing turnstile when grants present, or duplicate turnstile | [2]        |
| E12-003 | Missing or duplicate implication operator                     | [3]        |
| E12-004 | Sequent components in wrong order                             | [4]        |
| E12-005 | Effectful expression in must/will clause                      | [5]        |
| E12-006 | Undefined grant in grant clause                               | [6]        |
| E12-007 | `result` identifier used in must clause                       | [7]        |
| E12-008 | `@old` operator used in must clause                           | [8]        |
| E12-009 | Nested `@old` operators                                       | [8]        |
| E12-010 | Ambiguous abbreviated sequent                                 | [9]        |

#### §11.2.9 Conformance Requirements [contract.sequent.requirements]

[24] Implementations shall:

1. Parse contractual sequents using semantic brackets `⟦ ⟧` or ASCII `[[ ]]`
2. Recognize turnstile `⊢` or ASCII `|-` and implication `⇒` or ASCII `=>`
3. Support all smart defaulting forms listed in §12.2.2.2
4. Apply desugaring algorithm deterministically to produce canonical form
5. Validate sequent component ordering and delimiters
6. Enforce predicate purity (no side effects in must/will)
7. Scope `result` identifier to will clauses only
8. Scope `@old` operator to will clauses only
9. Reject nested `@old` operators
10. Emit diagnostics E12-001 through E12-010 for sequent syntax violations
11. Preserve sequent metadata through compilation for reflection and tooling

---

**Previous**: §12.1 Overview and Model (§12.1 [contract.overview]) | **Next**: §12.3 Grants (within Sequent) (§12.3 [contract.grant])
