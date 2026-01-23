# Cursive Language Specification

## Clause 11 — Contracts

**Clause**: 11 — Contracts
**File**: 12-7_Checking-Flow.md
**Section**: §11.7 Sequent Checking Flow
**Stable label**: [contract.checking]  
**Forward references**: §12.3 [contract.grant], §12.4 [contract.precondition], §12.5 [contract.postcondition], §12.8 [contract.verification], Clause 8 [expr], Clause 9 [stmt], Annex E §E.2 [implementation.algorithms]

---

### §11.7 Sequent Checking Flow [contract.checking]

#### §11.7.1 Overview

[1] This subclause specifies the algorithms for verifying contractual sequents at compilation and runtime. Checking occurs at three program points: procedure call sites (grant and precondition checking), procedure entry (precondition validation), and procedure exit (postcondition validation).

[2] The checking flow integrates with:

- Name resolution (Clause 6) for grant path lookup
- Type checking (Clause 7) for predicate expression typing
- Control-flow analysis (Clause 9) for flow-sensitive verification
- Verification modes (§12.8) for determining when to insert runtime checks

[3] This subclause provides normative algorithms that implementations shall follow or refine conservatively (rejecting strictly more programs).

#### §11.7.2 Call-Site Checking [contract.checking.callsite]

##### §11.7.2.1 Overview

[4] When a procedure $f$ is called, the compiler verifies that:

1. **Grant availability**: All grants required by $f$ are available in the calling context
2. **Precondition satisfaction**: The preconditions of $f$ hold for the provided arguments

[5] Grant checking is purely static. Precondition checking may be static or dynamic depending on provability and verification mode.

##### §11.7.2.2 Grant Checking Algorithm

[6] The grant checking algorithm:

```
check_grants_at_call(callee, caller_context):
    required_grants = callee.sequent.grants
    available_grants = caller_context.sequent.grants

    if required_grants ⊆ available_grants:
        return Success
    else:
        missing = required_grants \ available_grants
        return Error(E11-030, {
            callee: callee.name,
            required: required_grants,
            available: available_grants,
            missing: missing
        })
```

[7] **Grant availability rule**:

[ Given: Call to procedure $f$ with grants $G_f$ in procedure $g$ with grants $G_g$ ]

$$
\frac{G_f \subseteq G_g}
     {\text{Call } f() \text{ from } g \text{ permitted}}
\tag{WF-Grant-Available}
$$

[8] If $G_f \not\subseteq G_g$, the call is ill-formed and produces diagnostic E11-030 listing the missing grants.

**Example 12.7.2.1 (Grant checking)**:

```cursive
procedure write_log(message: string@View)
    [[ io::write |- true => true ]]
{ }

procedure process()
    [[ io::write, alloc::heap |- true => true ]]
{
    write_log("Processing...")  // ✅ {io::write} ⊆ {io::write, alloc::heap}
}

procedure invalid()
    [[ alloc::heap |- true => true ]]
{
    write_log("Error")  // ❌ E11-030: missing grant io::write
}
```

##### §11.7.2.3 Precondition Checking Algorithm

[9] The precondition checking algorithm:

```
check_precondition_at_call(callee, args, context):
    precondition = callee.sequent.must
    substituted = precondition[params ↦ args]

    // Attempt static proof
    if can_prove_statically(substituted, context):
        return Success  // No runtime check needed

    // Check verification mode
    match context.verification_mode:
        Static:
            return Error(E11-056, "Cannot prove precondition statically")

        Dynamic:
            insert_assertion(substituted, "precondition violated")
            return Success  // Check inserted

        Trusted:
            return Success  // No check

        Default:
            if debug_build():
                insert_assertion(substituted, "precondition violated")
            // Release: skip if unprovable
            return Success
```

[10] **Precondition verification judgment**:

[ Given: Call `f(args)` with precondition $P$ ]

$$
\frac{\Gamma \vdash P[params \mapsto args]}
     {\text{Precondition statically verified}}
\tag{Verify-Must}
$$

$$
\frac{\Gamma \not\vdash P[params \mapsto args] \quad \text{mode} = \text{dynamic}}
     {\text{Insert check: } \texttt{assert}(P[params \mapsto args])}
\tag{Insert-Must-Check}
$$

#### §11.7.3 Procedure Entry Checking [contract.checking.entry]

[11] At procedure entry, if dynamic precondition checks were inserted, they execute before the procedure body:

```
procedure_entry(callee, args):
    // Evaluate dynamic precondition checks
    for check in callee.dynamic_preconditions:
        if !evaluate(check[params ↦ args]):
            panic("precondition violated: {}", check.source)

    // Capture @old expressions for postconditions
    old_captures = {}
    for @old(expr) in callee.sequent.will:
        old_captures[@old(expr)] = evaluate(expr, current_state)

    // Execute procedure body
    execute_body(callee.body, args, old_captures)
```

[12] Precondition failures panic immediately with diagnostic messages identifying the violated condition and source location.

#### §11.7.4 Procedure Exit Checking [contract.checking.exit]

[13] At procedure exit (all `result` statements and implicit returns), the compiler verifies postconditions:

```
check_postcondition_at_exit(callee, result_value, old_captures, exit_state):
    postcondition = callee.sequent.will

    // Add type invariants if applicable
    if callee.return_type has invariant I:
        postcondition = postcondition && I

    // Substitute result and @old
    substituted = postcondition[
        result ↦ result_value,
        @old(expr) ↦ old_captures[@old(expr)],
        params ↦ exit_state[params]
    ]

    // Attempt static proof
    if can_prove_statically(substituted, callee.body):
        return Success  // No runtime check

    // Dynamic checking per mode
    match callee.verification_mode:
        Static:
            return Error(E11-056, "Cannot prove postcondition statically")

        Dynamic:
            insert_assertion_at_return(substituted, "postcondition violated")
            return Success

        Trusted:
            return Success  // No check

        Default:
            if debug_build():
                insert_assertion_at_return(substituted, "postcondition violated")
            return Success
```

[14] **Postcondition verification judgment**:

[ Given: Procedure return with postcondition $Q$ ]

$$
\frac{\text{All paths: } \text{body} \Rightarrow Q}
     {\text{Postcondition statically verified}}
\tag{Verify-Will}
$$

$$
\frac{\text{Cannot prove } \text{body} \Rightarrow Q \quad \text{mode} = \text{dynamic}}
     {\text{Insert check: } \texttt{assert}(Q) \text{ before return}}
\tag{Insert-Will-Check}
$$

#### §11.7.5 Flow-Sensitive Verification [contract.checking.flow]

[15] The compiler performs flow-sensitive analysis to prove contracts within conditional contexts and after definite assignments:

**Example 12.7.5.1 (Flow-sensitive precondition proof)**:

```cursive
procedure safe_divide(a: i32, b: i32): i32
    [[ |- b != 0 => true ]]
{ result a / b }

procedure caller(x: i32, y: i32): i32 \/ string@View
{
    if y == 0 {
        result "division by zero"
    }

    // Compiler knows: y != 0 in this branch
    result safe_divide(x, y)  // ✅ Proven: y != 0
}
```

[16] Flow-sensitive analysis tracks facts through control flow:

- **Conditional guards**: `if x > 0` establishes `x > 0` in then-branch
- **Pattern matching**: Match arms establish type refinement and value constraints
- **Definite assignment**: Initialized bindings have known values

#### §11.7.6 Compositional Checking [contract.checking.compositional]

[17] Contract checking is compositional: verifying a procedure's contract does not require inspecting called procedures' implementations. The compiler relies on callee contracts:

**Example 12.7.6.1 (Compositional verification)**:

```cursive
procedure validate(x: i32): bool
    [[ |- true => result == (x > 0) ]]
{ result x > 0 }

procedure process(value: i32)
    [[ |- value > 0 => true ]]
{ }

procedure caller(input: i32)
    [[ |- true => true ]]
{
    if validate(input) {
        // Compiler knows from validate's postcondition: input > 0
        process(input)  // ✅ Proven without inspecting validate's body
    }
}
```

[18] The postcondition of `validate` (`result == (input > 0)`) combined with the conditional `if validate(input)` establishes `input > 0`, satisfying `process`'s precondition.

#### §11.7.7 Grant Propagation Through Expressions [contract.checking.propagation]

[19] Grant sets propagate through expressions as specified in §8.1.5. The compiler accumulates grants bottom-up through the expression tree:

$$
\frac{\Gamma \vdash e_1 : \tau_1 \; ! G_1 \quad \Gamma \vdash e_2 : \tau_2 \; ! G_2}
     {\Gamma \vdash (e_1 \texttt{;} e_2) : \tau_2 \; ! (G_1 \cup G_2)}
\tag{Grant-Accumulate}
$$

[20] The accumulated grant set for the procedure body must be a subset of the declared grant clause. Missing grants produce diagnostic E11-030.

**Example 12.7.7.1 (Grant accumulation)**:

```cursive
procedure read_file(path: string@View): string@Managed
    [[ fs::read, alloc::heap |- true => true ]]
{ }

procedure write_file(path: string@View, content: string@View)
    [[ fs::write |- true => true ]]
{ }

procedure copy_file(source: string@View, dest: string@View)
    [[ fs::read, fs::write, alloc::heap |- true => true ]]
{
    let content = read_file(source)     // Requires: {fs::read, alloc::heap}
    write_file(dest, content.view())    // Requires: {fs::write}
    // Accumulated: {fs::read, alloc::heap} ∪ {fs::write}
    //            = {fs::read, fs::write, alloc::heap}
    // Must equal declared grants ✅
}
```

#### §11.7.8 Integration with Definite Assignment [contract.checking.definite]

[21] Precondition checking integrates with definite assignment analysis (§5.7). Conditionals that assign variables enable proving preconditions dependent on those variables:

**Example 12.7.8.1 (Definite assignment integration)**:

```cursive
procedure needs_positive(x: i32)
    [[ |- x > 0 => true ]]
{ }

procedure demo(input: i32)
{
    var validated: i32

    if input > 0 {
        validated = input
        needs_positive(validated)  // ✅ Proven: validated = input > 0
    } else {
        validated = 1
        needs_positive(validated)  // ✅ Proven: validated = 1 > 0
    }
}
```

#### §11.7.9 Diagnostics [contract.checking.diagnostics]

[22] Checking flow diagnostics:

| Code    | Condition                                  | Section |
| ------- | ------------------------------------------ | ------- |
| E11-030 | Call site missing required grants          | [8]     |
| E11-056 | Static verification failed (mode=static)   | [9, 13] |
| E11-070 | Grant accumulation exceeds declared grants | [20]    |

#### §11.7.10 Conformance Requirements [contract.checking.requirements]

[23] Implementations shall:

1. Implement call-site grant checking per §11.7.2 algorithm
2. Implement precondition checking per §11.7.2.3 algorithm
3. Implement postcondition checking per §11.7.4 algorithm
4. Support flow-sensitive analysis for conditional proof contexts
5. Enable compositional verification using callee postconditions
6. Accumulate grant sets through expression evaluation
7. Verify accumulated grants match declared grant clauses
8. Integrate with definite assignment analysis for verification
9. Emit diagnostics E11-030, E11-056, E11-070 for checking failures
10. Provide detailed diagnostic messages showing proof obligations and context

---

**Previous**: §11.6 Invariants: where (§11.6 [contract.invariant]) | **Next**: §11.8 Verification Modes (§11.8 [contract.verification])
