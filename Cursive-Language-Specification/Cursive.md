# The Cursive Language Specification

- [The Cursive Language Specification](#the-cursive-language-specification)
- [Part I: General and Lexical](#part-i-general-and-lexical)
  - [1. Introduction](#1-introduction)
    - [1.1 Scope and Conformance](#11-scope-and-conformance)
    - [1.2 Glossary and Notation](#12-glossary-and-notation)
    - [1.3 Implementation Limits](#13-implementation-limits)
    - [1.4 Behavior Classifications](#14-behavior-classifications)
  - [2. Lexical Structure](#2-lexical-structure)
    - [2.1 Source Text \& Encoding](#21-source-text--encoding)
    - [2.2 Line Structure and Termination](#22-line-structure-and-termination)
    - [2.3 Comments and Whitespace](#23-comments-and-whitespace)
    - [2.4 Tokens and Vocabulary](#24-tokens-and-vocabulary)
    - [2.5 Identifiers and Keywords](#25-identifiers-and-keywords)
    - [2.6 Literals](#26-literals)
    - [2.7 Operators and Punctuation](#27-operators-and-punctuation)
    - [2.8 Lexical Security](#28-lexical-security)
    - [2.9 Translation Phases](#29-translation-phases)
- [Part II: The Abstract Machine](#part-ii-the-abstract-machine)
  - [3. The Object and Memory Model](#3-the-object-and-memory-model)
    - [3.1 Objects and Storage Duration](#31-objects-and-storage-duration)
    - [3.2 Provenance and Lifetimes](#32-provenance-and-lifetimes)
    - [3.3 The Binding Model](#33-the-binding-model)
    - [3.4 Responsibility and Moves](#34-responsibility-and-moves)
    - [3.5 Deterministic Destruction](#35-deterministic-destruction)
    - [3.6 The Abstract Machine and Observable Behavior](#36-the-abstract-machine-and-observable-behavior)
    - [3.7 Unsafe Memory Operations](#37-unsafe-memory-operations)
  - [4. The Authority Model](#4-the-authority-model)
    - [4.1 Capability Authority Concepts](#41-capability-authority-concepts)
    - [4.2 The Root of Authority (`Context`)](#42-the-root-of-authority-context)
    - [4.3 Attenuation Semantics](#43-attenuation-semantics)
- [Part III: Program Structure](#part-iii-program-structure)
  - [5. Modules and Compilation](#5-modules-and-compilation)
    - [5.1 Compilation Units and Projects](#51-compilation-units-and-projects)
    - [5.3 Module Paths and Discovery](#53-module-paths-and-discovery)
  - [6. Scopes and Names](#6-scopes-and-names)
    - [6.1 Lexical Scopes and Program Points](#61-lexical-scopes-and-program-points)
    - [6.2 Name Resolution and Shadowing](#62-name-resolution-and-shadowing)
    - [6.3 Visibility and Access Control](#63-visibility-and-access-control)
  - [6.4 Import and Using Declarations](#64-import-and-using-declarations)
  - [7. Attributes and Metadata](#7-attributes-and-metadata)
    - [7.1 Attribute Syntax and Registry](#71-attribute-syntax-and-registry)
    - [7.2 Layout Attributes (`[[layout]]`)](#72-layout-attributes-layout)
    - [7.3 Optimization Hints](#73-optimization-hints)
    - [7.4 Diagnostics and Metadata](#74-diagnostics-and-metadata)
    - [7.5 FFI Attributes](#75-ffi-attributes)
  - [8. Program Lifecycle](#8-program-lifecycle)
    - [8.1 Top-Level Declarations](#81-top-level-declarations)
    - [8.2 Module Initialization Order](#82-module-initialization-order)
    - [8.3 Program Entry Point (`main`)](#83-program-entry-point-main)
- [Part IV: The Type System](#part-iv-the-type-system)
  - [9. Type System Fundamentals](#9-type-system-fundamentals)
    - [9.1 Architecture and Equivalence](#91-architecture-and-equivalence)
    - [9.2 Subtyping and Coercion](#92-subtyping-and-coercion)
    - [9.3 Variance](#93-variance)
    - [9.4 Type Inference](#94-type-inference)
  - [10. The Permission System](#10-the-permission-system)
    - [10.1 Permission Lattice (`const`, `unique`, `shared`)](#101-permission-lattice-const-unique-shared)
    - [10.2 Exclusivity and Aliasing Rules](#102-exclusivity-and-aliasing-rules)
    - [10.3 Permission Subtyping and Coercion](#103-permission-subtyping-and-coercion)
  - [11. Concrete Data Types](#11-concrete-data-types)
    - [11.0 Type Syntax](#110-type-syntax)
    - [11.1 Primitive Types](#111-primitive-types)
    - [11.2 Tuples](#112-tuples)
    - [11.3 Arrays](#113-arrays)
    - [11.4 Slices](#114-slices)
    - [11.5 Ranges](#115-ranges)
    - [11.6 Records](#116-records)
    - [11.7 Enums](#117-enums)
    - [11.8 Union Types](#118-union-types)
  - [12. Behavioral Types](#12-behavioral-types)
    - [12.1 Modal Types](#121-modal-types)
    - [12.2 String and Bytes Types](#122-string-and-bytes-types)
    - [12.3 Pointer Types](#123-pointer-types)
    - [12.4 Function Types](#124-function-types)
  - [13. Abstraction and Polymorphism](#13-abstraction-and-polymorphism)
    - [13.1 Generics](#131-generics)
    - [13.2 Classes (Forms)](#132-classes-forms)
    - [13.3 Class Implementation](#133-class-implementation)
    - [13.4 Class Constraints](#134-class-constraints)
    - [13.5 Dynamic Polymorphism (`$`)](#135-dynamic-polymorphism-)
    - [13.6 Opaque Polymorphism (`opaque`)](#136-opaque-polymorphism-opaque)
    - [13.7 Refinement Types](#137-refinement-types)
    - [13.8 Capability Classes](#138-capability-classes)
    - [13.9 Foundational Classes](#139-foundational-classes)
- [Part V: Executable Semantics](#part-v-executable-semantics)
  - [14. Procedures and Contracts](#14-procedures-and-contracts)
    - [14.1 Declarations](#141-declarations)
    - [14.2 Methods and Receivers](#142-methods-and-receivers)
    - [14.3 Overloading](#143-overloading)
    - [14.4 Contract Syntax](#144-contract-syntax)
    - [14.5 Pre/Postconditions](#145-prepostconditions)
    - [14.6 Invariants](#146-invariants)
    - [14.7 Verification Logic](#147-verification-logic)
    - [14.8 Behavioral Subtyping](#148-behavioral-subtyping)
  - [15. Expressions](#15-expressions)
    - [15.1 Evaluation Order](#151-evaluation-order)
    - [15.2 Literals and Identifiers](#152-literals-and-identifiers)
    - [15.3 Access Expressions](#153-access-expressions)
    - [15.4 Calls and Operators](#154-calls-and-operators)
    - [15.5 Casts](#155-casts)
    - [15.6 Closures](#156-closures)
    - [15.7 Pattern Matching](#157-pattern-matching)
    - [15.8 Conditionals](#158-conditionals)
    - [15.9 Loops](#159-loops)
  - [16. Statements](#16-statements)
    - [16.1 Blocks](#161-blocks)
    - [16.2 Declarations](#162-declarations)
    - [16.3 Assignment](#163-assignment)
    - [16.4 Expression Statements](#164-expression-statements)
    - [16.5 Control Flow Statements](#165-control-flow-statements)
    - [16.6 Defer](#166-defer)
    - [16.7 Regions](#167-regions)
    - [16.8 Panic \& Unwinding](#168-panic--unwinding)
  - [17. The Key System](#17-the-key-system)
    - [17.1 Fundamentals and Modes](#171-fundamentals-and-modes)
    - [17.2 Acquisition and `#` Blocks](#172-acquisition-and--blocks)
    - [17.3 Conflict Detection](#173-conflict-detection)
    - [17.4 Nested Release](#174-nested-release)
    - [17.5 Speculative Execution](#175-speculative-execution)
    - [17.6 Runtime Realization (`[[dynamic]]`)](#176-runtime-realization-dynamic)
    - [17.7 Memory Ordering](#177-memory-ordering)
- [Part VI: Concurrency](#part-vi-concurrency)
  - [18. Structured Parallelism](#18-structured-parallelism)
    - [18.1 Parallel Blocks](#181-parallel-blocks)
    - [18.2 Execution Domains](#182-execution-domains)
    - [18.3 Capture Semantics](#183-capture-semantics)
    - [18.4 Tasks and Spawning](#184-tasks-and-spawning)
    - [18.5 Data Parallelism (`dispatch`)](#185-data-parallelism-dispatch)
    - [18.6 Cancellation](#186-cancellation)
    - [18.7 Panic Handling in Parallel](#187-panic-handling-in-parallel)
    - [18.8 Determinism and Nesting](#188-determinism-and-nesting)
  - [19. Asynchronous Operations](#19-asynchronous-operations)
    - [19.1 Async Model](#191-async-model)
    - [19.2 Suspension](#192-suspension)
    - [19.3 Consumption and Composition](#193-consumption-and-composition)
    - [19.4 Async-Key Integration](#194-async-key-integration)
- [Part VII: Metaprogramming and Interop](#part-vii-metaprogramming-and-interop)
  - [20. Metaprogramming](#20-metaprogramming)
    - [20.1 Comptime Environment](#201-comptime-environment)
    - [20.2 Capabilities](#202-capabilities)
    - [20.3 Reflection](#203-reflection)
    - [20.4 Code Generation](#204-code-generation)
    - [20.5 Derivation](#205-derivation)
  - [21. Foreign Function Interface](#21-foreign-function-interface)
    - [21.1 FfiSafe](#211-ffisafe)
    - [21.2 Externs and Exports](#212-externs-and-exports)
    - [21.3 Capability Isolation](#213-capability-isolation)
    - [21.4 Foreign Contracts](#214-foreign-contracts)
  - [22. ABI and Linkage](#22-abi-and-linkage)
    - [22.1 Calling Conventions](#221-calling-conventions)
    - [22.2 Layout Rules and Alignment](#222-layout-rules-and-alignment)
    - [22.3 Linkage and Symbol Visibility](#223-linkage-and-symbol-visibility)
- [Part VIII: Appendices](#part-viii-appendices)
  - [Appendix A: Diagnostic Code Registry](#appendix-a-diagnostic-code-registry)
    - [A.1 Error Codes](#a1-error-codes)
    - [A.2 Warning Codes](#a2-warning-codes)
  - [Appendix B: Complete Grammar](#appendix-b-complete-grammar)
    - [B.1 Lexical Grammar](#b1-lexical-grammar)
    - [B.2 Type Grammar](#b2-type-grammar)
    - [B.3 Expression Grammar](#b3-expression-grammar)
    - [B.4 Pattern Grammar](#b4-pattern-grammar)
    - [B.5 Statement Grammar](#b5-statement-grammar)
    - [B.6 Declaration Grammar](#b6-declaration-grammar)
    - [B.7 Contract Grammar](#b7-contract-grammar)
    - [B.8 Attribute Grammar](#b8-attribute-grammar)
    - [B.9 Key System Grammar](#b9-key-system-grammar)
    - [B.10 Concurrency Grammar](#b10-concurrency-grammar)
    - [B.11 Async Grammar](#b11-async-grammar)
    - [B.12 Metaprogramming Grammar](#b12-metaprogramming-grammar)
    - [B.13 FFI Grammar](#b13-ffi-grammar)
    - [B.14 Region Grammar](#b14-region-grammar)
  - [Appendix C: Type Layout Reference](#appendix-c-type-layout-reference)
    - [C.1 Primitive Type Layouts](#c1-primitive-type-layouts)
    - [C.2 Compound Type Layout Rules](#c2-compound-type-layout-rules)
    - [C.3 Pointer Type Layouts](#c3-pointer-type-layouts)
    - [C.4 String and Bytes Type Layouts](#c4-string-and-bytes-type-layouts)
    - [C.5 Modal Type Layouts](#c5-modal-type-layouts)
  - [Appendix D: Standard Form Catalog](#appendix-d-standard-form-catalog)
    - [D.1 Foundational Classes](#d1-foundational-classes)
    - [D.2 Comparison Classes](#d2-comparison-classes)
    - [D.3 Capability Classes](#d3-capability-classes)
    - [D.4 Context Record](#d4-context-record)

---

# Part I: General and Lexical

## 1. Introduction

---

### 1.1 Scope and Conformance




#### 1.1.2 Program Requirements

**Conforming program.** A conforming program is a Cursive source program that satisfies all normative requirements of this specification applicable to programs.

**Static Semantics**

A conforming program satisfies:

1. All lexical constraints defined in §2.
2. All syntactic constraints defined in §2.
3. All static-semantic constraints defined throughout this specification.


#### 1.1.3 Well-Formed Programs

**Well-formed program.** A well-formed program is a conforming program that satisfies all lexical, syntactic, and static-semantic constraints.

**Constraints**

**Diagnostics:** See Appendix A, code `E-CNF-0101`.


### 1.2 Glossary and Notation

#### 1.2.1 Glossary of Terms

**Glossary.** The following terms have precise meanings in this specification:

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


#### 1.2.2 Typing Judgment Notation

**Typing notation.** Typing rules use the following notation:

$$\Gamma \vdash e : T \dashv \Gamma'$$

where:
- $\Gamma$ (input context): The typing context before evaluating $e$, containing bindings and their states.
- $\vdash$ (turnstile): Separates the context from the judgment; read as "entails" or "proves."
- $e : T$: The expression $e$ has type $T$.
- $\dashv$ (reverse turnstile): Separates the judgment from the output context; indicates "producing."
- $\Gamma'$ (output context): The typing context after evaluating $e$, reflecting any state changes (e.g., moved bindings).

When only the input context matters (no state change), the simpler form $\Gamma \vdash e : T$ is used.


#### 1.2.3 Inference Rule Format

**Inference rules.** Inference rules are written as:

$$\frac{\text{premise}_1 \quad \text{premise}_2 \quad \ldots}{\text{conclusion}}$$

The rule states: if all premises above the line hold, then the conclusion below the line holds.


### 1.3 Implementation Limits

This specification defines no implementation limits.


### 1.4 Behavior Classifications

**Behavior classifications.** All well-formed program behaviors partition into four mutually exclusive categories based on how this specification constrains outcomes and what guarantees implementations provide. Ill-formed programs are classified separately.


#### 1.4.1 Defined Behavior

**Defined Behavior.** Program behavior for which this specification prescribes exactly one permitted outcome. Conforming implementations MUST produce the defined outcome for all well-formed programs exercising such behavior.



#### 1.4.2 Implementation-Defined Behavior (IDB)

**IDB.** This specification permits multiple valid outcomes and requires the implementation to document which outcome it chooses. The implementation MUST:
1. Select exactly one outcome from the permitted set.
2. Maintain consistency across all compilations with the same configuration.

Implementations MUST document the selected outcome for each instance of Implementation-Defined Behavior. This documentation MUST be publicly available and MUST be consistent across all programs compiled under the same implementation version and target configuration.

Programs relying on a specific IDB choice are conforming but non-portable.


#### 1.4.3 Unspecified Behavior (USB)

**USB.** This specification permits a set of valid outcomes but does not require the implementation to document which outcome is chosen. The choice MAY vary between executions, compilations, or within a single execution. USB in safe code MUST NOT introduce memory unsafety or data races.


#### 1.4.4 Unverifiable Behavior (UVB)

**UVB.** Operations whose runtime correctness depends on properties external to the language's semantic model. UVB is permitted only within `unsafe` blocks and FFI calls. An `unsafe` block assertion transfers responsibility for maintaining external invariants to the programmer.

**Constraints**

**Safety Invariant**

Safe code cannot exhibit Unverifiable Behavior. For any well-formed program where all expressions are outside of `unsafe` blocks, the program MUST NOT exhibit any Unverifiable Behavior. This invariant is enforced by the type system (§9) and the permission system (§10).


---

## 2. Lexical Structure

---

### 2.1 Source Text & Encoding


#### 2.1.1 UTF-8 Mandate

**Source Text.** A sequence of Unicode scalar values (U+0000–U+D7FF ∪ U+E000–U+10FFFF) that constitutes the input to the Cursive translation pipeline.

**Static Semantics**

Cursive source input MUST be encoded as UTF-8 (RFC 3629). Invalid byte sequences are ill-formed.

**Diagnostics:** See Appendix A, code `E-SRC-0101`.


#### 2.1.2 BOM Handling

**BOM.** The Unicode code point U+FEFF when appearing at the start of a byte stream.

**Static Semantics**

1. If a source file begins with the UTF-8 BOM (U+FEFF) as the first decoded scalar value, the implementation MUST strip the BOM before lexical analysis.
2. A BOM appearing at any position after the first is ill-formed.
3. A conforming implementation SHOULD emit a warning when a UTF-8 BOM is present at the start of a file.

**Diagnostics:** See Appendix A, codes `E-SRC-0103`, `W-SRC-0101`.


#### 2.1.3 Unicode Normalization

**Unicode Normalization.** The process of converting Unicode text to a canonical form for comparison.

**Static Semantics**

1. Unicode normalization of source text outside identifiers and module-path components is Implementation-Defined Behavior (IDB).
2. For identifier lexemes and module-path components, implementations MUST normalize to Unicode Normalization Form C (NFC) prior to equality comparison, hashing, or name lookup.
3. Normalization MUST NOT change logical line boundaries or byte offsets used for diagnostic locations.


#### 2.1.4 Prohibited Code Points

**Prohibited Code Points.** Unicode code points that MUST NOT appear in source text outside of string and character literals.

**Static Semantics**

The prohibited code points are all characters with General_Category=Cc (control characters) except:
- U+0009 (Horizontal Tab)
- U+000A (Line Feed)
- U+000C (Form Feed)
- U+000D (Carriage Return)

**Diagnostics:** See Appendix A, code `E-SRC-0104`.


### 2.2 Line Structure and Termination


#### 2.2.1 Logical Line Definition

**Logical Line.** A sequence of Unicode scalar values terminated by a line feed character (U+000A) or end-of-file.

**Syntax**

```ebnf
source_file     ::= normalized_line*
normalized_line ::= code_point* line_terminator?
line_terminator ::= "\n"
code_point      ::= /* any Unicode scalar except U+000A and prohibited code points (§2.1.4) */
```

**Diagnostics:** None.


#### 2.2.2 Line Ending Normalization

**Line Ending Normalization.** The transformation that converts all platform-specific line ending sequences to a single canonical line feed character.

**Static Semantics**

Implementations MUST normalize line endings before tokenization:

1. Replace each CR LF (U+000D U+000A) with a single LF (U+000A).
2. Replace each standalone CR (U+000D) not followed by LF with a single LF (U+000A).
3. Each LF (U+000A) that does not follow a CR remains unchanged.

Mixed line endings are permitted within a single source file.


#### 2.2.3 Statement Termination Rules

**Statement Terminator.** Marks the end of a statement. Cursive supports explicit termination (`;`) and implicit termination (`<newline>`).

**Static Semantics**

**Termination Predicate**

A token terminates a statement when:
- The token is `;`, OR
- The token is `<newline>` AND none of the continuation conditions apply.

**Line Continuation Conditions**

A `<newline>` does NOT terminate a statement when any of the following hold:

| Condition            | Description                                                              |
| :------------------- | :----------------------------------------------------------------------- |
| Open Delimiter       | The newline appears inside an unclosed `()`, `[]`, or `{}`.              |
| Trailing Operator    | The last non-comment token on the line is a binary operator or `,`.      |
| Leading Continuation | The first non-comment token on the following line is `.`, `::`, or `~>`. |

**Disambiguation**

Tokens `{+, -, *, &, |}` may function as binary or unary operators. When such a token appears at line end and the next line begins with an operand, the token is interpreted as binary (triggering continuation). To force termination before a unary operator, insert an explicit semicolon.

**Diagnostics:** See Appendix A, code `E-SRC-0510`.


### 2.3 Comments and Whitespace


#### 2.3.1 Line Comments

**Line Comment.** A lexical construct beginning with `//` and extending to the next line feed or end-of-file.

**Static Semantics**

1. A line comment begins with `//` outside a string or character literal.
2. The comment extends to but does not include the next U+000A or end-of-file.
3. Line comments do not contribute to the token stream.

**Documentation Comments**

Comments beginning with `///` or `//!` are documentation comments:
- `///` associates with the following declaration.
- `//!` associates with the enclosing module.

Implementations MUST preserve documentation comments for extraction. Documentation comments MUST NOT appear in the token stream.


#### 2.3.2 Block Comments

**Block Comment.** A lexical construct delimited by `/*` and `*/`.

**Static Semantics**

1. A block comment begins with `/*` outside a string or character literal.
2. Block comments MUST nest: each `/*` increases nesting depth, each `*/` decreases it.
3. The comment terminates when nesting depth returns to zero.
4. Block comments do not contribute to the token stream.

**Diagnostics:** See Appendix A, code `E-SRC-0306`.


#### 2.3.3 Nested Comment Semantics

**Nested Comments.** Block comments nest properly, allowing `/*` and `*/` pairs within block comments.

**Static Semantics**

End-of-file with non-zero nesting depth is ill-formed. Characters within comments MUST NOT contribute to token formation, delimiter nesting, or statement termination.


#### 2.3.4 Whitespace Handling

**Whitespace.** Code points that serve as token separators but do not produce tokens (except newline).

**Static Semantics**

| Character      | Code Point | Treatment                    |
| :------------- | :--------- | :--------------------------- |
| Space          | U+0020     | Token separator; not emitted |
| Horizontal Tab | U+0009     | Token separator; not emitted |
| Form Feed      | U+000C     | Token separator; not emitted |
| Line Feed      | U+000A     | Emitted as `<newline>` token |


### 2.4 Tokens and Vocabulary


#### 2.4.1 Token Definition

**Token.** The atomic unit of lexical analysis, representing a classified fragment of source text.

**Static Semantics**

A token is a triple (kind, lexeme, span) where:
- **kind** is one of the token kinds defined in §2.5.2.
- **lexeme** is the character sequence from source text.
- **span** is the source location (file, start position, end position).


#### 2.4.2 Token Kinds

**Token Kinds.** The categories into which all tokens are classified.

**Static Semantics**

| Kind         | Description                               |
| :----------- | :---------------------------------------- |
| `identifier` | User-defined names (§2.5.1)               |
| `keyword`    | Reserved words (§2.5.2)                   |
| `literal`    | Compile-time constant values (§2.6)       |
| `operator`   | Operation symbols (§2.7.1–§2.7.6)         |
| `punctuator` | Structural delimiters (§2.7.7)            |
| `newline`    | Line feed token for statement termination |


#### 2.4.3 Deterministic Tokenization

**Deterministic Tokenization.** The property that tokenization produces identical output for identical input.

**Static Semantics**

1. Tokenization MUST be deterministic for a given normalized source file.
2. Every non-comment, non-whitespace character sequence corresponds to exactly one token.
3. The **Maximal Munch Rule** applies: when multiple tokenizations are possible, the longest valid token takes precedence.

**Generic Argument Exception**

When `>>` appears in a context closing two or more nested generic parameter lists, `>>` is treated as two separate `>` tokens. This exception applies recursively: `>>>` becomes three `>` tokens when closing three nested generic parameter lists. The original lexeme span is preserved for diagnostics.

**Diagnostics:** See Appendix A, code `E-SRC-0309`.


### 2.5 Identifiers and Keywords


#### 2.5.1 Identifier Syntax (UAX #31)

**Identifier.** A sequence of Unicode scalar values that names a declaration, binding, type, or other program entity, following Unicode Standard Annex #31 (UAX #31).

**Syntax**

```ebnf
identifier     ::= ident_start ident_continue*
ident_start    ::= /* Unicode XID_Start */ | "_"
ident_continue ::= /* Unicode XID_Continue */ | "_"
```

**Static Semantics**

1. Identifiers MUST NOT contain prohibited code points (§2.1.4), surrogate code points (U+D800–U+DFFF), or non-characters.
2. Reserved keywords (§2.5.2) MUST NOT be treated as identifiers.
3. Two identifiers are the same binding name if and only if their NFC-normalized forms are identical.

**Diagnostics:** See Appendix A, code `E-SRC-0307`.


#### 2.5.2 Reserved Keywords Table

**Keyword.** A reserved lexeme with special meaning in the language grammar that MUST NOT be used where an identifier is expected.

**Static Semantics**

The reserved keywords are:

```
all         as          break       class       comptime    const
continue    defer       dispatch    else        emits       enum
extern      false       for         from        frame       if
imm         import      in          internal    let         loop
match       modal       move        mut         null        opaque
override    parallel    private     procedure   protected   public
quote       race        record      region      requires    result
return      self        shadow      shared      spawn       sync
transition  transmute   true        type        unique      unsafe
using       var         where       widen       yield
```

Implementations MUST tokenize these as `<keyword>`, not `<identifier>`. Reserved keywords are unconditionally reserved in all syntactic contexts. The keyword set MUST be identical across conforming implementations for a given language version.

In addition to reserved keywords, this specification uses **fixed identifier lexemes** in certain constrained syntactic contexts (for example: attribute names, named-argument keys like `name:`, mode/option words like `read`, `write`, `release`, `ordered`, `speculative`, `chunk`, and `reduce`, reduction operators like `min`, `max`, `and`, and `or`, and intrinsic member names like `emit`). These lexemes are tokenized as `<identifier>` and are **not** reserved; they remain legal identifiers outside their defining context.

The following words are reserved for future language versions but have no grammar productions in this specification version:

```
async       atomic      do          escape      interrupt   mod
module      pool        select      set         simd        then
union       volatile    while
```

**Diagnostics:** See Appendix A, code `E-CNF-0401`.


#### 2.5.3 Reserved Namespaces

**Reserved Namespace.** An identifier prefix reserved for use by this specification or by implementations.

**Static Semantics**

1. The `cursive.*` namespace prefix is reserved for specification-defined features. User programs and vendor extensions MUST NOT use this namespace.
2. The identifier prefix `gen_` is reserved for synthetic identifiers. User declarations MUST NOT define identifiers beginning with `gen_`.
3. Implementations MAY reserve additional identifier patterns (IDB).

**Diagnostics:** See Appendix A, codes `E-CNF-0402`, `E-CNF-0406`, `W-CNF-0401`.


#### 2.5.4 Universe-Protected Bindings

**Universe-Protected Binding.** An identifier pre-declared in the implicit universal scope that MUST NOT be shadowed by user declarations at module scope.

**Static Semantics**

The following identifiers are universe-protected:

**Primitive type names:**
`i8`, `i16`, `i32`, `i64`, `i128`, `u8`, `u16`, `u32`, `u64`, `u128`, `f16`, `f32`, `f64`, `bool`, `char`, `usize`, `isize`

**Special types:**
`Self`, `string`, `bytes`, `Modal`

**Foundational classes:**
`Drop`, `Bitcopy`, `Clone`

**Async type aliases:**
`Async`, `Future`, `Sequence`, `Stream`, `Pipe`, `Exchange`

Attempting to shadow a universe-protected binding at module scope is ill-formed.

**Diagnostics:** See Appendix A, codes `E-CNF-0403`, `E-CNF-0404`, `E-CNF-0405`.


### 2.6 Literals


#### 2.6.1 Integer Literals

**Integer Literal.** A token representing a compile-time integer constant value.

**Syntax**

```ebnf
integer_literal  ::= (decimal_integer | hex_integer
                   | octal_integer | binary_integer) int_suffix?
decimal_integer  ::= dec_digit ("_"* dec_digit)*
decimal_literal  ::= decimal_integer
hex_integer      ::= "0x" hex_digit ("_"* hex_digit)*
octal_integer    ::= "0o" oct_digit ("_"* oct_digit)*
binary_integer   ::= "0b" bin_digit ("_"* bin_digit)*

int_suffix       ::= "i8" | "i16" | "i32" | "i64" | "i128"
                   | "u8" | "u16" | "u32" | "u64" | "u128"
                   | "isize" | "usize"

dec_digit        ::= "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9"
hex_digit        ::= dec_digit
                  | "a" | "b" | "c" | "d" | "e" | "f"
                  | "A" | "B" | "C" | "D" | "E" | "F"
oct_digit        ::= "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7"
bin_digit        ::= "0" | "1"
```

**Static Semantics**

1. Numeric underscore placement is defined by `NumericUnderscoreOk` (see below).
2. Based integer literals MUST begin with the appropriate prefix (`0x`, `0o`, `0b`) followed by at least one valid digit.

**Numeric Scan (Maximal Prefix).**

Let $T$ be the sequence of Unicode scalar values of the source.

**String Operations.**
$$s = \langle s_0,\ldots,s_{|s|-1}\rangle$$
$$\text{At}(s,i) = s_i$$
$$\text{Concat}(s,t) = \langle s_0,\ldots,s_{|s|-1},\ t_0,\ldots,t_{|t|-1}\rangle$$
$$\text{StartsWith}(s,p) \iff \exists u.\ s = \text{Concat}(p,u)$$
$$\text{EndsWith}(s,p) \iff \exists u.\ s = \text{Concat}(u,p)$$

**Digits.**
$$\text{DecDigit}(c) \iff c \in \{\texttt{'0'} \ldots \texttt{'9'}\}$$
$$\text{HexDigit}(c) \iff \text{DecDigit}(c) \lor c \in \{\texttt{'a'}\ldots\texttt{'f'},\ \texttt{'A'}\ldots\texttt{'F'}\}$$
$$\text{OctDigit}(c) \iff c \in \{\texttt{'0'}\ldots\texttt{'7'}\}$$
$$\text{BinDigit}(c) \iff c \in \{\texttt{'0'},\ \texttt{'1'}\}$$

**Suffix Sets.**
$$\text{IntSuffixSet} = \{\texttt{"i8"},\ \texttt{"i16"},\ \texttt{"i32"},\ \texttt{"i64"},\ \texttt{"i128"},\ \texttt{"u8"},\ \texttt{"u16"},\ \texttt{"u32"},\ \texttt{"u64"},\ \texttt{"u128"},\ \texttt{"isize"},\ \texttt{"usize"}\}$$
$$\text{FloatSuffixSet} = \{\texttt{"f16"},\ \texttt{"f32"},\ \texttt{"f64"}\}$$
$$\text{NumSuffix} = \text{IntSuffixSet} \cup \text{FloatSuffixSet}$$

**Underscore Constraints.**
$$\text{BasePrefix} = \{\texttt{"0x"},\ \texttt{"0o"},\ \texttt{"0b"}\}$$
$$\text{StartsWithUnderscore}(s) \iff \text{At}(s,0)=\texttt{"\_"}$$
$$\text{EndsWithUnderscore}(s) \iff \text{At}(s,|s|-1)=\texttt{"\_"}$$
$$\text{AfterBasePrefixUnderscore}(s) \iff \exists p \in \text{BasePrefix}.\ \text{StartsWith}(s,\ \text{Concat}(p,\ \texttt{"\_"}))$$
$$\text{AdjacentExponentUnderscore}(s) \iff \exists i.\ \text{At}(s,i)=\texttt{"\_"} \land ((i>0 \land \text{At}(s,i-1)\in\{\texttt{"e"},\texttt{"E"}\}) \lor (i+1<|s| \land \text{At}(s,i+1)\in\{\texttt{"e"},\texttt{"E"}\}))$$
$$\text{BeforeSuffixUnderscore}(s) \iff \exists \text{suf} \in \text{NumSuffix}.\ \text{EndsWith}(s,\ \text{Concat}(\texttt{"\_"},\ \text{suf}))$$
$$\text{NumericUnderscoreOk}(s) \iff \neg \text{StartsWithUnderscore}(s) \land \neg \text{EndsWithUnderscore}(s) \land \neg \text{AfterBasePrefixUnderscore}(s) \land \neg \text{AdjacentExponentUnderscore}(s) \land \neg \text{BeforeSuffixUnderscore}(s)$$

$$\text{Lexeme}(T,i,j) = T[i..j)$$

**Scan End (Maximal Prefix).**
$$\text{DecRun}(T,i) = \max(\{i\} \cup \{ j \mid i < j \le |T| \land \forall k \in [i,j).\ (\text{DecDigit}(T[k]) \lor T[k]=\texttt{"_"}) \})$$
$$\text{HexRun}(T,i) = \max(\{i\} \cup \{ j \mid i < j \le |T| \land \forall k \in [i,j).\ (\text{HexDigit}(T[k]) \lor T[k]=\texttt{"_"}) \})$$
$$\text{OctRun}(T,i) = \max(\{i\} \cup \{ j \mid i < j \le |T| \land \forall k \in [i,j).\ (\text{OctDigit}(T[k]) \lor T[k]=\texttt{"_"}) \})$$
$$\text{BinRun}(T,i) = \max(\{i\} \cup \{ j \mid i < j \le |T| \land \forall k \in [i,j).\ (\text{BinDigit}(T[k]) \lor T[k]=\texttt{"_"}) \})$$

$$\text{SuffixMatch}(T,i,U) = \max(\{i\} \cup \{ j \mid i < j \le |T| \land \text{Lexeme}(T,i,j) \in U \})$$

$$\text{ExpSignEnd}(T,i) = \begin{cases}
 i+1 & i<|T| \land T[i] \in \{\texttt{"+"},\texttt{"-"}\} \\
 i & \text{otherwise}
\end{cases}$$

$$\text{ExpEnd}(T,i) = \begin{cases}
\text{DecRun}(T,\text{ExpSignEnd}(T,i+1)) & i<|T| \land T[i] \in \{\texttt{"e"},\texttt{"E"}\} \\
 i & \text{otherwise}
\end{cases}$$

$$\text{DecCoreEnd}(T,i) = \begin{cases}
\text{ExpEnd}(T,q) & p=\text{DecRun}(T,i) \land p<|T| \land T[p]=\texttt{"."} \land q=\text{DecRun}(T,p+1) \\
\text{ExpEnd}(T,p) & p=\text{DecRun}(T,i) \land (p \ge |T| \lor T[p] \ne \texttt{"."})
\end{cases}$$

$$\text{NumericCoreEnd}(T,i) = \begin{cases}
\text{HexRun}(T,i+2) & T[i..i+2]=\texttt{"0x"} \\
\text{OctRun}(T,i+2) & T[i..i+2]=\texttt{"0o"} \\
\text{BinRun}(T,i+2) & T[i..i+2]=\texttt{"0b"} \\
\text{DecCoreEnd}(T,i) & \text{otherwise}
\end{cases}$$

$$\text{NumericScanEnd}(T,i) = \text{SuffixMatch}(T,\text{NumericCoreEnd}(T,i),\text{NumSuffix})$$

**Numeric Classification.**
$$\text{HasDot}(T,i,j) \iff \exists p.\ i \le p < j \land T[p]=\texttt{"."}$$
$$\text{HasExp}(T,i,j) \iff \exists p.\ i \le p < j \land T[p] \in \{\texttt{"e"},\texttt{"E"}\}$$

$$\text{NumericKind}(T,i) = \begin{cases}
\text{FloatLiteral} & \text{SuffixMatch}(T,\text{NumericCoreEnd}(T,i),\text{FloatSuffixSet}) > \text{NumericCoreEnd}(T,i) \\
\text{FloatLiteral} & \text{HasDot}(T,i,\text{NumericCoreEnd}(T,i)) \lor \text{HasExp}(T,i,\text{NumericCoreEnd}(T,i)) \\
\text{IntLiteral} & \text{otherwise}
\end{cases}$$

$$\text{NumericLexemeOk}(T,i,j) \iff (\text{Lexeme}(T,i,j)\ \text{matches}\ \text{integer\_literal} \lor \text{Lexeme}(T,i,j)\ \text{matches}\ \text{float\_literal}) \land \text{NumericUnderscoreOk}(\text{Lexeme}(T,i,j))$$
$$\text{NumericLexemeBad}(T,i,j) \iff \neg \text{NumericLexemeOk}(T,i,j)$$

**Tokenization Rule.**

1. If $\text{DecDigit}(T[i])$ holds, the lexer MUST consume the maximal prefix $T[i..\text{NumericScanEnd}(T,i))$ as a numeric literal token.
2. The token kind MUST be floating-point iff $\text{NumericKind}(T,i)=\text{FloatLiteral}$; otherwise it MUST be an integer literal.
3. If $\text{NumericLexemeBad}(T,i,\text{NumericScanEnd}(T,i))$ holds, the lexer MUST emit the malformed numeric literal diagnostic for that lexeme and MUST continue tokenization after $\text{NumericScanEnd}(T,i)$ (it MUST NOT treat the sequence as an unclassifiable token at position $i$).

**Diagnostics:** See Appendix A, codes `E-SRC-0304`, `W-SRC-0301`.


#### 2.6.2 Floating-Point Literals

**Floating-Point Literal.** A token representing a compile-time floating-point constant value.

**Syntax**

```ebnf
float_literal ::= decimal_integer "." decimal_integer? exponent? suffix?
exponent      ::= ("e" | "E") ("+" | "-")? decimal_integer
suffix        ::= "f16" | "f32" | "f64"
```

**Static Semantics**

1. A floating-point literal MUST contain a decimal point.
2. Underscores are permitted with the same restrictions as integer literals.

For malformed numeric literal diagnostics (`E-SRC-0304`), see §2.6.1.


#### 2.6.3 String Literals

**String Literal.** A token representing a compile-time string constant value.

**Syntax**

```ebnf
string_literal   ::= '"' (string_char | escape_sequence)* '"'
string_char      ::= /* any Unicode scalar value except ", \, or U+000A */
escape_sequence  ::= "\n" | "\r" | "\t" | "\\" | "\"" | "\'" | "\0"
                   | "\x" hex_digit hex_digit
                   | "\u{" hex_digit+ "}"
```

**Static Semantics**

1. A string literal MUST begin with `"` and end with a matching unescaped `"`.
2. A line feed or end-of-file before the closing quote is ill-formed.
3. The `\x` escape MUST be followed by exactly two hexadecimal digits.
4. The `\u{...}` escape MUST contain 1–6 hexadecimal digits representing a valid Unicode scalar value.

**Diagnostics:** See Appendix A, codes `E-SRC-0301`, `E-SRC-0302`.


#### 2.6.4 Character Literals

**Character Literal.** A token representing a single Unicode scalar value.

**Syntax**

```ebnf
char_literal     ::= "'" (char_content | escape_sequence) "'"
char_content     ::= /* any Unicode scalar value except ', \, or U+000A */
```

**Static Semantics**

1. A character literal MUST begin with `'` and end with a matching unescaped `'`.
2. A character literal MUST represent exactly one Unicode scalar value.
3. Empty or multi-value character literals are ill-formed.

**Diagnostics:** See Appendix A, code `E-SRC-0303`.


#### 2.6.5 Boolean Literals

**Boolean Literal.** A token representing a compile-time boolean constant value.

**Syntax**

```ebnf
bool_literal ::= "true" | "false"
```

**Static Semantics**

The tokens `true` and `false` are boolean literals. They are also reserved keywords (§2.5.2).


#### 2.6.6 Null Literal

**Null Literal.** A token representing a null raw pointer constant value.

**Syntax**

```ebnf
null_literal ::= "null"
```

**Static Semantics**

1. The token `null` is a null literal. It is also a reserved keyword (§2.5.2).
2. The null literal denotes the null address (`0x0`) and is permitted only where an expected raw pointer type (`*imm T` or `*mut T`) is available.
3. The null literal MUST NOT be used to construct safe pointer values; use `Ptr::null()` (§12.3.3) for safe null pointers.


### 2.7 Operators and Punctuation


#### 2.7.1 Arithmetic Operators

**Arithmetic Operators.** Operators that perform numeric computations.

**Static Semantics**

| Operator | Name           | Arity  |
| :------- | :------------- | :----- |
| `+`      | Addition       | Binary |
| `-`      | Subtraction    | Binary |
| `*`      | Multiplication | Binary |
| `/`      | Division       | Binary |
| `%`      | Remainder      | Binary |
| `**`     | Exponentiation | Binary |
| `-`      | Negation       | Unary  |


#### 2.7.2 Comparison Operators

**Comparison Operators.** Operators that compare two values and produce a boolean result.

**Static Semantics**

| Operator | Name                  |
| :------- | :-------------------- |
| `==`     | Equal                 |
| `!=`     | Not equal             |
| `<`      | Less than             |
| `<=`     | Less than or equal    |
| `>`      | Greater than          |
| `>=`     | Greater than or equal |


#### 2.7.3 Logical Operators

**Logical Operators.** Operators that perform boolean logic with short-circuit evaluation.

**Static Semantics**

| Operator | Name        | Evaluation                                           |
| :------- | :---------- | :--------------------------------------------------- |
| `&&`     | Logical AND | Short-circuit; evaluates right only if left is true  |
| `\|\|`   | Logical OR  | Short-circuit; evaluates right only if left is false |
| `!`      | Logical NOT | Unary prefix                                         |


#### 2.7.4 Bitwise Operators

**Bitwise Operators.** Operators that perform bit-level operations on integer values.

**Static Semantics**

| Operator | Name        | Arity  |
| :------- | :---------- | :----- |
| `&`      | Bitwise AND | Binary |
| `\|`     | Bitwise OR  | Binary |
| `^`      | Bitwise XOR | Binary |
| `<<`     | Left shift  | Binary |
| `>>`     | Right shift | Binary |
| `!`      | Bitwise NOT | Unary  |


#### 2.7.5 Assignment Operators

**Assignment Operators.** Operators that assign or update values in place expressions.

**Static Semantics**

| Operator | Name               |
| :------- | :----------------- |
| `=`      | Simple assignment  |
| `+=`     | Add-assign         |
| `-=`     | Subtract-assign    |
| `*=`     | Multiply-assign    |
| `/=`     | Divide-assign      |
| `%=`     | Remainder-assign   |
| `&=`     | Bitwise AND-assign |
| `\|=`    | Bitwise OR-assign  |
| `^=`     | Bitwise XOR-assign |
| `<<=`    | Left shift-assign  |
| `>>=`    | Right shift-assign |

**Binding Operators (Declarations Only)**

| Token | Semantic Meaning                                                       |
| :---- | :--------------------------------------------------------------------- |
| `=`   | Establishes movable responsibility (ownership may transfer via `move`) |
| `:=`  | Establishes immovable responsibility (ownership cannot transfer)       |


#### 2.7.6 Special Operators

**Special Operators.** Operators with unique semantics not covered by other categories.

**Static Semantics**

| Operator | Name              | Description                                                                 |
| :------- | :---------------- | :-------------------------------------------------------------------------- |
| `..`     | Range (exclusive) | Creates a range excluding the upper bound                                   |
| `..=`    | Range (inclusive) | Creates a range including the upper bound                                   |
| `=>`     | Fat arrow         | Match arm separator; pipeline composition; contract pre/post separator      |
| `->`     | Thin arrow        | Return type annotation                                                      |
| `::`     | Path separator    | Module and type path navigation                                             |
| `<:`     | Class bound       | Class bounds and implements-clause separator                                |
| `~`      | Const receiver    | Method receiver with `const` permission                                     |
| `~>`     | Method dispatch   | Instance method dispatch operator                                           |
| `~!`     | Unique receiver   | Method receiver with `unique` permission                                    |
| `~%`     | Shared receiver   | Method receiver with `shared` permission                                    |
| `?`      | Propagation       | Error/union propagation                                                     |
| `#`      | Coarsening        | Key coarsening for synchronization                                          |
| `@`      | Modal state       | Modal state annotation                                                      |
| `^`      | Region allocation | Region allocation (explicit `r^e` or implicit `^e` inside a `region` block) |
| `&`      | Address-of        | Creates a pointer to a place expression                                     |
| `*`      | Dereference       | Dereferences a pointer                                                      |
| `$`      | Dynamic dispatch  | Dynamic dispatch operator                                                   |

**Contract Operator**

| Token | Semantic Meaning                                    |
| :---- | :-------------------------------------------------- |
| `\|=` | Introduces procedure contract clause (precondition) |


#### 2.7.7 Punctuators

**Punctuators.** Symbols used for syntactic structure and grouping.

**Static Semantics**

| Token | Syntactic Role                                                               |
| :---- | :--------------------------------------------------------------------------- |
| `(`   | Open parenthesis (grouping, function calls, tuple construction)              |
| `)`   | Close parenthesis                                                            |
| `[`   | Open bracket (array literals, indexing, slicing)                             |
| `]`   | Close bracket                                                                |
| `{`   | Open brace (blocks, record literals, match arms)                             |
| `}`   | Close brace                                                                  |
| `,`   | Element separator (tuples, arrays, arguments, generic bounds)                |
| `:`   | Type annotation; label prefix; record field separator                        |
| `;`   | Statement terminator; generic parameter separator; where predicate separator |
| `.`   | Field access; tuple element access                                           |


### 2.8 Lexical Security


#### 2.8.1 Confusable Character Detection

**Lexically Sensitive Characters.** Unicode code points that may alter the visual appearance of source code without changing its tokenization.

**Static Semantics**

The sensitive character set includes:

| Category              | Code Points                 |
| :-------------------- | :-------------------------- |
| Bidirectional Control | U+202A–U+202E               |
| Directional Isolates  | U+2066–U+2069               |
| Zero-Width Characters | U+200C (ZWNJ), U+200D (ZWJ) |

When a character from this set appears outside string or character literals and comments, the implementation MUST emit a diagnostic. If the occurrence is within an `unsafe` block (§3.7), the diagnostic MUST be a warning and the source is accepted. Otherwise, the diagnostic MUST be an error and the program is ill-formed.

**Diagnostics:** See Appendix A, codes `W-SRC-0308`, `E-SRC-0308`.


#### 2.8.2 Homoglyph Mitigation

**Homoglyphs.** Visually similar characters from different Unicode blocks that could be confused with one another.

**Static Semantics**

1. Lexically sensitive characters within string or character literals or comments MUST NOT affect well-formedness (see §2.8.1).
2. Implementations MAY provide additional homoglyph detection for identifiers as a quality-of-implementation feature.
3. The Unicode Confusable mappings (Unicode TR39) MAY be used for detection.


### 2.9 Translation Phases


#### 2.9.1 Phase 1: Parsing

**Parsing.** The first translation phase that transforms source text into an Abstract Syntax Tree (AST).

**Static Semantics**

1. Phase 1 MUST complete for all source files in a compilation unit before Phase 2 begins.
2. All declarations are recorded in the AST.
3. Forward references to declarations within the same compilation unit MUST be permitted.


#### 2.9.2 Phase 2: Compile-Time Execution

**Compile-Time Execution.** The phase where `comptime` blocks are evaluated.

**Static Semantics**

1. `comptime` blocks are executed in dependency order.
2. Compile-time execution MAY generate additional declarations.
3. Generated declarations are added to the AST before Phase 3.


#### 2.9.3 Phase 3: Type Checking

**Type Checking.** The phase that performs semantic validation of the AST.

**Static Semantics**

Type checking includes:
1. Type inference and unification
2. Permission system validation
3. Contract verification
4. Move and responsibility analysis

For name resolution diagnostics, see §6.2.


#### 2.9.4 Phase 4: Code Generation

**Code Generation.** The final phase that transforms the validated AST into executable code.

**Static Semantics**

1. The AST is transformed into an intermediate representation.
2. The intermediate representation is translated to machine code or bytecode.
3. Cursive provides no C-style textual inclusion (`#include`) or preprocessing (`#define`).

---

# Part II: The Abstract Machine

## 3. The Object and Memory Model

---

### 3.1 Objects and Storage Duration


#### 3.1.1 Storage Classes

**Object.** The fundamental unit of typed storage in Cursive, defined by a quadruple:

$$\text{Object} ::= (\text{Storage}, \text{Type}, \text{Lifetime}, \text{Value})$$

**Storage Duration Classes**

| Duration Class | Description                                           | Provenance Tag           |
| :------------- | :---------------------------------------------------- | :----------------------- |
| **Static**     | Storage persists for entire program execution         | $\pi_{\text{Global}}$    |
| **Automatic**  | Storage allocated on scope entry, deallocated on exit | $\pi_{\text{Stack}}(S)$  |
| **Region**     | Storage allocated within a named region block (§16.7) | $\pi_{\text{Region}}(R)$ |
| **Dynamic**    | Storage allocated via heap allocator capability       | $\pi_{\text{Heap}}$      |

**Static Semantics**

Conforming programs MUST NOT rely on implicit heap allocation; safe heap allocation occurs only through explicit `$HeapAllocator` usage or explicit heap escape (§3.2.7).


#### 3.1.2 Object Lifecycle and Heap Semantics

**Object Lifecycle.** Every object passes through exactly three phases:

1. **Allocation and Initialization**: Storage is reserved and a valid initial value is written.
2. **Usage**: The object's value may be read or modified via valid bindings or pointers, subject to permission constraints (§10).
3. **Destruction and Deallocation**: If the object's type implements `Drop` (§13.9.1), its destructor is invoked. Storage is then released.

**Heap-Provenanced Values**

A **heap‑provenanced value** is a value of base type `T` whose storage duration is Dynamic and whose provenance tag is $\pi_{\text{Heap}}$. Heap‑provenanced values arise only through explicit heap escape (`to_heap`, §3.2.7) or through built‑in/core‑library operations that require a `$HeapAllocator` capability.

**Static Semantics**

1. Heap‑provenanced values are not distinguished by type from non‑heap values; provenance is compile‑time metadata.
2. Any operation that allocates heap backing MUST require an explicit `$HeapAllocator` capability parameter.
3. Safe code MUST NOT directly deallocate heap storage except via responsibility end.
4. Non‑owning address values (`Ptr<T>`, raw pointers) do not carry cleanup responsibility and cannot be used to deallocate heap storage in safe code.

**Dynamic Semantics**

The runtime associates each heap‑provenanced value with an allocator lineage derived from the `$HeapAllocator` argument used at allocation time. This association is not observable by programs and does not affect the value's layout.


### 3.2 Provenance and Lifetimes


#### 3.2.1 Provenance Model

**Provenance Tag.** A compile-time annotation ($\pi$) attached to every pointer and reference type, indicating the origin of the address.

$$\pi ::= \pi_{\text{Global}} \mid \pi_{\text{Stack}}(S) \mid \pi_{\text{Heap}} \mid \pi_{\text{Region}}(R) \mid \bot$$

**Provenance Categories**

| Provenance               | Meaning                                      |
| :----------------------- | :------------------------------------------- |
| $\pi_{\text{Global}}$    | Addresses of static storage                  |
| $\pi_{\text{Stack}}(S)$  | Addresses of automatic storage in scope $S$  |
| $\pi_{\text{Heap}}$      | Addresses of dynamically allocated storage   |
| $\pi_{\text{Region}}(R)$ | Addresses of storage allocated in region $R$ |
| $\bot$                   | No provenance (literals and constants)       |

Values with $\bot$ provenance have unbounded lifetime and can be stored anywhere.

**Lifetime Partial Order**

Provenance tags form a partial order ($\le$) based on containment:

$$\pi_{\text{Region}}(\text{Inner}) < \pi_{\text{Region}}(\text{Outer}) < \pi_{\text{Stack}}(S) < \pi_{\text{Heap}} \le \pi_{\text{Global}} \le \bot$$

The relation $\pi_A \le \pi_B$ holds iff storage with provenance $\pi_A$ is guaranteed to be deallocated no later than storage with provenance $\pi_B$.

| Relation           | Meaning                                                                 |
| :----------------- | :---------------------------------------------------------------------- |
| $<$ (strict)       | Left provenance is guaranteed to be deallocated before right provenance |
| $\le$ (non-strict) | Provenances may have equivalent lifetimes                               |

**Provenance Inference Rules**

- **Literals**: Have no provenance ($\bot$).
- **Static bindings**: Have global provenance ($\pi_{\text{Global}}$).
- **Local bindings**: Inherit provenance from their initializer. If the initializer has no provenance, the binding has stack provenance for the current scope ($\pi_{\text{Stack}}(S)$).
- **Region Allocation**: Values allocated in a region `r` have provenance $\pi_{\text{Region}}(r)$.
- **Field/Index Access**: Inherit the provenance of the parent object.
- **Dereference**: Has the provenance of the target memory location, not the pointer itself.

**Constraints**

Heap-allocated storage whose address has been stored in a binding with global provenance MUST NOT be deallocated during program execution. Deallocating such storage constitutes Unverifiable Behavior (UVB).


#### 3.2.2 Escape Rules and Heap Escape

**Escape Rule**

A value with provenance $\pi_e$ MUST NOT be assigned to a location with provenance $\pi_x$ if $\pi_e < \pi_x$.

**Static Semantics**

$$\frac{\Gamma \vdash e : T, \pi_e \quad \Gamma \vdash x : T, \pi_x \quad \pi_e < \pi_x}{\text{ill-formed}: \texttt{E-MEM-3020}}$$

**Diagnostics:** See Appendix A, code `E-MEM-3020`.

**Heap Escape (`to_heap`)**

`to_heap` is an explicit conversion that allocates heap storage for a value and returns the same base type `T` with heap provenance $\pi_{\text{Heap}}$. The implementation captures allocator metadata out‑of‑line using the provided `$HeapAllocator`. `to_heap` is the only safe mechanism by which region‑ or stack‑provenanced values may outlive their defining scope. The operation **transfers cleanup responsibility** from the source to the returned heap‑provenanced value.

**Syntax**

```cursive
procedure to_heap(move self: T, heap: $HeapAllocator) -> T | AllocationError
```

**(T-To-Heap)**
$$\frac{\Gamma \vdash e : T, \pi_e \quad \pi_e \neq \pi_{\text{Heap}} \quad \Gamma \vdash heap : \$HeapAllocator}{\Gamma \vdash e \mathord{\sim>} \texttt{to\_heap}(heap) : T, \pi_{\text{Heap}}}$$

1. The argument `heap` MUST be a capability value of type `$HeapAllocator` (possibly attenuated).
2. The operation is permitted in safe code.
3. If the source value has provenance $\pi_{\text{Region}}(R)$ or $\pi_{\text{Stack}}(S)$, the resulting value has provenance $\pi_{\text{Heap}}$.
4. The receiver is a transferring parameter; the call site MUST use a move expression per §3.4.5. After a successful call, the source binding is in Moved state.

**Dynamic Semantics**

1. Evaluate `e` to value `v`.
2. Allocate heap storage using `heap`, and initialize it with `v`.
3. Return a heap‑provenanced value `v_heap` of type `T` referring to the new allocation.
4. If allocation fails, return `AllocationError`. The receiver value is destroyed during unwinding of the failed call.


### 3.3 The Binding Model


#### 3.3.1 Binding Definition

**Binding.** A named association between an identifier and an object. Bindings are introduced by `let` and `var` declarations, pattern matching, and procedure parameters.


#### 3.3.2 Responsible Binding

**Responsible Binding.** A binding that manages the lifecycle of its associated object. When a responsible binding goes out of scope, it triggers destruction of the object (§3.5).


#### 3.3.3 Movable vs. Immovable Bindings

**Movable Binding.** A binding established with the `=` operator. Responsibility for the bound object can be transferred via `move`.

**Immovable Binding.** A binding established with the `:=` operator. Responsibility for the bound object CANNOT be transferred. Attempting to `move` from an immovable binding is a compile-time error.


#### 3.3.4 Binding States (Uninitialized, Valid, Moved, PartiallyMoved)

**Binding State.** The validity status of a binding:

$$\text{BindingState} ::= \text{Uninitialized} \mid \text{Valid} \mid \text{Moved} \mid \text{PartiallyMoved}$$

| State          | Meaning                                                      |
| :------------- | :----------------------------------------------------------- |
| Uninitialized  | No value has been assigned                                   |
| Valid          | Binding holds an initialized, accessible value               |
| Moved          | Binding's value has been moved; access is prohibited         |
| PartiallyMoved | Some fields have been moved; binding is partially accessible |

A **Temporary Value** is an object resulting from expression evaluation that is not immediately bound to a named identifier.


#### 3.3.5 Binding Mutability (`let` vs. `var`)

**Binding Mutability.** `let` establishes an immutable binding; `var` establishes a mutable binding. Binding mutability is orthogonal to data permission (§10) and responsibility transferability.


#### 3.3.6 Binding Operators (`=` vs. `:=`)

**Syntax**

```ebnf
binding_decl     ::= let_decl | var_decl | shadowed_binding
binding_op       ::= "=" | ":="
let_decl         ::= "let" pattern (":" type)? binding_op expression
var_decl         ::= "var" pattern (":" type)? binding_op expression
```

**Static Semantics**

**Binding Movability**

| Declaration  | Reassignable | Movable | Responsibility                      |
| :----------- | :----------- | :------ | :---------------------------------- |
| `let x = v`  | No           | Yes     | Transferable via `move`             |
| `let x := v` | No           | No      | Permanently fixed to `x`            |
| `var x = v`  | Yes          | Yes     | Transferable via `move`             |
| `var x := v` | Yes          | No      | Fixed; reassignment drops old value |

For `var x := v`: reassignment is permitted but `move` is not. Each reassignment drops the previous value and fixes the new value's responsibility to `x`.

**Binding Annotation Notation**

$$T^{\text{State}}_{\text{properties}}$$

| Component                  | Meaning                                              |
| :------------------------- | :--------------------------------------------------- |
| $^{\text{Valid}}$          | Binding holds an initialized, accessible value       |
| $^{\text{Moved}}$          | Binding's value has been moved; access is prohibited |
| $^{\text{PartiallyMoved}}$ | Some fields have been moved                          |
| $_{\text{mov}}$            | Binding is movable (established with `=`)            |
| $_{\text{immov}}$          | Binding is immovable (established with `:=`)         |
| $_{\text{mut}}$            | Binding is mutable (declared with `var`)             |

**Typing Rules**

*   `let x = e`: Defines an immutable, movable binding.
*   `let x := e`: Defines an immutable, immovable binding.
*   `var x = e`: Defines a mutable, movable binding.
*   `var x := e`: Defines a mutable, immovable binding.



#### 3.3.7 State Tracking and Control Flow Merge

**Static Semantics**

**State Tracking**

Binding state MUST be tracked through control flow. A binding's state is determined by its declaration, any `move` expressions consuming the binding, partial moves consuming fields, and reassignment of `var` bindings.

**Control Flow Merge**

At control flow merge points (after `if`/`else`, `match` arms, loop exits), binding state is the least-valid state from any branch:

$$\text{Valid} > \text{PartiallyMoved} > \text{Moved}$$

If a binding is Valid in one branch and Moved in another, the merged state is Moved.

**Temporary Lifetime**

Temporaries have a lifetime extending from their creation to the end of the innermost enclosing statement. If a temporary is used to initialize a `let` or `var` binding, its lifetime is promoted to match the binding's scope. Temporaries that are not moved or bound MUST be destroyed at statement end, in reverse order of creation.


### 3.4 Responsibility and Moves


#### 3.4.1 Move Definition

**Move.** An operation that transfers responsibility for an object from one binding to another. After a move, the source binding is invalidated and the destination binding becomes responsible for the object's lifecycle.

A move operation is only permitted from movable bindings (§3.3.3). Attempting to move from an immovable binding MUST be rejected with diagnostic `E-MEM-3006`.

$$\text{move} : (\Gamma, x : T^{\text{Valid}}_{\text{mov}}) \to (\Gamma[x \mapsto T^{\text{Moved}}], T)$$

The move operation takes a context $\Gamma$ containing a valid, movable binding $x$ of type $T$, and produces:
1. An updated context where $x$ is in the Moved state
2. The value of type $T$ (now available for binding elsewhere)


#### 3.4.2 Place Expressions

**Syntax**

```ebnf
place_expr       ::= identifier
                 | place_expr "." identifier
                 | place_expr "[" expression "]"
                 | "*" expression
```


#### 3.4.3 Move Expressions

**Syntax**

```ebnf
move_expr        ::= "move" place_expr
```

**Static Semantics**

**(T-Move)**

A move expression `move x` is valid if:
1. `x` is in the `Valid` state.
2. `x` is a movable binding (bound with `=`, not `:=`).

The expression yields the value of `x` and transitions `x` to the `Moved` state.


`move x` transitions the source binding `x` to the Moved state. Access to a Moved binding is forbidden. A `var` binding in Moved state MAY be reassigned, restoring it to Valid state. A `let` binding in Moved state MUST NOT be reassigned.


#### 3.4.4 Partial Moves

**Syntax**

```ebnf
partial_move     ::= "move" place_expr "." identifier
```

**Static Semantics**

**(T-Move-Field)**

A partial move `move x.f` is valid if:
1. `x` is a record type with field `f`.
2. `x` has `unique` permission.

The expression yields the value of `f` and transitions `x` to `PartiallyMoved(f)`.

Partial moves are permitted ONLY when both conditions are satisfied:


1. The binding is movable (established with `=`, not `:=`).
2. The binding has `unique` permission, providing exclusive access to the record's fields.

After a partial move, the parent binding enters the PartiallyMoved state. If the binding is a mutable `var`, reassignment restores it to Valid state. A `let` binding in PartiallyMoved state cannot be restored and remains invalid for the remainder of its scope.


#### 3.4.5 Parameter Responsibility Transfer

**Transferring Parameter.** A parameter declared with the `move` modifier. The callee assumes responsibility; the caller MUST pass a move expression (`move` applied to a place expression) at the call site; the source binding is invalidated after the call.

**Non-Transferring Parameter.** A parameter declared without `move`. The callee receives a non‑transferring alias to the same object, qualified with the parameter's permission. Responsibility remains with the caller; the source binding remains valid (subject to permission inactivation rules below).


#### 3.4.6 Move Typing Rules

**Diagnostics:** See Appendix A, codes `E-MEM-3001`–`E-MEM-3006`, `E-MOD-2411`.


### 3.5 Deterministic Destruction


#### 3.5.1 RAII Pattern

**RAII (Resource Acquisition Is Initialization).** The pattern where resources are acquired during object initialization and released during object destruction. Destruction in Cursive is deterministic—it occurs at a statically known program point.


#### 3.5.2 Drop Order (LIFO)

**Drop Order.** Bindings MUST be destroyed in strict LIFO order relative to their declaration within a scope.


#### 3.5.3 Normal Destruction Sequence

**Dynamic Semantics**

At scope exit, all responsible bindings MUST be destroyed in reverse declaration order:

1. If the binding's type implements `Drop` (§13.9.1), invoke `Drop::drop`.
2. Recursively destroy all fields and elements.
3. Release storage.


#### 3.5.4 Unwinding Semantics

**Dynamic Semantics**

During panic propagation, destructors for all in-scope responsible bindings MUST be invoked per LIFO semantics (§3.5.2) as each frame is unwound.


#### 3.5.5 Double Panic Handling

**Dynamic Semantics**

If a destructor panics while the thread is already unwinding, the runtime MUST abort the process immediately.

**Constraints**

Explicit calls to `Drop::drop` are forbidden. To destroy an object before scope exit, use the `mem::drop` utility procedure.

**Diagnostics:** See Appendix A, code `E-MEM-3005`.


### 3.6 The Abstract Machine and Observable Behavior


#### 3.6.1 Abstract Machine Definition

**Cursive Abstract Machine.** A state transition system that defines the execution semantics of a well-formed Cursive program. The abstract machine captures:

1. The **Storage Model** (§3.1) with provenance-tagged allocations
2. The **Binding Model** (§3.3) with liveness states
3. The **Permission Model** (§10) with aliasing constraints
4. The **Key Model** (§17) with synchronization state
5. The **Capability Model** (§4) with authority tracking

**Static Semantics**

**Machine State**

$$\sigma = (\mathcal{S}, \mathcal{B}, \mathcal{P}, \mathcal{K}, \mathcal{C}, \mathcal{F}, pc)$$

| Component     | Type                                                                  | Description                | Reference    |
| :------------ | :-------------------------------------------------------------------- | :------------------------- | :----------- |
| $\mathcal{S}$ | $\text{Address} \to (\text{Value}, \text{Type}, \pi)$                 | Storage with provenance    | §3.1, §3.2   |
| $\mathcal{B}$ | $\text{Name} \to (\text{Addr}, \text{State}, \text{Mov}, \text{Mut})$ | Binding context            | §3.3         |
| $\mathcal{P}$ | $\text{Path} \to (\text{Permission}, \text{ActiveState})$             | Permission context         | §10.1, §10.2 |
| $\mathcal{K}$ | $\mathcal{P}(\text{Path} \times \text{Mode} \times \text{Scope})$     | Key context                | §17.1        |
| $\mathcal{C}$ | $\text{CapabilitySet}$                                                | Available capabilities     | §4           |
| $\mathcal{F}$ | $[\text{Frame}]$                                                      | Control stack with cleanup | §3.5         |
| $pc$          | $\text{ProgramPoint}$                                                 | Current execution point    |              |

**Binding and Permission Substates**

Binding liveness states are defined in §3.3.4. `unique` downgrade active/inactive substates are defined in §10.2.2. The abstract machine tracks these substates but does not restate their transition tables here.

**Dynamic Semantics**

**Transition Relation**

The abstract machine progresses via transition $\sigma \to \sigma'$:

**(AM-Expr)** Expression evaluation:
$$\frac{
    \langle e, \sigma \rangle \Downarrow \langle v, \sigma' \rangle
}{
    \sigma \to \sigma'
}$$

**(AM-Stmt)** Statement execution:
$$\frac{
    \langle s, \sigma \rangle \to \sigma'
}{
    \sigma \to \sigma'
}$$

**(AM-Key-Acquire)** Key acquisition (from §17.2):
$$\frac{
    \text{Permission}(P) = \texttt{shared} \quad
    \neg\text{Covered}(P, M, \mathcal{K})
}{
    \mathcal{K}' = \mathcal{K} \cup \{(P, M, S)\}
}$$

**(AM-Drop)** Destruction (from §3.5):
$$\frac{
    \text{Responsible}(b) \quad
    \text{ScopeExit}(b)
}{
    \text{invoke } \texttt{Drop::drop}(b) \text{ then release storage}
}$$


#### 3.6.2 Observable Behavior

**Observable Behavior.** The externally detectable effect of program execution. The compiler MUST preserve observable behavior; all other aspects of execution are subject to transformation.

**Static Semantics**

**Observable Effects Classification**

| Effect                       | Mechanism                     | Authority Required  |
| :--------------------------- | :---------------------------- | :------------------ |
| File I/O                     | `$FileSystem` methods         | `ctx.fs`            |
| Network I/O                  | `$Network` methods            | `ctx.net`           |
| Heap allocation/deallocation | `$HeapAllocator` methods      | `ctx.heap`          |
| System operations            | `System` methods              | `ctx.sys`           |
| Async scheduling             | `$Reactor` methods            | `ctx.reactor`       |
| FFI calls                    | `extern` procedures           | `unsafe` block      |
| Panic initiation             | `panic!` or assertion failure | N/A                 |
| `Drop::drop` invocation      | Scope exit, explicit drop     | Responsible binding |

**Dynamic Semantics**

**Capability-Mediated Ordering**

Operations through the same capability instance are totally ordered:

$$\text{op}_1 \prec_{\text{cap}} \text{op}_2 \iff \text{op}_1 \text{ sequenced-before } \text{op}_2 \land \text{Capability}(\text{op}_1) = \text{Capability}(\text{op}_2)$$

Operations through distinct capability instances have no guaranteed relative ordering unless synchronized via:
1. `shared` permission with key acquisition (§17)
2. Explicit synchronization primitives

**Drop Ordering**

`Drop::drop` invocations are observable and MUST occur in LIFO order (§3.5.2):
1. Within a scope: reverse declaration order
2. During unwinding: as each frame is unwound

**Panic Observability**

1. Panic initiation is observable
2. Panic propagation through the control stack is observable
3. A program that panics MUST NOT exhibit behavior as if it had not panicked


#### 3.6.3 As-If Rule

**As-If Rule.** An implementation MAY perform any transformation of a Cursive program that does not change observable behavior.

**Static Semantics**

**Permitted Transformations**

| Transformation                   | Constraint                                       |
| :------------------------------- | :----------------------------------------------- |
| Dead code elimination            | Does not affect capability usage or `Drop` calls |
| Expression reordering            | Within sequence point constraints (§3.6.4)       |
| Procedure inlining               | Preserves capability requirements                |
| Loop unrolling                   | Preserves termination behavior                   |
| Common subexpression elimination | No capability-mediated side effects              |
| Register allocation              | N/A                                              |

**Prohibited Transformations**

| Transformation                                   | Reason                           |
| :----------------------------------------------- | :------------------------------- |
| Reordering capability operations                 | Violates observable I/O order    |
| Eliminating `Drop::drop` on responsible bindings | Drop is observable               |
| Introducing `shared` path data races             | Violates key system (§17)        |
| Changing provenance tags                         | Violates provenance model (§3.2) |
| Eliminating blocking key acquisitions            | May change program behavior      |
| Changing panic/non-panic outcome                 | Panic is observable              |

**Constraints**

**Invariant Preservation**

**(As-If-Provenance)**
$$\forall \text{transform } T : \pi(\text{ptr}_{\text{before}}) = \pi(\text{ptr}_{\text{after}})$$

**(As-If-Permission)**
$$\forall \text{transform } T : \text{Exclusivity}(\mathcal{P}_{\text{before}}) \Rightarrow \text{Exclusivity}(\mathcal{P}_{\text{after}})$$

**(As-If-Capability)**
$$\forall \text{transform } T : \text{order}_{\text{cap}}(\text{ops}_{\text{before}}) = \text{order}_{\text{cap}}(\text{ops}_{\text{after}})$$


#### 3.6.4 Sequence Points

**Sequence Point.** A program point where: (1) all side effects of preceding evaluations have completed, (2) no side effects of subsequent evaluations have begun, and (3) the binding context $\mathcal{B}$, permission context $\mathcal{P}$, and key context $\mathcal{K}$ are well-defined.

**Static Semantics**

**Mandatory Sequence Points**

| Context              | Sequence Point Location                             |
| :------------------- | :-------------------------------------------------- |
| Statement boundary   | After each terminated statement                     |
| Procedure call       | After all arguments evaluated, before callee begins |
| Short-circuit `&&`   | After left operand; before right if evaluated       |
| Short-circuit `\|\|` | After left operand; before right if evaluated       |
| `match` expression   | After scrutinee; before selected arm                |
| `if` expression      | After condition; before selected branch             |
| `#` block entry      | After coarsened path; before block body             |
| Key acquisition      | After path evaluation; before body executes         |
| Scope exit           | Before any `Drop::drop` invocations                 |

**Evaluation Order**

Subexpressions are evaluated left-to-right with sequence points:

**(Seq-Binary)**
$$\text{eval}(e_1 \oplus e_2) : \text{eval}(e_1) \to_{sp} \text{eval}(e_2) \to_{sp} \text{apply}(\oplus)$$

**(Seq-Call)**
$$\text{eval}(f(a_1, \ldots, a_n)) : \text{eval}(a_1) \to_{sp} \ldots \to_{sp} \text{eval}(a_n) \to_{sp} \text{call}(f)$$

**Key Context Determinacy**

At each sequence point, the key context $\mathcal{K}$ is determinate. Keys acquired during an expression's evaluation are held through subsequent sequence points until scope exit.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CON-0301`, `E-CON-0302`.


### 3.7 Unsafe Memory Operations


#### 3.7.1 Unsafe Block Definition

**Unsafe Block.** A lexical scope in which specific memory safety enforcement mechanisms are suspended.

**Syntax**

```ebnf
unsafe_block     ::= "unsafe" block
```

**Static Semantics**

**(T-Unsafe)**
$$\frac{\Gamma \vdash_{\text{unsafe}} \textit{body} : T}{\Gamma \vdash \texttt{unsafe } \{ \textit{body} \} : T}$$


#### 3.7.2 Suspended Checks

**Static Semantics**

Within an `unsafe` block, liveness, aliasing exclusivity, and key disjointness SHALL NOT be enforced.


#### 3.7.3 Enabled Operations

**Static Semantics**

The following operations are permitted ONLY within an `unsafe` block:

- Raw pointer dereference
- Unsafe procedure calls
- `transmute`
- Pointer arithmetic
- Calls to operations designated **unsafe-only** by this specification (e.g., `$HeapAllocator::alloc_raw`, `Region::reset_unchecked`, `Region::free_unchecked`)

Raw pointer types (`*imm T`, `*mut T`) are defined in §12.3.


#### 3.7.4 Transmute Constraints

**Syntax**

```ebnf
transmute_expr   ::= "transmute" "::" "<" type "," type ">" "(" expression ")"
```

**Static Semantics**

**(T-Transmute)**
$$\frac{\Gamma \vdash_{\text{unsafe}} e : S \quad \text{sizeof}(S) = \text{sizeof}(T)}{\Gamma \vdash_{\text{unsafe}} \texttt{transmute}::\langle S, T \rangle(e) : T}$$

`sizeof(S)` MUST equal `sizeof(T)`. The programmer MUST guarantee the bit pattern is valid for the target type; creating an invalid value constitutes Unverifiable Behavior (UVB).


#### 3.7.5 Programmer Responsibility Assertion

**Constraints**

The use of an `unsafe` block constitutes a normative assertion by the programmer that the code within satisfies all memory safety invariants. The effects of `unsafe` code MUST NOT compromise the safety of safe code outside the block.

**Diagnostics:** See Appendix A, codes `E-MEM-3030`, `E-MEM-3031`.

---

## 4. The Authority Model

---

### 4.1 Capability Authority Concepts


#### 4.1.1 Principle of No Ambient Authority

**No Ambient Authority.** The principle that all procedures producing observable external effects (I/O, networking, threading, heap allocation) MUST receive the required capability explicitly as a parameter. No global functions exist for these operations.

**Static Semantics**

A procedure $p$ may perform effect $e$ iff $p$ receives a capability $c$ where $e \in \text{Authority}(c)$:

$$\frac{\Gamma \vdash c : \$T \quad e \in \text{Authority}(T)}{\Gamma \vdash p(c) \text{ may perform } e}$$

**Ambient Authority Detection**

Violations are detected by verifying the **Capability Requirement Subset Property**: for every call from procedure $A$ to procedure $B$:

$$\text{capabilities}(B) \subseteq \text{capabilities}(A)$$

Intrinsic operations (file open, network connect) have hardcoded capability requirements. User procedures declare requirements through capability parameters. A procedure without capability parameters has empty requirements and cannot invoke effect-producing operations.

**Diagnostics:** See Appendix A, code `E-SYS-1001`.


#### 4.1.2 Capability Definition

**Capability.** A first-class value representing unforgeable authority to perform a specific class of external effects.

$$\text{Capability} ::= (\text{Authority}, \text{Interface}, \text{Lineage})$$

| Component | Description                                                 |
| :-------- | :---------------------------------------------------------- |
| Authority | The set of permitted operations (e.g., read files, connect) |
| Interface | The class defining available methods                        |
| Lineage   | The derivation chain from the root capability               |


#### 4.1.3 Capability Propagation

**Capability Propagation.** Capabilities travel through the call graph as explicit parameters. A procedure requiring a capability MUST accept it as a parameter; capabilities cannot be obtained through any other means in safe code.

**Syntax**

```ebnf
capability_param  ::= identifier ":" "$" class_path
```

The `$` sigil indicates capability type polymorphism. The `$Class` syntax accepts any concrete type implementing `Class`.

```cursive
procedure read_config(fs: $FileSystem, path: string@View): string
```

See §13.8.1 for a complete implementation example.

**Static Semantics**

A parameter of type `$Class` accepts any concrete type `T` where `T` implements `Class`.

**Receiver Permissions for Capability Methods**

Receiver permissions (`~`, `~!`, `~%`) govern aliasing and mutation rights on the capability *value itself* (see §10). They do not encode whether an operation is effectful; effectfulness is determined by the capability type and enforced by the authority model (§4.1.1).

Capability methods SHOULD choose the weakest receiver permission consistent with the capability's internal state requirements. For example, attenuation/configuration methods on `$HeapAllocator` are declared with `~!` receivers (Appendix D), while the standard I/O capability methods are declared with `~` receivers (Appendix D).

**Constraints**

**Diagnostics:** See Appendix A, code `E-SYS-1002`.


### 4.2 The Root of Authority (`Context`)


#### 4.2.1 Context Record Definition

**Context.** The root record of all system-level capabilities, injected into the program via the `main` procedure parameter at the entry point.

**Syntax**

```cursive
public procedure main(ctx: Context) -> i32
```

See §8.3.1 for complete entry point semantics.

**Static Semantics**

**Context Record Fields**

| Field     | Type                     | Authority                     |
| :-------- | :----------------------- | :---------------------------- |
| `fs`      | `$FileSystem`            | Filesystem operations         |
| `net`     | `$Network`               | Network operations            |
| `sys`     | `System`                 | System primitives (env, time) |
| `heap`    | `$HeapAllocator`         | Dynamic memory allocation     |
| `reactor` | `$Reactor`               | Async runtime (see §19)       |
| `cpu`     | `CpuDomainFactory`       | CPU parallel execution        |
| `gpu`     | `GpuDomainFactory \| ()` | GPU execution (see §18)       |
| `inline`  | `InlineDomainFactory`    | Inline execution (see §18)    |

**Runtime Injection Guarantee**

The runtime MUST guarantee that `ctx` is a valid, initialized `Context` record containing root capabilities before `main` begins execution.

**Main Procedure Requirements**

The `main` procedure:
- MUST be declared with `public` visibility
- MUST accept exactly one parameter of type `Context`
- MUST return `i32`
- MUST NOT be generic

**Diagnostics:** See Appendix A, code `E-MOD-2431`.


#### 4.2.2 Context Components (`fs`, `net`, `sys`, `heap`, `reactor`)

**Context Components.** The Context record fields are defined in §4.2.1. This section specifies the methods and operations available on each capability component.

**Static Semantics**

**`FileSystem` Class**

**Capability Type.** The capability type is `$FileSystem` (dynamic class object of `FileSystem`).

| Method | Return Type |
| :----- | :---------- |
| `open_read(path: string@View)` | `File@Read | IoError` |
| `open_write(path: string@View)` | `File@Write | IoError` |
| `open_append(path: string@View)` | `File@Append | IoError` |
| `create_write(path: string@View)` | `File@Write | IoError` |
| `read_file(path: string@View)` | `string@Managed | IoError` |
| `read_bytes(path: string@View)` | `bytes@Managed | IoError` |
| `write_file(path: string@View, data: bytes@View)` | `() | IoError` |
| `write_stdout(data: string@View)` | `() | IoError` |
| `write_stderr(data: string@View)` | `() | IoError` |
| `exists(path: string@View)` | `bool` |
| `remove(path: string@View)` | `() | IoError` |
| `open_dir(path: string@View)` | `DirIter@Open | IoError` |
| `create_dir(path: string@View)` | `() | IoError` |
| `ensure_dir(path: string@View)` | `() | IoError` |
| `kind(path: string@View)` | `FileKind | IoError` |
| `restrict(path: string@View)` | `$FileSystem` |

**`Network` Class**

**Capability Type.** The capability type is `$Network` (dynamic class object of `Network`).

| Method                            | Return Type            |
| :-------------------------------- | :--------------------- |
| `connect(addr: NetAddr)`          | `Stream \| NetError`   |
| `bind(addr: NetAddr)`             | `Listener \| NetError` |
| `restrict_to_host(addr: NetAddr)` | `$Network`             |

**`HeapAllocator` Class**

**Capability Type.** The capability type is `$HeapAllocator` (dynamic class object of `HeapAllocator`).

| Method                               | Return Type      |
| :----------------------------------- | :--------------- |
| `with_quota(size: usize)`            | `$HeapAllocator` |
| `alloc_raw<T>(count: usize)`         | `*mut T`         |
| `dealloc_raw<T>(ptr: *mut T, count)` | `()`             |

Safe heap allocation in Cursive occurs through explicit heap escape (`to_heap`, §3.2.7) or through built‑in/core‑library operations that require a `$HeapAllocator` parameter (e.g., `string@Managed` construction). The raw interface above is available only within `unsafe` blocks (§3.7).

**System Record**

| Method                      | Return Type |
| :-------------------------- | :---------- |
| `exit(code: i32)`           | `!`         |
| `get_env(key: string@View)` | `string`    |
| `time()`                    | `Timestamp` |

**`Reactor` Class**

**Capability Type.** The capability type is `$Reactor` (dynamic class object of `Reactor`).

| Method                                 | Return Type          |
| :------------------------------------- | :------------------- |
| `run<T, E>(future: Future<T, E>)`      | `T \| E`             |
| `register<T, E>(future: Future<T, E>)` | `FutureHandle<T, E>` |


#### 4.2.3 Context Threading

**Context Threading.** When a closure captures bindings from its enclosing scope for use within `spawn` or `dispatch` blocks, capability values follow specific capture rules.

**Static Semantics**

**Capability Delegation**

Spawned closures inherit the capability context of their parent. If the parent procedure has access to capabilities $C$, spawned children MAY use any capability $c \in C$. This is verified at the spawn site.

**Key Isolation**

The Key State Context (see §17) MUST NOT be inherited across spawn boundaries. Each spawned task MUST start with an empty key context and acquire its own keys independently.

**Resource Capture Rules**

| Capability Permission | Capture Behavior                                       |
| :-------------------- | :----------------------------------------------------- |
| `const`               | Captured by reference (read-only, no synchronization)  |
| `shared`              | Captured with `shared` permission (key-based sync)     |
| `unique`              | MUST be explicitly moved into exactly one spawned task |

**Spawn Capture Verification**

Each spawn site MUST satisfy:
1. All capabilities used within the spawned closure are captured from the enclosing scope or derived from captured capabilities
2. No `unique` capability is captured by multiple concurrent tasks
3. The parent's capability context covers all effects performed by spawned children


### 4.3 Attenuation Semantics


#### 4.3.1 Attenuation Definition

**Attenuation.** The process of creating a new capability with strictly less authority than the source capability.

$$\text{Attenuate}(C_{parent}, R) \to C_{child}$$

Where:
- $C_{parent}$: Source capability
- $R$: Restriction specification (path, quota, host filter)
- $C_{child}$: Derived capability where $\text{Authority}(C_{child}) \subseteq \text{Authority}(C_{parent})$

**Static Semantics**

**Attenuation Invariant**

A child capability MUST NOT grant more authority than its parent:

$$\forall c_{child} \in \text{Attenuate}(c_{parent}, \_) : \text{Authority}(c_{child}) \subseteq \text{Authority}(c_{parent})$$

**Attenuation Method Requirements**

Attenuation methods MUST return a capability that:
1. Implements the same capability class as the parent
2. Enforces the specified restrictions on all operations
3. Delegates authorized operations to the parent capability

**Type Preservation Rule**

$$\frac{\Gamma \vdash c : \$T \quad \Gamma \vdash c \mathord{\sim>} \texttt{restrict}(r) : \$T'}{\Gamma \vdash T' <: T}$$


#### 4.3.2 Quota Attenuation

**Quota Attenuation.** Restricts a capability by imposing resource limits on operations.

**Syntax**

```cursive
let limited_heap = heap~>with_quota(1024 * 1024)
```

**Static Semantics**

The `with_quota` method on `$HeapAllocator` returns an attenuated allocator that:
- Tracks cumulative allocation size
- Fails allocation requests that would exceed the quota
- Delegates permitted allocations to the parent allocator


#### 4.3.3 Permission Attenuation

**Permission Attenuation.** Restricts a capability by limiting the scope of permitted operations.

**Syntax**

```cursive
let app_fs = fs~>restrict("/app/data")
let local_net = net~>restrict_to_host(localhost)
```

**Static Semantics**

**FileSystem Restriction**

The `restrict(path)` method returns an attenuated filesystem capability that:
- Permits operations only within the specified path prefix
- Rejects operations targeting paths outside the restriction

**Network Restriction**

The `restrict_to_host(addr)` method returns an attenuated network capability that:
- Permits connections only to the specified host
- Rejects connection attempts to other hosts


#### 4.3.4 Capability Lifecycle

**Capability Lifecycle.** Attenuated capabilities maintain a dependency relationship with their parent. The following invariants hold:

1. **Derivation Dependency**: An attenuated capability maintains a runtime dependency on its parent. Operations on the child delegate to the parent.

2. **Parent Outlives Children**: The lifetime of a parent capability MUST equal or exceed the lifetime of all attenuated children derived from it.

3. **Child Drop Independence**: Dropping an attenuated child does not affect the parent capability.

4. **Parent Drop Restriction**: A parent capability MUST NOT be dropped while any attenuated children derived from it are still active.

**Static Semantics**

**Lifetime Enforcement Rule**

For any attenuation operation $c_{child} = c_{parent} \mathord{\sim>} \texttt{restrict}(r)$:

$$\text{Lifetime}(c_{parent}) \geq \text{Lifetime}(c_{child})$$

Attenuated capabilities MUST NOT escape the scope of their parent.

---

# Part III: Program Structure

## 5. Modules and Compilation

---

### 5.1 Compilation Units and Projects


#### 5.1.1 Project Definition

**Project.** The top-level organizational unit consisting of a collection of source files and a single manifest file (`Cursive.toml`) at its root. A project defines one or more assemblies.


#### 5.1.2 Assembly Definition

**Assembly.** A collection of modules compiled and distributed as a single unit. An assembly MAY be either a `library` or an `executable`. Each assembly is defined within the project manifest.


#### 5.1.3 Module Definition

**Module.** The fundamental unit of code organization and encapsulation. A module's contents are defined by the `.cursive` source files within a single directory. Its namespace is identified by a unique, filesystem-derived module path.

**Syntax**

```ebnf
top_level_item ::= import_decl
                 | using_decl
                 | static_decl
                 | procedure_decl
                 | record_decl
                 | enum_decl
                 | modal_decl
                 | type_alias_decl
                 | class_declaration

static_decl    ::= visibility? ("let" | "var") binding_decl
```

**Static Semantics**

**Module Well-Formedness**

A module is well-formed if:
1. All top-level declarations are valid according to the grammar
2. Each identifier is unique within the module's namespace
3. No control-flow constructs (`if`, `loop`, `match`) appear at top level
4. No expression statements appear at top level

**Visibility Default**

Top-level items without an explicit visibility modifier default to `internal`.

**Identifier Uniqueness**

Each identifier MUST be unique within module scope. Because Cursive uses a unified namespace (§6.1), a type declaration and a term declaration MUST NOT share the same identifier.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-MOD-1106`, `E-MOD-1302`, `E-SRC-0501`.


#### 5.1.4 Compilation Unit

**Compilation Unit.** The set of all source files constituting a single module. Each compilation unit defines exactly one module. An empty module (containing only whitespace and comments) is valid.

**Normative.** If any source file in a compilation unit cannot be read, loaded, tokenized, or parsed, compilation of the containing module MUST fail. The implementation MUST NOT continue to process later source files in that compilation unit and MUST NOT attempt to parse later modules for that compilation.








### 5.3 Module Paths and Discovery


#### 5.3.1 Module Path Definition

**Module Path.** A sequence of identifiers separated by `::` that uniquely identifies a module within an assembly. Module paths are derived from filesystem structure.

**Syntax**

```ebnf
module_path ::= identifier ("::" identifier)*
```

**Static Semantics**

**Module Path Derivation**

The module path for a given module MUST be derived from its directory path relative to the assembly's source root directory. Directory separators in the path MUST be replaced by the scope resolution operator `::`.

**(WF-Module-Path-Derivation)**

Let $P$ be the project context containing assembly definitions.

$$
\frac{P \vdash \text{assembly} \Rightarrow (\text{root\_path}, \_) \quad \text{dir\_path} = \text{absolute}(\text{module\_dir}) \quad \text{rel\_path} = \text{relative}(\text{dir\_path}, \text{root\_path})}{P \vdash \text{dir\_path} \Rightarrow \text{replace}(\text{rel\_path}, \text{PATH\_SEPARATOR}, "::")}
$$


#### 5.3.2 Path Component Validation

**Static Semantics**

**(WF-Module-Path)**

$$
\frac{
    \forall c \in \text{components}(p), (\vdash c : \text{Identifier} \land \vdash c : \text{NotKeyword})
}{
    \vdash p : WF_{path}
}
$$

A module path is well-formed if and only if every component is a valid Cursive identifier (§2.5) and is not a reserved keyword (§2.5.2).

**Case-Sensitivity Collision Detection**

On filesystems that are not case-sensitive, two file or directory names that differ only in case MUST be treated as ambiguous if they would resolve to the same module path component.

Let $N(p)$ be the NFC-normalized, case-folded version of a path component $p$:

**(WF-Module-Path-Collision)**

$$
\frac{
    \exists p_1, p_2 \in \text{project\_paths} \quad p_1 \neq p_2 \quad N(p_1) = N(p_2)
}{
    \text{Collision Error}
}
$$

**Diagnostics:** See Appendix A, codes `E-MOD-1104`, `E-MOD-1105`, `W-MOD-1101`. For `E-MOD-1106` (invalid identifier in module path), see §5.1.3.

---

## 6. Scopes and Names

---

### 6.1 Lexical Scopes and Program Points


#### 6.1.1 Program Point Definition

**Program Point.** An abstract location in the execution sequence of a program, situated between consecutive statements or at scope boundaries.

**Static Semantics**

Program points serve as reference locations for:
- Binding state transitions (Uninitialized → Valid → Moved)
- Scope entry and exit
- Lifetime validity checks


#### 6.1.2 Lexical Scope Definition

**Lexical Scope.** A contiguous region of source text where a set of names is valid. Scopes are lexically nested.

**Static Semantics**

A scope is characterized by:

$$\text{LexicalScope} ::= (\text{EntryPoint}, \text{ExitPoint}, \text{Parent}?)$$

Where:
- $\text{EntryPoint}$ is the program point where the scope begins
- $\text{ExitPoint}$ is the program point where the scope ends
- $\text{Parent}$ is the immediately enclosing scope (absent for procedure-level scope)


#### 6.1.3 Scope Nesting Rules

**Static Semantics**

Scopes form a strict hierarchy. An inner scope $S_i$ is **nested within** an outer scope $S_o$ if $S_i$ appears before $S_o$ in the context list. The innermost scope is always searched first during name lookup.

**Scope Hierarchy**

| Scope Level    | Contents                                                            |
| :------------- | :------------------------------------------------------------------ |
| $S_{local}$    | Block-local bindings (`let`, `var`, loop variables, match bindings) |
| $S_{proc}$     | Procedure parameters and local type parameters                      |
| $S_{module}$   | Module-level declarations and imported/used names                   |
| $S_{universe}$ | Primitive types (§2.5.4) and the `cursive.*` namespace              |


#### 6.1.4 Scope Context Structure

**Scope Context.** The abstract data structure ($\Gamma$) representing the mapping from identifiers to entities during name resolution.

**Static Semantics**

The scope context is an ordered list of scope mappings from innermost to outermost:

$$\Gamma ::= [S_{local}, S_{proc}, S_{module}, S_{universe}]$$

Where each $S$ is a scope that maps identifiers to entities:

$$S : \text{Identifier} \to \text{Entity}$$

**Unified Namespace**

Cursive MUST be implemented with a single, **unified namespace** per scope. An identifier's meaning is determined only by its spelling and the scope in which it is defined, not by the syntactic context of its use.

This single namespace MUST be shared by all declaration kinds:
1. **Terms**: Bindings for variables, constants, and procedures
2. **Types**: Bindings for type declarations (`record`, `enum`, `modal`, `type`)
3. **Modules**: Bindings for module import aliases

**Context Extension**

The context $\Gamma$ is extended by adding a binding to the innermost scope:

$$\Gamma, x : T \equiv [S_{local} \cup \{x \mapsto T\}, S_{proc}, S_{module}, S_{universe}]$$

**Name Binding Model**

Each scope $S$ maintains a mapping from identifiers to **entities**. An entity is one of:
- A term (variable, constant, procedure)
- A type (record, enum, modal, type alias)
- A class
- A module reference

**Constraints**

For duplicate name diagnostics (`E-MOD-1302`), see §5.1.3.


### 6.2 Name Resolution and Shadowing


#### 6.2.1 Name Introduction

**Name Introduction.** The process of adding a binding to a scope's namespace. A declaration introduces an identifier into the current scope.

**Static Semantics**

A declaration that introduces an identifier into the current scope MUST be well-formed only if that identifier is not already bound in the same scope.

**(WF-Name-Introduction)**

$$\frac{\Gamma \vdash \text{decl}(x) \quad x \notin \text{dom}(S_{current})}{\Gamma, x : T \vdash \text{decl}(x) : WF}$$


#### 6.2.2 Shadowing Rules (`shadow` keyword)

**Shadowing.** Occurs when a declaration in an inner scope introduces a binding that obscures a binding of the same name in an outer scope. Cursive requires explicit acknowledgment of shadowing.

**Syntax**

```ebnf
shadowed_binding ::= "shadow" "let" identifier (":" type)? "=" expression
                   | "shadow" "var" identifier (":" type)? "=" expression
```

**Static Semantics**

A declaration in an inner scope MAY shadow a name from an outer scope:
- Explicit shadowing with the `shadow` keyword is well-formed when the name exists in an outer scope but not in the current scope.
- Using `shadow` when no binding is being shadowed is ill-formed.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-MOD-1303`, `E-MOD-1306`.

**Deterministic Priority.**
When multiple name-introduction or shadowing constraints apply to the same declaration, the compiler MUST apply the first matching check in the following order:
1. Reserved identifier constraints (§2.5.2, §2.5.3).
2. Duplicate-in-scope constraint (§6.2.1).
3. Shadow-required constraint (missing `shadow` when a binding exists in an outer scope): `E-MOD-1303`.
4. Shadow-unnecessary constraint (`shadow` used when no outer binding exists): `E-MOD-1306`.


#### 6.2.3 Unqualified Lookup Algorithm

**Unqualified Name Lookup.** The process of resolving an identifier without a path prefix by searching the scope context from innermost to outermost.

**Static Semantics**

**(Lookup-Unqualified)**

$$\frac{\Gamma = [S_1, S_2, \ldots, S_n] \quad i = \min\{j \mid x \in \text{dom}(S_j)\}}{\Gamma \vdash \text{lookup}(x) = S_i(x)}$$

If the identifier is found in the innermost scope, that binding is returned. Otherwise, lookup proceeds to the next outer scope. If the context is exhausted and the name is not found, the lookup fails with `E-MOD-1301`.

**Constraints**

**Diagnostics:** See Appendix A, code `E-MOD-1301`.


#### 6.2.4 Qualified Lookup Algorithm

**Qualified Name Lookup.** Resolves a path of identifiers separated by `::`, such as `A::B::C`.

**Static Semantics**

To resolve a path $p::i$:

1. The prefix $p$ must first resolve to a module $m$
2. The identifier $i$ must resolve to a member of $m$
3. That member must be accessible from the current scope (§6.3)

**(Lookup-Qualified)**

$$\frac{\Gamma \vdash \text{lookup}(p) = m \quad m : \text{Module} \quad i \in \text{members}(m) \quad \text{accessible}(\Gamma, m.i)}{\Gamma \vdash \text{lookup}(p::i) = m.i}$$

**Diagnostics:** See Appendix A, code `E-MOD-1304`.


### 6.3 Visibility and Access Control


#### 6.3.1 Visibility Levels (`public`, `internal`, `private`, `protected`)

**Visibility Level.** Every top-level declaration has a visibility level that controls its accessibility from other modules.

**Syntax**

```ebnf
visibility ::= "public" | "internal" | "private" | "protected"
```

```cursive
public procedure foo() { ... }
internal record Bar { ... }
private let secret: i32 = 42
```

**Static Semantics**

| Modifier    | Scope of Accessibility                                              |
| :---------- | :------------------------------------------------------------------ |
| `public`    | Visible to any module in any assembly that depends on it            |
| `internal`  | (Default) Visible only to modules within the **same assembly**      |
| `private`   | Visible only within the **defining module** (same directory)        |
| `protected` | Visible only within the **defining type** and class implementations |


#### 6.3.2 Accessibility Definition

**Accessible.** An item is accessible from a given context if the visibility rules permit the reference.

**Static Semantics**

Let the context $\Gamma$ contain the accessor module $m_{acc}$ and its assembly $a_{acc}$. Let the target item $i$ be defined in module $m_{def}$ within assembly $a_{def}$. The judgment $\Gamma \vdash \text{can\_access}(i)$ holds if access is permitted.

**(WF-Access-Public)**

$$\frac{\text{visibility}(i) = \text{public}}{\Gamma \vdash \text{can\_access}(i)}$$

**(WF-Access-Internal)**

$$\frac{\text{visibility}(i) = \text{internal} \quad a_{acc} = a_{def}}{\Gamma \vdash \text{can\_access}(i)}$$

**(WF-Access-Private)**

$$\frac{\text{visibility}(i) = \text{private} \quad m_{acc} = m_{def}}{\Gamma \vdash \text{can\_access}(i)}$$

**(WF-Access-Protected-Self)**

$$\frac{\text{visibility}(item) = \text{protected} \quad \Gamma \subseteq T_{def}}{\Gamma \vdash \text{can\_access}(item)}$$

**(WF-Access-Protected-Class)**

$$\frac{\text{visibility}(item) = \text{protected} \quad \Gamma \subseteq \text{class } Cl \text{ for } T_{def} \quad A(\Gamma) = A(T_{def})}{\Gamma \vdash \text{can\_access}(item)}$$

**Diagnostics:** See Appendix A, code `E-MOD-1305`.


#### 6.3.3 Intra-Assembly Access Rules

**Static Semantics**

Modules within the same assembly are automatically available for qualified name access without requiring an `import` declaration.

A declaration `item` in module `mod` within assembly $A$ is accessible from another module in assembly $A$ via the qualified path `mod::item` if and only if the visibility of `item` is `public` or `internal`.


#### 6.3.4 Protected Member Restrictions

**Static Semantics**

The `protected` modifier MUST NOT be used on top-level (module-scope) declarations. It MUST only be applied to members (fields or procedures) within a `record`, `enum`, or `modal` declaration.

**Diagnostics:** See Appendix A, codes `E-MOD-2440`, `E-MOD-1207`.


### 6.4 Import and Using Declarations


#### 6.4.1 Import Declaration Syntax

**Import Declaration.** Declares a dependency on a module from an external assembly, making that module's public items available for qualified access.

**Syntax**

```ebnf
import_decl ::= "import" module_path ("as" identifier)?
```

```cursive
import core::io
import core::collections as col
```

**Static Semantics**

An `import` declaration:

1. Establishes a dependency edge from the current assembly to the imported assembly
2. Makes the imported module's namespace available for qualified access
3. Does **NOT** introduce any names into the current scope's unqualified namespace
4. If `as alias` is specified, the alias becomes the local name for the module; otherwise, the last component of the module path is used

**Import Requirement for External Access**

For modules in **external assemblies**, an `import` declaration MUST precede any `using` declaration that references items from that assembly.

For modules in the **same assembly** (intra-assembly access), `using` declarations MAY directly reference module paths without a preceding `import`.

**Diagnostics:** See Appendix A, codes `E-MOD-1201`, `E-MOD-1202`.


#### 6.4.2 Using Declaration Variants (Item, List, Self, Wildcard)

**Using Declaration.** Creates a scope alias that brings items into the current scope, enabling unqualified access to otherwise qualified names.

**Syntax**

```ebnf
using_decl      ::= visibility? "using" using_clause
using_clause    ::= using_path ("as" identifier)?
                 | using_path "::" using_list
                 | using_path "::" "*"
using_path      ::= module_path ("::" identifier)?
using_list      ::= "{" using_specifier ("," using_specifier)* ","? "}"
using_specifier ::= identifier ("as" identifier)?
                 | "self" ("as" identifier)?
```

```cursive
using core::io::File
using core::io::File as FileHandle
using core::collections::{Vec, HashMap}
using core::io::{self, File, Reader}
using core::prelude::*
```

**Static Semantics**

**Item Using** (`using path::Item`)

Resolves `Item` and introduces a binding with its original name (or the alias if `as alias` is specified).

**List Using** (`using path::{A, B, C}`)

Resolves each item in the list and introduces bindings for all of them. Each item in the list MUST be unique.

**Self Using** (`using path::{self, A}`)

The special specifier `self` in a using list introduces a binding for the module path itself, enabling qualified access via the module name alongside unqualified access to other items.

**Wildcard Using** (`using path::*`)

Brings all accessible (public or internal, depending on assembly relationship) items from the target module into the current scope. Each item retains its original name.

**(WF-Using-Item)**

$$\frac{
    \Gamma \vdash \text{resolve\_item}(\text{path}::i) \Rightarrow \text{item} \quad \Gamma \vdash \text{can\_access}(\text{item}) \quad \text{name} \notin \text{dom}(\Gamma)
}{
    \Gamma \vdash \text{using path}::i \text{ as name}: WF
}$$

**(WF-Using-Wildcard)**

$$\frac{
    \Gamma \vdash \text{resolve\_module}(\text{path}) \Rightarrow m \quad \forall i \in \text{accessible}(m), \text{name}(i) \notin \text{dom}(\Gamma)
}{
    \Gamma \vdash \text{using path}::*: WF
}$$

**Diagnostics:** See Appendix A, codes `E-MOD-1204`, `E-MOD-1206`, `W-MOD-1201`.


#### 6.4.3 Re-export Semantics

**Re-export.** A `public using` declaration re-exports an imported item, making it available to modules that depend on the current assembly.

**Syntax**

```cursive
public using core::io::File
```

**Static Semantics**

A `public using` declaration:

1. Performs the same resolution as a regular `using`
2. Additionally marks the introduced binding as `public`, making it visible to external dependents
3. The source item MUST itself be `public`; re-exporting `internal` or `private` items is forbidden

**(WF-Using-Public)**

$$\frac{
    \Gamma \vdash \text{using path}::i \text{ as name}: WF \quad \text{visibility}(\text{item}) = \text{public}
}{
    \Gamma \vdash \text{public using path}::i \text{ as name}: WF
}$$

**Diagnostics:** See Appendix A, code `E-MOD-1205`.


#### 6.4.4 Name Conflict Prevention

**Static Semantics**

Names introduced by `import as` or `using` declarations MUST NOT conflict with existing names in the same scope.

**Diagnostics:** See Appendix A, code `E-MOD-1203`.

---

## 7. Attributes and Metadata

---

### 7.1 Attribute Syntax and Registry


#### 7.1.1 Attribute Syntax

**Attribute.** A compile-time annotation attached to a declaration or expression that influences code generation, memory layout, diagnostics, verification strategies, and interoperability.

An attribute $A$ is defined by:

$$A = (\text{Name}, \text{Args})$$

where $\text{Name}$ is an identifier and $\text{Args}$ is a (possibly empty) sequence of arguments.

An **attribute list** is a sequence of one or more attributes:

$$\text{AttributeList} = \langle A_1, A_2, \ldots, A_n \rangle$$

**Syntax**

```ebnf
attribute_list     ::= attribute+
attribute          ::= "[[" attribute_spec ("," attribute_spec)* "]]"
attribute_spec     ::= attribute_name ("(" attribute_args ")")?
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

```cursive
[[attr1, attr2]]
declaration

[[attr1]]
[[attr2]]
declaration
```

**Static Semantics**

An attribute list MUST appear immediately before the declaration or expression it modifies. Multiple attributes MAY be specified in a single block or in separate blocks; the forms are semantically equivalent.

The order of attribute application within equivalent forms is Unspecified Behavior (USB).

**Diagnostics:** See Appendix A, code `E-MOD-2450`.


#### 7.1.2 Attribute Categories

**Attribute Registry.** The set ($\mathcal{R}$) of all attributes recognized by a conforming implementation:

$$\mathcal{R} = \mathcal{R}_{\text{spec}} \uplus \mathcal{R}_{\text{vendor}}$$

where $\mathcal{R}_{\text{spec}}$ contains specification-defined attributes and $\mathcal{R}_{\text{vendor}}$ contains vendor-defined extensions.

**Static Semantics**

**Attribute Processing**

For each attribute $A$ applied to declaration $D$:

1. The implementation MUST verify $A.\text{Name} \in \mathcal{R}$
2. The implementation MUST verify $D$ is a valid target for $A$
3. The implementation MUST verify $A.\text{Args}$ conforms to the argument specification
4. If all verifications succeed, the implementation applies the defined effect

**Attribute Target Matrix**

The matrix below enumerates all specification-defined attributes that may appear in Cursive source. Examples of vendor or custom attributes (e.g., `[[attr1]]`, `[[com.vendor.foo]]`) are not part of this registry.

| Attribute                                                                                              | `record` | `enum` | `modal` |     `procedure`     | Field | Binding |     `expression`      |
| :----------------------------------------------------------------------------------------------------- | :------: | :----: | :-----: | :-----------------: | :---: | :-----: | :-------------------: |
| `[[layout(...)]]`                                                                                      |   Yes    |  Yes   |   No    |         No          |  No   |   No    |          No           |
| `[[inline(...)]]`                                                                                      |    No    |   No   |   No    |         Yes         |  No   |   No    |          No           |
| `[[cold]]`                                                                                             |    No    |   No   |   No    |         Yes         |  No   |   No    |          No           |
| `[[static_dispatch_only]]`                                                                             |    No    |   No   |   No    |         Yes         |  No   |   No    |          No           |
| `[[deprecated(...)]]`                                                                                  |   Yes    |  Yes   |   Yes   |         Yes         |  Yes  |   Yes   |          No           |
| `[[reflect]]`                                                                                          |   Yes    |  Yes   |   Yes   |         No          |  No   |   No    |          No           |
| `[[derive(...)]]`                                                                                      |   Yes    |  Yes   |   Yes   |         No          |  No   |   No    |          No           |
| `[[symbol(...)]]`                                                                                      |    No    |   No   |   No    |      Yes (FFI)      |  No   |   No    |          No           |
| `[[library(...)]]`                                                                                     |    No    |   No   |   No    |      Yes (FFI)      |  No   |   No    |          No           |
| `[[no_mangle]]`                                                                                        |    No    |   No   |   No    |      Yes (FFI)      |  No   |   No    |          No           |
| `[[unwind(...)]]`                                                                                      |    No    |   No   |   No    |      Yes (FFI)      |  No   |   No    |          No           |
| `[[export(...)]]`                                                                                      |    No    |   No   |   No    |      Yes (FFI)      |  No   |   No    |          No           |
| `[[ffi_pass_by_value]]`                                                                                |   Yes    |  Yes   |   No    |         No          |  No   |   No    |          No           |
| `[[dynamic]]`                                                                                          |   Yes    |  Yes   |   Yes   |         Yes         |  No   |   No    |          Yes          |
| Verification-mode attributes (`[[static]]`, `[[assume]]`, `[[trust]]`)                                 |    No    |   No   |   No    | Yes (FFI contracts) |  No   |   No    |          No           |
| Memory-ordering attributes (`[[relaxed]]`, `[[acquire]]`, `[[release]]`, `[[acq_rel]]`, `[[seq_cst]]`) |    No    |   No   |   No    |         No          |  No   |   No    |          Yes          |
| `[[stale_ok]]`                                                                                         |    No    |   No   |   No    |         No          |  No   |   Yes   |          No           |
| `[[emit]]`                                                                                             |    No    |   No   |   No    |         No          |  No   |   No    | Yes (comptime blocks) |
| `[[files]]`                                                                                            |    No    |   No   |   No    |         No          |  No   |   No    | Yes (comptime blocks) |

**Diagnostics:** See Appendix A, codes `E-MOD-2451`, `E-MOD-2452`.


#### 7.1.3 Custom Attributes

**Vendor-Defined Attributes.** Attributes in $\mathcal{R}_{\text{vendor}}$ that extend the specification-defined set.

**Static Semantics**

1. Vendor attributes MUST use reverse-domain-style prefixes: `[[com.vendor.attribute_name]]`
2. The `cursive.*` namespace is reserved
3. Vendor-defined attributes are Implementation-Defined Behavior (IDB)

An implementation encountering an unknown attribute (not in $\mathcal{R}$) MUST diagnose error `E-MOD-2451`.


### 7.2 Layout Attributes (`[[layout]]`)


#### 7.2.1 `[[layout(C)]]`

**`[[layout(C)]]`.** Specifies C-compatible memory layout.

**Syntax**

```ebnf
layout_attribute ::= "[[" "layout" "(" layout_args ")" "]]"
layout_args      ::= layout_kind ("," layout_kind)*
layout_kind      ::= "C" | "packed" | "align" "(" integer_literal ")" | int_type
int_type         ::= "i8" | "i16" | "i32" | "i64" | "u8" | "u16" | "u32" | "u64"
```

**Static Semantics**

**For `record` declarations:**

1. Fields MUST be laid out in declaration order
2. Padding MUST be inserted only as required by the target platform's C ABI
3. Total size MUST be a multiple of the record's alignment

**For `enum` declarations:**

1. The discriminant MUST be represented as a C-compatible integer tag
2. Default tag type is Implementation-Defined (IDB)
3. Layout MUST conform to tagged union per target C ABI

**`[[layout(IntType)]]` (Explicit Discriminant):**

For an `enum` marked `[[layout(IntType)]]` where `IntType` is `i8`–`i64` or `u8`–`u64`:

1. The discriminant MUST use the specified integer type
2. Each variant's discriminant value MUST be representable in that type
3. This form is valid only on `enum` declarations


#### 7.2.2 `[[layout(packed)]]`

**`[[layout(packed)]]`.** Removes inter-field padding.

**Static Semantics**

For a `record` marked `[[layout(packed)]]`:

1. All inter-field padding is removed
2. Each field MUST be laid out with alignment 1
3. The record's overall alignment becomes 1

**Access Classification:**

| Operation                          | Classification         |
| :--------------------------------- | :--------------------- |
| `packed.field = value`             | Direct write (safe)    |
| `let x = packed.field`             | Direct read (safe)     |
| `packed.field~>method()`           | Reference-taking (UVB) |
| Pass to `move` parameter           | Direct read (safe)     |
| Pass to `const`/`unique` parameter | Reference-taking (UVB) |

Taking a reference to a packed field is Unverifiable Behavior (UVB); such operations MUST occur within an `unsafe` block.

**Diagnostics:** See Appendix A, code `E-MOD-2454`.


#### 7.2.3 `[[layout(align(N))]]`

**`[[layout(align(N))]]`.** Sets a minimum alignment.

**Static Semantics**

1. $N$ MUST be a positive integer that is a power of two
2. Effective alignment is $\max(N, \text{natural alignment})$
3. If $N < \text{natural alignment}$, natural alignment is used (warning emitted)
4. Type size is padded to a multiple of the effective alignment

**Diagnostics:** See Appendix A, codes `E-MOD-2453`, `W-MOD-2451`.


#### 7.2.4 Compile-Time Layout Verification

**Layout Verification.** Layout attribute combinations are verified at compile time for validity and applicability.

**Static Semantics**

**Valid Combinations:**

| Combination                | Validity | Effect                                  |
| :------------------------- | :------- | :-------------------------------------- |
| `layout(C)`                | Valid    | C-compatible layout                     |
| `layout(packed)`           | Valid    | Packed layout (records only)            |
| `layout(align(N))`         | Valid    | Minimum alignment $N$                   |
| `layout(C, packed)`        | Valid    | C-compatible packed layout              |
| `layout(C, align(N))`      | Valid    | C-compatible with minimum alignment $N$ |
| `layout(u8)` (enum)        | Valid    | 8-bit unsigned discriminant             |
| `layout(packed, align(N))` | Invalid  | Conflicting directives                  |

**Applicability Constraints:**

| Declaration Kind          |  `C`  | `packed` | `align(N)` | `IntType` |
| :------------------------ | :---: | :------: | :--------: | :-------: |
| `record`                  |   ✓   |    ✓     |     ✓      |     ✗     |
| `enum`                    |   ✓   |    ✗     |     ✓      |     ✓     |
| `modal`                   |   ✗   |    ✗     |     ✗      |     ✗     |
| Generic (unmonomorphized) |   ✗   |    ✗     |     ✗      |     ✗     |

**Diagnostics:** See Appendix A, code `E-MOD-2455`.


### 7.3 Optimization Hints


#### 7.3.1 `[[inline]]` and `[[inline(always)]]`

**`[[inline(...)]]`.** Specifies procedure inlining behavior.

**Syntax**

```ebnf
inline_attribute ::= "[[" "inline" ("(" inline_mode ")")? "]]"
inline_mode      ::= "always" | "never" | "default"
```

**Static Semantics**

**`[[inline]]`:**

The implementation SHOULD inline the procedure at call sites when feasible. No diagnostic is required if inlining is not performed.

**`[[inline(always)]]`:**

The implementation SHOULD inline the procedure at all call sites. If inlining is not possible (recursive, address taken), warning `W-MOD-2452` MUST be emitted.

**`[[inline(default)]]`:**

The implementation applies default heuristics. Semantically equivalent to omitting the attribute.

**No Attribute:**

The implementation MAY inline at its discretion.

**Diagnostics:** See Appendix A, code `W-MOD-2452`.


#### 7.3.2 `[[inline(never)]]`

**`[[inline(never)]]`.** Prohibits inlining.

**Static Semantics**

The implementation MUST NOT inline the procedure. The procedure body MUST be emitted as a separate callable unit; all calls MUST use the standard calling convention.


#### 7.3.3 `[[cold]]`

**`[[cold]]`.** Marks a procedure as unlikely to execute during typical runs.

**Syntax**

```ebnf
cold_attribute ::= "[[" "cold" "]]"
```

**Static Semantics**

Effects include:

1. Placement in separate code section for instruction cache locality
2. Reduced optimization effort
3. Branch prediction bias toward not calling

The attribute takes no arguments and is valid only on procedure declarations. It is a hint; implementations MAY ignore it.


### 7.4 Diagnostics and Metadata


#### 7.4.1 `[[deprecated]]`

**`[[deprecated(...)]]`.** Marks a declaration as deprecated.

**Syntax**

```ebnf
deprecated_attribute ::= "[[" "deprecated" ("(" string_literal ")")? "]]"
```

**Static Semantics**

When a program references a declaration marked `[[deprecated(...)]]`:

1. The implementation MUST emit warning `W-CNF-0601` (per §1.5)
2. If a message argument is present, the diagnostic SHOULD include it
3. The deprecated declaration remains fully functional


#### 7.4.2 `[[reflect]]`

**`[[reflect]]`.** Enables compile-time introspection for a type declaration, making its structure available to metaprogramming facilities.

**Syntax**

```ebnf
reflect_attribute ::= "[[" "reflect" "]]"
```

**Static Semantics**

1. Takes no arguments
2. Valid on `record`, `enum`, and `modal` declarations
3. No effect on runtime behavior or memory layout

**Exposed Metadata:**

For types marked `[[reflect]]`, the `reflect_type<T>()` intrinsic returns a `Type` value exposing:

| Type Kind | Available Metadata                                   |
| :-------- | :--------------------------------------------------- |
| `record`  | Field names, field types, field offsets, visibility  |
| `enum`    | Variant names, variant payloads, discriminant values |
| `modal`   | State names, state fields, transition signatures     |

**Compile-Time Only:**

Reflection operations are restricted to `comptime` blocks. The `Type` value and its accessors have no runtime representation.

**Diagnostics:** See Appendix A, codes `E-MOD-2460`, `E-MOD-2461`.


#### 7.4.3 `[[dynamic]]`

**`[[dynamic]]`.** Marks a declaration or expression as requiring runtime verification when static verification is insufficient.

**Syntax**

```ebnf
dynamic_attribute ::= "[[" "dynamic" "]]"
```

**Valid Targets:**

| Target       | Effect                                                     |
| :----------- | :--------------------------------------------------------- |
| `record`     | Operations on instances may use runtime verification       |
| `enum`       | Operations on instances may use runtime verification       |
| `modal`      | State transitions may use runtime verification             |
| `procedure`  | Contracts and body operations may use runtime verification |
| `expression` | The specific expression may use runtime verification       |

**Static Semantics**

**Scope Determination:**

An expression $e$ is within a `[[dynamic]]` scope if and only if:
1. $e$ is syntactically enclosed in a declaration marked `[[dynamic]]`, OR
2. $e$ is syntactically enclosed in an expression marked `[[dynamic]]`

**Propagation:**

1. Declaration-level: All expressions within the declaration are in scope
2. Expression-level: Only that expression and sub-expressions are in scope
3. Scope is determined lexically; does not propagate through procedure calls

**Effect on Key System (§17):**

Within a `[[dynamic]]` scope, if static key safety analysis fails, runtime synchronization MAY be inserted.

**Effect on Contract System (§14):**

Within a `[[dynamic]]` scope, if a contract predicate cannot be proven statically, a runtime check is inserted. Failed checks panic (`P-SEM-2850`).

**Effect on Refinement Types (§13.7):**

Within a `[[dynamic]]` scope, if a refinement predicate cannot be proven statically, a runtime check is inserted. Failed checks panic (`P-TYP-1953`).

**Dynamic Semantics**

Even within a `[[dynamic]]` scope, if a property is statically provable, no runtime code is generated.

**Diagnostics:** See Appendix A, codes `E-CON-0410`–`E-CON-0412`, `E-SEM-2801`, `E-TYP-1953`, `W-CON-0401`. For `E-CON-0020` (key safety outside `[[dynamic]]` scope), see §17.6.1.


#### 7.4.4 `[[static_dispatch_only]]`

**`[[static_dispatch_only]]`.** Excludes a procedure in a class from dynamic dispatch via `$Class` (vtable).

**Syntax**

```ebnf
static_dispatch_attr ::= "[[" "static_dispatch_only" "]]"
```

**Static Semantics**

**VTable Eligibility:**

A procedure $p$ in a class is vtable-eligible if:

$$\text{vtable\_eligible}(p) \iff \text{has\_receiver}(p) \land \neg\text{is\_generic}(p) \land \text{compatible\_signature}(p)$$

**Dispatchability (Dynamic Dispatch Safety):**

A class is **dispatchable** (usable as a dynamic class type `$Class`) if every procedure is either vtable-eligible or has `[[static_dispatch_only]]`:

$$\text{dispatchable}(C) \iff \forall p \in \text{procedures}(C).\ \text{vtable\_eligible}(p) \lor \text{has\_static\_dispatch\_attr}(p)$$

**Call Resolution:**

1. Static context: Calls on concrete types resolve normally
2. Dynamic context: Calls on dynamic class values (`$Class`) cannot resolve to `[[static_dispatch_only]]` procedures

**Constraints**

For diagnostic codes related to `[[static_dispatch_only]]` violations, see §13.5.3 (`E-TYP-2540`, `E-TYP-2542`).


#### 7.4.5 `[[stale_ok]]`

**`[[stale_ok]]`.** Suppresses staleness warnings (`W-CON-0011`) for a binding whose value was derived from `shared` data before a `release`/`yield release` boundary (§17.4.1, §19.2.2).

This attribute is intended for optimistic concurrency patterns where reusing a potentially stale snapshot is deliberate.

**Syntax**

```ebnf
stale_ok_attribute ::= "[[" "stale_ok" "]]"
```

**Static Semantics**

1. Takes no arguments.
2. Valid only on `let` and `var` binding declarations.
3. When present, the implementation MUST NOT emit `W-CON-0011` for uses of that binding.

The attribute does not alter key acquisition, release, or program behavior.

**Diagnostics:** See Appendix A, code `E-MOD-2465`.


### 7.5 FFI Attributes


#### 7.5.1 `[[symbol]]`

**`[[symbol("name")]]`.** Overrides the linker symbol name for a procedure, specifying the exact symbol used during linking.

**Syntax**

```ebnf
symbol_attribute ::= "[[" "symbol" "(" string_literal ")" "]]"
```

```cursive
extern "C" {
    [[symbol("__real_malloc")]]
    procedure malloc(size: usize) -> *mut c_void;
}
```

**Static Semantics**

1. Valid on procedure declarations within `extern` blocks and on exported procedures
2. The string argument specifies the exact symbol name used during linking
3. The symbol name MUST be a valid identifier for the target platform's linker
4. Does not affect Cursive-side name resolution; the procedure is still called by its Cursive identifier
5. Overrides any name mangling that would otherwise apply

**Symbol Resolution:**

| Declaration Context    | Without `[[symbol]]` | With `[[symbol("X")]]` |
| :--------------------- | :------------------- | :--------------------- |
| `extern "C"` procedure | Literal identifier   | `X`                    |
| `extern` procedure     | Mangled name         | `X`                    |
| Exported procedure     | Mangled name         | `X`                    |

**Diagnostics:** See Appendix A, codes `E-SYS-3340`–`E-SYS-3342`.


#### 7.5.2 `[[library]]`

**`[[library(...)]]`.** Specifies library linkage for an extern block.

**Syntax**

```ebnf
library_attribute ::= "[[" "library" "(" library_args ")" "]]"
library_args      ::= "name" ":" string_literal ("," "kind" ":" string_literal)?
```

```cursive
[[library(name: "ssl", kind: "dylib")]]
extern "C" {
    procedure SSL_new(ctx: *mut SSL_CTX) -> *mut SSL;
}
```

**Static Semantics**

**Link Kinds:**

| Kind          | Meaning                   |
| :------------ | :------------------------ |
| `"dylib"`     | Dynamic library (default) |
| `"static"`    | Static library            |
| `"framework"` | macOS framework           |
| `"raw-dylib"` | Windows delay-load        |

1. Valid only on `extern` blocks
2. The `name` argument specifies the library name without platform prefix/suffix (e.g., `"ssl"` not `"libssl.so"`)
3. If `kind` is omitted, `"dylib"` is assumed
4. Platform-specific library resolution applies (e.g., `"ssl"` resolves to `libssl.so` on Linux, `ssl.dll` on Windows)

**Diagnostics:** See Appendix A, codes `E-SYS-3345`–`E-SYS-3347`.


#### 7.5.3 `[[no_mangle]]`

**`[[no_mangle]]`.** Disables name mangling, causing the procedure to be exported with its literal Cursive identifier as the symbol name.

**Syntax**

```ebnf
no_mangle_attribute ::= "[[" "no_mangle" "]]"
```

**Static Semantics**

**Name Mangling:**

By default, Cursive encodes procedure symbols with module path, parameter types, and generic instantiations. The mangling scheme is Implementation-Defined Behavior (IDB).

**Effect of `[[no_mangle]]`:**

1. The procedure is exported with its identifier unchanged (e.g., `process`)
2. Valid on procedure declarations within `extern` blocks and on exported procedures
3. Implicit for procedures in `extern "C"` blocks (specifying it is redundant but permitted)
4. MUST NOT be combined with `[[symbol]]` on the same declaration

**Constraints**

**Diagnostics:** See Appendix A, codes `E-SYS-3350`, `E-SYS-3351`, `W-SYS-3350`.


#### 7.5.4 `[[unwind]]`

**`[[unwind(mode)]]`.** Controls panic propagation behavior at FFI boundaries, determining what happens when a panic attempts to cross into or out of foreign code.

**Syntax**

```ebnf
unwind_attribute ::= "[[" "unwind" "(" unwind_mode ")" "]]"
unwind_mode      ::= string_literal
```

**Static Semantics**

**Modes:**

| Mode      | Behavior                                                                                                                                                                                                               |
| :-------- | :--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `"abort"` | If a panic reaches the FFI boundary, the process terminates immediately via `abort()`. No destructors run for frames beyond the boundary.                                                                              |
| `"catch"` | Panics are caught at the boundary. For imported procedures, foreign exceptions are converted to Cursive panics. For exported procedures, Cursive panics are caught and the procedure returns an error indicator (IDB). |

**Default Behavior:**

If `[[unwind]]` is not specified, `"abort"` is assumed.

**Applicability:**

1. Valid on `extern` procedure declarations (imported foreign procedures)
2. Valid on exported procedures (Cursive procedures callable from foreign code)
3. The `"catch"` mode requires the foreign code's exception model to be compatible (IDB)

**Diagnostics:** See Appendix A, codes `E-SYS-3355`, `E-SYS-3356`, `W-SYS-3355`.

---

## 8. Program Lifecycle

---

### 8.1 Top-Level Declarations


#### 8.1.1 Declaration Kinds

**Top-Level Declaration.** A declaration appearing at module scope. Top-level declarations define the public and internal interface of a module.

**Static Semantics**

See §5.1.3 for the `top_level_item` grammar and module-level well-formedness constraints.


#### 8.1.2 Declaration Order Independence

**Declaration Order Independence.** The property that the semantic meaning of a well-formed module is independent of the textual order of its top-level declarations.

<!-- Source: §5.1.5 states "The resulting module semantics MUST be independent of file processing order for well-formed programs." This section formalizes that property for declaration order within files. -->

**Static Semantics**

**Order Independence Invariant**

For any well-formed module $M$ containing top-level declarations $D_1, \ldots, D_n$, and any permutation $\pi$ of those declarations:

$$\text{Semantics}(M[D_1, \ldots, D_n]) = \text{Semantics}(M[D_{\pi(1)}, \ldots, D_{\pi(n)}])$$

Name resolution proceeds in two phases:

1. **Collection Phase**: All top-level names are collected into the module namespace before any resolution occurs.
2. **Resolution Phase**: References are resolved against the complete namespace.


#### 8.1.3 Forward References

**Forward Reference.** A reference to a declaration that appears textually after the reference site within the same module.

**Static Semantics**

Forward references to types, classes, and procedure signatures are unconditionally permitted (type-level dependencies per §8.2.2).

Forward references to values (module-level `let` or `var` bindings) are subject to initialization order constraints (§8.2). A value forward reference is well-formed only if no cyclic dependency exists in the eager dependency graph.


### 8.2 Module Initialization Order


#### 8.2.1 Module Dependency Graph

**Module Dependency Graph.** A directed graph $G = (V, E)$ where: $V$ is the set of all modules in the program and its compiled dependencies, and $E$ is a set of directed edges $(A, B)$, where an edge from module $A$ to module $B$ signifies that $A$ depends on $B$.


#### 8.2.2 Edge Classification (Type-Level vs. Value-Level)

**Edge Classification.** Each edge in $E$ is classified as either Type-Level or Value-Level:

| Edge Type   | Definition                                                                |
| :---------- | :------------------------------------------------------------------------ |
| Type-Level  | Module $A$ refers to a type, class, or constant signature defined in $B$  |
| Value-Level | Module $A$ contains an expression that evaluates a binding defined in $B$ |

**Eager vs. Lazy Dependencies**

| Dependency Type | Definition                                                                      |
| :-------------- | :------------------------------------------------------------------------------ |
| **Eager**       | A Value-Level edge originating from a module-level initializer                  |
| **Lazy**        | A Type-Level edge, OR a Value-Level edge occurring only within procedure bodies |

**Static Semantics**

**(WF-Dep-Value)**

$$
\frac{
    \exists e \in \text{exprs}(A), \text{refers\_to\_value}(e, B)
}{
    (A, B) \in E, \text{class}((A, B)) = \text{Value-Level}
}
$$


#### 8.2.3 Topological Sort Execution

**Initialization Stages.** Module initialization is divided into two stages:

1. **Static Initialization**: Initializers that are compile-time constants (literals, `comptime` values) are evaluated at compile time and stored in the program's data section.

2. **Dynamic Initialization**: Initializers requiring runtime execution (procedure calls, values requiring capabilities) are executed after program startup but before the `main` procedure body executes.

**Static Semantics**

**Acyclic Eager Subgraph Requirement**

The subgraph $G_e = (V, E_e)$, containing all modules $V$ and only the set of eager edges $E_e$, MUST be a Directed Acyclic Graph (DAG).

**(WF-Acyclic-Eager-Deps)**

$$
\frac{
    \forall v \in V, \neg \text{is\_reachable}(v, v, E_e)
}{
    \vdash G_e : \text{DAG}
}
$$

Where $\text{is\_reachable}(u, v, E_e)$ is true if there is a path of one or more eager edges from $u$ to $v$.

**Dynamic Semantics**

**Execution Order**

Before the program's `main` procedure is invoked, the runtime MUST:

1. Compute a valid topological sort of modules based on the eager dependency graph $G_e$
2. For each module in the sorted list, execute the initializers for all module-level bindings in the order they appear within that module's source files

**Intra-Module Order**

Within a module, initializers MUST execute in strictly sequential lexical order within each source file. When a module comprises multiple source files, the order in which files are processed is Implementation-Defined Behavior (IDB) consistent with §5.1.5.


#### 8.2.4 Initialization Failure and Poisoning

**Dynamic Semantics**

If the evaluation of any module-level initializer terminates with a panic:

1. The initialization of that module is considered **failed**
2. The program state is considered **poisoned**
3. Any subsequent attempt to access a binding from that module, or from any module with an eager dependency path to it, MUST also result in a panic

**Diagnostics:** See Appendix A, code `E-MOD-1401`.


### 8.3 Program Entry Point (`main`)


#### 8.3.1 `main` Signature Requirements

**Entry Point.** The `main` procedure that receives the root capabilities and begins program execution after all module initialization is complete.

**Syntax**

```cursive
public procedure main(ctx: Context) -> i32
```

**Static Semantics**

**Entry Assembly Constraint**

Exactly one `main` procedure MUST exist in the entry assembly.

For signature requirements (`public` visibility, parameter type, return type, no generics), see §4.2.1.

**Diagnostics:** See Appendix A, codes `E-MOD-2430`, `E-MOD-2432`. For `E-MOD-2431` (invalid `main` signature), see §4.2.1.


#### 8.3.2 Context Parameter

**Context Parameter.** Provides the root capabilities for the program. The `Context` record is defined in §4.2.

**Static Semantics**

The `main` procedure receives the single `Context` instance containing all root capabilities. See §4.2.1 for the `Context` record definition and §4.2.2 for component specifications.

All side effects in the program require capabilities derived from this root `Context`.


#### 8.3.3 Return Semantics

**Return Semantics.** The `main` procedure returns an `i32` exit code indicating program termination status.

**Dynamic Semantics**

| Return Value | Meaning                                          |
| :----------- | :----------------------------------------------- |
| `0`          | Successful termination                           |
| Non-zero     | Failure (specific codes are application-defined) |

The runtime environment receives this exit code upon program termination.


#### 8.3.4 Global Mutable State Prohibition

**Global State.** Mutable storage with module lifetime, including: module-level `var` bindings and any other mutable storage accessible at module scope without requiring a capability parameter.

**Static Semantics**

Module-level `var` bindings MUST NOT have `public` visibility. This prohibition ensures capability-based security by preventing mutable state from being shared across module boundaries without explicit capability passing.

Module-level `var` bindings with `internal`, `protected`, or `private` visibility are permitted for module-internal state management.

**Diagnostics:** See Appendix A, code `E-MOD-2433`.

---

# Part IV: The Type System

---

## 9. Type System Fundamentals

---

### 9.1 Architecture and Equivalence


#### 9.1.1 Nominal vs. Structural Equivalence

**Type Equivalence.** See §1.2.1 for definitions of nominal equivalence and structural equivalence.

**Static Semantics**

Cursive MUST be implemented as a statically typed language. All type checking MUST be performed at compile time. A program that fails type checking MUST be diagnosed as ill-formed and rejected.

**Nominal Constructs**

The following type constructs use nominal equivalence:

- Record types (§11.6)
- Enum types (§11.7)
- Modal types (§12.1)

**Structural Constructs**

The following type constructs use structural equivalence:

- Tuple types (§11.2)
- Anonymous union types (§11.8)
- Function types (§12.4)
- Array and slice types (§11.3, §11.4)


#### 9.1.2 Type Identity

**Type Identity.** The type context $\Gamma$ and typing judgment notation are defined in §1.2.2. The scope context structure is defined in §6.1.4.

**Static Semantics**

**Equivalence Relation**

Two types $T$ and $U$ are equivalent, written $T \equiv U$, if one of the equivalence rules in §9.1.3 holds. Type equivalence MUST be reflexive, symmetric, and transitive.


#### 9.1.3 Type Compatibility

**Static Semantics**

**(T-Equiv-Nominal)** Nominal Equivalence:
Two nominal types are equivalent if and only if they refer to the same declaration.

**(T-Equiv-Tuple)** Tuple Equivalence: See §11.2.3.

**(T-Equiv-Func)** Function Type Equivalence:
Two function types are equivalent if they have the same:
1. Representation kind (sparse vs. closure)
2. Number of parameters
3. Parameter types (equivalent types at each position)
4. Parameter modifiers (e.g. `move`)
5. Return type (equivalent)

**(T-Equiv-Union)** Union Type Equivalence:
Union types are equivalent if their member type multisets are equal (order-independent).

**(T-Equiv-Permission)** Permission Equivalence:
Permission-qualified types are equivalent if both the permission and the underlying type are equivalent.


See §10 for the complete permission system.

**Constraints**

Top-level declarations (procedures, module-scope bindings, type definitions) MUST include explicit type annotations. Local bindings within procedure bodies MAY omit type annotations when the type is inferable (§9.4).

**Diagnostics:** See Appendix A, codes `E-TYP-1501`–`E-TYP-1503`.


### 9.2 Subtyping and Coercion


#### 9.2.1 Subtyping Relation

**Subtype Relation.** A partial order on types that determines when a value of one type may be used where a value of another type is expected. A type $S$ is a subtype of type $T$, written $S <: T$, if a value of type $S$ may be safely used in any context requiring a value of type $T$.

**Static Semantics**

The subtype relation $<:$ is a binary relation on types:

$$<:\ \subseteq\ \mathcal{T} \times \mathcal{T}$$

The relation MUST be:
- **Reflexive**: $T <: T$ for all $T$
- **Transitive**: if $S <: T$ and $T <: U$, then $S <: U$
- **Antisymmetric**: if $S <: T$ and $T <: S$, then $S \equiv T$


#### 9.2.2 Implicit Coercions

**Coercion.** Implicit conversion that occurs when an expression of a subtype is used in a context expecting a supertype.

**Static Semantics**

**(T-Coerce)**

Implicit coercion is permitted if the expression type $S$ is a subtype of the target type $T$ ($S <: T$).

**Constraints**


A coercion MUST NOT introduce Unverifiable Behavior (UVB).

An implementation MUST reject any assignment or argument where the source type is not a subtype of the target type.


#### 9.2.3 Coercion Sites

**Static Semantics**

**Permission Subtyping**

Permission types form a linear subtype lattice. See §10.3 for the complete lattice structure and formal subtyping rules.

**Class Implementation Subtyping**

A concrete type that implements a class is a subtype of that class. See §13.3 for class implementation and the formal rule (Sub-Class).

**Additional Subtyping Rules**

The following subtyping relationships are defined in their respective sections:

- Modal widening ($M@S <: M$, where $M$ is the general modal type): §12.1
- Union member subtyping: §11.8
- Refinement type subtyping: §13.7
- Permission subtyping: §10.3

**Diagnostics:** See Appendix A, codes `E-TYP-1510`–`E-TYP-1512`.


### 9.3 Variance

**Variance.** Specifies how subtyping of type arguments relates to subtyping of parameterized types. For a generic type constructor $F$ with parameter $T$, variance determines whether $F[A] <: F[B]$ when $A <: B$, when $B <: A$, both, or neither.

**Static Semantics**

**Variance Classifications**

| Variance      | Symbol | Condition for $F[A] <: F[B]$ | Position Requirement                              |
| :------------ | :----- | :--------------------------- | :------------------------------------------------ |
| Covariant     | $+$    | $A <: B$                     | Output positions (return types, immutable fields) |
| Contravariant | $-$    | $B <: A$                     | Input positions (parameter types)                 |
| Invariant     | $=$    | $A \equiv B$                 | Both input and output, or mutable storage         |
| Bivariant     | $\pm$  | Always (parameter unused)    | Parameter does not appear in type structure       |

**Generic Subtyping Rule**

For a generic type `Name<T1...Tn>` to be a subtype of `Name<U1...Un>`, each type argument pair `(Ti, Ui)` must satisfy the variance of the corresponding parameter:
- **Covariant (+)**: `Ti <: Ui`
- **Contravariant (-)**: `Ui <: Ti`
- **Invariant (=)**: `Ti == Ui`
- **Bivariant (+/-)**: Always compatible

**Function Type Variance**

Function types are contravariant in their parameter types and covariant in their return types. `(U) -> R1` is a subtype of `(T) -> R2` if `T <: U` and `R1 <: R2`.

**Permission Interaction**

The `const` permission enables covariance for otherwise invariant generic types: `const C<A>` is a subtype of `const C<B>` if `A <: B`. This applies because `const` prohibits mutation.

This relaxation does **not** apply to `unique` or `shared` permissions, which require strict invariance for mutable generic parameters.


**Constraints**

An implementation MUST reject a subtyping judgment that violates variance rules.

When a type parameter has invariant variance, the implementation MUST require exact type equivalence for that parameter in subtyping checks.

**Diagnostics:** See Appendix A, codes `E-TYP-1520`, `E-TYP-1521`.


### 9.4 Type Inference


#### 9.4.1 Bidirectional Type Checking

**Type Inference.** The process by which type information not explicitly annotated in the source program is derived. Cursive employs bidirectional type inference, combining two complementary modes of type propagation.

**Static Semantics**

**Judgment Classes**

| Judgment                        | Name      | Meaning                                                  |
| :------------------------------ | :-------- | :------------------------------------------------------- |
| $\Gamma \vdash e \Rightarrow T$ | Synthesis | Type $T$ is derived from the structure of expression $e$ |
| $\Gamma \vdash e \Leftarrow T$  | Checking  | Expression $e$ is validated against expected type $T$    |

In synthesis mode, the type flows outward from the expression. In checking mode, an expected type flows inward from context to guide type derivation for subexpressions.

**Synthesis Rules**

*   **Variables**: Type is looked up in the context.
*   **Tuples**: Type is synthesized from component types.
*   **Calls**: Return type is synthesized from the function signature; arguments are checked against parameter types.
*   **Annotations**: Explicitly annotated expressions synthesize the annotated type.

**Checking Rules**

*   **Subsumption**: An expression checks against type `T` if it synthesizes type `S` and `S <: T`.
*   **Closures**: Checked against an expected function type by inferring parameter types from the expected signature and checking the body against the return type.



#### 9.4.2 Local Type Inference

**Local Type Inference.** Restricts inference to operate within a single procedure body.

**Static Semantics**

Type inference MUST be local: inference operates within a single procedure body and MUST NOT propagate type information across procedure boundaries.

An implementation MUST NOT infer types for public API boundaries.


#### 9.4.3 Inference Constraints

**Static Semantics**

**Mandatory Annotations**

The following positions MUST have explicit type annotations:

- Procedure parameter types
- Procedure return types
- Public and protected module-scope bindings

**Permitted Omissions**

The following positions MAY omit type annotations when the type is inferable:

- Local bindings within procedure bodies (`let`, `var`)
- Closure parameters when the closure is checked against a known function type

**Diagnostics:** See Appendix A, code `E-TYP-1505`.


#### 9.4.4 Inference Failures

**Static Semantics**

An implementation MUST reject a program if inference fails to derive a unique type for any expression.

When type arguments cannot be inferred, explicit type arguments may be supplied using turbofish syntax (`::<>`). See §13.1 for the full specification.

**Diagnostics:** See Appendix A, code `E-TYP-1530`.

---

## 10. The Permission System

---

### 10.1 Permission Lattice (`const`, `unique`, `shared`)


#### 10.1.1 `const` Permission (Read-Only)

**Permission.** A type qualifier governing access, mutation, and aliasing of data referenced by a binding. When no permission is specified, `const` is the default.

**`const` Permission.** Grants read-only access to data with unlimited aliasing. Mutation through a `const` path is forbidden.

**Syntax**

```ebnf
permission       ::= "const" | "unique" | "shared"
permission_type  ::= type
```

```cursive
let cfg: const Config = load()
let x = cfg.value
```

**Static Semantics**

Let $\mathcal{P}$ denote the set of permissions:

$$\mathcal{P} = \{\texttt{const}, \texttt{unique}, \texttt{shared}\}$$

A **permission-qualified type** is a pair $(P, T)$ where $P \in \mathcal{P}$ and $T$ is a base type. The notation $P\ T$ denotes this qualification.

**Operations Permitted:**

| Operation | Permitted |
| :-------- | :-------- |
| Read      | Yes       |
| Write     | No        |
| Aliasing  | Unlimited |


#### 10.1.2 `unique` Permission (Exclusive Read-Write)

**`unique` Permission.** Grants exclusive read-write access to data. A live `unique` path to an object precludes any other path to that object or its sub-components.

**Static Semantics**

**Exclusivity Invariant:**

$$\forall p_1, p_2 \in \text{Paths}.\ (\text{perm}(p_1) = \texttt{unique} \land \text{overlaps}(p_1, p_2)) \implies p_1 = p_2$$

**Operations Permitted:**

| Operation | Permitted  |
| :-------- | :--------- |
| Read      | Yes        |
| Write     | Yes        |
| Aliasing  | No aliases |

The `unique` permission does **not** imply cleanup responsibility. A parameter of type `unique T` (without `move`) grants exclusive access while the caller retains responsibility.


#### 10.1.3 `shared` Permission (Synchronized Access)

**`shared` Permission.** Grants mutable access through implicit key acquisition. See §17 for complete key system semantics.

**Static Semantics**

**Operations Permitted:**

| Operation                   | Permitted | Key Mode  |
| :-------------------------- | :-------- | :-------- |
| Field read                  | Yes       | Read key  |
| Field mutation              | Yes       | Write key |
| Method call (`~` receiver)  | Yes       | Read key  |
| Method call (`~%` receiver) | Yes       | Write key |
| Method call (`~!` receiver) | No        | N/A       |

**Key Properties:**

| Property      | Description                                               |
| :------------ | :-------------------------------------------------------- |
| Path-specific | Keys acquired at accessed path granularity                |
| Implicit      | Accessing `shared` path acquires necessary key            |
| Minimal       | Keys held for minimal duration                            |
| Reentrant     | Covering key permits nested access without re-acquisition |

The `shared` permission does **not** imply cleanup responsibility.


#### 10.1.4 Lattice Ordering

**Lattice Ordering.** The permission lattice orders permissions by aliasing guarantee strength.

**Static Semantics**

$$\texttt{unique} <: \texttt{shared} <: \texttt{const}$$

**Lattice Diagram:**

```
    unique      (strongest: exclusive, mutable)
       ↓
    shared      (synchronized, mutable)
       ↓
     const      (weakest: unlimited, immutable)
```


### 10.2 Exclusivity and Aliasing Rules

**Aliasing.** Two paths alias when they refer to overlapping storage locations: $\text{aliases}(p_1, p_2) \iff \text{storage}(p_1) \cap \text{storage}(p_2) \neq \emptyset$.

**Binding State Machine**

A binding $b$ with `unique` permission exists in one of two states:

| State    | Definition                                        | Operations Permitted         |
| :------- | :------------------------------------------------ | :--------------------------- |
| Active   | No downgraded references to $b$ are live          | Read, write, move, downgrade |
| Inactive | One or more downgraded references to $b$ are live | No operations                |

**Static Semantics**

**(Inactive-Enter)**
$$\frac{b : \texttt{unique}\ T \quad b\ \text{is Active} \quad \text{downgrade to}\ P\ \text{where}\ P \in \{\texttt{const}, \texttt{shared}\}}{b\ \text{becomes Inactive}}$$

**(Inactive-Exit)**
$$\frac{b\ \text{is Inactive} \quad \text{all downgraded references to}\ b\ \text{go out of scope}}{b\ \text{becomes Active with}\ \texttt{unique}\ \text{permission restored}}$$

**Constraints**

1. During the inactive period, the original `unique` binding MUST NOT be read, written, or moved.
2. The transition back to Active occurs deterministically when the downgrade scope ends.

**Coexistence Matrix**

| Active Permission | May Add `unique` | May Add `shared` | May Add `const` |
| :---------------- | :--------------- | :--------------- | :-------------- |
| `unique`          | No               | No               | No              |
| `shared`          | No               | Yes              | Yes             |
| `const`           | No               | Yes              | Yes             |


### 10.3 Permission Subtyping and Coercion

**Permission Subtyping.** This section formalizes the subtype relation between permission-qualified types. General subtyping is defined in §9.2.

**Subtyping Rules**

Permission subtyping allows treating a stronger permission as a weaker one:
- `unique T <: shared T`
- `unique T <: const T`
- `shared T <: const T`

Permissions coerce implicitly in one direction only: from stronger to weaker. Upgrade from weaker to stronger is forbidden.

**Diagnostics:** See Appendix A, codes `E-TYP-1601`, `E-TYP-1602`, `E-TYP-1604`.

**Method Receiver Permissions**

*[REF: Shorthand syntax (`~`, `~!`, `~%`) is defined in §14.2.2.]*

A method with receiver permission $P_{\text{method}}$ is callable through a path with permission $P_{\text{caller}}$ iff:

$$P_{\text{caller}} <: P_{\text{method}}$$

| Caller Permission | May Call `~` | May Call `~%` | May Call `~!` |
| :---------------- | :----------- | :------------ | :------------ |
| `const`           | Yes          | No            | No            |
| `shared`          | Yes          | Yes           | No            |
| `unique`          | Yes          | Yes           | Yes           |

**Diagnostics:** See Appendix A, code `E-TYP-1605`.

---

## 11. Concrete Data Types

---

### 11.0 Type Syntax


#### 11.0.1 Type Grammar

**Syntax**

```ebnf
type                ::= permission? non_permission_type
non_permission_type ::= union_type | non_union_type

primitive_type      ::= integer_type | float_type | bool_type | char_type
                      | unit_type | never_type | string_type | bytes_type

nominal_type        ::= type_path generic_args?
                      | state_specific_type
                      | dynamic_type
                      | opaque_type

opaque_type         ::= "opaque" class_path

pointer_type        ::= safe_pointer_type | raw_pointer_type
```

### 11.1 Primitive Types


#### 11.1.1 Integer Types (`i8`–`i128`, `u8`–`u128`, `isize`, `usize`)

**Integer Types.** Built-in scalar types representing whole numbers with fixed bit widths and defined signedness.

**Syntax**

```ebnf
integer_type   ::= signed_int | unsigned_int | pointer_int
signed_int     ::= "i8" | "i16" | "i32" | "i64" | "i128"
unsigned_int   ::= "u8" | "u16" | "u32" | "u64" | "u128"
pointer_int    ::= "isize" | "usize"
```

**Static Semantics**

**Type Set**

$$\mathcal{T}_{\text{int}} = \{\texttt{i8}, \texttt{i16}, \texttt{i32}, \texttt{i64}, \texttt{i128}, \texttt{u8}, \texttt{u16}, \texttt{u32}, \texttt{u64}, \texttt{u128}, \texttt{isize}, \texttt{usize}\}$$

**(T-Int-Literal)**
$$\frac{v \in \text{IntLiteral} \quad T \in \mathcal{T}_{\text{int}} \quad \text{InRange}(v, T)}{\Gamma \vdash v : T}$$

where $\text{InRange}(v, T)$ holds when the numeric value $v$ is representable in type $T$.

**No Implicit Conversion**

$$\frac{T \neq U \quad T \in \mathcal{T}_{\text{int}} \quad U \in \mathcal{T}_{\text{int}}}{\Gamma \nvdash T <: U}$$

**Dynamic Semantics**

**Integer Overflow**

Arithmetic operations (`+`, `-`, `*`) may overflow. In **checked mode** (default for debug builds), signed and unsigned overflow causes panic. Release-mode overflow behavior is IDB: wrap, panic, or trap.

**Division by Zero**

Division (`/`) and remainder (`%`) operations MUST panic when the divisor is zero.

**Constraints**

**Layout Table**

| Type    | Size (bytes) | Alignment (bytes) | Representation          |
| :------ | :----------- | :---------------- | :---------------------- |
| `i8`    | 1            | 1                 | Two's complement signed |
| `u8`    | 1            | 1                 | Unsigned                |
| `i16`   | 2            | 2                 | Two's complement signed |
| `u16`   | 2            | 2                 | Unsigned                |
| `i32`   | 4            | 4                 | Two's complement signed |
| `u32`   | 4            | 4                 | Unsigned                |
| `i64`   | 8            | 8                 | Two's complement signed |
| `u64`   | 8            | 8                 | Unsigned                |
| `i128`  | 16           | IDB               | Two's complement signed |
| `u128`  | 16           | IDB               | Unsigned                |
| `isize` | IDB          | IDB               | Two's complement signed |
| `usize` | IDB          | IDB               | Unsigned                |

The alignment of `i128` and `u128` is IDB (8 or 16 bytes). The size of `isize` and `usize` equals the platform pointer width.

**Diagnostics:** See Appendix A, codes `E-TYP-1710`, `E-TYP-1712`, `P-TYP-1720`, `P-TYP-1721`.


#### 11.1.2 Floating-Point Types (`f16`, `f32`, `f64`)

**Floating-Point Types.** Built-in scalar types representing IEEE 754-2008 binary floating-point numbers.

**Syntax**

```ebnf
float_type ::= "f16" | "f32" | "f64"
```

**Static Semantics**

$$\mathcal{T}_{\text{float}} = \{\texttt{f16}, \texttt{f32}, \texttt{f64}\}$$

**(T-Float-Literal)**
$$\frac{v \in \text{FloatLiteral} \quad T \in \mathcal{T}_{\text{float}}}{\Gamma \vdash v : T}$$

Default type inference for floating-point literals is `f64`.

**Dynamic Semantics**

Floating-point arithmetic MUST conform to IEEE 754-2008 for the corresponding precision (`f16`/binary16, `f32`/binary32, `f64`/binary64), including NaN, infinities, signed zero, and division-by-zero semantics.

**Constraints**

**Layout Table**

| Type  | Size (bytes) | Alignment (bytes) | Representation         |
| :---- | :----------- | :---------------- | :--------------------- |
| `f16` | 2            | 2                 | IEEE 754-2008 binary16 |
| `f32` | 4            | 4                 | IEEE 754-2008 binary32 |
| `f64` | 8            | 8                 | IEEE 754-2008 binary64 |

**Diagnostics:** See Appendix A, code `W-TYP-1701`.


#### 11.1.3 Boolean Type (`bool`)

**Boolean Type.** The type `bool` represents logical truth values with exactly two inhabitants: `true` and `false`.

**Syntax**

```ebnf
bool_type    ::= "bool"
```

**Static Semantics**

**(T-Bool-Literal)**
$$\frac{v \in \{\texttt{true}, \texttt{false}\}}{\Gamma \vdash v : \texttt{bool}}$$

**Constraints**

**Layout**

| Type   | Size (bytes) | Alignment (bytes) | Representation                    |
| :----- | :----------- | :---------------- | :-------------------------------- |
| `bool` | 1            | 1                 | `0x00` = `false`, `0x01` = `true` |

A valid `bool` value MUST be represented by exactly `0x00` or `0x01`. Any other bit pattern is invalid; constructing such a value is UVB.


#### 11.1.4 Character Type (`char`)

**Character Type.** The type `char` represents a single Unicode Scalar Value (USV).

**Syntax**

```ebnf
char_type ::= "char"
```

**Static Semantics**

**(T-Char-Literal)**
$$\frac{v \in \text{CharLiteral} \quad \text{IsUSV}(v)}{\Gamma \vdash v : \texttt{char}}$$

where $\text{IsUSV}(v)$ holds when $v$ is in ranges U+0000–U+D7FF or U+E000–U+10FFFF.

**Constraints**

**Layout**

| Type   | Size (bytes) | Alignment (bytes) | Representation                                               |
| :----- | :----------- | :---------------- | :----------------------------------------------------------- |
| `char` | 4            | 4                 | Unicode scalar value (U+0000–U+10FFFF, excluding surrogates) |

Surrogate code points (U+D800–U+DFFF) and values above U+10FFFF are invalid; constructing such a value is UVB.

**Diagnostics:** See Appendix A, code `E-TYP-1711`.


#### 11.1.5 Unit Type (`()`)

**Unit Type.** The type `()` is a zero-sized type with exactly one value, also written `()`.

**Syntax**

```ebnf
unit_type    ::= "(" ")"
unit_literal ::= "(" ")"
```

**Static Semantics**

**(T-Unit-Literal)**
$$\frac{}{\Gamma \vdash () : ()}$$

**Dynamic Semantics**

The unit value is the implicit result of:
- Procedures without an explicit return type
- Block expressions whose final statement is not an expression
- The `result` statement with no expression

**Constraints**

**Layout**

| Type | Size (bytes) | Alignment (bytes) |
| :--- | :----------- | :---------------- |
| `()` | 0            | 1                 |


#### 11.1.6 Never Type (`!`)

**Never Type.** The type `!` is the uninhabited bottom type. No value of type `!` can be constructed.

**Syntax**

```ebnf
never_type ::= "!"
```

**Static Semantics**

**(Sub-Never)**
$$\frac{T \in \mathcal{T}}{\Gamma \vdash \texttt{!} <: T}$$

**Dynamic Semantics**

Expressions of type `!` represent computations that do not return normally:
- Invocations of diverging procedures (e.g., `panic()`, `sys::exit()`)
- Infinite loops with no `break` expression
- The `unreachable!()` intrinsic

Code following an expression of type `!` is unreachable.

**Constraints**

**Layout**

| Type | Size (bytes) | Alignment (bytes) |
| :--- | :----------- | :---------------- |
| `!`  | 0            | 1                 |


### 11.2 Tuples


#### 11.2.1 Tuple Syntax

**Tuple.** An ordered, fixed-length, heterogeneous sequence of values with structural equivalence.

**Syntax**

```ebnf
tuple_type       ::= "(" tuple_elements? ")"
tuple_elements   ::= type ";" | type ("," type)+
tuple_literal    ::= "(" tuple_expr_elements? ")"
tuple_expr_elements ::= expression ";" | expression ("," expression)+
```

Single-element tuples require a trailing semicolon: `(T;)` (1-tuple) vs. `(T)` (parenthesized expression).

**Static Semantics**

**Type Set**

$$\mathcal{T}_{\text{tuple}} = \{(T_1, T_2, \ldots, T_n) : n \geq 0 \land \forall i \in 1..n,\ T_i \in \mathcal{T}\}$$

**(T-Tuple-Type)**
$$\frac{\forall i \in 1..n,\ \Gamma \vdash T_i\ \text{wf}}{\Gamma \vdash (T_1, \ldots, T_n)\ \text{wf}}$$

**(T-Tuple-Literal)**
$$\frac{\forall i \in 1..n,\ \Gamma \vdash e_i : T_i}{\Gamma \vdash (e_1, \ldots, e_n) : (T_1, \ldots, T_n)}$$


#### 11.2.2 Tuple Element Access

**Tuple Element Access.** Tuple elements are accessed by constant zero-based integer index using dot notation.

**Syntax**

```ebnf
tuple_access ::= postfix_expr "." decimal_literal
```

**Static Semantics**

**(T-Tuple-Index)**
$$\frac{\Gamma \vdash e : (T_0, T_1, \ldots, T_{n-1}) \quad 0 \leq i < n}{\Gamma \vdash e.i : T_i}$$

**Constraints**

**Diagnostics:** See Appendix A, codes `E-TYP-1801`–`E-TYP-1803`.


#### 11.2.3 Tuple Structural Equivalence

**Tuple Structural Equivalence.** Tuple types use structural equivalence. See §9.1.1.

**Static Semantics**

**(T-Equiv-Tuple)**
$$\frac{n = m \quad \forall i \in 1..n,\ \Gamma \vdash T_i \equiv U_i}{\Gamma \vdash (T_1, \ldots, T_n) \equiv (U_1, \ldots, U_m)}$$

**(Sub-Tuple)**
$$\frac{n = m \quad \forall i \in 1..n,\ \Gamma \vdash T_i <: U_i}{\Gamma \vdash (T_1, \ldots, T_n) <: (U_1, \ldots, U_m)}$$

**Constraints**

**Layout**

$$\text{sizeof}((T_1, \ldots, T_n)) = \sum_{i=1}^{n} \text{sizeof}(T_i) + \text{padding}$$

$$\text{alignof}((T_1, \ldots, T_n)) = \max_{i \in 1..n}(\text{alignof}(T_i))$$

For the unit type: $\text{sizeof}(()) = 0$ and $\text{alignof}(()) = 1$.


### 11.3 Arrays


#### 11.3.1 Array Syntax and Initialization

**Array.** A contiguous, fixed-length, homogeneous sequence of elements. The length is part of the type and MUST be known at compile time.

**Syntax**

```ebnf
array_type    ::= "[" type ";" const_expression "]"
array_literal ::= "[" expression_list "]"
                | "[" expression ";" const_expression "]"
expression_list ::= expression ("," expression)* ","?
const_expression ::= expression
```

**Static Semantics**

$$\mathcal{T}_{\text{array}} = \{[T; N] : T \in \mathcal{T} \land N \in \mathbb{N}\}$$

**(T-Array-Type)**
$$\frac{\Gamma \vdash T\ \text{wf} \quad N : \texttt{usize} \quad N \geq 0}{\Gamma \vdash [T; N]\ \text{wf}}$$

**(T-Array-Literal-List)**
$$\frac{\forall i \in 1..n,\ \Gamma \vdash e_i : T}{\Gamma \vdash [e_1, \ldots, e_n] : [T; n]}$$

**(T-Array-Literal-Repeat)**
$$\frac{\Gamma \vdash e : T \quad N : \texttt{usize} \quad T <: \texttt{Bitcopy} \lor e \in \text{ConstExpr}}{\Gamma \vdash [e; N] : [T; N]}$$

**Constraints**

**Diagnostics:** See Appendix A, codes `E-TYP-1810`, `E-TYP-1813`, `E-TYP-1814`.


#### 11.3.2 Array Bounds Checking

**Array Bounds Checking.** Array element access uses indexing with runtime bounds verification.

**Syntax**

```ebnf
array_access ::= expression "[" expression "]"
```

**Static Semantics**

**(T-Array-Index)**
$$\frac{\Gamma \vdash a : [T; N] \quad \Gamma \vdash i : \texttt{usize}}{\Gamma \vdash a[i] : T}$$

**Dynamic Semantics**

All array indexing is bounds-checked at runtime. Access with index $i \geq N$ MUST panic.

**Constraints**

**Diagnostics:** See Appendix A, code `E-TYP-1812`.


#### 11.3.3 Array Layout

**Array Layout.** Arrays have a defined memory layout with no inter-element padding.

**Static Semantics**

**(T-Equiv-Array)**
$$\frac{\Gamma \vdash T \equiv U \quad N = M}{\Gamma \vdash [T; N] \equiv [U; M]}$$

**(Sub-Array)**
$$\frac{\Gamma \vdash T <: U \quad N = M}{\Gamma \vdash [T; N] <: [U; M]}$$

**Constraints**

**Layout**

$$\text{sizeof}([T; N]) = N \times \text{sizeof}(T)$$

$$\text{alignof}([T; N]) = \text{alignof}(T)$$

For zero-length arrays: $\text{sizeof}([T; 0]) = 0$ and $\text{alignof}([T; 0]) = \text{alignof}(T)$.

Implementations MUST NOT insert padding between array elements.


### 11.4 Slices


#### 11.4.1 Slice Definition

**Slice.** A dynamically-sized view into a contiguous sequence of elements. A slice does not own its data.

**Syntax**

```ebnf
slice_type ::= "[" type "]"
```

**Static Semantics**

$$\mathcal{T}_{\text{slice}} = \{[T] : T \in \mathcal{T}\}$$

**(T-Slice-Type)**
$$\frac{\Gamma \vdash T\ \text{wf}}{\Gamma \vdash [T]\ \text{wf}}$$

A slice value is a **dense pointer** consisting of a data pointer and a length.


#### 11.4.2 Slice Creation

**Slice Creation.** Slices are created by applying a range expression to an array or another slice.

**Static Semantics**

**(T-Slice-From-Array)**
$$\frac{\Gamma \vdash a : P\ [T; N] \quad \Gamma \vdash r : \text{Range}}{\Gamma \vdash a[r] : P\ [T]}$$

**(Coerce-Array-Slice)**
$$\frac{\Gamma \vdash a : P\ [T; N]}{\Gamma \vdash a : P\ [T]}$$

This coercion creates a slice viewing the entire array. Permission propagates per §10.1.

**Constraints**

**Diagnostics:** See Appendix A, code `E-TYP-1823`.


#### 11.4.3 Slice Bounds and Length

**Slice Bounds.** Slice indexing and sub-slicing are bounds-checked at runtime.

**Syntax**

```ebnf
slice_access ::= expression "[" expression "]"
slice_range  ::= expression "[" range_expression "]"
```

**Static Semantics**

**(T-Slice-Index)**
$$\frac{\Gamma \vdash s : [T] \quad \Gamma \vdash i : \texttt{usize}}{\Gamma \vdash s[i] : T}$$

**(T-Equiv-Slice)**
$$\frac{\Gamma \vdash T \equiv U}{\Gamma \vdash [T] \equiv [U]}$$

**(Sub-Slice)**
$$\frac{\Gamma \vdash T <: U}{\Gamma \vdash [T] <: [U]}$$

**Dynamic Semantics**

For a slice `s` of length $L$ and range `start..end`:
- `start` MUST satisfy $0 \leq \text{start} \leq L$
- `end` MUST satisfy $\text{start} \leq \text{end} \leq L$

Violation MUST cause panic.

**Constraints**

**Layout**

| Field | Type    | Description                     |
| :---- | :------ | :------------------------------ |
| `ptr` | `*T`    | Pointer to the first element    |
| `len` | `usize` | Number of elements in the slice |

$$\text{sizeof}([T]) = 2 \times \text{sizeof}(\texttt{usize})$$

$$\text{alignof}([T]) = \text{alignof}(\texttt{usize})$$

**Diagnostics:** See Appendix A, codes `E-TYP-1820`, `P-TYP-1822`.


### 11.5 Ranges


#### 11.5.1 Range Types (`Range`, `RangeFrom`, `RangeTo`, `RangeFull`)

**Range Types.** Intrinsic generic types representing bounded or unbounded intervals.

**Syntax**

```ebnf
range_expression    ::= exclusive_range | inclusive_range | from_range
                      | to_range | to_inclusive_range | full_range
exclusive_range     ::= logical_or_expr ".." logical_or_expr
inclusive_range     ::= logical_or_expr "..=" logical_or_expr
from_range          ::= logical_or_expr ".."
to_range            ::= ".." logical_or_expr
to_inclusive_range  ::= "..=" logical_or_expr
full_range          ::= ".."
```

**Static Semantics**

$$\mathcal{T}_{\text{range}} = \{\texttt{Range}\langle T \rangle, \texttt{RangeInclusive}\langle T \rangle, \texttt{RangeFrom}\langle T \rangle, \texttt{RangeTo}\langle T \rangle, \texttt{RangeToInclusive}\langle T \rangle, \texttt{RangeFull}\}$$

| Expression    | Type                  | Fields                             |
| :------------ | :-------------------- | :--------------------------------- |
| `start..end`  | `Range<T>`            | `public start: T`, `public end: T` |
| `start..=end` | `RangeInclusive<T>`   | `public start: T`, `public end: T` |
| `start..`     | `RangeFrom<T>`        | `public start: T`                  |
| `..end`       | `RangeTo<T>`          | `public end: T`                    |
| `..=end`      | `RangeToInclusive<T>` | `public end: T`                    |
| `..`          | `RangeFull`           | (none)                             |

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


#### 11.5.2 Range Bounds

**Range Bounds.** The semantics of inclusive vs. exclusive endpoints.

**Dynamic Semantics**

- `start..end` represents values $v$ where $\text{start} \leq v < \text{end}$
- `start..=end` represents values $v$ where $\text{start} \leq v \leq \text{end}$
- A range `start..end` where $\text{start} \geq \text{end}$ is **empty**
- A range `start..=end` where $\text{start} > \text{end}$ is **empty**

**Constraints**

**Diagnostics:** See Appendix A, code `E-TYP-1830`.


#### 11.5.3 Range Iteration

**Range Iteration.** Range types implement `Iterator` when the element type implements `Step`.

**Static Semantics**

**(T-Equiv-Range)**
$$\frac{\Gamma \vdash T \equiv U}{\Gamma \vdash \texttt{Range}\langle T \rangle \equiv \texttt{Range}\langle U \rangle}$$

**(Bitcopy-Range)**
$$\frac{T <: \texttt{Bitcopy}}{\texttt{Range}\langle T \rangle <: \texttt{Bitcopy}}$$

`RangeFull` unconditionally implements `Bitcopy`.

**Constraints**

**Layout**

| Type                  | Size                                         | Alignment           |
| :-------------------- | :------------------------------------------- | :------------------ |
| `Range<T>`            | $2 \times \text{sizeof}(T) + \text{padding}$ | $\text{alignof}(T)$ |
| `RangeInclusive<T>`   | $2 \times \text{sizeof}(T) + \text{padding}$ | $\text{alignof}(T)$ |
| `RangeFrom<T>`        | $\text{sizeof}(T)$                           | $\text{alignof}(T)$ |
| `RangeTo<T>`          | $\text{sizeof}(T)$                           | $\text{alignof}(T)$ |
| `RangeToInclusive<T>` | $\text{sizeof}(T)$                           | $\text{alignof}(T)$ |
| `RangeFull`           | $0$                                          | $1$                 |

**Diagnostics:** See Appendix A, code `E-TYP-1831`.


### 11.6 Records


#### 11.6.1 Record Declaration Syntax

**Record.** A nominal product type with named fields.

**Syntax**

```ebnf
record_decl        ::= visibility? "record" identifier generic_params?
                       implements_clause? "{" record_body "}" type_invariant?
record_body        ::= record_member ("," record_member)* ","?
record_member      ::= record_field_decl | method_def
record_field_decl  ::= visibility? identifier ":" type record_field_init?
record_field_init  ::= "=" expression
field_decl         ::= visibility? identifier ":" type
implements_clause ::= "<:" class_list
class_list       ::= type_path ("," type_path)*
```

**Static Semantics**

**(T-Record-WF)**
$$\frac{\forall i \in 1..n,\ \Gamma \vdash T_i\ \text{wf} \quad \forall i \neq j,\ f_i \neq f_j}{\Gamma \vdash \texttt{record}\ R\ \{f_1: T_1, \ldots, f_n: T_n\}\ \text{wf}}$$

Default field visibility is `private`. Field visibility MUST NOT exceed record visibility.

**Field Initializers**

Record field declarations MAY include an initializer expression. The initializer MUST be well-formed in the enclosing scope and its type MUST be a subtype of the field type (§9.2). Field initializers are evaluated only when a default record construction is performed (§11.6.5).

**Constraints**

**Diagnostics:** See Appendix A, codes `E-TYP-1901`, `E-TYP-1906`.


#### 11.6.2 Field Access

**Field Access.** Record fields are accessed using dot notation.

**Syntax**

```ebnf
field_access ::= postfix_expr "." identifier
```

**Static Semantics**

**(T-Field)**
$$\frac{\Gamma \vdash e : R \quad R.\text{fields}(f) = T \quad \text{visible}(f, \Gamma)}{\Gamma \vdash e.f : T}$$

Permission propagates from the record binding to field access per §10.1.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-TYP-1904`, `E-TYP-1905`.


#### 11.6.3 Record Construction

**Record Construction.** A record value is constructed using a literal specifying the type name and values for all fields.

**Syntax**

```ebnf
record_literal  ::= type_path "{" field_init_list "}"
field_init_list ::= field_init ("," field_init)* ","?
field_init      ::= identifier ":" expression | identifier
```

Field shorthand `{ field }` is equivalent to `{ field: field }`.

**Static Semantics**

**(T-Record-Lit)**
$$\frac{\forall i \in 1..n,\ \Gamma \vdash e_i : T_i \quad R = \{f_1: T_1, \ldots, f_n: T_n\}}{\Gamma \vdash R\ \{f_1: e_1, \ldots, f_n: e_n\} : R}$$

**Constraints**

**Diagnostics:** See Appendix A, codes `E-TYP-1902`, `E-TYP-1903`, `P-TYP-1909`, `P-TYP-1910`.


#### 11.6.4 Record Update Syntax

**Record Update.** Constructs a new record value by copying fields from an existing record and overriding specified fields with new values.

**Syntax**

```ebnf
record_update   ::= type_path "from" expression "{" field_init_list "}"
```

```cursive
let updated = Config from old_config { timeout: 60 }
let moved = Player from move old_player { health: 100 }
```

**Static Semantics**

**(T-Record-Update)**

$$\frac{
  \Gamma \vdash e_{\text{src}} : R \quad
  \{f_1, \ldots, f_k\} \subseteq \text{fields}(R) \quad
  \forall i \in 1..k,\ \Gamma \vdash e_i : T_{f_i}
}{
  \Gamma \vdash R\ \texttt{from}\ e_{\text{src}}\ \{f_1: e_1, \ldots, f_k: e_k\} : R
}$$

**Type Constraint**

The source expression type MUST equal the constructed record type. The type path $R$ in the construction MUST match the type of $e_{\text{src}}$.

**Field Override**

Explicitly provided fields override the corresponding fields from the source. Fields not explicitly provided are copied from the source.

**Bitcopy vs. Move**

For each field $f$ in $R$ not explicitly overridden:
- If $T_f$ implements `Bitcopy`: the field is implicitly copied.
- If $T_f$ does not implement `Bitcopy`: the source expression MUST be a move expression (`move` applied to a place expression).

When the source expression is a move expression, the source binding transitions to the Moved state (§3.3.4).

**Visibility**

Fields copied implicitly via record update MUST be visible in the current scope. Private fields not visible at the update site prevent the update.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-TYP-1906`–`E-TYP-1908`.


#### 11.6.5 Default Record Construction

**Default Construction.** A record with default-initialized fields MAY be constructed by calling the record name with no arguments.

**Static Semantics**

Define $\text{default\_constructible}(R)$ iff every field in record $R$ has a field initializer.

**(T-Record-Default)**
$$\frac{\Gamma \vdash R\ \texttt{record} \quad \text{default\_constructible}(R)}{\Gamma \vdash R() : R}$$

**Evaluation Order**

Default field initializers are evaluated in declaration order. Each initializer expression is evaluated in the caller's context and MUST NOT assume the presence of `self` or other record fields in scope. The resulting values are used to construct the record instance.

**Constraints**

**Diagnostics:** See Appendix A, code `E-TYP-1911`.


#### 11.6.6 Nominal Equivalence

**Nominal Equivalence.** Record types use nominal equivalence. See §9.1.1.

**Static Semantics**

**(T-Equiv-Record)**
$$\frac{D(R_1) = D(R_2)}{\Gamma \vdash R_1 \equiv R_2}$$

Two records with identical structure but different names are distinct types.

**(Sub-Record-Class)**
$$\frac{R\ \texttt{<:}\ \textit{Cl}}{\Gamma \vdash R <: \textit{Cl}}$$

**Constraints**

**Layout**

$$\text{alignof}(R) = \max_{f \in \text{fields}(R)}(\text{alignof}(T_f))$$

$$\text{sizeof}(R) \geq \sum_{f \in \text{fields}(R)} \text{sizeof}(T_f)$$

A record with no fields: $\text{sizeof}(R) = 0$ and $\text{alignof}(R) = 1$.


### 11.7 Enums


#### 11.7.1 Enum Declaration Syntax

**Enum.** A nominal sum type with named variants, each optionally carrying a payload.

**Syntax**

```ebnf
enum_decl       ::= visibility? "enum" identifier generic_params?
                    implements_clause? "{" variant_list "}" type_invariant?
variant_list    ::= variant ("," variant)* ","?
variant         ::= identifier variant_payload? ("=" integer_literal)?
variant_payload ::= "(" type_list ")" | "{" field_decl_list "}"
type_list       ::= type ("," type)* ","?
field_decl_list ::= field_decl ("," field_decl)* ","?
```

**Static Semantics**

**(T-Enum-WF)**
$$\frac{\forall i \in 1..n,\ \Gamma \vdash P_i\ \text{wf} \quad \forall i \neq j,\ v_i \neq v_j}{\Gamma \vdash \texttt{enum}\ E\ \{v_1(P_1), \ldots, v_n(P_n)\}\ \text{wf}}$$

**Constraints**

**Diagnostics:** See Appendix A, codes `E-TYP-2001`, `E-TYP-2002`, `E-TYP-2006`.


#### 11.7.2 Variant Payloads (Unit, Tuple, Record)

**Variant Payloads.** Variants may take one of three forms.

| Form        | Syntax                       | Payload Structure                    |
| :---------- | :--------------------------- | :----------------------------------- |
| Unit-like   | `Variant`                    | No associated data                   |
| Tuple-like  | `Variant(T₁, T₂, ...)`       | Positional fields, accessed by index |
| Record-like | `Variant { f₁: T₁, f₂: T₂ }` | Named fields, accessed by name       |

**Syntax**

```ebnf
enum_literal ::= type_path "::" identifier variant_args?
variant_args ::= "(" expression_list ")" | "{" field_init_list "}"
```

**Static Semantics**

**(T-Variant-Unit)**
$$\frac{E \text{ declares variant } V \text{ with no payload}}{\Gamma \vdash E::V : E}$$

**(T-Variant-Tuple)**
$$\frac{E \text{ declares variant } V(T_1, \ldots, T_n) \quad \forall i,\ \Gamma \vdash e_i : T_i}{\Gamma \vdash E::V(e_1, \ldots, e_n) : E}$$

**(T-Variant-Record)**
$$\frac{E \text{ declares variant } V\{f_1: T_1, \ldots, f_n: T_n\} \quad \forall i,\ \Gamma \vdash e_i : T_i}{\Gamma \vdash E::V\{f_1: e_1, \ldots, f_n: e_n\} : E}$$

**Constraints**

**Diagnostics:** See Appendix A, codes `E-TYP-2007`–`E-TYP-2009`.


#### 11.7.3 Enum Discriminants

**Discriminant.** A compile-time-assigned integer value that uniquely identifies each variant.

**Static Semantics**

Discriminant assignment rules:
1. Explicit discriminant: specified value is assigned
2. First variant without explicit discriminant: assigned `0`
3. Subsequent variants without explicit discriminant: previous discriminant plus one

Discriminant values MUST be unique within an enum. The discriminant type is IDB.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-TYP-2003`, `E-TYP-2004`, `E-TYP-2010`.


#### 11.7.4 Enum Pattern Matching

**Enum Pattern Matching.** Accessing enum variant data MUST be performed using a `match` expression. Direct field access is forbidden.

**Static Semantics**

**(T-Equiv-Enum)**
$$\frac{D(E_1) = D(E_2)}{\Gamma \vdash E_1 \equiv E_2}$$

**(Sub-Enum-Class)**
$$\frac{E\ \texttt{<:}\ \textit{Cl}}{\Gamma \vdash E <: \textit{Cl}}$$

A `match` expression on an enum MUST be exhaustive. See §15.8.4 for exhaustiveness requirements.

**Constraints**

**Layout**

$$\text{sizeof}(E) = \text{sizeof}(\text{Discriminant}) + \max_{v \in \text{Variants}}(\text{sizeof}(P_v)) + \text{Padding}$$

$$\text{alignof}(E) = \max(\text{alignof}(\text{Discriminant}), \max_{v \in \text{Variants}}(\text{alignof}(P_v)))$$

**Niche Optimization**

Implementations MUST apply niche optimization when variant payloads contain invalid bit patterns that can encode the discriminant.

**Diagnostics:** See Appendix A, codes `E-TYP-2005`, `P-TYP-2011`, `P-TYP-2012`.


### 11.8 Union Types


#### 11.8.1 Union Type Syntax

**Union Type.** A structural anonymous sum type representing a value that may be one of several distinct types.

**Syntax**

```ebnf
union_type     ::= non_union_type ("|" non_union_type)+
non_union_type ::= primitive_type | nominal_type | tuple_type
                 | array_type | slice_type | function_type
                 | pointer_type | "(" union_type ")"
```

**Static Semantics**

A union type $U$ is defined by a non-empty multiset of member types:

$$U = \bigcup_{i=1}^{n} T_i \quad \text{where } n \geq 2$$

**(T-Union-WF)**
$$\frac{n \geq 2 \quad \forall i \in 1..n,\ \Gamma \vdash T_i\ \text{wf}}{\Gamma \vdash T_1 \mid T_2 \mid \cdots \mid T_n\ \text{wf}}$$

**Precedence**

The `|` operator has lower precedence than function type arrows (`->`), array brackets, and pointer constructors.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-TYP-2201`, `W-TYP-2201`.


#### 11.8.2 Structural Sum Types

**Structural Sum Types.** Union types use structural equivalence. Order is irrelevant; duplicates are preserved.

**Static Semantics**

The following properties follow from multiset equivalence (§9.1.3):

**(Union-Commutativity)**
$$\Gamma \vdash (A \mid B) \equiv (B \mid A)$$

**(Union-Non-Collapse)**
$$\Gamma \vdash (A \mid A) \not\equiv A$$

**(T-Union-Intro)**
$$\frac{\Gamma \vdash e : T \quad T \in_U U}{\Gamma \vdash e : U}$$

**(T-Union-Match)**
$$\frac{\Gamma \vdash e : U \quad U = T_1 \mid \cdots \mid T_n \quad \forall i,\ \Gamma, x_i : T_i \vdash e_i : R}{\Gamma \vdash \texttt{match } e\ \{x_1 : T_1 \Rightarrow e_1, \ldots, x_n : T_n \Rightarrow e_n\} : R}$$

**(Sub-Member-Union)**
$$\frac{T \in_U U}{\Gamma \vdash T <: U}$$

**(Sub-Union-Width)**
$$\frac{\forall T \in_U U_1 : T \in_U U_2}{\Gamma \vdash U_1 <: U_2}$$

Direct field access on union values is forbidden. Pattern matching is required.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-TYP-2202`, `E-TYP-2203`, `E-SEM-2705`.


#### 11.8.3 Propagation Operator (`?`)

**Propagation Operator.** The `?` operator provides concise syntax for early return of error-like union members.

**Syntax**

```ebnf
try_expr ::= postfix_expr "?"
```

**Static Semantics**

**(T-Try-Union)**
$$\frac{
    \Gamma \vdash e : U \quad
    U = T_s \mid T_{e_1} \mid \cdots \mid T_{e_k} \quad
    \forall i \in 1..k,\ T_{e_i} <: R \quad
    T_s \not<: R
}{
    \Gamma \vdash e? : T_s
}$$

where $R$ is the return type of the enclosing procedure and $T_s$ is the success type.

**Success Type Inference**

1. Exactly one member type NOT a subtype of the return type is the success type
2. Multiple candidates: ill-formed (ambiguous propagation)
3. No candidates: ill-formed (all types propagate)

**Dynamic Semantics**

$$e? \quad \rightarrow \quad \texttt{match}\ e\ \{\ s: T_s \Rightarrow s,\ e: T_e \Rightarrow \texttt{return}\ e\ \}$$

**Constraints**

**Diagnostics:** See Appendix A, codes `E-TYP-2210`–`E-TYP-2214`.


#### 11.8.4 Union Flattening

**Union Flattening.** Nested unions are NOT automatically flattened.

**Static Semantics**

$$\Gamma \vdash ((A \mid B) \mid C) \not\equiv (A \mid B \mid C)$$

To match `(A | B) | C`, the outer union must first be matched, then the inner union if applicable.

**Constraints**

**Layout**

$$\text{sizeof}(U) = \text{sizeof}(\text{Discriminant}) + \max_{T \in_U U}(\text{sizeof}(T)) + \text{Padding}$$

$$\text{alignof}(U) = \max(\text{alignof}(\text{Discriminant}), \max_{T \in_U U}(\text{alignof}(T)))$$

**Discriminant Encoding**

Discriminant values are assigned based on canonical ordering (lexicographic by fully-qualified type name).

| Member Count | Minimum Discriminant Size |
| :----------- | :------------------------ |
| 2–256        | 1 byte (`u8`)             |
| 257–65,536   | 2 bytes (`u16`)           |
| > 65,536     | 4 bytes (`u32`)           |

**Niche Optimization**

Niche optimization MUST be applied to union types.

**Diagnostics:** See Appendix A, code `E-TYP-2204`.

---

## 12. Behavioral Types

---

### 12.1 Modal Types


#### 12.1.1 Modal Type Declaration

**Modal Type.** A nominal type embedding a compile-time-validated state machine. A modal type defines a family of related types:

1. **State-Specific Types (`M@S`)**: Concrete types containing only the data defined in their state's payload. No runtime discriminant; state is tracked statically.

2. **General Modal Type (`M`)**: A sum type (tagged union) holding a value in any declared state. Stores payload plus runtime discriminant.

**Formal Definition**

Let $\mathcal{M}$ denote the set of modal type names and $\text{States}(M)$ the finite, non-empty set of state names for modal $M$. For each $M \in \mathcal{M}$ and $S \in \text{States}(M)$:

- $M@S$ denotes the state-specific type for state $S$
- $M$ denotes the general modal type

The type family induced by a modal declaration:

$$\mathcal{T}_M = \{M\} \cup \{M@S : S \in \text{States}(M)\}$$

**Syntax**

```ebnf
modal_decl        ::= visibility? "modal" identifier generic_params?
                      implements_clause? "{" state_block+ "}"

state_block       ::= "@" state_name "{" state_member* "}"

state_name        ::= identifier

state_member      ::= state_field_decl | state_method_def | transition_def

modal_type_name     ::= type_path generic_args?
state_specific_type ::= modal_type_name "@" state_name
```

**Static Semantics**

**(Modal-WF)** Well-Formedness:

$$\frac{n \geq 1 \quad \forall i \in 1..n,\ S_i \text{ unique} \quad \forall i,\ \text{Payload}(S_i) \text{ wf}}{\Gamma \vdash \texttt{modal } M\ \{@S_1\ \ldots\ @S_n\}\ \text{wf}}$$

**(State-Specific-WF)** State-Specific Type Formation:

$$\frac{S \in \text{States}(M)}{\Gamma \vdash M@S\ \text{wf}}$$

**(T-Modal-State-Intro)** State-Specific Value Construction:

$$\frac{M@S \text{ has payload fields } f_1 : T_1, \ldots, f_k : T_k \quad \forall i,\ \Gamma \vdash e_i : T_i}{\Gamma \vdash M@S\ \{f_1: e_1, \ldots, f_k: e_k\} : M@S}$$

**(Modal-Incomparable)** Incomparability:

$$\frac{S_A \neq S_B}{\Gamma \vdash M@S_A \not<: M@S_B \quad \land \quad \Gamma \vdash M@S_B \not<: M@S_A}$$

**(Modal-Class-Sub)** Modal Class Subtyping:

$$\frac{M \text{ is a modal type} \quad M <: Cl \quad Cl \text{ is a modal class}}{\Gamma \vdash M <: Cl}$$

**Dynamic Semantics**

**State-Specific Type Layout**

$$\text{layout}(M@S) \equiv \text{layout}(\texttt{record}\ \{\text{Payload}(S)\})$$

$$\text{Payload}(S) = \emptyset \implies \text{sizeof}(M@S) = 0$$

**General Modal Type Layout**

$$\text{layout}(M) \equiv \text{layout}(\texttt{enum}\ \{S_1(\text{Payload}(S_1)),\ \ldots,\ S_n(\text{Payload}(S_n))\})$$

$$\text{sizeof}(M) = \text{sizeof}(\text{Discriminant}) + \max_{S \in \text{States}(M)}(\text{sizeof}(M@S)) + \text{Padding}$$

$$\text{alignof}(M) = \max(\text{alignof}(\text{Discriminant}),\ \max_{S \in \text{States}(M)}(\text{alignof}(M@S)))$$

Niche optimization (see §11.7) MUST be applied to modal types.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-TYP-2050`, `E-TYP-2051`, `E-TYP-2054`.


#### 12.1.2 State Definitions

**State Block.** Defines a named state within a modal type, containing payload fields, methods, and transitions.

**Syntax**
*[REF: State block syntax is defined in §12.1.1.]*

**Static Semantics**

1. A `modal` declaration MUST contain at least one state block.
2. All state names within a single `modal` declaration MUST be unique.
3. State names MUST NOT collide with the modal type name itself.


#### 12.1.3 State-Specific Fields

**Payload Fields.** Data members declared within a state block.

**Syntax**

```ebnf
state_field_decl        ::= identifier ":" type
```

**Static Semantics**

**(T-Modal-Field)** State Payload Field Access:

$$\frac{\Gamma \vdash e : M@S \quad f \in \text{Payload}(S) \quad \text{Payload}(S).f : T}{\Gamma \vdash e.f : T}$$

Field access through a general modal type $M$ or a different state-specific type $M@S'$ where $S' \neq S$ is ill-formed.

**Visibility**

Fields declared within a state payload are implicitly `protected`. Accessible only from:

1. Procedures declared within the same modal type's state blocks
2. Transition implementations for the modal type
3. Associated class implementations for the modal type

External code MUST NOT directly access payload fields.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-TYP-2052`, `E-TYP-2057`.


#### 12.1.4 Transitions

**State Transition.** A procedure that consumes a value of one state-specific type and produces a value of another (or the same) state-specific type.

$$\text{Transitions}(M) \subseteq \text{States}(M) \times \text{States}(M)$$

**Syntax**

```ebnf
transition_def    ::= "transition" identifier "(" param_list ")" "->" "@" target_state block

target_state      ::= identifier
```

**Static Semantics**

**Transition Resolution**

A `transition` defined in state `@S` targeting `@T` has:
- Implicit receiver type: `Self` = `M@S`
- Return type: `M@T`

**(T-Modal-Transition)** Transition Typing:

$$\frac{\Gamma \vdash e_{\text{self}} : P_{\text{src}}\ M@S_{\text{src}} \quad (S_{\text{src}}, S_{\text{tgt}}) \in \text{Transitions}(M) \quad \forall i,\ \Gamma \vdash a_i : T_i}{\Gamma \vdash e_{\text{self}} \mathord{\sim>} t(a_1, \ldots, a_n) : M@S_{\text{tgt}}}$$

where $P_{\text{src}}$ MUST be `unique` for consuming transitions.

**Body Constraint**

$$\frac{\Gamma \vdash \text{body} : M@S_{\text{tgt}}}{\text{transition } M\text{::}t(\ldots) \to M@S_{\text{tgt}}\ \{\text{body}\}\ \text{well-typed}}$$

**Dynamic Semantics**

When a transition procedure is invoked:

1. The receiver value (in the source state) is consumed
2. The transition body executes, constructing a new value of the target state
3. The resulting value is returned with the target state-specific type

After a consuming transition, the original binding is statically invalid.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-TYP-2055`, `E-TYP-2056`.


#### 12.1.5 State-Specific Methods

**State-Specific Methods.** Procedures declared within a state block, callable only when the receiver has the corresponding state-specific type.

**Syntax**

```ebnf
state_method_def        ::= "procedure" identifier "(" param_list ")" ("->" return_type)? block
```

Methods and transitions MUST be defined inline within their state block.

**Static Semantics**

**(T-Modal-Method)** State-Specific Method Invocation:

$$\frac{\Gamma \vdash e : M@S \quad m \in \text{Methods}(S) \quad m : (M@S, T_1, \ldots, T_n) \to R}{\Gamma \vdash e \mathord{\sim>} m(a_1, \ldots, a_n) : R}$$

Within `state_members`, the `self` parameter type is implicitly the enclosing state-specific type.

**Constraints**

**Diagnostics:** See Appendix A, code `E-TYP-2053`.


#### 12.1.6 Modal Pattern Matching

**Modal Pattern Matching.** Pattern matching narrows a general modal type to a state-specific type.

**Syntax**
*[REF: Modal patterns are defined in §15.7.1 (Pattern Syntax).]*

When a field pattern omits the `: pattern` suffix, the field name binds a variable of the field's type.

**Dynamic Semantics**

**Match on General Type**

When the scrutinee has general modal type $M$:

1. The runtime discriminant (or niche encoding) is inspected
2. Control transfers to the arm matching the active state
3. Within that arm, the bound variable has the corresponding state-specific type

The match MUST be exhaustive: all declared states MUST be covered.

**Match on State-Specific Type**

When the scrutinee has state-specific type $M@S$:

The state is statically known. Match is treated as irrefutable payload destructuring. Coverage of other states is neither required nor permitted.

**Constraints**

**Diagnostics:** See Appendix A, code `E-TYP-2060`.


#### 12.1.7 Widening (`widen`)

**Modal Widening.** Converts a state-specific type `M@S` to the general modal type `M`.

**Syntax**

```ebnf
widen_expr ::= "widen" expression
```

**Static Semantics**

**(T-Modal-Widen)**

$$\frac{\Gamma \vdash e : M@S \quad S \in \text{States}(M)}{\Gamma \vdash \texttt{widen } e : M}$$

**Widening Rules**

| Type Category               | `widen` Keyword | Implicit Coercion | Subtyping      |
| :-------------------------- | :-------------- | :---------------- | :------------- |
| Non-niche-layout-compatible | Required        | Prohibited        | $M@S \not<: M$ |
| Niche-layout-compatible     | Optional        | Permitted         | $M@S <: M$     |

**Niche-Layout-Compatible Conditions**

A state-specific type $M@S$ is niche-layout-compatible with $M$ when:

1. Niche encoding applies (unused bit pattern serves as discriminant)
2. $\text{sizeof}(M@S) = \text{sizeof}(M)$
3. $\text{alignof}(M@S) = \text{alignof}(M)$
4. No additional storage required for state tag

**Dynamic Semantics**

When `widen e` is evaluated where $e : M@S$:

1. Storage for the general representation is allocated
2. The payload from $e$ is moved into the general representation
3. The discriminant (or niche encoding) for state $@S$ is written
4. The resulting value has type $M$

**Size Relationship**

$\text{sizeof}(M@S) \leq \text{sizeof}(M)$

**Narrowing**

Narrowing from $M$ to $M@S$ requires explicit pattern matching (not implicit coercion).

**Constraints**

**Diagnostics:** See Appendix A, codes `E-TYP-2070`–`E-TYP-2072`, `W-SYS-4010`.

The threshold for `W-SYS-4010` is Implementation-Defined (default: 256 bytes).


### 12.2 String and Bytes Types

> See §12.1 for modal type semantics. This section defines string- and bytes-specific states and operations.


#### 12.2.1 String States (`@Managed`, `@View`)

**String Type.** A built-in modal type representing UTF-8 encoded Unicode scalar values.

$$\text{States}(\texttt{string}) = \{\ \texttt{@Managed},\ \texttt{@View}\ \}$$

| State            | Description                                  | Ownership |
| :--------------- | :------------------------------------------- | :-------- |
| `string@Managed` | Owned, heap-allocated, mutable string buffer | Owning    |
| `string@View`    | Non-owning, immutable slice into string data | Borrowing |

**Syntax**

```ebnf
string_type         ::= "string" ("@" string_state)?
bytes_type          ::= "bytes" ("@" bytes_state)?

string_state        ::= "Managed" | "View"
bytes_state         ::= "Managed" | "View"
```

**Static Semantics**

**Class Implementations**

| State            | `Bitcopy` | `Clone` | `Drop` |
| :--------------- | :----- | :------ | :----- |
| `string@Managed` | No     | No      | Yes    |
| `string@View`    | Yes    | Yes     | No     |

1. `string@Managed` MUST NOT implement `Bitcopy` (owns heap allocation)
2. `string@Managed` MUST NOT implement `Clone` (use `clone_with(heap)`)
3. `string@Managed` MUST implement `Drop` (deallocates buffer)
4. `string@View` MUST implement `Bitcopy` (non-owning pointer-length pair)

`string@Managed` values capture allocator metadata derived from the `$HeapAllocator` used at construction time. This metadata is used by `Drop::drop` to deallocate the buffer. All operations that allocate or resize MUST still receive an explicit `$HeapAllocator` capability parameter. Implementations MAY store allocator metadata out‑of‑line. The metadata is not observable by programs and does not affect the fixed three‑word layout shown below.

**Modal Widening**

$$\frac{S \in \{\texttt{@Managed}, \texttt{@View}\}}{\Gamma \vdash \texttt{string@}S <: \texttt{string}}$$

**Dynamic Semantics**

**`string@Managed` Representation**

| Field      | Type            | Offset  | Description                      |
| :--------- | :-------------- | :------ | :------------------------------- |
| `pointer`  | `Ptr<u8>@Valid` | 0       | Pointer to heap-allocated buffer |
| `length`   | `usize`         | 1 word  | Bytes of valid content           |
| `capacity` | `usize`         | 2 words | Total allocated buffer size      |

$$\text{sizeof}(\texttt{string@Managed}) = 3 \times \text{sizeof}(\texttt{usize})$$

**`string@View` Representation**

| Field     | Type                  | Offset | Description             |
| :-------- | :-------------------- | :----- | :---------------------- |
| `pointer` | `Ptr<const u8>@Valid` | 0      | Pointer to first byte   |
| `length`  | `usize`               | 1 word | Number of bytes in view |

$$\text{sizeof}(\texttt{string@View}) = 2 \times \text{sizeof}(\texttt{usize})$$


#### 12.2.2 String Literal Type Semantics

*[REF: String literal token syntax is defined in §2.6.3. This section specifies type semantics.]*

**String Literal.** Produces a `string@View` value referencing static, read-only memory.

**Static Semantics**

**(T-String-Literal)**

$$\frac{\Gamma \vdash s \text{ is a valid string literal}}{\Gamma \vdash s : \texttt{string@View}}$$

**Dynamic Semantics**

When a string literal is evaluated:

1. Literal content is allocated in static, read-only memory at compilation
2. A `string@View` value is constructed with pointer to static memory and byte length

String literals have static storage duration; backing memory is never deallocated.


#### 12.2.3 String Conversions

**String Conversions.** Conversion between `string@Managed` and `string@View` requires explicit operations.

**Dynamic Semantics**

**Managed to View (`as_view`)**

```cursive
procedure as_view(self: const string@Managed) -> string@View
```

Complexity: $O(1)$. The resulting view aliases the managed string's storage.

**View to Managed (`to_managed`)**

```cursive
procedure to_managed(self: const string@View, heap: $HeapAllocator) -> string@Managed
```

Complexity: $O(n)$ where $n$ is byte length.

**`string@Managed` Operations**

Operations requiring allocation MUST receive a `$HeapAllocator` capability:

- `string::from(source: string@View, heap: $HeapAllocator) -> string@Managed`
- `append(self: unique, data: string@View, heap: $HeapAllocator)`
- `clone_with(self: const, heap: $HeapAllocator) -> string@Managed`


#### 12.2.4 String Operations

**String Operations.** Slicing, length queries, and character iteration.

**Static Semantics**

**(T-String-Slice)**

$$\frac{\Gamma \vdash e : \texttt{string@}S \quad S \in \{\texttt{Managed}, \texttt{View}\} \quad \Gamma \vdash a : \texttt{usize} \quad \Gamma \vdash b : \texttt{usize}}{\Gamma \vdash e[a..b] : \texttt{string@View}}$$

**Indexing Prohibition**

$$\Gamma \vdash e : \texttt{string@}S \implies e[i] \text{ is ill-formed}$$

Direct byte indexing using single index is forbidden.

**Dynamic Semantics**

**`string@View` Methods**

- `length(self: const) -> usize` — Returns byte length
- `is_empty(self: const) -> bool` — Returns `true` if length is zero
- `chars(self: const) -> CharIterator` — Iterator over Unicode scalar values

**Slicing Evaluation**

When `s[a..b]` is evaluated:

1. If `a > b`, panic (`E-TYP-2153`)
2. If `b > s~>length()`, panic (`E-TYP-2154`)
3. If `a` or `b` not on UTF-8 character boundary, panic (`E-TYP-2151`)
4. Construct `string@View` with pointer `s.pointer + a` and length `b - a`

**UTF-8 Character Boundary**

Byte offset `i` is a valid character boundary iff:

1. `i == 0`, OR
2. `i == length`, OR
3. Byte at offset `i` is NOT a UTF-8 continuation byte (high bits NOT `10`)

**Constraints**

**Diagnostics:** See Appendix A, codes `E-TYP-2151`–`E-TYP-2154`.



#### 12.2.5 Byte Buffer Types (`bytes`)

**Bytes Type.** A built-in modal type representing an uninterpreted sequence of bytes.

$$\text{States}(\texttt{bytes}) = \{\ \texttt{@Managed},\ \texttt{@View}\ \}$$

| State           | Description                                 | Ownership |
| :-------------- | :------------------------------------------ | :-------- |
| `bytes@Managed` | Owned, heap-allocated, mutable byte buffer  | Owning    |
| `bytes@View`    | Non-owning, immutable slice of raw bytes    | Borrowing |

**Syntax**

```ebnf
bytes_type         ::= "bytes" ("@" bytes_state)?

bytes_state        ::= "Managed" | "View"
```

**Static Semantics**

**Class Implementations**

| State           | `Bitcopy` | `Clone` | `Drop` |
| :-------------- | :----- | :------ | :----- |
| `bytes@Managed` | No     | No      | Yes    |
| `bytes@View`    | Yes    | Yes     | No     |

1. `bytes@Managed` MUST NOT implement `Bitcopy` (owns heap allocation)
2. `bytes@Managed` MUST NOT implement `Clone`
3. `bytes@Managed` MUST implement `Drop` (deallocates buffer)
4. `bytes@View` MUST implement `Bitcopy` (non-owning pointer-length pair)

`bytes@Managed` values capture allocator metadata derived from the `$HeapAllocator` used at construction time. The metadata is used by `Drop::drop` to deallocate the buffer. All operations that allocate or resize MUST receive an explicit `$HeapAllocator` capability parameter. Implementations MAY store allocator metadata out-of-line. The metadata is not observable by programs and does not affect the fixed three-word layout shown below.

**Modal Widening**

$$\frac{S \in \{\texttt{@Managed}, \texttt{@View}\}}{\Gamma \vdash \texttt{bytes@}S <: \texttt{bytes}}$$

**Dynamic Semantics**

**`bytes@Managed` Representation**

| Field      | Type            | Offset  | Description                      |
| :--------- | :-------------- | :------ | :------------------------------- |
| `pointer`  | `Ptr<u8>@Valid` | 0       | Pointer to heap-allocated buffer |
| `length`   | `usize`         | 1 word  | Bytes of valid content           |
| `capacity` | `usize`         | 2 words | Total allocated buffer size      |

$$\text{sizeof}(\texttt{bytes@Managed}) = 3 \times \text{sizeof}(\texttt{usize})$$

**`bytes@View` Representation**

| Field     | Type                  | Offset | Description             |
| :-------- | :-------------------- | :----- | :---------------------- |
| `pointer` | `Ptr<const u8>@Valid` | 0      | Pointer to first byte   |
| `length`  | `usize`               | 1 word | Number of bytes in view |

$$\text{sizeof}(\texttt{bytes@View}) = 2 \times \text{sizeof}(\texttt{usize})$$

**Bytes Conversions**

**Managed to View (`as_view`)**

```cursive
procedure as_view(self: const bytes@Managed) -> bytes@View
```

Complexity: $O(1)$. The resulting view aliases the managed buffer.

**View to Managed (`to_managed`)**

```cursive
procedure to_managed(self: const bytes@View, heap: $HeapAllocator) -> bytes@Managed | AllocationError
```

Complexity: $O(n)$ where $n$ is byte length.

**Slice to View (`view`)**

```cursive
procedure view(data: const [u8]) -> bytes@View
```

Complexity: $O(1)$. The resulting view aliases the slice.

**String to View (`view_string`)**

```cursive
procedure view_string(data: string@View) -> bytes@View
```

Complexity: $O(1)$. The resulting view aliases the UTF-8 bytes of the string.

**Bytes Operations**

Operations requiring allocation MUST receive a `$HeapAllocator` capability:

- `bytes::with_capacity(cap: usize, heap: $HeapAllocator) -> bytes@Managed | AllocationError`
- `bytes::from_slice(data: const [u8], heap: $HeapAllocator) -> bytes@Managed | AllocationError`
- `append(self: unique, data: bytes@View, heap: $HeapAllocator) -> () | AllocationError`

**`bytes@View` Methods**

- `length(self: const) -> usize` — Returns byte length
- `is_empty(self: const) -> bool` — Returns `true` if length is zero

### 12.3 Pointer Types

> See §12.1 for modal type semantics and §3.2 for provenance and lifetime rules. This section defines pointer-specific states and operations.


#### 12.3.1 Pointer States (`@Valid`, `@Null`, `@Expired`)

**Ptr Type.** A built-in modal type for safe pointers with compile-time-tracked states.

$$\text{States}(\texttt{Ptr<T>}) = \{\ \texttt{@Valid},\ \texttt{@Null},\ \texttt{@Expired}\ \}$$

| State      | Description                                  | Dereferenceable |
| :--------- | :------------------------------------------- | :-------------- |
| `@Valid`   | Non-null, points to live, accessible memory  | Yes             |
| `@Null`    | Guaranteed null (address `0x0`)              | No              |
| `@Expired` | Was valid, now references deallocated memory | No              |

**Raw Pointer Types**

$$\mathcal{T}_{\text{RawPtr}} = \{\ \texttt{*imm T},\ \texttt{*mut T}\ :\ T \in \mathcal{T}\ \}$$

Raw pointers provide no safety guarantees; intended for `unsafe` blocks and FFI.

**Syntax**

```ebnf
safe_pointer_type     ::= "Ptr" "<" type ">" ("@" pointer_state)?

pointer_state         ::= "Valid" | "Null" | "Expired"

raw_pointer_type      ::= "*" raw_pointer_qual type

raw_pointer_qual      ::= "imm" | "mut"
```

**Static Semantics**

**Class Implementations**

| Type             | `Bitcopy` | `Clone` | `Drop` |
| :--------------- | :----- | :------ | :----- |
| `Ptr<T>@Valid`   | Yes    | Yes     | No     |
| `Ptr<T>@Null`    | Yes    | Yes     | No     |
| `Ptr<T>@Expired` | Yes    | Yes     | No     |
| `*imm T`         | Yes    | Yes     | No     |
| `*mut T`         | Yes    | Yes     | No     |

All pointer types implement `Bitcopy`. No pointer type implements `Drop`; pointers do not own referenced memory.

**Modal Subtyping (Niche-Layout-Compatible)**

$$\frac{S \in \{\texttt{@Valid}, \texttt{@Null}\}}{\Gamma \vdash \texttt{Ptr<T>@}S <: \texttt{Ptr<T>}}$$

**Dynamic Semantics**

**Safe Pointer Representation**

| State      | Representation         |
| :--------- | :--------------------- |
| `@Valid`   | Non-zero address       |
| `@Null`    | Address `0x0`          |
| `@Expired` | Formerly-valid address |

$$\text{sizeof}(\texttt{Ptr<T>}) = \text{sizeof}(\texttt{usize})$$

The `@Expired` state has no runtime representation; it is tracked purely statically.

**Raw Pointer Representation**

$$\text{sizeof}(\texttt{*imm T}) = \text{sizeof}(\texttt{*mut T}) = \text{sizeof}(\texttt{usize})$$

The `imm`/`mut` distinction is a compile-time property only.


#### 12.3.2 Pointer Dereferencing

**Pointer Dereferencing.** The dereference operator `*` reads the value at a pointer's address.

**Static Semantics**

**(T-Deref)** Safe Pointer Dereference:

$$\frac{\Gamma \vdash p : \texttt{Ptr<T>@Valid}}{\Gamma \vdash \texttt{*}p : T}$$

**(T-Raw-Deref-Imm)** Unsafe Raw Pointer Dereference:

$$\frac{\Gamma \vdash p : \texttt{*imm T} \quad \text{context is } \texttt{unsafe}}{\Gamma \vdash \texttt{*}p : T}$$

**(T-Raw-Deref-Mut)**

$$\frac{\Gamma \vdash p : \texttt{*mut T} \quad \text{context is } \texttt{unsafe}}{\Gamma \vdash \texttt{*}p : T}$$

**Dynamic Semantics**

**Safe Dereference**

When `*p` is evaluated for `p : Ptr<T>@Valid`:

1. Read the value of type `T` stored at address `p`
2. Return this value

**Unsafe Dereference**

When `*p` is evaluated for a raw pointer within `unsafe`:

1. Read the value of type `T` stored at address `p`
2. No safety guarantees; if `p` is null, dangling, misaligned, or uninitialized, behavior is Unverifiable (UVB)

**Constraints**

**Diagnostics:** See Appendix A, codes `E-TYP-2101`–`E-TYP-2103`.


#### 12.3.3 Null Safety

**Null Safety.** Static null safety is enforced through the modal type system.

**Static Semantics**

**(T-Addr-Of)** Address-Of Operator:

$$\frac{\Gamma \vdash e : T \quad e\ \text{is a valid place expression}}{\Gamma \vdash \texttt{\&}e : \texttt{Ptr<T>@Valid}}$$

**(T-Null-Ptr)** Null Pointer Construction:

$$\frac{}{\Gamma \vdash \texttt{Ptr::null()} : \texttt{Ptr<T>@Null}}$$

**Place Expressions**

Valid place expressions include:
- Variable bindings
- Field access on place expressions
- Indexed access into arrays or slices

**Dynamic Semantics**

**Address-Of Evaluation**

When `&e` is evaluated:

1. Let `loc` be the memory location denoted by `e`
2. Construct `Ptr<T>@Valid` containing address of `loc`

**Null Pointer Evaluation**

When `Ptr::null()` is evaluated:

1. Construct `Ptr<T>@Null` containing address `0x0`

**Region Exit State Transition**

When a region block exits:

1. For every `p : Ptr<T>@Valid` whose referent was in the exiting region
2. `p` transitions to `Ptr<T>@Expired`
3. Subsequent dereference attempts are ill-formed

**Constraints**

**Diagnostics:** See Appendix A, code `E-TYP-2104`.


#### 12.3.4 Pointer Arithmetic (Unsafe)

**Pointer Arithmetic.** Raw pointers support pointer arithmetic and casts within `unsafe` blocks.

**Syntax**

```ebnf
address_of_expr       ::= "&" place_expr

null_ptr_expr         ::= "Ptr" "::" "null" "()"

raw_ptr_cast_expr     ::= expression "as" raw_pointer_type
```

**Static Semantics**

**(T-Raw-Ptr-Cast-Imm)**

$$\frac{\Gamma \vdash p : \texttt{Ptr<T>@Valid}}{\Gamma \vdash p\ \texttt{as *imm T} : \texttt{*imm T}}$$

**(T-Raw-Ptr-Cast-Mut)**

$$\frac{\Gamma \vdash p : \texttt{Ptr<T>@Valid}}{\Gamma \vdash p\ \texttt{as *mut T} : \texttt{*mut T}}$$

**Cast Constraints**

1. Only `Ptr<T>@Valid` may be cast to raw pointer
2. Casting `Ptr<T>@Null` or `Ptr<T>@Expired` to raw pointer is ill-formed

**FFI Constraints**

Raw pointers are the only FFI-safe pointer types:

1. `Ptr<T>@State` MUST NOT appear in `extern` procedure signatures
2. FFI boundary procedures MUST use raw pointer types

**Constraints**

**Diagnostics:** See Appendix A, codes `E-TYP-2105`, `E-TYP-2106`.


### 12.4 Function Types


#### 12.4.1 Function Type Syntax

**Function Type.** A structural type representing a callable signature, mapping parameter types to a return type.

**Formal Definition**

A function type is defined by:

$$F = (P_1, \ldots, P_n, R, \kappa, D)$$

where:
- $P_i = (m_i, T_i)$ is a parameter descriptor with optional `move` modifier $m_i \in \{\epsilon, \texttt{move}\}$
- $R$ is the return type
- $\kappa \in \{\texttt{sparse}, \texttt{closure}\}$ is the representation kind
- $D$ is an optional shared dependency set (present only when $\kappa = \texttt{closure}$; otherwise $D = \emptyset$)

**Syntax**

```ebnf
function_type         ::= sparse_function_type | closure_type

sparse_function_type  ::= "(" param_type_list? ")" "->" type

closure_type          ::= "|" param_type_list? "|" "->" type closure_deps?

closure_deps          ::= "[" "shared" ":" "{" shared_dep_list? "}" "]"
shared_dep_list       ::= shared_dep ("," shared_dep)*
shared_dep            ::= identifier ":" type

param_type_list       ::= param_type ("," param_type)*

param_type            ::= "move"? type
```

**Static Semantics**

**(T-Func-WF)** Well-Formedness:

$$\frac{\Gamma \vdash R\ \text{wf} \quad \forall i \in 1..n,\ \Gamma \vdash T_i\ \text{wf}}{\Gamma \vdash (m_1\ T_1, \ldots, m_n\ T_n) \to R\ \text{wf}}$$

**(T-Closure-WF)**

$$\frac{\Gamma \vdash R\ \text{wf} \quad \forall i \in 1..n,\ \Gamma \vdash T_i\ \text{wf}}{\Gamma \vdash |m_1\ T_1, \ldots, m_n\ T_n| \to R\ \text{wf}}$$

**Type Identity**

1. Representation kind is part of identity: `(T) -> U` and `|T| -> U` are distinct
2. `move` modifier is part of identity: `(T) -> U` and `(move T) -> U` are distinct
3. A shared dependency set (if present) is part of closure type identity: `|T| -> U [shared: {...}]` and `|T| -> U` are distinct, and differing dependency sets are distinct types.
4. Contract annotations and parameter names are NOT part of identity


#### 12.4.2 Function Pointers

**Sparse Function Pointer.** A `(T) -> U` is a single-word code pointer. FFI-safe.

**Static Semantics**

**(T-Proc-As-Value)** Procedure Reference:

$$\frac{\text{procedure } f(m_1\ x_1 : T_1, \ldots, m_n\ x_n : T_n) \to R\ \text{declared}}{\Gamma \vdash f : (m_1\ T_1, \ldots, m_n\ T_n) \to R}$$

**(T-Closure-Sparse)** Non-Capturing Closure:

$$\frac{\text{captures}(|\ldots| \to e) = \emptyset}{\Gamma \vdash |\ldots| \to e : (T_1, \ldots, T_n) \to R}$$

**(T-Sparse-Sub-Closure)** Representation Subtyping:

$$\frac{\Gamma \vdash (T_1, \ldots, T_n) \to R\ \text{wf}}{\Gamma \vdash (T_1, \ldots, T_n) \to R <: |T_1, \ldots, T_n| \to R}$$

**Dynamic Semantics**

**Representation**

$$\text{sizeof}((T_1, \ldots, T_n) \to R) = \text{sizeof}(\texttt{usize})$$

```
┌─────────────────────────────────┐
│          code_ptr               │
└─────────────────────────────────┘
         (1 machine word)
```


#### 12.4.3 Closure Types

**Closure Type.** A `|T| -> U` is a two-word structure containing environment and code pointers. NOT FFI-safe.

An optional **shared dependency set** may appear as a suffix on a closure type:

```cursive
|T| -> U [shared: { x: shared A, y: shared B }]
```

The dependency set records the `shared` bindings a closure may access after it escapes its defining scope. It is required for **escaping closures** that capture `shared` data, and is governed by the Key System rules in §17.2.5.

**Static Semantics**

**(T-Closure-Capturing)** Capturing Closure:

$$\frac{\text{captures}(|\ldots| \to e) \neq \emptyset}{\Gamma \vdash |\ldots| \to e : |T_1, \ldots, T_n| \to R}$$

**(T-Call)** See §15.4.1 for the normative function invocation semantics.

**Dynamic Semantics**

**Representation**

$$\text{sizeof}(|T_1, \ldots, T_n| \to R) = 2 \times \text{sizeof}(\texttt{usize})$$

```
┌─────────────────────────────────┬─────────────────────────────────┐
│          env_ptr                │          code_ptr               │
└─────────────────────────────────┴─────────────────────────────────┘
         (1 machine word)                  (1 machine word)
```

- `env_ptr`: Pointer to captured environment (null for non-capturing coerced to closure)
- `code_ptr`: Pointer to executable code expecting `env_ptr` as implicit first argument

**Sparse-to-Closure Coercion**

When a sparse function pointer is coerced to closure type:

1. Construct closure value
2. Set `env_ptr` to null
3. Set `code_ptr` to thunk ignoring environment parameter


#### 12.4.4 Capture Modes

**Capture Modes.** Determine how bindings from the enclosing scope are captured by closure expressions.

**Static Semantics**

**Move Modifier Invariance**

The `move` modifier is part of a callable's calling convention: it determines whether responsibility transfers at the call boundary (§3.4.5). As a result, `move` modifiers are **invariant** under subtyping: a mismatch in `move`-ness is incompatible.

Formally:

$$\Gamma \nvdash (T) \to R <: (\texttt{move } T) \to R$$

$$\Gamma \nvdash (\texttt{move } T) \to R <: (T) \to R$$

**Variance**

Function types exhibit:
- Contravariant parameters
- Covariant return types

**Constraints**

**Structural Constraints**

1. Zero or more parameters, exactly one return type
2. Return type `()` for procedures returning no meaningful value
3. `move` modifier applies only to parameters, not return type

**FFI Constraints**

1. Closure types MUST NOT appear in `extern` signatures
2. Only sparse function pointer types are FFI-safe
3. Sparse function pointers in FFI MUST NOT have generic type parameters

**Invocation Constraints**

1. Argument count MUST equal parameter count
2. Each argument type MUST be a subtype of corresponding parameter type
3. For `move` parameters, argument MUST be explicit `move` expression

**Diagnostics:** See Appendix A, codes `E-TYP-2220`–`E-TYP-2226`. For `E-MOD-2411` (missing move expression at call site for transferring parameter), see §3.4.6.

---

## 13. Abstraction and Polymorphism

---

### 13.1 Generics


#### 13.1.1 Generic Type Parameters

**Generic Declaration.** Introduces one or more **type parameters** that serve as placeholders for concrete types supplied at instantiation.

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
- $\text{Bounds}_i \subseteq \mathcal{T}_{\text{class}}$ is a (possibly empty) set of class bounds

A type parameter with $\text{Bounds}_i = \emptyset$ is **unconstrained**. A type parameter with $\text{Bounds}_i \neq \emptyset$ is **constrained**; only types implementing all classes in $\text{Bounds}_i$ may be substituted.

**Syntax**

```ebnf
generic_params     ::= "<" generic_param_list ">"
generic_param_list ::= generic_param (";" generic_param)*
generic_param      ::= identifier bound_clause? default_clause?
bound_clause       ::= "<:" class_bound_list
default_clause     ::= "=" type
class_bound_list   ::= class_bound ("," class_bound)*

generic_args       ::= "<" type_arg_list ">"
type_arg_list      ::= type ("," type)*
turbofish          ::= "::" generic_args
```

```cursive
record Container<T> { value: T }
record Pair<K; V> { key: K, value: V }
```

**Static Semantics**

**(WF-Generic-Param)**

$$\frac{
    \forall i \neq j,\ \text{name}_i \neq \text{name}_j \quad
    \forall i,\ \forall B \in \text{Bounds}_i,\ \Gamma \vdash B : \text{Class}
}{
    \Gamma \vdash \langle P_1; \ldots; P_n \rangle\ \text{wf}
}$$

**(WF-Generic-Type)**

$$\frac{
    \Gamma \vdash \langle P_1, \ldots, P_n \rangle\ \text{wf} \quad
    \Gamma, T_1 : P_1, \ldots, T_n : P_n \vdash \text{Body}\ \text{wf}
}{
    \Gamma \vdash \texttt{type}\ \textit{Name}\langle P_1, \ldots, P_n \rangle\ \text{Body}\ \text{wf}
}$$

**Constraints**

| Separator | Role                                  | Scope                            |
| :-------- | :------------------------------------ | :------------------------------- |
| `;`       | Separates type parameters             | Between `<` and `>`              |
| `,`       | Separates bounds within one parameter | After `<:` until next `;` or `>` |

**Diagnostics:** See Appendix A, code `E-TYP-2304`.


#### 13.1.2 Generic Procedures

**Generic Procedure.** A procedure parameterized by type parameters. Type arguments may be explicit or inferred.

**Syntax**

```ebnf
generic_procedure ::= "procedure" identifier generic_params "(" param_list? ")" ("->" type)? block
```

```cursive
procedure identity<T>(x: T) -> T { result x }
procedure swap<T>(a: unique T, b: unique T) { ... }
```

**Static Semantics**

**(WF-Generic-Proc)**

$$\frac{
    \Gamma \vdash \langle P_1, \ldots, P_n \rangle\ \text{wf} \quad
    \Gamma' = \Gamma, T_1 : P_1, \ldots, T_n : P_n \quad
    \Gamma' \vdash \text{signature}\ \text{wf} \quad
    \Gamma' \vdash \text{body}\ \text{wf}
}{
    \Gamma \vdash \texttt{procedure}\ f\langle P_1, \ldots, P_n \rangle(\ldots) \to R\ \{\ldots\}\ \text{wf}
}$$

**(T-Generic-Call)**

$$\frac{
    \begin{gathered}
    f\langle T_1, \ldots, T_n \rangle(x_1 : S_1, \ldots, x_m : S_m) \to R\ \text{declared} \\
    \forall i \in 1..n,\ \Gamma \vdash A_i\ \text{satisfies}\ \text{Bounds}(T_i) \\
    \forall j \in 1..m,\ \Gamma \vdash e_j : S_j[A_1/T_1, \ldots, A_n/T_n]
    \end{gathered}
}{
    \Gamma \vdash f\langle A_1, \ldots, A_n \rangle(e_1, \ldots, e_m) : R[A_1/T_1, \ldots, A_n/T_n]
}$$

**Type Argument Inference**

When type arguments are not explicitly provided, the implementation MUST infer them using bidirectional type inference (§9.4). Inference sources:
1. Types of value arguments at the call site
2. Expected return type from the surrounding context

Generic procedures in classes cannot participate in dynamic dispatch. Such procedures MUST be marked with `[[static_dispatch_only]]` (§7.4) and can only be called through concrete type references, not through `$Class` references.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-TYP-2301`, `E-TYP-2306`.


#### 13.1.3 Generic Constraints

**Constraint Satisfaction.** Determines whether a type argument satisfies the bounds declared for a type parameter.

**Syntax**

```ebnf
where_clause         ::= "where" where_predicate_list
where_predicate_list ::= where_predicate (predicate_separator where_predicate)* ";"?
where_predicate      ::= identifier "<:" class_bound_list
predicate_separator  ::= terminator
```

```cursive
procedure sort<T>(arr: unique [T]) where T <: Ord { ... }
```

**Static Semantics**

**(T-Constraint-Sat)**

$$\frac{
    \forall B \in \text{Bounds},\ \Gamma \vdash A <: B
}{
    \Gamma \vdash A\ \text{satisfies}\ \text{Bounds}
}$$

**(T-Generic-Inst)**

$$\frac{
    \text{Name}\langle P_1, \ldots, P_n \rangle\ \text{declared} \quad
    \forall i \in 1..n,\ \Gamma \vdash A_i\ \text{satisfies}\ \text{Bounds}(P_i)
}{
    \Gamma \vdash \text{Name}\langle A_1, \ldots, A_n \rangle\ \text{wf}
}$$

When both inline bounds and a `where` clause specify constraints for the same parameter, the effective constraint is the conjunction of all bounds.

**Constraints**

1. A class bound MUST reference a valid class type; bounding by non-class types is forbidden.
2. A generic instantiation MUST provide exactly the number of type arguments declared.
3. Type arguments MUST satisfy all constraints declared for their corresponding parameters.

**Diagnostics:** See Appendix A, codes `E-TYP-2302`, `E-TYP-2303`, `E-TYP-2305`.


#### 13.1.4 Monomorphization

**Monomorphization.** The process of generating specialized code for each concrete instantiation of a generic type or procedure.

Given a generic declaration $D\langle T_1, \ldots, T_n \rangle$ and concrete type arguments $A_1, \ldots, A_n$, monomorphization produces a specialized declaration $D[A_1/T_1, \ldots, A_n/T_n]$ where each occurrence of $T_i$ in the body is replaced with $A_i$.

**Static Semantics**

**Monomorphization Requirements**

1. **Specialization:** For each instantiation $D\langle A_1, \ldots, A_n \rangle$, produce code equivalent to substituting each type argument for its corresponding parameter throughout the declaration body.

2. **Zero Overhead:** Calls to generic procedures MUST compile to direct static calls to the specialized instantiation. Virtual dispatch is prohibited for static polymorphism.

3. **Independent Instantiation:** Each distinct instantiation is an independent type or procedure. `Container<i32>` and `Container<i64>` are distinct types.

**Recursion Depth**

Monomorphization MAY produce recursive instantiations. Implementations MUST detect and reject infinite monomorphization recursion. The maximum instantiation depth is IDB; minimum supported depth is 128.

**Variance**

The variance of each type parameter is determined by its usage within the type definition. See §9.3 for variance specification.

**Dynamic Semantics**

**Layout Independence**

Each monomorphized instantiation has an independent memory layout:

$$\text{sizeof}(\text{Name}\langle A_1, \ldots, A_n \rangle) = \text{sizeof}(\text{Name}[A_1/T_1, \ldots, A_n/T_n])$$

$$\text{alignof}(\text{Name}\langle A_1, \ldots, A_n \rangle) = \text{alignof}(\text{Name}[A_1/T_1, \ldots, A_n/T_n])$$

**Constraints**

**Diagnostics:** See Appendix A, codes `E-TYP-2307`, `E-TYP-2308`.


### 13.2 Classes (Forms)


#### 13.2.1 Class Declaration Syntax

**Class.** A named declaration that defines an abstract interface consisting of procedure signatures, associated type declarations, abstract fields, and abstract states.

A class $Cl$ is defined as a tuple:

$$Cl = (N, G, S, P_{abs}, P_{con}, A_{abs}, A_{con}, F, St)$$

where:
- $N$ is the class name
- $G$ is the (possibly empty) set of generic type parameters
- $S$ is the (possibly empty) set of superclass bounds
- $P_{abs}$ is the set of abstract procedure signatures
- $P_{con}$ is the set of concrete procedure definitions (default implementations)
- $A_{abs}$ is the set of abstract associated type declarations
- $A_{con}$ is the set of concrete associated type bindings
- $F$ is the (possibly empty) set of abstract field declarations
- $St$ is the (possibly empty) set of abstract state declarations

A class with $St \neq \emptyset$ is a **modal class**. Only modal types may implement modal classes.

A type $T$ **implements** class $Cl$ (written $T <: Cl$) when:

$$T <: Cl \iff \forall p \in P_{abs}.\ T \text{ defines } p\ \land\ \forall a \in A_{abs}.\ T \text{ binds } a\ \land\ \forall f \in F.\ T \text{ has } f\ \land\ \forall s \in St.\ T \text{ has } s$$

**Syntax**

```ebnf
class_declaration ::=
    visibility? "class" identifier generic_params?
    ("<:" superclass_bounds)? "{"
        class_item*
    "}"

superclass_bounds ::= class_bound ("+" class_bound)*
class_bound       ::= type_path generic_args?

class_item ::=
    abstract_procedure
  | concrete_procedure
  | associated_type
  | abstract_field
  | abstract_state

abstract_procedure ::= "procedure" identifier signature contract_clause?
concrete_procedure ::= "procedure" identifier signature contract_clause? block
abstract_field     ::= identifier ":" type
abstract_state     ::= "@" identifier "{" field_list? "}"
field_list         ::= abstract_field ("," abstract_field)* ","?
```

```cursive
class Printable {
    procedure print(~, out: $FileSystem)
}
```

**Static Semantics**

**(WF-Class)**

$$\frac{
  \text{unique}(N) \quad
  \forall p \in P.\ \Gamma, \text{Self} : \text{Type} \vdash p\ \text{wf} \quad
  \neg\text{cyclic}(S) \quad
  \text{names\_disjoint}(P, A, F, St)
}{
  \Gamma \vdash \text{class } N\ [<: S]\ \{P, A, F, St\}\ \text{wf}
}$$

**(T-Superclass)**

$$\frac{\text{class } A <: B \quad T <: A}{\Gamma \vdash T <: B}$$

Within a class declaration, `Self` denotes the (unknown) implementing type.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-TYP-2500`, `E-TYP-2504`, `E-TYP-2505`, `E-TYP-2508`, `E-TYP-2509`, `E-TYP-2408`, `E-TYP-2409`.


#### 13.2.2 Associated Methods

**Abstract Procedure.** A procedure signature within a class that lacks a body. Implementing types MUST provide a concrete implementation.

**Concrete Procedure.** A procedure definition within a class that includes a body. Implementing types automatically inherit this procedure but MAY override it using the `override` keyword.

**Static Semantics**

**(WF-Class-Self)**

$$\frac{\Gamma, \text{Self} : \text{Type} \vdash \text{body} : \text{ok}}{\Gamma \vdash \text{class } T\ \{\ \text{body}\ \} : \text{Class}}$$


#### 13.2.3 Associated Types

**Associated Type.** A type declaration within a class:
- If abstract (no `= T`): implementing types MUST provide a concrete type binding
- If concrete (`= T`): provides a default type that MAY be overridden

**Syntax**

```ebnf
associated_type ::= "type" identifier ("=" type)?
```

See the `Iterator` class definition in §13.9.5 for an example of associated types in practice.

**Static Semantics**

Generic class parameters (`class Foo<T>`) are specified at use-site. Associated types are specified by the implementer within the type body.

**Class Alias Equivalence (T-Alias-Equiv)**

$$\frac{\text{type } Alias = A + B}{\Gamma \vdash T <: Alias \iff \Gamma \vdash T <: A \land \Gamma \vdash T <: B}$$


### 13.3 Class Implementation


#### 13.3.1 Implementation Blocks

**Class Implementation.** A type implements a class by:
1. Declaring the class in its "implements clause" using the `<:` operator
2. Providing implementations for all abstract procedures
3. Providing type bindings for all abstract associated types
4. Having fields with matching names and compatible types for all abstract fields
5. Having states with matching names and required payload fields for all abstract states (modal classes only)

Class implementation MUST occur at the type's definition site. Extension implementations are prohibited.

**Syntax**

```ebnf
impl_procedure    ::= visibility? "override"? "procedure" identifier signature block
```

```cursive
record Point { x: f64, y: f64 } <: Display {
    procedure display(~) -> string { ... }
}
```

**Static Semantics**

**(T-Impl-Complete)**

$$\frac{
  T <: Cl \quad
  \forall p \in P_{abs}(Cl).\ T \text{ defines } p \quad
  \forall a \in A_{abs}(Cl).\ T \text{ binds } a \quad
  \forall f \in F(Cl).\ T \text{ has } f \quad
  \forall s \in St(Cl).\ T \text{ has } s
}{
  \Gamma \vdash T \text{ implements } Cl
}$$

**(WF-Impl)**

$$\frac{
  \Gamma \vdash T\ \text{wf} \quad
  \Gamma \vdash Cl\ \text{wf} \quad
  St(Cl) \neq \emptyset \implies T \text{ is modal}
}{
  \Gamma \vdash T <: Cl\ \text{wf}
}$$

**Override Semantics**

- **Implementing an abstract procedure**: `override` keyword MUST NOT be used
- **Overriding a concrete procedure**: `override` keyword MUST be used

**Coherence Rule**

A type `T` MAY implement a class `Cl` at most once.

**Orphan Rule**

For `T <: Cl`, at least one of `T` or `Cl` MUST be defined in the current assembly.

**(T-Field-Compat)**

$$\frac{
  f : T_c \in F(Cl) \quad
  f : T_i \in \text{fields}(R) \quad
  T_i <: T_c
}{
  R \vdash f\ \text{present}
}$$

**(T-Modal-Class)**

$$\frac{
  St(Cl) \neq \emptyset \quad
  T <: Cl \quad
  T \text{ is not a modal type}
}{
  \text{ill-formed: E-TYP-2401}
}$$

**Constraints**

**Diagnostics:** See Appendix A, codes `E-TYP-2501`–`E-TYP-2507`, `E-TYP-2401`–`E-TYP-2407`.


### 13.4 Class Constraints


#### 13.4.1 Single Constraints

**Class Constraint.** A generic parameter `T <: Cl` restricts valid type arguments to types implementing class `Cl`.

**Static Semantics**

**Constraint Satisfaction**

A generic instantiation is well-formed only if every constrained parameter `T <: Cl` is instantiated with a type that implements `Cl`.

**Method Availability**

Within the body of a generic item, methods of `Cl` are callable on values of type `T` via static dispatch; calls resolve at monomorphization with no vtable lookup.


#### 13.4.2 Multiple Constraints (`+`)

**Multiple Constraints.** A type parameter MAY have multiple class bounds using the `+` separator or comma-separated bounds after `<:`.

**Syntax**

```cursive
procedure process<T <: Display, Clone>(value: T) { ... }
```

**Static Semantics**

The type argument must implement all specified classes.


#### 13.4.3 Where Clauses

**Where Clauses.** As an alternative to inline bounds, constraints MAY be specified in a `where` clause. See §13.1.3 for full grammar.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-TYP-2530`, `E-TYP-2531`.


### 13.5 Dynamic Polymorphism (`$`)


#### 13.5.1 Dynamic Class Objects

**Dynamic Class Type.** A `$Class` is a concrete, sized type representing any value implementing a dispatchable class. Implemented as a dense pointer.

**Syntax**

```ebnf
dynamic_type ::= "$" class_path
class_path   ::= type_path
```


**Static Semantics**

**(T-Dynamic-Form)**

$$\frac{\Gamma \vdash v : T \quad \Gamma \vdash T <: Cl \quad \text{dispatchable}(Cl)}{\Gamma \vdash v : \$Cl}$$

**Dispatchability**

A class is **dispatchable** if every procedure in the class is either:
1. VTable-eligible, OR
2. Explicitly excluded via `[[static_dispatch_only]]`

**VTable Eligibility**

A procedure is **vtable-eligible** if ALL of the following are true:
1. Has a receiver parameter (`self`, `~`, `~!`, `~%`)
2. Has NO generic type parameters
3. Does NOT return `Self` by value (except via pointer indirection)
4. Does NOT use `Self` in parameter types (except via pointer indirection)

**Formal Definition**

$$\text{dispatchable}(Cl) \iff \forall p \in \text{procedures}(Cl).\ \text{vtable\_eligible}(p) \lor \text{has\_static\_dispatch\_attr}(p)$$

**Dynamic Semantics**

**Dynamic Class Type Creation**

1. Let `v` be a value of concrete type `T` where `T <: Cl` and `dispatchable(Cl)`.
2. Let `dp` be a pointer to the storage of `v`.
3. Let `vt` be the static vtable for the `(T, Cl)` type-class pair.
4. Construct the dynamic class value as the pair `(dp, vt)`.

**(E-Dynamic-Form)**

$$\frac{
  \Gamma \vdash v : T \quad
  T <: Cl \quad
  \text{dispatchable}(Cl)
}{
  v \Rightarrow_{\$} (\text{ptr}(v), \text{vtable}(T, Cl))
}$$

**Memory Layout**

A dynamic class type (`$Class`) is represented as a two-word structure:

- **Size**: `2 * sizeof(usize)` (16 bytes on 64-bit platforms)
- **Alignment**: `alignof(usize)`

```
struct DynRepr {
    data: *imm ()       // Pointer to concrete instance
    vtable: *imm VTable // Pointer to class vtable
}
```


#### 13.5.2 Virtual Method Dispatch

**Virtual Method Dispatch.** A procedure call on a dynamic class type dispatches through the vtable.

**Dynamic Semantics**

**VTable Dispatch Algorithm**

A procedure call `w~>method(args)` on dynamic class type `w: $Cl` executes as follows:

1. Let `(dp, vt)` be the data pointer and vtable pointer components of `w`.
2. Let `offset` be the vtable offset for `method`.
3. Let `fp` be the function pointer at `vt + header_size + offset * sizeof(usize)`.
4. Return the result of calling `fp(dp, args)`.

**(E-Dynamic-Dispatch)**

$$\frac{
  w = (dp, vt) \quad
  \text{method} \in \text{interface}(Cl) \quad
  vt[\text{offset}(\text{method})] = fp
}{
  w\text{\textasciitilde>method}(args) \to fp(dp, args)
}$$

**VTable Layout (Stable ABI)**

For non-modal classes (header size = 3):
1. `size: usize` — Size of concrete type
2. `align: usize` — Alignment requirement
3. `drop: *imm fn` — Destructor function pointer (null if no `Drop`)
4. `methods[..]` — Function pointers in class declaration order

For modal classes (header size = 4): includes additional `state_map` pointer.


#### 13.5.3 Object Safety

**Object Safety.** Determines whether a class can be used as a dynamic class type (dispatchability).

**Static Semantics**

A class that contains non-vtable-eligible procedures without `[[static_dispatch_only]]` is NOT dispatchable.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-TYP-2540`–`E-TYP-2542`.


### 13.6 Opaque Polymorphism (`opaque`)


#### 13.6.1 Opaque Return Types

**Opaque Return Type.** An `opaque Class` exposes only the class's interface while hiding the concrete implementation type.

**Syntax**
*[REF: Return-type grammar is defined in §14.1.1.]*

```cursive
procedure get_iterator() -> opaque Iterator {
    result [1, 2, 3]~>iter()
}
```

**Static Semantics**

**(T-Opaque-Return)**

$$\frac{
  \Gamma \vdash \text{body} : T \quad
  \Gamma \vdash T <: Cl \quad
  \text{return\_type}(f) = \text{opaque } Cl
}{
  \Gamma \vdash f : () \to \text{opaque } Cl
}$$

**(T-Opaque-Project)**

At call sites, the opaque type is treated as an existential; callers may invoke only class methods:

$$\frac{
  \Gamma \vdash f() : \text{opaque } Cl \quad
  m \in \text{interface}(Cl)
}{
  \Gamma \vdash f()\text{~>}m(args) : R_m
}$$

**Type Encapsulation**

For a procedure returning `opaque Class`:
- The callee returns a concrete type implementing `Class`
- The caller observes only `Class` members
- Access to concrete type members is forbidden


#### 13.6.2 Type Erasure

**Type Erasure.** Opaque types provide type encapsulation without runtime overhead.

**Static Semantics**

**Opaque Type Equality**

Two opaque types `opaque Cl` are equivalent if and only if they originate from the same procedure definition:

$$\frac{
  f \neq g
}{
  \text{typeof}(f()) \neq \text{typeof}(g())
}$$

**Zero Overhead**

Opaque types MUST incur zero runtime overhead. The returned value is the concrete type, not a dense pointer. Type encapsulation is enforced statically.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-TYP-2510`–`E-TYP-2512`.


### 13.7 Refinement Types


#### 13.7.1 Refinement Syntax

**Refinement Type.** A type constructed by attaching a predicate constraint to a base type. The refinement type `T where { P }` denotes the subset of values of type `T` for which predicate `P` evaluates to `true`.

A refinement type $R$ is defined by:

$$R = (T_{\text{base}}, P)$$

where:
- $T_{\text{base}} \in \mathcal{T}$ is the base type being refined
- $P : T_{\text{base}} \to \texttt{bool}$ is a pure predicate constraining the value set

$$\text{Values}(T \text{ where } \{P\}) = \{ v \in \text{Values}(T) \mid P(v) = \texttt{true} \}$$

A refinement type is a **proper subtype** of its base type.

**Syntax**

```ebnf
refinement_type       ::= type "where" "{" predicate_expr "}"
type_alias_decl       ::= visibility? "type" identifier "=" type "where" "{" predicate_expr "}"
param_with_constraint ::= identifier ":" type "where" "{" predicate_expr "}"
```

```cursive
type NonZero = i32 where { self != 0 }
procedure divide(a: i32, b: i32 where { b != 0 }) -> i32
```

Within standalone refinement types, `self` refers to the constrained value. In parameter constraints, the parameter name is used instead.


#### 13.7.2 Refinement Constraints

**Static Semantics**

**(WF-Refine-Type)**

$$\frac{
    \Gamma \vdash T\ \text{wf} \quad
    \Gamma, \texttt{self} : T \vdash P : \texttt{bool} \quad
    \text{Pure}(P)
}{
    \Gamma \vdash (T \text{ where } \{P\})\ \text{wf}
}$$

**(T-Refine-Intro)**

$$\frac{
    \Gamma \vdash e : T \quad
    \Gamma \vdash F(P[e/\texttt{self}], L) \quad
    L \text{ dominates current location}
}{
    \Gamma \vdash e : T \text{ where } \{P\}
}$$

**(T-Refine-Elim)**

$$\frac{
    \Gamma \vdash e : T \text{ where } \{P\}
}{
    \Gamma \vdash e : T
}$$

**Subtyping Rules**

$$\Gamma \vdash (T \text{ where } \{P\}) <: T$$

$$\frac{
    \Gamma \vdash P \implies Q
}{
    \Gamma \vdash (T \text{ where } \{P\}) <: (T \text{ where } \{Q\})
}$$

**Nested Refinements**

$$(T \text{ where } \{P\}) \text{ where } \{Q\} \equiv T \text{ where } \{P \land Q\}$$

**Decidable Predicate Subset**

Implementations MUST support:
1. Literal comparisons
2. Bound propagation from control flow
3. Syntactic equality (up to alpha-renaming)
4. Transitive inequalities (linear arithmetic over integers)
5. Boolean combinations of decidable predicates


#### 13.7.3 Refinement Verification

**Static Semantics**

Refinement predicates require static proof by default:

1. Implementation attempts static verification
2. If verification succeeds, no runtime code is generated
3. If verification fails and not in `[[dynamic]]` scope: ill-formed (`E-TYP-1953`)
4. If verification fails in `[[dynamic]]` scope: runtime check is generated

**Dynamic Semantics**

**Representation**

$$\text{sizeof}(T \text{ where } \{P\}) = \text{sizeof}(T)$$

$$\text{alignof}(T \text{ where } \{P\}) = \text{alignof}(T)$$

The predicate is a compile-time and runtime constraint only; it does not affect physical representation.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-TYP-1954`–`E-TYP-1957`.


### 13.8 Capability Classes

*[REF: Capability fundamentals defined in §4 (The Authority Model). This section covers type-system integration only.]*


#### 13.8.1 Capability Class

**Capability Class.** Regular classes that define system authority interfaces. The dynamic dispatch operator `$` applies uniformly to all classes, including capability classes.

**Syntax**

```cursive
procedure read_config(fs: $FileSystem, path: string@View) -> string {
    let file = fs~>open_read(path)
    file~>read_all()
}
```

**Static Semantics**

A parameter of type `$Class` accepts any concrete type `T` implementing `Class`. This is the same mechanism as for any other class—capability classes have no special type-system behavior.

See §4 for capability propagation, attenuation, and no-ambient-authority rules.


#### 13.8.2 Capability Bounds

**Capability Bounds.** Capability classes can be used as bounds in generics like any other class.

**Syntax**

```cursive
procedure with_logging<L <: Logger>(fs: $FileSystem, log: L) { ... }
```

**Static Semantics**

No syntactic distinction exists between capability class bounds and regular class bounds. The capability nature is semantic (enforcement of no-ambient-authority), not syntactic.


### 13.9 Foundational Classes

*[REF: Complete semantics in Appendix D (Standard Form Catalog).]*


#### 13.9.1 `Drop` Class

**Drop Class.** Defines a destructor for RAII cleanup. The `drop` method is invoked implicitly when a binding goes out of scope.

**Syntax**

```cursive
class Drop {
    procedure drop(~!)
}
```

**Static Semantics**

- `Drop::drop` MUST NOT be called directly by user code (`E-MEM-3005`, §3.5.3).
- `Bitcopy` and `Drop` are mutually exclusive on the same type.

**Constraints**

**Diagnostics:** See Appendix A, code `E-TYP-2621`.


#### 13.9.2 `Bitcopy` Class

**Bitcopy Class.** A marker class for types that can be implicitly duplicated via bitwise copy.

**Static Semantics**

- `Bitcopy` types are duplicated (not moved) on assignment and parameter passing.
- `Bitcopy` requires all fields implement `Bitcopy`.
- Any type implementing `Bitcopy` MUST also implement `Clone`.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-TYP-2622`, `E-TYP-2623`.


#### 13.9.3 `Clone` Class

**Clone Class.** Defines explicit deep copy via the `clone` method.

**Syntax**

```cursive
class Clone {
    procedure clone(~) -> Self
}
```

**Static Semantics**

For `Bitcopy` types, `clone()` performs a bitwise copy identical to the implicit copy operation.


#### 13.9.4 `Eq` and `Hash` Classes

**Eq and Hash Classes.** Define equality comparison and hashing for use in collections.

**Static Semantics**

*[REF: `Eq` signature defined in Appendix D §D.2.1.]*

Types implementing `Hash` MUST also implement `Eq`. If two values are equal per `Eq::eq`, they MUST produce the same hash value.


#### 13.9.5 `Iterator` Class

**Iterator Class.** Defines the iteration protocol for `loop ... in` expressions.

**Syntax**

```cursive
class Iterator {
    type Item
    procedure next(~!) -> Self::Item | ()
}
```

**Static Semantics**

Range types implement `Iterator` when their element type implements `Step`.


#### 13.9.6 `Step` Class

**Step Class.** Defines discrete stepping for range iteration.

**Syntax**

```cursive
class Step {
    procedure successor(~) -> Self | ()
    procedure predecessor(~) -> Self | ()
}
```

---

# Part V: Executable Semantics

---

## 14. Procedures and Contracts

---

### 14.1 Declarations


#### 14.1.1 Procedure Syntax

**Procedure.** A named, callable unit of computation that accepts zero or more parameters, performs computation, and optionally returns a value.

A procedure $P$ is defined as:

$$P = (N, V, G, \Sigma, C, B)$$

where:
- $N$: Procedure name
- $V$: Visibility level
- $G$: Generic parameter list
- $\Sigma$: Signature (parameter types and return type)
- $C$: Contract clauses
- $B$: Body

**Syntax**

```ebnf
procedure_decl ::= visibility? "procedure" identifier generic_params?
                   "(" param_list? ")" ("->" return_type)?
                   contract_clause? block

signature      ::= "(" param_list? ")" ("->" return_type)?

param_list     ::= param ("," param)* ","?
param          ::= param_mode? identifier ":" type
param_mode     ::= "move"
return_type    ::= type | union_return
               | "opaque" class_path
union_return   ::= type ("|" type)+
```

```cursive
procedure divide(a: i32, b: i32) -> i32 | DivisionByZero {
    if b == 0 { result DivisionByZero {} }
    result a / b
}
```

**Static Semantics**

**(T-Proc)**

$$\frac{
  \Gamma \vdash p_1 : T_1, \ldots, p_n : T_n \quad
  \Gamma, p_1 : T_1, \ldots, p_n : T_n \vdash B : T_r
}{
  \Gamma \vdash \texttt{procedure } N(p_1: T_1, \ldots, p_n: T_n) \rightarrow T_r\ \{B\} : (T_1, \ldots, T_n) \rightarrow T_r
}$$

**(WF-Proc)**

A procedure declaration is well-formed if:
1. The procedure name is a valid identifier not already bound in the current scope
2. All parameter names are distinct
3. All parameter types and the return type are well-formed
4. The body is well-typed with respect to the parameter context
5. All return paths produce values of compatible types
6. Contract expressions are pure and of type `bool`


#### 14.1.2 Parameter Lists and Passing

*[REF: `move` parameter mode transfers responsibility per §3.4. This section defines parameter syntax only.]*

**Parameters.** Declared with an optional `move` modifier that determines responsibility transfer. Permission is expressed in the parameter type (e.g., `x: unique T`, `x: shared T`). When no permission is written, `const` is the default (§10.1.1).

**Static Semantics**

**Parameter Responsibility Modifier**

| Form    | Syntax      | Responsibility | Callee Access     |
| :------ | :---------- | :------------- | :---------------- |
| Default | `x: T`      | Caller retains | As written in `T` |
| Move    | `move x: T` | Callee assumes | As written in `T` |

**Parameter Evaluation Order**

Arguments are evaluated left-to-right in declaration order:

$$\text{eval}(f(e_1, e_2, \ldots, e_n)) = \text{eval}(e_1); \text{eval}(e_2); \ldots; \text{eval}(e_n); \text{call}(f)$$

**Dynamic Semantics**

**Argument Binding Sequence**

1. Each argument expression is evaluated left-to-right
2. For `move` parameters: responsibility transfers from caller to callee
3. For non-move parameters: a permission‑appropriate non‑transferring alias is created
4. Parameter bindings are introduced into the procedure's scope
5. The procedure body executes with these bindings

**Unique Alias Inactivation**

When an argument with `unique` permission is passed to a non‑`move` parameter, the caller’s `unique` path becomes **Inactive** for the dynamic extent of the call. While the caller’s path is inactive, the caller MUST NOT read, write, or move that storage; on return, the caller’s path reactivates. Any read, write, or move through an inactive `unique` binding is ill‑formed per the exclusivity invariant (§10.1.2) and the unique downgrade state machine (§10.2.2).

**Constraints**

**Diagnostics:** See Appendix A, codes `E-SEM-3001`, `E-SEM-3022`. For `E-MOD-2411` (missing move expression at call site for transferring parameter), see §3.4.6.


#### 14.1.3 Return Types

**Static Semantics**

*[REF: Return type grammar is defined in §14.1.1. Union types are defined in §11.8.]*

The return type $R$ may be any well-formed type. Union returns are used for error handling. Procedures returning no meaningful value use the unit type `()`.


#### 14.1.4 Visibility Modifiers

**Static Semantics**

Procedure visibility is controlled by the same modifiers as other declarations (`public`, `internal`, `protected`, `private`).

*[REF: Visibility level definitions and accessibility rules are specified in §6.3.]*


#### 14.1.5 Procedure Execution Model

**Dynamic Semantics**

**Invocation Sequence**

Procedure invocation proceeds through the following phases:

1. **Argument Evaluation**: Arguments evaluated left-to-right
2. **Frame Construction**: New stack frame allocated
3. **Parameter Binding**: Arguments bound to parameters
4. **Precondition Check**: If contracts enabled, preconditions verified
5. **Body Execution**: Procedure body executes
6. **Postcondition Check**: If contracts enabled, postconditions verified
7. **Return**: Return value produced; control returns to caller
8. **Cleanup**: Deferred actions and destructors execute per LIFO semantics (§3.5.2)

**Cleanup Ordering**

When a procedure exits, cleanup occurs per LIFO semantics (§3.5.2):
1. Deferred actions (`defer`) execute
2. Destructors for responsible bindings execute
3. The stack frame is deallocated

**Constraints**

**Diagnostics:** See Appendix A, codes `E-SEM-3002`, `E-SEM-3003`.


### 14.2 Methods and Receivers


#### 14.2.1 Method Syntax

**Method.** A procedure associated with a type. Methods are invoked on instances of their associated type through the receiver parameter.

**Syntax**

```ebnf
method_def     ::= visibility? "procedure" identifier "(" receiver ("," param_list)? ")"
                   ("->" return_type)? contract_clause? block

receiver       ::= receiver_shorthand | explicit_receiver
explicit_receiver  ::= param_mode? "self" ":" type
```

The presence of a receiver parameter distinguishes methods from standalone procedures.


#### 14.2.2 Receiver Permission Shorthand (`~`, `~!`, `~%`)

**Receiver Shorthand.** Shorthand notation for method receivers.

**Syntax**

```ebnf
receiver_shorthand ::= "~" | "~!" | "~%"
```

| Shorthand | Expansion           | Permission | Description                |
| :-------- | :------------------ | :--------- | :------------------------- |
| `~`       | `self: const Self`  | `const`    | Read-only access           |
| `~!`      | `self: unique Self` | `unique`   | Exclusive mutable access   |
| `~%`      | `self: shared Self` | `shared`   | Synchronized shared access |

Within a method body, `self` refers to the receiver instance. The `self` binding is introduced implicitly by the receiver declaration, has the type and permission specified by the receiver, and MUST NOT be explicitly declared in the parameter list.

*[REF: For receiver permission compatibility rules, see §10.3.3.]*


#### 14.2.3 Method Resolution Order

**Static Semantics**

**Receiver Permission Compatibility**

Receiver permission compatibility follows the subtyping rules in §10.3.3. A method with receiver permission $P_{\text{method}}$ is callable through a path with permission $P_{\text{caller}}$ iff $P_{\text{caller}} <: P_{\text{method}}$.

**(Receiver-Compat)**

$$\frac{
    \Gamma \vdash e : P_{\text{caller}}\ T \quad
    m \in \text{Methods}(T) \quad
    m.\text{receiver} = P_{\text{method}}\ \texttt{Self} \quad
    \Gamma \vdash P_{\text{caller}}\ T <: P_{\text{method}}\ T
}{
    \Gamma \vdash e \mathord{\sim>} m(\ldots)\ \text{well-typed}
}$$

**Dynamic Semantics**

**Key Acquisition in Method Calls**

When a method with `~%` receiver is called through a `shared` path, the key is acquired for the duration of the method call. See §17 for complete key system semantics.

**Constraints**

Receiver permission incompatibility is diagnosed as `E-TYP-1605` (§10.3.3).

**Diagnostics:** See Appendix A, codes `E-SEM-3011`, `E-SEM-3012`.


### 14.3 Overloading


#### 14.3.1 Overloading Rules

**Procedure Overloading.** Multiple procedures may share the same name if their signatures are distinguishable.

**Overload Set.** The collection of all procedures with the same name visible at a call site.

**Static Semantics**

Overloads are distinguishable if:
1. They have different arity, or
2. At the same arity, at least one parameter position has incompatible types


#### 14.3.2 Overload Resolution Algorithm

**Static Semantics**

Given a call $f(a_1, \ldots, a_n)$ where $f$ names an overload set $O$:

1. **Candidate Selection**: All procedures in $O$ with matching arity are candidates
2. **Type Filtering**: Candidates whose parameter types are incompatible with argument types are eliminated
3. **Best Match**: If exactly one candidate remains, it is selected
4. **Ambiguity**: If multiple candidates remain, the call is ambiguous (error)
5. **No Match**: If no candidates remain, the call is ill-formed (error)

**Disambiguation Rules**

When multiple overloads are candidates:
1. An exact type match is preferred over a subtype match
2. A non-generic overload is preferred over a generic overload
3. A more specific generic constraint is preferred over a less specific one


#### 14.3.3 Ambiguity Errors

**Constraints**

**Diagnostics:** See Appendix A, codes `E-SEM-3030`–`E-SEM-3032`.


### 14.4 Contract Syntax


#### 14.4.1 Contract Clause Position

**Contract.** A specification attached to a procedure declaration that asserts logical predicates over program state. Contracts govern logical validity through preconditions (caller obligations) and postconditions (callee guarantees).

A contract $C$ is a pair $(P_{pre}, P_{post})$ where:
- $P_{pre}$ is a conjunction of boolean predicates representing preconditions
- $P_{post}$ is a conjunction of boolean predicates representing postconditions

**Syntax**

```ebnf
contract_clause    ::= "|=" contract_body
contract_body      ::= precondition_expr
                     | precondition_expr "=>" postcondition_expr
                     | "=>" postcondition_expr

precondition_expr  ::= predicate_expr
postcondition_expr ::= predicate_expr
predicate_expr     ::= logical_or_expr
contract_intrinsic ::= "@result" | "@entry" "(" expression ")"
```

A contract clause (`|=`) MUST appear after the return type annotation (or after the parameter list if no return type) and before the procedure body.

```cursive
procedure divide(a: i32, b: i32) -> i32
    |= b != 0 => @result * b == a
{ a / b }
```

**Static Semantics**

**(WF-Contract)**

$$\frac{
    \Gamma_{pre} \vdash P_{pre} : \texttt{bool} \quad \text{pure}(P_{pre}) \\
    \Gamma_{post} \vdash P_{post} : \texttt{bool} \quad \text{pure}(P_{post})
}{
    \Gamma \vdash \texttt{|=}\ P_{pre} \Rightarrow P_{post} : \text{WF}
}$$

**Logical Operators**

Predicates use standard boolean operators with precedence (highest to lowest):
1. `!` (logical NOT) — right-associative
2. `&&` (logical AND) — left-associative, short-circuit
3. `||` (logical OR) — left-associative, short-circuit


#### 14.4.2 Purity Constraints

**Pure Expression.** An expression whose evaluation produces no observable side effects. All expressions within a contract MUST be pure.

**Static Semantics**

An expression $e$ satisfies $\text{pure}(e)$ iff:
1. $e$ MUST NOT invoke any procedure that accepts capability parameters
2. $e$ MUST NOT mutate state observable outside the expression's evaluation
3. Built-in operators on primitive types and `comptime` procedures are always pure

**Constraints**

**Diagnostics:** See Appendix A, codes `E-SEM-2802`, `E-SEM-3004`.


#### 14.4.3 Evaluation Contexts

**Evaluation Context.** Defines the set of bindings available for reference within a predicate expression.

**Static Semantics**

**Precondition Evaluation Context ($\Gamma_{pre}$)**

Includes:
- All procedure parameters at their entry state
- The receiver binding (if present)
- All bindings visible in the enclosing scope accessible without side effects

MUST NOT include:
- The `@result` intrinsic
- The `@entry` operator
- Any binding introduced within the procedure body

**Postcondition Evaluation Context ($\Gamma_{post}$)**

Includes:
- All procedure parameters:
  - Immutable parameters (`const`, `~`): same value as at entry
  - Mutable parameters (`unique`, `shared`): post-state value
- The receiver binding (post-state for mutable receivers)
- The `@result` intrinsic
- The `@entry` operator
- All bindings visible in the enclosing scope

**Mutable Parameter State Semantics**

| Location in Contract          | State Referenced |
| :---------------------------- | :--------------- |
| Left of `=>`                  | Entry state      |
| Right of `=>` (bare)          | Post-state       |
| Right of `=>` with `@entry()` | Entry state      |


### 14.5 Pre/Postconditions


#### 14.5.1 Precondition Syntax (`|=`)

**Precondition.** The logical expression appearing to the left of `=>` in a `|=` contract clause, or the entire expression if `=>` is absent. The caller MUST ensure this expression evaluates to `true` prior to the call.

**Static Semantics**

**(Pre-Satisfied)**

$$\frac{
    \Gamma \vdash f : (T_1, \ldots, T_n) \to R \quad
    \text{precondition}(f) = P_{pre} \quad
    \exists L.\ F(P_{pre}, L) \land L \text{ dom } S
}{
    \Gamma \vdash f(a_1, \ldots, a_n) @ S : \text{valid}
}$$

Failure to satisfy a precondition is diagnosed at the **caller**. The diagnostic MUST identify the call site.

**Elision Rules**

| Contract Form        | Precondition              |
| :------------------- | :------------------------ |
| `\|= P`              | $P$                       |
| `\|= P => Q`         | $P$                       |
| `\|= => Q`           | `true` (always satisfied) |
| (no contract clause) | `true` (always satisfied) |


#### 14.5.2 Postcondition Syntax (`=>`)

**Postcondition.** The logical expression appearing to the right of `=>` in a `|=` contract clause. The callee MUST ensure this expression evaluates to `true` immediately before returning.

**Static Semantics**

**(Post-Valid)**

$$\frac{
    \text{postcondition}(f) = P_{post} \quad
    \forall r \in \text{ReturnPoints}(f).\ \Gamma_r \vdash P_{post} : \text{satisfied}
}{
    f : \text{postcondition-valid}
}$$

Failure to satisfy a postcondition is diagnosed at the **callee**. The diagnostic MUST identify the return point.

**Elision Rules**

| Contract Form        | Postcondition             |
| :------------------- | :------------------------ |
| `\|= P`              | `true` (no postcondition) |
| `\|= P => Q`         | $Q$                       |
| `\|= => Q`           | $Q$                       |
| (no contract clause) | `true` (no postcondition) |


#### 14.5.3 `@result` Binding in Postconditions

**@result Intrinsic.** Refers to the return value in postcondition expressions.

**Static Semantics**

| Property     | Specification                                  |
| :----------- | :--------------------------------------------- |
| Availability | Postcondition expressions only (right of `=>`) |
| Type         | The return type of the enclosing procedure     |
| Value        | The value being returned from the procedure    |
| Unit Returns | If procedure returns `()`, `@result` has `()`  |

```cursive
procedure abs(x: i32) -> i32
    |= => @result >= 0 && (@result == x || @result == -x)
{ if x >= 0 { x } else { -x } }
```

**Constraints**

**Diagnostics:** See Appendix A, code `E-SEM-2806`.


#### 14.5.4 `@entry()` Expression

**@entry Operator.** Evaluates `expr` in the entry state of the procedure.

**Static Semantics**

| Property              | Specification                                          |
| :-------------------- | :----------------------------------------------------- |
| Availability          | Postcondition expressions only (right of `=>`)         |
| Semantics             | Evaluates `expr` in entry state of the procedure       |
| Evaluation Point      | After parameter binding, before body execution         |
| Expression Constraint | `expr` MUST be pure                                    |
| Expression Scope      | `expr` MUST reference only parameters and receiver     |
| Type Constraint       | Result type of `expr` MUST implement `Bitcopy` or `Clone` |

**(Entry-Type)**

$$\frac{\Gamma_{post} \vdash e : T \quad (T <: \text{Bitcopy} \lor T <: \text{Clone})}{\Gamma_{post} \vdash \text{@entry}(e) : T}$$

**Dynamic Semantics**

**Capture Semantics**

When `@entry(expr)` appears in a postcondition:
1. The implementation evaluates `expr` immediately after parameter binding
2. The result is captured: `Bitcopy` types use bitwise copy; `Clone` types invoke `clone()`
3. In the postcondition, `@entry(expr)` refers to this captured value

```cursive
record Counter { value: i32 }

procedure increment(~!)
    |= self.value >= 0 => self.value == @entry(self.value) + 1
{ self.value += 1 }
```

**Constraints**

**Diagnostics:** See Appendix A, code `E-SEM-2805`.


### 14.6 Invariants


#### 14.6.1 Type Invariants

**Type Invariant.** A `where` clause attached to a `record`, `enum`, or `modal` type declaration. The invariant constrains all instances of the type.

**Syntax**

```ebnf
type_invariant ::= "where" "{" predicate_expr "}"
```

```cursive
record BoundedCounter { value: u32, max: u32 }
where { self.value <= self.max }
```

**Static Semantics**

**Invariant Predicate Context**

Within a type invariant predicate:
- `self` refers to an instance of the type being defined
- Field access on `self` is permitted
- Method calls on `self` are permitted if the method is pure

**Enforcement Points**

| Enforcement Point   | Description                                       |
| :------------------ | :------------------------------------------------ |
| Post-Construction   | After constructor or literal initialization       |
| Pre-Call (Public)   | Before any public procedure with receiver         |
| Post-Call (Mutator) | Before any procedure taking `unique self` returns |

**Public Field Constraint**

Types with type invariants MUST NOT declare public mutable fields.

**Private Procedure Exemption**

Private procedures (`internal` or less) are exempt from the Pre-Call enforcement point. The type invariant MUST be verified when a private procedure returns to a public caller.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-SEM-2820`–`E-SEM-2824`.


#### 14.6.2 Loop Invariants

**Loop Invariant.** A `where` clause attached to a `loop` expression. The invariant specifies a predicate that MUST hold at the beginning of every iteration and after termination.

**Syntax**

```ebnf
loop_expression ::= "loop" loop_condition? loop_invariant? block
loop_condition  ::= expression
loop_invariant  ::= "where" "{" predicate_expr "}"
```

```cursive
var sum = 0
var i = 0
loop i < n where { sum == i * (i - 1) / 2 && i <= n } {
    sum += i
    i += 1
}
```

**Static Semantics**

**Enforcement Points**

| Point          | Description                               | Formal                                                 |
| :------------- | :---------------------------------------- | :----------------------------------------------------- |
| Initialization | Before the first iteration begins         | $\Gamma_0 \vdash Inv$                                  |
| Maintenance    | At the start of each subsequent iteration | $\Gamma_i \vdash Inv \implies \Gamma_{i+1} \vdash Inv$ |
| Termination    | Immediately after loop terminates         | $\Gamma_{exit} \vdash Inv$                             |

**Verification Fact Generation**

Upon successful verification at the Termination point, the implementation generates a Verification Fact $F(Inv, L_{exit})$ for use as a postcondition of the loop.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-SEM-2830`, `E-SEM-2831`.


#### 14.6.3 Invariant Verification

**Static Semantics**

Invariant verification follows the same rules as contract verification (§14.7):
- Static verification required by default
- `[[dynamic]]` attribute permits runtime verification when static proof fails


### 14.7 Verification Logic


#### 14.7.1 Contract Verification Modes

**Contract Verification.** Determines how predicates are ensured to hold. By default, **static verification is required**. The `[[dynamic]]` attribute permits runtime verification as an explicit opt-in.

**Static Semantics**

**Default: Static Verification Required**

**(Contract-Static-OK)**

$$\frac{
    \text{StaticProof}(\Gamma, P)
}{
    P : \text{verified (no runtime check)}
}$$

**(Contract-Static-Fail)**

$$\frac{
    \neg\text{StaticProof}(\Gamma, P) \quad
    \neg\text{InDynamicContext}
}{
    \text{program is ill-formed (E-SEM-2801)}
}$$

**With `[[dynamic]]`: Runtime Verification Permitted**

**(Contract-Dynamic-Elide)**

$$\frac{
    \text{StaticProof}(\Gamma, P)
}{
    P : \text{verified (no runtime check)}
}$$

**(Contract-Dynamic-Check)**

$$\frac{
    \neg\text{StaticProof}(\Gamma, P) \quad
    \text{InDynamicContext}
}{
    \text{emit runtime check for } P
}$$

**Mandatory Proof Techniques**

| Technique                | Description                                        |
| :----------------------- | :------------------------------------------------- |
| Constant propagation     | Evaluate expressions with compile-time constants   |
| Linear integer reasoning | Prove inequalities over bounded integer arithmetic |
| Boolean algebra          | Simplify and prove boolean expressions             |
| Control flow analysis    | Track predicates established by conditionals       |
| Type-derived bounds      | Use type constraints (e.g., `usize >= 0`)          |
| Verification facts       | Use facts established by prior checks (§14.7.2)    |

**Verification Location**

| Contract Type  | Verified Where    | `[[dynamic]]` Context Determined By  |
| :------------- | :---------------- | :----------------------------------- |
| Precondition   | Call site         | The call expression's context        |
| Postcondition  | Definition site   | The procedure's `[[dynamic]]` status |
| Type invariant | Enforcement point | The expression's context             |
| Loop invariant | Enforcement point | The enclosing scope's context        |

**Dynamic Semantics**

**Runtime Check Failure**

When a runtime-checked predicate evaluates to `false`:
1. The runtime MUST trigger a Panic
2. The panic payload MUST include predicate text, source location, and contract type
3. Normal panic propagation rules apply

**Constraints**

**Diagnostics:** See Appendix A, code `P-SEM-2850`.


#### 14.7.2 Verification Facts

**Verification Fact.** A static guarantee that a predicate $P$ holds at program location $L$. Used for static analysis, contract elision, and type narrowing.

A Verification Fact is a triple $F(P, L, S)$ where:
- $P$ is a predicate expression
- $L$ is a program location (CFG node)
- $S$ is the source of the fact (control flow, runtime check, or assumption)

**Static Semantics**

**Zero-Size Property**

Verification Facts:
- Have zero runtime size
- Have no runtime representation
- MUST NOT be stored in variables, passed as parameters, or returned

**Fact Dominance**

**(Fact-Dominate)**

$$\frac{
    F(P, L) \in \text{Facts} \quad
    L \text{ dom } S \quad
    L \neq S
}{
    P \text{ satisfied at } S
}$$

**Fact Generation Rules**

| Construct                    | Generated Fact                  | Location             |
| :--------------------------- | :------------------------------ | :------------------- |
| `if P { ... }`               | $F(P, \_)$                      | Entry of then-branch |
| `if !P { } else { ... }`     | $F(P, \_)$                      | Entry of else-branch |
| `match x { Pat => ... }`     | $F(x \text{ matches } Pat, \_)$ | Entry of match arm   |
| Runtime check for $P$        | $F(P, \_)$                      | After check          |
| Loop invariant $Inv$ at exit | $F(Inv, \_)$                    | After loop           |


#### 14.7.3 Fact Propagation

**Static Semantics**

**Type Narrowing**

When a Verification Fact $F(P, L)$ is active for binding $x$ at location $L$:

$$\text{typeof}(x) \xrightarrow{F(P, L)} \text{typeof}(x) \texttt{ where } \{P\}$$

**Dynamic Semantics**

**Dynamic Fact Injection**

When a `[[dynamic]]` scope requires a runtime check and no static fact dominates:

1. Identify requirement $P$ at program point $S$
2. Construct check block: `if !P { panic("Contract violation: {P}") }`
3. Insert check into CFG such that it dominates $S$
4. Successful execution establishes $F(P, \text{exit}(C))$


### 14.8 Behavioral Subtyping


#### 14.8.1 Liskov Substitution Principle

**Liskov Substitution.** When a type implements a class, procedure implementations MUST adhere to the Liskov Substitution Principle with respect to class-defined contracts.

**Static Semantics**

**Precondition Weakening**

An implementation MAY weaken (require less than) the preconditions defined in the class. An implementation MUST NOT strengthen (require more than) the preconditions.

**Postcondition Strengthening**

An implementation MAY strengthen (guarantee more than) the postconditions defined in the class. An implementation MUST NOT weaken (guarantee less than) the postconditions.

**Verification Strategy**

Behavioral subtyping constraints are verified **statically at compile-time**:

1. **Precondition Check**: Verify that the implementation's precondition logically implies the class's precondition
2. **Postcondition Check**: Verify that the class's postcondition logically implies the implementation's postcondition

No runtime checks are generated for behavioral subtyping; violations are structural errors.


#### 14.8.2 Contract Inheritance

**Static Semantics**

```cursive
class Container {
    procedure get(~, idx: usize) -> i32
        |= idx < self~>len() => @result != 0
}

record MyList <: Container {
    procedure get(~, idx: usize) -> i32
        |= idx < self~>len() => @result > 0
    { self.data[idx] }
}
```

In this example, `@result > 0` implies `@result != 0`, so the postcondition is validly strengthened.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-SEM-2803`, `E-SEM-2804`.

---

## 15. Expressions

---

### 15.1 Evaluation Order


#### 15.1.1 Left-to-Right Evaluation

**Left-to-Right Evaluation.** The deterministic evaluation order for compound expressions in which subexpressions are evaluated in source order from left to right.

**Static Semantics**

For any compound expression, subexpressions MUST be evaluated strictly left-to-right. Given expression $e = f(e_1, e_2, \ldots, e_n)$:

1. $e_1$ is evaluated first
2. $e_2$ is evaluated after $e_1$ completes
3. Each $e_i$ is evaluated after $e_{i-1}$ completes
4. $f$ is applied after all arguments are evaluated

**Dynamic Semantics**

Side effects of $e_i$ MUST be observable before evaluation of $e_{i+1}$ begins. No reordering of observable effects is permitted.


#### 15.1.2 Short-Circuit Evaluation

**Short-Circuit Evaluation.** Conditionally skips evaluation of the right operand of logical operators based on the left operand's value.

**Static Semantics**

**(T-Logic-And)**
$$\frac{\Gamma \vdash e_1 : \texttt{bool} \quad \Gamma \vdash e_2 : \texttt{bool}}{\Gamma \vdash e_1\ \texttt{\&\&}\ e_2 : \texttt{bool}}$$

**(T-Logic-Or)**
$$\frac{\Gamma \vdash e_1 : \texttt{bool} \quad \Gamma \vdash e_2 : \texttt{bool}}{\Gamma \vdash e_1\ \texttt{||}\ e_2 : \texttt{bool}}$$

**Dynamic Semantics**

- `e₁ && e₂`: If `e₁` evaluates to `false`, `e₂` is NOT evaluated; the result is `false`.
- `e₁ || e₂`: If `e₁` evaluates to `true`, `e₂` is NOT evaluated; the result is `true`.


#### 15.1.3 Sequence Points

**Sequence Point.** A point in program execution where all preceding side effects are complete and no subsequent side effects have begun.

**Static Semantics**

Sequence points occur:

1. At the end of each statement
2. Between evaluation of function arguments and call entry
3. After evaluation of the condition in `if` and `loop` expressions
4. Between arms of short-circuit operators when the right operand is evaluated


### 15.2 Literals and Identifiers


#### 15.2.1 Literal Expressions

**Literal Expression.** A primary expression whose value is directly determined by its lexical form.

**Syntax**

```ebnf
literal ::= integer_literal | float_literal | string_literal
          | char_literal | bool_literal | null_literal
```

**Static Semantics**

Literal types are determined by:

1. **Integer literals**: Type suffix if present; otherwise inferred from context or defaults to `i32`
2. **Float literals**: Type suffix if present; otherwise inferred or defaults to `f64`
3. **String literals**: `string@View` (static storage)
4. **Character literals**: `char`
5. **Boolean literals**: `bool`
6. **Null literals**: Inferred from expected raw pointer type; ill-formed if no expected raw pointer type is available

Literals are value expressions.


#### 15.2.2 Identifier Expressions

**Identifier Expression.** A primary expression that references a binding by name.

**Syntax**

```ebnf
identifier_expr ::= identifier
```

**Static Semantics**

**(T-Ident)**
$$\frac{(x : T) \in \Gamma}{\Gamma \vdash x : T}$$

The identifier MUST resolve to a value binding per §6.2. If it resolves to a type, module, or class, the program is ill-formed, except as noted below.

**Record Default Exception**

If the identifier resolves to a record type and is used as the callee of a call expression with no arguments, the expression is well-formed and is typed by **(T-Record-Default)** in §11.6.5.

An identifier expression is a place expression if it refers to a `let` or `var` binding.

**Constraints**

**Diagnostics:** See Appendix A, code `E-SEM-2511`.


#### 15.2.3 Path Expressions

**Path Expression.** Accesses a named item through a qualified path.

**Syntax**

```ebnf
path_expr ::= type_path "::" identifier
type_path ::= identifier ( "::" identifier )*
```

**Static Semantics**

Path expressions resolve through the module hierarchy per §6.2. The final component MUST be a value (constant, procedure, or enum variant constructor), except as noted below.

**Record Default Exception**

If the final component resolves to a record type and the path expression is used as the callee of a call expression with no arguments, the expression is well-formed and is typed by **(T-Record-Default)** in §11.6.5.


### 15.3 Access Expressions


#### 15.3.1 Field Access (`.`)

**Field Access.** Retrieves a named field from a record value. See §11.6.2 for record field declaration and access semantics.

**Static Semantics**

**(T-Field)** See §11.6.2 for the normative typing rule.

Value category propagates: field access on a place expression yields a place expression.

**Constraints**

See §11.6.2 for field access diagnostic codes (`E-TYP-1904`, `E-TYP-1905`).

**Diagnostics:** See Appendix A, code `E-SEM-2521`.


#### 15.3.2 Tuple Element Access (`.0`, `.1`, ...)

**Tuple Element Access.** Retrieves an element from a tuple by positional index. See §11.2.2 for tuple element access semantics.

**Static Semantics**

**(T-Tuple-Index)** See §11.2.2 for the normative typing rule.

**Constraints**

See §11.2.2 for tuple index diagnostic codes (`E-TYP-1801`, `E-TYP-1802`, `E-TYP-1803`).

**Diagnostics:** See Appendix A, code `E-SEM-2524`.


#### 15.3.3 Index Access (`[]`)

**Indexing Expression.** Retrieves an element from an array or slice by computed index.

**Syntax**

```ebnf
index_expr ::= postfix_expr "[" expression "]"
```

**Static Semantics**

**(T-Index-Array)**
$$\frac{\Gamma \vdash e_1 : [T; N] \quad \Gamma \vdash e_2 : \texttt{usize}}{\Gamma \vdash e_1[e_2] : T}$$

**(T-Index-Slice)**
$$\frac{\Gamma \vdash e_1 : [T] \quad \Gamma \vdash e_2 : \texttt{usize}}{\Gamma \vdash e_1[e_2] : T}$$

The index expression MUST have type `usize`. No implicit integer conversions are performed.

**Dynamic Semantics**

All indexing operations MUST be bounds-checked at runtime. If $\text{index} \ge \text{length}$, the executing thread MUST panic.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-SEM-2527`, `E-SEM-2528`, `P-SEM-2530`.


### 15.4 Calls and Operators

**Syntax**

```ebnf
unattributed_expr ::= range_expression | logical_or_expr

logical_or_expr  ::= logical_and_expr ("||" logical_and_expr)*
logical_and_expr ::= comparison_expr ("&&" comparison_expr)*

comparison_expr ::= bitwise_or_expr (comparison_op bitwise_or_expr)*
comparison_op   ::= "==" | "!=" | "<" | "<=" | ">" | ">="

bitwise_or_expr  ::= bitwise_xor_expr ("|" bitwise_xor_expr)*
bitwise_xor_expr ::= bitwise_and_expr ("^" bitwise_and_expr)*
bitwise_and_expr ::= shift_expr ("&" shift_expr)*

shift_expr ::= additive_expr (shift_op additive_expr)*
shift_op   ::= "<<" | ">>"

additive_expr ::= multiplicative_expr (additive_op multiplicative_expr)*
additive_op   ::= "+" | "-"

multiplicative_expr ::= power_expr (multiplicative_op power_expr)*
multiplicative_op   ::= "*" | "/" | "%"

power_expr ::= cast_expr ("**" power_expr)?

postfix_expr   ::= primary_expr postfix_suffix*
postfix_suffix ::= "." identifier
                | "." decimal_literal
                | "[" expression "]"
                | "(" argument_list? ")"
                | "~>" identifier "(" argument_list? ")"
                | "?"

primary_expr ::= literal
               | identifier_expr
               | path_expr
               | tuple_literal
               | array_literal
               | record_literal
               | record_update
               | enum_literal
               | null_ptr_expr
               | transmute_expr
               | alloc_expr
               | key_path_expr
               | if_expr
               | match_expr
               | loop_expr
               | block_expr
               | closure_expr
               | spawn_expr
               | dispatch_expr
               | yield_expr
               | yield_from_expr
               | sync_expr
               | race_expr
               | all_expr
               | comptime_expr
               | quote_expr
               | quote_type
               | quote_pattern
               | splice_expr
               | "(" expression ")"
```


#### 15.4.1 Procedure Calls

**Procedure Call.** Invokes a callable value with a list of arguments.

**Syntax**

```ebnf
call_expr ::= postfix_expr "(" argument_list? ")"
argument_list ::= argument ( "," argument )* ","?
argument ::= expression
```

**Static Semantics**

**(T-Call)**
$$\frac{\Gamma \vdash f : (m_1\ T_1, \ldots, m_n\ T_n) \to R \quad \forall i,\ \Gamma \vdash a_i : T_i}{\Gamma \vdash f(a_1, \ldots, a_n) : R}$$

**Argument Passing Rules**

1. The number of arguments MUST equal the number of parameters
2. Each argument type MUST be compatible with the parameter type per §9.2
3. If a parameter has the `move` modifier, the corresponding argument MUST be a move expression (`move` applied to a place expression)
4. Arguments are evaluated left-to-right before control transfers to the callee

**Constraints**

**Diagnostics:** See Appendix A, codes `E-SEM-2531`–`E-SEM-2535`.


#### 15.4.2 Method Calls

**Method Call.** Invokes a procedure associated with a type using instance dispatch (`~>`) or static dispatch (`::`).

**Syntax**

```ebnf
method_call ::= postfix_expr "~>" identifier "(" argument_list? ")"
static_call ::= type_path "::" identifier "(" argument_list? ")"
             | null_ptr_expr
```

**Static Semantics**

**(T-Method-Instance)**
$$\frac{\Gamma \vdash r : T \quad \text{method } m(\text{self} : T', \ldots) \to R \in \text{methods}(T) \quad T <: T'}{\Gamma \vdash r \mathord{\sim>} m(\ldots) : R}$$

**Receiver Dispatch Algorithm**

1. Search for method `m` in the inherent methods of receiver type $T$
2. If not found, search in all classes implemented by $T$ that are visible
3. If multiple classes provide method `m`, disambiguation via `Class::m(receiver, ...)` is required
4. The receiver type MUST match the method's `self` parameter type exactly

**Constraints**

**Diagnostics:** See Appendix A, codes `E-SEM-2536`, `E-SEM-2537`, `E-MOD-1307`.


#### 15.4.3 Pipeline Expressions (`=>`)

**Pipeline Expression.** Provides left-to-right function composition where the left operand becomes the first argument to the right operand.

**Syntax**

```ebnf
pipeline_expr ::= postfix_expr ("=>" postfix_expr)*
```

**Static Semantics**

**(T-Pipeline)**
$$\frac{\Gamma \vdash e_1 : T_1 \quad \Gamma \vdash e_2 : (T_1, \ldots) \to R}{\Gamma \vdash e_1 \Rightarrow e_2 : R}$$

*[Informative Desugaring]*

The pipeline expression `x => f` desugars to `f(x)`:

$$e_1 \Rightarrow e_2 \equiv e_2(e_1)$$

Pipelines are left-associative: `x => f => g => h` is equivalent to `h(g(f(x)))`.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-SEM-2538`, `E-SEM-2539`.


#### 15.4.4 Unary Operators

**Unary Operators.** Apply a single-operand operation. All unary operators have precedence 3 and are right-associative.

**Syntax**

```ebnf
unary_expr ::= unary_operator unary_expr | pipeline_expr
unary_operator ::= "!" | "-" | "&" | "*" | "^" | "move" | "widen"
```

**Static Semantics**

**(T-Not-Bool)** Logical negation:
$$\frac{\Gamma \vdash e : \texttt{bool}}{\Gamma \vdash !e : \texttt{bool}}$$

**(T-Not-Int)** Bitwise complement:
$$\frac{\Gamma \vdash e : I \quad I \in \mathcal{T}_{\text{int}}}{\Gamma \vdash !e : I}$$

**(T-Neg)** Numeric negation:
$$\frac{\Gamma \vdash e : N \quad N \in \mathcal{T}_{\text{signed}} \cup \mathcal{T}_{\text{float}}}{\Gamma \vdash -e : N}$$

Negation of unsigned integers is ill-formed.

**Address-Of (`&`)**: See §12.3 for normative typing rule **(T-Addr-Of)**.

**Dereference (`*`)**: See §12.3 for normative typing rule **(T-Deref)**.

**Region Allocation (`^`)**: See §16.7 for normative typing rules.

**Move (`move`)**: See §3.4 for normative typing rule **(T-Move)**.

**Modal Widening (`widen`)**: See §12.1 for normative typing rule **(T-Modal-Widen)**.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-SEM-2541`–`E-SEM-2545`, `E-MEM-3021`.


#### 15.4.5 Binary Operators

**Binary Operators.** Combine two operands with an infix operator.

**Static Semantics**

**(T-Arith)** Arithmetic operators (`+`, `-`, `*`, `/`, `%`, `**`):
$$\frac{\Gamma \vdash e_1 : N \quad \Gamma \vdash e_2 : N \quad N \in \mathcal{T}_{\text{numeric}}}{\Gamma \vdash e_1 \oplus e_2 : N}$$

Arithmetic operators require homogeneous operand types. No implicit numeric promotions.

**(T-Bitwise)** Bitwise operators (`&`, `|`, `^`):
$$\frac{\Gamma \vdash e_1 : I \quad \Gamma \vdash e_2 : I \quad I \in \mathcal{T}_{\text{int}}}{\Gamma \vdash e_1 \oplus e_2 : I}$$

**(T-Shift)** Shift operators (`<<`, `>>`):
$$\frac{\Gamma \vdash e_1 : I \quad \Gamma \vdash e_2 : \texttt{u32} \quad I \in \mathcal{T}_{\text{int}}}{\Gamma \vdash e_1 \ll e_2 : I}$$

**(T-Compare)** Comparison operators (`==`, `!=`, `<`, `<=`, `>`, `>=`):
$$\frac{\Gamma \vdash e_1 : T \quad \Gamma \vdash e_2 : T}{\Gamma \vdash e_1\ \text{cmp}\ e_2 : \texttt{bool}}$$

**Dynamic Semantics**

**Integer Overflow**: Arithmetic operations MUST panic on overflow in debug mode. Release mode behavior is Implementation-Defined per §11.1.

**Division by Zero**: MUST panic.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-SEM-2551`–`E-SEM-2556`, `P-SEM-2560`, `P-SEM-2561`, `I-SEM-2562`.


#### 15.4.6 Operator Precedence Table

**Operator Precedence.** Determines binding strength; higher precedence binds more tightly.

**Static Semantics**

| Precedence | Operators                          | Associativity |
| :--------- | :--------------------------------- | :------------ |
| 1          | `()` `[]` `.` `~>` `::`            | Left          |
| 2          | `=>` (pipeline)                    | Left          |
| 3          | `!` `-` `&` `*` `^` `move` `widen` | Right (unary) |
| 4          | `as`                               | Left          |
| 5          | `**`                               | Right         |
| 6          | `*` `/` `%`                        | Left          |
| 7          | `+` `-`                            | Left          |
| 8          | `<<` `>>`                          | Left          |
| 9          | `&` (bitwise)                      | Left          |
| 10         | `^` (bitwise)                      | Left          |
| 11         | `\|` (bitwise)                     | Left          |
| 12         | `==` `!=` `<` `<=` `>` `>=`        | Left          |
| 13         | `&&`                               | Left          |
| 14         | `\|\|`                             | Left          |
| 15         | `..` `..=`                         | Left          |
| 16         | `=` `+=` `-=` etc.                 | Right         |

**Operator Ambiguity Resolution**

| Operator | Unary Role (Prec. 3) | Binary Role              |
| :------- | :------------------- | :----------------------- |
| `-`      | Numeric negation     | Subtraction (Prec. 7)    |
| `&`      | Address-of           | Bitwise AND (Prec. 9)    |
| `*`      | Dereference          | Multiplication (Prec. 6) |
| `^`      | Region allocation    | Bitwise XOR (Prec. 10)   |

Disambiguation is syntactic: if the operator appears after a complete operand expression, it is binary; otherwise, it is unary.


### 15.5 Casts


#### 15.5.1 `as` Cast Expression

**Cast Expression.** Explicitly converts a value from one type to another using the `as` operator.

**Syntax**

```ebnf
cast_expr ::= unary_expr "as" type
```

The `as` operator has precedence 4 and is left-associative.

**Static Semantics**

**(T-Cast)**
$$\frac{\Gamma \vdash e : S \quad \text{CastValid}(S, T)}{\Gamma \vdash e\ \texttt{as}\ T : T}$$


#### 15.5.2 Safe Casts

**Safe Casts.** Type conversions that do not require `unsafe` context.

**Static Semantics**

**Cast Categories (Safe)**

| Category              | Source Type | Target Type        | Semantics                   |
| :-------------------- | :---------- | :----------------- | :-------------------------- |
| **Numeric Widening**  | `iN`        | `iM` where `M > N` | Sign-extended               |
|                       | `uN`        | `uM` where `M > N` | Zero-extended               |
|                       | `fN`        | `fM` where `M > N` | Precision-extended          |
| **Numeric Narrowing** | `iN`        | `iM` where `M < N` | Truncated (low-order bits)  |
|                       | `uN`        | `uM` where `M < N` | Truncated (low-order bits)  |
|                       | `fN`        | `fM` where `M < N` | Rounded to nearest          |
| **Sign Conversion**   | `iN`        | `uN`               | Bit reinterpretation        |
|                       | `uN`        | `iN`               | Bit reinterpretation        |
| **Integer ↔ Float**   | `iN`/`uN`   | `fM`               | Nearest representable value |
|                       | `fM`        | `iN`/`uN`          | Truncate toward zero†       |
| **Bool → Integer**    | `bool`      | Any integer        | `false`→0, `true`→1         |
| **Char ↔ Integer**    | `char`      | `u32`              | Unicode scalar value        |
|                       | `u8`        | `char`             | Always valid (ASCII subset) |

† Float-to-integer casts that overflow or produce NaN trigger a panic.

**Dynamic Semantics**

**Widening Casts**: Always lossless.

**Narrowing Casts**: Low-order bits preserved; high-order bits discarded. No panic; information loss is silent.

**Float-to-Integer Casts**: Truncate toward zero. Panic on overflow, NaN, or ±Infinity.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-SEM-2571`, `P-SEM-2580`–`P-SEM-2582`.


#### 15.5.3 Unsafe Casts

**Unsafe Casts.** Type conversions requiring `unsafe` context.

**Static Semantics**

**Cast Categories (Unsafe)**

| Category              | Source Type         | Target Type         | Context Required |
| :-------------------- | :------------------ | :------------------ | :--------------- |
| **Pointer ↔ Integer** | `usize`             | `*imm T` / `*mut T` | `unsafe`         |
|                       | `*imm T` / `*mut T` | `usize`             | `unsafe`         |
| **Safe → Raw Ptr**    | `Ptr<T>@Valid`      | `*imm T` / `*mut T` | `unsafe`         |
| **Enum → Integer**    | `enum E`            | Integer type        | `[[layout]]` req |

**Constraints**

**Diagnostics:** See Appendix A, codes `E-SEM-2572`–`E-SEM-2574`.


### 15.6 Closures


#### 15.6.1 Closure Syntax

**Closure Expression.** Creates an anonymous callable value that may capture bindings from its enclosing lexical scope.

**Syntax**

```ebnf
closure_expr ::= "|" closure_param_list? "|" ("->" type)? closure_body
closure_param_list ::= closure_param ( "," closure_param )* ","?
closure_param ::= "move"? identifier (":" type)?
closure_body ::= expression | block_expr
```

**Static Semantics**

**Non-Capturing Closure**: References no bindings from enclosing scope; has sparse function pointer type; FFI-compatible.

**Capturing Closure**: References bindings from enclosing scope; has closure type.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-SEM-2591`, `E-SEM-2594`.


#### 15.6.2 Capture Modes (Bitcopy, Move, Reference)

*[REF: Move capture applies §3.4 semantics to closure environment.]*

**Capture Modes.** Closures capture bindings according to the binding's permission.

**Static Semantics**

| Binding Permission | Capture Mode  | Closure Environment Contains        |
| :----------------- | :------------ | :---------------------------------- |
| `const`            | By reference  | `const` reference to binding        |
| `unique`           | Move required | Owned value (binding becomes Moved) |
| `shared`           | By reference  | `shared` reference to binding       |

A binding with `unique` permission MUST be captured via explicit `move`.

**Constraints**

**Diagnostics:** See Appendix A, code `E-SEM-2593`.


#### 15.6.3 Closure Type Inference

*[REF: Applies bidirectional inference per §9.4.]*

**Static Semantics**

**Parameter Typing**

1. If a parameter has explicit type annotation, that type is used
2. If checked against expected function type, infer from expected type
3. If no annotation and no expected type, the program is ill-formed

**Return Type Inference**

1. If return type is annotated, body is checked against that type
2. If omitted, inferred from body expression type


### 15.7 Pattern Matching


#### 15.7.1 Pattern Syntax

**Pattern.** A syntactic form that tests value shape and binds parts to identifiers.

**Syntax**

```ebnf
pattern ::= literal_pattern | wildcard_pattern | identifier_pattern
          | tuple_pattern | record_pattern | enum_pattern
          | modal_pattern | range_pattern

literal_pattern     ::= literal
wildcard_pattern    ::= "_"
identifier_pattern  ::= identifier
tuple_pattern       ::= "(" tuple_pattern_elements? ")"
tuple_pattern_elements ::= pattern ("," pattern)* ","?

field_pattern_list  ::= field_pattern ("," field_pattern)* ","?
field_pattern       ::= identifier ":" pattern | identifier

record_pattern      ::= type_path "{" field_pattern_list? "}"
enum_pattern        ::= type_path "::" identifier enum_payload_pattern?
enum_payload_pattern ::= "(" tuple_pattern_elements? ")" | "{" field_pattern_list? "}"
modal_pattern       ::= "@" identifier ("{" field_pattern_list? "}")?
range_pattern       ::= pattern ( ".." | "..=" ) pattern
```

**Static Semantics**

**(T-Pat-Wildcard)**
$$\frac{}{\Gamma \vdash \_ : T \Rightarrow \Gamma}$$

**(T-Pat-Ident)**
$$\frac{x \notin \text{dom}(\Gamma)}{\Gamma \vdash x : T \Rightarrow \Gamma, x : T}$$

**(T-Pat-Tuple)**
$$\frac{\Gamma_0 = \Gamma \quad \forall i,\ \Gamma_{i-1} \vdash p_i : T_i \Rightarrow \Gamma_i}{\Gamma \vdash (p_1, \ldots, p_n) : (T_1, \ldots, T_n) \Rightarrow \Gamma_n}$$


#### 15.7.2 Literal Patterns

**Literal Pattern.** Matches if the scrutinee equals the literal value.

**Static Semantics**

Matches if `scrutinee == literal`. The literal type MUST be compatible with the scrutinee type.


#### 15.7.3 Binding Patterns

**Identifier Pattern.** Matches any value and introduces a binding.

**Static Semantics**

Binding semantics depend on scrutinee permission:

- **`unique` scrutinee**: Value is moved; responsibility transfers to new binding
- **`shared` scrutinee**: Binding receives `shared` view with synchronized access
- **`const` scrutinee**: Binding receives `const` view; if `Bitcopy`, value is copied

The `let`/`var` keyword determines only reassignability, not transfer semantics.


#### 15.7.4 Destructuring Patterns

**Destructuring Patterns.** Decompose composite values into their components.

**Static Semantics**

**(T-Pat-Record)**
$$\frac{R = \texttt{record} \{ f_1 : T_1, \ldots \} \quad \forall j,\ \Gamma_{j-1} \vdash p_j : T_{f_j} \Rightarrow \Gamma_j}{\Gamma \vdash R \{ f_{j_1} : p_1, \ldots \} : R \Rightarrow \Gamma_{|J|}}$$

**(T-Pat-Enum)**
$$\frac{E = \texttt{enum} \{ \ldots, V(T_1, \ldots, T_m), \ldots \} \quad \forall i,\ \Gamma_{i-1} \vdash p_i : T_i \Rightarrow \Gamma_i}{\Gamma \vdash E::V(p_1, \ldots, p_m) : E \Rightarrow \Gamma_m}$$


#### 15.7.5 Range Patterns

**Range Pattern.** Matches values within a range.

**Static Semantics**

- **Exclusive (`start..end`)**: Matches if $\text{start} \le \text{scrutinee} < \text{end}$
- **Inclusive (`start..=end`)**: Matches if $\text{start} \le \text{scrutinee} \le \text{end}$

Both `start` and `end` MUST be compile-time constant expressions.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-SEM-2721`, `E-SEM-2722`.


#### 15.7.6 Guard Clauses

**Guard Clause.** An additional boolean condition on a match arm.

**Syntax**
*[REF: `match_arm` syntax is defined in §15.8.2.]*

**Static Semantics**

1. Guard MUST have type `bool`
2. Evaluated only if pattern matches
3. If guard evaluates to `false`, matching continues with next arm
4. Guard MAY reference pattern bindings

Guards do not affect exhaustiveness checking.

**Constraints**

**Diagnostics:** See Appendix A, code `E-SEM-2612`.


### 15.8 Conditionals


#### 15.8.1 `if` Expressions

**`if` Expression.** Evaluates one of two branches based on a boolean condition.

**Syntax**

```ebnf
if_expr ::= "if" expression block_expr ("else" (block_expr | if_expr))?
```

**Static Semantics**

**(T-If)**
$$\frac{\Gamma \vdash e_c : \texttt{bool} \quad \Gamma \vdash e_t : T \quad \Gamma \vdash e_f : T}{\Gamma \vdash \texttt{if } e_c\ \{ e_t \}\ \texttt{else}\ \{ e_f \} : T}$$

**(T-If-No-Else)**
$$\frac{\Gamma \vdash e_c : \texttt{bool} \quad \Gamma \vdash e_t : ()}{\Gamma \vdash \texttt{if } e_c\ \{ e_t \} : ()}$$

**Constraints**

**Diagnostics:** See Appendix A, codes `E-SEM-2601`–`E-SEM-2603`.


#### 15.8.2 `match` Expressions

*[REF: Pattern syntax defined in §15.7.]*

**`match` Expression.** Performs exhaustive pattern matching on a scrutinee value.

**Syntax**

```ebnf
match_expr ::= "match" expression "{" match_arm+ "}"
match_arm ::= pattern ("if" expression)? "=>" arm_body ","
arm_body ::= expression | block_expr
```

**Static Semantics**

**(T-Match)**
$$\frac{\Gamma \vdash e : T_s \quad \forall i,\ \Gamma, \text{bindings}(p_i) \vdash e_i : T \quad \text{exhaustive}(\{p_i\}, T_s)}{\Gamma \vdash \texttt{match } e\ \{ p_1 \Rightarrow e_1, \ldots \} : T}$$

**Constraints**

**Diagnostics:** See Appendix A, codes `E-SEM-2611`, `E-SEM-2741`, `E-SEM-2751`.


#### 15.8.3 Match Exhaustiveness

**Exhaustiveness.** A `match` expression MUST be exhaustive: the set of patterns MUST cover every possible value of the scrutinee type.

**Static Semantics**

**Exhaustiveness Checking Requirements**

- `enum` types: All variants MUST be covered
- `modal` types: All states MUST be covered
- `bool` type: Both `true` and `false` MUST be covered
- Integer types with range patterns: Union of ranges MUST cover all values, OR wildcard present

**Exhaustiveness Algorithm**

1. Construct set of all possible value shapes for scrutinee type
2. For each pattern arm, remove covered shapes
3. If shapes remain uncovered, match is non-exhaustive

**Unreachability**

An arm is unreachable if its pattern covers only values already covered by preceding arms.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-SEM-2711`–`E-SEM-2713`, `E-SEM-2731`.


### 15.9 Loops


#### 15.9.1 Infinite Loop (`loop`)

**Infinite Loop.** Executes its body repeatedly until terminated by `break` or `return`.

**Syntax**

```ebnf
loop_expr ::= label? "loop" block_expr
```

**Static Semantics**

**(T-Loop-Infinite)**
$$\frac{\Gamma \vdash e : T}{\Gamma \vdash \texttt{loop}\ \{ e \} : !}$$


#### 15.9.2 Conditional Loop (`loop condition`)

**Conditional Loop.** Evaluates its body while the condition is true.

**Syntax**

```ebnf
cond_loop ::= label? "loop" expression block_expr
```

**Static Semantics**

**(T-Loop-Conditional)**
$$\frac{\Gamma \vdash e_c : \texttt{bool} \quad \Gamma \vdash e_b : ()}{\Gamma \vdash \texttt{loop } e_c\ \{ e_b \} : ()}$$

**Dynamic Semantics**

*[Informative Desugaring]*

$$\texttt{loop}\ C\ \{\ B\ \} \quad \rightarrow \quad \texttt{loop}\ \{\ \texttt{if}\ !C\ \{\ \texttt{break}\ \}\ B\ \}$$


#### 15.9.3 Range Loop (`loop x in range`)

**Range Loop.** Iterates over values produced by an iterator.

**Syntax**

```ebnf
iter_loop ::= label? "loop" pattern (":" type)? "in" expression block_expr
```

**Static Semantics**

**(T-Loop-Iterator)**
$$\frac{\Gamma \vdash e_{iter} : I \quad I : \texttt{Iterator}\langle\texttt{Item} = T\rangle \quad \Gamma, x : T \vdash e_b : ()}{\Gamma \vdash \texttt{loop } x : T\ \texttt{in } e_{iter}\ \{ e_b \} : ()}$$

**Dynamic Semantics**

**Desugaring**

$$\texttt{loop}\ x: T\ \texttt{in}\ iter\ \{\ B\ \} \quad \rightarrow \quad \{\ \texttt{var}\ it = iter;\ \texttt{loop}\ \{\ \texttt{match}\ it\texttt{\sim>next()}\ \{\ x : T \Rightarrow B,\ u : () \Rightarrow \texttt{break}\ \}\ \}\ \}$$

**Constraints**

**Diagnostics:** See Appendix A, code `E-SEM-2621`.


#### 15.9.4 Loop Labels

**Loop Label.** Names a loop for targeted `break` or `continue`.

**Syntax**

```ebnf
label ::= "'" identifier ":"
```

**Static Semantics**

Labels are visible within the labeled loop's body. Nested loops MAY reference outer labels.


#### 15.9.5 `break` with Values

**`break` with Values.** Terminates the innermost (or labeled) loop, optionally providing a result value.

**Syntax**
*[REF: `break_stmt` syntax is defined in §16.5.3.]*

**Static Semantics**

1. `break` MUST appear within a `loop` body
2. If label provided, that loop is terminated
3. If expression provided, value becomes loop result
4. All `break` statements for same loop MUST provide compatible types

**Constraints**

**Diagnostics:** See Appendix A, code `E-SEM-2622`.


#### 15.9.6 `continue`

**`continue`.** Skips to the next iteration of the innermost (or labeled) loop.

**Syntax**
*[REF: `continue_stmt` syntax is defined in §16.5.4.]*

**Static Semantics**

1. `continue` MUST appear within a `loop` body
2. If label provided, that loop's next iteration begins

---

## 16. Statements

---

### 16.1 Blocks


#### 16.1.1 Block Syntax

**Block.** A brace-delimited sequence of statements optionally followed by a result expression.

**Syntax**

```ebnf
block_expr ::= "{" statement* expression? "}"

statement  ::= binding_decl terminator
             | assignment_stmt terminator
             | compound_assign terminator
             | return_stmt terminator
             | result_stmt terminator
             | break_stmt terminator
             | continue_stmt terminator
             | emit_stmt terminator
             | expr_stmt
             | defer_stmt
             | region_stmt
             | frame_stmt
```


#### 16.1.2 Block Scope

**Block Scope.** A block introduces a new lexical scope. Bindings declared within a block are visible only within that block and its nested scopes.

**Static Semantics**

1. Bindings introduced within a block are visible from their declaration point to the block's closing brace.
2. A binding MAY shadow an outer binding only via an explicit `shadow` declaration (§6.2.2).
3. When a block exits, all bindings introduced within it are destroyed per LIFO semantics (§3.5.2).


#### 16.1.3 Block Result Values

**Block Result.** A block evaluates to a value determined by its contents.

**Static Semantics**

**Block Type Determination**

The type of a block expression is determined by:

1. The type of an explicit `result` statement, if present, OR
2. The type of the final unterminated expression, if present, OR
3. The unit type `()` if the block is empty or ends with a terminated statement.

**Note:** While tail expressions determine block types, procedure and method bodies with non-`()` return types MUST use explicit `return` statements. Tail expressions MAY provide values for local block expressions (such as `if`/`match` arms), but MUST NOT serve as implicit procedure returns for non-unit types.

**(T-Block-Result)**
$$\frac{\Gamma \vdash s_1; \ldots; s_n \quad \Gamma \vdash e : T}{\Gamma \vdash \{ s_1; \ldots; s_n;\ e \} : T}$$

**(T-Block-Unit)**
$$\frac{\Gamma \vdash s_1; \ldots; s_n}{\Gamma \vdash \{ s_1; \ldots; s_n; \} : ()}$$

**Dynamic Semantics**

1. Statements execute sequentially in source order.
2. If a `result` statement is executed, the block immediately evaluates to that value.
3. If the block ends with an unterminated expression, that expression's value is the block result.
4. If the block ends with a terminated statement, the block result is `()`.
5. Upon block exit (normal or via control flow), cleanup actions execute per LIFO semantics (§3.5.2, §16.6.2).


### 16.2 Declarations


#### 16.2.1 `let` Declarations

**`let` Declaration.** Introduces an immutable binding.

**Syntax**
*[REF: Declaration binding syntax is defined in §3.3.6.]*

**Static Semantics**

The binding introduced by `let` cannot be reassigned. The binding operator (`=` vs `:=`) determines movability per §3.3.6.


#### 16.2.2 `var` Declarations

**`var` Declaration.** Introduces a mutable binding that permits reassignment.

**Syntax**
*[REF: Declaration binding syntax is defined in §3.3.6.]*

**Static Semantics**

The binding introduced by `var` can be reassigned. The binding operator (`=` vs `:=`) determines movability per §3.3.6.


#### 16.2.3 Binding Patterns in Declarations

**Binding Patterns.** Declaration statements support pattern destructuring.

**Static Semantics**

The pattern in a declaration statement MUST be irrefutable (§15.7). Refutable patterns are ill-formed.

**Dynamic Semantics**

1. The initializer expression is evaluated.
2. The value is bound to the pattern, introducing bindings into scope.
3. For destructuring patterns, components are bound to their respective identifiers.


#### 16.2.4 Type Annotations

**Type Annotation.** An explicit specification of the type of a binding.

**Static Semantics**

If the type annotation is omitted, the type is inferred from the initializer expression. If the type cannot be inferred, the program is ill-formed.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-MOD-2401`, `E-MOD-2402`.

For `E-SEM-2711` (refutable pattern in irrefutable context), see §15.7.


### 16.3 Assignment


#### 16.3.1 Simple Assignment

**Simple Assignment.** An assignment that stores a value in a place expression.

**Syntax**

```ebnf
assignment_stmt ::= place_expr "=" expression
```

**Static Semantics**

1. The left-hand side MUST be a place expression (§3.4.2).
2. The place MUST refer to a mutable binding (declared with `var`).
3. The place MUST be accessible via `unique` permission, or via `shared` permission with Write access available per the key system (§17). Assignment through `const` permission is forbidden.
4. The right-hand side type MUST be compatible with the place's type.


#### 16.3.2 Compound Assignment (`+=`, `-=`, etc.)

**Compound Assignment.** An assignment that combines an arithmetic or bitwise operation with assignment.

**Syntax**

```ebnf
compound_assign ::= place_expr compound_op expression
compound_op     ::= "+=" | "-=" | "*=" | "/=" | "%="
                  | "&=" | "|=" | "^=" | "<<=" | ">>="
```

**Static Semantics**

Compound assignment `place op= expr` desugars to `place = place op expr`, except that `place` is evaluated only once.


#### 16.3.3 Tuple Assignment

**Tuple Assignment.** An assignment that destructures a tuple value into multiple place expressions.

**Static Semantics**

Each component of the tuple is assigned to its corresponding place expression. All places MUST satisfy the requirements of §16.3.4.


#### 16.3.4 Place Expression Requirements

**Place Expression.** An expression that denotes a memory location that can be assigned.

**Static Semantics**

An assignment target MUST be:

1. A place expression (identifier, field access, index access, or dereference)
2. Bound with `var` (mutable)
3. Accessible via `unique` permission, or via `shared` permission with Write access available per the key system (§17)

**Movability Preservation**

The movability property of a binding is determined at declaration and preserved across reassignments:

- A `var x = v` binding remains movable after reassignment.
- A `var x := v` binding remains immovable after reassignment; the new value is also immovable.

**Dynamic Semantics**

**Drop on Reassignment**

Reassignment of a responsible binding triggers deterministic destruction of the prior value per §3.5.

Execution order:
1. The new value expression is evaluated.
2. The old value's destructor is invoked per §3.5.
3. The new value is installed.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-SEM-3131`–`E-SEM-3133`.

For `E-MOD-2401` (assignment to immutable `let` binding), see §16.2.2.


### 16.4 Expression Statements


#### 16.4.1 Expression as Statement

**Expression Statement.** A statement that evaluates an expression for its side effects; the resulting value is discarded.

**Syntax**

```ebnf
expr_stmt ::= expression terminator
```

**Static Semantics**

The type of an expression statement is the unit type `()`. The expression's value is discarded after evaluation.


#### 16.4.2 Statement Termination

**Statement Terminator.** A token that marks the end of a statement.

**Syntax**

```ebnf
terminator ::= ";" | newline
newline    ::= "\n"
```

**Dynamic Semantics**

1. The expression is evaluated.
2. If the expression produces a value, that value is discarded.
3. If the expression's result has a destructor and the value is not moved elsewhere, the destructor is invoked at statement end.


### 16.5 Control Flow Statements

**Syntax**

```ebnf
return_stmt   ::= "return" expression?
result_stmt   ::= "result" expression
break_stmt    ::= "break" ("'" identifier)? expression?
continue_stmt ::= "continue" ("'" identifier)?
```

**`return` Statement**

The `return` statement terminates the current procedure and returns control to the caller.

1. If an expression is provided, its type MUST match the procedure's declared return type.
2. If no expression is provided, the procedure's return type MUST be `()`.
3. `return` MUST NOT appear at module scope.
4. Deferred actions in all scopes between the `return` and procedure entry execute per LIFO semantics (§3.5.2).
5. Procedures and methods with non-`()` return types MUST terminate with an explicit `return` statement. Tail expressions (unterminated final expressions) MUST NOT serve as implicit returns for non-unit return types.

**`result` Statement**

The `result` statement terminates the current block and yields a value as the block's result.

1. The expression type becomes (or must match) the enclosing block's type.
2. `result` exits only the immediately enclosing block, not the procedure.
3. Deferred actions in the exited block execute per LIFO semantics (§3.5.2).

**`break` Statement**

The `break` statement terminates the innermost (or labeled) loop.

1. `break` MUST appear within a `loop` body.
2. If a label is provided, the labeled loop is terminated.
3. If an expression is provided, the value becomes the loop's result.
4. All `break` statements for the same loop MUST provide values of the same type (or all omit values).
5. Deferred actions in scopes between the statement and the target loop execute per LIFO semantics (§3.5.2).

**`continue` Statement**

The `continue` statement skips to the next iteration of the innermost (or labeled) loop.

1. `continue` MUST appear within a `loop` body.
2. If a label is provided, the labeled loop's next iteration begins.
3. Deferred actions in scopes between the statement and the target loop execute per LIFO semantics (§3.5.2).

**Diagnostics:** See Appendix A, codes `E-SEM-3161`–`E-SEM-3166`.


### 16.6 Defer

**Defer Statement.** A statement that schedules a block of code for execution when the enclosing scope exits.

**Syntax**

```ebnf
defer_stmt ::= "defer" block_expr
```

**Static Semantics**

**(T-Defer)**
$$\frac{\Gamma \vdash e : ()}{\Gamma \vdash \texttt{defer}\ \{ e \} : \text{stmt}}$$

**Constraints:**

1. The deferred block MUST NOT contain control flow that transfers outside the defer block:
   - `return` is prohibited within a defer block.
   - `break` targeting a label outside the defer block is prohibited.
   - `continue` targeting a label outside the defer block is prohibited.
   - `result` targeting a block outside the defer block is prohibited.
2. `break` and `continue` targeting constructs contained within the defer block are permitted.
3. The deferred block MAY reference bindings from the enclosing scope that are in scope at the `defer` statement.

**Dynamic Semantics**

When a `defer` statement is executed, the block is pushed onto a per-scope cleanup stack. When a scope exits—by normal completion, `return`, `result`, `break`, or panic—all cleanup actions execute in Last-In, First-Out (LIFO) order based on source position. Defer blocks and binding destructions are interleaved in a single unified stack.

**Panic Handling**

If a panic occurs during execution of a deferred block:
1. The remaining deferred blocks in that scope are still executed.
2. After all deferred blocks complete, the panic propagates.
3. If multiple panics occur during cleanup, behavior is Implementation-Defined (IDB).

**Diagnostics:** See Appendix A, codes `E-SEM-3151`, `E-SEM-3152`.


### 16.7 Regions


#### 16.7.1 `region` Block Syntax

**Region Block.** A block that provides arena-style memory allocation with deterministic bulk deallocation.

**Syntax**

```ebnf
region_stmt    ::= "region" region_options? region_alias? block
region_options ::= region_option*
region_option  ::= "." "stack_size" "(" expression ")"
                 | "." "name" "(" expression ")"
region_alias   ::= "as" identifier
```

**Static Semantics**

**(T-Region-Seq)**
$$\frac{\Gamma, r : \texttt{Region@Active} \vdash \textit{body} : T}{\Gamma \vdash \texttt{region as } r \{ \textit{body} \} : T}$$

**Dynamic Semantics**

Entering `region as r { body }` allocates a fresh region arena `r` and marks it Active for the dynamic extent of `body`.


#### 16.7.2 Region Options (`stack_size`, `name`)

**Region Options.** Configuration parameters for region behavior.

**Static Semantics**

| Option       | Type     | Effect                      |
| :----------- | :------- | :-------------------------- |
| `stack_size` | `usize`  | Initial allocation capacity |
| `name`       | `string` | Debug identifier for region |


#### 16.7.3 Region Allocation Operator (`^`)

**Region Allocation Operator.** The operator (`^`) that allocates storage in a named region.

**Syntax**

```ebnf
alloc_expr ::= identifier "^" expression
             | "^" expression
```

**Static Semantics**

**(T-Alloc-Explicit)**
$$\frac{\Gamma(r) = \texttt{Region@Active} \quad \Gamma \vdash e : T}{\Gamma \vdash r\texttt{\^{}}e : T_{\pi_r}}$$

**(T-Alloc-Implicit)**
$$\frac{R \text{ is the innermost active region} \quad \Gamma(R) = \texttt{Region@Active} \quad \Gamma \vdash e : T}{\Gamma \vdash \texttt{\^{}}e : T_{\pi_R}}$$

Here `T_{\pi_r}` denotes a value of base type `T` carrying provenance $\pi_{\text{Region}}(r)$. Bindings initialized from `r^expr` inherit this provenance via (P‑Local‑Inherit) (§3.2.4), ensuring region‑allocated values cannot escape their region without explicit heap escape (§3.2.7).

**Dynamic Semantics**

1. `r^expr` allocates storage from the explicitly named region `r`.
2. Each allocation returns an initialized object whose lifetime is bounded by the region's lifetime.


#### 16.7.4 Region Deallocation Order

**Dynamic Semantics**

1. On normal exit from the region block, cleanup proceeds in this order:
   1. Execute all `defer` blocks in the region scope per LIFO (§16.6.2).
   2. Destroy all responsible bindings in the region scope per §3.5.2. This includes bindings whose values carry $\pi_{\text{Region}}(R)$ provenance.
   3. Release the region’s backing storage in O(1) bulk deallocation.
2. Bulk deallocation of a region releases raw storage only; it does **not** invoke `Drop::drop` for allocations. Deterministic destruction remains the responsibility of bindings (§3.5).
3. On unwinding (panic), the same sequence occurs before unwinding continues to the enclosing scope.


#### 16.7.5 Nested Region Discipline

**Nested Regions.** Nested regions follow structured stack discipline.

**Static Semantics**

Nested regions are destroyed before their parent region resumes destruction.

**Constraints**

**Diagnostics:** See Appendix A, code `E-MEM-1206`.


#### 16.7.6 Region Arena Modal Type (`Region`)

**Region Type.** `Region` is a built-in modal type representing a concrete allocation arena. Region blocks are syntactic sugar for constructing a fresh `Region@Active` arena whose lifetime is the dynamic extent of the `region` block.

**Modal States**

$$\text{States}(\texttt{Region}) = \{\ \texttt{@Active},\ \texttt{@Frozen},\ \texttt{@Freed}\ \}$$

| State     | Description                                                 |
| :-------- | :---------------------------------------------------------- |
| `@Active` | Arena is accepting allocations                              |
| `@Frozen` | Arena is read-only; existing allocations remain valid       |
| `@Freed`  | Terminal state after bulk deallocation; no operations valid |

**Syntax**

```cursive
modal Region { @Active, @Frozen, @Freed }
```

**Core Procedures**

```cursive
procedure Region::new_scoped(~!, options: RegionOptions = {}) -> Region@Active
procedure alloc<T>(self: unique Region@Active, value: T) -> T_{π_r}
procedure reset_unchecked(self: unique Region@Active) -> Region@Active
procedure freeze(self: unique Region@Active) -> Region@Frozen
procedure thaw(self: unique Region@Frozen) -> Region@Active
procedure free_unchecked(self: unique Region@Active | Region@Frozen) -> Region@Freed
```

`reset_unchecked` bulk-frees all allocations in the arena while keeping the arena reusable. `freeze`/`thaw` allow staging phases where allocations are disallowed but data remains accessible. `free_unchecked` performs terminal bulk deallocation. Both `reset_unchecked` and `free_unchecked` are **unsafe-only** operations (§3.7); safe bulk reuse is provided by `frame` blocks (§16.7.7).

**Static Semantics**

1. `Region` values MUST NOT implement `Bitcopy`.
2. `alloc` yields a value with provenance $\pi_{\text{Region}}(r)$ where `r` is the arena receiver. The arena’s provenance tag is determined by its binding identifier in the enclosing `region` block.
3. `free_unchecked` MUST be invoked exactly once on any `Region` that remains in `@Active` or `@Frozen` at scope exit. Implementations MAY invoke `free_unchecked` implicitly during region block cleanup (§16.7.4).
4. After `reset_unchecked` or `free_unchecked`, all values and pointers with provenance $\pi_{\text{Region}}(r)$ derived from that arena become invalid. Accessing such values constitutes Unverifiable Behavior (UVB). Programs SHOULD explicitly drop any live region‑provenanced bindings before invoking these operations.

*[Informative Desugaring]*

`region [options] as r { body }` is semantically equivalent to:

1. `let r = Region::new_scoped(options)`
2. `defer unsafe { r~>free_unchecked() }`
3. Execute `body` with `r : Region@Active` in scope

If the `as r` clause is omitted, the implementation introduces an implicit arena binding used by unqualified `^` allocations in the block.


#### 16.7.7 Region Frames (`frame`)

**Region Frame.** A lexical sub-scope inside an active region arena that provides safe, O(1) bulk reclaim and reuse of the same arena backing store. Frames are the primary safe mechanism for mid-scope arena reuse; manual bulk reclaim is `unsafe` (§16.7.6).

**Syntax**

```ebnf
frame_stmt ::= "frame" block
            | identifier "." "frame" block
```

`frame { body }` targets the innermost active region arena. `r.frame { body }` targets the explicitly named arena `r`.

**Static Semantics**

Let `r` be the target arena and let `F` be a fresh synthetic region identifier scoped to `body`.

1. The target arena MUST be in state `Region@Active`.
   - For `frame { body }`, the innermost active region binding is selected as `r`.
   - For `r.frame { body }`, the identifier `r` MUST resolve to a `Region@Active` binding.
   Violations are ill-formed:
   - `E-MEM-1207`: no active region for implicit `frame`.
   - `E-MEM-1208`: explicit `r.frame` where `r` is not `Region@Active`.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-MEM-1207`, `E-MEM-1208`.

2. Entering a frame extends the typing environment with a fresh **synthetic active region binding** `F : Region@Active` for the duration of `body`. `F` is not a runtime value and cannot be named by user code; it exists only for provenance assignment. The frame introduces a corresponding provenance tag $\pi_{\text{Region}}(F)$ such that
   $$\pi_{\text{Region}}(F) < \pi_{\text{Region}}(r).$$
   Nested frames introduce strictly nested synthetic tags in lexical order.

3. Allocation inside the frame uses arena `r` for storage but assigns provenance $\pi_{\text{Region}}(F)$:

   **(T-Frame-Alloc-Implicit)**
   $$\frac{F \text{ is the innermost active frame targeting } r \quad \Gamma \vdash e : T}{\Gamma \vdash \texttt{\^{}}e : T_{\pi_F}}$$

   **(T-Frame-Alloc-Explicit)**
   $$\frac{F \text{ is an active frame targeting } r \quad \Gamma(r) = \texttt{Region@Active} \quad \Gamma \vdash e : T}{\Gamma \vdash r\texttt{\^{}}e : T_{\pi_F}}$$

4. By the Escape Rule (§3.2.5), any value or pointer with provenance $\pi_{\text{Region}}(F)$ cannot be stored into a location with provenance $\ge \pi_{\text{Region}}(r)$, and therefore cannot outlive the frame in safe code. Escaping a frame allocation requires explicit heap escape (§3.2.7).

**Dynamic Semantics**

Execution of a frame targeting arena `r` proceeds as:

1. Save an arena mark `m` representing the current allocation cursor of `r`.
2. Execute `body` as a normal lexical scope.
3. On exit (normal or panic):
   1. Execute `defer` blocks in the frame scope per LIFO.
   2. Destroy responsible bindings in the frame scope per §3.5.2.
   3. Restore the arena cursor of `r` to `m`, bulk-reclaiming all storage allocated in the frame.


### 16.8 Panic & Unwinding


#### 16.8.1 Panic Definition

**Panic.** An abnormal termination of the current thread's execution that triggers stack unwinding.

**Dynamic Semantics**

A panic may be triggered by:
- Explicit `panic()` invocation
- Failed runtime bounds checks
- Integer overflow in checked mode
- Division by zero
- Failed contract checks in `[[dynamic]]` scope


#### 16.8.2 Unwinding Process

*[REF: Unwinding semantics defined in §3.5.4. Double panic handling defined in §3.5.5.]*

**Unwinding.** The process of executing cleanup actions for all stack frames between the panic site and the panic handler. See §3.5.4 for destructor invocation order and §3.5.5 for double panic behavior.


#### 16.8.3 Panic in Parallel Blocks

**Parallel Panic.** Panic behavior in parallel contexts follows structured concurrency semantics.

See §18.7 for panic handling in parallel blocks.


#### 16.8.4 Catching Panics

**Catch Panic.** The `catch_panic` procedure captures a panic within a closure and returns a union type indicating success or failure.

**Syntax**

```cursive
catch_panic(|| expr)
```

**Dynamic Semantics**

1. The closure is executed.
2. If the closure completes normally, the result is the closure's return value.
3. If the closure panics, the panic is captured and a `PanicInfo` value is returned.

**Constraints**

The structure of `PanicInfo` is Implementation-Defined (IDB).

---

## 17. The Key System

---

### 17.1 Fundamentals and Modes


#### 17.1.1 Key Definition

**Key.** A static proof of access rights to a specific path within `shared` data.

**Static Semantics**

Keys are a compile-time verification mechanism. Key state is tracked during type checking. Runtime synchronization is introduced only when static analysis cannot prove safety (see §17.6).

**Key Invariants**

1. **Path-specificity:** A key to path $P$ grants access only to $P$ and paths for which $P$ is a prefix.
2. **Implicit acquisition:** Accessing a `shared` path logically acquires the necessary key.
3. **Scoped lifetime:** Keys are valid for a bounded lexical scope and become invalid when that scope exits.
4. **Reentrancy:** If a key covering path $P$ is already held, nested access to $P$ or any path prefixed by $P$ succeeds without conflict.
5. **Task locality:** Keys are associated with tasks. A key held by a task remains valid until its scope exits.


#### 17.1.2 Key Triple (Path, Mode, Scope)

**Key Triple.** A key consists of: **Path** (the memory location being accessed), **Mode** (`Read` or `Write`), and **Scope** (the lexical scope during which the key is valid).

**Syntax**

**Path Expression Grammar**

```ebnf
key_path_expr   ::= root_segment ("." path_segment)*
root_segment    ::= key_marker? identifier index_suffix?
path_segment    ::= key_marker? identifier index_suffix?
key_marker      ::= "#"
index_suffix    ::= "[" expression "]"
```

**Static Semantics**

**Path Well-Formedness**

A path expression is well-formed if each segment is valid for the type of the previous segment (field access on records, indexing on arrays/slices). At most one `#` marker may appear in the path.

**Key Analysis Requirement**

Key analysis is performed if and only if the path's root has `shared` permission. Paths with `const` or `unique` permission do not require keys.



#### 17.1.3 Read Mode

**Read Mode.** A key mode that permits read-only access to a path. Multiple Read keys to overlapping paths MAY coexist.

**Static Semantics**

**(K-Mode-Read)**
$$\frac{
    \Gamma \vdash e : \texttt{shared}\ T \quad
    \text{ReadContext}(e)
}{
    \text{RequiredMode}(e) = \text{Read}
}$$

**Read Context Classification**

| Syntactic Position                           | Context |
| :------------------------------------------- | :------ |
| Right-hand side of `let`/`var` initializer   | Read    |
| Right-hand side of assignment (`=`)          | Read    |
| Operand of arithmetic/logical operator       | Read    |
| Argument to `const` or `shared` parameter    | Read    |
| Condition expression (`if`, `loop`, `match`) | Read    |
| Receiver of method with `~` receiver         | Read    |


#### 17.1.4 Write Mode

**Write Mode.** A key mode that permits read and write access to a path. A Write key excludes all other keys to overlapping paths.

**Static Semantics**

**(K-Mode-Write)**
$$\frac{
    \Gamma \vdash e : \texttt{shared}\ T \quad
    \text{WriteContext}(e)
}{
    \text{RequiredMode}(e) = \text{Write}
}$$

**Write Context Classification**

| Syntactic Position                                 | Context |
| :------------------------------------------------- | :------ |
| Left-hand side of assignment (`=`)                 | Write   |
| Left-hand side of compound assignment (`+=`, etc.) | Write   |
| Receiver of method with `~!` receiver              | Write   |
| Receiver of method with `~%` receiver              | Write   |
| Argument to `unique` parameter                     | Write   |

**Disambiguation Rule**

If an expression appears in multiple contexts, the more restrictive context (Write) applies.

**Mode Ordering**

$$\text{Read} < \text{Write}$$

A held mode is sufficient for a required mode when:

$$\text{ModeSufficient}(M_{\text{held}}, M_{\text{required}}) \iff M_{\text{required}} \leq M_{\text{held}}$$

**Constraints**

**Diagnostics:** See Appendix A, code `E-CON-0005`.


#### 17.1.5 Key State Context

**Key State Context.** A mapping $\Gamma_{\text{keys}} : \text{ProgramPoint} \to \mathcal{P}(\text{Key})$ that associates each program point with the set of keys logically held at that point.

**Static Semantics**

**Key Set Operations**

Let $\Gamma_{\text{keys}}$ be the current key set, a collection of $(P, M, S)$ triples.

**(Acquire)**
$$\text{Acquire}(P, M, S, \Gamma_{\text{keys}}) = \Gamma_{\text{keys}} \cup \{(P, M, S)\}$$

**(Release)**
$$\text{Release}(P, \Gamma_{\text{keys}}) = \Gamma_{\text{keys}} \setminus \{(P, M, S) : (P, M, S) \in \Gamma_{\text{keys}}\}$$

**(Release by Scope)**
$$\text{ReleaseScope}(S, \Gamma_{\text{keys}}) = \Gamma_{\text{keys}} \setminus \{(P, M, S') : S' = S\}$$

**(Mode Transition)**
$$\text{ModeTransition}(P, M_{\text{new}}, \Gamma_{\text{keys}}) = (\Gamma_{\text{keys}} \setminus \{(P, M_{\text{old}}, S)\}) \cup \{(P, M_{\text{new}}, S)\}$$

**Panic Release Semantics**

$$\text{PanicRelease}(S, \Gamma_{\text{keys}}) = \Gamma_{\text{keys}} \setminus \{(P, M, S') : S' \leq_{\text{nest}} S\}$$

All keys held by the panicking scope and its nested scopes are released atomically before panic unwinding proceeds.


#### 17.1.6 Held Predicate

**Held Predicate.** A key is held at a program point if it is a member of the key state context at that point: $\text{Held}(P, M, S, \Gamma_{\text{keys}}, p) \iff (P, M, S) \in \Gamma_{\text{keys}}(p)$.

**Static Semantics**

**Key Compatibility**

Two keys $K_1 = (P_1, M_1, S_1)$ and $K_2 = (P_2, M_2, S_2)$ are **compatible** if and only if:

$$\text{Compatible}(K_1, K_2) \iff \text{Disjoint}(P_1, P_2) \lor (M_1 = \text{Read} \land M_2 = \text{Read})$$

**Compatibility Matrix**

| Key A Mode | Key B Mode | Paths Overlap? | Compatible? |
| :--------- | :--------- | :------------- | :---------- |
| Read       | Read       | Yes            | Yes         |
| Read       | Write      | Yes            | No          |
| Write      | Read       | Yes            | No          |
| Write      | Write      | Yes            | No          |
| Any        | Any        | No (disjoint)  | Yes         |

**Dynamic Semantics**

**Progress Guarantee**

Implementations MUST guarantee eventual progress: any task blocked waiting for a key MUST eventually acquire that key, provided the holder eventually releases it.

$$\text{Blocked}(t, K) \land \text{Held}(K, t') \land \Diamond\text{Released}(K, t') \implies \Diamond\text{Acquired}(K, t)$$


#### 17.1.7 Key Roots and Key Path Formation

**Key Root.** The base storage location from which a key path is derived. Key roots arise from: (1) **Lexical roots**: identifiers bound in the current scope; (2) **Boundary roots**: runtime object identities created at key boundaries (pointer indirection and type-level boundaries). For any place expression $e$ with `shared` permission, the **key path** $\text{KeyPath}(e)$ is the normalized `key_path_expr` used for key acquisition and conflict analysis.

**Static Semantics**

**Path Root Extraction**

Define $\text{Root}(e)$ for place expressions (§3.4.2) recursively:

$$\text{Root}(e) ::= \begin{cases}
x & \text{if } e = x \\
\text{Root}(e') & \text{if } e = e'.f \\
\text{Root}(e') & \text{if } e = e'[i] \\
\text{Root}(e') & \text{if } e = e' \mathord{\sim>} m(\ldots) \\
\bot_{\text{boundary}} & \text{if } e = (*e')
\end{cases}$$

where $\bot_{\text{boundary}}$ indicates a **key boundary** introduced by pointer dereference.

**Object Identity**

The **identity** of a reference or pointer $r$, written $\text{id}(r)$, is a unique runtime value denoting the storage location referred to by $r$.

Implementations MUST ensure:
1. **Uniqueness:** $\text{id}(r_1) = \text{id}(r_2)$ iff $r_1$ and $r_2$ refer to overlapping storage.
2. **Stability:** $\text{id}(r)$ remains constant for the lifetime of the referent.
3. **Opacity:** identities are not directly observable except through key semantics.

**KeyPath Formation**

Let $e$ be a place expression accessing `shared` data and let $e$’s field/index tail be $p_2.\ldots.p_n$.

- **Lexical root case:** If $\text{Root}(e) = x$, then

  $$\text{KeyPath}(e) = x.p_2.\ldots.p_n\ \text{truncated by any type-level boundary}.$$

- **Boundary root case:** If $\text{Root}(e) = \bot_{\text{boundary}}$ and $e = (*e').p_2.\ldots.p_n$, then

  $$\text{KeyPath}(e) = \text{id}(*e').p_2.\ldots.p_n\ \text{truncated by any type-level boundary}.$$

Type-level boundaries are defined in §17.2.4.

**Pointer Indirection**

A pointer dereference establishes a new key boundary. Key paths do not extend across pointer indirections.

For `(*e').p` where `e' : shared Ptr<T>@Valid`:
1. Dereferencing `e'` requires a key to $\text{KeyPath}(e')$ in Read mode.
2. Accessing `.p` on the dereferenced value uses a fresh key rooted at $\text{id}(*e')$.

**Constraints**

**Diagnostics:** See Appendix A, code `E-CON-0034`.

### 17.2 Acquisition and `#` Blocks


#### 17.2.1 Implicit Key Acquisition

**Implicit Acquisition.** Keys are logically acquired on demand during expression evaluation and released at scope exit.

**Static Semantics**

**Key Lifecycle Phases**

1. **Acquisition Phase:** Keys are logically acquired as `shared` paths are evaluated, in evaluation order
2. **Execution Phase:** The statement or block body executes with all keys logically held
3. **Release Phase:** All keys become invalid when the scope exits

**Covered Predicate**

An access to path $Q$ requiring mode $M_Q$ is covered by the current key state if:

$$\text{Covered}(Q, M_Q, \Gamma_{\text{keys}}) \iff \exists (P, M_P, S) \in \Gamma_{\text{keys}} : \text{Prefix}(P, Q) \land \text{ModeSufficient}(M_P, M_Q)$$

**Acquisition Rules**

**(K-Acquire-New)**
$$\frac{
    \Gamma \vdash P : \texttt{shared}\ T \quad
    M = \text{RequiredMode}(P) \quad
    \neg\text{Covered}(P, M, \Gamma_{\text{keys}}) \quad
    S = \text{CurrentScope}
}{
    \Gamma'_{\text{keys}} = \Gamma_{\text{keys}} \cup \{(P, M, S)\}
}$$

**(K-Acquire-Covered)**
$$\frac{
    \Gamma \vdash P : \texttt{shared}\ T \quad
    M = \text{RequiredMode}(P) \quad
    \text{Covered}(P, M, \Gamma_{\text{keys}})
}{
    \Gamma'_{\text{keys}} = \Gamma_{\text{keys}}
}$$

**Evaluation Order**

Subexpressions are evaluated left-to-right, depth-first. Key acquisition follows evaluation order.

**(K-Eval-Order)**
$$\text{eval}(e_1 \oplus e_2) \longrightarrow \text{eval}(e_1);\ \text{eval}(e_2);\ \text{apply}(\oplus)$$

**Constraints**

**Diagnostics:** See Appendix A, codes `W-CON-0001`, `W-CON-0002`.


#### 17.2.2 Key Release

**Key Release.** Keys are released when their defining scope exits.

**Static Semantics**

**(K-Release-Scope)**
$$\frac{
    \text{ScopeExit}(S)
}{
    \Gamma'_{\text{keys}} = \Gamma_{\text{keys}} \setminus \{(P, M, S') : S' = S\}
}$$

**(K-Release-Order)**

Keys acquired within a scope are released per LIFO semantics (§3.5.2).

**Dynamic Semantics**

**Release Guarantee**

Keys are released when their scope exits, regardless of exit mechanism:

| Exit Mechanism    | Keys Released? |
| :---------------- | :------------- |
| Normal completion | Yes            |
| `return`          | Yes            |
| `break`           | Yes            |
| `continue`        | Yes            |
| Panic propagation | Yes            |
| Task cancellation | Yes            |

**Key and RAII Cleanup Interaction**

Keys are released at scope exit before any `Drop::drop` calls for bindings in that scope (see §3.5).

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

**Constraints**

**Diagnostics:** See Appendix A, code `E-CON-0006`.


#### 17.2.3 `#` Block Syntax

**Key Block.** The `#` key block is the explicit key acquisition construct. It acquires a key to one or more paths at block entry and holds those keys until block exit.

**Syntax**

```ebnf
key_block       ::= "#" path_list mode_modifier* block
path_list       ::= key_path_expr ("," key_path_expr)*
mode_modifier   ::= "write" | "read" | release_modifier | "ordered" | "speculative"
release_modifier ::= "release" ("write" | "read")
```

**Mode Semantics**

| Block Form        | Key Mode | Mutation Permitted |
| :---------------- | :------- | :----------------- |
| `#path { }`       | Read     | No                 |
| `#path write { }` | Write    | Yes                |

**Static Semantics**

**(K-Block-Acquire)**
$$\frac{
    \Gamma \vdash P_1, \ldots, P_m : \texttt{shared}\ T_i \quad
    M = \text{BlockMode}(B) \quad
    (Q_1, \ldots, Q_m) = \text{CanonicalSort}(P_1, \ldots, P_m) \quad
    S = \text{NewScope}
}{
    \Gamma_{\text{keys}}' = \Gamma_{\text{keys}} \cup \{(Q_i, M, S) : i \in 1..m\}
}$$

**(K-Read-Block-No-Write)**
$$\frac{
    \text{BlockMode}(B) = \text{Read} \quad
    P \in \text{KeyedPaths}(B) \quad
    \text{WriteOf}(P) \in \text{Body}(B)
}{
    \text{Emit}(\texttt{E-CON-0070})
}$$

**Dynamic Semantics**

**Execution Order**

1. Acquire keys (mode per block specification) to all listed paths in canonical order
2. Execute the block body
3. Release all keys in reverse acquisition order

**Concurrent Access**

| Block A        | Block B        | Same/Overlapping Path | Result                          |
| :------------- | :------------- | :-------------------- | :------------------------------ |
| `#p { }`       | `#p { }`       | Yes                   | Both proceed concurrently       |
| `#p { }`       | `#p write { }` | Yes                   | One blocks until other releases |
| `#p write { }` | `#p write { }` | Yes                   | One blocks until other releases |

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CON-0001`–`E-CON-0004`, `E-CON-0030`–`E-CON-0032`, `E-CON-0070`, `W-CON-0003`.


#### 17.2.4 Key Coarsening

**Key Coarsening.** The `#` marker in a path expression sets the key granularity. The key is acquired at the marked position, covering all subsequent segments.

**Syntax**

**Inline Coarsening**

```ebnf
coarsened_path  ::= path_segment* "#" path_segment+
```

**Static Semantics**

**(K-Coarsen-Inline)**
$$\frac{
    P = p_1.\ldots.p_{k-1}.\#p_k.\ldots.p_n \quad
    \Gamma \vdash P : \texttt{shared}\ T \quad
    M = \text{RequiredMode}(P)
}{
    \text{AcquireKey}(p_1.\ldots.p_k, M, \Gamma_{\text{keys}})
}$$

**Type-Level Key Boundary**

A field declared with `#` establishes a permanent key boundary:

```ebnf
key_field_decl      ::= "#"? visibility? identifier ":" type
```

**(K-Type-Boundary)**
$$\frac{
    \Gamma \vdash r : R \quad
    R.\text{fields} \ni (\#f : U) \quad
    P = r.f.q_1.\ldots.q_n
}{
    \text{KeyPath}(P) = r.f
}$$

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CON-0033`, `W-CON-0013`.


#### 17.2.5 Closure Capture of `shared` Bindings

**Shared Closure Capture.** Closures that capture `shared` bindings participate in key analysis. Because a closure may be invoked outside its defining scope, key path roots depend on whether the closure escapes.

**Classification**

A closure expression $C$ is **local** if it appears only in:
1. Argument position of an immediate procedure or method call
2. The body of a `spawn` expression
3. The body of a `dispatch` expression
4. Operand of immediate invocation

A closure is **escaping** if it is:
1. Bound to a `let` or `var` binding
2. Returned from a procedure
3. Stored into a record or enum field

Let $\text{SharedCaptures}(C)$ be the set of captured bindings with `shared` permission.

**Static Semantics**

**Local Closure Key Paths**

For a local closure, key analysis treats the closure body as if it executed in the defining scope. Any access `x.p` where `x ∈ SharedCaptures(C)` uses lexical rooting (§17.1.7):

$$\text{KeyPath}(C, x.p) = \text{KeyPath}(x.p)$$

Keys are acquired at invocation, not at definition.

**Escaping Closure Type Requirement**

An escaping closure that captures `shared` bindings MUST carry a shared dependency set in its closure type (§12.4.3):

**(K-Closure-Escape-Type)**
$$\frac{
    C\ \text{is escaping} \quad
    \text{SharedCaptures}(C) = \{x_1, \ldots, x_n\}
}{
    \text{Type}(C) = |\vec{T}| \to R\ [\texttt{shared}: \{x_1 : \texttt{shared}\ T_1, \ldots, x_n : \texttt{shared}\ T_n\}]
}$$

The dependency set MAY be inferred when the closure is assigned to a binding with an expected closure type; otherwise it MUST be written explicitly.

**Escaping Closure Key Paths**

For an escaping closure, key paths are rooted at runtime identities of captured references:

**(K-Closure-Escape-Keys)**
$$\frac{
    C : |\vec{T}| \to R\ [\texttt{shared}: \{x : \texttt{shared}\ T\}] \quad
    \text{Access}(x.p, M) \in C.\text{body}
}{
    \text{KeyPath}(C, x.p) = \text{id}(C.x).p \quad \text{KeyMode} = M
}$$

where $C.x$ denotes the captured reference to $x$ stored in the closure environment.

**Lifetime Constraint**

An escaping closure MUST NOT outlive any captured `shared` binding. Implementations MUST reject any flow where the closure’s lifetime exceeds the lifetime of a captured `shared` root.

**Dynamic Semantics**

**Local Closure Invocation**

1. Determine accessed `shared` paths in the closure body.
2. Acquire required keys using lexical roots.
3. Execute the closure body.
4. Release keys at the end of the invocation.

**Escaping Closure Invocation**

1. For each dependency $(x : shared\ T)$ in the closure type, let $r = C.x$.
2. Acquire required keys for paths rooted at $\text{id}(r)$.
3. Execute the closure body.
4. Release keys at the end of the invocation.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CON-0085`, `E-CON-0086`, `W-CON-0009`.


#### 17.2.6 `shared` Dynamic Class Objects (`shared $Class`)

**Shared Dynamic Class.** Dynamic class objects (`$Cl`, §13.5) erase the concrete type at runtime. When such an object is viewed with `shared` permission, key analysis cannot inspect the concrete method bodies chosen by vtable dispatch. Therefore, `shared $Cl` is restricted to read-only dynamic interfaces.

**Static Semantics**

**Well-Formedness**

A dynamic class object type may be qualified with `shared` permission only if every vtable-eligible procedure in the class (including inherited ones) has a `const` receiver (`~`):

Let $\text{DynMethods}(Cl)$ denote the set of procedures callable via vtable dispatch on `$Cl` (i.e., vtable‑eligible and not marked `[[static_dispatch_only]]`).

**(K-Witness-Shared-WF)**
$$\frac{
    \forall m \in \text{DynMethods}(Cl).\ m.\text{receiver} = \texttt{\sim}
}{
    \Gamma \vdash \texttt{shared}\ \$Cl\ \text{wf}
}$$

If any method requires `shared` (`~%`) or `unique` (`~!`) receiver permission, `shared $Cl` is ill‑formed. Such methods may still exist on `Cl` if they are excluded from dynamic dispatch via `[[static_dispatch_only]]`.

**Dynamic Calls Through `shared $Cl`**

For a call `e~>m(args)` where `e : shared $Cl`, the key mode is Read and the key path is the root of `e`:

$$\text{KeyPath}(e~>m(\ldots)) = \text{Root}(e) \quad \text{KeyMode} = \text{Read}$$

**Constraints**

**Diagnostics:** See Appendix A, code `E-CON-0083`.


### 17.3 Conflict Detection


#### 17.3.1 Path Prefix and Disjointness

**Path Relations.** The prefix relation and disjointness relation on paths determine key coverage and static safety of concurrent accesses.

**Static Semantics**

**Path Prefix**

Path $P$ is a prefix of path $Q$ (written $\text{Prefix}(P, Q)$) if $P$ is an initial subsequence of $Q$:

$$\text{Prefix}(p_1.\ldots.p_m,\ q_1.\ldots.q_n) \iff m \leq n \land \forall i \in 1..m,\ p_i \equiv_{\text{seg}} q_i$$

**Path Disjointness**

Two paths $P$ and $Q$ are disjoint (written $\text{Disjoint}(P, Q)$) if neither is a prefix of the other:

$$\text{Disjoint}(P, Q) \iff \neg\text{Prefix}(P, Q) \land \neg\text{Prefix}(Q, P)$$

**Segment Equivalence**

$$p_i \equiv_{\text{seg}} q_i \iff \text{name}(p_i) = \text{name}(q_i) \land \text{IndexEquiv}(p_i, q_i)$$

**Index Expression Equivalence**

Two index expressions $e_1$ and $e_2$ are provably equivalent ($e_1 \equiv_{\text{idx}} e_2$) if:

1. Both are the same integer literal
2. Both are references to the same `const` binding
3. Both are references to the same generic const parameter
4. Both are references to the same variable binding in scope
5. Both normalize to the same canonical form under constant folding

**Disjointness for Static Safety**

**(K-Disjoint-Safe)**
$$\frac{
    \text{Disjoint}(P, Q)
}{
    \text{ConcurrentAccess}(P, Q) \text{ is statically safe}
}$$

**Prefix for Coverage**

**(K-Prefix-Coverage)**
$$\frac{
    \text{Prefix}(P, Q) \quad \text{Held}(P, M, \Gamma_{\text{keys}})
}{
    \text{Covers}((P, M), Q)
}$$


#### 17.3.2 Static Conflict Analysis

**Static Conflict Analysis.** When multiple dynamic indices access the same array within a single statement, they MUST be provably disjoint; otherwise, the program is ill-formed.

**Static Semantics**

**Provable Disjointness for Indices**

Two index expressions are provably disjoint ($\text{ProvablyDisjoint}(e_1, e_2)$) if:

1. **Static Disjointness:** Both are statically resolvable with different values
2. **Control Flow Facts:** A Verification Fact establishes $e_1 \neq e_2$
3. **Contract-Based:** A precondition asserts $e_1 \neq e_2$
4. **Refinement Types:** An index has a refinement type constraining its relationship
5. **Algebraic Offset:** Expressions share a common base but differ by constant offsets
6. **Dispatch Iteration:** Within a `dispatch` block, accesses indexed by the iteration variable are automatically disjoint
7. **Disjoint Loop Ranges:** Iteration variables from loops with non-overlapping ranges

**(K-Dynamic-Index-Conflict)**
$$\frac{
    P_1 = a[e_1] \quad P_2 = a[e_2] \quad
    \text{SameStatement}(P_1, P_2) \quad
    (\text{Dynamic}(e_1) \lor \text{Dynamic}(e_2)) \quad
    \neg\text{ProvablyDisjoint}(e_1, e_2)
}{
    \text{Emit}(\texttt{E-CON-0010})
}$$

**Constraints**

**Diagnostics:** See Appendix A, code `E-CON-0010`.


#### 17.3.3 Read-Then-Write Prohibition

**Read-Then-Write.** A pattern that occurs when a statement contains both a read and a write to the same `shared` path without a covering Write key.

**Static Semantics**

**Formal Definition**

$$\text{ReadThenWrite}(P, S) \iff \exists e_r, e_w \in \text{Subexpressions}(S) : \text{ReadsPath}(e_r, P) \land \text{WritesPath}(e_w, P)$$

**(K-Read-Write-Reject)**
$$\frac{
    \Gamma \vdash P : \texttt{shared}\ T \quad
    \text{ReadThenWrite}(P, S) \quad
    \neg\exists (Q, \text{Write}, S') \in \Gamma_{\text{keys}} : \text{Prefix}(Q, P)
}{
    \text{Emit}(\texttt{E-CON-0060})
}$$

**(K-RMW-Permitted)**
$$\frac{
    \Gamma \vdash P : \texttt{shared}\ T \quad
    \text{ReadThenWrite}(P, S) \quad
    \exists (Q, \text{Write}, S') \in \Gamma_{\text{keys}} : \text{Prefix}(Q, P)
}{
    \text{Permitted}
}$$

**Pattern Classification**

| Pattern                  | Covering Write Key?         | Action                |
| :----------------------- | :-------------------------- | :-------------------- |
| `p += e`                 | Yes (desugars to `#` block) | Permitted             |
| `#p write { p = p + e }` | Yes                         | Permitted             |
| `p = p + e`              | No                          | **Reject E-CON-0060** |
| `p.a = p.b + 1`          | No, but disjoint paths      | Permitted             |

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CON-0060`, `W-CON-0004`, `W-CON-0006`.


### 17.4 Nested Release


#### 17.4.1 Nested Key Semantics

**Nested Keys.** Nested `#` blocks acquire keys independently. The outer block's keys remain held while the inner block executes.

**Syntax**

The `release` modifier enables mode transitions in nested blocks:

| Outer Mode | `release` Target | Operation                                      |
| :--------- | :--------------- | :--------------------------------------------- |
| Read       | `write`          | Escalation: Release Read → Gap → Acquire Write |
| Write      | `read`           | Downgrade: Release Write → Gap → Acquire Read  |

**Static Semantics**

**(K-Nested-Same-Path)**
$$\frac{
    \text{Held}(P, M_{\text{outer}}, \Gamma_{\text{keys}}) \quad
    \#P\ M_{\text{inner}}\ \{\ \ldots\ \}
}{
    \begin{cases}
    \text{Covered (no acquisition)} & \text{if } M_{\text{inner}} = M_{\text{outer}} \\
    \text{Release-and-Reacquire per §17.4.1} & \text{if } M_{\text{inner}} \neq M_{\text{outer}} \land \texttt{release} \in \text{modifiers} \\
    \text{Emit}(\texttt{E-CON-0012}) & \text{if } M_{\text{inner}} \neq M_{\text{outer}} \land \texttt{release} \notin \text{modifiers}
    \end{cases}
}$$

**Dynamic Semantics**

**Release-and-Reacquire Sequence**

When entering `#path release <target> { body }`:

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

**Interleaving Windows**

Between steps 1 and 2 (entry), and between steps 4 and 5 (exit), other tasks MAY acquire keys to the same path.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CON-0011`, `E-CON-0012`, `E-CON-0017`, `E-CON-0018`, `W-CON-0010`–`W-CON-0012`.

**Staleness Suppression (`[[stale_ok]]`)**

`[[stale_ok]]` suppresses warning `W-CON-0011` on a binding derived from `shared` data across a `release` or `yield release` boundary. See §7.4.5 for the full definition. The attribute does not affect dynamic semantics.


#### 17.4.2 Key Reentrancy

**Key Reentrancy.** If a caller holds a key covering the callee's access paths, the callee's accesses are covered without additional acquisition.

**Static Semantics**

**(K-Reentrant)**
$$\frac{
    \text{Held}(P, M, \Gamma_{\text{keys}}) \quad
    \text{Prefix}(P, Q) \quad
    \text{CalleeAccesses}(Q)
}{
    \text{CalleeCovered}(Q)
}$$

**(K-Procedure-Boundary)**

Passing a `shared` value as a procedure argument does not constitute key acquisition:

$$\frac{
    \Gamma \vdash f : (\texttt{shared}\ T) \to R \quad
    \Gamma \vdash v : \texttt{shared}\ T
}{
    \text{call}(f, v) \longrightarrow \text{no key acquisition at call site}
}$$

**Constraints**

**Diagnostics:** See Appendix A, code `W-CON-0005`.


### 17.5 Speculative Execution


#### 17.5.1 Speculative Block Semantics

**Speculative Block.** A key block that executes without acquiring an exclusive key, relying on optimistic concurrency control. The block body executes against a snapshot; at block exit, writes are atomically committed if and only if the snapshot values remain unchanged.

**Syntax**

```ebnf
speculative_block ::= "#" path_list "speculative" "write" block
```

**Static Semantics**

**(K-Spec-Write-Required)**
$$\frac{
    \#P\ \texttt{speculative}\ M\ \{B\} \quad M \neq \texttt{write}
}{
    \text{Emit}(\texttt{E-CON-0095})
}$$

**(K-Spec-Pure-Body)**
$$\frac{
    \#P\ \texttt{speculative write}\ \{B\} \quad
    \text{Writes}(B) \not\subseteq \text{CoveredPaths}(P)
}{
    \text{Emit}(\texttt{E-CON-0091})
}$$

**Permitted Operations**

- Read from keyed paths
- Write to keyed paths
- Pure computation (arithmetic, logic, local bindings)
- Procedure calls to `const` receiver methods on keyed data

**Prohibited Operations**

- Write to paths outside the keyed set
- Nested key blocks
- `wait` expressions
- Procedure calls with side effects
- `defer` statements

**(K-Spec-No-Nested-Key)**
$$\frac{
    \#P\ \texttt{speculative write}\ \{B\} \quad
    \#Q\ \_\ \{\ldots\} \in \text{Subexpressions}(B)
}{
    \text{Emit}(\texttt{E-CON-0090})
}$$


#### 17.5.2 Snapshot-Execute-Commit Cycle

**Speculative Commit.** $\text{SpeculativeCommit}(R, W) \iff \text{Atomic}\left(\bigwedge_{(p,v) \in R} p = v \implies \bigwedge_{(q,w) \in W} q := w\right)$

**Dynamic Semantics**

**Execution Model**

1. **Initialize**: Set $\text{retries} := 0$
2. **Snapshot**: Read all paths in the keyed set, recording $(path, value)$ pairs in read set $R$
3. **Execute**: Evaluate $body$, collecting writes in write set $W$
4. **Commit**: Atomically verify $\bigwedge_{(p,v) \in R} (\text{current}(p) = v)$ and, if true, apply all writes in $W$
5. **On success**: Return the value of $body$
6. **On failure**: Increment $\text{retries}$; if below limit, goto step 2; otherwise fallback
7. **Fallback**: Acquire a blocking Write key, execute $body$, release key, return value

**Retry Limit**

$\text{MAX\_SPECULATIVE\_RETRIES}$ is IDB.

**Interaction with Blocking Keys**

| Concurrent Operation      | Speculative Block Behavior                  |
| :------------------------ | :------------------------------------------ |
| Another speculative block | Race; one commits, others retry             |
| Blocking Read key held    | Speculative may commit (compatible)         |
| Blocking Write key held   | Speculative commit fails, retry or fallback |

**Snapshot and Commit Requirements**

The snapshot step (2) MUST be observationally equivalent to an atomic snapshot over the keyed set. The commit step (4) MUST be atomic with respect to all other key operations on overlapping paths and MUST satisfy the `SpeculativeCommit` predicate above. The concrete mechanisms used to realize these properties are Implementation‑Defined (IDB).

*[Informative: implementations typically use atomic loads or seqlock‑style reads for snapshots and CAS or brief locks for commits. Hidden version counters may be used as an implementation technique.]* 


#### 17.5.3 Speculation and Panics

**Dynamic Semantics**

If a panic occurs during execution (step 3):

1. The write set $W$ is discarded (no writes are committed)
2. No key is held (nothing to release)
3. The panic propagates normally

**Memory Ordering**

- Snapshot reads: Acquire semantics
- Successful commit: Release semantics
- Failed commit: No ordering guarantees (writes discarded)

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CON-0090`–`E-CON-0096`, `W-CON-0020`, `W-CON-0021`.


### 17.6 Runtime Realization (`[[dynamic]]`)


#### 17.6.1 Dynamic Key Verification

**Dynamic Verification.** Keys are a compile-time abstraction. Runtime synchronization is emitted only when the `[[dynamic]]` attribute is present and static verification fails.

**Static Semantics**

**Static Safety Conditions**

| Condition              | Description                                                 | Rule   |
| :--------------------- | :---------------------------------------------------------- | :----- |
| **No escape**          | `shared` value never escapes to another task                | K-SS-1 |
| **Disjoint paths**     | Concurrent accesses target provably disjoint paths          | K-SS-2 |
| **Sequential context** | No `parallel` block encloses the access                     | K-SS-3 |
| **Unique origin**      | Value is `unique` at origin, temporarily viewed as `shared` | K-SS-4 |
| **Dispatch-indexed**   | Access indexed by `dispatch` iteration variable             | K-SS-5 |
| **Speculative-only**   | All accesses within speculative blocks with fallback        | K-SS-6 |

**(K-Static-Safe)**
$$\frac{
    \text{Access}(P, M) \quad
    \text{StaticallySafe}(P)
}{
    \text{NoRuntimeSync}(P)
}$$

**(K-Static-Required)**
$$\frac{
    \neg\text{StaticallySafe}(P) \quad
    \neg\text{InDynamicContext}
}{
    \text{Emit}(\texttt{E-CON-0020})
}$$

**(K-Dynamic-Permitted)**
$$\frac{
    \neg\text{StaticallySafe}(P) \quad
    \text{InDynamicContext}
}{
    \text{EmitRuntimeSync}(P)
}$$

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CON-0014`, `E-CON-0020`, `I-CON-0011`, `I-CON-0013`.


#### 17.6.2 Runtime Lock Acquisition

**Dynamic Semantics**

**Runtime Realization**

When runtime synchronization is required:

1. Mutual exclusion per key compatibility rules (§17.1.6)
2. Blocking when incompatible keys are held
3. Release on scope exit (including panic)
4. Progress guarantee (no indefinite starvation)

**Canonical Order**

A canonical order on paths defines deterministic acquisition order:

$$P <_{\text{canon}} Q \iff \exists k \geq 1 : (\forall i < k,\ p_i =_{\text{seg}} q_i) \land (p_k <_{\text{seg}} q_k \lor (k > m \land m < n))$$

**Segment Ordering**

1. **Identifiers:** Lexicographic by UTF-8 byte sequence
2. **Indexed segments:** By index value for statically resolvable indices
3. **Bare vs indexed:** Bare identifier precedes any indexed form

**Dynamic Ordering Guarantee**

Within `[[dynamic]]` scope, incomparable dynamic indices require runtime ordering satisfying:

1. **Totality**: For distinct paths $P$ and $Q$, exactly one of $\text{DynOrder}(P, Q)$ or $\text{DynOrder}(Q, P)$ holds
2. **Antisymmetry**: $\text{DynOrder}(P, Q) \land \text{DynOrder}(Q, P) \implies P = Q$
3. **Transitivity**: $\text{DynOrder}(P, Q) \land \text{DynOrder}(Q, R) \implies \text{DynOrder}(P, R)$
4. **Cross-Task Consistency**: All tasks compute the same ordering
5. **Value-Determinism**: Ordering depends only on runtime values, not timing

**Observational Equivalence**

$$\forall P : \text{Observable}(\text{StaticSafe}(P)) = \text{Observable}(\text{RuntimeSync}(P))$$

**Constraints**

**Diagnostics:** See Appendix A, code `W-CON-0021`.


### 17.7 Memory Ordering


#### 17.7.1 Default Sequential Consistency

**Sequential Consistency.** Key acquisition uses acquire semantics. Key release uses release semantics. Memory accesses default to sequentially consistent ordering.


#### 17.7.2 Relaxed Ordering Levels

**Syntax**

```cursive
[[relaxed]]
counter += 1

[[acquire]]
let ready = flag

[[release]]
flag = true
```

| Ordering  | Guarantee                             |
| :-------- | :------------------------------------ |
| `relaxed` | Atomicity only—no ordering            |
| `acquire` | Subsequent reads see prior writes     |
| `release` | Prior writes visible to acquire reads |
| `acq_rel` | Both acquire and release              |
| `seq_cst` | Total global order (default)          |


#### 17.7.3 Memory Ordering Attributes

**Syntax**

```ebnf
memory_order_attribute ::= "[[" memory_order "]]"
memory_order           ::= "relaxed" | "acquire" | "release" | "acq_rel" | "seq_cst"
```

**Static Semantics**

Memory ordering annotations are permitted inside standard `#` blocks. The annotation affects only the data access ordering; it does not modify the key's acquire/release semantics at block entry and exit.

Memory ordering annotations MUST NOT appear inside speculative blocks (§17.5).


#### 17.7.4 Fence Operations

**Syntax**

```cursive
fence(acquire)
fence(release)
fence(seq_cst)
```

---

# Part VI: Concurrency

## 18. Structured Parallelism

---

### 18.1 Parallel Blocks


#### 18.1.1 `parallel` Block Syntax

**Parallel Block.** A scope within which work executes concurrently across multiple workers. A parallel block $P$ is defined as $P = (D, A, B)$ where $D$ is the execution domain expression, $A$ is the set of block options, and $B$ is the block body.

**Syntax**

```ebnf
parallel_block    ::= "parallel" domain_expr block_options? block

domain_expr       ::= expression

block_options     ::= "[" block_option ("," block_option)* "]"

block_option      ::= "cancel" ":" expression
                    | "name" ":" string_literal

block             ::= "{" statement* "}"
```

| Option   | Type          | Effect                                       |
| :------- | :------------ | :------------------------------------------- |
| `cancel` | `CancelToken` | Provides cooperative cancellation capability |
| `name`   | `string`      | Labels the block for debugging and profiling |

**Static Semantics**

**Typing Rule**

$$\frac{
  \Gamma \vdash D : \text{\$ExecutionDomain} \quad
  \Gamma_P = \Gamma[\text{parallel\_context} \mapsto D] \quad
  \Gamma_P \vdash B : T
}{
  \Gamma \vdash \texttt{parallel } D\ \{B\} : T
} \quad \text{(T-Parallel)}$$

**Well-Formedness**

A parallel block is well-formed if and only if:

1. The domain expression evaluates to a type implementing `ExecutionDomain`.
2. All `spawn` and `dispatch` expressions within the block body reference the enclosing parallel block.
3. No `spawn` or `dispatch` expression appears outside a parallel block.
4. All captured bindings satisfy the permission requirements of §18.3.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CON-0101`–`E-CON-0103`.


#### 18.1.2 Fork-Join Semantics

**Structured Concurrency Invariant.** Let $\mathcal{W}$ denote the set of work items spawned within a parallel block $P$: $\forall w \in \mathcal{W}.\ \text{lifetime}(w) \subseteq \text{lifetime}(P)$. A **work item** is a unit of computation queued for execution by a worker (created by `spawn` and `dispatch` expressions). A **worker** is an execution context that executes work items. A **task** is the runtime representation of a work item during execution; tasks may suspend and resume, and are associated with key state per §17.1.

**Dynamic Semantics**

Evaluation of `parallel D [opts] { B }`:

1. Evaluate domain expression $D$ to obtain $d$.
2. Initialize the worker pool according to $d$'s configuration.
3. If the `cancel` option is present, associate its token with the block.
4. Evaluate statements in $B$ sequentially; `spawn` and `dispatch` expressions enqueue work items.
5. Block at the closing brace until all enqueued work items complete.
6. If any work item panicked, propagate the first panic (by completion order) after all work settles.
7. Release workers back to the domain.
8. Return the collected results per §18.1.3.

Work items complete in Unspecified order. The parallel block guarantees only that ALL work completes before the block exits.


#### 18.1.3 Parallel Block Completion

**Parallel Completion.** A parallel block is an expression that yields a value. The result type depends on the block's contents.

**Static Semantics**

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


### 18.2 Execution Domains


#### 18.2.1 CPU Domain

**CPU Domain.** An execution domain that executes work items on operating system threads managed by the runtime.

**Static Semantics**

The runtime provides a CPU execution domain via `ctx.cpu(...)`. This constructor, its parameters, and helper types (e.g., `CpuSet`, `Priority`) are core‑library/runtime interfaces and are not normative in this document. Any value produced by `ctx.cpu(...)` MUST implement `ExecutionDomain`.


#### 18.2.2 GPU Domain

**GPU Domain.** An execution domain that executes work items on graphics processing unit compute shaders.

**Static Semantics**

The runtime may provide a GPU execution domain via `ctx.gpu(...)`. Its constructor parameters, buffer types, and GPU intrinsics are core‑library/runtime interfaces and are not normative here. Within GPU-domain work items, capturing `shared` bindings, host pointers, or heap‑provenanced values is ill‑formed; only GPU‑accessible data may be captured.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CON-0150`–`E-CON-0152`.


#### 18.2.3 Inline Domain

**Inline Domain.** An execution domain that executes work items sequentially on the current thread.

**Syntax**

```cursive
ctx.inline()
```

**Dynamic Semantics**

- `spawn { e }` evaluates $e$ immediately and blocks until complete.
- `dispatch i in range { e }` executes as a sequential loop.
- No actual parallelism occurs.
- All capture rules and permission requirements remain enforced.


#### 18.2.4 Domain Selection

**Execution Domain.** A capability that provides access to computational resources. Domains implement the `ExecutionDomain` class.

**Static Semantics**

```cursive
class ExecutionDomain {
    procedure name(~) -> string
    procedure max_concurrency(~) -> usize
}
```

This class is dispatchable, enabling heterogeneous domain handling.


### 18.3 Capture Semantics

See §10.1 for permission definitions. This section defines parallel-specific capture constraints.


#### 18.3.1 Capture Rules for `const`

**Const Capture.** Bindings with `const` permission may be captured by reference into `spawn` and `dispatch` bodies. Captured `const` references are guaranteed valid because structured concurrency (§18.1.2) ensures all work completes before the parallel block exits.

**Static Semantics**

`const` capture does not transfer responsibility. Multiple work items may capture the same `const` binding concurrently.


#### 18.3.2 Capture Rules for `shared`

**Shared Capture.** Bindings with `shared` permission may be captured by reference into `spawn` and `dispatch` bodies. Accesses are synchronized via the key system (§17).

**Static Semantics**

`shared` capture does not transfer responsibility. Concurrent access follows key acquisition semantics per §17.2.


#### 18.3.3 Capture Rules for `unique` (Move Required)

See §3.4 for move semantics and §10.1.2 for `unique` permission rules.

**Unique Capture.** Bindings with `unique` permission MUST NOT be implicitly captured. To use a `unique` binding in a work item, explicit `move` is required. Only one work item may receive ownership.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CON-0120`–`E-CON-0122`.


### 18.4 Tasks and Spawning


#### 18.4.1 `spawn` Expression Syntax

**Spawn Expression.** An expression that creates a work item for concurrent execution within the enclosing parallel block.

**Syntax**

```ebnf
spawn_expr      ::= "spawn" spawn_option_list? block

spawn_option_list  ::= "[" spawn_option ("," spawn_option)* "]"

spawn_option       ::= "name" ":" string_literal
                    | "affinity" ":" expression
                    | "priority" ":" expression
```

| Attribute  | Type       | Default            | Effect                         |
| :--------- | :--------- | :----------------- | :----------------------------- |
| `name`     | `string`   | Anonymous          | Labels for debugging/profiling |
| `affinity` | `CpuSet`   | Domain default     | CPU core affinity hint         |
| `priority` | `Priority` | `Priority::Normal` | Scheduling priority hint       |

**Static Semantics**

**Typing Rule**

$$\frac{
  \Gamma[\text{parallel\_context}] = D \quad
  \Gamma_{\text{capture}} \vdash e : T
}{
  \Gamma \vdash \texttt{spawn}\ \{e\} : \text{SpawnHandle}\langle T \rangle
} \quad \text{(T-Spawn)}$$

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CON-0130`, `E-CON-0131`.


#### 18.4.2 Task Creation

**SpawnHandle Type.** A modal type representing a pending or completed spawn result:

```cursive
modal SpawnHandle<T> {
    @Pending { }
    @Ready { value: T }
}
```

**Dynamic Semantics**

Evaluation of `spawn [attrs] { e }`:

1. Capture free variables from enclosing scope per §18.3.
2. Package the captured environment with expression $e$ into a work item.
3. Enqueue the work item to the parallel block's worker pool.
4. Return `SpawnHandle<T>@Pending` immediately (non-blocking).
5. A worker eventually dequeues and evaluates $e$.
6. Upon completion, the handle transitions to `@Ready` with the result value.


#### 18.4.3 Task Result Collection

**Result Collection.** Spawn results are collected through three mechanisms:

| Mechanism           | Description                                                                 |
| :------------------ | :-------------------------------------------------------------------------- |
| Implicit collection | When a parallel block contains only spawn expressions, results form a tuple |
| Explicit wait       | Use `wait handle` to retrieve a specific result                             |
| Ignored             | Results not collected are discarded; work still completes                   |

**Dynamic Semantics**

`SpawnHandle<T>` supports the `wait` expression for result retrieval. Per §17, keys MUST NOT be held across `wait` suspension points.


### 18.5 Data Parallelism (`dispatch`)


#### 18.5.1 `dispatch` Expression Syntax

**Dispatch Expression.** An expression that expresses data-parallel iteration where each iteration may execute concurrently.

**Syntax**

```ebnf
dispatch_expr   ::= "dispatch" pattern "in" range_expression
                    key_clause?
                    dispatch_option_list?
                    block

key_clause      ::= "key" key_path_expr key_mode

key_mode        ::= "read" | "write"

dispatch_option_list  ::= "[" dispatch_option ("," dispatch_option)* "]"

dispatch_option       ::= "reduce" ":" reduce_op
                        | "ordered"
                        | "chunk" ":" expression

reduce_op       ::= "+" | "*" | "min" | "max" | "and" | "or" | identifier
```

| Attribute | Type         | Effect                                        |
| :-------- | :----------- | :-------------------------------------------- |
| `reduce`  | Reduction op | Combines iteration results using specified op |
| `ordered` | (flag)       | Forces sequential side-effect ordering        |
| `chunk`   | `usize`      | Groups iterations into chunks for granularity |

**Static Semantics**

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

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CON-0140`–`E-CON-0143`, `W-CON-0140`.


#### 18.5.2 Iteration Space

**Dynamic Semantics**

Evaluation of `dispatch i in range [attrs] { B }`:

1. Evaluate `range` to determine iteration count $n$.
2. Analyze key patterns to partition iterations into conflict-free groups.
3. For each group, enqueue iterations as work items to the worker pool.
4. Workers execute iterations, acquiring keys as needed per §17.2.
5. If `[reduce: op]` is present, combine partial results using `op`.
6. Block until all iterations complete.
7. Return the reduced value (if reduction) or unit.

**Reduction Semantics**

Reduction operators MUST be associative:

$$\forall a, b, c.\ (a \oplus b) \oplus c = a \oplus (b \oplus c)$$

For non-associative operations, the `[ordered]` attribute is required, which forces sequential execution.

Parallel reduction:

1. Partition iterations across workers.
2. Each worker reduces its partition to a partial result.
3. Combine partial results in a deterministic tree pattern.
4. Return the final result.


#### 18.5.3 Key-Based Parallelism Determination

See §17 for key system definition. This section defines dispatch-specific key usage.

**Static Semantics**

**Key Inference**

When no key clause is provided, the body is analyzed to infer key paths and modes:

| Body Access Pattern       | Inferred Key Path        | Inferred Mode   |
| :------------------------ | :----------------------- | :-------------- |
| `data[i] = ...`           | `data[i]`                | `write`         |
| `... = data[i]`           | `data[i]`                | `read`          |
| `data[i] = data[i] + ...` | `data[i]`                | `write`         |
| `result[i] = source[j]`   | `result[i]`, `source[j]` | `write`, `read` |

**Disjointness Guarantee**

$$\frac{
  \texttt{dispatch}\ v\ \texttt{in}\ r\ \{\ \ldots\ a[v]\ \ldots\ \}
}{
  \forall v_1, v_2 \in r,\ v_1 \neq v_2 \implies \text{ProvablyDisjoint}(a[v_1], a[v_2])
} \quad \text{(K-Disjoint-Dispatch)}$$

**Parallelism Determination**

| Key Pattern   | Keys Generated      | Parallelism Degree    |
| :------------ | :------------------ | :-------------------- |
| `data[i]`     | $n$ distinct keys   | Full parallel         |
| `data[i / 2]` | $n/2$ distinct keys | Pairs serialize       |
| `data[i % k]` | $k$ distinct keys   | $k$-way parallel      |
| `data[f(i)]`  | Unknown at compile  | Runtime serialization |


### 18.6 Cancellation


#### 18.6.1 Cancellation Propagation

**Cancellation.** The cooperative mechanism by which in-progress parallel work may be requested to stop early. Work items MUST explicitly check for cancellation; the runtime does not forcibly terminate work.

**Syntax**

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

**Dynamic Semantics**

When a cancel token is attached to a parallel block via the `cancel` option, the token is implicitly available within all `spawn` and `dispatch` bodies.


#### 18.6.2 Cancellation Cleanup

**Dynamic Semantics**

Cancellation is a request, not a guarantee:

| Scenario                       | Behavior                          |
| :----------------------------- | :-------------------------------- |
| Work checks and returns early  | Iteration completes immediately   |
| Work ignores cancellation      | Iteration runs to completion      |
| Work is queued but not started | MAY be dequeued without executing |
| Work is mid-execution          | Continues until next check point  |


### 18.7 Panic Handling in Parallel


#### 18.7.1 Single Panic Semantics

**Parallel Panic Handling.** When a work item panics within a parallel block, the panic is captured and propagated after all work settles.

**Dynamic Semantics**

1. The panicking work item captures panic information.
2. Other work items continue to completion (or cancellation if a token is attached).
3. After all work settles, the panic is re-raised at the block boundary.


#### 18.7.2 Multiple Panic Handling

**Dynamic Semantics**

1. Each panic is captured independently.
2. All work completes or is cancelled.
3. The first panic (by completion order) is raised.
4. Other panics are discarded; implementations MAY log them.


#### 18.7.3 Panic and Cancellation

**Dynamic Semantics**

Implementations MAY request cancellation on first panic to expedite block completion. This behavior is Implementation-Defined.


### 18.8 Determinism and Nesting


#### 18.8.1 Determinism Guarantees

**Determinism.** Given identical inputs and parallel structure, execution produces identical results.

**Static Semantics**

Dispatch is deterministic when:

1. Key patterns produce identical partitioning across runs.
2. Iterations with the same key execute in index order.
3. Reduction uses deterministic tree combination.

The `[ordered]` attribute forces sequential side-effect ordering. Iterations MAY execute in parallel, but side effects (I/O, shared mutation) are buffered and applied in index order.


#### 18.8.2 Nested Parallelism Rules

**Nested Parallelism.** Parallel blocks may be nested. Inner blocks execute within the context established by outer blocks.

**Dynamic Semantics**

**CPU Nesting**

Inner CPU parallel blocks share the worker pool with outer blocks. The `workers` parameter on inner blocks is a hint or limit, not additional workers.

**Heterogeneous Nesting**

CPU and GPU blocks may be nested.

**Constraints**

1. GPU parallel blocks MUST NOT be nested inside other GPU parallel blocks.
2. Inner CPU blocks share the outer block's worker pool.
3. Capture rules apply independently at each nesting level.


#### 18.8.3 Memory Allocation in Parallel

**Parallel Allocation.** Work items may allocate memory using captured allocator capabilities.

**Dynamic Semantics**

**Captured Allocator**

Work items may capture `ctx.heap` and invoke allocation methods.

**Region Allocation**

Work items executing within a `region` block may allocate from that region using the `^` operator.

---

## 19. Asynchronous Operations

---

### 19.1 Async Model


#### 19.1.1 `Async` Class Definition

**Async Class.** `Async<Out, In, Result, E>` is a modal class representing the interface for asynchronous computations.

**Syntax**

```ebnf
async_class ::= "class" "Async" "<" type_param (";" type_param)* ">"
type_param  ::= identifier ("=" type)?
```

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

    procedure resume(~!, input: In) -> Async@Suspended | Async@Completed | Async@Failed;
}
```

**Static Semantics**

**Type Parameters**

| Parameter | Meaning                                          | Default  |
| :-------- | :----------------------------------------------- | :------- |
| `Out`     | Type of values produced at each suspension point | Required |
| `In`      | Type of values received when resumed             | `()`     |
| `Result`  | Type of the final completion value               | `()`     |
| `E`       | Type of the error on failure                     | `!`      |

**Modal States**

| State        | Meaning                           | Payload         |
| :----------- | :-------------------------------- | :-------------- |
| `@Suspended` | Computation paused                | `output: Out`   |
| `@Completed` | Computation finished successfully | `value: Result` |
| `@Failed`    | Computation terminated with error | `error: E`      |

When `E = !`, the `@Failed` state is uninhabited and may be omitted from implementing modals per §9.3.

**Class Subtyping Rule**

**(Sub-Async-Class)**
$$\frac{
    \Gamma \vdash Out_1 <: Out_2 \quad
    \Gamma \vdash In_2 <: In_1 \quad
    \Gamma \vdash Result_1 <: Result_2 \quad
    \Gamma \vdash E_1 <: E_2
}{
    \Gamma \vdash \texttt{Async}\langle Out_1, In_1, Result_1, E_1 \rangle <:
                  \texttt{Async}\langle Out_2, In_2, Result_2, E_2 \rangle
}$$

**Default Parameter Expansion**

- `Async<T>` $\equiv$ `Async<T, (), (), !>`
- `Async<T, U>` $\equiv$ `Async<T, U, (), !>`
- `Async<T, U, R>` $\equiv$ `Async<T, U, R, !>`

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CON-0201`–`E-CON-0203`.


#### 19.1.2 Type Aliases (`Future`, `Sequence`, `Stream`, `Pipe`, `Exchange`)

**Async Aliases.** Type aliases provide names for common `Async` configurations.

**Syntax**

```cursive
type Sequence<T> = Async<T, (), (), !>
type Future<T; E = !> = Async<(), (), T, E>
type Pipe<In; Out> = Async<Out, In, (), !>
type Exchange<T> = Async<T, T, T, !>
type Stream<T; E> = Async<T, (), (), E>
```

**Static Semantics**

**Equivalence**

$$\texttt{Sequence}\langle T \rangle \equiv \texttt{Async}\langle T, (), (), ! \rangle$$

$$\texttt{Future}\langle T, E \rangle \equiv \texttt{Async}\langle (), (), T, E \rangle$$

$$\texttt{Pipe}\langle In, Out \rangle \equiv \texttt{Async}\langle Out, In, (), ! \rangle$$

$$\texttt{Stream}\langle T, E \rangle \equiv \texttt{Async}\langle T, (), (), E \rangle$$

**Variance**

| Alias           | Expansion               | Variance                              |
| :-------------- | :---------------------- | :------------------------------------ |
| `Sequence<T>`   | `Async<T, (), (), !>`   | Covariant in T                        |
| `Future<T, E>`  | `Async<(), (), T, E>`   | Covariant in T, E                     |
| `Pipe<In, Out>` | `Async<Out, In, (), !>` | Contravariant in In, Covariant in Out |
| `Stream<T, E>`  | `Async<T, (), (), E>`   | Covariant in T, E                     |


#### 19.1.3 Async State Machine

*[REF: Async procedures generate modal types per §12.1. Define state machine transformation only.]*

**Async State Machine.** A procedure returning `Async<Out, In, Result, E>` generates an anonymous modal type implementing the class.

**Syntax**

```ebnf
async_procedure ::= procedure_decl
```

No modifier is required; the return type determines async behavior.

**Static Semantics**

**Modal Generation**

When a procedure's return type is `Async<Out, In, Result, E>`:

1. An anonymous modal implementing `Async<Out, In, Result, E>` is generated.
2. The modal has states `@Suspended`, `@Completed`, and (unless `E = !`) `@Failed`.
3. State payloads include class-required fields plus implementation fields.
4. The procedure body is transformed into the `resume` transition.
5. Each `yield` becomes a suspension point; the modal stores the resumption point in a `gen_point` field.
6. `result` produces `@Completed`.
7. Error propagation via `?` transitions to `@Failed`.

**(T-Async-Proc)**
$$\frac{
    \Gamma \vdash \text{body} : Result \quad
    \text{yields in body have type } Out \quad
    \text{yields receive type } In
}{
    \Gamma \vdash \texttt{procedure}\ f(\ldots) \to \texttt{Async}\langle Out, In, Result, E \rangle\ \{\ \text{body}\ \}\ \text{wf}
}$$

**(T-Async-Call)**
$$\frac{
    \Gamma \vdash f : (\overline{T}) \to \texttt{Async}\langle Out, In, Result, E \rangle \quad
    \text{gen\_}f \text{ is the generated modal for } f
}{
    \Gamma \vdash f(\overline{e}) : \text{gen\_}f
}$$

**Generated Type Properties**

Generated modals:
1. Have the exact state names required by the class
2. Include all class-required payload fields
3. Add implementation fields prefixed with `gen_`
4. Use naming scheme that avoids collision with user types

**Dynamic Semantics**

**Size Formula**

$$\text{sizeof}(\text{gen\_}f) = \text{sizeof}(\text{Discriminant}) + \max_{S \in \text{States}}(\text{sizeof}(S.\text{payload}))$$

**Constraints**

**Diagnostics:** See Appendix A, code `E-CON-0230`.


### 19.2 Suspension


#### 19.2.1 Suspension Points

**Suspension Point.** A program point where an async computation yields control to its caller.

**Static Semantics**

Suspension points occur at:
1. `yield` expressions
2. `yield from` expressions
3. I/O operations on async-returning capability methods

At each suspension point, the async transitions to `@Suspended` and execution returns to the caller.


#### 19.2.2 `yield` Expression

**Yield Expression.** An expression that suspends an async computation, producing an intermediate value and awaiting resumption.

**Syntax**

```ebnf
yield_expr ::= "yield" "release"? expression
```

**Static Semantics**

**(T-Yield)**
$$\frac{
    \text{ReturnType}(\text{enclosing}) = \texttt{Async}\langle Out, In, Result, E \rangle \quad
    \Gamma \vdash e : Out
}{
    \Gamma \vdash \texttt{yield}\ e : In
}$$

The operand MUST have type `Out`. The expression evaluates to type `In`.

**Context Requirement**

`yield` is valid only within a procedure returning `Async<Out, In, Result, E>`.

**Key Interaction**

See §19.4.2 for key prohibition rules. The `release` modifier enables yielding while keys are held.

**Dynamic Semantics**

Evaluation of `yield e`:

1. Evaluate $e$ to $v$.
2. Transition to `@Suspended { output: v }`.
3. Return control to caller.
4. When `resume(input)` is called, bind $input$ as the result of the `yield` expression.
5. Continue execution after `yield`.

**Yield Release Semantics**

When `yield release` is used within a key block:

1. Release all held keys per LIFO semantics (§3.5.2).
2. Evaluate operand to $v$.
3. Transition to `@Suspended { output: v }`.
4. Return control to caller.
5. When resumed, reacquire all keys in canonical order.
6. Bind input as result; continue execution.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CON-0210`–`E-CON-0213`.


#### 19.2.3 `yield from` Expression

**Yield From Expression.** An expression that delegates to another async computation, forwarding outputs and inputs until completion.

**Syntax**

```ebnf
yield_from_expr ::= "yield" "release"? "from" expression
```

**Static Semantics**

**(T-Yield-From)**
$$\frac{
    \text{ReturnType}(\text{enclosing}) = \texttt{Async}\langle Out, In, \_, E_1 \rangle \quad
    \Gamma \vdash e : \texttt{Async}\langle Out, In, R, E_2 \rangle \quad
    E_2 <: E_1
}{
    \Gamma \vdash \texttt{yield from}\ e : R
}$$

**Compatibility Requirements**

The delegated async MUST have:
- Identical `Out` parameter
- Identical `In` parameter
- Error type `E_2 <: E_1`

The expression evaluates to the delegated computation's `Result` type.

**Dynamic Semantics**

**Delegation Loop**

Evaluation of `yield from source`:

1. Evaluate $source$ to $s$.
2. Loop:
   - Match $s$:
     - `@Suspended { output }`: Execute `yield output`. When resumed with $input$, let $s$ := $s$~>resume$(input)$.
     - `@Completed { value }`: Evaluate to $value$. Exit loop.
     - `@Failed { error }`: Propagate $error$. Exit loop.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CON-0220`–`E-CON-0225`.


### 19.3 Consumption and Composition


#### 19.3.1 Iteration (`loop...in`)

**Async Loop.** The `loop...in` construct iterates over an `Async<T, (), R, E>`, consuming each yielded value until completion or failure.

**Syntax**

```ebnf
async_loop ::= "loop" pattern "in" expression block
```

**Static Semantics**

**(T-Async-Loop)**
$$\frac{
    \Gamma \vdash e : \texttt{Async}\langle T, (), R, E \rangle \quad
    \Gamma, x : T \vdash \text{body} : ()
}{
    \Gamma \vdash \texttt{loop}\ x\ \texttt{in}\ e\ \{ \text{body} \} : ()
}$$

Iteration is available only for async values with `In = ()`.

**Dynamic Semantics**

Iteration repeatedly matches the async value’s modal state and resumes with unit input until `@Completed` or `@Failed`, per the `Async` state machine (§19.1.3).

**Constraints**

**Diagnostics:** See Appendix A, code `E-CON-0240`.


#### 19.3.2 Manual Stepping

**Manual Stepping.** Direct interaction with the `Async` state machine via pattern matching and explicit `resume` invocation.

**Dynamic Semantics**

The consumer matches on the async value's modal state and invokes `resume` to advance. This pattern is required for async values with non-unit `In` types.


#### 19.3.3 Synchronous Execution (`sync`)

**Sync Expression.** The `sync` expression runs a `Future<T, E>` to completion synchronously.

**Syntax**

```ebnf
sync_expr ::= "sync" expression
```

**Static Semantics**

**(T-Sync)**
$$\frac{
    \Gamma \vdash e : \texttt{Async}\langle (), (), T, E \rangle
}{
    \Gamma \vdash \texttt{sync}\ e : T \mid E
}$$

The operand MUST have `Out = ()` and `In = ()`.

**Context Restriction**

`sync` is permitted only in non-async contexts.

**Dynamic Semantics**

Evaluation of `sync e`:

1. Evaluate $e$ to $a$.
2. Loop:
   - Match $a$:
     - `@Suspended { output: () }`: Let $a$ := $a$~>resume$(())$.
     - `@Completed { value }`: Return $value$.
     - `@Failed { error }`: Propagate $error$.

**Reactor Integration**

$$\texttt{sync}\ f \quad \rightarrow \quad \texttt{ctx.reactor\sim>run}(f)$$

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CON-0250`–`E-CON-0252`.


#### 19.3.4 `race` Expression

**Race Expression.** An expression that initiates multiple async operations concurrently and returns when the first completes.

**Syntax**

```ebnf
race_expr    ::= "race" "{" race_arm ("," race_arm)* ","? "}"
race_arm     ::= expression "->" "|" pattern "|" race_handler
race_handler ::= expression | "yield" expression
```

**Static Semantics**

**(T-Race)**
$$\frac{
    \forall i \in 1..n.\ \Gamma \vdash e_i : \texttt{Async}\langle (), (), T_i, E_i \rangle \quad
    \forall i \in 1..n.\ \Gamma, x_i : T_i \vdash r_i : R \quad
    n \geq 2 \quad
    \text{no handler uses yield}
}{
    \Gamma \vdash \texttt{race}\ \{ e_1 \to |x_1|\ r_1, \ldots, e_n \to |x_n|\ r_n \} : R \mid (E_1 | \ldots | E_n)
}$$

**(T-Race-Stream)**
$$\frac{
    \forall i \in 1..n.\ \Gamma \vdash e_i : \texttt{Async}\langle T_i, (), R_i, E_i \rangle \quad
    \forall i \in 1..n.\ \Gamma, x_i : T_i \vdash u_i : U \quad
    \text{all handlers use yield}
}{
    \Gamma \vdash \texttt{race}\ \{ \ldots \} : \texttt{Sequence}\langle U, E_1 | \ldots | E_n \rangle
}$$

**Dynamic Semantics**

**First-Completion Evaluation**

1. Initiate all async expressions concurrently.
2. When any operation reaches `@Completed` or `@Failed`:
   - Execute the corresponding handler.
   - Cancel all other operations.
   - Return the handler's result.

**Streaming Evaluation**

1. Initiate all async expressions concurrently.
2. When any arm yields: execute handler, yield result, resume arm.
3. When an arm completes: remove from race.
4. When all arms complete: race completes.
5. If any arm fails: propagate error, cancel remaining.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CON-0260`–`E-CON-0263`.


#### 19.3.5 `all` Expression

**All Expression.** An expression that initiates multiple async operations concurrently and returns when all complete successfully.

**Syntax**

```ebnf
all_expr ::= "all" "{" expression ("," expression)* ","? "}"
```

**Static Semantics**

**(T-All)**
$$\frac{
    \forall i \in 1..n.\ \Gamma \vdash e_i : \texttt{Async}\langle (), (), T_i, E_i \rangle
}{
    \Gamma \vdash \texttt{all}\ \{ e_1, \ldots, e_n \} : (T_1, \ldots, T_n) \mid (E_1 | \ldots | E_n)
}$$

**Dynamic Semantics**

**All-Completion Evaluation**

1. Initiate all async expressions concurrently.
2. Wait for all operations to complete.
3. If all succeed: return tuple of results in declaration order.
4. If any fails: cancel remaining, propagate first error.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CON-0270`, `E-CON-0271`.


#### 19.3.6 `until` Expression

**Until Expression.** Condition waiting that suspends an async operation until a predicate on shared data becomes true.

**Syntax**

**Method Signature**

```cursive
procedure until<T; R>(
    self: shared T,
    predicate: procedure(const T) -> bool,
    action: procedure(unique T) -> R,
) -> Future<R>
```

**Dynamic Semantics**

Evaluation of `shared_value~>until(pred, action)`:

1. If `pred(shared_value)` is true immediately:
   - Acquire Write key.
   - Execute `action(shared_value)`.
   - Complete the Future with result.
2. Otherwise:
   - Register waiter record.
   - Transition to `@Suspended { output: () }`.

**Notification on Key Release**

When a Write key is released, matching waiters re-evaluate predicates. Waiters whose predicates return `true` are scheduled for execution.


#### 19.3.7 Transformations and Combinators

**Async Combinators.** Methods that transform and compose async computations.

**Static Semantics**

**`map`**

```cursive
procedure map<Out; In; Result; E; U>(
    self: Async<Out, In, Result, E>,
    f: procedure(Out) -> U,
) -> Async<U, In, Result, E>
```

**(T-Async-Map)**
$$\frac{
    \Gamma \vdash a : \texttt{Async}\langle Out, In, R, E \rangle \quad
    \Gamma \vdash f : Out \to U
}{
    \Gamma \vdash a\texttt{~>map}(f) : \texttt{Async}\langle U, In, R, E \rangle
}$$

**`filter`**

```cursive
procedure filter<T; E>(
    self: Async<T, (), (), E>,
    predicate: procedure(const T) -> bool,
) -> Async<T, (), (), E>
```

**`take`**

```cursive
procedure take<T; E>(
    self: Async<T, (), (), E>,
    count: usize,
) -> Async<T, (), (), E>
```

**`fold`**

```cursive
procedure fold<T; A; E>(
    self: Async<T, (), (), E>,
    initial: A,
    combine: procedure(A, T) -> A,
) -> Future<A, E>
```

**`chain`**

```cursive
procedure chain<T; U; E>(
    self: Future<T, E>,
    next: procedure(T) -> Future<U, E>,
) -> Future<U, E>
```

**(T-Async-Chain)**
$$\frac{
    \Gamma \vdash a : \texttt{Future}\langle T, E \rangle \quad
    \Gamma \vdash f : T \to \texttt{Future}\langle U, E \rangle
}{
    \Gamma \vdash a\texttt{~>chain}(f) : \texttt{Future}\langle U, E \rangle
}$$

**Dynamic Semantics**

**Lazy Evaluation**

Combinators return new async values that apply transformations when resumed:

1. `map(f)`: When source yields $v$, yield $f(v)$.
2. `filter(p)`: When source yields $v$, yield $v$ only if $p(v)$ is true.
3. `take(n)`: Yield first $n$ values, then complete.
4. `fold(init, f)`: Consume all values, accumulate, complete with final.
5. `chain(f)`: When source completes with $v$, delegate to $f(v)$.


### 19.4 Async-Key Integration


#### 19.4.1 Key State at Suspension

*[REF: Key system defined in §17. Define async-specific key behavior only.]*

**Suspension Key State.** Async suspension points interact with the Key System by managing the lifecycle of held keys.

**Static Semantics**

At suspension, the task releases access rights. Other tasks MAY acquire keys to the same paths during the suspension period.


#### 19.4.2 Key Prohibition in Yield

**Yield Key Prohibition.** By default, `yield` or `yield from` MUST NOT occur while keys are held.

**Static Semantics**

**(A-No-Keys)**
$$\frac{
    \Gamma_{\text{keys}} \neq \emptyset \quad
    (\text{IsYield}(e) \lor \text{IsYieldFrom}(e)) \quad
    \texttt{release} \notin \text{modifiers}
}{
    \text{Emit}(\texttt{E-CON-0213})
}$$

**Yield Release Mechanism**

*[REF: Complete yield release semantics defined in §19.2.2. Summary for key context:]*

The `release` modifier enables suspension with held keys. Keys are released (§3.5.2), the task suspends, and upon resumption keys are re-acquired in canonical order.

**Staleness**

Bindings derived from shared data before `yield release` are potentially stale after resumption.

**Constraints**

**Diagnostics:** See Appendix A, code `W-CON-0011`.


#### 19.4.3 Async Capability Requirements

**Async Capabilities.** Async operations require specific capabilities based on their effects.

**Static Semantics**

**Capability Classification**

| Async Category | Capability Required | Example                |
| :------------- | :------------------ | :--------------------- |
| Pure sequence  | No capability       | `range(0, 100)`        |
| I/O operation  | Specific capability | `fs~>read(file)`       |
| Timing         | `System`            | `sys~>after(duration)` |
| Network        | `Network`           | `net~>fetch(url)`      |

**Reactor Capability**

The async runtime is provided by the `$Reactor` capability carried in `Context`. The `$Reactor` interface (including `run` and `register`) is defined in Appendix D §D.3.4.


#### 19.4.4 Async Error Handling

**Async Errors.** Errors propagate through async boundaries via `?` or explicit pattern matching on `@Failed`.

**Static Semantics**

**(T-Async-Try)**
$$\frac{
    \text{ReturnType}(\text{enclosing}) = \texttt{Async}\langle Out, In, R, E_1 \rangle \quad
    \Gamma \vdash e : T \mid E_2 \quad
    E_2 <: E_1
}{
    \Gamma \vdash e? : T
}$$

A procedure returning `Async<Out, In, Result, !>` MUST NOT contain error-propagating expressions.

**Dynamic Semantics**

**Cleanup on Failure**

1. Error captured in `@Failed` state.
2. `defer` blocks execute in reverse order.
3. `Drop` implementations run for live bindings.
4. Async transitions to `@Failed { error }`.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CON-0280`, `E-CON-0281`, `W-CON-0201`.

To allow an async value to outlive its defining region, explicitly convert the captured state to heap provenance using `(move state)~>to_heap(heap)` (or equivalent) per §3.2.7 before the escape point.

---

# Part VII: Metaprogramming and Interop

## 20. Metaprogramming

---

### 20.1 Comptime Environment


#### 20.1.1 `comptime` Block Syntax

**Comptime Block.** A block of statements executed during the Metaprogramming Phase. A **comptime expression** is an expression evaluated at compile time, yielding a value incorporated into the program.

**Syntax**

```ebnf
comptime_block ::= "comptime" block
comptime_expr  ::= "comptime" "{" expression "}"
```

**Static Semantics**

**Typing Rules**

Comptime block (statement context):

**(T-ComptimeBlock)**
$$\frac{\Gamma_{ct} \vdash \textit{stmts} : ()}{\Gamma \vdash \texttt{comptime}\; \{\; \textit{stmts} \;\} : ()}$$

Comptime expression (expression context):

**(T-ComptimeExpr)**
$$\frac{\Gamma_{ct} \vdash e : T \quad \text{IsComptimeAvailable}(T)}{\Gamma \vdash \texttt{comptime}\; \{\; e \;\} : T}$$

The body of a comptime block or expression MUST satisfy all comptime restrictions (§20.1).

**Dynamic Semantics**

1. The implementation evaluates the comptime block body in $\Gamma_{ct}$.
2. For comptime expressions, the resulting value $v$ is converted to a literal node in the AST.
3. The literal node replaces the comptime expression in subsequent translation phases.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CTE-0020`–`E-CTE-0023`.


#### 20.1.2 Comptime-Available Types

**Comptime-Available Type.** A type whose values can exist within the comptime environment $\Gamma_{ct}$.

**Static Semantics**

**Formal Definition**

$$\text{IsComptimeAvailable}(T) \iff T \in \mathcal{T}_{ct}$$

where $\mathcal{T}_{ct}$ is defined inductively:

$$\mathcal{T}_{ct} ::= \text{Primitive} \mid \texttt{string} \mid \texttt{Type} \mid \texttt{Ast} \mid \text{Tuple}(\mathcal{T}_{ct}^*) \mid \text{Array}(\mathcal{T}_{ct}) \mid \text{Record}_{ct} \mid \text{Enum}_{ct}$$

**Typing Rules**

Primitive types are comptime-available:

**(CT-Prim)**
$$\frac{T \in \{\texttt{bool}, \texttt{i8}, \texttt{i16}, \texttt{i32}, \texttt{i64}, \texttt{i128}, \texttt{u8}, \texttt{u16}, \texttt{u32}, \texttt{u64}, \texttt{u128}, \texttt{f32}, \texttt{f64}, \texttt{char}\}}{\text{IsComptimeAvailable}(T)}$$

**(CT-String)**
$$\frac{}{\text{IsComptimeAvailable}(\texttt{string})}$$

**(CT-Type)**
$$\frac{}{\text{IsComptimeAvailable}(\texttt{Type})}$$

**(CT-Ast)**
$$\frac{}{\text{IsComptimeAvailable}(\texttt{Ast})}$$

**(CT-Tuple)**
$$\frac{\forall i \in 1..n.\; \text{IsComptimeAvailable}(T_i)}{\text{IsComptimeAvailable}((T_1, \ldots, T_n))}$$

**(CT-Array)**
$$\frac{\text{IsComptimeAvailable}(T)}{\text{IsComptimeAvailable}([T; n])}$$

**(CT-Record)**
$$\frac{\texttt{record}\; R\; \{\; f_1: T_1, \ldots, f_n: T_n \;\} \quad \forall i.\; \text{IsComptimeAvailable}(T_i)}{\text{IsComptimeAvailable}(R)}$$

**(CT-Enum)**
$$\frac{\texttt{enum}\; E\; \{\; V_1(T_1), \ldots, V_n(T_n) \;\} \quad \forall i.\; \text{IsComptimeAvailable}(T_i)}{\text{IsComptimeAvailable}(E)}$$

**Non-Comptime Types**

| Type Category                                     |
| :------------------------------------------------ |
| Reference types                                   |
| Capability types                                  |
| Modal types                                       |
| Types containing dynamic class objects (`$Class`) |

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CTE-0010`–`E-CTE-0012`.


#### 20.1.3 Comptime Procedures

**Comptime Procedure.** A procedure callable only during the Metaprogramming Phase. Comptime procedures operate on comptime-available types and may access comptime capabilities.

**Syntax**

```ebnf
comptime_procedure_decl ::= "comptime" "procedure" identifier generic_params?
                            "(" param_list ")" "->" return_type
                            requires_clause? block

requires_clause ::= "requires" comptime_predicate ("," comptime_predicate)*
comptime_predicate ::= comptime_expr
```

**Static Semantics**

**(T-ComptimeProc)**
$$\frac{
    \forall i.\; \text{IsComptimeAvailable}(T_i) \quad
    \text{IsComptimeAvailable}(R) \quad
    \Gamma_{ct}, x_1: T_1, \ldots, x_n: T_n \vdash \textit{body} : R
}{
    \Gamma \vdash \texttt{comptime procedure}\; f(x_1: T_1, \ldots, x_n: T_n) \to R\; \{\; \textit{body} \;\} : \text{wf}
}$$

**(T-ComptimeCall)**
$$\frac{
    \Gamma_{ct} \vdash f : (T_1, \ldots, T_n) \to R \quad
    \forall i.\; \Gamma_{ct} \vdash e_i : T_i
}{
    \Gamma_{ct} \vdash f(e_1, \ldots, e_n) : R
}$$

**Validation**

1. All parameter types MUST be comptime-available.
2. The return type MUST be comptime-available.
3. The procedure body MUST satisfy comptime restrictions.
4. If a `requires` clause is present, all predicates MUST evaluate to `true` at each call site.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CTE-0030`–`E-CTE-0034`.


#### 20.1.4 Comptime Control Flow

**Comptime Control Flow.** Control flow statements evaluated entirely at compile time, producing unrolled or selected code.

**Syntax**

```ebnf
comptime_if  ::= "comptime" "if" comptime_expr block ("else" (comptime_if | block))?
comptime_for ::= "comptime" "for" pattern "in" comptime_expr block
```

**Static Semantics**

**(T-ComptimeIf)**
$$\frac{
    \Gamma_{ct} \vdash e : \texttt{bool} \quad
    \Gamma \vdash \textit{then} : T \quad
    \Gamma \vdash \textit{else} : T
}{
    \Gamma \vdash \texttt{comptime if}\; e\; \textit{then}\; \texttt{else}\; \textit{else} : T
}$$

**(T-ComptimeFor)**
$$\frac{
    \Gamma_{ct} \vdash e : [T] \quad
    \Gamma, x: T \vdash \textit{body} : U
}{
    \Gamma \vdash \texttt{comptime for}\; x\; \texttt{in}\; e\; \textit{body} : [U]
}$$

**Dynamic Semantics**

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

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CTE-0080`–`E-CTE-0084`.


#### 20.1.5 Resource Limits

**Resource Limits.** Implementation-enforced bounds on compile-time execution to guarantee termination.

**Static Semantics**

A conforming implementation MUST enforce the following minimum guaranteed capacities:

| Resource                | Minimum Limit | Diagnostic on Exceed |
| :---------------------- | :------------ | :------------------- |
| Recursion depth         | 128 calls     | `E-CTE-0100`         |
| Loop iterations         | 65,536        | `E-CTE-0101`         |
| Memory allocation       | 64 MiB        | `E-CTE-0102`         |
| AST nodes generated     | 1,000,000     | `E-CTE-0103`         |
| Comptime procedure time | 60 seconds    | `E-CTE-0104`         |

Implementations MAY support higher limits. The actual limits are implementation-defined.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CTE-0100`–`E-CTE-0104`.


### 20.2 Capabilities


#### 20.2.1 `TypeEmitter` Capability

**TypeEmitter Capability.** A capability that authorizes emission of generated code into the compilation unit.

**Attribute Syntax**

```ebnf
emit_attribute ::= "[[" "emit" "]]"
```

**Syntax**

```cursive
capability TypeEmitter {
    procedure emit(~, ast: Ast)
    procedure acquire_write_key(~, target: Type, form: Type) -> WriteKey
    procedure target_type(~) -> Type
}
```

**Static Semantics**

**Provision**

The `TypeEmitter` capability is provided to:

1. Comptime blocks annotated with `[[emit]]`
2. Derive target procedure bodies (§20.5)

The capability is available via the identifier `emitter`.

**(T-Emit)**
$$\frac{
    \Gamma_{ct} \vdash \textit{emitter} : \texttt{TypeEmitter} \quad
    \Gamma_{ct} \vdash \textit{ast} : \texttt{Ast}
}{
    \Gamma_{ct} \vdash \textit{emitter}\texttt{~>emit}(\textit{ast}) : ()
}$$

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CTE-0040`–`E-CTE-0042`.


#### 20.2.2 `Introspect` Capability

**Introspect Capability.** A capability that authorizes reflection over type structure at compile time.

**Syntax**

```cursive
capability Introspect {
    procedure category<T>() -> TypeCategory
    procedure fields<T>() -> [FieldInfo]
        requires category::<T>() == TypeCategory::Record
    procedure variants<T>() -> [VariantInfo]
        requires category::<T>() == TypeCategory::Enum
    procedure states<T>() -> [StateInfo]
        requires category::<T>() == TypeCategory::Modal
    procedure implements_form<T; F>() -> bool
    procedure type_name<T>() -> string
    procedure module_path<T>() -> string
}
```

*[Informative: supporting types returned by `Introspect` (such as `TypeCategory`, `FieldInfo`, `VariantInfo`, `StateInfo`, `TransitionInfo`, and `Visibility`) are core‑library definitions. Their concrete shapes are not normative in this document.]* 

**Static Semantics**

**Provision**

The `Introspect` capability is implicitly provided to all comptime contexts. The capability is available via the identifier `introspect`.

**(T-Introspect-Category)**
$$\frac{
    \Gamma_{ct} \vdash T : \texttt{Type}
}{
    \Gamma_{ct} \vdash \textit{introspect}\texttt{~>category::<}T\texttt{>()} : \texttt{TypeCategory}
}$$

**(T-Introspect-Fields)**
$$\frac{
    \Gamma_{ct} \vdash T : \texttt{Type} \quad
    \text{category}(T) = \texttt{Record}
}{
    \Gamma_{ct} \vdash \textit{introspect}\texttt{~>fields::<}T\texttt{>()} : [\texttt{FieldInfo}]
}$$

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CTE-0050`–`E-CTE-0053`.


#### 20.2.3 `ProjectFiles` Capability

**ProjectFiles Capability.** A capability that authorizes read-only access to files within the project directory during compile time.

**Attribute Syntax**

```ebnf
files_attribute ::= "[[" "files" "]]"
```

**Syntax**

```cursive
capability ProjectFiles {
    procedure read(~, path: string) -> string | FileError
    procedure read_bytes(~, path: string) -> [u8] | FileError
    procedure exists(~, path: string) -> bool
    procedure list_dir(~, path: string) -> [string] | FileError
    procedure project_root(~) -> string
}

enum FileError {
    NotFound { path: string },
    PermissionDenied { path: string },
    NotUtf8 { path: string },
    IoError { message: string },
}
```

**Static Semantics**

**Provision**

The `ProjectFiles` capability is provided only to comptime blocks annotated with `[[files]]`. The capability is available via the identifier `files`.

**Path Resolution**

1. Paths MUST be relative to the project root directory.
2. Paths MUST NOT contain `..` components traversing above project root.
3. Paths MUST NOT follow symbolic links escaping the project directory.

**Determinism**

File contents are captured at the start of the Metaprogramming Phase. Modifications during compilation do not affect values returned by `read` operations.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CTE-0060`–`E-CTE-0064`.


#### 20.2.4 `ComptimeDiagnostics` Capability

**ComptimeDiagnostics Capability.** A capability that authorizes emission of compile-time diagnostics including errors, warnings, and notes.

**Syntax**

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

**Static Semantics**

**Provision**

The `ComptimeDiagnostics` capability is implicitly provided to all comptime contexts and derive target procedures. The capability is available via the identifier `diagnostics`.

**Never Type**

The `error` and `error_at` procedures have return type `!`. These procedures terminate the Metaprogramming Phase and do not return.

**Dynamic Semantics**

When `error` or `error_at` is invoked:

1. The diagnostic message is recorded with severity Error.
2. The Metaprogramming Phase terminates.
3. Compilation fails with diagnostic code `E-CTE-0070`.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CTE-0070`, `W-CTE-0071`.


### 20.3 Reflection


#### 20.3.1 `Type` Metatype

**Type Metatype.** A compile-time type whose values represent Cursive types. A value of type `Type` is a reified type descriptor.

**Syntax**

```ebnf
type_literal ::= "Type" "::<" type ">"
```

**Static Semantics**

**Formal Definition**

$$\texttt{Type} : \texttt{Kind}$$

where $\texttt{Kind}$ is the universe of types. For each type $T$ in the program, there exists a unique value $\langle T \rangle : \texttt{Type}$.

**(T-TypeLiteral)**
$$\frac{
    \Gamma \vdash T : \texttt{Type}_\text{wf}
}{
    \Gamma_{ct} \vdash \texttt{Type::<}T\texttt{>} : \texttt{Type}
}$$

The subscript $\text{wf}$ indicates that $T$ must be a well-formed type in context $\Gamma$.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CTE-0410`, `E-CTE-0411`.


#### 20.3.2 Type Categories

**Type Category.** A classification of a type by its fundamental structure. The `TypeCategory` enumeration partitions all Cursive types into disjoint categories.

**Static Semantics**

**Formal Definition**

$$\text{category} : \texttt{Type} \to \texttt{TypeCategory}$$

$$\texttt{TypeCategory} ::= \texttt{Record} \mid \texttt{Enum} \mid \texttt{Modal} \mid \texttt{Primitive} \mid \texttt{Tuple} \mid \texttt{Array} \mid \texttt{Procedure} \mid \texttt{Reference} \mid \texttt{Generic}$$

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
| Permission-qualified paths         | `Reference` |
| `T` where `T` is type parameter    | `Generic`   |

**Constraints**

**Diagnostics:** See Appendix A, code `E-CTE-0420`.


#### 20.3.3 Record Introspection

**Record Introspection.** Yields the ordered sequence of fields comprising a record type.

**Static Semantics**

**(T-Fields)**
$$\frac{
    \Gamma_{ct} \vdash T : \texttt{Type} \quad
    \text{category}(T) = \texttt{Record}
}{
    \Gamma_{ct} \vdash \texttt{introspect~>fields::<}T\texttt{>()} : [\texttt{FieldInfo}]
}$$

**Return Value**

The returned array contains one `FieldInfo` for each field of $T$, in declaration order. For a record with $n$ fields, the result has length $n$.

**Constraints**

**Diagnostics:** See Appendix A, code `E-CTE-0430`.


#### 20.3.4 Enum Introspection

**Enum Introspection.** Yields the variants comprising an enum type, including discriminant values and payload fields.

**Static Semantics**

**(T-Variants)**
$$\frac{
    \Gamma_{ct} \vdash T : \texttt{Type} \quad
    \text{category}(T) = \texttt{Enum}
}{
    \Gamma_{ct} \vdash \texttt{introspect~>variants::<}T\texttt{>()} : [\texttt{VariantInfo}]
}$$

**Return Value**

The returned array contains one `VariantInfo` for each variant of $T$, in declaration order. Unit variants have an empty `fields` array.

**Constraints**

**Diagnostics:** See Appendix A, code `E-CTE-0440`.


#### 20.3.5 Modal Introspection

*[REF: Modal type semantics defined in §12.1. This section defines introspection procedures only.]*

**Modal Introspection.** Yields the states and transitions comprising a modal type.

**Static Semantics**

**(T-States)**
$$\frac{
    \Gamma_{ct} \vdash T : \texttt{Type} \quad
    \text{category}(T) = \texttt{Modal}
}{
    \Gamma_{ct} \vdash \texttt{introspect~>states::<}T\texttt{>()} : [\texttt{StateInfo}]
}$$

**Constraints**

**Diagnostics:** See Appendix A, code `E-CTE-0450`.


#### 20.3.6 Type Predicates

**Type Predicates.** Comptime procedures returning boolean values based on type properties.

**Syntax**

```cursive
comptime procedure is_record<T>() -> bool
comptime procedure is_enum<T>() -> bool
comptime procedure is_modal<T>() -> bool
comptime procedure is_primitive<T>() -> bool
comptime procedure is_tuple<T>() -> bool
comptime procedure is_array<T>() -> bool
comptime procedure is_sized<T>() -> bool
comptime procedure is_bitcopy<T>() -> bool
comptime procedure is_form<T>() -> bool
```

**Static Semantics**

**Equivalences**

$$\texttt{is\_record::<}T\texttt{>()} \equiv (\text{category}(T) = \texttt{Record})$$

$$\texttt{is\_enum::<}T\texttt{>()} \equiv (\text{category}(T) = \texttt{Enum})$$

$$\texttt{is\_modal::<}T\texttt{>()} \equiv (\text{category}(T) = \texttt{Modal})$$

$$\texttt{is\_sized::<}T\texttt{>()} \equiv \exists n.\; \text{sizeof}(T) = n$$

$$\texttt{is\_bitcopy::<}T\texttt{>()} \equiv T <: \texttt{Bitcopy}$$

**Constraints**

**Diagnostics:** See Appendix A, code `E-CTE-0470`.


### 20.4 Code Generation


#### 20.4.1 `Ast` Type

**Ast Type.** An opaque type representing a syntax tree fragment. Values of type `Ast` are constructed via quote expressions and manipulated via splice expressions.

**Static Semantics**

**Formal Definition**

$$\texttt{Ast} ::= \texttt{Ast::Expr} \mid \texttt{Ast::Stmt} \mid \texttt{Ast::Item} \mid \texttt{Ast::Type} \mid \texttt{Ast::Pattern}$$

| Variant        | Syntactic Category            |
| :------------- | :---------------------------- |
| `Ast::Expr`    | Expressions (§15)             |
| `Ast::Stmt`    | Statements (§16)              |
| `Ast::Item`    | Items: type decls, procedures |
| `Ast::Type`    | Type expressions (§11)        |
| `Ast::Pattern` | Patterns (§15.7)              |

**Subtyping**

Each `Ast` variant is a subtype of `Ast`:

$$\texttt{Ast::Expr} <: \texttt{Ast}$$
$$\texttt{Ast::Stmt} <: \texttt{Ast}$$
$$\texttt{Ast::Item} <: \texttt{Ast}$$
$$\texttt{Ast::Type} <: \texttt{Ast}$$
$$\texttt{Ast::Pattern} <: \texttt{Ast}$$

**Constraints**

**Diagnostics:** See Appendix A, code `E-CTE-0210`.


#### 20.4.2 `quote` Expression

**Quote Expression.** Constructs an `Ast` value from literal Cursive syntax. The quoted code is not evaluated; it is captured as a syntax tree.

**Syntax**

```ebnf
quote_expr    ::= "quote" "{" quoted_content "}"
quote_type    ::= "quote" "type" "{" type "}"
quote_pattern ::= "quote" "pattern" "{" pattern "}"

quoted_content ::= expression | stmt_list | item_list

stmt_list     ::= statement+

item_list     ::= item+
item          ::= import_decl
               | using_decl
               | static_decl
               | record_decl
               | enum_decl
               | modal_decl
               | type_alias_decl
               | class_declaration
               | procedure_decl
               | comptime_procedure_decl
               | extern_block
               | derive_target_decl
```

**Static Semantics**

**(T-QuoteExpr)**
$$\frac{
    \textit{content} \text{ parses as expression}
}{
    \Gamma_{ct} \vdash \texttt{quote}\; \{\; \textit{content} \;\} : \texttt{Ast::Expr}
}$$

**(T-QuoteStmt)**
$$\frac{
    \textit{content} \text{ parses as statement sequence}
}{
    \Gamma_{ct} \vdash \texttt{quote}\; \{\; \textit{content} \;\} : \texttt{Ast::Stmt}
}$$

**(T-QuoteItem)**
$$\frac{
    \textit{content} \text{ parses as item}
}{
    \Gamma_{ct} \vdash \texttt{quote}\; \{\; \textit{content} \;\} : \texttt{Ast::Item}
}$$

**(T-QuoteType)**
$$\frac{
    \textit{type\_expr} \text{ parses as type}
}{
    \Gamma_{ct} \vdash \texttt{quote type}\; \{\; \textit{type\_expr} \;\} : \texttt{Ast::Type}
}$$

**(T-QuotePattern)**
$$\frac{
    \textit{pattern} \text{ parses as pattern}
}{
    \Gamma_{ct} \vdash \texttt{quote pattern}\; \{\; \textit{pattern} \;\} : \texttt{Ast::Pattern}
}$$

Quoted content MUST be syntactically valid. Type checking of quoted content is deferred until emission (§20.4.5).

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CTE-0220`, `E-CTE-0221`.


#### 20.4.3 `splice` Expression

**Splice Expression.** Inserts a compile-time value into a quote expression.

**Syntax**

```ebnf
splice_expr  ::= "$(" comptime_expr ")"
splice_ident ::= "$" identifier
```

The shorthand `$ident` is equivalent to `$(ident)`.

**Static Semantics**

**(T-SpliceAst)**
$$\frac{
    \Gamma_{ct} \vdash e : \texttt{Ast::}K \quad
    \text{context requires category } K
}{
    \Gamma_{ct} \vdash \texttt{\$(}e\texttt{)} : \text{spliced}
}$$

**(T-SpliceLiteral)**
$$\frac{
    \Gamma_{ct} \vdash e : T \quad
    \text{IsComptimeLiteral}(T)
}{
    \Gamma_{ct} \vdash \texttt{\$(}e\texttt{)} : \text{spliced}
}$$

**(T-SpliceIdent)**
$$\frac{
    \Gamma_{ct} \vdash e : \texttt{string} \quad
    \text{context is identifier position}
}{
    \Gamma_{ct} \vdash \texttt{\$(}e\texttt{)} : \text{spliced}
}$$

**Comptime Literal Types**

$$\text{IsComptimeLiteral}(T) \iff T \in \{\texttt{bool}, \texttt{i8}, \ldots, \texttt{i128}, \texttt{u8}, \ldots, \texttt{u128}, \texttt{f32}, \texttt{f64}, \texttt{char}, \texttt{string}\}$$

**Splice Context Compatibility**

| Splice Context   | Compatible Splice Types                            |
| :--------------- | :------------------------------------------------- |
| Expression       | `Ast::Expr`, comptime literals                     |
| Statement        | `Ast::Stmt`, `Ast::Expr`                           |
| Type position    | `Ast::Type`, `Type` (via `type_repr_of`)           |
| Pattern position | `Ast::Pattern`                                     |
| Identifier       | `string` (unhygienic), `Ast` with ident (hygienic) |

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CTE-0230`–`E-CTE-0233`.


#### 20.4.4 Hygiene

**Hygiene.** The property that identifiers introduced by code generation do not accidentally capture or shadow identifiers at the emission site.

**Static Semantics**

**Formal Definition**

For a quoted expression $q$:

**Capture set**: Identifiers referenced in $q$ but defined outside $q$:
$$\text{Captures}(q) = \text{FreeVars}(q) \cap \text{Dom}(\Gamma_{ct})$$

**Fresh name set**: Identifiers bound within $q$:
$$\text{Fresh}(q) = \text{BoundVars}(q)$$

**Hygiene Preservation Invariant**

For any identifier $x$ appearing in emitted code:

1. If $x \in \text{Captures}(q)$, then $x$ resolves to the same binding it referenced at the quote site.
2. If $x \in \text{Fresh}(q)$, then $x$ does not capture any binding at the emission site.

**Renaming Function**

$$\text{Rename}(x) = \text{gensym}(x, \text{EmissionContext})$$

The `gensym` function generates a unique identifier distinct from any identifier in both the quote scope and the emission scope.

**Hygiene Breaking**

| Splice Input Type | Hygiene Behavior                       |
| :---------------- | :------------------------------------- |
| `string`          | **Unhygienic**—binds in emission scope |
| `Ast`             | **Hygienic**—preserves original scope  |

**Dynamic Semantics**

Fresh names are generated deterministically using:

1. Original identifier name
2. Unique counter for emission context
3. Source location of quote

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CTE-0240`, `E-CTE-0241`.


#### 20.4.5 `emit` Statement

**Emission.** The process of inserting generated AST nodes into the compilation unit.

**Syntax**

```ebnf
emit_stmt ::= expression "~>" "emit" "(" expression ")"
```

**Static Semantics**

**(T-Emit)**
$$\frac{
    \Gamma_{ct} \vdash \textit{emitter} : \texttt{TypeEmitter} \quad
    \Gamma_{ct} \vdash \textit{ast} : \texttt{Ast::Item}
}{
    \Gamma_{ct} \vdash \textit{emitter}\texttt{~>emit}(\textit{ast}) : ()
}$$

**Emission Target**

Emitted items are inserted at module scope of the current compilation unit.

**Post-Emission Type Checking**

After the Metaprogramming Phase completes, the implementation MUST perform full type checking on the expanded AST. Type errors in emitted code MUST be reported with a diagnostic trace including:

1. The location of the emit call
2. The derive target or comptime block that generated the code
3. The specific type error in the emitted code

**Dynamic Semantics**

**Emission Order**

1. All derive targets execute in dependency order (§20.5).
2. Explicit `[[emit]]` blocks execute in declaration order.
3. Emitted items become visible to subsequent emissions within the same phase.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CTE-0250`–`E-CTE-0253`.


### 20.5 Derivation


#### 20.5.1 `derive` Attribute Syntax

**Derive Attribute.** A request for automatic generation of class implementations for an annotated type declaration.

**Syntax**

```ebnf
derive_attr        ::= "[[" "derive" "(" derive_target_list ")" "]]"
derive_target_list ::= derive_target ("," derive_target)*
derive_target      ::= identifier
```

A derive attribute MUST immediately precede a type declaration (`record`, `enum`, or `modal`).

**Static Semantics**

**(T-DeriveAttr)**
$$\frac{
    \texttt{[[derive(}D_1, \ldots, D_n\texttt{)]]} \text{ precedes type } T \quad
    \forall i.\; D_i \text{ is a valid derive target}
}{
    \text{schedule derive execution for } (T, D_1), \ldots, (T, D_n)
}$$

Each identifier in the derive target list MUST resolve to a derive target declaration (§20.5.2) or a standard derive target (§20.5.4).

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CTE-0310`–`E-CTE-0312`.


#### 20.5.2 Derive Target Declarations

**Derive Target Declaration.** A named code generator that produces class implementations for annotated types.

**Syntax**

```ebnf
derive_target_decl ::= "derive" "target" identifier
                       "(" "target" ":" "Type" ")"
                       derive_contract?
                       block

derive_contract ::= "|=" derive_clause ("," derive_clause)*
derive_clause   ::= "emits" class_ref
                 | "requires" class_ref
class_ref       ::= identifier
```

**Static Semantics**

**(T-DeriveTarget)**
$$\frac{
    \Gamma_{ct}, \textit{target}: \texttt{Type}, \textit{emitter}: \texttt{TypeEmitter} \vdash \textit{body} : ()
}{
    \Gamma \vdash \texttt{derive target}\; N\;(\texttt{target}: \texttt{Type})\; \{\; \textit{body} \;\} : \text{DeriveTarget}
}$$

**Contract Semantics**

| Clause       | Meaning                                              |
| :----------- | :--------------------------------------------------- |
| `emits F`    | The derive target emits an implementation of class F |
| `requires F` | The target type must implement class F before derive |

**Implicit Bindings**

Within a derive target body:

| Identifier    | Type                  | Description                       |
| :------------ | :-------------------- | :-------------------------------- |
| `target`      | `Type`                | The type being derived            |
| `emitter`     | `TypeEmitter`         | Capability for code emission      |
| `introspect`  | `Introspect`          | Capability for type introspection |
| `diagnostics` | `ComptimeDiagnostics` | Capability for error reporting    |

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CTE-0320`–`E-CTE-0322`.


#### 20.5.3 Derive Contracts

**Derive Contract.** A specification of the classes a derive target emits and the classes it requires as preconditions.

**Static Semantics**

**Formal Definition**

A derive contract is a pair $(E, R)$ where:
- $E \subseteq \text{Classes}$ is the set of classes emitted
- $R \subseteq \text{Classes}$ is the set of classes required

**Emits Clause**

For each `emits F` clause, the derive target MUST emit an AST node that, when type-checked, results in the target type implementing class $F$:

$$\texttt{emits } F \implies T <: F \text{ after derive execution}$$

**Requires Clause**

For each `requires F` clause, the target type MUST implement class $F$ before derive target execution:

$$\texttt{requires } F \implies T <: F \text{ before derive execution}$$

**Contract Verification**

After derive execution:

1. All `emits` classes are implemented by the target type.
2. No class is emitted that is not declared in an `emits` clause.

**Derive Execution Model**

Let $G = (V, E)$ be the derive dependency graph where:
- $V = \{(T, D) \mid T \text{ has } \texttt{[[derive(}D\texttt{)]]}\}$
- $E = \{((T, D_1), (T, D_2)) \mid D_1 \texttt{ requires } F \land D_2 \texttt{ emits } F\}$

Derive execution proceeds in topological order of $G$.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CTE-0330`–`E-CTE-0332`, `E-CTE-0340`–`E-CTE-0342`.


#### 20.5.4 Standard Derives

*[REF: Class definitions in §13.9. This section specifies derive target behavior only.]*

**Standard Derive Targets.** Derive targets provided by the language implementation for common classes.

| Target        | Contract                      | Applicable Categories |
| :------------ | :---------------------------- | :-------------------- |
| `Debug`       | `\|= emits Debug`             | Record, Enum          |
| `Clone`       | `\|= emits Clone`             | Record, Enum          |
| `Eq`          | `\|= emits Eq`                | Record, Enum          |
| `Hash`        | `\|= emits Hash, requires Eq` | Record, Enum          |
| `Default`     | `\|= emits Default`           | Record                |
| `Serialize`   | `\|= emits Serialize`         | Record, Enum          |
| `Deserialize` | `\|= emits Deserialize`       | Record, Enum          |

**Static Semantics**

For each standard derive, the implementation generates the corresponding class procedures with structural semantics over record fields or enum variants in declaration order. Generated signatures:

- `Debug`: `procedure debug(~) -> string`
- `Clone`: `procedure clone(~) -> Self`
- `Eq`: `procedure eq(~, other: const Self) -> bool`
- `Hash`: `procedure hash(~, hasher: unique Hasher)`
- `Default` (records only): `procedure default() -> Self`
- `Serialize` / `Deserialize`:  
  `procedure serialize(~, serializer: unique Serializer) -> () | SerializeError`  
  `procedure deserialize(deserializer: unique Deserializer) -> Self | DeserializeError`

The serialization format is Implementation‑Defined.

**Field Requirements**

All fields MUST implement the corresponding class being derived.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CTE-0360`–`E-CTE-0367`.

---

## 21. Foreign Function Interface

---

### 21.1 FfiSafe


#### 21.1.1 `FfiSafe` Class Definition

**FfiSafe Class.** A class that classifies types whose runtime representation is compatible with a stable C-style ABI.

**Syntax**

```ebnf
ffisafe_class ::= "class" "FfiSafe" "{" ffisafe_methods "}"

ffisafe_methods ::= c_size_method c_alignment_method verify_layout_method?

c_size_method ::= "comptime" "procedure" "c_size" "()" "->" "usize"

c_alignment_method ::= "comptime" "procedure" "c_alignment" "()" "->" "usize"

verify_layout_method ::= "comptime" "procedure" "verify_layout"
                        "(" param_list ")" "->" "bool" block
```

```cursive
class FfiSafe {
    comptime procedure c_size() -> usize;
    comptime procedure c_alignment() -> usize;
    comptime procedure verify_layout(expected_size: usize, expected_align: usize) -> bool {
        Self::c_size() == expected_size and Self::c_alignment() == expected_align
    }
}
```

**Static Semantics**

**Well-Formedness Rule (WF-FfiSafe)**

A type $T$ is FFI-safe if and only if:

1. $T$ declares `<: FfiSafe` and provides `c_size()` and `c_alignment()` implementations
2. $T$ has a complete, fixed layout
3. $T$ does not contain any prohibited types

$$\frac{
  T <: \texttt{FfiSafe} \quad
  \text{HasLayout}(T) \quad
  \neg\text{ContainsProhibited}(T)
}{
  \text{FfiSafe}(T)
} \quad \text{(WF-FfiSafe)}$$

**FfiSafe-Only Contexts**

Only `FfiSafe` types may appear in:

- Parameter and return types of foreign procedures
- Types of foreign globals
- Parameter and return types of exported Cursive procedures

**Prohibited Types**

The following types MUST NOT implement `FfiSafe`:

| Type Category                                     |
| :------------------------------------------------ |
| `bool`                                            |
| Modal types                                       |
| `Ptr<T>` (safe pointer)                           |
| Dynamic class object types (`$Class`)             |
| Capability classes                                |
| `Context`                                         |
| Types containing dynamic class objects (`$Class`) |
| Opaque classes (`opaque`)                         |
| Tuples                                            |
| Slices (`[T]`)                                    |
| String types                                      |
| Closure types (`                                  | T | -> U`) |

**RAII Types**

A type implementing both `Drop` and `FfiSafe` requires the `[[ffi_pass_by_value]]` attribute for by-value FFI passing.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-SYS-3301`, `E-SYS-3303`, `E-SYS-3305`, `E-SYS-3306`.


#### 21.1.2 Automatic `FfiSafe` Implementation

**Intrinsic FfiSafe Types.** Certain primitive and composite types have intrinsic `FfiSafe` implementations provided by the language.

**Static Semantics**

**Intrinsic Implementations**

| Type Category            | Examples                                   | Notes                                          |
| :----------------------- | :----------------------------------------- | :--------------------------------------------- |
| Signed integers          | `i8`, `i16`, `i32`, `i64`, `i128`, `isize` | Fixed-width, two's complement                  |
| Unsigned integers        | `u8`, `u16`, `u32`, `u64`, `u128`, `usize` | Fixed-width                                    |
| Floating-point           | `f16`, `f32`, `f64`                        | IEEE 754 representation                        |
| Raw pointers             | `*imm T`, `*mut T`                         | For any `T` (inner type need not be `FfiSafe`) |
| Fixed-size arrays        | `[T; N]`                                   | When `T <: FfiSafe`                            |
| Sparse function pointers | `(T₁, ..., Tₙ) -> R`                       | When all `Tᵢ` and `R` are `FfiSafe`            |

Primitive types implement `FfiSafe` with size and alignment matching the target C ABI.

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
```

**Generic FFI-Safe Types**

Type parameters appearing in the layout MUST be constrained to `FfiSafe`:

```cursive
[[layout(C)]]
record Pair<T <: FfiSafe> <: FfiSafe {
    first: T,
    second: T,
    comptime procedure c_size() -> usize { introspect~>size_of::<Pair<T>>() }
    comptime procedure c_alignment() -> usize { introspect~>align_of::<Pair<T>>() }
}
```

**Constraints**

**Diagnostics:** See Appendix A, codes `E-SYS-3302`, `E-SYS-3304`.


#### 21.1.3 `[[derive(FfiSafe)]]`

**Derive FfiSafe.** The `[[derive(FfiSafe)]]` attribute generates the `FfiSafe` implementation automatically for a type.

**Syntax**

```cursive
[[layout(C)]]
[[derive(FfiSafe)]]
record Point {
    x: f64,
    y: f64,
}
```

**Static Semantics**

`[[derive(FfiSafe)]]` is equivalent to writing an explicit `FfiSafe` implementation. It requires `[[layout(C)]]`, all fields `FfiSafe`, and no prohibited types; the generated implementation provides the required `c_size()` and `c_alignment()` methods.

**Constraints**

For diagnostic codes (`E-SYS-3301`, `E-SYS-3302`, `E-SYS-3303`), see §21.1.1 and §21.1.2.


### 21.2 Externs and Exports


#### 21.2.1 `extern` Procedure Declarations

**Foreign Procedure Declaration.** A declaration introducing a procedure whose implementation is provided by foreign (non-Cursive) code.

**Syntax**

```ebnf
extern_block        ::= "extern" abi_string? "{" extern_item* "}"
abi_string          ::= string_literal

extern_item         ::= foreign_procedure | foreign_global

foreign_procedure   ::= visibility? "procedure" identifier
                        "(" ffi_param_list? ")" ("->" type)?
                        variadic_spec? foreign_contract? ";"

variadic_spec       ::= "..." | "...," type

ffi_param_list      ::= ffi_param ("," ffi_param)*

ffi_param           ::= identifier ":" type
```

```cursive
extern "C" {
    procedure malloc(size: usize) -> *mut opaque c_void;
    procedure free(ptr: *mut opaque c_void);
    procedure printf(format: *imm c_char, ...) -> c_int;
}
```

**Static Semantics**

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

All parameter types and return types MUST be `FfiSafe`:

$$\frac{
  \texttt{extern}\ \textit{abi}\ \{\ \texttt{procedure}\ f(p_1: T_1, \ldots, p_n: T_n) \to R;\ \}
}{
  \forall i.\ T_i <: \texttt{FfiSafe} \quad R <: \texttt{FfiSafe}
} \quad \text{(T-Extern-Proc)}$$

**Variadic Procedures**

C-style variadic procedures use `...`. Variadic arguments undergo default argument promotion:

| Original Type            | Promoted To |
| :----------------------- | :---------- |
| `i8`, `i16`, `u8`, `u16` | `c_int`     |
| `f32`                    | `f64`       |
| All others               | Unchanged   |

**Call-Site Semantics**

Invoking a foreign procedure:

1. MUST occur within an `unsafe` block
2. Evaluates arguments left-to-right
3. Performs type promotions for variadic arguments
4. Transfers control to foreign code
5. Returns result upon foreign code completion

**Constraints**

**Diagnostics:** See Appendix A, codes `E-SYS-3310`–`E-SYS-3313`, `E-SYS-3320`, `E-SYS-3321`.


#### 21.2.2 `extern` Global Declarations

**Foreign Global Declaration.** A declaration introducing a global variable whose storage is provided by foreign code.

**Syntax**

```ebnf
foreign_global ::= visibility? "mut"? identifier ":" type ";"
```

```cursive
extern "C" {
    errno: c_int;
    mut environ: *mut *mut c_char;
}
```

**Static Semantics**

**Type Restriction**

The declared type MUST be `FfiSafe`.

**Mutability**

- Without `mut`: Read-only access; writes are ill-formed
- With `mut`: Read and write access permitted

**Access Semantics**

All access to foreign globals MUST occur within `unsafe` blocks.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-SYS-3330`–`E-SYS-3332`.


#### 21.2.3 `export` Procedures

**Exported Procedure.** A Cursive procedure made callable from foreign code via the `[[export]]` attribute.

**Syntax**

```ebnf
export_attr ::= "[[" "export" "(" string_literal ")" "]]"
              | "[[" "export" "(" string_literal "," export_opts ")" "]]"

export_opts ::= export_opt ("," export_opt)*

export_opt  ::= "link_name" ":" string_literal
```

```cursive
[[export("C")]]
public procedure add(a: c_int, b: c_int) -> c_int {
    a + b
}

[[export("C", link_name: "mylib_init")]]
public procedure initialize(config: *imm Config) -> c_int { 0 }
```

**Static Semantics**

**Visibility Requirement**

Exported procedures MUST be `public`.

**Signature Restrictions**

1. All parameter types MUST be `FfiSafe`
2. Return type MUST be `FfiSafe` (or unit `()`)
3. Parameters MUST NOT be dynamic class object types (`$Class`) or capability classes
4. Parameters MUST NOT contain `Context` or types containing dynamic class objects (`$Class`)

**Symbol Name**

By default, the exported symbol name matches the procedure name. The `link_name` option overrides this.

**No Mangling**

Exported procedures use unmangled names suitable for C linkage.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-SYS-3352`–`E-SYS-3354`, `E-SYS-3365`, `E-SYS-3366`.


#### 21.2.4 Link Attributes (`[[symbol]]`, `[[library]]`, `[[no_mangle]]`)

**Link Attributes.** Attributes controlling symbol naming and library linkage for FFI declarations.

**Syntax**

```cursive
extern "C" {
    [[symbol("__real_malloc")]]
    procedure malloc(size: usize) -> *mut opaque c_void;
}
```

**Static Semantics**

**`[[symbol]]`**

Overrides the linker symbol name for an `extern` or exported procedure. See §7.5.1 for complete syntax and constraints.

**`[[library]]`**

Specifies library linkage for an `extern` block. See §7.5.2 for complete syntax and the set of valid link kinds.

**`[[no_mangle]]`**

Disables name mangling. Implicit for `extern "C"` declarations. See §7.5.3 for complete semantics.


### 21.3 Capability Isolation


#### 21.3.1 FFI and No Ambient Authority

**Capability Isolation.** The property ensuring that FFI interactions do not grant foreign code unintended authority over Cursive capabilities.

**Static Semantics**

**Capability Blindness Rule**

Foreign code MUST NOT:

- Receive or return capability dynamic class objects (`$Class`)
- Inspect capability representations
- Exercise capability-mediated authority

This is enforced by prohibiting capability-bearing dynamic class objects (`$Class`) in all FFI signatures.

**Region Pointer Escape Detection**

Pointers to region-local storage MUST NOT be passed across the FFI boundary. Escape analysis MUST reject any flow of a region-local pointer into foreign code.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-SYS-3360`, `E-SYS-3367`.

For `E-SYS-3353` (Context or dynamic-class-containing type in export), see §21.2.3.


#### 21.3.2 Capability Wrapping Patterns

**Capability Wrapping Patterns.** Safe patterns for exposing Cursive resources to foreign code without violating capability isolation.

**Static Semantics**

**Runtime Handle Pattern**

When foreign code needs to reference Cursive resources:

1. Expose opaque, FfiSafe handle types whose fields do not contain capabilities
2. Handle registration, lookup, and revocation MUST be performed inside Cursive code
3. Invalid handles MUST be diagnosed or signaled as errors

**Callback Context Pattern**

When foreign code calls back into Cursive:

1. Capability-bearing data MUST be wrapped in a heap-allocated context owned by Cursive
2. Foreign code receives only an opaque pointer
3. Destruction of the context MUST be explicit and exactly once


### 21.4 Foreign Contracts


#### 21.4.1 Foreign Preconditions

**Foreign Preconditions.** Conditions that callers must satisfy before invoking foreign procedures, specified using the `@foreign_assumes` clause.

**Syntax**

```ebnf
ffi_verification_attr ::= "[[" ffi_verification_mode "]]"
ffi_verification_mode ::= "static" | "dynamic" | "assume" | "trust"

foreign_contract ::= "|=" "@foreign_assumes" "(" predicate_list ")"

predicate_list   ::= predicate ("," predicate)*

predicate        ::= comparison_expr | null_check | range_check

null_check       ::= expression "!=" null_literal | expression "==" null_literal

range_check      ::= expression "in" range_expression
```

```cursive
extern "C" {
    procedure write(fd: c_int, buf: *imm u8, count: usize) -> isize
        |= @foreign_assumes(fd >= 0, buf != null, count > 0);
}
```

**Static Semantics**

**Predicate Context**

Predicates may reference:

- Parameter names from the procedure signature
- Literal constants
- Pure functions and operators
- Fields of parameter values (for record types)

Predicates MUST NOT reference:

- Global mutable state
- Values not in scope at the call site
- Effectful operations

**Verification Modes**

| Mode                   | Behavior                                                   |
| :--------------------- | :--------------------------------------------------------- |
| `[[static]]` (default) | Caller must prove predicates at compile time               |
| `[[dynamic]]`          | Runtime checks inserted before `unsafe` call               |
| `[[assume]]`           | Predicates assumed true without checks (optimization only) |

**Constraints**

**Diagnostics:** See Appendix A, codes `E-SEM-2850`–`E-SEM-2852`, `P-SEM-2860`.


#### 21.4.2 Foreign Postconditions

**Foreign Postconditions.** Conditions that foreign code guarantees upon successful return, specified using the `@foreign_ensures` clause. These are trusted assertions about foreign behavior.

**Syntax**

```ebnf
foreign_ensures ::= "@foreign_ensures" "(" ensures_predicate_list ")"

ensures_predicate_list ::= ensures_predicate ("," ensures_predicate)*

ensures_predicate ::= predicate
                    | "@error" ":" predicate
                    | "@null_result" ":" predicate
```

```cursive
extern "C" {
    procedure malloc(size: usize) -> *mut opaque c_void
        |= @foreign_assumes(size > 0)
        |= @foreign_ensures(@result != null implies aligned(@result, 16));

    procedure read(fd: c_int, buf: *mut opaque c_void, count: usize) -> isize
        |= @foreign_assumes(fd >= 0, buf != null)
        |= @foreign_ensures(@result >= -1, @result <= count as isize);
}
```

**Static Semantics**

**Predicate Bindings**

Postcondition predicates may reference:

- `@result`: The return value of the foreign procedure
- Parameter names (for checking output parameters)
- `@error`: Predicates that hold when the call fails
- `@null_result`: Predicates that hold when result is null

**Verification Modes**

| Mode                   | Behavior                                                      |
| :--------------------- | :------------------------------------------------------------ |
| `[[static]]` (default) | Postconditions available as assumptions for downstream proofs |
| `[[dynamic]]`          | Runtime assertions after foreign call returns                 |
| `[[assume]]`           | Postconditions assumed without checks (optimization only)     |
| `[[trust]]`            | Postconditions trusted without runtime checks (audited code)  |

**Constraints**

**Diagnostics:** See Appendix A, codes `E-SEM-2853`–`E-SEM-2855`, `P-SEM-2861`.


#### 21.4.3 Trust Boundaries

**Trust Boundaries.** Verification behavior definitions for foreign contracts, controlling the trade-off between safety guarantees and performance.

**Syntax**

```cursive
[[trust]]
extern "C" {
    procedure optimized_memcpy(dest: *mut u8, src: *imm u8, n: usize) -> *mut u8
        |= @foreign_assumes(dest != null, src != null)
        |= @foreign_ensures(@result == dest);
}
```

**Static Semantics**

**Trust Annotation**

The `[[trust]]` attribute on an extern block suppresses runtime checks for all contracts within that block. Postconditions are assumed true without verification.

**Safety Implications**

Incorrect postconditions under `[[trust]]` constitute Unverifiable Behavior (UVB). The programmer asserts that the foreign code satisfies declared contracts.

**Verification Hierarchy**

| Level         | Precondition Check | Postcondition Check      |
| :------------ | :----------------- | :----------------------- |
| `[[static]]`  | Compile-time proof | Available as assumptions |
| `[[dynamic]]` | Runtime assertion  | Runtime assertion        |
| `[[assume]]`  | No check           | No check                 |
| `[[trust]]`   | No check           | No check (trusted)       |

---

## 22. ABI and Linkage

---

### 22.1 Calling Conventions


#### 22.1.1 Platform Default Conventions

**Calling Convention.** A protocol for procedure invocation defining parameter passing, return value handling, register usage, and stack management. Cursive's calling conventions are platform-specific and follow the target platform's native ABI.

**Static Semantics**

**Supported Platform ABIs**

| Platform       | Default Convention | Reference                                              |
| :------------- | :----------------- | :----------------------------------------------------- |
| Linux x86-64   | System V AMD64     | System V Application Binary Interface AMD64 Supplement |
| Windows x86-64 | Microsoft x64      | Microsoft x64 Calling Convention                       |
| Linux ARM64    | AAPCS64            | ARM Architecture Procedure Call Standard (64-bit)      |
| Windows ARM64  | ARM64 Windows      | ARM64 Windows ABI                                      |
| Linux x86-32   | cdecl              | System V i386 ABI                                      |
| Windows x86-32 | stdcall            | Microsoft Win32 Calling Convention                     |

The default calling convention for a target platform is Implementation-Defined (IDB).


#### 22.1.2 Explicit Annotations (`"C"`, `"C-unwind"`, `"system"`, `"stdcall"`, etc.)

**Explicit Convention Annotations.** Annotations that override the default calling convention for foreign procedure declarations.

**Syntax**
*[REF: `extern_block` and `abi_string` are defined in §21.2.1.]*

```cursive
extern "C" {
    procedure printf(format: *imm i8, ...) -> i32;
}
```

**Static Semantics**

| Convention     | Semantics                                                                    |
| :------------- | :--------------------------------------------------------------------------- |
| `"C"`          | Standard C calling convention for the target platform (mandatory)            |
| `"C-unwind"`   | C calling convention with unwinding support                                  |
| `"system"`     | Platform system call convention (equals `"C"` on Unix, `"stdcall"` on Win32) |
| `"stdcall"`    | Windows standard call (callee cleans stack on 32-bit)                        |
| `"fastcall"`   | Fast calling convention (first args in registers)                            |
| `"vectorcall"` | SIMD-optimized convention (Windows)                                          |

All implementations MUST support the `"C"` calling convention. Support for other conventions is Implementation-Defined (IDB).

**Constraints**

**Diagnostics:** See Appendix A, codes `E-SYS-2201`, `E-SYS-2202`.


#### 22.1.3 Parameter Passing Rules

**Parameter Passing Rules.** Specifications for how procedure arguments are transmitted between caller and callee.

**Static Semantics**

**System V AMD64 (Linux/Unix x86-64)**

| Parameter Class | Registers                  | Stack Order   |
| :-------------- | :------------------------- | :------------ |
| Integer/pointer | RDI, RSI, RDX, RCX, R8, R9 | Right-to-left |
| Floating-point  | XMM0–XMM7                  | Right-to-left |
| Overflow        | Stack                      | Right-to-left |

**Microsoft x64 (Windows x86-64)**

| Parameter Class | Registers         | Stack Order   |
| :-------------- | :---------------- | :------------ |
| Integer/pointer | RCX, RDX, R8, R9  | Right-to-left |
| Floating-point  | XMM0–XMM3         | Right-to-left |
| Shadow space    | 32 bytes reserved | —             |
| Overflow        | Stack             | Right-to-left |

**ARM64 (AAPCS64)**

| Parameter Class | Registers | Stack Order |
| :-------------- | :-------- | :---------- |
| Integer/pointer | X0–X7     | —           |
| Floating-point  | V0–V7     | —           |
| Overflow        | Stack     | —           |

Implementations MUST follow platform ABI specifications for parameter classification and register allocation.


#### 22.1.4 Return Value Handling

**Return Value Handling.** Specifications for how procedure results are communicated from callee to caller.

**Static Semantics**

**Return Value Conventions**

| Return Type         | System V AMD64       | Microsoft x64        | ARM64               |
| :------------------ | :------------------- | :------------------- | :------------------ |
| Integer/pointer     | RAX                  | RAX                  | X0                  |
| Floating-point      | XMM0                 | XMM0                 | V0                  |
| Large structures    | Hidden pointer (RDI) | Hidden pointer (RCX) | Hidden pointer (X8) |
| Pair (int, int)     | RAX, RDX             | RAX (packed)         | X0, X1              |
| Pair (float, float) | XMM0, XMM1           | XMM0 (packed)        | V0                  |

**Large Structure Threshold**

A structure is considered "large" and returned via hidden pointer when:
- System V AMD64: Size > 16 bytes or not classifiable as INTEGER/SSE
- Microsoft x64: Size not in {1, 2, 4, 8} bytes
- ARM64: Size > 16 bytes

The exact threshold is Implementation-Defined (IDB).


### 22.2 Layout Rules and Alignment

*[REF: Type layout semantics defined in §11.1 (Primitive Types) and §7.2 (Layout Attributes).]*


#### 22.2.1 Primitive Type Sizes and Alignment

**Primitive Type ABI.** *[REF: Normative size and alignment definitions for primitive types are in §11.1. This section provides C ABI equivalents for FFI interoperability.]*

**Static Semantics**

**C ABI Type Equivalents**

| Cursive Type | C Equivalent        |
| :----------- | :------------------ |
| `i8`         | `int8_t`            |
| `u8`         | `uint8_t`           |
| `bool`       | `uint8_t`           |
| `i16`        | `int16_t`           |
| `u16`        | `uint16_t`          |
| `f16`        | `_Float16`          |
| `i32`        | `int32_t`           |
| `u32`        | `uint32_t`          |
| `f32`        | `float`             |
| `char`       | `uint32_t`          |
| `i64`        | `int64_t`           |
| `u64`        | `uint64_t`          |
| `f64`        | `double`            |
| `i128`       | `__int128`          |
| `u128`       | `unsigned __int128` |
| `isize`      | `intptr_t`          |
| `usize`      | `uintptr_t`         |


#### 22.2.2 Record Layout Algorithm

**Record Layout Algorithm.** The algorithm determining field offsets, padding, and total size for record types.

**Static Semantics**

**Default Layout**

For records without `[[layout(C)]]`:
1. Field ordering is Unspecified Behavior (USB)
2. Padding insertion is Implementation-Defined (IDB)
3. The implementation MAY reorder fields to minimize padding

**C-Compatible Layout**

For records marked `[[layout(C)]]`:

$$\text{offset}_0 = 0$$

$$\text{offset}_i = \lceil (\text{offset}_{i-1} + \text{sizeof}(T_{i-1})) / \text{alignof}(T_i) \rceil \times \text{alignof}(T_i)$$

$$\text{sizeof}(R) = \lceil (\text{offset}_n + \text{sizeof}(T_n)) / \text{alignof}(R) \rceil \times \text{alignof}(R)$$

$$\text{alignof}(R) = \max_{i \in 1..n}(\text{alignof}(T_i))$$

Fields MUST be laid out in declaration order. Padding MUST be inserted only as required to satisfy alignment constraints.


#### 22.2.3 Enum Representation

**Enum Representation.** The memory layout definition for enumerated types.

**Static Semantics**

**Discriminant Type**

The discriminant (tag) type for an enum is determined by:

1. If `[[layout(IntType)]]` is specified, use `IntType`
2. Otherwise, the discriminant type is Implementation-Defined (IDB)

**Layout with `[[layout(C)]]`**

For enums marked `[[layout(C)]]`:
1. The discriminant MUST be represented as a C-compatible integer tag
2. The enum MUST be laid out as a tagged union per the target C ABI
3. The layout follows the platform's union alignment rules

**Size Calculation**

$$\text{sizeof}(\text{Enum}) = \text{sizeof}(\text{discriminant}) + \max_{v \in \text{variants}}(\text{sizeof}(v.\text{payload})) + \text{padding}$$

**Constraints**

**Diagnostics:** See Appendix A, code `E-SYS-2210`.


#### 22.2.4 Union Layout

**Union Layout.** The memory organization for Cursive's structural union types (`T1 | T2 | ... | Tn`).

**Static Semantics**

**Untagged Union Layout**

Cursive union types are *untagged* at the memory level. The runtime representation uses type-based discrimination rather than explicit tags.

**Size Calculation**

$$\text{sizeof}(T_1 \mid T_2 \mid \ldots \mid T_n) = \max_{i \in 1..n}(\text{sizeof}(T_i))$$

**Alignment Calculation**

$$\text{alignof}(T_1 \mid T_2 \mid \ldots \mid T_n) = \max_{i \in 1..n}(\text{alignof}(T_i))$$

**Padding**

Tail padding is inserted to ensure the total size is a multiple of the union's alignment:

$$\text{sizeof}_{\text{padded}} = \lceil \text{sizeof} / \text{alignof} \rceil \times \text{alignof}$$

**Niche Optimization**

When one or more member types have unused bit patterns (niches), the implementation MAY use niche optimization to reduce union size:

| Member Types                    | Niche Source                    | Optimization           |
| :------------------------------ | :------------------------------ | :--------------------- |
| `T \| ()`                       | Unit type has no representation | sizeof = sizeof(T)     |
| `Ptr<T>@Valid \| Ptr<T>@Null`   | Null pointer pattern            | sizeof = sizeof(usize) |
| `bool \| T` where sizeof(T) > 1 | Unused bool values (2-255)      | Implementation-defined |

Niche optimization is Implementation-Defined Behavior (IDB). Implementations MUST document which niche optimizations are applied.

**Discriminant Storage**

For unions where niche optimization does not apply and runtime type discrimination is required:

1. The implementation MAY prepend a hidden discriminant field
2. The discriminant type is the smallest unsigned integer sufficient to distinguish all members
3. Discriminant values are assigned in member declaration order (0, 1, 2, ...)

**Dynamic Semantics**

**Construction**

When a value of type `Ti` is stored in a union `T1 | ... | Tn`:

1. The value is stored at offset 0 within the union's storage
2. If a discriminant is present, it is set to the index of `Ti`
3. Remaining bytes (if any) are padding and have unspecified contents

**Access**

Union access via pattern matching:

1. If niche-optimized, inspect the value's bit pattern to determine the active type
2. If discriminant-tagged, read the discriminant to determine the active type
3. Interpret the stored bytes as the determined type

**Constraints**

**Diagnostics:** See Appendix A, codes `E-SYS-2215`, `W-SYS-2210`.


#### 22.2.5 Padding and Packing

**Padding and Packing.** Rules controlling inter-field spacing in composite types.

**Static Semantics**

**Padding Rules**

Padding bytes are inserted to satisfy alignment requirements:
1. Inter-field padding aligns each field to its natural alignment
2. Tail padding ensures the total size is a multiple of the record's alignment

**Packed Layout**

For records marked `[[layout(packed)]]`:
1. All inter-field padding is removed
2. Each field has effective alignment 1
3. The record's overall alignment becomes 1

**Packed Field Access**

| Operation                          | Safety Classification |
| :--------------------------------- | :-------------------- |
| Direct read (`packed.field`)       | Safe                  |
| Direct write (`packed.field = v`)  | Safe                  |
| Reference-taking (`&packed.field`) | UVB                   |
| Method call (`packed.field~>m()`)  | UVB                   |

Reference-taking on packed fields MUST occur within an `unsafe` block.


### 22.3 Linkage and Symbol Visibility


#### 22.3.1 Internal vs External Linkage

**Linkage.** The property determining whether a symbol is visible to other compilation units during linking.

**Static Semantics**

| Linkage  | Visibility                               | Declaration Context                  |
| :------- | :--------------------------------------- | :----------------------------------- |
| Internal | Visible only within the compilation unit | Private procedures, internal modules |
| External | Visible to other compilation units       | Public procedures, exported symbols  |

Procedures marked `public` in a library assembly have external linkage by default. Procedures without visibility modifiers have internal linkage.


#### 22.3.2 Symbol Visibility Levels

**Symbol Visibility Levels.** Controls for the visibility of externally linked symbols in shared libraries.

**Static Semantics**

| Level     | Description                                      |
| :-------- | :----------------------------------------------- |
| Default   | Symbol is visible to all modules                 |
| Hidden    | Symbol is not visible outside the shared library |
| Protected | Symbol is visible but cannot be overridden       |

The mechanism for specifying symbol visibility is Implementation-Defined (IDB).




---

# Part VIII: Appendices

---


## Appendix A: Diagnostic Code Registry

This appendix consolidates all diagnostic codes defined throughout the specification. Each code appears in exactly one location in the main specification text; this appendix serves as a lookup reference.

### A.1 Error Codes

#### A.1.1 E-CNF (Conformance)

| Code         | Severity | Detection    | Condition                                                                                   | Source |
| ------------ | -------- | ------------ | ------------------------------------------------------------------------------------------- | ------ |
| `E-CNF-0101` | Error    | Compile-time | Program is ill-formed due to syntactic or static-semantic violation                         | §1.1.3 |
| `E-CNF-0401` | Error    | Compile-time | Reserved keyword used as identifier                                                         | §2.5.2 |
| `E-CNF-0402` | Error    | Compile-time | Reserved namespace `cursive.*` used by user code                                            | §2.5.3 |
| `E-CNF-0403` | Error    | Compile-time | Primitive type name shadowed at module scope                                                | §2.5.4 |
| `E-CNF-0404` | Error    | Compile-time | Shadowing of `Self`, `Drop`, `Bitcopy`, `Clone`, `string`, `bytes`, or `Modal`                                           | §2.5.4 |
| `E-CNF-0405` | Error    | Compile-time | Shadowing of async type alias (`Async`, `Future`, `Sequence`, `Stream`, `Pipe`, `Exchange`) | §2.5.4 |
| `E-CNF-0406` | Error    | Compile-time | User declaration uses `gen_` prefix                                                         | §2.5.3 |

#### A.1.2 E-SRC (Source)

| Code         | Severity | Detection    | Condition                                            | Source |
| ------------ | -------- | ------------ | ---------------------------------------------------- | ------ |
| `E-SRC-0101` | Error    | Compile-time | Invalid UTF-8 byte sequence                          | §2.1.1 |
| `E-SRC-0103` | Error    | Compile-time | Embedded BOM found after the first position          | §2.1.2 |
| `E-SRC-0104` | Error    | Compile-time | Forbidden control character or null byte             | §2.1.4 |
| `E-SRC-0301` | Error    | Compile-time | Unterminated string literal                          | §2.6.3 |
| `E-SRC-0302` | Error    | Compile-time | Invalid escape sequence                              | §2.6.3 |
| `E-SRC-0303` | Error    | Compile-time | Invalid character literal                            | §2.6.4 |
| `E-SRC-0304` | Error    | Compile-time | Malformed numeric literal                            | §2.6.1 |
| `E-SRC-0306` | Error    | Compile-time | Unterminated block comment                           | §2.3.2 |
| `E-SRC-0307` | Error    | Compile-time | Invalid Unicode in identifier                        | §2.5.1 |
| `E-SRC-0308` | Error    | Compile-time | Lexically sensitive Unicode character outside `unsafe` block | §2.8.1 |
| `E-SRC-0309` | Error    | Compile-time | Tokenization failed to classify a character sequence | §2.4.3 |
| `E-SRC-0501` | Error    | Compile-time | Control-flow construct at module scope               | §5.1.3 |
| `E-SRC-0510` | Error    | Compile-time | Missing statement terminator                         | §2.2.3 |

#### A.1.3 E-MEM (Memory)

| Code         | Severity | Detection    | Condition                                                            | Source  |
| ------------ | -------- | ------------ | -------------------------------------------------------------------- | ------- |
| `E-MEM-1206` | Error    | Compile-time | Named region not found for allocation                                | §16.7.5 |
| `E-MEM-1207` | Error    | Compile-time | `frame` used with no active region in scope                          | §16.7.7 |
| `E-MEM-1208` | Error    | Compile-time | `r.frame` target is not in `Region@Active` state                     | §16.7.7 |
| `E-MEM-3001` | Error    | Compile-time | Read or move of a binding in Moved or PartiallyMoved state           | §3.4.6  |
| `E-MEM-3002` | Error    | Compile-time | Access to binding in Uninitialized state                             | §3.4.6  |
| `E-MEM-3003` | Error    | Compile-time | Reassignment of immutable binding                                    | §3.4.6  |
| `E-MEM-3004` | Error    | Compile-time | Partial move from binding without `unique` permission                | §3.4.6  |
| `E-MEM-3005` | Error    | Compile-time | Explicit call to `Drop::drop` method                                 | §3.5.5  |
| `E-MEM-3006` | Error    | Compile-time | Attempt to move from immovable binding (`:=`)                        | §3.4.6  |
| `E-MEM-3020` | Error    | Compile-time | Value with shorter-lived provenance escapes to longer-lived location | §3.2.6  |
| `E-MEM-3021` | Error    | Compile-time | Region allocation `^` outside region scope                           | §15.4.1 |
| `E-MEM-3030` | Error    | Compile-time | Unsafe operation outside block                                       | §3.7.5  |
| `E-MEM-3031` | Error    | Compile-time | Transmute size mismatch                                              | §3.7.5  |

#### A.1.4 E-TYP (Type System)

| Code         | Severity | Detection    | Condition                                                               | Source  |
| ------------ | -------- | ------------ | ----------------------------------------------------------------------- | ------- |
| `E-TYP-1501` | Error    | Compile-time | Type mismatch in expression or assignment                               | §9.1.3  |
| `E-TYP-1502` | Error    | Compile-time | No valid type derivable for expression                                  | §9.1.3  |
| `E-TYP-1503` | Error    | Compile-time | Operation not supported for operand type                                | §9.1.3  |
| `E-TYP-1505` | Error    | Compile-time | Missing required type annotation at module scope                        | §9.4.4  |
| `E-TYP-1510` | Error    | Compile-time | Source type is not a subtype of target type                             | §9.2.2  |
| `E-TYP-1511` | Error    | Compile-time | Implicit permission upgrade attempted                                   | §9.2.2  |
| `E-TYP-1512` | Error    | Compile-time | Coercion between incompatible sibling permissions                       | §9.2.2  |
| `E-TYP-1520` | Error    | Compile-time | Variance violation in generic type instantiation                        | §9.3    |
| `E-TYP-1521` | Error    | Compile-time | Invariant type parameter requires exact type match                      | §9.3    |
| `E-TYP-1530` | Error    | Compile-time | Type inference failed; unable to determine type                         | §9.4.4  |
| `E-TYP-1601` | Error    | Compile-time | Mutation through `const` path                                           | §10.3.1 |
| `E-TYP-1602` | Error    | Compile-time | `unique` exclusion violation (aliasing or inactive use)                 | §10.3.1 |
| `E-TYP-1604` | Error    | Compile-time | Direct field mutation through `shared` path without key                 | §10.3.1 |
| `E-TYP-1605` | Error    | Compile-time | Receiver permission incompatible with caller                            | §10.3.3 |
| `E-TYP-1710` | Error    | Compile-time | Integer literal out of range for target type                            | §11.1.1 |
| `E-TYP-1711` | Error    | Compile-time | Character literal is not a valid Unicode Scalar Value                   | §11.1.4 |
| `E-TYP-1712` | Error    | Compile-time | Implicit conversion between distinct primitive types                    | §11.1.1 |
| `E-TYP-1801` | Error    | Compile-time | Tuple index out of bounds                                               | §11.2.2 |
| `E-TYP-1802` | Error    | Compile-time | Tuple index is not a compile-time constant integer literal              | §11.2.2 |
| `E-TYP-1803` | Error    | Compile-time | Tuple arity mismatch in assignment or pattern                           | §11.2.2 |
| `E-TYP-1810` | Error    | Compile-time | Array length is not a compile-time constant                             | §11.3.1 |
| `E-TYP-1812` | Error    | Compile-time | Array index expression has non-`usize` type                             | §11.3.2 |
| `E-TYP-1813` | Error    | Compile-time | Array repeat literal requires `Bitcopy` or constant initializer            | §11.3.1 |
| `E-TYP-1814` | Error    | Compile-time | Array length mismatch in assignment or pattern                          | §11.3.1 |
| `E-TYP-1820` | Error    | Compile-time | Slice index expression has non-`usize` type                             | §11.4.3 |
| `E-TYP-1823` | Error    | Compile-time | Slice outlives viewed storage                                           | §11.4.2 |
| `E-TYP-1830` | Error    | Compile-time | Range bound types do not match                                          | §11.5.2 |
| `E-TYP-1831` | Error    | Compile-time | Unbounded range used in context requiring finite iteration              | §11.5.3 |
| `E-TYP-1901` | Error    | Compile-time | Duplicate field name in record declaration                              | §11.6.1 |
| `E-TYP-1902` | Error    | Compile-time | Missing field initializer in record literal                             | §11.6.3 |
| `E-TYP-1903` | Error    | Compile-time | Duplicate field initializer in record literal                           | §11.6.3 |
| `E-TYP-1904` | Error    | Compile-time | Access to nonexistent field                                             | §11.6.2 |
| `E-TYP-1905` | Error    | Compile-time | Access to field not visible in current scope                            | §11.6.2 |
| `E-TYP-1906` | Error    | Compile-time | Field visibility exceeds record visibility                              | §11.6.1 |
| `E-TYP-1907` | Error    | Compile-time | Non-`Bitcopy` field requires move source expression                        | §11.6.4 |
| `E-TYP-1908` | Error    | Compile-time | Private field not visible for implicit copy in update                   | §11.6.4 |
| `E-TYP-1911` | Error    | Compile-time | Default record construction requires default initializer for every field | §11.6.5 |
| `E-TYP-1953` | Error    | Compile-time | Refinement not provable outside `[[dynamic]]` scope                     | §7.4.3  |
| `E-TYP-1954` | Error    | Compile-time | Impure expression in refinement predicate                               | §14.2.1 |
| `E-TYP-1955` | Error    | Compile-time | Predicate does not evaluate to `bool`                                   | §14.2.1 |
| `E-TYP-1956` | Error    | Compile-time | `self` used in inline parameter constraint                              | §14.2.1 |
| `E-TYP-1957` | Error    | Compile-time | Circular type dependency in refinement predicate                        | §14.2.1 |
| `E-TYP-2001` | Error    | Compile-time | Enum declaration contains no variants                                   | §11.7.1 |
| `E-TYP-2002` | Error    | Compile-time | Duplicate variant name in enum declaration                              | §11.7.1 |
| `E-TYP-2003` | Error    | Compile-time | Duplicate discriminant value in enum declaration                        | §11.7.3 |
| `E-TYP-2004` | Error    | Compile-time | Discriminant value is not a compile-time constant                       | §11.7.3 |
| `E-TYP-2005` | Error    | Compile-time | Direct field access on enum value without pattern matching              | §11.7.4 |
| `E-TYP-2006` | Error    | Compile-time | Infinite type: recursive enum without indirection                       | §11.7.1 |
| `E-TYP-2007` | Error    | Compile-time | Unknown variant name in enum construction                               | §11.7.2 |
| `E-TYP-2008` | Error    | Compile-time | Variant payload arity mismatch                                          | §11.7.2 |
| `E-TYP-2009` | Error    | Compile-time | Missing field initializer in record-like variant                        | §11.7.2 |
| `E-TYP-2010` | Error    | Compile-time | Discriminant overflow during implicit assignment                        | §11.7.3 |
| `E-TYP-2050` | Error    | Compile-time | Modal type declares zero states                                         | §12.1.1 |
| `E-TYP-2051` | Error    | Compile-time | Duplicate state name within modal type                                  | §12.1.1 |
| `E-TYP-2052` | Error    | Compile-time | Field access for field not present in current state's payload           | §12.1.3 |
| `E-TYP-2053` | Error    | Compile-time | Method invocation for method not available in current state             | §12.1.5 |
| `E-TYP-2054` | Error    | Compile-time | State name collides with modal type name                                | §12.1.1 |
| `E-TYP-2055` | Error    | Compile-time | Transition body returns value not matching declared target state        | §12.1.4 |
| `E-TYP-2056` | Error    | Compile-time | Transition invoked on value not of declared source state                | §12.1.4 |
| `E-TYP-2057` | Error    | Compile-time | Direct field access on general modal type without pattern matching      | §12.1.3 |
| `E-TYP-2060` | Error    | Compile-time | Non-exhaustive match on general modal type                              | §12.1.6 |
| `E-TYP-2070` | Error    | Compile-time | Implicit widening on non-niche-layout-compatible type                   | §12.1.7 |
| `E-TYP-2071` | Error    | Compile-time | `widen` applied to non-modal type                                       | §12.1.7 |
| `E-TYP-2072` | Error    | Compile-time | `widen` applied to already-general modal type                           | §12.1.7 |
| `E-TYP-2073` | Error    | Compile-time | Record literal whose type is `File@S` or `DirIter@S` for any state `S` in the corresponding modal type | §D.3.1 |
| `E-TYP-2101` | Error    | Compile-time | Dereference of pointer in `@Null` state                                 | §12.3.2 |
| `E-TYP-2102` | Error    | Compile-time | Dereference of pointer in `@Expired` state                              | §12.3.2 |
| `E-TYP-2103` | Error    | Compile-time | Dereference of raw pointer outside `unsafe`                             | §12.3.2 |
| `E-TYP-2104` | Error    | Compile-time | Address-of applied to non-place expression                              | §12.3.3 |
| `E-TYP-2105` | Error    | Compile-time | Cast of non-`@Valid` pointer to raw pointer                             | §12.3.4 |
| `E-TYP-2106` | Error    | Compile-time | Modal pointer type in `extern` signature                                | §12.3.4 |
| `E-TYP-2151` | Error    | Runtime      | Slice boundary not on UTF-8 char boundary                               | §12.2.4 |
| `E-TYP-2152` | Error    | Compile-time | Direct byte indexing on string type                                     | §12.2.4 |
| `E-TYP-2153` | Error    | Runtime      | Slice end index less than start index                                   | §12.2.4 |
| `E-TYP-2154` | Error    | Runtime      | Slice index exceeds string length                                       | §12.2.4 |
| `E-TYP-2201` | Error    | Compile-time | Union type has fewer than two member types                              | §11.8.1 |
| `E-TYP-2202` | Error    | Compile-time | Direct access on union value without pattern matching                   | §11.8.2 |
| `E-TYP-2203` | Error    | Compile-time | Infinite type: recursive union without indirection                      | §11.8.2 |
| `E-TYP-2204` | Error    | Compile-time | Union type used in `[[layout(C)]]` context                              | §11.8.4 |
| `E-TYP-2210` | Error    | Compile-time | `?` applied to non-union type                                           | §11.8.3 |
| `E-TYP-2211` | Error    | Compile-time | Ambiguous success type in `?` expression                                | §11.8.3 |
| `E-TYP-2212` | Error    | Compile-time | Union member not propagation-compatible with return type                | §11.8.3 |
| `E-TYP-2213` | Error    | Compile-time | `?` used outside procedure body                                         | §11.8.3 |
| `E-TYP-2214` | Error    | Compile-time | No success type candidate (all types propagate)                         | §11.8.3 |
| `E-TYP-2220` | Error    | Compile-time | Argument count mismatch                                                 | §12.4.1 |
| `E-TYP-2221` | Error    | Compile-time | Type mismatch in function argument/return                               | §12.4.1 |
| `E-TYP-2223` | Error    | Compile-time | Closure type in `extern` signature                                      | §12.4.1 |
| `E-TYP-2224` | Error    | Compile-time | Closure value assigned to sparse pointer type                           | §12.4.1 |
| `E-TYP-2225` | Error    | Compile-time | `move` modifier on return type                                          | §12.4.1 |
| `E-TYP-2226` | Error    | Compile-time | Function type has invalid structure                                     | §12.4.1 |
| `E-TYP-2301` | Error    | Compile-time | Type arguments cannot be inferred; explicit instantiation required      | §13.1.2 |
| `E-TYP-2302` | Error    | Compile-time | Type argument does not satisfy class bound                              | §13.2.1 |
| `E-TYP-2303` | Error    | Compile-time | Wrong number of type arguments                                          | §13.2.1 |
| `E-TYP-2304` | Error    | Compile-time | Duplicate type parameter name in generic declaration                    | §13.1.1 |
| `E-TYP-2305` | Error    | Compile-time | Class bound references a non-class type                                 | §13.2.1 |
| `E-TYP-2306` | Error    | Compile-time | Generic parameter in `extern` procedure signature                       | §13.1.2 |
| `E-TYP-2307` | Error    | Compile-time | Infinite monomorphization recursion                                     | §13.2.2 |
| `E-TYP-2308` | Error    | Compile-time | Monomorphization depth limit exceeded                                   | §13.2.2 |
| `E-TYP-2401` | Error    | Compile-time | Non-modal type implements modal class                                   | §13.4.1 |
| `E-TYP-2402` | Error    | Compile-time | Implementing type missing required field                                | §13.4.1 |
| `E-TYP-2403` | Error    | Compile-time | Implementing modal missing required state                               | §13.4.1 |
| `E-TYP-2404` | Error    | Compile-time | Implementing field has incompatible type                                | §13.4.1 |
| `E-TYP-2405` | Error    | Compile-time | Implementing state missing required payload field                       | §13.4.1 |
| `E-TYP-2406` | Error    | Compile-time | Conflicting field names from multiple classes                           | §13.4.1 |
| `E-TYP-2407` | Error    | Compile-time | Conflicting state names from multiple classes                           | §13.4.1 |
| `E-TYP-2408` | Error    | Compile-time | Duplicate abstract field name in class                                  | §13.3.1 |
| `E-TYP-2409` | Error    | Compile-time | Duplicate abstract state name in class                                  | §13.3.1 |
| `E-TYP-2500` | Error    | Compile-time | Duplicate procedure name in class                                       | §13.3.1 |
| `E-TYP-2501` | Error    | Compile-time | `override` used on abstract procedure implementation                    | §13.4.1 |
| `E-TYP-2502` | Error    | Compile-time | Missing `override` on concrete procedure replacement                    | §13.4.1 |
| `E-TYP-2503` | Error    | Compile-time | Type does not implement required procedure from class                   | §13.4.1 |
| `E-TYP-2504` | Error    | Compile-time | Duplicate associated type name in class                                 | §13.3.1 |
| `E-TYP-2505` | Error    | Compile-time | Name conflict among class members                                       | §13.3.1 |
| `E-TYP-2506` | Error    | Compile-time | Coherence violation: duplicate class implementation                     | §13.4.1 |
| `E-TYP-2507` | Error    | Compile-time | Orphan rule violation: neither type nor class is local                  | §13.4.1 |
| `E-TYP-2508` | Error    | Compile-time | Cyclic superclass dependency detected                                   | §13.3.1 |
| `E-TYP-2509` | Error    | Compile-time | Superclass bound refers to undefined class                              | §13.3.1 |
| `E-TYP-2510` | Error    | Compile-time | Accessing member not defined on opaque type's class                     | §13.6.1 |
| `E-TYP-2511` | Error    | Compile-time | Opaque return type does not implement required class                    | §13.6.1 |
| `E-TYP-2512` | Error    | Compile-time | Attempting to assign incompatible opaque types                          | §13.6.1 |
| `E-TYP-2530` | Error    | Compile-time | Type argument does not satisfy class constraint                         | §13.5.2 |
| `E-TYP-2531` | Error    | Compile-time | Unconstrained type parameter used in class method                       | §13.5.2 |
| `E-TYP-2540` | Error    | Compile-time | Procedure with `[[static_dispatch_only]]` called on `$`                 | §13.5.3 |
| `E-TYP-2541` | Error    | Compile-time | Dynamic class type created from non-dispatchable class                  | §13.5.3 |
| `E-TYP-2542` | Error    | Compile-time | Generic procedure in class without `[[static_dispatch_only]]` attribute | §13.5.3 |
| `E-TYP-2621` | Error    | Compile-time | Type implements both `Bitcopy` and `Drop`                                  | §14.6.1 |
| `E-TYP-2622` | Error    | Compile-time | `Bitcopy` type has non-`Bitcopy` field                                        | §14.7.1 |
| `E-TYP-2623` | Error    | Compile-time | Type implementing `Bitcopy` does not implement `Clone`                                    | §13.9.2 |

#### A.1.5 E-SEM (Semantic)

| Code         | Severity | Detection    | Condition                                                   | Source  |
| ------------ | -------- | ------------ | ----------------------------------------------------------- | ------- |
| `E-SEM-2511` | Error    | Compile-time | Identifier resolves to type or module, not value            | §15.2.1 |
| `E-SEM-2521` | Error    | Compile-time | Field access on non-record type                             | §15.2.2 |
| `E-SEM-2524` | Error    | Compile-time | Tuple access on non-tuple                                   | §15.2.3 |
| `E-SEM-2527` | Error    | Compile-time | Indexing applied to non-indexable type                      | §15.2.4 |
| `E-SEM-2528` | Error    | Compile-time | Index expression is not of type `usize`                     | §15.2.4 |
| `E-SEM-2531` | Error    | Compile-time | Callee expression is not of callable type                   | §15.3.1 |
| `E-SEM-2532` | Error    | Compile-time | Argument count mismatch                                     | §15.3.1 |
| `E-SEM-2533` | Error    | Compile-time | Argument type incompatible with parameter type              | §15.3.1 |
| `E-SEM-2534` | Error    | Compile-time | `move` argument required but not provided                   | §15.3.1 |
| `E-SEM-2535` | Error    | Compile-time | `move` argument provided but parameter is not `move`        | §15.3.1 |
| `E-SEM-2536` | Error    | Compile-time | Method not found for receiver type                          | §15.3.2 |
| `E-SEM-2537` | Error    | Compile-time | Method call using `.` instead of `~>`                       | §15.3.2 |
| `E-SEM-2538` | Error    | Compile-time | Right-hand side of pipeline is not callable                 | §15.3.3 |
| `E-SEM-2539` | Error    | Compile-time | Pipeline target has no parameters                           | §15.3.3 |
| `E-SEM-2541` | Error    | Compile-time | Logical `!` applied to non-bool, non-integer type           | §15.4.1 |
| `E-SEM-2542` | Error    | Compile-time | Numeric negation of unsigned integer                        | §15.4.1 |
| `E-SEM-2543` | Error    | Compile-time | Address-of `&` applied to value expression                  | §15.4.1 |
| `E-SEM-2544` | Error    | Compile-time | Dereference of pointer in `@Null` state                     | §15.4.1 |
| `E-SEM-2545` | Error    | Compile-time | Raw pointer dereference outside `unsafe` block              | §15.4.1 |
| `E-SEM-2551` | Error    | Compile-time | Arithmetic operator applied to non-numeric types            | §15.4.2 |
| `E-SEM-2552` | Error    | Compile-time | Mismatched operand types for arithmetic operator            | §15.4.2 |
| `E-SEM-2553` | Error    | Compile-time | Bitwise operator applied to non-integer types               | §15.4.2 |
| `E-SEM-2554` | Error    | Compile-time | Comparison of incompatible types                            | §15.4.2 |
| `E-SEM-2555` | Error    | Compile-time | Logical operator applied to non-bool operands               | §15.4.2 |
| `E-SEM-2556` | Error    | Compile-time | Shift amount is not of type `u32`                           | §15.4.2 |
| `E-SEM-2571` | Error    | Compile-time | Cast between incompatible types                             | §15.5.1 |
| `E-SEM-2572` | Error    | Compile-time | Pointer-integer cast outside `unsafe` block                 | §15.5.1 |
| `E-SEM-2573` | Error    | Compile-time | Enum cast without `[[layout(IntType)]]` attribute           | §15.5.1 |
| `E-SEM-2574` | Error    | Compile-time | Cast of non-`@Valid` pointer to raw pointer                 | §15.5.1 |
| `E-SEM-2591` | Error    | Compile-time | Closure parameter type cannot be inferred                   | §15.6.1 |
| `E-SEM-2593` | Error    | Compile-time | Capture of `unique` binding without `move`                  | §15.6.2 |
| `E-SEM-2594` | Error    | Compile-time | Closure return type mismatch                                | §15.6.1 |
| `E-SEM-2601` | Error    | Compile-time | `if` condition is not of type `bool`                        | §15.8.1 |
| `E-SEM-2602` | Error    | Compile-time | `if` branches have incompatible types                       | §15.8.1 |
| `E-SEM-2603` | Error    | Compile-time | `if` without `else` used in non-unit context                | §15.8.1 |
| `E-SEM-2611` | Error    | Compile-time | Match arms have incompatible types                          | §15.8.2 |
| `E-SEM-2612` | Error    | Compile-time | Match guard is not type bool                                | §15.7.5 |
| `E-SEM-2621` | Error    | Compile-time | Iterator expression does not implement `Iterator`           | §15.9.3 |
| `E-SEM-2622` | Error    | Compile-time | `break` values have incompatible types                      | §15.9.5 |
| `E-SEM-2705` | Error    | Compile-time | `match` expression is not exhaustive for union type         | §11.8.2 |
| `E-SEM-2711` | Error    | Compile-time | Refutable pattern in irrefutable context (`let`)            | §15.7   |
| `E-SEM-2712` | Error    | Compile-time | Pattern type incompatible with scrutinee type               | §15.7   |
| `E-SEM-2713` | Error    | Compile-time | Duplicate binding identifier within single pattern          | §15.7   |
| `E-SEM-2721` | Error    | Compile-time | Range pattern bounds are not compile-time constants         | §15.7.4 |
| `E-SEM-2722` | Error    | Compile-time | Range pattern start exceeds end (empty range)               | §15.7.4 |
| `E-SEM-2731` | Error    | Compile-time | Record pattern references non-existent field                | §15.7   |
| `E-SEM-2741` | Error    | Compile-time | Match expression is not exhaustive                          | §15.8.2 |
| `E-SEM-2751` | Error    | Compile-time | Match arm is unreachable                                    | §15.8.2 |
| `E-SEM-2801` | Error    | Compile-time | Contract predicate not provable outside `[[dynamic]]` scope | §7.4.3  |
| `E-SEM-2802` | Error    | Compile-time | Impure expression in contract predicate                     | §16.1.1 |
| `E-SEM-2803` | Error    | Compile-time | Implementation strengthens class precondition               | §16.5.1 |
| `E-SEM-2804` | Error    | Compile-time | Implementation weakens class postcondition                  | §16.5.1 |
| `E-SEM-2805` | Error    | Compile-time | `@entry()` result type not `Bitcopy` or `Clone`                | §16.4.1 |
| `E-SEM-2806` | Error    | Compile-time | `@result` used outside postcondition                        | §16.3.1 |
| `E-SEM-2820` | Error    | See §14.7    | Type invariant violated at construction                     | §14.5.1 |
| `E-SEM-2821` | Error    | See §14.7    | Type invariant violated at public entry                     | §14.5.1 |
| `E-SEM-2822` | Error    | See §14.7    | Type invariant violated at mutator return                   | §14.5.1 |
| `E-SEM-2823` | Error    | See §14.7    | Type invariant violated at private-to-public return         | §14.5.1 |
| `E-SEM-2824` | Error    | Compile-time | Public mutable field on type with invariant                 | §14.5.1 |
| `E-SEM-2830` | Error    | See §14.7    | Loop invariant not established at initialization            | §14.8.1 |
| `E-SEM-2831` | Error    | See §14.7    | Loop invariant not maintained across iteration              | §14.8.1 |
| `E-SEM-2850` | Error    | Compile-time | Cannot prove `@foreign_assumes` predicate                   | §21.3.1 |
| `E-SEM-2851` | Error    | Compile-time | Invalid predicate in foreign contract                       | §21.3.1 |
| `E-SEM-2852` | Error    | Compile-time | Predicate references out-of-scope value                     | §21.3.1 |
| `E-SEM-2853` | Error    | Compile-time | Invalid predicate in `@foreign_ensures`                     | §21.3.2 |
| `E-SEM-2854` | Error    | Compile-time | `@result` used in non-return context                        | §21.3.2 |
| `E-SEM-2855` | Error    | Compile-time | `@error` predicate on void-returning procedure              | §21.3.2 |
| `E-SEM-3001` | Error    | Compile-time | Duplicate parameter name in signature                       | §15.1.1 |
| `E-SEM-3002` | Error    | Compile-time | Return type mismatch                                        | §15.1.2 |
| `E-SEM-3003` | Error    | Compile-time | Non-exhaustive return paths                                 | §15.1.2 |
| `E-SEM-3004` | Error    | Compile-time | Impure expression in contract clause                        | §16.1.1 |
| `E-SEM-3011` | Error    | Compile-time | Method defined outside of type context                      | §15.2.1 |
| `E-SEM-3012` | Error    | Compile-time | Duplicate method name in type                               | §15.2.1 |
| `E-SEM-3022` | Error    | Compile-time | Argument type incompatible with parameter                   | §15.1.1 |
| `E-SEM-3030` | Error    | Compile-time | Ambiguous overload resolution                               | §15.3.1 |
| `E-SEM-3031` | Error    | Compile-time | No matching overload found                                  | §15.3.1 |
| `E-SEM-3032` | Error    | Compile-time | Duplicate signature (overloads must differ)                 | §15.3.1 |
| `E-SEM-3131` | Error    | Compile-time | Assignment target is not a place expression                 | §16.3.1 |
| `E-SEM-3132` | Error    | Compile-time | Assignment through `const` permission                       | §16.3.1 |
| `E-SEM-3133` | Error    | Compile-time | Assignment type mismatch                                    | §16.3.1 |
| `E-SEM-3151` | Error    | Compile-time | Defer block has non-unit type                               | §16.6.3 |
| `E-SEM-3152` | Error    | Compile-time | Non-local control flow in defer block                       | §16.6.3 |
| `E-SEM-3161` | Error    | Compile-time | `return` type mismatch with procedure                       | §16.5.4 |
| `E-SEM-3162` | Error    | Compile-time | `break` outside `loop`                                      | §16.5.4 |
| `E-SEM-3163` | Error    | Compile-time | `continue` outside `loop`                                   | §16.5.4 |
| `E-SEM-3164` | Error    | Compile-time | `result` type mismatch with block                           | §16.5.4 |
| `E-SEM-3165` | Error    | Compile-time | `return` at module scope                                    | §16.5.4 |
| `E-SEM-3166` | Error    | Compile-time | Unknown loop label                                          | §16.5.4 |

#### A.1.6 E-CON (Concurrency/Contract)

| Code         | Severity | Detection    | Condition                                                        | Source  |
| ------------ | -------- | ------------ | ---------------------------------------------------------------- | ------- |
| `E-CON-0001` | Error    | Compile-time | Access to `shared` path outside valid key context                | §17.2.3 |
| `E-CON-0002` | Error    | Compile-time | `#` annotation on non-`shared` path                              | §17.2.3 |
| `E-CON-0003` | Error    | Compile-time | Multiple `#` markers in single path expression                   | §17.2.3 |
| `E-CON-0004` | Error    | Compile-time | Key escapes its defining scope                                   | §17.2.3 |
| `E-CON-0005` | Error    | Compile-time | Write access required but only Read available                    | §17.2.1 |
| `E-CON-0006` | Error    | Compile-time | Key acquisition in `defer` escapes to outer scope                | §17.2.2 |
| `E-CON-0010` | Error    | Compile-time | Potential conflict on dynamic indices (same statement)           | §17.3.2 |
| `E-CON-0011` | Error    | Compile-time | Detectable key ordering cycle within procedure                   | §17.4.1 |
| `E-CON-0012` | Error    | Compile-time | Nested mode change without `release` modifier                    | §17.4.1 |
| `E-CON-0014` | Error    | Compile-time | `ordered` modifier on paths with different array bases           | §17.6.1 |
| `E-CON-0017` | Error    | Compile-time | `release` modifier without target mode                           | §17.4.1 |
| `E-CON-0018` | Error    | Compile-time | `release` with target mode matching outer mode                   | §17.4.1 |
| `E-CON-0020` | Error    | Compile-time | Key safety not statically provable outside `[[dynamic]]`         | §17.6.1 |
| `E-CON-0030` | Error    | Compile-time | `#` immediately before method name                               | §17.2.3 |
| `E-CON-0031` | Error    | Compile-time | `#` block path not in scope                                      | §17.2.3 |
| `E-CON-0032` | Error    | Compile-time | `#` block path is not `shared`                                   | §17.2.3 |
| `E-CON-0033` | Error    | Compile-time | `#` on field of non-record type                                  | §17.2.4 |
| `E-CON-0034` | Error    | Compile-time | Key path root cannot be derived for shared access                | §17.2.1 |
| `E-CON-0060` | Error    | Compile-time | Read-then-write on same `shared` path without covering Write key | §17.3.3 |
| `E-CON-0070` | Error    | Compile-time | Write operation in `#` block without `write` modifier            | §17.2.3 |
| `E-CON-0083` | Error    | Compile-time | `shared $Class` where class has `~%`/`~!` methods                | §17.2.6 |
| `E-CON-0085` | Error    | Compile-time | Escaping closure with `shared` capture lacks dependency set      | §17.2.5 |
| `E-CON-0086` | Error    | Compile-time | Escaping closure outlives captured `shared` binding              | §17.2.5 |
| `E-CON-0090` | Error    | Compile-time | Nested key block inside speculative block                        | §17.5.1 |
| `E-CON-0091` | Error    | Compile-time | Write to path outside keyed set in speculative block             | §17.5.1 |
| `E-CON-0092` | Error    | Compile-time | `wait` expression inside speculative block                       | §17.5.1 |
| `E-CON-0093` | Error    | Compile-time | `defer` statement inside speculative block                       | §17.5.1 |
| `E-CON-0094` | Error    | Compile-time | `speculative` combined with `release`                            | §17.5.1 |
| `E-CON-0095` | Error    | Compile-time | `speculative` without `write` modifier                           | §17.5.1 |
| `E-CON-0096` | Error    | Compile-time | Memory ordering annotation inside speculative block              | §17.5.1 |
| `E-CON-0101` | Error    | Compile-time | `spawn` or `dispatch` outside parallel                           | §18.2.1 |
| `E-CON-0102` | Error    | Compile-time | Domain expression not ExecutionDomain                            | §18.2.1 |
| `E-CON-0103` | Error    | Compile-time | Invalid domain parameter type                                    | §18.2.1 |
| `E-CON-0120` | Error    | Compile-time | Implicit capture of `unique` binding                             | §18.3.1 |
| `E-CON-0121` | Error    | Compile-time | Move of already-moved binding                                    | §18.3.1 |
| `E-CON-0122` | Error    | Compile-time | Move of binding from outer parallel scope                        | §18.3.1 |
| `E-CON-0130` | Error    | Compile-time | Invalid spawn attribute type                                     | §18.3.2 |
| `E-CON-0131` | Error    | Compile-time | `spawn` in escaping closure                                      | §18.3.2 |
| `E-CON-0140` | Error    | Compile-time | Dispatch outside parallel block                                  | §18.4.1 |
| `E-CON-0141` | Error    | Compile-time | Key inference failed; explicit key required                      | §18.4.1 |
| `E-CON-0142` | Error    | Compile-time | Cross-iteration dependency detected                              | §18.4.1 |
| `E-CON-0143` | Error    | Compile-time | Non-associative reduction without `[ordered]`                    | §18.4.1 |
| `E-CON-0150` | Error    | Compile-time | Host memory access in GPU code                                   | §18.5.1 |
| `E-CON-0151` | Error    | Compile-time | `shared` capture in GPU dispatch                                 | §18.5.1 |
| `E-CON-0152` | Error    | Compile-time | Nested GPU parallel block                                        | §18.5.1 |
| `E-CON-0201` | Error    | Compile-time | `Async` type parameter is not well-formed                        | §19.1.1 |
| `E-CON-0202` | Error    | Compile-time | `yield` in non-async procedure                                   | §19.1.1 |
| `E-CON-0203` | Error    | Compile-time | `result` type mismatch with `Result` parameter                   | §19.1.1 |
| `E-CON-0210` | Error    | Compile-time | `yield` outside async-returning procedure                        | §19.2.2 |
| `E-CON-0211` | Error    | Compile-time | `yield` operand type does not match `Out`                        | §19.2.2 |
| `E-CON-0212` | Error    | Compile-time | `yield` inside `sync` expression                                 | §19.2.2 |
| `E-CON-0213` | Error    | Compile-time | `yield` while key is held (without `release`)                    | §19.2.2 |
| `E-CON-0220` | Error    | Compile-time | `yield from` outside async-returning procedure                   | §19.2.3 |
| `E-CON-0221` | Error    | Compile-time | Incompatible `Out` parameter in `yield from`                     | §19.2.3 |
| `E-CON-0222` | Error    | Compile-time | Incompatible `In` parameter in `yield from`                      | §19.2.3 |
| `E-CON-0223` | Error    | Compile-time | `yield from` inside `sync` expression                            | §19.2.3 |
| `E-CON-0224` | Error    | Compile-time | `yield from` while key is held (without `release`)               | §19.2.3 |
| `E-CON-0225` | Error    | Compile-time | Error type not compatible in `yield from`                        | §19.2.3 |
| `E-CON-0230` | Error    | Compile-time | Error propagation in infallible async procedure                  | §19.1.4 |
| `E-CON-0240` | Error    | Compile-time | Iteration over async with `In ≠ ()`                              | §19.3.1 |
| `E-CON-0250` | Error    | Compile-time | `sync` inside async-returning procedure                          | §19.3.3 |
| `E-CON-0251` | Error    | Compile-time | `sync` operand has `Out ≠ ()`                                    | §19.3.3 |
| `E-CON-0252` | Error    | Compile-time | `sync` operand has `In ≠ ()`                                     | §19.3.3 |
| `E-CON-0260` | Error    | Compile-time | `race` with fewer than 2 arms                                    | §19.3.4 |
| `E-CON-0261` | Error    | Compile-time | `race` arms have incompatible types                              | §19.3.4 |
| `E-CON-0262` | Error    | Compile-time | Non-streaming `race` operand has `Out ≠ ()`                      | §19.3.4 |
| `E-CON-0263` | Error    | Compile-time | Mixed yield/non-yield handlers in race                           | §19.3.4 |
| `E-CON-0270` | Error    | Compile-time | `all` operand has `Out ≠ ()`                                     | §19.3.5 |
| `E-CON-0271` | Error    | Compile-time | `all` operand has `In ≠ ()`                                      | §19.3.5 |
| `E-CON-0280` | Error    | Compile-time | Captured binding does not outlive async                          | §19.4.1 |
| `E-CON-0281` | Error    | Compile-time | Async operation escapes its region                               | §19.4.1 |
| `E-CON-0301` | Error    | Compile-time | Unsequenced modification and read of same binding                | §3.6.4  |
| `E-CON-0302` | Error    | Compile-time | Unsequenced modifications of same binding                        | §3.6.4  |
| `E-CON-0410` | Error    | Compile-time | `[[dynamic]]` applied to contract clause directly                | §7.4.3  |
| `E-CON-0411` | Error    | Compile-time | `[[dynamic]]` applied to type alias declaration                  | §7.4.3  |
| `E-CON-0412` | Error    | Compile-time | `[[dynamic]]` applied to field declaration                       | §7.4.3  |

#### A.1.7 E-MOD (Module)

| Code         | Severity | Detection    | Condition                                                       | Source  |
| ------------ | -------- | ------------ | --------------------------------------------------------------- | ------- |
| `E-MOD-1104` | Error    | Compile-time | Module path collision on case-insensitive filesystem            | §5.3.2  |
| `E-MOD-1105` | Error    | Compile-time | Module path component is a reserved keyword                     | §5.3.2  |
| `E-MOD-1106` | Error    | Compile-time | Module path component is not a valid identifier                 | §5.1.3  |
| `E-MOD-1201` | Error    | Compile-time | Circular import dependency between assemblies                   | §6.4.1  |
| `E-MOD-1202` | Error    | Compile-time | Import of non-existent assembly or module                       | §6.4.1  |
| `E-MOD-1203` | Error    | Compile-time | Name introduced by `using` or `import as` conflicts with existing | §6.4.4  |
| `E-MOD-1204` | Error    | Compile-time | Item in `using` path not found or not accessible                  | §6.4.2  |
| `E-MOD-1205` | Error    | Compile-time | Attempt to `public using` a non-public item                       | §6.4.3  |
| `E-MOD-1206` | Error    | Compile-time | Duplicate item in a `using` list                                  | §6.4.2  |
| `E-MOD-1207` | Error    | Compile-time | Cannot access a `protected` item from this scope                | §6.3.4  |
| `E-MOD-1301` | Error    | Compile-time | Unresolved name: identifier not found in any accessible scope   | §6.2.3  |
| `E-MOD-1302` | Error    | Compile-time | Duplicate declaration in module scope                           | §5.1.3  |
| `E-MOD-1303` | Error    | Compile-time | Shadowing without `shadow` keyword                              | §6.2.2  |
| `E-MOD-1304` | Error    | Compile-time | Unresolved module: path prefix did not resolve to a module      | §6.2.4  |
| `E-MOD-1305` | Error    | Compile-time | Inaccessible item: visibility does not permit access            | §6.3.2  |
| `E-MOD-1306` | Error    | Compile-time | Unnecessary `shadow` keyword: no binding is being shadowed      | §6.2.2  |
| `E-MOD-1307` | Error    | Compile-time | Ambiguous method resolution; disambiguation req                 | §15.3.2 |
| `E-MOD-1401` | Error    | Compile-time | Cyclic module dependency detected in eager initializers         | §8.2.4  |
| `E-MOD-2401` | Error    | Compile-time | Reassignment of immutable `let` binding                         | §16.2.2 |
| `E-MOD-2402` | Error    | Compile-time | Type annotation incompatible with inferred type                 | §16.2.2 |
| `E-MOD-2411` | Error    | Compile-time | Missing move expression at call site for transferring parameter | §3.4.6  |
| `E-MOD-2430` | Error    | Compile-time | Multiple `main` procedures defined                              | §8.3.1  |
| `E-MOD-2431` | Error    | Compile-time | Invalid `main` signature                                        | §4.2.1  |
| `E-MOD-2432` | Error    | Compile-time | `main` is generic (has type parameters)                         | §8.3.1  |
| `E-MOD-2433` | Error    | Compile-time | Public mutable global state                                     | §8.3.4  |
| `E-MOD-2440` | Error    | Compile-time | `protected` used on top-level declaration                       | §6.3.4  |
| `E-MOD-2450` | Error    | Compile-time | Malformed attribute syntax                                      | §7.1.1  |
| `E-MOD-2451` | Error    | Compile-time | Unknown attribute name                                          | §7.1.2  |
| `E-MOD-2452` | Error    | Compile-time | Attribute not valid on target declaration kind                  | §7.1.2  |
| `E-MOD-2453` | Error    | Compile-time | `align(N)` where N is not a power of two                        | §7.2.3  |
| `E-MOD-2454` | Error    | Compile-time | `packed` applied to non-record                                  | §7.2.2  |
| `E-MOD-2455` | Error    | Compile-time | Conflicting layout arguments                                    | §7.2.4  |
| `E-MOD-2460` | Error    | Compile-time | `[[reflect]]` on non-type declaration                           | §7.4.2  |
| `E-MOD-2461` | Error    | Compile-time | `reflect_type<T>()` where T lacks `[[reflect]]`                 | §7.4.2  |
| `E-MOD-2465` | Error    | Compile-time | `[[stale_ok]]` on non-binding declaration                       | §7.4.5  |

#### A.1.8 E-SYS (System/FFI)

| Code         | Severity | Detection    | Condition                                                           | Source  |
| ------------ | -------- | ------------ | ------------------------------------------------------------------- | ------- |
| `E-SYS-1001` | Error    | Compile-time | Ambient authority detected: global procedure performs side effects  | §4.1.1  |
| `E-SYS-1002` | Error    | Compile-time | Effect-producing procedure lacks required capability param          | §4.1.3  |
| `E-SYS-2201` | Error    | Compile-time | Unsupported calling convention                                      | §22.1.1 |
| `E-SYS-2202` | Error    | Compile-time | Calling convention invalid for platform                             | §22.1.1 |
| `E-SYS-2210` | Error    | Compile-time | Discriminant value exceeds specified type range                     | §22.2.1 |
| `E-SYS-2215` | Error    | Compile-time | Union member exceeds maximum type size                              | §22.2.2 |
| `E-SYS-3301` | Error    | Compile-time | `FfiSafe` on type without `[[layout(C)]]`                           | §21.1.1 |
| `E-SYS-3302` | Error    | Compile-time | `FfiSafe` type has non-FFI-safe field                               | §21.1.2 |
| `E-SYS-3303` | Error    | Compile-time | `FfiSafe` on prohibited type category                               | §21.1.1 |
| `E-SYS-3304` | Error    | Compile-time | Generic `FfiSafe` type with unconstrained parameter                 | §21.1.2 |
| `E-SYS-3305` | Error    | Compile-time | `FfiSafe` type has field with incomplete type                       | §21.1.1 |
| `E-SYS-3306` | Error    | Compile-time | `Drop + FfiSafe` by-value without `[[ffi_pass_by_value]]`           | §21.1.1 |
| `E-SYS-3310` | Error    | Compile-time | Non-FfiSafe type in extern signature                                | §21.2.1 |
| `E-SYS-3311` | Error    | Compile-time | Unknown ABI string                                                  | §21.2.1 |
| `E-SYS-3312` | Error    | Compile-time | Variadic without fixed parameters                                   | §21.2.1 |
| `E-SYS-3313` | Error    | Compile-time | Body provided for foreign procedure                                 | §21.2.1 |
| `E-SYS-3320` | Error    | Compile-time | Extern call outside `unsafe` block                                  | §21.2.1 |
| `E-SYS-3321` | Error    | Compile-time | Non-promotable type in variadic position                            | §21.2.1 |
| `E-SYS-3330` | Error    | Compile-time | Non-FfiSafe type for foreign global                                 | §21.2.2 |
| `E-SYS-3331` | Error    | Compile-time | Write to non-mut foreign global                                     | §21.2.2 |
| `E-SYS-3332` | Error    | Compile-time | Foreign global access outside unsafe                                | §21.2.2 |
| `E-SYS-3340` | Error    | Compile-time | `[[symbol]]` on non-FFI procedure                                   | §7.5.1  |
| `E-SYS-3341` | Error    | Compile-time | Empty symbol string                                                 | §7.5.1  |
| `E-SYS-3342` | Error    | Link-time    | Duplicate symbol name in compilation unit                           | §7.5.1  |
| `E-SYS-3345` | Error    | Compile-time | `[[library]]` outside `extern` block                                | §7.5.2  |
| `E-SYS-3346` | Error    | Compile-time | Unknown library kind                                                | §7.5.2  |
| `E-SYS-3347` | Error    | Link-time    | Library not found                                                   | §7.5.2  |
| `E-SYS-3350` | Error    | Compile-time | `[[no_mangle]]` on non-exportable procedure                         | §7.5.3  |
| `E-SYS-3351` | Error    | Compile-time | `[[no_mangle]]` combined with `[[symbol]]`                          | §7.5.3  |
| `E-SYS-3352` | Error    | Compile-time | Non-FfiSafe type in export signature                                | §21.2.3 |
| `E-SYS-3353` | Error    | Compile-time | Context or dynamic-class-containing type in export                  | §21.2.3 |
| `E-SYS-3354` | Error    | Link-time    | Duplicate export symbol name                                        | §21.2.3 |
| `E-SYS-3355` | Error    | Compile-time | Unknown unwind mode                                                 | §7.5.4  |
| `E-SYS-3356` | Error    | Compile-time | `[[unwind]]` on non-FFI procedure                                   | §7.5.4  |
| `E-SYS-3360` | Error    | Compile-time | Region-local pointer escapes via FFI                                | §21.3.3 |
| `E-SYS-3365` | Error    | Compile-time | `[[export]]` on non-public procedure                                | §21.2.3 |
| `E-SYS-3366` | Error    | Compile-time | Dynamic-class/capability parameter in export                        | §21.2.3 |
| `E-SYS-3367` | Error    | Compile-time | Capability-bearing dynamic class object or Context in FFI signature | §21.3.3 |

#### A.1.9 E-CTE (Compile-Time Evaluation)

| Code         | Severity | Detection    | Condition                                          | Source  |
| ------------ | -------- | ------------ | -------------------------------------------------- | ------- |
| `E-CTE-0010` | Error    | Compile-time | Non-comptime-available type in comptime expr       | §20.2.1 |
| `E-CTE-0011` | Error    | Compile-time | Reference type in comptime context                 | §20.2.1 |
| `E-CTE-0012` | Error    | Compile-time | Capability type in comptime context                | §20.2.1 |
| `E-CTE-0020` | Error    | Compile-time | Comptime block contains prohibited construct       | §20.1.1 |
| `E-CTE-0021` | Error    | Compile-time | Comptime expression has non-comptime type          | §20.1.1 |
| `E-CTE-0022` | Error    | Compile-time | Comptime evaluation diverges (exceeds limits)      | §20.1.1 |
| `E-CTE-0023` | Error    | Compile-time | Comptime evaluation panics                         | §20.1.1 |
| `E-CTE-0030` | Error    | Compile-time | Parameter type not comptime-available              | §20.2.2 |
| `E-CTE-0031` | Error    | Compile-time | Return type not comptime-available                 | §20.2.2 |
| `E-CTE-0032` | Error    | Compile-time | Comptime procedure body violates restrictions      | §20.2.2 |
| `E-CTE-0033` | Error    | Compile-time | Requires clause predicate evaluates to false       | §20.2.2 |
| `E-CTE-0034` | Error    | Compile-time | Comptime procedure called from runtime context     | §20.2.2 |
| `E-CTE-0040` | Error    | Compile-time | Emit operation without `TypeEmitter` capability    | §20.3.1 |
| `E-CTE-0041` | Error    | Compile-time | `[[emit]]` attribute on non-comptime block         | §20.3.1 |
| `E-CTE-0042` | Error    | Compile-time | Emitted AST is ill-formed                          | §20.3.1 |
| `E-CTE-0050` | Error    | Compile-time | `fields` called on non-record type                 | §20.3.2 |
| `E-CTE-0051` | Error    | Compile-time | `variants` called on non-enum type                 | §20.3.2 |
| `E-CTE-0052` | Error    | Compile-time | `states` called on non-modal type                  | §20.3.2 |
| `E-CTE-0053` | Error    | Compile-time | Introspection of incomplete type                   | §20.3.2 |
| `E-CTE-0060` | Error    | Compile-time | File operation without `ProjectFiles` capability   | §20.3.3 |
| `E-CTE-0061` | Error    | Compile-time | `[[files]]` attribute on non-comptime block        | §20.3.3 |
| `E-CTE-0062` | Error    | Compile-time | Path escapes project directory                     | §20.3.3 |
| `E-CTE-0063` | Error    | Compile-time | Absolute path in file operation                    | §20.3.3 |
| `E-CTE-0064` | Error    | Compile-time | File not found                                     | §20.3.3 |
| `E-CTE-0070` | Error    | Compile-time | Comptime error emitted by user code                | §20.3.4 |
| `E-CTE-0080` | Error    | Compile-time | Comptime if condition not comptime-evaluable       | §20.2.3 |
| `E-CTE-0081` | Error    | Compile-time | Comptime if condition not boolean                  | §20.2.3 |
| `E-CTE-0082` | Error    | Compile-time | Comptime for iterator not comptime-evaluable       | §20.2.3 |
| `E-CTE-0083` | Error    | Compile-time | Comptime for iterator not iterable                 | §20.2.3 |
| `E-CTE-0084` | Error    | Compile-time | Comptime for exceeds iteration limit               | §20.2.3 |
| `E-CTE-0100` | Error    | Compile-time | Comptime recursion depth exceeded                  | §20.2.4 |
| `E-CTE-0101` | Error    | Compile-time | Comptime iteration limit exceeded                  | §20.2.4 |
| `E-CTE-0102` | Error    | Compile-time | Comptime memory limit exceeded                     | §20.2.4 |
| `E-CTE-0103` | Error    | Compile-time | Comptime AST node limit exceeded                   | §20.2.4 |
| `E-CTE-0104` | Error    | Compile-time | Comptime execution time limit exceeded             | §20.2.4 |
| `E-CTE-0210` | Error    | Compile-time | Ast used in runtime context                        | §20.4.1 |
| `E-CTE-0220` | Error    | Compile-time | Quoted content is syntactically invalid            | §20.4.2 |
| `E-CTE-0221` | Error    | Compile-time | Quote expression outside comptime                  | §20.4.2 |
| `E-CTE-0230` | Error    | Compile-time | Splice type incompatible with context              | §20.4.3 |
| `E-CTE-0231` | Error    | Compile-time | Splice expression not comptime-evaluable           | §20.4.3 |
| `E-CTE-0232` | Error    | Compile-time | Invalid identifier string in splice                | §20.4.3 |
| `E-CTE-0233` | Error    | Compile-time | Splice expression outside quote context            | §20.4.3 |
| `E-CTE-0240` | Error    | Compile-time | Captured identifier no longer in scope at emission | §20.4.4 |
| `E-CTE-0241` | Error    | Compile-time | Hygiene renaming collision (implementation limit)  | §20.4.4 |
| `E-CTE-0250` | Error    | Compile-time | Emit without TypeEmitter capability                | §20.4.5 |
| `E-CTE-0251` | Error    | Compile-time | Emitted AST is not an item                         | §20.4.5 |
| `E-CTE-0252` | Error    | Compile-time | Emitted AST is ill-formed                          | §20.4.5 |
| `E-CTE-0253` | Error    | Compile-time | Type error in emitted code                         | §20.4.5 |
| `E-CTE-0310` | Error    | Compile-time | Unknown derive target name                         | §20.5.1 |
| `E-CTE-0311` | Error    | Compile-time | Derive attribute on non-type declaration           | §20.5.1 |
| `E-CTE-0312` | Error    | Compile-time | Duplicate derive target in attribute               | §20.5.1 |
| `E-CTE-0320` | Error    | Compile-time | Derive target body violates comptime rules         | §20.5.2 |
| `E-CTE-0321` | Error    | Compile-time | Contract references unknown class                  | §20.5.2 |
| `E-CTE-0322` | Error    | Compile-time | Derive target has invalid signature                | §20.5.2 |
| `E-CTE-0330` | Error    | Compile-time | Required class not implemented by target           | §20.5.3 |
| `E-CTE-0331` | Error    | Compile-time | Emits clause not satisfied after execution         | §20.5.3 |
| `E-CTE-0332` | Error    | Compile-time | Undeclared class emitted                           | §20.5.3 |
| `E-CTE-0340` | Error    | Compile-time | Cyclic derive dependency                           | §20.5.3 |
| `E-CTE-0341` | Error    | Compile-time | Derive target execution panics                     | §20.5.3 |
| `E-CTE-0342` | Error    | Compile-time | Derive execution exceeds resource limits           | §20.5.3 |
| `E-CTE-0360` | Error    | Compile-time | Field does not implement Debug                     | §20.5.4 |
| `E-CTE-0361` | Error    | Compile-time | Field does not implement Clone                     | §20.5.4 |
| `E-CTE-0362` | Error    | Compile-time | Field does not implement Eq                        | §20.5.4 |
| `E-CTE-0363` | Error    | Compile-time | Field does not implement Hash                      | §20.5.4 |
| `E-CTE-0364` | Error    | Compile-time | Field does not implement Default                   | §20.5.4 |
| `E-CTE-0365` | Error    | Compile-time | Default derive on non-record type                  | §20.5.4 |
| `E-CTE-0366` | Error    | Compile-time | Field does not implement Serialize                 | §20.5.4 |
| `E-CTE-0367` | Error    | Compile-time | Field does not implement Deserialize               | §20.5.4 |
| `E-CTE-0410` | Error    | Compile-time | Type argument to Type::<> is ill-formed            | §20.6.1 |
| `E-CTE-0411` | Error    | Compile-time | Type::<> used in runtime context                   | §20.6.1 |
| `E-CTE-0420` | Error    | Compile-time | Category query on incomplete type                  | §20.6.2 |
| `E-CTE-0430` | Error    | Compile-time | `fields` called on non-record type                 | §20.6.3 |
| `E-CTE-0440` | Error    | Compile-time | `variants` called on non-enum type                 | §20.6.4 |
| `E-CTE-0450` | Error    | Compile-time | `states` called on non-modal type                  | §20.6.5 |
| `E-CTE-0470` | Error    | Compile-time | Type predicate on incomplete type                  | §20.6.7 |

### A.2 Warning Codes

| Code         | Severity | Detection    | Condition                                                         | Source  |
| ------------ | -------- | ------------ | ----------------------------------------------------------------- | ------- |
| `W-SRC-0101` | Warning  | Compile-time | UTF-8 BOM present at the start of the file                        | §2.1.2  |
| `W-SRC-0301` | Warning  | Compile-time | Leading zeros in decimal literal                                  | §2.6.1  |
| `W-SRC-0308` | Warning  | Compile-time | Lexically sensitive Unicode character within `unsafe` block       | §2.8.1  |
| `W-CNF-0401` | Warning  | Compile-time | Implementation-reserved pattern used                              | §2.5.3  |
| `W-MOD-1101` | Warning  | Compile-time | Potential module path collision on case-insensitive systems       | §5.3.2  |
| `W-MOD-1201` | Warning  | Compile-time | Wildcard `using` (`*`) in a module exposing public API            | §6.4.2  |
| `W-MOD-2451` | Warning  | Compile-time | `align(N)` where N < natural alignment                            | §7.2.3  |
| `W-MOD-2452` | Warning  | Compile-time | `inline(always)` but inlining failed                              | §7.3.1  |
| `W-TYP-1701` | Warning  | Compile-time | `f16` arithmetic emulated on target platform                      | §11.1.2 |
| `W-TYP-2201` | Warning  | Compile-time | Union type contains duplicate member types                        | §11.8.1 |
| `W-SYS-2210` | Warning  | Compile-time | Union layout prevents niche optimization                          | §22.2.2 |
| `W-SYS-3350` | Warning  | Compile-time | `[[no_mangle]]` in `extern "C"` (redundant)                       | §7.5.3  |
| `W-SYS-3355` | Warning  | Compile-time | `[[unwind("abort")]]` (redundant)                                 | §7.5.4  |
| `W-SYS-4010` | Warning  | Compile-time | Modal widening involves large payload copy (> threshold)          | §12.1.7 |
| `W-CON-0001` | Warning  | Compile-time | Fine-grained keys in tight loop (performance hint)                | §17.2.1 |
| `W-CON-0002` | Warning  | Compile-time | Redundant key acquisition (already covered)                       | §17.2.1 |
| `W-CON-0003` | Warning  | Compile-time | `#` redundant (matches type boundary)                             | §17.2.4 |
| `W-CON-0004` | Warning  | Compile-time | Read-then-write may cause contention if parallelized              | §17.3.3 |
| `W-CON-0005` | Warning  | Compile-time | Callee access pattern unknown; assuming full access               | §17.4.2 |
| `W-CON-0006` | Warning  | Compile-time | Explicit read-then-write form used; compound assignment available | §17.3.3 |
| `W-CON-0009` | Warning  | Compile-time | Closure captures `shared` data                                    | §17.2.5 |
| `W-CON-0010` | Warning  | Compile-time | `release` block permits interleaving                              | §17.4.1 |
| `W-CON-0011` | Warning  | Compile-time | Access to potentially stale binding after release                 | §17.4.1 |
| `W-CON-0012` | Warning  | Compile-time | Nested `#` blocks with potential order cycle                      | §17.4.1 |
| `W-CON-0013` | Warning  | Compile-time | `ordered` modifier used with statically-comparable indices        | §17.2.4 |
| `W-CON-0020` | Warning  | Compile-time | Speculative block on large struct (may be inefficient)            | §17.5.1 |
| `W-CON-0021` | Warning  | Compile-time | Speculative block body may be expensive to re-execute             | §17.5.1 |
| `W-CON-0140` | Warning  | Compile-time | Dynamic key pattern; runtime serialization                        | §18.4.1 |
| `W-CON-0201` | Warning  | Compile-time | Large captured state (performance)                                | §19.4.2 |
| `W-CON-0401` | Warning  | Compile-time | `[[dynamic]]` present but all proofs succeed statically           | §7.4.3  |
| `W-CTE-0071` | Warning  | Compile-time | Comptime warning emitted by user code                             | §20.3.4 |


## Appendix B: Complete Grammar

This appendix consolidates all EBNF productions from the specification into a single reference suitable for parser implementation.

### B.1 Lexical Grammar

```ebnf
(* Source Structure *)
source_file     ::= normalized_line*
normalized_line ::= code_point* line_terminator?
line_terminator ::= "\n"
code_point      ::= (* Unicode scalar except U+000A and prohibited code points *)

(* Identifiers *)
identifier     ::= ident_start ident_continue*
ident_start    ::= (* Unicode XID_Start *) | "_"
ident_continue ::= (* Unicode XID_Continue *) | "_"

(* Integer Literals *)
integer_literal  ::= (decimal_integer | hex_integer | octal_integer | binary_integer) int_suffix?
decimal_integer  ::= dec_digit ("_"* dec_digit)*
hex_integer      ::= "0x" hex_digit ("_"* hex_digit)*
octal_integer    ::= "0o" oct_digit ("_"* oct_digit)*
binary_integer   ::= "0b" bin_digit ("_"* bin_digit)*
int_suffix       ::= "i8" | "i16" | "i32" | "i64" | "i128" | "u8" | "u16" | "u32" | "u64" | "u128"
dec_digit        ::= "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9"
hex_digit        ::= dec_digit | "a" | "b" | "c" | "d" | "e" | "f" | "A" | "B" | "C" | "D" | "E" | "F"
oct_digit        ::= "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7"
bin_digit        ::= "0" | "1"

(* Float Literals *)
float_literal ::= decimal_integer "." decimal_integer? exponent? float_suffix?
exponent      ::= ("e" | "E") ("+" | "-")? decimal_integer
float_suffix  ::= "f16" | "f32" | "f64"

(* String Literals *)
string_literal   ::= '"' (string_char | escape_sequence)* '"'
string_char      ::= (* Unicode scalar except ", \, or U+000A *)
escape_sequence  ::= "\n" | "\r" | "\t" | "\\" | "\"" | "\'" | "\0" | "\x" hex_digit hex_digit | "\u{" hex_digit+ "}"

(* Character Literals *)
char_literal ::= "'" (char_content | escape_sequence) "'"
char_content ::= (* Unicode scalar except ', \, or U+000A *)

(* Other Literals *)
bool_literal ::= "true" | "false"
null_literal ::= "null"
unit_literal ::= "(" ")"
```

### B.2 Type Grammar

```ebnf
(* Type Structure *)
type                ::= permission? non_permission_type
non_permission_type ::= union_type | non_union_type
permission          ::= "const" | "unique" | "shared"

(* Primitive Types *)
primitive_type ::= integer_type | float_type | bool_type | char_type | unit_type | never_type | string_type | bytes_type
integer_type   ::= signed_int | unsigned_int | pointer_int
signed_int     ::= "i8" | "i16" | "i32" | "i64" | "i128"
unsigned_int   ::= "u8" | "u16" | "u32" | "u64" | "u128"
pointer_int    ::= "isize" | "usize"
float_type     ::= "f16" | "f32" | "f64"
bool_type      ::= "bool"
char_type      ::= "char"
unit_type      ::= "(" ")"
never_type     ::= "!"

(* Compound Types *)
tuple_type       ::= "(" tuple_elements? ")"
tuple_elements   ::= type ";" | type ("," type)+
array_type       ::= "[" type ";" const_expression "]"
slice_type       ::= "[" type "]"
union_type       ::= non_union_type ("|" non_union_type)+

(* Nominal Types *)
nominal_type        ::= type_path generic_args?
type_path           ::= identifier ("::" identifier)*
opaque_type         ::= "opaque" class_path
dynamic_type        ::= "$" class_path
class_path          ::= type_path

(* String Types *)
string_type  ::= "string" ("@" string_state)?
bytes_type   ::= "bytes" ("@" bytes_state)?
string_state ::= "Managed" | "View"
bytes_state  ::= "Managed" | "View"

(* Pointer Types *)
pointer_type      ::= safe_pointer_type | raw_pointer_type
safe_pointer_type ::= "Ptr" "<" type ">" ("@" pointer_state)?
pointer_state     ::= "Valid" | "Null" | "Expired"
raw_pointer_type  ::= "*" raw_pointer_qual type
raw_pointer_qual  ::= "imm" | "mut"

(* Function Types *)
function_type        ::= sparse_function_type | closure_type
sparse_function_type ::= "(" param_type_list? ")" "->" type
closure_type         ::= "|" param_type_list? "|" "->" type closure_deps?
closure_deps         ::= "[" "shared" ":" "{" shared_dep_list? "}" "]"
shared_dep_list      ::= shared_dep ("," shared_dep)*
shared_dep           ::= identifier ":" type
param_type_list      ::= param_type ("," param_type)*
param_type           ::= "move"? type

(* Generic Types *)
generic_params     ::= "<" generic_param_list ">"
generic_param_list ::= generic_param (";" generic_param)*
generic_param      ::= identifier bound_clause? default_clause?
bound_clause       ::= "<:" class_bound_list
default_clause     ::= "=" type
class_bound_list   ::= class_bound ("," class_bound)*
class_bound        ::= type_path generic_args?
generic_args       ::= "<" type_arg_list ">"
type_arg_list      ::= type ("," type)*
turbofish          ::= "::" generic_args

(* Refinement Types *)
refinement_type       ::= type "where" "{" predicate_expr "}"
param_with_constraint ::= identifier ":" type "where" "{" predicate_expr "}"

(* Modal Types *)
modal_type_name     ::= type_path generic_args?
state_specific_type ::= modal_type_name "@" state_name
state_name          ::= identifier
```

### B.3 Expression Grammar

```ebnf
(* Expression Precedence - lowest to highest *)
expression         ::= attributed_expr | unattributed_expr
attributed_expr    ::= attribute_list expression
unattributed_expr  ::= range_expression | logical_or_expr

(* Range Expressions *)
range_expression    ::= exclusive_range | inclusive_range | from_range | to_range | to_inclusive_range | full_range
exclusive_range     ::= logical_or_expr ".." logical_or_expr
inclusive_range     ::= logical_or_expr "..=" logical_or_expr
from_range          ::= logical_or_expr ".."
to_range            ::= ".." logical_or_expr
to_inclusive_range  ::= "..=" logical_or_expr
full_range          ::= ".."

(* Binary Operators *)
logical_or_expr     ::= logical_and_expr ("||" logical_and_expr)*
logical_and_expr    ::= comparison_expr ("&&" comparison_expr)*
comparison_expr     ::= bitwise_or_expr (comparison_op bitwise_or_expr)*
comparison_op       ::= "==" | "!=" | "<" | "<=" | ">" | ">="
bitwise_or_expr     ::= bitwise_xor_expr ("|" bitwise_xor_expr)*
bitwise_xor_expr    ::= bitwise_and_expr ("^" bitwise_and_expr)*
bitwise_and_expr    ::= shift_expr ("&" shift_expr)*
shift_expr          ::= additive_expr (shift_op additive_expr)*
shift_op            ::= "<<" | ">>"
additive_expr       ::= multiplicative_expr (additive_op multiplicative_expr)*
additive_op         ::= "+" | "-"
multiplicative_expr ::= power_expr (multiplicative_op power_expr)*
multiplicative_op   ::= "*" | "/" | "%"
power_expr          ::= cast_expr ("**" power_expr)?
cast_expr           ::= unary_expr ("as" type)?

(* Unary Operators *)
unary_expr     ::= unary_operator unary_expr | pipeline_expr
unary_operator ::= "!" | "-" | "&" | "*" | "^" | "move" | "widen"
pipeline_expr  ::= postfix_expr ("=>" postfix_expr)*

(* Postfix Expressions *)
postfix_expr   ::= primary_expr postfix_suffix*
postfix_suffix ::= "." identifier | "." decimal_literal | "[" expression "]" | "~>" identifier "(" argument_list? ")" | "(" argument_list? ")" | "?"

(* Primary Expressions *)
primary_expr ::= literal | identifier_expr | path_expr | tuple_literal | array_literal | record_literal | closure_expr | if_expr | match_expr | loop_expr | block_expr | comptime_expr

(* Literals *)
literal ::= integer_literal | float_literal | string_literal | char_literal | bool_literal | null_literal | unit_literal

(* Identifier and Path *)
identifier_expr ::= identifier
path_expr       ::= type_path "::" identifier

(* Compound Literals *)
tuple_literal       ::= "(" tuple_expr_elements? ")"
tuple_expr_elements ::= expression ";" | expression ("," expression)+
array_literal       ::= "[" expression_list "]"
expression_list     ::= expression ("," expression)* ","?
record_literal      ::= type_path "{" field_init_list "}"
field_init_list     ::= field_init ("," field_init)* ","?
field_init          ::= identifier ":" expression | identifier
record_update       ::= type_path "from" expression "{" field_init_list "}"
enum_literal        ::= type_path "::" identifier variant_args?
variant_args        ::= "(" expression_list ")" | "{" field_init_list "}"

(* Call Expressions *)
call_expr     ::= postfix_expr "(" argument_list? ")"
argument_list ::= argument ("," argument)* ","?
argument      ::= "move"? expression
method_call   ::= postfix_expr "~>" identifier "(" argument_list? ")"
static_call   ::= type_path "::" identifier "(" argument_list? ")"

(* Closure Expressions *)
closure_expr       ::= "|" closure_param_list? "|" ("->" type)? closure_body
closure_param_list ::= closure_param ("," closure_param)* ","?
closure_param      ::= "move"? identifier (":" type)?
closure_body       ::= expression | block_expr

(* Control Flow Expressions *)
if_expr    ::= "if" expression block_expr ("else" (block_expr | if_expr))?
match_expr ::= "match" expression "{" match_arm+ "}"
match_arm  ::= pattern ("if" expression)? "=>" arm_body ","
arm_body   ::= expression | block_expr

(* Loop Expressions *)
loop_expr  ::= label? "loop" block_expr
cond_loop  ::= label? "loop" expression block_expr
iter_loop  ::= label? "loop" pattern (":" type)? "in" expression block_expr
label      ::= "'" identifier ":"

(* Block Expressions *)
block_expr ::= "{" statement* expression? "}"

(* Special Expressions *)
move_expr        ::= "move" place_expr
widen_expr       ::= "widen" expression
try_expr         ::= postfix_expr "?"
address_of_expr  ::= "&" place_expr
null_ptr_expr    ::= "Ptr" "::" "null" "()"
transmute_expr   ::= "transmute" "::" "<" type "," type ">" "(" expression ")"
```

### B.4 Pattern Grammar

```ebnf
pattern             ::= literal_pattern | wildcard_pattern | identifier_pattern | tuple_pattern | record_pattern | enum_pattern | modal_pattern | range_pattern
literal_pattern     ::= literal
wildcard_pattern    ::= "_"
identifier_pattern  ::= identifier
tuple_pattern       ::= "(" tuple_pattern_elements? ")"
tuple_pattern_elements ::= pattern ("," pattern)* ","?
record_pattern      ::= type_path "{" field_pattern_list? "}"
field_pattern_list  ::= field_pattern ("," field_pattern)* ","?
field_pattern       ::= identifier ":" pattern | identifier
enum_pattern        ::= type_path "::" identifier enum_payload_pattern?
enum_payload_pattern ::= "(" tuple_pattern_elements? ")" | "{" field_pattern_list? "}"
modal_pattern       ::= "@" identifier ("{" field_pattern_list? "}")?
range_pattern       ::= pattern (".." | "..=") pattern
```

### B.5 Statement Grammar

```ebnf
(* Statements *)
statement ::= binding_decl terminator | assignment_stmt terminator | expr_stmt | return_stmt | result_stmt | break_stmt | continue_stmt | defer_stmt | unsafe_block

(* Binding Declarations *)
binding_decl     ::= let_decl | var_decl | shadowed_binding
let_decl         ::= "let" pattern (":" type)? binding_op expression
var_decl         ::= "var" pattern (":" type)? binding_op expression
shadowed_binding ::= "shadow" "let" identifier (":" type)? "=" expression
binding_op       ::= "=" | ":="

(* Assignment *)
assignment_stmt ::= place_expr "=" expression
compound_assign ::= place_expr compound_op expression
compound_op     ::= "+=" | "-=" | "*=" | "/=" | "%="
place_expr      ::= identifier | postfix_expr "." identifier | postfix_expr "[" expression "]"

(* Expression Statement *)
expr_stmt  ::= expression terminator
terminator ::= ";" | newline
newline    ::= "\n"

(* Control Flow Statements *)
return_stmt   ::= "return" expression?
result_stmt   ::= "result" expression
break_stmt    ::= "break" ("'" identifier)? expression?
continue_stmt ::= "continue" ("'" identifier)?

(* Defer Statement *)
defer_stmt ::= "defer" block_expr

(* Unsafe Block *)
unsafe_block ::= "unsafe" block
```

### B.6 Declaration Grammar

```ebnf
(* Top-Level Items *)
top_level_item ::= import_decl | using_decl | static_decl | procedure_decl | record_decl | enum_decl | modal_decl | class_declaration | type_alias_decl

(* Import/Using *)
import_decl   ::= "import" module_path ("as" identifier)?
using_decl      ::= visibility? "using" using_clause
using_clause    ::= using_path ("as" identifier)? | using_list
using_path      ::= module_path ("::" identifier)?
using_list      ::= "{" using_specifier ("," using_specifier)* ","? "}"
using_specifier ::= identifier ("as" identifier)?
module_path   ::= identifier ("::" identifier)*

(* Static Declaration *)
static_decl ::= visibility? ("let" | "var") binding_decl

(* Visibility *)
visibility ::= "public" | "internal" | "private" | "protected"

(* Procedure Declaration *)
procedure_decl ::= visibility? "procedure" identifier generic_params? signature contract_clause? block?
signature      ::= "(" param_list? ")" ("->" return_type)?
param_list     ::= param ("," param)* ","?
param          ::= param_mode? identifier ":" type
param_mode     ::= "move"
return_type    ::= type | union_return
union_return   ::= type ("|" type)+

(* Method Definition *)
method_def        ::= visibility? "procedure" identifier "(" receiver ("," param_list)? ")" ("->" return_type)? contract_clause? block
receiver          ::= receiver_shorthand | explicit_receiver
receiver_shorthand ::= "~" | "~!" | "~%"
explicit_receiver ::= param_mode? "self" ":" type

(* Record Declaration *)
record_decl       ::= visibility? "record" identifier generic_params? implements_clause? "{" record_body "}"
record_body       ::= record_member ("," record_member)* ","?
record_member     ::= record_field_decl | method_def
record_field_decl ::= visibility? identifier ":" type record_field_init?
record_field_init ::= "=" expression
field_decl        ::= visibility? identifier ":" type
implements_clause ::= "<:" class_list
class_list        ::= type_path ("," type_path)*

(* Enum Declaration *)
enum_decl       ::= visibility? "enum" identifier generic_params? implements_clause? "{" variant_list "}"
variant_list    ::= variant ("," variant)* ","?
variant         ::= identifier variant_payload? ("=" integer_literal)?
variant_payload ::= "(" type_list ")" | "{" field_decl_list "}"
type_list       ::= type ("," type)* ","?
field_decl_list ::= field_decl ("," field_decl)* ","?

(* Modal Declaration *)
modal_decl   ::= visibility? "modal" identifier generic_params? implements_clause? "{" state_block+ "}"
state_block  ::= "@" state_name "{" state_member* "}"
state_member ::= state_field_decl | state_method_def | transition_def
state_field_decl ::= identifier ":" type
state_method_def ::= "procedure" identifier "(" param_list ")" ("->" return_type)? block
transition_def   ::= "transition" identifier "(" param_list ")" "->" "@" target_state block
target_state     ::= identifier

(* Class Declaration *)
class_declaration ::= visibility? "modal"? "class" identifier generic_params? ("<:" superclass_bounds)? where_clause? "{" class_item* "}"
superclass_bounds ::= class_bound ("+" class_bound)*
class_item        ::= abstract_procedure | concrete_procedure | abstract_field | abstract_state | associated_type
abstract_procedure ::= "procedure" identifier signature contract_clause?
concrete_procedure ::= "procedure" identifier signature contract_clause? block
abstract_field     ::= identifier ":" type
abstract_state     ::= "@" identifier "{" field_list? "}"
field_list         ::= abstract_field ("," abstract_field)* ","?
associated_type    ::= "type" identifier ("=" type)?

(* Type Alias *)
type_alias_decl ::= visibility? "type" identifier "=" type ("where" "{" predicate_expr "}")?

(* Where Clause *)
where_clause         ::= "where" where_predicate_list
where_predicate_list ::= where_predicate (terminator where_predicate)* ";"?
where_predicate      ::= identifier "<:" class_bound_list
```

### B.7 Contract Grammar

```ebnf
contract_clause    ::= "|=" contract_body
contract_body      ::= precondition_expr ("," postcondition_expr)?
precondition_expr  ::= predicate_expr
postcondition_expr ::= predicate_expr
predicate_expr     ::= logical_or_expr
contract_intrinsic ::= "@result" | "@entry" "(" expression ")"

(* Type/Loop Invariants *)
type_invariant ::= "where" "{" predicate_expr "}"
loop_invariant ::= "where" "{" predicate_expr "}"
```

### B.8 Attribute Grammar

```ebnf
attribute_list ::= attribute+
attribute      ::= "[[" attribute_spec ("," attribute_spec)* "]]"
attribute_spec ::= attribute_name ("(" attribute_args ")")?
attribute_name ::= identifier | vendor_prefix "::" identifier
vendor_prefix  ::= identifier ("." identifier)*
attribute_args ::= attribute_arg ("," attribute_arg)*
attribute_arg  ::= literal | identifier ":" literal

(* Specific Attributes *)
layout_attribute     ::= "[[" "layout" "(" layout_args ")" "]]"
layout_args          ::= layout_kind ("," layout_kind)*
layout_kind          ::= "C" | "packed" | "align" "(" integer_literal ")" | int_type
inline_attribute     ::= "[[" "inline" ("(" inline_mode ")")? "]]"
inline_mode          ::= "always" | "never" | "default"
cold_attribute       ::= "[[" "cold" "]]"
deprecated_attribute ::= "[[" "deprecated" ("(" string_literal ")")? "]]"
reflect_attribute    ::= "[[" "reflect" "]]"
dynamic_attribute    ::= "[[" "dynamic" "]]"
stale_ok_attribute   ::= "[[" "stale_ok" "]]"
symbol_attribute     ::= "[[" "symbol" "(" string_literal ")" "]]"
no_mangle_attribute  ::= "[[" "no_mangle" "]]"
export_attr          ::= "[[" "export" "(" string_literal ")" "]]"
derive_attr          ::= "[[" "derive" "(" derive_target_list ")" "]]"
derive_target_list   ::= derive_target ("," derive_target)*
derive_target        ::= identifier
```

### B.9 Key System Grammar

```ebnf
(* Key Path *)
key_path_expr   ::= root_segment ("." path_segment)*
root_segment    ::= key_marker? identifier index_suffix?
path_segment    ::= key_marker? identifier index_suffix?
key_marker      ::= "#"
index_suffix    ::= "[" expression "]"

(* Key Block *)
key_block        ::= "#" path_list mode_modifier* block
path_list        ::= key_path_expr ("," key_path_expr)*
mode_modifier    ::= "write" | "read" | release_modifier | "ordered" | "speculative"
release_modifier ::= "release" ("write" | "read")

(* Speculative Block *)
speculative_block ::= "#" path_list "speculative" "write" block

(* Coarsened Path *)
coarsened_path ::= path_segment* "#" path_segment+
```

### B.10 Concurrency Grammar

```ebnf
(* Parallel Block *)
parallel_block ::= "parallel" domain_expr block_options? block
domain_expr    ::= expression
block_options  ::= "[" block_option ("," block_option)* "]"
block_option   ::= "cancel" ":" expression

(* Spawn Expression *)
spawn_expr        ::= "spawn" spawn_option_list? block
spawn_option_list ::= "[" spawn_option ("," spawn_option)* "]"
spawn_option      ::= "name" ":" string_literal

(* Dispatch Expression *)
dispatch_expr        ::= "dispatch" pattern "in" range_expression key_clause? dispatch_option_list? block
key_clause           ::= "key" key_path_expr key_mode
key_mode             ::= "read" | "write"
dispatch_option_list ::= "[" dispatch_option ("," dispatch_option)* "]"
dispatch_option      ::= "reduce" ":" reduce_op
reduce_op            ::= "+" | "*" | "min" | "max" | "and" | "or" | identifier

(* Memory Ordering *)
memory_order_attribute ::= "[[" memory_order "]]"
memory_order           ::= "relaxed" | "acquire" | "release" | "acq_rel" | "seq_cst"
```

### B.11 Async Grammar

```ebnf
(* Async Type *)
async_class ::= "class" "Async" "<" type_param (";" type_param)* ">"
type_param  ::= identifier ("=" type)?

(* Async Procedure *)
async_procedure ::= procedure_decl

(* Yield Expressions *)
yield_expr      ::= "yield" "release"? expression
yield_from_expr ::= "yield" "release"? "from" expression

(* Async Loops and Expressions *)
async_loop ::= "loop" pattern "in" expression block
sync_expr  ::= "sync" expression
race_expr    ::= "race" "{" race_arm ("," race_arm)* ","? "}"
race_arm     ::= expression "->" "|" pattern "|" race_handler
race_handler ::= expression | "yield" expression
all_expr     ::= "all" "{" expression ("," expression)* ","? "}"
```

### B.12 Metaprogramming Grammar

```ebnf
(* Comptime *)
comptime_block ::= "comptime" block
comptime_expr  ::= "comptime" "{" expression "}"

(* Comptime Procedure *)
comptime_procedure_decl ::= "comptime" "procedure" identifier generic_params? signature requires_clause? block
requires_clause         ::= "requires" comptime_predicate ("," comptime_predicate)*
comptime_predicate      ::= comptime_expr

(* Comptime Control Flow *)
comptime_if  ::= "comptime" "if" comptime_expr block ("else" (comptime_if | block))?
comptime_for ::= "comptime" "for" pattern "in" comptime_expr block

(* Type Literal *)
type_literal ::= "Type" "::<" type ">"

(* Quote/Splice *)
quote_expr     ::= "quote" "{" quoted_content "}"
quote_type     ::= "quote" "type" "{" type "}"
quote_pattern  ::= "quote" "pattern" "{" pattern "}"
quoted_content ::= expression | stmt_list | item_list
stmt_list      ::= statement+
item_list      ::= item+
splice_expr    ::= "$(" comptime_expr ")"
splice_ident   ::= "$" identifier

(* Emit *)
emit_stmt      ::= expression "~>" "emit" "(" expression ")"
emit_attribute ::= "[[" "emit" "]]"

(* Derive *)
derive_target_decl ::= "derive" "target" identifier derive_contract? block
derive_contract    ::= "|=" derive_clause ("," derive_clause)*
derive_clause      ::= "emits" class_ref
class_ref          ::= identifier
```

### B.13 FFI Grammar

```ebnf
(* Extern Block *)
extern_block      ::= "extern" abi_string? "{" extern_item* "}"
abi_string        ::= string_literal
extern_item       ::= foreign_procedure | foreign_global

(* Foreign Procedure *)
foreign_procedure ::= visibility? "procedure" identifier "(" ffi_param_list? variadic_spec? ")" ("->" type)?
ffi_param_list    ::= ffi_param ("," ffi_param)*
ffi_param         ::= identifier ":" type
variadic_spec     ::= "..." | "...," type

(* Foreign Global *)
foreign_global ::= visibility? "mut"? identifier ":" type ";"

(* Foreign Contracts *)
ffi_verification_attr ::= "[[" ffi_verification_mode "]]"
ffi_verification_mode ::= "static" | "dynamic" | "assume" | "trust"
foreign_contract      ::= "|=" "@foreign_assumes" "(" predicate_list ")"
predicate_list        ::= predicate ("," predicate)*
predicate             ::= comparison_expr | null_check | range_check
null_check            ::= expression "!=" null_literal | expression "==" null_literal
range_check           ::= expression "in" range_expression
foreign_ensures       ::= "@foreign_ensures" "(" ensures_predicate_list ")"
ensures_predicate_list ::= ensures_predicate ("," ensures_predicate)*
ensures_predicate     ::= predicate
```

### B.14 Region Grammar

```ebnf
region_stmt    ::= "region" region_options? region_alias? block
region_options ::= region_option*
region_option  ::= "." "stack_size" "(" expression ")"
region_alias   ::= "as" identifier
alloc_expr     ::= identifier "^" expression
frame_stmt     ::= "frame" block
```


## Appendix C: Type Layout Reference

This appendix defines the memory layout for all Cursive types.

### C.1 Primitive Type Layouts

| Type    | Size (bytes) | Alignment | Representation                                            |
| ------- | ------------ | --------- | --------------------------------------------------------- |
| `i8`    | 1            | 1         | Two's complement signed                                   |
| `i16`   | 2            | 2         | Two's complement signed                                   |
| `i32`   | 4            | 4         | Two's complement signed                                   |
| `i64`   | 8            | 8         | Two's complement signed                                   |
| `i128`  | 16           | 16        | Two's complement signed                                   |
| `u8`    | 1            | 1         | Unsigned binary                                           |
| `u16`   | 2            | 2         | Unsigned binary                                           |
| `u32`   | 4            | 4         | Unsigned binary                                           |
| `u64`   | 8            | 8         | Unsigned binary                                           |
| `u128`  | 16           | 16        | Unsigned binary                                           |
| `f16`   | 2            | 2         | IEEE 754 binary16                                         |
| `f32`   | 4            | 4         | IEEE 754 binary32                                         |
| `f64`   | 8            | 8         | IEEE 754 binary64                                         |
| `bool`  | 1            | 1         | 0x00 = false, 0x01 = true                                 |
| `char`  | 4            | 4         | Unicode scalar value (0..=0x10FFFF, excluding surrogates) |
| `()`    | 0            | 1         | Zero-sized type                                           |
| `!`     | 0            | 1         | Uninhabited (zero-sized)                                  |
| `usize` | Platform     | Platform  | Pointer-sized unsigned                                    |
| `isize` | Platform     | Platform  | Pointer-sized signed                                      |

### C.2 Compound Type Layout Rules

**Tuples**

Size = sum of field sizes + padding for alignment. Fields laid out in declaration order. Alignment = max field alignment.

**Arrays `[T; N]`**

Size = `size_of::<T>() * N`. Alignment = `align_of::<T>()`. Elements contiguous with no padding between them.

**Slices `[T]`**

Fat pointer: `(data_ptr, length)`. Size = 2 × pointer size. Alignment = pointer alignment.

**Records**

Default layout: Fields in declaration order with padding for natural alignment. `[[layout(C)]]`: C-compatible layout. `[[layout(packed)]]`: No padding between fields. Size = sum of fields + padding. Alignment = max field alignment (or 1 if packed).

**Enums**

Discriminant + largest variant payload. Discriminant type: compiler-chosen or explicit via `[[layout(IntType)]]`. Size = discriminant size + max payload size + padding.

**Unions**

Size = max member size. Alignment = max member alignment. No discriminant.

### C.3 Pointer Type Layouts

| Type            | Size             | Alignment         | Notes                    |
| --------------- | ---------------- | ----------------- | ------------------------ |
| `Ptr<T>`        | pointer size     | pointer alignment | Safe modal pointer       |
| `*imm T`        | pointer size     | pointer alignment | Raw immutable pointer    |
| `*mut T`        | pointer size     | pointer alignment | Raw mutable pointer      |
| Closure         | 2 × pointer size | pointer alignment | Fat pointer (code + env) |
| Sparse function | pointer size     | pointer alignment | Code pointer only        |

*On 64-bit platforms: pointer size = 8, alignment = 8*
*On 32-bit platforms: pointer size = 4, alignment = 4*

### C.4 String and Bytes Type Layouts

| Type             | Size             | Alignment         | Representation                 |
| ---------------- | ---------------- | ----------------- | ------------------------------ |
| `string@Managed` | 3 × pointer size | pointer alignment | `(data_ptr, length, capacity)` |
| `string@View`    | 2 × pointer size | pointer alignment | `(data_ptr, length)`           |

**Bytes Type Layouts**

| Type            | Size             | Alignment         | Representation                 |
| --------------- | ---------------- | ----------------- | ------------------------------ |
| `bytes@Managed` | 3 × pointer size | pointer alignment | `(data_ptr, length, capacity)` |
| `bytes@View`    | 2 × pointer size | pointer alignment | `(data_ptr, length)`           |

### C.5 Modal Type Layouts

Modal types use discriminated union layout:
- Discriminant: compiler-chosen integer type (typically smallest sufficient)
- Payload: largest state payload + padding

Niche optimization: States with compatible layouts MAY share discriminant values with null representations.


## Appendix D: Standard Form Catalog

This appendix defines the signatures and semantics of built-in classes referenced throughout the specification.


### D.1 Foundational Classes


#### D.1.1 `Drop`

**Drop Class.** Defines deterministic destruction for types requiring cleanup.

**Syntax**

```cursive
class Drop {
    procedure drop(~!)
}
```

**Static Semantics**

*[REF: Drop invocation order defined in §3.5.2.]*

Types implementing `Drop` MUST NOT implement `Bitcopy`. The `drop` procedure is invoked implicitly when a responsible binding goes out of scope; direct calls are ill-formed.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-MEM-3005`, `E-TYP-2621`.


#### D.1.2 `Bitcopy`

**Bitcopy Class.** A marker indicating bitwise-copyable types.

**Syntax**

```cursive
class Bitcopy { }
```

**Static Semantics**

A type implementing `Bitcopy` is duplicated rather than moved on assignment. All fields of a `Bitcopy` type MUST also implement `Bitcopy`.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-TYP-2621`, `E-TYP-2622`.


#### D.1.3 `Clone`

**Clone Class.** Defines explicit duplication for non-`Bitcopy` types.

**Syntax**

```cursive
class Clone {
    procedure clone(~) -> Self
}
```

**Static Semantics**

The `clone` procedure creates a new instance with the same value. Unlike `Bitcopy`, cloning may involve heap allocation or other side effects.


#### D.1.4 `Iterator`

**Iterator Class.** Defines the iteration protocol for `loop...in` expressions.

**Syntax**

```cursive
class Iterator {
    type Item
    procedure next(~!) -> Self::Item | ()
}
```

**Static Semantics**

*[REF: Loop desugaring defined in §15.9.3.]*

The `next` procedure returns the next item or `()` when exhausted. The iterator MUST be accessed with `unique` permission.


#### D.1.5 `Default`

**Default Class.** Provides a default value constructor.

**Syntax**

```cursive
class Default {
    procedure default() -> Self
}
```

**Static Semantics**

The `default` procedure is a static constructor returning a default-initialized instance.


### D.2 Comparison Classes


#### D.2.1 `Eq`

**Eq Class.** Defines equality comparison.

**Syntax**

```cursive
class Eq {
    procedure eq(~, other: const Self) -> bool
}
```

**Static Semantics**

Equality MUST be reflexive, symmetric, and transitive.


#### D.2.2 `Ord`

**Ord Class.** Defines total ordering.

**Syntax**

```cursive
class Ord <: Eq {
    procedure cmp(~, other: const Self) -> Ordering
}
```

**Static Semantics**

`Ordering` is an enum with variants `Less`, `Equal`, `Greater`. Ordering MUST be transitive and antisymmetric.


#### D.2.3 `Hash`

**Hash Class.** Defines hashing for use in hash-based collections.

**Syntax**

```cursive
class Hash {
    procedure hash(~, hasher: unique Hasher)
}
```

**Static Semantics**

Types implementing `Hash` MUST also implement `Eq`. If `a~>eq(b)` returns `true`, then `a~>hash(h)` and `b~>hash(h)` MUST produce identical hasher states.


### D.3 Capability Classes


#### D.3.1 `FileSystem`

**FileSystem Capability Class.** Provides file I/O operations. The capability type is `$FileSystem`.

**Syntax**

**FileSystem Interface.**

| Method | Return Type |
| :----- | :---------- |
| `open_read(path: string@View)` | `File@Read | IoError` |
| `open_write(path: string@View)` | `File@Write | IoError` |
| `open_append(path: string@View)` | `File@Append | IoError` |
| `create_write(path: string@View)` | `File@Write | IoError` |
| `read_file(path: string@View)` | `string@Managed | IoError` |
| `read_bytes(path: string@View)` | `bytes@Managed | IoError` |
| `write_file(path: string@View, data: bytes@View)` | `() | IoError` |
| `write_stdout(data: string@View)` | `() | IoError` |
| `write_stderr(data: string@View)` | `() | IoError` |
| `exists(path: string@View)` | `bool` |
| `remove(path: string@View)` | `() | IoError` |
| `open_dir(path: string@View)` | `DirIter@Open | IoError` |
| `create_dir(path: string@View)` | `() | IoError` |
| `ensure_dir(path: string@View)` | `() | IoError` |
| `kind(path: string@View)` | `FileKind | IoError` |
| `restrict(path: string@View)` | `$FileSystem` |

**FileKind.**

`FileKind` has the following variants: `File`, `Dir`, `Other`.

`FileKind` implements `Bitcopy`.

**DirEntry Record.**

| Field | Type |
| :---- | :--- |
| `name` | `string@Managed` |
| `path` | `string@Managed` |
| `kind` | `FileKind` |

**DirIter Modal Type.**

States:

$$\text{States}(\texttt{DirIter}) = \{\texttt{@Open}, \texttt{@Closed}\}$$

Payloads:

$$\text{Payload}(\texttt{@Open}) = [\langle \texttt{handle}, \texttt{usize} \rangle]$$
$$\text{Payload}(\texttt{@Closed}) = []$$

State members:

| State | Member | Kind | Parameters | Return Type |
| :---- | :----- | :--- | :--------- | :---------- |
| `@Open` | `next` | method | `()` | `DirEntry | () | IoError` |
| `@Open` | `close` | transition | `()` | `DirIter@Closed` |
| `@Closed` | *(none)* | *(n/a)* | *(n/a)* | *(n/a)* |

**File Modal Type.**

States:

$$\text{States}(\texttt{File}) = \{\texttt{@Read}, \texttt{@Write}, \texttt{@Append}, \texttt{@Closed}\}$$

Payloads:

$$\text{Payload}(\texttt{@Read}) = [\langle \texttt{handle}, \texttt{usize} \rangle]$$
$$\text{Payload}(\texttt{@Write}) = [\langle \texttt{handle}, \texttt{usize} \rangle]$$
$$\text{Payload}(\texttt{@Append}) = [\langle \texttt{handle}, \texttt{usize} \rangle]$$
$$\text{Payload}(\texttt{@Closed}) = []$$

State members:

| State | Member | Kind | Parameters | Return Type |
| :---- | :----- | :--- | :--------- | :---------- |
| `@Read` | `read_all` | method | `()` | `string@Managed | IoError` |
| `@Read` | `read_all_bytes` | method | `()` | `bytes@Managed | IoError` |
| `@Read` | `close` | transition | `()` | `File@Closed` |
| `@Write` | `write` | method | `(data: bytes@View)` | `() | IoError` |
| `@Write` | `flush` | method | `()` | `() | IoError` |
| `@Write` | `close` | transition | `()` | `File@Closed` |
| `@Append` | `write` | method | `(data: bytes@View)` | `() | IoError` |
| `@Append` | `flush` | method | `()` | `() | IoError` |
| `@Append` | `close` | transition | `()` | `File@Closed` |
| `@Closed` | *(none)* | *(n/a)* | *(n/a)* | *(n/a)* |

**IoError.**

`IoError` has the following variants: `NotFound`, `PermissionDenied`, `AlreadyExists`, `InvalidPath`, `Busy`, `IoFailure`.

`IoError` implements `Bitcopy`.

**Static Semantics**

*[REF: Capability model defined in §4.]*

1. `File`, `DirIter`, `DirEntry`, `FileKind`, and `IoError` are built-in types with the definitions above.
2. The state members of `File` and `DirIter` are exactly those listed in the state member tables above.
3. Any record literal whose type is `File@S` or `DirIter@S` for any state `S` in the corresponding modal type is ill-formed.
4. `open_dir` returns a directory iterator over the direct children of `path` in the operating system's enumeration order.
5. `DirIter@Open~>next()` yields a `DirEntry` for the next entry, `()` when the iterator is exhausted, or `IoError` on failure.
6. `restrict` returns a capability value that is a restriction of the receiver.

**Dynamic Semantics**

**Primitive Relations (IDB).**

**Decision (IDB).** File system effects are defined by the following relations. The runtime MUST document the mapping from platform errors to `IoError` variants and the path rules used by `FSRestrict`.

Judgments:

- `FSOpenRead(fs, path) ⇓ r` where `r : File@Read | IoError`.
- `FSOpenWrite(fs, path) ⇓ r` where `r : File@Write | IoError`.
- `FSOpenAppend(fs, path) ⇓ r` where `r : File@Append | IoError`.
- `FSCreateWrite(fs, path) ⇓ r` where `r : File@Write | IoError`.
- `FSReadFile(fs, path) ⇓ r` where `r : string@Managed | IoError`.
- `FSReadBytes(fs, path) ⇓ r` where `r : bytes@Managed | IoError`.
- `FSWriteFile(fs, path, data) ⇓ r` where `r : () | IoError`.
- `FSWriteStdout(fs, data) ⇓ r` where `r : () | IoError`.
- `FSWriteStderr(fs, data) ⇓ r` where `r : () | IoError`.
- `FSExists(fs, path) ⇓ b` where `b : bool`.
- `FSRemove(fs, path) ⇓ r` where `r : () | IoError`.
- `FSOpenDir(fs, path) ⇓ r` where `r : DirIter@Open | IoError`.
- `FSCreateDir(fs, path) ⇓ r` where `r : () | IoError`.
- `FSEnsureDir(fs, path) ⇓ r` where `r : () | IoError`.
- `FSKind(fs, path) ⇓ r` where `r : FileKind | IoError`.
- `FSRestrict(fs, path) ⇓ fs'` where `fs' : $FileSystem`.
- `FileReadAll(handle) ⇓ r` where `r : string@Managed | IoError`.
- `FileReadAllBytes(handle) ⇓ r` where `r : bytes@Managed | IoError`.
- `FileWrite(handle, data) ⇓ r` where `r : () | IoError`.
- `FileFlush(handle) ⇓ r` where `r : () | IoError`.
- `FileClose(handle) ⇓ ok`.
- `DirNext(handle) ⇓ r` where `r : DirEntry | () | IoError`.
- `DirClose(handle) ⇓ ok`.

**Restriction Semantics.** Let `RestrictPath(base, path)` be an implementation-defined partial function yielding a canonical path or `⊥`. It MUST satisfy:

1. If `path` is absolute, `RestrictPath(base, path) = ⊥`.
2. If `path` escapes `base` after normalization, `RestrictPath(base, path) = ⊥`.
3. Otherwise, `RestrictPath(base, path)` yields a path within `base`.

Let `FSOp` range over {`FSOpenRead`, `FSOpenWrite`, `FSOpenAppend`, `FSCreateWrite`, `FSReadFile`, `FSReadBytes`, `FSWriteFile`, `FSWriteStdout`, `FSWriteStderr`, `FSExists`, `FSRemove`, `FSOpenDir`, `FSCreateDir`, `FSEnsureDir`, `FSKind`}.

Let `fs'` be any value returned by `FSRestrict(fs, base)`. For any FileSystem operation `Op` with a path argument `p`, its effect is defined by:

- If `RestrictPath(base, p) = q`, then `FSOp(fs', p) = FSOp(fs, q)`.
- If `RestrictPath(base, p) = ⊥`, then the operation yields `IoError::InvalidPath` (or `false` for `exists`).

**IoError Mapping.** For any file-system operation with a path argument:

1. If the path is invalid or `RestrictPath` is `⊥`, the operation MUST yield `IoError::InvalidPath`.
2. If the operation requires an existing entry and no entry exists at the path, it MUST yield `IoError::NotFound`.
3. If the operation fails due to insufficient permissions, it MUST yield `IoError::PermissionDenied`.
4. If `FileSystem::create_write` is invoked on an existing path, it MUST yield `IoError::AlreadyExists`.
5. If `FileSystem::create_dir` or `FileSystem::ensure_dir` is invoked on an existing non-directory path, it MUST yield `IoError::AlreadyExists`.
6. If `FileSystem::open_dir` is invoked on an existing non-directory path, it MUST yield `IoError::InvalidPath`.
7. If the operation fails because the file or directory is locked or busy, it MUST yield `IoError::Busy`.
8. Any other failure MUST yield `IoError::IoFailure`.

If `FileSystem::read_file` or `File::read_all` encounters non-UTF-8 byte sequences, it MUST yield `IoError::IoFailure`. `FileSystem::read_bytes` and `File::read_all_bytes` perform no UTF-8 validation.

`FSExists` MUST return `true` iff the path resolves to an existing entry and `RestrictPath` is defined; otherwise `false`.

**File and Directory Operation Semantics.**

1. `FileSystem::open_read` opens an existing file for reading with position at 0.
2. `FileSystem::open_write` opens an existing file for writing with position at 0 and MUST NOT truncate the file.
3. `FileSystem::create_write` creates a new file and opens it for writing with position at 0.
4. `FileSystem::open_append` opens an existing file for appending with position at EOF; writes append to the end.
5. `FileSystem::read_file` is equivalent to `FileSystem::open_read` followed by `File::read_all` and `File::close` on success.
6. `FileSystem::read_bytes` is equivalent to `FileSystem::open_read` followed by `File::read_all_bytes` and `File::close` on success.
7. `File::read_all` reads from the current position to EOF and advances the position to EOF.
8. `File::read_all_bytes` reads raw bytes from the current position to EOF and advances the position to EOF.
9. `File::write` writes all bytes at the current position (or EOF for `@Append`) and advances the position by the number of bytes written.
10. `File::flush` ensures that all prior writes are committed to the underlying device.
11. `File::close` invalidates the handle; any subsequent file operation on that handle MUST yield `IoError::IoFailure`.
12. If a handle was never issued by the runtime or is already closed, `File::read_all`, `File::read_all_bytes`, `File::write`, and `File::flush` MUST yield `IoError::IoFailure`.
13. `FileSystem::open_dir` opens an existing directory and returns a `DirIter@Open` iterator over its direct children in the operating system's enumeration order. The enumeration MUST NOT include `.` or `..`.
14. `DirIter::next` yields the next `DirEntry` in enumeration order, `()` when exhausted, or `IoError` on failure.
15. For any `DirEntry` returned by `DirIter::next`, `entry.path` MUST be a path accepted by FileSystem operations, `entry.name` MUST be the final path component of `entry.path`, and `entry.kind` MUST equal `FSKind(fs, entry.path)`.
16. `FileSystem::create_dir` creates a new directory at `path` and fails if any component is missing.
17. `FileSystem::ensure_dir` creates all missing path components and succeeds if the path already exists as a directory.
18. `FileSystem::kind` yields `FileKind::File`, `FileKind::Dir`, or `FileKind::Other` for existing paths.
19. `DirIter::close` invalidates the iterator; any subsequent directory operation on that handle MUST yield `IoError::IoFailure`.

#### D.3.2 `HeapAllocator`

**HeapAllocator Capability Class.** Provides dynamic memory allocation. The capability type is `$HeapAllocator`.

**Syntax**

```cursive
class HeapAllocator {
    // Attenuation
    procedure with_quota(~!, size: usize) -> $HeapAllocator

    // Low-level raw interface (unsafe-only)
    procedure alloc_raw<T>(~!, count: usize) -> *mut T
    procedure dealloc_raw<T>(~!, ptr: *mut T, count: usize)
}
```

**Static Semantics**

1. `with_quota` requires `unique` permission on the allocator and returns an attenuated child allocator enforcing the specified quota.
2. Low-level raw operations MUST occur only within `unsafe` blocks; dereferencing or freeing raw pointers outside `unsafe` is Unverifiable Behavior (UVB).
3. Safe heap allocation for values is specified by `to_heap` (§3.2.7) and by built‑in/core‑library operations that accept a `$HeapAllocator`.

**AllocationError**

Safe, fallible heap allocation operations (including `to_heap`) return `AllocationError` on failure.

```cursive
enum AllocationError {
    OutOfMemory { size: usize },
    QuotaExceeded { size: usize },
}
```

1. `OutOfMemory` indicates the allocator could not satisfy the request due to system exhaustion.
2. `QuotaExceeded` indicates the request exceeded the effective quota of the allocator lineage.


#### D.3.3 `Network`

**Network Capability Class.** Provides network I/O operations. The capability type is `$Network`.

**Syntax**

```cursive
class Network {
    procedure connect(~, address: string@View, port: u16) -> TcpStream | IoError
    procedure listen(~, address: string@View, port: u16) -> TcpListener | IoError
    procedure resolve(~, hostname: string@View) -> [IpAddress] | IoError
}
```


#### D.3.4 `Reactor`

**Reactor Capability Class.** Provides async runtime services. The capability type is `$Reactor`.

**Syntax**

```cursive
class Reactor {
    procedure run<T; E>(~, future: Future<T, E>) -> T | E
    procedure register<T; E>(~, future: Future<T, E>) -> FutureHandle<T, E>
}
```

**Static Semantics**

*[REF: Async runtime defined in §19.]*


### D.4 Context Record


#### D.4.1 `Context`

**Context Record.** The root of authority for program execution.

**Syntax**

```cursive
record Context {
    fs: $FileSystem,
    net: $Network,
    sys: System,
    heap: $HeapAllocator,
    reactor: $Reactor,
    cpu: CpuDomainFactory,
    gpu: GpuDomainFactory | (),
    inline: InlineDomainFactory
}
```

**Static Semantics**

*[REF: Authority model defined in §4.]*

The `Context` is constructed by the runtime before `main` is invoked. Programs MUST NOT construct `Context` directly.
