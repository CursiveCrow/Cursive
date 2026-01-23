## Clause 1: Frontmatter
### 1.1 Purpose {Source: 1.1}
### 1.2 Scope of the Specification {Source: 1.2}
### 1.3 Abstract Machine Model {Source: Implied by 1.2 & 29.1}
#### 1.3.1 Host vs. Machine
#### 1.3.2 Safety Boundaries
### 1.4 Exclusions {Source: 1.2}
### 1.5 ISO Standards {Source: 5.1}
#### 1.5.1 Character Sets (ISO/IEC 10646)
#### 1.5.2 Floating Point (IEEE 754)
#### 1.5.3 C Language (ISO/IEC 9899)
### 1.6 Platform ABIs {Source: 5.3}
#### 1.6.1 System V ABI
#### 1.6.2 Microsoft x64
### 1.7 Internet Standards {Source: 5.4}
#### 1.7.1 Requirement Keywords (RFC 2119)
#### 1.7.2 UTF-8 (RFC 3629)

---

## Clause 2: Terms and Definitions
### 2.1 Glossary {Source: 4}
#### 2.1.1 General Terms
#### 2.1.2 Conformance Terms
#### 2.1.3 Programming Terms
### 2.2 Notation and Conventions {Source: 3}
#### 2.2.1 Metavariables
#### 2.2.2 Grammar Notation (EBNF)
#### 2.2.3 Inference Rules
### 2.3 Behavior Classifications {Source: 6.1}
#### 2.3.1 Defined Behavior
#### 2.3.2 Implementation-Defined Behavior (IDB)
#### 2.3.3 Unspecified Behavior (USB)
#### 2.3.4 Unverifiable Behavior (UVB) & IFNDR

---

## Clause 3: General Principles and Conformance
### 3.1 Implementation Conformance {Source: 6.2.1}
#### 3.1.1 Conformance Obligations
#### 3.1.2 Strict Mode Requirements
#### 3.1.3 Permissive Mode Requirements
### 3.2 Program Conformance {Source: 6.2.2}
#### 3.2.1 Well-Formed Programs
#### 3.2.2 Ill-Formed Programs
### 3.3 The Conformance Dossier {Source: 6.4}
#### 3.3.1 Generation Requirements
#### 3.3.2 Required Documentation
### 3.4 Implementation Limits {Source: 6.5}
#### 3.4.1 Minimum Limits
#### 3.4.2 Resource Constraints
### 3.5 Language Evolution {Source: 7}
#### 3.5.1 Versioning Model
#### 3.5.2 Feature Stability Classes
#### 3.5.3 Extension Handling
### 3.6 The Attestation System {Source: 6.7}
#### 3.6.1 Attribute Schema
#### 3.6.2 Verification Metadata

---

## Clause 4: Lexical Structure
### 4.1 Character Sets & Encoding {Source: 8.1}
#### 4.1.1 UTF-8 Requirements
#### 4.1.2 BOM Handling
#### 4.1.3 Normalization Forms
### 4.2 Source File Structure {Source: 8.2}
#### 4.2.1 Line Endings
#### 4.2.2 Physical Constraints
### 4.3 Translation Phases {Source: 8.4}
#### 4.3.1 The Two-Phase Model
#### 4.3.2 Forward References
### 4.4 Lexical Elements {Source: 9.1}
#### 4.4.1 Comments (Line & Block)
#### 4.4.2 Whitespace & Formatting
#### 4.4.3 Tokens & Categories
### 4.5 Identifiers {Source: 9.3}
#### 4.5.1 Syntax (UAX31 Rules)
#### 4.5.2 Keywords & Reserved Words
#### 4.5.3 Case Sensitivity & Canonicalization
### 4.6 Literals {Source: 9.4}
#### 4.6.1 Integer Literals
#### 4.6.2 Floating-Point Literals
#### 4.6.3 Character Literals
#### 4.6.4 String Literals
### 4.7 Attributes {Source: 23.5}
#### 4.7.1 Syntax ([[...]])
#### 4.7.2 Arguments & Structure
### 4.8 Syntactic Stability Rules {Source: 3.3}
#### 4.8.1 Context-Insensitive Keywords
#### 4.8.2 Stable Ordering

---

## Clause 5: Program Structure and Names
### 5.1 Compilation Units {Source: 10.1, 8.3}
#### 5.1.1 Module Correspondence
#### 5.1.2 Top-Level Items
#### 5.1.3 Translation Phases {Source: 8.4}
### 5.2 Project Structure {Source: 11}
#### 5.2.1 Projects & Assemblies
#### 5.2.2 Manifest (Cursive.toml)
#### 5.2.3 Module Discovery Paths
### 5.3 Naming and Scopes {Source: 13}
#### 5.3.1 Namespaces
#### 5.3.2 Scope Nesting (Context)
#### 5.3.3 Name Resolution Algorithm
### 5.4 Visibility and Access {Source: 12}
#### 5.4.1 Visibility Modifiers
#### 5.4.2 Intra-Assembly Access
#### 5.4.3 Import Declarations
#### 5.4.4 Item Scoping (use)
### 5.5 Initialization {Source: 14}
#### 5.5.1 Dependency Graph
#### 5.5.2 Eager vs. Lazy Order
#### 5.5.3 Cycle Detection

---

## Clause 6: The Execution Model
### 6.1 The Object Model {Source: 29.1}
#### 6.1.1 Liveness vs. Aliasing
#### 6.1.2 Objects and Storage Duration
#### 6.1.3 Alignment and Layout
### 6.2 The Responsibility Model {Source: 29.2}
#### 6.2.1 Responsible Bindings
#### 6.2.2 Move Semantics & Invalidation
#### 6.2.3 Deterministic Destruction (Drop)
### 6.3 The Partitioning System {Source: 29.3}
#### 6.3.1 Static Record Partitioning
#### 6.3.2 Collection Partitioning
#### 6.3.3 The Partition Proof Verifier
### 6.4 Program Execution
#### 6.4.1 Program Entry (main Signature)
#### 6.4.2 Termination Semantics

---

## Clause 7: The Type System
### 7.1 Foundations {Source: 15}
#### 7.1.1 Classification (Static/Nominal/Structural)
#### 7.1.2 Equivalence & Subtyping
#### 7.1.3 Bidirectional Inference
#### 7.1.4 Layout & ABI Guarantees
#### 7.1.5 Type Aliases (type)
#### 7.1.6 The `Self` Type
### 7.2 Permission Types {Source: 16}
#### 7.2.1 The Permission Lattice
#### 7.2.2 const (Immutable)
#### 7.2.3 unique (Exclusive)
#### 7.2.4 partitioned (Aliased Mutable)
### 7.3 Primitive Types {Source: 17}
#### 7.3.1 Integers (Signed/Unsigned)
#### 7.3.2 Floating Point
#### 7.3.3 Boolean
#### 7.3.4 Character (char)
#### 7.3.5 Unit (()) & Never (!)
### 7.4 Composite Types {Source: 18}
#### 7.4.1 Records (record)
#### 7.4.2 Enums (enum)
#### 7.4.3 Tuples
#### 7.4.4 Arrays & Slices
#### 7.4.5 Unions
### 7.5 Modal Types {Source: 19}
#### 7.5.1 modal Declaration
#### 7.5.2 States (@State) & Payloads
#### 7.5.3 Transitions & Invariants
#### 7.5.4 Modal Pattern Matching
### 7.6 Pointer Types {Source: 21}
#### 7.6.1 Safe Pointers (Ptr<T>)
#### 7.6.2 Raw Pointers (*imm/*mut)
### 7.7 String Types {Source: 20}
#### 7.7.1 The string Modal Type
#### 7.7.2 string@Managed
#### 7.7.3 string@View
### 7.8 Function Types {Source: 22}
#### 7.8.1 Function Type Syntax
#### 7.8.2 Variance & Equivalence
#### 7.8.3 Parameter Responsibility (move)

---

## Clause 8: Traits and Polymorphism
### 8.1 Trait Declarations {Source: 28.1}
#### 8.1.1 Syntax (trait)
#### 8.1.2 Abstract Procedures
#### 8.1.3 Concrete Procedures
#### 8.1.4 The Self Type
### 8.2 Trait Implementation {Source: 28.2}
#### 8.2.1 Implementation Operator (<:)
#### 8.2.2 Requirements & Override
### 8.3 Generics (Static Polymorphism) {Source: 28.3}
#### 8.3.1 Syntax & Constraints
#### 8.3.2 Monomorphization Semantics
#### 8.3.3 Variance Inference
### 8.4 Witnesses (Dynamic Polymorphism) {Source: 28.4}
#### 8.4.1 Syntax (witness)
#### 8.4.2 VTable Layout
#### 8.4.3 Coercion & Safety
### 8.5 Opaque Types (Opaque Polymorphism) {Source: 28.5}
#### 8.5.1 Return Type Syntax (opaque)
#### 8.5.2 Encapsulation Rules

---

## Clause 9: Procedures
### 9.1 Definition {Source: 23.2}
#### 9.1.1 Declaration Syntax (procedure)
#### 9.1.2 Receiver Shorthands (~, ~!)
### 9.2 Parameters {Source: 22.3, 23.2}
#### 9.2.1 Parameter Syntax
#### 9.2.2 Responsibility Modifiers (move)
### 9.3 Special Procedures
#### 9.3.1 Program Entry (main)
#### 9.3.2 Compile-Time (comptime)

---

## Clause 10: Expressions
### 10.1 Fundamentals {Source: 24.1}
#### 10.1.1 Value Categories (Place vs. Value)
#### 10.1.2 Evaluation Order
#### 10.1.3 Precedence Table
### 10.2 Primary Expressions {Source: 24.3}
#### 10.2.1 Literals & Identifiers
#### 10.2.2 Access (., [])
#### 10.2.3 Calls ((), ~>, ::)
### 10.3 Operators {Source: 24.4}
#### 10.3.1 Logical & Comparison
#### 10.3.2 Arithmetic & Bitwise
#### 10.3.3 Address/Deref (&, *)
### 10.4 Special Expressions
#### 10.4.1 Move Expression
#### 10.4.2 Conditional (if)
#### 10.4.3 Matching (match)
#### 10.4.4 Loops (loop)
### 10.5 Structured Expressions
#### 10.5.1 region Blocks
#### 10.5.2 unsafe Blocks
#### 10.5.3 parallel Blocks
#### 10.5.4 comptime Blocks
#### 10.5.5 quote Expressions

---

## Clause 11: Statements and Bindings
### 11.1 Blocks and Scope {Source: 25.1}
#### 11.1.1 Sequencing
#### 11.1.2 Termination
### 11.2 Variable Bindings {Source: 23.1, 25.2}
#### 11.2.1 Syntax (let/var)
#### 11.2.2 Responsibility & Initialization
### 11.3 Assignment {Source: 25.3}
#### 11.3.1 Syntax & Constraints
#### 11.3.2 Drop on Assignment
### 11.4 Expression Statements {Source: 25.4}
### 11.5 Control Flow {Source: 25.6}
#### 11.5.1 Non-Local (return)
#### 11.5.2 Loop Control (break/continue)
#### 11.5.3 Block Result (result)
#### 11.5.4 Deferred Execution (defer)
### 11.6 Partition Statement {Source: 25.7}
#### 11.6.1 Syntax (partition)
#### 11.6.2 Proof Semantics

---

## Clause 12: The Capability System
### 12.1 Capability Principles {Source: 30.1}
### 12.2 The Root Context {Source: 30.2}
### 12.3 Standard Capabilities {Source: 30.3}
### 12.4 Attenuation and Propagation {Source: 30.4}
### 12.5 User-Defined Capabilities {Source: 30.6}

---

## Clause 13: Contracts and Verification
### 13.1 Contract Syntax {Source: 27.1}
### 13.2 Pre/Postconditions {Source: 27.2}
### 13.3 Invariants {Source: 27.4}
### 13.4 Verification {Source: 27.6}

---

## Clause 14: Concurrency
### 14.1 The Concurrency Model {Source: 31.1}
### 14.2 Path 1: Data Parallelism (CREW)
### 14.3 Path 2: Capability Threading

---

## Clause 15: Metaprogramming
### 15.1 Compile-Time Execution {Source: 33.1}
### 15.2 Introspection {Source: 33.2}
### 15.3 Quoting and Splicing {Source: 33.3}
### 15.4 Code Emission {Source: 33.5}
### 15.5 Resource Limits {Source: 33.6}

---

## Clause 16: Interoperability
### 16.1 External Linkage (extern) {Source: 32.1}
### 16.2 FFI-Safe Types {Source: 32.4}
### 16.3 Data Layout {Source: 32.3}
### 16.4 Unsafe Interaction {Source: 32.2}

---

## Clause 17: Core Library
### 17.1 Fundamental Traits {Source: Appx D.1}
### 17.2 System Traits {Source: Appx D.2}
### 17.3 Core Types {Source: Appx E}

---

# Appendices
## Appendix A: Formal Grammar (ANTLR)
## Appendix B: Diagnostic Code Taxonomy
## Appendix C: Conformance Dossier Schema
## Appendix D: Behavior Classification Index
## Appendix E: Formal Core Semantics
## Appendix F: Implementation Guide (Informative)