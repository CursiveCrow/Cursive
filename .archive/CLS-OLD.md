# The Cursive Language Specification

**Version**: 1.0.0 Draft  
**Date**: 2025-11-11  
**Status**: In Progress  
**Purpose**: Normative reference for Cursive language implementation and tooling

---

## Abstract

This document is the normative specification for the Cursive programming language. It defines the language's syntax, semantics, and behavior with precision sufficient for building conforming implementations and tools. The specification serves implementers, tool developers, and educators, covering all aspects from lexical structure to concurrency and interoperability.


## Specification Completion Matrix (Informative)

| Part | Title                                | Status             | Notes                                                                       |
| ---- | ------------------------------------ | ------------------ | --------------------------------------------------------------------------- |
| 0    | Document Orientation                 | Core (normative)   | Structure, notation, terminology stable.                                    |
| I    | Conformance and Governance           | Core (normative)   | Behavior classes, conformance, diagnostics defined.                         |
| II   | Source Text & Translation            | Core + Skeleton    | Encoding, pipeline, diagnostics defined; lex. skeleton.                     |
| III  | Module System & Organization         | Partial + Skeleton | Modules/imports/initialization defined; names skeletal.                     |
| IV   | Type System                          | Partial + Skeleton | Foundations and invariants defined; full typing rules forthcoming.          |
| V    | Expressions & Statements             | Skeleton           | High-level invariants only (this pass).                                     |
| VI   | Generics & Parametric Polymorphism   | Skeleton           | Core obligations defined; advanced features pending.                        |
| VII  | Memory Model, Regions, & Permissions | Core + Skeleton    | Safety invariants, regions, and permissions defined; formalization pending. |
| VIII | Contracts & Grants                   | Skeleton           | Model and obligations defined; full sequent calculus pending.               |
| IX   | Behaviors & Dynamic Dispatch         | Skeleton           | High-level behavior and witness rules defined.                              |
| X    | Compile-Time Evaluation & Reflection | Skeleton           | Core model and limits defined; detailed APIs pending.                       |
| XI   | Concurrency & Memory Ordering        | Skeleton           | DRF guarantee and ordering requirements defined.                            |
| XII  | Interoperability & ABI               | Skeleton           | FFI and ABI obligations defined; platform matrices pending.                 |


## Table of Contents

- [The Cursive Language Specification](#the-cursive-language-specification)
  - [Abstract](#abstract)
  - [Specification Completion Matrix (Informative)](#specification-completion-matrix-informative)
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
    - [3.3 LLM-Friendly Surface Rules \[notation.llm\]](#33-llm-friendly-surface-rules-notationllm)
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
- [Part I - Conformance and Governance](#part-i---conformance-and-governance)
  - [6. Fundamental Conformance \[conformance\]](#6-fundamental-conformance-conformance)
    - [6.1 Behavior Classifications \[conformance.behavior\]](#61-behavior-classifications-conformancebehavior)
    - [6.2 Conformance Obligations \[conformance.obligations\]](#62-conformance-obligations-conformanceobligations)
    - [6.3 Binary Compatibility \[conformance.abi\]](#63-binary-compatibility-conformanceabi)
    - [6.4 Implementation Limits \[conformance.limits\]](#64-implementation-limits-conformancelimits)
    - [6.5 Reserved Identifiers \[conformance.reserved\]](#65-reserved-identifiers-conformancereserved)
    - [6.6 Conformance Verification \[conformance.verification\]](#66-conformance-verification-conformanceverification)
  - [7. Language Evolution and Governance \[evolution\]](#7-language-evolution-and-governance-evolution)
    - [7.1 Versioning Model \[evolution.versioning\]](#71-versioning-model-evolutionversioning)
    - [7.2 Feature Lifecycle \[evolution.lifecycle\]](#72-feature-lifecycle-evolutionlifecycle)
    - [7.3 Extension System \[evolution.extensions\]](#73-extension-system-evolutionextensions)
    - [7.4 Specification Maintenance \[evolution.specification\]](#74-specification-maintenance-evolutionspecification)
- [Part II - Source Text \& Translation](#part-ii---source-text--translation)
  - [8. Source Text and Encoding \[source\]](#8-source-text-and-encoding-source)
    - [8.1 Character Encoding \[source.encoding\]](#81-character-encoding-sourceencoding)
    - [8.2 Source File Structure \[source.structure\]](#82-source-file-structure-sourcestructure)
    - [8.3 Source Inclusion Model \[source.inclusion\]](#83-source-inclusion-model-sourceinclusion)
  - [9. Lexical Structure \[lexical\]](#9-lexical-structure-lexical)
    - [9.1 Lexical Elements \[lexical.elements\]](#91-lexical-elements-lexicalelements)
    - [9.2 Keywords and Operators \[lexical.keywords\]](#92-keywords-and-operators-lexicalkeywords)
    - [9.3 Identifiers \[lexical.identifiers\]](#93-identifiers-lexicalidentifiers)
    - [9.4 Literals \[lexical.literals\]](#94-literals-lexicalliterals)
  - [10. Syntactic Structure \[syntax\]](#10-syntactic-structure-syntax)
    - [10.1 Grammar Organization \[syntax.organization\]](#101-grammar-organization-syntaxorganization)
    - [10.2 Syntactic Nesting Limits \[syntax.limits\]](#102-syntactic-nesting-limits-syntaxlimits)
    - [10.3 Grammar Notation \[syntax.notation\]](#103-grammar-notation-syntaxnotation)
  - [11. Translation Phases and Pipeline \[translation\]](#11-translation-phases-and-pipeline-translation)
    - [11.1 Translation Model \[translation.model\]](#111-translation-model-translationmodel)
    - [11.2 Phase 1: Source Ingestion \[translation.ingestion\]](#112-phase-1-source-ingestion-translationingestion)
    - [11.3 Phase 2: Lexical Analysis \[translation.lexical\]](#113-phase-2-lexical-analysis-translationlexical)
    - [11.4 Phase 3: Syntactic Analysis \[translation.parsing\]](#114-phase-3-syntactic-analysis-translationparsing)
    - [11.5 Phase 4: Semantic Analysis \[translation.semantic\]](#115-phase-4-semantic-analysis-translationsemantic)
    - [11.6 Phase 5: Compile-Time Evaluation \[translation.comptime\]](#116-phase-5-compile-time-evaluation-translationcomptime)
    - [11.7 Phase 6: Code Generation \[translation.codegen\]](#117-phase-6-code-generation-translationcodegen)
  - [12. Diagnostic System \[diagnostics\]](#12-diagnostic-system-diagnostics)
    - [12.1 Diagnostic Requirements \[diagnostics.requirements\]](#121-diagnostic-requirements-diagnosticsrequirements)
    - [12.2 Diagnostic Code System \[diagnostics.codes\]](#122-diagnostic-code-system-diagnosticscodes)
    - [12.3 Severity Levels \[diagnostics.severity\]](#123-severity-levels-diagnosticsseverity)
    - [12.4 IFNDR and Diagnostic Limits \[diagnostics.ifndr\]](#124-ifndr-and-diagnostic-limits-diagnosticsifndr)
    - [12.5 Implementation-Defined Behavior \[diagnostics.idb\]](#125-implementation-defined-behavior-diagnosticsidb)
    - [12.6 Diagnostic Catalog Reference \[diagnostics.catalog\]](#126-diagnostic-catalog-reference-diagnosticscatalog)
- [Part III - Module System \& Organization](#part-iii---module-system--organization)
  - [13. Modules and Assemblies \[modules\]](#13-modules-and-assemblies-modules)
    - [13.1 Folder-Scoped Modules \[modules.discovery\]](#131-folder-scoped-modules-modulesdiscovery)
    - [13.2 Assemblies and Projects \[modules.assemblies\]](#132-assemblies-and-projects-modulesassemblies)
    - [13.3 Manifest Schema \[modules.manifest\]](#133-manifest-schema-modulesmanifest)
    - [13.4 Module Path Validity \[modules.paths\]](#134-module-path-validity-modulespaths)
  - [14. Imports and Name Resolution \[names\]](#14-imports-and-name-resolution-names)
    - [14.1 Import System \[names.imports\]](#141-import-system-namesimports)
    - [14.2 Scopes \[names.scopes\]](#142-scopes-namesscopes)
    - [14.3 Bindings \[names.bindings\]](#143-bindings-namesbindings)
    - [14.4 Name Lookup \[names.lookup\]](#144-name-lookup-nameslookup)
  - [15. Initialization and Dependencies \[initialization\]](#15-initialization-and-dependencies-initialization)
    - [15.1 Dependency Graph \[initialization.graph\]](#151-dependency-graph-initializationgraph)
    - [15.2 Eager Initialization \[initialization.eager\]](#152-eager-initialization-initializationeager)
    - [15.3 Lazy Initialization \[initialization.lazy\]](#153-lazy-initialization-initializationlazy)
    - [15.4 Initialization Failure \[initialization.failure\]](#154-initialization-failure-initializationfailure)
  - [16. Tooling Integration \[tooling\]](#16-tooling-integration-tooling)
- [Part IV - Type System](#part-iv---type-system)
  - [17. Type Foundations \& Type Declarations \[types.foundations\]](#17-type-foundations--type-declarations-typesfoundations)
  - [18. Primitive Types \[types.primitive\]](#18-primitive-types-typesprimitive)
  - [19. Composite Types \[types.composite\]](#19-composite-types-typescomposite)
  - [20. Function Types \& Procedure Declarations \[types.functions\]](#20-function-types--procedure-declarations-typesfunctions)
  - [21. Pointer \& Reference Types \[types.pointers\]](#21-pointer--reference-types-typespointers)
  - [22. Modal Types \& Type Relations \[types.modal\]](#22-modal-types--type-relations-typesmodal)
- [Part V - Expressions \& Statements](#part-v---expressions--statements)
  - [23. Expressions \& Variable Bindings \[expressions\]](#23-expressions--variable-bindings-expressions)
  - [24. Statements \& Control Flow \[statements\]](#24-statements--control-flow-statements)
  - [25. Pattern Matching \[patterns\]](#25-pattern-matching-patterns)
- [Part VI - Generics \& Parametric Polymorphism](#part-vi---generics--parametric-polymorphism)
  - [26. Type Parameters \& Generic Declarations \[generics.types\]](#26-type-parameters--generic-declarations-genericstypes)
  - [27. Region Parameters \[generics.regions\]](#27-region-parameters-genericsregions)
  - [28. Bounds \& Where Constraints \[generics.bounds\]](#28-bounds--where-constraints-genericsbounds)
  - [29. Variance \& Inference \[generics.variance\]](#29-variance--inference-genericsvariance)
  - [30. Resolution \& Monomorphization \[generics.resolution\]](#30-resolution--monomorphization-genericsresolution)
- [Part VII - Memory Model, Regions, \& Permissions](#part-vii---memory-model-regions--permissions)
  - [31. Memory Model Overview \[memory.model\]](#31-memory-model-overview-memorymodel)
  - [32. Objects \& Storage Duration \[memory.objects\]](#32-objects--storage-duration-memoryobjects)
  - [33. Regions \& Region Safety \[memory.regions\]](#33-regions--region-safety-memoryregions)
  - [34. Permissions (Lexical Permission System) \[memory.permissions\]](#34-permissions-lexical-permission-system-memorypermissions)
  - [35. Move/Copy/Clone Semantics \[memory.semantics\]](#35-movecopyclone-semantics-memorysemantics)
  - [36. Layout, Alignment, \& Aliasing \[memory.layout\]](#36-layout-alignment--aliasing-memorylayout)
- [Part VIII - Contracts \& Grants](#part-viii---contracts--grants)
  - [37. Contract Model \& Sequent Syntax \[contracts.model\]](#37-contract-model--sequent-syntax-contractsmodel)
  - [38. Grants \& Grant Declarations \[contracts.grants\]](#38-grants--grant-declarations-contractsgrants)
  - [39. Preconditions \& Postconditions \[contracts.conditions\]](#39-preconditions--postconditions-contractsconditions)
  - [40. Invariants \[contracts.invariants\]](#40-invariants-contractsinvariants)
  - [41. Verification Flow \[contracts.verification\]](#41-verification-flow-contractsverification)
- [Part IX - Behaviors \& Dynamic Dispatch](#part-ix---behaviors--dynamic-dispatch)
  - [42. Behaviors (Interface/Trait Declarations) \[behaviors.declarations\]](#42-behaviors-interfacetrait-declarations-behaviorsdeclarations)
  - [43. Behavior Implementations \[behaviors.implementations\]](#43-behavior-implementations-behaviorsimplementations)
  - [44. Witness System \& vtables \[behaviors.witnesses\]](#44-witness-system--vtables-behaviorswitnesses)
- [Part X - Compile-Time Evaluation \& Reflection](#part-x---compile-time-evaluation--reflection)
  - [45. Comptime Model \& Comptime Declarations \[comptime.model\]](#45-comptime-model--comptime-declarations-comptimemodel)
  - [46. Comptime Blocks \& Contexts \[comptime.blocks\]](#46-comptime-blocks--contexts-comptimeblocks)
  - [47. Opt-In Reflection \& Attributes \[comptime.reflection\]](#47-opt-in-reflection--attributes-comptimereflection)
  - [48. Type Metadata \& Queries \[comptime.metadata\]](#48-type-metadata--queries-comptimemetadata)
  - [49. Code Generation API \[comptime.codegen\]](#49-code-generation-api-comptimecodegen)
- [Part XI - Concurrency \& Memory Ordering](#part-xi---concurrency--memory-ordering)
  - [50. Concurrency Model \[concurrency.model\]](#50-concurrency-model-concurrencymodel)
  - [51. Memory Ordering \& Atomics \[concurrency.atomics\]](#51-memory-ordering--atomics-concurrencyatomics)
- [Part XII - Interoperability \& ABI](#part-xii---interoperability--abi)
  - [52. Foreign Function Interface \[ffi\]](#52-foreign-function-interface-ffi)
  - [53. ABI \& Binary Layout \[abi\]](#53-abi--binary-layout-abi)
  - [54. Linkage \& Symbol Resolution \[linkage\]](#54-linkage--symbol-resolution-linkage)
- [Appendix A – Formal ANTLR Grammar (Normative)](#appendix-a--formal-antlr-grammar-normative)
- [Appendix B – Diagnostic Code Taxonomy (Normative)](#appendix-b--diagnostic-code-taxonomy-normative)
- [Appendix C – Conformance Dossier Schema (Normative); IDB \& UVB Attestation Index](#appendix-c--conformance-dossier-schema-normative-idb--uvb-attestation-index)
- [Appendix D – Diagnostic Catalog (Normative)](#appendix-d--diagnostic-catalog-normative)
- [Appendix E – Conformance Verification](#appendix-e--conformance-verification)
- [Appendix F – Tooling Integration](#appendix-f--tooling-integration)

---

# Part 0 - Document Orientation

## 1. Purpose, Scope, and Audience [intro]

### 1.1 Purpose [intro.purpose]

This specification **defines the Cursive programming language**: its source text, static semantics (typing and name resolution), dynamic semantics (runtime behavior), concurrency and memory model, and the required behavior of conforming implementations. This specification defines requirements for conforming implementations and establishes the normative behavior that programs may rely upon.

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

Code examples in this document use fenced blocks with the `cursive` info string. All code examples in this specification are **informative** unless explicitly marked as normative with the annotation `[Normative Example]`. A trailing comment like `// error[E-CAT-FFNN]: …` marks the diagnostic the example is expected to trigger.

> Any example explicitly annotated `[Normative Example]` **MUST** be executable (or rejected with the stated diagnostics), **MUST** be covered by the normative conformance test suite described in Appendix E, and **MUST** be versioned together with the specification’s MINOR and PATCH releases. Changes to a normative example that alter its observable behavior or expected diagnostics **MUST** be treated as normative specification changes and reflected in the conformance artifacts.

#### 2.1.5 Diagnostic Code Format [primer.structure.diagnostics]

The canonical diagnostic code format is defined in §12.2.1.

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

> The requirement itself, enclosed in a blockquote using normative keywords (**MUST**, **SHOULD**, **MAY**). When grammar patterns are included, they appear within the blockquote as part of the normative statement using template-based syntax (e.g., `<condition>`, `<block>`).

**_Formal rule:_**
$$ \text{A inference rule or judgment (when the requirement involves typing, evaluation, or well-formedness)} $$

**_Explanation:_**
If the normative rule needs additional context, it is provided via prose, tables, or diagrams clarifying the requirement's meaning, rationale, or scope.

**_Diagnostic:_**
Any diagnostics triggered by the requirement are documented in a table with the following columns:

| Code       | Severity                 | Description      |
| ---------- | ------------------------ | ---------------- |
| K-CAT-FFNN | Error \| Warning \| Note | Diagnostic text. |

#### 2.2.1 Subordinate Requirements [primer.requirements.subordinate]

Some requirements contain subordinate subsections that refine or constrain the parent requirement. Subordinate requirements appear as nested level-5 headings within a parent requirement, each with their own normative statements, formal rules, and examples when applicable.

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

- $'$ (prime) denotes resulting or "after" states (e.g., $\Gamma'$, $\sigma'$)
- Subscripts denote variants or specialized forms (e.g., $\Gamma_{\text{terms}}$, $\sigma_{\text{ct}}$)
- $\cdot$ denotes the empty context

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

### 3.3 LLM-Friendly Surface Rules [notation.llm]

To support reliable AI-assisted development, Cursive’s concrete syntax and formatting conventions **MUST** follow a small set of “LLM-friendly” rules:

> 1. **Stable keyword order** – Syntactic forms that combine multiple keywords (for example, visibility + `procedure` + `async`) **MUST** use a single, specified keyword order; implementations **MUST NOT** accept permutations of that order.  
> 2. **Explicit separators** – Statement, declaration, and list grammars **MUST** rely on explicit separators (such as semicolons and commas) rather than significant indentation or layout-sensitive parsing.  
> 3. **No context-sensitive keywords** – Tokens classified as keywords in §9.2.1 **MUST NOT** be usable as identifiers in any syntactic context; new keywords **MUST NOT** be introduced in a way that depends on surrounding syntactic form.  
> 4. **Formatting stability** – Implementations **SHOULD** provide a canonical formatter whose output depends only on the token sequence and configuration, and formatting **MUST NOT** change program meaning.

These rules are intended to minimize ambiguity in the surface syntax so that automated tools can produce stable, idiomatic code for Cursive.

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
_A program is conforming iff it is well‑formed, relies only on documented IDB, uses extensions only when explicitly enabled, and does not trigger UVB during execution unless every UVB site that is reachable in the link‑closed program image is attested by an external proof artifact recorded in the conformance dossier (see §6.2.4). The presence of any reachable UVB site that lacks an attestation entry compatible with the dossier used for the build makes a program non‑conforming._

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
_the association between an identifier and an entity within a particular scope. Binding introduction and properties are specified in Part III, §14.3 [names.bindings]._

**Translation Unit**
_the smallest source artifact processed as a whole during translation. See also compilation unit (§8.3.1 [source.inclusion.units])_.

**Declaration**
_a syntactic form that introduces a name and determines its category. Declaration forms and grammar are specified in Part III, §10.1.1 [syntax.organization.declarations]._

**Diagnostic**
_a message issued when a program violates a rule requiring detection. Diagnostics include errors (rejecting) and warnings (permissive)._

**Entity**
_any value, type, or module that may be named or referenced._

**Object**
_a region of storage with a type, storage location (contiguous bytes at a specific memory address), lifetime (bounded interval from creation to destruction), alignment constraint, and cleanup responsibility (association with zero or more responsible bindings). Objects and storage duration are specified in Part VII, §32 [memory.objects]._

**Value**
_a temporary result produced by an expression that may be moved (consumed) or copied. Literals, arithmetic expressions, and most procedure calls produce values. Move/copy semantics are specified in Part VII, §35 [memory.semantics]._

**Expression**
_a syntactic form that yields a value or place. Expression categories and evaluation are specified in Part V, §23 [expressions]._

**Place**
_a memory location that can be the target of assignment or the source of address-of operations; expressions are categorized as yielding values or places. See Part V, §23.1.1 for expression categories._

**Modal State**
_a named compile-time state (denoted `@State`) in a modal type's state machine; modal values must inhabit exactly one state at any time, and transitions between states are enforced by the type system. Modal types and state transitions are specified in Part IV, §22.1 [types.modal]._

**Statement**
_a syntactic form that executes for effects on program state without directly yielding a value. Statement well-formedness and control flow are specified in Part V, §24 [statements]._

**Scope**
_the syntactic region where a binding is visible. Scope kinds, nesting, and contents are specified in Part III, §14.2 [names.scopes]._

**Principal Type**
_in a type inference context, the most general type that satisfies all constraints. When type inference succeeds, the principal type is unique. See Part IV, §17.1.4 for typing judgments and principal types._

**Nominal Type**
_a type identified by a declaration and name, possibly parametrized; variance of parameters is declared per parameter. Nominal type declarations are specified in Part IV, §17.1.2 [types.foundations]._

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

- **[Unicode]** The Unicode Standard, Version 14.0.0 or later. Supplies derived properties, normalization forms, and identifier recommendations leveraged by the lexical grammar.

- **[UAX31]** Unicode Standard Annex #31 - _Unicode Identifier and Pattern Syntax_. Specifies identifier composition rules (`XID_Start`, `XID_Continue`) adopted in lexical tokenization.

### 5.3 Platform and ABI Standards [references.platform]

- **[SysV-ABI]** System V Application Binary Interface (AMD64 Architecture Processor Supplement, ARM Architecture Procedure Call Standard). Guides interoperability obligations for POSIX-compliant platforms.

- **[MS-x64]** Microsoft x64 Calling Convention - Calling convention documentation for Windows x64 platforms. Required for conforming implementations targeting Windows.

### 5.4 IETF Standards [references.ietf]

- **[RFC2119]** RFC 2119 - _Key words for use in RFCs to Indicate Requirement Levels_. Defines normative vocabulary (**MUST**, **SHOULD**, **MAY**).

- **[RFC8174]** RFC 8174 - _Ambiguity of Uppercase vs Lowercase in RFC 2119 Key Words_. Clarifies that requirement keywords apply only when in ALL CAPS.

- **[RFC3629]** RFC 3629 - _UTF-8, a transformation format of ISO/IEC 10646_. Defines the UTF-8 encoding scheme used for Cursive source text (§8.1.1).

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

# Part I - Conformance and Governance

This part establishes the foundational requirements for conforming Cursive implementations and specifies governance rules for language evolution, extensions, and backward compatibility. It provides the normative framework that all other parts of this specification build upon, including conformance obligations, behavior classification, and version management.

*(Informative)*

| Theme                                                                                                                                  | Key Diagnostics                                                                              |
| -------------------------------------------------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------- |
| Behavior classes (UVB, USB, IDB, defined, IFNDR) are applied consistently to all operations.                                           | Classification appears in behavior clauses; indirect coverage via all category/bucket codes. |
| Conforming implementations accept all well-formed programs and reject ill-formed programs with at least one error diagnostic.          | `E-SRC-*`, `E-TYP-*`, `E-MEM-*`, `E-EXP-*`, etc.                                             |
| Conforming programs rely only on documented implementation-defined behavior and attested UVB, and do not execute unattested UVB sites. | Conformance dossier and UVB attestation requirements (§6.1.4, §6.2.2, Appendix C).           |
| Implementation limits meet or exceed the specified minima and are documented; exceeding limits is diagnosed.                           | `E-SRC-0102`, `E-SRC-0402`, `E-SRC-1401`, and related limit diagnostics.                     |
| Reserved identifiers and namespaces are not used by user programs or vendor extensions.                                                | `E-SRC-0305`, `E-SRC-1103`, `E-SRC-1104`, reserved-identifier diagnostics.                   |

## 6. Fundamental Conformance and Diagnostics [conformance]

This chapter defines behavior classifications, the diagnostic system, and conformance obligations for implementations and programs.

### 6.1 Behavior Classifications [conformance.behavior]

This section defines four categories of program behavior based on how this specification constrains outcomes and verification feasibility.

**Unverifiable Behavior (UVB)** [§4.2] comprises operations whose runtime correctness depends on properties outside the language's semantic model or requires solving computationally intractable verification problems. UVB occurs when: (1) properties depend on external contracts, foreign memory validity, or environmental state not represented in Cursive's type system; (2) verification requires solving undecidable or NP-complete problems, or whole-program analysis across compilation boundaries; or (3) required verification information does not exist within the program's Cursive code. Array bounds checking is not classified as UVB because array length is known within Cursive semantics and may be enforced by static or runtime checks. FFI pointer dereferencing is classified as UVB because pointer validity depends on contracts and state outside Cursive's semantic model.

**Unspecified Behavior (USB)** [§4.2] occurs when this specification permits multiple valid outcomes but does not require implementations to document which outcome is chosen. Implementations may choose any permitted outcome, and the choice may vary between executions. Unlike implementation-defined behavior, no documentation is required. All permitted outcomes fall within bounds established by this specification.

**Implementation-Defined Behavior (IDB)** [§4.2] occurs when this specification permits multiple valid outcomes and requires implementations to document their choice. Unlike unspecified behavior, implementations must document their choices, enabling portability analysis. Common sources include target architecture characteristics (pointer size, endianness, alignment), calling conventions, and memory layout decisions.

**Defined Behavior** [§4.2] comprises all program behavior fully specified by this specification. Operations produce deterministic outcomes identical across conforming implementations (except where IDB allows documented variation). Defined behavior is the complement of UVB, USB, and IDB. Most language operations exhibit defined behavior: arithmetic on in-range values, control flow, pattern matching, bounded memory access, etc.

#### 6.1.1 Acceptance of UVB Programs [conformance.behavior.uvb]

> Implementations **MUST** accept well-formed programs containing UVB operations and **MUST NOT** reject programs solely for containing UVB.
>
> Translation **MUST** preserve program semantics up to and including UVB operation initiation. When UVB operations execute, implementations **MAY** produce any behavior, including abnormal termination.

**_Explanation:_**
Implementations need not detect UVB at compile time, emit warnings, or provide runtime protections. Programmers must ensure UVB operation correctness through external verification. When UVB occurs, observable behavior is constrained to the executing process.

#### 6.1.2 Verification Modes and Resource Limits [conformance.behavior.verification]

> UVB classification is independent of verification modes (see Part X, §45–§49 [comptime.model], [comptime.blocks], [comptime.reflection], [comptime.metadata], [comptime.codegen]). Properties that can be enforced by static analysis or runtime checks required by this specification are not classified as UVB, regardless of resource limits or policy choices.
>
> When verification fails due to resource constraints, implementations **MUST** either (a) reject the program with a diagnostic in strict mode, or (b) insert runtime checks in dynamic verification mode. Implementations **MUST NOT** insert runtime checks in strict mode unless explicitly enabled by a build flag or attribute.

**_Explanation:_**
UVB operations are inherently unverifiable due to problem structure. Non-UVB operations remain the implementation's verification responsibility regardless of resource cost.

#### 6.1.3 Unspecified Behavior Requirements [conformance.behavior.usb]

> Permitted outcomes for USB **MUST** be constrained by this specification. Implementations **MAY** choose any permitted outcome. Different executions **MAY** produce different outcomes.

**_Explanation:_**
Unlike IDB (§6.1.4), implementations need not document their choice, and the choice may vary between executions. All permitted outcomes fall within specification bounds. Programs exhibiting USB are conforming.

##### 6.1.3.1 Ill-Formed, No Diagnostic Required (IFNDR) [conformance.behavior.ifndr]

> Programs violating static-semantic rules are ill-formed. When violations are computationally infeasible to detect, implementations need not issue diagnostics. Such programs are **Ill-Formed, No Diagnostic Required (IFNDR)**.
>
> **Language Design Invariant (conditional on memory model):** Assuming the region and Lexical Permission System (LPS) rules specified in Part VII (§31–§36) hold for all programs classified as safe code, Cursive's syntax and static semantics ensure that IFNDR violations lead to one of the following outcomes:
> 1. Defined behavior that happens to satisfy the violated rule
> 2. Benign failure: process termination via panic or trap, without memory unsafety
> 3. Unspecified but bounded behavior (e.g., incorrect computation results, non-deterministic control flow)
>
> UVB arises exclusively from operations within `unsafe` blocks and FFI calls whose preconditions depend on foreign contracts. Under the assumptions above, IFNDR violations outside these constructs cannot introduce UVB because the language design prevents unsafe memory operations in safe code, regardless of static rule violations.

**_Explanation:_**
Violations are computationally infeasible to detect when detection requires solving NP-complete, undecidable, or exponential-complexity problems, or whole-program properties across compilation boundaries.

IFNDR differs from UVB: IFNDR represents ill-formedness; UVB represents operations whose runtime correctness depends on external properties. IFNDR programs are not conforming (§6.2.2). The language design ensures IFNDR cannot introduce UVB in safe code.

**_Formal property (conditional on Part VII):_**

The Cursive language design, together with the LPS and region rules of Part VII, establishes the following safety property:

$$
\begin{align*}
\forall \text{ programs } P \text{ in safe Cursive}: \; & \text{well\_formed}(P) \implies \\
& \left( \forall \text{ executions } e \text{ of } P: \right. \left( \text{terminates}(e) \lor \text{panics}(e) \lor \text{diverges}(e)\right)  \left.\land \; \lnot \; \text{exhibits\_UVB}(e) \right)
\end{align*}
$$


> [!note] IFNDR vs UVB distinction
>
> - **IFNDR**: Program violated a language rule, but detecting the violation is computationally infeasible. Behavior is unspecified but constrained.
> - **UVB**: Program is well-formed, but operation correctness depends on unverifiable properties. Behavior is unconstrained.

#### 6.1.4 Implementation-Defined Behavior [conformance.behavior.idb]

> IDB occurs when this specification permits multiple valid outcomes and requires implementations to document their choice. Permitted outcomes **MUST** be constrained by this specification.
>
> Implementations **MUST** document all IDB choices in a conformance dossier (§6.2.4). Programs **MUST NOT** rely on undocumented IDB.

**_Explanation:_**
IDB allows target-appropriate choices while ensuring predictability. Unlike USB [§6.1.3], implementations must document their choices. Common sources include target architecture characteristics (pointer size, endianness, alignment), calling conventions, and memory layout.

##### 6.1.4.1 Documentation Requirements [conformance.behavior.idb.documentation]

> Implementations **MUST** include, in their conformance dossier (Appendix C), documentation of implementation-defined behaviors that affect portability or correctness, including at least:
>
> The conformance dossier’s `implementation_defined_behaviors` section (Appendix C, §C.2.3) **MUST** serve as the normative Implementation-Defined Behavior (IDB) Index for the implementation.
>
> 1. Primitive type sizes, alignments, and representations (see Part IV, §18 [types.primitive]; Part VII, §36 [memory.layout]; Part XII, §53 [abi])
> 2. Pointer width, representation, and provenance rules (see Part VII, §34–§35 [memory.permissions], [memory.semantics]; Part XII, §53 [abi])
> 3. Foreign function calling conventions (see Part XII, §52–§53 [ffi], [abi])
> 4. Endianness for multi-byte values (see Part II, §8.1; Part XII, §53 [abi])
> 5. Memory layout and padding for compound types (see Part VII, §36 [memory.layout]; Part XII, §53 [abi])
> 6. Signed integer overflow behavior (see Part IV, §18 [types.primitive])
> 7. Thread scheduling, memory ordering, and synchronization (see Part XI, §50–§51 [concurrency.model], [concurrency.atomics])
> 8. Platform-specific panic and unwind behavior (see Part XII, §52 [ffi]; Part I, §6.3 [conformance.abi])
>
> Documentation **SHOULD** be accessible via `--version`, online docs, or metadata files, organized by target triple and language version.

### 6.2 Diagnostic System [conformance.diagnostics]

This section specifies how implementations report violations detected during translation. It defines the canonical format and organization of diagnostic codes that provide stable identifiers for each diagnostic across implementations and specification versions.

#### 6.2.1 Obligation to Diagnose [diagnostics.requirements.obligation]

> Implementations **MUST** issue at least one diagnostic for every ill-formed program as defined in §6.3.3, except for IFNDR violations (§6.1.3.1).
>
> Implementations **MUST NOT** produce executable artifacts for programs that trigger error diagnostics (severity E, see §6.2.5.1).
>
> Implementations **MAY** produce non-executable artifacts (syntax trees, symbol tables, diagnostic metadata) for tooling purposes regardless of program well-formedness.
>
> When multiple violations exist, implementations **SHOULD** issue diagnostics for independent violations but **MAY** suppress cascading diagnostics that result from a primary error.

#### 6.2.2 Minimum Diagnostic Content [diagnostics.requirements.content]

> Every diagnostic **MUST** include:
>
> 1. A diagnostic code following the format specified in §6.2.3.
> 2. A severity level as defined in §6.2.5.
> 3. Source location information: file path, line number, and column number.
> 4. A description of the violation sufficient to understand the error.
>
> Implementations **MAY** provide additional information including but not limited to: supplementary notes, related source locations, type information, suggested corrections, and structured metadata for tooling integration.

#### 6.2.3 Diagnostic Code System [diagnostics.codes]

> Diagnostic codes **MUST** follow the format `K-CAT-FFNN` where:
>
> - `K` is a single character severity indicator: `E` (error), `W` (warning), or `N` (note).
> - `CAT` is a three-letter category code in uppercase (see §6.2.4).
> - `FF` is a two-digit feature bucket in decimal notation (00-99).
> - `NN` is a two-digit sequence number within the bucket (00-99).
> - Separators **MUST** be ASCII hyphen-minus (U+002D).
>
> The normative catalog of specification-defined diagnostic codes appears in Appendix B and Appendix D. Once a diagnostic code is published, its meaning **MUST NOT** change in subsequent versions. Deprecated diagnostics **MUST** be marked in Appendix D with their deprecation version and replacement code.

#### 6.2.4 Diagnostic Categories [diagnostics.categories]

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
> Feature buckets within each category **MUST** be allocated according to the taxonomy defined in Appendix B.

#### 6.2.5 Severity Levels [diagnostics.severity]

This section defines the three severity levels that classify diagnostic importance and determine whether translation may proceed.

##### 6.2.5.1 Error Diagnostics [diagnostics.severity.error]

> Error diagnostics (severity indicator `E`) **MUST** be issued when a program violates a normative requirement expressed with **MUST** or **MUST NOT**. Error diagnostics indicate ill-formed programs that do not conform to this specification (§6.3.2).

##### 6.2.5.2 Warning Diagnostics [diagnostics.severity.warning]

> Warning diagnostics (severity indicator `W`) **MAY** be issued for constructs that are well-formed according to this specification but potentially problematic (e.g., unused variables, deprecated features). Programs that trigger only warnings are conforming.

##### 6.2.5.3 Note Diagnostics [diagnostics.severity.note]

> Note diagnostics (severity indicator `N`) **MUST** provide supplementary context for an associated error or warning and **MUST NOT** appear independently.

### 6.3 Conformance Obligations [conformance.obligations]

This section defines conformance for implementations and programs.

#### 6.3.1 Conforming Implementations [conformance.obligations.implementations]

> Implementations **MUST** satisfy all MUST requirements and produce programs whose observable behavior matches the dynamic semantics defined herein, except where IDB (§6.1.4) or USB (§6.1.3) permits variation.

#### 6.3.2 Conforming Programs [conformance.obligations.programs]

> A program conforms when it satisfies:
>
> 1. **Well-formedness**: Satisfies all lexical, syntactic, and static-semantic rules (§6.3.3)
> 2. **Documented IDB reliance**: Relies only on documented IDB (§6.1.4.1, §6.3.4)
> 3. **Extension usage**: Uses extensions only when explicitly enabled (§7.3.2)
> 4. **UVB attestation (static)**: Every operation that this specification classifies as UVB (§6.1.1) and that is reachable in the link‑closed program image **MUST** be accompanied by an attestation entry in the conformance dossier demonstrating external correctness verification. Attestations **MUST** reference the source location, the verification method (formal proof, manual audit, test coverage, or external contract), and provide either a proof artifact URI or cryptographic hash of the proof document.
>
> Implementations **MAY** additionally support runtime coverage mechanisms that record which UVB sites were actually executed in a given run and cross‑check those executions against the statically required attestations, but such mechanisms **MUST NOT** weaken the static obligation in item 4.

**_Explanation:_**
UVB operations require external verification through formal methods, manual audits, testing, or documented proofs. The attestation system enables build tools to verify that every UVB site that is syntactically present and reachable in the program and its linked artifacts has a corresponding attestation entry in the dossier, transforming UVB into externally verified behavior for the purposes of conformance.

Programs containing IFNDR violations (§6.1.3.1) are ill-formed and non-conforming, even without diagnostics.

#### 6.3.3 Well-Formedness [conformance.obligations.wellformedness]

> Implementations **MUST** accept well-formed programs and **MUST** reject ill-formed programs by issuing at least one error diagnostic, except for IFNDR violations (§6.1.3.1).
>
> Implementations **MUST NOT** produce executable artifacts for rejected programs. Implementations **MAY** produce non-executable artifacts for tooling (syntax trees, symbol tables, diagnostic metadata).
>
> Implementations **MAY** issue warnings for accepted programs without affecting conformance.

**_Explanation:_**
The executable/non-executable distinction supports tooling (IDEs, analyzers) for ill-formed programs while preventing execution.

IFNDR programs remain ill-formed and non-conforming even without diagnostics. Implementations are encouraged to diagnose IFNDR when feasible.

#### 6.3.4 Conformance Dossier [conformance.obligations.dossier]

> An implementation **MUST** provide a conformance dossier documenting implementation-defined behaviors and UVB attestations in a structured format (JSON or TOML) with stable keys. UVB attestations **MUST** enumerate all UVB sites that appear in the program and its linked artifacts by file, span, and rule, and attach proof URIs or embedded proof hashes. Conformance tools **MUST** fail builds when UVB sites lack attestation entries. Documentation **SHOULD** be accessible via online documentation, metadata files, or command-line flags.
>
> When conformance checking is requested for a target, an implementation **MUST** ensure that a conformance dossier conforming to Appendix C is available for the selected target triple and build configuration. If the required dossier is missing, fails the normative schema of Appendix C, or omits documentation for implementation-defined behaviors and limits that this specification requires to be documented (§6.1.4.1, §6.5.2, Appendix C), the implementation **MUST** diagnose this condition with an error diagnostic in the `CNF-02` feature bucket (e.g., `E-CNF-0204`) and **MUST NOT** claim conformance for that target while such a diagnostic remains outstanding.


#### 6.3.5 Observable Behavior [conformance.obligations.observable]

> Observable behavior comprises program effects that are externally visible or specified as observable by this specification, including:
>
> 1. Input and output operations
> 2. Accesses to volatile memory locations
> 3. Synchronization operations and their ordering
> 4. Externally visible termination events (such as process exit with a status code, or a documented panic/unwind boundary)
> 5. Diagnostic output required by this specification
>
> Implementations **MAY** perform any transformation to a program provided the observable behavior of the transformed program matches the observable behavior required by this specification. This is known as the **as-if rule**.
>
> Implementations **MAY** eliminate, reorder, or optimize operations that do not affect observable behavior. Operations on non-volatile memory locations that do not contribute to observable behavior **MAY** be elided.
>
> The as-if rule **MUST NOT** be used to justify transformations that alter:
>
> 1. The values or sequencing of observable operations
> 2. The occurrence or ordering of externally visible termination events
> 3. Whether the program triggers unverifiable behavior (UVB, §6.1.1)
> 4. The synchronization ordering guaranteed by the memory model (Part XI, §50–§51)
>
> Transformations that change whether a program eventually terminates versus diverges **MAY** be applied only when they do not introduce or remove any externally visible termination events and do not change any other observable effects in the sense above (for example, eliminating a busy‑wait loop that performs no observable operations).

#### 6.3.6 Language Safety Guarantees [conformance.obligations.safety]

> Implementations **MUST** provide the following safety guarantees for well-formed programs that do not use `unsafe` blocks or foreign function calls:
>
> 1. **Memory safety**: The type system and permission system prevent buffer overruns, use-after-free, and dangling pointer dereferences. Programs cannot read or write memory outside the bounds of allocated objects.
>
> 2. **Type safety**: The type system prevents type confusion, invalid enum variants, and violations of type system guarantees. Programs cannot observe values whose representation violates their type's invariants.
>
> 3. **Data-race freedom**: Programs are data-race-free by construction. The permission system and region model (Part VII, §33–§34) ensure that concurrent accesses to shared memory are properly synchronized or provably non-conflicting.
>
> 4. **Move semantics correctness**: Programs cannot use values after they have been moved. Responsibility and move semantics (Part VII, §35) together with the lexical permission system (Part VII, §34) enforce move semantics statically.
>
> These guarantees **MAY** be violated within `unsafe` blocks or when calling foreign functions, where correctness depends on programmer-supplied contracts and external verification (§6.1.1, §6.3.2).
>
> Implementations **MUST** document any language constructs that introduce UVB or rely on external verification in their conformance dossier (§6.3.4).

#### 6.3.7 Conformance Modes [conformance.obligations.modes]

> Implementations **MAY** provide named conformance modes that control enforcement and diagnostic strictness, but builds **MUST** be considered conforming only when compiled under a mode that satisfies all requirements of §6.3.1–§6.3.3.
>
> Implementations **MUST** support a `strict` conformance mode that enforces all MUST requirements of this specification without relaxing diagnostics or inserting additional runtime checks beyond those permitted in §6.1.2 and Part XI. In strict mode, violations that this specification classifies as errors **MUST** be diagnosed as errors and **MUST** prevent the production of executable artifacts for the affected compilation units.
>
> Implementations **MAY** support a `permissive` conformance mode in which additional diagnostics (such as warnings) may be suppressed or downgraded, provided that:
>
> 1. All ill-formed programs (§6.3.3) are still rejected with at least one error diagnostic.
> 2. All extension and feature-flag requirements (including `E-CNF-0101` and `E-CNF-0103`) continue to be enforced as errors in conforming modes (§7.3.2, §13.3.1).
> 3. The observable behavior of well-formed programs remains consistent with this specification, except where IDB (§6.1.4) or USB (§6.1.3) permits variation.
>
> Implementations **MAY** provide additional non-conforming compatibility modes (for example, legacy or vendor-specific modes) that relax these requirements, but programs compiled under such modes **MUST NOT** be claimed as conforming, and such modes **MUST** be documented as non-conforming in the conformance dossier (§6.3.4). Using permissive conformance modes for production or release builds **SHOULD** trigger `W-CNF-0201` diagnostics.
>
> Selecting a conformance mode that is not recognized by the implementation **MUST** be diagnosed with `E-CNF-0201`, conflicting conformance mode settings across configuration sources **MUST** be diagnosed with `E-CNF-0202`, and requesting a valid conformance mode that is unsupported for the current target **MUST** be diagnosed with `E-CNF-0203`.
>
> For conformance tools and build pipelines, the following mode matrix **MUST** apply:
>
> | Mode           | May be advertised as conforming? | Dossier and UVB requirements                                              | Additional obligations                                             |
> | -------------- | -------------------------------- | ------------------------------------------------------------------------- | ------------------------------------------------------------------ |
> | `strict`       | **YES**                          | Dossier **MUST** validate; all required IDB entries and UVB attestations **MUST** be present; missing or invalid entries **MUST** cause `E-CNF-0204/0205` and prevent executable artifact production. | Default mode for certification and conformance claims.            |
> | `permissive`   | **YES**, if explicitly labeled   | Same dossier and UVB obligations as `strict` for any build that is claimed to be conforming; implementations **MAY** additionally produce non‑conforming artifacts (e.g., for experimentation) but such artifacts **MUST NOT** be advertised as conforming. | Use in production or release builds **SHOULD** emit `W-CNF-0201`. |
> | compatibility* | **NO**                           | Mode is explicitly non‑conforming; dossier entries **MAY** be incomplete and UVB attestations **MAY** be omitted. | Mode **MUST** be documented as non‑conforming in the dossier.     |
>
> Compatibility modes are any named modes that the implementation documents as non‑conforming (for example, `"legacy"` or `"experimental"`); they **MUST NOT** be treated as satisfying §6.3.1–§6.3.3.

### 6.4 Binary Compatibility [conformance.abi]

This section defines requirements for binary compatibility and ABI stability across language versions, compilation modes, and implementations.

#### 6.4.1 ABI Stability Requirements [conformance.abi.stability]

> Implementations **MUST NOT** guarantee binary compatibility across different language versions (including minor versions), different compiler versions, different compilation modes, or different optimization levels unless explicitly documented in the conformance dossier (§6.3.4).
>
> Implementations **MAY** provide ABI stability guarantees within a documented scope (e.g., within a single compiler major version, within a single language major version, or across specified compilation modes). Such guarantees **MUST** be documented in the conformance dossier.
>
> Source-level compatibility **MUST** be maintained for programs across minor language versions within the same major version, subject to deprecation timelines (§7.2.3–§7.2.4).

#### 6.4.2 ABI Breaking Changes [conformance.abi.breaking]

> Changes that alter type layout, calling conventions, symbol mangling, or vtable structure **MUST** be classified as ABI-breaking changes.
>
> ABI-breaking changes **MUST** be documented in release notes and conformance dossiers with migration guidance.
>
> Implementations **SHOULD** provide tooling to detect ABI incompatibilities between compilation units.

#### 6.4.3 Cross-Version Linkage [conformance.abi.linkage]

> Implementations **MUST** diagnose attempts to link compilation units built with incompatible ABIs when such incompatibility is detectable.
>
> When ABI versioning is implemented, the ABI version **MUST** be encoded in symbols or metadata such that linkers can detect mismatches.
>
> Implementations **MAY** support multiple ABI versions concurrently through namespacing or versioned symbols, provided the versioning scheme is documented.

### 6.5 Implementation Limits [conformance.limits]

This section defines minimum guaranteed limits that conforming implementations must support and documentation requirements for implementation-specific limits.

#### 6.5.1 Minimum Guaranteed Limits [conformance.limits.minimum]

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
> Implementations **MAY** exceed these minimums. Implementations **SHOULD** document their actual limits in the conformance dossier (§6.3.4).

#### 6.5.2 Limit Documentation Requirements [conformance.limits.documentation]

> Implementations **MUST** document their implementation limits in the conformance dossier (Appendix C), including at least:
>
> 1. All limits where the implementation's actual limit differs from the minimum guaranteed limit
> 2. Platform-dependent limits including stack size, heap allocation limits, and thread concurrency limits
> 3. Whether limits are configurable via compiler flags, environment variables, or runtime settings
> 4. The diagnostic codes issued when specific limits are exceeded
>
> Implementations **SHOULD** provide diagnostic warnings when programs approach limits (e.g., at 75% or 90% of capacity) to enable proactive refactoring.

#### 6.5.3 Exceeding Limits [conformance.limits.exceeding]

> When a program exceeds an implementation limit:
>
> 1. For limits with specified diagnostic codes (per Appendix B), implementations **MUST** issue the specified diagnostic.
> 2. For limits without specified diagnostics, implementations **MUST** issue an error diagnostic indicating which limit was exceeded.
> 3. Implementations **MUST NOT** produce executable artifacts for programs exceeding limits.
>
> Programs that exceed the minimum guaranteed limits (§6.5.1) but remain within an implementation's actual limits **MAY** be conforming for that implementation but are not portable across all conforming implementations.
>
> For limits to which this specification assigns explicit diagnostic codes in Appendix B, exceeding the implementation's documented limit **MUST** be diagnosed using those codes and **MUST NOT** be treated as IFNDR. For other limits, implementations **MAY** either reject the program with an error diagnostic in the appropriate `SRC` or `CNF` feature bucket or classify the program as IFNDR (§6.1.3.1) when detection is computationally infeasible. In all cases, behavior resulting from exceeding an implementation limit **MUST NOT** be classified as UVB.

### 6.6 Reserved Identifiers [conformance.reserved]

This section defines identifiers reserved by the language and implementations, establishing the boundary between user code and implementation space.

#### 6.6.1 Reserved Keywords [conformance.reserved.keywords]

> The following identifiers are reserved as keywords and **MUST NOT** be used as user-defined identifiers:
>
> Keywords are defined by the lexical grammar in Part II, §9. Using a reserved keyword as an identifier **MUST** trigger diagnostic code E-SRC-0305.
>
> The complete list of reserved keywords is specified in the normative grammar (Appendix A). Implementations **MUST** reserve all keywords defined in the grammar for the target language version.

#### 6.6.2 Reserved Namespaces [conformance.reserved.namespaces]

> The following namespace prefixes are reserved for specification-defined features and **MUST NOT** be used by user programs or vendor extensions:
>
> 1. The `cursive.*` namespace prefix (all identifiers beginning with `cursive.`)
> 2. Non-namespaced feature flag identifiers (feature flags without vendor domain notation)
> 3. Non-namespaced attribute identifiers (attributes without vendor domain notation)
>
> User programs using reserved namespace prefixes **MUST** trigger diagnostics. Module path components using reserved keywords **MUST** trigger diagnostic E-SRC-1103.
>
> Vendor extensions **MUST** use namespaced identifiers as specified in §8.3.3.

#### 6.6.3 Implementation Reservations [conformance.reserved.implementation]

> Implementations **MAY** reserve additional identifier patterns for internal use, provided such reservations are documented in the conformance dossier (§6.3.4).
>
> Common implementation reservation patterns include:
>
> 1. Identifiers beginning with double underscore (`__`)
> 2. Identifiers beginning with underscore followed by uppercase letter (`_[A-Z]`)
> 3. Vendor-specific namespace prefixes (e.g., `vendor.com.*`)
>
> Implementations **SHOULD** issue warnings when user programs use implementation-reserved patterns. Conforming programs **SHOULD NOT** use patterns that match common implementation reservations for maximum portability.

#### 6.6.4 Universe-Protected Bindings [conformance.reserved.universe]

> Universe-protected bindings are identifiers that belong to the language’s intrinsic namespace or to implementation-reserved namespaces that this specification or the implementation designates as not part of the user namespace. At minimum, the following identifiers **MUST** be treated as universe-protected:
>
> 1. The primitive scalar type names and other built-in type constructors defined by Part IV for the target language version.
> 2. All identifiers in the `cursive.*` namespace (§6.6.2).
> 3. Any identifier that matches an implementation-reserved pattern recorded as universe-protected in the implementation’s conformance dossier (Appendix C, §C.2.3).
>
> Implementations **MUST** treat universe-protected bindings as reserved for the purposes of binding introduction and shadowing. User declarations **MUST NOT** introduce or shadow bindings whose identifiers are universe-protected, and implementations **MUST** diagnose any such attempt with `E-SRC-1502`.

### 6.7 Conformance Verification [conformance.verification]

Conformance test suites, certification processes, and conformance claims are specified in Appendix E – Conformance Verification.

---

## 7. Language Evolution and Governance [evolution]

This chapter defines how the Cursive language evolves over time, including versioning policy, feature lifecycle management, extension mechanisms, and specification maintenance. It establishes the governance framework that ensures stability, predictability, and controlled evolution for implementations and programs.

### 7.1 Versioning Model [evolution.versioning]

This section defines the version numbering scheme, edition system, and version declaration requirements for the Cursive language.

#### 7.1.1 Semantic Versioning Policy [evolution.versioning.policy]

> The Cursive language version identifier **MUST** use semantic versioning with the format `MAJOR.MINOR.PATCH` where:
>
> - **MAJOR** version increments for incompatible changes that break existing conforming programs or change language semantics, including new keywords that were previously valid identifiers, changes to type system rules, or modifications to evaluation order.
> - **MINOR** version increments for backwards-compatible additions and clarifications that do not change the meaning of existing conforming programs, including new language features with new syntax, clarifications that resolve ambiguity without changing behavior, or additional diagnostic requirements.
> - **PATCH** version increments for backwards-compatible fixes and editorial corrections, including typo corrections, improved wording, formatting changes, or correction of internal cross-references.
>
> Implementations **MUST** document which language specification version(s) they support in their conformance dossier (Appendix C; see §6.3.4).

#### 7.1.2 Language Editions [evolution.versioning.editions]

> Language editions **MAY** be introduced to manage incompatible changes while allowing programs written for earlier editions to remain compilable.
>
> When editions are supported, implementations **MUST** allow programs to explicitly declare their target edition. Each edition **MUST** maintain internal consistency and **MUST NOT** mix semantics from different editions within a single compilation unit.
>
> Implementations supporting multiple editions **MUST** document the semantic differences between editions and provide migration guidance in their conformance dossier.

#### 7.1.3 Version Declaration [evolution.versioning.declaration]

> Programs **MUST** declare their target language version in the project manifest using the `language.version` field as specified in Part III, §13.3.
>
> Implementations **MUST** reject manifests without a `language.version` declaration. There is no implicit default version.
>
> The declared version **MUST** use semantic versioning format. Implementations **MUST** reject manifests where the declared MAJOR version does not match the implementation's supported MAJOR version.
>
> Implementations **MAY** accept programs declaring older MINOR versions within the same MAJOR version, provided backward compatibility is maintained per §7.1.4.
>
> When the `language.version` is specified in the manifest, violations of these requirements **MUST** be diagnosed using source-manifest diagnostics in the `SRC-11` feature bucket (e.g., `E-SRC-1107`). When the target language version is supplied exclusively via other configuration channels (such as command-line flags or environment variables), incompatible versions **MUST** be diagnosed using `E-CNF-0301`.

#### 7.1.4 Compatibility Guarantees [evolution.versioning.compatibility]

> Implementations **MUST** maintain source-level backward compatibility for conforming programs across MINOR version increments within the same MAJOR version, subject to deprecation timelines (§7.2.4).
>
> Implementations **MUST NOT** silently change the semantics of existing conforming programs when processing them under a newer MINOR version. Changes requiring semantic alterations **MUST** trigger MAJOR version increments.
>
> Implementations **MAY** issue warnings for constructs that will be deprecated in future versions, provided such warnings do not prevent successful compilation.

#### 7.1.5 Version Migration [evolution.versioning.migration]

> When MAJOR version increments introduce breaking changes, implementations **SHOULD** provide automated migration tooling to assist in updating programs from the previous MAJOR version.
>
> Migration tooling **SHOULD** handle syntactic transformations, deprecated feature replacements, and semantic updates where feasible. Migration tooling **MUST NOT** introduce silent behavioral changes without user confirmation.
>
> Implementations **MUST** document migration procedures, including cases requiring manual intervention, in release notes and migration guides.

### 7.2 Feature Lifecycle [evolution.lifecycle]

This section defines stability classifications for language features, deprecation policies, and the process for feature removal.

#### 7.2.1 Stability Classes [evolution.lifecycle.stability]

> Every language feature **MUST** be classified into one of three stability classes:
>
> 1. **Stable**: Features available by default without opt-in. Stable features **MUST NOT** introduce breaking changes except in MAJOR version increments. Stable features **MUST** be documented in this specification.
>
> 2. **Preview**: Features intended for the next stable release, requiring explicit opt-in via feature flags (§7.3.2). Preview features **MAY** change between MINOR versions. Preview features **MUST** be documented with a `[Preview: target_version]` annotation indicating the version in which they are planned to stabilize.
>
> 3. **Experimental**: Highly unstable features guarded by feature flags. Experimental features **MAY** change significantly or be removed without following deprecation timelines. Experimental features **MUST NOT** be used in production code without explicit acknowledgment of stability risks.
>
> When an experimental feature is enabled for a compilation unit, implementations **SHOULD** emit at least one `W-CNF-0101` diagnostic per compilation configuration indicating that an experimental feature is in use, unless diagnostics for such warnings are explicitly suppressed.
>
> The stability class of each feature **MUST** be documented in this specification or in implementation-specific extension documentation.

#### 7.2.2 Feature Stabilization and Migration [evolution.lifecycle.stabilization]

> Preview features **MAY** transition to Stable status in a MINOR version increment after the preview period, provided no breaking changes to the preview semantics are required.
>
> If breaking changes are required during stabilization, the feature **MUST** either undergo another preview cycle or be deferred to the next MAJOR version.
>
> Experimental features **MUST** transition through Preview status before becoming Stable, except for features introduced in MAJOR version increments where the preview period may be shortened or bypassed.
>
> For every deprecated or removed feature, implementations **MUST** provide documentation describing the recommended replacement or migration strategy. Migration documentation **MUST** include code examples demonstrating the deprecated construct and its replacement. Where automated migration is feasible, implementations **SHOULD** provide tooling support.

#### 7.2.3 Deprecation and Removal [evolution.lifecycle.deprecation]

> Features **MAY** be deprecated when better alternatives exist, when they introduce maintenance burden, or when they conflict with language design principles.
>
> Deprecated features **MUST** remain functional and conforming for at least one MINOR version after deprecation, marked with the `[!caution] Deprecated` annotation (§2.2) including removal version.
>
> Features deprecated in version `X.Y.0` **MUST NOT** be removed before `X.(Y+2).0` (MINOR deprecations) or `(X+1).0.0` (MAJOR deprecations), ensuring at least one full MINOR version for migration.
>
> Implementations **SHOULD** issue `W-CNF-0102` diagnostics when deprecated features are used and **SHOULD** issue `W-CNF-0301` diagnostics when deprecated syntax forms are used, unless such warnings are explicitly suppressed (e.g., via diagnostic configuration). Implementations **MUST** document deprecated features in release notes with deprecation version, removal version, rationale, and migration path.
>
> Features **MAY** be removed only in MAJOR version increments, except for experimental features which **MAY** be removed in any version. Removed features **MUST** trigger error diagnostics (severity `E`, Part II §12.3.1) when encountered. The diagnostic code for the removed feature **MUST** be marked as reserved in Appendix D and **MUST NOT** be reused for different violations (Part II §12.2.4).
>
> Implementations **MAY** provide compatibility modes or feature flags to temporarily restore removed features, provided such modes are clearly documented as non-conforming extensions.

#### 7.2.4 Critical Security Deprecation [evolution.lifecycle.security]

> Features with discovered security vulnerabilities or fundamental safety defects **MAY** follow an expedited deprecation timeline.
>
> Security deprecations **MAY** be announced and removed within a single MINOR version increment, provided the vulnerability and rationale are publicly documented.
>
> Implementations **MUST** issue error diagnostics for uses of security-deprecated features once those features are disabled, using either the feature’s removal diagnostic or `E-CNF-0301` when policy disables the feature independently of version removal, unless an explicit opt-out flag is provided. Such opt-out flags **MUST** be documented as enabling potentially unsafe behavior.

### 7.3 Extension System [evolution.extensions]

This section defines how implementations may extend the language beyond the core specification while preserving portability and conformance.

#### 7.3.1 Extension Mechanisms [evolution.extensions.mechanisms]

> Implementations **MAY** provide language extensions through:
>
> 1. **Attributes**: Syntactic annotations using the `[[attribute_name]]` syntax that modify declarations without altering core language semantics (see Part VIII for reflection and attribute requirements).
> 2. **Compiler-specific features**: Additional language constructs, built-in types, or intrinsic functions not defined in this specification.
> 3. **Preview features**: Specification-defined features in preview stability class (§7.2.1) requiring explicit opt-in.
>
> Extensions **MUST NOT** alter the meaning of conforming programs written without the extension. Extensions **MUST NOT** suppress required diagnostics defined in this specification.

#### 7.3.2 Feature Flags [evolution.extensions.flags]

> Extensions requiring opt-in **MUST** be controlled through feature flags declared in the project manifest or via compiler command-line options.
>
> Feature flags **MUST** use a hierarchical dotted identifier format (e.g., `"vendor.category.feature"`). Flags for specification-defined preview features **MUST** use identifiers without vendor prefixes.
>
> The manifest schema **MUST** support a `[features]` table with an `enable` array listing feature flag identifiers. All available feature flags and their effects **MUST** be documented in the conformance dossier (Appendix C; see §6.3.4).
>
> Unknown feature flag identifiers **MUST** trigger an error diagnostic (`E-CNF-0101`) in conforming compilation modes. Implementations **MUST NOT** silently ignore unknown feature flags or treat them as enabled features. Implementations **MAY** provide non-conforming compatibility modes that relax this requirement, provided such modes are clearly documented as non-conforming and their use in production builds **MAY** trigger `W-CNF-0201` diagnostics.
>
> When a program uses a feature that this specification or an implementation designates as requiring explicit enablement, and the corresponding feature flag is not enabled in the project manifest or compilation configuration, the implementation **MUST** reject the program with an `E-CNF-0103` diagnostic.

#### 7.3.3 Extension Namespacing [evolution.extensions.namespacing]

> Vendor-specific extensions **MUST** use reverse-domain notation (e.g., `vendor.com.feature`). Attribute names and feature flag identifiers for vendor extensions **MUST** include this namespace.
>
> The `cursive.*` namespace is reserved for specification-defined features. Implementations **MUST NOT** define extensions using reserved namespaces.

#### 7.3.4 Extension Conformance [evolution.extensions.conformance]

> Programs using specification-defined preview or experimental features (§7.2.1) **MAY** be conforming provided:
>
> 1. The features are explicitly enabled via feature flags (§7.3.2).
> 2. The program satisfies all requirements for those features as documented.
> 3. The program does not rely on undocumented behavior.
>
> Programs using vendor-specific extensions **MAY** be considered conforming within the vendor's ecosystem, provided the vendor documents the extension's behavior in a conformance dossier following the same structure as §6.3.4.
>
> Conformance claims for programs using extensions **MUST** enumerate all enabled extensions and their versions.

#### 7.3.5 Extension Standardization [evolution.extensions.standardization]

> Successful vendor extensions **MAY** be considered for adoption into the core specification through the change proposal process (§7.4.2).
>
> Extensions considered for standardization **MUST** demonstrate:
>
> 1. Proven utility across multiple codebases or domains.
> 2. Implementation experience from at least one production-quality implementation.
> 3. Compatibility with language design principles (§cursive-language-design in project documentation).
> 4. Absence of conflicts with existing or planned specification features.
>
> Upon adoption, standardized features **MUST** transition through preview stability (§7.2.1) before becoming stable, unless introduced as part of a MAJOR version increment.

### 7.4 Specification Maintenance [evolution.specification]

This section defines processes for maintaining and evolving the specification itself through errata, change proposals, and breaking change management.

#### 7.4.1 Errata Process [evolution.specification.errata]

> Errors in the specification (typos, ambiguities, contradictions, omissions) **SHOULD** be reported through the specification's issue tracking system.
>
> Confirmed errata **MUST** be corrected in a PATCH version increment (§7.1.1). Errata corrections **MUST NOT** introduce behavioral changes; corrections requiring behavioral changes **MUST** be treated as MINOR or MAJOR changes per semantic versioning policy.
>
> All errata **MUST** be documented in a normative change log recording:
>
> 1. The version in which the error appeared.
> 2. The version in which it was corrected.
> 3. Description of the error and correction.
> 4. Affected section references.
>
> Implementations **SHOULD** adopt errata promptly. When implementation behavior diverges from corrected specification text, the divergence **MUST** be documented in the conformance dossier as implementation-defined behavior (§6.1.4).

#### 7.4.2 Change Proposal Process [evolution.specification.proposals]

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
> Proposals **SHOULD** undergo public review period before acceptance. Accepted proposals **MUST** be integrated into the next appropriate specification version per semantic versioning policy (§7.1.1).

#### 7.4.3 Breaking Change Policy [evolution.specification.breaking]

> Changes that invalidate previously conforming programs **MUST** be classified as breaking changes and **MUST** trigger MAJOR version increments (§7.1.1).
>
> Breaking changes **MUST** follow the deprecation timeline (§7.2.4) when feasible, giving users at least one MINOR version cycle to prepare.
>
> Breaking changes **MUST** be thoroughly documented with:
>
> 1. Rationale justifying the incompatibility.
> 2. Estimated impact on existing codebases.
> 3. Detailed migration guidance.
> 4. Alternatives considered and reasons for rejection.
>
> Breaking changes **SHOULD** be batched in MAJOR releases to minimize disruption. Gratuitous breaking changes without substantial benefits **SHOULD NOT** be accepted.

#### 7.4.4 Anchor Stability and Linting [evolution.specification.anchors]

> Semantic anchors in square brackets (for example, `[conformance.obligations]`, `[modules.manifest.format]`) **MUST** be treated as part of the normative API surface of this specification. Within a given MAJOR.MINOR specification series, anchors **MUST NOT** be removed or renamed except as part of a PATCH release that documents the change in the change log (§7.4.1).  
>
> The specification build process **SHOULD** include an “anchor linter” that verifies:
>
> 1. All internal cross-references to section numbers and anchors resolve; and  
> 2. No anchors are duplicated or dangling.
>
> Release artifacts for each specification version **SHOULD** include the anchor-linter report so that tool authors can validate that their links remain correct.
# Part II - Source Text & Translation

This part defines how source text is processed into executable programs, covering character encoding, lexical analysis, syntactic structure, the multi-phase translation pipeline, and the diagnostic system for reporting violations.

*(Informative)*

| Theme                                                                                                 | Key Diagnostics                                                                                                           |
| ----------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------- |
| Source files are valid UTF-8, contain only permitted code points, and respect size limits.            | `E-SRC-0101–0104`, `E-SRC-0102`                                                                                           |
| Line endings and physical structure are normalized before tokenization.                               | `E-SRC-0401`, `E-SRC-0402`                                                                                                |
| Lexical analysis produces well-formed tokens; malformed literals and comments are diagnosed.          | `E-SRC-0301–0307`                                                                                                         |
| Translation phases execute in a deterministic order and enforce compile-time resource limits.         | `E-SRC-0201–0208`, `E-EXP-0401–0403`                                                                                      |
| Module discovery, manifests, imports, scopes, and name lookup obey the structural rules of this Part. | `E-SRC-0501`, `E-SRC-1101–1108`, `E-SRC-1201–1205`, `E-SRC-1301–1303`, `E-SRC-1401`, `E-SRC-1501–1502`, `E-SRC-1601–1605` |

## 8. Source Text and Encoding [source]

This chapter defines requirements for source file encoding, structure, and ingestion.

For the purposes of this part:

- A _source byte stream_ is the raw sequence of bytes read from a file or other input.
- A _normalized source file_ is the sequence of Unicode scalar values obtained from a source byte stream after the preprocessing pipeline in §8.1.0 (UTF-8 decoding, optional BOM removal, and line-ending normalization) has successfully completed.
- A _compilation unit_ is defined in §8.3.1 [source.inclusion.units].

> Violations of any requirement in §8.1–§8.2 that result in an `SRC-01` error diagnostic (including `E-SRC-0101`–`E-SRC-0104` and errors for exceeding documented maximum file size, logical line count, or line length) **MUST** cause the affected compilation unit to be treated as ill-formed under §6.3.3. Such violations **MUST NOT** be classified as Unverifiable Behavior (UVB) or Unspecified Behavior (USB). Variation in behavior for this chapter is permitted only where this specification explicitly designates implementation-defined behavior (IDB), and those choices **MUST** be documented in the implementation’s conformance dossier (§6.1.4, §6.3.4).

### 8.1 Character Encoding [source.encoding]

#### 8.1.0 Preprocessing Pipeline [source.encoding.pipeline]

> Source text preprocessing MUST execute in the following order:
>
> (1) **File size validation** (§8.2.2 [source.structure.size]): Implementations MUST enforce the implementation-defined maximum byte length for the source byte stream;
>
> (2) **UTF-8 decoding and validation** (§8.1.1 [source.encoding.utf8]): The byte stream MUST be decoded as UTF-8;
>
> (3) **BOM removal** (§8.1.2 [source.encoding.bom]): If the first decoded scalar value is U+FEFF, it MUST be stripped;
>
> (4) **Line ending normalization** (§8.2.1 [source.structure.lines]): CR, LF, and CRLF sequences MUST be normalized to LF;
>
> (5) **Prohibited code point validation** (§8.1.3 [source.encoding.invalid]): Control characters MUST be validated;
>
> (6) **Physical structure validation** (§8.2.3 [source.structure.physical]): The normalized stream MUST conform to the EBNF grammar.
>
> Unicode normalization (§8.1.4 [source.encoding.normalization]), being implementation-defined, **MAY** be applied only after UTF-8 decoding and line-ending normalization (steps (2)–(4)) and **MUST** be accounted for in a way that does not change the byte offsets or logical line/column coordinates used for diagnostics. Implementations **MAY** maintain a separate normalized view of the decoded scalar sequence for comparisons while computing all diagnostic locations with respect to the pre-normalized decoded sequence.

#### 8.1.1 UTF-8 Requirements [source.encoding.utf8]

> Cursive source input MUST be a sequence of Unicode scalar values encoded as UTF-8 as specified in ISO/IEC 10646 and The Unicode Standard, Version 14.0 or later.
>
> Implementations MUST accept only byte streams that decode to legal UTF-8 sequences conforming to the encoding scheme specified in RFC 3629. A byte sequence is a legal UTF-8 sequence if and only if it satisfies the following conditions:
>
> (a) Each code unit sequence correctly encodes a Unicode scalar value (U+0000 through U+D7FF and U+E000 through U+10FFFF);
>
> (b) The encoding uses the shortest possible representation for each scalar value (overlong encodings are forbidden);
>
> (c) No byte sequence encodes a surrogate code point (U+D800 through U+DFFF).
>
> Invalid byte sequences MUST trigger diagnostic E-SRC-0101. The diagnostic MUST identify the byte offset of the first invalid byte in the source file.

#### 8.1.2 BOM Handling [source.encoding.bom]

> If a source file begins with the UTF-8 byte order mark (BOM, U+FEFF) as the first decoded scalar value, the implementation MUST strip the BOM before lexical analysis. A BOM appearing at any scalar value position after the first MUST trigger diagnostic E-SRC-0103.
>
> When a source file begins with a UTF-8 BOM and otherwise satisfies the constraints of §8.1 and §8.2, a conforming implementation SHOULD emit warning diagnostic W-SRC-0101 (UTF-8 BOM present) while still accepting the file.

#### 8.1.3 Invalid Sequences [source.encoding.invalid]

> Source files **MUST NOT** contain prohibited code points. After line ending normalization (§8.2.1 [source.structure.lines]), validation of prohibited code points **MUST** be performed on the normalized sequence of Unicode scalar values and **MUST NOT** depend on lexical context.
>
> Prohibited code points are:
>
> - The null character (U+0000) in any context; and
> - Any Unicode scalar value whose general category is `Cc` (control), **except**:
>   - Horizontal tab (U+0009)
>   - Line feed (U+000A)
>   - Form feed (U+000C)
>   - Carriage return (U+000D)
>
> These constraints **MUST** apply uniformly in all parts of the source file, including comments, string literals, and character literals. Validation of prohibited code points **MUST** occur after UTF-8 decoding and line ending normalization but before lexical analysis (§11.2 [translation.ingestion]). A source file that contains at least one prohibited code point **MUST** trigger diagnostic `E-SRC-0104` and **MUST** be treated as an ill-formed compilation unit under §6.3.3.

#### 8.1.4 Normalization [source.encoding.normalization]

> Unicode normalization of source text outside identifiers and module-path components is implementation-defined behavior (IDB). Implementations SHOULD accept source files in any Unicode normalization form (NFC, NFD, NFKC, or NFKD) and MUST document any normalization they perform in the conformance dossier (cf. §6.3 [conformance.obligations]), including the Unicode Standard version (and any Unicode Standard Annexes, such as UAX #31) used for validation and identifier classification.
>
> For identifier lexemes and module-path components, implementations MUST normalize the corresponding scalar sequences to Unicode Normalization Form C (NFC) prior to equality comparison, hashing, or name lookup, and MUST apply NFC consistently in all translation phases for these purposes.
>
> Any normalization performed **MUST NOT** change logical line boundaries or the byte offsets used when reporting diagnostic locations, and **MUST NOT** be applied to the interior of string or character literal lexemes; the scalar values of literals are derived directly from the decoded source together with escape processing. Implementations that maintain both a pre-normalized and normalized view of the source **MUST** compute all diagnostic locations and spans with respect to the pre-normalized decoded sequence as produced by §8.1.0.

#### 8.1.5 Diagnostic Location Requirements [source.encoding.locations]

> Diagnostics in the `SRC-01` feature bucket MUST report locations as follows:
>
> (a) `E-SRC-0101` (invalid UTF-8 byte sequence) MUST report a source location whose byte offset corresponds to the first invalid byte in the source file as required by §8.1.1 [source.encoding.utf8].
>
> (b) `E-SRC-0102` (source file exceeds implementation-defined maximum size) MUST be associated with the offending source file; when the implementation does not read file contents, it MAY report a conventional line and column (for example, line 1, column 1) while still identifying the file path.
>
> (c) `E-SRC-0103` (embedded BOM) MUST reference the source location of the first U+FEFF scalar value that appears after the first position in the decoded scalar sequence.
>
> (d) `E-SRC-0104` (forbidden control character or null byte) MUST reference the first prohibited code point encountered in the normalized source file.
>
> (e) `E-SRC-0105` (maximum logical line count exceeded) and `E-SRC-0106` (maximum line length exceeded), which report exceeding an implementation-defined maximum logical line count or maximum line length (§8.2.2 [source.structure.size]), **MUST** be associated with the first logical line or column that violates the documented limit.
>
> The warning diagnostic `W-SRC-0101` (UTF-8 BOM present) MUST be associated with the first scalar position of the file (line 1, column 1).

### 8.2 Source File Structure [source.structure]

#### 8.2.1 Line Endings [source.structure.lines]

> Implementations MUST recognize and normalize all three common line-ending sequences (LF, CR, CRLF) to a single canonical Line Feed character (U+000A) before tokenization. The normalization algorithm operates as follows:
>
> (a) Each occurrence of the two-character sequence CR LF (U+000D U+000A) MUST be replaced with a single LF (U+000A);
>
> (b) Each standalone CR (U+000D) not followed by LF MUST be replaced with a single LF (U+000A);
>
> (c) Each LF (U+000A) that does not follow a CR remains unchanged.
>
> Mixed line endings are permitted within a single source file.

#### 8.2.2 Maximum File Size [source.structure.size]

> Implementations **MUST** enforce an implementation-defined maximum byte length per source input. Conforming implementations **MUST** accept source files of at least 1 mebibyte (1 MiB = 2²⁰ bytes = 1,048,576 bytes). Each implementation **MUST** document its maximum in the conformance dossier (cf. §6.3 [conformance.obligations]); any choice of maximum above this minimum is implementation-defined behavior (IDB).
>
> For source inputs whose byte length is available prior to reading (for example, regular files), implementations **MUST** compare that length against the documented maximum before attempting UTF-8 decoding. If the length exceeds the documented maximum, the implementation **MUST** emit diagnostic `E-SRC-0102` and **MUST NOT** attempt to decode the file.
>
> For source inputs whose length is not available in advance (for example, non-seekable streams), implementations **MUST** track the number of bytes read and **MUST** emit diagnostic `E-SRC-0102` as soon as the accumulated length exceeds the documented maximum. In all cases, a compilation unit that triggers `E-SRC-0102` **MUST** be treated as ill-formed in the sense of §6.3.3 and **MUST NOT** yield executable artifacts.

> Implementations **MUST** also document an implementation-defined maximum logical line count and maximum line length (in Unicode scalar values) per source file. Conforming implementations **MUST** accept source files containing at least 65 536 logical lines and **MUST** accept lines containing at least 16 384 Unicode scalar values. Exceeding the documented maximum line count **MUST** be diagnosed as `E-SRC-0105` (Maximum logical line count exceeded), exceeding the documented maximum line length **MUST** be diagnosed as `E-SRC-0106` (Maximum line length exceeded), and either violation **MUST** render the affected compilation unit ill-formed.

> [!tip] Rationale
> Explicit limits on line count and line length provide deterministic worst-case bounds for lexing and tooling while setting generous minima to preserve expressiveness for large or generated sources. Treating violations as `SRC-01` errors keeps these constraints consistent with other source-ingestion rules and ensures that resource-limit failures are diagnosed, not left to implementation-specific crashes or undefined behavior.

#### 8.2.3 Physical Structure [source.structure.physical]

> A source file MUST conform to the following EBNF grammar after UTF-8 decoding, BOM removal, and line-ending normalization, but before tokenization:
>
> **_Syntax:_**
>
> ```ebnf
> <source_file>
>     ::= <normalized_line>*
>
> <normalized_line>
>     ::= <code_point>* <line_terminator>?
>
> <line_terminator>
>     ::= U+000A
>
> <code_point>
>     ::= /* any Unicode scalar value other than U+000A and those prohibited by §8.1.3 */
> ```
>
> The non-terminal `<code_point>` denotes any Unicode scalar value that satisfies the constraints specified in §8.1.3 [source.encoding.invalid] and is not U+000A. The non-terminal `<line_terminator>` represents the normalized line-ending character produced by §8.2.1 [source.structure.lines].

### 8.3 Source Inclusion Model [source.inclusion]

#### 8.3.1 Compilation Units [source.inclusion.units]

> A compilation unit is the syntactic content of a single source file after the source text preprocessing defined in §8.1 and §8.2. Each source file corresponds to exactly one compilation unit. There is a one-to-one correspondence between source files and compilation units throughout the compilation process.

#### 8.3.2 No Header/Include Mechanism [source.inclusion.noheaders]

> Cursive provides no textual inclusion or preprocessing mechanism. Source files MUST NOT be combined, merged, or textually substituted during translation. Implementations MUST NOT provide any textual inclusion, preprocessing, or macro expansion facility as part of the Cursive language.

---

## 9. Lexical Structure [lexical]

This chapter defines tokenization and lexical elements.

### 9.1 Lexical Elements [lexical.elements]

#### 9.1.1 Token Categories [lexical.elements.tokens]

> Lexical analysis **MUST** consume the normalized source file produced by §8.1 and §8.2 and **MUST** produce a finite, ordered sequence of tokens. Each token **MUST** record at least: a token kind, the exact source lexeme, and a source span (file, start line/column, end line/column).
>
> The lexer **MUST** classify every non-comment, non-whitespace fragment of the normalized source as exactly one of the following token kinds:
>
> 1. `<identifier>` — an identifier satisfying §9.3
> 2. `<keyword>` — a reserved word listed in §9.2.1
> 3. `<literal>` — a numeric, string, or character literal as defined in §9.4
> 4. `<operator>` — an operator symbol as defined in §9.2.2
> 5. `<punctuator>` — a punctuation symbol as defined in §9.2.2
> 6. `<newline>` — a line terminator token corresponding to U+000A as defined in §8.2.1 and §9.1.2
>
> Any maximal character sequence that cannot be classified as one of these token kinds **MUST** cause the implementation to emit at least one error diagnostic in the `SRC-03` feature bucket (Appendix B) and **MUST** be treated as ill-formed source; the implementation **MUST NOT** silently split such a sequence into multiple tokens to avoid diagnostics.

```ebnf
<token>
    ::= <identifier>
     | <keyword>
     | <literal>
     | <operator>
     | <punctuator>
     | <newline>
```

> The tokenization process **MUST** be deterministic for a given normalized source file and compilation configuration: repeated compilations **MUST** produce the same token sequence, except where implementation-defined behavior explicitly permits variation that is documented in the conformance dossier (§6.1.4, Appendix C).

#### 9.1.2 Whitespace Handling [lexical.elements.whitespace]

> After line-ending normalization (§8.2.1), the only whitespace code points that lexical analysis **MUST** treat specially are: space (U+0020), horizontal tab (U+0009), form feed (U+000C), and line feed (U+000A). Space, horizontal tab, and form feed **MUST** act solely as token separators and **MUST NOT** be emitted as tokens.
>
> Each line feed (U+000A) that is not part of a line comment terminator (§9.1.3) **MUST** be represented in the token stream as a `<newline>` token. Implementations **MUST NOT** reorder, insert, or remove `<newline>` tokens relative to non-comment tokens, except when discarding entire ill-formed constructs that have already been diagnosed.
>
> Outside string and character literals (§9.4), implementations **MUST NOT** treat any additional Unicode whitespace or control characters as having special lexical meaning; code points not permitted by §8.1.3 **MUST** already have been rejected in the preprocessing pipeline (`E-SRC-0104`) and **MUST NOT** reach lexical analysis.
>
> Statement-termination and continuation behavior **MUST** be defined only in terms of `<newline>` tokens, semicolons, and delimiter/continuation predicates as specified in §10.2 and §11.4; implementations **MUST NOT** introduce additional implicit terminators based on other whitespace.
>
> The `<newline>` token kind described in this section corresponds to the `<newline>` lexical production in Appendix A, §A.1.2.

#### 9.1.3 Comments [lexical.elements.comments]

> A line comment **MUST** begin with the two-character sequence `//` that is not inside a string or character literal and **MUST** extend to but not include the next line feed (U+000A) or end of file, whichever comes first. Line comments **MUST NOT** produce tokens; the characters they consume **MUST** be discarded by lexical analysis.
>
> Line comments whose first three characters are `///` **MUST** be classified as declaration documentation comments, and line comments whose first three characters are `//!` **MUST** be classified as module documentation comments. Documentation comments **MUST** be preserved and associated with the immediately following declaration or module (subject to the rules of Part III and Appendix F) but **MUST NOT** appear as ordinary tokens in the token stream.
>
> A block comment **MUST** be delimited by the two-character sequence `/*` and the two-character sequence `*/`, neither of which may appear inside a string or character literal for the purposes of comment recognition. Block comments **MUST** nest: each `/*` encountered while inside a block comment **MUST** increase a nesting-depth counter by one, and each `*/` **MUST** decrease it by one.
>
> If end of file is reached while the block-comment nesting depth is non-zero, the implementation **MUST** emit diagnostic `E-SRC-0306` (Unterminated block comment) at the source location of the `/*` that began the unterminated outermost comment and **MUST** treat the compilation unit as ill-formed. Implementations **MAY** additionally highlight the location of the end-of-file or last line as a related note diagnostic.
>
> Characters that appear within any kind of comment **MUST NOT** contribute to token formation, delimiter nesting depth (§10.2.4), or identifier and literal content, and **MUST NOT** change the semantics of statement termination except through the `<newline>` token that terminates a line comment.

```ebnf
<line_comment>
    ::= "//" <comment_char>* <newline>?

<block_comment>
    ::= "/*" (<block_comment> | <comment_char>)* "*/"
```

> The metavariable `<comment_char>` in the grammar above denotes any Unicode scalar value other than the start or end delimiters of the corresponding comment kind and other than line feed (U+000A) where prohibited by §8.1.3.

#### 9.1.4 Lexically Sensitive Unicode Characters [lexical.elements.security]

> Certain Unicode code points are considered *lexically sensitive* because they may alter the visual appearance of source code without changing its tokenization. At minimum, the following code points and code point classes **MUST** be treated as lexically sensitive:
>
> (a) Bidirectional formatting characters (for example, U+202A–U+202E and U+2066–U+2069);
>
> (b) Zero-width joiner (U+200D) and zero-width non-joiner (U+200C);
>
> (c) Any additional characters that an implementation documents as lexically sensitive in its conformance dossier.
>
> When a lexically sensitive character appears unescaped in an identifier (§9.3), operator or punctuator lexeme (§9.2.2), or immediately adjacent to token boundaries in non-comment, non-literal context, the implementation **MUST** emit warning diagnostic `W-SRC-0308` (Lexically sensitive Unicode character in identifier or token boundary) in the `SRC-03` feature bucket and **MUST** identify the location of the first affected code point. In strict conformance modes (§6.3.7), implementations **MUST** upgrade this condition to error diagnostic `E-SRC-0308` and **MUST NOT** accept the affected compilation unit while such diagnostics remain outstanding. Diagnostic messages **SHOULD** describe the class of lexically sensitive character involved.
>
> Lexically sensitive characters that appear only within string or character literals (§9.4) or are introduced via explicit escape sequences MUST NOT, by themselves, affect program well-formedness, but implementations MAY still emit non-error diagnostics (notes or warnings) for such occurrences as quality-of-implementation features.

### 9.2 Keywords and Operators [lexical.keywords]

#### 9.2.1 Reserved Keywords [lexical.keywords.reserved]

> The following lexemes **MUST** be treated as reserved keywords in this version of the specification and **MUST NOT** be used where an identifier is expected: `abstract`, `as`, `async`, `await`, `behavior`, `break`, `by`, `case`, `comptime`, `const`, `continue`, `contract`, `defer`, `else`, `enum`, `exists`, `false`, `forall`, `grant`, `if`, `import`, `internal`, `invariant`, `let`, `loop`, `match`, `modal`, `module`, `move`, `must`, `new`, `none`, `private`, `procedure`, `protected`, `public`, `record`, `region`, `result`, `select`, `self`, `Self`, `shadow`, `shared`, `state`, `static`, `true`, `type`, `unique`, `var`, `where`, `will`, `with`, `witness`.
>
> Implementations **MUST** recognize each reserved keyword as a `<keyword>` token rather than an `<identifier>` token during lexical analysis. Wherever the grammar of Part III–Part V requires an `<identifier>`, the appearance of a reserved keyword **MUST** be rejected and diagnostic `E-SRC-0305` (Reserved keyword used as identifier) **MUST** be emitted.
>
> The set of reserved keywords defined above **MUST** be identical across conforming implementations for a given language version. Implementations **MUST NOT** reserve additional keywords or unreserve any of the listed keywords except through a future revision of this specification that explicitly changes the list.

#### 9.2.2 Operators and Punctuators [lexical.keywords.operators]

> The lexer **MUST** recognize the following multi-character operator and punctuator tokens using a maximal-munch strategy (longest valid token first), subject to grammatical disambiguation rules in §10.2 and Appendix A:
>
> - `==`, `!=`, `<=`, `>=`, `&&`, `||`, `<<`, `>>`, `<<=`, `>>=`
> - `..`, `..=`, `=>`, `**`, `->`, `::`
>
> The lexer **MUST** also recognize at least the following single-character operator and punctuator tokens: `+`, `-`, `*`, `/`, `%`, `<`, `>`, `=`, `!`, `&`, `|`, `^`, `.`, `,`, `:`, `;`, `(`, `)`, `[`, `]`, `{`, `}`. Additional operator or punctuator tokens **MAY** be defined by later parts of this specification, but any such additions **MUST** appear in Appendix A and **MUST NOT** conflict with the tokens listed here.
>
> When multiple tokenizations are possible at a character position, the lexer **MUST** emit the longest token whose kind is permitted at that position by the grammar, except when parsing generic type arguments or similar contexts requires treating `>>` as two closing `>` tokens; in such contexts, the implementation **MAY** split a single lexeme `>>` into two logical punctuators for the purposes of parsing while preserving the original lexeme for diagnostics and tooling. This maximal-munch behavior **MUST** be consistent with the statement-termination rules in §10.2 and the grammar in Appendix A.

### 9.3 Identifiers [lexical.identifiers]

#### 9.3.1 XID_Start and XID_Continue Rules [lexical.identifiers.xid]

> Identifier tokens **MUST** be sequences of Unicode scalar values that satisfy the identifier recommendations of [UAX31] as constrained in this section. The first code point of an identifier **MUST** be either (a) a code point with `XID_Start = Yes` in the Unicode Standard, or (b) U+005F LOW LINE (`_`). Each subsequent code point **MUST** have `XID_Continue = Yes` or be U+005F.
>
> Identifiers **MUST NOT** contain code points that are prohibited by §8.1.3, surrogate code points (U+D800–U+DFFF), or non-characters. If a candidate identifier contains any code point that violates these conditions, the implementation **MUST** emit diagnostic `E-SRC-0307` (Invalid Unicode in identifier) at the first offending code point and **MUST** treat the identifier as ill-formed.
>
> Reserved keywords (§9.2.1) **MUST NOT** be treated as identifiers even if they satisfy the `XID_Start`/`XID_Continue` constraints; they **MUST** always be tokenized as `<keyword>` and rejected in identifier positions with `E-SRC-0305`.

```ebnf
<identifier>
    ::= <ident_start> <ident_continue>*
```

> The metavariables `<ident_start>` and `<ident_continue>` above denote the sets of code points described in this section; the concrete character classes used by an implementation **MUST** correspond to the `XID_Start` and `XID_Continue` properties of the Unicode version documented in its conformance dossier (§8.1.4, Appendix C).

#### 9.3.2 Length Limits [lexical.identifiers.length]

> Implementations **MUST** accept identifiers whose length (measured in Unicode scalar values) is at least 1,023 code points (§6.5.1). Each implementation **MUST** document in its conformance dossier the maximum identifier length it supports; this documented limit **MUST NOT** be less than 1,023.
>
> If a source file contains an identifier whose length exceeds the implementation’s documented limit, the implementation **MAY** classify the program as IFNDR (§6.1.3.1) or **MAY** reject the program as ill-formed. When the program is rejected for this reason, the implementation **MUST** emit at least one error diagnostic in the `SRC-03` feature bucket whose code is reserved in Appendix D and **MUST** identify the offending identifier’s source span.

#### 9.3.3 Unicode Normalization [lexical.identifiers.normalization]

> Unicode normalization behavior for identifiers and module-path components **MUST** follow the general rules of §8.1.4. Implementations **MUST** normalize every identifier lexeme to Unicode Normalization Form C (NFC) before comparing, hashing, or inserting it into any name table, and **MUST** apply NFC consistently to all such lexemes within a compilation.
>
> Two identifier occurrences **MUST** be considered the same binding name if and only if their NFC-normalized forms are identical sequences of Unicode scalar values. All name-comparison operations (including module path comparison, scope formation, and diagnostic reporting) **MUST** use this same NFC-normalized form.
>
> Implementations **MUST NOT** perform identifier normalization in a way that changes logical line boundaries or the byte offsets used for reporting diagnostic locations, and **MUST NOT** normalize identifiers differently in different phases of translation.

### 9.4 Literals [lexical.literals]

#### 9.4.1 Numeric Literals [lexical.literals.numeric]

> A numeric literal token **MUST** match one of the integer or floating-point forms defined in this section. Any maximal sequence of characters that begins with a digit or a base prefix and fails to match one of these forms **MUST** be diagnosed as `E-SRC-0304` (Malformed numeric literal) and **MUST NOT** be silently split into multiple tokens.
>
> A decimal integer literal **MUST** consist of a non-empty sequence of decimal digits (`0`–`9`), optionally separated by single underscores (`_`) between digits. Underscores **MUST NOT** appear at the beginning or end of the literal, immediately after a base prefix, immediately before or after an exponent marker, or immediately before a type suffix.
>
> Hexadecimal (`0x`), octal (`0o`), and binary (`0b`) integer literals **MUST** begin with the corresponding two-character prefix followed by at least one digit of the appropriate base; digits appearing after a base prefix **MUST** all be valid for that base. Violations of these requirements (missing digits after the prefix or invalid digits for the base) **MUST** be diagnosed as `E-SRC-0304`.
>
> Floating-point literals **MUST** consist of an integer part, an optional fractional part introduced by `.`, an optional exponent part introduced by `e` or `E` followed by an optional sign and at least one decimal digit, and an optional suffix indicating the floating-point type. An exponent marker with no following digits, or any other deviation from this structure, **MUST** be diagnosed as `E-SRC-0304`.
>
> Integer and floating-point literals **MAY** carry type suffixes drawn from the sets defined in the type system (Part IV and Appendix A). If a numeric literal is followed by a suffix that is not one of the suffixes permitted for that literal kind, the implementation **MUST** diagnose the literal as malformed with `E-SRC-0304`.
>
> A decimal integer literal whose first digit is `0` and whose textual representation contains at least one additional decimal digit **MUST** be accepted as a valid decimal literal. Implementations **SHOULD** emit warning `W-SRC-0301` for such literals, but **MUST NOT** treat them as octal or reject them solely for having leading zeros.

```ebnf
<decimal_integer>
    ::= <dec_digit> ("_"* <dec_digit>)*

<hex_integer>
    ::= "0x" <hex_digit> ("_"* <hex_digit>)*

<octal_integer>
    ::= "0o" <oct_digit> ("_"* <oct_digit>)*

<binary_integer>
    ::= "0b" <bin_digit> ("_"* <bin_digit>)*
```

> The helper non-terminals `<dec_digit>`, `<hex_digit>`, `<oct_digit>`, and `<bin_digit>` above correspond to the digit classes used in Appendix A. Implementations **MUST** ensure that these productions and the numeric literal productions in Appendix A describe the same set of token lexemes.

#### 9.4.2 String Literals [lexical.literals.string]

> A string literal **MUST** begin with a double-quote character (`"` = U+0022) and **MUST** end with a matching unescaped double-quote. The characters between the opening and closing quotes **MAY** include any Unicode scalar values other than an unescaped double-quote and control characters forbidden by §8.1.3.
>
> Inside a string literal, the backslash character (`\` = U+005C) **MUST** introduce an escape sequence. The only valid escape sequences in string literals **MUST** be:
>
> - `\\n`, `\\r`, `\\t`, `\\\\`, `\\\"`, `\\'`, `\\0`
> - `\\xHH` where `H` is a hexadecimal digit (two hex digits total)
> - `\\u{H+}` where each `H` is a hexadecimal digit and the resulting code point is a Unicode scalar value
>
> Any other sequence beginning with `\\` inside a string literal **MUST** be diagnosed as `E-SRC-0302` (Invalid escape sequence), and the implementation **MUST NOT** attempt to reinterpret it as multiple smaller tokens.
>
> If a line feed (U+000A) or end-of-file is encountered before a closing double-quote for a string literal, the implementation **MUST** emit diagnostic `E-SRC-0301` (Unterminated string literal) at the location of the opening quote and **MUST** treat the remainder of the source line (or file) as part of the ill-formed literal for diagnostic purposes.
>
> Implementations **MUST** accept string literals whose length (in Unicode scalar values) is at least 65,535 characters (§6.5.1). Each implementation **MUST** document any stricter limit it imposes. When a string literal in source exceeds the implementation’s documented limit, the implementation **MAY** reject the program with an error diagnostic in the `SRC-03` bucket and **MUST**, when it does so, identify the offending literal; when a string literal produced or manipulated during compile-time evaluation exceeds the guaranteed minimum, the implementation **MUST** emit `E-SRC-0204` (§8.1, Appendix B).

#### 9.4.3 Character Literals [lexical.literals.character]

> A character literal **MUST** begin with a single-quote character (`'` = U+0027), **MUST** end with a matching unescaped single-quote, and **MUST** represent exactly one Unicode scalar value.
>
> The content of a character literal **MUST** be either (a) a single Unicode scalar value that is not an unescaped single-quote, backslash, or forbidden control character (§8.1.3), or (b) one of the escape sequences permitted for string literals (`\\n`, `\\r`, `\\t`, `\\\\`, `\\\"`, `\\'`, `\\0`, `\\xHH`, `\\u{H+}`). Any other escape sequence inside a character literal **MUST** be diagnosed as `E-SRC-0302`.
>
> A character literal that is empty (`''`), contains more than one Unicode scalar value after escape interpretation, or reaches end-of-file or a newline before the closing single-quote **MUST** be diagnosed as `E-SRC-0303` (Invalid character literal) and **MUST** cause the compilation unit to be treated as ill-formed.

#### 9.4.4 Boolean Literals [lexical.literals.boolean]

> The lexemes `true` and `false` **MUST** be tokenized as `<keyword>` tokens as specified in §9.2.1 and **MUST NOT** be treated as ordinary identifiers. Their spelling is case-sensitive: only the all-lowercase forms `true` and `false` **MUST** be accepted as boolean literals where the grammar admits a boolean literal.
>
> Using `true` or `false` where an identifier is expected **MUST** be rejected with diagnostic `E-SRC-0305` (Reserved keyword used as identifier). Implementations **MUST** ensure that the boolean-literal nonterminal in Appendix A and the keyword list in §9.2.1 remain consistent.

---

## 10. Syntactic Structure [syntax]

This chapter provides high-level grammar organization. Detailed syntax appears in respective parts; the normative grammar appears in Appendix A.

### 10.1 Grammar Organization [syntax.organization]

> Appendix A presents the canonical grammar of the Cursive language, and conforming implementations **MUST** treat that grammar as canonical. When implementations derive parsers or auxiliary grammars from other presentations, those grammars **MUST NOT** accept token sequences that are not derivable from Appendix A and **MUST NOT** assign precedence or associativity that contradicts Appendix A.
>
> A source program is syntactically well-formed if and only if the token stream produced by §9 can be parsed according to the canonical grammar in Appendix A and all syntactic side conditions in Parts III–V are satisfied. Conforming implementations **MUST** accept exactly the syntactically well-formed programs, except where this specification explicitly classifies violations as IFNDR (§6.1.3.1, §6.3.3).

#### 10.1.1 Declarations [syntax.organization.declarations]

> Grammar productions for declarations in Part III **MUST** be consistent with and refine the declaration-related productions in Appendix A (including, but not limited to, `decl`, `type_decl`, `record_decl`, `enum_decl`, `modal_decl`, `procedure_decl`, and `grant_decl`). Any construct that the implementation parses as a declaration **MUST** correspond to one of these canonical forms, possibly after desugaring defined in Part III.
>
> At module scope, only declaration forms defined by Part III **MAY** appear. If a construct that is classified as a statement (§10.1.2) appears at module scope, the implementation **MUST** reject it and **MUST** emit diagnostic `E-SRC-0501` (Statement not permitted at module scope).

#### 10.1.2 Statements [syntax.organization.statements]

> Grammar productions for statements in Part V **MUST** be consistent with and refine the statement-related productions in Appendix A (including `statement`, `block_stmt`, `var_decl_stmt`, `assign_stmt`, `expr_stmt`, and control-flow statements). Any construct parsed as a statement **MUST** occur only in contexts permitted by Part V and **MUST NOT** appear where the canonical grammar expects a declaration, expression, or pattern.
>
> Statement sequences **MUST** respect the separator and termination rules defined by the lexical grammar (§9.1.2–§9.1.3) and §10.2.4. When the parser determines that a statement is incomplete at end-of-file solely because required tokens are missing (such as an unclosed block or parenthesized expression), it **MUST** emit diagnostic `E-SRC-0401` (Unexpected EOF during unterminated statement or continuation).

#### 10.1.3 Expressions [syntax.organization.expressions]

> Expression productions in Part V **MUST** refine the expression hierarchy given in Appendix A (including `expr`, `assignment_expr`, `pipeline_expr`, and the various operator-precedence levels) without changing operator precedence or associativity. Implementations **MUST NOT** extend the expression grammar in ways that cause previously well-formed programs to be parsed with different precedence, except in a new MAJOR language version (§7.1).
>
> When expression parsing fails due to malformed operator sequences or unmatched delimiters, implementations **MUST** emit diagnostics in the appropriate `EXP-01` or `SRC-04` buckets as specified in Appendix B.

#### 10.1.4 Patterns [syntax.organization.patterns]

> Pattern productions in Part V **MUST** refine the pattern grammar in Appendix A (including `pattern`, `tuple_pattern`, `record_pattern`, `enum_pattern`, and related productions). Any construct that the implementation parses as a pattern **MUST** satisfy the structural constraints of Appendix A and Part V, including any arity and field-name constraints specified there.
>
> Patterns **MUST** appear only in syntactic positions explicitly designated as pattern positions by Appendix A and Part V, including `match` arms, destructuring bindings, and loop heads as specified there.

### 10.2 Syntactic Nesting Limits [syntax.limits]

#### 10.2.1 Reference to Implementation Limits [syntax.limits.reference]

> Implementations **MUST** support syntactic nesting depths at least as large as the minima specified in §6.5.1 for blocks, expressions, types, delimiter nesting, and scope depth. Each implementation **MUST** document any stricter limits it imposes in its conformance dossier (Appendix C) and **MUST** state which diagnostics are used when such limits are exceeded.
>
> When syntactic nesting limits are exceeded, implementations **MAY** classify the resulting programs as IFNDR (§6.1.3.1) or **MAY** reject them as ill-formed. If a program is rejected solely because a syntactic nesting limit was exceeded, the implementation **MUST** emit at least one error diagnostic in the appropriate `SRC-04` or `SRC-14` feature bucket as described in Appendix B.

#### 10.2.2 Block Nesting [syntax.limits.blocks]

> A block is any syntactic construct delimited by `{` and `}` that introduces a new statement scope, including procedure bodies, explicit blocks, and control-flow bodies. The depth of block nesting for a given point in a compilation unit **MUST** be defined as the number of enclosing blocks at that point.
>
> Implementations **MUST** support block nesting depth of at least 256 levels (§6.5.1). When the actual block nesting depth supported by an implementation is exceeded in otherwise well-formed source, the implementation **MUST** either treat the program as IFNDR or **MUST** reject it and emit diagnostic `E-SRC-1401` (Scope nesting exceeds implementation limit), attributing the diagnostic to a representative location within the deepest block.

#### 10.2.3 Expression Nesting [syntax.limits.expressions]

> Expression nesting depth at a program point **MUST** be defined as the maximum number of expressions that are syntactically nested within one another according to the expression grammar, including nested function calls, arithmetic expressions, and conditional expressions.
>
> Implementations **MUST** support expression nesting depth of at least 256 levels (§6.5.1). When an implementation rejects a program solely because expression nesting depth exceeds its supported limit, it **MUST** emit an error diagnostic whose code is drawn from the `SRC-04` feature bucket and is documented in Appendix D, and **MUST** identify an expression whose nesting contributes to the violation.

#### 10.2.4 Delimiter Nesting [syntax.limits.delimiters]

> Delimiter nesting depth **MUST** be tracked independently of block and expression nesting. The delimiter depth at a program point **MUST** be defined as the length of the stack of unmatched opening delimiters chosen from the set `{`, `}`, `(`, `)`, `[`, `]`.
>
> Implementations **MUST** support delimiter nesting depth of at least 256 levels (§6.5.1). When delimiter nesting exceeds the implementation’s supported limit while parsing otherwise well-formed constructs, the implementation **MUST** emit diagnostic `E-SRC-0402` (Delimiter nesting depth exceeded supported bound) and **MUST** treat the affected compilation unit as ill-formed.
>
> The delimiter stack used for this purpose **MUST** ignore delimiters that appear inside comments (§9.1.3) and **MUST** treat delimiters that appear inside string or character literals (§9.4) as part of the literal rather than as syntactic delimiters.

### 10.3 Grammar Notation [syntax.notation]

Grammar notation follows the meta-notation conventions of Part 0 §3 (including §3.1.1). Where this Part and the ANTLR-style grammar in Appendix A differ, Appendix A **MUST** be treated as canonical for determining whether a token sequence is syntactically well-formed.

---

## 11. Translation Pipeline [translation]

This chapter defines the conceptual process of translating Cursive source text into executable artifacts.

### 11.1 Translation Model [translation.model]

> Translation of a compilation unit **MUST** proceed through a series of logical stages that respect the dependencies outlined in this chapter. While implementations may interleave, parallelize, or iterate these stages, the observable outcome for any given compilation unit **MUST** be equivalent to a process where:
>
> 1.  Source text is ingested and normalized (§11.2).
> 2.  The normalized text is tokenized through lexical analysis (§11.3).
> 3.  The token stream is parsed into an abstract syntax tree (AST) (§11.4).
> 4.  The AST is subjected to semantic analysis, including name resolution and type checking (§11.5).
> 5.  Compile-time evaluation is performed (§11.6).
> 6.  Target code is generated from the validated program representation (§11.7).
>
> If any stage of translation for a compilation unit emits at least one error diagnostic, the unit **MUST** be treated as ill-formed. Later stages **MAY** still operate on that unit solely to report additional diagnostics or to produce non-executable tooling outputs, in accordance with §6.3.3 and §6.2.1.
>
> For a fixed compilation configuration, the combined effect of all translation stages on a given compilation unit (the accept/reject decision, the set of diagnostics, and any generated artifacts) **MUST** be deterministic, except where this specification explicitly permits variation through IDB or USB.

### 11.2 Source Ingestion [translation.ingestion]

> The first stage of translation **MUST** implement the preprocessing pipeline of §8.1.0 for each compilation unit, including file size validation, UTF-8 decoding, BOM handling, and line-ending normalization.
>
> A compilation unit that fails this stage **MUST** be classified as ill-formed and **MUST NOT** proceed to lexical analysis, except for the purpose of producing further diagnostics as permitted by §6.2.1.

### 11.3 Lexical Analysis [translation.lexical]

> Following source ingestion, the lexical analysis stage **MUST** apply the rules of §9 to the normalized source text, producing a token sequence. Each token **MUST** carry source-span metadata sufficient to satisfy the diagnostic requirements of §6.2.2.
>
> Any violation of the lexical rules in §9 that is not classified as IFNDR **MUST** be diagnosed during this stage.

### 11.4 Syntactic Analysis [translation.parsing]

> The syntactic analysis stage **MUST** consume the token sequence from lexical analysis and parse it according to the canonical grammar (Appendix A), producing an abstract syntax tree (AST).
>
> The parser **MUST** identify all declarations and their syntactic containers to enable subsequent name resolution. Violations of the grammar, such as statements appearing at module scope (`E-SRC-0501`) or unterminated constructs at end-of-file (`E-SRC-0401`), **MUST** be diagnosed in this stage.

### 11.5 Semantic Analysis [translation.semantic]

> The semantic analysis stage **MUST** operate on the AST to perform module discovery, import resolution, name resolution, type checking, permission checking, and contract verification as specified in Parts III–VIII.
>
> All static-semantic violations related to types (`TYP-*`), memory (`MEM-*`), expressions (`EXP-*`), grants (`GRN-*`), and name resolution (`SRC-11` through `SRC-16`) **MUST** be diagnosed during this stage. If semantic analysis determines a compilation unit is ill-formed, the unit **MUST** be rejected.

### 11.6 Compile-Time Evaluation [translation.comptime]

> Following successful semantic analysis, the compile-time evaluation stage **MUST** execute `comptime` constructs in a well-defined dependency order as specified in Part X.
>
> This stage **MUST** enforce the compile-time resource limits defined in §6.5.1. Violations, such as exceeding recursion depth (`E-SRC-0201`) or depending on runtime-only information (`E-EXP-0402`), **MUST** be diagnosed here.

### 11.7 Code Generation [translation.codegen]

> The final stage of translation, code generation, **MUST** translate the fully validated program representation into target-specific artifacts (e.g., object files, libraries).
>
> This stage **MAY** perform any optimizations that adhere to the as-if rule (§6.3.5) and the ABI requirements of Part XII. Code generation **MUST NOT** proceed for any compilation unit that has been rejected in a prior stage.

---

## 12. Diagnostic System Mechanics [diagnostics]

This chapter specifies the mechanical aspects of how implementations report violations, complementing the normative requirements in §6.2.

### 12.1 Diagnostic Presentation [diagnostics.idb.presentation]

> The presentation format of diagnostics is implementation-defined. Implementations **MAY** choose:
>
> 1. Text formatting (color, Unicode characters, ASCII fallback)
> 2. Source context window size (lines before and after error location)
> 3. Message wording (provided the description remains accurate and clear)
> 4. Additional information beyond minimum requirements (§6.2.2)
>
> Implementations that support machine-readable diagnostic output **MUST** document the output format in the conformance dossier (Appendix C).

### 12.2 Additional Diagnostics [diagnostics.idb.additional]

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

### 12.3 Diagnostic Catalog Reference [diagnostics.catalog]

> The normative catalog of specification-defined diagnostic codes appears in Appendix B and Appendix D. The catalog is normative: implementations **MUST** use the specified codes for the specified violations. The feature bucket taxonomy organizing diagnostics within categories appears in Appendix B.
>
> The conformance dossier schema in Appendix C defines where implementation-defined aspects of diagnostic reporting **MUST** be documented.

This section addresses cases where diagnostic requirements are relaxed due to computational infeasibility.

#### 12.4.1 Ill-Formed, No Diagnostic Required [diagnostics.ifndr.definition]

> When detecting a violation requires solving computationally infeasible problems as defined in §6.1.3.1, implementations are not required to issue diagnostics.
>
> Programs containing undetected IFNDR violations remain ill-formed and non-conforming (§6.2.2).
>
> The behavior of programs with IFNDR violations is unspecified but **MUST NOT** include UVB (§6.1.1).
>
> Diagnostics for IFNDR violations are governed by §6.1.3.1 and §6.2.3.

#### 12.4.2 IFNDR Diagnostic Encouragement [diagnostics.ifndr.encouragement]

> Implementations **SHOULD** diagnose IFNDR violations when detection is feasible within reasonable resource limits.
>
> Implementations **MAY** issue warnings for patterns that commonly indicate IFNDR violations even when definitive detection is infeasible.
>
> Implementations **MAY** provide configuration options to control the effort expended on IFNDR detection (analysis depth, timeout limits, complexity thresholds).

### 12.5 Implementation-Defined Behavior [diagnostics.idb]

This section specifies aspects of diagnostic reporting where implementations have discretion within conformance constraints.

#### 12.5.1 Diagnostic Presentation [diagnostics.idb.presentation]

> The presentation format of diagnostics is implementation-defined. Implementations **MAY** choose:
>
> 1. Text formatting (color, Unicode characters, ASCII fallback)
> 2. Source context window size (lines before and after error location)
> 3. Message wording (provided the description remains accurate and clear)
> 4. Additional information beyond minimum requirements (§12.1.2)
>
> Implementations that support machine-readable diagnostic output **MUST** document the output format in the conformance dossier (Appendix C; see §6.2.4).

**_Explanation:_**
Presentation format flexibility allows implementations to adapt to different terminal capabilities, user preferences, and tooling requirements. However, the core information (code, severity, location, description) remains mandatory. Machine-readable format documentation enables tooling integration.

#### 12.5.2 Additional Diagnostics [diagnostics.idb.additional]

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


### 12.6 Diagnostic Catalog Reference [diagnostics.catalog]

This section provides references to the comprehensive diagnostic catalog and related appendices.

#### 12.6.1 Complete Diagnostic Catalog [diagnostics.catalog.reference]

> The normative catalog of specification-defined diagnostic codes appears in Appendix B, §B.9, and is cross-indexed in Appendix D. The catalog includes:
>
> 1. Diagnostic code and severity
> 2. Specification section reference
> 3. Brief description of the violation
> 4. Example triggering code (for selected diagnostics)
>
> The catalog is normative: implementations **MUST** use the specified codes for the specified violations.

**_Explanation:_**
Appendix B and Appendix D together serve as the authoritative reference for all diagnostic codes, enabling implementers to ensure coverage and users to look up diagnostic meanings. The normative status ensures consistency across implementations.

#### 12.6.2 Diagnostic Taxonomy [diagnostics.catalog.taxonomy]

> The feature bucket taxonomy organizing diagnostics within categories appears in Appendix B. The taxonomy specifies:
>
> 1. Bucket number ranges for each category
> 2. Functional area corresponding to each bucket
> 3. Allocation strategy for future diagnostics
>
> The taxonomy is normative: new diagnostics **MUST** be assigned to appropriate buckets according to the allocation strategy.
>
> Violations of feature enablement, conformance-mode selection, and versioning requirements in Part I **MUST** be reported using `CNF`-category diagnostics and the feature-bucket allocations specified for `CNF-01`–`CNF-03` in Appendix B.

**_Explanation:_**
Appendix B provides the organizational structure that keeps diagnostic codes stable and predictable as the language evolves. Following the allocation strategy prevents conflicts and maintains the hierarchical organization.

#### 12.6.3 Implementation-Defined Behavior Index [diagnostics.catalog.idb]

> Implementation-defined aspects of diagnostic reporting and implementation-defined behavior choices are encoded in the conformance dossier schema defined in Appendix C. The conformance dossier’s `implementation_defined_behaviors` section (Appendix C, §C.2.3) **MUST** serve as the normative IDB Index for the implementation. The dossier’s `diagnostics` section (Appendix C, §C.2.6) **MUST** describe diagnostic presentation formats, machine-readable output formats, and any additional diagnostic policies that the implementation treats as implementation-defined.
>
> Implementations **MUST** document their choices for items listed in Appendix C as part of their conformance dossier (§6.2.4).

**_Explanation:_**
Appendix C identifies which aspects of diagnostic reporting are implementation-defined, guiding implementers on what must be documented and helping users understand variation across implementations.

---

# Part III - Module System & Organization

This part defines how Cursive programs are organized into modules and assemblies, how names are imported and resolved, how modules are initialized, and how development tools integrate with the language.

*(Informative)*

| Theme                                                                                      | Key Diagnostics    |
| ------------------------------------------------------------------------------------------ | ------------------ |
| Each module has a unique module path within an assembly.                                   | `E-SRC-1103, 1104` |
| All imports refer to existing, reachable modules.                                          | `E-SRC-1202`       |
| Import aliases are unique per module.                                                      | `E-SRC-1201`       |
| The eager module-dependency subgraph is acyclic.                                           | `E-SRC-1301`       |
| No module reads an eager binding before it is initialized or after initialization failure. | `E-SRC-1302, 1303` |

## 13. Modules and Assemblies [modules]

This chapter defines Cursive's core organizational units: modules and assemblies. It specifies how they are discovered from the file system and configured via a manifest file.

### 13.1 Module and Assembly Definitions [modules.definitions]

> A **module** is the fundamental unit of code organization and encapsulation in Cursive. It consists of a collection of declarations (types, procedures, constants, etc.) contained within one or more `.cursive` source files.
>
> An **assembly** is a named collection of modules that are compiled and distributed as a single unit. An assembly can represent a library or an executable.
>
> A **project** consists of one or more assemblies that are developed and versioned together. Every project is defined by a `Cursive.toml` manifest file at its root.

### 13.2 Module Discovery and Paths [modules.discovery]

#### 13.2.1 Folder-Scoped Modules [modules.discovery.rules]

> Any folder containing at least one `.cursive` source file **MUST** be treated as a single module. All `.cursive` files located directly within that folder contribute their declarations to that module.
>
> Subfolders of a module folder are treated as nested child modules. The path to a nested module is formed by `::`-separating the folder names from the assembly's root.

#### 13.2.2 Module Path [modules.paths]

> A module path is a `::`-separated sequence of identifiers that uniquely locates a module within an assembly. The path components **MUST** be valid identifiers (§9.3) and **MUST NOT** be reserved keywords (§9.2.1).
>
> On platforms with case-insensitive filesystems, implementations **MUST** normalize module path components to a consistent case (e.g., lowercase) for comparison to prevent ambiguity. Collisions that arise after normalization **MUST** be diagnosed with `E-SRC-1104`. Implementations **SHOULD** issue a warning (`W-SRC-1105`) for potential case-insensitivity collisions even on case-sensitive platforms.

### 13.3 Project Manifest (`Cursive.toml`) [modules.manifest]

Every project **MUST** include a UTF-8 encoded manifest file named `Cursive.toml` at its root. The manifest **MUST** follow TOML 1.0 syntax.

#### 13.3.1 Manifest Schema and Requirements [modules.manifest.schema]

The manifest **MUST** conform to the following semantic requirements:

1.  **`language.version`**: A string that **MUST** be a valid semantic version. The `MAJOR` version **MUST** match the compiler's supported major version. Missing or incompatible versions **MUST** be diagnosed (`E-SRC-1107`).
2.  **`[roots]`**: A table mapping a name to a source directory path. It **MUST** contain at least one entry. Each path **MUST** be a normalized relative path from the project root. Invalid or duplicate roots **MUST** be diagnosed (`E-SRC-1101`, `E-SRC-1102`).
3.  **`[[assemblies]]`**: An array of tables, each defining an assembly. Each assembly **MUST** have a unique `name` and **MUST** reference only declared roots. Assembly dependency graphs **MUST NOT** contain cycles (`E-SRC-1108`).
4.  **`[features]`**: An optional table for enabling language features. The `enable` key **MUST** be an array of feature-flag strings. Unknown feature flags **MUST** trigger an error (`E-CNF-0101`) in conforming modes (§7.3.2).
5.  **`[build]`**: An optional table for build configuration. The `conformance` key, if present, **MUST** select a valid conformance mode (§6.3.7).

> Malformed manifest structures **MUST** be diagnosed with `E-SRC-1106`. Unknown top-level keys **SHOULD** be diagnosed with `W-SRC-1106`.

---

## 14. Imports and Name Resolution [names]

This chapter defines how names are imported across modules, how scopes are formed, and how name lookup is performed.

### 14.1 Import System [names.imports]

#### 14.1.1 Import Declarations [names.imports.forms]

> An `import` declaration makes declarations from another module available for qualified access. The basic form is:
>
> ```cursive
> import module_path;
> import module_path as alias;
> ```
>
> The `module_path` **MUST** resolve to a module visible within the current assembly or its dependencies. An unresolved module path **MUST** be diagnosed with `E-SRC-1202`.
>
> An `alias` **MUST** be a unique identifier within the importing module's scope. Attempting to redefine an import alias **MUST** be diagnosed with `E-SRC-1201`.

#### 14.1.2 Qualified Access [names.imports.qualified]

> By default, an `import` declaration enables **qualified access** only. Unqualified access to imported symbols is not permitted. A reference to an item from an imported module **MUST** be qualified by the module's path or its alias (e.g., `my_module::my_function()` or `m::my_function()`).

#### 14.1.3 Re-Exporting [names.imports.reexport]

> To re-export a symbol from another module, a `public import` declaration **MUST** be used.
>
> ```cursive
> public import module_path::Item;
> public import module_path::{Item1, Item2};
> ```
>
> This makes the specified items part of the current module's public API. The re-exported items **MUST** be visible in the source module. An attempt to re-export a private item or a non-existent item **MUST** be diagnosed (`E-SRC-1604`, `E-SRC-1603`).

### 14.2 Scopes [names.scopes]

> A **scope** is a syntactic region where an identifier is bound to a specific entity. Cursive has the following scope kinds:
>
> 1.  **Module Scope**: Encompasses all source files within a module. Top-level declarations and imports belong to this scope.
> 2.  **Procedure Scope**: Contains the parameters and labels of a procedure.
> 3.  **Block Scope**: A lexical scope created by a brace-enclosed block (`{ ... }`).
>
> Scopes nest to form a tree. Name lookup proceeds from the innermost scope outwards.

### 14.3 Bindings [names.bindings]

> A **binding** is an association between an identifier and an entity (e.g., a variable, type, or module).
>
> A declaration introduces a new binding into the current scope. If an identifier is already bound in the current scope, redeclaring it is an error, unless an explicit shadowing mechanism is used. Such violations **MUST** be diagnosed with an error from the `SRC-15` feature bucket.
>
> User declarations **MUST NOT** shadow universe-protected bindings (§6.6.4), which **MUST** be diagnosed with `E-SRC-1502`.

### 14.4 Name Lookup [names.lookup]

#### 14.4.1 Unqualified Lookup [names.lookup.unqualified]

> An unqualified identifier is resolved by searching for a binding in the scope chain, starting from the current scope and moving outwards to parent scopes. If no binding is found after searching all enclosing scopes (up to the module scope), the lookup fails.
>
> Unqualified lookup **MUST NOT** cross module boundaries. An unresolved unqualified identifier **MUST** be diagnosed with `E-SRC-1601`.

#### 14.4.2 Qualified Lookup [names.lookup.qualified]

> A qualified identifier of the form `Head::Name` **MUST** be resolved as follows:
>
> 1.  Resolve `Head` to a module or module alias visible in the current scope. If `Head` is not a visible module, it is an error (`E-SRC-1605`).
> 2.  Look up `Name` within the public export set of the resolved module. If `Name` is not a public member of that module, it is an error (`E-SRC-1603` or `E-SRC-1604`).

#### 14.4.3 Ambiguity [names.lookup.ambiguity]

> If a name lookup could resolve to multiple distinct entities and no language rule disambiguates the reference, the lookup is ambiguous. Ambiguous lookups **MUST** be diagnosed with `E-SRC-1602`. Implementations **MUST NOT** use declaration order or other non-normative heuristics to resolve ambiguity.

---

## 15. Initialization and Dependencies [initialization]

This chapter defines module initialization ordering and dependency management.

### 15.1 Dependency Graph [initialization.graph]

#### 15.1.1 Graph Construction [initialization.graph.construction]

Implementations **MUST** build a module dependency graph with vertices as modules and edges arising from imports and module-scope initializers/comptime blocks referencing other modules' exports.

#### 15.1.2 Eager vs. Lazy Edges [initialization.graph.edges]

Edges **MUST** be marked eager when initialization reads bindings from the target; otherwise they are lazy.

#### 15.1.3 Cycle Detection [initialization.graph.cycles]

The eager subgraph **MUST** be acyclic (E-SRC-1301).

### 15.2 Eager Initialization [initialization.eager]

#### 15.2.1 Eager Dependency Definition [initialization.eager.definition]

Eager dependencies arise when a module's initialization code directly reads bindings from another module.

#### 15.2.2 Topological Ordering [initialization.eager.ordering]

Initialization **MUST** run in topological order for eager dependencies.

#### 15.2.3 Initialization Timing [initialization.eager.timing]

Consuming an uninitialized eager dependency **MUST** raise E-SRC-1302.

### 15.3 Lazy Initialization [initialization.lazy]

#### 15.3.1 Lazy Semantics [initialization.lazy.semantics]

Lazy edges **MAY** form cycles, and implementations **MAY** evaluate initializers lazily provided they execute at most once, preserve observable behavior (§6.2.5), and maintain ordering.

#### 15.3.2 Cycle Handling [initialization.lazy.cycles]

Lazy edges **MAY** form cycles without raising diagnostics.

#### 15.3.3 Execution Guarantees [initialization.lazy.guarantees]

Lazy initializers **MUST** execute at most once.

### 15.4 Initialization Failure [initialization.failure]

#### 15.4.1 Failure Propagation [initialization.failure.propagation]

If a module's initialization fails, every module reachable via eager edges **MUST** be reported as blocked (E-SRC-1303).

#### 15.4.2 Blocked Modules [initialization.failure.blocked]

Blocked modules **MUST NOT** execute their initialization code.

#### 15.4.3 Retry Semantics [initialization.failure.retry]

Safe retries **MAY** occur only if failed effects can be rolled back.

---

## 16. Tooling Integration [tooling]

Requirements and guidance for compiler APIs, language servers, metadata, and IDE integration are collected in Appendix F – Tooling Integration.

---

# Part IV - Type System

This part defines the Cursive type system, including type foundations, primitive and composite types, function types, pointers, and modal types. Type declarations are defined alongside the semantics of each type category.

## 17. Type Foundations & Type Declarations [types.foundations]

> **17.1.1 Static Typing Requirement**  
> For every expression, pattern, and declaration in a well-formed program, the implementation **MUST** assign a unique static type before code generation.  
> Implementations **MUST NOT** execute or emit code for any construct whose static type cannot be determined or that violates the type-formation rules of this Part; instead, they **MUST** emit at least one `E-TYP-01xx` diagnostic.

> **17.1.2 Nominal Type Declarations**  
> Named types introduced by type declarations (records, enums, modal types, and other nominal types) **MUST** be treated as distinct nominal types, even if their structure is identical.  
> A type name **MUST NOT** be redeclared in the same module scope; violations **MUST** produce a type-formation error in the `TYP-01` category.

> **17.1.3 Type Well-Formedness**  
> A type expression is well-formed only if all referenced type names are declared, all generic parameters are in scope, and all constituent types are themselves well-formed.  
> If any of these conditions fails, the implementation **MUST** emit an error in the `E-TYP-0101` or `E-TYP-0104` family and **MUST NOT** treat the program as well-typed.

> **17.1.4 Typing Judgments and Principal Types**  
> The static semantics of Cursive is defined in terms of judgments of the form `Γ ⊢ e : T`, where `Γ` is a typing context (mapping bindings to types) and `T` is a type expression. For every expression `e` that is well-typed in some context `Γ`, there **MUST** exist a unique principal type `T₀` such that:
>
> 1. `Γ ⊢ e : T₀`; and  
> 2. for any type `T` with `Γ ⊢ e : T`, the relation `T₀ ⪯ T` holds in the type-compatibility relation of §22.1.2.
>
> Type inference **MAY** be used to compute `T₀` in positions where the grammar permits omission of explicit type annotations (for example, local `let`/`var` bindings and non-exported temporaries). In the signatures of public declarations, exported types, and FFI surfaces, missing type annotations **MUST** be diagnosed as errors in the `E-TYP-0104` family rather than inferred in an implementation-defined way. Implementations **MUST** ensure that the inference algorithm they implement is sound and complete with respect to the typing rules of this Part in all contexts where inference is permitted.

## 18. Primitive Types [types.primitive]

> **18.1.1 Primitive Type Set**  
> The implementation **MUST** support a fixed set of primitive scalar types sufficient to represent the integer and floating-point formats required by the target platform and the specification’s numeric model.  
> Each primitive type **MUST** have a well-defined size, alignment, and representation documented in the conformance dossier (§6.1.4.1 and Appendix C).

> **18.1.2 Primitive Type Operations**  
> For each primitive type, the implementation **MUST** define the set of permitted operations (arithmetic, comparison, bitwise, conversion) and their static types.  
> Applying an operation to primitive operands outside its domain **MUST** be rejected with a type error diagnostic in the `E-TYP-01xx` or `E-EXP-01xx` families.

## 19. Composite Types [types.composite]

> **19.1.1 Record Types**  
> A record type declaration **MUST** specify a finite set of named fields; field names **MUST** be unique within the record. Duplicate field names **MUST** be rejected with `E-TYP-0201`.  
> Each field **MUST** have a well-formed type; if a field type is not sized where required, the implementation **MUST** emit `E-TYP-0209`.

> **19.1.2 Enum and Sum Types**  
> An enum type declaration **MUST** declare at least one variant. Enums with zero variants **MUST** be rejected with `E-TYP-0208`.  
> Variant names **MUST** be unique within the enum; duplicate variant names **MUST** be rejected with `E-TYP-0204`.

> **19.1.3 Recursive Types and Size**  
> Composite types **MUST NOT** rely on unbounded recursion without an indirection mechanism defined elsewhere in this specification; types whose size would be infinite **MUST** be rejected with `E-TYP-0105`.

## 20. Function Types & Procedure Declarations [types.functions]

> **20.1.1 Function Type Formation**  
> A function type **MUST** specify an ordered list of parameter types and a result type. All parameter and result types **MUST** be well-formed.  
> The implementation **MUST** enforce that calls to a function supply arguments whose types match the parameter types, modulo permitted conversions; violations **MUST** produce `E-EXP-0104` or `E-EXP-0105`.

> **20.1.2 Procedure Declarations and Bodies**  
> Every procedure declaration **MUST** have a type consistent with its body: all control-flow paths that terminate the procedure **MUST** either return a value of the declared result type or diverge according to the rules in Part V.  
> If the implementation cannot prove that all paths satisfy this condition, it **MUST** emit a diagnostic in the `E-EXP-0201` family and **MUST NOT** treat the procedure as well-typed.

## 21. Pointer & Reference Types [types.pointers]

> **21.1.1 Pointer Type Formation**  
> Pointer and reference types **MUST** be formed only over well-formed referent types. The specification of pointer provenance, alignment, and safety obligations for dereference is given in Part VII.  
> Using a pointer type in a context that requires a sized type **MUST** be rejected if the referent type does not satisfy the required sizing behavior.

> **21.1.2 Safe Dereference in Safe Code**  
> In code that does not use `unsafe` constructs, dereference operations **MUST** be statically or dynamically checked according to the memory and permission model such that memory safety is preserved.  
> If the implementation cannot guarantee that a dereference is safe under these rules, it **MUST** reject the program with a diagnostic in the `MEM-04` or `TYP-01` categories rather than permitting unverifiable behavior.

## 22. Modal Types & Type Relations [types.modal]

> **22.1.1 Modal State Machines**  
> A modal type declaration **MUST** define a finite set of named states and a set of permitted transitions between those states. Exactly one state **MUST** be designated as the initial state; modal declarations lacking an initial state or declaring multiple initial states **MUST** be rejected with `E-TYP-0205` or `E-TYP-0206`.  
> The implementation **MUST** enforce that all operations on modal values are valid only in states where those operations are declared.

> **22.1.2 Type Relations and Compatibility**  
> The type system defines a type-compatibility relation that determines when a value of one type may be used where another type is expected. Implementations **MUST** apply this relation consistently when checking assignments, parameter passing, and return statements.  
> When no rule of the type-compatibility relation permits a conversion, the implementation **MUST** emit a type error diagnostic in the `E-TYP-01xx` or `E-EXP-01xx` families and **MUST NOT** attempt implicit conversions beyond those sanctioned by this Part.

> **22.1.3 Modal Typing and State Transitions**  
> Modal values are typed with judgments of the form `Γ ⊢ e : M @S`, where `M` is a modal type and `S` is one of its declared states. For any operation `op` on a modal value, the modal type declaration **MUST** specify:
>
> 1. The set of states in which `op` is permitted to be invoked; and  
> 2. The state (or set of states) in which the receiver will reside after successful completion of `op`.
>
> Typing and effect rules **MUST** enforce that:
>
> - If `Γ ⊢ e : M @S` and `op` is invoked on `e`, then `S` **MUST** be one of the states in which `op` is declared; otherwise the implementation **MUST** reject the program with `E-TYP-0207` or a related modal-type diagnostic.  
> - The resulting state annotation in the type of `op(e)` **MUST** correspond to one of the declared target states for `op`.  
>
> Informally, the modal state machine declared for `M` induces a labeled transition system, and well-typed programs **MUST NOT** contain executions whose modal state transitions fall outside this system. Implementations **MAY** enforce these constraints purely statically, purely dynamically, or with a hybrid of the two, but in all cases violations in safe code **MUST** be diagnosed rather than classified as UVB.

---

# Part V - Expressions & Statements

This part defines the expression language, statement forms, variable bindings, and control flow constructs including pattern matching.

## 23. Expressions & Variable Bindings [expressions]

> **23.1.1 Expression Categories**
> Every expression (§4.3 [terminology.programming]) **MUST** be classified as producing either a value (§4.3 [terminology.programming]), a place (§4.3 [terminology.programming]) that can be used as the target of assignment, or a divergent computation that does not produce a value.  
> The implementation **MUST** assign a static type to each expression category consistent with the type rules in Part IV. Expressions that cannot be assigned a type **MUST** be rejected with `E-EXP-0101` or `E-TYP-0104`.

> **23.1.2 Evaluation Order**  
> The evaluation order of subexpressions within an expression **MUST** be fixed and deterministic. Implementations **MUST NOT** reorder subexpression evaluation in a way that changes observable behavior as defined in §6.2.5.  
> When an operator or construct has a defined left-to-right or right-to-left evaluation order, implementations **MUST** follow that order.

> **23.1.2.1 Operator Precedence and Associativity**  
> The precedence and associativity of common infix operators **MUST** follow the table below (from lowest to highest precedence); operators in the same row associate left-to-right unless otherwise noted:
>
> | Level | Operators                                | Associativity    |
> | ----- | ---------------------------------------- | ---------------- |
> | 1     | `||`                                     | left-to-right    |
> | 2     | `&&`                                     | left-to-right    |
> | 3     | `==`, `!=`                               | left-to-right    |
> | 4     | `<`, `<=`, `>`, `>=`                     | left-to-right    |
> | 5     | `<<`, `>>`                               | left-to-right    |
> | 6     | `+`, `-`                                 | left-to-right    |
> | 7     | `*`, `/`, `%`                            | left-to-right    |
> | 8     | `..`, `..=`                              | left-to-right    |
> | 9     | `**`                                     | right-to-left    |
>
> Unary operators (such as unary `-` and logical negation) bind more tightly than any of the infix operators above and are applied after postfix expression formation but before binary operators. Implementations **MUST NOT** reorder or regroup operators in ways that violate this precedence table.

> **23.1.3 Variable Bindings**  
> Variable bindings introduced by `let` or `var` **MUST** be typed using the static type of the initializing expression, possibly subject to explicit type annotations.  
> Bindings declared without `var` are immutable by default; attempts to assign to an immutable binding **MUST** be rejected with an error diagnostic in the `EXP-02` or `TYP-04` families.

## 24. Statements & Control Flow [statements]

> **24.1.1 Statement Well-Formedness**
> A statement (§4.3 [terminology.programming]) is well-formed only if all expressions it contains are well-typed, all referenced bindings are in scope, and all control-flow constructs satisfy their structural requirements.  
> Statements that violate these conditions **MUST** be rejected with diagnostics in the `E-EXP-02xx` family; implementations **MUST NOT** attempt to repair or reinterpret ill-formed statements silently.

> **24.1.2 Procedure Bodies and Returns**  
> For procedures with a non-unit result type, every terminating control-flow path through the body **MUST** either return a value of the declared result type or diverge.  
> If the implementation cannot prove that all terminating paths respect this requirement, it **MUST** emit `E-EXP-0201` or an equivalent diagnostic and **MUST NOT** treat the procedure as well-formed.

> **24.1.3 Loop and Branch Constructs**  
> `break` and `continue` statements **MUST** appear only within the dynamic extent of loop constructs defined by this Part; appearances outside such contexts **MUST** be rejected with diagnostics in the `E-EXP-0204` and `E-EXP-0205` families.  
> Branching constructs **MUST** have conditions of boolean type; if a condition does not have boolean type and no permitted conversion applies, the implementation **MUST** emit `E-EXP-0101` or `E-TYP-0101`.

## 25. Pattern Matching [patterns]

> **25.1.1 Pattern Typing and Compatibility**  
> Each pattern in a binding or match construct **MUST** be typed against the static type of the value being matched. Patterns whose structure or component types are incompatible with the scrutinee type **MUST** be rejected with `E-EXP-0208` or a related type error.  
> The implementation **MUST** ensure that bindings introduced by patterns receive the static types implied by the pattern and scrutinee type.

> **25.1.2 Exhaustiveness and Reachability**  
> When the specification requires a pattern match to be exhaustive for a given scrutinee type, the implementation **MUST** either verify exhaustiveness or reject the match as ill-formed with `E-EXP-0206`.  
> Pattern arms that can never match any value of the scrutinee type, given the preceding patterns, **MUST** be diagnosed as unreachable with `E-EXP-0207`.

> **25.1.3 Non-overlapping Bindings**  
> For patterns that introduce bindings, the implementation **MUST** ensure that no identifier is introduced multiple times in the same pattern in a way that would create multiple bindings for the same name in the same scope. Violations **MUST** be rejected with an error diagnostic in the `SRC-15` or `EXP-02` families.

---

# Part VI - Generics & Parametric Polymorphism

This part defines Cursive's generic programming model, including type and region parameters, bounds, variance, and monomorphization.

*(Informative)*

| Theme                                                                     | Key Diagnostics    |
| ------------------------------------------------------------------------- | ------------------ |
| All generic parameters referenced are declared and in scope.              | `E-TYP-0301, 0308` |
| All declared bounds and constraints are satisfied at instantiation sites. | `E-TYP-0302, 0306` |
| Declared variance annotations are consistent with actual usage.           | `E-TYP-0303`       |
| Generic instantiation and recursion terminate or are rejected.            | `E-TYP-0305`       |
| Region parameters used with generics respect region-safety rules.         | `E-MEM-0301–0304`  |

## 26. Type Parameters & Generic Declarations [generics.types]

> **26.1.1 Type Parameter Declaration**  
> Generic declarations **MUST** explicitly list all type parameters they use. Each type parameter name **MUST** be unique within the declaration’s parameter list.  
> Any reference to a type parameter outside the scope of its declaration **MUST** be rejected with `E-TYP-0301`.

> **26.1.2 Generic Declaration Well-Formedness**  
> A generic declaration is well-formed only if all its parameters, bounds, and body types are themselves well-formed according to Part IV and this Part.  
> Implementations **MUST NOT** instantiate or use a generic declaration that is not well-formed.

## 27. Region Parameters [generics.regions]

> **27.1.1 Region Parameter Declaration**  
> When a declaration introduces region parameters, those parameters **MUST** be listed explicitly and **MUST** be distinguishable from type parameters.  
> Region parameters **MUST** be used only in positions where the memory and region model permits region abstraction; uses in unsupported positions **MUST** be rejected with diagnostics in the `E-MEM-0301` or `E-MEM-0304` families.

> **27.1.2 Region Argument Consistency**  
> When instantiating a declaration with region parameters, the implementation **MUST** ensure that provided region arguments satisfy all region constraints declared by the generic.  
> Mismatches between region arguments and region parameters **MUST** be diagnosed with `E-MEM-0301` or another region-parameter diagnostic defined in Appendix B. Region constraints **MUST** be interpreted in a way that is consistent with the region lifetime and escape rules of Part VII (§33.1.1–§33.1.2); implementations **MUST NOT** accept an instantiation that would permit values to outlive their regions or violate those rules.

## 28. Bounds & Where Constraints [generics.bounds]

> **28.1.1 Bound Satisfaction**  
> Each bound or constraint declared on a type or region parameter **MUST** be checked at every instantiation site.  
> If the implementation cannot establish that a bound holds, it **MUST** reject the instantiation with `E-TYP-0302` or `E-GRN-0201` as appropriate.

> **28.1.2 Contract and Behavior Bounds**  
> When bounds reference contracts, behaviors, or associated types, the implementation **MUST** ensure that all required witnesses or implementations exist and are visible at the instantiation site.  
> Missing or incompatible witnesses **MUST** be diagnosed with `E-GRN-0401` or `E-TYP-0306`.

## 29. Variance & Inference [generics.variance]

> **29.1.1 Variance Declaration and Checking**  
> If the language permits explicit variance annotations on type parameters, the implementation **MUST** verify that each annotation is consistent with the parameter’s usage in the declaration.  
> Inconsistent variance between the annotation and inferred usage **MUST** be rejected with `E-TYP-0303`.

> **29.1.2 Type Parameter Inference**  
> When type parameters are inferred rather than explicitly specified, the implementation **MUST** either compute a unique principal solution or reject the program as ambiguous.  
> Situations where multiple distinct, well-typed solutions exist and no rule selects among them **MUST** be diagnosed with `E-TYP-0307`.

## 30. Resolution & Monomorphization [generics.resolution]

> **30.1.1 Instantiation and Resolution**  
> For each use of a generic declaration, the implementation **MUST** resolve type and region arguments, apply bounds and constraints, and obtain a concrete instance prior to code generation.  
> Instantiations that depend on unresolved or ill-formed arguments **MUST** be rejected rather than deferred to runtime.

> **30.1.2 Instantiation Limits and Termination**  
> Implementations **MUST** enforce limits on generic instantiation depth and count that are at least as permissive as the minima in §6.4.1 and Appendix C.  
> Recursive or mutually recursive instantiations that would exceed these limits or fail to terminate **MUST** be rejected with `E-TYP-0305` or a related diagnostic, and implementations **MUST NOT** enter unbounded instantiation loops.

---

# Part VII - Memory Model, Regions, & Permissions

*(Partially normative; see completion matrix in §Specification Completion Matrix)*

| Theme                                                                                       | Key Diagnostics                    |
| ------------------------------------------------------------------------------------------- | ---------------------------------- |
| Safe code preserves memory safety (no out-of-bounds, use-after-destroy, or double-destroy). | `E-MEM-0101–0108, 0401–0406`       |
| Region-based allocations respect region lifetimes and do not escape improperly.             | `E-MEM-0301–0306`                  |
| Pointer provenance and alignment rules are respected.                                       | `E-MEM-0401–0407, E-FFI-0401–0404` |
| Permission lattice constraints are enforced at static and dynamic boundaries.               | `E-MEM-0201–0203, E-TYP-0401–0404` |
| Layout and aliasing properties match the documented ABI and conformance dossier.            | `E-FFI-0105–0106`                  |

## 31. Memory Model Overview [memory.model]

> **31.1.1 Memory Objects and State**  
> The implementation **MUST** model program memory as a collection of objects with well-defined lifetimes, types, and storage locations.  
> Operations that read or write memory **MUST** be defined in terms of these objects and their associated permissions, regions, and layouts.

> **31.1.2 Safety in Safe Code**  
> For programs that do not use `unsafe` constructs or foreign function calls, executions **MUST NOT** perform out-of-bounds reads or writes, use-after-destruction, or double-destruction of the same object.  
> When the implementation cannot guarantee these properties statically, it **MUST** insert checks or reject the program rather than allowing such operations to proceed without classification as UVB.

## 32. Objects & Storage Duration [memory.objects]

> **32.1.1 Object Lifetime**
> Each object (§4.3 [terminology.programming]) **MUST** have a well-defined lifetime, beginning at initialization and ending at destruction.  
> Reads or writes to objects outside their lifetime **MUST** be rejected or prevented by the implementation; if such accesses occur in safe code, they **MUST** be diagnosed with memory-model errors in the `E-MEM-01xx` or `E-MEM-04xx` families.

> **32.1.2 Storage Duration Categories**  
> Cursive distinguishes at least between stack-allocated, heap-allocated, and region-allocated storage durations.  
> Implementations **MUST** ensure that storage duration categories interact with region and permission rules such that safe code cannot observe dangling references.

## 33. Regions & Region Safety [memory.regions]

> **33.1.1 Region Scope and Lifetime**  
> A region construct **MUST** delimit the lifetime of objects allocated within that region. When a region scope ends, all objects allocated in that region **MUST** be considered destroyed, and further access **MUST** be rejected or prevented.  
> Values that would escape a region in a way that extends their lifetime beyond the region **MUST** be rejected with diagnostics such as `E-MEM-0302` or `E-MEM-0305`.

> **33.1.2 Cross-Region References**  
> References between regions **MUST** obey lifetime relationships defined by this Part. A region that contains references to objects in another region **MUST NOT** outlive the referent region unless an explicit relationship is established.  
> Violations of region-lifetime relationships **MUST** be diagnosed with `E-MEM-0303` or `E-MEM-0306`.

## 34. Permissions (Lexical Permission System) [memory.permissions]

> **34.1.1 Permission Categories**  
> The permission system **MUST** define a finite set of permissions describing allowed access to values (for example, read-only, shared mutable, exclusive).  
> For any value in safe code, the implementation **MUST** associate permissions that determine which operations are permitted; operations that require stronger permissions than currently held **MUST** be rejected with `E-MEM-0203` or `E-TYP-0401`.

> **34.1.2 Permission Lattice and Transitions**  
> Permissions **MUST** form a lattice or partial order describing valid upgrades and downgrades.  
> The implementation **MUST** enforce that all permission transitions at static or dynamic boundaries respect this ordering; invalid transitions **MUST** be diagnosed with `E-MEM-0201–0202` or `E-TYP-0402–0403`.

> **34.1.3 Static and Dynamic Enforcement**  
> For each operation that reads or writes memory through a pointer, reference, or aliasable handle, the specification **MUST** classify whether:
>
> 1. The required permissions can be established purely statically from typing and region information; or  
> 2. A runtime check is required to ensure that the current permissions suffice.
>
> In **strict** conformance mode (§6.2.7), operations whose required permissions cannot be established statically **MUST** either be rejected with an appropriate `E-MEM-02xx`/`E-TYP-04xx` diagnostic or proven safe by compile‑time reasoning; implementations **MUST NOT** silently insert additional runtime checks beyond those mandated elsewhere in this specification. In dynamic verification modes, implementations **MAY** insert runtime checks for such operations, but any failure of a permission check at runtime **MUST** be treated as a memory-safety violation and reported using `E-MEM-01xx`/`E-MEM-04xx` diagnostics rather than being classified as UVB.

## 35. Move/Copy/Clone Semantics [memory.semantics]

> **35.1.1 Responsibility and Moves**  
> Each value that requires cleanup **MUST** have a well-defined set of bindings responsible for invoking its destructor or equivalent cleanup.  
> Moving a value from a non-responsible binding **MUST** be rejected with `E-MEM-0101`; using a value after it has been moved out of a responsible binding **MUST** be rejected with `E-MEM-0102`.

> **35.1.2 Partial Moves and Aggregates**  
> For aggregate types that are not designated as freely copyable, moving out individual fields **MUST** either be forbidden or must follow rules that leave the aggregate in a state that is safe to destroy.  
> Violations of these rules, including partial moves that would cause double-destruction or use of uninitialized subobjects, **MUST** be diagnosed using `E-MEM-0103` and related codes.

## 36. Layout, Alignment, & Aliasing [memory.layout]

> **36.1.1 Alignment Requirements**  
> Each type **MUST** have an alignment, and the implementation **MUST** ensure that objects of that type are stored at addresses that respect this alignment.  
> Dereferencing a pointer that does not satisfy the alignment requirements of its referent type **MUST** result in a diagnostic such as `E-MEM-0406` in safe code.

> **36.1.2 Layout and Aliasing Guarantees**  
> Implementations **MUST** document implementation-defined layout properties in the conformance dossier and **MUST NOT** assume stronger aliasing guarantees than those specified when performing optimizations.
>
> **36.1.3 Pointer Provenance Modes**  
> The `implementation_defined_behaviors.pointer_semantics.provenance_tracking` field in the conformance dossier (§C.2.3.2) **MUST** be interpreted as follows:
>
> - `"strict"`: The implementation tracks pointer provenance precisely enough that dereferences which would violate object bounds, lifetime, or alignment in safe code are either rejected statically or trapped at runtime, and optimizations **MUST NOT** assume aliasing properties stronger than those derivable from this tracking.  
> - `"permissive"`: The implementation preserves provenance information sufficient to uphold the safety guarantees of §31–§35 in safe code but **MAY** permit additional behaviors in the presence of certain implementation-defined casts or FFI calls, as documented in the dossier.  
> - `"none"`: The implementation does not provide provenance-based guarantees beyond those implied by the abstract object model; optimizations **MUST NOT** rely on provenance to distinguish pointers that may alias.
>
> Implementations **MUST** ensure that the chosen provenance mode is consistent with the behavior of pointer creation, casts, arithmetic, and dereference described in Part VII and Appendix C, and **MUST** classify violations in safe code using `E-MEM-0401–0404` as appropriate.

---

# Part VIII - Contracts & Grants

*(Informative)*

| Theme                                                                                   | Key Diagnostics               |
| --------------------------------------------------------------------------------------- | ----------------------------- |
| Procedure calls respect declared preconditions and postconditions.                      | `E-GRN-0201, 0202, 0205`      |
| Loop and type invariants are maintained where declared.                                 | `E-GRN-0203, 0204`            |
| Grants required for operations are declared and not leaked beyond their intended scope. | `E-GRN-0101–0104, E-SRC-0206` |
| Contract refinements obey substitutability requirements.                                | `E-GRN-0301–0303`             |

## 37. Contract Model & Sequent Syntax [contracts.model]

> **37.1.1 Contract Association**  
> Contracts **MUST** be associated explicitly with procedures, types, or behaviors using the syntax defined in this Part.  
> When a contract is present, the implementation **MUST** treat its clauses as normative obligations on calls and implementations, not as advisory documentation.

> **37.1.2 Sequent Structure**  
> The contract language **MUST** define a structured form for specifying required grants, preconditions, and postconditions.  
> The implementation **MUST** interpret these components consistently when checking calls and implementations, and **MUST** map violations to diagnostics in the `E-GRN-02xx` family.
>
> **37.1.3 Contract Syntax Overview**  
> At a minimum, Cursive **MUST** support attaching contracts to procedure declarations using a block of contract clauses with the following abstract grammar:
>
> ```ebnf
> contract_block
>     ::= "contract" "{" contract_clause* "}"
>
> contract_clause
>     ::= "requires"  predicate_list
>      |  "ensures"   predicate_list
>      |  "grants"    grant_list
>      |  "invariant" predicate_list
>
> predicate_list
>     ::= <expression> ("," <expression>)*
>
> grant_list
>     ::= <grant_identifier> ("," <grant_identifier>)*
> ```
>
> where `<expression>` ranges over boolean expressions in the host language and `<grant_identifier>` ranges over grant names declared according to §38.1.1. Implementations **MUST** reject contract blocks that are ill-formed according to this grammar with diagnostics in the `E-GRN-0202` or `E-GRN-0205` families.

## 38. Grants & Grant Declarations [contracts.grants]

> **38.1.1 Grant Declarations**  
> Grants **MUST** be declared as named capabilities that procedures or modules may require to perform specific classes of operations (for example, file I/O or network access).  
> A procedure that uses a grant-restricted operation **MUST** declare the corresponding grant in its contract; failure to do so **MUST** be diagnosed with `E-GRN-0102` or `E-SRC-0206`.
>
> Grant declarations **SHOULD** follow a simple, declarative form at module scope such as:
>
> ```ebnf
> grant_decl
>     ::= "grant" <grant_identifier> ";"
> ```
>
> A grant identifier declared in this way **MUST** denote a capability whose semantics are documented either in the specification or in implementation-specific extension documentation, and **MUST** be eligible for use in `grants` clauses and in the conformance dossier’s `extensions`/`implementation_defined_behaviors` sections.

> **38.1.2 Grant Usage and Accumulation**  
> During static analysis, the implementation **MUST** track grant requirements through calls and higher-order functions.  
> If an operation requires a grant that is not available in the current context, the implementation **MUST** emit `E-GRN-0101`. Grant leakage that would allow untrusted code to use capabilities beyond its declared contract **MUST** be diagnosed with `E-GRN-0103` or `E-GRN-0104`.

## 39. Preconditions & Postconditions [contracts.conditions]

> **39.1.1 Preconditions on Callers**  
> Preconditions declared for a procedure **MUST** be treated as requirements on all call sites.  
> If the implementation cannot establish that a precondition holds at a call, it **MUST** either reject the program or insert runtime checks according to the verification mode; static failures **MUST** be reported using `E-GRN-0201`.

> **39.1.2 Postconditions on Implementations**  
> Postconditions **MUST** be treated as obligations of the procedure body.  
> If the implementation cannot show that a postcondition holds on all terminating paths, it **MUST** reject the implementation with `E-GRN-0202` or perform dynamic checking if permitted by the verification mode.

## 40. Invariants [contracts.invariants]

> **40.1.1 Type and Data Structure Invariants**  
> When invariants are declared for types or data structures, the implementation **MUST** enforce that all operations that can observe or mutate those structures preserve the invariants.  
> Violations detected statically or dynamically **MUST** be diagnosed with `E-GRN-0204`.

> **40.1.2 Loop Invariants**  
> Loop invariants, when present, **MUST** hold before the first iteration of the loop and after each iteration.  
> If the implementation cannot prove that a loop invariant is maintained, it **MUST** reject the loop with `E-GRN-0203` or emit diagnostics indicating that the invariant may not be preserved.

## 41. Verification Flow [contracts.verification]

> **41.1.1 Static vs Dynamic Verification**  
> Contracts are either enforced statically, enforced dynamically at runtime, or both, according to the verification model defined in this Part.  
> Implementations **MUST** document which contract features are statically enforced and which may require runtime checks; dynamic checks **MUST** be inserted in a way that preserves the behavior classifications of Part I.
> Optimizations and code transformations **MUST NOT** strengthen preconditions or weaken postconditions and invariants implied by contracts; any transformation that would do so **MUST** be rejected as non-conforming.

> **41.1.2 Treatment of Unverifiable Contract Properties**  
> Contract properties that cannot be verified within the language’s verification model **MUST** be treated as UVB sites when they affect correctness, or as IFNDR sites when they render the program ill-formed but impractical to diagnose.  
> Implementations **MUST** classify such sites consistently and reflect any UVB that depends on external proofs in the conformance dossier as required by §6.2.4.

---

# Part IX - Behaviors & Dynamic Dispatch

*(Informative)*

| Theme                                                                                                 | Key Diagnostics   |
| ----------------------------------------------------------------------------------------------------- | ----------------- |
| Behavior declarations are well-formed and reference only well-formed signatures and associated types. | `E-TYP-0501–0505` |
| Behavior implementations satisfy all required operations and associated constraints.                  | `E-GRN-0401–0402` |
| Witnesses for behavior satisfaction obey coherence and orphan rules.                                  | `E-GRN-0403–0404` |

## 42. Behaviors (Interface/Trait Declarations) [behaviors.declarations]

> **42.1.1 Behavior Declaration**  
> A behavior declaration **MUST** specify a finite set of required operations and, where applicable, associated types or constants.  
> Each required operation **MUST** have a well-formed type signature; behavior declarations that reference ill-formed types or contracts **MUST** be rejected with diagnostics in the `E-TYP-0501–0505` range.

> **42.1.2 Behavior Bounds on Types**  
> When a type declares that it satisfies a behavior, the implementation **MUST** treat that declaration as a promise that all behavior requirements are met by at least one implementation.  
> Missing or incomplete implementations **MUST** be diagnosed with an appropriate behavior or contract diagnostic.

## 43. Behavior Implementations [behaviors.implementations]

> **43.1.1 Implementation Completeness**  
> A behavior implementation for a given type **MUST** provide concrete definitions for all required operations and associated items declared by the behavior, subject to any defaulting rules defined by this Part.  
> Implementations that omit required elements **MUST** be rejected with `E-GRN-0401` or a related diagnostic.

> **43.1.2 Substitutability and Contracts**  
> When behaviors interact with contracts, implementations **MUST** respect substitutability constraints: preconditions **MUST NOT** be strengthened and postconditions **MUST NOT** be weakened relative to any inherited contracts.  
> Violations **MUST** be diagnosed with `E-GRN-0301–0303`.

## 44. Witness System & vtables [behaviors.witnesses]

> **44.1.1 Witness Representation**  
> The witness system **MUST** provide a representation of the fact that a type satisfies a behavior, suitable for use at runtime to enable dynamic dispatch.  
> Witnesses **MUST** be constructed only when all behavior requirements are met; attempts to construct witnesses without complete implementations **MUST** be rejected with `E-GRN-0401`.

> **44.1.2 Coherence and Orphan Rules**  
> Implementations **MUST** enforce coherence rules so that, for any given (type, behavior) pair, there is at most one applicable behavior implementation visible in a given program. The **coherence domain** for a program is the set of all assemblies that are linked together to form the final executable; within this domain, behavior resolution for a (type, behavior) pair **MUST** be unambiguous.
>
> A behavior implementation is said to be **orphaned** when neither the behavior nor the implementing type are declared in the same assembly as the implementation itself. Implementations **MUST** forbid orphan implementations: for every behavior implementation, at least one of the following **MUST** hold:
>
> 1. The behavior is declared in the same assembly as the implementation; or  
> 2. The implementing type is declared in the same assembly as the implementation.
>
> Witnesses that violate coherence or orphan rules (for example, multiple visible implementations for the same (type, behavior) pair within the coherence domain, or implementations where neither the behavior nor the type are local to the current assembly) **MUST** be rejected with `E-GRN-0403` or `E-GRN-0404`.

---

# Part X - Compile-Time Evaluation & Reflection

*(Informative)*

| Theme                                                                                                                 | Key Diagnostics                    |
| --------------------------------------------------------------------------------------------------------------------- | ---------------------------------- |
| Compile-time evaluation respects resource limits and termination conditions.                                          | `E-SRC-0201–0205, E-EXP-0401–0403` |
| Compile-time expressions appear only in contexts where they are permitted and produce constant results when required. | `E-EXP-0402`                       |
| Code generation APIs produce well-formed code without name collisions or hygiene violations.                          | `E-SRC-0207, E-EXP-0501–0504`      |

## 45. Comptime Model & Comptime Declarations [comptime.model]

> **45.1.1 Comptime Contexts**  
> Implementations **MUST** ensure that compile-time code is executed only in designated comptime contexts and that side effects of comptime execution are limited to compile-time-visible state and generated code.

> **45.1.2 Comptime Constant Requirements**  
> Where the grammar or type system requires a compile-time constant, the implementation **MUST** either evaluate the expression at compile time or reject it with `E-EXP-0402`.  
> Expressions that depend on runtime-only information **MUST NOT** be used where compile-time constants are required.

## 46. Comptime Blocks & Contexts [comptime.blocks]

> **46.1.1 Resource Limits**  
> Compile-time evaluation **MUST** respect the resource limits specified in §6.4.1 and Appendix B (`E-SRC-0201–0205`).  
> When evaluation exceeds recursion depth, step count, memory, or collection-size limits, the implementation **MUST** abort evaluation of the affected comptime block and emit the corresponding diagnostic.

> **46.1.2 Isolation of Comptime State**  
> The state used by compile-time evaluation **MUST** be isolated from runtime state except where explicitly defined by this Part.  
> Implementations **MUST NOT** allow compile-time state mutations that would violate the determinism or reproducibility of compile-time execution.

## 47. Opt-In Reflection & Attributes [comptime.reflection]

> **47.1.1 Opt-In Reflection**  
> Reflection capabilities **MUST** be opt-in and **MUST NOT** impose overhead on code that does not request them.  
> Types and declarations that do not explicitly enable reflection **MUST NOT** have reflection metadata generated or exposed.

> **47.1.2 Attribute Semantics**  
> Attributes **MUST** be treated as structured metadata attached to declarations.  
> The implementation **MUST** interpret attributes consistently with the rules in this Part and **MUST NOT** allow attributes to silently change the meaning of programs in ways not specified here.

## 48. Type Metadata & Queries [comptime.metadata]

> **48.1.1 Metadata Availability**  
> Type metadata and query facilities **MUST** reflect the same type information used by the type system during static analysis.  
> Queries that would expose information inconsistent with static typing **MUST** be rejected or treated as UVB if they depend on unverifiable assumptions.

> **48.1.2 Metadata Costs**  
> Implementations **MUST** ensure that metadata generation for reflection or tooling does not change the memory layout or semantics of types unless explicitly requested by the program.

## 49. Code Generation API [comptime.codegen]

> **49.1.1 Generated Code Well-Formedness**  
> Code generation APIs **MUST** produce code that is subject to the same well-formedness, typing, and safety checks as user-authored code.  
> Generated code that is ill-formed or ill-typed **MUST** be rejected with `E-EXP-0502` or a related diagnostic.

> **49.1.2 Name Collisions and Hygiene**  
> When generating identifiers, the implementation **MUST** avoid collisions with existing declarations in the applicable scopes.  
> Name collisions detected during generation or validation **MUST** be diagnosed with `E-SRC-0207`, and hygiene violations **MUST** be reported using `E-EXP-0503` or `E-EXP-0504`.

---

# Part XI - Concurrency & Memory Ordering

*(Informative)*

| Theme                                                                       | Key Diagnostics   |
| --------------------------------------------------------------------------- | ----------------- |
| Safe code is free from data races.                                          | `E-MEM-0501–0504` |
| Atomic operations obey the specified memory ordering semantics.             | `E-MEM-0502–0504` |
| Synchronization primitives are used according to their specified contracts. | `E-MEM-0501–0504` |

## 50. Concurrency Model [concurrency.model]

> **50.1.1 Threads and Tasks**  
> In safe code, concurrent accesses to shared memory **MUST** either be synchronized using the primitives defined in this Part or be statically proven non-conflicting; otherwise, they **MUST** be diagnosed with `E-MEM-0501` or related codes.

> **50.1.2 Communication and Synchronization**  
> Implementations **MUST** ensure that synchronization operations enforce the required happens-before relationships and **MUST NOT** perform optimizations that violate those relationships.

> **50.1.3 Data-Race-Free Semantics**  
> The concurrency semantics of Cursive **MUST** satisfy a data-race-free (DRF) guarantee: if a program is free of data races according to the rules of this Part (that is, all conflicting accesses are ordered by happens-before via synchronization or permissions), then all executions of that program are observationally equivalent to some sequentially consistent interleaving of its operations. Optimizations **MUST NOT** introduce new data races or change the observable behavior of data-race-free programs.

## 51. Memory Ordering & Atomics [concurrency.atomics]

> **51.1.1 Atomic Types and Operations**  
> Atomic types and operations **MUST** provide well-defined memory ordering semantics, including at least a strongest ordering that is sequentially consistent.  
> Using atomic operations with unsupported orderings or on types that are not declared atomic **MUST** be diagnosed with `E-MEM-0502–0503`.

> **51.1.2 Ordering Guarantees**  
> The implementation **MUST** document which memory orderings are supported for each atomic type and how they interact with the concurrency model.  
> Misuse of ordering annotations that violates required ordering guarantees **MUST** be rejected or diagnosed with appropriate concurrency diagnostics.
>
> For each atomic operation, the implementation **MUST** interpret ordering annotations drawn from a fixed set that includes at least:
>
> - `Relaxed` – no cross-thread ordering guarantees beyond modification order for the affected atomic location;  
> - `Acquire` / `Release` / `AcqRel` – orderings that establish one-way or two-way happens-before edges between threads for participating operations; and  
> - `SeqCst` – a strongest ordering that induces a single global total order on all sequentially consistent atomic operations consistent with happens-before.
>
> The precise set of supported orderings **MUST** be documented per atomic type in the conformance dossier’s `implementation_defined_behaviors.concurrency.atomic_support` table, and attempts to request orderings not listed there **MUST** be diagnosed using `E-MEM-0502` or a related concurrency diagnostic.

---

# Part XII - Interoperability & ABI

*(Informative)*

| Theme                                                                                          | Key Diagnostics         |
| ---------------------------------------------------------------------------------------------- | ----------------------- |
| FFI interfaces use only types that have well-defined foreign representations.                  | `E-FFI-0101–0102`       |
| Calling conventions and linkage attributes are consistent with the target ABI.                 | `E-FFI-0103, 0201–0204` |
| Panics and unwinds do not cross FFI boundaries except where explicitly allowed.                | `E-FFI-0301–0305`       |
| Pointer provenance, lifetime, and mutability requirements are preserved across FFI boundaries. | `E-FFI-0401–0404`       |

## 52. Foreign Function Interface [ffi]

> **52.1.1 FFI Declarations**  
> Foreign functions and types **MUST** be declared using the FFI syntax defined by this Part.  
> Types used in FFI signatures **MUST** have well-defined representations in the target ABI; types that cannot be represented **MUST** be rejected with `E-FFI-0101` or `E-FFI-0102`.

> **52.1.2 Panic and Unwind Boundaries**  
> Panics and language-level unwinding mechanisms **MUST NOT** cross FFI boundaries unless this Part explicitly permits them for a particular ABI mode.  
> Potential cross-boundary panics or unwinds **MUST** be diagnosed with `E-FFI-0301–0302` or handled according to the platform’s defined behavior.
>
> **52.1.3 FFI ABI Modes**  
> For each supported target, the implementation **MUST** define a finite set of FFI ABI modes (for example, `"C"`, `"system"`, or other platform-specific conventions) and **MUST** document, in the conformance dossier’s `implementation_defined_behaviors.ffi.calling_conventions` table, whether language-level unwinding is permitted to cross boundaries using that ABI mode. At minimum:
>
> 1. There **MUST** be at least one ABI mode for which language-level unwinding across the boundary is forbidden; panics propagating across such a boundary **MUST** be diagnosed with `E-FFI-0301–0302` or cause immediate process termination in a documented way.  
> 2. If the implementation supports any ABI mode that permits unwinding across the boundary, the conditions under which such unwinding is sound (for example, restrictions on foreign frames or required annotations on foreign callers) **MUST** be documented, and misuse **MUST** be diagnosed with appropriate `E-FFI-03xx` diagnostics.

## 53. ABI & Binary Layout [abi]

> **53.1.1 ABI Documentation**  
> Implementations **MUST** document, in the conformance dossier (Appendix C), the calling conventions, type layouts, and linkage conventions they use for each supported target.  
> Differences in ABI that affect portability **MUST** be classified as implementation-defined behavior and documented accordingly.

> **53.1.2 Layout Compatibility**  
> Types intended for use across FFI boundaries **MUST** have layouts that are compatible with the corresponding foreign types as defined by the target ABI.  
> Mismatches that could cause misinterpretation of data **MUST** be diagnosed with `E-FFI-0105–0106`.

## 54. Linkage & Symbol Resolution [linkage]

> **54.1.1 Symbol Visibility and Linkage**  
> Symbol visibility and linkage attributes control whether functions and data are available to other translation units or foreign code.  
> Implementations **MUST** enforce the rules for visibility and linkage such that symbol collisions, unresolved references, or inconsistent linkage specifications are diagnosed rather than silently ignored.

> **54.1.2 Cross-Version and Cross-Language Linking**  
> When linking Cursive code compiled against different language or ABI versions, or when linking with foreign code, the implementation **MUST** obey the binary compatibility requirements of §6.3 and this Part.  
> Attempts to link units with incompatible ABIs **MUST** be diagnosed with appropriate conformance or ABI diagnostics and **MUST NOT** produce executables with undefined linkage behavior.

---

# Appendix A – Formal ANTLR Grammar (Normative)

**See [Appendix-A-Formal-ANTLR-Grammar.md](./Appendix-A-Formal-ANTLR-Grammar.md)**

---

# Appendix B – Diagnostic Code Taxonomy (Normative)

**See [Appendix-B-Diagnostic-Code-Taxonomy.md](./Appendix-B-Diagnostic-Code-Taxonomy.md)**

---

# Appendix C – Conformance Dossier Schema (Normative); IDB & UVB Attestation Index

**See [Appendix-C-Conformance-Dossier.md](./Appendix-C-Conformance-Dossier.md)**

---

# Appendix D – Diagnostic Catalog (Normative)

**See [Appendix-D-Diagnostic-Catalog.md](./Appendix-D-Diagnostic-Catalog.md)**

---

# Appendix E – Conformance Verification

**See [Appendix-E-Conformance-Verification.md](./Appendix-E-Conformance-Verification.md)**

---

# Appendix F – Tooling Integration

**See [Appendix-F-Tooling-Integration.md](./Appendix-F-Tooling-Integration.md)**
