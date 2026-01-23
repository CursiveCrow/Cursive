# The Cursive Language Specification

**Part**: I - Foundations  
**File**: 01_Foundations.md  
**Previous**: (Start) | **Next**: [Type System](02_Type-System.md)

---

## Abstract

Cursive is a systems programming language designed for memory safety, deterministic performance, and AI-assisted development.

It achieves these goals through:

- **Lexical Permission System (LPS)**: Compile-time memory safety without garbage collection or borrow checking
- **Explicit Contracts**: Preconditions and postconditions as executable specifications
- **Effect System**: Compile-time tracking of side effects, allocations, and I/O
- **Modal System**: State machines as first-class types with compile-time verification
- **Memory Regions**: Explicit lifetime control with zero-overhead allocation
- **Comptime Metaprogramming**: Compile-time code generation without macros
- **File-Based Modules**: Code organization through file system structure

Cursive compiles to native code with performance matching C/C++ while providing memory safety guarantees through region-based lifetime management.

### Design Philosophy

1. **Explicit over implicit** - All effects, lifetimes, and permissions visible in code
2. **Local reasoning** - Understanding code requires minimal global context
3. **Zero abstraction cost** - Safety guarantees without runtime overhead (no garbage collection pauses or virtual machine needed)
4. **Predictable patterns** - Consistent syntax and semantics for AI code generation
5. **Simple ownership** - No borrow checker complexity
6. **No macros** - Metaprogramming through comptime only for predictability

### Safety Model

- **Prevents**: Use-after-free, double-free, memory leaks
- **Provides**: Deterministic deallocation, zero GC pauses
- **Does NOT prevent**: Aliasing bugs, data races *(these remain the programmer's responsibility)*

## Conformance

An implementation conforms to this specification if and only if it satisfies all normative requirements stated herein. Extensions MUST NOT invalidate any program that is valid under this specification. Diagnostics, implementation-defined limits, and unspecified behavior MUST be documented.

### Key Words for Requirement Levels

The key words **MUST**, **MUST NOT**, **REQUIRED**, **SHALL**, **SHALL NOT**, **SHOULD**, **SHOULD NOT**, **RECOMMENDED**, **MAY**, and **OPTIONAL** in this document are to be interpreted as described in RFC 2119 and RFC 8174 when, and only when, they appear in all capitals.

### Document Conventions

- Code blocks for the language use the fenced code label `cursive`.
- Grammar productions use `ebnf` fences.
- Mathematical judgments are typeset with standard notation.
- Error identifiers (e.g., `E1001`) are normative and MUST be reported for corresponding violations.

---

## 1. Notation and Mathematical Foundations

**Definition 1.1 (Mathematical Foundations):** This section establishes the mathematical notation, metavariables, and judgment forms used throughout the Cursive Language Specification to formally describe syntax, typing rules, and operational semantics.

### 1.1 Overview

**Key innovation/purpose:** Provides a formal mathematical foundation for precisely specifying language semantics through standardized notation, judgment forms, and inference rules.

### 1.2 Grammar Notation

Grammar productions are expressed in Extended Backus-Naur Form (EBNF):

| Symbol | Meaning |
|--------|---------|
| `::=` | is defined as |
| `\|` | alternatives |
| `*` | zero or more |
| `+` | one or more |
| `?` | optional |
| `~` | negation |
| `[ ]` | optional elements or character classes |
| `( )` | grouping |

Complete productions are in Appendix A.1.

### 1.3 Metavariables

The following metavariable conventions are used throughout the specification:

**Program Entity Metavariables:**

```
x, y, z ∈ Var          (variables)
f, g, h ∈ FunName      (function names)
m, n, o ∈ ProcName     (procedure names)
R, S, T ∈ RecordName   (record names)
E, F, G ∈ EnumName     (enum names)
```

**Type and Expression Metavariables:**

```
T, U, V ∈ Type         (types)
e, e₁, e₂ ∈ Expr       (expressions)
v, v₁, v₂ ∈ Value      (values)
p, p₁, p₂ ∈ Pattern    (patterns)
```

**Modal and Effect Metavariables:**

```
@S, @S', @S₁ ∈ State   (modal states)
ε, ε₁, ε₂ ∈ Effect     (effects)
Θ ∈ TransitionSet      (state transition relation)
```

**Contract and Assertion Metavariables:**

```
P, Q, R ∈ Assertion    (contract assertions)
{P} ... {Q}            (Hoare triple notation)
```

**Context Metavariables:**

```
Γ, Γ' ∈ Context        (type contexts: Var → Type)
Σ, Σ' ∈ StateContext   (state contexts: modal state tracking)
Δ, Δ' ∈ RegionCtx      (region contexts: stack of active regions)
ε_ctx ∈ EffContext     (effect contexts: available effects)
σ, σ' ∈ Store          (memory stores: Location → Value)
```

**Context Operations:**

- `Γ, x: T` means "context Γ extended with binding x: T"
- `σ[ℓ ↦ v]` means "store σ updated so location ℓ maps to value v"

### 1.4 Judgment Forms

Judgments are formal statements about programs. Each judgment form has a specific meaning:

```
Γ ⊢ e : T                         (expression e has type T in context Γ)
Γ ⊢ e : T@S                       (type T in modal state @S)
Γ ⊢ e : T@S ! ε                   (type T, state @S, effect ε)
Γ ⊢ e ! ε                         (expression e has effect ε)
{P} e {Q}                         (Hoare triple: if P holds before e, Q holds after)
σ ⊨ P                             (store σ satisfies assertion P)
⟨e, σ⟩ → ⟨e', σ'⟩                 (small-step reduction)
⟨e, σ⟩ ⇓ ⟨v, σ'⟩                   (big-step evaluation)
@S₁ →ₘ @S₂                        (state transition via procedure m)
```

### 1.5 Formal Operators

**Set Theory:**

```
∈       (element of)               x ∈ S           "x is an element of set S"
⊆       (subset)                   A ⊆ B           "A is a subset of B"
∪       (union)                    A ∪ B           "union of sets A and B"
∩       (intersection)             A ∩ B           "intersection of sets A and B"
∅       (empty set)                ε = ∅           "effect ε is the empty set (pure)"
×       (cartesian product)        A × B           "product of sets A and B"
```

**Logic:**

```
∧       (conjunction)              P ∧ Q           "P and Q both hold"
∨       (disjunction)              P ∨ Q           "P or Q (or both) holds"
¬       (negation)                 ¬P              "P does not hold"
⇒       (implication)              P ⇒ Q           "if P then Q"
⟺      (bi-implication)           P ⟺ Q           "P if and only if Q"
∀       (universal quantification) ∀x. P           "for all x, P holds"
∃       (existential quantification) ∃x. P         "there exists x such that P holds"
```

**Relations:**

```
→       (maps to / reduces to)     x → y           "x maps to y" or "x reduces to y"
⇒       (implies / pipeline)       P ⇒ Q           "P implies Q"
⇓       (evaluates to)             e ⇓ v           "expression e evaluates to value v"
≡       (equivalence)              e₁ ≡ e₂         "e₁ and e₂ are equivalent"
⊢       (entails / proves)         Γ ⊢ e : T       "context Γ entails e has type T"
⊨       (satisfies / models)       σ ⊨ P           "store σ satisfies assertion P"
```

**Substitution:**

```
[x ↦ v]                            (substitution: replace x with v)
e[x ↦ v]                           (expression e with x replaced by v)
```

### 1.6 Inference Rule Format

Inference rules are presented in the following format:

```
[Rule-Name]
premise₁    premise₂    ...    premiseₙ
----------------------------------------
conclusion
```

The premises appear above the horizontal line, and the conclusion appears below. All premises must hold for the conclusion to be valid.

**Example: Function application**

```
[T-App]
Γ ⊢ f : T₁ → T₂
Γ ⊢ e : T₁
-----------------
Γ ⊢ f(e) : T₂
```

This rule reads: "If f has function type from T₁ to T₂, and e has type T₁, then the application f(e) has type T₂."

### 1.7 Reading Type Rules: Complete Example

This subsection demonstrates how the notation defined above appears in actual specification sections through one comprehensive example showing type checking, effect propagation, and rule application.

**Cursive code:**

```cursive
function read_file(path: string): string
    uses fs.read
{
    // Implementation reads file
}

procedure process_data()
    uses fs.read, io.write
{
    let content = read_file("/data.txt")
    println("Read: {}", content)
}
```

**Type and effect rules:**

```
[Effect-Aggregate-Call]
Γ ⊢ f : (τ₁, ..., τₙ) → U ! ε_f
Γ ⊢ aᵢ : τᵢ ! ε_aᵢ  (∀i)
ε_total = ε_f ∪ ε_a₁ ∪ ... ∪ ε_aₙ
--------------------------------------------------------
Γ ⊢ f(a₁, ..., aₙ) : U ! ε_total
```

---

### 1.8 Diagnostic Format Examples

This section specifies the format and content of diagnostic messages produced by Cursive compilers.

#### 1.8.1 Diagnostic Message Structure

**Definition 1.8 (Diagnostic Message):** A diagnostic message consists of:

1. **Primary message**: Error code and summary
2. **Source location**: File, line, column
3. **Code snippet**: Relevant source code with highlights
4. **Notes**: Additional context and explanations
5. **Help**: Suggested fixes or related information

**Canonical format:**

```
error[CODE]: primary message
  --> file.cursive:line:column
   |
LL | source code line
   | ^^^^^ highlight
   |
note: explanatory note
help: suggested fix
```

#### 1.8.2 Error Categories and Codes

| Category | Code Range | Description |
|----------|------------|-------------|
| Type errors | E4001-E4099 | Type mismatches, inference failures |
| Permission errors | E4003, E4006, E4014-E4015 | Ownership, borrowing, moves |
| Effect errors | E4004, E7C01-E7C03 | Effect availability, propagation |
| Contract errors | E7C02-E7C05, E7C07-E7C09 | Pre/postconditions, invariants |
| Pattern errors | E4005, E5005 | Non-exhaustive matches |
| Control flow errors | E4011-E4013 | Break/continue/return outside context |
| Region errors | E4014-E4015, E3D09 | Escape analysis violations |
| Syntax errors | E1001-E1099 | Parsing failures |

#### 1.8.3 Representative Diagnostic Examples

**E4001: Type Mismatch**

```
error[E4001]: mismatched types
  --> example.cursive:12:18
   |
12 |     let x: i32 = "hello"
   |                  ^^^^^^^ expected `i32`, found `string`
   |
note: expected type `i32`
          found type `string`
help: you can convert a string to a number using `.parse()`
```

**Application of rule:** Type checking requires `Γ ⊢ e : τ₁` and `τ₁ ≡ τ₂` for `let x: τ₂ = e`. Violation when `string ≢ i32` produces E4001.

---

## 2. Lexical Structure

**Definition 2.1 (Lexical Structure):** The lexical structure defines the syntax of tokens—the smallest elements of Cursive source text—including identifiers, literals, keywords, operators, and delimiters.

### 2.2 Characters and Source Text Encoding

**Source Files:**

A Cursive source file is a sequence of Unicode characters encoded in UTF-8.

**Formal properties:**

- Encoding: UTF-8 (REQUIRED)
- Line endings: LF (`\n`), CRLF (`\r\n`), or CR (`\r`) (all accepted, normalized to LF during lexing)
- Newlines are significant tokens: Newlines are preserved as tokens during lexical analysis
- BOM: Optional UTF-8 BOM (U+FEFF) is ignored if present
- File extension: `.cursive` (RECOMMENDED)
- Maximum file size: Implementation-defined (RECOMMENDED ≤ 1 MiB)
- Normalization: Unicode normalization form NFC is RECOMMENDED but not required

**Character Restrictions:**

- NUL bytes (U+0000) are disallowed in source text
- Control characters other than horizontal tab, line feed, carriage return, and form feed are disallowed outside string and character literals

### 2.3 Comments and Whitespace

The lexical productions for comments and whitespace appear in Appendix A.1. Key semantic rules:

**Semantic rules:**

- Line comments extend from `//` to end of line
- Block comments nest: `/* outer /* inner */ outer */` is valid
- Doc comments (`///`) document the item that follows
- Module docs (`//!`) document the containing module
- Doc comments are preserved for documentation generation
- Non-doc comments are stripped before parsing
- Comments do not affect statement continuation (newlines after comments are significant)

**Whitespace:**

Whitespace characters (spaces, tabs, carriage returns) are generally ignored except where they serve as token separators. Newlines are preserved as tokens for statement termination rules.

### 2.4 Identifiers

**Definition 2.2 (Identifier):** An identifier is a sequence of characters used to name variables, functions, types, modules, and other program entities. See Appendix A.1 for the formal production.

**Restrictions:**

- Cannot be a reserved keyword (see §2.5)
- Cannot start with a digit
- Case-sensitive: `Variable` ≠ `variable`
- Maximum length: Implementation-defined (RECOMMENDED: 255 characters)

### 2.5 Keywords

**Definition 2.3 (Keyword):** A keyword is a reserved identifier with special syntactic meaning that cannot be used as an identifier.

**Reserved keywords (MUST NOT be used as identifiers):**

```
abstract    as          async       await       break
by          case        comptime    continue
defer       effect      else        enum        exists
false       forall      function    if          import
internal    invariant   let         loop
match       modal       module      move        must
mut         new         none        own         private
procedure   protected   public      record      ref
region      result      select      self        Self
shadow      state       static      trait       true
type        uses        var         where       will
with
```

**Contextual keywords (special meaning in specific contexts):**

```
effects     pure
```

**Note:** `needs`, `requires`, and `ensures` are DEPRECATED (replaced by `uses`, `must`, `will`).

### 2.6 Literals

Literals are token sequences that directly represent constant values.

#### 2.6.1 Numeric Literals

**Integer Literals:** The lexical forms are defined in Appendix A.1.

**Underscore separators:**

- Underscores (`_`) may appear between digits for readability
- Underscores are ignored during value computation
- Cannot appear at the start or end of the literal
- Cannot appear adjacent to the base prefix (`0x`, `0b`, `0o`)

**Examples:**

```cursive
42          → IntLiteral(42, None)           // No suffix, default i32
```

**Default types:** Unsuffixed integer literals default to `i32`. Floating-point literals default to `f64`.

**Floating-Point Literals:** See Appendix A.1 for the syntactic forms of floating-point literals and their optional suffixes.

**Example:**

```cursive
3.14        → FloatLiteral(3.14, None)       // No suffix, default f64
```

#### 2.6.2 String Literals

The lexical production for `StringLiteral` and the escape rules are defined in Appendix A.1.

**String properties:**

- Strings are sequences of Unicode scalar values
- Escape sequences are processed during lexing
- Multi-line strings are supported (newlines in source → newlines in string)



**Escape sequences:**

| Sequence   | Description     | Unicode Code Point          |
| ---------- | --------------- | --------------------------- |
| `\n`       | newline         | U+000A                      |
| `\r`       | carriage return | U+000D                      |
| `\t`       | tab             | U+0009                      |
| `\\`       | backslash       | U+005C                      |
| `\'`       | single quote    | U+0027                      |
| `\"`       | double quote    | U+0022                      |
| `\0`       | null character  | U+0000                      |
| `\xNN`     | ASCII character | U+00NN (00-7F)              |
| `\u{N...}` | Unicode scalar  | U+N... (up to 6 hex digits) |

**Examples:**

```cursive
"hello"                    → StringLiteral("hello")
"line 1\nline 2"          → StringLiteral("line 1\nline 2")
```

#### 2.6.3 Character Literals

The lexical form of `CharLiteral` is defined in Appendix A.1; escape sequences mirror those for strings.

**Character properties:**

- Character literals represent a single Unicode scalar value
- Must contain exactly one character (or one escape sequence)
- Escape sequences are the same as for string literals

**Example:**

```cursive
'a'         → CharLiteral('a')               // ASCII character
```

#### 2.6.4 Boolean Literals

`BooleanLiteral` is defined in Appendix A.1.

Boolean literals represent the values `true` and `false`. They are keywords and cannot be used as identifiers.

### 2.7 Statement Termination Rules

Cursive uses newlines as primary statement terminators. Semicolons are optional and may be used to separate multiple statements on a single line.

**Primary rule:** A newline terminates a statement unless a continuation rule applies.

**Optional separator:** Semicolons `;` may be used to separate statements on the same line.

**Continuation Rules:**

A statement continues across newlines in exactly four cases:

#### Rule 2.7-1 (Unclosed Delimiters)

Statement continues when `(`, `[`, or `<` remains unclosed.

#### Rule 2.7-2 (Trailing Operator)

Statement continues when line ends with a binary or assignment operator.

Binary operators: `+`, `-`, `*`, `/`, `%`, `**`, `==`, `!=`, `<`, `<=`, `>`, `>=`, `&&`, `||`, `&`, `|`, `^`, `<<`, `>>`, `..`, `..=`, `=>`

Assignment operators: `=`, `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `|=`, `^=`, `<<=`, `>>=`

#### Rule 2.7-3 (Leading Dot)

Statement continues when next line begins with `.` (field access). Use `::` for scope/procedure/effect calls; `::` is not a chaining operator. For chaining of calls, use the pipeline operator `=>`.

#### Rule 2.7-4 (Leading Pipeline)

Statement continues when next line begins with `=>` (pipeline operator).

### 2.8 Token Formation

The lexer uses maximal munch: the longest matching token is chosen.

---

**Definition 3.1 (Abstract Syntax):** The abstract syntax defines the structure of Cursive programs as abstract syntax trees (ASTs), independent of concrete textual representation.

## 3. Abstract Syntax

### 3.1 Overview

**Key innovation/purpose:** Provides a mathematical representation of program structure that separates the essence of code (what it means) from its textual surface form (how it's written), enabling precise formal reasoning.

**When to use this section:**

- When understanding how programs are represented internally
- When reading type rules and operational semantics
- When implementing parsers, type checkers, or interpreters
- When formal reasoning about program transformations

**Relationship to other features:**

- **§2 (Lexical Structure)**: Tokens are parsed into ASTs
- **Type System**: Type rules operate on AST nodes
- **Operational Semantics**: Evaluation rules operate on ASTs
- **Contract System**: Contracts annotate AST nodes

**Why separate abstract from concrete syntax?**

- **Multiple concrete forms**: `x + y` and `(x) + (y)` have different concrete syntax but identical abstract syntax
- **Easier analysis**: Type checking and evaluation operate on ASTs, not strings
- **Formal reasoning**: Mathematical semantics are defined over abstract syntax
- **Implementation**: Compilers work with ASTs internally

**Relationship in compilation:**

```
§2 Concrete Syntax → [Parser] → §3 Abstract Syntax → [Type Checker] → [Evaluator]
     Token Stream              AST                   Typed AST         Values
```

### 3.2 Type Language

**Definition 3.2 (Type Language):** The abstract syntax of types.

```
T ::= i8 | i16 | i32 | i64 | isize          (signed integers)
    | u8 | u16 | u32 | u64 | usize          (unsigned integers)
    | f32 | f64                              (floats)
    | bool                                   (booleans)
    | char                                   (characters)
    | ()                                     (unit type)
    | string                                 (unified string type)
    | [T; n]                                 (fixed array)
    | [T]                                    (slice)
    | (T₁, ..., Tₙ)                          (tuple)
    | (T₁, ..., Tₙ) → U ! ε                  (function type)
    | own T                                  (owned type)
    | mut T                                  (mutable reference)
    | imm T                                  (immutable reference)
    | T@S                                    (type T in modal state @S)
    | ∀α. T                                  (polymorphic type)
    | !                                      (never type)
    | record Name                            (record type)
    | modal Name                             (modal type)
    | enum Name                              (enum type)
    | trait Name                             (trait type)
```

 
### 3.3 Expression Language

**Definition 3.3 (Expression Language):** The abstract syntax of expressions.

```
e ::= x                                     (variable)
    | v                                     (value)
    | e₁ ⊕ e₂                               (binary operation)
    | e₁ => e₂                              (pipeline operation)
    | if e₁ then e₂ else e₃                (conditional)
    | let x = e₁ in e₂                     (let binding)
    | var x = e₁ in e₂                     (mutable binding)
    | f(e₁, ..., eₙ)                        (function call)
    | λx:T. e                               (lambda abstraction)
    | e₁ SEP e₂                             (sequencing, SEP = newline or semicolon)
    | move e                                (ownership transfer)
    | region r { e }                        (region block)
    | alloc_in⟨r⟩(e)                        (region allocation)
    | match e { pᵢ -> eᵢ }                  (pattern matching)
    | loop e { e₂ }                         (conditional loop)
    | loop p in e { e₂ }                   (iteration loop)
    | loop { e }                            (infinite loop)
    | break                                 (loop exit)
    | continue                              (loop continue)
    | return e                              (early return)
    | contract(P, e, Q)                     (contract annotation)
    | transition(e, S)                      (explicit state transition)
    | comptime { e }                        (compile-time execution)
    | e.f                                   (field access)
    | e[i]                                  (array/slice index)
    | {f₁: e₁, ..., fₙ: eₙ}                (record construction)
    | Variant(e)                            (enum construction)

where SEP ::= NEWLINE | ";"                 (statement separator)
```

### 3.4 Pattern Language

**Definition 3.4 (Patterns):** The abstract syntax of patterns for destructuring.

```
p ::= x                                     (variable pattern)
    | _                                     (wildcard)
    | n                                     (integer literal)
    | true | false                          (boolean literal)
    | 'c'                                   (character literal)
    | (p₁, ..., pₙ)                         (tuple pattern)
    | Variant(p)                            (enum pattern)
    | Record { f₁: p₁, ..., fₙ: pₙ }       (record pattern)
    | [p₁, ..., pₙ]                         (array pattern)
    | p if e                                (guard pattern)
    | p as x                                (binding pattern)
```

### 3.5 Value Language

**Definition 3.5 (Values):** The abstract syntax of values (fully evaluated expressions).

```
v ::= n                                     (integer literal)
    | f                                     (float literal)
    | true | false                          (boolean)
    | 'c'                                   (character literal)
    | "s"                                   (string literal)
    | ()                                    (unit/none)
    | (v₁, ..., vₙ)                         (tuple value)
    | [v₁, ..., vₙ]                         (array value)
    | {f₁: v₁, ..., fₙ: vₙ}                (record value)
    | Variant(v)                            (enum variant value)
    | λx:T. e                               (closure)
    | ℓ                                     (location/pointer)
```

### 3.6 Assertion Language

**Definition 3.6 (Assertions):** The abstract syntax of pre/post-conditions and invariants.

```
P, Q ::= none                               (trivial assertion)
       | e₁ = e₂                            (equality)
       | e₁ < e₂ | e₁ ≤ e₂                  (ordering)
       | P ∧ Q | P ∨ Q                      (conjunction/disjunction)
       | ¬P                                 (negation)
       | P ⇒ Q                              (implication)
       | ∀x. P                              (universal quantification)
       | ∃x. P                              (existential quantification)
       | forall(x in c) { P }               (collection quantification)
       | exists(x in c) { P }               (existential over collection)
       | @old(e)                            (value on entry)
       | result                             (return value)

Function Contracts ::=
    uses ε                                 (effect declaration)
    must P                                 (preconditions)
    will Q                                 (postconditions)
```

### 3.7 Effect Language

**Definition 3.7 (Effects):** The abstract syntax of effect signatures.

```
ε ::= ∅                                     (pure/no effects)
    | alloc.heap                            (heap allocation)
    | alloc.heap(bytes≤n)                   (bounded heap)
    | alloc.region                          (region allocation)
    | alloc.stack                           (stack allocation)
    | alloc.temp                            (temporary allocation)
    | alloc.*                               (any allocation)
    | fs.read | fs.write | fs.delete       (filesystem)
    | net.read(d) | net.write               (network)
    | time.read | time.sleep(duration≤d)    (time operations)
    | random                                (non-determinism)
    | thread.spawn(count≤n) | thread.join   (threading)
    | render.draw | render.compute          (rendering)
    | audio.play | input.read               (I/O)
    | panic                                 (divergence)
    | ε₁ ∪ ε₂                               (effect union)
    | ε₁ ; ε₂                               (effect sequencing)
    | ¬ε                                    (forbidden effect)
    | effects(f)                            (effects of function f)
    | name                                  (named effect reference)
    | _?                                    (effect hole)
```

### 3.8 AST Representation

**Well-Formedness:**

Not all syntactically valid ASTs are well-formed. Well-formedness constraints include:

- **Variables in scope**: All variable references must be bound
- **Type well-formedness**: All type expressions must be valid
- **Pattern exhaustiveness**: Match expressions must handle all cases
- **State validity**: Modal state transitions must be valid


---

**Definition 4.1 (Attributes):** An attribute is a metadata annotation that modifies the compilation behavior, analysis, or code generation for the item it decorates, evaluated at compile-time without affecting runtime semantics unless explicitly specified.

## 4. Attributes and Annotations

### 4.1 Overview

**Key innovation/purpose:** Attributes provide declarative compiler directives that modify how code is compiled, verified, or represented without changing the code's fundamental logic.

**When to use attributes:**

- Controlling memory layout for FFI interoperability (`[[repr(C)]]`)
- Selecting verification modes (`[[verify(static)]]`, `[[verify(runtime)]]`)
- Configuring runtime behavior (`[[overflow_checks(true)]]`)
- Providing metadata for tools and documentation (`[[module(...)]]`, `[[alias(...)]]`)
- Optimizing compilation (`[[inline]]`, `[[no_mangle]]`)

**Relationship to other features:**

- **Contracts**: Attributes can select verification mode for contract checking
- **Effects**: Some attributes affect effect checking
- **Memory Layout**: `[[repr(...)]]` controls record/enum layout
- **FFI**: Attributes critical for C interoperability

**Design principle:** Attributes are **declarative compiler hints**, not executable code. They configure the compiler's behavior but don't execute at runtime (unless they change runtime behavior like `[[overflow_checks]]`).

### 4.2 Syntax

The formal productions for `Attribute` and `AttributeBody` appear in Appendix A.1.

**Syntax examples:**

```cursive
// Single attribute
[[repr(C)]]
record Point { x: f32, y: f32 }

// Multiple attributes (stacked)
[[verify(static)]]
[[inline]]
function critical_math(x: i32) { ... }

// Multiple attributes (grouped)
[[repr(C), module(name = "ffi")]]
record CPoint { x: f32, y: f32 }

// Attribute with multiple arguments
[[alias("to_string", "stringify", "as_str")]]
function to_str(x: Debug): string { ... }
```

**Placement rules:**

Attributes appear immediately before the item they modify:

- Module declarations
- Record, enum, modal, trait declarations
- Function and procedure declarations
- Function/procedure parameters
- Record/enum fields
- Type aliases
- Effect declarations

### 4.3 Core Attributes

| Attribute | Applies To | Effect | Notes |
|-----------|------------|--------|-------|
| `[[repr(C|transparent|packed|align(n))]]` | Records, enums | Selects layout strategy | Packed removes padding; align(n) raises minimum alignment. |
| `[[verify(static|runtime|none)]]` | Functions, procedures, record types | Chooses when `uses/must/will` clauses run | `static` proves clauses, `runtime` injects checks, `none` documents intent. |
| `[[overflow_checks(true|false)]]` | Functions, procedures, blocks | Forces integer overflow behaviour | Overrides build-mode defaults. |
| `[[module(key=..., ...)]]` | Module declarations | Records metadata | Keys are implementation-defined; no semantic impact. |
| `[[alias(name₁, …, nameₙ)]]` | Top-level declarations | Supplies tooling aliases | Does not change linkage or lookup. |
| `[[inline]]`, `[[no_inline]]` | Functions, procedures | Inlining hints | Conflicting hints are rejected. |
| `[[no_mangle]]` | External-facing declarations | Preserves symbol names | Typically used for FFI. |
| `[[deprecated(message)]]` | Any declaration | Emits diagnostic on use | Message SHOULD describe the replacement. |
| `[[must_use]]` | Functions, return types | Warns if result ignored | Common on resource-returning APIs. |
| `[[cold]]`, `[[hot]]` | Functions, procedures | Optimisation temperature hints | Advisory only. |
### 4.4 Attribute Semantics

- Attributes precede the declarations they modify.
- Placement is validated statically; using an attribute on an unsupported declaration is an error.
- Conflicting annotations (e.g. `[[inline]]` with `[[no_inline]]`, or two incompatible `[[repr(..)]]` variants) are rejected.
- Each declaration is annotated independently; attributes do not inherit to nested members.
### 4.5 Attribute Effects

`[[verify(runtime)]]`, layout attributes, overflow hints, and inlining hints can change generated code. All other attributes are compile-time metadata with no direct runtime impact.
### 4.6 User-Defined Attributes

**Status:** User-defined attributes are RESERVED for future versions.

Currently, only core attributes defined in this specification are recognized. User-defined attributes will cause a compilation error:

```cursive
[[my_custom_attr]]    // ERROR: unknown attribute 'my_custom_attr'
function foo() { ... }
```

**Future design:** User-defined attributes may be added in conjunction with procedural macros or compile-time reflection.

---

**Previous**: (Start) | **Next**: [Type System](02_Type-System.md)
## 7. Glossary

| Term | Definition |
|------|------------|
| Callable | **Generic term** for any function or procedure (collectively, callables are invocable declarations with parameters and return types). Use this term when referring to functions and procedures without distinction. See Part IX. |
| Contract | Abstract behavioral specification with procedure signatures and clauses (`uses`, `must`, `will`). See Part VII §7.1. |
| Contract clause | One of `uses`, `must`, `will` attached to a callable signature. |
| Effect set | Finite set of capability tokens (e.g., `fs.write`, `alloc.heap`). |
| Function | A callable declared without a `self` parameter. Functions may be declared at module scope or as associated functions within a type's scope (invoked via `::`). Functions cannot directly access instance fields. Declared with the `function` keyword. See Part IX §9.2. |
| Mixin | **Informal term** for trait-based code composition pattern. In Cursive, use the `trait` keyword for reusable implementations. See "Trait" below. |
| Modal state | A value of a modal type annotated with `@State`. |
| Permission wrapper | One of `own T`, `mut T`, or `imm T`, indicating ownership semantics. |
| Pipeline stage | An expression of the form `expr => stage: Type` chaining transformations. |
| Procedure | A callable with an explicit `self` parameter that operates on a receiver value. Procedures are invoked using the scope operator `::` (e.g., `value::method(args)`). Procedures can access instance fields through `self`. Declared with the `procedure` keyword. See Part IX §9.3. |
| Region | Lexically-scoped allocation arena released in LIFO order. |
| Trait | Reusable procedure implementations (enables mixin-style composition). All procedures in a trait MUST have bodies. Distinct from contracts (which have no bodies). See Part II §2.7. |

## Appendix A: Grammar Reference

The canonical grammar for Cursive is maintained in `Spec/A1_Grammar.md`. Chapters reference that appendix for lexical, type, expression, and statement productions instead of re-stating them inline. Implementers SHOULD consult:

- **A.1 Lexical Grammar** — token-level productions shared by all parts.
- **A.2 Type Grammar** — all type constructors, permission wrappers, pointers, and unions.
- **A.3 Pattern Grammar** — patterns used in match expressions and destructuring.
- **A.4 Expression Grammar** — precedence hierarchy and expression forms.
- **A.5 Statement Grammar** — block, declaration, assignment, and control-flow statements.

Helper productions (e.g., `AssignOp`, `RangeOp`) and lexical tokens follow the same appendix; Foundations, the Type System, Expressions, and Statements now reference those sections directly.

## 8. Diagnostics Directory

| ID | Summary | Primary Definition |
|----|---------|--------------------|
| E3D10 | Assignment to immutable binding | §3.2.8 (Declarations) |
| E4001 | Type mismatch | §4.5 (Expressions) |
| E4002 | Arity mismatch | §4.3 (Expressions) |
| E4003 | Permission violation | §4.3/§5.4 |
| E4004 | Effect unavailable | §4.3/§4.19 |
| E4005 | Non-exhaustive match | §4.8 |
| E4006 | Use after move | §4.4.5 |
| E4007 | Index out of bounds | §4.3.5 |
| E4008 | Division by zero | §4.5.1 |
| E4009 | Integer overflow (debug) | §4.5.1 |
| E4010 | Uninitialised variable | §3.2.7 (Declarations) |
| E4011 | Break outside loop | §4.9.5 / §5.8 |
| E4012 | Continue outside loop | §4.9.6 / §5.8 |
| E4013 | Return outside function | §4.14 / §5.7 |
| E4014 | Region value escape | §2.0.8.4 / §5.8 |
| E4015 | Closure captures escaping region | §2.10 / §5.8 |
| E4016 | Contract precondition violation | §4.16 |
| E4017 | Contract postcondition violation | §4.16 |
| E4401 | Missing `result` keyword in block expression | §4.2.4 |
| E4402 | Missing type annotation on match binding | §4.8.1 |
| E4403 | Missing type annotation on pipeline stage | §4.5.9 |
| E4404 | Missing type annotation on loop iterator | §4.9.3 |
| E5001 | Duplicate label | §5.18 |
| E5002 | Undefined label | §5.18 |
| E5003 | Continue targeting non-loop label | §5.18 |
| E5005 | Non-exhaustive modal match | §5.18 |
| E5006 | Loop postcondition violation | §5.18 |
| E5007 | Block precondition violation | §5.18 |
| E5008 | Block effect exceeds function effects | §5.18 |
| E5009 | Panic inside [[no_panic]] section | §5.18 |
| E7C01 | Undeclared effect used in body | Part VII §7.3 |
| E7C02 | Precondition violated at runtime | Part VII §7.4 |
| E7C03 | Postcondition violated at runtime | Part VII §7.5 |
| E7C04 | Type invariant violated at construction | Part VII §7.6 |
| E7C05 | Type invariant violated after mutation | Part VII §7.6 |
| E7C06 | Body provided inside contract signature | Part VII §7.2 |
| E7C07 | Implementation strengthens contract precondition | Part VII §7.2 |
| E7C08 | Implementation weakens contract postcondition | Part VII §7.2 |
| E7C09 | Implementation effect set exceeds contract | Part VII §7.2 |
| E7C10 | Incompatible contract requirements | Part VII §7.2 |
| E7C11 | Missing implementation for contract member | Part VII §7.2 |
| E7C12 | Orphan contract implementation | Part VII §7.2 |
| E7C13 | Duplicate contract implementation | Part VII §7.2 |
| E7C14 | Recursive associated type definition | Part VII §7.2 |
| E7C15 | Contract extends itself (inheritance cycle) | Part VII §7.2 |
| E7C16 | Illegal `@old` capture | Part VII §7.5 |
| E7C17 | Effect inference ambiguity | Part VII §7.3 |
| E7C18 | Duplicate contract clause | Part VII §7.2 |
| E4411 | Unresolved expression hole | Part VIII §8.5 |
| E4412 | Unresolved/ambiguous type hole (or ambiguous kind) | Part VIII §8.6 |
| E4413 | Unresolved/ambiguous permission hole | Part VIII §8.7 |
| E4414 | Unresolved/ambiguous modal‑state hole | Part VIII §8.8 |
| E4415 | Hole in explicitness‑required position | Part VIII §8.10 |
