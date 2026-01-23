# Cantrip Language Specification - Complete Outline

**Version:** 1.3.0  
**Date:** 2025-01-28  
**Status:** Master Outline

## Overview

This outline structures the complete Cantrip language specification, consolidating the best elements from multiple outline proposals while aligning with existing specification files and the LanguageSpecGuide.md structure.

### Design Principles

1. **Practical Organization**: Balance between comprehensiveness and usability
2. **Existing File Alignment**: Respects existing file structure (00*LLM_Onboarding.md, 01-04*\*.md)
3. **Logical Dependencies**: Parts build on each other in order
4. **Comprehensive Coverage**: All Cantrip features documented
5. **Consistent Structure**: Each section follows LanguageSpecGuide.md template

---

## Part I: Foundations (Spec/01)

### 01_FrontMatter.md (Existing - Expand/Verify)

**Status:** Substantially complete (~2880 lines), verify completeness and integration

**Contents:**

1. **Title, Conformance, and Abstract**

   - Specification version and status
   - Target audience and purpose
   - Conformance definition
   - Key words for requirement levels (RFC 2119)
   - Document conventions

2. **Mathematical Foundations**

   - Metavariables and conventions (variables, types, expressions, effects, states, assertions, contexts, stores)
   - Judgment forms (typing, effects, evaluation, Hoare triples, state transitions)
   - Formal operators (set theory, logic, relations, substitution)
   - Inference rule format
   - Reading guide for formal rules
   - Cross-references to notation usage

3. **Lexical Structure**

   - Source files (UTF-8 encoding, line endings, BOM handling)
   - Comments (line, block, doc, module)
   - Identifiers (syntax, restrictions, naming)
   - Keywords (reserved and contextual)
   - Literals (integer, float, boolean, character, string)
   - Statement termination (newline-based with 4 continuation rules)
   - Optional semicolons
   - Whitespace handling
   - Tokenization rules (maximal munch)

4. **Abstract Syntax**

   - Type language (abstract syntax)
   - Expression language
   - Pattern language (for matching/destructuring)
   - Value language (fully evaluated expressions)
   - Assertion language (contract predicates)
   - Effect language (abstract syntax)
   - AST representation
   - Relationship to concrete syntax
   - Well-formedness foundations

5. **Attributes and Annotations**
   - Attribute syntax and placement
   - Core attributes:
     - `[[repr(...)]]` - memory layout control
     - `[[verify(...)]]` - verification mode
     - `[[overflow_checks(...)]]` - overflow behavior
     - `[[module(...)]]` - module metadata
     - `[[alias(...)]]` - alternative names for tooling
     - `[[inline]]`, `[[no_inline]]` - inlining hints
     - `[[no_mangle]]` - symbol name preservation
     - `[[deprecated(...)]]` - deprecation markers
     - `[[must_use]]`, `[[cold]]`, `[[hot]]` - optimization hints
   - Attribute processing phases
   - Attribute inheritance rules (do not inherit)
   - Conflict detection

**Action Items:**

- [ ] Verify all sections complete
- [ ] Ensure consistent terminology throughout
- [ ] Integrate any missing content from other proposals

---

## Part II: Type System (Spec/02)

### 02_Types.md (Expand from existing scaffold)

**Status:** Scaffold exists, needs expansion into complete specification

**Structure:**

1. **Type System Overview**

   - Type taxonomy (primitive, compound, reference, abstraction, permission)
   - Type properties (Copy, Sized markers)
   - Type classification summary
   - Relationship to permissions, effects, contracts, regions

2. **Type Foundations**

   - Type grammar (concrete and abstract)
   - Kind system (Type, Eff, Reg)
   - Typing contexts and judgments (Γ ⊢ e : T)
   - Well-formedness rules
   - Subtyping relation (<:) with variance rules
   - Type equivalence (≡)
   - Permission-indexed types integration
   - Effect-annotated map types (map(...) → U ! ε)
   - Effect lattice laws (idempotence, commutativity, associativity)
   - Effect polymorphism (∀ε) hooks

3. **Primitive Types**

   - Integer types (i8, i16, i32, i64, isize, u8, u16, u32, u64, usize)
     - Size, alignment, ranges
     - Default literal types (i32 for integers, f64 for floats)
     - Overflow behavior
   - Floating-point types (f32, f64)
     - IEEE 754 conformance
     - Literal syntax
   - Boolean type (bool)
   - Character type (char) - Unicode scalar value semantics
   - Never type (!) - bottom type, uninhabited
   - String types (str, String)
     - Slice vs owned
     - UTF-8 encoding

4. **Product Types**

   - Unit type (())
   - Tuples ((T₁, ..., Tₙ))
     - Field access
     - Memory layout
   - Records (record Name { fields })
     - Nominally typed
     - Field access and initialization
     - Procedures on records
     - Memory layout and alignment
     - Default values (if any)
     - Field visibility

5. **Sum Types**

   - Enums (enum Name { variants })
     - Variant syntax (unit, tuple, record variants)
     - Pattern matching exhaustiveness
     - Discriminant representation
     - Memory layout
     - Size calculation

6. **Collection Types**

   - Arrays ([T; n])
     - Fixed-size arrays
     - Size as part of type
     - Array indexing and bounds
   - Slices ([T])
     - Dynamic views
     - Dense pointer representation (fat pointer)
     - Slice operations
     - Bounds checking rules

7. **Modal Types** (Cantrip-specific - State Machines)

   - Modal declaration syntax
   - State definitions (@State)
   - State types (T@S) - type-level state tracking
   - State transitions (→ₘ)
   - Transition procedures
   - Static verification (compile-time state checking)
   - Invalid state access prevention
   - State transition rules
   - Modal guarantees

8. **Permission Types**

   - Three permission categories:
     - `own T` - ownership (exactly one)
     - `mut T` - mutable reference (multiple allowed)
     - `imm T` or `T` - immutable reference (unlimited)
   - Uniform parameter passing (all types pass by permission)
   - Copy trait semantics (explicit copying only)
   - Move semantics (move e)
   - Permission rules:
     - Multiple mutable references allowed (key difference from Rust)
     - Owned type constraints
     - Permission inference
   - Permission conversion and casting

9. **Reference and Pointer Types**

   - Safe pointers (Ptr<T>@State)
   - Raw pointers (*T, *mut T)
     - Unsafe operations
     - Null pointer handling
   - Pointer safety guarantees
   - Pointer arithmetic (if allowed)

10. **Abstraction Types**

    - Behavioral Contracts (abstract interfaces)
      - Contract declaration (abstract signatures only, NO implementations)
      - Contract implementation
      - Contract extension (single and multiple)
      - Polymorphism through contracts
      - Contract satisfaction rules (precondition weakening, postcondition strengthening, effect subsumption)
      - Associated types in contracts
      - Coherence and orphan rules
    - Traits (concrete code reuse)
      - Trait declaration (concrete implementations with bodies)
      - Trait attachment
      - Trait vs Contracts distinction (critical - abstract vs concrete)
      - Trait bounds and constraints
      - Associated types
    - Contract vs Trait (explicit distinction)

11. **Parametric Types**

    - Generics syntax (<T, U>)
    - Type parameters and bounds
    - Const generics (<const N: usize>)
    - Generic instantiation (monomorphization)
    - Type inference in generics
    - Generic specialization (if supported)
    - Where clauses
    - Coherence rules

12. **Map Types** (Function/Procedure Types)

    - Function types: `map(T₁, ..., Tₙ) → U ! ε`
    - Procedure types (effectful functions)
    - Effect annotations (! ε)
    - Parameter passing semantics
    - Return types
    - Higher-order functions
    - Effect polymorphism

13. **Type Aliases and Definitions**

    - Type aliases (syntax and semantics)
    - New type names (distinct types vs aliases)
    - Associated types (in contracts and traits)

14. **Type Inference**
    - Local type inference
    - Inference rules
    - Inference limitations
    - Explicit annotations

**Action Items:**

- [ ] Expand scaffold into complete specification
- [ ] Integrate content from OLD_Types.md where appropriate
- [ ] Ensure all type categories comprehensively covered
- [ ] Verify consistency with 01_FrontMatter.md notation

---

## Part III: Expressions (Spec/03)

### 03_Expressions.md (Expand from existing loops section)

**Status:** Contains loops (§1), needs expansion for all expression types

**Structure:**

1. **Expression System Overview**

   - Expression categories (primary, postfix, unary, binary, control flow)
   - Expression vs statement distinction
   - Evaluation order (left-to-right guarantees)
   - Sequence points
   - Expression properties (value categories, effect composition)

2. **Primary Expressions**

   - Literals (reference to §01 lexical structure)
     - Integer, float, boolean, character, string literals
     - Literal type inference
   - Variable references
   - Parenthesized expressions
   - Block expressions (statements as values, return values from blocks)

3. **Operators**

   - Arithmetic operators (+, -, \*, /, %, \*\*)
     - Integer and floating-point semantics
     - Overflow behavior
   - Bitwise operators (&, |, ^, <<, >>)
   - Comparison operators (==, !=, <, <=, >, >=)
   - Logical operators (&&, ||, !)
     - Short-circuit evaluation
   - Assignment operators (=, +=, -=, \*=, /=, %=, &=, |=, ^=, <<=, >>=)
   - Pipeline operator (=>) - left-to-right chaining, method call desugaring
   - Operator precedence table (15 levels)
   - Operator associativity rules

4. **Function Calls and Member Access**

   - Function calls
     - Call syntax
     - Argument evaluation order
     - Return values
   - Method calls
     - Dot notation
     - Receiver types
     - Method resolution
   - Field access
     - Record fields
     - Tuple fields
     - Method chaining (leading dot continuation rule)
   - Array/slice indexing ([index])
     - Bounds checking
     - Safe indexing rules

5. **Conditional Expressions**

   - If expressions
     - Syntax and semantics
     - Type requirements (condition must be bool)
     - Expression vs statement forms
   - If-let pattern matching
     - Pattern matching in conditionals
     - Guard patterns

6. **Pattern Matching**

   - Match expressions
     - Match syntax
     - Exhaustiveness checking (must handle all cases)
   - Pattern types:
     - Literal patterns
     - Variable binding
     - Wildcard (\_)
     - Tuple destructuring
     - Enum patterns (with variant matching)
     - Record patterns
     - Guard patterns (pattern if condition)
     - Or patterns (p₁ | p₂)
   - Pattern matching semantics
     - Order of evaluation
     - Binding scope
     - Pattern exhaustiveness algorithms

7. **Loops** (Status: COMPLETE - from existing §1)

   - Unified loop construct (single `loop` keyword)
     - Infinite loops: `loop { body }`
     - While-style: `loop condition { body }`
     - For-style: `loop pattern in expr { body }`
     - Iterator-style: `loop item in collection { body }`
   - Loop verification:
     - Loop invariants (`with { inv₁, ..., invₙ }`)
     - Loop variants (`by expr`) - termination metric
     - Verification conditions
   - Loop control:
     - `break` with values
     - `continue`
     - Labels ('label: loop ...)
   - Loop typing rules
   - Loop evaluation semantics

8. **Let and Variable Bindings**

   - Let bindings (let x = e)
     - Immutable by default
     - Type inference
     - Pattern bindings (destructuring)
   - Variable bindings (var x = e)
     - Mutable variables
     - Scope and lifetime
     - Pattern bindings

9. **Ownership Transfer**

   - Move semantics (move e)
     - Explicit ownership transfer
     - After-move restrictions
     - Move vs copy

10. **Region Expressions**

    - Region blocks (region r { ... })
    - Region allocation (alloc_in<region>(value))
    - Region lifetime rules
    - Cannot escape regions
    - LIFO deallocation order
    - O(1) bulk deallocation

11. **Comptime Expressions**

    - Compile-time execution (comptime { ... })
    - Comptime context
    - Compile-time vs runtime distinction
    - Compile-time evaluation restrictions

12. **Lambda Expressions** (if supported)
    - Closure syntax
    - Capture semantics (by value vs by reference)
    - Closure types
    - Effect tracking through closures

**Action Items:**

- [ ] Complete all expression types beyond loops
- [ ] Integrate existing loops content
- [ ] Ensure comprehensive coverage
- [ ] Add practical examples for each expression type

---

## Part IV: Contracts and Clauses (Spec/04)

### 04_Contracts.md (Existing - Verify completeness)

**Status:** Substantially complete (~2862 lines), verify all sections

**Structure:**

1. **Contract System Overview** (Status: COMPLETE)

   - Contract components (uses, must, will, where)
   - Contract hierarchy
   - Contract composition
   - Contract checking strategies (static, dynamic, hybrid)
   - Relationship to other features

2. **Behavioral Contracts** (Status: COMPLETE)

   - Contract declaration (abstract signatures only, NO implementations)
   - Contract implementation
   - Contract extension (single and multiple)
   - Contract satisfaction rules
   - Polymorphism through contracts
   - Associated types in contracts
   - Coherence and orphan rules
   - Contract vs Trait distinction (critical)

3. **Effect Clauses** (Status: COMPLETE)

   - Effect declaration (`uses` clause)
   - Standard effects catalog:
     - alloc.\* (heap, region, stack)
     - io.\* (read, write)
     - fs.\* (read, write, delete)
     - net.\* (tcp, udp, http)
     - time.\* (read, sleep)
     - thread.\* (spawn, join)
     - ffi.\* (call)
     - unsafe.\* (ptr)
     - panic
   - Effect composition (transitive through calls)
   - Effect checking (static)
   - Pure procedures (no uses clause = pure)
   - Effect polymorphism
   - Effect lattice laws

4. **Precondition Clauses** (Status: COMPLETE)

   - Precondition syntax (`must`)
   - Precondition semantics
   - Static vs dynamic checking
   - Precondition inference (compiler can infer)
   - Precondition obligations (caller responsibility)
   - Precondition weakening in implementations

5. **Postcondition Clauses** (Status: COMPLETE)

   - Postcondition syntax (`will`)
   - Postcondition semantics
   - Special constructs:
     - `result` - return value reference
     - `@old(expr)` - value at procedure entry
   - Static vs dynamic checking
   - Postcondition guarantees (procedure responsibility)
   - Postcondition strengthening in implementations

6. **Constraint Clauses** (where - Type-level invariants)

   - Constraint syntax (`where`)
   - Type-level invariants
   - Field constraints
   - Constraint checking (at construction and mutation)
   - Always-maintained invariants

7. **Clause Composition**
   - Clause interaction
     - Effects and conditions (orthogonal)
     - Multiple clauses together
   - Inheritance through calls
     - Transitive effects
     - Precondition obligations
     - Postcondition guarantees
   - Contract composition rules

**Action Items:**

- [ ] Verify all sections complete
- [ ] Ensure consistent terminology (uses/must/will, not needs/requires/ensures)
- [ ] Check that all contract features are documented
- [ ] Verify cross-references to other parts

---

## Part V: Statements and Control Flow (Spec/05)

### 05_Statements.md (New file needed)

**Structure:**

1. **Statements Overview**

   - Statement vs expression distinction
   - Statement termination (newline-based with 4 continuation rules)
   - Statement sequencing
   - Statement categories

2. **Expression Statements**

   - Using expressions as statements
   - Side effects
   - Discarding return values

3. **Declaration Statements**

   - Let statements (let x = e)
   - Variable declarations (var x = e)
   - Type aliases in statements (if applicable)

4. **Assignment Statements**

   - Simple assignment (=)
   - Compound assignment (+=, -=, \*=, /=, %=, &=, |=, ^=, <<=, >>=)
   - Assignment rules
   - Assignment effects

5. **Control Flow Statements**

   - If statements
     - Conditional execution
     - Else clauses
     - If-let statements
   - Match statements (reference to §03 pattern matching)
   - Loop statements (reference to §03 loops)
   - Break statements
     - Break with values
     - Labeled break
   - Continue statements
     - Labeled continue
   - Return statements
     - Single and multiple returns (if applicable)
     - Early returns
     - Return type inference

6. **Block Statements**

   - Block syntax ({ ... })
   - Lexical scope
   - Variable visibility
   - Block expressions (return values)
   - Statement sequencing within blocks

7. **Defer Statements** (if applicable)

   - Defer syntax
   - Defer execution order (LIFO)
   - Cleanup patterns

8. **Labeled Statements**
   - Label syntax ('label:)
   - Label scope
   - Break/continue with labels

**Action Items:**

- [ ] Create new file with complete statement specification
- [ ] Ensure alignment with expression rules in §03
- [ ] Document statement termination rules clearly
- [ ] Include comprehensive examples

---

## Part VI: Functions and Procedures (Spec/06)

### 06_Functions.md (New file needed)

**Structure:**

1. **Functions and Procedures Overview**

   - Function vs procedure distinction
     - Functions: pure computations (no side effects, no uses clause)
     - Procedures: effectful computations (may have uses clause)
   - Function types (map types)
   - Procedure types
   - Uniform parameter passing

2. **Function Declarations**

   - Syntax: `function name<Generics>(params): ReturnType { body }`
   - Parameters
   - Return types
   - Generic functions
   - Visibility modifiers (public, private, internal)

3. **Procedure Declarations**

   - Syntax: `procedure name<Generics>(params): ReturnType uses ε must P will Q { body }`
   - Self parameter (self: Type)
   - State transitions (for modals)
   - Contract clauses integration (uses, must, will)
   - Visibility modifiers

4. **Parameters**

   - Parameter syntax
   - Parameter types and permissions (own, mut, imm)
   - Default parameter values (if applicable)
   - Named parameters (if applicable)
   - Variadic parameters (if applicable)
   - Parameter passing semantics (uniform - all by permission)

5. **Return Values**

   - Return syntax (return e)
   - Single returns
   - Multiple returns (if applicable)
   - Named returns (if applicable)
   - Return type inference
   - Implicit returns (last expression in block)

6. **Method Calls**

   - Method resolution
   - Self dispatch
   - Method chaining
   - Receiver types

7. **Function Overloading** (if applicable)

   - Overloading rules
   - Resolution
   - Orphan rules

8. **Anonymous Functions and Closures**

   - Lambda syntax (if supported)
   - Closure capture
   - Closure types
   - Effect tracking in closures

9. **Higher-Order Functions**

   - Functions as values
   - Function composition
   - Effect polymorphism in higher-order functions

10. **Call Semantics**

    - Evaluation order (left-to-right)
    - Stack frame layout
    - Calling conventions
    - Effect propagation

11. **Inline and Optimization**
    - Inline hints ([[inline]], [[no_inline]])
    - Optimization attributes
    - Compiler optimization hints

**Action Items:**

- [ ] Create new file covering all function/procedure features
- [ ] Clearly distinguish functions from procedures
- [ ] Document uniform parameter passing semantics
- [ ] Integrate with contracts (§04) and types (§02)

---

## Part VII: Declarations and Scope (Spec/07)

### 07_Declarations.md (New file needed)

**Structure:**

1. **Declarations Overview**

   - Declaration categories
   - Declaration order
   - Forward declarations (if applicable)

2. **Constant Declarations**

   - Const syntax (const name: Type = value)
   - Compile-time constants
   - Constant expressions
   - Const generics integration

3. **Variable Declarations**

   - Let vs var distinction
     - Let: immutable bindings
     - Var: mutable bindings
   - Type inference
   - Explicit type annotations
   - Initialization rules
   - Definite assignment
   - Default/zero values

4. **Type Declarations**

   - Record declarations
   - Enum declarations
   - Modal declarations
   - Type aliases
   - Forward references

5. **Function/Procedure Declarations** (reference to §06)

6. **Contract Declarations** (reference to §04)

7. **Scope Rules**

   - Block scope
   - Function scope
   - Module scope (file scope)
   - Shadowing rules
   - Name resolution order

8. **Lifetime and Storage**

   - Static storage
   - Automatic storage (stack)
   - Region storage
   - Lifetime rules
   - Lifetime inference (if applicable)

9. **Visibility and Access Control**

   - Public (default or explicit)
   - Internal (module-level)
   - Private (file-level)
   - Visibility modifiers
   - Cross-module access rules

10. **Name Resolution**

    - Identifier lookup
    - Qualified names
    - Resolution order
    - Ambiguity resolution

11. **Predeclared Identifiers**
    - Built-in types (i32, bool, String, etc.)
    - Built-in functions (if any)
    - Built-in constants (if any)
    - Reserved names

**Action Items:**

- [ ] Create new file with complete declaration and scope rules
- [ ] Ensure consistency with visibility rules in modules (§08)
- [ ] Document name resolution algorithm clearly
- [ ] Include examples of scope and shadowing

---

## Part VIII: Modules and Organization (Spec/08)

### 08_Modules.md (New file needed)

**Structure:**

1. **Modules Overview**

   - Module concept
   - File-based module system
   - Module organization
   - One module per file

2. **Module Declaration**

   - Module syntax (implicit from file structure)
   - Module naming
   - File system mapping
   - Module hierarchy

3. **Import System**

   - Import syntax
   - Import paths
   - Import aliases
   - Selective imports
   - Re-exports (if applicable)
   - Circular imports (if allowed)
   - Import resolution

4. **Module Visibility**

   - Public items
   - Internal items (module-level)
   - Private items (file-level)
   - Cross-module access
   - Visibility modifiers

5. **Module Initialization**

   - Initialization order
   - Init functions (if applicable)
   - Static initialization

6. **Package System** (if applicable)

   - Package structure
   - Package dependencies
   - Package management

7. **Program Entry Point**

   - Main function/procedure
   - Entry point rules
   - Command-line arguments (if applicable)

8. **Linking and Code Generation**
   - Separate compilation
   - Linking rules
   - Symbol export/import
   - ABI compatibility

**Action Items:**

- [ ] Create new file covering module system
- [ ] Document file-based module mapping clearly
- [ ] Integrate with visibility rules (§07)
- [ ] Include examples of module organization

---

## Part IX: Memory Model and Permissions (Spec/09)

### 09_Memory.md (New file needed)

**Structure:**

1. **Memory Model Overview**

   - Memory safety guarantees
   - What Cantrip prevents (use-after-free, double-free, memory leaks)
   - What Cantrip does NOT prevent (aliasing bugs, data races)
   - Memory model design goals
   - Differences from Rust's memory model

2. **Lexical Permission System (LPS)**

   - Permission system overview
   - Three permissions: own, mut, imm
   - Permission semantics
   - Permission rules:
     - Multiple mutable references allowed (key difference from Rust)
     - Owned types: exactly one reference
     - Immutable references: unlimited
     - Mutable references: multiple allowed
   - Permission conversion (coercions)
   - Differences from Rust borrow checker

3. **Memory Regions**

   - Region syntax (region r { ... })
   - Region allocation (alloc_in<region>(value))
   - Region deallocation (LIFO order)
   - Region lifetime rules
   - Region escape rules (cannot escape)
   - O(1) bulk deallocation
   - Region vs lifetime parameters (regions not lifetimes)

4. **Stack Allocation**

   - Automatic allocation
   - Stack frame layout
   - Stack limits
   - Local variable storage

5. **Heap Allocation**

   - Explicit heap allocation
   - Heap lifetime management
   - RAII-style cleanup
   - Allocation effects (uses alloc.heap)

6. **Pointer Safety**

   - Safe pointers (Ptr<T>@State)
   - Raw pointers (*T, *mut T)
   - Null safety
   - Dangling pointer prevention
   - Unsafe pointer operations

7. **Copy and Move Semantics**

   - Copy trait (explicit copying only)
   - Move semantics (move e)
   - Ownership transfer
   - Copy vs move rules
   - Copy-capable types

8. **Memory Layout**

   - Type layout rules
   - Alignment rules
   - Padding
   - Record layout
   - Enum layout
   - Array layout
   - Layout attributes (`[[repr(...)]]`)

9. **Memory Safety Properties**

   - Guarantees:
     - Use-after-free prevention (region-based)
     - Double-free prevention (single ownership)
     - Memory leak prevention (RAII, region cleanup)
   - Limitations:
     - Aliasing bugs (programmer responsibility)
     - Data races (programmer responsibility)
     - Iterator invalidation (no borrow checker)

10. **Lifetime Management**
    - Scope-based lifetimes
    - Region-based lifetimes
    - Lifetime inference (if applicable)
    - Lifetime vs region distinction

**Action Items:**

- [ ] Create new file covering memory model comprehensively
- [ ] Emphasize differences from Rust (no borrow checker)
- [ ] Document region-based lifetime model clearly
- [ ] Include safety guarantees and limitations

---

## Part X: Operational Semantics (Spec/10)

### 10_Semantics.md (New file needed)

**Structure:**

1. **Operational Semantics Overview**

   - Evaluation model
   - Small-step vs big-step semantics
   - Evaluation judgments (⟨e, σ⟩ ⇓ ⟨v, σ'⟩)
   - Store representation

2. **Expression Evaluation**

   - Value categories
   - Evaluation order (left-to-right guarantees)
   - Side effects
   - Effect tracking
   - Evaluation rules for each expression type

3. **Statement Execution**

   - Statement semantics
   - Control flow
   - State transitions
   - Statement sequencing

4. **Function Call Semantics**

   - Parameter passing (uniform by permission)
   - Stack frames
   - Return semantics
   - Effect propagation

5. **Memory Operations**

   - Allocation semantics
   - Deallocation semantics
   - Region operations
   - Stack operations
   - Heap operations

6. **Type System Properties**

   - Progress theorem (well-typed expressions don't get stuck)
   - Preservation theorem (types preserved during evaluation)
   - Type safety
   - Soundness guarantees

7. **Contract Semantics**

   - Precondition checking (static and dynamic)
   - Postcondition checking (static and dynamic)
   - Effect checking (static only)
   - Contract violation handling

8. **Runtime Behavior**
   - Undefined behavior
   - Implementation-defined behavior
   - Unspecified behavior
   - Panic semantics

**Action Items:**

- [ ] Create new file with formal operational semantics
- [ ] Use notation from §01 mathematical foundations
- [ ] Include progress and preservation theorems
- [ ] Document all evaluation rules comprehensively

---

## Part XI: Advanced Features (Spec/11)

### 11_Advanced.md (New file needed)

**Structure:**

1. **Modals** (State Machines - already covered in §02, reference)

   - Type-level state tracking
   - State transitions
   - Compile-time verification
   - Modal procedures

2. **Comptime** (Compile-Time Execution)

   - Comptime execution (`comptime { ... }`)
   - Comptime expressions
   - Compile-time constants
   - Compile-time evaluation restrictions
   - Comptime metaprogramming (no macros)
   - Code generation without macros

3. **Generics Advanced**

   - Higher-ranked trait bounds (if applicable)
   - Associated type constraints
   - Generic specialization (if supported)
   - Phantom types
   - Type-level computation

4. **Unsafe Operations**

   - Unsafe blocks
   - Unsafe operations
   - Raw pointers in unsafe
   - FFI interactions
   - Safety guarantees and limitations

5. **Type Inference Advanced**
   - Global type inference (if applicable)
   - Inference limitations
   - Inference algorithms

**Action Items:**

- [ ] Create new file covering advanced features
- [ ] Reference modal types from §02
- [ ] Document comptime thoroughly
- [ ] Explain unsafe operations clearly

---

## Part XII: Error Handling (Spec/12)

### 12_ErrorHandling.md (New file needed)

**Structure:**

1. **Error Handling Overview**

   - Explicit error handling (no ? operator)
   - Result types vs exceptions
   - Error propagation patterns

2. **Result Types**

   - Result<T, E> enum
   - Ok and Err variants
   - Error propagation (explicit pattern matching)
   - Result patterns

3. **Option Types**

   - Option<T> enum
   - Some and None variants
   - None handling (explicit pattern matching)

4. **Pattern Matching Errors**

   - Exhaustive checking
   - Error handling patterns
   - Match expressions for errors

5. **Panic and Termination**
   - Panic semantics
   - Panic behavior
   - Stack unwinding (if any)
   - Normal termination
   - Abnormal termination
   - Panic effects (effect system integration)

**Action Items:**

- [ ] Create new file covering error handling
- [ ] Emphasize explicit error handling (no ? operator)
- [ ] Document Result and Option types thoroughly
- [ ] Include error handling patterns

---

## Part XIII: Concurrency (Spec/13)

### 13_Concurrency.md (New file needed - if applicable)

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
   - Thread effects (uses thread.spawn)

3. **Synchronization**

   - Mutex
   - Locks
   - Condition variables (if applicable)
   - Synchronization effects

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
   - Prevention mechanisms (programmer responsibility)

**Action Items:**

- [ ] Create new file if concurrency features exist
- [ ] Document threading model
- [ ] Integrate with effect system
- [ ] Document data race model

---

## Part XIV: FFI and Interoperability (Spec/14)

### 14_FFI.md (New file needed - if applicable)

**Structure:**

1. **FFI Overview**

   - Foreign function interface
   - C interoperability
   - Safety considerations
   - Zero-cost FFI

2. **FFI Syntax**

   - External function declarations
   - Calling conventions
   - Linkage specifiers
   - Symbol export/import

3. **Type Compatibility**

   - C-compatible types
   - Layout guarantees (`[[repr(C)]]`)
   - Type mappings
   - Struct layout compatibility

4. **FFI Effects**

   - `ffi.call` effect
   - Safety requirements
   - Effect checking

5. **Unsafe Operations**

   - Unsafe blocks for FFI
   - Raw pointer operations
   - FFI safety rules

6. **Interoperability Examples**
   - Common patterns
   - Best practices
   - C library bindings

**Action Items:**

- [ ] Create new file if FFI features exist
- [ ] Document C interoperability thoroughly
- [ ] Integrate with unsafe operations (§11)
- [ ] Include practical examples

---

## Appendices

### appendix_grammar.md (Existing - Verify completeness)

**Status:** Comprehensive EBNF grammar exists

**Content:**

- Complete formal grammar (EBNF)
- Lexical grammar
- Syntactic grammar
- Well-formedness rules
- Grammar production index

**Action Items:**

- [ ] Verify completeness and consistency with specification
- [ ] Cross-reference all syntax forms
- [ ] Ensure grammar matches all specification sections

### appendix_errors.md (Existing - Verify completeness)

**Status:** Error code catalog exists

**Content:**

- Error code catalog
- Error categories:
  - Lexical errors
  - Parsing errors
  - Name resolution errors
  - Type errors
  - Contract errors
  - Memory errors
  - Permission errors
  - Effect errors
  - FFI errors (if applicable)
  - Concurrency errors (if applicable)
- Diagnostic messages
- Error recovery suggestions

**Action Items:**

- [ ] Verify completeness and cross-reference with all specification sections
- [ ] Ensure all error codes are documented
- [ ] Check consistency with error examples in specification

### appendix_precedence.md (New file needed)

**Content:**

- Complete operator precedence table (15 levels)
- Associativity rules
- Operator reference
- Examples of precedence

**Action Items:**

- [ ] Create operator precedence appendix
- [ ] Include all operators
- [ ] Provide clear examples

### appendix_keywords.md (New file needed)

**Content:**

- Complete keyword list (reserved and contextual)
- Reserved words (cannot be used as identifiers)
- Contextual keywords (special meaning in specific contexts)
- Deprecated keywords (if any)
- Future reserved keywords (if any)

**Action Items:**

- [ ] Create keyword reference
- [ ] List all keywords clearly
- [ ] Note deprecated keywords

---

## Supplementary Files

### 00_LLM_Onboarding.md (Existing - Keep as supplementary)

**Status:** Keep as-is, not part of formal spec structure

**Purpose:** Quick reference for LLMs/AI tools working with Cantrip

**Note:** This file remains separate and provides practical quick reference, not normative specification.

---

## Implementation Priority

### Phase 1: Foundation Completion (Highest Priority)

1. Verify and complete 01_FrontMatter.md
2. Expand 02_Types.md from scaffold
3. Complete 03_Expressions.md (expand beyond loops)
4. Verify 04_Contracts.md completeness

### Phase 2: Core Language Features

5. Create 05_Statements.md
6. Create 06_Functions.md
7. Create 07_Declarations.md
8. Create 08_Modules.md

### Phase 3: Memory and Semantics

9. Create 09_Memory.md
10. Create 10_Semantics.md

### Phase 4: Advanced Features

11. Create 11_Advanced.md
12. Create 12_ErrorHandling.md
13. Create 13_Concurrency.md (if applicable)
14. Create 14_FFI.md (if applicable)

### Phase 5: Appendices

15. Verify appendix_grammar.md
16. Verify appendix_errors.md
17. Create appendix_precedence.md
18. Create appendix_keywords.md

---

## Notes

1. **File Naming Convention**: Use `NN_Topic.md` format where NN is two-digit number (01-14)
2. **Cross-References**: Use `§NN.M.M` format for precise citations
3. **Status Tracking**: Mark sections as complete/incomplete for tracking
4. **Style Consistency**: Follow LanguageSpecGuide.md template for each section
5. **Dependencies**: Pay attention to cross-references between chapters

---

**Total Specification Files: 18**

- Main specification: 14 files (01-14)
- Appendices: 4 files
- Supplementary: 1 file (00_LLM_Onboarding.md)
