# Cantrip Language Specification - Complete Final Outline

**Version:** 1.0  
**Date:** 2025-01-28  
**Purpose:** Comprehensive, consolidated outline combining the best elements of all three previous outlines, aligned with existing specification files.

---

## Overview

This outline structures the complete Cantrip language specification, consolidating:

- The sequential numbering approach from Outline 3 (aligning with existing files)
- The comprehensive coverage from Outlines 1 and 2
- Status tracking based on existing specification content
- Logical organization that respects dependencies and cross-references

**Existing Files:**

- `00_LLM_Onboarding.md` - Quick reference for LLMs (supplementary, not part of formal spec)
- `01_FrontMatter.md` - Title, conformance, notation, lexical structure, abstract syntax, attributes
- `02_Types.md` - Type system (scaffold, needs expansion)
- `03_Expressions.md` - Expressions (loops complete, others need addition)
- `04_Contracts.md` - Contract system (comprehensive)

**File Numbering Strategy:**

- Sequential numbering (00-14 for main spec, appendices separately)
- Aligns with existing structure
- Allows easy cross-referencing
- Matches specification section numbering where possible

---

## Part I: Foundations

### 00_LLM_Onboarding.md (Existing - Keep as supplementary reference)

**Status:** COMPLETE (keep as-is)  
**Purpose:** Quick reference for LLMs/AI tools working with Cantrip

**Note:** This file is supplementary and not part of the formal specification structure. It provides a condensed, LLM-friendly overview.

---

### 01_FrontMatter.md (Existing - Verify and consolidate)

**Status:** MOSTLY COMPLETE (~2880 lines)

**Structure:**

1. **Title and Conformance**

   - Abstract
   - Version information (1.3.0)
   - Status and target audience
   - Key words for requirement levels (RFC 2119)
   - Document conventions

2. **Notation and Mathematical Foundations**

   - Metavariables (x, y ∈ Var; T, U ∈ Type; etc.)
   - Judgment forms (Γ ⊢ e : T, ⟨e, σ⟩ ⇓ ⟨v, σ'⟩, {P} e {Q})
   - Formal operators (set theory, logic, relations)
   - Inference rule format
   - Substitution notation ([x ↦ v])
   - Reading guide with examples

3. **Lexical Structure**

   - Source files (UTF-8 encoding, line endings, BOM)
   - Comments (line, block, doc, module)
   - Identifiers (syntax, restrictions, naming conventions)
   - Keywords (reserved and contextual)
   - Literals (integer, float, boolean, character, string)
   - Statement termination (newline-based with 4 continuation rules)
   - Tokenization rules (maximal munch, etc.)

4. **Abstract Syntax**

   - Type language (abstract syntax)
   - Expression language
   - Pattern language
   - Value language
   - Assertions (contract predicates)
   - Effects (abstract syntax)
   - AST representation and well-formedness

5. **Attributes and Annotations**
   - Attribute syntax and placement
   - Core attributes:
     - `[[repr(...)]]` - memory layout control
     - `[[verify(...)]]` - verification mode (static/runtime/none)
     - `[[overflow_checks(...)]]` - overflow behavior
     - `[[module(...)]]` - module metadata
     - `[[alias(...)]]` - alternative names for tooling
   - Additional attributes: inline, no_inline, deprecated, must_use, cold, hot
   - Attribute processing and inheritance rules
   - Conflict detection

**Action Items:**

- [ ] Verify all sections present and complete
- [ ] Ensure consistent terminology throughout
- [ ] Verify cross-references are accurate
- [ ] Check that notation usage is consistent with subsequent sections

---

## Part II: Type System

### 02_Types.md (Existing - Expand from scaffold)

**Status:** SCAFFOLD EXISTS (needs expansion)

**Structure:**

1. **Type System Overview**

   - Type taxonomy (primitives, compounds, abstraction, permissions)
   - Type properties (Copy, Sized, Send, Sync markers)
   - Subtyping relation (<:) and type equivalence (≡)
   - Well-formedness rules
   - Type checking model (bidirectional)
   - Integration with permissions, effects, contracts, regions

2. **Primitive Types**

   - Integer types (i8, i16, i32, i64, isize, u8, u16, u32, u64, usize)
     - Size, alignment, ranges
     - Default literal types (i32, f64)
     - Overflow behavior and semantics
   - Floating-point types (f32, f64)
     - IEEE 754 conformance
     - Literal syntax
   - Boolean type (bool)
   - Character type (char)
     - Unicode scalar value semantics
     - Escape sequences
   - Never type (!)
     - Bottom type semantics
     - Divergence
   - String types (str, String)
     - Slice vs owned
     - UTF-8 encoding
     - String literal syntax

3. **Compound Types**

   - Arrays `[T; n]`
     - Fixed-size, stack-allocated
     - Size as part of type
     - Memory layout
     - Bounds checking
   - Slices `[T]`
     - Dynamic views
     - Dense pointer representation
     - Unsized types
   - Tuples `(T₁, ..., Tₙ)`
     - Product types
     - Field access (.0, .1, etc.)
     - Unit type ()

4. **Records (Product Types)**

   - Declaration syntax (`record Name { fields }`)
   - Nominally typed (distinct by name)
   - Field access (.field)
   - Memory layout (field ordering, padding, alignment)
   - Procedures on records (methods)
   - Field visibility (public, private, internal)
   - Default values (if applicable)

5. **Enums (Sum Types)**

   - Declaration syntax (`enum Name { variants }`)
   - Variant syntax (unit, tuple, record)
   - Tagged union representation
   - Discriminant representation
   - Pattern matching (exhaustiveness)
   - Memory layout
   - Size calculation

6. **Modals (State Machines)** - Cantrip-specific innovation

   - Modal declaration syntax
   - State declarations (@State)
   - Transition definitions (@S₁ -> method() -> @S₂)
   - Type-level state tracking (T@S)
   - State transitions
   - Static verification (compile-time state checking)
   - Invalid state access prevention
   - No runtime overhead (states erased after type checking)

7. **Contracts (Abstract Interfaces)**

   - Contract declaration syntax
   - Abstract signatures only (NO implementations)
   - Contract clauses (uses, must, will)
   - Contract implementation
   - Contract extension (single and multiple)
   - Contract satisfaction rules
   - Polymorphism through contracts
   - Contract bounds in generics
   - Associated types in contracts
   - Coherence and orphan rules

8. **Traits (Concrete Code Reuse)**

   - Trait declaration syntax
   - Concrete implementations (WITH bodies)
   - Template code sharing
   - Trait attachment to types
   - Trait vs Contracts distinction (critical!)
   - Use cases and guidance

9. **Generics (Parametric Polymorphism)**

   - Type parameters (`<T, U>`)
   - Generic declarations
   - Parameter syntax and bounds
   - Const generics (`<const N: usize>`)
   - Where clauses
   - Generic instantiation (monomorphization)
   - Type inference
   - Specialization rules (if applicable)
   - Coherence rules
   - Variance and subtyping

10. **Map Types (Function/Procedure Types)**

    - Function types (`map(T₁, ..., Tₙ) -> U ! ε`)
    - Procedure types
    - Effect annotations (ε)
    - Parameter passing semantics
    - Return types
    - Higher-order functions

11. **Permission Types**

    - Lexical Permission System (LPS) overview
    - Three permission categories:
      - `own T` - ownership (exactly one reference)
      - `mut T` - mutable reference (multiple allowed)
      - `imm T` or `T` - immutable reference (unlimited)
    - Permission rules
    - Uniform parameter passing (all types pass by permission)
    - Copy trait semantics (explicit copying only)
    - Move semantics (explicit ownership transfer)
    - Multiple mutable references allowed (KEY DIFFERENCE from Rust)
    - Permission inference (if applicable)

12. **Reference Types**

    - Safe pointers (`Ptr<T>@State`)
    - Raw pointers (`*T`, `*mut T`)
    - Pointer arithmetic (if allowed)
    - Null pointer handling
    - Pointer safety guarantees
    - Unsafe operations

13. **Type Aliases**

    - Type alias syntax
    - New type names (distinct types vs aliases)
    - Associated types (in contracts and traits)

14. **Type Inference**
    - Local type inference (let bindings, function parameters)
    - Inference rules (Hindley-Milner style, if applicable)
    - Inference limitations
    - Explicit type annotations (when needed)

**Action Items:**

- [ ] Expand scaffold into complete specification
- [ ] Ensure all type categories fully covered
- [ ] Include formal type rules (T-\* rules) where appropriate
- [ ] Provide comprehensive examples for each type category
- [ ] Cross-reference with Expressions (§03), Permissions (§09), Contracts (§04)

---

## Part III: Expressions

### 03_Expressions.md (Existing - Expand from loops)

**Status:** PARTIAL (Loops section complete, others need addition)

**Structure:**

1. **Expression System Overview**

   - Expression categories (primary, postfix, unary, binary, control flow)
   - Expression vs statement distinction
   - Evaluation order (left-to-right guarantees)
   - Sequence points
   - Value categories
   - Effect composition

2. **Primary Expressions**

   - Literals (reference to §01 for syntax)
     - Integer, float, boolean, character, string literals
     - Literal type inference
   - Variable references
   - Parenthesized expressions
   - Block expressions (statements as values)

3. **Operators and Operations**

   - Arithmetic operators (+, -, \*, /, %, \*\*)
     - Integer and floating-point semantics
     - Overflow behavior
   - Bitwise operators (&, |, ^, <<, >>)
   - Comparison operators (==, !=, <, <=, >, >=)
   - Logical operators (&&, ||, !)
     - Short-circuit evaluation
   - Assignment operators (=, +=, -=, \*=, /=, %=, etc.)
   - Pipeline operator (=>)
   - Operator precedence table (15 levels)
   - Operator associativity rules

4. **Function Calls and Member Access**

   - Function calls
     - Call syntax
     - Argument evaluation order
     - Return values
     - Effect propagation
   - Method calls
     - Dot notation
     - Receiver types
     - Method resolution
   - Field access
     - Record fields (.field)
     - Tuple fields (.0, .1)
   - Array/Slice indexing ([index])
     - Bounds checking
     - Unsafe indexing

5. **Conditional Expressions**

   - If expressions
     - Syntax and semantics
     - Type requirements (condition: bool)
     - Expression vs statement forms
   - If-let pattern matching
     - Pattern matching in conditionals
     - Guard patterns

6. **Pattern Matching (match)**

   - Match expression syntax
   - Exhaustiveness checking
   - Pattern types:
     - Literal patterns
     - Variable binding
     - Wildcard (\_)
     - Tuple destructuring
     - Enum patterns
     - Record patterns
     - Guard patterns (if condition)
   - Pattern matching semantics
     - Order of evaluation
     - Binding scope
     - Match guards

7. **Loops** (Status: COMPLETE)

   - Unified loop syntax (`loop` keyword)
   - Infinite loops
   - While-style (condition)
   - For-style (pattern in range/collection)
   - Iterator-style
   - Loop verification:
     - Invariants (`with`)
     - Variants (`by`)
   - Loop control:
     - `break` with values
     - `continue`
     - Labels
   - Termination verification
   - Loop examples and patterns

8. **Let and Variable Bindings**

   - Let bindings (`let x = e`)
     - Immutable by default
     - Type inference
     - Pattern bindings (destructuring)
   - Variable bindings (`var x = e`)
     - Mutable variables
     - Scope and lifetime
   - Shadowing rules

9. **Ownership Transfer**

   - Move semantics (`move e`)
     - Explicit ownership transfer
     - After-move restrictions
   - Copy semantics (`.copy()` method)
   - Ownership rules

10. **Region Expressions**

    - Region blocks (`region r { ... }`)
    - Region allocation (`alloc_in<r>(value)`)
    - Region lifetime rules
    - O(1) bulk deallocation
    - Region escape prevention

11. **Comptime Expressions**

    - Compile-time execution (`comptime { ... }`)
    - Comptime context
    - Compile-time vs runtime distinction
    - Comptime restrictions

12. **Lambda Expressions** (if supported)
    - Closure syntax
    - Capture semantics (by value vs by reference)
    - Closure types
    - Effect capture

**Action Items:**

- [ ] Expand from existing loops section
- [ ] Add all expression types beyond loops
- [ ] Ensure consistent with §02 (Types) and §04 (Contracts)
- [ ] Include formal evaluation rules where appropriate
- [ ] Provide comprehensive examples

---

## Part IV: Contracts and Clauses

### 04_Contracts.md (Existing - Verify completeness)

**Status:** COMPREHENSIVE (~2862 lines)

**Structure:**

1. **Contract System Overview**

   - Purpose and design philosophy
   - Contract components:
     - Effects (`uses`)
     - Preconditions (`must`)
     - Postconditions (`will`)
     - Type constraints (`where`)
   - Contract checking strategies (static vs dynamic)
   - Contract composition
   - Relationship to other features

2. **Behavioral Contracts**

   - Contract declaration (abstract interfaces)
   - Contract vs Traits distinction (CRITICAL)
   - Contract implementation
   - Contract extension (single and multiple)
   - Contract satisfaction rules
   - Polymorphism through contracts
   - Associated types in contracts
   - Coherence and orphan rules

3. **Effect Clauses (`uses`)**

   - Effect declaration syntax
   - Standard effects catalog:
     - I/O effects (io.read, io.write)
     - Network effects (net.tcp, net.udp)
     - Memory effects (alloc.heap, alloc.region)
     - Concurrency effects (thread.spawn, thread.join)
     - System effects (sys.env, sys.time)
     - FFI effects (ffi.call)
   - Effect composition and propagation
   - Effect checking (static)
   - Pure procedures (no effects)
   - Effect polymorphism

4. **Precondition Clauses (`must`)**

   - Precondition syntax
   - Precondition semantics
   - Static vs dynamic checking
   - Precondition inference (if applicable)
   - Frame conditions
   - Precondition weakening rules

5. **Postcondition Clauses (`will`)**

   - Postcondition syntax
   - Postcondition semantics
   - `@old(expr)` operator (capture at entry)
   - `result` reference (return value)
   - Static vs dynamic checking
   - Postcondition strengthening rules

6. **Constraint Clauses (`where`)**

   - Type constraint syntax
   - Type-level invariants
   - Field constraints
   - Constraint checking (at construction and mutation)

7. **Clause Composition**
   - Clause interaction
   - Effects and conditions
   - Multiple clauses on same procedure
   - Inheritance through calls
   - Transitive effects
   - Precondition obligations
   - Postcondition guarantees

**Action Items:**

- [ ] Verify all sections complete
- [ ] Ensure consistent terminology (uses/must/will, not needs/requires/ensures)
- [ ] Cross-reference with Types (§02), Expressions (§03), Functions (§06)
- [ ] Verify examples are comprehensive and accurate

---

## Part V: Statements and Control Flow

### 05_Statements.md (New file needed)

**Status:** NEEDS CREATION

**Structure:**

1. **Statements Overview**

   - Statement vs expression distinction
   - Statement categories:
     - Expression statements
     - Declaration statements
     - Control flow statements
   - Statement termination (newline-based, semicolon-separated)
   - Statement sequencing

2. **Expression Statements**

   - Using expressions as statements
   - Side effects
   - Discarded values

3. **Declaration Statements**

   - Let statements
   - Variable declarations
   - Type aliases in statements

4. **Assignment Statements**

   - Simple assignment (=)
   - Compound assignment (+=, -=, \*=, /=, etc.)
   - Assignment rules and semantics

5. **Control Flow Statements**

   - If statements (vs if expressions)
   - Match statements (vs match expressions)
   - Loop statements (reference to §03 for details)
   - Break statements (with values, with labels)
   - Continue statements (with labels)
   - Return statements
     - Single returns
     - Multiple returns (if applicable)
     - Early returns

6. **Block Statements**

   - Block syntax ({ ... })
   - Lexical scope
   - Variable visibility
   - Block expressions (returning values)

7. **Defer Statements** (if applicable)

   - Defer syntax
   - Defer execution order (LIFO)
   - Cleanup patterns

8. **Labeled Statements**
   - Label syntax
   - Label scope
   - Break/continue with labels

**Action Items:**

- [ ] Create new file
- [ ] Ensure consistency with Expressions (§03)
- [ ] Cross-reference with Functions (§06) for return statements
- [ ] Include formal semantics where appropriate

---

## Part VI: Functions and Procedures

### 06_Functions.md (New file needed)

**Status:** NEEDS CREATION

**Structure:**

1. **Functions and Procedures Overview**

   - Function vs procedure distinction
   - Function types (map types from §02)
   - Procedure types
   - Side effects distinction

2. **Function Declarations**

   - Function syntax (`function name(params): ReturnType { body }`)
   - Parameters and types
   - Return types
   - Generic functions
   - Visibility modifiers
   - Contract clauses (uses, must, will)

3. **Procedure Declarations**

   - Procedure syntax (`procedure name(params): ReturnType { body }`)
   - Self parameter (for methods)
   - State transitions (for modals)
   - Contract clauses

4. **Parameters**

   - Parameter syntax
   - Permission annotations (own, mut, imm/T)
   - Default parameter values (if applicable)
   - Named parameters (if applicable)
   - Variadic parameters (if applicable)
   - Parameter passing semantics (uniform, all by permission)

5. **Return Values**

   - Return syntax
   - Single returns
   - Multiple returns (if applicable)
   - Named return values (if applicable)
   - Return type inference (if applicable)
   - Implicit returns (block expressions)

6. **Methods**

   - Method declaration syntax
   - Self parameter (`self: Type`, `mut self`, etc.)
   - Method receivers
   - Associated procedures
   - Method resolution
   - Method chaining

7. **Higher-Order Functions**

   - Functions as values
   - Function composition
   - Closures and capture
   - Effect polymorphism

8. **Call Semantics**

   - Argument evaluation order
   - Stack frame layout
   - Calling conventions
   - Parameter passing (uniform semantics)

9. **Function Overloading** (if applicable)

   - Overloading rules
   - Resolution
   - Orphan rules

10. **Inline and Optimization**
    - Inline hints (`[[inline]]`)
    - Optimization attributes
    - Compiler hints

**Action Items:**

- [ ] Create new file
- [ ] Cross-reference with Types (§02) for map types
- [ ] Cross-reference with Contracts (§04) for contract clauses
- [ ] Ensure consistency with Expressions (§03) for calls

---

## Part VII: Declarations and Scope

### 07_Declarations.md (New file needed)

**Status:** NEEDS CREATION

**Structure:**

1. **Declarations Overview**

   - Declaration categories:
     - Constant declarations
     - Variable declarations
     - Type declarations (record, enum, modal, contract, trait)
     - Function/procedure declarations (reference §06)
   - Declaration order
   - Forward declarations (if applicable)

2. **Constant Declarations**

   - Const syntax
   - Compile-time constants
   - Constant expressions
   - Const generics

3. **Variable Declarations**

   - Let vs var distinction
   - Type inference
   - Initialization rules
   - Definite assignment
   - Default/zero values (if applicable)
   - Mutability rules

4. **Type Declarations**

   - Record declarations (reference §02)
   - Enum declarations (reference §02)
   - Modal declarations (reference §02)
   - Type aliases (reference §02)
   - Contract declarations (reference §04)
   - Trait declarations (reference §02)

5. **Scope Rules**

   - Block scope
   - Function scope
   - Module scope
   - Shadowing rules
   - Name resolution order

6. **Visibility and Access Control**

   - Public (exported)
   - Internal (module-internal)
   - Private (type-internal)
   - Visibility modifiers
   - Cross-module access rules

7. **Name Resolution**

   - Identifier lookup
   - Qualified names (module::item)
   - Resolution order
   - Ambiguity resolution

8. **Lifetime and Storage**

   - Static storage
   - Automatic storage (stack)
   - Region storage (reference §09)
   - Lifetime rules

9. **Predeclared Identifiers**
   - Built-in types (reference §02)
   - Built-in functions (reference §12)
   - Built-in constants
   - Reserved names

**Action Items:**

- [ ] Create new file
- [ ] Ensure consistency across all referenced sections
- [ ] Cross-reference extensively with other sections
- [ ] Include formal scoping rules where appropriate

---

## Part VIII: Modules and Organization

### 08_Modules.md (New file needed)

**Status:** NEEDS CREATION

**Structure:**

1. **Modules Overview**

   - Module concept
   - File-based module system
   - Module organization
   - Module hierarchy

2. **Module Declaration**

   - Module syntax
   - Module naming
   - File system mapping
   - Module attributes (`[[module(...)]]`)

3. **Import System**

   - Import syntax (`import module::item`)
   - Import paths
   - Qualified imports
   - Selective imports (if applicable)
   - Re-exports
   - Circular dependencies (if allowed)
   - Import resolution

4. **Module Visibility**

   - Public items
   - Internal items
   - Private items
   - Cross-module access
   - Encapsulation rules

5. **Package System** (if applicable)

   - Package structure
   - Package dependencies
   - Package management
   - Package initialization order

6. **Program Entry Point**

   - Main function/procedure
   - Entry point rules
   - Initialization order

7. **Linking and Code Generation**
   - Separate compilation
   - Linking rules
   - Symbol export
   - Linkage specifiers

**Action Items:**

- [ ] Create new file
- [ ] Cross-reference with Declarations (§07) for visibility
- [ ] Ensure consistency with FrontMatter (§01) for attributes

---

## Part IX: Memory Model and Permissions

### 09_Memory.md (New file needed)

**Status:** NEEDS CREATION

**Structure:**

1. **Memory Model Overview**

   - Memory safety guarantees
   - What Cantrip prevents (use-after-free, double-free, leaks)
   - What Cantrip does NOT prevent (aliasing, data races)
   - Design philosophy

2. **Lexical Permission System (LPS)**

   - Permission overview (reference §02 for details)
   - Permission types (own, mut, imm/T)
   - Permission semantics
   - Permission conversion rules
   - Multiple mutable references (allowed)
   - Differences from Rust borrow checker

3. **Memory Regions**

   - Region syntax (`region r { ... }`)
   - Region allocation (`alloc_in<r>(value)`)
   - Region deallocation (LIFO order)
   - Region lifetime rules
   - Region escape prevention
   - O(1) bulk deallocation
   - Performance characteristics

4. **Stack Allocation**

   - Automatic allocation
   - Stack frame layout
   - Scope-based lifetime
   - Stack limits

5. **Heap Allocation**

   - Explicit heap allocation
   - Heap lifetime management
   - RAII-style cleanup
   - Allocation functions

6. **Pointer Safety**

   - Safe pointers (`Ptr<T>@State`)
   - Raw pointers (`*T`, `*mut T`)
   - Null safety
   - Dangling pointer prevention
   - Unsafe operations (reference §13)

7. **Copy and Move Semantics**

   - Copy trait
   - Move semantics (explicit `move`)
   - Ownership transfer
   - Copy vs move rules
   - Uniform parameter passing

8. **Memory Layout**

   - Type sizes and alignment
   - Record layout
   - Enum layout
   - Array layout
   - Pointer representation
   - Layout attributes (`[[repr(...)]]`)
   - Padding and alignment rules

9. **Memory Safety Properties**

   - Guarantees (use-after-free, double-free prevention)
   - Limitations (aliasing bugs, data races not prevented)
   - Trade-offs and rationale

10. **Lifetime Management**
    - Scope-based lifetimes
    - Region-based lifetimes
    - Lifetime inference (if applicable)
    - Lifetime constraints

**Action Items:**

- [ ] Create new file
- [ ] Cross-reference extensively with Types (§02) for permissions
- [ ] Cross-reference with Expressions (§03) for region expressions
- [ ] Ensure consistent terminology with rest of spec

---

## Part X: Advanced Features

### 10_Comptime.md (New file needed)

**Status:** NEEDS CREATION

**Structure:**

1. **Comptime Overview**

   - Compile-time execution concept
   - Comptime vs runtime distinction
   - Design philosophy (no macros, comptime only)

2. **Comptime Expressions**

   - Comptime blocks (`comptime { ... }`)
   - Compile-time evaluation
   - Compile-time constants
   - Comptime restrictions

3. **Generic Specialization**

   - Const generics (reference §02)
   - Type specialization
   - Compile-time code generation
   - Monomorphization

4. **Constant Folding**

   - Constant propagation
   - Optimization opportunities
   - Compile-time computations

5. **Comptime Metaprogramming**

   - Code generation without macros
   - Template expansion
   - Type-level computation
   - Compile-time reflection (if applicable)

6. **Comptime Limitations**
   - Restrictions
   - Turing completeness (if applicable)
   - Termination guarantees
   - Performance considerations

**Action Items:**

- [ ] Create new file
- [ ] Cross-reference with Types (§02) for const generics
- [ ] Cross-reference with Expressions (§03) for comptime expressions
- [ ] Ensure clarity on comptime vs runtime boundaries

---

### 11_Concurrency.md (New file needed)

**Status:** NEEDS CREATION (if concurrency features exist)

**Structure:**

1. **Concurrency Overview**

   - Concurrency model
   - Threading primitives
   - Safety guarantees
   - Data race model

2. **Threads**

   - Thread creation
   - Thread joining
   - Thread lifecycle
   - Thread effects (`thread.spawn`, `thread.join`)

3. **Synchronization**

   - Mutex
   - Locks
   - Condition variables (if applicable)

4. **Atomic Operations**

   - Atomic types
   - Atomic operations
   - Memory ordering
   - Compare-and-swap

5. **Channels** (if applicable)

   - Channel types
   - Send/receive operations
   - Channel semantics

6. **Concurrency Effects**

   - Effect system integration
   - Thread effects
   - Synchronization effects

7. **Data Races**
   - Race conditions
   - Undefined behavior
   - Prevention mechanisms (if any)
   - Programmer responsibility

**Action Items:**

- [ ] Create new file IF concurrency features exist
- [ ] Cross-reference with Contracts (§04) for effects
- [ ] Ensure safety guarantees are clearly stated

---

### 12_Builtins.md (New file needed)

**Status:** NEEDS CREATION

**Structure:**

1. **Built-in Functions Overview**

   - Built-in vs library distinction
   - Compiler integration
   - Special semantics

2. **Memory Functions**

   - Allocation functions (if any)
   - Size functions (`size_of`, `align_of`, etc.)
   - Memory operations

3. **Collection Functions**

   - Array operations (if any)
   - String operations (if any)

4. **Type Functions**

   - Type queries
   - Type conversions (explicit casts)

5. **System Functions**

   - I/O functions (if any)
   - System calls (if any)

6. **Debug Functions**

   - Assertions
   - Debugging aids

7. **Standard Library Overview**
   - Library organization
   - Core modules
   - Common types (Option, Result, Vec, etc.)
   - Reference to library documentation

**Action Items:**

- [ ] Create new file
- [ ] Catalog all built-in functions
- [ ] Provide standard library overview
- [ ] Cross-reference with Types (§02) for standard types

---

### 13_Unsafe.md (New file needed)

**Status:** NEEDS CREATION

**Structure:**

1. **Unsafe Overview**

   - Unsafe blocks concept
   - When unsafe is needed
   - Safety guarantees and limitations

2. **Unsafe Blocks**

   - Unsafe block syntax
   - Restrictions lifted in unsafe
   - Safety requirements

3. **Unsafe Operations**

   - Raw pointer operations
   - FFI calls (reference §14)
   - Memory operations
   - Unsafe effect (`unsafe.ptr`, `ffi.call`)

4. **FFI Primitives**

   - Foreign function calls
   - Calling conventions
   - Type compatibility

5. **Safety Guarantees**
   - What unsafe blocks allow
   - What unsafe blocks do NOT allow
   - Programmer responsibilities
   - Best practices

**Action Items:**

- [ ] Create new file
- [ ] Cross-reference with FFI (§14)
- [ ] Ensure safety boundaries are clearly defined
- [ ] Include examples of safe vs unsafe usage

---

### 14_FFI.md (New file needed)

**Status:** NEEDS CREATION

**Structure:**

1. **FFI Overview**

   - Foreign function interface concept
   - C interoperability goals
   - Safety considerations

2. **FFI Syntax**

   - External function declarations
   - Calling conventions
   - Linkage specifiers
   - `[[no_mangle]]` attribute

3. **Type Compatibility**

   - C-compatible types
   - Layout guarantees (`[[repr(C)]]`)
   - Type mappings (C types ↔ Cantrip types)
   - Size and alignment guarantees

4. **FFI Effects**

   - `ffi.call` effect
   - Effect requirements for FFI
   - Unsafe operations

5. **Unsafe Operations**

   - Unsafe blocks for FFI (reference §13)
   - Raw pointer operations
   - FFI safety rules

6. **Interoperability Examples**
   - Common patterns
   - Best practices
   - Error handling

**Action Items:**

- [ ] Create new file
- [ ] Cross-reference with Attributes (§01) for `[[repr(C)]]`
- [ ] Cross-reference with Unsafe (§13) for safety rules
- [ ] Include practical C interop examples

---

## Part XI: Operational Semantics

### 15_Semantics.md (New file needed)

**Status:** NEEDS CREATION

**Structure:**

1. **Operational Semantics Overview**

   - Evaluation model
   - Small-step vs big-step semantics
   - Evaluation judgments (⟨e, σ⟩ ⇓ ⟨v, σ'⟩)
   - Store model (σ)

2. **Expression Evaluation**

   - Value categories
   - Evaluation order (left-to-right)
   - Side effects
   - Effect tracking
   - Formal evaluation rules (E-\* rules)

3. **Statement Execution**

   - Statement semantics
   - Control flow
   - State transitions
   - Store updates

4. **Function Call Semantics**

   - Parameter passing (uniform semantics)
   - Stack frames
   - Return semantics
   - Calling conventions

5. **Memory Operations**

   - Allocation semantics
   - Deallocation semantics
   - Region operations (O(1) bulk free)
   - Store updates

6. **Type System Properties**

   - Progress theorem (well-typed programs don't get stuck)
   - Preservation theorem (types preserved under reduction)
   - Type safety guarantees

7. **Contract Semantics**

   - Precondition checking (static and dynamic)
   - Postcondition checking (static and dynamic)
   - Effect checking (static)
   - Contract failure behavior

8. **Runtime Behavior**
   - Undefined behavior
   - Implementation-defined behavior
   - Unspecified behavior
   - Panic semantics

**Action Items:**

- [ ] Create new file
- [ ] Provide formal operational semantics
- [ ] Cross-reference with all previous sections
- [ ] Ensure type safety theorems are stated and proven (sketched)

---

## Appendices

### appendix_grammar.md (Existing - Verify completeness)

**Status:** COMPLETE (verify consistency)

**Content:**

- Complete EBNF grammar
- Lexical grammar
- Syntactic grammar
- Well-formedness rules
- Grammar production index

**Action Items:**

- [ ] Verify all syntax forms from specification are covered
- [ ] Ensure consistency with main specification sections
- [ ] Verify operator precedence matches specification

---

### appendix_errors.md (Existing - Verify completeness)

**Status:** COMPLETE (verify cross-references)

**Content:**

- Error code catalog
- Error categories:
  - Parsing errors
  - Name resolution errors
  - Type errors
  - Contract errors
  - Memory errors
  - Permission errors
  - Effect errors
  - FFI errors
  - Concurrency errors
- Warning codes
- Diagnostic messages
- Error recovery suggestions

**Action Items:**

- [ ] Verify all error codes referenced in specification
- [ ] Ensure all error codes have corresponding sections
- [ ] Cross-reference with all specification sections
- [ ] Identify any missing error codes

---

### appendix_precedence.md (New file needed)

**Status:** NEEDS CREATION

**Content:**

- Complete operator precedence table (15 levels)
- Associativity rules (left/right)
- Operator descriptions
- Examples for each level

**Action Items:**

- [ ] Create new file
- [ ] Extract precedence information from Expressions (§03)
- [ ] Provide comprehensive examples
- [ ] Cross-reference with grammar appendix

---

### appendix_keywords.md (New file needed)

**Status:** NEEDS CREATION

**Content:**

- Complete keyword list (54 reserved keywords)
- Contextual keywords
- Deprecated keywords (needs, requires, ensures - replaced by uses, must, will)
- Reserved for future use

**Action Items:**

- [ ] Create new file
- [ ] Extract keyword list from FrontMatter (§01)
- [ ] Document deprecation status
- [ ] Include usage guidance

---

### appendix_naming.md (New file needed)

**Status:** NEEDS CREATION

**Content:**

- Naming conventions:
  - Type names (CamelCase)
  - Function names (snake_case)
  - Constants (SCREAMING_SNAKE_CASE)
  - Type variables (single uppercase: T, U, V)
- Identifier rules
- Reserved names
- Best practices

**Action Items:**

- [ ] Create new file
- [ ] Extract naming conventions from FrontMatter (§01)
- [ ] Provide examples
- [ ] Include rationale

---

## Summary Statistics

**Total Specification Files:** 16 main files + 5 appendices = 21 files

**Main Specification Files:**

1. 00_LLM_Onboarding.md (supplementary)
2. 01_FrontMatter.md (foundations)
3. 02_Types.md (type system)
4. 03_Expressions.md (expressions)
5. 04_Contracts.md (contracts)
6. 05_Statements.md (statements)
7. 06_Functions.md (functions)
8. 07_Declarations.md (declarations)
9. 08_Modules.md (modules)
10. 09_Memory.md (memory)
11. 10_Comptime.md (comptime)
12. 11_Concurrency.md (concurrency - if applicable)
13. 12_Builtins.md (built-ins)
14. 13_Unsafe.md (unsafe)
15. 14_FFI.md (FFI)
16. 15_Semantics.md (operational semantics)

**Appendices:**

- appendix_grammar.md
- appendix_errors.md
- appendix_precedence.md
- appendix_keywords.md
- appendix_naming.md

**Status Summary:**

- **Complete (4):** 00, 01 (mostly), 03 (partial), 04
- **Needs Expansion (2):** 01 (verify), 02
- **Needs Creation (10-11):** 05-15 (depending on concurrency)

---

## Implementation Priority

**Phase 1: Foundations (Complete existing work)**

1. Verify and complete 01_FrontMatter.md
2. Expand 02_Types.md from scaffold
3. Complete 03_Expressions.md (add all expression types)

**Phase 2: Core Language Features** 4. Create 05_Statements.md 5. Create 06_Functions.md 6. Create 07_Declarations.md 7. Create 08_Modules.md

**Phase 3: Memory and Advanced Features** 8. Create 09_Memory.md 9. Create 10_Comptime.md 10. Create 11_Concurrency.md (if applicable) 11. Create 12_Builtins.md 12. Create 13_Unsafe.md 13. Create 14_FFI.md

**Phase 4: Semantics and Appendices** 14. Create 15_Semantics.md 15. Create appendix_precedence.md 16. Create appendix_keywords.md 17. Create appendix_naming.md

**Phase 5: Cross-Reference and Quality** 18. Verify all cross-references 19. Verify error codes match sections 20. Verify grammar matches specification 21. Consistency pass (terminology, formatting, style)

---

## Quality Criteria

Each specification file should:

1. **Structure:**

   - Overview section
   - Syntax (concrete and abstract)
   - Static Semantics (type rules, well-formedness)
   - Dynamic Semantics (evaluation rules, operational behavior)
   - Examples and Patterns
   - Integration with other features

2. **Style:**

   - Professional, clear, concise
   - Not terse or abbreviated
   - Include all necessary content
   - Avoid verbosity

3. **Formal Notation:**

   - Use metavariables from §01
   - Use judgment forms from §01
   - Include formal type rules (T-\*) where appropriate
   - Include formal evaluation rules (E-\*) where appropriate

4. **Cross-References:**

   - Use section references (§X.Y.Z format)
   - Link to related features
   - Reference notation usage

5. **Examples:**

   - Provide concrete code examples
   - Show both correct and incorrect usage
   - Include patterns and best practices

6. **Completeness:**
   - Cover all language features in scope
   - Address edge cases
   - Document limitations and trade-offs

---

**End of Outline**
