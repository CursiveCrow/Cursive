# The Cursive Language Specification

**Version**: 1.0.0 Draft  
**Date**: 2025-11-11  
**Status**: In Progress  
**Purpose**: Normative reference for Cursive language implementation and tooling

---

## Abstract

This document is the normative specification for the Cursive programming language. It defines the language's syntax, semantics, and behavior with precision sufficient for building conforming implementations and tools. The specification serves implementers, tool developers, and educators, covering all aspects from lexical structure to concurrency and interoperability.


## Table of Contents

- [The Cursive Language Specification](#the-cursive-language-specification)
  - [Abstract](#abstract)
  - [Table of Contents](#table-of-contents)
- [Part 0 - Document Orientation](#part-0---document-orientation)
  - [1. Purpose, Scope, and Audience \[intro\]](#1-purpose-scope-and-audience-intro)
    - [1.1 Purpose \[intro.purpose\]](#11-purpose-intropurpose)
    - [1.2 Scope \[intro.scope\]](#12-scope-introscope)
    - [1.3 Audience \[intro.audience\]](#13-audience-introaudience)
  - [2. Document Primer \[primer\]](#2-document-primer-primer)
    - [2.1 Structure and Reference \[primer.structure\]](#21-structure-and-reference-primerstructure)
    - [2.2 Annotations \[primer.annotations\]](#22-annotations-primerannotations)
    - [2.3 Versioning \[primer.versioning\]](#23-versioning-primerversioning)
    - [2.4 Normative Requirement Organization \[primer.requirements\]](#24-normative-requirement-organization-primerrequirements)
  - [3 Notation \[notation\]](#3-notation-notation)
    - [3.1 Mathematical metavariables \[notation.metavariables\]](#31-mathematical-metavariables-notationmetavariables)
    - [3.1.1 Grammar template syntax \[notation.grammar\]](#311-grammar-template-syntax-notationgrammar)
    - [3.1.2 Mathematical Notation \[notation.mathematical\]](#312-mathematical-notation-notationmathematical)
  - [3.2 Inference Rules and Formal Semantics \[notation.inference\]](#32-inference-rules-and-formal-semantics-notationinference)
  - [4 Terminology \[terminology\]](#4-terminology-terminology)
    - [4.1 General Terms \[terminology.general\]](#41-general-terms-terminologygeneral)
    - [4.2 Conformance Terms \[terminology.conformance\]](#42-conformance-terms-terminologyconformance)
    - [4.3 Programming Terms \[terminology.programming\]](#43-programming-terms-terminologyprogramming)
    - [4.4 Symbols and Abbreviations \[terminology.symbols\]](#44-symbols-and-abbreviations-terminologysymbols)
  - [5. References and Citations \[references\]](#5-references-and-citations-references)
    - [5.1 ISO/IEC and International Standards \[references.iso\]](#51-isoiec-and-international-standards-referencesiso)
    - [5.2 Unicode Standards \[references.unicode\]](#52-unicode-standards-referencesunicode)
    - [5.3 Platform and ABI Standards \[references.platform\]](#53-platform-and-abi-standards-referencesplatform)
    - [5.4 IETF Standards \[references.ietf\]](#54-ietf-standards-referencesietf)
    - [5.5 Conventions, Tooling, and Miscellaneous \[references.conventions\]](#55-conventions-tooling-and-miscellaneous-referencesconventions)
    - [5.6 Research \[references.research\]](#56-research-referencesresearch)
    - [5.7 Reference Availability \[references.availability\]](#57-reference-availability-referencesavailability)
- [Part I - Conformance, Diagnostics, and Governance](#part-i---conformance-diagnostics-and-governance)
  - [6. Fundamental Conformance \[conformance\]](#6-fundamental-conformance-conformance)
    - [6.1 Behavior Classifications \[conformance.behavior\]](#61-behavior-classifications-conformancebehavior)
    - [6.2 Conformance Obligations \[conformance.obligations\]](#62-conformance-obligations-conformanceobligations)
    - [6.3 Binary Compatibility \[conformance.abi\]](#63-binary-compatibility-conformanceabi)
    - [6.4 Implementation Limits \[conformance.limits\]](#64-implementation-limits-conformancelimits)
    - [6.5 Reserved Identifiers \[conformance.reserved\]](#65-reserved-identifiers-conformancereserved)
    - [6.6 Conformance Verification \[conformance.verification\]](#66-conformance-verification-conformanceverification)
  - [7. Language Evolution and Governance \[evolution\]](#7-language-evolution-and-governance-evolution)
    - [7.1 Diagnostic Requirements \[diagnostics.requirements\]](#71-diagnostic-requirements-diagnosticsrequirements)
    - [7.2 Diagnostic Code System \[diagnostics.codes\]](#72-diagnostic-code-system-diagnosticscodes)
    - [7.3 Severity Levels \[diagnostics.severity\]](#73-severity-levels-diagnosticsseverity)
    - [7.4 IFNDR and Diagnostic Limits \[diagnostics.ifndr\]](#74-ifndr-and-diagnostic-limits-diagnosticsifndr)
    - [7.5 Implementation-Defined Behavior \[diagnostics.idb\]](#75-implementation-defined-behavior-diagnosticsidb)
    - [7.6 Diagnostic Catalog Reference \[diagnostics.catalog\]](#76-diagnostic-catalog-reference-diagnosticscatalog)
  - [8. Language Evolution and Governance \[evolution\]](#8-language-evolution-and-governance-evolution)
    - [8.1 Versioning Model \[evolution.versioning\]](#81-versioning-model-evolutionversioning)
    - [8.2 Feature Lifecycle \[evolution.lifecycle\]](#82-feature-lifecycle-evolutionlifecycle)
    - [8.3 Extension System \[evolution.extensions\]](#83-extension-system-evolutionextensions)
    - [8.4 Specification Maintenance \[evolution.specification\]](#84-specification-maintenance-evolutionspecification)
- [Part II - Program Structure \& Translation](#part-ii---program-structure--translation)
  - [8 Source Text \& Translation Pipeline](#8-source-text--translation-pipeline)
  - [9 Lexical Structure](#9-lexical-structure)
  - [10 Modules, Assemblies, and Projects](#10-modules-assemblies-and-projects)
    - [10.1 Folder-scoped modules](#101-folder-scoped-modules)
    - [10.2 Assemblies and projects](#102-assemblies-and-projects)
    - [10.3 Imports, aliases, and qualified visibility](#103-imports-aliases-and-qualified-visibility)
    - [10.4 Dependency graph and initialization](#104-dependency-graph-and-initialization)
  - [11 Scopes, Bindings, and Lookup](#11-scopes-bindings-and-lookup)
  - [12 Tooling hooks](#12-tooling-hooks)
- [Part III - Declarations \& the Type System](#part-iii---declarations--the-type-system)
  - [10 Declarations](#10-declarations)
  - [11 Type Foundations](#11-type-foundations)
  - [12 Primitive Types](#12-primitive-types)
  - [13 Composite Types](#13-composite-types)
  - [13. Modal Types](#13-modal-types)
  - [14. Pointer \& Reference Model](#14-pointer--reference-model)
  - [15. Generics \& Parametric Polymorphism](#15-generics--parametric-polymorphism)
- [Part IV - Ownership, Permissions, Regions](#part-iv---ownership-permissions-regions)
  - [16. Ownership \& Responsibility](#16-ownership--responsibility)
  - [17. Lexical Permission System (LPS)](#17-lexical-permission-system-lps)
  - [18. Region-Based Memory \& Arenas](#18-region-based-memory--arenas)
  - [19. Moves, Assignments, and Destruction](#19-moves-assignments-and-destruction)
- [Part V - Expressions, Statements, and Control Flow](#part-v---expressions-statements-and-control-flow)
  - [20. Expressions](#20-expressions)
  - [21. Statements](#21-statements)
  - [22. Control Flow and Pattern Matching](#22-control-flow-and-pattern-matching)
  - [23. Error Handling and Panics](#23-error-handling-and-panics)
- [Part VI - Contracts, and Grants](#part-vi---contracts-and-grants)
  - [14. Contract Language](#14-contract-language)
  - [15. Grants](#15-grants)
  - [16. Diagnostics for Contracts and Effects](#16-diagnostics-for-contracts-and-effects)
- [Part VII - Abstraction, Behaviors, and Dynamic Dispatch](#part-vii---abstraction-behaviors-and-dynamic-dispatch)
  - [17. Behaviors (Interfaces/Traits)](#17-behaviors-interfacestraits)
  - [18. Witness System (Runtime Polymorphism)](#18-witness-system-runtime-polymorphism)
  - [19. Generics \& Behaviors Integration](#19-generics--behaviors-integration)
- [Part VIII - Compile-Time Evaluation \& Reflection (No Macros)](#part-viii---compile-time-evaluation--reflection-no-macros)
  - [20. Comptime Execution](#20-comptime-execution)
  - [21. Opt-In Reflection](#21-opt-in-reflection)
  - [22. Code Generation APIs (Explicit Metaprogramming)](#22-code-generation-apis-explicit-metaprogramming)
- [Part IX - Memory Model, Layout, and ABI](#part-ix---memory-model-layout-and-abi)
  - [23. Memory Layout \& Alignment](#23-memory-layout--alignment)
  - [24. Interoperability \& FFI](#24-interoperability--ffi)
  - [25. Linkage, ODR, and Binary Compatibility](#25-linkage-odr-and-binary-compatibility)
- [Appendix A – Formal ANTLR Grammar (Normative)](#appendix-a--formal-antlr-grammar-normative)
- [Appendix B – Diagnostic Code Taxonomy (Normative)](#appendix-b--diagnostic-code-taxonomy-normative)
- [Appendix C – Implementation-Defined Behavior Index (Normative)](#appendix-c--implementation-defined-behavior-index-normative)
- [Appendix D – Diagnostic Catalog (Normative)](#appendix-d--diagnostic-catalog-normative)

---

# Part 0 - Document Orientation

## 1. Purpose, Scope, and Audience [intro]

### 1.1 Purpose [intro.purpose]

This specification **defines the Cursive programming language**: its source text, static semantics (typing and name resolution), dynamic semantics (runtime behavior), concurrency and memory model, and the required behavior of conforming implementations. Its purpose is to make the language precise for **implementers** (compiler/runtime authors) and **practitioners** building systems software.

This specification is the sole normative reference for the Cursive programming language. All implementations must conform to the requirements stated herein.

### 1.2 Scope [intro.scope]

The specification covers:

- The **lexical grammar**, **concrete syntax**, and **abstract syntax** of Cursive source files.
- The **type system**, including generics and variance, nominal types, and contracts (preconditions, postconditions, and effects).
- The **module, assembly, and package system** as well as cross-compilation units.
- The **execution semantics**, including evaluation order, error conditions, and observable program behavior.
- The **concurrency and memory model**, including happens-before relations and data-race freedom conditions where applicable.
- The definition of **unverifiable behavior (UVB)**, **implementation-defined behavior**, and **unspecified behavior**.
- The **Foreign Function Interface (FFI)** surface, insofar as the behavior of Cursive programs depends on it.
- Minimal **core library** requirements necessary for program execution where the language mandates them.

Out of scope (informative only): performance guidance, style advice, tutorial material, and non-normative rationale.

### 1.3 Audience [intro.audience]

The primary audiences of this document are: (a) **implementation authors** who must conform to these rules and (b) **users writing systems software** who require predictable, portable, and safe behavior. Secondary audiences include tool authors (linters, formatters, analyzers) and educators.

---

## 2. Document Primer [primer]

### 2.1 Structure and Reference [primer.structure]

This specification uses hierarchical decimal numbering (ISO 2145) with semantic anchors for stable cross-references.

#### Document Structure

```
# Document Title                              (Level 1 - Title)
# Part X - Part Name                          (Level 2 - Part headers)
## N. Chapter Name [chapter.id]               (Level 2 - Numbered chapters)
### N.M Grouping Section [chapter.section]    (Level 3 - Organizational sections)
#### N.M.K Normative Requirement              (Level 4 - All MUST/SHOULD/MAY statements)
##### N.M.K.J Sub-requirement                 (Level 5 - Rare exceptional cases only)
```

#### 2.1.1 Cross-References [primer.structure.references]

In-text cross-references use section numbers accompanied by semantic anchors, e.g. (1.2 [intro.scope]).

#### 2.1.2 Semantic Anchors [primer.structure.anchors]

Major sections include semantic anchors in square brackets `[category.subcategory]` or `[category.subcategory.detail]` providing stable links across document versions. Examples:

- `[intro.scope]` - Introduction and scope
- `[decl.bindings]` - Declaration of bindings
- `[types.composite.modal]` - Composite types, modal subsection

These anchors remain constant even if section numbers change between specification versions.

#### 2.1.3 Typographic Conventions [primer.structure.typography]

- Source code and tokens appear in `monospace`.
- Metavariables and definitions use _italics_.
- Keywords appear in **bold** when called out in prose.
- **_Bold italics_** denote intra-sectional divisions, such as **_Formal rule_**, **_Example_**, and **_Explanation_**.

The keywords **MUST**, **MUST NOT**, **SHOULD**, **SHOULD NOT**, **REQUIRED**, **RECOMMENDED**, and **MAY** are to be interpreted as described in RFC 2119 and RFC 8174 when, and only when, they appear in all capitals.

#### 2.1.4 Code Examples [primer.structure.examples]

Code examples in this document use fenced blocks with the `cursive` info string. Unless the text indicates otherwise, examples are informative; a trailing comment like `// error[E-CAT-FFNN]: …` marks the diagnostic the example is expected to trigger.

#### 2.1.5 Diagnostic Code Format [primer.structure.diagnostics]

Diagnostics follow the format `K-CAT-FFNN` where **K** indicates severity (`E` for error, `W` for warning, `N` for note), **CAT** is a three-letter category code, **FF** is a two-digit feature bucket, and **NN** is a sequence number within the bucket.

### 2.2 Annotations [primer.annotations]

The specification uses Markdown callout syntax for non-normative annotations:

> [!note]
> Non-normative clarifications, additional context, or explanatory remarks.

> [!tip] Rationale
> Design decisions and justifications (used sparingly, only for non-obvious choices).

> [!caution] Deprecated
> This feature is scheduled for removal in version <MAJOR.MINOR.PATCH>.
>
> This marker indicates a feature that:
>
> - The feature remains part of the current specification
> - The feature may be removed in the indicated version.
> - Implementations are encouraged to provide warnings when deprecated features are used (see §7.6 for implementation requirements)

> [!warning] Experimental
> This feature is experimental and subject to change.
>
> This annotation indicates a feature that:
>
> - Is not required for conformance
> - May change significantly in future versions
> - Require explicit opt-in at the implementation level

### 2.3 Versioning [primer.versioning]

This specification uses semantic versioning with the format `MAJOR.MINOR.PATCH`:

- **MAJOR** - incremented for incompatible changes that break existing conforming programs or change language semantics. Examples: new keywords that were previously valid identifiers, changes to type system rules, modifications to evaluation order.
- **MINOR** - incremented for backwards-compatible additions and clarifications that do not change the meaning of existing conforming programs. Examples: new language features with new syntax, clarifications that resolve ambiguity without changing behavior, additional diagnostic requirements.
- **PATCH** - incremented for backwards-compatible fixes and editorial corrections. Examples: typo corrections, improved wording, formatting changes, correction of internal cross-references.

The version identifier appears in the document header. This versioning scheme follows standard semantic versioning principles as defined in [SemVer 2.0.0](https://semver.org/).

### 2.4 Normative Requirement Organization [primer.requirements]

Throughout this specification, individual requirements generally follow this presentation pattern:

#### Normative Requirement Description

> The requirement itself, enclosed in a blockquote using normative keywords (**MUST**, **SHOULD**, **MAY**).
>
> **_Syntax:_**
>
> ```cursive
> Grammar patterns, when present, appear within the blockquote as part of the normative statement using template-based syntax (e.g., `<condition>`, `<block>`).
> ```

**_Formal rule:_**
$$ \text{A inference rule or judgment (when the requirement involves typing, evaluation, or well-formedness)} $$

**_Explanation:_**
Elaboration via prose, tables, or diagrams clarifying the requirement's meaning, rationale, or scope.

**_Example - Example Description_**

```cursive
Concrete code demonstrating the rule in practice, often including both valid and invalid cases
```

#### 2.2.1 Subordinate Requirements [primer.requirements.subordinate]

Some requirements contain subordinate subsections that refine or constrain the parent requirement. When present, subordinate requirements have the following structure:

- #### Main Heading
- > Main Requirement
  - ##### Subordinate Heading 1
  - > Subordinate Requirement 1
  - **_Subordinate Formal Rule 1_**
  - **_Subordinate Explanation 1_**
  - **_Subordinate Example 1_**
  - ##### Subordinate Heading 2
  - > Subordinate Requirement 2
  - [...]
- **_Main Formal Rule_**
- **_Main Explanation_**
- **_Main Example_**

**_Example - Syntactic and semantic requirement:_**

````markdown
##### 10.1.1 Binding kinds

> Every declaration **MUST** use either `let` (immutable) or `var` (rebindable).
> Rebinding a `let` binding **MUST NOT** be permitted.
>
> **_Syntax:_**
>
> ```
> let <pattern> = <expr>
> let <pattern> <- <expr>
> var <pattern> = <expr>
> var <pattern> <- <expr>
> ```

**_Formal rule:_**
[inference rule for well-formed bindings]

The distinction between `let` and `var` affects only rebindability, not the value's
internal mutability. The `=` operator creates a responsible binding that runs
destructors at scope exit, while `<-` creates a non-responsible binding.

**_Example - Valid bindings:_**

```cursive
let x = 5;
x = 10;  // error: cannot rebind `let` binding

var y = 5;
y = 10;  // ok: var allows rebinding
```
````

> [!note]
> The complete, formal ANTLR-format grammar production of Cursive is provided in Appendix A.

---

## 3 Notation [notation]

### 3.1 Mathematical metavariables [notation.metavariables]

Formal notation throughout this specification uses the following metavariables to denote syntactic and semantic entities:

- $x, y, z$ - _variables and identifiers_
- $T, U, V$ _(or $\tau$) - types_
- $e, f, g$ - _expressions_
- $p$ - _patterns_
- $s$ - _statements_
- $\Gamma, \Delta, \Sigma$ - _contexts_
- $\sigma$ - _program stores_
- $G$ - _grant sets_
- $@S$ - _modal states_
- $\ell$ - _memory locations_
- $v$ - _values_

**_Metavariable conventions:_**

- $'$ (prime) - _resulting or "after" states (e.g., $\Gamma'$, $\sigma'$)_
- Subscripts - _variants or specialized forms (e.g., $\Gamma_{\text{terms}}$, $\sigma_{\text{ct}}$)\_
- $\cdot$ - _empty context_

These symbols serve as placeholders in inference rules, judgments, and formal definitions.

### 3.1.1 Grammar template syntax [notation.grammar]

Grammar productions within normative statements use angle-bracket placeholders to denote syntactic categories. These placeholders represent portions of syntax that must be filled with concrete constructs:

***Common syntactic categories:***

- `<identifier>` - any valid identifier (as defined in Part II, §9.1.1)
- `<expression>` - any expression
- `<statement>` or `<statements>` - one or more statements
- `<pattern>` - a binding pattern (identifier, tuple destructuring, etc.)
- `<condition>` - a boolean-valued expression
- `<block>` - a brace-enclosed block of statements `{ ... }`
- `<type>` - a type expression
- `<literal>` - a literal value (numeric, string, character, or boolean)
- `<parameters>` - comma-separated list of function parameters

**_Repetition and optional elements:_**
When inline grammar fragments require repetition or optionality, the following standard conventions apply:

- `element*` - zero or more repetitions
- `element+` - one or more repetitions
- `element?` - optional (zero or one occurrence)
- `alt1 | alt2` - alternatives (either alt1 or alt2)

**_Example usage in normative statements:_**

```
let <pattern> = <expr>
if <condition> { <statements> }
procedure <identifier>(<parameters>) -> <type>
```

These placeholders are purely descriptive and indicate what kind of syntax is expected in each position. The complete formal ANTLR grammar appears in an appendix; inline grammar fragments use this template notation for readability.

### 3.1.2 Mathematical Notation [notation.mathematical]

Formal definitions and inference rules use standard mathematical notation:

**_Set operations:_**

- $\in$, $\notin$ - set membership and non-membership
- $\cup$ - union
- $\cap$ - intersection
- $\subseteq$ - subset relation

**_Logical connectives:_**

- $\land$ - logical and
- $\lor$ - logical or
- $\lnot$ - logical negation
- $\implies$ - implication
- $\iff$ - if and only if (bidirectional implication)

**_Quantifiers:_**

- $\forall$ - universal quantification ("for all")
- $\exists$ - existential quantification ("there exists")

**_Functions and mappings:_**

- $f: A \to B$ - (total) function from type $A$ to type $B$
- $f: A \rightharpoonup B$ - partial function (may be undefined for some inputs)
- $(a, b, c)$ - tuple or sequence of elements

These symbols follow their standard mathematical meanings without additional specification-specific interpretation.

## 3.2 Inference Rules and Formal Semantics [notation.inference]

Inference rules use standard fraction notation:

$$
\frac{\text{premise}_1 \quad \cdots \quad \text{premise}_n}{\text{conclusion}}
\tag{Rule-Name}
$$

Rules without premises are axioms. Side conditions appear to the right of the line when additional constraints apply.

**_Judgment grammar_:**

- $\vdash$ - turnstile; $\Gamma \vdash J$ means judgment $J$ holds under context $\Gamma$
- $\Rightarrow$ - transformation; $\Gamma \vdash X \Rightarrow \Gamma'$ means processing $X$ transforms $\Gamma$ to $\Gamma'$
- $\Downarrow$ - big-step evaluation; $\sigma \vdash e \Downarrow v, \sigma'$ means $e$ evaluates to $v$ in store $\sigma$ producing store $\sigma'$
- $[\text{cat}]$ - categorization; indicates expression category in typing judgments
- $,$ (comma) - sequencing; separates input/output components (e.g., $v, \sigma'$)
- $\Gamma, x : B$ - context extension (adds binding of $x$ to $B$)
- $\Gamma[x \mapsto \tau]$ - context update (replaces binding for $x$)

**_Rule naming prefixes_:**

- **T**-_Feature_-_Case_ - Type formation and typing rules
- **E**-_Feature_-_Case_ - Evaluation and operational semantics
- **WF**-_Feature_-_Case_ - Well-formedness and static checking
- **P**-_Feature_-_Case_ - Permission and memory safety
- **Prop**-_Feature_-_Case_ - Behavior satisfaction and property proofs
- **Coerce**-_Feature_-_Case_ - Type coercion rules
- **Prov**-_Feature_-_Case_ - Provenance and aliasing
- **Ptr**-_Feature_-_Case_ - Pointer-specific properties
- **QR**-_Feature_-_Case_ - Qualified name resolution

---

## 4 Terminology [terminology]

The vocabulary below defines terms as used throughout this specification. Terms defined in this section carry the same meaning in all later sections.

### 4.1 General Terms [terminology.general]

**Implementation**
_Any compiler, interpreter, JIT, transpiler, or runtime system that accepts Cursive source code and produces program behavior. Unless stated otherwise, every requirement labeled "implementation" applies to all of these tool categories (cf. §7.1)._

**Program**
_A complete, standalone unit comprising one or more Cursive translation units intended for execution. Programs may link against or embed libraries. For conformance evaluation, the entire program (including all linked code) must be well-formed, and execution must not trigger unverifiable behavior. A conforming program, together with its dependencies, exhibits only defined behavior (cf. §6.2.2)._

**Library**
_A reusable collection of declarations intended for use by programs or other libraries. Libraries are evaluated for conformance independently. A conforming library must not rely on unverifiable behavior and must document all implementation-defined behavior choices (cf. §6.2.2)._

**Translation**
_The process of converting source code into executable artifacts._

**Execution**
_The runtime evaluation of translated code._

### 4.2 Conformance Terms [terminology.conformance]

**Conforming Implementation**
_An implementation is conforming iff it satisfies all MUST requirements of this specification and, for any accepted program, produces observable behavior that matches the dynamic semantics, except where implementation‑defined or unspecified behavior permits variation. See §6.2.1._

**Conforming Program**
_A program is conforming iff it is well‑formed, relies only on documented IDB, uses extensions only when explicitly enabled, and does not trigger UVB during execution unless every executed UVB site is attested by an external proof artifact recorded in the conformance dossier (see §6.2.4). The presence of unattested UVB site executions makes a program non‑conforming._

**Unverifiable Behavior (UVB)**
_Behavior arising from operations whose correctness the implementation cannot verify statically or dynamically. Implementations are not required to detect or prevent UVB and may terminate the process outside normal control flow if it occurs; external contracts (e.g., FFI) govern soundness at such sites. See §6.1.1 and §6.1.2._

**Unspecified Behavior (USB)**
_A set of outcomes permitted by this specification for a construct where the implementation need not document which outcome it chooses; the set of permitted outcomes is constrained by this specification. See §6.1.3._

**Implementation-Defined Behavior (IDB)**
_A choice among multiple outcomes permitted by this specification that the implementation MUST document (e.g., in a conformance dossier or target notes). Programs MUST NOT rely on undocumented choices. See §6.1.4 and §6.1.4.1._

**Well-formed Program**
_Source that satisfies all lexical, syntactic, and static‑semantic rules. Implementations MUST accept well‑formed programs. See §6.2.3._

**Ill-formed Program**
_Source that violates lexical, syntactic, or static‑semantic rules. Implementations MUST diagnose and reject such programs; no artifacts may be produced. Certain violations may be classified IFNDR when detection is computationally infeasible (§6.1.3.1); otherwise, at least one error MUST be issued. See §6.2.3._

### 4.3 Programming Terms [terminology.programming]

**Binding**
_the association between an identifier and an entity within a particular scope._

**Translation Unit**
_the smallest source artifact processed as a whole during translation_.

**Declaration**
_a syntactic form that introduces a name and determines its category._

**Diagnostic**
_a message issued when a program violates a rule requiring detection. Diagnostics include errors (rejecting) and warnings (permissive)._

**Entity**
_any value, type, or module that may be named or referenced._

**Object**
_a region of storage with a type, storage location (contiguous bytes at a specific memory address), lifetime (bounded interval from creation to destruction), alignment constraint, and cleanup responsibility (association with zero or more responsible bindings)._

**Value**
_a temporary result produced by an expression that may be moved (consumed) or copied. Literals, arithmetic expressions, and most procedure calls produce values._

**Expression**
_a syntactic form that yields a value or place._

**Place**
_a memory location that can be the target of assignment or the source of address-of operations; expressions are categorized as yielding values or places._

**Modal State**
_a named compile-time state (denoted `@State`) in a modal type's state machine; modal values must inhabit exactly one state at any time, and transitions between states are enforced by the type system._

**Statement**
_a syntactic form that executes for effects on program state without directly yielding a value._

**Scope**
_the syntactic region where a binding is visible._

**Principal Type**
_in a type inference context, the most general type that satisfies all constraints. When type inference succeeds, the principal type is unique._

**Nominal Type**
_a type identified by a declaration and name, possibly parametrized; variance of parameters is declared per parameter._

### 4.4 Symbols and Abbreviations [terminology.symbols]

- **ABI** - Application Binary Interface
- **AST** - Abstract Syntax Tree
- **EBNF** - Extended Backus-Naur Form
- **FFI** - Foreign Function Interface
- **IFNDR** - Ill-Formed, No Diagnostic Required
- **LPS** - Lexical Permission System
- **RAII** - Resource Acquired on Initialization
- **UVB** - Unverifiable Behavior
- **UTF-8** - Unicode Transformation Format, 8-bit

---

## 5. References and Citations [references]

The documents listed below contain provisions that, through citation, form requirements of this specification. Documents identified as normative define requirements for conforming implementations.

### 5.1 ISO/IEC and International Standards [references.iso]

- **[ISO10646]** ISO/IEC 10646:2020 - _Information technology - Universal Coded Character Set (UCS)_. Defines the character repertoire permitted in source text and informs identifier classifications.

- **[IEEE754]** ISO/IEC 60559:2020 (IEEE 754-2019) - _Floating-Point Arithmetic_. Governs semantics for binary32 (`f32`) and binary64 (`f64`) arithmetic used throughout numeric type definitions and runtime semantics.

- **[C18]** ISO/IEC 9899:2018 - _Programming Languages - C_. The Foreign Function Interface (FFI) relies on C definitions of object representation, calling conventions, and interoperability semantics.

### 5.2 Unicode Standards [references.unicode]

- **[Unicode]** The Unicode Standard, Version 15.0.0 or later. Supplies derived properties, normalization forms, and identifier recommendations leveraged by the lexical grammar.

- **[UAX31]** Unicode Standard Annex #31 - _Unicode Identifier and Pattern Syntax_. Specifies identifier composition rules (`XID_Start`, `XID_Continue`) adopted in lexical tokenization.

### 5.3 Platform and ABI Standards [references.platform]

- **[SysV-ABI]** System V Application Binary Interface (AMD64 Architecture Processor Supplement, ARM Architecture Procedure Call Standard). Guides interoperability obligations for POSIX-compliant platforms.

- **[MS-x64]** Microsoft x64 Calling Convention - Calling convention documentation for Windows x64 platforms. Required for conforming implementations targeting Windows.

### 5.4 IETF Standards [references.ietf]

- **[RFC2119]** RFC 2119 - _Key words for use in RFCs to Indicate Requirement Levels_. Defines normative vocabulary (**MUST**, **SHOULD**, **MAY**).

- **[RFC8174]** RFC 8174 - _Ambiguity of Uppercase vs Lowercase in RFC 2119 Key Words_. Clarifies that requirement keywords apply only when in ALL CAPS.

### 5.5 Conventions, Tooling, and Miscellaneous [references.conventions]

- **[SemVer2]** Tom Preston‑Werner, “Semantic Versioning 2.0.0.” Defines versioning rules for artifacts and tools referenced by this specification.

- **[ANTLR4]** Terence Parr and Sam Harwell, “ANTLR 4 Grammar Syntax and Toolchain.” Cited for grammar notation and tooling guidance used in the formal grammar appendix.

### 5.6 Research [references.research]

- **[C18-Spec]** ISO/IEC 9899:2018 — Programming Languages — C. Informative comparison point for FFI object representation and linkage models.
- **[CXX23]** ISO/IEC 14882:2023 — Programming Languages — C++. Referenced for comparative discussion of templates, layout, and ABI interactions.
- **[Rust-Ref]** The Rust Reference — The Rust Project Developers. Informative background on contracts, ownership models, and diagnostics organization.
- **[Go-Spec]** The Go Programming Language Specification — The Go Authors. Informative background on packages, interfaces, and memory model choices.
- **[ECMA-334]** ECMA‑334 — C# Language Specification (latest edition). Informative background on attributes, metadata, and generics.
- **[PLP5]** Michael L. Scott, "Programming Language Pragmatics," Fifth Edition. ISBN‑13: 978‑0323999663. General reference for language design trade‑offs and formalism.

### 5.7 Reference Availability [references.availability]

Where references are not available freely, they are available for purchase from their respective publishers.

---

# Part I - Conformance, Diagnostics, and Governance

This part establishes the foundational requirements for conforming Cursive implementations, defines diagnostic reporting standards, and specifies governance rules for extensions and backward compatibility. It provides the normative framework that all other parts of this specification build upon, including conformance obligations, diagnostic taxonomy, and behavior classification.

## 6. Fundamental Conformance [conformance]

This chapter defines behavior classifications and conformance obligations for implementations and programs.

### 6.1 Behavior Classifications [conformance.behavior]

This section defines four categories of program behavior based on how this specification constrains outcomes and verification feasibility.

**Unverifiable Behavior (UVB)** [§4.2] comprises operations whose runtime correctness depends on properties that exist outside the language's semantic model or require solving computationally intractable verification problems:

1. **Semantic boundary**: Properties depend on external contracts, foreign memory validity, or environmental state not represented in Cursive's type system
2. **Computational intractability**: Verification requires solving undecidable problems (halting), NP-complete problems with exponential worst-case, or whole-program analysis across compilation boundaries
3. **Information absence**: The required verification information does not exist within the program's Cursive code

UVB classification is about the verification *problem structure*, not implementation capabilities. Operations are UVB when their correctness depends on information unavailable to the language's verification model.

**Contrast:** Array bounds checking is NOT UVB because array length is known within Cursive semantics; verification is feasible through static analysis or runtime checks. FFI pointer dereferencing IS UVB because pointer validity depends on foreign contracts outside Cursive's semantic model.

**Unspecified Behavior (USB)** [§4.2] occurs when this specification permits multiple valid outcomes but does not require implementations to document which outcome is chosen. Implementations may choose any permitted outcome, and the choice may vary between executions. Unlike implementation-defined behavior, no documentation is required. All permitted outcomes fall within bounds established by this specification.

**Implementation-Defined Behavior (IDB)** [§4.2] occurs when this specification permits multiple valid outcomes and requires implementations to document their choice. Unlike unspecified behavior, implementations must document their choices, enabling portability analysis. Common sources include target architecture characteristics (pointer size, endianness, alignment), calling conventions, and memory layout decisions.

**Defined Behavior** [§4.2] comprises all program behavior fully specified by this specification. Operations produce deterministic outcomes identical across conforming implementations (except where IDB allows documented variation). Defined behavior is the complement of UVB, USB, and IDB. Most language operations exhibit defined behavior: arithmetic on in-range values, control flow, pattern matching, bounded memory access, etc.

#### 6.1.1 Acceptance of UVB Programs [conformance.behavior.uvb]

> Implementations **MUST** accept well-formed programs containing UVB operations and **MUST NOT** reject programs solely for containing UVB.
>
> Translation **MUST** preserve program semantics up to and including UVB operation initiation. When UVB operations execute, implementations **MAY** produce any behavior, including abnormal termination.

**_Explanation:_**
Implementations need not detect UVB at compile time, emit warnings, or provide runtime protections. Programmers must ensure UVB operation correctness through external means (documentation, testing, manual proof, or external verification tools). The scope of “any behavior” at a UVB site is limited to the current process’s address space and externally observable effects of that process; the specification makes no guarantees outside that scope.

#### 6.1.2 Verification Modes and Resource Limits [conformance.behavior.verification]

> UVB classification is independent of verification modes (see Part VI, §16.5). Properties verifiable through feasible static analysis or runtime checks are NOT UVB, regardless of resource limits or policy choices.
>
> When verification fails due to resource constraints, implementations **MUST** either (a) reject the program with a diagnostic in strict mode, or (b) insert runtime checks in dynamic verification mode. Implementations **MUST NOT** insert runtime checks in strict mode unless explicitly enabled by a build flag or attribute.

**_Explanation:_**
Implementations cannot avoid verification obligations by claiming resource exhaustion. UVB operations are inherently unverifiable due to problem structure; non-UVB operations may be expensive to verify but remain the implementation's responsibility.

**_Example - UVB from FFI (logical impossibility):_**

```cursive
// Foreign function contract exists outside Cursive's semantic model
[[extern(C)]]
procedure legacy_get_buffer(): *mut u8;

unsafe {
    let ptr: *mut u8 = legacy_get_buffer();
    let first_byte = *ptr;  // UVB: Correctness depends on foreign contract:
                            // - Is the pointer non-null? (unknowable)
                            // - Does it point to valid, initialized memory? (unknowable)
                            // - Is the memory region accessible? (unknowable)
                            // - What is the pointer's provenance? (unknowable)
                            //
                            // Implementation cannot verify these properties.
                            // Programmer must ensure correctness through external means.
}
```

#### 6.1.3 Unspecified Behavior Requirements [conformance.behavior.usb]

> Permitted outcomes for USB **MUST** be constrained by this specification. Implementations **MAY** choose any permitted outcome. Different executions **MAY** produce different outcomes.

**_Explanation:_**
Unlike IDB (§6.1.4), implementations need not document their choice, and the choice may vary between executions. All permitted outcomes fall within specification bounds. Programs exhibiting USB are conforming.

**_Example - Unspecified padding bytes:_**

```cursive
record Point { x: i32, y: i32 }

procedure example() {
    let p = Point { x: 10, y: 20 };
    // Unspecified: padding bytes between fields and after the last field
    // may contain any bit pattern. Reading padding is well-formed but
    // produces unspecified values.

    let bytes = &p as *const u8;
    // bytes[0..3] are specified (x's representation)
    // bytes[4..7] are specified (y's representation)
    // Any padding bytes are unspecified
}
```

##### 6.1.3.1 Ill-Formed, No Diagnostic Required (IFNDR) [conformance.behavior.ifndr]

> Programs violating static-semantic rules are ill-formed. When violations are computationally infeasible to detect, implementations need not issue diagnostics. Such programs are **Ill-Formed, No Diagnostic Required (IFNDR)**.
>
> **Language Design Invariant:** Cursive's syntax and static semantics ensure that IFNDR violations lead to one of the following outcomes:
> 1. Defined behavior that happens to satisfy the violated rule
> 2. Benign failure: process termination via panic or trap, without memory unsafety
> 3. Unspecified but bounded behavior (e.g., incorrect computation results, non-deterministic control flow)
>
> UVB arises exclusively from operations within `unsafe` blocks and FFI calls whose preconditions depend on foreign contracts. IFNDR violations outside these constructs cannot introduce UVB because the language design prevents unsafe memory operations in safe code, regardless of static rule violations.

**_Explanation:_**
Violations are computationally infeasible to detect when detection requires solving NP-complete, undecidable, or exponential-complexity problems, or whole-program properties across compilation boundaries.

IFNDR differs from UVB: IFNDR represents ill-formedness; UVB represents operations whose runtime correctness depends on external properties. IFNDR programs are not conforming (§6.2.2). The language design ensures IFNDR cannot introduce UVB in safe code.

> [!note] IFNDR vs UVB distinction
>
> - **IFNDR**: Program violated a language rule, but detecting the violation is computationally infeasible. Behavior is unspecified but constrained.
> - **UVB**: Program is well-formed, but operation correctness depends on unverifiable properties. Behavior is unconstrained.

**_Example - IFNDR via Semantic Unreachability_**

```cursive
// Whether this loop terminates reduces to the halting problem for 'f'.
procedure f(n: i64) -> i64 { /* ... Turing-complete ... */ }
procedure g(n: i64) {
    match f(n) {
        0 => handle_zero(),
        _ => handle_nonzero(),
    }
    // Omitting a 'panic' branch that is reachable iff f(n) never halts
    // makes 'g' ill-formed only if one can prove non-termination.
    // Diagnosing that is computationally infeasible: IFNDR.
}
```

#### 6.1.4 Implementation-Defined Behavior [conformance.behavior.idb]

> IDB occurs when this specification permits multiple valid outcomes and requires implementations to document their choice. Permitted outcomes **MUST** be constrained by this specification.
>
> Implementations **MUST** document all IDB choices in a conformance dossier (§6.2.4). Programs **MUST NOT** rely on undocumented IDB.

**_Explanation:_**
IDB allows target-appropriate choices while ensuring predictability. Unlike USB [§6.1.3], implementations must document their choices. Common sources include target architecture characteristics (pointer size, endianness, alignment), calling conventions, and memory layout.

##### 6.1.4.1 Documentation Requirements [conformance.behavior.idb.documentation]

> Implementations **MUST** document behaviors affecting portability or correctness:
>
> 1. Primitive type sizes, alignments, and representations (see Part III, §12.1; Part IX, §23.1)
> 2. Pointer width, representation, and provenance rules (see Part III, §14; Part IV, §17)
> 3. Foreign function calling conventions (see Part IX, §24.4)
> 4. Endianness for multi-byte values
> 5. Memory layout and padding for compound types (see Part IX, §23.4)
> 6. Signed integer overflow behavior
> 7. Thread scheduling, memory ordering, and synchronization (see Part IX, §23.8; Part IV, §17)
> 8. Platform-specific panic and unwind behavior (see Part V, §23.2)
>
> Documentation **SHOULD** be accessible via `--version`, online docs, or metadata files, organized by target triple and language version.

**_Example - Target-specific sizes and calling conventions:_**

```cursive
// Implementation-defined: size and alignment of primitive types
let x: i32 = 42;        // Size: documented in conformance dossier (typically 4 bytes)
                        // Alignment: documented per target (typically 4 bytes)

let p: usize = 0x1000;  // Size: pointer width, documented per target
                        // 8 bytes on 64-bit targets, 4 bytes on 32-bit targets

// Implementation-defined: endianness affects byte representation
let value: u32 = 0x01020304;
let bytes = &value as *const u8;
// Byte order documented for target:
// - Little-endian (x86-64, ARM64): [0x04, 0x03, 0x02, 0x01]
// - Big-endian (some MIPS, PowerPC): [0x01, 0x02, 0x03, 0x04]

// Implementation-defined: calling convention for extern functions
[[extern(C)]]    // C convention documented per platform ABI:
                 // - x86-64 Linux: System V AMD64 ABI
                 // - x86-64 Windows: Microsoft x64 calling convention
                 // - ARM64: ARM AAPCS64
procedure foreign_call();
```

#### 6.1.5 Behavior Classification Criteria [conformance.behavior.classification]

> Behaviors are classified as implementation-defined (IDB) when any of the following apply:
>
> 1. **Portability impact**: Different choices produce observably different program behavior across conforming implementations
> 2. **ABI impact**: The choice affects binary interfaces, memory layout, or cross-language interoperability
> 3. **Correctness reasoning**: Developers must understand the choice to reason about program correctness
> 4. **Determinism**: The choice affects whether program behavior is deterministic
>
> Behaviors are classified as unspecified (USB) when:
>
> 1. The choice affects only internal implementation details
> 2. Well-formed programs cannot observe the difference
> 3. The specification constrains all permitted outcomes to exclude incorrect behavior

**_Example - IDB vs USB classification:_**

```cursive
// IDB: Pointer size affects observable behavior
let p: *const u8 = 0x1000 as *const u8;
let size = size_of::<*const u8>();  // 4 or 8 bytes - IDB, must document

// USB: Padding byte values unobservable in safe code
record Point { x: i32, y: i64 }
// Padding between x and y: USB, no documentation required
// Safe code cannot read padding bytes
```

### 6.2 Conformance Obligations [conformance.obligations]

This section defines conformance for implementations and programs.

#### 6.2.1 Conforming Implementations [conformance.obligations.implementations]

> Implementations **MUST** satisfy all MUST requirements and produce programs whose observable behavior matches the dynamic semantics defined herein, except where IDB (§6.1.4) or USB (§6.1.3) permits variation.

#### 6.2.2 Conforming Programs [conformance.obligations.programs]

> A program conforms when it satisfies:
>
> 1. **Well-formedness**: Satisfies all lexical, syntactic, and static-semantic rules (§6.2.3)
> 2. **Documented IDB reliance**: Relies only on documented IDB (§6.1.4.1, §6.2.4)
> 3. **Extension usage**: Uses extensions only when explicitly enabled (§8)
> 4. **UVB attestation**: Every executed UVB operation **MUST** be accompanied by an attestation entry in the conformance dossier proving external correctness verification. Attestations **MUST** reference the source location, the verification method (formal proof, manual audit, test coverage), and provide either a proof artifact URI or cryptographic hash of the proof document.

**_Explanation:_**
UVB operations are non-conforming by default because their correctness cannot be verified within the language's semantic model. External verification establishes correctness through means outside the type system: formal methods tools, manual security audits, comprehensive test suites covering the UVB operation's preconditions, or mathematical proofs documented separately.

The attestation system enables conformance checking: build tools can verify that every UVB site executed during testing has a corresponding attestation entry, failing the build for unattested UVB operations. This transforms UVB from "undefined behavior" into "externally verified behavior."

Programs containing IFNDR violations (§6.1.3.1) are ill-formed and non-conforming, even without diagnostics.

#### 6.2.3 Well-Formedness [conformance.obligations.wellformedness]

> Implementations **MUST** accept well-formed programs and **MUST** reject ill-formed programs by issuing at least one error diagnostic, except for IFNDR violations (§6.1.3.1).
>
> Implementations **MUST NOT** produce executable artifacts for rejected programs. Implementations **MAY** produce non-executable artifacts for tooling (syntax trees, symbol tables, diagnostic metadata).
>
> Implementations **MAY** issue warnings for accepted programs without affecting conformance.

**_Explanation:_**
The executable/non-executable distinction supports tooling (IDEs, analyzers) for ill-formed programs while preventing execution.

IFNDR programs remain ill-formed and non-conforming even without diagnostics. Implementations are encouraged to diagnose IFNDR when feasible (§7.6.5).

#### 6.2.4 Conformance Dossier [conformance.obligations.dossier]

> An implementation **MUST** document all "Implementation‑defined behaviors" per target in a JSON or TOML document with stable keys: primitive_sizes, endianness, pointer_width, panic_policy, thread_model, etc. “UVB attestations” **MUST** enumerate executed UVB sites by file, span, rule, and attach proof_uri or an embedded proof hash. Conformance tools **MUST** be able to fail the build when any executed UVB site lacks an attestation entry. Documentation **SHOULD** be accessible via online docs, metadata files (JSON, TOML), or command-line flags (`--print-targets`, `--print-features`, `--print-conformance`).


**_Example - Conformance dossier structure:_**

```
Cursive Reference Implementation Conformance Dossier
Version: 1.0.0
Release Date: 2025-01-15
Vendor: Example Corporation

1. Supported Targets
  - x86_64-unknown-linux-gnu
    - Pointer size: 8 bytes
    - Endianness: little-endian
    - Default calling convention: System V AMD64 ABI
    - Maximum alignment: 16 bytes
    [... detailed target characteristics ...]

2. Implementation-Defined Behaviors
  2.1 Primitive Type Sizes
    - i8, u8: 1 byte, alignment 1
    - i16, u16: 2 bytes, alignment 2
    - i32, u32: 4 bytes, alignment 4
    [...]

  2.2 Memory Layout
    - Aggregate field ordering: declaration order preserved
    - Padding insertion: minimum required for alignment
    [...]

3. Extensions
  [Feature table listing all extensions...]

4. Diagnostic Codes
  [Complete diagnostic taxonomy...]
```

#### 6.2.5 Observable Behavior [conformance.obligations.observable]

> Observable behavior comprises program effects that are externally visible or specified as observable by this specification, including:
>
> 1. Input and output operations
> 2. Accesses to volatile memory locations
> 3. Synchronization operations and their ordering
> 4. Program termination and exit status
> 5. Diagnostic output required by this specification
>
> Implementations **MAY** perform any transformation to a program provided the observable behavior of the transformed program matches the observable behavior required by this specification. This is known as the **as-if rule**.
>
> Implementations **MAY** eliminate, reorder, or optimize operations that do not affect observable behavior. Operations on non-volatile memory locations that do not contribute to observable behavior **MAY** be elided.
>
> The as-if rule **MUST NOT** be used to justify transformations that alter:
>
> 1. The values or sequencing of observable operations
> 2. Whether the program terminates or diverges
> 3. Whether the program triggers unverifiable behavior (UVB, §6.1.1)
> 4. The synchronization ordering guaranteed by the memory model (Part IX, §23.8)

#### 6.2.6 Language Safety Guarantees [conformance.obligations.safety]

> Implementations **MUST** provide the following safety guarantees for well-formed programs that do not use `unsafe` blocks or foreign function calls:
>
> 1. **Memory safety**: Programs cannot read or write memory outside the bounds of allocated objects. Buffer overruns, use-after-free, and dangling pointer dereferences are prevented by the type system and permission system.
>
> 2. **Type safety**: Programs cannot observe values whose representation violates their type's invariants. Type confusion, invalid enum variants, and violations of type system guarantees are prevented.
>
> 3. **Data-race freedom**: Programs are data-race-free by construction. The permission system (Part IV, §17) ensures that concurrent accesses to shared memory are properly synchronized or provably non-conflicting.
>
> 4. **Move semantics correctness**: Programs cannot use values after they have been moved. The ownership system (Part IV, §16) and lexical permission system (Part IV, §17) enforce move semantics statically.
>
> These guarantees **MAY** be violated within `unsafe` blocks or when calling foreign functions, where correctness depends on programmer-supplied contracts and external verification (§6.1.1, §6.2.2).
>
> Implementations **MUST** document any language constructs that introduce UVB or rely on external verification in their conformance dossier (§6.2.4).

### 6.3 Binary Compatibility [conformance.abi]

This section defines requirements for binary compatibility and ABI stability across language versions, compilation modes, and implementations.

#### 6.3.1 ABI Stability Requirements [conformance.abi.stability]

> Implementations **MUST NOT** guarantee binary compatibility across different language versions (including minor versions), different compiler versions, different compilation modes, or different optimization levels unless explicitly documented in the conformance dossier (§6.2.4).
>
> Implementations **MAY** provide ABI stability guarantees within a documented scope (e.g., within a single compiler major version, within a single language major version, or across specified compilation modes). Such guarantees **MUST** be documented in the conformance dossier.
>
> Source-level compatibility **MUST** be maintained for programs across minor language versions within the same major version, subject to deprecation timelines (§8.2).

#### 6.3.2 ABI Breaking Changes [conformance.abi.breaking]

> Changes that alter type layout, calling conventions, symbol mangling, or vtable structure **MUST** be classified as ABI-breaking changes.
>
> ABI-breaking changes **MUST** be documented in release notes and conformance dossiers with migration guidance.
>
> Implementations **SHOULD** provide tooling to detect ABI incompatibilities between compilation units.

#### 6.3.3 Cross-Version Linkage [conformance.abi.linkage]

> Implementations **MUST** diagnose attempts to link compilation units built with incompatible ABIs when such incompatibility is detectable.
>
> When ABI versioning is implemented, the ABI version **MUST** be encoded in symbols or metadata such that linkers can detect mismatches.
>
> Implementations **MAY** support multiple ABI versions concurrently through namespacing or versioned symbols, provided the versioning scheme is documented.

### 6.4 Implementation Limits [conformance.limits]

This section defines minimum guaranteed limits that conforming implementations must support and documentation requirements for implementation-specific limits.

#### 6.4.1 Minimum Guaranteed Limits [conformance.limits.minimum]

> Implementations **MUST** support at least the following minimum limits. Programs within these limits **MUST** be accepted and processed correctly. Exceeding these limits **MAY** trigger diagnostics or result in ill-formed programs (IFNDR, §6.1.3.1).
>
> **Source text limits:**
>
> - Source file size: 1 MiB (1,048,576 bytes)
> - Identifier length: 1,023 characters
> - String literal length: 65,535 characters (64 KiB)
>
> **Syntactic nesting limits:**
>
> - Block nesting depth: 256 levels
> - Expression nesting depth: 256 levels
> - Type nesting depth: 128 levels
> - Delimiter nesting depth: 256 levels
>
> **Declaration limits:**
>
> - Procedure parameters: 255 parameters
> - Record fields: 1,024 fields
> - Enum variants: 1,024 variants
> - Modal states: 64 states per modal type
> - Generic parameters: 256 parameters per entity
>
> **Compile-time evaluation limits:**
>
> - Comptime recursion depth: 256 stack frames
> - Comptime evaluation steps: 1,000,000 steps per comptime block
> - Comptime memory allocation: 64 MiB per compilation unit
> - Comptime string size: 1 MiB per string value
> - Comptime collection cardinality: 10,000 elements per collection
>
> **Name resolution limits:**
>
> - Qualified name chain length: 32 components
> - Scope depth: 256 nested scopes
>
> **Generic and instantiation limits:**
>
> - Monomorphization instantiations: 1,024 distinct instantiations per generic entity
> - Generic instantiation nesting: 32 levels of nested instantiation
> - Type unification complexity: 1,024 unification variables per inference problem
>
> Implementations **MAY** exceed these minimums. Implementations **SHOULD** document their actual limits in the conformance dossier (§6.2.4).

#### 6.4.2 Limit Documentation Requirements [conformance.limits.documentation]

> Implementations **MUST** document the following limits in their conformance dossier:
>
> 1. All limits where the implementation's actual limit differs from the minimum guaranteed limit
> 2. Platform-dependent limits including stack size, heap allocation limits, and thread concurrency limits
> 3. Whether limits are configurable via compiler flags, environment variables, or runtime settings
> 4. The diagnostic codes issued when specific limits are exceeded
>
> Implementations **SHOULD** provide diagnostic warnings when programs approach limits (e.g., at 75% or 90% of capacity) to enable proactive refactoring.

#### 6.4.3 Exceeding Limits [conformance.limits.exceeding]

> When a program exceeds an implementation limit:
>
> 1. For limits with specified diagnostic codes (per Appendix B), implementations **MUST** issue the specified diagnostic.
> 2. For limits without specified diagnostics, implementations **MUST** issue an error diagnostic indicating which limit was exceeded.
> 3. Implementations **MUST NOT** produce executable artifacts for programs exceeding limits.
>
> Programs that exceed the minimum guaranteed limits (§6.4.1) but remain within an implementation's actual limits **MAY** be conforming for that implementation but are not portable across all conforming implementations.

### 6.5 Reserved Identifiers [conformance.reserved]

This section defines identifiers reserved by the language and implementations, establishing the boundary between user code and implementation space.

#### 6.5.1 Reserved Keywords [conformance.reserved.keywords]

> The following identifiers are reserved as keywords and **MUST NOT** be used as user-defined identifiers:
>
> Keywords are defined by the lexical grammar in Part II, §9. Using a reserved keyword as an identifier **MUST** trigger diagnostic code E-SRC-0305.
>
> The complete list of reserved keywords is specified in the normative grammar (Appendix A). Implementations **MUST** reserve all keywords defined in the grammar for the target language version.

#### 6.5.2 Reserved Namespaces [conformance.reserved.namespaces]

> The following namespace prefixes are reserved for specification-defined features and **MUST NOT** be used by user programs or vendor extensions:
>
> 1. The `cursive.*` namespace prefix (all identifiers beginning with `cursive.`)
> 2. Non-namespaced feature flag identifiers (feature flags without vendor domain notation)
> 3. Non-namespaced attribute identifiers (attributes without vendor domain notation)
>
> User programs using reserved namespace prefixes **MUST** trigger diagnostics. Module path components using reserved keywords **MUST** trigger diagnostic E-SRC-1103.
>
> Vendor extensions **MUST** use namespaced identifiers as specified in §8.3.3.

#### 6.5.3 Implementation Reservations [conformance.reserved.implementation]

> Implementations **MAY** reserve additional identifier patterns for internal use, provided such reservations are documented in the conformance dossier (§6.2.4).
>
> Common implementation reservation patterns include:
>
> 1. Identifiers beginning with double underscore (`__`)
> 2. Identifiers beginning with underscore followed by uppercase letter (`_[A-Z]`)
> 3. Vendor-specific namespace prefixes (e.g., `vendor.com.*`)
>
> Implementations **SHOULD** issue warnings when user programs use implementation-reserved patterns. Conforming programs **SHOULD NOT** use patterns that match common implementation reservations for maximum portability.

### 6.6 Conformance Verification [conformance.verification]

This section defines requirements for conformance test suites, certification processes, and conformance claims that enable validation of implementation and program conformance.

#### 6.6.1 Test Suite Structure [conformance.verification.structure]

> A normative conformance test suite **SHOULD** be maintained alongside this specification to verify implementation conformance.
>
> The test suite **MUST** be organized by specification part and chapter, with tests clearly labeled by the requirement they verify (using section anchors, e.g., `[conformance.obligations.implementations]`).
>
> Each test **MUST** include:
>
> 1. A description of the requirement being tested.
> 2. Source code demonstrating the tested construct.
> 3. Expected behavior (successful compilation, specific diagnostic, runtime outcome).
> 4. Any necessary environment or configuration requirements.

#### 6.6.2 Test Categories [conformance.verification.categories]

> Conformance tests **MUST** be categorized as:
>
> 1. **Positive tests**: Well-formed programs that implementations MUST accept and execute according to specified semantics.
> 2. **Negative tests**: Ill-formed programs that implementations MUST reject with appropriate diagnostics (§7), each verifying a specific error condition.
> 3. **Behavioral tests**: Programs verifying runtime semantics, memory safety properties, or concurrency guarantees.
>
> Negative tests **MUST** specify the expected diagnostic code(s) (§7.2). Implementations passing a negative test **MUST** issue at least one of the expected diagnostics.
>
> Test coverage **SHOULD** include at least one positive and one negative test for every normative MUST/MUST NOT requirement in this specification.

#### 6.6.3 Certification Process [conformance.verification.certification]

> Implementations **MAY** seek conformance certification by demonstrating that they pass the normative test suite and satisfy all conformance obligations (§6.2).
>
> Certification **MUST** be version-specific, indicating which specification version the implementation conforms to. Implementations **MUST** provide a complete conformance dossier (§6.2.4) as part of certification.
>
> Certified implementations **MUST** document any test suite exclusions (e.g., for partial conformance per §6.6.5) with justifications for why specific tests cannot be satisfied.

#### 6.6.4 Conformance Claims [conformance.verification.claims]

> Conformance claims **MUST** include:
>
> 1. Specification version conformed to (§8.1.1).
> 2. Implementation name and version.
> 3. Supported target platforms and their conformance dossiers.
> 4. Test suite version used and test results summary (pass/fail/excluded counts).
> 5. Any partial conformance declarations (§6.6.5).
>
> Conformance claims **MUST** be publicly accessible and **SHOULD** be machine-readable (JSON or TOML format preferred).

#### 6.6.5 Partial Conformance [conformance.verification.partial]

> Implementations targeting embedded systems, restricted environments, or specialized domains **MAY** claim partial conformance by explicitly documenting which specification requirements they do not satisfy.
>
> Partial conformance claims **MUST** enumerate:
>
> 1. Unsupported language features with specification section references.
> 2. Unsupported standard library components (if core library requirements exist).
> 3. Reduced implementation limits below the minimums specified in §6.4.1.
> 4. Rationale for each exclusion (platform constraints, domain requirements).
>
> Partial conformance implementations **MUST** still satisfy all conformance obligations for the subset of the language they support. Features not supported **MUST** be diagnosed with appropriate error codes.

---

## 7. Language Evolution and Governance [evolution]

This chapter defines how the Cursive language evolves over time, including versioning policy, feature lifecycle management, extension mechanisms, and specification maintenance. It establishes the governance framework that ensures stability, predictability, and controlled evolution for implementations and programs.

### 7.1 Diagnostic Requirements [diagnostics.requirements]

This section establishes the fundamental obligations for diagnostic reporting in conforming implementations.

#### 7.1.1 Obligation to Diagnose [diagnostics.requirements.obligation]

> Implementations **MUST** issue at least one diagnostic for every ill-formed program as defined in §6.2.3, except for IFNDR violations (§6.1.3.1).
>
> Implementations **MUST NOT** produce executable artifacts for programs that trigger error diagnostics (severity E, see §7.3.1).
>
> Implementations **MAY** produce non-executable artifacts (syntax trees, symbol tables, diagnostic metadata) for tooling purposes regardless of program well-formedness.

**_Example - Diagnostic obligation:_**

```cursive
procedure example() {
    let x: i32 = "hello";  // error[E-TYP-0201]: type mismatch
                           // expected i32, found &str
}
// Implementation MUST issue diagnostic, MUST NOT produce executable
```

#### 7.1.2 Minimum Diagnostic Content [diagnostics.requirements.content]

> Every diagnostic **MUST** include:
>
> 1. A diagnostic code following the format specified in §7.2.1
> 2. A severity level as defined in §7.3
> 3. Source location information: file path, line number, and column number
> 4. A description of the violation sufficient to understand the error
>
> Implementations **MAY** provide additional information including but not limited to: supplementary notes, related source locations, type information, suggested corrections, and structured metadata for tooling integration.

**_Example - Minimum diagnostic content:_**

```
E-MEM-0102 [error] at src/main.cursive:15:9
Cannot move value: x is not a responsible binding

// Required elements present:
// - Code: E-MEM-0102
// - Severity: error
// - Location: src/main.cursive:15:9
// - Description: Cannot move value: x is not a responsible binding
```

#### 7.1.3 Diagnostic Timing [diagnostics.requirements.timing]

> Implementations **MUST** issue diagnostics during translation of each compilation unit, before executing any code that depends on that unit.
>
> For ahead-of-time compilation, this means before program execution begins. For JIT compilation, this means before executing each newly translated unit. For incremental or on-demand compilation, this means before the first execution of code requiring the newly compiled unit.
>
> **Invariant:** No ill-formed code may execute. Implementations **MUST** reject ill-formed units before any execution that depends on them.
>
> Implementations **MAY** issue multiple diagnostics for a single translation unit.
>
> When multiple violations exist, implementations **SHOULD** issue diagnostics for independent violations but **MAY** suppress cascading diagnostics that result from a primary error.

**_Explanation:_**
Cascading diagnostics are secondary errors caused by a primary error; suppression focuses on root causes.

**_Example - Cascading diagnostic suppression:_**

```cursive
procedure example() {
    let x = undefined_function();  // E-SRC-1601: undefined_function not found

    let y: i32 = x + 5;            // Implementation MAY suppress type error here
                                   // because x's type is unknown due to primary error
}
```

### 7.2 Diagnostic Code System [diagnostics.codes]

This section defines the canonical format and organization of diagnostic codes that provide stable identifiers for each diagnostic across implementations and specification versions.

#### 7.2.1 Canonical Format [diagnostics.codes.format]

> Diagnostic codes **MUST** follow the format `K-CAT-FFNN` where:
>
> - `K` is a single character severity indicator: `E` (error), `W` (warning), or `N` (note)
> - `CAT` is a three-letter category code in uppercase (see §7.2.2)
> - `FF` is a two-digit feature bucket in decimal notation (00-99)
> - `NN` is a two-digit sequence number within the bucket (00-99)
> - Separators **MUST** be ASCII hyphen-minus (U+002D)
>
> **_Syntax:_**
> ```ebnf
> diagnostic_code ::= severity "-" category "-" bucket sequence
> severity        ::= "E" | "W" | "N"
> category        ::= [A-Z] [A-Z] [A-Z]
> bucket          ::= [0-9] [0-9]
> sequence        ::= [0-9] [0-9]
> ```


**_Example - Diagnostic code structure:_**

```
E-SRC-1103: Error in Source category, bucket 11 (modules), sequence 03
W-CNF-0201: Warning in Conformance category, bucket 02 (modes), sequence 01
N-TYP-0305: Note in Type category, bucket 03 (generics), sequence 05
```

#### 7.2.2 Category Codes [diagnostics.codes.categories]

> The following category codes are defined:
>
> | Code  | Category                               |
> |-------|----------------------------------------|
> | `SRC` | Source text, lexical, translation      |
> | `TYP` | Type system and type checking          |
> | `MEM` | Memory model, permissions, regions     |
> | `EXP` | Expressions, statements, control flow  |
> | `GRN` | Grants, contracts, effects             |
> | `FFI` | Foreign function interface             |
> | `DIA` | Diagnostic system meta-diagnostics     |
> | `CNF` | Conformance, extensions, versioning    |
>
> Additional categories **MAY** be defined in future specification versions. Implementations **MUST NOT** define custom categories using the three-letter format reserved for specification-defined codes.

**_Explanation:_**
The prohibition on custom categories prevents conflicts with future specification versions.

#### 7.2.3 Feature Bucket Allocation [diagnostics.codes.buckets]

> Feature buckets within each category **MUST** be allocated according to the taxonomy defined in Appendix B.
>
> Each feature bucket corresponds to a specific functional area within its category. Buckets **MUST** contain logically related diagnostics.
>
> Diagnostic codes **MUST NOT** be reused or repurposed across specification versions. When diagnostics are removed or superseded, their codes **MUST** be marked as reserved in Appendix D.

**_Explanation:_**
For example, the `SRC` category uses bucket 01 for source encoding, bucket 11 for module discovery, and bucket 12 for imports. Code stability ensures that documentation, tooling, and error tracking systems remain valid across specification versions. Reserved codes prevent confusion when old diagnostics are deprecated.

**_Example - Feature bucket organization for SRC category:_**

```
SRC category (Part II):
  Bucket 01: Source ingestion (E-SRC-0101 through E-SRC-0199)
  Bucket 02: Translation pipeline (E-SRC-0201 through E-SRC-0299)
  Bucket 03: Lexical tokens (E-SRC-0301 through E-SRC-0399)
  Bucket 11: Module discovery (E-SRC-1101 through E-SRC-1199)
  Bucket 12: Imports and aliases (E-SRC-1201 through E-SRC-1299)
  [See Appendix B for complete taxonomy]
```

#### 7.2.4 Diagnostic Code Stability [diagnostics.codes.stability]

> Once a diagnostic code is published in a specification version, its meaning **MUST NOT** change in subsequent versions.
>
> Deprecated diagnostics **MUST** be marked in Appendix D with their deprecation version and replacement code (if any).
>
> Implementations **MAY** continue to support deprecated diagnostic codes for backward compatibility but **SHOULD** also emit the replacement code when applicable.

**_Explanation:_**
Code stability ensures that error tracking systems, documentation, and automated tools remain reliable across specification updates. Deprecation marking preserves historical context while guiding users toward current diagnostics.

### 7.3 Severity Levels [diagnostics.severity]

This section defines the three severity levels that classify diagnostic importance and determine whether translation may proceed.

#### 7.3.1 Error Diagnostics [diagnostics.severity.error]

> Error diagnostics (severity indicator `E`) **MUST** be issued when a program violates a normative requirement expressed with **MUST** or **MUST NOT**.
>
> Implementations **MUST** reject programs that trigger error diagnostics by not producing executable artifacts (§7.1.1).
>
> Error diagnostics indicate ill-formed programs that do not conform to this specification (§6.2.2).

**_Formal rule:_**

$$
\frac{
  \text{violates}(P, R) \land \text{normative}(R) \land \neg\text{IFNDR}(R)
}{
  \text{issue}(\text{Error}, \text{code}(R))
}
\tag{Diag-Error}
$$


**_Example - Error diagnostic:_**

```cursive
record Point { x: i32, y: i32 }

procedure example() {
    let p = Point { x: 10 };  // error[E-TYP-0201]: missing field y
                               // record Point requires field y: i32
}
```

#### 7.3.2 Warning Diagnostics [diagnostics.severity.warning]

> Warning diagnostics (severity indicator `W`) **MAY** be issued for constructs that are well-formed according to this specification but potentially problematic.
>
> Implementations **MUST** continue translation when only warnings are present and **MAY** produce executable artifacts.
>
> Programs that trigger only warnings (no errors) are conforming (§6.2.2).

**_Explanation:_**
Examples include unused variables, unreachable code, or patterns that commonly indicate logic errors.

**_Example - Warning diagnostic:_**

```cursive
procedure example() {
    let x: i32 = 42;         // warning[W-EXP-0105]: unused binding x
                             // consider removing or prefixing with underscore

    return;
}
```

#### 7.3.3 Note Diagnostics [diagnostics.severity.note]

> Note diagnostics (severity indicator `N`) provide supplementary context for errors or warnings.
>
> Notes **MUST NOT** appear without an associated error or warning diagnostic.
>
> Notes **MAY** reference related source locations, provide type information, explain why a construct is invalid, or suggest corrections.

**_Explanation:_**
For example, when an error reports a type mismatch, a note might show where the conflicting type was defined or inferred. Notes do not increase the count of primary issues.

**_Example - Note diagnostic providing context:_**

```cursive
procedure takes_string(s: &str) { }

procedure example() {
    let x: i32 = 42;
    takes_string(x);  // error[E-TYP-0301]: type mismatch in argument 1
                      //   expected &str, found i32
                      // note[N-TYP-0302]: parameter s declared here (line 1)
                      // note[N-TYP-0303]: binding x has type i32 (line 4)
}
```

### 7.4 IFNDR and Diagnostic Limits [diagnostics.ifndr]

This section addresses cases where diagnostic requirements are relaxed due to computational infeasibility.

#### 7.4.1 Ill-Formed, No Diagnostic Required [diagnostics.ifndr.definition]

> When detecting a violation requires solving computationally infeasible problems as defined in §6.1.3.1, implementations are not required to issue diagnostics.
>
> Programs containing undetected IFNDR violations remain ill-formed and non-conforming (§6.2.2).
>
> The behavior of programs with IFNDR violations is unspecified but **MUST NOT** include UVB (§6.1.1).

**_Explanation:_**
IFNDR acknowledges that certain language violations are undecidable (halting problem, Rice's theorem) or NP-complete to detect. IFNDR differs from UVB: IFNDR programs are ill-formed (violate a rule), while UVB operations are well-formed but depend on unverifiable runtime properties.

**_Example - IFNDR due to halting problem:_**

```cursive
procedure complex_computation(n: i64) -> i64 {
    // ... Turing-complete computation ...
}

procedure example(x: i64) {
    match complex_computation(x) {
        0 => handle_zero(),
        _ => handle_nonzero(),
    }
    // If complex_computation(x) can return values other than 0,
    // but detecting this requires solving the halting problem,
    // the match may be non-exhaustive but IFNDR applies
}
```

#### 7.4.2 IFNDR Diagnostic Encouragement [diagnostics.ifndr.encouragement]

> Implementations **SHOULD** diagnose IFNDR violations when detection is feasible within reasonable resource limits.
>
> Implementations **MAY** issue warnings for patterns that commonly indicate IFNDR violations even when definitive detection is infeasible.
>
> Implementations **MAY** provide configuration options to control the effort expended on IFNDR detection (analysis depth, timeout limits, complexity thresholds).

**_Explanation:_**
While implementations are not required to detect IFNDR violations, doing so when practical improves program quality. Warning about suspicious patterns (e.g., likely non-exhaustive matches, possible infinite loops) helps developers even when certainty is impossible. Resource-limited analysis trades precision for feasibility.

**_Example - IFNDR diagnostic encouragement:_**

```cursive
// Implementation MAY warn about potentially infinite loop
procedure unclear_termination(n: i32) {
    var x = n;
    while complex_condition(x) {  // warning[W-EXP-0210]: loop may not terminate
        x = complex_update(x);    //   unable to prove termination statically
    }
}
```

### 7.5 Implementation-Defined Behavior [diagnostics.idb]

This section specifies aspects of diagnostic reporting where implementations have discretion within conformance constraints.

#### 7.5.1 Diagnostic Presentation [diagnostics.idb.presentation]

> The presentation format of diagnostics is implementation-defined. Implementations **MAY** choose:
>
> 1. Text formatting (color, Unicode characters, ASCII fallback)
> 2. Source context window size (lines before and after error location)
> 3. Message wording (provided the description remains accurate and clear)
> 4. Additional information beyond minimum requirements (§7.1.2)
>
> Implementations that support machine-readable diagnostic output **MUST** document the output format in their conformance dossier (§6.2.4).

**_Explanation:_**
Presentation format flexibility allows implementations to adapt to different terminal capabilities, user preferences, and tooling requirements. However, the core information (code, severity, location, description) remains mandatory. Machine-readable format documentation enables tooling integration.

**_Example - Implementation-defined presentation variations:_**

```
// Implementation A: Color, Unicode, detailed context
error[E-TYP-0201]: type mismatch
  ┌─ src/main.cursive:10:18
  │
10│     let x: i32 = "hello";
  │                  ^^^^^^^ expected i32, found String@View
  │
  = note: string literals have type String@View

// Implementation B: ASCII, minimal context
E-TYP-0201 [error] src/main.cursive:10:18
  type mismatch: expected i32, found String@View

// Both conforming: contain required elements (code, severity, location, description)
```

#### 7.5.2 Additional Diagnostics [diagnostics.idb.additional]

> Implementations **MAY** issue diagnostics beyond those required by this specification, including:
>
> 1. Code quality warnings (style, complexity, performance)
> 2. Best practice recommendations
> 3. Platform-specific considerations
> 4. Deprecation notices for implementation-specific features
>
> Additional diagnostics **SHOULD** use vendor-specific code prefixes (not matching `K-CAT-FFNN`) to distinguish them from specification-defined codes.
>
> Implementations **SHOULD** provide mechanisms to selectively enable or disable additional diagnostics (warning levels, diagnostic groups, individual code suppression).

**_Explanation:_**
Additional diagnostics improve code quality without mandating specific coding styles or practices. Vendor-specific prefixes prevent confusion with specification codes and allow implementations to experiment with new diagnostics.

**_Example - Additional implementation diagnostics:_**

```cursive
// Specification-required diagnostic
let x: i32 = 5;
x = "hello";  // error[E-TYP-0301]: type mismatch in assignment

// Implementation-specific additional diagnostics (hypothetical)
let y = 42;   // impl-warn[complexity-001]: magic number
              // impl-note: consider defining a named constant

procedure long_function() {  // impl-warn[style-042]: function exceeds 200 lines
    // ... 250 lines ...      // impl-note: consider breaking into smaller functions
}
```

### 7.6 Diagnostic Catalog Reference [diagnostics.catalog]

This section provides references to the comprehensive diagnostic catalog and related appendices.

#### 7.6.1 Complete Diagnostic Catalog [diagnostics.catalog.reference]

> The complete catalog of specification-defined diagnostic codes appears in Appendix D. The catalog includes:
>
> 1. Diagnostic code and severity
> 2. Specification section reference
> 3. Brief description of the violation
> 4. Example triggering code (for selected diagnostics)
>
> The catalog is normative: implementations **MUST** use the specified codes for the specified violations.

**_Explanation:_**
Appendix B serves as the authoritative reference for all diagnostic codes, enabling implementers to ensure coverage and users to look up diagnostic meanings. The normative status ensures consistency across implementations.

#### 7.6.2 Diagnostic Taxonomy [diagnostics.catalog.taxonomy]

> The feature bucket taxonomy organizing diagnostics within categories appears in Appendix B. The taxonomy specifies:
>
> 1. Bucket number ranges for each category
> 2. Functional area corresponding to each bucket
> 3. Allocation strategy for future diagnostics
>
> The taxonomy is normative: new diagnostics **MUST** be assigned to appropriate buckets according to the allocation strategy.

**_Explanation:_**
Appendix B provides the organizational structure that keeps diagnostic codes stable and predictable as the language evolves. Following the allocation strategy prevents conflicts and maintains the hierarchical organization.

#### 7.6.3 Implementation-Defined Behavior Index [diagnostics.catalog.idb]

> Implementation-defined aspects of diagnostic reporting are indexed in Appendix B. The index references:
>
> 1. Diagnostic presentation format choices (§7.5.1)
> 2. Machine-readable output format documentation requirements
> 3. Additional diagnostic policies
>
> Implementations **MUST** document their choices for items listed in Appendix B as part of their conformance dossier (§6.2.4).

**_Explanation:_**
Appendix C identifies which aspects of diagnostic reporting are implementation-defined, guiding implementers on what must be documented and helping users understand variation across implementations.

## 8. Language Evolution and Governance [evolution]

This chapter defines how the Cursive language evolves over time, including versioning policy, feature lifecycle management, extension mechanisms, conformance verification, and specification maintenance. It establishes the governance framework that ensures stability, predictability, and controlled evolution for implementations and programs.

### 8.1 Versioning Model [evolution.versioning]

This section defines the version numbering scheme, edition system, and version declaration requirements for the Cursive language.

#### 8.1.1 Semantic Versioning Policy [evolution.versioning.policy]

> The Cursive language specification **MUST** follow semantic versioning with the format `MAJOR.MINOR.PATCH` where:
>
> - **MAJOR** version increments for incompatible changes that break existing conforming programs or change language semantics, including new keywords that were previously valid identifiers, changes to type system rules, or modifications to evaluation order.
> - **MINOR** version increments for backwards-compatible additions and clarifications that do not change the meaning of existing conforming programs, including new language features with new syntax, clarifications that resolve ambiguity without changing behavior, or additional diagnostic requirements.
> - **PATCH** version increments for backwards-compatible fixes and editorial corrections, including typo corrections, improved wording, formatting changes, or correction of internal cross-references.
>
> Implementations **MUST** document which language specification version(s) they support in their conformance dossier (§6.2.4).

#### 8.1.2 Language Editions [evolution.versioning.editions]

> Language editions **MAY** be introduced to manage incompatible changes while allowing programs written for earlier editions to remain compilable.
>
> When editions are supported, implementations **MUST** allow programs to explicitly declare their target edition. Each edition **MUST** maintain internal consistency and **MUST NOT** mix semantics from different editions within a single compilation unit.
>
> Implementations supporting multiple editions **MUST** document the semantic differences between editions and provide migration guidance in their conformance dossier.

#### 8.1.3 Version Declaration [evolution.versioning.declaration]

> Programs **MUST** declare their target language version in the project manifest using the `language.version` field as specified in Part II, §10.1.5.
>
> The declared version **MUST** use semantic versioning format. Implementations **MUST** reject manifests where the declared MAJOR version does not match the implementation's supported MAJOR version.
>
> Implementations **MAY** accept programs declaring older MINOR versions within the same MAJOR version, provided backward compatibility is maintained per §8.1.4.

#### 8.1.4 Compatibility Guarantees [evolution.versioning.compatibility]

> Implementations **MUST** maintain source-level backward compatibility for conforming programs across MINOR version increments within the same MAJOR version, subject to deprecation timelines (§8.2.4).
>
> Implementations **MUST NOT** silently change the semantics of existing conforming programs when processing them under a newer MINOR version. Changes requiring semantic alterations **MUST** trigger MAJOR version increments.
>
> Implementations **MAY** issue warnings for constructs that will be deprecated in future versions, provided such warnings do not prevent successful compilation.

#### 8.1.5 Version Migration [evolution.versioning.migration]

> When MAJOR version increments introduce breaking changes, implementations **SHOULD** provide automated migration tooling to assist in updating programs from the previous MAJOR version.
>
> Migration tooling **SHOULD** handle syntactic transformations, deprecated feature replacements, and semantic updates where feasible. Migration tooling **MUST NOT** introduce silent behavioral changes without user confirmation.
>
> Implementations **MUST** document migration procedures, including cases requiring manual intervention, in release notes and migration guides.

### 8.2 Feature Lifecycle [evolution.lifecycle]

This section defines stability classifications for language features, deprecation policies, and the process for feature removal.

#### 8.2.1 Stability Classes [evolution.lifecycle.stability]

> Every language feature **MUST** be classified into one of three stability classes:
>
> 1. **Stable**: Features available by default without opt-in. Stable features **MUST NOT** introduce breaking changes except in MAJOR version increments. Stable features **MUST** be documented in this specification.
>
> 2. **Preview**: Features intended for the next stable release, requiring explicit opt-in via feature flags (§8.3.2). Preview features **MAY** change between MINOR versions. Preview features **MUST** be documented with a `[Preview: target_version]` annotation indicating the version in which they are planned to stabilize.
>
> 3. **Experimental**: Highly unstable features guarded by feature flags. Experimental features **MAY** change significantly or be removed without following deprecation timelines. Experimental features **MUST NOT** be used in production code without explicit acknowledgment of stability risks.
>
> The stability class of each feature **MUST** be documented in this specification or in implementation-specific extension documentation.

#### 8.2.2 Feature Stabilization [evolution.lifecycle.stabilization]

> Preview features **MAY** transition to Stable status in a MINOR version increment after the preview period, provided no breaking changes to the preview semantics are required.
>
> If breaking changes are required during stabilization, the feature **MUST** either undergo another preview cycle or be deferred to the next MAJOR version.
>
> Experimental features **MUST** transition through Preview status before becoming Stable, except for features introduced in MAJOR version increments where the preview period may be shortened or bypassed.

#### 8.2.3 Deprecation Policy [evolution.lifecycle.deprecation]

> Features **MAY** be deprecated when better alternatives exist, when they introduce maintenance burden, or when they conflict with language design principles.
>
> Deprecated features **MUST** remain functional and conforming for at least one MINOR version after deprecation is announced. Deprecated features **MUST** be marked with the `[!caution] Deprecated` annotation as specified in §2.2, including the version in which they will be removed.
>
> Implementations **SHOULD** issue warnings (severity `W`, §7.3.2) when deprecated features are used, referencing the deprecation diagnostic code and replacement guidance.

#### 8.2.4 Deprecation Timeline [evolution.lifecycle.timeline]

> Features deprecated in version `X.Y.0` **MUST NOT** be removed before version `X.(Y+2).0` for MINOR version deprecations, or version `(X+1).0.0` for MAJOR version deprecations.
>
> The minimum deprecation period **MUST** be at least one full MINOR version release to provide users time to migrate.
>
> Implementations **MUST** document all deprecated features in release notes, including the deprecation version, planned removal version, rationale, and migration path.

#### 8.2.5 Feature Removal [evolution.lifecycle.removal]

> Features **MAY** be removed only in MAJOR version increments, except for experimental features which **MAY** be removed in any version.
>
> Removed features **MUST** trigger error diagnostics (severity `E`, §7.3.1) when encountered. The diagnostic code for the removed feature **MUST** be marked as reserved in Appendix D and **MUST NOT** be reused for different violations (§7.2.4).
>
> Implementations **MAY** provide compatibility modes or feature flags to temporarily restore removed features, provided such modes are clearly documented as non-conforming extensions.

#### 8.2.6 Migration Paths [evolution.lifecycle.migration]

> For every deprecated or removed feature, implementations **MUST** provide documentation describing the recommended replacement or migration strategy.
>
> Migration documentation **MUST** include code examples demonstrating the deprecated construct and its replacement. Where automated migration is feasible, implementations **SHOULD** provide tooling support.
>
> Deprecation warnings (§8.2.3) **SHOULD** include brief inline migration guidance or references to detailed migration documentation.

#### 8.2.7 Critical Security Deprecation [evolution.lifecycle.security]

> Features with discovered security vulnerabilities or fundamental safety defects **MAY** follow an expedited deprecation timeline.
>
> Security deprecations **MAY** be announced and removed within a single MINOR version increment, provided the vulnerability and rationale are publicly documented.
>
> Implementations **MUST** issue error diagnostics for security-deprecated features unless an explicit opt-out flag is provided. Such opt-out flags **MUST** be documented as enabling potentially unsafe behavior.

### 8.3 Extension System [evolution.extensions]

This section defines how implementations may extend the language beyond the core specification while preserving portability and conformance.

#### 8.3.1 Extension Mechanisms [evolution.extensions.mechanisms]

> Implementations **MAY** provide language extensions through:
>
> 1. **Attributes**: Syntactic annotations using the `[[attribute_name]]` syntax that modify declarations without altering core language semantics (see Part VIII for reflection and attribute requirements).
> 2. **Compiler-specific features**: Additional language constructs, built-in types, or intrinsic functions not defined in this specification.
> 3. **Preview features**: Specification-defined features in preview stability class (§8.2.1) requiring explicit opt-in.
>
> Extensions **MUST NOT** alter the meaning of conforming programs written without the extension. Extensions **MUST NOT** suppress required diagnostics defined in this specification.

#### 8.3.2 Feature Flags [evolution.extensions.flags]

> Extensions requiring opt-in **MUST** be controlled through feature flags declared in the project manifest or via compiler command-line options.
>
> Feature flags **MUST** use a hierarchical dotted identifier format (e.g., `"vendor.category.feature"`). Flags for specification-defined preview features **MUST** use identifiers without vendor prefixes.
>
> The manifest schema **MUST** support a `[features]` table with an `enable` array listing feature flag identifiers. Implementations **MUST** document all available feature flags and their effects in the conformance dossier (§6.2.4).
>
> Unknown feature flag identifiers **SHOULD** trigger warnings (diagnostic code `W-CNF-0101`) unless overridden by explicit compiler flags.

#### 8.3.3 Extension Namespacing [evolution.extensions.namespacing]

> Vendor-specific extensions **MUST** use namespaced identifiers to prevent conflicts with specification-defined features and other vendors' extensions.
>
> Vendor namespaces **MUST** follow the format `vendor.domain` where `vendor` is a unique vendor identifier and `domain` is a reverse-domain notation (e.g., `"example.com.advanced_features"`).
>
> Attribute names for vendor extensions **MUST** include the vendor namespace (e.g., `[[vendor.com.custom_attribute]]`). Feature flag identifiers for vendor extensions **MUST** similarly include the namespace.
>
> The specification reserves all non-namespaced identifiers and identifiers under the `cursive.*` namespace for specification-defined features. Implementations **MUST NOT** define extensions using reserved namespaces.

#### 8.3.4 Extension Conformance [evolution.extensions.conformance]

> Programs using specification-defined preview or experimental features (§8.2.1) **MAY** be conforming provided:
>
> 1. The features are explicitly enabled via feature flags (§8.3.2).
> 2. The program satisfies all requirements for those features as documented.
> 3. The program does not rely on undocumented behavior.
>
> Programs using vendor-specific extensions **MAY** be considered conforming within the vendor's ecosystem, provided the vendor documents the extension's behavior in a conformance dossier following the same structure as §6.2.4.
>
> Conformance claims for programs using extensions **MUST** enumerate all enabled extensions and their versions.

#### 8.3.5 Extension Standardization [evolution.extensions.standardization]

> Successful vendor extensions **MAY** be considered for adoption into the core specification through the change proposal process (§8.4.2).
>
> Extensions considered for standardization **MUST** demonstrate:
>
> 1. Proven utility across multiple codebases or domains.
> 2. Implementation experience from at least one production-quality implementation.
> 3. Compatibility with language design principles (§cursive-language-design in project documentation).
> 4. Absence of conflicts with existing or planned specification features.
>
> Upon adoption, standardized features **MUST** transition through preview stability (§8.2.1) before becoming stable, unless introduced as part of a MAJOR version increment.

### 8.4 Specification Maintenance [evolution.specification]

This section defines processes for maintaining and evolving the specification itself through errata, change proposals, and breaking change management.

#### 8.4.1 Errata Process [evolution.specification.errata]

> Errors in the specification (typos, ambiguities, contradictions, omissions) **SHOULD** be reported through the specification's issue tracking system.
>
> Confirmed errata **MUST** be corrected in a PATCH version increment (§8.1.1). Errata corrections **MUST NOT** introduce behavioral changes; corrections requiring behavioral changes **MUST** be treated as MINOR or MAJOR changes per semantic versioning policy.
>
> All errata **MUST** be documented in a normative change log (planned for Appendix H) recording:
>
> 1. The version in which the error appeared.
> 2. The version in which it was corrected.
> 3. Description of the error and correction.
> 4. Affected section references.
>
> Implementations **SHOULD** adopt errata promptly. When implementation behavior diverges from corrected specification text, the divergence **MUST** be documented in the conformance dossier as implementation-defined behavior (§6.1.4).

#### 8.4.2 Change Proposal Process [evolution.specification.proposals]

> Substantive changes to the specification (new features, semantic modifications, requirement changes) **MUST** follow a formal proposal process.
>
> Change proposals **MUST** include:
>
> 1. **Motivation**: Problem being solved and rationale for the change.
> 2. **Specification changes**: Precise wording for additions/modifications with section references.
> 3. **Impact analysis**: Effect on existing conforming programs and implementations.
> 4. **Migration strategy**: For breaking changes, how existing code should be updated.
> 5. **Implementation experience**: Evidence from prototype implementations or production use where available.
>
> Proposals **SHOULD** undergo public review period before acceptance. Accepted proposals **MUST** be integrated into the next appropriate specification version per semantic versioning policy (§8.1.1).

#### 8.4.3 Breaking Change Policy [evolution.specification.breaking]

> Changes that invalidate previously conforming programs **MUST** be classified as breaking changes and **MUST** trigger MAJOR version increments (§8.1.1).
>
> Breaking changes **MUST** follow the deprecation timeline (§8.2.4) when feasible, giving users at least one MINOR version cycle to prepare.
>
> Breaking changes **MUST** be thoroughly documented with:
>
> 1. Rationale justifying the incompatibility.
> 2. Estimated impact on existing codebases.
> 3. Detailed migration guidance.
> 4. Alternatives considered and reasons for rejection.
>
> Breaking changes **SHOULD** be batched in MAJOR releases to minimize disruption. Gratuitous breaking changes without substantial benefits **SHOULD NOT** be accepted.

---

# Part II - Program Structure & Translation

## 8 Source Text & Translation Pipeline

## 9 Lexical Structure

## 10 Modules, Assemblies, and Projects

### 10.1 Folder-scoped modules

#### 10.1.1 Module discovery

Any folder containing at least one `.cursive` source file **MUST** be treated as a module, all files directly within it **MUST** belong to that module, and subfolders **MUST** be treated as child modules with `::`-separated names.

$$
\frac{assembly(name) ∈ Γ \quad root ∈ Roots \quad path(module) = root/segments}
{Γ ⊢ module(name::segments) ⇒ Γ, module(name::segments)}
\tag{WF-Module}
$$

#### 10.1.2 Compilation unit uniqueness

Each module **MUST** contain exactly one compilation unit per source file, and a source file **MUST NOT** contribute to multiple modules.

#### 10.1.3 Manifest requirements

Manifests **MUST** enumerate every source root with unique normalized paths. Missing manifests, undefined roots, or duplicates **MUST** be diagnosed (E-SRC-1101…E-SRC-1102).

#### 10.1.4 Path validity

Module path components **MUST** be valid identifiers and **MUST NOT** collide by case on case-insensitive platforms; violations raise E-SRC-1103/E-SRC-1104, and implementations **SHOULD** warn (W-SRC-1105) about potential collisions.

#### 10.1.5 Manifest format and schema

Every project **MUST** include a UTF‑8 manifest file named `Cursive.toml` at the project root. The manifest **MUST** follow TOML 1.0 syntax with the schema below; unknown keys **MUST** raise W-SRC-1106 while malformed structures **MUST** raise E-SRC-1106.

```toml
[language]
version = "1.0.0"            # REQUIRED. Matches the Part I version triplet.

[roots]                        # REQUIRED. Maps logical root names to relative paths.
app = "src"
tests = "tests"

[[assemblies]]                # At least one entry required.
name = "app"                  # REQUIRED, unique per manifest.
roots = ["app"]              # REQUIRED. Entries must reference [roots] keys.
deps = ["runtime"]           # OPTIONAL. Names of other assemblies in the same manifest.

[build]
conformance = "strict"       # OPTIONAL. Overrides default per [NDF_REF].

[features]
enable = ["net.socket.experimental"]   # OPTIONAL. Matches [NDF_REF] identifiers.
```

Semantic requirements (failures raise the listed diagnostics):

1. `language.version` **MUST** be a semantic-version string whose `MAJOR` matches the compiler (E-SRC-1107).

   $$
   \frac{parse_{semver}(v) = (M,m,p) \quad M = M_{compiler}}
   {Γ ⊢ manifest.language(version = v) ⇒ Γ}
   \tag{WF-Manifest-Language}
   $$

2. `[roots]` **MUST** contain at least one entry and each value **MUST** be a normalized relative path without `..` segments.

   $$
   \frac{\forall r ∈ Roots.\ normalized(r) ∧ no\_dotdot(r)}
   {Γ ⊢ manifest.roots(Roots) ⇒ Γ, roots(Roots)}
   \tag{WF-Manifest-Roots}
   $$

3. Each `[[assemblies]]` entry **MUST** reference only declared roots, **MUST** use unique `name` fields, and **MUST NOT** form dependency cycles (E-SRC-1108).

   $$
   \frac{Γ ⊢ manifest.roots(Roots) ⇒ Γ' \quad R \subseteq dom(Roots) \quad deps ⊆ declared\_assemblies \quad acyclic(deps)}
   {Γ' ⊢ assembly(name, R, deps) ⇒ Γ', assembly(name)}
   \tag{WF-Assembly}
   $$

4. The optional `[features].enable` array **MUST** contain only feature identifiers defined in [NDF_REF]; unknown identifiers raise W-CNF-0101 unless overridden by `--enable-feature`.
5. Additional manifest sections **MUST** be ignored only when they appear in a vendor namespace of the form `vendor."domain"` to preserve forward compatibility.

Failing any premise **MUST** trigger the diagnostics enumerated in [NDF_REF] (E/W-SRC-1101…1108).

### 10.2 Assemblies and projects

#### 10.2.1 Assembly definition

An assembly **MUST** be treated as a named collection of modules discovered under a manifest entry. Assemblies may reference each other but remain separately buildable units.

#### 10.2.2 Project definition

A project **MUST** consist of one or more assemblies linked, packaged, or distributed together, **MUST** declare which assemblies it includes, and **MUST** expose a target language version consistent with Part I.

#### 10.2.3 Discovery linkage

Module discovery within an assembly **MUST** follow the folder rules in [NDF_REF]; assemblies merely group discovered modules for distribution/import control.

### 10.3 Imports, aliases, and qualified visibility

#### 10.3.1 Qualified access

Implementations **MUST** make every declaration inside a module available via qualified names derived from folder structure, so intra-assembly access **MUST NOT** require extra keywords.

#### 10.3.2 Import forms

`import module_path` **MUST** precede references to modules in other assemblies or projects, while `import module_path as alias` **MAY** be used for any module to introduce a local alias for qualified access.

$$
\frac{Γ ⊢ module(p) ⇒ Γ' \quad module\_path(p)}
{Γ' ⊢ import\ module(p) ⇒ Γ'}
\tag{WF-Import}
$$

#### 10.3.3 Binding constraints

Imports **MUST NOT** introduce unqualified bindings; qualified references **MUST** be `module::item` or `alias::item`.

#### 10.3.4 Alias rules

Alias names **MUST** be unique within a module and **MUST NOT** be rebound (E-SRC-1201). Referencing undefined module paths **MUST** raise E-SRC-1202.

$$
\frac{Γ' ⊢ import\ module(p) ⇒ Γ' \quad alias \notin dom(aliases)}
{Γ' ⊢ import\ module(p)\ as\ alias ⇒ Γ', alias ↦ p}
\tag{WF-Import-Alias}
$$

#### 10.3.5 Import availability

Imports **MUST** appear somewhere in the module before dependent references, though textual order **MAY** vary because parsing inventories all imports up front.

#### 10.3.6 Re-export policy

Public declarations **MUST** enter the export set automatically, and re-exporting **MUST** occur explicitly via public declarations since `use`/`public use` are unavailable.

#### 10.3.7 Assembly policies

Assemblies **MAY** impose additional import policies, and violations **MUST** be diagnosed with `E-SRC-1203` so policy enforcement remains within the imports bucket and tooling can surface a stable, specific error.

#### 10.3.8 Module-path grammar

The grammar for `module_path` referenced in [NDF_REF]–[NDF_REF] **MUST** be:

```ebnf
module_path ::= identifier ("::" identifier)*
identifier  ::= lexical identifier defined in [NDF_REF]
```

Leading `::` tokens, empty components, and trailing separators **MUST** be rejected with E-SRC-1204. Module paths **MUST NOT** exceed 32 components (matching [NDF_REF]); longer paths raise E-SRC-1205. Implementations **MUST** normalize each component using the same Unicode normalization as [NDF_REF] before comparing paths so manifests, imports, and on-disk layouts stay consistent.

##### 10.3.8.1 Import inference rules

$$
\frac{Γ ⊢ module(p) ⇒ Γ' \quad module\_path(p)}
{Γ' ⊢ import\ module(p) ⇒ Γ'}
\tag{WF-Import}
$$

$$
\frac{Γ' ⊢ import\ module(p) ⇒ Γ' \quad alias \notin dom(aliases)}
{Γ' ⊢ import\ module(p)\ as\ alias ⇒ Γ', alias ↦ p}
\tag{WF-Import-Alias}
$$

Premise failures (unknown module, malformed path, duplicate alias) **MUST** raise the diagnostics listed in [NDF_REF] and [NDF_REF].

### 10.4 Dependency graph and initialization

#### 10.4.1 Graph construction

Implementations **MUST** build a module dependency graph with vertices as modules and edges arising from imports and module-scope initializers/comptime blocks referencing other modules’ exports.

#### 10.4.2 Eager vs. lazy edges

Edges **MUST** be marked eager when initialization reads bindings from the target; otherwise they are lazy.

#### 10.4.3 Eager subgraph constraints

The eager subgraph **MUST** be acyclic (E-SRC-1301), and initialization **MUST** run in topological order; consuming an uninitialized eager dependency **MUST** raise E-SRC-1302.

#### 10.4.4 Lazy semantics

Lazy edges **MAY** form cycles, and implementations **MAY** evaluate initializers lazily provided they execute at most once, preserve observable behavior, and maintain ordering.

#### 10.4.5 Failure propagation

If a module’s initialization fails, every module reachable via eager edges **MUST** be reported as blocked (E-SRC-1303). Safe retries **MAY** occur only if failed effects can be rolled back.

## 11 Scopes, Bindings, and Lookup

## 12 Tooling hooks

---

# Part III - Declarations & the Type System

## 10 Declarations

## 11 Type Foundations

## 12 Primitive Types

## 13 Composite Types

## 13. Modal Types

## 14. Pointer & Reference Model

## 15. Generics & Parametric Polymorphism

---

# Part IV - Ownership, Permissions, Regions

## 16. Ownership & Responsibility

## 17. Lexical Permission System (LPS)

## 18. Region-Based Memory & Arenas

## 19. Moves, Assignments, and Destruction

---

# Part V - Expressions, Statements, and Control Flow

## 20. Expressions

## 21. Statements

## 22. Control Flow and Pattern Matching

## 23. Error Handling and Panics

---

# Part VI - Contracts, and Grants

## 14. Contract Language

## 15. Grants

## 16. Diagnostics for Contracts and Effects

---

# Part VII - Abstraction, Behaviors, and Dynamic Dispatch

## 17. Behaviors (Interfaces/Traits)

## 18. Witness System (Runtime Polymorphism)

## 19. Generics & Behaviors Integration

---

# Part VIII - Compile-Time Evaluation & Reflection (No Macros)

## 20. Comptime Execution

## 21. Opt-In Reflection

## 22. Code Generation APIs (Explicit Metaprogramming)

---

# Part IX - Memory Model, Layout, and ABI

## 23. Memory Layout & Alignment

## 24. Interoperability & FFI

## 25. Linkage, ODR, and Binary Compatibility

---

# Appendix A – Formal ANTLR Grammar (Normative)

**[Placeholder]**

This appendix will contain the complete ANTLR grammar for Cursive, providing a machine-readable specification of the language's lexical and syntactic structure.

---

# Appendix B – Diagnostic Code Taxonomy (Normative)

**See [Appendix-B-Diagnostic-Code-Taxonomy.md](./Appendix-B-Diagnostic-Code-Taxonomy.md)**

The code schema `K-CAT-FFNN` is defined in [§7.4.1](#74-diagnostic-code-system-diagnosticscodes), and the external appendix provides the detailed feature bucket assignments for each category (SRC, TYP, MEM, EXP, GRN, FFI, DIA, CNF).

---

# Appendix C – Implementation-Defined Behavior Index (Normative)

**[Placeholder]**

This appendix will provide a comprehensive index of all implementation-defined behaviors referenced throughout the specification, with cross-references to the defining sections.

---

# Appendix D – Diagnostic Catalog (Normative)

This appendix aggregates every diagnostic defined elsewhere in the specification. Editors **MUST** update this catalog whenever a clause adds, removes, or renames a diagnostic so implementers can audit coverage from a single location.

| Code         | Severity | Clause    | Summary                                                      |
| ------------ | -------- | --------- | ------------------------------------------------------------ |
| `E-SRC-0101` | error    | [NDF_REF] | Invalid UTF‑8 sequence in source input                       |
| `E-SRC-0102` | error    | [NDF_REF] | Source file exceeds documented size limit                    |
| `E-SRC-0103` | error    | [NDF_REF] | Embedded BOM found after the first scalar value              |
| `E-SRC-0104` | error    | [NDF_REF] | Forbidden control character outside literals                 |
| `E-SRC-0201` | error    | [NDF_REF] | Comptime recursion depth exceeds guaranteed minimum          |
| `E-SRC-0202` | error    | [NDF_REF] | Comptime evaluation step budget exceeded                     |
| `E-SRC-0203` | error    | [NDF_REF] | Comptime heap limit exceeded                                 |
| `E-SRC-0204` | error    | [NDF_REF] | String literal exceeds guaranteed size during comptime       |
| `E-SRC-0205` | error    | [NDF_REF] | Collection cardinality exceeds comptime minimum              |
| `E-SRC-0206` | error    | [NDF_REF] | Comptime body requested undeclared or disallowed grants      |
| `E-SRC-0207` | error    | [NDF_REF] | Generated identifier collided with an existing declaration   |
| `E-SRC-0208` | error    | [NDF_REF] | Cycle detected in comptime dependency graph                  |
| `E-SRC-0301` | error    | [NDF_REF] | Unterminated string literal                                  |
| `E-SRC-0302` | error    | [NDF_REF] | Invalid escape sequence                                      |
| `E-SRC-0303` | error    | [NDF_REF] | Invalid character literal                                    |
| `E-SRC-0304` | error    | [NDF_REF] | Malformed numeric literal                                    |
| `E-SRC-0305` | error    | [NDF_REF] | Reserved keyword used as identifier                          |
| `E-SRC-0306` | error    | [NDF_REF] | Unterminated block comment                                   |
| `E-SRC-0307` | error    | [NDF_REF] | Invalid Unicode in identifier                                |
| `E-SRC-0401` | error    | [NDF_REF] | Unexpected EOF during unterminated statement or continuation |
| `E-SRC-0402` | error    | [NDF_REF] | Delimiter nesting depth exceeded supported bound             |
