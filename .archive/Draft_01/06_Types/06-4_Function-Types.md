# Cursive Language Specification

## Clause 6 — Types

**Part**: VI — Type System  
**File**: 07-4_Function-Types.md
**Section**: §6.4 Function Types (Callable Types)  
**Stable label**: [type.function]  
**Forward references**: §6.5 [type.pointer], §6.7 [type.relation], Clause 7 [expr], Clause 8 [stmt], Clause 9 [generic], Clause 10 [memory], Clause 11 [contract], Clause 12 [witness]

---

### §6.4 Function Types [type.function]

[1] Function types (also called callable types) describe procedure declarations. They record parameter types, return type, required grants, and contractual sequents. Callable types carry this information wherever they appear (variable bindings, record fields, generic arguments), ensuring that higher-order programming preserves grant and contract obligations.

[2] Cursive uses a single arrow `->` for all callable types. Purity is indicated by an empty grant set and the trivial contractual sequent `[[ ∅ |- true => true ]]` (canonical: `[[ |- true => true ]]`). When sequents are omitted, they default to this form. Grant-requiring callables list their grants explicitly. Procedures with receivers desugar to ordinary callable types whose first parameter is the receiver pointer annotated with the appropriate permission (§7.4.3.4).

[2.1] **Transition types**: Modal type transitions use a special form of function type syntax `@Source -> @Target` that is syntactic sugar for `(Self@Source, ...params) -> Self@Target`, where `Self` is the modal type and `@State` desugars to `Self@State` in modal scope. Transition types are first-class function types and can be bound and passed as parameters. See §7.6.4 for details.

#### §7.4.1 Syntax [type.function.syntax]

[3] Callable type syntax follows (authoritative grammar: Annex A §A.3):

```
FunctionType   ::= '(' ParamTypes? ')' '->' Type GrantClause? SequentClause?
ParamTypes     ::= Type (',' Type)*
GrantClause    ::= '!' GrantSet
SequentClause  ::= '[[' SequentBody ']]'
SequentBody    ::= '|-' MustClause '=>' WillClause
MustClause     ::= PredicateExpr (',' PredicateExpr)* | 'true'
WillClause     ::= PredicateExpr (',' PredicateExpr)* | 'true'
```

[ Note: The token `|-` is the ASCII representation of the mathematical turnstile symbol `⊢` (U+22A2). Lexers shall recognize `|-` as a single two-character token used exclusively in contractual sequents. This ensures the sequent syntax `[[ grants |- must => will ]]` is parsed consistently.
— end note ]

[4] An omitted `GrantClause` denotes the empty grant set. An omitted `SequentClause` is equivalent to `[[ ∅ |- true => true ]]` (empty grant set, canonical: `[[ |- true => true ]]`). Parameters may be absent (`()`), yielding a nullary callable. Tuples are used to encode variadic parameters after desugaring (§9.3).

#### §7.4.2 Formation Rules [type.function.formation]

[5] Callable types are well-formed when each parameter and the result type are well-formed, the grant set is valid (§12.2), and the contractual sequent predicates are type-correct:

$$
\dfrac{\Gamma \vdash \tau_1 : \text{Type} \; \cdots \; \Gamma \vdash \tau_n : \text{Type} \quad \Gamma \vdash \tau_{\text{ret}} : \text{Type} \quad \varepsilon \text{ valid}}{\Gamma \vdash (\tau_1, \ldots, \tau_n) \to \tau_{\text{ret}} ! \varepsilon : \text{Type}}
\tag{WF-Function}
$$

[6] Contractual sequents must be well-formed under the same environment:

$$
\dfrac{\Gamma \vdash \text{Must predicates valid} \quad \Gamma \vdash \text{Will predicates valid}}{\Gamma \vdash \texttt{[[} \; \texttt{|-} \; \text{Must} \; \texttt{=>} \; \text{Will} \; \texttt{]]} \text{ ok}}
\tag{WF-Sequent}
$$

[7] Callable literals (closures) and declarations are checked against these types. Generic callable types follow §10.2; grant parameters (if any) are handled via contractual sequent quantification (§12.4).

#### §7.4.3 Semantics [type.function.semantics]

##### §7.4.3.1 Evaluation Model [type.function.semantics.eval]

[8] A callable value comprises:

- A code pointer referencing the implementation.
- An environment record capturing free variables (possibly empty).
- Metadata describing parameter count, grant set, and contractual sequent.

The environment record layout follows §8.6; it is a nominal record whose fields correspond to captured bindings. The callable type does not expose the environment type directly; instead, closure conversion generates a concrete record type `_Closure_X` with fields and an `apply` method matching the declared FunctionType.

##### §7.4.3.2 Grants [type.function.semantics.grants]

[9] The grant set `ε` enumerates capabilities the callable may exercise. Invocation requires the caller’s context to supply each grant in `ε`. Grant sets compose transitively: if a higher-order procedure receives `(T) -> U ! ε`, then its own grant set must include `ε` unless the procedure guarantees it never invokes the argument.

##### §7.4.3.3 Contracts [type.function.semantics.contract]

[10] The contractual sequent `[[ must => will ]]` (or `[[ grants |- must => will ]]` when grants are present) is interpreted as follows:

- `must` predicates describe caller obligations. At a call site, the type checker verifies these predicates under the caller's context.
- `will` predicates describe callee guarantees. After a call, these predicates may be assumed by the caller.

Logical implication in subtyping (§7.4.3.5) relies on Clause 12's contract entailment rules.

##### §7.4.3.4 Receivers [type.function.semantics.receiver]

[11] Procedure declarations use receiver shorthands `~`, `~%`, and `~!` (§5.4). They desugar to an explicit first parameter `self` with a lexical permission annotation on `Self`:

| Shorthand | Desugared parameter |
| --------- | ------------------- |
| `~`       | `self: const Self`  |
| `~%`      | `self: shared Self` |
| `~!`      | `self: unique Self` |

When a procedure is treated as a first-class callable, that desugared `self` parameter appears as the leading entry in its FunctionType. Section §7.5 explains how these lexical permissions interact with pointer and modal types when the receiver is lowered.

##### §7.4.3.5 Variance and Subtyping [type.function.semantics.variance]

[12] Callable types support the following subtyping rule:

$$
\dfrac{\forall i.\; \upsilon_i <: \tau_i \quad \tau_{\text{ret}} <: \upsilon_{\text{ret}} \quad \varepsilon \subseteq \varepsilon' \quad \text{Must}' \Rightarrow \text{Must} \quad \text{Will} \Rightarrow \text{Will}'}{(\tau_1, \ldots, \tau_n) \to \tau_{\text{ret}} ! \varepsilon \; \texttt{[[} \; \texttt{|-} \; \text{Must} \; \texttt{=>} \; \text{Will} \; \texttt{]]} <: (\upsilon_1, \ldots, \upsilon_n) \to \upsilon_{\text{ret}} ! \varepsilon' \; \texttt{[[} \; \texttt{|-} \; \text{Must}' \; \texttt{=>} \; \text{Will}' \; \texttt{]]}}
\tag{Sub-Function}
$$

- Parameter positions are contravariant (requirements on inputs cannot increase).
- Return type is covariant.
- Grant sets are contravariant (a subtype may require fewer grants).
- `must` clauses are contravariant; `will` clauses are covariant.

##### §7.4.3.6 Copy Predicate [type.function.semantics.copy]

[13] Theorem 7.4.1 (_Callable Copy Predicate_): A callable value is `Copy` iff its closure environment is empty and the underlying implementation marks the code pointer representation as duplicable (e.g., plain procedure items). Capturing closures are `Move`-only.

**Proof sketch**: copying a closure duplicates captured resources. Unless the environment is empty or each captured value satisfies `Copy` and the runtime knows how to duplicate the environment structure safely, copying cannot be guaranteed. The specification leaves implementation-defined hooks for marking specific closures `Copy` but forbids assuming copyability by default.

##### §7.4.3.7 Conversion to Pointers [type.function.semantics.pointer]

[14] Callable values may be converted to pointer types (`Ptr<FnSig>@State`) described in §7.5, enabling FFI interop. Such conversion is permitted only when the callable has an empty environment. Otherwise, the conversion is ill-formed (diagnostic E07-206).

#### §7.4.4 Diagnostics [type.function.diagnostics]

[15] Implementations shall emit at least the diagnostics in Table 7.4.1 when callable types are misused.

**Table 7.4.1 — Callable type diagnostics**

| Code    | Condition                                        |
| ------- | ------------------------------------------------ |
| E07-200 | Grant set contains undeclared grant              |
| E07-201 | Sequent references undefined predicate           |
| E07-202 | Call site lacks required grants                  |
| E07-203 | Call site cannot prove `must` clause             |
| E07-204 | Stored callable loses required grant information |
| E07-205 | Subtyping check violates variance rules          |
| E07-206 | Conversion of capturing closure to raw pointer   |

Diagnostics reference Annex E §E.5 for payload structure (e.g., missing grant names, behavior locations).

#### §7.4.5 Interaction with Declarations [type.function.declarations]

[16] Function and procedure declarations synthesize callable types:

```cursive
procedure add(lhs: i32, rhs: i32): i32
{ ... }
// Type: (i32, i32) -> i32 ! ∅ [[ ∅ |- true => true ]] (canonical: `[[ |- true => true ]]`)

procedure write_line(~%, text: string@View): ()
    [[ io::write |- text.len() < 4096 => true ]]
{
    io::write_all(text)
}
// Type: (Ptr<Self>@Shared, string@View) -> () ! { io::write }
//       [[ text.len() < 4096 => true ]] (canonical: `[[ |- text.len() < 4096 => true ]]`)
```

[17] Closure literals adopt their inferred FunctionType. Example:

```cursive
let scale: (f64) -> f64 = |value| { result value * 2.0 }
```

Closures capturing variables produce a synthesized record `_Closure_Scale` with fields for each capture and an apply function `_Closure_Scale::call(self@Const, value: f64)` matching the FunctionType.

[18] When callable values cross module boundaries, their FunctionType (including grants and sequents) must appear in the exported signature. Re-exporting a callable with a wider grant set or weaker `will` clause is ill-formed (E07-204).

#### §7.4.6 Examples (Informative) [type.function.examples]

**Example 7.4.6.1 (Higher-order procedure):**

```cursive
procedure map<T, U>(values: [T], f: (T) -> U ! ε): [U]
    [[ ε |- values.len() > 0 => result.len() == values.len() ]]
{
    let result = array::with_len(values.len())
    loop i in 0..values.len() {
        result[i] = f(values[i])
    }
    result
}
```

**Example 7.4.6.2 (Closure with capture):**

```cursive
procedure make_logger(stream: io::Writer@Unique): (string@View) -> () ! { io::write }
    [[ alloc::heap |- true => true ]]
{
    result (| message: string@View |)
        [[ io::write |- message.len() <= 1024 => true ]]
    {
        stream.write(message)
    }
}
```

**Example 7.4.6.3 (Subtyping):**

```cursive
let pure_add: (i32, i32) -> i32 = add
let noisy_add: (i32, i32) -> i32 ! { io::write } = annotate(add)
// pure_add <: noisy_add because it requires no grants and has identical contract
```

#### §7.4.7 Conformance Requirements [type.function.requirements]

[19] Implementations shall:

1. Recognize FunctionType syntax with inline grant and sequent clauses.
2. Enforce formation, variance, and subtyping rules, including grant containment and sequent entailment checks.
3. Preserve callable metadata through type inference, closure conversion, and ABI lowering.
4. Diagnose violations using the codes in Table 7.4.1.
5. Ensure receiver shorthand desugars to the pointer forms in §7.4.3.4 and obey the pointer semantics in §7.5.
6. Forbid conversion of capturing closures to raw pointers unless the environment is empty.
7. Expose callable information via introspection (§7.7) exactly as represented in the FunctionType.

[20] Deviations from these obligations render an implementation non-conforming unless explicitly categorized as implementation-defined elsewhere in the specification.

---

**Previous**: §7.3 Composite Types (§7.3 [type.composite]) | **Next**: §7.5 Pointer and Reference Types (§7.5 [type.pointer])
