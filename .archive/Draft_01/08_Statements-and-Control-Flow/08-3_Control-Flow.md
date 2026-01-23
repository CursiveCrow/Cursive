# Cursive Language Specification

## Clause 8 — Statements and Control Flow

**Part**: VIII — Statements and Control Flow
**File**: 09-3_Control-Flow.md  
**Section**: §8.3 Control Flow Statements  
**Stable label**: [stmt.control]  
**Forward references**: §8.1 [stmt.fundamental], §8.2 [stmt.simple], §8.4 [stmt.order], Clause 4 [name], Clause 5 [type], Clause 7 §7.4 [expr.structured]

---

### §8.3 Control Flow Statements [stmt.control]

#### §9.3.1 Conditional Statements [stmt.control.conditional]

##### §9.3.1.1 Overview

[1] Conditional statements execute code selectively based on boolean conditions or pattern matching. They include `if` expressions, `if let` expressions, and `match` expressions when used in statement position.

[2] Conditional expressions are specified in Clause 7 §7.4 [expr.structured]. This subsection specifies their behavior when used as statements: result values are discarded, and execution continues sequentially.

##### §9.3.1.2 Syntax Reference

[3] Conditional statement forms reference Clause 7 §7.4:

- **If expressions** (§8.4.4): `if condition block else? block`
- **If-let expressions** (§8.4.4): `if let pattern = expr block else? block`
- **Match expressions** (§8.4.5): `match expr { arms }`

[ Note: Complete syntax and typing rules are in Clause 7 §7.4 [expr.structured]. This subsection specifies statement-specific execution semantics.
— end note ]

##### §9.3.1.3 Constraints

[4] When conditional expressions are used as statements, they shall be well-formed per Clause 7 §7.4. No additional statement-specific constraints apply.

[5] Result values from conditional statements are discarded. Implementations may warn when conditional expressions producing non-unit values are used as statements (quality-of-implementation feature).

##### §9.3.1.4 Semantics

[6] **Conditional statement execution:**

When a conditional expression is used as a statement, its value is evaluated per Clause 7 and then discarded:

[ Given: Conditional expression $e_{\text{cond}}$ with type $\tau$, store $\sigma$ ]

$$
\frac{\langle e_{\text{cond}}, \sigma \rangle \Downarrow \langle v, \sigma' \rangle}{\langle e_{\text{cond}}, \sigma \rangle \Downarrow \text{Normal}(\sigma')}
\tag{E-Conditional-Stmt}
$$

[7] Side effects from condition evaluation and executed branches persist in the store. The result value $v$ is discarded.

##### §9.3.1.5 Examples

**Example 9.3.1.1 (If statement for side effects):**

```cursive
procedure update_status(should_update: bool, status: unique Status)
    [[ |- true => true ]]
{
    if should_update {
        status.mark_completed()    // Executed if condition is true
    }
    // Result value (unit) discarded; continues to next statement
}
```

**Example 9.3.1.2 (Match statement with multiple branches):**

```cursive
procedure handle_event(event: Event)
    [[ io::write |- true => true ]]
{
    match event {
        Event::KeyPress(key) => println("Key: {}", key),
        Event::MouseClick(x, y) => println("Click at ({}, {})", x, y),
        Event::Quit => println("Quit event"),
    }
    // Match result value discarded; execution continues
}
```

#### §9.3.2 Loop Statements [stmt.control.loop]

##### §9.3.2.1 Overview

[8] Loop statements execute code repeatedly based on conditions or iteration protocols. They include infinite loops, conditional loops, and iterator loops.

[9] Loop expressions are specified in Clause 7 §7.4.6 [expr.structured]. This subsection specifies their behavior when used as statements.

##### §9.3.2.2 Syntax Reference

[10] Loop statement forms reference Clause 7 §7.4.6:

- **Infinite loop**: `loop block`
- **Conditional loop**: `loop condition block`
- **Iterator loop**: `loop pattern: Type in expr block`

[ Note: Complete syntax and typing rules are in Clause 7 §7.4.6 [expr.structured]. This subsection specifies statement-specific execution semantics.
— end note ]

##### §9.3.2.3 Constraints

[11] When loop expressions are used as statements, they shall be well-formed per Clause 7 §7.4.6.

[12] Infinite loops without `break` have type `!` (never). When used as statements, they do not complete normally. Execution never continues past an infinite loop.

[13] Loops with `break` statements or finite iterations have type determined by break values (if valued) or `()` (if unvalued). When used as statements, these types are discarded.

##### §9.3.2.4 Semantics

[14] **Loop statement execution:**

Loops execute per Clause 7 §7.4.6 semantics. When used as statements, result values are discarded:

[ Given: Loop expression $e_{\text{loop}}$, store $\sigma$ ]

$$
\frac{\langle e_{\text{loop}}, \sigma \rangle \Downarrow \langle v, \sigma' \rangle}{\langle e_{\text{loop}}, \sigma \rangle \Downarrow \text{Normal}(\sigma')}
\tag{E-Loop-Stmt}
$$

[15] For infinite loops without breaks:

$$
\frac{\langle \texttt{loop } \{ \ldots \}, \sigma \rangle \Downarrow \bot}{\langle \texttt{loop } \{ \ldots \}, \sigma \rangle \Downarrow \text{Panic} \text{ or divergence}}
\tag{E-Loop-Infinite}
$$

[16] Infinite loops either diverge (server loops) or exit via panic/explicit termination. They never produce Normal outcomes.

##### §9.3.2.5 Examples

**Example 9.3.2.1 (Loop statement for iteration):**

```cursive
procedure process_items(items: [Item])
    [[ io::write |- items.len() > 0 => true ]]
{
    loop item: Item in items {
        println("Processing: {}", item.name)
        transform(item)
    }
    // Loop completes; result (unit) discarded; continues
}
```

**Example 9.3.2.2 (Conditional loop statement):**

```cursive
procedure retry_until_success()
    [[ net::send |- true => true ]]
{
    var attempts = 0
    loop attempts < 10 {
        if try_connect() {
            break  // Exit loop
        }
        attempts += 1
    }
    // Loop exits normally with () type
}
```

**Example 9.3.2.3 (Infinite loop - never returns):**

```cursive
procedure server()
    [[ net::accept, io::write |- true => false ]]  // will: false (never returns)
{
    loop {
        let connection = accept_connection()
        handle_request(connection)
    }
    // Unreachable: loop never exits
}
```

#### §9.3.3 Return Statements [stmt.control.return]

##### §9.3.3.1 Overview

[17] Return statements exit the enclosing procedure immediately, optionally producing a return value. They have type `!` (never) because execution does not continue past a return.

##### §9.3.3.2 Syntax

[18] Return statement syntax:

```ebnf
return_stmt
    ::= "return" expression?
```

[ Note: See Annex A §A.5 [grammar.statement] for complete return grammar.
— end note ]

[19] The expression is optional. When omitted, the return value is `()` (unit).

##### §9.3.3.3 Constraints

[20] **Context requirement.** Return statements shall appear only within procedure bodies. Return at module scope or in `comptime` blocks produces diagnostic E08-201 (return outside procedure).

[21] **Type compatibility.** When a return value is provided, its type shall match the procedure's declared return type. Type mismatches produce diagnostic E08-202.

[22] **Definite return.** Procedures with non-unit return types shall ensure all control-flow paths produce a return value. Missing returns on some paths trigger diagnostic E08-203 (missing return value).

##### §9.3.3.4 Semantics

[23] Return execution:

[ Given: Expression $e$, store $\sigma$ ]

$$
\frac{\langle e, \sigma \rangle \Downarrow \langle v, \sigma' \rangle}{\langle \texttt{return } e, \sigma \rangle \Downarrow \text{Return}(v, \sigma')}
\tag{E-Return}
$$

[24] For unit returns:

$$
\frac{}{\langle \texttt{return}, \sigma \rangle \Downarrow \text{Return}((), \sigma)}
\tag{E-Return-Unit}
$$

[25] Return outcomes propagate through statement sequences and trigger defer execution (§9.2.6) before the procedure exits.

##### §9.3.3.5 Examples

**Example 9.3.3.1 (Early return):**

```cursive
procedure validate(value: i32): i32 \/ ValidationError
    [[ |- value >= 0 => true ]]
{
    if value < 0 {
        result ValidationError::Negative  // Early return with error
    }
    if value > 1000 {
        result ValidationError::TooLarge  // Another early return
    }
    result value  // Normal path
}
```

**Example 9.3.3.2 (Return with defer):**

```cursive
procedure process(): i32
    [[ alloc::heap |- true => true ]]
{
    let resource = acquire_resource()
    defer { release_resource(resource) }

    if should_abort() {
        return -1  // defer executes before return
    }

    result compute(resource)  // defer executes here too
}
```

#### §9.3.4 Break Statements [stmt.control.break]

##### §9.3.4.1 Overview

[26] Break statements exit loops or labeled blocks, optionally producing a value. They enable structured early exit without goto.

##### §9.3.4.2 Syntax

[27] Break statement forms:

```ebnf
break_stmt
    ::= "break" label? expression?

label
    ::= "'" identifier
```

[ Note: See Annex A §A.5 [grammar.statement] for complete break grammar.
— end note ]

[28] Four syntactic forms exist:

- `break` — Exit innermost loop
- `break value` — Exit innermost loop with value
- `break 'label` — Exit labeled construct
- `break 'label value` — Exit labeled construct with value

##### §9.3.4.3 Constraints

[29] **Context requirement.** Break shall appear within a loop or labeled block. Break outside these contexts produces diagnostic E08-211 (break outside loop or block).

[30] **Label resolution.** When a label is specified, it shall refer to a label declared in an enclosing scope within the same procedure. Undefined labels produce diagnostic E08-212.

[31] **Type unification.** When a loop or labeled block contains multiple `break` statements with values, all values shall have the same type or a common supertype. Type mismatches produce diagnostic E08-213.

(31.1) A loop or block with at least one valued `break` has result type equal to the unified break value type. Loops with no valued breaks have type `!` (never) if infinite, or `()` if conditional.

##### §9.3.4.4 Semantics

[32] Break execution:

[ Given: Optional label $\ell$, optional value $v$, store $\sigma$ ]

$$
\frac{\langle e, \sigma \rangle \Downarrow \langle v, \sigma' \rangle}{\langle \texttt{break } \ell \; e, \sigma \rangle \Downarrow \text{Break}(\ell, v, \sigma')}
\tag{E-Break-Value}
$$

[33] For breaks without values:

$$
\frac{}{\langle \texttt{break } \ell, \sigma \rangle \Downarrow \text{Break}(\ell, (), \sigma)}
\tag{E-Break}
$$

[34] Break outcomes propagate outward until reaching the target construct (labeled block/loop or innermost loop if unlabeled).

##### §9.3.4.5 Examples

**Example 9.3.4.1 (Break with label and value):**

```cursive
let result: i32 = 'search: {
    loop i in 0..100 {
        if found(i) {
            break 'search i  // Exit labeled block with value
        }
    }
    result -1  // Not found
}
```

**Example 9.3.4.2 (Multi-level break):**

```cursive
'outer: loop {
    'inner: loop {
        if should_exit_all {
            break 'outer     // Exit both loops
        }
        if should_exit_inner {
            break            // Exit inner loop only
        }
    }
}
```

#### §9.3.5 Continue Statements [stmt.control.continue]

##### §9.3.5.1 Overview

[35] Continue statements skip to the next iteration of a loop, bypassing remaining statements in the current iteration.

##### §9.3.5.2 Syntax

```ebnf
continue_stmt
    ::= "continue" label?
```

[36] Two forms: `continue` (innermost loop) and `continue 'label` (specific loop).

##### §9.3.5.3 Constraints

[37] **Loop context requirement.** Continue shall appear only within loop expressions. Continue outside loops produces diagnostic E08-221.

[38] **Label targets loops.** When labeled, continue shall target a label attached to a loop. Continue targeting a non-loop labeled block produces diagnostic E08-222.

##### §9.3.5.4 Semantics

[39] Continue execution:

[ Given: Optional label $\ell$, store $\sigma$ ]

$$
\frac{}{\langle \texttt{continue } \ell, \sigma \rangle \Downarrow \text{Continue}(\ell, \sigma)}
\tag{E-Continue}
$$

[40] Continue outcomes propagate outward until reaching the target loop, where the next iteration begins.

##### §9.3.5.5 Examples

**Example 9.3.5.1 (Continue in loop):**

```cursive
loop i in 0..10 {
    if i % 2 == 0 {
        continue  // Skip even numbers
    }
    println("{}", i)  // Prints only odd numbers
}
```

**Example 9.3.5.2 (Labeled continue):**

```cursive
'outer: loop row in rows {
    'inner: loop col in row {
        if skip_condition(col) {
            continue 'inner  // Next column
        }
        if row_done_condition(col) {
            continue 'outer  // Next row
        }
        process(col)
    }
}
```

#### §9.3.6 Labeled Statements [stmt.control.labeled]

##### §9.3.6.1 Overview

[41] Labeled statements attach identifiers to loops and blocks, enabling multi-level control transfer via `break 'label` and `continue 'label`.

##### §9.3.6.2 Syntax

[42] Label syntax:

```ebnf
labeled_stmt
    ::= label ":" statement

label
    ::= "'" identifier
```

[ Note: See Annex A §A.5 [grammar.statement] for complete label grammar.
— end note ]

[43] Labels use a leading apostrophe `'` to distinguish them from regular identifiers.

##### §9.3.6.3 Constraints

[44] **Unique labels.** Labels shall be unique within a procedure body. Duplicate labels produce diagnostic E09-231.

[45] **Procedure scope.** Labels have procedure-wide scope (§6.2): they are visible throughout the procedure regardless of nesting. Labels do not escape procedures.

[46] **Attachable constructs.** Labels may attach to:

- Loop expressions (Clause 7 §7.4.6)
- Block statements
- Conditional statements (if, match)

Labels shall not attach to simple statements (assignments, expression statements, variable declarations) or other labeled statements directly.

##### §9.3.6.4 Semantics

[47] Labels do not affect execution directly. They serve as targets for `break` and `continue`:

[ Given: Label $\ell$, statement $s$, store $\sigma$ ]

$$
\frac{\langle s, \sigma \rangle \Downarrow \text{outcome}}{\langle \ell: s, \sigma \rangle \Downarrow \text{outcome}}
\tag{E-Label}
$$

[48] The labeled statement executes identically to the unlabeled form. Labels enable break/continue to reference the construct.

##### §9.3.6.5 Examples

**Example 9.3.6.1 (Label on loop):**

```cursive
'retry: loop {
    if attempt_operation() {
        break 'retry  // Success, exit
    }
    if too_many_attempts() {
        break 'retry  // Give up
    }
    wait_and_retry()
}
```

**Example 9.3.6.2 (Label on conditional for early exit):**

```cursive
'validate: if complex_check(data) {
    if !subcheck_a(data) {
        break 'validate  // Exit the if statement early
    }
    if !subcheck_b(data) {
        break 'validate
    }
    perform_action(data)
}
```

#### §9.3.7 Typed Early-Exit Blocks [stmt.control.earlyexit]

##### §9.3.7.1 Overview

[49] Labeled blocks combined with valued `break` statements enable typed early-exit patterns without goto. The block's type is determined by the break values and result expression.

##### §9.3.7.2 Syntax

[50] Typed early-exit uses labeled blocks with valued breaks:

```cursive
'label: block_expr
// Inside block:
break 'label value
```

[51] The block type is the unified type of all `break 'label value` expressions and the block's `result` expression (if any).

##### §9.3.7.3 Constraints

[52] **Type unification.** All valued breaks targeting the same label shall produce values of compatible types. The block's type is the join of all break value types and the result expression type.

[53] **Consistent usage.** A labeled block should use either all valued breaks or all unit breaks. Mixing valued and unit breaks may produce type inference ambiguity (diagnostic E08-213).

##### §9.3.7.4 Semantics

[54] Break capture at labeled block:

[ Given: Labeled block with break outcomes ]

$$
\frac{\langle b, \sigma \rangle \Downarrow \text{Break}(\ell, v, \sigma') \quad \text{block has label } \ell}{\langle \ell: b, \sigma \rangle \Downarrow \text{Normal}(\sigma') \text{ with result } v}
\tag{E-Block-Capture-Break}
$$

[55] When a break targets the labeled block, the block completes normally with the break value as its result.

##### §9.3.7.5 Examples

**Example 9.3.7.1 (Typed early-exit for validation):**

```cursive
let parsed: Data \/ ParseError = 'parse: {
    let header = read_header(input)
    if header.is_invalid() {
        break 'parse ParseError::InvalidHeader
    }

    let body = read_body(input)
    if body.is_invalid() {
        break 'parse ParseError::InvalidBody
    }

    result Data { header, body }
}
// Type: Data \/ ParseError (unified from breaks and result)
```

**Example 9.3.7.2 (Multi-exit error handling):**

```cursive
let outcome: i32 \/ Error = 'validate: {
    if !check_precondition() {
        break 'validate Error::PreconditionFailed
    }
    if !allocate_resources() {
        break 'validate Error::AllocationFailed
    }
    result perform_computation()  // Returns i32
}
// All exits (breaks and result) unify to i32 \/ Error
```

#### §9.3.8 No Goto Rationale

[56] Cursive intentionally omits unstructured `goto` statements. The combination of:

- Conditional statements (if, match)
- Loop statements (loop, conditional loop, iterator loop)
- Labeled blocks with typed early-exit
- Multi-level break and continue
- Structured expressions (Clause 7)

provides equivalent expressiveness while maintaining:

- Local reasoning (control flow is lexically scoped)
- Verifiable properties (definite assignment, region escape analysis)
- LLM-friendly patterns (structured forms are predictable)
- Zero abstraction cost (labeled break compiles to direct jumps)

[57] Programs requiring goto-like control flow shall use labeled blocks with `break 'label value` for early exit or restructure using the provided control primitives.

#### §9.3.9 Diagnostics

[58] Diagnostics defined in this subclause:

| Code    | Condition                                     |
| ------- | --------------------------------------------- |
| E08-201 | Return statement outside procedure body       |
| E08-202 | Return value type mismatch                    |
| E08-203 | Missing return value on control-flow path     |
| E08-211 | Break statement outside loop or labeled block |
| E08-212 | Break/continue references undefined label     |
| E08-213 | Break values have incompatible types          |
| E08-221 | Continue statement outside loop               |
| E08-222 | Continue targets non-loop label               |
| E09-231 | Duplicate label in procedure                  |

#### §9.3.10 Conformance Requirements

[59] Implementations shall:

1. Support conditional expressions (if, if let, match) as statements per Clause 7 §7.4
2. Support loop expressions (infinite, conditional, iterator) as statements per Clause 7 §7.4.6
3. Discard result values when control-flow expressions are used as statements
4. Support return statements with type checking against procedure return types
5. Execute return statements by producing Return outcomes with defer execution
6. Support break statements with optional labels and values
7. Unify break value types and determine loop/block result types
8. Support continue statements with optional labels
9. Enforce that continue targets only loops (not blocks or conditionals)
10. Implement label resolution with procedure-wide scope per Clause 6
11. Reject duplicate labels within procedures (E09-231)
12. Provide structured control flow only (no goto)
13. Support typed early-exit via labeled blocks with valued breaks

---

**Previous**: §9.2 Simple Statements (§9.2 [stmt.simple]) | **Next**: §9.4 Evaluation Order, Sequencing, and Divergence (§9.4 [stmt.order])
