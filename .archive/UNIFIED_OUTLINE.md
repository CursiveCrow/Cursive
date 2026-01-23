# Cantrip Language Specification - Unified Outline

**Purpose:** This outline synthesizes the best elements from three previous outline proposals, evaluated against existing specification documents (`00_LLM_Onboarding.md`, `01_FrontMatter.md`, `02_Types.md`, `03_Expressions.md`, `04_Contracts.md`) and the LanguageSpecGuide.md to create a final, high-quality specification structure.

**Status:** This outline guides the complete specification structure for Cantrip 1.3.0.

---

## Part I: Foundations (Complete - Verify)

### 00_LLM_Onboarding.md (Keep as Supplementary Reference)

- **Purpose:** Quick reference for LLMs/AI tools
- **Status:** Separate from formal spec structure
- **Action:** Keep as-is, update as spec evolves

### 01_FrontMatter.md (Status: COMPLETE - Verify Integration)

**Contents (Consolidated from multiple foundation topics):**

1. **Title and Conformance**

   - Abstract and purpose
   - Version information (1.3.0)
   - Key words (MUST, SHALL, etc. - RFC 2119)
   - Conformance definition
   - Document conventions

2. **Notation and Mathematical Foundations** (§1 from existing)

   - Metavariables and conventions
   - Judgment forms (typing, evaluation, contracts)
   - Formal operators (set theory, logic, relations)
   - Inference rule format
   - Substitution notation
   - Reading guide for formal rules
   - Cross-references to notation usage

3. **Lexical Structure** (§2 from existing)

   - Source files (UTF-8, line endings, BOM)
   - Comments (line, block, doc, module)
   - Identifiers
   - Integer literals (decimal, hex, binary, octal)
   - Floating-point literals
   - Boolean literals
   - Character literals and escape sequences
   - String literals
   - Keywords (reserved and contextual)
   - **Statement Termination** (newline-based with 4 continuation rules)
   - Operators and delimiters
   - Whitespace handling

4. **Abstract Syntax** (§3 from existing)

   - Type language
   - Expression language
   - Pattern language
   - Value language
   - Assertion language (contract predicates)
   - Effect language
   - AST representation
   - Relationship to concrete syntax

5. **Attributes and Annotations** (§4 from existing)
   - Attribute syntax and placement
   - Core attributes:
     - `[[repr(...)]]` - memory layout control
     - `[[verify(...)]]` - verification mode
     - `[[overflow_checks(...)]]` - overflow behavior
     - `[[module(...)]]` - module metadata
     - `[[alias(...)]]` - alternative names
     - `[[inline]]`, `[[no_inline]]`, `[[no_mangle]]`
     - `[[deprecated(...)]]`, `[[must_use]]`, `[[cold]]`, `[[hot]]`
   - User-defined attributes (reserved for future)
   - Attribute inheritance rules
   - Conflict detection
   - Processing phases

**Action:** Verify all sections complete and properly integrated

---

## Part II: Type System (Expand from Existing Scaffold)

### 02_Types.md (Status: PARTIAL - Expand to Complete)

**Structure:**

1. **Type System Overview**

   - Type taxonomy (primitives, compounds, abstractions, permissions)
   - Type properties (Copy, Sized, etc.)
   - Subtyping and type equivalence
   - Type identity rules
   - Permission-indexed types integration
   - Effect-annotated map types

2. **Primitive Types**

   - Integer types (i8-i64, u8-u64, isize, usize)
     - Size, alignment, ranges
     - Default literal types
     - Overflow behavior
   - Floating-point types (f32, f64)
     - IEEE 754 conformance
     - Literal syntax
   - Boolean type (bool)
   - Character type (char)
     - Unicode scalar value semantics
   - Never type (!)
     - Bottom type semantics
   - String types
     - `str` (slice, unsized)
     - `String` (owned, heap-allocated)

3. **Compound Types**

   - **Arrays** ([T; n])
     - Fixed-size arrays
     - Size as part of type
     - Memory layout
     - Array indexing and bounds
   - **Slices** ([T])
     - Dynamic views
     - Dense pointer representation
     - Slice operations
     - Bounds checking rules
   - **Tuples** ((T₁, ..., Tₙ))
     - Product types
     - Field access
     - Unit type ()
   - **Records** (record Name { fields })
     - Nominally typed
     - Field access
     - Memory layout and alignment
     - Procedures on records
     - Default values
     - Field visibility
   - **Enums** (enum Name { variants })
     - Sum types (tagged unions)
     - Variant syntax (unit, tuple, record)
     - Pattern matching (exhaustiveness)
     - Discriminant representation
     - Memory layout

4. **Modal Types** (Cantrip-specific)

   - State declarations (@State)
   - State types (T@S)
   - State transitions
   - Type-level state tracking
   - Compile-time verification
   - Invalid state access prevention
   - Modal procedures

5. **Abstraction Types**

   - **Behavioral Contracts** (contract Name)
     - Abstract interfaces
     - NO implementations (signatures + clauses only)
     - Contract satisfaction rules
     - Contract extension
     - Polymorphism through contracts
     - Associated types in contracts
   - **Traits** (trait Name)
     - Concrete code reuse
     - WITH implementations (always have bodies)
     - Trait attachment
     - Trait bounds
     - Associated types
   - **Clear distinction:** Contracts = abstract, Traits = concrete

6. **Parametric Types**

   - **Generics** (<T, U>)
     - Type parameters and bounds
     - Generic declarations
     - Generic instantiation (monomorphization)
     - Type inference
   - **Const Generics** (<const N: usize>)
     - Value-level parameters
     - Constant expressions
   - **Type Aliases**
     - Type aliases
     - New type names
     - Distinct types vs aliases
   - **Where Clauses**
     - Constraint syntax
     - Generic bounds

7. **Function Types (Map Types)**

   - Function types (map(T₁, ..., Tₙ) → U ! ε)
   - Procedure types
   - Effect annotations
   - Parameter passing
   - Return types
   - Higher-order functions
   - Closures and capture semantics
   - Effect polymorphism

8. **Permission Types**

   - **Lexical Permission System (LPS) Overview**
   - Three permissions:
     - `own T` - ownership
     - `mut T` - mutable reference
     - `imm T` or `T` - immutable reference
   - **Uniform Parameter Passing**
     - All types pass by permission
     - Copy trait semantics
     - Move semantics
   - **Permission Rules**
     - Multiple mutable references allowed
     - Owned type constraints
     - Permission conversion
     - Differences from Rust borrow checker

9. **Reference Types**

   - **Safe Pointers** (Ptr<T>@State)
     - State-annotated pointers
     - Safety guarantees
   - **Raw Pointers** (*T, *mut T)
     - Unsafe operations
     - Null pointers
     - Pointer arithmetic (if any)
     - Lifetime constraints

10. **Type Inference**
    - Local type inference
    - Type inference rules
    - Inference limitations
    - Explicit annotations required when inference fails

**Action:** Expand scaffold into complete specification covering all type categories

---

## Part III: Expressions (Complete Beyond Loops)

### 03_Expressions.md (Status: PARTIAL - Has Loops, Needs Rest)

**Structure:**

1. **Expression System Overview**

   - Expression categories
   - Evaluation order (left-to-right guarantees)
   - Expression vs statement
   - Value categories
   - Effect composition

2. **Primary Expressions**

   - Literals (reference to §01 Lexical Structure)
   - Variable references
   - Parenthesized expressions
   - Block expressions (statements as values)

3. **Operators**

   - **Operator Precedence Table** (15 levels)
   - **Unary Operators** (!, -, \*, &, mut)
   - **Binary Operators**
     - Arithmetic (+, -, \*, /, %)
     - Comparison (==, !=, <, <=, >, >=)
     - Logical (&&, ||)
     - Bitwise (&, |, ^, <<, >>)
   - **Assignment Operators** (=, +=, -=, \*=, /=, %=, etc.)
   - **Pipeline Operator** (=>)
   - Operator associativity
   - Overflow behavior
   - Short-circuit evaluation

4. **Function Calls and Member Access**

   - **Function Calls**
     - Call syntax
     - Argument evaluation order
     - Return values
   - **Method Calls**
     - Dot notation
     - Receiver types
   - **Field Access**
     - Record fields
     - Tuple fields
   - **Array/Slice Indexing**
     - Bounds checking
     - Unsafe indexing (if applicable)

5. **Conditional Expressions**

   - **If Expressions**
     - Syntax and semantics
     - Type requirements (bool)
     - Ternary-like behavior
   - **If-Let Pattern Matching**
     - Pattern matching in conditionals

6. **Pattern Matching**

   - **Match Expressions**
     - Match syntax
     - Exhaustiveness checking
     - Match guards
   - **Pattern Types**
     - Literal patterns
     - Variable binding
     - Wildcard
     - Tuple destructuring
     - Enum patterns
     - Record patterns
     - Guard patterns
   - **Pattern Matching Semantics**
     - Order of evaluation
     - Binding scope

7. **Loops** (Status: COMPLETE from existing §1)

   - Unified loop construct (loop keyword)
   - Infinite loops
   - Conditional loops (while-style)
   - For-style loops (pattern in expr)
   - Loop invariants (`with` clause)
   - Loop variants (`by` clause)
   - Break and continue
   - Loop labels
   - Termination verification

8. **Blocks and Sequences**

   - Block expressions
   - Statement sequences
   - Block scope
   - Implicit returns

9. **Let and Variable Bindings**

   - **Let Bindings** (let x = e)
     - Immutable by default
     - Type inference
   - **Variable Bindings** (var x = e)
     - Mutable variables
     - Scope and lifetime
   - **Pattern Bindings**
     - Destructuring in bindings

10. **Ownership Transfer**

    - **Move Semantics** (move e)
    - Explicit ownership transfer
    - After-move restrictions
    - Copy semantics

11. **Region Expressions**

    - Region blocks (region r { ... })
    - Region allocation
    - Region lifetime rules
    - O(1) bulk deallocation

12. **Comptime Expressions**

    - Compile-time execution (comptime { ... })
    - Comptime context
    - Compile-time vs runtime
    - Constant folding

13. **Lambda Expressions** (if supported)
    - Closure syntax
    - Capture semantics
    - Closure types
    - Effect tracking

**Action:** Complete all expression types beyond loops, integrate existing loops content

---

## Part IV: Contracts and Clauses (Status: COMPLETE - Verify)

### 04_Contracts.md (Status: COMPREHENSIVE - Verify Completeness)

**Verify all sections complete:**

1. **Contract System Overview** ✓ (from existing §1)

   - Contract components
   - Contract checking strategies
   - Contract composition
   - Contract hierarchy

2. **Behavioral Contracts** ✓ (from existing §2)

   - Contract declaration
   - Contract implementation
   - Contract extension
   - Contract satisfaction rules
   - Polymorphism through contracts
   - Associated types in contracts
   - Coherence and orphan rules
   - Advanced contract patterns

3. **Effect Clauses** ✓ (from existing §3)

   - Effect syntax (uses clause)
   - Standard effects catalog
   - Effect composition
   - Effect checking
   - Pure procedures
   - Effect polymorphism
   - Effect refinement

4. **Precondition Clauses** ✓ (from existing §4)

   - Must clause syntax
   - Precondition semantics
   - Static vs dynamic checking
   - Precondition inference
   - Frame conditions

5. **Postcondition Clauses** ✓ (from existing §5)

   - Will clause syntax
   - Postcondition semantics
   - @old operator
   - Result reference
   - Static vs dynamic checking

6. **Type Constraints** (where clause) - **Verify exists**

   - Constraint syntax
   - Invariants
   - Type-level invariants
   - Field constraints
   - Constraint checking

7. **Contract Composition** - **Verify exists**
   - Clause interaction
   - Effects and conditions
   - Multiple clauses
   - Inheritance through calls
   - Transitive effects
   - Precondition obligations
   - Postcondition guarantees

**Action:** Verify all sections complete, ensure consistent terminology with rest of spec

---

## Part V: Statements and Control Flow (New File Needed)

### 05_Statements.md (New File)

**Structure:**

1. **Statements Overview**

   - Statement vs expression
   - Statement termination (newline-based)
   - Statement sequencing
   - Statement categories

2. **Expression Statements**

   - Using expressions as statements
   - Side effects

3. **Declaration Statements**

   - Let statements
   - Variable declarations (var)
   - Type aliases in statements
   - Constant declarations (const)

4. **Assignment Statements**

   - Simple assignment (=)
   - Compound assignment (+=, -=, etc.)
   - Assignment rules

5. **Control Flow Statements**

   - **If Statements**
     - Conditional execution
     - Else clauses
     - Chained conditionals
   - **Match Statements**
     - Match syntax
     - Pattern matching
     - Exhaustiveness checking
     - Guards
   - **Loop Statements** (reference to §03 Expressions for loop expressions)
     - Loop statements vs expressions
   - **Break Statements**
     - Break with values
     - Break with labels
   - **Continue Statements**
     - Continue semantics
     - Continue with labels
   - **Return Statements**
     - Single and multiple returns
     - Early returns
     - Return from blocks

6. **Block Statements**

   - Block syntax ({ ... })
   - Lexical scope
   - Variable visibility
   - Block expressions

7. **Defer Statements** (if applicable)

   - Defer syntax
   - Defer execution order
   - Defer with cleanup patterns

8. **Labeled Statements**

   - Label syntax ('label:)
   - Label scope
   - Break/continue with labels

9. **Assert Statements** (if applicable)
   - Assert syntax
   - Assert behavior
   - Debug vs release behavior

**Action:** Create new file with complete statement specification

---

## Part VI: Functions and Procedures (New File Needed)

### 06_Functions.md (New File)

**Structure:**

1. **Functions and Procedures Overview**

   - Function vs procedure distinction
   - Function types (map types - reference to §02)
   - Procedure types
   - Design philosophy (explicit effects)

2. **Function Declarations**

   - Function syntax
   - Generic functions
   - Parameters
   - Return types
   - Visibility modifiers
   - Contract clauses (uses, must, will - reference to §04)

3. **Procedure Declarations**

   - Procedure syntax
   - Self parameter
   - State transitions (for modals - reference to §02)
   - Contract clauses
   - Distinction from functions

4. **Parameters**

   - Parameter syntax
   - Permission annotations (own, mut, imm)
   - Default parameter values (if applicable)
   - Named parameters (if applicable)
   - Variadic parameters (if applicable)
   - Parameter passing semantics (uniform by permission)

5. **Return Values**

   - Return syntax
   - Single return values
   - Multiple return values (if applicable)
   - Named return values (if applicable)
   - Return type inference

6. **Method Calls**

   - Method resolution
   - Self dispatch
   - Method chaining
   - Receiver types

7. **Function Overloading** (if applicable)

   - Overloading rules
   - Resolution
   - Coherence

8. **Anonymous Functions**

   - Lambda syntax (if supported)
   - Closure capture
   - Closure types
   - Effect tracking in closures

9. **Higher-Order Functions**

   - Functions as values
   - Function composition
   - Effect polymorphism

10. **Call Semantics**

    - Evaluation order (left-to-right guarantees)
    - Stack frame layout
    - Calling conventions
    - Effect propagation

11. **Inline and Optimization**
    - Inline hints ([[inline]])
    - Optimization attributes
    - Compiler optimization rules

**Action:** Create new file covering all function/procedure features

---

## Part VII: Declarations and Scope (New File Needed)

### 07_Declarations.md (New File)

**Structure:**

1. **Declarations Overview**

   - Declaration categories
   - Declaration order
   - Declaration context

2. **Constant Declarations**

   - Const syntax
   - Compile-time constants
   - Constant expressions
   - Const generics integration

3. **Variable Declarations**

   - Let vs var
   - Type inference (:: type annotation)
   - Initialization
   - Mutability
   - Default/zero values
   - Definite assignment

4. **Type Declarations**

   - Record declarations (reference to §02)
   - Enum declarations (reference to §02)
   - Modal declarations (reference to §02)
   - Type aliases
   - New type names

5. **Function/Procedure Declarations** (reference to §06)

   - Declaration syntax
   - Forward declarations (if applicable)

6. **Contract Declarations** (reference to §04)

   - Contract declaration syntax
   - Contract implementation declarations

7. **Scope Rules**

   - Block scope
   - File scope
   - Module scope (reference to §08)
   - Shadowing rules
   - Name resolution

8. **Lifetime and Storage**

   - Static storage
   - Automatic storage (stack)
   - Region storage (reference to §09)
   - Lifetime rules
   - RAII-style cleanup

9. **Visibility and Access Control**

   - Public (default or explicit)
   - Internal
   - Private
   - Visibility modifiers
   - Cross-module access

10. **Name Resolution**

    - Identifier lookup
    - Qualified names
    - Resolution order
    - Import resolution (reference to §08)

11. **Predeclared Identifiers**
    - Built-in types
    - Built-in functions
    - Built-in constants
    - Reserved names

**Action:** Create new file with complete declaration and scope rules

---

## Part VIII: Modules and Organization (New File Needed)

### 08_Modules.md (New File)

**Structure:**

1. **Modules Overview**

   - Module concept
   - File-based modules
   - Module organization
   - Design philosophy

2. **Module Declaration**

   - Module syntax
   - Module naming
   - File system mapping
   - Module structure

3. **Import System**

   - Import syntax
   - Import paths
   - Qualified imports
   - Import aliases
   - Selective imports
   - Re-exports
   - Circular dependencies (if allowed)

4. **Module Visibility**

   - Public items
   - Internal items
   - Private items
   - Cross-module access
   - Visibility rules

5. **Module Initialization**

   - Initialization order
   - Init functions (if applicable)
   - Module dependencies

6. **Package System** (if applicable)

   - Package structure
   - Package dependencies
   - Package management
   - Package naming

7. **Program Entry Point**

   - Main function/procedure
   - Entry point rules
   - Program initialization

8. **Linking and Code Generation**
   - Separate compilation
   - Linking rules
   - Symbol export/import
   - ABI compatibility

**Action:** Create new file covering module system comprehensively

---

## Part IX: Memory Model and Permissions (New File Needed)

### 09_Memory.md (New File)

**Structure:**

1. **Memory Model Overview**

   - Memory safety guarantees
   - Memory model design goals
   - What Cantrip prevents
   - What Cantrip does not prevent (aliasing, data races)

2. **Lexical Permission System (LPS)**

   - Permission system overview
   - Permission types (own, mut, imm)
   - Permission rules
   - Permission conversion
   - Multiple mutable references
   - Differences from Rust borrow checker
   - Permission inference

3. **Memory Regions**

   - Region syntax (region r { ... })
   - Region allocation (alloc_in<r>)
   - Region deallocation (LIFO)
   - Region lifetime rules
   - Region escape rules
   - O(1) bulk deallocation
   - Use cases

4. **Allocation Strategies**

   - **Stack Allocation**
     - Automatic allocation
     - Stack frame layout
     - Stack limits
   - **Heap Allocation**
     - Explicit heap allocation
     - Heap lifetime management
     - RAII-style cleanup
   - **Region Allocation**
     - Temporary batches
     - Performance characteristics
     - Allocation effects (alloc.heap, alloc.region, alloc.stack)

5. **Pointer Safety**

   - Safe pointers (Ptr<T>@State)
   - Raw pointers (*T, *mut T)
   - Null safety
   - Dangling pointer prevention
   - Unsafe operations (reference to §12)

6. **Copy and Move Semantics**

   - Copy trait
   - Explicit copying (.copy())
   - Move semantics (move e)
   - Ownership transfer
   - Copy vs move rules
   - Uniform parameter passing (all by permission)

7. **Memory Layout**

   - Type layout
   - Alignment rules
   - Padding
   - Layout attributes (`[[repr(...)]]` - reference to §01)
   - Record layout
   - Enum layout
   - Array layout
   - Pointer representation

8. **Memory Safety Properties**

   - Guarantees (use-after-free, double-free prevention)
   - Limitations (aliasing, data races - programmer responsibility)
   - Memory leak prevention

9. **Lifetime Management**
   - Scope-based lifetimes
   - Region-based lifetimes
   - Lifetime inference (if applicable)
   - Lifetime vs Rust lifetimes (regions, not lifetime parameters)

**Action:** Create new file covering memory model comprehensively

---

## Part X: Advanced Features (New Files Needed)

### 10_Comptime.md (New File)

**Structure:**

1. **Comptime Overview**

   - Compile-time execution
   - Comptime vs runtime
   - Design philosophy (no macros)
   - Metaprogramming without macros

2. **Comptime Expressions**

   - Comptime blocks (comptime { ... })
   - Compile-time evaluation
   - Compile-time constants
   - Constant folding

3. **Generic Specialization**

   - Const generics
   - Type specialization
   - Compile-time code generation
   - Monomorphization

4. **Constant Propagation**

   - Constant folding
   - Optimization
   - Compile-time computation

5. **Comptime Limitations**
   - Restrictions
   - Turing completeness
   - Termination guarantees

**Action:** Create new file covering compile-time features

### 11_ErrorHandling.md (New File)

**Structure:**

1. **Error Handling Overview**

   - Error handling philosophy
   - Explicit error propagation
   - No `?` operator (explicit pattern matching)

2. **Result Types**

   - `Result<T, E>` type
   - Error propagation patterns
   - Match-based error handling
   - Error composition

3. **Option Types**

   - `Option<T>` type
   - None handling
   - Pattern matching on Option

4. **Pattern Matching Errors**

   - Exhaustive checking
   - Error matching patterns
   - Error transformation

5. **Panic and Termination**

   - Panic semantics
   - Panic behavior
   - Stack unwinding (if any)
   - Termination
   - Contract failures

6. **Error Handling Patterns**
   - Common patterns
   - Best practices
   - Error propagation idioms

**Action:** Create new file covering error handling comprehensively

### 12_Unsafe.md (New File)

**Structure:**

1. **Unsafe Operations Overview**

   - Unsafe blocks (unsafe { ... })
   - When to use unsafe
   - Safety guarantees and limitations

2. **Unsafe Blocks**

   - Syntax and semantics
   - Restrictions lifted
   - Safety requirements

3. **Raw Pointers**

   - Usage in unsafe blocks
   - Pointer arithmetic (if allowed)
   - Null pointer handling
   - Dangling pointer risks

4. **FFI Primitives**

   - Foreign function interface (reference to §14)
   - FFI safety rules
   - FFI effects (ffi.call)

5. **Unsafe Patterns**
   - Common unsafe patterns
   - Safety considerations
   - Best practices

**Action:** Create new file covering unsafe operations

---

## Part XI: Operational Semantics (New File Needed)

### 13_Semantics.md (New File)

**Structure:**

1. **Operational Semantics Overview**

   - Evaluation model
   - Small-step vs big-step semantics
   - Evaluation judgments
   - Formal semantics foundation

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
   - Execution rules

4. **Function Call Semantics**

   - Parameter passing
   - Stack frames
   - Return semantics
   - Calling conventions

5. **Memory Operations**

   - Allocation semantics
   - Deallocation semantics
   - Region operations
   - Memory safety invariants

6. **Type System Properties**

   - Progress theorem
   - Preservation theorem
   - Type safety
   - Soundness properties

7. **Contract Semantics**

   - Precondition checking
   - Postcondition checking
   - Effect checking
   - Contract satisfaction

8. **Runtime Behavior**
   - Undefined behavior
   - Implementation-defined behavior
   - Unspecified behavior
   - Defined vs undefined operations

**Action:** Create new file with formal operational semantics

---

## Part XII: Concurrency (New File Needed - If Applicable)

### 14_Concurrency.md (New File - If Applicable)

**Structure (if concurrency features exist):**

1. **Concurrency Overview**

   - Concurrency model
   - Threading primitives
   - Safety guarantees
   - Data races (programmer responsibility)

2. **Threads**

   - Thread creation
   - Thread joining
   - Thread lifecycle
   - Thread effects (thread.spawn, thread.join)

3. **Synchronization**

   - Mutex
   - Locks
   - Condition variables (if applicable)
   - Synchronization effects

4. **Atomic Operations**

   - Atomic types
   - Atomic operations
   - Memory ordering
   - Atomic effects

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

**Action:** Create new file if concurrency features are part of the language

---

## Part XIII: Foreign Function Interface (New File Needed)

### 15_FFI.md (New File)

**Structure:**

1. **FFI Overview**

   - Foreign function calls
   - C interoperability
   - Safety considerations
   - Zero-cost interoperability

2. **FFI Syntax**

   - External function declarations
   - Calling conventions
   - Linkage specifiers
   - FFI effect (ffi.call)

3. **Type Compatibility**

   - C-compatible types
   - Layout guarantees (`[[repr(C)]]` - reference to §01)
   - Type mappings
   - ABI compatibility

4. **Unsafe Operations** (reference to §12)

   - Unsafe blocks for FFI
   - Raw pointer operations
   - FFI safety rules

5. **Interoperability Examples**
   - Common patterns
   - Best practices
   - C library integration

**Action:** Create new file covering FFI features comprehensively

---

## Part XIV: Built-in Functions and Standard Library (New File Needed)

### 16_Builtins.md (New File)

**Structure:**

1. **Built-in Functions Overview**

   - Built-in vs library
   - Compiler integration
   - Predeclared identifiers

2. **Memory Functions**

   - Allocation functions
   - Size functions (if applicable)
   - Alignment functions

3. **Collection Functions**

   - Array operations
   - String operations
   - Slice operations

4. **Type Functions**

   - Type queries
   - Type conversions
   - Type size/alignment queries

5. **System Functions**

   - I/O functions (uses io.read, io.write)
   - System calls
   - Panic and abort

6. **Debug Functions**

   - Assertions
   - Debugging aids

7. **Standard Library Overview**
   - Library organization
   - Core modules
   - Common types (Option, Result, Vec, HashMap)
   - I/O primitives
   - Math operations
   - String operations
   - Reference to library documentation

**Action:** Create new file cataloging built-ins and standard library

---

## Appendices (Complete - Verify)

### appendix_grammar.md (Status: COMPLETE - Verify Consistency)

**Contents:**

- Complete formal grammar (EBNF)
- Lexical grammar
- Syntactic grammar
- Grammar production index
- Syntax summary tables
- Well-formedness rules

**Action:** Verify completeness and consistency with specification sections

### appendix_errors.md (Status: COMPLETE - Verify Cross-References)

**Contents:**

- Error code catalog
- Diagnostic messages
- Error categories:
  - Parsing errors
  - Name resolution errors
  - Type errors
  - Contract errors
  - Memory errors
  - Permission errors
  - Effect errors
  - FFI errors (if applicable)
  - Concurrency errors (if applicable)
- Warning codes
- Implementation-defined ranges
- Error recovery suggestions

**Action:** Verify all error codes are cross-referenced with specification sections, ensure completeness

### appendix_precedence.md (New File Needed)

**Contents:**

- Complete operator precedence table (15 levels)
- Associativity rules
- Operator reference
- Operator descriptions

**Action:** Create operator precedence appendix

### appendix_keywords.md (New File Needed)

**Contents:**

- Complete keyword list (54 reserved keywords)
- Contextual keywords
- Deprecated keywords (needs, requires, ensures → uses, must, will)
- Future reserved keywords

**Action:** Create keyword reference appendix

### appendix_naming.md (New File Needed)

**Contents:**

- Naming conventions
- Identifier rules
- Type naming (CamelCase)
- Function naming (snake_case)
- Constant naming (SCREAMING_SNAKE_CASE)
- Variable naming
- Best practices

**Action:** Create naming conventions appendix

---

## Additional Documentation Files

### ISSUES.md (Keep for Tracking)

- Issue tracking
- Open questions
- Planned features
- Known gaps

### README.md (Update or Create)

**Contents:**

- Specification overview
- How to read this spec
- Specification structure
- Version information (1.3.0)
- Document conventions
- Contributing guidelines
- Cross-reference guide

**Action:** Create comprehensive README for spec

---

## Summary of Actions Required

### Files to Expand/Complete:

1. ✓ `01_FrontMatter.md` - Verify completeness and integration
2. ⚠ `02_Types.md` - Expand scaffold into complete specification
3. ⚠ `03_Expressions.md` - Complete all expression types beyond loops
4. ✓ `04_Contracts.md` - Verify completeness

### Files to Create:

5. ✗ `05_Statements.md` - Control flow and statements
6. ✗ `06_Functions.md` - Functions and procedures
7. ✗ `07_Declarations.md` - Declarations and scope
8. ✗ `08_Modules.md` - Module system
9. ✗ `09_Memory.md` - Memory model and permissions
10. ✗ `10_Comptime.md` - Compile-time features
11. ✗ `11_ErrorHandling.md` - Error handling
12. ✗ `12_Unsafe.md` - Unsafe operations
13. ✗ `13_Semantics.md` - Operational semantics
14. ✗ `14_Concurrency.md` - Concurrency (if applicable)
15. ✗ `15_FFI.md` - Foreign function interface
16. ✗ `16_Builtins.md` - Built-in functions and standard library

### Appendices to Create:

17. ✓ `appendix_grammar.md` - Verify completeness
18. ✓ `appendix_errors.md` - Verify cross-references
19. ✗ `appendix_precedence.md` - Operator precedence
20. ✗ `appendix_keywords.md` - Keywords reference
21. ✗ `appendix_naming.md` - Naming conventions

### Documentation to Create/Update:

22. ✗ `README.md` - Specification overview and guide

### Key Design Principles:

- **Consolidation**: Foundation material (notation, lexical, abstract syntax, attributes) consolidated in `01_FrontMatter.md`
- **Logical Flow**: Foundations → Types → Expressions → Contracts → Statements → Functions → Declarations → Modules → Memory → Advanced
- **Cantrip-Specific Features**: Modals, Contracts vs Traits, Regions, LPS prominently featured
- **Cross-References**: Each section references related sections appropriately
- **Completeness**: All language features covered comprehensively
- **Professional Quality**: Clear, concise, formal where needed, practical where appropriate

---

**This outline provides a complete, well-organized structure for the Cantrip Language Specification version 1.3.0, incorporating the best ideas from all three previous outlines while aligning with existing documentation and industry best practices.**
