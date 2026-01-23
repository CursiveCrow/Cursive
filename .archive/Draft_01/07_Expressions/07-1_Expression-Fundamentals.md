# Cursive Language Specification

## Clause 7 — Expressions

**Clause**: 7 — Expressions  
**File**: 08-1_Expression-Fundamentals.md  
**Section**: §7.1 Expression Fundamentals  
**Stable label**: [expr.fundamental]  
**Forward references**: §7.2 [expr.primary], §7.3 [expr.operator], §7.4 [expr.structured], §7.5 [expr.pattern], §7.6 [expr.conversion], §7.7 [expr.constant], §7.8 [expr.typing], Clause 2 §2.3 [lex.tokens], Clause 4 §4.2 [decl.variable], Clause 4 §4.7 [decl.initialization], Clause 4 §4.4 [name.lookup], Clause 6 §6.5 [type.pointer], Clause 6 §6.7 [type.relation], Clause 7 §7.4 [stmt.order], Clause 8 [generic], Clause 9 [memory], Clause 11 [contract]

---

### §7.1 Expression Fundamentals [expr.fundamental]

#### §7.1.1 Scope and Purpose

[1] This subclause establishes the global properties shared by all expressions. Expressions are analysed relative to a _context_ consisting of: (a) the lexical typing environment `Γ`, (b) the active grant budget supplied by the enclosing procedure's grants clause (the grant component of its contractual sequent, see Clause 9), (c) the region and permission stacks defined by Clause 9, and (d) the evaluation schedule described in §7.1.3. Every well-formed expression shall respect all contextual obligations simultaneously; violation of any dimension renders the expression ill-formed.

#### §8.1.2 Grammar Map

[3] Annex A §A.4 is the authoritative grammar for expressions. Table 8.1 lists each relevant production together with the specification subclause that defines its semantics.

| Annex production                                    | Description                                               | Spec reference |
| --------------------------------------------------- | --------------------------------------------------------- | -------------- |
| `PrimaryExpr`, `PostfixExpr`                        | Literals, identifiers, blocks, calls, indexing, pipelines | §8.2           |
| `UnaryExpr`, `BinaryExpr`, `AssignExpr`             | Prefix, infix, and assignment operators                   | §8.3           |
| `StructuredExpr`, `IfExpr`, `MatchExpr`, `LoopExpr` | Composite constructors and control expressions            | §8.4           |
| `Pattern`, `MatchArm`                               | Expression patterns and guards                            | §8.5           |
| `CastExpr`                                          | Explicit conversions (`as`)                               | §8.6           |
| `ComptimeExpr`                                      | Compile-time blocks                                       | §8.7           |
| `Expr`                                              | Type/diagnostic consolidation                             | §8.8           |

#### §8.1.3 Deterministic Evaluation Model

[4] **Strict left-to-right order.** Subexpressions evaluate strictly left-to-right unless a subclause explicitly states otherwise. Formally,

$$
\frac{\langle e_1, \sigma \rangle \Downarrow \langle v_1, \sigma_1 \rangle \quad \cdots \quad \langle e_n[v_1,\ldots,v_{n-1}], \sigma_{n-1} \rangle \Downarrow \langle v_n, \sigma_n \rangle}{\langle f(e_1,\ldots,e_n), \sigma \rangle \Downarrow \langle f(v_1,\ldots,v_n), \sigma_n \rangle}}
\tag{E-LeftToRight}
$$

applies to every compound expression `f`. No implementation may reorder independent operands or hoist subexpressions.

[5] **Short-circuit exception.** The only sanctioned deviation from §8.1.3[4] is the short-circuit behaviour of `&&` and `||` (§8.3.7), which may skip evaluating their right operand when the left operand determines the result. All other operators, including pipelines and ternary constructs, must evaluate each operand.

[6] **Call-by-value semantics.** Procedure calls, grant-requiring operations, and pipeline stages evaluate their arguments completely before transferring control to the callee. Divergent subexpressions (type `!`) propagate divergence to the enclosing expression, ensuring predictable side-effect ordering.

[7] **Determinism.** Given identical inputs, grant budgets, and compile options, a conforming implementation shall produce identical observable behaviour: evaluation order, side effects, and panic points are deterministic. There is no unspecified behaviour akin to C/C++ sequence points.

[8] **Comptime vs runtime contexts.** Expressions appearing in const/comptime positions (§7.7) must be evaluable during the compile-time execution phase (§2.2). Injecting runtime-only constructs (IO, heap allocation, raw pointer arithmetic) into such contexts is ill-formed (E07-001).

[9] **Pipeline annotation rule.** In `lhs => stage : Type`, the annotation `: Type` may be omitted only when the compiler proves `typeof(lhs) ≡ typeof(stage(lhs))` (type equivalence per §6.7). If the stage changes the type—or if equivalence cannot be proven—an explicit annotation is mandatory; omission yields E07-020. Type-preserving stages may omit the annotation to reduce redundancy, but tooling shall still record the before/after types.

#### §8.1.4 Expression Categories

[10] Each expression is classified into exactly one of the following categories:

- **Value** — produces a temporary result that may be moved (consumed) or copied depending on the `Copy` predicate of its type (§6.2–§6.3). Literals, arithmetic expressions, and most calls fall into this category.
- **Place** — denotes a storage location (variables, fields, tuple components, indexed elements, dereferenced pointers). Places may appear on the left side of assignments or as operands to operators requiring mutable access, provided they carry `mut` or `own` permission (§9.4).
- **Divergent** — expressions of type `!` that never return (`loop {}` without `break`, `panic`). Divergent expressions coerce to any type via the rule in §6.2.7 and retain category `divergent` for diagnostic purposes.

[11] The typing judgment records the category: `Γ ⊢ e : τ ! ε [cat]`. While Cursive does not currently expose category information as surface syntax, compilers and IDE tooling shall surface it in diagnostics and metadata. (Informative note: future editions may extend the comptime reflection system with predicates such as `is_place(expr)` when metaprogramming demand justifies the added complexity.)

#### §8.1.5 Grant Accumulation

[12] Grants describe observable capabilities (Clause 11). Each expression carries a grant set `ε`. The grant set of a compound expression is the union of its subexpression grants plus any intrinsic grants contributed by the expression form.

[13] Rule `Grant-Union` summarises grant accumulation:

$$
\frac{\Gamma \vdash e_i : \tau_i ! \varepsilon_i}{\Gamma \vdash f(e_1,\ldots,e_n) : \tau_f ! \left(\bigcup_i \varepsilon_i \cup \varepsilon_f\right)}
\tag{Grant-Union}
$$

where `ε_f` covers intrinsic grants (e.g., `unsafe.ptr` for raw-pointer dereference).

[14] The enclosing procedure shall list at least this union in its contractual sequent's grants clause. Violations: E07-004. This edition deliberately avoids grant bundles to keep obligations explicit. (Informative note: if production experience shows repeated unions hinder readability, the working group may introduce declared bundles in a future revision.)

#### §8.1.6 Typing Discipline

[15] Cursive employs a bidirectional typing discipline:

- **Inference sites** (literals, identifiers, most operands) synthesise a type without needing contextual information.
- **Checking sites** (block `result` expressions, match arms, annotated bindings) require a contextual type. When inference fails and no context exists, E07-002 recommends adding annotations.
- **Bidirectional sites** (function calls, pipelines, polymorphic literals) first attempt to satisfy the contextual expectation; failing that, they synthesise a type and ask the context to accept it via subtyping (§6.7).

[16] Every expression must also satisfy three auxiliary checks:

- **Definite assignment** — Clause 4 §4.7 ensures bindings are initialised before use. Ill-formed usages trigger E07-210 (identifier used before assignment) within expression contexts.
- **Permission safety** — Clause 9 tracks permissions on places. Assigning through a `let` binding or reading a moved `own` value emits E09-403.
- **Type compatibility** — Operands must satisfy the subtyping/compatibility rules of Clause 6. Failures result in E07-800, which reports both types, the relevant rule (variance, equality), and suggested remedies.

#### §8.1.7 Diagnostics

[17] [Note: Diagnostics defined in this subsection are cataloged in Annex E §E.5.1.7. — end note]

#### §8.1.8 Canonical Example

**Example 8.1.8.1 (Deterministic evaluation and pipeline annotations).**

```cursive
use io::{read_config, write_log}

procedure refresh(user: mut Session, raw: string@View): ()
    [[ io::read, io::write |- raw.len() > 0 => user.last_refresh >= clock::now() - 5m ]]
{
    let normalized: Config \/ Error = raw
        => trim
        => parse_config: Config \/ Error
        => validate

    match normalized {
        cfg: Config => {
            user.apply(cfg)
            write_log("refresh ok")
        }
        err: Error => {
            write_log(err.message())
        }
    }
}
```

[19] The example illustrates: (a) strict left-to-right evaluation (`trim` runs before `parse_config`), (b) the pipeline annotation rule (only type-changing stage `parse_config` uses `: Config \/ Error`), (c) place semantics (`user` is a place requiring `mut` permission), and (d) grant accumulation (`io::read` introduced by `read_config` plus `io::write` from `write_log`).

---
