# PART II: BASICS AND PROGRAM MODEL - Writing Plan

**Version**: 1.0
**Date**: 2025-11-03
**Status**: Approved and Validated
**Validation**: Checked against `Proposed_Organization.md` and `Content_Location_Mapping.md`

---

## Mission

Write a complete, correct, ISO/ECMA-compliant formal specification for PART II, transforming existing content and creating new sections where needed. Length determined by completeness, not targets.

---

## Core Writing Principles

1. **TRANSFORM, not copy-paste** - Restructure and formalize existing content
2. **ONE canonical example per concept** - Definitive, comprehensive demonstration
3. **Complete normative precision** - Every requirement specified with shall/should/may
4. **Implementer-ready** - Sufficient detail to build a conforming implementation
5. **Publication quality** - ISO/ECMA standard formatting and rigor

---

## File Structure

All files in `Spec/02_Basics-and-Program-Model/` directory:

| File | Type | Subsections | Source |
|------|------|-------------|--------|
| `02-0_Translation-Phases.md` | ⭐ NEW | 2.0.1-2.0.6 (6) | `11_Metaprogramming.md` §11.11.1 |
| `02-1_Source-Encoding.md` | 🔄 TRANSFORM | 2.1.1-2.1.4 (4) | `01_Foundations.md` lines 318-337 |
| `02-2_Lexical-Elements.md` | 🔄 TRANSFORM | 2.2.1-2.2.9 (9) | `01_Foundations.md` lines 340-518 |
| `02-3_Syntax-Notation.md` | 🔄 TRANSFORM | 2.3.1-2.3.3 (3) | `01_Foundations.md` lines 65-81 + `A1_Grammar.md` |
| `02-4_Program-Structure.md` | 🔄 EXPAND | 2.4.1-2.4.4 (4) | `03_Declarations-and-Scope.md` + `09_Functions.md` §9.2.8 |
| `02-5_Scopes-Name-Lookup.md` | 🔄 TRANSFORM | 2.5.1-2.5.8 (8) | `03_Declarations-and-Scope.md` lines 723-858, 1377-1491 |
| `02-6_Linkage-Program-Units.md` | ⭐ NEW | 2.6.1-2.6.5 (5) | Design decisions + FFI references |
| `02-7_Modules-Overview.md` | ⭐ NEW | 2.7.1-2.7.5 (5) | Archive file-based design |
| `02-8_Memory-Objects-Overview.md` | 🔄 SYNTHESIZE | 2.8.1-2.8.6 (6) | Multiple files (scattered) |
| `02-9_Lexical-Namespaces.md` | 🔄 EXPAND | 2.9.1-2.9.6 (6) | `03_Declarations-and-Scope.md` lines 859-902 |

**Total**: 10 sections, 50 subsections

---

## Section 2.0: Translation Phases ⭐ NEW

### Source
- `11_Metaprogramming.md` §11.11.1 (compilation phases list)
- Design principle: No preprocessing phase

### Strategy
Write complete formal specification from scratch using source as reference.

### Subsections

#### 2.0.1 Phase Overview
- Introduction to 5-phase compilation model
- Key distinction: No preprocessing phase (unlike C/C++)
- Phase dependencies and ordering constraints
- Table showing phase progression

#### 2.0.2 Source File Processing
- File discovery and loading rules
- UTF-8 validation requirements (reference §2.1)
- BOM handling procedure
- Line ending normalization
- Source file well-formedness

#### 2.0.3 Lexical Analysis
- Tokenization process specification
- Maximal munch rule formalization
- Comment handling (strip non-doc, preserve doc)
- Token stream generation
- Lexical error detection

#### 2.0.4 Syntactic Analysis
- Parsing to AST specification
- Grammar conformance (reference Annex A)
- Error recovery (implementation-defined)
- AST well-formedness requirements
- Parse error reporting

#### 2.0.5 Semantic Analysis
Five sub-phases (formal specification):

1. **Phase 1 - Declaration Collection:**
   - Collect all declarations including comptime blocks
   - Build symbol tables
   - No type checking yet

2. **Phase 2 - Comptime Evaluation:**
   - Execute comptime blocks in dependency order
   - Resolve compile-time expressions
   - Detect circular dependencies

3. **Phase 3 - Code Generation:**
   - Emit generated declarations to AST
   - Integrate with original AST
   - Validate generated code syntax

4. **Phase 4 - Type Checking:**
   - Type check original + generated code
   - All declarations now visible (enables forward references)
   - Effect checking
   - Contract checking

5. **Phase 5 - Lowering:**
   - Lower to intermediate representation (IR)
   - Eliminate comptime constructs
   - Prepare for code generation

**Key Normative Statement:**
> "Cursive has no textual preprocessing phase. All metaprogramming operates on the Abstract Syntax Tree during semantic analysis."

#### 2.0.6 Code Generation
- IR lowering specification
- Optimization passes (implementation-defined)
- Target code emission
- Binary generation

### Formal Elements Required
- Definition 2.0: Compilation phase
- Definition 2.1: Translation unit
- Algorithm: Two-phase compilation procedure
- Inference rules for phase dependencies

### Canonical Example
Single program showing transformation through all 5 phases:
- Source text with comptime block
- Token stream after lexical analysis
- AST after parsing
- AST after comptime execution
- Type-checked AST
- Final IR

---

## Section 2.1: Source Text Encoding 🔄 TRANSFORM

### Source
- `01_Foundations.md` lines 318-337 (current: ~20 lines)
- Target: Complete formal specification

### Strategy
Transform from brief normative statements to comprehensive formal specification with complete rules, algorithms, and examples.

### Subsections

#### 2.1.1 Character Encoding (UTF-8)
- **Formal Definition 2.2:** Character encoding
- **Requirement:** UTF-8 encoding (shall)
- **Conformance rule:** "A conforming implementation shall accept only UTF-8 encoded source files."
- **Invalid UTF-8 handling:** "Byte sequences that do not constitute valid UTF-8 shall result in a diagnostic."
- Error code: E2001

#### 2.1.2 Source File Structure
- **Formal Definition 2.3:** Source file
- File extension: `.cursive` (recommended)
- Maximum file size: implementation-defined (recommended ≤ 1 MiB)
- Unicode Normalization Form: NFC recommended (not required)
- File as sequence of Unicode scalar values

#### 2.1.3 Line Endings
- Accepted line ending sequences: LF (`\n`), CRLF (`\r\n`), CR (`\r`)
- **Normalization algorithm (formal):**
  ```
  normalize_line_endings(input):
      replace all CRLF sequences with LF
      replace all CR sequences with LF
      return normalized text
  ```
- Newline significance: Newlines are significant tokens (reference §2.2.8)
- Mixed line endings: permitted (normalized during lexing)

#### 2.1.4 Byte Order Mark Handling
- UTF-8 BOM (U+FEFF): optional
- If present at file start: ignored (not part of source text)
- BOM in non-initial position: ERROR E2002 (invalid character)
- **Normative rule:** "If present, the BOM shall be ignored and shall not contribute to the source text."

### Character Restrictions
- **NUL byte (U+0000):** Disallowed
  - "Source files shall not contain NUL bytes (U+0000)."
  - Violation: ERROR E2003

- **Control characters:** Restricted outside literals
  - Permitted: horizontal tab (U+0009), line feed (U+000A), carriage return (U+000D), form feed (U+000C)
  - All others disallowed outside string/character literals
  - Violation: ERROR E2004

### Formal Elements Required
- Definition 2.2: Character encoding
- Definition 2.3: Source file
- Definition 2.4: Line ending
- Algorithm: Line ending normalization
- Error codes: E2001-E2004

### Canonical Example
```cursive
// Multi-line source file demonstrating encoding features
// UTF-8 multi-byte characters: © 2025 Cursive™
// Line ending types (this file has mixed LF/CRLF - normalized to LF)

function greet(name: string): string {
    result "Hello, " + name + "!" // Newline significance
}

// After normalization:
// - All line endings -> LF
// - BOM removed if present
// - UTF-8 validated
// - Control chars checked
```

---

## Section 2.2: Lexical Elements 🔄 TRANSFORM

### Source
- `01_Foundations.md` lines 340-518 (current informal coverage)
- Target: Formal specification with complete rules

### Strategy
Transform from informal lists to formal specification with grammar, algorithms, and error handling.

### Subsections

#### 2.2.1 Lexical Analysis Overview
- Role of lexical analyzer
- Input: character stream
- Output: token stream
- Relationship to §2.0.3

#### 2.2.2 Input Elements and Token Classification
- **Token categories:**
  ```
  Token ::= Identifier | Keyword | Literal | Operator | Punctuator
  ```
- Token properties: type, lexeme, location
- Whitespace and comment handling

#### 2.2.3 Whitespace Characters
- **Formal specification:**
  - Horizontal tab (U+0009)
  - Line feed (U+000A)
  - Carriage return (U+000D)
  - Form feed (U+000C)
  - Space (U+0020)
- Whitespace between tokens is insignificant (except newlines)
- Newlines are significant (see §2.2.8)

#### 2.2.4 Comment Syntax and Preservation
**Line comments:** `//` to end of line
```cursive
// This is a line comment
let x = 5 // Inline comment
```

**Block comments:** `/* ... */` with nesting
```cursive
/* Outer comment
   /* Nested comment */
   Still outer
*/
```

**Doc comments:**
- Item documentation: `///`
- Module documentation: `//!`
- **Preservation rule:** Doc comments shall be preserved for documentation generation
- **Stripping rule:** Non-doc comments shall be stripped before parsing

**Comment and statement termination:**
> "Comments do not affect statement continuation rules. Newlines after comments are significant."

#### 2.2.5 Identifier Syntax and Restrictions
**Syntax:**
```
Identifier ::= XID_Start XID_Continue*
```

**Properties:**
- Case-sensitive: `Variable` ≠ `variable`
- Maximum length: implementation-defined (recommended 255 characters)
- Unicode support: XID_Start and XID_Continue per Unicode UAX#31

**Restrictions:**
- Cannot be a reserved keyword
- Cannot start with a digit
- **Normative rule:** "An identifier shall not be a reserved keyword."
- Error code: E2005 (keyword used as identifier)

#### 2.2.6 Reserved and Contextual Keywords
**Reserved keywords (shall not be used as identifiers):**
```
abstract, as, async, await, break, by, case, comptime, continue,
defer, effect, else, enum, exists, false, forall, function, if, import,
internal, invariant, let, loop, match, modal, module, move, must,
mut, new, none, own, private, procedure, protected, public, record, ref,
region, result, select, self, Self, shadow, state, static, predicate, true,
type, uses, var, where, will, with
```

**Contextual keywords:**
- `effects` - contextual in predicate/contract declarations
- `pure` - contextual in function declarations

**Deprecated keywords:**
- `needs`, `requires`, `ensures` - deprecated, use `must`/`will` instead

#### 2.2.7 Literal Syntax and Semantics
**Integer literals:**
- Decimal: `42`, `1_000_000`
- Hexadecimal: `0xFF`, `0x1A2B`
- Octal: `0o755`
- Binary: `0b1010_1111`
- Type suffixes: `42i8`, `100u64`, `0xFFu32`
- Default type: `i32`

**Floating-point literals:**
- Standard: `3.14`, `0.5`
- Exponent: `1.5e10`, `2.0e-5`
- Type suffixes: `3.14f32`, `2.5f64`
- Default type: `f64`

**String literals:**
- Syntax: `"string content"`
- Multi-line support (preserves newlines)
- Escape sequences: `\n`, `\r`, `\t`, `\\`, `\'`, `\"`, `\0`, `\xNN`, `\u{N...}`

**Character literals:**
- Syntax: `'c'`
- Single Unicode scalar value
- Same escape sequences as strings
- Type: `char`

**Boolean literals:**
- `true`, `false`
- Type: `bool`

**Underscore separators:**
- Permitted in numeric literals for readability
- Ignored during parsing
- Example: `1_000_000` = `1000000`

#### 2.2.8 Operators and Punctuators
Reference complete list from Annex A. Major categories:
- Arithmetic: `+`, `-`, `*`, `/`, `%`, `**`
- Comparison: `==`, `!=`, `<`, `>`, `<=`, `>=`
- Logical: `&&`, `||`, `!`
- Bitwise: `&`, `|`, `^`, `<<`, `>>`
- Assignment: `=`
- Punctuation: `(`, `)`, `{`, `}`, `[`, `]`, `,`, `;`, `:`, `.`, `::`, `=>`, `?`

#### 2.2.9 Statement Termination and Token Formation
**Primary rule:** A newline terminates a statement unless a continuation rule applies.

**Continuation Rules:**
1. **Unclosed delimiters:** `(`, `[`, `{`, `<`
   ```cursive
   let result = long_function_name(
       arg1, arg2, arg3  // Continuation due to (
   )
   ```

2. **Trailing operator:** `+`, `-`, `*`, `/`, etc.
   ```cursive
   let sum = a +
       b +  // Continuation due to trailing +
       c
   ```

3. **Leading dot:** `.` for field access
   ```cursive
   let value = object
       .field  // Continuation due to leading .
       .method()
   ```

4. **Leading pipeline:** `=>`
   ```cursive
   let result = value
       => transform  // Continuation due to leading =>
       => finalize
   ```

**Note:** `::` is NOT a chaining operator (does not trigger continuation)

**Token Formation - Maximal Munch:**
- **Rule:** The lexer uses maximal munch: the longest matching token is chosen.
- **Formal specification:**
  ```
  [Lex-Token-Maximal]
  input stream I at position p
  multiple token matches: T₁ (length n₁), T₂ (length n₂)
  n₁ > n₂
  ---------------------------------------------------------
  Select T₁ (maximal munch)
  ```

**Lexical Errors:**
- E2001: Invalid UTF-8 encoding
- E2002: Invalid BOM position
- E2003: NUL byte in source
- E2004: Invalid control character
- E2005: Keyword used as identifier
- E2006: Invalid escape sequence
- E2007: Unterminated string literal
- E2008: Unterminated block comment
- E2009: Integer literal overflow

### Formal Elements Required
- Token classification grammar
- Maximal munch algorithm
- Statement termination decision procedure
- Literal type inference rules
- Complete error code catalog

### Canonical Examples
**One example per major subsection:**

**Comments:**
```cursive
// Line comment
/* Block comment /* nested */ still block */
/// Doc comment for following item
//! Module documentation
```

**Identifiers:**
```cursive
valid_identifier
_starting_with_underscore
with_numbers_123
μετάβλητη  // Unicode identifier (Greek)

// ❌ Invalid:
// 123invalid  // Starts with digit
// let  // Reserved keyword
```

**Literals:**
```cursive
// Integers
let dec = 42
let hex = 0xFF
let bin = 0b1010
let separated = 1_000_000

// Floats
let pi = 3.14159
let sci = 6.022e23

// Strings and chars
let s = "Hello, world!\nNext line"
let c = 'x'

// Boolean
let flag = true
```

**Statement termination:**
```cursive
// ✅ Continuation due to trailing operator
let sum = a +
    b + c

// ✅ Continuation due to unclosed paren
let result = function_call(
    arg1,
    arg2
)

// ❌ No continuation (newline terminates)
let x = 5
let y = 10  // Separate statement
```

---

## Section 2.3: Syntax Notation 🔄 TRANSFORM

### Source
- `01_Foundations.md` lines 65-81 (EBNF table)
- `A1_Grammar.md` (complete grammar reference)

### Strategy
Transform from brief EBNF table to complete metasyntactic specification with grammar organization principles.

### Subsections

#### 2.3.1 Grammar Organization
**Complete grammar authority:**
> "The complete grammar for Cursive is specified in Annex A (normative). In-context grammar snippets throughout this specification are informative illustrations only. For complete production rules, consult Annex A."

**EBNF notation:** Defined in §[REF_TBD]
**Grammar presentation policy:** Defined in §[REF_TBD]

**Dual presentation:**
- **In-context grammar:** Appears within semantic sections (informative, 2-5 productions typical)
- **Consolidated grammar:** Annex A (normative, complete, authoritative)

**Synchronization requirement:**
> "In-context grammar productions shall match the corresponding productions in Annex A. In case of conflict, Annex A is authoritative."

**EBNF Notation Recap:**
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

#### 2.3.2 Syntax Categories
**Major categories:**
- **Lexical syntax:** Tokens, identifiers, literals, keywords (Annex A.1)
- **Type syntax:** Type expressions, type constructors (Annex A.2)
- **Pattern syntax:** Match patterns, destructuring (Annex A.3)
- **Expression syntax:** Value-producing constructs (Annex A.4)
- **Statement syntax:** Control flow, declarations (Annex A.5)
- **Declaration syntax:** Top-level constructs (Annex A.6)

**Specialized syntax:**
- **Contract syntax:** Preconditions, postconditions, invariants (Annex A.7)
- **Assertion syntax:** Predicates, quantifiers (Annex A.8)
- **Effect syntax:** Effect paths, effect sets (Annex A.9)
- **Attribute syntax:** Attribute application (Annex A.10)

#### 2.3.3 Reference to Appendix A (Complete Grammar)
**Annex A organization:**
- **A.1: Lexical Grammar** - Tokens, identifiers, literals, keywords
- **A.2: Type Grammar** - All type constructors and expressions
- **A.3: Pattern Grammar** - Pattern matching forms
- **A.4: Expression Grammar** - Complete expression syntax with precedence
- **A.5: Statement Grammar** - All statement forms
- **A.6: Declaration Grammar** - Top-level declarations
- **A.7: Contract Grammar** - Contract clauses (must/will/where)
- **A.8: Assertion Grammar** - Predicates and quantifiers
- **A.9: Effect Grammar** - Effect syntax
- **A.10: Attribute Grammar** - Attribute syntax
- **A.11: Grammar Reference** - Consolidated EBNF (quick reference)

**Usage guidance:**
> "Implementers should consult Annex A for the authoritative and complete grammar specification."

### Formal Elements Required
- Meta-grammar principles
- Grammar well-formedness rules
- Ambiguity resolution policy
- Precedence specifications

### Canonical Example
**Single production demonstrating all EBNF operators:**
```
Expression ::=
    Literal                              // Terminal
  | Identifier                           // Terminal
  | "(" Expression ")"                   // Grouping
  | Expression BinaryOp Expression       // Sequencing
  | UnaryOp Expression                   // Prefix
  | Expression "." Identifier            // Postfix
  | Expression "[" Expression "]"        // Indexing
  | "if" Expression BlockExpr ("else" BlockExpr)?  // Optional
  | Expression ("," Expression)*         // Zero or more
  | Expression ("," Expression)+         // One or more
  | Expression ("," | ";" | "|")         // Alternatives
```

---

## Section 2.4: Program Structure 🔄 EXPAND

### Source
- `03_Declarations-and-Scope.md` lines 25-791 (partial coverage)
- `09_Functions.md` §9.2.8 (main function complete specification)

### Strategy
Synthesize scattered content into comprehensive program structure specification, adding complete main function specification.

### Subsections

#### 2.4.1 Compilation Units
**Formal Definition 2.4:** A compilation unit is the source code contained in a single source file, processed as a unit by the compiler.

**Properties:**
- One source file = one compilation unit
- File path determines module identity (see §[REF_TBD])
- Compilation units linked to form complete program

**Well-formedness:**
- Must be valid UTF-8 (§2.1)
- Must be syntactically valid (§2.0.4)
- Must be semantically valid (§2.0.5)

#### 2.4.2 Top-Level Constructs
**Allowed declarations at module scope:**
- Variable declarations: `let`, `var`
- Function declarations: `function`
- Procedure declarations: `procedure`
- Type declarations: `record`, `enum`, `type` (alias), `newtype`
- Predicate declarations: `predicate`
- Contract declarations: `contract`
- Effect declarations: `effect`
- Modal declarations: `modal`
- Import declarations: `import`, `use`

**Prohibited at module scope:**
- Statements (expression statements, control flow)
- Local-only constructs

**Visibility modifiers:**
> "All top-level declarations may be annotated with visibility modifiers: `public`, `internal`, `private`, `protected`."

**Default visibility:** `internal` (package-local)

#### 2.4.3 Declaration Order
**Forward references:**
> "Forward references to types and functions are permitted through two-phase compilation (see §2.0.5)."

**Self-referential types (allowed):**
```cursive
record Node {
    value: i32
    next: Ptr<Node>  // ✅ OK: forward reference to Node itself
}
```

**Mutually recursive types (allowed):**
```cursive
record Tree {
    data: i32
    forest: Forest  // ✅ Forward reference
}

record Forest {
    trees: Ptr<[Tree]>@Valid  // ✅ Forward reference
}
```

**Declaration collection phase:**
> "During the first phase of semantic analysis, all declarations are collected and made visible before any type checking occurs. This enables forward references."

#### 2.4.4 Program Entry Point
**Main function requirement:**
> "Every executable Cursive program must define an entry point function named `main`. Execution begins at `main` after static initialization."

**Permitted signatures:**

**Form 1: No arguments**
```cursive
function main(): i32 {
    // Program logic
    result 0
}
```

**Form 2: Command-line arguments**
```cursive
function main(args: [string]): i32 {
    // Process args
    result 0
}
```

**Effectful main (procedure):**
```cursive
procedure main()
    uses io.write, io.read, fs, alloc.heap
: i32 {
    println("Hello, world!")
    result 0
}
```

**Effect constraints:**
- `main` may be pure (`function`) or effectful (`procedure`)
- If effectful, implementation provides standard effect handlers
- Effects declared via `uses` clause

**Uniqueness constraints:**
```
[Main-Unique]
program contains multiple definitions of `main`
--------------------------------------------------------
ERROR E9F-0105: Multiple definitions of entry point `main`

[Main-Missing]
executable program contains no definition of `main`
--------------------------------------------------------
ERROR E9F-0106: Entry point `main` not found
```

**Library vs executable:**
> "Libraries (non-executable compilation units) do NOT require a `main` function. Only executable programs require an entry point."

**Initialization order (formal):**
```
Program Execution Sequence:
1. Static initialization:
   - Module-level constants initialized
   - Module-level statics initialized in dependency order

2. Main execution:
   - `main` function is called
   - Command-line arguments passed (if signature accepts them)

3. Deferred cleanup:
   - Deferred actions execute (§[REF_TBD])
   - Static destructors run in reverse order

4. Process exit:
   - Exit code from `main` returned to operating system
```

**Return value semantics:**
- Type: `i32` (exit code)
- Value `0`: conventionally indicates success
- Non-zero: conventionally indicates error
- Range: implementation-defined (typically -128 to 127 or 0 to 255)

### Formal Elements Required
- Definition 2.4: Compilation unit
- Definition 2.5: Entry point
- Algorithm: Program initialization sequence
- Inference rules: Main signature validity

### Canonical Examples

**Complete well-formed source file:**
```cursive
// Module: example
// Demonstrates top-level constructs

// Import declarations
use std::io::println

// Type declarations
record Point {
    x: f64
    y: f64
}

// Function declaration
function distance(p1: Point, p2: Point): f64 {
    let dx = p2.x - p1.x
    let dy = p2.y - p1.y
    result (dx * dx + dy * dy).sqrt()
}

// Entry point
function main(args: [string]): i32 {
    let origin = Point { x: 0.0, y: 0.0 }
    let point = Point { x: 3.0, y: 4.0 }
    let dist = distance(origin, point)
    println("Distance: {}", dist)
    result 0
}
```

**Forward references (mutually recursive):**
```cursive
record Tree {
    data: i32
    children: Forest  // ✅ Forward reference to Forest
}

record Forest {
    trees: Ptr<[Tree]>@Valid  // ✅ Forward reference to Tree
}

// Both types visible during type checking
```

**Main function with effects:**
```cursive
procedure main(args: [string])
    uses io.read, io.write, fs, alloc.heap
: i32 {
    // Effects explicitly declared
    let config_path = args.get(1) match {
        Some(path) => path,
        None => {
            eprintln("Usage: program <config>")
            return 1
        }
    }

    // Perform I/O (permitted by uses clause)
    let config = fs::read_to_string(config_path) match {
        Ok(contents) => contents,
        Err(e) => {
            eprintln("Error reading config: {}", e)
            return 1
        }
    }

    println("Config loaded successfully")
    result 0
}
```

**Initialization order:**
```cursive
// Static initialization first
let GLOBAL_CONFIG: Config = Config::default()

static mut INSTANCE_COUNT: i32 = 0

function create_instance(): Instance {
    unsafe {
        INSTANCE_COUNT += 1  // Modifies static
    }
    Instance::new()
}

function main(): i32 {
    // Statics initialized before main() runs
    println("Config: {:?}", GLOBAL_CONFIG)

    let inst1 = create_instance()  // Count: 1
    let inst2 = create_instance()  // Count: 2

    // Cleanup happens after main returns
    result 0
}
// inst2 dropped (LIFO)
// inst1 dropped
// Static destructors run
```

---

## Section 2.5: Scopes and Name Lookup 🔄 TRANSFORM

### Source
- `03_Declarations-and-Scope.md` lines 723-858 (§3.7 Scope Rules)
- `03_Declarations-and-Scope.md` lines 1377-1491 (§3.11 Name Resolution)

### Strategy
Transform good existing content with enhanced formal rigor - add set-theoretic definitions, complete algorithms, inference rules.

### Subsections

#### 2.5.1 Scope Concept
**Formal Definition 2.5:** A scope is a lexical region of program text in which a declared name is visible and may be referenced.

**Properties:**
- Scopes are determined statically at compile time
- Each scope has an associated namespace (set of name bindings)
- Scopes form a hierarchical nesting structure

**Scope as mapping:**
```
Scope = Name -> Binding
where Binding = (Entity, Type, Visibility, Location)
```

#### 2.5.2 Lexical Scoping
**Lexical scoping principle:**
> "Cursive uses lexical scoping. The scope of a name is determined by the program's syntactic structure, not by runtime execution."

**Properties:**
- Nested scopes can access outer scope names
- Outer scopes cannot access inner scope names
- Scope boundaries are explicit (braces `{}`)

#### 2.5.3 Scope Nesting
**Scope hierarchy (innermost to outermost):**

**Block Scope:**
```cursive
{
    let x = 5  // x visible from here to end of block
    {
        let y = 10  // y visible in inner block only
        // Both x and y visible here
    }
    // Only x visible here
}
```

**Function Scope:**
- Parameters visible in function's outermost scope
- Labels visible anywhere in function
```cursive
function example(param: i32) {  // param: function scope
    'outer: loop {  // Label 'outer: function-wide visibility
        let local = param * 2  // local: block scope
        break 'outer  // Can reference label from anywhere in function
    }
}
```

**Module Scope:**
- Top-level declarations
- Two-phase compilation enables forward references
- Visibility modifiers control external access

**Formal nesting relation:**
```
[Scope-Nested]
S_inner ⊂ S_outer
name lookup in S_inner
name not found in S_inner
name ∈ S_outer
----------------------------------------
name resolution continues to S_outer
```

#### 2.5.4 Name Binding
**Binding creation:**
- Declarations create bindings in current scope
- Bindings associate names with entities (variables, types, functions)
- Bindings are immutable (cannot rebind without `shadow`)

**Binding properties:**
- Name (identifier)
- Entity (what it refers to)
- Type
- Visibility
- Location (source position)

#### 2.5.5 Name Lookup Algorithm
**5-step resolution algorithm (formal):**

```
resolve_name(identifier) -> Binding | Error:
    // Step 1: Current block scope
    if identifier ∈ current_block_scope:
        return current_block_scope.lookup(identifier)

    // Step 2: Outward through lexical scopes
    for scope in enclosing_scopes (innermost to outermost):
        if identifier ∈ scope:
            return scope.lookup(identifier)

    // Step 3: Module scope
    if identifier ∈ module_scope:
        return module_scope.lookup(identifier)

    // Step 4: Imported namespaces
    candidates := []
    for import in current_module.imports:
        if identifier ∈ import.exported_names:
            candidates.append(import.lookup(identifier))

    if len(candidates) == 0:
        goto Step 5
    else if len(candidates) == 1:
        return candidates[0]
    else:  // len(candidates) > 1
        ERROR E3D04: Ambiguous identifier '{identifier}'

    // Step 5: Universe scope (predeclared)
    if identifier ∈ universe_scope:
        return universe_scope.lookup(identifier)

    // Not found anywhere
    ERROR E3D05: Undefined identifier '{identifier}'
```

**Complexity:** O(n) where n is lexical nesting depth

**Ambiguity detection:**
```
[Name-Ambiguous]
identifier I
multiple imports provide I: M₁::I, M₂::I
unqualified reference to I
--------------------------------------------
ERROR E3D04: Ambiguous identifier I
```

#### 2.5.6 Qualified Name Lookup
**Syntax:** `module::name` or `Type::associated_item`

**Resolution algorithm:**
```
resolve_qualified(prefix::name) -> Binding | Error:
    // Step 1: Resolve prefix
    prefix_entity := resolve_name(prefix)

    if prefix_entity is not found:
        ERROR E3D06: Unknown module or type '{prefix}'

    if prefix_entity is not (module | type):
        ERROR E3D07: '{prefix}' is not a module or type (cannot qualify)

    // Step 2: Look up name in prefix's exports/associated items
    if prefix_entity is module:
        if name ∈ prefix_entity.exported_names:
            return prefix_entity.lookup(name)
        else if name ∈ prefix_entity.all_names:
            ERROR E3D08: '{name}' exists in '{prefix}' but is not public
        else:
            ERROR E3D09: '{name}' not found in module '{prefix}'

    if prefix_entity is type:
        if name ∈ prefix_entity.associated_items:
            return prefix_entity.lookup(name)
        else:
            ERROR E3D10: '{name}' is not an associated item of '{prefix}'
```

#### 2.5.7 Unqualified Name Lookup
**Definition:** Name lookup without a qualifier (single identifier)

**Process:** Follows 5-step algorithm in §2.5.5

**Shadowing consideration:** Inner scopes shadow outer scopes (see §2.5.8)

#### 2.5.8 Shadowing
**Explicit shadowing requirement:**
> "Shadowing must be explicit using the `shadow` keyword. Redeclaration without `shadow` is an error."

**Valid shadowing:**
```cursive
let x = 5
{
    shadow let x = 10  // ✅ OK: explicit shadowing
    // x = 10 here
}
// x = 5 here
```

**Invalid redeclaration:**
```cursive
let y = 5
let y = 10  // ❌ ERROR E3D01: Redeclaration without shadow
```

**Formal semantics:**
```
[Shadow-Valid]
Γ; S_outer ⊢ x : T₁
shadow let x : T₂ = e in S_inner
S_inner ⊂ S_outer
--------------------------------------
Γ; S_inner ⊢ x : T₂   (shadows outer)

[Shadow-Required]
Γ; S ⊢ x : T₁
let x : T₂ = e in S  (without shadow keyword)
-----------------------------------------------
ERROR E3D01: Redeclaration of '{x}' without shadow

[Scope-No-Redeclare]
let x = e1 in scope S
let x = e2 in scope S
--------------------------------------
ERROR E3D01: Redeclaration of '{x}' in same scope
```

**Shadowing across namespace categories:**
> "Shadowing applies within the unified namespace. A shadowed name may be a type in the outer scope and a value in the inner scope, or vice versa."

### Formal Elements Required
- Definition 2.5: Scope
- Definition 2.6: Binding
- Algorithm: Name resolution (5-step)
- Algorithm: Qualified name resolution
- Inference rules: [Shadow-Valid], [Shadow-Required], [Scope-No-Redeclare]
- Error codes: E3D01, E3D04-E3D10

### Canonical Examples

**Scope nesting hierarchy:**
```cursive
// Module scope
let MODULE_CONST: i32 = 100

function outer(param: i32) {  // Function scope: param
    let func_local = param * 2  // Block scope: func_local

    {
        let inner_local = 42  // Inner block scope
        // Visible: MODULE_CONST, param, func_local, inner_local
    }

    // Visible: MODULE_CONST, param, func_local
    // NOT visible: inner_local
}

// Visible: MODULE_CONST
// NOT visible: param, func_local, inner_local
```

**Name lookup demonstrating all 5 steps:**
```cursive
// Step 5: Universe scope (predeclared)
let builtin = i32::MAX  // i32 is predeclared

// Step 3: Module scope
let MODULE_VAR = 10

use io::file::File  // Step 4: Imports

function example() {
    // Step 2: Enclosing function scope
    let func_var = 20

    {
        // Step 1: Current block scope
        let block_var = 30

        // Resolution order:
        println(block_var)    // Step 1: Current block
        println(func_var)     // Step 2: Enclosing scope
        println(MODULE_VAR)   // Step 3: Module scope
        println(File)         // Step 4: Imported
        println(i32::MAX)     // Step 5: Universe (predeclared)
    }
}
```

**Qualified name resolution:**
```cursive
// Module structure:
// src/math.cursive
pub function abs(x: i32): i32 { ... }

// src/main.cursive
import math

function main(): i32 {
    let value = -5
    let positive = math::abs(value)  // Qualified lookup
    result 0
}
```

**Shadowing (valid and invalid):**
```cursive
function shadowing_example() {
    let x = 5  // Outer binding
    println("Outer x: {}", x)  // Prints 5

    {
        shadow let x = 10  // ✅ OK: Explicit shadow
        println("Inner x: {}", x)  // Prints 10

        {
            shadow let x = "hello"  // ✅ OK: Shadow again (different type!)
            println("Innermost x: {}", x)  // Prints "hello"
        }

        println("Back to inner x: {}", x)  // Prints 10
    }

    println("Back to outer x: {}", x)  // Prints 5
}

function redeclaration_error() {
    let y = 5
    let y = 10  // ❌ ERROR E3D01: Redeclaration without shadow
}

function cross_category_shadow() {
    record Point { x: i32, y: i32 }  // Point is a type

    {
        shadow let Point = Point { x: 0, y: 0 }  // ✅ OK: Shadow type with value
        // Point now refers to value, not type
    }

    // Point refers to type again
}
```

---

## Section 2.6: Linkage and Program Units ⭐ NEW

### Source
- Design decisions (visibility determines linkage + opt-in attributes)
- FFI references from `09_Functions.md`

### Strategy
Write complete formal linkage specification from scratch.

### Subsections

#### 2.6.1 Linkage Concept
**Formal Definition 2.6:** Linkage determines whether a symbol (name of an entity) is visible across compilation unit boundaries.

**Distinction:**
- **Linkage:** Symbol visibility in the binary/linking phase
- **Visibility:** Access control at compile time (name resolution)

**Relationship:**
> "Linkage affects the symbol table of the compiled binary. Visibility affects compile-time name resolution. An entity may be visible (compile-time) but have no linkage (not in binary)."

**Linkage categories:**
- External linkage: visible across compilation units
- Internal linkage: visible within compilation unit only
- No linkage: not in symbol table

#### 2.6.2 External Linkage
**Default rule:**
> "`public` items have external linkage by default."

**Properties:**
- Symbol exported in binary
- Callable/accessible from other compilation units
- Name mangling applies (implementation-defined) unless overridden
- Subject to One Definition Rule (§2.6.5)

**Example:**
```cursive
// file1.cursive
public function add(x: i32, y: i32): i32 {
    result x + y
}
// Symbol 'add' has external linkage

// file2.cursive
import file1

function main(): i32 {
    let sum = file1::add(5, 3)  // Links to external symbol
    result 0
}
```

#### 2.6.3 Internal Linkage
**Default rule:**
> "`internal` items have internal (module or package) linkage."

**Properties:**
- Symbol not visible outside compilation unit
- May appear in binary as local symbol
- Enables optimizations (inlining, dead code elimination)
- Not subject to ODR across compilation units

**Example:**
```cursive
// file1.cursive
internal function helper(x: i32): i32 {
    result x * 2
}
// Symbol 'helper' has internal linkage

// file2.cursive - CANNOT access file1::helper
```

#### 2.6.4 No Linkage
**Default rule:**
> "`private` items and local variables have no linkage."

**Properties:**
- No symbol in binary
- Compiler may optimize away entirely
- Storage is automatic for local variables

**Example:**
```cursive
function example() {
    let local_var = 42  // No linkage
    // Compiler may eliminate if unused
}

private function utility() { ... }  // No linkage
```

#### 2.6.5 One Definition Rule (ODR)
**For types:**
> "A type shall have exactly one definition across the entire program."

```
[ODR-Type-Unique]
program P contains type definitions D₁, D₂ of type T
D₁ ≠ D₂ (structurally distinct)
--------------------------------------------------------
ERROR (link-time): Multiple definitions of type T
```

**For functions:**
> "A function with external linkage shall have exactly one definition. Multiple declarations are permitted."

```
[ODR-Function-Single-Def]
program P contains function definitions D₁, D₂ of function f
D₁ and D₂ both have external linkage
--------------------------------------------------------
ERROR (link-time): Multiple definitions of function f
```

**Inline semantics:**
- Functions marked `[[inline]]` may be duplicated across compilation units
- Implementation performs deduplication (implementation-defined)

**Violation consequences:**
- Link-time error (most cases)
- Undefined behavior if undetected [UB: B.2 #N]

#### 2.6.6 Linkage Control Attributes (Optional)
**Opt-in overrides for special cases:**

**`[[no_mangle]]` - Preserve exact symbol name:**
```cursive
[[no_mangle]]
public function my_c_function(x: i32): i32 {
    result x * 2
}
// Symbol name in binary: "my_c_function" (not mangled)
```

**`[[extern("C")]]` - C calling convention + no mangling:**
```cursive
[[extern("C")]]
public function ffi_function(x: i32, y: i32): i32 {
    result x + y
}
// C-compatible ABI and symbol name
```

**`[[weak]]` - Weak symbol linkage:**
```cursive
[[weak]]
public function default_handler() {
    println("Default handler")
}
// Can be overridden by strong symbol with same name
```

**Attribute interaction with visibility:**
- Attributes override default linkage behavior
- `[[no_mangle]]` requires `public` visibility
- Used primarily for FFI and advanced linking scenarios

### Formal Elements Required
- Definition 2.6: Linkage
- Definition 2.7: External linkage
- Definition 2.8: Internal linkage
- Definition 2.9: One Definition Rule
- Inference rules: [ODR-Type-Unique], [ODR-Function-Single-Def]
- Attribute specifications

### Canonical Example

**Multi-file program showing all linkage types:**

```cursive
// ========== file: math.cursive ==========
// External linkage (public)
public function add(x: i32, y: i32): i32 {
    result internal_add_impl(x, y)
}

// Internal linkage (internal - default for many items)
internal function internal_add_impl(x: i32, y: i32): i32 {
    result x + y
}

// No linkage (private)
private function unused_helper() {
    // This function has no linkage, may be eliminated
}

// No linkage (local variable)
function compute(n: i32): i32 {
    let temp = n * 2  // No linkage
    result temp
}

// External linkage with no mangling (FFI)
[[no_mangle]]
public function c_compatible_add(x: i32, y: i32): i32 {
    result add(x, y)
}
// Binary symbol: "c_compatible_add" (exact name preserved)

// ========== file: main.cursive ==========
import math

function main(): i32 {
    // Can access external linkage items
    let sum = math::add(5, 3)

    // CANNOT access internal linkage items
    // let result = math::internal_add_impl(5, 3)  // ❌ ERROR: not visible

    // CANNOT access no-linkage items
    // let x = math::unused_helper()  // ❌ ERROR: private

    result 0
}

// Linking:
// - math::add exported with external linkage
// - math::internal_add_impl kept internal (may be inlined)
// - math::unused_helper may be eliminated
// - c_compatible_add exported with exact name "c_compatible_add"
```

---

## Section 2.7: Modules (Overview) ⭐ NEW

### Source
- Archive file-based module design
- Design decision: Pure file-based (no module keyword)

### Strategy
Write complete formal module system specification based on file-based design.

### Subsections

#### 2.7.1 Module Concept
**Formal Definition 2.10:** A module is a namespace unit defined by a source file. The module's path is determined by the file's location relative to the source root.

**Key principle:**
> "Cursive uses a file-based module system. There is no `module` keyword. Module structure follows the filesystem structure."

**Module as tuple:**
```
Module = (Path, Exports, Declarations)

where:
  Path = sequence of identifiers (e.g., "math.geometry")
  Exports = subset of Declarations with public visibility
  Declarations = set of items defined in the module
```

**Properties:**
- One file = one module
- File path determines module path
- Module path determines import path

#### 2.7.2 Module Declaration
**Not applicable:**
> "Module declarations are implicit. The filesystem structure determines the module hierarchy. No explicit `module` statements are required in source files."

#### 2.7.3 Import and Export
**Import syntax:**

**Import entire module:**
```cursive
import math.geometry
// Access items: math.geometry::area_of_circle
```

**Use specific items:**
```cursive
use math.geometry::area_of_circle
use math.geometry::{area, circumference}
// Direct access: area_of_circle, area, circumference
```

**Use with alias:**
```cursive
use math.geometry::area_of_circle as circle_area
// Access: circle_area
```

**Use wildcard:**
```cursive
use math.geometry::*
// Import all public items from math.geometry
```

**Export control:**
> "Export control is determined by visibility modifiers. Only `public` items are exported from a module."

**Formal semantics:**
```
[Import-Module]
import M
module M exists at path P
----------------------------------------
M is available for qualified access: M::item

[Use-Item]
use M::item
module M exports item
----------------------------------------
item is imported into current scope (unqualified access)

[Use-Wildcard]
use M::*
module M exports items I = {i₁, i₂, ..., iₙ}
no naming conflicts
----------------------------------------
all items in I imported into current scope

[Import-Ambiguous]
use M₁::item, use M₂::item
unqualified reference to item
----------------------------------------
ERROR E3D04: Ambiguous identifier 'item'
```

#### 2.7.4 Module Visibility
**Visibility levels and exports:**
- **`public`:** Exported from module, accessible everywhere
- **`internal`:** Package-local (default), accessible within same package
- **`private`:** Module-local only, not exported

**Module boundary:**
> "The module boundary is the source file boundary. Items in different modules cannot access each other's private items."

#### 2.7.5 Module Resolution Algorithm (Optional - can be in 2.7.3)
**Module path to file path mapping:**

```
resolve_module_path(module_path: "math.geometry") -> FilePath:
    1. Split module path by '.' delimiter
       -> ["math", "geometry"]

    2. Join components with OS path separator
       Unix:    "math/geometry"
       Windows: "math\geometry"

    3. Prepend source root ("src/" by convention)
       -> "src/math/geometry"

    4. Append file extension (".cursive")
       -> "src/math/geometry.cursive"

    5. Check file exists
       If not found: ERROR E2M01: Module not found

    6. Return absolute file path
       -> "/absolute/path/to/project/src/math/geometry.cursive"
```

**Error handling:**
```
[Module-Not-Found]
import statement references module M
resolve_module_path(M) returns no file
----------------------------------------
ERROR E2M01: Module not found: M
```

**Cross-reference:**
> "For detailed module system specification, including package organization and circular dependency resolution, see §[REF_TBD]."

### Formal Elements Required
- Definition 2.10: Module
- Definition 2.11: Module path
- Algorithm: Module resolution
- Inference rules: [Import-Module], [Use-Item], [Use-Wildcard], [Import-Ambiguous]
- Error codes: E2M01 (module not found)

### Canonical Example

**Project structure demonstrating file-based modules:**

```
myproject/
├── Cursive.toml              # Project manifest
└── src/
    ├── main.cursive           # Module: main (entry point)
    ├── utils.cursive          # Module: utils
    ├── math/
    │   ├── geometry.cursive   # Module: math.geometry
    │   └── algebra.cursive    # Module: math.algebra
    └── data/
        └── structures.cursive # Module: data.structures
```

**Source code demonstrating imports:**

```cursive
// ========== src/math/geometry.cursive ==========
// Module: math.geometry

public function area_of_circle(radius: f64): f64 {
    result 3.14159 * radius * radius
}

public function circumference(radius: f64): f64 {
    result 2.0 * 3.14159 * radius
}

internal function helper(x: f64): f64 {
    result x * 2.0  // Not exported (internal)
}

// ========== src/math/algebra.cursive ==========
// Module: math.algebra

public function solve_linear(a: f64, b: f64): f64 {
    result -b / a
}

// ========== src/utils.cursive ==========
// Module: utils

use math.geometry::area_of_circle  // Specific import

public function print_circle_area(radius: f64) {
    let area = area_of_circle(radius)
    println("Circle area: {}", area)
}

// ========== src/main.cursive ==========
// Module: main (entry point)

// Import entire module
import math.geometry
import math.algebra

// Use specific items
use utils::print_circle_area

function main(): i32 {
    // Qualified access
    let area = math.geometry::area_of_circle(5.0)
    let circ = math.geometry::circumference(5.0)

    println("Area: {}, Circumference: {}", area, circ)

    // Unqualified access (via use)
    print_circle_area(10.0)

    // Qualified access to algebra module
    let solution = math.algebra::solve_linear(2.0, -4.0)
    println("Solution: {}", solution)

    result 0
}

// Module resolution examples:
// import math.geometry -> src/math/geometry.cursive
// import math.algebra -> src/math/algebra.cursive
// import utils -> src/utils.cursive
```

---

## Section 2.8: Memory and Objects (Overview) 🔄 SYNTHESIZE

### Source
- `03_Declarations-and-Scope.md` lines 1027-1159 (storage duration)
- `02_Type-System.md` lines 972-1027 (regions)
- Multiple scattered references

### Strategy
Synthesize coherent overview with clear forward references to Part VI. This is an OVERVIEW only - formal definitions in Part VI.

### Subsections

#### 2.8.1 Object Model (Conceptual)
**Concept:**
> "An object is a value with associated storage, a type, a lifetime, and a location in memory."

**Properties (conceptual):**
- **Type:** Determines representation and operations
- **Size:** Amount of storage occupied (implementation-defined for many types)
- **Alignment:** Address alignment requirement (implementation-defined)
- **Lifetime:** Duration from creation to destruction

**Forward reference:**
> "See §[REF_TBD] for the formal object model specification, including object creation, destruction, and lifetime rules."

#### 2.8.2 Memory Locations (Conceptual)
**Concept:**
> "A memory location is a distinct addressable region of storage. Objects occupy one or more memory locations."

**Properties (conceptual):**
- Identity (distinct from other locations)
- Byte sequence
- Alignment
- Accessibility (determined by permissions)

**Forward reference:**
> "See §[REF_TBD] for formal memory location specification and aliasing rules."

#### 2.8.3 Object Lifetime (Overview)
**RAII (Resource Acquisition Is Initialization):**
> "Objects are automatically cleaned up at the end of their scope. Cursive employs RAII for deterministic resource management."

**Cleanup order (LIFO):**
> "Objects are destroyed in reverse order of declaration (Last In, First Out)."

**Example:**
```cursive
{
    let a = Resource::new("A")  // Declared first
    let b = Resource::new("B")  // Declared second
    let c = Resource::new("C")  // Declared third
}
// Cleanup order: c, b, a (reverse)
```

**Determinism:**
> "Cleanup points are statically determined. Destructors run at well-defined points in the program."

**Forward reference:**
> "See §[REF_TBD] for detailed object lifetime rules, including temporary object lifetime and lifetime extension."

#### 2.8.4 Storage Duration (Overview)
**Definition:** Storage duration determines when storage for an object is allocated and deallocated.

**Storage classes:**

| Context | Keyword | Duration | Allocation | Deallocation |
|---------|---------|----------|------------|--------------|
| Module scope | `let`/`var` | Static (program lifetime) | Before main | After main |
| Function scope | `let`/`var` | Automatic (scope lifetime) | On entry | On exit |
| Block scope | `let`/`var` | Automatic (scope lifetime) | At declaration | At end of block |
| Region block | (in region) | Region lifetime | At allocation | At region end |
| Heap | Box, Rc, Arc | Manual/reference-counted | Explicit | Explicit/ref count zero |

**Static storage:**
```cursive
// Module level
let CONSTANT: i32 = 42  // Lives entire program
static mut GLOBAL: i32 = 0  // Lives entire program
```

**Automatic storage:**
```cursive
function example() {
    let local = 42  // Lives until end of function
    {
        let block_local = 10  // Lives until end of block
    }
}
```

**Region storage:**
```cursive
region r {
    let value = alloc_in<r>(data)
    // value allocated in region r
}
// All region allocations freed at once (O(1))
```

**Forward reference:**
> "See §[REF_TBD] for complete storage duration specification, including static initialization order and region escape analysis."

#### 2.8.5 Alignment (Brief Mention)
**Concept:**
> "Alignment is the address requirement for objects. Most types have natural alignment determined by their size and composition."

**Implementation-defined:**
> "Specific alignment values are implementation-defined. Typical alignments: `i8` -> 1 byte, `i32` -> 4 bytes, `i64` -> 8 bytes."

**Forward reference:**
> "See §[REF_TBD] for memory layout and alignment rules, including alignment attributes and padding."

#### 2.8.6 Cross-Reference to Part VI
**Part VI provides comprehensive formal memory model:**

| Topic | Part VI Section |
|-------|-----------------|
| Objects and Memory Locations (detailed) | §[REF_TBD] |
| Permission System (imm/mut/own) | §[REF_TBD] |
| Move Semantics (move checking, Copy types) | §[REF_TBD] |
| Regions and Lifetimes (escape analysis) | §[REF_TBD] |
| Storage Duration (complete specification) | §[REF_TBD] |
| Memory Layout (sizes, alignment, padding) | §[REF_TBD] |
| Aliasing and Uniqueness (formal rules) | §[REF_TBD] |
| Unsafe Operations (safety obligations) | §[REF_TBD] |

**Principle:**
> "Part II provides conceptual foundation for understanding program structure. Part VI provides formal semantics for memory safety and resource management."

### Formal Elements Required
- High-level definitions (conceptual)
- Storage duration table
- Clear forward references
- Minimal formal rules (overview only)

### Canonical Example

**Single program demonstrating all storage classes and RAII:**

```cursive
// ========== Static storage (module level) ==========
let MODULE_CONSTANT: i32 = 100  // Static storage, lives entire program

static mut CALL_COUNT: i32 = 0  // Static storage, mutable

// ========== Program execution ==========
function create_resource(name: string): Resource {
    unsafe {
        CALL_COUNT += 1  // Access static
    }
    Resource::new(name)
}

function demonstrate_storage() {
    // Automatic storage (function scope)
    let func_var = 42  // Lives until end of function

    {
        // Automatic storage (block scope)
        let block_var = Resource::new("block")
        // block_var lives until end of this block
        println("Block scope")
    }
    // block_var destroyed here (RAII)

    // Region storage
    region r {
        let r1 = alloc_in<r>(Resource::new("region1"))
        let r2 = alloc_in<r>(Resource::new("region2"))
        let r3 = alloc_in<r>(Resource::new("region3"))
        // All allocated in region r
    }
    // All region allocations freed at once (O(1) bulk free)

    // Heap storage (explicit)
    let heap_box = Box::new(Resource::new("heap"))
    // heap_box freed when dropped (RAII manages Box)

    println("Function ending")
}
// func_var, heap_box destroyed here in reverse order (LIFO)

function main(): i32 {
    println("Starting (CALL_COUNT: {})", unsafe { CALL_COUNT })

    demonstrate_storage()

    println("Ending (CALL_COUNT: {})", unsafe { CALL_COUNT })

    result 0
}
// Static destructors run after main returns

/* Execution trace (RAII cleanup):
1. Static initialization: MODULE_CONSTANT, CALL_COUNT
2. main() executes
3. demonstrate_storage() executes:
   - func_var created
   - block_var created -> destroyed (end of block)
   - r1, r2, r3 created in region -> all freed (region end)
   - heap_box created -> destroyed (end of function)
   - func_var destroyed (end of function)
4. main() returns
5. Static destructors (if any)
*/
```

---

## Section 2.9: Lexical Namespaces 🔄 EXPAND

### Source
- `03_Declarations-and-Scope.md` lines 859-902 (§3.7.3 Namespace Unification)
- Current: ~20 lines -> Target: Complete formal specification

### Strategy
Expand from brief description to comprehensive formal specification. Document unified namespace model (per-scope), not separate namespace categories.

### Important Note
**Structure deviation from Proposed_Organization.md:**
- Proposed: Separate namespace categories (Type/Value/Module)
- Implemented: Lexical namespaces (unified per scope)
- Justification: Matches actual language semantics and user decision

### Subsections

#### 2.9.1 Lexical Namespace Concept
**Formal Definition 2.11:** Each lexical scope has exactly one namespace, which is the set of all name bindings visible within that scope.

**Key property:**
> "Names in different scopes are independent. The same identifier may be bound to different entities in different scopes."

**Lexical characteristic:**
> "Namespace boundaries correspond to lexical scope boundaries. Namespace structure is determined statically."

**Namespace as set:**
```
Namespace(S) = {(name, binding) | name declared in scope S}
```

#### 2.9.2 Unified Namespace Per Scope
**Core rule (from spec §3.7.3):**
> **Rule 3.7.2 (Single Namespace):** Types and values share a single namespace per scope.

**Implication:**
> "Within a single scope, a name cannot refer to both a type and a value. If a name is declared as a type, it cannot be redeclared as a value in the same scope, and vice versa."

**Allowed (different scopes):**
```cursive
function foo() {
    let x = 5  // x is a value in foo's scope
}

function bar() {
    let x = "hello"  // ✅ OK: Different scope, independent namespace
}
```

**Not allowed (same scope):**
```cursive
record Point { x: i32, y: i32 }  // Point is a type in module scope
let Point = get_point()  // ❌ ERROR E3D03: 'Point' already declared as type
```

**Formal semantics:**
```
[Namespace-Unified]
scope S has namespace N
name x declared as type in N
name x declared as value in N
----------------------------------------
ERROR E3D03: Name conflict in unified namespace

[Namespace-Independent-Scopes]
scope S₁ has namespace N₁
scope S₂ has namespace N₂
S₁ ≠ S₂ (distinct scopes)
name x declared in N₁
name x declared in N₂
----------------------------------------
OK: Names in different scopes are independent
```

#### 2.9.3 Name Categories (Informative)
**Definition:** Names have categories for semantic analysis purposes:
- Type names: records, enums, type aliases, predicates
- Value names: variables, functions, procedures, constants
- Label names: loop and block labels

**Important distinction:**
> "Name categories are used for well-formedness checking (e.g., cannot call a type as a function), but they do not create separate namespaces. Resolution checks the single unified namespace per scope."

**Category matters for:**
- Type checking (cannot use type where value expected)
- Well-formedness (cannot call a variable as a function)
- Error messages (clearer diagnostics)

**Category does not affect:**
- Name resolution (single namespace lookup)
- Shadowing rules (can shadow type with value)

#### 2.9.4 Contrast with Other Languages
**C/C++ Model (separate namespaces):**
```cpp
// C++ - ALLOWED
struct Point { int x, y; };  // Point in type namespace
int Point = 5;               // Point in value namespace
// ✅ OK: Separate type and value namespaces

// Context determines which 'Point':
Point p;         // Type: struct Point
int x = Point;   // Value: variable Point
```

**Rust Model (separate namespaces):**
```rust
// Rust - ALLOWED
struct Point { x: i32, y: i32 }  // Type namespace
let Point = 5;                   // Value namespace
// ✅ OK: Separate namespaces
```

**Cursive Model (unified namespace):**
```cursive
// Cursive - ERROR
record Point { x: i32, y: i32 }  // Type in unified namespace
let Point = 5  // ❌ ERROR E3D03: 'Point' already declared as type
// Conflict: unified namespace per scope
```

**Why unified?**
- Simpler mental model (one lookup per scope)
- Reduced ambiguity for LLMs
- Clearer code (no context-dependent resolution)

#### 2.9.5 Rationale (from spec)
**From §3.7.3:**
> **Rationale:**
> - Simplifies name resolution
> - Reduces ambiguity for LLMs
> - Prevents confusion between type and value contexts

**Detailed rationale:**

**1. Simplifies name resolution:**
- Single namespace -> single lookup per scope
- No need to track context (type position vs value position)
- Simpler compiler implementation

**2. Reduces ambiguity for LLMs:**
- AI code generation benefits from unambiguous names
- No context-dependent meaning
- Easier to reason about code

**3. Prevents confusion:**
- Human readers: clearer which entity a name refers to
- No surprise context switches
- Explicit is better than implicit

#### 2.9.6 Scope-Namespace Relationship
**Definitions:**
- **Scope:** Lexical region where names are visible
- **Namespace:** Set of name bindings within a scope

**Relationship:**
> "Each scope has exactly one namespace. Nested scopes create nested namespaces, but each namespace is independent."

**Nesting:**
```
Module scope -> Module namespace N_module
    ↓
Function scope -> Function namespace N_function
    ↓
Block scope -> Block namespace N_block
```

**Independence:**
> "While inner scopes can access outer scope names (through lookup), each scope maintains its own independent namespace. A name in an inner namespace does not affect the outer namespace."

**Shadowing interaction:**
> "Shadowing (§2.5.8) allows an inner scope to reuse a name from an outer scope, creating a new binding in the inner namespace that temporarily hides the outer binding."

### Formal Elements Required
- Definition 2.11: Namespace
- Definition 2.12: Name category
- Inference rules: [Namespace-Unified], [Namespace-Independent-Scopes]
- Error code: E3D03

### Canonical Example

**Complete program demonstrating unified namespace model:**

```cursive
// ========== Module scope namespace ==========
record Point { x: i32, y: i32 }  // Point (type) in module namespace

// ❌ ERROR: Cannot have both type and value named Point in same scope
// let Point = 5  // ERROR E3D03: 'Point' already declared as type

function calculate_distance(p1: Point, p2: Point): f64 {
    // ========== Function scope namespace ==========
    // 'p1' and 'p2' in function namespace

    let dx = p2.x - p1.x  // 'dx' in function namespace
    let dy = p2.y - p1.y  // 'dy' in function namespace

    {
        // ========== Inner block namespace (independent) ==========
        let dx = 100  // ✅ OK: Different scope, independent namespace
        let dy = 200  // ✅ OK: Different scope, independent namespace
        println("Inner: dx={}, dy={}", dx, dy)  // Prints 100, 200
    }

    // Back to function namespace
    println("Outer: dx={}, dy={}", dx, dy)  // Original values

    result (dx * dx + dy * dy).sqrt()
}

function demonstrate_shadowing() {
    // ========== Function scope namespace ==========
    record Point { a: i32 }  // Point (type) in this function's namespace
    // ✅ OK: Different scope from module-level Point

    {
        // ========== Block scope namespace ==========
        shadow let Point = Point { a: 42 }  // Point (value) shadows type
        // ✅ OK: Explicit shadow in inner scope
        // Point now refers to value, not type
        println("Value: {}", Point.a)
    }

    // Back to function namespace - Point is type again
    let p: Point = Point { a: 10 }
}

function cross_category_examples() {
    // Type in function namespace
    type Distance = f64

    {
        // ✅ OK: Can shadow type with value
        shadow let Distance = 3.14
        println("Distance value: {}", Distance)
    }

    // Distance is type again
    let d: Distance = 10.5
}

// ========== Different scopes, same names (independent) ==========
function scope_a() {
    let x = 5  // x (value) in scope_a's namespace
    record MyType { field: i32 }  // MyType (type) in scope_a
}

function scope_b() {
    let x = "hello"  // ✅ OK: Different scope, independent namespace
    record MyType { data: string }  // ✅ OK: Different scope
    // MyType in scope_b is independent of MyType in scope_a
}

function main(): i32 {
    // Demonstrate unified namespace
    let origin = Point { x: 0, y: 0 }
    let p = Point { x: 3, y: 4 }
    let dist = calculate_distance(origin, p)
    println("Distance: {}", dist)

    demonstrate_shadowing()
    cross_category_examples()

    result 0
}

/* Summary of namespace behavior:
1. Each scope has ONE namespace (unified)
2. Types and values share this namespace (cannot conflict)
3. Different scopes have independent namespaces (names can reoccur)
4. Shadowing allows reusing names in inner scopes
5. Name categories exist but don't create separate namespaces
*/
```

---

## Writing Standards (Apply to All Sections)

### Normative Language
- **shall / shall not** = mandatory requirements
- **should / should not** = recommendations
- **may / may not** = permissions
- **can / cannot** = capability (informative)
- Per ISO/IEC Directives Part 2

### Cross-References
- Use `[REF_TBD]` for all cross-references
- Examples:
  - "See §[REF_TBD] for detailed rules"
  - "Part [REF_TBD] specifies formal semantics"
  - "Cross-reference to Annex [REF_TBD]"

### Formal Notation
- **Judgment forms:** `Γ ⊢ e : T` per §[REF_TBD]
- **Inference rules:** `[Rule-Name]` format per §[REF_TBD]
- **Grammar snippets:** EBNF notation per §[REF_TBD]
- **Note:** "For complete grammar, see Annex A"

### Example Requirements
- ONE canonical example per major concept
- Example is comprehensive (demonstrates all aspects)
- Example is minimal yet complete
- Both valid (✅) and invalid (❌) cases shown where appropriate
- Example is self-contained and runnable where possible
- Example quality: publication-ready

### ISO/ECMA Section Structure
Each section follows this structure:
1. **Section number + title**
2. **Introduction paragraph** (overview of section)
3. **Numbered subsections** (X.Y.Z format)
4. **Formal definitions** (Definition X.Y format)
5. **Normative rules** (using shall/should/may)
6. **Inference rules and algorithms** (formal notation)
7. **Canonical example** (one comprehensive demonstration)
8. **Cross-references** ([REF_TBD] placeholders)
9. **Notes** (informative clarifications where helpful)

### Error Codes
- Part II range: E2xxx
- Format: E2001, E2002, etc.
- Each error code documented with:
  - Brief description
  - Context where it occurs
  - Example triggering the error

### Consistency Requirements
- Terminology consistent with glossary
- Judgment form notation consistent
- Example formatting consistent
- Section numbering follows Proposed_Organization.md
- Cross-references use [REF_TBD]

---

## Validation Checklist (For Each Section)

Before considering a section complete, verify:

- ✅ All subsections from Proposed_Organization.md present (or deviation justified)
- ✅ Formal definitions for key concepts (Definition X.Y format)
- ✅ Normative language used correctly (shall/should/may)
- ✅ Inference rules for semantic specifications
- ✅ Algorithms for procedures (where applicable)
- ✅ Grammar productions (inline + Annex A reference)
- ✅ ONE canonical example per major concept
- ✅ Example is comprehensive and publication-quality
- ✅ Cross-references use [REF_TBD]
- ✅ Error codes defined and documented
- ✅ Implementation-defined aspects marked
- ✅ Undefined behavior tagged [UB]
- ✅ Section structure follows ISO/ECMA format
- ✅ Content is transformed, not copied
- ✅ Sufficient detail for implementer

---

## Success Criteria

PART II is complete when:

✅ All 10 sections written to specification
✅ Every concept has formal definition
✅ Every requirement is normative (shall/should/may)
✅ ONE canonical example per concept
✅ All sections self-contained within Part II scope
✅ All forward references use [REF_TBD]
✅ ISO/ECMA formatting throughout
✅ Zero ambiguity - normative precision achieved
✅ Implementer-ready specification (can build compiler from Part II for program structure)
✅ LLM-friendly clarity maintained
✅ Publication-quality technical writing
✅ Complete transformation (no copy-paste)
✅ All design decisions incorporated
✅ Structure validated against Proposed_Organization.md
✅ Content sourcing validated against Content_Location_Mapping.md

---

## Estimated Completion

**Sections:** 10
**Subsections:** 50
**Completeness criterion:** Publication-ready ISO/ECMA specification

**This plan serves as the authoritative guide for writing PART II: BASICS AND PROGRAM MODEL of the Cursive Language Specification.**
