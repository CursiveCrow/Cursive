# Cantrip Language Specification - Complete Unified Outline

**Version:** 1.0  
**Date:** 2025-01-28  
**Purpose:** Unified outline combining best elements from all three outline proposals  
**Status:** Master reference for specification development

---

## Overview

This outline synthesizes the best organizational elements from three separate outline proposals, evaluated against the existing specification content and the LanguageSpecGuide.md template. It maintains compatibility with existing files while providing a clear roadmap for completing the specification.

### Key Principles

1. **Build on Existing Work**: Preserve and expand existing comprehensive files
2. **Logical Progression**: Organize from foundations through advanced features
3. **Clear Separation**: Separate concerns (e.g., Variables from Expressions, Declarations from Statements)
4. **Comprehensive Coverage**: Ensure all Cantrip features are documented
5. **Consistent Structure**: Follow LanguageSpecGuide.md template pattern

---

## Part I: Foundations

### 00_LLM_Onboarding.md

**Status:** ✅ COMPLETE  
**Purpose:** Quick reference for LLMs and AI tools  
**Action:** Keep as-is, maintain as supplementary reference

### 01_FrontMatter.md

**Status:** ✅ COMPLETE (~2880 lines)  
**Current Contents:**

- Title, Conformance, Abstract
- Notation and Mathematical Foundations (metavariables, judgment forms, operators)
- Lexical Structure (§2): Source files, comments, identifiers, literals, keywords, statement termination
- Abstract Syntax (§3): Type, expression, pattern, value languages; well-formedness
- Attributes (§4): Syntax, core attributes, processing

**Action:** Verify all sections complete, ensure cross-references accurate

---

## Part II: Type System

### 02_Types.md

**Status:** 🚧 SCAFFOLD (needs expansion)  
**Current State:** Chapter structure with dependency tracking, needs full content

**Required Expansion:**

1. **Type Foundations**

   - Type taxonomy overview
   - Type grammar (concrete and abstract)
   - Kind system
   - Typing contexts and judgments (Γ ⊢ e : T)
   - Well-formedness rules
   - Subtyping relation (<:) and type equivalence (≡)
   - Permission-indexed types (own T, mut T, imm T)
   - Effect-annotated map types (map(T₁, ..., Tₙ) → U ! ε)
   - Variance and subtyping relationships

2. **Primitive Types**

   - Integer types (i8, i16, i32, i64, isize, u8, u16, u32, u64, usize)
     - Size, alignment, ranges, overflow behavior
     - Default literal types (42 → i32, etc.)
   - Floating-point types (f32, f64)
     - IEEE 754 conformance, literal syntax
   - Boolean type (bool)
   - Character type (char)
     - Unicode scalar value semantics (32-bit)
   - Never type (!)
     - Bottom type, uninhabited
   - String types (str, String)
     - Slice vs owned, UTF-8 encoding

3. **Product Types**

   - Unit type (())
   - Tuples ((T₁, ..., Tₙ))
     - Field access, memory layout
   - Records (record Name { fields })
     - Nominally typed, field access, methods
     - Memory layout and alignment
     - Field visibility

4. **Sum Types**

   - Enums (enum Name { variants })
     - Tagged unions, variant syntax (unit, tuple, record)
     - Discriminant representation
     - Pattern matching support
     - Memory layout

5. **Modal Types** (Cantrip-specific innovation)

   - Modal declaration syntax
   - State declarations (@State)
   - State transitions (@State₁ → transition() → @State₂)
   - Type-level state tracking (T@S)
   - Static verification rules
   - Invalid state access prevention

6. **Collection Types**

   - Arrays ([T; n])
     - Fixed-size, stack allocation, bounds
   - Slices ([T])
     - Dynamic views, dense pointer representation
     - Bounds checking rules

7. **Reference Types**

   - Safe pointers (Ptr<T>@State)
   - Raw pointers (*T, *mut T)
     - Unsafe operations, null handling
   - Pointer safety guarantees

8. **Abstraction Types**

   - **Behavioral Contracts** (contract Name)
     - Abstract interfaces only (NO implementations)
     - Contract clauses (uses, must, will)
     - Contract implementation and satisfaction
     - Contract extension (single and multiple)
     - Polymorphism through contracts
     - Associated types in contracts
   - **Traits** (trait Name)
     - Concrete code reuse (WITH implementations)
     - Trait attachment
     - Trait bounds
     - Clear distinction from contracts (§04_Contracts.md §2.1)

9. **Parametric Types**

   - Generics syntax (<T, U>)
   - Type parameters and bounds (where clauses)
   - Const generics (<const N: usize>)
   - Generic instantiation and monomorphization
   - Type inference rules
   - Coherence and orphan rules

10. **Map Types** (Function/Procedure Types)

    - Function types (map(T₁, ..., Tₙ) → U ! ε)
    - Procedure types
    - Effect annotations
    - Higher-order functions
    - Closure types

11. **Permission Types**

    - Own (own T)
    - Mutable (mut T)
    - Immutable (T or imm T)
    - Permission rules and conversion
    - Multiple mutable references (allowed, unlike Rust)

12. **Type Aliases and Utilities**
    - Type aliases (type Alias = T)
    - New type names
    - Associated types
    - Copy trait
    - Sized marker
    - Send/Sync markers (if applicable)

**Action:** Expand scaffold with complete normative prose following LanguageSpecGuide.md template

---

## Part III: Expressions

### 03_Expressions.md

**Status:** 🚧 PARTIAL (Loops complete, needs other expression types)  
**Current Contents:**

- Loops (§1): Complete with invariants, variants, all loop forms

**Required Expansion:**

1. **Expression System Overview**

   - Expression categories (primary, postfix, unary, binary, control flow)
   - Evaluation order (left-to-right guarantees)
   - Expression vs statement distinction
   - Value categories
   - Effect composition

2. **Primary Expressions**

   - Literals (reference to §01 lexical structure)
   - Variable references
   - Parenthesized expressions
   - Block expressions ({ statements; result_expr })
   - Constant expressions

3. **Operators**

   - **Arithmetic** (+, -, \*, /, %, \*\*)
     - Integer and floating-point semantics
     - Overflow behavior
   - **Bitwise** (&, |, ^, <<, >>)
   - **Comparison** (==, !=, <, <=, >, >=)
   - **Logical** (&&, ||, !)
     - Short-circuit evaluation
   - **Assignment** (=, +=, -=, \*=, /=, %=, etc.)
   - **Pipeline** (=>)
   - **Range** (.., ..=)
   - Operator precedence table (15 levels)
   - Operator associativity rules

4. **Function Calls and Member Access**

   - Function call syntax
   - Method calls (dot notation)
   - Argument evaluation order
   - Field access (record.field)
   - Tuple field access
   - Array/slice indexing ([index])
   - Safe indexing rules
   - Method chaining

5. **Conditional Expressions**

   - If expressions
   - If-let pattern matching
   - Ternary-like behavior (expression forms)

6. **Pattern Matching** (match)

   - Match expression syntax
   - Pattern exhaustiveness checking
   - Pattern types:
     - Literal patterns
     - Variable binding
     - Wildcard (\_)
     - Tuple destructuring
     - Enum patterns
     - Record patterns
     - Guard patterns (if condition)
   - Pattern matching semantics
   - Binding scope rules

7. **Loops** ✅ (COMPLETE from existing §1)

   - Unified loop construct (loop keyword)
   - Infinite loops
   - Conditional loops (while-style)
   - Pattern-based loops (for-style)
   - Loop invariants (with clause)
   - Loop variants (by clause)
   - Break and continue
   - Loop labels

8. **Bindings**

   - Let bindings (let x = e)
     - Immutable by default
     - Type inference
     - Pattern bindings
   - Variable bindings (var x = e)
     - Mutable variables
     - Scope and lifetime

9. **Ownership Transfer**

   - Move semantics (move e)
   - Explicit ownership transfer
   - After-move restrictions
   - Copy semantics (.copy() method)

10. **Region Expressions**

    - Region blocks (region r { ... })
    - Region allocation (alloc_in<region>(value))
    - Region lifetime rules
    - Cannot escape
    - LIFO deallocation

11. **Comptime Expressions**

    - Compile-time execution (comptime { ... })
    - Comptime blocks
    - Compile-time constants
    - Comptime vs runtime distinction

12. **Lambda Expressions** (if supported)
    - Closure syntax
    - Capture semantics (by value, by reference)
    - Closure types

**Action:** Complete all expression types, integrate existing loops content

---

## Part IV: Contracts and Clauses

### 04_Contracts.md

**Status:** ✅ COMPLETE (~2862 lines)  
**Current Contents:**

- Contract System Overview ✓
- Behavioral Contracts ✓
- Effect Clauses ✓
- Precondition Clauses ✓
- Postcondition Clauses ✓
- (May need: Constraint Clauses (where) and Clause Composition sections)

**Verify Completeness:**

- [ ] Type constraints (where clauses) for type invariants
- [ ] Clause composition rules
- [ ] Contract inheritance through calls
- [ ] Transitive effect composition
- [ ] All terminology consistent (uses/must/will, not requires/ensures/needs)

**Action:** Verify all sections complete, add missing constraint/composition if needed

---

## Part V: Statements and Control Flow

### 05_Statements.md

**Status:** ❌ NEW FILE NEEDED

**Required Content:**

1. **Statements Overview**

   - Statement vs expression distinction
   - Statement termination (newline-based with continuation rules)
   - Statement sequencing
   - Statement categories

2. **Expression Statements**

   - Using expressions as statements
   - Side effects
   - Expression statement semantics

3. **Declaration Statements**

   - Let statements (let x = e)
   - Variable declarations (var x = e)
   - Type annotations in statements

4. **Assignment Statements**

   - Simple assignment (=)
   - Compound assignment (+=, -=, \*=, /=, etc.)
   - Assignment rules
   - Permission requirements

5. **Control Flow Statements**

   - If statements (statement form vs expression)
   - Match statements
   - Loop statements (reference to §03_Expressions.md loops)
   - Break statements
   - Continue statements
   - Return statements
     - Single and multiple returns
     - Early returns
     - Implicit returns (if supported)

6. **Block Statements**

   - Block syntax ({ ... })
   - Scope rules
   - Block expressions
   - Statement sequencing within blocks

7. **Defer Statements** (if applicable)

   - Defer syntax (defer { ... })
   - Defer execution order
   - Cleanup patterns

8. **Labeled Statements**
   - Label syntax ('label:)
   - Label scope
   - Break/continue with labels

**Action:** Create new file with complete statement specification

---

## Part VI: Variables and Declarations

### 06_Declarations.md

**Status:** ❌ NEW FILE NEEDED

**Required Content:**

1. **Declarations Overview**

   - Declaration categories
   - Declaration order rules
   - Top-level vs local declarations

2. **Constant Declarations**

   - Const syntax (const NAME: Type = value)
   - Compile-time constants
   - Constant expressions
   - Const generics integration

3. **Variable Declarations**

   - Let vs var distinction
   - Type inference (:: or omit)
   - Explicit type annotations
   - Initialization rules
   - Definite assignment
   - Default/zero values (if applicable)
   - Shadowing rules

4. **Type Declarations**

   - Record declarations (reference to §02_Types.md)
   - Enum declarations (reference to §02_Types.md)
   - Modal declarations (reference to §02_Types.md)
   - Type aliases (type Alias = T)
   - Contract declarations (reference to §04_Contracts.md)
   - Trait declarations (reference to §02_Types.md)

5. **Function/Procedure Declarations** (reference to §07_Functions.md)

6. **Scope Rules**

   - Block scope
   - Function scope
   - Module scope
   - File scope
   - Shadowing and name resolution
   - Scope nesting rules

7. **Visibility and Access Control**

   - Public (exported)
   - Internal (module-private)
   - Private (default)
   - Visibility modifiers
   - Cross-module access rules

8. **Name Resolution**

   - Identifier lookup rules
   - Qualified names (module::item)
   - Resolution order
   - Shadowing resolution
   - Predeclared identifiers

9. **Storage and Lifetime**
   - Static storage (if applicable)
   - Automatic storage (stack)
   - Region storage
   - Lifetime rules

**Action:** Create new file covering declarations, scope, and visibility comprehensively

---

## Part VII: Functions and Procedures

### 07_Functions.md

**Status:** ❌ NEW FILE NEEDED

**Required Content:**

1. **Functions and Procedures Overview**

   - Function vs procedure distinction
   - Function types (map types from §02_Types.md)
   - Procedure types
   - Pure functions (no effects)

2. **Function Declarations**

   - Function syntax (function name(params): ReturnType { body })
   - Parameters (syntax, types, names)
   - Return types
   - Generic functions (<T, U>)
   - Visibility modifiers
   - Default parameter values (if applicable)
   - Named parameters (if applicable)
   - Variadic parameters (if applicable)

3. **Procedure Declarations**

   - Procedure syntax (procedure name(params): ReturnType uses ε must P will Q { body })
   - Self parameter (self: Type or self: mut Type)
   - State transitions (for modals)
   - Contract clauses integration
   - Side effect declarations

4. **Parameters**

   - Parameter syntax (name: Type, name: mut Type, name: own Type)
   - Permission annotations on parameters
   - Parameter passing semantics (uniform by permission)
   - Argument evaluation order
   - Default values (if applicable)
   - Named arguments (if applicable)
   - Variadic parameters (if applicable)

5. **Return Values**

   - Return syntax (return value or implicit last expression)
   - Multiple return values (if applicable)
   - Named return values (if applicable)
   - Return type inference
   - Early returns

6. **Method Calls**

   - Method resolution
   - Self dispatch (explicit self parameter)
   - Method chaining
   - Associated procedures

7. **Function Overloading** (if supported)

   - Overloading rules
   - Resolution algorithm
   - Orphan rules

8. **Anonymous Functions**

   - Lambda syntax (if supported)
   - Closure capture semantics
   - Closure types
   - Effect capture

9. **Higher-Order Functions**

   - Functions as values
   - Function composition
   - Effect polymorphism
   - Generic function specialization

10. **Call Semantics**

    - Evaluation order
    - Stack frame layout
    - Calling conventions
    - Parameter passing mechanism

11. **Inline and Optimization**
    - Inline hints ([[inline]])
    - Optimization attributes
    - Compiler hints

**Action:** Create new file covering all function/procedure features comprehensively

---

## Part VIII: Modules and Organization

### 08_Modules.md

**Status:** ❌ NEW FILE NEEDED

**Required Content:**

1. **Modules Overview**

   - Module concept
   - File-based module system
   - Module organization principles
   - Relationship to packages

2. **Module Declaration**

   - Module syntax (module name { ... })
   - Module naming conventions
   - File system mapping
   - Module hierarchy

3. **Import System**

   - Import syntax (import module::item)
   - Import paths and resolution
   - Qualified imports
   - Selective imports (import module::{item1, item2})
   - Import aliases
   - Re-exports
   - Circular dependencies (if allowed)

4. **Module Visibility**

   - Public items (exported)
   - Internal items (module-private)
   - Private items (default)
   - Cross-module access rules
   - Visibility modifiers

5. **Module Initialization**

   - Initialization order
   - Init functions (if applicable)
   - Static initialization

6. **Package System** (if applicable)

   - Package structure
   - Package dependencies
   - Package management
   - Package metadata ([[module(...)]] attributes)

7. **Program Entry Point**

   - Main function/procedure
   - Entry point rules
   - Command-line arguments (if applicable)
   - Return value semantics

8. **Linking and Code Generation**
   - Separate compilation model
   - Linking rules
   - Symbol export/import
   - Static vs dynamic linking

**Action:** Create new file covering module system comprehensively

---

## Part IX: Memory Model and Permissions

### 09_Memory.md

**Status:** ❌ NEW FILE NEEDED

**Required Content:**

1. **Memory Model Overview**

   - Memory safety guarantees
   - What Cantrip prevents (use-after-free, double-free, leaks)
   - What Cantrip does NOT prevent (aliasing, data races)
   - Memory model design goals

2. **Lexical Permission System (LPS)**

   - Permission types overview (own, mut, imm)
   - Permission semantics
   - Permission rules
   - Permission conversion (own → mut → imm)
   - Multiple mutable references (allowed, key difference from Rust)
   - Differences from Rust borrow checker

3. **Memory Regions**

   - Region syntax (region r { ... })
   - Region allocation (alloc_in<region>(value))
   - Region deallocation (LIFO order)
   - Region escape rules (cannot escape)
   - Region lifetime
   - O(1) bulk deallocation
   - Performance characteristics

4. **Stack Allocation**

   - Automatic stack allocation
   - Stack frame layout
   - Stack limits
   - Local variable storage

5. **Heap Allocation**

   - Explicit heap allocation
   - Heap lifetime management (RAII-style)
   - Allocation functions
   - Heap allocation effects (uses alloc.heap)

6. **Pointer Safety**

   - Safe pointers (Ptr<T>@State)
   - Raw pointers (*T, *mut T)
   - Null safety (non-null guarantees for safe pointers)
   - Dangling pointer prevention
   - Pointer arithmetic (if allowed)

7. **Copy and Move Semantics**

   - Copy trait
   - Copy method (.copy())
   - Move semantics (move e)
   - Ownership transfer
   - Copy vs move rules
   - Uniform parameter passing (all by permission)

8. **Memory Layout**

   - Type sizes and alignment
   - Record layout (field ordering, padding, alignment)
   - Enum layout (discriminant + payload)
   - Array layout (contiguous storage)
   - Pointer representation
   - Layout attributes ([[repr(C)]], [[repr(packed)]], etc.)

9. **Memory Safety Properties**

   - Guarantees:
     - Use-after-free prevention
     - Double-free prevention
     - Memory leak prevention (region cleanup)
   - Limitations:
     - Aliasing bugs (programmer responsibility)
     - Data races (not prevented by type system)

10. **Lifetime Management**
    - Scope-based lifetimes
    - Region-based lifetimes (explicit, not lifetime parameters)
    - No lifetime parameters ('a, 'b) - use regions instead
    - Lifetime inference (if applicable)

**Action:** Create new file covering memory model comprehensively

---

## Part X: Advanced Features

### 10_Comptime.md

**Status:** ❌ NEW FILE NEEDED

**Required Content:**

1. **Comptime Overview**

   - Compile-time execution model
   - Comptime vs runtime distinction
   - Design philosophy (no macros, use comptime instead)
   - Compile-time code generation

2. **Comptime Expressions**

   - Comptime blocks (comptime { ... })
   - Compile-time evaluation
   - Compile-time constants
   - Constant expressions
   - Compile-time vs runtime contexts

3. **Generic Specialization**

   - Const generics (<const N: usize>)
   - Type specialization
   - Compile-time code generation
   - Monomorphization

4. **Constant Folding**

   - Constant propagation
   - Optimization opportunities
   - Compile-time evaluation guarantees

5. **Compile-Time Reflection** (if applicable)

   - Type information access
   - Reflection capabilities
   - Limitations

6. **Comptime Limitations**
   - Restrictions on comptime code
   - Termination guarantees
   - Computational limits

**Action:** Create new file covering compile-time features

---

## Part XI: Error Handling

### 11_ErrorHandling.md

**Status:** ❌ NEW FILE NEEDED

**Required Content:**

1. **Error Handling Overview**

   - Explicit error handling (no ? operator)
   - Result and Option types
   - Pattern matching for errors

2. **Result Types**

   - Result<T, E> enum
   - Ok and Err variants
   - Error propagation patterns
   - Explicit matching (match result { ... })

3. **Option Types**

   - Option<T> enum
   - Some and None variants
   - None handling
   - Optional value patterns

4. **Pattern Matching Errors**

   - Exhaustive error checking
   - Error handling patterns
   - Error transformation

5. **Panic and Termination**

   - Panic semantics
   - Panic behavior
   - Stack unwinding (if applicable)
   - Normal vs abnormal termination
   - Contract violation panics

6. **Exception Model** (if applicable)
   - Exception syntax (if supported)
   - Throw/catch (if supported)
   - Exception safety
   - Resource cleanup

**Action:** Create new file covering error handling comprehensively

---

## Part XII: Concurrency

### 12_Concurrency.md

**Status:** ❌ NEW FILE NEEDED (if applicable)

**Required Content:**

1. **Concurrency Overview**

   - Concurrency model
   - Threading primitives
   - Safety guarantees
   - Data race model (not prevented by type system)

2. **Threads** (if applicable)

   - Thread creation (thread.spawn)
   - Thread joining (thread.join)
   - Thread lifecycle
   - Thread effects (uses thread.spawn, thread.join)

3. **Synchronization**

   - Mutex types (if applicable)
   - Locks (if applicable)
   - Condition variables (if applicable)
   - Synchronization effects

4. **Atomic Operations** (if applicable)

   - Atomic types (AtomicU32, etc.)
   - Atomic operations
   - Memory ordering
   - Compare-and-swap

5. **Channels** (if applicable)

   - Channel types (Channel<T>)
   - Send/receive operations
   - Channel semantics

6. **Concurrency Effects**

   - Effect system integration
   - Thread effects (thread.spawn, thread.join)
   - Synchronization effects (sync.lock, sync.atomic)

7. **Data Races**

   - Race condition definition
   - Undefined behavior from races
   - Prevention mechanisms (programmer responsibility)

8. **Concurrent Data Structures** (if applicable)
   - Thread-safe collections
   - Shared state patterns

**Action:** Create file if concurrency features are part of language specification

---

## Part XIII: Operational Semantics

### 13_Semantics.md

**Status:** ❌ NEW FILE NEEDED

**Required Content:**

1. **Operational Semantics Overview**

   - Evaluation model
   - Small-step vs big-step semantics
   - Evaluation judgments (⟨e, σ⟩ → ⟨e', σ'⟩, ⟨e, σ⟩ ⇓ ⟨v, σ'⟩)
   - Formal semantics notation

2. **Expression Evaluation**

   - Value categories
   - Evaluation order rules
   - Side effects
   - Effect tracking during evaluation

3. **Statement Execution**

   - Statement semantics
   - Control flow semantics
   - State transitions

4. **Function Call Semantics**

   - Parameter passing semantics
   - Stack frames
   - Return semantics
   - Calling conventions

5. **Memory Operations**

   - Allocation semantics
   - Deallocation semantics
   - Region operations

6. **Type System Properties**

   - Progress theorem (well-typed programs don't get stuck)
   - Preservation theorem (types preserved under reduction)
   - Type safety guarantees

7. **Contract Semantics**

   - Precondition checking semantics
   - Postcondition checking semantics
   - Effect checking semantics
   - @old capture semantics

8. **Runtime Behavior**
   - Undefined behavior
   - Implementation-defined behavior
   - Unspecified behavior
   - Observable behavior

**Action:** Create new file with formal operational semantics

---

## Part XIV: Foreign Function Interface

### 14_FFI.md

**Status:** ❌ NEW FILE NEEDED (if applicable)

**Required Content:**

1. **FFI Overview**

   - Foreign function calls
   - C interoperability
   - Safety considerations
   - FFI effects

2. **FFI Syntax**

   - External function declarations
   - Calling conventions
   - Linkage specifiers
   - Symbol export/import

3. **Type Compatibility**

   - C-compatible types
   - Layout guarantees ([[repr(C)]])
   - Type mappings
   - Pointer compatibility

4. **FFI Effects**

   - `ffi.call` effect (uses ffi.call)
   - Safety requirements
   - Effect tracking through FFI

5. **Unsafe Operations**

   - Unsafe blocks (unsafe { ... })
   - Raw pointer operations in FFI
   - FFI safety rules

6. **Interoperability Examples**
   - Common FFI patterns
   - Best practices
   - C library bindings

**Action:** Create file if FFI features are part of language specification

---

## Appendices

### appendix_grammar.md

**Status:** ✅ COMPLETE  
**Action:** Verify completeness, ensure consistency with all specification sections

### appendix_errors.md

**Status:** ✅ COMPLETE  
**Action:** Verify all error codes referenced in spec match this catalog, identify any missing codes

### appendix_operators.md (NEW)

**Status:** ❌ NEW FILE NEEDED

**Required Content:**

- Complete operator precedence table (15 levels)
- Associativity rules
- Operator descriptions
- Operator overloading rules (if applicable)

**Action:** Create operator reference appendix

### appendix_keywords.md (NEW)

**Status:** ❌ NEW FILE NEEDED

**Required Content:**

- Complete reserved keywords list (54 keywords)
- Contextual keywords
- Deprecated keywords (needs, requires, ensures - replaced by uses, must, will)
- Reserved for future use

**Action:** Create keyword reference appendix

### appendix_naming.md (NEW)

**Status:** ❌ NEW FILE NEEDED

**Required Content:**

- Naming conventions:
  - Type names (CamelCase)
  - Function names (snake_case)
  - Constants (SCREAMING_SNAKE_CASE)
  - Type variables (single uppercase: T, U, V)
- Identifier rules
- Reserved identifier patterns

**Action:** Create naming conventions appendix

---

## Summary and Action Plan

### Files to Verify/Update

1. ✅ **01_FrontMatter.md** - Verify completeness
2. 🚧 **02_Types.md** - Expand scaffold to full specification
3. 🚧 **03_Expressions.md** - Complete all expression types
4. ✅ **04_Contracts.md** - Verify completeness, add constraints/composition if needed

### New Files to Create

5. ❌ **05_Statements.md** - Statements and control flow
6. ❌ **06_Declarations.md** - Declarations, scope, visibility
7. ❌ **07_Functions.md** - Functions and procedures
8. ❌ **08_Modules.md** - Module system and imports
9. ❌ **09_Memory.md** - Memory model and permissions
10. ❌ **10_Comptime.md** - Compile-time features
11. ❌ **11_ErrorHandling.md** - Error handling
12. ❌ **12_Concurrency.md** - Concurrency (if applicable)
13. ❌ **13_Semantics.md** - Operational semantics
14. ❌ **14_FFI.md** - Foreign function interface (if applicable)

### Appendices to Create

15. ❌ **appendix_operators.md** - Operator reference
16. ❌ **appendix_keywords.md** - Keywords reference
17. ❌ **appendix_naming.md** - Naming conventions

### Cross-Reference Verification

- [ ] Ensure all sections cross-reference correctly
- [ ] Verify error codes match specification sections
- [ ] Verify grammar matches all specification sections
- [ ] Check terminology consistency (uses/must/will throughout)
- [ ] Verify no Rustisms in terminology or examples

### Quality Standards

- Professional, clear, concise writing
- Consistent formatting (LanguageSpecGuide.md template)
- Complete examples for all features
- Formal notation where appropriate
- Practical guidance where needed
- Explicit Cantrip semantics (not Rust semantics)

---

## File Organization

**Total Specification Files:** 14 main + 4 appendices + 1 LLM reference = **19 files**

**Organization:**

- **Part I (Foundations)**: 2 files (00_LLM_Onboarding.md, 01_FrontMatter.md)
- **Part II (Type System)**: 1 file (02_Types.md)
- **Part III (Expressions)**: 1 file (03_Expressions.md)
- **Part IV (Contracts)**: 1 file (04_Contracts.md)
- **Part V (Statements)**: 1 file (05_Statements.md)
- **Part VI (Declarations)**: 1 file (06_Declarations.md)
- **Part VII (Functions)**: 1 file (07_Functions.md)
- **Part VIII (Modules)**: 1 file (08_Modules.md)
- **Part IX (Memory)**: 1 file (09_Memory.md)
- **Part X (Advanced)**: 1 file (10_Comptime.md)
- **Part XI (Errors)**: 1 file (11_ErrorHandling.md)
- **Part XII (Concurrency)**: 1 file (12_Concurrency.md, conditional)
- **Part XIII (Semantics)**: 1 file (13_Semantics.md)
- **Part XIV (FFI)**: 1 file (14_FFI.md, conditional)
- **Appendices**: 4 files (grammar, errors, operators, keywords, naming)

Each file should follow the LanguageSpecGuide.md template structure:

- Overview
- Syntax (Concrete and Abstract)
- Static Semantics
- Dynamic Semantics
- Examples and Patterns
- Integration with other features
