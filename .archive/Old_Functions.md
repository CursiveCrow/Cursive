# The Cursive Language Specification

**Part**: IX - Functions and Procedures
**File**: 09_Functions.md
**Previous**: [Holes and Inference](08_Holes-and-Inference.md) | **Next**: [Modals](10_Modals.md)

---

## Abstract

This chapter is the primary, canonical specification of functions and procedures in Cursive. It defines declaration forms, parameter and argument semantics (including named and default arguments), return semantics (including divergence), callable bodies (block- and expression-bodied forms), call evaluation and dispatch, type-scope procedure calls via `::`, closures and function pointers, higher-order programming, recursion, FFI and unsafe functions, the entry point, compile-time functions, and attributes affecting callables.

**Definition 9.1 (Functions and Procedures):** A **function** or **procedure** (collectively, a **callable**) is a declared computation with a name, parameter list, return type, and effect signature. Functions are pure or effectful callables declared at module scope or as associated functions within a type's scope (without a `self` parameter). Procedures are type-scope callables with an explicit receiver parameter `self: perm Self`, invoked using the scope operator `::`. This part is the authoritative specification for callable declarations, bodies, arguments/parameters, call semantics, dispatch/resolution, defaults and named arguments, expression-bodied forms, comptime functions, unsafe functions, FFI function declarations, and the entry point.

Where other parts remain authoritative (e.g., function types in Part II §2.10, contracts/effects in Part VII), this chapter integrates those rules at callable boundaries.

**Key components**:
- Function and procedure declarations (§9.2, §9.3)
- Parameters: positional, named, defaults, variadics (§9.4)
- Return semantics: single, tuple, divergence (§9.5)
- Call syntax and dispatch: module-scope, type-scope `::`, resolution algorithm (§9.6)
- Closures and function pointers (§9.7)
- Higher-order functions and effect polymorphism (§9.8)
- Argument evaluation order and binding (§9.9)
- Recursion: direct and mutual (§9.10)
- Integration with regions, modals, metaprogramming, unsafe (§9.11)
- Attributes: optimization, FFI, diagnostic (§9.12)
- Complete diagnostic catalog (§9.14)

**Key design decisions and conventions**:
- Function types use arrow notation `(T₁, …, Tₙ) → U ! ε` (the legacy `map(...)` alias is deprecated and removed from grammar).
- Scope operator `::` is used for navigation and invocation within a scope (modules, types, effects). The dot `.` is reserved for field access only.
- No ad-hoc overloading; names denote a single declaration per scope (arity must match exactly).
- UFCS (Uniform Function Call Syntax) is not supported; use the pipeline operator `=>` for chaining.
- Default parameters are allowed (must be const-evaluable). Named arguments are allowed with positional-first rules.
- Caller-side permission weakening only (`own → mut → imm`); moves consume; callees cannot upgrade permissions.
- Expression-bodied callables provide concise syntax; blocks remain explicit-result only (no implicit returns).

**Conformance:** See Part I §1.5–1.7 for conformance criteria. Implementations MUST support all normative features specified in this chapter. Effect checking, permission checking, and type checking at call boundaries are REQUIRED. Tail call optimization is NOT guaranteed unless `[[must_tail]]` is specified.

---

**Definition 9.2 (Foundations):** This section establishes the scope, dependencies, principles, and notational conventions for Part IX.

## 9.0 Foundations

### 9.0.1 Scope

This chapter provides complete normative rules for:

- **Callable declarations**: Functions (module-scope and associated), procedures (type-scope with `self`)
- **Parameter kinds**: Positional, named (after `*`), defaults (const-evaluable), variadics (FFI-only)
- **Return semantics**: Single values, tuples, divergence via never type `!`, ownership transfer
- **Body forms**: Block-bodied (explicit `result`), expression-bodied (`= expr`), control flow completeness
- **Call formation**: Argument evaluation order (receiver first, then left-to-right), type checking, effect propagation
- **Dispatch resolution**: Module-scope calls, type-scope procedure calls via `::`, desugaring, static resolution algorithm
- **Closures**: Capture semantics, minimal-permission capture, move closures, region escape analysis
- **Function pointers**: Zero-capture callables, coercion from non-capturing closures
- **Higher-order functions**: Effect-polymorphic functionals, functions as parameters/returns
- **Recursion**: Direct and mutual recursion, forward references via two-phase compilation
- **Special callables**: Unsafe functions, extern/FFI declarations, entry point (`main`), comptime functions
- **Attributes**: Optimization hints, FFI control, diagnostic configuration
- **Integration**: Interaction with regions, modal types, metaprogramming, unsafe code

### 9.0.2 Cross-Part Dependencies

This chapter depends on: Part I (notation, judgments, conformance, lexical structure); Part II (function types §2.10, never type §2.1.5, effect polymorphism §2.9.4, tuples §2.8, modals §2.3); Part III (two-phase compilation §3.1.3, const evaluation §3.3, name resolution §3.11, regions §3.9.2); Part IV (permission lattice §4.2, moves §4.5, call-boundary checks §4.9, closure captures §4.10); Part V (call syntax §5.4, closures §5.10, pipeline §5.5.9, evaluation order §5.2, explicitness §5.3); Part VI (`return` §6.3, `defer` §6.9, blocks §6.2, control flow §6.7); Part VII (contract clauses §7.3-7.7, effect availability §7.6); Part X (modal states §10.3, state-dependent procedures §10.5); Part XI (comptime contexts §11.4-11.5); Part XIII (region escape §13.3-13.4, thread safety §13.7); Part XV (unsafe §15.3, FFI §15.5-15.6); Appendix A (grammar).

### 9.0.3 Design Principles

This chapter enforces Cursive's core principles:

1. **Explicit over implicit** — Parameter and return types are required (except closure returns in clear contexts); required explicitness from Part V applies (block `result`, typed match bindings); defaults must be const-evaluable; effect usage is declared in signatures (`uses ε`).

2. **Local reasoning** — Permissions and effects are checked at call boundaries; dispatch is static and scope-based (predictable resolution); no ad-hoc overloading (one name per scope).

3. **Zero-cost abstractions** — All safety checks occur at compile time; static dispatch by default; no runtime overhead for permission checking; monomorphization for generics; closures with captures are explicit (no hidden allocations).

4. **Determinism** — Receiver is evaluated first, then arguments strictly left-to-right (exception: `&&`/`||` short-circuit per Part V §5.2); defaults evaluate in textual order after binding provided arguments; named arguments evaluate in textual order.

5. **Simple ownership** — Caller may implicitly weaken permission (`own → mut → imm`); passing to `own` parameter consumes binding (move); callees cannot upgrade permissions; return of `own` transfers ownership to new binding in caller.

6. **Fail-fast compilation** — Missing arguments, unknown named arguments, insufficient permissions, unavailable effects, and arity mismatches are hard errors (no runtime fallback).

### 9.0.4 Mathematical Foundations

Callable semantics are formalized using standard type-theoretic judgments:

**Typing judgment for callables:**
```
Γ ⊢ f : (τ₁, …, τₙ) → τᵣ ! ε
```
Read as: "In context Γ, callable f has function type accepting parameters of types τ₁ through τₙ, returning type τᵣ, and performing effects ε."

**Call typing judgment:**
```
Γ ⊢ f(e₁, …, eₙ) : τᵣ ! ε'
```
Where ε' includes the callee's effects ε plus any effects from evaluating arguments e₁ through eₙ.

**Evaluation judgment:**
```
⟨f(v₁, …, vₙ), σ⟩ ⇓ ⟨v, σ'⟩
```
Read as: "Calling f with values v₁ through vₙ in store σ evaluates to value v with updated store σ'."

**Permission ordering:**
```
own ≥ mut ≥ imm
```
Caller may weaken (provide stronger permission than required); callee cannot upgrade.

**Effect subset relation:**
```
ε₁ ⊆ ε₂
```
Caller context must provide superset of callee requirements.

### 9.0.5 Notational Conventions

This part reuses judgment and metavariable conventions from Part I. Additional metavariables used throughout §9:

```
f, g            ∈ Name          (callable names: functions, procedures)
p               ∈ Name          (procedure names, type-scope)
x, y            ∈ Name          (parameter names)
T, U, τ, σ      ∈ Type          (types)
ε, ε'           ∈ Effect        (effect sets)
perm            ∈ {own, mut, imm}  (permissions)
recv            ∈ Expr          (receiver expression)
e, v            ∈ Expr          (expressions, values)
args            ∈ [Expr]        (argument list)
Γ               : Context       (typing context: names → types)
Δ               : RegionCtx     (region context)
Σ               : ModalCtx      (modal state context)
σ, σ'           : Store         (runtime store/heap)
```

**Inference rule format:**
```
[Rule-Name]
premise₁    premise₂    ...    premiseₙ
----------------------------------------
conclusion
```

**EBNF grammar snippets** are deltas to Appendix A, presented in fenced code blocks with `ebnf` language tag.

**Typing rules** follow standard natural deduction format with horizontal lines separating premises from conclusions.

**Evaluation rules** use small-step (`→`) or big-step (`⇓`) operational semantics as appropriate.

### 9.0.6 Organization

The remainder of Part IX is organized as follows:

- **§9.1** Overview — Terminology and quick examples
- **§9.2** Function Declarations — Module-scope and associated functions
- **§9.3** Procedure Declarations — Type-scope callables with `self` parameter
- **§9.4** Parameters and Arguments — Kinds, binding rules, permissions
- **§9.5** Return Semantics — Single, tuple, divergence, ownership
- **§9.6** Calls and Dispatch — Evaluation order, resolution, static/dynamic dispatch
- **§9.7** Closures and Function Pointers — Capture semantics, coercions
- **§9.8** Higher-Order Functions — Functionals, effect polymorphism, composition
- **§9.9** Argument Processing Details — Evaluation order, defaults, named arguments
- **§9.10** Recursion — Direct, mutual, termination considerations
- **§9.11** Integration with Other Features — Regions, modals, comptime, unsafe
- **§9.12** Attributes — Optimization, FFI, diagnostics
- **§9.13** Grammar Additions and Updates — Deltas to Appendix A
- **§9.14** Diagnostics — Complete error catalog with E-codes

---

## 9.1 Overview

### 9.1.1 Terminology

**Callable** — Function or procedure; any named declaration invocable with arguments. **Function** — Module-scope or type-scope callable without `self` parameter; called as `function_name(args)` or `Type::function_name(args)`. **Associated Function** — Function within type's scope, no `self`; commonly used for constructors. **Procedure** — Type-scope callable with explicit `self: perm Self`; invoked as `receiver::procedure_name(args)` (desugars to `Type::procedure_name(receiver, args)`). **Receiver** — Expression before `::`; bound to `self` parameter. **Closure** — Anonymous callable capturing environment variables. **Function Pointer** — Zero-sized callable value referencing named function or non-capturing closure. **Higher-Order Function** — Accepts or returns function-typed values. **Parameter/Argument** — Declared input / call-site expression. **Effect Signature** — Set of effects `ε` declared via `uses ε`. **Dispatch** — Static resolution of callable name to declaration with well-formedness verification.

### 9.1.2 Functions vs Procedures

**Functions**: No `self` parameter; invoked as `function_name(args)` or `Type::function_name(args)`. **Procedures**: With `self: perm Self`; invoked as `receiver::procedure_name(args)` (desugars to `Type::procedure_name(receiver, args)`). Separation provides clarity (`::` for scope navigation, `.` for field access), explicitness (no UFCS), and predictability.

### 9.1.3 First-Class Functions

Functions are first-class values: names denote callable values with function types (Part II §2.10); can be passed as arguments, returned from functions, stored in data structures; closures extend with environment capture.

### 9.1.4 Calling Model

**Call-by-value** with explicit permissions: arguments evaluated left-to-right after receiver; `imm` (read-only), `mut` (temporary mutable), `own` (consuming move); returned `own` transfers ownership. Effects checked at compile time: callee's `uses ε` ⊆ caller's available effects; effect polymorphism abstracts over effect sets.

### 9.1.5 Canonical Example

This comprehensive example demonstrates functions, procedures, permissions, defaults, closures, and effect polymorphism:

```cursive
// Record with associated functions (constructors) and procedures (self methods)
record Point {
    x: f64
    y: f64

    function new(x: f64, y: f64): Point { result Point { x, y } }
    function origin(): Point = Point { x: 0.0, y: 0.0 }

    procedure translate(self: mut Point, dx: f64, dy: f64) {
        self.x += dx; self.y += dy
    }

    procedure distance(self: Point, other: Point): f64 {
        let dx = self.x - other.x
        let dy = self.y - other.y
        result ((dx * dx) + (dy * dy)).sqrt()
    }
}

// Named and default parameters
function connect(host: string, port: i32 = 80, *, tls: bool = false): ()
    uses io.network
{
    // ... implementation
}

// Higher-order function with effect polymorphism
function map<T, U, ε>(xs: [T], f: (T) → U ! ε): [U]
    uses alloc.heap, ε
{
    let out = Vec::new()
    loop x in xs { out::push(f(x)) }
    result out::into_array()
}

// Usage examples
let p1 = Point::new(0.0, 0.0)                   // Associated function call
p1::translate(1.0, 1.0)                         // Procedure (mut permission)
let d = p1::distance(Point::origin())           // Procedure (imm permission)

connect("example.com")                          // Defaults: port=80, tls=false
connect("example.com", 443, tls: true)          // Named argument

let doubled = map([1, 2, 3], |x| -> x * 2)      // Pure closure
let multiplier = 10
let scaled = map([1, 2, 3], |x| -> x * multiplier)  // Capturing closure
```

### 9.1.6 Core Semantic Summary

Key behaviors (detailed in subsequent sections):

1. **Evaluation order** (§9.6.1, §9.9.1): Receiver first (if present), then arguments strictly left-to-right; defaults evaluate after binding provided arguments in textual order.

2. **Desugaring** (§9.6.2): `receiver::procedure(args)` → `Type::procedure(receiver, args)` where `Type` is the static type of `receiver`.

3. **Permission checking** (§9.2.5, §9.6.5): Receiver and arguments must provide at least the permission required by parameters; `own → mut → imm` weakening is implicit; moves consume bindings.

4. **Effect checking** (§9.6.4): Caller context must provide superset of callee effects plus argument-evaluation effects; effects declared via `uses ε` clause.

5. **Named arguments** (§9.4, §9.9): Follow all positional arguments; unknown/duplicate names are errors; binding is name-based after positional binding.

6. **Expression-bodied forms** (§9.5.2): `function f(...): T = expr` is sugar for `function f(...): T { result expr }`; `return` not allowed in expression-bodied forms.

7. **Resolution** (§9.6.3): Static, scope-based; no overloading (arity must match exactly); fully qualified names disambiguate.

8. **No UFCS**: Type-scope procedures require `::` syntax; pipeline operator `=>` is used for chaining instead of dot-chaining.

---

## 9.2 Function Declarations

### 9.2.1 Syntax

Function declarations introduce module-scope or type-scope callables without a `self` parameter. The canonical grammar resides in Appendix A; this subsection presents the relevant productions and deltas introduced by Part IX.

**Canonical grammar (from Appendix A):**

```ebnf
FunctionDecl   ::= Attribute* Visibility? "function" Ident
                   GenericParams? "(" ParamList? ")" ":" Type
                   WhereClause? ContractClause*
                   ("=" Expr | Block)

Visibility     ::= "pub" | "pub" "(" VisibilityRestriction ")"
GenericParams  ::= "<" GenericParamList ">"
GenericParamList ::= GenericParam ("," GenericParam)*
GenericParam   ::= TypeParam | ConstParam | EffectParam

ParamList      ::= Param ("," Param)* ("," StarParams)?
                 | StarParams

Param          ::= Ident ":" Permission? Type ("=" ConstExpr)?
StarParams     ::= "*" ("," NamedParam)*
NamedParam     ::= Ident ":" Permission? Type ("=" ConstExpr)?

Permission     ::= "own" | "mut" | "imm"

ContractClause ::= UsesClause | MustClause | WillClause
UsesClause     ::= "uses" EffectSet
MustClause     ::= "must" Expr
WillClause     ::= "will" Expr

Block          ::= "{" Stmt* "}"
```

**Delta: Expression-bodied form** (introduced by Part IX):

```ebnf
FunctionDecl   ::= Attribute* Visibility? "function" Ident
                   GenericParams? "(" ParamList? ")" ":" Type
                   (WhereClause? ContractClause* "=" Expr
                   | WhereClause? ContractClause* Block)
```

An expression-bodied function uses `= Expr` instead of a block body. The expression is equivalent to a block containing `result Expr`.

**Syntactic constraints:**

1. The return type annotation `": Type"` is REQUIRED for function declarations (unlike closures where it may be inferred).

2. Generic parameters (type, const, effect) may be declared and are instantiated at call sites via monomorphization (§9.2.4).

3. Parameters before `*` are positional; parameters after `*` are named-only and must be supplied by name at call sites (§9.4).

4. Default parameter expressions (`= ConstExpr`) must be compile-time evaluable (Part III §3.3). They may reference earlier parameters in the parameter list.

5. Permission annotations on parameters default to `imm` if omitted.

6. Contract clauses (`uses`, `must`, `will`) follow the rules of Part VII; `uses` declares required effects.

7. Where clauses constrain generic parameters (Part II §2.9).

**Well-formedness constraints:**

```
[Func-WF]
Γ ⊢ function f<generics>(params): T where_clause contract_clauses body
generics are well-formed and distinct
params are well-formed, positional before named, defaults are const-evaluable
T is a well-formed type
where_clause constraints are satisfiable
contract_clauses are well-formed (Part VII)
body has type T under extended context Γ, generics, params
--------------------------------------------------------
Γ ⊢ f : (param_types) → T ! ε
```

Where `ε` is the effect set from the `uses` clause or `∅` if pure.

**Examples:**

```cursive
// Simple pure function
function add(x: i32, y: i32): i32 {
    result x + y
}

// Function with default parameter
function greet(name: string, punctuation: string = "!"): string {
    result name + punctuation
}

// Function with named parameters
function connect(host: string, port: i32, *, timeout: i32 = 5000, retry: bool = true): Result<Connection, Error>
    uses io.network
{
    // ... implementation
}

// Generic function
function identity<T>(x: T): T {
    result x
}

// Effect-polymorphic function
function apply<T, U, ε>(x: T, f: (T) → U ! ε): U
    uses ε
{
    result f(x)
}

// Expression-bodied function
function square(x: i32): i32 = x * x

function max(a: i32, b: i32): i32 = if a > b { a } else { b }

// Associated functions in type scope are declared inside the record
record Point {
    x: f64
    y: f64

    function origin(): Point {
        Point { x: 0.0, y: 0.0 }
    }

    function from_tuple(t: (f64, f64)): Point =
        Point { x: t.0, y: t.1 }
}
```

### 9.2.2 Parameters and Arguments

#### Positional and Named Parameters

Parameters are classified based on their position relative to the `*` separator:

**Positional parameters** appear before `*` in the parameter list:
- Must be supplied positionally at call sites (by position, not name)
- May have defaults
- Evaluated left-to-right at call site

**Named parameters** appear after `*` in the parameter list:
- Must be supplied by name at call sites (using `name: expr` syntax)
- May have defaults
- Evaluated in textual order after positional arguments

**Separator `*`:**
- Marks the transition from positional to named section
- If present with no following parameters, all parameters are positional
- If absent, all parameters are positional

**Syntax rules:**

```
[Param-Syntax]
ParamList ::= Positional* ("," "*" ("," Named*)?)?
--------------------------------------------------------
positional section precedes named section
```

#### Default Parameters

Parameters (positional or named) may have default values:

**Syntax:** `param_name: Type = default_expr`

**Constraints:**

1. **Const-evaluability:** `default_expr` must be compile-time evaluable (Part III §3.3). This includes:
   - Literals
   - Const expressions
   - Calls to `comptime` functions (§9.2.9)
   - References to earlier parameters (see below)

2. **Forward references forbidden:** Default expressions may reference only parameters declared earlier (left-to-right) in the parameter list:

```
[Param-Default-Order]
parameter p at position i has default expression d
d references parameter q at position j
j < i
--------------------------------------------------------
default accepted

[Param-Default-Forward-Ref]
default expression references later parameter
--------------------------------------------------------
ERROR (E9F/P-0206): default cannot reference later parameter
```

3. **Evaluation timing:** Defaults are evaluated after binding all provided arguments, in textual order (§9.9.1).

**Examples:**

```cursive
// Simple default
function log(msg: string, level: string = "INFO"): () {
    println("[{}] {}", level, msg)
}

// Default referencing earlier parameter
function allocate(size: usize, align: usize = size): *mut u8 {
    // align defaults to size if not provided
    // ...
}

// Named parameter with default
function configure(host: string, *, port: i32 = 80, secure: bool = (port == 443)): Config {
    // secure defaults based on port
    Config { host, port, secure }
}

// Const function in default
comptime function default_buffer_size(): usize { 4096 }

function create_buffer(size: usize = default_buffer_size()): Buffer {
    // ...
}
```

**Invalid examples:**

```cursive
// ERROR: Default references later parameter
function invalid(x: i32 = y, y: i32): i32 {  // ERROR E9F/P-0206
    result x + y
}

// ERROR: Default is not const-evaluable
function invalid2(n: i32, buffer: [i32] = allocate_array(n)): () {  // ERROR E9F-0102
    // allocate_array is not a comptime function
}
```

#### Variadic Parameters (FFI-Only)

Variadic parameters using `...` are permitted ONLY in extern declarations (§9.2.7):

```ebnf
ExternParam ::= Ident ":" Type | "..."
```

Variadic parameters appear last in the parameter list. Calls to variadic functions are `unsafe` and require the `ffi.call` effect.

```
[Param-Variadic-Extern-Only]
variadic parameter ... appears in non-extern declaration
--------------------------------------------------------
ERROR (E9F-0601): variadic parameters allowed only in extern declarations
```

**Example:**

```cursive
extern "C" {
    function printf(fmt: *u8, ...): i32;
}

procedure call_printf()
    uses ffi.call
{
    unsafe {
        printf(c"Value: %d\n", 42)
    }
}
```

#### Argument Binding Rules

At call sites, arguments bind to parameters according to these rules:

**Order constraint:**

```
[Args-Order]
call f(positional_args..., named_args...)
--------------------------------------------------------
all positional arguments precede all named arguments
```

**Positional binding:**

```
[Bind-Positional]
positional arguments bind left-to-right to positional parameters
if fewer positional arguments than positional parameters:
    remaining positional parameters must have defaults or be filled by named args
--------------------------------------------------------
binding succeeds or error if required param unbound
```

**Named binding:**

```
[Bind-Named]
named argument name: expr
lookup name in callable's named parameter list
bind expr to that parameter
--------------------------------------------------------
binding succeeds or error (E9F/P-0201) if name not found

[Bind-Named-Duplicate]
same parameter name appears twice in named arguments
--------------------------------------------------------
ERROR (E9F/P-0202): duplicate named argument

[Bind-Named-Positional-Conflict]
named argument attempts to bind a positional parameter
--------------------------------------------------------
ERROR (E9F/P-0205): parameter is positional, not named
```

**Completeness:**

```
[Bind-Complete]
after binding positional and named arguments:
    every required parameter (no default) is bound
--------------------------------------------------------
call well-formed

[Bind-Incomplete]
required parameter remains unbound
--------------------------------------------------------
ERROR (E9F/P-0203): missing required argument
```

**Examples:**

```cursive
function example(a: i32, b: i32 = 10, *, c: i32 = 20, d: i32): i32 {
    result a + b + c + d
}

example(1, 2, c: 3, d: 4)       // OK: all explicit
example(1, d: 4)                 // OK: b and c use defaults
example(1, c: 3, d: 4)           // OK: b uses default
// example(1, 2, 3, 4)           // ERROR: c and d are named-only
// example(1)                    // ERROR E9F/P-0203: d is required
// example(a: 1, d: 4)           // ERROR E9F/P-0205: a is positional
// example(1, d: 4, d: 5)        // ERROR E9F/P-0202: duplicate d
// example(1, d: 4, e: 5)        // ERROR E9F/P-0201: e not found
```

### 9.2.3 Function Body Semantics

Function bodies define the computation performed when the function is called. Bodies may be block-form or expression-form.

#### Block-Bodied Functions

Block-bodied functions use a block `{ ... }` as the body. Blocks follow Part VI semantics:

**Value-producing blocks:**
- MUST end with `result expr` to produce a value
- Control flow analysis ensures all paths either `return`, `result`, or diverge (`!`)
- No implicit returns; explicitness is required

**Unit-producing blocks:**
- Blocks that don't produce a value have type `()`
- No `result` required for `()` return type
- May end with a statement or be empty

**Control flow completeness:**

```
[Body-Complete]
function f(...): T { body }
T ≠ ()
every control path in body ends with `return`, `result`, or diverges
--------------------------------------------------------
body well-formed

[Body-Incomplete]
function f(...): T { body }
T ≠ ()
some control path neither returns, results, nor diverges
--------------------------------------------------------
ERROR (E9F-0301): missing return on some control paths
```

**Return statement:**

The `return` statement (Part VI §6.3) immediately exits the function with the specified value:
- `return expr` exits with value of `expr`
- `return` (no expression) exits with `()` value (only valid for unit-returning functions)
- Deferred actions execute in LIFO order before return (Part VI §6.9)
- Contract postconditions (`will` clauses) are checked at return boundary

**Result expression:**

The `result` keyword (Part VI §6.2) produces the value of a block:
- `result expr` makes the enclosing block evaluate to `expr`
- Only valid as the final expression in a block
- Does not exit outer functions, only produces block value

**Divergence:**

Functions may diverge (never return) using the never type `!`:
- `panic()`, `abort()`, `loop { }` all have type `!`
- Never type coerces to any type (subtyping rule)
- Diverging functions must be explicitly typed with return type `!` (§9.5.5)

**Examples:**

```cursive
// Block with result
function abs(x: i32): i32 {
    if x < 0 {
        result -x
    } else {
        result x
    }
}

// Early return
function find_first<T>(xs: [T], predicate: (T) → bool ! ∅): Option<T> {
    loop x in xs {
        if predicate(x) {
            return Some(x)
        }
    }
    result None
}

// Unit return (no result needed)
procedure print_stats(values: [i32])
    uses io.write
{
    let sum = values::sum()
    let avg = sum / values.len()
    println("Sum: {}, Average: {}", sum, avg)
}

// Diverging function
function panic(msg: string): !
    uses panic
{
    std::process::abort()
}

// Multiple control paths, all complete
function sign(x: i32): i32 {
    if x < 0 {
        result -1
    } else if x > 0 {
        result 1
    } else {
        result 0
    }
}
```

**Invalid examples:**

```cursive
// ERROR: Missing return on else branch
function bad1(x: i32): i32 {    // ERROR E9F-0301
    if x > 0 {
        result x
    }
    // Missing else branch or final result
}

// ERROR: Result in middle of block
function bad2(x: i32): i32 {    // ERROR from Part VI
    result x
    println("After result")      // Unreachable
}
```

#### Expression-Bodied Functions

Expression-bodied functions provide concise syntax for simple functions:

**Syntax:**
```cursive
function f(params): T = expr
```

**Desugaring:**
```cursive
function f(params): T { result expr }
```

**Constraints:**

```
[Body-Expr-Form]
function f(...): T = expr
Γ, params ⊢ expr : T
--------------------------------------------------------
function body well-formed

[Body-Expr-No-Return]
expression-bodied function contains `return` statement
--------------------------------------------------------
ERROR (E9F-0302): `return` not allowed in expression-bodied callable
```

The `return` statement is forbidden because expression-bodied forms are syntactic sugar for `result`, not a function body that could have multiple exit points.

**Effect compatibility:**

Effects from evaluating `expr` must be covered by the function's `uses` clause:

```
[Body-Expr-Effects]
function f(...): T = expr   uses ε
Γ ⊢ expr : T ! ε_expr
ε_expr ⊆ ε
--------------------------------------------------------
effects satisfied
```

**Examples:**

```cursive
// Simple expression-bodied functions
function square(x: i32): i32 = x * x
function double(x: i32): i32 = x * 2
function negate(x: i32): i32 = -x

// Conditional expression
function abs(x: i32): i32 = if x < 0 { -x } else { x }

// Tuple return
function swap<T, U>(pair: (T, U)): (U, T) = (pair.1, pair.0)

// With effects
function read_config(path: string): Result<Config, Error>
    uses fs.read
=
    fs::read_to_string(path)::and_then(|s| -> Config::parse(s))

// Block expression (note: block itself needs result if value-producing)
function greeting(name: string): string = {
    let prefix = "Hello"
    result prefix + ", " + name + "!"
}
```

**Invalid examples:**

```cursive
// ERROR: return not allowed in expression-bodied form
function bad1(x: i32): i32 = return x    // ERROR E9F-0302

// ERROR: Effects not covered
function bad2(path: string): string =    // ERROR E9F/P-0405
    fs::read_to_string(path)              // fs.read effect not declared
```

### 9.2.4 Generic Functions and Instantiation

Generic functions abstract over types, const values, and effects using generic parameters. Generic parameters are instantiated at call sites via monomorphization.

#### Generic Parameter Kinds

**Type parameters** (`<T>`, `<T: Bound>`):
- Abstract over types
- May have trait bounds (contracts) constraining valid instantiations
- Inferred from argument types or explicitly provided

**Const parameters** (`<const N: usize>`):
- Abstract over compile-time constant values
- Must have integer or boolean type
- Used for array sizes, numeric constraints

**Effect parameters** (`<ε>`, `<ε ⊆ E>`):
- Abstract over effect sets
- May have upper bounds constraining which effects are permitted
- Inferred from context or explicitly provided

**Syntax:**

```ebnf
GenericParams    ::= "<" GenericParamList ">"
GenericParamList ::= GenericParam ("," GenericParam)*
GenericParam     ::= TypeParam | ConstParam | EffectParam

TypeParam        ::= Ident (":" Bounds)?
Bounds           ::= Contract ("+" Contract)*

ConstParam       ::= "const" Ident ":" Type

EffectParam      ::= Ident                          (* effect variable *)
                   | Ident "⊆" "{" EffectSet "}"    (* bounded effect variable *)
```

#### Monomorphization Semantics

Cursive uses **monomorphization** for generics:
- Each distinct instantiation generates specialized code
- No runtime polymorphism or vtables for generic functions (unless contracts use witnesses)
- Type checking occurs after instantiation
- Code size increases with number of instantiations (implementation may optimize)

**Type rules:**

```
[Gen-Intro]
Γ, α: Type ⊢ body : τ
α ∉ FreeVars(Γ)
----------------------------------------
Γ ⊢ function f<α>(x: α): τ { body } : ∀α. (α) → τ ! ε

[Gen-Inst]
Γ ⊢ f : ∀α. (α) → τ ! ε
Γ ⊢ σ : Type
[σ/α]τ is well-formed
----------------------------------------
Γ ⊢ f<σ> : (σ) → [σ/α]τ ! ε

[Gen-Call-Infer]
Γ ⊢ f : ∀α. (α) → τ ! ε
Γ ⊢ e : σ
unification: α ≈ σ succeeds
----------------------------------------
Γ ⊢ f(e) : [σ/α]τ ! ε
```

**Effect polymorphism:**

Effect-polymorphic functions abstract over the effects of function-typed parameters:

```
[Gen-Effect-Poly]
Γ, ε: Effect ⊢ body : τ
body uses ε
----------------------------------------
Γ ⊢ function f<ε>(g: (T) → U ! ε): τ uses ε { body } : ∀ε. (((T) → U ! ε)) → τ ! ε
```

At instantiation, the effect variable is replaced with the concrete effect set of the provided function.

#### Type Inference

Generic instantiations are inferred when possible:

**Inference algorithm:**
1. Collect constraints from argument types
2. Unify generic parameters with concrete types
3. Check bounds are satisfied after substitution
4. If inference fails or is ambiguous, require explicit instantiation

```
[Gen-Inference-Failure]
call to generic function f<α, β, ...>
type inference cannot determine α, β, ... uniquely
--------------------------------------------------------
ERROR (E9F/P-0413): type parameter inference failed; provide explicit types
```

**Examples with inference:**

```cursive
// Simple generic function
function identity<T>(x: T): T {
    result x
}

let a = identity(42)           // T inferred as i32
let b = identity<string>("hi") // T explicitly i32

// Generic with multiple parameters
function pair<T, U>(first: T, second: U): (T, U) {
    result (first, second)
}

let p = pair(1, "hello")       // T=i32, U=string inferred

// Generic with bounds
function max<T: Ordered>(a: T, b: T): T {
    result if a > b { a } else { b }
}

let m = max(10, 20)            // T=i32 inferred, i32: Ordered required

// Effect-polymorphic function
function apply<T, U, ε>(x: T, f: (T) → U ! ε): U
    uses ε
{
    result f(x)
}

let squared = apply(5, |x| -> x * x)                    // ε = ∅
let printed = apply(5, |x| -> { println("{}", x); x })  // ε = {io.write}

// Const generic
function create_array<const N: usize, T>(value: T): [T; N] {
    // Implementation creates array of size N
    // ...
}

let arr = create_array<10, i32>(0)  // Explicit N, T inferred
```

**Examples requiring explicit instantiation:**

```cursive
function collect<T>(): Vec<T> {
    Vec::new()
}

// let v = collect()       // ERROR E9F/P-0413: cannot infer T
let v = collect<i32>()      // OK: T explicitly i32

function from_parts<T>(parts: impl Iterator<T>): Vec<T> {
    // ...
}

// Type ascription helps inference
let v: Vec<i32> = from_parts(iter)  // T inferred from expected type
```

#### No Generic Defaults on Functions

Unlike type declarations, generic function parameters do NOT support defaults:

```
[Gen-Func-No-Defaults]
function declaration includes default value for generic parameter
--------------------------------------------------------
ERROR (E9F-0104): generic function parameters cannot have defaults
```

**Rationale:** Defaults on functions complicate call-site inference and can lead to ambiguity. Use separate functions or explicit instantiation instead.

### 9.2.5 Permissions at Call Boundaries

Function parameters may declare required permissions (`own`, `mut`, `imm`). Call sites must provide arguments with at least the required permission strength according to the permission lattice `own ≥ mut ≥ imm`.

#### Permission Lattice

```
own   (exclusive ownership, can consume)
 ≥
mut   (exclusive mutable access, cannot consume)
 ≥
imm   (shared immutable access)
```

Caller may implicitly **weaken** (provide stronger permission than required):
- `own` satisfies `mut` or `imm`
- `mut` satisfies `imm`

Caller CANNOT **upgrade** (provide weaker permission than required):
- `imm` does NOT satisfy `mut` or `own`
- `mut` does NOT satisfy `own`

#### Default Permission

If a parameter omits a permission annotation, it defaults to `imm`:

```cursive
function f(x: i32): i32        // x has imm permission by default
```

#### Permission Checking Rules

```
[Perm-Call-Exact]
argument has permission perm
parameter requires perm
--------------------------------------------------------
call permitted

[Perm-Call-Weaken]
argument has permission perm₁
parameter requires perm₂
perm₁ ≥ perm₂
--------------------------------------------------------
call permitted (implicit weakening)

[Perm-Call-No-Upgrade]
argument has permission perm₁
parameter requires perm₂
perm₁ < perm₂
--------------------------------------------------------
ERROR (E9F/P-0204): insufficient permission (cannot upgrade)
```

#### Move Semantics for `own`

Passing an argument to an `own` parameter **consumes** the caller's binding:

```
[Perm-Call-Own-Consume]
argument a bound with own permission
parameter p requires own T
--------------------------------------------------------
move occurs; binding a becomes invalid after call
```

After the move, the binding cannot be used again (enforced by move checking in Part IV).

**Return with `own`:**

Returning `own T` transfers ownership to the caller as a new binding:

```
[Perm-Return-Own]
function returns own T
--------------------------------------------------------
caller receives new binding with own permission
```

#### Example

```cursive
function consume(s: own String): usize { result s.len() }
procedure increment(x: mut i32) { x += 1 }
function read_value(x: imm i32): i32 { result x }

let a: own i32 = 42
read_value(a)             // OK: own → imm weakening
increment(mut a)          // OK: own → mut weakening (explicit `mut` annotation)
let b = a                 // OK: a still valid (not consumed)

let s: own String = String::from("hello")
consume(move s)           // OK: move consumes binding
// let c = s.len()        // ERROR: s consumed, no longer valid

let x: imm i32 = 10
// increment(mut x)       // ERROR E9F/P-0204: cannot upgrade imm to mut
read_value(x)             // OK: imm → imm exact match
```

#### Permission Annotations in Calls

When weakening, the caller must use explicit permission annotations:
- `mut binding` for weakening to `mut`
- `move binding` for consuming to `own`

```cursive
let x: own i32 = 10
increment(mut x)       // Explicit `mut` weakens own to mut
let y: own String = String::from("hi")
consume(move y)        // Explicit `move` consumes binding
```

Without the annotation, Cursive would attempt to pass with the binding's declared permission, which might not match the parameter's requirement.

### 9.2.6 Unsafe Functions

Unsafe functions are callables whose correct use cannot be fully verified by the type, permission, and effect systems. They represent operations that require manual verification of safety invariants.

#### Declaration Syntax

```ebnf
UnsafeFunctionDecl ::= Attribute* Visibility? "unsafe" "function" Ident
                        GenericParams? "(" ParamList? ")" ":" Type
                        WhereClause? ContractClause* Block
```

**The `unsafe` keyword precedes `function` in the declaration.**

#### Semantics

**Declaring unsafe:** The `unsafe` modifier on a function indicates:
- The function body may perform operations that are not statically checkable (raw pointer dereference, type transmutation, inline assembly)
- The function may establish invariants that the type system cannot express
- Callers must manually ensure preconditions are met

**Calling unsafe:** Calls to unsafe functions MUST appear within an `unsafe { ... }` block:

```
[Call-Unsafe-Context]
call to unsafe function f
call site is within unsafe { ... } block
--------------------------------------------------------
call permitted

[Call-Unsafe-No-Context]
call to unsafe function f
call site NOT within unsafe { ... } block
--------------------------------------------------------
ERROR (E9F-0101): call to unsafe function requires `unsafe` block
```

**Contract clauses still apply:**

Unsafe functions may still have contract clauses (`uses`, `must`, `will`):
- `uses` declares required effects
- `must` declares preconditions (checked statically or at runtime depending on verification mode)
- `will` declares postconditions

These clauses are not bypassed by `unsafe`; they document the function's contract.

**Effect declarations:**

Unsafe operations (pointer dereference, FFI calls) may require specific effects:
- `unsafe.ptr` — raw pointer operations
- `ffi.call` — foreign function calls

These must be declared in the `uses` clause.

#### Example

```cursive
// Unsafe function for raw pointer operations
unsafe function deref<T>(ptr: *T): T
    uses unsafe.ptr
    must ptr != null
{
    result *ptr
}

// Safe wrapper pattern - unsafe primitive wrapped in safe interface
unsafe function raw_alloc(size: usize): *mut u8
    uses unsafe.ptr, alloc.heap
{ /* ... */ }

function allocate<T>(): own Box<T>
    uses alloc.heap
{
    unsafe {
        let ptr = raw_alloc(size_of::<T>())
        result Box::from_raw(ptr as *mut T)
    }
}

// Usage: unsafe functions require unsafe blocks
procedure use_unsafe() {
    let ptr: *i32 = get_pointer()
    unsafe { let value = deref(ptr) }
    // deref(ptr)  // ERROR E9F-0101: requires unsafe block
}

let b = allocate::<i32>()  // Safe wrapper, no unsafe block needed
```

### 9.2.7 Extern / FFI Declarations

Extern declarations introduce bindings to foreign (non-Cursive) functions, enabling FFI (Foreign Function Interface) interoperability.

#### Syntax

```ebnf
ExternBlock         ::= "extern" StringLit? "{" ExternItem* "}"
ExternItem          ::= ExternFunctionDecl | ExternStaticDecl
ExternFunctionDecl  ::= Attribute* Visibility? "function" Ident
                         "(" ExternParamList? ")" ":" Type ";"
ExternParamList     ::= ExternParam ("," ExternParam)*
ExternParam         ::= Ident ":" Type | "..."
```

**Calling convention:**
- Default is `extern "C"` (C calling convention)
- Other conventions may be specified: `extern "system"`, `extern "rust"`, etc. (implementation-defined)

**Variadic parameters:**
- `...` is permitted ONLY in extern declarations
- Represents variable-length argument list (C-style varargs)

**Semicolon termination:**
- Extern function declarations end with `;` (no body)

#### Semantics

**Extern functions are inherently unsafe:**

Calling an extern function requires:
1. An `unsafe { ... }` block
2. The `ffi.call` effect in the caller's `uses` clause

```
[Call-Extern]
call to extern function f
--------------------------------------------------------
call MUST be in unsafe block AND require uses ffi.call

[Call-Extern-No-Unsafe]
call to extern function f not in unsafe block
--------------------------------------------------------
ERROR (E9F-0601): extern function call requires unsafe block

[Call-Extern-No-Effect]
call to extern function f without uses ffi.call
--------------------------------------------------------
ERROR (E9F-0602): extern function call requires uses ffi.call
```

**Type compatibility:**

Extern function types must use FFI-safe types:
- Primitive integers: `i8`, `i16`, `i32`, `i64`, `isize`, `u8`, `u16`, `u32`, `u64`, `usize`
- Floating point: `f32`, `f64`
- Raw pointers: `*T`, `*mut T`
- `()` for void
- C-compatible structs (marked with `[[repr(C)]]`)

Complex Cursive types (enums, tuples, closures, references) are NOT FFI-safe without explicit representation control.

#### FFI Attributes

**Name mangling control:**

```cursive
[[no_mangle]]              // Preserve symbol name exactly
[[export_name("sym")]]     // Set exported symbol name
[[link_name("sym")]]       // Set linked/imported symbol name
```

**Example usage:**

```cursive
// Export Cursive function to C
[[no_mangle]]
function cursive_add(a: i32, b: i32): i32 {
    result a + b
}
// Exported as symbol "cursive_add" (no name mangling)

// Link to C function with different name
extern "C" {
    [[link_name("platform_specific_malloc")]]
    function malloc(size: usize): *mut u8;
}
```

**Representation control:**

```cursive
[[repr(C)]]
record Point {
    x: f64,
    y: f64
}
// Guaranteed C-compatible layout
```

#### Example

```cursive
// C FFI with variadic function
extern "C" {
    function printf(fmt: *u8, ...): i32;
    function puts(s: *u8): i32;
}

procedure demo_ffi(value: i32)
    uses ffi.call
{
    unsafe {
        puts(c"Hello from Cursive!\n")
        printf(c"Value: %d\n", value)
    }
}

// Export Cursive function for C interop
[[no_mangle, export_name("cursive_process")]]
function process_data(data: *u8, len: usize): i32 {
    result 0  // Status code
}
```

#### String and Pointer Interop

**C strings:**
- Use `c"..."` syntax for null-terminated C strings (type `*u8`)
- Cursive `string` is NOT FFI-safe (fat pointer with length)

**Pointer conversions:**
- Cursive references (`&T`, `&mut T`) are NOT FFI-safe
- Must use raw pointers (`*T`, `*mut T`)
- Convert references to pointers with `&x as *T` or `&mut x as *mut T`

**Example:**

```cursive
extern "C" {
    function process_buffer(buf: *mut u8, len: usize): i32;
}

procedure call_extern(buffer: mut [u8])
    uses ffi.call, unsafe.ptr
{
    unsafe {
        let status = process_buffer(&mut buffer[0] as *mut u8, buffer.len())
    }
}
```

### 9.2.8 Entry Point (Main Function)

Every executable Cursive program must define an entry point function named `main`. Execution begins at `main` after static initialization.

#### Permitted Main Signatures

Two canonical forms are supported:

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

**Return value:** Both forms return `i32`, which becomes the process exit code:
- `0` conventionally indicates success
- Non-zero indicates error (meaning is platform/application-specific)

**Effect constraints:**

`main` may be pure (`function`) or effectful (`procedure`):

```cursive
// Pure main (unusual but permitted)
function main(): i32 {
    result 0
}

// Effectful main (typical)
procedure main()
    uses io.write, io.read, fs, alloc.heap, process.spawn
: i32 {
    println("Hello, world!")
    result 0
}
```

If `main` is effectful, the implementation provides the required effect implementations (standard library implementations for `io`, `fs`, etc.).

#### Initialization Order

1. **Static initialization**: Module-level constants and statics are initialized in dependency order (Part III §3.8)
2. **Main execution**: `main` function is called
3. **Deferred cleanup**: Deferred actions (§6.9) and static destructors run
4. **Process exit**: Exit code from `main` is returned to OS

#### Uniqueness Constraint

```
[Main-Unique]
program contains multiple definitions of `main`
--------------------------------------------------------
ERROR (E9F-0105): multiple definitions of entry point `main`

[Main-Missing]
executable program contains no definition of `main`
--------------------------------------------------------
ERROR (E9F-0106): entry point `main` not found
```

Libraries (non-executable compilation units) do NOT require `main`.

#### Example

```cursive
// Main with command-line arguments and effects
function main(args: [string]): i32
    uses io.write, fs
{
    if args.len() < 2 {
        eprintln("Usage: program <input>")
        result 1
    }

    let config = load_config(args[1]) match {
        Ok(c) => c,
        Err(e) => {
            eprintln("Config error: {}", e)
            return 1
        }
    }

    // Process with config...
    result 0
}
```

### 9.2.9 Compile-Time Functions (Comptime)

Comptime functions are callables that MUST be evaluable at compile time. They enable compile-time computation in const contexts.

#### Declaration Syntax

```ebnf
ComptimeFunctionDecl ::= Attribute* Visibility? "comptime" "function" Ident
                          GenericParams? "(" ParamList? ")" ":" Type
                          WhereClause? Block
```

The `comptime` keyword precedes `function`.

#### Constraints

**Body evaluability:**

The body of a comptime function MUST be compile-time evaluable (Part III §3.3):
- May call only other comptime functions or intrinsics
- May allocate only in the compile-time arena (not runtime heap)
- May not perform I/O or other runtime effects
- All control flow must be statically determinable for the provided arguments

```
[Comptime-Body]
body of comptime function is not compile-time evaluable
--------------------------------------------------------
ERROR (E9F-0102): comptime function body must be compile-time evaluable
```

**Effect purity:**

Comptime functions MUST have empty effect set (`uses ∅`):

```
[Comptime-Pure]
comptime function f
--------------------------------------------------------
ε(f) = ∅  (principal effect must be empty)

[Comptime-Has-Effects]
comptime function declares or infers non-empty effects
--------------------------------------------------------
ERROR (E9F-0107): comptime functions must be pure (uses ∅)
```

#### Call Contexts

Comptime functions may be called ONLY in compile-time contexts:
- Module-level const/static initializers
- Type parameter default values (on types, not functions)
- Array length expressions `[T; N]` where `N` is const
- Const generic arguments
- Default parameter expressions
- Inside `comptime { ... }` blocks

```
[Comptime-Call-Context]
call to comptime function appears outside compile-time context
--------------------------------------------------------
ERROR (E9F-0103): call to comptime function requires compile-time context
```

**Runtime calls forbidden:**

```cursive
comptime function double(x: i32): i32 {
    result x * 2
}

const VALUE: i32 = double(21)  // OK: const context

function runtime_call(n: i32): i32 {
    // let r = double(n)  // ERROR E9F-0103: not a compile-time context
    result n
}
```

#### Evaluation Semantics

**Memoization:** Implementations MAY cache results of comptime function calls with identical arguments (referential transparency required).

**Termination:** Comptime function calls must terminate within implementation-defined limits:
- Recursion depth limits
- Iteration count limits
- Computation time limits

If limits are exceeded, compilation fails:

```
[Comptime-Timeout]
comptime function evaluation exceeds implementation limits
--------------------------------------------------------
ERROR (E9F-0108): comptime evaluation exceeded limits
```

#### Example

```cursive
// Comptime function for compile-time computation
comptime function factorial(n: i32): i32 {
    if n <= 1 { result 1 }
    else { result n * factorial(n - 1) }
}

const FACT_10: i32 = factorial(10)          // Const initializer
type Buffer = [u8; factorial(4) * 256]      // Array length expression

function connect(host: string, timeout: i32 = factorial(5) * 100): () {
    // Default parameter uses comptime function
}

// ERROR: Runtime call to comptime function
function runtime(n: i32): i32 {
    // factorial(n)  // ERROR E9F-0103: requires compile-time context
    result n
}

// ERROR: Comptime with effects
// comptime function read_cfg(): string  // ERROR E9F-0107: must be pure
//     uses fs.read
// { ... }
```

### 9.2.10 Summary

See preceding subsections (§9.2.1-9.2.9) for detailed examples and diagnostics for each function declaration feature.

---

## 9.3 Procedure Declarations

Procedures are type-scope callables with an explicit receiver parameter `self`. They enable type-directed operations with permission-checked receivers and are invoked using the scope operator `::`.

### 9.3.1 Syntax

Procedures are declared within a type's scope (record, enum, contract implementation) and include a `self` parameter.

**Canonical grammar (from Appendix A):**

```ebnf
ProcedureDecl  ::= Attribute* Visibility? "procedure" TransitionSpec?
                    Ident GenericParams?
                    "(" ParamList? ")" (":" Type)?
                    WhereClause? ContractClause* Block

SelfParam      ::= "$"
                 | "self" ":" Permission? "Self"

TransitionSpec ::= "@" Ident "->" "@" Ident
```

**Delta: Expression-bodied form** (introduced by Part IX):

```ebnf
ProcedureDecl  ::= Attribute* Visibility? "procedure" Ident
                    GenericParams? "(" SelfParam ("," ParamList)? ")" ":" Type
                    (WhereClause? ContractClause* "=" Expr
                   | WhereClause? ContractClause* Block)
```

**The `$` shorthand:**
- `$` is syntactic sugar for `self: Self` (immutable receiver)
- Equivalent to `self: imm Self` explicitly
- Cannot use both `$` and an explicit `self: ...` in the same signature

**Syntactic constraints:**

1. The first parameter MUST be a `self` parameter (explicit or `$`)
2. The `self` parameter's type MUST be `Self` (the enclosing type)
3. Permission annotation on `self` defaults to `imm` if omitted
4. Remaining parameters follow the same rules as function parameters (§9.2.2)
5. Return type annotation is REQUIRED
6. Generic parameters may be declared (in addition to type's generics)
7. Contract clauses follow Part VII rules

**Well-formedness:**

```
[Proc-WF]
Γ ⊢ procedure p<generics>(self: perm Self, params): U where_clause contract_clauses body
T is the enclosing type declaration
generics are well-formed and distinct from T's generics
self has permission perm ∈ {own, mut, imm}
params are well-formed
U is well-formed type
where_clause constraints are satisfiable
contract_clauses are well-formed
body has type U under extended context Γ, self, params
--------------------------------------------------------
Γ ⊢ p : (perm Self, param_types) → U ! ε within type T
```

**Examples:**

```cursive
record Counter {
    value: i32

    // Immutable receiver (default)
    procedure get(self: Counter): i32 {
        result self.value
    }

    // Using $ shorthand
    procedure get_alt($): i32 {
        result self.value
    }

    // Mutable receiver
    procedure increment(self: mut Counter) {
        self.value += 1
    }

    procedure add(self: mut Counter, amount: i32) {
        self.value += amount
    }

    // Consuming receiver
    procedure into_value(self: own Counter): i32 {
        result self.value
    }

    // Expression-bodied procedure
    procedure double(self: Counter): i32 = self.value * 2

    // Generic procedure
    procedure map<F, ε>(self: Counter, f: (i32) → i32 ! ε): i32
        uses ε
    {
        result f(self.value)
    }

    // With contracts
    procedure set_positive(self: mut Counter, n: i32)
        must n > 0
        will self.value > 0
    {
        self.value = n
    }
}

record File {
    path: string

    procedure read_contents(self: File): Result<string, Error>
        uses fs.read
    {
        fs::read_to_string(self.path)
    }
}
```

### 9.3.2 Receiver Permissions

The receiver parameter's permission determines what operations are allowed and whether the receiver is consumed.

#### Permission Semantics

**`self: Self`** or **`self: imm Self`** (default):
- Read-only access to receiver fields
- Cannot mutate receiver
- Receiver not consumed; usable after call
- Most permissive for callers (any permission satisfies)

**`self: mut Self`**:
- Mutable access to receiver fields
- Can mutate receiver state
- Receiver not consumed; usable after call
- Requires caller provide `mut` or `own` permission

**`self: own Self`**:
- Consuming access; receiver moved into procedure
- Can mutate or consume receiver
- Receiver becomes invalid after call
- Requires caller provide `own` permission
- Typically used for transformations or destruction

#### Type Rules

```
[T-Proc-Receiver-Imm]
procedure p : (self: imm Self, ...) → U ! ε
Γ ⊢ recv : perm T
perm ∈ {own, mut, imm}
--------------------------------------------------------
Γ ⊢ recv::p(...) : U ! ε        (any permission satisfies imm)

[T-Proc-Receiver-Mut]
procedure p : (self: mut Self, ...) → U ! ε
Γ ⊢ recv : perm T
perm ∈ {own, mut}
--------------------------------------------------------
Γ ⊢ recv::p(...) : U ! ε        (own or mut satisfies mut)

[T-Proc-Receiver-Own]
procedure p : (self: own Self, ...) → U ! ε
Γ ⊢ recv : own T
--------------------------------------------------------
Γ ⊢ recv::p(...) : U ! ε        (only own satisfies own)
recv becomes invalid after call
```

#### Examples

```cursive
record Box<T> {
    value: T

    // Immutable receiver - any permission works
    procedure peek(self: Box<T>): T {
        result self.value
    }

    // Mutable receiver - requires mut or own
    procedure set(self: mut Box<T>, new_value: T) {
        self.value = new_value
    }

    // Consuming receiver - requires own
    procedure unwrap(self: own Box<T>): T {
        result self.value
    }
}

let b1: own Box<i32> = Box { value: 10 }
let b2: mut Box<i32> = Box { value: 20 }
let b3: imm Box<i32> = Box { value: 30 }

b1::peek()  // OK: own → imm weakening
b2::peek()  // OK: mut → imm weakening
b3::peek()  // OK: imm → imm exact match

b1::set(42)      // OK: own → mut weakening
b2::set(42)      // OK: mut → mut exact match
// b3::set(42)   // ERROR E9F/P-0402: imm cannot satisfy mut

let val1 = b1::unwrap()  // OK: own → own, b1 consumed
// let val2 = b2::unwrap()  // ERROR E9F/P-0402: mut cannot satisfy own
// let val3 = b3::unwrap()  // ERROR E9F/P-0402: imm cannot satisfy own
// let x = b1.value         // ERROR: b1 consumed by unwrap
```

### 9.3.3 Procedure Body Semantics

Procedure bodies follow the same semantics as function bodies (§9.2.3):

**Block-bodied procedures:**
- Must end with `result expr` for value-producing procedures
- Unit-returning procedures (`→ ()`) don't require `result`
- All control paths must `return`, `result`, or diverge
- `return` statement exits immediately with deferred cleanup
- `self` is an immutable binding; reassignment forbidden (even with `mut` or `own` permission)

- Syntax: `procedure p(...): U = expr` (declared inside the type's body)
- Desugars to `procedure p(...): U { result expr }`
- `return` not allowed in expression-bodied form

**Operations on `self`:**

The `self` parameter has special semantics that preserve object identity and ensure predictable behavior:

##### 9.3.3.1 Self Reassignment (Forbidden)

**Rule**: The `self` binding itself **cannot be reassigned**, regardless of permission level.

**Rationale**: Reassigning `self` would change the object identity visible to the caller, violating the principle that method calls operate on the same object throughout the call. This ensures:
- Predictable object identity for debugging and reasoning
- No confusion about which object is being modified
- Consistent semantics with implicit receivers in other languages

**Error E9F-9301**: Cannot reassign `self` parameter
The `self` binding is immutable. To replace the object, return a new instance from the procedure.

```cursive
record Counter {
    value: i32

    procedure reset(self: mut Counter) {
        self.value = 0        // ✅ OK: mutate field

        self = Counter { value: 0 }  // ❌ ERROR E9F-9301: cannot reassign self binding
    }
}
```

##### 9.3.3.2 Field Mutation (Allowed with `mut`)

With `self: mut Self`, individual fields can be mutated:

```cursive
record Point {
    x: f64,
    y: f64

    procedure translate(self: mut Point, dx: f64, dy: f64) {
        self.x += dx  // ✅ OK: mutate field
        self.y += dy  // ✅ OK: mutate field
    }

    procedure set_to_origin(self: mut Point) {
        self.x = 0.0  // ✅ OK: mutate field
        self.y = 0.0  // ✅ OK: mutate field

        self = Point { x: 0.0, y: 0.0 }  // ❌ ERROR E9F-9301: cannot reassign self
    }
}
```

##### 9.3.3.3 Partial Moves (Allowed with `own`)

With `self: own Self`, fields can be moved out (partial move):

```cursive
record Box<T> {
    value: T

    procedure unbox(self: own Box<T>): T {
        result self.value  // ✅ OK: move field out, self is consumed
    }

    procedure replace(self: own Box<T>, new_value: T): Box<T> {
        let _old = self.value  // ✅ OK: move field out
        result Box { value: new_value }

        self = Box { value: new_value }  // ❌ ERROR E9F-9301: cannot reassign self
    }
}
```

##### 9.3.3.4 Allowed Operations Summary

The following operations ARE allowed on `self`:

| Operation | `self: Self` | `self: mut Self` | `self: own Self` | Example |
|-----------|--------------|------------------|------------------|---------|
| Field access | ✅ | ✅ | ✅ | `self.field` |
| Field mutation | ❌ | ✅ | ✅ | `self.field = value` |
| Field move | ❌ | ❌ | ✅ | `let x = self.field` |
| Method calls | ✅ | ✅ | ✅ | `self::method()` |
| Borrow fields | ✅ | ✅ | ✅ | `&self.field` |
| **Self reassignment** | **❌** | **❌** | **❌** | `self = other` |
| **Self shadowing** | **❌** | **❌** | **❌** | `let self = other` |

**Note**: All permissions (imm, mut, own) forbid reassigning or shadowing the `self` binding itself.

##### 9.3.3.5 Shadowing and Pattern Matching

**Rule**: The `self` binding **cannot be shadowed** by inner `let` or `var` declarations.

**Rationale**: Shadowing `self` would create confusion about which object is being operated on, violating local reasoning principles.

```cursive
record Point {
    x: f64, y: f64

    // procedure bad(self: Point) { let self = ...; }  // ERROR E9F-9302: cannot shadow self

    procedure describe(self: Point): String {
        let Point { x, y } = self  // OK: destructure fields, self remains valid
        result format("Point at ({}, {})", x, y)
    }
}
```

**Error E9F-9302**: Cannot shadow `self` parameter
The `self` binding is reserved and cannot be shadowed by inner declarations.

**Example:**

```cursive
record Stack<T> {
    items: Vec<T>

    procedure push(self: mut Stack<T>, item: T) {       // mut permission
        self.items::push(item)
    }

    procedure into_vec(self: own Stack<T>): Vec<T> {    // own permission (consuming)
        result self.items
    }

    procedure len(self: Stack<T>): usize = self.items.len()  // imm permission (expression-bodied)
}
```

### 9.3.4 Effect Checking

Procedures with effects must declare them via `uses` clauses. Effect checking follows Part VII rules:

**Effect sources:**
- Operations in procedure body
- Calls to other effectful procedures or functions
- Field accesses that trigger computed properties (if applicable)

**Effect verification:**

```
[Proc-Effect-Check]
procedure p : (self: perm Self, ...) → U uses ε_decl
body has effects ε_body
ε_body ⊆ ε_decl
--------------------------------------------------------
procedure effect signature valid

[Proc-Effect-Call]
Γ ⊢ recv::p(...) : U ! ε_proc
Γ.effects ⊇ ε_proc
--------------------------------------------------------
call permitted
```

**Example:**

```cursive
record Logger {
    file: File

    procedure log(self: mut Logger, msg: string)
        uses io.write
    {
        self.file::write_line(msg)
    }

    // Effect-polymorphic procedure
    procedure log_with<F, ε>(self: mut Logger, f: () → string ! ε)
        uses io.write, ε
    {
        self::log(f())  // Combines io.write + ε
    }
}
```

### 9.3.5 Contract Clauses Integration

Procedures may have contract clauses (`must` preconditions, `will` postconditions) that are checked at call boundaries.

**Preconditions (`must`):**
- Checked before procedure body executes
- May reference `self` and parameters
- Caller responsible for ensuring preconditions hold

**Postconditions (`will`):**
- Checked after procedure body completes (before return to caller)
- May reference `self` (post-state), parameters, and `result` (return value)
- Procedure responsible for ensuring postconditions hold

**Type rules with contracts:**

```
[Proc-Contract]
procedure p : (self: perm Self, params) → U
    must P
    will Q
--------------------------------------------------------
at call site: P must hold
at return: Q must hold
```

**Example:**

```cursive
record BoundedCounter {
    value: i32
    max: i32

    procedure set(self: mut BoundedCounter, n: i32)
        must n >= 0
        must n <= self.max
        will self.value == n
    {
        self.value = n
    }

    procedure increment(self: mut BoundedCounter): bool
        will result == (self.value < self.max)
    {
        if self.value < self.max {
            self.value += 1
            result true
        } else {
            result false
        }
    }
}
```

### 9.3.6 Modal State Transitions

Procedures on modal types (Part X) may specify state transitions. The receiver's modal state determines which procedures are callable.

**Example:**

```cursive
modal File {
    states { Closed, Open }

    procedure File@Closed::open(self: own File@Closed): Result<File@Open, Error>
        uses fs.open
    {
        // Transition: Closed → Open
    }

    procedure File@Open::read(self: File@Open, buf: mut [u8]): Result<usize, Error>
        uses fs.read
    {
        // Remains in Open state
    }

    procedure File@Open::close(self: own File@Open): File@Closed
        uses fs.close
    {
        // Transition: Open → Closed
    }
}
```

**Type rules with modals:**

```
[Proc-Modal]
Γ, Σ ⊢ recv : T@S₁
procedure p : (self: own T@S₁) → T@S₂
--------------------------------------------------------
Γ, Σ' ⊢ recv::p() : T@S₂        (where Σ' tracks S₂)
```

State transitions are verified at compile time using typestate analysis (Part X §10.3).

### 9.3.7 Associated Functions vs Instance Procedures

It's important to distinguish:

**Associated functions** (covered in §9.2):
- Declared inside a type's body WITHOUT a `self` parameter
- Called as `Type::function_name(args)`
- Common for constructors, factory functions, type-level operations

**Instance procedures** (this section):
- Declared inside a type's body WITH a `self` parameter
- Called as `receiver::procedure_name(args)` or `Type::procedure_name(receiver, args)`
- Operate on instances of the type

**Examples:**

```cursive
record Point {
    x: f64
    y: f64

    // Associated functions (no self)
    function origin(): Point {
        Point { x: 0.0, y: 0.0 }
    }

    function new(x: f64, y: f64): Point {
        Point { x, y }
    }

    function from_polar(r: f64, theta: f64): Point {
        Point { x: r * theta.cos(), y: r * theta.sin() }
    }

    // Instance procedures (with self)
    procedure translate(self: mut Point, dx: f64, dy: f64) {
        self.x += dx
        self.y += dy
    }

    procedure distance(self: Point, other: Point): f64 {
        let dx = self.x - other.x
        let dy = self.y - other.y
        result ((dx * dx) + (dy * dy)).sqrt()
    }

    procedure magnitude(self: Point): f64 {
        self::distance(Point::origin())
    }
}

// Usage
let p1 = Point::origin()           // Associated function
let p2 = Point::new(3.0, 4.0)      // Associated function
let p3 = Point::from_polar(5.0, 0.927)  // Associated function

p1::translate(1.0, 1.0)            // Instance procedure
let d = p1::distance(p2)           // Instance procedure
let m = p2::magnitude()            // Instance procedure

// Fully qualified (equivalent)
Point::translate(p1, 1.0, 1.0)     // Explicit receiver
let d2 = Point::distance(p1, p2)   // Explicit receiver
```

### 9.3.8 Summary

See preceding subsections (§9.3.1-9.3.7) for detailed examples and diagnostics for procedure declarations, receiver permissions, state transitions, and contract integration.

---

## 9.4 Parameters and Arguments

This section consolidates parameter semantics from §9.2.2 and provides the complete argument binding algorithm.

### 9.4.1 Parameter Kinds

Cursive supports four kinds of parameters:

**1. Positional parameters:**
- Appear before `*` separator (or entire list if no `*`)
- Must be supplied positionally at call sites
- May have defaults
- Bound by position (first argument → first param, etc.)

**2. Named parameters:**
- Appear after `*` separator
- Must be supplied by name using `name: expr` syntax
- May have defaults
- Bound by name matching

**3. Default parameters:**
- Any positional or named parameter may have a default value
- Default expression must be compile-time evaluable (Part III §3.3)
- May reference earlier parameters in the parameter list
- Evaluated after binding all provided arguments, in textual order

**4. Self parameter:**
- First parameter in procedure declarations
- Syntax: `self: perm Self` or `$`
- Bound from receiver expression in `receiver::procedure(...)` calls
- Permission-checked against procedure's declared requirement

**5. Variadic parameters:**
- Only permitted in `extern` declarations
- Syntax: `...`
- Represents C-style variable argument lists
- Always appears last in parameter list

### 9.4.2 Binding Algorithm

The complete argument-to-parameter binding algorithm follows these steps:

**Algorithm 9.1: Argument Binding**

```
Input:
  - Function/procedure signature with parameters P = [p₁, p₂, ..., pₙ]
  - Call-site arguments A = [a₁, a₂, ..., aₘ] (positional) + named args N

Output:
  - Binding B: Parameter → Argument/Default
  - Or error if binding fails

Steps:

1. Separate parameters into positional and named sections at `*` boundary
   Let P_pos = positional parameters
   Let P_named = named-only parameters

2. Bind receiver (if procedure):
   if signature is procedure:
       Bind self parameter to receiver expression
       Check receiver permission ≥ self's declared permission

3. Bind positional arguments to positional parameters:
   for i = 1 to min(|A|, |P_pos|):
       Bind P_pos[i] to A[i]
       Check argument type matches parameter type
       Check argument permission satisfies parameter permission

4. Process named arguments:
   for each named_arg (name: expr) in N:
       if name ∈ P_named:
           if name already bound:
               ERROR E9F/P-0202: duplicate named argument
           Bind parameter named 'name' to expr
           Check expr type matches parameter type
           Check expr permission satisfies parameter permission
       else if name ∈ P_pos:
           ERROR E9F/P-0205: positional parameter passed by name
       else:
           ERROR E9F/P-0201: unknown named argument

5. Fill defaults for unbound parameters:
   for each parameter p in P:
       if p not bound:
           if p has default:
               Bind p to its default expression
               Evaluate default (may reference earlier bound params)
           else:
               ERROR E9F/P-0203: missing required argument

6. Verify all parameters bound:
   if any parameter remains unbound:
       ERROR E9F/P-0203: missing required argument

Return binding B
```

**Evaluation order (see §9.9.1 for details):**
1. Receiver expression (if present)
2. Positional arguments left-to-right
3. Named argument expressions in textual order
4. Default expressions in parameter list order (for unbound params)

### 9.4.3 Type and Permission Checking

For each bound argument-parameter pair:

**Type checking:**

```
[Arg-Type-Check]
parameter p has type τ_p
argument a has type τ_a
τ_a <: τ_p  (subtyping per Part II)
--------------------------------------------------------
type check succeeds

[Arg-Type-Mismatch]
τ_a ⊄ τ_p
--------------------------------------------------------
ERROR (E9F/P-0404): argument type mismatch
```

**Permission checking:**

```
[Arg-Perm-Check]
parameter p requires permission perm_p
argument a provides permission perm_a
perm_a ≥ perm_p  (lattice: own ≥ mut ≥ imm)
--------------------------------------------------------
permission check succeeds (implicit weakening if perm_a > perm_p)

[Arg-Perm-Insufficient]
perm_a < perm_p
--------------------------------------------------------
ERROR (E9F/P-0204): insufficient permission (cannot upgrade)
```

### 9.4.4 Default Parameter Evaluation

Default parameters are evaluated lazily (only if not supplied) according to these rules:

**Evaluation timing:**
- Defaults are evaluated AFTER all provided arguments are evaluated and bound
- Defaults evaluate in textual order (parameter list order)
- Each default may reference parameters bound earlier in the list

**Const-evaluability constraint:**

```
[Default-Const]
parameter p has default expression d
d is compile-time evaluable (Part III §3.3)
d may reference parameters p₁, ..., pᵢ₋₁ where i is position of p
--------------------------------------------------------
default valid
```

**Examples:**

```cursive
// Simple defaults
function greet(name: string, greeting: string = "Hello"): string {
    result greeting + ", " + name
}

greet("Alice")              // greeting = "Hello"
greet("Bob", "Hi")          // greeting = "Hi"

// Default referencing earlier parameter
function allocate_aligned(size: usize, alignment: usize = size): Allocation {
    // If alignment not provided, use size as alignment
    // ...
}

allocate_aligned(1024)           // alignment = 1024
allocate_aligned(1024, 512)      // alignment = 512

// Named parameter with default referencing positional parameter
function create_buffer(capacity: usize, *, initial_size: usize = capacity / 2): Buffer {
    // ...
}

create_buffer(1000)                         // initial_size = 500
create_buffer(1000, initial_size: 100)      // initial_size = 100

// Comptime function in default
comptime function default_port(): u16 { 8080 }

function start_server(host: string, port: u16 = default_port()): Server {
    // ...
}

// Complex default expression
function configure(
    width: i32,
    height: i32,
    *,
    aspect_ratio: f64 = (width as f64) / (height as f64)
): Config {
    Config { width, height, aspect_ratio }
}

configure(1920, 1080)                  // aspect_ratio = 1.777...
configure(1920, 1080, aspect_ratio: 2.0)  // aspect_ratio = 2.0
```

### 9.4.5 Named Argument Rules

Named arguments provide flexibility and readability at call sites.

**Syntax:** `name: expression`

**Rules:**

1. **Position constraint:** Named arguments must appear AFTER all positional arguments
2. **Uniqueness:** Each named argument name must be unique in the call
3. **Existence:** Each named argument must correspond to a declared named parameter
4. **Type/permission:** Named argument expressions must satisfy parameter type and permission requirements

**Desugaring:**

Named arguments are syntactic sugar for binding by name rather than position. The compiler:
1. Matches each `name: expr` to the parameter named `name`
2. Verifies the parameter is in the named section (after `*`)
3. Binds the expression to that parameter
4. Type-checks and permission-checks normally

**Examples:**

```cursive
function request(
    url: string,
    method: string = "GET",
    *,
    timeout_ms: i32 = 5000,
    retry: bool = true,
    headers: Map<string, string> = Map::new()
): Result<Response, Error> {
    // ...
}

// All positional (method), then named
request("https://api.example.com", "POST", timeout_ms: 10000, retry: false)

// Use defaults for method, specify named
request("https://api.example.com", timeout_ms: 3000)

// Specify all named
request("https://api.example.com", "GET", timeout_ms: 1000, retry: false, headers: my_headers)

// Named arguments in any order (after positionals)
request("https://api.example.com", retry: false, headers: my_headers, timeout_ms: 1000)

// ERROR: Named before positional
// request(url: "https://api.example.com", "POST")  // ERROR: positional after named

// ERROR: Unknown named argument
// request("https://api.example.com", cache: true)  // ERROR E9F/P-0201: 'cache' not found

// ERROR: Duplicate named argument
// request("https://api.example.com", retry: true, retry: false)  // ERROR E9F/P-0202

// ERROR: Positional parameter passed by name
// request(url: "https://api.example.com", method: "POST")  // ERROR E9F/P-0205: url, method are positional
```

### 9.4.6 Variadic Parameters (FFI-Only)

Variadic parameters using `...` are permitted ONLY in extern function declarations for C FFI compatibility.

**Syntax:**

```ebnf
ExternParam ::= Ident ":" Type | "..."
```

**Constraints:**

```
[Variadic-Extern-Only]
variadic parameter ... in non-extern declaration
--------------------------------------------------------
ERROR (E9F-0601): variadic parameters allowed only in extern declarations

[Variadic-Last]
variadic parameter not last in parameter list
--------------------------------------------------------
ERROR (E9F-0603): variadic parameter must be last
```

**Calling variadic functions:**

Calls to variadic extern functions:
- MUST be within `unsafe { ... }` block
- MUST have `uses ffi.call` effect
- Arguments passed after fixed parameters are type-checked but not count-checked
- Caller responsible for providing correct types per C varargs conventions

**Examples:**

```cursive
extern "C" {
    function printf(fmt: *u8, ...): i32;
    function fprintf(file: *File, fmt: *u8, ...): i32;
    function sprintf(buf: *mut u8, fmt: *u8, ...): i32;
}

procedure demonstrate_varargs()
    uses ffi.call
{
    unsafe {
        printf(c"Hello, %s!\n", c"World")
        printf(c"Values: %d, %d, %d\n", 1, 2, 3)

        var buf: [u8; 100] = [0; 100]
        sprintf(&mut buf[0] as *mut u8, c"Number: %d\n", 42)
    }
}

// ERROR: Variadic in regular function
// function bad_variadic(x: i32, ...): i32 { 0 }  // ERROR E9F-0601
```

---

## 9.5 Return Semantics

This section specifies how functions and procedures return values to their callers, including single returns, multiple returns, divergence, and ownership transfer.

### 9.5.1 Single Value Returns

The default return mechanism is a single value of the declared return type.

**Syntax:**
- `return expr` — immediately exits the callable with value `expr`
- `result expr` — produces value `expr` as the result of the enclosing block
- `return` (no expression) — returns unit `()` (only valid for unit-returning callables)

**Type rule:**

```
[T-Return]
Γ ⊢ e : τ
callable return type is τ
--------------------------------------------------------
Γ ⊢ return e : !        (never type, doesn't continue)

[T-Result]
Γ ⊢ e : τ
enclosing block expects type τ
--------------------------------------------------------
Γ ⊢ result e : τ        (block evaluates to τ)
```

**Semantics:**

`return` statement:
- Evaluates the return expression
- Executes all deferred actions in LIFO order (Part VI §6.9)
- Checks postconditions (`will` clauses) if present
- Transfers control back to caller with return value
- Binding at call site receives ownership if return type is `own T`

`result` expression:
- Makes the enclosing block evaluate to the given value
- Does NOT exit outer functions
- Used as the final expression in block-bodied callables

**Examples:**

```cursive
// Simple return
function factorial(n: i32): i32 {
    if n <= 1 {
        return 1
    }
    result n * factorial(n - 1)
}

// Early return with deferred cleanup
function process_file(path: string): Result<(), Error>
    uses fs.read, fs.write
{
    let file = open_file(path)?
    defer { file::close() }  // Always runs before return

    if file::size() == 0 {
        return Err(Error::new("Empty file"))  // Defer runs here
    }

    // Process file...
    result Ok(())  // Defer runs here too
}

// Result in nested blocks
function compute(x: i32): i32 {
    let value = {
        if x > 0 {
            result x * 2  // Block result
        } else {
            result 0
        }
    }
    result value + 1  // Function result
}
```

### 9.5.2 Expression-Bodied Callables

Expression-bodied functions and procedures (§9.2.3, §9.3.3) provide concise syntax for simple cases:

**Syntax:**
```cursive
function f(params): T = expr
procedure p(self, params): U = expr  // declared inside the containing type
```

**Desugaring:**
```cursive
function f(params): T { result expr }
procedure p(self, params): U { result expr }
```

**Constraints:**
- The `return` statement is NOT permitted in expression-bodied form
- The expression's type must match the declared return type
- Effects from evaluating `expr` must be covered by `uses` clause

**Examples:**

```cursive
// Simple arithmetic
function add(x: i32, y: i32): i32 = x + y
function square(x: i32): i32 = x * x

// Conditional expression
function max(a: i32, b: i32): i32 = if a > b { a } else { b }
function abs(x: i32): i32 = if x < 0 { -x } else { x }

// Procedure expression-bodied
record Point {
    x: f64
    y: f64

    procedure magnitude(self: Point): f64 =
        (self.x * self.x + self.y * self.y).sqrt()

    procedure scaled(self: Point, factor: f64): Point =
        Point { x: self.x * factor, y: self.y * factor }
}

// Block expression (block itself still needs result)
function greeting(name: string): string = {
    let prefix = "Hello"
    result prefix + ", " + name + "!"
}
```

### 9.5.3 Multiple Returns (Tuple-Based)

Cursive supports returning multiple values using tuples.

**Syntax:**
- Return type: `(T₁, T₂, ..., Tₙ)`
- Return expression: `return (e₁, e₂, ..., eₙ)` or `result (e₁, e₂, ..., eₙ)`

**Semantics:**
- Multiple returns are semantic tuples
- Implementations MAY use multiple registers for small tuples (non-normative optimization)
- Callers can destructure with tuple patterns (§9.5.4)

**Type rule:**

```
[T-Return-Tuple]
Γ ⊢ e₁ : τ₁    Γ ⊢ e₂ : τ₂    ...    Γ ⊢ eₙ : τₙ
callable return type is (τ₁, τ₂, ..., τₙ)
--------------------------------------------------------
Γ ⊢ return (e₁, e₂, ..., eₙ) : !
```

**ABI considerations (informative):**

Implementations MAY optimize tuple returns:
- Small tuples (2-3 elements of primitive types) may use multiple registers
- Larger tuples may use hidden pointer (sret) convention
- The semantic model remains: function returns a single tuple value

**Examples:**

```cursive
// Return pair
function div_rem(dividend: i32, divisor: i32): (i32, i32) {
    let quotient = dividend / divisor
    let remainder = dividend % divisor
    result (quotient, remainder)
}

// Return triple
function split_name(full: string): (string, string, string) {
    // Parse into first, middle, last
    // ...
    result (first, middle, last)
}

// Expression-bodied multiple return
function swap<T, U>(pair: (T, U)): (U, T) = (pair.1, pair.0)

// Procedure returning tuple
record Point {
    x: f64
    y: f64

    procedure to_polar(self: Point): (f64, f64) {
        let r = self::magnitude()
        let theta = (self.y / self.x).atan()
        result (r, theta)
    }
}

// Complex return with ownership transfer
function extract_parts(s: own String): (own [u8], usize) {
    let bytes = s::into_bytes()
    let len = bytes.len()
    result (bytes, len)
}
```

### 9.5.4 Destructuring Multiple Returns at Call Sites

Tuple returns can be destructured at call sites using pattern matching or tuple bindings.

**Syntax:**

```cursive
// Tuple pattern in let binding
let (x, y, z) = multi_return_function()

// Tuple pattern in match
match function_returning_result() {
    Ok((value, metadata)) => ...,
    Err(e) => ...
}
```

**Examples:**

```cursive
function parse_coordinate(s: string): Result<(f64, f64), Error> {
    // Parse "x,y" format
    // ...
}

// Destructure in let
let (x, y) = parse_coordinate("3.5,4.2")?

// Destructure in match
let point = match parse_coordinate(input) {
    Ok((x, y)) => Point { x, y },
    Err(e) => {
        eprintln("Parse error: {}", e)
        return Err(e)
    }
}

// Nested destructuring
function analyze(data: [i32]): (i32, i32, (i32, i32)) {
    let sum = data::sum()
    let count = data.len() as i32
    let min = data::min()
    let max = data::max()
    result (sum, count, (min, max))
}

let (sum, count, (min, max)) = analyze(numbers)

// Ignore some components with _
let (quotient, _) = div_rem(10, 3)  // Ignore remainder
```

### 9.5.5 Diverging Functions (Never Type)

Functions that never return normally have return type `!` (never type, defined in Part II §2.1.5).

**Semantics:**
- Diverging functions never transfer control back to caller
- May loop infinitely, call `panic()`, call `abort()`, or throw exceptions
- Never type coerces to any type (universal subtype)

**Type rule:**

```
[T-Diverge]
Γ ⊢ f : (τ₁, ..., τₙ) → ! ! ε
--------------------------------------------------------
Γ ⊢ f(...) : T        (for any type T)

[T-Never-Coerce]
Γ ⊢ e : !
--------------------------------------------------------
Γ ⊢ e : T             (for any type T)
```

This coercion allows diverging expressions in any type context:

```cursive
function example(x: i32): i32 {
    if x < 0 {
        panic("Negative input")  // panic returns !, coerces to i32
    } else {
        result x
    }
}
```

**Common diverging functions:**

```cursive
// Panic function
function panic(msg: string): !
    uses panic
{
    std::process::abort_with_message(msg)
}

// Infinite loop
function server_loop(): !
    uses io.network
{
    loop {
        handle_request(accept_connection())
    }
}

// Unconditional exit
function exit(code: i32): !
    uses process.exit
{
    std::process::exit(code)
}

// Unreachable marker
function unreachable(): ! {
    panic("Entered unreachable code")
}
```

**Usage examples:**

```cursive
function safe_divide(a: i32, b: i32): i32 {
    if b == 0 {
        panic("Division by zero")  // Returns !, coerces to i32
    }
    result a / b
}

function get_config(key: string): Config {
    match config_map::get(key) {
        Some(c) => c,
        None => panic("Missing config key: {}", key)  // Coerces to Config
    }
}

// Control flow analysis recognizes divergence
function complete(x: i32): i32 {
    if x > 0 {
        result x
    } else {
        panic("Non-positive value")  // No need for else branch
    }
    // Compiler knows this is unreachable
}
```

### 9.5.6 Return Type Requirements

Return types for functions and procedures MUST be explicitly declared (except for closures where inference may apply).

**Constraints:**

```
[Return-Type-Required]
function or procedure declaration
return type annotation omitted
--------------------------------------------------------
ERROR (E9F-0109): return type annotation required

[Return-Type-Unit-Special]
callable returns unit ()
may omit result/return in block body
--------------------------------------------------------
unit return type has special treatment
```

**Examples:**

```cursive
// Return type required
function compute(x: i32): i32 {  // : i32 required
    result x * 2
}

// Even for unit
procedure log(msg: string): ()   // : () required (though usually written)
    uses io.write
{
    println(msg)
    // No result needed for unit
}

// Typical unit return omits ()
procedure log_simple(msg: string)  // Return type defaults to () in syntax
    uses io.write
{
    println(msg)
}

// ERROR: Missing return type
// function bad(x: i32) {  // ERROR E9F-0109
//     result x * 2
// }
```

### 9.5.7 Ownership and Returns

Return values may transfer ownership to the caller using `own` permission.

**Ownership transfer:**

```
[Return-Own]
function f(...) : own T
--------------------------------------------------------
caller receives new binding with own permission
function body must produce value of type own T
```

**Examples:**

```cursive
// Create and return owned value
function create_buffer(size: usize): own Buffer {
    result Buffer::new(size)
}

let buf = create_buffer(1024)  // buf has own permission

// Transform consuming receiver into owned return
record Box<T> {
    value: T

    procedure map<U>(self: own Box<T>, f: (T) → U): own Box<U> {
        let new_value = f(self.value)
        result Box { value: new_value }
    }
}

let b1: own Box<i32> = Box { value: 10 }
let b2: own Box<i32> = b1::map(|x| -> x * 2)  // b1 consumed, b2 owned
// b1.value  // ERROR: b1 consumed

// Return ownership of moved-in value
function take_and_return(s: own String): own String {
    // Process s...
    result s  // Return ownership back
}

// Multiple returns with ownership
function split_buffer(buf: own Buffer): (own Buffer, own Buffer) {
    let (first, second) = buf::split_at(buf.len() / 2)
    result (first, second)
}

let big_buf = Buffer::new(2048)
let (buf1, buf2) = split_buffer(move big_buf)
// big_buf consumed, buf1 and buf2 are owned
```

**Immutable and mutable returns:**

Returns with `imm` or `mut` permissions are rare (typically only for references, which are covered in Part IV):

```cursive
// Returning references (lifetime-bound)
function find_max<'a>(items: &'a [i32]): &'a i32 {
    // Return reference tied to input lifetime
    // ...
}

// Mutable reference return
function get_element_mut<'a, T>(vec: &'a mut Vec<T>, index: usize): &'a mut T {
    // Return mutable reference tied to input lifetime
    // ...
}
```

Most return types are by-value (implicitly `own`) or are references with explicit lifetime annotations.

---

## 9.6 Calls and Dispatch

This section specifies how callable names are resolved at call sites, how arguments are evaluated and bound, and how control transfers to the callee.

### 9.6.1 Evaluation Order

Call expressions evaluate their components in a deterministic left-to-right order.

**Evaluation sequence:**

1. **Function/receiver expression**: Evaluate the callable or receiver first
2. **Arguments**: Evaluate arguments strictly left-to-right
3. **Default parameters**: Evaluate defaults for unbound parameters in parameter list order
4. **Call**: Transfer control to callee with bound arguments

**Formal rule:**

```
[E-Call-Order]
⟨f, σ₀⟩ ⇓ ⟨v_f, σ₁⟩
⟨a₁, σ₁⟩ ⇓ ⟨v₁, σ₂⟩
⟨a₂, σ₂⟩ ⇓ ⟨v₂, σ₃⟩
...
⟨aₙ, σₙ⟩ ⇓ ⟨vₙ, σₙ₊₁⟩
defaults d₁, ..., dₘ evaluated in order → vₙ₊₁, ..., vₙ₊ₘ
⟨v_f(v₁, ..., vₙ₊ₘ), σₙ₊₁⟩ ⇓ ⟨v_result, σ'⟩
--------------------------------------------------------
⟨f(a₁, ..., aₙ), σ₀⟩ ⇓ ⟨v_result, σ'⟩
```

**Determinism guarantee:**

Cursive guarantees left-to-right evaluation order (except for short-circuit operators `&&` and `||` from Part V). This enables reasoning about side effects:

```cursive
procedure example()
    uses io.write
{
    let x = 1
    let result = f(
        { println("First"); x + 1 },   // Prints "First", evaluates to 2
        { println("Second"); x + 2 },  // Prints "Second", evaluates to 3
        { println("Third"); x + 3 }    // Prints "Third", evaluates to 4
    )
    // Output order: "First", "Second", "Third" (guaranteed)
}
```

**Procedure call evaluation order:**

For `receiver::procedure(args)`:

1. Evaluate `receiver` expression
2. Evaluate arguments `args` left-to-right
3. Desugar to `Type::procedure(receiver, args)` (§9.6.2)
4. Transfer control

```cursive
let p = { println("Receiver"); get_point() }
let result = p::distance(
    { println("Arg"); other_point }
)
// Output: "Receiver", "Arg" (guaranteed order)
```

### 9.6.2 Type-Scope Procedure Calls

Type-scope procedure calls use the `::` operator to invoke procedures on receivers.

**Syntax:**
```cursive
receiver::procedure_name(args)
```

**Desugaring:**

```
[Desugar-Procedure-Call]
receiver::procedure_name(a₁, ..., aₙ)
Type is the static type of receiver
--------------------------------------------------------
desugars to: Type::procedure_name(receiver, a₁, ..., aₙ)
```

The receiver becomes the first argument (bound to `self` parameter).

**Type rule:**

```
[T-Type-Scope-Procedure-Call]
Γ ⊢ receiver : T
T has procedure p : (self: perm T, τ₁, ..., τₙ) → τᵣ ! ε
Γ ⊢ aᵢ : τᵢ  (∀i ∈ 1..n)
receiver provides permission ≥ perm
ε ⊆ Γ.effects
--------------------------------------------------------
Γ ⊢ receiver::p(a₁, ..., aₙ) : τᵣ ! ε
```

**Permission checking:**

The receiver's permission must satisfy the procedure's `self` parameter requirement:
- `self: imm Self` — any receiver permission works (own, mut, imm)
- `self: mut Self` — requires own or mut receiver
- `self: own Self` — requires own receiver (consumes it)

**Examples:**

```cursive
record Vec3 {
    x: f64
    y: f64
    z: f64

    procedure length(self: Vec3): f64 {
        ((self.x * self.x) + (self.y * self.y) + (self.z * self.z)).sqrt()
    }

    procedure normalize(self: mut Vec3) {
        let len = self::length()
        self.x /= len
        self.y /= len
        self.z /= len
    }

    procedure into_array(self: own Vec3): [f64; 3] {
        [self.x, self.y, self.z]
    }
}

let v1: own Vec3 = Vec3 { x: 1.0, y: 2.0, z: 3.0 }
let len = v1::length()        // OK: own → imm weakening
v1::normalize()               // OK: own → mut weakening
let arr = v1::into_array()    // OK: own → own, v1 consumed

// Equivalent fully qualified calls:
let len2 = Vec3::length(v1)
Vec3::normalize(v1)
let arr2 = Vec3::into_array(v1)
```

**Distinction from field access:**

- `receiver.field` — field access (data member)
- `receiver::procedure(args)` — procedure call (computation)

```cursive
record Point {
    x: f64
    y: f64

    procedure magnitude(self: Point): f64 {
        (self.x * self.x + self.y * self.y).sqrt()
    }
}

let p = Point { x: 3.0, y: 4.0 }
let x = p.x                // Field access: x = 3.0
let m = p::magnitude()     // Procedure call: m = 5.0
```

### 9.6.3 Resolution Algorithm

Callable name resolution determines which declaration a name refers to at a call site.

**Algorithm 9.2: Procedure/Function Resolution**

```
Input:
  - Call expression: optionally receiver, name, arguments
  - Scope context: current module, imports, type scopes

Output:
  - Resolved callable declaration
  - Or error if resolution fails

Steps:

1. Determine call form:
   if call is `receiver::name(args)`:
       goto TYPE_SCOPE_RESOLUTION
   else if call is `name(args)`:
       goto MODULE_SCOPE_RESOLUTION
   else if call is `Type::name(args)`:
       goto QUALIFIED_RESOLUTION

2. TYPE_SCOPE_RESOLUTION:
   a. Compute static type T of receiver
   b. Look up `name` in procedures of type T
   c. If found:
        - Verify first parameter is `self: perm Self`
        - Check arity: |args| + 1 == |params|
        - Check types: receiver type and arg types match params
        - Check permissions: receiver permission ≥ self permission
        - Check effects: required effects available
        - Resolve to T::name
      If not found:
        - ERROR E9F/P-0401: procedure not found in type scope

3. MODULE_SCOPE_RESOLUTION:
   a. Look up `name` in current module scope
   b. If found:
        - Verify it's a function (no self parameter)
        - Check arity: |args| == |params|
        - Check types: arg types match params
        - Check effects: required effects available
        - Resolve to function
      If not found:
        - Search imported modules (Part III §3.11)
        - If found in imports: resolve to imported function
        - If not found anywhere: ERROR E9F/P-0410: name not found

4. QUALIFIED_RESOLUTION:
   a. Parse Type::name
   b. Resolve Type to type declaration
   c. Look up `name` in Type's scope (functions or procedures)
   d. If function (no self):
        - Check arity: |args| == |params|
        - Resolve to Type::name as associated function call
      If procedure (has self):
        - Check arity: |args| == |params| (including self)
        - First arg must match self parameter type and permission
        - Resolve to Type::name as fully qualified procedure call
      If not found:
        - ERROR E9F/P-0401: name not found in type scope

5. GENERIC INSTANTIATION:
   if callable has generic parameters:
       - Attempt to infer generic arguments from call-site arguments
       - If inference fails: ERROR E9F/P-0413: cannot infer generics
       - If inference succeeds: monomorphize with inferred types
       - If explicit generics provided: use those, check well-formedness

6. OVERLOAD RESOLUTION:
   Cursive does NOT support overloading
   if multiple candidates with same name:
       - ERROR E9F/P-0411: ambiguous call (should not happen with name rules)

Return resolved callable
```

**Disambiguation rules:**

1. **Type-scope precedence**: `receiver::name` always looks in receiver's type scope first
2. **Fully qualified wins**: `Module::name` or `Type::name` is unambiguous
3. **No overloading**: At most one callable with given name per scope
4. **Arity must match**: Argument count must match parameter count exactly (after self)
5. **Inference failure**: If generic inference fails, compiler requires explicit type arguments or ascriptions

**Examples:**

```cursive
// Type-scope resolution
record Point {
    x: f64
    y: f64

    procedure distance(self: Point, other: Point): f64 { ... }
}

let p1 = Point { x: 0.0, y: 0.0 }
let p2 = Point { x: 3.0, y: 4.0 }

// Type-scope: looks up `distance` in Point's procedures
let d = p1::distance(p2)  // Resolves to Point::distance

// Fully qualified (equivalent)
let d2 = Point::distance(p1, p2)  // Explicit receiver as first arg

// Module-scope resolution
function calculate(x: i32, y: i32): i32 { ... }

// Module-scope: looks up `calculate` in current module, then imports
let result = calculate(10, 20)

// Qualified module-scope
let result2 = math::calculate(10, 20)  // Looks in `math` module

// Generic resolution with inference
function identity<T>(x: T): T { x }

let a = identity(42)          // T inferred as i32
let b = identity<string>("hi")  // T explicitly string

// Generic inference failure
function collect<T>(): Vec<T> { Vec::new() }

// let v = collect()          // ERROR E9F/P-0413: cannot infer T
let v = collect<i32>()         // OK: T explicitly i32

// Disambiguation with imports
// Assume both `std::collections::map` and `my_module::map` imported

// Ambiguous without qualification
// let result = map(data, func)  // ERROR E9F/P-0411: ambiguous

// Qualified to disambiguate
let result1 = std::collections::map(data, func)
let result2 = my_module::map(data, func)
```

**Error cases:**

```cursive
record Example {
    value: i32

    procedure method(self: Example, x: i32): i32 { ... }

    procedure mutate(self: mut Example) { ... }

    procedure log(self: Example)
        uses io.write
    { ... }
}

let e = Example { value: 10 }

// ERROR E9F/P-0403: Wrong arity
// e::method()           // ERROR: missing argument x
// e::method(1, 2)       // ERROR: too many arguments

// ERROR E9F/P-0404: Type mismatch
// e::method("hello")    // ERROR: x expects i32, got string

// ERROR E9F/P-0402: Permission insufficient
let immut: imm Example = Example { value: 0 }
// immut::mutate()       // ERROR: imm cannot satisfy mut

// ERROR E9F/P-0405: Effects not available
function pure_context() {
    let e = Example { value: 0 }
    // e::log()           // ERROR: io.write not available in pure context
}

// ERROR E9F/P-0401: Procedure not found
// e::nonexistent()      // ERROR: no such procedure in Example
```

### 9.6.4 Static Dispatch

Cursive uses **static dispatch** by default: callable resolution occurs at compile time and calls are direct (not through vtables).

**Monomorphization:**

Generic functions and procedures are monomorphized:
- Each distinct instantiation generates specialized code
- No runtime type parameters
- No virtual dispatch overhead

```cursive
function process<T>(value: T) {
    // ...
}

process(42)        // Generates process_i32
process("hello")   // Generates process_string
// Two different compiled functions
```

**Inlining opportunities:**

Static dispatch enables aggressive inlining optimizations:
- Small functions can be inlined at call sites
- `[[inline]]` attribute provides hints to compiler
- Cross-module inlining possible with LTO

**Performance characteristics:**

- **No vtable lookup**: Call target known at compile time
- **Direct call**: Single instruction to target address
- **Inline candidate**: Compiler can inline small functions
- **Cache-friendly**: Predictable call targets

### 9.6.5 Dynamic Dispatch (Contract-Based)

Dynamic dispatch occurs when calling procedures through contract witnesses (Part VII).

**Mechanism:**

Contracts may be implemented via witnesses, which are essentially vtables:

```cursive
contract Drawable {
    procedure draw(self)
        uses io.write
}

// Witness type for Drawable
// (compiler-generated, conceptual)
type DrawableWitness = {
    draw: (self: *opaque) → () ! {io.write}
}
```

When calling through a witness, dispatch is dynamic:

```cursive
function render(drawable: impl Drawable)
    uses io.write
{
    drawable::draw()  // Dynamic dispatch through witness
}

// Internally desugars to:
// witness.draw(drawable_data)
```

**Performance implications:**

- **Vtable lookup**: One indirection through witness table
- **No inlining**: Cannot inline across dynamic boundary
- **Cache impact**: Branch prediction harder
- **Necessary for**: Heterogeneous collections, plugin systems

**Static vs dynamic choice:**

```cursive
// Static: monomorphized for each T
function process_static<T: Drawable>(item: T)
    uses io.write
{
    item::draw()  // Static dispatch, can inline
}

// Dynamic: single compiled function
function process_dynamic(item: impl Drawable)
    uses io.write
{
    item::draw()  // Dynamic dispatch through witness
}
```

### 9.6.6 Effect Propagation at Call Sites

Calling an effectful function or procedure requires the caller to have those effects available.

**Effect aggregation:**

```
[Effect-Aggregate]
Γ ⊢ f : (τ₁, ..., τₙ) → U ! ε_f
Γ ⊢ aᵢ : τᵢ ! ε_aᵢ  (∀i)
ε_total = ε_f ∪ ε_a₁ ∪ ... ∪ ε_aₙ
--------------------------------------------------------
Γ ⊢ f(a₁, ..., aₙ) : U ! ε_total

[Effect-Check]
Γ ⊢ call : τ ! ε_call
ε_call ⊆ Γ.effects
--------------------------------------------------------
call permitted

[Effect-Unavailable]
ε_call ⊄ Γ.effects
--------------------------------------------------------
ERROR (E9F/P-0405): required effects not available
```

**Examples:**

```cursive
// Effectful function
function read_config(path: string): Result<Config, Error>
    uses fs.read
{
    // ...
}

// Caller must provide fs.read
procedure load_app()
    uses fs.read, io.write
{
    let config = read_config("/etc/app.conf")  // OK: fs.read available
    println("Config loaded")  // OK: io.write available
}

// Pure caller cannot call effectful function
function pure_function() {
    // let config = read_config("/etc/app.conf")  // ERROR E9F/P-0405: fs.read unavailable
}

// Effect-polymorphic caller
function with_logging<ε>(operation: () → () ! ε)
    uses io.write, ε
{
    println("Starting operation")
    operation()  // OK: ε covered by uses clause
    println("Operation complete")
}

// Argument effects are checked too
procedure process_data()
    uses io.write
{
    with_logging(|| -> {
        println("Processing")  // This closure uses io.write
    })  // OK: io.write available in enclosing context
}
```

### 9.6.7 Fully Qualified Calls

Fully qualified syntax makes scope resolution explicit and can disambiguate names.

**Syntax:**
```cursive
Module::function_name(args)
Type::function_name(args)           // Associated function
Type::procedure_name(receiver, args)  // Procedure with explicit receiver
```

**Semantics:**

Fully qualified calls bypass normal resolution and directly reference the named declaration:

```
[Qualified-Function]
Γ ⊢ M::f : (τ₁, ..., τₙ) → U ! ε
Γ ⊢ aᵢ : τᵢ  (∀i)
--------------------------------------------------------
Γ ⊢ M::f(a₁, ..., aₙ) : U ! ε

[Qualified-Procedure]
Γ ⊢ T::p : (self: perm T, τ₁, ..., τₙ) → U ! ε
Γ ⊢ recv : perm' T    (perm' ≥ perm)
Γ ⊢ aᵢ : τᵢ  (∀i)
--------------------------------------------------------
Γ ⊢ T::p(recv, a₁, ..., aₙ) : U ! ε
```

**Use cases:**

1. **Disambiguation**: When names conflict in imports
2. **Clarity**: Make scope explicit for readers
3. **Testing**: Call type-scope procedures with explicit receivers

**Examples:**

```cursive
// Disambiguate imported names
import std::collections::map
import my_lib::map

let result1 = std::collections::map(data, func)  // Explicit std
let result2 = my_lib::map(data, other_func)      // Explicit my_lib

// Call associated function explicitly
record Point {
    x: f64
    y: f64

    function origin(): Point { Point { x: 0.0, y: 0.0 } }

    procedure translate(self: mut Point, dx: f64, dy: f64) {
        self.x += dx
        self.y += dy
    }
}

let p = Point::origin()  // Fully qualified (typical usage)

var p = Point { x: 1.0, y: 1.0 }
Point::translate(p, 2.0, 2.0)  // Fully qualified
// Equivalent to:
p::translate(2.0, 2.0)

// Useful for testing or when receiver binding awkward
let points = [Point { x: 0.0, y: 0.0 }, Point { x: 1.0, y: 1.0 }]
Point::translate(points[0], 5.0, 5.0)  // Clearer than points[0]::translate(...)

// Fully qualified with generics
function identity<T>(x: T): T { x }

let a = identity::<i32>(42)  // Explicit generic instantiation
let b = std::clone::clone::<String>(s)  // Module::function::<T>(args)
```

### 9.6.8 Pipeline Operator Integration

The pipeline operator `=>` (Part V §5.5.9) provides call chaining without UFCS.

**Syntax:**
```cursive
value => function
value => (|x| -> receiver::procedure(x, args))
```

**Semantics:**

Pipeline passes the left operand as the first argument to the right operand:

```
[Pipeline-Call]
Γ ⊢ v : T
Γ ⊢ f : (T, τ₁, ..., τₙ) → U ! ε
--------------------------------------------------------
Γ ⊢ v => f : U ! ε
// Desugars to: f(v)

[Pipeline-Chain]
Γ ⊢ v : T₁
Γ ⊢ f : (T₁) → T₂ ! ε₁
Γ ⊢ g : (T₂) → T₃ ! ε₂
--------------------------------------------------------
Γ ⊢ v => f => g : T₃ ! (ε₁ ∪ ε₂)
// Desugars to: g(f(v))
```

**Integration with type-scope procedures:**

Since `::` syntax doesn't chain, use pipelines for sequential operations:

```cursive
record Buffer {
    data: [u8]

    procedure compress(self: Buffer): Buffer { ... }
    procedure encrypt(self: Buffer, key: Key): Buffer { ... }
    procedure encode(self: Buffer): string { ... }
}

// Without pipeline (nested calls, awkward)
let result = encode(encrypt(compress(buffer), key))

// With pipeline (left-to-right, clear)
let result = buffer
    => compress
    => (|b| -> b::encrypt(key))
    => encode

// Or with module functions instead of procedures:
let result = buffer
    => compress_buffer
    => encrypt_buffer(_, key)  // _ placeholder for piped value
    => encode_buffer
```

**No UFCS:**

Cursive explicitly does NOT support UFCS (Uniform Function Call Syntax):

```cursive
// NOT SUPPORTED (UFCS-style dot chaining):
// let result = buffer.compress().encrypt(key).encode()

// Use pipeline instead:
let result = buffer => compress => encrypt(_, key) => encode
```

**Rationale:** Separating field access (`.`) from calls (`::`, `=>`) provides syntactic clarity and avoids ambiguity.

---

## 9.7 Closures and Function Pointers

This section covers anonymous callable expressions (closures) and zero-capture function pointers.

### 9.7.1 Lambda Syntax and Closure Expressions

Closures are anonymous functions that may capture variables from their environment.

**Syntax** (from Part V §5.10):

```ebnf
ClosureExpr ::= "|" ParamList "|" "->" Expr
              | "|" ParamList "|" Block

ParamList   ::= (Ident (":" Type)?)  ("," Ident (":" Type)?)*
```

**Type inference:**

- Parameter types may be inferred from context
- Return type is inferred from body expression/block
- Effect set is inferred from body operations and captures

**Examples:**

```cursive
// Explicit parameter types
let add: (i32, i32) → i32 ! ∅ = |x: i32, y: i32| -> x + y

// Inferred parameter types (from expected type)
let add_infer: (i32, i32) → i32 ! ∅ = |x, y| -> x + y

// Block body
let complex = |x: i32| -> {
    let doubled = x * 2
    let squared = doubled * doubled
    result squared
}

// Closure with effects
let logger = |msg: string| -> {
    println("Log: {}", msg)
}  // Type: (string) → () ! {io.write}

// Capturing environment
let multiplier = 10
let scale = |x: i32| -> x * multiplier  // Captures `multiplier`
```

### 9.7.2 Closure Capture Semantics

Closures capture variables from their lexical environment. Cursive uses **minimal-permission capture**: each captured variable is captured with the minimum permission required by the closure body.

**Capture classifications** (from Part IV §4.10.3):

1. **Immutable capture (`imm`):**
   - Closure reads but doesn't mutate the captured variable
   - Original binding remains usable
   - Captured value is shared (reference semantics)

2. **Mutable capture (`mut`):**
   - Closure mutates the captured variable
   - Original binding remains usable
   - Captured reference is exclusive
   - Closure cannot escape the variable's region

3. **Owned capture (`own`):**
   - Closure takes ownership of the captured variable
   - Original binding becomes invalid (moved)
   - Requires explicit `move` keyword on closure
   - Closure can outlive the original scope

**Type rules:**

```
[T-Closure-Immutable-Capture]
Γ ⊢ x : τ with permission imm
closure body reads x but doesn't mutate
--------------------------------------------------------
closure captures x as imm τ

[T-Closure-Mutable-Capture]
Γ ⊢ x : τ with permission mut
closure body mutates x
--------------------------------------------------------
closure captures x as mut τ
closure cannot escape region of x

[T-Closure-Move-Capture]
Γ ⊢ x : τ with permission own
closure marked with `move` keyword
--------------------------------------------------------
closure captures x as own τ
original binding x becomes invalid
```

**Examples:**

```cursive
// Immutable capture
let value = 42
let read_closure = || -> {
    println("Value is: {}", value)  // Captures value as imm
}
read_closure()
println("Original: {}", value)  // Still usable

// Mutable capture
var counter = 0
let increment = || -> {
    counter += 1  // Captures counter as mut
}
increment()
increment()
println("Counter: {}", counter)  // Prints 2

// Move capture (explicit)
let message = String::from("Hello")
let owned_closure = move || -> {
    println("{}", message)  // Captures message as own (moved)
}
owned_closure()
// println("{}", message)  // ERROR: message moved into closure

// Multiple captures with different permissions
let a = 10
var b = 20
let c = String::from("text")

let mixed = move || -> {
    let sum = a + b  // a captured as imm, b as imm
    b += 1           // b captured as mut (upgrades from imm)
    println("{}: {}", c, sum)  // c captured as own (due to move)
}
```

### 9.7.3 Region Escape Analysis

Closures that capture region-local values cannot escape that region unless the closure is region-polymorphic.

**Escape rules** (from Part XIII §13.3):

```
[Closure-Region-Escape]
closure captures value v from region r
closure itself escapes region r
v not region-polymorphic
--------------------------------------------------------
ERROR (E4015): captured value cannot escape region
```

**Examples:**

```cursive
function create_closure(): (i32) → i32 ! ∅ {
    let local = 10
    // let closure = |x| -> x + local  // ERROR E4015: local would escape
    // result closure

    // OK: Capture compile-time constant or owned value
    let owned = String::from("hello")
    let closure = move |x| -> x + owned.len()  // OK: owned moved
    result closure
}

// Region-polymorphic function can return closures over parameters
function create_adder<'r>(n: &'r i32): impl (i32) → i32 ! ∅ + 'r {
    |x| -> x + *n  // OK: closure tied to lifetime 'r
}
```

### 9.7.4 Effect Tracking in Closures

Closure effect sets include effects from both the body and captured variables.

**Effect aggregation:**

```
[Closure-Effects]
Γ ⊢ closure_body : τ ! ε_body
captured_vars have effects ε_capture
ε_total = ε_body ∪ ε_capture
--------------------------------------------------------
Γ ⊢ closure : (...) → τ ! ε_total
```

**Examples:**

```cursive
// Pure closure
let pure = |x: i32| -> x * 2  // Effects: ∅

// Closure with body effects
let with_io = |msg: string| -> {
    println("{}", msg)  // Uses io.write
}  // Effects: {io.write}

// Closure capturing effectful values (advanced)
let file = open_file("log.txt")  // Has associated effects
let log_closure = move || -> {
    file::write("Entry")  // Inherits file effects
}  // Effects: {fs.write}
```

### 9.7.5 Function Pointers

Function pointers are zero-sized callable values referring to named functions or non-capturing closures.

**Characteristics:**

- **Zero-sized**: Just a code address, no captured data
- **First-class**: Can be passed, returned, stored
- **Type**: Same as corresponding function type `(T₁, ..., Tₙ) → U ! ε`
- **Coercion**: Non-capturing closures can coerce to function pointers

**Type rules:**

```
[T-Function-Pointer]
Γ ⊢ f : (τ₁, ..., τₙ) → U ! ε
f is named function
--------------------------------------------------------
Γ ⊢ f : (τ₁, ..., τₙ) → U ! ε  (function pointer)

[T-Closure-To-FnPtr]
Γ ⊢ closure : (τ₁, ..., τₙ) → U ! ε
closure has no captures
--------------------------------------------------------
Γ ⊢ closure as (τ₁, ..., τₙ) → U ! ε  (coerce to function pointer)

[T-Closure-Capturing-No-Coerce]
closure has captures
--------------------------------------------------------
ERROR (E9F/P-0501): capturing closure cannot coerce to function pointer
```

**Examples:**

```cursive
// Named function to function pointer
function add(x: i32, y: i32): i32 { x + y }

let fp: (i32, i32) → i32 ! ∅ = add  // Function pointer
let result = fp(3, 4)  // Call through pointer: 7

// Non-capturing closure to function pointer
let multiply_ptr: (i32, i32) → i32 ! ∅ = |x, y| -> x * y
let result2 = multiply_ptr(5, 6)  // 30

// Capturing closure CANNOT coerce
let factor = 10
let scale_closure = |x: i32| -> x * factor  // Captures factor

// let scale_ptr: (i32) → i32 ! ∅ = scale_closure  // ERROR E9F/P-0501

// Function pointers in collections
let operations: [(i32, i32) → i32 ! ∅] = [add, |x, y| -> x - y, |x, y| -> x * y]

for op in operations {
    println("Result: {}", op(10, 5))
}

// Function pointer as parameter
function apply_twice(f: (i32) → i32 ! ∅, x: i32): i32 {
    f(f(x))
}

let double = |x: i32| -> x * 2
let result = apply_twice(double, 5)  // 20
```

### 9.7.6 Closure Types

Closures have function types but may carry captured environment data.

**Representation** (informative):

- **Non-capturing closure**: Same as function pointer (zero-sized)
- **Capturing closure**: Struct containing captures + function pointer
  - Captured data stored in struct
  - Function takes struct as hidden first parameter

**Size considerations:**

```cursive
// Non-capturing: size = size of function pointer (typically 8 bytes)
let nc = |x: i32| -> x * 2

// Capturing one i32: size = size of i32 + function pointer
let factor = 10
let c1 = |x: i32| -> x * factor

// Capturing multiple values: size = sum of captured sizes + function pointer
let a = 10
let b = 20
let c2 = |x: i32| -> x * a + b
```

**No automatic heap allocation:**

Cursive does NOT automatically box closures. If a closure needs to be stored beyond its natural lifetime, explicit boxing is required:

```cursive
// ERROR: Closure would escape without boxing
// function create_closure(n: i32): (i32) → i32 ! ∅ {
//     |x| -> x + n  // ERROR: captures n, cannot escape
// }

// OK: Explicit move and boxing (if needed)
function create_boxed_closure(n: i32): Box<(i32) → i32 ! ∅> {
    Box::new(move |x| -> x + n)  // Heap-allocated closure
}
```

---

## 9.8 Higher-Order Functions

Higher-order functions (functionals) accept function-typed parameters or return function-typed values.

### 9.8.1 Definition and Terminology

**Higher-order function (functional):** A function or procedure that:
1. Accepts one or more function-typed parameters, OR
2. Returns a function-typed value

Common functionals in functional programming:
- `map`: Transform elements of a collection
- `filter`: Select elements matching a predicate
- `fold`/`reduce`: Accumulate collection into single value
- `compose`: Combine two functions into one

### 9.8.2 Functions as Parameters

Functions can be passed as arguments to enable abstraction over computation.

**Type signature:**

```cursive
function higher_order(
    data: [T],
    operation: (T) → U ! ε
): [U]
    uses ε
{
    // Apply operation to each element
    // ...
}
```

**Examples:**

```cursive
// Map: transform each element
function map<T, U, ε>(items: [T], f: (T) → U ! ε): [U]
    uses alloc.heap, ε
{
    let result = Vec::new()
    loop item in items {
        result::push(f(item))
    }
    result result::into_array()
}

let numbers = [1, 2, 3, 4]
let doubled = map(numbers, |x| -> x * 2)  // [2, 4, 6, 8]

// Filter: select matching elements
function filter<T, ε>(items: [T], predicate: (T) → bool ! ε): [T]
    uses alloc.heap, ε
{
    let result = Vec::new()
    loop item in items {
        if predicate(item) {
            result::push(item)
        }
    }
    result result::into_array()
}

let even_numbers = filter(numbers, |x| -> x % 2 == 0)  // [2, 4]

// Fold: accumulate into single value
function fold<T, Acc, ε>(
    items: [T],
    initial: Acc,
    combine: (Acc, T) → Acc ! ε
): Acc
    uses ε
{
    var accumulator = initial
    loop item in items {
        accumulator = combine(accumulator, item)
    }
    result accumulator
}

let sum = fold(numbers, 0, |acc, x| -> acc + x)  // 10

// Multiple function parameters
function apply_binary<T, U, V>(
    a: T,
    b: U,
    f: (T, U) → V ! ∅
): V {
    f(a, b)
}

let result = apply_binary(3, 4, |x, y| -> x + y)  // 7
```

### 9.8.3 Functions as Return Values

Functions can return other functions (or closures), enabling function factories and currying.

**Type signature:**

```cursive
function create_adder(n: i32): (i32) → i32 ! ∅ {
    result move |x| -> x + n
}
```

**Examples:**

```cursive
// Function factory
function make_multiplier(factor: i32): (i32) → i32 ! ∅ {
    move |x| -> x * factor
}

let times_three = make_multiplier(3)
let result = times_three(5)  // 15

// Currying: transform multi-parameter function into chain of single-parameter functions
function add(x: i32, y: i32): i32 { x + y }

function curry_add(x: i32): (i32) → i32 ! ∅ {
    move |y| -> x + y
}

let add_five = curry_add(5)
let result = add_five(3)  // 8

// Partial application
function partial_apply<T, U, V>(
    f: (T, U) → V ! ∅,
    fixed_arg: T
): (U) → V ! ∅ {
    move |arg| -> f(fixed_arg, arg)
}

function power(base: i32, exp: i32): i32 {
    // ... compute base^exp
}

let square = partial_apply(power, _, 2)  // Fix second arg to 2
let result = square(5)  // 25
```

### 9.8.4 Effect Polymorphism in Functionals

Functionals can abstract over effects using effect polymorphism (Part II §2.9.4).

**Effect-polymorphic functional:**

```cursive
function for_each<T, ε>(items: [T], action: (T) → () ! ε)
    uses ε
{
    loop item in items {
        action(item)
    }
}
```

The effect set `ε` is a type parameter, allowing `for_each` to work with both pure and effectful actions:

```cursive
let numbers = [1, 2, 3]

// Pure action (ε = ∅)
for_each(numbers, |x| -> {
    let _ = x * 2
})

// Effectful action (ε = {io.write})
for_each(numbers, |x| -> {
    println("Number: {}", x)
})  // Requires uses io.write in calling context
```

**Multiple effect parameters:**

```cursive
function zip_with<T, U, V, ε₁, ε₂>(
    list1: [T],
    list2: [U],
    combine: (T, U) → V ! (ε₁ ∪ ε₂)
): [V]
    uses alloc.heap, ε₁, ε₂
{
    let result = Vec::new()
    let len = min(list1.len(), list2.len())
    loop i in 0..len {
        result::push(combine(list1[i], list2[i]))
    }
    result result::into_array()
}
```

### 9.8.5 Common Functional Patterns

**Map-reduce pattern:**

```cursive
function sum_squares(numbers: [i32]): i32 {
    numbers
        => map(_, |x| -> x * x)
        => fold(_, 0, |acc, x| -> acc + x)
}
```

**Function composition:**

```cursive
function compose<A, B, C, ε₁, ε₂>(
    f: (A) → B ! ε₁,
    g: (B) → C ! ε₂
): (A) → C ! (ε₁ ∪ ε₂) {
    move |x| -> g(f(x))
}

let double = |x: i32| -> x * 2
let increment = |x: i32| -> x + 1
let double_then_increment = compose(double, increment)

let result = double_then_increment(5)  // 11
```

**Predicates:**

```cursive
function all<T, ε>(items: [T], predicate: (T) → bool ! ε): bool
    uses ε
{
    loop item in items {
        if !predicate(item) {
            return false
        }
    }
    result true
}

function any<T, ε>(items: [T], predicate: (T) → bool ! ε): bool
    uses ε
{
    loop item in items {
        if predicate(item) {
            return true
        }
    }
    result false
}

let numbers = [2, 4, 6, 8]
let all_even = all(numbers, |x| -> x % 2 == 0)  // true
let any_odd = any(numbers, |x| -> x % 2 == 1)   // false
```

**Chaining operations:**

```cursive
procedure process_data(input: [i32]): [string]
    uses alloc.heap, io.write
{
    input
        => filter(_, |x| -> x > 0)                    // Keep positive
        => map(_, |x| -> x * x)                        // Square
        => filter(_, |x| -> x < 100)                   // Keep < 100
        => map(_, |x| -> x.to_string())                // Convert to string
}
```

### 9.8.6 Performance Considerations

**Inlining:**

Small function parameters can often be inlined at call sites:

```cursive
function fast_map<T, U>(items: [T], f: (T) → U ! ∅): [U]
    uses alloc.heap
{
    // If f is small (like |x| -> x * 2), compiler can inline it
    // Resulting in performance similar to hand-written loop
}
```

**Dynamic dispatch vs. monomorphization:**

```cursive
// Monomorphized: separate compiled version for each f
function process_static<F, T, U>(items: [T], f: F): [U]
where F: (T) → U ! ∅
    uses alloc.heap
{
    // Optimized per instantiation
}

// Dynamic: single compiled version, vtable dispatch
function process_dynamic(items: [i32], f: impl (i32) → i32 ! ∅): [i32]
    uses alloc.heap
{
    // More code size efficient, but slower calls
}
```

**Zero-cost abstractions:**

Well-designed higher-order functions can achieve zero overhead:

```cursive
// This functional code:
let result = numbers
    => filter(_, |x| -> x > 0)
    => map(_, |x| -> x * 2)

// Can compile to same performance as hand-written:
var result = Vec::new()
loop x in numbers {
    if x > 0 {
        result::push(x * 2)
    }
}
```

---

## 9.9 Argument Processing Details

This section consolidates the evaluation order, binding rules, and permission checking for arguments at call sites.

### 9.9.1 Complete Evaluation Order Specification

Arguments are evaluated in a strict, deterministic order to enable reasoning about side effects.

**Order specification:**

```
1. Receiver expression (if procedure call)
2. Positional argument expressions (left-to-right)
3. Named argument expressions (textual order in source)
4. Default parameter expressions (parameter list order, for unbound params)
5. Call transfer to callee
```

**Formal semantics:**

```
[Eval-Order-Procedure]
⟨receiver, σ₀⟩ ⇓ ⟨recv_val, σ₁⟩
⟨pos_arg₁, σ₁⟩ ⇓ ⟨val₁, σ₂⟩
...
⟨pos_argₘ, σₘ⟩ ⇓ ⟨valₘ, σₘ₊₁⟩
⟨named_arg₁.expr, σₘ₊₁⟩ ⇓ ⟨nval₁, σₘ₊₂⟩
...
⟨named_argₖ.expr, σₘ₊ₖ⟩ ⇓ ⟨nvalₖ, σₘ₊ₖ₊₁⟩
defaults d₁, ..., dₚ evaluated → default_vals
⟨Type::proc(recv_val, val₁, ..., valₘ, nval₁, ..., nvalₖ, default_vals), σ'⟩ ⇓ ⟨result, σ''⟩
--------------------------------------------------------
⟨receiver::proc(pos_args, named_args), σ₀⟩ ⇓ ⟨result, σ''⟩
```

**Examples:**

```cursive
procedure demonstrate_order()
    uses io.write
{
    var counter = 0

    function next(): i32 {
        counter += 1
        println("Eval {}", counter)
        result counter
    }

    // Observe evaluation order
    process(
        next(),           // Prints "Eval 1"
        next(),           // Prints "Eval 2"
        third: next(),    // Prints "Eval 3"
        fourth: next()    // Prints "Eval 4"
    )
    // Guaranteed order: 1, 2, 3, 4
}

// Procedure call order
record Point {
    x: f64
    y: f64

    procedure transform(self: Point, scale: f64, offset: f64): Point {
        // ...
    }
}

let p = { println("Receiver"); Point { x: 1.0, y: 1.0 } }
let result = p::transform(
    { println("Arg 1"); 2.0 },
    { println("Arg 2"); 3.0 }
)
// Output: "Receiver", "Arg 1", "Arg 2" (guaranteed)
```

### 9.9.2 Default Parameter Evaluation Details

Default parameters are evaluated lazily—only when not supplied at the call site.

**Evaluation context:**

Default expressions are evaluated in a context where:
- All earlier parameters (positional and named) are bound
- The default can reference earlier bound parameters
- The default must be compile-time evaluable (Part III §3.3)

**Evaluation timing:**

```
[Default-Eval-Order]
all provided arguments evaluated and bound
for each unbound parameter p with default d (in parameter list order):
    evaluate d in context with earlier parameters bound
    bind p to result of d
--------------------------------------------------------
all parameters bound
```

**Examples:**

```cursive
// Default referencing earlier parameter
function create_window(
    width: i32,
    height: i32,
    title: string = "Window",
    *,
    resizable: bool = true,
    aspect_ratio: f64 = (width as f64) / (height as f64)
): Window {
    // aspect_ratio default computed from width and height
    // ...
}

create_window(800, 600)
// Evaluates: title = "Window", resizable = true, aspect_ratio = 800.0/600.0

create_window(1920, 1080, "Game", resizable: false)
// Evaluates: aspect_ratio = 1920.0/1080.0

// Order matters for defaults
function example(
    x: i32 = 10,
    y: i32 = x * 2,      // References x
    z: i32 = x + y       // References x and y
): (i32, i32, i32) {
    (x, y, z)
}

example()           // (10, 20, 30) - all defaults evaluated in order
example(5)          // (5, 10, 15) - y and z defaults use provided x
example(5, 3)       // (5, 3, 8) - only z default evaluated
example(5, 3, 1)    // (5, 3, 1) - no defaults evaluated
```

### 9.9.3 Permission Weakening at Call Sites

Callers may implicitly weaken permissions when passing arguments, but cannot upgrade.

**Weakening lattice:**
```
own ≥ mut ≥ imm
```

**Weakening rules:**

```
[Weaken-Own-To-Mut]
argument has own permission
parameter requires mut
--------------------------------------------------------
implicit weakening: argument passed as mut (no move)

[Weaken-Own-To-Imm]
argument has own permission
parameter requires imm
--------------------------------------------------------
implicit weakening: argument passed as imm (no move)

[Weaken-Mut-To-Imm]
argument has mut permission
parameter requires imm
--------------------------------------------------------
implicit weakening: argument passed as imm

[Move-Own-To-Own]
argument has own permission
parameter requires own
--------------------------------------------------------
move occurs: argument consumed

[No-Upgrade]
argument has perm₁
parameter requires perm₂
perm₁ < perm₂
--------------------------------------------------------
ERROR (E9F/P-0204): cannot upgrade permission
```

**Explicit annotations:**

When weakening, explicit annotations clarify intent:

```cursive
function takes_imm(x: imm i32) { }
function takes_mut(x: mut i32) { }
function takes_own(x: own i32) { }

let x: own i32 = 42

takes_imm(x)        // OK: implicit weakening own → imm
takes_mut(mut x)    // OK: explicit `mut` weakening own → mut
takes_own(move x)   // OK: explicit `move` consumes x
// x is now invalid
```

**Examples:**

```cursive
record Buffer { data: [u8] }

function read_buffer(buf: imm Buffer): usize { buf.data.len() }
function modify_buffer(buf: mut Buffer) { /* ... */ }
function consume_buffer(buf: own Buffer): [u8] { buf.data }

let b: own Buffer = Buffer { data: [1, 2, 3] }

// Implicit weakenings (b remains usable)
let len = read_buffer(b)           // own → imm
modify_buffer(mut b)               // own → mut (explicit annotation)

// Consuming call
let data = consume_buffer(move b)  // own → own (explicit move)
// let len2 = read_buffer(b)        // ERROR: b consumed

// Error: cannot upgrade
let c: imm Buffer = Buffer { data: [4, 5, 6] }
// modify_buffer(mut c)             // ERROR E9F/P-0204: imm cannot upgrade to mut
```

### 9.9.4 Argument Type Checking

Each argument's type must be compatible with the corresponding parameter's type via subtyping.

**Type checking rule:**

```
[Arg-Type-Compatible]
Γ ⊢ arg : τ_arg
parameter has type τ_param
τ_arg <: τ_param  (subtyping from Part II)
--------------------------------------------------------
argument type-checks

[Arg-Type-Incompatible]
τ_arg ⊄ τ_param
--------------------------------------------------------
ERROR (E9F/P-0404): argument type incompatible with parameter type
```

**Subtyping interactions:**

```cursive
// Covariant return types (in function types)
function accepts_producer(f: () → i32 ! ∅) { }

let specific: () → i32 ! ∅ = || -> 42
let general: () → Any ! ∅ = || -> 42  // Assuming Any supertype

accepts_producer(specific)  // OK: exact match
// accepts_producer(general)  // ERROR: covariant mismatch (i32 expected)

// Effect subtyping
function accepts_pure(f: () → () ! ∅) { }
function accepts_effectful(f: () → () ! {io.write}) { }

let pure_fn: () → () ! ∅ = || -> { }
let effectful_fn: () → () ! {io.write} = || -> println("hi")

accepts_pure(pure_fn)              // OK: ∅ <: ∅
// accepts_pure(effectful_fn)      // ERROR: {io.write} ⊄ ∅

accepts_effectful(pure_fn)         // OK: ∅ <: {io.write} (pure is subtype)
accepts_effectful(effectful_fn)    // OK: {io.write} <: {io.write}
```

### 9.9.5 Named Argument Resolution

Named arguments bind by name after all positional arguments are processed.

**Resolution algorithm:**

```
For each named argument (name: expr):
    1. Verify name exists in named parameter section
    2. Verify name not already bound (no duplicates)
    3. Verify name not in positional section
    4. Bind expr to parameter named 'name'
    5. Type-check and permission-check normally
```

**Error cases:**

```
[Named-Unknown]
named argument name not in parameter list
--------------------------------------------------------
ERROR (E9F/P-0201): unknown named argument

[Named-Duplicate]
same name appears multiple times in named arguments
--------------------------------------------------------
ERROR (E9F/P-0202): duplicate named argument

[Named-Positional-Conflict]
named argument attempts to bind positional parameter
--------------------------------------------------------
ERROR (E9F/P-0205): parameter is positional, cannot be passed by name

[Named-Before-Positional]
named argument appears before positional argument in call
--------------------------------------------------------
ERROR (E9F/P-0207): positional arguments must precede named arguments
```

**Examples:**

```cursive
function configure(
    host: string,
    port: i32 = 80,
    *,
    timeout: i32 = 5000,
    secure: bool = false
) { }

// Valid calls
configure("localhost")
configure("localhost", 443)
configure("localhost", 443, timeout: 10000)
configure("localhost", timeout: 3000, secure: true)
configure("localhost", 443, secure: true, timeout: 10000)  // Order doesn't matter

// Invalid calls
// configure(timeout: 3000, "localhost")           // ERROR E9F/P-0207: named before positional
// configure("localhost", host: "other")           // ERROR E9F/P-0205: host is positional
// configure("localhost", timeout: 1000, timeout: 2000)  // ERROR E9F/P-0202: duplicate
// configure("localhost", retry: true)             // ERROR E9F/P-0201: unknown 'retry'
```

---

## 9.10 Recursion

Cursive supports both direct and mutual recursion with static analysis for termination verification where required.

### 9.10.1 Direct Recursion

A function or procedure may call itself directly.

**Well-formedness:**

Direct recursion is always syntactically valid. The function name is in scope within its own body.

**Examples:**

```cursive
// Simple direct recursion
function factorial(n: i32): i32 {
    if n <= 1 {
        result 1
    } else {
        result n * factorial(n - 1)
    }
}

// Tail-recursive (potentially optimizable)
function factorial_tail(n: i32, acc: i32 = 1): i32 {
    if n <= 1 {
        result acc
    } else {
        result factorial_tail(n - 1, acc * n)
    }
}

// Recursive procedure
record List<T> {
    head: T
    tail: Option<Box<List<T>>>

    procedure length(self: List<T>): usize {
        match self.tail {
            None => 1,
            Some(tail) => 1 + tail::unbox()::length()
        }
    }
}

// Diverging recursive function
function loop_forever(): ! {
    result loop_forever()  // Never returns
}
```

### 9.10.2 Mutual Recursion

Multiple functions or procedures may call each other cyclically. Cursive supports mutual recursion via two-phase compilation (Part III §3.1.3).

**Two-phase compilation:**

Phase 1: Declare all names and signatures
Phase 2: Check all bodies

This allows forward references:

```cursive
// Mutually recursive functions
function is_even(n: i32): bool {
    if n == 0 {
        result true
    } else {
        result is_odd(n - 1)  // Forward reference to is_odd
    }
}

function is_odd(n: i32): bool {
    if n == 0 {
        result false
    } else {
        result is_even(n - 1)
    }
}
```

**Examples:**

```cursive
// Expression evaluation with mutual recursion
enum Expr {
    Literal(i32),
    Add(Box<Expr>, Box<Expr>),
    Mul(Box<Expr>, Box<Expr>)
}

function eval(e: Expr): i32 {
    match e {
        Expr::Literal(n) => n,
        Expr::Add(l, r) => eval_add(l, r),
        Expr::Mul(l, r) => eval_mul(l, r)
    }
}

function eval_add(l: Box<Expr>, r: Box<Expr>): i32 {
    eval(*l) + eval(*r)
}

function eval_mul(l: Box<Expr>, r: Box<Expr>): i32 {
    eval(*l) * eval(*r)
}

// Mutual recursion across types
record Node {
    value: i32
    edges: [Edge]

    procedure total_weight(self: Node): f64 {
        var sum = 0.0
        loop edge in self.edges {
            sum += edge::weight_to_target()
        }
        result sum
    }
}

record Edge {
    target: Box<Node>
    weight: f64

    procedure weight_to_target(self: Edge): f64 {
        self.weight + self.target::unbox()::total_weight()
    }
}
```

### 9.10.3 Termination Analysis

Cursive does NOT require termination proofs for general functions. However:

1. **Comptime functions** MUST terminate (compilation fails if limit exceeded)
2. **Type-level computation** must terminate
3. **Diverging functions** must be explicitly typed with `!`

**Non-terminating functions:**

```
[Diverge-Explicit]
function never returns
--------------------------------------------------------
return type MUST be ! (never type)

[Diverge-Implicit-Error]
function inferred to never return
return type is not !
--------------------------------------------------------
ERROR (E9F-0303): function may not terminate; use return type !
```

**Examples:**

```cursive
// Explicit non-termination
function infinite_loop(): ! {
    loop {
        // Never exits
    }
}

// Potentially non-terminating (OK without proof)
function collatz(n: i32): i32 {
    if n == 1 {
        result 1
    } else if n % 2 == 0 {
        result collatz(n / 2)
    } else {
        result collatz(3 * n + 1)
    }
    // No termination proof required
}

// Comptime MUST terminate
comptime function factorial_ct(n: i32): i32 {
    if n <= 1 {
        result 1
    } else {
        result n * factorial_ct(n - 1)
    }
}

const FACT_10: i32 = factorial_ct(10)  // OK: terminates
// const FACT_NEG: i32 = factorial_ct(-5)  // ERROR: exceeds comptime limits
```

### 9.10.4 Recursive Contracts

Recursive functions may have contracts. Preconditions and postconditions are checked at each recursive call.

**Contract checking:**

```cursive
// Precondition checked at each call
function fibonacci(n: i32): i32
    must n >= 0
{
    if n <= 1 {
        result n
    } else {
        result fibonacci(n - 1) + fibonacci(n - 2)
        // Preconditions checked: (n-1) >= 0 and (n-2) >= 0
    }
}

// Postcondition with result
function sum_to(n: i32): i32
    must n >= 0
    will result >= 0
{
    if n == 0 {
        result 0
    } else {
        result n + sum_to(n - 1)
    }
    // Postcondition checked at each return
}

// Recursive procedure with postcondition
record Tree {
    value: i32
    left: Option<Box<Tree>>
    right: Option<Box<Tree>>

    procedure size(self: Tree): usize
        will result >= 1
    {
        var count = 1
        match self.left {
            Some(left) => count += left::unbox()::size(),
            None => {}
        }
        match self.right {
            Some(right) => count += right::unbox()::size(),
            None => {}
        }
        result count
    }
}
```

### 9.10.5 Stack Overflow Considerations

Recursive functions use stack space. Deep recursion may overflow the stack.

**Implementation considerations (informative):**

- Stack size is implementation-defined
- No guaranteed tail call optimization (except with `[[must_tail]]`)
- Deep recursion may require heap-allocated data structures (explicit)

**Tail recursion:**

Tail-recursive functions (where recursive call is last operation) MAY be optimized to iteration by implementations:

```cursive
// Tail-recursive (optimizable)
function sum_tail(n: i32, acc: i32 = 0): i32 {
    if n == 0 {
        result acc
    } else {
        result sum_tail(n - 1, acc + n)  // Tail position
    }
}

// Not tail-recursive (cannot optimize)
function sum_recursive(n: i32): i32 {
    if n == 0 {
        result 0
    } else {
        result n + sum_recursive(n - 1)  // Not tail position (addition after)
    }
}

// Enforce tail call optimization with attribute
[[must_tail]]
function loop_with_accumulator(n: i32, acc: i32): i32 {
    if n == 0 {
        result acc
    } else {
        result loop_with_accumulator(n - 1, acc + n)  // Must be tail call or error
    }
}
```

---

## 9.11 Integration with Other Features

This section describes how functions and procedures interact with other major language features.

### 9.11.1 Functions and Regions

Functions interact with the region system (Part XIII) for memory safety.

**Region boundaries:**

```
[Func-Region-Boundary]
function f allocates in region r_callee
values from r_callee cannot escape to caller
--------------------------------------------------------
region boundary enforced at return

[Func-Return-Region-Bound]
function returns reference with lifetime 'a
'a must be tied to an input parameter's lifetime
--------------------------------------------------------
reference cannot outlive source
```

**Examples:**

```cursive
// Region-local allocation cannot escape
function create_local(): &i32 {
    let local = 42
    // result &local  // ERROR: local's region ends at function return
}

// Region-polymorphic function ties output to input
function first<'r, T>(slice: &'r [T]): &'r T {
    result &slice[0]  // Lifetime 'r ties result to input
}

// Procedure with region-bound return
record Container<T> {
    items: Vec<T>

    procedure get<'a, T>(self: &'a Container<T>, index: usize): &'a T {
        result &self.items[index]  // Tied to self's lifetime
    }
}
```

### 9.11.2 Functions and Modal Types

Functions and procedures can operate on modal types (Part X), enabling typestate programming.

**State-dependent procedures:**

```cursive
modal File {
    states { Closed, Open }

    // State-specific constructor
    function File@Closed::new(path: string): File@Closed {
        File@Closed { path, handle: None }
    }

    // Transition: Closed → Open
    procedure File@Closed::open(self: own File@Closed): Result<File@Open, Error>
        uses fs.open
    {
        match sys_open(self.path) {
            Ok(handle) => Ok(File@Open { path: self.path, handle: Some(handle) }),
            Err(e) => Err(e)
        }
    }

    // Only callable in Open state
    procedure File@Open::read(self: File@Open, buf: mut [u8]): Result<usize, Error>
        uses fs.read
    {
        sys_read(self.handle.unwrap(), buf)
    }

    // Transition: Open → Closed
    procedure File@Open::close(self: own File@Open): File@Closed
        uses fs.close
    {
        sys_close(self.handle.unwrap())
        File@Closed { path: self.path, handle: None }
    }
}

// Usage enforces state transitions
procedure process_file(path: string): Result<(), Error>
    uses fs.open, fs.read, fs.close
{
    let file_closed = File::new(path)
    let file_open = file_closed::open()?  // Transition to Open

    var buf = [0u8; 1024]
    file_open::read(buf)?  // Only works in Open state

    let file_closed = file_open::close()  // Transition back to Closed
    result Ok(())
}
```

### 9.11.3 Functions and Metaprogramming

Comptime functions enable compile-time metaprogramming (Part XI).

**Comptime function integration:**

```cursive
// Comptime functions in const contexts
comptime function generate_lookup_table(size: usize): [u8; size] {
    var table = [0u8; size]
    loop i in 0..size {
        table[i] = (i * 2) as u8
    }
    result table
}

const LOOKUP: [u8; 256] = generate_lookup_table(256)

// Comptime functions in type-level computation
comptime function next_power_of_two(n: usize): usize {
    var power = 1
    loop power < n {
        power *= 2
    }
    result power
}

type AlignedBuffer<const N: usize> = [u8; next_power_of_two(N)]

// Comptime functions in default parameters
comptime function default_capacity(): usize { 16 }

function create_vec<T>(capacity: usize = default_capacity()): Vec<T> {
    Vec::with_capacity(capacity)
}
```

**Restrictions:**

- Comptime functions can only call other comptime functions
- No runtime I/O or effects
- Allocation only in compile-time arena
- Must terminate within implementation limits

### 9.11.4 Functions and Unsafe Code

Unsafe functions represent operations that require manual safety verification.

**Unsafe boundaries:**

```cursive
// Unsafe function performing raw pointer operations
unsafe function ptr_read<T>(ptr: *const T): T
    uses unsafe.ptr
    must ptr != null
{
    result *ptr  // Dereferences raw pointer
}

// Safe wrapper establishing safety invariants
function safe_read<T>(ptr: *const T): Option<T>
    uses unsafe.ptr
{
    if ptr == null {
        result None
    }

    unsafe {
        result Some(ptr_read(ptr))
    }
}

// Unsafe function with contract
unsafe function assume_sorted<T>(slice: mut [T])
    must slice.len() > 0
{
    // Marks slice as sorted without verification
    // Caller responsible for correctness
}

// FFI functions are inherently unsafe
extern "C" {
    function malloc(size: usize): *mut u8;
    function free(ptr: *mut u8);
}

procedure allocate_raw(size: usize): *mut u8
    uses ffi.call
{
    unsafe {
        result malloc(size)
    }
}
```

**Safety encapsulation pattern:**

```cursive
// Unsafe low-level implementation
unsafe function vec_push_unchecked<T>(vec: mut Vec<T>, item: T)
    uses unsafe.ptr
    must vec.len() < vec.capacity()
{
    // Unchecked push (caller ensures capacity)
    // ...
}

// Safe high-level interface
record Vec<T> { /* fields omitted */
    procedure push(self: mut Vec<T>, item: T)
        uses alloc.heap
    {
        if self.len() == self.capacity() {
            self::grow()
        }

        unsafe {
            vec_push_unchecked(self, item)
        }
    }
}
```

### 9.11.5 Functions and Contracts

Functions integrate with the contract system (Part VII) for design-by-contract programming.

**Contract clauses:**

```cursive
// Preconditions constrain inputs
function divide(numerator: i32, denominator: i32): i32
    must denominator != 0
{
    result numerator / denominator
}

// Postconditions guarantee outputs
function abs(x: i32): i32
    will result >= 0
{
    if x < 0 {
        result -x
    } else {
        result x
    }
}

// Invariant preservation
record Counter {
    value: i32

    procedure increment(self: mut Counter)
        must self.value < i32::MAX
        will self.value > old(self.value)
    {
        self.value += 1
    }

    // Frame conditions (what doesn't change)
    procedure reset(self: mut Counter, new_value: i32)
        will self.value == new_value
    {
        self.value = new_value
    }
}
```

### 9.11.6 Functions and Effect System

Functions declare and propagate effects through the effect system (Part VII).

**Effect propagation:**

```cursive
// Effect composition
function read_and_log(path: string): Result<string, Error>
    uses fs.read, io.write
{
    println("Reading file: {}", path)
    let content = fs::read_to_string(path)?
    println("Read {} bytes", content.len())
    result Ok(content)
}

// Effect polymorphism
function with_retry<T, ε>(
    operation: () → Result<T, Error> ! ε,
    max_attempts: i32
): Result<T, Error>
    uses ε
{
    loop attempt in 1..=max_attempts {
        match operation() {
            Ok(value) => return Ok(value),
            Err(e) => {
                if attempt == max_attempts {
                    return Err(e)
                }
            }
        }
    }
    unreachable()
}

// Effect override with `with` blocks
procedure custom_logging()
    uses io.write
{
    with {
        implement io.write {
            procedure write(msg: string) {
                // Custom logging implementation
                sys_log(msg)
            }
        }
    } {
        println("This uses custom logging")
    }
}
```

---

## 9.12 Function Attributes

Attributes modify function behavior, provide compiler hints, and control code generation.

### 9.12.1 Optimization Attributes

**`[[inline]]`** — Suggest inlining this function:

```cursive
[[inline]]
function small_helper(x: i32): i32 {
    x * 2 + 1
}
```

**`[[no_inline]]`** — Prevent inlining (for debugging, profiling):

```cursive
[[no_inline]]
function debug_checkpoint() {
    // Keep as separate stack frame for debugging
}
```

**`[[inline(always)]]`** — Strong inline hint:

```cursive
[[inline(always)]]
function critical_path(x: i32): i32 {
    // Always inline for performance
}
```

**`[[hot]]`** — Mark as hot path (optimize aggressively):

```cursive
[[hot]]
function inner_loop(data: [i32]): i32 {
    // Frequently called, optimize heavily
}
```

**`[[cold]]`** — Mark as cold path (optimize for size):

```cursive
[[cold]]
function error_handler(error: Error) {
    // Rarely called, optimize for code size
}
```

### 9.12.2 FFI Attributes

**`[[no_mangle]]`** — Disable name mangling:

```cursive
[[no_mangle]]
function exported_function(x: i32): i32 {
    result x * 2
}
// Exports as "exported_function" exactly
```

**`[[export_name("name")]]`** — Control exported symbol name:

```cursive
[[export_name("my_api_v1_process")]]
function process(data: [u8]): Result<(), Error> {
    // Exports as "my_api_v1_process"
}
```

**`[[link_name("name")]]`** — Control linked symbol name:

```cursive
extern "C" {
    [[link_name("__platform_specific_malloc")]]
    function malloc(size: usize): *mut u8;
}
```

### 9.12.3 Diagnostic Attributes

**`[[deprecated]]`** and **`[[deprecated("message")]]`** — Mark as deprecated:

```cursive
[[deprecated]]
function old_api() {
    // Compiler warns on use
}

[[deprecated("Use new_api() instead")]]
function legacy_api() {
    // Compiler warns with custom message
}
```

**`[[must_use]]`** and **`[[must_use("reason")]]`** — Result must be used:

```cursive
[[must_use]]
function allocate_buffer(size: usize): Buffer {
    // Compiler warns if result ignored
}

[[must_use("Error must be handled")]]
function fallible_operation(): Result<(), Error> {
    // Compiler warns with reason if ignored
}
```

**`[[allow(lint)]]`, `[[warn(lint)]]`, `[[deny(lint)]]`** — Configure lints:

```cursive
[[allow(unused_variables)]]
function debug_placeholder(x: i32) {
    // Allow unused x for debugging
}

[[deny(unsafe_code)]]
function must_be_safe() {
    // Error on any unsafe blocks
}
```

### 9.12.4 Semantic Attributes

**`[[must_tail]]`** — Enforce tail call optimization:

```cursive
[[must_tail]]
function tail_recursive(n: i32, acc: i32): i32 {
    if n == 0 {
        result acc
    } else {
        result tail_recursive(n - 1, acc + n)  // Must be optimized or compile error
    }
}
```

**`[[test]]`** — Mark as test function:

```cursive
[[test]]
function test_addition() {
    assert(add(2, 3) == 5)
}
```

**`[[bench]]`** — Mark as benchmark function:

```cursive
[[bench]]
function bench_sorting(b: Bencher) {
    b::iter(|| -> sort([5, 2, 8, 1, 9]))
}
```

### 9.12.5 Attribute Composition

Multiple attributes may be applied to a single function:

```cursive
[[inline]]
[[must_use("Resource must be freed")]]
[[deprecated("Use new_allocator() instead")]]
function old_allocator(size: usize): Buffer {
    // Multiple attributes active
}

[[no_mangle]]
[[export_name("api_v2_process")]]
[[cold]]
function exported_handler(code: i32): i32 {
    // FFI + optimization attributes combined
}
```

---

## 9.13 Grammar Additions and Updates

The canonical grammar resides in Appendix A. This section lists deltas introduced by Part IX.

### Delta 1: Arrow Function Types (Replaces `map(...)`)

```ebnf
(* Function types use arrow notation *)
FunctionType     ::= "(" TypeList? ")" "→" Type EffectAnnotation?
EffectAnnotation ::= "!" EffectSet

(* Effect-polymorphic function types *)
EffectPolyType   ::= "∀" EffectVar EffectBound? "." FunctionType
EffectBound      ::= "where" EffectVar "⊆" "{" EffectSet "}"
EffectVar        ::= Ident

(* REMOVED: MapType production is deprecated *)
```

### Delta 2: Expression-Bodied Callables

```ebnf
(* Expression-bodied functions *)
FunctionDecl ::= Attribute* Visibility? "function" Ident
                  GenericParams? "(" ParamList? ")" ":" Type
                  WhereClause? ContractClause*
                  ( Block | "=" Expr )

(* Expression-bodied procedures *)
ProcedureDecl ::= Attribute* Visibility? "procedure" Ident
                   GenericParams? "(" ParamList? ")" ":" Type
                   WhereClause? ContractClause*
                   ( Block | "=" Expr )
```

### Delta 3: Default Parameters

```ebnf
Param ::= Ident ":" Permission? Type ("=" ConstExpr)?
```

### Delta 4: Named Arguments (Star Separator)

```ebnf
ParamList  ::= PositionalParams ("," "*" ("," NamedParams)?)?
             | "*" ("," NamedParams)?

PositionalParams ::= Param ("," Param)*
NamedParams      ::= Param ("," Param)*

(* Call site *)
ArgList ::= PositionalArgs ("," NamedArg)*
PositionalArgs ::= Expr ("," Expr)*
NamedArg ::= Ident ":" Expr
```

### Delta 5: Type-Scope Procedure Calls

```ebnf
(* Procedure call with :: *)
ProcedureCallExpr ::= Expr "::" Ident GenericArgs? "(" ArgList? ")"
                    | QualifiedName "::" Ident GenericArgs? "(" ArgList? ")"

(* Field access remains with . *)
FieldAccessExpr ::= Expr "." Ident
```

### Delta 6: Unsafe Functions

```ebnf
UnsafeFunctionDecl ::= Attribute* Visibility? "unsafe" "function" Ident
                        GenericParams? "(" ParamList? ")" ":" Type
                        WhereClause? ContractClause* Block
```

### Delta 7: Extern / FFI Declarations

```ebnf
ExternBlock         ::= "extern" StringLit? "{" ExternItem* "}"
ExternItem          ::= ExternFunctionDecl | ExternStaticDecl
ExternFunctionDecl  ::= Attribute* "function" Ident "(" ExternParamList? ")" ":" Type ";"
ExternParamList     ::= ExternParam ("," ExternParam)*
ExternParam         ::= Ident ":" Type | "..."
```

### Delta 8: Comptime Functions

```ebnf
ComptimeFunctionDecl ::= Attribute* Visibility? "comptime" "function" Ident
                          GenericParams? "(" ParamList? ")" ":" Type
                          WhereClause? Block
```

### Delta 9: Self Parameter Syntax

```ebnf
SelfParam ::= "self" ":" Permission? "Self"
            | "$"
```

### Delta 10: Closure Expressions

```ebnf
ClosureExpr ::= "move"? "|" ClosureParamList "|" ( "->" Expr | Block )

ClosureParamList ::= (Ident (":" Type)?) ("," Ident (":" Type)?)*
```

---

## 9.14 Diagnostics

This section catalogs errors, warnings, and lints related to functions and procedures.

### 9.14.1 Function Declaration Errors (E9F-01xx)

| Code | Description | Example |
|------|-------------|---------|
| E9F-0101 | Unsafe function call requires `unsafe` block | `dangerous()` outside unsafe |
| E9F-0102 | Comptime function body not compile-time evaluable | `comptime function f() { io::read() }` |
| E9F-0103 | Comptime function called outside const context | `let x = comptime_fn(y)` where y is runtime |
| E9F-0104 | Generic function parameters cannot have defaults | `function f<T = i32>()` |
| E9F-0105 | Multiple definitions of entry point `main` | Two `main` functions |
| E9F-0106 | Entry point `main` not found | Executable without `main` |
| E9F-0107 | Comptime function must be pure (`uses ∅`) | `comptime function f() uses io.write` |
| E9F-0108 | Comptime evaluation exceeded implementation limits | Infinite recursion in comptime |
| E9F-0109 | Return type annotation required | `function f(x: i32) { x }` (missing return type) |

### 9.14.2 Procedure Declaration Errors (E9P-01xx)

| Code | Description | Example |
|------|-------------|---------|
| E9P-0101 | Procedure missing `self` parameter | `procedure p(): i32` (no self) |
| E9P-0102 | Self parameter type must be `Self` | `procedure p(self: U)` where U ≠ Self |
| E9P-0103 | Cannot use both `$` and explicit `self` | `procedure p($, self: T)` |

### 9.14.3 Self Parameter Errors (E9F-93xx)

| Code | Description | Example |
|------|-------------|---------|
| E9F-9301 | Cannot reassign `self` parameter | `self = other` in procedure body |
| E9F-9302 | Cannot shadow `self` parameter | `let self = other` in procedure body |

**Note**: The `self` binding is special and cannot be reassigned or shadowed, regardless of its permission level (imm, mut, or own). See §9.3.3 for complete semantics.

### 9.14.4 Parameter Errors (E9F/P-02xx)

| Code | Description | Example |
|------|-------------|---------|
| E9F/P-0201 | Unknown named argument | `f(x: 10)` where `x` not a parameter |
| E9F/P-0202 | Duplicate named argument | `f(x: 10, x: 20)` |
| E9F/P-0203 | Missing required argument | `f()` where f requires parameter |
| E9F/P-0204 | Insufficient permission | Passing `imm` where `mut` required |
| E9F/P-0205 | Positional parameter passed by name | `f(x: 10)` where `x` is positional |
| E9F/P-0206 | Default references later parameter | `f(x: i32 = y, y: i32)` |
| E9F/P-0207 | Named arguments must follow positional | `f(x: 10, 20)` |

### 9.14.5 Return Value Errors (E9F/P-03xx)

| Code | Description | Example |
|------|-------------|---------|
| E9F-0301 | Missing return on some control paths | Function without `return`/`result` on all paths |
| E9F-0302 | `return` not allowed in expression-bodied | `function f(): i32 = return 10` |
| E9F-0303 | Function may not terminate; use return type `!` | Inferred divergence without `!` type |

### 9.14.6 Call Errors (E9F/P-04xx)

| Code | Description | Example |
|------|-------------|---------|
| E9F/P-0401 | Procedure not found in type scope | `receiver::nonexistent()` |
| E9F/P-0402 | Insufficient receiver permission | `imm_receiver::mutating_proc()` |
| E9F/P-0403 | Wrong number of arguments (arity mismatch) | `f(1, 2)` where f takes 1 argument |
| E9F/P-0404 | Argument type mismatch | `f("hello")` where f expects `i32` |
| E9F/P-0405 | Required effects not available | `pure_fn() { effectful_call() }` |
| E9F/P-0410 | Name not found in any scope | `undefined_function()` |
| E9F/P-0411 | Ambiguous call (multiple candidates) | Name collision in imports |
| E9F/P-0413 | Type parameter inference failed | `collect()` without type annotation |

### 9.14.7 Closure Errors (E9F/P-05xx)

| Code | Description | Example |
|------|-------------|---------|
| E9F/P-0501 | Capturing closure cannot coerce to function pointer | `let fp: fn() = \|\| -> captured_var` |

### 9.14.8 FFI Errors (E9F/P-06xx)

| Code | Description | Example |
|------|-------------|---------|
| E9F-0601 | Variadic parameters allowed only in extern | `function f(x: i32, ...): i32` |
| E9F-0602 | Extern call requires `uses ffi.call` | Calling FFI without declaring effect |
| E9F-0603 | Variadic parameter must be last | `function f(..., x: i32)` in extern |

### 9.14.9 Attribute Errors (E9F/P-07xx)

| Code | Description | Example |
|------|-------------|---------|
| E9F/P-0701 | Unknown attribute | `[[unknown_attr]]` |
| E9F/P-0702 | Attribute requires argument | `[[export_name]]` without string |
| E9F/P-0703 | Attribute not applicable to this item | `[[must_tail]]` on non-tail-recursive |

### 9.14.10 Warnings

| Code | Description | Example |
|------|-------------|---------|
| W9F/P-0001 | Unused function parameter | `function f(unused: i32): i32 { 0 }` |
| W9F/P-0002 | Deprecated function called | Calling `[[deprecated]]` function |
| W9F/P-0003 | Result of `[[must_use]]` function ignored | `must_use_fn();` without binding |
| W9F/P-0004 | Unreachable code after `return` | Statements after `return` |

### 9.14.11 Selected Diagnostic Examples

**E9F/P-0204: Insufficient Permission**

```cursive
record Example {
    value: i32

    procedure mutate(self: mut Example) { ... }
}

let e: imm Example = Example { value: 0 }
e::mutate()
// ERROR E9F/P-0204: insufficient permission for procedure call
// note: procedure requires `mut` permission
// note: receiver has `imm` permission
// help: consider declaring `e` as `mut`
```

**E9F/P-0401: Procedure Not Found**

```cursive
record Point { x: f64, y: f64 }

let p = Point { x: 1.0, y: 1.0 }
p::rotate(90.0)
// ERROR E9F/P-0401: no procedure named `rotate` found for type `Point`
// help: available procedures: `translate`, `scale`, `magnitude`
```

**E9F/P-0405: Required Effects Not Available**

```cursive
function read_file(path: string): string
    uses fs.read
{ ... }

function pure_context() {
    let content = read_file("/tmp/file.txt")
// ERROR E9F/P-0405: effect `fs.read` not available in this context
// note: function `pure_context` is pure (no `uses` clause)
// help: add `uses fs.read` to function signature
}
```

**E9F/P-0413: Type Parameter Inference Failed**

```cursive
function collect<T>(): Vec<T> { Vec::new() }

let v = collect()
// ERROR E9F/P-0413: cannot infer type parameter `T`
// help: provide explicit type: `collect::<i32>()`
// help: or use type ascription: `let v: Vec<i32> = collect()`
```

---

**Previous**: [Holes and Inference](08_Holes-and-Inference.md) | **Next**: [Modals](10_Modals.md)
