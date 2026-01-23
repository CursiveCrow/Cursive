# Cursive Language Specification

## Clause 11 — Contracts

**Clause**: 11 — Contracts
**File**: 12-5_Postconditions.md
**Section**: §11.5 Postconditions: will
**Stable label**: [contract.postcondition]  
**Forward references**: §12.2 [contract.sequent], §12.4 [contract.precondition], §12.6 [contract.invariant], §12.7 [contract.checking], §12.8 [contract.verification], Clause 8 [expr], Clause 9 [stmt]

---

### §11.5 Postconditions: will [contract.postcondition]

#### §11.5.1 Overview

[1] _Postconditions_ are boolean predicates that must hold when a procedure returns. They appear in the will clause of contractual sequents and specify _callee guarantees_: conditions the procedure implementation must ensure before returning.

[2] Postconditions enable:

- **Output validation**: Specify properties of return values and mutated parameters
- **Contract-based programming**: Explicit guarantees callers can rely upon
- **Static verification**: Provable postconditions enable optimization
- **Caller assumptions**: Callers may assume postconditions hold after calls
- **Documentation**: Machine-checkable specification of procedure effects

[3] This subclause specifies postcondition syntax, the `result` identifier, the `@old` temporal operator, verification strategies, and integration with contracts and behaviors.

#### §11.5.2 Syntax [contract.postcondition.syntax]

[4] Postcondition syntax appears in the will clause of sequents:

```ebnf
will_clause
    ::= predicate_expression
     | predicate_expression ("," predicate_expression)*
     | "true"

predicate_expression
    ::= expression

expression
    ::= "result" rel_op expression
     | "@old" "(" expression ")" rel_op expression
     | parameter_reference rel_op expression
     | logical_expression
```

[ Note: See Annex A §A.7 [grammar.sequent] and §A.8 [grammar.assertion] for complete grammar.
— end note ]

[5] Postcondition expressions may reference:

- The `result` identifier (procedure's return value)
- The `@old(expression)` operator (pre-state values)
- Procedure parameters (possibly mutated)
- Pure procedure calls
- Logical and comparison operators

[6] Postconditions shall not:

- Perform side effects (must be pure)
- Reference procedure-local bindings
- Nest `@old` operators: `@old(@old(x))` is ill-formed

**Example 12.5.2.1 (Postcondition forms)**:

```cursive
// Return value constraint
procedure absolute_value(x: i32): i32
    [[ |- true => result >= 0 ]]
{
    if x < 0 { result -x } else { result x }
}

// Relationship to input
procedure increment(x: i32): i32
    [[ |- true => result == x + 1 ]]
{
    result x + 1
}

// Using @old for mutation
procedure deposit(~!, amount: i64)
    [[
        ledger::post
        |-
        amount > 0
        =>
        self.balance == @old(self.balance) + amount,
        self.balance >= amount
    ]]
{
    self.balance += amount
}

// Multiple postconditions
procedure clamp(value: i32, min: i32, max: i32): i32
    [[
        |-
        min <= max
        =>
        result >= min,
        result <= max,
        (value >= min && value <= max) => result == value
    ]]
{
    if value < min { result min }
    else if value > max { result max }
    else { result value }
}
```

#### §11.5.3 The result Identifier [contract.postcondition.result]

[7] The `result` identifier is a special binding available exclusively in will clauses. It has the type of the procedure's declared return type and represents the value being returned.

[8] **result typing rule**:

[ Given: Procedure with return type $\tau$ ]

$$
\frac{\text{procedure } f(\ldots): \tau \; \texttt{[[} \cdots \texttt{=>} Q \texttt{]]}}
     {\text{In } Q: \texttt{result} : \tau}
\tag{T-Result}
$$

[9] Using `result` in must clauses is ill-formed (diagnostic E12-007). Using `result` in procedures with return type `()` (unit) is permitted; `result` has type `()` and value `()`.

**Example 12.5.3.1 (result identifier)**:

```cursive
// Numeric result
procedure factorial(n: i32): i32
    [[ |- n >= 0 => result >= 1 ]]
{
    // Implementation
}

// Union result (pattern matching in postcondition)
procedure parse_int(s: string@View): i32 \/ ParseError
    [[
        |-
        s.len() > 0
        =>
        match result {
            value: i32 => true,
            err: ParseError => s.contains_invalid_chars()
        }
    ]]
{
    // Implementation
}

// Unit result
procedure log(message: string@View)
    [[ io::write |- true => result == () ]]
    //                      ^^^^^^^^^^^^^^ Valid but trivial
{
    println("{}", message)
}
```

#### §11.5.4 The @old Temporal Operator [contract.postcondition.old]

[10] The `@old(expression)` operator evaluates `expression` in the procedure's pre-state (at entry, before the body executes) and captures its value for use in postconditions.

[11] **@old evaluation rule**:

[ Given: Procedure call at state $\sigma_{entry}$, body execution produces state $\sigma_{exit}$ ]

$$
\frac{\langle expr, \sigma_{entry} \rangle \Downarrow \langle v, \sigma_{entry} \rangle}
     {\texttt{@old}(expr) \text{ in postcondition} = v}
\tag{E-Old-Capture}
$$

[12] The expression is evaluated once at procedure entry and the resulting value is saved. When the postcondition is evaluated at return, `@old(expr)` refers to the captured value, not the current value.

##### §11.5.4.1 @old Constraints

[13] _Purity requirement._ Expressions within `@old(...)` shall be pure. Effectful expressions produce diagnostic E11-050.

[14] _Parameter scope._ @old expressions may reference parameters and their fields. They shall not reference procedure-local bindings or the `result` identifier. Violations produce diagnostic E11-051.

[15] _No nesting._ `@old` operators shall not nest. `@old(@old(x))` is ill-formed and produces diagnostic E11-009.

[16] _Will clause only._ `@old` is valid only in will clauses. Using it in must clauses produces diagnostic E11-008.

##### §11.5.4.2 @old Semantics

[17] Multiple `@old` expressions are evaluated independently at procedure entry:

**Example 12.5.4.1 (Multiple @old captures)**:

```cursive
procedure swap(a: unique Point, b: unique Point): (Point, Point)
    [[
        |-
        true
        =>
        result.0 == @old(b),
        result.1 == @old(a)
    ]]
{
    result (b, a)
}

// Evaluation:
// 1. Entry: Capture @old(a) → a_entry_value
// 2. Entry: Capture @old(b) → b_entry_value
// 3. Execute body: Swap values
// 4. Exit: Check result.0 == b_entry_value, result.1 == a_entry_value
```

[18] @old expressions may access fields of mutable parameters:

**Example 12.5.4.2 (Field access in @old)**:

```cursive
procedure increment_balance(~!, delta: i64)
    [[
        ledger::post
        |-
        delta > 0
        =>
        self.balance == @old(self.balance) + delta,
        self.balance >= @old(self.balance)
    ]]
{
    self.balance += delta
}

// @old(self.balance) captures the field value at entry
```

#### §11.5.5 Postcondition Verification [contract.postcondition.verification]

##### §11.5.5.1 Static Verification

[19] The compiler attempts to prove postconditions by analyzing all return paths in the procedure body:

$$
\frac{\forall \text{return path } \pi. \; \text{prove}(\pi \Rightarrow Q)}
     {\text{Postcondition } Q \text{ statically verified}}
\tag{Verify-Will-Static}
$$

[20] If all return paths provably satisfy the postcondition, no runtime check is needed.

**Example 12.5.5.1 (Static postcondition proof)**:

```cursive
procedure abs(x: i32): i32
    [[ |- true => result >= 0 ]]
{
    if x < 0 { result -x } else { result x }
}

// Compiler proves both paths:
// Path 1 (x < 0): result = -x, and -x >= 0 when x < 0 ✅
// Path 2 (x >= 0): result = x, and x >= 0 ✅
// No runtime check needed
```

##### §11.5.5.2 Dynamic Verification

[21] When static proof fails, the compiler inserts runtime postcondition checks at return points according to the verification mode (§12.8):

$$
\text{Dynamic check:} \quad \texttt{if } \lnot Q \{ \texttt{panic}(\text{"postcondition violated"}) \}
$$

[22] The check executes immediately before returning from the procedure. If the postcondition evaluates to `false`, execution panics with a diagnostic identifying the violated condition.

**Example 12.5.5.2 (Dynamic postcondition checking)**:

```cursive
procedure complex_calc(x: i32): i32
    [[ |- true => result > 0 ]]
{
    let computed = compute_value(x)
    result computed
    // Runtime check inserted:
    // if !(computed > 0) {
    //     panic("postcondition violated: result > 0")
    // }
}
```

##### §11.5.5.3 Multiple Return Paths

[23] Postconditions must hold on all non-diverging return paths. The compiler verifies each `result` statement and implicit return independently:

**Example 12.5.5.3 (Multiple return paths)**:

```cursive
procedure find_positive(numbers: [i32]): i32 \/ None
    [[
        |-
        numbers.len() > 0
        =>
        match result {
            value: i32 => value > 0,
            _: None => true
        }
    ]]
{
    loop n: i32 in numbers {
        if n > 0 {
            result n  // Check 1: n > 0 ✅
        }
    }
    result None { }   // Check 2: True (always) ✅
}
```

#### §11.5.6 Postconditions in Contracts [contract.postcondition.contracts]

[24] Contract declarations may specify postconditions on abstract procedure signatures. Implementing types must provide implementations whose postconditions are _as strong or stronger_ than the contract's postconditions:

[25] **Postcondition strengthening rule**:

$$
\frac{\text{contract } C \{ \texttt{procedure } m \; \texttt{[[} \cdots \texttt{=>} Q_{contract} \texttt{]]} \} \quad \text{type } T: C \{ \texttt{procedure } m \; \texttt{[[} \cdots \texttt{=>} Q_{impl} \texttt{]]} \}}
     {Q_{impl} \Rightarrow Q_{contract}}
\tag{WF-Will-Strengthen}
$$

[26] The implementation's postcondition must imply the contract's postcondition. Implementations may provide stronger guarantees but not weaker ones. Violations produce diagnostic E11-052.

**Example 12.5.6.1 (Postcondition strengthening)**:

```cursive
public contract PositiveProducer {
    procedure produce(~): i32
        [[ |- true => result > 0 ]]
}

// Valid: Stronger postcondition (better guarantee)
record StrictProducer: PositiveProducer {
    value: i32,

    procedure produce(~): i32
        [[ |- true => result > 10 ]]  // ✅ OK: result > 10 ⇒ result > 0
    {
        result self.value
    }
}

// Invalid: Weaker postcondition
record WeakProducer: PositiveProducer {
    procedure produce(~): i32
        [[ |- true => result >= 0 ]]  // ❌ ERROR: result >= 0 does NOT imply result > 0
    {
        result 0
    }
}
```

#### §11.5.7 Constraints [contract.postcondition.constraints]

[1] _Type requirement._ Predicate expressions in will clauses shall have type `bool`. Non-boolean predicates produce diagnostic E11-053.

[2] _Purity requirement._ Postconditions shall be pure expressions without side effects. Effectful postconditions produce diagnostic E11-054.

[3] _Scope restrictions._ Postconditions may reference parameters, `result`, and `@old` expressions. They shall not reference procedure-local bindings. Violations produce diagnostic E11-055.

[4] _result availability._ The `result` identifier is available in will clauses of procedures with non-unit return types. For procedures returning `()`, `result` is available but has trivial value `()`.

[5] _@old purity._ Expressions within `@old(...)` shall be pure and reference only parameters or parameter fields. Effectful @old expressions produce diagnostic E11-050; references to locals produce diagnostic E11-051.

[6] _No @old nesting._ The `@old` operator shall not contain nested `@old` operators. Violations produce diagnostic E11-009.

[7] _Postcondition coverage._ Postconditions shall be evaluable at all non-diverging exit points: every `result` statement and implicit procedure return. Unreachable code may omit postcondition satisfaction.

#### §11.5.8 Semantics [contract.postcondition.semantics]

##### §11.5.8.1 Guarantee Interpretation

[8] A postcondition $Q$ in sequent `[[ G |- P => Q ]]` provides a guarantee to callers:

$$
\frac{G \text{ available} \quad P \text{ holds at entry} \quad \text{procedure returns}}
     {Q \text{ holds at exit}}
\tag{Guarantee-Will}
$$

[9] After calling a procedure with postcondition $Q$, callers may assume $Q$ holds in the post-call state. This assumption is valid for subsequent static verification and enables compositional reasoning.

##### §11.5.8.2 @old Evaluation Semantics

[10] The `@old(expression)` operator is evaluated according to the following algorithm:

```
evaluate_postcondition_with_old(will_clause, entry_state, exit_state):
    // Phase 1: Capture @old expressions at entry
    old_values = {}
    for each @old(expr) in will_clause:
        value = evaluate(expr, entry_state)
        old_values[@old(expr)] = value

    // Phase 2: Evaluate postcondition at exit
    result_value = procedure_return_value
    postcond = substitute(will_clause, {
        "result" → result_value,
        @old(expr) → old_values[@old(expr)] for all @old in will_clause,
        parameters → exit_state[parameters]
    })

    // Phase 3: Check postcondition
    if evaluate(postcond, exit_state) == true:
        return Success
    else:
        return Failure("postcondition violated")
```

[11] Multiple `@old` expressions are captured independently. Each is evaluated exactly once at procedure entry.

**Example 12.5.8.1 (@old evaluation)**:

```cursive
procedure swap_balances(account1: unique Account, account2: unique Account)
    [[
        ledger::modify
        |-
        account1.balance > 0,
        account2.balance > 0
        =>
        account1.balance == @old(account2.balance),
        account2.balance == @old(account1.balance)
    ]]
{
    let temp = account1.balance
    account1.balance = account2.balance
    account2.balance = temp
}

// Execution:
// 1. Entry: Capture @old(account1.balance) = 100
// 2. Entry: Capture @old(account2.balance) = 200
// 3. Execute: Swap values
// 4. Exit: Verify account1.balance == 200, account2.balance == 100
```

##### §11.5.8.3 Multiple Postconditions

[12] Multiple postconditions are conjoined. All must hold simultaneously:

$$
\texttt{will} \; Q_1, Q_2, \ldots, Q_n \equiv \texttt{will} \; (Q_1 \land Q_2 \land \cdots \land Q_n)
$$

[13] If any postcondition fails verification, the procedure is ill-formed (if unprovable and mode is `static`) or inserts a check that may panic (if mode is `dynamic` or default).

#### §11.5.9 Postconditions on All Return Paths [contract.postcondition.paths]

[14] Procedures with multiple return points must satisfy postconditions on each path:

**Example 12.5.9.1 (Postconditions on multiple paths)**:

```cursive
procedure search(items: [i32], target: i32): i32 \/ None
    [[
        |-
        true
        =>
        match result {
            index: i32 => items[index] == target,
            _: None => true
        }
    ]]
{
    loop i in 0..items.len() {
        if items[i] == target {
            result i              // Path 1: Verify items[i] == target ✅
        }
    }
    result None { }               // Path 2: True (always) ✅
}
```

#### §11.5.10 Diagnostics [contract.postcondition.diagnostics]

[15] Postcondition diagnostics:

| Code    | Condition                                            | Constraint |
| ------- | ---------------------------------------------------- | ---------- |
| E11-009 | Nested `@old` operators                              | [6]        |
| E11-050 | Effectful expression in `@old(...)`                  | [5]        |
| E11-051 | `@old` references local binding (not parameter)      | [5]        |
| E11-052 | Contract implementation has weaker postcondition     | [26]       |
| E11-053 | Postcondition expression has non-bool type           | [1]        |
| E11-054 | Postcondition performs side effects (not pure)       | [2]        |
| E11-055 | Postcondition references local binding               | [3]        |
| E11-056 | Postcondition unprovable in static verification mode | [20]       |

#### §11.5.11 Conformance Requirements [contract.postcondition.requirements]

[16] Implementations shall:

1. Parse will clauses as boolean predicate expressions
2. Provide `result` identifier with procedure return type in will clauses
3. Implement `@old(expression)` operator capturing pre-state values
4. Evaluate @old expressions at procedure entry before body execution
5. Support multiple independent @old captures in single postcondition
6. Enforce type requirement: postconditions must have type `bool`
7. Enforce purity: reject effectful predicates and @old expressions
8. Scope checking: parameters and result accessible, locals inaccessible
9. Reject nested @old operators
10. Verify postconditions on all non-diverging return paths
11. Enforce Liskov substitution: implementation postconditions must strengthen contract postconditions
12. Insert dynamic checks when static proof fails (per verification mode)
13. Emit diagnostics E11-009, E11-050 through E11-056 for violations
14. Provide diagnostic messages showing violated postcondition and return point

---

**Previous**: §11.4 Preconditions: must (§11.4 [contract.precondition]) | **Next**: §11.6 Invariants: where (§11.6 [contract.invariant])
