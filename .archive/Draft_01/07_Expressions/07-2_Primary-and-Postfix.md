# Cursive Language Specification

## Clause 7 — Expressions

**Clause**: 7 — Expressions  
**File**: 08-2_Primary-and-Postfix.md  
**Section**: §7.2 Primary and Postfix Expressions  
**Stable label**: [expr.primary]  
**Forward references**: §8.3 [expr.operator], §8.4 [expr.structured], Clause 2 §2.3 [lex.tokens], Clause 4 §4.3 [module.scope], Clause 5 §5.2 [decl.variable], Clause 6 §6.4 [name.lookup], Clause 7 §7.5 [type.pointer], Clause 10 [generic], Clause 11 [memory]

---

### §7.2 Primary and Postfix Expressions [expr.primary]

#### §7.2.1 Overview

[1] Primary expressions form the leaves of the expression grammar (Annex A §A.4.1–§A.4.3); postfix expressions extend those primaries with calls, field access, indexing, slicing, pointer manipulation, and pipelines (Annex A §A.4.4–§A.4.8). This subclause specifies their syntax, typing, evaluation, and diagnostics in detail.

#### §7.2.2 Literals

[2] Grammar reference: Annex A §A.4.1 (`Literal`). Literal tokens arise from the lexical grammar (§2.3). Each literal’s default type is defined in Clause 7 §7.2.

[3] Typing rules:

- **Integer literals** default to `i32`. Contextual typing may coerce them to any integer type `τ` whose value set contains the literal. Literal suffixes (`123u64`, `0xffu8`) override both defaults and contextual expectations. Ill-fitting literals produce E08-201.
- **Floating literals** default to `f64`; suffix `f32` selects `f32`. Mixed numeric literals require explicit casts (§8.6).
- **Boolean literals** `true`/`false` always have type `bool`.
- **Character literals** have type `char`. They shall denote a single Unicode scalar value; invalid escapes or surrogate code points emit E08-202.
- **String literals** have type `string@View` and refer to statically allocated UTF-8 data. Modal transitions to `string@Owned` require explicit constructors (§7.3.4).
- **Tuple literals** and **array literals** are structured forms handled in §8.4, but their element typing relies on the literal rules above.

[4] Evaluation: literals evaluate to themselves without modifying the store. String literals may share backing storage; this does not affect semantics because `string@View` values are immutable views.

#### §7.2.3 Identifier Expressions

[5] Grammar reference: Annex A §A.4.1 (`PrimaryExpr ::= IDENT`). Identifiers are resolved using the algorithm in Clause 6 §6.4. If resolution yields a type, module, or contract, the expression is ill-formed (E08-211).

[6] Typing rule:

$$
\frac{(x : \tau [cat]) \in \Gamma}{\Gamma \vdash x : \tau ! \emptyset [cat]}
\tag{T-Ident}
$$

[7] Using an identifier before definite assignment (Clause 5 §5.7) emits E08-210. When the binding category is `place`, the expression inherits that category; otherwise it is a value.

#### §7.2.4 Parenthesised Expressions

[8] Grammar: `ParenExpr ::= '(' Expr ')'` (Annex A §A.4.2).

[9] Typing: parentheses do not alter type, grant set, or category. They exist solely for disambiguation and readability. Implementations shall preserve parentheses in diagnostics to aid tooling.

#### §7.2.5 Block Expressions

[10] Grammar: `BlockExpr ::= '{' Statement* ('result' Expr)? '}'` (Annex A §A.4.3).

[11] Typing:

- Blocks introduce a lexical scope (§6.2). Statements inside the block are type-checked per Clause 9.
- If the block includes `result expr`, the block’s type is `typeof(expr)`; otherwise it is `()`.
- Non-unit blocks shall use `result` explicitly; omission triggers E08-220.

[12] Evaluation: statements execute in order, each possibly mutating the store. `result expr` evaluates `expr` as the final value; the scope then exits, dropping bindings in reverse creation order (Clause 12 §12.2).

#### §7.2.6 Unit Expression

[13] `()` denotes both the unit literal and the unit type (§7.2.6). It is the default result when an expression exists solely for side effects.

#### §7.2.7 Procedure and Function Calls

[14] Grammar: `CallExpr ::= PostfixExpr '(' ArgumentList? ')'` (Annex A §A.4.4).

[15] Typing:

$$
\frac{\Gamma \vdash f : (\tau_1,\ldots,\tau_n) \to \tau_r ! \varepsilon_f \quad \Gamma \vdash a_i : \tau_i ! \varepsilon_i}{\Gamma \vdash f(a_1,\ldots,a_n) : \tau_r ! (\varepsilon_f \cup \varepsilon_1 \cup \cdots \cup \varepsilon_n)}
\tag{T-Call}
$$

[16] Variadic or keyword argument patterns are expanded into syntactic sugar that ultimately matches the above rule. Call arity mismatches or missing required arguments emit E08-230; extra arguments emit E08-231.

[17] Associated procedures (`receiver::method(args)`) desugar to calls on values defined in Clause 5. The receiver expression is evaluated first and must provide the permission declared by the method’s `self` parameter. Violations produce E08-232 (insufficient permission) or E08-233 (receiver type mismatch).

[18] Evaluation order is left-to-right: callee expression, then each argument. Partial evaluation (e.g., due to `move`) follows the semantics of the argument expressions themselves.

#### §7.2.8 Field Access and Tuple Projection

[19] Grammar: `FieldExpr ::= PostfixExpr '.' IDENT`, `TupleProj ::= PostfixExpr '.' INTEGER_LITERAL` (Annex A §A.4.5).

[20] Typing:

- Records: if `Γ ⊢ e : R` and `R` declares field `f : τ` visible in the current module (Clause 5 §5.6), then `Γ ⊢ e.f : τ [cat]` where `cat` matches `e`’s category. Accessing a private/internal field from another module emits E08-240.
- Tuple projections: if `Γ ⊢ e : (τ_1,…,τ_n)` and `0 ≤ i < n`, then `Γ ⊢ e.i : τ_{i+1}`. Indices outside this range emit E08-241.

[21] Both forms preserve place/value categories: reading a field from a place yields a place.

#### §7.2.9 Array Indexing and Slicing

[22] Grammar: `IndexExpr ::= PostfixExpr '[' Expr ']'`, `SliceExpr ::= PostfixExpr '[' RangeExpr? ']'` (Annex A §A.4.6).

[23] Typing:

- Arrays `[τ; n]` and slices `[τ]` may be indexed with `usize`. The result type is `τ`. Index expressions evaluate left-to-right.
- Slicing `[start..end]` yields `[τ]`. Bounds may be partially omitted (defaulting to `0` or `len`). Bounds must satisfy `0 ≤ start ≤ end ≤ len`; otherwise E08-251 is raised.

[24] Runtime bounds checking is mandatory. Out-of-range indices panic with diagnostic E08-250 containing the index, length, and expression span.

#### §7.2.10 Pointer Address-of and Dereference

[25] Grammar: unary `&` and unary `*` (Annex A §A.4.7).

[26] Address-of (`&place`) is valid only for place expressions tied to storage. Typing rule:

$$
\frac{\Gamma \vdash p : \tau [place]}{\Gamma \vdash \&p : Ptr\langle \tau \rangle@Valid}
\tag{T-Addr}
$$

Attempting to take the address of a value emits E08-260.

[27] Dereference typing is defined in Clause 7 §7.5, but evaluation semantics require the pointer to be in state `@Valid`. Dereferencing `@Null`, `@Weak`, or `@Expired` pointers produces diagnostics E07-301/E07-304/E07-305 as appropriate.

#### §7.2.11 Pipelines

[28] Grammar: `PipelineExpr ::= PostfixExpr '=>' PostfixExpr ( '=>' PostfixExpr )*` (Annex A §A.4.8).

[29] Semantics: `lhs => stage` desugars to `stage(lhs)`. Each stage shall be callable with exactly one argument—the output of the preceding stage. Additional parameters shall be supplied via closures or partial application.

[30] Typing: let `Γ ⊢ lhs : τ_0`. For each stage `stage_k`, require `Γ ⊢ stage_k : (τ_{k-1}) -> τ_k ! ε_k`. The pipeline’s type is `τ_m`. Stages that omit `: τ_k` must satisfy the type-preserving rule in §8.1.3[9]. Missing or mismatched annotations trigger E08-020/E08-021.

[31] Evaluation is left-to-right: compute `lhs`, feed it into `stage_1`, feed the result into `stage_2`, and so on. The grant set is the union of `lhs`’s grants and all stage grants.

#### §7.2.12 Diagnostics Summary

| Code    | Condition                                              |
| ------- | ------------------------------------------------------ |
| E08-201 | Integer literal cannot fit target type                 |
| E08-202 | Invalid character literal                              |
| E08-210 | Identifier used before definite assignment             |
| E08-211 | Identifier resolves to non-value entity                |
| E08-220 | Block producing non-unit value without `result`        |
| E08-230 | Procedure call arity mismatch                          |
| E08-231 | Procedure call has extra arguments                     |
| E08-232 | Method receiver lacks required permission              |
| E08-233 | Method receiver has incompatible type                  |
| E08-240 | Field access violates visibility                       |
| E08-241 | Tuple projection index out of range                    |
| E08-250 | Array/slice index out of range                         |
| E08-251 | Slice bounds invalid                                   |
| E08-260 | Address-of applied to non-place expression             |
| E08-020 | Pipeline stage omitted type while changing type        |
| E08-021 | Pipeline stage annotation disagrees with inferred type |

#### §7.2.13 Canonical Example

```cursive
let payload: string@Managed \/ ParseError = raw
    => trim
    => parse: Ast \/ ParseError
    => render: string@Managed \/ ParseError

match payload {
    html: string@Managed => io::write(html),
    err: ParseError => log::error(err.message()),
}
```

[32] This example demonstrates literal strings, pipelines with mandatory annotation on the type-changing stage, field access on union types, and pattern matching (detailed further in §8.5). The pipeline's grant set is the union of the individual stage grants, ensuring the enclosing procedure's grants clause (in its contractual sequent) is explicit.

---
