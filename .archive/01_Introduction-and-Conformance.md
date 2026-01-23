# Cursive Language Specification

## Clause 1 — Introduction and Conformance [intro]

**Forward references**: §2.1–§2.5 [lex.source]–[lex.units], Clause 5 [decl], Clause 6 [name], Clause 7 [type], Clause 8 [expr], Clause 10 [generic], Clause 11 [memory], Clause 12 [contract], Clause 13 [witness], §16.6 [interop.abi], Annex A [grammar], Annex B.2 [behavior.undefined], Annex C [formal]

---

### §1.1 Scope [intro.scope]

[1] This clause establishes the boundaries of the Cursive language specification, defines the actors it serves, and identifies complementary material external to the core standard. The scope applies to all implementations that claim adherence to Cursive version 1.0 and later compatible editions.

[2] The specification governs every program element required to compile and execute portable, deterministic Cursive code. The governed areas are:

- lexical structure and translation phases (§2.1–§2.5 [lex.source]–[lex.units]);
- foundational semantic concepts: objects, values, and their correspondence (§1.4 [intro.model]);
- modules, declarations, names and scopes, types, and expressions (Clauses 4–8);
- statements and control flow (Clause 9);
- the generics, permission, contract, and witness systems that ensure memory safety and semantic guarantees (Clauses 10–13);
- concurrency, interoperability, and compile-time evaluation (Clauses 14–16).

[3] The specification intentionally excludes topics whose evolution is decoupled from language conformance. Excluded topics include:

- the normative standard library catalogue (Annex F [library]);
- platform-specific ABI and calling convention details left to implementation policy (§15.6 [interop.abi]);
- concurrency libraries beyond the surface guarantees described in §14.1 [concurrency.model];
- tooling, build systems, or project layout conventions (informative guidance may appear in companion documents).

[4] Certain behaviors are implementation-defined but remain within the specification's boundary. Conforming implementations shall document at least integer and floating-point representations, pointer and region sizes, alignment rules, diagnostic formatting, and resource ceilings consistent with §1.6.3 [intro.conformance.impldef].

[5] Undefined behavior is catalogued centrally in Annex B.2 [behavior.undefined]. Any construct not explicitly marked as undefined shall be diagnosed or defined. Implementers shall ensure that undefined entries referenced in later clauses map back to the Annex catalogue.

#### §1.1.1 Intended Audiences [intro.scope.audience]

[6] Implementers require a precise definition of syntax, typing, evaluation, and diagnostic obligations. Paragraphs marked **Normative** in Clauses 2–16 and Annexes A–B constitute their primary reference set.

[7] Tool developers (formatters, linters, debuggers, analysers) depend on the grammar (Annex A [grammar]), semantic judgments (§1.5 [intro.notation]), and diagnostic contracts (§1.6 [intro.conformance]).

[8] Language designers and researchers rely on the formal notation, meta-theory, and rationale supplied throughout Clause 1 and Annex C [formal].

[9] Programmers seeking educational material should use the informative Programming Guide; Clause 1 provides orientation but does not substitute for tutorials.

#### §1.1.2 Forward References [intro.scope.forward]

[10] Clause 1 references later material only when necessary to state the conformance boundary. Every forward reference is identified explicitly so that implementers can trace dependencies. Circular references are avoided by delegating operational detail to the destination clause.

---

### §1.2 Normative References [intro.refs]

[1] The documents listed here contain provisions that, through citation, become requirements of the Cursive specification. Where a document is identified as normative, conforming implementations shall obey the referenced clauses exactly.

[2] When a referenced standard is revised, the most recent edition applies provided it remains backward compatible with the requirements stated in this specification. Implementations shall document which revision they use when a choice exists.

#### §1.2.1 ISO/IEC and ECMA Standards [intro.refs.iso]

[3] **ISO/IEC Directives, Part 2: Principles and rules for the structure and drafting of ISO and IEC documents (2021).** Establishes the normative vocabulary (`shall`, `should`, `may`, `can`) and organisational conventions adopted in §1.6 [intro.conformance].

[4] **ISO/IEC 10646:2020 — Information technology — Universal Coded Character Set (UCS).** Defines the character repertoire permitted in source text (§2.1 [lex.source]) and informs identifier classifications (§2.3 [lex.tokens]).

[5] **ISO/IEC 60559:2020 (IEEE 754-2019) — Floating-Point Arithmetic.** Governs semantics for binary32 and binary64 arithmetic used throughout the numeric type definitions (§7.2 [type.primitive]) and runtime semantics (§8.3 [expr.operator]).

[6] **ISO/IEC 9899:2018 — Programming Languages — C.** The foreign-function interface (§15.1 [interop.ffi]) relies on C definitions of object representation, calling conventions, and variadic semantics.

[7] **ECMA-334 — C# Language Specification (current edition).** Cited solely for the cross-reference and documentation practices mirrored in §1.7 [intro.document].

#### §1.2.2 Unicode and IETF Publications [intro.refs.unicode]

[8] **The Unicode Standard, Version 15.0.0 or later.** Supplies derived properties, normalization forms, and identifier recommendations leveraged by the lexical grammar and diagnostics (§2.1 [lex.source], Annex A.2 [grammar.lexical]).

[9] **Unicode Standard Annex #31 — Unicode Identifier and Pattern Syntax.** Specifies identifier composition rules adopted verbatim in §2.3 [lex.tokens].

[10] **Unicode Standard Annex #29 — Unicode Text Segmentation** and **UAX #44 — Unicode Character Database.** Provide segmentation and property metadata for tokenisation and editor guidance; they apply where §2.2 [lex.phases] references grapheme boundaries.

[11] **RFC 2119** and **RFC 8174** — Key words for use in RFCs. While Clause 1 follows ISO terminology, these RFCs define equivalent terminology for readers familiar with IETF documents and are referenced in §1.6.1 [intro.conformance.keywords].

#### §1.2.3 Platform and ABI References [intro.refs.abi]

[12] **System V Application Binary Interface** (generic and architecture supplements including AMD64 psABI and ARM AAPCS). These documents guide interoperability obligations in §15.6 [interop.abi] and Annex G [portability].

[13] **Microsoft x64 Calling Convention** documentation. Required for conforming implementations targeting Windows platforms; referenced alongside other ABI requirements in §15.6 [interop.abi].

#### §1.2.4 Availability and Access [intro.refs.access]

[14] ISO/IEC and IEEE publications typically require purchase; implementers shall secure legitimate access before claiming conformance. Unicode and IETF materials are freely available online and shall be consulted in their latest release form.

[15] Annex G.1 [portability.platform] summarises how to reconcile conflicts when platform-specific guidance diverges from these normative sources.

---

### §1.3 Terms and Definitions [intro.terms]

[1] The vocabulary below is normative. When a term is defined in this clause it shall carry the same meaning throughout Clauses 2–16 and the annexes. Paragraph numbering resets within each subclause per the organizational template described in §1.7 [intro.document].

#### §1.3.1 General Language Terms [intro.terms.general]

[2] **binding** — the association between an identifier and an entity within a particular scope.

[3] **compilation unit** — the smallest source artifact processed as a whole during translation (§2.5 [lex.units]).

[4] **conforming implementation** — any compiler, interpreter, or analysis tool that meets the requirements of §1.6.2 [intro.conformance.impl] and documents its implementation-defined behavior.

[5] **conforming program** — a program that satisfies the grammar, static semantics, and behavioral requirements outlined in §1.6.4 [intro.conformance.program]; conforming programs shall not rely on undefined behavior.

[6] **declaration** — a syntactic form that introduces a name and determines its category (§5 [decl]).

[7] **diagnostic** — a message issued when a program violates a rule that requires detection (§1.6.5 [intro.conformance.diagnostics]). Diagnostics include errors (rejecting) and warnings (permissive).

[8] **entity** — any value, type, module, predicate, contract, region, or grant that may be named or referenced.

[9] **expression** — a syntactic form that yields a value or place (§8 [expr]).

[10] **statement** — a syntactic form that executes for effects on program state without directly yielding a value (§9 [stmt]).

[11] **scope** — the syntactic region where a binding is visible (§6.2 [name.scope]).

#### §1.3.2 Cursive-Specific Concepts [intro.terms.language]

[12] **binding responsibility mode** — the axis that distinguishes responsible (`=`) and non-responsible (`<-`) initialisation for `let`/`var` bindings (§5.2 [decl.variable]).

[13] **permission** — a capability (`const`, `shared`, `unique`) describing how a value may be accessed (§11.4 [memory.permission]).

[14] **grant** — a token representing authority to perform a capability such as file I/O (§12.3 [contract.grant]).

[15] **grant set** — the finite set of grants required or produced by an expression, preserved throughout typing (§8.8 [expr.typing]).

[16] **grants clause** — the component of a contractual sequent that enumerates required capabilities, appearing before the turnstile in the form `[[ grants_clause |- must => will ]]`. The grants clause lists zero or more grant identifiers (capability tokens) that must be available for the procedure to execute.

[17] **contract** — a user-defined interface declaration that specifies required procedure signatures and associated types (§10.4 [generic.contract]). Contracts are distinct from contractual sequents.

[18] **contractual sequent** — a complete specification of procedure requirements and guarantees, attached to a procedure declaration using the form `[[ grants |- must => will ]]` where `grants` is the grants clause (capability requirements), `must` lists preconditions, and `will` lists postconditions (§12.2 [contract.sequent]). The semantic brackets `⟦ ⟧` (Unicode U+27E6, U+27E7) or their ASCII equivalent `[[ ]]` denote formal specification content, consistent with their use for type value sets throughout this specification. The turnstile `⊢` or its ASCII equivalent `|-` separates the grants clause from the logical implications.

[19] **behavior** — a collection of callable signatures with concrete implementations that types may attach to gain functionality (§10.4 [generic.behavior]). Behaviors provide reusable code and differ from contracts (which specify abstract requirements) by providing procedure bodies. The term "behavior" is distinct from "predicate expression," which refers to boolean-valued expressions used in contractual sequent clauses.

[20] **predicate expression** — a boolean-valued expression used in contractual sequent clauses (`must`, `will`, `where`) to express logical constraints and conditions.

[21] **modal type** — a type whose values transition through named states validated at compile time (§7.6 [type.modal]).

[22] **witness** — runtime evidence that a contract or behavior obligation has been satisfied, or that a modal value is in a specific state (§13.2 [witness.kind]). Witnesses enable dynamic polymorphism through dense pointer representation with vtable-based dispatch.

[23] **region** — a lexically-scoped allocation arena released in strict LIFO order (§11.3 [memory.region]).

[24] **unsafe block** — an explicit region where the programmer assumes responsibility for safety constraints while interacting with raw operations (§11.8 [memory.unsafe]).

[25] **hole** — a placeholder for an inferred type, permission, or grant inserted by programmers or tools, denoted `_?` in various contexts.

[26] **principal type** — in a type inference context, the most general type that satisfies all constraints. When type inference succeeds, the principal type is unique and is a supertype of all other valid types for the expression. Bidirectional type checking (§8.1.6) produces principal types for expressions at inference sites.

#### §1.3.3 Behavioral Classifications [intro.terms.behaviour]

[27] **implementation-defined behavior** — behavior where the specification provides a set of allowed outcomes and requires documentation (§1.6.3 [intro.conformance.impldef]).

[28] **unspecified behavior** — behavior for which multiple outcomes are permitted without documentation, yet each outcome is well-defined (§1.6.6 [intro.conformance.unspecified]).

[29] **undefined behavior (UB)** — behavior for which the specification imposes no requirements; UB entries are enumerated in Annex B.2 [behavior.undefined].

[30] **ill-formed program** — a program that violates a static semantic rule requiring diagnosis (§1.6.5 [intro.conformance.diagnostics]).

[31] **ill-formed, no diagnostic required (IFNDR)** — rare violations that implementations are not obligated to detect (§1.6.5 [intro.conformance.ifndr]).

#### §1.3.4 Symbols and Abbreviations [intro.terms.abbrev]

[32] **ABI** — Application Binary Interface.

[33] **AST** — Abstract Syntax Tree.

[34] **EBNF** — Extended Backus–Naur Form.

[35] **FFI** — Foreign Function Interface.

[36] **IFNDR** — Ill-Formed, No Diagnostic Required.

[37] **LPS** — Lexical Permission System.

[38] **RAII** — Resource Acquisition Is Initialisation.

[39] **UB** — Undefined Behavior.

[40] **UTF-8** — Unicode Transformation Format, 8-bit.

[41] Numerical and symbolic notations used in inference rules are catalogued in §1.5.3 [intro.notation.math].

---

### §1.4 Objects, Values, and the Semantic Model [intro.model]

[1] This section establishes the foundational distinction between **objects** and **values**, the central abstractions in Cursive's semantic model. Understanding this distinction is prerequisite to reasoning about the type system (Clause 7 [type]), memory model (Clause 11 [memory]), and expression semantics (Clause 8 [expr]).

[2] Every well-formed Cursive program operates by creating objects, storing values in those objects, computing new values from existing ones, and eventually releasing object storage according to deterministic lifetime rules. The language guarantees memory safety by ensuring that every object access occurs within the object's lifetime and respects its permission and region constraints.

#### §1.4.1 Objects [intro.model.object]

[3] An **object** is a region of storage with the following intrinsic properties:

- **Type**: Every object has exactly one type that governs its size, alignment, valid operations, and interpretation.
- **Lifetime**: Every object has a bounded lifetime during which the storage is valid; accessing an object outside its lifetime produces undefined behavior.
- **Storage location**: Every object occupies contiguous bytes at a specific address with alignment matching its type's requirements.
- **Identity**: Objects have identity—two distinct objects are distinguishable even when they store equivalent values.

[4] Objects are created by variable bindings (§5.2 [decl.variable]), procedure parameters (§5.4 [decl.procedure]), composite value construction (§8.4 [expr.structured]), region allocation (§11.3 [memory.region]), and other allocation sites specified throughout this specification. Objects are destroyed when their lifetime ends, triggering destructor execution if the type provides one (§11.5 [memory.destruction]).

[5] **Size and alignment.** Every object has a size (number of bytes) and alignment (address constraint). For type $T$, size is denoted $\text{sizeof}(T)$ and alignment is $\text{alignof}(T)$. These properties are determined at compile time based on the type's structure and any representation attributes (§11.6 [memory.layout]).

[6] **Representation.** An object's representation is the sequence of bytes it occupies. Not all byte patterns are valid for all types; reading an invalid representation produces undefined behavior unless explicitly permitted (e.g., when using raw pointers in unsafe blocks, §11.8 [memory.unsafe]).

[7] **Subobjects.** Composite types (records, tuples, arrays) contain subobjects: each field, element, or variant payload is itself an object with its own type, lifetime, and alignment. Subobject lifetimes are nested within their containing object's lifetime. Accessing a subobject after the containing object's lifetime ends is undefined behavior.

**Example 1.4.1.1** (Objects with distinct identity):

```cursive
let x: i32 = 42
let y: i32 = 42
// x and y are distinct objects even though they store equal values
// They have different storage locations and lifetimes
```

**Example 1.4.1.2** (Subobjects in composite types):

```cursive
record Point { x: f64, y: f64 }

let origin = Point { x: 0.0, y: 0.0 }
// `origin` is an object of type Point
// `origin.x` and `origin.y` are subobjects of type f64
// All three objects share the same lifetime (the enclosing scope)
```

#### §1.4.2 Values [intro.model.value]

[8] A **value** is an abstract mathematical entity of a particular type. Values exist independently of storage; they may:

- Reside in an object (stored values)
- Exist transiently during expression evaluation (temporaries)
- Appear only at compile time (constant expressions, type-level computations)
- Never exist at runtime (uninhabited types such as the never type `!`)

[9] The **value set** of a type $T$, denoted $\llbracket T \rrbracket$, is the collection of all abstract values that type $T$ can represent. For example:

$$\llbracket \text{bool} \rrbracket = \{\text{true}, \text{false}\}$$

$$\llbracket \text{i32} \rrbracket = \{n \in \mathbb{Z} \mid -2^{31} \leq n < 2^{31}\}$$

[10] Values participate in three fundamental operations:

1. **Reading** — Interpreting an object's representation as a value of its type
2. **Writing** — Storing a value into an object's storage
3. **Computing** — Producing new values from existing ones through expressions

[11] Reading a value from an object requires that:

- The object's lifetime is active
- The object's representation is valid for its type
- The access respects permission constraints (§11.4 [memory.permission])

Violations produce undefined behavior or compile-time diagnostics depending on what can be proven statically.

**Example 1.4.2.1** (Values without objects):

```cursive
let result = (2 + 3) * 4
// The intermediate values 2, 3, 5 (=2+3), and 4 never occupy objects
// Only the final result 20 is stored in the object named `result`
```

**Example 1.4.2.2** (Value sets):

```cursive
let flag: bool = true            // value ∈ ⟦bool⟧
let count: u8 = 255              // value ∈ ⟦u8⟧
let invalid: u8 = 256            // error: 256 ∉ ⟦u8⟧
```

#### §1.4.3 Object-Value Correspondence [intro.model.correspondence]

[12] An object **stores** a value when its representation corresponds to a member of its type's value set. The interpretation function:

$$\text{interpret}: \text{Bytes} \times \text{Type} \to \llbracket T \rrbracket \cup \{\bot\}$$

maps byte sequences to values, with $\bot$ indicating invalid representations.

[13] Formally, if object $o$ has type $T$ and representation $r$, then $o$ stores value $v$ when:

$$\text{interpret}(r, T) = v \quad \text{and} \quad v \in \llbracket T \rrbracket$$

[14] **Initialization and assignment.** Assignment and initialization write values to objects:

- **Initialization** — The first write to an object, occurring at its creation site
- **Assignment** — Subsequent writes to objects with mutable bindings (`var`) or appropriate permissions

[15] Initialization is mandatory: every object must be initialized before its value is read. Definite assignment analysis (§5.7 [decl.initialization]) enforces this requirement statically.

[16] Assignment to immutable bindings (`let`) is forbidden after initialization. Assignment to mutable bindings (`var`) requires appropriate permissions (§11.4 [memory.permission]). Assignment replaces the object's stored value; when the previous value's type has a destructor, the destructor executes before the new value is written.

#### §1.4.4 Integration with Language Features [intro.model.integration]

[17] The object-value model integrates with language features as follows:

- **Type system (Clause 7 [type])**: Types classify values and determine what operations are permitted; the type system ensures that values stored in objects are members of the object's type's value set
- **Memory model (Clause 11 [memory])**: Objects have storage duration, lifetimes, regions, and permissions that govern when and how they may be accessed
- **Expression semantics (Clause 8 [expr])**: Expressions compute values; some expressions also designate objects (place expressions) that may be read from or written to
- **Declarations (Clause 5 [decl])**: Bindings associate names with objects; the binding responsibility mode (`=` vs `<-`) determines whether a binding assumes cleanup responsibility

[18] These abstractions compose to provide Cursive's memory safety guarantee: well-typed programs that do not use unsafe code cannot produce use-after-free, double-free, or null-pointer-dereference errors.

---

### §1.5 Notation and Conventions [intro.notation]

[1] This subclause formalises the notational systems used throughout the specification. All mathematical and formal notation is written in LaTeX, using inline `$…$` or display `\[ … \]`/`$$ … $$` delimiters.

#### §1.5.1 Grammar Presentation [intro.notation.grammar]

[2] Grammar fragments appear in fenced code blocks using the `ebnf` fence. Productions follow Extended Backus–Naur Form with terminals enclosed in single quotes and non-terminals expressed in `snake_case`.

[3] The following meta-symbols are used:

- `::=` definition;
- `|` alternative;
- `*`, `+`, `?` zero-or-more, one-or-more, and optional;
- parentheses for grouping; brackets for character classes.

[4] Annex A [grammar] consolidates the authoritative grammar. When a section presents a local excerpt, it shall cite the matching Annex production and the excerpt shall remain textually consistent with the Annex.

**Example 1.5.1.1** (EBNF notation):

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

#### §1.5.2 Metavariables [intro.notation.meta]

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

#### §1.5.3 Mathematical Notation [intro.notation.math]

[7] Set membership ($\in$, $\notin$), unions ($\cup$), intersections ($\cap$), and subset relations ($\subseteq$) follow standard mathematical meaning. Logical connectives ($\land$, $\lor$, $\lnot$, $\Rightarrow$, $\Leftrightarrow$) and quantifiers ($\forall$, $\exists$) are also used without further embellishment.

[8] Operational relations are written as $\langle e, \sigma \rangle \to \langle e', \sigma' \rangle$ for small-step reductions, $\langle e, \sigma \rangle \Downarrow \langle v, \sigma' \rangle$ for big-step evaluation, and $\to^{*}$ for the reflexive transitive closure.

#### §1.5.4 Inference Rules [intro.notation.rules]

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

**Example 1.5.4.1** (Inference rule format):

[ Given: $\Gamma$ is a well-formed type environment ]

$$
\frac{\Gamma \vdash e_1 : \text{bool} \quad \Gamma \vdash e_2 : T \quad \Gamma \vdash e_3 : T}{\Gamma \vdash \texttt{if}\ e_1\ \texttt{then}\ e_2\ \texttt{else}\ e_3 : T}
\tag{T-If}
$$

This rule states that an if-expression has type $T$ when the condition has type `bool` and both branches have the same type $T$. The precondition box establishes that $\Gamma$ must be well-formed. The tag `T-If` identifies this as a typing rule for the if-expression.

#### §1.5.5 Typography and Examples [intro.notation.style]

[12] Normative requirements use **bold** emphasis. Newly introduced defined terms appear in _italics_ on first use within a subclause. Language tokens, identifiers, and keywords use `monospace` formatting.

[13] Examples are fenced with the `cursive` info string. Valid and invalid examples may include inline markers `// correct` and `// incorrect` purely for informative purposes. Unless otherwise stated, examples are informative.

[14] Notes, warnings, and rationale blocks follow the bracketed ISO style:

```
[ Note: … — end note ]
[ Warning: … — end warning ]
[ Rationale: … — end rationale ]
```

**Example 1.5.5.1** (ISO bracketed formats):

[ Note: This is informative commentary that clarifies normative requirements without imposing additional constraints. Notes may include implementation guidance or rationale for design decisions.
— end note ]

[ Warning: Violating this constraint results in undefined behavior [UB-ID: B.2.15]. Implementations are not required to diagnose this condition.
— end warning ]

[ Rationale: This design choice ensures type safety while maintaining zero-cost abstraction. Alternative approaches were considered but rejected due to runtime overhead concerns.
— end rationale ]

These three formats distinguish informative notes, safety-critical warnings referencing the undefined behavior catalog (Annex B §B.2), and design rationale respectively.

#### §1.5.6 Cross-References [intro.notation.refs]

[15] In prose, cross-references use the pattern `§X.Y[label]`. Within Clause 1 the stable labels defined in §1.7 [intro.document] apply. Digital versions shall hyperlink every cross-reference.

---

### §1.6 Conformance [intro.conformance]

[1] This subclause defines what it means to conform to the Cursive specification. Conformance applies both to implementations and to programs.

#### §1.6.1 Normative Vocabulary [intro.conformance.keywords]

[2] The keywords **shall** and **shall not** express absolute requirements and prohibitions. **Should** and **should not** express strong recommendations; implementations may deviate provided the deviation is documented and justified. **May** and **may not** describe permissions. **Can** and **cannot** describe capability, and are informative. These meanings follow ISO/IEC Directives, Part 2 and mirror RFC 2119 terminology.

[3] When a paragraph is marked Informative it contains guidance, examples, or rationale that does not impose requirements. Notes, warnings, and rationale blocks are informative by default.

#### §1.6.2 Conforming Implementations [intro.conformance.impl]

[4] An implementation conforms if and only if it satisfies all of the following conditions:

- **Acceptance**: It shall accept every program that satisfies the rules of §1.6.4 [intro.conformance.program].
- **Rejection**: It shall issue a diagnostic for any ill-formed program (§1.6.5 [intro.conformance.diagnostics]).
- **Semantics**: It shall implement the runtime behavior described in Clauses 2–16 for well-formed programs that avoid undefined behavior.
- **Documentation**: It shall document every implementation-defined behavior identified in §1.6.3 [intro.conformance.impldef].
- **Extension safety**: Extensions shall not alter the meaning of conforming programs nor mask diagnostics that the base language would require.

[5] Conforming implementations may provide additional tools, optimisations, and analyses. Such features shall be disableable when they would otherwise change language semantics.

#### §1.6.3 Implementation-Defined Behavior [intro.conformance.impldef]

[6] Implementation-defined behavior occurs when this specification allows multiple outcomes but requires that an implementation choose one deterministically and document it. The following categories are implementation-defined:

- Numeric representation widths beyond mandated minima (§11.6 [memory.layout]);
- Pointer and region size, alignment, and layout details (§11.6 [memory.layout]);
- Resource ceilings such as maximum recursion depth or compilation unit size;
- Diagnostic formatting and display conventions (§1.6.5 [intro.conformance.diagnostics]);
- ABI choices for each supported platform (§15.6 [interop.abi]).

[7] Implementations shall publish these choices in user-facing documentation and apply them consistently.

#### §1.6.4 Conforming Programs [intro.conformance.program]

[8] A program is conforming when it:

- Satisfies the grammar of Annex A and the static semantics of Clauses 2–12;
- Abides by all contract, grant, permission, and modal requirements (§12–§14);
- Avoids undefined behaviour as catalogued in Annex B.2;
- Respects implementation-defined limits documented by the chosen implementation;
- Indicates the language edition it targets when required by the build manifest (§1.8 [intro.versioning]).

[9] Conforming programs may rely on unspecified behaviour, but shall not assume a particular outcome when the specification leaves choices open.

#### §1.6.5 Diagnostics and IFNDR [intro.conformance.diagnostics]

[10] A program that violates a static semantic rule is _ill-formed_ and shall provoke a diagnostic. Diagnostics shall include at minimum an error code, source location, and a brief description of the violation.

[11] Some violations are classified _ill-formed, no diagnostic required_ (IFNDR). These cases are documented explicitly at the point of definition; implementations are encouraged, but not required, to diagnose them. IFNDR is reserved for situations that are impractical to detect statically, such as exceeding translation limits that are only observable post-link.

[12] Diagnostic presentation style is implementation-defined, yet tools should strive for clarity and actionable guidance. Informative advice, suggested fixes, and machine-readable output formats are quality-of-implementation features, not conformance requirements.

#### §1.6.6 Behavioural Classifications [intro.conformance.unspecified]

[13] **Unspecified behaviour** permits multiple well-defined outcomes without documentation. **Undefined behaviour** confers no guarantees; once triggered, any result is permitted, and the program ceases to be conforming. Undefined behaviour sites are labelled `[UB: B.2 #N]` and mapped to Annex B.2 [behavior.undefined].

[14] Distinguishing these categories enables implementers to provide stronger diagnostics and enables programmers to reason about portability.

---

### §1.7 Document Conventions [intro.document]

[1] This subclause prescribes the structural conventions used throughout the specification: stable labels, cross-references, example formatting, and validation expectations. Digital and print editions shall follow the same logical structure.

#### §1.7.1 Stable Labels [intro.document.labels]

[2] Each clause and subclause carries a stable label of the form `[clause.topic[.subtopic]]`. Clause 1 uses the following labels:

- `[intro.scope]`, `[intro.refs]`, `[intro.terms]`, `[intro.model]`, `[intro.notation]`, `[intro.conformance]`, `[intro.document]`, `[intro.versioning]`.
- Subclauses append descriptive suffixes, e.g. `[intro.document.labels]` or `[intro.conformance.impl]`.

[3] Later clauses follow the same style: clause prefix (e.g., `lex`, `decl`, `memory`) plus a concise topic. Labels are unique and stable across editions. When content moves, the label moves with it to preserve hyperlinks.

#### §1.7.2 Cross-Reference Format [intro.document.references]

[4] Cross-references shall use the format `§X.Y[label]`. For example, "§5.2 [decl.variable]" references the variable binding rules in Clause 5. Within the same clause the leading clause number may be omitted when unambiguous (`§1.6 [intro.conformance]`).

[5] Digital publications shall hyperlink the rendered text to the referenced section. PDF editions should provide bidirectional navigation where feasible.

[6] Forward references are permitted when necessary to inform the reader about required prerequisites. Authors shall explicitly mark the forward reference (e.g., "Forward reference: §12.4 [memory.permission]"). Cycles should be avoided; when unavoidable they must be documented in both target sections.

#### §1.7.3 Validation Requirements [intro.document.validation]

[7] Prior to publication, editors shall verify that every label resolves to an existing target and that no dangling references remain. Automated tooling should generate:

- an outbound reference table listing, for each section, the labels it references;
- an inbound reference table to identify orphans (sections with no references);
- a report for unresolved labels or malformed cross-references.

[8] Failing references shall block publication. Errata releases may fix discovered issues; patch releases (e.g., 1.0.1) shall include updated validation artefacts.

#### §1.7.4 Examples and Annotations [intro.document.examples]

[9] Examples are informative unless clearly marked otherwise. Code examples use the `cursive` fence. Diagnostics may annotate expected errors using comments (`// ERROR E4001: …`). When examples demonstrate invalid code the surrounding prose shall identify the normative rule being violated.

[10] Notes and warnings employ the bracketed ISO format described in §1.5.5 [intro.notation.style]. Informative commentary shall never contradict normative rules.

#### §1.7.5 Document Metadata [intro.document.metadata]

[11] Each file includes navigation metadata (**Previous**, **Next**) to aid editors working in split files. These links are informative; they shall mirror the logical order determined by Clause 1 but impose no additional requirements on implementations.

[12] Version history entries belong in dedicated change logs (Annex H [changes]). Individual clause files should reference the change log instead of embedding historical commentary.

#### §1.7.6 Diagnostic Code Format [intro.document.diagnostics]

[13] All diagnostic codes in this specification shall use the canonical format `E[CC]-[NNN]`, where:

- `E` denotes an error diagnostic;
- `[CC]` is a two-digit clause number with leading zero (01, 02, ..., 16);
- `-` is a hyphen separator for visual clarity;
- `[NNN]` is a three-digit sequential number with leading zeros (001, 002, ..., 999).

**Example**: `E02-001` indicates the first diagnostic in Clause 02 (Lexical Structure and Translation).

[14] Each clause allocates sequential diagnostic codes starting from 001. Within a clause, diagnostic codes should be allocated by subsection to facilitate maintenance and avoid conflicts. For instance, Clause 07 (Type System) may reserve:

- `E07-001` through `E07-099` for §7.1 (Type foundations);
- `E07-100` through `E07-299` for §7.2 (Primitive types);
- `E07-300` through `E07-499` for §7.3 (Composite types);
- and so forth.

[15] Implementations shall report diagnostic codes using this exact format in compiler output, error messages, and diagnostic payloads. The format facilitates:

- **Clause identification**: The two-digit prefix immediately identifies which specification clause defines the error.
- **Visual parsing**: The hyphen separator aids human readers in distinguishing clause from sequential number.
- **Machine processing**: The regular pattern `E\d{2}-\d{3}` enables reliable automated parsing and validation.

[16] Reserved ranges within a clause may be documented to accommodate future expansion of specific subsections without renumbering existing diagnostics. Implementations should not define diagnostic codes outside the ranges specified by this document.

[17] Annex E §E.5 [implementation.diagnostics] provides the authoritative diagnostic code registry, including:

- Complete enumeration of all diagnostic codes across all clauses;
- Structured payload schemas for machine-readable diagnostic output;
- Suggested diagnostic message templates and quality guidelines;
- Severity levels (error, warning, note) for each code.

[18] Deprecated diagnostic codes from earlier specification versions shall be documented in the migration mapping (Annex H [changes]). Implementations may recognize legacy codes for backward compatibility but shall normalize them to the canonical format in output.

**NOTE 1**: The two-digit clause prefix supports clauses up to 99. Current specification planning does not anticipate exceeding 20 clauses.

**NOTE 2**: The three-digit sequential number provides 999 codes per clause. If a clause exhausts its allocation, the specification editors should consider subdividing the clause or consolidating related diagnostics.

**NOTE 3**: Warning and note diagnostics may use prefix `W` or `N` in future versions (e.g., `W02-001`, `N02-001`). Currently, all diagnostics use the `E` prefix.

---

### §1.8 Versioning and Evolution [intro.versioning]

[1] Cursive follows semantic versioning (`MAJOR.MINOR.PATCH`). This subclause describes the meaning of each component and the obligations imposed on implementations and programs as the language evolves.

#### §1.8.1 Version Identifiers [intro.versioning.identifiers]

[2] The specification defined herein corresponds to version **1.0.0**. Minor revisions (`1.x.0`) add backwards-compatible features; patch revisions (`1.0.x`) provide errata and clarifications without changing behavior. Major revisions (`2.0.0`, `3.0.0`, …) may introduce breaking changes but shall follow the migration process in §1.8.4 [intro.versioning.deprecation].

[3] Projects shall declare a minimum required language version in their manifest or build configuration. If no version is declared, implementations may default to the latest supported minor release within the highest major version they provide (for example, 1.0 defaults to 1.latest within the 1.x series).

#### §1.8.2 Implementation Support [intro.versioning.impl]

[4] Conforming implementations shall document the set of language versions they support, including the default compilation version. When multiple versions are supported simultaneously, the implementation shall expose a mechanism (flag, configuration value, or project manifest entry) that selects the desired version.

[5] Preview or experimental versions may be offered, but they shall require explicit opt-in (e.g., `--language-version 2.0-preview`). Preview behavior shall never silently override stable semantics.

#### §1.8.3 Feature Stability Classes [intro.versioning.features]

[6] Features fall into three stability classes:

- **Stable**: Available by default in all minor releases of the current major version; breaking changes require a major release.
- **Preview**: Opt-in features that are intended for the next major release. Preview features shall be tagged `[Preview: target_version]` in the specification.
- **Experimental**: Highly unstable features guarded by feature-control directives. Experimental features may change or disappear without notice and shall never be enabled implicitly.

[7] Implementations shall clearly report when preview or experimental features are enabled. Tooling should emit warnings reminding users of the stability level.

#### §1.8.4 Deprecation and Removal [intro.versioning.deprecation]

[8] A feature slated for removal shall first be marked **deprecated** in a minor release. Deprecation notes include the planned removal version, rationale, and migration guidance. Deprecation lasts for at least one minor release before removal.

[9] Removal occurs only in a major release. Implementations shall issue diagnostics in the new major version when code relies on removed features and should provide migration hints. Automated migration tooling is a recommended, but not mandatory, quality-of-implementation feature.

[10] Deprecated features remain fully functional until removal but may trigger warnings. Programs may suppress such warnings only through documented means that do not mask other diagnostics.

#### §1.8.5 Errata and Patch Releases [intro.versioning.patch]

[11] Patch releases address specification defects, typographical errors, or clarifications that do not change behavior. When errata alter normative text, the change log in Annex H [changes] shall record the revision, rationale, and impacted sections.

[12] Implementations are encouraged to adopt patch releases promptly. When behavior diverges from a published patch release, implementations shall document the discrepancy and, if possible, provide compatibility modes.

#### §1.8.6 Project Manifest Schema [intro.versioning.manifest]

[13] Every project shall provide a UTF-8 encoded manifest file named `Cursive.toml` at the workspace root. The manifest shall conform to TOML 1.0 (ISO/IEC - ISO/IEC DIS 30170:2022) so that tooling can parse it deterministically.

[14] The manifest shall contain a `[cursive.language]` table with a `version = "MAJOR.MINOR.PATCH"` entry that names the language version targeted by the project. The version string shall follow the semantic-versioning grammar defined in §1.8.1 and shall be present even when the implementation also accepts command-line overrides.

[15] The manifest shall declare at least one source root using the `[cursive.source]` table with a `roots = ["relative/path", ...]` array. Each entry shall be a POSIX-style relative path rooted at the manifest directory, shall resolve to an existing directory, and shall be unique after path normalisation. Clause 4 derives module paths exclusively from these roots.

[16] Implementations shall issue diagnostics when the manifest is missing, malformed, omits the `language.version`, or provides an empty `roots` array. Implementations may extend the manifest with additional tables provided that the `cursive.language` and `cursive.source` tables remain intact and their semantics are not altered.

---

**Previous**: _(start)_ | **Next**: Clause 2 — Lexical Structure and Translation (§2 [lex])
