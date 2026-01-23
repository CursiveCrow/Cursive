Finalized Outline – Cursive Language Specification (v1.3.0)

This outline defines the complete structure for the Cursive language specification. It integrates the best aspects of the proposed outlines and follows industry best practices. All main spec files reside in the Spec/ directory, using a sequential two-digit numbering (01–16) that aligns with existing documents for stable cross-references. Each specification chapter is organized into logical sections (Overview, Syntax grammar, Static Semantics rules, Dynamic Semantics behavior, Algorithms, etc.) to ensure clarity and maintainability. This structure proceeds from fundamental lexemes through type system and execution, ending with advanced topics and appendices, mirroring the logical flow of language specs like C and Go. Large chapters (e.g. Contracts) are subdivided into sub-files for ease of navigation and normative referencing.

Part I: Foundations

File: Spec/01_Foundations.md (Foundational Concepts and Notation)
Contents: This chapter introduces the spec’s purpose, notation, and basic building blocks of the language. It consolidates front-matter material so that later parts can reference common definitions. Required sections:

1. Title, Conformance, and Abstract: Provides the specification’s title, version, status, target audience, and a brief abstract of the language’s purpose. Defines conformance criteria and interprets RFC 2119 keywords (“MUST”, “SHALL”, etc.) for requirement levels. Also describes document conventions (notation, fonts, etc.).

2. Notation and Mathematical Foundations: Defines the formal notation used throughout the spec. This includes metavariables (e.g. identifiers for variables, types, expressions), judgment forms (typing judgments Γ ⊢ e: T, evaluation judgments ⟨e, σ⟩ ⇓ ⟨v, σ'⟩, contract triples {P} e {Q}, etc.), and mathematical symbols (set notation, logical operators, substitution [x ↦ v], etc.). Explains the format of inference rules and how to read them, with examples. These conventions underpin the static and dynamic semantics in later chapters.

3. Lexical Structure: Specifies the Syntax of source text elements at the character/token level. Defines the Unicode character set (UTF-8 encoding, handling of BOM, line terminators) and how source files are composed. Describes comments (line //…, block /_…_/, doc comments), whitespace, and how tokens are formed (identifiers, keywords, literals, operators). Lists all keywords (both reserved and contextual) and other delimiters. Discusses identifier syntax and naming rules (allowed characters, case sensitivity, reserved identifiers). Defines numeric literals (integer in decimal/hex/binary/octal, floating-point notation), boolean literals, character and string literal forms (including escape sequences). Describes the statement termination rules – Cursive uses newline-sensitive semicolons with specific continuation rules (four rules for when a newline does not end a statement). Explains how the lexer uses maximal munch to tokenize input. (The formal lexical grammar is included in the appendix but summarized here for clarity.)

4. Abstract Syntax: Presents the core abstract syntax trees (AST) that the language compiles to, connecting concrete syntax to formal rules. Introduces the abstract syntax categories for types, expressions, patterns (for matching/destructuring), values (runtime values of expressions), assertions (contract conditions), and effects. Describes each category briefly and shows how they relate (e.g. an expression can be a literal, a variable, a binary operation, etc.). Also defines well-formedness constraints at the AST level if any (e.g. what constitutes a valid AST node). This section provides a bridge between the concrete grammar and the formal semantic rules.

5. Attributes and Annotations: Defines the syntax for attributes ([[...]]) and how they annotate declarations or types. Lists core attributes built into the language, with their meaning: e.g. [[repr(...)]] for controlling memory layout, [[verify(...)]] to set verification mode (static/runtime/none), [[overflow_checks(...)]] to configure integer overflow behavior, [[module(...)]] for module metadata, [[alias(...)]] to give alternate tool-friendly names, [[inline]]/[[no_inline]] hints for inlining, [[no_mangle]] for preserving symbol names, [[deprecated(...)]] to mark deprecations, [[must_use]] to require using a value, and optimization hints like [[cold]] or [[hot]]. Also mention that user-defined attributes are reserved for future use (if applicable). Specifies where attributes can appear in the syntax and that they do not inherit to sub-elements. Describes how conflicting attributes are detected and how attributes are processed by the compiler (e.g. some might affect code generation, others only for linter/analysis).
   Purpose: Part I establishes terminology and notation, ensuring that readers and the spec itself have a common foundation. By consolidating lexical syntax and formal conventions here, later parts can reference “as defined in Part I” for brevity. This chapter should be verified for completeness and consistency with existing onboarding material.

Supplementary: Spec/00_LLM_Onboarding.md (LLM/AI Onboarding Guide) – This is a non-normative quick-reference for AI assistants. It provides a high-level summary of Cursive’s syntax and concepts (for example, a condensed EBNF and a cheat-sheet of key features) to aid language model interactions. This file is kept separate from the formal spec and updated as needed alongside spec changes.

Part II: Type System

File: Spec/02_Type-System.md (Types and Type Properties)
Contents: This chapter defines all data types of Cursive, their syntax, and static semantics. It is one of the largest sections of the spec (covering primitives, composites, generics, etc.). Each category of types is presented with its grammar and semantics. Required sections:

1. Type System Overview: An Overview of Cursive’s type system design and terminology. Describes the taxonomy of types (primitive vs. compound vs. abstract vs. permission-indexed) and general properties like copyability and size. Introduces concepts of subtyping or type equivalence rules (e.g. name equivalence for nominal types, structural equivalence for others), and how type identity is determined. Mentions that certain type qualifiers (like permission modifiers own/mut/imm) are integrated into types. If the type system relies on a kind hierarchy (for example, kinds for types, effects, regions), that is noted here. This section sets context for the detailed sections that follow.

2. Type Foundations (Formal Basis): (If needed for formality.) Presents the formal underpinnings of the type system. This includes the Syntax of type-related grammar (EBNF for type expressions, type parameters, etc.) and definition of typing contexts. It defines well-formedness rules for types and type judgments (e.g. the form of the typing relation Γ ⊢ e: T). Introduces any special kind system (e.g. if types of effects or regions have their own kind) and variance rules. Defines subtyping (T₁ <: T₂) if the language supports it, and type equivalence (T₁ ≡ T₂) rules (for instance, when an alias is considered the same type as its target). Also includes the formal laws for the effect system if applicable: for example, if effects form a lattice or monoid (idempotence, commutativity, etc.), those algebraic laws are stated here for reference. (This formal section ensures that the type system’s foundations are rigorously specified for later static semantics and soundness proofs.)

3. Primitive Types: Lists all built-in scalar types and their definitions. This includes:

Integer types (i8, i16, i32, i64, isize and unsigned u8…u64, usize): For each, specify size (in bits) and range. Note which are machine-dependent (e.g. isize/usize depending on pointer width). State default literal type (e.g. integer literals default to i32 unless suffixed) and behavior on overflow (e.g. wraparound, panic, or undefined – Cursive likely specifies defined overflow behavior or uses checked arithmetic by default).

Floating-point types (f32, f64): Confirm IEEE 754 compliance and precisions. Describe literal syntax (e.g. decimal and exponential notation).

Boolean type (bool): two values true and false. Clarify that booleans may not implicitly convert to integers (if that’s the case).

Character type (char): Represents a Unicode scalar value (32-bit). Explain literal notation (e.g. 'A', '🦀').

The never type (!): An uninhabited bottom type, used for functions that never return (diverging). Explain its role in the type system (can coerce to any type, etc.).

String types: Describe str (perhaps an immutable string slice, e.g. UTF-8 view into memory) versus String (an owning, heap-allocated UTF-8 string). Discuss that str is unsized (cannot be a standalone variable, only behind a pointer) whereas String is a growable owned buffer.

Unit type (): A singleton type with a single value (), often used for functions with no meaningful return.
(Each primitive type definition should include any special semantics or default values; e.g. integers default to 0, bool default false, etc., as the “zero value” for that type.)

4. Composite Types: Describes compound data types that aggregate other types:

Arrays ([T; N]): Fixed-length compile-time-sized arrays. Define that the length N is part of the type. Describe how indexing works (an [index] expression yields an lvalue of type T), and mention bounds checking (if out-of-range access is a runtime error). Mention memory layout (contiguous, element i at offset i \* sizeof(T)), and that copy/assignment of arrays copies all elements (unless optimized or if type is non-copyable, etc.).

Slices ([T] or perhaps slice<T>): Dynamically sized views into an array or heap memory. Explain that a slice isn’t a first-class type on its own for variables (unless Cursive supports dynamically sized types) but is typically used behind a pointer (e.g. &[T]). Describe representation (likely a fat pointer: data pointer + length). Cover slice operations (like slicing syntax arr[a..b]) and that bounds are checked.

Tuples ((T₁, T₂, ..., Tₙ)): Ordered heterogenous product types. Note that the 0-tuple is the unit type (). Describe tuple construction syntax and accessing tuple elements (perhaps via pattern matching or special .0 .1 syntax if provided). Memory layout: contiguous sequence of fields with possible padding (could reference that layout rules are in the Memory chapter).

Records (Structs) (record Name { fields }): Nominal product types with named fields. Explain record declaration syntax (possibly with optional visibility on fields), how an instance is constructed (using field name initializers), and how fields are accessed (obj.field). Discuss default field values if any, and whether records can have type‑scope procedures attached (though method behavior is detailed under Functions). Mention alignment and padding (but details in Memory model section). Also note field visibility rules (if some fields can be private to a module).

Unions (if supported): A type where only one of several fields is valid at a time (like a C union). If Cursive has union types or a similar sum type by another name, describe syntax and that the active field must be tracked manually (since language might not enforce it). If not supported, this section may be omitted or just mentioned as not present.

Enumerations (Enums) (enum Name { Variant1, Variant2, ... }): Sum types with discrete variants (tagged unions). Describe the syntax for defining enums and their variants. Variants may be simple (no data), tuple-like, or record-like (with payload fields). Explain how enum values are constructed and matched (detailed matching semantics in the Expressions/Pattern Matching chapter). Define whether an enum’s variants have implicit discriminant values (e.g. 0, 1, …) and if those can be specified explicitly. Mention that all variants together cover the type, and how exhaustive pattern matching is enforced. Memory layout: describe how an enum is stored (e.g. a tag plus payload, possibly union-like) and how size is determined (size = max size of any variant plus tag).
(By the end of composite types, the reader should have grammar and basic semantics for constructing and using arrays, slices, tuples, records, (unions), and enums.)

5. Modal Types (Stateful Types): Cursive-specific feature. Describes the modal state system integrated into types (stateful type annotations for state machines). Explain the syntax for declaring states (perhaps via @State declarations or similar) and how a type can be annotated with a state: e.g. T@S meaning an instance of type T currently in state S. Define how state transitions are expressed (a special arrow operator or in procedure signatures). Discuss compile-time verification rules that prevent invalid state transitions or using an object in the wrong state (the compiler checks that a value of type T@S1 is only used in contexts allowed for S1, and after a state transition it is treated as T@S2). Summarize how modal procedures can change the state of their receiver (for example, a procedure signature might indicate it consumes an object in state S1 and produces it in state S2). This section gives an overview; specific rules for state transition are also referenced in the Functions and Contracts parts if needed.

6. Abstraction Types: Covers interfaces and code reuse abstractions in Cursive:

Behavioral Contracts (Interfaces): Describes the contract construct, which defines an abstract interface (a set of procedure signatures with contract clauses but without implementations). Explain that contracts cannot have concrete code – they specify required behaviors (preconditions, postconditions, effects) that implementing types must fulfill. Outline how a type implements a contract (e.g. by providing procedures matching the signatures and meeting the contract conditions), and how contract satisfaction is checked (including rules like precondition weakening and postcondition strengthening when overriding implementations). Mention whether multiple contract inheritance is allowed and how conflicts are resolved. Also cover associated types in contracts if the contract can define type members that implementations must provide.

Traits (Concrete Mixins): If Cursive has a separate notion of concrete code reuse (called “traits”), describe it. A trait in this context might be similar to a mixin or default method interface: it can provide procedure implementations that types can adopt. Explain trait declaration syntax and how a trait is attached to a type (or how a type “adopts” a trait’s methods). Clarify the difference between contracts and traits (traits carry code and possibly state, contracts are purely abstract specifications). Cover any rules on trait usage (e.g. coherence rules to prevent conflicting trait implementations, or orphan rules restricting where traits can be defined for types to avoid overlap).
Both contracts and traits should be clearly delineated since they address distinct needs – one for interface specification (with no implementations in the spec), and one for code reuse (with default implementations).

7. Parametric Types (Generics): Explains generic type parameters and how parametric polymorphism works in Cursive. Topics include:

Generic Type Parameters: Syntax for declaring type parameters on types or procedures (e.g. Name<T, U>). Discuss constraints/bounds on type parameters (like a type parameter implementing a certain contract or trait, if any).

Const Generics: If the language allows compile-time constant parameters (e.g. a generic parameter that is a value like an integer for array length), describe the syntax (<const N: usize> for example) and how those integrate into types.

Generic Instantiation: Describe how generics are used – e.g. via monomorphization at compile time or a single instantiation (depending on implementation strategy). The spec should define that for each set of argument types, a logically distinct instance is created if monomorphized. If any form of specialization or overload resolution for generics exists, mention it here or in an advanced section.

Where Clauses: Syntax for attaching additional constraints on generics (e.g. where T: SomeContract). These constraints often include contract implementations or traits that T must satisfy. Explain that these constraints act as assumptions within the generic body and obligations on any instantiation.

Type Aliases and New Types: Syntax for defining a new type name (alias) for an existing type, e.g. type Alias = ExistingType. Clarify whether this creates an actual distinct type (in some languages a newtype is distinct and not assignment-compatible with the original) or simply an alias interchangeable with the original. If Cursive supports both alias and newtype semantics, explain each. Also cover associated types as part of generics/contracts (if not already discussed under contracts).

(Static semantics: the spec should include typing rules ensuring generic type parameters and arguments match, but these details might be left to the static semantics rules section or Part XI on operational semantics for how generics are expanded. Mention at least that all generic parameters must be satisfied by appropriate types or const values at instantiation time.)\*

8. Function Types (Function Types): Describes the type of functions/procedures in the language. In Cursive, function types are written as (T₁, T₂) → U ! ε (indicating a function from T₁,T₂ to U with effect set ε).

9. Permission Types (Ownership Types): Introduces Cursive’s Lexical Permission System (LPS) for memory safety. This overlays the base types with qualifiers own, mut, imm (or an implicit default immutable). Topics:

The three permission modifiers: own T for exclusive ownership (exactly one binding owns the value, responsible for freeing it), mut T for a mutable reference (aliasable, multiple can exist), and imm T (or just T by default) for immutable references (read-only, unlimited aliases).

Explain uniform pass-by-permission: all function parameters and assignment operations carry the permission of the value (so you can’t implicitly copy-own a value without transferring ownership).

Move semantics: using move on an expression yields an owned value, transferring ownership out and invalidating the original variable. Contrast with copy: values do not copy implicitly unless they implement a Copy trait; by default, passing an own T moves it.

Permission rules: Document the key safety rules enforced at compile time: Unlike Rust, multiple mutable references are allowed concurrently in Cursive (no exclusive mutable borrow rule), which is a design difference. Owned values cannot be aliased (only one owner). Immutable references can be freely aliased. Describe any conversions allowed (e.g. an own T can be temporarily lent as mut T or imm T?). If the language infers permissions or allows coercions (like auto-coercing own T to imm T when needed), state those rules. Also highlight differences from Rust’s borrow checker (e.g. Cursive’s runtime or region-based approach in contrast).

Copy trait: Mention that types must explicitly implement a Copy marker trait to be copyable; otherwise, they are move-only. Copyable types can be duplicated without move semantics. This ties into permissions because a copy is effectively a new own instance.
(This section primarily concerns Static Semantics – the compiler ensures these permission rules – but some aspects (like actual deallocation) are in the Memory Model part. Here we focus on the type-level view of permissions.)

10. Reference Types (Pointers): Details pointers and references, which are types that refer to values in memory.

Safe Pointers (Ptr<T>@State): If Cursive has a notion of a managed pointer with a state (perhaps as part of its modal system), describe it. This could mean a pointer that carries an associated state token (for example Ptr<File>@Open meaning a pointer to a File in the Open state). Explain how these are obtained and used safely – likely through the type system and effect system (ensuring you can’t dereference a pointer outside its valid state).

Raw Pointers (*T or *mut T): Unsafe, unstructured pointers. Describe their syntax and that they bypass the permission system (thus only allowed in unsafe contexts). Include rules such as: they may be null (if null exists as a constant), pointer arithmetic is permitted only on raw pointers (if at all) and must be done in unsafe. Clarify any constraints (e.g. pointer arithmetic perhaps only within an allocated object’s bounds, though not checked). Raw pointers are not implicitly convertible to safe references.

Lifetimes/Nullability: Note how null is handled – e.g. perhaps \*T can be null, whereas safe references (like own/mut/imm) cannot be null. If there’s a special Option<T> type for nullability, mention that (though details in Error Handling maybe).

If the language imposes any lifetime or aliasing constraints on pointers beyond permissions (for example, no dangling pointers because regions reclaim memory in bulk), mention that here or reference the Memory Model chapter.

11. Type Inference: Describes how the compiler infers types when they are not explicitly written. Explain that local let bindings can omit type and will have their type inferred from the initializer. Summarize the inference algorithm at a high level (likely a unification or flow-based algorithm). Note any limitations of inference – e.g. no global type inference for function signatures (those must be annotated), or that certain complex cases require manual annotation to resolve ambiguities. Also mention if generic parameter types can be inferred from call context. This section might also point out that in error messages or in the spec rules, metavariables represent unknown types solved by inference.
    Purpose: The Types chapter provides the normative definition of all forms of types and how they behave, which is crucial for both the Static Semantics (type-checking rules) and understanding runtime representation. Each subsection should include grammar productions for defining that type, static rules for type compatibility, and any runtime considerations (though detailed memory layout is in Part IX). Consistent structure (Syntax → Semantics) for each type form helps maintain clarity. The content here should be expanded from the existing scaffold in 02_Types.md and cross-checked with any older type system documents to ensure nothing is missing. All type categories unique to Cursive (like modal and permission types) are prominently documented.

Part III: Declarations and Scope

File: Spec/03_Declarations-and-Scope.md (Name Introduction, Scoping, and Visibility)
Contents: This chapter specifies how names are introduced into scopes, how scoping works across different levels (block, function, module), and how visibility controls access across module boundaries. It establishes the foundational rules for all name introduction and resolution in Cursive. This chapter MUST come before Expressions because expressions reference declared names. Required sections:

1. Declarations Overview: Overview of the declaration system, including taxonomy of all six declaration forms (variables, types, functions, procedures, contracts, traits), declaration levels (module-scope, function-scope, block-scope), two-phase compilation model enabling forward references, and the two-keyword binding model (let/var only, no const keyword). Introduces the four key design rules: Single-Binding Rule, Explicit Shadowing, Uniform Binding Invariants, and Permission-Aware Parameters.

2. Variable Declarations: Complete specification of variable bindings using the two-keyword model. Covers let (immutable binding) and var (mutable binding) with explicit statement that const keyword does NOT exist in Cursive. Includes:

Single-Binding Rule: Formal rule preventing redeclaration in same scope (E3D01). A name may be introduced at most once per lexical scope.

Explicit Shadowing: Formal rules requiring shadow keyword for shadowing outer bindings (E3D02), with validity conditions (E3D07) and namespace conflict prevention (E3D03). Includes [Decl-Explicit-Shadow], [Decl-Implicit-Shadow-Error], [Decl-Invalid-Shadow], and [Decl-Namespace-Conflict] formal rules.

Uniform Binding Invariants: Definition clarifying that binding mutability (let/var) is orthogonal to value mutability (controlled by permissions). Binding immutability ≠ value immutability. Includes examples with modals, permissions, and reassignment.

Global vs Local Variables: Module-scope variables have static storage; function/block-scope have automatic storage. Global let bindings become compile-time constants if evaluable.

Definite Assignment: Complete algorithm with path dominance analysis ensuring variables are initialized on all paths before use (E3D06). Includes definite_assignment_check pseudocode.

Assignment to Immutable: Rule preventing assignment to let bindings (E3D10) with diagnostic and examples.

3. Compile-Time Evaluation: Specifies context-driven evaluation replacing const keyword. A let binding becomes compile-time constant when: (1) declared at module scope AND expression is compile-time evaluable, OR (2) declared inside comptime block. Includes complete list of compile-time evaluable expressions (literals, arithmetic, comptime function calls, references to other compile-time constants, type expressions). Specifies type position requirements (Rule 3.3.2) with formal rules [Type-Position-Const] and [Type-Position-Error] for E3D08 diagnostic. Examples contrast global vs local scope, comptime blocks, and generic parameters.

4. Type Declarations: Overview of type declaration forms (records, enums, modals, type aliases), cross-referencing Part II for full type system. Covers type alias transparency, newtype pattern for distinct types, forward references via pointer indirection, and type visibility modifiers. Establishes that types cannot be stored in let/var bindings (types are not values).

5. Function and Procedure Declarations: Overview establishing that functions/procedures introduce names in module scope. Local function declarations are NOT PERMITTED (E3D11). Specifies Permission-Aware Parameters (Rule 3.5.1) with complete Param grammar showing own/mut/imm modifiers and default imm. Defines Parameter Scope (Rule 3.5.2): parameters occupy function's outermost local scope and cannot be shadowed in that scope. Cross-references Part VII for full function syntax.

6. Contract Declarations: Overview establishing contracts introduce abstract interface types in module scope, cannot be instantiated, support forward references. Cross-references Part V for full contract system.

7. Scope Rules: Complete scope hierarchy specification (block, function, module scopes). Formal Shadowing Rules (Rule 3.7.1) with [Scope-Shadow-Valid] and [Scope-No-Redeclare] inference rules. Namespace Unification (Rule 3.7.2) establishing single namespace for types and values per scope. Includes diagnostics E3D01, E3D02, E3D03, E3D07.

8. Import and Use Declarations: Complete UseDecl grammar (3 alternatives: use QualifiedName, use with braces, import with alias). Import Scope rules (Rule 3.8.1) for module-level vs block-level. Qualification and Ambiguity rules (Rule 3.8.2) with [Import-Ambiguous] and [Import-Qualified-OK] formal rules for E3D04. Import shadowing precedence (locals override imports). Cross-references Part VIII for full module system.

9. Lifetime and Storage Duration: Three storage classes (static, automatic, region) with context mapping table. Region Bindings and Escape rules (Rule 3.9.1) with [Region-Escape-Binding] and [Region-Escape-Return] formal rules for E3D09. RAII cleanup semantics (LIFO, scope-based).

10. Visibility and Access Control: Complete specification of four visibility modifiers (public, internal, private, protected) with precise semantics. Default Visibility rule (Rule 3.10.1): internal by default. Field-level visibility syntax for records. Procedure Access rule (Rule 3.10.2): same-module methods access private fields. Cross-Module Access Checking with [Visibility-Check] formal rule.

11. Name Resolution Algorithm: Complete normative five-step algorithm: (1) current block, (2) outward lexical scopes, (3) module scope, (4) imported namespaces with ambiguity handling, (5) universe scope. Includes resolve_name and resolve_qualified algorithms with complete pseudocode. Handles ambiguity detection (E3D04) and undefined identifier errors.

12. Predeclared Identifiers: Complete list of 14 predeclared types, boolean constants (true/false), and built-in functions (panic, assert). Protection rules (Rule 3.12.1) with [Predeclared-No-Redefine] and [Predeclared-No-Shadow] formal rules preventing redefinition or shadowing (E3D12).

13. Inter-Part Authority: Normative authority statement establishing Part III as authoritative for name introduction, scope rules, visibility semantics, and name resolution. Cross-part coordination requirements. Integration points with Parts II, IV, V, VII, VIII.

14. Complete Diagnostic Catalog: All 12 diagnostics (E3D01-E3D12) with trigger conditions, error messages, and suggested fixes: E3D01 (Redeclaration), E3D02 (Implicit shadowing), E3D03 (Namespace conflict), E3D04 (Ambiguous identifier), E3D06 (Uninitialized variable), E3D07 (Invalid shadow), E3D08 (Non-const in type position), E3D09 (Region escape), E3D10 (Assign to immutable), E3D11 (Local function), E3D12 (Redefine predeclared).

15. Examples and Idiomatic Patterns: Seven comprehensive examples covering global compile-time constants, local runtime bindings, explicit shadowing, permission-aware parameters, type-position constants, import with alias, and region-scoped bindings.

Purpose: Part III establishes the foundation for all name-based program structure. It MUST come before Expressions (Part IV) because expressions reference variables that must be declared. It provides complete normative rules for compiler implementers on name introduction, scoping, visibility, and resolution. The two-keyword model (let/var only) optimizes for LLM-friendliness while maintaining explicitness through context-driven compile-time evaluation. All scope and visibility rules are centralized here, with other parts referencing this authority.

Part IV: Expressions

File: Spec/04_Expressions-and-Operators.md (Expressions and Operators)
Contents: This chapter defines how expressions are formed and evaluated. It covers everything from primary literals up to complex expressions and is structured by increasing precedence and complexity. Each expression form includes its syntax, static typing rules, and dynamic semantics (evaluation order, side effects). Required sections:

1. Expression System Overview: Provides a high-level Overview that an expression “specifies the computation of a value by applying operators and functions to operands”. Introduce the idea that expressions produce values and may have side effects. Mention value categories if relevant (e.g. lvalue vs rvalue; if Cursive requires this distinction for assignments or move, define here). State the guaranteed evaluation order (Cursive evaluates sub-expressions left-to-right in all cases, if that’s the design) and note any sequence point or sequencing rules. Summarize how effects propagate in expressions (e.g. if any expression could carry an effect requirement, but likely effects are tied to function calls via the effect system). This sets the stage for the detailed forms.

2. Primary Expressions: These are the simplest expressions that do not involve an operator acting on two operands (highest precedence forms). Include:

Literals: All literal forms (numeric, string, char, bool) are expressions yielding a value of the corresponding type. (Refer back to Lexical Structure for literal syntax; here focus on their type and value.) For example, an integer literal without suffix is of default type i32 unless context forces a bigger type.

Variable references: Using an identifier as an expression yields the value bound to that name. Static rule: the identifier must be in scope and initialized; dynamic: it yields the current value. If the variable is of a permission type, using it might consume or borrow it depending on context (the rules for that interplay with the move/copy semantics).

Parenthesized expressions: Just yield the inner expression’s value (used to override precedence, no semantic change).

Block expressions: A { ... } block can be an expression whose value is the value of the last expression in the block (or () if the block ends with a statement). Define the rules: all paths in a block expression must produce a value of the same type (or the block diverges). This effectively allows multi-statement expressions.

(Other possible primary forms: e.g. if the language had lambdas as expressions or struct/array literals, those might be covered here as well. Lambdas/closures are covered later if supported.)\*

3. Operators: Define all unary and binary operators, their precedence, and semantics. It’s helpful to include an operator precedence table (which will also be in an appendix for quick reference). This section can be organized by operator arity and category:

Unary Operators: e.g. logical NOT !, arithmetic negation - (on numeric types), dereference \* (if dereferencing pointers), address-of & (if obtaining addresses of variables, likely returns a safe pointer or permissioned reference), possibly a mut or imm cast operator if those are explicit (though likely not; if mut x is allowed to cast an immutable reference to mutable, that would be defined here). Each unary op: specify the operand type requirements and result type, plus any side effects (usually none beyond evaluation of the operand).

Binary Operators: Include: Arithmetic (+ - \* / % on numbers; define integer division rounding and modulo behavior), Bitwise (& | ^ << >> on integers; define that shifts are only valid within bit-size, etc.), Comparison (== != < <= > >= on comparable types – define equality rules for each type, e.g. can you compare two strings by value? likely not unless built-in; mention that for reference types equality compares addresses or content depending on design), Logical (&& || for booleans with short-circuit evaluation semantics). Each operator’s result type and truth-table or effect should be clear (e.g. && and || yield a boolean and only evaluate the right operand if needed).

Assignment Operators: Simple assignment = and compound assignments like +=, -=, \*=, …. Define that x = e evaluates e, possibly moving or copying into x. For compound x += y, it’s equivalent to x = x + y with x evaluated once (specify order precisely). State that assignment is not an expression (if that’s the case in Cursive, likely it is a statement only – though in some languages it yields a value; decide and clarify).

Pipeline Operator (=>): If supported, explain that expr1 => expr2 is a chaining operator (for example, could desugar to expr2(expr1) or method chaining). Specify how it associates (likely left-to-right) and where it sits in precedence. This is a unique operator in Cursive, so give an example of usage.

Other Operators: If the language has a ternary conditional (e.g. ? :), include it. If not (perhaps if is an expression form instead), skip. Also mention range operators (if any, like a..b) or spread operators (if supported in patterns or calls).

Precedence and Associativity: After listing operator semantics, include a summary (table or text) of operator precedence from highest to lowest and their associativity (left or right). For example: postfix (calls, indexing) highest, then unary, multiplicative, additive, shifts, comparisons, logical, then assignment lowest. This helps readers resolve ambiguous expressions (and is normatively defined here and in Appendix).

Overflow/Edge-case behavior: E.g., explicitly state that integer overflow triggers a runtime overflow check or wraps, as per Cursive’s design (with references to the [[overflow_checks]] attribute controlling this). Also, division by zero handling (likely runtime error). Short-circuit rules for && || (do not evaluate second operand if result determined). If any operators introduce sequencing guarantees or lack thereof, state them (though if all evaluation is left-to-right strict, this is simpler).
(Many of these details (like full operator table) may be deferred to an appendix for brevity, but the main spec should at least reference that table and highlight non-standard behaviors.)

4. Function Call and Member Access: Describes expression forms for calling functions and accessing members:

Function Calls: Syntax f(expr1, expr2, …). Static: f must be an expression of function type or a built-in callable. The number and types of arguments must match the function’s parameter types (after inference/coercion if any). Discuss argument evaluation order – Cursive guarantees left-to-right evaluation of arguments (including any effects in arguments) to avoid undefined behaviors. Dynamic: each argument is evaluated, then the call transfers control to the function, then returns a result. If there are multiple return values, describe how the call yields them (tuple or direct assignment to multiple targets).

Method Calls: If using object-oriented dot-call syntax (e.g. obj.method(args)), describe how it resolves to either an actual method (if obj’s type has that procedure) or a trait/contract default. The receiver obj is often passed as an implicit first parameter (like self). Static dispatch vs dynamic dispatch: if contracts are interfaces, a call via a contract reference might be a dynamic dispatch – specify if the language does dynamic dispatch or if all calls are static/monomorphized. (Likely contracts have static dispatch since implementations are known, unless using trait objects which isn’t mentioned.)

Field Access: expr.field for record fields or tuple elements (expr.0). Static: the type of expr must have that field; result is an lvalue of the field’s type (if used on left of assignment) or rvalue if just reading. For tuple indexing, an integer literal index selects the nth element (only allowed if the type is a tuple of at least that length).

Array/Slice Indexing: expr[index]. Static: if expr is an array [T; N] or slice [T], then index must be an integer (likely usize), and result type is T. Discuss that in a safe context, an out-of-bounds index causes a runtime panic/error (and possibly in an unsafe context, you could bypass bounds checking with a different operation).

Member access and Method chaining: Mention that the leading dot rule (if any) allows chaining methods in a fluent style line by line (this might be something like if a line starts with .method() it’s treated as continuing the previous expression). If such syntax exists, clarify it.

5. Conditional Expressions: Describes expressions that choose between alternatives:

If Expressions: In Cursive, if can be used as an expression (as well as a statement). Syntax: if cond { e1 } else { e2 } yields a value. Static: cond must be of type bool. Both branches must produce a value of a compatible type (unifying to a common type, or exactly the same type). If an else is omitted, the “missing” branch yields (), so the whole if-expression type might be (). Dynamic: evaluate condition, then exactly one of the branches, yield that branch’s value. This works like a ternary operator.

If-let Expressions: If pattern matching is possible in an if, describe if let PAT = expr { ... } else { ... }. This attempts to match expr against a pattern, and executes the first block if successful (with any bindings introduced), otherwise the else. Static: type of expr must match the pattern’s type. In an expression context, likely if-let also yields a value (both branches must yield a value).

(Ternary Operator?: If the language had a cond ? e1 : e2 syntax, mention it as equivalent to an if-expression. If not, skip.)

6. Pattern Matching (Match Expressions): The match expression provides multi-way branching based on patterns. Syntax:

match expr {
Pattern1 => result1,
Pattern2 => result2,
\_ => resultN
}

Static rules: The scrutinee expr can be of any type that the patterns can match against. All patterns must be exhaustive – cover all possible values of the scrutinee type, or include a wildcard _. The type of each arm’s result must be the same (or coerce to a common type) because the match as a whole yields that type. Pattern forms include: literal patterns (match constants), variable binding patterns (bind the value to a name), wildcard _, tuple patterns, record patterns (match by field names), enum variant patterns (with subpatterns for payloads), OR-patterns (p1 | p2), and guard conditions (pattern if condition). The spec should state that patterns are checked in order and the first matching arm is chosen. If no arm matches, it’s a dynamic error (but exhaustiveness checking should prevent that unless match is non-exhaustive in an unsafe scenario).
Dynamic semantics: Evaluate expr, then sequentially test it against each pattern. When a pattern matches, evaluate that arm’s expression and that is the result. During the arm evaluation, any pattern-bound variables are in scope, and for guard expressions, the guard is evaluated only after a successful pattern match. Order of evaluation: the scrutinee is evaluated first, then pattern matching is conceptually done (with guard evaluation considered part of matching). Only the chosen arm’s expression is evaluated.
(This section should cross-reference the detailed pattern grammar given in Part I’s abstract syntax and Part II’s enum definitions. Exhaustiveness checking algorithm can be described here or in an Appendix/Algorithms section, as it can be complex.)

7. Loops: Cursive uses a unified loop construct (already implemented in part in existing spec). This section covers both the syntax and the semantics of all loop forms:

Infinite loop: loop { block } – executes block indefinitely. This expression never terminates normally; if used in an expression context, it can only type-check if it breaks with a value or diverges.

While-style loop: loop condition { block } – semantics: evaluate condition, if true execute block then repeat, if false, exit loop. Essentially a while loop.

For-style loop: loop pattern in expr { block } – iterate over an iterator or collection. expr should be a collection or implement an iteration trait. For each yielded value, match it against pattern and execute block. Explains that this is desugared (for example, into a call to an iterator’s next method in a loop).

(If there’s a distinct syntax for while or for, list them, but it seems everything is unified under loop with optional parts.)

Loop control: Explain break and continue. break may optionally take a value (if the loop itself is used as an expression, e.g., val = loop { if condition { break result; } ... }). The value of the break becomes the value of the loop expression. continue skips the remainder of the loop body and starts the next iteration. Labeled breaks/continues: syntax 'label: loop { ... } allows breaking out of an outer loop by name. Document that break 'label value will break the loop named 'label.

Loop contracts: Cursive’s loops support formal verification aids: invariants (with { ... }) and variants (by expr). Invariants are conditions that must hold at the start of each iteration (the compiler may check these or assume them for static proofs), and variants are a measure that decreases each iteration to prove termination. The spec should describe that these clauses do not change runtime behavior but are used for static verification. The static semantics must ensure the invariants type-check (they are boolean conditions) and likely generate proof obligations. Variants must be some integer measure that is proven to decrease (the spec can mention what types of expressions are allowed and that they must decrease towards 0).

Typing rules: A loop with no break value has type ! (never returns) or (), whereas a loop that breaks with a value in all cases can have that value’s type. Detail these rules so that type checking of loop expressions is defined.

Dynamic semantics: Mostly straightforward: for infinite or while loops, it’s an obvious repetition; for loop in (for-loop), it’s iteration until exhaustion. Termination is not guaranteed unless variants/invariants used – if variants are present, they must decrease but the spec may not enforce termination at runtime beyond what the code naturally does.

(Since the existing spec already has a full section on loops, ensure all that content is integrated and that loops link to relevant parts of the spec like expressions vs statements.)

8. Blocks and Sequencing: Although blocks were mentioned as expressions, here we emphasize statement sequences. A block { s1; s2; …; en } (where en is an expression or nothing) can be seen as a way to combine multiple sub-expressions. Static: all statements inside must type-check (with any new declarations scoped to the block), and if the block is used as an expression, the last element determines its type (or unit if last element is a statement). Dynamic: execute each statement in order, then evaluate the final expression if any. Define that blocks introduce a new lexical scope – any declarations inside are not visible outside (scope rules are elaborated in Part VII but should be mentioned here as well for context).

Also clarify how implicit returns work: if a block is in expression position (e.g., right side of an assignment or as a loop body in expression form), the value returned is the last expression not followed by a semicolon. If a semicolon ends the block, it yields (). This is the typical Rust/Go block expression behavior.

9. Let and Var Bindings in Expressions: While variable declarations are statements, in Cursive some bindings can occur within expressions (if allowed). For example, a let binding could be used in a match guard or maybe an inline assignment. If relevant, mention that let x = expr inside an expression is not allowed (assuming Cursive keeps let as a statement). Likely skip if no expression-level binding aside from patterns in if-let and match which are covered already.
   (If nothing special here, this section can be omitted or merged with statements. Otherwise, ensure any expression-level binding (like a match arm’s pattern introduces new variables) is covered under pattern matching semantics.)

10. Ownership and Moves in Expressions: Highlight how the act of using values in expressions interacts with the ownership system. For instance:

Using an own T value in an expression context (like passing to a function or evaluating in an arithmetic operation) will move it unless it implements Copy. After an own value is moved, the original variable is considered uninitialized (cannot be used). The spec’s static semantics must enforce that (we might refer to Part VII for the definite assignment rules after move).

The explicit move expr operator: If Cursive requires an explicit move to transfer ownership in closures or to force moving out of a variable (when it could be inferred), define it here. move expr takes an own T and yields an own T by value, invalidating the source. It may mainly be used to document intent or in closure capture lists.

Distinguish this from copy: if T is Copy, using it in expression does not invalidate the original. The spec should clarify at least informally here, while the formal rules for move vs copy are in the Permissions/Memory part.

11. Region Expressions: Covers the syntax and semantics for region-based memory management in expressions:

Region block: region r { ... } – introduces a new memory region named r for the enclosed code. All allocations annotated to use region r will go into this region’s pool. The region (and any allocated objects within) is automatically freed when the block exits (either normally or via unwind, if applicable). Static: r is a fresh region name that can be used in allocation expressions inside. Region names likely cannot escape the block.

Allocation in region: If the language has a way to allocate an object in a specific region (e.g. alloc_in<r>(value) or new r T(...)), define that here. The result is an own T allocated in region r. Static semantics ensures this call is only valid inside the corresponding region r {} scope.

Region lifetime rules: Any pointer into a region (or any value owning data in region r) must not survive beyond the region’s scope. The compiler must reject code that attempts to return or store such a pointer outside. This is akin to Rust’s lifetimes but here tied to lexical region blocks.

Dynamic: all alloc_in<r> allocations are freed en masse when leaving the region block. This gives O(1) deallocation of all region data. Attempting to allocate in a non-existent region or after leaving the region is an error.
(This section ties into the Memory Model (Part IX) but here we introduce how the syntax works in code as an expression.)

12. Compile-Time Expressions (Comptime): If Cursive supports executing code at compile time:

Comptime block: comptime { ... } – an expression that is evaluated at compile time. The result of the block must be a compile-time constant that can be used in the surrounding program (for example, to initialize a constant or as a type parameter, etc.). Static: The block’s contents have restrictions (no runtime-only operations like I/O or heap allocation, unless those are allowed but would run at compile time). Possibly it can call const-evaluable functions. Dynamic: Actually executed by the compiler during compilation; in the running program this just yields the precomputed result. Clarify distinction: side effects in a comptime block affect the compilation environment or produce constants, not the runtime.

Constant expressions: Define what constitutes a constant expression (e.g. arithmetic on literals, calls to const functions, etc.). This might be partially covered here or in the Part X (Comptime). But at least mention that certain expressions can be evaluated at compile time and are required in contexts like array lengths or const initializers.

13. Lambda/Closure Expressions (if supported): If the language has anonymous functions:

Syntax for a lambda (e.g. |x, y| x + y or similar).

Capture semantics: if the closure can capture variables from the enclosing scope, explain capture by value vs by reference. If by value (like C++ default), the captured variables get moved or copied into the closure’s environment. If by reference, ensure the lifetime rules tie in with region or scope so you don’t escape references. Perhaps Cursive chooses explicit permission-based capture (like capturing an own moves it, capturing an imm or mut creates an alias).

Closure’s type: typically some function type with environment. Possibly mention that closures are essentially records with a call operator, but the details can be left abstract.

Static semantics must handle the type of captured variables (if an own variable is captured by value, it’s moved into the closure, invalidating it outside).

Dynamic semantics: a closure creation returns a closure value (with any captured data stored), and calling the closure calls the enclosed code.
(If Cursive does not have lambdas, this section can be omitted or marked as future.)
Purpose: The Expressions chapter precisely defines how every expression is formed and what it means at runtime, which is crucial for both compiler implementers and users. It should include enough formal detail to avoid ambiguity – for example, specifying operator precedence and evaluation order so there is no undefined evaluation order as found in C. Each expression subsection should have a Syntax (grammar production), Static Semantics (type-checking rules, e.g. operand types and result type), and Dynamic Semantics (how to evaluate and what the result/side effects are). Where appropriate, examples or notes can illustrate tricky points (like short-circuit logic, or how a match covers all cases). The existing content on loops will be integrated here, and all other expression forms must be completed beyond what is currently written. An operator precedence summary will also be provided (and duplicated in an appendix for convenience). By the end of this part, a reader knows how to form any expression and what behavior to expect when it executes.

Part V: Contracts and Clauses

File: Spec/05_Statements-and-Control-Flow.md (Design by Contract System) – plus subsidiary clause files in Spec/contracts/
Contents: This extensive chapter covers Cursive’s contract-based programming features – its system of preconditions, postconditions, type invariants, and effect clauses that can be attached to procedures and types. It is largely complete in existing spec files and needs verification for consistency. The main file provides an overview and references sub-sections (which may be split into separate files for each clause type for manageability, e.g. Spec/contracts/03_Effects.md, 04_Preconditions.md, 05_Postconditions.md, 06_TypeConstraints.md, 07_ContractComposition.md). Required sections:

1. Contract System Overview: Explains the purpose and components of Cursive’s contract system in an Overview. Introduce the four kinds of clauses a procedure or type can have: effects (uses clause), preconditions (must), postconditions (will), and type constraints (where clauses on record types). Discuss the philosophy: explicit declaration of assumptions and guarantees, compositional behavior through calls, statically verifiable where possible (to reduce runtime cost), and zero runtime cost when proven (verified contracts can be omitted at runtime). Also mention how contracts relate to other language features – e.g., they complement the type system by specifying semantic conditions beyond type correctness. This overview sets context for the detailed sections on each clause type.

2. Behavioral Contracts (Interfaces): Specifies how to declare and use contract interfaces (abstract behavioral specifications) – largely overlapping with Part II’s abstraction types but focusing on the clause aspects:

Contract Declaration: Syntax: contract Name { ... } with a body containing only procedure signatures with optional clauses (uses/must/will) but no implementations. Emphasize that contracts are pure specifications – they define a required interface of operations (including their pre/post conditions and allowed effects) that an implementing type must fulfill.

Implementing a Contract: How a concrete type or procedure indicates it implements a contract. Possibly via an implements ContractName for TypeName { ... } or by the procedure declaration matching a contract signature. Static rules: the implementing procedure must have clauses that meet or exceed the contract’s requirements (preconditions can be looser or equal – i.e. an implementation cannot require more than the contract promised, postconditions can be equal or stronger – cannot promise less than the contract, effects must be subset or equal – cannot do more side effects than allowed). These rules enforce Liskov substitution principle in terms of contracts.

Contract Inheritance/Extension: If contracts can extend other contracts (multiple inheritance of interface specifications), define how that works. For example, a contract could refine another, adding more procedures or strengthening some clauses. The spec should mention any constraints (like no conflicting procedure specs).

Usage of Contracts: A contract might be used as a type (if the language has trait objects or interface references). If so, mention that a value of contract type C can hold any implementing object, and calls on it dispatch to the concrete implementation (dynamic dispatch). If dynamic dispatch exists, mention that effect checking and contract checking still apply at runtime appropriately.

Coherence and Orphan Rules: If relevant, specify that contracts implemented for types must either be in the same module as the contract or type (to avoid multiple conflicting implementations, analogous to Rust’s orphan rules), ensuring uniqueness of implementation.
This section aligns with Part II’s abstraction types but focuses on the semantic aspect of fulfilling contracts.

3. Effect Clauses (uses): Details the effects system for side-effect control.

Syntax: A uses clause on a procedure, listing effect names (possibly namespaced like io.read, fs.write, etc.). Example: procedure foo() uses io.read, alloc.heap { ... }.

Standard Effects Catalog: Enumerate the predefined effect categories in Cursive (these might be organized by module): e.g. alloc._ (memory allocation effects: alloc.heap, alloc.stack, alloc.region), io._ (I/O operations like io.read, io.write), fs._ (filesystem), net._ (network), time._ (timers/sleep), thread._ (concurrency operations like thread spawn/join), ffi._ (foreign function calls), unsafe._ (unsafe memory accesses), panic (panic/abort). This list informs what side-effects a procedure can have.

Static Semantics: If a procedure is declared with certain effects, it may not perform operations outside those effects. The compiler enforces that (for example, calling a function with a certain effect requires your own effect list to include it; operations like file I/O are only allowed if io.\* is in uses, etc.). If a function calls another function, the caller’s effect set must be the union of its own side effects and all effects of the functions it calls – effect composition is transitive. This means effects propagate up the call stack; a pure function (no uses) can only call other pure functions.

Dynamic Semantics: Effects themselves don’t do anything at runtime (they are a static restriction), but this clause can be checked statically. If the language supports effect polymorphism, mention that (e.g. a generic function can be effect-generic and the effect set is an implicit type parameter).

Pure Procedures: Define that a procedure with no uses clause (or an explicit uses []) is considered pure – it should not perform any side effects (no global state changes, no I/O, etc.). The compiler can optimize based on purity or enforce purity.

Possibly mention effect inference: the compiler could infer certain effects if not explicitly stated, but likely explicit.

Effect Lattice: If effects have a hierarchical or lattice structure (e.g. maybe io encompasses io.read and io.write), describe that relation. However, it might not be needed if effect names are simple tags.

4. Precondition Clauses (must): Specifies preconditions – conditions that must hold true when a procedure is called.

Syntax: In a procedure signature, must { P1, P2, ... } where each Pi is a boolean expression (an assertion about parameters or global state) that the caller guarantees before calling. Example: procedure divide(x: f64, y: f64) must y != 0.0 { ... } means callers must ensure y is not zero. For readability, in code blocks often written as must condition1, condition2.

Static vs Dynamic: If a precondition can be determined at compile time (e.g. a constant or something proven by the type system or prior contracts), the compiler might not emit a runtime check. Otherwise, at runtime the implementation should check the condition at the procedure entry and handle failure (likely by triggering a panic or returning an error). The spec should say that failing a precondition is a programming error – either undefined behavior or a runtime panic, depending on design (preferably a defined panic).

Enforcement: The caller is responsible for ensuring the precondition, which means the compiler may emit warnings or errors if it can detect a call site that obviously violates a precondition (for example, static analysis could catch it, though that’s advanced). At minimum, document that if a precondition is not met, the behavior is not guaranteed (or “the behavior is undefined unless specified otherwise”, or that a runtime check will trigger a contract violation error).

Weakening in Implementations: When a type implements a contract’s procedure, it may weaken the precondition – i.e. require less of the caller than the abstract contract did. However, it cannot strengthen it (cannot add new required conditions that weren’t present in the contract), because that would break substitutability. The spec should clarify this rule to ensure consistency between interface and implementation.

Provide examples and mention typical uses: e.g. a function requiring a non-null pointer, or an input array to be non-empty.

5. Postcondition Clauses (will): Specifies postconditions – conditions guaranteed true after a procedure finishes.

Syntax: will { Q1, Q2, ... } in a procedure declaration. For example: procedure increment(x: mut i32): i32 will result == @old(x) + 1 { x += 1; x } guarantees the return value is one greater than the old input.

Explain special symbols in postconditions: result refers to the return value of the procedure, and @old(expr) denotes the value of an expression at procedure entry (often used to state how output relates to input state). If multiple returns, perhaps there’s a way to name them. If the procedure has out-parameters or modifies state, you can also mention how to refer to those (maybe @old can be used on object fields, etc.).

Static vs Dynamic: Some postconditions can be verified at compile time (especially simple ones or ones that mirror a precondition of a called function), but generally many will be checked at runtime before returning. The implementation must ensure the condition holds on exit – if not, it should trigger a runtime failure (since that indicates a bug in the function). Postconditions are the implementer’s responsibility: they promise these to all callers.

Strengthening in Implementations: An implementation can provide a stronger postcondition than the contract specifies, but not a weaker one. (Stronger means it guarantees more – which is fine because callers only relied on the weaker guarantee, so they get an even better one; weaker would break the contract).

Example uses: ensuring an output is non-negative, or that a data structure remains sorted after an operation, etc.

6. Type Constraint Clauses (where on types): Defines invariants that must hold for every instance of a type. This is like a class invariant in Eiffel or a lightweight refinement type.

Syntax: In a record (or possibly contract) definition, a where { R1, R2, ... } block listing boolean expressions about the fields. Example: record Percentage { value: i32 where { value >= 0, value <= 100 } } ensures any Percentage is between 0 and 100.

Semantics: These conditions must be true after construction of any object of that type and must be preserved by any operation that mutates the object (any method must ensure the invariant still holds on return). The compiler can treat these as assumptions whenever it has an object of that type (for static verification).

Enforcement: At runtime, when an object is constructed (or possibly when a field is assigned), the invariant should be checked. If violated, it’s a programming error (likely triggers a panic). Statically, if a field is public and can be mutated freely, each mutation should be accompanied by re-checking the invariant (unless proven by a precondition or so). This ties into design by contract: invariants are like always-on postconditions for constructors and mutators.

Also clarify that these are universal truths: e.g. for an abstract data type, they define the valid state space. They can be referenced in other contracts (e.g. a precondition might assume a record’s invariant as part of its requirement – often not needed to state because it’s implicitly guaranteed by the type itself).

7. Clause Composition and Interaction: Describes how multiple clauses interact, especially when procedures call other procedures (contract inheritance through calls). Key points:

Effect propagation: If procedure A calls procedure B which has effects, A’s uses must include those effects (unless A’s effect clause is more general and covers it). All called effects accumulate – “effects aggregate” in the caller.

Precondition obligations: If A calls B, A must ensure B’s preconditions at the call site. That often means A’s own preconditions must imply B’s, or A must perform checks. If A’s precondition is stronger, that might indirectly ensure B’s (as in the example where caller’s must y>5 implies callee’s must y>0). The spec can illustrate that the caller’s obligations include callee’s preconditions – “preconditions strengthen upward”.

Postcondition usage: A can rely on B’s postconditions after the call. Thus A’s postconditions may be formulated assuming B delivered its guarantees. “Postconditions chain (weaken) downwards”, meaning a caller can promise something that is guaranteed given the callee’s guarantee, possibly a subset of it.

Type invariants and contracts: Type invariants are always implicitly in effect; a procedure’s preconditions can assume the invariant of any object passed in (because it should be true unless broken elsewhere), and postconditions must not violate invariants on output.

Multiple clauses together: Procedures often have a combination of effects, preconditions, and postconditions. The spec should note that these are largely orthogonal (effects talk about side effects, preconditions about inputs, postconditions about outputs), and all must be satisfied. If any clause is violated, the contract as a whole is violated.

If contracts can be inherited (via contract extension), describe how clauses from parent contracts combine with child contracts (probably all must apply).

Violations: It might be worth stating how contract violations are handled uniformly (likely by panicking or throwing an assertion error). Possibly reference an Errors appendix for standardized error codes for contract violations.
Purpose: This Contracts part provides a normative reference for Cursive's design-by-contract features, which is a standout feature of the language. It must ensure that the rules for checking and enforcing contracts are crystal clear, as they affect both compile-time verification and runtime checking. The structure here follows the logical division: each kind of clause in its own section for clarity. Each clause section should include Syntax of the clause, Static Semantics (what the compiler must enforce or assume), and Dynamic Semantics (what checks happen at runtime). The existing 05_Contracts.md and its subdocuments (Preconditions, Postconditions, etc.) will be used to fill in these sections – they are marked as comprehensive, so mainly we verify consistency in terminology (using the standardized "uses/must/will/where" keywords throughout). Cross-references to other parts of the spec (e.g. effects linking to Part IV for actual IO operations, or type invariants linking to Part II type definitions) are included to help navigation.

Part VI: Statements and Control Flow

File: Spec/06_Contracts-and-Effects.md (Statements)
Contents: This chapter covers language constructs that are used as statements (top-level actions or declarations within function bodies) rather than expressions. Some constructs exist in both expression and statement forms (like if or loop), so this part focuses on statement-specific rules and any differences from expression usage. Required sections:

1. Statements Overview: An Overview explaining what a statement is (an instruction that does not produce a value consumed by another expression). Clarify the distinction between statements and expressions in Cursive (though the line is blurred since many control flows are expressions, but e.g. declarations and assignments are pure statements). Reiterate the statement termination rules (newlines end statements unless a continuation rule applies), as this is crucial to parsing. Also mention how statements execute in sequence within a block. Provide a brief taxonomy of statements: expression statements, declarations, assignments, control flow (if, match, loop, etc.), and special ones like return, break.

2. Expression Statements: Using an expression solely for its side effects as a statement (e.g. calling a function and ignoring the result, or incrementing a variable with an operator). Static: any expression can be a statement, but if it produces a value that isn’t used, that’s fine (some languages warn on unused values, but the spec might not require it). Dynamic: just evaluate the expression and discard the result. Emphasize that this is how function calls that return something are often used when only the side effect matters. If certain expressions are disallowed as statements (maybe assignments must be statements, not nested in expressions), clarify those grammar restrictions here.

3. Declaration Statements: These introduce new variables or constants in a scope. In Cursive, let and var declarations are statements.

Immutable Let: let x: T = expr; or let x = expr; (with type inference). Static: type of expr must be compatible with T if given, otherwise T is inferred. The variable x becomes available in this block after this point, and cannot be reassigned if it’s a let. If a pattern is on left (like let (a,b) = expr destructuring a tuple), ensure pattern matching rules apply (all components must be initialized).

Mutable Var: var y: T = expr; for a mutable binding. Same rules except y can later be assigned to. Discuss definite assignment: all local variables must be initialized before use. In Cursive, the syntax requires an initializer at declaration, so that’s guaranteed unless the language allows uninitialized var (which it likely does not for safety). If uninitialized declaration is possible, static rules must require that it’s assigned to before any read.

Constant Declarations: If there’s a keyword (possibly const) for compile-time constants at local scope or file scope, mention it. These must be initialized with constant expressions and then are treated as inlined values. (File-scope constants would be covered in Part VII under declarations as well.)

Clarify that type aliases or record/enum definitions are not statements inside a function (they are only at module scope), so here we focus on runtime declarations.

Scope: each declaration is valid until end of the block. This overlaps with Part VII which will talk about scopes generally, but mention here that these statements introduce names visible in the remainder of the block.

4. Assignment Statements: Modifying an existing variable’s value. Syntax: target = expr; or compound target op= expr;.

Static: The target must be an assignable lvalue (a mutable variable, a record field through a mutable reference, an array element, etc.). If the target’s type is not exactly the expr type, normal type conversion/coercion rules apply (likely must match exactly or via implicit conversion if any). Compound assignments (+= etc.) require the target to be numeric or appropriate type; they are essentially shorthand so they require the same static checks as the expanded form (x = x + y).

Dynamic: Evaluate the right-hand side, then assign to the target. For compound, evaluate target (for reading its original value) and RHS, then apply operation, then store back. All such operations happen left-to-right, and the target is evaluated once (to avoid double side effects). Example: a[i++]+=2 (if allowed) – clarify how many times i increments (should be once). The spec should define these evaluation order details.

Effects: assignments to variables have no special effect beyond memory write (which is not tracked by effect system since that’s local state). But assignment to a field of a struct might be considered part of normal operation. If assignment triggers any drop or deallocation of an overwritten value (like if the value holds owned data being replaced), mention that – though drop semantics likely in Memory model.

5. Control Flow Statements: These mirror some expression forms but as statements (no resulting value), plus some that only exist as statements:

If Statements: if cond { … } else { … }. Static: cond must be bool. No value is produced, so the two branches don’t need to have matching types; they are just two blocks of statements. (They can still contain returns or breaks, etc.) Dynamic: obvious branching, if no else and condition false, it does nothing (execution continues).

Match Statements: If Cursive allows match as a statement (all arms just execute blocks and don’t produce a unified value), describe it. It’s similar to a match expression, except that since it’s not producing a value, you can allow each arm to do different things (they could even produce different types within their block, since it doesn’t matter). Still requires exhaustiveness unless there’s a \_. Often used for pattern matching when you don’t need a result.

Loop Statements: Using loop purely as a loop, not as an expression producing a value. The syntax is the same; the only difference is we don’t utilize a result. Many loops in imperative style are of this form. (Since we already cover loops thoroughly in expressions, here we might just reference that section for semantics. Emphasize that a loop used as a statement may use break without a value or with if needed to exit.)

Break/Continue Statements: Already covered conceptually in loops, but list separately:

break; – exits the innermost loop. If it has a label, break 'outer; exits the named loop. In a pure statement context, break cannot carry a value (unless breaking out of a loop that is an expression expecting a value – a subtlety that belongs to expression context). So in statement context, just break; is used.

continue; – jumps to next iteration of innermost loop (or labeled loop if continue 'label;).

Static: only valid inside loops.

Dynamic: as described, with labeled resolution.

Return Statements: return expr; or just return; if returning void/unit. Static: can only appear inside a function/procedure body. The type of expr must match the function’s return type (or if multiple returns, perhaps syntax is return x, y; matching multiple output types). If returning from a procedure with an effect or state, ensure any necessary finalization is done (this is runtime, e.g., dropping local variables, handled by runtime automatically). If return; with no expression is used in a function with a return type, that’s a static error (unless the return type is ()).

If Cursive supports early returns from blocks (like Rust’s do { ...; return ...; } expression or just using return in a block expression yields from the function), mention that return always exits the entire function, not just the block.

If the function has ensures (postconditions), conceptually those are checked after return before the function truly finishes (but the spec might treat that as part of function semantics in Part VI or Contracts, not needed here).

Assert Statement (if provided): Some languages have assert(cond); as a statement that checks a condition at runtime and aborts if false. If Cursive has an assert (for debug checks), document it here: it’s like a short form for “if not cond, panic”. Could mention that in optimized builds it might be disabled (if that’s a thing).

Defer Statement (if provided): A defer expr; or defer {…} statement which schedules expr or block to run at function exit (like Go’s defer). If present, explain LIFO ordering and that deferred statements execute even on early returns or panics (except if truly aborting). This interacts with resource management but might belong in advanced or not at all. (Given “if applicable” in outline, optional.)

Empty statement: maybe mention an empty statement (just a semicolon or newline) does nothing.

6. Block Statements: Although blocks {…} are expressions, they can also be seen as statements for grouping. Ensure we note that a bare block is a statement that introduces a new scope (especially useful to limit lifetime of variables). Already covered in expressions, but reaffirm usage here.

7. Labeled Statements: Mentioned partly under loops, but in general a statement can have a label 'name: prefix. Those labels can be target of break/continue. Static: label must be unique in the function and can only label loops (and maybe blocks if break to label is allowed on blocks). They don’t affect execution except for the control flow.
   Purpose: The Statements chapter defines the imperative structure of Cursive programs – how code is organized in sequences, blocks, and control flows. It complements the Expressions part by covering the forms that don't produce values (or where values are optional). Many rules here overlap with expression counterparts (like if/loop), but it's important to specify them in both contexts for completeness. Each statement form includes Syntax and Semantics (mostly dynamic semantics, as static semantics often just ensure correct usage). This part also ties into scoping (labels and declarations) referencing Part III (Declarations & Scope). By isolating statements, we ensure that the spec covers both expression-oriented usage and statement-oriented usage clearly. New file 06_Statements.md will be created with this content, ensuring it aligns with the rules from Part IV for expression vs statement where needed.

Part VII: Functions and Procedures

File: Spec/07_Holes-and-Inference.md (Functions and Procedures)
Contents: This chapter details how to define and use functions (pure) and procedures (effectful) in Cursive. It builds on the type and contract system (function types from Part II, effect clauses from Part IV) but focuses on the syntax of definitions and the semantics of calls, parameters, returns, and method dispatch. Required sections:

1. Functions and Procedures Overview: Overview explaining Cursive’s approach to subroutines. Differentiate functions vs procedures: in Cursive, a “function” typically means no side effects (no uses clause, purely deterministic), whereas a “procedure” may perform side effects and can have pre/postconditions. Clarify that both are declared similarly except procedures include contract clauses and possibly have slightly different semantics (like procedures can be thought of as actions with effects). Note that semantically, functions are just procedures with an empty effect set and perhaps no uses clause (so the spec might not treat them drastically differently beyond the presence/absence of effects).

Reiterate the uniform calling model: all parameters are passed by value with permission semantics (so an own T parameter means the caller’s value is moved, a mut T means caller’s value is shared with mutation, etc.). There is no implicit pass-by-reference except what the type system encodes.

Mention that first-class function values exist and have types as defined in Part II (map types). This section sets context that functions can be treated as values but here we focus on definition and invocation.

2. Function Declarations: Syntax for defining a pure function. For example:

function name<Generics>(param1: Type1, param2: Type2, ...) -> ReturnType {
...body...
}

If “function” is a keyword (or perhaps all procedures use the keyword procedure and pure ones are just those with no effects), specify accordingly. Static: A function cannot have a uses clause or any contract clauses beyond perhaps an implicit guarantee of purity. If a function’s body tries to perform disallowed side effects, that’s a compile error (ensured by no uses).

Parameters: Each parameter has a name and type, and may include a permission modifier (e.g. x: mut T means the function accepts a mutable reference, x: own T means it takes ownership). Default is likely imm if no modifier. The function can modify mut parameters (which are references), but cannot modify imm ones (except through interior mutability if type allows).

Return type: After -> the return type (or tuple of types). If a function returns multiple values, it could be a tuple or explicit multiple return syntax (design choice). Cursive might allow multiple return values directly. If so, show syntax -> (T1, T2). If a function returns nothing, it could be -> () or omitted return type (depending on syntax).

Generic functions: If <T, U> after name, they follow the same pattern as generic types: can have where clauses constraining type parameters (e.g. where T: SomeContract). These constraints become obligations on callers and assumptions in body.

Visibility modifiers: If the language has pub or export keywords for functions to be accessible from other modules, mention that here (though details in Modules part). Possibly, by default everything might be public or internal. Clarify if needed that those keywords come before function.

The function body is a block of statements. Static rules: all code paths must either return a value of the proper type or (if function diverges) never return. If control reaches end of function with no return in a non-void function, what happens? (Often that’s an error – but maybe in Cursive, reaching the end implicitly returns () if return type is unit, or is disallowed otherwise). The spec should state it (e.g. “falling off the end of a non-() function is equivalent to returning a default value or is a compile error” depending on design).

3. Procedure Declarations: Syntax for effectful procedures. Likely:

procedure name<Gen>(params): ReturnType
uses effect_list
must { preconditions... }
will { postconditions... }
{
...body...
}

i.e., similar to functions but with optional clauses. All aspects of functions apply, plus:

Self parameter: If procedures can be methods on records or implement contracts, there might be an implicit or explicit self parameter. For instance, in a contract implementation for type X, you might have procedure X.method(self: mut X, other: Y) .... If methods are declared outside type definitions (like Go), the first parameter can be a special self. If inside an impl block (like Rust), they might be declared differently. The spec should clarify how methods on a type are declared (Part VIII Modules or here). For now, mention that if a procedure is intended to operate on an instance (like a method), it typically takes that instance as a parameter (with own/mut/imm as needed) explicitly.

State transitions (for modals): If this procedure is associated with a modal type, it might consume an object in one state and produce it in another. Possibly the syntax for that could be in the parameter or return type (e.g. proc transfer(self: own T@State1) -> T@State2). Or a special arrow notation in the contract. However done, specify that this procedure, when called, will result in the object changing its compile-time state type. The statics must ensure you only call it when object is in State1, and after call it should be treated as State2. This likely ties into either the precondition (“must obj.state == State1”) and postcondition (“will obj.state == State2”), or a specialized syntax. Document how modal state changes in procedures are described.

Contract clauses integration: The uses, must, will clauses appear here as part of syntax, linking to Part IV. Ensure it’s clear that the meaning of these is as defined in Part IV (static effect checking, precondition obligations on caller, postconditions guarantees by callee). Also possibly allow a where clause on the procedure if it’s within a type with invariants (though invariants primarily on type, not on individual methods).

The body of a procedure can contain anything including side effects. It must ensure the postconditions before any return (likely by explicit checks or logical reasoning – the spec can’t enforce, but the implementation should consider runtime checks for postconditions).

Visibility modifiers: same as functions.

4. Parameters: More detail on parameter passing semantics (some of this overlaps with the overview but put normative details here):

All parameters are passed by value in terms of implementation, but the value might be a pointer (for mut or imm references) or an actual value (for own). So effectively, own T parameters cause move of T into the function (caller loses ownership), mut T and imm T cause the function to receive a reference (alias) to the caller’s data. The spec should clarify that semantics: e.g. if a procedure takes mut T, any modifications it does are visible to the caller (shared mutable), whereas if it takes own T, it has a distinct copy/instance and the caller can’t use the original afterwards (unless it’s returned).

Default parameter values: If Cursive supports default arguments, describe syntax (e.g. param: Type = default_val). Static: callers can omit that arg, it defaults to the given expression. All default expressions must be const-evaluable or maybe just compile-time known.

Named parameters: If supported (like calling with name: expr out of order), mention that the definition needs to allow it (and how it maps; often just sugar and doesn’t affect semantics except argument matching by name).

Variadic parameters: If the language has varargs (like C’s ... or Python’s \*args), mention how they are declared and accessed. Possibly not in first version unless needed for FFI. If included, static: a variadic param must come last and captures any number of trailing args of certain type.

After execution starts, each parameter behaves like a local variable initialized with the argument value (or reference). So modifications to a mut parameter affect the caller’s data, modifications to an own parameter only affect the local copy (caller’s original is separate but now possibly moved away).

5. Return Values:

Return syntax: covered in statements, but reiterate here as part of function semantics. A return statement returns either one value or a tuple of values matching the signature.

Single vs multiple returns: If multiple, they might be implicitly tupled or distinct. Ensure spec clarifies (e.g. “A procedure may return multiple values, written as -> (T1, T2) in the signature. The return statement then provides two expressions. These are equivalent to returning a tuple, just syntactic sugar.”).

Named return values: If the language allows naming return values (like Go’s named return parameters), mention it: e.g. you can specify procedure foo() -> (x: i32, y: bool) and then within the function use x and y as locals and just return; to return their values. If supported, detail that these are like predeclared local variables that will be returned.

Type inference for returns: Possibly not needed if return type must be explicit, but if the language can infer return type from return statements (some languages do for single-expression closures), mention whether that is done or not. Probably not in a systems language spec – require explicit.

Clarify that returning by value moves out owned values (unless Copy). This ties into the permission system: if a function returns an own T, then any local own T being returned is moved to caller. If it returns mut T or imm T (likely by reference), it’s returning an alias – ensure that alias is not to a local that’s about to go out of scope (the region/lifetime rules must prevent that). So likely functions should not return mut T or imm T that point to a local variable’s memory (would be invalid). Possibly only own or references that came in or are global can be returned. The spec might mention this as part of lifetime considerations (though formal in Memory Model).

6. Method Calls (Dispatch): (Some of this was covered in Expressions for method call syntax, but here focus on how methods are defined and resolved.)

If Cursive supports type‑scope procedures attached to types (like in an impl block or in the type definition), specify how the compiler knows which procedure to call for obj.method(). Usually, it’s based on the static type of obj. If multiple contracts provide the same method name, how is ambiguity resolved? If trait-like methods exist, possibly require fully qualified calls to disambiguate.

Method resolution: State that a method call x.f() is resolved to the procedure f defined for the type of x (or some contract it implements) according to some precedence (for example, an explicitly attached impl for the concrete type takes priority over any default provided by a trait). If interface dispatch (virtual call) is possible, mention that calling via a contract reference uses dynamic dispatch to the implementation.

Self dispatch and chaining: If within a method implementation you call another method on self or on other objects, it uses the same resolution rules. This likely “just works” but mention no special handling except that self might have to be treated with the correct permission (like calling another method might consume self if that method’s receiver is own).

Chaining: Already allowed via expression rules. Nothing new, except maybe note that obj.method1().method2() means method1 returns an object on which method2 is called. That’s fine if types line up.

7. Function Overloading (if applicable): Cursive might or might not allow multiple functions with same name but different parameters. If yes, describe the rules: resolution by arity and types at compile time, no ambiguous overloads, etc. If no, state that function names must be unique in a scope (apart from possibly methods where overloading is by type receiver). Overloading can complicate things, so it may not be present. Outline only if it exists.

8. Anonymous Functions and Closures: If not already covered in expressions, mention here from the perspective of functions as first-class:

Syntax for lambdas (as described in Expressions). Possibly provide an example: let f = |x: i32| x\*x; which creates a closure capturing no external variables, type (i32) → i32.

Closures capturing environment need to have an internal representation (likely as a struct with fields for each captured variable). If by value, capturing an own moves it into closure, etc. The spec may not detail implementation, but should ensure the rules: e.g. cannot capture a mut local by reference and outlive it (so either disallow or require region 'static or something; maybe just disallow capturing by reference unless 'static or use a region). Given region system, maybe any captured local must either be copied or owned by closure (thus remove from outer scope).

Effect tracking: closures have effect clauses too if they perform operations. Possibly, if a closure uses an effectful operation, its type’s effect set includes that (effect polymorphism might allow closures to capture effect variables too). Keep this high-level.

9. Higher-Order Functions: Briefly note that because functions are first-class, functions can be passed as arguments or returned. The type system covers it. For example, a function taking a comparator function: function sort(arr: [T], cmp: (T, T) → bool) -> [T]. The semantics of calling function values is straightforward: just invokes them. If there are any restrictions (like you cannot store a function with effects in a variable without declaring that effect in the variable’s type – effect polymorphism?), mention that. Probably treat function values similarly to any other value of function type.

If effect polymorphism is in language: e.g. a higher-order function might accept a function parameter with some effect and itself be marked with a polymorphic effect that includes whatever the passed function’s effects are. This could be advanced, but mention if supported: maybe via generics on effect sets (∀ε). If not formal, skip.

10. Call Semantics: Describes in more detail what happens on a function/procedure call (some of this is runtime/ABI oriented):

Evaluation order: (Already stated: left-to-right for arguments and the function expression). Ensure it’s reiterated that all arguments are evaluated before entering the callee.

Stack frame layout: Not too much detail (that’s implementation-defined), but mention that each call creates a new stack frame (activation record) and local variables and parameters reside there. If needed, mention tail call optimization is not guaranteed unless specified (likely not).

Calling conventions: The spec might say that the precise ABI (how arguments are passed in registers/stack) is implementation-defined or follows a platform ABI. But if certain calls (like FFI calls) require specific conventions, Part XIII covers that.

Effect propagation: Reiterate that when a call occurs, any side effects the callee may perform are considered as happening during that call. When it returns, if it had side effects, those have occurred. The effect system ensures you knew about them. (This is more static, but dynamic semantics wise, just execute callee code).

Lifetimes and returns: If the callee returns a reference, it must refer to something that still exists (usually either a parameter or global). Ensure dynamic semantics doesn’t allow returning pointer to a local (should be prevented by static, but it’s a runtime crash if it happened – which we say is undefined behavior if it were forced via unsafe).

If needed, mention recursion and that the language supports it (no special case, just calls itself, new frame, etc. Termination is up to developer or proven with variants etc.).

11. Inline and Optimization: This small section ties back to attributes: mention that functions can be annotated [[inline]] or [[no_inline]] as hints. The spec should clarify that these do not change program semantics, only advise the compiler’s optimization (so it’s non-normative as far as language behavior, but normative that compilers may choose to inline or not).

Also mention other optimization-related attributes like [[hot]], [[cold]] that hint at branch prediction or inlining decisions. These don’t alter semantics, so the spec might just list them as optional annotations (which we did in Part I attributes).

State explicitly that the presence or absence of inlining does not affect the program’s observable behavior (aside from performance), as required for a consistent spec.
Purpose: The Functions and Procedures chapter brings together many earlier threads (types, expressions, contracts) to define how executable units are declared and behave. It ensures that the Static Semantics of function definitions (e.g. matching types, no unsatisfied contract clauses) and Dynamic Semantics of calls (argument passing, returns, side effects) are well specified. It will reference Part V for the meaning of uses/must/will clauses on procedures, Part II for function types and generics, and Part IX for how memory and ownership behave across calls. After reading this part, one should understand how to write a function, what the compiler requires for it, and what happens when it's invoked. This is a new file to create, consolidating what was hinted in outlines and ensuring consistency with the rest of the spec.

Part VIII: Modules and Program Organization

File: Spec/08_Functions.md (Module System and Program Structure)
Contents: This chapter describes how source code is organized into modules/packages, how modules interact (imports/exports), program entry point, and build/linking aspects. It leverages standard module system concepts (similar to Rust or Odin perhaps). Required sections:

1. Modules Overview: Overview of the module system design. In Cursive, assume a file-based module system: each source file is a module (or each file must declare its module name). Modules are hierarchically organized potentially by filesystem paths. Summarize that modules provide namespaces and encapsulation for declarations, and you use imports to access other modules’ public definitions. Also mention design philosophy: simplicity (no complex layered packages beyond directories), maybe one module per file for now, and no separate header files (the spec itself serves as the definition of module interfaces).

2. Module Declaration: If required, syntax for declaring a module’s name at the top of a file (like module Foo.Bar; if hierarchical). Some languages like Rust infer module name from file path, others (like D or older languages) require an explicit declaration. The outline suggests maybe implicit from file structure. Clarify either: “The module’s name is derived from the file path relative to some base (like package name + directory). Optionally, a file can include a [[module(name)]] attribute (as listed in Part I attributes) to override or specify the module metadata.” If [[module(... )]] attribute is used, refer to it.

Module naming: Allowed characters for module names (likely identifiers separated by . for hierarchy). Possibly mapping to directories. For instance, module util.math corresponds to folder util/ and file math.cursive (assuming .cursive extension or .md if spec files double as source, but presumably .md here is just the spec).

One module per file: State that each source file defines at most one module. The file name usually matches the module name (with some extension). If multiple files share the same module name, perhaps they are part of one logical module (like partial classes). That’s an option but likely not allowed to avoid confusion. So probably 1-1 file to module.

Module hierarchy: Outline that modules can be nested in packages/namespaces by naming convention and folder structure. For example, io.network.tcp might be a module, which implies directories io/network/tcp.cursive. This is akin to Java/Rust packages. If no hierarchy, keep it flat, but likely support some.

3. Import System: How to use names from other modules.

Import syntax: e.g. import math or import util.math as mathutil or from util.math import sin, cos depending on style. Need to decide a style: Could do Rust-like import util.math which then you use math::sin or something. Or Python-like import util.math as m. Or Odin-like (Odin auto imports from files by just referencing the package).

Possibly: Given outline mentions import aliases, selective imports, re-exports, something akin to Rust:

import foo; (brings module foo into scope as foo).

import foo::{X, Y}; (bring specific names).

Or Python style: import foo.X; (just bring X).

We should specify one approach clearly. Let’s assume:

Simple form: import ModuleName; imports all public names of that module under a namespace ModuleName (so you must prefix uses with ModuleName. unless you also import particular symbols).

Selective: import ModuleName.Name; could import a specific name directly into current scope. Or use a syntax like import ModuleName { Name1, Name2 };.

Aliasing: import ModuleName as Alias; allows referencing module as Alias. Or import ModuleName.Name as AliasName; to import a single symbol under a different name.

The spec should define exactly what forms are allowed and how resolution works. For example, if you do a selective import, the imported symbols become available in the module scope as if declared there (unless name conflict, which is error or needs alias).

Circular imports: specify whether allowed or not. Likely, to avoid complexity, either disallow cycles or allow but with lazy binding (like perhaps allow if no initialization issues). Could say: at compile time modules can be mutually dependent, but at runtime there must be an initialization order. Many languages forbid direct cyclic module dependencies. For now, probably say circular imports are not allowed (the build graph must be acyclic).

Import resolution: how does the compiler find the module file given an import path? For example, if import is by module name hierarchy, it maps to relative path or some search path. Possibly mention a simple rule: the module name corresponds to a path relative to a project root or an import path list. (This can be left to implementation, but some note: e.g. current directory or standard library directories are searched).

Visibility of imports: typically, if you import something in a module, it’s only visible in that module unless you explicitly export it (re-export). This leads to next.

Re-exports: The ability to re-export imported names. Possibly via syntax like export import ModuleName.Name; or in spec terms “If a module’s public API should include something from another module, it can re-export it by an export declaration.” Provide how (maybe just listing it in module export section or using an pub modifier on import, like Rust’s pub use). If supported, explain that those names become part of this module’s public interface as if declared here.

4. Module Visibility (Encapsulation): Summarize rules from Part VII’s visibility but at module granularity:

Public items in a module are accessible to any module that imports it.

Internal (module-private) items are not accessible from outside, even if you import the module (they won’t be imported).

The module itself might have a notion of friend modules if we had packages – likely not, keep it simple (no friend concept).

If hierarchical, maybe internal could mean package-private (visible to sibling modules in same package), but if each file is separate, no intermediate. Possibly skip that nuance for now.

Clarify that the module boundary is the primary unit of encapsulation: only what is explicitly made public is reachable externally. This fosters separate compilation and library boundaries.

5. Module Initialization: If the language demands certain code run when a module is loaded, describe it. For instance:

Are global variables initialized in a specific order? Possibly topologically sorted by import dependencies (so that if Module A imports B, then B’s globals init first). The spec should specify that: modules are initialized in an order consistent with the import graph.

If side-effects in global initializers or an explicit init function: some languages have a special block (like D’s static constructors, or Odin’s init() procedure in a package). If Cursive allows a function named init to act as module initializer, mention how it’s picked up and when executed (likely after all imports of that module are resolved, before main starts).

If multiple files form one program, the entry main is separate, but all modules used must be initialized.

If we disallow complex global initialization (like requiring const initializers only), then runtime init might not be needed except for weird cases. But if allowed (like global could be result of function call), then we need an init step to compute it.

Summarize: At program start, modules are initialized in dependency order; each module’s constants are computed and any module initialization code runs. If none beyond constants, then just ensure order for constants that depend on others. Possibly mention that if a circular dependency exists, one of them will see uninitialized value or it’s an error (another reason to forbid cycles).

6. Package System (if applicable): If beyond modules, there’s a concept of package (a collection of modules, e.g. a library or program), mention it. Possibly not needed if module = compilation unit. But might consider grouping modules into packages for distribution. Could skip in first spec or just note future possibility. Outline suggests maybe leaving for future if at all (“if applicable”).

If including: define what constitutes a package (maybe a directory with multiple modules, compiled together). Dependencies between packages might be resolved via a package manager. Possibly beyond scope here, so a brief placeholder: “Cursive’s specification does not fix a build/package manager, but conceptually a package is a set of modules developed and released together. Dependencies between packages are resolved before compiling, and the module import mechanism covers cross-package references by module name (assuming unique module names per package).”

7. Program Entry Point: Define how a complete program’s execution starts. Likely, like C or Rust, there is a special function (like procedure main(): int or procedure main(): void) that serves as the entry. If the language is primarily for binaries (not scripts), then yes main function.

If main can have arguments (like C’s argv, or not), mention. Possibly main(args: [String]) -> int like typical. Or simpler, no args.

If not having a main (like a library), then obviously that spec part only applies to executables. But for completeness, define main for programs. Possibly mention that if multiple mains (like in multiple modules), the build system chooses one (should only be one in final binary).

Also mention whether main can be in any module or a specific one. Likely anywhere, but typically one writes a main in some root module. If it must be public or have a certain signature, state that.

If no main and the program is just all code executed top-level (like in a script), that’s different – but since we emphasize module initialization, probably need main.

8. Linking and Code Generation: Describes how separate modules combine into a program or library:

Separate compilation: Each module can be compiled to an object code independently (provided it can see interfaces of imports). The spec can say that a conforming implementation may compile modules separately and later link them, as long as module dependencies are respected.

Linking rules: If a symbol is defined in one module and referenced in another, the object files must be linked so that references resolve. The spec might not go deep into binary formats, but should ensure that identical names from different modules don’t collide thanks to namespacing (in practice, mangling or putting module name in symbol). Perhaps mention that an implementation should ensure that fully qualified names are unique (like using module name in symbol).

Symbol export/import: If the target is a static or dynamic library, which symbols are exported? Typically only public symbols should be visible outside the library. The spec might state that non-public symbols can be given internal linkage (not visible in final binary’s symbol table). This is an implementation detail but relevant for FFI and hiding internal details.

ABI compatibility: If needed, mention that the calling convention and memory layout is stable so that modules compiled separately can interoperate. (Unless using FFI to external languages where you might have to specify extern "C" etc, which might belong to Part XIV FFI). At least say: by default, Cursive modules compiled with the same compiler are ABI-compatible, but the exact ABI is implementation-defined or platform-defined (except where FFI requires a specific one).

If the language has any #[no_mangle] attribute (they do have [[no_mangle]] to preserve symbol names), mention that that ensures the symbol name in the binary is exactly as declared (for FFI). Otherwise, compilers might mangle names (particularly if function overloading or generics to encode types). The spec can allow that because it doesn’t affect semantics.

If addressing dynamic linking: the spec might remain neutral (it can be compiled to static or dynamic libs, beyond spec scope how linking is done).
Purpose: The Modules part defines how larger programs are structured from multiple source files, enabling maintainability and separate development. It uses standard mechanisms (imports, exports) to describe how code from one file is made available to another, and ensures that global initialization and program startup are well-defined. This part is crucial for understanding the visibility rules from Part VII in a cross-file context. The new file 08_Modules.md will document these rules and should be consistent with earlier assumptions (like what [[module()]] attribute does, how visibility is used). It provides a normative base for any build system or tooling to know what the language expects in terms of module boundaries and initialization.

Part IX: Memory Model and Permissions

File: Spec/09_Modals.md (Memory Safety and Ownership Model)
Contents: This chapter formally describes Cursive’s memory model – how values reside in memory, what safety guarantees exist (no use-after-free, etc.), and how the Lexical Permission System (LPS) enforces safe aliasing. It complements the type system and operational semantics by focusing on memory behavior. Required sections:

1. Memory Model Overview: Overview summarizing Cursive’s approach to memory safety. It should clearly state what classes of errors are prevented by the language’s design (e.g. use-after-free, double free, buffer overflow if bounds checked) and what are not fully prevented (data races, logical memory leaks if not using regions or RAII). Outline the design goals: to have Rust-like safety without a strict borrow checker, using permissions + regions as an alternative. Also note differences from Rust’s model for context.

Summarize: memory is logically divided into stack, heap, and region allocations. The language ensures that freeing of memory is automatic or structured, so programmers do not explicitly free in most cases (except region end frees region, or program termination frees everything). There is no general pointer arithmetic outside unsafe. This section sets the expectations of what "safe code" in Cursive guarantees.

2. Lexical Permission System (LPS): Explain thoroughly the rules and effect of the own/mut/imm permission qualifiers, as they are central to preventing certain bugs.

Recap permission categories (already introduced in Part II Types under Permission Types, but here from memory perspective):

own T: sole ownership, implies the holder can modify and eventually drop the value. Owning pointer unique.

mut T: a pointer/reference that allows mutation, and multiple such references can exist to the same pointee concurrently (unlike Rust).

imm T: a pointer/reference that does not allow mutation (enforces read-only access), unlimited can exist.

Memory safety via LPS: Because the language runtime (or static rules) ensures that an own T value’s lifetime is tracked, once it goes out of scope or is moved, it’s invalid to use. mut and imm references are tied to either stack (for function parameters) or region lifetimes, so even though multiple mut can alias, some mechanism (like dynamic checks or developer discipline) must prevent they don’t dangle or cause issues. Emphasize that unlike Rust, multiple mutable aliases are allowed, meaning Cursive trades strict compile-time alias exclusion for possibly run-time checks or a simpler guarantee (like regions to limit scope of those aliases).

Conversions and coercions: Possibly allow implicitly converting an own T to an imm T (you can treat owned data as read-only alias to itself, maybe), or an own to mut if you want to lend it (though if multiple mut allowed, lending doesn’t require exclusive access). Document which implicit conversions are allowed: e.g. perhaps own T can be passed to a function expecting mut T (since giving up uniqueness is fine if you allow multiple mut), or mut T to imm T (you can always restrict to read-only), but not vice versa. And after converting own to mut or imm, do we still consider the original own exists? Possibly own to mut could be more like borrowing in Rust – but since no exclusive requirement, maybe it’s fine to just copy the pointer. But then you risk multiple owners. Likely they intend that you cannot implicitly make a second owner, so own T to mut T gives a reference and keeps original owner, meaning the original still must not drop it until references done. This is analogous to how in C++ passing pointer to object doesn’t free original. So maybe we rely on regions or lexical scopes: a mut reference can’t outlive the owner. The spec might enforce via static rule that any mut referencing an own data can’t outlive that data’s scope (like a simpler borrow rule using region block or lexical nesting).

Permission inference: The compiler might infer the minimal permission needed for a given variable if not explicitly annotated (or default to imm?). If such inference exists, mention it. Possibly by default local variables could be considered own, but the compiler sees if you never move it, it’s effectively imm? Or just let developer annotate. Simpler: default to own for let (immutable let would be own but not modifiable after init? That’s contradictory unless own doesn’t necessarily mean mutability, which is separate concept possibly). Actually, maybe let x = expr means x is immutable (no reassign) but if expr yields an own, x has an own permission to the data. So it's still own but you can't rebind x; you could still pass x (moving it) somewhere. var y (mutable var) means y is a reassignable name, but what's the permission of the data it holds? Possibly own by default. We should clarify that imm/mut refer to the permission of pointers or function parameters, not to the variable rebinding. The outlines separated let vs var as immutability of binding. Meanwhile permission qualifiers are part of types.

Example: let p: mut T = &mut someVar; p is immutable binding (cannot assign a new pointer to p after init) but it points to someVar with permission to mutate it.

Or var p: imm T = &x; p is a mutable binding to an immutable reference.

This nuance could be covered here or earlier. Possibly mention that permission qualifiers propagate through type system and are distinct from the let/var binding mutability.

3. Memory Regions: Formalize the region-based allocation model.

Explain that a region is an isolated memory arena with LIFO lifetime. Syntax was covered (region blocks). Now describe memory model: when a region block is entered, a new arena is created. Allocations in that region (through alloc_in) are taken from that arena, typically on heap but grouped. When region exits, all memory in that arena is freed at once. This provides efficient deallocation and limits lifetimes of those objects to the region.

No escape rule: If you try to let a reference to a region-allocated object escape the region (store it in a global or return it), the compiler should reject it, because it would become dangling after region ends.

Use cases: mention that regions are great for batch-allocating many objects (like in a game level or request handling) then freeing them together, avoiding per-object overhead and without a complex borrow checker.

Possibly mention there is no partial freeing of region contents (no free individual object from region, except dropping which might run destructors but memory is reclaimed all at once).

Ensure it's clear how region relates to stack: region is somewhat like an extended stack for dynamic allocs that are freed at scope end. But region content is on heap (unless the compiler stack-allocates large region arrays, which it could but not guaranteed).

4. Allocation Strategies: Summarize how memory is obtained in different contexts:

Stack Allocation: Locals and function call frames are on the stack. Each function call pushes a new frame of a fixed size (plus possibly dynamic size for e.g. variadic or slice locals if allowed, but assume mostly fixed or small dynamic allocations on stack if needed). The spec can say there is an implementation-defined stack size limit, and deep recursion or large stack usage can cause stack overflow (which is not undefined behavior but typically a runtime error or abort; but spec might not define it beyond mention).

Stack variables are automatically freed when their scope ends (which is when stack frame is popped or block ends for smaller scopes where compilers might reorder but conceptually at block end).

Heap Allocation: Objects requiring dynamic lifetime beyond stack (like owned heap memory). Cursive likely has an allocator behind the scenes (like a global heap), but the language might not expose a manual free. Instead, an object allocated on heap will be freed when its own pointer goes out of scope and if regionless, maybe by the runtime (like reference counting or GC or immediate free? But no GC likely). If using RAII, perhaps the Drop trait concept (not explicitly mentioned, but if you have destructors you need to call them). Possibly the language expects region or explicit call for free. However, since double free is prevented, maybe freeing is solely by letting an own drop or region end. They might require all own T to either be on stack (if T is value type) or if it's a pointer to heap memory, then that pointer's destructor frees it. So akin to unique_ptr in C++. The spec should clarify if an own Box<T> concept exists or if all own are inherently like unique pointer for heap. It’s a bit unclear from outline if there's a separate mechanism. The mention of alloc.heap in effects indicates to allocate heap memory, you likely call some standard library allocator which yields an own T pointing to heap. The presence of effect ensures you only do it in allowed places.

We can say: Heap allocation is done via standard library (e.g. new or alloc function), which returns an own T that owns a heap-allocated T. Then that will be freed when that own T goes out of scope (or is moved to region or something).

RAII cleanup: If an own pointer goes out of scope, the runtime will free the associated memory. This is a bit of a leap unless we define a drop semantics. Possibly Part XI Operational semantics covers that or we do here: we might define that at scope end, for each own value, if it's a complex type with destructor, call it; if it is heap allocated, free it. For simplicity, mention that at scope exit all own values that hold heap memory are reclaimed. It's part of memory safety - preventing leaks. Actually, memory leak prevention was mentioned as a guarantee partly, thanks to RAII and region.

Region Allocation: Already done above, but maybe here emphasize performance: O(1) freeing of all at once, etc..

If region uses stack for small allocation and heap for big, that's implementation detail.

Possibly mention other alloc if exists: maybe no others. If concurrency part had thread-local alloc, not needed.

Effects: point out that performing any heap allocation is an effect alloc.heap, using stack is implicit (no effect), using region is alloc.region. Those effect tags correspond to these strategies (so a function can allow or forbid them).

5. Pointer Safety: Summarize safe vs raw pointers as types and what can go wrong only with raw.

Safe pointers: by this they likely mean normal references (our mut T and imm T and maybe Ptr<T>@State if that is a tracked pointer for modals). These are checked by the type system/permissions and never dangle (if used correctly). Possibly describe that Ptr<T>@State is a special pointer that carries state info and might be used in system programming contexts. But it's still considered safe because it’s checked that you only call it when in correct state.

Raw pointers: *T or *mut T that bypass checks. They can be null, can do pointer arithmetic, can violate aliasing, etc. The spec should declare that any use of raw pointers is inherently unsafe and thus only allowed in an unsafe block or function (if that concept exists, likely yes given part X advanced features includes unsafe).

Null safety: safe references are never null (no concept of null for mut T or imm T - they always point to a valid object or not exist at all). Raw pointers can be null and must be checked or cause crash if dereferenced. The spec might define a canonical null constant for raw ptr (maybe nil or use 0 if allowed).

Dangling pointer prevention: safe pointers are prevented from dangling by compile rules (cannot outlive what they point to due to region or stack discipline). Raw pointers are not prevented; it’s on the programmer to ensure a raw pointer is valid when used.

Unsafe operations: enumerating like dereferencing a raw pointer, casting an integer to pointer, pointer arithmetic on raw pointers – all are only allowed in an unsafe context and can break memory safety if done wrong. The spec references this in Part XII (Unsafe).

If pointer arithmetic on safe references is disallowed (it should be), mention it's only allowed on raw pointers.

6. Copy and Move Semantics: Explain how values are transferred or duplicated in memory.

Copy trait: Some types implement Copy, meaning assignments or passing them does a bitwise copy and both the original and new location are valid. Usually small simple types (ints, bool, maybe imm references if we consider them like pointer copy, though pointer copy duplicating alias is fine if both are imm or even if mut, since we allow multiple mut alias, copying a mut reference is conceptually making a second alias – if it's allowed by design, then maybe mut T could be copy? But likely not to avoid double free confusion. Actually if two mut references alias the same thing, which one is responsible for freeing if that was own? If it’s just referencing stack or global, no free. If referencing an own’s heap, copying the pointer does not transfer ownership, it's just alias, but the memory will be freed when the original own is dropped regardless of copies – which would leave dangling copies if they outlive. So likely mut T and imm T are not Copy if they reference something with owner somewhere else, to avoid outliving issues. Or they could be copy but static rules ensure they don't outlive anyway, so copying doesn't break lifetime if region prevents going out-of-scope. Possibly they allow copying references freely as long as region/lifetime rules ensure safety. This might be fine: copying an imm reference just gives another imm alias – safe. Copying a mut reference just gives another mut alias – which they allow. So yes, references can be implicitly copied to create more aliases if needed. They might still not call it "implements Copy" explicitly, but conceptually they are copyable because aliasing is allowed. Could refine: All imm T and mut T are implicitly Copy because they represent non-owning alias. own T is not Copy by default, because that would mean two owners. If a type is marked Copy, maybe that implies it's effectively just value semantics (like small ints) and no destructor, so multiple copies can exist concurrently. The spec should state that by default, assigning or passing an own T moves it (invalidating the source), unless T is Copy. If T is Copy (like an int or maybe a struct of all Copy fields), then assignment does not invalidate original – it duplicates the content (like normal in C++). So basically borrow Rust’s rule: types are Copy unless they or any of their fields have own semantics or custom drop. Actually easier: define a set of built-in Copy types (primitives, imm references, perhaps mut references) and everything else is move-only. Possibly an attribute or trait indicates custom ones. They had mention of a Copy trait in outlines so likely it's an opt-in or opt-out marker.

Move semantics: For non-Copy types, an assignment or passing to function results in a move: after the move, the original cannot be used. This ensures only one own pointer (owner) exists. They explicitly mention move e as an operator to force move in some contexts (like capturing in closures). But generally, move is the default for own.

By preventing use of moved values, they prevent double free: the object will be freed exactly once (when the final owner goes out).

If the programmer wants to copy a non-Copy type, they must call an explicit clone method (not mentioned but likely present via a trait or library – maybe out of spec or in builtins).

Summarize rules with examples:

If you do x = y; and y is own T and T not Copy, then y is moved to x; y becomes invalid (the compiler should treat any further use as error).

If T is Copy, then it’s just copied and both x and y are valid (pointing to independent or same underlying? If it’s a pointer type like imm T, copying pointer means two pointers to same data – but since it’s not owning, that’s fine).

Uniform parameter passing: as covered in functions, all passing is effectively assignment, so same rules – if you pass an own it moves by default (unless function takes parameter by mut reference, in which case it doesn't take ownership, just alias, so you could still use your original after call as you didn't lose it). So “uniform” refers that there's no special by-ref or by-val keywords at call site; it’s determined by param type (if param is own, it moves; if param is mut or imm, it passes a pointer alias).

7. Memory Layout: Define how data is laid out in memory for various types. This can be normative or leave some as implementation-defined but mention conditions:

Type layout rules: For primitives, usually fixed sizes (e.g. i32 4 bytes, i64 8 bytes, etc. maybe spec ties to architecture for isize/usize).

Alignment: Each type has an alignment requirement (commonly equal to size for primitives or power of two <= size). The spec might define them (like i32 aligned 4, i64 aligned 8, etc.) and that composite types get alignment of the max of their fields. If not wanting to commit, could say “alignment is implementation-defined but must be at least the size of type or a divisor thereof, and all types have an alignment power of two typically, etc.” But better to specify for consistency across compilers.

Padding: When placing fields in a struct, compilers may insert padding to meet alignment. The spec could define that fields are laid out in declaration order with padding as needed (like C), or allow reordering for optimization (like some languages might). Usually, for a systems language, keep declaration order.

Record layout: Summarize: the size of a record is at least the sum of sizes of fields plus padding. Possibly allow a trailing padding to make the record alignment equal to max field alignment (so arrays of the record align properly).

Enum layout: Typically, an enum's size = max size of any variant + maybe additional space for a tag if needed. If the variants are non-overlapping (like a tagged union), they often allocate as a union of variants plus a discriminant tag. If some variants are smaller, there might be padding to ensure any variant can fit. The spec could define that a simple enum with no data (just cases) might be represented as an integer (like int starting at 0). If data, use union+tag. Possibly mention representation strategy. This might be complex but at least say an enum’s memory may include unused padding corresponding to the largest variant.

Array layout: contiguous elements, each aligned properly, with no gaps between elements (assuming alignment divides into the whole array properly, any needed alignment padding goes after previous element which effectively means it’s already aligned). The total array size = element size \* count, possibly rounded up to alignment (but if element alignment > element size maybe some tail pad, rarely needed since if element align bigger than element size, align usually not bigger than size for normal data, except zero-sized type which we might not have concept of). If there's a zero-size type (like unit type), it's tricky; probably size 0 and array of them size 0 or maybe at least 1 for distinct addresses, but speculation.

Pointer representation: If safe references mut T and imm T are implemented as raw pointers under the hood or maybe fat pointers if they carry some metadata (like if they point to a region maybe a hidden id? But likely not, region check is static, or there's runtime check on use). Might keep it simple: a reference is just a memory address (like a pointer) at runtime. If Ptr<T>@State carries state, it might be pair of pointer + state tag, or an index etc. Possibly skip detail.

repr attributes: The spec supports [[repr(C)]] or similar via attributes (like they had repr(...)). Document that using [[repr(C)]] on a record or enum will adjust layout to be C-compatible (no reordering, specific packing, etc.). If repr(packed(N)) exists, then it would reduce alignment to N, etc. These attributes would be defined in Part I attributes list; here mention they override defaults. E.g. repr(C) ensure struct layout is exactly as C would lay it (which usually is same as our rules if we already do nothing fancy, so maybe redundant unless we consider different ABIs or want to guarantee stable ABI).

Big-endian vs little-endian: likely state that endianness is platform-defined (like follow the hardware, we don't hide it), so multi-byte values are in the system's endianness. Possibly mention not to assume one in portable code.

So in summary: we should give enough so that someone writing FFI or doing manual memory knows what to expect.

8. Memory Safety Properties: Recap the guarantees vs limitations.

Guarantees:

No use-after-free: Owned memory is freed only when no accessible references remain (ensured by static rules that references can't outlive and only one owner frees at end). If the programmer manually frees something in unsafe, that can break it, but in safe code this holds.

No double-free: Since at most one owner, and after free the owner is invalidated, you can't free same memory twice in safe code.

No memory leaks (if claimable): This one is trickier – do they ensure no leaks? Possibly yes if every own is freed at end of scope. But if you intentionally create a cycle or global own that lives till program end, that might leak but it’s at program end or explicit. If region covers many allocs but region is never closed until program exit, it's not a leak in spec perspective because freed on program exit. They mention RAII and region as preventing leaks. So maybe indeed, aside from deliberately not cleaning something by design (like global), memory is reclaimed. We can state that memory allocated on heap via own pointers is automatically freed when it goes out of scope, so there's no need for manual free (and long-term unreachable objects won't accumulate because they must be owned by something that eventually goes out of scope or a region that ends). This is a strong guarantee, similar to RAII in C++ and unlike GC languages where a leak is possible if references left somewhere or cycles break it. In Rust you can still leak if you intentionally use std::mem::forget, but spec wise we don't have such. We can ignore that possibility or mention if there's an explicit leak function that prevents drop. Probably not.

Also, buffer overflow should be mentioned: arrays and slices are bounds-checked in safe code, so you can't overflow into other memory via array indexing – that's a guarantee (with runtime check raising error if out of bounds).

Dangling references created by user not possible in safe code due to region system.

Null pointer dereference prevented in safe code because no safe null references.

Limitations:

Aliasing bugs: Because multiple mutable references to the same object are allowed, the language does not prevent logical bugs like unexpected concurrent modification (except there's no data race concept yet – but if single-threaded baseline, then concurrently maybe not at same time, but e.g. one alias changes something the other wasn't expecting; that's a logical bug not memory unsafe though). They might be referring to issues like iterator invalidation (two mut references to a vector, one modifies length, other uses stale index, etc.).

Data races: If concurrency exists, since multiple mut alias are allowed across threads (if they can be, likely protected by effect system maybe, but if not, data races are possible in safe code unless they design something to prevent it – probably relying on effect to mark thread spawn as requiring thread.spawn effect, but not automatically preventing race, just tracking usage). So they likely consider data races as not prevented (especially if no borrow checker to enforce exclusive mutation per thread). That would be in concurrency part.

Iterator invalidation: Without borrow rules, you can modify a collection while iterating with another alias, possibly causing the iterator to point to freed/invalid memory? Actually if you have two mut references to a vector: one pops an element (freeing it), the other had pointer to that element (like an iterator referencing internal array) – then that's a use-after-free in safe code if allowed. That is indeed a scenario of memory unsafety if allowed. Rust prevents it by not allowing that aliasing pattern. How does Cursive handle? Possibly via effect system or by disallowing certain actions concurrently – but I'm not aware of anything in spec to prevent it. They mention "no borrow checker" explicitly. So indeed, I think safe Cursive might have some holes or they rely on programmer not doing such things, or use static analysis separately. They at least list it as a limitation: "iterator invalidation (no borrow checker)" which implies yes, if you do that pattern, it's not caught, and could cause error at runtime. But if you index out of bound you'll catch, but if you hold a pointer inside and then structural modify, the pointer might become invalid. Maybe region concept helps if vector is in region and you ensure lifetimes? Not necessarily, because the vector internal buffer could be on heap within region. Or they expect library to caution against using two mut pointers that way.

So they'd list such scenario as user responsibility.

Possibly mention that pointer aliasing can cause surprises (like you modify through one alias, the other alias sees changed data, which might break invariants if not careful – not memory unsafe, but logical bug).

So basically, concurrency and aliasing issues are not eliminated. They sacrifice that for simpler rules.

9. Lifetime Management: Summarize how the language determines when values are destroyed (some already in scope rules, but bring them together).

Scope-based lifetimes: Each local variable (and by extension any owned resources it holds) is destroyed at end of its lexical scope (block or function). The exact point is after the last statement in the scope, in reverse order of declaration or something (like C++ RAII). If multiple in same scope, typical semantics: last in first out destruction. If we have a formal semantics part, they'd precisely define that. But here just mention that. If one object owns another (like a struct with an own field), when struct is dropped, its fields are dropped as well.

Region-based lifetimes: as said, memory in region freed at end of region. All objects in it considered dropped at that time (if any need destructors, they'd all run at region end – presumably in unspecified order or reverse of allocation? Actually, region allocate might not track destructors individually unless they store them somewhere – maybe region is intended for trivial types or there's a mechanism to call destructors for objects in region when it ends. Perhaps a simple approach: only allow trivially destructible types in region for now, or if needed, call destructors in reverse allocation order. Could specify one if we want completeness: likely reverse allocation order to mirror stack unwinding logic).

Lifetime vs Rust lifetimes: Reiterate difference that we do not have explicit lifetime parameters on types; lifetimes are either lexical (for stack) or tied to region scopes (for regions), not something user annotates on types. So simpler model at cost of not capturing some constraints.

If any sort of dynamic allocation is not cleaned (like if you purposely do something to leak or circumvent, but in safe code you can't really circumvent easily because you'd have to use unsafe or global that never goes out of scope which is not a leak, it's just static).

The concept of manual memory management like malloc/free isn't here; it's automated by RAII. There's likely no general garbage collector either. It's deterministic destruction. The spec might highlight that as an advantage (like real-time friendly, etc.).
Purpose: The Memory Model part ensures the specification clearly defines how memory is managed and what safety guarantees the language provides – crucial for a systems programming language claim. It references earlier parts (like the LPS from Part II, region from Part III, attributes from Part I for repr, etc.) and sets the stage for the formal operational semantics in Part XI by describing intended behavior. The new file 09_Memory.md will incorporate many points from outlines (especially complete-language outline which had a thorough list) and cross-reference to relevant sections (e.g. uses of alloc.heap effect from Part IV, pointer types from Part II). This part should be meticulously checked as it affects correctness and security of programs.

(Parts X onward cover advanced and platform-specific topics, continuing in similar detail, but due to length, they are summarized more briefly below. These parts must still outline file structure, sections, and content as required.)

Part X: Advanced Compile-Time Features

File: Spec/10_Comptime.md (Compile-Time Execution and Metaprogramming)
Contents: Focuses on Cursive’s facilities for executing code at compile time (similar to constexpr in C++ or comptime in Zig). Required sections:

1. Comptime Overview: Explains the philosophy of compile-time code in Cursive: enabling computations and checks during compilation without macros, to generate efficient code or constants. Clarify that Cursive deliberately avoids a traditional preprocessor or macro system; instead, functions can be executed by the compiler if marked or invoked in a comptime context, providing metaprogramming capabilities while keeping one language (no separate macro language).

2. Comptime Expressions: Detailed in Part III (we introduced comptime { ... } blocks). Here, formalize that any code within a comptime block is executed by the compiler at compile time. The result of the block is treated as a constant in the program. Static: within comptime blocks, some operations are forbidden (I/O, allocation? Unless it’s allowed but that would affect build system, likely not allowed or severely limited since compile time I/O could be used for codegen tasks carefully). The spec should define which expressions are compile-time evaluable: typically pure computations on literals and data, calls to functions that themselves can be evaluated at compile time (no side effects or allowed side effects possibly limited to emitting compile errors or warnings). Possibly the verify attribute ties into static verification usage in contracts.

Example: computing a constant array or mathematical lookup table at compile time rather than runtime.

Also mention that functions can be partially evaluated: e.g. if a function is called with constant arguments, the compiler might run it at compile time and embed the result, if it has no disallowed operations. (This is like an optimization beyond spec, but spec might permit it if it's not observable difference).

3. Generic Specialization: Although generics are mainly monomorphized (each instantiation compiles separate code), one can consider this a compile-time feature: generating code per type. If any special handling needed, mention it here (like you can use compile-time type reflection or detect types in generics to branch differently, etc.). Possibly the spec doesn't include active type reflection at compile time (no mention earlier). If none, then generics just monomorphize straightforwardly. But if advanced, mention type as a kind that can be used in compile-time context or something. Could skip heavy detail if none.

Also mention const generics as they allow computing array sizes etc at compile time. Already done in generics part. Emphasize that those indeed are evaluated at compile time.

If the language allows partial specialization or overload of generics, mention here or say not in v1 (likely not needed now).

4. Constant Propagation: Could combine with above; basically state that the implementation will attempt to evaluate constant expressions at compile time (like folding arithmetic, etc.), and maybe mention optimization passes like inlining or propagation but not too much (that's compiler behavior more than language). The presence of this section suggests highlighting that compile-time computation isn't just explicit comptime blocks, but also any pure function called with constants might be folded away. Summarize it as allowed but not required (optimizations are not mandated by spec usually, but we can allow it by saying "the compiler may evaluate any expression at compile time if it can determine the result is not dependent on runtime inputs and has no side effects visible at runtime").

But be careful: the spec might not want to overspecify optimization capabilities. Possibly this section is an informative note that compilers do optimize and it's allowed because it doesn't change semantics.

5. Comptime Limitations: Clarify what you cannot do in compile-time code. For example, no infinite loops (or must prove termination of comptime code, or else compiler error if it can't determine termination). Possibly restrict certain operations: no spawning threads at compile time, no infinite recursion (the compiler should detect or at least error out if hits recursion in constant eval). If the language is Turing-complete at compile time, one could hang the compiler with a wrong code, which might be acceptable as long as eventually times out, but safer to forbid obviously non-terminating patterns if possible. Many languages just risk it, maybe with a recursion depth limit or evaluation step limit. Might mention that the compiler can impose a limit to ensure it doesn't run forever (e.g. depth or time).

Also mention that while at runtime some side effects are allowed, at compile time they might either be disallowed or have different semantics (e.g. printing something at compile time could be considered a compiler message rather than program output). Perhaps simply disallow I/O in comptime contexts for determinism.

If any differences in semantics at compile vs runtime, probably want to minimize that, ideally none beyond not allowing certain things. So the results should be the same as if it ran at runtime (transparency), aside from side effects which are not allowed so the difference is moot.
Purpose: This part collects all compile-time evaluation capabilities to support features like advanced constant expressions, computed tables, etc. It is relatively small but important to define to avoid confusion between what happens at compile time vs runtime. It sets stage for advanced metaprogramming and static reflection if added in future.

Part XI: Error Handling

File: Spec/11_ErrorHandling.md (Error Handling Mechanisms)
Contents: Defines how errors and exceptional conditions are represented and managed in Cursive (since no exceptions). Likely uses Result and Option types as in Rust for recoverable errors, and panics for unrecoverable. Required sections:

1. Error Handling Overview: Philosophy: explicit error handling, no hidden exceptions, so programmers must handle errors manually (like Rust’s approach). Mention that there is no ? operator (as outline specifically notes), meaning you can’t propagate automatically, you must check and return explicitly or use combinators.

Highlight differences from exceptions: no stack unwinding except in panic (which might abort or unwind optionally), errors are part of return types (Result). Also mention common patterns: using match or if let to handle results, etc.

2. Result Types: Describe the Result<T, E> enum (likely part of standard prelude or built-in) for error handling.

Syntax of Result in code: something like

enum Result<T, E> { Ok(T), Err(E) }

If in prelude, just say it's provided by standard library with those two variants.

How to use: functions that may fail typically return Result<SuccessType, ErrorType>.

Handling: user must match on it or convert.

Encourage pattern matching for error propagation (like example in search snippet: using match to propagate or handle).

Possibly mention some helper methods from library (like is_ok(), etc.), but not necessary at spec level.

3. Option Types: Similarly, define Option<T> for maybe-values.

It's basically enum Option<T> { Some(T), None }.

Use for cases where absence of value is not an error but an expected possibility (like searching in array).

It's conceptually separate from Result (which usually indicates error vs success, Option indicates presence vs absence).

Handling similarly with match or if let.

4. Pattern Matching Errors: Emphasize using match to handle Result and Option exhaustively.

Show a snippet perhaps:

match compute() {
Ok(val) => { ... use val ... }
Err(e) => { ... handle e ... }
}

Possibly mention combinators or manual propagation by returning Err up (like doing return Err(e) in a function if a call returned Err).

Also mention that not handling an error (like ignoring a Result) might be warned by compiler because that’s losing an error silently (in Rust, unwrapping or ignoring is typically flagged). The spec can recommend that ignoring a Result without handling is bad practice (maybe even define that Result doesn’t implement Copy, so if you don’t use it that’s okay but you might get a lint).

Could mention "exhaustive checking" meaning you must handle both Ok and Err in a match (no default allowed if not covering all).

5. Panic and Termination: Explain how unrecoverable errors are handled through panics.

A panic might be triggered by assert failures, out-of-bounds, explicit call to panic function.

Define what panic does: Possibly abort the program (maybe by default), or if there's some unwind mechanism. If including an unwind, mention that it unwinds stack calling destructors, unless in an unsafe block or FFI boundary where it's UB, etc. But might keep simple: by default, panic = abort (like in Rust's release mode). If a more friendly environment, maybe they allow catching panic at top-level thread.

Mention that panics are considered a last resort and code should prefer returning errors.

Also mention program exit conditions: hitting end of main returns 0, calling panic in main aborts with error code or message.

If no exceptions, there is no try-catch; so can't catch panic except maybe in separate thread (like Rust catch unwind). But if not covered, skip.

Possibly detail that any contract assertion failure (precondition at runtime) triggers a panic.

Similarly for index out of range triggers a panic.

This section sets the expectation that panics are not meant for normal error handling, just for bugs or not-expected conditions.

Implementation: maybe the panic macro or function logs an error and aborts. Could mention effect: calling panic might be considered an effect like panic effect (they listed it in effect clauses). But since it's not recoverable, maybe not needed in effect system (though they did include panic in effect catalog). That suggests you must list panic in uses if your function may panic? Possibly for static analysis of exceptions. Not sure if they'd require that. They said "panic" in effect list means a function might panic. That is unusual because effect system for panic is not typical, but maybe they want to track which functions cannot ensure not to panic (for safety-critical, you might want to know all panics). It's interesting if included. We should mention that: there's an unsafe.panic or just panic effect that indicates potential panic. But an effect is meant to be something to check or restrict. If you want to say "this function will never panic", you might enforce uses not containing panic. Could be a nice use of effect system to guarantee no panics in some code.

If so, mention that a pure function should not panic or else it's not truly pure/hard to optimize. Up to them.

Stack unwinding: if any, mention it. Could say by default, panic aborts (no unwinding). If they intend to support unwinding in future, might mention as an option. Outline indicates "if any" for unwinding.

Abnormal termination: differentiate panic vs an OS-level abort or segfault. Ideally, no segfault because safe code checks everything. So abnormal termination only from explicit panic or hitting an unimplemented/NYI with a panic.

Conclude that after a panic, program stops (unless multi-thread maybe others run but if main thread panicked, probably stops whole program; if a child thread panics maybe just that thread ends if not catching, as in Rust). If concurrency, mention that each thread panic might not kill whole program unless main panicked or nothing catches it. But we can leave such nuance to concurrency part.

6. Error Handling Patterns: Offer guidelines or known patterns for how to handle errors in code. This might not be normative, more explanatory:

For example, mention that one can propagate errors up by returning Err from your function if an internal call returned Err (like the ? operator does implicitly in Rust, but since we don't have it, you'll write if let Err(e) = something() { return Err(e); } or use a library function to map it).

Possibly mention creating custom error types or using a sum type for multiple error variants. But maybe too detailed for spec (that might be library guidance).

Best practices: e.g. prefer using Result for expected errors and reserve panic for truly unexpected ones (like invariants).

Also maybe mention that ignoring errors (like calling a function returning Result and not using the result) might lead to a warning because it's likely a mistake. Not sure if spec enforces that or it's just a lint. But can state that compilers should warn on unused Result or Option.
Purpose: This part codifies that Cursive uses a Rust-like approach to error handling. It ensures that everyone knows there are no hidden exceptions; all errors are explicit either in return values or a program panic. It references the standard Option/Result types (likely defined in a core library but effectively language fundamentals), ensuring spec completeness. 11_ErrorHandling.md might rely on describing those types which might technically be in standard lib, but for spec completeness, they treat them as built-in conceptual types (since they appear in examples and pattern matching needed them). It also sets up how contract failures are handled (likely panic) linking to Part IV, and how effect system enumerates panic for static assurance. This part is fairly straightforward but important for writing robust code.

Part XII: Unsafe Operations

File: Spec/12_Unsafe.md (Unsafe Code and Contracts)
Contents: Defines what unsafe means in Cursive – certain operations or blocks where the programmer takes over responsibility for safety. This part outlines what one can do only in unsafe contexts and what obligations the programmer has. Required sections:

1. Unsafe Operations Overview: Overview that unsafe code is a necessary escape hatch for low-level tasks, allowing the use of raw pointers, FFI, and other unchecked operations. Emphasize that while safe code guarantees certain safety, unsafe code can break them if used incorrectly, so it should be used sparingly and carefully. Define that an unsafe block or function is needed to perform unsafe actions, signaling to reviewers/compilers "here be dragons". Also note that marking code unsafe does not automatically do anything except allow those actions – it’s up to the programmer to ensure they uphold invariants (like pointer not null, etc.).

Possibly mention that unsafe doesn’t turn off all checks (e.g. integer overflow may still panic unless in unchecked release mode, etc., but pointer deref and such won't be checked so yeah).

2. Unsafe Blocks: Syntax: unsafe { ... } – a block in which unsafe operations are permitted. Static: inside it, you can do things like dereference raw pointers, call unsafe functions. Outside an unsafe block, those are errors. Note that an entire function can be marked unsafe meaning callers must call it in an unsafe context (because it might have safety preconditions). Actually, two uses: one is unsafe block used inside safe function to isolate a few operations, another is marking function itself as unsafe meaning "calling this function is unsafe because it has some requirements the caller must uphold".

If supporting the latter: mention that if a function is declared unsafe procedure foo(...), then any call foo() must occur in an unsafe block or unsafe function. That indicates the function is not memory safe unless the caller does something right (like not passing bad pointer, etc.).

The spec should define how the compiler treats that: it basically requires an unsafe context to call it (like Rust).

If we have unsafe trait implementation concept (like implementing an interface unsafely), might skip for now unless needed for some trait safety.

Also mention any restrictions: presumably inside an unsafe block you can do anything including safe operations. The block just allows extra stuff. Exiting the block doesn't automatically verify what you did (like in Rust, it's just to scope it).

3. Raw Pointers (Unsafe Use): Summarize from Memory part that using *T or *mut T is unsafe. Outline what one can do:

Create a raw pointer from an address or by casting a safe reference to raw (that cast is safe to do?). Actually casting a safe reference to raw is safe and does not need unsafe (in Rust, &T as \*const T is a safe conversion). Using the raw pointer is unsafe part (dereference or free etc.). Might mirror that: making raw pointer is safe, dereferencing it is unsafe.

Pointer arithmetic: Only allowed on raw pointers, in an unsafe block. The spec should note it doesn’t check overflow or out-of-bounds in an unsafe context. If you compute a pointer outside the allocated object, and deref it, that's undefined behavior (spec should list that as an unsafe contract: you must only deref pointers that are valid).

Null pointer: deref is UB etc.

Essentially list out that raw pointers must not be used to violate memory safety: not point to freed memory, align correctly, etc. But these are not enforced by compiler, just obligations on programmer.

Possibly mention converting raw pointer to safe reference is unsafe as well because you promise it's valid for that lifetime/region. So maybe a function like unsafe fn raw_to_mut<'r, T>(p: \*mut T) -> mut T is how you convert (not actual code, but conceptually you need to do an unsafe assertion to get a safe reference from a raw pointer, because the compiler can't check if that pointer is valid or in correct region).

Memory model wise, spec can say that if you do access to invalid pointer, the behavior is undefined (so not just a runtime error or trap, truly undefined i.e. could crash or corrupt or do nothing obviously wrong but break invariants).

4. FFI Primitives: Many FFI calls are unsafe because they might violate Rust (or Cursive) invariants, or require correct usage. Outline how calling external functions works. (Part XIII covers FFI more, but here focusing on unsafe aspect.)

Usually, calling a C function is unsafe because compiler can't check if you passed correct pointer or that the C function upholds memory rules etc. So if cursive has an extern keyword or @ffi attribute, usage would require an unsafe context.

For spec: state that to call a function imported via FFI (declared with some foreign or extern interface), the call must be in an unsafe block or function. Similarly, certain operations interacting with FFI (like forming a C struct pointer) might be unsafe.

Also mention any special rules like the ffi.call effect must be declared (so static check ensures you can't call FFI function unless you listed ffi.call in uses? Possibly yes if they treat FFI as effect).

Also if any guarantee differences: maybe mention that the language cannot guarantee anything about what the foreign code does (it might violate memory safety or not adhere to effect declarations except we trust it as given). So it's on the programmer to ensure those contracts hold at boundaries.

5. Unsafe Patterns: Provide common patterns that require unsafe and how to do them safely (basically guidelines).

Example: implementing a memory allocator or garbage collector might involve manipulating raw memory in an unsafe way. Or writing low-level device drivers with memory-mapped I/O (casting addresses to pointer).

Also "transmute" or reinterpret cast is often an unsafe operation – if allowed, mention it (like casting between incompatible pointer types or pointer-to-int and back can be considered unsafe or at least not recommended outside unsafe context).

Another pattern: bypassing the permission system temporarily. Perhaps one could do something like forcibly cast imm T to mut T via raw pointer hack (not recommended but possible). That would break the aliasing rules and thus is unsafe to do. The spec should note that such aliasing with mismatched permissions can cause undefined behavior if it violates fundamental design assumptions (like two own pointing same memory, double free scenario, etc.).

If assembly inline is allowed, it's definitely unsafe. They might not cover inline assembly in spec now, but maybe mention if they'd allow it within unsafe blocks as an asm! macro or such. Possibly skip for now if not planned.

Summarize best practices: minimize unsafe, isolate it, reason carefully about invariants. Possibly reference the concurrency part if relevant (like atomic operations are safe if used properly, but messing with thread data unsafely is a problem – but atomic operations might require unsafe or not? They often are safe in Rust if properly API). We'll see concurrency part.
Purpose: This part draws a clear boundary between what’s guaranteed by the language vs what the programmer can do in unchecked ways. It ensures that the spec enumerates all undefined behaviors (like deref invalid pointer, data races, etc.) in one place, or at least references them. It's important for implementers and advanced users to know exactly what is not allowed in safe code and what happens if they circumvent it. 12_Unsafe.md thus serves as a reference for all these behaviors and ties into other parts (Memory, FFI, Concurrency, etc. mention unsafe aspects but here is central). It's analogous to Rust’s "Unsafe Code Guidelines" albeit smaller.

Part XIII: Operational Semantics

File: Spec/13_Semantics.md (Formal Operational Semantics and Type Soundness)
Contents: Presents a formal model of how programs execute and the guarantees of the type system (progress and preservation). This part is more academic and may use inference rules and judgments introduced in Part I. It's meant for language designers and those proving compiler correctness. Required sections:

1. Operational Semantics Overview: Explain whether they present a small-step or big-step semantics (or a mix) and the general approach. Possibly they lean on a structured operational semantics with configurations like <expr, state> ⇓ <val, state'> as hinted by the notation in Part I.

Clarify the relationship: We have already in Part I introduced judgment forms for evaluation and state transitions. This part will fill in rules for them. Also mention that both expressions and statements need semantics, and that it will show how well-typed programs evaluate without getting "stuck" (progress) and that types are preserved at runtime (preservation theorem).

If small-step vs big-step: the notation ⟨e, σ⟩ ⇓ ⟨v, σ'⟩ looks like a big-step (evaluation from initial state to final state yields value v and new state σ'). But they might also have small-step for fine details. Possibly a mix: big-step for functional semantics and small-step for concurrency or effect interleaving? Outline doesn't explicitly mention small-step except in listing. Actually it mentions small-step vs big-step in the introduction. They likely say: one can define semantics either way; they might choose big-step (easier for expression evaluation) and small-step (for type soundness proofs often use small-step reduction and progress, but big-step can also prove preservation etc).

I'd guess they will provide either big-step rules for expressions and an argument for type soundness via induction (like a proof outline).

They likely don't do an entire proof in spec but state the theorems and maybe mention that a proof exists or is intended.

2. Expression Evaluation: Provide rules for evaluating each kind of expression to a value.

For instance, rules like:

If expression is a literal, it evaluates to itself (with no state change).

If expression is x + y, evaluate x to a value, y to a value, then perform addition producing a new value (with potential overflow check or if ignoring, state whether it traps or wraps).

If expression is a function call, define how arguments are evaluated and then how control transfers.

This should reflect the informal semantics in Part III but in formal rule style. For side effects, they introduced an effect component in the state possibly, or use evaluation context for sequence.

Also mention how evaluation order (left-to-right) is enforced (like using evaluation contexts or specifying that in rules).

Possibly use a small-step approach here: e.g. an eval context for leftmost evaluation, then an arithmetic step, etc. But big-step might be simpler because side-effect state can be threaded easily with big-step as well (like their σ representing memory state).

Also mention "value categories" if any, but they earlier said not focusing on lvalue vs rvalue except maybe needed for assignment semantics. They might have an environment mapping variables to addresses or such. Some formal details could be heavy, but hopefully they keep it digestible.

The presence of effect tracking in outline suggests they incorporate effect in semantics either as an attribute of expressions or as part of state changes, or have separate rules for effect checking at compile time vs run time (since at runtime effect doesn't exist, it's only static except actual side effects like I/O or memory changes). Possibly they want to ensure evaluation rules include "this operation requires effect X to be present", but that belongs in static semantics check not runtime. So maybe effect tracking in semantics means if they do small-step, they might label transitions with an effect (like a step rule could have a label "alloc.heap" when memory allocated, and type system ensures if that label occurs then it was allowed by uses).

Might skip doing that and just trust static enforcement. Instead, they might show in dynamic semantics that any effectful operation modifies state in a certain way and rely on static to ensure allowed.

A detail: they may incorporate sequence points or evaluation contexts to handle the left-to-right and short-circuits.

Also define when something is a value vs needs more evaluation. Values include literal values, addresses, closures maybe, etc.

3. Statement Execution: Formal rules for each statement type.

For example, assignment: evaluate RHS to v, update variable in environment to v (if it's an own, probably allocate or move pointers in store).

Control structures: if: evaluate cond, then depending on bool choose branch, then continue.

Loops: in big-step semantics, loop would be a recursive rule that if cond true and body terminates then loop evaluates again, etc, or they might give it as a tail recursive semantic rule.

Break/continue: might be easier in small-step with labels or by desugaring loop into a function. But they can do it by returning a special status up the big-step chain (like an intermediate result "broke with value X" which the loop rule catches).

They might simplify by not formalizing break/continue precisely, or doing it in an informal way. But as spec, better to at least outline them (maybe in prose).

We should mention they define state transitions (like a state includes the store or heap and variable env, which gets updated on assignment, allocation, etc.).

If they have a separate judgment for statements (like ⟨stmt, σ⟩ ⇓ σ' or something).

Also include effect of return (would likely be handled similarly to break but returns up out of function).

If using big-step, return can be handled by the function call rule directly rather than in statements semantics (the call rule might capture that the body returns a value).

Possibly easier: define semantics for blocks and return in an operational way.

4. Function Call Semantics: Explain how function calls are handled in semantics explicitly.

Possibly the most complex because of new scope, parameter passing, etc. The semantics has to show that when calling a function, you create a new stack frame environment for parameters and locals, then execute the body, then get the result (unless it diverges or panics).

They likely treat a function call like:

Evaluate all args to values (in current state),

Extend state with new frame (with initial store for new variables),

Evaluate function body in that extended state,

On normal return, get result and updated state, then pop frame and give back result and state to caller.

On a return encountered, they'd capture that as an outcome without executing rest of body.

On a panic (if modeling, they might consider it an "undefined" or error outcome, possibly not formalized or it aborts semantics).

Also mention uniform call conv: all parameters passed by value or pointer per earlier definition, which should align with how environment is set up (like copying values or pointer values into the new frame).

Also mention how effect propagation might not matter at runtime aside from actual state changes, so no runtime effect check beyond performing actual I/O or memory operations.

5. Memory Operations: Formalize what happens with allocation, deallocation, region ops, etc., at runtime.

E.g. a rule for alloc_in<r>(v) might say: allocate a fresh address in region r with content v, return a pointer to it, update heap in σ with that new mapping. Also ensure if region r not present (like out of scope) then it's invalid (but static prevents that).

Region block semantics: on entering region block, push a new region context in state (like a stack of region frames), on exit, deallocate all memory in that region (removing those addresses from the heap). Possibly call finalizers if needed, but maybe skip formalizing that if not essential.

Stack allocation is implicit in pushing new frames (the environment covers it).

Heap allocation presumably calls some allocator (we might not formalize algorithm, just treat it as generating a fresh address).

Deallocation: either explicit (region end) or implicit (function returns and all owned locals freed).

They may not simulate memory free explicitly if they treat it as just unreachable memory (but to prove no leaks they might incorporate that it does get freed).

Or they could have an explicit state element "free list" or available memory, etc. Probably not needed for semantics correctness, just ensure no use after free by static means.

pointer dereference: semantics should say reading from a pointer yields the value stored at that memory address in the heap or stack. If address not allocated or out of region, then the semantics is not defined (like no rule applies, which is one way to model UB: stuck state).

That ties into type safety argument: well-typed program won't do that (progress says never stuck on a memory error perhaps).

6. Type System Properties: State and perhaps prove (or outline proof) the key soundness properties.

Progress: If a program is well-typed (and all preconditions satisfied?), then either it evaluates to a value or can take a step (doesn't get stuck) unless maybe it terminates with a panic (which arguably is a defined outcome if effect allowed).

They might clarify how they treat a panic in terms of progress/preservation. In type soundness, a panic is usually considered a defined outcome (like the semantics might have a rule for "panic terminates program" which is not a stuck state, it's just a halting state).

So they'd claim well-typed programs either run forever, terminate normally with a value, or terminate via a runtime check (panic), but they do not get into an impossible state like segfault (which they consider outside semantics).

Preservation: If a program state is well-typed (according to typing rules linking runtime store and type context), then after one step or one big-step evaluation, the new state is also well-typed (types of values preserved, etc.). This prevents, for example, an int turning into a string unexpectedly.

Considering pointer and region, this can be complex but presumably they assert the type system prevents alias issues that would break type safety.

Type Safety (Soundness): Summarize the two above into: well-typed programs do not exhibit undefined behavior (no segmentation faults, no contract violations that aren't caught, etc.). This is the high-level guarantee.

They may disclaim that data races in concurrency are outside this safety definition (some specs define type safety as memory safety and partial correctness).

If any other meta properties: maybe mention determinism of single-threaded eval (each step is deterministic given our sequential semantics, which helps reasoning).

They might also mention memory safety as corollary of type safety, since the type system and permission system ensure no double free etc.

7. Contract Semantics: Describe how preconditions, postconditions, and effects are enforced in the runtime model.

Most contracts are enforced statically or via runtime checks that result in panics or error values. Possibly formalize that if a procedure has a must clause, at call the semantics either assumes it's true (if doing partial correctness approach you can assume preconditions hold as a precondition of your theorem) or you could integrate a check that if not, then a panic occurs (like an evaluation rule: before calling body, if pre is false, then call triggers a runtime error/panic).

For postconditions, either you check at return and if false, panic or just consider program invalid if it returns not satisfying (but at runtime they'd likely check).

Effects: have no runtime effect except actual I/O or others, so nothing to do operationally except maybe a note that pure functions (no uses) indeed don't modify global state (the semantics should reflect that if uses is none, there's no rule that modifies certain global parts).

Could mention a formal approach: treat the Hoare triple {P} body {Q} as a separate specification aside from operational semantics. They might not formalize proving them in spec, just state that given how we've enforced, if code passes static checks and doesn't panic, then the postconditions hold (assuming any necessary mathematical truths).

But likely keep it simple:

Static: code won't compile if must not provably satisfied at call? Probably not fully, they don't have theorem prover. They rely on runtime for complex stuff. So at runtime, if must fails, they do dynamic check (some might be optimized out if proved).

Similarly for will: if at return the condition fails, either they do runtime check and panic, or for static known conditions they'd optimize out impossible branches, etc.

So semantics wise: after body executes, if post false, then do a panic or mark program as incorrect.

The effect system I guess is irrelevant at runtime aside from actual effects (like doing I/O if effect allowed).

Possibly mention that their design ensures partial correctness: if preconditions held on call and function doesn't panic internally, then on return the postcondition will hold (this is the guarantee if their contract system is sound, though they might not fully enforce it automatically unless proven by static analysis; but since we have dynamic checks, we can say it's enforced by checking and if it would fail, we don't let it silently continue - we panic).

They likely won't fully formalize a logic for verifying contracts, but mention it.

8. Runtime Behavior: Clarify categories of behaviors at runtime.

Undefined Behavior: List things that are UB (we already said memory misuse, data races maybe). The spec should list all conditions under which the spec no longer guarantees anything (this is typically everything that requires an unsafe block to perform or severe contract violations).

They might incorporate data races as undefined (like in C++ memory model, data race is UB). Many languages do that (Rust included, any data race is undefined if you consider interleavings outside sequentially consistent).

Other UB: calling a function with false preconditions might not be UB if they check it, they'd just panic. But if an unsafe function had a precondition that wasn't met and no check, that can cause UB. So refine:

For safe code, ideally no UB because anything that could go wrong triggers a defined panic or error. For unsafe code, UB is possible.

Possibly mention integer overflow is not UB (because we define it either traps or wraps depending on mode).

Null deref in safe code can't happen; in unsafe code it is UB.

misaligned pointer, etc. UB.

Implementation-defined behavior: Things that depend on platform but are documented, e.g. how exactly a repr(C) struct is laid out or endianness of numeric representation, or size of isize is pointer size. The spec might not deeply list these here, but mention that some things (like FFI calling convention or bit size of certain numeric if not fixed to 32/64 etc. Actually they likely fix them for portability).

Another example: order of destruction for fields or local variables might be unspecified but not affecting correctness. But often they define those for determinism. If not, mention which aspects are left to implementation (like evaluation of function arguments left-to-right is defined, so not unspecified; maybe only minor like how it chooses alignment for user-defined types if not all specified).

Memory layout they mostly specified, maybe the only impl-defined could be things like how the system chooses to allocate on heap (buddy alloc vs something, not relevant in spec).

Possibly concurrency memory model (if they define, or rely on underlying hardware memory ordering for mut references).

It's safe to say: "Unless otherwise specified, the language's semantics are the same across all conforming implementations. In the few cases where behavior depends on the implementation or target platform (e.g., size of usize, endianness), these are documented as implementation-defined and must be consistent within a compilation."

Unspecified behavior: If any (like if something not pinned down intentionally e.g. evaluation order if we hadn't defined it, but we did define left-to-right, so none there).

Perhaps order of evaluation of independent expressions if we left any unspecified? We tried to define all. Or iteration order in a HashMap is unspecified, but that's library, not language semantics.

Could skip if none at language level beyond minor nondeterminism in concurrency scheduling (which concurrency part will talk about).

Summarize defined vs undefined clearly: code that stays within safe and respects language rules will not hit UB; any time UB is invoked, all bets off (thus soundness would be void beyond that).
Purpose: This final part ensures that the language spec isn't just informal but has a rigorous foundation. It gives a measure of confidence (type soundness theorem) that the design works as intended (no segfaults unless you went unsafe, etc.). It's likely more condensed than our prose outlines above, since formal rules can be quite dense but they might keep them high-level in spec text.
This part ties heavily to earlier definitions (the rules will reference grammar from Part I lexical and abstract syntax, and types from Part II for static context, and uses the categories defined in semantics headings above). For maintainability, each sub-section in this part corresponds to a group of rules or a theorem, which is good for referencing.
A developer not interested in proofs might skip this part, but it's crucial for completeness and for those implementing the language to know the intended semantics precisely. The file 13_Semantics.md would be created containing these formal rules, possibly in LaTeX math or similar notation.

Part XIV: Concurrency (Optional)

File: Spec/14_Concurrency.md (Concurrent Execution Model) – if the language includes concurrency primitives.
Contents: Describes threads, synchronization, atomic operations, channels if supported. This part may be omitted or minimal if concurrency is not fully designed yet (as indicated by "if applicable"). If included, required sections (in unsafe or advanced category):

1. Concurrency Overview: States the concurrency model (likely threads and shared memory, no green threads mentioned). If using OS threads, mention that. If no GC, memory is still manual but multi-thread, meaning some things like an own T can be moved to another thread if thread spawn transfers ownership. Possibly tie in effect system: they had thread.spawn and thread.join effects to ensure static tracking of concurrency usage. But that doesn’t prevent data races. However, they might enforce that if you want to share data between threads, you use atomic or synchronization.

Clarify safety: concurrency introduces potential data races which are not caught by type system (since multiple mut references can exist, one per thread easily if you send one to thread without restricting original...). So they rely on programmer to use locks or mark variables as atomic.

Might mention memory model: maybe sequential consistency for data-race-free programs, but if data race occurs, it's UB (like C++ model). If they follow that, state it. Possibly they assume that strongly (like Rust does).

Data races mention in limitations earlier suggests they treat data races as UB (which is typical in languages aiming for performance because you rely on CPU memory model).

Might not detail memory ordering beyond that in spec, leaving it for atomic section.

If no complex memory model section, at least mention "The behavior of unsynchronized concurrent writes (data races) is undefined" which is standard.

2. Threads: Syntax/Mechanism to create and manage threads.

Likely via a spawn procedure or a standard library call. Possibly:

procedure spawn(f: function() -> ()) uses thread.spawn

which creates a new OS thread running function f. Or they might allow a block syntax like thread { ... }.

The effect thread.spawn in uses is to mark that function spawns threads.

Joining: thread.join effect for waiting on thread completion. Possibly a join(threadHandle) function.

Life-cycle: threads start, run concurrently, can finish returning a value which can be obtained via join, or detach. If procedure main ends, likely program ends even if threads running (unless they specify otherwise).

Possibly mention threads share memory with the parent (like POSIX threads).

So, if multiple threads access mutable data concurrently without locks, it's a data race (UB).

The language might encourage message passing (channels) to avoid data races, but they included locks and atomic topics so they foresee shared memory usage.

3. Synchronization: List provided primitives (maybe library provided but conceptually important).

Mutex locks: provide mutual exclusion. Possibly an Mutex<T> type in library with lock/unlock methods.

If Mutex is poisonable if thread panics while holding it (like in Rust)? Possibly skip detail.

Condition variables if any, to wait on conditions (the outline says if applicable).

Outline "Synchronization effects": possibly using effect system to track usage of locks? Not sure how they'd track that, unless they have something like must hold lock effect (not likely).

They might just include an effect for using locks (some effect categories maybe in effect list like thread._ or sync._).

But likely no static enforcement beyond marking that this function uses locks (for documentation).

So effectively, just describe how a lock is used to protect shared data.

Maybe mention that with proper locking, you avoid data races and then memory operations are sequentially consistent as expected.

4. Atomic Operations: If they have atomic types (like atomic integers) and operations on them for lock-free concurrency.

Possibly a Atomic<T> for T that is Copy (like int).

Provide operations like atomic load, store, fetch_add, compare_and-swap (CAS).

Memory ordering: if they don't want to specify multiple orderings in first version, they might say all atomic ops are sequentially consistent (SC) by default for simplicity, or maybe provide relaxed, acquire-release, etc. The outline implies memory ordering is considered (calls it out explicitly).

Given this is advanced, they might list them similar to C++: atomic ops with different orderings and their meaning. That can be quite heavy for spec but maybe minimal:

default is sequentially consistent,

or they mention Acquire/Release and that to use them properly to avoid overhead.

Could possibly skip details if not certain, but if included:

define what "atomic" means: multiple threads can use atomic variables without locks and results are well-defined as per chosen memory model.

e.g. SC means operations appear in some total order consistent with program order.

If they include others: define Acquire prevents memory reordering before, Release prevents after, etc., but might be too detailed.

At least mention CAS (compare-and-swap) to build other sync primitives. Outline does mention "compare-and-swap".

They likely treat these as safe operations because using them correctly ensures no UB even without locks (except misuse can still be logic bug but not memory unsafe).

The effect system might have an unsafe._ effect but not needed specifically for atomic (maybe categorize atomic under sync effects, but they may consider atomic operations safe, no effect needed beyond possibly sync._ but outline doesn't show a separate category, just atomic under concurrency section).

5. Channels (if applicable): If they support CSP-style messaging, mention channel types (like bounded/unbounded channel between threads).

Possibly provided as library too, but spec can mention semantics:

Channel<T> with send and receive operations.

Send is usually non-blocking if space, or blocking if full (depending on design).

If unbounded, send always succeeds (just memory overhead).

Receive blocks if empty (or returns an Option if non-blocking variant to check).

Deadlock concerns not covered by spec except to caution user.

Channels are safe if used correctly (lack of consumption can lead to memory leak maybe).

They treat these as high-level concurrency building block.

6. Concurrency Effects: Possibly describing how the effect system interacts (like any use of thread spawn or atomic operations should be declared).

They explicitly listed thread effects and sync effects in effect clauses earlier.

So a procedure that spawns a thread must list uses thread.spawn (and maybe thread.join if it waits).

If it uses locks, perhaps uses thread.sync or something (depending on how granular they went).

Atomic operations might be considered as unsafe.ptr effect or maybe not needed (they had unsafe.ptr for raw pointer likely, not for atomics).

Possibly they just consider lock operations as part of thread._ or sync._ effects.

We should mention that if a function does concurrency (spawns threads, uses shared mem), it should declare the appropriate effect so that, e.g., pure code won't spawn threads unexpectedly.

This static info can help at higher-level reasoning (like to know if a library function might start threads or block, etc).

Also mention that the effect system does not guarantee freedom from data races; it only tracks the usage of concurrency primitives.

The contract system might incorporate some thread safety conditions if any (likely not at this stage).

7. Data Races: Reiterate what constitutes a data race and that it is not allowed (undefined behavior).

Data race defined as two threads accessing same memory location at least one is write and not synchronization, without ordering.

If that happens, the language makes no guarantees (so it’s a program error from spec perspective).

Encourage always using locks or atomics to protect shared mutable state.

Possibly mention that if user follows that, then the program is data-race-free and thus behaves in a sequentially consistent manner (assuming their underlying memory model).

They likely reflect the common knowledge: data race is basically an "unsafe" behavior. So treat it as "undefined".

Might also mention that detection of data races is not done by the language (the compiler won't catch it for you).
Purpose: If concurrency is indeed included, this part ensures developers and implementers know how threads and sync are supposed to work in Cursive. It heavily references Memory model and Unsafe part, since concurrency is where many of those unsafety aspects appear. The effect system integration is unique to Cursive (most languages don't track concurrency via effect), which is a neat idea for documentation at least.
If concurrency is not fully implemented in initial release, this part might be less detailed or omitted, but we've structured it for completeness. The file 14_Concurrency.md would hold this content, likely marked as optional in the spec (like "This section applies only if concurrency features are provided by the implementation").

Part XV: Foreign Function Interface (FFI)

File: Spec/15_FFI.md (Interoperability with C and other languages)
Contents: Describes how Cursive code can call and be called from foreign code (especially C), including any syntax for external declarations, and rules for memory layout to match C (like repr(C)). Required sections:

1. FFI Overview: States that Cursive supports calling foreign functions and being linked into foreign programs, aiming for zero-cost interop with C when using repr(C) types.

Outline the typical use: you can declare an external function (in a C library) and call it, or mark a Cursive function to be exported with a C ABI so that C code can call it.

Safety: interacting with foreign code is unsafe because the language can't enforce its invariants on that code.

There's no automatic marshalling aside from basic conversion (basically expecting you to use compatible types).

Perhaps mention that beyond C, other languages can be interfaced if they have compatible calling conventions or via C as a middle layer.

Possibly mention the role of effect system: ffi.call effect should be used for any foreign calls (to let static analysis know you're doing FFI which might throw exceptions or violate pure, etc.). Actually, they listed ffi.call in effect taxonomy, so yes.

2. FFI Syntax: Likely something like:

extern "C" function declarations for imports (like external procedure printf(fmt: \*i8, ...): i32 or such).

Or an attribute: [[foreign(name="printf", conv="C")]] procedure printf(...).

Also how to specify the calling convention (C by default, maybe support others if needed).

Possibly mention that foreign functions are always unsafe to call (like require unsafe block).

For exporting: maybe external procedure could also define a Cursive function to use a foreign name or conv (like in C++ you do extern "C" void foo() { ... } to expose with C ABI). They might allow similar.

If not formal, at least say: to use a C function, declare it with the same signature in Cursive and mark it extern "C", then call. To expose, define the function normally but mark it [[no_mangle]] and perhaps [[foreign(export)]] or just rely on linking by naming convention if available. Possibly an attribute or effect? Actually effect not for definition, just calls.

They should also mention how to handle varargs (like C's ...). Possibly they might not support calling varargs easily except maybe through some built-in interface or not at all at first (could skip).

Linkage specifiers: like static vs dynamic linking maybe not in spec deeply, but mention if there's a way to tell linker which library to link for a given extern (some languages have attributes for library name).

Simpler: you call the foreign function, the linking is done either by linking the library at compile time or dynamic load, but spec likely leaves that to build system, not language.

3. Type Compatibility: Outline which Cursive types correspond to which C types and how to ensure layout aligns.

repr(C) attribute is key for structures to ensure field alignment and padding match C compiler (assuming both use similar rules, which if abiding by common ABI they should).

Basic type mapping: e.g. i8 to C int8_t or char depending on usage, i32 to C int32_t (or just int on typical 32-bit, but better to use stdint exact sizes).

Pointers in Cursive (like mut T or imm T) correspond to C pointers in that mut T roughly like T\* because it allows writing. But not exactly because mut T ensures some safety, but at ABI level it's just a pointer value.

The differences in calling convention for value vs reference: an own T passed might be by value if small or by hidden pointer if large (like in C++). But simpler is to consider own T is by value as if passing that struct by value in C (so abide by ABI for that).

They might say: use repr(C) on struct if passing to C, and ensure no types that don't exist in C (like Result not directly, but you can always treat it as a struct).

bool in Cursive might map to \_Bool in C (1 byte possibly).

char in Cursive was a Unicode code point (32-bit), no direct C analog except maybe uint32_t.

At least list major ones.

Also mention endianness: The underlying hardware endianness is used for multi-byte values, so if interop with a specific system, that matches what C uses on that system (so just caution that cross-endian needs conversion).

If any hidden differences: e.g. Cursive arrays vs C arrays – but if you pass a slice [T], in Cursive it's a fat pointer (data+len), whereas C would see just a pointer. So if passing a slice to C, how does that happen? Possibly they wouldn't allow that implicitly. The programmer would have to pass slice.ptr and slice.len separately. There might be bridging functions.

Document that reference types in Cursive (mut T, imm T) are not automatically recognized by C, but since at ABI it's just a pointer, you have to use compatible pointer types. e.g. mut i32 in Cursive call expecting a int\* in C is fine as both are a 64-bit address passed. But constness isn't enforced across, so be careful if you pass imm T to a function that writes to it in C, that's violating your promise.

They might not delve that deep, but at least mention that such mismatches are possible and are responsibility of unsafe code.

Also mention struct layout: with repr(C), fields will appear in order and alignment same as if defined in C with equivalent field types. Without repr(C), not guaranteed (though likely they'd still be same by default, but not guaranteed if they allow some reordering for optimization or unstable).

If union allowed in Cursive (not sure if union is supported as sum type or union type? They didn't explicitly list union as first-class type but could treat an enum with repr(C) and all variants as same type as union).

When interfacing with a C union, maybe you'd have to use raw pointer and manual memory or some such if lacking union type.

Also mention how string is handled: String in Cursive is not just a char pointer, to pass to C maybe call something to get a pointer to internal buffer (like a C string), because String is an owning struct (like length + pointer). Possibly not straightforward, so they'd consider conversions or copy to a C buffer.

Might skip that detail and just say if you need to pass strings, use pointers to char (like mut i8 as buffer).

4. FFI Effects: Reiterate that calling foreign functions is considered an effect (ffi.call) that must be declared and is always unsafe.

This ensures static knowledge of where external calls happen.

Possibly mention that FFI calls might throw exceptions (C++ exceptions) which if not caught will blow up, and since no exception handling in Cursive, that leads to UB unless you mark them nothrow or handle via std::terminate. They might not mention C++ specifically, maybe focusing on C where no exceptions.

But if concurrency, or signals (C signals), or longjmp, those can all interfere unpredictably. Could say they are not recommended or undefined to use across FFI boundary into Cursive code (like a longjmp that jumps out of a Cursive function bypassing destructors).

Possibly beyond scope but mention "Note: throwing C++ exceptions through Cursive frames or using longjmp to bypass them results in undefined behavior, as it would in C/C++ between languages".

FFI effect basically signals "this might do anything because it's foreign".

5. Unsafe Operations (for FFI): Usually, interacting with FFI often requires unsafe, e.g. if you get a pointer from C, converting it to mut T is unsafe but necessary. Already covered in Unsafe part. But might highlight here some specifics:

Must ensure any data from C meets Cursive's alignment and validity requirements.

If a C function returns a pointer to memory, the Cursive code now has an \*T raw pointer that it should handle carefully (maybe wrap in an own T if it takes ownership and eventually free via another call or know it's static memory and not free at all).

The spec might not provide a standard way to free memory allocated in C directly unless through another call, so user must call the appropriate free function from C.

If passing an own pointer to C that C then frees, that is dangerous as Cursive might also try to free it later unless you relinquish it by mem::forget or such (not sure if they allow mem::forget to leak intentionally).

So ensure the user either uses Result or protocol to avoid double free between languages.

All this is really usage guidelines.

They already listed "FFI safety rules" in the outline, presumably meaning things like: both sides must agree on calling conv, memory ownership responsibilities must be clear (like if C allocs memory, Cursive should free only if documented and using correct free).

Could mention you should mark Cursive struct as repr(C) if passing to C, etc (which was earlier).

6. Interoperability Examples: Provide a couple of examples to clarify how to do it.

E.g. declaring and calling a simple C function like int add(int, int) from Cursive.

Or exposing a Cursive function to C, e.g.

// C side
void do_something(int32_t x);

and in Cursive

[[no_mangle]]
procedure do_something(x: i32) {
...
}

so the C program can link to it.

Also a struct example: define the struct in C, and in Cursive with repr(C).

Possibly how to handle a callback: passing a function pointer from Cursive to C or vice versa (like qsort comparator). That might require additional features like closure to fn pointer conversion. If too advanced, skip.

Best practices:

ensure to include appropriate headers or prototypes to avoid mismatches,

test FFI boundary thoroughly,

caution with memory and allocate/free on same side,

ensure thread local or global state is properly managed if crossing boundary,

if foreign function might call back into Cursive, be careful about reentrancy or static state.

The spec may not go deep into these, but some pointers (pun intended).
Purpose: This part is crucial for systems programming because it allows using existing libraries and OS interfaces. The spec must cover enough that an implementer can get the calling conventions and type alignments correct to be ABI-compatible with C (which influences code generation). It references Part I (attributes like no_mangle), Part VII (visibility if needed to keep some functions external), Part XII (unsafe for calls), and the Memory (for layout). 15_FFI.md ensures that writing bindings to C or writing Cursive code to be called from C is well-defined. It fosters adoption because without FFI, a new language can't use OS facilities easily. The level of detail can be moderate – no need to re-explain C in full, just the points of contact. Possibly more details could be left as "the implementation shall follow the platform ABI for the C calling convention", which covers many aspects implicitly.

Part XVI: Built-in Functions and Standard Library

File: Spec/16_Builtins.md (Core Built-in Operations and Standard Library Overview)
Contents: Summarizes any compiler built-in functions (like sizeof, assert, etc.) and gives an overview of the standard library (which is outside the core spec but important to mention). Since a full standard library spec is huge, they likely only outline major components. Required sections:

1. Built-in Functions Overview: Explain difference between built-in and ordinary library:

Built-ins are handled specially by the compiler (either as intrinsics or directly recognized), often for low-level operations or to implement language semantics (like memory operations).

Standard library functions are provided in source or binary but not hardcoded in compiler (except maybe some known).

Possibly mention which ones are built-in vs just library.

E.g. in C, memcpy often is intrinsic, or rust has some intrinsics for atomic or simd.

They might list things like malloc isn't built-in, but size_of<T> could be a compile-time built-in (maybe a function or an operator).

Outline mentions "Memory Functions" – probably akin to C's malloc/free or new. If they consider them built-ins or part of library.

Could be a alloc function in library that calls underlying system, but in spec they might treat as library.

Possibly they consider a set of functions that are always available as predeclared (like we listed in predeclared identifiers maybe).

2. Memory Functions: Common ones:

Allocation: if not wanting to expose manual free normally, maybe they have a new that returns an own T for heap allocated T (like calling malloc and constructing).

Or simpler: alloc_heap<T>() -> own T – that calls underlying, and maybe you need to call free or drop of own automatically frees, so maybe they provide to allocate and then rely on RAII to free.

free might not exist explicitly (since drop covers freeing).

But maybe alloc_in (region) and alloc_stack or such which we've covered differently.

Size functions: e.g. size_of<T>() -> usize to get size at compile time, that could be a built-in or constant function.

Alignment queries: align_of<T>().

Possibly offset_of field in struct if needed.

Those are helpful in FFI and maybe considered built-ins since they are compile-time or simple queries.

They mention alignment functions explicitly, so yeah align_of likely.

Memory copy, set: maybe built-in like a memcpy or memmove or library functions.

But if included, mention that the compiler may optimize calls to these built-ins with intrinsic knowledge.

3. Collection Functions: e.g. operations on arrays or strings that might be built-in:

Perhaps len(arr) built-in to get array length (if they have it).

But if array length is part of type, not needed as function, maybe slice.len() if slice has property.

Outline lists "Vec, HashMap" in standard library summary, but those are not built-in, just part of library.

For built-ins: not sure any specifically, maybe array concatenation or such could be library.

Standard library likely has a Vector type and push/pop etc. They might mention them here as common data structures.

4. Type Functions: Possibly compile-time type queries as I said:

is_signed<T>(), max_value<T>(), etc. Or a type conversion function for safe casts as() operator which might be built-in.

If reflection or typeid features, probably not now, skip.

The outline might mean like type_of(x) etc or cast operations which are already in language semantics (like from memory part or expressions we did mention type conversion).

"Type conversions" listed could refer to built-in casts (like numeric conversion built-ins e.g. int_to_float if needed).

"Type size/alignment queries" as already discussed built-ins.

5. System Functions: Things interfacing with environment:

I/O: probably not built-in, they would be library calls (like printing etc).

But they mention uses io.read io.write in effect, implying some runtime function or syscalls behind that.

Could mention that standard lib has modules for file and network I/O that correspond to those effect tags (like an IO module with read_file, write_file, etc. using OS calls).

System calls: maybe not direct, presumably via library or FFI to C.

Panic and abort: panic(msg) could be a built-in function to raise panic (calls runtime or abort).

Possibly thread spawn/join will be library functions but with known effect.

No direct memory management beyond what's said in Memory functions if they had any.

Possibly here they note hooking into OS (like environment variables, program args).

It's likely minimal: just mention that you can use FFI or standard library to call OS.

6. Debug Functions: Perhaps an assert(condition) macro or built-in as in C.

They did mention debug vs release differences for assert in statements part. Possibly assert(cond) is a built-in that either calls panic if false in debug mode, and is no-op in release mode if they've concept of release mode controlling it.

Or might treat assert always as just a macro that calls panic with a message that includes file and line (like a built-in macro).

Also print or eprintln might be considered part of standard library, not built-in, but could mention as common debug output.

"Debugging aids": maybe a dump(x) that prints internal rep or something? Not sure.

assert_eq perhaps as library macro.

Possibly mention they plan a debugging/tracing facility but that’s likely outside core spec.

Covering at least assert (which they did in grammar? We added assert statement optional. If it's in grammar, then it must be built-in).

If it's a macro or keyword, either way, define it.

7. Standard Library Overview: Summarize what exists beyond core language.

They list common modules: Option, Result (we already detail, but they are kind of language core), Vec (growable array), HashMap, strings, I/O streams, etc.

Clarify that the standard library provides a base set of containers, algorithms, etc., but is specified separately (maybe not fully in this doc).

Possibly mention that the spec covers language only, but the library follows certain conventions.

If they have multiple layers (like a core minimal library vs a full lib), mention if relevant (like std vs core in Rust).

Provide links or references (if this spec were online, they'd link to library docs).

Emphasize that things like concurrency primitives might be in library but follow the semantics given in concurrency part.

Possibly note that some library functions are implicitly unsafe or have effect (like if open file requires effect fs.read etc., the library functions will have uses fs.read and be implemented accordingly).

This section likely remains high-level, maybe even a bullet list of main library features.
Purpose: This part acknowledges that beyond the language specification, practical usage relies on a standard library. It helps ensure the spec doesn't have to define every possible operation but at least points out that certain needed functionalities exist. For example, we didn't specify how to open a file in the core language (because that’s library job), but we mention that the library covers it. That prevents someone from thinking "there's no way to do X in this language" because it's just not in the core spec.
It's likely a lighter section and might be partially non-normative, but including it is good for completeness and orientation for new users reading the spec. The file 16_Builtins.md collects a miscellaneous set of items (some normative like built-in functions behaviors, some informative like listing library modules). If the spec were to be complete, possibly Option and Result would be defined here as part of standard types, but they integrated them in error handling part instead, which is fine.

Appendices

We also maintain Appendices as separate files for quick reference and supplementary details:

Appendix: Formal Grammar (appendix_grammar.md): Contains the complete grammar of Cursive in EBNF (both lexical and syntactic). This must be kept consistent with all earlier parts. For maintainability, grammar productions in each part (like expressions grammar, statements grammar given in spec text) should match this consolidated grammar. It might list nonterminals and reference sections where they're explained. Also include any well-formedness rules not easily captured in grammar (like restrictions on continue usage, etc.).

Appendix: Error Codes and Diagnostics (appendix_errors.md): List all compile-time and runtime error codes or categories, with descriptions. E.g., E001 might be "Unresolved identifier", E002 "Type mismatch", etc., along with suggested message text and maybe references to spec sections. Also covers runtime error categories (like panics due to contract violation vs out-of-bounds) possibly with different codes or messages. This helps implementers ensure consistent diagnostics. The outline suggests verifying cross-references, ensuring each error scenario in spec correlates to an entry here.

Appendix: Operator Precedence (appendix_precedence.md): Contains a table of all operators with their precedence levels and associativity. This complements Part III (Expressions) by providing a quick reference rather than searching through text. It should list from highest (like field access, call) to lowest (maybe assignment) with a brief note on associativity (left, right, none). Possibly includes unary vs binary distinction. This is especially helpful for users to disambiguate complex expressions.

Appendix: Keywords (appendix_keywords.md): Lists all reserved keywords and contextual keywords. Reserved cannot be used as identifiers, contextual have special meaning only in certain contexts (like owns maybe if not reserved globally, or where might be contextual in generics). Also list deprecated keywords if any (the outline mentions needs/requires/ensures replaced by uses/must/will – presumably those were old syntax now reserved to avoid confusion). And future reserved (names we plan to use so don't use them as id, like async perhaps if plan future).

Appendix: Naming Conventions (appendix_naming.md): The spec encourages certain naming style (CamelCase for types, snake_case for variables/functions, SCREAMING_SNAKE for constants, etc.). This appendix codifies those as guidelines or requirements (maybe not strictly enforced by compiler, but could be by lints). It's partly style guide. But since the outline specifically says to create it, likely important for consistency and teaching. It can also mention identifiers rules like no leading underscore for exported items, etc., if any policy.

It might reference how standard library follows these and suggest user code do same.

Additional Documentation Files

Beyond the spec files, a few other documents assist in development and maintenance:

ISSUES.md: A file for internal tracking of open issues, design questions, and future features. This is not part of the formal spec but rather a living list the team uses to remember what needs resolution or addition. For example, "Should we add covariance for generics? (Issue #25)" or "Memory model under concurrency – finalize if data race UB or not." This is not distributed to end users typically, but the outline includes it so probably in the repository. It's helpful to link relevant spec sections to each issue for context.

README.md: A top-level readme for the specification repository. It should provide an overview of what the spec contains, how to navigate it, and any version info. Possibly a quick start on reading spec or building the spec docs. Also mention contribution guidelines (if open source development of spec), and how the spec versioning works.

It can also include a list of files (Parts I-XVI, Appendices) with short descriptions, so new readers know where to find what.

The outline suggests including cross-reference guide (maybe how to cite or link to sections) – maybe they plan an index or cross-ref table separate, but at least some directions.

Finally, a Summary of Actions Required (as in outline's end) enumerates which existing parts needed verification and which new files to create. In our context, we've now incorporated the best of all outlines into one unified structure.

To build out the full specification:

We will verify and integrate existing content from 01_FrontMatter.md, 02_Types.md, 03_Expressions.md, 04_Contracts.md into the above outline sections for Parts I–IV.

Then create new files for Parts V–XVI and Appendices as specified, fleshing them out according to this outline.

Ensure throughout to avoid redundancy, use consistent terminology (e.g. "procedure" vs "function" distinction clear, use "must clause" not "requires", etc.), and maintain forward/backward references properly (like Part IV can refer to effect clauses and Part IX covers effect details, etc.).

The structure is designed to be modular: each part stands as its own file focusing on a coherent aspect, and within each, sections follow a consistent pattern (Overview, then subdivisions by concept, and within those by categories and then by required sub-sections such as syntax, semantics, examples as needed). This will facilitate both navigation (readers can jump to the chapter they need, e.g. "I want to see how modules work, go to Part VIII") and maintenance (one file can be updated without confusing other parts, cross-references keep them connected).

Normative references: each file should have references to others like "as defined in §3.2" for clarity when needed, but avoid duplicating definitions. For example, Part V (Statements) when talking about expression statements can refer to Part III for expression syntax.

Finally, the spec should be versioned (v1.3.0 as per outline) and include a change log if applicable (though outline did not specify, sometimes an appendix might have changes from previous versions, but since this is initial version maybe not needed yet).

This finalized outline picks the strongest elements from the proposals:

We adopted the sequential part structure (Parts I–XVI) aligning with numbering from Outline 3/Unified (for cross-ref ease).

We kept the comprehensive coverage of Outline 1 (complete-language-spec), ensuring each language feature (modal types, effect system, etc.) is included.

We used the status tracking and integration notes idea from Outline 3 in the action plan (to know what content exists vs to be written).

We followed the LanguageSpecGuide template closely in ordering (intro → lexical → types → expressions → statements → functions → declarations → modules → memory → advanced), which ensures no forward references to unknown concepts.

We have avoided redundancy by referencing rather than repeating information (e.g. not re-describing syntax of expressions in statements part, but referencing it).

The modular breakdown into many smaller files (rather than a few monolithic chapters) was chosen from the unified outline to improve maintainability (one can update the Memory model part without touching others, etc.).

Normative referencing is supported by stable part.section numbering and the separate Appendix with grammar and index of terms.

The entire structure is designed to be navigable (with clear part and section titles matching topics, and an index and naming convention appendix to quickly find things).

With this outline agreed upon, the next step is to implement it by updating existing spec files and creating the new ones as listed, populating each section with the necessary details and formal definitions. This will result in a complete Cursive Language Specification document that is well-organized, cross-referenced, and ready for both implementers and users to follow.
