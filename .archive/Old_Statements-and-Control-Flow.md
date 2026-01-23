# The Cursive Language Specification

**Part**: VI - Statements and Control Flow
**File**: 06_Statements-and-Control-Flow.md
**Previous**: [Expressions and Operators](05_Expressions-and-Operators.md) | **Next**: [Contracts and Effects](07_Contracts-and-Effects.md)

---

## Abstract

This chapter specifies statement execution through judgment `⟨s, σ⟩ ⇓ outcome` with control outcomes (Normal, Return, Break, Continue, Panic). Structured control flow only: no `goto`. Integration with Part II (types, permissions), Part VII (effects, contracts), Part VIII (regions).

---

**Definition 6.2 (Statement Foundations):** This section establishes the scope, formal foundations, abstract syntax, execution semantics, and cross-part dependencies for the statement system.

## 6.0 Statement Foundations

### 6.0.1 Scope

This chapter provides the complete normative specification of statement syntax, static well-formedness, and dynamic execution semantics for all statement forms in Cursive.

**Normative coverage:**

- Abstract syntax for statements (extension of Foundations §3.3)
- Statement execution judgment `⟨s, σ⟩ ⇓ outcome` with control outcome classification
- Complete statement grammar (EBNF)
- Static semantics (well-formedness, type checking) for each statement form
- Dynamic semantics (execution rules) for each statement form
- Control transfer semantics (break, continue, return) and outcome propagation
- Defer execution order and interaction with control flow
- Modal-aware branching and state refinement
- Region-scoped iteration discipline
- Loop verification contracts (invariants, variants, postconditions)
- Block-scoped contracts and effect narrowing
- No-panic sections (`[[no_panic]]` attribute integration)
- Effect-gated compile-time branching
- Typed early-exit via labeled blocks
- Complete diagnostic catalog (E6001-E6018)

**Statement forms specified:**

- Block statements (unit and value-producing with `result`)
- Variable declaration statements (`let`, `var`) - references Part III for full semantics
- Assignment statements (simple and compound)
- Expression statements
- Labeled statements (loops and blocks)
- Return statements
- Break statements (with optional label and value)
- Continue statements (with optional label)
- Defer statements
- Empty statements

**This chapter defers normatively to:**

- **Foundations §2.7** — Statement termination rules (newline-based with four continuation cases)
- **Declarations §3.7** — Label scope and uniqueness (function-wide scope)
- **Declarations §3.2.7** — Definite assignment analysis
- **Expressions (Part V)** — Typing and evaluation of control-flow expressions (if, match, loops when used as expressions)
- **Foundations §4.3** — Attribute definitions (`[[verify]]`, `[[overflow_checks]]`, `[[no_panic]]`)

**This chapter coordinates with:**

- **Part II (Types)** — Modal types and state transitions
- **Part III (Declarations)** — Variable bindings, scopes, visibility
- **Part V (Expressions)** — Value categories, expression typing, control-flow expression evaluation
- **Part VII (Contracts and Effects)** — Contract checking, effect propagation
- **Part VIII (Regions)** — Region allocation and escape analysis

### 6.0.2 Cross-Part Dependencies

This chapter integrates with the following specification parts:

**Part I (Foundations):**

- §1.3: Metavariables (s, σ, Γ, Δ) - statement and store notation
- §1.4: Judgment forms (execution judgments)
- §1.6: Inference rule format
- §2.7: Statement termination rules (SOLE AUTHORITY - newlines and continuation)
- §3.3: Abstract syntax (expression language; extended here with statements)
- §4.3: Attributes (`[[verify]]`, `[[overflow_checks]]`, `[[no_panic]]`)

CITE: Part I — Foundations.

**Part II (Types):**

- §2.3.3: Modal types and state transitions
- §2.0.8.1: Permissions (own, mut, imm) and permission checking
- §2.0.8.5: Modal transition notation `@S₁ →ₘ @S₂`

CITE: Part II — Type System.

**Part III (Declarations and Scope):**

- §3.2: Variable declarations (`let`, `var`) - authoritative for syntax/binding
- §3.2.7: Definite assignment analysis
- §3.2.8: Assignment to immutable binding (E3D10)
- §3.7.1: Scope hierarchy (block, function, module)
- §3.7: Label scope (function-wide) and uniqueness - AUTHORITATIVE

CITE: Part III — Declarations and Scope.

**Part V (Expressions):**

- §5.0.4: Value categories (lvalue, rvalue, place)
- §5.2.4: Block expressions and `result` keyword requirement (E6401)
- §5.7: Conditional expressions (if, if-let)
- §5.8: Match expressions with required type annotations (E6402)
- §5.9: Loop expressions (infinite, conditional, iterator) with E6404
- §5.9.5-6: Break and continue expressions
- §5.14: Return expressions
- §5.15: Defer expressions
- §5.18: Sequencing and statement separators

CITE: Part V — Expressions and Operators.

**Part VII (Contracts and Effects):**

- Contract clause syntax (`uses`, `must`, `will`)
- Effect checking and propagation
- Verification modes (`[[verify(static|runtime|none)]]`)

CITE: Part VII — Contracts and Effects.

**Part VIII (Regions):**

- Region block syntax and semantics
- Escape analysis
- Per-region allocation and deallocation

CITE: Part VIII — Regions.

### 6.0.3 Design Principles

CITE: Part I — Design Philosophy.

### 6.0.4 Notational Conventions

This chapter extends the metavariables from Foundations §1.3 with statement-specific notation.

**Statement Metavariables:**

```
s, s₁, s₂ ∈ Stmt          (statements)
b, b₁, b₂ ∈ Block         (block statements)
label, ℓ ∈ Label          (label identifiers)
```

**Execution Context Metavariables:**

```
σ, σ', σ'' ∈ Store        (memory stores: Location → Value)
Γ ∈ Context               (type contexts: Var → Type)
Δ ∈ RegionCtx             (region contexts: stack of active regions)
Σ ∈ StateCtx              (modal state contexts)
```

**Control Outcome Metavariables:**

```
outcome ∈ Outcome ::= Normal(σ')
                     | Return(v, σ')
                     | Break(label?, v?, σ')
                     | Continue(label?, σ')
                     | Panic
```

**Reused from Foundations:**

```
e, e₁, e₂ ∈ Expr          (expressions)
v, v₁, v₂ ∈ Value         (values)
x, y, z ∈ Var             (variables)
τ, υ, ρ ∈ Type            (types)
@S, @S' ∈ State           (modal states)
ε, ε₁, ε₂ ∈ Effect        (effect sets)
P, Q, R ∈ Assertion       (contract assertions)
```

CITE: §1.3 — Metavariables; §1.4 — Judgment Forms.

### 6.0.5 Abstract Syntax for Statements

**Definition 6.3 (Statement Abstract Syntax):** The abstract syntax of statements extends the expression language from Foundations §3.3 to include imperative constructs.

```
s ::= { s₁; ...; sₙ }                     (block - sequence of statements)
    | { s₁; ...; sₙ; result e }           (value-producing block)
    | let x : τ = e                       (immutable binding)
    | var x : τ = e                       (mutable binding)
    | shadow (let|var) x : τ = e          (shadowing binding)
    | lvalue = e                          (simple assignment)
    | lvalue ⊕= e                         (compound assignment, ⊕ ∈ {+,-,*,/,%,&,|,^,<<,>>})
    | e                                   (expression statement)
    | 'label: s                           (labeled statement)
    | return e?                           (early return)
    | break 'label? e?                    (loop/block exit)
    | continue 'label?                    (loop continue)
    | defer { s₁; ...; sₙ }               (deferred execution)
    | ;                                   (empty statement)

lvalue ::= x                              (variable)
         | e.f                            (field access)
         | e.n                            (tuple projection)
         | e[i]                           (array/slice indexing)
         | *ptr                           (pointer dereference)
```

**Note on control-flow expressions:** Conditional expressions (`if`, `match`), loop expressions (`loop`), and other control constructs are defined in Part V (Expressions) but MAY be used in statement position. This chapter specifies their behavior as statements.

**Statement vs Expression distinction:**

- **Expressions** produce values and have type τ; evaluation judgment `⟨e, σ⟩ ⇓ ⟨v, σ'⟩`
- **Statements** execute for effect and control flow; execution judgment `⟨s, σ⟩ ⇓ outcome`
- A statement MAY consist of an expression used for its side effects (expression statement)
- Control-transfer statements (`return`, `break`, `continue`) produce divergent outcomes

CITE: Foundations §3.3 — Expression Language; Part V — Expressions and Operators.

### 6.0.6 Execution Judgment and Control Outcomes

**Definition 6.4 (Statement Execution):** The execution judgment `⟨s, σ⟩ ⇓ outcome` denotes that statement s, when executed in store σ, produces a control outcome.

**Control Outcome Classification:**

```
Outcome ::= Normal(σ')                    (normal completion with updated store)
          | Return(v, σ')                 (early return from function with value v)
          | Break(label?, v?, σ')         (exit loop or labeled block)
          | Continue(label?, σ')          (continue to next loop iteration)
          | Panic                         (abnormal termination)
```

**Outcome semantics:**

- **Normal(σ')**: Statement completed successfully; execution continues with updated store σ'
- **Return(v, σ')**: Function returns immediately with value v (after executing defers)
- **Break(label, v, σ')**: Exit labeled construct with optional value v
- **Continue(label, σ')**: Skip to next iteration of labeled loop
- **Panic**: Abnormal termination; unwind stack executing defers (or abort if no unwind)

**Outcome propagation:**

Control outcomes propagate through statement sequences:

```
[Outcome-Propagate-Normal]
⟨s₁, σ⟩ ⇓ Normal(σ')
⟨s₂, σ'⟩ ⇓ outcome
----------------------------
⟨s₁; s₂, σ⟩ ⇓ outcome

[Outcome-Propagate-Transfer]
⟨s₁, σ⟩ ⇓ outcome    where outcome ∈ {Return, Break, Continue, Panic}
----------------------------------------------------
⟨s₁; s₂, σ⟩ ⇓ outcome    (s₂ not executed)
```

Non-normal outcomes bypass remaining statements in a sequence.

**Judgment forms:**

```
⟨s, σ⟩ ⇓ outcome          (statement execution)
Γ ⊢ s : ()                (statement well-formedness and typing)
Γ ⊢ s reachable           (statement is reachable on some control path)
```

CITE: §1.4 — Judgment Forms; §1.6 — Inference Rule Format.

---

**Definition 6.5 (Statement Grammar):** This section provides the complete concrete syntax of all statement forms in EBNF.

## 6.1 Complete Statement Grammar

The canonical statement grammar appears in Foundations Appendix A.4. This chapter references those productions when describing individual statements.

## 6.2 Block Statements

### 6.2.1 Syntax

The production `BlockStmt` in Appendix A.4 defines the two recognised forms:

1. **Unit block**: Contains only statements; evaluates to `()` (unit type)
2. **Value-producing block**: Contains statements followed by `result expr`; evaluates to type of `expr`

**Rule 6.2.1-1 (Block Result Annotation).** A block that produces a non-unit value MUST use the `result` keyword; implicit returns are forbidden.

**Examples:**

```cursive
// Unit block (statement form)
{
    let x = compute()
    process(x)
}  // Type: ()

// Value-producing block (expression form)
let value: i32 = {
    let x = compute()
    result x * 2  // REQUIRED: explicit result
}  // Type: i32

// ERROR: implicit return
let invalid = {
    let x = compute()
    x * 2  // ERROR E4401: missing 'result' keyword
}
```

### 6.2.2 Static Semantics

Block well-formedness: unit blocks type to `()`, result blocks require `result` keyword and type annotation on binding. CITE: Part V §5.2.4 (authoritative block semantics).

### 6.2.3 Dynamic Semantics

**Unit block execution:**

```
[Exec-Block-Unit]
⟨s₁, σ⟩ ⇓ Normal(σ₁)    ...    ⟨sₙ, σₙ₋₁⟩ ⇓ Normal(σₙ)
------------------------------------------------------
⟨{ s₁; ...; sₙ }, σ⟩ ⇓ Normal(σₙ)

[Exec-Block-Unit-Transfer]
⟨s₁, σ⟩ ⇓ Normal(σ₁)    ...    ⟨sᵢ, σᵢ₋₁⟩ ⇓ outcome
outcome ∈ {Return, Break, Continue, Panic}
------------------------------------------------------
⟨{ s₁; ...; sₙ }, σ⟩ ⇓ outcome    (sᵢ₊₁...sₙ not executed)
```

**Value-producing block execution:**

```
[Exec-Block-Result]
⟨s₁, σ⟩ ⇓ Normal(σ₁)    ...    ⟨sₙ, σₙ₋₁⟩ ⇓ Normal(σₙ)
⟨result e, σₙ⟩ ⇓ ⟨v, σ'⟩                              (evaluate result expression)
------------------------------------------------------
⟨{ s₁; ...; sₙ; result e }, σ⟩ ⇓ Normal(σ')
    with result value v

[Exec-Block-Result-Transfer]
⟨sᵢ, σᵢ₋₁⟩ ⇓ outcome    where outcome ∈ {Return, Break, Continue, Panic}
------------------------------------------------------
⟨{ s₁; ...; sₙ; result e }, σ⟩ ⇓ outcome
    (statements after sᵢ and result expression not executed)
```

**Execution order:** Statements execute sequentially in textual order. Control transfer outcomes bypass remaining statements.


### 6.2.4 Scope and Lifetime

**Block scope rule:**

A block `{ ... }` introduces a new lexical scope:

- Declarations within the block are visible from point of declaration to end of block
- Declarations are NOT visible outside the block
- Nested blocks inherit outer scope bindings
- Shadowing requires `shadow` keyword (Part III §3.2.3)

**RAII cleanup:**

At block exit (normal or via control transfer), all owned values declared in the block are dropped in reverse order of declaration (LIFO).

```
[Block-RAII-Cleanup]
block contains bindings x₁: own τ₁, ..., xₙ: own τₙ
block exits (normal or transfer)
------------------------------------------
drop xₙ, ..., drop x₁    (reverse declaration order)
```

CITE: Part III §3.7.1 — Scope Hierarchy (block scope); Part III §3.9.3 — RAII Cleanup.

### 6.2.5 Example

```cursive
let result: i32 = {
    let x = compute_base()
    result x * 2  // Explicit result required
}
```

---

**Definition 6.7 (Variable Declaration Statements):** Variable declaration statements introduce new bindings into the current scope using `let` or `var` keywords.

## 6.3 Variable Declaration Statements

### 6.3.1 Overview

Variable declarations (`let`, `var`) are statements that introduce bindings to values. The complete semantics of variable bindings (syntax, immutability, shadowing, definite assignment) are specified in Part III (Declarations and Scope). This section specifies only their behavior as statements in the execution model.

**Key properties:**

- All variable declarations are statements (not expressions)
- Initialization is mandatory at declaration
- Type annotation is optional (inferred if omitted)
- Bindings extend the execution environment

CITE: Part III §3.2 — Variable Declarations (AUTHORITATIVE for syntax and binding semantics).

### 6.3.2 Syntax

**Grammar reference:** `VarDeclStmt` is defined in Appendix A.4; this subsection inherits the binding semantics described in Part III §3.2.

**Semantics:**

- **`let`**: Immutable binding (cannot reassign)
- **`var`**: Mutable binding (may be reassigned)
- **`shadow`**: Explicit shadowing of outer binding (required if name exists in enclosing scope)

CITE: Part III §3.2.1 — Syntax and Semantics; Part III §3.2.3 — Explicit Shadowing.

### 6.3.3 Static Semantics

**Well-formedness:**

```
[WF-Let]
Γ ⊢ e : τ
x ∉ Γ    (or shadow keyword present with x ∈ outer scope)
------------------------------------
Γ ⊢ let x : τ = e : ()

[WF-Var]
Γ ⊢ e : τ
x ∉ Γ    (or shadow keyword present)
------------------------------------
Γ ⊢ var x : τ = e : ()
```

**Type inference:**

If type annotation is omitted, τ is synthesized from the initializer expression.

**Definite assignment integration:**

Variables declared with `var` (without immediate initialization in the same statement) MUST be initialized on all control-flow paths before use. The definite assignment algorithm is specified in Part III §3.2.7.

**Shadowing enforcement:**

Shadowing requires `shadow` keyword; violations produce E3D02 (implicit shadowing) or E3D07 (invalid shadow).

CITE: Part III §3.2.2 — Single-Binding Rule; Part III §3.2.3 — Explicit Shadowing; Part III §3.2.7 — Definite Assignment.

### 6.3.4 Dynamic Semantics

**Let binding execution:**

```
[Exec-Let]
⟨e, σ⟩ ⇓ ⟨v, σ'⟩
ℓ = fresh location
--------------------------------------
⟨let x = e, σ⟩ ⇓ Normal(σ'[ℓ ↦ v])
    with environment extended: env' = env, x ↦ ℓ
```

**Var binding execution:**

```
[Exec-Var]
⟨e, σ⟩ ⇓ ⟨v, σ'⟩
ℓ = fresh location
--------------------------------------
⟨var x = e, σ⟩ ⇓ Normal(σ'[ℓ ↦ v])
    with environment extended: env' = env, x ↦ ℓ
    and x marked as mutable
```

**Execution order:**

1. Evaluate initializer expression to value
2. Allocate location for variable
3. Store value at location
4. Extend environment with binding

**Shadowing execution:**

When `shadow` is present, the new binding masks the outer binding for the remainder of the inner scope. The outer binding becomes inaccessible until the inner scope ends.

---

**Definition 6.8 (Assignment Statements):** Assignment statements modify the value of an lvalue expression.

## 6.4 Assignment Statements

### 6.4.1 Syntax

The grammar productions `AssignStmt`, `LValue`, and `AssignOp` reside in Appendix A.4. Assignment operators encompass simple assignment plus ten compound variants for arithmetic, bitwise, and shift operations.

**Examples:**

```cursive
// Simple assignment
var x = 10
x = 20

// Compound assignment
var count = 5
count += 10  // count = 15
count *= 2   // count = 30

// Field assignment
var point = Point { x: 0.0, y: 0.0 }
point.x = 10.0

// Array element assignment
var arr = [1, 2, 3]
arr[0] = 99
```

### 6.4.2 Static Semantics

**Well-formedness:**

```
[WF-Assign-Simple]
Γ ⊢ lvalue : τ
lvalue is lvalue (not rvalue)
lvalue has mut or own permission
lvalue is mutable binding (var) or mutable place
Γ ⊢ expr : τ
------------------------------------
Γ ⊢ lvalue = expr : ()
```

**Lvalue requirement:**

The target MUST be an lvalue (memory location that can be assigned). Lvalue forms:

- Variable references (if `var` binding): `x`
- Field access: `expr.field`
- Tuple projection: `expr.N`
- Array/slice indexing: `arr[index]`
- Pointer dereference (mutable pointer): `*mut_ptr`

CITE: Part V §5.0.4 — Value Categories.

**Permission requirement:**

```
[Assign-Permission-Check]
lvalue has permission perm
perm ∉ {mut, own}
------------------------------------
ERROR E4003: Permission violation (assignment requires mut or own)
```

**Immutable binding check:**

```
[Assign-Immutable-Binding]
lvalue is let binding (not var)
------------------------------------
ERROR E3D10: Cannot assign to immutable binding
```

CITE: Part III §3.2.8 — Assignment to Immutable Binding.

**Type matching:**

The expression type MUST match the lvalue type (no implicit conversions except documented coercions):

```
[Assign-Type-Match]
Γ ⊢ lvalue : τ
Γ ⊢ expr : υ
υ ≠ τ and υ ⊈: τ
------------------------------------
ERROR E4001: Type mismatch in assignment
```

**Compound assignment:**

```
[WF-Assign-Compound]
Γ ⊢ lvalue ⊕= expr  ≡  Γ ⊢ lvalue = (lvalue ⊕ expr)
lvalue is lvalue, mutable
Γ ⊢ lvalue : τ
Γ ⊢ expr : τ
τ supports operator ⊕
------------------------------------
Γ ⊢ lvalue ⊕= expr : ()
```

Compound assignment `target ⊕= expr` is semantically equivalent to `target = target ⊕ expr` with single evaluation of target.

### 6.4.3 Dynamic Semantics

**Simple assignment:**

```
[Exec-Assign]
⟨expr, σ⟩ ⇓ ⟨v, σ'⟩              (evaluate RHS first)
lvalue ↦ ℓ in environment
------------------------------------
⟨lvalue = expr, σ⟩ ⇓ Normal(σ'[ℓ ↦ v])
```

**Evaluation order:**

1. Evaluate right-hand side expression to value
2. Resolve lvalue to memory location
3. Store value at location
4. Result is Normal outcome with updated store

**Compound assignment:**

```
[Exec-Assign-Compound]
lvalue ↦ ℓ in environment
⟨σ(ℓ), σ⟩ ⇓ ⟨v_old, σ⟩         (read current value)
⟨expr, σ⟩ ⇓ ⟨v_rhs, σ'⟩
v_result = v_old ⊕ v_rhs
------------------------------------
⟨lvalue ⊕= expr, σ⟩ ⇓ Normal(σ'[ℓ ↦ v_result])
```

**Target evaluated once:**

Compound assignment evaluates the lvalue location expression only once, even though it must be both read and written. This matters for side effects:

```cursive
var arr = [0, 1, 2]
var i = 0

procedure next_index(): usize {
    i += 1
    result i - 1
}

arr[next_index()] += 10  // next_index() called ONCE
// Equivalent to: let idx = next_index(); arr[idx] = arr[idx] + 10
```

### 6.4.4 Examples

**Valid assignments:**

```cursive
var x = 5
x = 10         // OK: var allows reassignment

var point = Point { x: 0.0, y: 0.0 }
point.x = 5.0  // OK: field of mutable variable

var numbers = [1, 2, 3]
numbers[0] = 99  // OK: element of mutable array
```

**Invalid assignments:**

```cursive
let x = 5
x = 10  // ERROR E3D10: Cannot assign to immutable binding 'x'

let point = Point { x: 0.0, y: 0.0 }
point.x = 5.0  // ERROR E3D10: Cannot assign through immutable binding

let imm_ref: Point = get_point()
imm_ref.x = 10.0  // ERROR E4003: Permission violation (imm reference)
```

---

**Definition 6.9 (Expression Statements):** Expression statements execute expressions solely for their side effects, discarding any produced value.

## 6.5 Expression Statements

### 6.5.1 Syntax

`ExprStmt` is defined in Appendix A.4; any expression may be evaluated for its side effects with the resulting value discarded.

**Examples:**

```cursive
// Function call for side effect
println("Hello, world!")

// Procedure call
buffer.clear()

// Compound expression
(x + y).validate()
```

### 6.5.2 Static Semantics

**Well-formedness:**

```
[WF-Expr-Stmt]
Γ ⊢ e : τ
------------------------------------
Γ ⊢ e : ()    (as statement)
```

**Type checking:** Delegates to Part V expression typing rules.

**Unused result warning (non-normative):**

```
[Warn-Unused-Result]
e used as statement
Γ ⊢ e : τ where τ ≠ ()
------------------------------------
SHOULD WARN: Unused expression result of type τ
```

Compilers SHOULD issue a warning when an expression with non-unit type is used as a statement, as the value is discarded. This helps catch errors like:

```cursive
let opt = Option::Some(42)
opt.unwrap()  // WARNING: Result of type i32 unused
```

Certain expressions are typically exempted from this warning:

- Assignments (already return ())
- Procedure calls that clearly perform side effects
- Expressions marked with `[[must_use]]` attribute produce errors instead of warnings

CITE: Part V — Expressions and Operators (expression typing).

### 6.5.3 Dynamic Semantics

**Execution:**

```
[Exec-Expr-Stmt]
⟨e, σ⟩ ⇓ ⟨v, σ'⟩              (evaluate expression)
------------------------------------
⟨e, σ⟩ ⇓ Normal(σ')           (discard value v)
```

Expression statements evaluate the expression and discard the resulting value, keeping only the side effects (state changes).

---

**Definition 6.10 (Labeled Statements):** Labeled statements attach identifiers to loops and blocks, enabling multi-level control transfer via labeled break and continue.

## 6.6 Labeled Statements and Typed Early Exit

### 6.6.1 Syntax

The productions `LabeledStmt`, `Label`, and `LabelRef` are defined in Appendix A.4. Labels MAY decorate:

- Loop expressions (`loop`, Part V §5.9)
- Block statements (unit or value-producing)

Labels MUST NOT be attached to:

- Variable declarations
- Assignment statements
- Expression statements (other than loops/blocks)
- Other labeled statements (no nested labels on same construct)

**Examples:**

```cursive
// Labeled loop
'outer: loop {
    'inner: loop {
        if condition {
            break 'outer  // Exit outer loop
        }
        if other {
            continue 'inner  // Next iteration of inner
        }
    }
}

// Labeled block for early exit
'validate: {
    if !check_header(data) {
        break 'validate Err("invalid header")
    }
    if !check_body(data) {
        break 'validate Err("invalid body")
    }
    result Ok(parse(data))
}
```

### 6.6.2 Static Semantics

**Label scope and uniqueness:**

Labels have function-wide scope and MUST be unique within a function. Scope and uniqueness rules are specified in Part III §3.7.1.

```
[Label-Scope]
label 'ℓ declared in function f
------------------------------------
'ℓ visible anywhere in f (function scope)

[Label-Unique]
labels 'ℓ₁ and 'ℓ₂ declared in function f
ℓ₁ = ℓ₂
------------------------------------
ERROR E6001: Duplicate label 'ℓ₁ in function
```

CITE: Part III §3.7.1 — Scope Hierarchy (AUTHORITATIVE for label scope).

**Typed break with value:**

When a label targets a block (not a loop), `break 'label value` exits the block with the value. The block's type becomes the type of all break values:

```
[Typed-Block-Exit]
'label: block contains break 'label v₁ : τ₁, ..., break 'label vₙ : τₙ
all τᵢ unify to τ
----------------------------------------
block type is τ (result of break values)
```

This enables typed early-exit without goto:

```cursive
let result: Result<Data, Error> = 'validate: {
    if bad_header {
        break 'validate Err("header")  // Type: Result<Data, Error>
    }
    if bad_body {
        break 'validate Err("body")
    }
    result Ok(parsed_data)  // Type: Result<Data, Error>
}
// All branches unify to Result<Data, Error>
```

**Label resolution:**

```
[Label-Resolve]
break 'ℓ or continue 'ℓ
'ℓ declared in enclosing function
'ℓ targets a construct enclosing the break/continue
------------------------------------
label resolves to target construct

[Label-Undefined]
break 'ℓ or continue 'ℓ
'ℓ not declared in function
------------------------------------
ERROR E6002: Undefined label 'ℓ
```

### 6.6.3 Dynamic Semantics

**Label attachment:**

Labels do not change execution behavior; they serve as targets for control transfer.

```
[Exec-Labeled]
⟨s, σ⟩ ⇓ outcome
------------------------------------
⟨'label: s, σ⟩ ⇓ outcome'
    where outcome' = outcome with label context
```

**Label resolution at runtime:**

When `break 'label` or `continue 'label` executes, control transfers to the labeled construct. The label is resolved statically (at compile time), so no runtime lookup is needed.

### 6.6.4 No Goto

**Normative prohibition:**

Cursive does NOT include a `goto` statement. All control transfer is structured through:

- `return` (exit function)
- `break` (exit loop or labeled block, optionally with value)
- `continue` (next loop iteration)
- `if`/`match` (conditional branching)
- `loop` (iteration)

**Rationale:**

- **Local reasoning**: Unstructured jumps complicate definite assignment, region escape analysis, and permission tracking
- **LLM-friendly**: Structured patterns are more predictable and easier to generate/verify
- **Verifiable**: Structured control enables static analysis without flow-sensitive complexity
- **Sufficient**: Labeled break/continue + return + structured expressions cover all practical use cases

**If goto-like behavior is needed:**

Use labeled blocks with typed early-exit:

```cursive
// Instead of goto:
'cleanup: {
    let resource = acquire()
    defer { release(resource) }

    if !setup(resource) {
        break 'cleanup Err("setup failed")
    }
    if !process(resource) {
        break 'cleanup Err("process failed")
    }
    result Ok(finalize(resource))
}
```

---

**Definition 6.11 (Return Statements):** Return statements exit the enclosing function immediately, optionally with a return value.

## 6.7 Return Statements

### 6.7.1 Syntax

The production `ReturnStmt` is defined in Appendix A.4. Two forms exist:

1. `return` — Return unit `()` from function
2. `return expr` — Return value from function

**Examples:**

```cursive
// Return unit
procedure print_message(msg: string)
    uses io.write
{
    println(msg)
    return  // Explicit return (optional at end)
}

// Return value
function compute(): i32 {
    let result = calculate()
    if result > 100 {
        return result  // Early return
    }
    result result * 2
}
```

### 6.7.2 Static Semantics

**Well-formedness:**

```
[WF-Return-Value]
within function with return type τ
Γ ⊢ e : τ
------------------------------------
Γ ⊢ return e : !

[WF-Return-Unit]
within function with return type ()
------------------------------------
Γ ⊢ return : !

[WF-Return-Type-Mismatch]
within function with return type τ
Γ ⊢ e : σ where υ ≠ τ and υ ⊈: τ
------------------------------------
ERROR E4001: Type mismatch (expected τ, found υ)
```

**Context requirement:**

```
[Return-Context-Required]
return statement outside function body
------------------------------------
ERROR E4013: Return statement outside function
```

**Divergence typing:**

Return statements have type `!` (never type) because they never complete normally. Control does not continue after a return.

```
[T-Return-Divergence]
Γ ⊢ return e : !
------------------------------------
any code after return is unreachable
```

**Diagnostic E4013:**

- **Trigger:** `return` appears outside function body (e.g., at module scope, in comptime block)
- **Message:** "Return statement outside function"
- **Note:** "Return can only appear inside function or procedure bodies"
- **Fix:** "Remove return statement or place inside a function"

### 6.7.3 Dynamic Semantics

**Return execution:**

```
[Exec-Return-Value]
⟨e, σ⟩ ⇓ ⟨v, σ'⟩
------------------------------------
⟨return e, σ⟩ ⇓ Return(v, σ')

[Exec-Return-Unit]
------------------------------------
⟨return, σ⟩ ⇓ Return((), σ)
```

**Function exit with defers:**

When a function returns (normally or via `return` statement), deferred blocks execute in LIFO order before the function exits:

```
[Function-Exit-With-Defers]
function body execution produces Return(v, σ')
deferred blocks in function: [d₁, ..., dₙ]
⟨dₙ, σ'⟩ ⇓ Normal(σₙ)    ...    ⟨d₁, σₙ₋₁⟩ ⇓ Normal(σ₁)
--------------------------------------------------------
function returns v with final store σ₁
```

**RAII cleanup:**

After defer execution, all owned values in function scope are dropped in reverse declaration order.

CITE: §6.9 — Defer Statements; Part III §3.9.3 — RAII Cleanup.

### 6.7.4 Examples

**Early return:**

```cursive
function validate(data: Data): Result<(), Error> {
    if !check_format(data) {
        return Err("invalid format")  // Early return
    }
    if !check_content(data) {
        return Err("invalid content")
    }
    result Ok(())  // Normal path
}
```

**Return with defer:**

```cursive
function process_file(path: string): Result<Data, Error>
    uses fs.read, alloc.heap
{
    let file = open(path)
    defer { file.close() }  // Executes on ALL return paths

    if !file.is_valid() {
        return Err("invalid file")  // defer executes here
    }

    result Ok(file::read())  // defer executes here too
}
```

---

**Definition 6.12 (Break and Continue):** Break and continue statements provide structured control transfer within loops and (for break) labeled blocks.

## 6.8 Break and Continue Statements

### 6.8.1 Syntax

The productions `BreakStmt`, `ContinueStmt`, and `LabelRef` appear in Appendix A.4. For clarity, break supports four syntactic forms:

1. `break` — Exit innermost loop with `()`
2. `break value` — Exit innermost loop with value
3. `break 'label` — Exit labeled construct with `()`
4. `break 'label value` — Exit labeled construct with value

**Two continue forms:**

1. `continue` — Continue innermost loop
2. `continue 'label` — Continue labeled loop

**Examples:**

```cursive
// Unlabeled break
loop {
    if done {
        break  // Exit loop
    }
}

// Break with value
let result: i32 = loop {
    let x = compute()
    if x > 100 {
        break x  // Exit loop, return x
    }
}

// Labeled break
'outer: loop {
    'inner: loop {
        if condition {
            break 'outer  // Exit outer loop
        }
    }
}

// Labeled block early-exit with value
let output: Result<Data, Error> = 'process: {
    if invalid_input {
        break 'process Err("bad input")
    }
    result Ok(transform(input))
}
```

### 6.8.2 Static Semantics

**Context requirement (break):**

```
[Break-Context-Required]
break statement
not within loop or labeled block
------------------------------------
ERROR E4011: Break statement outside loop or labeled block
```

**Context requirement (continue):**

```
[Continue-Context-Required]
continue statement
not within loop
------------------------------------
ERROR E4012: Continue statement outside loop
```

**Continue on non-loop label:**

```
[Continue-Non-Loop]
continue 'label
'label targets a block (not a loop)
------------------------------------
ERROR E6003: Continue cannot target non-loop label
```

**Type unification for break values:**

```
[Break-Value-Unify]
loop or labeled block contains break v₁, ..., break vₙ
Γ ⊢ v₁ : τ    ...    Γ ⊢ vₙ : τ
------------------------------------
loop/block result type is τ

[Break-Value-Mismatch]
loop or labeled block contains break v₁ : τ₁, break v₂ : τ₂
τ₁ ≠ τ₂ and neither is subtype of other
------------------------------------
ERROR E4001: Type mismatch in break values
```

All break values within a construct MUST have the same type.

**Divergence typing:**

```
[T-Break-Divergence]
Γ ⊢ break : !

[T-Continue-Divergence]
Γ ⊢ continue : !
```

Break and continue are divergent control transfers; code immediately after them is unreachable.

**Diagnostic E4011:**

- **Trigger:** `break` outside loop or labeled block
- **Message:** "Break statement outside loop or labeled block"
- **Note:** "Break can only appear inside loop expressions or labeled blocks"
- **Fix:** "Remove break or place inside a loop/labeled block"

**Diagnostic E4012:**

- **Trigger:** `continue` outside loop
- **Message:** "Continue statement outside loop"
- **Note:** "Continue can only appear inside loop expressions"
- **Fix:** "Remove continue or place inside a loop"

**Diagnostic E6003:**

- **Trigger:** `continue 'label` where label targets a block (not loop)
- **Message:** "Continue cannot target non-loop label '{label}'"
- **Note:** "Continue requires a loop target; use break for blocks"
- **Fix:** "Change to 'break {label}' or target a loop label"

CITE: Part V §5.9.5-6 — Break and Continue Expressions.

### 6.8.3 Dynamic Semantics

**Break execution:**

```
[Exec-Break-Unlabeled]
within loop or labeled block
------------------------------------
⟨break, σ⟩ ⇓ Break(none, none, σ)

[Exec-Break-Value]
⟨e, σ⟩ ⇓ ⟨v, σ'⟩
within loop or labeled block
------------------------------------
⟨break e, σ⟩ ⇓ Break(none, some(v), σ')

[Exec-Break-Labeled]
within scope of 'label
------------------------------------
⟨break 'label, σ⟩ ⇓ Break(some(label), none, σ)

[Exec-Break-Labeled-Value]
⟨e, σ⟩ ⇓ ⟨v, σ'⟩
within scope of 'label
------------------------------------
⟨break 'label e, σ⟩ ⇓ Break(some(label), some(v), σ')
```

**Continue execution:**

```
[Exec-Continue-Unlabeled]
within loop
------------------------------------
⟨continue, σ⟩ ⇓ Continue(none, σ)

[Exec-Continue-Labeled]
within scope of 'label (loop)
------------------------------------
⟨continue 'label, σ⟩ ⇓ Continue(some(label), σ)
```

**Outcome propagation:**

Break and continue outcomes propagate outward through enclosing statements until reaching their target construct:

```
[Break-Propagate]
⟨s, σ⟩ ⇓ Break(some(label), v?, σ')
current construct has label ≠ target label
--------------------------------------------
propagate Break(some(label), v?, σ') outward

[Break-Capture]
⟨s, σ⟩ ⇓ Break(some(label), v?, σ')
current construct has matching label
--------------------------------------------
exit this construct with value v? and store σ'
```

### 6.8.4 Examples

**Break with unified types:**

```cursive
let value: i32 = loop {
    let x = compute()
    if x < 0 {
        break 0  // Type: i32
    }
    if x > 100 {
        break 100  // Type: i32
    }
    break x  // Type: i32
}  // All breaks have type i32
```

**Type mismatch error:**

```cursive
let invalid = loop {
    if cond1 {
        break 42      // Type: i32
    }
    if cond2 {
        break "text"  // Type: string
    }
}  // ERROR E4001: break values have incompatible types
```

**Multi-level break:**

```cursive
'outer: loop {
    'inner: loop {
        if should_exit_all {
            break 'outer  // Exit both loops
        }
        if should_exit_inner {
            break  // Exit inner loop only
        }
    }
}
```

---

**Definition 6.13 (Defer Statements):** Defer statements schedule blocks to execute at scope exit, ensuring cleanup occurs on all control-flow paths.

## 6.9 Defer Statements

### 6.9.1 Syntax

`DeferStmt` is defined in Appendix A.4. Examples:

```cursive
{
    let file = open("data.txt")
    defer { file.close() }  // Executes on scope exit

    process(file)
}  // file.close() executes here

function transaction() {
    begin_transaction()
    defer { rollback() }  // Executes on ANY exit

    if error {
        return  // rollback() executes before return
    }

    commit()
    defer { log("committed") }  // Second defer
}  // Defers execute in LIFO: log, then rollback
```

### 6.9.2 Static Semantics

**Well-formedness:**

```
[WF-Defer]
Γ ⊢ block : ()
------------------------------------
Γ ⊢ defer { block } : ()
```

The deferred block MUST have unit type (cannot produce a value).

**Multiple defers:**

Multiple defer statements may appear in the same scope. They execute in LIFO (last-in, first-out) order.

### 6.9.3 Dynamic Semantics

**Defer registration:**

```
[Exec-Defer-Register]
------------------------------------
⟨defer { block }, σ⟩ ⇓ Normal(σ)
    with defer queue: append block to current scope's defer list
```

Executing a defer statement does NOT execute the block immediately; it registers the block for later execution.

**Defer execution on scope exit:**

```
[Exec-Scope-Exit-Defers]
scope exits with outcome
deferred blocks in scope: [d₁, ..., dₙ]    (in registration order)
⟨dₙ, σ⟩ ⇓ Normal(σₙ)    ...    ⟨d₁, σₙ₋₁⟩ ⇓ Normal(σ₁)
--------------------------------------------------------
defers execute in LIFO order
final store is σ₁
```

**Execution on ALL control paths:**

Defers execute regardless of how the scope exits:

- Normal completion: Defers execute before scope ends
- `return`: Defers execute before function returns
- `break`: Defers execute before exiting loop/block
- `panic`: Defers execute during stack unwind (if unwind enabled)

**Defer execution outcome:**

If a deferred block produces a non-Normal outcome (e.g., panic), that outcome takes precedence:

```
[Defer-Panic]
executing deferred blocks
⟨dᵢ, σ⟩ ⇓ Panic
------------------------------------
overall outcome is Panic (remaining defers may or may not execute)
```

### 6.9.4 Examples

**LIFO execution order:**

```cursive
{
    defer { println("First defer") }
    defer { println("Second defer") }
    defer { println("Third defer") }
    println("Body")
}
// Output:
// Body
// Third defer
// Second defer
// First defer
```

**Defer with early return:**

```cursive
function process(): Result<(), Error> {
    let lock = acquire_lock()
    defer { release_lock(lock) }  // Executes on both paths

    if error_condition {
        return Err("error")  // defer executes before return
    }

    result Ok(())  // defer executes before normal return
}
```

**Resource cleanup pattern:**

```cursive
{
    let file = open("data.txt")
    defer { file.close() }

    let connection = connect("server")
    defer { connection.disconnect() }

    // Use resources
    transfer(file, connection)
}  // Cleanup order: connection.disconnect(), then file.close()
```

---

**Definition 6.14 (Modal-Aware Branching):** Modal-aware branching refines modal state types through pattern matching and enforces exhaustiveness over modal state spaces.

## 6.10 Modal-Aware Branching and State Refinement

### 6.10.1 Overview

Modal-aware control flow integrates Cursive's modal type system (Part II §2.3.3) with branching constructs to provide compile-time verification of state machine protocols. This section specifies how match expressions and conditional branches refine modal states.

**Key innovations:**

- **State refinement**: Match arms automatically refine the modal type to the matched state
- **Exhaustiveness over states**: Pattern exhaustiveness extends to modal state coverage
- **Zero-cost**: All verification occurs at compile time; runtime behavior is standard branching

CITE: Part II §2.3.3 — Modal Types; Part V §5.8 — Match Expressions.

### 6.10.2 State Refinement in Match Arms

**Type refinement rule:** (Union syntax `∨` defined in Part II §2.3.4.)

When matching on a modal type value, each arm refines the matched value's type to the specific state:

```
[Match-Modal-Refine]
Γ ⊢ scrutinee : Modal@S₁ ∨ Modal@S₂ ∨ ... ∨ Modal@Sₙ
arm matches Modal@Sᵢ
--------------------------------------------------------
within arm body: Γ ⊢ scrutinee : Modal@Sᵢ    (refined type)
```

**Syntax (using standard match from Part V):**

```cursive
let conn: Connection@Disconnected ∨ Connection@Connected = get_connection()

let status: string = match conn {
    Connection@Disconnected => {
        // Here: conn : Connection@Disconnected (refined)
        result "not connected"
    },
    Connection@Connected => {
        // Here: conn : Connection@Connected (refined)
        result conn.get_status()  // Can call @Connected methods
    },
}
```

**Refinement propagation:**

The refined type is available for procedure dispatch, field access, and further matching within the arm:

```
[Modal-Procedure-After-Refine]
match value : Modal@S₁ ∨ Modal@S₂
arm matches Modal@S₁
within arm: value::procedure()
procedure signature: procedure proc(self: Modal@S₁) ...
--------------------------------------------------------
call is well-typed (refined type provides required state)
```

**Example with procedure calls:**

```cursive
modal File {
    @Closed { path: string }
    @Open { path: string, handle: FileHandle }

    @Closed -> open() -> @Open
    @Open -> read() -> @Open
    @Open -> close() -> @Closed
}

procedure process(file: File@Closed ∨ File@Open) {
    match file {
        File@Closed => {
            // file : File@Closed here
            let opened = file.open()  // OK: open() requires @Closed
            result opened
        },
        File@Open => {
            // file : File@Open here
            file::read()  // OK: read() requires @Open
            result file
        },
    }
}
```

CITE: Part II §2.3.3 — Modal Types and State Transitions.

### 6.10.3 Exhaustiveness Over Modal States

**Exhaustiveness requirement:**

When matching on a modal type, patterns MUST cover every declared state (or include a wildcard). Non-exhaustive matches are rejected:

```
[Modal-Exhaustiveness]
Γ ⊢ scrutinee : Modal    (states S = {@S₁, ..., @Sₙ})
match arms cover states S' ⊆ S
S' ≠ S
--------------------------------------------------------
ERROR E6005: Non-exhaustive match over modal states
    Missing: S \ S'
```

**Canonical algorithm reference:**

The exhaustiveness checking algorithm is defined once in Part V §5.8.5. Modal matches reuse that algorithm by providing the modal state set in lieu of enum variants—implementations MUST dispatch to the canonical routine instead of maintaining a duplicate copy here. Consequently the complexity remains O(|S| × |P|), where |S| is the number of modal states and |P| the number of patterns.

**Diagnostic E6005:**

- **Trigger:** Match on modal type does not cover all states
- **Message:** "Non-exhaustive patterns over modal type {Modal}"
- **Note:** "Missing coverage for state(s): {list}"
- **Fix:** "Add patterns for missing states or use wildcard '\_'"

**Example:**

```cursive
modal Status {
    @Idle
    @Running { progress: f64 }
    @Completed { result: Data }
    @Failed { error: Error }
}

let message: string = match status {
    Status@Idle => "waiting",
    Status@Running => "in progress",
    Status@Completed => "done",
    Status@Failed => "error",
}  // Exhaustive: all 4 states covered
```

If any state (for example `Status@Completed` or `Status@Failed`) is omitted the canonical checker reports the missing states and emits ERROR E6005.

CITE: Part V §5.8.5 — Exhaustiveness Checking; Part II §2.3.3 — Modal Types.

---

**Definition 6.16 (Loop Verification and Postconditions):** Loop verification provides formal contracts on loop behavior through invariants, termination metrics, and postconditions on loop results.

## 6.11 Loop Verification and Postconditions

### 6.11.1 Overview

Loop verification integrates Cursive's contract system with loop constructs to enable formal reasoning about loop correctness and termination. This section specifies loop invariants, termination metrics (variants), and postconditions on loop results.

**Verification components:**

- **Invariants** (`with { ... }`): Conditions that hold at the start of each iteration
- **Variants** (`by expr`): Metrics that decrease each iteration to prove termination
- **Postconditions** (`will { ... }`): Guarantees on the loop's result value

**Verification modes:**

Controlled by `[[verify(mode)]]` attribute on the loop or enclosing function:

- `[[verify(static)]]`: Invariants and variants checked via SMT solver at compile time
- `[[verify(runtime)]]`: Invariants checked dynamically (assertions injected)
- `[[verify(none)]]`: Verification clauses are documentation only (no checking)

CITE: Foundations §4.3 — Attributes; Part VII — Contracts and Effects.

### 6.11.2 Loop Invariants and Variants

**Syntax:** Verification clauses (`LoopVerif` and `AssertionList`) are defined in Appendix A.4; the forms summarised below explain their semantics.

**Grammar integration with loops:**

From Part V §5.9, loops may include verification clauses:

```cursive
loop iterator_binding in collection by metric
    with { invariants }
{
    body
}
```

**Static semantics (invariants):**

```
[Loop-Invariant-WF]
loop with { I₁, ..., Iₙ }
Γ ⊢ I₁ : bool    ...    Γ ⊢ Iₙ : bool
--------------------------------------------------------
invariants well-formed
```

Each invariant MUST be a boolean expression.

**Invariant verification:**

```
[Loop-Invariant-Entry]
loop with invariants {I₁, ..., Iₙ}
before first iteration
--------------------------------------------------------
verify: I₁ ∧ ... ∧ Iₙ    (establishment)

[Loop-Invariant-Preservation]
loop with invariants {I₁, ..., Iₙ}
assume: I₁ ∧ ... ∧ Iₙ at iteration start
execute: loop body
--------------------------------------------------------
verify: I₁ ∧ ... ∧ Iₙ at iteration end    (preservation)
```

**Static semantics (variants):**

```
[Loop-Variant-WF]
loop by expr
Γ ⊢ expr : τ where τ is well-ordered (integer type)
--------------------------------------------------------
variant well-formed
```

**Variant verification (termination proof):**

```
[Loop-Variant-Decreases]
loop by metric
vᵢ = value of metric at iteration i
--------------------------------------------------------
verify: vᵢ₊₁ < vᵢ for all iterations i
verify: metric ≥ 0 (non-negative)
```

The metric MUST decrease each iteration and remain non-negative, proving termination.

**Verification strategy:**

- **Static (`[[verify(static)]]`)**: Use SMT solver to prove invariant preservation and variant decrease
- **Runtime (`[[verify(runtime)]]`)**: Inject assertions at loop entry and after body
- **None (`[[verify(none)]]`)**: Clauses are documentation; no checks emitted

**Example (static verification):**

```cursive
[[verify(static)]]
function sum_array(arr: [i32], n: usize): i32
    must n <= arr.len()
{
    var sum = 0
    loop i: usize in 0..n by n - i
        with {
            0 <= i,
            i <= n,
            sum == sum_of(arr[0..i])
        }
    {
        sum += arr[i]
    }
    result sum
}
// Compiler proves: invariants hold, metric decreases, loop terminates
```

**Example (runtime verification):**

```cursive
[[verify(runtime)]]
procedure process_items(items: [Item])
    uses io.write
{
    var count = 0
    loop item: Item in items
        with { count >= 0 }
    {
        println(item)
        count += 1
        // Runtime check: count >= 0 after each iteration
    }
}
```

CITE: Part V §5.9.7 — Loop Contracts; Part VII — Contracts (verification modes).

### 6.11.3 Loop Postconditions on Results

**Definition 6.17 (Loop Postconditions):** A loop postcondition specifies guarantees on the value produced by `break value` statements within the loop.

**Syntax:** `LoopPostcondition` and `AssertionList` are defined in Appendix A.4; only their semantics are elaborated here.

**Grammar integration:**

```cursive
loop condition by metric
    with { invariants }
    will { postconditions }
{
    body
}
```

**Static semantics:**

```
[Loop-Postcondition-WF]
loop will { Q₁, ..., Qₙ }
loop result type is τ (from break values)
Γ, result: τ ⊢ Q₁ : bool    ...    Γ, result: τ ⊢ Qₙ : bool
--------------------------------------------------------
postconditions well-formed
```

Postconditions may reference `result` (the value from `break value`) and loop-scope variables.

**Verification:**

```
[Loop-Postcondition-Check]
loop will Q
loop exits via break value with value v
--------------------------------------------------------
verify: Q[result ↦ v]
```

At each `break value` site, the postcondition is verified (statically or at runtime per verification mode).

**Example:**

```cursive
[[verify(runtime)]]
function find_positive(arr: [i32]): Option<i32>
    will {
        match result {
            Option::Some(x) => x > 0,
            Option::None => true
        }
    }
{
    let found: Option<i32> = loop i: usize in 0..arr.len()
        will {
            match result {
                Option::Some(val) => val > 0,  // Guarantee: returned value is positive
                Option::None => true
            }
        }
    {
        if arr[i] > 0 {
            break Option::Some(arr[i])  // Runtime check: arr[i] > 0
        }
    }

    result found
}
```

**Diagnostic E6006:**

- **Trigger:** Break value does not satisfy loop postcondition
- **Message:** "Loop postcondition violated at break"
- **Note:** "Break value {value} does not satisfy declared postcondition"
- **Fix:** "Ensure break value satisfies 'will' clause or adjust postcondition"

---

**Definition 6.18 (Region-Scoped Iteration):** Region-scoped iteration establishes a fresh region for each loop iteration, providing O(1) reclamation and preventing inter-iteration memory escapes.

## 6.12 Region-Scoped Iteration

### 6.12.1 Overview

Region-scoped iteration applies Cursive's region-based memory model to loops, enabling per-iteration allocation with guaranteed cleanup and escape prevention. This pattern is particularly useful for batch processing where each iteration allocates temporary data.

**Key properties:**

- Fresh region per iteration
- O(1) deallocation at iteration end
- Static escape prevention (iteration-local values cannot leak)
- Zero overhead beyond normal region mechanics

CITE: Part II §2.0.8.4 — Regions; Part V §5.11 — Region Expressions.

### 6.12.2 Syntax

**Explicit syntax (desugared):**

```cursive
loop item: Type in collection {
    region iter {
        let temp = alloc_in<iter>(process(item))
        use_temp(temp)
    }  // temp freed here
}
```

**Syntactic sugar:** Appendix A.4 (`LoopWithRegion`) defines the optional `region iter` clause; the previous example illustrates the desugared form.

### 6.12.3 Static Semantics

**Non-escape verification:**

Values allocated in the iteration region MUST NOT escape beyond the iteration:

```
[Region-Iter-Escape]
loop with per-iteration region r
body allocates value v in region r
v escapes iteration (assigned to outer variable, returned, etc.)
--------------------------------------------------------
ERROR E3D09: Region value escapes iteration
```

**Region stack management:**

```
[Region-Iter-Stack]
loop with per-iteration region r
region context at loop entry: Δ
--------------------------------------------------------
region context per iteration: Δ · r
region context after iteration: Δ    (r popped)
```

Each iteration pushes the region, executes the body, then pops the region.

CITE: Part III §3.9.2 — Region Escape Prevention.

### 6.12.4 Dynamic Semantics

**Per-iteration region lifecycle:**

```
[Exec-Loop-Region]
loop item in collection region r
for each iteration with value vᵢ:
    push region r onto region stack
    ⟨body[item ↦ vᵢ], σ, Δ·r⟩ ⇓ outcome
    deallocate all allocations in r    (O(1))
    pop region r
--------------------------------------------------------
continue to next iteration or exit based on outcome
```

**Deallocation guarantee:**

At the end of each iteration (normal completion or `break`/`continue`), all memory allocated in the iteration region is freed in O(1) time.

**Example:**

```cursive
procedure process_batch(items: [Item])
    uses alloc.region, io.write
{
    loop item: Item in items region iter {
        // Fresh region per iteration
        let buffer = alloc_in<iter>(Buffer.new(1024))
        let processed = alloc_in<iter>(transform(item))

        buffer::write(processed)
        output(buffer)

        // buffer and processed freed here (O(1))
        // No memory accumulation across iterations
    }
}
```

**Use case:**

Prevents memory accumulation in long-running loops by ensuring iteration-local allocations are reclaimed immediately rather than held until loop completion.

---

**Definition 6.19 (Block-Scoped Contracts and Effect Narrowing):** Block-scoped contracts and effect narrowing enable local specification and capability restriction within functions.

## 6.13 Block-Scoped Contracts and Effect Narrowing

### 6.13.1 Overview

Block-scoped contracts allow local preconditions and postconditions within function bodies. Effect narrowing restricts the effect budget within a block to a subset of the enclosing function's effects. These features support fine-grained reasoning and capability discipline.

**Key properties:**

- Contracts localized to block scope
- Effect subsets statically enforced
- Integrates with function-level contracts
- Zero runtime overhead for static verification

CITE: Part VII — Contracts and Effects.

### 6.13.2 Block Contracts

**Syntax:** Appendix A.4 (`BlockContract`) defines shorthand for block-level contracts; the following example illustrates usage.

```cursive
procedure safe_divide(a: i32, b: i32): Result<i32, string> {
    if b == 0 {
        result Err("division by zero")
    }

    // Block with precondition
    let quotient = contract(b != 0) {
        a / b  // Safe: b ≠ 0 guaranteed
    } will result == a / b

    result Ok(quotient)
}
```

**Static semantics:**

```
[Block-Contract-Check]
contract(P) { body } will Q
Γ ⊢ P : bool    (precondition)
Γ ⊢ body : τ
Γ, result: τ ⊢ Q : bool    (postcondition)
--------------------------------------------------------
block well-formed with type τ
```

**Verification:**

- **Static**: Prove P before block, prove Q after block via SMT
- **Runtime**: Check P before entry (panic if false), check Q before exit

**Diagnostic E6007:**

- **Trigger:** Block precondition violation (runtime)
- **Message:** "Block precondition violated"
- **Note:** "Condition {condition} does not hold at block entry"
- **Fix:** "Ensure precondition is satisfied before block"

### 6.13.3 Effect Narrowing

**Syntax:** Effect narrowing uses the `EffectNarrowBlock` production in Appendix A.4.

```cursive
procedure mixed_operations(data: Data)
    uses io.read, io.write, alloc.heap, net.write
{
    // Narrow to only I/O effects
    uses io.read, io.write {
        let input = read_input()
        write_output(process(input))
        // let buf = Vec.new()  // ERROR E4004: alloc.heap not available
    }

    // Full effects available here
    let buffer = Vec.new()  // OK: alloc.heap available
    send_network(buffer)    // OK: net.write available
}
```

**Static semantics:**

```
[Effect-Narrow-Subset]
enclosing function uses ε_function
block uses ε_block { ... }
ε_block ⊈ ε_function
--------------------------------------------------------
ERROR E6008: Block effect set not subset of function effects
    ε_block \ ε_function = {excess effects}
```

The block's effect set MUST be a subset of the enclosing function's declared effects.

**Call checking within narrow block:**

```
[Effect-Narrow-Call-Check]
within uses ε_block { ... }
call to procedure with effects ε_callee
ε_callee ⊈ ε_block
--------------------------------------------------------
ERROR E4004: Effect(s) unavailable in narrowed block
    Required: ε_callee \ ε_block
```

Calls within the narrowed block MUST respect the narrowed effect budget.

**Rationale:**

- Documents which effects are actually used in each section
- Enables local reasoning about side effects
- Supports verification of capability discipline
- Helps prevent accidental use of dangerous effects

**Diagnostic E6008:**

- **Trigger:** Block declares effects not in enclosing function
- **Message:** "Block effect narrowing exceeds function effects"
- **Note:** "Block declares effects {list} not present in function 'uses' clause"
- **Fix:** "Remove excess effects from block or add to function 'uses' clause"

---

**Definition 6.20 (No-Panic Sections):** No-panic sections forbid the `panic` effect within a scope, enabling safety-critical and real-time code with guaranteed non-divergence.

## 6.14 No-Panic Sections

### 6.14.1 Overview

The `[[no_panic]]` attribute on functions or blocks statically prohibits any operation that may panic, including:

- Explicit `panic()` calls
- Operations that may panic (division by zero, array bounds violations, integer overflow in debug mode)
- Calls to functions that may panic

This enables safety-critical sections with formal no-divergence guarantees.

CITE: Foundations §4.3 — Attributes.

### 6.14.2 Attribute Definition

**Attribute signature:**

```
[[no_panic]]
```

**Applies to:**

- Function declarations
- Procedure declarations
- Block statements

**Attribute summary:**

| Attribute | Applies To | Effect Budget Impact | Notes |
|-----------|------------|----------------------|-------|
| `[[no_panic]]` | Functions, procedures, block statements | Implicitly removes `panic` from the allowable effect set inside the annotated scope | Any operation that would require the `panic` effect (including implicit panics such as division by zero or bounds violations) becomes a compile-time error unless statically proven unreachable. The attribute composes with `[[overflow_checks(false)]]` to permit wrapping arithmetic without panics. |

**Example:**

```cursive
[[no_panic]]
function safe_divide(a: i32, b: i32): Option<i32> {
    if b == 0 {
        return Option::None  // Must check explicitly; cannot panic
    }
    // let x = a / b  // ERROR E6009: division may panic
    result Option::Some(a / b)  // OK if proven b ≠ 0
}
```

CITE: Foundations §4.3 — Core Attributes (attribute definition).

### 6.14.3 Static Semantics

**Panic effect prohibition:**

```
[No-Panic-Effect-Check]
within [[no_panic]] function or block
operation or call has effect ε
panic ∈ ε
--------------------------------------------------------
ERROR E6009: Panic effect forbidden in no-panic section
```

**Operations that may panic:**

- Explicit `panic(msg)` calls
- Division by potentially-zero divisor
- Array indexing without bounds proof
- Integer arithmetic without overflow checks disabled
- Calls to functions with `panic` in their effect set

**Proven-safe exception:**

Operations may be allowed if statically proven safe:

```
[No-Panic-Proven-Safe]
within [[no_panic]] section
operation may panic under effect ε
static analysis proves operation cannot panic
--------------------------------------------------------
operation allowed (panic effect requirement waived)
```

**Example:**

```cursive
[[no_panic]]
function safe_access(arr: [i32; 10], i: usize): Option<i32> {
    if i < 10 {
        result Option::Some(arr[i])  // OK: i < 10 proven, no panic
    } else {
        result Option::None
    }
    // let x = arr[i]  // ERROR E6009: i may be >= 10, would panic
}
```

**Diagnostic E6009:**

- **Trigger:** Operation with `panic` effect in `[[no_panic]]` section
- **Message:** "Operation may panic in no-panic section"
- **Note:** "{operation} requires 'panic' effect which is forbidden by [[no_panic]]"
- **Fix:** "Remove [[no_panic]] attribute, avoid panicking operation, or prove operation cannot panic"

**Interaction with [[overflow_checks]]:**

```cursive
[[no_panic]]
[[overflow_checks(false)]]
function wrapping_add(a: i32, b: i32): i32 {
    result a + b  // OK: overflow checks disabled, no panic
}
```

CITE: Foundations §4.3.3 — Overflow Checks Attribute.

---

**Definition 6.21 (Effect-Gated Compile-Time Branching):** Effect-gated branching selects code paths at compile time based on available effects, enabling effect-polymorphic implementations.

## 6.15 Effect-Gated Compile-Time Branching

### 6.15.1 Overview

Effect-gated branching allows compile-time selection of implementation strategies based on the effect set available in the calling context. This supports writing effect-polymorphic code that adapts to different capability environments.

**Key properties:**

- Branch selection occurs at compile time only
- Both branches MUST type-check independently
- No runtime overhead (selected branch is the only code generated)
- Integrates with effect polymorphism (Part II §2.0.8.2)

### 6.15.2 Syntax

The productions `EffectGatedBranch` and `EffectPredicate` are defined in Appendix A.4; the example below demonstrates the `comptime if` form.

```cursive
procedure log<ε>(message: string)
    uses ε
    where ε ⊆ {io.write, alloc.heap}
{
    comptime if effects_include(io.write) {
        // This branch selected if io.write ∈ ε
        println(message)
    } else {
        // This branch selected if io.write ∉ ε
        // Silent: no logging capability
    }
}

// Usage:
log<io.write>("Event occurred")     // First branch
log<∅>("Event occurred")             // Second branch (silent)
```

### 6.15.3 Static Semantics

**Both branches must type-check:**

```
[Comptime-Effect-Branch-WF]
comptime if effects_include(ε_test) { b₁ } else { b₂ }
Γ, assume(ε_test ⊆ ε_available) ⊢ b₁ : τ
Γ, assume(ε_test ⊈ ε_available) ⊢ b₂ : τ
--------------------------------------------------------
branch well-formed with type τ
```

Both branches MUST type-check to the same type τ, even though only one will be selected.

**Effect predicate evaluation:**

```
[Effect-Pred-Include]
comptime if effects_include(ε_test)
ε_test ⊆ ε_current_context
--------------------------------------------------------
select then-branch at compile time

[Effect-Pred-Exclude]
comptime if effects_exclude(ε_test)
ε_test ∩ ε_current_context = ∅
--------------------------------------------------------
select then-branch at compile time
```

### 6.15.4 Dynamic Semantics

**Compile-time selection only:**

```
[Exec-Comptime-Effect-Branch]
comptime if predicate { b₁ } else { b₂ }
predicate evaluated at compile time → selected branch bₛ
--------------------------------------------------------
code generation: emit only bₛ
runtime: execute bₛ with no branching overhead
```

The non-selected branch does not appear in generated code.

**Example use case:**

```cursive
procedure serialize<T, ε>(value: T): string
    where T: Display
    uses ε
    where ε ⊆ {alloc.heap, io.write}
{
    let result = value.to_string()  // uses alloc.heap

    comptime if effects_include(io.write) {
        println("Serializing: {result}")  // Debug logging if available
    }

    result result
}
```

### 6.15.5 With Blocks (Effect Implementation Override)

With blocks provide custom implementations for effect operations within a specific scope. This enables testing with mocks, dependency injection, and alternative runtime behaviors.

**Key properties:**

- Custom implementations override default implementations for a specific scope
- Multiple effects can be overridden in a single `with` block
- Partial implementations are allowed (unspecified operations use defaults)
- Implementations can be external (caller provides) or internal (procedure provides)

#### Syntax

The `WithBlock` production is defined in Appendix A.5.

```cursive
with EffectList {
    ImplementationList
} {
    Block
}
```

**Examples:**

```cursive
// External (caller provides)
with FileSystem {
    read(path) => mock_read(path)
    write(path, data) => mock_write(path, data)
} {
    foo()  // foo uses mock implementations
}

// Internal (procedure provides)
procedure foo()
    uses FileSystem
{
    with FileSystem {
        read(path) => cached_read(path)
    } {
        let a = FileSystem::read("/tmp/a")
        let b = FileSystem::read("/tmp/b")
    }
}

// Multiple effects
with FileSystem, Network {
    FileSystem::read(path) => mock_read(path)
    Network.get(url) => mock_get(url)
} {
    fetch_data()
}
```

#### Static Semantics

**Well-formedness:**

```
[With-Block-WF]
with E₁, ..., Eₙ { impl₁; ...; implₘ } { body }
∀i. Γ ⊢ Eᵢ well-formed effect
∀j. implⱼ implements operation from some Eᵢ
∀j. Γ ⊢ implⱼ : τⱼ where τⱼ matches operation signature
Γ, E₁ ↦ {impl₁, ...}, ..., Eₙ ↦ {...} ⊢ body : τ
------------------------------------------------------------
Γ ⊢ with E₁, ..., Eₙ { ... } { body } : τ
```

**Implementation compatibility:**

```
[With-Impl-Compat]
effect E { op(x: τ₁): τ₂ { default_body } }
with E { op(x) => custom_body } { ... }
Γ, x: τ₁ ⊢ custom_body : τ₂
------------------------------------------------------------
Implementation compatible
```

**Diagnostics:**

- **E7E11**: `with` block implementation does not match operation signature
- **E7E12**: `with` block references undefined effect
- **E7E13**: `with` block references undefined operation

#### Dynamic Semantics

**Implementation resolution:**

1. Check innermost `with` block for custom implementation
2. If not found, check outer `with` blocks
3. If not found, use default implementation
4. If no default, error (effect requires implementation)

**Evaluation:**

```
[E-With-Block]
with E { E.op(x) => e_impl } { body }
⟨body, σ⟩ ⇓ ⟨v, σ'⟩
where all E.op calls in body use λx. e_impl
------------------------------------------------------------
⟨with E { E.op(x) => e_impl } { body }, σ⟩ ⇓ ⟨v, σ'⟩
```

**Scoping:**

Custom implementations are lexically scoped to the `with` block body. Nested `with` blocks shadow outer implementations.

```cursive
with FileSystem {
    read(path) => outer_read(path)
} {
    FileSystem::read("/tmp/a")  // Uses outer_read

    with FileSystem {
        read(path) => inner_read(path)
    } {
        FileSystem::read("/tmp/b")  // Uses inner_read
    }

    FileSystem::read("/tmp/c")  // Uses outer_read again
}
```

CITE: Part VII §7.3.6 — Custom Implementations (With Blocks); Appendix A.5 — Statement Grammar.

---

**Definition 6.22 (Unreachable Paths):** The `unreachable` expression marks statically impossible control-flow paths, enabling optimizer hints and exhaustiveness verification.

## 6.16 Unreachable Paths in Control Flow

### 6.16.1 Overview

The `unreachable` expression (type `!`) indicates a control-flow path that is logically impossible. It integrates with reachability analysis to detect dead code and provides optimizer hints.

**Typing:** Defined in Part V as an expression of type `!`.

**Usage in control flow:**

- Default arms that should never execute
- Impossible state combinations in modal matching
- Dead branches after exhaustive checks

CITE: Part V — Expressions (unreachable expression typing).

### 6.16.2 Syntax and Usage

**Expression form:**

```cursive
unreachable       // Type: ! (never)
unreachable(msg)  // With message for debugging
```

**Usage in match:**

```cursive
enum Status { Active, Inactive }

let status: Status = get_status()
let is_active = check_active(status)

// After check, one branch is unreachable
let result: string = match status {
    Status::Active => {
        if !is_active {
            unreachable  // Impossible: status is Active but check failed
        }
        result "active"
    },
    Status::Inactive => "inactive",
}
```

### 6.16.3 Reachability Analysis Integration

**Static reachability:**

The compiler tracks reachable and unreachable code paths:

```
[Reachable-After-Divergence]
statement s has type !    (e.g., return, break, continue, unreachable, panic)
statement s' follows s in sequence
--------------------------------------------------------
s' is statically unreachable
```

**Unreachable code warning:**

```
[Warn-Unreachable-Code]
statement s is statically unreachable
--------------------------------------------------------
SHOULD WARN: Unreachable code detected
```

**Optimizer integration:**

When `unreachable` is encountered at runtime (due to faulty logic), behavior is undefined. Optimizers MAY assume `unreachable` is never reached and eliminate surrounding checks.

**Example with modal states:**

```cursive
modal Lock {
    @Unlocked
    @Locked { holder: ThreadId }

    @Unlocked -> acquire() -> @Locked
    @Locked -> release() -> @Unlocked
}

procedure use_lock(lock: Lock@Unlocked ∨ Lock@Locked) {
    match lock {
        Lock@Unlocked => {
            let acquired = lock.acquire()
            use_resource(acquired)
        },
        Lock@Locked => {
            // If we know lock is always Unlocked in practice
            unreachable  // Documents impossible path
        },
    }
}
```

---

**Definition 6.23 (Statement Sequencing and Execution Order):** Statement sequencing defines how statements execute in order and how control outcomes propagate through sequences.

## 6.17 Statement Sequencing and Execution Order

### 6.17.1 Execution Order

**Sequential execution:**

Statements in a sequence execute in textual order until a control-transfer outcome occurs:

```
[Exec-Sequence]
⟨s₁, σ⟩ ⇓ Normal(σ₁)
⟨s₂, σ₁⟩ ⇓ Normal(σ₂)
...
⟨sₙ, σₙ₋₁⟩ ⇓ Normal(σₙ)
--------------------------------------------------------
⟨s₁; s₂; ...; sₙ, σ⟩ ⇓ Normal(σₙ)
```

**Early termination:**

If any statement produces a control-transfer outcome, remaining statements do not execute:

```
[Exec-Sequence-Transfer]
⟨s₁, σ⟩ ⇓ Normal(σ₁)    ...    ⟨sᵢ, σᵢ₋₁⟩ ⇓ outcome
outcome ∈ {Return, Break, Continue, Panic}
--------------------------------------------------------
⟨s₁; ...; sₙ, σ⟩ ⇓ outcome
    (statements sᵢ₊₁...sₙ not executed)
```

**State threading:**

Each statement receives the store produced by the previous statement:

```
σ → s₁ → σ₁ → s₂ → σ₂ → ... → sₙ → σₙ
```

Side effects from earlier statements are visible to later statements.

**Determinism guarantee:**

Statement execution order is deterministic and strictly sequential. Unlike C (which has unspecified evaluation order for some constructs), Cursive guarantees that statements execute in the order written.

### 6.17.2 Statement Termination (Reference)

Statement termination semantics are defined in §6.1.2 and Foundations §2.7.


---


## 6.18 Diagnostics

Diagnostics for statement and control-flow behaviour are catalogued in Foundations §8 (entries `E600x`).


**Definition 6.25 (Inter-Part Authority):** This section establishes the normative boundaries and integration points between Part VI and other specification parts.

## 6.19 Inter-Part Authority and Cross-References

### 6.19.1 Normative Authority

**Authority Statement:** Part VI (Statements and Control Flow) is the authoritative specification for:

1. **Statement formation** — Abstract syntax, concrete grammar, and well-formedness for all statement forms
2. **Statement execution semantics** — Execution judgment, control outcomes, and dynamic behavior
3. **Control transfer** — Break, continue, return outcome propagation through statement sequences
4. **Defer execution** — LIFO ordering and interaction with all control-flow exits
5. **Labeled statements** — Usage of labels on loops and blocks; typed early-exit semantics
6. **Modal-aware control** — State refinement in branches, exhaustiveness over modal states
7. **Loop verification** — Invariants, variants, and postconditions on loops
8. **Block-level contracts** — Block preconditions, postconditions, and effect narrowing
9. **No-panic discipline** — Integration and usage rules for [[no_panic]] attribute
10. **Effect-gated branching** — Compile-time effect-based branch selection
11. **Reachability analysis** — Unreachable code detection and unreachable expression usage
12. **Statement sequencing** — Execution order and state threading

**Precedence:** Where conflicts arise between this chapter and other chapters regarding statement-specific semantics, this chapter takes precedence.

### 6.19.2 Deferred Authority

**This chapter defers normatively to:**

**Foundations §2.7 (AUTHORITATIVE):**

- Statement termination rules (newline-based with four continuation cases)
- Semicolon usage as statement separator
- Multi-line statement continuation

**Declarations §3.7 (AUTHORITATIVE):**

- Label scope (function-wide)
- Label uniqueness checking
- Label name resolution

**Declarations §3.2 (AUTHORITATIVE):**

- Variable declaration syntax (`let`, `var`, `shadow`)
- Single-binding rule and shadowing requirements
- Definite assignment analysis

**Declarations §3.2.8 (AUTHORITATIVE):**

- Assignment to immutable binding (E3D10)

**Expressions (Part V) (AUTHORITATIVE for expression forms):**

- If expressions (§5.7): complete typing and evaluation
- Match expressions (§5.8): pattern matching, exhaustiveness algorithm (enum), E5402
- Loop expressions (§5.9): all loop forms, typing, evaluation, E5404
- Block expressions (§5.2.4): result keyword requirement, E5401
- Return expressions (§5.14): typing as divergent expression
- Break/Continue (§5.9.5-6): typing as divergent expressions
- Defer expressions (§5.15): typing and defer queue management

**Foundations §4.3 (AUTHORITATIVE):**

- Attribute definitions: `[[verify]]`, `[[overflow_checks]]`, `[[no_panic]]`

**Part VII (Contracts and Effects) (AUTHORITATIVE):**

- Contract clause semantics (`uses`, `must`, `will`)
- Effect checking and propagation algorithms
- Verification mode implementations (static, runtime, none)

**Part VIII (Regions) (AUTHORITATIVE):**

- Region block syntax and semantics
- Escape analysis algorithms
- Allocation and deallocation mechanics

### 6.19.3 Integration Points

**With Part I (Foundations):**

- Notation (§1.3): statement and outcome metavariables
- Judgment forms (§1.4): execution judgment `⟨s, σ⟩ ⇓ outcome`
- Inference rules (§1.6): format for static and dynamic semantics
- Abstract syntax (§3.3): extended with statement forms
- Termination (§2.7): referenced for statement separation
- Attributes (§4.3): verify, overflow_checks, no_panic

**With Part II (Types):**

- Modal types (§2.3.3): state machines, state graph, transition validation
- Permissions (§2.0.8.1): own/mut/imm checking in assignments and control flow
- Effect signatures (§2.0.8.2): effect sets in block narrowing and no-panic

**With Part III (Declarations and Scope):**

- Variable bindings (§3.2): let/var semantics, initialization
- Scope hierarchy (§3.7): block scope introduction, label scope
- Definite assignment (§3.2.7): integration with control-flow paths
- Storage (§3.9): automatic, static, region storage classes
- RAII (§3.9.3): cleanup order at scope exit

**With Part V (Expressions):**

- Value categories (§5.0.4): lvalue requirement for assignments
- All control-flow expressions: typing delegated to Part V
- Precedence (§5.1.2): operator precedence in compound assignments
- Effect composition (§5.19): effect propagation in statement sequences

**With Part VII (Contracts and Effects):**

- Contract clauses: block and loop contracts
- Effect narrowing: subset checking
- Verification modes: static/runtime/none strategies

**With Part VIII (Regions):**

- Per-iteration regions: region stack discipline
- Escape prevention: static checking at iteration boundaries

### 6.19.4 Cross-Reference Table

| Concept                  | Authoritative Part     | This Chapter's Role                        |
| ------------------------ | ---------------------- | ------------------------------------------ |
| Statement termination    | Foundations §2.7       | Reference only (no redefinition)           |
| Label scope/uniqueness   | Declarations §3.7      | Usage rules on loops/blocks                |
| Variable declarations    | Declarations §3.2      | Execution semantics only                   |
| Definite assignment      | Declarations §3.2.7    | Integration with control flow              |
| Block expressions        | Expressions §5.2.4     | Usage as statements; result requirement    |
| If/Match                 | Expressions §5.7-8     | Usage as statements; modal refinement      |
| Loops                    | Expressions §5.9       | Usage as statements; verification clauses  |
| Break/Continue/Return    | Expressions §5.9/5.14  | Control transfer outcome semantics         |
| Defer                    | Expressions §5.15      | LIFO execution order specification         |
| Assignment               | This chapter (§6.4)    | Authoritative for statement form           |
| Modal refinement         | This chapter (§6.10.2) | Authoritative for state refinement rules   |
| Modal exhaustiveness     | This chapter (§6.10.3) | Authoritative for state coverage           |
| Loop invariants/variants | This chapter (§6.11.2) | Authoritative for loop contracts           |
| Loop postconditions      | This chapter (§6.11.3) | Authoritative for loop result guarantees   |
| Region iteration         | This chapter (§6.12)   | Authoritative for per-iteration discipline |
| Block contracts          | This chapter (§6.13.2) | Authoritative for block-scope contracts    |
| Effect narrowing         | This chapter (§6.13.3) | Authoritative for block effect subsets     |
| No-panic integration     | This chapter (§6.14)   | Authoritative for usage and checking       |
| Effect-gated branching   | This chapter (§6.15)   | Authoritative for comptime effect branch   |
| Unreachable in control   | This chapter (§6.16)   | Authoritative for usage and reachability   |
| Statement sequencing     | This chapter (§6.17.1) | Authoritative for execution order          |
| Control outcomes         | This chapter (§6.0.6)  | Authoritative for outcome classification   |

---

**Definition 6.26 (Comprehensive Examples):** This section provides comprehensive examples demonstrating all statement forms and innovations in practical usage.

## 6.20 Comprehensive Examples and Patterns

### 6.20.1 Basic Statement Forms

**Block with result:**

```cursive
let value: i32 = {
    let base = 10
    let modifier = compute_modifier()
    result base * modifier  // Explicit result required
}
```

**Variable declarations and assignment:**

```cursive
// Immutable binding
let x = 42
// x = 50  // ERROR E3D10: cannot assign to immutable

// Mutable binding
var y = 10
y = 20     // OK
y += 5     // OK: compound assignment

// Shadowing
let outer = 1
{
    shadow let outer = 2
    println(outer)  // Prints 2
}
println(outer)  // Prints 1
```

**Expression statements:**

```cursive
procedure demo()
    uses io.write
{
    println("Starting")  // Expression statement
    compute_value()      // WARNING: unused i32 result
    process_data()       // OK: returns ()
}
```

### 6.20.2 Control Flow Patterns

**Early return with defer:**

```cursive
function validate_and_process(data: Data): Result<Output, Error>
    uses fs.write, alloc.heap
{
    let log_file = open_log()
    defer { log_file.close() }  // Always executes

    if !data.is_valid() {
        log_file::write("Invalid data")
        return Err("validation failed")  // Defer executes before return
    }

    let processed = process(data)
    log_file::write("Success")
    result Ok(processed)  // Defer executes before normal return
}
```

**Labeled early-exit from block:**

```cursive
let result: Result<Data, Error> = 'parse: {
    let header = parse_header(input)
    if header.is_err() {
        break 'parse Err("bad header")
    }

    let body = parse_body(input)
    if body.is_err() {
        break 'parse Err("bad body")
    }

    result Ok(Data { header: header.unwrap(), body: body.unwrap() })
}
```

**Multi-level loop control:**

```cursive
'search: loop row: usize in 0..height {
    loop col: usize in 0..width {
        if matrix[row][col] == target {
            break 'search Option::Some((row, col))
        }
    }
}
// If not found, loop completes normally
```

### 6.20.3 Modal Programming Patterns

**State refinement and transitions:**

```cursive
modal Connection {
    @Disconnected { url: string }
    @Connecting { url: string, timeout: Duration }
    @Connected { url: string, socket: Socket }
    @Failed { url: string, error: Error }

    @Disconnected -> connect() -> @Connecting
    @Connecting -> succeed() -> @Connected
    @Connecting -> fail() -> @Failed
    @Connected -> disconnect() -> @Disconnected
}

procedure handle_connection(conn: Connection@Disconnected ∨ Connection@Failed) {
    let active: Connection@Connected ∨ Connection@Failed = match conn {
        c @ Connection@Disconnected => {
            // Call transition procedure directly
            result c::connect()
        },
        c @ Connection@Failed => {
            println("Already failed: {c.error}")
            result c  // No transition
        },
    }

    // active : Connection@Connected ∨ Connection@Failed
    match active {
        c @ Connection@Connected => {
            send_data(c)
        },
        c @ Connection@Failed => {
            handle_error(c.error)
        },
    }
}
```

**Exhaustive modal state coverage:**

```cursive
modal Task {
    @Pending
    @Running { progress: f64 }
    @Completed { result: Data }
    @Cancelled
}

procedure status_message(task: Task): string {
    result match task {
        Task@Pending => "not started",
        Task@Running => "in progress",
        Task@Completed => "finished",
        Task@Cancelled => "cancelled",
    }  // Exhaustive: all 4 states covered
}
```

### 6.20.4 Verification Patterns

**Loop with invariants and variant:**

```cursive
[[verify(static)]]
function binary_search(arr: [i32], target: i32): Option<usize>
    must is_sorted(arr)
{
    var low: usize = 0
    var high: usize = arr.len()

    loop low < high by high - low
        with {
            0 <= low,
            high <= arr.len(),
            low <= high,
            forall(i in 0..low) { arr[i] < target },
            forall(i in high..arr.len()) { arr[i] > target }
        }
    {
        let mid = low + (high - low) / 2

        if arr[mid] < target {
            low = mid + 1
        } else if arr[mid] > target {
            high = mid
        } else {
            break Option::Some(mid)
        }
    }

    result Option::None
}
// Compiler proves: invariants preserved, variant decreases, terminates
```

**Loop postcondition:**

```cursive
function find_max(arr: [i32]): Option<i32>
    must arr.len() > 0
    will {
        match result {
            Option::Some(max) => forall(x in arr) { x <= max },
            Option::None => false  // Impossible given precondition
        }
    }
{
    var max_val = arr[0]

    let found: Option<i32> = loop i: usize in 1..arr.len()
        will {
            match result {
                Option::Some(m) => forall(x in arr[0..i]) { x <= m },
                Option::None => true
            }
        }
    {
        if arr[i] > max_val {
            max_val = arr[i]
        }
    }

    result Option::Some(max_val)
}
```

### 6.20.5 Region-Scoped Iteration

**Per-iteration cleanup:**

```cursive
procedure process_large_dataset(items: [Item])
    uses alloc.region, io.write
{
    loop item: Item in items region iter {
        // Allocate temporaries in iteration region
        let buffer = alloc_in<iter>(Buffer.new(LARGE_SIZE))
        let processed = alloc_in<iter>(transform(item))

        buffer::write(processed)
        output(buffer)

        // buffer and processed freed here (O(1))
        // No memory accumulation across iterations
    }
}
```

### 6.20.6 Block Contracts and Effect Narrowing

**Block precondition:**

```cursive
procedure safe_divide(a: i32, b: i32): Result<i32, string> {
    if b == 0 {
        result Err("division by zero")
    }

    // Block with precondition
    let quotient = contract(b != 0) {
        a / b  // Safe: b ≠ 0 guaranteed
    } will result == a / b

    result Ok(quotient)
}
```

**Effect narrowing:**

```cursive
procedure mixed_io(data: Data)
    uses io.read, io.write, net.write, alloc.heap
{
    // Narrow to only local I/O
    uses io.read, io.write {
        let input = read_input()
        write_output(process(input))
        // send_network(data)  // ERROR E4004: net.write not available
    }

    // Full effects available
    let buffer = Vec.new()     // alloc.heap
    send_network(buffer)       // net.write
}
```

### 6.20.7 No-Panic Safety-Critical Code

**Safety-critical function:**

```cursive
[[no_panic]]
[[verify(static)]]
function safe_control_law(sensor: i32, setpoint: i32): i32
    must sensor >= 0
    must setpoint >= 0
    will {
        result >= 0,
        result <= MAX_OUTPUT
    }
{
    let error = setpoint - sensor
    let output = clamp(error * GAIN, 0, MAX_OUTPUT)

    // let div = error / 0  // ERROR E6009: may panic
    // panic("test")        // ERROR E6009: explicit panic forbidden

    result output  // Proven non-panic, bounded output
}
```

### 6.20.8 Effect-Gated Implementation Selection

**Adapting to capabilities:**

```cursive
procedure cache_result<T, ε>(key: string, value: T)
    uses ε
    where ε ⊆ {alloc.heap, io.write}
{
    comptime if effects_include(alloc.heap) {
        // Heap available: use persistent cache
        global_cache.insert(key, value)
    } else {
        // No heap: skip caching
        // (Or could use stack-based temporary cache)
    }

    comptime if effects_include(io.write) {
        println("Cached: {key}")
    }
}
```

---

**Previous**: [Expressions and Operators](04_Expressions-and-Operators.md) | **Next**: [Contracts and Effects](06_Contracts-and-Effects.md)
