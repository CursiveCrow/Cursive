# Cursive Language Specification

## Clause 8 — Statements and Control Flow

**Part**: VIII — Statements and Control Flow
**File**: 09-4_Evaluation-Order.md  
**Section**: §8.4 Evaluation Order, Sequencing, Short-Circuit, and Divergence  
**Stable label**: [stmt.order]  
**Forward references**: §8.1 [stmt.fundamental], §8.2 [stmt.simple], §8.3 [stmt.control], Clause 7 [expr]

---

### §8.4 Evaluation Order, Sequencing, Short-Circuit, and Divergence [stmt.order]

#### §9.4.1 Statement Sequencing [stmt.order.sequencing]

##### §9.4.1.1 Overview

[1] Statement sequencing defines the order in which statements execute and how control outcomes propagate through sequences. Cursive guarantees deterministic, left-to-right execution.

##### §9.4.1.2 Sequential Execution

[2] Statements in a sequence execute in textual order:

[ Given: Statement sequence $s_1; s_2; \ldots; s_n$ ]

$$
\frac{\langle s_1, \sigma \rangle \Downarrow \text{Normal}(\sigma_1) \quad \langle s_2, \sigma_1 \rangle \Downarrow \text{Normal}(\sigma_2) \quad \cdots \quad \langle s_n, \sigma_{n-1} \rangle \Downarrow \text{Normal}(\sigma_n)}{\langle s_1; s_2; \ldots; s_n, \sigma \rangle \Downarrow \text{Normal}(\sigma_n)}
\tag{E-Seq-All}
$$

[3] Each statement receives the store produced by its predecessor. Side effects from earlier statements are visible to later statements.

##### §9.4.1.3 Early Termination

[4] Control-transfer outcomes bypass remaining statements:

[ Given: Statement $s_i$ producing control-transfer outcome ]

$$
\frac{\langle s_1, \sigma \rangle \Downarrow \text{Normal}(\sigma_1) \quad \cdots \quad \langle s_i, \sigma_{i-1} \rangle \Downarrow \text{outcome} \quad \text{outcome} \ne \text{Normal}}{\langle s_1; \ldots; s_n, \sigma \rangle \Downarrow \text{outcome}}
\tag{E-Seq-Early}
$$

[5] Statements $s_{i+1}$ through $s_n$ are not executed. The control outcome propagates to the enclosing construct.

##### §9.4.1.4 Examples

**Example 9.4.1.1 (Statement sequencing with early exit):**

```cursive
procedure process(data: [i32]): i32
    [[ |- data.len() > 0 => true ]]
{
    let first = data[0]          // Statement 1
    var accumulator = first      // Statement 2
    
    if first < 0 {
        return -1                // Early return: remaining statements skipped
    }
    
    accumulator += 10            // Statement 3 (not executed if first < 0)
    result accumulator           // Statement 4
}
```

#### §9.4.2 Deterministic Execution Order [stmt.order.determinism]

##### §9.4.2.1 Determinism Guarantee

[6] Statement execution order is deterministic and strictly sequential. Unlike some languages with unspecified evaluation order, Cursive guarantees that:

(6.1) Statements execute in the order written in source code.

(6.2) For a given statement, all subexpressions evaluate in the order specified by Clause 8 §8.1.3.

(6.3) Side effects from earlier statements are observable by later statements.

(6.4) No statement reordering, speculation, or parallel execution alters observable behavior.

[7] This determinism guarantee enables local reasoning about program behavior and supports reproducible execution for testing and verification.

##### §9.4.2.2 Interaction with Expression Evaluation

[8] When a statement contains expressions, those expressions evaluate per Clause 8 rules:

- Arithmetic and logical operations follow strict left-to-right evaluation
- Procedure calls evaluate arguments left-to-right
- Short-circuit operators (`&&`, `||`) may skip right operands (§8.3.7)

[9] Statement-level determinism composes with expression-level determinism to provide end-to-end predictability.

##### §9.4.2.3 Examples

**Example 9.4.2.1 (Deterministic side effects):**

```cursive
var counter = 0

procedure increment(): i32
    [[ |- true => true ]]
{
    counter += 1
    result counter
}

procedure demo()
    [[ |- true => true ]]
{
    let a = increment()  // counter = 1, a = 1
    let b = increment()  // counter = 2, b = 2
    let c = increment()  // counter = 3, c = 3
    // Order guaranteed: a=1, b=2, c=3
}
```

#### §9.4.3 Short-Circuit Behavior [stmt.order.shortcircuit]

##### §9.4.3.1 Reference to Clause 8

[10] Short-circuit evaluation for logical operators (`&&`, `||`) is specified in Clause 8 §8.3.7. Those rules apply when such expressions appear in statement contexts.

[11] No additional short-circuit behavior exists at the statement level. Statement sequences always execute until a control-transfer outcome occurs; there is no statement-level "short-circuiting" of sequences.

#### §9.4.4 Divergence and Unreachability [stmt.order.divergence]

##### §9.4.4.1 Divergent Statements

[12] Statements that produce control-transfer outcomes have type `!` (never) because they do not complete normally:

- `return` statements
- `break` statements  
- `continue` statements
- Expressions of type `!` (panic, infinite loops)

[13] Divergent statements may coerce to any type per §7.2.7, enabling their use in contexts expecting specific types.

##### §9.4.4.2 Unreachable Code

[14] Statements following a divergent statement are unreachable:

[ Given: Divergent statement $s_1$ followed by statement $s_2$ ]

$$
\frac{\Gamma \vdash s_1 : !}{\Gamma \vdash s_2 \text{ unreachable}}
\tag{WF-Unreachable}
$$

[15] Implementations should warn about unreachable code (quality of implementation feature). Unreachable code does not affect conformance provided the reachable portions satisfy all normative requirements.

##### §9.4.4.3 Examples

**Example 9.4.4.1 (Unreachable code after return):**

```cursive
procedure example(): i32
    [[ |- true => true ]]
{
    return 42
    // WARNING: Following code is unreachable
    let unused = compute()  // Never executes
}
```

**Example 9.4.4.2 (Divergent loop):**

```cursive
procedure infinite_server()
    [[ io::read |- true => false ]]  // will clause: false (never returns)
{
    loop {
        handle_request()
        // Loop has type ! (never exits)
    }
    // Unreachable: loop never exits
}
```

#### §9.4.5 State Threading

[16] The execution model threads state through statement sequences explicitly. Each statement:

1. Receives the current store $\sigma$
2. Executes, possibly reading and modifying the store
3. Produces an updated store $\sigma'$ (or a control outcome with store)

[17] This explicit state threading ensures that the memory model (Clause 11) and grant system (Clause 12) can reason about program state at each step.

#### §9.4.6 Conformance Requirements

[18] Implementations shall:

1. Execute statements in strict textual order
2. Thread state through statement sequences as specified in §9.4.1
3. Guarantee deterministic execution order for statements and subexpressions
4. Propagate control-transfer outcomes per §9.1.7, bypassing remaining statements
5. Respect short-circuit behavior from Clause 8 for operators in expressions
6. Treat divergent statements as having type `!` and mark following code unreachable
7. Warn about unreachable code (recommended quality-of-implementation feature)
8. Maintain the execution model's semantic consistency with the memory model (Clause 11) and expression evaluation (Clause 8)

---

**Previous**: §9.3 Control Flow Statements (§9.3 [stmt.control]) | **Next**: Clause 10 — Generics and Predicates (§10 [generic])

