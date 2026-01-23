# Cursive Language Specification

## Clause 8 — Statements and Control Flow

**Part**: VIII — Statements and Control Flow
**File**: 09-1_Statement-Fundamentals.md  
**Section**: §8.1 Statement Fundamentals  
**Stable label**: [stmt.fundamental]  
**Forward references**: §8.2 [stmt.simple], §8.3 [stmt.control], §8.4 [stmt.order], Clause 4 §4.7 [decl.initialization], Clause 7 [expr], Clause 9 [memory], Clause 10 [contract]

---

### §8.1 Statement Fundamentals [stmt.fundamental]

#### §8.1.1 Overview

[1] A _statement_ is a syntactic form that executes for its effects on program state and control flow. Unlike expressions (Clause 7), statements do not directly produce values for consumption; instead, they modify bindings, transfer control, or orchestrate execution order. The execution model uses judgment $\langle s, \sigma \rangle \Downarrow \text{outcome}$, where statement $s$ transforms store $\sigma$ and produces a control outcome. Control outcomes classify normal completion, early returns, loop exits, and abnormal termination.

#### §9.1.2 Statement vs Expression Distinction

[4] Cursive distinguishes statements from expressions:

- **Expressions** (Clause 7) produce typed values and use judgment $\Gamma \vdash e : \tau \; ! \varepsilon$.
- **Statements** (this clause) execute for effects and use judgment $\langle s, \sigma \rangle \Downarrow \text{outcome}$.

[5] Control-flow constructs (`if`, `match`, `loop`) are defined as expressions in Clause 7. When used in statement position, their result values are discarded. This clause specifies how statements sequence, how control outcomes propagate, and how statement-specific forms (assignments, `return`, `defer`) integrate with the expression-based control flow.

#### §9.1.3 Grammar Map

[6] Annex A §A.5 provides the authoritative statement grammar. Table 9.1 maps grammar productions to specification subclauses.

**Table 9.1 — Statement grammar organization**

| Annex production | Description | Specified in |
|---|---|---|
| `Statement`, `SimpleStmt` | Statement categories | §9.1.4 |
| `AssignStmt`, `CompoundAssign` | Assignment forms | §9.2.2 |
| `ExprStmt` | Expression as statement | §9.2.3 |
| `DeferStmt` | Deferred cleanup | §9.2.5 |
| `ReturnStmt` | Early function exit | §9.3.1 |
| `BreakStmt`, `ContinueStmt` | Loop control | §9.3.2, §9.3.3 |
| `LabeledStmt`, `Label` | Statement labels | §9.3.4 |
| `StatementSeq` | Statement sequencing | §9.4.1 |

[ Note: See Annex A §A.5 [grammar.statement] for complete statement grammar.
— end note ]

#### §9.1.4 Statement Categories

[7] Statements are classified into three categories for specification organization:

1. **Simple statements** (§9.2): assignments, expression statements, defer, empty statements
2. **Control-flow statements** (§9.3): return, break, continue, labeled statements
3. **Structured statements**: Control-flow expressions from Clause 8 used as statements

[8] This classification is organizational only; all statements participate uniformly in the execution and sequencing model.

#### §9.1.5 Execution Judgment

[9] The execution judgment has form:

$$
\langle s, \sigma \rangle \Downarrow \text{outcome}
$$

where:
- $s$ is the statement to execute
- $\sigma$ is the current program store (memory state)
- $\text{outcome}$ is a control outcome (§9.1.6)

[10] The store $\sigma$ maps memory locations to values. Statement execution may read from the store, modify existing bindings, allocate new locations, or leave the store unchanged.

#### §9.1.6 Control Outcomes

[11] Control outcomes classify how statement execution terminates. The outcome type is defined:

$$
\text{Outcome} ::= \text{Normal}(\sigma') \mid \text{Return}(v, \sigma') \mid \text{Break}(\ell?, v?, \sigma') \mid \text{Continue}(\ell?, \sigma') \mid \text{Panic}
$$

where:
- $\sigma'$ is the updated store
- $v$ is an optional result value
- $\ell$ is an optional label identifier

[12] **Outcome semantics:**

- **Normal**($\sigma'$): Statement completed successfully; execution continues with updated store
- **Return**($v$, $\sigma'$): Function returns immediately with value $v$ (after executing defers)
- **Break**($\ell?$, $v?$, $\sigma'$): Exit labeled construct, optionally with value $v$
- **Continue**($\ell?$, $\sigma'$): Skip to next iteration of labeled loop
- **Panic**: Abnormal termination; implementation may unwind stack or abort

[13] Non-**Normal** outcomes are _control-transfer outcomes_. They bypass remaining statements in a sequence and propagate outward to their target construct.

#### §9.1.7 Outcome Propagation

[14] Control outcomes propagate through statement sequences according to the following rules:

[ Given: Statements $s_1, s_2$ and store $\sigma$ ]

$$
\frac{\langle s_1, \sigma \rangle \Downarrow \text{Normal}(\sigma') \quad \langle s_2, \sigma' \rangle \Downarrow \text{outcome}}{\langle s_1; s_2, \sigma \rangle \Downarrow \text{outcome}}
\tag{E-Seq-Normal}
$$

[15] When $s_1$ produces a Normal outcome, execution continues with $s_2$ using the updated store.

[ Given: Statement $s_1$ producing control-transfer outcome ]

$$
\frac{\langle s_1, \sigma \rangle \Downarrow \text{outcome} \quad \text{outcome} \in \{\text{Return}, \text{Break}, \text{Continue}, \text{Panic}\}}{\langle s_1; s_2, \sigma \rangle \Downarrow \text{outcome}}
\tag{E-Seq-Transfer}
$$

[16] Control-transfer outcomes bypass remaining statements in the sequence. Statement $s_2$ is not executed.

#### §9.1.8 Well-Formedness Judgment

[17] Statement well-formedness uses judgment $\Gamma \vdash s \; \text{ok}$, where $\Gamma$ is the typing environment. Well-formed statements satisfy:

- Type constraints for subexpressions
- Permission requirements for assignments
- Definite assignment for variable uses
- Label resolution for break/continue
- Grant availability for operation execution

[18] Ill-formed statements produce compile-time diagnostics as specified in the relevant subclauses.

#### §9.1.9 Integration with Expressions

[19] Statements integrate with Clause 7 expressions in three ways:

(19.1) **Expression statements**: Any expression may be used as a statement; its value is discarded (§9.2.3).

(19.2) **Control-flow expressions**: Constructs like `if`, `match`, `loop` (Clause 7 §7.4) may be used as statements. Their execution follows Clause 7 semantics; when used as statements, result values are discarded.

(19.3) **Subexpressions**: Statements contain expressions (initializers, conditions, operands). Expression evaluation follows Clause 7 rules; statement execution sequences those evaluations.

#### §9.1.10 Statement Termination

[20] Statement termination follows the rules in §2.4 [lex.terminators]:

- Newlines terminate statements unless a continuation rule applies
- Semicolons may separate multiple statements on one line
- Four continuation cases: unclosed delimiters, trailing operators, leading dot, leading pipeline

[21] These rules apply uniformly to all statement forms. Statement-specific syntax does not alter termination behavior.

#### §8.1.11 Conformance Requirements

[22] Implementations shall execute defer blocks in LIFO order on all control-flow exits and provide structured control flow only (no goto).

---

**Previous**: Clause 7 — Expressions (§7.8 [expr.typing]) | **Next**: §8.2 Simple Statements (§8.2 [stmt.simple])

