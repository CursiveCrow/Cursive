# Cursive Language Specification

## Clause 1 — Introduction and Conformance

**Part**: I — Introduction and Conformance
**File**: 01-3_Notation-Conventions.md
**Section**: §1.4 Notation and Conventions
**Stable label**: [intro.notation]  
**Forward references**: Annex A [grammar], Annex C [formal], Clause 8 [expr], Clause 9 [stmt]

---

### §1.4 Notation and Conventions [intro.notation]

[1] This subclause formalises the notational systems used throughout the specification. All mathematical and formal notation is written in LaTeX, using inline `$…$` or display `\[ … \]`/`$$ … $$` delimiters.

#### §1.4.1 Grammar Presentation [intro.notation.grammar]

[2] Grammar fragments appear in fenced code blocks using the `ebnf` fence. Productions follow Extended Backus–Naur Form with terminals enclosed in single quotes and non-terminals expressed in `snake_case`.

[3] The following meta-symbols are used:

- `::=` definition;
- `|` alternative;
- `*`, `+`, `?` zero-or-more, one-or-more, and optional;
- parentheses for grouping; brackets for character classes.

[4] Annex A [grammar] consolidates the authoritative grammar. When a section presents a local excerpt, it shall remain textually consistent with the Annex. Throughout this specification, in-text grammar fragments are informative excerpts provided for convenience; Annex A contains the normative productions that implementations shall follow. Specific Annex A section references are provided where the in-text grammar significantly differs from or extends the Annex.

**Example 1.4.1.1** (EBNF notation):

```ebnf
variable_declaration
    ::= binding_head identifier type_annotation? '=' expression

binding_head
    ::= 'let'
     | 'var'
     | 'shadow' 'let'
     | 'shadow' 'var'
```

This example demonstrates the standard EBNF notation used throughout the specification: non-terminals in `snake_case`, terminals in single quotes, `::=` for definition, `|` for alternatives, and `?` for optional elements.

#### §1.4.2 Metavariables [intro.notation.meta]

[5] Metavariables denote ranges of syntactic or semantic entities. The canonical sets are:

- $x, y, z$ for variables and identifiers;
- $T, U, V$ (or $\tau$) for types;
- $e, f, g$ for expressions;
- $p$ for patterns;
- $s$ for statements;
- $\Gamma, \Sigma, \Delta$ for contexts (type, state, region respectively);
- $G$ for grant sets;
- $@S$ for modal states.

[6] Context operations use conventional notation: $\Gamma, x : T$ extends a context; $\Gamma[x \mapsto T]$ updates a binding; $\sigma[\ell \mapsto v]$ updates a store.

#### §1.4.3 Mathematical Notation [intro.notation.math]

[7] Set membership ($\in$, $\notin$), unions ($\cup$), intersections ($\cap$), and subset relations ($\subseteq$) follow standard mathematical meaning. Logical connectives ($\land$, $\lor$, $\lnot$, $\Rightarrow$, $\Leftrightarrow$) and quantifiers ($\forall$, $\exists$) are also used without further embellishment.

[8] **Evaluation judgments** use big-step operational semantics as the primary notation:

$$
\langle e, \sigma \rangle \Downarrow \langle v, \sigma' \rangle
$$

This judgment states: "Expression $e$ evaluated in store $\sigma$ reduces to value $v$ with resulting store $\sigma'$."

**Store notation conventions:**
- $\sigma$ — Current program store (maps locations to values)
- $\sigma'$ — Resulting store after evaluation (by convention, prime indicates "after")
- $\sigma_{\text{ct}}$ — Compile-time evaluation store (for comptime contexts)
- $\sigma_{\text{entry}}$, $\sigma_{\text{exit}}$ — Temporal stores (procedure entry/exit)
- $\sigma[x \mapsto v]$ — Store $\sigma$ updated with binding $x$ mapped to value $v$

**Evaluation contexts:**
- $\Downarrow$ — Big-step evaluation (expression to value in one step)
- $\Downarrow_{\text{comptime}}$ — Compile-time evaluation (for comptime blocks)
- $\to$ — Small-step reduction (rarely used; expression to expression)

[8.1] **Notation policy**: This specification uses big-step semantics as the primary evaluation model. Small-step is defined for completeness but not used extensively. All evaluation rules should use the big-step form with consistent store threading.

#### §1.4.4 Inference Rules [intro.notation.rules]

[9] Inference rules adopt the standard fraction layout rendered in LaTeX as

$$
\frac{\text{premise}_1 \quad \cdots \quad \text{premise}_n}{\text{conclusion}}\tag{\text{Rule-Name}}
$$

[10] Rules without premises are axioms. Precondition boxes (`[ Given: … ]`) precede rules when necessary to state environmental requirements. Immediately following each rule, prose shall explain the rule's effect in plain language.

[11] **Formal rule naming convention.** Rule tags shall follow a consistent prefix scheme to indicate their semantic category:

- **T-Feature-Case**: Type formation and typing rules (e.g., T-If, T-Bool-True)
- **E-Feature-Case**: Evaluation and operational semantics rules
- **WF-Feature-Case**: Well-formedness and static checking rules (e.g., WF-Modal-Type, WF-Import)
- **P-Feature-Case**: Permission and memory safety rules

Additional prefixes used for specialized semantic categories:

- **Prop-Feature-Case**: Behavior satisfaction and property proofs (e.g., Prop-Int-Copy proves integers satisfy Copy behavior)
- **Coerce-Feature-Case**: Type coercion rules (e.g., Coerce-Never for never-type coercions)
- **Prov-Feature-Case**: Provenance and aliasing rules (e.g., Prov-Addr for address provenance)
- **Ptr-Feature-Case**: Pointer-specific properties and constraints (e.g., Ptr-Size)
- **QR-Feature-Case**: Qualified name resolution rules (e.g., QR-Resolve)

The prefix indicates the judgment form or semantic domain; Feature identifies the language construct; Case distinguishes variants when multiple rules apply to the same construct.

**Example 1.4.4.1** (Inference rule format):

[ Given: $\Gamma$ is a well-formed type environment ]

$$
\frac{\Gamma \vdash e_1 : \text{bool} \quad \Gamma \vdash e_2 : T \quad \Gamma \vdash e_3 : T}{\Gamma \vdash \texttt{if}\ e_1\ \texttt{then}\ e_2\ \texttt{else}\ e_3 : T}
\tag{T-If}
$$

This rule states that an if-expression has type $T$ when the condition has type `bool` and both branches have the same type $T$. The precondition box establishes that $\Gamma$ must be well-formed. The tag `T-If` identifies this as a typing rule for the if-expression.

#### §1.4.5 Typography and Examples [intro.notation.style]

[11] Normative requirements use **bold** emphasis. Newly introduced defined terms appear in _italics_ on first use within a subclause. Language tokens, identifiers, and keywords use `monospace` formatting.

[12] Examples are fenced with the `cursive` info string. Valid and invalid examples may include inline markers `// correct` and `// incorrect` purely for informative purposes. Unless otherwise stated, examples are informative.

[13] Notes, warnings, and rationale blocks follow the bracketed ISO style:

```
[ Note: … — end note ]
[ Warning: … — end warning ]
[ Rationale: … — end rationale ]
```

**Example 1.4.5.1** (ISO bracketed formats):

[ Note: This is informative commentary that clarifies normative requirements without imposing additional constraints. Notes may include implementation guidance or rationale for design decisions.
— end note ]

[ Warning: Violating this constraint results in undefined behavior [UB-ID: B.2.15]. Implementations are not required to diagnose this condition.
— end warning ]

[ Rationale: This design choice ensures type safety while maintaining zero-cost abstraction. Alternative approaches were considered but rejected due to runtime overhead concerns.
— end rationale ]

These three formats distinguish informative notes, safety-critical warnings referencing the undefined behavior catalog (Annex B §B.2), and design rationale respectively.

#### §1.4.6 Cross-References [intro.notation.refs]

[14] In prose, cross-references use the pattern `§X.Y[label]`. Within Clause 1 the stable labels defined in §1.6 [intro.document] apply. Digital versions shall hyperlink every cross-reference.

---

**Previous**: §1.3 Terms and Definitions (§1.3 [intro.terms]) | **Next**: §1.5 Conformance (§1.5 [intro.conformance])
