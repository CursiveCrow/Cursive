# The Cursive Language Specification (Draft 3)

# Part 0: Preliminaries

## Clause 0: Preliminaries

---

### 0.1 Glossary of Terms

##### Definition

The following terms have precise meanings in this specification:

**Lexeme**: The character sequence matched by a lexical production during tokenization. A lexeme is the concrete syntactic representation of a token in source text.

**Linear** (lattice theory): A lattice in which every pair of elements is comparable; also called "totally ordered." A linear lattice has exactly one path between top and bottom.

**Nominal Equivalence**: Two types are nominally equivalent if and only if they refer to the same declaration. Records and enums use nominal equivalence.

**Payload**: Associated data carried by an enum variant or union member. A variant may have no payload (unit-like), a tuple payload (positional fields), or a record payload (named fields).

**Place Expression**: An expression that denotes a memory location (also known as an l-value). Place expressions include:
- Identifiers bound to variables (`x`)
- Field access (`x.field`)
- Indexed access (`x[i]`)
- Dereference (`*ptr`)

Place expressions may appear on the left-hand side of assignments and as operands of address-of (`&`) and move expressions.

**Structural Equivalence**: Two types are structurally equivalent if their component types are equivalent and in the same order. Tuples, arrays, slices, and union types use structural equivalence.

---

### 0.2 Notation and Symbols

##### Definition

**Typing Judgment Notation**

Typing rules use the following notation:

$$\Gamma \vdash e : T \dashv \Gamma'$$

where:
- $\Gamma$ (input context, read "under context Gamma"): The typing context before evaluating $e$, containing bindings and their states.
- $\vdash$ (turnstile): Separates the context from the judgment; read as "entails" or "proves."
- $e : T$: The expression $e$ has type $T$.
- $\dashv$ (reverse turnstile): Separates the judgment from the output context; indicates "producing."
- $\Gamma'$ (output context): The typing context after evaluating $e$, reflecting any state changes (e.g., moved bindings).

When only the input context matters (no state change), the simpler form $\Gamma \vdash e : T$ is used.

**Inference Rule Format**

Inference rules are written as:

$$\frac{\text{premise}_1 \quad \text{premise}_2 \quad \ldots}{\text{conclusion}}$$

The rule states: if all premises above the line hold, then the conclusion below the line holds.

---

# Part 1: General Principles and Conformance

## Clause 1: General Principles and Conformance

### 1.1 Conformance Obligations

##### Definition

A **conforming implementation** is a translator (compiler, interpreter, or hybrid) that satisfies all normative requirements of this specification applicable to implementations.

A **conforming program** is a Cursive source program that satisfies all normative requirements of this specification applicable to programs.

##### Static Semantics

**Implementation Requirements**

A conforming implementation:

1. MUST accept all well-formed programs and translate them to an executable representation.

2. MUST reject all ill-formed programs (§1.2.1) (except those classified as IFNDR) with at least one error diagnostic identifying the violation.

3. MUST generate a **Conformance Dossier** as a build artifact. The dossier MUST be a machine-readable JSON document conforming to the schema defined in Appendix C.

4. MUST document all Implementation-Defined Behavior (IDB) choices in the Conformance Dossier.

5. MUST record all IFNDR instances per §1.2.1.


**Program Requirements**

A **well-formed program** satisfies the following conditions:

1. Satisfies all lexical constraints defined in Clause 2.
2. Satisfies all syntactic constraints defined in Clause 3.
3. Satisfies all static-semantic constraints defined throughout this specification.

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                                            | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------------------------------- | :----------- | :-------- |
| `E-CNF-0101` | Error    | Program is ill-formed due to syntactic or static-semantic violation. | Compile-time | Rejection |
| `E-CNF-0102` | Error    | Conformance Dossier generation failed.                               | Compile-time | Rejection |

Implementation limit violations are diagnosed per §1.4.

---

### 1.2 Behavior Classifications

##### Definition

**Behavior classifications** partition all program behaviors into four mutually exclusive categories based on how this specification constrains outcomes and what guarantees implementations provide.

##### Static Semantics

**Defined Behavior** is program behavior for which this specification prescribes exactly one permitted outcome. Conforming implementations MUST produce this outcome for all well-formed programs exercising such behavior.

**Implementation-Defined Behavior (IDB)** occurs when this specification permits multiple valid outcomes and requires the implementation to document which outcome it chooses. The implementation MUST select exactly one outcome from the permitted set, document its choice in the Conformance Dossier, and maintain consistency across all compilations with the same configuration. Programs relying on a specific IDB choice are conforming but non-portable. See Appendix I for the complete IDB index.

**Unspecified Behavior (USB)** occurs when this specification permits a set of valid outcomes but does not require the implementation to document which outcome is chosen. The choice MAY vary between executions, compilations, or within a single execution. USB in safe code MUST NOT introduce memory unsafety or data races. See Appendix I for the complete USB index.

**Unverifiable Behavior (UVB)** comprises operations whose runtime correctness depends on properties external to the language's semantic model. UVB is permitted only within `unsafe` blocks and Foreign Function Interface (FFI) calls. An `unsafe` block assertion transfers responsibility for maintaining external invariants to the programmer. See Appendix I for the complete UVB index.

**Diagnostic Coordination**

Multiple diagnostic codes across different clauses may be triggered by the same underlying program violation when detected at different compilation phases. See Appendix B.4 for conflict resolution rules.

##### Constraints & Legality

**Safety Invariant**

Safe code cannot exhibit Unverifiable Behavior. For any well-formed program where all expressions are outside of `unsafe` blocks, the program MUST NOT exhibit any Unverifiable Behavior. This invariant is enforced by the type system (Clause 4) and the permission system (§4.5).

---

#### 1.2.1 Ill-Formed, No Diagnostic Required (IFNDR)

##### Definition

A program that violates a static-semantic rule of the language is **ill-formed**. When detecting such a violation is computationally undecidable or infeasible, an implementation is not required to issue a diagnostic. Such a program is classified as **Ill-Formed, No Diagnostic Required (IFNDR)**.

##### Static Semantics

**Classification Criteria**

A violation is classified as IFNDR when at least one of the following conditions holds:

1. The violation involves a property that is undecidable in general (e.g., halting, unbounded symbolic execution).
2. The violation requires analysis that exceeds polynomial complexity in the size of the program.
3. The violation depends on runtime values that cannot be statically determined.

**IFNDR Categories**

The following are IFNDR categories (these are classification categories, not diagnostics; IFNDR conditions by definition do not require diagnostics):

- **OOB in Const Eval**: Index out of bounds during constant evaluation where index is not a static constant
- **Recursion Limits**: Exceeding implementation limits in complex metaprogramming that cannot be bounded
- **Infinite Type**: Type definitions that would require infinite expansion
- **Unbounded Unrolling**: Loop unrolling in `comptime` context that cannot be statically bounded

##### Constraints & Legality

**Safety Boundary**

The observable effects of executing an IFNDR program are Unspecified Behavior (USB). The Safety Invariant (§1.2) still applies: when the IFNDR condition occurs in safe code, the observable effects MUST NOT exhibit Unverifiable Behavior (UVB).

Permitted outcomes include:

- Compilation failure (if detected)
- Deterministic panic at runtime
- Non-deterministic but safe execution

Prohibited outcomes include:

- Memory corruption
- Data races
- Arbitrary code execution

**Tracking and Reporting**

IFNDR instances MUST be recorded in the Conformance Dossier per §1.1. The dossier MUST record the source file path, line number, and error category for every IFNDR instance.

---

### 1.3 Reserved Identifiers

##### Definition

A **reserved identifier** is an identifier that is reserved for use by this specification or by implementations, and MUST NOT be used as a user-defined identifier in conforming programs.

##### Static Semantics

**Reserved Keywords**

The identifiers listed in the keyword table (§2.6) are reserved keywords per the constraints defined therein.

**Reserved Namespaces**

The `cursive.*` namespace prefix is reserved for specification-defined features. User programs and vendor extensions MUST NOT use this namespace.

**Implementation Reservations**

Implementations MAY reserve additional identifier patterns.

**Synthetic Identifier Prefix**

The identifier prefix `gen_` is reserved for synthetic identifiers. User declarations MUST NOT define identifiers beginning with `gen_`.

Unlike implementation-reserved patterns (which are IDB), the `gen_` prefix is specification-mandated and applies uniformly across all conforming implementations.

**Universe-Protected Bindings**

**A Universe-Protected Binding is an identifier that is pre-declared in the implicit universal scope and MUST NOT be shadowed by user declarations at module scope (§8).**

The following identifiers are universe-protected:

1. **Primitive type names:** `i8`, `i16`, `i32`, `i64`, `i128`, `u8`, `u16`, `u32`, `u64`, `u128`, `f16`, `f32`, `f64`, `bool`, `char`, `usize`, `isize`

2. **Special types:** `Self`, `string`, `Modal`

3. **Async type aliases (§15.3):** `Async`, `Future`, `Sequence`, `Stream`, `Pipe`, `Exchange`

> **Forward Reference:** `Self` is the type-relative self-reference (§4.2). `string` is a modal type for text (§6.2). `Modal` is the metatype for modal declarations (§6.1). Async type aliases are defined in §15.3.

Attempting to shadow a universe-protected binding at module scope MUST be diagnosed as an error.

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                                                                    | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------------------------------------------------------- | :----------- | :-------- |
| `E-CNF-0401` | Error    | Reserved keyword used as identifier.                                                         | Compile-time | Rejection |
| `E-CNF-0402` | Error    | Reserved namespace `cursive.*` used by user code.                                            | Compile-time | Rejection |
| `E-CNF-0403` | Error    | Primitive type name shadowed at module scope.                                                | Compile-time | Rejection |
| `E-CNF-0404` | Error    | Shadowing of `Self`, `string`, or `Modal`.                                                   | Compile-time | Rejection |
| `E-CNF-0405` | Error    | Shadowing of async type alias (`Async`, `Future`, `Sequence`, `Stream`, `Pipe`, `Exchange`). | Compile-time | Rejection |
| `W-CNF-0401` | Warning  | Implementation-reserved pattern used.                                                        | Compile-time | N/A       |
| `E-CNF-0406` | Error    | User declaration uses gen_ prefix                                                            | Compile-time | Rejection |

---

### 1.4 Implementation Limits

##### Definition

**Implementation limits** are minimum guaranteed capacities that all conforming implementations MUST support. Programs exceeding these limits are ill-formed.

##### Static Semantics

**Minimum Guaranteed Limits**:
*   **Source Size**: 1 MiB
*   **Logical Lines**: 65,535
*   **Line Length**: 16,384 chars
*   **Nesting Depth**: 256
*   **Identifier Length**: 1,023 chars
*   **Parameters**: 255
*   **Fields**: 1,024

**Compile-Time Execution Limits**:

The minimum guaranteed capacities for compile-time execution are defined in §17.8. For reference:

*   **Comptime Recursion Depth**: 256 frames
*   **Comptime Evaluation Steps**: 10,000,000
*   **Comptime Memory Allocation**: 256 MiB
*   **Comptime String Length**: 16 MiB
*   **Comptime Sequence Length**: 1,000,000 elements
*   **Total Emitted Declarations**: 100,000

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                      | Detection    | Effect    |
| :----------- | :------- | :----------------------------- | :----------- | :-------- |
| `E-CNF-0301` | Error    | Implementation limit exceeded. | Compile-time | Rejection |

---

### 1.5 Language Evolution

##### Definition

**Language evolution** governs the versioning, feature lifecycle, edition system, and extension mechanisms for the Cursive language.

##### Static Semantics

**Versioning Model**

The Cursive language version identifier MUST follow Semantic Versioning 2.0.0 (`MAJOR.MINOR.PATCH`).

**Version Declaration**

Programs MUST declare their target language version in the project manifest. The mechanism for version declaration is implementation-defined.

**Version Compatibility**

An implementation MUST reject a program if the declared `MAJOR` version does not match a `MAJOR` version supported by the implementation.

Implementations MUST maintain source-level backward compatibility for conforming programs across `MINOR` version increments within the same `MAJOR` version.

**Editions**

When editions are supported:

1. An implementation MUST allow a program to declare its target edition.
2. Semantics from different editions MUST NOT be mixed within a single compilation unit.
3. An edition MAY introduce new keywords, alter syntax, or change semantic rules.
4. Changes introduced by an edition apply only to code that has opted into that edition.

**Feature Stability Classes**

Every language feature MUST be classified into exactly one stability class:

| Class          | Availability          | Change Policy                                                |
| :------------- | :-------------------- | :----------------------------------------------------------- |
| `Stable`       | Default (no opt-in)   | No breaking changes except in `MAJOR` increments or editions |
| `Preview`      | Requires feature flag | MAY change between `MINOR` versions                          |
| `Experimental` | Requires feature flag | MAY change or be removed in any version                      |

**Deprecation Policy**

The deprecation policy defines the lifecycle for removing language features:

1. Deprecated features MUST remain functional for at least one full `MINOR` version after the version in which they are deprecated.

2. Implementations MUST issue a warning diagnostic when a deprecated feature is used.

3. Features, other than `Experimental` ones, MAY be removed only in `MAJOR` version increments or new editions.

4. Removal of a deprecated feature MUST result in an error diagnostic.

5. Features with discovered security vulnerabilities MAY follow an expedited deprecation and removal timeline, documented in the release notes.

**Feature Flags**

Extensions and preview features requiring opt-in MUST be controlled through feature flags.

1. Feature flags for specification-defined preview features MUST use identifiers without vendor prefixes.

2. An implementation MUST issue an error for an unknown feature flag identifier.

3. Use of a feature that requires a flag, when that flag is not enabled, MUST be diagnosed as an error.

**Vendor Extensions**

Vendor-specific extensions enable implementations to provide additional capabilities beyond the core specification:

1. Vendor-specific extensions MUST use a reverse-domain-style namespace (e.g., `com.vendor.feature`).

2. The `cursive.*` namespace is reserved (§1.3).

3. Extensions MUST NOT alter the meaning of conforming programs written without the extension.

4. Extensions MUST NOT suppress required diagnostics.

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                               | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------------------ | :----------- | :-------- |
| `E-CNF-0601` | Error    | Declared MAJOR version not supported by implementation. | Compile-time | Rejection |
| `E-CNF-0602` | Error    | Unknown feature flag.                                   | Compile-time | Rejection |
| `E-CNF-0603` | Error    | Feature requires flag that is not enabled.              | Compile-time | Rejection |
| `E-CNF-0604` | Error    | Edition mixing within compilation unit.                 | Compile-time | Rejection |
| `W-CNF-0601` | Warning  | Use of deprecated feature.                              | Compile-time | N/A       |
| `E-CNF-0605` | Error    | Use of removed feature.                                 | Compile-time | Rejection |

---

### 1.6 Foundational Semantic Concepts

##### Definition

**Program Point**

A **Program Point** is a unique location in the abstract control flow representation of a procedure body.

$$\text{ProgramPoint} ::= (\text{ProcedureId}, \text{CFGNodeId})$$

where:
- $\text{ProcedureId}$ uniquely identifies the enclosing procedure
- $\text{CFGNodeId}$ uniquely identifies a node in that procedure's control flow graph

Program points are partially ordered by control flow reachability: $p_1 \leq_{cf} p_2$ if and only if $p_2$ is reachable from $p_1$ via zero or more control flow edges.

**Lexical Scope**

A **Lexical Scope** is a contiguous region of source text that defines a lifetime boundary for bindings, keys, and deferred actions.

$$\text{LexicalScope} ::= (\text{EntryPoint}, \text{ExitPoint}, \text{Parent}?)$$

where:
- $\text{EntryPoint}$ is the program point where the scope begins
- $\text{ExitPoint}$ is the program point where the scope ends
- $\text{Parent}$ is the immediately enclosing scope (absent for procedure-level scope)

---

## Clause 2: Lexical Structure and Source Text

### 2.0 Semantic Domains

##### Definition

The following semantic domains are used throughout this clause:

| Domain              | Notation                                          | Description                                                                                                             |
| :------------------ | :------------------------------------------------ | :---------------------------------------------------------------------------------------------------------------------- |
| Unicode Scalars     | $\mathcal{U}$                                     | The set of all Unicode scalar values (U+0000–U+D7FF ∪ U+E000–U+10FFFF)                                                  |
| Prohibited          | $\mathcal{F}$                                     | Code points prohibited in source text (§2.1)                                                                            |
| Characters          | $c \in \mathcal{U}$                               | A single Unicode scalar value                                                                                           |
| Character Sequence  | $s \in \mathcal{U}^*$                             | Finite sequence of Unicode scalars                                                                                      |
| Byte Stream         | $B \in \mathbb{B}^*$                              | Raw byte sequence before decoding                                                                                       |
| Token               | $\tau = (\kappa, \lambda, \pi)$                   | Triple of kind, lexeme, and source span                                                                                 |
| Token Kind          | $\kappa \in \mathcal{K}$                          | $\{\texttt{identifier}, \texttt{keyword}, \texttt{literal}, \texttt{operator}, \texttt{punctuator}, \texttt{newline}\}$ |
| Lexeme              | $\lambda \in \mathcal{U}^*$                       | Character sequence matched by token                                                                                     |
| Source Position     | $\pi = (f, l, c)$                                 | File path, line number, column number                                                                                   |
| Source Span         | $\sigma = (\pi_{\text{start}}, \pi_{\text{end}})$ | Start and end positions                                                                                                 |
| Token Stream        | $\Theta \in \tau^*$                               | Ordered sequence of tokens                                                                                              |
| Lexer Configuration | $\mathcal{C} = (s, \pi, \Theta)$                  | Current input, position, accumulated tokens                                                                             |

##### Judgment Forms

**Lexical Classification Judgment**

$$s \vdash_{\text{lex}} \tau : \kappa \dashv s'$$

The character sequence $s$ produces token $\tau$ of kind $\kappa$, leaving residual input $s'$.

**Small-Step Tokenization**

$$\mathcal{C} \longrightarrow \mathcal{C}'$$

Lexer configuration $\mathcal{C}$ transitions to $\mathcal{C}'$ by consuming input and optionally emitting a token.

**Big-Step Preprocessing**

$$B \Downarrow_{\text{preprocess}} \Theta$$

Byte stream $B$ preprocesses to token stream $\Theta$.

**Validation Judgment**

$$\vdash c\ \text{valid}$$

Character $c$ satisfies source text constraints.

**Error Judgment**

$$\vdash c \dashv E$$

Character $c$ violates constraint, producing diagnostic $E$.

---

### 2.1 Source Text Encoding

##### Definition

**Source Text** is a sequence of Unicode scalar values (as defined by the Unicode Standard) that constitutes the input to the Cursive translation pipeline. A **Source Byte Stream** is the raw sequence of bytes read from a file or other input source before decoding. A **Normalized Source File** is the Unicode scalar sequence obtained from a source byte stream after the preprocessing pipeline (§2.3) has successfully completed.

**Formal Definition**

Let $\mathcal{U}$ denote the set of Unicode scalar values. Let $\mathcal{F}$ denote the set of prohibited code points (defined below). A valid source text $S$ is:

$$S \in \mathcal{U}^* \setminus \{s : \exists c \in s. c \in \mathcal{F}\}$$

##### Static Semantics

**UTF-8 Mandate**

Cursive source input MUST be encoded as UTF-8 (RFC 3629). Invalid byte sequences are ill-formed and MUST trigger diagnostic `E-SRC-0101`.

**BOM Handling**

If a source file begins with the UTF-8 byte order mark (BOM, U+FEFF) as the first decoded scalar value, the implementation MUST strip the BOM before lexical analysis. A BOM appearing at any position after the first MUST trigger diagnostic `E-SRC-0103`.

When a source file begins with a UTF-8 BOM and otherwise satisfies the constraints of this clause, a conforming implementation SHOULD emit warning `W-SRC-0101` while still accepting the file.

**Unicode Normalization**

Unicode normalization of source text outside identifiers and module-path components is Implementation-Defined Behavior (IDB).

For identifier lexemes and module-path components, implementations MUST normalize the corresponding scalar sequences to Unicode Normalization Form C (NFC) prior to equality comparison, hashing, or name lookup.

Any normalization performed MUST NOT change logical line boundaries or the byte offsets used for diagnostic locations.

##### Constraints & Legality

**Prohibited Code Points**

Source files MUST NOT contain prohibited code points outside of string and character literals (§2.8). The prohibited code points $\mathcal{F}$ are:

$$\mathcal{F} = \{c : \text{GeneralCategory}(c) = \texttt{Cc}\} \setminus \{\text{U+0009}, \text{U+000A}, \text{U+000C}, \text{U+000D}\}$$

A source file containing a prohibited code point outside of a literal MUST be rejected with diagnostic `E-SRC-0104`.

**Diagnostic Table**

| Code         | Severity | Condition                                    | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------- | :----------- | :-------- |
| `E-SRC-0101` | Error    | Invalid UTF-8 byte sequence.                 | Compile-time | Rejection |
| `E-SRC-0103` | Error    | Embedded BOM found after the first position. | Compile-time | Rejection |
| `E-SRC-0104` | Error    | Forbidden control character or null byte.    | Compile-time | Rejection |
| `W-SRC-0101` | Warning  | UTF-8 BOM present at the start of the file.  | Compile-time | N/A       |

---

### 2.2 Source File Structure

##### Definition

A **Logical Line** is a sequence of Unicode scalar values terminated by a line feed character (U+000A) or end-of-file. **Line Ending Normalization** is the transformation that converts all platform-specific line ending sequences to a single canonical line feed character.

**Formal Definition**

$$L ::= c^* \cdot (\text{U+000A})? \quad \text{where } c \in \mathcal{U} \setminus (\mathcal{F} \cup \{\text{U+000A}\})$$

A source file is a sequence of logical lines: $S = L_1 \cdot L_2 \cdot \ldots \cdot L_n$.

##### Syntax & Declaration

A source file MUST conform to the following grammar after preprocessing:

```ebnf
<source_file>     ::= <normalized_line>*
<normalized_line> ::= <code_point>* <line_terminator>?
<line_terminator> ::= U+000A
<code_point>      ::= U - {U+000A} - F   /* where F is the prohibited set (§2.1) */
```

##### Static Semantics

Implementations MUST normalize line endings before tokenization as follows:

1. Replace each CR LF (U+000D U+000A) with a single LF (U+000A).
2. Replace each standalone CR (U+000D) not followed by LF with a single LF (U+000A).
3. Each LF (U+000A) that does not follow a CR remains unchanged.

Mixed line endings are permitted within a single source file.

##### Constraints & Legality

Implementations MUST enforce the implementation limits specified in §1.4 for source file size and logical line count.

**Diagnostic Table**

| Code         | Severity | Condition                            | Detection    | Effect    |
| :----------- | :------- | :----------------------------------- | :----------- | :-------- |
| `E-SRC-0102` | Error    | Source file exceeds maximum size.    | Compile-time | Rejection |
| `E-SRC-0105` | Error    | Maximum logical line count exceeded. | Compile-time | Rejection |

---

### 2.3 Preprocessing Pipeline

##### Definition

The **Preprocessing Pipeline** is the mandatory sequence of transformations applied to a source byte stream to produce a normalized source file suitable for tokenization.

**Formal Definition**

$$\text{Preprocess} : \text{ByteStream} \to \text{TokenStream} \cup \{\bot\}$$

The pipeline is the sequential composition of six steps, where $\bot$ indicates failure:

$$\text{Preprocess} = \text{Tokenize} \circ \text{ValidateCtrl} \circ \text{NormLines} \circ \text{StripBOM} \circ \text{DecodeUTF8} \circ \text{CheckSize}$$

##### Dynamic Semantics

Source text preprocessing MUST execute in the following order:

| Step | Name                         | Operation                                    | Failure Diagnostic |
| :--- | :--------------------------- | :------------------------------------------- | :----------------- |
| 1    | Size Check                   | Verify file size ≤ documented maximum.       | `E-SRC-0102`       |
| 2    | UTF-8 Decoding               | Decode byte stream as UTF-8.                 | `E-SRC-0101`       |
| 3    | BOM Removal                  | Strip leading U+FEFF; reject embedded BOMs.  | `E-SRC-0103`       |
| 4    | Line Normalization           | Convert CR, LF, CRLF → LF.                   | —                  |
| 5    | Control Character Validation | Reject prohibited code points.               | `E-SRC-0104`       |
| 6    | Tokenization                 | Produce token stream from normalized source. | (per §2.4–§2.9)    |

Each step MUST complete successfully before the next step begins. The pipeline MUST be deterministic for a given source byte stream and compilation configuration.

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                               | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------------------ | :----------- | :-------- |
| `E-SRC-0101` | Error    | UTF-8 decoding failed during preprocessing.             | Compile-time | Rejection |
| `E-SRC-0102` | Error    | Source size exceeds implementation limit during step 1. | Compile-time | Rejection |
| `E-SRC-0103` | Error    | Embedded or misplaced BOM encountered in step 3.        | Compile-time | Rejection |
| `E-SRC-0104` | Error    | Prohibited control character encountered in step 5.     | Compile-time | Rejection |
| `E-SRC-0309` | Error    | Tokenization failed to classify a character sequence.   | Compile-time | Rejection |

---

### 2.4 Lexical Vocabulary

##### Definition

A **Token** is the atomic unit of lexical analysis. Tokenization transforms a normalized source file into a finite, ordered sequence of tokens.

**Formal Definition**

A token is a triple $t = (\kappa, \lambda, \sigma)$ where:
- $\kappa \in \mathcal{T} = \{\texttt{identifier}, \texttt{keyword}, \texttt{literal}, \texttt{operator}, \texttt{punctuator}, \texttt{newline}\}$
- $\lambda \in \mathcal{U}^*$ is the lexeme
- $\sigma \in \text{Span}$ is a source span (file, start position, end position)

##### Static Semantics

Every non-comment, non-whitespace fragment corresponds to exactly one token kind in $\mathcal{T}$.

Tokenization MUST be deterministic for a given normalized source file.

##### Constraints & Legality

Any maximal character sequence that cannot be classified MUST trigger diagnostic `E-SRC-0309`.

**Diagnostic Table**

| Code         | Severity | Condition                                    | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------- | :----------- | :-------- |
| `E-SRC-0309` | Error    | Unclassifiable character sequence in source. | Compile-time | Rejection |

---

### 2.5 Identifiers

##### Definition

An **Identifier** is a sequence of Unicode scalar values that names a declaration, binding, type, or other program entity, following Unicode Standard Annex #31 (UAX #31).

**Formal Definition**

$$\text{Ident} = \{s \in \mathcal{U}^+ : s_0 \in \text{XID\_Start} \cup \{\_\} \land \forall i > 0.\, s_i \in \text{XID\_Continue} \cup \{\_\}\}$$

##### Syntax & Declaration

```ebnf
<identifier>     ::= <ident_start> <ident_continue>*
<ident_start>    ::= /* Unicode XID_Start */ | "_"
<ident_continue> ::= /* Unicode XID_Continue */ | "_"
```

##### Static Semantics

Identifiers MUST NOT contain code points prohibited by §2.1, surrogate code points (U+D800–U+DFFF), or non-characters.

Reserved keywords (§2.6) MUST NOT be treated as identifiers.

**Name Equivalence**

Two identifiers are the same binding name if and only if their NFC-normalized forms (per §2.1) are identical.

##### Constraints & Legality

Implementations MUST accept identifiers whose length is at least the minimum specified in §1.4.

**Diagnostic Table**

| Code         | Severity | Condition                      | Detection    | Effect    |
| :----------- | :------- | :----------------------------- | :----------- | :-------- |
| `E-SRC-0307` | Error    | Invalid Unicode in identifier. | Compile-time | Rejection |

Reserved keyword violations are diagnosed per §1.3 (`E-CNF-0401`).

---

### 2.6 Keywords

##### Definition

A **Keyword** is a reserved lexeme that has special meaning in the language grammar and MUST NOT be used where an identifier is expected.

**Formal Definition**

Let $\mathcal{K}$ denote the set of reserved keywords. The complete enumeration is provided in the Static Semantics block below.

##### Static Semantics

The reserved keywords are:

```
and         as          async       atomic      break       class
comptime    const       continue    defer       dispatch    do
drop        else        emit        enum        escape
extern      false       for         gpu         if          import
in          interrupt   let         loop        match       modal
mod         module      move        mut         override    parallel
pool        private     procedure   protected   public      quote
record      region      result      return      select      self
Self        set         shared      simd        spawn       sync
then        transition  transmute   true        type        union
unique      unsafe      using       var         volatile    where
while       widen       yield
```

Implementations MUST tokenize these as `<keyword>`, not `<identifier>`. The keyword set MUST be identical across conforming implementations for a given language version. All keywords are unconditionally reserved in all syntactic contexts.

##### Constraints & Legality

Using a reserved keyword where an identifier is expected MUST trigger diagnostic `E-CNF-0401` (§1.3).

---

### 2.7 Operators and Punctuators

##### Definition

An **Operator** is a symbol or symbol sequence denoting an operation. A **Punctuator** is a symbol or symbol sequence used for syntactic structure.

**Formal Definition**

$$
\mathcal{O}_\text{multi} = \{\texttt{==}, \texttt{!=}, \texttt{<=}, \texttt{>=}, \texttt{\&\&}, \texttt{||}, \texttt{<<}, \texttt{>>}, \texttt{..}, \texttt{..=}, \texttt{=>}, \texttt{->}, \texttt{**}, \texttt{::}, \texttt{:=}, \texttt{|=}, \texttt{\~>}, \texttt{\~!}, \texttt{\~\%}
$$

$$
\mathcal{O}_\text{single} = \{+, -, *, /, \%, <, >, =, !, \&, |, \hat{}, \sim, .\}
$$

$$
\mathcal{P} = \{(, ), [, ], \{, \}, ,, :, ;\}
$$

##### Syntax & Declaration

**Multi-Character Tokens**

| Category          | Tokens                   |
| :---------------- | :----------------------- |
| Comparison        | `==`, `!=`, `<=`, `>=`   |
| Logical           | `&&`, `\|\|`             |
| Bitwise Shift     | `<<`, `>>`, `<<=`, `>>=` |
| Range             | `..`, `..=`              |
| Arrow             | `=>`, `->`               |
| Binding           | `:=`                     |
| Contract          | `\|=`                    |
| Other             | `**`, `::`               |
| Receiver/Dispatch | `~`, `~>`, `~!`, `~%`    |

**Single-Character Tokens**

```
+  -  *  /  %  <  >  =  !  &  |  ^  ~  .  ,  :  ;  (  )  [  ]  {  }  #  @  ?  !
```

**Receiver Token Lexemes**

| Token | Lexical Role                             |
| :---- | :--------------------------------------- |
| `~`   | Receiver reference in method context     |
| `~>`  | Lens operator / method dispatch operator |
| `~!`  | Permission-qualified receiver (unique)   |
| `~%`  | Permission-qualified receiver (shared)   |

**Region Operators**

| Token | Lexical Role                                  |
| :---- | :-------------------------------------------- |
| `r^`  | Region-prefixed allocation (e.g., `r^data()`) |
| `^`   | Implicit region allocation (innermost named)  |

**Binding Operator Semantics**

| Token | Semantic Meaning                                                       |
| :---- | :--------------------------------------------------------------------- |
| `=`   | Establishes movable responsibility (ownership may transfer via `move`) |
| `:=`  | Establishes immovable responsibility (ownership cannot transfer)       |

**Contract Operator Semantics**

| Token | Semantic Meaning                                                  |
| :---- | :---------------------------------------------------------------- |
| `\|=` | Introduces procedure contract clause (semantic entailment symbol) |

**Punctuator Semantics**

| Token | Syntactic Role                                                                                     |
| :---- | :------------------------------------------------------------------------------------------------- |
| `;`   | Statement terminator (§2.11); Generic parameter and where predicate separator (§7.1)               |
| `,`   | Element separator in tuples, arrays, argument lists; Bound separator in generic constraints (§7.1) |
| `:`   | Type annotation; Label prefix; Record field separator                                              |

##### Static Semantics

**Maximal Munch Rule**

When multiple tokenizations are possible, the longest valid token takes precedence.

**Generic Argument Exception**

When `>>` appears in a context where it would close two or more nested generic parameter lists, `>>` is treated as two separate `>` tokens. The original lexeme span is preserved for diagnostic purposes. This exception applies recursively: `>>>` is treated as three `>` tokens when closing three nested generic parameter lists.

---

### 2.8 Literals

##### Definition

A **Literal** is a token representing a compile-time constant value. Cursive supports integer, floating-point, string, character, and boolean literals.

**Formal Definition**

$$\mathcal{L} = \mathcal{L}_\text{int} \cup \mathcal{L}_\text{float} \cup \mathcal{L}_\text{string} \cup \mathcal{L}_\text{char} \cup \mathcal{L}_\text{bool}$$

Each subset is defined by the grammar below.

##### Syntax & Declaration

**Integer Literals**

```ebnf
<integer_literal>  ::= <decimal_integer> | <hex_integer>
                     | <octal_integer> | <binary_integer>
<decimal_integer>  ::= <dec_digit> ("_"* <dec_digit>)*
<hex_integer>      ::= "0x" <hex_digit> ("_"* <hex_digit>)*
<octal_integer>    ::= "0o" <oct_digit> ("_"* <oct_digit>)*
<binary_integer>   ::= "0b" <bin_digit> ("_"* <bin_digit>)*

<dec_digit>        ::= "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9"
<hex_digit>        ::= <dec_digit> | "a" | "b" | "c" | "d" | "e" | "f"
                     | "A" | "B" | "C" | "D" | "E" | "F"
<oct_digit>        ::= "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7"
<bin_digit>        ::= "0" | "1"
```

**Floating-Point Literals**

```ebnf
<float_literal> ::= <decimal_integer> "." <decimal_integer>? <exponent>? <suffix>?
<exponent>      ::= ("e" | "E") ("+" | "-")? <decimal_integer>
<suffix>        ::= "f16" | "f32" | "f64"
```

**String Literals**

```ebnf
<string_literal>   ::= '"' (<string_char> | <escape_sequence>)* '"'
<string_char>      ::= /* any Unicode scalar value except ", \, or U+000A */
<escape_sequence>  ::= "\n" | "\r" | "\t" | "\\" | "\"" | "\'" | "\0"
                     | "\x" <hex_digit> <hex_digit>
                     | "\u{" <hex_digit>+ "}"
```

**Character Literals**

```ebnf
<char_literal>     ::= "'" (<char_content> | <escape_sequence>) "'"
<char_content>     ::= /* any Unicode scalar value except ', \, or U+000A */
```

##### Static Semantics

Underscores in numeric literals MUST NOT appear at the beginning or end, immediately after a base prefix, adjacent to an exponent marker, or before a type suffix.

Based integer literals MUST begin with the appropriate prefix (`0x`, `0o`, `0b`) followed by at least one valid digit.

A string literal MUST begin with `"` and end with a matching unescaped `"`. A line feed or end-of-file before the closing quote MUST trigger `E-SRC-0301`.

A character literal MUST represent exactly one Unicode scalar value. Empty or multi-value character literals MUST trigger `E-SRC-0303`.

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                         | Detection    | Effect    |
| :----------- | :------- | :-------------------------------- | :----------- | :-------- |
| `E-SRC-0301` | Error    | Unterminated string literal.      | Compile-time | Rejection |
| `E-SRC-0302` | Error    | Invalid escape sequence.          | Compile-time | Rejection |
| `E-SRC-0303` | Error    | Invalid character literal.        | Compile-time | Rejection |
| `E-SRC-0304` | Error    | Malformed numeric literal.        | Compile-time | Rejection |
| `W-SRC-0301` | Warning  | Leading zeros in decimal literal. | Compile-time | N/A       |

---

### 2.9 Whitespace and Comments

##### Definition

**Whitespace** characters are code points that serve as token separators. **Comments** are lexical constructs for human-readable annotations that do not contribute to the token stream.

**Formal Definition**

$$\mathcal{W} = \{\text{U+0020}, \text{U+0009}, \text{U+000C}\}$$

Whitespace characters separate tokens but are not emitted. Line feed (U+000A) is emitted as a `<newline>` token.

##### Static Semantics

**Whitespace Characters**

| Character      | Code Point | Treatment                    |
| :------------- | :--------- | :--------------------------- |
| Space          | U+0020     | Token separator; not emitted |
| Horizontal Tab | U+0009     | Token separator; not emitted |
| Form Feed      | U+000C     | Token separator; not emitted |
| Line Feed      | U+000A     | Emitted as `<newline>` token |

**Line Comments**

A line comment begins with `//` outside a literal and extends to but does not include the next U+000A or end-of-file.

**Documentation Comments**

Comments beginning with `///` or `//!` are documentation comments. Implementations MUST preserve documentation comments and associate them with their following declaration (`///`) or enclosing module (`//!`). Documentation comments MUST NOT appear in the token stream. Documentation extraction and rendering are implementation-defined.

**Block Comments**

A block comment is delimited by `/*` and `*/`. Block comments MUST nest: each `/*` increases nesting depth, each `*/` decreases it. End-of-file with non-zero nesting depth MUST trigger `E-SRC-0306`.

Characters within comments MUST NOT contribute to token formation, delimiter nesting, or statement termination.

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                   | Detection    | Effect    |
| :----------- | :------- | :-------------------------- | :----------- | :-------- |
| `E-SRC-0306` | Error    | Unterminated block comment. | Compile-time | Rejection |

---

### 2.10 Lexical Security

##### Definition

**Lexically Sensitive Characters** are Unicode code points that may alter the visual appearance of source code without changing its tokenization.

**Formal Definition**

$$\mathcal{S} = \{\text{U+202A–U+202E}\} \cup \{\text{U+2066–U+2069}\} \cup \{\text{U+200C}, \text{U+200D}\}$$

##### Static Semantics

**Sensitive Character Categories**

The following Unicode categories contain lexically sensitive characters:

| Category              | Unicode Property       |
| :-------------------- | :--------------------- |
| Bidirectional Control | Bidi_Control           |
| Zero-Width Characters | General_Category=Cf    |
| Homoglyphs            | Confusable mappings    |
| Combining Characters  | General_Category=Mn/Mc |

**Enforcement Modes**

When a character in $\mathcal{S}$ appears unescaped in an identifier, operator/punctuator, or adjacent to token boundaries outside literals and comments, the implementation MUST emit a diagnostic. Severity depends on conformance mode:

| Mode       | Behavior                                  | Diagnostic |
| :--------- | :---------------------------------------- | :--------- |
| Permissive | Issue warning `W-SRC-0308`; accept source | Warning    |
| Strict     | Issue error `E-SRC-0308`; reject source   | Error      |

The default mode is IDB. Implementations MUST provide a mechanism to select either mode.

Lexically sensitive characters within string or character literals MUST NOT affect well-formedness.

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                           | Detection    | Effect    |
| :----------- | :------- | :-------------------------------------------------- | :----------- | :-------- |
| `W-SRC-0308` | Warning  | Lexically sensitive Unicode character (Permissive). | Compile-time | N/A       |
| `E-SRC-0308` | Error    | Lexically sensitive Unicode character (Strict).     | Compile-time | Rejection |

---

### 2.11 Statement Termination

##### Definition

A **Statement Terminator** marks the end of a statement. Cursive supports explicit termination (`;`) and implicit termination (`<newline>`).

**Formal Definition**

$$\text{Terminates}(t, L) = \begin{cases}
\texttt{true} & \text{if } t = \texttt{`;'} \\
\texttt{true} & \text{if } t = \texttt{<newline>} \land \neg\text{Continues}(L) \\
\texttt{false} & \text{otherwise}
\end{cases}$$

##### Static Semantics

**Line Continuation Rules**

A `<newline>` does NOT terminate a statement ($\text{Continues}(L) = \texttt{true}$) when any of the following conditions hold:

| Condition            | Description                                                              |
| :------------------- | :----------------------------------------------------------------------- |
| Open Delimiter       | The newline appears inside an unclosed `()`, `[]`, or `{}`.              |
| Trailing Operator    | The last non-comment token on line $L$ is a binary operator or `,`.      |
| Leading Continuation | The first non-comment token on the following line is `.`, `::`, or `~>`. |

**Disambiguation**: Tokens in the set $\{+, -, *, \&, |\}$ may function as either binary or unary operators. When such a token appears at the end of line $L$ and the first token on line $L+1$ is an operand, the token is interpreted as a binary operator (triggering continuation per the Trailing Operator condition). To force termination before a unary operator, insert an explicit semicolon.

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                                                                        | Detection    | Effect    |
| :----------- | :------- | :----------------------------------------------------------------------------------------------- | :----------- | :-------- |
| `E-SRC-0510` | Error    | Missing statement terminator (no `;` and newline is not a terminator by rules in this section).  | Compile-time | Rejection |
| `E-SRC-0511` | Error    | Line continuation used where none of the continuation conditions apply (newline must terminate). | Compile-time | Rejection |

---

### 2.12 Translation Phases

##### Definition

A **Translation Phase** is a discrete stage in the compilation pipeline. Compilation operates on **Compilation Units** as defined in §8.1 (Module System Architecture).

**Formal Definition**

$$\text{Translate} : \text{CompilationUnit} \to \text{Executable} \cup \{\bot\}$$

The translation pipeline is the sequential composition of four phases, where $\bot$ indicates failure:

$$\text{Translate} = \text{Parse} \triangleright \text{ComptimeExec} \triangleright \text{TypeCheck} \triangleright \text{CodeGen}$$

where $f \triangleright g$ denotes "apply $f$, then apply $g$" (left-to-right sequencing).

##### Static Semantics

Phase 1 MUST complete for all source files in a compilation unit before Phase 2 begins. Forward references to declarations within the same compilation unit MUST be permitted.

Cursive provides no C-style textual inclusion (`#include`) or preprocessing (`#define`).

##### Dynamic Semantics

Compilation MUST proceed through the following phases:

| Phase | Name                   | Description                                              |
| :---- | :--------------------- | :------------------------------------------------------- |
| 1     | Parsing                | Source → AST; all declarations recorded.                 |
| 2     | Compile-Time Execution | `comptime` blocks executed; may generate declarations.   |
| 3     | Type Checking          | Semantic validation including permissions and contracts. |
| 4     | Code Generation        | AST → intermediate representation → machine code.        |

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                    | Detection    | Effect    |
| :----------- | :------- | :--------------------------- | :----------- | :-------- |
| `E-MOD-1301` | Error    | Unresolved symbol reference. | Compile-time | Rejection |

---

### 2.13 Syntactic Nesting Limits

##### Definition

**Block Nesting Depth** is the count of lexically nested block statements. **Expression Nesting Depth** is the count of lexically nested expressions.

**Formal Definition**

For any syntactic position $p$:

$$\text{BlockDepth}(p) = |\{b : b \text{ is a block ancestor of } p\}| \leq \text{BLOCK\_LIMIT}$$

$$\text{ExprDepth}(p) = |\{e : e \text{ is an expression ancestor of } p\}| \leq \text{EXPR\_LIMIT}$$

where BLOCK_LIMIT and EXPR_LIMIT are the implementation's documented limits (minimum 256 per §1.4).

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                          | Detection    | Effect    |
| :----------- | :------- | :--------------------------------- | :----------- | :-------- |
| `E-SRC-0501` | Error    | Block nesting depth exceeded.      | Compile-time | Rejection |
| `E-SRC-0502` | Error    | Expression nesting depth exceeded. | Compile-time | Rejection |

---

## Clause 3: The Object and Memory Model

### 3.1 Foundational Principles

##### Definition

**Liveness**: The property that all pointers and bindings refer to allocated, initialized memory.

**Aliasing**: The property that shared or reentrant access to the same memory location does not violate data integrity.

A program is **memory-safe** if and only if it satisfies both the Liveness property and the Aliasing property.

##### Static Semantics

**Liveness Enforcement**

Liveness is enforced through RAII and deterministic destruction (§3.6), modal pointer states (§6.3), and region escape analysis (§3.3).

**Aliasing Enforcement**

Aliasing is enforced through permission types (§4.5) and the key-based concurrency model (§14.1).

---

### 3.2 The Object Model

##### Definition

An **Object** is the fundamental unit of typed storage in Cursive, defined by a quadruple:

$$\text{Object} ::= (\text{Storage}, \text{Type}, \text{Lifetime}, \text{Value})$$

**Storage Duration** classifies an object's lifetime based on when and how its storage is allocated and deallocated:

$$\text{StorageDuration} ::= \text{Static} \mid \text{Automatic} \mid \text{Region}(R) \mid \text{Dynamic}$$

##### Static Semantics

**Object Lifecycle**

Every object passes through exactly three phases:

1.  **Allocation and Initialization**: Storage is reserved and a valid initial value is written.
2.  **Usage**: The object's value may be read or modified via valid bindings or pointers, subject to permission constraints (§4.5).
3.  **Destruction and Deallocation**: If the object's type implements `Drop` (§9.7), its destructor is invoked. Storage is then released. (`Drop` is a class  that defines custom destruction logic; see §9.7 for the complete specification.)

Storage duration categories correspond to provenance tags (§3.3):

- Static → $\pi_{\text{Global}}$
- Automatic (in scope $S$) → $\pi_{\text{Stack}}(S)$
- Region($R$) → $\pi_{\text{Region}}(R)$
- Dynamic → $\pi_{\text{Heap}}$

##### Memory & Layout

The size, alignment, and internal layout of objects are governed by their types. Universal layout principles are defined in §5 (Data Types), and the `[[layout(...)]]` attribute is specified in §7.2.1.

---

### 3.3 The Provenance Model

##### Definition

A **Provenance Tag** ($\pi$) is a compile-time annotation attached to every pointer and reference type, indicating the origin of the address.

**Formal Definition**

$$\pi ::= \pi_{\text{Global}} \mid \pi_{\text{Stack}}(S) \mid \pi_{\text{Heap}} \mid \pi_{\text{Region}}(R) \mid \bot$$

where:
-   $\pi_{\text{Global}}$: Addresses of static storage.
-   $\pi_{\text{Stack}}(S)$: Addresses of automatic storage in scope $S$.
-   $\pi_{\text{Heap}}$: Addresses of dynamically allocated storage.
-   $\pi_{\text{Region}}(R)$: Addresses of storage allocated in region $R$.
-   $\bot$: No provenance (literals and constants). Values with $\bot$ provenance have unbounded lifetime and can be stored anywhere.

**Lifetime Partial Order**

Provenance tags form a partial order ($\le$) based on containment:

$$\pi_{\text{Region}}(\text{Inner}) < \pi_{\text{Region}}(\text{Outer}) < \pi_{\text{Stack}}(S) < \pi_{\text{Heap}} \le \pi_{\text{Global}} \le \bot$$

The relation $\pi_A \le \pi_B$ holds if and only if storage with provenance $\pi_A$ is guaranteed to be deallocated no later than storage with provenance $\pi_B$.

The distinction between strict ordering ($<$) and non-strict ordering ($\le$) is significant:

- **Strict ordering ($<$):** The left provenance is *guaranteed* to be deallocated *before* the right provenance. Values with the shorter-lived provenance MUST NOT escape to locations with longer-lived provenance.

- **Non-strict ordering ($\le$):** The provenances may have equivalent lifetimes. Specifically, $\pi_{\text{Heap}} \le \pi_{\text{Global}}$ because heap-allocated storage with static lifetime (e.g., intentional memory leaks, process-lifetime allocations) may persist until program termination, matching global storage duration.

**Constraint:** Heap-allocated storage whose address has been stored in a binding with global provenance MUST NOT be deallocated during program execution. Deallocating such storage constitutes Unverifiable Behavior (UVB). Violation in `unsafe` code constitutes UVB (§1.2).

##### Static Semantics

**Provenance Inference Rules**

Provenance MUST be inferred for all pointer and reference expressions:

**(P-Literal)**
$$\frac{c \text{ is a literal constant}}{\Gamma \vdash c : T, \bot}$$

**(P-Global)**
$$\frac{\Gamma \vdash \texttt{static } x : T}{\Gamma \vdash x : T, \pi_{\text{Global}}}$$

**(P-Local)**
$$\frac{\Gamma \vdash \texttt{let } x : T = e \quad x \text{ declared in scope } S}{\Gamma \vdash x : T, \pi_{\text{Stack}}(S)}$$

**(P-Region-Alloc)**
$$\frac{\Gamma \vdash e : T \quad R \text{ is the innermost active region}}{\Gamma \vdash \hat{\ } e : T, \pi_{\text{Region}}(R)}$$

**(P-Field)**
$$\frac{\Gamma \vdash e : \texttt{record } T, \pi \quad f \in \text{fields}(T)}{\Gamma \vdash e.f : \text{typeof}(f), \pi}$$

**(P-Index)**
$$\frac{\Gamma \vdash e : [T; N], \pi \quad \Gamma \vdash i : \texttt{usize}}{\Gamma \vdash e[i] : T, \pi}$$

**(P-Deref)**
$$\frac{\Gamma \vdash p : \texttt{Ptr}\langle T \rangle @\texttt{Valid}, \pi_p \quad \text{target}(p) \text{ has provenance } \pi_t}{\Gamma \vdash *p : T, \pi_t}$$

**The Escape Rule**

A value with provenance $\pi_e$ MUST NOT be assigned to a location with provenance $\pi_x$ if $\pi_e < \pi_x$:

$$\frac{\Gamma \vdash e : T, \pi_e \quad \Gamma \vdash x : T, \pi_x \quad \pi_e < \pi_x}{\text{ill-formed}: \texttt{E-MEM-3020}}$$

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                                             | Detection    | Effect    |
| :----------- | :------- | :-------------------------------------------------------------------- | :----------- | :-------- |
| `E-MEM-3020` | Error    | Value with shorter-lived provenance escapes to longer-lived location. | Compile-time | Rejection |

---

### 3.4 The Binding Model

##### Definition

A **Binding** is a named association between an identifier and an object. Bindings are introduced by `let` and `var` declarations, pattern matching, and procedure parameters.

A **Responsible Binding** is a binding that manages the lifecycle of its associated object. When a responsible binding goes out of scope, it triggers destruction of the object (§3.6).

A **Movable Binding** is a binding established with the `=` operator. Responsibility for the bound object can be transferred via `move`.

An **Immovable Binding** is a binding established with the `:=` operator. Responsibility for the bound object CANNOT be transferred. Attempting to `move` from an immovable binding is a compile-time error.

**Binding State**

$$\text{BindingState} ::= \text{Uninitialized} \mid \text{Valid} \mid \text{Moved} \mid \text{PartiallyMoved}$$

A **Temporary Value** is an object resulting from expression evaluation that is not immediately bound to a named identifier.

##### Syntax & Declaration

```ebnf
<binding_decl>     ::= <let_decl> | <var_decl>
<binding_op>       ::= "=" | ":="
<let_decl>         ::= "let" <pattern> (":" <type>)? <binding_op> <expression>
<var_decl>         ::= "var" <pattern> (":" <type>)? <binding_op> <expression>
```

See §12.2 for the `<pattern>` production.

##### Static Semantics

**Binding Mutability**

`let` establishes an immutable binding; `var` establishes a mutable binding. Binding mutability is orthogonal to data permission (§4.5) and responsibility transferability.

**Binding Movability**

The binding operator determines whether responsibility may be transferred:

| Declaration  | Reassignable | Movable | Responsibility                      |
| :----------- | :----------- | :------ | :---------------------------------- |
| `let x = v`  | No           | Yes     | Transferable via `move`             |
| `let x := v` | No           | No      | Permanently fixed to `x`            |
| `var x = v`  | Yes          | Yes     | Transferable via `move`             |
| `var x := v` | Yes          | No      | Fixed; reassignment drops old value |

For `var x := v`: reassignment is permitted but `move` is not. Each reassignment drops the previous value and fixes the new value's responsibility to `x`.

**Binding Annotation Notation**

The typing rules below use a decorated type notation to track binding state and properties:

$$T^{\text{State}}_{\text{properties}}$$

where:

- **Superscript** ($^{\text{State}}$) denotes the **binding state** from the BindingState enumeration defined above:
  - $^{\text{Valid}}$: The binding holds an initialized, accessible value.
  - $^{\text{Moved}}$: The binding's value has been moved; access is statically prohibited.
  - $^{\text{PartiallyMoved}}$: Some fields have been moved; the binding is partially accessible.

- **Subscript** ($_{\text{properties}}$) denotes **binding properties** as a comma-separated list:
  - $_{\text{mov}}$: The binding is movable (established with `=`).
  - $_{\text{immov}}$: The binding is immovable (established with `:=`).
  - $_{\text{mut}}$: The binding is mutable (declared with `var`).

**Typing Rules**

**(T-Let-Movable)**
$$\frac{\Gamma \vdash e : T}{\Gamma, x : T \vdash \texttt{let } x = e : () \dashv \Gamma, x : T^{\text{Valid}}_{\text{mov}}}$$

**(T-Let-Immovable)**
$$\frac{\Gamma \vdash e : T}{\Gamma, x : T \vdash \texttt{let } x := e : () \dashv \Gamma, x : T^{\text{Valid}}_{\text{immov}}}$$

**(T-Var-Movable)**
$$\frac{\Gamma \vdash e : T}{\Gamma, x : T \vdash \texttt{var } x = e : () \dashv \Gamma, x : T^{\text{Valid}}_{\text{mut,mov}}}$$

**(T-Var-Immovable)**
$$\frac{\Gamma \vdash e : T}{\Gamma, x : T \vdash \texttt{var } x := e : () \dashv \Gamma, x : T^{\text{Valid}}_{\text{mut,immov}}}$$

**State Tracking**

Binding state MUST be tracked through control flow. A binding's state is determined by its declaration, any `move` expressions consuming the binding, partial moves consuming fields, and reassignment of `var` bindings.

**Control Flow Merge**

At control flow merge points (after `if`/`else`, `match` arms, loop exits), binding state is the least-valid state from any branch. The state ordering from most-valid to least-valid is: Valid > PartiallyMoved > Moved. If a binding is Valid in one branch and Moved in another, the merged state is Moved.

**Responsibility and Move Semantics**

Both binding operators create a responsible binding; the mechanics of responsibility transfer via `move` are defined in §3.5. The key distinction:

- **`=` (movable)**: Responsibility may be transferred per §3.5.
- **`:=` (immovable)**: Responsibility is permanently fixed; `move` is prohibited.

**Temporary Lifetime**

Temporaries have a lifetime extending from their creation to the end of the innermost enclosing statement. If a temporary is used to initialize a `let` or `var` binding, its lifetime is promoted to match the binding's scope. Temporaries that are not moved or bound MUST be destroyed at statement end, in reverse order of creation.

##### Constraints & Legality

Binding state diagnostics are consolidated in §3.5.

---

### 3.5 Responsibility and Move Semantics

##### Definition

A **Move** is an operation that transfers responsibility for an object from one binding to another. After a move, the source binding is invalidated and the destination binding becomes responsible for the object's lifecycle.

A move operation is only permitted from **movable bindings** (§3.4). Attempting to move from an immovable binding MUST be rejected with diagnostic `E-MEM-3006`.

**Formal Definition**

$$\text{move} : (\Gamma, x : T^{\text{Valid}}_{\text{mov}}) \to (\Gamma[x \mapsto T^{\text{Moved}}], T)$$

The move operation takes a context $\Gamma$ containing a valid, movable binding $x$ of type $T$, and produces:
1. An updated context where $x$ is in the Moved state
2. The value of type $T$ (now available for binding elsewhere)

The subscript $_{\text{mov}}$ indicates the binding must be movable (established with `=`, not `:=`).

##### Syntax & Declaration

```ebnf
<place_expr>       ::= <identifier>
                     | <place_expr> "." <identifier>
                     | <place_expr> "[" <expr> "]"
                     | "*" <expr>

<move_expr>        ::= "move" <place_expr>
<partial_move>     ::= "move" <place_expr> "." <identifier>
```

A `<partial_move>` is a specific form of `<move_expr>` for moving individual fields from a record. See §0.1 for the definition of place expression.

##### Static Semantics

**Typing Rules**

**(T-Move)**
$$\frac{\Gamma \vdash x : T \quad \text{state}(\Gamma, x) = \text{Valid} \quad \text{movable}(\Gamma, x)}{\Gamma \vdash \texttt{move } x : T \dashv \Gamma[x \mapsto \text{Moved}]}$$

Where $\text{movable}(\Gamma, x)$ holds iff $x$ was bound with `=`, not `:=`.

**(T-Move-Field)**
$$\frac{\Gamma \vdash x : R \quad R = \texttt{record} \{ \ldots, f : T, \ldots \} \quad \text{perm}(\Gamma, x) = \text{unique}}{\Gamma \vdash \texttt{move } x.f : T \dashv \Gamma[x \mapsto \text{PartiallyMoved}(f)]}$$

**Move Semantics**

`move x` transitions the source binding `x` to the Moved state. Access to a Moved binding is forbidden. A `var` binding in Moved state MAY be reassigned, restoring it to Valid state. A `let` binding in Moved state MUST NOT be reassigned.

**Partial Move Constraints**

Partial moves are permitted ONLY when both of the following conditions are satisfied:

1. The binding is **movable** (established with `=`, not `:=`). Immovable bindings prohibit all `move` operations, including partial moves.
2. The binding has **`unique` permission**, providing exclusive access to the record's fields.

After a partial move, the parent binding enters the PartiallyMoved state. If the binding is a mutable `var`, reassignment restores it to Valid state (§3.4). A `let` binding in PartiallyMoved state cannot be restored and remains invalid for the remainder of its scope. Any read or move of a PartiallyMoved binding is a compile-time error diagnosed as `E-MEM-3001`.

**Parameter Responsibility**

A **Transferring Parameter** is declared with the `move` modifier. The callee assumes responsibility; the caller MUST apply `move` at the call site; the source binding is invalidated after the call.

A **Non-Transferring Parameter** is declared without `move`. The callee receives a view; responsibility remains with the caller; the source binding remains valid.

##### Constraints & Legality

**Binding State Diagnostic Table**

This table consolidates all diagnostics related to binding state violations (§3.4 and §3.5):

| Code         | Severity | Condition                                                    | Detection    | Effect    |
| :----------- | :------- | :----------------------------------------------------------- | :----------- | :-------- |
| `E-MEM-3001` | Error    | Read or move of a binding in Moved or PartiallyMoved state.  | Compile-time | Rejection |
| `E-MEM-3002` | Error    | Access to binding in Uninitialized state.                    | Compile-time | Rejection |
| `E-MEM-3003` | Error    | Reassignment of immutable binding.                           | Compile-time | Rejection |
| `E-MEM-3004` | Error    | Partial move from binding without `unique` permission.       | Compile-time | Rejection |
| `E-MEM-3006` | Error    | Attempt to move from immovable binding (`:=`).               | Compile-time | Rejection |
| `E-MOD-2411` | Error    | Missing `move` keyword at call site for consuming parameter. | Compile-time | Rejection |

---

### 3.6 Deterministic Destruction

##### Definition

**RAII (Resource Acquisition Is Initialization)** is the pattern where resources are acquired during object initialization and released during object destruction. Destruction in Cursive is deterministic—it occurs at a statically known program point.

**Drop Order** specifies that bindings MUST be destroyed in strict LIFO order relative to their declaration within a scope.

##### Dynamic Semantics

**Normal Destruction**

At scope exit, all responsible bindings MUST be destroyed in reverse declaration order:

1.  If the binding's type implements `Drop` (§9.7), invoke `Drop::drop`.
2.  Recursively destroy all fields and elements.
3.  Release storage.

**Unwinding**

During panic propagation, destructors for all in-scope responsible bindings MUST be invoked in LIFO order as each frame is unwound.

**Double Panic**

If a destructor panics while the thread is already unwinding, the runtime MUST abort the process immediately.

##### Constraints & Legality

Explicit calls to `Drop::drop` are forbidden. To destroy an object before scope exit, use the `mem::drop` utility procedure.

**Diagnostic Table**

| Code         | Severity | Condition                             | Detection    | Effect    |
| :----------- | :------- | :------------------------------------ | :----------- | :-------- |
| `E-MEM-3005` | Error    | Explicit call to `Drop::drop` method. | Compile-time | Rejection |

---

### 3.7 Regions

##### Definition

A **Region** is a lexical scope that provides arena-style memory allocation with
deterministic bulk deallocation. Data allocated in a region lives until the region
exits.

**Region** provides **memory lifetime** management:
- Arena-style bulk allocation with deterministic deallocation
- LIFO lifetime ordering relative to nested regions

##### Syntax & Declaration

**Grammar**

```ebnf
<region_stmt>      ::= "region" <region_options>? <region_alias>? <block>

<region_options>   ::= <region_option>*
<region_option>    ::= "." "stack_size" "(" <expr> ")"
                     | "." "name" "(" <expr> ")"

<region_alias>     ::= "as" <identifier>

<alloc_expr>       ::= <identifier> "^" <expression>
```

Execution modifiers are specified on `parallel` blocks (§14.3), not regions.

**Region Allocation**

The `^` operator allocates storage in a named region. The region identifier MUST be provided explicitly: `r^expr` allocates in the region `r`.

##### Static Semantics

**Typing Rules**

**(T-Region-Seq)**
$$\frac{\Gamma, r : \texttt{Region@Active} \vdash \textit{body} : T}{\Gamma \vdash \texttt{region as } r \{ \textit{body} \} : T}$$

**(T-Alloc)**
$$\frac{\Gamma(r) = \texttt{Region@Active} \quad \Gamma \vdash e : T}{\Gamma \vdash r\texttt{\^{}}e : T_{\pi_r}}$$

##### Dynamic Semantics

**Region Entry**

Entering `region as r { body }` allocates a region frame `r` and marks it Active for the dynamic extent of `body`.

**Allocation Lifetime**

1. `r^expr` allocates storage from the explicitly named region `r`.
2. Each allocation returns an initialized object whose lifetime is bounded by the lifetime of the region that supplied the storage.

**Region Exit and Deallocation Order**

1. On normal exit from the region block, all objects allocated in the region are destroyed, then the region storage is released in bulk.
2. Destruction order within a region is LIFO with respect to allocation order. Nested regions are destroyed before their parent region resumes destruction (structured stack discipline).
3. On unwinding (panic), the same destruction and bulk deallocation sequence occurs before unwinding continues to the enclosing region or scope.

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                             | Detection    | Effect    |
| :----------- | :------- | :------------------------------------ | :----------- | :-------- |
| `E-MEM-1206` | Error    | Named region not found for allocation | Compile-time | Rejection |

---

### 3.8 Unsafe Memory Operations

##### Definition

An **Unsafe Block** is a lexical scope in which specific memory safety enforcement mechanisms are suspended.

##### Syntax & Declaration

```ebnf
<unsafe_block>     ::= "unsafe" <block>
<transmute_expr>   ::= "transmute" "::" "<" <type> "," <type> ">" "(" <expr> ")"
```

##### Static Semantics

**Typing Rules**

**(T-Unsafe)**
$$\frac{\Gamma \vdash_{\text{unsafe}} \textit{body} : T}{\Gamma \vdash \texttt{unsafe } \{ \textit{body} \} : T}$$

**(T-Transmute)**
$$\frac{\Gamma \vdash_{\text{unsafe}} e : S \quad \text{sizeof}(S) = \text{sizeof}(T)}{\Gamma \vdash_{\text{unsafe}} \texttt{transmute}::\langle S, T \rangle(e) : T}$$

**Suspended Checks**

Within an `unsafe` block, liveness, aliasing exclusivity, and key disjointness SHALL NOT be enforced.

**Enabled Operations**

The following operations are permitted ONLY within an `unsafe` block: raw pointer dereference, unsafe procedure calls, `transmute`, and pointer arithmetic. Raw pointer types (`*imm T`, `*mut T`) are defined in §6.3.

**Transmute Constraints**

`sizeof(S)` MUST equal `sizeof(T)`. The programmer MUST guarantee the bit pattern is valid for the target type; creating an invalid value constitutes Unverifiable Behavior (UVB).

##### Constraints & Legality

The use of an `unsafe` block constitutes a normative assertion by the programmer that the code within satisfies all memory safety invariants. The effects of `unsafe` code MUST NOT compromise the safety of safe code outside the block.

**Diagnostic Table**

| Code         | Severity | Condition                       | Detection    | Effect    |
| :----------- | :------- | :------------------------------ | :----------- | :-------- |
| `E-MEM-3030` | Error    | Unsafe operation outside block. | Compile-time | Rejection |
| `E-MEM-3031` | Error    | Transmute size mismatch.        | Compile-time | Rejection |

---

# Part 2: Types

---


## Clause 4: Type System Foundations

### 4.1 Type System Architecture

##### Static Semantics

**Static Type Checking**

Cursive MUST be implemented as a statically typed language. All type checking MUST be performed at compile time. A program that fails type checking MUST be diagnosed as ill-formed and rejected.

**The type system is primarily **nominal**. Two types with different names are never equivalent, even if their structure is identical. The following constructs are nominal:

- Record types (§5.3)
- Enum types (§5.4)
- Modal types (§6.1)

The following constructs are **structural**. Their equivalence is determined by their composition:

- Tuple types (§5.2.1)
- Anonymous union types (§5.5)
- Function types (§6.4)
- Array and slice types (§5.2.2, §5.2.3)

**The Type Context ($\Gamma$)**

The **type context** is the environment mapping identifiers to types during type checking:

$$\Gamma ::= \emptyset \mid \Gamma, x : T$$

where $\emptyset$ denotes the empty context and $\Gamma, x : T$ extends context $\Gamma$ by binding identifier $x$ to type $T$. The judgment $\Gamma \vdash e : T$ asserts that expression $e$ has type $T$ in context $\Gamma$.

**Type Equivalence ($\equiv$)**

Two types $T$ and $U$ are equivalent, written $T \equiv U$, if one of the following rules holds. Type equivalence MUST be reflexive, symmetric, and transitive.

**(T-Equiv-Nominal)** Nominal Equivalence:
$$\frac{D(T) = D(U)}{\Gamma \vdash T \equiv U}$$

Two nominal types are equivalent if and only if they refer to the same declaration $D$.

**(T-Equiv-Tuple)** Tuple Equivalence:
$$\frac{T = (T_1, \ldots, T_n) \quad U = (U_1, \ldots, U_n) \quad \forall i \in 1..n,\ \Gamma \vdash T_i \equiv U_i}{\Gamma \vdash T \equiv U}$$

**(T-Equiv-Func)** Function Type Equivalence:
$$\frac{T = (P_1, \ldots, P_n) \to R_T \quad U = (Q_1, \ldots, Q_n) \to R_U \quad \Gamma \vdash R_T \equiv R_U \quad \forall i,\ \Gamma \vdash P_i \equiv Q_i}{\Gamma \vdash T \equiv U}$$

This rule defines equivalence for the abstract calling signature. For full function type equivalence including representation kind (sparse function pointer vs. closure) and `move` parameter modifiers:

$$\frac{\begin{gathered}
\kappa_F = \kappa_G \quad n = k \\
\forall i \in 1..n,\ (m_i = m'_i \land \Gamma \vdash T_i \equiv U_i) \\
\Gamma \vdash R_F \equiv R_G
\end{gathered}}{\Gamma \vdash F \equiv G} \quad \text{(T-Equiv-Func-Extended)}$$

where $F = (\kappa_F,\ m_1\ T_1, \ldots, m_n\ T_n,\ R_F)$ and $G = (\kappa_G,\ m'_1\ U_1, \ldots, m'_k\ U_k,\ R_G)$, $\kappa$ denotes representation kind (sparse function pointer or closure), and $m_i$ denotes `move` modifiers. See §6.4 for representation kind semantics.

**(T-Equiv-Union)** Union Type Equivalence:
$$\frac{\text{multiset\_equiv}(\text{members}(T), \text{members}(U))}{\Gamma \vdash T \equiv U}$$

Union types are equivalent if their member type multisets are equal (order-independent).

**(T-Equiv-Permission)** Permission Equivalence:
$$\frac{P_1 = P_2 \quad \Gamma \vdash T \equiv U}{\Gamma \vdash P_1\ T \equiv P_2\ U}$$

The complete permission system is defined in §4.5.

##### Constraints & Legality

Top-level declarations (procedures, module-scope bindings, type definitions) MUST include explicit type annotations. Local bindings within procedure bodies MAY omit type annotations when the type is inferable (§4.4).

**Diagnostic Table**

| Code         | Severity | Condition                                         | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------------ | :----------- | :-------- |
| `E-TYP-1501` | Error    | Type mismatch in expression or assignment.        | Compile-time | Rejection |
| `E-TYP-1502` | Error    | No valid type derivable for expression.           | Compile-time | Rejection |
| `E-TYP-1503` | Error    | Operation not supported for operand type.         | Compile-time | Rejection |
| `E-TYP-1505` | Error    | Missing required type annotation at module scope. | Compile-time | Rejection |

---

### 4.2 Subtyping and Coercion

##### Definition

The **subtype relation** is a partial order on types that determines when a value of one type may be used where a value of another type is expected. A type $S$ is a **subtype** of type $T$, written $S <: T$, if a value of type $S$ may be safely used in any context requiring a value of type $T$.

**Coercion** is the implicit conversion that occurs when an expression of a subtype is used in a context expecting a supertype.

**Formal Definition**

The subtype relation $<:$ is a binary relation on types:

$$<:\ \subseteq\ \mathcal{T} \times \mathcal{T}$$

The relation MUST be reflexive ($T <: T$ for all $T$), transitive (if $S <: T$ and $T <: U$, then $S <: U$), and antisymmetric (if $S <: T$ and $T <: S$, then $S \equiv T$).

##### Static Semantics

**Coercion Rule**

When an expression has a subtype of the expected type, implicit coercion occurs:

$$\frac{\Gamma \vdash e : S \quad \Gamma \vdash S <: T}{\Gamma \vdash e : T} \quad \text{(T-Coerce)}$$

**Permission Subtyping**

Permission types form a linear subtype lattice. The complete lattice structure, formal subtyping rules, and the prohibition on implicit upgrades are all defined in §4.5.

**Class Implementation Subtyping**

A concrete type that implements a class is a subtype of that class. See §9.3 for class implementation and the formal rule (Sub-Class).

**Additional Subtyping Rules**

The following subtyping relationships are defined in their respective sections:

- Modal widening ($M@S <: M$, where $M$ is the general modal type): §6.1 (includes storage cost documentation)
- Union member subtyping: §5.5
- Refinement type subtyping: §7.3
- Permission subtyping: §4.5

##### Constraints & Legality

A coercion MUST NOT introduce Unverifiable Behavior (UVB).

An implementation MUST reject any assignment or argument where the source type is not a subtype of the target type.

**Diagnostic Table**

| Code         | Severity | Condition                                          | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------------- | :----------- | :-------- |
| `E-TYP-1510` | Error    | Source type is not a subtype of target type.       | Compile-time | Rejection |
| `E-TYP-1511` | Error    | Implicit permission upgrade attempted.             | Compile-time | Rejection |
| `E-TYP-1512` | Error    | Coercion between incompatible sibling permissions. | Compile-time | Rejection |

---

### 4.3 Variance

##### Definition

**Variance** specifies how subtyping of type arguments relates to subtyping of parameterized types. For a generic type constructor $F$ with parameter $T$, variance determines whether $F[A] <: F[B]$ when $A <: B$, when $B <: A$, both, or neither.

**Formal Definition**

The variance of a type parameter is one of four variances:

| Variance      | Symbol | Condition for $F[A] <: F[B]$ |
| :------------ | :----- | :--------------------------- |
| Covariant     | $+$    | $A <: B$                     |
| Contravariant | $-$    | $B <: A$                     |
| Invariant     | $=$    | $A \equiv B$                 |
| Bivariant     | $\pm$  | Always (parameter unused)    |

##### Static Semantics

**Variance Determination**

The variance of a type parameter is determined by its usage within the type definition:

- **Covariant ($+$):** Parameter appears only in output positions (return types, immutable fields).
- **Contravariant ($-$):** Parameter appears only in input positions (parameter types).
- **Invariant ($=$):** Parameter appears in both input and output positions, or in mutable storage.
- **Bivariant ($\pm$):** Parameter does not appear in the type's structure.

**Generic Subtyping Rule**

For a generic type $\textit{Name}[T_1, \ldots, T_n]$ with parameter variances $V_1, \ldots, V_n$:

$$\frac{\forall i.\ \text{variance\_check}(V_i, A_i, B_i)}{\Gamma \vdash \textit{Name}[A_1, \ldots, A_n] <: \textit{Name}[B_1, \ldots, B_n]} \quad \text{(Sub-Generic)}$$

where $\text{variance\_check}(V, A, B)$ holds iff:
- $V = +$ and $\Gamma \vdash A <: B$
- $V = -$ and $\Gamma \vdash B <: A$
- $V = {=}$ and $\Gamma \vdash A \equiv B$
- $V = \pm$

**Function Type Variance**

Function types exhibit contravariant parameters and covariant return types:

$$\frac{\Gamma \vdash U <: T \quad \Gamma \vdash R_1 <: R_2}{\Gamma \vdash (T) \to R_1 <: (U) \to R_2} \quad \text{(Var-Func)}$$

A function accepting a supertype may substitute for one accepting a subtype. A function returning a subtype may substitute for one returning a supertype.

**Permission Interaction**

The `const` permission enables covariant treatment of otherwise invariant generic types (see §4.5.3 for the `const` permission semantics that guarantee immutability):

$$\frac{\Gamma \vdash A <: B}{\Gamma \vdash \texttt{const}\ C[A] <: \texttt{const}\ C[B]} \quad \text{(Var-Const)}$$

This rule applies because `const` prohibits mutation, eliminating the safety concern that motivates invariance. The rule does not apply to `unique` or `shared` permissions:

$$\frac{A \not\equiv B}{\Gamma \nvdash \texttt{unique}\ C[A] <: \texttt{unique}\ C[B]}$$

$$\frac{A \not\equiv B}{\Gamma \nvdash \texttt{shared}\ C[A] <: \texttt{shared}\ C[B]}$$

##### Constraints & Legality

An implementation MUST reject a subtyping judgment that violates variance rules.

When a type parameter has invariant variance, the implementation MUST require exact type equivalence for that parameter in subtyping checks.

**Diagnostic Table**

| Code         | Severity | Condition                                           | Detection    | Effect    |
| :----------- | :------- | :-------------------------------------------------- | :----------- | :-------- |
| `E-TYP-1520` | Error    | Variance violation in generic type instantiation.   | Compile-time | Rejection |
| `E-TYP-1521` | Error    | Invariant type parameter requires exact type match. | Compile-time | Rejection |

---

### 4.4 Bidirectional Type Inference

##### Definition

**Type inference** is the process by which type information not explicitly annotated in the source program is derived. Cursive employs **bidirectional type inference**, combining two complementary modes of type propagation.

**Formal Definition**

Bidirectional type inference defines two judgment classes:

| Judgment                        | Name      | Meaning                                                  |
| :------------------------------ | :-------- | :------------------------------------------------------- |
| $\Gamma \vdash e \Rightarrow T$ | Synthesis | Type $T$ is derived from the structure of expression $e$ |
| $\Gamma \vdash e \Leftarrow T$  | Checking  | Expression $e$ is validated against expected type $T$    |

In synthesis mode, the type flows outward from the expression. In checking mode, an expected type flows inward from context to guide type derivation for subexpressions.

##### Static Semantics

**Synthesis Rules**

Variable lookup synthesizes the type from the context:

$$\frac{(x : T) \in \Gamma}{\Gamma \vdash x \Rightarrow T} \quad \text{(Synth-Var)}$$

Tuple construction synthesizes component types:

$$\frac{\Gamma \vdash e_1 \Rightarrow T_1 \quad \Gamma \vdash e_2 \Rightarrow T_2}{\Gamma \vdash (e_1, e_2) \Rightarrow (T_1, T_2)} \quad \text{(Synth-Tuple)}$$

Function application synthesizes the return type while checking arguments:

$$\frac{\Gamma \vdash f \Rightarrow (T_1, \ldots, T_n) \to R \quad \forall i.\ \Gamma \vdash a_i \Leftarrow T_i}{\Gamma \vdash f(a_1, \ldots, a_n) \Rightarrow R} \quad \text{(Synth-App)}$$

**Checking Rules**

Synthesis satisfies checking when the synthesized type is a subtype of the expected type:

$$\frac{\Gamma \vdash e \Rightarrow S \quad \Gamma \vdash S <: T}{\Gamma \vdash e \Leftarrow T} \quad \text{(Check-Sub)}$$

Closures check against an expected function type, propagating parameter types inward:

$$\frac{\Gamma, x : T_1 \vdash e \Leftarrow T_2}{\Gamma \vdash |x|\ e \Leftarrow (T_1) \to T_2} \quad \text{(Check-Lambda)}$$

**Annotation Rule**

An explicit type annotation switches from checking to synthesis:

$$\frac{\Gamma \vdash e \Leftarrow T}{\Gamma \vdash (e : T) \Rightarrow T} \quad \text{(Synth-Annot)}$$

**Locality Requirement**

Type inference MUST be local: inference operates within a single procedure body and MUST NOT propagate type information across procedure boundaries.

**Mandatory Annotations**

The following positions MUST have explicit type annotations:

- Procedure parameter types
- Procedure return types
- Public and protected module-scope bindings

**Permitted Omissions**

The following positions MAY omit type annotations when the type is inferable:

- Local bindings within procedure bodies (`let`, `var`)
- Closure parameters when the closure is checked against a known function type

##### Constraints & Legality

An implementation MUST reject a program if inference fails to derive a unique type for any expression.

An implementation MUST NOT infer types for public API boundaries.

**Diagnostic Table**

| Code         | Severity | Condition                                        | Detection    | Effect    |
| :----------- | :------- | :----------------------------------------------- | :----------- | :-------- |
| `E-TYP-1505` | Error    | Missing required type annotation (see §4.1).     | Compile-time | Rejection |
| `E-TYP-1530` | Error    | Type inference failed; unable to determine type. | Compile-time | Rejection |

When type arguments cannot be inferred, explicit type arguments may be supplied using turbofish syntax (`::<>`). See §7.1 for the full specification.

---

### 4.5 Permission Types

---

#### 4.5.1 Permission Definitions

##### Definition

A **permission** is a type qualifier that governs how the data referenced by a binding may be accessed, mutated, and aliased. Permissions are fundamental to the language's memory safety guarantees, providing compile-time enforcement of aliasing rules without runtime overhead. When no permission is explicitly specified, `const` is the default.

**Formal Definition**

Let $\mathcal{P}$ denote the set of permissions:

$$\mathcal{P} = \{\texttt{const}, \texttt{unique}, \texttt{shared}\}$$

A **permission-qualified type** is a pair $(P, T)$ where $P \in \mathcal{P}$ and $T$ is a base type. The notation $P\ T$ denotes this qualification (e.g., `unique Buffer`, `const Config`).

**Permission Equivalence**

Two permission-qualified types are equivalent if and only if both their base types and their permissions are identical. The formal rule (T-Equiv-Permission) is defined in §4.1.

---

#### 4.5.2 The Permission Lattice

##### Definition

The permission lattice orders permissions by the strength of their aliasing guarantees.

##### Static Semantics

The three permissions form a linear lattice ordered by the subtype relation:

$$\texttt{unique} <: \texttt{shared} <: \texttt{const}$$

The formal subtyping rules are defined in §4.5.4.

---

#### 4.5.3 Detailed Permission Semantics

##### Definition

Each permission defines the permitted operations and aliasing constraints on a permission-qualified path (see §11 for path expression definitions).

##### Syntax & Declaration

**Grammar**

```ebnf
permission       ::= "const" | "unique" | "shared"
permission_type  ::= permission? type
```

See §5-6 for `type` productions.

When no permission is specified, `const` is implied (see §4.5.1).

**Shorthand Notation**

Receiver shorthand notation is defined in §8.5.

##### Static Semantics

**`const` — Immutable Access (Default)**

The `const` permission grants read-only access to data and permits unlimited aliasing. Mutation through a `const` path is forbidden; any number of `const` paths to the same data may coexist simultaneously.

**`unique` — Exclusive Mutable Access**

The `unique` permission grants exclusive read-write access to data. The existence of a live `unique` path to an object statically precludes the existence of any other path to that object or its sub-components. A `unique` path may be temporarily downgraded to a weaker permission for a bounded scope; during this period, the original path is **inactive** and the binding becomes unusable until the scope ends. The `unique` permission does **not** imply cleanup responsibility; a procedure parameter of type `unique T` (without `move`) grants exclusive access while the caller retains responsibility for cleanup.

**`shared` — Key-Synchronized Mutable Access**

The `shared` permission grants mutable access to data through implicit key acquisition. Read access requires a Read key; write access requires a Write key. Keys are automatically acquired when accessing the data and released after use.

**Key Properties:**

- **Path-specific:** Keys are acquired at the granularity of the accessed path
- **Implicitly acquired:** Accessing a `shared` path acquires the necessary key automatically
- **Minimal scope:** Keys are held for the minimal duration preserving correctness
- **Reentrant:** If a covering key is already held, nested access succeeds without additional acquisition

The `shared` permission does **not** imply cleanup responsibility; a procedure parameter
of type `shared T` (without `move`) grants synchronized access while the caller retains
responsibility for cleanup.

The following table specifies which operations are permitted through a `shared` path when key semantics are available (see §13.1 for key system rules):

| Operation                   | Permitted | Key Mode  | Notes                                |
| :-------------------------- | :-------- | :-------- | :----------------------------------- |
| Field read                  | Yes       | Read key  |                                      |
| Field mutation              | Yes*      | Write key | *Via key system only; see note below |
| Method call (`~` receiver)  | Yes       | Read key  |                                      |
| Method call (`~%` receiver) | Yes       | Write key |                                      |
| Method call (`~!` receiver) | No        | N/A       | Requires `unique` permission         |

**Note on Field Mutation:** Direct field assignment through a `shared` path (e.g., `obj.field = value`) requires key acquisition per §13.1. For types that do not participate in the key system or outside valid key contexts, direct field mutation is rejected with `E-TYP-1604`. See §4.5.4 (shared Access to Types Without Synchronized Methods) for read-only shared access patterns.

Key modes (Read, Write) are defined in §14.1.2 (Key Modes).

**Exclusion Principle (Normative)**

The rules governing which permission-qualified paths may coexist for the same object are defined in this section. The **coexistence matrix** specifies which permissions may exist simultaneously:

| Active Permission | May Add `unique` | May Add `shared` | May Add `const` |
| :---------------- | :--------------- | :--------------- | :-------------- |
| `unique`          | No               | No               | No              |
| `shared`          | No               | Yes              | Yes             |
| `const`           | No               | Yes              | Yes             |

**Note:** While the matrix shows `const`+`shared` as theoretically compatible, this combination cannot occur in practice: `const` cannot upgrade to `shared` (permissions only downgrade), and when a `unique` binding downgrades to `shared`, the original binding becomes inactive.

#### 4.5.4 Permission Subtyping Rules

##### Definition

This section formalizes the subtype relation between permission-qualified types.

##### Static Semantics

**Subtyping Rules**

The following rules formalize the permission subtyping lattice ($\texttt{unique} <: \texttt{shared} <: \texttt{const}$) defined in §4.5.2:

$$\frac{}{\Gamma \vdash \texttt{unique}\ T <: \texttt{shared}\ T} \quad \text{(Sub-Perm-US)}$$

$$\frac{}{\Gamma \vdash \texttt{unique}\ T <: \texttt{const}\ T} \quad \text{(Sub-Perm-UC)}$$

$$\frac{}{\Gamma \vdash \texttt{shared}\ T <: \texttt{const}\ T} \quad \text{(Sub-Perm-SC)}$$

**No Implicit Upgrade**

Coercion from a weaker permission to a stronger permission is **not derivable**:

$$\Gamma \nvdash \texttt{const}\ T <: \texttt{unique}\ T$$

$$\Gamma \nvdash \texttt{const}\ T <: \texttt{shared}\ T$$

$$\Gamma \nvdash \texttt{shared}\ T <: \texttt{unique}\ T$$

Implicit upgrade attempts are rejected per §4.2 (`E-TYP-1511`).

**shared-Safe Method Rule**

A method $m$ on type $T$ is callable through a `shared` path if and only if the method's receiver accepts `shared` permission or a weaker permission (`const`):

$$\frac{\Gamma \vdash m : T.\text{method} \quad m.\text{receiver} \in \{\texttt{shared}, \texttt{const}\}}{\Gamma \vdash (\texttt{shared}\ T).m\ \text{callable}} \quad \text{(shared-Method)}$$

**shared Access to Types Without Synchronized Methods**

A type that provides no methods accepting `shared Self` (or `~%`) receiver is **read-only through `shared` paths**. Such types may still be useful in shared contexts when:

1. Only field reads are required, OR
2. The type implements `const`-compatible methods sufficient for the use case.

```cursive
let config: shared Config = load_config()
let timeout = config.timeout_ms    // OK: field read
// config.timeout_ms = 5000        // ERROR E-TYP-1604: field mutation forbidden
```

---

#### 4.5.5 Method Receiver Permissions

##### Definition

Method receiver permissions determine which paths may be used to invoke a method. This section summarizes the key constraint; the complete treatment is in §9.2 (Method Definitions and Receivers).

##### Static Semantics

A method with receiver permission $P_{\text{method}}$ is callable through a path with permission $P_{\text{caller}}$ if and only if $P_{\text{caller}} <: P_{\text{method}}$ in the permission subtyping lattice (§4.5.4).

**Summary:** `unique` callers may invoke any method. `const` callers may only invoke `const` methods. `shared` callers may invoke `const` or `shared` methods.

> **Cross-Reference:** See §9.2 for the complete receiver compatibility matrix, formal typing rules (Receiver-Compat), key acquisition semantics for `shared` receivers, and diagnostic codes.

---

**Downgrading Scope and Inactive Bindings**

A binding is **inactive** when its permission has been temporarily downgraded for a bounded scope. The binding state transitions are governed by the following formal rules:

**Binding State Machine**

A binding $b$ with `unique` permission exists in one of two states:

| State    | Definition                                        | Operations Permitted         |
| :------- | :------------------------------------------------ | :--------------------------- |
| Active   | No downgraded references to $b$ are live          | Read, write, move, downgrade |
| Inactive | One or more downgraded references to $b$ are live | None                         |

**State Transitions**

$$\text{Active} \xrightarrow{\text{downgrade begins}} \text{Inactive} \xrightarrow{\text{downgrade scope ends}} \text{Active}$$

The transitions are defined as follows:

**(Inactive-Enter)** When a `unique` binding $b$ is passed to a context expecting a weaker permission ($\texttt{const}$ or $\texttt{shared}$), $b$ transitions from Active to Inactive:

$$\frac{b : \texttt{unique}\ T \quad b\ \text{is Active} \quad \text{downgrade to}\ P\ \text{where}\ P \in \{\texttt{const}, \texttt{shared}\}}{b\ \text{becomes Inactive}}$$

**(Inactive-Exit)** When the scope containing the downgraded reference ends, $b$ transitions from Inactive to Active:

$$\frac{b\ \text{is Inactive} \quad \text{all downgraded references to}\ b\ \text{go out of scope}}{b\ \text{becomes Active with}\ \texttt{unique}\ \text{permission restored}}$$

**Invariants:**

1. During the inactive period, the original `unique` binding MUST NOT be read, written, or moved.
2. Any attempt to use an inactive binding MUST be rejected with diagnostic `E-TYP-1602`.
3. The transition back to Active occurs deterministically when the downgrade scope ends.

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                                                             | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------------------------------------------------ | :----------- | :-------- |
| `E-TYP-1601` | Error    | Attempt to mutate data via a `const` path.                                            | Compile-time | Rejection |
| `E-TYP-1602` | Error    | Violation of `unique` exclusion (aliasing detected or inactive use).                  | Compile-time | Rejection |
| `E-TYP-1604` | Error    | Direct field mutation through `shared` path.                                          | Compile-time | Rejection |
| `E-TYP-1605` | Error    | Method receiver permission incompatible with caller's permission (see §4.5.5 matrix). | Compile-time | Rejection |

**Canonical Diagnostic Codes**

Per Appendix B.4, type system diagnostics (`E-TYP-`) are canonical; memory model diagnostics (`E-MEM-`) are secondary.

---

## Clause 5: Data Types

### 5.1 Primitive Types

##### Definition

**Primitive types** are the built-in scalar types that form the foundation of the Cursive type system. These types represent indivisible units of data with fixed size and well-defined representation. All composite types are ultimately constructed from primitive types.

**Formal Definition**

Let $\mathcal{T}_{\text{prim}}$ denote the set of primitive types:

$$\mathcal{T}_{\text{prim}} = \mathcal{T}_{\text{int}} \cup \mathcal{T}_{\text{float}} \cup \{\texttt{bool}, \texttt{char}, \texttt{()}, \texttt{!}\}$$

where:

$$\mathcal{T}_{\text{int}} = \{\texttt{i8}, \texttt{i16}, \texttt{i32}, \texttt{i64}, \texttt{i128}, \texttt{u8}, \texttt{u16}, \texttt{u32}, \texttt{u64}, \texttt{u128}, \texttt{isize}, \texttt{usize}\}$$

$$\mathcal{T}_{\text{float}} = \{\texttt{f16}, \texttt{f32}, \texttt{f64}\}$$

##### Syntax & Declaration

**Grammar**

```ebnf
primitive_type     ::= integer_type | float_type | "bool" | "char" | unit_type | never_type

integer_type       ::= signed_int | unsigned_int | pointer_int 
signed_int         ::= "i8" | "i16" | "i32" | "i64" | "i128"
unsigned_int       ::= "u8" | "u16" | "u32" | "u64" | "u128"
pointer_int        ::= "isize" | "usize"

float_type         ::= "f16" | "f32" | "f64" 

unit_type          ::= "(" ")"
never_type         ::= "!"
```

**Literal Syntax**

Literal syntax is defined in §2.8 (Literals). The following specifies the default type inference for each literal category:

| Literal Category | Default Type | Type Suffix Available   |
| :--------------- | :----------- | :---------------------- |
| Integer          | `i32`        | Any integer type suffix |
| Floating-point   | `f64`        | `f32`, `f16`            |
| Boolean          | `bool`       | —                       |
| Character        | `char`       | —                       |
| Unit             | `()`         | —                       |

##### Static Semantics

**Type Equivalence**

Primitive types have **strict name equivalence**: two primitive type expressions are equivalent if and only if, after alias resolution, they denote the same type:

$$\frac{T \in \mathcal{T}_{\text{prim}} \quad U \in \mathcal{T}_{\text{prim}} \quad T = U}{\Gamma \vdash T \equiv U} \quad \text{(T-Equiv-Prim)}$$

**Never Type as Bottom**

The never type `!` is the **bottom type** of the type system. It is a subtype of all other types:

$$\frac{T \in \mathcal{T}}{\Gamma \vdash \texttt{!} <: T} \quad \text{(Sub-Never)}$$

This rule permits expressions of type `!` (such as `panic()` or infinite loops) to appear in any context regardless of the expected type.

**Literal Typing Rules**

**(T-Int-Literal)**
$$\frac{v \in \text{IntLiteral} \quad T \in \mathcal{T}_{\text{int}} \quad \text{InRange}(v, T)}{\Gamma \vdash v : T}$$

**(T-Float-Literal)**
$$\frac{v \in \text{FloatLiteral} \quad T \in \mathcal{T}_{\text{float}}}{\Gamma \vdash v : T}$$

**(T-Bool-Literal)**
$$\frac{v \in \{\texttt{true}, \texttt{false}\}}{\Gamma \vdash v : \texttt{bool}}$$

**(T-Char-Literal)**
$$\frac{v \in \text{CharLiteral} \quad \text{IsUSV}(v)}{\Gamma \vdash v : \texttt{char}}$$

**(T-Unit-Literal)**
$$\frac{}{\Gamma \vdash () : ()}$$

where $\text{InRange}(v, T)$ holds when the numeric value $v$ is representable in type $T$, and $\text{IsUSV}(v)$ holds when $v$ is a valid Unicode Scalar Value.

**Implicit Numeric Conversions**

There are **no implicit conversions** between distinct primitive types. A conforming implementation MUST reject any expression where a value of one primitive type is used in a context requiring a different primitive type without an explicit cast.

$$\frac{T \neq U \quad T \in \mathcal{T}_{\text{prim}} \quad U \in \mathcal{T}_{\text{prim}} \quad U \neq \texttt{!}}{\Gamma \nvdash T <: U} \quad \text{(No-Implicit-Prim-Coerce)}$$

##### Dynamic Semantics

**Integer Overflow**

Arithmetic operations on integer types (`+`, `-`, `*`) may overflow when the mathematical result is not representable in the result type.

A conforming implementation MUST provide a **checked mode** in which signed and unsigned integer overflow causes the executing thread to **panic**. This mode MUST be the default for debug builds.

The behavior of integer overflow in release (optimized) builds is **implementation-defined (IDB)**. Conforming implementations MAY choose one of the following behaviors for release mode:

1. **Wrap** — The result is the mathematical result modulo $2^N$ where $N$ is the bit width.
2. **Panic** — Same behavior as checked mode.
3. **Trap** — Invoke a platform-specific trap or signal.

Release-mode overflow behavior is IDB. Implementations MAY choose different behaviors for signed and unsigned overflow, or MAY apply the same behavior to both. Within each category (signed or unsigned), the chosen behavior MUST apply uniformly to all operations of that category; an implementation MUST NOT vary overflow behavior on a per-operation or per-type basis within a single compilation.


**Integer Division and Remainder**

Division (`/`) and remainder (`%`) operations on integer types MUST trigger a **panic** when the divisor is zero. Implementations MUST NOT optimize away division-by-zero checks.

**Floating-Point Semantics**

Floating-point arithmetic MUST conform to **IEEE 754-2008** for the corresponding precision (`f16`/binary16, `f32`/binary32, `f64`/binary64), including NaN, infinities, signed zero, and division-by-zero semantics.

**Unit Type Semantics**

The unit type `()` has exactly one value, also written `()`. The unit value is the implicit result of:

- Procedures without an explicit return type
- Block expressions whose final statement is not an expression
- The `result` statement with no expression

**Never Type Semantics**

The never type `!` is **uninhabited**: no value of type `!` can be created. Expressions of type `!` represent computations that do not return normally. Such expressions include:

- Invocations of diverging procedures (e.g., `panic()`, `sys::exit()`)
- Infinite loops with no `break` expression
- The `unreachable!()` intrinsic

Because `!` is uninhabited, code following an expression of type `!` is unreachable.

##### Memory & Layout

**Primitive Type Layout**

Primitive types have defined layouts. Implementations MUST use the following sizes and alignments:

| Type   | Size (bytes) | Alignment (bytes) | Representation                                               |
| :----- | :----------- | :---------------- | :----------------------------------------------------------- |
| `i8`   | 1            | 1                 | Two's complement signed integer                              |
| `u8`   | 1            | 1                 | Unsigned integer                                             |
| `bool` | 1            | 1                 | `0x00` = `false`, `0x01` = `true`; other values are invalid  |
| `i16`  | 2            | 2                 | Two's complement signed integer                              |
| `u16`  | 2            | 2                 | Unsigned integer                                             |
| `f16`  | 2            | 2                 | IEEE 754-2008 binary16                                       |
| `i32`  | 4            | 4                 | Two's complement signed integer                              |
| `u32`  | 4            | 4                 | Unsigned integer                                             |
| `f32`  | 4            | 4                 | IEEE 754-2008 binary32                                       |
| `char` | 4            | 4                 | Unicode scalar value (U+0000–U+10FFFF, excluding surrogates) |
| `i64`  | 8            | 8                 | Two's complement signed integer                              |
| `u64`  | 8            | 8                 | Unsigned integer                                             |
| `f64`  | 8            | 8                 | IEEE 754-2008 binary64                                       |
| `i128` | 16           | IDB               | Two's complement signed integer                              |
| `u128` | 16           | IDB               | Unsigned integer                                             |

The alignment of `i128` and `u128` is implementation-defined (IDB). Conforming implementations MUST choose an alignment of 8 or 16 bytes.

**Platform-Dependent Types**

The pointer-sized integer types have sizes and alignments determined by the target platform:

| Type    | Size                   | Alignment              | Representation                  |
| :------ | :--------------------- | :--------------------- | :------------------------------ |
| `usize` | Platform pointer width | Platform pointer width | Unsigned integer                |
| `isize` | Platform pointer width | Platform pointer width | Two's complement signed integer |

On 32-bit platforms, `usize` and `isize` are 4 bytes with 4-byte alignment. On 64-bit platforms, they are 8 bytes with 8-byte alignment. The exact pointer width is implementation-defined (IDB).

**Unit and Never Types**

| Type | Size (bytes) | Alignment (bytes) |
| :--- | :----------- | :---------------- |
| `()` | 0            | 1                 |
| `!`  | 0            | 1                 |

The unit type `()` and never type `!` are zero-sized types (ZSTs). Values of these types do not occupy storage.

**Representation Invariants**

The following invariants define valid bit patterns for specific primitive types. Constructing a value that violates these invariants is **Unverifiable Behavior (UVB)** and MUST occur only within an `unsafe` block.

*Boolean:* A valid `bool` value MUST be represented by exactly one of two bit patterns: `0x00` (false) or `0x01` (true). Any other bit pattern is invalid.

*Character:* A valid `char` value MUST contain a **Unicode Scalar Value (USV)**, defined as any Unicode code point in the ranges `U+0000`–`U+D7FF` or `U+E000`–`U+10FFFF`. Surrogate code points (`U+D800`–`U+DFFF`) and values above `U+10FFFF` are invalid.

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                              | Detection    | Effect    |
| :----------- | :------- | :----------------------------------------------------- | :----------- | :-------- |
| `W-TYP-1701` | Warning  | `f16` arithmetic emulated on target platform.          | Compile-time | Warning   |
| `E-TYP-1710` | Error    | Integer literal out of range for target type.          | Compile-time | Rejection |
| `E-TYP-1711` | Error    | Character literal is not a valid Unicode Scalar Value. | Compile-time | Rejection |
| `E-TYP-1712` | Error    | Implicit conversion between distinct primitive types.  | Compile-time | Rejection |
| `E-TYP-1713` | Error    | Shadowing of primitive type alias identifier.          | Compile-time | Rejection |
| `P-TYP-1720` | Panic    | Integer overflow in checked mode.                      | Runtime      | Panic     |
| `P-TYP-1721` | Panic    | Integer division or remainder by zero.                 | Runtime      | Panic     |

---

### 5.2 Composite Types

##### Definition

**Composite types** are types constructed by aggregating other types into structured collections. The structural composite types are: tuples, arrays, slices, and ranges. Two composite type expressions are equivalent if their structure matches, regardless of where or how they are declared (**structural equivalence**).

**Formal Definition**

Let $\mathcal{T}_{\text{composite}}$ denote the set of structural composite types defined in this section:

$$\mathcal{T}_{\text{composite}} = \mathcal{T}_{\text{tuple}} \cup \mathcal{T}_{\text{array}} \cup \mathcal{T}_{\text{slice}} \cup \mathcal{T}_{\text{range}}$$

#### 5.2.1 Tuples

##### Definition

A **tuple** is an ordered, fixed-length, heterogeneous sequence of values. Tuples are anonymous product types with **structural equivalence**.

**Formal Definition**

A tuple type is an ordered sequence of component types:

$$\mathcal{T}_{\text{tuple}} = \{(T_1, T_2, \ldots, T_n) : n \geq 0 \land \forall i \in 1..n,\ T_i \in \mathcal{T}\}$$

The **arity** of a tuple type $(T_1, \ldots, T_n)$ is $n$. The **unit type** `()` is the tuple of arity zero.

##### Syntax & Declaration

**Grammar**

```ebnf
tuple_type       ::= "(" tuple_elements? ")"
tuple_elements   ::= type ";" | type ("," type)+

tuple_literal    ::= "(" tuple_expr_elements? ")"
tuple_expr_elements ::= expression ";" | expression ("," expression)+

tuple_access     ::= expression "." decimal_literal

expression_list  ::= expression ("," expression)*
```

> **Note:** `expression_list` is used by array literals, enum variant construction, and function calls where the parentheses ambiguity does not apply.

**Type Expression**

A tuple type is written as a parenthesized, comma-separated list of component types: `(T₁, T₂, ..., Tₙ)`.

**Literal Syntax**

A tuple value is constructed with a corresponding literal: `(e₁, e₂, ..., eₙ)`. Single-element tuples require a trailing semicolon: `(T;)` (1-tuple) vs. `(T)` (parenthesized expression).

**Element Access**

Individual elements of a tuple `t` are accessed by a constant, zero-based integer index using dot notation: `t.0`, `t.1`, etc. The index MUST be a decimal integer literal; it MUST NOT be a variable or computed expression.

##### Static Semantics

**Typing Rules**

**(T-Tuple-Type)**
A tuple type is well-formed when all component types are well-formed:
$$\frac{\forall i \in 1..n,\ \Gamma \vdash T_i\ \text{wf}}{\Gamma \vdash (T_1, \ldots, T_n)\ \text{wf}}$$

**(T-Tuple-Literal)**
A tuple literal has a tuple type when each component expression has the corresponding component type:
$$\frac{\forall i \in 1..n,\ \Gamma \vdash e_i : T_i}{\Gamma \vdash (e_1, \ldots, e_n) : (T_1, \ldots, T_n)}$$

**(T-Tuple-Index)**
Accessing element $i$ of a tuple yields the $i$-th component type (zero-indexed):
$$\frac{\Gamma \vdash e : (T_0, T_1, \ldots, T_{n-1}) \quad 0 \leq i < n}{\Gamma \vdash e.i : T_i} \quad \text{(T-Tuple-Index)}$$

**Type Equivalence**

Tuple type equivalence is structural, per rule (T-Equiv-Tuple) in §4.1.

**Subtyping**

Tuple subtyping is covariant in all component positions:

$$\frac{n = m \quad \forall i \in 1..n,\ \Gamma \vdash T_i <: U_i}{\Gamma \vdash (T_1, \ldots, T_n) <: (U_1, \ldots, U_m)} \quad \text{(Sub-Tuple)}$$

Permission propagates per §4.5.

##### Memory & Layout

**Representation**

A tuple MUST be laid out as a contiguous sequence of its component values. Components MUST be stored in declaration order (left to right).

**Size and Alignment**

$$\text{sizeof}((T_1, \ldots, T_n)) = \sum_{i=1}^{n} \text{sizeof}(T_i) + \text{padding}$$

$$\text{alignof}((T_1, \ldots, T_n)) = \max_{i \in 1..n}(\text{alignof}(T_i))$$

For the unit type: $\text{sizeof}(()) = 0$ and $\text{alignof}(()) = 1$.

Padding follows Universal Layout Principles (Clause 5 introduction).

##### Constraints & Legality

**Constraints**

Tuple indices MUST be compile-time constant integers within bounds.

**Diagnostic Table**

| Code         | Severity | Condition                                                   | Detection    | Effect    |
| :----------- | :------- | :---------------------------------------------------------- | :----------- | :-------- |
| `E-TYP-1801` | Error    | Tuple index out of bounds.                                  | Compile-time | Rejection |
| `E-TYP-1802` | Error    | Tuple index is not a compile-time constant integer literal. | Compile-time | Rejection |
| `E-TYP-1803` | Error    | Tuple arity mismatch in assignment or pattern.              | Compile-time | Rejection |

#### 5.2.2 Arrays

##### Definition

An **array** is a contiguous, fixed-length, homogeneous sequence of elements. The length of an array is part of its type and MUST be known at compile time.

**Formal Definition**

An array type is parameterized by an element type and a compile-time constant length:

$$\mathcal{T}_{\text{array}} = \{[T; N] : T \in \mathcal{T} \land N \in \mathbb{N}\}$$

where $N$ is a non-negative integer constant expression of type `usize`.

##### Syntax & Declaration

**Grammar**

```ebnf
array_type       ::= "[" type ";" const_expression "]"

array_literal    ::= "[" expression_list "]"
                   | "[" expression ";" const_expression "]"

array_access     ::= expression "[" expression "]"
```

See §11 for `const_expression` definition.

**Type Expression**

An array type is written as `[T; N]`, where `T` is the element type and `N` is a compile-time constant expression of type `usize` specifying the array length.

**Literal Syntax**

Array values are constructed using one of two literal forms:

1. **List form:** `[e₁, e₂, ..., eₙ]` — Creates an array of length $n$ with each element initialized to the corresponding expression.

2. **Repeat form:** `[e; N]` — Creates an array of length $N$ with all elements initialized to copies of `e`. The expression `e` MUST have a type that implements `Copy` (see Appendix G), or MUST be a constant expression (see §11).

**Element Access**

Elements of an array `a` are accessed via indexing: `a[i]`, where `i` is an expression of type `usize`.

##### Static Semantics

**Typing Rules**

**(T-Array-Type)**
An array type is well-formed when the element type is well-formed and the length is a valid constant:
$$\frac{\Gamma \vdash T\ \text{wf} \quad N : \texttt{usize} \quad N \geq 0}{\Gamma \vdash [T; N]\ \text{wf}}$$

**(T-Array-Literal-List)**
A list-form array literal has an array type when all element expressions have the element type:
$$\frac{\forall i \in 1..n,\ \Gamma \vdash e_i : T}{\Gamma \vdash [e_1, \ldots, e_n] : [T; n]}$$

**(T-Array-Literal-Repeat)**
A repeat-form array literal has an array type when the initializer has the element type:
$$\frac{\Gamma \vdash e : T \quad N : \texttt{usize} \quad T <: \texttt{Copy} \lor e \in \text{ConstExpr}}{\Gamma \vdash [e; N] : [T; N]}$$

**(T-Array-Index)**
Indexing an array yields the element type:
$$\frac{\Gamma \vdash a : [T; N] \quad \Gamma \vdash i : \texttt{usize}}{\Gamma \vdash a[i] : T}$$

**Type Equivalence**

Array types have structural equivalence. Two array types are equivalent if and only if their element types are equivalent and their lengths are equal:

$$\frac{\Gamma \vdash T \equiv U \quad N = M}{\Gamma \vdash [T; N] \equiv [U; M]} \quad \text{(T-Equiv-Array)}$$

**Subtyping**

Array subtyping is covariant in the element type and invariant in length:

$$\frac{\Gamma \vdash T <: U \quad N = M}{\Gamma \vdash [T; N] <: [U; M]} \quad \text{(Sub-Array)}$$

Permission propagates per §4.5.

##### Dynamic Semantics

**Bounds Checking**

**Bounds Checking**: All array indexing is bounds-checked at runtime per §12.4.2.

**Initialization**

For list-form literals, elements are initialized in left-to-right order. For repeat-form literals, the initialization order is unspecified.

##### Memory & Layout

**Representation**

An array MUST be laid out as a contiguous sequence of elements with no inter-element padding:

$$\text{layout}([T; N]) = T\ ||\ T\ ||\ \cdots\ ||\ T \quad (N \text{ times})$$

where $||$ denotes contiguous concatenation.

**Size and Alignment**

$$\text{sizeof}([T; N]) = N \times \text{sizeof}(T)$$

$$\text{alignof}([T; N]) = \text{alignof}(T)$$

For zero-length arrays: $\text{sizeof}([T; 0]) = 0$ and $\text{alignof}([T; 0]) = \text{alignof}(T)$.

**ABI Guarantee**

The array layout formula above is **defined behavior**. Implementations MUST NOT insert padding between array elements.

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                                     | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------------------------ | :----------- | :-------- |
| `E-TYP-1810` | Error    | Array length is not a compile-time constant.                  | Compile-time | Rejection |
| `E-TYP-1812` | Error    | Array index expression has non-`usize` type.                  | Compile-time | Rejection |
| `E-TYP-1813` | Error    | Array repeat literal requires `Copy` or constant initializer. | Compile-time | Rejection |
| `E-TYP-1814` | Error    | Array length mismatch in assignment or pattern.               | Compile-time | Rejection |

#### 5.2.3 Slices

##### Definition

A **slice** is a dynamically-sized view into a contiguous sequence of elements. A slice does not own its data; it borrows from an array or other contiguous storage.

**Formal Definition**

A slice type is parameterized by an element type:

$$\mathcal{T}_{\text{slice}} = \{[T] : T \in \mathcal{T}\}$$

A slice value is a **dense pointer** consisting of a data pointer and a length.

##### Syntax & Declaration

**Grammar**

```ebnf
slice_type       ::= "[" type "]"

slice_access     ::= expression "[" expression "]"
slice_range      ::= expression "[" range_expression "]"
```

**Type Expression**

A slice type is written as `[T]`, where `T` is the element type. Note the absence of a length parameter, distinguishing slices from arrays.

**Element Access**

Elements of a slice `s` are accessed via indexing: `s[i]`, where `i` is an expression of type `usize`.

##### Static Semantics

**Typing Rules**

**(T-Slice-Type)**
A slice type is well-formed when the element type is well-formed:
$$\frac{\Gamma \vdash T\ \text{wf}}{\Gamma \vdash [T]\ \text{wf}}$$

**(T-Slice-From-Array)**
A range expression on an array produces a slice:
$$\frac{\Gamma \vdash a : P\ [T; N] \quad \Gamma \vdash r : \text{Range}}{\Gamma \vdash a[r] : P\ [T]}$$

where $P$ is the permission of the array and Range is any range type from §5.2.4.

**(T-Slice-Index)**
Indexing a slice yields the element type:
$$\frac{\Gamma \vdash s : [T] \quad \Gamma \vdash i : \texttt{usize}}{\Gamma \vdash s[i] : T}$$

**Type Equivalence**

Slice types have structural equivalence based on their element type:

$$\frac{\Gamma \vdash T \equiv U}{\Gamma \vdash [T] \equiv [U]} \quad \text{(T-Equiv-Slice)}$$

**Subtyping**

Slice subtyping is covariant in the element type:

$$\frac{\Gamma \vdash T <: U}{\Gamma \vdash [T] <: [U]} \quad \text{(Sub-Slice)}$$

**Coercion from Array**

An array type coerces to a slice type with the same element type:

$$\frac{\Gamma \vdash a : P\ [T; N]}{\Gamma \vdash a : P\ [T]} \quad \text{(Coerce-Array-Slice)}$$

This coercion is implicit and creates a slice viewing the entire array.

Permission propagates per §4.5.

##### Dynamic Semantics

**Bounds Checking**: All slice indexing is bounds-checked at runtime per §12.4.2.

**Slice Range Bounds**

Creating a sub-slice with a range MUST verify that the range bounds are within the slice. For a slice `s` of length $L$ and range `start..end`:
- `start` MUST satisfy $0 \leq \text{start} \leq L$
- `end` MUST satisfy $\text{start} \leq \text{end} \leq L$

Violation of either condition MUST cause a panic with diagnostic `P-TYP-1822`.

**Length Accessor**

The `len()` method on slices returns the number of elements:
$$\text{len}(s) : \texttt{usize}$$

##### Memory & Layout

**Representation**

A slice MUST be represented as a **dense pointer** containing two machine words:

| Field | Type    | Description                     |
| :---- | :------ | :------------------------------ |
| `ptr` | `*T`    | Pointer to the first element    |
| `len` | `usize` | Number of elements in the slice |

**Size and Alignment**

$$\text{sizeof}([T]) = 2 \times \text{sizeof}(\texttt{usize})$$

$$\text{alignof}([T]) = \text{alignof}(\texttt{usize})$$

On a 64-bit platform, a slice occupies 16 bytes (two 8-byte words).

**Ownership**

A slice does not own its data. The underlying storage MUST remain valid for the lifetime of the slice. The slice's permission determines whether it may read or mutate the underlying elements.

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                    | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------- | :----------- | :-------- |
| `E-TYP-1820` | Error    | Slice index expression has non-`usize` type. | Compile-time | Rejection |
| `P-TYP-1822` | Panic    | Slice range out of bounds.                   | Runtime      | Panic     |
| `E-TYP-1823` | Error    | Slice outlives borrowed storage.             | Compile-time | Rejection |

#### 5.2.4 Range Types

##### Definition

**Range types** are intrinsic types with record-like syntax produced by range expressions. Ranges represent bounded or unbounded intervals. Although range types have structural equivalence semantics and expose public fields like records, they are built-in rather than user-definable. Implementations MUST provide these types with the specified field structure and semantics.

**Formal Definition**

The set of range types comprises six distinct intrinsic types:

$$\mathcal{T}_{\text{range}} = \{\texttt{Range}\langle T \rangle, \texttt{RangeInclusive}\langle T \rangle, \texttt{RangeFrom}\langle T \rangle, \texttt{RangeTo}\langle T \rangle, \texttt{RangeToInclusive}\langle T \rangle, \texttt{RangeFull}\}$$

##### Syntax & Declaration

**Grammar**

```ebnf
range_expression ::= exclusive_range | inclusive_range | from_range
                   | to_range | to_inclusive_range | full_range

exclusive_range     ::= expression ".." expression
inclusive_range     ::= expression "..=" expression
from_range          ::= expression ".."
to_range            ::= ".." expression
to_inclusive_range  ::= "..=" expression
full_range          ::= ".."
```

**Range Type Definitions**

Implementations MUST provide the following generic record definitions with **public fields**:

| Expression    | Type                  | Fields                             |
| :------------ | :-------------------- | :--------------------------------- |
| `start..end`  | `Range<T>`            | `public start: T`, `public end: T` |
| `start..=end` | `RangeInclusive<T>`   | `public start: T`, `public end: T` |
| `start..`     | `RangeFrom<T>`        | `public start: T`                  |
| `..end`       | `RangeTo<T>`          | `public end: T`                    |
| `..=end`      | `RangeToInclusive<T>` | `public end: T`                    |
| `..`          | `RangeFull`           | (none)                             |

##### Static Semantics

**Typing Rules**

**(T-Range-Exclusive)**
$$\frac{\Gamma \vdash e_1 : T \quad \Gamma \vdash e_2 : T}{\Gamma \vdash e_1\text{..}e_2 : \texttt{Range}\langle T \rangle}$$

**(T-Range-Inclusive)**
$$\frac{\Gamma \vdash e_1 : T \quad \Gamma \vdash e_2 : T}{\Gamma \vdash e_1\text{..=}e_2 : \texttt{RangeInclusive}\langle T \rangle}$$

**(T-Range-From)**
$$\frac{\Gamma \vdash e : T}{\Gamma \vdash e\text{..} : \texttt{RangeFrom}\langle T \rangle}$$

**(T-Range-To)**
$$\frac{\Gamma \vdash e : T}{\Gamma \vdash \text{..}e : \texttt{RangeTo}\langle T \rangle}$$

**(T-Range-To-Inclusive)**
$$\frac{\Gamma \vdash e : T}{\Gamma \vdash \text{..=}e : \texttt{RangeToInclusive}\langle T \rangle}$$

**(T-Range-Full)**
$$\frac{}{\Gamma \vdash \text{..} : \texttt{RangeFull}}$$

**Type Equivalence**

Range types have structural equivalence based on their element type:

$$\frac{\Gamma \vdash T \equiv U}{\Gamma \vdash \texttt{Range}\langle T \rangle \equiv \texttt{Range}\langle U \rangle} \quad \text{(T-Equiv-Range)}$$

Analogous rules apply to all other range type constructors.

**Copy Semantics**

A range type implements `Copy` if and only if its element type `T` implements `Copy`:

$$\frac{T <: \texttt{Copy}}{\texttt{Range}\langle T \rangle <: \texttt{Copy}} \quad \text{(Copy-Range)}$$

This rule applies to all generic range types. The type `RangeFull` unconditionally implements `Copy`.

**Iteration**

Range types `Range<T>` and `RangeInclusive<T>` implement the `Iterator` class when `T` implements `Step` (see Appendix G). This enables their use in `for` loops:

```cursive
for i in 0..10 { /* i: i32, values 0 through 9 */ }
for i in 0..=10 { /* i: i32, values 0 through 10 */ }
```

> **Note:** Additional methods for range types are defined in the standard library.

##### Dynamic Semantics

**Inclusive vs. Exclusive Semantics**

The exclusive range `start..end` represents values $v$ where $\text{start} \leq v < \text{end}$.

The inclusive range `start..=end` represents values $v$ where $\text{start} \leq v \leq \text{end}$.

**Empty Ranges**

A range `start..end` where $\text{start} \geq \text{end}$ is **empty** and produces no elements when iterated.

A range `start..=end` where $\text{start} > \text{end}$ is **empty**.

**Unbounded Ranges**

The types `RangeFrom`, `RangeTo`, `RangeToInclusive`, and `RangeFull` represent unbounded ranges. These types are valid for slicing operations but MUST NOT be used directly in `for` loops without an explicit bound, as iteration would be unbounded.

##### Memory & Layout

**Representation**

Range types MUST be laid out as their equivalent record definitions. Each generic range type containing element type `T` has:

| Type                  | Size                                         | Alignment           |
| :-------------------- | :------------------------------------------- | :------------------ |
| `Range<T>`            | $2 \times \text{sizeof}(T) + \text{padding}$ | $\text{alignof}(T)$ |
| `RangeInclusive<T>`   | $2 \times \text{sizeof}(T) + \text{padding}$ | $\text{alignof}(T)$ |
| `RangeFrom<T>`        | $\text{sizeof}(T)$                           | $\text{alignof}(T)$ |
| `RangeTo<T>`          | $\text{sizeof}(T)$                           | $\text{alignof}(T)$ |
| `RangeToInclusive<T>` | $\text{sizeof}(T)$                           | $\text{alignof}(T)$ |
| `RangeFull`           | $0$                                          | $1$                 |

Padding within range types follows the same implementation-defined rules as records (§5.3).

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                                   | Detection    | Effect    |
| :----------- | :------- | :---------------------------------------------------------- | :----------- | :-------- |
| `E-TYP-1830` | Error    | Range bound types do not match.                             | Compile-time | Rejection |
| `E-TYP-1831` | Error    | Unbounded range used in context requiring finite iteration. | Compile-time | Rejection |

---

### 5.3 Records

##### Definition

A **record** is a nominal product type with named fields. A record aggregates zero or more typed values into a single composite value, where each constituent value is identified by a field name rather than by position.

**Formal Definition**

Let $\mathcal{R}$ denote the set of all record types. A record type $R$ is defined by:

$$R = (\text{Name}, \text{Params}, \text{Fields}, \text{Forms}, \text{Invariant})$$

where:

- $\text{Name}$ is the unique type identifier within its declaring module
- $\text{Params}$ is an optional list of generic type parameters
- $\text{Fields} = \{(f_1, V_1, T_1), \ldots, (f_n, V_n, T_n)\}$ is an ordered sequence of field declarations, where $f_i$ is the field name, $V_i$ is the field visibility, and $T_i$ is the field type
- $\text{Forms}$ is the set of classes the record implements (possibly empty)
- $\text{Invariant}$ is an optional predicate that constrains valid instances

Two record types are **equivalent** if and only if they refer to the same declaration:

$$\frac{D(R_1) = D(R_2)}{\Gamma \vdash R_1 \equiv R_2} \quad \text{(T-Equiv-Record)}$$

##### Syntax & Declaration

**Grammar**

```ebnf
record_decl      ::= [visibility] "record" identifier [generic_params] 
                     [implements_clause] "{" record_body "}" [type_invariant]

record_body      ::= record_member ("," record_member)* ","?

record_member    ::= field_decl | method_def

field_decl       ::= [visibility] identifier ":" type

method_def       ::= [visibility] "procedure" identifier [generic_params]
                     "(" [param_list] ")" ["->" return_type] block

implements_clause ::= "<:" class_list
class_list       ::= type_path ("," type_path)*

type_invariant   ::= "where" "{" predicate "}"
```

See §8 for `type_path` definition; §10 for `predicate` definition.

**Record Literal**

A record value is constructed using a literal that specifies the type name and provides values for all fields:

```ebnf
record_literal   ::= type_path "{" field_init_list "}"
field_init_list  ::= field_init ("," field_init)* ","?
field_init       ::= identifier ":" expression
                   | identifier
```

**Field Shorthand**

When the initializing expression is a variable with the same name as the field, the shorthand syntax `{ field }` is equivalent to `{ field: field }`.

**Field Access**

Fields of a record instance are accessed using dot notation:

```ebnf
field_access     ::= expression "." identifier
```

**Type Invariant**

A record declaration MAY include a `where` clause to specify a type invariant. The syntax, semantics, predicate context, and enforcement points are defined in §11.3.1 (Type Invariants).

##### Static Semantics

**Typing Rules**

**(T-Record-Type)**
A record type is well-formed when all field types are well-formed and all fields have distinct names:
$$\frac{\forall i \in 1..n,\ \Gamma \vdash T_i\ \text{wf} \quad \forall i \neq j,\ f_i \neq f_j}{\Gamma \vdash \texttt{record}\ R\ \{f_1: T_1, \ldots, f_n: T_n\}\ \text{wf}} \quad \text{(T-Record-WF)}$$

**(T-Record-Literal)**
A record literal has the declared record type when each field initializer has the corresponding field type:
$$\frac{\forall i \in 1..n,\ \Gamma \vdash e_i : T_i \quad R = \{f_1: T_1, \ldots, f_n: T_n\}}{\Gamma \vdash R\ \{f_1: e_1, \ldots, f_n: e_n\} : R} \quad \text{(T-Record-Lit)}$$

**(T-Field-Access)**
Accessing field $f$ of a record yields the field's type:
$$\frac{\Gamma \vdash e : R \quad R.\text{fields}(f) = T \quad \text{visible}(f, \Gamma)}{\Gamma \vdash e.f : T} \quad \text{(T-Field)}$$

**Nominal Equivalence**

Record types have nominal equivalence per §4.1 (T-Equiv-Nominal). Two records with identical structure but different names are distinct types.

**Field Visibility**

Record fields use the same visibility modifiers defined in §8.5 (Visibility and Access Control). The default visibility for record fields is `private`. Field visibility MUST NOT exceed the visibility of the enclosing record type.

**Class Implementation**

A record declaration MAY implement classes using the `<:` syntax. See §9.3 for class implementation syntax and semantics.

**Invariant Enforcement**

Invariant enforcement modes and verification strategies are defined in §11.4.

**Permission Propagation**

The permission of a record value propagates to field access. If binding `r` has permission $P$, then `r.f` has permission $P$ for all accessible fields $f$.

**Subtyping**

Record types do not participate in structural subtyping. A record type $R$ is a subtype of a class type $\textit{Cl}$ if and only if $R$ implements $\textit{Cl}$:

$$\frac{R\ \texttt{<:}\ \textit{Cl}}{\Gamma \vdash R <: \textit{Cl}} \quad \text{(Sub-Record-Class)}$$

##### Memory & Layout

**Representation**

A record MUST be laid out as a contiguous sequence of its field values. Layout MUST follow Universal Layout Principles (Clause 5 introduction).

**Size and Alignment**

$$\text{alignof}(R) = \max_{f \in \text{fields}(R)}(\text{alignof}(T_f))$$

$$\text{sizeof}(R) \geq \sum_{f \in \text{fields}(R)} \text{sizeof}(T_f)$$

**C-Compatible Layout**

When the `[[layout(C)]]` attribute is applied to a record declaration, the implementation MUST produce a C-compatible memory layout as specified in §7.2.1 (The `[[layout(...)]]` Attribute).

**Zero-Field Records**

A record with no fields has size 0 and alignment 1:

$$\text{sizeof}(\texttt{record}\ R\ \{\}) = 0 \qquad \text{alignof}(\texttt{record}\ R\ \{\}) = 1$$

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                      | Detection    | Effect    |
| :----------- | :------- | :--------------------------------------------- | :----------- | :-------- |
| `E-TYP-1901` | Error    | Duplicate field name in record declaration.    | Compile-time | Rejection |
| `E-TYP-1902` | Error    | Missing field initializer in record literal.   | Compile-time | Rejection |
| `E-TYP-1903` | Error    | Duplicate field initializer in record literal. | Compile-time | Rejection |
| `E-TYP-1904` | Error    | Access to nonexistent field.                   | Compile-time | Rejection |
| `E-TYP-1905` | Error    | Access to field not visible in current scope.  | Compile-time | Rejection |
| `E-TYP-1906` | Error    | Field visibility exceeds record visibility.    | Compile-time | Rejection |
| `P-TYP-1909` | Panic    | Type invariant violated at construction.       | Runtime      | Panic     |
| `P-TYP-1910` | Panic    | Type invariant violated at procedure boundary. | Runtime      | Panic     |

---

### 5.4 Enums

##### Definition

An **enum** (enumerated type) is a nominal sum type. An enum value is exactly one of several defined **variants**, each of which may carry a payload of associated data. Unlike union types (§5.5), which are structural and anonymous, enums are nominal: two enum types with identical variants but different names are distinct types.

A **variant** is a named alternative within an enum. Each variant is identified by name and may optionally contain associated data in one of three forms: unit-like (no data), tuple-like (positional data), or record-like (named fields).

A **discriminant** is a compile-time-assigned integer value that uniquely identifies each variant within an enum. The discriminant enables runtime determination of which variant is active and is stored alongside the payload in the enum's memory representation.

**Formal Definition**

Let $\mathcal{E}$ denote the set of all enum types. An enum type $E$ is defined by:

$$E = (\text{Name}, \text{Params}, \text{Variants}, \text{Forms}, \text{Invariant})$$

where:

- $\text{Name}$ is the unique type identifier within its declaring module
- $\text{Params}$ is an optional list of generic type parameters
- $\text{Variants} = \{(v_1, D_1, P_1), \ldots, (v_n, D_n, P_n)\}$ is a non-empty sequence of variant declarations, where $v_i$ is the variant name, $D_i$ is the discriminant value, and $P_i$ is the optional payload type
- $\text{Forms}$ is the set of classes the enum implements (possibly empty)
- $\text{Invariant}$ is an optional predicate that constrains valid instances

Two enum types are **equivalent** if and only if they refer to the same declaration:

$$\frac{D(E_1) = D(E_2)}{\Gamma \vdash E_1 \equiv E_2} \quad \text{(T-Equiv-Enum)}$$

##### Syntax & Declaration

**Grammar**

```ebnf
enum_decl        ::= [visibility] "enum" identifier [generic_params]
                     [implements_clause] "{" variant_list "}" [type_invariant]

variant_list     ::= variant ("," variant)* ","?

variant          ::= identifier [variant_payload] ["=" integer_constant]

variant_payload  ::= "(" type_list ")"
                   | "{" field_decl_list "}"

type_list        ::= type ("," type)* ","?

field_decl_list  ::= field_decl ("," field_decl)* ","?
```

The `field_decl`, `implements_clause`, and `type_invariant` productions are defined in §5.3.

**Variant Forms**

Variants may take one of three forms:

| Form        | Syntax                       | Payload Structure                    |
| :---------- | :--------------------------- | :----------------------------------- |
| Unit-like   | `Variant`                    | No associated data                   |
| Tuple-like  | `Variant(T₁, T₂, ...)`       | Positional fields, accessed by index |
| Record-like | `Variant { f₁: T₁, f₂: T₂ }` | Named fields, accessed by name       |

**Value Construction**

An enum value is constructed by qualifying the variant name with the enum's type path:

```ebnf
enum_literal     ::= type_path "::" identifier [variant_args]

variant_args     ::= "(" expression_list ")"
                   | "{" field_init_list "}"
```

See §8 for `type_path` definition.

For tuple-like variants, the arguments are positional expressions. For record-like variants, the arguments are named field initializers following the same rules as record literals (§5.3), including support for field shorthand.

**Explicit Discriminant Assignment**

A variant MAY specify an explicit discriminant value using the `=` syntax followed by a compile-time integer constant:

```cursive
enum Status {
    Pending = 0,
    Active = 1,
    Completed = 100
}
```

**Type Invariant**

An enum declaration MAY include a `where` clause to specify a type invariant. The syntax, semantics, predicate context, and enforcement points are defined in §11.3.1 (Type Invariants).

When the invariant predicate requires access to variant-specific data, it MUST use a `match` expression.

##### Static Semantics

**Typing Rules**

**(T-Enum-Type)**
An enum type is well-formed when all variant payload types are well-formed and all variant names are distinct:
$$\frac{\forall i \in 1..n,\ \Gamma \vdash P_i\ \text{wf} \quad \forall i \neq j,\ v_i \neq v_j}{\Gamma \vdash \texttt{enum}\ E\ \{v_1(P_1), \ldots, v_n(P_n)\}\ \text{wf}} \quad \text{(T-Enum-WF)}$$

**(T-Enum-Variant-Unit)**
A unit-like variant has the enum type:
$$\frac{E \text{ declares variant } V \text{ with no payload}}{\Gamma \vdash E::V : E} \quad \text{(T-Variant-Unit)}$$

**(T-Enum-Variant-Tuple)**
A tuple-like variant has the enum type when each argument has the corresponding payload type:
$$\frac{E \text{ declares variant } V(T_1, \ldots, T_n) \quad \forall i,\ \Gamma \vdash e_i : T_i}{\Gamma \vdash E::V(e_1, \ldots, e_n) : E} \quad \text{(T-Variant-Tuple)}$$

**(T-Enum-Variant-Record)**
A record-like variant has the enum type when each field initializer has the corresponding field type:
$$\frac{E \text{ declares variant } V\{f_1: T_1, \ldots, f_n: T_n\} \quad \forall i,\ \Gamma \vdash e_i : T_i}{\Gamma \vdash E::V\{f_1: e_1, \ldots, f_n: e_n\} : E} \quad \text{(T-Variant-Record)}$$

**Nominal Equivalence**

Enum types have nominal equivalence per §4.1 (T-Equiv-Nominal). Two enums with identical variants but different names are distinct types.

**Discriminant Assignment**

Discriminant values are assigned as follows:

1. If a variant specifies an explicit discriminant value, that value is assigned to the variant.
2. If a variant does not specify an explicit discriminant and is the first variant, it is assigned the value `0`.
3. If a variant does not specify an explicit discriminant and is not the first variant, it is assigned the value of the previous variant's discriminant plus one.

Discriminant values MUST be unique within an enum. The discriminant type is implementation-defined but MUST be an integer type large enough to represent all discriminant values.

**Discriminant Overflow**

If the implicit discriminant value of a variant would exceed the maximum value of the discriminant type (due to sequential assignment following an explicit discriminant), the program is ill-formed.

**Variant Access**

Accessing the data stored within an enum variant MUST be performed using a `match` expression (§12.2). Direct field access on an enum value is forbidden; the active variant must first be determined through pattern matching.

**Exhaustiveness**

A `match` expression on an enum type MUST be exhaustive: the set of patterns in its arms, taken together, MUST cover every variant of the enum. Exhaustiveness checking is mandatory for all enum types. See §12.2 for pattern matching semantics.

**Class Implementation**

An enum declaration MAY implement classes using the `<:` syntax. See §9.3 for class implementation syntax and semantics.

**Invariant Enforcement**

Invariant enforcement modes and verification strategies are defined in §11.4.

**Subtyping**

Enum types do not participate in structural subtyping. An enum type $E$ is a subtype of a class type $\textit{Cl}$ if and only if $E$ implements $\textit{Cl}$:

$$\frac{E\ \texttt{<:}\ \textit{Cl}}{\Gamma \vdash E <: \textit{Cl}} \quad \text{(Sub-Enum-Class)}$$

##### Memory & Layout

**Representation**

An enum MUST be represented as a discriminant followed by a payload region large enough to hold any variant's data:

$$\text{layout}(E) = \text{Discriminant}\ ||\ \text{Payload}\ ||\ \text{Padding}$$

where $||$ denotes contiguous concatenation.

The **discriminant** is an integer value identifying the active variant. The **payload** is storage for the variant's associated data. The payload region is sized to accommodate the largest variant.

**Size and Alignment**

$$\text{sizeof}(E) = \text{sizeof}(\text{Discriminant}) + \max_{v \in \text{Variants}}(\text{sizeof}(P_v)) + \text{Padding}$$

$$\text{alignof}(E) = \max(\text{alignof}(\text{Discriminant}), \max_{v \in \text{Variants}}(\text{alignof}(P_v)))$$

Padding is inserted as necessary to satisfy alignment requirements.

**Discriminant Size**

For enums without a `[[layout(...)]]` attribute, the discriminant size is implementation-defined but MUST be sufficient to represent all discriminant values. Implementations SHOULD use the smallest integer type that can represent all discriminants.

**Niche Optimization (Canonical Definition)**

A **niche** is an invalid bit pattern within a type that cannot represent a valid value. **Niche optimization** is an implementation technique that repurposes niches to encode discriminant information. Implementations MAY elide separate discriminant storage when niche optimization is applied.

Implementations MUST apply niche optimization to enums when variant payloads contain invalid bit patterns (niches) that can unambiguously encode the discriminant.

> **Cross-Reference:** See §5.5 for union types and §6.1 for modal types.

**C-Compatible Layout**

When an enum is annotated with `[[layout(C)]]` or `[[layout(IntType)]]` (e.g., `[[layout(u8)]]`), the implementation MUST use a C-compatible layout as specified in §7.2.1 (The `[[layout(...)]]` Attribute).

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                                   | Detection    | Effect    |
| :----------- | :------- | :---------------------------------------------------------- | :----------- | :-------- |
| `E-TYP-2001` | Error    | Enum declaration contains no variants.                      | Compile-time | Rejection |
| `E-TYP-2002` | Error    | Duplicate variant name in enum declaration.                 | Compile-time | Rejection |
| `E-TYP-2003` | Error    | Duplicate discriminant value in enum declaration.           | Compile-time | Rejection |
| `E-TYP-2004` | Error    | Discriminant value is not a compile-time constant.          | Compile-time | Rejection |
| `E-TYP-2005` | Error    | Direct field access on enum value without pattern matching. | Compile-time | Rejection |
| `E-TYP-2006` | Error    | Infinite type: recursive enum without indirection.          | Compile-time | Rejection |
| `E-TYP-2007` | Error    | Unknown variant name in enum construction.                  | Compile-time | Rejection |
| `E-TYP-2008` | Error    | Variant payload arity mismatch.                             | Compile-time | Rejection |
| `E-TYP-2009` | Error    | Missing field initializer in record-like variant.           | Compile-time | Rejection |
| `E-TYP-2010` | Error    | Discriminant overflow during implicit assignment.           | Compile-time | Rejection |
| `P-TYP-2011` | Panic    | Type invariant violated at construction.                    | Runtime      | Panic     |
| `P-TYP-2012` | Panic    | Type invariant violated at procedure boundary.              | Runtime      | Panic     |

See §12.2 for pattern exhaustiveness diagnostics (`E-SEM-2741`).

---

### 5.5 Union Types

##### Definition

A **union type** is a structural anonymous sum type representing a value that may be one of several distinct types. Union types are written using the pipe operator (`|`) to combine member types. Unlike enums (§5.4), which are nominal and require explicit declaration, union types are structural: two union types are equivalent if they have the same member types, regardless of the order in which those members appear.

A **member type** is one of the component types that constitutes the union. The set of member types defines the possible runtime values that an expression of the union type may hold.

Union types in Cursive are **safe** and **tagged**. Every union value carries a runtime discriminant that identifies the currently active member type, enabling type-safe access through pattern matching.

**Formal Definition**

Let $\mathcal{T}$ denote the universe of all types. A union type $U$ is defined by a non-empty multiset of member types:

$$U = \bigcup_{i=1}^{n} T_i \quad \text{where } n \geq 2 \text{ and } T_i \in \mathcal{T}$$

The union type is denoted syntactically as $T_1 \mid T_2 \mid \cdots \mid T_n$.

The **membership relation** $\in_U$ defines which types are members of a union:

$$T \in_U (T_1 \mid T_2 \mid \cdots \mid T_n) \iff \exists i \in 1..n : T \equiv T_i$$

Two union types are **equivalent** if and only if they have the same member type multisets, per rule (T-Equiv-Union) defined in §4.1.

##### Syntax & Declaration

**Grammar**

```ebnf
union_type       ::= type ("|" type)+

type             ::= union_type
                   | non_union_type

non_union_type   ::= primitive_type
                   | nominal_type
                   | tuple_type
                   | array_type
                   | slice_type
                   | function_type
                   | pointer_type
                   | "(" union_type ")"
```

**Precedence and Associativity**

The union type operator `|` has the following properties:

1. **Precedence:** The `|` operator has lower precedence than function type arrows (`->`), array brackets, and pointer constructors. Parentheses are required to use a union type as a function parameter or return type without ambiguity.

2. **Associativity:** The `|` operator is **non-associative** because union type equivalence is defined over multisets, making associativity semantically irrelevant.

**Parsing Disambiguation**

When a `|` appears in a type position, it MUST be interpreted as the union type operator. The following contexts disambiguate union types:

| Context               | Interpretation               |
| :-------------------- | :--------------------------- | :-------------------------- |
| Type annotation       | Union type                   | `let x: i32 \| bool`        |
| Function parameter    | Requires parentheses         | `proc(x: (i32 \| bool))`    |
| Function return       | Requires parentheses         | `-> (i32 \| bool)`          |
| Generic type argument | Union type                   | `Vec<i32 \| string>`        |
| Pattern match binding | Not a type; use member types | `match x { i: i32 => ... }` |

**Value Construction**

A value of union type is constructed by assigning a value of any member type:

```ebnf
union_value      ::= expression
```

No explicit constructor or wrapping syntax is required. When an expression of type $T$ appears in a context expecting a union type $U$ where $T \in_U U$, the value is implicitly wrapped.

##### Static Semantics

**Typing Rules**

**(T-Union-Intro)** Union Introduction:
A value of a member type may be used where a union type is expected:
$$\frac{\Gamma \vdash e : T \quad T \in_U U}{\Gamma \vdash e : U} \quad \text{(T-Union-Intro)}$$

**(T-Union-Member)** Member Type Check:
A type $T$ is a member of union $U = T_1 \mid T_2 \mid \cdots \mid T_n$ if:
$$\frac{\exists i \in 1..n : \Gamma \vdash T \equiv T_i}{\Gamma \vdash T \in_U U} \quad \text{(T-Union-Member)}$$

**(T-Union-WF)** Union Well-Formedness:
A union type is well-formed when all member types are well-formed and there are at least two member types:
$$\frac{n \geq 2 \quad \forall i \in 1..n,\ \Gamma \vdash T_i\ \text{wf}}{\Gamma \vdash T_1 \mid T_2 \mid \cdots \mid T_n\ \text{wf}} \quad \text{(T-Union-WF)}$$

**Recursion Detection**

A union type $U$ is **recursive** if $U$ appears in its own member list without intervening pointer indirection. Formally:

$$\text{Recursive}(U) \iff U \in_U U \land \neg\text{IndirectedOccurrence}(U, U)$$

where $\text{IndirectedOccurrence}(T, U)$ holds when every occurrence of $T$ within the member types of $U$ appears behind a pointer constructor. The following type constructors provide indirection:

- `Ptr<T>` — the safe pointer type
- `const T`, `unique T`, `shared T` — permission-qualified pointer types

A recursive union type without indirection has infinite size and is ill-formed (diagnostic `E-TYP-2203`).

```cursive
type Node = i32 | Ptr<Node>  // OK: recursion behind Ptr
```

```cursive
type Bad = i32 | Bad         // ERROR E-TYP-2203: infinite type
```

**(T-Union-Match)** Union Elimination:
A union value MUST be accessed through exhaustive pattern matching:
$$\frac{\Gamma \vdash e : U \quad U = T_1 \mid \cdots \mid T_n \quad \forall i,\ \Gamma, x_i : T_i \vdash e_i : R}{\Gamma \vdash \texttt{match } e\ \{x_1 : T_1 \Rightarrow e_1, \ldots, x_n : T_n \Rightarrow e_n\} : R} \quad \text{(T-Union-Match)}$$

The result type $R$ may be any well-formed type, including union types.

**Type Equivalence**

Union type equivalence follows (T-Equiv-Union). Equivalence is order-independent:

$$\Gamma \vdash (A \mid B) \equiv (B \mid A)$$

Duplicate members are distinct entries:

$$\Gamma \vdash (A \mid A) \not\equiv A$$

**Subtyping**

Union types participate in the subtype relation in two ways:

**(Sub-Union-Width)** A union type is a subtype of another union type if the first's member types form a subset of the second's:
$$\frac{\forall T \in_U U_1 : T \in_U U_2}{\Gamma \vdash U_1 <: U_2} \quad \text{(Sub-Union-Width)}$$

**(Sub-Union-Depth)** A union type is a subtype of another union type if corresponding members are subtypes:
$$\frac{U_1 = T_1 \mid \cdots \mid T_n \quad U_2 = S_1 \mid \cdots \mid S_n \quad \forall i,\ \exists j : \Gamma \vdash T_i <: S_j}{\Gamma \vdash U_1 <: U_2} \quad \text{(Sub-Union-Depth)}$$

**(Sub-Member-Union)** A single type is a subtype of any union containing it as a member:
$$\frac{T \in_U U}{\Gamma \vdash T <: U} \quad \text{(Sub-Member-Union)}$$

**Coercion**

Implicit coercion from a member type to its containing union type occurs at assignment, argument passing, and return, per the coercion rule (T-Coerce) defined in §4.2. This coercion injects the value into the union representation, setting the appropriate discriminant.

**Access Restriction**

Direct field access, method invocation, or operator application on a union value is forbidden. The active member type MUST first be determined through pattern matching:

$$\Gamma \vdash e : U \implies \Gamma \nvdash e.f : T \quad \text{(Union-No-Direct-Access)}$$

**Exhaustiveness Requirement**

A `match` expression over a union type MUST be exhaustive. The set of patterns MUST cover all member types of the union (§12.2).

**Nested Unions**

When a union type appears as a member of another union, the nested union is **not** automatically flattened. The types `(A | B) | C` and `A | B | C` are distinct:

$$\Gamma \vdash ((A \mid B) \mid C) \not\equiv (A \mid B \mid C)$$

To match a value of type `(A | B) | C`, the outer union must first be matched, then the inner union if applicable.

##### Memory & Layout

**Representation**

A union value MUST be represented as a discriminant followed by a payload region:

$$\text{layout}(U) = \text{Discriminant}\ ||\ \text{Payload}\ ||\ \text{Padding}$$

where $||$ denotes contiguous concatenation.

The **discriminant** (tag) is an integer value identifying which member type is currently active. The **payload** is storage for the active member's data, sized to accommodate the largest member.

**Size and Alignment**

$$\text{sizeof}(U) = \text{sizeof}(\text{Discriminant}) + \max_{T \in_U U}(\text{sizeof}(T)) + \text{Padding}$$

$$\text{alignof}(U) = \max(\text{alignof}(\text{Discriminant}), \max_{T \in_U U}(\text{alignof}(T)))$$

Padding is inserted as necessary to satisfy alignment requirements.

**Discriminant Encoding**

The discriminant value for each member type is assigned based on the **canonical ordering** of member types. The canonical ordering is determined by:

1. Sorting member types lexicographically by their fully-qualified type name.
2. Assigning discriminant values starting from `0` in sorted order.

This deterministic assignment ensures that equivalent union types have identical discriminant mappings.

**Discriminant Size**

The discriminant size is implementation-defined but MUST be sufficient to represent all member types. Implementations SHOULD use the smallest integer type capable of representing all discriminants:

| Member Count | Minimum Discriminant Size |
| :----------- | :------------------------ |
| 2–256        | 1 byte (`u8`)             |
| 257–65,536   | 2 bytes (`u16`)           |
| > 65,536     | 4 bytes (`u32`)           |

**Niche Optimization**

Niche optimization (see §5.4, Niche Optimization under Memory & Layout) MUST be applied to union types.

**Layout Guarantees**

| Aspect             | Classification         |
| :----------------- | :--------------------- |
| Discriminant size  | Implementation-defined |
| Member ordering    | Defined (canonical)    |
| Padding            | Implementation-defined |
| Niche optimization | Defined (MUST apply)   |

**Zero-Sized Types**

If a union contains zero-sized types (ZSTs) as members, those members occupy no payload space but still require a discriminant value. The union's payload size is determined by the largest non-ZST member.

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                              | Detection    | Effect    |
| :----------- | :------- | :----------------------------------------------------- | :----------- | :-------- |
| `E-TYP-2201` | Error    | Union type has fewer than two member types.            | Compile-time | Rejection |
| `E-TYP-2202` | Error    | Direct access on union value without pattern matching. | Compile-time | Rejection |
| `E-TYP-2203` | Error    | Infinite type: recursive union without indirection.    | Compile-time | Rejection |
| `E-TYP-2204` | Error    | Union type used in `[[layout(C)]]` context.            | Compile-time | Rejection |
| `E-SEM-2705` | Error    | `match` expression is not exhaustive for union type.   | Compile-time | Rejection |
| `W-TYP-2201` | Warning  | Union type contains duplicate member types.            | Compile-time | N/A       |

#### 5.5.1 Propagation Operator (`?`)

##### Definition

The **propagation operator** (`?`) provides concise syntax for early return of error-like union members. When applied to a union-typed expression, it either unwraps the "success" member or immediately returns the "error" member to the caller.

**Formal Definition**

Let $e$ be an expression of union type $U = T_1 \mid T_2 \mid \cdots \mid T_n$. Let $R$ be the return type of the enclosing procedure, where $R = S_1 \mid S_2 \mid \cdots \mid S_m$ (or $R$ is a single type).

The expression $e?$ is well-formed when:

1. **Exactly one success type exists:** There exists exactly one member $T_s \in U$ such that $T_s \not<: R$ (the success type).
2. **All other members propagate:** For all $T_i \in U$ where $i \neq s$, either $T_i <: R$ or $T_i$ is a member of $R$ (the error types).

The result type of $e?$ is $T_s$ (the success type).

##### Syntax & Declaration

**Grammar**

```ebnf
try_expr ::= postfix_expr "?"
```

The `?` operator is a postfix operator with precedence 2 (highest, same as field access and method calls).

##### Static Semantics

**Typing Rules**

**(T-Try-Union)** Propagation on Union Type:

$$\frac{
    \Gamma \vdash e : U \quad
    U = T_s \mid T_{e_1} \mid \cdots \mid T_{e_k} \quad
    \forall i \in 1..k,\ T_{e_i} <: R \quad
    T_s \not<: R
}{
    \Gamma \vdash e? : T_s
} \quad \text{(T-Try-Union)}$$

Where $R$ is the return type of the enclosing procedure.

**(T-Try-Single-Error)** Common Case (Single Error Type):

For the common pattern $T \mid E$ where $E <: R$:

$$\frac{
    \Gamma \vdash e : T \mid E \quad
    E <: R \quad
    T \not<: R
}{
    \Gamma \vdash e? : T
} \quad \text{(T-Try-Single)}$$

**Success Type Inference**

The success type $T_s$ is determined by:

1. If exactly one member type is NOT a subtype of (or member of) the return type, that type is the success type.
2. If multiple candidates exist, the program is ill-formed (ambiguous propagation).
3. If no candidates exist (all types propagate), the program is ill-formed.

**Propagation Compatibility**

A union member $T_e$ is **propagation-compatible** with return type $R$ when:

| Return Type Form                      | Compatibility Condition                      |
| :------------------------------------ | :------------------------------------------- |
| Single type $S$                       | $T_e <: S$                                   |
| Union type $S_1 \mid \cdots \mid S_m$ | $\exists j : T_e <: S_j$ OR $T_e \equiv S_j$ |

##### Dynamic Semantics

**Evaluation**

The expression $e?$ evaluates as follows:

1. Evaluate $e$ to obtain value $v$ of union type $U$.
2. Determine the active member type $T_v$ of $v$.
3. **If $T_v$ is the success type $T_s$:** Extract the payload and continue with type $T_s$.
4. **If $T_v$ is a propagating type $T_{e_i}$:** Execute `return v` (coerced to return type $R$ if necessary).

**Desugaring**

$$e? \quad \rightarrow \quad \texttt{match}\ e\ \{\ s: T_s \Rightarrow s,\ e: T_e \Rightarrow \texttt{return}\ e\ \}$$

Where $T_s$ is the success type and $T_e$ ranges over all propagating types.

##### Constraints & Legality

**Negative Constraints**

1. The `?` operator MUST NOT be applied outside a procedure body.
2. The `?` operator MUST NOT be applied to non-union types.
3. The success type MUST be unambiguously determinable.
4. All non-success types MUST be propagation-compatible with the return type.

**Diagnostic Table**

| Code         | Severity | Condition                                                 | Detection    | Effect    |
| :----------- | :------- | :-------------------------------------------------------- | :----------- | :-------- |
| `E-TYP-2210` | Error    | `?` applied to non-union type.                            | Compile-time | Rejection |
| `E-TYP-2211` | Error    | Ambiguous success type in `?` expression.                 | Compile-time | Rejection |
| `E-TYP-2212` | Error    | Union member not propagation-compatible with return type. | Compile-time | Rejection |
| `E-TYP-2213` | Error    | `?` used outside procedure body.                          | Compile-time | Rejection |
| `E-TYP-2214` | Error    | No success type candidate (all types propagate).          | Compile-time | Rejection |

---

## Clause 6: Behavioral Types

### 6.1 Modal Types

##### Definition

A **modal type** is a nominal type that embeds a compile-time-validated state machine into the type system. Modal types statically prevent operations from being performed in an incorrect state, such as dereferencing a null pointer or reading from a closed file. A modal type defines a family of related types:

1. **State-Specific Types (`M@S`)**: Concrete types containing only the data defined in their state's payload. State-specific types do not store a runtime discriminant; the state is tracked statically by the type system.

2. **General Modal Type (`M`)**: A sum type (tagged union) capable of holding a value in any of the declared states. The general type stores the state payload together with a runtime discriminant that identifies the currently active state.

**Formal Definition**

Let $\mathcal{M}$ denote the set of modal type names and let $\text{States}(M)$ denote the finite, non-empty set of state names declared for modal type $M$. For each $M \in \mathcal{M}$ and $S \in \text{States}(M)$:

- $M@S$ denotes the state-specific type for state $S$ of modal $M$.
- $M$ denotes the general modal type.

The type family induced by a modal declaration is:

$$\mathcal{T}_M = \{M\} \cup \{M@S : S \in \text{States}(M)\}$$

A **state transition** is a procedure that consumes a value of one state-specific type and produces a value of another (or the same) state-specific type. Transitions define the edges of the modal state graph:

$$\text{Transitions}(M) \subseteq \text{States}(M) \times \text{States}(M)$$

**Modal Class Implementation**

A modal type may implement one or more classes, including modal classes (§9.2). When a modal implements a modal class, the modal's states must satisfy the class's abstract state requirements per §9.3. The modal may have additional states and fields beyond those required by the class.

##### Syntax & Declaration

**Grammar**

```ebnf
modal_decl        ::= [visibility] "modal" identifier [generic_params]
                      [implements_clause] "{" state_block+ "}"

state_block       ::= "@" state_name "{" state_member* "}"

state_name        ::= identifier

state_member      ::= field_decl | method_def | transition_def

field_decl        ::= identifier ":" type

method_def        ::= "procedure" identifier "(" param_list ")" ["->" return_type] block

transition_def    ::= "transition" identifier "(" param_list ")" "->" "@" target_state block

target_state      ::= identifier
```

The `implements_clause` production is defined in §5.3 (Records). The `param_list` production is defined in §9.1 (Procedure Declarations).

Methods and transitions MUST be defined inline within their state block. Separate declaration and definition is not supported.

**State-Specific Type Syntax**

A state-specific type is written as the modal type name followed by `@` and the state name:

```ebnf
state_specific_type ::= modal_type_name "@" state_name
```

**Syntactic Constraints**

The following syntactic constraints apply to modal declarations:

1. A `modal` declaration MUST contain at least one state block.
2. All state names within a single `modal` declaration MUST be unique.
3. State names MUST NOT collide with the modal type name itself.
4. Within `state_members`, the `self` parameter type is implicitly the enclosing state-specific type.

**Transition Resolution**

A `transition` defined in state `@S` targeting `@T` has:
- Implicit receiver type: `Self` = `M@S` (the enclosing state)
- Return type: `M@T` (the target state-specific type)

The transition `connect` in `@Disconnected` targeting `@Connecting` resolves to the function type: `(unique Connection@Disconnected, Duration) → Connection@Connecting`

##### Static Semantics

**Well-Formedness**

A modal type declaration is well-formed when:

$$\frac{n \geq 1 \quad \forall i \in 1..n,\ S_i \text{ unique} \quad \forall i,\ \text{Payload}(S_i) \text{ wf}}{\Gamma \vdash \texttt{modal } M\ \{@S_1\ \ldots\ @S_n\}\ \text{wf}} \quad \text{(Modal-WF)}$$

**State-Specific Type Formation**

A state-specific type $M@S$ is well-formed when $S$ is a declared state of modal $M$:

$$\frac{S \in \text{States}(M)}{\Gamma \vdash M@S\ \text{wf}} \quad \text{(State-Specific-WF)}$$

**Typing Rules**

**(T-Modal-State-Intro)** State-Specific Value Construction:

A state-specific value is constructed by providing values for all payload fields:

$$\frac{M@S \text{ has payload fields } f_1 : T_1, \ldots, f_k : T_k \quad \forall i,\ \Gamma \vdash e_i : T_i}{\Gamma \vdash M@S\ \{f_1: e_1, \ldots, f_k: e_k\} : M@S} \quad \text{(T-Modal-State-Intro)}$$

**(T-Modal-Field)** State Payload Field Access:

Payload fields are accessible only when the binding has the corresponding state-specific type:

$$\frac{\Gamma \vdash e : M@S \quad f \in \text{Payload}(S) \quad \text{Payload}(S).f : T}{\Gamma \vdash e.f : T} \quad \text{(T-Modal-Field)}$$

Field access through a general modal type $M$ or a different state-specific type $M@S'$ where $S' \neq S$ is ill-formed.

**(T-Modal-Method)** State-Specific Method Invocation:

A method declared in state block `@S` is callable only on bindings of type `M@S`:

$$\frac{\Gamma \vdash e : M@S \quad m \in \text{Methods}(S) \quad m : (M@S, T_1, \ldots, T_n) \to R}{\Gamma \vdash e.m(a_1, \ldots, a_n) : R} \quad \text{(T-Modal-Method)}$$

**Incomparability**

Two distinct state-specific types of the same modal type are **incomparable**. Neither is a subtype of the other:

$$\frac{S_A \neq S_B}{\Gamma \vdash M@S_A \not<: M@S_B \quad \land \quad \Gamma \vdash M@S_B \not<: M@S_A} \quad \text{(Modal-Incomparable)}$$

This rule prohibits implicit conversion between states; state changes MUST occur through explicit transition procedures.

**Modal Class Subtyping**

A modal type that implements a modal class is a subtype of that class:

$$\frac{
  M \text{ is a modal type} \quad
  M <: Cl \quad
  Cl \text{ is a modal class}
}{
  \Gamma \vdash M <: Cl
} \quad \text{(Modal-Class-Sub)}$$

Pattern matching through the class type uses the class's state names; the runtime dispatches to the concrete modal's corresponding state.

**Modal Widening**

Modal widening converts a state-specific type `M@S` to the general modal type `M`:

$$\frac{\Gamma \vdash e : M@S \quad S \in \text{States}(M)}{\Gamma \vdash \texttt{widen } e : M} \quad \text{(T-Modal-Widen)}$$

**Widening Rules Summary:**

| Type Category               | `widen` Keyword | Implicit Coercion | Subtyping      |
| :-------------------------- | :-------------- | :---------------- | :------------- |
| Non-niche-layout-compatible | Required        | Prohibited        | $M@S \not<: M$ |
| Niche-layout-compatible     | Optional        | Permitted         | $M@S <: M$     |

A state-specific type $M@S$ is **niche-layout-compatible** with $M$ when all of the following conditions hold:

1. **Niche Optimization Applies:** The modal type uses niche encoding (see §5.4, Niche Optimization under Memory & Layout) rather than an explicit discriminant field. This requires at least one payload field with an unused bit pattern that can serve as a state discriminant.

2. **Size Equality:** The state-specific type has the same size as the general type: $\text{sizeof}(M@S) = \text{sizeof}(M)$.

3. **Alignment Equality:** The state-specific type has the same alignment as the general type: $\text{alignof}(M@S) = \text{alignof}(M)$.

4. **No Discriminant Storage Overhead:** The discriminant is encoded entirely within niche bits of the payload; no additional storage is required for the state tag.

When these conditions hold, converting from $M@S$ to $M$ is a no-op at runtime—the bit representation is identical, and only the type system's view changes.

**Widen Expression Constraints:**

1. The operand of `widen` MUST have a state-specific modal type `M@S` (diagnostic `E-TYP-2071` if non-modal).
2. The result type is the general modal type `M`.
3. Applying `widen` to an already-general modal type is ill-formed (diagnostic `E-TYP-2072`).

For storage costs and runtime widening semantics, see **Modal Widening Operation** under Dynamic Semantics below.

**Transition Typing**

A transition procedure consumes a value of the source state and produces a value of the target state:

$$\frac{\Gamma \vdash e_{\text{self}} : P_{\text{src}}\ M@S_{\text{src}} \quad (S_{\text{src}}, S_{\text{tgt}}) \in \text{Transitions}(M) \quad \forall i,\ \Gamma \vdash a_i : T_i}{\Gamma \vdash e_{\text{self}}.t(a_1, \ldots, a_n) : M@S_{\text{tgt}}} \quad \text{(T-Modal-Transition)}$$

where $P_{\text{src}}$ MUST be `unique` for consuming transitions.

The implementation of a transition procedure MUST return a value of exactly the declared target state-specific type:

$$\frac{\Gamma \vdash \text{body} : M@S_{\text{tgt}}}{\text{transition } M\text{::}t(\ldots) \to M@S_{\text{tgt}}\ \{\text{body}\}\ \text{well-typed}}$$

**Visibility of Payload Fields**

Fields declared within a state payload are implicitly `protected`. They are accessible only from:

1. Procedures declared within the same modal type's state blocks.
2. Transition implementations for the modal type.
3. Associated class implementations for the modal type.

External code MUST NOT directly access payload fields; access is mediated through methods and pattern matching.

##### Dynamic Semantics

**State Transition Evaluation**

When a transition procedure is invoked:

1. The receiver value (in the source state) MUST be consumed.
2. The transition body MUST execute, constructing a new value of the target state.
3. The resulting value MUST be returned with the target state-specific type.

After a consuming transition, the original binding is statically invalid; the source state's data has been moved into the transition.

**Pattern Matching**

Pattern matching provides the mechanism for safely narrowing a general modal type to a state-specific type.

**(Match on General Type)** When the scrutinee has general modal type $M$:

1. The runtime discriminant (or niche encoding) is inspected to determine the active state.
2. Control transfers to the arm matching the active state.
3. Within that arm, the bound variable has the corresponding state-specific type.

The match MUST be exhaustive: all declared states MUST be covered by match arms.

**(Match on State-Specific Type)** When the scrutinee has state-specific type $M@S$:

The state is statically known. A match expression on $M@S$ is treated as irrefutable payload destructuring, analogous to matching on a record type. Coverage of other states is neither required nor permitted.

**Pattern Syntax**

```ebnf
modal_pattern     ::= "@" state_name ["{" payload_pattern "}"]

payload_pattern   ::= (field_name [":" pattern] ("," field_name [":" pattern])* ","?)?
```

When a field pattern omits the `: pattern` suffix, the field name binds a variable of the field's type.

##### Memory & Layout

**State-Specific Type Layout**

The layout of a state-specific type $M@S$ is equivalent to a `record` containing the fields declared in the payload of state $@S$:

$$\text{layout}(M@S) \equiv \text{layout}(\texttt{record}\ \{\text{Payload}(S)\})$$

If the state payload is empty, the state-specific type is a zero-sized type (ZST):

$$\text{Payload}(S) = \emptyset \implies \text{sizeof}(M@S) = 0$$

State-specific types carry no runtime discriminant; the type system tracks the state statically.

**General Modal Type Layout**

The layout of the general modal type $M$ is equivalent to a tagged union (`enum`) where each variant corresponds to a declared state:

$$\text{layout}(M) \equiv \text{layout}(\texttt{enum}\ \{S_1(\text{Payload}(S_1)),\ \ldots,\ S_n(\text{Payload}(S_n))\})$$

The general type stores:

1. A **discriminant** (tag) identifying the currently active state.
2. A **payload region** sized to accommodate the largest state payload.
3. **Padding** as required for alignment.

$$\text{sizeof}(M) = \text{sizeof}(\text{Discriminant}) + \max_{S \in \text{States}(M)}(\text{sizeof}(M@S)) + \text{Padding}$$

$$\text{alignof}(M) = \max(\text{alignof}(\text{Discriminant}),\ \max_{S \in \text{States}(M)}(\text{alignof}(M@S)))$$

**Niche Optimization**

Niche optimization (see §5.4, Niche Optimization under Memory & Layout) MUST be applied to modal types. When applicable, the general type has the same size as the largest state-specific type.

**Modal Widening Operation**

Modal widening converts a state-specific type $M@S$ to the general modal type $M$ via the **explicit `widen` keyword**.

**Semantic Operation**

When `widen e` is evaluated where $e : M@S$:

1. Storage for the general representation is allocated (stack or destination).
2. The payload from $e$ is moved into the corresponding storage region of the general representation.
3. The discriminant (or niche encoding) for state $@S$ is written.
4. The resulting value has type $M$.

**Size Relationships:**

$\text{sizeof}(M@S) \leq \text{sizeof}(M)$

The inequality is strict when multiple states exist with different payload sizes AND niche optimization does not fully eliminate the discriminant. The inequality is an equality when only one state exists OR niche optimization fully absorbs the discriminant into payload bits.

```cursive
let p1: Ptr<Buffer>@Valid = &buffer
let p2: Ptr<Buffer> = p1        // Implicit widening: no copy when niche-layout-compatible
let p3: Ptr<Buffer> = widen p1  // Explicit `widen` also permitted (for clarity)
```

**Narrowing via Pattern Match**

The converse of widening—narrowing from $M$ to $M@S$—is **not** an implicit coercion. Narrowing requires explicit pattern matching, which performs a runtime discriminant check:

```cursive
let conn: Connection = get_connection()

// Narrowing requires pattern match
match conn {
    @Connected { socket } => {
        // Here: socket has type Socket
        // Type of matched value: Connection@Connected
        socket.send(data)
    }
    @Connecting { timeout } => { /* ... */ }
    @Disconnected => { /* ... */ }
}
```

**Diagnostic Table Addition:**

| Code         | Severity | Condition                                                                             | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------------------------------------------------ | :----------- | :-------- |
| `E-TYP-2070` | Error    | Implicit modal widening on non-niche-layout-compatible type (missing `widen` keyword) | Compile-time | Rejection |
| `E-TYP-2071` | Error    | `widen` applied to non-modal type                                                     | Compile-time | Rejection |
| `E-TYP-2072` | Error    | `widen` applied to already-general modal type                                         | Compile-time | Rejection |
| `W-SYS-4010` | Warning  | Modal widening involves large payload copy (> threshold)                              | Compile-time | Advisory  |

The threshold for `W-SYS-4010` is Implementation-Defined but SHOULD be configurable. A reasonable default is 256 bytes.

##### Constraints & Legality

Modal type constraints are enforced by typing rules (T-Modal-*). Key constraints:

1. A modal type MUST declare at least one state.
2. Direct access on general modal type $M$ requires pattern matching.
3. Match expressions on $M$ MUST be exhaustive.

**Diagnostic Table**

| Code         | Severity | Condition                                                                             | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------------------------------------------------ | :----------- | :-------- |
| `E-TYP-2050` | Error    | Modal type declares zero states.                                                      | Compile-time | Rejection |
| `E-TYP-2051` | Error    | Duplicate state name within modal type declaration.                                   | Compile-time | Rejection |
| `E-TYP-2052` | Error    | Field access for field not present in the current state's payload.                    | Compile-time | Rejection |
| `E-TYP-2053` | Error    | Method invocation for method not available in the current state.                      | Compile-time | Rejection |
| `E-TYP-2054` | Error    | State name collides with modal type name.                                             | Compile-time | Rejection |
| `E-TYP-2055` | Error    | Transition body returns a value not matching the declared target state-specific type. | Compile-time | Rejection |
| `E-TYP-2056` | Error    | Transition invoked on value not of the declared source state-specific type.           | Compile-time | Rejection |
| `E-TYP-2057` | Error    | Direct field or method access on general modal type without pattern matching.         | Compile-time | Rejection |
| `E-TYP-2060` | Error    | Non-exhaustive match on general modal type; missing states must be listed.            | Compile-time | Rejection |

---

### 6.2 String Types

##### Definition

The **`string`** type is a built-in modal type representing sequences of Unicode scalar values encoded as UTF-8. The `string` type has two states:

1. **`string@Managed`**: An owned, heap-allocated, mutable string buffer responsible for its underlying memory allocation.

2. **`string@View`**: A non-owning, immutable slice into string data, referencing either a `string@Managed` buffer or a statically allocated string literal.

All string content MUST be valid UTF-8 as defined in §2.1 (Source Text Encoding) and RFC 3629. The type system enforces this invariant statically for literals and at runtime for slicing operations.

**Formal Definition**

The `string` type is defined as a member of the modal type family (§6.1):

$$\text{States}(\texttt{string}) = \{\ \texttt{@Managed},\ \texttt{@View}\ \}$$

The type family induced by the `string` modal type is:

$$\mathcal{T}_{\texttt{string}} = \{\ \texttt{string},\ \texttt{string@Managed},\ \texttt{string@View}\ \}$$

Per the modal incomparability rule (§6.1), `string@Managed` and `string@View` are pairwise incomparable. Conversion between states requires explicit operations (see Dynamic Semantics).

##### Syntax & Declaration

The `string` type is a built-in primitive; it is not user-declarable.

**Grammar**

```ebnf
string_type         ::= "string" ["@" string_state]

string_state        ::= "Managed" | "View"
```

String literal syntax and escape sequences are defined in §2.8.

##### Static Semantics

**Typing Rules**

**(T-String-Literal)** String Literal Typing:

A string literal has type `string@View`. The literal content MUST be valid UTF-8:

$$\frac{\Gamma \vdash s \text{ is a valid string literal}}{\Gamma \vdash s : \texttt{string@View}} \quad \text{(T-String-Literal)}$$

**(T-String-Slice)** Slice Typing:

A slice operation on any string type produces a `string@View`:

$$\frac{\Gamma \vdash e : \texttt{string@}S \quad S \in \{\texttt{Managed}, \texttt{View}\} \quad \Gamma \vdash a : \texttt{usize} \quad \Gamma \vdash b : \texttt{usize}}{\Gamma \vdash e[a..b] : \texttt{string@View}} \quad \text{(T-String-Slice)}$$

**Modal Widening**

Both state-specific types are subtypes of the general modal type `string` per the modal widening rule (§6.1):

$$\frac{S \in \{\texttt{@Managed}, \texttt{@View}\}}{\Gamma \vdash \texttt{string@}S <: \texttt{string}} \quad \text{(Sub-Modal-Widen)}$$

A procedure accepting the general type `string` may receive either state; the active state is determined via pattern matching.

**Class Implementations**

| State            | `Copy` | `Clone` | `Drop` |
| :--------------- | :----- | :------ | :----- |
| `string@Managed` | No     | No      | Yes    |
| `string@View`    | Yes    | Yes     | No     |

The following constraints apply per Appendix D.1:

1. `string@Managed` MUST NOT implement `Copy` because it owns a heap allocation.
2. `string@Managed` MUST NOT implement `Clone`. Duplication MUST use the explicit `clone_with(heap)` method to inject the required capability (see Dynamic Semantics).
3. `string@Managed` MUST implement `Drop` to deallocate its buffer when responsibility ends.
4. `string@View` MUST implement `Copy` because it is a non-owning pointer-length pair.
5. `string@View` MUST implement `Clone` (implied by `Copy` per Appendix D.1).

##### Dynamic Semantics

**String Literal Evaluation**

When a string literal is evaluated:

1. The implementation allocates the literal content in static, read-only memory during compilation.
2. At runtime, a `string@View` value is constructed containing a pointer to this static memory and the byte length.

String literals have static storage duration; their backing memory is never deallocated.

**State Conversion**

Conversion between `string@Managed` and `string@View` requires explicit operations:

**(Managed to View)** The `as_view` method creates an immutable view of a managed string's buffer:

```cursive
procedure as_view(self: const string@Managed) -> string@View
```

This operation has complexity $O(1)$. The resulting view borrows from the managed string; the view's lifetime MUST NOT exceed the managed string's lifetime.

**(View to Managed)** The `to_managed` method creates a new managed string by copying a view's data:

```cursive
procedure to_managed(self: const string@View, heap: dyn HeapAllocator) -> string@Managed
```

This operation has complexity $O(n)$ where $n$ is the view's byte length.

**`string@Managed` Operations**

Operations on `string@Managed` that may allocate or reallocate the buffer MUST receive a `HeapAllocator` capability parameter (§11.5):

- **Construction:** `string::from(source: string@View, heap: dyn HeapAllocator): string@Managed`
- **Mutation:** `append(self: unique, data: string@View, heap: dyn HeapAllocator)`
- **Duplication:** `clone_with(self: const, heap: dyn HeapAllocator): string@Managed`

**`string@View` Operations**

The `string@View` state provides non-mutating methods:

- `length(self: const): usize` — Returns the byte length.
- `is_empty(self: const): bool` — Returns `true` if the length is zero.
- `chars(self: const): CharIterator` — Returns an iterator over Unicode scalar values.

**Slicing Evaluation**

When a slice expression `s[a..b]` is evaluated:

1. Let `start` be the value of `a`.
2. Let `end` be the value of `b`.
3. If `start > end`, the operation MUST panic (`E-TYP-2153`).
4. If `end > s.length()`, the operation MUST panic (`E-TYP-2154`).
5. If `start` does not fall on a valid UTF-8 character boundary, the operation MUST panic (`E-TYP-2151`).
6. If `end` does not fall on a valid UTF-8 character boundary, the operation MUST panic (`E-TYP-2151`).
7. A new `string@View` is constructed with pointer `s.pointer + start` and length `end - start`.

**UTF-8 Character Boundary Definition**

A byte offset `i` within a UTF-8 string is a **valid character boundary** if and only if one of the following holds:

1. `i == 0` (start of string).
2. `i == length` (end of string).
3. The byte at offset `i` is NOT a UTF-8 continuation byte (the two high bits are NOT `10`).

##### Memory & Layout

**`string@Managed` Representation**

| Field      | Type            | Offset  | Description                            |
| :--------- | :-------------- | :------ | :------------------------------------- |
| `pointer`  | `Ptr<u8>@Valid` | 0       | Pointer to heap-allocated UTF-8 buffer |
| `length`   | `usize`         | 1 word  | Number of bytes of valid content       |
| `capacity` | `usize`         | 2 words | Total allocated buffer size in bytes   |

$$\text{sizeof}(\texttt{string@Managed}) = 3 \times \text{sizeof}(\texttt{usize})$$

$$\text{alignof}(\texttt{string@Managed}) = \text{alignof}(\texttt{usize})$$

**`string@View` Representation**

| Field     | Type                  | Offset | Description                           |
| :-------- | :-------------------- | :----- | :------------------------------------ |
| `pointer` | `Ptr<const u8>@Valid` | 0      | Pointer to the first byte of the view |
| `length`  | `usize`               | 1 word | Number of bytes in the view           |

$$\text{sizeof}(\texttt{string@View}) = 2 \times \text{sizeof}(\texttt{usize})$$

$$\text{alignof}(\texttt{string@View}) = \text{alignof}(\texttt{usize})$$

**General `string` Type Representation**

The general modal type `string` uses standard modal type layout (§6.1): a discriminant plus a payload region sized to the largest state.

**Storage Characteristics**

- String content is NOT null-terminated; length is tracked explicitly.
- All content MUST be valid UTF-8.
- The `pointer` field of `string@View` for string literals points into static, read-only memory.
- The `pointer` field of `string@Managed` points into heap-allocated memory.

**Layout Classification**

| Aspect                  | Classification         |
| :---------------------- | :--------------------- |
| `string@View` size      | Defined (2 words)      |
| `string@Managed` size   | Defined (3 words)      |
| Field ordering          | Defined                |
| Buffer null-termination | Defined (not present)  |
| Content encoding        | Defined (UTF-8)        |
| Pointer alignment       | Implementation-Defined |

##### Constraints & Legality

**Indexing Prohibition**

Direct byte indexing on any string type using the subscript operator is forbidden:

$$\Gamma \vdash e : \texttt{string@}S \implies e[i] \text{ is ill-formed}$$

where `i` is a single index expression (not a range).

**Slicing Requirements**

1. Slice indices are interpreted as **byte offsets**, not character indices.
2. Both boundaries MUST fall on valid UTF-8 character boundaries.
3. Boundary violations MUST cause a panic at runtime.

**Responsibility Constraints**

The `string@Managed` state follows standard responsibility semantics (§3.5):

1. A `string@Managed` binding MUST have exactly one responsible owner.
2. Transfer of responsibility MUST use explicit `move`.
3. When responsibility ends, the `Drop` implementation deallocates the buffer.

**Diagnostic Table**

| Code         | Severity | Condition                                     | Detection    | Effect    |
| :----------- | :------- | :-------------------------------------------- | :----------- | :-------- |
| `E-TYP-2151` | Error    | Slice boundary not on UTF-8 char boundary.    | Runtime      | Panic     |
| `E-TYP-2152` | Error    | Direct byte indexing (`s[i]`) on string type. | Compile-time | Rejection |
| `E-TYP-2153` | Error    | Slice end index less than start index.        | Runtime      | Panic     |
| `E-TYP-2154` | Error    | Slice index exceeds string length.            | Runtime      | Panic     |

---

### 6.3 Pointer Types

##### Definition

A **pointer type** is a type whose values are memory addresses. Cursive provides two families of pointer types designed for distinct purposes:

1. **Safe Modal Pointers (`Ptr<T>`)**: The primary pointer type for safe code. `Ptr<T>` is a built-in modal type with compile-time-tracked states that statically prevent null-pointer dereferences and use-after-free errors.

2. **Raw Pointers (`*imm T`, `*mut T`)**: Unsafe, C-style pointers providing no safety guarantees. Raw pointers are intended for `unsafe` blocks and Foreign Function Interface (FFI) interoperability.

**Formal Definition**

The `Ptr<T>` type is defined as a member of the modal type family (§6.1):

$$\text{States}(\texttt{Ptr<T>}) = \{\ \texttt{@Valid},\ \texttt{@Null},\ \texttt{@Expired}\ \}$$

The type family induced by the `Ptr<T>` modal type is:

$$\mathcal{T}_{\texttt{Ptr<T>}} = \{\ \texttt{Ptr<T>},\ \texttt{Ptr<T>@Valid},\ \texttt{Ptr<T>@Null},\ \texttt{Ptr<T>@Expired}\ \}$$

Per §6.1, `Ptr<T>` state-specific types are pairwise incomparable.

The raw pointer types form a separate type family:

$$\mathcal{T}_{\text{RawPtr}} = \{\ \texttt{*imm T},\ \texttt{*mut T}\ :\ T \in \mathcal{T}\ \}$$

where $\mathcal{T}$ is the set of all types.

##### Syntax & Declaration

The `Ptr<T>` type is a built-in primitive; it is not user-declarable.

**Grammar**

```ebnf
safe_pointer_type     ::= "Ptr" "<" type ">" ["@" pointer_state]

pointer_state         ::= "Valid" | "Null" | "Expired"

raw_pointer_type      ::= "*" raw_pointer_qual type

raw_pointer_qual      ::= "imm" | "mut"
```

**Address-Of Operator**

The `&` operator creates a safe pointer from a valid memory location:

```ebnf
address_of_expr       ::= "&" place_expr
```

**Null Pointer Constructor**

A null pointer is created using the built-in constructor:

```ebnf
null_ptr_expr         ::= "Ptr" "::" "null" "()"
```

**Raw Pointer Cast**

A safe pointer in the `@Valid` state may be cast to a raw pointer:

```ebnf
raw_ptr_cast_expr     ::= safe_ptr_expr "as" raw_pointer_type
```

##### Static Semantics

**Pointer States**

The `Ptr<T>` type has three compile-time states that track pointer validity:

| State      | Description                                                                           | Dereferenceable |
| :--------- | :------------------------------------------------------------------------------------ | :-------------- |
| `@Valid`   | The pointer is guaranteed to be non-null and to point to live, accessible memory.     | Yes             |
| `@Null`    | The pointer is guaranteed to be null (address `0x0`).                                 | No              |
| `@Expired` | The pointer was valid but now references deallocated memory (e.g., an exited region). | No              |

**Typing Rules**

**(T-Addr-Of)** Address-Of Operator:

The `&` operator applied to a valid place expression produces a pointer in the `@Valid` state:

$$\frac{\Gamma \vdash e : T \quad e\ \text{is a valid place expression}}{\Gamma \vdash \texttt{\&}e : \texttt{Ptr<T>@Valid}} \quad \text{(T-Addr-Of)}$$

A **place expression** is an expression that denotes a memory location. Place expressions include:
- Variable bindings
- Field access expressions on place expressions
- Indexed access into arrays or slices

**(T-Null-Ptr)** Null Pointer Construction:

The `Ptr::null()` constructor produces a pointer in the `@Null` state:

$$\frac{}{\Gamma \vdash \texttt{Ptr::null()} : \texttt{Ptr<T>@Null}} \quad \text{(T-Null-Ptr)}$$

The type parameter `T` is inferred from context or explicitly annotated.

**(T-Deref)** Dereference Operator:

The dereference operator `*` applied to a safe pointer MUST only be applied to a pointer in the `@Valid` state:

$$\frac{\Gamma \vdash p : \texttt{Ptr<T>@Valid}}{\Gamma \vdash \texttt{*}p : T} \quad \text{(T-Deref)}$$

This rule statically enforces memory safety. An attempt to dereference a pointer in the `@Null` or `@Expired` state is ill-formed.

**(T-Raw-Ptr-Cast)** Raw Pointer Cast:

A safe pointer in the `@Valid` state may be cast to a raw pointer of compatible mutability:

$$\frac{\Gamma \vdash p : \texttt{Ptr<T>@Valid}}{\Gamma \vdash p\ \texttt{as *imm T} : \texttt{*imm T}} \quad \text{(T-Raw-Ptr-Cast-Imm)}$$

$$\frac{\Gamma \vdash p : \texttt{Ptr<T>@Valid}}{\Gamma \vdash p\ \texttt{as *mut T} : \texttt{*mut T}} \quad \text{(T-Raw-Ptr-Cast-Mut)}$$

**(T-Raw-Deref)** Unsafe Raw Pointer Dereference:

Dereferencing a raw pointer is permitted only within an `unsafe` block:

$$\frac{\Gamma \vdash p : \texttt{*imm T} \quad \text{context is } \texttt{unsafe}}{\Gamma \vdash \texttt{*}p : T} \quad \text{(T-Raw-Deref-Imm)}$$

$$\frac{\Gamma \vdash p : \texttt{*mut T} \quad \text{context is } \texttt{unsafe}}{\Gamma \vdash \texttt{*}p : T} \quad \text{(T-Raw-Deref-Mut)}$$

**Modal Subtyping**

The `Ptr<T>` modal type is **niche-layout-compatible**: all runtime-observable states (`@Valid`, `@Null`) can be distinguished via niche encoding (null vs non-null address), and state-specific types share identical layout with the general type. Per the Niche-Induced Layout-Compatible Modal Subtyping rule (§6.1):

$$\frac{S \in \{\texttt{@Valid}, \texttt{@Null}\}}{\Gamma \vdash \texttt{Ptr<T>@}S <: \texttt{Ptr<T>}} \quad \text{(Sub-Ptr-Widen)}$$

Implicit widening is permitted; a procedure accepting `Ptr<T>` may receive any state-specific pointer directly.

**Class Implementations**

| State            | `Copy` | `Clone` | `Drop` |
| :--------------- | :----- | :------ | :----- |
| `Ptr<T>@Valid`   | Yes    | Yes     | No     |
| `Ptr<T>@Null`    | Yes    | Yes     | No     |
| `Ptr<T>@Expired` | Yes    | Yes     | No     |
| `*imm T`         | Yes    | Yes     | No     |
| `*mut T`         | Yes    | Yes     | No     |

The following constraints apply per Appendix D.1:

1. All pointer types (safe and raw) MUST implement `Copy` because they are address values.
2. All pointer types MUST implement `Clone` (implied by `Copy` per Appendix D.1).
3. No pointer type implements `Drop`. Pointers do not own the memory they reference; the referenced data's lifetime is managed by the responsibility system (§3.5) or region system (§3.7).

##### Dynamic Semantics

**Address-Of Evaluation**

When an address-of expression `&e` is evaluated:

1. Let `loc` be the memory location denoted by place expression `e`.
2. Construct a `Ptr<T>@Valid` value containing the address of `loc`.
3. Return this pointer value.

The resulting pointer's validity is tied to the lifetime of the referenced storage. When the storage is deallocated, pointers into that storage transition to the `@Expired` state.

**Null Pointer Evaluation**

When `Ptr::null()` is evaluated:

1. Construct a `Ptr<T>@Null` value containing address `0x0`.
2. Return this pointer value.

**Dereference Evaluation**

> **Cross-Reference:** The evaluation semantics of the dereference operator `*` are defined in §12.4.5 (Unary Operators).

**Region Exit State Transition**

When a region block (§3.7) exits:

1. For every pointer `p : Ptr<T>@Valid` whose referent was allocated in the exiting region:
2. `p` transitions to state `Ptr<T>@Expired`.
3. Subsequent attempts to dereference `p` are ill-formed.

See "The `@Expired` State is Compile-Time Only" below for details on this static-only state transition.

**Raw Pointer Dereference Evaluation**

When a raw pointer dereference `*p` is evaluated within an `unsafe` block:

1. Let `addr` be the address stored in `p`.
2. Read the value of type `T` stored at address `addr`.
3. Return this value.

The implementation provides no safety guarantees. If `addr` is null, dangling, misaligned, or points to uninitialized memory, the behavior is Unverifiable Behavior (UVB) per §1.2.

##### Memory & Layout

**Safe Pointer Representation**

The safe pointer type `Ptr<T>` uses niche optimization (§5.4) to achieve a single-word representation:

| State      | Representation                    |
| :--------- | :-------------------------------- |
| `@Valid`   | Non-zero address (pointer to `T`) |
| `@Null`    | Address `0x0`                     |
| `@Expired` | Formerly-valid address            |

$$\text{sizeof}(\texttt{Ptr<T>}) = \text{sizeof}(\texttt{usize})$$

$$\text{alignof}(\texttt{Ptr<T>}) = \text{alignof}(\texttt{usize})$$

The general modal type `Ptr<T>` MUST occupy exactly one machine word. The null address (`0x0`) serves as an implicit discriminant for the `@Null` state. Non-zero addresses indicate either `@Valid` or `@Expired`; the distinction between these states is tracked statically, not at runtime.

**The `@Expired` State is Compile-Time Only:** The `@Expired` state has no runtime representation and no discriminant value. At runtime, an expired pointer is indistinguishable from a valid pointer—both hold a non-zero address. The `@Expired` state exists purely within the type system. Any code path where an `@Expired` pointer is dereferenced or widened to the general type is ill-formed; consequently, no runtime check is needed or performed.

**Raw Pointer Representation**

| Type     | Size            | Alignment        |
| :------- | :-------------- | :--------------- |
| `*imm T` | `sizeof(usize)` | `alignof(usize)` |
| `*mut T` | `sizeof(usize)` | `alignof(usize)` |

Raw pointers have identical representation regardless of mutability qualifier. The `imm`/`mut` distinction is a compile-time property only.

**Layout Classification**

| Aspect                  | Classification         |
| :---------------------- | :--------------------- |
| Pointer size            | Implementation-Defined |
| Pointer alignment       | Implementation-Defined |
| `@Null` representation  | Defined (`0x0`)        |
| `@Valid` representation | Defined (non-zero)     |
| Niche optimization      | MUST apply             |

##### Constraints & Legality

**Dereference Constraints**

The following constraints govern pointer dereferencing:

1. The dereference operator `*` applied to `Ptr<T>@Null` is ill-formed.
2. The dereference operator `*` applied to `Ptr<T>@Expired` is ill-formed.
3. The dereference operator `*` applied to a raw pointer (`*imm T` or `*mut T`) outside an `unsafe` block is ill-formed.

**Address-Of Constraints**

The following constraints govern the address-of operator:

1. The operand of `&` MUST be a place expression (a memory location).
2. The operand of `&` MUST be initialized.
3. The resulting pointer MUST NOT escape the lifetime of the referenced storage (see §3.3 Escape Rule).

**Cast Constraints**

The following constraints govern pointer casts:

1. Only `Ptr<T>@Valid` may be cast to a raw pointer.
2. Casting `Ptr<T>@Null` or `Ptr<T>@Expired` to a raw pointer is ill-formed.

**FFI Constraints**

Raw pointers are the only FFI-safe (Clause 21) pointer types:

1. The `Ptr<T>@State` modal type MUST NOT appear in `extern` procedure signatures.
2. Procedures accepting or returning pointers across the FFI boundary MUST use raw pointer types.

**Diagnostic Table**

| Code         | Severity | Condition                                                    | Detection    | Effect    |
| :----------- | :------- | :----------------------------------------------------------- | :----------- | :-------- |
| `E-TYP-2101` | Error    | Dereference of pointer in `@Null` state.                     | Compile-time | Rejection |
| `E-TYP-2102` | Error    | Dereference of pointer in `@Expired` state (use-after-free). | Compile-time | Rejection |
| `E-TYP-2103` | Error    | Dereference of raw pointer outside `unsafe` block.           | Compile-time | Rejection |
| `E-TYP-2104` | Error    | Address-of operator applied to non-place expression.         | Compile-time | Rejection |
| `E-TYP-2105` | Error    | Cast of non-`@Valid` pointer to raw pointer.                 | Compile-time | Rejection |
| `E-TYP-2106` | Error    | Modal pointer type in `extern` procedure signature.          | Compile-time | Rejection |

---

### 6.4 Function Types

##### Definition

A **function type** is a structural type representing a callable signature. Function types describe the interface of callable entities—procedures and closures—as mappings from parameter types to a return type. A function type is defined by its ordered parameter types (including any `move` responsibility modifiers) and its return type.

Cursive distinguishes two representations of function types based on whether the callable captures state from its environment:

1. **Sparse Function Pointer (`(T) -> U`)**: A direct pointer to executable code. Sparse function pointers occupy a single machine word and are FFI-safe. Non-capturing procedures and non-capturing closure expressions have sparse function pointer types.

2. **Closure (`|T| -> U`)**: A dense pointer containing both a code pointer and an environment pointer. Closures occupy two machine words and are NOT FFI-safe. Closure expressions that capture bindings from their enclosing scope have closure types.

This distinction ensures that the representation cost of each callable form is explicit in the type system and that FFI boundary constraints are enforced statically.

**Formal Definition**

Let $\mathcal{F}$ denote the set of all function types. A function type is defined by the tuple:

$$F = (P_1, \ldots, P_n, R, \kappa)$$

where:
- $P_i = (m_i, T_i)$ is a parameter descriptor consisting of an optional `move` modifier $m_i \in \{\epsilon, \texttt{move}\}$ and a type $T_i$
- $R$ is the return type
- $\kappa \in \{\texttt{sparse}, \texttt{closure}\}$ is the representation kind

The type families are:

$$\mathcal{F}_{\text{sparse}} = \{(m_1\ T_1, \ldots, m_n\ T_n) \to R : \forall i,\ T_i \in \mathcal{T},\ R \in \mathcal{T}\}$$

$$\mathcal{F}_{\text{closure}} = \{|m_1\ T_1, \ldots, m_n\ T_n| \to R : \forall i,\ T_i \in \mathcal{T},\ R \in \mathcal{T}\}$$

where $\mathcal{T}$ is the universe of all types.

**Function Type vs. Procedure Declaration**

A function type is distinct from a procedure declaration. A `procedure` declaration (Clause 7) is a named, top-level item that *has* a corresponding function type. The function type captures the procedure's complete callable interface without its name or contract annotations.

| Aspect                | Procedure Declaration           | Function Type                |
| :-------------------- | :------------------------------ | :--------------------------- |
| Nature                | Named, top-level item           | Structural, anonymous type   |
| Name                  | Yes                             | No                           |
| Contract (`[[...]]`)  | Yes (as metadata, per Clause 7) | No                           |
| Capability parameters | Yes (as parameters)             | Yes (as parameter types)     |
| `move` modifiers      | Yes (on parameters)             | Yes (part of parameter type) |

##### Syntax & Declaration

**Grammar**

```ebnf
function_type         ::= sparse_function_type | closure_type

sparse_function_type  ::= "(" [param_type_list] ")" "->" type

closure_type          ::= "|" [param_type_list] "|" "->" type

param_type_list       ::= param_type ("," param_type)*

param_type            ::= ["move"] type
```

**Syntactic Disambiguation**

The `|` token serves as both a closure type delimiter and the bitwise OR operator. Disambiguation follows these rules:

1. In type position (type annotations, return types, generic bounds), `|` introduces a closure type and MUST be followed by a parameter type list, a closing `|`, `->`, and a return type.
2. In expression position, `|` is the bitwise OR operator.

Type position is recognized by syntactic context: following `:` in declarations, following `->` in procedure signatures, or within generic parameter bounds.

**Syntactic Constraints**

The following syntactic constraints apply to function types:

1. The `param_type_list` is a comma-separated sequence of zero or more parameter types. Each parameter type MAY be prefixed with the `move` keyword.
2. Parameter types include capability types (e.g., `FileSystem`, `HeapAllocator`) on equal footing with data types.
3. The `->` token separates the parameter list from the return type.
4. The return type MUST be a single type. Procedures returning no meaningful value use the unit type `()`.
5. Parameter names are not part of function type syntax; only types appear.

**Type Identity**

A function type's identity is determined by the tuple $(P_1, \ldots, P_n, R, \kappa)$ as defined in the Formal Definition above. Specifically:

1. Representation kind (sparse or closure) is part of identity; `(T) -> U` and `|T| -> U` are distinct types.
2. The `move` modifier is part of identity; `(T) -> U` and `(move T) -> U` are distinct types.
3. Contract annotations and parameter names are NOT part of function type identity.

##### Static Semantics

**Well-Formedness (T-Func-WF)**

A function type is well-formed if and only if all its constituent types are well-formed:

$$\frac{\Gamma \vdash R\ \text{wf} \quad \forall i \in 1..n,\ \Gamma \vdash T_i\ \text{wf}}{\Gamma \vdash (m_1\ T_1, \ldots, m_n\ T_n) \to R\ \text{wf}} \quad \text{(T-Func-WF)}$$

where $m_i \in \{\epsilon, \texttt{move}\}$ is the optional `move` modifier for parameter $i$.

The same rule applies to closure types:

$$\frac{\Gamma \vdash R\ \text{wf} \quad \forall i \in 1..n,\ \Gamma \vdash T_i\ \text{wf}}{\Gamma \vdash |m_1\ T_1, \ldots, m_n\ T_n| \to R\ \text{wf}} \quad \text{(T-Closure-WF)}$$

**Type Equivalence**

Function type equivalence, including representation kind and `move` modifiers, is governed by rules **(T-Equiv-Func)** and **(T-Equiv-Func-Extended)** defined in §4.1.


**When Representation Kind Matters**

The distinction between sparse function pointers `(T) -> U` and closures `|T| -> U` affects type checking in the following contexts:

1. **Storage and Passing:** Closures require two machine words (code pointer + environment pointer), while sparse function pointers require one. A binding of type `|T| -> U` cannot hold a value of type `(T) -> U` without implicit coercion (which is permitted via subtyping, see below).

2. **FFI Boundaries:** Only sparse function pointer types are FFI-safe (§21). A procedure accepting a callback from foreign code MUST declare that parameter with sparse function pointer type `(T) -> U`, not closure type `|T| -> U`.

3. **Environment Capture:** A closure type indicates that the callable may hold references to captured bindings from its defining scope. Procedures may require this property (accepting `|T| -> U`) when the callback must close over external state, or prohibit it (accepting `(T) -> U`) when callbacks must be stateless.

4. **Coercion Direction:** A sparse function pointer coerces to a closure (one direction only). Code accepting `|T| -> U` can receive either representation; code accepting `(T) -> U` can receive only non-capturing callables.

**Variance**

Function types exhibit contravariant parameters and covariant return types as defined by rule `(Var-Func)` in §4.3. This variance applies to both sparse function pointer types and closure types independently.


**Representation Subtyping (T-Sparse-Sub-Closure)**

A sparse function pointer is a subtype of the corresponding closure type with the same parameter and return types:

$$\frac{\Gamma \vdash (T_1, \ldots, T_n) \to R\ \text{wf}}{\Gamma \vdash (T_1, \ldots, T_n) \to R <: |T_1, \ldots, T_n| \to R} \quad \text{(T-Sparse-Sub-Closure)}$$

This rule permits a non-capturing function to be used where a capturing closure is expected. The implicit coercion constructs a closure value with a null environment pointer (see Memory & Layout). The converse does NOT hold: a closure type is NOT a subtype of a sparse function pointer type.

**Move Modifier Subtyping (T-NonMove-Sub-Move)**

A function type with a non-`move` parameter is a subtype of a function type with a `move` parameter for the same underlying type:

$$\frac{\Gamma \vdash T\ \text{wf}}{\Gamma \vdash (T) \to R <: (\texttt{move } T) \to R} \quad \text{(T-NonMove-Sub-Move)}$$

This rule permits a procedure that does not take responsibility for an argument to substitute for a procedure that does. The caller prepares for the argument's responsibility to be transferred, but the callee declines to accept responsibility, which is always safe. The reverse substitution is NOT permitted: a procedure that requires responsibility transfer cannot substitute for one that does not.

This rule extends to corresponding parameters in multi-parameter function types and applies equally to sparse and closure representations.

**Combined Subtyping**

$$\Gamma \vdash (T) \to R <: |\texttt{move } T| \to R$$

**Typing Rules**

**(T-Proc-As-Value)** Procedure Reference:

When a procedure name appears in value position (not immediately followed by an argument list), the expression has the sparse function pointer type corresponding to the procedure's signature:

$$\frac{\text{procedure } f(m_1\ x_1 : T_1, \ldots, m_n\ x_n : T_n) \to R\ \text{declared}}{\Gamma \vdash f : (m_1\ T_1, \ldots, m_n\ T_n) \to R} \quad \text{(T-Proc-As-Value)}$$

**(T-Closure-Sparse)** Non-Capturing Closure Expression:

A closure expression that captures no bindings from its environment has a sparse function pointer type:

$$\frac{\Gamma \vdash |p_1, \ldots, p_n| \to e : (T_1, \ldots, T_n) \to R \quad \text{captures}(|p_1, \ldots, p_n| \to e) = \emptyset}{\Gamma \vdash |p_1, \ldots, p_n| \to e : (T_1, \ldots, T_n) \to R} \quad \text{(T-Closure-Sparse)}$$

**(T-Closure-Capturing)** Capturing Closure Expression:

A closure expression that captures one or more bindings from its environment has a closure type:

$$\frac{\Gamma \vdash |p_1, \ldots, p_n| \to e : (T_1, \ldots, T_n) \to R \quad \text{captures}(|p_1, \ldots, p_n| \to e) \neq \emptyset}{\Gamma \vdash |p_1, \ldots, p_n| \to e : |T_1, \ldots, T_n| \to R} \quad \text{(T-Closure-Capturing)}$$

> See §11.5 for the capture analysis algorithm.

**(T-Call)** Function Invocation:

A value of function type may be invoked with arguments matching the parameter types:

$$\frac{\Gamma \vdash f : (m_1\ T_1, \ldots, m_n\ T_n) \to R \quad \forall i \in 1..n,\ \Gamma \vdash a_i : T_i}{\Gamma \vdash f(a_1, \ldots, a_n) : R} \quad \text{(T-Call)}$$

When $m_i = \texttt{move}$, the argument $a_i$ MUST be passed via an explicit `move` expression, and the source binding becomes invalid after the call (per §3.5).

The same rule applies to closure type values.

##### Memory & Layout

**Sparse Function Pointer Representation**

A sparse function pointer MUST be represented as a single machine word containing the address of the callable code:

$$\text{sizeof}((T_1, \ldots, T_n) \to R) = \text{sizeof}(\texttt{usize})$$

$$\text{alignof}((T_1, \ldots, T_n) \to R) = \text{alignof}(\texttt{usize})$$

The representation is a code pointer:

```
┌─────────────────────────────────┐
│          code_ptr               │  ← pointer to executable code
└─────────────────────────────────┘
         (1 machine word)
```

**Closure Representation**

A closure MUST be represented as a two-word structure containing an environment pointer and a code pointer:

$$\text{sizeof}(|T_1, \ldots, T_n| \to R) = 2 \times \text{sizeof}(\texttt{usize})$$

$$\text{alignof}(|T_1, \ldots, T_n| \to R) = \text{alignof}(\texttt{usize})$$

The representation is:

```
┌─────────────────────────────────┬─────────────────────────────────┐
│          env_ptr                │          code_ptr               │
└─────────────────────────────────┴─────────────────────────────────┘
         (1 machine word)                  (1 machine word)
```

where:
- `env_ptr` points to the captured environment (or is null for non-capturing callables coerced to closure type).
- `code_ptr` points to the executable code, which expects `env_ptr` as an implicit first argument.

**Sparse-to-Closure Coercion**

When a sparse function pointer value is coerced to a closure type (via the subtyping rule T-Sparse-Sub-Closure):

1. A closure value is constructed.
2. The `env_ptr` field is set to null.
3. The `code_ptr` field is set to a thunk that ignores the environment parameter and calls the original code.

Implementations MAY optimize this coercion to avoid the thunk when the calling convention permits.

**Layout Classification**

| Aspect                    | Classification         |
| :------------------------ | :--------------------- |
| Sparse function pointer   | Defined (1 word)       |
| Closure type              | Defined (2 words)      |
| Field ordering in closure | Defined (env, code)    |
| Environment layout        | Implementation-Defined |


##### Constraints & Legality

**Structural Constraints**

The following structural constraints apply to function types:

1. A function type MUST have zero or more parameters and exactly one return type.
2. The return type `()` (unit) MUST be used when a callable returns no meaningful value.
3. The `move` modifier MUST NOT appear on the return type; it applies only to parameters.

**FFI Constraints**

Closure types are NOT FFI-safe:

1. Closure types (`|T| -> U`) MUST NOT appear in `extern` procedure signatures (Clause 21).
2. Only sparse function pointer types are permitted at FFI boundaries.
3. Sparse function pointers used in FFI contexts MUST NOT have generic type parameters.

**Invocation Constraints**

The following constraints apply when invoking a function type value:

1. The number of arguments MUST equal the number of parameters.
2. Each argument type MUST be a subtype of the corresponding parameter type.
3. For parameters with the `move` modifier, the argument MUST be an explicit `move` expression.

**Diagnostic Table**

| Code         | Severity | Condition                                                    | Detection    | Effect    |
| :----------- | :------- | :----------------------------------------------------------- | :----------- | :-------- |
| `E-TYP-2220` | Error    | Argument count mismatch: expected $n$ arguments, found $m$.  | Compile-time | Rejection |
| `E-TYP-2221` | Error    | Type mismatch in function argument or return position.       | Compile-time | Rejection |
| `E-TYP-2222` | Error    | Missing `move` on argument to `move` parameter.              | Compile-time | Rejection |
| `E-TYP-2223` | Error    | Closure type in `extern` procedure signature.                | Compile-time | Rejection |
| `E-TYP-2224` | Error    | Assignment of closure value to sparse function pointer type. | Compile-time | Rejection |
| `E-TYP-2225` | Error    | `move` modifier on return type.                              | Compile-time | Rejection |
| `E-TYP-2226` | Error    | Function type has invalid structure (zero return types).     | Compile-time | Rejection |

---

## Clause 7: Type Extensions

### 7.1 Static Polymorphism (Generics)

##### Definition

A **generic declaration** introduces one or more **type parameters** that serve as placeholders for concrete types supplied at instantiation. Generic declarations are resolved entirely at compile time via **monomorphization**.

**Formal Definition**

A generic declaration $D$ is defined by:

$$D = (\text{Name}, \text{Params}, \text{Body})$$

where:

- $\text{Name}$ is the declaration's identifier
- $\text{Params} = \langle P_1, P_2, \ldots, P_n \rangle$ is an ordered sequence of type parameters
- $\text{Body}$ is the declaration body (type definition or procedure body)

Each type parameter $P_i$ is defined by:

$$P_i = (\text{name}_i, \text{Bounds}_i)$$

where:

- $\text{name}_i$ is an identifier serving as the parameter name
- $\text{Bounds}_i \subseteq \mathcal{T}_{\text{form}}$ is a (possibly empty) set of class bounds that constrain valid type arguments

A type parameter with $\text{Bounds}_i = \emptyset$ is **unconstrained**; any type may be substituted. A type parameter with $\text{Bounds}_i \neq \emptyset$ is **constrained**; only types implementing all classes in $\text{Bounds}_i$ may be substituted.

**Monomorphization**

**Monomorphization** is the process of generating specialized code for each concrete instantiation of a generic type or procedure. Given a generic declaration $D\langle T_1, \ldots, T_n \rangle$ and concrete type arguments $A_1, \ldots, A_n$, monomorphization produces a specialized declaration $D[A_1/T_1, \ldots, A_n/T_n]$ where each occurrence of $T_i$ in the body is replaced with $A_i$.

##### Syntax & Declaration

**Grammar**

```ebnf
(* Generic parameter declarations use semicolon to separate parameters *)
generic_params     ::= "<" generic_param_list ">"
generic_param_list ::= generic_param (";" generic_param)*  (* semicolon separates parameters *)
generic_param      ::= identifier [bound_clause] [default_clause]
bound_clause       ::= "<:" class_bound_list
default_clause     ::= "=" type
class_bound_list   ::= class_bound ("," class_bound)*      (* comma separates bounds *)

(* Where clauses: semicolons separate predicates; newline-before-identifier-<: is implicit semicolon *)
where_clause         ::= "where" where_predicate_list
where_predicate_list ::= where_predicate (predicate_separator where_predicate)* [";"]
where_predicate      ::= identifier "<:" class_bound_list
predicate_separator  ::= ";" | NEWLINE_BEFORE_BOUND   (* NEWLINE followed by identifier "<:" *)

(* Type argument lists use comma to separate arguments *)
generic_args       ::= "<" type_arg_list ">"
type_arg_list      ::= type ("," type)*                    (* comma separates type arguments *)

turbofish          ::= "::" generic_args
```

The `class_bound` production is defined in §9.2 (Form Declarations). A `class_bound` matches a class name such as `Ord`, `Display`, or `Clone`.

**Generic Parameter Declaration**

A generic parameter list appears after the name in type and procedure declarations:

```cursive
record Container<T> { ... }
procedure sort<T <: Ord>(arr: unique [T]) { ... }
```

**Bound Clause Syntax**

A type parameter MAY be followed by a bound clause using the `<:` operator. Multiple class bounds on a single parameter are separated by commas. Multiple type parameters are separated by semicolons:

```cursive
<T>                         // One unconstrained parameter
<T; U>                      // Two unconstrained parameters
<T <: Display>              // One parameter with single bound
<T <: Display, Ord>         // One parameter with multiple bounds (must implement both)
<T <: Display; U>           // Two parameters: T bounded, U unconstrained
<T <: Display; U <: Clone>  // Two parameters, each with one bound
<T <: Display, Ord; U <: Clone, Hash>  // Two parameters, each with multiple bounds
```

**Separator Hierarchy**

The semicolon (`;`) is the *parameter separator*; the comma (`,`) is the *bound separator*:

| Separator | Role                                  | Scope                            |
| --------- | ------------------------------------- | -------------------------------- |
| `;`       | Separates type parameters             | Between `<` and `>`              |
| `,`       | Separates bounds within one parameter | After `<:` until next `;` or `>` |

This hierarchy ensures unambiguous parsing: a comma after `<:` always adds another bound to the current parameter; a semicolon always begins a new parameter.

**Where Clause Syntax**

As an alternative to inline bounds, constraints MAY be specified in a `where` clause following the parameter list. Where clause predicates are separated by semicolons; bounds within a predicate are separated by commas:

```cursive
procedure compare<T>(a: T, b: T) -> Ordering
where T <: Ord
{ ... }

procedure process<T; U>(x: T, y: U) -> string
where T <: Display, Clone;
      U <: Hash
{ ... }
```

A newline following a complete predicate (after all its bounds) MAY serve as an implicit semicolon when the next line begins with an identifier followed by `<:`. This permits the visually aligned style shown above.

When both inline bounds and a `where` clause specify constraints for the same parameter, the effective constraint is the conjunction of all specified bounds (the type must satisfy all bounds from both sources).

**Parsing Rules**

The semicolon (`;`) separates type parameters; the comma (`,`) separates bounds within a single parameter. This eliminates ambiguity:

1. Within a generic parameter list `<...>`:
   - Semicolon (`;`) terminates the current parameter and begins a new one
   - Comma (`,`) after `<:` adds another bound to the current parameter
   - Comma (`,`) *without* a preceding `<:` in the current parameter is **ill-formed** (`E-SRC-0501`)

2. Within a where clause:
   - Semicolon (`;`) or newline-before-identifier-`<:` terminates the current predicate
   - Comma (`,`) adds another bound to the current predicate

##### Static Semantics

**Well-Formedness of Generic Parameters (WF-Generic-Param)**

A generic parameter list is well-formed if all parameter names are distinct and all class bounds reference valid class types:

$$\frac{
    \forall i \neq j,\ \text{name}_i \neq \text{name}_j \quad
    \forall i,\ \forall B \in \text{Bounds}_i,\ \Gamma \vdash B : \text{Form}
}{
    \Gamma \vdash \langle P_1; \ldots; P_n \rangle\ \text{wf}
} \quad \text{(WF-Generic-Param)}$$

**Generic Type Well-Formedness (WF-Generic-Type)**

A generic type declaration is well-formed if its parameter list is well-formed and its body is well-formed under a context extended with the type parameters:

$$\frac{
    \Gamma \vdash \langle P_1, \ldots, P_n \rangle\ \text{wf} \quad
    \Gamma, T_1 : P_1, \ldots, T_n : P_n \vdash \text{Body}\ \text{wf}
}{
    \Gamma \vdash \texttt{type}\ \textit{Name}\langle P_1, \ldots, P_n \rangle\ \text{Body}\ \text{wf}
} \quad \text{(WF-Generic-Type)}$$

**Generic Procedure Well-Formedness (WF-Generic-Proc)**

A generic procedure declaration is well-formed if its parameter list is well-formed and its signature and body are well-formed under a context extended with the type parameters:

$$\frac{
    \Gamma \vdash \langle P_1, \ldots, P_n \rangle\ \text{wf} \quad
    \Gamma' = \Gamma, T_1 : P_1, \ldots, T_n : P_n \quad
    \Gamma' \vdash \text{signature}\ \text{wf} \quad
    \Gamma' \vdash \text{body}\ \text{wf}
}{
    \Gamma \vdash \texttt{procedure}\ f\langle P_1, \ldots, P_n \rangle(\ldots) \to R\ \{\ldots\}\ \text{wf}
} \quad \text{(WF-Generic-Proc)}$$

**Constraint Satisfaction (T-Constraint-Sat)**

A type argument $A$ satisfies a constraint set $\text{Bounds}$ if $A$ implements all forms in $\text{Bounds}$:

$$\frac{
    \forall B \in \text{Bounds},\ \Gamma \vdash A <: B
}{
    \Gamma \vdash A\ \text{satisfies}\ \text{Bounds}
} \quad \text{(T-Constraint-Sat)}$$

**Generic Instantiation (T-Generic-Inst)**

A generic type instantiation is well-formed if all type arguments satisfy their corresponding constraints:

$$\frac{
    \text{Name}\langle P_1, \ldots, P_n \rangle\ \text{declared} \quad
    \forall i \in 1..n,\ \Gamma \vdash A_i\ \text{satisfies}\ \text{Bounds}(P_i)
}{
    \Gamma \vdash \text{Name}\langle A_1, \ldots, A_n \rangle\ \text{wf}
} \quad \text{(T-Generic-Inst)}$$

**Generic Procedure Call (T-Generic-Call)**

A call to a generic procedure is well-typed if the inferred or explicit type arguments satisfy all constraints and the value arguments match the specialized parameter types:

$$\frac{
    \begin{gathered}
    f\langle T_1, \ldots, T_n \rangle(x_1 : S_1, \ldots, x_m : S_m) \to R\ \text{declared} \\
    \forall i \in 1..n,\ \Gamma \vdash A_i\ \text{satisfies}\ \text{Bounds}(T_i) \\
    \forall j \in 1..m,\ \Gamma \vdash e_j : S_j[A_1/T_1, \ldots, A_n/T_n]
    \end{gathered}
}{
    \Gamma \vdash f\langle A_1, \ldots, A_n \rangle(e_1, \ldots, e_m) : R[A_1/T_1, \ldots, A_n/T_n]
} \quad \text{(T-Generic-Call)}$$

where $T_i$ are type parameters and $A_i$ are the corresponding type arguments.

**Type Argument Inference**

When type arguments are not explicitly provided, the implementation MUST infer them using bidirectional type inference (§4.4). Type arguments are inferred from:

1. The types of value arguments at the call site
2. The expected return type from the surrounding context

If type arguments cannot be uniquely determined, the program is ill-formed with diagnostic `E-TYP-2301`.

**Variance of Generic Parameters**

The variance of each type parameter is determined by its usage within the type definition. See §4.3 for the complete variance specification and subtyping rules for generic types.

**Monomorphization Requirements**

Implementations MUST generate specialized code for each distinct combination of concrete type arguments used in the program. The following requirements apply:

1. **Specialization:** For each instantiation $D\langle A_1, \ldots, A_n \rangle$, the implementation MUST produce code equivalent to manually substituting each type argument for its corresponding parameter throughout the declaration body.

2. **Zero Overhead:** Calls to generic procedures MUST compile to direct static calls to the specialized instantiation. Virtual dispatch (vtable lookup) is prohibited for static polymorphism.

3. **Independent Instantiation:** Each distinct instantiation is an independent type or procedure. `Container<i32>` and `Container<i64>` are distinct types with no implicit relationship.

**Generic Procedures in Classes**

When a generic procedure is declared within a class, it cannot participate in dynamic dispatch (vtable lookup) because the type arguments are not known at the dyn call site. Such procedures MUST be marked with `[[static_dispatch_only]]` (§7.2.10) and can only be called through concrete type references, not through `dyn Class` references.

**Recursion Depth Limit**

Monomorphization MAY produce recursive instantiations (e.g., `List<Option<List<T>>>`). Implementations MUST detect and reject infinite monomorphization recursion.

The maximum instantiation depth is IDB. Implementations MUST support a minimum instantiation depth of 128 (i.e., 128 nested generic instantiations within a single monomorphization chain).


##### Memory & Layout

**Layout Independence**

Each monomorphized instantiation has an independent memory layout. The layout of `Container<i32>` is computed using the rules for `i32` as the element type; the layout of `Container<i64>` is computed independently using `i64`.

**Size and Alignment**

The size and alignment of a generic type instantiation are determined by substituting the concrete type arguments and applying the layout rules for the resulting concrete type (see Universal Layout Principles in §5 and §7.2.1).

$$\text{sizeof}(\text{Name}\langle A_1, \ldots, A_n \rangle) = \text{sizeof}(\text{Name}[A_1/T_1, \ldots, A_n/T_n])$$

$$\text{alignof}(\text{Name}\langle A_1, \ldots, A_n \rangle) = \text{alignof}(\text{Name}[A_1/T_1, \ldots, A_n/T_n])$$

##### Constraints & Legality

**Negative Constraints**

The following constraints apply to generic declarations and instantiations:

1. A generic parameter list MUST NOT contain duplicate parameter names.
2. A class bound MUST reference a valid class type; bounding by non-class types is forbidden.
3. A generic instantiation MUST provide exactly the number of type arguments declared by the generic; partial application is not permitted.
4. Type arguments MUST satisfy all constraints (class bounds) declared for their corresponding parameters.
5. Generic parameters are **prohibited** in `extern` procedure signatures. FFI boundaries require monomorphic types.
6. Infinite monomorphization recursion MUST be detected and rejected.
7. The monomorphization depth MUST NOT exceed the implementation's documented limit.

**Diagnostic Table**

| Code         | Severity | Condition                                                           | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------------------------------ | :----------- | :-------- |
| `E-TYP-2301` | Error    | Type arguments cannot be inferred; explicit instantiation required. | Compile-time | Rejection |
| `E-TYP-2302` | Error    | Type argument does not satisfy class bound.                         | Compile-time | Rejection |
| `E-TYP-2303` | Error    | Wrong number of type arguments for generic instantiation.           | Compile-time | Rejection |
| `E-TYP-2304` | Error    | Duplicate type parameter name in generic declaration.               | Compile-time | Rejection |
| `E-TYP-2305` | Error    | Class bound references a non-class type.                            | Compile-time | Rejection |
| `E-TYP-2306` | Error    | Generic parameter in `extern` procedure signature.                  | Compile-time | Rejection |
| `E-TYP-2307` | Error    | Infinite monomorphization recursion detected.                       | Compile-time | Rejection |
| `E-TYP-2308` | Error    | Monomorphization depth limit exceeded.                              | Compile-time | Rejection |

---

### 7.2 Attributes

##### Definition

An **attribute** is a compile-time annotation attached to a declaration or expression. Attributes influence code generation, memory layout, diagnostics, verification strategies, and interoperability without altering the core semantics of the annotated construct except where this specification explicitly defines such alteration.

**Formal Definition**

An attribute $A$ is defined by:

$$A = (\text{Name}, \text{Args})$$

where:

- $\text{Name}$ is an identifier designating the attribute
- $\text{Args}$ is a (possibly empty) sequence of arguments

An **attribute list** is a sequence of one or more attributes applied to a declaration or expression:

$$\text{AttributeList} = \langle A_1, A_2, \ldots, A_n \rangle$$

The **attribute registry** $\mathcal{R}$ is the set of all attributes recognized by a conforming implementation. The registry partitions into:

$$\mathcal{R} = \mathcal{R}_{\text{spec}} \uplus \mathcal{R}_{\text{vendor}}$$

where $\mathcal{R}_{\text{spec}}$ contains specification-defined attributes and $\mathcal{R}_{\text{vendor}}$ contains vendor-defined extensions.

##### Syntax & Declaration

**Grammar**

```ebnf
attribute_list     ::= attribute+
attribute          ::= "[[" attribute_spec ("," attribute_spec)* "]]"
attribute_spec     ::= attribute_name ["(" attribute_args ")"]
attribute_name     ::= identifier | vendor_prefix "::" identifier
vendor_prefix      ::= identifier ("." identifier)*
attribute_args     ::= attribute_arg ("," attribute_arg)*
attribute_arg      ::= literal
                     | identifier
                     | identifier ":" literal
                     | identifier "(" attribute_args ")"

expression         ::= attributed_expr | unattributed_expr
attributed_expr    ::= attribute_list expression
```

**Placement**

**Declaration-Level Attributes**

An attribute list MUST appear immediately before the declaration it modifies. The attribute list and its target declaration MUST NOT be separated by other declarations or statements.

**Expression-Level Attributes**

An attribute list MAY appear immediately before an expression. The attributed expression has the same type as the underlying expression; the attribute affects only static properties (e.g., verification strategy), not the expression's value or type.

**Attribute Block Equivalence**

Multiple attributes MAY be specified in a single block or in separate blocks. The following forms are semantically equivalent:

```cursive
[[attr1, attr2]]
declaration

[[attr1]]
[[attr2]]
declaration
```

The order of attribute application within equivalent forms is Unspecified Behavior (USB). Implementations MUST ensure that order-independent attributes produce identical results regardless of application order. For attributes whose effects may depend on order (e.g., conflicting layout directives), the program is ill-formed per the conflict rules defined for each attribute.

##### Static Semantics

**Attribute Processing**

Attribute processing occurs during semantic analysis. For each attribute $A$ applied to declaration $D$:

1. The implementation MUST verify that $A.\text{Name} \in \mathcal{R}$.
2. The implementation MUST verify that $D$ is a valid target for $A$ per the target constraints defined below.
3. The implementation MUST verify that $A.\text{Args}$ conforms to the argument specification for $A$.
4. If all verifications succeed, the implementation applies the effect defined for $A$.

The complete set of specification-defined attributes ($\mathcal{R}_{\text{spec}}$) and their full semantics are defined in subsections §7.2.1–§7.2.9 below.

**Attribute Target Matrix**

The following matrix defines which targets are valid for each specification-defined attribute. Applying an attribute to an invalid target is ill-formed (`E-MOD-2452`).

| Attribute                  | `record` | `enum` | `modal` | `procedure` | Field | Parameter | `expression` |
| :------------------------- | :------: | :----: | :-----: | :---------: | :---: | :-------: | :----------: |
| `[[layout(...)]]`          |   Yes    |  Yes   |   No    |     No      |  No   |    No     |      No      |
| `[[inline(...)]]`          |    No    |   No   |   No    |     Yes     |  No   |    No     |      No      |
| `[[cold]]`                 |    No    |   No   |   No    |     Yes     |  No   |    No     |      No      |
| `[[static_dispatch_only]]` |    No    |   No   |   No    |     Yes     |  No   |    No     |      No      |
| `[[deprecated(...)]]`      |   Yes    |  Yes   |   Yes   |     Yes     |  Yes  |    No     |      No      |
| `[[reflect]]`              |   Yes    |  Yes   |   Yes   |     No      |  No   |    No     |      No      |
| `[[link_name(...)]]`       |    No    |   No   |   No    |  Yes (FFI)  |  No   |    No     |      No      |
| `[[no_mangle]]`            |    No    |   No   |   No    |  Yes (FFI)  |  No   |    No     |      No      |
| `[[unwind(...)]]`          |    No    |   No   |   No    |  Yes (FFI)  |  No   |    No     |      No      |
| `[[dynamic]]`              |   Yes    |  Yes   |   Yes   |     Yes     |  No   |    No     |     Yes      |
| `[[relaxed]]`              |    No    |   No   |   No    |     No      |  No   |    No     |     Yes      |
| `[[acquire]]`              |    No    |   No   |   No    |     No      |  No   |    No     |     Yes      |
| `[[release]]`              |    No    |   No   |   No    |     No      |  No   |    No     |     Yes      |
| `[[acq_rel]]`              |    No    |   No   |   No    |     No      |  No   |    No     |     Yes      |
| `[[seq_cst]]`              |    No    |   No   |   No    |     No      |  No   |    No     |     Yes      |

**Vendor Extension Namespacing**

Vendor-defined attributes MUST use a namespaced identifier to prevent collision with specification-defined attributes and other vendors' extensions:

1. Vendor attributes MUST use reverse-domain-style prefixes: `[[com.vendor.attribute_name]]`
2. The `cursive.*` namespace is reserved (§1.3).
3. Vendor-defined attributes are IDB.

An implementation encountering an unknown attribute (one not in $\mathcal{R}$) MUST diagnose error `E-MOD-2451`.

---

#### 7.2.1 The `[[layout(...)]]` Attribute

##### Definition

The `[[layout(...)]]` attribute controls the memory representation of composite types. It specifies field ordering, padding behavior, alignment requirements, and discriminant representation.

##### Syntax & Declaration

**Grammar**

```ebnf
layout_attribute ::= "[[" "layout" "(" layout_args ")" "]]"
layout_args      ::= layout_kind ("," layout_kind)*
layout_kind      ::= "C"
                   | "packed"
                   | "align" "(" integer_literal ")"
                   | int_type
int_type         ::= "i8" | "i16" | "i32" | "i64" | "u8" | "u16" | "u32" | "u64"
```

##### Static Semantics

**`layout(C)` — C-Compatible Layout**

For a `record` marked `[[layout(C)]]`:
1. Fields MUST be laid out in declaration order.
2. Padding MUST be inserted only as required to satisfy alignment constraints per the target platform's C ABI.
3. The total size MUST be a multiple of the record's alignment.

For an `enum` marked `[[layout(C)]]`:
1. The discriminant MUST be represented as a C-compatible integer tag.
2. The default tag type is implementation-defined (IDB).
3. The enum MUST be laid out as a tagged union per the target C ABI.

**`layout(IntType)` — Explicit Discriminant Type**

For an `enum` marked `[[layout(IntType)]]` where `IntType` is one of `i8`, `i16`, `i32`, `i64`, `u8`, `u16`, `u32`, `u64`:
1. The discriminant MUST be represented using the specified integer type.
2. The discriminant value for each variant MUST be representable in the specified type.
3. This form is valid only on `enum` declarations. Applying `[[layout(IntType)]]` to a `record` is ill-formed (`E-MOD-2452`).

**`layout(packed)` — Packed Layout**

For a `record` marked `[[layout(packed)]]`:
1. All inter-field padding is removed.
2. Each field MUST be laid out with alignment 1, regardless of its natural alignment.
3. The record's overall alignment becomes 1.

Packed layout has the following implications:
- Field access MAY require unaligned memory operations.
- Taking a reference to a packed field is Unverifiable Behavior (UVB) because the resulting reference may be misaligned. Such operations MUST occur within an `unsafe` block.
- Direct field reads and writes remain safe (not UVB).

**Direct Access vs Reference-Taking**

The distinction between safe direct access and UVB reference-taking is:

| Operation                          | Classification         |
| :--------------------------------- | :--------------------- |
| `packed.field = value`             | Direct write (safe)    |
| `let x = packed.field`             | Direct read (safe)     |
| `packed.field.method()`            | Reference-taking (UVB) |
| Pass to `move` parameter           | Direct read (safe)     |
| Pass to `const`/`unique` parameter | Reference-taking (UVB) |

The `packed` layout kind is valid only on `record` declarations. Applying `packed` to an `enum` is ill-formed (`E-MOD-2454`).

**`layout(align(N))` — Minimum Alignment**

The `align(N)` layout kind sets a minimum alignment for the type:
1. $N$ MUST be a positive integer that is a power of two. Non-power-of-two values are ill-formed (`E-MOD-2453`).
2. The effective alignment of the type is $\max(N, \text{natural alignment})$.
3. If $N$ is less than the natural alignment, the natural alignment is used and a warning (`W-MOD-2451`) is emitted.
4. The type's size is padded to be a multiple of the effective alignment.

**Valid Combinations**

Multiple layout kinds MAY be combined in a single attribute:

| Combination                | Validity | Effect                                         |
| :------------------------- | :------- | :--------------------------------------------- |
| `layout(C)`                | Valid    | C-compatible layout                            |
| `layout(packed)`           | Valid    | Packed layout (records only)                   |
| `layout(align(N))`         | Valid    | Minimum alignment $N$                          |
| `layout(C, packed)`        | Valid    | C-compatible packed layout                     |
| `layout(C, align(N))`      | Valid    | C-compatible layout with minimum alignment $N$ |
| `layout(u8)` (on enum)     | Valid    | 8-bit unsigned discriminant                    |
| `layout(packed, align(N))` | Invalid  | Conflicting directives (`E-MOD-2455`)          |

**Applicability Constraints**

| Declaration Kind          |  `C`  | `packed` | `align(N)` | `IntType` |
| :------------------------ | :---: | :------: | :--------: | :-------: |
| `record`                  |   ✓   |    ✓     |     ✓      |     ✗     |
| `enum`                    |   ✓   |    ✗     |     ✓      |     ✓     |
| `modal`                   |   ✗   |    ✗     |     ✗      |     ✗     |
| Generic (unmonomorphized) |   ✗   |    ✗     |     ✗      |     ✗     |

Applying `[[layout(...)]]` to a `modal` type or an unmonomorphized generic type is ill-formed (`E-SYS-3303`, as defined in §21).

---

#### 7.2.2 The `[[inline(...)]]` Attribute

##### Definition

The `[[inline(...)]]` attribute specifies procedure inlining behavior.

##### Static Semantics

**`inline(always)`**

The implementation SHOULD inline the procedure at all call sites. If inlining is not possible (e.g., the procedure is recursive, exceeds implementation limits, or its address is taken), the implementation MUST emit warning `W-MOD-2452`.

**`inline(never)`**

The implementation MUST NOT inline the procedure. The procedure body MUST be emitted as a separate callable unit, and all calls MUST use the standard calling convention.

**`inline(default)`**

The implementation applies its default inlining heuristics. This is semantically equivalent to omitting the `[[inline(...)]]` attribute entirely.

**Default Behavior (No Attribute)**

When no `[[inline(...)]]` attribute is present, the implementation MAY inline or not inline the procedure at its discretion based on optimization heuristics.

---

#### 7.2.3 The `[[cold]]` Attribute

##### Definition

The `[[cold]]` attribute marks a procedure as unlikely to be executed during typical program runs. This hint enables the implementation to optimize for the common case by:

1. Placing the procedure's code in a separate section to improve instruction cache locality of hot code.
2. Reducing optimization effort on the cold procedure.
3. Biasing branch prediction at call sites toward not calling the procedure.

##### Static Semantics

The `[[cold]]` attribute takes no arguments. It is valid only on procedure declarations.

The attribute is a hint; implementations MAY ignore it without diagnostic.

---

#### 7.2.4 The `[[deprecated(...)]]` Attribute

##### Definition

The `[[deprecated(...)]]` attribute marks a declaration as deprecated. Usage of a deprecated declaration triggers a warning diagnostic.

##### Syntax & Declaration

```ebnf
deprecated_attribute ::= "[[" "deprecated" ["(" string_literal ")"] "]]"
```

The optional string argument provides a deprecation message that SHOULD be included in the warning diagnostic.

##### Static Semantics

When a program references a declaration marked `[[deprecated(...)]]`:

1. The implementation MUST emit warning `W-CNF-0601` (as defined in §1.5).
2. If a message argument is present, the diagnostic SHOULD include the message text.
3. The deprecated declaration remains fully functional; only a warning is emitted.

---

#### 7.2.5 The `[[reflect]]` Attribute

##### Definition

The `[[reflect]]` attribute enables compile-time introspection for a type declaration. Types marked with `[[reflect]]` have their metadata accessible to the `reflect_type<T>()` intrinsic during compile-time evaluation.

##### Static Semantics

1. The `[[reflect]]` attribute takes no arguments.
2. It is valid on `record`, `enum`, and `modal` type declarations.
3. The attribute has no effect on runtime behavior or memory layout.
4. Full semantics for compile-time introspection are defined in Clause 17 (Compile-Time Execution) and Clause 18 (Type Reflection).

---

#### 7.2.6 FFI Attributes

##### Definition

**FFI Attributes** are attributes specific to Foreign Function Interface declarations. These attributes control linking behavior and panic propagation across the language boundary.

##### Static Semantics

The following FFI attributes are recognized. Complete semantics are defined in Clause 21 (Foreign Function Interface):

| Attribute              | Reference | Effect                                                       |
| :--------------------- | :-------- | :----------------------------------------------------------- |
| `[[link_name("sym")]]` | §21.3     | Overrides the linker symbol name for an extern procedure     |
| `[[no_mangle]]`        | §21.5     | Disables name mangling (implicit for `extern "C"`)           |
| `[[unwind(mode)]]`     | §21.3     | Controls panic behavior at FFI boundary (`abort` or `catch`) |

These attributes are valid only on `extern` procedure declarations.

---

#### 7.2.7 Memory Ordering Attributes

##### Definition

**Memory Ordering Attributes** control the memory ordering semantics of individual memory operations in concurrent code. These attributes allow relaxation of the default sequential consistency ordering for performance-critical operations.

##### Static Semantics

The following memory ordering attributes are recognized. Complete semantics are defined in §14.10 (Memory Ordering):

| Attribute     | Target        | Effect                                |
| :------------ | :------------ | :------------------------------------ |
| `[[relaxed]]` | Memory access | Atomicity only—no ordering guarantees |
| `[[acquire]]` | Memory access | Subsequent reads see prior writes     |
| `[[release]]` | Memory access | Prior writes visible to acquire reads |
| `[[acq_rel]]` | Memory access | Both acquire and release semantics    |
| `[[seq_cst]]` | Memory access | Total global order (default)          |

These attributes are valid only on expressions that perform memory access to `shared` data.

---

#### 7.2.8 Consolidated Constraints and Diagnostics

##### Definition

This subsection consolidates the validation rules and diagnostics that apply to all attributes defined in §7.2.

##### Constraints & Legality

**Negative Constraints**

1. An attribute name not in $\mathcal{R}$ (the attribute registry) MUST be diagnosed as unknown.
2. An attribute applied to an invalid target declaration kind MUST be diagnosed.
3. Malformed attribute syntax (e.g., missing brackets, invalid argument types) MUST be diagnosed.
4. Conflicting layout arguments (`packed` with `align`) MUST be diagnosed.
5. The `align(N)` argument MUST be a power of two.
6. The `packed` layout kind MUST NOT be applied to `enum` declarations.

**Diagnostic Table**

| Code         | Severity | Condition                                             | Detection    | Effect    |
| :----------- | :------- | :---------------------------------------------------- | :----------- | :-------- |
| `E-MOD-2450` | Error    | Malformed attribute syntax.                           | Compile-time | Rejection |
| `E-MOD-2451` | Error    | Unknown attribute name.                               | Compile-time | Rejection |
| `E-MOD-2452` | Error    | Attribute not valid on target declaration kind.       | Compile-time | Rejection |
| `E-MOD-2453` | Error    | `align(N)` where N is not a power of two.             | Compile-time | Rejection |
| `E-MOD-2454` | Error    | `packed` applied to non-record declaration.           | Compile-time | Rejection |
| `E-MOD-2455` | Error    | Conflicting layout arguments (`packed` with `align`). | Compile-time | Rejection |
| `W-MOD-2451` | Warning  | `align(N)` where N < natural alignment (no effect).   | Compile-time | N/A       |
| `W-MOD-2452` | Warning  | `inline(always)` but inlining not possible.           | Compile-time | N/A       |

---

#### 7.2.9 The `[[dynamic]]` Attribute

##### Definition

The **`[[dynamic]]` attribute** marks a declaration or expression as requiring **runtime verification** when static verification is insufficient. This attribute is the explicit opt-in mechanism for runtime checks, reflecting the language's philosophy of **static safety by default**.

##### Syntax & Declaration

**Grammar**

```ebnf
dynamic_attribute     ::= "[[" "dynamic" "]]"
attributed_decl       ::= dynamic_attribute declaration
attributed_expr       ::= dynamic_attribute expression
```

**Valid Targets**

| Target       | Effect                                                     |
| :----------- | :--------------------------------------------------------- |
| `record`     | All operations on instances may use runtime verification   |
| `enum`       | All operations on instances may use runtime verification   |
| `modal`      | All state transitions may use runtime verification         |
| `procedure`  | Contracts and body operations may use runtime verification |
| `expression` | The specific expression may use runtime verification       |

##### Static Semantics

**Attribute Scope**

The **`[[dynamic]]` attribute scope** is the lexical region where the `[[dynamic]]` attribute's effects apply. This is a compile-time verification concept, not a runtime execution context (unlike `region` or `parallel` blocks).

**Definition**: An expression $e$ is within a `[[dynamic]]` scope if and only if:
1. $e$ is syntactically enclosed in a declaration marked `[[dynamic]]`, OR
2. $e$ is syntactically enclosed in an expression marked `[[dynamic]]`

**Propagation Rules**

The `[[dynamic]]` attribute scope propagates inward according to lexical structure:

1. **Declaration-Level Propagation**: When `[[dynamic]]` is applied to a type or procedure declaration, all expressions within that declaration are within the `[[dynamic]]` scope.

2. **Expression-Level Scope**: When `[[dynamic]]` is applied to an expression, only that expression and its sub-expressions are within the `[[dynamic]]` scope.

3. **Lexical Determination**: The attribute scope is determined lexically at compile time. A procedure call from within a `[[dynamic]]` scope does not extend that scope to the callee.

4. **No Transitive Propagation**: The `[[dynamic]]` attribute does not propagate through procedure calls. Each procedure's verification mode is determined by its own attributes.

**Effect on Key System (§13)**

Within a `[[dynamic]]` scope:

- If static key safety analysis fails, runtime synchronization MAY be inserted
- The rules K-Static-Required and K-Dynamic-Permitted (§14.9) govern this behavior
- Same-statement conflicts (§14.6.1) remain compile-time errors regardless of `[[dynamic]]`

**Dynamic Index Ordering**

Within a `[[dynamic]]` scope, `#` blocks containing dynamically-indexed paths that cannot be statically ordered use runtime ordering to determine acquisition sequence. The semantic guarantee is deadlock freedom; the ordering mechanism is Implementation-Defined (see §14.7).

This converts what would otherwise be ill-formed code into safe, deadlock-free code at the cost of runtime ordering computation.

**Effect on Contract System (§10)**

Within a `[[dynamic]]` scope:

- If a contract predicate cannot be proven statically, a runtime check is inserted
- Failed runtime checks result in panic (`P-SEM-2850`)
- The rules Contract-Static-OK, Contract-Static-Fail, Contract-Dynamic-Elide, Contract-Dynamic-Check (§11.4) govern this behavior

**Effect on Refinement Types (§7.3)**

Within a `[[dynamic]]` scope:

- If a refinement predicate cannot be proven statically, a runtime check is inserted
- Failed runtime checks result in panic (`P-TYP-1953`)

##### Dynamic Semantics

**Runtime Behavior**

When `[[dynamic]]` is present and static verification fails:

1. **Key Operations**: Runtime synchronization primitives are inserted to ensure safe concurrent access
2. **Contract Checks**: Runtime assertion code is generated at the appropriate verification point (call site for preconditions, return site for postconditions)
3. **Refinement Validation**: Runtime predicate evaluation is inserted at type coercion points

**Static Elision**

Even within a `[[dynamic]]` scope, if a property is statically provable, no runtime code is generated. The `[[dynamic]]` attribute permits but does not require runtime verification.

##### Constraints & Legality

**Negative Constraints**

1. `[[dynamic]]` MUST NOT be applied to contract clauses directly (`E-CON-0410`); apply to the enclosing procedure instead.
2. `[[dynamic]]` MUST NOT be applied to type alias declarations (`E-CON-0411`).
3. `[[dynamic]]` on a field declaration is ill-formed (`E-CON-0412`); apply to the containing record instead.

**Diagnostic Table**

| Code         | Severity | Condition                                                                 | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------------------------------------ | :----------- | :-------- |
| `E-CON-0410` | Error    | `[[dynamic]]` applied to contract clause directly.                        | Compile-time | Rejection |
| `E-CON-0411` | Error    | `[[dynamic]]` applied to type alias declaration.                          | Compile-time | Rejection |
| `E-CON-0412` | Error    | `[[dynamic]]` applied to field declaration.                               | Compile-time | Rejection |
| `E-CON-0020` | Error    | Key safety not statically provable outside `[[dynamic]]` scope.           | Compile-time | Rejection |
| `E-SEM-2801` | Error    | Contract predicate not statically provable outside `[[dynamic]]` scope.   | Compile-time | Rejection |
| `E-TYP-1953` | Error    | Refinement predicate not statically provable outside `[[dynamic]]` scope. | Compile-time | Rejection |
| `W-CON-0401` | Warning  | `[[dynamic]]` present but all proofs succeed statically.                  | Compile-time | N/A       |

#### 7.2.10 The `[[static_dispatch_only]]` Attribute

##### Definition

The **`[[static_dispatch_only]]` attribute** marks a procedure in a class as excluded from dyn dispatch (vtable). This attribute allows classes to contain procedures that are not vtable-eligible while maintaining overall dyn safety.

##### Syntax & Declaration

**Grammar**

```ebnf
static_dispatch_attr ::= "[[" "static_dispatch_only" "]]"
```

**Valid Targets**

| Target      | Effect                                                                   |
| :---------- | :----------------------------------------------------------------------- |
| `procedure` | Procedure excluded from dyn vtable; callable only through concrete types |

##### Static Semantics

**VTable Eligibility**

A procedure $p$ in a class is **vtable-eligible** if it can be dispatched through a vtable (virtual method table) at runtime. Formally:

$$
\text{vtable\_eligible}(p) \iff \text{has\_receiver}(p) \land \neg\text{is\_generic}(p) \land \text{compatible\_signature}(p)
$$

where:
- $\text{has\_receiver}(p)$: The procedure has a `self` parameter with receiver permission (`~`, `~!`, or `~%`)
- $\text{is\_generic}(p)$: The procedure has one or more type parameters
- $\text{compatible\_signature}(p)$: The return type and all parameter types are representable through a vtable (no `Self` in return position, no associated types that vary by implementation)

**Dyn Safety**

A class is **dyn-safe** if every procedure is either vtable-eligible or explicitly excluded from the vtable:

$$
\text{dyn\_safe}(C) \iff \forall p \in \text{procedures}(C).\ \text{vtable\_eligible}(p) \lor \text{has\_static\_dispatch\_attr}(p)
$$

where $\text{has\_static\_dispatch\_attr}(p)$ holds if $p$ has the `[[static_dispatch_only]]` attribute.

**Call Resolution Rules**

1. **Static Context**: Calls on concrete types resolve to `[[static_dispatch_only]]` procedures normally
2. **Dynamic Context**: Calls on `dyn Class` types cannot resolve to `[[static_dispatch_only]]` procedures (compile error `E-TYP-2640`)

##### Constraints & Legality

**Valid Usage**

- May be applied to any procedure in a class declaration
- Most commonly used for generic procedures or procedures returning `Self`

**Invalid Usage**

- MUST NOT be applied to procedures outside class declarations
- MUST NOT be applied to non-procedure declarations

**Diagnostic Table**

| Code         | Severity | Condition                                                                | Detection    | Effect    |
| :----------- | :------- | :----------------------------------------------------------------------- | :----------- | :-------- |
| `E-TYP-2640` | Error    | Procedure with `[[static_dispatch_only]]` called on dyn.                 | Compile-time | Rejection |
| `E-TYP-2642` | Error    | Generic procedure in class without `[[static_dispatch_only]]` attribute. | Compile-time | Rejection |

---

### 7.3 Refinement Types

##### Definition

A **refinement type** is a type constructed by attaching a predicate constraint to a base type. The refinement type `T where { P }` denotes the subset of values of type `T` for which predicate `P` evaluates to `true`. Refinement types enable dependent typing patterns where type validity depends on runtime-checkable value properties.

**Formal Definition**

Let $\mathcal{T}$ denote the set of all types and $\mathcal{P}$ denote the set of all pure boolean predicates. A refinement type $R$ is defined by:

$$R = (T_{\text{base}}, P)$$

where:

- $T_{\text{base}} \in \mathcal{T}$ is the base type being refined
- $P : T_{\text{base}} \to \texttt{bool}$ is a pure predicate constraining the value set

The value set of a refinement type is the subset of base type values satisfying the predicate:

$$\text{Values}(T \text{ where } \{P\}) = \{ v \in \text{Values}(T) \mid P(v) = \texttt{true} \}$$

A refinement type is a **proper subtype** of its base type; a value of type `T where { P }` may be used wherever a value of type `T` is expected.

##### Syntax & Declaration

**Grammar**

```ebnf
refinement_type       ::= type "where" "{" predicate "}"

type_alias_refine     ::= "type" identifier "=" type "where" "{" predicate "}"

param_with_constraint ::= identifier ":" type "where" "{" predicate "}"

predicate             ::= expression
```

**Self Reference**

Within the predicate of a **type alias** or **standalone refinement type expression**, the keyword `self` refers to the value being constrained. The identifier `self` is implicitly bound within the predicate scope with the base type `T`.

##### Static Semantics

**Well-Formedness (WF-Refine-Type)**

A refinement type `T where { P }` is well-formed when the base type is well-formed and the predicate is a valid pure expression of type `bool` under a context extended with the constrained value:

$$\frac{
    \Gamma \vdash T\ \text{wf} \quad
    \Gamma, \texttt{self} : T \vdash P : \texttt{bool} \quad
    \text{Pure}(P)
}{
    \Gamma \vdash (T \text{ where } \{P\})\ \text{wf}
} \quad \text{(WF-Refine-Type)}$$

**Well-Formedness of Parameter Constraint (WF-Param-Constraint)**

A parameter constraint `x: T where { P }` is well-formed when the base type is well-formed, the predicate is a valid pure expression of type `bool`, and the predicate references the parameter name (not `self`):

$$\frac{
    \Gamma \vdash T\ \text{wf} \quad
    \Gamma, x : T \vdash P : \texttt{bool} \quad
    \text{Pure}(P) \quad
    \texttt{self} \notin \text{FreeVars}(P)
}{
    \Gamma \vdash (x : T \text{ where } \{P\})\ \text{wf}
} \quad \text{(WF-Param-Constraint)}$$

**Typing Rules**

**(T-Refine-Intro)**

A value of base type `T` has refinement type `T where { P }` when a Verification Fact (§11.5) establishes that `P` holds for that value:

$$\frac{
    \Gamma \vdash e : T \quad
    \Gamma \vdash F(P[e/\texttt{self}], L) \quad
    L \text{ dominates current location}
}{
    \Gamma \vdash e : T \text{ where } \{P\}
} \quad \text{(T-Refine-Intro)}$$

**(T-Refine-Elim)**

A value of refinement type may be used as a value of its base type without any runtime check:

$$\frac{
    \Gamma \vdash e : T \text{ where } \{P\}
}{
    \Gamma \vdash e : T
} \quad \text{(T-Refine-Elim)}$$

**Subtyping Rules**

A refinement type is a subtype of its base type:

$$\Gamma \vdash (T \text{ where } \{P\}) <: T \quad \text{(Sub-Refine-Base)}$$

A refinement type with a stronger predicate is a subtype of one with a weaker predicate when both share the same base type:

$$\frac{
    \Gamma \vdash P \implies Q
}{
    \Gamma \vdash (T \text{ where } \{P\}) <: (T \text{ where } \{Q\})
} \quad \text{(Sub-Refine)}$$

**Type Equivalence**

Two refinement types with the same base type are equivalent if and only if their predicates are logically equivalent:

$$\Gamma \vdash (T \text{ where } \{P\}) \equiv (T \text{ where } \{Q\}) \iff \Gamma \vdash P \iff Q$$

**Decidable Predicate Subset**

Proving implication ($P \implies Q$) and equivalence ($P \iff Q$) is undecidable in the general case. Implementations MUST support the following decidable subset of predicates:

1. **Literal comparisons**: Comparisons of runtime values against compile-time constants (e.g., `self > 0`, `len != 0`)
2. **Bound propagation**: Predicates implied by control flow (e.g., after `if x > 0 { ... }`, the predicate `x > 0` holds)
3. **Syntactic equality**: Predicates that are syntactically identical (up to alpha-renaming)
4. **Transitive inequalities**: Linear arithmetic over integer types (e.g., `a < b` and `b < c` implies `a < c`)
5. **Boolean combinations**: Conjunction, disjunction, and negation of decidable predicates

For predicates outside this subset, implementations MAY:
- Reject the program with `E-TYP-1953` if static verification is required
- Insert runtime checks if within a `[[dynamic]]` scope (§7.2.9)
- Use IDB-specified decision procedures (e.g., SMT solvers) to attempt proof

**Nested Refinements**

Nested refinement types flatten to a single refinement with conjoined predicates:

$$(T \text{ where } \{P\}) \text{ where } \{Q\} \equiv T \text{ where } \{P \land Q\}$$

**Predicate Scope and Purity**

The predicate `P` in a refinement type `T where { P }` MUST satisfy:

1. The predicate MUST be a pure expression (§11.1.1).
2. The predicate MUST evaluate to type `bool`.
3. In type aliases and standalone refinement type expressions, the predicate MUST use `self` to refer to the constrained value.
4. In inline parameter constraints, the predicate MUST use the parameter name to refer to the constrained value; use of `self` is forbidden.

The predicate MAY reference other in-scope bindings, including earlier parameters in procedure signatures.

**Automatic Coercion**

The implementation MUST automatically coerce a value of type `T` to type `T where { P }` when active Verification Facts (§11.5) prove that `P` holds for that value. This coercion is implicit and requires no runtime check.

**Variance Interaction**

Refinement types participate in the standard subtyping relation defined in §4.2. Because `T where { P }` is a subtype of `T`, refinement types interact with generic type constructors according to the variance rules defined in §4.3. For a type constructor `C<T>` with covariant parameter `T`, if `S where { P } <: S` then `C<S where { P }> <: C<S>` follows from transitivity.

##### Dynamic Semantics

**Verification**

When a value is assigned to a binding of refinement type, or passed to a parameter of refinement type, and no Verification Fact (§11.5) establishes that the predicate holds, the predicate MUST be verified.

**Static Verification by Default**

Refinement predicates require static proof by default:

1. The implementation attempts static verification of the predicate
2. If static verification succeeds, no runtime code is generated
3. If static verification fails and the expression is not within a `[[dynamic]]` scope (§7.2.9), the program is ill-formed (`E-TYP-1953`)
4. If static verification fails and the expression is within a `[[dynamic]]` scope (§7.2.9), a runtime check is generated

##### Memory & Layout

**Representation**

A refinement type `T where { P }` has identical size, alignment, and layout to its base type `T`. The predicate is a compile-time and runtime constraint only; it does not affect physical representation.

$$\text{sizeof}(T \text{ where } \{P\}) = \text{sizeof}(T)$$

$$\text{alignof}(T \text{ where } \{P\}) = \text{alignof}(T)$$

The memory representation of a value of type `T where { P }` is indistinguishable from a value of type `T`.

##### Constraints & Legality

**Negative Constraints**

The following constraints apply to refinement types:

1. A refinement predicate MUST be a pure expression. Impure expressions (those performing I/O, allocation, mutation, or capability invocation) are forbidden.

2. A refinement predicate MUST evaluate to type `bool`. Predicates of other types are forbidden.

3. In inline parameter constraints, the `self` keyword MUST NOT appear in the predicate; the parameter name MUST be used instead.

4. Refinement predicates MUST NOT create circular type dependencies. A predicate that references a type alias containing the refinement being defined is ill-formed.

5. If static verification fails and the expression is not within a `[[dynamic]]` scope (§7.2.9), the program is ill-formed (`E-TYP-1953`).

**Diagnostic Table**

| Code         | Severity | Condition                                                                 | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------------------------------------ | :----------- | :-------- |
| `E-TYP-1950` | Error    | `self` used in inline parameter constraint.                               | Compile-time | Rejection |
| `E-TYP-1951` | Error    | Refinement predicate is not of type `bool`.                               | Compile-time | Rejection |
| `E-TYP-1952` | Error    | Circular type dependency in refinement predicate.                         | Compile-time | Rejection |
| `E-TYP-1953` | Error    | Refinement predicate not statically provable outside `[[dynamic]]` scope. | Compile-time | Rejection |
| `E-SEM-2802` | Error    | Impure expression in refinement predicate.                                | Compile-time | Rejection |
| `P-TYP-1953` | Panic    | Runtime refinement validation failed.                                     | Runtime      | Panic     |

---

# Part 3: Program Execution

## Clause 8: Modules and Name Resolution

### 8.1 Module System Architecture

##### Definition

**Project**

A **Project** is the top-level organizational unit, consisting of a collection of source files and a single manifest file (`Cursive.toml`) at its root. A project defines one or more assemblies.

**Assembly**

An **Assembly** is a collection of modules that are compiled and distributed as a single unit. An assembly **MAY** be either a `library` or an `executable`. Each assembly is defined within the project manifest.

**Module**

A **Module** is the fundamental unit of code organization and encapsulation. A module's contents are defined by the `.cursive` source files within a single directory, and its namespace is identified by a unique, filesystem-derived **module path**.

**Compilation Unit**

A **Compilation Unit** is the set of all source files constituting a single module. Each compilation unit defines exactly one module. An empty module (containing only whitespace and comments) is valid.

**Folder-as-Module Principle**

Each directory within a declared source root that contains one or more `.cursive` files **MUST** be treated as a single module. All `.cursive` files located directly within that directory contribute their top-level declarations to that single module's namespace.

**Multi-File Processing Order**

When a module comprises multiple `.cursive` files, the order in which files are processed is IDB.

Regardless of file processing order:
1. All files in the directory contribute to a single unified namespace
2. Identifier uniqueness constraints apply across all files
3. The resulting module semantics **MUST** be independent of file processing order for well-formed programs

The IDB file processing order affects only compilation artifacts (error message ordering, intermediate file timestamps, symbol table construction order) but **MUST NOT** affect the observable semantics of the compiled program. Any program whose semantics would change based on file processing order is ill-formed.

##### Syntax & Declaration

**Top-Level Item Grammar**

Only the following declaration kinds are permitted at module scope:

```ebnf
top_level_item ::= import_decl
                 | use_decl
                 | static_decl
                 | procedure_decl
                 | type_decl
                 | class_declaration

static_decl    ::= visibility? ("let" | "var") binding
type_decl      ::= visibility? ("record" | "enum" | "modal" | "type") identifier ...
procedure_decl ::= visibility? "procedure" identifier ...
```

The `class_declaration` production is defined in §10.2 (Class Declarations). The semantics of `let` and `var` bindings in `static_decl` (mutability and movability) are defined in §3.4 (The Binding Model).

**Module Path Grammar**

```ebnf
module_path    ::= identifier ("::" identifier)*
```

##### Static Semantics

**Module Path Derivation**

The module path for a given module **MUST** be derived from its directory path relative to the assembly's source root directory. Directory separators in the path **MUST** be replaced by the scope resolution operator `::`.

**Formal Rule (WF-Module-Path-Derivation):**

Let $P$ be the project context containing assembly definitions.

$$
\frac{P \vdash \text{assembly} \Rightarrow (\text{root\_path}, \_) \quad \text{dir\_path} = \text{absolute}(\text{module\_dir}) \quad \text{rel\_path} = \text{relative}(\text{dir\_path}, \text{root\_path})}{P \vdash \text{dir\_path} \Rightarrow \text{replace}(\text{rel\_path}, \text{PATH\_SEPARATOR}, "::")}
$$

**Module Well-Formedness**

A module is well-formed if:
1. All top-level declarations are valid according to the grammar
2. Each identifier is unique within the module's namespace
3. No control-flow constructs (`if`, `loop`, `match`) appear at top level
4. No expression statements appear at top level

##### Constraints & Legality

**Visibility Default**: Top-level items without an explicit visibility modifier default to `internal`.

**Identifier Uniqueness**: Each identifier **MUST** be unique within module scope. Because Cursive uses a unified namespace (§8.4), a type declaration and a term declaration **MUST NOT** share the same identifier.

**Forbidden Constructs**: Control-flow constructs (`if`, `loop`, `match`) and expression statements **MUST NOT** appear at top level.

| Code         | Severity | Condition                                        |
| :----------- | :------- | :----------------------------------------------- |
| `E-MOD-1106` | Error    | Module path component is not a valid identifier. |
| `E-MOD-1302` | Error    | Duplicate declaration in module scope.           |
| `E-SRC-0501` | Error    | Control-flow construct at module scope.          |

---

### 8.2 Project Manifest

##### Definition

Every project **MUST** be defined by a UTF-8 encoded manifest file named `Cursive.toml` at its root directory. The manifest **MUST** follow the TOML 1.0 syntax and be well-formed according to the rules in this section.

The manifest defines:
- Project metadata (`name`, `version`)
- Language version compatibility
- Source path mappings
- Assembly definitions

##### Syntax & Declaration

**Required Tables**

The manifest **MUST** contain the following tables:

| Table          | Purpose                                    |
| :------------- | :----------------------------------------- |
| `[project]`    | Project name and version                   |
| `[language]`   | Required Cursive language version          |
| `[paths]`      | Symbolic names mapping to source roots     |
| `[[assembly]]` | Assembly definitions (one or more entries) |

**`[project]` Table Schema**

```toml
[project]
name = "<identifier>"      # REQUIRED: Project identifier
version = "<semver>"       # REQUIRED: Project version (SemVer format)
```

**`[language]` Table Schema**

```toml
[language]
version = "<semver>"       # REQUIRED: Minimum language version
```

**`[paths]` Table Schema**

```toml
[paths]
<symbolic_name> = "<relative_path>"  # One or more entries
```

**`[[assembly]]` Table Schema**

```toml
[[assembly]]
name = "<identifier>"      # REQUIRED: Assembly name (unique)
root = "<path_key>"        # REQUIRED: Key from [paths] table
path = "<relative_path>"   # REQUIRED: Path relative to root
type = "library" | "executable"  # OPTIONAL: defaults to "library"
```

##### Static Semantics

**Manifest Well-Formedness Judgment**

The judgment $\vdash M : WF$ holds if the manifest $M$ is well-formed:

$$
\frac{
    M \vdash \text{project} : WF \quad
    M \vdash \text{language} : WF \quad
    M \vdash \text{paths} : WF \quad
    \forall a \in M.\text{assemblies}, M \vdash a : WF
}{
    \vdash M : WF
}
\tag{WF-Manifest}
$$

**`[project]` Table Well-Formedness (WF-Manifest-Project):**

$$
\frac{
    \text{name} \in M.\text{project} \quad \text{version} \in M.\text{project} \quad \vdash \text{name} : \text{Identifier} \quad \vdash \text{version} : \text{SemVer}
}{
    M \vdash \text{project} : WF
}
$$

**`[language]` Table Well-Formedness (WF-Manifest-Language):**

$$
\frac{
    \text{version} \in M.\text{language} \quad \vdash \text{version} : \text{SemVer} \quad \text{is\_compatible}(\text{version}, \text{implementation\_version})
}{
    M \vdash \text{language} : WF
}
$$

**`[paths]` Table Well-Formedness (WF-Manifest-Paths):**

$$
\frac{
    |M.\text{paths}| \ge 1 \quad \forall (k, v) \in M.\text{paths}, \text{is\_valid\_path}(v)
}{
    M \vdash \text{paths} : WF
}
$$

**`[[assembly]]` Well-Formedness (WF-Manifest-Assembly):**

$$
\frac{
    a.\text{name} \in \text{identifiers} \quad a.\text{root} \in \text{dom}(M.\text{paths}) \quad \text{is\_valid\_path}(a.\text{path})
}{
    M \vdash a : WF
}
$$

##### Constraints & Legality

| Code         | Severity | Condition                                                           |
| :----------- | :------- | :------------------------------------------------------------------ |
| `E-MOD-1101` | Error    | Manifest file `Cursive.toml` not found or syntactically malformed.  |
| `E-MOD-1102` | Error    | `[paths]` table is missing, empty, or contains invalid path values. |
| `E-MOD-1103` | Error    | Assembly references a `root` not defined in `[paths]` table.        |
| `E-MOD-1107` | Error    | `[project]` table or required keys (`name`, `version`) are missing. |
| `E-MOD-1108` | Error    | Duplicate assembly name in manifest.                                |
| `E-MOD-1109` | Error    | `[language]` version missing or incompatible.                       |

---

### 8.3 Module Discovery and Paths

##### Definition

**Module Path**

A **Module Path** is a sequence of identifiers separated by `::` that uniquely identifies a module within an assembly. Module paths are derived from filesystem structure per the algorithm defined in §8.1 (WF-Module-Path-Derivation).

##### Static Semantics

**Path Well-Formedness Judgment**

The judgment $\vdash p : WF_{path}$ holds if a path $p$ is well-formed:

$$
\frac{
    \forall c \in \text{components}(p), (\vdash c : \text{Identifier} \land \vdash c : \text{NotKeyword})
}{
    \vdash p : WF_{path}
}
\tag{WF-Module-Path}
$$

A module path is well-formed if and only if every component is a valid Cursive identifier (§2.5) and is not a reserved keyword (§2.6).

**Case-Sensitivity Collision Detection**

On filesystems that are not case-sensitive, two file or directory names that differ only in case **MUST** be treated as ambiguous if they would resolve to the same module path component.

Let $N(p)$ be the NFC-normalized, case-folded version of a path component $p$:

$$
\frac{
    \exists p_1, p_2 \in \text{project\_paths} \quad p_1 \neq p_2 \quad N(p_1) = N(p_2)
}{
    \text{Collision Error}
}
\tag{WF-Module-Path-Collision}
$$

##### Constraints & Legality

| Code         | Severity | Condition                                                    |
| :----------- | :------- | :----------------------------------------------------------- |
| `E-MOD-1104` | Error    | Module path collision on case-insensitive filesystem.        |
| `E-MOD-1105` | Error    | Module path component is a reserved keyword.                 |
| `E-MOD-1106` | Error    | Module path component is not a valid identifier.             |
| `W-MOD-1101` | Warning  | Potential module path collision on case-insensitive systems. |

---

### 8.4 Scope Context Structure

##### Definition

**Scope**

A **Scope** is a region of source text where a set of names is valid. Scopes are lexically nested.

**Scope Context ($\Gamma$)**

The **Scope Context** is the abstract data structure representing the mapping from identifiers to entities during name resolution. It is an ordered list of scope mappings from innermost to outermost:

$$\Gamma ::= [S_{local}, S_{proc}, S_{module}, S_{universe}]$$

Where each $S$ is a scope that maps identifiers to entities:

$$S : \text{Identifier} \to \text{Entity}$$

**Scope Hierarchy**

| Scope Level    | Contents                                                            |
| :------------- | :------------------------------------------------------------------ |
| $S_{local}$    | Block-local bindings (`let`, `var`, loop variables, match bindings) |
| $S_{proc}$     | Procedure parameters and local type parameters                      |
| $S_{module}$   | Module-level declarations and imported/used names                   |
| $S_{universe}$ | Primitive types and the `cursive.*` namespace                       |

**Universe Scope**

The **Universe Scope** ($S_{universe}$) is the implicit outermost scope that terminates all name resolution chains. It contains:
- All primitive types (§5.1)
- Core literals (`true`, `false`)
- The `cursive.*` namespace for standard library access

**Unified Namespace**

Cursive **MUST** be implemented with a single, **unified namespace** per scope. An identifier's meaning is determined only by its spelling and the scope in which it is defined, not by the syntactic context of its use.

This single namespace **MUST** be shared by all declaration kinds:
1. **Terms**: Bindings for variables, constants, and procedures
2. **Types**: Bindings for type declarations (`record`, `enum`, `modal`, `type`, `form`)
3. **Modules**: Bindings for module import aliases

##### Static Semantics

**Scope Nesting**

Scopes form a strict hierarchy. An inner scope $S_i$ is **nested within** an outer scope $S_o$ if $S_i$ appears before $S_o$ in the context list. The innermost scope is always searched first during name lookup.

**Context Extension**

The context $\Gamma$ is extended by adding a binding to the innermost scope:

$$\Gamma, x : T \equiv [S_{local} \cup \{x \mapsto T\}, S_{proc}, S_{module}, S_{universe}]$$

**Name Binding Model**

Each scope $S$ maintains a mapping from identifiers to **entities**. An entity is one of:
- A term (variable, constant, procedure)
- A type (record, enum, modal, type alias)
- A form
- A module reference

##### Constraints & Legality

Because Cursive uses a unified namespace, declaring a type and a term with the same identifier in the same scope **MUST** be rejected:

| Code         | Severity | Condition                                                  |
| :----------- | :------- | :--------------------------------------------------------- |
| `E-MOD-1302` | Error    | Duplicate name: identifier already declared in this scope. |

---

### 8.5 Visibility and Access Control

##### Definition

**Visibility**

Every top-level declaration has a **visibility level** that controls its accessibility from other modules.

**Visibility Levels**

| Modifier    | Scope of Accessibility                                              |
| :---------- | :------------------------------------------------------------------ |
| `public`    | Visible to any module in any assembly that depends on it            |
| `internal`  | (Default) Visible only to modules within the **same assembly**      |
| `private`   | Visible only within the **defining module** (same directory)        |
| `protected` | Visible only within the **defining type** and class implementations |

**Accessibility**

An item is **accessible** from a given context if the visibility rules permit the reference.

##### Syntax & Declaration

```ebnf
visibility ::= "public" | "internal" | "private" | "protected"
```

Visibility modifiers **MUST** appear as a prefix on declarations:

```cursive
public procedure foo() { ... }
internal record Bar { ... }
private let secret: i32 = 42
```

##### Static Semantics

**Access Judgment**

Let the context $\Gamma$ contain the accessor module $m_{acc}$ and its assembly $a_{acc}$. Let the target item $i$ be defined in module $m_{def}$ within assembly $a_{def}$. The judgment $\Gamma \vdash \text{can\_access}(i)$ holds if access is permitted.

**(WF-Access-Public):**
$$
\frac{\text{visibility}(i) = \text{public}}{\Gamma \vdash \text{can\_access}(i)}
$$

**(WF-Access-Internal):**
$$
\frac{\text{visibility}(i) = \text{internal} \quad a_{acc} = a_{def}}{\Gamma \vdash \text{can\_access}(i)}
$$

**(WF-Access-Private):**
$$
\frac{\text{visibility}(i) = \text{private} \quad m_{acc} = m_{def}}{\Gamma \vdash \text{can\_access}(i)}
$$

**(WF-Access-Protected-Self):**
$$
\frac{\text{visibility}(item) = \text{protected} \quad \Gamma \subseteq T_{def}}{\Gamma \vdash \text{can\_access}(item)}
$$

**(WF-Access-Protected-Class):**
$$
\frac{\text{visibility}(item) = \text{protected} \quad \Gamma \subseteq \text{class } Cl \text{ for } T_{def} \quad A(\Gamma) = A(T_{def})}{\Gamma \vdash \text{can\_access}(item)}
$$

##### Constraints & Legality

**Protected Restriction**: The `protected` modifier **MUST NOT** be used on top-level (module-scope) declarations. It **MUST** only be applied to members (fields or procedures) within a `record`, `enum`, or `modal` declaration.

**Intra-Assembly Access**: Modules within the same assembly are automatically available for qualified name access without requiring an `import` declaration. A declaration `item` in module `mod` within assembly `A` is accessible from another module in assembly `A` via the qualified path `mod::item` if and only if the visibility of `item` is `public` or `internal`.

**Diagnostic Table**

| Code         | Severity | Condition                                             | Detection    | Effect    |
| :----------- | :------- | :---------------------------------------------------- | :----------- | :-------- |
| `E-MOD-1305` | Error    | Inaccessible item: visibility does not permit access. | Compile-time | Rejection |
| `E-MOD-2440` | Error    | `protected` used on top-level declaration.            | Compile-time | Rejection |
| `E-MOD-1207` | Error    | Cannot access a `protected` item from this scope.     | Compile-time | Rejection |

---

### 8.6 Import and Use Declarations

##### Definition

**Import Declaration**

An `import` declaration declares a dependency on a module from an **external assembly**, making that module's public items available for qualified access. It is the primary mechanism for building the inter-assembly dependency graph.

**Use Declaration**

A `use` declaration creates a **scope alias** that brings items into the current scope, enabling unqualified access to otherwise qualified names.

**Re-export**

A `public use` declaration **re-exports** an imported item, making it available to modules that depend on the current assembly.

##### Syntax & Declaration

**Grammar:**

```ebnf
import_decl ::= "import" module_path ("as" identifier)?

use_decl    ::= visibility? "use" use_clause
use_clause  ::= use_path ("as" identifier)?
              | use_path "::" use_list
              | use_path "::" "*"

use_path    ::= module_path ("::" identifier)?
use_list    ::= "{" use_specifier ("," use_specifier)* ","? "}"
use_specifier ::= identifier ("as" identifier)?
                | "self" ("as" identifier)?
```

```cursive
// Import external assembly module
import core::io

// Import with alias
import core::collections as col

// Use single item
use core::io::File

// Use with alias
use core::io::File as FileHandle

// Use multiple items (list form)
use core::collections::{Vec, HashMap}

// Use with self (brings module AND items into scope)
use core::io::{self, File, Reader}
// After this: `io::write()` and `File` both work

// Use with aliased self
use core::collections::{self as col, Vec}

// Wildcard use (brings all accessible items into scope)
use core::prelude::*

// Re-export
public use core::io::File
```

##### Static Semantics

**Import Semantics**

An `import` declaration:

1. Establishes a dependency edge from the current assembly to the imported assembly
2. Makes the imported module's namespace available for qualified access
3. Does **NOT** introduce any names into the current scope's unqualified namespace
4. If `as alias` is specified, the alias becomes the local name for the module; otherwise, the last component of the module path is used

**Import Requirement for External Access**

For modules in **external assemblies**, an `import` declaration **MUST** precede any `use` declaration that references items from that assembly. The `import` declaration establishes the inter-assembly dependency; the `use` declaration then brings specific items into scope.

For modules in the **same assembly** (intra-assembly access), `use` declarations **MAY** directly reference module paths without a preceding `import`.

**Use Semantics**

A `use` declaration resolves a path and introduces bindings into the current scope:

1. **Item Use** (`use path::Item`): Resolves `Item` and introduces a binding with its original name (or the alias if `as alias` is specified).

2. **List Use** (`use path::{A, B, C}`): Resolves each item in the list and introduces bindings for all of them. Each item in the list **MUST** be unique.

3. **Self Use** (`use path::{self, A}`): The special specifier `self` in a use list introduces a binding for the module path itself, enabling qualified access via the module name alongside unqualified access to other items.

4. **Wildcard Use** (`use path::*`): Brings all accessible (public or internal, depending on assembly relationship) items from the target module into the current scope. Each item retains its original name.

**Formal Rule (WF-Use-Item):**

$$
\frac{
    \Gamma \vdash \text{resolve\_item}(\text{path}::i) \Rightarrow \text{item} \quad \Gamma \vdash \text{can\_access}(\text{item}) \quad \text{name} \notin \text{dom}(\Gamma)
}{
    \Gamma \vdash \text{use path}::i \text{ as name;}: WF
}
$$

**Formal Rule (WF-Use-Wildcard):**

$$
\frac{
    \Gamma \vdash \text{resolve\_module}(\text{path}) \Rightarrow m \quad \forall i \in \text{accessible}(m), \text{name}(i) \notin \text{dom}(\Gamma)
}{
    \Gamma \vdash \text{use path}::*: WF
}
$$

**Re-export Semantics**

A `public use` declaration re-exports an item, making it part of the current module's public API:

1. Performs the same resolution as a regular `use`
2. Additionally marks the introduced binding as `public`, making it visible to external dependents
3. The source item **MUST** itself be `public`; re-exporting `internal` or `private` items is forbidden

**Formal Rule (WF-Use-Public):**

$$
\frac{
    \Gamma \vdash \text{use path}::i \text{ as name;}: WF \quad \text{visibility}(\text{item}) = \text{public}
}{
    \Gamma \vdash \text{public use path}::i \text{ as name;}: WF
}
$$

##### Constraints & Legality

**Import Cycle Detection**: Cyclic import dependencies between assemblies are ill-formed.

**Import Requirement**: For external assemblies, an `import` declaration **MUST** precede any `use` that references that assembly. Attempting to `use` an item from an external assembly without a prior `import` **MUST** be diagnosed as an error.

**Visibility Enforcement**: Re-exported items **MUST NOT** expose items with more restrictive visibility than the re-export itself. Re-exporting a `private` or `internal` item via `public use` is forbidden.

**Uniqueness in Use Lists**: Each identifier in a use list **MUST** be unique. Duplicate items in a single `use` declaration **MUST** be diagnosed as an error.

**Name Conflict Prevention**: Names introduced by `import as` or `use` declarations **MUST NOT** conflict with existing names in the same scope.

**Wildcard Use Warning**: Wildcard use declarations (`use path::*`) in modules that expose a `public` API **SHOULD** trigger a warning. This warning is informational and does not affect program validity.

**Diagnostic Table**

| Code         | Severity | Condition                                                        | Detection    | Effect    |
| :----------- | :------- | :--------------------------------------------------------------- | :----------- | :-------- |
| `E-MOD-1201` | Error    | Circular import dependency between assemblies.                   | Compile-time | Rejection |
| `E-MOD-1202` | Error    | Import of non-existent assembly or module.                       | Compile-time | Rejection |
| `E-MOD-1203` | Error    | Name introduced by `use` or `import as` conflicts with existing. | Compile-time | Rejection |
| `E-MOD-1204` | Error    | Item in `use` path not found or not accessible.                  | Compile-time | Rejection |
| `E-MOD-1205` | Error    | Attempt to `public use` a non-public item.                       | Compile-time | Rejection |
| `E-MOD-1206` | Error    | Duplicate item in a `use` list.                                  | Compile-time | Rejection |
| `W-MOD-1201` | Warning  | Wildcard `use` (`*`) in a module exposing public API.            | Compile-time | N/A       |

See §8.5 for visibility violation diagnostics (`E-MOD-1305`).

---

### 8.7 Name Resolution

##### Definition

**Name Introduction**

**Name Introduction** is the process of adding a binding to a scope's namespace. A declaration introduces an identifier into the current scope.

**Shadowing**

**Shadowing** occurs when a declaration in an inner scope introduces a binding that obscures a binding of the same name in an outer scope. Cursive requires explicit acknowledgment of shadowing.

**Name Lookup (Resolution)**

**Name Lookup** is the process of finding the entity to which an identifier refers.

##### Static Semantics

**Name Introduction Rules**

A declaration that introduces an identifier into the current scope MUST be well-formed only if that identifier is not already bound in the same scope.

**Explicit Shadowing Rules**

A declaration in an inner scope MAY shadow a name from an outer scope, but:
- Explicit shadowing with the `shadow` keyword is well-formed when the name exists in an outer scope but not in the current scope.
- Using `shadow` when no binding is being shadowed is ill-formed.

**Conformance Mode Behavior:**
- **Strict Mode**: Implicit shadowing (without `shadow` keyword) MUST trigger error `E-MOD-1303`
- **Permissive Mode**: Implicit shadowing MUST trigger warning `W-MOD-1303`

**Unqualified Name Lookup**

Unqualified lookup searches the scope context from innermost to outermost. If the identifier is found in the innermost scope, that binding is returned. Otherwise, lookup proceeds to the next outer scope. If the context is exhausted and the name is not found, the lookup fails.

**Qualified Name Lookup**

Qualified lookup resolves a path of identifiers, such as `A::B::C`. To resolve a path `p::i`:
1. The prefix `p` must first resolve to a module `m`
2. The identifier `i` must resolve to a member of `m`
3. That member must be accessible from the current scope (§8.5)

##### Constraints & Legality

| Code         | Severity | Condition                                                      |
| :----------- | :------- | :------------------------------------------------------------- |
| `E-MOD-1301` | Error    | Unresolved name: identifier not found in any accessible scope. |
| `E-MOD-1302` | Error    | Duplicate name: identifier already declared in this scope.     |
| `E-MOD-1303` | Error    | Shadowing without `shadow` keyword (Strict Mode).              |
| `W-MOD-1303` | Warning  | Shadowing without `shadow` keyword (Permissive Mode).          |
| `E-MOD-1304` | Error    | Unresolved module: path prefix did not resolve to a module.    |
| `E-MOD-1306` | Error    | Unnecessary `shadow` keyword: no binding is being shadowed.    |

See §8.5 for visibility violation diagnostics (`E-MOD-1305`).

---

### 8.8 Module Initialization

##### Definition

**Module Dependency Graph (MDG)**

The **Module Dependency Graph** is a directed graph $G = (V, E)$ where:
- $V$ is the set of all modules in the program and its compiled dependencies
- $E$ is a set of directed edges $(A, B)$, where an edge from module $A$ to module $B$ signifies that $A$ depends on $B$

**Edge Classification**

Each edge in $E$ is classified as either **Type-Level** or **Value-Level**:

| Edge Type   | Definition                                                                    |
| :---------- | :---------------------------------------------------------------------------- |
| Value-Level | Module $A$ contains an expression that evaluates a binding defined in $B$     |
| Type-Level  | Module $A$ refers to a type, class, or constant signature defined in $B$ only |

**Eager vs Lazy Dependencies**

| Dependency Type | Definition                                                                      |
| :-------------- | :------------------------------------------------------------------------------ |
| **Eager**       | A Value-Level edge originating from a **module-level initializer**              |
| **Lazy**        | A Type-Level edge, OR a Value-Level edge occurring only within procedure bodies |

A dependency is **eager** if its resolution is required during module initialization. This includes:
- Constant evaluation (`comptime` expressions) referencing external bindings
- Module-level `let` or `var` initializers that invoke procedures or access values from other modules
- Type definitions that require evaluating constant expressions from other modules

A dependency is **lazy** if it is only required during procedure execution. Lazy dependencies include:
- Type-only references (using a type from another module without evaluating its values)
- Value-level references that occur exclusively within procedure bodies (not at module scope)

**Initialization Stages**

Module initialization **MUST** be divided into two stages:

1. **Static Initialization**: Initializers that are compile-time constants (literals, `comptime` values) **MUST** be evaluated at compile time and stored in the program's data section.

2. **Dynamic Initialization**: Initializers requiring runtime execution (procedure calls, values requiring capabilities) **MUST** be executed after program startup but before the `main` procedure body executes.

##### Static Semantics

**Edge Classification Judgment (WF-Dep-Value):**

$$
\frac{
    \exists e \in \text{exprs}(A), \text{refers\_to\_value}(e, B)
}{
    (A, B) \in E, \text{class}((A, B)) = \text{Value-Level}
}
$$

**Acyclic Eager Subgraph Requirement**

The initialization order is derived *only* from the subgraph of eager dependencies. The subgraph $G_e = (V, E_e)$, containing all modules $V$ and only the set of eager edges $E_e$, **MUST** be a Directed Acyclic Graph (DAG).

**(WF-Acyclic-Eager-Deps):**

$$
\frac{
    \forall v \in V, \neg \text{is\_reachable}(v, v, E_e)
}{
    \vdash G_e : \text{DAG}
}
$$

Where $\text{is\_reachable}(u, v, E_e)$ is true if there is a path of one or more eager edges from $u$ to $v$.

##### Dynamic Semantics

**Execution Order**

Before the program's `main` procedure is invoked, the runtime **MUST**:

1. Compute a valid topological sort of modules based on the eager dependency graph $G_e$
2. For each module in the sorted list, execute the initializers for all module-level bindings in the order they appear within that module's source files

**Intra-Module Order**: Within a module, initializers **MUST** execute in strictly sequential lexical order within each source file. When a module comprises multiple source files, the order in which files are processed for initialization is **Implementation-Defined Behavior (IDB)** consistent with the file processing order documented per §8.1. Within each file, lexical order is preserved.

**Failure Semantics**

If the evaluation of any module-level initializer terminates with a panic:
1. The initialization of that module is considered **failed**
2. The program state is considered **poisoned**
3. Any subsequent attempt to access a binding from that module, or from any module with an eager dependency path to it, **MUST** also result in a panic

##### Constraints & Legality

| Code         | Severity | Condition                                                |
| :----------- | :------- | :------------------------------------------------------- |
| `E-MOD-1401` | Error    | Cyclic module dependency detected in eager initializers. |

---

### 8.9 Program Entry Point

##### Definition

**Entry Point**

The **Entry Point** is the `main` procedure that receives the root capabilities and begins program execution after all module initialization is complete.

**No Ambient Authority Principle**

Cursive enforces the principle of **No Ambient Authority**: all procedures that produce observable external effects (I/O, networking, threading, heap allocation) **MUST** receive the required capability explicitly. There are no global functions for these operations.

The `Context` record is defined in Appendix E.

##### Syntax & Declaration

**Required Signature:**

```cursive
public procedure main(ctx: Context) -> i32
```

The `main` procedure:
- **MUST** be declared with `public` visibility
- **MUST** accept exactly one parameter of type `Context`
- **MUST** return type `i32`

##### Static Semantics

**Main Procedure Well-Formedness:**

1. Exactly one `main` procedure **MUST** exist in the entry assembly
2. The signature **MUST** match `main(ctx: Context) -> i32`
3. `main` **MUST NOT** be generic (no type parameters)

**Return Semantics:**

- Return value `0` indicates successful termination
- Non-zero return values indicate failure (specific codes are application-defined)

##### Constraints & Legality

**Global State Definition**

**Global state** comprises any mutable storage with module lifetime. In Cursive, this includes:
- Module-level `var` bindings
- Any other mutable storage accessible at module scope without requiring a capability parameter

**Global Mutable State Prohibition**: Module-level `var` bindings **MUST NOT** have `public` visibility. Global mutable state accessible across module boundaries is forbidden to ensure capability-based security.

| Code         | Severity | Condition                                |
| :----------- | :------- | :--------------------------------------- |
| `E-MOD-2430` | Error    | Multiple `main` procedures defined.      |
| `E-MOD-2431` | Error    | Invalid `main` signature.                |
| `E-MOD-2432` | Error    | `main` is generic (has type parameters). |
| `E-MOD-2433` | Error    | Public mutable global state.             |

---

## Clause 9: Procedures and Methods

### 9.1 Procedure Declarations

##### Definition

A **procedure** is a named, callable unit of computation that accepts zero or more parameters, performs computation, and optionally returns a value. Procedures are the primary abstraction for encapsulating behavior in Cursive.

**Formal Definition**

A procedure $P$ is defined as a tuple:

$$P = (N, V, G, \Sigma, C, B)$$

Where:
- $N$: The procedure name (identifier)
- $V$: Visibility level (public, internal, protected, private)
- $G$: Generic parameter list (possibly empty)
- $\Sigma$: Signature (parameter types and return type)
- $C$: Contract clauses (preconditions, postconditions)
- $B$: Body (statement block)

##### Syntax & Declaration

**Grammar**

```ebnf
procedure_decl ::= [visibility] "procedure" identifier [generic_params]
                   "(" [param_list] ")" ["->" return_type]
                   [contract_clauses] block

param_list     ::= param ("," param)* [","]

param          ::= [param_mode] identifier ":" type

param_mode     ::= "move" | permission_prefix

permission_prefix ::= "const" | "unique" | "shared"

return_type    ::= type | union_return

union_return   ::= type ("|" type)+

contract_clauses ::= (precondition | postcondition)+

precondition   ::= "|=" expression

postcondition  ::= "=>" expression
```

The `visibility`, `generic_params`, and `block` productions are defined in §8.5 (Visibility), §7.1 (Generics), and §12.7 (Block Expressions), respectively.

**Parameter Modes**

| Mode            | Syntax        | Semantics                                                          |
| :-------------- | :------------ | :----------------------------------------------------------------- |
| Default (const) | `x: T`        | Caller retains responsibility; callee receives `const` access      |
| Unique          | `unique x: T` | Caller retains responsibility; callee receives exclusive access    |
| Shared          | `shared x: T` | Caller retains responsibility; callee receives synchronized access |
| Move            | `move x: T`   | Caller transfers responsibility to callee                          |

**Return Type Syntax**

A procedure may return a single type or a union of types. Union returns are used for error handling:

```cursive
procedure divide(a: i32, b: i32) -> i32 | DivisionByZero {
    if b == 0 {
        result DivisionByZero {}
    }
    result a / b
}
```

##### Static Semantics

**Procedure Type Derivation (T-Proc)**

$$\frac{
  \Gamma \vdash p_1 : T_1, \ldots, p_n : T_n \quad
  \Gamma, p_1 : T_1, \ldots, p_n : T_n \vdash B : T_r
}{
  \Gamma \vdash \texttt{procedure } N(p_1: T_1, \ldots, p_n: T_n) \rightarrow T_r\ \{B\} : (T_1, \ldots, T_n) \rightarrow T_r
}$$

**Well-Formedness (WF-Proc)**

A procedure declaration is well-formed if:

1. The procedure name is a valid identifier not already bound in the current scope
2. All parameter names are distinct
3. All parameter types are well-formed
4. The return type (if specified) is well-formed
5. The body is well-typed with respect to the parameter context
6. If the return type is specified, all return paths produce values of compatible types
7. Contract expressions are pure and of type `bool`

##### Constraints & Legality

| Code         | Severity | Condition                                                   | Detection    | Effect    |
| :----------- | :------- | :---------------------------------------------------------- | :----------- | :-------- |
| `E-SEM-3001` | Error    | Duplicate parameter name in procedure signature.            | Compile-time | Rejection |
| `E-SEM-3002` | Error    | Return type mismatch in procedure body.                     | Compile-time | Rejection |
| `E-SEM-3003` | Error    | Non-exhaustive return paths (not all paths return a value). | Compile-time | Rejection |
| `E-SEM-3004` | Error    | Impure expression in contract clause.                       | Compile-time | Rejection |

---

### 9.2 Method Definitions and Receivers

##### Definition

A **method** is a procedure associated with a type. Methods are invoked on instances of their associated type through the receiver parameter.

**Receiver Shorthand**

Cursive provides shorthand notation for method receivers:

| Shorthand | Expansion           | Permission | Description                  |
| :-------- | :------------------ | :--------- | :--------------------------- |
| `~`       | `self: const Self`  | `const`    | Read-only access to receiver |
| `~!`      | `self: unique Self` | `unique`   | Exclusive mutable access     |
| `~%`      | `self: shared Self` | `shared`   | Synchronized shared access   |

##### Syntax & Declaration

**Grammar**

```ebnf
method_def     ::= [visibility] "procedure" identifier "(" receiver ["," param_list] ")"
                   ["->" return_type] [contract_clauses] block

receiver       ::= receiver_shorthand | explicit_receiver

receiver_shorthand ::= "~" | "~!" | "~%"

explicit_receiver  ::= [param_mode] "self" ":" type
```

**Method vs. Standalone Procedure**

The presence of a receiver parameter distinguishes methods from standalone procedures:

```cursive
// Standalone procedure (no receiver)
procedure compute(x: i32, y: i32) -> i32 {
    result x + y
}

// Method (has receiver)
record Counter { value: i32 }
// Within type block:
procedure increment(~!) {
    self.value += 1
}
```

##### Static Semantics

**Receiver Permission Compatibility**

A method with receiver permission $P_{\text{method}}$ is callable through a path with permission $P_{\text{caller}}$ if and only if $P_{\text{caller}} <: P_{\text{method}}$ in the permission subtyping lattice.

| Caller's Permission | `~` (const) | `~!` (unique) | `~%` (shared) |
| :------------------ | :---------: | :-----------: | :-----------: |
| `const`             |      ✓      |       ✗       |       ✗       |
| `unique`            |      ✓      |       ✓       |       ✓       |
| `shared`            |      ✓      |       ✗       |       ✓       |

**Reading the matrix:** Row = permission of the caller's path to the receiver. Column = permission required by the method's receiver parameter. ✓ = method is callable. ✗ = method is not callable.

**Formal Rule (Receiver-Compat):**

$$\frac{
    \Gamma \vdash e : P_{\text{caller}}\ T \quad
    m \in \text{Methods}(T) \quad
    m.\text{receiver} = P_{\text{method}}\ \texttt{Self} \quad
    \Gamma \vdash P_{\text{caller}}\ T <: P_{\text{method}}\ T
}{
    \Gamma \vdash e.m(\ldots)\ \text{well-typed}
} \quad \text{(Receiver-Compat)}$$

**Key Acquisition in Method Calls**

When a method with `~%` receiver is called through a `shared` path, the key is acquired for the duration of the method call:

```cursive
let counter: shared Counter = Counter { value: 0 }

// Key acquisition sequence:
counter.increment()
// 1. Acquire write key to `counter` (method has ~% receiver)
// 2. Execute increment method body
// 3. Release key
```

For explicit control over key scope, see the `#` coarsening annotation in §14.4.

##### Constraints & Legality

| Code         | Severity | Condition                                                         | Detection    | Effect    |
| :----------- | :------- | :---------------------------------------------------------------- | :----------- | :-------- |
| `E-SEM-3010` | Error    | Method receiver permission incompatible with caller's permission. | Compile-time | Rejection |
| `E-SEM-3011` | Error    | Method defined outside of type context.                           | Compile-time | Rejection |
| `E-SEM-3012` | Error    | Duplicate method name in type.                                    | Compile-time | Rejection |

---

### 9.3 Parameter Passing and Binding

##### Static Semantics

**Parameter Evaluation Order**

Arguments are evaluated left-to-right in declaration order:

$$\text{eval}(f(e_1, e_2, \ldots, e_n)) = \text{eval}(e_1); \text{eval}(e_2); \ldots; \text{eval}(e_n); \text{call}(f)$$

**Move Parameter Semantics**

When a parameter is declared with `move`, the caller MUST use the `move` keyword at the call site:

```cursive
procedure consume(move data: Buffer) {
    // data is now owned by this procedure
}

let buffer = Buffer::new()
consume(move buffer)  // Caller uses `move` keyword
// buffer is now INVALID (Moved state)
```

**Non-Move Parameter Semantics**

For parameters without `move`, the caller retains responsibility:

```cursive
procedure inspect(data: const Buffer) {
    // Read-only access to data
}

let buffer = Buffer::new()
inspect(buffer)  // No move required
// buffer is still valid
```

##### Dynamic Semantics

**Argument Binding Sequence**

1. Each argument expression is evaluated in left-to-right order
2. For `move` parameters: responsibility transfers from caller to callee
3. For non-move parameters: a permission-appropriate view is created
4. Parameter bindings are introduced into the procedure's scope
5. The procedure body executes with these bindings

##### Constraints & Legality

| Code         | Severity | Condition                                       | Detection    | Effect    |
| :----------- | :------- | :---------------------------------------------- | :----------- | :-------- |
| `E-SEM-3020` | Error    | Missing `move` keyword for move parameter.      | Compile-time | Rejection |
| `E-SEM-3021` | Error    | `move` keyword on non-move parameter.           | Compile-time | Rejection |
| `E-SEM-3022` | Error    | Argument type incompatible with parameter type. | Compile-time | Rejection |

---

### 9.4 Procedure Execution Model

##### Dynamic Semantics

**Invocation Sequence**

Procedure invocation proceeds through the following phases:

1. **Argument Evaluation**: Arguments are evaluated left-to-right
2. **Frame Construction**: A new stack frame is allocated
3. **Parameter Binding**: Arguments are bound to parameters in the new frame
4. **Precondition Check**: If contracts are enabled, preconditions are verified
5. **Body Execution**: The procedure body executes
6. **Postcondition Check**: If contracts are enabled, postconditions are verified
7. **Return**: The return value is produced and control returns to caller
8. **Cleanup**: Deferred actions and destructors execute in LIFO order

**Return Semantics**

The `return` statement exits the procedure and produces a value:

```cursive
procedure compute() -> i32 {
    if condition {
        return 42  // Early return
    }
    result expensive()  // Final result (tail position)
}
```

> **Cross-Reference:** See §12.10 (Control Flow Statements) for the distinction between `return` (exits procedure) and `result` (exits block).

**Cleanup Ordering**

When a procedure exits (normally or via `return`), cleanup proceeds in reverse declaration order:

1. Deferred actions (`defer`) execute in LIFO order
2. Destructor calls for bindings with responsibility execute in reverse declaration order
3. The stack frame is deallocated

> **Cross-Reference:** See §3.6 (Deterministic Destruction) and §12.12 (Defer Statements) for detailed cleanup semantics.

---

### 9.5 Overloading and Signature Matching

##### Definition

Cursive supports **procedure overloading**: multiple procedures may share the same name if their signatures are distinguishable.

**Overload Set**

An **overload set** is the collection of all procedures with the same name visible at a call site.

##### Static Semantics

**Overload Resolution**

Given a call $f(a_1, \ldots, a_n)$ where $f$ names an overload set $O$:

1. **Candidate Selection**: All procedures in $O$ with matching arity are candidates
2. **Type Filtering**: Candidates whose parameter types are not compatible with argument types are eliminated
3. **Best Match**: If exactly one candidate remains, it is selected
4. **Ambiguity**: If multiple candidates remain, the call is ambiguous (error)
5. **No Match**: If no candidates remain, the call is ill-formed (error)

**Disambiguation Rules**

When multiple overloads are candidates:

1. An exact type match is preferred over a subtype match
2. A non-generic overload is preferred over a generic overload
3. A more specific generic constraint is preferred over a less specific one

##### Constraints & Legality

| Code         | Severity | Condition                                                          | Detection    | Effect    |
| :----------- | :------- | :----------------------------------------------------------------- | :----------- | :-------- |
| `E-SEM-3030` | Error    | Ambiguous overload resolution.                                     | Compile-time | Rejection |
| `E-SEM-3031` | Error    | No matching overload found.                                        | Compile-time | Rejection |
| `E-SEM-3032` | Error    | Duplicate procedure signature (overloads must be distinguishable). | Compile-time | Rejection |

---

## Clause 10: Classes and Polymorphism

### 10.1 Introduction to Classes

##### Definition

A **class** is a named declaration that defines an abstract interface consisting of procedure signatures, associated type declarations, abstract fields, and abstract states. Classes establish structural and behavioral contracts that implementing types MUST satisfy.

**Formal Definition**

A class $Cl$ is defined as a tuple:

$$Cl = (N, G, S, P_{abs}, P_{con}, A_{abs}, A_{con}, F, St)$$

where:

- $N$ is the class name (an identifier)
- $G$ is the (possibly empty) set of generic type parameters
- $S$ is the (possibly empty) set of superclass bounds
- $P_{abs}$ is the set of abstract procedure signatures
- $P_{con}$ is the set of concrete procedure definitions (default implementations)
- $A_{abs}$ is the set of abstract associated type declarations
- $A_{con}$ is the set of concrete associated type bindings (defaults)
- $F$ is the (possibly empty) set of abstract field declarations
- $St$ is the (possibly empty) set of abstract state declarations

A class with $St \neq \emptyset$ is a **modal class**. Only modal types may implement modal classes.

The **interface** of class $Cl$ is defined as:

$$\text{interface}(Cl) = P_{abs} \cup P_{con} \cup A_{abs} \cup A_{con} \cup F \cup St$$

A type $T$ **implements** class $Cl$ (written $T <: Cl$) when $T$ provides concrete definitions for all abstract members:

$$T <: Cl \iff \forall p \in P_{abs}.\ T \text{ defines } p\ \land\ \forall a \in A_{abs}.\ T \text{ binds } a\ \land\ \forall f \in F.\ T \text{ has } f\ \land\ \forall s \in St.\ T \text{ has } s$$

Additionally, if $St \neq \emptyset$, then $T$ must be a modal type.

##### Static Semantics

Cursive provides three distinct mechanisms for polymorphism, each with different dispatch semantics, performance characteristics, and use cases:

| Path | Name                 | Dispatch                        | Cost              | Use Case                  |
| :--- | :------------------- | :------------------------------ | :---------------- | :------------------------ |
| 1    | Static Polymorphism  | Compile-time (monomorphization) | Zero              | Constrained generics      |
| 2    | Dynamic Polymorphism | Runtime (vtable)                | One indirect call | Heterogeneous collections |
| 3    | Opaque Polymorphism  | Compile-time (type erasure)     | Zero              | Hidden return types       |

**Path Selection**

The polymorphism path is determined by usage context:

1. **Static (Path 1):** Selected when a generic parameter `T <: Cl` is used in a procedure signature. The procedure is monomorphized for each concrete type argument.

2. **Dynamic (Path 2):** Selected when `dyn Cl` is used as a type. A vtable-based dense pointer enables runtime dispatch.

3. **Opaque (Path 3):** Selected when `opaque Cl` is used as a return type. The concrete type is hidden from callers while retaining direct static dispatch within the defining module.

---

### 10.2 Class Declarations

##### Definition

A class declaration introduces a named interface that types MAY implement. Classes define requirements through abstract procedures and MAY provide default implementations through concrete procedures.

**Abstract Procedure**

An **abstract procedure** is a procedure signature within a class that lacks a body. Implementing types MUST provide a concrete implementation.

**Concrete Procedure (Default Implementation)**

A **concrete procedure** is a procedure definition within a class that includes a body. Implementing types automatically inherit this procedure but MAY override it using the `override` keyword.

**Associated Type**

An **associated type** is a type declaration within a class:
- If abstract (no `= T`): implementing types MUST provide a concrete type binding
- If concrete (`= T`): provides a default type that MAY be overridden

**Abstract Field**

An **abstract field** is a field declaration within a class that specifies a required field name and type. Implementing types MUST have a field with the same name and a compatible type (the implementing type's field may have a more specific type than required). For example, if the class declares `data: Animal`, the implementing type may declare `data: Dog` since `Dog <: Animal`.

**Abstract State**

An **abstract state** is a state declaration within a class using `@StateName { fields }` syntax. This specifies a required state name and its minimum payload fields. Implementing modal types MUST have a state with the same name containing at least the declared fields. Additional fields are permitted in the implementing state.

**Modal Class**

A class containing one or more abstract state declarations is a **modal class**. Only modal types may implement modal classes. Records and enums MUST NOT implement modal classes.

**State References in Signatures**

Procedure signatures in modal classes may reference declared states using the standard `ClassName@StateName` syntax. Return types may be unions of class states.

**The `Self` Type**

Within a class declaration, the identifier `Self` denotes the (unknown) implementing type. `Self` MUST be used for the receiver parameter and MAY be used in any other type position.

**Generic Class Parameters**

A class MAY declare generic type parameters that parameterize the entire class definition. These parameters are distinct from associated types:
- Generic parameters (`class Foo<T>`) are specified at use-site by the implementer/consumer
- Associated types are specified by the implementer within the type body

**Superclass (Class Bounds)**

A class MAY extend one or more superclasses using the `<:` operator. A type implementing a subclass MUST also implement all of its superclasses. A subclass declaration (`class Sub <: A + B`) establishes a **nominal** subtyping relationship: `Sub` is a distinct class with its own identity, and implementing `Sub` requires implementing both `A` and `B`.

**Class Alias**

A **class alias** declares a new name that is equivalent to a combination of one or more class bounds. Class aliases enable concise expression of compound bounds.

##### Syntax & Declaration

**Grammar**

```ebnf
class_declaration ::=
    [ <visibility> ] "class" <identifier> [ <generic_params> ]
    [ "<:" <superclass_bounds> ] "{"
        <class_item>*
    "}"

superclass_bounds ::= <class_bound> ( "+" <class_bound> )*
class_bound       ::= <type_path> [ <generic_args> ]
class_path        ::= <type_path>

class_item ::=
    <abstract_procedure>
  | <concrete_procedure>
  | <associated_type>
  | <abstract_field>
  | <abstract_state>

abstract_procedure ::=
    "procedure" <identifier> <signature> [ <contract_clause> ]

concrete_procedure ::=
    "procedure" <identifier> <signature> [ <contract_clause> ] <block>

associated_type ::=
    "type" <identifier> [ "=" <type> ]

abstract_field ::=
    <identifier> ":" <type>

abstract_state ::=
    "@" <identifier> "{" [ <field_list> ] "}"

class_alias ::=
    [ <visibility> ] "type" <identifier> [ <generic_params> ]
    "=" <class_bound> ( "+" <class_bound> )* ";"
```

Class items are separated by newlines. Semicolons are not used as terminators within class bodies.

A `class_path` is syntactically identical to `type_path`; the semantic constraint is that the path must resolve to a class declaration.

The `generic_params` and `generic_args` productions are defined in §7.1 (Static Polymorphism). The `contract_clause` production is defined in §11.1 (Contract Fundamentals). The `field_list` production is defined in §5.3 (Record Types).

##### Static Semantics

**Well-Formedness (WF-Class)**

A class declaration is well-formed if:
1. The identifier is unique within its namespace
2. All procedure signatures are well-formed
3. All associated types have unique names within the class
4. All abstract fields have unique names within the class
5. All abstract states have unique names within the class
6. No name conflicts exist among procedures, associated types, fields, and states
7. All superclass bounds refer to valid classes
8. No cyclic superclass dependencies exist
9. State payload fields within each state declaration have unique names
10. State references in procedure signatures refer to states declared in the same class

**Formal Well-Formedness Rule**

$$\frac{
  \text{unique}(N) \quad
  \forall p \in P.\ \Gamma, \text{Self} : \text{Type} \vdash p\ \text{wf} \quad
  \forall a \in A.\ \Gamma \vdash a\ \text{wf} \quad
  \forall f \in F.\ \Gamma \vdash f\ \text{wf} \quad
  \forall s \in St.\ \Gamma \vdash s\ \text{wf} \quad
  \neg\text{cyclic}(S) \quad
  \text{names\_disjoint}(P, A, F, St)
}{
  \Gamma \vdash \text{class } N\ [<: S]\ \{P, A, F, St\}\ \text{wf}
} \quad \text{(WF-Class)}$$

where:
- $N$ is the class name
- $P$ is the set of procedure declarations
- $A$ is the set of associated type declarations
- $F$ is the set of abstract field declarations
- $St$ is the set of abstract state declarations
- $S$ is the set of superclass bounds

**Typing Rule for Self**

$$\frac{\Gamma, \text{Self} : \text{Type} \vdash \text{body} : \text{ok}}{\Gamma \vdash \text{class } T\ \{\ \text{body}\ \} : \text{Form}}$$
\tag{WF-Class-Self}

**Modal Class Constraint (T-Modal-Class)**

A class containing state declarations may only be implemented by modal types:

$$\frac{
  St(Cl) \neq \emptyset \quad
  T <: Cl \quad
  T \text{ is not a modal type}
}{
  \text{ill-formed: E-TYP-2401}
} \quad \text{(T-Modal-Class)}$$

**State Reference Well-Formedness (WF-State-Ref)**

A state reference `Cl@S` in a procedure signature is well-formed when `@S` is declared in class `Cl`:

$$\frac{
  @S \in St(Cl)
}{
  \Gamma \vdash Cl@S\ \text{wf}
} \quad \text{(WF-State-Ref)}$$

**State Union Return Types (T-State-Union)**

A procedure may declare a return type as a union of class states:

$$\frac{
  \forall i.\ @S_i \in St(Cl)
}{
  \Gamma \vdash Cl@S_1 \mid Cl@S_2 \mid \ldots \mid Cl@S_n : \text{Type}
} \quad \text{(T-State-Union)}$$

State union return types use the union type syntax defined in §5.5 (Union Types).

**Superclass Inheritance (T-Superclass)**

$$\frac{\text{class } A <: B \quad T <: A}{\Gamma \vdash T <: B}$$
\tag{T-Superclass}

A type implementing subclass `A` transitively implements all superclasses.

**Generic Class Instantiation**

For a generic class `class Foo<T>`:
- Each use-site `Foo<ConcreteType>` produces a distinct class bound
- `T` is available within the class body as a type parameter

**Class Alias Equivalence (T-Alias-Equiv)**

$$\frac{\text{type } Alias = A + B}{\Gamma \vdash T <: Alias \iff \Gamma \vdash T <: A \land \Gamma \vdash T <: B}$$
\tag{T-Alias-Equiv}

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                         | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------------ | :----------- | :-------- |
| `E-TYP-2500` | Error    | Duplicate procedure name in class.                | Compile-time | Rejection |
| `E-TYP-2504` | Error    | Duplicate associated type name in class.          | Compile-time | Rejection |
| `E-TYP-2505` | Error    | Name conflict among class members.                | Compile-time | Rejection |
| `E-TYP-2508` | Error    | Cyclic superclass dependency detected.            | Compile-time | Rejection |
| `E-TYP-2509` | Error    | Superclass bound refers to undefined class.       | Compile-time | Rejection |
| `E-TYP-2408` | Error    | Duplicate abstract field name in class.           | Compile-time | Rejection |
| `E-TYP-2409` | Error    | Duplicate abstract state name in class.           | Compile-time | Rejection |
| `E-TYP-2410` | Error    | State reference to undeclared state in signature. | Compile-time | Rejection |
| `E-TYP-2411` | Error    | Duplicate field name within state payload.        | Compile-time | Rejection |

---

### 10.3 Class Implementation

##### Definition

**Class Implementation**

A type implements a class by:
1. Declaring the class in its "implements clause" using the `<:` operator
2. Providing implementations for all abstract procedures
3. Providing type bindings for all abstract associated types
4. Having fields with matching names and compatible types for all abstract fields
5. Having states with matching names and at least the required payload fields for all abstract states (modal classes only)

**Implementation Site**

Class implementation MUST occur at the type's definition site. Class implementations at sites other than the type's definition (extension implementations) are prohibited.

##### Syntax & Declaration

**Grammar**

The class implementation clause is integrated into type declarations using the `<:` operator followed by a comma-separated list of class paths.

> **Cross-Reference:** The complete grammar for type declarations with class implementation is defined in:
> - §5.3 (`record_decl` with `implements_clause`)
> - §5.4 (`enum_decl` with `implements_clause`)
> - §6.1 (`modal_decl` with `implements_clause`)
>
> The `implements_clause` and `class_list` productions are canonically defined in §5.3 (Record Types) and referenced by §5.4 (Enum Types) and §6.1 (Modal Types).

**Class List**

The `class_list` production (defined in §5.3) specifies one or more classes to implement:

```ebnf
implements_clause ::= "<:" class_list
class_list         ::= type_path ("," type_path)*
```

##### Static Semantics

**Implementation Completeness (T-Impl-Complete)**

$$\frac{
  T <: Cl \quad
  \forall p \in P_{abs}(Cl).\ T \text{ defines } p \quad
  \forall a \in A_{abs}(Cl).\ T \text{ binds } a \quad
  \forall f \in F(Cl).\ T \text{ has } f \quad
  \forall s \in St(Cl).\ T \text{ has } s
}{
  \Gamma \vdash T \text{ implements } Cl
}$$
\tag{T-Impl-Complete}

**Implementation Well-Formedness (WF-Impl)**

A class implementation `T <: Cl` is well-formed if:

$$\frac{
  \Gamma \vdash T\ \text{wf} \quad
  \Gamma \vdash Cl\ \text{wf} \quad
  \forall p \in P_{abs}(Cl).\ T \vdash p\ \text{implemented} \quad
  \forall a \in A_{abs}(Cl).\ T \vdash a\ \text{bound} \quad
  \forall f \in F(Cl).\ T \vdash f\ \text{present} \quad
  \forall s \in St(Cl).\ T \vdash s\ \text{present} \quad
  St(Cl) \neq \emptyset \implies T \text{ is modal}
}{
  \Gamma \vdash T <: Cl\ \text{wf}
} \quad \text{(WF-Impl)}$$

**Override Semantics**

The `override` keyword distinguishes between two implementation scenarios:

- **Implementing an abstract procedure**: When a type provides the first implementation of a procedure that has no body in the class, the `override` keyword MUST NOT be used. This is an *initial implementation*, not an override.
- **Overriding a concrete procedure**: When a type provides its own implementation of a procedure that already has a default body in the class, the `override` keyword MUST be used. This signals that the type is intentionally replacing the inherited behavior.

**Override Syntax**

Within a type's implementation of a class, procedure implementations follow this grammar:

```ebnf
impl_procedure ::= [ <visibility> ] [ "override" ] "procedure" <identifier> <signature> <block>
```

Example:
```cursive
record MyType <: SomeClass {
    public override procedure defaultMethod(~!) {
        // Replaces the default implementation from SomeClass
    }
}
```

**Coherence Rule**

A type `T` MAY implement a class `Cl` at most once. Multiple implementations of the same class for the same type are forbidden.

**Orphan Rule**

For `T <: Cl`, at least one of the following MUST be true:
- `T` is defined in the current assembly
- `Cl` is defined in the current assembly

This rule prevents external code from creating conflicting implementations.

#### Field and State Implementation

##### Static Semantics

**Field Compatibility (T-Field-Compat)**

An implementing type's field satisfies an abstract field requirement when it has the same name and a compatible type:

$$\frac{
  f : T_c \in F(Cl) \quad
  f : T_i \in \text{fields}(R) \quad
  R <: Cl \quad
  T_i <: T_c
}{
  R \vdash f\ \text{present}
} \quad \text{(T-Field-Compat)}$$

**State Compatibility (T-State-Compat)**

An implementing modal's state satisfies an abstract state requirement when the state has the same name and contains at least the required payload fields:

$$\frac{
  @S \{ f_1: T_1, \ldots, f_n: T_n \} \in St(Cl) \quad
  @S \{ f_1: T'_1, \ldots, f_n: T'_n, \ldots \} \in \text{states}(M) \quad
  M <: Cl \quad
  \forall i \in 1..n.\ T'_i <: T_i
}{
  M \vdash @S\ \text{present}
} \quad \text{(T-State-Compat)}$$

The implementing state MAY have additional fields beyond those required by the class. These additional fields are invisible when the value is accessed through the class interface.

**Uninhabited State Omission**

When any field of a class state has an uninhabited type, the implementing modal MAY omit that state:

$$\frac{
  @S \{ f_1: T_1, \ldots, f_n: T_n \} \in St(Cl) \quad
  \exists i.\ T_i = \texttt{!} \quad
  M <: Cl
}{
  M \text{ may omit } @S
} \quad \text{(T-State-Uninhabited)}$$

**Pattern Matching through Class Interface**

When pattern matching on a value through a class type, match arms use the class's state names. The runtime dispatches to the concrete modal's corresponding state. Only class-declared fields are accessible in the pattern; additional implementation fields are not bindable.

**Name Conflict Rule**

A type implementing multiple classes with identically-named fields is ill-formed:

$$\frac{
  T <: Cl_1 + Cl_2 \quad
  \exists f.\ f \in F(Cl_1) \land f \in F(Cl_2)
}{
  \text{ill-formed: E-TYP-2406}
} \quad \text{(T-Field-Conflict)}$$

Similarly, a modal implementing multiple modal classes with identically-named states is ill-formed:

$$\frac{
  M <: Cl_1 + Cl_2 \quad
  \exists @S.\ @S \in St(Cl_1) \land @S \in St(Cl_2)
}{
  \text{ill-formed: E-TYP-2407}
} \quad \text{(T-State-Conflict)}$$

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                               | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------------------ | :----------- | :-------- |
| `E-TYP-2501` | Error    | `override` used on abstract procedure implementation.   | Compile-time | Rejection |
| `E-TYP-2502` | Error    | Missing `override` on concrete procedure replacement.   | Compile-time | Rejection |
| `E-TYP-2503` | Error    | Type does not implement required procedure from class.  | Compile-time | Rejection |
| `E-TYP-2506` | Error    | Coherence violation: duplicate class implementation.    | Compile-time | Rejection |
| `E-TYP-2507` | Error    | Orphan rule violation: neither type nor class is local. | Compile-time | Rejection |
| `E-TYP-2401` | Error    | Non-modal type implements modal class.                  | Compile-time | Rejection |
| `E-TYP-2402` | Error    | Implementing type missing required field.               | Compile-time | Rejection |
| `E-TYP-2403` | Error    | Implementing modal missing required state.              | Compile-time | Rejection |
| `E-TYP-2404` | Error    | Implementing field has incompatible type.               | Compile-time | Rejection |
| `E-TYP-2405` | Error    | Implementing state missing required payload field.      | Compile-time | Rejection |
| `E-TYP-2406` | Error    | Conflicting field names from multiple classes.          | Compile-time | Rejection |
| `E-TYP-2407` | Error    | Conflicting state names from multiple classes.          | Compile-time | Rejection |

---

### 10.4 Class Constraints in Generics

##### Definition

Class constraints integrate classes with the generic type system: a generic parameter `T <: Cl` restricts valid type arguments to types implementing class `Cl`.

> **Cross-Reference:** The complete specification of static polymorphism, including grammar and monomorphization, is defined in §7.1. This section documents class-specific constraint usage.

##### Static Semantics

1. **Constraint Satisfaction:** A generic instantiation is well-formed only if every constrained parameter `T <: Cl` is instantiated with a type that implements `Cl`. Violations are diagnosed as `E-TYP-2530`.
2. **Method Availability:** Within the body of a generic item, methods of `Cl` are callable on values of type `T` via static dispatch; calls resolve at monomorphization with no vtable lookup.
3. **Use of Unconstrained Parameters:** Using an unconstrained type parameter where a class method would be required is ill-formed (`E-TYP-2531`).

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                          | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------------- | :----------- | :-------- |
| `E-TYP-2530` | Error    | Type argument does not satisfy class constraint.   | Compile-time | Rejection |
| `E-TYP-2531` | Error    | Unconstrained type parameter used in class method. | Compile-time | Rejection |

> ```cursive
> procedure summarize<T <: Display>(items: const [T], output: dyn Writer) {
>     loop item in items {
>         item~>display(output)  // T <: Display guarantees display method
>     }
> }
> ```

---

### 10.5 Dynamic Polymorphism

##### Definition

**Dynamic Polymorphism**

Dynamic polymorphism is opt-in runtime dispatch using dynamic class types. It enables heterogeneous collections and runtime polymorphism at the cost of one vtable lookup per call.

**Dynamic Class Type**

A **dynamic class type** (`dyn Class`) is a concrete, sized type representing any value implementing a dispatchable class. It is implemented as a dense pointer.

**Dispatchability**

A class is **dispatchable** if every procedure in the class (including inherited ones) is either:
1. **VTable-eligible**, OR
2. **Explicitly excluded** via `[[static_dispatch_only]]`

**VTable Eligibility**

A procedure is **vtable-eligible** if **ALL** of the following are true:
1. Has a receiver parameter (`self`, `~`, `~!`, `~%`)
2. Has **NO** generic type parameters
3. Does **NOT** return `Self` by value (except via pointer indirection to `Self` or `dyn Self`)
4. Does **NOT** use `Self` in parameter types (except via pointer indirection)

**The `[[static_dispatch_only]]` Exclusion Attribute**

A procedure that is not vtable-eligible MAY be excluded from dispatchability requirements by applying the `[[static_dispatch_only]]` attribute. This attribute restricts the procedure to contexts where the concrete type is statically known, making it unavailable through dynamic dispatch while preserving the class's overall dispatchability.

**State Dispatch in Modal Class Dynamic Types**

When a modal class is used as a dynamic class type, pattern matching on class states dispatches through vtable metadata. The vtable includes a state mapping table that translates between class state indices and concrete modal discriminant values.

**Formal Definition of `dispatchable`**

$$
\text{dispatchable}(Cl) \iff \forall p \in \text{procedures}(Cl).\ \text{vtable\_eligible}(p) \lor \text{has\_static\_dispatch\_attr}(p)
$$

where:
- $\text{procedures}(Cl)$ includes all procedures declared in $Cl$ and its superclasses
- $\text{vtable\_eligible}(p)$ holds if $p$ satisfies all four criteria above
- $\text{has\_static\_dispatch\_attr}(p)$ holds if $p$ has the `[[static_dispatch_only]]` attribute

##### Syntax & Declaration

**Grammar**

```ebnf
dyn_type ::= "dyn" <class_path>
```

##### Static Semantics

**Dynamic Class Formation (T-Dynamic-Form)**

A value of concrete type `T` implementing dispatchable class `Cl` MAY be coerced to `dyn Cl`:

$$\frac{\Gamma \vdash v : T \quad \Gamma \vdash T <: Cl \quad \text{dispatchable}(Cl)}{\Gamma \vdash v : \text{dyn } Cl}$$
\tag{T-Dynamic-Form}

##### Dynamic Semantics

**Evaluation**

**Dynamic Class Type Creation**

Formation of a dynamic class type from a concrete value proceeds as follows:

1. Let `v` be a value of concrete type `T` where `T <: Cl` and `dispatchable(Cl)`.
2. Let `dp` be a pointer to the storage of `v`.
3. Let `vt` be the static vtable for the `(T, Cl)` type-class pair.
4. Construct the dynamic class value as the pair `(dp, vt)`.

**Formal Dynamic Class Formation Rule**

$$\frac{
  \Gamma \vdash v : T \quad
  T <: Cl \quad
  \text{dispatchable}(Cl)
}{
  v \Rightarrow_{\text{dyn}} (\text{ptr}(v), \text{vtable}(T, Cl))
} \quad \text{(E-Dynamic-Form)}$$

**VTable Dispatch Algorithm**

A procedure call `w~>method(args)` on dynamic class type `w: dyn Cl` executes as follows:

1. Let `(dp, vt)` be the data pointer and vtable pointer components of `w`.
2. Let `offset` be the vtable offset for `method` (determined at compile time from class declaration order, offset by header size).
3. Let `header_size` be `3 * sizeof(usize)` for non-modal classes, or `4 * sizeof(usize)` for modal classes.
4. Let `fp` be the function pointer at `vt + header_size + offset * sizeof(usize)`.
5. Return the result of calling `fp(dp, args)`.

**Formal Dispatch Rule**

$$\frac{
  w = (dp, vt) \quad
  \text{method} \in \text{interface}(Cl) \quad
  vt[\text{offset}(\text{method})] = fp
}{
  w\text{\textasciitilde>method}(args) \to fp(dp, args)
} \quad \text{(E-Dynamic-Dispatch)}$$

##### Memory & Layout

**Dense Pointer Layout**

A dynamic class type (`dyn Class`) is represented as a two-word structure:

**Memory Layout (Implementation Detail):**

```
struct DynRepr {
    data: *imm (),      // Pointer to concrete instance
    vtable: *imm VTable // Pointer to class vtable
}
```

- **Size**: `2 * sizeof(usize)` (16 bytes on 64-bit platforms)
- **Alignment**: `alignof(usize)`

**VTable Layout (Stable ABI)**

VTable entries MUST appear in this exact order:

**For non-modal classes (header size = 3):**
1. `size: usize` — Size of concrete type in bytes
2. `align: usize` — Alignment requirement of concrete type
3. `drop: *imm fn` — Destructor function pointer (null if no `Drop`)
4. `methods[..]` — Function pointers in class declaration order

**For modal classes (header size = 4):**
1. `size: usize` — Size of concrete type in bytes
2. `align: usize` — Alignment requirement of concrete type
3. `drop: *imm fn` — Destructor function pointer (null if no `Drop`)
4. `state_map: *imm StateMap` — State discriminant mapping
5. `methods[..]` — Function pointers in class declaration order

**State Map Structure**

For modal classes, the `state_map` pointer references a mapping structure. The structure contains the class state count and an array mapping each class state index to the corresponding concrete modal discriminant value. A sentinel value (the maximum value for the discriminant type, such as `0xFF` for u8 discriminants or `0xFFFFFFFF` for u32 discriminants) is used for omitted uninhabited states.

**State Match Dispatch**

When pattern matching on `dyn ModalClass`:
1. Read the concrete value's discriminant from its memory representation
2. Look up the class state index corresponding to this discriminant via the state map
3. Dispatch to the match arm for that class state
4. Bind only the class-declared fields (additional implementation fields are inaccessible)

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                                                | Detection    | Effect    |
| :----------- | :------- | :----------------------------------------------------------------------- | :----------- | :-------- |
| `E-TYP-2540` | Error    | Procedure with `[[static_dispatch_only]]` called on dyn.                 | Compile-time | Rejection |
| `E-TYP-2541` | Error    | Dynamic class type created from non-dispatchable class.                  | Compile-time | Rejection |
| `E-TYP-2542` | Error    | Generic procedure in class without `[[static_dispatch_only]]` attribute. | Compile-time | Rejection |

**Permission Interaction**

Dynamic class types may be combined with permissions per §4.5. Additional constraints apply when combining dynamic class types with the shared permission.

---

### 10.6 Opaque Polymorphism

##### Definition

**Opaque Polymorphism**

Opaque polymorphism is compile-time polymorphism for return types without exposing the concrete type; only the class interface is accessible.

**Opaque Type**

An **opaque return type** (`opaque Class`) exposes only the class's interface while hiding the concrete implementation type.

##### Syntax & Declaration

**Grammar**

```ebnf
return_type ::= ... | "opaque" <class_path>
```

##### Static Semantics

**Type Encapsulation**

For a procedure returning `opaque Class`:
- The callee returns a concrete type implementing `Class`
- The caller observes only `Class` members
- Access to concrete type members is forbidden

**Opaque Return Typing (T-Opaque-Return)**

A procedure body returning a concrete type is well-typed against an opaque return type declaration:

$$\frac{
  \Gamma \vdash \text{body} : T \quad
  \Gamma \vdash T <: Cl \quad
  \text{return\_type}(f) = \text{opaque } Cl
}{
  \Gamma \vdash f : () \to \text{opaque } Cl
} \quad \text{(T-Opaque-Return)}$$

Where:
- The procedure body evaluates to a value of concrete type $T$
- The concrete type $T$ implements class $Cl$
- The declared return type is `opaque Cl`

**Opaque Type Projection (T-Opaque-Project)**

At call sites, the opaque type is treated as an existential; callers may invoke only class methods:

$$\frac{
  \Gamma \vdash f() : \text{opaque } Cl \quad
  m \in \text{interface}(Cl) \quad
  \text{signature}(Cl.m) = (S_1, \ldots, S_n) \to R_m
}{
  \Gamma \vdash f().\text{~>}m(a_1, \ldots, a_n) : R_m
} \quad \text{(T-Opaque-Project)}$$

Where:
- $f()$ returns an opaque type with class bound $Cl$
- $m$ is a procedure declared in class $Cl$ (or its superclasses)
- The call is well-typed if arguments match the procedure signature

**Opaque Type Equality**

Two opaque types `opaque Cl` are considered equivalent types if and only if they originate from the same procedure definition. Different procedures returning `opaque Cl` produce distinct, incompatible types:

$$\frac{
  \Gamma \vdash f : () \to \text{opaque } Cl \quad
  \Gamma \vdash g : () \to \text{opaque } Cl \quad
  f \neq g
}{
  \text{typeof}(f()) \neq \text{typeof}(g())
} \quad \text{(T-Opaque-Distinct)}$$

**Zero Overhead**

Opaque types MUST incur zero runtime overhead. The returned value is the concrete type, not a dense pointer. Type encapsulation is enforced statically.

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                             | Detection    | Effect    |
| :----------- | :------- | :---------------------------------------------------- | :----------- | :-------- |
| `E-TYP-2510` | Error    | Accessing member not defined on opaque type's class.  | Compile-time | Rejection |
| `E-TYP-2511` | Error    | Opaque return type does not implement required class. | Compile-time | Rejection |
| `E-TYP-2512` | Error    | Attempting to assign incompatible opaque types.       | Compile-time | Rejection |

---

### 10.7 Foundational Classes

##### Definition

Foundational classes (`Drop`, `Copy`, `Clone`, `Iterator`) have special semantics. Their normative signatures, constraints, and complete semantics are defined in **Appendix G (Foundational Forms)**.

---

## Clause 11: Contracts and Verification

### 11.1 Contract Fundamentals

##### Definition

A **Contract** is a specification attached to a procedure declaration that asserts logical predicates over program state. Contracts govern the logical validity of procedures through preconditions (caller obligations) and postconditions (callee guarantees).

**Formal Definition**

A contract $C$ is a pair $(P_{pre}, P_{post})$ where:
- $P_{pre}$ is a conjunction of boolean predicates representing preconditions
- $P_{post}$ is a conjunction of boolean predicates representing postconditions

$$C = (P_{pre}, P_{post}) \quad \text{where} \quad P_{pre}, P_{post} \in \text{Predicate}^*$$

A **Predicate** is a pure boolean expression interpreted in a specific evaluation context (see §11.1.2).

**Distinguished from Capabilities**

Contracts control the logical validity of data and operations. Capabilities (Clause 13) control authority to perform external effects. A procedure may have both.

##### Syntax & Declaration

**Grammar**

```ebnf
contract_clause
    ::= "|=" contract_body

contract_body
    ::= precondition_expr
      | precondition_expr "=>" postcondition_expr
      | "=>" postcondition_expr

precondition_expr
    ::= predicate_expr

postcondition_expr
    ::= predicate_expr

predicate_expr
    ::= logical_or_expr

logical_or_expr
    ::= logical_and_expr
      | logical_or_expr "||" logical_and_expr

logical_and_expr
    ::= logical_not_expr
      | logical_and_expr "&&" logical_not_expr

logical_not_expr
    ::= "!" logical_not_expr
      | primary_predicate

primary_predicate
    ::= comparison_expr
      | "(" predicate_expr ")"
      | identifier
      | procedure_call
      | field_access
      | contract_intrinsic

contract_intrinsic
    ::= "@result"
      | "@entry" "(" expression ")"
```

> **Note:** The `comparison_expr` and `procedure_call` productions reference the expression grammar defined in Clause 12. Pure procedure calls are enforced semantically per §11.1.1.

**Positioning**

A contract clause (introduced by `|=`) MUST appear after the return type annotation (or after the parameter list if no return type is specified) and before the procedure body or terminating semicolon.

```cursive
// Contract after return type, before body
procedure divide(a: i32, b: i32) -> i32
    |= b != 0 => @result * b == a
{
    a / b
}

// Contract on procedure without return type annotation
procedure assert_positive(x: i32)
    |= x > 0
{
    // ...
}
```

##### Static Semantics

**Well-Formedness Rule (WF-Contract)**

A contract clause is well-formed if and only if both its precondition and postcondition expressions are well-formed predicates:

$$
\frac{
    \Gamma_{pre} \vdash P_{pre} : \texttt{bool} \quad \text{pure}(P_{pre}) \\
    \Gamma_{post} \vdash P_{post} : \texttt{bool} \quad \text{pure}(P_{post})
}{
    \Gamma \vdash \texttt{|=}\ P_{pre} \Rightarrow P_{post} : \text{WF}
}
\tag{WF-Contract}
$$

where:
- $\Gamma_{pre}$ is the precondition evaluation context (§11.1.2)
- $\Gamma_{post}$ is the postcondition evaluation context (§11.1.2)
- $\text{pure}(e)$ holds iff $e$ satisfies all purity constraints (§11.1.1)

**Logical Operators**

Predicates use standard boolean operators with the following precedence (highest to lowest):
1. `!` (logical NOT) — right-associative
2. `&&` (logical AND) — left-associative
3. `||` (logical OR) — left-associative

Short-circuit evaluation applies: `&&` does not evaluate its right operand if the left is `false`; `||` does not evaluate its right operand if the left is `true`.

##### Constraints & Legality

A syntactically malformed contract clause causes the program to be ill-formed; the implementation issues a diagnostic per §2.12 (grammar enforcement).

---

#### 11.1.1 Purity Constraints

##### Definition

A **pure expression** is an expression whose evaluation produces no observable side effects. All expressions within a contract MUST be pure.

##### Static Semantics

An expression $e$ satisfies $\text{pure}(e)$ if and only if:

1. $e$ MUST NOT invoke any procedure that accepts capability parameters.
2. $e$ MUST NOT mutate state observable outside the expression's evaluation.
3. Built-in operators on primitive types and `comptime` procedures are always pure.

##### Constraints & Legality

| Code         | Severity | Condition                                     | Detection    | Effect    |
| :----------- | :------- | :-------------------------------------------- | :----------- | :-------- |
| `E-SEM-2802` | Error    | Impure expression used in contract predicate. | Compile-time | Rejection |

---

#### 11.1.2 Evaluation Contexts

##### Definition

An **Evaluation Context** defines the set of bindings available for reference within a predicate expression. Preconditions and postconditions have distinct evaluation contexts.

##### Static Semantics

**Precondition Evaluation Context ($\Gamma_{pre}$)**

The precondition evaluation context includes:
- All procedure parameters at their entry state
- The receiver binding (if present), accessible via receiver shorthand (`~`, `~!`, `~%`)
- All bindings visible in the enclosing scope that are accessible without side effects

The precondition context MUST NOT include:
- The `@result` intrinsic
- The `@entry` operator
- Any binding introduced within the procedure body

**Postcondition Evaluation Context ($\Gamma_{post}$)**

The postcondition evaluation context includes:
- All procedure parameters:
  - Immutable parameters (`const`, `~`): same value as at entry
  - Mutable parameters (`unique`, `shared`): post-state value
- The receiver binding (post-state for mutable receivers)
- The `@result` intrinsic (§11.2.2)
- The `@entry` operator (§11.2.2)
- All bindings visible in the enclosing scope

**Mutable Parameter State Semantics**

For parameters with mutable permission (`~!`, `~%`):

| Location in Contract          | State Referenced |
| :---------------------------- | :--------------- |
| Left of `=>`                  | Entry state      |
| Right of `=>` (bare)          | Post-state       |
| Right of `=>` with `@entry()` | Entry state      |

```cursive
record Counter {
    value: i32,

    procedure increment(~!)
        |= self.value >= 0 => self.value == @entry(self.value) + 1
    {
        self.value += 1;
    }
}
```

---

#### 11.1.3 Inline Parameter Constraints

##### Definition

An **Inline Parameter Constraint** is a `where` clause attached directly to a parameter declaration. This syntax is sugar for a refinement type (§7.3) that is also added as a precondition.

##### Syntax & Declaration

```ebnf
parameter_with_constraint
    ::= identifier ":" type_expr "where" "{" predicate_expr "}"
```

##### Static Semantics

**Desugaring**

A parameter declaration `x: T where { P }` is semantically equivalent to:
1. Declaring `x` with refinement type `T where { P }` (per §7.3)
2. Adding `P` as a conjunct to the procedure's effective precondition

The refinement type semantics (scope rules, evaluation, validity) are defined in §7.3.

**Effective Precondition**

Given inline constraints $\{P_1, P_2, \ldots, P_n\}$ and procedure-level precondition $P_{proc}$:

$$P_{eff} = P_1 \land P_2 \land \cdots \land P_n \land P_{proc}$$

Constraints are conjoined left-to-right in parameter declaration order.

---

### 11.2 Contract Clauses

##### Definition

Contract clauses define preconditions and postconditions expressed with the `|=` syntax that govern caller obligations and callee guarantees.

---

#### 11.2.1 Preconditions

##### Definition

A **Precondition** is the logical expression appearing to the left of `=>` in a `|=` contract clause, or the entire expression if `=>` is absent. The caller MUST ensure this expression evaluates to `true` at the call site prior to transfer of control to the callee.

##### Static Semantics

**Caller Obligation Rule**

When a procedure $f$ with precondition $P_{pre}$ is called at site $S$, the implementation MUST verify that a Verification Fact (§11.5) $F(P_{pre}, L)$ exists where $L$ dominates $S$ in the Control Flow Graph.

$$
\frac{
    \Gamma \vdash f : (T_1, \ldots, T_n) \to R \quad
    \text{precondition}(f) = P_{pre} \quad
    \exists L.\ F(P_{pre}, L) \land L \text{ dom } S
}{
    \Gamma \vdash f(a_1, \ldots, a_n) @ S : \text{valid}
}
\tag{Pre-Satisfied}
$$

**Attribution**

Failure to satisfy a precondition is diagnosed at the **caller**. The diagnostic MUST identify the call site, not the callee's declaration.

**Elision Rules**

| Contract Form        | Precondition              |
| :------------------- | :------------------------ |
| `\|= P`              | $P$                       |
| `\|= P => Q`         | $P$                       |
| `\|= => Q`           | `true` (always satisfied) |
| (no contract clause) | `true` (always satisfied) |

##### Constraints & Legality

Precondition scope is defined by $\Gamma_{pre}$ (§11.1.2). Static verification is required by default; a precondition that cannot be statically verified is diagnosed as `E-SEM-2801`. If the expression is within a `[[dynamic]]` scope (§11.4), a runtime check is inserted when static proof fails; runtime failure causes a panic.

---

#### 11.2.2 Postconditions

##### Definition

A **Postcondition** is the logical expression appearing to the right of `=>` in a `|=` contract clause. The callee MUST ensure this expression evaluates to `true` immediately before returning control to the caller.

##### Static Semantics

**Callee Obligation Rule**

The procedure implementation MUST ensure the postcondition holds at every return point:

$$
\frac{
    \text{postcondition}(f) = P_{post} \quad
    \forall r \in \text{ReturnPoints}(f).\ \Gamma_r \vdash P_{post} : \text{satisfied}
}{
    f : \text{postcondition-valid}
}
\tag{Post-Valid}
$$

> **Note:** The judgments `: valid` and `: satisfied` denote successful verification of the predicate in context. These are equivalent and indicate the predicate holds.

**Attribution**

Failure to satisfy a postcondition is diagnosed at the **callee**. The diagnostic MUST identify the return point within the procedure body.

**Elision Rules**

| Contract Form        | Postcondition             |
| :------------------- | :------------------------ |
| `\|= P`              | `true` (no postcondition) |
| `\|= P => Q`         | $Q$                       |
| `\|= => Q`           | $Q$                       |
| (no contract clause) | `true` (no postcondition) |

##### Dynamic Semantics

**Contract Intrinsics**

Two intrinsics are available exclusively in postcondition expressions:

**1. `@result` Intrinsic**

| Property     | Specification                                                             |
| :----------- | :------------------------------------------------------------------------ |
| Availability | Postcondition expressions only (right of `=>`)                            |
| Type         | The return type of the enclosing procedure                                |
| Value        | The value being returned from the procedure                               |
| Unit Returns | If procedure returns `()`, `@result` has value `()`                       |
| Shadowing    | `@result` is an intrinsic keyword; it MUST NOT be shadowed by any binding |

> **Note:** `@result` and `@entry` are contract intrinsics and are reserved identifiers within contract contexts. They cannot be shadowed or used as user-defined identifiers in any scope.

```cursive
procedure abs(x: i32) -> i32
    |= => @result >= 0 && (@result == x || @result == -x)
{
    if x >= 0 { x } else { -x }
}
```

**2. `@entry(expr)` Operator**

| Property              | Specification                                          |
| :-------------------- | :----------------------------------------------------- |
| Availability          | Postcondition expressions only (right of `=>`)         |
| Semantics             | Evaluates `expr` in the entry state of the procedure   |
| Evaluation Point      | After parameter binding, before body execution         |
| Expression Constraint | `expr` MUST be pure (§11.1.1)                          |
| Expression Scope      | `expr` MUST reference only parameters and receiver     |
| Type Constraint       | Result type of `expr` MUST implement `Copy` or `Clone` |
| Capture Mechanism     | Implementation captures the value at procedure entry   |

**Capture Semantics**

When `@entry(expr)` appears in a postcondition:
1. The implementation evaluates `expr` immediately after parameter binding
2. The result is captured and stored according to the type's copy semantics:
   - **Copy types**: A bitwise copy is performed (zero runtime cost beyond the copy itself)
   - **Clone types**: The `clone()` procedure is invoked to produce the captured value
3. In the postcondition, `@entry(expr)` refers to this captured value

> **Performance Note:** For `Clone` types, the `clone()` invocation occurs unconditionally at procedure entry, regardless of whether the postcondition will ultimately be evaluated (e.g., in `dynamic` mode where the procedure may panic before return). Authors should prefer capturing `Copy` projections (e.g., `@entry(self.len())`) over cloning entire structures when possible.

```cursive
record Vec<T> {
    // ...

    procedure push(~!, item: T)
        |= => self.len() == @entry(self.len()) + 1
    {
        // Implementation...
    }
}
```

**Non-Copyable Resources**

Direct capture of non-copyable resources is prohibited. To reference properties of non-copyable resources, capture a copyable projection:

```cursive
// Invalid: Buffer is not Copy
procedure process(buf: unique Buffer)
    |= => buf == @entry(buf)  // Error: Buffer does not implement Copy
{ /* ... */ }

// Valid: Capture the length (usize is Copy)
procedure process_valid(buf: unique Buffer)
    |= => buf.len() == @entry(buf.len())  // OK
{ /* ... */ }
```

**Formal Typing**

The typing judgment for `@entry` establishes that the expression must be well-typed in the postcondition context and its type must satisfy the copyability constraint:

$$
\frac{\Gamma_{post} \vdash e : T \quad (T <: \text{Copy} \lor T <: \text{Clone})}{\Gamma_{post} \vdash \text{@entry}(e) : T}
\tag{Entry-Type}
$$

##### Constraints & Legality

**Scope Permissions**

Postcondition expressions:
- MAY reference all procedure parameters (post-state for mutable)
- MAY reference receiver shorthand (`~`, `~!`, etc.)
- MAY reference `@result`
- MAY reference `@entry(expr)` where `expr` references only parameters/receiver

| Code         | Severity | Condition                                             | Detection    | Effect    |
| :----------- | :------- | :---------------------------------------------------- | :----------- | :-------- |
| `E-SEM-2805` | Error    | `@entry()` result type does not implement Copy/Clone. | Compile-time | Rejection |
| `E-SEM-2806` | Error    | `@result` used outside postcondition context.         | Compile-time | Rejection |

Static verification is required by default; a postcondition that cannot be statically verified is diagnosed as `E-SEM-2801`. If the expression is within a `[[dynamic]]` scope (§11.4), a runtime check is inserted when static proof fails; runtime failure causes a panic.

---

### 11.3 Invariants

##### Definition

An **Invariant** is a predicate that MUST hold true over a defined scope or lifetime. Cursive defines two forms of invariants: Type Invariants and Loop Invariants.

> **Cross-Reference:** Refinement Types (§7.3) provide anonymous invariants attached to type references (e.g., `usize where { self < len }`). Verification of refinement type predicates uses the mechanisms defined in §11.4 and §11.5.

---

#### 11.3.1 Type Invariants

##### Definition

A **Type Invariant** is a `where` clause attached to a `record`, `enum`, or `modal` type declaration. The invariant constrains all instances of the type: every instance MUST satisfy the invariant at designated enforcement points.

##### Syntax & Declaration

**Grammar**

```ebnf
record_declaration
    ::= [ visibility ] "record" identifier [ generic_params ] 
        "{" field_list "}" [ type_invariant ]

enum_declaration
    ::= [ visibility ] "enum" identifier [ generic_params ]
        "{" variant_list "}" [ type_invariant ]

modal_declaration
    ::= [ visibility ] "modal" identifier [ generic_params ]
        "{" state_list "}" [ type_invariant ]

type_invariant
    ::= "where" "{" predicate_expr "}"
```

```cursive
record BoundedCounter {
    value: u32,
    max: u32,
}
where { self.value <= self.max }
```

##### Static Semantics

**Invariant Predicate Context**

Within a type invariant predicate:
- `self` refers to an instance of the type being defined
- Field access on `self` is permitted
- Method calls on `self` are permitted if the method accepts no capability parameters (i.e., is pure)

**Enforcement Points**

A type invariant MUST be verified at the following program points:

| Enforcement Point   | Description                                                          |
| :------------------ | :------------------------------------------------------------------- |
| Post-Construction   | Immediately after a constructor or literal initialization completes  |
| Pre-Call (Public)   | Before any public procedure is invoked with the instance as receiver |
| Post-Call (Mutator) | Before any procedure taking `unique` `self` returns                  |

At each enforcement point:
- If the expression is within a `[[dynamic]]` scope (§7.2.9): runtime check is emitted when static proof fails
- If the expression is outside `[[dynamic]]` scope: static proof is required; failure is ill-formed (`E-SEM-2801`)

**Public Field Constraint**

Types with type invariants MUST NOT declare public mutable fields. Allowing direct public field mutation would bypass invariant enforcement. All mutations to such types MUST go through procedures.

| Code         | Severity | Condition                                   | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------ | :----------- | :-------- |
| `E-SEM-2824` | Error    | Public mutable field on type with invariant | Compile-time | Rejection |

**Effective Postcondition Desugaring**

For a procedure $P$ on type $T$ with:
- Type invariant $Inv$
- Explicit postcondition $Post$
- Mutable receiver (`unique`)

The **effective postcondition** is:

$$\text{EffectivePost}(P) = Post \land Inv(\texttt{self})$$

The implementation MUST verify both the explicit postcondition and the type invariant before return.

**Private Procedure Exemption**

Private procedures (visibility `internal` or less) are exempt from the Pre-Call enforcement point. This exemption enables internal helper procedures to temporarily violate the type invariant during complex state transitions.

1. The type invariant MAY be violated at any point during private procedure execution.
2. The type invariant MAY remain violated when returning to another private caller (intra-private composition).
3. **Boundary Enforcement:** The type invariant MUST be verified when a private procedure returns to a public caller. This return boundary is a **Type Invariant Enforcement Point**.

Enforcement at the private-to-public boundary follows standard verification rules (§11.4): static proof is required by default; if static proof fails and the scope is marked `[[dynamic]]`, a runtime check is inserted.

##### Constraints & Legality

| Code         | Severity | Condition                                                     | Detection | Effect    |
| :----------- | :------- | :------------------------------------------------------------ | :-------- | :-------- |
| `E-SEM-2820` | Error    | Type invariant violated at construction.                      | See §11.4 | See §11.4 |
| `E-SEM-2821` | Error    | Type invariant violated at public procedure entry.            | See §11.4 | See §11.4 |
| `E-SEM-2822` | Error    | Type invariant violated at mutator procedure return.          | See §11.4 | See §11.4 |
| `E-SEM-2823` | Error    | Type invariant violated at private-to-public return boundary. | See §11.4 | See §11.4 |

---

#### 11.3.2 Loop Invariants

##### Definition

A **Loop Invariant** is a `where` clause attached to a `loop` expression. The invariant specifies a predicate that MUST hold at the beginning of every iteration and after the loop terminates.

##### Syntax & Declaration

**Grammar**

```ebnf
loop_expression
    ::= "loop" [ loop_condition ] [ loop_invariant ] block

loop_invariant
    ::= "where" "{" predicate_expr "}"

loop_condition
    ::= expression
```

> The `block` production is defined in §11.7.

```cursive
var sum = 0;
var i = 0;

loop i < n
    where { sum == i * (i - 1) / 2 && i <= n }
{
    sum += i;
    i += 1;
}
```

##### Static Semantics

**Enforcement Points**

A loop invariant $Inv$ MUST be verified at three points:

| Point          | Description                               | Formal                                                 |
| :------------- | :---------------------------------------- | :----------------------------------------------------- |
| Initialization | Before the first iteration begins         | $\Gamma_0 \vdash Inv$                                  |
| Maintenance    | At the start of each subsequent iteration | $\Gamma_i \vdash Inv \implies \Gamma_{i+1} \vdash Inv$ |
| Termination    | Immediately after loop terminates         | $\Gamma_{exit} \vdash Inv$                             |

**Verification Fact Generation**

Upon successful verification of a loop invariant at the Termination point, the implementation MUST generate a Verification Fact $F(Inv, L_{exit})$ where $L_{exit}$ is the program location immediately after the loop. This enables the invariant to be used as a postcondition of the loop construct.

##### Constraints & Legality

| Code         | Severity | Condition                                         | Detection | Effect    |
| :----------- | :------- | :------------------------------------------------ | :-------- | :-------- |
| `E-SEM-2830` | Error    | Loop invariant not established at initialization. | See §11.4 | See §11.4 |
| `E-SEM-2831` | Error    | Loop invariant not maintained across iteration.   | See §11.4 | See §11.4 |

---

### 11.4 Contract Verification

##### Definition

Contract verification determines how contract predicates are ensured to hold. By default, **static verification is required**: the contract MUST be statically provable, or the program is ill-formed. The `[[dynamic]]` attribute permits runtime verification as an explicit opt-in when static proof is not achievable.

##### Static Semantics

**Default: Static Verification Required**

For any contract predicate $P$ without enclosing `[[dynamic]]` scope (§7.2.9):

$$
\frac{
    \text{StaticProof}(\Gamma, P)
}{
    P : \text{verified (no runtime check)}
}
\tag{Contract-Static-OK}
$$

$$
\frac{
    \neg\text{StaticProof}(\Gamma, P) \quad
    \neg\text{InDynamicContext}
}{
    \text{program is ill-formed (E-SEM-2801)}
}
\tag{Contract-Static-Fail}
$$

**With `[[dynamic]]`: Runtime Verification Permitted**

For any contract predicate $P$ within a `[[dynamic]]` scope:

$$
\frac{
    \text{StaticProof}(\Gamma, P)
}{
    P : \text{verified (no runtime check)}
}
\tag{Contract-Dynamic-Elide}
$$

$$
\frac{
    \neg\text{StaticProof}(\Gamma, P) \quad
    \text{InDynamicContext}
}{
    \text{emit runtime check for } P
}
\tag{Contract-Dynamic-Check}
$$

**Proof Techniques**

To ensure portable static contract verification, proof techniques are organized into two tiers: mandatory techniques that all conforming implementations MUST support, and recommended techniques that implementations SHOULD support for enhanced verification capabilities.

**Mandatory Proof Techniques**

All conforming implementations MUST support the following proof techniques:

| Technique                | Description                                           |
| :----------------------- | :---------------------------------------------------- |
| Constant propagation     | Evaluate expressions involving compile-time constants |
| Linear integer reasoning | Prove inequalities over bounded integer arithmetic    |
| Boolean algebra          | Simplify and prove boolean expressions                |
| Control flow analysis    | Track predicates established by conditionals          |
| Type-derived bounds      | Use type constraints (e.g., `usize >= 0`)             |
| Verification facts       | Use facts established by prior checks (§11.5)         |

**Recommended Proof Techniques**

Implementations SHOULD additionally support:

| Technique                | Description                                                          |
| :----------------------- | :------------------------------------------------------------------- |
| Transitive reasoning     | Deduce `a < c` from `a < b ∧ b < c`                                  |
| Interprocedural analysis | Use postconditions from called procedures as Verification Facts      |
| Numeric range inference  | Deduce bounds from type constraints beyond basic type-derived bounds |

Support for techniques beyond the mandatory set is Implementation-Defined Behavior.

**Precondition vs Postcondition Verification Location**

| Contract Type  | Verified Where    | `[[dynamic]]` Context Determined By  |
| :------------- | :---------------- | :----------------------------------- |
| Precondition   | Call site         | The call expression's context        |
| Postcondition  | Definition site   | The procedure's `[[dynamic]]` status |
| Type invariant | Enforcement point | The expression's context             |
| Loop invariant | Enforcement point | The enclosing scope's context        |

**Precondition Verification at Call Sites**

A precondition is verified at each call site. The `[[dynamic]]` status of the call site determines whether runtime checking is permitted:

```cursive
procedure require_positive(x: i32)
    |= x > 0
{ ... }

procedure caller(n: i32) {
    require_positive(5)              // OK: trivially provable
    require_positive(n)              // Error E-SEM-2801: cannot prove n > 0
    [[dynamic]] require_positive(n)  // OK: runtime check on n > 0

    if n > 0 {
        require_positive(n)          // OK: guarded, provable
    }
}
```

**Postcondition Verification at Definition Site**

A postcondition is verified within the procedure body. The procedure's own `[[dynamic]]` status determines whether runtime checking is permitted:

```cursive
procedure abs(x: i32) -> i32
    |= => @result >= 0
{
    if x >= 0 { result x }
    else { result -x }
}

[[dynamic]]
procedure complex_computation(x: f64) -> f64
    |= => @result.is_finite()
{ /* ... */ }
```

##### Dynamic Semantics

**Runtime Check Insertion Points**

When `[[dynamic]]` permits and static proof fails:

| Contract Type  | Check Location                         |
| :------------- | :------------------------------------- |
| Precondition   | Immediately before call (at call site) |
| Postcondition  | Immediately before return (in callee)  |
| Type invariant | At each enforcement point (§11.3.1)    |
| Loop invariant | At each enforcement point (§11.3.2)    |

**Runtime Check Failure**

When a runtime-checked predicate evaluates to `false`:

1. The runtime MUST trigger a **Panic** (`P-SEM-2850`)
2. The panic payload MUST include:
   - The predicate text (if available)
   - Source location of the contract
   - Whether it was a precondition or postcondition failure
3. Normal panic propagation rules apply (Clause 3, RAII destruction)

**Fact Synthesis After Runtime Check**

Successful execution of a runtime check synthesizes a Verification Fact per §11.5:

```cursive
[[dynamic]]
procedure process(x: i32)
    |= x > 0
{
    let y = x - 1
    if y > 0 { /* ... */ }
}
```

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                              | Detection    | Effect    |
| :----------- | :------- | :----------------------------------------------------- | :----------- | :-------- |
| `E-SEM-2801` | Error    | Contract not statically provable outside `[[dynamic]]` | Compile-time | Rejection |
| `P-SEM-2850` | Panic    | Runtime contract check failed                          | Runtime      | Panic     |

---

### 11.5 Verification Facts

##### Definition

A **Verification Fact** is a static guarantee that a predicate $P$ holds at program location $L$. Verification Facts are used for static analysis, contract elision, and type narrowing.

**Formal Definition**

A Verification Fact is a triple $F(P, L, S)$ where:
- $P$ is a predicate expression
- $L$ is a program location (CFG node)
- $S$ is the source of the fact (control flow, runtime check, or assumption)

##### Static Semantics

**Zero-Size Property**

Verification Facts:
- Have **zero runtime size**
- Have **no runtime representation**
- MUST NOT be stored in variables
- MUST NOT be passed as parameters
- MUST NOT be returned from procedures
- Exist solely for static analysis

**Fact Dominance**

A Verification Fact $F(P, L)$ satisfies a requirement for predicate $P$ at statement $S$ if and only if $L$ **strictly dominates** $S$ in the Control Flow Graph (CFG):

$$
\frac{
    F(P, L) \in \text{Facts} \quad
    L \text{ dom } S \quad
    L \neq S
}{
    P \text{ satisfied at } S
}
\tag{Fact-Dominate}
$$

**Fact Generation Rules**

The following constructs generate Verification Facts:

| Construct                    | Generated Fact                  | Location                |
| :--------------------------- | :------------------------------ | :---------------------- |
| `if P { ... }`               | $F(P, \_)$                      | Entry of then-branch    |
| `if !P { } else { ... }`     | $F(P, \_)$                      | Entry of else-branch    |
| `match x { Pat => ... }`     | $F(x \text{ matches } Pat, \_)$ | Entry of match arm      |
| Runtime check for $P$        | $F(P, \_)$                      | Immediately after check |
| Loop invariant $Inv$ at exit | $F(Inv, \_)$                    | Immediately after loop  |

**Type Narrowing**

When a Verification Fact $F(P, L)$ is active for binding $x$ at location $L$, the type of $x$ is effectively refined:

$$\text{typeof}(x) \xrightarrow{F(P, L)} \text{typeof}(x) \texttt{ where } \{P\}$$


##### Dynamic Semantics

**Dynamic Fact Injection**

When a `[[dynamic]]` scope requires a runtime check and no static fact dominates the check site:

1. Identify requirement $P$ at program point $S$
2. Construct check block $C$:
   ```cursive
   if !P { panic("Contract violation: {P}"); }
   ```
3. Insert $C$ into CFG such that $C$ dominates $S$
4. Successful execution of $C$ establishes $F(P, \text{exit}(C))$

---

### 11.6 Liskov Substitution Principle

##### Definition

When a type implements a class (Clause 10), procedure implementations MUST adhere to the **Liskov Substitution Principle** with respect to class-defined contracts.

##### Static Semantics

**Precondition Weakening**

An implementation MAY weaken (require less than) the preconditions defined in the class. An implementation MUST NOT strengthen (require more than) the preconditions. If the class says "caller must provide X", the implementation may accept X or something weaker (easier to satisfy).

**Postcondition Strengthening**

An implementation MAY strengthen (guarantee more than) the postconditions defined in the class. An implementation MUST NOT weaken (guarantee less than) the postconditions. If the class promises "callee will provide Y", the implementation must deliver Y or something stronger (more informative).

**Verification Strategy**

Behavioral subtyping constraints are verified **statically at compile-time**. The implementation performs the following checks when a type declares class implementation (`<:`):

1. **Precondition Check**: For each procedure in the class, verify that the implementation's precondition logically implies the class's precondition (i.e., the implementation accepts at least what the class requires).

2. **Postcondition Check**: For each procedure in the class, verify that the class's postcondition logically implies the implementation's postcondition (i.e., the implementation guarantees at least what the class promises).

These checks use the same proof infrastructure as standard static contract verification (§11.4). If the implication cannot be proven, the program is ill-formed. No runtime checks are generated for behavioral subtyping; violations are structural errors in the type definition.

```cursive
class Container {
    procedure get(~, idx: usize) -> i32
        |= idx < self.len() => @result != 0;
}

// Valid: Postcondition strengthened (guarantees more)
record MyList <: Container {
    data: [i32; 100],
    length: usize,

    procedure get(~, idx: usize) -> i32
        |= idx < self.len() => @result > 0  // > 0 implies != 0
    {
        self.data[idx]
    }
}

// Invalid: Postcondition weakened
record BadList <: Container {
    data: [i32; 100],
    length: usize,

    procedure get(~, idx: usize) -> i32
        |= idx < self.len()  // No guarantee about @result
    {
        self.data[idx]
    }  // Error: E-SEM-2804
}
```

##### Constraints & Legality

| Code         | Severity | Condition                                     | Detection    | Effect    |
| :----------- | :------- | :-------------------------------------------- | :----------- | :-------- |
| `E-SEM-2803` | Error    | Implementation strengthens form precondition. | Compile-time | Rejection |
| `E-SEM-2804` | Error    | Implementation weakens form postcondition.    | Compile-time | Rejection |

---

## Clause 12: Expressions & Statements

### 12.1 Foundational Definitions

##### Definition

**Expression**

An **expression** is a syntactic form that, when evaluated, produces a typed value. Every expression has:

1. **Type**: A compile-time type $T$ determined by static analysis.
2. **Value Category**: Classification as either a *place expression* or *value expression*.
3. **Evaluation Result**: A runtime value conforming to the expression's type.

**Statement**

A **statement** is a syntactic form that is executed for its side effects and does not produce a value. Statements form the executable body of procedures, loops, and other control-flow blocks.

**Statement Categories**

Statements are classified into the following categories:

1. **Declaration Statements**: Introduce new bindings (`let`, `var`). See §11.8.
2. **Assignment Statements**: Modify the value stored in a place. See §11.11.
3. **Expression Statements**: Execute an expression for its side effects. See §11.9.
4. **Control Flow Statements**: Alter the flow of execution (`return`, `result`, `break`, `continue`). See §11.10.
5. **Defer Statements**: Schedule cleanup code for scope exit. See §11.12.

**Typing Context**

Expression type judgments use the **typing context** $\Gamma$ as defined in §4.1. The judgment $\Gamma \vdash e : T$ asserts that expression $e$ has type $T$ under context $\Gamma$.


**Value Categories**

Every expression is classified into exactly one **value category** (terminology adapted from C++ lvalue/rvalue distinction):

| Category         | Notation    | Definition                                               |
| :--------------- | :---------- | :------------------------------------------------------- |
| Place Expression | $e^{place}$ | Denotes a memory location with a stable address          |
| Value Expression | $e^{value}$ | Produces a temporary value without a persistent location |

**Formal Classification Rules**

The value category of an expression is determined by its syntactic form:

| Expression Form                       | Value Category |
| :------------------------------------ | :------------- |
| Identifier bound by `let` or `var`    | Place          |
| Dereferenced pointer (`*p`)           | Place          |
| Field access on place (`place.field`) | Place          |
| Tuple access on place (`place.0`)     | Place          |
| Indexed access on place (`place[i]`)  | Place          |
| Literals                              | Value          |
| Arithmetic and logical results        | Value          |
| Procedure and closure call results    | Value          |
| Constructor expressions               | Value          |
| Block expression results              | Value          |

##### Static Semantics

**Evaluation Order**

Cursive's evaluation order MUST be deterministic. For any compound expression, subexpressions MUST be evaluated strictly left-to-right, with the following exception:

- Short-circuit operators (`&&`, `||`) conditionally skip evaluation of their right operand as specified in §11.4.6 (Binary Operators, Short-Circuit Evaluation).

**Sequential Statement Execution**

Statements within a block MUST be executed sequentially in source order. The effects of a statement MUST be fully complete before the next statement begins execution.

**Statement Termination**

Statement termination rules, including implicit newline termination and continuation conditions, are defined in §2.11.

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                                 | Detection    | Effect    |
| :----------- | :------- | :-------------------------------------------------------- | :----------- | :-------- |
| `E-SEM-2501` | Error    | Type mismatch: expected type differs from actual type.    | Compile-time | Rejection |
| `E-SEM-2502` | Error    | Value expression used where place expression is required. | Compile-time | Rejection |

---

### 12.2 Pattern Matching

##### Definition

**Pattern**

A **pattern** is a syntactic form that:

1. Tests whether a value has a particular shape (for refutable patterns), and
2. Binds parts of the value to identifiers.

**Irrefutable Pattern**

An **irrefutable pattern** is a pattern that matches any value of its expected type. Irrefutable patterns include:

- Identifier patterns (`x`)
- Wildcard patterns (`_`)
- Tuple patterns composed entirely of irrefutable sub-patterns
- Record patterns composed entirely of irrefutable sub-patterns
- Single-variant enum patterns

**Refutable Pattern**

A **refutable pattern** is a pattern that can fail to match a value of its expected type. Refutable patterns include:

- Literal patterns (`42`, `"hello"`, `true`)
- Multi-variant enum patterns
- Modal state patterns (`@State`)
- Range patterns (`0..10`)

**Scrutinee**

The **scrutinee** is the value being matched against in a `match` expression. In `match expr { ... }`, the scrutinee is `expr`. The scrutinee's type determines which patterns are valid and how pattern bindings receive their permissions.

**Pattern Binding Judgment**

The judgment $\Gamma \vdash p : T \Rightarrow \Gamma'$ asserts that pattern $p$ matches type $T$ and extends context $\Gamma$ with bindings to produce $\Gamma'$.

##### Syntax & Declaration

**Grammar**

```ebnf
pattern ::=
    literal_pattern
  | wildcard_pattern
  | identifier_pattern
  | tuple_pattern
  | record_pattern
  | enum_pattern
  | modal_pattern
  | range_pattern

literal_pattern     ::= integer_literal | float_literal | string_literal 
                      | char_literal | "true" | "false"

wildcard_pattern    ::= "_"

identifier_pattern  ::= identifier

tuple_pattern       ::= "(" tuple_pattern_elements? ")"
tuple_pattern_elements ::= pattern ";" | pattern ("," pattern)+

record_pattern      ::= type_path "{" [ field_pattern ( "," field_pattern )* ] "}"

field_pattern       ::= identifier [ ":" pattern ]

enum_pattern        ::= type_path "::" identifier [ enum_payload_pattern ]

enum_payload_pattern ::= "(" tuple_pattern_elements? ")"
                       | "{" [ field_pattern ( "," field_pattern )* ] "}"

modal_pattern       ::= "@" identifier [ "{" [ field_pattern ( "," field_pattern )* ","? ] "}" ]

range_pattern       ::= pattern ( ".." | "..=" ) pattern
```

##### Static Semantics

**Matching Rules**

1. **Literal Pattern**: Matches if `scrutinee == literal`. The literal type MUST be compatible with the scrutinee type.

2. **Wildcard Pattern (`_`)**: Matches any value. No binding is introduced. The matched value is neither moved nor bound; it remains in place.

3. **Identifier Pattern (`x`)**: Matches any value. The binding semantics depend on the **permission** of the scrutinee path, not the binding keyword (`let`/`var`):
   - **`unique` scrutinee**: The value is moved; responsibility transfers to the new binding. The original path becomes invalid.
   - **`shared` scrutinee**: The binding receives a `shared` view with synchronized access.
   - **`const` scrutinee**: The binding receives a `const` view. If the type implements `Copy`, the value is copied.

   The `let`/`var` keyword determines only whether the new binding is reassignable, not the transfer semantics.

4. **Tuple Pattern (`(p₁, p₂, ..., pₙ)`)**: Matches if the scrutinee is a tuple of arity $n$ and each component matches the corresponding sub-pattern. Sub-patterns are matched left-to-right.

5. **Record Pattern (`Type { f₁: p₁, ... }`)**: Matches if the scrutinee is of the named record type and each named field matches the corresponding sub-pattern. Field shorthand (`{ x }`) is equivalent to (`{ x: x }`). Fields not mentioned are ignored.

6. **Enum Pattern (`Enum::Variant(p...)`)**: Matches if the scrutinee's discriminant equals the specified variant. Payload fields are recursively matched against sub-patterns.

7. **Modal Pattern (`@State { ... }`)**: Matches if the scrutinee's active modal state is `@State`. State payload fields are recursively matched.

8. **Range Pattern**: Two forms exist:
   - **Exclusive (`start..end`)**: Matches if $\text{start} \le \text{scrutinee} < \text{end}$ (end-exclusive).
   - **Inclusive (`start..=end`)**: Matches if $\text{start} \le \text{scrutinee} \le \text{end}$ (end-inclusive).

   Both `start` and `end` MUST be compile-time constant expressions of the same ordered type as the scrutinee.

**Pattern Examples**

```cursive
// Range patterns (exclusive and inclusive)
match score {
    0..60 => "failing",       // 0 <= score < 60
    60..=100 => "passing",    // 60 <= score <= 100
    _ => "invalid",
}

// Modal pattern matching
match connection {
    @Disconnected { last_error } => handle_disconnect(last_error),
    @Connecting { host, started } => check_timeout(host, started),
    @Connected { socket, host } => process_data(socket, host),
}

// Record pattern with field shorthand
let point = Point { x: 10, y: 20 }
match point {
    Point { x, y: 0 } => handle_on_x_axis(x),  // y bound explicitly
    Point { x, y } => handle_general(x, y),     // shorthand: { x } means { x: x }
}
```

**Typing Rules**

**(T-Pat-Wildcard)**
$$\frac{}{\Gamma \vdash \_ : T \Rightarrow \Gamma}$$

**(T-Pat-Ident)**
$$\frac{x \notin \text{dom}(\Gamma)}{\Gamma \vdash x : T \Rightarrow \Gamma, x : T}$$

**(T-Pat-Tuple)**
$$\frac{\Gamma_0 = \Gamma \quad \forall i \in 1..n,\ \Gamma_{i-1} \vdash p_i : T_i \Rightarrow \Gamma_i}{\Gamma \vdash (p_1, \ldots, p_n) : (T_1, \ldots, T_n) \Rightarrow \Gamma_n}$$

**(T-Pat-Record)**
$$\frac{R = \texttt{record} \{ f_1 : T_1, \ldots, f_k : T_k \} \quad \Gamma_0 = \Gamma \quad \forall j \in J,\ \Gamma_{j-1} \vdash p_j : T_{f_j} \Rightarrow \Gamma_j}{\Gamma \vdash R \{ f_{j_1} : p_1, \ldots \} : R \Rightarrow \Gamma_{|J|}}$$

where $J \subseteq \{1, \ldots, k\}$ is the set of matched field indices.

**(T-Pat-Enum)**
$$\frac{E = \texttt{enum} \{ \ldots, V(T_1, \ldots, T_m), \ldots \} \quad \Gamma_0 = \Gamma \quad \forall i \in 1..m,\ \Gamma_{i-1} \vdash p_i : T_i \Rightarrow \Gamma_i}{\Gamma \vdash E::V(p_1, \ldots, p_m) : E \Rightarrow \Gamma_m}$$

**Pattern Binding Scopes**

- **Let/Var Statement**: Bindings are introduced into the enclosing scope. The pattern MUST be irrefutable.

- **Match Arm**: Bindings are introduced into an arm-local scope encompassing only the arm body. Shadowing of outer bindings is implicitly permitted within match arms.

**Exhaustiveness**

A `match` expression MUST be exhaustive. The set of patterns in its arms, taken together, MUST cover every possible value of the scrutinee type.

Exhaustiveness checking is MANDATORY for:

- `enum` types: All variants MUST be covered.
- `modal` types: All states MUST be covered.
- `bool` type: Both `true` and `false` MUST be covered.
- Integer types with range patterns: The union of ranges MUST cover all values, OR a wildcard/identifier pattern MUST be present.

**Exhaustiveness Algorithm**

Implementations MUST use a sound exhaustiveness algorithm. The algorithm:

1. Constructs the set of all possible value shapes for the scrutinee type.
2. For each pattern arm in order, removes the shapes covered by that pattern.
3. If any shapes remain uncovered after processing all arms, the match is non-exhaustive.

**Unreachability**

A match arm MUST NOT be unreachable. An arm is unreachable if its pattern covers only values already covered by preceding arms.

Implementations MUST detect unreachable arms and issue diagnostic `E-SEM-2751`.

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                                            | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------------------------------- | :----------- | :-------- |
| `E-SEM-2711` | Error    | Refutable pattern used in irrefutable context (e.g., `let` binding). | Compile-time | Rejection |
| `E-SEM-2712` | Error    | Pattern type incompatible with scrutinee type.                       | Compile-time | Rejection |
| `E-SEM-2713` | Error    | Duplicate binding identifier within a single pattern.                | Compile-time | Rejection |
| `E-SEM-2721` | Error    | Range pattern bounds are not compile-time constants.                 | Compile-time | Rejection |
| `E-SEM-2722` | Error    | Range pattern start exceeds end (empty range).                       | Compile-time | Rejection |
| `E-SEM-2731` | Error    | Record pattern references non-existent field.                        | Compile-time | Rejection |
| `E-SEM-2732` | Error    | Record pattern missing required field (when exhaustive matching).    | Compile-time | Rejection |
| `E-SEM-2741` | Error    | Match expression is not exhaustive for scrutinee type.               | Compile-time | Rejection |
| `E-SEM-2751` | Error    | Match arm is unreachable; pattern covered by preceding arms.         | Compile-time | Rejection |

---

### 12.3 Operator Precedence and Associativity

##### Definition

**Precedence**

**Operator precedence** is an integer ranking that determines how tightly an operator binds to its operands. Operators with higher precedence bind more tightly than operators with lower precedence.

**Associativity**

**Associativity** determines how operators of equal precedence are grouped when parentheses are absent:

- **Left-associative**: Groups left-to-right. `a op b op c` parses as `(a op b) op c`.
- **Right-associative**: Groups right-to-left. `a op b op c` parses as `a op (b op c)`.

##### Syntax & Declaration

##### Static Semantics

**Lexical Tokenization**

The normative rules for lexical tokenization of operators (Maximal Munch Rule, Generic Argument Exception) are defined in §2.7 (Operators and Punctuators).

**Operator Ambiguity Resolution**

Certain operators serve multiple roles depending on context:

| Operator | Unary Role (Precedence 3) | Binary Role              |
| :------- | :------------------------ | :----------------------- |
| `-`      | Numeric negation          | Subtraction (Prec. 7)    |
| `&`      | Address-of                | Bitwise AND (Prec. 9)    |
| `*`      | Dereference               | Multiplication (Prec. 6) |
| `^`      | Region allocation         | Bitwise XOR (Prec. 10)   |

Disambiguation is syntactic: if the operator appears after a complete operand expression, it is binary; otherwise, it is unary.

---

### 12.4 Primary Expressions and Operators

##### Definition

**Primary Expressions** are the fundamental operand forms from which all other expressions are built: literals, identifiers, parenthesized expressions, and constructor expressions.

**Postfix Expressions** extend primary expressions with operations that follow the operand: field access, indexing, procedure calls, and method dispatch.

**Unary Expressions** apply a prefix operator to a single operand.

**Binary Expressions** combine two operands with an infix operator.

##### Syntax & Declaration

**Grammar**

```ebnf
expression ::= assignment_expr

assignment_expr ::= logical_or_expr [ assignment_operator assignment_expr ]

logical_or_expr ::= logical_and_expr ( "||" logical_and_expr )*

logical_and_expr ::= comparison_expr ( "&&" comparison_expr )*

comparison_expr ::= bitor_expr ( comparison_operator bitor_expr )*

bitor_expr ::= bitxor_expr ( "|" bitxor_expr )*

bitxor_expr ::= bitand_expr ( "^" bitand_expr )*

bitand_expr ::= shift_expr ( "&" shift_expr )*

shift_expr ::= additive_expr ( shift_operator additive_expr )*

additive_expr ::= multiplicative_expr ( additive_operator multiplicative_expr )*

multiplicative_expr ::= power_expr ( multiplicative_operator power_expr )*

power_expr ::= cast_expr [ "**" power_expr ]

cast_expr ::= unary_expr [ "as" type ]

unary_expr ::= unary_operator unary_expr | postfix_expr

postfix_expr ::= primary_expr postfix_operation*

postfix_operation ::=
    "." identifier
  | "." integer_literal
  | "[" expression "]"
  | "(" [ argument_list ] ")"
  | "~>" identifier "(" [ argument_list ] ")"
  | "=>" expression

primary_expr ::=
    literal
  | identifier
  | "(" expression ")"
  | tuple_expr
  | array_expr
  | block_expr
  | if_expr
  | match_expr
  | loop_expr
  | closure_expr

argument_list ::= argument ( "," argument )* ","?

argument ::= [ "move" ] expression

assignment_operator ::= "=" | "+=" | "-=" | "*=" | "/=" | "%=" 
                      | "&=" | "|=" | "^=" | "<<=" | ">>="

comparison_operator ::= "==" | "!=" | "<" | "<=" | ">" | ">="

shift_operator ::= "<<" | ">>"

additive_operator ::= "+" | "-"

multiplicative_operator ::= "*" | "/" | "%"

unary_operator ::= "!" | "-" | "&" | "*" | "^" | "move" | "widen"
```

##### Static Semantics

**Literals**

A **literal** (integer, float, string, character, or boolean) is a primary expression whose type is determined by Clause 2, §2.8 (lexical form) and Clause 5 (type inference and suffixes). String literals are typed as `string@View` per the authoritative definition in §6.2 (String Types). Literals are value expressions.

**Identifiers**

An **identifier** in expression position MUST resolve to a value binding according to the name resolution rules in Clause 8. If the identifier resolves to a type, module, or form (rather than a value), the program is ill-formed.

**(T-Ident)** Identifier Typing:
$$\frac{(x : T) \in \Gamma}{\Gamma \vdash x : T}$$

An identifier expression is a place expression if it refers to a `let` or `var` binding, and a value expression otherwise.

**Parenthesized Expressions**

A **parenthesized expression** `(e)` has the same type, value, and value category as the enclosed expression `e`. Parentheses serve only to override precedence; they introduce no semantic effect.

**Constructor Expressions**

Constructor expressions for composite types (tuples, arrays, records, and enum variants) are defined in Clause 5:

| Constructor Form               | Authoritative Section |
| :----------------------------- | :-------------------- |
| Tuple: `(e₁, e₂, ..., eₙ)`     | §5.2.1                |
| Array: `[e₁, e₂, ..., eₙ]`     | §5.2.2                |
| Record: `Type { f₁: e₁, ... }` | §5.3                  |
| Enum: `Enum::Variant(e₁, ...)` | §5.4                  |

This clause does not redefine constructor expression syntax or semantics; see the authoritative sections.

**Range Expressions**

Range expressions (e.g., `start..end`, `start..=end`, `start..`, `..end`, `..=end`, `..`) produce intrinsic range types. The syntax, typing rules, and semantics of range expressions are authoritatively defined in §5.2.4 (Range Types).

> **Cross-Reference:** See §5.2.4 for the complete grammar of range expressions and their associated types (`Range<T>`, `RangeInclusive<T>`, `RangeFrom<T>`, `RangeTo<T>`, `RangeToInclusive<T>`, `RangeFull`).

---

#### 11.4.1 Field and Tuple Access

##### Definition

**Field access** retrieves a named field from a record value. **Tuple access** retrieves an element from a tuple by positional index.

##### Syntax & Declaration

```ebnf
field_access ::= postfix_expr "." identifier
tuple_access ::= postfix_expr "." integer_literal
```

##### Static Semantics

**(T-Field)** Field Access Typing:
$$\frac{\Gamma \vdash e : R \quad R = \texttt{record} \{ \ldots, f : T, \ldots \} \quad f \text{ visible}}{\Gamma \vdash e.f : T}$$

The field `f` MUST be:
1. A declared field of record type $R$.
2. Visible in the current scope per the visibility rules of Clause 8.

**(T-Tuple-Index)** Tuple Access Typing:
$$\frac{\Gamma \vdash e : (T_0, T_1, \ldots, T_{n-1}) \quad 0 \le i < n}{\Gamma \vdash e.i : T_i}$$

The index MUST be:
1. A non-negative decimal integer literal (not a variable or computed expression).
2. Within bounds: $0 \le i < n$ where $n$ is the tuple arity.

**Value Category**

Value category propagates per §11.1: field/tuple access on a place expression yields a place expression; on a value expression, a value expression.

##### Constraints & Legality

| Code         | Severity | Condition                                         | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------------ | :----------- | :-------- |
| `E-SEM-2511` | Error    | Identifier resolves to type or module, not value. | Compile-time | Rejection |
| `E-SEM-2521` | Error    | Field access on non-record type.                  | Compile-time | Rejection |
| `E-SEM-2522` | Error    | Field does not exist in record type.              | Compile-time | Rejection |
| `E-SEM-2523` | Error    | Field is not visible in current scope.            | Compile-time | Rejection |
| `E-SEM-2524` | Error    | Tuple access on non-tuple type.                   | Compile-time | Rejection |
| `E-SEM-2525` | Error    | Tuple index out of bounds.                        | Compile-time | Rejection |
| `E-SEM-2526` | Error    | Tuple index is not a constant integer literal.    | Compile-time | Rejection |

---

#### 11.4.2 Indexing

##### Definition

An **indexing expression** retrieves an element from an array or slice by computed index.

##### Syntax & Declaration

```ebnf
index_expr ::= postfix_expr "[" expression "]"
```

##### Static Semantics

**(T-Index-Array)** Array Indexing:
$$\frac{\Gamma \vdash e_1 : [T; N] \quad \Gamma \vdash e_2 : \texttt{usize}}{\Gamma \vdash e_1[e_2] : T}$$

**(T-Index-Slice)** Slice Indexing:
$$\frac{\Gamma \vdash e_1 : [T] \quad \Gamma \vdash e_2 : \texttt{usize}}{\Gamma \vdash e_1[e_2] : T}$$

The index expression MUST have type `usize`. No implicit integer conversions are performed.

> **Cross-Reference:** Range indexing (slicing) for arrays and slices is defined in §5.2.3 (T-Slice-From-Array). For string types, indexing and slicing have distinct semantics: direct byte indexing is prohibited (E-TYP-2152), and range slicing produces `string@View` with UTF-8 boundary validation—see §6.2 for the authoritative rules (T-String-Slice).

**Value Category**

Value category propagates per §11.1: indexing a place expression yields a place expression (enabling element assignment); indexing a value expression yields a value expression.

##### Dynamic Semantics

**Bounds Checking**

All indexing operations MUST be bounds-checked at runtime. If $\text{index} \ge \text{length}$, the executing thread MUST panic with diagnostic `P-SEM-2530`.

**Relationship to `[[dynamic]]`**

Bounds checking is a **safety mechanism** and is not affected by the `[[dynamic]]` attribute. The `[[dynamic]]` attribute controls:
- Key synchronization for `shared` data
- Contract verification
- Refinement type validation

Bounds checks are always performed (with static elision when provable) regardless of `[[dynamic]]` status.

##### Constraints & Legality

| Code         | Severity | Condition                                | Detection    | Effect    |
| :----------- | :------- | :--------------------------------------- | :----------- | :-------- |
| `E-SEM-2527` | Error    | Indexing applied to non-indexable type.  | Compile-time | Rejection |
| `E-SEM-2528` | Error    | Index expression is not of type `usize`. | Compile-time | Rejection |
| `P-SEM-2530` | Panic    | Index out of bounds at runtime.          | Runtime      | Panic     |

---

#### 11.4.3 Procedure and Method Calls

##### Definition

A **procedure call** invokes a callable value with a list of arguments. A **method call** invokes a procedure associated with a type, using either instance dispatch (`~>`) or static/qualified dispatch (`::`).

##### Syntax & Declaration

```ebnf
call_expr ::= postfix_expr "(" [ argument_list ] ")"

method_call ::= postfix_expr "~>" identifier "(" [ argument_list ] ")"

static_call ::= type_path "::" identifier "(" [ argument_list ] ")"

argument_list ::= argument ( "," argument )* ","?

argument ::= [ "move" ] expression
```

##### Static Semantics

**(T-Call)** Procedure Call Typing:
$$\frac{\Gamma \vdash f : (m_1\ T_1, \ldots, m_n\ T_n) \to R \quad \forall i \in 1..n,\ \Gamma \vdash a_i : T_i}{\Gamma \vdash f(a_1, \ldots, a_n) : R}$$

**Argument Passing Rules**

1. The number of arguments MUST equal the number of parameters.
2. Each argument type MUST be compatible with the corresponding parameter type per the subtyping rules of Clause 4.
3. If a parameter has the `move` modifier, the corresponding argument MUST be passed via an explicit `move` expression. After the call, the source binding enters the Moved state (§3.5).
4. Arguments are evaluated left-to-right before control transfers to the callee.

**Instance Method Dispatch (`~>`)**

The receiver dispatch operator `~>` invokes a method on a value instance:

```cursive
receiver~>method_name(args)
```

**(T-Method-Instance)** Instance Method Typing:
$$\frac{\Gamma \vdash r : T \quad \text{method } m(self : T', \ldots) \to R \in \text{methods}(T) \quad T <: T'}{\Gamma \vdash r \mathord{\sim>} m(\ldots) : R}$$

**Receiver Dispatch Algorithm**

1. Search for method `m` in the inherent methods of the receiver's type $T$.
2. If not found, search for method `m` in all forms implemented by $T$ that are visible in the current scope.
3. If multiple forms provide method `m`, the call is ambiguous. Disambiguation via qualified syntax `Form::m(receiver, ...)` is required.
4. **Strict Receiver Matching**: The receiver type MUST match the method's `self` parameter type exactly. Implicit dereference (e.g., automatically dereferencing `&T` to call a method expecting `T`) and implicit reference creation (e.g., automatically inserting `&` to match a `const Self` parameter) are NOT performed.

**Static/Qualified Dispatch (`::`)**

The scope resolution operator `::` invokes a method without an instance receiver, or disambiguates form methods:

```cursive
Type::method(args)           // Static method (no self parameter)
Form::method(receiver, ...) // Disambiguated form method
```

##### Constraints & Legality

| Code         | Severity | Condition                                             | Detection    | Effect    |
| :----------- | :------- | :---------------------------------------------------- | :----------- | :-------- |
| `E-SEM-2531` | Error    | Callee expression is not of callable type.            | Compile-time | Rejection |
| `E-SEM-2532` | Error    | Argument count mismatch.                              | Compile-time | Rejection |
| `E-SEM-2533` | Error    | Argument type incompatible with parameter type.       | Compile-time | Rejection |
| `E-SEM-2534` | Error    | `move` argument required but not provided.            | Compile-time | Rejection |
| `E-SEM-2535` | Error    | `move` argument provided but parameter is not `move`. | Compile-time | Rejection |
| `E-SEM-2536` | Error    | Method not found for receiver type.                   | Compile-time | Rejection |
| `E-SEM-2537` | Error    | Method call using `.` instead of `~>`.                | Compile-time | Rejection |
| `E-MOD-1305` | Error    | Ambiguous method resolution; disambiguation required. | Compile-time | Rejection |

> **Clarification (E-SEM-2537):** The `.` operator is valid for field access; the `~>` operator is required for method dispatch. Chained expressions such as `obj.field~>method()` are valid: `.` accesses the field, then `~>` dispatches the method on the field's value.

---

#### 11.4.4 Pipeline Expressions

##### Definition

A **pipeline expression** provides left-to-right function composition syntax, where the left operand becomes the first argument to the right operand.

##### Syntax & Declaration

```ebnf
pipeline_expr ::= postfix_expr "=>" expression
```

##### Static Semantics

**Desugaring**

The pipeline expression `x => f` desugars to `f(x)` before type checking. The desugaring is:

$$e_1 \Rightarrow e_2 \equiv e_2(e_1)$$

The right-hand expression MUST evaluate to a callable that accepts at least one argument, where the first parameter type is compatible with the left-hand expression type.

**(T-Pipeline)**
$$\frac{\Gamma \vdash e_1 : T_1 \quad \Gamma \vdash e_2 : (T_1, \ldots) \to R}{\Gamma \vdash e_1 \Rightarrow e_2 : R}$$

**Chaining**

Pipelines are left-associative and may be chained:

```cursive
x => f => g => h   // Equivalent to: h(g(f(x)))
```

##### Constraints & Legality

| Code         | Severity | Condition                                    | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------- | :----------- | :-------- |
| `E-SEM-2538` | Error    | Right-hand side of pipeline is not callable. | Compile-time | Rejection |
| `E-SEM-2539` | Error    | Pipeline target has no parameters.           | Compile-time | Rejection |

---

#### 11.4.5 Unary Operators

##### Definition

Unary operators apply a single-operand operation. All unary operators have precedence 3 and are right-associative.

##### Syntax & Declaration

```ebnf
unary_expr ::= unary_operator unary_expr | postfix_expr

unary_operator ::= "!" | "-" | "&" | "*" | "^" | "move" | "widen"
```

##### Static Semantics

**Logical Negation (`!`)**

**(T-Not-Bool)**
$$\frac{\Gamma \vdash e : \texttt{bool}}{\Gamma \vdash !e : \texttt{bool}}$$

**(T-Not-Int)** Bitwise complement on integers:
$$\frac{\Gamma \vdash e : I \quad I \in \mathcal{T}_{\text{int}}}{\Gamma \vdash !e : I}$$

**Numeric Negation (`-`)**

**(T-Neg)**
$$\frac{\Gamma \vdash e : N \quad N \in \mathcal{T}_{\text{signed}} \cup \mathcal{T}_{\text{float}}}{\Gamma \vdash -e : N}$$

Negation is defined for signed integer types and floating-point types. Negation of unsigned integers is ill-formed.

**Address-Of (`&`)**

The `&` operator takes the address of a place expression, producing a safe pointer in the `@Valid` modal state. The authoritative typing rule **(T-Addr-Of)** is defined in §6.3 (Pointer Types).

> **Cross-Reference:** See §6.3 for the normative typing judgment, place expression requirements, and the complete modal pointer state model.

The permission of the resulting pointer is determined by the permission of the source place (§4.5).

**Dereference (`*`)**

The `*` operator dereferences a pointer to access the value it points to. The authoritative typing rule **(T-Deref)** for safe pointers is defined in §6.3 (Pointer Types).

> **Cross-Reference:** See §6.3 for the normative typing judgment requiring `@Valid` state for safe pointer dereference, and **(T-Raw-Deref)** for raw pointer dereference rules.

Dereferencing a raw pointer (`*imm T` or `*mut T`) is permitted only within an `unsafe` block (§3.8).

**Region Allocation (`^`)**

The `^` operator allocates the operand value into an active region. The authoritative typing rules **(T-Alloc-Named)** and **(T-Alloc-Implicit)** are defined in §3.7 (Regions).

> **Cross-Reference:** See §3.7 for the normative typing judgments distinguishing named region allocation (`r^e`) from implicit allocation (`^e`), provenance assignment, and escape analysis rules.

**Move (`move`)**

The `move` operator transfers responsibility for a value from its source binding. The typing rule **(T-Move)** and full semantics are authoritatively defined in §3.5 (Responsibility and Move Semantics).

> **Cross-Reference:** See §3.5 for the normative typing judgment, post-move state tracking, and parameter responsibility rules.

**Modal Widening (`widen`)**

The `widen` operator converts a state-specific modal type `M@S` to the general modal type `M`. The authoritative typing rule **(T-Modal-Widen)** is defined in §6.1 (Modal Types).

> **Cross-Reference:** See §6.1 for the normative typing judgment, storage cost implications, and conditions under which explicit `widen` is required versus implicit coercion.

##### Constraints & Legality

| Code         | Severity | Condition                                          | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------------- | :----------- | :-------- |
| `E-SEM-2541` | Error    | Logical `!` applied to non-bool, non-integer type. | Compile-time | Rejection |
| `E-SEM-2542` | Error    | Numeric negation of unsigned integer.              | Compile-time | Rejection |
| `E-SEM-2543` | Error    | Address-of `&` applied to value expression.        | Compile-time | Rejection |
| `E-SEM-2544` | Error    | Dereference of pointer in `@Null` state.           | Compile-time | Rejection |
| `E-SEM-2545` | Error    | Raw pointer dereference outside `unsafe` block.    | Compile-time | Rejection |
| `E-MEM-3021` | Error    | Region allocation `^` outside region scope.        | Compile-time | Rejection |
| `E-MEM-3001` | Error    | Move from binding in Moved state.                  | Compile-time | Rejection |
| `E-MEM-3006` | Error    | Move from immovable binding (`:=`).                | Compile-time | Rejection |

---

#### 11.4.6 Binary Operators

##### Definition

Binary operators combine two operands with an infix operator. This section covers arithmetic, bitwise, comparison, and logical operators.

##### Static Semantics

**Arithmetic Operators (`+`, `-`, `*`, `/`, `%`, `**`)**

**(T-Arith)**
$$\frac{\Gamma \vdash e_1 : N \quad \Gamma \vdash e_2 : N \quad N \in \mathcal{T}_{\text{numeric}}}{\Gamma \vdash e_1 \oplus e_2 : N}$$

where $\oplus \in \{+, -, *, /, \%, **\}$.

Arithmetic operators require homogeneous operand types. No implicit numeric promotions or conversions are performed. If operand types differ, the program is ill-formed.

**Bitwise Operators (`&`, `|`, `^`, `<<`, `>>`)**

**(T-Bitwise)**
$$\frac{\Gamma \vdash e_1 : I \quad \Gamma \vdash e_2 : I \quad I \in \mathcal{T}_{\text{int}}}{\Gamma \vdash e_1 \oplus e_2 : I}$$

where $\oplus \in \{\&, |, \texttt{\^{}}\}$.

**(T-Shift)**
$$\frac{\Gamma \vdash e_1 : I \quad \Gamma \vdash e_2 : \texttt{u32} \quad I \in \mathcal{T}_{\text{int}}}{\Gamma \vdash e_1 \ll e_2 : I} \quad \frac{\Gamma \vdash e_1 : I \quad \Gamma \vdash e_2 : \texttt{u32} \quad I \in \mathcal{T}_{\text{int}}}{\Gamma \vdash e_1 \gg e_2 : I}$$

Shift operators require the right operand to be `u32`. The result type is the type of the left operand.

**Comparison Operators (`==`, `!=`, `<`, `<=`, `>`, `>=`)**

**(T-Compare)**
$$\frac{\Gamma \vdash e_1 : T \quad \Gamma \vdash e_2 : T}{\Gamma \vdash e_1\ \text{cmp}\ e_2 : \texttt{bool}}$$

Comparison operators require homogeneous operand types. The result type is always `bool`.

Equality operators (`==`, `!=`) are defined for all types implementing the `Eq` class.

Ordering operators (`<`, `<=`, `>`, `>=`) are defined for types implementing the `Ord` class.

**Logical Operators (`&&`, `||`)**

**(T-Logic)**
$$\frac{\Gamma \vdash e_1 : \texttt{bool} \quad \Gamma \vdash e_2 : \texttt{bool}}{\Gamma \vdash e_1 \land\land e_2 : \texttt{bool}} \quad \frac{\Gamma \vdash e_1 : \texttt{bool} \quad \Gamma \vdash e_2 : \texttt{bool}}{\Gamma \vdash e_1 \lor\lor e_2 : \texttt{bool}}$$

##### Dynamic Semantics

**Short-Circuit Evaluation**

Logical operators MUST implement short-circuit evaluation:

- `e₁ && e₂`: If `e₁` evaluates to `false`, `e₂` is NOT evaluated; the result is `false`.
- `e₁ || e₂`: If `e₁` evaluates to `true`, `e₂` is NOT evaluated; the result is `true`.

This is the exception to strict left-to-right evaluation within sequential code: the right operand may be skipped entirely. (Parallel blocks in Clause 15 introduce concurrent evaluation, which is a different evaluation model.)

**Integer Overflow**

Integer arithmetic operations (`+`, `-`, `*`) MUST panic on overflow in **debug mode**. Behavior in **release mode** is Implementation-Defined (IDB); implementations MUST document whether overflow wraps or panics.

> **Cross-Reference:** See §4.1.3 (Integer Overflow) for the authoritative definition of overflow behavior, including the full set of permitted release-mode behaviors.

**Division and Remainder**

Division by zero MUST cause the executing thread to panic.

For signed integer division, the result is truncated toward zero. The remainder operation `a % b` satisfies the identity: `a == (a / b) * b + (a % b)`.

**Shift Operations**

If the right operand of a shift operation is greater than or equal to the bit width of the left operand type, the behavior is Implementation-Defined. Implementations MUST document whether this condition panics or produces a defined result.

##### Constraints & Legality

| Code         | Severity | Condition                                         | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------------ | :----------- | :-------- |
| `E-SEM-2551` | Error    | Arithmetic operator applied to non-numeric types. | Compile-time | Rejection |
| `E-SEM-2552` | Error    | Mismatched operand types for arithmetic operator. | Compile-time | Rejection |
| `E-SEM-2553` | Error    | Bitwise operator applied to non-integer types.    | Compile-time | Rejection |
| `E-SEM-2554` | Error    | Comparison of incompatible types.                 | Compile-time | Rejection |
| `E-SEM-2555` | Error    | Logical operator applied to non-bool operands.    | Compile-time | Rejection |
| `E-SEM-2556` | Error    | Shift amount is not of type `u32`.                | Compile-time | Rejection |
| `P-SEM-2560` | Panic    | Integer overflow in debug mode.                   | Runtime      | Panic     |
| `P-SEM-2561` | Panic    | Division by zero.                                 | Runtime      | Panic     |
| `I-SEM-2562` | IDB      | Shift amount ≥ bit width of left operand.         | Runtime      | IDB       |

---

#### 11.4.7 Cast Expressions

##### Definition

A **cast expression** explicitly converts a value from one type to another using the `as` operator.

##### Syntax & Declaration

```ebnf
cast_expr ::= unary_expr "as" type
```

The `as` operator has precedence 4 (between unary operators and exponentiation) and is left-associative. Multiple casts may be chained: `x as i32 as f64` parses as `(x as i32) as f64`.

##### Static Semantics

**(T-Cast)**
$$\frac{\Gamma \vdash e : S \quad \text{CastValid}(S, T)}{\Gamma \vdash e\ \texttt{as}\ T : T}$$

A cast is well-formed if and only if the source-target type pair is in one of the categories defined below. All other combinations are ill-formed.


**Cast Categories**

| Category              | Source Type         | Target Type         | Semantics                   |
| :-------------------- | :------------------ | :------------------ | :-------------------------- |
| **Numeric Widening**  | `iN`                | `iM` where `M > N`  | Sign-extended               |
|                       | `uN`                | `uM` where `M > N`  | Zero-extended               |
|                       | `fN`                | `fM` where `M > N`  | Precision-extended          |
| **Numeric Narrowing** | `iN`                | `iM` where `M < N`  | Truncated (low-order bits)  |
|                       | `uN`                | `uM` where `M < N`  | Truncated (low-order bits)  |
|                       | `fN`                | `fM` where `M < N`  | Rounded to nearest          |
| **Sign Conversion**   | `iN`                | `uN`                | Bit reinterpretation        |
|                       | `uN`                | `iN`                | Bit reinterpretation        |
| **Integer ↔ Float**   | `iN` / `uN`         | `fM`                | Nearest representable value |
|                       | `fM`                | `iN` / `uN`         | Truncate toward zero†       |
| **Pointer ↔ Integer** | `usize`             | `*imm T` / `*mut T` | Address interpretation‡     |
|                       | `*imm T` / `*mut T` | `usize`             | Address extraction‡         |
| **Safe → Raw Ptr**    | `Ptr<T>@Valid`      | `*imm T` / `*mut T` | Extract address‡‡           |
| **Enum → Integer**    | `enum E`            | Integer type        | Discriminant value§         |
| **Bool → Integer**    | `bool`              | Any integer         | `false`→0, `true`→1         |
| **Char ↔ Integer**    | `char`              | `u32`               | Unicode scalar value        |
|                       | `u32`               | `char`              | Validated conversion††      |
|                       | `char`              | `u8`                | Validated conversion††      |
|                       | `u8`                | `char`              | Always valid (ASCII subset) |

† Float-to-integer casts that overflow or produce NaN trigger a panic.

‡ Pointer-integer casts are permitted only within `unsafe` blocks. See §6.3 for the authoritative typing rules (T-Raw-Ptr-Cast-Imm, T-Raw-Ptr-Cast-Mut).

§ Enum-to-integer casts are valid only when the enum has an explicit `[[layout(IntType)]]` attribute (see §7.2.1).

†† Panics if the value is not a valid Unicode scalar value or exceeds the target range.

‡‡ See §6.3 for the authoritative typing rules (T-Raw-Ptr-Cast-Imm, T-Raw-Ptr-Cast-Mut).

Any cast not listed in the Cast Categories table above is ill-formed.

##### Dynamic Semantics

**Widening Casts**

Widening casts are always lossless:
- Signed integers: Sign-extended to fill additional bits.
- Unsigned integers: Zero-extended to fill additional bits.
- Floats: Exactly represented in the larger format.

**Narrowing Casts**

Narrowing casts may lose information:
- Integer narrowing: Low-order bits are preserved; high-order bits are discarded. No panic occurs; information loss is silent.
- Float narrowing: Rounded to nearest representable value per IEEE 754.

**Float-to-Integer Casts**

Float-to-integer casts truncate toward zero. The following conditions trigger a panic:

| Condition                  | Description            |
| :------------------------- | :--------------------- |
| Value exceeds target range | `1e20_f64 as i32`      |
| Value is NaN               | `f64::NAN as i32`      |
| Value is ±Infinity         | `f64::INFINITY as i32` |

**Integer-to-Float Casts**

Large integers that cannot be exactly represented are rounded to the nearest representable value per IEEE 754 "round to nearest, ties to even."

##### Constraints & Legality

| Code         | Severity | Condition                                          | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------------- | :----------- | :-------- |
| `E-SEM-2571` | Error    | Cast between incompatible types.                   | Compile-time | Rejection |
| `E-SEM-2572` | Error    | Pointer-integer cast outside `unsafe` block.       | Compile-time | Rejection |
| `E-SEM-2573` | Error    | Enum cast without `[[layout(IntType)]]` attribute. | Compile-time | Rejection |
| `E-SEM-2574` | Error    | Cast of non-`@Valid` pointer to raw pointer.       | Compile-time | Rejection |
| `P-SEM-2580` | Panic    | Float-to-integer cast overflow or NaN.             | Runtime      | Panic     |
| `P-SEM-2581` | Panic    | `u32 as char` with invalid Unicode scalar value.   | Runtime      | Panic     |
| `P-SEM-2582` | Panic    | `char as u8` with value > 255.                     | Runtime      | Panic     |

---

### 12.5 Closure Expressions

##### Definition

A **closure expression** creates an anonymous callable value. Closures may capture bindings from their enclosing lexical scope.

**Capturing Closure**

A closure that references bindings from its enclosing scope is a **capturing closure**. The captured bindings form the closure's **environment**.

**Non-Capturing Closure**

A closure that references no bindings from its enclosing scope is a **non-capturing closure**. Non-capturing closures have sparse function pointer types and are FFI-compatible.

##### Syntax & Declaration

**Grammar**

```ebnf
closure_expr ::= "|" [ closure_param_list ] "|" [ "->" type ] closure_body

closure_param_list ::= closure_param ( "," closure_param )* ","?

closure_param ::= [ "move" ] identifier [ ":" type ]

closure_body ::= expression | block_expr
```

**Syntactic Notes**

1. The `|` delimiters distinguish closure expressions from other uses of `|` (bitwise OR). Context disambiguates: `|` at expression start, followed by an optional parameter list and another `|`, forms a closure. For example, `|x| x + 1` is a closure, while `a | b` is bitwise OR. The ambiguity `| x |` at statement start is resolved by requiring closure parameters: `|x|` with no spaces is a closure with parameter `x`, whereas `| x |` in expression context after an operand is parsed as `(operand) | x |` (two bitwise OR operations).

2. Parameter types MAY be omitted when the closure is checked against a known function type (see Bidirectional Type Checking).

3. The return type MAY be omitted; it is inferred from the closure body.

4. The closure body is either a single expression (for simple closures) or a block expression (for multi-statement closures).

##### Static Semantics

**Closure Typing**

Closure expressions have function types (§6.4). Non-capturing closures have sparse function pointer types; capturing closures have closure types.

**Parameter Typing**

1. If a parameter has an explicit type annotation, that type is used.
2. If a parameter lacks a type annotation and the closure is checked against an expected function type, the parameter type is inferred from the expected type.
3. If a parameter lacks a type annotation and no expected type is available, the program is ill-formed.

**Return Type Inference**

1. If the return type is explicitly annotated, the body is checked against that type.
2. If the return type is omitted, it is inferred from the body expression type.

**Capture Semantics**

Closures capture bindings from their enclosing scope according to the following rules:

| Binding Permission | Capture Mode  | Closure Environment Contains        |
| :----------------- | :------------ | :---------------------------------- |
| `const`            | By reference  | `const` reference to binding        |
| `unique`           | Move required | Owned value (binding becomes Moved) |
| `shared`           | By reference  | `shared` reference to binding       |

**Move Captures**

A binding with `unique` permission MUST be captured via an explicit `move` in the closure parameter list or by using `move` when referencing the binding in the closure body:

```cursive
let buffer: unique Buffer = ...
let closure = |x| process(move buffer, x)  // 'buffer' moved into closure
// 'buffer' is now in Moved state
```

**Capture Analysis**

Capture analysis MUST be performed to determine which bindings are referenced by the closure body. A binding is captured if:

1. It is referenced by name within the closure body, AND
2. It is not a parameter of the closure, AND
3. It is not shadowed by a local binding within the closure body.

**Closure Subtyping**

Per §6.4 (T-Sparse-Sub-Closure), sparse function pointers are subtypes of the corresponding closure types. A non-capturing closure may be used where a capturing closure is expected.

##### Dynamic Semantics

**Closure Construction**

When a closure expression is evaluated:

1. All captured bindings are evaluated and stored in the closure's environment.
2. For `const` and `shared` captures, references to the original bindings are stored.
3. For `move` captures, the values are moved into the closure environment; the source bindings become invalid.
4. A closure value is constructed containing the code pointer and environment pointer.

**Closure Invocation**

When a closure is invoked:

1. Arguments are bound to parameters.
2. The closure body is evaluated with access to both parameters and captured environment.
3. The result is returned to the caller.

**Environment Lifetime**

The closure's environment has a lifetime tied to the closure value. When the closure value is dropped, captured owned values are destroyed.

##### Constraints & Legality

| Code         | Severity | Condition                                   | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------ | :----------- | :-------- |
| `E-SEM-2591` | Error    | Closure parameter type cannot be inferred.  | Compile-time | Rejection |
| `E-SEM-2593` | Error    | Capture of `unique` binding without `move`. | Compile-time | Rejection |
| `E-SEM-2594` | Error    | Closure return type mismatch.               | Compile-time | Rejection |

---

### 12.6 Control Flow Expressions

##### Definition

Control flow expressions alter the sequential flow of evaluation and may produce values.

**Conditional Expression (`if`)**: Evaluates one of two branches based on a boolean condition.

**Match Expression**: Performs exhaustive pattern matching on a scrutinee value.

**Loop Expression**: Iterates until a termination condition is met.

##### Syntax & Declaration

**Grammar**

```ebnf
if_expr ::= "if" expression block_expr [ "else" ( block_expr | if_expr ) ]

match_expr ::= "match" expression "{" match_arm+ "}"

match_arm ::= pattern [ "if" expression ] "=>" arm_body ","

arm_body ::= expression | block_expr

loop_expr ::= [ label ] loop_kind

label ::= "'" identifier ":"

loop_kind ::=
    "loop" block_expr
  | "loop" expression block_expr
  | "loop" pattern [ ":" type ] "in" expression block_expr
```

##### Static Semantics

**Conditional Expression (`if`)**

**(T-If)**
$$\frac{\Gamma \vdash e_c : \texttt{bool} \quad \Gamma \vdash e_t : T \quad \Gamma \vdash e_f : T}{\Gamma \vdash \texttt{if } e_c\ \{ e_t \}\ \texttt{else}\ \{ e_f \} : T}$$

Both branches MUST have the same type. The result type is the common branch type.

**(T-If-No-Else)**
$$\frac{\Gamma \vdash e_c : \texttt{bool} \quad \Gamma \vdash e_t : ()}{\Gamma \vdash \texttt{if } e_c\ \{ e_t \} : ()}$$

An `if` without `else` has type `()`. The then-branch MUST also have type `()`.

**Match Expression**

**(T-Match)**
$$\frac{\Gamma \vdash e : T_s \quad \forall i,\ \Gamma, \text{bindings}(p_i) \vdash e_i : T \quad \text{exhaustive}(\{p_i\}, T_s) \quad \text{reachable}(p_i)}{\Gamma \vdash \texttt{match } e\ \{ p_1 \Rightarrow e_1, \ldots \} : T}$$

1. All arm bodies MUST have the same type.
2. The pattern set MUST be exhaustive for the scrutinee type (§12.2).
3. No arm MUST be unreachable (§12.2).

**Reachability Predicate**: The predicate $\text{reachable}(p_i)$ holds when pattern $p_i$ can potentially match some value not already covered by preceding patterns $p_1, \ldots, p_{i-1}$. Formally, the arm is reachable if the value space covered by $p_i$ is not a subset of the union of value spaces covered by earlier patterns.

**Match Guards**

A match arm MAY include a guard clause `if guard_expr`. The guard:

1. MUST have type `bool`.
2. Is evaluated only if the pattern matches.
3. If the guard evaluates to `false`, matching continues with the next arm.
4. Guard expressions MAY reference bindings introduced by the pattern.
5. Guard expressions MUST NOT mutate pattern bindings derived from `const` or `shared` scrutinees. Bindings from `unique` scrutinees MAY be mutated if declared with `var`.

Guards do not affect exhaustiveness checking; patterns with guards are considered to cover their full range for exhaustiveness purposes.

**Loop Expressions**

**(T-Loop-Infinite)**
$$\frac{\Gamma \vdash e : T}{\Gamma \vdash \texttt{loop}\ \{ e \} : !}$$

An infinite loop with no `break` has type `!` (never). If a `break value` is present, the loop has the type of the break values.

**(T-Loop-Conditional)**
$$\frac{\Gamma \vdash e_c : \texttt{bool} \quad \Gamma \vdash e_b : ()}{\Gamma \vdash \texttt{loop } e_c\ \{ e_b \} : ()}$$

A conditional loop (`loop condition { ... }`) has type `()`.

**(T-Loop-Iterator)**
$$\frac{\Gamma \vdash e_{iter} : I \quad I : \texttt{Iterator}\langle\texttt{Item} = T\rangle \quad \Gamma, x : T \vdash e_b : ()}{\Gamma \vdash \texttt{loop } x : T\ \texttt{in } e_{iter}\ \{ e_b \} : ()}$$

An iterator loop has type `()`. The iterator expression MUST implement the `Iterator` class. When the type annotation is omitted, `T` is inferred from the iterator's `Item` associated type.

**Loop with Break Value**

When a loop contains `break value` statements, all such breaks for the same loop MUST provide values of the same type. That type becomes the loop's result type.

##### Dynamic Semantics

**Conditional Evaluation**

1. The condition expression is evaluated.
2. If true, the then-branch is evaluated and its value is the result.
3. If false and an else-branch exists, the else-branch is evaluated and its value is the result.
4. If false and no else-branch exists, the result is `()`.

**Match Evaluation**

1. The scrutinee expression is evaluated.
2. Patterns are tested in declaration order.
3. For each pattern that matches structurally, the guard (if any) is evaluated.
4. The first arm whose pattern matches and whose guard (if any) evaluates to `true` is selected.
5. Pattern bindings are introduced into scope, and the arm body is evaluated.
6. The arm body's value is the match result.

**Loop Desugaring**

Conditional loops desugar to infinite loops:

$$\texttt{loop}\ C\ \{\ B\ \} \quad \rightarrow \quad \texttt{loop}\ \{\ \texttt{if}\ !C\ \{\ \texttt{break}\ \}\ B\ \}$$

Iterator loops desugar to match on `Iterator::next`:

$$\texttt{loop}\ x: T\ \texttt{in}\ iter\ \{\ B\ \} \quad \rightarrow \quad \{\ \texttt{var}\ it = iter;\ \texttt{loop}\ \{\ \texttt{match}\ it\texttt{\sim>next()}\ \{\ \texttt{Some}(x) \Rightarrow B,\ \texttt{None} \Rightarrow \texttt{break}\ \}\ \}\ \}$$

The desugaring binds the iterator to a `var` binding because `Iterator::next(~!)` requires a mutable receiver. The iterator expression `iter` must therefore evaluate to a type implementing `Iterator`, and its `next` method mutates the iterator state.

**Iterator Loop Desugaring Example**

```cursive
// Source: Iterator loop
loop x in items {
    process(x)
}

// Desugared form:
{
    var it = items
    loop {
        match it~>next() {
            Some(x) => process(x),
            None => break,
        }
    }
}
```

##### Constraints & Legality

| Code         | Severity | Condition                                          | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------------- | :----------- | :-------- |
| `E-SEM-2601` | Error    | `if` condition is not of type `bool`.              | Compile-time | Rejection |
| `E-SEM-2602` | Error    | `if` branches have incompatible types.             | Compile-time | Rejection |
| `E-SEM-2603` | Error    | `if` without `else` used in non-unit context.      | Compile-time | Rejection |
| `E-SEM-2611` | Error    | Match arms have incompatible types.                | Compile-time | Rejection |
| `E-SEM-2612` | Error    | Match guard is not of type `bool`.                 | Compile-time | Rejection |
| `E-SEM-2621` | Error    | Iterator expression does not implement `Iterator`. | Compile-time | Rejection |
| `E-SEM-2622` | Error    | `break` values have incompatible types.            | Compile-time | Rejection |

---

### 12.7 Block Expressions

##### Definition

A **block expression** introduces a new lexical scope and evaluates to a value. Blocks are delimited by braces and contain zero or more statements followed by an optional result expression.

##### Syntax & Declaration

```ebnf
block_expr ::= "{" statement* [ expression ] "}"
```

##### Static Semantics

**Block Type**

The type of a block expression is determined by:

1. The type of an explicit `result` statement, if present, OR
2. The type of the final unterminated expression, if present, OR
3. The unit type `()` if the block is empty or ends with a terminated statement.

> **Cross-Reference:** See §2.11 (Statement Terminator) for the rules governing statement termination and the distinction between terminated statements and unterminated result expressions.

**(T-Block-Result)**
$$\frac{\Gamma \vdash s_1; \ldots; s_n \quad \Gamma \vdash e : T}{\Gamma \vdash \{ s_1; \ldots; s_n;\ e \} : T}$$

**(T-Block-Unit)**
$$\frac{\Gamma \vdash s_1; \ldots; s_n}{\Gamma \vdash \{ s_1; \ldots; s_n; \} : ()}$$

**Scope Rules**

1. Bindings introduced within a block are visible only within that block.
2. Bindings shadow outer bindings of the same name for the duration of the block.
3. When a block exits, all bindings introduced within it are destroyed in reverse declaration order.

##### Dynamic Semantics

1. Statements are executed sequentially in source order.
2. If a `result` statement is executed, the block immediately evaluates to that value.
3. If the block ends with an unterminated expression, that expression's value is the block result.
4. If the block ends with a terminated statement, the block result is `()`.
5. Upon block exit (normal or via control flow), destructors are invoked for all bindings in reverse declaration order.

**Specialized Block Forms**

The following block forms have specialized semantics defined in other clauses:

| Block Form              | Authoritative Section              |
| :---------------------- | :--------------------------------- |
| `region { ... }`        | §3.7 Regions                       |
| `unsafe { ... }`        | §3.8 Unsafe Memory Operations      |
| `parallel(...) { ... }` | Clause 15 (Structured Parallelism) |
| `comptime { ... }`      | Clause 17 (Compile-Time Execution) |

This clause does not redefine these block forms; see the authoritative sections.

---

### 12.8 Declaration Statements

##### Definition

A **declaration statement** introduces new bindings into the current scope.

##### Syntax & Declaration

```ebnf
decl_stmt ::= let_decl | var_decl

let_decl ::= "let" pattern [ ":" type ] binding_op expression

var_decl ::= "var" pattern [ ":" type ] binding_op expression
```

The `binding_op` production is defined in §3.4.

##### Static Semantics

**Binding Semantics**

The semantics of binding keywords (`let`/`var`) and binding operators (`=`/`:=`) are defined in §3.4 (The Binding Model). The key distinctions are:

- `let` vs. `var`: Determines mutability (whether the binding can be reassigned)
- `=` vs. `:=`: Determines movability (whether responsibility can be transferred via `move`)

These properties are orthogonal to each other and to permission types.

**Type Inference**

If the type annotation is omitted, the type is inferred from the initializer expression. If the type cannot be inferred, the program is ill-formed.

**Pattern Requirements**

The pattern in a declaration statement MUST be irrefutable (§12.2). Refutable patterns are diagnosed as errors.

##### Dynamic Semantics

1. The initializer expression is evaluated.
2. The value is bound to the pattern, introducing bindings into scope.
3. For destructuring patterns, components are bound to their respective identifiers.

##### Constraints & Legality

| Code         | Severity | Condition                                        | Detection    | Effect    |
| :----------- | :------- | :----------------------------------------------- | :----------- | :-------- |
| `E-MOD-2401` | Error    | Reassignment of immutable `let` binding.         | Compile-time | Rejection |
| `E-MOD-2402` | Error    | Type annotation incompatible with inferred type. | Compile-time | Rejection |

See §12.2 for refutable pattern diagnostics (`E-SEM-2711`).

---

### 12.9 Expression Statements

##### Definition

An **expression statement** evaluates an expression for its side effects. The resulting value is discarded.

##### Syntax & Declaration

```ebnf
expr_stmt ::= expression terminator

terminator ::= ";" | <newline>
```

##### Static Semantics

The type of an expression statement is the unit type `()`. The expression's value is discarded after evaluation.

##### Dynamic Semantics

1. The expression is evaluated.
2. If the expression produces a value, that value is discarded.
3. If the expression's result is a value with a destructor and the value is not moved elsewhere, the destructor is invoked at statement end.

---

### 12.10 Control Flow Statements

##### Definition

Control flow statements alter the normal sequential execution of statements.

##### Syntax & Declaration

```ebnf
return_stmt   ::= "return" [ expression ]

result_stmt   ::= "result" expression

break_stmt    ::= "break" [ "'" identifier ] [ expression ]

continue_stmt ::= "continue" [ "'" identifier ]
```

##### Static Semantics

**Return Statement**

`return` terminates the current procedure and returns control to the caller.

1. If an expression is provided, its type MUST match the procedure's declared return type.
2. If no expression is provided, the procedure's return type MUST be `()`.
3. `return` MUST NOT appear at module scope.

**Result Statement**

`result` terminates the current block and yields a value as the block's result.

1. The expression type becomes (or must match) the enclosing block's type.
2. `result` exits only the immediately enclosing block, not the procedure. This differs from `return`, which exits the entire procedure.
3. `result` is the primary mechanism for yielding values from blocks.

```cursive
// result vs return distinction
procedure example() -> i32 {
    let value = {
        let temp = compute()
        if temp < 0 {
            result 0           // Exits this block with value 0
        }
        result temp * 2        // Exits this block with temp * 2
    }
    // value is now bound to the block's result
    return value + 1           // Exits the procedure
}
```

**Break Statement**

`break` terminates the innermost (or labeled) loop.

1. `break` MUST appear within a `loop` body.
2. If a label is provided, the labeled loop is terminated.
3. If an expression is provided, the value becomes the loop's result.
4. All `break` statements for the same loop MUST provide values of the same type (or all omit values).

**Continue Statement**

`continue` skips to the next iteration of the innermost (or labeled) loop.

1. `continue` MUST appear within a `loop` body.
2. If a label is provided, the labeled loop's next iteration begins.

##### Dynamic Semantics

**Return Execution**

1. The return expression (if any) is evaluated.
2. Deferred actions in all scopes between the `return` and procedure entry are executed in LIFO order.
3. Control transfers to the caller.

**Result Execution**

1. The result expression is evaluated.
2. Deferred actions in the exited block (only) are executed in LIFO order.
3. The block completes with the yielded value.

**Break/Continue Execution**

1. Deferred actions in scopes between the statement and the target loop are executed in LIFO order.
2. For `break`: the loop terminates with the break value (if any).
3. For `continue`: the loop's next iteration begins.

##### Constraints & Legality

| Code         | Severity | Condition                               | Detection    | Effect    |
| :----------- | :------- | :-------------------------------------- | :----------- | :-------- |
| `E-SEM-3161` | Error    | `return` type mismatch with procedure.  | Compile-time | Rejection |
| `E-SEM-3165` | Error    | `return` at module scope.               | Compile-time | Rejection |
| `E-SEM-3164` | Error    | `result` type mismatch with block.      | Compile-time | Rejection |
| `E-SEM-3162` | Error    | `break` outside `loop`.                 | Compile-time | Rejection |
| `E-SEM-3163` | Error    | `continue` outside `loop`.              | Compile-time | Rejection |
| `E-SEM-3166` | Error    | Unknown loop label.                     | Compile-time | Rejection |
| `E-SEM-3167` | Error    | `break` values have incompatible types. | Compile-time | Rejection |

---

### 12.11 Assignment Statements

##### Definition

An **assignment statement** modifies the value stored in a place expression.

##### Syntax & Declaration

```ebnf
assignment_stmt ::= place_expr assignment_operator expression

assignment_operator ::= "=" | "+=" | "-=" | "*=" | "/=" | "%=" 
                      | "&=" | "|=" | "^=" | "<<=" | ">>="
```

##### Static Semantics

**Requirements**

1. The left-hand side MUST be a place expression (§11.1).
2. The place MUST refer to a mutable binding (declared with `var`).
3. The place MUST be accessible via a `unique` permission. Assignment through `const` or `shared` permission is forbidden.
4. The right-hand side type MUST be compatible with the place's type.

**Compound Assignment Desugaring**

Compound assignment `place op= expr` desugars to `place = place op expr`, except that `place` is evaluated only once.

**Movability Preservation**

The movability property of a binding is determined at declaration and preserved across reassignments:

- A `var x = v` binding remains movable after reassignment.
- A `var x := v` binding remains immovable after reassignment; the new value is also immovable. This means `move x` is always prohibited, but `x = new_value` is permitted. The value currently in `x` is dropped before the new value is assigned.

Reassignment uses the plain `=` operator regardless of movability. The `:=` operator appears only at binding declaration.

##### Dynamic Semantics

**Drop on Reassignment**

Reassignment of a responsible binding triggers deterministic destruction of the prior value as specified in §3.4 (The Binding Model) and §3.6 (Deterministic Destruction).

> **Cross-Reference:** See §3.4 for the normative statement "Each reassignment drops the previous value" and §3.6 for the destruction sequence and LIFO ordering guarantees.

The execution order is:
1. The new value expression is evaluated.
2. The old value's destructor is invoked per §3.6.
3. The new value is installed.

##### Constraints & Legality

| Code         | Severity | Condition                                    | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------- | :----------- | :-------- |
| `E-SEM-3131` | Error    | Assignment target is not a place expression. | Compile-time | Rejection |
| `E-MOD-2401` | Error    | Assignment to immutable `let` binding.       | Compile-time | Rejection |
| `E-SEM-3132` | Error    | Assignment through `const` permission.       | Compile-time | Rejection |
| `E-SEM-3133` | Error    | Assignment type mismatch.                    | Compile-time | Rejection |

---

### 12.12 Defer Statements

##### Definition

A **defer statement** schedules a block of code for execution when the enclosing scope exits.

##### Syntax & Declaration

```ebnf
defer_stmt ::= "defer" block_expr
```

##### Static Semantics

**(T-Defer)**
$$\frac{\Gamma \vdash e : ()}{\Gamma \vdash \texttt{defer}\ \{ e \} : \text{stmt}}$$

The defer statement is well-typed when its body expression has type `()`.

**Constraints:**

1. The deferred block MUST have type `()`.
2. The deferred block MUST NOT contain control flow that transfers outside the defer block:
   - `return` is **always prohibited** within a defer block.
   - `break` targeting a label outside the defer block is prohibited.
   - `continue` targeting a label outside the defer block is prohibited.
   - `result` targeting an expression outside the defer block is prohibited.

   **Permitted:** `break` and `continue` targeting loops or labeled blocks **contained within** the defer block are allowed.
3. The deferred block MAY reference bindings from the enclosing scope that are in scope at the `defer` statement.

##### Dynamic Semantics

**Registration**

When a `defer` statement is executed, the block is not evaluated immediately. Instead, it is pushed onto a per-scope stack of deferred actions.

**Execution Order (Unified LIFO)**

When a scope exits—by normal completion, `return`, `result`, `break`, or panic—all cleanup actions for that scope are executed in Last-In, First-Out (LIFO) order based on source position. **Defer blocks and binding destructions are interleaved in a single unified stack**, not executed as separate phases.

The cleanup stack includes:
1. **Defer blocks** registered by `defer` statements
2. **Binding drops** for bindings with responsibility that implement `Drop`

These are ordered by their source position: the last statement in source order executes its cleanup first.

```cursive
{
    let a = Resource::new()   // Position 1: drop(a) added to cleanup stack
    defer { cleanup_a() }     // Position 2: defer block added to cleanup stack
    let b = Resource::new()   // Position 3: drop(b) added to cleanup stack
}
// Scope exit cleanup order: drop(b), defer { cleanup_a() }, drop(a)
```

**Nested Scopes**

Each block has its own defer stack. When a nested block exits, only its deferred actions execute. When the outer scope exits, its deferred actions execute.

**Panics in Defer**

If a panic occurs during execution of a deferred block:

1. The remaining deferred blocks in that scope are still executed.
2. After all deferred blocks complete, the panic propagates.
3. If multiple panics occur (e.g., a deferred block panics while unwinding from an earlier panic), behavior is Implementation-Defined (IDB). Conforming implementations MUST document their behavior via the Conformance Dossier field `defer.multiple_panic_policy`.

##### Constraints & Legality

| Code         | Severity | Condition                              | Detection    | Effect    |
| :----------- | :------- | :------------------------------------- | :----------- | :-------- |
| `E-SEM-3151` | Error    | Defer block has non-unit type.         | Compile-time | Rejection |
| `E-SEM-3152` | Error    | Non-local control flow in defer block. | Compile-time | Rejection |

---

## Clause 13: The Capability System

### 13.1 Foundational Principles

##### Definition

**Capability**

A **Capability** is a first-class value representing unforgeable authority to perform a specific class of external effects.

**Formal Definition**

$$\text{Capability} ::= (\text{Authority}, \text{Interface}, \text{Lineage})$$

Where:
- **Authority**: The set of permitted operations (e.g., read files, connect to network)
- **Interface**: The form defining available methods
- **Lineage**: The derivation chain from the root capability

##### Static Semantics

**Authority Derivation Rule**

A procedure $p$ may perform effect $e$ iff $p$ receives a capability $c$ where $e \in \text{Authority}(c)$:

$$\frac{\Gamma \vdash c : \$T \quad e \in \text{Authority}(T)}{\Gamma \vdash p(c) \text{ may perform } e}$$

##### Constraints & Legality

| Code         | Severity | Condition                                                           | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------------------------------ | :----------- | :-------- |
| `E-SYS-1001` | Error    | Ambient authority detected: global procedure performs side effects. | Compile-time | Rejection |

**Ambient Authority Detection Mechanism**

Ambient authority violations (E-SYS-1001) are detected by checking the **Capability Requirement Subset Property**: for every call from procedure $A$ to procedure $B$, $\text{capabilities}(B) \subseteq \text{capabilities}(A)$ MUST hold. Intrinsic operations (e.g., file open, network connect) have hardcoded capability requirements; user procedures declare their required capabilities through capability parameters in their signatures. A procedure without capability parameters has empty requirements and therefore cannot invoke effect-producing operations.

---

### 13.2 The Root of Authority

##### Definition

**The Capability Root**

All system-level capabilities originate from the Cursive runtime and are injected into the program via the `Context` parameter at the entry point.

**Cross-Reference**: Entry point signature (§8.9); `Context` record (Appendix H).

**Runtime Injection**

The Cursive runtime constructs the `Context` record before program execution begins. This record contains concrete implementations of all system capability forms, initialized with full authority over system resources.

##### Static Semantics

**Runtime Injection Guarantee**

The runtime **MUST** guarantee that `ctx` is a valid, initialized `Context` record containing root capabilities before `main` begins execution.

##### Constraints & Legality

| Code         | Severity | Condition                                       | Detection    | Effect    |
| :----------- | :------- | :---------------------------------------------- | :----------- | :-------- |
| `E-MOD-2431` | Error    | `main` signature incorrect (see §8.9 for spec). | Compile-time | Rejection |

---

### 13.3 Capability Attenuation

##### Definition

**Attenuation**

**Attenuation** is the process of creating a new capability with strictly **less** authority than the source capability. Attenuation is the primary mechanism for implementing the principle of least privilege.

**Formal Definition**

$$\text{Attenuate}(C_{parent}, R) \to C_{child}$$

Where:
- $C_{parent}$: Source capability
- $R$: Restriction specification (e.g., path, quota, host filter)
- $C_{child}$: Derived capability where $\text{Authority}(C_{child}) \subseteq \text{Authority}(C_{parent})$

**Attenuation Invariant**

A child capability **MUST NOT** grant more authority than its parent:

$$\forall c_{child} \in \text{Attenuate}(c_{parent}, \_) : \text{Authority}(c_{child}) \subseteq \text{Authority}(c_{parent})$$

##### Static Semantics

**Attenuation Method Requirements**

Attenuation methods (e.g., `restrict`, `with_quota`) **MUST** return a capability that:

1. Implements the same capability form as the parent
2. Enforces the specified restrictions on all operations
3. Delegates authorized operations to the parent capability

**Type Preservation Rule**

Attenuation preserves the capability form:

$$\frac{\Gamma \vdash c : \$T \quad \Gamma \vdash c.restrict(r) : \$T'}{\Gamma \vdash T' <: T}$$

---

### 13.4 Capability Propagation

##### Definition

**Capability Propagation**

Capabilities travel through the call graph as explicit parameters. A procedure requiring a capability **MUST** accept it as a parameter; capabilities cannot be obtained through any other means in safe code.

##### Syntax & Declaration

**Capability Parameter Syntax**

Capability parameters use the `$` sigil to indicate capability type polymorphism. The `$Class` syntax accepts any concrete type implementing `Class`:

```cursive
procedure read_config(fs: $FileSystem, path: string@View): string {
    let file = fs~>open(path, Mode::Read)
    file~>read_all()
}
```

**Cross-Reference**: For dynamic dispatch mechanics and existential types, see §7.6.3 (Dynamic Class Types).

##### Static Semantics

**Capability Parameter Rule**

A parameter of type `$Class` accepts any concrete type `T` where `T` implements `Class`.

**Permission Requirements for Capability Methods**

Capability methods **MUST** declare appropriate receiver permissions per the semantics defined in §4.5:

| Operation Type                 | Required Permission |
| :----------------------------- | :------------------ |
| Side-effecting I/O             | `shared` (`~%`)     |
| Pure queries (no state change) | `const` (`~`)       |

##### Constraints & Legality

**Implicit Capability Prohibition**

A procedure **MUST NOT** access capabilities not explicitly provided as parameters or derived from provided parameters.

| Code         | Severity | Condition                                                   | Detection    | Effect    |
| :----------- | :------- | :---------------------------------------------------------- | :----------- | :-------- |
| `E-SYS-1002` | Error    | Effect-producing procedure lacks required capability param. | Compile-time | Rejection |

---

### 13.5 Capability-Concurrency Interaction

##### Definition

**Capability Capture Rules**

When a closure captures bindings from its enclosing scope for use within `spawn` or `dispatch` blocks, capability values follow specific capture rules based on the concurrency model:

1. **Capability Delegation**: Spawned closures inherit the capability context of their parent. If the parent procedure has access to capabilities `C`, spawned children MAY use any capability `c ∈ C`. This property MUST be verified at the spawn site.

2. **Key Isolation**: The Key State Context (§14.1) MUST NOT be inherited across spawn boundaries. Each spawned task MUST start with an empty key context and acquire its own keys independently.

3. **Resource Capture**: Capability values captured by spawned closures follow the standard capture semantics:
   - `const` capabilities are captured by reference (read-only access, no synchronization required)
   - `shared` capabilities are captured with `shared` permission (concurrent access with key-based synchronization)
   - `unique` capabilities MUST be explicitly moved into exactly one spawned task using the `move` keyword

**Cross-Reference**: Parallelism primitives (§15); Key System (§14).

##### Static Semantics

**Spawn Capture Verification**

Each spawn site MUST satisfy the following:

1. All capabilities used within the spawned closure are either captured from the enclosing scope or derived from captured capabilities
2. No `unique` capability is captured by multiple concurrent tasks
3. The parent's capability context covers all effects performed by spawned children

---

### 13.6 Attenuated Capability Lifecycle

##### Definition

**Attenuation Lifetime Constraint**

Attenuated capabilities are **derived** from parent capabilities and maintain a dependency relationship with their parent. The following invariants hold:

1. **Derivation Dependency**: An attenuated capability maintains a runtime dependency on its parent capability. Operations on the child delegate to the parent.

2. **Parent Outlives Children**: The lifetime of a parent capability MUST equal or exceed the lifetime of all attenuated children derived from it.

3. **Child Drop Independence**: Dropping an attenuated child does not affect the parent capability. The parent remains valid and usable.

4. **Parent Drop Restriction**: A parent capability **MUST NOT** be dropped while any attenuated children derived from it are still active.

##### Static Semantics

**Lifetime Enforcement Rule**

For any attenuation operation $c_{child} = c_{parent}.restrict(r)$:

$$\text{Lifetime}(c_{parent}) \geq \text{Lifetime}(c_{child})$$

This is enforced through standard scope analysis. Attenuated capabilities MUST NOT escape the scope of their parent.

**Example**

```cursive
procedure process_files(fs: $FileSystem) {
    let restricted_fs = fs~>restrict("/app/data")    // Child derived from fs
    use_restricted(restricted_fs)
    // restricted_fs dropped here (child)
}   // fs still valid (parent outlives child)
```

---

# Part 4: Concurrency

## Clause 14: The Key System

### 14.1 Key Fundamentals

##### Definition

A **key** is a **static proof of access rights** to a specific path within `shared` data. Keys are primarily a compile-time verification mechanism: key state is tracked during type checking to ensure safe access patterns. Runtime synchronization is introduced only when static analysis cannot prove safety.

**Formal Definition**

A key is a triple:

$$\text{Key} ::= (\text{Path}, \text{Mode}, \text{Scope})$$

| Component | Type                            | Description                                     |
| :-------- | :------------------------------ | :---------------------------------------------- |
| Path      | $\text{PathExpr}$               | The memory location being accessed              |
| Mode      | $\{\text{Read}, \text{Write}\}$ | The access grant (see §14.1.2)                  |
| Scope     | $\text{LexicalScope}$           | The lexical scope during which the key is valid |

The definitions of **Program Point** and **Lexical Scope** are provided in **§1.6 (Foundational Semantic Concepts)**.

**Key State Context**

Key state MUST be tracked at each program point using a key state context:

$$\Gamma_{\text{keys}} : \text{ProgramPoint} \to \mathcal{P}(\text{Key})$$

This mapping associates each program point with the set of keys logically held at that point.

**Held Predicate**

A key is **held** at a program point if it is a member of the key state context at that point:

$$\text{Held}(P, M, S, \Gamma_{\text{keys}}, p) \iff (P, M, S) \in \Gamma_{\text{keys}}(p)$$

When the program point is clear from context, we write $\text{Held}(P, M, \Gamma_{\text{keys}})$ or simply $\text{Held}(P, M)$.

**Key Invariants**

The following invariants govern key behavior:

1. **Path-specificity:** A key to path $P$ MUST grant access only to $P$ and paths for which $P$ is a prefix.

2. **Implicit acquisition:** Accessing a `shared` path MUST logically acquire the necessary key.

3. **Scoped lifetime:** Keys MUST be valid for a bounded lexical scope and MUST become invalid when that scope exits.

4. **Reentrancy:** If a key covering path $P$ is already held by the current task, nested access to $P$ or any path prefixed by $P$ MUST succeed without conflict.

5. **Task locality:** Keys MUST be associated with tasks (§14.1). A key held by a task MUST remain valid until its scope exits.

##### Syntax & Declaration

**Path Expression Grammar**

A **path expression** identifies a location within a value:

```ebnf
path_expr       ::= root_segment ("." path_segment)*

root_segment    ::= key_marker? IDENTIFIER index_suffix?

path_segment    ::= key_marker? IDENTIFIER index_suffix?

key_marker      ::= "#"

index_suffix    ::= "[" expression "]"
```

**Terminology:**

- The **root** of a path is the first identifier (e.g., `player` in `player.stats.health`)
- A **segment** is a single field access or indexed access
- The **depth** of a path is the number of segments (e.g., `player.stats.health` has depth 3)

**Path Root Extraction**

The **root** of an expression, written $\text{Root}(e)$, extracts the base identifier from which key path analysis begins. The operation is defined recursively:

**Formal Definition**

$$\text{Root}(e) ::= \begin{cases}
x & \text{if } e = x \text{ (identifier)} \\
\text{Root}(e') & \text{if } e = e'.f \text{ (field access)} \\
\text{Root}(e') & \text{if } e = e'[i] \text{ (index access)} \\
\text{Root}(e') & \text{if } e = e'.m(\ldots) \text{ (method call)} \\
\bot_{\text{boundary}} & \text{if } e = (*e') \text{ (dereference)}
\end{cases}$$

where $\bot_{\text{boundary}}$ indicates a **key boundary**—the dereferenced value establishes an independent key context (see "Pointer Indirection and Key Boundaries" below).

**Key Boundary Semantics**

When $\text{Root}(e) = \bot_{\text{boundary}}$, the expression $e$ introduces a new key path rooted at the runtime identity of the dereferenced value:

$$\frac{
    e = (*e').p \quad
    \text{Root}(e) = \bot_{\text{boundary}}
}{
    \text{KeyPath}(e) = \text{id}(*e').p
}$$

The $\text{id}$ operation is defined in §14.3.3.

```cursive
player                      // Depth 1: root only
player.health               // Depth 2: root + one field
player.stats.health         // Depth 3: root + two fields
player.inventory[5]         // Depth 2: root + indexed field
team.players[0].health      // Depth 3: root + indexed field + field
```

**Pointer Indirection and Key Boundaries**

A pointer dereference creates a **key boundary**: the dereferenced value establishes a new, independent key context. Key paths do not extend across pointer indirections.

For path `(*e).p` where `e : shared Ptr<T>@Valid`:
- A key is first acquired for `e` (to safely dereference)
- A separate key is then acquired for `(*e).p`, rooted at the dereferenced value

**Key Boundary Rules:**

1. **Direct field paths:** A path `a.b.c` MUST be treated as a single key path.
2. **Pointer field access:** Accessing a pointer field MUST acquire a fine-grained key to that field path.
3. **Pointer dereference:** `(*ptr).field` MUST start a new key path rooted at the dereferenced value.
4. **Coarsening applies:** The `#` operator MAY coarsen pointer field access to the containing node.

**(K-Deref-Boundary)**
$$\frac{
    \Gamma \vdash e : \texttt{shared}\ \text{Ptr}\langle T \rangle@\text{Valid} \quad
    \Gamma \vdash (*e).p : \texttt{shared}\ U
}{
    \text{KeyPath}((*e).p) = \text{id}(*e).p \quad
    \text{(independent of KeyPath}(e)\text{)}
}$$

```cursive
// Linked list traversal
let list: shared List<i32> = ...

list.value          // Key: list.value (path within first node)
(*list.next).value  // Key: (*list.next).value (new key context)

// Each node access is independent
parallel {
    spawn { list.value = 1 }           // Key to list.value
    spawn { (*list.next).value = 2 }   // Key to (*list.next).value — DISJOINT
}
```

##### Static Semantics

**Path Well-Formedness**

A path expression $P = p_1.p_2.\ldots.p_n$ is **well-formed** if and only if:

1. The root identifier $p_1$ is bound in scope $\Gamma$ with type $T_0$
2. For each subsequent segment $p_i$ ($i > 1$):
   - If $p_i$ is a field access, $T_{i-1}$ has a field named $p_i$ with type $T_i$
   - If $p_i$ is an index access $[e]$, $T_{i-1}$ is an array or slice type with element type $T_i$, and $e$ has type `usize`
3. At most one `#` marker appears in the entire path (see §14.4)

**(WF-Path)**
$$\frac{
    \Gamma \vdash p_1 : T_0 \quad
    \forall i \in 2..n,\ \Gamma \vdash T_{i-1}.p_i : T_i
}{
    \Gamma \vdash p_1.p_2.\ldots.p_n : T_n\ \text{wf}
}$$

**Path Typing for Key Determination**

A path expression has an associated **permission** derived from the root binding:

**(Path-Permission)**
$$\frac{
    \Gamma \vdash p_1 : P\ T_0 \quad P \in \{\texttt{const}, \texttt{unique}, \texttt{shared}\}
}{
    \Gamma \vdash p_1.p_2.\ldots.p_n : P\ T_n
}$$

**Key Analysis Applicability**

Key analysis is performed if and only if the path permission is `shared`:

$$\text{RequiresKeyAnalysis}(P) \iff \text{Permission}(P) = \texttt{shared}$$

Paths with `const` or `unique` permission require no key analysis (they are statically safe by the permission system defined in §4.5).

**Key Path Determination for Method Calls**

When a path expression includes a method call, the key path is determined by the method's return type and receiver:

1. If the method returns a reference into the receiver's data (e.g., a field accessor), the key path extends through the method call to the returned reference's target.

2. If the method returns an independent value (not a reference into the receiver), the returned value is a fresh binding and key analysis applies to accesses on that binding independently.

3. For method chaining on `shared` data, each method call in the chain is analyzed for its receiver permission. If a method requires `unique` or write access to its receiver, the key path MUST include the receiver with Write mode.

**(K-Method-Call-Path)**
$$\frac{
    \Gamma \vdash e.\text{method}() : \texttt{shared}\ T \quad
    \text{method returns reference into } e
}{
    \text{KeyPath}(e.\text{method}().f) = \text{KeyPath}(e) \cup \{f\}
}$$


##### Memory & Layout

**Compile-Time Representation**

Keys have no runtime representation when statically proven safe. Key state is tracked in $\Gamma_{\text{keys}}$ during type checking as an abstract data structure with no physical manifestation.

**Runtime Representation (When Required)**

When runtime synchronization is necessary, key metadata representation is IDB. The following aspects are implementation-defined:

1. Whether synchronization metadata is stored inline within `shared` values or in external data structures
2. Size overhead per `shared` value, if any
3. Lock table or hash table organization, if used
4. Memory ordering guarantees for metadata access

**ABI Guarantees**

| Aspect                          | Guarantee                                           |
| :------------------------------ | :-------------------------------------------------- |
| Key metadata presence           | IDB — may or may not exist at runtime               |
| Key metadata layout             | IDB — not observable by conforming programs         |
| `shared T` vs `unique T` layout | MAY be identical when statically safe               |
| `shared T` size                 | MAY differ from `T` size when runtime sync required |

##### Constraints & Legality

**Negative Constraints**

1. A program MUST NOT access a `shared` path outside a valid key context.
2. A program MUST NOT apply the `#` annotation to a non-`shared` path.
3. A program MUST NOT include multiple `#` markers in a single path expression.
4. A program MUST NOT cause a key to escape its defining scope.

**Key Escape**

A key **escapes** its defining scope $S$ if any of the following conditions hold:

1. A reference to a `shared` path $P$ for which a key is held in $S$ is stored in a binding whose lifetime exceeds $S$
2. A reference to such a path is returned from a procedure whose body contains $S$
3. A reference to such a path is captured by a closure that outlives $S$
4. A reference to such a path is passed to a `spawn` expression (transferring to another task)

**(K-No-Escape)**
$\frac{
    \text{Held}(P, M, S) \quad
    \text{Ref}(P) \text{ stored in } B \quad
    \text{Lifetime}(B) > \text{Lifetime}(S)
}{
    \text{Emit}(\texttt{E-CON-0004})
}$

**Diagnostic Table**

| Code         | Severity | Condition                                         | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------------ | :----------- | :-------- |
| `E-CON-0001` | Error    | Access to `shared` path outside valid key context | Compile-time | Rejection |
| `E-CON-0002` | Error    | `#` annotation on non-`shared` path               | Compile-time | Rejection |
| `E-CON-0003` | Error    | Multiple `#` markers in single path expression    | Compile-time | Rejection |
| `E-CON-0004` | Error    | Key escapes its defining scope                    | Compile-time | Rejection |

#### 13.1.1 Path Prefix and Disjointness

##### Definition

The **prefix relation** and **disjointness relation** on paths determine key coverage and static safety of concurrent accesses.

**Formal Definition**

**Path Prefix:**

Path $P$ is a **prefix** of path $Q$ (written $\text{Prefix}(P, Q)$) if $P$ is an initial subsequence of $Q$:

$$\text{Prefix}(p_1.\ldots.p_m,\ q_1.\ldots.q_n) \iff m \leq n \land \forall i \in 1..m,\ p_i \equiv_{\text{seg}} q_i$$

**Path Disjointness:**

Two paths $P$ and $Q$ are **disjoint** (written $\text{Disjoint}(P, Q)$) if neither is a prefix of the other:

$$\text{Disjoint}(P, Q) \iff \neg\text{Prefix}(P, Q) \land \neg\text{Prefix}(Q, P)$$

**Path Overlap**

Two paths $P$ and $Q$ **overlap** (written $\text{Overlaps}(P, Q)$) if they are not disjoint—that is, if one is a prefix of the other:

$$\text{Overlaps}(P, Q) \iff \neg\text{Disjoint}(P, Q)$$

Equivalently:

$$\text{Overlaps}(P, Q) \iff \text{Prefix}(P, Q) \lor \text{Prefix}(Q, P)$$

**Overlap Semantics**

| Relationship         | Overlaps? | Implication for Keys                |
| :------------------- | :-------- | :---------------------------------- |
| $P$ is prefix of $Q$ | Yes       | Key to $P$ covers access to $Q$     |
| $Q$ is prefix of $P$ | Yes       | Key to $Q$ covers access to $P$     |
| Neither is prefix    | No        | Keys to $P$ and $Q$ are independent |
| $P = Q$              | Yes       | Same path                           |

**Segment Equivalence:**

Two segments are equivalent ($\equiv_{\text{seg}}$) if they have the same identifier and, for indexed segments, provably equivalent index expressions:

$$p_i \equiv_{\text{seg}} q_i \iff \text{name}(p_i) = \text{name}(q_i) \land \text{IndexEquiv}(p_i, q_i)$$

where $\text{IndexEquiv}$ is defined as:

$$\text{IndexEquiv}(p, q) \iff \begin{cases}
\text{true} & \text{if neither } p \text{ nor } q \text{ is indexed} \\
e_p \equiv_{\text{idx}} e_q & \text{if both are indexed with expressions } e_p, e_q \\
\text{false} & \text{otherwise}
\end{cases}$$

**Index Expression Equivalence:**

Two index expressions $e_1$ and $e_2$ are **provably equivalent** ($e_1 \equiv_{\text{idx}} e_2$) if and only if one of the following conditions holds:

1. Both are the same integer literal
2. Both are references to the same `const` binding
3. Both are references to the same generic const parameter
4. Both are references to the same variable binding in scope
5. Both normalize to the same canonical form under constant folding

**Canonical Form Normalization**

Implementations MUST perform constant folding and canonicalization of linear integer expressions. Two expressions that normalize to the same canonical form are **provably equivalent**.

Implementations MAY apply additional algebraic simplifications to prove index equivalence. If normalization exceeds implementation limits (§1.4), expressions MUST be treated as **not provably equivalent**. In such cases, runtime synchronization MUST be emitted or `E-CON-0010` MUST be diagnosed.

**Provable Disjointness for Indices:**

Two index expressions $e_1$ and $e_2$ are **provably disjoint** ($\text{ProvablyDisjoint}(e_1, e_2)$) if any of the following conditions hold:

1. **Static Disjointness:** Both are statically resolvable with different values:
   $$\text{StaticResolvable}(e_1) \land \text{StaticResolvable}(e_2) \land \text{StaticValue}(e_1) \neq \text{StaticValue}(e_2)$$

2. **Control Flow Facts:** A Verification Fact (§11.5) establishes $e_1 \neq e_2$ via conditional guard (e.g., `if i != j { ... }`).

3. **Contract-Based:** A procedure precondition or inline parameter constraint asserts $e_1 \neq e_2$.

4. **Refinement Types:** An index has a refinement type (§7.3) constraining its relationship to another index.

5. **Algebraic Offset:** Index expressions share a common base but differ by statically-known constant offsets ($e_1 = v + c_1$, $e_2 = v + c_2$ where $c_1 \neq c_2$).

6. **Dispatch Iteration:** Within a `dispatch` block (§14.5), accesses indexed by the iteration variable are automatically disjoint across iterations.

7. **Disjoint Loop Ranges:** Index expressions reference iteration variables from loops with non-overlapping ranges.

An index expression is **statically resolvable** if it is: an integer literal, a reference to a `const` binding, a generic const parameter, or an arithmetic expression where all operands are statically resolvable.

##### Static Semantics

**Disjointness for Static Safety**

When two paths are provably disjoint, concurrent accesses to those paths are statically safe and require no runtime synchronization:

**(K-Disjoint-Safe)**
$$\frac{
    \text{Disjoint}(P, Q)
}{
    \text{ConcurrentAccess}(P, Q) \text{ is statically safe}
}$$

**Prefix for Coverage**

When a held key's path is a prefix of an access path, the held key covers the access:

**(K-Prefix-Coverage)**
$$\frac{
    \text{Prefix}(P, Q) \quad \text{Held}(P, M, \Gamma_{\text{keys}})
}{
    \text{Covers}((P, M), Q)
}$$

#### 13.1.2 Key Modes

##### Definition

A key's **mode** determines what operations it permits and how it interacts with other keys to the same or overlapping paths.

**Formal Definition**

$$\text{Mode} ::= \text{Read} \mid \text{Write}$$

| Mode  | Permitted Operations       | Exclusivity                                                       |
| :---- | :------------------------- | :---------------------------------------------------------------- |
| Read  | Read the value at the path | Shared: multiple Read keys to overlapping paths MAY coexist       |
| Write | Read and write the value   | Exclusive: Write key excludes all other keys to overlapping paths |

**Read Context and Write Context**

The predicates $\text{ReadContext}(e)$ and $\text{WriteContext}(e)$ classify expression positions by their access requirements.

**Formal Definition:**

$$\text{ReadContext}(e) \iff e \text{ appears in a syntactic position requiring only read access}$$

$$\text{WriteContext}(e) \iff e \text{ appears in a syntactic position requiring write access}$$

**Classification Rules:**

| Syntactic Position                                   | Context |
| :--------------------------------------------------- | :------ |
| Right-hand side of `let`/`var` initializer           | Read    |
| Right-hand side of assignment (`=`)                  | Read    |
| Operand of arithmetic/logical operator               | Read    |
| Argument to `const` or `shared` parameter            | Read    |
| Condition expression (`if`, `while`, `match`)        | Read    |
| Left-hand side of assignment (`=`)                   | Write   |
| Left-hand side of compound assignment (`+=`, etc.)   | Write   |
| Receiver of method with `~!` (unique) receiver       | Write   |
| Receiver of method with `~%` (shared-write) receiver | Write   |
| Argument to `unique` parameter                       | Write   |
| Receiver of method with `~` (const) receiver         | Read    |

**Disambiguation Rule:**

If an expression appears in multiple contexts (e.g., compound assignment target), the more restrictive context (Write) applies.

##### Static Semantics

**Mode Determination**

The required key mode is determined by the syntactic context of the access:

**(K-Mode-Read)**
$$\frac{
    \Gamma \vdash e : \texttt{shared}\ T \quad
    \text{ReadContext}(e)
}{
    \text{RequiredMode}(e) = \text{Read}
}$$

**(K-Mode-Write)**
$$\frac{
    \Gamma \vdash e : \texttt{shared}\ T \quad
    \text{WriteContext}(e)
}{
    \text{RequiredMode}(e) = \text{Write}
}$$

**Key Compatibility**

Two keys $K_1 = (P_1, M_1, S_1)$ and $K_2 = (P_2, M_2, S_2)$ are **compatible** (may coexist) if and only if:

$$\text{Compatible}(K_1, K_2) \iff \text{Disjoint}(P_1, P_2) \lor (M_1 = \text{Read} \land M_2 = \text{Read})$$

**Compatibility Matrix:**

| Key A Mode | Key B Mode | Paths Overlap? | Compatible? |
| :--------- | :--------- | :------------- | :---------- |
| Read       | Read       | Yes            | Yes         |
| Read       | Write      | Yes            | No          |
| Write      | Read       | Yes            | No          |
| Write      | Write      | Yes            | No          |
| Any        | Any        | No (disjoint)  | Yes         |

##### Dynamic Semantics

**When Runtime Synchronization Is Required**

If concurrent access compatibility cannot be statically proven, runtime synchronization MUST enforce the compatibility rules:

- **Compatible keys:** May proceed concurrently
- **Incompatible keys:** One task blocks until the other releases

**Blocking Semantics (Runtime Only)**

When runtime synchronization is active and a task attempts to acquire an incompatible key:

1. The task MUST block until the conflicting key is released
2. Blocked tasks MUST be queued in a manner that ensures eventual acquisition
3. The blocking mechanism is Implementation-Defined Behavior (IDB)

**Progress Guarantee**

Implementations MUST guarantee **eventual progress**: any task blocked waiting for a key MUST eventually acquire that key, provided the holder eventually releases it.

Formally, for any task $t$ blocked on key $K$ held by task $t'$:

$$\text{Blocked}(t, K) \land \text{Held}(K, t') \land \Diamond\text{Released}(K, t') \implies \Diamond\text{Acquired}(K, t)$$

where $\Diamond$ is the temporal "eventually" operator.


##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                     | Detection    | Effect    |
| :----------- | :------- | :-------------------------------------------- | :----------- | :-------- |
| `E-CON-0005` | Error    | Write access required but only Read available | Compile-time | Rejection |

---

### 14.2 Key Acquisition and Release

##### Definition

Keys are logically acquired **on demand** during expression evaluation and released **at scope exit**.

**Formal Definition**

Let $\text{KeySet}(s)$ denote the set of keys logically acquired during evaluation of statement or block $s$. The key lifecycle has three phases:

1. **Acquisition Phase:** Keys are logically acquired as `shared` paths are evaluated, in evaluation order
2. **Execution Phase:** The statement or block body executes with all keys logically held
3. **Release Phase:** All keys in $\text{KeySet}(s)$ become invalid when $s$'s scope exits

##### Static Semantics

**Mode Ordering and Sufficiency**

Key modes form a total order (§14.1.2): $\text{Read} < \text{Write}$.

A held mode is **sufficient** for a required mode when the held mode is at least as strong:

$$\text{ModeSufficient}(M_{\text{held}}, M_{\text{required}}) \iff M_{\text{required}} \leq M_{\text{held}}$$

**Key Set Operations**

Let $\Gamma_{\text{keys}}$ be the current key set, a collection of $(P, M, S)$ triples 
where $P$ is a path, $M$ is a mode, and $S$ is the scope identifier.

**(Acquire)**
$$\text{Acquire}(P, M, S, \Gamma_{\text{keys}}) = \Gamma_{\text{keys}} \cup \{(P, M, S)\}$$

**(Release)**
$$\text{Release}(P, \Gamma_{\text{keys}}) = \Gamma_{\text{keys}} \setminus \{(P, M, S) : (P, M, S) \in \Gamma_{\text{keys}}\}$$

**(Release by Scope)**
$$\text{ReleaseScope}(S, \Gamma_{\text{keys}}) = \Gamma_{\text{keys}} \setminus \{(P, M, S') : S' = S\}$$

**(Mode Transition)**

The mode transition operation replaces a key of one mode with a key of a different mode for the same path and scope. This is a compile-time transformation of the key state context; runtime behavior follows release-and-reacquire semantics (§14.7.2).

$$\text{ModeTransition}(P, M_{\text{new}}, \Gamma_{\text{keys}}) = (\Gamma_{\text{keys}} \setminus \{(P, M_{\text{old}}, S)\}) \cup \{(P, M_{\text{new}}, S)\}$$

where $S$ is the scope of the existing key and $M_{\text{old}} \neq M_{\text{new}}$.

**Precondition**: A key to path $P$ with a different mode MUST exist in $\Gamma_{\text{keys}}$:

$$\text{ModeTransition}(P, M, \Gamma_{\text{keys}}) \text{ is defined} \iff \exists S, M' : (P, M', S) \in \Gamma_{\text{keys}} \land M' \neq M$$

**Dynamic Realization**

At runtime, key mode transition follows the release-and-reacquire protocol defined in §14.7.2. The compile-time `ModeTransition` operation reflects the intended final state; the runtime achieves this state through the five-step sequence.

**Panic Release Semantics**

When a panic occurs in scope $S$:

$$\text{PanicRelease}(S, \Gamma_{\text{keys}}) = \Gamma_{\text{keys}} \setminus \{(P, M, S') : S' \leq_{\text{nest}} S\}$$

where $S' \leq_{\text{nest}} S$ indicates $S'$ is $S$ or any scope nested within $S$.

All keys held by the panicking scope and its nested scopes are released atomically
before panic unwinding proceeds. Keys held by enclosing scopes are unaffected.

**Key and RAII Cleanup Interaction**

Keys acquired within explicit `#` blocks are released at the closing brace of the `#` block, not at the containing scope exit. For implicit key acquisition (direct access to `shared` paths without `#`), keys are released at scope exit **before** any `Drop::drop` calls for bindings in that scope.

Within the unified LIFO cleanup stack (§12.12), key releases precede destructor invocations. This ordering prevents potential deadlocks where a destructor might attempt to acquire a key that is still held.

**Cross-Reference**: For the unified cleanup ordering rules governing defer blocks and destructor interleaving, see §12.12 (Defer Statements).

##### Static Semantics

**Compile-Time Key Tracking**

Key state MUST be tracked at each program point. For each `shared` path access:

1. The key path is computed (per §14.4)
2. The required mode is determined (per §14.1.2)
3. Coverage by an existing key is checked
4. The key is added to $\Gamma_{\text{keys}}$ if not covered
5. The key is marked for release at scope exit

**Covered Predicate**

An access to path $Q$ requiring mode $M_Q$ is **covered** by the current key state if there exists a held key whose path is a prefix of $Q$ and whose mode is sufficient:

$$\text{Covered}(Q, M_Q, \Gamma_{\text{keys}}) \iff \exists (P, M_P, S) \in \Gamma_{\text{keys}} : \text{Prefix}(P, Q) \land \text{ModeSufficient}(M_P, M_Q)$$

Mode Sufficiency ($\text{ModeSufficient}$) is defined above in §14.2 Formal Operation Definitions.

**Acquisition Rules**

**(K-Acquire-New)**

If an access requires a key and no covering key exists, a new key is acquired:

$$\frac{
    \Gamma \vdash P : \texttt{shared}\ T \quad
    M = \text{RequiredMode}(P) \quad
    \neg\text{Covered}(P, M, \Gamma_{\text{keys}}) \quad
    S = \text{CurrentScope}
}{
    \Gamma'_{\text{keys}} = \Gamma_{\text{keys}} \cup \{(P, M, S)\}
}$$

**(K-Acquire-Covered)**

If an access is covered by an existing key with sufficient mode, no acquisition occurs:

$$\frac{
    \Gamma \vdash P : \texttt{shared}\ T \quad
    M = \text{RequiredMode}(P) \quad
    \text{Covered}(P, M, \Gamma_{\text{keys}})
}{
    \Gamma'_{\text{keys}} = \Gamma_{\text{keys}}
}$$

**(K-Acquire-Mode-Transition)**

If an access is covered by an existing key but requires a different mode, a mode transition occurs:

$$\frac{
    \Gamma \vdash P : \texttt{shared}\ T \quad
    \text{RequiredMode}(P) = M_{\text{new}} \quad
    \exists (Q, M_{\text{old}}, S) \in \Gamma_{\text{keys}} : \text{Prefix}(Q, P) \land M_{\text{old}} \neq M_{\text{new}}
}{
    \text{ModeTransition}(Q, M_{\text{new}}, \Gamma_{\text{keys}})
}$$

**Release Rules**

**(K-Release-Scope)**

When a scope $S$ exits, all keys with that scope are released:

$$\frac{
    \text{ScopeExit}(S)
}{
    \Gamma'_{\text{keys}} = \Gamma_{\text{keys}} \setminus \{(P, M, S') : S' = S\}
}$$

**(K-Release-Order)**

Keys acquired within a scope are released in reverse acquisition order (LIFO). This mirrors the destruction order for RAII bindings.

**Evaluation Order**

Subexpressions MUST be evaluated **left-to-right, depth-first**. Key acquisition follows evaluation order. This order is Defined Behavior.

**(K-Eval-Order)** For expression $e_1 \oplus e_2$:

$$\text{eval}(e_1 \oplus e_2) \longrightarrow \text{eval}(e_1);\ \text{eval}(e_2);\ \text{apply}(\oplus)$$

**Destructuring Patterns**

When a `shared` value is destructured via pattern matching, a single key is acquired to the root of the matched expression. The key is held for the duration of pattern evaluation and binding initialization. All bindings introduced by the pattern receive copies/moves of the destructured values; they do not retain key requirements.

**(K-Destructure)**
$$\frac{
    \Gamma \vdash e : \texttt{shared}\ (T_1, \ldots, T_n) \quad
    \texttt{let}\ (p_1, \ldots, p_n) = e
}{
    \text{KeyPath} = \text{Root}(e) \quad
    \text{KeyMode} = \text{Read}
}$$

**Pattern Destructuring Key Behavior:**

| Pattern                       | Expression             | Key Path                   | Mode     |
| :---------------------------- | :--------------------- | :------------------------- | :------- |
| `let (a, b) = tuple`          | `tuple: shared (T, U)` | `tuple`                    | Read     |
| `let Point { x, y } = point`  | `point: shared Point`  | `point`                    | Read     |
| `match shared_enum { ... }`   | scrutinee              | Root of scrutinee          | Read     |
| `let (a, b) = (expr1, expr2)` | tuple literal          | Each expression separately | Per-expr |

##### Dynamic Semantics

**Runtime Realization (When Required)**

If static safety cannot be proven for a key acquisition, the implementation MUST emit runtime synchronization that:

1. Blocks if an incompatible key is held by another task
2. Proceeds once compatibility is established
3. Releases the key when the scope exits

**Release Guarantee**

Keys MUST be released (logically, and at runtime if synchronized) when their scope exits, regardless of exit mechanism:

| Exit Mechanism    | Keys Released? |
| :---------------- | :------------- |
| Normal completion | Yes            |
| `return`          | Yes            |
| `break`           | Yes            |
| `continue`        | Yes            |
| Panic propagation | Yes            |
| Task cancellation | Yes            |

**Panic Release Semantics**

If a panic occurs while keys are held:

1. All keys held by the panicking task MUST be released before unwinding propagates to the caller.
2. If panic occurs during key acquisition, any partially-acquired keys MUST be released.
3. If panic occurs during mode transition—whether while blocked waiting for conflicting keys to be released or during the atomic state transition—the key MUST be released entirely (not left in original mode).
4. Key release during panic MUST occur **before** any `Drop::drop` calls for bindings in the same scope.

**Mode Transition Panic Clarification**

"During mode transition" encompasses:
- The period while the task is blocked waiting for conflicting keys to be released
- The atomic state transition between modes (e.g., Read to Write or Write to Read)

In either case, the original key (if any) is released, and no new key is acquired.

**Defer Interaction**

Keys acquired within a `defer` block are scoped to that block. Keys held when a scope exits are released **before** `defer` blocks execute. Deferred statements that access `shared` paths acquire fresh keys when the `defer` body executes.

##### Constraints & Legality

**Scope Definitions**

| Construct                      | Key Scope                  | Release Point          |
| :----------------------------- | :------------------------- | :--------------------- |
| Expression statement (`expr;`) | The statement              | Semicolon              |
| `let`/`var` declaration        | The declaration            | Semicolon              |
| Assignment statement           | The statement              | Semicolon              |
| `if` condition                 | The condition only         | Before entering branch |
| `match` scrutinee              | The scrutinee only         | Before entering arm    |
| `loop` condition               | Each iteration's condition | Before entering body   |
| `#path { ... }` block          | The entire block           | Closing brace          |
| Procedure body                 | Callee's body              | Procedure return       |
| `defer` body                   | The defer block            | Defer completion       |

**Diagnostic Table**

| Code         | Severity | Condition                                          | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------------- | :----------- | :-------- |
| `W-CON-0001` | Warning  | Fine-grained keys in tight loop (performance hint) | Compile-time | Warning   |
| `W-CON-0002` | Warning  | Redundant key acquisition (already covered)        | Compile-time | Warning   |
| `E-CON-0006` | Error    | Key acquisition in `defer` escapes to outer scope  | Compile-time | Rejection |

---

### 14.3 Key Acquisition at Procedure Boundaries

##### Static Semantics

**(K-Procedure-Boundary)**

Passing a `shared` value as a procedure argument does not constitute key acquisition. The callee's accesses are analyzed independently:

$$\frac{
    \Gamma \vdash f : (\texttt{shared}\ T) \to R \quad
    \Gamma \vdash v : \texttt{shared}\ T
}{
    \text{call}(f, v) \longrightarrow \text{no key acquisition at call site}
}$$

**Callee `[[dynamic]]` Independence**

The `[[dynamic]]` status of a callee is independent of the caller's status. A `[[dynamic]]` caller MAY invoke non-`[[dynamic]]` callees, and vice versa. Each procedure's key safety is analyzed independently based on its own `[[dynamic]]` annotation.

**Interprocedural Analysis**

Interprocedural analysis MAY be performed to prove static safety across procedure boundaries, subject to the following constraints:

**Soundness Requirement:** If interprocedural analysis concludes that no runtime synchronization is needed for a callee's accesses, then concurrent execution of that callee MUST be safe. The analysis MUST be sound—false negatives (missing synchronization) are forbidden.

**Conservatism Requirement:** If interprocedural analysis cannot prove safety and the callee is not `[[dynamic]]`, the program is ill-formed.

**Separate Compilation:** For procedures in separate compilation units whose source is not available:
1. The implementation MUST assume the callee may access **any** path reachable from its `shared` parameters
2. The implementation MUST assume the callee requires **Write** access unless the parameter is declared `const`
3. If safety cannot be proven and the call site is not in a `[[dynamic]]` scope (§7.2.9), the program is ill-formed

**Inlining Interaction:** If a procedure is inlined, the inlined body MUST be analyzed as if its statements were written at the call site, with full visibility of the caller's key context and the caller's `[[dynamic]]` status.

##### Dynamic Semantics

**Fine-Grained Parallelism**

When interprocedural analysis proves that callees access disjoint paths, the implementation MAY execute those callees concurrently without synchronization.

**Callee-Determined Granularity**

The callee determines key granularity through its access patterns. A callee MAY use explicit `#` blocks to coarsen granularity or rely on implicit fine-grained key acquisition.

##### Constraints & Legality

**Reentrancy**

If a caller holds a key covering the callee's access paths, the callee's accesses are covered (reentrant). No additional key acquisition occurs for paths already protected by a covering key held in the call stack.

**(K-Reentrant)**
$$\frac{
    \text{Held}(P, M, \Gamma_{\text{keys}}) \quad
    \text{Prefix}(P, Q) \quad
    \text{CalleeAccesses}(Q)
}{
    \text{CalleeCovered}(Q)
}$$

**Diagnostic Table**

| Code         | Severity | Condition                                           | Detection    | Effect  |
| :----------- | :------- | :-------------------------------------------------- | :----------- | :------ |
| `W-CON-0005` | Warning  | Callee access pattern unknown; assuming full access | Compile-time | Warning |

#### 13.3.1 Witness Types and Shared Permission

##### Definition

A `shared dyn Class` type is the composition of the `shared` permission (§4.5) applied to a dyn type (§9.5). This combination permits polymorphic read access to shared data.

Because dynamic dispatch erases the concrete type, only forms with exclusively read-only methods (`~` receiver) are compatible with `shared` permission. This constraint follows from the receiver compatibility rules defined in §4.5.4–§4.5.5.

##### Static Semantics

The following well-formedness rule constrains which forms can appear in `shared dyn` types. This rule derives from §4.5's receiver compatibility matrix and §9.5's dyn safety requirements:

**(K-Witness-Shared-WF)**
$$\frac{
    \forall m \in \text{AllMethods}(Tr) : m.\text{receiver} = \texttt{\sim}
}{
    \Gamma \vdash \texttt{shared dyn } Tr\ \text{wf}
}$$

where $\text{AllMethods}(Tr)$ includes inherited methods from superclasses.

When invoking a method on `shared dyn Class`:

**(K-Witness-Call)**
$$\frac{
    \Gamma \vdash e : \texttt{shared dyn } Cl \quad
    m \in \text{Methods}(Cl)
}{
    \text{KeyPath}(e.m(\ldots)) = \text{Root}(e) \quad
    \text{KeyMode} = \text{Read}
}$$

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                     | Detection    | Effect    |
| :----------- | :------- | :-------------------------------------------- | :----------- | :-------- |
| `E-CON-0083` | Error    | `shared dyn Class` where Form has `~%` method | Compile-time | Rejection |

#### 13.3.2 Modal Type Transitions

##### Definition

A state transition modifies the discriminant of a modal value, which constitutes mutation. When a transition is invoked on a `shared` modal value, a Write key MUST be acquired.

##### Static Semantics

**(K-Modal-Transition)**
$$\frac{
    \Gamma \vdash e : \texttt{shared}\ M@S_1 \quad
    t : (M@S_1) \to M@S_2 \in \text{Transitions}(M)
}{
    \text{KeyPath}(e.t(\ldots)) = \text{Root}(e) \quad
    \text{KeyMode} = \text{Write}
}$$

State-specific field access follows standard rules:
- Reading a payload field requires a Read key
- Writing a payload field requires a Write key
- The key path is the modal value's root (not the individual field)

#### 13.3.3 Closure Capture of Shared Bindings

##### Definition

Closures capturing `shared` bindings are subject to **escape-based classification**. 
The analysis and type requirements differ based on whether the closure escapes its 
defining scope.

**Classification**

A closure is **local** if it appears in one of the following contexts:
1. Argument position of a procedure or method call
2. Body of a `spawn` expression
3. Body of a `dispatch` expression
4. Operand of immediate invocation

A closure is **escaping** if it is:
1. Bound to a `let` or `var` binding
2. Returned from a procedure
3. Stored in a data structure field

**Object Identity**

The **identity** of a reference $r$, written $\text{id}(r)$, is a unique value representing the storage location to which $r$ refers at runtime.

**Formal Definition**

$$\text{id} : \text{Reference} \to \text{Identity}$$

Two references have equal identity if and only if they refer to the same storage location:

$$\text{id}(r_1) = \text{id}(r_2) \iff r_1 \text{ and } r_2 \text{ refer to the same storage location}$$

**Properties**

| Property      | Guarantee                                                                      |
| :------------ | :----------------------------------------------------------------------------- |
| Uniqueness    | Distinct storage locations produce distinct identity values                    |
| Stability     | The identity of a reference remains constant for the reference's lifetime      |
| Comparability | Identity values support equality comparison                                    |
| Non-forgeable | Identity values cannot be constructed except by taking identity of a reference |

**##### Static Semantics

**Local Closure Analysis**

For local closures, key analysis uses the **defining scope's paths**:

**(K-Closure-Local)**
$$\frac{
    C \text{ is local} \quad
    x \in \text{SharedCaptures}(C) \quad
    x : \texttt{shared}\ T \in \Gamma_{\text{def}}
}{
    \text{KeyPath}(C, x.p) = x.p
}$$

Key acquisition occurs when the closure is invoked. The defining scope's binding 
serves as the key path root. Fine-grained path analysis is preserved.

**Escaping Closure Type Requirement**

For escaping closures, the type MUST include a **shared dependency set**:

**(K-Closure-Escape-Type)**
$$\frac{
    C \text{ is escaping} \quad
    \text{SharedCaptures}(C) = \{x_1, \ldots, x_n\}
}{
    \text{Type}(C) = |\vec{T}| \to R\ [\texttt{shared}: \{x_1 : T_1, \ldots, x_n : T_n\}]
}$$

**Escaping Closure Key Analysis**

For escaping closures, key analysis uses **object identity**:

**(K-Closure-Escape-Keys)**
$$\frac{
    C : |\vec{T}| \to R\ [\texttt{shared}: \{x : T\}] \quad
    \text{Access}(x.p, M) \in C.\text{body}
}{
    \text{KeyPath}(C, x.p) = \text{id}(C.x).p \quad \text{KeyMode} = M
}$$

At invocation, keys are acquired using the captured reference's identity as the 
root, preserving fine-grained path analysis.

##### Dynamic Semantics

**Local Closure Invocation**

1. Analyze accessed paths using defining scope's bindings
2. Acquire keys for accessed paths
3. Execute closure body
4. Release keys

**Escaping Closure Invocation**

1. For each shared dependency $(x, T)$ in the closure's type:
   - Let $r$ be the captured reference to $x$
   - Acquire keys for paths in $\text{AccessedPaths}(C, x)$ using $\text{id}(r)$ as root
2. Execute closure body
3. Release keys

##### Constraints & Legality

**Negative Constraints**

1. An escaping closure capturing `shared` bindings MUST have an explicitly annotated 
   type or be assigned to a binding with inferred dependency type.
2. An escaping closure MUST NOT outlive any of its captured `shared` bindings.

**Diagnostic Table**

| Code         | Severity | Condition                                                    | Detection    | Effect    |
| :----------- | :------- | :----------------------------------------------------------- | :----------- | :-------- |
| `E-CON-0085` | Error    | Escaping closure with `shared` capture lacks type annotation | Compile-time | Rejection |
| `E-CON-0086` | Error    | Escaping closure outlives captured `shared` binding          | Compile-time | Rejection |
| `W-CON-0009` | Warning  | Closure captures `shared` data                               | Compile-time | Warning   |

---

### 14.4 The `#` Key Block

##### Definition

The **`#` key block** is the explicit key acquisition construct. It acquires a key to one or more paths at block entry and holds those keys until block exit.

**Mode Semantics**

| Block Form        | Key Mode | Mutation Permitted |
| :---------------- | :------- | :----------------- |
| `#path { }`       | Read     | No                 |
| `#path write { }` | Write    | Yes                |

##### Syntax & Declaration

**Grammar**

```ebnf
key_block       ::= "#" path_list mode_modifier* block

path_list       ::= path_expr ("," path_expr)*

mode_modifier   ::= "write" | "read" | release_modifier | "ordered" | "speculative"

release_modifier ::= "release" ("write" | "read")
```

**Modifier Constraints:**
- `release` MUST be followed by a target mode (`write` or `read`)
- `release write` is used for escalation (Read → Write) in nested blocks
- `release read` is used for downgrade (Write → Read) in nested blocks
- `ordered` MAY combine with any other modifiers
- `speculative` MUST be followed by `write`; speculative read-only blocks are ill-formed
- `speculative` MUST NOT combine with `release`
- Modifier order is not significant: `ordered write` ≡ `write ordered`

The `path_expr`, `key_marker`, and `index_suffix` productions are defined in §13.1.

The `release` modifier is used for nested key release (mode transitions); see §14.7.2 for semantics.

**Desugaring**

The `#` block without explicit mode modifier desugars to Read mode. The `write` modifier specifies Write mode:

$$\texttt{\#}P\ \{\ B\ \} \quad \equiv \quad \texttt{\#}P\ \texttt{with mode=Read}\ \{\ B\ \}$$

$$\texttt{\#}P\ \texttt{write}\ \{\ B\ \} \quad \equiv \quad \texttt{\#}P\ \texttt{with mode=Write}\ \{\ B\ \}$$

For multiple paths, mode applies uniformly:

$$\texttt{\#}P_1, P_2\ \texttt{write}\ \{\ B\ \} \quad \equiv \quad \texttt{\#}P_1\ \texttt{write},\ P_2\ \texttt{write}\ \{\ B\ \}$$

**Read Mode (Default)**

When no modifier is specified, the `#` block acquires Read keys, and the body MUST NOT write through keyed paths.

**Write Mode (Explicit)**

The `write` modifier acquires Write keys, allowing read–modify–write within the block.

**Multiple Paths**

`#` blocks MAY list multiple paths; keys are acquired in canonical order (§14.7).

**Ordered Mode**

The `ordered` modifier enforces deterministic acquisition for dynamic indices and MUST be used when static ordering cannot be proven.

**Speculative Mode**

`speculative write` requests optimistic execution with retry on commit failure; semantics are in §14.7.3.

**Inline Coarsening**

`#` may appear inline in a path expression to set key granularity; the mode is derived from the surrounding read/write context.

**Desugaring: Compound Assignment**

Compound assignment operators desugar to `#` blocks with `write` mode before key analysis.

##### Static Semantics

**Block Key Acquisition Rule**

**(K-Block-Acquire)**
$$\frac{
    \Gamma \vdash P_1, \ldots, P_m : \texttt{shared}\ T_i \quad
    M = \text{BlockMode}(B) \quad
    (Q_1, \ldots, Q_m) = \text{CanonicalSort}(P_1, \ldots, P_m) \quad
    S = \text{NewScope}
}{
    \Gamma_{\text{keys}}' = \Gamma_{\text{keys}} \cup \{(Q_i, M, S) : i \in 1..m\}
}$$

where $\text{BlockMode}(B) = \text{Write}$ if `write` modifier is present, otherwise $\text{Read}$.

**Read Mode Restriction**

A `#` block without `write` modifier MUST NOT contain write accesses to keyed paths:

**(K-Read-Block-No-Write)**
$$\frac{
    \text{BlockMode}(B) = \text{Read} \quad
    P \in \text{KeyedPaths}(B) \quad
    \text{WriteOf}(P) \in \text{Body}(B)
}{
    \text{Emit}(\texttt{E-CON-0070})
}$$

**Read-Then-Write Permission**

The read-then-write prohibition (`E-CON-0060`) does not apply when a covering Write key is statically held:

**(K-RMW-Permitted)**
$$\frac{
    \Gamma \vdash P : \texttt{shared}\ T \quad
    \text{ReadThenWrite}(P, S) \quad
    \exists (Q, \text{Write}, S') \in \Gamma_{\text{keys}} : \text{Prefix}(Q, P)
}{
    \text{Permitted}
}$$

**Inline Coarsening Rule**

**(K-Coarsen-Inline)**
$$\frac{
    P = p_1.\ldots.p_{k-1}.\#p_k.\ldots.p_n \quad
    \Gamma \vdash P : \texttt{shared}\ T \quad
    M = \text{RequiredMode}(P)
}{
    \text{AcquireKey}(p_1.\ldots.p_k, M, \Gamma_{\text{keys}})
}$$

##### Dynamic Semantics

**Execution Order**

When a `#` block executes:

1. Acquire keys (mode per block specification) to all listed paths in canonical order
2. Execute the block body
3. Release all keys in reverse acquisition order

**Concurrent Access**

| Block A        | Block B        | Same/Overlapping Path | Result                          |
| :------------- | :------------- | :-------------------- | :------------------------------ |
| `#p { }`       | `#p { }`       | Yes                   | Both proceed concurrently       |
| `#p { }`       | `#p write { }` | Yes                   | One blocks until other releases |
| `#p write { }` | `#p write { }` | Yes                   | One blocks until other releases |

##### Constraints & Legality

**Negative Constraints**

1. A `#` block without `write` modifier MUST NOT contain write operations to keyed paths.
2. A program MUST NOT place `#` immediately before a method name.
3. A program MUST NOT use `#` block syntax with a non-`shared` path.

**Method Receiver Coarsening**

To coarsen the key path when accessing `shared` data through a method call, place the `#` marker on the receiver expression, not on the method name:

| Expression                | Key Path       | Validity               |
| :------------------------ | :------------- | :--------------------- |
| `#player.get_health()`    | `player`       | Valid                  |
| `player.#get_health()`    | —              | Invalid (`E-CON-0030`) |
| `#player.stats.compute()` | `player`       | Valid                  |
| `player.#stats.compute()` | `player.stats` | Valid                  |

The `#` marker coarsens from that point in the path; subsequent segments (including method calls) are covered by the coarsened key.

**Diagnostic Table**

| Code         | Severity | Condition                                                  | Detection    | Effect    |
| :----------- | :------- | :--------------------------------------------------------- | :----------- | :-------- |
| `E-CON-0014` | Error    | `ordered` modifier on paths with different array bases     | Compile-time | Rejection |
| `E-CON-0030` | Error    | `#` immediately before method name                         | Compile-time | Rejection |
| `E-CON-0031` | Error    | `#` block path not in scope                                | Compile-time | Rejection |
| `E-CON-0032` | Error    | `#` block path is not `shared`                             | Compile-time | Rejection |
| `E-CON-0070` | Error    | Write operation in `#` block without `write` modifier      | Compile-time | Rejection |
| `W-CON-0003` | Warning  | `#` redundant (matches type boundary)                      | Compile-time | Warning   |
| `W-CON-0013` | Warning  | `ordered` modifier used with statically-comparable indices | Compile-time | Warning   |

---

### 14.5 Type-Level Key Boundaries

##### Definition

A **type-level key boundary** is a permanent granularity constraint declared on a record field. All access paths through that field use the field as the minimum key granularity.

##### Syntax & Declaration

```ebnf
field_decl      ::= "#"? visibility? IDENTIFIER ":" type
```

```cursive
record Player {
    health: i32,              // Fine-grained
    mana: i32,                // Fine-grained
    #inventory: [Item; 20],   // Boundary: always keys to player.inventory
}
```

##### Static Semantics

**Boundary Propagation Rule**

**(K-Type-Boundary)**

Let $R$ be a record type with field $f$ declared as `#f: U`. For any path $P = r.f.q_1.\ldots.q_n$ where $r : R$:

$$\text{KeyPath}(P) = r.f$$

regardless of the suffix $q_1.\ldots.q_n$.

**Formal Rule:**

$$\frac{
    \Gamma \vdash r : R \quad
    R.\text{fields} \ni (\#f : U) \quad
    P = r.f.q_1.\ldots.q_n
}{
    \text{KeyPath}(P) = r.f
}$$

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                        | Detection    | Effect    |
| :----------- | :------- | :----------------------------------------------- | :----------- | :-------- |
| `E-CON-0033` | Error    | `#` on field of non-record type                  | Compile-time | Rejection |
| `W-CON-0003` | Warning  | Expression `#` redundant (matches type boundary) | Compile-time | Warning   |

---

### 14.6 Static Index Resolution and Disjointness Proofs

##### Definition

When multiple dynamic indices access the same array within a single statement, they MUST be provably disjoint (per §14.1.1); otherwise, the program is ill-formed (`E-CON-0010`). Same-statement accesses cannot be serialized by runtime key acquisition—they occur as part of one atomic key-holding scope.

##### Static Semantics

**Same-Statement Conflict Detection**

For two index expressions $e_1$ and $e_2$ accessing the same array within a single statement, classification follows the provable disjointness rules from §14.1.1:

| Classification          | Condition (per §14.1.1)                                  | Effect                       |
| :---------------------- | :------------------------------------------------------- | :--------------------------- |
| Statically Disjoint     | $\text{ProvablyDisjoint}(e_1, e_2)$ via static values    | Fine-grained keys permitted  |
| Provably Disjoint       | $\text{ProvablyDisjoint}(e_1, e_2)$ via any §14.1.1 rule | Fine-grained keys permitted  |
| Potentially Overlapping | $\neg\text{ProvablyDisjoint}(e_1, e_2)$                  | **Rejection** (`E-CON-0010`) |

---

#### 13.6.1 Dynamic Index Conflict Detection

##### Definition

When multiple dynamic indices access the same array within a single statement, they MUST be provably disjoint (per §14.1.1); otherwise, the program is ill-formed (`E-CON-0010`). Same-statement accesses cannot be serialized by runtime key acquisition—they occur as part of one atomic key-holding scope.

##### Static Semantics

**Same-Statement Conflict Detection**

For two index expressions $e_1$ and $e_2$ accessing the same array within a single statement, classification follows the provable disjointness rules from §14.1.1:

| Classification          | Condition (per §14.1.1)                                  | Effect                       |
| :---------------------- | :------------------------------------------------------- | :--------------------------- |
| Statically Disjoint     | $\text{ProvablyDisjoint}(e_1, e_2)$ via static values    | Fine-grained keys permitted  |
| Provably Disjoint       | $\text{ProvablyDisjoint}(e_1, e_2)$ via any §14.1.1 rule | Fine-grained keys permitted  |
| Potentially Overlapping | $\neg\text{ProvablyDisjoint}(e_1, e_2)$                  | **Rejection** (`E-CON-0010`) |

This table applies regardless of `[[dynamic]]` scope. Same-statement conflicts are always ill-formed.

**(K-Dynamic-Index-Conflict)**
$$\frac{
    P_1 = a[e_1] \quad P_2 = a[e_2] \quad
    \text{SameStatement}(P_1, P_2) \quad
    (\text{Dynamic}(e_1) \lor \text{Dynamic}(e_2)) \quad
    \neg\text{ProvablyDisjoint}(e_1, e_2)
}{
    \text{Emit}(\texttt{E-CON-0010})
}$$

**Cross-Statement Dynamic Conflicts**

For dynamic index accesses in *different* statements, `[[dynamic]]` permits runtime serialization:

```cursive
[[dynamic]]
procedure swap(arr: shared [i32], i: usize, j: usize) {
    let temp = arr[i]    // Statement 1: runtime key on arr[i]
    arr[i] = arr[j]      // Statement 2: runtime keys, serialized
    arr[j] = temp        // Statement 3: runtime key on arr[j]
    // OK: different statements, runtime serialization works
}

procedure same_statement_conflict(arr: shared [i32], i: usize, j: usize) {
    arr[i] = arr[j]      // Error E-CON-0010: same statement, cannot serialize
    // [[dynamic]] does not help here
}
```

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                              | Detection    | Effect    |
| :----------- | :------- | :----------------------------------------------------- | :----------- | :-------- |
| `E-CON-0010` | Error    | Potential conflict on dynamic indices (same statement) | Compile-time | Rejection |

---

### 14.7 Deadlock Prevention

##### Definition

Cursive prevents deadlock through:

1. **Scoped keys:** Keys are released at scope exit, limiting hold duration
2. **Canonical ordering:** `#` blocks acquire keys in deterministic order
3. **Await prohibition:** Keys cannot be held across suspension points

##### Static Semantics

**Canonical Order**

A **canonical order** on paths ensures deterministic acquisition order to prevent circular wait conditions.

For paths $P = p_1.\ldots.p_m$ and $Q = q_1.\ldots.q_n$:

$$P <_{\text{canon}} Q \iff \exists k \geq 1 : (\forall i < k,\ p_i =_{\text{seg}} q_i) \land (p_k <_{\text{seg}} q_k \lor (k > m \land m < n))$$

**Segment Ordering ($<_{\text{seg}}$):**

1. **Identifiers:** Lexicographic by UTF-8 byte sequence
2. **Indexed segments:** By index value for statically resolvable indices ($a[i] <_{\text{seg}} a[j] \iff \text{StaticValue}(i) < \text{StaticValue}(j)$)
3. **Bare vs indexed:** Bare identifier precedes any indexed form ($a <_{\text{seg}} a[i]$ for all $i$)

**Dynamic Index Ordering**

If indices $i$ and $j$ in segments $a[i]$ and $a[j]$ cannot be statically compared:

1. If $i$ and $j$ are the same binding: $a[i] =_{\text{seg}} a[i]$ (same segment)
2. Otherwise, within a `[[dynamic]]` scope: ordering is determined at runtime per the **Dynamic Ordering Guarantee** below
3. Otherwise, outside `[[dynamic]]` scope: the program is ill-formed (`E-CON-0020`)

**Atomic Key Acquisition**

When a `#` block lists multiple paths, the implementation acquires keys to all listed paths before executing the block body. This acquisition is **atomic** in the following sense:

**Formal Definition**

Let $\text{Paths} = \{P_1, \ldots, P_n\}$ be the paths listed in a `#` block, sorted into canonical order as $Q_1, \ldots, Q_n$. Atomic acquisition proceeds as follows:

1. For $i = 1$ to $n$:
   - Attempt to acquire key $(Q_i, M, S)$
   - If a conflicting key is held by another task, **block** until the conflict is resolved
   - Keys $Q_1, \ldots, Q_{i-1}$ remain held during blocking

2. Once all keys are acquired, execute the block body.

**Properties**

| Property             | Guarantee                                                          |
| :------------------- | :----------------------------------------------------------------- |
| No rollback          | Partially-acquired keys are retained during blocking, not released |
| Eventual acquisition | Each key is eventually acquired (per progress guarantee, §14.1.2)  |
| Order preservation   | Keys are acquired in canonical order                               |
| Body precondition    | Block body executes only after all keys are held                   |

**Dynamic Ordering Guarantee**

When a `#` block within a `[[dynamic]]` scope contains paths with incomparable dynamic indices, the runtime MUST determine an acquisition order satisfying the following properties:

**Required Properties**

Let $\text{DynOrder}(P, Q)$ denote the runtime ordering relation between paths $P$ and $Q$:

1. **Totality**: For any two distinct paths $P$ and $Q$, exactly one of $\text{DynOrder}(P, Q)$ or $\text{DynOrder}(Q, P)$ holds.

2. **Antisymmetry**: $\text{DynOrder}(P, Q) \land \text{DynOrder}(Q, P) \implies P = Q$

3. **Transitivity**: $\text{DynOrder}(P, Q) \land \text{DynOrder}(Q, R) \implies \text{DynOrder}(P, R)$

4. **Cross-Task Consistency**: For any paths $P$ and $Q$, all tasks compute the same value for $\text{DynOrder}(P, Q)$.

5. **Value-Determinism**: $\text{DynOrder}(P, Q)$ depends only on the runtime values (indices, addresses) of $P$ and $Q$, not on task identity, timing, or execution history.

These properties ensure that $\text{DynOrder}$ is a total order consistent across all tasks, eliminating circular wait conditions.

**Same-Element Coalescing**

When two paths resolve to the same memory location at runtime, the implementation MUST acquire only one key:

$\text{SameLocation}(P, Q) \implies \text{AcquireOnce}(P)$

**Ordering Mechanism (Implementation-Defined)**

The mechanism for computing $\text{DynOrder}$ is Implementation-Defined Behavior (IDB). The implementation MUST satisfy the cross-task consistency property.

**Deadlock Avoidance**

Canonical ordering ensures that all tasks acquiring the same set of keys do so in the same order, preventing circular wait conditions. A task holding keys $Q_1, \ldots, Q_{i-1}$ while blocking on $Q_i$ cannot be blocked by another task that needs $Q_j$ where $j < i$, because canonical ordering guarantees that task would have acquired $Q_j$ before $Q_i$.

**Single-Task Guarantee**

A single task cannot deadlock with itself. All keys in a `#` block are acquired sequentially by one task; the task either:
1. Acquires all keys and proceeds, or
2. Blocks on a key held by another task (not itself)

**Statement Keys: Evaluation Order**

Keys within a single statement are acquired in **evaluation order** (left-to-right, depth-first). This is Defined Behavior.

**`#` Block Keys: Canonical Order**

Keys in a `#` block MUST be acquired in canonical order, regardless of the order listed:

```cursive
#zebra, alpha, mango {
    // Acquired: alpha, mango, zebra (canonical order)
}
```

**(K-Block-Canonical)**
$$\frac{
    \text{Paths} = \{P_1, \ldots, P_n\} \quad
    (Q_1, \ldots, Q_n) = \text{CanonicalSort}(\text{Paths})
}{
    \text{AcquireSequence} = [Q_1, Q_2, \ldots, Q_n]
}$$

**Nested `#` Block Semantics**

Nested `#` blocks acquire keys independently. The outer block's keys remain held while the inner block executes:

```cursive
#alpha {                    // Acquires: alpha
    // alpha held
    #beta {                 // Acquires: beta (alpha still held)
        // Both alpha and beta held
    }                       // Releases: beta
    // alpha still held
}                           // Releases: alpha
```

**Nested `#` Block on Same or Overlapping Path**

When a nested `#` block specifies a path that overlaps with an outer `#` block's path:

1. **Identical path, same mode:** The inner block's key request is covered by the outer block's key. No additional acquisition occurs.

2. **Identical path, different mode (escalation or downgrade):** If the outer and inner blocks request different modes, the inner block MUST use the `release` modifier with a target mode. This triggers release-and-reacquire semantics as defined in §14.7.2. The `release` keyword is required to acknowledge that interleaving may occur between the release of the outer key and acquisition of the inner key.

   - `release write`: Escalation (Read → Write)
   - `release read`: Downgrade (Write → Read)

   Omitting the `release` modifier when changing mode is a compile-time error (`E-CON-0012`).

   Using `release` with a target mode matching the outer mode is a compile-time error (`E-CON-0018`).

3. **Prefix relationship:** If the inner path is a prefix of the outer path (coarser granularity), the inner block acquires a separate, coarser key. If the outer path is a prefix of the inner path (finer granularity), the inner block's access is covered by the outer key.

**(K-Nested-Same-Path)**
$$\frac{
    \text{Held}(P, M_{\text{outer}}, \Gamma_{\text{keys}}) \quad
    \#P\ M_{\text{inner}}\ \{\ \ldots\ \}
}{
    \begin{cases}
    \text{Covered (no acquisition)} & \text{if } M_{\text{inner}} = M_{\text{outer}} \\
    \text{Release-and-Reacquire per §14.7.2} & \text{if } M_{\text{inner}} \neq M_{\text{outer}} \land \texttt{release} \in \text{modifiers} \\
    \text{Emit}(\texttt{E-CON-0012}) & \text{if } M_{\text{inner}} \neq M_{\text{outer}} \land \texttt{release} \notin \text{modifiers}
    \end{cases}
}$$

**Nested Block Ordering:**

Each `#` block orders its own listed paths canonically. The relative order of keys between nesting levels is determined by nesting structure (outer acquired before inner).

**Deadlock Hazard from Inconsistent Nesting:**

Nested `#` blocks that acquire overlapping keys in different orders across tasks create potential deadlock. The programmer MUST ensure consistent nesting order across tasks. This pattern SHOULD be detected when both blocks are visible:

**(K-Nested-Cycle-Detection)**
$$\frac{
    \text{Block}_1 : \#P_1\ \{\ \#P_2\ \{\ \ldots\ \}\ \} \quad
    \text{Block}_2 : \#P_2\ \{\ \#P_1\ \{\ \ldots\ \}\ \} \quad
    \neg\text{Disjoint}(P_1, P_2)
}{
    \text{Emit}(\texttt{W-CON-0012})
}$$

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                               | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------------------ | :----------- | :-------- |
| `E-CON-0011` | Error    | Detectable key ordering cycle within procedure          | Compile-time | Rejection |
| `W-CON-0012` | Warning  | Nested `#` blocks with potential order cycle            | Compile-time | Warning   |
| `I-CON-0011` | Info     | `#` block uses runtime ordering for dynamic indices     | Compile-time | Advisory  |
| `I-CON-0013` | Info     | `#` block coarsened due to incomparable dynamic indices | Compile-time | Advisory  |

#### 13.7.1 Read-Then-Write Prohibition

##### Definition

The **read-then-write prohibition** prevents patterns where a `shared` path is read and then written within the same statement without holding a covering Write key.

**Formal Definition**

Let $\text{Subexpressions}(S)$ denote the set of all subexpressions within statement $S$. Define:

$$\text{ReadsPath}(e, P) \iff \text{Accesses}(e, P) \land \text{ReadContext}(e)$$

$$\text{WritesPath}(e, P) \iff \text{Accesses}(e, P) \land \text{WriteContext}(e)$$

where $\text{Accesses}(e, P)$ holds when expression $e$ accesses path $P$ (directly or through a prefix), and $\text{ReadContext}$ and $\text{WriteContext}$ are defined in §14.1.2.

A **read-then-write pattern** occurs when a statement contains both a read and a write to the same `shared` path:

$$\text{ReadThenWrite}(P, S) \iff \exists e_r, e_w \in \text{Subexpressions}(S) : \text{ReadsPath}(e_r, P) \land \text{WritesPath}(e_w, P)$$

##### Static Semantics

**Prohibition Rule**

Read-then-write patterns MUST be rejected when no covering Write key is statically held:

**(K-Read-Write-Reject)**
$$\frac{
    \Gamma \vdash P : \texttt{shared}\ T \quad
    \text{ReadThenWrite}(P, S) \quad
    \neg\exists (Q, \text{Write}, S') \in \Gamma_{\text{keys}} : \text{Prefix}(Q, P)
}{
    \text{Emit}(\texttt{E-CON-0060})
}$$

**Pattern Classification**

| Pattern            | Covering Write Key?         | Classification      | Action                |
| :----------------- | :-------------------------- | :------------------ | :-------------------- |
| `p += e`           | Yes (desugars to `#` block) | Compound assignment | Permitted             |
| `#p { p = p + e }` | Yes                         | Explicit `#` block  | Permitted             |
| `p = p + e`        | No                          | Read-then-write     | **Reject E-CON-0060** |
| `p = f(p)`         | No                          | Read-then-write     | **Reject E-CON-0060** |
| `p.a = p.b + 1`    | No, but disjoint paths      | Disjoint fields     | Permitted             |

**Separate Statements**

Reading and writing the **same** `shared` path in **separate statements** is permitted. Keys are released between statements:

```cursive
let current = player.health    // Read key acquired, released at semicolon
// ... other code ...
player.health = current + 10   // Write key acquired fresh — no conflict
```

This is safe because no key is held across statements; no upgrade occurs.

**Aliasing Considerations**

When two paths cannot be proven distinct, they MUST be assumed to potentially alias:

```cursive
procedure update(a: shared Player, b: shared Player) {
    a.health = b.health + 10   // Potentially a == b — E-CON-0060
}

procedure update_distinct(a: shared Player, b: shared Player)
    |= a != b                  // Contract proves distinctness
{
    a.health = b.health + 10   // OK — proven distinct
}
```

##### Constraints & Legality

**Negative Constraints**

1. A program MUST NOT read and write the same `shared` path in the same statement without a covering Write key.
2. A program MUST NOT rely on implicit read-to-write key upgrade semantics; no such operation exists.

**Advisory Warnings**

The implementation SHOULD emit `W-CON-0006` when a programmer writes an explicit `#path write { let v = path; path = v + e }` block that could be expressed as compound assignment (`path += e`). This warning is advisory; the explicit form is valid but unnecessarily verbose.

**Diagnostic Table**

| Code         | Severity | Condition                                                                   | Detection    | Effect    |
| :----------- | :------- | :-------------------------------------------------------------------------- | :----------- | :-------- |
| `E-CON-0060` | Error    | Read-then-write on same `shared` path without covering Write key            | Compile-time | Rejection |
| `W-CON-0004` | Warning  | Read-then-write in sequential context; may cause contention if parallelized | Compile-time | Warning   |
| `W-CON-0006` | Warning  | Explicit read-then-write form used; compound assignment available           | Compile-time | Warning   |

---

#### 13.7.2 Nested Key Release

##### Definition

A nested `#` block that requests a different mode than its enclosing block performs a **key release and reacquire**. This includes both escalation (Read → Write) and downgrade (Write → Read).

The `release` keyword MUST be present to indicate the programmer understands and accepts potential interleaving between the release and reacquisition.

##### Syntax & Declaration

**Grammar**

The `release write` and `release read` modifiers are part of the `key_block` grammar defined in §14.4.

**Mode Transition Table**

| Outer Mode | `release` Target | Operation                                                          |
| ---------- | ---------------- | ------------------------------------------------------------------ |
| Read       | `write`          | Escalation: Release Read → Gap → Acquire Write                     |
| Write      | `read`           | Downgrade: Release Write → Gap → Acquire Read                      |
| X          | X                | **Error E-CON-0018**: Redundant release (already hold target mode) |

Both escalation and downgrade create **two interleaving windows**: one at block entry (release outer, acquire inner) and one at block exit (release inner, reacquire outer).

##### Static Semantics

**(K-Release-Required)**
$$\frac{
    \text{Held}(P, M_{\text{outer}}, \Gamma_{\text{keys}}) \quad
    \#Q\ M_{\text{inner}}\ \{\ \ldots\ \} \quad
    \text{Overlaps}(P, Q) \quad
    M_{\text{inner}} \neq M_{\text{outer}} \quad
    \texttt{release} \notin \text{modifiers}
}{
    \text{Emit}(\texttt{E-CON-0012})
}$$

**(K-Release-Redundant)**
$$\frac{
    \#Q\ \texttt{release}\ M\ \{\ \ldots\ \} \quad
    \exists (P, M', \_) \in \Gamma_{\text{keys}} : \text{Overlaps}(P, Q) \land M = M'
}{
    \text{Emit}(\texttt{E-CON-0018})
}$$

**(K-Release-No-Target)**
$$\frac{
    \#Q\ \texttt{release}\ \{\ \ldots\ \} \quad
    \text{no target mode specified}
}{
    \text{Emit}(\texttt{E-CON-0017})
}$$

##### Dynamic Semantics

**Release-and-Reacquire Sequence**

When entering `#path release <target> { body }` while holding a key to an overlapping path with a different mode:

1. **Release:** Release the outer key held by the enclosing block
2. **Acquire:** Acquire the target mode key to `path` (blocking if contended)
3. **Execute:** Evaluate `body`
4. **Release:** Release the inner key
5. **Reacquire:** Acquire the outer mode key for the enclosing block's remaining scope

**(K-Release-Sequence)**
$$\frac{
    \text{Held}(P, M_{\text{outer}}, S_{\text{outer}}) \quad
    \#P\ \texttt{release}\ M_{\text{inner}}\ \{\ B\ \}
}{
    \begin{array}{l}
    \text{Release}(P, \Gamma_{\text{keys}});\
    \text{Acquire}(P, M_{\text{inner}}, S_{\text{inner}});\
    \text{Eval}(B);\ \\
    \text{Release}(P, \Gamma_{\text{keys}});\
    \text{Acquire}(P, M_{\text{outer}}, S_{\text{outer}})
    \end{array}
}$$

**Reacquisition Semantics**

Step 5 (Reacquire) follows normal key acquisition semantics:

1. If another task holds a conflicting key to an overlapping path, the current task blocks until the key is released.
2. Multiple tasks MAY hold Read keys concurrently after reacquisition.
3. An interleaving window exists between steps 4 and 5, during which other tasks MAY acquire and release keys.

**Optimization: Eliding Reacquisition**

The implementation MAY elide step 5 if static analysis proves that no statements in the enclosing block's remaining scope access paths covered by the original key. This is an optimization; observable behavior MUST be equivalent to performing the reacquisition.

**(K-Release-Reacquire-Elision)**
$$\frac{
    \text{RemainingStatements}(S_{\text{outer}}) \cap \text{AccessPaths}(P) = \emptyset
}{
    \text{Step 5 MAY be elided}
}$$

**Interleaving Windows**

Between steps 1 and 2 (entry), and between steps 4 and 5 (exit), other tasks MAY:
- Acquire keys to the same path
- Modify the shared data
- Complete their operations

A binding derived from `shared` data before a `release` block is **potentially stale** after the block completes. Accessing a potentially stale binding triggers `W-CON-0011`.

**Staleness Warning Suppression**

Apply `#[stale_ok]` to a binding declaration to suppress the staleness warning when intentional (e.g., optimistic concurrency patterns):

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                         | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------------ | :----------- | :-------- |
| `E-CON-0012` | Error    | Nested mode change without `release` modifier     | Compile-time | Rejection |
| `E-CON-0017` | Error    | `release` modifier without target mode            | Compile-time | Rejection |
| `E-CON-0018` | Error    | `release` with target mode matching outer mode    | Compile-time | Rejection |
| `W-CON-0010` | Warning  | `release` block permits interleaving              | Compile-time | Warning   |
| `W-CON-0011` | Warning  | Access to potentially stale binding after release | Compile-time | Warning   |

---

#### 13.7.3 Speculative Block Semantics

##### Definition

A **speculative key block** executes without acquiring an exclusive key, instead relying on optimistic concurrency control. The block body executes against a snapshot of the keyed paths; at block exit, the implementation attempts to atomically commit all writes if and only if the snapshot values remain unchanged.

**Formal Definition**

A speculative block `#P speculative write { B }` has three components:

1. **Read set** $R(B)$: The set of $(path, value)$ pairs read during evaluation of $B$
2. **Write set** $W(B)$: The set of $(path, value)$ pairs to be written by $B$
3. **Commit predicate**: $\bigwedge_{(p,v) \in R(B)} (\text{current}(p) = v)$

$$\text{SpeculativeCommit}(R, W) \iff \text{Atomic}\left(\bigwedge_{(p,v) \in R} p = v \implies \bigwedge_{(q,w) \in W} q := w\right)$$

##### Syntax & Declaration

**Grammar**

The `speculative write` modifier is part of the `key_block` grammar defined in §14.4.

##### Static Semantics

**Well-Formedness Rules**

A speculative block is well-formed if and only if all of the following conditions hold:

**(K-Spec-Write-Required)**
$$\frac{
    \#P\ \texttt{speculative}\ M\ \{B\} \quad M \neq \texttt{write}
}{
    \text{Emit}(\texttt{E-CON-0095})
}$$

**(K-Spec-Pure-Body)**

The block body MUST be **effect-pure** with respect to paths outside the keyed set. Formally:

$$\frac{
    \#P\ \texttt{speculative write}\ \{B\} \quad
    \text{Writes}(B) \not\subseteq \text{CoveredPaths}(P)
}{
    \text{Emit}(\texttt{E-CON-0091})
}$$

where $\text{CoveredPaths}(P)$ is the set of all paths for which $P$ is a prefix.

**Permitted operations** within a speculative block:
- Read from keyed paths
- Write to keyed paths
- Pure computation (arithmetic, logic, local bindings)
- Procedure calls to `const` receiver methods on keyed data
- Procedure calls to pure procedures (no `shared` or `unique` parameters)

**Prohibited operations** within a speculative block:
- Write to paths outside the keyed set
- Nested key blocks (speculative or blocking)
- `wait` expressions
- Procedure calls with side effects
- `defer` statements
- Panic-inducing operations (bounds checks are permitted; panic aborts the speculation)

**(K-Spec-No-Nested-Key)**
$$\frac{
    \#P\ \texttt{speculative write}\ \{B\} \quad
    \#Q\ \_\ \{\ldots\} \in \text{Subexpressions}(B)
}{
    \text{Emit}(\texttt{E-CON-0090})
}$$

**(K-Spec-No-Wait)**
$$\frac{
    \#P\ \texttt{speculative write}\ \{B\} \quad
    \texttt{wait}\ e \in \text{Subexpressions}(B)
}{
    \text{Emit}(\texttt{E-CON-0092})
}$$

**(K-Spec-No-Defer)**
$$\frac{
    \#P\ \texttt{speculative write}\ \{B\} \quad
    \texttt{defer}\ \{\ldots\} \in \text{Statements}(B)
}{
    \text{Emit}(\texttt{E-CON-0093})
}$$

**(K-Spec-No-Release)**
$$\frac{
    \#P\ \texttt{speculative release}\ M\ \{B\}
}{
    \text{Emit}(\texttt{E-CON-0094})
}$$

**Type of Speculative Block**

A speculative block has the same type as its body expression:

$$\frac{
    \Gamma \vdash B : T
}{
    \Gamma \vdash \#P\ \texttt{speculative write}\ \{B\} : T
}$$

The block is an expression and may produce a value. The value is that of the successful execution.

##### Dynamic Semantics

**Execution Model**

Evaluation of `#path speculative write { body }` proceeds as follows:

1. **Initialize**: Set $\text{retries} := 0$
2. **Snapshot**: Read all paths in the keyed set, recording $(path, value)$ pairs in the read set $R$
3. **Execute**: Evaluate $body$, collecting writes in the write set $W$
4. **Commit**: Atomically verify $\bigwedge_{(p,v) \in R} (\text{current}(p) = v)$ and, if true, apply all writes in $W$
5. **On success**: Return the value of $body$
6. **On failure**:
   - Increment $\text{retries}$
   - If $\text{retries} < \text{MAX\_SPECULATIVE\_RETRIES}$: goto step 2
   - Otherwise: proceed to fallback (step 7)
7. **Fallback**: Acquire a blocking Write key (per §14.2), execute $body$, release key, return value

**Retry Limit**

The constant $\text{MAX\_SPECULATIVE\_RETRIES}$ is IDB.

**Snapshot Granularity**

The snapshot (step 2) captures the state of keyed paths at a single instant. The mechanism is Implementation-Defined:

| Data Type             | Permitted Snapshot Mechanism                    |
| :-------------------- | :---------------------------------------------- |
| Primitive (≤ 8 bytes) | Atomic load                                     |
| Small struct          | Seqlock read or atomic load of version + fields |
| Large struct          | Copy under brief lock or seqlock                |

**Commit Atomicity**

The commit operation (step 4) MUST be atomic with respect to all other key operations (speculative or blocking) on overlapping paths. The mechanism is Implementation-Defined:

| Data Type                  | Permitted Commit Mechanism           |
| :------------------------- | :----------------------------------- |
| Single primitive           | Compare-and-swap                     |
| Single pointer-sized value | Compare-and-swap                     |
| Two pointer-sized values   | Double-word CAS (DCAS) or lock-based |
| Struct with version field  | CAS on version + write fields        |
| Large struct               | Acquire lock, verify, write, release |

**Interaction with Blocking Keys**

Speculative blocks and blocking key blocks operate on the same underlying synchronization state:

- A blocking Write key acquisition waits for in-flight speculative commits to complete
- A speculative commit fails if a blocking Write key is held
- Multiple concurrent speculative blocks may race; at most one succeeds per commit attempt

| Concurrent Operation      | Speculative Block Behavior                  |
| :------------------------ | :------------------------------------------ |
| Another speculative block | Race; one commits, others retry             |
| Blocking Read key held    | Speculative may commit (compatible)         |
| Blocking Write key held   | Speculative commit fails, retry or fallback |

**Panic During Speculation**

If a panic occurs during step 3 (Execute):

1. The write set $W$ is discarded (no writes are committed)
2. No key is held (nothing to release)
3. The panic propagates normally

**Memory Ordering**

Speculative blocks use the following memory ordering:

- Snapshot reads: Acquire semantics
- Successful commit: Release semantics
- Failed commit: No ordering guarantees (writes discarded)

Memory ordering annotations (`[[relaxed]]`, etc.) MUST NOT appear inside speculative blocks. The implementation controls ordering to ensure correctness.

##### Memory & Layout

**Version Counters**

To implement speculative commits on non-primitive data, implementations MAY add hidden version counters to `shared` values. This is Implementation-Defined Behavior:

| Aspect                         | Specification                       |
| :----------------------------- | :---------------------------------- |
| Presence of version field      | IDB                                 |
| Size of version field          | IDB; if present, MUST be documented |
| Location (inline vs. external) | IDB                                 |

When version counters are used:
- Increment version on every successful Write key release or speculative commit
- Speculative snapshot records version
- Speculative commit verifies version unchanged

**No-Overhead Elision**

If a `shared` value is provably never accessed speculatively (no `speculative write` blocks target it), the implementation MAY omit version counter overhead. This analysis is Implementation-Defined.

##### Constraints & Legality

**Negative Constraints**

1. A speculative block MUST include the `write` modifier.
2. A speculative block MUST NOT contain nested key blocks.
3. A speculative block MUST NOT contain `wait` expressions.
4. A speculative block MUST NOT contain `defer` statements.
5. A speculative block MUST NOT write to paths outside the keyed set.
6. A speculative block MUST NOT combine with the `release` modifier.
7. Memory ordering annotations MUST NOT appear inside speculative blocks.

**Diagnostic Table**

| Code         | Severity | Condition                                              | Detection    | Effect    |
| :----------- | :------- | :----------------------------------------------------- | :----------- | :-------- |
| `E-CON-0090` | Error    | Nested key block inside speculative block              | Compile-time | Rejection |
| `E-CON-0091` | Error    | Write to path outside keyed set in speculative block   | Compile-time | Rejection |
| `E-CON-0092` | Error    | `wait` expression inside speculative block             | Compile-time | Rejection |
| `E-CON-0093` | Error    | `defer` statement inside speculative block             | Compile-time | Rejection |
| `E-CON-0094` | Error    | `speculative` combined with `release`                  | Compile-time | Rejection |
| `E-CON-0095` | Error    | `speculative` without `write` modifier                 | Compile-time | Rejection |
| `E-CON-0096` | Error    | Memory ordering annotation inside speculative block    | Compile-time | Rejection |
| `W-CON-0020` | Warning  | Speculative block on large struct (may be inefficient) | Compile-time | Warning   |
| `W-CON-0021` | Warning  | Speculative block body may be expensive to re-execute  | Compile-time | Warning   |

---

### 14.8 Keys and Async Suspension

##### Definition

Keys MUST NOT be held across async suspension points. This is enforced at compile time.

> **Cross-Reference:** Async suspension points are defined by the `yield` and `yield from` expressions in §15.4.2 and §15.4.3. The normative prohibition rule is specified in §15.11.2 (Permission and Capture Rules).

##### Static Semantics

The diagnostic `E-CON-0213` is emitted when `yield` or `yield from` occurs while keys are logically held.

**Scope Interaction**

The prohibition applies to keys held at the suspension program point, not to keys in enclosing scopes that have already been released:

```cursive
// VALID: Key released before yield
let health = #player { player.health }  // Key acquired and released
yield health                             // No keys held at yield point

// INVALID: Key held at yield point
#player {
    yield player.health    // ERROR E-CON-0213
}
```

##### Constraints & Legality


---

### 14.9 Compile-Time Verification and Runtime Realization

##### Definition

Keys are a **compile-time abstraction**. Static key analysis verifies safe access patterns. Runtime synchronization is emitted **only when** the `[[dynamic]]` attribute is present and static verification fails.

**The Verification Hierarchy**

| Level                       | Mechanism                 | Runtime Cost | Requirement                |
| :-------------------------- | :------------------------ | :----------- | :------------------------- |
| **Static Proof**            | Safety proven statically  | Zero         | **Default (mandatory)**    |
| **Runtime Synchronization** | Static proof insufficient | Non-zero     | **Requires `[[dynamic]]`** |

##### Static Semantics

**Compile-Time Key Analysis**

Key analysis MUST be performed for all `shared` path accesses:

1. Compute the key path (per §14.4, §13.5)
2. Determine the required mode (per §14.1.2)
3. Track key state through control flow
4. Verify no conflicting accesses
5. Determine if static safety can be proven

**Static Safety Conditions**

An access is **statically safe** if one of the following conditions is proven:

| Condition              | Description                                                       | Rule   |
| :--------------------- | :---------------------------------------------------------------- | :----- |
| **No escape**          | `shared` value never escapes to another task                      | K-SS-1 |
| **Disjoint paths**     | Concurrent accesses target provably disjoint paths                | K-SS-2 |
| **Sequential context** | No `parallel` block encloses the access                           | K-SS-3 |
| **Unique origin**      | Value is `unique` at origin, temporarily viewed as `shared`       | K-SS-4 |
| **Dispatch-indexed**   | Access indexed by `dispatch` iteration variable                   | K-SS-5 |
| **Speculative-only**   | All accesses are within speculative blocks and fallback permitted | K-SS-6 |

**(K-Static-Safe)**
$$\frac{
    \text{Access}(P, M) \quad
    \text{StaticallySafe}(P)
}{
    \text{NoRuntimeSync}(P)
}$$

**Default: Static Proof Required**

When static safety cannot be proven and the access is not within a `[[dynamic]]` scope (§7.2.9), the program is ill-formed:

**(K-Static-Required)**
$$\frac{
    \neg\text{StaticallySafe}(P) \quad
    \neg\text{InDynamicContext}
}{
    \text{Emit}(\texttt{E-CON-0020})
}$$

**With `[[dynamic]]`: Runtime Synchronization Permitted**

When static safety cannot be proven but the access is within a `[[dynamic]]` scope, runtime synchronization is emitted:

**(K-Dynamic-Permitted)**
$$\frac{
    \neg\text{StaticallySafe}(P) \quad
    \text{InDynamicContext}
}{
    \text{EmitRuntimeSync}(P)
}$$

##### Dynamic Semantics

**Runtime Realization**

When runtime synchronization is required, the implementation MUST provide:

1. Mutual exclusion per key compatibility rules (§14.1.2)
2. Blocking when incompatible keys are held
3. Release on scope exit (including panic)
4. Progress guarantee (no indefinite starvation)

**Runtime-Ordered Key Acquisition**

When a `#` block within a `[[dynamic]]` scope contains incomparable dynamic indices:

1. All index expressions are evaluated.
2. Acquisition order is determined (mechanism is IDB).
3. Key acquisition proceeds in the computed order.

**Observational Equivalence**

Program behavior MUST be identical whether safety is proven statically or enforced at runtime. The only permitted difference is performance:

$$\forall P : \text{Observable}(\text{StaticSafe}(P)) = \text{Observable}(\text{RuntimeSync}(P))$$

##### Memory & Layout

**Static Case — No Overhead**

When all accesses to a `shared` value are statically safe:
- No synchronization metadata is required
- Layout MAY be identical to `unique` equivalent
- No runtime code is generated for key operations

**Runtime Case — With `[[dynamic]]`**

When runtime synchronization is permitted via `[[dynamic]]`: 
- Synchronization metadata is added to the `shared` value
- Layout is implementation-defined
- Runtime code is generated for key operations

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                                        | Detection    | Effect    |
| :----------- | :------- | :--------------------------------------------------------------- | :----------- | :-------- |
| `E-CON-0020` | Error    | Key safety not statically provable outside `[[dynamic]]`         | Compile-time | Rejection |
| `W-CON-0021` | Warning  | `[[dynamic]]` present but all key operations are statically safe | Compile-time | Advisory  |

### 14.10 Memory Ordering

##### Definition

**Default: Sequential Consistency**

Key acquisition uses **acquire** semantics. Key release uses **release** semantics. Memory accesses default to **sequentially consistent** ordering.

##### Syntax & Declaration

**Relaxed Ordering**

For performance-critical code, weaker orderings are available via annotation:

```cursive
[[relaxed]]
counter += 1

[[acquire]]
let ready = flag

[[release]]
flag = true
```

**Ordering Levels**

| Ordering  | Guarantee                             |
| :-------- | :------------------------------------ |
| `relaxed` | Atomicity only—no ordering            |
| `acquire` | Subsequent reads see prior writes     |
| `release` | Prior writes visible to acquire reads |
| `acq_rel` | Both acquire and release              |
| `seq_cst` | Total global order (default)          |

The annotation affects the memory operation, not the key synchronization. Keys always use acquire/release semantics.

**Ordering Annotations and `#` Blocks**

Memory ordering annotations (including `[[relaxed]]`) are **permitted** inside standard `#` blocks. The annotation affects only the data access ordering within the block; it does not modify the key's acquire/release semantics at block entry and exit.

```cursive
#counter write {
    [[relaxed]] counter += 1   // Relaxed increment, but key still uses acquire/release
}
```

In contrast, memory ordering annotations MUST NOT appear inside **speculative blocks** (§14.9), where the implementation controls all ordering to ensure rollback correctness.

**Fence Operations**

```cursive
fence(acquire)       // Acquire fence
fence(release)       // Release fence
fence(seq_cst)       // Full barrier
```

---

## Clause 15: Structured Parallelism

### 15.1 Parallelism Overview

##### Definition

**Formal Definition**

Let $\mathcal{W}$ denote the set of work items spawned within a parallel block $P$. The **structured concurrency invariant** states:

$$\forall w \in \mathcal{W}.\ \text{lifetime}(w) \subseteq \text{lifetime}(P)$$

A **work item** is a unit of computation queued for execution by a worker. Work items are created by `spawn` and `dispatch` expressions. A **worker** is an execution context (such as an OS thread or GPU compute unit) that executes work items.

**Task** is the runtime representation of a work item during execution. A task may suspend (at `wait` expressions) and resume. Tasks are associated with key state per §13.1.

---

### 15.2 The Parallel Block

##### Definition

A **parallel block** establishes a scope within which work may execute concurrently across multiple workers. The block specifies an execution domain and contains `spawn` and `dispatch` expressions that create work items. Per the structured concurrency invariant (§14.1), all work items complete before execution proceeds past the block's closing brace.

**Formal Definition**

A parallel block $P$ is defined as a tuple:

$$P = (D, A, B)$$

where:
- $D$ is the execution domain expression
- $A$ is the (possibly empty) set of block options
- $B$ is the block body containing statements and work-creating expressions

##### Syntax & Declaration

**Grammar**

```ebnf
parallel_block    ::= "parallel" domain_expr block_options? block

domain_expr       ::= expression

block_options     ::= "[" block_option ("," block_option)* "]"

block_option      ::= "cancel" ":" expression
                    | "name" ":" string_literal

block             ::= "{" statement* "}"
```

The `domain_expr` MUST evaluate to a type that implements the `ExecutionDomain` form (§14.6).


**Block Option Semantics**

| Option   | Type          | Purpose                                      |
| :------- | :------------ | :------------------------------------------- |
| `cancel` | `CancelToken` | Provides cooperative cancellation capability |
| `name`   | `string`      | Labels the block for debugging and profiling |

##### Static Semantics

**Typing Rule**

$$\frac{
  \Gamma \vdash D : \text{dyn ExecutionDomain} \quad
  \Gamma_P = \Gamma[\text{parallel\_context} \mapsto D] \quad
  \Gamma_P \vdash B : T
}{
  \Gamma \vdash \texttt{parallel } D\ \{B\} : T
} \quad \text{(T-Parallel)}$$

The type of a parallel block is determined by its contents per §14.9.

**Well-Formedness**

A parallel block is well-formed if and only if:

1. The domain expression evaluates to a type implementing `ExecutionDomain`.
2. All `spawn` and `dispatch` expressions within the block body reference the enclosing parallel block.
3. No `spawn` or `dispatch` expression appears outside a parallel block.
4. All captured bindings satisfy the permission requirements of §14.3.

##### Dynamic Semantics

**Evaluation**

Evaluation of `parallel D [opts] { B }` proceeds as follows:

1. Let $d$ be the result of evaluating domain expression $D$.
2. Initialize the worker pool according to $d$'s configuration.
3. If the `cancel` option is present, associate its token with the block.
4. Evaluate statements in $B$ sequentially; `spawn` and `dispatch` expressions enqueue work items to the worker pool.
5. Block at the closing brace until all enqueued work items complete.
6. If any work item panicked, propagate the first panic (by completion order) after all work settles.
7. Release workers back to the domain.
8. Return the collected results (per §14.9).

**Completion Ordering**

Work items complete in Unspecified order. The parallel block guarantees only that ALL work completes before the block exits.

##### Constraints & Legality

**Negative Constraints**

1. A `spawn` expression MUST NOT appear outside a parallel block.
2. A `dispatch` expression MUST NOT appear outside a parallel block.
3. Parallel blocks MUST NOT be nested within `async` procedure bodies without an intervening `wait` expression.

**Diagnostic Table**

| Code         | Severity | Condition                              | Detection    | Effect    |
| :----------- | :------- | :------------------------------------- | :----------- | :-------- |
| `E-CON-0101` | Error    | `spawn` or `dispatch` outside parallel | Compile-time | Rejection |
| `E-CON-0102` | Error    | Domain expression not ExecutionDomain  | Compile-time | Rejection |
| `E-CON-0103` | Error    | Invalid domain parameter type          | Compile-time | Rejection |

---

### 15.3 Capture Semantics

##### Definition

When a `spawn` or `dispatch` body references bindings from enclosing scopes, those bindings are **captured** into the work item. The capture rules for `spawn` and `dispatch` bodies follow the same semantics as closure capture, defined in **§11.5 (Closure Expressions)**.

##### Static Semantics

**Lifetime Safety**

Captured references are guaranteed valid because structured concurrency (§14.1) ensures all work completes before the parallel block exits. This distinguishes parallel capture from closure capture: closures may escape their defining scope, requiring move semantics for all non-`Copy` captures. Parallel work items enable safe reference capture for `const` and `shared` bindings.

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                 | Detection    | Effect    |
| :----------- | :------- | :---------------------------------------- | :----------- | :-------- |
| `E-CON-0120` | Error    | Implicit capture of `unique` binding      | Compile-time | Rejection |
| `E-CON-0121` | Error    | Move of already-moved binding             | Compile-time | Rejection |
| `E-CON-0122` | Error    | Move of binding from outer parallel scope | Compile-time | Rejection |

---

### 15.4 The Spawn Expression

##### Definition

A **spawn expression** creates a work item for concurrent execution within the enclosing parallel block. The work item executes independently and produces a result that may be collected.

##### Syntax & Declaration

**Grammar**

```ebnf
spawn_expr      ::= "spawn" attribute_list? block

attribute_list  ::= "[" attribute ("," attribute)* "]"

attribute       ::= "name" ":" string_literal
                  | "affinity" ":" expression
                  | "priority" ":" expression
```

**Attribute Semantics**

| Attribute  | Type       | Default            | Purpose                        |
| :--------- | :--------- | :----------------- | :----------------------------- |
| `name`     | `string`   | Anonymous          | Labels for debugging/profiling |
| `affinity` | `CpuSet`   | Domain default     | CPU core affinity hint         |
| `priority` | `Priority` | `Priority::Normal` | Scheduling priority hint       |

##### Static Semantics

**Typing Rule**

$$\frac{
  \Gamma[\text{parallel\_context}] = D \quad
  \Gamma_{\text{capture}} \vdash e : T
}{
  \Gamma \vdash \texttt{spawn}\ \{e\} : \text{SpawnHandle}\langle T \rangle
} \quad \text{(T-Spawn)}$$

A spawn expression returns a `SpawnHandle<T>` representing the pending result.

**SpawnHandle Type**

```cursive
modal SpawnHandle<T> {
    @Pending { }
    
    @Ready {
        value: T,
    }
}
```

The `SpawnHandle<T>` type supports the `wait` expression (§15.3) for result retrieval.

##### Dynamic Semantics

**Evaluation**

Evaluation of `spawn [attrs] { e }` proceeds as follows:

1. Capture free variables from enclosing scope per §14.3.
2. Package the captured environment with expression $e$ into a work item.
3. Enqueue the work item to the parallel block's worker pool.
4. Return a `SpawnHandle<T>@Pending` immediately (non-blocking).
5. A worker eventually dequeues and evaluates $e$.
6. Upon completion, the handle transitions to `@Ready` with the result value.

**Result Collection**

Spawn results are collected through three mechanisms:

1. **Implicit collection**: When a parallel block contains only spawn expressions, results form a tuple in declaration order.
2. **Explicit wait**: Use `wait handle` to retrieve a specific result.
3. **Ignored**: Results not collected are discarded; work still completes.

##### Constraints & Legality

**Negative Constraints**

1. A `spawn` expression MUST appear directly within a parallel block body.
2. A `spawn` expression MUST NOT appear within an escaping closure.

**Diagnostic Table**

| Code         | Severity | Condition                    | Detection    | Effect    |
| :----------- | :------- | :--------------------------- | :----------- | :-------- |
| `E-CON-0130` | Error    | Invalid spawn attribute type | Compile-time | Rejection |
| `E-CON-0131` | Error    | `spawn` in escaping closure  | Compile-time | Rejection |

---

### 15.5 The Dispatch Expression

##### Definition

A **dispatch expression** expresses data-parallel iteration where each iteration may execute concurrently. The key system (§13) determines which iterations can safely parallelize based on their access patterns to `shared` data.

##### Syntax & Declaration

**Grammar**

```ebnf
dispatch_expr   ::= "dispatch" pattern "in" range_expr 
                    key_clause?
                    attribute_list?
                    block

key_clause      ::= "key" path_expr key_mode

key_mode        ::= "read" | "write"

attribute_list  ::= "[" attribute ("," attribute)* "]"

attribute       ::= "reduce" ":" reduce_op
                  | "ordered"
                  | "chunk" ":" expression

reduce_op       ::= "+" | "*" | "min" | "max" | "and" | "or" | identifier
```

**Key Clause**

The optional key clause explicitly declares the key path and mode for each iteration. When omitted, key requirements are inferred from the body's access patterns.

**Attribute Semantics**

| Attribute | Type         | Purpose                                       |
| :-------- | :----------- | :-------------------------------------------- |
| `reduce`  | Reduction op | Combines iteration results using specified op |
| `ordered` | (flag)       | Forces sequential side-effect ordering        |
| `chunk`   | `usize`      | Groups iterations into chunks for granularity |

##### Static Semantics

**Typing Rule (Without Reduction)**

$$\frac{
  \Gamma \vdash \text{range} : \text{Range}\langle I \rangle \quad
  \Gamma, i : I \vdash B : T
}{
  \Gamma \vdash \texttt{dispatch } i \texttt{ in range } \{B\} : ()
} \quad \text{(T-Dispatch)}$$

**Typing Rule (With Reduction)**

$$\frac{
  \Gamma \vdash \text{range} : \text{Range}\langle I \rangle \quad
  \Gamma, i : I \vdash B : T \quad
  \Gamma \vdash \oplus : (T, T) \to T
}{
  \Gamma \vdash \texttt{dispatch } i \texttt{ in range [reduce: } \oplus \texttt{] } \{B\} : T
} \quad \text{(T-Dispatch-Reduce)}$$

**Key Inference**

When no key clause is provided, the body is analyzed to infer key paths and modes:

| Body Access Pattern       | Inferred Key Path        | Inferred Mode   |
| :------------------------ | :----------------------- | :-------------- |
| `data[i] = ...`           | `data[i]`                | `write`         |
| `... = data[i]`           | `data[i]`                | `read`          |
| `data[i] = data[i] + ...` | `data[i]`                | `write`         |
| `result[i] = source[j]`   | `result[i]`, `source[j]` | `write`, `read` |

If inference fails or is ambiguous, diagnostic `E-CON-0141` is emitted.

**Disjointness Guarantee**

Within a dispatch expression, accesses indexed by the iteration variable to the same array are proven disjoint across iterations:

$$\frac{
  \texttt{dispatch}\ v\ \texttt{in}\ r\ \{\ \ldots\ a[v]\ \ldots\ \}
}{
  \forall v_1, v_2 \in r,\ v_1 \neq v_2 \implies \text{ProvablyDisjoint}(a[v_1], a[v_2])
} \quad \text{(K-Disjoint-Dispatch)}$$

Per §14.6, this enables full parallelization when iterations access disjoint array elements.

##### Dynamic Semantics

**Evaluation**

Evaluation of `dispatch i in range [attrs] { B }` proceeds as follows:

1. Evaluate `range` to determine iteration count $n$.
2. Analyze key patterns to partition iterations into conflict-free groups.
3. For each group, enqueue iterations as work items to the worker pool.
4. Workers execute iterations, acquiring keys as needed per §14.2.
5. If `[reduce: op]` is present, combine partial results using `op`.
6. Block until all iterations complete.
7. Return the reduced value (if reduction) or unit.

**Parallelism Determination**

The key system partitions iterations based on key paths:

| Key Pattern   | Keys Generated      | Parallelism Degree    |
| :------------ | :------------------ | :-------------------- |
| `data[i]`     | $n$ distinct keys   | Full parallel         |
| `data[i / 2]` | $n/2$ distinct keys | Pairs serialize       |
| `data[i % k]` | $k$ distinct keys   | $k$-way parallel      |
| `data[f(i)]`  | Unknown at compile  | Runtime serialization |

**Reduction Semantics**

Reduction operators MUST be associative:

$$\forall a, b, c.\ (a \oplus b) \oplus c = a \oplus (b \oplus c)$$

For non-associative operations, the `[ordered]` attribute is required, which forces sequential execution.

Parallel reduction proceeds as follows:

1. Partition iterations across workers.
2. Each worker reduces its partition to a partial result.
3. Combine partial results in a deterministic tree pattern.
4. Return the final result.

The tree combination order is deterministic based on iteration indices, ensuring reproducible results for non-commutative associative operators.

##### Constraints & Legality

**Cross-Iteration Dependency**

Accesses that reference a different iteration's data are rejected:

```cursive
dispatch i in 0..arr.len() - 1 {
    arr[i] = arr[i + 1]     // ERROR E-CON-0142: cross-iteration dependency
}
```

**Diagnostic Table**

| Code         | Severity | Condition                                     | Detection    | Effect    |
| :----------- | :------- | :-------------------------------------------- | :----------- | :-------- |
| `E-CON-0140` | Error    | Dispatch outside parallel block               | Compile-time | Rejection |
| `E-CON-0141` | Error    | Key inference failed; explicit key required   | Compile-time | Rejection |
| `E-CON-0142` | Error    | Cross-iteration dependency detected           | Compile-time | Rejection |
| `E-CON-0143` | Error    | Non-associative reduction without `[ordered]` | Compile-time | Rejection |
| `W-CON-0140` | Warning  | Dynamic key pattern; runtime serialization    | Compile-time | Warning   |

---

### 15.6 Execution Domains

##### Definition

An **execution domain** is a capability that provides access to computational resources. Domains are accessed through the `Context` record and implement the `ExecutionDomain` class.

**Formal Definition**

```cursive
class ExecutionDomain {
    /// Human-readable name for debugging and profiling.
    procedure name(~) -> string

    /// Maximum concurrent work items supported.
    procedure max_concurrency(~) -> usize
}
```

This form is dyn-safe, enabling heterogeneous domain handling.

> **Cross-Reference:** The specific domain implementations (`ctx.cpu`, `ctx.gpu`, `ctx.inline`) and their APIs are library interfaces defined in **Appendix E**.

#### 15.6.1 CPU Domain

##### Definition

The **CPU domain** executes work items on operating system threads managed by the runtime.

##### Syntax & Declaration

**Access**

```cursive
ctx.cpu(workers: n, affinity: cpus, priority: p, stack_size: s)
```

**Parameters**

| Parameter    | Type       | Default                | Purpose                  |
| :----------- | :--------- | :--------------------- | :----------------------- |
| `workers`    | `usize`    | System CPU count       | Number of worker threads |
| `affinity`   | `CpuSet`   | All CPUs               | CPU core affinity mask   |
| `priority`   | `Priority` | `Priority::Normal`     | Thread scheduling hint   |
| `stack_size` | `usize`    | Implementation-defined | Stack size per worker    |

**Supporting Types**

```cursive
record CpuSet {
    mask: [bool; MAX_CPUS],

    procedure all() -> CpuSet
    procedure cores(indices: const [usize]) -> CpuSet
    procedure range(start: usize, end: usize) -> CpuSet
    procedure numa_node(node: usize) -> CpuSet
}

enum Priority {
    Idle,
    Low,
    Normal,
    High,
    Realtime,
}
```

The `Realtime` priority requires elevated OS privileges. Attempting to use `Realtime` without sufficient privileges results in Implementation-Defined Behavior; implementations SHOULD fall back to `High` and emit a warning.

##### Dynamic Semantics

#### 15.6.2 GPU Domain

##### Definition

The **GPU domain** executes work items on graphics processing unit compute shaders.

##### Syntax & Declaration

**Access**

```cursive
ctx.gpu(device: d, queue: q)
```

**Parameters**

| Parameter | Type    | Default | Purpose             |
| :-------- | :------ | :------ | :------------------ |
| `device`  | `usize` | 0       | GPU device index    |
| `queue`   | `usize` | 0       | Command queue index |

##### Static Semantics

**GPU Capture Rules**

Code within GPU dispatch may capture only:

| Capturable Type       | Reason                       |
| :-------------------- | :--------------------------- |
| `GpuBuffer<T>`        | Device-accessible memory     |
| Primitive constants   | Embedded in shader constants |
| `const` small records | Uniform buffer compatible    |

The following are NOT capturable:

| Forbidden Type       | Reason                        |
| :------------------- | :---------------------------- |
| `shared` types       | Key system unavailable on GPU |
| Host pointers        | Different address space       |
| Heap-allocated types | Host memory inaccessible      |

##### Dynamic Semantics

**Memory Model**

GPU and CPU have separate memory spaces. Data transfer is explicit:

**GPU Intrinsics**

The following intrinsics are available within GPU parallel blocks:

| Intrinsic                        | Type           | Purpose                       |
| :------------------------------- | :------------- | :---------------------------- |
| `gpu::alloc<T>(count: usize)`    | `GpuBuffer<T>` | Allocate device buffer        |
| `gpu::upload(src, dst)`          | `()`           | Copy host to device           |
| `gpu::download(src, dst)`        | `()`           | Copy device to host           |
| `gpu::global_id()`               | `usize`        | Global thread index           |
| `gpu::local_id()`                | `usize`        | Workgroup-local index         |
| `gpu::workgroup_id()`            | `usize`        | Workgroup index               |
| `gpu::barrier()`                 | `()`           | Workgroup sync barrier        |
| `gpu::atomic_add(ptr, val)`      | `T`            | Atomic add; returns old value |
| `gpu::atomic_min(ptr, val)`      | `T`            | Atomic min; returns old value |
| `gpu::atomic_max(ptr, val)`      | `T`            | Atomic max; returns old value |
| `gpu::atomic_cas(ptr, cmp, new)` | `T`            | Compare-and-swap; returns old |

**Dispatch Dimensions**

```cursive
// 1D dispatch
dispatch i in 0..n { ... }

// 2D dispatch
dispatch (x, y) in (0..width, 0..height) { ... }

// 3D dispatch
dispatch (x, y, z) in (0..w, 0..h, 0..d) { ... }

// Explicit workgroup size
dispatch i in 0..n [workgroup: 256] { ... }
```

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                        | Detection    | Effect    |
| :----------- | :------- | :------------------------------- | :----------- | :-------- |
| `E-CON-0150` | Error    | Host memory access in GPU code   | Compile-time | Rejection |
| `E-CON-0151` | Error    | `shared` capture in GPU dispatch | Compile-time | Rejection |
| `E-CON-0152` | Error    | Nested GPU parallel block        | Compile-time | Rejection |

#### 15.6.3 Inline Domain

##### Definition

The **inline domain** executes work items sequentially on the current thread. This domain is useful for testing, debugging, and single-threaded execution contexts.

##### Syntax & Declaration

**Access**

```cursive
ctx.inline()
```

##### Dynamic Semantics

**Evaluation**

- `spawn { e }` evaluates $e$ immediately and blocks until complete.
- `dispatch i in range { e }` executes as a sequential loop.
- No actual parallelism occurs.
- All capture rules and permission requirements remain enforced.

---

### 15.7 Async Integration

##### Definition

Async operations (`wait`, `Async<T>` per §15) compose with parallel blocks. Workers may suspend on async operations, yielding to other work items while waiting.

##### Dynamic Semantics

**Wait Inside Parallel**

When a worker evaluates a `wait` expression:

1. The worker suspends the current work item.
2. The worker picks up another queued work item (if available).
3. When the async operation completes, the suspended work item becomes ready.
4. Some worker resumes the work item.

**Key Prohibition**

Per §13.8, keys MUST NOT be held across `wait` suspension points.

**SpawnHandle and Wait**

`SpawnHandle<T>` supports the `wait` expression:

```cursive
parallel ctx.cpu() {
    let handle = spawn { expensive_compute() }
    
    other_work()
    
    let result = wait handle
}
```

---

### 15.8 Cancellation

##### Definition

**Cancellation** is the cooperative mechanism by which in-progress parallel work may be requested to stop early. Work items MUST explicitly check for cancellation; the runtime does not forcibly terminate work.

##### Syntax & Declaration

**CancelToken Type**

```cursive
modal CancelToken {
    @Active { } {
        procedure cancel(~%)
        procedure is_cancelled(~) -> bool
        procedure wait_cancelled(~) -> Async<()>
        procedure child(~) -> CancelToken@Active
    }
    
    @Cancelled { } {
        procedure is_cancelled(~) -> bool { result true }
    }

    procedure new() -> CancelToken@Active
}
```

##### Dynamic Semantics

**Propagation**

When a cancel token is attached to a parallel block via the `cancel` option, the token is implicitly available within all `spawn` and `dispatch` bodies.

**Cancellation vs Completion**

Cancellation is a request, not a guarantee:

| Scenario                       | Behavior                          |
| :----------------------------- | :-------------------------------- |
| Work checks and returns early  | Iteration completes immediately   |
| Work ignores cancellation      | Iteration runs to completion      |
| Work is queued but not started | MAY be dequeued without executing |
| Work is mid-execution          | Continues until next check point  |

---

### 15.9 Parallel Block Result Types

##### Definition

A parallel block is an expression that yields a value. The result type depends on the block's contents.

##### Static Semantics

**Result Type Determination**

| Block Contents                     | Result Type          |
| :--------------------------------- | :------------------- |
| No `spawn` or `dispatch`           | `()`                 |
| Single `spawn { e }` where $e : T$ | $T$                  |
| Multiple spawns $e_1, \ldots, e_n$ | $(T_1, \ldots, T_n)$ |
| `dispatch` without `[reduce]`      | `()`                 |
| `dispatch ... [reduce: op] { e }`  | $T$ where $e : T$    |
| Mixed spawns and reducing dispatch | Tuple of all results |

**Typing Rules**

$$\frac{
  \Gamma \vdash \texttt{parallel } D\ \{\}\ : ()
}{} \quad \text{(T-Parallel-Empty)}$$

$$\frac{
  \Gamma \vdash \texttt{spawn } \{e\} : \text{SpawnHandle}\langle T \rangle
}{
  \Gamma \vdash \texttt{parallel } D\ \{\texttt{spawn } \{e\}\} : T
} \quad \text{(T-Parallel-Single)}$$

$$\frac{
  \Gamma \vdash \texttt{spawn } \{e_i\} : \text{SpawnHandle}\langle T_i \rangle \quad \forall i \in 1..n
}{
  \Gamma \vdash \texttt{parallel } D\ \{\texttt{spawn } \{e_1\}; \ldots; \texttt{spawn } \{e_n\}\} : (T_1, \ldots, T_n)
} \quad \text{(T-Parallel-Multi)}$$

---

### 15.10 Panic Handling

##### Definition

When a work item panics within a parallel block, the panic is captured and propagated after all work settles.

##### Dynamic Semantics

**Single Panic**

1. The panicking work item captures panic information.
2. Other work items continue to completion (or cancellation if a token is attached).
3. After all work settles, the panic is re-raised at the block boundary.

**Multiple Panics**

1. Each panic is captured independently.
2. All work completes or is cancelled.
3. The first panic (by completion order) is raised.
4. Other panics are discarded; implementations MAY log them.

**Panic and Cancellation**

Implementations MAY request cancellation on first panic to expedite block completion. This behavior is Implementation-Defined.

**Catching Panics**

To handle panics within the block without propagation:

```cursive
let results = parallel ctx.cpu() {
    spawn { catch_panic(|| risky_work_a()) }
    spawn { catch_panic(|| risky_work_b()) }
}
// results: (Result<A, PanicInfo>, Result<B, PanicInfo>)
```

---

### 15.11 Nested Parallelism

##### Definition

Parallel blocks may be nested. Inner blocks execute within the context established by outer blocks.

##### Dynamic Semantics

**CPU Nesting**

Inner CPU parallel blocks share the worker pool with outer blocks. The `workers` parameter on inner blocks is a hint or limit, not additional workers.

```cursive
parallel ctx.cpu(workers: 8) {
    dispatch i in 0..4 {
        parallel ctx.cpu(workers: 2) {
            dispatch j in 0..100 {
                #data[i][j] write {
                    data[i][j] = compute(i, j)
                }
            }
        }
    }
}
```

**Heterogeneous Nesting**

CPU and GPU blocks may be nested:

```cursive
parallel ctx.cpu(workers: 4) {
    spawn {
        let preprocessed = cpu_preprocess(data)
        
        parallel ctx.gpu() {
            dispatch i in 0..n {
                gpu_process(preprocessed, i)
            }
        }
        
        cpu_postprocess(preprocessed)
    }
}
```

##### Constraints & Legality

**Nesting Constraints**

1. GPU parallel blocks MUST NOT be nested inside other GPU parallel blocks.
2. Inner CPU blocks share the outer block's worker pool.
3. Capture rules apply independently at each nesting level.

---

### 15.12 Determinism

##### Definition

**Determinism** guarantees that given identical inputs and parallel structure, execution produces identical results.

##### Static Semantics

**Deterministic Dispatch**

Dispatch is deterministic when:

1. Key patterns produce identical partitioning across runs.
2. Iterations with the same key execute in index order.
3. Reduction uses deterministic tree combination.

**Ordered Dispatch**

The `[ordered]` attribute forces sequential side-effect ordering:

```cursive
parallel ctx.cpu() {
    dispatch i in 0..n [ordered] {
        println(f"Processing {i}")  // Prints 0, 1, 2, ... in order
    }
}
```

Iterations MAY execute in parallel, but side effects (I/O, shared mutation) are buffered and applied in index order.

---

### 15.13 Memory Allocation in Parallel Blocks

##### Definition

Work items may allocate memory using captured allocator capabilities.

##### Dynamic Semantics

**Captured Allocator**

```cursive
parallel ctx.cpu() {
    spawn {
        let buffer = ctx.heap~>alloc::<[u8; 1024]>()
        use(buffer)
    }
}
```

**Region Allocation**

```cursive
region work_arena {
    parallel ctx.cpu() {
        dispatch i in 0..n {
            let temp = ^work_arena compute_result(i)
            process(temp)
        }
    }
}
```

---

## Clause 16: Asynchronous Operations

### 16.2 The Async Class

##### Definition

`Async<Out, In, Result, E>` is a modal class representing the interface for asynchronous computations. The type parameters specify the data flow between the computation and its consumer. Async-returning procedures produce anonymous generated modal types that implement this class.

| Parameter | Meaning                                          | Default  |
| --------- | ------------------------------------------------ | -------- |
| `Out`     | Type of values produced at each suspension point | Required |
| `In`      | Type of values received when resumed             | `()`     |
| `Result`  | Type of the final completion value               | `()`     |
| `E`       | Type of the error on failure                     | `!`      |

When `E` is the never type (`!`), the async operation cannot fail.

##### Syntax & Declaration

**Class Declaration**

```cursive
class Async<Out; In = (); Result = (); E = !> {
    @Suspended {
        output: Out,
    }

    @Completed {
        value: Result,
    }

    @Failed {
        error: E,
    }

    /// Advance the computation by providing input.
    /// Returns the new state after one step.
    procedure resume(~!, input: In) -> Async@Suspended | Async@Completed | Async@Failed;
}
```

As a modal class (§9.2), `Async` declares abstract states that implementing modal types must provide. The `resume` procedure is an abstract procedure that all implementations must define.

##### Static Semantics

| State        | Meaning                                          | Payload         |
| ------------ | ------------------------------------------------ | --------------- |
| `@Suspended` | Computation paused, intermediate value available | `output: Out`   |
| `@Completed` | Computation finished successfully                | `value: Result` |
| `@Failed`    | Computation terminated with error                | `error: E`      |

**Type of Async-Returning Procedures**

A procedure declared with return type `Async<Out, In, Result, E>` has a concrete return type that is the generated modal implementing the class. When the procedure is called, the result has this concrete modal type:

$$\frac{
    \Gamma \vdash f : (\overline{T}) \to \texttt{Async}\langle Out, In, Result, E \rangle \quad
    \text{gen\_}f \text{ is the generated modal for } f
}{
    \Gamma \vdash f(\overline{e}) : \text{gen\_}f
} \quad \text{(T-Async-Call)}$$

The concrete type is a subtype of the declared class type:

$$\text{gen\_}f <: \texttt{Async}\langle Out, In, Result, E \rangle$$

**Monomorphic vs Polymorphic Use**

| Context                    | Type Used                                     | Indirection                    |
| :------------------------- | :-------------------------------------------- | :----------------------------- |
| Direct call, local binding | Concrete generated modal                      | None                           |
| Homogeneous collection     | Concrete generated modal                      | None                           |
| Heterogeneous collection   | `dyn Async<...>`                              | Required                       |
| Polymorphic parameter      | `dyn Async<...>` or generic `T <: Async<...>` | Via vtable or monomorphization |

**Class Subtyping Rule**

The `Async` class follows standard class subtyping with variance applied to type parameters:

$$\frac{
    \Gamma \vdash Out_1 <: Out_2 \quad
    \Gamma \vdash In_2 <: In_1 \quad
    \Gamma \vdash Result_1 <: Result_2 \quad
    \Gamma \vdash E_1 <: E_2
}{
    \Gamma \vdash \texttt{Async}\langle Out_1, In_1, Result_1, E_1 \rangle <:
                  \texttt{Async}\langle Out_2, In_2, Result_2, E_2 \rangle
} \quad \text{(Sub-Async-Class)}$$

A concrete modal implementing `Async` is a subtype of any `Async` class type it satisfies:

$$\frac{
    M <: \texttt{Async}\langle Out_1, In_1, Result_1, E_1 \rangle \quad
    \texttt{Async}\langle Out_1, In_1, Result_1, E_1 \rangle <: \texttt{Async}\langle Out_2, In_2, Result_2, E_2 \rangle
}{
    M <: \texttt{Async}\langle Out_2, In_2, Result_2, E_2 \rangle
} \quad \text{(Sub-Async-Modal)}$$

**Generated Implementations**

For each procedure with return type `Async<Out, In, Result, E>`, an anonymous modal type implementing this class is generated:

```cursive
// For: procedure countdown(n: i32) -> Async<i32, (), i32, !>
// Generated (conceptually):
modal gen_countdown <: Async<i32, (), i32, !> {
    @Suspended {
        output: i32,          // required by Async
        gen_point: u8,        // resumption point discriminant
        i: i32,               // captured local
        n: i32,               // captured parameter
    }

    @Completed {
        value: i32,           // required by Async
    }

    // @Failed omitted: E = ! makes it uninhabited
}
```

The generated modal:
1. Has the exact state names required by the class (`@Suspended`, `@Completed`, `@Failed`)
2. Includes all class-required payload fields in each state
3. Adds implementation-specific fields (prefixed with `gen_`) for resumption state and captured locals
4. May omit `@Failed` when `E = !` per the uninhabited state omission rule (§9.3)

**Generated Type Naming**

Generated modal types use the `gen_` prefix and are not directly nameable by user code. Any naming scheme MAY be used that:
1. Avoids collision with user-defined types
2. Produces distinct names for distinct async procedures
3. Is consistent across compilations with the same source

**Well-Formedness**

When `E = !` (the never type), the `@Failed` state is uninhabited and may be omitted from the implementing modal. Pattern matches through `Async@Failed` are unreachable in this case.

**Default Parameter Expansion**

When fewer than four type arguments are provided:
- `Async<T>` ≡ `Async<T, (), (), !>`
- `Async<T, U>` ≡ `Async<T, U, (), !>`
- `Async<T, U, R>` ≡ `Async<T, U, R, !>`

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                      | Detection    | Effect    |
| ------------ | -------- | ---------------------------------------------- | ------------ | --------- |
| `E-CON-0201` | Error    | `Async` type parameter is not well-formed      | Compile-time | Rejection |
| `E-CON-0202` | Error    | `yield` in non-async procedure                 | Compile-time | Rejection |
| `E-CON-0203` | Error    | `result` type mismatch with `Result` parameter | Compile-time | Rejection |

---

### 16.3 Type Aliases for Common Patterns

##### Definition

The following type aliases provide convenient names for common asynchronous patterns. These are type aliases for the `Async` modal class with specific type parameter bindings. Concrete implementations are generated modal types that implement these class types.

```cursive
type Sequence<T> = Async<T, (), (), !>
type Future<T; E = !> = Async<(), (), T, E>
type Pipe<In; Out> = Async<Out, In, (), !>
type Exchange<T> = Async<T, T, T, !>
type Stream<T; E> = Async<T, (), (), E>
```

These aliases may be used as bounds, return types, or with `dyn` for type erasure.

##### Static Semantics

**Equivalence**

Type aliases are structurally equivalent to their expanded forms:

$$\texttt{Sequence}\langle T \rangle \equiv \texttt{Async}\langle T, (), (), ! \rangle$$

$$\texttt{Future}\langle T, E \rangle \equiv \texttt{Async}\langle (), (), T, E \rangle$$

$$\texttt{Pipe}\langle In, Out \rangle \equiv \texttt{Async}\langle Out, In, (), ! \rangle$$

$$\texttt{Stream}\langle T, E \rangle \equiv \texttt{Async}\langle T, (), (), E \rangle$$

**
Type aliases inherit variance from their expansion:

| Alias           | Expansion               | Variance                              |
| :-------------- | :---------------------- | :------------------------------------ |
| `Sequence<T>`   | `Async<T, (), (), !>`   | Covariant in T                        |
| `Future<T, E>`  | `Async<(), (), T, E>`   | Covariant in T, E                     |
| `Pipe<In, Out>` | `Async<Out, In, (), !>` | Contravariant in In, Covariant in Out |
| `Stream<T, E>`  | `Async<T, (), (), E>`   | Covariant in T, E                     |

---

### 16.4 Producing Async Values

##### Definition

Producing async values covers constructs that create `Async` instances, including async-returning procedures and `yield` forms that drive the generated modal state machine.

#### 16.4.1 Async-Returning Procedures

##### Definition

A procedure returns an async value by declaring the `Async` class (or an alias) as its return type. A distinct anonymous modal type is generated for each such procedure, and the procedure body is transformed into that modal's state machine implementation.

##### Syntax & Declaration

**Grammar**

```ebnf
async_procedure ::= procedure_decl
                    (* where return type is Async<...> *)
```

No special syntax modifier is required. The return type determines async behavior.

##### Static Semantics

**Modal Generation Rules**

When a procedure's return type is `Async<Out, In, Result, E>`:

1. An anonymous modal type implementing `Async<Out, In, Result, E>` is generated.
2. The generated modal has states `@Suspended`, `@Completed`, and (unless `E = !`) `@Failed`.
3. Each state includes the class-required payload fields plus implementation fields:
   - `@Suspended`: `output: Out` plus `gen_point` (resumption discriminant) and all locals live across suspension points
   - `@Completed`: `value: Result`
   - `@Failed`: `error: E`
4. The procedure body is transformed into the `resume` transition implementation.
5. Each `yield` expression becomes a suspension point; the generated modal stores the resumption point in `gen_point`.
6. The `result` expression produces the final `@Completed` value.
7. Returning without `result` in a procedure with `Result = ()` implicitly completes with `()`.
8. Error propagation via `?` transitions to `@Failed`.

**(T-Async-Proc)**
$$\frac{
    \Gamma \vdash \text{body} : Result \quad
    \text{yields in body have type } Out \quad
    \text{yields receive type } In
}{
    \Gamma \vdash \texttt{procedure}\ f(\ldots) \to \texttt{Async}\langle Out, In, Result, E \rangle\ \{\ \text{body}\ \}\ \text{wf}
}$$

##### Dynamic Semantics

**Normal Completion**

A procedure completes by executing `result value` or by reaching the end of its body (for `Result = ()`):

```cursive
procedure countdown(n: i32) -> Async<i32, (), i32, !> {
    var i = n
    loop i > 0 {
        yield i
        i -= 1
    }
    result 0
}
```

**Failure**

An `Async` fails when an error propagates via `?` or explicit error return:

```cursive
procedure read_lines(path: string, fs: dyn FileSystem) -> Stream<string, IoError> {
    let file = fs~>open(path)?
    loop {
        let line = yield from fs~>read_line(file)?
        if line~>is_empty() {
            return
        }
        yield line
    }
}
```

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                       | Detection    | Effect    |
| :----------- | :------- | :---------------------------------------------- | :----------- | :-------- |
| `E-CON-0230` | Error    | Error propagation in infallible async procedure | Compile-time | Rejection |

---

#### 16.4.2 The `yield` Expression

##### Definition

The `yield` expression suspends an asynchronous computation, producing an intermediate value and awaiting resumption with an input value.

##### Syntax & Declaration

**Grammar**

```ebnf
yield_expr ::= "yield" ["release"] expression
```

The optional `release` modifier enables yielding while keys are held, using release-and-reacquire semantics.

##### Static Semantics

**Typing Rule**

Within a procedure returning `Async<Out, In, Result, E>`:

$$\frac{
    \text{ReturnType}(\text{enclosing}) = \texttt{Async}\langle Out, In, Result, E \rangle \quad
    \Gamma \vdash e : Out
}{
    \Gamma \vdash \texttt{yield}\ e : In
} \quad \text{(T-Yield)}$$

The operand expression MUST have type `Out`. The `yield` expression evaluates to type `In`—the value provided when the computation is resumed.

**Context Requirement**

The `yield` expression is valid only within a procedure whose return type is `Async<Out, In, Result, E>` for some `Out`, `In`, `Result`, `E`.

**Key Interaction**

When `yield` is combined with a `#` key block in the same expression, the key is released BEFORE suspension.

**Desugaring**

```cursive
yield #path { expr }

// Desugars to:
{
    let __temp = #path { expr }  // Key acquired, expr evaluated, key released
    yield __temp                  // Yield occurs with no keys held
}
```

##### Dynamic Semantics

**Evaluation**

Evaluation of `yield e`:

1. Let $v$ be the result of evaluating $e$.
2. Transition the `Async` to `@Suspended { output: v }`.
3. Return control to the caller.
4. When `resume(input)` is called, bind $input$ as the result of the `yield` expression.
5. Continue execution from the point immediately after `yield`.

**Evaluation of `yield release e` (Key Parking)**

When `yield release` is used within a key block:

1. Let $\Gamma_{keys}$ be the set of currently held keys.
2. **Release all keys** in LIFO order (reverse acquisition order).
3. Let $v$ be the result of evaluating $e$.
4. Transition the `Async` to `@Suspended { output: v }`.
5. Return control to the caller.
6. When `resume(input)` is called:
   a. **Reacquire all keys** in canonical order (same keys as before suspension).
   b. Bind $input$ as the result of the `yield release` expression.
7. Continue execution from the point immediately after `yield release`.

**Staleness at Yield Release**

A binding derived from `shared` data before `yield release` is **potentially stale** after the yield returns. During the suspension period, other tasks MAY modify the shared data. `W-CON-0011` MUST be emitted when a potentially stale binding is accessed after a `yield release` on an overlapping path.

##### Constraints & Legality

**Negative Constraints**

1. `yield` MUST NOT appear outside an async-returning procedure.
2. `yield` MUST NOT appear inside a `sync` expression.
3. `yield` (without `release`) MUST NOT appear while any keys are logically held (§13).
4. `yield release` MAY appear while keys are held; keys are released before suspension and reacquired after resumption.

**Diagnostic Table**

| Code         | Severity | Condition                                                 | Detection    | Effect    |
| :----------- | :------- | :-------------------------------------------------------- | :----------- | :-------- |
| `E-CON-0210` | Error    | `yield` outside async-returning procedure                 | Compile-time | Rejection |
| `E-CON-0211` | Error    | `yield` operand type does not match `Out`                 | Compile-time | Rejection |
| `E-CON-0212` | Error    | `yield` inside `sync` expression                          | Compile-time | Rejection |
| `E-CON-0213` | Error    | `yield` while key is held (without `release`)             | Compile-time | Rejection |
| `W-CON-0011` | Warning  | Access to potentially stale binding after `yield release` | Compile-time | Warning   |

---

#### 16.4.3 The `yield from` Expression

##### Definition

The `yield from` expression delegates to another asynchronous computation, forwarding all its outputs and inputs until completion, then evaluating to the delegated computation's result.

##### Syntax & Declaration

**Grammar**

```ebnf
yield_from_expr ::= "yield" ["release"] "from" expression
```

The optional `release` modifier enables delegation while keys are held, using release-and-reacquire semantics at each suspension point.

##### Static Semantics

**Typing Rule**

$$\frac{
    \text{ReturnType}(\text{enclosing}) = \texttt{Async}\langle Out, In, \_, E_1 \rangle \quad
    \Gamma \vdash e : \texttt{Async}\langle Out, In, R, E_2 \rangle \quad
    E_2 <: E_1
}{
    \Gamma \vdash \texttt{yield from}\ e : R
} \quad \text{(T-Yield-From)}$$

**Compatibility Requirements**

The source expression MUST be an `Async` with:
- Identical `Out` parameter to the enclosing procedure
- Identical `In` parameter to the enclosing procedure
- Error type `E_2` that is a subtype of the enclosing error type `E_1`

The expression evaluates to the source's `Result` type.

##### Dynamic Semantics

**Delegation Loop**

Evaluation of `yield from source`:

1. Let $s$ be the result of evaluating $source$.
2. Loop:
   - Match $s$:
     - `@Suspended { output }`: Execute `yield output` in the enclosing computation. When resumed with $input$, let $s$ := $s$~>resume$(input)$.
     - `@Completed { value }`: The `yield from` expression evaluates to $value$. Exit loop.
     - `@Failed { error }`: Propagate $error$ to the enclosing computation. Exit loop.

**Delegation with Key Parking**

Evaluation of `yield release from source`:

1. Let $\Gamma_{keys}$ be the set of currently held keys.
2. Let $s$ be the result of evaluating $source$.
3. Loop:
   - Match $s$:
     - `@Suspended { output }`:
       a. **Release all keys** in LIFO order.
       b. Execute `yield output` in the enclosing computation.
       c. When resumed with $input$, **reacquire all keys** in canonical order.
       d. Let $s$ := $s$~>resume$(input)$.
     - `@Completed { value }`: The `yield release from` expression evaluates to $value$. Exit loop.
     - `@Failed { error }`: Propagate $error$ to the enclosing computation. Exit loop.

Keys are released before each suspension point and reacquired after each resumption, allowing other tasks to access the shared data during delegation.

##### Constraints & Legality

**Negative Constraints**

1. `yield from` MUST NOT appear outside an async-returning procedure.
2. `yield from` MUST NOT appear inside a `sync` expression.
3. `yield from` (without `release`) MUST NOT appear while any keys are logically held.
4. `yield release from` MAY appear while keys are held; keys are released at each suspension and reacquired after each resumption.
5. The delegated `Async` MUST have compatible `Out` and `In` parameters.

**Diagnostic Table**

| Code         | Severity | Condition                                                      | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------------------------- | :----------- | :-------- |
| `E-CON-0220` | Error    | `yield from` outside async-returning proc                      | Compile-time | Rejection |
| `E-CON-0221` | Error    | Incompatible `Out` parameter in `yield from`                   | Compile-time | Rejection |
| `E-CON-0222` | Error    | Incompatible `In` parameter in `yield from`                    | Compile-time | Rejection |
| `E-CON-0223` | Error    | `yield from` inside `sync` expression                          | Compile-time | Rejection |
| `E-CON-0224` | Error    | `yield from` while key is held (without `release`)             | Compile-time | Rejection |
| `E-CON-0225` | Error    | Error type not compatible in `yield from`                      | Compile-time | Rejection |
| `W-CON-0011` | Warning  | Access to potentially stale binding after `yield release from` | Compile-time | Warning   |

---

### 16.5 Consuming Async Values

##### Definition

An async value may be consumed through three patterns: iteration over outputs, manual stepping with explicit state matching, or synchronous execution via `sync` until completion. The appropriate pattern depends on the async type parameters and the consumption context.

---

#### 16.5.1 Iteration (`loop...in`)

##### Definition

The `loop...in` construct iterates over an `Async<T, (), R, E>`, consuming each yielded value until completion or failure. Iteration is available only for async values with `In = ()`.

##### Syntax & Declaration

**Grammar**

```ebnf
async_loop ::= "loop" pattern "in" expression block
```

**Desugaring**

```cursive
loop item in source {
    body
}

// Desugars to:
{
    var __s = source
    loop {
        match __s {
            @Suspended { output: item } => {
                body
                __s = __s~>resume(())
            }
            @Completed { .. } => break,
            @Failed { error } => panic(error),
        }
    }
}
```

##### Static Semantics

**Typing Rule**

$$\frac{
    \Gamma \vdash e : \texttt{Async}\langle T, (), R, E \rangle \quad
    \Gamma, x : T \vdash \text{body} : ()
}{
    \Gamma \vdash \texttt{loop}\ x\ \texttt{in}\ e\ \{ \text{body} \} : ()
} \quad \text{(T-Async-Loop)}$$

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                           | Detection    | Effect    |
| :----------- | :------- | :---------------------------------- | :----------- | :-------- |
| `E-CON-0240` | Error    | Iteration over async with `In ≠ ()` | Compile-time | Rejection |

---

#### 16.5.2 Manual Stepping

##### Definition

Direct interaction with the `Async` state machine via pattern matching and explicit `resume` transition invocation. This pattern is required for async values with non-unit `In` types.

##### Dynamic Semantics

The consumer matches on the async value's modal state and invokes `resume` to advance. Resumptions MAY be chained when intermediate states do not need inspection.

---

#### 16.5.3 Synchronous Execution (`sync`)

##### Definition

The `sync` expression runs a `Future<T, E>` (equivalently `Async<(), (), T, E>`) to completion synchronously, halting the current execution context until the result is available.

##### Syntax & Declaration

**Grammar**

```ebnf
sync_expr ::= "sync" expression
```

##### Static Semantics

**Typing Rule**

$$\frac{
    \Gamma \vdash e : \texttt{Async}\langle (), (), T, E \rangle
}{
    \Gamma \vdash \texttt{sync}\ e : T \mid E
} \quad \text{(T-Sync)}$$

The operand MUST be an `Async` with `Out = ()` and `In = ()`.

**Context Restriction**

The `sync` expression is permitted only in non-async contexts (procedures not returning `Async`).

##### Dynamic Semantics

**Evaluation**

Evaluation of `sync e`:

1. Let $a$ be the result of evaluating $e$.
2. Loop:
   - Match $a$:
     - `@Suspended { output: () }`: Let $a$ := $a$~>resume$(())$.
     - `@Completed { value }`: Return $value$.
     - `@Failed { error }`: Propagate $error$.

**Reactor Integration**

The `sync` expression desugars to use the reactor capability (§15.11.1):

$$\texttt{sync}\ f \quad \rightarrow \quad \texttt{ctx.reactor\sim>run}(f)$$

The reactor polls the future and any dependent I/O operations until completion.

##### Constraints & Legality

**Negative Constraints**

1. `sync` MUST NOT appear inside an async-returning procedure.
2. The operand MUST have `Out = ()` and `In = ()`.

**Diagnostic Table**

| Code         | Severity | Condition                          | Detection    | Effect    |
| :----------- | :------- | :--------------------------------- | :----------- | :-------- |
| `E-CON-0250` | Error    | `sync` inside async-returning proc | Compile-time | Rejection |
| `E-CON-0251` | Error    | `sync` operand has `Out ≠ ()`      | Compile-time | Rejection |
| `E-CON-0252` | Error    | `sync` operand has `In ≠ ()`       | Compile-time | Rejection |

---

### 16.6 Concurrent Composition

##### Definition

Concurrent composition allows multiple async operations to execute simultaneously. The `race` expression returns when the first operation completes; the `all` expression returns when all operations complete; the `until` method suspends until a condition becomes true.

---

#### 16.6.1 The `race` Expression

##### Definition

The `race` expression initiates multiple asynchronous operations concurrently and returns when the first completes. When handler arms use `yield`, the expression produces a streaming result.

##### Syntax & Declaration

**Grammar**

```ebnf
race_expr    ::= "race" "{" race_arm ("," race_arm)* [","] "}"
race_arm     ::= expression "->" "|" pattern "|" race_handler
race_handler ::= expression | "yield" expression
```

##### Static Semantics

**First-Completion Typing (Non-Streaming)**

$$\frac{
    \forall i \in 1..n.\ \Gamma \vdash e_i : \texttt{Async}\langle (), (), T_i, E_i \rangle \quad
    \forall i \in 1..n.\ \Gamma, x_i : T_i \vdash r_i : R \quad
    n \geq 2 \quad
    \text{no handler uses yield}
}{
    \Gamma \vdash \texttt{race}\ \{ e_1 \to |x_1|\ r_1, \ldots, e_n \to |x_n|\ r_n \} : R \mid (E_1 | \ldots | E_n)
} \quad \text{(T-Race)}$$

All handler arms MUST produce the same result type. The error type is the union of all constituent error types.

**Streaming Typing**

$$\frac{
    \forall i \in 1..n.\ \Gamma \vdash e_i : \texttt{Async}\langle T_i, (), R_i, E_i \rangle \quad
    \forall i \in 1..n.\ \Gamma, x_i : T_i \vdash u_i : U \quad
    \exists j.\ T_j \neq () \quad
    \text{all handlers use yield}
}{
    \Gamma \vdash \texttt{race}\ \{ \ldots \} : \texttt{Sequence}\langle U, E_1 | \ldots | E_n \rangle
} \quad \text{(T-Race-Stream)}$$

When all arms use `yield` in their handlers, the result is a `Sequence` that yields each handled value.

##### Dynamic Semantics

**First-Completion Evaluation**

1. Initiate all async expressions concurrently.
2. When any operation reaches `@Completed` or `@Failed`:
   - Execute the corresponding handler arm (binding the result or error to the pattern).
   - Cancel all other operations (drop them, invoking cleanup per §15.9).
   - Return the handler's result.
3. If an operation fails, the failure propagates unless explicitly handled in the arm.

**Streaming Evaluation**

1. Initiate all async expressions concurrently.
2. When any arm yields a value (transitions to `@Suspended`):
   - Execute the corresponding handler, binding the output to the pattern.
   - Yield the handler's result from the streaming race.
   - Resume the arm that produced the value.
3. When an arm completes (`@Completed`): remove it from the race.
4. When all arms complete: the streaming race completes.
5. If any arm fails: propagate error, cancel remaining arms.

**Cancellation**

When one arm completes (non-streaming) or fails:
1. All other in-flight operations are dropped.
2. `Drop` implementations execute per §15.9.
3. Resources are released before the race expression completes.

##### Constraints & Legality

**Negative Constraints**

1. `race` MUST have at least 2 arms.
2. All arms MUST have compatible result types.
3. All operand expressions in non-streaming race MUST have `Out = ()` and `In = ()`.
4. All arms MUST use the same handler form (all `yield` or all non-`yield`).

**Diagnostic Table**

| Code         | Severity | Condition                                   | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------ | :----------- | :-------- |
| `E-CON-0260` | Error    | `race` with fewer than 2 arms               | Compile-time | Rejection |
| `E-CON-0261` | Error    | `race` arms have incompatible types         | Compile-time | Rejection |
| `E-CON-0262` | Error    | Non-streaming `race` operand has `Out ≠ ()` | Compile-time | Rejection |
| `E-CON-0263` | Error    | Mixed yield/non-yield handlers in race      | Compile-time | Rejection |

---

#### 16.6.2 The `all` Expression

##### Definition

The `all` expression initiates multiple asynchronous operations concurrently and returns when all complete successfully, or when any fails.

##### Syntax & Declaration

**Grammar**

```ebnf
all_expr ::= "all" "{" expression ("," expression)* [","] "}"
```

##### Static Semantics

**Typing Rule**

$$\frac{
    \forall i \in 1..n.\ \Gamma \vdash e_i : \texttt{Async}\langle (), (), T_i, E_i \rangle
}{
    \Gamma \vdash \texttt{all}\ \{ e_1, \ldots, e_n \} : (T_1, \ldots, T_n) \mid (E_1 | \ldots | E_n)
} \quad \text{(T-All)}$$

The result is a tuple of all completion values. The error type is the union of all constituent error types.

##### Dynamic Semantics

**All-Completion Evaluation**

1. Initiate all async expressions concurrently.
2. Wait for all operations to complete.
3. If all succeed: return a tuple of results in declaration order.
4. If any fails: cancel remaining operations, propagate the first error.

**First-Failure Semantics**

When any operation transitions to `@Failed`:
1. Record the error.
2. Cancel all remaining in-flight operations.
3. Wait for cancellation to complete (cleanup runs).
4. Propagate the recorded error.

##### Constraints & Legality

**Negative Constraints**

1. All operand expressions MUST have `Out = ()` and `In = ()`.

**Diagnostic Table**

| Code         | Severity | Condition                    | Detection    | Effect    |
| :----------- | :------- | :--------------------------- | :----------- | :-------- |
| `E-CON-0270` | Error    | `all` operand has `Out ≠ ()` | Compile-time | Rejection |
| `E-CON-0271` | Error    | `all` operand has `In ≠ ()`  | Compile-time | Rejection |

---

#### 16.6.3 Condition Waiting (`until`)

##### Definition

Condition waiting suspends an async operation until a predicate on shared data becomes true. The `until` method is provided on all `shared` types.

##### Syntax & Declaration

**Method Signature**

Every `shared` type MUST provide a generated `until` method:

```cursive
procedure until<T; R>(
    self: shared T,
    predicate: procedure(const T) -> bool,
    action: procedure(unique T) -> R,
) -> Future<R>
```

##### Dynamic Semantics

**Predicate Registration and Wake**

Evaluation of `shared_value~>until(pred, action)`:

1. If `pred(shared_value)` is true immediately:
   - Acquire Write key to `shared_value`.
   - Execute `action(shared_value)`.
   - Complete the Future with the result.
2. Otherwise:
   - Register a waiter record: $(path, pred, action, continuation)$.
   - Transition the Future to `@Suspended { output: () }`.
   - The waiter does NOT hold any keys while suspended.

**Notification on Key Release**

When a Write key to path $P$ is released (per §14.2):

1. The runtime checks for waiters registered on $P$ or any prefix of $P$.
2. Each matching waiter is marked **potentially ready**.
3. Potentially ready waiters re-evaluate their predicates.
4. Waiters whose predicates return `true` are transitioned to **ready** and scheduled for execution.

**Notification Granularity**

| Write Key Released | Waiters Notified                          |
| :----------------- | :---------------------------------------- |
| `player.health`    | Waiters on `player.health`, `player`      |
| `player`           | All waiters on `player` or any `player.*` |
| `arr[i]`           | Waiters on `arr[i]`, `arr`                |

**Predicate Re-evaluation**

When a waiter is notified:

1. Acquire a Read key to the waiter's path.
2. Evaluate $pred(value)$.
3. If true: transition to Write key via release-and-reacquire, execute $action(value)$, complete the Future.
4. If false: release Read key, remain in wait table.

**Ordering Guarantees**

1. A waiter that observes a mutation's effect is guaranteed to see all prior mutations by the same task (program order).
2. Across tasks, waiters see mutations in an order consistent with key acquisition order.
3. Spurious wakeups are permitted.

---

### 16.7 Transformations and Combinators

##### Definition

The `Async` type provides methods for transforming and composing asynchronous computations. These combinators enable declarative pipeline construction.

##### Static Semantics

**Core Transformation Signatures**

**`map`**: Transform output values.

```cursive
procedure map<Out; In; Result; E; U>(
    self: Async<Out, In, Result, E>,
    f: procedure(Out) -> U,
) -> Async<U, In, Result, E>
```

$$\frac{
    \Gamma \vdash a : \texttt{Async}\langle Out, In, R, E \rangle \quad
    \Gamma \vdash f : Out \to U
}{
    \Gamma \vdash a\texttt{~>map}(f) : \texttt{Async}\langle U, In, R, E \rangle
} \quad \text{(T-Async-Map)}$$

**`filter`**: Conditionally yield values.

```cursive
procedure filter<T; E>(
    self: Async<T, (), (), E>,
    predicate: procedure(const T) -> bool,
) -> Async<T, (), (), E>
```

**`take`**: Limit number of outputs.

```cursive
procedure take<T; E>(
    self: Async<T, (), (), E>,
    count: usize,
) -> Async<T, (), (), E>
```

**`fold`**: Reduce to single value.

```cursive
procedure fold<T; A; E>(
    self: Async<T, (), (), E>,
    initial: A,
    combine: procedure(A, T) -> A,
) -> Future<A, E>
```

**`chain`**: Sequence dependent async operations.

```cursive
procedure chain<T; U; E>(
    self: Future<T, E>,
    next: procedure(T) -> Future<U, E>,
) -> Future<U, E>
```

$$\frac{
    \Gamma \vdash a : \texttt{Future}\langle T, E \rangle \quad
    \Gamma \vdash f : T \to \texttt{Future}\langle U, E \rangle
}{
    \Gamma \vdash a\texttt{~>chain}(f) : \texttt{Future}\langle U, E \rangle
} \quad \text{(T-Async-Chain)}$$

**Combinator Return Types**

Combinator methods are declared with `Async` class return types, but each invocation produces a distinct generated modal implementing that class. The generated modal captures the source async and transformation closure:

$$\frac{
    \Gamma \vdash a : M_1 \quad
    M_1 <: \texttt{Async}\langle Out, In, R, E \rangle \quad
    \Gamma \vdash f : Out \to U \quad
    \text{gen\_map} \text{ is the generated modal for map}
}{
    \Gamma \vdash a\texttt{~>map}(f) : \text{gen\_map}\langle M_1, Out, U, In, R, E \rangle
} \quad \text{(T-Map-Concrete)}$$

The concrete generated type is a subtype of the declared return type:

$$\text{gen\_map}\langle M_1, Out, U, In, R, E \rangle <: \texttt{Async}\langle U, In, R, E \rangle$$

This enables direct chaining when types are statically known, while still supporting polymorphic use via `dyn Async<...>` when needed.

##### Dynamic Semantics

**Lazy Evaluation Model**

Combinators do not eagerly evaluate their source async. Instead, they return a new async that, when resumed, applies the transformation:

1. `map(f)`: When the source yields $v$, yield $f(v)$.
2. `filter(p)`: When the source yields $v$, yield $v$ only if $p(v)$ is true; otherwise, resume source.
3. `take(n)`: Yield the first $n$ values from source, then complete.
4. `fold(init, f)`: Consume all source values, accumulating via $f$, then complete with final accumulator.
5. `chain(f)`: When source completes with $v$, evaluate $f(v)$ and delegate to the resulting async.

---

### 16.8 Memory Model and State Representation

##### Dynamic Semantics

**Region Allocation**

Async state may be allocated in regions (§12.3):

```cursive
region task_arena {
    // Homogeneous array: all elements have same concrete type gen_range
    let ops = [
        ^task_arena range(0, 100),
        ^task_arena range(100, 200),
    ]

    loop op in ops {
        loop value in op {
            process(value)
        }
    }
}    // All async states freed here
```

For heterogeneous collections of async values, use `dyn Async<...>`:

```cursive
region task_arena {
    // Heterogeneous: requires dyn for type erasure
    let ops: [dyn Sequence<i32>] = [
        ^task_arena range(0, 100),
        ^task_arena countdown(50),
    ]
    // ...
}
```

**Heap Escape**

When async values escape their defining scope, they MUST be heap-allocated:

```cursive
procedure create_counter(
    start: i32, 
    heap: dyn HeapAllocator,
) -> Ptr<Sequence<i32>>@Valid {
    heap~>alloc(range(start, i32::MAX))
}
```

**Structured Lifetime**

An async operation allocated in a region MUST NOT escape that region:

$$\text{lifetime}(A) \leq \text{lifetime}(\text{region}(A))$$

##### Memory & Layout

**Generated Modal Structure**

For each async-returning procedure, a modal type MUST be generated containing:
- States required by the `Async` class (`@Suspended`, `@Completed`, `@Failed`)
- Class-required payload fields in each state
- Implementation fields for resumption state and captured locals

```cursive
// Source
procedure range(start: i32, end: i32) -> Sequence<i32> {
    var i = start
    loop i < end {
        yield i
        i += 1
    }
}

// Generated (conceptually):
modal gen_range <: Sequence<i32> {
    @Suspended {
        output: i32,       // required by Async (via Sequence alias)
        gen_point: u8,     // resumption point discriminant
        i: i32,            // captured local
        end: i32,          // captured parameter
    }

    @Completed {
        value: (),         // required by Async
    }

    // @Failed omitted: E = ! in Sequence<i32>
}
```

**Size Formula**

The size of a generated async modal depends on the procedure's local state:

$$\text{sizeof}(\text{gen\_}f) = \text{sizeof}(\text{Discriminant}) + \max_{S \in \text{States}}(\text{sizeof}(S.\text{payload}))$$

where each state's payload size includes both class-required fields and implementation fields:

$$\text{sizeof}(@\text{Suspended}.\text{payload}) = \text{sizeof}(Out) + \text{sizeof}(\text{gen\_point}) + \sum_{v \in \text{live\_locals}} \text{sizeof}(v)$$

$$\text{sizeof}(@\text{Completed}.\text{payload}) = \text{sizeof}(Result)$$

$$\text{sizeof}(@\text{Failed}.\text{payload}) = \text{sizeof}(E)$$

**Class Type Has No Fixed Size**

The `Async` class itself has no fixed size—it is an interface. Only concrete modal types implementing `Async` have defined sizes. When used polymorphically via `dyn Async<...>`, the value is accessed through indirection and the vtable provides size information.

**ABI Classification**

| Property                       | Classification         |
| :----------------------------- | :--------------------- |
| Generated modal layout         | Implementation-Defined |
| Implementation field placement | Implementation-Defined |
| Discriminant encoding          | Implementation-Defined |
| Alignment                      | Implementation-Defined |
| Generated type naming scheme   | Implementation-Defined |

---

### 16.9 Cancellation and Cleanup

##### Definition

Cancellation terminates an async operation before completion, ensuring proper cleanup of resources. Cursive uses drop-based cancellation.

##### Dynamic Semantics

**Drop-Based Cancellation**

When an `Async@Suspended` value is dropped, the operation is cancelled:

```cursive
{
    let op = ctx.net~>fetch(url)
    // op dropped here without being consumed
}    // Fetch cancelled
```

**Explicit Cancellation**

For controlled cancellation with a signal, use `race` with a cancellation condition:

```cursive
procedure cancellable_fetch(
    url: string,
    cancel: shared CancelFlag,
    net: dyn Network,
) -> Future<Response | Cancelled, NetError> {
    race {
        net~>fetch(url) -> |response| result response,
        cancel~>until(|c| c.is_set, |_| ()) -> |_| result Cancelled,
    }
}
```

**Cleanup Order**

When an async operation is cancelled:

1. Execution does not continue past the current suspension point.
2. `defer` blocks are executed in reverse declaration order.
3. `Drop` implementations are called for all captured resources.
4. Cleanup proceeds from innermost scope outward.

##### Constraints & Legality

**In-Flight I/O Behavior**

The behavior of in-flight I/O operations on cancellation is Implementation-Defined Behavior (IDB):

| I/O State          | Permitted Behaviors                    |
| :----------------- | :------------------------------------- |
| Pending read/write | Cancel, complete, or abandon           |
| Connected socket   | Close immediately or graceful shutdown |
| Open file          | Flush and close, or abandon            |

Implementations MUST document their cancellation behavior in the Conformance Dossier.

---

### 16.10 Error Handling

##### Definition

Errors propagate through async boundaries via the `?` operator or explicit pattern matching on the `@Failed` state.

##### Static Semantics

**Error Propagation via `?`**

The `?` operator propagates errors through async boundaries:

$$\frac{
    \text{ReturnType}(\text{enclosing}) = \texttt{Async}\langle Out, In, R, E_1 \rangle \quad
    \Gamma \vdash e : T \mid E_2 \quad
    E_2 <: E_1
}{
    \Gamma \vdash e? : T
} \quad \text{(T-Async-Try)}$$

**Error Type Constraints**

A procedure returning `Async<Out, In, Result, E>` where `E = !` MUST NOT contain error-propagating expressions for fallible operations. The generated modal for such a procedure omits the `@Failed` state per the uninhabited state omission rule (§9.3).

##### Dynamic Semantics

**Cleanup on Failure**

When an async operation fails:

1. The error is captured in the `@Failed` state.
2. `defer` blocks execute in reverse order.
3. `Drop` implementations run for all live bindings.
4. The async transitions to `@Failed { error }`.

```cursive
procedure with_temp_file(fs: dyn FileSystem) -> Stream<string, IoError> {
    let temp = fs~>create_temp()?
    defer { fs~>delete(temp) }    // Runs on completion, failure, or drop

    loop line in lines {
        fs~>write(temp, line)?
        yield line
    }

    result fs~>finalize(temp)?
}
```

---

### 16.11 Integration with Other Language Features

#### 16.11.1 Capability Requirements

##### Static Semantics

**Pure vs I/O Async**

| Async Category | Capability Required | Description            |
| :------------- | :------------------ | :--------------------- |
| Pure sequence  | None                | `range(0, 100)`        |
| I/O operation  | Specific dyn        | `fs~>read(file)`       |
| Timing         | `System` dyn        | `sys~>after(duration)` |
| Network        | `Network` dyn       | `net~>fetch(url)`      |

**Capability Propagation**

Capabilities captured by an async procedure remain valid across suspensions. The capability dyn is stored in the async state object.

**Reactor Capability**

The `Reactor` capability manages the lifecycle of suspended async operations awaiting external events:

```cursive
class Reactor {
    /// Run a Future to completion, polling until done.
    procedure run<T; E>(~, future: Future<T, E>) -> T | E

    /// Register a future for concurrent execution.
    procedure register<T; E>(~, future: Future<T, E>) -> FutureHandle<T, E>
}
```

The `sync` expression implicitly uses the reactor from `Context`.

**I/O Capability Methods**

Capability forms provide methods returning `Async`:

```cursive
class FileSystem {
    procedure open(~, path: string) -> FileHandle | IoError
    procedure read(~, file: FileHandle) -> Future<Data, IoError>
    procedure write(~, file: FileHandle, data: const [u8]) -> Future<usize, IoError>
    procedure close(~, file: FileHandle) -> Future<(), IoError>
}

class Network {
    procedure connect(~, addr: NetAddr) -> Future<Connection, NetError>
    procedure send(~, conn: Connection, data: const [u8]) -> Future<usize, NetError>
    procedure receive(~, conn: Connection) -> Future<[u8], NetError>
    procedure fetch(~, url: string) -> Future<Response, NetError>
}

class Time {
    procedure now(~) -> Timestamp
    procedure after(~, duration: Duration) -> Future<()>
}
```

---

#### 16.11.2 Permission and Capture Rules

##### Static Semantics

**Capture Table**

| Permission | Capturable | Behavior                                |
| :--------- | :--------- | :-------------------------------------- |
| `const T`  | Yes        | Referenced data must outlive the async  |
| `unique T` | Yes        | Exclusive access for async's lifetime   |
| `shared T` | Yes        | Subject to key rules at each suspension |
| `move T`   | Yes        | Ownership transfers to async's state    |

**`[[dynamic]]` Inheritance in Parallel Contexts**

When a `spawn` or `dispatch` block captures `shared` data, the `[[dynamic]]` status is inherited from the enclosing scope:

```cursive
[[dynamic]]
procedure parallel_updates(data: shared [Record], indices: [usize]) {
    parallel {
        loop idx in indices {
            spawn {
                data[idx].process()    // Inherits [[dynamic]]: runtime sync permitted
            }
        }
    }
}

procedure static_parallel(data: shared [Record]) {
    parallel {
        spawn { data[0].process() }    // Must be statically safe
        spawn { data[1].process() }    // Must be statically safe
        // spawn { data[i].process() } // Would be Error E-CON-0020
    }
}
```

**Capture with `[[dynamic]]` on Spawn**

The `[[dynamic]]` attribute may be applied to individual spawn expressions:

```cursive
procedure mixed_parallel(data: shared [Record], idx: usize) {
    parallel {
        spawn { data[0].process() }           // Static: must be provable
        [[dynamic]] spawn { data[idx].update() }  // Dynamic: runtime sync
    }
}
```

**Lifetime Constraint**

For any binding $b$ with permission $p$ captured by an async procedure:

$$\text{lifetime}(b) \geq \text{lifetime}(\texttt{Async})$$

The captured data MUST remain valid for the entire lifetime of the `Async` value.



##### Constraints & Legality

**Key State at Suspension**

Asynchronous suspension points interact with the Key System by managing the lifecycle of held keys.

**Default: Prohibition**

By default, `yield` or `yield from` (without `release`) MUST NOT occur while any keys are logically held.

$$\frac{
    \Gamma_{\text{keys}} \neq \emptyset \quad
    (\text{IsYield}(e) \lor \text{IsYieldFrom}(e)) \quad
    \texttt{release} \notin \text{modifiers}
}{
    \text{Emit}(\texttt{E-CON-0213})
} \quad \text{(A-No-Keys)}$$

**Opt-In: Yield Release**

To suspend within a key block, the programmer MUST use the `release` modifier (`yield release` or `yield release from`). This invokes the **Release-and-Reacquire** mechanism analogous to §14.7.2.

**Semantics:**
1. **Release:** All keys in $\Gamma_{keys}$ are released (LIFO order).
2. **Suspend:** The task yields. Keys are available to other tasks.
3. **Resume:** The task wakes.
4. **Re-acquire:** All keys in $\Gamma_{keys}$ are re-acquired (canonical order).

The `release` keyword serves as a visual marker that atomicity is broken. Local bindings derived from shared data prior to the `yield release` SHOULD be treated as potentially stale.

**Valid Pattern: Manual Key Release**

```cursive
procedure read_player_stats(player: shared Player, ctx: Context) -> Sequence<i32> {
    loop {
        let health = #player { player.health }    // Key acquired and released
        yield health                               // No key held
        yield from ctx.sys~>after(1.second)
    }
}
```

**Valid Pattern: Yield Release**

```cursive
procedure monitor_player(player: shared Player, ctx: Context) -> Sequence<i32> {
    #player {
        loop {
            let health = player.health
            yield release health       // Release key, suspend, reacquire key
            // health may be stale after reacquisition
        }
    }
}
```

**Invalid Pattern**

```cursive
procedure bad(player: shared Player) -> Sequence<i32> {
    #player {
        yield player.health    // ERROR E-CON-0213: key held at yield (no release)
    }
}
```

**Diagnostic Table**

| Code         | Severity | Condition                                                 | Detection    | Effect    |
| :----------- | :------- | :-------------------------------------------------------- | :----------- | :-------- |
| `E-CON-0213` | Error    | `yield` while key is held (without `release`)             | Compile-time | Rejection |
| `E-CON-0280` | Error    | Captured binding does not outlive async                   | Compile-time | Rejection |
| `E-CON-0281` | Error    | Async operation escapes its region                        | Compile-time | Rejection |
| `W-CON-0201` | Warning  | Large captured state (performance)                        | Compile-time | Warning   |
| `W-CON-0011` | Warning  | Access to potentially stale binding after `yield release` | Compile-time | Warning   |

---

#### 16.11.3 Parallel Block Composition

##### Static Semantics

**Async in `spawn` Blocks**

`spawn` blocks may contain async operations:

```cursive
parallel ctx.cpu {
    spawn {
        let data = yield from ctx.net~>fetch(url)
        process(data)
    }
    spawn {
        let config = yield from ctx.fs~>read("/config")
        apply(config)
    }
}
```

When a spawned task encounters `yield from` on an I/O Future:
1. The task suspends.
2. The reactor registers the I/O operation.
3. The worker picks up other work items.
4. When I/O is ready, the reactor notifies and the task resumes.

**Async in `dispatch`**

`dispatch` may iterate over async sequences:

```cursive
parallel ctx.cpu {
    dispatch item in async_producer() {
        process(item)
    }
}
```

Each yielded item is processed as a separate work item.

**Structured Concurrency**

All async operations spawned within a `parallel` block MUST complete or be cancelled before the block exits.

When a `parallel` block reaches its closing brace:
1. Wait for all `spawn` tasks to complete or fail.
2. If the block exits early (panic, break), cancel all pending tasks.
3. Run `defer` and `Drop` for all task-local state.
4. Propagate the first panic (if any) after cleanup.

---

# Part 5: Metaprogramming

---

## Clause 17: Compile-Time Execution

### 17.1 The Comptime Environment

##### Definition

**Compile-time execution** is the evaluation of Cursive code during the Metaprogramming Phase of translation, prior to semantic analysis of the complete program. Code executed at compile time operates within the **comptime environment**.

The **comptime environment** is a sandboxed execution context, denoted $\Gamma_{ct}$, distinct from the runtime environment $\Gamma_{rt}$.

**Formal Definition**

$$\Gamma_{ct} ::= (\Sigma_{stdlib}, \Sigma_{imports}, \Sigma_{types}, \Sigma_{caps}, \emptyset)$$

| Component          | Description                                                 |
| :----------------- | :---------------------------------------------------------- |
| $\Sigma_{stdlib}$  | Standard library subset available at compile time           |
| $\Sigma_{imports}$ | Modules imported via `import` declarations                  |
| $\Sigma_{types}$   | Type definitions visible at the point of comptime execution |
| $\Sigma_{caps}$    | Compile-time capabilities provided to the current context   |
| $\emptyset$        | Empty set: no shared mutable state with $\Gamma_{rt}$       |

##### Static Semantics

**Isolation Property**

The comptime environment is isolated from the runtime environment. Comptime code MUST NOT access:

1. Runtime memory or heap allocations from $\Gamma_{rt}$
2. File handles, network sockets, or I/O resources (except via `ProjectFiles` capability, §17.5.3)
3. Foreign function interfaces (§21)
4. Runtime capabilities (§13)

**Determinism Property**

Comptime code MUST be deterministic. For any comptime expression $e$ evaluated in environment $\Gamma_{ct}$:

$$\Gamma_{ct} \vdash e \Downarrow v_1 \land \Gamma_{ct} \vdash e \Downarrow v_2 \implies v_1 = v_2$$

**Termination Property**

Comptime execution MUST terminate. Implementations MUST enforce resource limits as specified in §17.8.

**Purity Requirement**

All expressions evaluated in comptime context MUST be pure as defined in §11.1.1, with exceptions:

1. Operations on compile-time capabilities (§17.5)
2. File reads via `ProjectFiles` capability (§17.5.3)
3. Diagnostic emissions via `ComptimeDiagnostics` capability (§17.5.4)

##### Constraints & Legality

The following constructs are prohibited in comptime context:

| Construct                              |
| :------------------------------------- |
| `unsafe` blocks                        |
| FFI calls (`extern` procedures)        |
| Runtime capability access              |
| Random number generation               |
| System time queries                    |
| I/O operations (except `ProjectFiles`) |

**Diagnostic Table**

| Code         | Severity | Condition                                       | Detection    | Effect    |
| :----------- | :------- | :---------------------------------------------- | :----------- | :-------- |
| `E-CTE-0001` | Error    | `unsafe` block in comptime context              | Compile-time | Rejection |
| `E-CTE-0002` | Error    | FFI call in comptime context                    | Compile-time | Rejection |
| `E-CTE-0003` | Error    | Runtime capability access in comptime context   | Compile-time | Rejection |
| `E-CTE-0004` | Error    | Non-deterministic operation in comptime context | Compile-time | Rejection |
| `E-CTE-0005` | Error    | Prohibited I/O operation in comptime context    | Compile-time | Rejection |

---

### 17.2 Comptime-Available Types

##### Definition

A **comptime-available type** is a type whose values can exist within the comptime environment $\Gamma_{ct}$. The predicate $\text{IsComptimeAvailable}(T)$ determines whether type $T$ is comptime-available.

**Formal Definition**

$$\text{IsComptimeAvailable}(T) \iff T \in \mathcal{T}_{ct}$$

where $\mathcal{T}_{ct}$ is defined inductively:

$$\mathcal{T}_{ct} ::= \text{Primitive} \mid \texttt{string} \mid \texttt{Type} \mid \texttt{Ast} \mid \text{Tuple}(\mathcal{T}_{ct}^*) \mid \text{Array}(\mathcal{T}_{ct}) \mid \text{Record}_{ct} \mid \text{Enum}_{ct}$$

##### Static Semantics

**Typing Rules**

Primitive types are comptime-available:

$$\frac{T \in \{\texttt{bool}, \texttt{i8}, \texttt{i16}, \texttt{i32}, \texttt{i64}, \texttt{i128}, \texttt{u8}, \texttt{u16}, \texttt{u32}, \texttt{u64}, \texttt{u128}, \texttt{f32}, \texttt{f64}, \texttt{char}\}}{\text{IsComptimeAvailable}(T)} \quad \text{(CT-Prim)}$$

String type is comptime-available:

$$\frac{}{\text{IsComptimeAvailable}(\texttt{string})} \quad \text{(CT-String)}$$

Metatypes are comptime-available:

$$\frac{}{\text{IsComptimeAvailable}(\texttt{Type})} \quad \text{(CT-Type)}$$

$$\frac{}{\text{IsComptimeAvailable}(\texttt{Ast})} \quad \text{(CT-Ast)}$$

Tuple types are comptime-available if all components are:

$$\frac{\forall i \in 1..n.\; \text{IsComptimeAvailable}(T_i)}{\text{IsComptimeAvailable}((T_1, \ldots, T_n))} \quad \text{(CT-Tuple)}$$

Array types are comptime-available if the element type is:

$$\frac{\text{IsComptimeAvailable}(T)}{\text{IsComptimeAvailable}([T; n])} \quad \text{(CT-Array)}$$

Record types are comptime-available if all field types are and no field has a runtime capability type:

$$\frac{\texttt{record}\; R\; \{\; f_1: T_1, \ldots, f_n: T_n \;\} \quad \forall i.\; \text{IsComptimeAvailable}(T_i)}{\text{IsComptimeAvailable}(R)} \quad \text{(CT-Record)}$$

Enum types are comptime-available if all variant payload types are:

$$\frac{\texttt{enum}\; E\; \{\; V_1(T_1), \ldots, V_n(T_n) \;\} \quad \forall i.\; \text{IsComptimeAvailable}(T_i)}{\text{IsComptimeAvailable}(E)} \quad \text{(CT-Enum)}$$

##### Constraints & Legality

The following types are NOT comptime-available:

| Type Category          |
| :--------------------- |
| Reference types        |
| Capability types       |
| Modal types            |
| Types containing `dyn` |

**Diagnostic Table**

| Code         | Severity | Condition                                    | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------- | :----------- | :-------- |
| `E-CTE-0010` | Error    | Non-comptime-available type in comptime expr | Compile-time | Rejection |
| `E-CTE-0011` | Error    | Reference type in comptime context           | Compile-time | Rejection |
| `E-CTE-0012` | Error    | Capability type in comptime context          | Compile-time | Rejection |

---

### 17.3 Comptime Blocks and Expressions

##### Definition

A **comptime block** is a block of statements executed during the Metaprogramming Phase. A **comptime expression** is an expression evaluated at compile time, yielding a value incorporated into the program.

##### Syntax & Declaration

**Grammar**

```ebnf
comptime_block ::= "comptime" block
comptime_expr  ::= "comptime" "{" expr "}"
```

The `block` production is defined in §4.2. The `expr` production is defined in §7.

##### Static Semantics

**Typing Rules**

Comptime block (statement context):

$$\frac{\Gamma_{ct} \vdash \textit{stmts} : ()}{\Gamma \vdash \texttt{comptime}\; \{\; \textit{stmts} \;\} : ()} \quad \text{(T-ComptimeBlock)}$$

Comptime expression (expression context):

$$\frac{\Gamma_{ct} \vdash e : T \quad \text{IsComptimeAvailable}(T)}{\Gamma \vdash \texttt{comptime}\; \{\; e \;\} : T} \quad \text{(T-ComptimeExpr)}$$

**Validation**

The body of a comptime block or expression MUST satisfy all comptime restrictions (§17.1).

##### Dynamic Semantics

**Evaluation**

1. The implementation evaluates the comptime block body in $\Gamma_{ct}$.
2. For comptime expressions, the resulting value $v$ is converted to a literal node in the AST.
3. The literal node replaces the comptime expression in subsequent translation phases.

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                     | Detection    | Effect    |
| :----------- | :------- | :-------------------------------------------- | :----------- | :-------- |
| `E-CTE-0020` | Error    | Comptime block contains prohibited construct  | Compile-time | Rejection |
| `E-CTE-0021` | Error    | Comptime expression has non-comptime type     | Compile-time | Rejection |
| `E-CTE-0022` | Error    | Comptime evaluation diverges (exceeds limits) | Compile-time | Rejection |
| `E-CTE-0023` | Error    | Comptime evaluation panics                    | Compile-time | Rejection |

---

### 17.4 Comptime Procedures

##### Definition

A **comptime procedure** is a procedure callable only during the Metaprogramming Phase. Comptime procedures operate on comptime-available types and may access comptime capabilities.

##### Syntax & Declaration

**Grammar**

```ebnf
comptime_procedure_decl ::= "comptime" "procedure" identifier generic_params? "(" param_list ")" "->" return_type requires_clause? block

requires_clause ::= "requires" comptime_predicate ("," comptime_predicate)*
comptime_predicate ::= comptime_expr
```

##### Static Semantics

**Typing Rules**

Comptime procedure declaration well-formedness:

$$\frac{
    \forall i.\; \text{IsComptimeAvailable}(T_i) \quad
    \text{IsComptimeAvailable}(R) \quad
    \Gamma_{ct}, x_1: T_1, \ldots, x_n: T_n \vdash \textit{body} : R
}{
    \Gamma \vdash \texttt{comptime procedure}\; f(x_1: T_1, \ldots, x_n: T_n) \to R\; \{\; \textit{body} \;\} : \text{wf}
} \quad \text{(T-ComptimeProc)}$$

Comptime procedure call:

$$\frac{
    \Gamma_{ct} \vdash f : (T_1, \ldots, T_n) \to R \quad
    \forall i.\; \Gamma_{ct} \vdash e_i : T_i
}{
    \Gamma_{ct} \vdash f(e_1, \ldots, e_n) : R
} \quad \text{(T-ComptimeCall)}$$

**Validation**

1. All parameter types MUST be comptime-available.
2. The return type MUST be comptime-available.
3. The procedure body MUST satisfy comptime restrictions.
4. If a `where` clause is present, all predicates MUST evaluate to `true` at each call site.

##### Constraints & Legality

Comptime procedures MUST NOT:
- Call runtime-only procedures
- Access runtime capabilities
- Perform I/O (except via comptime capabilities)

**Diagnostic Table**

| Code         | Severity | Condition                                      | Detection    | Effect    |
| :----------- | :------- | :--------------------------------------------- | :----------- | :-------- |
| `E-CTE-0030` | Error    | Parameter type not comptime-available          | Compile-time | Rejection |
| `E-CTE-0031` | Error    | Return type not comptime-available             | Compile-time | Rejection |
| `E-CTE-0032` | Error    | Comptime procedure body violates restrictions  | Compile-time | Rejection |
| `E-CTE-0033` | Error    | Requires clause predicate evaluates to false   | Compile-time | Rejection |
| `E-CTE-0034` | Error    | Comptime procedure called from runtime context | Compile-time | Rejection |

---

### 17.5 Comptime Capabilities

##### Definition

**Comptime capabilities** are capability types that authorize specific compile-time operations. Unlike runtime capabilities, comptime capabilities are intrinsic and operate on the compilation unit rather than external resources.

#### 17.5.1 TypeEmitter

##### Definition

The **TypeEmitter capability** authorizes emission of generated code into the compilation unit. This capability is the sole mechanism for code generation in Cursive.

##### Syntax & Declaration

```cursive
capability TypeEmitter {
    /// Emits an AST node at module scope
    procedure emit(~, ast: Ast)
    
    /// Acquires exclusive write access for a (type, form) pair
    procedure acquire_write_key(~, target: Type, form: Type) -> WriteKey
    
    /// Returns the target type for derive contexts
    procedure target_type(~) -> Type
}
```

##### Static Semantics

**Provision**

The `TypeEmitter` capability is provided to:

1. Comptime blocks annotated with `[[emit]]`
2. Derive target procedure bodies (§19.2)

The capability dyn is available via the identifier `emitter`.

**Typing Rules**

$$\frac{
    \Gamma_{ct} \vdash \textit{emitter} : \texttt{TypeEmitter} \quad
    \Gamma_{ct} \vdash \textit{ast} : \texttt{Ast}
}{
    \Gamma_{ct} \vdash \textit{emitter}\texttt{\textasciitilde>emit}(\textit{ast}) : ()
} \quad \text{(T-Emit)}$$

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                       | Detection    | Effect    |
| :----------- | :------- | :---------------------------------------------- | :----------- | :-------- |
| `E-CTE-0040` | Error    | Emit operation without `TypeEmitter` capability | Compile-time | Rejection |
| `E-CTE-0041` | Error    | `[[emit]]` attribute on non-comptime block      | Compile-time | Rejection |
| `E-CTE-0042` | Error    | Emitted AST is ill-formed                       | Compile-time | Rejection |

#### 17.5.2 Introspect

##### Definition

The **Introspect capability** authorizes reflection over type structure at compile time. This capability provides access to field information, variant information, form implementations, and type metadata.

##### Syntax & Declaration

```cursive
capability Introspect {
    /// Returns the type category
    procedure category<T>() -> TypeCategory
    
    /// Returns field information for record types
    procedure fields<T>() -> [FieldInfo]
        where category::<T>() == TypeCategory::Record
    
    /// Returns variant information for enum types
    procedure variants<T>() -> [VariantInfo]
        where category::<T>() == TypeCategory::Enum
    
    /// Returns state information for modal types
    procedure states<T>() -> [StateInfo]
        where category::<T>() == TypeCategory::Modal
    
    /// Tests whether a type implements a form
    procedure implements_form<T; F>() -> bool
    
    /// Returns the type's name as a string
    procedure type_name<T>() -> string
    
    /// Returns the type's module path
    procedure module_path<T>() -> string
}
```

The supporting types are:

```cursive
enum TypeCategory {
    Record,
    Enum,
    Modal,
    Primitive,
    Tuple,
    Array,
    Procedure,
    Reference,
    Generic,
}

record FieldInfo {
    name: string,
    type_id: Type,
    offset: u64,
    visibility: Visibility,
}

record VariantInfo {
    name: string,
    discriminant: i64,
    fields: [FieldInfo],
}

record StateInfo {
    name: string,
    transitions: [TransitionInfo],
}

record TransitionInfo {
    name: string,
    target_state: string,
    parameters: [FieldInfo],
}

enum Visibility {
    Public,
    Private,
    Module,
}
```

##### Static Semantics

**Provision**

The `Introspect` capability is implicitly provided to all comptime contexts. The capability dyn is available via the identifier `introspect`.

**Typing Rules**

$$\frac{
    \Gamma_{ct} \vdash T : \texttt{Type}
}{
    \Gamma_{ct} \vdash \textit{introspect}\texttt{\textasciitilde>category::<}T\texttt{>()} : \texttt{TypeCategory}
} \quad \text{(T-Introspect-Category)}$$

$$\frac{
    \Gamma_{ct} \vdash T : \texttt{Type} \quad
    \text{category}(T) = \texttt{Record}
}{
    \Gamma_{ct} \vdash \textit{introspect}\texttt{\textasciitilde>fields::<}T\texttt{>()} : [\texttt{FieldInfo}]
} \quad \text{(T-Introspect-Fields)}$$

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                          | Detection    | Effect    |
| :----------- | :------- | :--------------------------------- | :----------- | :-------- |
| `E-CTE-0050` | Error    | `fields` called on non-record type | Compile-time | Rejection |
| `E-CTE-0051` | Error    | `variants` called on non-enum type | Compile-time | Rejection |
| `E-CTE-0052` | Error    | `states` called on non-modal type  | Compile-time | Rejection |
| `E-CTE-0053` | Error    | Introspection of incomplete type   | Compile-time | Rejection |

#### 17.5.3 ProjectFiles

##### Definition

The **ProjectFiles capability** authorizes read-only access to files within the project directory during compile time.

##### Syntax & Declaration

```cursive
capability ProjectFiles {
    procedure read(~, path: string) -> Result<string, FileError>
    procedure read_bytes(~, path: string) -> Result<[u8], FileError>
    procedure exists(~, path: string) -> bool
    procedure list_dir(~, path: string) -> Result<[string], FileError>
    procedure project_root(~) -> string
}

enum FileError {
    NotFound { path: string },
    PermissionDenied { path: string },
    NotUtf8 { path: string },
    IoError { message: string },
}
```

##### Static Semantics

**Provision**

The `ProjectFiles` capability is provided only to comptime blocks annotated with `[[files]]`. The capability dyn is available via the identifier `files`.

**Path Resolution**

All paths are relative to the project root directory. The following restrictions apply:

1. Paths MUST be relative.
2. Paths MUST NOT contain `..` components traversing above project root.
3. Paths MUST NOT follow symbolic links escaping the project directory.

**Determinism**

File contents are captured at the start of the Metaprogramming Phase. Modifications during compilation do not affect values returned by `read` operations.

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                        | Detection    | Effect    |
| :----------- | :------- | :----------------------------------------------- | :----------- | :-------- |
| `E-CTE-0060` | Error    | File operation without `ProjectFiles` capability | Compile-time | Rejection |
| `E-CTE-0061` | Error    | `[[files]]` attribute on non-comptime block      | Compile-time | Rejection |
| `E-CTE-0062` | Error    | Path escapes project directory                   | Compile-time | Rejection |
| `E-CTE-0063` | Error    | Absolute path in file operation                  | Compile-time | Rejection |
| `E-CTE-0064` | Error    | File not found                                   | Compile-time | Rejection |

#### 17.5.4 ComptimeDiagnostics

##### Definition

The **ComptimeDiagnostics capability** authorizes emission of compile-time diagnostics including errors, warnings, and notes.

##### Syntax & Declaration

```cursive
capability ComptimeDiagnostics {
    procedure error(~!, message: string) -> !
    procedure error_at(~!, message: string, span: SourceSpan) -> !
    procedure warning(~, message: string)
    procedure warning_at(~, message: string, span: SourceSpan)
    procedure note(~, message: string)
    procedure note_at(~, message: string, span: SourceSpan)
    procedure current_span(~) -> SourceSpan
    procedure current_module(~) -> string
}

record SourceSpan {
    file: string,
    start_line: u32,
    start_column: u32,
    end_line: u32,
    end_column: u32,
}
```

##### Static Semantics

**Provision**

The `ComptimeDiagnostics` capability is implicitly provided to all comptime contexts and derive target procedures. The capability dyn is available via the identifier `diagnostics`.

**Never Type**

The `error` and `error_at` procedures have return type `!`. These procedures terminate the Metaprogramming Phase and do not return.

##### Dynamic Semantics

**Error Emission**

When `error` or `error_at` is invoked:

1. The diagnostic message is recorded with severity Error.
2. The Metaprogramming Phase terminates.
3. Compilation fails with diagnostic code `E-CTE-0070`.

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                             | Detection    | Effect    |
| :----------- | :------- | :------------------------------------ | :----------- | :-------- |
| `E-CTE-0070` | Error    | Comptime error emitted by user code   | Compile-time | Rejection |
| `W-CTE-0071` | Warning  | Comptime warning emitted by user code | Compile-time | Continue  |

---

### 17.6 Comptime Control Flow

##### Definition

**Comptime control flow** constructs are control flow statements evaluated entirely at compile time, producing unrolled or selected code.

##### Syntax & Declaration

**Grammar**

```ebnf
comptime_if  ::= "comptime" "if" comptime_expr block ("else" (comptime_if | block))?
comptime_for ::= "comptime" "for" pattern "in" comptime_expr block
```

##### Static Semantics

**Typing Rules**

Comptime if:

$$\frac{
    \Gamma_{ct} \vdash e : \texttt{bool} \quad
    \Gamma \vdash \textit{then} : T \quad
    \Gamma \vdash \textit{else} : T
}{
    \Gamma \vdash \texttt{comptime if}\; e\; \textit{then}\; \texttt{else}\; \textit{else} : T
} \quad \text{(T-ComptimeIf)}$$

Comptime for:

$$\frac{
    \Gamma_{ct} \vdash e : [T] \quad
    \Gamma, x: T \vdash \textit{body} : U
}{
    \Gamma \vdash \texttt{comptime for}\; x\; \texttt{in}\; e\; \textit{body} : [U]
} \quad \text{(T-ComptimeFor)}$$

##### Dynamic Semantics

**Comptime If Evaluation**

1. Evaluate condition $e$ in $\Gamma_{ct}$ to obtain boolean $b$.
2. If $b = \texttt{true}$, include the then-block in the AST.
3. If $b = \texttt{false}$, include the else-block (if present) in the AST.
4. The non-selected branch is discarded and not type-checked.

**Comptime For Evaluation**

1. Evaluate iterator expression $e$ in $\Gamma_{ct}$ to obtain sequence $[v_1, \ldots, v_n]$.
2. For each $v_i$, instantiate the loop body with pattern bound to $v_i$.
3. Concatenate all instantiated bodies in order.
4. The resulting unrolled code replaces the comptime for in the AST.

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                    | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------- | :----------- | :-------- |
| `E-CTE-0080` | Error    | Comptime if condition not comptime-evaluable | Compile-time | Rejection |
| `E-CTE-0081` | Error    | Comptime if condition not boolean            | Compile-time | Rejection |
| `E-CTE-0082` | Error    | Comptime for iterator not comptime-evaluable | Compile-time | Rejection |
| `E-CTE-0083` | Error    | Comptime for iterator not iterable           | Compile-time | Rejection |
| `E-CTE-0084` | Error    | Comptime for exceeds iteration limit         | Compile-time | Rejection |

---

### 17.7 Comptime Assertions

##### Definition

A **comptime assertion** is a compile-time check that halts compilation if the condition is false.

##### Syntax & Declaration

**Grammar**

```ebnf
comptime_assert ::= "comptime" "assert" "(" comptime_expr ("," string_literal)? ")"
```

##### Static Semantics

**Typing Rule**

$$\frac{
    \Gamma_{ct} \vdash e : \texttt{bool}
}{
    \Gamma \vdash \texttt{comptime assert}(e) : ()
} \quad \text{(T-ComptimeAssert)}$$

##### Dynamic Semantics

**Evaluation**

1. Evaluate condition $e$ in $\Gamma_{ct}$ to obtain boolean $b$.
2. If $b = \texttt{true}$, the assertion succeeds; continue compilation.
3. If $b = \texttt{false}$, emit diagnostic `E-CTE-0090` with optional message and halt.

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                      | Detection    | Effect    |
| :----------- | :------- | :--------------------------------------------- | :----------- | :-------- |
| `E-CTE-0090` | Error    | Comptime assertion failed                      | Compile-time | Rejection |
| `E-CTE-0091` | Error    | Comptime assertion condition not boolean       | Compile-time | Rejection |
| `E-CTE-0092` | Error    | Comptime assertion condition not comptime-eval | Compile-time | Rejection |

---

### 17.8 Resource Limits

##### Definition

**Resource limits** are implementation-enforced bounds on compile-time execution to guarantee termination.

**Formal Definition**

A conforming implementation MUST enforce the following minimum guaranteed capacities:

| Resource                | Minimum Limit | Diagnostic on Exceed |
| :---------------------- | :------------ | :------------------- |
| Recursion depth         | 128 calls     | `E-CTE-0100`         |
| Loop iterations         | 65,536        | `E-CTE-0101`         |
| Memory allocation       | 64 MiB        | `E-CTE-0102`         |
| AST nodes generated     | 1,000,000     | `E-CTE-0103`         |
| Comptime procedure time | 60 seconds    | `E-CTE-0104`         |

Implementations MAY support higher limits. The actual limits are implementation-defined.

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                              | Detection    | Effect    |
| :----------- | :------- | :------------------------------------- | :----------- | :-------- |
| `E-CTE-0100` | Error    | Comptime recursion depth exceeded      | Compile-time | Rejection |
| `E-CTE-0101` | Error    | Comptime iteration limit exceeded      | Compile-time | Rejection |
| `E-CTE-0102` | Error    | Comptime memory limit exceeded         | Compile-time | Rejection |
| `E-CTE-0103` | Error    | Comptime AST node limit exceeded       | Compile-time | Rejection |
| `E-CTE-0104` | Error    | Comptime execution time limit exceeded | Compile-time | Rejection |

---

## Clause 18: Type Reflection

### 18.1 The Type Metatype

##### Definition

The **Type metatype** is a compile-time type whose values represent Cursive types. A value of type `Type` is a reified type descriptor.

**Formal Definition**

$$\texttt{Type} : \texttt{Kind}$$

where $\texttt{Kind}$ is the universe of types. For each type $T$ in the program, there exists a unique value $\langle T \rangle : \texttt{Type}$.

##### Syntax & Declaration

**Grammar**

```ebnf
type_literal ::= "Type" "::<" type ">"
```

##### Static Semantics

**Typing Rule**

$$\frac{
    \Gamma \vdash T : \texttt{Type}_\text{wf}
}{
    \Gamma_{ct} \vdash \texttt{Type::<}T\texttt{>} : \texttt{Type}
} \quad \text{(T-TypeLiteral)}$$

The subscript $\text{wf}$ indicates that $T$ must be a well-formed type in context $\Gamma$.

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                               | Detection    | Effect    |
| :----------- | :------- | :-------------------------------------- | :----------- | :-------- |
| `E-CTE-0410` | Error    | Type argument to Type::<> is ill-formed | Compile-time | Rejection |
| `E-CTE-0411` | Error    | Type::<> used in runtime context        | Compile-time | Rejection |

---

### 18.2 Type Categories

##### Definition

A **type category** classifies a type by its fundamental structure. The `TypeCategory` enumeration partitions all Cursive types into disjoint categories.

**Formal Definition**

$$\text{category} : \texttt{Type} \to \texttt{TypeCategory}$$

$$\texttt{TypeCategory} ::= \texttt{Record} \mid \texttt{Enum} \mid \texttt{Modal} \mid \texttt{Primitive} \mid \texttt{Tuple} \mid \texttt{Array} \mid \texttt{Procedure} \mid \texttt{Reference} \mid \texttt{Generic}$$

##### Syntax & Declaration

```cursive
enum TypeCategory {
    Record,
    Enum,
    Modal,
    Primitive,
    Tuple,
    Array,
    Procedure,
    Reference,
    Generic,
}
```

##### Static Semantics

**Category Assignment**

| Type Form                          | Category    |
| :--------------------------------- | :---------- |
| `record R { ... }`                 | `Record`    |
| `enum E { ... }`                   | `Enum`      |
| `modal M { ... }`                  | `Modal`     |
| `bool`, `i32`, `f64`, `char`, etc. | `Primitive` |
| `(T1, T2, ...)`                    | `Tuple`     |
| `[T; n]`, `[T]`                    | `Array`     |
| `procedure(...) -> T`              | `Procedure` |
| `&T`, `&unique T`, `&const T`      | `Reference` |
| `T` where `T` is type parameter    | `Generic`   |

**Introspection Function**

```cursive
comptime procedure category<T>() -> TypeCategory
```

This procedure is available via the `Introspect` capability (§17.5.2).

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                         | Detection    | Effect    |
| :----------- | :------- | :-------------------------------- | :----------- | :-------- |
| `E-CTE-0420` | Error    | Category query on incomplete type | Compile-time | Rejection |

---

### 18.3 Structural Introspection

##### Definition

**Structural introspection** provides compile-time access to the internal structure of composite types: fields of records, variants of enums, and states of modal types.

#### 18.3.1 Record Introspection

##### Definition

Record introspection yields the ordered sequence of fields comprising a record type.

##### Syntax & Declaration

```cursive
record FieldInfo {
    name: string,
    type_id: Type,
    offset: u64,
    visibility: Visibility,
}

enum Visibility {
    Public,
    Private,
    Module,
}
```

**Introspection Function**

```cursive
comptime procedure fields<T>() -> [FieldInfo]
    requires category::<T>() == TypeCategory::Record
```

##### Static Semantics

**Typing Rule**

$$\frac{
    \Gamma_{ct} \vdash T : \texttt{Type} \quad
    \text{category}(T) = \texttt{Record}
}{
    \Gamma_{ct} \vdash \texttt{introspect\textasciitilde>fields::<}T\texttt{>()} : [\texttt{FieldInfo}]
} \quad \text{(T-Fields)}$$

**Return Value**

The returned array contains one `FieldInfo` for each field of $T$, in declaration order. For a record with $n$ fields, the result has length $n$.

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                          | Detection    | Effect    |
| :----------- | :------- | :--------------------------------- | :----------- | :-------- |
| `E-CTE-0430` | Error    | `fields` called on non-record type | Compile-time | Rejection |

#### 18.3.2 Enum Introspection

##### Definition

Enum introspection yields the variants comprising an enum type, including discriminant values and payload fields.

##### Syntax & Declaration

```cursive
record VariantInfo {
    name: string,
    discriminant: i64,
    fields: [FieldInfo],
}
```

**Introspection Function**

```cursive
comptime procedure variants<T>() -> [VariantInfo]
    requires category::<T>() == TypeCategory::Enum
```

##### Static Semantics

**Typing Rule**

$$\frac{
    \Gamma_{ct} \vdash T : \texttt{Type} \quad
    \text{category}(T) = \texttt{Enum}
}{
    \Gamma_{ct} \vdash \texttt{introspect\textasciitilde>variants::<}T\texttt{>()} : [\texttt{VariantInfo}]
} \quad \text{(T-Variants)}$$

**Return Value**

The returned array contains one `VariantInfo` for each variant of $T$, in declaration order. Unit variants have an empty `fields` array.

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                          | Detection    | Effect    |
| :----------- | :------- | :--------------------------------- | :----------- | :-------- |
| `E-CTE-0440` | Error    | `variants` called on non-enum type | Compile-time | Rejection |

#### 18.3.3 Modal Introspection

##### Definition

Modal introspection yields the states and transitions comprising a modal type.

##### Syntax & Declaration

```cursive
record StateInfo {
    name: string,
    fields: [FieldInfo],
    transitions: [TransitionInfo],
}

record TransitionInfo {
    name: string,
    target_state: string,
    parameters: [FieldInfo],
}
```

**Introspection Function**

```cursive
comptime procedure states<T>() -> [StateInfo]
    requires category::<T>() == TypeCategory::Modal
```

##### Static Semantics

**Typing Rule**

$$\frac{
    \Gamma_{ct} \vdash T : \texttt{Type} \quad
    \text{category}(T) = \texttt{Modal}
}{
    \Gamma_{ct} \vdash \texttt{introspect\textasciitilde>states::<}T\texttt{>()} : [\texttt{StateInfo}]
} \quad \text{(T-States)}$$

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                         | Detection    | Effect    |
| :----------- | :------- | :-------------------------------- | :----------- | :-------- |
| `E-CTE-0450` | Error    | `states` called on non-modal type | Compile-time | Rejection |

---

### 18.4 Form Introspection

##### Definition

**Form introspection** determines whether a type implements a given form and retrieves the procedures required by a form.

##### Syntax & Declaration

**Introspection Functions**

```cursive
comptime procedure implements_form<T; F>() -> bool

comptime procedure required_procedures<F>() -> [ProcedureInfo]
    requires is_form::<F>()
```

```cursive
record ProcedureInfo {
    name: string,
    parameters: [ParameterInfo],
    return_type: Type,
    is_procedure: bool,
}

record ParameterInfo {
    name: string,
    type_id: Type,
    mode: ParameterMode,
}

enum ParameterMode {
    Value,
    Ref,
    RefUnique,
    RefConst,
    Self_,
}
```

##### Static Semantics

**Typing Rules**

$$\frac{
    \Gamma_{ct} \vdash T : \texttt{Type} \quad
    \Gamma_{ct} \vdash F : \texttt{Type} \quad
    \text{is\_form}(F)
}{
    \Gamma_{ct} \vdash \texttt{introspect\textasciitilde>implements\_form::<}T, F\texttt{>()} : \texttt{bool}
} \quad \text{(T-ImplementsForm)}$$

$$\frac{
    \Gamma_{ct} \vdash F : \texttt{Type} \quad
    \text{is\_form}(F)
}{
    \Gamma_{ct} \vdash \texttt{introspect\textasciitilde>required\_procedures::<}F\texttt{>()} : [\texttt{ProcedureInfo}]
} \quad \text{(T-RequiredProcs)}$$

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                  | Detection    | Effect    |
| :----------- | :------- | :----------------------------------------- | :----------- | :-------- |
| `E-CTE-0460` | Error    | `required_procedures` called on non-form   | Compile-time | Rejection |
| `E-CTE-0461` | Error    | `implements_form` with non-form second arg | Compile-time | Rejection |

---

### 18.5 Type Predicates

##### Definition

**Type predicates** are comptime procedures returning boolean values based on type properties.

##### Syntax & Declaration

```cursive
comptime procedure is_record<T>() -> bool
comptime procedure is_enum<T>() -> bool
comptime procedure is_modal<T>() -> bool
comptime procedure is_primitive<T>() -> bool
comptime procedure is_tuple<T>() -> bool
comptime procedure is_array<T>() -> bool
comptime procedure is_sized<T>() -> bool
comptime procedure is_copy<T>() -> bool
comptime procedure is_form<T>() -> bool
```

##### Static Semantics

**Equivalences**

$$\texttt{is\_record::<}T\texttt{>()} \equiv (\text{category}(T) = \texttt{Record})$$

$$\texttt{is\_enum::<}T\texttt{>()} \equiv (\text{category}(T) = \texttt{Enum})$$

$$\texttt{is\_modal::<}T\texttt{>()} \equiv (\text{category}(T) = \texttt{Modal})$$

$$\texttt{is\_sized::<}T\texttt{>()} \equiv \exists n.\; \text{sizeof}(T) = n$$

$$\texttt{is\_copy::<}T\texttt{>()} \equiv T <: \texttt{Copy}$$

**Usage in Generic Constraints**

Type predicates enable conditional code generation:

```ebnf
comptime_requires ::= "requires" comptime_predicate_expr
comptime_predicate_expr ::= identifier "::<" type_args ">" "()"
                          | comptime_predicate_expr "&&" comptime_predicate_expr
                          | comptime_predicate_expr "||" comptime_predicate_expr
                          | "!" comptime_predicate_expr
```

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                         | Detection    | Effect    |
| :----------- | :------- | :-------------------------------- | :----------- | :-------- |
| `E-CTE-0470` | Error    | Type predicate on incomplete type | Compile-time | Rejection |

---

### 18.6 Type Name and Path Introspection

##### Definition

Type name introspection retrieves the string representation of a type's name and module path.

##### Syntax & Declaration

```cursive
comptime procedure type_name<T>() -> string
comptime procedure module_path<T>() -> string
comptime procedure full_path<T>() -> string
```

##### Static Semantics

**Return Values**

| Procedure     | Returns                                             |
| :------------ | :-------------------------------------------------- |
| `type_name`   | Unqualified type name (e.g., `"Point"`)             |
| `module_path` | Module path without type (e.g., `"geo::shapes"`)    |
| `full_path`   | Fully qualified path (e.g., `"geo::shapes::Point"`) |

For generic types, the name includes type parameters: `"Vec<i32>"`.

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                             | Detection    | Effect    |
| :----------- | :------- | :------------------------------------ | :----------- | :-------- |
| `E-CTE-0480` | Error    | Name introspection on incomplete type | Compile-time | Rejection |

---

## Clause 19: Code Generation

### 19.1 The Ast Type

##### Definition

The **Ast type** is an opaque type representing a syntax tree fragment. Values of type `Ast` are constructed via quote expressions and manipulated via splice expressions.

**Formal Definition**

$$\texttt{Ast} ::= \texttt{Ast::Expr} \mid \texttt{Ast::Stmt} \mid \texttt{Ast::Item} \mid \texttt{Ast::Type} \mid \texttt{Ast::Pattern}$$

Each variant corresponds to a syntactic category:

| Variant        | Syntactic Category            |
| :------------- | :---------------------------- |
| `Ast::Expr`    | Expressions (§7)              |
| `Ast::Stmt`    | Statements (§4.2)             |
| `Ast::Item`    | Items: type decls, procedures |
| `Ast::Type`    | Type expressions (§4.1)       |
| `Ast::Pattern` | Patterns (§11.5)              |

##### Static Semantics

**Subtyping**

Each `Ast` variant is a subtype of `Ast`:

$$\texttt{Ast::Expr} <: \texttt{Ast}$$
$$\texttt{Ast::Stmt} <: \texttt{Ast}$$
$$\texttt{Ast::Item} <: \texttt{Ast}$$
$$\texttt{Ast::Type} <: \texttt{Ast}$$
$$\texttt{Ast::Pattern} <: \texttt{Ast}$$

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                   | Detection    | Effect    |
| :----------- | :------- | :-------------------------- | :----------- | :-------- |
| `E-CTE-0210` | Error    | Ast used in runtime context | Compile-time | Rejection |

---

### 19.2 Quote Expressions

##### Definition

A **quote expression** constructs an `Ast` value from literal Cursive syntax. The quoted code is not evaluated; it is captured as a syntax tree.

##### Syntax & Declaration

**Grammar**

```ebnf
quote_expr ::= "quote" "{" quoted_content "}"
quote_type ::= "quote" "type" "{" type_expr "}"
quote_pattern ::= "quote" "pattern" "{" pattern "}"

quoted_content ::= expr | stmt_list | item_list
```

##### Static Semantics

**Typing Rules**

Expression quote:

$$\frac{
    \textit{content} \text{ parses as expression}
}{
    \Gamma_{ct} \vdash \texttt{quote}\; \{\; \textit{content} \;\} : \texttt{Ast::Expr}
} \quad \text{(T-QuoteExpr)}$$

Statement quote:

$$\frac{
    \textit{content} \text{ parses as statement sequence}
}{
    \Gamma_{ct} \vdash \texttt{quote}\; \{\; \textit{content} \;\} : \texttt{Ast::Stmt}
} \quad \text{(T-QuoteStmt)}$$

Item quote:

$$\frac{
    \textit{content} \text{ parses as item}
}{
    \Gamma_{ct} \vdash \texttt{quote}\; \{\; \textit{content} \;\} : \texttt{Ast::Item}
} \quad \text{(T-QuoteItem)}$$

Type quote:

$$\frac{
    \textit{type\_expr} \text{ parses as type}
}{
    \Gamma_{ct} \vdash \texttt{quote type}\; \{\; \textit{type\_expr} \;\} : \texttt{Ast::Type}
} \quad \text{(T-QuoteType)}$$

Pattern quote:

$$\frac{
    \textit{pattern} \text{ parses as pattern}
}{
    \Gamma_{ct} \vdash \texttt{quote pattern}\; \{\; \textit{pattern} \;\} : \texttt{Ast::Pattern}
} \quad \text{(T-QuotePattern)}$$

**Validation**

Quoted content MUST be syntactically valid. Type checking of quoted content is deferred until emission (§19.5).

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                               | Detection    | Effect    |
| :----------- | :------- | :-------------------------------------- | :----------- | :-------- |
| `E-CTE-0220` | Error    | Quoted content is syntactically invalid | Compile-time | Rejection |
| `E-CTE-0221` | Error    | Quote expression outside comptime       | Compile-time | Rejection |

---

### 19.3 Splice Expressions

##### Definition

A **splice expression** inserts a compile-time value into a quote expression.

##### Syntax & Declaration

**Grammar**

```ebnf
splice_expr  ::= "$(" comptime_expr ")"
splice_ident ::= "$" identifier
```

The shorthand `$ident` is equivalent to `$(ident)`.

##### Static Semantics

**Typing Rules**

Ast splice (hygienic):

$$\frac{
    \Gamma_{ct} \vdash e : \texttt{Ast::}K \quad
    \text{context requires category } K
}{
    \Gamma_{ct} \vdash \texttt{\$(}e\texttt{)} : \text{spliced}
} \quad \text{(T-SpliceAst)}$$

Literal splice:

$$\frac{
    \Gamma_{ct} \vdash e : T \quad
    \text{IsComptimeLiteral}(T)
}{
    \Gamma_{ct} \vdash \texttt{\$(}e\texttt{)} : \text{spliced}
} \quad \text{(T-SpliceLiteral)}$$

Identifier splice (unhygienic):

$$\frac{
    \Gamma_{ct} \vdash e : \texttt{string} \quad
    \text{context is identifier position}
}{
    \Gamma_{ct} \vdash \texttt{\$(}e\texttt{)} : \text{spliced}
} \quad \text{(T-SpliceIdent)}$$

**Comptime Literal Types**

A type $T$ satisfies $\text{IsComptimeLiteral}(T)$ if values of $T$ can be converted to AST literal nodes:

$$\text{IsComptimeLiteral}(T) \iff T \in \{\texttt{bool}, \texttt{i8}, \ldots, \texttt{i128}, \texttt{u8}, \ldots, \texttt{u128}, \texttt{f32}, \texttt{f64}, \texttt{char}, \texttt{string}\}$$

**Splice Context Compatibility**

| Splice Context   | Compatible Splice Types                            |
| :--------------- | :------------------------------------------------- |
| Expression       | `Ast::Expr`, comptime literals                     |
| Statement        | `Ast::Stmt`, `Ast::Expr`                           |
| Type position    | `Ast::Type`, `Type` (via `type_repr_of`)           |
| Pattern position | `Ast::Pattern`                                     |
| Identifier       | `string` (unhygienic), `Ast` with ident (hygienic) |

##### Constraints & Legality

**Identifier Validation**

A string spliced in identifier position MUST conform to the identifier grammar (§2.5):

$$\frac{
    \Gamma_{ct} \vdash e : \texttt{string} \quad
    \neg\text{IsValidIdentifier}(\text{value}(e))
}{
    \text{Emit}(\texttt{E-CTE-0232})
} \quad \text{(Splice-Ident-Check)}$$

**Diagnostic Table**

| Code         | Severity | Condition                                | Detection    | Effect    |
| :----------- | :------- | :--------------------------------------- | :----------- | :-------- |
| `E-CTE-0230` | Error    | Splice type incompatible with context    | Compile-time | Rejection |
| `E-CTE-0231` | Error    | Splice expression not comptime-evaluable | Compile-time | Rejection |
| `E-CTE-0232` | Error    | Invalid identifier string in splice      | Compile-time | Rejection |
| `E-CTE-0233` | Error    | Splice expression outside quote context  | Compile-time | Rejection |

---

### 19.4 Hygiene

##### Definition

**Hygiene** is the property that identifiers introduced by code generation do not accidentally capture or shadow identifiers at the emission site. Cursive implements hygienic macro semantics.

**Formal Definition**

For a quoted expression $q$:

**Capture set**: Identifiers referenced in $q$ but defined outside $q$:
$$\text{Captures}(q) = \text{FreeVars}(q) \cap \text{Dom}(\Gamma_{ct})$$

**Fresh name set**: Identifiers bound within $q$:
$$\text{Fresh}(q) = \text{BoundVars}(q)$$

##### Static Semantics

**Hygiene Preservation Invariant**

For any identifier $x$ appearing in emitted code:

1. If $x \in \text{Captures}(q)$, then $x$ resolves to the same binding it referenced at the quote site.
2. If $x \in \text{Fresh}(q)$, then $x$ does not capture any binding at the emission site.

**Renaming Function**

Fresh names are renamed at emission:

$$\text{Rename}(x) = \text{gensym}(x, \text{EmissionContext})$$

The `gensym` function generates a unique identifier distinct from any identifier in both the quote scope and the emission scope.

**Hygiene Breaking**

To intentionally introduce identifiers into the emission scope, splice a `string` value:

| Splice Input Type | Hygiene Behavior                       |
| :---------------- | :------------------------------------- |
| `string`          | **Unhygienic**—binds in emission scope |
| `Ast`             | **Hygienic**—preserves original scope  |

##### Dynamic Semantics

**Name Generation**

Fresh names are generated deterministically using:

1. Original identifier name
2. Unique counter for emission context
3. Source location of quote

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                          | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------------- | :----------- | :-------- |
| `E-CTE-0240` | Error    | Captured identifier no longer in scope at emission | Compile-time | Rejection |
| `E-CTE-0241` | Error    | Hygiene renaming collision (implementation limit)  | Compile-time | Rejection |

---

### 19.5 Emission

##### Definition

**Emission** is the process of inserting generated AST nodes into the compilation unit. Emission is the terminal operation of code generation.

##### Syntax & Declaration

**Grammar**

```ebnf
emit_stmt ::= emitter_expr "~>" "emit" "(" ast_expr ")"
```

##### Static Semantics

**Typing Rule**

$$\frac{
    \Gamma_{ct} \vdash \textit{emitter} : \texttt{TypeEmitter} \quad
    \Gamma_{ct} \vdash \textit{ast} : \texttt{Ast::Item}
}{
    \Gamma_{ct} \vdash \textit{emitter}\texttt{\textasciitilde>emit}(\textit{ast}) : ()
} \quad \text{(T-Emit)}$$

**Emission Target**

Emitted items are inserted at module scope of the current compilation unit. The emitter context determines the valid emission targets.

**Post-Emission Type Checking**

After the Metaprogramming Phase completes, the implementation MUST perform full type checking on the expanded AST. Type errors in emitted code MUST be reported with a diagnostic trace including:

1. The location of the emit call
2. The derive target or comptime block that generated the code
3. The specific type error in the emitted code

##### Dynamic Semantics

**Emission Order**

Within a single Metaprogramming Phase:

1. All derive targets execute in dependency order (§19.4).
2. Explicit `[[emit]]` blocks execute in declaration order.
3. Emitted items become visible to subsequent emissions within the same phase.

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                           | Detection    | Effect    |
| :----------- | :------- | :---------------------------------- | :----------- | :-------- |
| `E-CTE-0250` | Error    | Emit without TypeEmitter capability | Compile-time | Rejection |
| `E-CTE-0251` | Error    | Emitted AST is not an item          | Compile-time | Rejection |
| `E-CTE-0252` | Error    | Emitted AST is ill-formed           | Compile-time | Rejection |
| `E-CTE-0253` | Error    | Type error in emitted code          | Compile-time | Rejection |

---

### 19.6 Type Representation

##### Definition

**Type representation** converts a `Type` metatype value to an `Ast::Type` node suitable for splicing into generated code.

##### Syntax & Declaration


##### Static Semantics

**Typing Rule**

$$\frac{
    \Gamma_{ct} \vdash t : \texttt{Type}
}{
    \Gamma_{ct} \vdash \texttt{type\_repr\_of}(t) : \texttt{Ast::Type}
} \quad \text{(T-TypeRepr)}$$

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                           | Detection    | Effect    |
| :----------- | :------- | :---------------------------------- | :----------- | :-------- |
| `E-CTE-0260` | Error    | type_repr_of called on invalid Type | Compile-time | Rejection |

---

### 19.7 Write Keys

##### Definition

A **write key** provides exclusive write access to a (type, form) pair during derive execution.

##### Syntax & Declaration

```cursive
record WriteKey {
    target_type: Type,
    form: Type,
}
```

**Acquisition**

```cursive
comptime procedure acquire_write_key(emitter: TypeEmitter, target: Type, form: Type) -> WriteKey
```

##### Static Semantics

**Single-Writer Property**

For each (type, form) pair, at most one write key may be held at any time during the Metaprogramming Phase:

$$\forall (T, F).\; |\{k : \texttt{WriteKey} \mid k.\text{target\_type} = T \land k.\text{form} = F\}| \leq 1$$

**Typing Rule**

$$\frac{
    \Gamma_{ct} \vdash \textit{emitter} : \texttt{TypeEmitter} \quad
    \Gamma_{ct} \vdash T : \texttt{Type} \quad
    \Gamma_{ct} \vdash F : \texttt{Type} \quad
    \text{is\_form}(F)
}{
    \Gamma_{ct} \vdash \textit{emitter}\texttt{\textasciitilde>acquire\_write\_key}(T, F) : \texttt{WriteKey}
} \quad \text{(T-WriteKey)}$$

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                    | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------- | :----------- | :-------- |
| `E-CTE-0270` | Error    | Write key already held for (type, form) pair | Compile-time | Rejection |
| `E-CTE-0271` | Error    | Duplicate form implementation emitted        | Compile-time | Rejection |

---

## Clause 20: Derivation

---

### 20.1 Derive Attributes

##### Definition

A **derive attribute** requests automatic generation of form implementations for an annotated type declaration. The derive attribute invokes one or more derive targets (§19.2) during the Metaprogramming Phase.

##### Syntax & Declaration

**Grammar**

```ebnf
derive_attr ::= "[[" "derive" "(" derive_target_list ")" "]]"
derive_target_list ::= derive_target ("," derive_target)*
derive_target ::= identifier
```

**Placement**

A derive attribute MUST immediately precede a type declaration (`record`, `enum`, or `modal`).

##### Static Semantics

**Typing Rule**

$$\frac{
    \texttt{[[derive(}D_1, \ldots, D_n\texttt{)]]} \text{ precedes type } T \quad
    \forall i.\; D_i \text{ is a valid derive target}
}{
    \text{schedule derive execution for } (T, D_1), \ldots, (T, D_n)
} \quad \text{(T-DeriveAttr)}$$

**Validation**

Each identifier in the derive target list MUST resolve to a derive target declaration (§19.2) or a standard derive target (§19.6).

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                | Detection    | Effect    |
| :----------- | :------- | :--------------------------------------- | :----------- | :-------- |
| `E-CTE-0310` | Error    | Unknown derive target name               | Compile-time | Rejection |
| `E-CTE-0311` | Error    | Derive attribute on non-type declaration | Compile-time | Rejection |
| `E-CTE-0312` | Error    | Duplicate derive target in attribute     | Compile-time | Rejection |

---

### 20.2 Derive Target Declarations

##### Definition

A **derive target declaration** defines a named code generator that produces form implementations for annotated types. Derive targets are comptime procedures with access to the `TypeEmitter` capability.

##### Syntax & Declaration

**Grammar**

```ebnf
derive_target_decl ::= "derive" "target" identifier 
                       "(" "target" ":" "Type" ")" 
                       derive_contract?
                       block

derive_contract ::= "|=" contract_clause ("," contract_clause)*
contract_clause ::= "emits" class_ref
                  | "requires" class_ref
class_ref ::= identifier
```

##### Static Semantics

**Typing Rule**

$$\frac{
    \Gamma_{ct}, \textit{target}: \texttt{Type}, \textit{emitter}: \texttt{TypeEmitter} \vdash \textit{body} : ()
}{
    \Gamma \vdash \texttt{derive target}\; N\;(\texttt{target}: \texttt{Type})\; \{\; \textit{body} \;\} : \text{DeriveTarget}
} \quad \text{(T-DeriveTarget)}$$

**Contract Semantics**

| Clause       | Meaning                                              |
| :----------- | :--------------------------------------------------- |
| `emits F`    | The derive target emits an implementation of class F |
| `requires F` | The target type must implement class F before derive |

**Implicit Bindings**

Within a derive target body, the following bindings are implicitly available:

| Identifier    | Type                  | Description                       |
| :------------ | :-------------------- | :-------------------------------- |
| `target`      | `Type`                | The type being derived            |
| `emitter`     | `TypeEmitter`         | Capability for code emission      |
| `introspect`  | `Introspect`          | Capability for type introspection |
| `diagnostics` | `ComptimeDiagnostics` | Capability for error reporting    |

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                  | Detection    | Effect    |
| :----------- | :------- | :----------------------------------------- | :----------- | :-------- |
| `E-CTE-0320` | Error    | Derive target body violates comptime rules | Compile-time | Rejection |
| `E-CTE-0321` | Error    | Contract references unknown form           | Compile-time | Rejection |
| `E-CTE-0322` | Error    | Derive target has invalid signature        | Compile-time | Rejection |

---

### 20.3 Derive Contracts

##### Definition

A **derive contract** specifies the classes a derive target emits and the classes it requires as preconditions.

**Formal Definition**

A derive contract is a pair $(E, R)$ where:
- $E \subseteq \text{Classes}$ is the set of classes emitted
- $R \subseteq \text{Classes}$ is the set of classes required

##### Static Semantics

**Emits Clause**

For each `emits F` clause, the derive target MUST emit an AST node that, when type-checked, results in the target type implementing form $F$:

$$\texttt{emits } F \implies T <: F \text{ after derive execution}$$

**Requires Clause**

For each `requires F` clause, the target type MUST implement form $F$ before derive target execution:

$$\texttt{requires } F \implies T <: F \text{ before derive execution}$$

**Contract Verification**

After derive execution, the implementation MUST verify:

1. All `emits` forms are implemented by the target type.
2. No form is emitted that is not declared in an `emits` clause.

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                    | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------- | :----------- | :-------- |
| `E-CTE-0330` | Error    | Required form not implemented by target type | Compile-time | Rejection |
| `E-CTE-0331` | Error    | Emits clause not satisfied after execution   | Compile-time | Rejection |
| `E-CTE-0332` | Error    | Undeclared form emitted                      | Compile-time | Rejection |

---

### 20.4 Derive Execution Model

##### Definition

**Derive execution** is the process by which derive targets are invoked during the Metaprogramming Phase. Execution follows a dependency order determined by `requires` clauses.

**Formal Definition**

Let $G = (V, E)$ be the derive dependency graph where:
- $V = \{(T, D) \mid T \text{ has } \texttt{[[derive(}D\texttt{)]]}\}$
- $E = \{((T, D_1), (T, D_2)) \mid D_1 \texttt{ requires } F \land D_2 \texttt{ emits } F\}$

Derive execution proceeds in topological order of $G$.

##### Dynamic Semantics

**Execution Algorithm**

1. Construct derive dependency graph $G$ for all derive attributes in the compilation unit.
2. Detect cycles in $G$. If cycles exist, emit `E-CTE-0340` and halt.
3. Compute topological order $\pi$ of $G$.
4. For each $(T, D)$ in order $\pi$:
   a. Verify all `requires` forms are implemented.
   b. Execute derive target $D$ with `target` bound to $T$.
   c. Verify all `emits` forms are now implemented.

**Parallel Execution**

Derive targets with no dependencies between them MAY execute in parallel. The implementation MUST ensure:

1. Write key acquisition is atomic.
2. Emission order within a type is deterministic.

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                | Detection    | Effect    |
| :----------- | :------- | :--------------------------------------- | :----------- | :-------- |
| `E-CTE-0340` | Error    | Cyclic derive dependency                 | Compile-time | Rejection |
| `E-CTE-0341` | Error    | Derive target execution panics           | Compile-time | Rejection |
| `E-CTE-0342` | Error    | Derive execution exceeds resource limits | Compile-time | Rejection |

---

### 20.5 Generated Type Declarations

##### Definition

Derive targets emit **complete type declarations** with form implementations inline. The generated declaration replaces the original annotated declaration.

**Formal Definition**

Given input:
```cursive
[[derive(F)]]
record T {
    fields...
}
```

A derive target emitting form $F$ produces:
```cursive
record T <: F {
    fields...,
    // form procedures
}
```

##### Static Semantics

**Output Structure**

The emitted AST MUST be a complete type declaration that:

1. Preserves all original fields in declaration order.
2. Includes the subtyping clause `<: F` for each emitted class.
3. Includes all procedure implementations required by the class.

**Orphan Rule Compliance**

Derive targets MUST NOT emit standalone class implementations. Per §9.3, class implementations MUST occur at the type's definition site. The derive system enforces this by:

1. Requiring derive attributes on type declarations.
2. Emitting complete type declarations, not separate implementation blocks.

##### Constraints & Legality

**Negative Constraints**

The following emission patterns are prohibited:

| Pattern                           | Violation             |
| :-------------------------------- | :-------------------- |
| Emit implementation without type  | Orphan rule (§9.3)    |
| Emit type in different module     | Module boundary       |
| Emit for type without derive attr | Unauthorized emission |

**Diagnostic Table**

| Code         | Severity | Condition                                   | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------ | :----------- | :-------- |
| `E-CTE-0350` | Error    | Emitted declaration missing original fields | Compile-time | Rejection |
| `E-CTE-0351` | Error    | Emitted declaration has wrong type name     | Compile-time | Rejection |
| `E-CTE-0352` | Error    | Form procedure missing from emission        | Compile-time | Rejection |

---

### 20.6 Standard Derive Targets

##### Definition

The **standard derive targets** are derive targets provided by the language implementation for common forms.

**Formal Definition**

| Target        | Contract                      | Applicable Categories |
| :------------ | :---------------------------- | :-------------------- |
| `Debug`       | `\|= emits Debug`             | Record, Enum          |
| `Clone`       | `\|= emits Clone`             | Record, Enum          |
| `Eq`          | `\|= emits Eq`                | Record, Enum          |
| `Hash`        | `\|= emits Hash, requires Eq` | Record, Enum          |
| `Default`     | `\|= emits Default`           | Record                |
| `Serialize`   | `\|= emits Serialize`         | Record, Enum          |
| `Deserialize` | `\|= emits Deserialize`       | Record, Enum          |

#### 20.6.1 Debug

##### Definition

The `Debug` derive target generates a human-readable string representation procedure.

**Generated Procedure**

```cursive
procedure debug(~) -> string
```

**Generation Rules**

For records: Concatenate type name, `{`, field name-value pairs separated by `, `, and `}`.

For enums: Concatenate variant name and, if present, `(` payload values `)`.

**Field Requirements**

All fields MUST implement `Debug`. If any field does not implement `Debug`, emit `E-CTE-0360`.

#### 20.6.2 Clone

##### Definition

The `Clone` derive target generates a deep copy procedure.

**Generated Procedure**

```cursive
procedure clone(~) -> Self
```

**Generation Rules**

For records: Construct new instance with each field cloned.

For enums: Match on variant, clone payload, construct same variant.

**Field Requirements**

All fields MUST implement `Clone`. If any field does not implement `Clone`, emit `E-CTE-0361`.

#### 20.6.3 Eq

##### Definition

The `Eq` derive target generates structural equality comparison.

**Generated Procedure**

```cursive
procedure eq(~, other: ~Self) -> bool
```

**Generation Rules**

For records: Return `true` iff all corresponding fields are equal.

For enums: Return `true` iff variants match and all payload fields are equal.

**Field Requirements**

All fields MUST implement `Eq`. If any field does not implement `Eq`, emit `E-CTE-0362`.

#### 20.6.4 Hash

##### Definition

The `Hash` derive target generates hash computation. Requires `Eq`.

**Generated Procedure**

```cursive
procedure hash(~, hasher: &unique Hasher)
```

**Generation Rules**

For records: Hash each field in declaration order.

For enums: Hash discriminant, then hash payload fields.

**Field Requirements**

All fields MUST implement `Hash`. If any field does not implement `Hash`, emit `E-CTE-0363`.

#### 20.6.5 Default

##### Definition

The `Default` derive target generates default value construction. Applicable to records only.

**Generated Procedure**

```cursive
procedure default() -> Self
```

**Generation Rules**

Construct instance with each field set to its default value.

**Field Requirements**

All fields MUST implement `Default`. If any field does not implement `Default`, emit `E-CTE-0364`.

#### 20.6.6 Serialize and Deserialize

##### Definition

The `Serialize` and `Deserialize` derive targets generate serialization procedures.

**Generated Procedures**

```cursive
// Serialize
procedure serialize(~, serializer: &unique Serializer) -> Result<(), SerializeError>

// Deserialize  
procedure deserialize(deserializer: &unique Deserializer) -> Result<Self, DeserializeError>
```

**Generation Rules**

Implementation-defined serialization format. The implementation MUST document the format.

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                            | Detection    | Effect    |
| :----------- | :------- | :----------------------------------- | :----------- | :-------- |
| `E-CTE-0360` | Error    | Field does not implement Debug       | Compile-time | Rejection |
| `E-CTE-0361` | Error    | Field does not implement Clone       | Compile-time | Rejection |
| `E-CTE-0362` | Error    | Field does not implement Eq          | Compile-time | Rejection |
| `E-CTE-0363` | Error    | Field does not implement Hash        | Compile-time | Rejection |
| `E-CTE-0364` | Error    | Field does not implement Default     | Compile-time | Rejection |
| `E-CTE-0365` | Error    | Default derive on non-record type    | Compile-time | Rejection |
| `E-CTE-0366` | Error    | Field does not implement Serialize   | Compile-time | Rejection |
| `E-CTE-0367` | Error    | Field does not implement Deserialize | Compile-time | Rejection |

---

### 20.7 Type Category Dispatch

##### Definition

**Type category dispatch** enables derive targets to generate different code based on the target type's category (record, enum, modal).

##### Syntax & Declaration

**Grammar**

```ebnf
category_match ::= "match" "introspect" "~>" "category" "::<" type_var ">" "()" "{" category_arm+ "}"
category_arm ::= "TypeCategory" "::" category_name "=>" block ","
category_name ::= "Record" | "Enum" | "Modal" | "Primitive" | "Tuple" | "Array" | "Procedure" | "Reference" | "Generic"
```

##### Static Semantics

**Exhaustiveness**

A category match in a derive target need not be exhaustive. Unmatched categories result in `E-CTE-0370`.

**Typing Rule**

$$\frac{
    \Gamma_{ct} \vdash T : \texttt{Type} \quad
    \forall \text{arm } C_i \Rightarrow e_i.\; \Gamma_{ct} \vdash e_i : R
}{
    \Gamma_{ct} \vdash \texttt{match introspect\textasciitilde>category::<}T\texttt{>()} \{ \ldots \} : R
} \quad \text{(T-CategoryMatch)}$$

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                     | Detection    | Effect    |
| :----------- | :------- | :-------------------------------------------- | :----------- | :-------- |
| `E-CTE-0370` | Error    | Derive target not applicable to type category | Compile-time | Rejection |

---

# Part 6: Interoperability and ABI

## Clause 21: Foreign Function Interface

### 21.1 The FfiSafe Class

##### Definition

The `FfiSafe` class classifies types whose runtime representation is compatible with a stable C-style ABI. Only `FfiSafe` types may appear in:

* Parameter and return types of foreign procedures
* Types of foreign globals
* Parameter and return types of exported Cursive procedures

##### Syntax & Declaration

```cursive
/// Types that can safely cross the FFI boundary.
/// Provides compile-time layout introspection for ABI compatibility verification.
class FfiSafe {
    /// The C-compatible size of this type in bytes.
    comptime procedure c_size() -> usize;
    
    /// The C-compatible alignment of this type in bytes.
    comptime procedure c_alignment() -> usize;
    
    /// Verifies this type's layout matches expectations.
    /// Returns true if layout matches, false otherwise.
    comptime procedure verify_layout(expected_size: usize, expected_align: usize) -> bool {
        Self::c_size() == expected_size and Self::c_alignment() == expected_align
    }
}
```

##### Static Semantics

**Well-Formedness Constraint (WF-FfiSafe)**

A type `T` is FFI-safe if and only if:

1. `T` declares `<: FfiSafe` and provides implementations of `c_size()` and `c_alignment()`
2. `T` has a complete, fixed layout (`HasLayout(T)` holds per §17.6.4)
3. `T` does not contain any prohibited types (see below)

$$\frac{
  T <: \texttt{FfiSafe} \quad
  \text{HasLayout}(T) \quad
  \neg\text{ContainsProhibited}(T)
}{
  \text{FfiSafe}(T)
} \quad \text{(WF-FfiSafe)}$$

**Automatic Implementations**

The following types have intrinsic `FfiSafe` implementations:

| Type Category            | Examples                                   | Notes                                          |
| :----------------------- | :----------------------------------------- | :--------------------------------------------- |
| Signed integers          | `i8`, `i16`, `i32`, `i64`, `i128`, `isize` | Fixed-width, two's complement                  |
| Unsigned integers        | `u8`, `u16`, `u32`, `u64`, `u128`, `usize` | Fixed-width                                    |
| Floating-point           | `f16`, `f32`, `f64`                        | IEEE 754 representation                        |
| Raw pointers             | `*imm T`, `*mut T`                         | For any `T` (inner type need not be `FfiSafe`) |
| Fixed-size arrays        | `[T; N]`                                   | When `T <: FfiSafe`                            |
| Sparse function pointers | `(T₁, ..., Tₙ) -> R`                       | When all `Tᵢ` and `R` are `FfiSafe`            |

The primitive types `i8`, `i16`, `i32`, `i64`, `i128`, `isize`, `u8`, `u16`, `u32`, `u64`, `u128`, `usize`, `f16`, `f32`, `f64` implement `FfiSafe` with size and alignment matching the target C ABI.

**User-Defined FFI-Safe Types**

A user-defined type implements `FfiSafe` by:

1. Applying the `[[layout(C)]]` attribute
2. Declaring `<: FfiSafe` in the type definition
3. Providing `c_size()` and `c_alignment()` implementations
4. Ensuring all fields are themselves `FfiSafe`

```cursive
[[layout(C)]]
record Point <: FfiSafe {
    x: f64,
    y: f64,
    
    comptime procedure c_size() -> usize { 16 }
    comptime procedure c_alignment() -> usize { 8 }
}

[[layout(C)]]
record Rect <: FfiSafe {
    origin: Point,
    size: Point,
    
    comptime procedure c_size() -> usize { 32 }
    comptime procedure c_alignment() -> usize { 8 }
}
```

**Derive Support**

The `[[derive(FfiSafe)]]` attribute generates the `FfiSafe` implementation automatically:

```cursive
[[layout(C)]]
[[derive(FfiSafe)]]
record Point {
    x: f64,
    y: f64,
}
```

The derive macro:

1. Verifies `[[layout(C)]]` is present (else emits `E-SYS-3301`)
2. Verifies all fields implement `FfiSafe` (else emits `E-SYS-3302`)
3. Verifies the type contains no prohibited types (else emits `E-SYS-3303`)
4. Generates `c_size()` and `c_alignment()` using introspection

**Generic FFI-Safe Types**

For a generic type to be FFI-safe, type parameters appearing in the layout MUST be constrained to `FfiSafe`:

```cursive
[[layout(C)]]
record Pair<T <: FfiSafe> <: FfiSafe {
    first: T,
    second: T,
    
    comptime procedure c_size() -> usize { 
        introspect~>size_of::<Pair<T>>() 
    }
    comptime procedure c_alignment() -> usize { 
        introspect~>align_of::<Pair<T>>() 
    }
}
```

**Prohibited Types**

The following types MUST NOT implement `FfiSafe`:

| Type Category                |
| :--------------------------- |
| `bool`                       |
| Modal types                  |
| `Ptr<T>` (safe pointer)      |
| Dyn types (`dyn Class`)      |
| Capability classes           |
| `Context`                    |
| Types containing dynes       |
| Opaque classes (`opaque`)    |
| Tuples                       |
| Slices (`[T]`)               |
| String types                 |
| Closure types (`\|T\| -> U`) |

**Diagnostic for Prohibited Types**

```
E-SYS-3303: Type `Connection` cannot implement `FfiSafe`: contains modal state
  --> src/network.cur:42:1
   |
42 | record Connection <: FfiSafe {
   | ^^^^^^^^^^^^^^^^^^^^^^^^^^^^
   |
   = note: modal types have runtime discriminants incompatible with C ABI
   = help: use a raw pointer or handle type for FFI
```

**RAII Types and FfiSafe**

A type implementing both `Drop` and `FfiSafe` requires the `[[ffi_pass_by_value]]` attribute if it will be passed by value across the FFI boundary:

```cursive
[[layout(C)]]
[[ffi_pass_by_value]]
record CString <: FfiSafe, Drop {
    ptr: *mut c_char,
    len: usize,
    cap: usize,
    
    comptime procedure c_size() -> usize { introspect~>size_of::<CString>() }
    comptime procedure c_alignment() -> usize { introspect~>align_of::<CString>() }
    
    procedure drop(~!) {
        if self.ptr != null {
            unsafe { c_free(self.ptr as *mut opaque c_void); }
        }
    }
}
```

Without `[[ffi_pass_by_value]]`, passing a `Drop + FfiSafe` type by value in FFI emits `E-SYS-3306`.

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                                 | Detection    | Effect    |
| :----------- | :------- | :-------------------------------------------------------- | :----------- | :-------- |
| `E-SYS-3301` | Error    | `FfiSafe` on type without `[[layout(C)]]`                 | Compile-time | Rejection |
| `E-SYS-3302` | Error    | `FfiSafe` type has non-FFI-safe field                     | Compile-time | Rejection |
| `E-SYS-3303` | Error    | `FfiSafe` on prohibited type category                     | Compile-time | Rejection |
| `E-SYS-3304` | Error    | Generic `FfiSafe` type with unconstrained parameter       | Compile-time | Rejection |
| `E-SYS-3305` | Error    | `FfiSafe` type has field with incomplete type             | Compile-time | Rejection |
| `E-SYS-3306` | Error    | `Drop + FfiSafe` by-value without `[[ffi_pass_by_value]]` | Compile-time | Rejection |

---

### 21.2 Foreign Procedure Declarations

##### Definition

A **foreign procedure declaration** introduces a procedure whose implementation is provided by foreign (non-Cursive) code. Foreign procedures are declared in `extern` blocks and MUST only be called within `unsafe` blocks.

##### Syntax & Declaration

**Grammar**

```ebnf
extern_block        ::= "extern" string_literal "{" extern_item* "}"

extern_item         ::= foreign_procedure | foreign_global

foreign_procedure   ::= [visibility] "procedure" identifier 
                        "(" [param_list] ")" ["->" type]
                        [variadic_spec] [foreign_contract] ";"

variadic_spec       ::= "..." | "...," type

param_list          ::= param ("," param)*

param               ::= identifier ":" type

foreign_contract    ::= "|=" foreign_assumes

foreign_assumes     ::= "@foreign_assumes" "(" predicate_list ")"

predicate_list      ::= predicate ("," predicate)*
```

```cursive
extern "C" {
    procedure malloc(size: usize) -> *mut opaque c_void;
    procedure free(ptr: *mut opaque c_void);
    procedure memcpy(
        dest: *mut opaque c_void,
        src: *imm opaque c_void,
        n: usize
    ) -> *mut opaque c_void;
    
    procedure printf(format: *imm c_char, ...) -> c_int;
    
    procedure read(fd: c_int, buf: *mut opaque c_void, count: usize) -> isize
        |= @foreign_assumes(fd >= 0, buf != null);
}
```

##### Static Semantics

**ABI String**

The string literal following `extern` specifies the calling convention:

| ABI String     | Meaning                                  |
| :------------- | :--------------------------------------- |
| `"C"`          | Platform's standard C calling convention |
| `"C-unwind"`   | C convention with unwinding support      |
| `"system"`     | Platform's system call convention        |
| `"stdcall"`    | x86 stdcall (Windows)                    |
| `"fastcall"`   | x86 fastcall                             |
| `"vectorcall"` | x86-64 vectorcall (Windows)              |

The default is `"C"` if omitted.

**Type Restrictions**

All parameter types and return types in foreign procedure declarations MUST be `FfiSafe`:

$$\frac{
  \texttt{extern}\ \textit{abi}\ \{\ \texttt{procedure}\ f(p_1: T_1, \ldots, p_n: T_n) \to R;\ \}
}{
  \forall i.\ T_i <: \texttt{FfiSafe} \quad R <: \texttt{FfiSafe}
} \quad \text{(T-Extern-Proc)}$$

Violation emits `E-SYS-3310`.

**Variadic Procedures**

C-style variadic procedures are declared with `...`:

```cursive
extern "C" {
    // Untyped variadic (like C's printf)
    procedure printf(format: *imm c_char, ...) -> c_int;
    
    // Typed variadic (all varargs must be same type)
    procedure sum_ints(count: c_int, ..., c_int) -> c_int;
}
```

Variadic arguments undergo **default argument promotion** per C semantics:

| Original Type            | Promoted To |
| :----------------------- | :---------- |
| `i8`, `i16`, `u8`, `u16` | `c_int`     |
| `f32`                    | `f64`       |
| All others               | Unchanged   |

**Call-Site Semantics**

Invoking a foreign procedure:

1. MUST occur within an `unsafe` block
2. Evaluates arguments left-to-right
3. Performs any necessary type promotions for variadic arguments
4. Transfers control to foreign code
5. Returns result (if any) upon foreign code completion

```cursive

```

**Link Attributes**

**`[[link_name]]`** — Override symbol name:

```cursive
extern "C" {
    [[link_name("__real_malloc")]]
    procedure malloc(size: usize) -> *mut opaque c_void;
}
```

**`[[link]]`** — Specify library linkage:

```cursive
[[link(name: "ssl", kind: "dylib")]]
extern "C" {
    procedure SSL_new(ctx: *mut SSL_CTX) -> *mut SSL;
}
```

Link kinds:

| Kind          | Meaning                   |
| :------------ | :------------------------ |
| `"dylib"`     | Dynamic library (default) |
| `"static"`    | Static library            |
| `"framework"` | macOS framework           |
| `"raw-dylib"` | Windows delay-load        |

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                | Detection    | Effect    |
| :----------- | :------- | :--------------------------------------- | :----------- | :-------- |
| `E-SYS-3310` | Error    | Non-FfiSafe type in extern signature     | Compile-time | Rejection |
| `E-SYS-3311` | Error    | Unknown ABI string                       | Compile-time | Rejection |
| `E-SYS-3312` | Error    | Variadic without fixed parameters        | Compile-time | Rejection |
| `E-SYS-3313` | Error    | Body provided for foreign procedure      | Compile-time | Rejection |
| `E-SYS-3320` | Error    | Extern call outside `unsafe` block       | Compile-time | Rejection |
| `E-SYS-3321` | Error    | Non-promotable type in variadic position | Compile-time | Rejection |

---

### 21.3 Foreign Global Declarations

##### Definition

A **foreign global declaration** introduces a global variable whose storage is provided by foreign code.

##### Syntax & Declaration

**Grammar**

```ebnf
foreign_global      ::= [visibility] ["mut"] identifier ":" type ";"
```

```cursive
extern "C" {
    errno: c_int;                      // immutable binding to foreign global
    mut environ: *mut *mut c_char;     // mutable binding
}
```

##### Static Semantics

**Type Restriction**

The declared type MUST be `FfiSafe`. Violation emits `E-SYS-3330`.

**Mutability**

- Without `mut`: Read-only access; writes are ill-formed
- With `mut`: Read and write access permitted

**Access Semantics**

All access to foreign globals MUST occur within `unsafe` blocks:

```cursive
procedure get_errno() -> c_int {
    unsafe { errno }  // ✓
}

procedure clear_errno() {
    // errno = 0;  // ✗ not declared mut
}
```

##### Constraints & Legality

| Code         | Severity | Condition                            | Detection    | Effect    |
| :----------- | :------- | :----------------------------------- | :----------- | :-------- |
| `E-SYS-3330` | Error    | Non-FfiSafe type for foreign global  | Compile-time | Rejection |
| `E-SYS-3331` | Error    | Write to non-mut foreign global      | Compile-time | Rejection |
| `E-SYS-3332` | Error    | Foreign global access outside unsafe | Compile-time | Rejection |

---

### 21.4 Exported Procedures

##### Definition

An **exported procedure** is a Cursive procedure made callable from foreign code. Exported procedures use the `[[export]]` attribute and have restricted signatures.

##### Syntax & Declaration

**Grammar**

```ebnf
export_attr         ::= "[[" "export" "(" string_literal ")" "]]"
                      | "[[" "export" "(" string_literal "," export_opts ")" "]]"

export_opts         ::= export_opt ("," export_opt)*

export_opt          ::= "link_name" ":" string_literal
```

```cursive
[[export("C")]]
public procedure add(a: c_int, b: c_int) -> c_int {
    a + b
}

[[export("C", link_name: "mylib_init")]]
public procedure initialize(config: *imm Config) -> c_int {
    // initialization logic
    0
}
```

##### Static Semantics

**Visibility Requirement**

Exported procedures MUST be `public`. Non-public exports emit `E-SYS-3350`.

**Signature Restrictions**

Exported procedure signatures are subject to strict constraints:

1. All parameter types MUST be `FfiSafe`
2. Return type MUST be `FfiSafe` (or unit `()`)
3. Parameters MUST NOT be dyn types or capability classes
4. Parameters MUST NOT contain `Context` or dyn-containing types
5. The procedure MUST NOT have capability dyn parameters

```cursive
// ✓ Valid export
[[export("C")]]
public procedure process(data: *imm u8, len: usize) -> c_int {
    // ...
}

// ✗ Invalid: dyn parameter
[[export("C")]]
public procedure bad_export(fs: dyn FileSystem) -> c_int {  // E-SYS-3351
    // ...
}

// ✗ Invalid: Context parameter
[[export("C")]]
public procedure also_bad(ctx: Context) -> c_int {  // E-SYS-3353
    // ...
}
```

**Calling Convention**

The ABI string specifies the calling convention, identical to `extern` blocks.

**Symbol Name**

By default, the exported symbol name matches the procedure name. Use `link_name` to override:

```cursive
[[export("C", link_name: "crs_buffer_new")]]
public procedure buffer_new(cap: usize) -> *mut Buffer {
    // ...
}
```

**No Mangling**

Exported procedures use unmangled names suitable for C linkage. Name conflicts with other exports emit `E-SYS-3354`.

##### Constraints & Legality

| Code         | Severity | Condition                                | Detection    | Effect    |
| :----------- | :------- | :--------------------------------------- | :----------- | :-------- |
| `E-SYS-3350` | Error    | `[[export]]` on non-public procedure     | Compile-time | Rejection |
| `E-SYS-3351` | Error    | Dyn/capability parameter in export       | Compile-time | Rejection |
| `E-SYS-3352` | Error    | Non-FfiSafe type in export signature     | Compile-time | Rejection |
| `E-SYS-3353` | Error    | Context or dyn-containing type in export | Compile-time | Rejection |
| `E-SYS-3354` | Error    | Duplicate export symbol name             | Link-time    | Rejection |

---

### 21.5 Capability Isolation Patterns

##### Definition

Capability isolation patterns ensure that FFI interactions do not grant foreign code unintended authority over Cursive capabilities.

##### Static Semantics

1. **Capability Blindness Rule:** Foreign code MUST NOT receive or return capability dynes, inspect capability representations, or exercise capability-mediated authority. This is enforced by prohibiting capability-bearing dyn types in all FFI signatures (§21.3–§21.5).
2. **Runtime Handle Pattern:** When foreign code needs to reference Cursive resources, the implementation MUST expose opaque, FfiSafe handle types whose fields do not contain capabilities. Handle registration, lookup, and revocation MUST be performed inside Cursive code; invalid handles MUST be diagnosed or signaled as errors.
3. **Callback Context Pattern:** When foreign code calls back into Cursive, capability-bearing data MUST be wrapped in a heap-allocated context owned by Cursive. Foreign code receives only an opaque pointer; destruction of the context MUST be explicit and exactly once.
4. **Region Pointer Escape Detection:** Pointers to region-local storage MUST NOT be passed across the FFI boundary. Escape analysis MUST reject any flow of a region-local pointer into foreign code.

##### Constraints & Legality

**Diagnostic Table**

| Code         | Severity | Condition                                          | Detection    | Effect    |
| :----------- | :------- | :------------------------------------------------- | :----------- | :-------- |
| `E-SYS-3351` | Error    | Capability-bearing dyn or Context in FFI signature | Compile-time | Rejection |
| `E-SYS-3353` | Error    | Context or dyn-containing type in export           | Compile-time | Rejection |
| `E-SYS-3360` | Error    | Region-local pointer escapes via FFI               | Compile-time | Rejection |


### 21.6 Standard FFI Types

Standard library FFI helper types (string wrappers, pointer wrappers, buffers, booleans, file descriptors, opaque void, error enums, modal foreign resources, variadic helpers, and wide character aliases) are specified in **Appendix H (Core Library Specification)** and are not repeated here.

----

### 21.7 Foreign Contracts

##### Definition

**Foreign contracts** extend the contract system (§10) to FFI boundaries. They specify preconditions that callers must satisfy before invoking foreign procedures.

##### Syntax & Declaration

**Grammar**

```ebnf
foreign_contract    ::= "|=" "@foreign_assumes" "(" predicate_list ")"

predicate_list      ::= predicate ("," predicate)*

predicate           ::= comparison_expr | null_check | range_check

comparison_expr     ::= expr comparison_op expr

null_check          ::= expr "!=" "null" | expr "==" "null"

range_check         ::= expr "in" range_expr
```

```cursive
extern "C" {
    procedure write(fd: c_int, buf: *imm u8, count: usize) -> isize
        |= @foreign_assumes(fd >= 0, buf != null, count > 0);
    
    procedure read(fd: c_int, buf: *mut u8, count: usize) -> isize
        |= @foreign_assumes(fd >= 0, buf != null);
    
    procedure memcpy(dest: *mut u8, src: *imm u8, n: usize) -> *mut u8
        |= @foreign_assumes(dest != null, src != null);
}
```

##### Static Semantics

**Predicate Context**

Predicates in `@foreign_assumes` may reference:

- Parameter names from the procedure signature
- Literal constants
- Pure functions and operators
- Fields of parameter values (for record types)

Predicates MUST NOT reference:

- Global mutable state
- Values not in scope at the call site
- Effectful operations

**Verification Modes**

Foreign contracts integrate with the contract verification modes defined in §11.4:

| Mode                   | Behavior                                                                  |
| :--------------------- | :------------------------------------------------------------------------ |
| `[[static]]` (default) | Caller must prove predicates at compile time. Failure emits `E-SEM-2850`. |
| `[[dynamic]]`          | Runtime checks inserted before `unsafe` call. Violation causes panic.     |
| `[[assume]]`           | Predicates assumed true. No checks. For optimization only.                |

**Static Verification**

```cursive
extern "C" {
    procedure process(buf: *imm u8, len: usize) -> c_int
        |= @foreign_assumes(buf != null, len > 0);
}

procedure bad_example() {
    unsafe {
        process(null, 0);  // E-SEM-2850: Cannot prove `buf != null`
    }
}
```

**Dynamic Verification**

```cursive
[[dynamic]]
extern "C" {
    procedure risky_call(ptr: *mut u8) -> c_int
        |= @foreign_assumes(ptr != null);
}

```

#### 20.8.1 Foreign Postconditions

##### Definition

Foreign postconditions specify conditions that foreign code guarantees upon successful return. Unlike `@foreign_assumes` (which Cursive verifies at call sites), `@foreign_ensures` predicates are **trusted assertions** about foreign behavior that enable downstream verification.

##### Syntax & Declaration

**Grammar Extension**

```ebnf
foreign_contract    ::= "|=" foreign_assumes
                      | "|=" foreign_ensures
                      | "|=" foreign_assumes "|=" foreign_ensures

foreign_ensures     ::= "@foreign_ensures" "(" ensures_predicate_list ")"

ensures_predicate_list ::= ensures_predicate ("," ensures_predicate)*

ensures_predicate   ::= predicate
                      | "@error" ":" predicate
                      | "@null_result" ":" predicate
```

**Semantics**

Postcondition predicates may reference:

- `@result`: The return value of the foreign procedure
- Parameter names (for checking output parameters)
- `@error`: Predicates that hold when the call fails (e.g., returns null or error code)
- `@null_result`: Predicates that hold specifically when result is null

```cursive
extern "C" {
    /// Allocates `size` bytes. Returns null on failure.
    procedure malloc(size: usize) -> *mut opaque c_void
        |= @foreign_assumes(size > 0)
        |= @foreign_ensures(
            @result != null implies aligned(@result, 16),
            @null_result: errno == ENOMEM
        );

    /// Reads up to `count` bytes. Returns bytes read, or -1 on error.
    procedure read(fd: c_int, buf: *mut opaque c_void, count: usize) -> isize
        |= @foreign_assumes(fd >= 0, buf != null)
        |= @foreign_ensures(
            @result >= -1,
            @result <= count as isize,
            @error: @result == -1 and errno != 0
        );

    /// Writes exactly `n` bytes or fails.
    procedure fwrite(
        ptr: *imm opaque c_void,
        size: c_size_t,
        count: c_size_t,
        stream: *mut FILE
    ) -> c_size_t
        |= @foreign_assumes(ptr != null, stream != null)
        |= @foreign_ensures(@result <= count);
}
```

**Verification Modes**

| Mode                   | `@foreign_ensures` Behavior                                      |
| :--------------------- | :--------------------------------------------------------------- |
| `[[static]]` (default) | Postconditions available as assumptions for downstream proofs    |
| `[[dynamic]]`          | Runtime assertions after foreign call returns                    |
| `[[assume]]`           | Postconditions assumed without checks (optimization only)        |
| `[[trust]]`            | Postconditions trusted without runtime checks (for audited code) |

**Static Usage**

When `@foreign_ensures` predicates are present, they MAY be used as assumptions for subsequent verification:

```cursive
extern "C" {
    procedure get_buffer(size: usize) -> *mut u8
        |= @foreign_assumes(size > 0 and size <= 1024)
        |= @foreign_ensures(@result != null, valid_for(@result, size));
}

```

**Dynamic Verification**

```cursive
[[dynamic]]
extern "C" {
    procedure risky_alloc(size: usize) -> *mut u8
        |= @foreign_ensures(@result != null);
}

```

**Trust Annotation**

For performance-critical code with audited foreign libraries:

```cursive
[[trust]]
extern "C" {
    procedure optimized_memcpy(dest: *mut u8, src: *imm u8, n: usize) -> *mut u8
        |= @foreign_assumes(dest != null, src != null)
        |= @foreign_ensures(@result == dest);
}
```

> **Warning:** `[[trust]]` suppresses runtime checks. Incorrect postconditions cause undefined behavior.

**Constraints & Legality**

| Code         | Severity | Condition                                 | Detection    | Effect    |
| :----------- | :------- | :---------------------------------------- | :----------- | :-------- |
| `E-SEM-2853` | Error    | Invalid predicate in `@foreign_ensures`   | Compile-time | Rejection |
| `E-SEM-2854` | Error    | `@result` used in non-return context      | Compile-time | Rejection |
| `E-SEM-2855` | Error    | `@error` predicate on void-returning proc | Compile-time | Rejection |
| `P-SEM-2861` | Panic    | `[[dynamic]]` postcondition violation     | Runtime      | Panic     |

##### Constraints & Legality

| Code         | Severity | Condition                                 | Detection    | Effect    |
| :----------- | :------- | :---------------------------------------- | :----------- | :-------- |
| `E-SEM-2850` | Error    | Cannot prove `@foreign_assumes` predicate | Compile-time | Rejection |
| `E-SEM-2851` | Error    | Invalid predicate in foreign contract     | Compile-time | Rejection |
| `E-SEM-2852` | Error    | Predicate references out-of-scope value   | Compile-time | Rejection |
| `P-SEM-2860` | Panic    | `[[dynamic]]` contract violation          | Runtime      | Panic     |

---

### 21.8 Platform Type Aliases

Platform ABI-specific aliases (`c_long`, `c_ulong`, `c_size_t`, etc.) MUST match the target C ABI types for the compilation target; exact sizes and alignments are implementation-defined and documented with the target triple.

## Appendix A: C Type Mapping Reference

##### Definition

This appendix provides the normative mapping between C types and their Cursive equivalents for FFI interoperability.

| C Type               | Cursive Type                               | Notes                         |
| :------------------- | :----------------------------------------- | :---------------------------- |
| `void`               | `()` (return) or `opaque c_void` (pointer) |                               |
| `_Bool`              | `CBool`                                    | NOT `bool`                    |
| `char`               | `c_char`                                   | Platform-dependent signedness |
| `signed char`        | `c_schar` / `i8`                           |                               |
| `unsigned char`      | `c_uchar` / `u8`                           |                               |
| `short`              | `c_short` / `i16`                          |                               |
| `unsigned short`     | `c_ushort` / `u16`                         |                               |
| `int`                | `c_int` / `i32`                            |                               |
| `unsigned int`       | `c_uint` / `u32`                           |                               |
| `long`               | `c_long`                                   | IDB: 32 or 64 bits            |
| `unsigned long`      | `c_ulong`                                  | IDB: 32 or 64 bits            |
| `long long`          | `c_longlong` / `i64`                       |                               |
| `unsigned long long` | `c_ulonglong` / `u64`                      |                               |
| `size_t`             | `c_size_t` / `usize`                       |                               |
| `ssize_t`            | `c_ssize_t` / `isize`                      |                               |
| `ptrdiff_t`          | `c_ptrdiff_t` / `isize`                    |                               |
| `float`              | `c_float` / `f32`                          |                               |
| `double`             | `c_double` / `f64`                         |                               |
| `T*`                 | `*mut T` or `*imm T`                       |                               |
| `const T*`           | `*imm T`                                   |                               |
| `T[]` / `T*` (array) | `BufferView<T>` / `BufferMut<T>`           |                               |
| `char*` (string)     | `CStr` / `CString`                         |                               |
| `void*`              | `*mut opaque c_void`                       |                               |
| Function pointer     | Sparse function type                       |                               |

---

## Appendix B: Compile-Time Layout Verification

##### Definition

For critical FFI types, layout MAY be verified at compile time using the `[[layout(C)]]` and `[[derive(FfiSafe)]]` facilities.

```cursive
[[layout(C)]]
[[derive(FfiSafe)]]
record MyStruct {
    field1: i32,
    field2: f64,
    field3: *mut u8,
}

comptime {
    // Verify layout matches C expectations
    static_assert(
        MyStruct::verify_layout(24, 8),
        "MyStruct layout mismatch with C definition"
    );
    
    // Or verify individual properties
    static_assert(
        MyStruct::c_size() == 24,
        "MyStruct size mismatch"
    );
    static_assert(
        MyStruct::c_alignment() == 8,
        "MyStruct alignment mismatch"
    );
}
```

---

## Appendix D: Formal Grammar (ANTLR)

##### Definition

Complete normative grammar for Cursive in ANTLR4 format. This grammar defines all lexical and syntactic productions required to parse valid Cursive source code.

##### Syntax & Declaration

**Grammar Structure**
- **Lexer Rules:** Keywords, identifiers (XID_START/XID_Continue), literals (integer/float/string/char), operators, comments
- **Parser Rules:** All declarations (record, enum, modal, class, procedure), expressions (precedence-encoded), statements, patterns
- **Operator Precedence:** Encoded in grammar hierarchy (14 levels from Postfix to Assignment)
- **Comment Nesting:** Block comments (`/* ... */`) MUST nest recursively; lexer maintains nesting counter
- **Maximal Munch Exception:** Generic argument exception per §2.7 (context-sensitive lexing)

##### Constraints & Legality

- Keywords are reserved per §2.6.
- Receiver shorthands (`~`, `~%`, `~!`) MUST appear only as first parameter in type method declarations.
- All lexical rules MUST conform to preprocessing pipeline output (§2.1).

## Appendix E: Diagnostics (Normative)

##### Definition

This appendix defines the normative taxonomy for diagnostics and the authoritative allocation of diagnostic code ranges. All conforming implementations **MUST** use these codes when reporting the corresponding conditions to ensure consistent error reporting across toolchains.

#### B.1 Diagnostic Code Format

##### Definition

Diagnostic codes follow the format `K-CAT-FFNN`:

*   **K (Kind/Severity):**
    *   `E`: **Error**. A violation of a normative requirement. Compilation cannot proceed to codegen.
    *   `W`: **Warning**. Code is well-formed but contains potential issues (e.g., deprecated usage).
    *   `N`: **Note**. Informational message attached to an error or warning.
    *   `I`: **Info / IDB Advisory**. Informational diagnostic indicating Implementation‑Defined or advisory behavior.
    *   `P`: **Panic**. Runtime trap with deterministic unwinding/abort semantics.
*   **CAT (Category):** A three-letter code identifying the language subsystem.
*   **FF (Feature Bucket):** Two digits identifying the specific feature area or chapter within the category.
*   **NN (Number):** Two digits uniquely identifying the specific condition within the bucket.

#### B.2 Code Range Allocation

##### Definition

This subsection defines the feature buckets (`FF`) used within each category (`CAT`) and their meanings.

| CAT   | FF   | Feature Area                                | Clause/Domain |
| :---- | :--- | :------------------------------------------ | :------------ |
| `CNF` | 01   | Conformance obligations and well‑formedness | Clause 1      |
| `CNF` | 03   | Implementation limits                       | Clause 1      |
| `CNF` | 04   | Reserved identifiers/namespaces             | Clause 1–2    |
| `CNF` | 06   | Editions and feature flags                  | Clause 1      |
| `SRC` | 01   | Source text, encoding, line structure       | Clause 2      |
| `SRC` | 03   | Literals and tokenization                   | Clause 2      |
| `SYN` | 01   | Terminators, continuations, general syntax  | Clause 2      |
| `SYN` | 05   | Module‑scope syntax restrictions            | Clause 3/12   |
| `MEM` | 30   | Memory model, provenance, moves             | Clause 3      |
| `TYP` | 15   | Type system foundations                     | Clause 4      |
| `TYP` | 16   | Permission system                           | Clause 4      |
| `TYP` | 17   | Primitive scalar types                      | Clause 5      |
| `TYP` | 18   | Composite sequence types                    | Clause 5      |
| `TYP` | 19   | Records and refinements                     | Clause 5/7    |
| `TYP` | 20   | Enums and modal types                       | Clause 5–6    |
| `TYP` | 21   | Pointer and string types                    | Clause 6      |
| `TYP` | 22   | Union and function types                    | Clause 5–6    |
| `TYP` | 23   | Generics                                    | Clause 7      |
| `DEC` | 24   | Declarations and attributes                 | Clause 8      |
| `NAM` | 13   | Name resolution                             | Clause 9      |
| `EXP` | 25   | Core expressions                            | Clause 12     |
| `EXP` | 26   | Control‑flow expressions                    | Clause 12     |
| `STM` | 26   | Statements                                  | Clause 12     |
| `PAT` | 22   | Union exhaustiveness                        | Clause 12     |
| `PAT` | 27   | General patterns                            | Clause 12     |
| `TRS` | 29   | Forms/foundational class constraints        | Clause 9      |
| `CON` | 28   | Contracts                                   | Clause 11     |
| `KEY` | 00   | Key system                                  | Clause 14     |
| `PAR` | 00   | Structured parallelism                      | Clause 15     |
| `ASY` | 00   | Async operations                            | Clause 16     |
| `DRV` | 00   | Derivation                                  | Clause 17     |
| `GEN` | 00   | Quote/splice/code generation                | Clause 17     |
| `DYN` | 00   | `[[dynamic]]` runtime proof insertion       | Clause 7/14   |
| `FFI` | 33   | Foreign Function Interface                  | Clause 18     |
| `ABI` | 22   | ABI and linkage                             | Clause 18     |

#### B.3 Reserved Ranges

##### Definition

Buckets not listed above, and `NN` values not assigned in the diagnostic catalog, are reserved for future use. Standard‑library and implementation‑specific diagnostics MUST use distinct categories or reserved buckets.

#### B.4 Conflict Resolution

When a program violation is detectable by multiple specification subsystems (e.g., both the type system and the memory model), the **canonical diagnostic** is determined by the following precedence:

1. Type System diagnostics (`E-TYP-`) are canonical for permission and type violations
2. Memory Model diagnostics (`E-MEM-`) are canonical for lifetime and move violations
3. Source diagnostics (`E-SRC-`) are canonical for lexical violations

Implementations MAY emit secondary diagnostics in addition to the canonical diagnostic when doing so provides distinct information.

## Appendix F: Conformance Dossier Schema

##### Definition

This appendix defines the normative requirements for the Conformance Dossier. A conforming implementation MUST produce a dossier artifact when the `dossier` emission phase is active.

#### C.1 File Format

The dossier MUST be a valid JSON document encoded in UTF-8.

#### C.2 Required Information

##### Definition

The Conformance Dossier MUST include the following information:

**Metadata:**
- Implementation identifier (vendor name)
- Implementation version (semantic versioning)
- Target triple (architecture-vendor-os)
- Build timestamp

**Configuration:**
- Conformance mode (`strict` or `permissive`)
- List of enabled language feature flags

**Safety Report:**
- Count of `unsafe` blocks in the compiled program
- List of IFNDR instances (file path, line number, category)

**Implementation-Defined Behavior:**
- Pointer width (32 or 64 bits)
- Type layout map for primitive types (size and alignment)

**Implementation Limits:**
- Maximum recursion depth
- Maximum identifier length
- Maximum source file size

## Appendix G: Standard Form Catalog

##### Definition

This appendix provides normative definitions for foundational forms and system capability forms built into Cursive or its core library.

##### Syntax & Declaration

**Foundational Forms** (§D.1):
- `Drop`: `procedure drop(~!)` — RAII cleanup, implicitly invoked
- `Copy`: Marker form for implicit bitwise duplication
- `Clone`: `procedure clone(~): Self` — explicit deep copy
- `Iterator`: `type Item; procedure next(~!): Self::Item | None` — iteration protocol for `loop ... in`
- `Step`: `procedure successor(~): Self | None; procedure predecessor(~): Self | None` — discrete stepping for range iteration

**System Capability Forms** (§D.2):
- `FileSystem`:
  - `open(path: string@View, mode: FileMode): FileHandle | IoError`
  - `exists(path: string@View): bool`
  - `restrict(path: string@View): dyn FileSystem` (attenuation)
- `Network`:
  - `connect(addr: NetAddr): Stream | NetError`
  - `bind(addr: NetAddr): Listener | NetError`
  - `restrict_to_host(addr: NetAddr): dyn Network` (attenuation)
- `HeapAllocator`:
  - `alloc<T>(count: usize): *mut T`
  - `dealloc<T>(ptr: *mut T, count: usize)`
  - `with_quota(size: usize): dyn HeapAllocator` (attenuation)
- `System`:
  - `exit(code: i32): !`
  - `get_env(key: string@View): string`
  - `spawn<T>(closure: () -> T): Thread<T>@Spawned` (Concurrency Path 2)
  - `time(): Timestamp`
  - `after(duration: Duration): Future<()>` (async timer, §15.6.1)
    *   `Reactor` (§15.11.1):
        -   `run<T, E>(async_val: Async<T, (), T, E>): T | E` - Drive async to completion
        -   `register(handle: IoHandle, interest: Interest): Token` - Register I/O source
        -   `poll_ready(token: Token): Readiness` - Check I/O readiness
    *   `Time`: Monotonic time access with `now(): Timestamp` procedure
  
##### Constraints & Legality

- `Drop::drop` MUST NOT be called directly by user code (`E-TYP-2620`).
- `Copy` and `Drop` are mutually exclusive on the same type (`E-TYP-2621`).
- `Copy` requires all fields implement `Copy` (`E-TYP-2622`).
- `HeapAllocator::alloc` MUST panic on OOM (never return null).
- Variadic implementations across all form types are prohibited.
- Any type implementing `Copy` MUST also implement `Clone`; for `Copy` types, `clone()` performs a bitwise copy identical to the implicit copy operation (Copy-Implies-Clone).

##### Static Semantics

- Drop behavior: See §3.6 (Deterministic Destruction) for complete semantics.
- `Copy` types are duplicated (not moved) on assignment/parameter passing.
- Static invalidation applies to aliases when Drop is invoked on owner.
- Capability forms enable attenuation patterns (e.g., `with_quota` on HeapAllocator).

##### Dynamic Semantics

- Drop execution and panic handling: See §3.6.
- HeapAllocator failures cause panic (no null returns).

## Appendix H: Core Library Specification

##### Definition

Minimal normative definitions for core types assumed to be available by the language without explicit import.

##### Syntax & Declaration

**Context Capability Record (Normative Definition)**

```cursive
record Context {
    // System capabilities
    fs: $FileSystem,
    net: $Network,
    sys: System,
    heap: $HeapAllocator,

    // Async runtime (§15.11.1)
    reactor: Reactor,

    // Execution domains (§14.6)
    cpu: CpuDomainFactory,
    gpu: GpuDomainFactory | None,
    inline: InlineDomainFactory,
}
```

**Field Descriptions:**

| Field     | Type                       | Purpose                          |
| :-------- | :------------------------- | :------------------------------- |
| `fs`      | `$FileSystem`              | Filesystem authority             |
| `net`     | `$Network`                 | Network authority                |
| `sys`     | `System`                   | System primitives (env, time)    |
| `heap`    | `$HeapAllocator`           | Dynamic allocation               |
| `reactor` | `Reactor`                  | Async runtime (§15.11.1)         |
| `cpu`     | `CpuDomainFactory`         | CPU parallel execution (§14.6.1) |
| `gpu`     | `GpuDomainFactory \| None` | GPU execution (§14.6.2)          |
| `inline`  | `InlineDomainFactory`      | Inline execution (§14.6.3)       |

*(Note: `Option` and `Result` are removed. Optionality is handled via Union Types e.g., `T | None`, and failure via `T | Error`. Pointers handle nullability via `Ptr<T>@Null` states.)*

##### Constraints & Legality

*   `Context` field types MUST match the capability form dynes defined in Appendix D
*   Core library types (`Context`, primitives) MUST be available in the universe scope (no import required)
*   `main` procedure MUST accept `Context` parameter as defined in §8.9

##### Static Semantics

*   All context capabilities are passed explicitly (no ambient authority)
  
### H.1 Standard FFI Types

The following library types provide C-compatible representations. They are **normative** parts of the core library and are referenced by Clause 20.

| Type                       | Definition (fields or states)                                                                          | Constraints                                                                                          |
| :------------------------- | :----------------------------------------------------------------------------------------------------- | :--------------------------------------------------------------------------------------------------- |
| `CStr`                     | `ptr: *imm c_char`                                                                                     | `ptr` non-null; buffer MUST be null-terminated.                                                      |
| `CString`                  | `ptr: *mut c_char`, `len: usize`, `cap: usize`                                                         | `ptr` MAY be null only when `len = cap = 0`; `len < cap`; byte at `ptr[len]` MUST be null.           |
| `NonNull<T>`               | `ptr: *mut T`                                                                                          | `ptr` non-null.                                                                                      |
| `OwnedPtr<T>`              | `ptr: *mut T`, `allocator: extern "C" fn(*mut opaque c_void)`                                          | `Drop`; destructor calls `allocator(ptr)` when `ptr` non-null.                                       |
| `BufferView<T <: FfiSafe>` | `ptr: *imm T`, `len: usize`                                                                            | `ptr` MAY be null only when `len = 0`; bounds checking is caller responsibility.                     |
| `BufferMut<T <: FfiSafe>`  | `ptr: *mut T`, `len: usize`                                                                            | Same null/len rule as `BufferView`; caller ensures uniqueness and bounds.                            |
| `CBool`                    | `[[layout(C)]] record { value: u8 }`                                                                   | Valid values `0` or `1`; size/alignment are IDB per platform ABI.                                    |
| `OwnedFd`                  | `fd: c_int`                                                                                            | `Drop` closes descriptor.                                                                            |
| `BorrowedFd`               | `fd: c_int`                                                                                            | Does not own; no `Drop`.                                                                             |
| `opaque c_void`            | Opaque type                                                                                            | Represents C `void`.                                                                                 |
| `IoError`                  | `enum { WouldBlock, Interrupted, InvalidData, NotFound, PermissionDenied, UnexpectedEof, Other(u32) }` | Enumerates standard I/O errors; `Other` carries platform code.                                       |
| `ForeignResource<T>`       | Modal with states `@Borrowed { ptr: *imm T }`, `@Owned { ptr: *mut T, dtor: extern "C" fn(*mut T) }`   | Transition rules follow modal semantics; ownership respected by destructor.                          |
| `VaList`                   | `record { handle: opaque c_void }`; `va_copy(dst: *mut VaList, src: *imm VaList)`                      | ABI-defined handle layout; `va_copy` mirrors C `va_copy`.                                            |
| Wide character aliases     | `c_wchar`, `c_char16`, `c_char32`                                                                      | `c_wchar` size/alignment IDB (2 or 4 bytes); `c_char16` = 2-byte UTF-16; `c_char32` = 4-byte UTF-32. |
  

## Appendix I: Behavior Classification Index (Normative)

##### Definition

This appendix provides an index of behaviors by classification. Definitions for each classification (Defined, IDB, USB, UVB) are in §1.2.

**F.1 Unverifiable Behavior (UVB) Instances**:
*   FFI Calls (§17)
*   Raw Pointer Dereference (§6.3)
*   Transmute Operations
*   Pointer Arithmetic

**F.2 Implementation-Defined Behavior (IDB) Instances**:
*   Type Layout (non-C)
*   Integer Overflow (Release)
*   Pointer Width
*   Resource Limits
*   Panic Abort Mechanism
*   Async State Layout (§15.3)
*   Async Discriminant Encoding (§15.3)
*   Async Cancellation I/O Behavior (§15.9)
*   Condition Wake Mechanism (§15.6.3)
*   **Dynamic Index Ordering Mechanism (§14.7)**: Index-value comparison, address comparison, lock coarsening, or hybrid. MUST satisfy cross-task consistency.

### F.3 Unspecified Behavior (USB) Instances

##### Definition

This subsection lists behaviors classified as Unspecified Behavior.

*   **Map Iteration**: Order of iteration for hash-based collections.
*   **Padding Bytes**: The values of padding bytes in composite types (§5 Universal Layout Principles).

### F.4 Defined Runtime Panics

##### Definition

This subsection lists runtime panics that are defined by the specification.

*   Integer Overflow (Checked Mode)
*   Array/Slice Bounds Check
*   **Dynamic Key Conflict** — occurs only in `[[dynamic]]` scopes where runtime key acquisition blocks indefinitely or detects deadlock (see §14.7)
*   **Contract Check Failure** — occurs in `[[dynamic]]` scopes when a runtime-checked contract predicate evaluates to false (`P-SEM-2850`)
*   **Refinement Validation Failure** — occurs in `[[dynamic]]` scopes when a runtime-checked refinement predicate evaluates to false (`P-TYP-1953`)

---
