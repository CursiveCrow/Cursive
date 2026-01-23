# The Cantrip Language Specification

**Version:** 1.0.2
**Date:** October 22, 2025
**Status:** Publication Ready
**Specification Type:** Normative
**Target Audience:** Language implementers, compiler writers, tool developers

---
## Abstract

Cantrip is a systems programming language designed for memory safety, deterministic performance, and AI-assisted development. It achieves these goals through:

- **Lexical Permission System (LPS)**: Compile-time memory safety without garbage collection or borrow checking
- **Explicit Contracts**: Preconditions and postconditions as executable specifications
- **Effect System**: Compile-time tracking of side effects, allocations, and I/O
- **Modal System**: State machines as first-class types with compile-time verification
- **Memory Regions**: Explicit lifetime control with zero-overhead allocation
- **Comptime Metaprogramming**: Compile-time code generation without macros
- **File-Based Modules**: Code organization through file system structure

Cantrip compiles to native code with performance matching C/C++ while providing memory safety guarantees through region-based lifetime management.

**Design Philosophy:**
1. **Explicit over implicit** - All effects, lifetimes, and permissions visible in code
2. **Local reasoning** - Understanding code requires minimal global context
3. **Zero abstraction cost** - Safety guarantees without runtime overhead
4. **LLM-friendly** - Predictable patterns for AI code generation
5. **Simple ownership** - No borrow checker complexity
6. **No macros** - Metaprogramming through comptime only for predictability

**Safety Model:**
- **Prevents**: Use-after-free, double-free, memory leaks
- **Provides**: Deterministic deallocation, zero GC pauses
- **Does NOT prevent**: Aliasing bugs, data races (programmer's responsibility)

**Conformance:** An implementation conforms to this specification if and only if it satisfies all normative requirements stated herein.

### Key Words for Requirement Levels
The key words **MUST**, **MUST NOT**, **REQUIRED**, **SHALL**, **SHALL NOT**, **SHOULD**, **SHOULD NOT**,
**RECOMMENDED**, **MAY**, and **OPTIONAL** in this document are to be interpreted as described in
RFC 2119 and RFC 8174 when, and only when, they appear in all capitals.

### Conformance
An implementation conforms to this specification if and only if it satisfies all normative requirements
stated herein. Extensions MUST NOT invalidate any program that is valid under this specification. Diagnostics,
implementation-defined limits, and unspecified behavior MUST be documented.

### Document Conventions
- Code blocks for the language use the fenced code label `cantrip`.
- Grammar productions use `ebnf` fences.
- Mathematical judgments are typeset with standard notation.
- Error identifiers (e.g., `E9001`) are normative and MUST be reported for corresponding violations.

---

## Table of Contents
- [The Cantrip Language Specification](#the-cantrip-language-specification)
  - [Abstract](#abstract)
    - [Key Words for Requirement Levels](#key-words-for-requirement-levels)
    - [Conformance](#conformance)
    - [Document Conventions](#document-conventions)
  - [Table of Contents](#table-of-contents)
  - [1. Notation and Mathematical Foundations](#1-notation-and-mathematical-foundations)
    - [1.1 Metavariables](#11-metavariables)
    - [1.2 Judgment Forms](#12-judgment-forms)
    - [1.3 Formal Operators](#13-formal-operators)
    - [1.4 Conventions](#14-conventions)
  - [2. Lexical Structure](#2-lexical-structure)
    - [2.1 Source Files](#21-source-files)
    - [2.2 Comments](#22-comments)
    - [2.3 Identifiers](#23-identifiers)
  - [3. Abstract Syntax](#3-abstract-syntax)
    - [3.1 Type Syntax](#31-type-syntax)
    - [3.2 Expression Syntax](#32-expression-syntax)
    - [3.3 Value Syntax](#33-value-syntax)
    - [3.4 Pattern Syntax](#34-pattern-syntax)
    - [3.5 Contract Syntax](#35-contract-syntax)
    - [3.6 Effect Syntax](#36-effect-syntax)
- [Part II: Type System](#part-ii-type-system)
  - [4. Types and Values](#4-types-and-values)
    - [4.1 Primitive Types](#41-primitive-types)
      - [4.1.1 Integer Types](#411-integer-types)
      - [4.1.2 Floating-Point Types](#412-floating-point-types)
      - [4.1.3 Boolean Type](#413-boolean-type)
  - [6. Records and Classes](#6-records-and-classes)
    - [6.1 Record Declaration](#61-record-declaration)
    - [6.6 Structure-of-Arrays](#66-structure-of-arrays)
  - [7. Enums and Pattern Matching](#7-enums-and-pattern-matching)
    - [7.1 Enum Declaration](#71-enum-declaration)
    - [7.4 Discriminant Values](#74-discriminant-values)
  - [8. Traits](#8-traits)
    - [8.1 Trait Declaration](#81-trait-declaration)
    - [8.6 Trait-Declared Effects](#86-trait-declared-effects)
    - [9.5 Type Bounds](#95-type-bounds)
    - [9.6 Const Generics](#96-const-generics)
    - [10.3 Using Modals](#103-using-modals)
    - [10.4 State-Dependent Data](#104-state-dependent-data)
    - [10.5 Complex State Machines](#105-complex-state-machines)
    - [10.6 State Unions](#106-state-unions)
    - [10.7 Common Fields](#107-common-fields)
    - [10.10 Modal Instantiation](#1010-modal-instantiation)
      - [10.9.1 Resource Lifecycle Pattern](#1091-resource-lifecycle-pattern)
      - [10.9.2 Request-Response Pattern](#1092-request-response-pattern)
      - [10.9.3 Multi-Stage Pipeline Pattern](#1093-multi-stage-pipeline-pattern)
      - [10.9.4 State Recovery Pattern](#1094-state-recovery-pattern)
    - [10.10 Modal Instantiation](#1010-modal-instantiation-1)
  - [11. Modal Formal Semantics](#11-modal-formal-semantics)
    - [11.1 Modal Definition](#111-modal-definition)
    - [11.2 State Machine Graph](#112-state-machine-graph)
    - [11.5 Transition Validity](#115-transition-validity)
    - [11.6 Reachability Analysis](#116-reachability-analysis)
  - [12. State Transition Verification](#12-state-transition-verification)
    - [12.1 Static Verification](#121-static-verification)
    - [12.2 Dynamic Verification](#122-dynamic-verification)
    - [12.3 Exhaustiveness Checking](#123-exhaustiveness-checking)
    - [12.4 State Flow Analysis](#124-state-flow-analysis)
- [Part IV: Functions and Expressions](#part-iv-functions-and-expressions)
  - [13. Functions and Procedures](#13-functions-and-procedures)
    - [13.1 Function Definition Syntax](#131-function-definition-syntax)
    - [13.2 Pure Functions](#132-pure-functions)
    - [13.4 Functions with Contracts](#134-functions-with-contracts)
    - [13.5 Parameters and Permissions](#135-parameters-and-permissions)
      - [13.5.1 Immutable Reference (Default)](#1351-immutable-reference-default)
      - [13.5.2 Owned Parameters](#1352-owned-parameters)
      - [13.5.3 Mutable Parameters](#1353-mutable-parameters)
    - [13.6 Return Values](#136-return-values)
    - [13.7 Procedure Syntax](#137-procedure-syntax)
    - [14.9 Operator Precedence](#149-operator-precedence)
    - [15.2 While Loops](#152-while-loops)
    - [15.3 For Loops](#153-for-loops)
    - [15.4 Loop Control](#154-loop-control)
  - [16. Higher-Order Functions and Closures](#16-higher-order-functions-and-closures)
    - [16.1 Function Types (fn) and Procedure Types (proc)](#161-function-types-fn-and-procedure-types-proc)
    - [16.4 Function Traits](#164-function-traits)
- [Part V: Contract System](#part-v-contract-system)
  - [17. Contracts and Specifications](#17-contracts-and-specifications)
    - [17.1 Contract Overview](#171-contract-overview)
    - [17.2 Preconditions](#172-preconditions)
    - [17.3 Postconditions](#173-postconditions)
    - [17.4 Contract Messages](#174-contract-messages)
    - [17.5 Empty Contracts](#175-empty-contracts)
    - [17.6 Old-Value References](#176-old-value-references)
    - [17.9 Common Contract Patterns](#179-common-contract-patterns)
      - [17.9.1 Bounds Checking Pattern](#1791-bounds-checking-pattern)
      - [17.9.2 Collection Invariant Pattern](#1792-collection-invariant-pattern)
      - [17.9.3 State Consistency Pattern](#1793-state-consistency-pattern)
      - [17.9.4 Resource Conservation Pattern](#1794-resource-conservation-pattern)
      - [17.9.5 Ordering Preservation Pattern](#1795-ordering-preservation-pattern)
      - [17.9.6 Error Handling Contract Pattern](#1796-error-handling-contract-pattern)
  - [18. Contract Formal Logic](#18-contract-formal-logic)
    - [18.1 Assertion Language](#181-assertion-language)
    - [18.2 Hoare Logic](#182-hoare-logic)
    - [18.3 Weakest Precondition](#183-weakest-precondition)
    - [18.5 Contract Soundness](#185-contract-soundness)
  - [19. Invariants](#19-invariants)
    - [19.1 Record Invariants](#191-record-invariants)
    - [19.3 Class Invariants](#193-class-invariants)
    - [19.4 Modal State Invariants](#194-modal-state-invariants)
    - [19.5 Loop Invariants](#195-loop-invariants)
  - [20. Verification and Testing](#20-verification-and-testing)
    - [20.1 Verification Modes](#201-verification-modes)
    - [20.2 Static Verification](#202-static-verification)
    - [20.3 Runtime Verification](#203-runtime-verification)
    - [20.4 Contract Fuzzing](#204-contract-fuzzing)
    - [20.5 Fuzzing Configuration](#205-fuzzing-configuration)
    - [20.6 Integration with Testing](#206-integration-with-testing)
- [Part VI: Effect System](#part-vi-effect-system)
  - [21. Effects and Side Effects](#21-effects-and-side-effects)
    - [21.1 Effect System Overview](#211-effect-system-overview)
    - [21.2 Effect Syntax](#212-effect-syntax)
    - [21.2.7 User-Defined Effects](#2127-user-defined-effects)
    - [21.3 Memory Effects](#213-memory-effects)
    - [21.7 Standard Effect Definitions](#217-standard-effect-definitions)
      - [21.7.1 Importing Standard Effects](#2171-importing-standard-effects)
      - [21.7.2 Core Standard Effects](#2172-core-standard-effects)
      - [21.7.3 Effect Pattern Examples](#2173-effect-pattern-examples)
      - [21.7.4 Custom Effect Composition](#2174-custom-effect-composition)
      - [21.7.5 Effect Documentation Pattern](#2175-effect-documentation-pattern)
    - [21.9 Rendering Effects](#219-rendering-effects)
      - [21.11.2 File System Effects (`fs.*`)](#21112-file-system-effects-fs)
      - [21.11.3 Network Effects (`net.*`)](#21113-network-effects-net)
      - [21.11.4 I/O Effects (`io.*`)](#21114-io-effects-io)
      - [21.11.5 Time Effects (`time.*`)](#21115-time-effects-time)
      - [21.11.6 Threading Effects (`thread.*`)](#21116-threading-effects-thread)
      - [21.11.7 Rendering Effects (`render.*`)](#21117-rendering-effects-render)
      - [21.11.8 Audio Effects (`audio.*`)](#21118-audio-effects-audio)
      - [21.11.9 Input Effects (`input.*`)](#21119-input-effects-input)
      - [21.11.10 Process Effects (`process.*`)](#211110-process-effects-process)
      - [21.11.11 FFI Effects (`ffi.*`)](#211111-ffi-effects-ffi)
      - [21.11.12 Unsafe Effects (`unsafe.*`)](#211112-unsafe-effects-unsafe)
      - [21.11.13 System Effects](#211113-system-effects)
      - [21.11.14 Complete Effect Hierarchy](#211114-complete-effect-hierarchy)
      - [21.11.15 Standard Effect Definitions](#211115-standard-effect-definitions)
  - [22. Effect Rules and Checking](#22-effect-rules-and-checking)
    - [22.7 Async Effect Masks](#227-async-effect-masks)
    - [22.3.1 Named Effects in Declarations](#2231-named-effects-in-declarations)
    - [22.4 Forbidden Effects](#224-forbidden-effects)
      - [22.4.1 Valid Use Case 1: Wildcard Restriction](#2241-valid-use-case-1-wildcard-restriction)
      - [22.4.2 Valid Use Case 2: Polymorphic Effect Constraint](#2242-valid-use-case-2-polymorphic-effect-constraint)
      - [22.4.3 INVALID: Redundant Forbidden Effects](#2243-invalid-redundant-forbidden-effects)
    - [22.5 Effect Wildcards](#225-effect-wildcards)
    - [22.6 Higher-Order Functions](#226-higher-order-functions)
    - [Async Effect Masking (await)](#async-effect-masking-await)
  - [23. Effect Budgets](#23-effect-budgets)
    - [23.1 Budget Overview](#231-budget-overview)
    - [23.2 Static Budgets](#232-static-budgets)
    - [23.3 Dynamic Budgets](#233-dynamic-budgets)
    - [23.4 Time Budgets](#234-time-budgets)
    - [23.5 Count Budgets](#235-count-budgets)
    - [23.6 Budget Composition](#236-budget-composition)
- [Part VII: Memory Management](#part-vii-memory-management)
  - [25. Lexical Permission System](#25-lexical-permission-system)
    - [25.1 LPS Overview](#251-lps-overview)
    - [25.2 Design Goals](#252-design-goals)
    - [25.3 Memory Safety Model](#253-memory-safety-model)
    - [25.4 Compilation Model](#254-compilation-model)
  - [26. Permission Types and Rules](#26-permission-types-and-rules)
    - [26.1 Permission Overview](#261-permission-overview)
    - [26.2 Immutable Reference (Default)](#262-immutable-reference-default)
    - [26.3 Owned Permission](#263-owned-permission)
    - [26.5 Isolated Permission](#265-isolated-permission)
    - [27.2 Passing by Reference](#272-passing-by-reference)
    - [27.3 Permission Transitions](#273-permission-transitions)
    - [27.8 The Cantrip Safety Model: Trade-offs and Guarantees](#278-the-cantrip-safety-model-trade-offs-and-guarantees)
      - [27.8.1 What Cantrip Guarantees (✅ Compile-Time Safe)](#2781-what-cantrip-guarantees--compile-time-safe)
      - [27.8.2 What Cantrip Does NOT Guarantee (⚠️ Programmer Responsibility)](#2782-what-cantrip-does-not-guarantee-️-programmer-responsibility)
  - [28. Memory Regions](#28-memory-regions)
    - [28.1 Region Overview](#281-region-overview)
    - [28.2 Three Allocation Strategies Compared](#282-three-allocation-strategies-compared)
    - [28.3 Region Declaration](#283-region-declaration)
    - [28.5 Region Allocation Syntax](#285-region-allocation-syntax)
    - [28.6 Implementing Region Allocation](#286-implementing-region-allocation)
    - [28.7 Region vs Heap Decision Guide](#287-region-vs-heap-decision-guide)
    - [28.8 Performance Characteristics](#288-performance-characteristics)
    - [28.9 Common Patterns](#289-common-patterns)
    - [28.10 Safety Restrictions](#2810-safety-restrictions)
  - [29. Region Formal Semantics](#29-region-formal-semantics)
    - [29.1 Region Algebra](#291-region-algebra)
    - [29.2 Allocation Rules](#292-allocation-rules)
    - [29.3 Deallocation Rules](#293-deallocation-rules)
    - [29.4 Escape Analysis](#294-escape-analysis)
    - [29.5 Region Typing](#295-region-typing)
    - [29.6 Memory Model](#296-memory-model)
    - [29.7 Happens-Before Relation](#297-happens-before-relation)
- [Part VIII: Module System](#part-viii-module-system)
  - [30. Modules and Code Organization](#30-modules-and-code-organization)
    - [30.1 File-Based Module System](#301-file-based-module-system)
    - [30.2 Module Definition](#302-module-definition)
    - [30.3 Module Metadata](#303-module-metadata)
    - [30.4 Module Resolution Algorithm](#304-module-resolution-algorithm)
    - [30.6 Modules vs Regions](#306-modules-vs-regions)
  - [31. Import and Export Rules](#31-import-and-export-rules)
    - [31.1 Import Syntax](#311-import-syntax)
    - [31.2 Import Resolution](#312-import-resolution)
    - [31.3 Re-exports](#313-re-exports)
    - [31.6 Import Ordering](#316-import-ordering)
  - [32. Visibility and Access Control](#32-visibility-and-access-control)
    - [32.1 Visibility Modifiers](#321-visibility-modifiers)
    - [32.2 Public Items](#322-public-items)
    - [32.3 Internal Items](#323-internal-items)
    - [32.4 Private Items](#324-private-items)
    - [32.5 Record Field Visibility](#325-record-field-visibility)
    - [32.6 Procedure Visibility](#326-procedure-visibility)
    - [32.7 Trait Implementation Visibility](#327-trait-implementation-visibility)
  - [33. Module Resolution](#33-module-resolution)
    - [33.1 Resolution Rules](#331-resolution-rules)
    - [33.2 Search Path](#332-search-path)
    - [33.4 Package Configuration](#334-package-configuration)
    - [33.5 Module Cache](#335-module-cache)
- [Part IX: Advanced Features](#part-ix-advanced-features)
  - [34. Compile-Time Programming](#34-compile-time-programming)
    - [34.6 Opt‑In Reflection](#346-optin-reflection)
    - [34.1 Comptime Keyword](#341-comptime-keyword)
    - [34.3 Type Introspection](#343-type-introspection)
    - [34.5 Comptime Type Introspection](#345-comptime-type-introspection)
    - [34.6 Comptime Code Generation](#346-comptime-code-generation)
    - [34.7 Comptime Validation](#347-comptime-validation)
  - [35. Concurrency](#35-concurrency)
    - [35.5 Structured Concurrency](#355-structured-concurrency)
    - [35.2 Message Passing](#352-message-passing)
    - [35.3 Send and Sync Traits](#353-send-and-sync-traits)
    - [35.4 Atomic Operations](#354-atomic-operations)
  - [36. Actors (First-Class Type)](#36-actors-first-class-type)
    - [36.1 Actor Pattern Overview](#361-actor-pattern-overview)
    - [36.2 Basic Actor Pattern Implementation](#362-basic-actor-pattern-implementation)
    - [36.4 Standard Library Support](#364-standard-library-support)
  - [37. Async/Await](#37-asyncawait)
    - [37.4 Async Iteration](#374-async-iteration)
    - [37.1 Async Functions](#371-async-functions)
    - [37.2 Await Expressions](#372-await-expressions)
    - [37.3 Select Expression](#373-select-expression)
  - [55. Machine‑Readable Output](#55-machinereadable-output)
- [Part XIV: Foreign Function Interface (FFI)](#part-xiv-foreign-function-interface-ffi)
  - [56. Foreign Function Interface Overview](#56-foreign-function-interface-overview)
  - [57. Declarations and Linkage](#57-declarations-and-linkage)
  - [58. Type Mappings](#58-type-mappings)
  - [59. Ownership and Allocation Across FFI](#59-ownership-and-allocation-across-ffi)
  - [60. Callbacks from C into Cantrip](#60-callbacks-from-c-into-cantrip)
  - [61. Errors and Panics](#61-errors-and-panics)
  - [62. Inline Assembly (Reserved)](#62-inline-assembly-reserved)
- [Part X: Operational Semantics](#part-x-operational-semantics)
  - [38. Small-Step Semantics](#38-small-step-semantics)
    - [38.1 Small-Step Reduction](#381-small-step-reduction)
    - [38.2 Evaluation Contexts](#382-evaluation-contexts)
  - [39. Big-Step Semantics](#39-big-step-semantics)
    - [39.1 Big-Step Evaluation](#391-big-step-evaluation)
  - [40. Memory Model](#40-memory-model)
    - [40.1 Memory State](#401-memory-state)
    - [40.2 Happens-Before](#402-happens-before)
  - [41. Evaluation Order](#41-evaluation-order)
    - [41.1 Left-to-Right Evaluation](#411-left-to-right-evaluation)
    - [41.2 Short-Circuit Evaluation](#412-short-circuit-evaluation)
- [Part XI: Soundness and Properties](#part-xi-soundness-and-properties)
  - [42. Type Soundness](#42-type-soundness)
  - [43. Effect Soundness](#43-effect-soundness)
  - [44. Memory Safety](#44-memory-safety)
  - [45. Modal Safety](#45-modal-safety)
- [Part XII: Standard Library](#part-xii-standard-library)
  - [46. Core Types and Operations](#46-core-types-and-operations)
    - [46.1 Core Type System](#461-core-type-system)
    - [46.2 Option Type](#462-option-type)
    - [46.3 Result Type](#463-result-type)
    - [46.4 String Type](#464-string-type)
  - [47. Collections](#47-collections)
    - [47.1 Vec](#471-vec)
    - [47.2 HashMap\<K, V\>](#472-hashmapk-v)
    - [47.3 HashSet](#473-hashset)
  - [48. I/O and File System](#48-io-and-file-system)
    - [48.1 File I/O](#481-file-io)
    - [48.2 Standard Streams](#482-standard-streams)
    - [48.3 File System Operations](#483-file-system-operations)
  - [49. Networking](#49-networking)
    - [49.1 HTTP Client](#491-http-client)
    - [49.2 TCP Sockets](#492-tcp-sockets)
  - [50. Concurrency Primitives](#50-concurrency-primitives)
      - [50.1 Structured Concurrency Helpers](#501-structured-concurrency-helpers)
    - [50.1 Mutex](#501-mutex)
    - [50.2 Channels](#502-channels)
    - [50.3 Atomic Types](#503-atomic-types)
- [Part XIII: Tooling and Implementation](#part-xiii-tooling-and-implementation)
  - [51. Compiler Architecture](#51-compiler-architecture)
    - [51.1 Compilation Pipeline](#511-compilation-pipeline)
    - [51.2 Compiler Invocation](#512-compiler-invocation)
    - [51.3 Optimization Levels](#513-optimization-levels)
    - [51.4 Verification Modes](#514-verification-modes)
    - [51.5 Incremental Compilation](#515-incremental-compilation)
  - [52. Error Reporting](#52-error-reporting)
    - [52.1 Error Message Format](#521-error-message-format)
    - [52.2 Example Error Messages](#522-example-error-messages)
    - [52.3 Machine-Readable Output](#523-machine-readable-output)
  - [53. Package Management](#53-package-management)
    - [53.1 Project Structure](#531-project-structure)
    - [53.2 Package Manifest](#532-package-manifest)
    - [53.3 Commands](#533-commands)
    - [53.4 Dependency Resolution](#534-dependency-resolution)
  - [54. Testing Framework](#54-testing-framework)
    - [54.1 Unit Tests](#541-unit-tests)
    - [54.2 Integration Tests](#542-integration-tests)
    - [54.3 Property-Based Testing](#543-property-based-testing)
    - [54.4 Benchmarks](#544-benchmarks)
  - [56. Overview](#56-overview)
  - [57. Declarations and Linkage](#57-declarations-and-linkage-1)
  - [58. Type Mappings](#58-type-mappings-1)
  - [59. Ownership and Allocation Across FFI](#59-ownership-and-allocation-across-ffi-1)
  - [60. Callbacks from C into Cantrip](#60-callbacks-from-c-into-cantrip-1)
  - [61. Errors and Panics](#61-errors-and-panics-1)
  - [62. Inline Assembly (Reserved)](#62-inline-assembly-reserved-1)
- [Part XII: Standard Library](#part-xii-standard-library-1)
  - [46. Core Types and Operations {#46-core-types-and-operations}](#46-core-types-and-operations-46-core-types-and-operations)
    - [46.1 Option](#461-option)
    - [46.2 Result](#462-result)
    - [46.3 String and str](#463-string-and-str)
    - [46.4 Vec](#464-vec)
    - [46.5 HashMap / HashSet](#465-hashmap--hashset)
    - [46.6 Core Functions (Normative)](#466-core-functions-normative)
  - [47. Collections {#47-collections}](#47-collections-47-collections)
  - [48. I/O and File System {#48-io-and-file-system}](#48-io-and-file-system-48-io-and-file-system)
  - [49. Networking {#49-networking}](#49-networking-49-networking)
  - [50. Concurrency Primitives {#50-concurrency-primitives}](#50-concurrency-primitives-50-concurrency-primitives)
- [Part XIII: Tooling and Implementation](#part-xiii-tooling-and-implementation-1)
  - [51. Compiler Architecture {#51-compiler-architecture}](#51-compiler-architecture-51-compiler-architecture)
  - [52. Error Reporting {#52-error-reporting}](#52-error-reporting-52-error-reporting)
  - [53. Package Management {#53-package-management}](#53-package-management-53-package-management)
  - [54. Testing Framework {#54-testing-framework}](#54-testing-framework-54-testing-framework)
  - [55. Machine‑Readable Output {#55-machine-readable-output}](#55-machinereadable-output-55-machine-readable-output)
- [Part XIV: Foreign Function Interface](#part-xiv-foreign-function-interface)
  - [56. Overview {#56-overview}](#56-overview-56-overview)
  - [57. Declarations and Linkage {#57-declarations-and-linkage}](#57-declarations-and-linkage-57-declarations-and-linkage)
  - [58. Type Mappings {#58-type-mappings}](#58-type-mappings-58-type-mappings)
  - [59. Ownership and Allocation Across FFI {#59-ownership-and-allocation-across-ffi}](#59-ownership-and-allocation-across-ffi-59-ownership-and-allocation-across-ffi)
  - [60. Callbacks from C into Cantrip {#60-callbacks-from-c-into-cantrip}](#60-callbacks-from-c-into-cantrip-60-callbacks-from-c-into-cantrip)
  - [61. Errors and Panics {#61-errors-and-panics}](#61-errors-and-panics-61-errors-and-panics)
  - [62. Inline Assembly (Reserved) {#62-inline-assembly-reserved}](#62-inline-assembly-reserved-62-inline-assembly-reserved)
  
---

## 1. Notation and Mathematical Foundations

### 1.1 Metavariables

Throughout this specification, we use the following metavariable conventions:

```
x, y, z ∈ Var          (variables)
f, g, h ∈ FunName      (function names)
T, U, V ∈ Type         (types)
@S, @S' ∈ State        (modal states)
e ∈ Expr               (expressions)
v ∈ Value              (values)
ε ∈ Effect             (effects)
P, Q ∈ Assertion       (contract assertions)
σ ∈ Store              (memory stores)
Γ ∈ Context            (type contexts)
Δ ∈ EffContext         (effect contexts)
Σ ∈ StateContext       (state contexts)
```

### 1.2 Judgment Forms

**Type judgments:**
```
Γ ⊢ e : T                         (expression e has type T in context Γ)
Γ ⊢ e : T@S                       (expression e has type T in state @S)
Γ ⊢ e : T@S ! ε                   (type T, state @S, effect ε)
```

**Effect judgments:**
```
Γ ⊢ e ! ε                         (expression e has effect ε)
effects(e) = ε                    (computed effect of e)
```

**Contract judgments:**
```
{P} e {Q}                         (Hoare triple)
σ ⊨ P                             (store satisfies assertion)
needs(f) = ε                      (function f needs effects ε)
requires(f) = P                   (function f requires precondition P)
ensures(f) = Q                    (function f ensures postcondition Q)
```

**Operational semantics:**
```
⟨e, σ, ε⟩ → ⟨e', σ', ε'⟩          (small-step reduction)
⟨e, σ⟩ ⇓ ⟨v, σ'⟩                   (big-step evaluation)
```

**State transitions:**
```
@S₁ →ₘ @S₂                        (state transition via procedure m)
Σ ⊢ T@S₁ →ₘ T@S₂                  (valid state transition)
```

### 1.3 Formal Operators

**Set theory:**
```
∈       (element of)
⊆       (subset)
∪       (union)
∩       (intersection)
∅       (empty set)
×       (cartesian product)
```

**Logic:**
```
∧       (conjunction)
∨       (disjunction)
¬       (negation)
⇒       (implication)
∀       (universal quantification)
∃       (existential quantification)
```

**Relations:**
```
→       (maps to / reduces to)
⇒       (implies / pipeline)
≡       (equivalence)
⊢       (entails / proves)
⊨       (satisfies / models)
```

### 1.4 Conventions

**Naming conventions:**
- Type names: `CamelCase` (e.g., `Point`, `Vec`)
- Functions: `snake_case` (e.g., `read_file`, `compute_sum`)
- Constants: `SCREAMING_SNAKE` (e.g., `MAX_SIZE`, `DEFAULT_PORT`)
- Type variables: Single uppercase letters (e.g., `T`, `U`, `V`)

**Syntactic sugar notation:**
- `[T]` - Array/slice of T
- `T?` - `Option<T>`
- `T!` - `Result<T, Error>`

---

## 2. Lexical Structure

### 2.1 Source Files

**Definition 2.1 (Source File):** An Cantrip source file is a UTF-8 encoded text file with extension `.cantrip`.

**Formal properties:**
- Encoding: UTF-8
- Line endings: LF (`\n`) or CRLF (`\r\n`)
- BOM: Optional (ignored if present)
- Maximum file size: Implementation-defined (recommended: 1MB)

### 2.2 Comments

**Syntax:**
```cantrip
// Line comment

/* Block comment
   spanning multiple lines */

/// Documentation comment for following item
function example() { ... }

//! Module-level documentation
```

**Formal definition:**
```ebnf
LineComment ::= "//" ~[\n\r]* [\n\r]
BlockComment ::= "/*" (~"*/" any)* "*/"
DocComment ::= "///" ~[\n\r]*
ModuleDoc ::= "//!" ~[\n\r]*
```

**Semantic rules:**
- Comments do not affect program semantics
- Comments are stripped before parsing
- Doc comments are preserved for documentation generation

### 2.3 Identifiers

**Definition 2.2 (Identifier):** An identifier is a sequence of characters used to name program entities.

**Syntax:**
```ebnf
Identifier ::= IdentStart IdentContinue*
IdentStart ::= [a-zA-Z_]
IdentContinue ::= [a-zA-Z0-9_]
```

```cantrip
valid_identifier
_private
CamelCase
snake_case
CONSTANT_VALUE
identifier123
**Restrictions:**
- Cannot be a reserved keyword
- Cannot start with a digit
- Case-sensitive: `Variable` ≠ `variable`
- Maximum length: Implementation-defined (recommended: 255 characters)

**Formal property (uniqueness):**
```
∀ x, y ∈ Identifier. x = y ⟺ string(x) = string(y)
```

### 2.4 Literals

#### 2.4.1 Integer Literals

**Syntax:**
```ebnf
IntegerLiteral ::= DecimalLiteral
                 | HexLiteral
                 | BinaryLiteral
                 | OctalLiteral

DecimalLiteral ::= [0-9] [0-9_]*
HexLiteral ::= "0x" [0-9a-fA-F] [0-9a-fA-F_]*
BinaryLiteral ::= "0b" [01] [01_]*
OctalLiteral ::= "0o" [0-7] [0-7_]*
```

```cantrip
42              // Decimal
0x2A            // Hexadecimal (42)
0b101010        // Binary (42)
0o52            // Octal (42)
1_000_000       // With separators (1000000)
**Type inference:**
- Default type: `i32`
- Suffix determines type: `42u64`, `100i8`
- Context can override: `let x: u8 = 42;`

**Formal semantics:**
```
⟦n⟧ᵢₙₜ = value of integer n in base-10
⟦0xN⟧ᵢₙₜ = value of N in base-16
⟦0bN⟧ᵢₙₜ = value of N in base-2
⟦0oN⟧ᵢₙₜ = value of N in base-8
```

#### 2.4.2 Floating-Point Literals

**Syntax:**
```ebnf
FloatLiteral ::= DecimalLiteral "." DecimalLiteral Exponent?
Exponent ::= [eE] [+-]? DecimalLiteral
```

```cantrip
3.14
1.0e10
2.5e-3
0.1
**Type inference:**
- Default type: `f64`
- Suffix determines type: `3.14f32`

**Formal semantics:**
```
⟦d₁.d₂⟧_float = d₁ + d₂ × 10^(-|d₂|)
⟦x eᵏ⟧_float = x × 10^k
```

#### 2.4.3 Boolean Literals

**Syntax:**
```
BooleanLiteral ::= "true" | "false"
```

**Formal semantics:**
```
⟦true⟧_bool = ⊤
⟦false⟧_bool = ⊥
```

#### 2.4.4 Character Literals

**Syntax:**
```ebnf
CharLiteral ::= "'" (EscapeSequence | ~['\\]) "'"
EscapeSequence ::= "\\" [nrt\\'"0]
                 | "\\x" HexDigit HexDigit
                 | "\\u{" HexDigit+ "}"
```

```cantrip
'a'
'\n'            // Newline
'\t'            // Tab
'\\'            // Backslash
'\x41'          // 'A'
'\u{1F600}'     // 😀 (emoji)
**Type:** `char` (32-bit Unicode scalar value)

#### 2.4.5 String Literals

**Syntax:**
```ebnf
StringLiteral ::= '"' (EscapeSequence | ~["\\])* '"'
```

```cantrip
"hello world"
"line 1\nline 2"
"unicode: 🚀"
"quotes: \" "
**Type:** `str` (immutable string slice)

**Formal semantics:**
```
⟦"s"⟧_string = sequence of Unicode code points
```

### 2.5 Keywords

### 2.6 Attributes

**Purpose:** Attributes are metadata annotations that modify compilation behavior of the item that follows.
Attributes are parsed but do not change program semantics unless specified below.

**Syntax:**
```ebnf
Attribute      ::= "#[" AttributeBody "]"
AttributeBody  ::= Ident ( "(" AttrArgs? ")" )?
AttrArgs       ::= AttrArg ( "," AttrArg )*
AttrArg        ::= Ident "=" Literal | Literal | Ident
```

**Placement:** Attributes MAY appear on modules, records, enums, traits, impl blocks, functions, procedures,
parameters, type aliases, and effects, unless a specific attribute restricts its placement.

**Core attributes:**

- `#[repr(...)]` — layout control (see §6.5)
- `#[verify(static|runtime|none)]` — verification mode (see §20.1)
- `#[overflow_checks(true|false)]` — integer overflow checking (see §4.1.1)
- `#[module(...)]` — module metadata (see §30.3)

**New attribute: `#[alias(...)]` (Informative tooling; Normative parsing)**

The `#[alias]` attribute declares alternative *source-level* names for an item to improve diagnostics,
code search, and LLM tooling. It is ignored for symbol linkage and ABI.

```cantrip
#[alias("to_string", "stringify")]
public function to_str(x: Debug): String { x.debug() }
```

Rules:
1. **Scope:** Aliases are accepted in diagnostics, code actions, and import suggestions. They MUST NOT
   create new exported symbols nor change name resolution in compiled code.
2. **Collision:** If an alias collides with a real item name in the same scope, the real name wins.
   A warning `W0101` SHOULD be emitted.
3. **Debug-only export:** Aliases are not exported; they MAY appear in machine-readable output (see §55)
   behind `emitAliases=true` to help IDEs/LLMs.
4. **FFI:** To control external symbol names, use `#[repr(C)]` and FFI attributes in §56, not `#[alias]`.

**Definition 2.3 (Keyword):** A keyword is a reserved identifier with special syntactic meaning.

**Reserved keywords (cannot be used as identifiers):**

```
abstract    as          async       await       break
case        comptime    const       continue    defer
effect      else        ensures     enum        exists
false       for         forall      function    if
impl        import      internal    invariant   iso
let         loop        match       modal       module
move        mut         needs       new         none
own         private     protected   procedure   public
record      ref         region      requires    result
select      self        Self        state       static
trait       true        type        var         where
while
```

**Contextual keywords (special meaning in specific contexts):**
```
effects     pure
```

**Normative requirement:** Implementations MUST reject programs that use reserved keywords as identifiers. Contextual keywords MAY be used as identifiers outside their specific contexts.

---

## 3. Abstract Syntax

### 3.1 Type Syntax

**Definition 3.1 (Type Language):**

```
T ::= i8 | i16 | i32 | i64 | isize          (signed integers)
    | u8 | u16 | u32 | u64 | usize          (unsigned integers)
    | f32 | f64                              (floats)
    | bool                                   (booleans)
    | char                                   (characters)
    | str                                    (string slice)
    | [T; n]                                 (fixed array)
    | [T]                                    (slice)
    | (T₁, ..., Tₙ)                          (tuple)
    | fn(T₁, ..., Tₙ): U                     (function type)
    | proc(SelfPerm, T₁, ..., Tₙ): U         (procedure type)
    | own T                                  (owned type)
    | mut T                                  (mutable reference)
    | iso T                                  (isolated reference)
    | T@S                                    (type T in modal state @S)
    | ∀α. T                                  (polymorphic type)
    | !                                      (never type)
    | record Name                            (record type)
    | modal Name                             (modal type)
    | enum Name                              (enum type)
    | trait Name                             (trait type)
```

**Type well-formedness:**

```
[T-Int]
────────────────
Γ ⊢ i32 : Type

[T-Array]
Γ ⊢ T : Type    n > 0
────────────────────────
Γ ⊢ [T; n] : Type

[T-Fun]
Γ ⊢ T₁ : Type    Γ ⊢ T₂ : Type
─────────────────────────────────
Γ ⊢ T₁ -> T₂ : Type

[T-Modal]
modal P { ... } well-formed
───────────────────────────────
Γ ⊢ modal P : Type
```

### 3.2 Expression Syntax

**Definition 3.2 (Expression Language):**

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
    | e₁ ; e₂                               (sequencing)
    | move e                                (ownership transfer)
    | region r { e }                        (region block)
    | alloc_in⟨r⟩(e)                        (region allocation)
    | match e { pᵢ -> eᵢ }                  (pattern matching)
    | while e₁ do e₂                        (loop)
    | loop { e }                            (infinite loop)
    | break                                 (loop exit)
    | continue                              (loop continue)
    | return e                              (early return)
    | contract(P, e, Q)                     (contract annotation)
    | transition(e, S)                      (explicit state transition)
    | comptime { e }                        (compile-time execution)
```

### 3.3 Value Syntax

**Definition 3.3 (Values):**

```
v ::= n                                     (integer literal)
    | f                                     (float literal)
    | true | false                          (boolean)
    | ()                                    (unit/none)
    | (v₁, ..., vₙ)                         (tuple value)
    | λx:T. e                               (closure)
    | ℓ                                     (location/pointer)
```

### 3.4 Pattern Syntax

**Definition 3.4 (Patterns):**

```
p ::= x                                     (variable pattern)
    | _                                     (wildcard)
    | n                                     (integer literal)
    | true | false                          (boolean literal)
    | (p₁, ..., pₙ)                         (tuple pattern)
    | Variant(p)                            (enum pattern)
    | Record { f₁: p₁, ..., fₙ: pₙ }       (record pattern)
    | p if e                                (guard pattern)
```

### 3.5 Contract Syntax

**Definition 3.5 (Assertions):**

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
    needs ε;                                (effect declaration)
    requires P;                             (preconditions)
    ensures Q;                              (postconditions)
```

### 3.6 Effect Syntax

**Definition 3.6 (Effects):**

```
ε ::= ∅                                     (pure/no effects)
    | alloc.heap                            (heap allocation)
    | alloc.heap(bytes≤n)                   (bounded heap)
    | alloc.region                          (region allocation)
    | alloc.stack                           (stack allocation)
    | alloc.temp                            (temporary allocation)
    | alloc.*                               (any allocation)
    | fs.read | fs.write | fs.delete       (filesystem)
    | net.read(d) | net.write               (network, d ∈ {inbound, outbound})
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

---

# Part II: Type System

## 4. Types and Values

### 4.1 Primitive Types

**Definition 4.1 (Primitive Types):** Cantrip provides built-in types for common data representations.

#### 4.1.1 Integer Types

**Signed integers:**

| Type | Size | Range | Formal Definition |
|------|------|-------|-------------------|
| `i8` | 8 bits | -128 to 127 | ℤ ∩ [-2⁷, 2⁷-1] |
| `i16` | 16 bits | -32,768 to 32,767 | ℤ ∩ [-2¹⁵, 2¹⁵-1] |
| `i32` | 32 bits | -2³¹ to 2³¹-1 | ℤ ∩ [-2³¹, 2³¹-1] |
| `i64` | 64 bits | -2⁶³ to 2⁶³-1 | ℤ ∩ [-2⁶³, 2⁶³-1] |
| `isize` | Platform | Pointer-sized | ℤ ∩ [-2ⁿ⁻¹, 2ⁿ⁻¹-1] where n = ptr_width |

**Unsigned integers:**

| Type | Size | Range | Formal Definition |
|------|------|-------|-------------------|
| `u8` | 8 bits | 0 to 255 | ℕ ∩ [0, 2⁸-1] |
| `u16` | 16 bits | 0 to 65,535 | ℕ ∩ [0, 2¹⁶-1] |
| `u32` | 32 bits | 0 to 4,294,967,295 | ℕ ∩ [0, 2³²-1] |
| `u64` | 64 bits | 0 to 2⁶⁴-1 | ℕ ∩ [0, 2⁶⁴-1] |
| `usize` | Platform | Pointer-sized | ℕ ∩ [0, 2ⁿ-1] where n = ptr_width |

**Overflow behavior:**

**Axiom 4.1 (Integer Arithmetic):**
```
For type T with min M and max N:
- Debug mode: x ⊕ y ∉ [M, N] ⟹ panic
- Release mode: (x ⊕ y) mod (N - M + 1) ∈ [M, N]
```

```cantrip
let x: u8 = 255;
let y = x + 1;  // Debug: panic, Release: 0 (wrapping)
**Attribute control:**
```cantrip
#[overflow_checks(true)]
function critical_math(a: i32, b: i32): i32 {
    a * b  // Always checked, even in release
}
```

#### 4.1.2 Floating-Point Types

**Definition 4.2 (Floating-Point Types):**

| Type | Size | Standard | Precision | Formal |
|------|------|----------|-----------|--------|
| `f32` | 32 bits | IEEE 754 | Single | ℝ ∩ representable₃₂ |
| `f64` | 64 bits | IEEE 754 | Double | ℝ ∩ representable₆₄ |

**Special values:**
```cantrip
let inf: f64 = 1.0 / 0.0;       // ∞
let neg_inf: f64 = -1.0 / 0.0;  // -∞
let nan: f64 = 0.0 / 0.0;       // NaN
```

**Formal properties:**
```
⟦f₁ + f₂⟧ = round(⟦f₁⟧ + ⟦f₂⟧)
⟦f₁ * f₂⟧ = round(⟦f₁⟧ × ⟦f₂⟧)
NaN ≠ NaN
x + ∞ = ∞ (for x ≠ -∞)
```

#### 4.1.3 Boolean Type

**Definition 4.3 (Boolean Type):**
```
⟦bool⟧ = {true, false} = {⊤, ⊥}
```

**Operations:**
```
¬⊤ = ⊥
¬⊥ = ⊤
⊤ ∧ ⊤ = ⊤
⊤ ∧ ⊥ = ⊥
⊥ ∧ _ = ⊥    (short-circuit)
```

```cantrip
let a = true;
let b = false;
let c = a && b;  // false
let d = a || b;  // true
#### 4.1.4 Character Type

**Definition 4.4 (Character Type):**
```
⟦char⟧ = Unicode Scalar Values = [U+0000, U+D7FF] ∪ [U+E000, U+10FFFF]
```

**Properties:**
- Size: 32 bits (4 bytes)
- Valid range: Excludes surrogate pairs [U+D800, U+DFFF]
- Representation: UTF-32

```cantrip
let ch: char = 'A';         // U+0041
let emoji: char = '🚀';     // U+1F680
### 4.2 Compound Types

#### 4.2.1 Arrays

**Definition 4.5 (Fixed-Size Array):**
```
[T; n] = { (v₁, ..., vₙ) | ∀i. vᵢ : T }
```

**Formal properties:**
```
size([T; n]) = n × size(T)
align([T; n]) = align(T)
layout = contiguous memory
```

```cantrip
let numbers: [i32; 5] = [1, 2, 3, 4, 5];
let zeros: [i32; 100] = [0; 100];  // Repeat syntax

// Type rules:
// Γ ⊢ numbers : [i32; 5]
// Γ ⊢ numbers[0] : i32
**Access semantics:**
```
[T-Array-Index]
Γ ⊢ arr : [T; n]    Γ ⊢ i : usize    i < n
────────────────────────────────────────────
Γ ⊢ arr[i] : T
```

#### 4.2.2 Slices

**Definition 4.6 (Slice):**
```
[T] = { (ptr, len) | ptr : *T, len : usize }
```

**Fat pointer representation:**
```
record SliceRepr<T> {
    data: *T;      // Pointer to first element
    length: usize; // Number of elements
}
```

```cantrip
let arr = [1, 2, 3, 4, 5];
let slice: [i32] = arr[1..3];  // [2, 3]

// Range semantics:
// arr[a..b] = { arr[i] | a ≤ i < b }
**Type rules:**
```
[T-Slice]
Γ ⊢ arr : [T; n]    Γ ⊢ start : usize    Γ ⊢ end : usize
start ≤ end ≤ n
────────────────────────────────────────────────────────────
Γ ⊢ arr[start..end] : [T]
```

#### 4.2.3 Tuples

**Definition 4.7 (Tuple):**
```
(T₁, ..., Tₙ) = T₁ × T₂ × ... × Tₙ
```

```cantrip
let pair: (i32, str) = (42, "answer");
let (x, y) = pair;          // Destructuring
let first = pair.0;          // Index access
let second = pair.1;

// Type rules:
// Γ ⊢ pair : (i32, str)
// Γ ⊢ pair.0 : i32
// Γ ⊢ pair.1 : str
**Formal semantics:**
```
[T-Tuple]
Γ ⊢ e₁ : T₁    ...    Γ ⊢ eₙ : Tₙ
───────────────────────────────────
Γ ⊢ (e₁, ..., eₙ) : (T₁, ..., Tₙ)

[T-Tuple-Proj]
Γ ⊢ e : (T₁, ..., Tₙ)    i ∈ [0, n)
───────────────────────────────────────
Γ ⊢ e.i : Tᵢ
```

### 4.3 Never Type

**Definition 4.8 (Never Type):**
```
⟦!⟧ = ∅  (empty type, no values)
```

**Formal property (Bottom Type):**
```
[T-Never]
Γ ⊢ e : !
────────────────  (for any type T)
Γ ⊢ e : T
```

**Uses:**
1. Functions that never return
2. Match arms that are unreachable
3. Error handling branches

```cantrip
function panic(message: str): !
    needs panic;
{
    std.process.abort();
}

function example(x: i32): i32 {
    if x < 0 {
            panic("negative value");  // Type: !
            // Compiler knows this never returns
        } else {
            x * 2  // Only this branch returns
        }
}
---

## 5. Type Rules and Semantics

### 5.1 Core Type Rules

**[T-Var] Variable typing:**
```
x : T ∈ Γ
───────────
Γ ⊢ x : T
```

```cantrip
let x: i32 = 42;
// Context: Γ = {x : i32}
// Judgment: Γ ⊢ x : i32
**[T-Let] Let binding:**
```
Γ ⊢ e₁ : T₁    Γ, x : T₁ ⊢ e₂ : T₂
───────────────────────────────────────
Γ ⊢ let x = e₁ in e₂ : T₂
```

```cantrip
let result = let x = 42 in x + 1;
// Type derivation:
// ⊢ 42 : i32
// x : i32 ⊢ x + 1 : i32
// ⊢ let x = 42 in x + 1 : i32
**[T-App] Function application:**
```
Γ ⊢ e₁ : T -> U    Γ ⊢ e₂ : T
──────────────────────────────
Γ ⊢ e₁(e₂) : U
```

```cantrip
function double(x: i32): i32 {
    x * 2
}

let result = double(21);
// double : i32 -> i32
// 21 : i32
// double(21) : i32
**[T-Abs] Lambda abstraction:**
```
Γ, x : T ⊢ e : U
────────────────────
Γ ⊢ λx:T. e : T -> U
```

```cantrip
let add_one = |x: i32| { x + 1 };
// Type: i32 -> i32
**[T-If] Conditional:**
```
Γ ⊢ e₁ : bool    Γ ⊢ e₂ : T    Γ ⊢ e₃ : T
────────────────────────────────────────────
Γ ⊢ if e₁ then e₂ else e₃ : T
```

```cantrip
let max = if a > b { a } else { b };
// Both branches must have same type
**[T-Pipeline] Pipeline operator:**
```
Γ ⊢ e₁ : T    Γ ⊢ e₂ : T -> U
──────────────────────────────
Γ ⊢ e₁ => e₂ : U
```

```cantrip
let result = data
    => filter(predicate)
    => map(transform)
    => sum;
// Each stage must match types
### 5.2 Subtyping

**Definition 5.1 (Subtype Relation):**
```
T <: U  (T is a subtype of U)
```

**[Sub-Refl] Reflexivity:**
```
─────────
T <: T
```

**[Sub-Trans] Transitivity:**
```
T <: U    U <: V
────────────────
T <: V
```

**[Sub-Never] Bottom type:**
```
─────────
! <: T
```

**[Sub-Fun] Function contravariance:**
```
T₂ <: T₁    U₁ <: U₂
────────────────────────
T₁ -> U₁ <: T₂ -> U₂
```

**[Sub-Permission] Permission hierarchy:**
```
own T <: mut T <: T
iso T <: own T
```

### 5.3 Type Inference

**Type inference algorithm:** Hindley-Milner with extensions for effects and modals.

**Constraint generation:**
```
Γ ⊢ e : T | C  (expression e has type T with constraints C)
```

**Example inference:**
```cantrip
function identity(x) {  // No type annotation, return type inferred
    x
}

// Generates constraints:
// ∀α. identity : α -> α
// Solution: polymorphic type
```

**Inference limits:**
1. Function signatures must declare effects explicitly (no inference)
2. Record fields must have explicit types
3. Function return types can be inferred from body
4. Local variables can be inferred from initialization

---

## 6. Records and Classes

**Definition 6.0 (Record as Labeled Product Type):**
A *record* is a labeled Cartesian product of its fields. Formally,
if `R { f₁: T₁; …; fₙ: Tₙ }` then `⟦R⟧ ≅ T₁ × … × Tₙ` with
named projections `fᵢ : R → Tᵢ`. Records provide *procedures* (see §13.7)
via `impl R` blocks; inheritance is not part of the model—use traits and composition.

### 6.1 Record Declaration

**Syntax:**
```cantrip
record Name {
    field₁: Type₁;
    ...
    fieldₙ: Typeₙ;
}
```

**Formal definition:**
```
[Record-Def]
Γ ⊢ T₁ : Type    ...    Γ ⊢ Tₙ : Type
fields distinct
──────────────────────────────────────────────
Γ ⊢ record Name { f₁: T₁; ...; fₙ: Tₙ } : Type
```

```cantrip
record Point {
    x: f64;
    y: f64;
}

public record User {
    public name: String;
    public email: String;
    private password_hash: String;
}
**Memory layout:**
```
Point layout:
[x: 8 bytes][y: 8 bytes]
Total: 16 bytes, align: 8
```

### 6.2 Record Construction

**Syntax:**
```cantrip
Name { field₁: expr₁, ..., fieldₙ: exprₙ }
```

**Type rule:**
```
[T-Record]
record Name { f₁: T₁; ...; fₙ: Tₙ }
Γ ⊢ e₁ : T₁    ...    Γ ⊢ eₙ : Tₙ
───────────────────────────────────────────
Γ ⊢ Name { f₁: e₁, ..., fₙ: eₙ } : Name
```

```cantrip
let origin = Point { x: 0.0, y: 0.0 };

// Field shorthand
let x = 3.0;
let y = 4.0;
let point = Point { x, y };  // Equivalent to Point { x: x, y: y }
### 6.3 Field Access

**Type rule:**
```
[T-Field]
Γ ⊢ e : record S    field f : T in S
────────────────────────────────────
Γ ⊢ e.f : T
```

```cantrip
let p = Point { x: 1.0, y: 2.0 };
let x_coord = p.x;  // x_coord : f64
### 6.4 Procedures

**Definition 6.1 (Procedure):** A procedure is a function with an explicit `self` parameter.

**Syntax:**
```cantrip
impl StructName {
    function method_name(self: Permission StructName, params): T
        requires ...;
        ensures ...;
    {
        ...
    }
}
```

```cantrip
impl Point {
    function new(x: f64, y: f64): own Point {
        own Point { x, y }
    }

    function distance(self: Point, other: Point): f64 {
        let dx = self.x - other.x;
        let dy = self.y - other.y;
        (dx * dx + dy * dy).sqrt()
    }

    function move_by(self: mut Point, dx: f64, dy: f64) {
        self.x += dx;
        self.y += dy;
    }
}
**Procedure call desugaring:**
```
obj.procedure(args) ≡ Type::procedure(obj, args)
```

**Type rule:**
```
[T-Procedure]
procedure m : Self -> T -> U in impl for S
Γ ⊢ self : S    Γ ⊢ arg : T
───────────────────────────────────────
Γ ⊢ self.m(arg) : U
```

### 6.5 Memory Layout Control

**C-compatible layout:**
```cantrip
#[repr(C)]
record Vector3 {
    x: f32;  // offset 0
    y: f32;  // offset 4
    z: f32;  // offset 8
}
// Size: 12 bytes, Align: 4
```

**Packed layout (no padding):**
```cantrip
#[repr(packed)]
record NetworkHeader {
    magic: u32;     // offset 0
    version: u16;   // offset 4
    length: u16;    // offset 6
}
// Size: 8 bytes, Align: 1
```

**Explicit alignment:**
```cantrip
#[repr(align(64))]
record CacheLine {
    data: [u8; 64];
}
// Aligned to 64-byte boundary
```

**Formal semantics:**
```
layout(record S) = {
    size = ∑ᵢ size(fᵢ) + padding
    align = max(align(f₁), ..., align(fₙ))
    offset(fᵢ) = aligned(∑ⱼ₍ⱼ₍ᵢ₎ size(fⱼ), align(fᵢ))
}
```

### 6.6 Structure-of-Arrays

**Attribute:**
```cantrip
#[soa]
record Particle {
    position: Vec3;
    velocity: Vec3;
    lifetime: f32;
}
```

**Compiler transformation:**
```cantrip
// Generates:
record ParticleSOA {
    positions: Vec<Vec3>;
    velocities: Vec<Vec3>;
    lifetimes: Vec<f32>;
    length: usize;
}

impl ParticleSOA {
    function get(self: ParticleSOA, index: usize): Particle
        requires index < self.length;
    {
        Particle {
            position: self.positions[index],
            velocity: self.velocities[index],
            lifetime: self.lifetimes[index],
        }
    }
}
```

## 7. Enums and Pattern Matching

### 7.1 Enum Declaration

**Definition 7.1 (Enum):** An enum is a sum type (tagged union) with named variants.

**Syntax:**
```cantrip
enum Name {
    Variant₁,
    Variant₂(Type),
    Variant₃ { field: Type },
}
```

**Formal definition:**
```
⟦enum E { V₁(T₁), ..., Vₙ(Tₙ) }⟧ = T₁ + T₂ + ... + Tₙ
```

```cantrip
enum Status {
    Active,
    Pending,
    Inactive,
}

enum Result<T, E> {
    Ok(T),
    Err(E),
}

enum Option<T> {
    Some(T),
    None,
}

enum Message {
    Quit,
    Move { x: i32, y: i32 },
    Write(String),
    ChangeColor(u8, u8, u8),
}
**Memory layout:**
```
enum Layout = {
    discriminant: usize,    // Tag
    payload: union {        // Largest variant
        variant₁: T₁,
        ...
        variantₙ: Tₙ,
    }
}
```

**Type rule:**
```
[T-Enum]
enum E { V₁(T₁), ..., Vₙ(Tₙ) } well-formed
Γ ⊢ e : Tᵢ
──────────────────────────────────────────
Γ ⊢ E.Vᵢ(e) : E
```

### 7.2 Pattern Matching

**Syntax:**
```cantrip
match expression {
    pattern₁ -> expression₁,
    pattern₂ -> expression₂,
    ...
}
```

**Type rule:**
```
[T-Match]
Γ ⊢ e : T
∀i. Γ, pᵢ ⊢ eᵢ : U
patterns exhaustive for T
──────────────────────────────
Γ ⊢ match e { pᵢ -> eᵢ } : U
```

**Exhaustiveness checking:**

**Definition 7.2 (Exhaustive Patterns):** A set of patterns is exhaustive if every possible value matches at least one pattern.

**Algorithm:**
```
exhaustive(patterns, type) = {
    1. Collect all constructors of type
    2. For each constructor C:
       - Check if any pattern matches C
       - Recursively check nested patterns
    3. Return true if all constructors covered
}
```

```cantrip
enum Status {
    Active,
    Pending,
    Inactive,
}

function describe(status: Status): str {
    match status {
        Status.Active -> "active",
        Status.Pending -> "pending",
        Status.Inactive -> "inactive",
        // Exhaustive - all variants covered
    }
}
### 7.3 Pattern Forms

**Literal patterns:**
```cantrip
match number {
    0 -> "zero",
    1 -> "one",
    2 -> "two",
    _ -> "many",  // Wildcard
}
```

**Tuple patterns:**
```cantrip
match point {
    (0, 0) -> "origin",
    (0, y) -> "on y-axis",  // Binds y
    (x, 0) -> "on x-axis",  // Binds x
    (x, y) -> "point",       // Binds both
}
```

**Enum patterns:**
```cantrip
match result {
    Result.Ok(value) -> process(value),
    Result.Err(error) -> handle_error(error),
}
```

**Record patterns:**
```cantrip
match person {
    Person { name, age: 0..=17 } -> "minor: {name}",
    Person { name, age: 18..=64 } -> "adult: {name}",
    Person { name, age: 65.. } -> "senior: {name}",
}
```

**Guard patterns:**
```cantrip
match value {
    x if x > 0 -> "positive",
    x if x < 0 -> "negative",
    _ -> "zero",
}
```

**Type rules for patterns:**

```
[P-Var]
────────────────────
Γ ⊢ x : T ⇝ {x : T}

[P-Tuple]
Γ ⊢ p₁ : T₁ ⇝ Γ₁    ...    Γ ⊢ pₙ : Tₙ ⇝ Γₙ
────────────────────────────────────────────────
Γ ⊢ (p₁, ..., pₙ) : (T₁, ..., Tₙ) ⇝ Γ₁ ∪ ... ∪ Γₙ

[P-Enum]
enum E { V(T), ... }
Γ ⊢ p : T ⇝ Γ'
────────────────────
Γ ⊢ E.V(p) : E ⇝ Γ'
```

### 7.4 Discriminant Values

**Explicit discriminants:**
```cantrip
enum HttpStatus {
    Ok = 200,
    NotFound = 404,
    ServerError = 500,
}

let code: u16 = HttpStatus.Ok as u16;  // 200
```

**Type rule:**
```
[T-Discriminant]
enum E { V₁ = n₁, ..., Vₖ = nₖ }
───────────────────────────────────
E as IntType valid
```

---

## 8. Traits

### 8.1 Trait Declaration

**Definition 8.1 (Trait):** An trait specifies a set of procedures that types must implement.

**Syntax:**
```cantrip
public trait InterfaceName {
    function method₁(self: Self, params) {
        requires: ...;
        returns<T>: ...;
    }

    // Default implementation optional
    function method₂(self: Self): U
        requires ...;
        ensures ...;
    {
        ...
    }
}
```

```cantrip
public trait Drawable {
    function draw(self: Self, canvas: Canvas)
        needs render.draw;

    function bounds(self: Self): Rectangle;
}

public trait Comparable<T> {
    function compare(self: Self, other: T): i32
        ensures
            (result < 0) == (self < other);
            (result == 0) == (self == other);
            (result > 0) == (self > other);
}
**Type rule:**
```
[T-Trait]
procedures m₁, ..., mₙ well-formed
────────────────────────────────────────
trait I { m₁; ...; mₙ } well-formed
```

### 8.2 Default Procedures

**Definition 8.2 (Default Implementation):** An trait can provide default implementations for procedures.

```cantrip
public trait Logger {
    // Required procedure
    function log(self: Self, message: String)
        needs io.write;

    // Default implementations
    function log_error(self: Self, err: Error)
        needs io.write;
    {
        self.log("ERROR: " + err.to_string());
    }

    function log_warning(self: Self, msg: String)
        needs io.write;
    {
        self.log("WARN: " + msg);
    }
}
**Inheritance of defaults:**
```
impl Logger for MyLogger {
    function log(self: Self, message: String)
    needs io.write;
{
    std.io.println(message);
    }
    // log_error and log_warning inherited
}
```

### 8.3 Implementation Blocks

**Syntax:**
```cantrip
impl InterfaceName for TypeName {
    // Implement required procedures
    function procedure(self: Self, params): T
        requires ...;
        ensures ...;
    {
        ...
    }
}
```

```cantrip
record Point { x: f64, y: f64 }

impl Drawable for Point {
    function draw(self: Self, canvas: Canvas)
        needs render.draw;
    {
        canvas.draw_circle(self.x, self.y, 2.0);
    }

    function bounds(self: Self): Rectangle {
        Rectangle.new(self.x - 2.0, self.y - 2.0, 4.0, 4.0)
    }
}
**Type rule:**
```
[T-Impl]
trait I { m₁: T₁; ...; mₙ: Tₙ }
record S { ... }
∀i. impl mᵢ : Tᵢ[Self ↦ S]
───────────────────────────────────
impl I for S well-formed
```

**Coherence requirement:** At most one implementation of an trait for a given type.

```
[Coherence]
impl I for T₁
impl I for T₂
T₁ ≠ T₂
─────────────────
Error: Overlapping implementations
```

### 8.4 Generic Constraints

**Syntax:**
```cantrip
function name<T: Constraint>(param: T): U
    where T: AdditionalConstraint;
    requires ...;
    ensures ...;
{
    ...
}
```

```cantrip
function max<T>(a: T, b: T): T
    where T: Comparable<T>
{
    if a.compare(b) > 0 {
        a
    } else {
        b
    }
}
**Type rule:**
```
[T-Generic-Constraint]
Γ, T : Type, T : Constraint ⊢ e : U
─────────────────────────────────────────
Γ ⊢ function f<T: Constraint>(...) : U
```

### 8.5 Associated Types

**Syntax:**
```cantrip
public trait Iterator {
    type Item;

    function next(self: mut Self): Option<Self.Item>;
}
```

**Implementation:**
```cantrip
record RangeIterator {
    current: i32;
    end: i32;
}

impl Iterator for RangeIterator {
    type Item = i32;

    function next(self: mut Self): Option<i32> {
        if self.current < self.end {
            let value = self.current;
            self.current += 1;
            Some(value)
        } else {
            None
        }
    }
}
```

**Type rule:**
```
[T-Assoc-Type]
trait I { type A; ... }
impl I for T { type A = U; ... }
──────────────────────────────────
T : I    T.A = U
```

---

### 8.6 Trait-Declared Effects

**Grammar extension (add to §8.1):**
```ebnf
TraitMethod   ::= "function" Ident "(" ParamList? ")" ( ":" Type )?
                  NeedsClause? RequiresClause? EnsuresClause? TraitMethodBody?
NeedsClause   ::= "needs" EffectExpr ";"
```
The `needs` clause on a trait method declares an *upper bound* on effects that any implementation may perform.
An implementation method MUST satisfy `ε_impl ⊆ ε_trait` (effect subsumption).

**Type rule (trait effect bound):**
```
[T-Trait-Effects]
trait I { function m(...) : U  needs ε_trait; }
impl I for T { function m(...) : U  needs ε_impl;  body }
ε_impl ⊆ ε_trait
──────────────────────────────────────────────────────────
impl well-typed
```

**Diagnostics:**
- `E9120` — *Trait effect exceeds bound*: implementation declares or exhibits effects not permitted by the trait.
  Fix-it: “narrow `needs` to be ≤ specified bound” or “refine trait bound”.

```cantrip
public trait Logger {
    function log(self: Self, message: String)
        needs io.write;
}

impl Logger for MyLogger {
    function log(self: Self, message: String)
        needs io.write; // OK: equal
    { std.io.println(message); }
}

impl Logger for BadLogger {
    function log(self: Self, message: String)
        needs io.write, fs.write; // ERROR E9120: fs.write exceeds trait bound
    { std.fs.append("log.txt", message) }
}
### 8.7 Coherence and Orphan Rules

**Coherence (global uniqueness):** For any fully monomorphized pair `(Trait, Type)`, at most one implementation
is visible in the *program* (the primary package plus all transitive dependencies).
Overlapping implementations are a compile error.

```
[Coherence-Global]
(impl I for T) and (impl I for T) from distinct crates
──────────────────────────────────────────────────────
ERROR E8201: overlapping implementations
```

**Orphan rule (locality):** An implementation `impl I for T` is permitted iff at least one of the following holds:
1. `I` is defined in the current package, or
2. the *outermost* type constructor of `T` is defined in the current package.

For generic `T = C<U1, ..., Un>`, the outer constructor `C` must be local.
Parametric arguments do not make an impl local by themselves.

```
[Orphan]
I ∉ Local  ∧  outer(T) ∉ Local
───────────────────────────────
ERROR E8202: orphan implementation
```

**Blanket impls:** Blanket implementations (e.g., `impl<T: Bound> I for Vec<T>`) are allowed if they satisfy the orphan rule.
Specialization is not part of this specification.

**Visibility of impls:** Implementations are public by definition (§32.7).

## 9. Generics and Parametric Polymorphism

### 9.1 Generic Functions

**Definition 9.1 (Parametric Polymorphism):** Generic functions abstract over types.

**Syntax:**
```cantrip
function name<T₁, ..., Tₙ>(params): U {
    ...
}
```

```cantrip
function identity<T>(x: T): T {
    x
}

function swap<T>(a: T, b: T): (T, T) {
    (b, a)
}

function first<T, U>(pair: (T, U)): T {
    pair.0
}
**Type rule:**
```
[T-Generic-Fun]
Γ, α₁ : Type, ..., αₙ : Type ⊢ body : T
─────────────────────────────────────────────
Γ ⊢ function f<α₁, ..., αₙ>(...) : ∀α₁...αₙ. T
```

**Instantiation:**
```
[T-Inst]
Γ ⊢ e : ∀α. T    Γ ⊢ U : Type
──────────────────────────────
Γ ⊢ e : T[α ↦ U]
```

### 9.2 Generic Records

**Syntax:**
```cantrip
record Name<T₁, ..., Tₙ> {
    field₁: Type₁,
    ...
}
```

```cantrip
record Pair<T, U> {
    first: T;
    second: U;
}

record Box<T> {
    value: T;
}

impl<T> Box<T> {
    function new(value: T): own Box<T>
        needs alloc.heap;
    {
        own Box { value }
    }

    function get(self: Box<T>): T {
        self.value
    }
}
**Type rule:**
```
[T-Generic-Record]
Γ, α₁ : Type, ..., αₙ : Type ⊢ fields well-formed
──────────────────────────────────────────────────────
Γ ⊢ record S<α₁, ..., αₙ> { ... } : Type -> ... -> Type
```

### 9.3 Where Clauses

**Syntax:**
```cantrip
function name<T>(param: T)
    where T: Constraint₁,
          T: Constraint₂,
          U: Constraint₃
{
    ...
}
```

```cantrip
function process<T, U>(data: T): U
    where T: Clone + Serialize,
          U: Display + Debug
    needs alloc.heap;
{
    // Can use clone(), serialize(), display(), debug()
    let copy = data.clone();
    // ...
}
**Type rule:**
```
[T-Where]
Γ, T : Type, T : C₁, ..., T : Cₙ ⊢ body : U
────────────────────────────────────────────────
Γ ⊢ function f<T> where T: C₁, ..., Cₙ : U
```

### 9.4 Monomorphization

**Definition 9.2 (Monomorphization):** Generic code is specialized for each concrete type at compile time.

**Transformation:**
```cantrip
// Generic function
function max<T: Comparable>(a: T, b: T): T {
    if a.compare(b) > 0 { a } else { b }
}

// Usage
let x = max(1, 2);       // T = i32
let y = max(1.0, 2.0);   // T = f64

// Compiler generates:
function max_i32(a: i32, b: i32): i32 { ... }
function max_f64(a: f64, b: f64): f64 { ... }
```

**Performance:** Zero-cost abstraction - no runtime dispatch

**Code size:** May increase binary size (trade-off for speed)

### 9.5 Type Bounds
### 9.6 Const Generics

**Purpose:** *Const generics* allow types and functions to be parameterized by compile‑time constants
(e.g., array lengths, matrix dimensions, bit widths) without runtime overhead.

**Syntax (parameters and instantiation):**
```ebnf
ConstParam     ::= "const" Ident ":" ConstType
ConstType      ::= "usize" | "u8" | "u16" | "u32" | "u64" | "isize" | "i8" | "i16" | "i32" | "i64" | "bool"
GenericParams  ::= "<" (TypeParam | ConstParam) ("," (TypeParam | ConstParam))* ">"
ConstArg       ::= IntegerLiteral | BooleanLiteral | ConstExpr
ConstExpr      ::= (literal | arithmetic over consts | sizeof(T) | alignof(T))
```

```cantrip
// Function parameterized by a const length N
function process<T, const N: usize>(data: [T; N]): T {
    data[0]
}

// Type aliases with const parameters
type Matrix<T, const ROWS: usize, const COLS: usize> = [[T; COLS]; ROWS];

// Records with const parameters
record RingBuffer<T, const CAP: usize> {
    data: [T; CAP];
    head: usize;
    tail: usize;
}
**Type rules:**
```
[T-Const-Generic]
Γ, N : const usize ⊢ e : T
───────────────────────────────────────────
Γ ⊢ function f<const N: usize>(...) : T

[T-Const-Inst]
Γ ⊢ n : usize      n is a compile-time constant
Γ ⊢ f : ∀(const N). T
───────────────────────────────────────────
Γ ⊢ f<n> : T[N ↦ n]
```

**Well-formedness of const expressions:** A const argument MUST be evaluable at compile time using only
other consts, integer/boolean arithmetic, and compile‑time queries such as `@sizeOf(T)`, `@alignOf(T)`.
Effects are forbidden in const expressions.

**Interaction with arrays:** The expression `[T; N]` requires `N` to be a const value known at compile time.
Indexing rules (§4.2.1) use the instantiated `N`.

**Monomorphization:** Const parameters participate in monomorphization alongside type parameters.
Each distinct const argument produces a distinct specialized instance.

**Diagnostics:**
- `E9310` — non-constant used where a const generic argument is required.
- `E9311` — const expression not evaluable at compile time.

**Syntax:**
```cantrip
T: Bound₁ + Bound₂ + ... + Boundₙ
```

```cantrip
function serialize<T>(value: T): String
    where T: Serialize + Clone
    needs alloc.heap;
{
    value.clone().to_json()
}
**Type rule:**
```
[T-Bounds]
Γ ⊢ T : B₁    ...    Γ ⊢ T : Bₙ
──────────────────────────────────
Γ ⊢ T : B₁ + ... + Bₙ
```

---

# Part III: Modal System

## 10. Modals: State Machines as Types

### 10.1 Modal Overview

**Definition 10.1 (Modal):** A modal is a type with an explicit finite state machine, where each state can have different data and the compiler enforces valid state transitions.

**Key innovation:** State machines are first-class types with compile-time verification.

**When to use modals:**
- Resources with lifecycles (files, connections, transactions)
- Stateful business logic (orders, shopping carts, workflows)
- Network modals (TCP handshakes, authentication flows)
- Any type where "state matters" for correctness

**When to use structs:**
- Simple data with no state machine
- Mathematical types (vectors, matrices)
- Configuration objects

### 10.2 Basic Modal Syntax

**Syntax:**
```cantrip
modal Name {
    state State₁ {
        field₁: Type₁;
        ...
    }

    state State₂ {
        field₁: Type₁;
        field₂: Type₂;  // Different fields!
        ...
    }

    procedure name@State₁(params) -> @State₂
        needs ε;
        requires P;
        ensures Q;
    {
        // Implementation
        State₂ { ... }
    }
}
```

```cantrip
modal File {
    // Each state has its own data shape
    state Closed {
        path: String;
    }

    state Open {
        path: String;
        handle: FileHandle;
        position: usize;
    }

    // State transformation functions use @State notation
    procedure open@Closed() -> @Open
        needs fs.read;
    {
        let handle = FileSystem.open(self.path)?;
        Open {
            path: self.path,
            handle: handle,
            position: 0,
        }
    }

    procedure read@Open(n: usize) -> @Open
        needs fs.read;
    {
        let data = self.handle.read(n)?;
        self.position += data.len();
        (data, self)
    }

    procedure close@Open() -> @Closed {
        self.handle.close();
        Closed { path: self.path }
    }
}
```

**Key features:**
- Each state can have **different fields**
- `@State` notation indicates the source state for procedures
- `-> @State` indicates the target state after transition
- Compiler enforces valid transitions

### 10.3 Using Modals

```cantrip
function example(): Result<(), Error>
    needs fs.read, alloc.heap;
{
    let file = File.new("data.txt");  // File@Closed

    // ERROR: Cannot read from closed file
    // let data = file.read(1024)?;

    let file = file.open()?;  // Now File@Open
    let data = file.read(1024)?;  // OK!

    let file = file.close();  // Back to File@Closed

    // ERROR: Cannot read after closing
    // let data2 = file.read(1024)?;

    Ok(())
}
**Type annotation:**
```cantrip
let file: File@Closed = File.new("data.txt");
let file: File@Open = file.open()?;
```

### 10.4 State-Dependent Data

```cantrip
modal Transaction {
    state Pending {
        conn: Connection;
        operations: Vec<Operation>;
    }

    state Committed {
        conn: Connection;
        commit_id: u64;        // Only exists after commit!
        timestamp: DateTime;   // Only exists after commit!
    }

    state RolledBack {
        conn: Connection;
        error: Error;          // Only exists after rollback!
    }

    procedure commit@Pending() -> @Committed {
        let (id, timestamp) = self.conn.commit(self.operations)?;
        Committed {
            conn: self.conn,
            commit_id: id,
            timestamp: timestamp,
        }
    }

    procedure rollback@Pending(err: Error) -> @RolledBack {
        self.conn.rollback();
        RolledBack {
            conn: self.conn,
            error: err,
        }
    }
}

// Usage
let mut tx = Transaction.begin(conn);
tx.add_operation(op1);

let tx = tx.commit()?;

// Compile-time proof that commit_id exists:
std.io.println(String.format("Committed as {}", tx.commit_id));  // OK!
```

**Access to state-specific fields:**
```cantrip
function process(tx: Transaction@Committed) {
    // Can access commit_id because state is Committed
    log_commit(tx.commit_id);
    // tx.error;  // ERROR: error only exists in RolledBack state
}
```

### 10.5 Complex State Machines

**Example: TCP Connection**
```cantrip
modal TcpConnection {
    state Disconnected {
        config: ConnectionConfig;
    }

    state Connecting {
        config: ConnectionConfig;
        socket: Socket;
    }

    state Connected {
        config: ConnectionConfig;
        socket: Socket;
        stream: TcpStream;
    }

    state Failed {
        config: ConnectionConfig;
        error: Error;
    }

    procedure connect@Disconnected(host: String) -> @Connecting
        needs net.read(outbound);
    {
        let socket = Socket.new(host)?;
        Connecting { config: self.config, socket }
    }

    procedure handshake@Connecting() -> (@Connected | @Failed)
        needs net.read, net.write;
    {
        match self.socket.handshake() {
            Ok(stream) -> Ok(Connected {
                config: self.config,
                socket: self.socket,
                stream,
            }),
            Err(err) -> Ok(Failed {
                config: self.config,
                error: err,
            }),
        }
    }

    procedure send@Connected(data: [u8]) -> @Connected
        needs net.write;
    {
        self.stream.write(data)?;
        Ok(self)
    }

    procedure disconnect@Connected() -> @Disconnected {
        self.stream.close();
        Disconnected { config: self.config }
    }
}
```

### 10.6 State Unions

**Procedures can work with multiple states:**

```cantrip
modal OrderProcessor {
    state Draft { items: Vec<Item>; }
    state Submitted { order_id: u64; items: Vec<Item>; }
    state Cancelled { order_id: u64; reason: String; }

    // Can cancel from either state
    procedure cancel@(Submitted | Draft)(reason: String) -> @Cancelled {
        let order_id = match self {
            Submitted { order_id, .. } -> order_id,
            Draft { .. } -> generate_id(),
        };
        Cancelled { order_id, reason }
    }
}
```

**Type rule:**
```
[T-State-Union]
Γ ⊢ e : P@S₁    @S₁ ⊆ (@S₂ | @S₃)
──────────────────────────────────
Γ ⊢ e : P@(@S₂ | @S₃)
```

### 10.7 Common Fields

**For fields that exist in ALL states:**

```cantrip
modal HttpRequest {
    // Common fields across all states
    common {
        url: String;
        headers: HashMap<String, String>;
    }

    state Building {
        // Only common fields
    }

    state Ready {
        procedure: HttpMethod;
        body: Option<Vec<u8>>;
    }

    state Sent {
        procedure: HttpMethod;
        body: Option<Vec<u8>>;
        sent_at: DateTime;
    }

    procedure set_method@Building(m: HttpMethod) -> @Ready {
        Ready {
            // common fields (url, headers) automatically included
            procedure: m,
            body: None,
        }
    }
}
```

**Formal semantics:**
```
state @S with common C = {
    fields(@S) = C ∪ @S.specific_fields
}
```

### 10.10 Modal Instantiation

**Creating initial state:**
```cantrip
modal File {
    state Closed { path: String; }
    state Open { ... }

    // Constructor creates initial state
    function new(path: String): File@Closed {
    File@Closed { path }
    }
}

// Usage
let file = File.new("data.txt");  // Type: File@Closed
```

---

#### 10.9.1 Resource Lifecycle Pattern

**Pattern:** Uninitialized → Ready → InUse → Closed

```cantrip
modal Resource {
    //
    //   Uninitialized --[initialize]--> Ready --[acquire]--> InUse
    //                                     |                     |
    //                                     |                     |
    //                                     +------[release]------+
    //                                     |
    //                                 [close]
    //                                     |
    //                                     v
    //                                  Closed
    //

    state Uninitialized { config: Config; }

    state Ready {
        config: Config;
        handle: Handle;
    }

    state InUse {
        config: Config;
        handle: Handle;
        usage: UsageInfo;
    }

    state Closed { }

    procedure initialize@Uninitialized() -> @Ready
        needs alloc.heap;
    {
        let handle = create_handle(self.config)?;
        Ready { config: self.config, handle }
    }

    procedure acquire@Ready() -> @InUse
        needs time.read;
    {
        let usage = UsageInfo.start();
        InUse {
            config: self.config,
            handle: self.handle,
            usage,
        }
    }

    procedure release@InUse() -> @Ready
        needs time.read;
    {
        self.usage.end();
        Ready {
            config: self.config,
            handle: self.handle,
        }
    }

    procedure close@(Ready | InUse)() -> @Closed
        needs time.read;
    {
        self.handle.close();
        Closed { }
    }
}
```

**Usage:**

```cantrip
function use_resource()
    needs alloc.heap, time.read;
{
    let resource = Resource.new(config);
    let resource = resource.initialize()?;
    let resource = resource.acquire()?;

    // Use resource
    process(resource);

    let resource = resource.release()?;
    resource.close();
}
```

#### 10.9.2 Request-Response Pattern

**Pattern:** Idle → Processing → Responded → Idle (cycle)

```cantrip
modal RequestHandler {
    //
    //   Idle --[receive]--> Processing --[respond]--> Responded
    //    ^                                                 |
    //    |                                                 |
    //    +--------------------[reset]---------------------+
    //

    state Idle { }

    state Processing {
        request: Request;
        started_at: DateTime;
    }

    state Responded {
        request: Request;
        response: Response;
    }

    procedure receive@Idle(req: Request) -> @Processing
        needs time.read;
    {
        Processing {
            request: req,
            started_at: DateTime.now(),
        }
    }

    procedure respond@Processing(res: Response) -> @Responded {
        Responded {
            request: self.request,
            response: res,
        }
    }

    procedure reset@Responded() -> @Idle {
        Idle { }
    }
}
```

#### 10.9.3 Multi-Stage Pipeline Pattern

**Pattern:** Linear progression through stages

```cantrip
modal Pipeline {
    //
    //   Raw --[validate]--> Validated --[transform]--> Transformed
    //                                                       |
    //                                                  [finalize]
    //                                                       |
    //                                                       v
    //                                                   Complete
    //

    state Raw { data: Vec<u8>; }
    state Validated { data: Vec<u8>; schema: Schema; }
    state Transformed { data: ProcessedData; }
    state Complete { result: Output; }

    procedure validate@Raw(schema: Schema) -> @Validated
        needs alloc.heap;
    {
        validate_data(self.data, schema)?;
        Validated {
            data: self.data,
            schema,
        }
    }

    procedure transform@Validated() -> @Transformed
        needs alloc.heap;
    {
        let processed = process_data(self.data)?;
        Transformed { data: processed }
    }

    procedure finalize@Transformed() -> @Complete
        needs alloc.heap;
    {
        let output = generate_output(self.data)?;
        Complete { result: output }
    }
}
```

#### 10.9.4 State Recovery Pattern

**Pattern:** Multiple failure states with recovery paths

```cantrip
modal ResilientService {
    //
    //         [start]
    //            |
    //            v
    //        Running --[fail]--> Failed
    //            ^                  |
    //            |                  |
    //            |             [diagnose]
    //            |                  |
    //            |                  v
    //            |            Diagnosing --[recover]?--> Recovering
    //            |                  |                         |
    //            |                  |                         |
    //            |              [abort]                       |
    //            |                  |                         |
    //            |                  v                         |
    //            |               Stopped                      |
    //            |                                            |
    //            +----------------------[restart]-------------+
    //

    state Running { service: Service; }
    state Failed { service: Service; error: Error; }
    state Diagnosing { service: Service; diagnostics: Diagnostics; }
    state Recovering { service: Service; recovery_plan: RecoveryPlan; }
    state Stopped { }

    procedure fail@Running(err: Error) -> @Failed {
        Failed {
            service: self.service,
            error: err,
        }
    }

    procedure diagnose@Failed() -> @Diagnosing
        needs alloc.heap;
    {
        let diagnostics = run_diagnostics(self.service, self.error)?;
        Diagnosing {
            service: self.service,
            diagnostics,
        }
    }

    procedure recover@Diagnosing() -> @Recovering
        needs alloc.heap;
    {
        let plan = create_recovery_plan(self.diagnostics)?;
        Recovering {
            service: self.service,
            recovery_plan: plan,
        }
    }

    procedure restart@Recovering() -> @Running
        needs alloc.heap;
    {
        let recovered_service = execute_recovery(
            self.service,
            self.recovery_plan
        )?;
        Running { service: recovered_service }
    }

    procedure abort@Diagnosing() -> @Stopped {
        self.service.shutdown();
        Stopped { }
    }
}
```

---

### 10.10 Modal Instantiation

**Creating initial state:**
```cantrip
modal File {
    state Closed { path: String; }
    state Open { ... }

    // Constructor creates initial state
    function new(path: String): File@Closed {
    File@Closed { path }
    }
}

// Usage
let file = File.new("data.txt");  // Type: File@Closed
```

---
## 11. Modal Formal Semantics

### 11.1 Modal Definition

**Definition 11.1 (Modal Formal Structure):**

```
modal P {
    state S₁ { f₁₁: T₁₁; ...; f₁ₙ: T₁ₙ }
    ...
    state Sₖ { fₖ₁: Tₖ₁; ...; fₖₘ: Tₖₘ }

    procedure mⱼ@Sᵢ(params) -> @Sₗ { body }
}
```

**Formal components:**
- States: @S = {@S₁, ..., @Sₖ}
- Transitions: T = {(@Sᵢ, m, @Sⱼ) | transition exists}
- State data: Fields(@S) = {f : T | f declared in @S}

### 11.2 State Machine Graph

**Definition 11.2 (State Graph):**

```
G(P) = (V, E) where:
  V = {@S₁, ..., @Sₖ}
  E = {(@Sᵢ, m, @Sⱼ) | procedure m@Sᵢ -> @Sⱼ in P}
```

**Properties:**
1. **Reachability:** All states must be reachable from initial state
2. **Determinism:** At most one transition per (state, procedure) pair
3. **Totality:** Not required - some procedures may not be available in some states

```
File state graph:
  Closed --[open]--> Open
  Open --[read]--> Open
  Open --[close]--> Closed
### 11.3 State Type Rules

**[T-Modal-Type] Modal as type:**
```
modal P { state S₁ {...}; ...; state Sₙ {...}; ... } well-formed
────────────────────────────────────────────────────────────────────
Γ ⊢ modal P : Type
```

**[T-State-Annot] State annotation:**
```
Γ ⊢ e : modal P    Σ ⊢ P has state @S    σ ⊨ invariant(@S)
─────────────────────────────────────────────────────────────
Γ, Σ ⊢ e : P@S
```

**[T-State-Trans] State transition:**
```
Γ, Σ ⊢ e : P@S₁
Σ ⊢ procedure m@S₁(args) -> @S₂
needs(m) = ε    requires(m) = P    ensures(m) = Q
σ ⊨ P ∧ invariant(@S₁)
───────────────────────────────────────────────────────────
Γ, Σ ⊢ e.m(args) : P@S₂ ! ε
σ' ⊨ Q ∧ invariant(@S₂)
```

**[T-State-Procedure] Procedure invocation:**
```
Γ ⊢ self : P@S₁    procedure m@S₁ -> @S₂ in modal P
──────────────────────────────────────────────────────────
Γ ⊢ self.m(...) : P@S₂
```

**[T-State-Union] State union:**
```
Γ ⊢ e : P@S₁    @S₁ ⊆ (@S₂ | @S₃)
──────────────────────────────────
Γ ⊢ e : P@(@S₂ | @S₃)
```

### 11.4 State Invariants

**Definition 11.3 (State Invariant):**

Each modal state @S has an invariant I(@S):
```
modal P {
    state S
        must P₁;
        must P₂;
        ...
        must Pₙ;
}

I(@S) = P₁ ∧ P₂ ∧ ... ∧ Pₙ
```

**Axiom 11.1 (State Exclusivity):**

For all distinct states @S₁, @S₂ of modal P:
```
I(@S₁) ∧ I(@S₂) = false
```

States must be mutually exclusive.

**Axiom 11.2 (State Completeness):**

For all modals P with states @S₁, ..., @Sₙ:
```
I(@S₁) ∨ I(@S₂) ∨ ... ∨ I(@Sₙ) = true
```

Every valid instance must be in exactly one state.

**Example with invariants:**
```cantrip
modal BankAccount {
    state Active {
        balance: f64;
        account_id: u64;

        must balance >= 0.0;
        must account_id > 0;
    }

    state Frozen {
        balance: f64;
        account_id: u64;
        reason: String;

        must !reason.is_empty();
        must account_id > 0;
    }

    state Closed {
        account_id: u64;
        final_balance: f64;
        closed_at: DateTime;

        must final_balance == 0.0;  // Must be zero to close
    }
}
```

### 11.5 Transition Validity

**Definition 11.4 (Valid Transition):**

A transition procedure m@S₁ -> @S₂ is valid if:
1. Procedure m exists in modal
2. Pre-state @S₁ exists in modal
3. Post-state @S₂ exists in modal
4. Transition preserves invariants

**Formal rule:**
```
[Valid-Transition]
(@S₁, m, @S₂) ∈ Transitions(P)
σ ⊨ I(@S₁)
⟨m(σ), σ⟩ ⇓ ⟨v, σ'⟩
σ' ⊨ I(@S₂)
────────────────────────────────
@S₁ →ₘ @S₂ valid
```

### 11.6 Reachability Analysis

**Algorithm 11.1 (Reachability):**

```
reach(P, @S) = {@S' | ∃ path in G(P) from initial state to @S'}
```

**Compiler verification:**
1. All declared states must be reachable
2. Dead states generate warnings
3. Unreachable transitions generate errors

```cantrip
modal Example {
    state Start { }
    state Middle { }
    state End { }
    state Unreachable { }  // WARNING: Unreachable state

    procedure go@Start() -> @Middle
    procedure finish@Middle() -> @End
    // No path to Unreachable!
}
```

---

## 12. State Transition Verification

### 12.1 Static Verification

**Verification goal:** Prove that all state transitions preserve invariants.

**Verification condition for transition:**
```
[VC-Transition]
modal P {
    procedure m@S₁(x: T) -> @S₂
        requires P;
        ensures Q;
    { e }
}

Verification Condition:
∀σ. σ ⊨ P ∧ I(@S₁) ⟹
  ∃σ', v. ⟨e, σ⟩ ⇓ ⟨v, σ'⟩ ∧ σ' ⊨ Q ∧ I(@S₂)
```

```cantrip
modal BankAccount {
    state Active {
        balance: f64;
        must balance >= 0.0;
    }

    procedure withdraw@Active(amount: f64) -> @Active
        requires
            amount > 0.0;
            self.balance >= amount;
        ensures
            self.balance == @old(self.balance) - amount;
            self.balance >= 0.0;  // Maintains invariant
    {
        self.balance -= amount;
        self
    }
}
```

**Verification steps:**
1. Check preconditions imply invariant of pre-state
2. Verify implementation satisfies postconditions
3. Check postconditions imply invariant of post-state

### 12.2 Dynamic Verification

**Runtime checks:**
- Preconditions checked at procedure entry
- Postconditions checked at procedure exit
- State invariants checked after transitions

**Generated code:**
```cantrip
// Source
procedure withdraw@Active(amount: f64) -> @Active { ... }

// Generated
function withdraw(self: BankAccount@Active, amount: f64) {
    // Check preconditions
    assert(amount > 0.0, "Precondition: amount > 0.0");
    assert(self.balance >= amount, "Precondition: balance >= amount");

    let old_balance = self.balance;

    // Execute transition
    self.balance -= amount;

    // Check postconditions
    assert(self.balance == old_balance - amount, "Postcondition");
    assert(self.balance >= 0.0, "Invariant: balance >= 0.0");

    self
}
```

### 12.3 Exhaustiveness Checking

**Definition 12.1 (Procedure Exhaustiveness):** For a given state, the compiler checks which procedures are available.

```cantrip
modal File {
    state Closed { ... }
    state Open { ... }

    procedure open@Closed() -> @Open
    procedure read@Open() -> @Open
    procedure close@Open() -> @Closed
}

function process(file: File@Closed) {
    // file.read();  // ERROR: read() not available in Closed state
    let file = file.open();
    file.read();  // OK: read() available in Open state
}
```

**Type checking algorithm:**
```
check_method_call(obj: P@S, procedure: m) = {
    transitions = {(@S, m, @S') | procedure m@S -> @S' in P}
    if transitions.is_empty() {
        error("Procedure {m} not available in state {@S}")
    }
    return result_state(transitions)
}
```

### 12.4 State Flow Analysis

**Definition 12.2 (Flow-Sensitive Types):** Modal state types are flow-sensitive - the type changes as the program executes.

```cantrip
function example(): Result<(), Error>
    needs fs.read;
{
    let file = File.new("data.txt");  // Type: File@Closed

    let file = file.open()?;          // Type: File@Open

    let data = file.read(100)?;       // Type: File@Open (still)

    let file = file.close();          // Type: File@Closed

    // file.read(100)?;  // ERROR: Type is File@Closed

        Ok(())
}
**Flow analysis:**
```
Point 1: file : File@Closed
Point 2: file : File@Open   (after open())
Point 3: file : File@Open   (read preserves state)
Point 4: file : File@Closed (after close())
```

### 12.5 State Transfer Checking

**Definition 12.3 (Linear State Transfer):** When a transition occurs, the old state is consumed and the new state is produced.

**Type rule:**
```
[Linear-Transition]
Γ ⊢ obj : P@S₁
Γ ⊢ obj.m() : P@S₂
──────────────────────────────
Γ \ {obj : P@S₁} ∪ {obj : P@S₂}
```

```cantrip
let file = File.new("data.txt");  // file : File@Closed

// After this call, file : File@Open
let file = file.open()?;

// Cannot use old binding - consumed by transition
// The name 'file' now refers to the Open state
**Rebinding semantics:**
```cantrip
let file = file.open()?;
// Equivalent to:
// let file_closed = file;
// let file = transition(file_closed, Open);
// file_closed is no longer accessible
```

---

# Part IV: Functions and Expressions

## 13. Functions and Procedures

### 13.1 Function Definition Syntax

**Function signature with optional contract clauses:**

```cantrip
function name(param: Type): ReturnType
    needs effects;        // Optional if pure
    requires contracts;   // Optional if none
    ensures contracts;    // Optional if none
{
    body
}
```

**Components:**
1. **Signature** - `function name(params): ReturnType`
2. **`needs`** - Effect declarations (optional if pure)
3. **`requires`** - Preconditions (optional if none)
4. **`ensures`** - Postconditions (optional if none)
5. **Body** - Implementation in `{ }`

### 13.2 Pure Functions

**Definition 13.1 (Pure Function):** A function with no side effects.

**Syntax:**
```cantrip
function name(params): T {  // Pure, no needs/requires/ensures
    expr
}

// With postconditions:
function name(params): T
    ensures postconditions;
{
    expr
}
```

```cantrip
function add(a: i32, b: i32): i32 {
    a + b
}

function max(a: i32, b: i32): i32
    ensures
        result >= a;
        result >= b;
        (result == a) || (result == b);
{
    if a > b { a } else { b }
}
**Type rule:**
```
[T-Pure-Fun]
Γ, x₁ : T₁, ..., xₙ : Tₙ ⊢ e : U
effects(e) = ∅
─────────────────────────────────────
Γ ⊢ function f(x₁: T₁, ..., xₙ: Tₙ): U {
    e
} : T₁ -> ... -> Tₙ -> U
```

### 13.3 Functions with Effects

**Single effect:**
```cantrip
function print_message(msg: String)
    needs io.write;
{
    std.io.println(msg);
}
```

**Multiple effects:**
```cantrip
function save_config(path: String, data: Config): Result<(), Error>
    needs fs.write, alloc.heap;
    requires !path.is_empty();
{
    std.fs.write(path, data.serialize())
}
```

**Type rule:**
```
[T-Effect-Fun]
Γ, x : T ⊢ e : U ! ε
ε ⊆ declared_effects
─────────────────────────────────────
Γ ⊢ function f(x: T): U
    needs ε;
{
    e
} : fn(T): U ! ε
```

**Critical:** Effects are NEVER inferred. All must be declared explicitly.

### 13.4 Functions with Contracts

**Simple contract:**
```cantrip
function divide(a: f64, b: f64): f64
    requires b != 0.0;
{
    a / b
}
```

**Multiple preconditions:**
```cantrip
function withdraw(account: mut Account, amount: f64)
    requires
        amount > 0.0;
        account.balance >= amount;
{
    account.balance -= amount;
}
```

**Postconditions:**
```cantrip
function sqrt(x: f64): f64
    requires x >= 0.0;
    ensures
        result >= 0.0;
        result * result ≈ x;
{
    x.sqrt()
}
```

**Type rule:**
```
[T-Contract-Fun]
Γ ⊢ P : Assertion    Γ, x : T ⊢ e : U    Γ ⊢ Q : Assertion
{P} e {Q} provable or runtime-checked
──────────────────────────────────────────────────────────
Γ ⊢ function f(x: T): U
    requires P;
    ensures Q;
{
    e
} : T -> U with {P} -> {Q}
```

### 13.5 Parameters and Permissions

#### 13.5.1 Immutable Reference (Default)

**Default parameters are immutable references** (like C++ `const&`):

```cantrip
function process(data: String)
    requires !data.is_empty();
{
    std.io.println(data);  // Can read
    // data.clear();  // ERROR: Cannot mutate
}
```

**Multiple calls allowed (no borrow checking):**
```cantrip
function example()
    needs alloc.heap;
{
    let data = String.from("hello");

        // Can pass many times - no borrow checking
        process(data);
        process(data);
        process(data);
}
```

**Type rule:**
```
[T-Param-Ref]
Γ ⊢ data : String
──────────────────────────────────
function f(data: String) accepts data
```

#### 13.5.2 Owned Parameters

**Take ownership with `own`** (like C++ `unique_ptr`):

```cantrip
function consume(own data: String) {
    // data is consumed
    // Automatically freed at end of scope
}
```

**Must use `move` to transfer:**
```cantrip
function example()
    needs alloc.heap;
{
    let data = String.from("hello");
        consume(move data);
        // data no longer accessible
        // consume(move data);  // ERROR: value moved
}
```

**Type rule:**
```
[T-Param-Own]
Γ ⊢ e : own T
──────────────────────────────────
function f(own x: T) accepts move e
Γ' = Γ \ {e}  (e consumed)
```

#### 13.5.3 Mutable Parameters

**Exclusive mutable access with `mut`** (like C++ `&`):

```cantrip
function increment(counter: mut Counter) {
    counter.value += 1;
}
```

**Multiple mutable calls allowed (programmer's responsibility):**
```cantrip
function example()
    needs alloc.heap;
{
    var data = Data.new();

        // NO borrow checking - all allowed simultaneously
        modify1(mut data);
        modify2(mut data);
        read(data);
        modify3(mut data);

        // Programmer ensures this is safe!
}
```

**Type rule:**
```
[T-Param-Mut]
Γ ⊢ e : T    mutable(e) = true
──────────────────────────────────
function f(e: mut T) accepts mut e
```

**Key difference from Rust:** No borrow checker enforcing aliasing rules. Multiple `mut` references can coexist - programmer ensures correct usage.

### 13.6 Return Values

**Explicit return:**
```cantrip
function max(a: i32, b: i32): i32 {
    if a > b {
            a
        } else {
            b
        }
}
```

**Early return:**
```cantrip
function find(arr: [i32], target: i32): Option<usize> {
    for i in 0..arr.length() {
        if arr[i] == target {
            return Some(i);
        }
    }
    None
}
```

**Void return:**
```cantrip
function log_message(msg: String)  // Void/unit return
    needs io.write;
{
    std.io.println(msg);
}
```

**Type rule:**
```
[T-Return]
Γ ⊢ e : T
function f returns<U>
T = U
─────────────────────
return e : !
```

### 13.7 Procedure Syntax

**Procedures are functions with `self` parameter:**

```cantrip
impl Counter {
    function new(): own Counter
        needs alloc.heap;
    {
        own Counter { value: 0 }
    }

    function increment(self: mut Counter) {
        self.value += 1;
    }

    function get(self: Counter): i32 {
        self.value
    }
}
```

**Procedure call desugaring:**
```
obj.procedure(args) ≡ Type::procedure(obj, args)
```

```cantrip
var counter = Counter.new();
counter.increment();
let value = counter.get();
---

## 14. Expressions and Operators

### 14.1 Expression Language

**Definition 14.1 (Expression):** An expression is a syntactic construct that evaluates to a value.

**Type rule for expressions:**
```
Γ ⊢ e : T
```

**Evaluation semantics:**
```
⟨e, σ⟩ ⇓ ⟨v, σ'⟩
```

### 14.2 Arithmetic Operators

**Binary operators:**
```
+   // Addition
-   // Subtraction
*   // Multiplication
/   // Division
%   // Remainder (modulo)
```

**Type rules:**
```
[T-Add]
Γ ⊢ e₁ : T    Γ ⊢ e₂ : T    T : Numeric
─────────────────────────────────────────
Γ ⊢ e₁ + e₂ : T

[T-Div]
Γ ⊢ e₁ : T    Γ ⊢ e₂ : T    T : Numeric
─────────────────────────────────────────
Γ ⊢ e₁ / e₂ : T
```

**Operational semantics:**
```
[E-Add]
⟨e₁, σ⟩ ⇓ ⟨n₁, σ₁⟩    ⟨e₂, σ₁⟩ ⇓ ⟨n₂, σ₂⟩
────────────────────────────────────────
⟨e₁ + e₂, σ⟩ ⇓ ⟨n₁ + n₂, σ₂⟩
```

```cantrip
let sum = a + b;
let product = x * y;
let remainder = n % 10;
### 14.3 Comparison Operators

**Operators:**
```
==  // Equal
!=  // Not equal
<   // Less than
<=  // Less than or equal
>   // Greater than
>=  // Greater than or equal
```

**Type rule:**
```
[T-Eq]
Γ ⊢ e₁ : T    Γ ⊢ e₂ : T    T : Eq
───────────────────────────────────
Γ ⊢ e₁ == e₂ : bool

[T-Ord]
Γ ⊢ e₁ : T    Γ ⊢ e₂ : T    T : Ord
───────────────────────────────────
Γ ⊢ e₁ < e₂ : bool
```

**Operational semantics:**
```
[E-Eq]
⟨e₁, σ⟩ ⇓ ⟨v₁, σ₁⟩    ⟨e₂, σ₁⟩ ⇓ ⟨v₂, σ₂⟩
────────────────────────────────────────
⟨e₁ == e₂, σ⟩ ⇓ ⟨v₁ = v₂, σ₂⟩
```

### 14.4 Logical Operators

**Operators:**
```
&&  // Logical AND (short-circuit)
||  // Logical OR (short-circuit)
!   // Logical NOT
```

**Type rules:**
```
[T-And]
Γ ⊢ e₁ : bool    Γ ⊢ e₂ : bool
──────────────────────────────
Γ ⊢ e₁ && e₂ : bool

[T-Not]
Γ ⊢ e : bool
────────────
Γ ⊢ !e : bool
```

**Operational semantics (short-circuit):**
```
[E-And-True]
⟨e₁, σ⟩ ⇓ ⟨true, σ₁⟩    ⟨e₂, σ₁⟩ ⇓ ⟨v, σ₂⟩
────────────────────────────────────────
⟨e₁ && e₂, σ⟩ ⇓ ⟨v, σ₂⟩

[E-And-False]
⟨e₁, σ⟩ ⇓ ⟨false, σ₁⟩
────────────────────────────
⟨e₁ && e₂, σ⟩ ⇓ ⟨false, σ₁⟩
```

```cantrip
let valid = x > 0 && x < 100;
let has_value = opt.is_some() || use_default;
let inverted = !flag;
### 14.5 Bitwise Operators

**Operators:**
```
&   // Bitwise AND
|   // Bitwise OR
^   // Bitwise XOR
<<  // Left shift
>>  // Right shift (arithmetic)
```

**Type rules:**
```
[T-BitAnd]
Γ ⊢ e₁ : T    Γ ⊢ e₂ : T    T : Integer
─────────────────────────────────────────
Γ ⊢ e₁ & e₂ : T

[T-Shift]
Γ ⊢ e₁ : T    Γ ⊢ e₂ : u32    T : Integer
─────────────────────────────────────────
Γ ⊢ e₁ << e₂ : T
```

```cantrip
let masked = value & 0xFF;
let combined = flags | new_flags;
let shifted = n << 2;  // Multiply by 4
### 14.6 Assignment Operators

**Operators:**
```
=   // Assignment
+=  // Add and assign
-=  // Subtract and assign
*=  // Multiply and assign
/=  // Divide and assign
%=  // Modulo and assign
```

**Type rules:**
```
[T-Assign]
Γ ⊢ x : T    Γ ⊢ e : T    mutable(x) = true
────────────────────────────────────────────
Γ ⊢ x = e : ()

[T-Add-Assign]
Γ ⊢ x : T    Γ ⊢ e : T    mutable(x) = true    T : Numeric
──────────────────────────────────────────────────────────
Γ ⊢ x += e : ()
```

**Desugaring:**
```
x += e  ≡  x = x + e
x -= e  ≡  x = x - e
x *= e  ≡  x = x * e
```

### 14.7 Range Operators

**Operators:**
```
..   // Exclusive range: a..b → [a, b)
..=  // Inclusive range: a..=b → [a, b]
```

**Type rules:**
```
[T-Range]
Γ ⊢ e₁ : T    Γ ⊢ e₂ : T    T : Integer
─────────────────────────────────────────
Γ ⊢ e₁..e₂ : Range<T>

[T-Range-Inclusive]
Γ ⊢ e₁ : T    Γ ⊢ e₂ : T    T : Integer
─────────────────────────────────────────
Γ ⊢ e₁..=e₂ : RangeInclusive<T>
```

```cantrip
for i in 0..10 {  // 0, 1, 2, ..., 9
    println(i);
}

for i in 0..=10 {  // 0, 1, 2, ..., 10
    println(i);
}

let slice = arr[2..5];  // Elements at indices 2, 3, 4
### 14.8 Pipeline Operator

**The pipeline operator `=>` chains function calls:**

```cantrip
function process(data: Vec<i32>): i32 {
    data
            => filter(|x| x > 0)
            => map(|x| x * 2)
            => sum
}
```

**Desugaring:**
```
e₁ => e₂ => e₃  ≡  e₃(e₂(e₁))
```

**Type rule:**
```
[T-Pipeline]
Γ ⊢ e₁ : T    Γ ⊢ e₂ : T -> U
──────────────────────────────
Γ ⊢ e₁ => e₂ : U
```

**Operational semantics:**
```
[E-Pipeline]
⟨e₁, σ⟩ ⇓ ⟨v₁, σ₁⟩    ⟨e₂(v₁), σ₁⟩ ⇓ ⟨v₂, σ₂⟩
───────────────────────────────────────────────
⟨e₁ => e₂, σ⟩ ⇓ ⟨v₂, σ₂⟩
```

**With effects:**
```cantrip
function load_and_process(path: str): Result<Data, Error>
    needs fs.read, alloc.heap;
{
    read_file(path)
        => parse_json
        => validate
        => transform
}
```

### 14.9 Operator Precedence

**From highest to lowest:**

| Precedence | Operators | Associativity |
|------------|-----------|---------------|
| 1 (Highest) | `!`, `-` (unary), `*` (deref) | Right |
| 2 | `*`, `/`, `%` | Left |
| 3 | `+`, `-` | Left |
| 4 | `<<`, `>>` | Left |
| 5 | `&` | Left |
| 6 | `^` | Left |
| 7 | `|` | Left |
| 8 | `==`, `!=`, `<`, `<=`, `>`, `>=` | Left |
| 9 | `&&` | Left |
| 10 | `||` | Left |
| 11 | `=>` | Left |
| 12 (Lowest) | `=`, `+=`, `-=`, etc. | Right |

```cantrip
a + b * c       ≡  a + (b * c)
a && b || c     ≡  (a && b) || c
x = y = z       ≡  x = (y = z)
a => f => g     ≡  g(f(a))
---

### 14.10 Typed Holes

**Goal:** Improve developer flow and LLM-assisted coding by allowing *incomplete* expressions that type-check
and produce precise diagnostics and fix-its, while preserving safety and AOT compilation.

**Syntax (expressions):**
```ebnf
ExprHole      ::= "?" (Ident)? ( ":" Type )?
UntypedHole   ::= "???"
```
- `?` introduces a *typed hole*. The optional `: Type` annotation constrains its type.
  Without an explicit `Type`, the hole participates in normal type inference.
- `???` is a hard untyped hole: it has type `!` and cannot be used except in positions
  that are already diverging.

**Typing:**
```
[T-Hole]
Γ ⊢ ?x : T  (fresh metavariable α; if annotation present, α = T)
Emits obligation: "hole(?x) : α must be defined"
```

**Elaboration and build modes:**
- **Verify/static:** `E9501` — *Unfilled typed hole*. Compilation fails. Fix-it includes a skeleton snippet.
- **Verify/runtime (default dev):** A typed hole elaborates to a trap of type `T` that panics at runtime
  (`needs panic`) with a precise message and source span. `E9502` (warning) is emitted.
- **Verify/none:** Typed holes are denied (`E9501`).

**Effects:** Filling a hole MUST NOT widen declared effects. A hole in a function with `needs ε`
is considered to have effects ⊆ `ε` until filled; otherwise `E9503` is raised.

```cantrip
function parse(line: String): Result<Item, Error>
    needs alloc.heap;
{
    let name = ?n: String;          // typed hole requiring a String
    let qty  = ?;                   // hole with inferred type
    ???                              // hard hole: diverges here
}
**Machine-Readable Output:** See §55 for the `typedHole` field in diagnostics.

## 15. Control Flow

### 15.1 If Expressions

**Syntax:**
```cantrip
if condition {
    // then branch
}

if condition {
    // then branch
} else {
    // else branch
}
```

**Type rule:**
```
[T-If]
Γ ⊢ e₁ : bool    Γ ⊢ e₂ : T    Γ ⊢ e₃ : T
────────────────────────────────────────────
Γ ⊢ if e₁ then e₂ else e₃ : T
```

**Operational semantics:**
```
[E-If-True]
⟨e₁, σ⟩ ⇓ ⟨true, σ₁⟩    ⟨e₂, σ₁⟩ ⇓ ⟨v, σ₂⟩
────────────────────────────────────────────
⟨if e₁ then e₂ else e₃, σ⟩ ⇓ ⟨v, σ₂⟩

[E-If-False]
⟨e₁, σ⟩ ⇓ ⟨false, σ₁⟩    ⟨e₃, σ₁⟩ ⇓ ⟨v, σ₂⟩
────────────────────────────────────────────
⟨if e₁ then e₂ else e₃, σ⟩ ⇓ ⟨v, σ₂⟩
```

**If as expression:**
```cantrip
let max = if a > b { a } else { b };
let sign = if x > 0 { 1 } else if x < 0 { -1 } else { 0 };
```

**Type requirement:** Both branches must have same type (or one is `!`)

### 15.2 While Loops

**Syntax:**
```cantrip
while condition {
    // loop body
}
```

**Type rule:**
```
[T-While]
Γ ⊢ e₁ : bool    Γ ⊢ e₂ : ()
──────────────────────────────
Γ ⊢ while e₁ do e₂ : ()
```

**With invariant:**
```cantrip
var i = 0;
while i < n {
    invariant:
        i >= 0;
        i <= n;

    i += 1;
}
```

**Loop invariant verification:**
```
[VC-While]
{I ∧ condition} body {I}
────────────────────────────────────────
{I} while condition { invariant: I; body } {I ∧ ¬condition}
```

**Operational semantics:**
```
[E-While-True]
⟨e₁, σ⟩ ⇓ ⟨true, σ₁⟩    ⟨e₂, σ₁⟩ ⇓ ⟨(), σ₂⟩
⟨while e₁ do e₂, σ₂⟩ ⇓ ⟨(), σ₃⟩
────────────────────────────────────────────
⟨while e₁ do e₂, σ⟩ ⇓ ⟨(), σ₃⟩

[E-While-False]
⟨e₁, σ⟩ ⇓ ⟨false, σ₁⟩
─────────────────────────────
⟨while e₁ do e₂, σ⟩ ⇓ ⟨(), σ₁⟩
```

### 15.3 For Loops

**Range iteration:**
```cantrip
for i in 0..10 {
    std.io.println(i);
}
```

**Desugaring:**
```cantrip
// for i in range { body }
// Desugars to:
{
    var iter = range.into_iter();
    loop {
        match iter.next() {
            Some(i) -> body,
            None -> break,
        }
    }
}
```

**Collection iteration:**
```cantrip
for item in collection {
    process(item);
}
```

**With index:**
```cantrip
for i, item in collection.enumerate() {
    std.io.println("Item {i}: {item}");
}
```

**Type rule:**
```
[T-For]
Γ ⊢ e₁ : T    T : Iterator<Item = U>
Γ, x : U ⊢ e₂ : ()
─────────────────────────────────────
Γ ⊢ for x in e₁ { e₂ } : ()
```

### 15.4 Loop Control

**Infinite loop:**
```cantrip
loop {
    // infinite loop
    if should_exit {
        break;
    }
}
```

**Break with value:**
```cantrip
let result = loop {
    if condition {
        break value;
    }
};
```

**Continue:**
```cantrip
for i in 0..10 {
    if i % 2 == 0 {
        continue;  // Skip even numbers
    }
    println(i);
}
```

**Type rules:**
```
[T-Loop]
Γ ⊢ e : T
─────────────────────
Γ ⊢ loop { e } : !

[T-Break]
Γ ⊢ e : T
in_loop with result type T
─────────────────────────────
Γ ⊢ break e : !

[T-Continue]
in_loop
────────────────
Γ ⊢ continue : !
```

---

## 16. Higher-Order Functions and Closures

### 16.1 Function Types (fn) and Procedure Types (proc)

**Definition 16.1 (Function Type):** The type `fn(T): U` represents functions from `T` to `U`.

**Type constructor:**
```
->  (function type constructor)
```

```cantrip
type Handler = fn(Request): Response;
type BinaryOp = fn(i32, i32): i32;
type Transform = fn(String): Result<Data, Error>;
**Currying:**
```
i32 -> i32 -> i32  ≡  i32 -> (i32 -> i32)
```

### 16.2 Higher-Order Functions

**Definition 16.2 (Higher-Order Function):** A function that takes functions as parameters or returns functions.

```cantrip
function apply_twice<T>(value: T, f: T -> T): T {
    f(f(value))
}

function filter<T>(items: [T], pred: T -> bool): Vec<T>
    needs alloc.heap;
{
    var result = Vec.new();
    for item in items {
        if pred(item) {
            result.push(item);
        }
    }
    result
}

function compose<A, B, C>(f: fn(A): B, g: fn(B): C): fn(A): C {
    |x: A| { g(f(x)) }
}
**Type rule:**
```
[T-Higher-Order]
Γ ⊢ f : T -> U    Γ ⊢ g : U -> V
──────────────────────────────────
compose(f, g) : T -> V
```

### 16.3 Closures

**Definition 16.3 (Closure):** An anonymous function that captures its environment.

**Syntax:**
```cantrip
|param| { body }
|param: Type|: RetType { body }
move |param| { body }  // Move captured variables
```

```cantrip
let add_n = |x| { x + n };  // Captures n

// Explicit types
let multiply: fn(i32, i32): i32 = |a, b| { a * b };

// Move semantics
let closure = move |x| { x + captured_value };
```

**Type rule:**
```
[T-Closure]
Γ, x : T ⊢ e : U    free_vars(e) ⊆ dom(Γ)
───────────────────────────────────────────
Γ ⊢ |x: T| { e } : T -> U
```

**Capture semantics:**
```
[Capture-Ref]
x : T ∈ Γ
────────────────────────
closure captures x by reference

[Capture-Move]
x : T ∈ Γ
────────────────────────
move closure captures x by value
Γ' = Γ \ {x}
```

**Example with state:**
```cantrip
function make_counter(): fn(): i32
    needs alloc.heap;
{
    var count = 0;
    move || {
        count += 1;
        count
    }
}

let counter = make_counter();
println(counter());  // 1
println(counter());  // 2
```

### 16.4 Function Traits

**Three closure traits:**

```cantrip
trait Fn<Args> {
    function call(self: Self, args: Args): Output;
}

trait FnMut<Args> {
    function call(self: mut Self, args: Args): Output;
}

trait FnOnce<Args> {
    function call(self: own Self, args: Args): Output;
}
```

**Hierarchy:**
```
Fn <: FnMut <: FnOnce
```

```cantrip
function apply<F>(f: F, x: i32): i32
    where F: Fn(i32): i32
{
    f.call(x)
}
### 16.5 Effect Propagation

**Use `effects(F)` to propagate callback effects:**

```cantrip
function map<T, U, F>(items: [T], mapper: F): Vec<U>
    where F: Fn(T): U
    needs effects(F), alloc.heap;
    requires items.length() > 0;
{
    var result = Vec.with_capacity(items.length());
    for item in items {
        result.push(mapper(item));
    }
    result
}
```

**Type rule:**
```
[T-Effect-Propagate]
Γ ⊢ f : fn(T): U ! ε₁
Γ ⊢ map : (T, T -> U) -> Vec<U> ! (ε₂, effects(f))
──────────────────────────────────────────────────
map(items, f) ! (ε₂ ∪ ε₁)
```

**Constraining callbacks:**
```cantrip
function parallel_map<T, U, F>(items: [T], mapper: F): Vec<U>
    where F: Fn(T): U
    needs effects(F), !time.read, !random, alloc.heap;
    requires items.length() > 0;
{
    std.parallel.map_n(items, 8)
}
```

# Part V: Contract System

## 17. Contracts and Specifications

### 17.1 Contract Overview

**Definition 17.1 (Contract):** A contract is an executable specification consisting of preconditions, postconditions, and invariants.

**Design goals:**
- Machine-verifiable specifications
- Contracts as first-class citizens
- Clear separation from implementation
- Support both static and dynamic verification

**Contract components:**
1. **Preconditions** - What must be true when function is called
2. **Postconditions** - What is guaranteed when function returns
3. **Invariants** - What is always true for a type

### 17.2 Preconditions

**Syntax:**
```cantrip
function name(params): Type
    needs effects;       // Optional
    requires
        assertion₁;
        assertion₂;
        ...;
```

**Pure preconditions (no effects):**
```cantrip
function divide(a: f64, b: f64): f64
    requires
        b != 0.0;
        a.is_finite();
        b.is_finite();
    ensures result.is_finite();
{
    a / b
}
```

**Preconditions with effects:**
```cantrip
function read_file(path: String): Result<String, Error>
    needs fs.read, alloc.heap;
    requires
        !path.is_empty();
        path.len() < 4096;
{
    std.fs.read_to_string(path)
}
```

**Type rule:**
```
[T-Requires]
Γ ⊢ P : Assertion    Γ ⊢ e : T ! ε
──────────────────────────────────────
Γ ⊢ function f(): T
    needs ε;
    requires P;
{
    e
} : T ! ε with precondition P
```

### 17.3 Postconditions

**Syntax:**
```cantrip
function name(params): Type
    ensures
        assertion₁;
        assertion₂;
        ...;
```

**Postconditions can reference:**
- `result`: The returned value
- `@old(expr)`: Value of expression on entry
- Parameters: Original parameter values

```cantrip
function push(self: mut Vec<T>, item: T)
    needs alloc.heap;
    requires self.length() < self.capacity();
    ensures
        self.length() == @old(self.length()) + 1;
        self[self.length() - 1] == item;
{
    self.data[self.length()] = item;
    self.length += 1;
}
**Complex postconditions:**
```cantrip
function sqrt(x: f64): f64
    requires x >= 0.0;
    ensures
        result >= 0.0;
        result * result ≈ x;  // Within floating-point precision
        abs(result * result - x) < 1e-10;
{
    x.sqrt()
}
```

**Type rule:**
```
[T-Ensures]
Γ ⊢ e : T    Γ, result : T ⊢ Q : Assertion
────────────────────────────────────────────
Γ ⊢ function f(): T
    ensures Q;
{
    e
} : T with postcondition Q
```

### 17.4 Contract Messages

**Custom error messages for contract violations:**

```cantrip
function withdraw(self: mut Account, amount: f64)
    requires
        amount > 0.0,
            "Withdrawal amount must be positive";
        self.balance >= amount,
            "Insufficient funds: need {amount}, have {self.balance}";
    ensures self.balance == @old(self.balance) - amount;
{
    self.balance -= amount;
}
```

**Syntax:**
```
assertion, "error message with {interpolation}"
```

**Runtime behavior:**
```cantrip
// If precondition fails:
panic("Contract violation in withdraw:
       Insufficient funds: need 100.0, have 50.0
       at src/account.cantrip:42")
```

### 17.5 Empty Contracts

**When there are no constraints:**

```cantrip
function add(a: i32, b: i32): i32 {
    a + b
    // No needs, requires, or ensures clauses - all optional
}

// Pure functions simply omit contract clauses
function identity(x: i32): i32 {
    x
}
```

### 17.6 Old-Value References

**Definition 17.2 (@old):** The `@old(expr)` construct captures the value of an expression at function entry.

**Syntax:**
```
@old(expression)
```

```cantrip
function transfer(from: mut Account, to: mut Account, amount: f64)
    requires
        amount > 0.0;
        from.balance >= amount;
    ensures
        from.balance == @old(from.balance) - amount;
        to.balance == @old(to.balance) + amount;
        @old(from.balance) + @old(to.balance) ==
            from.balance + to.balance;  // Conservation
{
    from.balance -= amount;
    to.balance += amount;
}
**Formal semantics:**
```
[Old-Value]
⟦@old(e)⟧σ = ⟦e⟧σ₀  where σ₀ is initial store
```

**Implementation:**
```
// Compiler generates:
function transfer(from: mut Account, to: mut Account, amount: f64) {
    // Capture old values
    let from_balance_old = from.balance;
    let to_balance_old = to.balance;

    // Execute
    from.balance -= amount;
    to.balance += amount;

    // Check postconditions
    assert(from.balance == from_balance_old - amount, "from balance updated correctly");
    assert(to.balance == to_balance_old + amount, "to balance updated correctly");
}
```

### 17.7 Result References

**The `result` identifier in postconditions refers to the return value:**

```cantrip
function factorial(n: u32): u64
    requires n <= 20;  // Prevent overflow
    ensures
        result >= 1;
        n == 0 implies result == 1;
        n > 0 implies result == n * factorial(n - 1);
{
    if n == 0 { 1 } else { n as u64 * factorial(n - 1) }
}
```

**For tuples:**
```cantrip
function divmod(a: i32, b: i32): (i32, i32)
    requires b != 0;
    ensures
        result.0 * b + result.1 == a;
        result.1 >= 0;
        result.1 < abs(b);
{
    (a / b, a % b)
}
```

### 17.9 Common Contract Patterns

#### 17.9.1 Bounds Checking Pattern

**Pattern:** Ensure indices are within valid ranges

```cantrip
function safe_index<T>(arr: [T], index: usize): Option<T>
    ensures
        result.is_some() == (index < arr.length());
        result.is_some() implies result.unwrap() == arr[index];
{
    if index < arr.length() {
        Some(arr[index])
    } else {
        None
    }
}

function get_or_default<T>(arr: [T], index: usize, default: T): T
    ensures
        (index < arr.length()) implies result == arr[index];
        (index >= arr.length()) implies result == default;
{
    if index < arr.length() {
        arr[index]
    } else {
        default
    }
}
```

#### 17.9.2 Collection Invariant Pattern

**Pattern:** Maintain collection properties across operations

```cantrip
function append<T>(left: Vec<T>, right: Vec<T>): Vec<T>
    ensures
        result.length() == left.length() + right.length();
        forall(i in 0..left.length()) {
            result[i] == left[i]
        };
        forall(i in 0..right.length()) {
            result[left.length() + i] == right[i]
        };
    needs alloc.heap;
{
    let mut combined = left.clone();
    for item in right {
        combined.push(item);
    }
    combined
}

function filter<T, F>(items: Vec<T>, predicate: F): Vec<T>
    where F: Fn(T): bool
    ensures
        result.length() <= items.length();
        forall(x in result) { predicate(x) };
    needs effects(F), alloc.heap;
{
    var filtered = Vec.new();
    for item in items {
        if predicate(item) {
            filtered.push(item);
        }
    }
    filtered
}
```

#### 17.9.3 State Consistency Pattern

**Pattern:** Ensure related fields stay synchronized

```cantrip
record CachedValue<T> {
    value: T;
    cached: Option<T>;
    dirty: bool;

    invariant:
        !dirty implies cached.is_some();
        cached.is_some() implies cached.unwrap() == value;
}

impl<T> CachedValue<T> where T: Clone {
    function update(self: mut CachedValue<T>, new_value: T)
        ensures
            self.value == new_value;
            self.dirty == true;
            self.cached.is_none();
    {
        self.value = new_value;
        self.dirty = true;
        self.cached = None;
    }

    function get_cached(self: mut CachedValue<T>): T
        ensures
            !self.dirty;
            self.cached.is_some();
            result == self.value;
        needs alloc.heap;
    {
        if self.dirty {
            self.cached = Some(self.value.clone());
            self.dirty = false;
        }
        self.cached.unwrap()
    }
}
```

#### 17.9.4 Resource Conservation Pattern

**Pattern:** Prove resources are neither created nor destroyed improperly

```cantrip
function transfer_funds(
    from: mut Account,
    to: mut Account,
    amount: f64
)
    requires
        amount > 0.0,
            "Transfer amount must be positive";
        from.balance >= amount,
            "Insufficient funds: need {amount}, have {from.balance}";
    ensures
        from.balance == @old(from.balance) - amount;
        to.balance == @old(to.balance) + amount;
        // Conservation law: total unchanged
        @old(from.balance + to.balance) == (from.balance + to.balance);
{
    from.balance -= amount;
    to.balance += amount;
}

function swap<T>(a: mut T, b: mut T)
    ensures
        *a == @old(*b);
        *b == @old(*a);
{
    let temp = *a;
    *a = *b;
    *b = temp;
}
```

#### 17.9.5 Ordering Preservation Pattern

**Pattern:** Maintain order relationships through transformations

```cantrip
function merge_sorted(left: Vec<i32>, right: Vec<i32>): Vec<i32>
    requires
        is_sorted(left);
        is_sorted(right);
    ensures
        is_sorted(result);
        result.length() == left.length() + right.length();
        forall(x in left) { result.contains(x) };
        forall(x in right) { result.contains(x) };
    needs alloc.heap;
{
    var merged = Vec.new();
    var i = 0;
    var j = 0;

    while i < left.length() && j < right.length() {
        if left[i] <= right[j] {
            merged.push(left[i]);
            i += 1;
        } else {
            merged.push(right[j]);
            j += 1;
        }
    }

    while i < left.length() {
        merged.push(left[i]);
        i += 1;
    }

    while j < right.length() {
        merged.push(right[j]);
        j += 1;
    }

    merged
}

function reverse<T>(arr: Vec<T>): Vec<T>
    ensures
        result.length() == arr.length();
        forall(i in 0..arr.length()) {
            result[i] == arr[arr.length() - 1 - i]
        };
    needs alloc.heap;
{
    var reversed = Vec.new();
    for i in (0..arr.length()).rev() {
        reversed.push(arr[i]);
    }
    reversed
}
```

#### 17.9.6 Error Handling Contract Pattern

**Pattern:** Specify when functions can fail and what they guarantee on success

```cantrip
function parse_int(s: String): Result<i32, ParseError>
    requires !s.is_empty();
    ensures
        result.is_ok() implies (
            result.unwrap() >= i32::MIN &&
            result.unwrap() <= i32::MAX
        );
        result.is_err() implies (
            !is_valid_integer(s)
        );
{
    match s.parse() {
        Ok(n) -> Ok(n),
        Err(_) -> Err(ParseError.InvalidFormat),
    }
}

function divide_checked(a: i32, b: i32): Result<i32, ArithmeticError>
    ensures
        result.is_ok() implies (
            b != 0 &&
            result.unwrap() == a / b
        );
        result.is_err() implies (
            b == 0
        );
{
    if b == 0 {
        Err(ArithmeticError.DivisionByZero)
    } else {
        Ok(a / b)
    }
}
```

---

## 18. Contract Formal Logic

### 18.1 Assertion Language

**Definition 18.1 (Assertion):** An assertion is a boolean-valued predicate over program state.

**Syntax:**
```
P, Q ::= none                               (trivial)
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
```

**Satisfaction relation:**
```
σ ⊨ P  (store σ satisfies assertion P)
```

**Satisfaction rules:**
```
[Sat-Eq]
⟦e₁⟧σ = ⟦e₂⟧σ
──────────────
σ ⊨ e₁ = e₂

[Sat-And]
σ ⊨ P    σ ⊨ Q
──────────────
σ ⊨ P ∧ Q

[Sat-Forall]
∀v. σ[x ↦ v] ⊨ P
─────────────────
σ ⊨ ∀x. P
```

### 18.2 Hoare Logic

**Definition 18.2 (Hoare Triple):** A Hoare triple {P} e {Q} states that if P holds before executing e, then Q holds after (if e terminates).

**Axioms and rules:**

**[H-Skip]**
```
──────────────
{P} skip {P}
```

**[H-Assign]**
```
──────────────────────────
{P[x ↦ e]} x = e {P}
```

**[H-Seq]**
```
{P} e₁ {Q}    {Q} e₂ {R}
─────────────────────────
{P} e₁ ; e₂ {R}
```

**[H-If]**
```
{P ∧ b} e₁ {Q}    {P ∧ ¬b} e₂ {Q}
──────────────────────────────────
{P} if b then e₁ else e₂ {Q}
```

**[H-While]**
```
{I ∧ b} e {I}
──────────────────────────────────
{I} while b do e {I ∧ ¬b}
```

**[H-Consequence]**
```
P' ⇒ P    {P} e {Q}    Q ⇒ Q'
─────────────────────────────
{P'} e {Q'}
```

### 18.3 Weakest Precondition

**Definition 18.3 (Weakest Precondition):** For command c and postcondition Q, the weakest precondition wp(c, Q) is the weakest assertion P such that {P} c {Q}.

**Weakest precondition calculus:**

```
wp(skip, Q) = Q
wp(x = e, Q) = Q[x ↦ e]
wp(c₁ ; c₂, Q) = wp(c₁, wp(c₂, Q))
wp(if b then c₁ else c₂, Q) = (b ⇒ wp(c₁, Q)) ∧ (¬b ⇒ wp(c₂, Q))
wp(while b do c, Q) = I where I is loop invariant
```

```cantrip
// Command: x = x + 1
// Postcondition: x > 0
// wp(x = x + 1, x > 0) = (x + 1 > 0) = (x > -1)

function increment(x: i32): i32
    requires x > -1;  // Weakest precondition
    ensures result > 0;
{
    x + 1
}
### 18.4 Verification Conditions

**Definition 18.4 (Verification Condition):** A verification condition (VC) is a logical formula that, if valid, proves correctness of a program with respect to its contracts.

**For function:**
```cantrip
function f(x: T): U
    requires P;
    ensures Q;
{
    e
}
```

**Verification condition:**
```
∀σ, x. σ ⊨ P ⟹ ∃σ', v. ⟨e, σ⟩ ⇓ ⟨v, σ'⟩ ∧ σ' ⊨ Q[result ↦ v]
```

**For state transition:**
```cantrip
modal P {
    procedure m@S₁(x: T) -> @S₂
        requires P;
        ensures Q;
    { e }
}
```

**Verification condition:**
```
∀σ. σ ⊨ P ∧ I(@S₁) ⟹
  ∃σ', v. ⟨e, σ⟩ ⇓ ⟨v, σ'⟩ ∧ σ' ⊨ Q ∧ I(@S₂)
```

### 18.5 Contract Soundness

**Theorem 18.1 (Contract Soundness):**

If a function f satisfies its contracts and f is called with arguments satisfying the precondition, then f's result satisfies the postcondition (if f terminates).

**Formal statement:**
```
function f(x: T): U
    requires P;
    ensures Q;
{
    e
}

∀v : T. ⊨ P[x ↦ v] ⟹
  ∃w : U. f(v) ⇓ w ∧ ⊨ Q[x ↦ v, result ↦ w]
```

**Proof obligation:** Must be verified for all functions with contracts.

---

## 19. Invariants

### 19.1 Record Invariants

**Definition 19.1 (Record Invariant):** An invariant is a predicate that must hold for all instances of a type at all observable points.

**Syntax:**
```cantrip
record Name {
    fields;

    invariant:
        assertion₁;
        assertion₂;
        ...
}
```

```cantrip
record BankAccount {
    balance: f64;

    invariant:
        balance >= 0.0;
}
**Enforcement points:**
1. After construction
2. Before and after public procedure calls
3. Not checked during private procedure execution

**Type rule:**
```
[T-Record-Inv]
record S { fields } with invariant I
Γ ⊢ e : S
─────────────────────────────
σ ⊨ I[self ↦ e]
```

### 19.2 Invariant Preservation

**Procedures must preserve invariants:**

```cantrip
impl BankAccount {
    function new(initial: f64): own BankAccount
        requires initial >= 0.0;
        ensures
            result.balance == initial;
            // Implicitly: result.balance >= 0.0 (invariant)
    {
        own BankAccount { balance: initial }
    }

    function withdraw(self: mut BankAccount, amount: f64)
        requires
            amount > 0.0;
            self.balance >= amount;
        ensures
            self.balance == @old(self.balance) - amount;
            // Implicitly checked: self.balance >= 0.0 (invariant)
    {
        self.balance -= amount;
    }
}
```

**Verification condition:**
```
[VC-Inv-Procedure]
record S with invariant I
procedure m(self: mut S, x: T)
    requires P;
    ensures Q;
{
    e
}

VC: ∀σ. σ ⊨ I ∧ P ⟹
      ∃σ'. ⟨e, σ⟩ ⇓ σ' ∧ σ' ⊨ I ∧ Q
```

### 19.3 Class Invariants

**Classes support invariants with inheritance:**

```cantrip
public record Counter {
    private value: i32;

    invariant:
        value >= 0;

    public function new(): own Counter
        ensures result.value == 0;
    {
        own Counter { value: 0 }
    }

    public function increment(self: mut Counter)
        ensures self.value == @old(self.value) + 1;
    {
        self.value += 1;
    }
}
```

**Composition-based bounded counter with invariants:**
```cantrip
public record BoundedCounter {
    counter: Counter;  // Composition pattern
    max_value: i32;

    invariant:
        counter.value >= 0;           // Maintains base invariant
        counter.value <= max_value;   // Additional constraint
        max_value > 0;

    public function increment(self: mut BoundedCounter)
        requires self.counter.value < self.max_value;
        ensures
            self.counter.value == @old(self.counter.value) + 1;
            self.counter.value <= self.max_value;
    {
        if self.counter.value < self.max_value {
            self.counter.increment();
        }
    }
}
```

### 19.4 Modal State Invariants

**Modal states can have invariants:**

```cantrip
modal Transaction {
    state Pending {
        operations: Vec<Operation>;

        must !operations.is_empty();
    }

    state Committed {
        commit_id: u64;
        timestamp: DateTime;

        must commit_id > 0;
        must timestamp.is_valid();
    }

    state RolledBack {
        error: Error;

        must !error.message.is_empty();
    }
}
```

**Checked at transition boundaries:**
```cantrip
procedure commit@Pending() -> @Committed {
    // Must establish Committed invariants
    let id = generate_commit_id();  // Must be > 0
    let ts = DateTime.now();        // Must be valid

    Committed {
        commit_id: id,
        timestamp: ts,
    }  // Invariants automatically verified here
}
```

### 19.5 Loop Invariants

**Loop invariants specify properties maintained across iterations:**

```cantrip
function sum_array(arr: [i32]): i32 {
    var sum = 0;
        var i = 0;

        while i < arr.length() {
            invariant:
                0 <= i;
                i <= arr.length();
                sum == sum_of(arr[0..i]);

            sum += arr[i];
            i += 1;
        }

        sum
}
```

**Verification:**
1. **Initialization:** Invariant holds before loop
2. **Preservation:** If invariant holds at start of iteration, it holds at end
3. **Termination:** When loop exits, invariant ∧ ¬condition gives postcondition

**Verification conditions:**
```
// Initialization
sum == 0 ∧ i == 0 ⟹ I

// Preservation
I ∧ i < arr.length() ∧ (sum' = sum + arr[i]) ∧ (i' = i + 1) ⟹ I'

// Conclusion
I ∧ ¬(i < arr.length()) ⟹ sum == sum_of(arr[0..arr.length()])
```

---

## 20. Verification and Testing

### 20.1 Verification Modes

**Three verification strategies:**

1. **Static verification:** Prove contracts at compile time
2. **Runtime verification:** Check contracts at runtime (default)
3. **No verification:** Trust the programmer (unsafe)

**Compiler flags:**
```bash
# Static verification (must prove or fail compilation)
cantrip compile --verify=static main.cantrip

# Runtime verification (default)
cantrip compile --verify=runtime main.cantrip

# No verification (unsafe)
cantrip compile --verify=none main.cantrip
```

**Per-function control:**
```cantrip
#[verify(static)]
function calculate_tax(income: f64, rate: f64): f64
    requires
        income >= 0.0;
        rate >= 0.0 && rate <= 1.0;
    ensures
        result >= 0.0;
        result <= income;
{
    income * rate
}

#[verify(runtime)]
function user_input_handler(input: String): Result<Data, Error>
    requires !input.is_empty();
{
    parse(input)
}

#[verify(none)]
unsafe function low_level_operation(ptr: *u8) {
    // Unchecked pointer operations
}
```

### 20.2 Static Verification

**Uses SMT solvers (Z3, CVC5) to prove contract validity:**

**Process:**
1. Generate verification conditions
2. Convert to SMT-LIB format
3. Query SMT solver
4. If provable: compilation succeeds
5. If not provable: compilation fails with counterexample

```cantrip
#[verify(static)]
function abs(x: i32): i32
    ensures
        result >= 0;
        (x >= 0 implies result == x);
        (x < 0 implies result == -x);
{
    if x >= 0 { x } else { -x }
}

// Compiler generates SMT query:
// (declare-fun x () Int)
// (assert (=> (>= x 0) (= (abs x) x)))
// (assert (=> (< x 0) (= (abs x) (- x))))
// (assert (>= (abs x) 0))
// (check-sat)  → sat (provable)
**When static verification fails:**
```bash
$ cantrip compile --verify=static main.cantrip
error[E4001]: Cannot prove postcondition
  --> src/main.cantrip:15:5
   |
15 |         result >= 0;
   |         ^^^^^^^^^^^ cannot prove this always holds
   |
   = note: Counterexample: x = -2147483648
   = help: Consider adding precondition: x > i32::MIN
```

### 20.3 Runtime Verification

**Default mode - inserts runtime checks:**

**Generated code:**
```cantrip
// Source
function divide(a: f64, b: f64): f64
    requires b != 0.0;
{
    a / b
}

// Generated (conceptual)
function divide(a: f64, b: f64): f64 {
    // Check preconditions
    if !(b != 0.0) {
        std.panic(String.format("Precondition violation in divide: b != 0.0
                at src/math.cantrip:42
                b = {}", b));
    }

    // Execute
    let result = a / b;

    // Check postconditions (if any)

    result
}
```

**Optimization:** Precondition checks can be elided if caller proves contracts.

**Performance:**
- Debug mode: All checks enabled
- Release mode: Configurable (default: enabled)
- Critical path: Use `#[verify(static)]` to eliminate runtime overhead

### 20.4 Contract Fuzzing

**Compiler can automatically fuzz contracts to generate test cases:**

```cantrip
#[fuzz_contracts(cases = 1000)]
function binary_search(arr: [i32], target: i32): Option<usize>
    requires forall(i in 0..arr.length() - 1) {
        arr[i] <= arr[i + 1]
    };
    ensures match result {
        Some(idx) -> arr[idx] == target,
        None -> forall(i in 0..arr.length()) { arr[i] != target },
    };
{
    // Binary search implementation
    var low = 0;
    var high = arr.length();

    while low < high {
        let mid = (low + high) / 2;
        if arr[mid] < target {
            low = mid + 1;
        } else if arr[mid] > target {
            high = mid;
        } else {
            return Some(mid);
        }
    }
    None
}
```

**Compiler generates:**
```cantrip
@generated
test {
    // Edge cases
    expect binary_search(&[], 5) == None;
    expect binary_search(&[42], 42) == Some(0);
    expect binary_search(&[42], 43) == None;

    // Found cases
    expect binary_search(&[1, 2, 3, 4, 5], 3) == Some(2);
    expect binary_search(&[1, 2, 3, 4, 5], 1) == Some(0);
    expect binary_search(&[1, 2, 3, 4, 5], 5) == Some(4);

    // Not found cases
    expect binary_search(&[1, 3, 5, 7], 4) == None;
    expect binary_search(&[1, 3, 5, 7], 0) == None;
    expect binary_search(&[1, 3, 5, 7], 8) == None;

    // ... 991 more generated test cases
}
```

### 20.5 Fuzzing Configuration

**Project configuration:**
```toml
# cantrip.toml
[testing]
contract_fuzzing = true
fuzz_cases_default = 100
fuzz_shrink_attempts = 100
fuzz_timeout = "30s"
fuzz_seed = 42  # For reproducibility
```

**Command line:**
```bash
cantrip test --contract-fuzzing
cantrip test --fuzz-cases=10000
cantrip test --no-contract-fuzzing
cantrip test --fuzz-only=binary_search
```

**Fuzzing strategies:**
1. **Random generation:** Generate random inputs
2. **Boundary testing:** Test edge cases (0, max, min)
3. **Shrinking:** Find minimal failing case
4. **Mutation:** Mutate known inputs

### 20.6 Integration with Testing

**Contracts work with unit tests:**

```cantrip
#[test]
function test_withdraw() {
    var account = BankAccount.new(100.0);

    // These contracts are checked:
    account.withdraw(50.0);  // OK

    // This would panic:
    // account.withdraw(60.0);  // Precondition violation
}

#[test]
#[should_panic(expected = "Insufficient funds")]
function test_overdraft() {
    var account = BankAccount.new(100.0);
    account.withdraw(150.0);  // Should panic
}
```

**Property-based testing:**
```cantrip
#[property_test(cases = 1000)]
function test_sum_commutative(a: i32, b: i32) {
    // Property: addition is commutative
    assert(a + b == b + a, "addition is commutative");
}

#[property_test(cases = 1000)]
function test_sort_preserves_length(arr: Vec<i32>) {
    let original_len = arr.length();
    let sorted = arr.sort();
    assert(sorted.length() == original_len, "sort preserves length");
}
```

---

# Part VI: Effect System

## 21. Effects and Side Effects

### 21.1 Effect System Overview

**Definition 21.1 (Effect):** An effect represents a computational side effect - any observable interaction with the world outside pure computation.

**Design goals:**
- Track allocations, I/O, non-determinism, and resource usage
- Enforce determinism for networking/replay systems
- Prevent allocations in hot loops
- Budget tracking with compile-time and runtime enforcement
- Zero runtime overhead for pure functions

**Effect categories:**
1. **Memory effects** - Heap, stack, region allocations
2. **I/O effects** - File system, network, console
3. **Non-determinism** - Time, random, threading
4. **Rendering** - GPU operations, audio
5. **System** - Process spawning, FFI, unsafe operations

### 21.2 Effect Syntax

**Pure function (no effects):**
```cantrip
function add(a: i32, b: i32): i32 {
    a + b
}
```

**Function with effects:**
```cantrip
function load_data(path: String): Result<Data, Error>
    needs fs.read, alloc.heap;
    requires !path.is_empty();
{
    std.fs.read_to_string(path)
}
```

**Critical rule:** Effects are NEVER inferred. All functions must declare effects explicitly or be pure.

### 21.2.7 User-Defined Effects

**Definition 21.7 (Effect Definition):** A named effect is a composite of primitive effects and/or other named effects.

**Syntax:**

```ebnf
EffectDefinition ::= Visibility? "effect" Identifier "=" EffectExpr ";"

EffectExpr ::= EffectTerm
             | EffectExpr "+" EffectTerm      // Union
             | EffectExpr "-" EffectTerm      // Difference

EffectTerm ::= PrimitiveEffect
             | "!" EffectFactor                // Negation
             | EffectFactor

EffectFactor ::= EffectAtom
               | "(" EffectExpr ")"

EffectAtom ::= EffectName ("." "*")?          // e.g., fs.read or fs.*
             | EffectName "(" EffectParam ")"  // e.g., net.read(inbound)
             | Identifier                      // Reference to named effect
```

```cantrip
// Simple combination
effect WebService = fs.read + fs.write + net.read(inbound) + net.write;

// With negation
effect NoNetwork = !net.*;

// Composition of named effects
effect SafeWebService = WebService - fs.delete;

// With effect budgets
effect LimitedHeap = alloc.heap(bytes<=1MB);

// Simple effect
effect GameTick = alloc.temp;  // Only temp allocation
```

**Formal semantics:**

```
⟦effect E = ε₁ + ε₂ + ... + εₙ⟧ = ε₁ ∪ ε₂ ∪ ... ∪ εₙ
⟦effect E = E₁ + E₂⟧ = ⟦E₁⟧ ∪ ⟦E₂⟧
⟦effect E = E₁ - ε⟧ = ⟦E₁⟧ \ {ε}
⟦effect E = !ε⟧ = AllEffects \ {ε}
```

**Type rules:**

```
[Effect-Def]
ε₁ : Effect    ...    εₙ : Effect
names distinct
─────────────────────────────────────────
Γ ⊢ effect E = ε₁ + ... + εₙ well-formed

[Effect-Ref]
effect E = expr defined    Γ ⊢ expr : EffectSet
───────────────────────────────────────────────────
Γ ⊢ E : EffectSet    ⟦E⟧ = ⟦expr⟧

[Effect-Union]
Γ ⊢ ε₁ : EffectSet    Γ ⊢ ε₂ : EffectSet
─────────────────────────────────────────
Γ ⊢ ε₁ + ε₂ : EffectSet
⟦ε₁ + ε₂⟧ = ⟦ε₁⟧ ∪ ⟦ε₂⟧

[Effect-Difference]
Γ ⊢ ε₁ : EffectSet    Γ ⊢ ε₂ : EffectSet
─────────────────────────────────────────
Γ ⊢ ε₁ - ε₂ : EffectSet
⟦ε₁ - ε₂⟧ = ⟦ε₁⟧ \ ⟦ε₂⟧

[Effect-Negation]
Γ ⊢ ε : EffectSet
─────────────────────────────────────
Γ ⊢ !ε : EffectSet
⟦!ε⟧ = AllEffects \ ⟦ε⟧
```

**Expansion semantics:**

Effect definitions are expanded inline during effect checking:

```cantrip
// Definition
effect WebService = fs.read + fs.write + net.read(inbound);

// Usage
function handler()
    needs WebService;
{
    { ... }
}

// Expanded form (compile-time only)
function handler()
    needs fs.read, fs.write, net.read(inbound);
{
    { ... }
}
```

**Visibility rules:**

Effect definitions follow standard visibility rules:

```cantrip
// Private (module-only, default)
effect InternalOps = alloc.temp + fs.read;

// Public (exported)
public effect APIEffects = fs.read + net.read(outbound);

// Internal (package-only)
internal effect PackageOps = fs.* + alloc.heap;
```

**Scope:**

Effect definitions can appear:

1. At module level (after imports, before items)
2. In dedicated effect definition modules

```cantrip
// Module-level
import std.fs;

effect LocalOps = fs.read + fs.write;

function process()
    needs LocalOps;
{
    { ... }
}
```

```cantrip
// In effects module: src/effects.cantrip
public effect WebService = fs.read + fs.write + net.read(inbound);
public effect DatabaseOps = fs.read + fs.write + alloc.heap;

// Import and use: src/handlers.cantrip
import effects.WebService;

function handler()
    needs WebService;
{
    { ... }
}
```

### 21.3 Memory Effects

**Effect family: `alloc.*`**

```cantrip
alloc.heap          // Heap allocation
alloc.stack         // Explicit stack allocation
alloc.region        // Region allocation
alloc.temp          // Temporary allocations (frame-local)
alloc.*             // Any allocation
```

```cantrip
function create_vector(): own Vec<i32>
    needs alloc.heap;
{
    Vec.new()
}

function with_temp_buffer(): Result<(), Error>
    needs alloc.temp;
{
    let buffer = [0u8; 1024];  // Stack/temp allocation
    process(buffer)
}

function parse(): Result<Data, Error>
    needs alloc.region;
{
    region temp {
        let tokens = lex_in<temp>(input);
        let ast = parse_in<temp>(tokens);
        Ok(optimize(ast))
    }
}
**Type rule:**
```
[T-Alloc]
Γ ⊢ e : T
effects(e) ⊇ alloc.heap
─────────────────────────────
Γ ⊢ function f(): T
    needs alloc.heap;
{
    e
} : T ! alloc.heap
```

### 21.4 File System Effects

**Effect family: `fs.*`**

```cantrip
fs.read             // Read files
fs.write            // Write files
fs.delete           // Delete files
fs.metadata         // Read file metadata
fs.*                // Any filesystem operation
```

```cantrip
function read_config(path: String): Result<Config, Error>
    needs fs.read, alloc.heap;
    requires !path.is_empty();
{
    let contents = std.fs.read_to_string(path)?;
    Config.parse(contents)
}

function save_log(path: String, message: String): Result<(), Error>
    needs fs.write;
    requires !path.is_empty();
{
    std.fs.write(path, message)
}

function backup_directory(source: String, dest: String): Result<(), Error>
    needs fs.read, fs.write, alloc.heap;
{
    std.fs.copy_dir(source, dest)
}
### 21.5 Network Effects

**Effect family: `net.*`**

```cantrip
net.read(inbound)   // Accept incoming connections
net.read(outbound)  // Make outgoing connections
net.write           // Send network data
net.*               // Any network operation
```

**Direction parameter:**
- `inbound`: Server accepting connections
- `outbound`: Client making connections

```cantrip
function fetch_url(url: String): Result<String, Error>
    needs net.read(outbound), alloc.heap;
    requires !url.is_empty();
{
    let client = HttpClient.new();
    client.get(url)
}

function start_server(port: u16): Result<Server, Error>
    needs net.read(inbound), alloc.heap;
{
    Server.bind(port)
}

function handle_request(req: Request): Result<(), Error>
    needs net.write, alloc.heap;
{
    let response = process(req);
    response.send()
}
### 21.6 Non-Determinism Effects

**Time and randomness:**

```cantrip
time.read           // Read system time/clock
time.sleep          // Sleep/delay operations
random              // Non-deterministic RNG
thread.spawn        // Create threads
thread.join         // Wait for threads
```

```cantrip
function current_timestamp(): i64
    needs time.read;
{
    std.time.now().unix_timestamp()
}

function generate_uuid(): Uuid
    needs random, time.read;
{
    Uuid.new_v4()
}

function parallel_map<T, U>(items: Vec<T>, f: T -> U): Vec<U>
    needs thread.spawn, alloc.heap;
{
    std.parallel.map(items, f)
}
**Determinism enforcement:**
```cantrip
function replay_simulation(events: [Event]): State
    // Pure function - all effects denied by default (including time, random)
{
    var state = State.initial();
    for event in events {
        state = state.apply(event);
    }
    state
}
```

### 21.7 Standard Effect Definitions

**Definition 21.7 (Standard Library Effects):** The `std.effects` module provides common effect combinations designed for typical programming patterns. These definitions should be preferred over custom effect unions for consistency and LLM recognition.

#### 21.7.1 Importing Standard Effects

```cantrip
// Import commonly used effects
import std.effects.{SafeIO, WebService, GameTick};

// Use in function signatures
function process_file(path: String): Result<Data, Error>
    needs SafeIO;  // Instead of fs.read + fs.write + alloc.heap
{
    let contents = std.fs.read_to_string(path)?;
    Data.parse(contents)
}
```

#### 21.7.2 Core Standard Effects

**Module:** `std.effects`

```cantrip
// Pure computation (no effects)
public effect Pure = {};

// All allocation types
public effect AllAlloc = alloc.*;

// Safe file I/O (read and write, no delete)
public effect SafeIO = fs.read + fs.write + alloc.heap;

// All I/O operations
public effect AllIO = fs.* + net.* + io.write;

// Network I/O only
public effect NetworkIO = net.read(inbound) + net.read(outbound) + net.write + alloc.heap;

// File I/O only
public effect FileIO = fs.read + fs.write + alloc.heap;

// Console I/O only
public effect ConsoleIO = io.write;

// Forbid all allocation
public effect NoAlloc = !alloc.*;

// Forbid all I/O
public effect NoIO = !(fs.* + net.* + io.write);

// Deterministic computation (no time, no random)
public effect Deterministic = !(time.* + random);

// Game tick constraints (temp allocation only, no I/O)
public effect GameTick = alloc.temp + !alloc.heap + !alloc.region + NoIO;

// Real-time constraints (no heap allocation, no I/O)
public effect RealTime = !alloc.heap + NoIO;

// Web service (network + file I/O + allocation)
public effect WebService = net.* + fs.read + fs.write + alloc.heap;

// Database operations (file I/O + allocation, no network)
public effect DatabaseOps = fs.* + alloc.heap + !net.*;
```

#### 21.7.3 Effect Pattern Examples

**Pattern 1: Web Service Handler**

```cantrip
import std.effects.WebService;

function handle_http_request(req: Request): Response
    needs WebService;
    requires !req.path.is_empty();
{
    // Can use: network, file system, heap allocation
    let data = load_from_disk(req.path)?;
    let processed = transform(data);
    send_response(processed)
}
```

**Pattern 2: Game Engine Update Loop**

```cantrip
import std.effects.GameTick;

function update_entities(entities: mut [Entity], dt: f32)
    needs GameTick;
    requires dt > 0.0 && dt < 1.0;
{
    // Only temporary allocations allowed
    // No heap, no file system, no network
    region frame {
        let temp_data = process_frame_in<frame>(entities, dt);
        apply_changes(entities, temp_data);
    } // O(1) bulk free - perfect for 60 FPS
}
```

**Pattern 3: Real-Time Audio Processing**

```cantrip
import std.effects.RealTime;

function process_audio_buffer(buffer: mut [f32], params: AudioParams)
    needs RealTime;
    requires buffer.length() == 512;
{
    // No heap allocation (latency spike)
    // No I/O (unpredictable timing)
    for sample in buffer {
        *sample = apply_filter(*sample, params);
    }
}
```

**Pattern 4: Pure Computation**

```cantrip
import std.effects.Pure;

function calculate_checksum(data: [u8]): u32
    needs Pure;  // Explicit no-effects marker
    requires !data.is_empty();
{
    var checksum: u32 = 0;
    for byte in data {
        checksum = checksum ^ (byte as u32);
    }
    checksum
}
```

#### 21.7.4 Custom Effect Composition

**Compose standard effects for project-specific needs:**

```cantrip
// Backend API server
effect BackendAPI = std.effects.WebService + std.effects.DatabaseOps;

// Frontend (no file system access)
effect Frontend = std.effects.NetworkIO + !fs.*;

// Read-only service
effect ReadOnly = std.effects.FileIO - fs.write;

// Batch processor
effect BatchProcessor = std.effects.FileIO + std.effects.Deterministic;
```

```cantrip
function api_endpoint(req: Request): Response
    needs BackendAPI;
{
    let user_data = query_database(req.user_id)?;
    let cached = load_cache_file(req.cache_key)?;
    generate_response(user_data, cached)
}
```

#### 21.7.5 Effect Documentation Pattern

**When defining custom effects, document purpose and usage:**

```cantrip
/// Defines effects for cryptocurrency transaction processing.
///
/// Includes network I/O for blockchain communication, file I/O for
/// local storage, and deterministic computation for reproducibility.
///
/// Explicitly forbids:
/// - Random number generation (must use deterministic sources)
/// - Time operations (must use block timestamps)
///
/// # Examples
/// ```
/// function process_transaction(tx: Transaction): Result<Receipt, Error>
///     needs CryptoEffects;
/// {
///     validate_signatures(tx)?;
///     broadcast_to_network(tx)?;
///     store_locally(tx)
/// }
/// ```
effect CryptoEffects =
    std.effects.NetworkIO +
    std.effects.FileIO +
    std.effects.Deterministic +
    !random +
    !time.read;
```

---

### 21.9 Rendering Effects

**GPU and audio:**

```cantrip
render.draw         // GPU draw calls
render.compute      // GPU compute shaders
audio.play          // Audio playback
input.read          // Read input devices
```

```cantrip
function render_frame(scene: Scene)
    needs render.draw;
{
    gpu.clear();
        for entity in scene.entities {
            gpu.draw(entity);
        }
        gpu.present();
}

function run_physics(data: ComputeData): Result<PhysicsState, Error>
    needs render.compute;
{
    gpu.dispatch_compute_shader(data)
}
### 21.10 System Effects

**Process and FFI:**

```cantrip
process.spawn       // Execute external processes
ffi.call            // Call foreign functions
unsafe.ptr          // Raw pointer operations
panic               // May panic/abort
```

```cantrip
function run_command(cmd: String): Result<Output, Error>
    needs process.spawn, alloc.heap;
{
    std.process.Command.new(cmd).execute()
}

function call_c_library(data: *u8): i32
    needs ffi.call, unsafe.ptr;
{
    unsafe {
            external_c_function(data)
        }
}
### 21.11 Complete Effect Taxonomy

**Definition 21.11 (Effect Taxonomy):** The complete set of primitive effects in Cantrip, organized hierarchically by category.

#### 21.11.1 Memory Effects (`alloc.*`)

```cantrip
alloc.heap          // Heap allocation via malloc/new
alloc.stack         // Explicit stack allocation (large arrays)
alloc.region        // Region/arena allocation
alloc.temp          // Frame-local temporary allocation
```

**Usage guidelines:**
- `alloc.heap`: Long-lived data, return values, dynamic structures
- `alloc.stack`: Large stack arrays, local buffers
- `alloc.region`: Batch processing, compilation phases, parsers
- `alloc.temp`: Small frame-local temporaries

**Wildcards:**
- `alloc.*`: Any allocation type

#### 21.11.2 File System Effects (`fs.*`)

```cantrip
fs.read             // Read files and directories
fs.write            // Write/modify files
fs.delete           // Delete files and directories
fs.metadata         // Read file metadata (size, permissions, etc.)
```

**Wildcards:**
- `fs.*`: Any file system operation

#### 21.11.3 Network Effects (`net.*`)

```cantrip
net.read(inbound)   // Accept incoming connections (server)
net.read(outbound)  // Make outgoing connections (client)
net.write           // Send data over network
```

**Direction parameters:**
- `inbound`: Server accepting connections
- `outbound`: Client making connections

**Wildcards:**
- `net.*`: Any network operation
- `net.read`: Both inbound and outbound

#### 21.11.4 I/O Effects (`io.*`)

```cantrip
io.write            // Write to stdout/stderr (console output)
```

#### 21.11.5 Time Effects (`time.*`)

```cantrip
time.read           // Read system clock/time
time.sleep          // Sleep/delay operations
```

#### 21.11.6 Threading Effects (`thread.*`)

```cantrip
thread.spawn        // Create new threads
thread.join         // Wait for thread completion
thread.atomic       // Atomic operations
```

#### 21.11.7 Rendering Effects (`render.*`)

```cantrip
render.draw         // GPU draw calls
render.compute      // GPU compute shaders
```

#### 21.11.8 Audio Effects (`audio.*`)

```cantrip
audio.play          // Audio playback
```

#### 21.11.9 Input Effects (`input.*`)

```cantrip
input.read          // Read from input devices (keyboard, mouse, gamepad)
```

#### 21.11.10 Process Effects (`process.*`)

```cantrip
process.spawn       // Execute external processes
```

#### 21.11.11 FFI Effects (`ffi.*`)

```cantrip
ffi.call            // Call foreign functions (C, etc.)
```

#### 21.11.12 Unsafe Effects (`unsafe.*`)

```cantrip
unsafe.ptr          // Raw pointer operations
```

#### 21.11.13 System Effects

```cantrip
panic               // May panic/abort execution
async               // Asynchronous computation
random              // Non-deterministic random number generation
```

#### 21.11.14 Complete Effect Hierarchy

```
All Effects (*)
├── alloc.*
│   ├── alloc.heap
│   ├── alloc.stack
│   ├── alloc.region
│   └── alloc.temp
├── fs.*
│   ├── fs.read
│   ├── fs.write
│   ├── fs.delete
│   └── fs.metadata
├── net.*
│   ├── net.read(inbound)
│   ├── net.read(outbound)
│   └── net.write
├── io.*
│   └── io.write
├── time.*
│   ├── time.read
│   └── time.sleep
├── thread.*
│   ├── thread.spawn
│   ├── thread.join
│   └── thread.atomic
├── render.*
│   ├── render.draw
│   └── render.compute
├── audio.*
│   └── audio.play
├── input.*
│   └── input.read
├── process.*
│   └── process.spawn
├── ffi.*
│   └── ffi.call
├── unsafe.*
│   └── unsafe.ptr
├── panic
├── async
└── random
```

#### 21.11.15 Standard Effect Definitions

The standard library (`std.effects`) provides common effect combinations:

```cantrip
// Pure computation (no effects)
public effect Pure = !*;

// All allocation types
public effect AllAlloc = alloc.*;

// All I/O operations
public effect AllIO = fs.* + net.* + io.write;

// Safe I/O (read/write but no delete)
public effect SafeIO = fs.read + fs.write + alloc.heap;

// Network I/O only
public effect NetworkIO = net.* + alloc.heap;

// File I/O only
public effect FileIO = fs.read + fs.write + alloc.heap;

// Console I/O only
public effect ConsoleIO = io.write;

// Forbid all allocation
public effect NoAlloc = !alloc.*;

// Forbid all I/O
public effect NoIO = !(fs.* + net.* + io.write);

// Deterministic computation (no time, no random)
public effect Deterministic = !(time.* + random);

// Game tick constraints (temp allocation only, no I/O)
public effect GameTick = alloc.temp + !alloc.heap + !alloc.region + NoIO;

// Real-time constraints (no heap allocation, no I/O)
public effect RealTime = !alloc.heap + NoIO;

// Web service (network + file I/O + allocation)
public effect WebService = net.* + fs.read + fs.write + alloc.heap;

// Database operations (file I/O + allocation, no network)
public effect DatabaseOps = fs.* + alloc.heap + !net.*;
```

**Usage example:**
```cantrip
import std.effects.SafeIO;

function process_config(path: String): Result<Config, Error>
    needs SafeIO;
{
    let contents = std.fs.read_to_string(path)?;
    Config.parse(contents)
}
```

---

## 22. Effect Rules and Checking

### 22.7 Async Effect Masks

Awaiting a future imports its declared effects into the enclosing async function’s *effect mask*.
To reduce boilerplate while preserving explicitness, Cantrip provides targeted diagnostics and fix‑its
for common async sources:

- `net.*` from awaited network futures
- `time.sleep` from awaited timers

**Rule (masking):**
```
[T-Async-Mask]
Γ ⊢ await e : T ! ε_fut
enclosing function declares ε_fun
if ε_fut ⊄ ε_fun then
  ERROR E9201 with fix-it: add missing effects to 'needs' clause
```

**Notes:**
- This is **not** effect inference; no code is accepted unless the signature is updated.
- Implementations MAY surface the exact missing atoms (e.g., `net.read(outbound)`), and SHOULD
  offer “Add `needs net.read(outbound)`” code actions.

```cantrip
async function fetch(url: str): String
    needs alloc.heap; // missing net.read(outbound)
{
    let body = await http.get(url)?;  // E9201 → fix-it adds net.read(outbound)
    body
}

### 22.1 Effect Type System

**Effect judgment:**
```
Γ ⊢ e ! ε  (expression e has effect ε)
```

**Effect lattice:**
```
(E, ⊑, ⊔, ⊓)

where:
  ∅ ⊑ ε                           (pure is subeffect)
  ε₁ ⊑ ε₁ ⊔ ε₂                     (union upper bound)
  ε₁ ⊔ ε₂ = ε₂ ⊔ ε₁                 (commutativity)
  (ε₁ ⊔ ε₂) ⊔ ε₃ = ε₁ ⊔ (ε₂ ⊔ ε₃)    (associativity)
```

### 22.2 Core Effect Rules

**[E-Pure] Pure expression:**
```
───────────────
Γ ⊢ v ! ∅
```

**[E-Var] Variable:**
```
x : T ∈ Γ
───────────────
Γ ⊢ x ! ∅
```

**[E-App] Function application:**
```
Γ ⊢ f : fn(T): U ! ε₁
Γ ⊢ e : T ! ε₂
─────────────────────
Γ ⊢ f(e) ! ε₁ ⊔ ε₂
```

**[E-Seq] Sequencing:**
```
Γ ⊢ e₁ : T₁ ! ε₁    Γ ⊢ e₂ : T₂ ! ε₂
─────────────────────────────────────
Γ ⊢ e₁ ; e₂ : T₂ ! (ε₁ ; ε₂)
```

**[E-If] Conditional:**
```
Γ ⊢ e₁ : bool ! ε₁
Γ ⊢ e₂ : T ! ε₂
Γ ⊢ e₃ : T ! ε₃
──────────────────────────────
Γ ⊢ if e₁ then e₂ else e₃ : T ! (ε₁ ; (ε₂ ⊔ ε₃))
```

**[E-Pipeline] Flow operator:**
```
Γ ⊢ e₁ : T ! ε₁    Γ ⊢ e₂ : fn(T): U ! ε₂
──────────────────────────────────────────
Γ ⊢ e₁ => e₂ : U ! (ε₁ ; ε₂)
```

### 22.3 Explicit Declaration Required

**Effects are NEVER inferred:**

```cantrip
function helper(): Vec<i32>
    needs alloc.heap;  // MUST be explicit
{
    Vec.new()
}

// ERROR: Missing effect declaration
function bad(): Vec<i32> {
    Vec.new()  // ERROR E9001: alloc.heap not declared
}
```

**Type checking:**
```
[Check-Effects]
function f(): T
    needs ε_declared;
{
    e
}

Γ ⊢ e : T ! ε_actual
ε_actual ⊑ ε_declared
─────────────────────────
f well-typed
```

**Error if undeclared:**
```bash
$ cantrip compile main.cantrip
error[E9001]: Missing effect declaration
  --> src/main.cantrip:5:9
   |
 5 |         Vec.new()
   |         ^^^^^^^^^ requires effect alloc.heap
   |
note: function requires no effects
  --> src/main.cantrip:2:5
   |
 2 |     requires:
   |     ^^^^^^^^^ declared as pure
   |
help: add effect declaration
   |
 2 |     requires<alloc.heap>:
   |             ^^^^^^^^^^^^
```

### 22.3.1 Named Effects in Declarations

Named effects can be used anywhere primitive effects are used:

```cantrip
effect WebOps = fs.read + fs.write + net.read(inbound);

function process()
    needs WebOps;  // Expands to constituent effects
{
    { ... }
}
```

**Combining named and primitive effects:**

```cantrip
function process_with_logging()
    needs WebOps, time.read;  // Named + primitive
{
    { ... }
}
```

**Effect subtraction:**

```cantrip
function readonly_web()
    needs WebOps - fs.write;  // Remove specific effect
{
    { ... }
}
```

**Type checking treats named effects as their expanded form:**

```
[Check-Named-Effects]
effect E = ε₁ + ... + εₙ

function f(): T
    needs E;
{
    e
}

Γ ⊢ e : T ! ε_actual
{ε₁, ..., εₙ} ⊇ ε_actual
─────────────────────────────────
f well-typed
```

**Example integration:**

```cantrip
// Before (with primitive effects)
function load_data(path: String): Result<Data, Error>
    needs fs.read, alloc.heap;
    requires !path.is_empty();
{
    std.fs.read_to_string(path)
}

// After (with named effect)
effect FileOps = fs.read + alloc.heap;

function load_data(path: String): Result<Data, Error>
    needs FileOps;
    requires !path.is_empty();
{
    std.fs.read_to_string(path)
}
```

### 22.4 Forbidden Effects

**Purpose:** Forbidden effects (`!effect`) restrict wildcards and polymorphic effects. They are **NOT** for documenting intent.

#### 22.4.1 Valid Use Case 1: Wildcard Restriction

Restrict a wildcard to exclude specific effects:

**✅ CORRECT - Wildcard Restriction:**

```cantrip
function safe_fs_operations()
    needs fs.*, !fs.delete;  // All fs operations except delete
{
    read_file(path)?;
    write_file(path)?;
    // delete_file(path)?;  // ERROR E9002: fs.delete forbidden
}
```

**❌ WRONG - Not a Wildcard Restriction:**

```cantrip
function broken()
    needs fs.read, fs.write, !fs.delete;  // ERROR E9010: Redundant
{
    read_file(path)?;
}
```

**Type rule:**
```
[T-Wildcard-Restriction]
function f()
    needs epsilon.*, !epsilon.specific;
{
    e
}

epsilon.specific in epsilon.*
epsilon_actual subset (epsilon.* \ {epsilon.specific})
──────────────────────────────────
f well-typed
```

#### 22.4.2 Valid Use Case 2: Polymorphic Effect Constraint

Constrain effects propagated from generic parameters:

**✅ CORRECT - Polymorphic Constraint:**

```cantrip
function deterministic_map<F>(items: [i32], f: F): Vec<i32>
    where F: Fn(i32): i32
    needs effects(F), !time.read, !random;
{
    items.map(f)
}
```

**❌ WRONG - Not Polymorphic:**

```cantrip
function broken()
    needs alloc.heap, !time.read;  // ERROR E9010: Redundant
{
    Vec.new()
}
```

**Type rule:**
```
[T-Polymorphic-Constraint]
function f<F>()
    needs effects(F), !epsilon_forbidden;
{
    e
}

effects(F) intersect epsilon_forbidden = empty
─────────────────────────────
f well-typed
```

#### 22.4.3 INVALID: Redundant Forbidden Effects

These patterns are **compile errors** (E9010):

**❌ WRONG - Redundant Forbidden Effects:**

```cantrip
function broken()
    needs alloc.heap, !fs.*;  // ERROR E9010: fs.* already forbidden
{
    Vec.new()
}
```

**✅ CORRECT - Just Declare What You Need:**

```cantrip
function correct()
    needs alloc.heap;  // Everything else denied by default
{
    Vec.new()
}
```

**Error rule:**
```
[E-Redundant-Forbidden]
function f()
    needs epsilon_1, ..., epsilon_n, !epsilon_forbidden;
{
    e
}

epsilon_forbidden not_in (epsilon_1 union ... union epsilon_n)
not is_wildcard_restriction(!epsilon_forbidden)
not is_polymorphic_constraint(!epsilon_forbidden)
──────────────────────────────────────────
ERROR E9010: Redundant forbidden effect

Forbidden effects are only valid for:
1. Wildcard restriction: needs effect.*, !effect.specific
2. Polymorphic constraint: needs effects(F), !effect
```

### 22.5 Effect Wildcards

**Use `*` for effect families:**

**✅ CORRECT - Wildcard Usage:**

```cantrip
function network_ops()
    needs net.*;  // All network operations
{
    fetch_data()?;
    post_data()?;
}
```

**❌ WRONG - Redundant Forbidden with Wildcard:**

```cantrip
function broken()
    needs fs.read, fs.write, !fs.delete;  // ERROR E9010
{
    // Should be: needs fs.*, !fs.delete (wildcard restriction)
    // OR: needs fs.read, fs.write (delete already forbidden)
}
```

**Wildcard expansion:**
```
alloc.* = {alloc.heap, alloc.stack, alloc.region, alloc.temp}
fs.* = {fs.read, fs.write, fs.delete, fs.metadata}
net.* = {net.read(inbound), net.read(outbound), net.write}
```

### 22.6 Higher-Order Functions

**Use `effects(F)` to propagate callback effects:**

```cantrip
function map<T, U, F>(items: [T], mapper: F): Vec<U>
    where F: Fn(T): U
    needs effects(F), alloc.heap;
    requires items.length() > 0;
{
    var result = Vec.with_capacity(items.length());
    for item in items {
        result.push(mapper(item));  // May have effects
    }
    result
}
```

**Type rule:**
```
[T-Effects-Of]
Γ ⊢ f : fn(T): U ! ε
────────────────────────────────
effects(f) = ε
```

```cantrip
let numbers = [1, 2, 3, 4, 5];

// Pure mapper
let doubled = map(numbers, |x| x * 2);  // effects(F) = ∅

// Mapper with effects
let logged = map(numbers, |x| {
    std.io.println(x);  // io.write effect
    x * 2
});  // map requires<io.write, alloc.heap>
**Constraining callbacks:**
```cantrip
function parallel_map<T, U, F>(items: [T], mapper: F): Vec<U>
    where F: Fn(T): U
    needs effects(F), !time.read, !random, alloc.heap;
    requires items.length() > 0;
{
    std.parallel.map_n(items, 8)
}

// Usage
let result = parallel_map(data, |x| {
    // Cannot use time.read or random in parallel map
    // let now = std.time.now();  // ERROR
    process(x)
});
```

---

### Async Effect Masking (await)

Awaiting a future imports its declared effects into the enclosing async function’s *effect mask*.
To reduce boilerplate while preserving explicitness, Cantrip provides targeted diagnostics and fix‑its
for common async sources:

- `net.*` from awaited network futures
- `time.sleep` from awaited timers

**Rule (masking):**
```
[T-Async-Mask]
Γ ⊢ await e : T ! ε_fut
enclosing function declares ε_fun
if ε_fut ⊄ ε_fun then
  ERROR E9201 with fix-it: add missing effects to 'needs' clause
```

**Notes:**
- This is **not** effect inference; no code is accepted unless the signature is updated.
- Implementations MAY surface the exact missing atoms (e.g., `net.read(outbound)`), and SHOULD
  offer “Add `needs net.read(outbound)`” code actions.

**Examples:**
```cantrip
async function fetch(url: str): String
    needs alloc.heap; // missing net.read(outbound)
{
    let body = await http.get(url)?;  // E9201 → fix-it adds net.read(outbound)
    body
}
```
## 23. Effect Budgets

### 23.1 Budget Overview

**Definition 23.1 (Effect Budget):** A budget is a quantitative constraint on resource usage, enforced at compile time or runtime.

**Budget types:**
1. **Memory budgets** - Limit allocation size
2. **Time budgets** - Limit operation duration
3. **Count budgets** - Limit operation count

**Syntax:**
```
effect(constraint)

Examples:
alloc.heap(bytes<=1024)
time.sleep(duration<=5s)
thread.spawn(count<=8)
```

### 23.2 Static Budgets

**Compile-time verification of resource limits:**

```cantrip
function small_allocation(): Vec<u8>
    needs alloc.heap(bytes<=1024);
{
    Vec.with_capacity(100)  // OK: 100 bytes < 1024

    // ERROR: Exceeds budget
    // Vec.with_capacity(10000)
}
```

**Verification:**
```
[Check-Budget-Static]
function f(): T
    needs alloc.heap(bytes<=N);
{
    e
}

∀ allocation in e. size(allocation) ≤ N
─────────────────────────────────────────
f satisfies budget
```

**Compiler analysis:**
```cantrip
function bounded(): Vec<i32>
    needs alloc.heap(bytes<=100);
{
    // Compiler computes: Vec<i32> with capacity 10
    // Size = 10 * sizeof(i32) = 10 * 4 = 40 bytes
    Vec.with_capacity(10)  // OK: 40 < 100
}
```

### 23.3 Dynamic Budgets

**Runtime enforcement with budget tracking:**

```cantrip
function process_chunks(data: [u8], chunk_size: usize): Vec<Vec<u8>>
    needs alloc.heap(bytes<=65536);  // 64KB budget
    requires chunk_size > 0;
{
    var result = Vec.new();
    var budget_used = 0;

    for chunk in data.chunks(chunk_size) {
        let chunk_vec = chunk.to_vec();
        budget_used += chunk_vec.capacity() * sizeof<u8>();

        if budget_used > 65536 {
            std.panic("Budget exceeded");
        }

        result.push(chunk_vec);
    }
    result
}
```

**Compiler-generated tracking:**
```cantrip
// Source
function f(): Vec<i32>
    needs alloc.heap(bytes<=1000);
{
    dynamic_allocation(n)
}

// Generated
function f() {
    let mut budget = Budget::new(1000);

    let result = dynamic_allocation_tracked(n, &mut budget);

    if budget.exceeded() {
        std.panic(String.format("Budget exceeded: used {} bytes", budget.used()));
    }

    result
}
```

### 23.4 Time Budgets

**Limit operation duration:**

```cantrip
function timeout_operation(): Result<(), Error>
    needs time.sleep(duration<=5s);
{
    std.time.sleep(Duration.from_secs(3))  // OK: 3s < 5s
}

function may_timeout(task: Task): Result<Output, Error>
    needs time.sleep(duration<=10s);
{
    with_timeout(Duration.from_secs(10), || {
        task.execute()
    })
}
```

**Verification:**
```
[Check-Budget-Time]
function f()
    needs time.sleep(duration<=D);
{
    e
}

∑(all sleep calls in e) ≤ D
───────────────────────────
f satisfies time budget
```

### 23.5 Count Budgets

**Limit operation count:**

```cantrip
function parallel_map(items: [Task]): Vec<Result>
    needs thread.spawn(count<=8);
    requires items.length() > 0;
{
    std.parallel.map_n(items, 8)  // OK: spawns 8 threads
}

function limited_retries(operation: Operation): Result<(), Error>
    needs net.read(outbound, count<=3);
{
    var attempts = 0;
    loop {
        match operation.try() {
            Ok(()) -> return Ok(()),
            Err(e) if attempts < 3 -> attempts += 1,
            Err(e) -> return Err(e),
            }
        }
}
```

### 23.6 Budget Composition

**Budgets compose additively:**

```cantrip
function composed()
    needs alloc.heap(bytes<=1000);
{
    small_allocation();   // Uses 100 bytes
        small_allocation();   // Uses 100 bytes
        // Total: 200 bytes < 1000 ✓
}
```

**Type rule:**
```
[T-Budget-Compose]
Γ ⊢ e₁ : T₁ ! alloc.heap(bytes≤N₁)
Γ ⊢ e₂ : T₂ ! alloc.heap(bytes≤N₂)
────────────────────────────────────────
Γ ⊢ e₁ ; e₂ : T₂ ! alloc.heap(bytes≤N₁+N₂)
```

```cantrip
function process()
    needs alloc.heap(bytes<=10000);
{
    step1();  // 3000 bytes
        step2();  // 4000 bytes
        step3();  // 2000 bytes
        // Total: 9000 bytes < 10000 ✓
}
### 23.7 Budget Units

**Supported unit types:**

**Memory:**
```cantrip
bytes<=N
KiB<=N    // Kibibytes (1024 bytes)
MiB<=N    // Mebibytes (1024 KiB)
GiB<=N    // Gibibytes (1024 MiB)
```

**Time:**
```cantrip
ms<=N     // Milliseconds
s<=N      // Seconds
min<=N    // Minutes
```

**Count:**
```cantrip
count<=N  // Number of operations
```

```cantrip
requires<alloc.heap(KiB<=64)>:      // 64 KiB
requires<alloc.heap(MiB<=1)>:       // 1 MiB
requires<time.sleep(ms<=500)>:      // 500ms
requires<thread.spawn(count<=16)>:  // 16 threads
---

## 24. Effect Soundness

### 24.1 Effect Soundness Theorem

**Theorem 24.1 (Effect Soundness):**

If Γ ⊢ e : T ! ε and ⟨e, σ, ∅⟩ →* ⟨v, σ', ε'⟩, then ε' ⊆ ε.

**Proof sketch:**
1. By induction on the derivation of Γ ⊢ e : T ! ε
2. Base cases (values, variables) have no effects
3. Inductive cases preserve effect bounds
4. Effect composition rules ensure ε' ⊆ ε

**Consequence:** Declared effects are an upper bound on actual effects.

### 24.2 Budget Compliance

**Theorem 24.2 (Budget Compliance):**

If Γ ⊢ e : T ! alloc.heap(bytes≤N) and ⟨e, σ⟩ →* ⟨v, σ'⟩,
then total_allocated(e) ≤ N.

**Proof obligation:** Must be verified for all functions with budget constraints.

### 24.3 Effect Approximation

**Lemma 24.1 (Effect Subsumption):**

```
Γ ⊢ e : T ! ε₁    ε₁ ⊆ ε₂
────────────────────────────
Γ ⊢ e : T ! ε₂
```

**Application:** Can always overapproximate effects.

```cantrip
function specific()
    needs fs.read;
{
    read_file("config.txt")
}

function general()
    needs fs.*, alloc.heap;  // Overapproximation
{
    specific()  // OK: fs.read ⊆ fs.*
}
### 24.4 Effect Monotonicity

**Lemma 24.2 (Effect Monotonicity):**

If e₁ ⊆ e₂ (e₁ is a subexpression of e₂), then effects(e₁) ⊆ effects(e₂).

**Proof:** By structural induction on expressions.

### 24.5 No Hidden Effects

**Axiom 24.1 (Effect Visibility):**

All effects must be explicitly declared in function signatures. No implicit effects exist.

**Violations caught at compile time:**
```cantrip
function sneaky(): i32 {  // Claims pure
    std.io.println("side effect!");  // ERROR E9001
    42
}
```

# Part VII: Memory Management

## 25. Lexical Permission System

### 25.1 LPS Overview

**Definition 25.1 (Lexical Permission System):** The LPS provides memory safety through explicit permissions, lexical lifetimes, and region-based memory management, without borrow checking or garbage collection.

**Core principles:**
1. **Explicit permissions** - All access rights visible in code
2. **Lexical lifetimes** - Memory scopes follow code structure (regions)
3. **Zero runtime overhead** - Compiles to raw pointers
4. **Local reasoning** - No global program analysis needed
5. **Simple model** - No complex borrow checker

**Key insight:** Cantrip follows the **Cyclone model**, not the Rust model. Regions control WHEN memory is freed, not WHO can access it.

### 25.2 Design Goals

**Safety guarantees provided:**
- ✅ No use-after-free (regions enforce lifetime)
- ✅ No double-free (regions handle deallocation)
- ✅ No memory leaks (automatic cleanup)
- ✅ Deterministic destruction order (LIFO)

**Safety guarantees NOT provided:**
- ❌ No aliasing bugs (can have multiple mut refs)
- ❌ No data races (programmer's responsibility)
- ❌ No iterator invalidation (programmer's responsibility)

**Philosophy:** Provide memory safety and deterministic deallocation, but don't prevent aliasing. Simpler than Rust, safer than C++.

### 25.3 Memory Safety Model

**Three allocation strategies:**

1. **Stack (automatic):**
   - Fastest allocation
   - LIFO deallocation
   - Limited size
   - Example: Local variables

2. **Heap (explicit):**
   - Flexible lifetime
   - Can escape function
   - Requires explicit management
   - Example: `Vec.new()`

3. **Region (bulk deallocation):**
   - Fast allocation (bump pointer)
   - O(1) bulk deallocation
   - Cannot escape region
   - Example: Temporary parsing data

**Formal model:**

```
Memory = Stack ∪ Heap ∪ ⋃ᵢ Regionᵢ

where:
  Stack = {(addr, value, scope) | addr ∈ StackAddrs}
  Heap = {(addr, value) | addr ∈ HeapAddrs}
  Regionᵢ = {(addr, value, region) | addr ∈ RegionAddrs}
```

### 25.4 Compilation Model

**High-level:**
```cantrip
function example() {
    let x = 42;                    // Stack
    let v = Vec.new();             // Heap
    region temp {
        let buf = Vec.new_in<temp>();  // Region
    }
}
```

**Compiles to (conceptual C):**
```c
void example() {
    int32_t x = 42;  // Stack

    Vec* v = vec_new();  // malloc

    Arena temp = arena_create();
    Vec* buf = vec_new_in_arena(&temp);
    arena_destroy(&temp);  // O(1) bulk free

    vec_free(v);  // free
}
```

**No runtime overhead:**
- No reference counting
- No garbage collection
- No borrow checking at runtime
- Just pointers and deterministic cleanup

---

## 26. Permission Types and Rules

### 26.1 Permission Overview

**Definition 26.1 (Permission):** A permission controls what operations are allowed on a value.

**Permission hierarchy:**

| Permission | Sigil | Read | Write | Move | Destroy | Aliasing |
|------------|-------|------|-------|------|---------|----------|
| Reference | (none) | ✓ | ✗ | ✗ | ✗ | Many allowed |
| Owned | `own` | ✓ | ✓ | ✓ | ✓ | One owner |
| Mutable | `mut` | ✓ | ✓ | ✗ | ✗ | Many allowed |
| Isolated | `iso` | ✓ | ✓ | ✓ | ✗ | Exactly one |

**Key difference from Rust:** No borrow checker. Multiple `mut` references can coexist - programmer ensures correct usage.

### 26.2 Immutable Reference (Default)

**Default for function parameters** - like C++ `const&`:

```cantrip
function read_data(data: Data): i32 {  // Immutable reference
    data.field  // Can read
    // data.field = 42;  // ERROR: Cannot write
}
```

**Type rule:**
```
[T-Ref]
Γ ⊢ e : T
──────────────────────
function f(x: T) accepts immutable reference to e
```

**Multiple references allowed (no borrow checking):**
```cantrip
function example()
    needs alloc.heap;
{
    let data = Data.new();

        // All of these work - no borrow checker
        read1(data);
        read2(data);
        read3(data);
        // Programmer ensures this is safe
}
```

**Formal semantics:**
```
⟦T⟧ = { reference to value of type T }
Operations: Read only
Aliasing: Unlimited
```

### 26.3 Owned Permission

**Full ownership** - like C++ `unique_ptr`:

```cantrip
function example()
    needs alloc.heap;
{
    let data: own Data = Data.new();

        data.field = 42;           // Can write
        let x = data.field;        // Can read
        consume(move data);        // Transfer ownership
        // data is gone - compiler enforces this
}
```

**Type rule:**
```
[T-Own]
Γ ⊢ e : T    copyable(T) = false
──────────────────────────────────
Γ ⊢ e : own T
```

**Ownership rules:**
1. Exactly one owner exists
2. Automatically destroyed when scope ends
3. Must use `move` to transfer ownership
4. Can pass by reference without transferring

```cantrip
function take_ownership(own data: Data) {
    // data is owned by this function
    // Automatically freed when function returns
}

function example()
    needs alloc.heap;
{
    let data = Data.new();

        // Pass by reference - still own data
        read(data);

        // Transfer ownership
        take_ownership(move data);
        // data no longer accessible
}
**Formal semantics:**
```
⟦own T⟧ = { unique pointer to value of type T }
Operations: Read, Write, Move, Destroy
Aliasing: Exactly one
Lifetime: Lexical scope
```

### 26.4 Mutable Permission

**Mutable reference** - like C++ `&`:

```cantrip
function update_data(data: mut Data) {
    data.field = 42;  // Can read and write
}
```

**Type rule:**
```
[T-Mut]
Γ ⊢ e : T    mutable(e) = true
──────────────────────────────────
Γ ⊢ e : mut T
```

**Multiple mutable references allowed (programmer's responsibility):**
```cantrip
function example()
    needs alloc.heap;
{
    var data = Data.new();

        // NO borrow checking - all allowed simultaneously
        modify1(mut data);
        modify2(mut data);
        read(data);
        modify3(mut data);

        // Programmer must ensure this doesn't cause bugs!
}
```

**This is different from Rust:**
```rust
// Rust: ERROR - can't have multiple mut refs
let mut data = Data::new();
let r1 = &mut data;
let r2 = &mut data;  // ERROR in Rust

// Cantrip: Allowed, programmer's responsibility
var data = Data.new();
modify1(mut data);
modify2(mut data);  // OK in Cantrip
```

**Formal semantics:**
```
⟦mut T⟧ = { mutable reference to value of type T }
Operations: Read, Write
Aliasing: Unlimited (programmer ensures safety)
Lifetime: Lexical scope
```

### 26.5 Isolated Permission

**Unique reference for thread safety:**

```cantrip
record Atomic<T> {
    value: iso T,
}

impl Atomic<T> {
    function swap(self: Atomic<T>, new_value: iso T): iso T
    needs thread.atomic;
{
    atomic_swap(&self.value, new_value)
    }
}
```

**Type rule:**
```
[T-Iso]
Γ ⊢ e : T    unique(e) in Γ
──────────────────────────────
Γ ⊢ e : iso T
```

**Isolation rules:**
1. Guaranteed unique - exactly one `iso` reference
2. Cannot have any other references
3. Can be safely transferred between threads
4. Used for lock-free data structures

```cantrip
function send_to_thread(data: iso Data): JoinHandle<()>
    needs thread.spawn, alloc.heap;
{
    thread.spawn(move || {
        // data is isolated - safe to move between threads
        process(data);
    })
}
**Formal semantics:**
```
⟦iso T⟧ = { isolated reference to value of type T }
Operations: Read, Write, Move
Aliasing: Exactly one (enforced)
Thread-safety: Yes
```

### 26.6 Permission Subtyping

**Subtyping hierarchy:**
```
[Sub-Permission]
own T <: mut T <: T
iso T <: own T
```

```cantrip
function read_only(data: Data) {
    data.field
}

function example()
    needs alloc.heap;
{
    let owned = Data.new();        // own Data
        let mutable = mut owned;       // mut Data

        read_only(owned);     // own Data <: Data ✓
        read_only(mutable);   // mut Data <: Data ✓
}
---

## 27. Ownership and Transfer

### 27.1 Move Semantics

**Use `move` to explicitly transfer ownership:**

```cantrip
function consume(own data: Data) {
    // data destroyed at end of scope
}

function example()
    needs alloc.heap;
{
    let data = Data.new();
        consume(move data);  // Explicit transfer
        // data no longer accessible
        // data.field;  // ERROR E3004: value moved
}
```

**Type rule:**
```
[T-Move]
Γ ⊢ e : own T    Γ' = Γ \ {e}
──────────────────────────────
Γ ⊢ move e : own T
Γ' ⊬ e  (e no longer accessible)
```

**Operational semantics:**
```
[E-Move]
⟨e, σ⟩ ⇓ ⟨v, σ'⟩
────────────────────────────
⟨move e, σ⟩ ⇓ ⟨v, σ'⟩
σ'(e) = moved
```

### 27.2 Passing by Reference

**Default parameters take references without transferring ownership:**

```cantrip
function process(data: Data) {  // Takes reference
    std.io.println(data.field);
}

function example()
    needs alloc.heap;
{
    let data = Data.new();

        // Pass by reference - data still owned here
        process(data);
        process(data);
        process(data);

        // Still own data
        consume(move data);
}
```

**Type rule:**
```
[T-Ref-Pass]
Γ ⊢ e : own T
function f(x: T)
──────────────────────────────
f(e)  valid (temporary reference)
e still owned after call
```

### 27.3 Permission Transitions

**Valid transitions:**
```
own T → (pass by ref)  // Temporary reference
own T → own T          // Transfer with move
own T → iso T          // Convert to isolated
mut T → T              // Borrow immutably
iso T → own T          // Relax isolation
```

**Invalid transitions:**
```
T (ref) → own T    // ERROR: Cannot steal ownership
mut T → own T      // ERROR: Cannot steal from mut ref
T → iso T          // ERROR: Cannot prove unique
```

```cantrip
function example()
    needs alloc.heap;
{
    let owned = Data.new();

        // Can pass by reference
        read(owned);  // OK

        // Can pass mut reference
        modify(mut owned);  // OK

        // Must move to transfer
        consume(move owned);  // OK

        // Cannot use after move
        // read(owned);  // ERROR E3004
}
### 27.4 No Borrow Syntax

**Cantrip does NOT have explicit borrow blocks or lifetime annotations.**

Permissions are declared in function signatures:

```cantrip
// Just pass the value - permission determined by signature
function read(data: Data) { ... }      // Takes immutable ref
function modify(data: mut Data) { ... } // Takes mutable ref
function take(own data: Data) { ... }   // Takes ownership

// Usage
let data = Data.new();
read(data);          // Passes ref
modify(mut data);    // Passes mut ref
take(move data);     // Transfers ownership
```

**No lifetime parameters:**
```rust
// Rust
fn longest<'a>(x: &'a str, y: &'a str) -> &'a str { ... }

// Cantrip - no lifetimes needed
function longest(x: str, y: str): str {
    ...
}
```

**Lifetimes are determined by regions, not annotations.**

### 27.8 The Cantrip Safety Model: Trade-offs and Guarantees

**Purpose:** This section explicitly documents what Cantrip's memory model guarantees and what it leaves to programmer responsibility. Understanding these trade-offs is critical for writing safe code.

#### 27.8.1 What Cantrip Guarantees (✅ Compile-Time Safe)

**1. No Use-After-Free**

Regions enforce LIFO deallocation. Once a region ends, all allocations within it are freed. The compiler prevents returning region-allocated data.

```cantrip
function guaranteed_safe()
    needs alloc.region;
{
    region temp {
        let data = Vec.new_in<temp>();
        process(data);  // OK: data valid within region
    } // data freed here

    // Impossible to access data after region ends
}
```

**2. No Double-Free**

Regions handle cleanup automatically. Manual deallocation is not exposed.

```cantrip
function automatic_cleanup()
    needs alloc.region;
{
    region temp {
        let data1 = Data.new_in<temp>();
        let data2 = Data.new_in<temp>();
    } // Both freed exactly once, automatically
}
```

**3. No Memory Leaks**

Deterministic destruction at region boundaries. All allocations in a region are freed when the region ends.

```cantrip
function no_leaks()
    needs alloc.region;
{
    for i in 0..1000 {
        region iteration {
            let large = Vec.with_capacity_in<iteration>(10000);
            process(large);
        } // Freed each iteration
    }
}
```

**4. No Dangling Pointers**

Cannot return pointers to region-allocated data.

```cantrip
function prevented_at_compile_time(): Vec<i32>
    needs alloc.region;
{
    region temp {
        let vec = Vec.new_in<temp>();
        vec  // ❌ ERROR E5001: Cannot return region data
    }
}
```

#### 27.8.2 What Cantrip Does NOT Guarantee (⚠️ Programmer Responsibility)

**1. No Aliasing Prevention**

Multiple `mut` references to the same data are allowed. Programmer must ensure they don't conflict.

```cantrip
function aliasing_allowed()
    needs alloc.heap;
{
    var data = Vec.new();

    let ref1 = mut data;  // First mutable reference
    let ref2 = mut data;  // Second mutable reference - ALLOWED

    ref1.push(1);  // Modifies through ref1
    ref2.push(2);  // Modifies through ref2

    // ⚠️ PROGRAMMER MUST ENSURE: ref1 and ref2 don't cause conflicts
    // No compile-time enforcement
}
```

**Compare to Rust:**

```rust
// Rust prevents this at compile time
fn rust_prevents() {
    let mut data = Vec::new();
    let ref1 = &mut data;  // First mutable borrow
    let ref2 = &mut data;  // ❌ ERROR: cannot borrow as mutable twice
}
```

**2. No Data Race Prevention**

Cantrip allows concurrent mutable access. Use `iso` types or synchronization primitives.

```cantrip
function data_race_possible()
    needs thread.spawn, alloc.heap;
{
    var shared = Vec.new();

    thread.spawn(|| {
        shared.push(1);  // ⚠️ Concurrent modification
    });

    shared.push(2);  // ⚠️ Concurrent modification

    // Programmer must add synchronization
}
```

**Safe alternative with `iso`:**

```cantrip
function data_race_prevented()
    needs thread.spawn, alloc.heap;
{
    let isolated: iso Vec<i32> = Vec.new();

    let handle = thread.spawn(move || {
        isolated.push(1);  // Exclusive access via iso
    });

    let isolated = handle.join();
    isolated.push(2);
}
```

**3. No Iterator Invalidation Prevention**

Can modify collections while iterating. Programmer must ensure safety.

```cantrip
function iterator_invalidation_possible()
    needs alloc.heap;
{
    var numbers = Vec.from_array([1, 2, 3, 4, 5]);

    for n in numbers {
        numbers.push(n * 2);  // ⚠️ Modifies while iterating
        // Behavior is undefined - programmer's responsibility
    }
}
```

**Safe pattern:**

```cantrip
function iterator_safe()
    needs alloc.heap;
{
    let numbers = Vec.from_array([1, 2, 3, 4, 5]);
    var new_numbers = Vec.new();

    for n in numbers {
        new_numbers.push(n);
        new_numbers.push(n * 2);  // ✅ Modifies different collection
    }
}
```

---

## 28. Memory Regions

### 28.1 Region Overview

**Definition 28.1 (Memory Region):** A region is a lexically-scoped memory arena enabling fast allocation and O(1) bulk deallocation.

**Benefits:**
- **LIFO deallocation** - Inner regions freed before outer
- **Zero-overhead allocation** - Pointer bump (3-5 CPU cycles vs 50-100 for malloc)
- **Bulk deallocation** - O(1) regardless of allocation count
- **Explicit lifetimes** - Region boundaries visible in code
- **Cache-friendly** - Contiguous allocations

**Key insight:** Regions control WHEN memory is freed, not WHO can access it.

### 28.2 Three Allocation Strategies Compared

**1. Stack (automatic, fastest):**
```cantrip
function compute(x: i32) {
    let local = x * 2;  // Stack allocated automatically
    local
}
```
- **Speed:** Instantaneous (register adjustment)
- **Lifetime:** Function scope
- **Size:** Limited (typically MB range)
- **Use:** Local variables, small temporary data

**2. Heap (explicit, flexible):**
```cantrip
function create_user(name: String): own User
    needs alloc.heap;
{
    own User { name }  // Heap allocated
}
```
- **Speed:** 50-100 CPU cycles
- **Lifetime:** Until explicitly freed
- **Size:** Large (GB range)
- **Use:** Long-lived data, unknown lifetime, return values

**3. Region (explicit, fast bulk deallocation):**
```cantrip
function parse_file(path: str): Result<Data, Error>
    needs alloc.region;
{
    region temp {
        let tokens = lex_in<temp>(read(path));
        let ast = parse_in<temp>(tokens);
        Ok(optimize(ast))
    } // Bulk free: O(1)
}
```
- **Speed:** 3-5 CPU cycles (10-20x faster than heap)
- **Lifetime:** Region scope (LIFO)
- **Size:** Large (configurable)
- **Use:** Temporary data, batch processing, compilation phases

### 28.3 Region Declaration

**Syntax:**
```cantrip
region name {
    // Region body
    // All allocations in this scope use region
}
// Region destroyed here (O(1))
```

```cantrip
function process_data(input: String): Result<Data, Error>
    needs alloc.region;
    requires !input.is_empty();
{
    region temp {
        // All allocations use 'temp' region
        let buffer = Buffer.new_in<temp>(1024);
        let parser = Parser.new_in<temp>();

        let data = parser.parse(buffer, input)?;

        // Must convert to heap before region ends
        Ok(data.to_heap())
    } // All temp memory freed in O(1)
}
**Type rule:**
```
[T-Region]
Γ, r : Region ⊢ e : T ! (ε ∪ alloc.region)
────────────────────────────────────────────
Γ ⊢ region r { e } : T ! ε
```

### 28.4 Region Lifetimes

**Regions enforce strict LIFO hierarchy:**

```cantrip
function nested_regions()
    needs alloc.region;
{
    region outer {
            let outer_data = Vec.new_in<outer>();

            region inner {
                let inner_data = Vec.new_in<inner>();

                // CAN reference outer from inner
                outer_data.push(42);

                // CANNOT return inner_data (would escape region)
            } // inner freed here

            // inner_data no longer accessible
            outer_data.push(100);

        } // outer freed here
}
```

**Lifetime rules:**
1. Inner regions deallocate before outer (LIFO)
2. Inner regions CAN reference outer regions
3. Outer regions CANNOT reference inner regions
4. Cannot return region-allocated data

**Formal semantics:**
```
[Region-LIFO]
r₂ nested in r₁
──────────────────────
dealloc(r₂) <ʜᴀᴘᴘᴇɴs-ʙᴇғᴏʀᴇ dealloc(r₁)

[Region-Escape]
alloc(v, r)    e returns v    e in region r
──────────────────────────────────────────
ERROR E5001: Cannot return region data
```

### 28.5 Region Allocation Syntax

**Allocate into specific regions using `_in<region>`:**

```cantrip
region temp {
    // Standard library support
    let vec = Vec.new_in<temp>();
    let string = String.new_in<temp>();
    let buffer = Buffer.with_capacity_in<temp>(1024);

    // Custom types
    let data = Data.new_in<temp>(args);
}
```

**Type rule:**
```
[T-Alloc-In-Region]
Γ, r : Region ⊢ e : T
──────────────────────────────────
Γ ⊢ Type.new_in<r>(e) : T
alloc(result, r)
```

### 28.6 Implementing Region Allocation

**Custom types can support region allocation:**

```cantrip
record Data {
    values: Vec<i32>,
}

impl Data {
    // Heap allocation
    function new(size: usize): own Data
    needs alloc.heap;
{
    own Data {
                values: Vec.with_capacity(size)
            }
    }

    // Region allocation
    function new_in<'r>(size: usize): own Data
        needs alloc.region;
    {
        own Data {
            values: Vec.with_capacity_in<'r>(size)
        }
    }

    // Convert region-allocated to heap
    function to_heap(self: own Data): own Data
        needs alloc.heap;
    {
        self.clone()  // Deep copy to heap
    }
}
```

**Convention:** Types supporting region allocation provide:
- `new_in<'r>()` - Allocate in region
- `to_heap()` - Convert to heap-allocated

### 28.7 Region vs Heap Decision Guide

**When to use heap:**
- Long-lived data
- Return values
- Unknown lifetime
- Data structure with cycles
- Shared ownership needed

**When to use regions:**
- Temporary data
- Batch processing
- Parsing/compilation phases
- Frame-local game data
- Known short lifetime

**Example - Request/response processing:**
```cantrip
function handle_request(request: Request): Response
    needs alloc.region, alloc.heap;
{
    region request_scope {
            // Temporary parsing data
            let parsed = parse_request_in<request_scope>(request);
            let data = fetch_data_in<request_scope>(parsed);

            // Long-lived result escapes region
            Response.from_data(data)  // Heap allocated
        } // Temporary data freed
}
```

### 28.8 Performance Characteristics

**Allocation speed:**
```
malloc/free:  50-100 CPU cycles
Region bump:  3-5 CPU cycles     (10-20x faster)
Stack:        ~1 CPU cycle       (register adjustment)
```

**Deallocation:**
```
Individual free: O(n) for n allocations
Region bulk:     O(1) regardless of n
Stack:           O(1) (automatic)
```

**Memory layout - Arena with exponential pages:**
```
Region: temp
├── Page 1 (4 KB)    [████████████▒▒▒▒] 75% full
├── Page 2 (8 KB)    [██████▒▒▒▒▒▒▒▒▒▒] 30% full
└── Page 3 (16 KB)   [█▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒]  5% full
                      ↑
                      allocation_ptr
```

**Algorithm:**
1. Check current page has space
2. If yes: bump pointer, return (fast path - 3-5 cycles)
3. If no: allocate new page (2x size), add to list
4. Amortized O(1)

### 28.9 Common Patterns

**Request/response processing:**
```cantrip
function handle_request(request: Request): Response
    needs alloc.region;
{
    region request_scope {
            let parsed = parse_request_in<request_scope>(request);
            let data = fetch_data_in<request_scope>(parsed);
            Response.from_data(data)  // Heap allocated
        } // Temporary data freed
}
```

**Batch processing:**
```cantrip
function process_batch(items: Vec<Data>): Vec<Result>
    needs alloc.region;
{
    var results = Vec.new();

    region batch {
        for item in items {
            let temp = transform_in<batch>(item);
            results.push(finalize(temp));
        }
    } // Batch temps freed

    results
}
```

**Parsing/compilation:**
```cantrip
function compile(source: str): Program
    needs alloc.region;
{
    region parsing {
            let tokens = lex_in<parsing>(source);
            let ast = parse_in<parsing>(tokens);

            region analysis {
                let analyzed = analyze_in<analysis>(ast);
                codegen(analyzed)  // Result on heap
            } // Analysis freed
        } // Parsing freed
}
```

**Game engine frame:**
```cantrip
function game_tick(state: mut GameState, dt: f32)
    needs alloc.region;
{
    region frame {
            let entities = update_entities_in<frame>(state, dt);
            let particles = update_particles_in<frame>(state, dt);
            render(entities, particles);
        } // O(1) bulk free - perfect for 60 FPS
}
```

### 28.10 Safety Restrictions

**Cannot escape region:**
```cantrip
function unsafe_example(): Vec<i32>
    needs alloc.region;
{
    region temp {
        let vec = Vec.new_in<temp>();
        vec  // ERROR E5001: Cannot return region data
    }
}
```

**Must convert to heap:**
```cantrip
function safe_example(): Vec<i32>
    needs alloc.region, alloc.heap;
{
    region temp {
        let vec = Vec.new_in<temp>();
        vec.to_heap()  // OK: Converts to heap
    }
}
```

**Permissions still apply:**
```cantrip
function example()
    needs alloc.region;
{
    region temp {
            let data = Data.new_in<temp>();

            // Can pass refs multiple times - no borrow checking
            read(data);
            modify(mut data);
            read(data);
        } // data freed
}
```

---

## 29. Region Formal Semantics

### 29.1 Region Algebra

**Definition 29.1 (Region):** A region r is a tuple (id, allocations, parent).

```
Region = (RegionId, Set<Allocation>, Option<RegionId>)

where:
  RegionId = unique identifier
  Allocation = (address, size, type)
  parent = enclosing region (if nested)
```

**Region hierarchy:**
```
[Region-Hierarchy]
r₁ = (id₁, A₁, None)        // Top-level region
r₂ = (id₂, A₂, Some(id₁))   // Nested region
r₃ = (id₃, A₃, Some(id₂))   // Deeper nesting
```

### 29.2 Allocation Rules

**[Alloc-Heap]**
```
⟨alloc(size), σ, alloc.heap⟩ → ⟨ℓ, σ[ℓ ↦ uninit(size)], alloc.heap⟩
```

**[Alloc-Region]**
```
⟨alloc_in<r>(size), σ, alloc.region⟩ → ⟨ℓ, σ[ℓ ↦ uninit(size), r], alloc.region⟩
```

**[Alloc-Region-Bump]**
```
r.current_page.has_space(size)
────────────────────────────────────
alloc_in<r>(size) = {
    let ptr = r.current_page.ptr;
    r.current_page.ptr += size;
    ptr
}
```

### 29.3 Deallocation Rules

**[Dealloc-Heap]**
```
ℓ ∈ Heap
────────────────────────────
⟨free(ℓ), σ⟩ → σ \ {ℓ}
```

**[Dealloc-Region]**
```
r = (id, allocs, parent)
────────────────────────────────────
⟨end region r, σ⟩ → σ \ allocs    // O(1)
```

**[Dealloc-Region-Nested]**
```
r₂ nested in r₁
dealloc(r₂) happens before dealloc(r₁)
──────────────────────────────────────
LIFO ordering enforced
```

### 29.4 Escape Analysis

**Definition 29.2 (Escape Analysis):** Determine if a value allocated in region r escapes the region's scope.

**Escape rules:**
```
[Escape-Return]
alloc(v, r)    return v in region r
───────────────────────────────────
ERROR: value escapes region

[Escape-Store]
alloc(v, r)    store v in global/heap
──────────────────────────────────────
ERROR: value escapes region

[No-Escape-Local]
alloc(v, r)    v only used within region r
──────────────────────────────────────────
OK: value does not escape
```

**Compiler verification:**
```
check_region_escape(region r, expr e) = {
    for alloc in r.allocations {
        if escapes(alloc, e) {
            error("Cannot return region data");
        }
    }
}

escapes(alloc, e) = {
    return alloc ∈ returned_values(e)
        || alloc ∈ stored_globally(e)
        || alloc ∈ captured_by_closure(e)
}
```

### 29.5 Region Typing

**[T-Region-Alloc]**
```
Γ, r : Region ⊢ e : T
────────────────────────────
Γ ⊢ alloc_in<r>(e) : T@r

where T@r means "T allocated in region r"
```

**[T-Region-Escape]**
```
Γ ⊢ e : T@r    e escapes region r
───────────────────────────────────
Type error
```

**[T-Region-Convert]**
```
Γ ⊢ e : T@r
────────────────────────────
Γ ⊢ e.to_heap() : T@heap
```

### 29.6 Memory Model

**Definition 29.3 (Memory State):** The memory state σ consists of heap, regions, and stack.

```
σ = (Heap, {r₁, r₂, ..., rₙ}, Stack)

where:
  Heap = {(addr, value) | addr ∈ HeapAddrs}
  rᵢ = (id, allocations, parent)
  Stack = {(addr, value, scope) | addr ∈ StackAddrs}
```

**Axiom 29.1 (Region Disjointness):**
```
∀r₁, r₂. r₁ ≠ r₂ ⟹ allocations(r₁) ∩ allocations(r₂) = ∅
```

**Axiom 29.2 (Region LIFO):**
```
r₂ nested in r₁ ⟹ lifetime(r₂) ⊂ lifetime(r₁)
```

**Axiom 29.3 (No Dangling):**
```
alloc(v, r) ∧ dealloc(r) →ʜᴀᴘᴘᴇɴs-ʙᴇғᴏʀᴇ use(v) ⟹ ERROR
```

### 29.7 Happens-Before Relation

**Definition 29.4 (Happens-Before for Regions):**

```
e₁ →ʜʙ e₂  (event e₁ happens before e₂)

Rules:
1. Program order: e₁ <ᴘ e₂ ⟹ e₁ →ʜʙ e₂
2. Region nesting: alloc(v, r₂) ∧ r₂ in r₁ ⟹ alloc(v, r₁) →ʜʙ alloc(v, r₂)
3. Region end: alloc(v, r) →ʜʙ dealloc(r)
4. Transitivity: e₁ →ʜʙ e₂ ∧ e₂ →ʜʙ e₃ ⟹ e₁ →ʜʙ e₃
```

**Memory safety theorem:**

**Theorem 29.1 (Region Memory Safety):**

If alloc(v, r) →ʜʙ dealloc(r) →ʜʙ use(v), then ERROR.

**Proof:** By construction of region semantics and happens-before relation.

---
# Part VIII: Module System

## 30. Modules and Code Organization

### 30.1 File-Based Module System

**Definition 30.1 (Module):** A module is a namespace unit defined by a source file, where the module path is determined by the file's location in the project structure.

**Key principle:** There is **no `namespace` keyword**. Module structure follows directory structure.

**Project structure:**
```
myproject/
├── cantrip.toml
└── src/
    ├── main.cantrip              # Module: main
    ├── http.cantrip              # Module: http
    ├── math/
    │   ├── geometry.cantrip      # Module: math.geometry
    │   └── algebra.cantrip       # Module: math.algebra
    └── data/
        ├── structures.cantrip    # Module: data.structures
        └── algorithms.cantrip    # Module: data.algorithms
```

**Formal definition:**
```
Module = (Path, Declarations, Exports)

where:
  Path = sequence of identifiers (e.g., math.geometry)
  Declarations = set of items defined in module
  Exports = subset of Declarations with public visibility
```

### 30.2 Module Definition

**Each file automatically defines a module based on its path:**

```cantrip
// File: src/math/geometry.cantrip
// Module path: math.geometry

public function area_of_circle(radius: f64): f64
    requires radius > 0.0;
    ensures result >= 0.0;
{
    3.14159 * radius * radius
}

public function area_of_rectangle(width: f64, height: f64): f64
    requires
        width > 0.0;
        height > 0.0;
    ensures result >= 0.0;
{
    width * height
}

function internal_helper(): i32 {  // Not exported
    42
}
```

**Type rule:**
```
[T-Module]
File at path P defines items I₁, ..., Iₙ
────────────────────────────────────────────
Module M = (module_path(P), {I₁, ..., Iₙ}, exports({I₁, ..., Iₙ}))
```

**Path resolution:**
```
module_path(src/main.cantrip) = main
module_path(src/http.cantrip) = http
module_path(src/http/client.cantrip) = http.client
module_path(src/a/b/c/deep.cantrip) = a.b.c.deep
```

### 30.3 Module Metadata

**Optional metadata using the `#[module]` attribute:**

```cantrip
// File: src/math/geometry.cantrip

#[module(
    purpose = "Geometric calculations for 2D and 3D shapes",
    version = "1.2.0",
    requires = ["std.math"],
    deprecated = false
)]

public function area_of_circle(radius: f64): f64
    requires radius > 0.0;
    ensures result >= 0.0;
{
    3.14159 * radius * radius
}
```

**Metadata fields:**
- `purpose: str` - Human-readable description
- `version: str` - Semantic version (follows semver)
- `requires: [str]` - Module dependencies
- `deprecated: bool | str` - Deprecation status/message
- `authors: [str]` - Module authors
- `license: str` - License identifier

**Type rule:**
```
[T-Module-Metadata]
#[module(metadata)] precedes module definition
metadata well-formed
────────────────────────────────────────────
Module M has metadata
```

### 30.4 Module Resolution Algorithm

**Definition 30.2 (Module Resolution):** The process of mapping a module path to a source file.

**Algorithm:**
```
resolve_module(path: ModulePath) -> FilePath {
    1. Split path by '.' separator
    2. Join with OS path separator
    3. Prepend 'src/'
    4. Append '.cantrip'
    5. Check file exists
    6. Return absolute path
}
```

```
resolve_module("main")
  → src/main.cantrip

resolve_module("http.client")
  → src/http/client.cantrip

resolve_module("data.structures.linked_list")
  → src/data/structures/linked_list.cantrip
**Formal specification:**
```
[Module-Resolution]
path = id₁.id₂.....idₙ
file = src/id₁/id₂/.../idₙ.cantrip
file exists
────────────────────────────────
resolve(path) = file
```

### 30.5 Standard Library Modules

**Standard library uses the `std` prefix:**

```cantrip
import std.io.File;
import std.collections.Vec;
import std.collections.HashMap;
import std.fs.read_to_string;
import std.net.http.Client;
```

**Standard library structure:**
```
std/
├── collections.cantrip       # std.collections
├── io.cantrip               # std.io
├── fs.cantrip               # std.fs
├── string.cantrip           # std.string
├── net/
│   ├── http.cantrip         # std.net.http
│   └── tcp.cantrip          # std.net.tcp
├── sync/
│   ├── mutex.cantrip        # std.sync.mutex
│   └── channel.cantrip      # std.sync.channel
└── math.cantrip             # std.math
```

**Resolution:**
```
resolve_std_module(path: ModulePath) -> FilePath {
    assert(path.starts_with("std."), "path must start with std.");
    let relative = path.strip_prefix("std.");
    return stdlib_root / relative.replace('.', '/') + ".cantrip";
}
```

### 30.6 Modules vs Regions

**Critical distinction:**

| Aspect | Modules | Regions |
|--------|---------|---------|
| **Purpose** | Code organization | Memory management |
| **Lifetime** | Compile-time only | Runtime memory scope |
| **Scope** | File/package level | Function-local blocks |
| **Syntax** | File path (implicit) | `region name { }` |
| **Visibility** | `public`/`private` | N/A (all local) |
| **Imports** | Yes (`import`) | No |
| **Nesting** | Directory hierarchy | Lexical block nesting |

**Example showing both:**
```cantrip
// File: src/parser/lexer.cantrip
// Module: parser.lexer (from file path)

#[module(
    purpose = "Lexical analysis",
    version = "1.0.0"
)]

public function tokenize(source: str): Vec<Token>
    needs alloc.region;
    requires !source.is_empty();
{
    // REGION: Runtime memory scope
    region temp {
        let chars = source.chars_in<temp>();
        let tokens = Vec.new();

        for ch in chars {
            tokens.push(classify(ch));
        }

        tokens  // Heap-allocated, survives region
    } // temp memory freed here (O(1) bulk deallocation)
}
```

**Key insight:**
- **Modules** organize code spatially (files and directories)
- **Regions** organize memory temporally (creation and destruction order)

---

## 31. Import and Export Rules

### 31.1 Import Syntax

**Definition 31.1 (Import):** An import statement makes items from another module accessible in the current scope.

**Syntax forms:**

**1. Import specific item:**
```cantrip
import math.geometry.area_of_circle;

let area = area_of_circle(5.0);
```

**2. Import all public items:**
```cantrip
import math.geometry.*;

let area = area_of_circle(5.0);
let rect = area_of_rectangle(10.0, 20.0);
```

**3. Import with alias:**
```cantrip
import math.geometry.area_of_circle as circle_area;

let area = circle_area(5.0);
```

**4. Import module:**
```cantrip
import math.geometry;

let area = geometry.area_of_circle(5.0);
```

**Type rules:**

```
[T-Import-Item]
module M exports item I with type T
────────────────────────────────────
import M.I  adds  I : T  to current scope

[T-Import-Wildcard]
module M exports items {I₁ : T₁, ..., Iₙ : Tₙ}
────────────────────────────────────────────────
import M.*  adds  {I₁ : T₁, ..., Iₙ : Tₙ}  to current scope

[T-Import-Alias]
module M exports item I with type T
────────────────────────────────────
import M.I as J  adds  J : T  to current scope
```

### 31.2 Import Resolution

**Definition 31.2 (Import Resolution):** The process of finding and validating imported items.

**Algorithm:**
```
resolve_import(import_stmt: Import) -> Result<Item, Error> {
    1. Parse module path from import
    2. Resolve module path to file
    3. Parse module to get exports
    4. Check imported item is exported
    5. Check visibility permissions
    6. Add to current scope with appropriate name
}
```

```cantrip
import math.geometry.area_of_circle;

// Resolution steps:
// 1. Module path: math.geometry
// 2. File: src/math/geometry.cantrip
// 3. Parse and extract exports
// 4. Verify area_of_circle is public
// 5. Add to scope: area_of_circle : f64 -> f64
**Error cases:**
```cantrip
import math.geometry.nonexistent;
// ERROR E2001: Item 'nonexistent' not found in module 'math.geometry'

import math.geometry.internal_helper;
// ERROR E2002: Item 'internal_helper' is private

import nonexistent.module.item;
// ERROR E2003: Module 'nonexistent.module' not found
```

### 31.3 Re-exports

**Definition 31.3 (Re-export):** A module can re-export items from other modules to create a unified public API.

**Syntax:**
```cantrip
public import source.module.item;
```

```cantrip
// File: src/collections.cantrip
// Module: collections

// Re-export from other modules
public import data.structures.Vec;
public import data.structures.HashMap;
public import data.structures.HashSet;
public import data.algorithms.sort;

// Now users can do:
// import collections.Vec;
// Instead of:
// import data.structures.Vec;
**Type rule:**
```
[T-Reexport]
module M₁ exports item I with type T
module M₂ contains: public import M₁.I
────────────────────────────────────────
M₂ exports I : T
```

**Use cases:**
1. **Facade pattern:** Hide internal organization
2. **Deprecation:** Maintain old paths while reorganizing
3. **Convenience:** Create logical groupings

### 31.4 Import Cycles

**Definition 31.4 (Import Cycle):** A circular dependency where module A imports from B and B imports from A (directly or transitively).

**Detection:**
```
[Cycle-Detection]
M₁ imports M₂, M₂ imports M₃, ..., Mₙ imports M₁
────────────────────────────────────────────────
ERROR: Import cycle detected
```

```cantrip
// File: src/a.cantrip
import b.function_b;

public function function_a() {
    function_b()
}

// File: src/b.cantrip
import a.function_a;  // ERROR: Import cycle

public function function_b() {
    function_a()
}
**Error message:**
```
error[E2004]: Import cycle detected
  --> src/b.cantrip:1:1
   |
 1 | import a.function_a;
   | ^^^^^^^^^^^^^^^^^^^^ import creates cycle
   |
   = note: Cycle: a -> b -> a
   = help: Consider using dependency injection or splitting modules
```

**Solutions:**
1. Introduce a third module for shared functionality
2. Restructure dependencies
3. Use dependency injection

### 31.5 Selective Imports

**Import specific items to avoid namespace pollution:**

```cantrip
// Instead of:
import std.collections.*;
let vec = Vec.new();
let map = HashMap.new();

// Prefer:
import std.collections.Vec;
import std.collections.HashMap;
let vec = Vec.new();
let map = HashMap.new();
```

**Rationale:**
- **Clarity:** Explicit about what's used
- **Compile time:** Faster (fewer symbols to resolve)
- **Maintainability:** Easy to see dependencies

### 31.6 Import Ordering

**Convention (not enforced):**
```cantrip
// 1. Standard library
import std.io.File;
import std.collections.Vec;

// 2. External dependencies
import http.Client;
import json.parse;

// 3. Internal modules
import utils.helpers;
import models.User;
```

---

## 32. Visibility and Access Control

### 32.1 Visibility Modifiers

**Definition 32.1 (Visibility):** Visibility determines which modules can access an item.

**Three visibility levels:**

```cantrip
public      // Accessible everywhere
internal    // Accessible within package (default)
private     // Accessible within module only
```

**Type rules:**

```
[Vis-Public]
public item I in module M
M₂ imports M
────────────────────────
M₂ can access I

[Vis-Internal]
internal item I in module M₁
M₂ in same package as M₁
────────────────────────────
M₂ can access I

[Vis-Private]
private item I in module M
────────────────────────────
Only M can access I
```

### 32.2 Public Items

**Public items are accessible from any module:**

```cantrip
// File: src/math/geometry.cantrip

public record Point {
    public x: f64;
    public y: f64;
}

public function distance(p1: Point, p2: Point): f64 {
    let dx = p1.x - p2.x;
        let dy = p1.y - p2.y;
        (dx * dx + dy * dy).sqrt()
}
```

**Usage from other modules:**
```cantrip
// File: src/main.cantrip
import math.geometry.Point;
import math.geometry.distance;

function main() {
    let p1 = Point { x: 0.0, y: 0.0 };
    let p2 = Point { x: 3.0, y: 4.0 };
    let d = distance(p1, p2);
}
```

### 32.3 Internal Items

**Internal visibility (default) - accessible within the same package:**

```cantrip
// File: src/utils/helpers.cantrip

record Config {  // Internal by default
    data: String;
}

function parse_config(source: String): Config {  // Internal
    // Implementation
}
```

**Package boundary:**
```
Package = {all modules under src/}

Same package: src/a.cantrip and src/b/c.cantrip
Different package: external dependency
```

**Type rule:**
```
[Vis-Internal-Same-Package]
item I in module M₁
M₂ in same package as M₁
────────────────────────────
M₂ can access I (if internal or public)
```

### 32.4 Private Items

**Private items are only accessible within the defining module:**

```cantrip
// File: src/database/connection.cantrip

private function validate_credentials(user: str, pass: str): bool {
    // Only accessible within this file
}

public function connect(user: str, pass: str): Result<Connection, Error> {
    if !validate_credentials(user, pass) {
        return Err(Error.InvalidCredentials);
    }
    // ...
}
```

**Error on access:**
```cantrip
// File: src/main.cantrip
import database.connection.validate_credentials;
// ERROR E2002: Item 'validate_credentials' is private
```

### 32.5 Record Field Visibility

**Fields can have different visibility:**

```cantrip
public record User {
    public name: String;        // Public field
    public email: String;       // Public field
    private password_hash: String;  // Private field
    internal user_id: u64;      // Internal field
}

// In same module
function update_password(user: mut User, hash: String) {
    user.password_hash = hash;  // OK: private access in same module
}

// In different module
function external_access(user: mut User) {
    let name = user.name;  // OK: public
    // let hash = user.password_hash;  // ERROR: private field
}
```

**Type rule:**
```
[Field-Access]
record S { vis f: T }
access to s.f from module M
────────────────────────────
allowed iff visibility(f, M) permits
```

### 32.6 Procedure Visibility

**Procedures inherit record visibility by default but can be more restrictive:**

```cantrip
public record Counter {
    private value: i32;

    public function new(): own Counter {
    own Counter { value: 0 }
    }

    public function increment(self: mut Counter) {
        self.value += 1;
    }

    private function unsafe_set(self: mut Counter, val: i32) {
        self.value = val;  // Private: only for internal use
    }
}
```

### 32.7 Trait Implementation Visibility

**Trait implementations are public by definition:**

```cantrip
trait Drawable {
    function draw(self: Self);
}

record Shape {
    // ...
}

// Implementation is public
impl Drawable for Shape {
    function draw(self: Shape) {
        // ...
    }
}
```

**Rationale:** Traits define public contracts, so implementations must be accessible.

---

## 33. Module Resolution

### 33.1 Resolution Rules

**Definition 33.1 (Module Resolution):** The process of mapping import statements to source files and validating accessibility.

**Resolution algorithm:**
```
resolve(import_path: Path) -> Result<Module, Error> {
    // 1. Parse import path
    let segments = import_path.split('.');

    // 2. Check if standard library
    if segments[0] == "std" {
        return resolve_std_module(segments);
    }

    // 3. Resolve to source file
    let file_path = src_root / segments.join('/') + ".cantrip";

    // 4. Check file exists
    if !file_path.exists() {
        return Err(ModuleNotFound(import_path));
    }

    // 5. Parse module
    let module = parse_file(file_path)?;

    // 6. Return resolved module
    Ok(module)
}
```

### 33.2 Search Path

**Module search order:**

1. **Current package** (`src/`)
2. **Standard library** (`std/`)
3. **Dependencies** (from `arc.toml`)

```cantrip
import collections.Vec;

// Search order:
// 1. src/collections.cantrip (or src/collections/vec.cantrip)
// 2. std/collections.cantrip
// 3. dependencies/collections/src/vec.cantrip
### 33.3 Ambiguity Resolution

**If multiple modules match:**

```cantrip
// src/data.cantrip exists
// dependency 'data' also exists

import data.process;

// ERROR E2005: Ambiguous import 'data'
// note: Could refer to:
//   - src/data.cantrip (local module)
//   - data v1.0 (dependency)
// help: Use full qualification:
//   - ::data.process (local)
//   - extern::data.process (dependency)
```

**Disambiguation syntax:**
```cantrip
import ::data.process;        // Local module
import extern::data.process;  // External dependency
```

### 33.4 Package Configuration

**Package manifest (`cantrip.toml`):**

```toml
[package]
name = "myproject"
version = "1.0.0"
authors = ["Your Name <you@example.com>"]
edition = "0.4.0"

[dependencies]
http = "2.0"
json = { version = "1.5", features = ["pretty"] }
```

**Dependency resolution:**
```
resolve_dependency(name: str, version: str) -> Path {
    let dep_path = dependencies_dir / name / version;
    return dep_path / "src";
}
```

### 33.5 Module Cache

**Compiler maintains a module cache:**

```
ModuleCache = HashMap<ModulePath, CompiledModule>

where:
  ModulePath = unique identifier
  CompiledModule = (AST, TypeInfo, Exports)
```

**Benefits:**
- Avoid re-parsing same module
- Incremental compilation
- Faster build times

**Cache invalidation:**
- Source file modified
- Dependency updated
- Compiler flags changed

---

# Part IX: Advanced Features

## 34. Compile-Time Programming

### 34.6 Opt‑In Reflection

Cantrip supports *opt‑in* reflection for types explicitly marked as reflective. Reflection is split into
compile‑time introspection (§34.3) and **runtime reflection** gated by effects and attributes.

**Enabling reflection:**
```cantrip
#[reflect]          // opt-in on type
public record User { name: String; id: u64; }
```
If a type lacks `#[reflect]`, runtime reflection on it fails with `E9601`.

**Effect family:**
```cantrip
reflect.read    // read type/field metadata at runtime
reflect.invoke  // invoke methods by name through dynamic dispatch
```
Using runtime reflection requires a `needs` clause including `reflect.read` and/or `reflect.invoke`.

**Runtime API (stdlib sketch):**
```cantrip
module std.reflect

public trait Any { function type_id(self: Self): u128; }

public record Mirror<T> {
    // opaque handle, only constructed for #[reflect] types
}

public function mirror_of<T>(value: T): Result<Mirror<T>, Error>
    needs reflect.read;

impl<T> Mirror<T> {
    public function fields(self: Mirror<T>): Vec<(String, TypeInfo)>
        needs reflect.read;

    public function get(self: Mirror<T>, name: str): Option<Value>
        needs reflect.read;

    public function call(self: Mirror<T>, name: str, args: [Value]): Result<Value, Error>
        needs reflect.invoke;
}
```

**Privacy & safety:**
- Reflection respects visibility: private fields are inaccessible from other modules.
- Reflection does not bypass the effect system; reflective calls contribute the callee’s effects.
- No heap allocation is implied; any allocation must be explicitly declared by the API call sites.

**Diagnostics:**
- `E9601` — reflection on a non‑`#[reflect]` type.
- `E9602` — unknown member in `reflect.get/call`.
- `E9603` — effect mismatch when invoking reflected member.

### 34.1 Comptime Keyword

**Definition 34.1 (Compile-Time Evaluation):** Code within `comptime` blocks executes during compilation.

**Syntax:**
```cantrip
comptime {
    // Executed at compile time
    expression
}
```

```cantrip
function example(): i32 {
    comptime {
            const SIZE = compute_size();
        }

        let arr = [0; SIZE];  // SIZE known at compile time
        arr.length() as i32
}
**Type rule:**
```
[T-Comptime]
Γ ⊢ e : T    effects(e) = ∅    e is constant expression
─────────────────────────────────────────────────────────
Γ ⊢ comptime { e } : T
compile_time_eval(e) succeeds
```

### 34.2 Const Functions

**Functions callable at compile time:**

```cantrip
function factorial(n: u32): u64
    requires n <= 20;
{
    if n == 0 { 1 } else { n as u64 * factorial(n - 1) }
}

comptime {
    const FACT_10 = factorial(10);  // Computed at compile time
}
```

**Requirements for const functions:**
1. Pure (no effects)
2. All parameters must be compile-time constants
3. No mutable state
4. Finite recursion depth

### 34.3 Type Introspection

**Query type properties at compile time:**

```cantrip
@typeInfo(T) -> TypeInfo
@typeName(T) -> str
@hasField(T, "field") -> bool
@sizeOf(T) -> usize
@alignOf(T) -> usize
@isNumeric(T) -> bool
@isCopyable(T) -> bool
```

```cantrip
function serialize<T>(value: T): String {
    comptime {
            if @hasField(T, "serialize") {
                return value.serialize();
            } else {
                return default_serialize(value);
            }
        }
}
### 34.4 Code Generation

**Generate types and functions at compile time:**

```cantrip
function Array(comptime T: type, comptime N: usize): type {
    record {
        data: [T; N];

        function new(): own Self {
            own Self { data: [T::default(); N] }
        }

        function length(self: Self): usize {
            N
        }
    }
}

// Usage
type IntArray10 = Array(i32, 10);
let arr = IntArray10.new();
```

### 34.5 Comptime Type Introspection

**Requirement:** Implementations MUST provide compile-time type introspection through the `comptime` mechanism.

**Type Information Query:**
```
comptime {
    type_info: TypeInfo<T>
}
```

**Normative Type Information:**
- `size_of<T>(): usize` - Size in bytes of type T
- `align_of<T>(): usize` - Alignment requirement of type T
- `has_trait<T, Trait>(): bool` - Whether T implements Trait
- `field_count<T>(): usize` - Number of fields in record type T
- `variant_count<T>(): usize` - Number of variants in enum type T

**Type rule:**
```
[T-Comptime-TypeInfo]
Γ ⊢ T : Type
────────────────────────────────
Γ ⊢ comptime { size_of<T>() } : usize
```

### 34.6 Comptime Code Generation

**Requirement:** Implementations MUST support compile-time code generation through comptime functions.

**Comptime Function Definition:**
```cantrip
comptime function generate_impl<T>(comptime info: TypeInfo<T>): type {
    // Compile-time computation
}
```

**Normative Requirements:**
1. Comptime functions MUST be evaluated during compilation
2. Comptime function parameters marked `comptime` MUST be compile-time constants
3. Comptime functions MUST NOT perform effects
4. Comptime function results MUST be deterministic

**Type rule:**
```
[T-Comptime-Function]
Γ ⊢ f : comptime (T) -> U
Γ ⊢ e : T    e is compile-time constant
────────────────────────────────────────
Γ ⊢ f(e) : U    evaluated at compile time
```

### 34.7 Comptime Validation

**Requirement:** Implementations MUST support compile-time assertions.

**Comptime Assertion:**
```cantrip
comptime {
    assert(condition, "message");
}
```

**Normative Requirements:**
1. Comptime assertions MUST be evaluated during compilation
2. Failed comptime assertions MUST produce compilation errors
3. Comptime assertions MUST NOT affect runtime behavior

**Validation rule:**
```
[T-Comptime-Assert]
Γ ⊢ e : bool    e evaluates to true at compile time
────────────────────────────────────────────────────
Γ ⊢ comptime { assert(e, msg) } : ()
```

---

## 35. Concurrency

### 35.5 Structured Concurrency

**API (stdlib):**
```cantrip
module std.concurrent.scope

public function scope<R, F>(f: F): R
    where F: FnOnce(Scope) -> R
    needs thread.spawn, thread.join;

public record Scope { /* opaque token */ }

impl Scope {
    public function spawn<T, F>(self: Scope, f: F): JoinHandle<T>
        where F: FnOnce() -> T + Send
        needs thread.spawn;
}
```

**Semantics:**
1. Every `JoinHandle` created via `scope.spawn` MUST be joined before `scope` returns.
2. On scope exit, if any handle is not joined, the runtime performs a **forced join** and emits `W7800`.
3. If a spawned task panics, `scope` re-raises on join with context.

**Diagnostics:**
- `E7801` — attempt to detach/escape a `JoinHandle` created by `scope.spawn`.
- `W7800` — implicit join at scope end due to missing explicit `join()`.

```cantrip
import std.concurrent.scope;

function parallel_sum(input: [i32]): i64
    needs thread.spawn, thread.join;
{
    scope(|s| {
        let left  = s.spawn(|| sum(&input[0..input.length()/2]));
        let right = s.spawn(|| sum(&input[input.length()/2..]));
        (left.join() + right.join()) as i64
    })
}
### 35.1 Thread Spawning

**Create new threads:**

```cantrip
import std.thread;

function example()
    needs thread.spawn, alloc.heap;
{
    let handle = thread.spawn(|| {
            compute_something();
        });

        handle.join();
}
```

**Type rule:**
```
[T-Thread-Spawn]
Γ ⊢ f : () -> T    T : Send
────────────────────────────────────
Γ ⊢ thread.spawn(f) : JoinHandle<T>
```

### 35.2 Message Passing

**Channel-based communication:**

```cantrip
import std.sync.channel;

function example()
    needs thread.spawn, alloc.heap;
{
    let (sender, receiver) = channel.new<Message>();

        thread.spawn(move || {
            sender.send(Message.new());
        });

        let msg = receiver.receive();
}
```

### 35.3 Send and Sync Traits

**Marker traits for thread safety:**

```cantrip
trait Send {
    // Type can be transferred between threads
}

trait Sync {
    // Type can be shared between threads (immutably)
}
```

**Auto-implementation:**
- Primitives: `Send + Sync`
- `own T`: `Send` if `T: Send`
- Default ref: `Sync` if `T: Sync`
- `mut T`: `Send + Sync` if `T: Send + Sync`
- `iso T`: Always `Send`

### 35.4 Atomic Operations

**Lock-free operations:**

```cantrip
import std.sync.atomic.AtomicI32;

record Counter {
    value: AtomicI32,
}

impl Counter {
    function increment(self: Counter): i32
    needs thread.atomic;
{
    self.value.fetch_add(1)
    }
}
```

---

## 36. Actor Pattern (Standard Library)

**Definition 36.1 (Actor Pattern):** The actor pattern is a concurrency design pattern provided by the standard library that composes Cantrip's existing features—modals, channels, effects, and the permission system—to achieve isolated, message-driven concurrent computation.

**Key insight:** Actors are not a primitive language feature but emerge naturally from Cantrip's composable type system.

### 36.1 Actor Pattern Overview

**Core composition:**
1. **Modal types** (§10) - State machine semantics for actor state
2. **Channels** (§35.2) - Message passing primitives
3. **Permission system** (§26) - `own` ensures isolation
4. **Effect system** (§21) - Tracks all side effects
5. **Async/await** (§37) - Asynchronous message processing

**Guarantees achieved:**
- **Isolation**: Enforced by `own` permission on actor state
- **Sequential processing**: Single-threaded message loop
- **Type safety**: Messages constrained by enum types
- **Effect tracking**: All I/O and mutations explicitly declared

### 36.2 Basic Actor Pattern

**Message type (user-defined enum):**
```cantrip
enum CounterMessage {
    Increment,
    Decrement,
    Get(reply: Sender<usize>),
    Shutdown,
}
```

**Actor state (using modals for state machines):**
```cantrip
modal Counter {
    state Running {
        count: usize;
    }

    state Shutdown;

    // State transitions via procedures
    procedure increment@Running(self: mut Counter)
        ensures self.count == @old(self.count) + 1;
    {
        self.count += 1;
    }

    procedure decrement@Running(self: mut Counter)
        requires self.count > 0;
        ensures self.count == @old(self.count) - 1;
    {
        self.count -= 1;
    }

    procedure get@Running(self: Counter): usize {
        self.count
    }

    procedure shutdown@Running(self: mut Counter) -> @Shutdown;
}
```

**Actor wrapper (user-defined record):**
```cantrip
record CounterActor {
    state: own Counter@Running;
    mailbox: Receiver<CounterMessage>;
}

impl CounterActor {
    public function new(): (CounterActor, Sender<CounterMessage>)
        needs alloc.heap;
    {
        let (tx, rx) = channel();
        let actor = CounterActor {
            state: own Counter@Running { count: 0 },
            mailbox: rx,
        };
        (actor, tx)
    }

    // Message processing loop
    public async function run(self: mut CounterActor)
        needs alloc.heap;
    {
        loop {
            match self.mailbox.receive().await {
                CounterMessage.Increment => {
                    self.state.increment();
                },
                CounterMessage.Decrement => {
                    self.state.decrement();
                },
                CounterMessage.Get(reply) => {
                    let count = self.state.get();
                    reply.send(count).await;
                },
                CounterMessage.Shutdown => {
                    self.state.shutdown();
                    break;
                },
            }
        }
    }
}
```

**Usage:**
```cantrip
function example()
    needs alloc.heap, thread.spawn;
{
    let (actor, handle) = CounterActor.new();

    // Spawn actor on separate thread
    thread.spawn(move || {
        actor.run().await;
    });

    // Send messages
    handle.send(CounterMessage.Increment).await;
    handle.send(CounterMessage.Increment).await;

    // Request-reply pattern
    let (reply_tx, reply_rx) = channel();
    handle.send(CounterMessage.Get(reply_tx)).await;
    let count = reply_rx.receive().await;

    assert_eq(count, 2);
}
```

### 36.3 Standard Library Support

The standard library provides `std.actor` with generic helpers to reduce boilerplate:

```cantrip
module std.actor

// Generic actor wrapper
public record Actor<S, M> {
    state: own S;
    mailbox: Receiver<M>;
}

impl<S, M> Actor<S, M>
    where M: Send
{
    public function spawn<F>(initial_state: own S, handler: F): (Actor<S, M>, Sender<M>)
        where F: FnMut(mut S, M) -> S
        needs alloc.heap;
    {
        let (tx, rx) = channel();
        (Actor { state: initial_state, mailbox: rx }, tx)
    }

    public async function run<F>(self: mut Actor<S, M>, mut handler: F)
        where F: FnMut(mut S, M) -> S
        needs alloc.heap;
    {
        loop {
            match self.mailbox.receive().await {
                Some(msg) => {
                    self.state = handler(self.state, msg);
                },
                None => break,  // Channel closed
            }
        }
    }
}
```

**Simplified usage with std.actor:**
```cantrip
import std.actor.Actor;

function simplified_example()
    needs alloc.heap, thread.spawn;
{
    let initial = Counter@Running { count: 0 };

    let (mut actor, handle) = Actor.spawn(initial, |mut state, msg| {
        match msg {
            CounterMessage.Increment => {
                state.increment();
                state
            },
            CounterMessage.Decrement => {
                state.decrement();
                state
            },
            CounterMessage.Get(reply) => {
                reply.send(state.get());
                state
            },
            CounterMessage.Shutdown => {
                state.shutdown()
            },
        }
    });

    thread.spawn(move || actor.run().await);

    // Use handle...
}
```

### 36.4 Actor Pattern Guarantees

**Isolation (via permission system):**
```
Invariant 36.1 (State Isolation):
Actor state is owned (`own S`), preventing:
  - Shared mutable access from other threads
  - Aliasing of mutable state
  - Data races on actor state
```

**Sequential processing (via single receiver):**
```
Axiom 36.1 (FIFO Ordering):
Receiver<M> ensures messages are processed in order:
  send(m₁) happens-before send(m₂) ⟹
  receive() delivers m₁ before m₂
```

**Type safety (via modal constraints):**
```
Theorem 36.1 (State Machine Safety):
If actor state is modal S@State₁:
  - Only procedures valid in State₁ can be called
  - State transitions are verified at compile time
  - Invalid state access is a compile error
```

### 36.5 Actors with Complex State Machines

Modals enable sophisticated actor state machines:

```cantrip
modal FileActor {
    state Closed { path: String; }
    state Opening;
    state Open { fd: FileDescriptor; }
    state Error { err: Error; }

    procedure open@Closed(self: mut FileActor) -> @Opening
        needs fs.read;

    procedure complete_open@Opening(self: mut FileActor, fd: FileDescriptor) -> @Open;

    procedure fail_open@Opening(self: mut FileActor, err: Error) -> @Error;

    procedure read@Open(self: FileActor, buf: mut [u8]): usize
        needs fs.read;

    procedure close@Open(self: mut FileActor) -> @Closed
        needs fs.write;
}

enum FileMessage {
    Open(reply: Sender<Result<(), Error>>),
    Read(buf: Vec<u8>, reply: Sender<Result<Vec<u8>, Error>>),
    Close,
}

// Actor wrapper follows same pattern as Counter
```

**Key advantage:** The modal type system ensures the file actor cannot:
- Read from a closed file (compile error)
- Close an already-closed file (compile error)
- Violate the state machine invariants (compile error)

### 36.6 Why Pattern, Not Primitive?

**Design rationale:**

1. **Composability**: Actors emerge from combining existing features, demonstrating language expressiveness

2. **Flexibility**: Users can customize the pattern (e.g., prioritized queues, selective receive)

3. **Transparency**: No "magic"—users see exactly how actors work

4. **LLM-friendly**: Explicit patterns are more predictable than language-specific syntax

5. **Simplicity**: No new keywords, grammar rules, or type system complexity

6. **Teachability**: Understanding actors teaches modals, channels, effects, and permissions simultaneously

**Trade-off accepted:** More verbose than built-in syntax, but gains transparency and composability

---

## 37. Async/Await

### 37.4 Async Iteration

**Trait:** Asynchronous iteration yields items over time via an `async next` method.

```cantrip
public trait AsyncIterator {
    type Item;

    async function next(self: mut Self): Option<Self.Item>
        needs effects(Self);
}
```

**Typing rule:**
```
[T-Async-Iterator]
trait AsyncIterator { type Item; async fn next(Self) -> Option<Item>; }
Γ ⊢ T : AsyncIterator
──────────────────────────────────────────────
Γ ⊢ T.next() : Future<Option<T.Item>>
```

**Consumption pattern:**
```cantrip
async function consume<I>(iter: I)
    where I: AsyncIterator
    needs effects(I);
{
    while let Some(item) = await iter.next() {
        process(item);
    }
}
```

**Syntactic sugar (optional):** Implementations MAY provide `for await item in iter { ... }` desugaring to the
`while`/`await next()` form above. If provided, `for await` requires the enclosing function to be `async` and
imports the iterator’s effects into the enclosing effect mask (see §22.7).

### 37.1 Async Functions

**Define asynchronous functions:**

```cantrip
async function fetch_data(url: str): Result<Data, Error>
    needs net.read(outbound), alloc.heap;
    requires !url.is_empty();
{
    let response = await http.get(url)?;
    let data = await response.body()?;
    Ok(data)
}
```

**Type rule:**
```
[T-Async]
Γ ⊢ e : T ! ε
────────────────────────────────
Γ ⊢ async function f() : Future<T> ! ε
```

### 37.2 Await Expressions

**Wait for async operations:**

```cantrip
function example(): Result<(), Error>
    needs net.read(outbound), alloc.heap;
{
    let data = await fetch_data("https://api.example.com")?;
    process(data);
    Ok(())
}
```

### 37.3 Select Expression

**Wait on multiple futures:**

```cantrip
function fetch_with_timeout(url: str): Result<Data, Error>
    needs net.read(outbound), time.read, alloc.heap;
    requires !url.is_empty();
{
    select {
        case data = await http.get(url): Ok(data),
        case _ = await timer(5.seconds): Err(Error.Timeout),
    }
}
```

**Semantics:** First future to complete wins.

---

## 55. Machine‑Readable Output

Compilers and tools MUST provide a machine‑readable diagnostics stream to support IDEs and LLMs.

**Format:** JSON Lines (one JSON object per diagnostic).

**Schema (stable fields):**
```json
{
  "version": "0.7.1",
  "severity": "error|warning|note",
  "code": "E9001",
  "message": "Missing effect declaration: alloc.heap",
  "spans": [
    {"file":"src/main.ctr","from": {"line":5,"col":9},"to":{"line":5,"col":16}}
  ],
  "fixIts": [
    {"title":"Add 'needs alloc.heap;'", "edits":[{"file":"src/main.ctr","insertAfterLine":2,"text":"    needs alloc.heap;\n"}]}
  ],
  "context": {
    "effectMask": ["net.read(outbound)"],  // from §22.7 async mask
    "typedHole": {"name":"n","type":"String"}, // see §14.10
    "aliases": ["to_string","stringify"]  // from #[alias], §2.6
  }
}
```

**Stream guarantees:**
- Stable `code` identifiers (Appendix C).
- All primary spans MUST include file/line/column.
- Fix‑its MUST be localized textual edits with no hidden side effects.

**Compiler switches:**
```
--json                         emit JSONL diagnostics to stdout
--json-file=PATH               write JSONL to file
--json-emit-aliases            include alias metadata (see §2.6)
--json-emit-effect-mask        include async effect mask (see §22.7)
```

# Part XIV: Foreign Function Interface (FFI)

## 56. Foreign Function Interface Overview

The FFI enables calls between Cantrip and external languages (notably C). It defines symbol linkage,
type layouts, calling conventions, and ownership boundaries. FFI is **unsafe by nature**; effect `ffi.call`
and, for raw pointers, `unsafe.ptr` MUST be declared (§21.10).

## 57. Declarations and Linkage

**Extern blocks:**
```cantrip
extern "C" {
    function memcpy(dest: *u8, src: *u8, n: usize): *u8
        needs ffi.call, unsafe.ptr;
}
```

**Linkage names:** Use `extern "C"` for C ABI. Mangling is suppressed for `extern "C"` items.
Library linkage is configured in the build manifest.

## 58. Type Mappings

C ↔ Cantrip canonical mappings (subset):
- `int8_t` ↔ `i8`, `uint8_t` ↔ `u8`, …
- `float` ↔ `f32`, `double` ↔ `f64`
- `char*` ↔ `*u8` (pointer to bytes), length‑managed by caller/callee convention
- `struct` ↔ `#[repr(C)] record` (§6.5)
- Enums with explicit `#[repr(IntType)]` when interoperating with C integer enums

Pointers are raw; dereferencing requires `unsafe` section and `unsafe.ptr` effect.

## 59. Ownership and Allocation Across FFI

**Principle:** The side that allocates owns the memory and must free it using the same allocator family.
Crossing the boundary requires explicit transfer semantics or copy.

**Patterns:**
- **Borrow:** Pass `*const T`/`*mut T` with lifetime limited to the call.
- **Take & return:** `own T` transferred as an opaque pointer handle; provide destroy callbacks.
- **Copy:** Convert to POD or heap‑owned data on one side, copy back results.

## 60. Callbacks from C into Cantrip

C may call back into Cantrip via function pointers. Such callbacks are declared `extern "C"`
and must specify effects explicitly. Re‑entrancy MUST be documented by the callee.

## 61. Errors and Panics

FFI functions MUST NOT unwind across the boundary. Cantrip panics are trapped and converted to error
codes or nulls as specified by the binding. Conversely, C long‑jumps must not cross into Cantrip frames.

## 62. Inline Assembly (Reserved)

Inline assembly is reserved in 1.0. Implementations MAY provide experimental features behind flags,
but no normative syntax is defined.

# Part X: Operational Semantics

## 38. Small-Step Semantics

### 38.1 Small-Step Reduction

**Definition 38.1 (Small-Step Semantics):** Single-step reduction relation ⟨e, σ, ε⟩ → ⟨e', σ', ε'⟩.

**Core rules:**

**[E-App-β]**
```
─────────────────────────────────────────────
⟨(λx:T. e) v, σ, ε⟩ → ⟨e[x ↦ v], σ, ε⟩
```

**[E-Let-Val]**
```
────────────────────────────────────────
⟨let x = v in e, σ, ε⟩ → ⟨e[x ↦ v], σ, ε⟩
```

**[E-If-True]**
```
────────────────────────────────────────
⟨if true then e₁ else e₂, σ, ε⟩ → ⟨e₁, σ, ε⟩
```

**[E-If-False]**
```
────────────────────────────────────────
⟨if false then e₁ else e₂, σ, ε⟩ → ⟨e₂, σ, ε⟩
```

### 38.2 Evaluation Contexts

**Definition 38.2 (Evaluation Context):** Contexts defining evaluation order.

```
E ::= []                    (hole)
    | E e                   (left of application)
    | v E                   (right of application)
    | let x = E in e        (let binding)
    | E op e₂               (left of operation)
    | v₁ op E               (right of operation)
    | if E then e₁ else e₂  (condition)
```

**Context rule:**
```
[E-Context]
⟨e, σ, ε⟩ → ⟨e', σ', ε'⟩
─────────────────────────────
⟨E[e], σ, ε⟩ → ⟨E[e'], σ', ε'⟩
```

---

## 39. Big-Step Semantics

### 39.1 Big-Step Evaluation

**Definition 39.1 (Big-Step Semantics):** Multi-step evaluation relation ⟨e, σ⟩ ⇓ ⟨v, σ'⟩.

**[Eval-Val]**
```
─────────────────
⟨v, σ⟩ ⇓ ⟨v, σ⟩
```

**[Eval-Let]**
```
⟨e₁, σ⟩ ⇓ ⟨v₁, σ₁⟩    ⟨e₂[x ↦ v₁], σ₁⟩ ⇓ ⟨v₂, σ₂⟩
──────────────────────────────────────────────────
⟨let x = e₁ in e₂, σ⟩ ⇓ ⟨v₂, σ₂⟩
```

**[Eval-App]**
```
⟨e₁, σ⟩ ⇓ ⟨λx:T. e, σ₁⟩    ⟨e₂, σ₁⟩ ⇓ ⟨v₂, σ₂⟩    ⟨e[x ↦ v₂], σ₂⟩ ⇓ ⟨v, σ₃⟩
────────────────────────────────────────────────────────────────────────────────
⟨e₁ e₂, σ⟩ ⇓ ⟨v, σ₃⟩
```

---

## 40. Memory Model

### 40.1 Memory State

**Definition 40.1 (Memory State):**

```
σ = (Heap, {r₁, r₂, ..., rₙ}, Stack)

where:
  Heap = {(addr, value) | addr ∈ HeapAddrs}
  rᵢ = (id, allocations, parent)
  Stack = {(addr, value, scope) | addr ∈ StackAddrs}
```

### 40.2 Happens-Before

**Definition 40.2 (Happens-Before):**

```
→ₕᵦ = (<ₚ ∪ ≺ₛ)⁺  (transitive closure)

where:
  eâ‚ <ₚ e₂  iff  e₁ before e₂ in program order
  e₁ ≺ₛ e₂  iff  e₁ synchronizes-with e₂
```

---

## 41. Evaluation Order

### 41.1 Left-to-Right Evaluation

**Axiom 41.1:** Arguments evaluate left-to-right.

```
f(e₁, e₂, e₃)  evaluates:
  1. e₁
  2. e₂
  3. e₃
  4. f(v₁, v₂, v₃)
```

### 41.2 Short-Circuit Evaluation

**Logical operators:**
```
false && e  →  false  (e not evaluated)
true || e   →  true   (e not evaluated)
```

---

# Part XI: Soundness and Properties

## 42. Type Soundness

**Theorem 42.1 (Progress):**

If Γ ⊢ e : T and e closed, then either:
1. e is a value, or
2. ∃e', σ, σ'. ⟨e, σ⟩ → ⟨e', σ'⟩

**Theorem 42.2 (Preservation):**

If Γ ⊢ e : T and ⟨e, σ⟩ → ⟨e', σ'⟩, then Γ ⊢ e' : T.

---

## 43. Effect Soundness

**Theorem 43.1 (Effect Approximation):**

If Γ ⊢ e : T ! ε and ⟨e, σ, ∅⟩ →* ⟨v, σ', ε'⟩, then ε' ⊆ ε.

---

## 44. Memory Safety

**Theorem 44.1 (No Use-After-Free):**

If ⟨e, σ⟩ →* ⟨e', σ'⟩ and e' accesses ℓ, then ℓ ∈ dom(σ').

---

## 45. Modal Safety

**Theorem 45.1 (State Invariant Preservation):**

If Γ, Σ ⊢ e : P@S and σ ⊨ I(@S) and ⟨e, σ⟩ →* ⟨v, σ'⟩, then σ' ⊨ I(@S).

---

# Part XII: Standard Library

## 46. Core Types and Operations

### 46.1 Core Type System

**Standard library core types (`std`):**

```cantrip
// Fundamental types
std.Option<T>           // Optional values
std.Result<T, E>        // Error handling
std.String              // UTF-8 string
std.Vec<T>              // Dynamic array
std.Box<T>              // Heap-allocated value
```

### 46.2 Option Type

**Definition 46.1 (Option):**

```cantrip
// Module: std
public enum Option<T> {
    Some(T),
    None,
}

impl<T> Option<T> {
    public function is_some(self: Option<T>): bool {
    match self {
                Option.Some(_) -> true,
                Option.None -> false,
            }
    }

    public function is_none(self: Option<T>): bool {
        match self {
            Option.Some(_) -> false,
            Option.None -> true,
        }
    }

    public function unwrap(self: Option<T>): T
    needs panic;
{
    match self {
                Option.Some(value) -> value,
                Option.None -> panic("unwrap on None"),
            }
    }

    public function unwrap_or(self: Option<T>, default: T): T {
        match self {
            Option.Some(value) -> value,
            Option.None -> default,
        }
    }

    public function map<U>(self: Option<T>, f: T -> U): Option<U>
        needs effects(f);
    {
        match self {
            Option.Some(value) -> Option.Some(f(value)),
            Option.None -> Option.None,
        }
    }
}
```

### 46.3 Result Type

**Definition 46.2 (Result):**

```cantrip
// Module: std
public enum Result<T, E> {
    Ok(T),
    Err(E),
}

impl<T, E> Result<T, E> {
    public function is_ok(self: Result<T, E>): bool {
    match self {
                Result.Ok(_) -> true,
                Result.Err(_) -> false,
            }
    }

    public function is_err(self: Result<T, E>): bool {
        match self {
            Result.Ok(_) -> false,
            Result.Err(_) -> true,
        }
    }

    public function unwrap(self: Result<T, E>): T
    needs panic;
{
    match self {
                Result.Ok(value) -> value,
                Result.Err(err) -> panic("unwrap on Err: {}", err),
            }
    }

    public function unwrap_or(self: Result<T, E>, default: T): T {
        match self {
            Result.Ok(value) -> value,
            Result.Err(_) -> default,
        }
    }

    public function map<U>(self: Result<T, E>, f: T -> U): Result<U, E>
        needs effects(f);
    {
        match self {
            Result.Ok(value) -> Result.Ok(f(value)),
            Result.Err(err) -> Result.Err(err),
        }
    }

    public function map_err<F>(self: Result<T, E>, f: E -> F): Result<T, F>
        needs effects(f);
    {
        match self {
            Result.Ok(value) -> Result.Ok(value),
            Result.Err(err) -> Result.Err(f(err)),
        }
    }
}
```

### 46.4 String Type

**Definition 46.3 (String):**

```cantrip
// Module: std
public record String {
    // Internal representation
    data: own Vec<u8>;

    public function new(): own String
    needs alloc.heap;
{
    own String { data: Vec.new() }
    }

    public function from(slice: str): own String
        needs alloc.heap;
        requires !slice.is_empty();
    {
        own String { data: slice.as_bytes().to_vec() }
    }

    public function length(self: String): usize {
    self.data.length()
    }

    public function is_empty(self: String): bool {
        self.data.is_empty()
    }

    public function push_str(self: mut String, s: str)
    needs alloc.heap;
{
    self.data.extend_from_slice(s.as_bytes());
    }

    public function split(self: String, delimiter: str): own Vec<str>
        needs alloc.heap;
        requires !delimiter.is_empty();
    {
        // Implementation
    }
}
```

---

## 47. Collections

### 47.1 Vec<T>

**Definition 47.1 (Vec):** Dynamic array with amortized O(1) push.

**Module:** `std.collections`

```cantrip
public record Vec<T> {
    // Internal fields
    data: *mut T;
    length: usize;
    capacity: usize;

    public function new(): own Vec<T>
        needs alloc.heap;
    {
        own Vec {
            data: null,
                length: 0,
                capacity: 0,
            }
    }

    public function new_in<'r>(): own Vec<T>
        needs alloc.region;
    {
        // Region-allocated vector
    }

    public function with_capacity(capacity: usize): own Vec<T>
        needs alloc.heap;
        requires capacity > 0;
    {
        // Pre-allocate space
    }

    public function push(self: mut Vec<T>, item: T)
    needs alloc.heap;
{
    if self.length == self.capacity {
                self.grow();
            }
            self.data[self.length] = item;
            self.length += 1;
    }

    public function pop(self: mut Vec<T>): Option<T> {
        if self.length == 0 {
            None
        } else {
            self.length -= 1;
            Some(self.data[self.length])
        }
    }

    public function length(self: Vec<T>): usize {
    self.length
    }

    public function capacity(self: Vec<T>): usize {
        self.capacity
    }

    public function is_empty(self: Vec<T>): bool {
        self.length == 0
    }

    public function get(self: Vec<T>, index: usize): Option<T> {
        if index < self.length {
            Some(self.data[index])
        } else {
            None
        }
    }

    public function contains(self: Vec<T>, item: T): bool
        where T: PartialEq;
    {
        for i in 0..self.length {
            if self.data[i] == item {
                return true;
            }
        }
        false
    }

    public function clear(self: mut Vec<T>) {
        self.length = 0;
    }
}
```

**Complexity guarantees:**
- `push`: O(1) amortized
- `pop`: O(1)
- `get`: O(1)
- `contains`: O(n)

### 47.2 HashMap<K, V>

**Definition 47.2 (HashMap):** Hash table with O(1) average access.

**Module:** `std.collections`

```cantrip
public record HashMap<K, V> where K: Hash + Eq {
    // Internal structure
    buckets: Vec<Bucket<K, V>>;
    size: usize;

    public function new(): own HashMap<K, V>
        needs alloc.heap;
    {
        own HashMap {
            buckets: Vec.with_capacity(16),
            size: 0,
        }
    }

    public function insert(self: mut HashMap<K, V>, key: K, value: V): Option<V>
        needs alloc.heap;
    {
        let hash = key.hash();
        let index = hash % self.buckets.length();

        // Check if key exists
        if let Some(old_value) = self.buckets[index].find(key) {
            self.buckets[index].replace(key, value);
            Some(old_value)
        } else {
            self.buckets[index].insert(key, value);
            self.size += 1;
            None
        }
    }

    public function get(self: HashMap<K, V>, key: K): Option<V> {
        let hash = key.hash();
        let index = hash % self.buckets.length();
        self.buckets[index].find(key)
    }

    public function remove(self: mut HashMap<K, V>, key: K): Option<V> {
        let hash = key.hash();
        let index = hash % self.buckets.length();

        if let Some(value) = self.buckets[index].remove(key) {
            self.size -= 1;
            Some(value)
        } else {
            None
        }
    }

    public function contains_key(self: HashMap<K, V>, key: K): bool {
    self.get(key).is_some()
    }

    public function length(self: HashMap<K, V>): usize {
        self.size
    }
}
```

### 47.3 HashSet<T>

**Definition 47.3 (HashSet):** Set implemented as HashMap<T, ()>.

```cantrip
public record HashSet<T> where T: Hash + Eq {
    map: HashMap<T, ()>,

    public function new(): own HashSet<T>
        needs alloc.heap;
    {
        own HashSet { map: HashMap.new() }
    }

    public function insert(self: mut HashSet<T>, value: T): bool
    needs alloc.heap;
{
    self.map.insert(value, ()).is_none()
    }

    public function contains(self: HashSet<T>, value: T): bool {
    self.map.contains_key(value)
    }

    public function remove(self: mut HashSet<T>, value: T): bool {
        self.map.remove(value).is_some()
    }
}
```

---

## 48. I/O and File System

### 48.1 File I/O

**Module:** `std.io`

```cantrip
public record File {
    handle: FileHandle;
    path: String;

    public function open(path: str): Result<own File, Error>
        needs fs.read, alloc.heap;
        requires !path.is_empty();
    {
        match FileSystem.open(path) {
            Ok(handle) -> Ok(own File {
                handle,
                path: String.from(path),
            }),
            Err(err) -> Err(err),
        }
    }

    public function create(path: str): Result<own File, Error>
        needs fs.write, alloc.heap;
        requires !path.is_empty();
    {
        match FileSystem.create(path) {
            Ok(handle) -> Ok(own File {
                handle,
                path: String.from(path),
            }),
            Err(err) -> Err(err),
        }
    }

    public function read(self: mut File, buffer: mut [u8]): Result<usize, Error>
        needs fs.read;
        requires buffer.length() > 0;
    {
        self.handle.read(buffer)
    }

    public function read_all(self: mut File): Result<Vec<u8>, Error>
        needs fs.read, alloc.heap;
    {
        let mut data = Vec.new();
        let mut buffer = [0u8; 4096];

        loop {
            match self.read(mut buffer) {
                Ok(0) -> break,
                Ok(n) -> data.extend_from_slice(buffer[0..n]),
                Err(err) -> return Err(err),
            }
        }

        Ok(data)
    }

    public function write(self: mut File, data: [u8]): Result<usize, Error>
        needs fs.write;
        requires data.length() > 0;
    {
        self.handle.write(data)
    }

    public function close(self: own File) {
        self.handle.close();
    }
}
```

### 48.2 Standard Streams

```cantrip
// Module: std.io

public function print(msg: str)
    needs io.write;
{
    stdout().write(msg.as_bytes());
}

public function println(msg: str)
    needs io.write;
{
    stdout().write(msg.as_bytes());
        stdout().write("\n".as_bytes());
}

public function eprint(msg: str)
    needs io.write;
{
    stderr().write(msg.as_bytes());
}

public function eprintln(msg: str)
    needs io.write;
{
    stderr().write(msg.as_bytes());
        stderr().write("\n".as_bytes());
}
```

### 48.3 File System Operations

**Module:** `std.fs`

```cantrip
public function read_to_string(path: str): Result<String, Error>
    needs fs.read, alloc.heap;
    requires !path.is_empty();
{
    let mut file = File.open(path)?;
    let data = file.read_all()?;
    String.from_utf8(data)
}

public function write(path: str, contents: str): Result<(), Error>
    needs fs.write, alloc.heap;
    requires !path.is_empty();
{
    let mut file = File.create(path)?;
    file.write(contents.as_bytes())?;
    Ok(())
}

public function exists(path: str): bool
    needs fs.metadata;
    requires !path.is_empty();
{
    FileSystem.exists(path)
}

public function remove(path: str): Result<(), Error>
    needs fs.delete;
    requires !path.is_empty();
{
    FileSystem.remove(path)
}
```

---

## 49. Networking

### 49.1 HTTP Client

**Module:** `std.net.http`

```cantrip
public record Client {
    timeout: Duration;

    public function new(): own Client
    needs alloc.heap;
{
    own Client {
                timeout: Duration.from_secs(30),
            }
    }

    public function get(self: Client, url: str): Result<Response, Error>
        needs net.read(outbound), alloc.heap;
        requires !url.is_empty();
    {
        let request = Request.builder()
            .procedure(Procedure.GET)
            .url(url)
            .build()?;

        self.execute(request)
    }

    public function post(self: Client, url: str, body: [u8]): Result<Response, Error>
        needs net.read(outbound), net.write, alloc.heap;
        requires !url.is_empty();
    {
        let request = Request.builder()
            .procedure(Procedure.POST)
            .url(url)
            .body(body)
            .build()?;

            self.execute(request)
    }
}

public record Response {
    status: u16;
    headers: HashMap<String, String>;
    body: Vec<u8>;

    public function status(self: Response): u16 {
    self.status
    }

    public function body(self: Response): Vec<u8> {
        self.body
    }

    public function text(self: Response): Result<String, Error>
        needs alloc.heap;
    {
        String.from_utf8(self.body)
    }
}
```

### 49.2 TCP Sockets

**Module:** `std.net.tcp`

```cantrip
public record TcpListener {
    addr: SocketAddr;

    public function bind(addr: str): Result<own TcpListener, Error>
        needs net.read(inbound), alloc.heap;
    {
        // Implementation
    }

    public function accept(self: TcpListener): Result<TcpStream, Error>
        needs net.read(inbound), alloc.heap;
    {
        // Implementation
    }
}

public record TcpStream {
    socket: Socket;

    public function connect(addr: str): Result<own TcpStream, Error>
        needs net.read(outbound), alloc.heap;
    {
        // Implementation
    }

    public function read(self: mut TcpStream, buffer: mut [u8]): Result<usize, Error>
        needs net.read(outbound);
    {
        self.socket.read(buffer)
    }

    public function write(self: mut TcpStream, data: [u8]): Result<usize, Error>
        needs net.write;
    {
        self.socket.write(data)
    }
}
```

---

## 50. Concurrency Primitives

#### 50.1 Structured Concurrency Helpers

The `std.concurrent.scope` module is part of the standard library. See §35.5 for semantics.
Using `scope.spawn` eliminates leaked threads by construction and integrates with the effect
system (`thread.spawn`, `thread.join`).

### 50.1 Mutex

**Module:** `std.sync`

```cantrip
public record Mutex<T> {
    data: T;
    lock: AtomicBool;

    public function new(value: T): own Mutex<T>
        needs alloc.heap;
    {
        own Mutex {
            data: value,
            lock: AtomicBool.new(false),
        }
    }

    public function lock(self: Mutex<T>): MutexGuard<T> {
        while !self.lock.compare_exchange(false, true) {
            // Spin
        }
        MutexGuard { mutex: self }
    }
}

public record MutexGuard<T> {
    mutex: Mutex<T>;

    // Automatic unlock on drop
}
```

### 50.2 Channels

**Module:** `std.sync.channel`

```cantrip
public function new<T>(): (Sender<T>, Receiver<T>)
    needs alloc.heap;
{
    let channel = Channel.new();
    (Sender { channel }, Receiver { channel })
}

public record Sender<T> {
    channel: Channel<T>;

    public function send(self: Sender<T>, value: T): Result<(), Error> {
        self.channel.push(value)
    }
}

public record Receiver<T> {
    channel: Channel<T>;

    public function receive(self: Receiver<T>): Result<T, Error> {
        self.channel.pop()
    }

    public function try_receive(self: Receiver<T>): Option<T> {
        self.channel.try_pop()
    }
}
```

### 50.3 Atomic Types

**Module:** `std.sync.atomic`

```cantrip
public record AtomicI32 {
    value: i32;

    public function new(initial: i32): own AtomicI32 {
    own AtomicI32 { value: initial }
    }

    public function load(self: AtomicI32): i32
    needs thread.atomic;
{
    atomic_load(&self.value)
    }

    public function store(self: mut AtomicI32, value: i32)
    needs thread.atomic;
{
    atomic_store(&self.value, value)
    }

    public function fetch_add(self: mut AtomicI32, value: i32): i32
        needs thread.atomic;
    {
        atomic_fetch_add(&self.value, value)
    }

    public function compare_exchange(
        self: mut AtomicI32,
        current: i32,
        new: i32
    ): bool
        needs thread.atomic;
    {
        atomic_compare_exchange(&self.value, current, new)
    }
}
```

---

# Part XIII: Tooling and Implementation

## 51. Compiler Architecture

### 51.1 Compilation Pipeline

**Definition 51.1 (Compilation Pipeline):** The sequence of transformations from source to executable.

**Phases:**
```
Source Code (.cantrip)
    ↓
[1. Lexical Analysis]
    ↓
Token Stream
    ↓
[2. Parsing]
    ↓
Abstract Syntax Tree (AST)
    ↓
[3. Name Resolution]
    ↓
Resolved AST
    ↓
[4. Type Checking]
    ↓
Typed AST
    ↓
[5. Effect Checking]
    ↓
Effect-Annotated AST
    ↓
[6. Contract Verification]
    ↓
Verified AST
    ↓
[7. Modal State Analysis]
    ↓
State-Verified AST
    ↓
[8. Borrow/Region Analysis]
    ↓
Lifetime-Annotated AST
    ↓
[9. Optimization]
    ↓
Optimized IR
    ↓
[10. Code Generation]
    ↓
Native Code / Bytecode
```

### 51.2 Compiler Invocation

**Basic compilation:**
```bash
cantrip compile main.cantrip -o output
```

**Options:**
```bash
cantrip compile main.cantrip \
    --opt=2 \                    # Optimization level (0-3)
    --verify=static \            # Contract verification mode
    --target=x86_64-linux \      # Target platform
    --emit=asm \                 # Emit assembly
    --no-default-features \      # Disable default features
    --features=experimental      # Enable features
```

### 51.3 Optimization Levels

**Optimization levels:**

| Level | Description | Compile Time | Runtime Performance |
|-------|-------------|--------------|---------------------|
| `--opt=0` | No optimization | Fast | Slow |
| `--opt=1` | Basic optimization | Moderate | Good |
| `--opt=2` | Standard (default) | Moderate | Very Good |
| `--opt=3` | Aggressive | Slow | Excellent |

**Optimizations:**
- Level 0: None
- Level 1: Dead code elimination, constant folding
- Level 2: Inlining, loop optimizations, register allocation
- Level 3: Inter-procedural optimization, vectorization, LTO

### 51.4 Verification Modes

**Three verification strategies:**

```bash
# Static verification (must prove or fail compilation)
cantrip compile --verify=static main.cantrip

# Runtime verification (default - insert runtime checks)
cantrip compile --verify=runtime main.cantrip

# No verification (unsafe - trust programmer)
cantrip compile --verify=none main.cantrip
```

**Per-function override:**
```cantrip
#[verify(static)]
function critical() { ... }

#[verify(runtime)]
function normal() { ... }

#[verify(none)]
unsafe function low_level() { ... }
```

### 51.5 Incremental Compilation

**Compiler maintains dependency graph:**

```
Module Dependency Graph:
  main → http → net.tcp
  main → json
  http → json
```

**Recompilation:**
- Only recompile changed modules
- Recompile dependent modules
- Use cached artifacts for unchanged modules

**Cache structure:**
```
target/
├── cache/
│   ├── main.cantrip.o
│   ├── http.cantrip.o
│   └── json.cantrip.o
└── deps/
    └── dependency_graph.json
```

---

## 52. Error Reporting

### 52.1 Error Message Format

**Standard error format:**
```
error[E####]: Error message
  --> file:line:column
   |
## | source code line
   | ^^^^^ explanation
   |
   = note: Additional information
   = help: Suggestion for fix
```

### 52.2 Example Error Messages

**Type error:**
```bash
error[E2001]: Type mismatch
  --> src/main.cantrip:15:9
   |
15 |     let x: i32 = "string";
   |                  ^^^^^^^^ expected i32, found str
   |
   = note: Types must match exactly in Cantrip
   = help: Consider using: let x: i32 = 42;
```

**Effect error:**
```bash
error[E9001]: Missing effect declaration
  --> src/io.cantrip:8:9
   |
 8 |         Vec.new()
   |         ^^^^^^^^^ requires effect alloc.heap
   |
note: function declared as pure
  --> src/io.cantrip:5:5
   |
 5 |     requires:
   |     ^^^^^^^^^ declared with no effects
   |
help: add effect declaration
   |
 5 |     requires<alloc.heap>:
   |             ^^^^^^^^^^^^
```

**Modal state error:**
```bash
error[E3003]: Procedure not available in current state
  --> src/file.cantrip:42:10
   |
42 |     file.read(buffer);
   |          ^^^^ procedure 'read' not available in state Closed
   |
note: file has type File@Closed
  --> src/file.cantrip:40:9
   |
40 |     let file = File.new("data.txt");
   |         ^^^^ inferred type: File@Closed
   |
help: call 'open' to transition to Open state
   |
41 |     let file = file.open()?;
   |                ^^^^^^^^^^^
```

### 52.3 Machine-Readable Output

**JSON format:**
```bash
cantrip compile --output-format=json main.cantrip
```

**Output:**
```json
{
  "version": "0.4.0",
  "diagnostics": [
    {
      "id": "E2001",
      "severity": "error",
      "message": "Type mismatch",
      "location": {
        "file": "src/main.cantrip",
        "line": 15,
        "column": 9,
        "span": {
          "start": 245,
          "end": 253
        }
      },
      "fixes": [
        {
          "description": "Change to i32",
          "edits": [
            {
              "location": { "start": 245, "end": 253 },
              "replacement": "42"
            }
          ]
        }
      ]
    }
  ]
}
```

---

## 53. Package Management

### 53.1 Project Structure

**Standard project layout:**
```
myproject/
├── cantrip.toml              # Package manifest
├── source/
│   ├── main.cantrip          # Entry point
│   └── lib.cantrip           # Library root (optional)
├── tests/
│   └── integration.cantrip   # Integration tests
├── examples/
│   └── simple.cantrip        # Example programs
├── benches/
│   └── performance.cantrip   # Benchmarks
└── target/               # Build artifacts (generated)
    ├── debug/
    └── release/
```

### 53.2 Package Manifest

**`cantrip.toml` format:**
```toml
[package]
name = "myproject"
version = "1.0.0"
authors = ["Your Name <you@example.com>"]
edition = "0.4.0"
description = "A short description"
license = "MIT OR Apache-2.0"
repository = "https://github.com/user/myproject"

[dependencies]
http = "2.0"
json = { version = "1.5", features = ["pretty"] }
local_crate = { path = "../local_crate" }

[dev-dependencies]
test_utils = "0.3"

[features]
default = ["std"]
std = []
experimental = []

[profile.release]
opt-level = 3
verify = "runtime"

[profile.dev]
opt-level = 0
verify = "runtime"
```

### 53.3 Commands

**Package management commands:**

```bash
# Create new project
cantrip new myproject
cantrip new --lib mylibrary

# Build project
cantrip build
cantrip build --release

# Run project
cantrip run
cantrip run --release

# Test project
cantrip test
cantrip test --test integration

# Benchmark
cantrip bench

# Clean build artifacts
cantrip clean

# Update dependencies
cantrip update

# Publish to registry
cantrip publish
```

### 53.4 Dependency Resolution

**Version specification:**
```toml
[dependencies]
# Exact version
exact = "=1.0.0"

# Compatible versions (semver)
compatible = "1.2"         # >=1.2.0, <2.0.0
caret = "^1.2.3"           # >=1.2.3, <2.0.0
tilde = "~1.2.3"           # >=1.2.3, <1.3.0

# Version ranges
range = ">=1.2, <1.5"

# Wildcard
any = "*"
```

**Resolution algorithm:**
1. Parse dependency specifications
2. Fetch package metadata from registry
3. Build dependency graph
4. Check for conflicts
5. Select versions satisfying all constraints
6. Download and cache packages

---

## 54. Testing Framework

### 54.1 Unit Tests

**Test annotation:**
```cantrip
#[test]
function test_addition() {
    assert_eq(2 + 2, 4);
}

#[test]
function test_subtraction() {
    assert_eq(5 - 3, 2);
}
```

**Test assertions:**
```cantrip
assert(condition, "message");
assert(left == right, "values must be equal");
assert(left != right, "values must not be equal");
```

### 54.2 Integration Tests

**Integration test file (`tests/integration.cantrip`):**
```cantrip
import mylib;

#[test]
function test_end_to_end() {
    let result = mylib.process_data(input);
    assert_eq(result, expected);
}
```

### 54.3 Property-Based Testing

**Property tests:**
```cantrip
#[property_test(cases = 1000)]
function test_sort_preserves_length(arr: Vec<i32>) {
    let original_len = arr.length();
    let sorted = arr.sort();
    assert_eq(sorted.length(), original_len);
}

#[property_test(cases = 1000)]
function test_addition_commutative(a: i32, b: i32) {
    assert_eq(a + b, b + a);
}
```

### 54.4 Benchmarks

**Benchmark file (`benches/performance.cantrip`):**
```cantrip
#[bench]
function bench_vector_push(b: Bencher) {
    b.iter(|| {
        var v = Vec.new();
        for i in 0..1000 {
            v.push(i);
        }
    });
}

#[bench]
function bench_hashmap_insert(b: Bencher) {
    b.iter(|| {
        var map = HashMap.new();
        for i in 0..1000 {
            map.insert(i, i * 2);
        }
    });
}
```

---

# Appendix A: Complete EBNF Grammar

This appendix collects all grammar productions from the specification into a single normative reference.

## A.1 Lexical Grammar

### A.1.1 Comments
```ebnf
LineComment ::= "//" ~[\n\r]* [\n\r]
BlockComment ::= "/*" (~"*/" any)* "*/"
DocComment ::= "///" ~[\n\r]*
ModuleDoc ::= "//!" ~[\n\r]*
```

### A.1.2 Identifiers
```ebnf
Identifier ::= IdentStart IdentContinue*
IdentStart ::= [a-zA-Z_]
IdentContinue ::= [a-zA-Z0-9_]
```

### A.1.3 Literals
```ebnf
IntegerLiteral ::= DecimalLiteral
                 | HexLiteral
                 | BinaryLiteral
                 | OctalLiteral

DecimalLiteral ::= [0-9] [0-9_]*
HexLiteral ::= "0x" [0-9a-fA-F] [0-9a-fA-F_]*
BinaryLiteral ::= "0b" [01] [01_]*
OctalLiteral ::= "0o" [0-7] [0-7_]*

FloatLiteral ::= DecimalLiteral "." DecimalLiteral Exponent?
Exponent ::= [eE] [+-]? DecimalLiteral

BooleanLiteral ::= "true" | "false"

CharLiteral ::= "'" (EscapeSequence | ~['\\]) "'"
EscapeSequence ::= "\\" [nrt\\'"0]
                 | "\\x" HexDigit HexDigit
                 | "\\u{" HexDigit+ "}"

StringLiteral ::= '"' (EscapeSequence | ~["\\])* '"'
```

### A.1.4 Attributes
```ebnf
Attribute ::= "#[" AttributeBody "]"
AttributeBody ::= Ident ( "(" AttrArgs? ")" )?
AttrArgs ::= AttrArg ( "," AttrArg )*
AttrArg ::= Ident "=" Literal | Literal | Ident
```

## A.2 Type Grammar

```ebnf
Type ::= PrimitiveType
       | CompoundType
       | UserType
       | PermissionType
       | ModalType
       | FunctionType
       | GenericType

PrimitiveType ::= "i8" | "i16" | "i32" | "i64" | "isize"
                | "u8" | "u16" | "u32" | "u64" | "usize"
                | "f32" | "f64"
                | "bool" | "char" | "str"

CompoundType ::= ArrayType | TupleType | SliceType

ArrayType ::= "[" Type ";" IntegerLiteral "]"
SliceType ::= "[" Type "]"
TupleType ::= "(" Type ("," Type)* ")"

UserType ::= RecordType | EnumType | TraitType

PermissionType ::= "own" Type
                 | "mut" Type
                 | "iso" Type

ModalType ::= Type "@" StateIdent

FunctionType ::= "fn" "(" TypeList? ")" (":" Type)?
ProcedureType ::= "proc" "(" SelfParam "," TypeList? ")" (":" Type)?

TypeList ::= Type ("," Type)*

GenericType ::= Identifier "<" TypeArgs ">"
TypeArgs ::= Type ("," Type)*
```

## A.3 Expression Grammar

```ebnf
Expr ::= Literal
       | Identifier
       | BinaryExpr
       | UnaryExpr
       | CallExpr
       | IfExpr
       | MatchExpr
       | BlockExpr
       | LetExpr
       | PipelineExpr
       | LambdaExpr
       | RegionExpr
       | ComptimeExpr
       | TypedHoleExpr

Literal ::= IntegerLiteral
          | FloatLiteral
          | BooleanLiteral
          | CharLiteral
          | StringLiteral

BinaryExpr ::= Expr BinOp Expr
BinOp ::= "+" | "-" | "*" | "/" | "%"
        | "==" | "!=" | "<" | ">" | "<=" | ">="
        | "&&" | "||"
        | "&" | "|" | "^" | "<<" | ">>"
        | "=>" | "=" | "+=" | "-=" | "*=" | "/="

UnaryExpr ::= UnaryOp Expr
UnaryOp ::= "-" | "!" | "*" | "&"

CallExpr ::= Expr "(" ArgList? ")"
ArgList ::= Expr ("," Expr)*

IfExpr ::= "if" Expr BlockExpr ("else" (IfExpr | BlockExpr))?

MatchExpr ::= "match" Expr "{" MatchArm* "}"
MatchArm ::= Pattern ("if" Expr)? "=>" Expr ","

BlockExpr ::= "{" Statement* Expr? "}"

LetExpr ::= "let" Pattern (":" Type)? "=" Expr

PipelineExpr ::= Expr "=>" Expr

LambdaExpr ::= "|" ParamList? "|" Expr
             | "|" ParamList? "|" BlockExpr

RegionExpr ::= "region" Identifier BlockExpr

ComptimeExpr ::= "comptime" BlockExpr

TypedHoleExpr ::= "?" Identifier? (":" Type)?
                | "???"
```

## A.4 Pattern Grammar

```ebnf
Pattern ::= IdentPattern
          | WildcardPattern
          | LiteralPattern
          | TuplePattern
          | EnumPattern
          | RecordPattern

IdentPattern ::= Identifier
WildcardPattern ::= "_"
LiteralPattern ::= IntegerLiteral | BooleanLiteral | CharLiteral

TuplePattern ::= "(" Pattern ("," Pattern)* ")"

EnumPattern ::= Identifier ("::" Identifier)? ("(" Pattern ")")?

RecordPattern ::= Identifier "{" FieldPatterns? "}"
FieldPatterns ::= FieldPattern ("," FieldPattern)*
FieldPattern ::= Identifier (":" Pattern)?
```

## A.5 Statement Grammar

```ebnf
Statement ::= LetStmt
            | VarStmt
            | ExprStmt
            | AssignStmt
            | ReturnStmt
            | BreakStmt
            | ContinueStmt

LetStmt ::= "let" Pattern (":" Type)? "=" Expr ";"
VarStmt ::= "var" Pattern (":" Type)? "=" Expr ";"
ExprStmt ::= Expr ";"
AssignStmt ::= Expr "=" Expr ";"
ReturnStmt ::= "return" Expr? ";"
BreakStmt ::= "break" ";"
ContinueStmt ::= "continue" ";"
```

## A.6 Declaration Grammar

### A.6.1 Function and Procedure Declarations
```ebnf
FunctionDecl ::= Attribute* Visibility? "function" Identifier GenericParams?
                 "(" ParamList? ")" (":" Type)?
                 ContractClause*
                 FunctionBody

ProcedureDecl ::= Attribute* Visibility? "procedure" Identifier GenericParams?
                  "(" SelfParam ("," ParamList)? ")" (":" Type)?
                  StateTransition?
                  ContractClause*
                  FunctionBody

ParamList ::= Param ("," Param)*
Param ::= Identifier ":" Type

SelfParam ::= "self" (":" SelfType)?
SelfType ::= "Self" | "mut" "Self" | "own" "Self"

StateTransition ::= "@" Identifier "->" "@" Identifier

ContractClause ::= RequiresClause | EnsuresClause | NeedsClause | InvariantClause

RequiresClause ::= "requires" Assertion (";" Assertion)* ";"
EnsuresClause ::= "ensures" Assertion (";" Assertion)* ";"
NeedsClause ::= "needs" EffectList ";"
InvariantClause ::= "invariant" ":" Assertion (";" Assertion)* ";"

FunctionBody ::= BlockExpr | ";"
```

### A.6.2 Type Declarations
```ebnf
RecordDecl ::= Attribute* Visibility? "record" Identifier GenericParams?
               "{" RecordField* "}"

RecordField ::= Visibility? Identifier ":" Type ";"

EnumDecl ::= Attribute* Visibility? "enum" Identifier GenericParams?
             "{" EnumVariant ("," EnumVariant)* ","? "}"

EnumVariant ::= Identifier ("(" Type ")")? ("=" IntegerLiteral)?

ModalDecl ::= Attribute* Visibility? "modal" Identifier GenericParams?
              "{" ModalState+ "}"

ModalState ::= "state" Identifier "{" RecordField* "}"

TraitDecl ::= Attribute* Visibility? "trait" Identifier GenericParams? (":" TraitBounds)?
              "{" TraitItem* "}"

TraitItem ::= FunctionDecl | TypeDecl

ImplDecl ::= "impl" GenericParams? Type ("for" Type)? WhereClause?
             "{" ImplItem* "}"

ImplItem ::= FunctionDecl | ProcedureDecl
```

### A.6.3 Generic Parameters
```ebnf
GenericParams ::= "<" GenericParam ("," GenericParam)* ">"
GenericParam ::= TypeParam | ConstParam

TypeParam ::= Identifier (":" TraitBounds)?
ConstParam ::= "const" Identifier ":" Type

TraitBounds ::= Trait ("+" Trait)*

WhereClause ::= "where" WherePredicate ("," WherePredicate)*
WherePredicate ::= Type ":" TraitBounds
```

## A.7 Module Grammar

```ebnf
ModuleItem ::= ImportDecl
             | FunctionDecl
             | ProcedureDecl
             | RecordDecl
             | EnumDecl
             | ModalDecl
             | TraitDecl
             | ImplDecl
             | EffectDecl
             | TypeAlias

ImportDecl ::= "import" ImportPath ("as" Identifier)? ";"
ImportPath ::= Identifier ("::" Identifier)*

TypeAlias ::= Visibility? "type" Identifier GenericParams? "=" Type ";"

EffectDecl ::= "effect" Identifier ("{" EffectAtom* "}")? ";"
EffectAtom ::= Identifier ("(" EffectParam ("," EffectParam)* ")")?

Visibility ::= "public" | "internal" | "private"
```

## A.8 Contract Grammar

```ebnf
Assertion ::= LogicExpr

LogicExpr ::= LogicTerm
            | LogicExpr "&&" LogicExpr
            | LogicExpr "||" LogicExpr
            | "!" LogicExpr
            | LogicExpr "=>" LogicExpr

LogicTerm ::= Expr CompareOp Expr
            | "forall" "(" Identifier "in" Expr ")" "{" Assertion "}"
            | "exists" "(" Identifier "in" Expr ")" "{" Assertion "}"
            | "@old" "(" Expr ")"
            | "result"
            | Expr

CompareOp ::= "==" | "!=" | "<" | ">" | "<=" | ">="
```

## A.9 Effect Grammar

```ebnf
EffectList ::= Effect (";" Effect)*

Effect ::= EffectAtom
         | "effects" "(" Identifier ")"
         | "!" Effect
         | "_?"

EffectAtom ::= "alloc" "." AllocEffect
             | "fs" "." FileSystemEffect
             | "net" "." NetworkEffect
             | "time" "." TimeEffect
             | "thread" "." ThreadEffect
             | "render" "." RenderEffect
             | "audio" "." AudioEffect
             | "input" "." InputEffect
             | "ffi" "." FFIEffect
             | "unsafe" "." UnsafeEffect
             | "panic"
             | Identifier

AllocEffect ::= "heap" | "region" | "stack" | "temp" | "*"
FileSystemEffect ::= "read" | "write" | "delete"
NetworkEffect ::= "read" "(" Direction ")" | "write"
Direction ::= "inbound" | "outbound"
TimeEffect ::= "read" | "sleep"
ThreadEffect ::= "spawn" | "join" | "atomic"
RenderEffect ::= "draw" | "compute"
AudioEffect ::= "play"
InputEffect ::= "read"
FFIEffect ::= "call"
UnsafeEffect ::= "ptr"
```

---
# Appendix B: Error Code Catalog

This appendix catalogs all normative error codes referenced throughout the specification. Implementations MUST emit these codes for the corresponding violations.

## B.1 Parsing Errors (E10xx)

| Code | Description | Section |
|------|-------------|---------|
| E1001 | Unexpected token | §2 |
| E1002 | Unterminated string literal | §2.4.5 |
| E1003 | Unterminated block comment | §2.2 |
| E1004 | Invalid escape sequence | §2.4.4-5 |
| E1005 | Invalid integer literal | §2.4.1 |
| E1006 | Invalid float literal | §2.4.2 |

## B.2 Name Resolution Errors (E20xx)

| Code | Description | Section |
|------|-------------|---------|
| E2001 | Undefined identifier | §30-33 |
| E2002 | Ambiguous name | §31.2 |
| E2003 | Circular import | §31 |
| E2004 | Module not found | §33.1 |
| E2005 | Private item access violation | §32 |

## B.3 Type Errors (E2xxx)

| Code | Description | Section |
|------|-------------|---------|
| E2101 | Type mismatch | §4-9 |
| E2102 | Unknown type | §4 |
| E2103 | Trait not implemented | §8 |
| E2104 | Conflicting trait implementations | §8 |
| E2105 | Type parameter mismatch | §9 |
| E2106 | Const generic mismatch | §9.6 |
| E2107 | Array size mismatch | §5.1 |
| E2108 | Infinite type recursion | §4 |

## B.4 Modal Errors (E30xx)

| Code | Description | Section |
|------|-------------|---------|
| E3001 | Invalid state transition | §10-12 |
| E3002 | Procedure called in wrong state | §10.3 |
| E3003 | Unreachable state | §11.6 |
| E3004 | Non-exhaustive state coverage | §12.3 |
| E3005 | Modal state invariant violation | §19.4 |

## B.5 Contract Errors (E40xx)

| Code | Description | Section |
|------|-------------|---------|
| E4001 | Precondition may fail | §17.2 |
| E4002 | Postcondition cannot be proven | §17.3 |
| E4003 | Invariant violation | §19 |
| E4004 | Loop invariant not maintained | §19.5 |
| E4005 | Invalid @old reference | §17.6 |
| E4006 | Contract references private field | §17 |

## B.6 Region/Memory Errors (E50xx)

| Code | Description | Section |
|------|-------------|---------|
| E5001 | Region escape detected | §29.4 |
| E5002 | Region outlived allocation | §28 |
| E5003 | Invalid region nesting | §28.3 |
| E5004 | Region allocation without declaration | §28.5 |

## B.7 Permission Errors (E60xx)

| Code | Description | Section |
|------|-------------|---------|
| E6001 | Permission violation | §26 |
| E6002 | Moved value used | §26.3 |
| E6003 | Mutable reference to immutable data | §26 |
| E6004 | Isolated permission violation | §26.5 |

## B.8 Effect Errors (E90xx)

| Code | Description | Section |
|------|-------------|---------|
| E9001 | Missing effect declaration | §21-24 |
| E9002 | Effect exceeds declared needs | §22 |
| E9003 | Forbidden effect used | §22.4 |
| E9004 | Effect budget exceeded | §23 |
| E9005 | Async effect mask violation | §22.7 |
| E9201 | Callback exceeds effect mask | §36.5, §60 |

## B.9 Typed Holes (E95xx)

| Code | Description | Section |
|------|-------------|---------|
| E9501 | Unfilled typed hole | §14.10 |
| E9502 | Typed hole elaborated to trap (warning) | §14.10 |
| E9503 | Hole effects exceed function effects | §14.10 |

## B.10 FFI Errors (E97xx)

| Code | Description | Section |
|------|-------------|---------|
| E9701 | FFI call without ffi.call effect | §57 |
| E9702 | Invalid type across FFI boundary | §58 |
| E9703 | Mismatched calling convention | §57 |
| E9704 | Region value crosses FFI boundary | §59 |

## B.11 Concurrency Errors (E78xx)

| Code | Description | Section |
|------|-------------|---------|
| E7801 | JoinHandle escape from scope | §35.5 |

## B.12 Warnings (W0xxx-W9xxx)

| Code | Description | Section |
|------|-------------|---------|
| W0101 | Alias collides with real name | §2.6 |
| W7800 | Implicit join at scope end | §35.5 |

## B.13 Implementation-Defined Diagnostics

Implementations MAY define additional error codes in the following ranges:
- **I1000-I1999**: Implementation-specific parsing warnings
- **I2000-I2999**: Implementation-specific type system warnings
- **I9000-I9999**: Implementation-specific optimization hints

These codes SHOULD be documented in the implementation's reference manual and MUST NOT conflict with normative codes defined in this specification.

---
