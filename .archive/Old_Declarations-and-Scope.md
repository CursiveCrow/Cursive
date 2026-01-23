# The Cursive Language Specification

**Part**: III - Declarations and Scope
**File**: 03_Declarations-and-Scope.md
**Previous**: [Type System](02_Type-System.md) | **Next**: [Lexical Permission System](04_Lexical-Permission-System.md)

---

## Abstract

This chapter specifies how Cursive names are introduced, resolved, and made visible across program boundaries. It provides: memory safety through explicit binding semantics, AI-assisted development support via a predictable two-keyword model (`let`/`var`), and modular organization through comprehensive visibility and scope rules.

**Definition 3.1 (Declarations and Scope):** The Cursive declaration system comprises:

- The universe of declarations (variables, types, functions, contracts)
- The binding judgment for name introduction
- The scope hierarchy (block, function, module)
- The visibility system (public, internal, private, protected)
- The name resolution algorithm

The system MUST provide unambiguous name resolution, prevent redeclaration errors, support forward references through two-phase compilation, and integrate seamlessly with the permission system (Part IV), effect system (Part VI), and modal types (Part II).

---

## 3.0 Declaration Grammar and Foundations

**Definition 3.2 (Declaration Grammar):** This section establishes the complete syntactic foundations for all declaration forms in Cursive, including variables, types, functions, contracts, and imports.

### 3.0.1 Scope

This section provides the normative EBNF grammar for all declaration forms that introduce names into scopes. It establishes:

- Complete syntax for module-level and block-level declarations
- Grammar for visibility modifiers and permission qualifiers
- Import and use declaration syntax
- Foundation for name introduction semantics

**Normative coverage:**

- Top-level declaration forms (module scope)
- Variable declaration syntax (let, var, shadow)
- Type declaration syntax (aliases and nominal types)
- Function and procedure declaration syntax
- Contract and trait declaration syntax
- Import and use declaration syntax
- Visibility and permission grammar
- Auxiliary productions for qualified names and parameter lists

### 3.0.2 Cross-Part Dependencies

CITE: Part I (notation), Part II (types), Part IV (permissions), Part VI (effects).

### 3.0.3 Complete Declaration Grammar

The canonical EBNF for declarations resides in Appendix A.4. For convenience:

- **Top-level forms:** `TopLevel`, `Decl`, and related productions cover variables, types, functions, procedures, contracts, and traits; see A.4.1.
- **Variable declarations:** `VarDecl` captures `let`, `var`, and `shadow`; see A.4.2. The `const` keyword does not exist—compile-time constants arise via context (see §3.3).
- **Type declarations:** `TypeDecl`, `RecordDecl`, `EnumDecl`, and `ModalDecl` appear in A.4.3 and cross-reference the type constructors defined in Part II.
- **Callable declarations:** `FuncDecl` and `ProcDecl` (A.4.4) reference parameter lists and contract clauses defined in A.5 and Part VI.
- **Imports and uses:** `UseDecl`, `ModulePath`, and supporting productions are listed in A.4.5.
- **Visibility and permissions:** `Visibility`, `Permission`, and `ParamList` appear in Appendix A alongside their semantic rules in this chapter and Part II.

### 3.0.4 Single Namespace Policy

Types and values occupy a unified namespace per scope; names cannot collide. CITE: §3.7.3.

### 3.0.5 Scope Ownership

Module scope: top-level declarations. Block scope: declarations within `{}`. Import scope: use/import declarations. CITE: §3.7, §3.10.

---

## 3.1 Declarations Overview

Cursive supports six declaration forms (variables, types, functions, procedures, contracts, traits) appearing at module, function, or block scope. Two-phase compilation enables forward references. The two-keyword binding model (`let` immutable, `var` mutable) maximizes LLM-friendliness. Single-binding rule prevents redeclaration; shadowing requires explicit `shadow` keyword to prevent bugs. CITE: §3.2–3.5 for detailed specifications.

---

## 3.2 Variable Declarations

### 3.2.1 Syntax and Semantics

**Two-Keyword Model:**

```cursive
let x = 5      // Immutable binding
var y = 5      // Mutable binding
```

**Explicit statement:** The `const` keyword does NOT exist in Cursive. Compile-time constants are expressed using `let` bindings whose context makes them compile-time evaluable.

**Basic Syntax:** The canonical production `VarDecl` is defined in Appendix A.4 (Declarations Grammar).

**Semantics:**

- **`let`**: Introduces an immutable binding. The binding cannot be reassigned after initialization.
- **`var`**: Introduces a mutable binding. The binding may be reassigned.

**Type annotation:** Optional. If omitted, type is inferred from the initializer expression.

**Initializer:** Mandatory. All variables must be initialized at declaration.

**Example:**

```cursive
let x: i32 = 42        // Immutable
var counter = 0        // Mutable, type inferred
```

### 3.2.2 Single-Binding Rule (Normative)

**Rule 3.2.1 (Single-Binding):** A name may be introduced at most once per lexical scope.

**Formal statement:** For any lexical scope S, each identifier may be bound at most once.

**Formal rule:**

```
[Decl-Single-Binding]
name introduced in scope S
name already exists in scope S
--------------------------------
ERROR E3D01: Redeclaration in same scope
```

**Enforcement:** The compiler MUST reject any attempt to redeclare a name within the same lexical scope.

**Example (Error):**

```cursive
let x = 5
let x = 10  // ERROR E3D01: Redeclaration of 'x' in same scope
```

**Example (Valid - nested scope):**

```cursive
let x = 5
{
    shadow let x = 10  // OK: different scope, explicit shadow
}
```

**Diagnostic E3D01:**

- **Message:** "Redeclaration of '{name}' in same scope"
- **Fix:** Use a different name or shadow in a nested scope

CITE: §3.7.2 — Shadowing Rules for nested scope behavior.

### 3.2.3 Explicit Shadowing (Normative)

**Rule 3.2.2 (Explicit Shadowing):** Shadowing an outer binding requires the `shadow` keyword.

**Purpose:** Prevent accidental shadowing, a common source of bugs in both human-written and LLM-generated code.

**Formal rules:**

```
[Decl-Explicit-Shadow]
shadow let x = expr in scope S_inner
∃ outer scope S_outer. x ∈ S_outer ∧ S_outer encloses S_inner
same namespace category (both type or both value)
----------------------------------------------------
Introduces new binding x in S_inner, shadowing x in S_outer
x in S_outer inaccessible within S_inner

[Decl-Implicit-Shadow-Error]
let x = expr (without shadow) in scope S_inner
∃ outer scope S_outer. x ∈ S_outer ∧ S_outer encloses S_inner
----------------------------------------------------
ERROR E3D02: Implicit shadowing of '{x}'; use 'shadow let x'

[Decl-Invalid-Shadow]
shadow let x = expr in scope S_inner
∄ outer scope S_outer. x ∈ S_outer
----------------------------------------------------
ERROR E3D07: Invalid shadow; no outer binding named '{x}'

[Decl-Namespace-Conflict]
shadow let x = expr
outer x is type, attempting to shadow with value (or vice versa)
----------------------------------------------------------------
ERROR E3D03: Cannot shadow type '{x}' with value (or vice versa)
```

**Example:**

```cursive
let value = 10
{
    shadow let value = value * 2  // OK: explicit shadow
    println(value)  // Prints 20
}
println(value)  // Prints 10 (original)
```

### 3.2.4 Uniform Binding Invariants (Normative)

**Definition 3.3 (Binding vs Value Mutability):** Binding mutability is orthogonal to value mutability.

**Principle:** The keywords `let` and `var` control whether a **binding** can be reassigned. Value mutability is controlled by the **permission system** (Part IV).

**Key distinctions:**

- **Binding immutability** (`let`): The binding itself cannot be reassigned to point to a different value.
- **Value mutability** (permissions): Whether the value can be mutated in-place through the binding depends on permissions (`own`, `mut`, `imm`).

**Canonical example:**

```cursive
// Immutable binding with mutable permission
let buffer: mut Vec<u8> = get_buffer()
buffer.push(42)     // OK: mut permission allows mutation
buffer = other_vec  // ERROR E3D10: cannot reassign let binding

// Mutable binding allows reassignment
var counter: i32 = 0
counter = 5         // OK: var allows reassignment
```

**Formal integration:** This design integrates cleanly with:

- **Permission system** (Part IV — Lexical Permission System): Permissions control value access, not binding
- **Modal types** (Part II): Modal transitions work with immutable bindings
- **Effect system** (Part VI — Contracts and Effects): Effects declared on operations, not bindings

CITE: Part IV — Lexical Permission System; Part II §2.3.3 — Modal Types.

### 3.2.5 Global Variables (Module-Scope)

**Module-scope variable declarations:**

```cursive
let GLOBAL_CONST: usize = 1024  // Module-scope immutable
var global_state: i32 = 0        // Module-scope mutable
```

**Properties:**

- **Storage duration:** Static (exists for entire program execution)
- **Visibility:** Subject to visibility modifiers (default: internal)
- **Initialization:** Evaluated at program startup (before main)
- **Compile-time evaluation:** Global `let` bindings become compile-time constants if the initializer expression is compile-time evaluable

**Compile-time constant property:**

```cursive
let MAX_SIZE: usize = 1024
let buffer: [i32; MAX_SIZE]  // OK: MAX_SIZE is compile-time constant
```

CITE: §3.3 — Compile-Time Evaluation; §3.9.1 — Static Storage.

### 3.2.6 Local Variables

**Function and block-scope variable declarations:**

```cursive
function process() {
    let x = 42        // Function-scope
    var counter = 0   // Function-scope

    {
        let y = 100   // Block-scope
    }
    // y not accessible here
}
```

**Properties:**

- **Storage duration:** Automatic (stack-allocated, freed at scope end)
- **Type inference:** Type may be omitted; inferred from initializer
- **Visibility:** Local to the declaring scope
- **Definite assignment:** Must be initialized on all paths before use

**Type inference examples:**

```cursive
let x = 42                    // Inferred: i32
let name = "Alice"            // Inferred: string
let point = Point { x: 1.0, y: 2.0 }  // Inferred: Point
let numbers = [1, 2, 3]       // Inferred: [i32; 3]
```

CITE: §3.2.7 — Definite Assignment; §3.9.1 — Automatic Storage.

### 3.2.7 Definite Assignment (Normative)

**Rule 3.2.3 (Definite Assignment):** Every use of a variable must be dominated by an initialization on all control flow paths.

**Formal statement:** For a variable v and use site u, all paths P from v's declaration to u must contain an initialization of v.

**Algorithm:**

```
definite_assignment_check(var, use_site):
    paths := all_control_flow_paths_to(use_site)

    for each path in paths:
        if not path.dominates_initialization(var):
            ERROR E3D06: Use of possibly uninitialized variable '{var}'

    return OK

dominates_initialization(path, var):
    // Check if var is initialized on this path before use
    for each statement in path (in order):
        if statement assigns to var:
            return true
    return false
```

**Canonical example:**

```cursive
var x: i32
if condition {
    x = 5
} else {
    x = 10
}
use(x)  // OK: all paths initialize x

var y: i32
if other_condition {
    y = 3
}
use(y)  // ERROR E3D06: y may not be initialized (else branch missing)
```

**Diagnostic E3D06:**

- **Message:** "Use of possibly uninitialized variable '{name}'"
- **Note:** "Variable may not be initialized on all code paths"
- **Fix:** "Ensure variable is assigned on all paths before use"

### 3.2.8 Assignment to Immutable Binding

**Rule:** Assignment requires a mutable binding (`var`).

**Formal constraint:**

```
[Assign-Requires-Var]
target = expr
target is let binding
--------------------------------
ERROR E3D10: Cannot assign to immutable binding '{target}'
```

**Examples:**

**Valid (var allows reassignment):**

```cursive
var counter = 0
counter = 5     // OK: var allows reassignment
counter += 1    // OK: compound assignment
```

**Invalid (let prohibits reassignment):**

```cursive
let x = 5
x = 10  // ERROR E3D10: Cannot assign to immutable binding 'x'
```

**Note on mutation vs reassignment:**

```cursive
let vec: Vec<i32> = Vec.new()
vec::push(42)  // OK: mutating through procedure (not reassignment)
vec = other   // ERROR E3D10: cannot reassign let binding
```

**Diagnostic E3D10:**

- **Message:** "Cannot assign to immutable binding '{name}'"
- **Help:** "Consider using 'var {name}' if reassignment is needed"

---

## 3.3 Compile-Time Evaluation

### 3.3.1 Context-Driven Evaluation (Normative)

**Rule 3.3.1 (Compile-Time Constants):** A `let` binding becomes a compile-time constant when:

1. Declared at **module scope** AND the initializer expression is compile-time evaluable, OR
2. Declared inside a **`comptime` block**

**Context determines evaluation time, not a keyword.**

**Formal conditions:**

```
[Const-Module-Scope]
let name = expr
declared at module scope
expr is compile-time evaluable
--------------------------------
name is compile-time constant

[Const-Comptime-Block]
comptime {
    let name = expr
}
--------------------------------
name is compile-time constant
```

**Implications:**

- Module-scope `let` with compile-time initializer → compile-time constant (usable in types)
- Function-scope `let` with compile-time initializer → runtime binding (not usable in types)
- Any `let` in `comptime` block → compile-time constant

CITE: §3.3.2 — Compile-Time Evaluable Expressions; Part X — Comptime Metaprogramming.

### 3.3.2 Compile-Time Evaluable Expressions

| Category | Examples | Evaluable |
|----------|----------|-----------|
| Literals | `42`, `3.14`, `true`, `'A'`, `"hello"` | Yes |
| Arithmetic/logical ops | `A + B * 2` (on const values) | Yes |
| Comptime function calls | `comptime function f(x) {...}; f(21)` | Yes |
| Const references | `let MAX = 1000; MAX / 2` | Yes |
| Type expressions | `[i32; SIZE]` | Yes |
| Runtime function calls | Regular function calls | No |
| I/O operations | File/network operations | No |
| Heap allocations | `new`, `alloc` (non-comptime) | No |
| Runtime variables | Function-scope variables | No |

### 3.3.3 Type Position Usage (Normative)

**Rule 3.3.2 (Type Position Requirements):** Names used in type positions MUST refer to compile-time constants.

**Type positions include:**

- Array lengths: `[T; N]`
- Const generic arguments: `Function<N>`
- Other type expressions requiring constant values

**Formal rules:**

```
[Type-Position-Const]
Type ::= "[" Type ";" Expr "]"
Expr is compile-time evaluable
--------------------------------
Array type well-formed

[Type-Position-Error]
Type uses name N in position requiring compile-time value
N is not compile-time constant
--------------------------------
ERROR E3D08: Non-const name '{N}' used in type position
```

**Example:**

```cursive
let N = 100
let buffer: [u8; N]  // OK: N is module-scope compile-time constant

function foo() {
    let size = 1024  // Runtime binding (function scope)
    let arr: [i32; size]  // ERROR E3D08: size is not compile-time here
}
```

---

## 3.4 Type Declarations

### 3.4.1 Overview

Type declarations introduce new nominal types or transparent aliases into the namespace.

**Forms:**

- **Record declarations**: Introduce nominal product types (Part II §2.2.2)
- **Enum declarations**: Introduce nominal sum types (Part II §2.3.1)
- **Modal declarations**: Introduce state machine types (Part II §2.3.3)
- **Type aliases**: Introduce transparent name aliases for existing types

**Name introduction:** All type declarations introduce a type name in the declaring scope (typically module scope).

**Visibility:** Type declarations respect visibility modifiers; private types are not accessible outside their module.

CITE: Part II — Type System; §3.10 — Visibility and Access Control.

### 3.4.2 Type Alias Syntax

**Syntax:** The canonical `TypeAlias` production is defined in Appendix A.4; this section focuses on semantics.

**Examples:**

```cursive
type UserId = u64
type Point2D = (f64, f64)
type Result<T> = Result<T, Error>
```

**Semantics:** Type aliases are **transparent**. The alias name and the aliased type are fully interchangeable.

**Equivalence:**

```
[Equiv-Alias]
type A = τ
----------
A ≡ τ
```

The compiler treats `A` and `τ` as the same type.

**Use cases:**

- Improving code readability
- Shortening complex type names
- Providing domain-specific names

CITE: Part II §2.11 — Type Aliases; §2.0.6.2 — Type Equivalence.

### 3.4.3 Newtype Pattern

For creating **distinct** types (not just aliases), use the single-field record pattern:

```cursive
// Alias - NOT distinct
type Meters = f64
type Feet = f64
let m: Meters = 10.0
let f: Feet = m  // OK: Meters ≡ Feet (both are f64)

// Newtype - distinct
record Meters(f64)
record Feet(f64)
let m = Meters(10.0)
let f: Feet = m  // ERROR: Meters ≢ Feet (nominal types)
```

**Rationale:** Newtype pattern prevents accidental mixing of semantically different values with the same representation.

CITE: Part II §2.2.3 — Tuple-Structs.

### 3.4.4 Forward References

**Policy:** Forward references to types are permitted through two-phase compilation.

**Self-referential types:**

```cursive
record Node {
    value: i32
    next: Ptr<Node>  // OK: forward reference to Node itself
}
```

**Mutually recursive types:**

```cursive
record Tree {
    data: i32
    forest: Forest
}

record Forest {
    trees: Vec<Tree>
}
```

**Requirement:** Recursive types must include pointer indirection to avoid infinite size.

```cursive
record Bad {
    child: Bad  // ERROR: infinite size
}

record Good {
    child: Ptr<Good>  // OK: pointer breaks cycle
}
```

### 3.4.5 Type Visibility

Type declarations respect visibility modifiers:

```cursive
public record PublicType { ... }     // Accessible from other modules
internal record InternalType { ... } // Module-only (default)
private record PrivateType { ... }   // Context-only
```

**Private types** are not accessible outside their declaring module, even through re-exports or type aliases.

CITE: §3.10 — Visibility and Access Control.

---

## 3.5 Function and Procedure Declarations (Overview)

### 3.5.1 Name Introduction

**Module-scope functions:**

Top-level `function` and `procedure` declarations introduce callable names in module scope.

```cursive
function add(a: i32, b: i32): i32 { a + b }
procedure log(msg: string) uses io.write { println(msg) }
```

**Local function declarations: NOT PERMITTED**

Local function declarations are not permitted. Use closures for local callables. CITE: Part IX — Functions and Procedures.

### 3.5.2 Permission-Aware Parameters (Normative)

Parameters have explicit permissions (`imm`, `mut`, `own`) controlling aliasing and ownership. Default: `imm`. CITE: Part IV — Lexical Permission System for full semantics.

### 3.5.3 Parameter Scope

**Rule 3.5.2 (Parameter Scope):** Parameters occupy the function's outermost local scope.

**Implications:**

1. Parameters are in scope for the entire function body
2. Parameters CANNOT be shadowed in the function's outer block
3. Parameters CAN be shadowed in nested inner blocks with `shadow`

**Example (Error - shadowing param in outer block):**

```cursive
function process(x: i32) {
    shadow let x = x + 1  // ERROR E3D01: cannot shadow param in same scope
    println(x)
}
```

**Example (Valid - shadowing in nested block):**

```cursive
function process(x: i32) {
    println(x)  // Uses parameter

    {
        shadow let x = x * 2  // OK: nested block
        println(x)  // Uses shadowed value
    }

    println(x)  // Uses parameter again
}
```

**Rationale:** Parameters are conceptually initialized at function entry and exist in the same scope as the function body's outermost block, preventing confusion about which `x` is in scope.

### 3.5.4 Cross-References

Full syntax and semantics for functions and procedures are specified in later chapters:

- **Part IX (Functions and Procedures):** Complete function/procedure syntax, bodies, return semantics
- **Part VI (Contracts and Effects):** Contract clauses (`uses`, `must`, `will`)
- **Part V (Expressions and Operators):** Function call expressions
- **Part II (Types):** Map types (function types)

This chapter establishes only the **name introduction** and **parameter scoping** aspects of function declarations.

---

## 3.6 Contract Declarations (Overview)

### 3.6.1 Name Introduction

The `contract` keyword introduces an abstract interface type in module scope:

```cursive
contract Drawable {
    procedure draw($)
        uses io.write
}
```

**Properties:**

- Introduces a type name in module scope
- Abstract (cannot be directly instantiated)
- Defines behavioral interface without implementation
- Types implement contracts by providing matching procedures

### 3.6.2 Forward References

Contract declarations support forward references to types:

```cursive
contract Container {
    type Element  // Associated type
    procedure get($, index: usize): Option<Element>
}

// Forward reference OK
record IntVec: Container {
    type Element = i32
    // ... implementation
}
```

### 3.6.3 Cross-Reference

Full contract syntax, semantics, implementation rules, and clause specifications are in:

- **Part VI (Contracts and Effects):** Complete contract system
- **Part II §2.7:** Traits and contract distinction

This chapter establishes only the **name introduction** aspect of contract declarations.

---

## 3.7 Scope Rules

### 3.7.1 Scope Hierarchy

Cursive has three scope levels with distinct visibility and lifetime characteristics.

**Block Scope:**

A block `{ ... }` introduces a new lexical scope:

- Names declared within the block are visible from the point of declaration to the end of the block
- Nested blocks inherit outer scope bindings
- Block bindings may shadow outer bindings (with `shadow` keyword)
- Block bindings are NOT visible outside the block
- Storage: automatic (stack-allocated, freed at block end)

**Example:**

```cursive
let outer = 5
{
    let inner = 10  // Block-scoped
    shadow let outer = 20  // Shadows outer binding
    println(outer)  // Prints 20
}
// inner not accessible here
// outer accessible here (original value 5)
```

**Function Scope:**

Certain names have function-wide visibility:

- **Labels:** Loop and block labels visible anywhere in the function
- **Parameters:** Visible in function's outermost local scope (conceptually)

**Example:**

```cursive
function process() {
    'outer: loop {
        {
            break 'outer  // OK: label has function scope
        }
    }
}
```

**Module Scope:**

Top-level declarations are visible throughout the module:

- All module-level declarations share module scope
- Two-phase compilation allows forward references
- Visibility modifiers control cross-module access
- Storage: static (for variables)

**Example:**

```cursive
function foo() {
    bar()  // OK: forward reference
}

function bar() {
    foo()  // OK: mutual recursion
}
```

CITE: §3.1.2 — Declaration Levels; §3.9.1 — Storage Duration.

### 3.7.2 Shadowing Rules (Normative)

**Rule 3.7.1 (Shadowing):** Inner scopes may shadow outer scopes with the `shadow` keyword.

**Formal rules:**

```
[Scope-Shadow-Valid]
shadow let x = expr in scope S_inner
∃ S_outer. S_outer encloses S_inner ∧ x ∈ S_outer
same namespace category (both type or both value)
----------------------------------------------------
x in S_inner shadows x in S_outer
x in S_outer inaccessible within S_inner

[Scope-No-Redeclare]
let x = e1 in scope S
let x = e2 in scope S
----------------------------------------
ERROR E3D01: Redeclaration of 'x' in same scope
```

**Shadowing lifecycle:**

```cursive
let x = 1        // Outer scope
{
    shadow let x = 2  // Inner scope shadows
    println(x)   // Prints 2
    {
        shadow let x = 3  // Nested shadow
        println(x)   // Prints 3
    }
    println(x)   // Prints 2 (inner shadow, not nested)
}
println(x)       // Prints 1 (original)
```

**Invalid redeclaration:**

```cursive
{
    let x = 5
    let x = 10   // ERROR E3D01: Redeclaration in same scope
}
```

**Valid shadow alternatives:**

```cursive
let x = 5
{
    shadow let x = 10  // OK: different scope
}
// OR
let x = 5
shadow let x = 10  // ERROR E3D07: No outer scope (file-level)
```

**Diagnostics:**

- E3D01: Redeclaration in same scope
- E3D02: Implicit shadowing (see §3.2.3)
- E3D07: Invalid shadow (see §3.2.3)

### 3.7.3 Namespace Unification (Normative)

**Rule 3.7.2 (Single Namespace):** Types and values share a single namespace per scope.

**Policy:** A type name and a value name cannot both exist with the same identifier in the same scope.

**Rationale:**

- Simplifies name resolution
- Reduces ambiguity for LLMs
- Prevents confusion between type and value contexts

**Example (record vs variable conflict):**

```cursive
record Point { x: f64, y: f64 }

let Point = get_point()  // ERROR E3D03: 'Point' already declared as type
```

**Example (type alias vs variable conflict):**

```cursive
let x = 5

type x = i32  // ERROR E3D03: 'x' already declared as variable
```

**Valid (different scopes):**

```cursive
record Data { value: i32 }

{
    shadow let Data = compute_data()  // ERROR E3D03: Cannot shadow type with value
}
```

**Diagnostic E3D03:**

- **Message:** "Name '{name}' conflicts with existing {type|value} declaration"
- **Note:** "Types and values share the same namespace"
- **Fix:** "Use a different name for the {type|value}"

---

## 3.8 Import and Use Declarations

### 3.8.1 Syntax

**Import and use declaration forms:** The productions `UseDecl`, `QualifiedName`, and `ModulePath` are defined in Appendix A.4; this subsection focuses on scope and visibility semantics.

**Examples:**

**Simple use:**

```cursive
use std::collections::HashMap
let map: HashMap<string, i32> = HashMap.new()
```

**Selective use:**

```cursive
use std::io::{println, eprintln}
println("Hello")
eprintln("Error")
```

**Import with alias:**

```cursive
import std.collections as coll
let map: coll::HashMap<string, i32> = coll::HashMap.new()
```

### 3.8.2 Scope of Imports (Normative)

**Rule 3.8.1 (Import Scope):** Use/import declarations introduce names in the enclosing scope.

**Module-level imports:**

```cursive
use std::collections::Vec

function foo() {
    let v: Vec<i32> = Vec.new()  // Vec visible here
}
```

Names introduced in module scope are visible throughout the module.

**Block-level imports:**

```cursive
function foo() {
    {
        use std::collections::HashMap
        let map = HashMap.new()  // OK: HashMap visible in block
    }
    // HashMap NOT visible here (block-scoped import)
}
```

Names introduced in block scope are visible only within that block.

### 3.8.3 Qualification and Ambiguity (Normative)

**Rule 3.8.2 (Unambiguous Resolution):** Unqualified names must resolve unambiguously.

**Formal rules:**

```
[Import-Ambiguous]
use mod1::Symbol
use mod2::Symbol  // same unqualified name
reference to Symbol (unqualified)
----------------------------------------
ERROR E3D04: Ambiguous identifier 'Symbol'

[Import-Qualified-OK]
use mod1::Symbol as Symbol1
use mod2::Symbol as Symbol2
reference to Symbol1 or Symbol2
----------------------------------------
OK: explicit aliases resolve ambiguity
```

**Example:**

```cursive
use net::http::Request
use net::websocket::Request
let req = Request::new()  // ERROR E3D04: Ambiguous 'Request'

// Fix: use aliases or qualification
use net::http::Request as HttpRequest
let http_req = HttpRequest::new()  // OK
```

### 3.8.4 Import Shadowing

**Precedence rule:** Local declarations take precedence over imported names.

```cursive
use std::collections::Vec

function foo() {
    let Vec = get_vector_data()  // ERROR E3D02: Must use 'shadow let Vec'

    shadow let Vec = get_vector_data()  // OK: explicit shadow
    println(Vec)  // Uses local binding, not imported type
}
```

Imports are conceptually in module scope, so local declarations require `shadow` to override them.

### 3.8.5 Cross-Reference

Full import and module system specifications:

- **Part VIII (Modules):** Complete module system, import resolution, module paths
- **Part VIII:** Re-exports, circular dependency rules, initialization order

This chapter establishes only the **scoping** behavior of imports.

---

## 3.9 Lifetime and Storage Duration

### 3.9.1 Storage Classes

Cursive variables have three storage classes based on declaration context:

**Storage Classes Table:**

| Context        | Keyword | Storage Class | Duration         | Allocation   |
| -------------- | ------- | ------------- | ---------------- | ------------ |
| Module scope   | `let`   | Static        | Program lifetime | Data segment |
| Module scope   | `var`   | Static        | Program lifetime | Data segment |
| Function scope | `let`   | Automatic     | Scope lifetime   | Stack        |
| Function scope | `var`   | Automatic     | Scope lifetime   | Stack        |
| Block scope    | `let`   | Automatic     | Scope lifetime   | Stack        |
| Block scope    | `var`   | Automatic     | Scope lifetime   | Stack        |
| Region         | `let`   | Region        | Region lifetime  | Region pool  |

**Context mapping:**

- **Module-scope `let`**: Static storage, potentially compile-time constant
- **Module-scope `var`**: Static storage, mutable global
- **Function-scope `let`**: Automatic storage, immutable local
- **Function-scope `var`**: Automatic storage, mutable local

**Examples:**

```cursive
// Static storage
let GLOBAL_CONST = 100  // Module scope
var global_counter = 0  // Module scope

function foo() {
    // Automatic storage
    let local = 42      // Function scope
    var accumulator = 0 // Function scope

    {
        let block_local = 99  // Block scope
    }
}
```

CITE: §3.9.2 — Region Storage; Part IX — Memory Model.

### 3.9.2 Region Bindings and Escape (Normative)

**Rule 3.9.1 (Region Escape Prevention):** Bindings to region-allocated values cannot escape the region.

**Purpose:** Prevent use-after-free by ensuring region-allocated values are not accessible after the region ends.

**Formal rules:**

```
[Region-Escape-Binding]
region r {
    let value = alloc_in<r>(data)
}
value accessible outside region r
----------------------------------------
ERROR E3D09: Region value escapes region via binding

[Region-Escape-Return]
region r {
    let value = alloc_in<r>(data)
    return value  // attempts to return region value
}
----------------------------------------
ERROR E3D09: Region value escapes region via return
```

**Examples:**

**Invalid (escape via binding):**

```cursive
let escaped
region temp {
    let local = alloc_in<temp>(Buffer.new())
    escaped = local  // ERROR E3D09: region value escapes
}
```

**Invalid (escape via return):**

```cursive
function bad() -> Buffer {
    region temp {
        let buf = alloc_in<temp>(Buffer.new())
        return buf  // ERROR E3D09: cannot return region value
    }
}
```

**Valid (contained usage):**

```cursive
region temp {
    let buffer = alloc_in<temp>(Buffer.new())
    process(buffer)  // OK: used within region
}
// buffer deallocated here
```

**Diagnostic E3D09:**

- **Message:** "Region value escapes region '{region}' via {binding|return}"
- **Note:** "Region-allocated values cannot outlive their region"
- **Fix:** "Use value within region or allocate with different lifetime"

CITE: Part II §2.0.8.4 — Regions; Part IX — Memory Model.

### 3.9.3 RAII Cleanup

**Cleanup semantics:** Owned values are automatically cleaned up at the end of their scope.

**Order:** Reverse order of declaration (LIFO - Last In, First Out).

**Example:**

```cursive
{
    let a = Resource.new("A")  // Declared first
    let b = Resource.new("B")  // Declared second
    let c = Resource.new("C")  // Declared third
}
// Cleanup order: c, b, a (reverse)
```

**Rationale:** LIFO order ensures that dependencies are properly cleaned up (resources allocated later may depend on earlier resources).

CITE: Part IX — Memory Model; Part IV — Lexical Permission System (Ownership).

---

## 3.10 Visibility and Access Control

### 3.10.1 Visibility Modifiers (Normative)

**Grammar reference:** `Visibility` is defined in Appendix A.4. This section describes semantics only.

**Semantics:**

**`public`** — Accessible from any module that imports this module

```cursive
public let API_VERSION = "1.0"
public function process(data: string) { ... }
```

Any module that imports this module can access public declarations.

**`internal`** — Accessible only within the same module (DEFAULT)

```cursive
internal let impl_detail = 42  // Explicit (same as default)
let other_detail = 100          // Implicit internal
```

Not accessible from other modules, even with import.

**`private`** — Accessible only within the declaration context

For records and enums: accessible only within the declaring record/enum.

```cursive
record Account {
    private hash: string  // Only Account methods can access
}
```

For module-level: equivalent to `internal` (module-private).

**`protected`** — Accessible to implementing types (contracts/traits only)

```cursive
contract Base {
    protected procedure helper($) { ... }
}

record Derived: Base {
    procedure proc($) {
        self::helper()  // OK: protected accessible to implementations
    }
}
```

### 3.10.2 Default Visibility Policy (Normative)

**Rule 3.10.1 (Default Visibility):** Module-level declarations have `internal` visibility by default.

**Formal statement:**

```
[Visibility-Default]
declaration D at module scope
no visibility modifier specified
--------------------------------
visibility(D) = internal
```

**Examples:**

```cursive
// All internal by default
let config = load_config()
var state = 0
function helper() { ... }
record InternalType { ... }

// Must explicitly mark public
public let PUBLIC_API = "v1"
public function api_call() { ... }
```

**Rationale:** Default internal promotes encapsulation and prevents accidental API exposure.

### 3.10.3 Field-Level Visibility

Record and enum fields may specify visibility modifiers; the production `FieldDecl` in Appendix A.4 captures the syntax.

**Example:**

```cursive
record User {
    public name: string        // Accessible anywhere
    internal email: string     // Module-only
    private password_hash: string

    internal procedure reveal_hash(self: User): string {
        result self.password_hash
    }
}
```

**Access behavior:**

```cursive
let user = User {
    name: "Alice",
    email: "alice@example.com",
    password_hash: hash("secret")
}

// From same module
println(user.name)           // OK: public
println(user.email)          // OK: internal, same module
// println(user.password_hash)  // ERROR: private field; use a helper
println(user::reveal_hash())  // OK: internal procedure has access

// From different module
println(user.name)           // OK: public
println(user.email)          // ERROR: internal, cross-module
// println(user.password_hash)  // ERROR: private, cross-module
// println(user::reveal_hash())  // ERROR: procedure is internal to defining module
```

CITE: Part II §2.2.2 — Records.

### 3.10.4 Procedure Access to Private Fields (Normative)

**Rule 3.10.2 (Procedure Access):** Procedures defined in the same module as a type can access that type's private fields.

**Formal statement:**

```
[Procedure-Access-Private]
procedure M defined in module Mod
type T defined in module Mod
field f of T has private visibility
----------------------------------------
M can access T.f
```

**Example:**

```cursive
// In module 'auth'
record Account {
    public id: u64
    private balance: i64
}

procedure Account.withdraw(self: mut Account, amount: i64) {
    self.balance -= amount  // OK: same module, can access private
}

// In different module
use auth::Account
function steal(acct: Account) {
    acct.balance -= 100  // ERROR: private field, cross-module
}
```

**Rationale:** Allows encapsulation while enabling methods to manipulate internal state.

### 3.10.5 Cross-Module Access Checking (Normative)

**Formal rule:**

```
[Visibility-Check]
reference to symbol S from module M2
S declared in module M1 with visibility V
----------------------------------------
If M1 = M2: OK (same module)
If M1 ≠ M2 and V = public: OK
If M1 ≠ M2 and V ∈ {internal, private, protected}: ERROR
```

**Examples:**

**Valid (public cross-module):**

```cursive
// Module A
public let MAX_SIZE = 1024
public function process() { ... }

// Module B
use A::{MAX_SIZE, process}
let buffer: [i32; MAX_SIZE]  // OK: public
process()  // OK: public
```

**Invalid (internal cross-module):**

```cursive
// Module A
let internal_detail = 42  // internal by default

// Module B
use A::internal_detail  // ERROR: internal, not accessible
```

**Invalid (private cross-module):**

```cursive
// Module A
record Data {
    private secret: string
}

// Module B
use A::Data
let d = Data { secret: "x" }  // ERROR: private field, cross-module
```

---

## 3.11 Name Resolution Algorithm

### 3.11.1 Resolution Order (Normative)

**Algorithm:** The compiler resolves unqualified identifiers using a five-step lookup process.

```
resolve_name(identifier):
    // Step 1: Current block scope
    if identifier ∈ current_block:
        return current_block.lookup(identifier)

    // Step 2: Outward lexical scopes
    for scope in enclosing_scopes (innermost to outermost):
        if identifier ∈ scope:
            return scope.lookup(identifier)

    // Step 3: Module scope
    if identifier ∈ module_scope:
        return module_scope.lookup(identifier)

    // Step 4: Imported namespaces
    candidates := []
    for import in current_scope.imports:
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
    ERROR: Undefined identifier '{identifier}'
```

**Resolution order guarantees:**

1. Innermost scope takes precedence (shadowing)
2. Local declarations override imports
3. Imports checked before predeclared identifiers
4. Ambiguous imports are errors (require qualification)

**Example:**

```cursive
let global = 1

function foo() {
    let local = 2
    {
        shadow let local = 3
        println(local)  // Resolves to: block-scope local (3)
    }
    println(local)  // Resolves to: function-scope local (2)
}

println(global)  // Resolves to: module-scope global (1)
```

### 3.11.2 Qualified Name Resolution

**Algorithm:** Qualified names (`Prefix::Name`) are resolved in two steps.

```
resolve_qualified(prefix::name):
    // Step 1: Resolve prefix as module or alias in current scope
    module := resolve_name(prefix)

    if module is not found:
        ERROR: Unknown module or alias '{prefix}'

    if module is not a module/import alias:
        ERROR: '{prefix}' is not a module (cannot qualify)

    // Step 2: Lookup name in module's exported symbols
    if name ∈ module.exported_names:
        return module.lookup(name)
    else if name ∈ module.all_names:
        ERROR: '{name}' exists in '{prefix}' but is not public
    else:
        ERROR: '{name}' not found in module '{prefix}'
```

**Examples:**

**Valid:**

```cursive
use std::collections
let map: collections::HashMap<string, i32> = collections::HashMap.new()
```

**Invalid (not exported):**

```cursive
use mymodule
let x = mymodule::internal_fn()  // ERROR: internal_fn not public
```

**Invalid (prefix not a module):**

```cursive
let x = 5
let y = x::something  // ERROR: x is not a module
```

### 3.11.3 Diagnostics

- **E3D04:** Ambiguous unqualified identifier (see §3.8.3)

---

## 3.12 Predeclared Identifiers (Universe Scope)

### 3.12.1 Predeclared Types

The following primitive type names are predeclared in the universe scope and are available in all modules without import:

**Integer types:**

- Signed: `i8`, `i16`, `i32`, `i64`, `isize`
- Unsigned: `u8`, `u16`, `u32`, `u64`, `usize`

**Floating-point types:**

- `f32`, `f64`

**Other primitive types:**

- `bool` — Boolean type
- `char` — Unicode scalar value
- `string` — Modal string type (Part II §2.1.6)
- `!` — Never type (bottom type)
- `()` — Unit type

**Total:** 14 predeclared type names.

**Immutability:** Predeclared type names are immutable and non-shadowable.

CITE: Part II §2.1 — Primitive Types.

### 3.12.2 Predeclared Constants

**Boolean constants:**

- `true: bool`
- `false: bool`

These are predeclared in the universe scope and always available.

### 3.12.3 Predeclared Functions

**Diverging functions:**

- `panic: (string) → ! uses panic`

**Debug assertions (if built-in):**

- `assert: (bool) → () uses panic`

**Note:** Additional built-in functions may be specified in Part XVI (Builtins).

CITE: Part XVI — Built-in Functions and Standard Library.

### 3.12.4 Protection (Normative)

**Rule 3.12.1 (Predeclared Protection):** Predeclared identifiers cannot be redefined or shadowed.

**Formal rules:**

```
[Predeclared-No-Redefine]
let identifier = value
identifier ∈ {i8, i16, i32, ..., bool, char, string, true, false}
----------------------------------------------------------------
ERROR E3D12: Cannot redefine predeclared identifier '{identifier}'

[Predeclared-No-Shadow]
shadow let identifier = value
identifier ∈ universe_scope
----------------------------------------
ERROR E3D12: Cannot shadow predeclared identifier '{identifier}'
```

**Example (redefine error):**

```cursive
let i32 = 42  // ERROR E3D12: Cannot redefine predeclared type 'i32'
```

**Example (shadow error):**

```cursive
{
    shadow let bool = get_bool()  // ERROR E3D12: Cannot shadow 'bool'
}
```

**Rationale:** Protecting predeclared identifiers prevents confusion and maintains stable semantics for fundamental language constructs.

**Diagnostic E3D12:**

- **Message:** "Cannot {redefine|shadow} predeclared identifier '{name}'"
- **Note:** "'{name}' is a built-in {type|constant|function}"
- **Fix:** "Choose a different name"

---

## 3.13 Inter-Part Authority Contract

### 3.13.1 Normative Authority

**Authority Statement:** Part III (Declarations and Scope) is the authoritative specification for:

1. **Name introduction semantics** — How declarations introduce names into scopes
2. **Scope rules** — Block, function, and module scope hierarchy
3. **Visibility semantics** — Cross-module access control
4. **Name resolution** — The algorithm for resolving identifiers to declarations
5. **Shadowing and rebinding** — Rules for name reuse across scopes

**Precedence:** Where conflicts arise between this chapter and other chapters regarding scoping or visibility, this chapter takes precedence.

### 3.13.2 Cross-Part Coordination

Other specification parts define full syntax and semantics of their constructs but MUST NOT contradict:

- Scoping rules established in §3.7
- Visibility semantics established in §3.10
- Name resolution algorithm in §3.11
- Shadowing requirements in §3.2.3

**Obligation:** Part authors must ensure their specifications reference Part III for name introduction and scoping behavior rather than redefining those rules.

### 3.13.3 Integration Points

**Part II (Types):**

- References Part III for type declaration forms (§3.4)
- Type grammar includes visibility modifiers defined here
- Record/enum field visibility follows rules in §3.10

**Part IV (Lexical Permission System):**

- Permission qualifiers (`own`, `mut`, `imm`) used in variable bindings and parameters
- Distinction between binding mutability and value mutability (§3.2.4)

**Part V (Expressions and Operators):**

- Variable references resolved using algorithm in §3.11
- Name resolution errors (undefined identifier) originate from §3.11

**Part VI (Contracts and Effects):**

- Contract declarations follow scoping rules in §3.7
- Contract visibility follows rules in §3.10
- Contract parameters use permission-aware syntax from §3.5.2

**Part IX (Functions and Procedures):**

- Function declarations follow scoping rules in §3.7
- Parameter scoping follows rules in §3.5.3
- Function visibility follows rules in §3.10

**Part VIII (Modules):**

- Import/use declarations follow scoping rules in §3.8
- Module-level visibility rules reference §3.10
- Name resolution integrates with §3.11

---


**Previous**: [Type System](02_Type-System.md) | **Next**: [Expressions and Operators](04_Expressions-and-Operators.md)
