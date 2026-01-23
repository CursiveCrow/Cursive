# PART III: TYPE SYSTEM - Writing Plan

**Version**: 1.0
**Date**: 2025-11-03
**Status**: Approved Plan
**Critical Change**: Predicate terminology replaces "predicate" for concrete code reuse mechanisms
**Terminology Updates**: "predicate"->"predicate", "for"->"loop", "effect"->"grant", System 1->System 3
**Primary Source**: `old_Type-System.md` (~7129 lines, comprehensive)
**Validation**: Checked against updated `Proposed_Organization.md` (dependency-driven structure)

---

## Mission

Write a complete, correct, ISO/ECMA-compliant formal specification for PART III, using **predicates** (not "predicates") for concrete code reuse, **System 3** permissions (binding categories × permissions), and correct **`loop`** syntax (not `for`). Transform content from old_Type-System.md into publication-ready normative specification with unified predicate system and proper terminology.

---

## Critical Terminology Decisions

**IMPORTANT**: The following terminology changes apply throughout Part III:

###

 1. **Predicates (Cursive Keyword)**:
- **Cursive uses**: `predicate` keyword for concrete code reuse
- **Critical rule**: ALL procedures in predicates MUST have bodies (concrete implementations)
- **Distinction**: `contract` keyword for abstract interfaces (NO bodies allowed)
- **Not to confuse with**: Rust's `trait` (which allows both abstract and concrete methods)
- **Rationale**: Clear separation between concrete code reuse (predicates) and behavioral contracts (contracts)
- **Cursive design principle**:
  - `predicate` = Concrete code reuse only
  - `contract` = Abstract interface specification only

### 2. **Loop Syntax**:
- ❌ **Old**: `for item in collection { }`
- ✅ **New**: `loop item in collection { }`
- **Rationale**: `for` keyword does not exist in Cursive

### 3. **Binding and Permission System**:
- ❌ **System 1**: `imm / mut / own` (single axis)
- ✅ **System 3**: Three orthogonal axes:
  - **Rebindability**: `let` (non-rebindable) / `var` (rebindable)
  - **Ownership**: `=` (owning) / `<-` (reference)
  - **Permissions**: `const` / `unique` / `shared`

**Binding Matrix:**
| | Owning (`=`) | Reference (`<-`) |
|---|---|---|
| **Non-rebindable (`let`)** | `let x: T = value` | `let x: T <- value` |
| **Rebindable (`var`)** | `var x: T = value` | `var x: T <- value` |

- **Rationale**: The `<-` operator creates reference bindings (non-owning), while `=` creates owning bindings
- **Reading**: `let x <- value` reads as "x references from value" or "x is bound to reference value"

### 4. **Grant System**:
- ❌ **Old**: "effects" or "effect polymorphism"
- ✅ **New**: "grants" or "grant polymorphism"
- **Forward reference**: Canonical specification in Part IX

### 5. **Structural Changes**:
- **Dependency-driven ordering**: Subtyping moved early (foundational)
- **Unified predicates**: All predicate content in Section 3.7
- **New sections**: Union Types (3.3.3), Range Types (3.2.7)

---

## Core Writing Principles

1. **TRANSFORM, not copy-paste** - Formalize existing content with correct terminology
2. **ONE canonical example per concept** - Single comprehensive demonstration
3. **Complete normative precision** - Every requirement with shall/should/may
4. **Implementer-ready** - Compiler implementation guidance included
5. **Publication quality** - ISO/ECMA standards compliance

---

## File Structure

All files in `Spec/03_Type-System/` directory:

| File | Type | Subsections | Primary Source |
|------|------|-------------|----------------|
| `03-0_Type-Foundations.md` | 🔄 TRANSFORM | 3.0.1-3.0.6 (6) | old 2.0 lines 26-1211 (~1185 lines) |
| `03-1_Subtyping-Equivalence.md` | 🔄 EXTRACT | 3.1.1-3.1.4 (4) | old 2.0.6 lines 169-336 (~167 lines) |
| `03-2_Primitive-Types.md` | 🔄 TRANSFORM + ⭐ NEW | 3.2.1-3.2.7 (7) | old 2.1 lines 1212-2050 (~838 lines) + NEW |
| `03-3_Composite-Types.md` | 🔄 SYNTHESIZE + ⭐ NEW | 3.3.1-3.3.3 (3) | old 2.2+2.3 lines 2051-3256 (~1204 lines) + NEW |
| `03-4_Collection-Types.md` | 🔄 TRANSFORM | 3.4.1-3.4.3 (3) | old 2.4 lines 3257-3683 (~426 lines) |
| `03-5_Function-Types.md` | 🔄 TRANSFORM | 3.5.1-3.5.4 (4) | old 2.10 lines 4898-5130 (~232 lines) |
| `03-6_Pointer-Types.md` | 🔄 SYNTHESIZE | 3.6.1-3.6.2 (2) | old 2.5+2.6 lines 3684-3919 (~234 lines) |
| `03-7_Predicates.md` | 🔄 EXPAND | 3.7.1-3.7.4 (4) | old 2.7+2.8 lines 3920-4086 (~165 lines) |
| `03-8_Generics.md` | 🔄 TRANSFORM | 3.8.1-3.8.5 (5) | old 2.9 lines 4087-4897 (~810 lines) |
| `03-9_Type-Bounds.md` | 🔄 SYNTHESIZE + ⭐ NEW | 3.9.1-3.9.4 (4) | Scattered + NEW grant bounds |
| `03-10_Type-Aliases.md` | 🔄 TRANSFORM | 3.10.1-3.10.3 (3) | old 2.11 lines 5131-5216 (~85 lines) |
| `03-11_Type-Introspection.md` | 🔄 TRANSFORM | 3.11.1-3.11.3 (3) | old 2.12 lines 5217-5328 (~111 lines) |

**Total**: 12 sections, 44 subsections

**Structural changes from old_Type-System.md**:
- Section 3.1 Subtyping moved up (from old 2.0.6, was section 3.8 in proposed)
- Section 3.2 adds Range Types (3.2.7)
- Section 3.3 adds Union Types (3.3.3)
- Section 3.7 Predicates unified (combines old 2.7 + 2.8, moves from end)
- Section 3.9 adds Grant Bounds (forward reference to Part IX)
- Dependency-driven ordering: Subtyping -> Types -> Predicates -> Generics -> Bounds

---

## Section 3.0: Type System Foundations 🔄 TRANSFORM

### Source
- `old_Type-System.md` lines 26-1211 (~1185 lines, comprehensive)
- Sections: 2.0.0-2.0.9 (Type System Overview through Permission Integration)

### Strategy
Transform comprehensive type foundations with System 3 permission updates throughout. This is the most extensive section requiring systematic permission system conversion from `own/mut/imm` to the orthogonal system: rebindability (`let`/`var`) × ownership (`=`/`<-`) × permissions (`const`/`unique`/`shared`).

### Subsections

#### 3.0.1 Type System Overview
- Classification of types (primitive, composite, abstract, parametric)
- Type categories: value types, reference types, function types
- Nominal vs structural typing
- Type checking approach (bidirectional)
- Type inference strategy
- Relationship to other parts (cross-references)

**Normative statements**:
- "Cursive employs a primarily nominal type system with structural typing for tuples and function types."
- "Type checking shall use bidirectional type inference."

#### 3.0.2 Type Formation Rules
- Well-formed types (formal definition)
- Type contexts and environments (Γ)
- Type formation judgments
- Kinding rules for type constructors
- Ill-formed types and error conditions

**Grammar**:
```
Type τ ::= PrimitiveType
         | CompositeType
         | FunctionType
         | GenericType⟨τ₁, ..., τₙ⟩
         | τ₁ \/ τ₂              (union type)
         | Self                  (self type)
```

**Inference rules**:
```
[Type-Well-Formed]
Γ ⊢ τ : Type    components(τ) well-formed
----------------------------------------
Γ ⊢ τ well-formed
```

#### 3.0.3 Type Environments (Γ)
- Environment structure and lookup
- Binding types to identifiers
- Scope-based environment extension
- Shadowing rules
- Environment composition

**Formal definition**:
```
Γ ::= ∅                          (empty)
    | Γ, x: τ                    (binding extension)
    | Γ, T: Kind                 (type binding)
    | Γ, α: τ where φ            (constrained type parameter)
```

#### 3.0.4 Type Equality and Equivalence
- Nominal equivalence (name-based)
- Structural equivalence (shape-based)
- Type alias transparency
- Equivalence for generic types
- Subtype equivalence

**From old lines 250-336**: Comprehensive formal rules

**Normative statement**:
- "Two types are equivalent if they denote the same type under the applicable equivalence relation."

#### 3.0.5 Subtyping Relation
**NOTE**: This subsection provides introduction only. Full specification in Section 3.1.

- Subtyping concept introduction
- Role in type checking
- Variance preview
- Forward reference to §3.1 for complete rules

**Cross-reference**: "The complete subtyping relation is specified in §3.1 Subtyping and Equivalence."

#### 3.0.6 Type Safety Properties
- Progress theorem
- Preservation theorem
- Type soundness guarantee
- Memory safety relationship
- Reference to formal semantics (Appendix)

**Formal property**:
```
Type Safety Theorem:
If ⊢ e : τ and ∅ ⊢ τ well-formed, then either:
  (a) e is a value, or
  (b) ∃e'. e ->* e' (progress + preservation)
```

**Normative statement**:
- "A well-typed program shall not exhibit undefined behavior arising from type errors."

### Formal Elements Required
- **Definition 3.0.1**: Well-formed type
- **Definition 3.0.2**: Type environment
- **Definition 3.0.3**: Type equivalence
- **Definition 3.0.4**: Subtyping relation (preview)
- **Theorem 3.0.1**: Type safety
- **Algorithm 3.0.1**: Type well-formedness checking
- **Inference rules**: Type-Well-Formed, Type-Equiv, Type-Env-Lookup

### Canonical Example
Single comprehensive program demonstrating:
- Multiple type categories (primitive, composite, function, generic)
- Type environments and scoping
- Type equivalence and subtyping
- Type safety guarantees
- System 3 permissions in type annotations

```cursive
// Canonical Example 3.0: Type System Foundations

// Primitive types with System 3 permissions
// System 3 = Rebindability (let/var) × Ownership (=/←) × Permissions (const/unique/shared)
let x: const i32 = 42       // Binding: let (non-rebindable, owning) | Permission: const (immutable)
var counter: const i32 = 0  // Binding: var (rebindable, owning) | Permission: const (immutable)

// Composite types (product)
record Point {
    x: f64,
    y: f64
}

// Composite types (sum)
enum SearchResult<T> {
    Found(T),
    NotFound
}

// Function types
function add(a: i32, b: i32): i32 {
    result a + b
}

// Generic types with bounds
function find<T>(items: [T], predicate: (T) -> bool): SearchResult<T>
    where T: Copy
{
    loop item in items {
        if predicate(item) {
            result SearchResult::Found(copy item)
        }
    }
    result SearchResult::NotFound
}

// Type environment demonstration
procedure demonstrate_scoping() {
    let outer: const i32 = 10        // Γ₁ = {outer: i32}
    {
        let inner: const f64 = 3.14  // Γ₂ = Γ₁, {inner: f64}
        // Both outer and inner visible
    }
    // Only outer visible
}

// Type equivalence
type Coordinate = (f64, f64)         // Transparent alias
let p1: const Point = Point { x: 1.0, y: 2.0 }
let p2: const Coordinate = (1.0, 2.0)      // Structurally different (tuple vs record)

// Subtyping example (preview)
let value: const i32 = 42
let immutable_ref: const i32 <- value     // reference binding for subtype context
```

---

## Section 3.1: Subtyping and Equivalence 🔄 EXTRACT

### Source
- `old_Type-System.md` lines 169-336 (~167 lines)
- Section 2.0.6: Subtyping, Equivalence, and Type Compatibility

### Strategy
Extract and elevate subtyping to standalone foundational section. This material is complete and formal, requiring primarily extraction and reorganization with terminology updates.

### Subsections

#### 3.1.1 Type Equivalence Rules
- Nominal equivalence definition
- Structural equivalence definition
- Alias transparency rules
- Generic type equivalence
- Formal equivalence judgments

**From old lines 250-294**: Complete formal rules

**Inference rules**:
```
[Equiv-Refl]
-----------
τ ≡ τ

[Equiv-Trans]
τ₁ ≡ τ₂    τ₂ ≡ τ₃
--------------------
τ₁ ≡ τ₃

[Equiv-Alias]
type T = τ
----------
T ≡ τ
```

#### 3.1.2 Subtyping Rules
- Subtype relation definition (τ <: υ)
- Reflexivity, transitivity, antisymmetry
- Subtyping for primitive types
- Subtyping for composite types
- Function type subtyping (contravariant arguments, covariant return)

**From old lines 169-249**: Comprehensive inference rules

**Key rules**:
```
[Sub-Refl]
----------
τ <: τ

[Sub-Trans]
τ₁ <: τ₂    τ₂ <: τ₃
--------------------
τ₁ <: τ₃

[Sub-Function]
υ₁ <: τ₁    τ₂ <: υ₂    ε₁ <: ε₂
--------------------------------------------
(τ₁) -> τ₂ ! ε₁  <:  (υ₁) -> υ₂ ! ε₂
  contravariant↑      covariant↑   covariant↑
```

#### 3.1.3 Variance
- Variance definition (covariant, contravariant, invariant)
- Variance for type constructors
- Variance table for all Cursive type constructors
- Variance checking algorithm
- Safe vs unsafe variance

**From old lines 224-237**: Complete variance table

**Variance Table**:
| Type Constructor | Variance | Rationale |
|-----------------|----------|-----------|
| `Ptr<T>@State` | Covariant in T, Invariant in State | Safe pointer with modal state |
| `[T]` | Covariant | Immutable view |
| `(T₁, T₂)` | Covariant | Product type |
| `T₁ -> T₂` | T₁: Contravariant, T₂: Covariant | Function subtyping |

**Note**: Reference bindings (`let x: T <- value`) do not introduce type constructors but follow invariance for type safety. Complete variance rules for all Cursive type constructors will be specified in the final section.

#### 3.1.4 Type Compatibility
- Compatibility relation (weaker than equivalence)
- Implicit conversions (if any)
- Numeric type compatibility
- Compatibility in assignment context
- Compatibility in function call context

**Normative statement**:
- "A type τ is compatible with type υ in context C if either τ <: υ or an implicit conversion exists."

### Formal Elements Required
- **Definition 3.1.1**: Type equivalence relation
- **Definition 3.1.2**: Subtyping relation
- **Definition 3.1.3**: Variance
- **Definition 3.1.4**: Type compatibility
- **Algorithm 3.1.1**: Subtype checking
- **Algorithm 3.1.2**: Variance computation
- **Inference rules**: Sub-Refl, Sub-Trans, Sub-Function, Sub-Record, Sub-Variant, Equiv-*
- **Variance Table**: Complete table for all type constructors

### Canonical Example
Single program demonstrating:
- Type equivalence (nominal and structural)
- Subtyping relationships
- Variance in action (covariant, contravariant, invariant)
- Function subtyping
- Type compatibility

```cursive
// Canonical Example 3.1: Subtyping and Equivalence

// Type equivalence
record Point { x: f64, y: f64 }
type Coordinate = Point              // Nominal alias
let p1: const Point = Point { x: 0.0, y: 0.0 }
let p2: const Coordinate = p1              // OK: Point ≡ Coordinate

// Structural equivalence for tuples
let tuple1: const (i32, i32) = (1, 2)
let tuple2: const (i32, i32) = (3, 4)      // Same type structurally

// Subtyping: numeric widening
let narrow: const i32 = 42
let wide: const i64 = (narrow as i64)  // Explicit: no implicit subtyping

// Function subtyping (contravariance + covariance)
function specific_processor(input: i32): i64 {
    result input as i64
}

function general_processor(input: i16): i32 {
    result input as i32
}

// Can substitute general_processor where specific_processor expected:
// (i32) -> i64  <:  (i16) -> i32  is FALSE
// Correct subtyping: (i16) -> i64  <:  (i32) -> i32
// - Accepts more inputs (i16 < i32: contravariant)
// - Returns more specific (i64 > i32: covariant)

// Variance in generic types
function covariant_example() {
    let arrays: [[i32]] = [[1, 2], [3, 4]]
    // [T] is covariant in T
    // If i32 <: i64, then [i32] <: [i64] would hold (if subtyping existed)
}

// Invariant reference example
function invariant_example() {
    var value: const i32 = 42
    let mutable_ref: const i32 <- value
    // Cannot treat reference binding as different type
    // Type system enforces invariance
}

// Type compatibility in function calls
function process(x: i64) { }

procedure call_with_compatible() {
    let value: const i32 = 100
    process(value as i64)  // Explicit cast required
}
```

---

## Section 3.2: Primitive Types 🔄 TRANSFORM + ⭐ NEW

### Source
- `old_Type-System.md` lines 1212-2050 (~838 lines, comprehensive)
- Section 2.1: Primitive Types

### Strategy
Transform existing comprehensive primitive type specification with correct `loop` syntax

### Subsections

#### 3.2.1 Integer Types
- Signed integers: i8, i16, i32, i64, i128, isize
- Unsigned integers: u8, u16, u32, u64, u128, usize
- Size and range specifications
- Overflow behavior (wrapping, checked, saturating, overflowing)
- Bit-level operations
- Numeric literals

**From old lines 1212-1450**: Complete specification

**Normative statements**:
- "Integer types shall have two's complement representation."
- "Overflow behavior shall be deterministic and specified per operation context."

#### 3.2.2 Floating-Point Types
- IEEE 754 compliance (f32, f64)
- Special values (NaN, ±∞, ±0)
- Rounding modes
- Precision guarantees
- Operations and semantics
- Floating-point literals

**From old lines 1451-1610**: Complete IEEE 754 spec

**Normative statement**:
- "Floating-point types shall conform to IEEE 754-2019 binary floating-point arithmetic."

#### 3.2.3 Boolean Type
- bool type definition
- True and false values
- Boolean operations (&&, ||, !, ^)
- Short-circuit evaluation
- Boolean literals

**From old lines 1611-1685**: Complete specification

#### 3.2.4 Character Type
- char type (Unicode scalar value)
- UTF-8 encoding considerations
- Character literals
- Escape sequences
- Unicode property access

**From old lines 1686-1780**: Complete specification

**Normative statement**:
- "The char type shall represent a Unicode scalar value (range U+0000 to U+D7FF and U+E000 to U+10FFFF)."

#### 3.2.5 Unit Type
- Unit type `()` definition
- Zero-size type
- Use cases (procedures with no return value)
- Unit value representation

**From old lines 1781-1820**: Complete specification

#### 3.2.6 Never Type
- Never type `!` definition
- Diverging expressions
- Use cases (panic, infinite loop, process exit)
- Subtyping properties (⊥ type)

**From old lines 1821-1900**: Complete specification

**Key property**:
```
[Never-Subtype]
For all types τ:
---------------
! <: τ
```

#### 3.4.# Range Types ⭐ NEW
- Six range type constructors
- Range syntax and semantics
- Use in `loop` iteration (essential for language)
- Slicing operations
- Range as first-class values

**NEW CONTENT** (synthesize from operator docs + user requirements):

**Six Range Types**:
1. **`Range<T>`**: `a..b` - Half-open interval [a, b)
2. **`RangeInclusive<T>`**: `a..=b` - Closed interval [a, b]
3. **`RangeFrom<T>`**: `a..` - Unbounded from [a, ∞)
4. **`RangeTo<T>`**: `..b` - Unbounded to (-∞, b)
5. **`RangeToInclusive<T>`**: `..=b` - Unbounded to inclusive (-∞, b]
6. **`RangeFull`**: `..` - Fully unbounded (-∞, ∞)

**Grammar**:
```
RangeExpr ::= Expr '..' Expr          // Range<T>
            | Expr '..=' Expr          // RangeInclusive<T>
            | Expr '..'                // RangeFrom<T>
            | '..' Expr                // RangeTo<T>
            | '..=' Expr               // RangeToInclusive<T>
            | '..'                     // RangeFull
```

**Type Formation**:
```
[Type-Range]
Γ ⊢ T : Type    T has ordering
-------------------------------
Γ ⊢ Range<T> : Type

[Type-RangeInclusive]
Γ ⊢ T : Type    T has ordering
-------------------------------
Γ ⊢ RangeInclusive<T> : Type
```

**Use Cases**:

1. **Loop Iteration** (PRIMARY USE CASE):
```cursive
// Half-open range
loop i in 0..10 {
    // i ranges from 0 to 9
}

// Closed range
loop i in 0..=10 {
    // i ranges from 0 to 10 (inclusive)
}

// Infinite range
loop i in 0.. {
    if i > 100 { break }
    // Infinite iteration from 0
}
```

2. **Slicing**:
```cursive
let array: [i32; 100] = initialize_array()
let slice1 = array[10..20]     // Elements 10-19
let slice2 = array[..50]       // Elements 0-49
let slice3 = array[50..]       // Elements 50-99
let slice4 = array[..]         // All elements
```

3. **First-Class Values**:
```cursive
function make_range(start: i32, end: i32): Range<i32> {
    result start..end
}

let r = make_range(5, 15)
loop i in r {
    println("{}", i)
}
```

**Normative statements**:
- "Range types shall be zero-cost abstractions compiled to direct index arithmetic."
- "A Range<T> shall represent a half-open interval [start, end)."
- "A RangeInclusive<T> shall represent a closed interval [start, end]."
- "Range types shall be usable in loop iteration expressions."

**Value Provided**:
- Type-safe iteration bounds for `loop in` syntax
- Natural slicing syntax: `arr[range]`
- First-class interval semantics
- Six distinct interval types for precise semantics
- Zero-cost compilation to index arithmetic

### Formal Elements Required
- **Definition 3.2.1**: Integer type family
- **Definition 3.2.2**: Floating-point type conformance (IEEE 754)
- **Definition 3.2.3**: Boolean type
- **Definition 3.2.4**: Character type (Unicode scalar value)
- **Definition 3.2.5**: Unit type
- **Definition 3.2.6**: Never type
- **Definition 3.2.7**: Range type family (6 types)
- **Algorithm 3.2.1**: Integer overflow detection
- **Algorithm 3.2.2**: Range iteration protocol
- **Inference rules**: Type-Int, Type-Float, Type-Bool, Type-Char, Type-Range, Type-RangeInclusive
- **Error codes**: E0301-E0320 (primitive type errors)

### Canonical Example
Single program demonstrating all primitive types with System 3 permissions and correct `loop` syntax:

```cursive
// Canonical Example 3.2: Primitive Types

procedure demonstrate_primitives() {
    // Integer types
    let byte: const u8 = 255
    let small: const i16 = -32768
    let standard: const i32 = 2_147_483_647
    let large: const i64 = 9_223_372_036_854_775_807

    // Note: Integer overflow behavior is implementation-defined
    // Standard library may provide overflow-checking operations

    // Floating-point types
    let single: const f32 = 3.14159
    let double: const f64 = 2.718281828459045
    let infinity: const f64 = (1.0 / 0.0)   // +∞
    let nan: const f64 = (0.0 / 0.0)        // NaN

    // Boolean type
    let flag: const bool = true
    let result = flag && false              // Short-circuit evaluation

    // Character type
    let ascii_char: const char = 'A'
    let unicode_char: const char = '🦀'
    let escaped: const char = '\n'

    // Unit type
    let unit_value: () = ()

    // Never type (diverging function)
    function diverge(): ! {
        loop {
            // Infinite loop, never returns
        }
    }

    // Range types - Essential for loop syntax

    // 1. Range<i32> - Half-open [0, 10)
    loop i in 0..10 {
        println("Half-open: {}", i)  // Prints 0-9
    }

    // 2. RangeInclusive<i32> - Closed [0, 10]
    loop i in 0..=10 {
        println("Inclusive: {}", i)  // Prints 0-10
    }

    // 3. RangeFrom<i32> - Unbounded [5, ∞)
    loop i in 5.. {
        if i > 15 { break }
        println("From: {}", i)       // Prints 5-15
    }

    // 4-6. Range types for slicing
    let array: [i32; 20] = [0; 20]
    let slice1 = array[5..15]        // RangeTo: elements 5-14
    let slice2 = array[..10]         // RangeTo: elements 0-9
    let slice3 = array[10..]         // RangeFrom: elements 10-19
    let slice4 = array[..]           // RangeFull: all elements

    // Range as first-class value
    let range: Range<i32> = 0..100
    loop value in range {
        if value % 10 == 0 {
            println("Multiple of 10: {}", value)
        }
    }
}
```

---

## Section 3.3: Composite Types 🔄 SYNTHESIZE + ⭐ NEW

### Source
- `old_Type-System.md` lines 2051-3256 (~1204 lines)
- Section 2.2: Product Types (lines 2051-2565, ~514 lines)
- Section 2.3: Sum and Modal Types (lines 2566-3256, ~690 lines)
- NEW: Union Types (user requirement)

### Strategy
Synthesize product types (2.2) and sum types (2.3) into unified composite types section. Add new Section 3.3.3 Union Types for discriminated unions. Update all examples with System 3 permissions.

**Design Note on Product Types**: Cursive has **two** product type forms, not four:

1. **Tuples**: Structural types with positional access only (`(T1, T2)`)
2. **Records**: Nominal types with **both** named and positional access

**Why tuple-structs are not separate**: Traditional languages (Rust) separate "tuple-structs" (nominal + positional) from "records" (nominal + named). Cursive unifies these: records support both access patterns. The grammar already supports `.0` syntax alongside `.field_name`. Developers choose the appropriate access style based on context:
- Named access (`.field_name`) when field semantics are important
- Positional access (`.0`, `.1`) for generic operations, coordinate-like data, or ergonomic terseness

This design reduces language complexity while increasing expressiveness. Fields are indexed in declaration order (left-to-right, zero-based).

**Why newtypes are not included**: For nominal type distinction, use single-field records (`record Meters { value: f64 }`). For transparent naming, use type aliases (`type Meters = f64`). Cursive's permission system (const/unique/shared), contract system (sequent clauses), and modal types provide more powerful type safety mechanisms than traditional newtypes, making dedicated newtype syntax redundant.

### Subsections

#### 3.3.1 Product Types (Tuples, Records)
- **Tuples**: Structural anonymous products
  - Tuple syntax: `(τ₁, τ₂, ..., τₙ)`
  - Field access by position: `.0`, `.1`
  - Structural typing (equivalence by shape)
  - Empty tuple as unit type `()`

- **Records**: Nominal products with dual access
  - Record declaration syntax
  - Field access by name: `.field_name`
  - Field access by position: `.0`, `.1`, `.2` etc. (zero-based, declaration order)
  - Developers choose access style based on context
  - Nominal typing (equivalence by name)
  - Initialization and construction
  - Struct update syntax

**From old lines 2051-2565**: Complete product type specification

**Grammar**:
```
ProductType ::= TupleType | RecordType

TupleType ::= '(' Type (',' Type)* ')'

RecordDecl ::= 'record' Ident '{' FieldDecl* '}'

FieldAccess ::= Expr '.' Ident              // Named field access
             | Expr '.' IntegerLiteral      // Positional field access (0-based)
```

**Note**: Both tuples and records support positional access (`.0`, `.1`). Only records additionally support named access (`.field_name`).

**Example**:
```cursive
// Tuple (structural)
let point_tuple: (f64, f64) = (3.0, 4.0)
let x = point_tuple.0           // Positional only

// Record (nominal with dual access)
record Point {
    x: f64,
    y: f64
}

let p = Point { x: 1.0, y: 2.0 }
let x1 = p.x                    // Named access
let x2 = p.0                    // Positional access (same as p.x)
let y1 = p.y                    // Named access
let y2 = p.1                    // Positional access (same as p.y)
```

#### 3.3.2 Sum Types (Enums)
- **Enums**: Discriminated unions
  - Enum declaration syntax
  - Variant constructors
  - Payload types (tuple variants, struct variants, unit variants)
  - Pattern matching requirement
  - Tag representation
  - Generic enum parameters

- **Common enum patterns**:
  - Optional values (two-variant enums)
  - Error handling (success/failure variants)
  - State machines
  - Message passing

**From old lines 2566-3256**: Complete sum type specification

**Grammar**:
```
EnumDecl ::= 'enum' Ident TypeParams? '{' VariantDecl* '}'

VariantDecl ::= Ident
              | Ident '(' Type (',' Type)* ')'
              | Ident '{' FieldDecl* '}'
```

**Example**:
```cursive
// Enum with different variant types
enum Shape {
    Circle(f64),                              // Tuple variant
    Rectangle { width: f64, height: f64 },    // Struct variant
    Point                                     // Unit variant
}

// Generic enum example
enum Status<T> {
    Ready(T),
    Waiting,
    Failed(i32)  // Error code
}

let status: Status<f64> = Status::Ready(3.14)

match status {
    Status::Ready(v) => println("Ready with value: {}", v),
    Status::Waiting => println("Waiting"),
    Status::Failed(code) => println("Failed with code: {}", code)
}
```

#### 3.3.3 Union Types ⭐ NEW
- **Union types**: Untagged unions (safe discriminated)
- Union type syntax: `τ₁ \/ τ₂`
- Use cases: control-flow joins, modal state unions
- Type-safe access (require exhaustive checking)
- Distinction from unsafe unions (Part XIV)

**NEW CONTENT** (synthesize from user requirements + modal system):

**Definition**:
A union type `τ₁ \/ τ₂` represents a value that is *either* of type `τ₁` *or* type `τ₂`. Union types are discriminated (safe) and require pattern matching or type testing to access.

**Grammar**:
```
UnionType ::= Type '\/' Type
            | Type ('\/' Type)+      // n-ary unions
```

**Type Formation**:
```
[Type-Union]
Γ ⊢ τ₁ : Type    Γ ⊢ τ₂ : Type
--------------------------------
Γ ⊢ τ₁ \/ τ₂ : Type
```

**Use Cases**:

1. **Control-Flow Joins**:
```cursive
function process(flag: bool): i32 \/ string {
    if flag {
        result 42              // i32 branch
    } else {
        result "error"         // string branch
    }
    // Return type: i32 \/ string
}
```

2. **Modal State Unions**:
```cursive
modal Connection {
    @Disconnected { retry_count: u32 }
    @Connected { handle: i32 }
    @Error { message: string }

    // Transition can result in multiple possible states
    @Connected -> attempt_send(data: [u8]) -> (@Connected \/ @Error)
}
```

3. **Type-Safe Access**:
```cursive
function handle_result(value: i32 \/ string) {
    // Requires exhaustive pattern matching
    match value {
        v: i32 => println("Number: {}", v),
        s: string => println("String: {}", s)
    }
}
```

**Properties**:
- **Commutativity**: `τ₁ \/ τ₂` ≡ `τ₂ \/ τ₁`
- **Associativity**: `(τ₁ \/ τ₂) \/ τ₃` ≡ `τ₁ \/ (τ₂ \/ τ₃)`
- **Idempotence**: `τ \/ τ` ≡ `τ`
- **Unit**: `τ \/ !` ≡ `τ` (never type is unit)

**Distinction from Unsafe Unions**:
- **Safe (discriminated)**: Union types `τ₁ \/ τ₂` carry runtime tag, require pattern match
- **Unsafe (raw)**: C-style unions (Part XIV) have no tag, allow reinterpretation

**Forward References**:
- Pattern matching: Part V (Expressions)
- Unsafe unions: Part XIV (Unsafe Behaviors and FFI)

**Normative statements**:
- "A union type shall maintain a runtime discriminator to identify the active variant."
- "Access to union type values shall require pattern matching or explicit type testing."
- "Union types shall not allow untagged reinterpretation. See Part XIV for unsafe unions."

### Formal Elements Required
- **Definition 3.3.1**: Product type (tuple, record with dual access)
- **Definition 3.3.2**: Sum type (enum) with generic parameters
- **Definition 3.3.3**: Union type (discriminated)
- **Definition 3.3.4**: Field indexing for records (zero-based, declaration order)
- **Algorithm 3.3.1**: Record field access (both named and positional)
- **Algorithm 3.3.2**: Enum variant matching
- **Algorithm 3.3.3**: Union type discrimination
- **Inference rules**: Type-Tuple, Type-Record, Type-Record-Positional-Access, Type-Enum, Type-Union
- **Error codes**: E0321-E0350 (composite type errors, including positional access errors)

### Canonical Example
Single comprehensive program demonstrating all composite types with System 3 permissions:

```cursive
// Canonical Example 3.3: Composite Types

// ===== Product Types =====

// Tuple (structural)
let point_tuple: (f64, f64) = (3.0, 4.0)
let x_coord = point_tuple.0

// Record (nominal with dual access)
record Point {
    x: f64,
    y: f64
}

let point_record = Point { x: 1.0, y: 2.0 }

// Named access
let x_named = point_record.x
let y_named = point_record.y

// Positional access (fields indexed in declaration order, zero-based)
let x_positional = point_record.0    // Same as point_record.x
let y_positional = point_record.1    // Same as point_record.y

// Choose access style based on context:
// - Named when field meaning is important
// - Positional for generic/coordinate-like operations

// Struct update syntax
let point_updated = Point { x: 5.0, ..point_record }

// ===== Sum Types =====

// Enum with multiple variant kinds
enum Message {
    Quit,                                    // Unit variant
    Move { x: i32, y: i32 },                // Struct variant
    Write(string),                           // Tuple variant
    ChangeColor(u8, u8, u8)                 // Multi-field tuple variant
}

function process_message(msg: Message) {
    match msg {
        Message::Quit => println("Quitting"),
        Message::Move { x, y } => println("Moving to ({}, {})", x, y),
        Message::Write(text) => println("Writing: {}", text),
        Message::ChangeColor(r, g, b) => println("Color: RGB({}, {}, {})", r, g, b)
    }
}

// Generic enum with multiple type parameters
enum Status<T> {
    Ready(T),
    Waiting,
    Failed(i32)  // Error code
}

function find_value(array: [i32], target: i32): Status<i32> {
    loop item in array {
        if item == target {
            result Status::Ready(item)
        }
    }
    result Status::Waiting  // Not found
}

// ===== Union Types =====

// Union type for control-flow joins
function get_value(use_int: bool): i32 \/ string {
    if use_int {
        result 42
    } else {
        result "forty-two"
    }
}

// Union type handling (requires pattern match)
procedure handle_union_value() {
    let value: i32 \/ string = get_value(true)

    match value {
        n: i32 => println("Integer: {}", n),
        s: string => println("String: {}", s)
    }
}

// Union type in modal state transitions
modal FileHandle {
    @Closed { path: string }
    @Open { descriptor: i32 }
    @Error { code: i32, message: string }

    // Transition can result in Open or Error (syntax specified in Part XI §11.1)
    @Closed -> open() -> (@Open \/ @Error)
}

// Three-way union
type Value = i32 \/ f64 \/ string

function process_value(v: Value) {
    match v {
        n: i32 => println("Int: {}", n),
        f: f64 => println("Float: {}", f),
        s: string => println("String: {}", s)
    }
}

// System 3 permissions with composite types
procedure demonstrate_permissions() {
    // Immutable reference binding to record
    let p: const Point = Point { x: 1.0, y: 2.0 }
    let immutable_ref: const Point <- p

    // Mutable unique reference binding
    var mut_point: const Point = Point { x: 0.0, y: 0.0 }
    let unique_ref: unique Point <- mut_point
    unique_ref.x = 10.0  // Mutation through unique reference binding

    // Shared mutable reference binding
    var shared_data: const Point = Point { x: 5.0, y: 5.0 }
    let shared_ref1: shared Point <- shared_data
    let shared_ref2: shared Point <- shared_data  // OK: shared allows aliasing
}
```

---

## Section 3.4: Collection Types 🔄 TRANSFORM

### Source
- `old_Type-System.md` lines 3257-3683 (~426 lines)
- Section 2.4: Collections and Views

### Strategy
Transform existing collection types specification with `loop` syntax corrections throughout. Primary changes: replace all `for` with `loop` in iteration examples.

### Subsections

#### 3.4.1 Arrays
- Fixed-size arrays: `[T; N]`
- Array literals and initialization
- Array indexing and bounds checking
- Multi-dimensional arrays
- Array type parameters (const generics)
- Zero-sized arrays

**From old lines 3257-3450**: Complete array specification

**Grammar**:
```
ArrayType ::= '[' Type ';' ConstExpr ']'

ArrayLiteral ::= '[' Expr (',' Expr)* ']'
               | '[' Expr ';' ConstExpr ']'    // Repeat syntax
```

**Example with correct `loop` syntax**:
```cursive
let array: [i32; 5] = [1, 2, 3, 4, 5]

// CORRECT: loop iteration
loop element in array {
    println("{}", element)
}

// Index access with bounds check
let value = array[2]  // Runtime bounds check
```

#### 3.4.2 Slices
- Dynamic-size views: `[T]`
- Slice creation from arrays
- Slice indexing and subslicing
- Slice iteration
- Relationship to arrays
- Fat pointer representation

**From old lines 3451-3570**: Complete slice specification

**Example**:
```cursive
let array: [i32; 10] = [0; 10]
let slice: [i32] = array[2..8]      // Creates slice view

// Iteration over slice
loop item in slice {
    println("{}", item)
}

// Subslicing
let subslice = slice[1..4]
```

#### 3.4.3 Strings
- String types: `string` (UTF-8)
- String literals
- String slicing (byte-indexed)
- Character iteration vs byte iteration
- String mutability considerations
- Encoding guarantees (UTF-8)

**From old lines 3571-3683**: Complete string specification

**Example**:
```cursive
let text: string = "Hello, 世界"

// Character iteration (CORRECT syntax)
loop ch in text.chars() {
    println("Char: {}", ch)
}

// Byte iteration
loop byte in text.bytes() {
    println("Byte: {:#x}", byte)
}

// String slicing (byte indices)
let hello = text[0..5]  // "Hello"
```

### Formal Elements Required
- **Definition 3.4.1**: Array type
- **Definition 3.4.2**: Slice type
- **Definition 3.4.3**: String type
- **Algorithm 3.4.1**: Array bounds checking
- **Algorithm 3.4.2**: Slice creation
- **Algorithm 3.4.3**: UTF-8 validation
- **Inference rules**: Type-Array, Type-Slice, Type-String
- **Error codes**: E0351-E0370 (collection errors)

### Canonical Example
Single program demonstrating all collection types with correct `loop` syntax:

```cursive
// Canonical Example 3.4: Collection Types

procedure demonstrate_collections() {
    // ===== Arrays =====

    // Fixed-size array
    let numbers: [i32; 5] = [10, 20, 30, 40, 50]

    // Array with repeat syntax
    let zeros: [i32; 100] = [0; 100]

    // Array iteration (CORRECT: loop, not for)
    loop num in numbers {
        println("Number: {}", num)
    }

    // Indexed iteration
    loop i in 0..numbers.length {
        println("numbers[{}] = {}", i, numbers[i])
    }

    // Bounds checking
    let value = numbers[2]           // OK: within bounds
    // let invalid = numbers[10]     // ERROR: out of bounds (compile-time if const)

    // Multi-dimensional array
    let matrix: [[i32; 3]; 3] = [
        [1, 2, 3],
        [4, 5, 6],
        [7, 8, 9]
    ]

    // ===== Slices =====

    // Create slice from array
    let full_slice: [i32] = numbers[..]
    let partial_slice: [i32] = numbers[1..4]  // Elements at indices 1, 2, 3

    // Slice iteration
    loop element in partial_slice {
        println("Slice element: {}", element)
    }

    // Subslicing
    let subslice = partial_slice[0..2]

    // Slice length (runtime property)
    let len = partial_slice.length

    // ===== Strings =====

    // String literals
    let greeting: string = "Hello, World!"
    let unicode: string = "Hello, 世界 🦀"

    // String slicing (BYTE indices, not character indices)
    let hello = greeting[0..5]      // "Hello"

    // Note: String iteration methods (characters, bytes, lines, etc.)
    // are provided by the standard library

    // ===== System 3 Permissions with Collections =====

    // Immutable array reference binding
    let arr: [i32; 3] = [1, 2, 3]
    let immutable_ref: const [i32; 3] <- arr
    // Cannot mutate through immutable_ref

    // Mutable unique array
    var mut_arr: [i32; 3] = [10, 20, 30]
    let unique_ref: unique [i32; 3] <- mut_arr
    unique_ref[0] = 100              // Mutation through unique reference binding

    // Shared mutable slice
    var shared_arr: [i32; 5] = [0; 5]
    let shared_slice1: shared [i32] <- shared_arr[..]
    let shared_slice2: shared [i32] <- shared_arr[..]  // OK: shared allows aliasing
}
```

---

## Section 3.5: Function Types 🔄 TRANSFORM

### Source
- `old_Type-System.md` lines 4898-5130 (~232 lines)
- Section 2.10: Function Types

### Strategy
Transform existing function type specification with grant terminology updates (replace "effect" with "grant"). Update examples with System 3 permissions.

### Subsections

#### 3.5.1 Function Type Syntax
- Function type notation: `(τ₁, ..., τₙ) -> τ_ret ! ε`
- Parameter types
- Return type
- Grant set (`! ε` clause)
- Function vs procedure distinction

**Grammar**:
```
FunctionType ::= '(' TypeList? ')' '->' Type GrantClause?

GrantClause ::= '!' GrantSet

GrantSet ::= Ident (',' Ident)*
           | '{' Ident (',' Ident)* '}'
```

**Example**:
```cursive
// Pure function (no grants)
type PureFunc = (i32, i32) -> i32

// Function with grants
type IOFunc = (string) -> () ! {fs::write, alloc::heap}

// Multi-parameter function
type ProcessFunc = (i32, f64, string) -> bool ! {io::write}
```

#### 3.5.2 Function Type Formation
- Well-formed function types
- Parameter type constraints
- Return type constraints
- Grant set validation
- Type checking for function types

**From old lines 4898-4990**: Complete formation rules

**Inference rule**:
```
[Type-Function]
Γ ⊢ τ₁, ..., τₙ : Type    Γ ⊢ τ_ret : Type    ε valid grant set
------------------------------------------------------------------
Γ ⊢ (τ₁, ..., τₙ) -> τ_ret ! ε : Type
```

#### 3.5.3 Function Subtyping
- Contravariance in parameters
- Covariance in return type
- Covariance in grant sets (more grants = subtype)
- Subtyping rule for function types

**From old lines 4991-5055**: Complete subtyping specification

**Key subtyping rule**:
```
[Sub-Function]
υ₁ <: τ₁    ...    υₙ <: τₙ    (contravariant parameters)
τ_ret <: υ_ret                  (covariant return)
ε₁ <: ε₂                        (covariant grants)
-------------------------------------------------------
(τ₁, ..., τₙ) -> τ_ret ! ε₁  <:  (υ₁, ..., υₙ) -> υ_ret ! ε₂
```

#### 3.5.4 Closure Types
- Closure as first-class function
- Capture semantics (move, borrow)
- Closure type inference
- Relationship to function pointers

**From old lines 5056-5130**: Complete closure specification

**Example**:
```cursive
// Closure that captures environment
let multiplier: const i32 = 10

let closure: (i32) -> i32 = |x| x * multiplier

// Closure with explicit capture
let moved_closure = move |x: i32| {
    let local_multiplier = move multiplier
    x * local_multiplier
}
```

### Formal Elements Required
- **Definition 3.5.1**: Function type
- **Definition 3.5.2**: Grant set
- **Definition 3.5.3**: Closure type
- **Algorithm 3.5.1**: Function type checking
- **Algorithm 3.5.2**: Function subtyping check
- **Inference rules**: Type-Function, Sub-Function, Type-Closure
- **Error codes**: E0371-E0390 (function type errors)

### Canonical Example
Single program demonstrating function types with grants and System 3 permissions:

```cursive
// Canonical Example 3.5: Function Types

// ===== Pure Functions =====

// Pure function type (no grants)
function add(a: i32, b: i32): i32 {
    result a + b
}

let add_fn: (i32, i32) -> i32 = add

// ===== Functions with Grants =====

// Function with file system grants
function read_config(path: string): string
    grants fs::read, alloc::heap
{
    result read_file_to_string(path)
}

// Function type includes grant set
let config_reader: (string) -> string ! {fs::read, alloc::heap} = read_config

// ===== Function Subtyping =====

// Function subtyping rule:
// (T₁) -> T₂ ! E₁  <:  (T₃) -> T₄ ! E₂  when all hold:
//   1. T₃ <: T₁  (parameters are contravariant)
//   2. T₂ <: T₄  (return types are covariant)
//   3. E₁ <: E₂  (grant requirements are covariant)

// Example subtype and supertype pair
function subtype_fn(x: i32): i32
    grants io::write
{
    println("Processing {}", x)
    result x
}

function supertype_fn(x: i16): i64
    grants io::write, fs::read
{
    println("General processing {}", x)
    result x as i64
}

// Here subtype_fn  <:  supertype_fn because:
//   • Parameter contravariance: callers of supertype_fn will only pass i16 values, and i16 <: i32.
//   • Return covariance: subtype_fn returns i32 and i32 <: i64, so callers still receive at least what they expect.
//   • Grant covariance: subtype_fn requires only {io::write}, which is a subset of {io::write, fs::read}.
// Consequently, any context expecting supertype_fn can safely receive subtype_fn.

// ===== Closures =====

procedure demonstrate_closures() {
    let multiplier: const i32 = 100

    // Closure capturing by reference
    let closure_ref = |x: i32| x * multiplier

    let result1 = closure_ref(5)    // 500

    // Closure with move semantics
    let closure_move = move |x: i32| {
        let local_mult = move multiplier
        x * local_mult
    }

    // multiplier is now moved, cannot use

    let result2 = closure_move(3)   // 300
}

// ===== Higher-Order Functions =====

// Function taking function as parameter
function apply_twice(f: (i32) -> i32, x: i32): i32 {
    let temp = f(x)
    result f(temp)
}

function double(n: i32): i32 {
    result n * 2
}

procedure test_higher_order() {
    let result = apply_twice(double, 5)  // (5 * 2) * 2 = 20
    println("Result: {}", result)
}

// ===== Grant Polymorphism (Forward Reference) =====

// NOTE: Full specification in Part IX §9.3
// Preview of grant-polymorphic function

function map<T, U, G>(
    items: [T],
    f: (T) -> U ! grants<G>
): [U]
    grants<G>, alloc::heap
    where G <: {io::write, fs::read}
{
    // Forward reference: see Part IX for grant polymorphism
    result items.map(f)
}

// ===== System 3 Permissions with Functions =====

procedure demonstrate_permissions() {
    // Function reference binding with const permission
    let fn_ref: const <- add

    // Cannot rebind fn_ref (const permission)
    // fn_ref = double;  // ERROR: cannot rebind const

    // Function with mutable state (via closure)
    var counter: const i32 = 0

    let increment_counter = || {
        counter = counter + 1
        result counter
    }

    // Closure captures mutable reference to counter
    let count1 = increment_counter()  // 1
    let count2 = increment_counter()  // 2
}
```

---

## Section 3.6: Pointer Types 🔄 SYNTHESIZE

### Source
- `old_Type-System.md` lines 3684-3919 (~234 lines)
- Section 2.5: Safe Pointers (lines 3684-3779, ~95 lines)
- Section 2.6: Raw and Unsafe Pointers (lines 3780-3919, ~139 lines)

### Strategy
Synthesize safe and unsafe pointer sections into unified pointer types specification. Emphasize distinction between safe (tracked) and raw (unsafe) pointers. Update with System 3 permissions.

### Memory Allocation and Pointer Integration

Cursive uses a three-tier memory hierarchy (Stack, Region, Heap) with pointers integrating into the region system. See Part VI: Memory Model and Permissions for complete memory allocation semantics.

**Key Points**:
- **Stack Allocation**: Default for local variables
- **Region Allocation**: Primary dynamic memory (`^` operator)
- **Heap Allocation**: Secondary mechanism, region data escapes via explicit `.to_heap()`

**Pointer Types**: Cursive's modal pointer type `Ptr<T>@State` integrates with the region system to provide compile-time verification of pointer validity and state transitions.

### Subsections

#### 3.6.1 Modal Pointers (Ptr<T>@State)

**Overview:** Cursive's `Ptr<T>@State` provides safe pointers with compile-time modal state tracking. Modal states enable null-safety, weak references for cycle breaking, and escape analysis—all with zero runtime overhead.

**Forward Reference**: Complete modal type system specified in Part XI §11.1 (Modal Types and State Machines).

**From old lines 3684-3779**: Complete modal pointer specification

---

##### 3.6.1.1 Pointer Creation and Address-Of Operator

**Address-Of Operator Semantics:**
- **Syntax**: `&expr`
- **Returns**: `Ptr<T>@Valid` directly (NOT an intermediate reference type)
- **Type declaration**: Required for pointer variables (no type inference for pointer declarations)

**Critical Distinction from Rust:**
- Cursive `&expr` produces `Ptr<T>` (a first-class modal pointer type)
- **NOT** Rust-style references `&T` or `&mut T`
- Cursive has no reference types—use `Ptr<T>` or reference bindings (`let x <- value`)

**Type Formation Rule:**
```
[Address-Of]
Γ ⊢ expr: T
expr has memory location
provenance(expr) = P
───────────────────────────────
Γ ⊢ &expr: Ptr<T>@Valid with provenance P
```

**Examples:**
```cursive
// Valid pointer creation
let value: i32 = ^42
let ptr: Ptr<i32> = &value  // Type: Ptr<i32>@Valid

// Type declaration required
let ptr2 = &value  // ❌ ERROR: Type inference not allowed for pointers
let ptr3: Ptr<i32> = &value  // ✅ OK: Type explicitly declared

// Error: Cannot take address of alias binding
alias temp = value
let bad_ptr = &temp  // ❌ ERROR E4030: alias has no memory location
```

**Normative Statements:**
- "The address-of operator SHALL produce values of type `Ptr<T>@Valid` directly."
- "Type annotations for pointer declarations SHALL be required; pointer types SHALL NOT be inferred."
- "The address-of operator SHALL only be applicable to expressions with memory locations."

**Error Codes:**
- **E4030**: Cannot take address of alias binding (alias has no memory location)

---

##### 3.6.1.2 Four Modal States

**Modal State Definition:**

Cursive's `Ptr<T>` is a modal type with four distinct states that track pointer validity at compile time:

```cursive
modal Ptr<T> {
    @Null      // Explicit null pointer
    @Valid     // Strong reference, dereferenceable
    @Weak      // Weak reference, breaks cycles
    @Expired   // Weak reference after target destroyed
}
```

**State Semantics Table:**

| State | Meaning | Dereferenceable? | Reference Strength | Use Case |
|-------|---------|------------------|-------------------|----------|
| `@Null` | Explicit null | ❌ Compile error E4028 | N/A | Nullable pointers |
| `@Valid` | Points to valid memory | ✅ Yes | Strong (prevents destruction) | Normal pointers |
| `@Weak` | Weak reference | ❌ Must upgrade first (E4032) | Weak (allows destruction) | Breaking cycles |
| `@Expired` | Target was destroyed | ❌ Target gone (E4033) | N/A | Detected dead refs |

**State Transition Diagram:**

```
Ptr::null() ──────────→ [@Null]
                           │
                           │ widen (implicit)
                           ↓
                        [Ptr<T>] ←──── unconstrained type
                           ↑
                           │ widen (implicit)
                           │
&value ────────────────→ [@Valid]
                           │
                           │ .downgrade()
                           ↓
                        [@Weak]
                           │
                           │ .upgrade()
                           ├─────→ @Valid (if target alive)
                           │
                           │ (target destroyed)
                           ↓
                       [@Expired]
```

**Type Widening and Narrowing:**

```cursive
// WIDENING (safe, implicit):
let valid: Ptr<i32>@Valid = &value
let unconstrained: Ptr<i32> = valid  // ✅ OK: widen to unconstrained Ptr<T>

// NARROWING (requires pattern matching):
let ptr: Ptr<i32> = get_ptr()
let narrow: Ptr<i32>@Valid = ptr  // ❌ ERROR: must pattern match first

// Correct narrowing:
match ptr {
    @Valid => {
        // ptr has type Ptr<i32>@Valid in this branch
        let narrow: Ptr<i32>@Valid = ptr  // ✅ OK: type refined by pattern match
        let v = *ptr  // ✅ Safe to dereference
    }
    @Null => { /* handle null case */ }
    @Weak => { /* handle weak case */ }
    @Expired => { /* handle expired case */ }
}
```

**Dereference Rules:**

```
[Deref-Valid]
Γ ⊢ ptr: Ptr<T>@Valid
─────────────────────
Γ ⊢ *ptr: T

[Deref-Null]
Γ ⊢ ptr: Ptr<T>@Null
─────────────────────
ERROR E4028: Cannot dereference null pointer

[Deref-Weak]
Γ ⊢ ptr: Ptr<T>@Weak
─────────────────────
ERROR E4032: Cannot dereference weak pointer (must upgrade first)

[Deref-Expired]
Γ ⊢ ptr: Ptr<T>@Expired
─────────────────────
ERROR E4033: Cannot dereference expired pointer
```

**Pattern Matching and State Refinement:**

```cursive
// State refinement through pattern matching
let ptr: Ptr<i32> = get_maybe_null()

match ptr {
    @Null => {
        // In this branch: ptr has type Ptr<i32>@Null
        println("Pointer is null")
    }
    @Valid => {
        // In this branch: ptr has type Ptr<i32>@Valid
        let value = *ptr  // ✅ Safe to dereference here
        println("Value: {}", value)
    }
    @Weak => {
        // In this branch: ptr has type Ptr<i32>@Weak
        // Must upgrade before use
        match ptr.upgrade() {
            @Valid => { /* now usable */ }
            @Null => { /* target expired */ }
        }
    }
    @Expired => {
        // In this branch: ptr has type Ptr<i32>@Expired
        println("Weak reference expired")
    }
}
```

**Normative Statements:**
- "Dereferencing SHALL only be permitted on pointers in the @Valid state."
- "Pattern matching SHALL refine pointer state types within match branches."
- "State transitions SHALL be tracked at compile time with zero runtime overhead."
- "The compiler SHALL reject attempts to dereference pointers not in @Valid state."

**Error Codes:**
- **E4028**: Dereference of null pointer (@Null state)
- **E4032**: Dereference of weak pointer (@Weak state) without upgrade
- **E4033**: Dereference of expired weak reference (@Expired state)

---

##### 3.6.1.3 Weak Reference Semantics

**Purpose:**

Weak references solve the fundamental problem of cyclic data structures by allowing non-owning pointers that don't prevent target destruction.

**Problem Without Weak References:**
```cursive
// ❌ MEMORY LEAK: Cyclic strong references
record Node {
    parent: Ptr<Node>,         // Strong → keeps parent alive
    children: Vec<Ptr<Node>>,  // Strong → keeps children alive
}
// Parent and children keep each other alive forever → memory leak
```

**Solution With Weak References:**
```cursive
// ✅ NO LEAK: Weak references break cycles
record Node {
    parent: Ptr<Node>@Weak,         // Weak → doesn't keep parent alive
    children: Vec<Ptr<Node>@Valid>, // Strong → parent owns children
}
// Parent can be destroyed even if children have weak refs to it
```

**Weak Reference API:**

```cursive
modal Ptr<T> {
    // Downgrade strong reference to weak
    procedure @Valid downgrade(self): Ptr<T>@Weak
    {
        // Creates weak reference from strong reference
        // Original strong reference remains valid after downgrade
    }

    // Upgrade weak reference to strong (attempt)
    procedure @Weak upgrade(self): Ptr<T>
    {
        // Returns @Valid if target still alive
        // Returns @Null if target was destroyed
        // Returns unconstrained Ptr<T>, must pattern match on result
    }

    // Check if weak reference expired
    procedure @Weak is_expired(self): bool
    {
        // Returns true if target destroyed, false if still alive
    }
}
```

**Formal Specifications:**

```
[Weak-Downgrade]
Γ ⊢ ptr: Ptr<T>@Valid
────────────────────────
Γ ⊢ ptr.downgrade(): Ptr<T>@Weak
ptr remains valid after downgrade

[Weak-Upgrade-Success]
Γ ⊢ ptr: Ptr<T>@Weak
target(ptr) is alive
────────────────────────
Γ ⊢ ptr.upgrade(): Ptr<T>@Valid (runtime check determines state)

[Weak-Upgrade-Expired]
Γ ⊢ ptr: Ptr<T>@Weak
target(ptr) was destroyed
────────────────────────
Γ ⊢ ptr.upgrade(): Ptr<T>@Null (runtime check determines state)
```

**Usage Pattern 1: Parent-Child Trees**

```cursive
record TreeNode {
    value: i32,
    parent: Ptr<TreeNode>@Weak,      // Weak: child doesn't own parent
    children: Vec<Ptr<TreeNode>>,     // Strong: parent owns children
}

procedure traverse_up(node: Ptr<TreeNode>) {
    var current = node
    loop {
        match (*current).parent.upgrade() {
            @Valid => {
                println("Parent value: {}", (*current).value)
                current = (*current).parent  // Move to parent
            }
            @Null => {
                println("Reached root (no parent)")
                break
            }
        }
    }
}

procedure build_tree(): Ptr<TreeNode>
    grants alloc::region, alloc::heap
{
    region temp {
        var root = ^TreeNode {
            value: 1,
            parent: Ptr::null(),
            children: Vec::new(),
        }

        var child = ^TreeNode {
            value: 2,
            parent: Ptr::null(),  // Set below
            children: Vec::new(),
        }

        // Set up bidirectional links
        child.parent = (&root).downgrade()  // Weak parent link (breaks cycle)
        root.children.push(&child)          // Strong child link (parent owns child)

        // Escape to heap
        result &root.to_heap()
    }
}
```

**Usage Pattern 2: Observer Pattern**

```cursive
record Subject {
    value: i32,
    observers: Vec<Ptr<Observer>@Weak>,  // Weak: observers don't prevent destruction
}

record Observer {
    id: i32,
}

procedure Subject.notify(self: unique Self)
    grants io::write
{
    // Iterate over weak observer references
    loop obs in self.observers {
        match obs.upgrade() {
            @Valid => {
                println("Notifying observer {}", (*obs).id)
                // Could call observer.on_update(self.value) here
            }
            @Null => {
                // Observer was destroyed, can remove from list
                println("Observer expired, removing from list")
            }
        }
    }
}

procedure Subject.attach(self: unique Self, obs: Ptr<Observer>) {
    // Store weak reference so observers don't keep subject alive
    self.observers.push(obs.downgrade())
}
```

**Usage Pattern 3: Cache Implementations**

```cursive
record Cache<K, V> {
    entries: Map<K, Ptr<V>@Weak>,  // Weak: cache doesn't keep values alive
}

procedure Cache.get<K, V>(self: shared Self, key: K): Option<V>
    where K: Eq + Hash
{
    match self.entries.lookup(key) {
        Some(weak_ptr) => {
            match weak_ptr.upgrade() {
                @Valid => {
                    // Cache hit: value still alive
                    result Some(copy *weak_ptr)
                }
                @Null => {
                    // Cache miss: value was evicted/destroyed
                    result None
                }
            }
        }
        None => {
            // Not in cache
            result None
        }
    }
}

procedure Cache.insert<K, V>(self: unique Self, key: K, value_ptr: Ptr<V>) {
    // Store weak reference: cache doesn't own the values
    self.entries.insert(key, value_ptr.downgrade())
}
```

**Additional Use Cases:**

4. **Graph Back-Edges**: Forward edges strong, back edges weak (prevents cycles)
5. **Doubly-Linked Lists**: Next pointers strong, previous pointers weak
6. **Event Listeners**: Subject doesn't keep listeners alive

**Normative Statements:**
- "Weak references SHALL NOT prevent the destruction of their target."
- "Upgrading a weak reference SHALL return @Null if the target has been destroyed."
- "Weak reference checking SHALL have zero runtime overhead beyond the state discriminant."
- "Downgrading a strong reference SHALL NOT invalidate the original strong reference."
- "The `.downgrade()` operation SHALL create a new weak reference without consuming the strong reference."

---

##### 3.6.1.4 Escape Analysis and Provenance Tracking

**Purpose:** Prevent dangling pointers through compile-time escape analysis with zero runtime overhead.

**Provenance System:**

Every `Ptr<T>` has compile-time provenance metadata tracking where the pointed-to data is allocated:

```
Provenance ::= Stack           // Stack-allocated data (function-local)
             | Region(RegionId) // Region-allocated data (^expr)
             | Heap             // Heap-allocated data (explicit .to_heap())
```

**Escape Rules:**

```
[Escape-Rule-Stack]
ptr: Ptr<T>
provenance(ptr) = Stack
ptr escapes function scope
──────────────────────────────
ERROR: Stack-backed pointer cannot escape function

[Escape-Rule-Region]
ptr: Ptr<T>
provenance(ptr) = Region(r)
ptr escapes region r
──────────────────────────────
ERROR E4027: Region-backed pointer cannot escape region

[Escape-Rule-Heap]
ptr: Ptr<T>
provenance(ptr) = Heap
──────────────────────────────
OK: Heap-backed pointers can escape (subject to ownership rules)
```

**Provenance Propagation:**

```
[Address-Of-Provenance]
storage(value) = provenance
────────────────────────────
provenance(&value) = provenance

[Pointer-Copy-Provenance]
provenance(ptr₁) = P
ptr₂ = ptr₁
────────────────────────────
provenance(ptr₂) = P

[Pointer-Move-Provenance]
provenance(ptr₁) = P
ptr₂ = move ptr₁
────────────────────────────
provenance(ptr₂) = P
```

**Examples:**

**❌ INVALID: Region Escape**
```cursive
procedure returns_dangling(): Ptr<i32> {
    region r {
        let value: i32 = ^42
        let ptr: Ptr<i32> = &value
        result ptr  // ❌ ERROR E4027: Cannot escape region r
    }
}
```

**✅ VALID: Heap Escape**
```cursive
procedure returns_valid(): Ptr<Data>
    grants alloc::region, alloc::heap
{
    region r {
        let data = ^Data::new()
        let heap_data = data.to_heap()  // Explicit heap escape
        result &heap_data               // ✅ OK: heap-backed pointer
    }
}
```

**✅ VALID: Local Use Only**
```cursive
procedure process() {
    let value: i32 = 42
    let ptr: Ptr<i32> = &value  // Stack-backed pointer
    println("{}", *ptr)         // ✅ OK: used locally within scope
    // ptr destroyed at scope end, no escape
}
```

**❌ INVALID: Stack Escape**
```cursive
procedure bad_stack_escape(): Ptr<i32> {
    let value: i32 = 42
    result &value  // ❌ ERROR: Stack pointer cannot escape function
}
```

**Escape Check Algorithm:**

```
ALGORITHM: Check-Pointer-Escape(ptr, target_scope)
INPUT: ptr: Ptr<T>, target_scope: Scope
OUTPUT: OK | ERROR

1. provenance ← get_provenance(ptr)
2. MATCH provenance:
   CASE Stack:
     IF target_scope is outside current_function:
       RETURN ERROR("Stack pointer cannot escape function")
     ELSE:
       RETURN OK
   CASE Region(r):
     IF target_scope is outside region r:
       RETURN ERROR("E4027: Region pointer cannot escape region")
     ELSE:
       RETURN OK
   CASE Heap:
     // Heap pointers can escape (ownership rules still apply)
     RETURN OK
3. RETURN OK
```

**Diagnostic Requirements:**

Error E4027 SHALL include:
1. Location where pointer is created
2. Region or scope where data is allocated
3. Location where escape is attempted
4. Suggestion to use `.to_heap()` for legitimate heap escape

**Example Diagnostic:**
```
error[E4027]: cannot escape region-backed pointer
  --> example.cursive:5:12
   |
3  |     region r {
   |            - region 'r' declared here
4  |         let data = ^Data::new()
   |                    ------------ allocated in region 'r'
5  |         result &data
   |                ^^^^^ cannot return pointer to region-allocated data
   |
   = note: region-allocated data is destroyed when region ends (line 6)
   = note: this pointer would become dangling after region destruction
   = help: if you need to return this data, explicitly escape to heap:

           let heap_data = data.to_heap()
           result &heap_data

   = help: alternatively, pass the data by value:

           result data  // Moves data out of region (if type is movable)
```

**Provenance Tracking Implementation:**

The compiler SHALL track provenance through:
1. **Variable bindings**: Each binding records its storage location
2. **Address-of operations**: Inherit provenance from target expression
3. **Pointer copies**: Propagate provenance metadata
4. **Function returns**: Check escape constraints
5. **Control flow joins**: Unify provenance (conservative analysis)

**Normative Statements:**
- "Pointer provenance SHALL be tracked at compile time with zero runtime cost."
- "Pointers to stack-allocated data SHALL NOT escape function scope."
- "Pointers to region-allocated data SHALL NOT escape their allocating region."
- "Escape analysis SHALL prevent all dangling pointer formation at compile time."
- "Explicit heap escape via `.to_heap()` SHALL be the only mechanism for region data to outlive its region."
- "All escape violations SHALL be detected during compilation and SHALL prevent program compilation."

**Error Codes:**
- **E4027**: Region-backed pointer escape attempt (includes region-to-outer-scope and region-to-heap without .to_heap())

#### 3.6.2 Raw Pointers
- **Raw pointer types**: `*const T`, `*mut T`
- No safety guarantees
- Explicit unsafe operations
- Pointer arithmetic
- Dereferencing requires `unsafe`
- FFI use cases
- Relationship to safe pointers

**From old lines 3780-3919**: Complete raw pointer specification

**Example**:
```cursive
// Raw pointers (unsafe)
let value: const i32 = 42
let ptr: *const i32 = &value as *const i32

// Dereferencing requires unsafe block
unsafe {
    let deref_value = *ptr
    println("Dereferenced: {}", deref_value)
}

// Mutable raw pointer
var mutable: unique i32 = 100
let mut_ptr: *mut i32 = (&mutable) as *mut i32

unsafe {
    *mut_ptr = 200
}
```

**Normative statement**:
- "Dereferencing a raw pointer shall require an `unsafe` block."
- "The validity of raw pointer operations is the responsibility of the programmer."

### Formal Elements Required

**Definitions:**
- **Definition 3.6.1.1**: Address-of operator semantics
- **Definition 3.6.1.2**: Modal state (four states: @Null, @Valid, @Weak, @Expired)
- **Definition 3.6.1.3**: Weak reference semantics
- **Definition 3.6.1.4**: Provenance (Stack, Region, Heap)
- **Definition 3.6.2**: Raw pointer types (*const T, *mut T)

**Algorithms:**
- **Algorithm 3.6.1**: State transition for modal pointers
- **Algorithm 3.6.2**: Weak reference upgrade
- **Algorithm 3.6.3**: Escape check (Check-Pointer-Escape)
- **Algorithm 3.6.4**: Provenance propagation
- **Algorithm 3.6.5**: Pointer conversion (safe to raw)

**Inference Rules:**
- **Type Formation**: Type-Ptr, Type-RawPtr, Type-ModalState
- **Address-Of**: [Address-Of], [Address-Of-Provenance]
- **Dereference**: [Deref-Valid], [Deref-Null], [Deref-Weak], [Deref-Expired]
- **Weak References**: [Weak-Downgrade], [Weak-Upgrade-Success], [Weak-Upgrade-Expired]
- **Escape Analysis**: [Escape-Rule-Stack], [Escape-Rule-Region], [Escape-Rule-Heap]
- **Provenance**: [Pointer-Copy-Provenance], [Pointer-Move-Provenance]

**Error Codes:**
- **E4027**: Region-backed pointer escape attempt
- **E4028**: Dereference of null pointer (@Null state)
- **E4030**: Cannot take address of alias binding
- **E4032**: Dereference of weak pointer (@Weak state)
- **E4033**: Dereference of expired pointer (@Expired state)
- **E0391-E0410**: Additional pointer-related errors (to be specified in full section)

### Canonical Example
Single program demonstrating allocation, pointers, and raw pointers with System 3 permissions:

```cursive
// Canonical Example 3.6: Pointer Types

record Point { x: f64, y: f64 }

// ===== Stack Allocation (Default) =====

procedure demonstrate_stack() {
    // Stack allocation - default, no marker needed
    let value: const i32 = 42
    let point = Point { x: 1.0, y: 2.0 }

    println("Stack value: {}", value)
    println("Stack point: ({}, {})", point.x, point.y)

    // Automatic deallocation when scope ends
}

// ===== Region Allocation (Primary Dynamic Memory) =====

procedure demonstrate_regions()
    grants alloc::region
{
    region r {
        // Region allocation with ^ operator
        let data: const i32 = ^42
        println("Region value: {}", data)

        // Composite types in regions
        let point = ^Point { x: 3.0, y: 4.0 }
        println("Region point: ({}, {})", point.x, point.y)

        // Nested regions with stacking
        region inner {
            let a = ^100        // Allocates in 'inner'
            let b = ^^200       // Allocates in 'r' (parent)
            let c = ^^^300      // Allocates in grandparent region (two levels up)
        }
        // 'inner' region deallocated here (RAII)
    }
    // 'r' region deallocated here (RAII)
}

// ===== Heap Escape (Secondary - Rare) =====

procedure demonstrate_heap_escape()
    grants alloc::region, alloc::heap
{
    region r {
        let regional_data = ^42

        // Explicit heap escape for cross-region data
        let heap_data = regional_data.to_heap()

        // heap_data survives beyond the region scope
    }

    // heap_data still valid here
    // Cleaned up via RAII when owner drops (no GC in Cursive)
}

// ===== Safe Modal Pointers (Ptr<T>@State) =====

// Demonstrates 3.6.1.1: Address-Of Operator
procedure demonstrate_address_of()
    grants alloc::region
{
    region r {
        // Valid pointer creation with explicit type declaration
        let value: i32 = ^42
        let ptr: Ptr<i32> = &value  // ✅ Type: Ptr<i32>@Valid

        // Type declaration required (no inference for pointers)
        // let bad = &value  // ❌ ERROR: Type inference not allowed

        // Cannot take address of alias binding
        alias temp = value
        // let bad_ptr = &temp  // ❌ ERROR E4030: alias has no memory location

        println("Address-of demonstration complete")
    }
}

// Demonstrates 3.6.1.2: Four Modal States
procedure demonstrate_modal_states()
    grants alloc::region
{
    region r {
        // @Null state
        let null_ptr: Ptr<i32> = Ptr::null()

        // @Valid state
        let value: i32 = ^100
        let valid_ptr: Ptr<i32> = &value  // Ptr<i32>@Valid

        // Pattern matching for state refinement
        let ptr: Ptr<i32> = get_maybe_null()
        match ptr {
            @Null => {
                println("Pointer is null")
                // Cannot dereference here: *ptr would be E4028
            }
            @Valid => {
                // Type refined to Ptr<i32>@Valid in this branch
                let value = *ptr  // ✅ Safe to dereference
                println("Value: {}", value)
            }
        }

        // Widening (safe, implicit)
        let specific: Ptr<i32>@Valid = &value
        let general: Ptr<i32> = specific  // ✅ OK: widen to unconstrained

        // Narrowing requires pattern matching (shown above)
    }
}

// Demonstrates 3.6.1.3: Weak Reference Semantics
procedure demonstrate_weak_references()
    grants alloc::region
{
    region r {
        let value: i32 = ^42
        let strong: Ptr<i32> = &value  // Ptr<i32>@Valid

        // Create weak reference (downgrade)
        let weak: Ptr<i32>@Weak = strong.downgrade()

        // Strong reference still valid after downgrade
        println("Strong: {}", *strong)

        // Weak reference must be upgraded before use
        match weak.upgrade() {
            @Valid => {
                // Successfully upgraded
                println("Upgraded weak: {}", *weak)
            }
            @Null => {
                // Target was destroyed
                println("Weak reference expired")
            }
        }

        // Check if expired without upgrading
        if weak.is_expired() {
            println("Weak reference has expired")
        }
    }

    // Example: Breaking cycles with weak references
    record Node {
        parent: Ptr<Node>@Weak,      // Weak: doesn't own parent
        children: Vec<Ptr<Node>>,     // Strong: owns children
    }
}

// Demonstrates 3.6.1.4: Escape Analysis
procedure demonstrate_escape_analysis()
    grants alloc::region, alloc::heap
{
    // ❌ This would fail escape analysis (commented out)
    // procedure bad_region_escape(): Ptr<i32> {
    //     region r {
    //         let value: i32 = ^42
    //         result &value  // ERROR E4027: Cannot escape region
    //     }
    // }

    // ❌ This would fail escape analysis (commented out)
    // procedure bad_stack_escape(): Ptr<i32> {
    //     let value: i32 = 42
    //     result &value  // ERROR: Stack pointer cannot escape function
    // }

    // ✅ Valid: Heap escape
    procedure good_heap_escape(): Ptr<i32>
        grants alloc::region, alloc::heap
    {
        region r {
            let value: i32 = ^42
            let heap_value = value.to_heap()  // Explicit heap escape
            result &heap_value  // ✅ OK: heap-backed pointer
        }
    }

    // ✅ Valid: Local use only (no escape)
    region r {
        let value: i32 = ^100
        let ptr: Ptr<i32> = &value  // Region-backed pointer
        println("{}", *ptr)         // ✅ OK: used locally
        // ptr destroyed when region ends, no escape
    }

    // Demonstrate provenance tracking
    region outer {
        let outer_val: i32 = ^10

        region inner {
            let inner_val: i32 = ^20
            let inner_ptr: Ptr<i32> = &inner_val  // Provenance: Region(inner)

            // Can use within inner region
            println("Inner: {}", *inner_ptr)

            // Cannot escape to outer region
            // let escaped = inner_ptr;  // ERROR E4027 if returned or stored in outer
        }

        // inner_ptr no longer accessible here
    }
}

// ===== Raw Pointers (Unsafe) =====

procedure demonstrate_raw_pointers()
    grants unsafe::ptr
{
    // Creating raw pointers (safe operation)
    let value: const i32 = 42
    let const_ptr: *const i32 = &value as *const i32

    var mutable: unique i32 = 100
    let mut_ptr: *mut i32 = (&mutable) as *mut i32

    // Dereferencing raw pointer (unsafe operation)
    unsafe {
        let deref_value = *const_ptr
        println("Dereferenced const ptr: {}", deref_value)

        *mut_ptr = 200
        println("Modified through mut ptr: {}", *mut_ptr)
    }

    // Pointer arithmetic (unsafe)
    let array: [i32; 5] = [10, 20, 30, 40, 50]
    let array_ptr: *const i32 = &array[0] as *const i32

    unsafe {
        // Access array elements via pointer arithmetic
        loop i in 0..5 {
            let element_ptr = array_ptr.offset(i as isize)
            let element = *element_ptr
            println("array[{}] = {}", i, element)
        }
    }
}

// ===== FFI Use Case =====

// External C function declaration
// Note: Grant clause placement in extern blocks - verify against Part IX §9.1
extern "C" {
    function malloc(size: usize): *mut u8
        grants unsafe::ptr, ffi::call;

    function free(ptr: *mut u8)
        grants unsafe::ptr, ffi::call;
}

procedure demonstrate_ffi()
    grants unsafe::ptr, ffi::call
{
    unsafe {
        // Allocate memory via C malloc
        let size: usize = 1024
        let ptr = malloc(size)

        if ptr == null {
            panic("Allocation failed")
        }

        // Use the memory
        *ptr = 65  // ASCII 'A'

        // Deallocate
        free(ptr)
    }
}

// ===== System 3 Permissions with Allocations =====

procedure demonstrate_permissions()
    grants alloc::region
{
    region r {
        // Const permission (immutable binding, immutable data)
        let immutable: const i32 = ^42
        let const_ref: const i32 <- immutable
        // Cannot mutate through const_ref

        // Unique permission (mutable, exclusive access)
        var mutable: unique i32 = ^100
        let unique_ref: unique i32 <- mutable
        *unique_ref = 200  // Mutation through unique reference binding

        // Shared permission (shared read-only access)
        let shared_data: shared i32 = ^300
        let shared_ref: shared i32 <- shared_data
        // Multiple shared reference bindings allowed
    }

    // Raw pointers bypass permission system
    let value: const i32 = 42
    let raw: *const i32 = &value as *const i32
    // Safety is programmer responsibility in unsafe blocks
}
```

**Enhancement Note**: Section 3.6.1 has been expanded to include four detailed subsubsections:
- 3.6.1.1: Pointer Creation and Address-Of Operator (type formation rules, E4030)
- 3.6.1.2: Four Modal States (state semantics, transitions, pattern matching, E4028/E4032/E4033)
- 3.6.1.3: Weak Reference Semantics (API, usage patterns, formal specifications)
- 3.6.1.4: Escape Analysis and Provenance Tracking (escape rules, algorithm, E4027)

This enhancement brings Section 3.6 to the same level of formal specification detail as other sections, with complete inference rules, algorithms, and implementer-ready specifications aligned with POINTER_SYSTEM_DESIGN.md.

---

*The pattern continues with the same level of detail for Predicates, Generics, Type Bounds, Type Aliases, and Type Introspection.*

---

## Section 3.7: Predicates 🔄 EXPAND

### Source
- `old_Type-System.md` lines 3920-4086 (~165 lines)
- Section 2.7: Predicates and Code Reuse (lines 3920-4061, ~141 lines)
- Section 2.8: Marker Predicates and Utilities (lines 4062-4086, ~24 lines)

### Strategy
**EXPAND** unified predicate section combining concrete code reuse (old 2.7) and marker predicates (old 2.8). This is a critical section requiring:
1. Global terminology change: "predicate" -> "predicate"
2. Expansion of marker predicates from 24 lines to full subsection
3. Unified presentation of predicate concept
4. Clear distinction from contracts (abstract vs concrete)

### Subsections

#### 3.7.1 Predicate Concept and Purpose
- **Definition**: Predicates are concrete code reuse mechanisms
- **Critical distinction**: ALL procedures MUST have bodies
- **Contrast with contracts**: Contracts have NO bodies (abstract interfaces)
- Purpose: Code sharing without inheritance
- Static dispatch through monomorphization
- Relationship to type system

**NEW CONTENT** (synthesize from old + conversation decisions):

**Fundamental Rule**:
> **Predicates provide concrete implementations. Contracts specify abstract requirements.**

**Comparison Table**:
| Aspect | Predicates | Contracts |
|--------|-----------|-----------|
| Purpose | Code reuse | Interface specification |
| Procedure bodies | ALL MUST have bodies | NO bodies allowed |
| Implementation | Concrete | Abstract |
| Dispatch | Static (monomorphization) | Static or dynamic (witnesses) |
| Use case | Default implementations | Behavioral requirements |

**Example**:
```cursive
// PREDICATE: Concrete code reuse
predicate DefaultRenderable {
    procedure render(self: $) {
        println("Rendering default")  // MUST have body
    }

    procedure render_twice(self: $) {
        self.render()
        self.render()                 // MUST have body
    }
}

// CONTRACT: Abstract interface
contract Drawable {
    procedure draw(self)
        grants io::write
        must valid(self)
        // NO body - pure specification
}

// Using both
record Shape: Drawable, DefaultRenderable {
    kind: ShapeKind

    // MUST implement contract (Drawable)
    procedure draw(self) grants io::write {
        draw_shape(self.kind)
    }

    // Inherits predicate implementations (DefaultRenderable)
    // render() and render_twice() available automatically
}
```

**Normative statements**:
- "All procedures declared in a predicate shall have concrete implementations."
- "A predicate provides code reuse; it does not specify abstract requirements."
- "Types may include multiple predicates to compose implementations."

#### 3.7.1.1 Predicate Categories

Cursive distinguishes two categories of predicates based on implementation origin:

**1. User-Defined Predicates**
- Declared by users with the `predicate` keyword
- All procedures MUST have concrete bodies provided by user code
- Users control procedure implementations and behavior
- Example: `predicate Drawable { procedure draw(self) { /* user code */ } }`
- Implementation: User-written procedure bodies executed at runtime

**2. System-Defined Predicates (Marker Predicates)**
- Provided by the Cursive compiler as language primitives
- Examples: `Copy`, `Send`, `Sync`, `Sized`
- Cannot be user-declared, overridden, or extended
- Implementation: Compiler-intrinsic semantics (not user code)
- Auto-implementation: Compiler automatically determines which types satisfy marker predicates through structural analysis
- Zero user-definable procedures: Users cannot add custom methods to marker predicates
- Zero user-definable associated types: Marker predicates have no user-customizable associated types

**Comparison**:
| Aspect | User-Defined | System-Defined (Marker) |
|--------|-------------|-------------------------|
| Declaration | Users declare with `predicate` keyword | Provided by compiler |
| Procedure bodies | User-written code | Compiler-intrinsic implementation |
| Customization | Full user control | No customization allowed |
| Purpose | Code reuse, shared behavior | Compile-time type properties |
| Examples | `Drawable`, `Serializable` | `Copy`, `Send`, `Sync`, `Sized` |

**Auto-Implementation Distinction**:
- **User-Defined**: Types declare predicate satisfaction in type definition (e.g., `record Shape: Drawable { }`)
- **Marker Predicates**: Compiler automatically derives satisfaction based on structural rules

**Normative statements**:
- "Marker predicates shall be implemented by the compiler with intrinsic semantics."
- "Users shall not declare predicates with reserved names: `Copy`, `Send`, `Sync`, or `Sized`."
- "Users shall not provide procedure implementations for marker predicates."
- "All marker predicates shall have zero user-definable procedures and zero user-definable associated types."

**Important Note on "All Procedures Must Have Bodies"**:
The requirement that "all procedures declared in a predicate shall have concrete implementations" (§3.7.1) applies to BOTH categories:
- **User-Defined Predicates**: Bodies provided by user code
- **Marker Predicates**: Bodies provided by compiler-intrinsic implementations

Marker predicates satisfy the "must have bodies" requirement through compiler-provided semantics. The "zero-procedure" characteristic refers to zero USER-definable procedures, not zero implementations. The compiler provides the complete implementation for Copy (bitwise copy), Send (thread-transfer safety), Sync (shared-access safety), and Sized (compile-time size determination).

**Forward Reference**: Complete predicate implementation mechanism specified in Part VIII §8.2.

---

#### 3.7.2 Marker Predicates (Copy, Send, Sync, Sized)
**EXPAND** from old 24 lines to comprehensive specification.

- **Marker predicate definition**: System-defined predicates with compiler-intrinsic implementations
  - Zero user-definable procedures (users cannot add custom methods)
  - Zero user-definable associated types (users cannot add associated types)
  - Implementation provided by compiler, not user code
  - Types automatically satisfy marker predicates via structural analysis
  - Cannot be declared, overridden, or extended by users
- **Purpose**: Compile-time reasoning about type properties through compiler-enforced semantics
- **Four core marker predicates**:
  1. **Copy**: Bitwise-copyable types
  2. **Send**: Types safe to transfer across threads
  3. **Sync**: Types safe to share across threads via shared references
  4. **Sized**: Types with statically-known size

**From old lines 4062-4086**: Core definitions

**Expanded specification**:

##### 3.7.2.1 Copy Predicate
**Definition**: A type `T` is `Copy` if values of type `T` can be safely duplicated via bitwise copy.

**Auto-implementation rule**:
```
[Auto-Copy]
T is primitive scalar    OR
(T = (T₁, ..., Tₙ) AND ∀i. Tᵢ : Copy)    OR
(T = record { f₁: T₁, ..., fₙ: Tₙ } AND ∀i. Tᵢ : Copy)
----------------------------------------------------------
T : Copy
```

**Example**:
```cursive
// Automatically Copy (all fields are Copy)
record Point {
    x: i32,  // i32 : Copy
    y: i32   // i32 : Copy
}
// Therefore: Point : Copy

let p1 = Point { x: 1, y: 2 }
let p2 = copy p1  // OK: Point : Copy

// NOT Copy (contains field with unique permission, which is not Copy)
record Container {
    data: unique i32
}
// Therefore: Container ∉ Copy (unique permission fields are not Copy)
```

**Normative statement**:
- "A type shall be Copy if and only if bitwise duplication preserves semantic equivalence."

##### 3.7.2.2 Send Predicate
**Definition**: A type `T` is `Send` if ownership of values of type `T` can be safely transferred across thread boundaries.

**Auto-implementation rule**:
```
[Auto-Send]
T is primitive    OR
(T is composite AND ∀ field types Tᵢ. Tᵢ : Send)
------------------------------------------------
T : Send
```

**Examples**:
```cursive
// Automatically Send
record Data {
    value: i32,       // i32 : Send
    buffer: [u8; 100] // [u8; N] : Send
}
// Therefore: Data : Send

// NOT Send (contains raw pointer)
record UnsafeData {
    ptr: *mut i32     // *mut T ∉ Send
}
// Therefore: UnsafeData ∉ Send
```

**Usage**:
```cursive
function spawn_worker<T: Send>(data: T)
    grants thread::spawn
{
    thread_spawn(move || {
        process(data)  // OK: T : Send
    })
}
```

**Normative statement**:
- "A type shall be Send if transferring ownership across threads preserves memory safety."

##### 3.7.2.3 Sync Predicate
**Definition**: A type `T` is `Sync` if shared immutable references to values of type `T` can be safely shared across threads.

**Auto-implementation rule**:
```
[Auto-Sync]
T is primitive with no interior mutability    OR
(T is composite AND ∀ field types Tᵢ. Tᵢ : Sync)
--------------------------------------------------
T : Sync
```

**Relationship to Send**:
> If `T : Sync`, then `&T : Send` (immutable references can be sent across threads)

**Examples**:
```cursive
// Automatically Sync
record ImmutableData {
    value: i32,
    label: string
}
// Therefore: ImmutableData : Sync

// NOT Sync (contains interior mutability)
// Note: Hypothetical example for illustration. Cursive's interior mutability
// mechanisms are specified in Part XIV.
record Cell<T> {
    value: InnerCell<T>  // Hypothetical interior mutability type
}
// Therefore: Cell<T> ∉ Sync (even if T : Sync)

// Sync through thread-safe primitives
record Counter {
    count: i32      // Assume atomic primitives : Sync
}
// Therefore: Counter : Sync
```

**Normative statement**:
- "A type shall be Sync if shared immutable access from multiple threads preserves memory safety."

##### 3.7.2.4 Sized Predicate
**Definition**: A type `T` is `Sized` if the size of `T` is known at compile time.

**Auto-implementation**:
> **All types are `Sized` by default.** Types can opt out with `?Sized` bound.

**Not Sized**:
- Slices: `[T]`
- String slices: `string`
- Predicate objects (Part VIII)

**Example**:
```cursive
// Sized types (default)
let x: const i32 = 42                // i32 : Sized
let p: Point = Point { x: 1.0, y: 2.0 }  // Point : Sized

// NOT Sized (dynamically-sized)
let slice: [i32] = array[..]         // [i32] ∉ Sized (?Sized)
let string_slice: string = "hello"   // string ∉ Sized (?Sized)

// Generic function requiring Sized (implicit)
function process<T>(value: T) {
    // T : Sized (implicit bound)
}

// Generic function allowing ?Sized
function process_unsized<T: ?Sized>(value: T) {
    // T may or may not be Sized (unsized types passed by reference implicitly)
}
```

**Normative statement**:
- "A type shall be Sized if its size is determinable at compile time."
- "All type parameters have an implicit `Sized` bound unless explicitly marked `?Sized`."

#### 3.7.3 Predicate Bounds and Constraints
- Using predicates in `where` clauses
- Predicate bounds on type parameters
- Multiple predicate bounds
- Predicate bound composition
- Relationship to type checking

**Preview** (full specification in §3.9):

**Example**:
```cursive
// Predicate bound on type parameter
function clone_value<T: Copy>(value: T): T {
    result copy value
}

// Multiple predicate bounds
function transfer<T>(value: T)
    where T: Send + Sync
    grants thread::spawn
{
    thread_spawn(move || process(value))
}

// Using marker predicates
function serialize<T>(value: T): [u8]
    where T: Copy + Sized
    grants alloc::heap
{
    // Can safely copy and know size
    result to_bytes(copy value)
}
```

**Forward reference**:
> "Complete specification of predicate bounds is provided in §3.9 Type Bounds and Constraints."

#### 3.7.4 Standard Library Predicates (Preview)
- Common predicates in standard library
- Predicate categories (comparison, formatting, conversion)
- Forward reference to Part VIII for full predicate system

**Preview of standard predicates**:
- **Eq**: Equality comparison
- **Ord**: Total ordering
- **Hash**: Hashing support
- **Default**: Default value construction
- **Iterator**: Iteration protocol

**Example**:
```cursive
// Generic predicate usage
predicate Comparable {
    procedure compare(self: $, other: $): i32 {
        // Default comparison implementation
        // Returns: -1 (less), 0 (equal), 1 (greater)
        result builtin_compare(self, other)
    }
}

record Point: Comparable {
    x: i32,
    y: i32
}

// Can now use Comparable functionality
let p1 = Point { x: 1, y: 2 }
let p2 = Point { x: 3, y: 4 }
let cmp_result = p1.compare(p2)
```

**Forward reference**:
> "Complete predicate system including definition, implementation, dispatch, and coherence is specified in Part VIII: Advanced Type Features."

### Formal Elements Required
- **Definition 3.7.1**: Predicate (concrete code reuse mechanism)
- **Definition 3.7.2**: Marker predicate
- **Definition 3.7.3**: Copy predicate
- **Definition 3.7.4**: Send predicate
- **Definition 3.7.5**: Sync predicate
- **Definition 3.7.6**: Sized predicate
- **Algorithm 3.7.1**: Copy auto-derivation
- **Algorithm 3.7.2**: Send auto-derivation
- **Algorithm 3.7.3**: Sync auto-derivation
- **Inference rules**: Predicate-Impl, Auto-Copy, Auto-Send, Auto-Sync, Auto-Sized
- **Error codes**: E0411-E0440 (predicate errors)

### Canonical Example
Single comprehensive program demonstrating predicates vs contracts, marker predicates, and bounds:

```cursive
// Canonical Example 3.7: Predicates

// ===== Predicate: Concrete Code Reuse =====

predicate Loggable {
    procedure log_info(self: $, message: string)
        grants io::write
    {
        println("[INFO] {}: {}", self.type_name(), message)
    }

    procedure log_error(self: $, message: string)
        grants io::write
    {
        eprintln("[ERROR] {}: {}", self.type_name(), message)
    }
}

// ===== Contract: Abstract Interface =====

contract Processable {
    procedure process(self)
        grants io::write, alloc::heap
        must valid(self)
        // NO body - abstract specification
}

// ===== Type Using Both =====

record Task: Processable, Loggable {
    id: i32,
    description: string
}

// MUST implement contract (Processable)
implement Processable for Task {
    procedure process(self)
        grants io::write, alloc::heap
    {
        println("Processing task {}", self.id)
    }
}

// Automatically has predicate implementations (Loggable)
procedure use_task() {
    let task = Task { id: 1, description: "Example" }

    task.log_info("Starting")  // From Loggable predicate
    task.process()              // From Processable contract
    task.log_error("Failed")    // From Loggable predicate
}

// ===== Marker Predicates =====

// Copy: Automatically derived
record Point {
    x: i32,  // i32 : Copy
    y: i32   // i32 : Copy
}
// Compiler: Point : Copy ✓

procedure demonstrate_copy() {
    let p1 = Point { x: 1, y: 2 }
    let p2 = copy p1  // OK: Point : Copy

    // Both p1 and p2 are valid
    println("p1: ({}, {})", p1.x, p1.y)
    println("p2: ({}, {})", p2.x, p2.y)
}

// Send: Thread-safe transfer
record ThreadSafeData {
    value: i32,
    buffer: [u8; 1024]
}
// Compiler: ThreadSafeData : Send ✓

function spawn_processor<T: Send>(data: T)
    grants thread::spawn
{
    thread_spawn(move || {
        process_data(data)  // OK: T : Send
    })
}

// Sync: Thread-safe sharing
record SharedState {
    counter: i32  // Assume thread-safe primitive : Sync
}
// Compiler: SharedState : Sync ✓

procedure demonstrate_sync() {
    var state: SharedState = SharedState { counter: 0 }

    let shared1 <- state
    let shared2 <- state  // OK: SharedState : Sync

    // Both threads can safely access shared references
}

// Sized: Compile-time known size
function stack_allocate<T: Sized>(value: T): T {
    // Can allocate on stack because size known
    result value
}

// ?Sized: Allow dynamically-sized types
function reference_unsized<T: ?Sized>(value: T) {
    // T might be dynamically-sized (e.g., [i32] or string, passed by reference implicitly)
}

// ===== Predicate Bounds in Generics =====

// Single predicate bound
function clone_if_copy<T: Copy>(value: T): T {
    result copy value
}

// Multiple predicate bounds
function safe_transfer<T>(value: T)
    where T: Send + Sync + Sized
    grants thread::spawn
{
    thread_spawn(move || {
        // Safe because T : Send + Sync
        process(value)
    })
}

// Marker predicate composition
function serialize<T>(value: T): [u8]
    where T: Copy + Sized
    grants alloc::heap
{
    // Can safely copy and know size at compile time
    let copied = copy value
    result convert_to_bytes(copied)
}

// ===== Generic Predicate Example =====

predicate Printable {
    procedure print(self: $) {
        // Default implementation using compiler intrinsics
        print_value(self)
    }
}

record CustomType: Printable {
    data: i32
}

procedure use_debug() {
    let obj = CustomType { data: 42 }
    println("{:?}", obj)  // Uses debug_fmt from Debug predicate
}

// ===== Distinction from Contracts =====

procedure demonstrate_distinction() {
    // Predicates: Get implementation automatically
    record Widget: Loggable {
        id: i32
    }

    let widget = Widget { id: 1 }
    widget.log_info("Ready")  // Implementation from Loggable predicate

    // Contracts: MUST provide implementation
    record Processor: Processable {
        name: string
    }

    implement Processable for Processor {
        procedure process(self) grants io::write, alloc::heap {
            println("Processing {}", self.name)
        }
    }
}
```

---

## Section 3.8: Type Constructors and Generics 🔄 TRANSFORM

### Source
- `old_Type-System.md` lines 4087-4897 (~810 lines)
- Section 2.9: Parametric Features

### Strategy
Transform the original generics material with predicate terminology and System 3 permissions. Highlight explicit generic parameter binding, const generics, and associated types in predicates so that later sections (bounds, introspection) have a consistent foundation.

### Subsections

#### 3.8.1 Type Parameters
- Syntax for declaring type parameters (`<T, U>`)
- Implicit `Sized` bound on unconstrained parameters
- Parameter variance annotations (forward reference to §3.1)

#### 3.8.2 Generic Types
- Instantiation of generic records/enums/predicates
- Type argument inference versus explicit annotation
- Interaction with System 3 permissions in fields

#### 3.8.3 Generic Functions
- Function-level type parameters and predicate bounds
- Grant-polymorphic parameters in signatures (`grants<G>`)
- Monomorphization rules (forward reference to Part II §2.0.6)

#### 3.8.4 Const Generics
- Const parameter syntax (`Buffer<const N: usize>`)
- Admissible const parameter types (integers, booleans, enum values)
- Compile-time evaluation guarantees and diagnostic rules

#### 3.8.5 Associated Types
- Associated types declared in predicates
- Projection syntax (`Sequence::Item<T>`)
- Coherence requirements (forward reference to Part VIII)

### Formal Elements Required
- **Definition 3.8.1**: Type parameter scope and binding
- **Definition 3.8.2**: Generic instantiation
- **Definition 3.8.3**: Const generic parameter
- **Definition 3.8.4**: Associated type projection
- **Algorithm 3.8.1**: Type argument inference
- **Algorithm 3.8.2**: Const generic evaluation
- **Inference rules**: Generic Function Formation, Generic Type Formation, Associated Type Well-Formedness

### Canonical Example
```cursive
// Canonical Example 3.8: Generics

predicate Sequence<T> {
    associated type Item

    procedure push(self: shared $, value: Item)
        grants alloc::heap
    {
        self::extend_with(value)
    }
}

record RingBuffer<T, const N: usize>
    where N > 0
{
    data: [T; N],
    head: usize,
    tail: usize
}

function map_buffer<T, U, G, const N: usize>(
    source: RingBuffer<T, N>,
    f: (T) -> U ! grants<G>
): RingBuffer<U, N>
    grants<G>
    where G <: {io::write, fs::read}
{
    var result: unique RingBuffer<U, N> = RingBuffer {
        data: default(),
        head: 0,
        tail: 0
    }

    loop index in 0..N {
        let item: const T = source.data[index]
        result.data[index] = f(item)
    }

    result result
}

predicate Iterable<T> {
    associated type Item

    procedure iter(self: const $, body: (Item) -> ())
        grants io::write
    {
        self::for_each(body)
    }
}

implement<T, const N: usize> Iterable<RingBuffer<T, N>> {
    associated type Item = T

    procedure iter(self: const RingBuffer<T, N>, body: (T) -> ())
        grants io::write
    {
        loop index in 0..N {
            body(self.data[index])
        }
    }
}
```

---

## Section 3.9: Type Bounds and Constraints 🔄 SYNTHESIZE + ⭐ NEW

### Source
- `old_Type-System.md` scattered content from sections 2.0 and 2.9
- NEW: Grant bounds (forward reference to Part IX)

### Strategy
Unify all bound forms—predicate, lifetime, grant, and syntactic `where` clauses—into a single reference section. Emphasize how bounds interact with generics, System 3 permissions, and the grant system so later chapters (predicates, introspection) can assume a single canonical description.

### Subsections

#### 3.9.1 Predicate Bounds
- Syntax `T: Predicate` and chained bounds `T: Predicate + Iterable`
- Explicit versus implicit bounds (e.g., `Sized`)
- Bound satisfaction via predicate implementation lookup

#### 3.9.2 Region Bounds
- Region escape constraints (forward reference to Part VI §6.4)
- Modal state transition bounds: `State ≥ Transition`
- Lexical permission checking (no explicit lifetime annotations in Cursive)

#### 3.9.3 Grant Bounds ⭐ NEW
- Grant-parameter subset constraints `where G <: {io::write, fs::read}`
- Composition of multiple grant parameters (`grants<G₁>, grants<G₂>`)
- Interaction with built-in grants from Part IX §9.4

#### 3.9.4 Where Clauses
- Ordering and readability guidelines for complex bounds
- Scoping rules for parameters introduced in `where` clauses
- Diagnostic requirements for unsatisfied bounds

### Formal Elements Required
- **Definition 3.9.1**: Predicate bound satisfaction
- **Definition 3.9.2**: Region bound (escape constraints)
- **Definition 3.9.3**: Grant bound (`<:` relation)
- **Algorithm 3.9.1**: Bound evaluation order
- **Algorithm 3.9.2**: Grant subset checking (using `grants<G>`)
- **Inference rules**: Bound Introduction, Bound Satisfaction, Grant Bound Satisfaction

### Canonical Example
```cursive
// Canonical Example 3.9: Bounds

predicate BufferLike<T> {
    associated type Item

    procedure push(self: unique $, value: Item)
        grants alloc::heap
    {
        self::extend_with(value)
    }
}

procedure write_all<T, G>(
    items: [T],
    mut buffer: unique BufferLike<T>,
    sink: (T) -> () ! grants<G>
)
    grants<G>, alloc::heap
    where
        T: Copy,
        BufferLike<T>: Sequence,
        G <: {io::write}
{
    loop element in items {
        buffer.push(copy element)
        sink(element)
    }
}


// Region-based example (Cursive uses lexical scope, not lifetime parameters)
procedure demonstrate_region_bounds()
    grants alloc::region
{
    region r {
        let value: i32 = ^42
        let shared_ref: shared i32 <- value
        println("Shared value: {}", shared_ref)

        // Region escape analysis prevents returning region-backed data
        // result shared_ref  // ERROR E4027: Cannot escape region
    }
    // Region 'r' deallocated here (RAII)
}

procedure compare_grant_sets<G₁, G₂>(f: () -> () ! grants<G₁>)
    grants<G₁>
    where G₁ <: G₂, G₂ <: {io::write, alloc::heap}
{
    f()
}
```

---

## Section 3.10: Type Aliases 🔄 TRANSFORM

### Source
- `old_Type-System.md` lines 5131-5216 (~85 lines)

### Strategy
Clarify the difference between transparent and opaque aliases, document alias visibility/linkage interactions, and show how aliases participate in generics and System 3 permission annotations.

### Subsections

#### 3.10.1 Type Alias Declaration
- Syntax `type Name = ExistingType`
- Module-level visibility and linkage
- Aliases in documentation and diagnostics

#### 3.10.2 Transparent vs Opaque Aliases
- Transparent alias: identical substitution semantics
- Opaque alias: distinct nominal type with separate implementations
- Use cases and migration guidance

#### 3.10.3 Generic Type Aliases
- Type parameters and const generics on aliases
- Aliases for partially applied generic types
- Interaction with predicate bounds and grants

### Formal Elements Required
- **Definition 3.10.1**: Transparent alias
- **Definition 3.10.2**: Opaque alias
- **Definition 3.10.3**: Generic alias instantiation
- **Algorithm 3.10.1**: Alias substitution during type checking
- **Inference rules**: Alias Formation, Alias Visibility

### Canonical Example
```cursive
// Canonical Example 3.10: Type Aliases

// Transparent alias: interchangeable with (f64, f64)
type Coordinate = (f64, f64)

// Opaque alias: distinct nominal type
opaque type Kilograms = f64
// Construction and projection helpers for opaque aliases are
// specified in the final text to enforce type-safety boundaries.

function centroid(points: [Coordinate]): Coordinate {
    var sum_x: const f64 = 0.0
    var sum_y: const f64 = 0.0

    loop point in points {
        sum_x = sum_x + point.0
        sum_y = sum_y + point.1
    }

    let count: const f64 = points.length as f64
    result (sum_x / count, sum_y / count)
}

// Generic type alias with permission annotation
type SharedSlice<T> = shared [T]

procedure print_slice<T: Display>(items: SharedSlice<T>)
    grants io::write
{
    loop item in items {
        println("{}", item)
    }
}
```

---

## Section 3.11: Type Introspection 🔄 TRANSFORM

### Source
- `old_Type-System.md` lines 5217-5328 (~111 lines)

### Strategy
Document compile-time type queries, runtime reflection hooks, and predicate-based introspection in a unified manner. Ensure the section references Part I notation conventions and Part II translation phases for comptime evaluation.

### Subsections

#### 3.11.1 typeof Operator
- Syntax `typeof(expr)` returning compile-time type objects
- Evaluation rules (executes during type checking)
- Use in diagnostics and generic programming

#### 3.11.2 Type Predicates
- `is` expression syntax (`value is T`)
- Exhaustiveness checking with union types (§3.3.3)
- Interaction with predicates (e.g., `if value is Iterable<T>`)

#### 3.11.3 Compile-time Type Queries
- Compiler intrinsics: `type_name_of<T>()`, `size_of<T>()`, `alignment_of<T>()`
- Grant requirements for introspection (typically no grants required for compile-time queries)
- Const-evaluation guarantees and diagnostics

### Formal Elements Required
- **Definition 3.11.1**: Type metaobject (result of `typeof`)
- **Definition 3.11.2**: Type predicate expression
- **Algorithm 3.11.1**: Compile-time evaluation of type queries
- **Inference rules**: Typeof Introduction, Type Predicate Narrowing, Type Query Well-Formedness

### Canonical Example
```cursive
// Canonical Example 3.11: Type Introspection

function describe_slice<T>(items: [T]) {
    let ty = typeof(items)
    // Compiler intrinsics for compile-time type information
    println("Slice element type: {}", type_name_of<T>())
    println("Slice size: {} bytes", size_of::<[T]>())

    if items is [Kilograms] {
        println("Mass slice detected")
    }
}

procedure print_dynamic(value: i32 \/ string)
    grants io::write
{
    match value {
        v: i32 => println("Integer => {}", v),
        s: string => println("String => {}", s)
    }
}

procedure ensure_iterable<T: Iterable>(collection: T)
    grants io::write
{
    if collection is Iterable<T> {
        collection.iter(|item| println("{}", item))
    }
}
```

---

## Writing Standards (Apply to All Sections)

### Normative Language
- SHALL/MUST: Absolute requirement
- SHOULD: Recommended
- MAY: Optional
- MUST NOT: Absolute prohibition

### Cross-References
- Use `§X.Y.Z` for internal references
- Use `Part N §X.Y` for cross-part references
- Use `[REF_TBD]` as placeholder during writing

### Formal Notation
- Inference rules: Horizontal line with premises above, conclusion below
- Judgments: `Γ ⊢ e : τ`
- Subtyping: `τ <: υ`
- Equivalence: `τ ≡ υ`

### Examples
- ONE canonical example per section
- Use correct Cursive syntax (loop, not for)
- Use System 3 permissions
- Use predicate terminology (not predicate)
- Include System 3 permission demonstrations

### Structure
- Definition -> Rules -> Examples -> Formal elements

---

## Validation Checklist (For Each Section)

1. ✅ All "predicate" changed to "predicate"
2. ✅ All "for" changed to "loop"
3. ✅ All System 1 changed to System 3
4. ✅ All "effect" changed to "grant"
5. ✅ One canonical example per section
6. ✅ Formal elements listed
7. ✅ Cross-references validated
8. ✅ Normative language correct
9. ✅ Error codes assigned
10. ✅ Grammar specifications complete
11. ✅ Inference rules provided
12. ✅ Algorithm specifications complete
13. ✅ Forward references documented
14. ✅ System 3 permissions demonstrated

---

## Success Criteria

1. **Complete coverage**: All 12 sections written
2. **Correct terminology**: "predicate" used consistently
3. **Correct syntax**: `loop` (not `for`) throughout
4. **System 3 permissions**: All examples use binding categories × permissions
5. **Unified predicates**: Section 3.7 combines marker + concrete predicates
6. **Dependency order**: Subtyping -> Types -> Predicates -> Generics -> Bounds
7. **New content**: Union types (3.3.3), Range types (3.2.7), Grant bounds (3.9.3)
8. **Publication quality**: ISO/ECMA compliance
9. **Implementer-ready**: Sufficient detail for compiler implementation
10. **Cross-references validated**: All [REF_TBD] resolved

---

## Estimated Completion

- **12 sections**
- **44 subsections**
- **~50 formal definitions**
- **~30 algorithms**
- **~40 inference rules**
- **12 canonical examples**
- **~2500-3000 lines total**

**Estimated effort**: 40-50 hours of detailed technical writing

---

**END OF WRITING PLAN**
