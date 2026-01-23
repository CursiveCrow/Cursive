# The Cursive Language Specification

**Part**: V - Expressions and Operators
**File**: 05_Expressions-and-Operators.md
**Previous**: [Lexical Permission System](04_Lexical-Permissions.md) | **Next**: [Statements and Control Flow](06_Statements-and-Control-Flow.md)

---

## Abstract

This chapter specifies the Cursive expression system, defining how values are computed through operators, function calls, pattern matching, and control flow. The expression system enforces memory safety through permission-aware evaluation and provides deterministic semantics through strict left-to-right evaluation. It supports AI-assisted development through required explicitness in block returns, match bindings, pipeline stages, and loop iterators.

**Definition 5.1 (Expression System):** The Cursive expression system comprises:

- The universe of expressions **Expr** from Foundations §3.3
- The typing judgment `Γ ⊢ e : τ ! ε` (expression e has type τ with effects ε)
- The evaluation judgment `⟨e, σ⟩ ⇓ ⟨v, σ'⟩` (expression e evaluates to value v)
- The operator precedence relation (15 levels)
- The value category classification (lvalue, rvalue, place)

The system MUST provide deterministic evaluation, explicit type annotations at key boundaries, effect composition tracking, and integration with the permission system (Part IV), effect system (Part VI), and region system (Part VII).

**Required Explicitness Features (New in v1.3.0):**

This specification introduces four REQUIRED explicitness features that change the language from Foundations §3.3:

1. **`result` keyword REQUIRED** in block expressions (§5.2.4)
2. **Type annotations REQUIRED** on match expression bindings (§5.8.1)
3. **Type annotations REQUIRED** on pipeline stages (§5.5.9)
4. **Type annotations REQUIRED** on loop iterator bindings (§5.9.3)

_Rationale:_ These requirements enforce the design principle "explicit over implicit" and eliminate type inference puzzles for LLM code generation.

**Conformance:** See Part I §1 for conformance criteria, RFC2119 keywords, and document conventions.

---

**Definition 5.2 (Expression Foundations):** This section establishes the scope, evaluation model, effect propagation, value categories, and cross-part dependencies for the expression system.

## 5.0 Expression Foundations

### 5.0.1 Scope

This chapter provides the complete normative specification of expression syntax, static typing, and dynamic evaluation for all expression forms in Cursive.

**Normative coverage:**

- All expression forms from Foundations §3.3 abstract syntax
- Concrete syntax (EBNF grammar) for each expression construct
- Static semantics (typing rules `Γ ⊢ e : τ ! ε`) for type checking
- Dynamic semantics (evaluation rules `⟨e, σ⟩ ⇓ ⟨v, σ'⟩`) for execution
- Operator precedence and associativity (15 levels, normative)
- Effect composition and propagation across compound expressions
- Value categories (lvalue, rvalue, place) and permission interaction
- Complete diagnostic catalog (E4001-E4017, E4401-E4404)

**Expression forms specified (from Foundations §3.3):**

**Primary expressions:**

- Literals (integer, float, boolean, character, string)
- Variable references
- Parenthesized expressions
- Block expressions `{ statements; result expr }`
- Unit `()`
- Record construction `RecordName { fields }`
- Enum variant construction `EnumName.Variant(payload)`
- Array literals `[elements]`, `[value; count]`
- Tuple literals `(expr1, expr2, ...)`

**Postfix expressions:**

- Function calls `f(args)`
- Type-scope procedure calls `receiver::procedure(args)`
- Field access `expr.field`
- Tuple projection `expr.N`
- Array indexing `arr[index]`
- Slice creation `arr[start..end]`

**Unary expressions:**

- Logical NOT `!expr`
- Arithmetic negation `-expr`
- Pointer dereference `*ptr`
- Ownership transfer `move expr`

**Binary expressions:**

- Arithmetic operators `+`, `-`, `*`, `/`, `%`
- Power operator `**`
- Bitwise operators `&`, `|`, `^`
- Shift operators `<<`, `>>`
- Comparison operators `<`, `<=`, `>`, `>=`
- Equality operators `==`, `!=`
- Logical operators `&&`, `||`
- Range operators `..`, `..=`
- Pipeline operator `=>`

**Assignment expressions:**

- Simple assignment `target = expr`
- Compound assignment `+=`, `-=`, `*=`, `/=`, `%=`, etc.

**Conditional expressions:**

- If expression `if cond { e1 } else { e2 }`
- If-let expression `if let pattern = expr { e1 } else { e2 }`

**Match expressions:**

- Pattern matching `match expr { patterns => arms }`
- Guard clauses `pattern if condition => expr`
- Exhaustiveness checking

**Loop expressions:**

- Infinite loop `loop { body }`
- Conditional loop `loop condition { body }`
- Iterator loop `loop item: Type in collection { body }`
- Loop contracts `with { invariants }`, `by variant`
- Break `break`, `break value`, `break 'label value`
- Continue `continue`, `continue 'label`

**Closure expressions:**

- Closure literals `|params| -> expr`, `|params| { block }`
- Capture semantics (own, mut, imm)
- Effect propagation
- Region escape prevention

**Region expressions:**

- Region blocks `region r { body }`
- Region allocation `alloc_in<r>(value)`

**Compile-time expressions:**

- Comptime blocks `comptime { body }`

**Type conversion expressions:**

- Explicit casts `expr as Type`
- Permission coercions (implicit)
- Modal coercions (implicit/explicit)
- Unsafe transmute (intrinsic)

**Control transfer expressions:**

- Return `return expr`
- Defer `defer { block }`

**Contract and modal expressions:**

- Contract annotations `contract(P, expr, Q)`
- Explicit transitions `transition(expr, @State)`

**Sequencing expressions:**

- Statement sequencing `expr1 SEP expr2` where SEP ∈ {newline, `;`}

### 5.0.2 Evaluation Model

**Definition 5.3 (Evaluation Model):** Cursive employs a strict, deterministic evaluation model with the following properties:

**Evaluation strategy:** Call-by-value. All arguments to functions are fully evaluated to values before the function body executes.

**Evaluation order:** Strict left-to-right. Subexpressions are evaluated in left-to-right order with one exception (short-circuit logical operators).

**Determinism:** Unlike C/C++ (which leave many evaluation orders unspecified), Cursive makes two distinct guarantees:

1. **Deterministic ORDER**: Evaluation order is always deterministic (strict left-to-right), even for expressions with I/O or concurrency
2. **Deterministic RESULT**: For any expression without I/O or concurrency effects, repeated evaluation with the same initial state produces the same result value and final state

Expressions with I/O or concurrency have deterministic evaluation order but may produce different results on repeated evaluation due to external factors (user input, file contents, network responses, thread scheduling, etc.).

**Formal properties:**

```
[Eval-Deterministic]
⟨e, σ⟩ ⇓ ⟨v₁, σ'⟩
⟨e, σ⟩ ⇓ ⟨v₂, σ''⟩
e has no I/O or concurrency effects
------------------------------------
v₁ = v₂ ∧ σ' = σ''
```

**Evaluation guarantee:** Strict left-to-right evaluation ensures:

1. Function arguments evaluate left-to-right before the call
2. Binary operator operands evaluate left operand first, then right
3. Tuple/array/record fields evaluate in declaration order
4. Sequential expressions evaluate in textual order

**Exception:** Logical operators `&&` and `||` use short-circuit evaluation and MAY skip the right operand (§5.5.7). CITE: §5.20 — Evaluation Order and Determinism.

### 5.0.3 Effect Propagation

Effects compose via union: compound expressions require all subexpression effects. CITE: Part VI — Contracts and Effects (authoritative effect system).

### 5.0.4 Value Categories

**Definition 5.5 (Value Categories):** Expressions are classified into three value categories that determine their usage contexts and interaction with the permission system.

**Lvalue (Location Value):**

An lvalue is an expression denoting a memory location that can be assigned to.

**Properties:**

- Can appear on left-hand side of assignment
- Has a memory address
- Persists beyond expression evaluation
- Can be bound with `mut` or `own` permissions

**Lvalue forms:**

- Variable references (if `var` binding): `x`
- Field access: `expr.field`
- Tuple projection: `expr.N`
- Array indexing: `arr[index]`
- Dereference (of mutable pointer): `*mut_ptr`

**Rvalue (Result Value):**

An rvalue is an expression denoting a temporary value without a persistent location.

**Properties:**

- Cannot be assigned to
- May be anonymous/temporary
- Ceases to exist after expression evaluation
- Can only be bound with `imm` permission (read-only)

**Rvalue forms:**

- Literals: `42`, `"text"`, `true`
- Arithmetic results: `x + y`
- Function calls: `f(args)`
- Temporary constructions: `Point { x: 1.0, y: 2.0 }`

**Place:**

A place is a memory location with an associated permission (own, mut, imm).

**Properties:**

- Combines location with permission
- Determines what operations are allowed
- Checked at compile time by LPS

**Permission interaction with value categories:**

```
[Lvalue-Mut-Required]
target = expr
----------------------------
target must be lvalue with mut or own permission

[Rvalue-Imm-Only]
expr is rvalue
----------------------------
expr can only be accessed with imm permission
```

CITE: §5.21 — Value Categories (complete specification); Part IV — Lexical Permission System.

### 5.0.5 Cross-Part Dependencies

This chapter integrates with the following specification parts:

**Part I (Foundations):**

- §1.1: Grammar notation (EBNF conventions)
- §1.3: Metavariables (e, τ, ε, Γ, σ)
- §1.4: Judgment forms (typing, evaluation, effects)
- §1.6: Inference rule format
- §2.6: Lexical syntax of literals
- §2.7: Statement termination rules (newline, continuation)
- §3.3: Abstract expression syntax (authoritative for expression forms)
- §3.4: Pattern syntax

CITE: Part I — Foundations.

**Part II (Types):**

- §2.0.6.1: Subtyping relation `<:`
- §2.0.6.6: Numeric promotion policy (no implicit conversions)
- §2.1: Primitive types (integer, float, bool, char, string, !, ())
- §2.2: Product types (tuples, records, tuple-structs)
- §2.3: Sum and modal types (enums, modals)
- §2.4: Collections (arrays, slices)
- §2.6: Raw pointers
- §2.10: Map types (function types)
- §2.12.3: Intrinsics (transmute)

CITE: Part II — Type System.

**Part III (Declarations):**

- §3.2.7: Definite assignment checking
- §3.2.8: Assignment to immutable binding (E3D10)
- §3.3: Compile-time evaluation (comptime blocks, const expressions)
- §3.7.1: Scope hierarchy (block, function, module)
- §3.9.2: Region escape prevention
- §3.10: Visibility and access control
- §3.11: Name resolution algorithm

CITE: Part III — Declarations and Scope.

**Part VI (Contracts and Effects):**

- Effect lattice laws (∪, ⊆, idempotence)
- Effect tokens (alloc.heap, io.read, unsafe.ptr, etc.)
- Effect checking at procedure boundaries
- Principal effects and inference

CITE: Part VI — Contracts and Effects.

**Part VII (Regions):**

- Region lifetime constraints
- Region allocation (`alloc_in<r>`)
- Escape analysis
- LIFO deallocation

CITE: Part VII — Regions.

**Part IX (Functions and Procedures):**

- Function return type matching
- Procedure effect declarations (`uses ε`)
- Parameter passing semantics
- Procedure dispatch

CITE: Part IX — Functions and Procedures.

### 5.0.6 Design Principles

This chapter enforces the following design principles through normative requirements:

**1. Explicit over implicit:**

All type-critical boundaries require explicit annotations:

- Block expressions MUST use `result` keyword (§5.2.4)
- Match expressions MUST annotate result type (§5.8.1)
- Pipeline stages MUST annotate intermediate types (§5.5.9)
- Loop iterators MUST annotate element type (§5.9.3)

**Rationale:** Eliminates type inference puzzles for LLMs and human readers. Makes expected types visible at every transformation boundary.

**2. Local reasoning:**

Every expression's type and effects are determinable from local context:

- Effects visible through `uses` clauses and call sites
- Permissions explicit in types (`own T`, `mut T`, `imm T`)
- No hidden coercions or implicit conversions (except documented permission coercions)

**3. Deterministic evaluation:**

Strict left-to-right evaluation eliminates undefined behavior:

- No unspecified evaluation order (unlike C)
- Side effects occur in predictable sequence
- Parallelism requires explicit concurrent constructs (not in this chapter)

**4. Zero-cost abstractions:**

All expression checking occurs at compile time:

- Type checking, effect checking, permission checking: compile-time
- No runtime type tags or dynamic dispatch (except explicit trait objects)
- Evaluation rules describe runtime behavior without overhead

**5. LLM-friendly:**

Predictable patterns for AI code generation:

- Required annotations eliminate inference
- No complex subtyping puzzles
- Clear error messages with specific diagnostics

---

**Definition 5.6 (Expression Grammar):** This section establishes the complete concrete syntax of expressions through EBNF grammar, the normative 15-level operator precedence hierarchy, and integration with statement termination rules.

## 5.1 Expression Grammar and Precedence

Expression productions and precedence are centralised in Foundations Appendix A.3. This chapter references those nonterminals and precedence levels directly; implementers should consult Appendix A.3 for the canonical EBNF.

## 5.2 Primary Expressions

### 5.2.1 Literals

Literals evaluate to themselves without state change: `⟨literal, σ⟩ ⇓ ⟨literal, σ⟩`. Grammar: Appendix A.1, Foundations §2.6. Typing: Part II §2.1.

| Literal | Default Type | Example | Special Notes |
|---------|--------------|---------|---------------|
| Integer | `i32` | `42`, `0xFF`, `100u64` | Suffixes override, hex/bin/oct supported |
| Float | `f64` | `3.14`, `1e10`, `2.0f32` | IEEE 754, special values: NaN, ±∞, ±0 |
| Boolean | `bool` | `true`, `false` | - |
| Character | `char` | `'A'`, `'🚀'`, `'\n'` | Unicode scalar, rejects surrogates [U+D800-DFFF] |
| String | `string@View` | `"hello"`, `"multi\nline"` | Static storage, zero allocation, UTF-8 validated |

CITE: Foundations §2.6.2 — String Literals; Part II §2.1.6 — String Types (modal string, Owned vs View).

### 5.2.2 Variable References

Variable references are expressions that evaluate to the value bound to a name.

#### 5.2.2.1 Syntax

A variable reference is an identifier used in expression position:

Variable references follow the `VarExpr` production in Appendix A.4.

The identifier must be in scope and initialized.

#### 5.2.2.2 Typing

```
[T-Var]
(x : τ) ∈ Γ
x is initialized
-----------
Γ ⊢ x : τ
```

The type of a variable reference is the type assigned to that variable in the typing context.

**Name resolution:**

The identifier is resolved using the algorithm defined in Part III §3.11:

1. Current block scope
2. Outward lexical scopes (innermost to outermost)
3. Module scope
4. Imported namespaces
5. Universe scope (predeclared identifiers)

If the identifier is not found in any scope, a compile-time error occurs.

**Definite assignment:**

The variable must be definitely initialized on all control flow paths before use. This is verified by the definite assignment algorithm defined in Part III §3.2.7.

```
[T-Var-Uninitialized]
(x : τ) ∈ Γ
x not initialized on all paths
------------------------------
ERROR E4010: Use of possibly uninitialized variable 'x'
```

CITE: Part III §3.11 — Name Resolution Algorithm; Part III §3.2.7 — Definite Assignment.

#### 5.2.2.3 Evaluation

```
[E-Var]
(x ↦ ℓ) ∈ env
σ(ℓ) = v
-----------------------
⟨x, σ⟩ ⇓ ⟨v, σ⟩
```

A variable reference evaluates to the current value stored at that variable's memory location.

**Evaluation properties:**

- No state change (σ unchanged)
- No side effects
- No allocation

#### 5.2.2.4 Use-After-Move Checking

After moving `own` values, original bindings are invalid (E4006). CITE: Part IV — Lexical Permission System (authoritative ownership semantics).

### 5.2.3 Parenthesized Expressions

**Syntax:** Parenthesized expressions follow the `ParenExpr` production in Appendix A.4.

**Typing:**

```
[T-Paren]
Γ ⊢ e : τ
-----------------
Γ ⊢ (e) : τ
```

Parentheses do not change the type of the enclosed expression.

**Evaluation:**

```
[E-Paren]
⟨e, σ⟩ ⇓ ⟨v, σ'⟩
----------------------
⟨(e), σ⟩ ⇓ ⟨v, σ'⟩
```

Parentheses do not change evaluation semantics.

**Purpose:**

Parentheses override operator precedence and improve readability:

```cursive
(x + y) * z     // Forces addition before multiplication
x + (y * z)     // Explicit (matches default precedence)
```

### 5.2.4 Block Expressions

**REQUIRED EXPLICITNESS FEATURE:** Block expressions MUST use the `result` keyword to return a value. This changes the specification from Foundations §3.3.

#### 5.2.4.1 Grammar

Block expression forms (`BlockExpr`) are defined in Appendix A.4; the required `result` keyword is elaborated here.

**Two forms:**

1. **Value-producing block:** Contains `result` keyword before final expression
2. **Unit block:** Contains only statements, evaluates to `()`

**Rule 5.2.4-1 (Block Result Annotation).** A block expression that produces a non-unit value MUST include `result` before its final expression; implicit returns are forbidden.

#### 5.2.4.2 Typing

```
[T-Block-Result]
Γ ⊢ s₁    ...    Γ ⊢ sₙ    (statements type-check)
Γ' ⊢ e : τ                  (final expr with extended context Γ')
Γ' extends Γ with bindings from s₁...sₙ
----------------------------------------
Γ ⊢ { s₁; ...; sₙ; result e } : τ

[T-Block-Unit]
Γ ⊢ s₁    ...    Γ ⊢ sₙ
------------------------------------
Γ ⊢ { s₁; ...; sₙ } : ()
```

**Type annotation requirement:**

When a block expression is bound to a variable, the binding MUST have a type annotation:

```cursive
let value: Type = {    // Type annotation REQUIRED
    statements
    result expr
}
```

**Context extension:**

Statements within the block may introduce new bindings (via `let` or `var` declarations). These bindings extend the context for subsequent statements and the result expression.

#### 5.2.4.3 Evaluation

```
[E-Block-Result]
⟨s₁, σ⟩ ⇓ σ₁    ...    ⟨sₙ, σₙ₋₁⟩ ⇓ σₙ    (execute statements)
⟨e, σₙ⟩ ⇓ ⟨v, σ'⟩                        (evaluate result expression)
------------------------------------------
⟨{ s₁; ...; sₙ; result e }, σ⟩ ⇓ ⟨v, σ'⟩

[E-Block-Unit]
⟨s₁, σ⟩ ⇓ σ₁    ...    ⟨sₙ, σₙ₋₁⟩ ⇓ σₙ
------------------------------------------
⟨{ s₁; ...; sₙ }, σ⟩ ⇓ ⟨(), σₙ⟩
```

Statements execute in order, each potentially modifying the store. The result expression (if present) is evaluated in the final store state.

#### 5.2.4.4 Scope and Diagnostics

Blocks introduce new lexical scope. CITE: Part III §3.7. Missing `result` keyword in value position triggers E4401.

### 5.2.5 Unit

**Syntax:** The `UnitExpr` production is defined in Appendix A.4.

**Type:**

```
[T-Unit]
------------------
Γ ⊢ () : ()
```

The unit expression `()` has type `()` (the unit type).

**Evaluation:**

```
[E-Unit]
------------------------
⟨(), σ⟩ ⇓ ⟨(), σ⟩
```

The unit value evaluates to itself.

**Usage:**

Unit is used when no meaningful value is needed:

- Functions/procedures that perform side effects but return no value
- Blocks that execute statements without producing a result
- Default result when else branch is omitted in if expression

**Examples:**

```cursive
let unit_val = ()                    // Type: ()
procedure print_msg(msg: string)     // Return type: () (implicit)
    uses io.write
{
    println(msg)
}  // Returns () implicitly
```

CITE: Part II §2.1 — Primitive Types (unit type).

### 5.2.6 Record Construction Expressions

Record construction expressions create instances of record types by providing values for all fields.

#### 5.2.6.1 Syntax

Record literal syntax (`RecordExpr` and `FieldInit`) is defined in Appendix A.4.

**Field shorthand:**

When the field name matches a variable name in scope, the shorthand `{ field }` is equivalent to `{ field: field }`.

**Examples:**

```cursive
// Explicit initialization
let p1 = Point { x: 10.0, y: 20.0 }

// Mixed explicit and shorthand
let x = 5.0
let y = 10.0
let p2 = Point { x, y }  // Equivalent to Point { x: x, y: y }

// Nested construction
let rect = Rectangle {
    top_left: Point { x: 0.0, y: 0.0 },
    bottom_right: Point { x: 100.0, y: 100.0 }
}
```

#### 5.2.6.2 Typing

```
[T-Record-Lit]
record R { f₁: τ₁, ..., fₙ: τₙ } declared
Γ ⊢ e₁ : τ₁    ...    Γ ⊢ eₙ : τₙ
all fields f₁, ..., fₙ provided
field order may differ from declaration
--------------------------------------------
Γ ⊢ R { f₁: e₁, ..., fₙ: eₙ } : R
```

**Requirements:**

- All fields MUST be provided (no partial initialization)
- Field types MUST match declaration types exactly
- Field names MUST exist in the record declaration
- Duplicate field names are an error

**Type checking errors:**

```cursive
record Point { x: f64, y: f64 }

let invalid = Point { x: 10.0 }        // ERROR: missing field 'y'
let wrong_type = Point { x: 10, y: 20.0 }  // ERROR: x expects f64, got i32
let extra = Point { x: 1.0, y: 2.0, z: 3.0 }  // ERROR: no field 'z'
let duplicate = Point { x: 1.0, x: 2.0, y: 3.0 }  // ERROR: duplicate field 'x'
```

#### 5.2.6.3 Evaluation

```
[E-Record-Ctor]
⟨e₁, σ⟩ ⇓ ⟨v₁, σ₁⟩    ...    ⟨eₙ, σₙ₋₁⟩ ⇓ ⟨vₙ, σₙ⟩
----------------------------------------------------------
⟨R { f₁: e₁, ..., fₙ: eₙ }, σ⟩ ⇓ ⟨R { f₁: v₁, ..., fₙ: vₙ }, σₙ⟩
```

**Evaluation order:** Fields are evaluated in left-to-right order as written in the source (NOT necessarily in declaration order).


**Examples:**

```cursive
// Evaluation order demonstration
record Data { a: i32, b: i32 }

var counter = 0
procedure next(): i32 {
    counter += 1
    result counter
}

let d = Data { a: next(), b: next() }
// Fields evaluate left-to-right: a = 1, b = 2
```

#### 5.2.6.4 Visibility Checks

**Field visibility:**

Private fields can only be accessed within the same module as the record declaration.

```
[Record-Private-Field-Check]
record R in module M
field f of R has private visibility
construction in module M' where M' ≠ M
-----------------------------------------
ERROR: Cannot access private field 'f' from different module
```

**Examples:**

```cursive
// Module A
record Account {
    public id: u64
    private balance: i64
}

// Module A (same module)
let acct = Account { id: 1, balance: 100 }  // OK

// Module B (different module)
use A::Account
let invalid = Account { id: 1, balance: 100 }  // ERROR: 'balance' is private
```

CITE: Part III §3.10 — Visibility and Access Control; Part II §2.2.2 — Records.

### 5.2.7 Enum Variant Construction

Enum variant construction creates an instance of an enum type by specifying the variant and its payload (if any).

#### 5.2.7.1 Syntax

Enum construction syntax (`EnumExpr`) is defined in Appendix A.4.

**Three variant forms:**

1. **Unit variant:** No payload data
2. **Tuple variant:** Positional payload fields
3. **Struct variant:** Named payload fields

**Examples:**

```cursive
// Unit variant
let none_val = Option::None

// Tuple variant
let some_val = Option::Some(42)
let ok_result = Result::Ok("success")

// Struct variant
enum Message {
    Quit
    Move { x: i32, y: i32 }
    Write(string)
}

let msg1 = Message.Quit
let msg2 = Message.Move { x: 10, y: 20 }
let msg3 = Message.Write("hello")
```

#### 5.2.7.2 Typing

```
[T-Enum-Unit-Variant]
enum E { ..., V, ... } declared
------------------------------
Γ ⊢ E.V : E

[T-Enum-Tuple-Variant]
enum E { ..., V(τ₁, ..., τₙ), ... } declared
Γ ⊢ e₁ : τ₁    ...    Γ ⊢ eₙ : τₙ
----------------------------------------------
Γ ⊢ E.V(e₁, ..., eₙ) : E

[T-Enum-Struct-Variant]
enum E { ..., V { f₁: τ₁, ..., fₙ: τₙ }, ... } declared
Γ ⊢ e₁ : τ₁    ...    Γ ⊢ eₙ : τₙ
----------------------------------------------------------
Γ ⊢ E.V { f₁: e₁, ..., fₙ: eₙ } : E
```

**Requirements:**

- Variant name must exist in enum declaration
- Payload types must match variant declaration
- All payload fields must be provided

**Type checking errors:**

```cursive
enum Option<T> { Some(T), None }

let invalid = Option::Some()           // ERROR: Some expects 1 argument
let wrong = Option::Some(42, 43)       // ERROR: Some expects 1 argument, got 2
let unknown = Option::Maybe(42)        // ERROR: no variant 'Maybe' in Option
```

#### 5.2.7.3 Evaluation

```
[E-Enum-Unit-Variant]
variant V has index i in enum E
----------------------------------------------------------
⟨E.V, σ⟩ ⇓ ⟨{ discriminant: i, payload: () }, σ⟩

[E-Enum-Tuple-Variant]
⟨e₁, σ⟩ ⇓ ⟨v₁, σ₁⟩    ...    ⟨eₙ, σₙ₋₁⟩ ⇓ ⟨vₙ, σₙ⟩
variant V has index i in enum E
----------------------------------------------------------
⟨E.V(e₁, ..., eₙ), σ⟩ ⇓ ⟨{ discriminant: i, payload: (v₁, ..., vₙ) }, σₙ⟩

[E-Enum-Struct-Variant]
⟨e₁, σ⟩ ⇓ ⟨v₁, σ₁⟩    ...    ⟨eₙ, σₙ₋₁⟩ ⇓ ⟨vₙ, σₙ⟩
variant V has index i in enum E
----------------------------------------------------------
⟨E.V { f₁: e₁, ..., fₙ: eₙ }, σ⟩ ⇓ ⟨{ discriminant: i, payload: { f₁: v₁, ..., fₙ: vₙ } }, σₙ⟩
```

**Evaluation order:** Payload expressions evaluate left-to-right.


CITE: Part II §2.3.1 — Enums (variant declarations, discriminant representation).

### 5.2.8 Array Literals

Array literals create fixed-size array values.

#### 5.2.8.1 Syntax

Array literal syntax (`ArrayExpr` and `ExprList`) is defined in Appendix A.4.

**Two forms:**

1. **Element list:** `[expr1, expr2, ..., exprN]` — explicit elements
2. **Repeat syntax:** `[value; count]` — repeat value count times

**Examples:**

```cursive
let numbers = [1, 2, 3, 4, 5]         // Type: [i32; 5]
let zeros = [0; 100]                   // Type: [i32; 100]
let matrix = [[0; 3]; 3]               // Type: [[i32; 3]; 3]
```

#### 5.2.8.2 Typing

```
[T-Array-Literal]
Γ ⊢ e₁ : τ    ...    Γ ⊢ eₙ : τ    (all elements same type)
--------------------------------
Γ ⊢ [e₁, ..., eₙ] : [τ; n]

[T-Array-Repeat]
Γ ⊢ e : τ
τ : Copy                    (element type must be Copy)
n ∈ ℕ⁺                      (count must be positive compile-time constant)
--------------------------------
Γ ⊢ [e; n] : [τ; n]
```

**Type unification:**

All elements in an array literal must have the same type. The type is inferred from the elements or from context.

**Repeat form requirements:**

- Element type MUST implement Copy trait
- Count MUST be a compile-time constant (not a runtime variable)

**Type checking errors:**

```cursive
let mixed = [1, 2.0, 3]           // ERROR: cannot unify i32 and f64
let non_copy = [string.from("a"); 10]  // ERROR: string@Owned is not Copy

function runtime_array(n: usize) {
    let arr = [0; n]               // ERROR E3D08: n is not compile-time constant
}
```

#### 5.2.8.3 Evaluation

```
[E-Array-Literal]
⟨e₁, σ⟩ ⇓ ⟨v₁, σ₁⟩    ...    ⟨eₙ, σₙ₋₁⟩ ⇓ ⟨vₙ, σₙ⟩
------------------------------------------------------
⟨[e₁, ..., eₙ], σ⟩ ⇓ ⟨[v₁, ..., vₙ], σₙ⟩

[E-Array-Repeat]
⟨e, σ⟩ ⇓ ⟨v, σ'⟩
v : Copy
------------------------------------
⟨[e; n], σ⟩ ⇓ ⟨[v, v, ..., v], σ'⟩
                          (n copies)
```

**Evaluation order:** Elements evaluate left-to-right.


CITE: Part II §2.4.1 — Arrays (array types, Copy requirement for repeat).

### 5.2.9 Tuple Literals

Tuple literals create tuple values with heterogeneous element types.

#### 5.2.9.1 Syntax

Tuple literal syntax (`TupleExpr`) is provided in Appendix A.4.

**Requirement:** Tuples must have at least 2 elements. Single-element tuples `(expr,)` are disallowed.

**Examples:**

```cursive
let pair = (42, true)                    // Type: (i32, bool)
let triple = (1.0, 'x', "text")          // Type: (f64, char, string)
let nested = ((1, 2), (3, 4))            // Type: ((i32, i32), (i32, i32))
```

#### 5.2.9.2 Typing

```
[T-Tuple-Ctor]
Γ ⊢ e₁ : τ₁    ...    Γ ⊢ eₙ : τₙ
n ≥ 2
-------------------------------------
Γ ⊢ (e₁, ..., eₙ) : (τ₁, ..., τₙ)
```

Tuple types preserve the heterogeneous types of all elements.

**Type checking errors:**

```cursive
let single = (42,)     // ERROR: tuples require at least 2 elements
let empty = ()         // OK: this is unit, not a tuple
```

#### 5.2.9.3 Evaluation

```
[E-Tuple]
⟨e₁, σ⟩ ⇓ ⟨v₁, σ₁⟩    ...    ⟨eₙ, σₙ₋₁⟩ ⇓ ⟨vₙ, σₙ⟩
------------------------------------------------------
⟨(e₁, ..., eₙ), σ⟩ ⇓ ⟨(v₁, ..., vₙ), σₙ⟩
```

**Evaluation order:** Tuple elements evaluate left-to-right.


CITE: Part II §2.2.1 — Tuples (tuple types, minimum arity).

---

**Definition 5.10 (Postfix Expressions):** Postfix expressions apply postfix operators to primary expressions, including function calls, procedure calls, field access, tuple projection, indexing, and slicing.

## 5.3 Postfix Expressions

Postfix expressions have the highest precedence (Level 1) and associate left-to-right.

### 5.3.1 Function Calls

Function call expressions invoke procedures or functions with argument expressions.

#### 5.3.1.1 Syntax

Function call syntax (`CallExpr` and `ExprList`) is defined in Appendix A.4.

**Examples:**

```cursive
let result = add(10, 20)
let value = compute()
let nested = outer(inner(x), y)
```

#### 5.3.1.2 Typing

```
[T-Function-Call]
Γ ⊢ f : (τ₁, ..., τₙ) → τᵣ ! ε_callee
Γ ⊢ e₁ : τ₁    ...    Γ ⊢ eₙ : τₙ
------------------------------------------------
Γ ⊢ f(e₁, ..., eₙ) : τᵣ ! ε_callee
```

**Arity checking:**

The number of arguments MUST match the number of parameters.

```
[Call-Arity-Mismatch]
Γ ⊢ f : (τ₁, ..., τₙ) → τᵣ ! ε
call provides m arguments where m ≠ n
------------------------------------------------
ERROR E4002: Arity mismatch (expected n, got m)
```

**Argument type checking:**

Each argument expression must have a type compatible with the corresponding parameter type.


#### 5.3.1.3 Effect Checking

```
[Effect-Call-Check]
Γ ⊢ f : (τ₁, ..., τₙ) → τᵣ ! ε_callee
Γ ⊢ e₁ : τ₁ ! ε₁    ...    Γ ⊢ eₙ : τₙ ! εₙ
ε_total = ε₁ ∪ ... ∪ εₙ ∪ ε_callee
ε_caller ⊇ ε_total
-------------------------------------------------
Γ ⊢ f(e₁, ..., eₙ) : τᵣ ! ε_caller
```

**Effect requirement:** The calling context must provide all effects required by:

1. Argument evaluation (ε₁ ∪ ... ∪ εₙ)
2. Callee execution (ε_callee)

**Diagnostic E4004:**

```
[Effect-Unavailable]
ε_total ⊈ ε_caller
missing_effects = ε_total \ ε_caller
-------------------------------------------------
ERROR E4004: Effect(s) unavailable: {missing_effects}
```

**Message:** "Effect(s) unavailable in calling context: {list}"

**Fix:** "Add missing effects to procedure's 'uses' clause"

**Example:**

```cursive
procedure read_data(path: string): string
    uses io.read, alloc.heap
{
    // Implementation
}

procedure pure_compute() {
    let data = read_data("config.txt")  // ERROR E4004: io.read, alloc.heap not available
}

procedure allowed_compute()
    uses io.read, alloc.heap
{
    let data = read_data("config.txt")  // OK: effects available
}
```

#### 5.3.1.4 Evaluation

```
[E-Procedure-Call]
⟨f, σ⟩ ⇓ ⟨procedure_ref(body, env), σ'⟩
⟨a₁, σ'⟩ ⇓ ⟨v₁, σ₁⟩    ...    ⟨aₙ, σₙ₋₁⟩ ⇓ ⟨vₙ, σₙ⟩    (args evaluate left-to-right)
⟨body[params ↦ (v₁, ..., vₙ), env], σₙ⟩ ⇓ ⟨result, σᵣ⟩
----------------------------------------------------------
⟨f(a₁, ..., aₙ), σ⟩ ⇓ ⟨result, σᵣ⟩
```

**Evaluation order:**

1. Evaluate function expression `f` to a callable
2. Evaluate arguments `a₁, ..., aₙ` **left-to-right** to values `v₁, ..., vₙ`
3. Execute function body with parameters bound to argument values
4. Return result value


**Example demonstrating left-to-right order:**

```cursive
var counter = 0
procedure next(): i32 {
    counter += 1
    result counter
}

procedure process(a: i32, b: i32, c: i32) {
    println("{a}, {b}, {c}")
}

process(next(), next(), next())
// Arguments evaluate left-to-right: prints "1, 2, 3"
```

CITE: Part II §2.10 — Map Types (function types); §5.20.2 — Function Arguments (evaluation order guarantee).

#### 5.3.1.5 Effect Operation Calls

Effect operation calls invoke operations defined within effect declarations and use the `::` scope operator (e.g., `FileSystem::read(path)`).

**Syntax:**

```cursive
EffectPath "." Ident "(" ArgumentList? ")"
```

**Examples:**

```cursive
procedure foo()
    uses FileSystem
{
    let content = FileSystem::read("/tmp/file.txt")
    FileSystem::write("/tmp/output.txt", content)
}

procedure bar()
    uses Async<Response>
{
    let response = Async.await(future)
}
```

**Typing:**

```
[T-Effect-Op-Call]
effect E { op(x₁: τ₁, ..., xₙ: τₙ): τᵣ { body } }
Γ ⊢ E.op : (τ₁, ..., τₙ) → τᵣ ! {E}
Γ ⊢ e₁ : τ₁    ...    Γ ⊢ eₙ : τₙ
E ∈ ε_context  (E is available in context)
------------------------------------------------
Γ ⊢ E.op(e₁, ..., eₙ) : τᵣ ! {E}
```

**Effect availability:**

The effect must be available in the calling context:

```
[Effect-Op-Unavailable]
E ∉ ε_context
------------------------------------------------
ERROR E7E10: Effect E not available in context
```

**Fine-grained capabilities:**

When a procedure declares `uses E.op`, only that specific operation is available:

```cursive
procedure read_only()
    uses FileSystem.read
{
    FileSystem::read("/tmp/file.txt")  // ✅ OK
    FileSystem::write("/tmp/file.txt", "data")  // ❌ ERROR E7E09
}
```

**Evaluation:**

Effect operation calls are evaluated according to the current implementation:

1. **Default implementation** — Uses the implementation provided in the effect declaration
2. **Custom implementation** — Uses the implementation provided by the innermost enclosing `with` block

```
[E-Effect-Op-Default]
effect E { op(x: τ): τᵣ { default_body } }
no custom implementation in scope
⟨a, σ⟩ ⇓ ⟨v, σ'⟩
⟨default_body[x ↦ v], σ'⟩ ⇓ ⟨result, σᵣ⟩
------------------------------------------------
⟨E::op(a), σ⟩ ⇓ ⟨result, σᵣ⟩

[E-Effect-Op-Custom]
with E { op(x) => custom_body } { ... E::op(a) ... }
⟨a, σ⟩ ⇓ ⟨v, σ'⟩
⟨custom_body[x ↦ v], σ'⟩ ⇓ ⟨result, σᵣ⟩
------------------------------------------------
⟨E::op(a), σ⟩ ⇓ ⟨result, σᵣ⟩
```

CITE: Part VII §7.3 — Unified Effect System; Part II §2.10.4 — Effect Operation Types.

### 5.3.2 Type-Scope Procedure Calls

Type-scope procedure call expressions invoke procedures defined on or associated with a type.

#### 5.3.2.1 Syntax

The grammar (`ProcedureCallExpr`) appears in Appendix A.4.

**Examples:**

```cursive
let result = value::process()
let transformed = data::transform(arg1, arg2)
// chaining should use the pipeline operator, not ::
```

#### 5.3.2.2 Typing

```
[T-Procedure-Call]
Γ ⊢ receiver : T
T has procedure p : (self: perm T, τ₁, ..., τₙ) → τᵣ ! ε
Γ ⊢ a₁ : τ₁    ...    Γ ⊢ aₙ : τₙ
receiver provides permission perm
----------------------------------------------------------
Γ ⊢ receiver::p(a₁, ..., aₙ) : τᵣ ! ε
```

**Permission checking:**

The receiver must provide the permission required by the procedure's `self` parameter:

- Procedure declares `self: T` (imm) → receiver can be any permission
- Procedure declares `self: mut T` → receiver must be `mut` or `own`
- Procedure declares `self: own T` → receiver must be `own` (consumes receiver)


#### 5.3.2.3 Self Type

```
[T-Procedure-Self]
within procedure defined on type T
------------------------
Γ ⊢ Self : Type
Γ ⊢ Self ≡ T
```

Within procedure bodies, the `Self` type refers to the enclosing type.

**$ receiver expansion:**

```
[T-Dollar-Receiver]
procedure m($, ...) defined on type T
------------------------------------
$ expands to: self: T
    (permission inferred from usage)
```

The `$` notation is shorthand for `self` with inferred permission.


#### 5.3.2.4 Procedure Dispatch

```
[T-Procedure-Dispatch]
Γ ⊢ receiver : T
T has procedure p : (self: perm T, τ₁, ..., τₙ) → τᵣ ! ε
Γ ⊢ a₁ : τ₁    ...    Γ ⊢ aₙ : τₙ
receiver provides permission perm
----------------------------------------------------------
Γ ⊢ receiver::p(a₁, ..., aₙ) : τᵣ ! ε
```

**Dispatch resolution:**

Procedures are resolved statically based on the receiver's type. Cursive uses static dispatch (monomorphization), not dynamic dispatch.

**Permission checking:**

```
[Procedure-Permission-Violation]
procedure requires permission perm_required
receiver has permission perm_actual
perm_actual ⊉ perm_required
----------------------------------------
ERROR E4003: Permission violation
```

**Examples:**

```cursive
record Counter {
    value: i32

    procedure increment(self: mut Counter) {
        self.value += 1
    }

    procedure get(self: Counter): i32 {
        result self.value
    }
}

let c = Counter { value: 0 }
c::increment()    // ERROR E4003: increment requires mut, c is imm
c::get()          // OK: get accepts imm

let mut_c: mut Counter = get_counter()
mut_c::increment()  // OK: mut_c provides mut permission
```


CITE: Part IX — Functions and Procedures (procedure declarations); Part IV — Lexical Permission System.

### 5.3.3 Field Access

Field access expressions retrieve or reference fields of record or tuple-struct types.

#### 5.3.3.1 Syntax

Field access expressions follow the `FieldAccessExpr` production in Appendix A.4.

**Examples:**

```cursive
let x_coord = point.x
let name = user.name
let nested = rect.top_left.x
```

#### 5.3.3.2 Typing

```
[T-Record-Field-Access]
Γ ⊢ e : R
record R { ..., fᵢ: τᵢ, ... } declared
field fᵢ is visible in current context
----------------------------------------
Γ ⊢ e.fᵢ : τᵢ
```

**Requirements:**

- Expression must have record or tuple-struct type
- Field name must exist in the type
- Field must be visible (respects visibility modifiers)


#### 5.3.3.3 Visibility Checks

```
[Field-Visibility-Check]
field f of type T has visibility v
access from module M
T declared in module M'
-----------------------------------------
If M = M': OK (same module)
If M ≠ M' and v = public: OK
If M ≠ M' and v ∈ {internal, private}: ERROR
```

**Examples:**

```cursive
// Module A
record Data {
    public id: u64
    internal value: i32
    private secret: string
}

// Module A (same module)
let d = get_data()
let id = d.id        // OK: public
let val = d.value    // OK: internal, same module
let sec = d.secret   // OK: private, same module

// Module B (different module)
use A::Data
let d = get_data()
let id = d.id        // OK: public
let val = d.value    // ERROR: internal, cross-module
let sec = d.secret   // ERROR: private, cross-module
```

CITE: Part III §3.10 — Visibility and Access Control.

#### 5.3.3.4 Evaluation

```
[E-Record-Field-Access]
⟨e, σ⟩ ⇓ ⟨R { ..., fᵢ: v, ... }, σ'⟩
-------------------------------------------
⟨e.fᵢ, σ⟩ ⇓ ⟨v, σ'⟩
```

Field access evaluates the record expression, then extracts the field value.


#### 5.3.3.5 Lvalue Semantics

```
[Field-Access-Lvalue]
e is lvalue
e has type R with field f
-------------------------
e.f is lvalue
```

Field access produces an lvalue when the record expression is an lvalue:

```cursive
var point = Point { x: 0.0, y: 0.0 }
point.x = 10.0  // OK: point.x is lvalue (point is var)

let immutable_point = Point { x: 0.0, y: 0.0 }
immutable_point.x = 10.0  // ERROR E3D10: cannot assign through immutable binding
```

**Permission interaction:**

```cursive
let rec: mut Record = get_record()
rec.field = new_value  // OK: rec has mut permission

let imm_rec: Record = get_record()
imm_rec.field = value  // ERROR E4003: permission violation
```

CITE: §5.21 — Value Categories; Part II §2.2.2 — Records.

### 5.3.4 Tuple Projection

Tuple projection accesses elements of tuples by zero-based index.

#### 5.3.4.1 Syntax

Tuple projection syntax (`TupleProjectionExpr`) is described in Appendix A.4.

**Examples:**

```cursive
let pair = (10, 20)
let first = pair.0   // Type: i32, value: 10
let second = pair.1  // Type: i32, value: 20

let triple = (1.0, 'x', "text")
let x = triple.0  // Type: f64
let y = triple.1  // Type: char
let z = triple.2  // Type: string
```

#### 5.3.4.2 Typing

```
[T-Tuple-Projection]
Γ ⊢ e : (τ₁, ..., τₙ)
0 ≤ i < n
-------------------------------------
Γ ⊢ e.i : τᵢ₊₁    (0-indexed: .0 → τ₁, .1 → τ₂, etc.)
```

**Compile-time bounds checking:**

The index must be a literal integer within bounds.

```
[Tuple-Projection-OOB]
Γ ⊢ e : (τ₁, ..., τₙ)
i ≥ n
-------------------------------------
ERROR: Tuple index {i} out of bounds (tuple has {n} elements)
```


**Examples:**

```cursive
let pair = (10, 20)
let valid = pair.0   // OK
let invalid = pair.2 // ERROR: tuple has 2 elements, index 2 out of bounds
```

#### 5.3.4.3 Evaluation

```
[E-Tuple-Projection]
⟨e, σ⟩ ⇓ ⟨(v₁, ..., vₙ), σ'⟩
0 ≤ i < n
--------------------------------------
⟨e.i, σ⟩ ⇓ ⟨vᵢ₊₁, σ'⟩
```


**Lvalue semantics:**

```cursive
var t = (1, 2, 3)
t.0 = 10  // OK: t.0 is lvalue
t.1 = 20  // OK: t.1 is lvalue
```

CITE: Part II §2.2.1 — Tuples.

### 5.3.5 Array Indexing

Array indexing accesses elements of arrays or slices by runtime index.

#### 5.3.5.1 Syntax

Array indexing uses the `IndexExpr` production from Appendix A.4.

**Examples:**

```cursive
let arr = [1, 2, 3, 4, 5]
let first = arr[0]        // Type: i32
let third = arr[2]        // Type: i32
let matrix = [[1, 2], [3, 4]]
let element = matrix[1][0]  // Type: i32, value: 3
```

#### 5.3.5.2 Typing

```
[T-Array-Index]
Γ ⊢ arr : [τ; n]    (or Γ ⊢ arr : [τ] for slice)
Γ ⊢ idx : usize
--------------------------------
Γ ⊢ arr[idx] : τ
    (bounds checked at runtime)
```

**Requirements:**

- Array/slice expression must have type `[τ; n]` or `[τ]`
- Index expression must have type `usize`
- Bounds checked at runtime (not compile-time for dynamic indices)


#### 5.3.5.3 Evaluation

```
[E-Array-Index]
⟨arr, σ⟩ ⇓ ⟨[v₁, ..., vₙ], σ'⟩
⟨idx, σ'⟩ ⇓ ⟨i, σ''⟩
0 ≤ i < n
------------------------------------
⟨arr[idx], σ⟩ ⇓ ⟨vᵢ₊₁, σ''⟩    (0-indexed)

[E-Array-Index-OOB]
⟨arr, σ⟩ ⇓ ⟨[v₁, ..., vₙ], σ'⟩
⟨idx, σ'⟩ ⇓ ⟨i, σ''⟩
¬(0 ≤ i < n)
------------------------------------
⟨arr[idx], σ⟩ ⇓ panic("index out of bounds: index {i}, len {n}")
```

**Runtime bounds checking:**

Every array/slice access is bounds-checked at runtime. Out-of-bounds access triggers a panic.

**Diagnostic E4007:**

**Runtime panic:** "Index out of bounds: index {i}, len {len}"

**Trigger:** `i ≥ len` or `i < 0`


**Examples:**

```cursive
let arr = [1, 2, 3]
let x = arr[0]   // OK: 0 < 3
let y = arr[10]  // PANIC E4007: index 10 out of bounds, len 3
```

#### 5.3.5.4 Lvalue Semantics

```
[Index-Lvalue]
arr is lvalue
----------------------------
arr[i] is lvalue
```

Array indexing produces an lvalue, allowing assignment:

```cursive
var numbers = [1, 2, 3]
numbers[0] = 10  // OK: numbers[0] is lvalue

let immutable = [1, 2, 3]
immutable[0] = 10  // ERROR E3D10: immutable binding
```

**Permission requirement:**

```cursive
let arr: mut [i32; 3] = get_array()
arr[0] = 5  // OK: mut permission

let imm_arr: [i32; 3] = get_array()
imm_arr[0] = 5  // ERROR E4003: permission violation
```

### 5.3.6 Slice Creation

Slicing creates slice views from arrays or slices using range syntax.

#### 5.3.6.1 Syntax

Slice and range syntax (`SliceExpr`, `RangeExpr`) are defined in Appendix A.4.

**Five range forms:**

1. `arr[a..b]` — elements from `a` to `b-1` (exclusive)
2. `arr[a..=b]` — elements from `a` to `b` (inclusive)
3. `arr[..b]` — from start (0) to `b-1`
4. `arr[a..]` — from `a` to end
5. `arr[..]` — full array as slice

**Examples:**

```cursive
let arr = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
let slice1 = arr[0..5]    // [0, 1, 2, 3, 4]
let slice2 = arr[3..=7]   // [3, 4, 5, 6, 7]
let slice3 = arr[..3]     // [0, 1, 2]
let slice4 = arr[5..]     // [5, 6, 7, 8, 9]
let slice5 = arr[..]      // [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
```

#### 5.3.6.2 Typing

```
[T-Slice-From-Array]
Γ ⊢ arr : [τ; n]
--------------------------------
Γ ⊢ arr[..] : [τ]
    (coercion [τ; n] <: [τ])

[T-Slice-Range]
Γ ⊢ arr : [τ; n]    (or Γ ⊢ arr : [τ])
Γ ⊢ start : usize
Γ ⊢ end : usize
--------------------------------
Γ ⊢ arr[start..end] : [τ]
    (bounds checked at runtime)
```

**Result type:** Slicing always produces `[τ]` (slice type).


#### 5.3.6.3 Evaluation

```
[E-Slice-Range]
⟨arr, σ⟩ ⇓ ⟨[v₁, ..., vₙ], σ'⟩
⟨start, σ'⟩ ⇓ ⟨a, σ''⟩
⟨end, σ''⟩ ⇓ ⟨b, σ'''⟩
0 ≤ a ≤ b ≤ n
ptr_base = address_of(arr)
------------------------------------------------
⟨arr[start..end], σ⟩ ⇓ ⟨{ ptr: ptr_base + a, len: b - a }, σ'''⟩

[E-Slice-Range-OOB]
⟨arr, σ⟩ ⇓ ⟨[v₁, ..., vₙ], σ'⟩
⟨start, σ'⟩ ⇓ ⟨a, σ''⟩
⟨end, σ''⟩ ⇓ ⟨b, σ'''⟩
¬(0 ≤ a ≤ b ≤ n)
------------------------------------------------
⟨arr[start..end], σ⟩ ⇓ panic("slice index out of bounds: start {a}, end {b}, len {n}")
```

**Bounds checking:**

- Start must be ≥ 0
- End must be ≤ array length
- Start must be ≤ end


**Examples:**

```cursive
let arr = [1, 2, 3, 4, 5]
let valid = arr[1..4]   // OK: [2, 3, 4]
let invalid = arr[3..2] // PANIC: start > end
let oob = arr[0..10]    // PANIC: end > len
```

CITE: Part II §2.4.2 — Slices; §5.5.8 — Range Operators.

### 5.3.7 Error Propagation Operator (?)

#### 5.3.7.1 Overview

The error propagation operator `?` provides syntactic sugar for early return from functions when encountering error values in `Result<T, E>` computations.

**Purpose**: Reduce boilerplate in error handling by automatically propagating errors up the call stack.

#### 5.3.7.2 Syntax

```
PostfixExpr ::= ...
              | PostfixExpr "?"
```

**Grammar**: The `?` operator is a postfix operator with precedence Level 14 (same as field access and method calls).

#### 5.3.7.3 Type Rules

```
[Try-Ok]
Γ ⊢ e : Result<T, E>
Γ ⊢ return_type : Result<U, E>  // Same error type E
-------------------------------------------------
Γ ⊢ e? : T
```

**Requirements**:
1. The operand `e` must have type `Result<T, E>` for some types `T` and `E`
2. The enclosing function must return `Result<U, E'>` where `E'` is a supertype of `E`
3. The `?` operator has type `T` (unwraps the success value)

**Error E9F-5301**: Cannot use `?` operator on non-Result type
**Error E9F-5302**: Cannot use `?` operator in function that doesn't return Result
**Error E9F-5303**: Result error types are incompatible (cannot convert `E` to `E'`)

#### 5.3.7.4 Desugaring

The `?` operator desugars to a `match` expression:

**Source**:
```cursive
let value = computation()?
```

**Desugars to**:
```cursive
let value = computation() match {
    Result::Ok(val) => val,
    Result::Err(err) => { result Result::Err(err) }
}
```

**Full example desugaring**:
```cursive
// Before
function process(path: string): Result<i32, string> {
    let contents = read_file(path)?
    let parsed = parse_int(contents)?
    result Result::Ok(parsed * 2)
}

// After desugaring
function process(path: string): Result<i32, string> {
    let contents = read_file(path) match {
        Result::Ok(val) => val,
        Result::Err(err) => { result Result::Err(err) }
    }
    let parsed = parse_int(contents) match {
        Result::Ok(val) => val,
        Result::Err(err) => { result Result::Err(err) }
    }
    result Result::Ok(parsed * 2)
}
```

#### 5.3.7.5 Static Semantics

**Well-formedness**:

```
[WF-Try]
Γ ⊢ e : Result<T, E>
Γ(return_type) = Result<U, E'>
E <: E'
-------------------------------------------------
Γ ⊢ e? : T  well-formed
```

**Effect propagation**:
The `?` operator propagates the effects of its operand plus any effects from the early return:

```
[Try-Effects]
Γ ⊢ e : Result<T, E> ! ε₁
effects(early_return) = ε₂
-------------------------------------------------
Γ ⊢ e? : T ! (ε₁ ∪ ε₂)
```

#### 5.3.7.6 Dynamic Semantics

**Evaluation**: The `?` operator evaluates its operand, then:
- If the result is `Result::Ok(v)`, evaluates to `v`
- If the result is `Result::Err(e)`, returns `Result::Err(e)` from the enclosing function

```
[Eval-Try-Ok]
⟨e, σ⟩ ⇓ ⟨Result::Ok(v), σ'⟩
-------------------------------------------------
⟨e?, σ⟩ ⇓ ⟨v, σ'⟩

[Eval-Try-Err]
⟨e, σ⟩ ⇓ ⟨Result::Err(err), σ'⟩
-------------------------------------------------
⟨e?, σ⟩ ⇓ ⟨return Result::Err(err), σ'⟩
```

#### 5.3.7.7 Examples

**Basic usage**:
```cursive
function read_and_parse(filename: string): Result<i32, string>
    uses fs.read, alloc.heap
{
    let contents = fs::read_file(filename)?  // Early return if Err
    let value = string::parse_int(contents)? // Early return if Err
    result Result::Ok(value)
}
```

**Chaining operations**:
```cursive
function process_data(): Result<Data, Error> {
    let raw = fetch_data()?
    let validated = validate(raw)?
    let transformed = transform(validated)?
    result Result::Ok(transformed)
}
```

**Error type compatibility**:
```cursive
// OK: Error types match
function foo(): Result<i32, string> {
    let x = bar()?  // bar returns Result<i32, string>
    result Result::Ok(x + 1)
}

// ERROR E9F-5303: Incompatible error types
function baz(): Result<i32, IoError> {
    let x = bar()?  // ERROR: bar returns Result<i32, string>
                    // Cannot convert string to IoError
    result Result::Ok(x)
}
```

#### 5.3.7.8 Limitations

1. **Result-only**: The `?` operator works only with `Result<T, E>`, not with `Option<T>` or other types
2. **Function-scope only**: Cannot be used at module or block scope outside a function
3. **No type conversion**: Error types must match exactly (or have explicit subtype relationship)

CITE: Part II §2.2.3 — Enum Types (Result definition); Part IX §9.5 — Return Statements.

---

**Definition 5.11 (Unary Expressions):** Unary expressions apply prefix operators to operands.

## 5.4 Unary Expressions

Unary expressions have precedence Level 2 and associate right-to-left.

### 5.4.1 Logical NOT

#### 5.4.1.1 Syntax

Logical NOT expressions follow the `NotExpr` production in Appendix A.4.

Precedence: Level 2

**Examples:**

```cursive
let negated = !true
let result = !is_valid
let compound = !(x > 0 && y < 10)
```

#### 5.4.1.2 Typing

```
[T-Not]
Γ ⊢ e : bool
----------------
Γ ⊢ !e : bool
```

**Type error:**

```cursive
let invalid = !42  // ERROR E4001: expected bool, found i32
```

#### 5.4.1.3 Evaluation

```
[E-Not-True]
⟨e, σ⟩ ⇓ ⟨true, σ'⟩
------------------------
⟨!e, σ⟩ ⇓ ⟨false, σ'⟩

[E-Not-False]
⟨e, σ⟩ ⇓ ⟨false, σ'⟩
------------------------
⟨!e, σ⟩ ⇓ ⟨true, σ'⟩
```

### 5.4.2 Arithmetic Negation

#### 5.4.2.1 Syntax

Arithmetic negation expressions correspond to the `NegExpr` production in Appendix A.4.

Precedence: Level 2

**Examples:**

```cursive
let neg_int = -42
let neg_float = -3.14
let neg_expr = -(x + y)
```

#### 5.4.2.2 Typing

```
[T-Neg]
Γ ⊢ e : τ
τ ∈ NumericTypes
----------------
Γ ⊢ -e : τ
```

**Type error:**

```cursive
let invalid = -true  // ERROR E4001: expected numeric, found bool
```

#### 5.4.2.3 Evaluation

```
[E-Neg]
⟨e, σ⟩ ⇓ ⟨n, σ'⟩
------------------------
⟨-e, σ⟩ ⇓ ⟨-n, σ'⟩

[E-Neg-Overflow]
⟨e, σ⟩ ⇓ ⟨τ::MIN, σ'⟩    (minimum value of signed type)
------------------------------------------------
⟨-e, σ⟩ ⇓ panic("arithmetic overflow") in debug
            ⇓ ⟨τ::MIN, σ'⟩ in release (wraps to same value)
```

**Overflow case:**

```cursive
let x: i8 = -128  // i8::MIN
let y = -x
// Debug: panic
// Release: y = -128 (wraps)
```

### 5.4.3 Pointer Dereference

#### 5.4.3.1 Syntax

Pointer dereference expressions use the `DerefExpr` production defined in Appendix A.4.

Precedence: Level 2

#### 5.4.3.2 Typing

```
[T-Ptr-Deref-Raw]
Γ ⊢ ptr : *τ    (or *mut τ)
------------------------------------
Γ ⊢ *ptr : τ
    uses unsafe.ptr
```

**Requirements:**

- Operand must be raw pointer
- Requires `unsafe.ptr` effect
- Must be in unsafe context


#### 5.4.3.3 Evaluation

```
[E-Raw-Ptr-Deref]
⟨ptr, σ⟩ ⇓ ⟨address(ℓ), σ'⟩
σ'(ℓ) defined
------------------------------------
⟨*ptr, σ⟩ ⇓ ⟨σ'(ℓ), σ'⟩

[E-Raw-Ptr-Deref-Invalid]
⟨ptr, σ⟩ ⇓ ⟨address(ℓ), σ'⟩
σ'(ℓ) undefined
------------------------------------
⟨*ptr, σ⟩ has undefined behavior
```


CITE: Part II §2.6 — Raw Pointers; Part XII — Unsafe Operations.

### 5.4.5 Move Expression

#### 5.4.5.1 Syntax

Move expressions correspond to the `MoveExpr` production in Appendix A.4.

Precedence: Level 2

#### 5.4.5.2 Typing

```
[T-Move]
Γ ⊢ e : τ
e has own permission
----------------------------
Γ ⊢ move e : τ
e invalidated after move
```


#### 5.4.5.3 Evaluation

```
[E-Move]
⟨e, σ⟩ ⇓ ⟨v, σ'⟩
------------------------------------
⟨move e, σ⟩ ⇓ ⟨v, σ'⟩
if e is variable x: x marked as moved
```

#### 5.4.5.4 Diagnostic E4006

**ERROR E4006: Use of moved value**

**Trigger:** Using variable after move

**Message:** "Use of moved value '{name}'"

**Note:** "Value was moved at {location}"

**Fix:** "Use value before move"

**Examples:**

```cursive
let s = string.from("data")
let moved = move s
// let x = s  // ERROR E4006

// Implicit move:
procedure take(data: own string) { }
let s2 = string.from("data")
take(s2)
// let y = s2  // ERROR E4006
```

---

**Definition 5.12 (Binary Expressions):** Binary expressions apply infix operators to two operands.

## 5.5 Binary Expressions

### 5.5.1 Arithmetic Operators

#### 5.5.1.1 Syntax

The additive, multiplicative, and remainder operators use the `AddExpr`, `SubExpr`, `MulExpr`, `DivExpr`, and `ModExpr` productions enumerated in Appendix A.4.

Precedence: Level 4 (`*`, `/`, `%`), Level 5 (`+`, `-`)

Associativity: Left

#### 5.5.1.2 Typing

```
[T-Add]
Γ ⊢ e₁ : τ    Γ ⊢ e₂ : τ    τ ∈ NumericTypes
--------------------------------------------
Γ ⊢ e₁ + e₂ : τ

[T-Sub]
Γ ⊢ e₁ : τ    Γ ⊢ e₂ : τ    τ ∈ NumericTypes
--------------------------------------------
Γ ⊢ e₁ - e₂ : τ

[T-Mul]
Γ ⊢ e₁ : τ    Γ ⊢ e₂ : τ    τ ∈ NumericTypes
--------------------------------------------
Γ ⊢ e₁ * e₂ : τ

[T-Div]
Γ ⊢ e₁ : τ    Γ ⊢ e₂ : τ    τ ∈ NumericTypes
--------------------------------------------
Γ ⊢ e₁ / e₂ : τ

[T-Mod]
Γ ⊢ e₁ : τ    Γ ⊢ e₂ : τ    τ ∈ IntegerTypes
--------------------------------------------
Γ ⊢ e₁ % e₂ : τ
```

**Type errors:**

```cursive
let invalid = 10 + 20.0  // ERROR: cannot mix i32 and f64
```

#### 5.5.1.3 Integer Arithmetic

```
[E-Add-Int]
⟨e₁, σ⟩ ⇓ ⟨n₁, σ'⟩    ⟨e₂, σ'⟩ ⇓ ⟨n₂, σ''⟩
------------------------------------------------
⟨e₁ + e₂, σ⟩ ⇓ ⟨n₁ ⊕ n₂, σ''⟩
where ⊕ = checked (debug) or wrapping (release)

[E-Add-Overflow]
n₁ ⊕ n₂ ∉ ⟦τ⟧
------------------------------------------------
Debug: ⟨e₁ + e₂, σ⟩ ⇓ panic("arithmetic overflow")
Release: ⟨e₁ + e₂, σ⟩ ⇓ ⟨(n₁ + n₂) mod 2^bits, σ''⟩
```


**Examples:**

```cursive
let x: u8 = 255
let y = x + 1
// Debug: panic
// Release: y = 0
```

#### 5.5.1.4 Float Arithmetic

```
[E-Add-Float]
⟨e₁, σ⟩ ⇓ ⟨f₁, σ'⟩    ⟨e₂, σ'⟩ ⇓ ⟨f₂, σ''⟩
------------------------------------------------
⟨e₁ + e₂, σ⟩ ⇓ ⟨f₁ +_IEEE754 f₂, σ''⟩
```

**IEEE 754 rules:**

```
x + ∞ = ∞
∞ + ∞ = ∞
∞ - ∞ = NaN
NaN + x = NaN
```


#### 5.5.1.5 Division by Zero

```
[E-Div-Zero-Int]
⟨e₂, σ'⟩ ⇓ ⟨0, σ''⟩
------------------------------------------------
⟨e₁ / e₂, σ⟩ ⇓ panic("division by zero")
```

**Diagnostic E4008:**

**Runtime panic:** "Division by zero"

**Examples:**

```cursive
let x = 10 / 0     // PANIC E4008
let f = 1.0 / 0.0  // OK: +∞ (IEEE 754)
```

#### 5.5.1.6 Overflow Control

**Attribute:** `[[overflow_checks(enabled)]]`

Controls per-function overflow behavior:

```cursive
[[overflow_checks(true)]]   // Always check
[[overflow_checks(false)]]  // Never check

// Default:
// Debug: checks enabled
// Release: checks disabled
```

CITE: Foundations §1.3.3 — Attributes.

### 5.5.2 Power Operator

The power operator performs exponentiation.

#### 5.5.2.1 Syntax

Exponentiation expressions follow the `PowerExpr` production in Appendix A.4.

Precedence: Level 3 (higher than multiplication)

Associativity: **Right**

**Examples:**

```cursive
let squared = 5 ** 2
let cubed = 2 ** 3
let tower = 2 ** 3 ** 2  // Right assoc: 2 ** (3 ** 2) = 512
```

#### 5.5.2.2 Typing

```
[T-Pow]
Γ ⊢ e₁ : τ    Γ ⊢ e₂ : τ    τ ∈ NumericTypes
--------------------------------------------
Γ ⊢ e₁ ** e₂ : τ
```

#### 5.5.2.3 Evaluation

```
[E-Pow]
⟨e₁, σ⟩ ⇓ ⟨n₁, σ'⟩    ⟨e₂, σ'⟩ ⇓ ⟨n₂, σ''⟩
------------------------------------------------
⟨e₁ ** e₂, σ⟩ ⇓ ⟨n₁^n₂, σ''⟩
```

**Overflow:** Follows integer overflow rules (debug panic, release wrap).

#### 5.5.2.4 Associativity

**Right associative:**

```cursive
2 ** 3 ** 2    // 2 ** (3 ** 2) = 2 ** 9 = 512
               // NOT (2 ** 3) ** 2 = 8 ** 2 = 64

2 * 3 ** 4     // 2 * (3 ** 4) = 2 * 81 = 162
```

### 5.5.3 Bitwise Operators

#### 5.5.3.1 Syntax

Bitwise operators (`&`, `^`, `|`) correspond to the `BitwiseAndExpr`, `BitwiseXorExpr`, and `BitwiseOrExpr` productions in Appendix A.4.

Precedence: Level 7 (`&`), Level 8 (`^`), Level 9 (`|`)

Associativity: Left

#### 5.5.3.2 Typing

```
[T-Bitwise-And]
Γ ⊢ e₁ : τ    Γ ⊢ e₂ : τ    τ ∈ IntegerTypes
--------------------------------------------
Γ ⊢ e₁ & e₂ : τ

[T-Bitwise-Or]
Γ ⊢ e₁ : τ    Γ ⊢ e₂ : τ    τ ∈ IntegerTypes
--------------------------------------------
Γ ⊢ e₁ | e₂ : τ

[T-Bitwise-Xor]
Γ ⊢ e₁ : τ    Γ ⊢ e₂ : τ    τ ∈ IntegerTypes
--------------------------------------------
Γ ⊢ e₁ ^ e₂ : τ
```

#### 5.5.3.3 Evaluation

```
[E-Bitwise-And]
⟨e₁, σ⟩ ⇓ ⟨n₁, σ'⟩    ⟨e₂, σ'⟩ ⇓ ⟨n₂, σ''⟩
------------------------------------------------
⟨e₁ & e₂, σ⟩ ⇓ ⟨n₁ bitwise-and n₂, σ''⟩

[E-Bitwise-Or]
⟨e₁, σ⟩ ⇓ ⟨n₁, σ'⟩    ⟨e₂, σ'⟩ ⇓ ⟨n₂, σ''⟩
------------------------------------------------
⟨e₁ | e₂, σ⟩ ⇓ ⟨n₁ bitwise-or n₂, σ''⟩

[E-Bitwise-Xor]
⟨e₁, σ⟩ ⇓ ⟨n₁, σ'⟩    ⟨e₂, σ'⟩ ⇓ ⟨n₂, σ''⟩
------------------------------------------------
⟨e₁ ^ e₂, σ⟩ ⇓ ⟨n₁ bitwise-xor n₂, σ''⟩
```

### 5.5.4 Shift Operators

#### 5.5.4.1 Syntax

Shift expressions (`<<`, `>>`) are captured by the `ShiftLeftExpr` and `ShiftRightExpr` productions in Appendix A.4.

Precedence: Level 6

Associativity: Left

#### 5.5.4.2 Typing

```
[T-Shift-Left]
Γ ⊢ e₁ : τ    τ ∈ IntegerTypes
Γ ⊢ e₂ : usize
--------------------------------------------
Γ ⊢ e₁ << e₂ : τ

[T-Shift-Right]
Γ ⊢ e₁ : τ    τ ∈ IntegerTypes
Γ ⊢ e₂ : usize
--------------------------------------------
Γ ⊢ e₁ >> e₂ : τ
```

#### 5.5.4.3 Evaluation

```
[E-Shift]
⟨e₁, σ⟩ ⇓ ⟨n, σ'⟩    ⟨e₂, σ'⟩ ⇓ ⟨amount, σ''⟩
amount < bitwidth(τ)
------------------------------------------------
⟨e₁ << e₂, σ⟩ ⇓ ⟨n << amount, σ''⟩

[E-Shift-Overflow]
amount ≥ bitwidth(τ)
------------------------------------------------
⟨e₁ << e₂, σ⟩ ⇓ panic("shift amount exceeds bitwidth")
```

### 5.5.5 Comparison Operators

#### 5.5.5.1 Syntax

Relational comparisons (`<`, `<=`, `>`, `>=`) use the `CmpExpr` production in Appendix A.4.

Precedence: Level 10

Associativity: Left

#### 5.5.5.2 Typing

```
[T-Cmp]
Γ ⊢ e₁ : τ    Γ ⊢ e₂ : τ    τ ∈ {NumericTypes, char}
--------------------------------------------
Γ ⊢ e₁ < e₂ : bool
(similarly for <=, >, >=)
```

#### 5.5.5.3 Evaluation

```
[E-Cmp]
⟨e₁, σ⟩ ⇓ ⟨v₁, σ'⟩    ⟨e₂, σ'⟩ ⇓ ⟨v₂, σ''⟩
------------------------------------------------
⟨e₁ < e₂, σ⟩ ⇓ ⟨v₁ < v₂, σ''⟩
```

**NaN behavior:**

```
NaN < x = false
NaN <= x = false
NaN > x = false
NaN >= x = false
```

### 5.5.6 Equality Operators

#### 5.5.6.1 Syntax

Equality comparisons (`==`, `!=`) are described by the `EqExpr` production in Appendix A.4.

Precedence: Level 10

#### 5.5.6.2 Typing

```
[T-Eq]
Γ ⊢ e₁ : τ    Γ ⊢ e₂ : τ
--------------------------------------------
Γ ⊢ e₁ == e₂ : bool
Γ ⊢ e₁ != e₂ : bool
```

#### 5.5.6.3 Evaluation

```
[E-Eq]
⟨e₁, σ⟩ ⇓ ⟨v₁, σ'⟩    ⟨e₂, σ'⟩ ⇓ ⟨v₂, σ''⟩
------------------------------------------------
⟨e₁ == e₂, σ⟩ ⇓ ⟨v₁ = v₂, σ''⟩
```

**NaN special case:**

```
NaN == NaN = false
NaN != NaN = true
```

### 5.5.7 Logical Operators

#### 5.5.7.1 Syntax

Logical AND/OR expressions correspond to the `LogicalAndExpr` and `LogicalOrExpr` productions in Appendix A.4.

Precedence: Level 11 (`&&`), Level 12 (`||`)

Associativity: Left

#### 5.5.7.2 Typing

```
[T-And]
Γ ⊢ e₁ : bool    Γ ⊢ e₂ : bool
--------------------------------------------
Γ ⊢ e₁ && e₂ : bool

[T-Or]
Γ ⊢ e₁ : bool    Γ ⊢ e₂ : bool
--------------------------------------------
Γ ⊢ e₁ || e₂ : bool
```

#### 5.5.7.3 Short-Circuit Evaluation

```
[E-And-True]
⟨e₁, σ⟩ ⇓ ⟨true, σ'⟩    ⟨e₂, σ'⟩ ⇓ ⟨v₂, σ''⟩
----------------------------------------------
⟨e₁ && e₂, σ⟩ ⇓ ⟨v₂, σ''⟩

[E-And-False]
⟨e₁, σ⟩ ⇓ ⟨false, σ'⟩
----------------------------------------------
⟨e₁ && e₂, σ⟩ ⇓ ⟨false, σ'⟩    (e₂ NOT evaluated)

[E-Or-True]
⟨e₁, σ⟩ ⇓ ⟨true, σ'⟩
----------------------------------------------
⟨e₁ || e₂, σ⟩ ⇓ ⟨true, σ'⟩    (e₂ NOT evaluated)

[E-Or-False]
⟨e₁, σ⟩ ⇓ ⟨false, σ'⟩    ⟨e₂, σ'⟩ ⇓ ⟨v₂, σ''⟩
----------------------------------------------
⟨e₁ || e₂, σ⟩ ⇓ ⟨v₂, σ''⟩
```

**Short-circuit property:**

- `&&`: If LHS is `false`, RHS is NOT evaluated
- `||`: If LHS is `true`, RHS is NOT evaluated

**This is the ONLY exception to strict left-to-right evaluation.**


**Examples:**

```cursive
let x = false && expensive_check()  // expensive_check() NOT called
let y = true || expensive_check()   // expensive_check() NOT called
```

CITE: §5.20.4 — Short-Circuit Exception.

### 5.5.8 Range Operators

Range operators create range values for iteration and slicing.

#### 5.5.8.1 Grammar Reference

The canonical grammar for range literals is defined in Appendix A.3 (`RangeExpr`). For convenience the six literal forms are summarised in §5.5.8.3. Precedence Level 13; associativity is left-to-right.

#### 5.5.8.2 Syntax

```cursive
let r1 = 0..10                  // Range<i32>
let r2 = 0..=10                 // RangeInclusive<i32>
let r3 = 5..                    // RangeFrom<i32>
let r4 = ..5                    // RangeTo<i32>
let r5 = ..=5                   // RangeToInclusive<i32>
let r6: RangeFull<usize> = ..   // RangeFull requires contextual type
```

#### 5.5.8.3 Typing

Each literal form maps to a range type defined in Part II §2.4.0.4. All bounds, when present, MUST share the same numeric type `τ`.

| Literal | Typing Rule |
|---------|-------------|
| `e₁..e₂` | `Γ ⊢ e₁ : τ`, `Γ ⊢ e₂ : τ`, `τ ∈ NumericTypes` ⇒ `Γ ⊢ e₁..e₂ : Range<τ>` |
| `e₁..=e₂` | Same premises ⇒ `Γ ⊢ e₁..=e₂ : RangeInclusive<τ>` |
| `e₁..` | `Γ ⊢ e₁ : τ`, `τ ∈ NumericTypes` ⇒ `Γ ⊢ e₁.. : RangeFrom<τ>` |
| `..e₂` | `Γ ⊢ e₂ : τ`, `τ ∈ NumericTypes` ⇒ `Γ ⊢ ..e₂ : RangeTo<τ>` |
| `..=e₂` | `Γ ⊢ e₂ : τ`, `τ ∈ NumericTypes` ⇒ `Γ ⊢ ..=e₂ : RangeToInclusive<τ>` |
| `..` | Requires contextual type `RangeFull<τ>`; the surrounding expression must provide `τ`. Absent context, an explicit type annotation (e.g., `let r: RangeFull<usize> = ..;`) is REQUIRED. |

#### 5.5.8.4 Evaluation

Evaluation produces range values whose `start`/`end` fields are represented using `Option<τ>`; missing bounds map to `Option::None`.

```
[E-Range-Bounded]
⟨e₁, σ⟩ ⇓ ⟨v₁, σ'⟩    ⟨e₂, σ'⟩ ⇓ ⟨v₂, σ''⟩
--------------------------------------------------------
⟨e₁..e₂, σ⟩ ⇓ ⟨Range { start: Option::Some(v₁),
                        end:   Option::Some(v₂),
                        inclusive: false }, σ''⟩

[E-Range-Inclusive]
⟨e₁, σ⟩ ⇓ ⟨v₁, σ'⟩    ⟨e₂, σ'⟩ ⇓ ⟨v₂, σ''⟩
--------------------------------------------------------
⟨e₁..=e₂, σ⟩ ⇓ ⟨RangeInclusive { start: v₁, end: v₂ }, σ''⟩

[E-Range-From]
⟨e₁, σ⟩ ⇓ ⟨v₁, σ'⟩
--------------------------------------------------------
⟨e₁.., σ⟩ ⇓ ⟨RangeFrom { start: v₁ }, σ'⟩

[E-Range-To]
⟨e₂, σ⟩ ⇓ ⟨v₂, σ'⟩
--------------------------------------------------------
⟨..e₂, σ⟩ ⇓ ⟨RangeTo { end: v₂ }, σ'⟩

[E-Range-To-Inclusive]
⟨e₂, σ⟩ ⇓ ⟨v₂, σ'⟩
--------------------------------------------------------
⟨..=e₂, σ⟩ ⇓ ⟨RangeToInclusive { end: v₂ }, σ'⟩

[E-Range-Full]
--------------------------------------------------------
⟨.., σ⟩ ⇓ ⟨RangeFull {}, σ⟩
```

#### 5.5.8.5 Usage

**In loops:**

```cursive
loop i: i32 in 0..10 {  // Iterate from 0 to 9
    process(i)
}

loop j: i32 in 0..=10 {  // Iterate from 0 to 10
    process(j)
}
```

**In slicing:**

```cursive
let arr = [1, 2, 3, 4, 5]
let slice = arr[1..4]    // Uses range 1..4 for slicing
```

**As first-class values:**

```cursive
let r: Range<i32> = 0..100
let filtered = r.filter(|x| -> x % 2 == 0)
```

CITE: §5.3.6 — Slice Creation; §5.9.3 — Iterator Loops.

### 5.5.9 Pipeline Operator

**REQUIRED EXPLICITNESS FEATURE:** Pipeline stages MUST have type annotations.

#### 5.5.9.1 Grammar

The concrete syntax for pipeline expressions is defined by `PipelineExpr` in Appendix A.4.

**Rule 5.5.9-1 (Pipeline Stage Annotation).** Each pipeline stage after `=>` MUST supply a `: Type` annotation for its result.

Precedence: Level 14

Associativity: Left

#### 5.5.9.2 Typing

```
[T-Pipeline]
Γ ⊢ e₁ : τ₁
Γ ⊢ e₂ : (τ₁) → τ₂ ! ε
τ₂ matches annotated type T
------------------------------------------------
Γ ⊢ e₁ => e₂: T : τ₂ ! ε
```

**Type checking:**

Each stage's output type must match the next stage's input type AND the explicit annotation.


**Example:**

```cursive
let result: string = input
    => validate: Result<Data, Error>
    => transform: Data
    => serialize: string
```

The type checker verifies:

- `input` type matches `validate` input
- `validate` returns `Result<Data, Error>` (matches annotation)
- `Result<Data, Error>` matches `transform` input
- `transform` returns `Data` (matches annotation)
- `Data` matches `serialize` input
- `serialize` returns `string` (matches annotation)

#### 5.5.9.3 Evaluation

```
[E-Pipeline]
⟨e₁, σ⟩ ⇓ ⟨v, σ'⟩
⟨e₂(v), σ'⟩ ⇓ ⟨result, σ''⟩
--------------------------------------
⟨e₁ => e₂, σ⟩ ⇓ ⟨result, σ''⟩
```

**Desugaring:** `e₁ => e₂` desugars to `e₂(e₁)` (function application).


**Evaluation order:**

Pipeline chains evaluate left-to-right:

```cursive
a => b => c  // Evaluates as: c(b(a))
             // Order: a first, then b(a), then c(b(a))
```

#### 5.5.9.4 Diagnostic E4403

**ERROR E4403: Missing type annotation on pipeline stage**

**Trigger:** Pipeline stage lacks `: Type` annotation.

**Message:** "Pipeline stage missing type annotation"

**Note:** "Each stage after '=>' must declare its result type with ': Type'"

**Fix:** "Add type annotation: => expression: Type"

**Error examples:**

```cursive
let invalid = input
    => validate      // ERROR E4403: missing type annotation
    => transform     // ERROR E4403: missing type annotation

// Correct:
let valid: Output = input
    => validate: Result<Data, Error>
    => transform: Data
    => finalize: Output
```

**Rationale:**

- Documents transformations explicitly at each stage
- Localizes type errors to specific pipeline stage
- LLMs see expected types without inference
- Improves error messages (compiler reports which stage failed)

---

**Definition 5.13 (Assignment Expressions):** Assignment expressions modify the value of lvalue expressions.

## 5.6 Assignment Expressions

Assignment expressions have the lowest precedence (Level 15) and associate right-to-left.

### 5.6.1 Simple Assignment

#### 5.6.1.1 Syntax

The grammar for simple assignments appears as `AssignExpr` in Appendix A.4.

Precedence: Level 15 (lowest)

Associativity: Right

**Examples:**

```cursive
var x = 5
x = 10         // Simple assignment

var counter = 0
counter = counter + 1
```

#### 5.6.1.2 Typing

```
[T-Assign]
Γ ⊢ target : τ
target is lvalue
target is var binding (not let)
Γ ⊢ expr : τ
--------------------------------------------
Γ ⊢ target = expr : ()
```

**Requirements:**

- Target must be an lvalue (memory location)
- Target must be a `var` binding (or mutable field/element)
- Expression type must match target type

**Cross-reference to E3D10:**

Attempting to assign to a `let` binding produces error E3D10 (defined in Part III §3.2.8).

```cursive
let x = 5
x = 10  // ERROR E3D10: Cannot assign to immutable binding 'x'
```

CITE: Part III §3.2.8 — Assignment to Immutable Binding.

#### 5.6.1.3 Evaluation

```
[E-Assign]
⟨expr, σ⟩ ⇓ ⟨v, σ'⟩    (evaluate RHS first)
target ↦ ℓ in environment
--------------------------------------------
⟨target = expr, σ⟩ ⇓ ⟨(), σ'[ℓ ↦ v]⟩
```

**Evaluation order:**

1. Evaluate right-hand side expression to a value
2. Store value at target's memory location
3. Result is unit `()`

**Example:**

```cursive
var x = 1
var y = 2
x = y = 3  // Right assoc: x = (y = 3)
           // y = 3 (returns ()), then x = () -- ERROR: type mismatch
           // Assignment is NOT chainable in practice
```

#### 5.6.1.4 Diagnostic Reference

**ERROR E3D10** (from Part III):

- **Trigger:** Assignment to `let` binding
- **Message:** "Cannot assign to immutable binding '{name}'"
- **Fix:** "Use 'var {name}' if reassignment needed"

CITE: Part III §3.2.8 — Assignment to Immutable Binding.

### 5.6.2 Compound Assignment

#### 5.6.2.1 Syntax

Compound assignments reuse the `AssignOp` alternatives listed in Appendix A.4.

**Ten compound assignment operators.**

Precedence: Level 15

Associativity: Right

**Examples:**

```cursive
var x = 10
x += 5   // x = 15
x *= 2   // x = 30
x -= 10  // x = 20
```

#### 5.6.2.2 Typing

```
[T-Compound-Assign]
Γ ⊢ target OP= expr  ≡  Γ ⊢ target = target OP expr
target is lvalue, var binding
Γ ⊢ target : τ
Γ ⊢ expr : τ
τ supports operator OP
--------------------------------------------
Γ ⊢ target OP= expr : ()
```

**Semantics:** `target OP= expr` is equivalent to `target = target OP expr`.

**Requirements:**

- Same as simple assignment
- Type must support the operator OP

#### 5.6.2.3 Evaluation

```
[E-Compound-Assign]
⟨target, σ⟩ ⇓ ⟨v_old, σ'⟩    (evaluate target for reading)
⟨expr, σ'⟩ ⇓ ⟨v_rhs, σ''⟩
v_result = v_old OP v_rhs
target ↦ ℓ
--------------------------------------------
⟨target OP= expr, σ⟩ ⇓ ⟨(), σ''[ℓ ↦ v_result]⟩
```

**Target evaluated once:**

Unlike the expanded form `target = target OP expr`, the compound form evaluates `target` only once. This matters for expressions with side effects:

```cursive
var arr = [0, 1, 2]
var i = 0
procedure next_index(): usize {
    i += 1
    result i - 1
}

arr[next_index()] += 10  // Calls next_index() once
// Equivalent to: arr[0] = arr[0] + 10, NOT arr[1] = arr[0] + 10
```

### 5.6.3 Assignment Properties

**Result type:** All assignment expressions have type `()` (unit).

**Not chainable:**

Because assignments return `()`, chaining assignments is not practical:

```cursive
var x = 0
var y = 0
x = y = 5  // y = 5 returns (), then x = () type error
```

**Lvalue requirement:**

The left-hand side must be an lvalue:

```cursive
10 = x         // ERROR: literal is not lvalue
(x + y) = z    // ERROR: arithmetic result is not lvalue
var a = 5
a = 10         // OK: var binding is lvalue
```

**Permission requirement:**

The lvalue must have `mut` or `own` permission:

```cursive
let imm_val = 5
imm_val = 10  // ERROR E3D10: let binding is immutable

let mut_val: mut i32 = get_value()
mut_val = 20  // ERROR E4003: cannot assign through imm reference
```

---

**Definition 5.14 (Conditional Expressions):** Conditional expressions select between alternatives based on conditions or pattern matching.

## 5.7 Conditional Expressions

### 5.7.1 If Expression

#### 5.7.1.1 Syntax

The full grammar for `if` expressions is given by the `IfExpr` production in Appendix A.4.

**Examples:**

```cursive
let result: i32 = if condition {
    result 1
} else {
    result 2
}

let value = if x > 0 {
    result positive(x)
} else if x < 0 {
    result negative(x)
} else {
    result zero()
}
```

#### 5.7.1.2 Typing

```
[T-If]
Γ ⊢ cond : bool
Γ ⊢ e₁ : τ
Γ ⊢ e₂ : τ
--------------------------------------------
Γ ⊢ if cond { e₁ } else { e₂ } : τ

[T-If-No-Else]
Γ ⊢ cond : bool
Γ ⊢ e : τ
--------------------------------------------
Γ ⊢ if cond { e } : ()
    (missing else branch implicitly returns ())
```

**Branch type unification:**

Both branches must have the same type (or unify to a common supertype via subtyping).

**Omitted else:**

When `else` is omitted, the implicit else branch returns `()`, so the entire if expression has type `()`.

**Examples:**

```cursive
let x: i32 = if cond { result 1 } else { result 2 }  // OK: both i32

let y = if cond { result 1 }  // Type: (), implicit else returns ()

let invalid: i32 = if cond { result 1 }  // ERROR: if without else cannot produce i32
```

#### 5.7.1.3 Evaluation

```
[E-If-True]
⟨cond, σ⟩ ⇓ ⟨true, σ'⟩
⟨e₁, σ'⟩ ⇓ ⟨v, σ''⟩
--------------------------------------------
⟨if cond { e₁ } else { e₂ }, σ⟩ ⇓ ⟨v, σ''⟩
    (e₂ NOT evaluated)

[E-If-False]
⟨cond, σ⟩ ⇓ ⟨false, σ'⟩
⟨e₂, σ'⟩ ⇓ ⟨v, σ''⟩
--------------------------------------------
⟨if cond { e₁ } else { e₂ }, σ⟩ ⇓ ⟨v, σ''⟩
    (e₁ NOT evaluated)
```

**Evaluation order:**

1. Evaluate condition to boolean
2. Evaluate exactly ONE branch (not both)
3. Return that branch's result

#### 5.7.1.4 Examples

```cursive
// With else
let abs: i32 = if x >= 0 {
    result x
} else {
    result -x
}

// Without else (statement form)
if error_occurred {
    log_error()
}  // Returns ()

// Type unification
let result: f64 = if flag {
    result 1.0
} else {
    result 2.0
}
```

### 5.7.2 If-Let Expression

#### 5.7.2.1 Syntax

The `IfLetExpr` production in Appendix A.4 captures the exact syntax for destructuring conditionals.

**Examples:**

```cursive
if let Option::Some(value) = option_expr {
    process(value)
} else {
    default_case()
}

if let Result::Ok(data) = fetch() {
    result data.transform()
} else {
    result fallback()
}
```

#### 5.7.2.2 Typing

```
[T-If-Let]
Γ ⊢ expr : τ
Γ ⊢ pattern : τ ⇝ Γ'    (pattern matches type τ, extends context)
Γ' ⊢ e₁ : σ             (then-block with pattern bindings)
Γ ⊢ e₂ : σ              (else-block without bindings)
--------------------------------------------
Γ ⊢ if let pattern = expr { e₁ } else { e₂ } : σ
```

**Pattern bindings:**

Variables bound by the pattern are in scope in the then-block but NOT in the else-block.

**Branch type matching:**

Both branches must have the same type.

#### 5.7.2.3 Evaluation

```
[E-If-Let-Match]
⟨expr, σ⟩ ⇓ ⟨v, σ'⟩
pattern matches v with bindings θ
⟨e₁[θ], σ'⟩ ⇓ ⟨result, σ''⟩
--------------------------------------------
⟨if let pattern = expr { e₁ } else { e₂ }, σ⟩ ⇓ ⟨result, σ''⟩
    (e₂ NOT evaluated)

[E-If-Let-No-Match]
⟨expr, σ⟩ ⇓ ⟨v, σ'⟩
pattern does NOT match v
⟨e₂, σ'⟩ ⇓ ⟨result, σ''⟩
--------------------------------------------
⟨if let pattern = expr { e₁ } else { e₂ }, σ⟩ ⇓ ⟨result, σ''⟩
    (e₁ NOT evaluated)
```

**Evaluation order:**

1. Evaluate expression to value
2. Attempt pattern match
3. If match succeeds: bind variables, evaluate then-block
4. If match fails: evaluate else-block

#### 5.7.2.4 Examples

**Option unpacking:**

```cursive
let opt: Option<i32> = Option::Some(42)

let value: i32 = if let Option::Some(x) = opt {
    result x
} else {
    result 0
}
// value = 42
```

**Result handling:**

```cursive
let res: Result<Data, Error> = fetch_data()

if let Result::Ok(data) = res {
    process(data)
} else {
    handle_error()
}
```

**Definition 5.15 (Match Expressions):** Match expressions provide multi-way branching based on pattern matching with exhaustiveness checking.

## 5.8 Match Expressions

**REQUIRED EXPLICITNESS FEATURE:** Match expression result bindings MUST have type annotations.

### 5.8.1 Syntax

#### 5.8.1.1 Grammar

Appendix A.4 specifies match expressions via `MatchExpr` and `MatchArm` productions.

**NORMATIVE REQUIREMENT:** The binding that receives the match result MUST have an explicit type annotation.

#### 5.8.1.2 Required Syntax

```cursive
let output: OutputType = match input {
    Pattern1 => expr1,
    Pattern2 if guard => expr2,
    _ => default,
}
```

**Type annotation REQUIRED** on the binding (`output: OutputType`).

#### 5.8.1.3 Typing

```
[T-Match]
Γ ⊢ scrutinee : τ_scrutinee
enum τ_scrutinee { V₁, ..., Vₙ } declared
∀i. Γ, pᵢ ⊢ armᵢ : τ_result
patterns {p₁, ..., pₘ} exhaustively cover {V₁, ..., Vₙ}
all arms have type τ_result
--------------------------------------------
Γ ⊢ match scrutinee { arms } : τ_result
```

**Requirements:**

- All arms must have the same result type
- Patterns must be exhaustive (cover all possible values)
- When the result of `match` is bound to a name, the binding MUST declare an explicit result type (see Diagnostic E4402)

#### 5.8.1.4 Diagnostic E4402

**ERROR E4402: Missing type annotation on match expression binding**

**Trigger:** Match result binding lacks type annotation.

**Message:** "Match expression binding missing type annotation"

**Note:** "Match result must have explicit type: let name: Type = match ..."

**Fix:** "Add type annotation to binding"

**Error examples:**

```cursive
let output = match input {  // ERROR E4402: missing type annotation
    Pattern1 => expr1,
    Pattern2 => expr2,
}

// Correct:
let output: ResultType = match input {
    Pattern1 => expr1,
    Pattern2 => expr2,
}
```

**Rationale:**

- Eliminates type inference for LLMs
- Makes expected result type explicit
- Documents intent of match expression
- Localizes type errors

### 5.8.2 Pattern Forms

Patterns are specified in Foundations §3.4. This section summarizes pattern forms and their semantics in match expressions.

#### 5.8.2.1 Literal Patterns

**Syntax:** Literal patterns correspond to the `LiteralPattern` alternative in Appendix A.3.

**Matching:** Value must equal the literal.

```cursive
let status: string = match code {
    0 => "success",
    1 => "warning",
    2 => "error",
    _ => "unknown",
}
```

#### 5.8.2.2 Variable Patterns

**Syntax:** Variable patterns use `IdentPattern` from Appendix A.3 to bind the scrutinee value.

**Matching:** Always matches, binds value to identifier.

```cursive
let result: i32 = match option {
    Option::Some(x) => x * 2,  // x binds to payload
    Option::None => 0,
}
```

**Scope extension:**

The bound variable is in scope for the arm expression.

#### 5.8.2.3 Wildcard Pattern

**Syntax:** The catch-all `_` form is represented by `WildcardPattern` in Appendix A.3.

**Matching:** Matches any value, creates no binding.

```cursive
let safe_divide: Option<f64> = match denominator {
    0.0 => Option::None,
    _ => Option::Some(numerator / denominator),  // Matches all non-zero
}
```

#### 5.8.2.4 Tuple Patterns

**Syntax:** Tuple destructuring follows the `TuplePattern` production in Appendix A.3.

**Matching:** Destructures tuple, matches nested patterns.

```cursive
let sum: i32 = match pair {
    (x, y) => x + y,  // Destructures (i32, i32)
}

let nested: i32 = match data {
    ((a, b), c) => a + b + c,  // Nested destructuring
}
```

#### 5.8.2.5 Record Patterns

**Syntax:** Record patterns reuse `RecordPattern` and `FieldPattern` from Appendix A.3; shorthand field names bind directly, while an explicit `field: pattern` rebinding uses the second alternative.

**Matching:** Matches record, destructures fields.

```cursive
let distance: f64 = match point {
    Point { x, y } => ((x * x) + (y * y)).sqrt(),
    // Shorthand for Point { x: x, y: y }
}

let value: i32 = match data {
    Data { field: x, other: _ } => x * 2,  // Explicit pattern
}
```

#### 5.8.2.6 Enum Patterns

**Syntax:** Variant destructuring is covered by the `EnumPattern` production in Appendix A.3.

**Matching:** Matches specific enum variant, destructures payload.

```cursive
let value: i32 = match option {
    Option::Some(x) => x,
    Option::None => 0,
}

let status: string = match result {
    Result::Ok(data) => data.to_string(),
    Result::Err(error) => error.message(),
}
```

#### 5.8.2.7 OR Patterns

**Syntax:** Alternation uses the `OrPattern` non-terminal from Appendix A.3 (`Pattern "|" Pattern`).

**Typing and binding constraints (normative):**

- For an OR pattern `p1 | p2` used against a scrutinee of type τ, both alternatives MUST be valid patterns for τ.
- `p1` and `p2` MUST bind the same set of identifiers with identical types at identical structural positions; otherwise the program is ill‑typed (E4001).
- The resulting binding environment of `p1 | p2` has exactly those identifiers; at runtime their values are taken from the alternative that matched.

**Associativity and precedence (normative):**

- Pattern `|` is left‑associative: `a | b | c` ≡ `(a | b) | c`.
- `|` has the lowest precedence among pattern combinators; parentheses MAY be used to group.

**Matching:** `p1 | p2` matches a value v iff either `p1` matches v or `p2` matches v.

```
[E-Pat-Or-Match]
p1 matches v with θ₁  ⇒ success
-------------------------------------------
(p1 | p2) matches v with θ₁

¬(p1 matches v) ∧ p2 matches v with θ₂  ⇒ success
-------------------------------------------
(p1 | p2) matches v with θ₂
```

If both alternatives can match, either may succeed; bindings θ are well‑defined because both sides bind the same identifiers and types.

**Exhaustiveness (normative):** In coverage analysis, `p1 | p2` covers the union of variants/constructors covered by `p1` and `p2`.

CITE: §5.8.5 — Exhaustiveness Checking.

```cursive
let category: string = match value {
    1 | 2 | 3 => "small",
    4 | 5 | 6 => "medium",
    _ => "large",
}
```

#### 5.8.2.8 Binding Patterns

**Syntax:** Appendix A.3 (`Pattern`) allows an `as` clause to bind the entire matched value.

**Matching:** Matches nested pattern, also binds entire value.

```cursive
let data: Data = match result {
    Result::Ok(value) as success => {
        log("Success: {success}")
        result value
    },
    Result::Err(e) => default_data(),
}
```

### 5.8.3 Guard Clauses

#### 5.8.3.1 Syntax

The optional guard syntax is already included in `MatchArm` (Appendix A.4), where a pattern may be followed by `if Expr`.

**Examples:**

```cursive
let category: string = match value {
    x if x < 0 => "negative",
    x if x > 0 => "positive",
    _ => "zero",
}
```

#### 5.8.3.2 Evaluation Order

**Guard evaluation happens AFTER pattern match succeeds:**

1. Attempt pattern match
2. If pattern matches: evaluate guard expression
3. If guard is `true`: execute arm
4. If guard is `false`: try next arm

```
[E-Match-Guard]
pattern p matches value v with bindings θ
⟨guard[θ], σ⟩ ⇓ ⟨true, σ'⟩
⟨arm[θ], σ'⟩ ⇓ ⟨result, σ''⟩
--------------------------------------------
arm with guard executes

[E-Match-Guard-Fail]
pattern p matches value v with bindings θ
⟨guard[θ], σ⟩ ⇓ ⟨false, σ'⟩
--------------------------------------------
try next arm (guard failed)
```

#### 5.8.3.3 Guard Scope

**Pattern bindings available in guard:**

```cursive
let result: string = match option {
    Option::Some(x) if x > 0 => "positive",  // x in scope for guard
    Option::Some(x) if x < 0 => "negative",  // Different x binding
    Option::Some(_) => "zero",
    Option::None => "none",
}
```

**Guard must be boolean:**

```
[T-Match-Guard]
Γ, bindings_from_pattern ⊢ guard : bool
--------------------------------------------
guard is well-typed
```

#### 5.8.3.4 Examples

**Filtering with guards:**

```cursive
let category: string = match num {
    x if x < 0 => "negative",
    x if x == 0 => "zero",
    x if x > 0 && x <= 10 => "small positive",
    x if x > 10 => "large positive",
}  // Exhaustive with guards
```

**Numeric ranges:**

```cursive
let grade: char = match score {
    s if s >= 90 => 'A',
    s if s >= 80 => 'B',
    s if s >= 70 => 'C',
    s if s >= 60 => 'D',
    _ => 'F',
}
```

### 5.8.5 Exhaustiveness Checking

#### 5.8.5.1 Algorithm

**Definition 5.16 (Exhaustiveness Checking):** The exhaustiveness algorithm verifies that patterns cover all possible enum variants.


**Algorithm:**

```
exhaustiveness_check(E, P):
    // E: enum with variants V = {V₁, ..., Vₙ}
    // P: pattern set {p₁, ..., pₘ}

    covered := ∅

    for each pattern p in P:
        match p:
            case E.Vᵢ:
                covered := covered ∪ {Vᵢ}
            case E.Vᵢ(subpatterns):
                covered := covered ∪ {Vᵢ}
            case E.Vᵢ { field_patterns }:
                covered := covered ∪ {Vᵢ}
            case _ (wildcard):
                covered := V  // Covers all
                return COVERED
            case x (variable):
                covered := V  // Covers all
                return COVERED

    uncovered := V \ covered

    if uncovered = ∅:
        return COVERED
    else:
        return MISSING(uncovered)
```

#### 5.8.5.2 Complexity

**Time complexity:** O(|V| × |P|)

- |V| = number of enum variants
- |P| = number of patterns

**Optimizations:**

- Decision tree compilation: O(d) where d = pattern depth
- Early termination on wildcard/variable patterns
- Pattern specialization precomputation


#### 5.8.5.3 Diagnostic E4005

**ERROR E4005: Non-exhaustive match**

**Trigger:** Patterns do not cover all enum variants.

**Message:** "Non-exhaustive patterns"

**Note:** "Missing coverage for variant(s): {list}"

**Fix:** "Add patterns for missing variants or use wildcard '\_'"

**Examples:**

```cursive
enum Option<T> { Some(T), None }

// ERROR E4005:
let result: i32 = match opt {
    Option::Some(x) => x,
}  // Missing Option::None

// Correct:
let result: i32 = match opt {
    Option::Some(x) => x,
    Option::None => 0,  // Covers all variants
}

// Alternative with wildcard:
let result: i32 = match opt {
    Option::Some(x) => x,
    _ => 0,  // Catches remaining (None)
}
```

### 5.8.6 Match Evaluation

#### 5.8.6.1 Sequential Matching

```
[E-Match]
⟨scrutinee, σ⟩ ⇓ ⟨v, σ'⟩
pattern pᵢ matches v with bindings θ
guard (if present) evaluates to true
⟨armᵢ[θ], σ'⟩ ⇓ ⟨result, σ''⟩
--------------------------------------------
⟨match scrutinee { arms }, σ⟩ ⇓ ⟨result, σ''⟩
```

**First match wins:** Patterns are tried in order from top to bottom. The first matching pattern (with true guard if present) determines the result.


#### 5.8.6.2 Evaluation Order

**Complete evaluation sequence:**

1. Evaluate scrutinee expression to value
2. For each arm (top to bottom):
   a. Attempt pattern match
   b. If pattern matches:
   - Extract bindings from pattern
   - If guard present: evaluate guard with bindings
   - If guard true (or no guard): evaluate arm expression with bindings
   - Return arm result
     c. If pattern doesn't match or guard false: try next arm
3. If no arm matches: runtime error (prevented by exhaustiveness checking)

**Example demonstrating order:**

```cursive
var counter = 0
procedure next(): i32 {
    counter += 1
    result counter
}

let result: string = match next() {
    1 => "first",
    2 => "second",
    x => "other",
}
// counter incremented once (scrutinee evaluated once)
// result = "first"
```

#### 5.8.6.3 Examples

**Option matching:**

```cursive
let value: i32 = match get_option() {
    Option::Some(x) => x * 2,
    Option::None => 0,
}
```

**Result matching:**

```cursive
let data: Data = match fetch() {
    Result::Ok(d) => d,
    Result::Err(e) => {
        log_error(e)
        result default_data()
    },
}
```

**With guards:**

```cursive
let category: string = match number {
    x if x < 0 => "negative",
    x if x == 0 => "zero",
    x if x <= 10 => "small",
    x => "large",
}
```

**Exhaustiveness:**

```cursive
enum Status { Active, Inactive, Pending }

let message: string = match status {
    Status::Active => "running",
    Status::Inactive => "stopped",
    Status::Pending => "waiting",
}  // Exhaustive: all 3 variants covered
```

**Definition 5.17 (Loop Expressions):** Loop expressions provide unified iteration constructs with optional verification contracts.

## 5.9 Loop Expressions

Cursive provides a unified `loop` keyword for all iteration patterns.

### 5.9.1 Infinite Loop

#### 5.9.1.1 Syntax

Infinite loops are a special case of `LoopExpr` without a loop head (Appendix A.4).

**Examples:**

```cursive
loop {
    process()
    if should_exit() {
        break
    }
}
```

#### 5.9.1.2 Typing

```
[T-Loop-Infinite-Never]
Γ ⊢ body : τ
no break statement in body
--------------------------------------------
Γ ⊢ loop { body } : !    (never type)

[T-Loop-Infinite-Break]
Γ ⊢ body : τ
all break statements have value of type σ
--------------------------------------------
Γ ⊢ loop { body } : σ
```

**Type depends on breaks:**

- No `break` statements → type `!` (never returns)
- `break` without value → type `()`
- `break value` → type determined by break values

#### 5.9.1.3 Break Value Typing

**With break values:**

```cursive
let result: i32 = loop {
    let x = compute()
    if x > 100 {
        break x  // Loop result is i32
    }
}
```

**Multiple breaks must unify:**

```cursive
let value: i32 = loop {
    if cond1 {
        break 1  // i32
    }
    if cond2 {
        break 2  // i32
    }
}  // OK: both breaks have i32

let invalid: i32 = loop {
    if cond1 {
        break 1    // i32
    }
    if cond2 {
        break "text"  // string
    }
}  // ERROR E4001: break values don't unify
```

### 5.9.2 Conditional Loop

#### 5.9.2.1 Syntax

A conditional loop is the `LoopExpr` form whose head is a standalone boolean expression (Appendix A.4).

**While-style semantics.**

**Examples:**

```cursive
loop x < 100 {
    x += 1
}

loop !done {
    process()
    done = check_completion()
}
```

#### 5.9.2.2 Typing

```
[T-Loop-Cond]
Γ ⊢ cond : bool
Γ ⊢ body : τ
--------------------------------------------
Γ ⊢ loop cond { body } : ()
    (unless body contains break with value)
```

**Condition must be boolean.**

#### 5.9.2.3 Evaluation

```
[E-Loop-Cond]
⟨cond, σ⟩ ⇓ ⟨true, σ'⟩
⟨body, σ'⟩ ⇓ σ''    (no break)
⟨loop cond { body }, σ''⟩ ⇓ ⟨result, σ_final⟩    (recurse)
--------------------------------------------
⟨loop cond { body }, σ⟩ ⇓ ⟨result, σ_final⟩

[E-Loop-Cond-Exit]
⟨cond, σ⟩ ⇓ ⟨false, σ'⟩
--------------------------------------------
⟨loop cond { body }, σ⟩ ⇓ ⟨(), σ'⟩
```

**Semantics:**

1. Evaluate condition
2. If true: execute body, repeat
3. If false: exit with `()`

### 5.9.3 Iterator Loop

**Rule 5.9.3-1 (Iterator Binding Annotation).** Loop iterator bindings MUST specify their element type.

#### 5.9.3.1 Grammar

Iterator loops correspond to the `LoopExpr` production with a pattern-and-type head (`Pattern "in" Expr`) in Appendix A.4. **NORMATIVE REQUIREMENT:** The iterator binding MUST have an explicit type annotation.

#### 5.9.3.2 Syntax

```cursive
loop item: ItemType in collection {
    process(item)
}

loop i: i32 in 0..10 {
    println(i)
}
```

**Type annotation REQUIRED** on the binding (`item: ItemType`, `i: i32`).

#### 5.9.3.3 Typing

```
[T-Loop-Iter]
Γ ⊢ collection : Iterable<τ>
annotation declares type τ
Γ, item: τ ⊢ body : σ
--------------------------------------------
Γ ⊢ loop item: τ in collection { body } : ()
```

**Requirements:**

- Collection must implement iteration protocol (or be a `Range<T>`)
- Type annotation must match collection's element type
- Type annotation is REQUIRED

#### 5.9.3.4 Evaluation

```
[E-Loop-Iter]
⟨collection, σ⟩ ⇓ ⟨iter, σ'⟩
⟨iter.next(), σ'⟩ ⇓ ⟨Option::Some(v₁), σ₁⟩
⟨body[item ↦ v₁], σ₁⟩ ⇓ σ₂    (no break)
⟨loop item in iter { body }, σ₂⟩ ⇓ ⟨result, σ_final⟩    (recurse)
--------------------------------------------
⟨loop item in collection { body }, σ⟩ ⇓ ⟨result, σ_final⟩

[E-Loop-Iter-Done]
⟨iter.next(), σ⟩ ⇓ ⟨Option::None, σ'⟩
--------------------------------------------
⟨loop item in iter { body }, σ⟩ ⇓ ⟨(), σ'⟩
```

**Desugaring:** Iterator loops desugar to calls to an iteration protocol (e.g., `::next()` procedure).

#### 5.9.3.5 Diagnostic E4404

**ERROR E4404: Missing type annotation on loop iterator binding**

**Trigger:** Iterator loop binding lacks type annotation.

**Message:** "Loop iterator binding missing type annotation"

**Note:** "Iterator element type must be explicit: loop item: Type in collection"

**Fix:** "Add type annotation to iterator binding"

**Error examples:**

```cursive
loop item in collection {  // ERROR E4404: missing type annotation
    process(item)
}

// Correct:
loop item: ItemType in collection {
    process(item)
}

loop i: i32 in 0..100 {  // OK: type annotation present
    println(i)
}
```

**Rationale:**

- Eliminates type inference for iterator elements
- Makes element type explicit for LLMs
- Documents expected type in code

### 5.9.5 Break

#### 5.9.5.1 Syntax

The `BreakExpr` and `LabelRef` rules in Appendix A.4 enumerate the allowed break forms.

**Four forms:**

1. `break` — exit loop, return `()`
2. `break value` — exit loop, return value
3. `break 'label` — exit labeled loop, return `()`
4. `break 'label value` — exit labeled loop, return value

**Examples:**

```cursive
loop {
    if done {
        break  // Exit, return ()
    }
}

let result: i32 = loop {
    let x = compute()
    if x > 100 {
        break x  // Exit, return x
    }
}

'outer: loop {
    'inner: loop {
        if condition {
            break 'outer  // Break outer loop
        }
    }
}
```

#### 5.9.5.2 Typing

```
[T-Break]
break appears in loop context
--------------------------------------------
Γ ⊢ break : !

[T-Break-Value]
Γ ⊢ value : τ
break appears in loop context
--------------------------------------------
Γ ⊢ break value : !
loop result type becomes τ
```

**Effect on loop type:**

Break statements determine the loop's result type.

#### 5.9.5.3 Unification

**All break values in a loop must have the same type:**

```
[Break-Value-Unify]
loop contains break v₁, ..., break vₙ
Γ ⊢ v₁ : τ    ...    Γ ⊢ vₙ : τ
--------------------------------------------
loop result type is τ

[Break-Value-Mismatch]
loop contains break v₁: τ₁, break v₂: τ₂
τ₁ ≠ τ₂
--------------------------------------------
ERROR E4001: Type mismatch in break values
```

#### 5.9.5.4 Evaluation

```
[E-Break]
⟨break value, σ⟩
--------------------------------------------
exit innermost loop with value (or () if no value)
```

**Control flow:** Break immediately exits the loop, skipping remaining iterations.

#### 5.9.5.5 Diagnostic E4011

**ERROR E4011: Break outside loop**

**Trigger:** `break` appears outside loop context.

**Message:** "Break statement outside loop"

**Fix:** "Break can only appear inside loop expressions"

**Example:**

```cursive
function invalid() {
    break  // ERROR E4011: not in loop
}
```

### 5.9.6 Continue

#### 5.9.6.1 Syntax

The grammar for continue statements is captured by `ContinueStmt`/`LabelRef` in Appendix A.5.

**Two forms:**

1. `continue` — skip to next iteration of innermost loop
2. `continue 'label` — skip to next iteration of labeled loop

#### 5.9.6.2 Typing

```
[T-Continue]
continue appears in loop context
--------------------------------------------
Γ ⊢ continue : !
```

#### 5.9.6.3 Evaluation

```
[E-Continue]
⟨continue, σ⟩
--------------------------------------------
skip remainder of loop body, start next iteration
```

#### 5.9.6.4 Diagnostic E4012

**ERROR E4012: Continue outside loop**

**Trigger:** `continue` outside loop.

**Message:** "Continue statement outside loop"

**Fix:** "Continue can only appear inside loop expressions"

### 5.9.7 Loop Contracts

**Syntax:** Verification clauses use the `LoopVerif` production recorded in Appendix A.4.

**Two clauses:**

1. **Invariants:** `with { inv1, inv2, ... }`
2. **Variant:** `by metric`

**Example:**

```cursive
loop i: i32 in 0..n by n - i
    with {
        0 <= i,
        i <= n,
        sum == sum_of(arr[0..i])
    }
{
    sum += arr[i]
}
```

**Semantics:**

- `with { }`: Invariants that must hold at start of each iteration (verified statically or at runtime)
- `by expr`: Termination metric that must decrease each iteration

**Verification:** These clauses enable formal verification but do not change runtime behavior in release builds.

### 5.9.8 Loop Result Types

```
[T-Loop-Infinite-Never]
loop { body } with no break
--------------------------------------------
type: !

[T-Loop-Break-Value]
loop contains break v₁: τ, ..., break vₙ: τ
all break values have same type τ
--------------------------------------------
type: τ

[T-Loop-Unit]
loop contains break (no value) or no breaks
--------------------------------------------
type: ()
```

**Examples:**

```cursive
// Type: ! (never)
loop {
    process()
}

// Type: i32
let x: i32 = loop {
    break 42
}

// Type: ()
loop condition {
    work()
}
```

### 5.9.9 Loop Labels

**Syntax:** Label declarations follow the `LabeledStmt` grammar in Appendix A.5.

**Scope:** Labels have function-wide scope (visible anywhere in the function).

**Example:**

```cursive
'outer: loop {
    'inner: loop {
        if cond1 {
            break 'outer  // Exit outer loop
        }
        if cond2 {
            continue 'inner  // Next iteration of inner
        }
    }
}
```

**Uniqueness:** Labels must be unique within a function.

CITE: Part III §3.7.1 — Scope Hierarchy (function scope for labels).

**Definition 5.18 (Closure Expressions):** Closure expressions create anonymous functions with captured environment.

## 5.10 Closure Expressions

**Content relocated from:** Part II §2.10 — Map Types and Higher-Order Constructs.

### 5.10.1 Syntax

Closure expressions use the `ClosureExpr` and `ParamList` productions defined in Appendix A.4.


**Examples:**

```cursive
let add = |x: i32, y: i32| -> x + y
let scale = |x: i32| -> x * 10
let complex = |x: i32| {
    println(x)
    result x * 2
}

// With captures
let multiplier = 10
let multiply = |x: i32| -> x * multiplier
```

### 5.10.2 Typing

```
[T-Closure]
Γ, x₁: τ₁, ..., xₙ: τₙ ⊢ body : τᵣ
Γ ⊢ body ! ε
------------------------------------------------
Γ ⊢ |x₁: τ₁, ..., xₙ: τₙ| -> body : (τ₁, ..., τₙ) → τᵣ ! ε

[T-Closure-With-Capture]
Γ ⊢ x₁ : τ_cap₁, ..., xₖ : τ_capₖ    (captured variables)
Γ; Δ ⊢ captured values do not escape active regions
Γ, env{x₁: τ_cap₁, ..., xₖ: τ_capₖ}, p₁: σ₁, ..., pₙ: σₙ ⊢ body : τᵣ
Γ ⊢ body ! ε_body
ε_captured ∪ ε_body ⊆ ε_declared
----------------------------------------------------------
Γ; Δ ⊢ |p₁, ..., pₙ| -> body : (σ₁, ..., σₙ) → τᵣ ! ε_declared
    with capture environment env{x₁: τ_cap₁, ..., xₖ: τ_capₖ}
```


### 5.10.3 Capture Classification

#### 5.10.3.1 Owned Capture

```
[Capture-Owned]
Γ ⊢ x : own τ
x captured in closure
-------------------------
x moved into closure
x invalid after closure creation
closure has exclusive ownership of captured value
```

**Example:**

```cursive
let data: string@Owned = string.from("captured")
let consumer: () → () ! io.write = || {
    println(data)  // data moved into closure
}
// data no longer accessible here (moved)
consumer()
```


#### 5.10.3.2 Mutable Capture

```
[Capture-Mutable]
Γ ⊢ x : mut τ
x captured in closure
Γ; Δ ⊢ closure does not escape region containing x
-------------------------------------------------
closure captures mutable reference
x remains accessible after closure creation
closure lifetime ⊆ x lifetime
```

**Example:**

```cursive
region temp {
    var counter = 0
    let increment: () → () ! ∅ = || {
        counter += 1  // Mutable access
    }
    increment()
    increment()
    // counter == 2
}
```


#### 5.10.3.3 Immutable Capture

```
[Capture-Immutable]
Γ ⊢ x : τ    (immutable)
x captured in closure
-------------------------
closure captures immutable reference
no lifetime restrictions (read-only access)
```

**Example:**

```cursive
let multiplier: i32 = 10
let scale: (i32) → i32 ! ∅ = |x: i32| -> x * multiplier
// multiplier still accessible
let result = scale(5)  // 50
```


### 5.10.4 Effect Propagation

```
[Closure-Effect-Propagation]
closure captures variables with access effects ε_captured
closure body has effects ε_body
closure declared with effects ε_declared
-------------------------------------------------
ε_captured ∪ ε_body ⊆ ε_declared
```

**Effects composition:** Closure effects = capture effects ∪ body effects.

**Example:**

```cursive
procedure create_logger(file: string@Owned): (string) → () ! io.write
    uses alloc.heap, io.write
{
    result |message: string| {
        println(file)    // No effect (immutable capture)
        println(message) // io.write effect
    }
    // Closure effect: io.write
}
```


### 5.10.5 Region Escape

```
[Closure-Region-Escape]
Γ; (Δ · r) ⊢ x : τ    (x allocated in region r)
closure captures own x
Γ; Δ ⊢ closure created (escapes region r)
-------------------------------------------------
ERROR E4015: cannot move region-allocated value into escaping closure

[Closure-Region-Safe]
Γ; (Δ · r) ⊢ x : τ
closure captures imm x or mut x
Γ; (Δ · r) ⊢ closure used only within region r
-------------------------------------------------
OK: closure does not outlive captured references
```

**Diagnostic E4015:**

**Trigger:** Closure captures region value and escapes.

**Message:** "Closure captures region-allocated value '{name}' and escapes region"

**Fix:** "Use closure within region"


### 5.10.6 Dynamic Semantics

**Closure creation:**

```
[E-Closure-Create]
capture environment env from enclosing scope
------------------------------------------------
⟨|params| -> body, σ⟩ ⇓ ⟨closure { env, params, body }, σ⟩
```

**Closure invocation:**

```
[E-Closure-Call]
⟨closure, σ⟩ ⇓ ⟨closure { env, params, body }, σ'⟩
⟨args, σ'⟩ ⇓ ⟨values, σ''⟩
⟨body[env, params ↦ values], σ''⟩ ⇓ ⟨result, σ_final⟩
------------------------------------------------
⟨closure(args), σ⟩ ⇓ ⟨result, σ_final⟩
```

### 5.10.7 Examples

**Immutable capture (simple):**

```cursive
let multiplier: i32 = 10
let scale: (i32) → i32 ! ∅ = |x: i32| -> x * multiplier
let result = scale(5)  // 50
```

**Owned capture (move):**

```cursive
let data: string@Owned = string.from("captured")
let consumer: () → () ! io.write = || {
    println(data)
}
// data moved, no longer accessible
consumer()
```

**Mutable capture (region-bounded):**

```cursive
region temp {
    var counter = 0
    let increment: () → () ! ∅ = || {
        counter += 1
    }
    increment()
    increment()
    // counter == 2
}
```

**Effect propagation:**

```cursive
procedure create_logger(file: string@Owned): (string) → () ! io.write
    uses alloc.heap, io.write
{
    result |message: string| {
        println(file)
        println(message)
    }
}
```

**Region escape violation:**

```cursive
procedure bad_closure(): () → i32 ! ∅ {
    region temp {
        let local: i32 = 42
        let closure = || -> local
        result closure  // ERROR E4015
    }
}
```

**Correct region usage:**

```cursive
procedure good_closure() {
    region temp {
        let local: i32 = 42
        let closure = || -> local
        let result = closure()  // OK: used within region
    }
}
```

CITE: Part II §2.10 — Map Types and Higher-Order Constructs (source of all relocated closure content).

---

**Definition 5.19 (Region Expressions):** Region expressions manage memory with explicit lifetime scopes.

## 5.11 Region Expressions

### 5.11.1 Region Block

#### 5.11.1.1 Syntax

Region blocks use the `RegionExpr` production defined in Appendix A.4.

**From Foundations §3.3:** `region r { e }`

**Examples:**

```cursive
region temp {
    let data = alloc_in<temp>(Buffer.new())
    process(data)
}  // data deallocated here (O(1))
```

#### 5.11.1.2 Typing

```
[T-Region]
r is fresh region name
Γ; (Δ · r) ⊢ body : τ    (region r added to context)
--------------------------------------------
Γ; Δ ⊢ region r { body } : τ
```

**Result type:** Type of block body.

**Escape checking:** Values allocated in r cannot escape.

#### 5.11.1.3 Evaluation

```
[E-Region]
push region r onto region stack
⟨body, σ⟩ ⇓ ⟨v, σ'⟩
deallocate all allocations in region r (LIFO, O(1))
pop region r
--------------------------------------------
⟨region r { body }, σ⟩ ⇓ ⟨v, σ''⟩
```

**Deallocation:** O(1) bulk free, LIFO order.

### 5.11.2 Region Allocation

#### 5.11.2.1 Syntax

Region allocations follow the `AllocInExpr` production in Appendix A.4.

**Examples:**

```cursive
region temp {
    let buffer = alloc_in<temp>(Buffer.new(1024))
}
```

#### 5.11.2.2 Typing

```
[T-Alloc-In]
r ∈ Δ    (region r active)
Γ ⊢ value : τ
--------------------------------------------
Γ; Δ ⊢ alloc_in<r>(value) : own τ
```

#### 5.11.2.3 Evaluation

```
[E-Alloc-In]
⟨value, σ⟩ ⇓ ⟨v, σ'⟩
ℓ = fresh address in region r
--------------------------------------------
⟨alloc_in<r>(value), σ⟩ ⇓ ⟨ℓ, σ'[ℓ ↦ v]⟩
```

### 5.11.3 Escape Prevention

**Diagnostic E4014:**

**Trigger:** Region value escapes region.

**Message:** "Value allocated in region '{region}' escapes region"

**Fix:** "Use value within region or allocate differently"

**Error:**

```cursive
function bad() -> Buffer {
    region temp {
        let buf = alloc_in<temp>(Buffer.new())
        result buf  // ERROR E4014
    }
}
```

CITE: Part III §3.9.2; Part VII — Regions.

---

**Definition 5.20 (Comptime Expressions):** Comptime expressions execute at compile-time.

## 5.12 Comptime Expressions

### 5.12.1 Comptime Blocks

The syntax of compile-time blocks is given by `ComptimeExpr` in Appendix A.4.

**From Foundations §3.3:** `comptime { e }`

```
[T-Comptime]
Γ ⊢ body : τ
body is compile-time evaluable
--------------------------------------------
Γ ⊢ comptime { body } : τ
    result is compile-time constant
```

### 5.12.2 Compile-Time Evaluable Expressions

From Part III §3.3.2: literals, arithmetic, comptime functions, compile-time constants.

### 5.12.3 Restrictions

No I/O, no heap allocation, no non-determinism, pure functions only.

### 5.12.4 Use Cases

Array lengths, const generics, type position values.

CITE: Part III §3.3 — Compile-Time Evaluation; Part X — Comptime Metaprogramming.

---

**Definition 5.21 (Type Conversions):** Type conversion expressions.

## 5.13 Type Conversions and Casts

### 5.13.1 Explicit Casts

Explicit casts use the `CastExpr` production in Appendix A.4.

```
[T-Cast]
Γ ⊢ expr : τ_source
τ_source and τ_target compatible
--------------------------------------------
Γ ⊢ expr as τ_target : τ_target
```

### 5.13.2 Numeric Conversion Policy

**NORMATIVE:** No implicit numeric promotions. All conversions MUST be explicit.

```cursive
let x: i32 = 42
let y: i64 = x          // ERROR E4001
let z: i64 = x as i64   // OK: explicit
```

CITE: Part II §2.0.6.6 — Numeric Promotion Policy.

### 5.13.3 Permission Coercions

Implicit: `own T → imm T`, `own T → mut T`, `mut T → imm T`

### 5.13.4 Modal Coercions

`string@Owned → string@View` (implicit), `string@View → string@Owned` (requires `alloc.heap`)

CITE: Part II §2.1.6.

### 5.13.5 Unsafe Transmute

```
transmute<T, U>: (T) → U ! unsafe.transmute
```

CITE: Part II §2.12.3; Part XII — Unsafe.

---

**Definition 5.22 (Return Expressions):** Return expressions exit functions.

## 5.14 Return Expressions

### 5.14.1 Syntax

Return syntax is governed by the `ReturnExpr` production in Appendix A.4.

**From Foundations §3.3:** `return e`

### 5.14.2 Typing

```
[T-Return]
Γ ⊢ e : τ
function return type is τ
-------------------------
Γ ⊢ return e : !

[T-Return-Unit]
function return type is ()
-------------------------
Γ ⊢ return : !
```

### 5.14.3 Early Exit Semantics

Exits function immediately. RAII cleanup: defers execute, own values dropped, regions deallocated.

### 5.14.4 Diagnostics

**E4013:** Return outside function

**E4001:** Type mismatch with function return type

CITE: Part IX — Functions and Procedures.

---

**Definition 5.23 (Defer Expressions):** Defer expressions schedule cleanup.

## 5.15 Defer Expressions

### 5.15.1 Syntax

Deferred blocks use the `DeferExpr` production described in Appendix A.4.

**Examples:**

```cursive
{
    let file = open("data.txt")
    defer { file.close() }
    process(file)
}  // file.close() executes here
```

### 5.15.2 Typing

```
[T-Defer]
Γ ⊢ block : ()
--------------------------------------------
Γ ⊢ defer { block } : ()
```

### 5.15.3 Execution Order

**LIFO at scope exit.** Executes on ALL paths: normal, return, break, panic.

### 5.15.4 Evaluation

```
[E-Defer-Block-Exit]
scope exits
defers = [d₁, ..., dₙ]
--------------------------------------------
execute dₙ, ..., d₁  (reverse order)
```

### 5.15.5 Examples

```cursive
{
    defer { println("Third") }
    defer { println("Second") }
    defer { println("First") }
}
// Output: First, Second, Third (LIFO)
```

---

**Definition 5.24 (Contract Annotations):** Contract annotation expressions.

## 5.16 Contract Annotation Expressions

### 5.16.1 Syntax

The `ContractExpr` production in Appendix A.4 formalizes contract annotation syntax.

**From Foundations §3.3:** `contract(P, e, Q)`

### 5.16.2 Typing

```
[T-Contract]
Γ ⊢ P : bool    (precondition)
Γ ⊢ e : τ
Γ, result: τ ⊢ Q : bool    (postcondition)
--------------------------------------------
Γ ⊢ contract(P, e, Q) : τ
```

### 5.16.3 Evaluation

```
[E-Contract]
⟨P, σ⟩ ⇓ ⟨true, σ'⟩
⟨e, σ'⟩ ⇓ ⟨v, σ''⟩
⟨Q[result ↦ v], σ''⟩ ⇓ ⟨true, σ'''⟩
--------------------------------------------
⟨contract(P, e, Q), σ⟩ ⇓ ⟨v, σ'''⟩

[E-Contract-Violation]
⟨P, σ⟩ ⇓ ⟨false, σ'⟩  OR  ⟨Q, σ''⟩ ⇓ ⟨false, σ'''⟩
--------------------------------------------
⟨contract(P, e, Q), σ⟩ ⇓ panic("contract violation")
```

### 5.16.4 Diagnostics

**E4016:** Contract precondition violation (runtime panic)

**E4017:** Contract postcondition violation (runtime panic)

### 5.16.5 Examples

```cursive
let result = contract(
    input > 0,
    compute(input),
    result > 0 && result < 100
)
```

---

**Definition 5.25 (Explicit Transitions):** Explicit state transition expressions.

## 5.17 Explicit State Transition Expressions

### 5.17.1 Syntax

Transition expressions use the `TransitionExpr` production in Appendix A.4.

**From Foundations §3.3:** `transition(e, S)`

### 5.17.2 Typing

```
[T-Transition]
Γ ⊢ e : Modal@S₁
Modal@S₁ → Modal@S₂ valid transition
--------------------------------------------
Γ ⊢ transition(e, @S₂) : Modal@S₂
```

### 5.17.3 Evaluation

```
[E-Transition]
⟨e, σ⟩ ⇓ ⟨Modal@S₁ { data }, σ'⟩
--------------------------------------------
⟨transition(e, @S₂), σ⟩ ⇓ ⟨Modal@S₂ { data }, σ'⟩
```

### 5.17.4 Transition vs Procedure

**When to use:** Alternative to procedure-based state transitions for explicit state changes.

---

**Definition 5.26 (Sequencing):** Statement sequencing expressions.

## 5.18 Sequencing and Statement Separators

### 5.18.1 Syntax

Sequencing relies on the `SeqExpr` and `Separator` productions in Appendix A.4/A.1.

**From Foundations §3.3:** `e₁ SEP e₂`

### 5.18.2 Typing

```
[T-Sequence]
Γ ⊢ e₁ : τ₁
Γ ⊢ e₂ : τ₂
τ₁ = () OR warn("unused expression result")
--------------------------------------------
Γ ⊢ e₁ SEP e₂ : τ₂
```

### 5.18.3 Evaluation

```
[E-Sequence]
⟨e₁, σ⟩ ⇓ ⟨v₁, σ'⟩    (evaluate, discard)
⟨e₂, σ'⟩ ⇓ ⟨v₂, σ''⟩
------------------------
⟨e₁ SEP e₂, σ⟩ ⇓ ⟨v₂, σ''⟩
```

### 5.18.4 Warning

**Unused non-() result:** LHS should have type `()` or value is wasted.

CITE: Foundations §2.7 — Statement Termination.

---

**Definition 5.27 (Effect Composition):** Effect composition rules for compound expressions.

## 5.19 Effect Composition

### 5.19.1 Effect Judgment

**Form:** `Γ ⊢ e : T ! ε`

Effects compose across subexpressions via union (∪) according to the effect lattice.

### 5.19.2 Effect Union

```
[Effect-Union]
Γ ⊢ e₁ : τ₁ ! ε₁
Γ ⊢ e₂ : τ₂ ! ε₂
--------------------------------
Γ ⊢ compound(e₁, e₂) : τ ! (ε₁ ∪ ε₂)
```

**Lattice laws:** Idempotent, commutative, associative.

CITE: Part VI — Contracts and Effects (effect lattice).

### 5.19.3 Effect Checking

```
[Effect-Check-Call]
ε_caller ⊇ ε_callee
--------------------------------------------
call allowed

[Effect-Unavailable]
ε_total ⊈ ε_caller
--------------------------------------------
ERROR E4004: Effect(s) unavailable
```

### 5.19.4 Principal Effects

**Theorem:** For every expression, there exists a minimal effect set.

```
Γ ⊢ e : τ ! ε_minimal
∀ε'. (Γ ⊢ e : τ ! ε' ⟹ ε_minimal ⊆ ε')
```

### 5.19.5 Effect Holes (See Part VIII §8.9)

Effect holes are specified canonically in Part VIII §8.9. This section is informative.

- Use sites: effect holes appear only in effect‑set positions within type expressions used in expression contexts (e.g., closure type annotations).
- Semantics: hole introduction, principal‑effect substitution, and ambiguity diagnostics are defined in Part VIII §8.9; principal effects themselves are defined in §5.19.4.

---

**Definition 5.28 (Evaluation Order):** Evaluation order and determinism guarantees.

## 5.20 Evaluation Order and Determinism

**AUTHORITATIVE SECTION**: This section is the normative, authoritative specification for evaluation order in Cursive. All other mentions of evaluation order in the specification defer to the rules defined here.

### 5.20.1 Strict Left-to-Right

**NORMATIVE GUARANTEE:** All subexpressions evaluate in strict left-to-right order.

This is a **required** behavior for all conforming Cursive implementations. Unlike C/C++ (which leave evaluation order unspecified), Cursive guarantees deterministic, predictable evaluation order to:
- Enable reliable reasoning about effect order
- Ensure reproducible behavior across compilers
- Support debugging and testing
- Allow safe interleaving of effectful operations

**Scope**: This guarantee applies to:
- Function call arguments (§5.20.2)
- Binary operator operands (§5.20.3)
- Record field initialization
- Tuple element construction
- Array element initialization
- All compound expressions

**Exception**: Short-circuit logical operators `&&` and `||` (§5.20.4)

### 5.20.2 Function Arguments

**Arguments evaluate left-to-right before call.**

```cursive
process(first(), second(), third())
// Order: first(), then second(), then third(), then call
```

### 5.20.3 Binary Operators

**Left operand before right operand.**

```cursive
f() + g()  // f() evaluates first, then g()
```

### 5.20.4 Short-Circuit Exception

**ONLY exception:** `&&` and `||` may skip RHS.

```cursive
false && expensive()  // expensive() NOT called
true || expensive()   // expensive() NOT called
```

### 5.20.5 Statement Separators

Newline/semicolon guarantee sequencing.

CITE: Foundations §2.7.

### 5.20.6 Side Effect Visibility

Effects occur in evaluation order, observable externally.

### 5.20.7 Determinism Guarantees

**CRITICAL DISTINCTION:** Cursive provides two levels of determinism:

**1. Order Determinism (Always Guaranteed)**

The **evaluation order** is always deterministic, regardless of effects:

```cursive
// Evaluation order: f() → g() → h() (guaranteed)
process(f(), g(), h())

// Even with I/O, order is deterministic
log(read_file("a.txt"), read_file("b.txt"))  // a.txt read before b.txt
```

**2. Result Determinism (Pure Expressions Only)**

The **evaluation result** is deterministic only for pure expressions:

```cursive
// Pure: same input → same output (deterministic result)
let x = 2 + 3  // Always produces 5

// Impure: same input → potentially different output
let user_input = read_line()  // Non-deterministic result
let timestamp = get_current_time()  // Non-deterministic result
```

**Formal Statement:**

```
[Order-Determinism]
For all expressions e:
  evaluation order is deterministic (left-to-right)

[Result-Determinism]
For all expressions e without I/O or concurrency:
  ⟨e, σ⟩ ⇓ ⟨v₁, σ'⟩
  ⟨e, σ⟩ ⇓ ⟨v₂, σ''⟩
  ------------------------------------
  v₁ = v₂ ∧ σ' = σ''
```

**Rationale:**

This distinction enables:
- **Predictable effect sequencing**: I/O operations occur in source order
- **Reproducible pure computations**: Mathematical operations always produce same results
- **Reliable debugging**: Side effects happen in deterministic order
- **Testing purity**: Pure functions can be tested with property-based testing

**Examples:**

```cursive
// Order deterministic, result non-deterministic
uses io.read
procedure load_configs(): (String, String)
{
    // Files read in this order: config1, then config2
    // But file contents may vary → result non-deterministic
    let cfg1 = read_file("config1.txt")
    let cfg2 = read_file("config2.txt")
    return (cfg1, cfg2)
}

// Both order and result deterministic
function compute_sum(a: i32, b: i32): i32
{
    // No I/O, no concurrency → fully deterministic
    return a + b
}
```

---

**Definition 5.29 (Value Categories):** Lvalues, rvalues, and places.

## 5.21 Value Categories

### 5.21.1 Lvalues

**Definition:** Expressions denoting memory locations that can be assigned.

**Forms:** Variables (var), field access, indexing.

### 5.21.2 Lvalue Forms

- `var x`
- `expr.field`
- `expr.N`
- `arr[i]`
- `*mut_ptr`

### 5.21.3 Rvalues

**Definition:** Temporary values without persistent location.

**Forms:** Literals, arithmetic, calls.

### 5.21.4 Rvalue Forms

- Literals: `42`, `"text"`
- Arithmetic: `x + y`
- Calls: `f(args)`
- Constructions: `Point { x, y }`

### 5.21.5 Places

**Definition:** Memory locations with permissions (own/mut/imm).

### 5.21.6 Permission Interaction

```
[Lvalue-Mut-Required]
assignment or mutation
--------------------------------------------
target must be lvalue with mut or own

[Rvalue-Imm-Only]
rvalue
--------------------------------------------
can only access with imm permission
```

---

## 5.24 Diagnostics

See Foundations §8 for the full diagnostic directory. Expression-specific entries are tagged `E400x`, `E401x`, and `E440x`.

## 5.26 Inter-Part Authority

### 5.26.1 Authority Statement

Part V is authoritative for expression formation, typing, and evaluation.

### 5.26.2 Integration

Integrates with Parts I-IV, VI-VIII without contradictions.

---

**Previous**: [Lexical Permission System](04_Lexical-Permissions.md) | **Next**: [Statements and Control Flow](06_Statements-and-Control-Flow.md)
