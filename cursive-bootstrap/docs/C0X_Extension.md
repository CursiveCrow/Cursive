# C0X: Cursive0 Extension — Shared Permission, Generics, Full Classes, Attributes, Imports, and Concurrency

**Status:** Draft extension (normative)

This document is a *normative* addendum to **Cursive0.md** (“the C0 baseline specification”). It introduces and fully specifies the following language features that are present in the original design document (**Cursive.md**) but are not currently represented (or are explicitly rejected) in the C0 baseline:

- **Shared permission** (`shared`) and the **Key System** (including key blocks, key-path expressions, coarsening markers, and type-level boundaries)
- **Generics** (type parameters/arguments, defaults, `where` clauses, monomorphization, and variance)
- **Full classes** (associated types, abstract states for modal classes, object-safety/dispatchability, and coherent implementation at the type definition site)
- **Attributes** (`[[...]]`) for declarations and expressions (excluding metaprogramming/comptime)
- **Module importing** (`import`) and the complete `using` surface (item/list/self/wildcard, re-exports)
- **Structured concurrency** (`parallel`, `spawn`, `dispatch`, `wait`) and its interaction with the Key System

## 0. Scope and non-goals

### 0.1 Normative terms

The keywords **MUST**, **MUST NOT**, **SHOULD**, **SHOULD NOT**, and **MAY** are to be interpreted as described in RFC 2119.

### 0.2 Relationship to Cursive0.md

- All syntax, judgments, helper functions, and dynamic semantics not explicitly defined or modified in this document are inherited from **Cursive0.md**.
- Where this document defines a construct already mentioned in Cursive0.md as “unsupported,” the corresponding “unsupported” constraint is removed and replaced by the formal semantics herein.

### 0.3 Explicit non-goals

This extension does **not** specify metaprogramming/comptime. In particular, it does not define semantics for:

- `comptime` evaluation
- quoting/splicing constructs
- `[[derive]]`, reflection APIs, file embedding, or similar compile-time facilities

This extension also does not attempt to fully restate **Async/Stream** semantics from Cursive.md §19. (Only the concurrency constructs listed above are specified.)

---

# Appendix H: Contracts and Verification (Normative)

The following section is incorporated from the original design and is normative for C0X.

*[REF: Procedure declarations, methods, and overloading are defined in Cursive0.md. This appendix specifies contracts, invariants, verification logic, behavioral subtyping, and foreign contract clauses.]*

## 14. Procedures and Contracts

### 14.4 Contract Syntax

#### 14.4.1 Contract Clause Position

**Contract.** A specification attached to a procedure declaration that asserts logical predicates over program state. Contracts govern logical validity through preconditions (caller obligations) and postconditions (callee guarantees).

A contract C is a pair (P_pre, P_post) where:
- P_pre is a conjunction of boolean predicates representing preconditions
- P_post is a conjunction of boolean predicates representing postconditions

**Syntax**

```ebnf
contract_clause    ::= "|=" contract_body
contract_body      ::= precondition_expr
                     | precondition_expr "=>" postcondition_expr
                     | "=>" postcondition_expr

precondition_expr  ::= predicate_expr
postcondition_expr ::= predicate_expr
predicate_expr     ::= logical_or_expr
contract_intrinsic ::= "@result" | "@entry" "(" expression ")"
```

A contract clause (`|=`) MUST appear after the return type annotation (or after the parameter list if no return type) and before the procedure body.

```cursive
procedure divide(a: i32, b: i32) -> i32
    |= b != 0 => @result * b == a
{ a / b }
```

```cursive
procedure abs(x: i32) -> i32
    |= => @result >= 0 && (@result == x || @result == -x)
{ if x >= 0 { x } else { -x } }
```

```cursive
record Counter { value: i32 }
procedure increment(~!)
    |= self.value >= 0 => self.value == @entry(self.value) + 1
{ self.value += 1 }
```

**Static Semantics**

**(WF-Contract)**

Γ_pre ⊢ P_pre : `bool`    pure(P_pre)
Γ_post ⊢ P_post : `bool`    pure(P_post)
────────────────────────────────────────────────────────────
Γ ⊢ `|=` P_pre ⇒ P_post : WF

**Logical Operators**

Predicates use standard boolean operators with precedence (highest to lowest):
1. `!` (logical NOT) — right-associative
2. `&&` (logical AND) — left-associative, short-circuit
3. `||` (logical OR) — left-associative, short-circuit


#### 14.4.2 Purity Constraints

**Pure Expression.** An expression whose evaluation produces no observable side effects. All expressions within a contract MUST be pure.

**Static Semantics**

An expression e satisfies pure(e) iff:
1. e MUST NOT invoke any procedure that accepts capability parameters
2. e MUST NOT mutate state observable outside the expression's evaluation
3. Built-in operators on primitive types and `comptime` procedures are always pure


#### 14.4.3 Evaluation Contexts

**Evaluation Context.** Defines the set of bindings available for reference within a predicate expression.

**Static Semantics**

**Precondition Evaluation Context (Γ_pre)**

Includes:
- All procedure parameters at their entry state
- The receiver binding (if present)
- All bindings visible in the enclosing scope accessible without side effects

MUST NOT include:
- The `@result` intrinsic
- The `@entry` operator
- Any binding introduced within the procedure body

**Postcondition Evaluation Context (Γ_post)**

Includes:
- All procedure parameters:
  - Immutable parameters (`const`, `~`): same value as at entry
  - Mutable parameters (`unique`, `shared`): post-state value
- The receiver binding (post-state for mutable receivers)
- The `@result` intrinsic
- The `@entry` operator
- All bindings visible in the enclosing scope

**Mutable Parameter State Semantics**

| Location in Contract          | State Referenced |
| :---------------------------- | :--------------- |
| Left of `=>`                  | Entry state      |
| Right of `=>` (bare)          | Post-state       |
| Right of `=>` with `@entry()` | Entry state      |


### 14.5 Pre/Postconditions

#### 14.5.1 Precondition Syntax (`|=`)

**Precondition.** The logical expression appearing to the left of `=>` in a `|=` contract clause, or the entire expression if `=>` is absent. The caller MUST ensure this expression evaluates to `true` prior to the call.

**Static Semantics**

**(Pre-Satisfied)**

Γ ⊢ f : (T_1, …, T_n) → R    precondition(f) = P_pre    StaticProof(Γ_S, P_pre)
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ f(a_1, …, a_n) @ S : valid

Failure to satisfy a precondition is diagnosed at the **caller**. The diagnostic MUST identify the call site.

**Elision Rules**

| Contract Form        | Precondition              |
| :------------------- | :------------------------ |
| `|= P`              | P                         |
| `|= P => Q`         | P                         |
| `|= => Q`           | `true` (always satisfied) |
| (no contract clause) | `true` (always satisfied) |


#### 14.5.2 Postcondition Syntax (`=>`)

**Postcondition.** The logical expression appearing to the right of `=>` in a `|=` contract clause. The callee MUST ensure this expression evaluates to `true` immediately before returning.

**Static Semantics**

**(Post-Valid)**

postcondition(f) = P_post    ∀ r ∈ ReturnPoints(f). Γ_r ⊢ P_post : satisfied
───────────────────────────────────────────────────────────────────────────
f : postcondition-valid

Failure to satisfy a postcondition is diagnosed at the **callee**. The diagnostic MUST identify the return point.

**Elision Rules**

| Contract Form        | Postcondition             |
| :------------------- | :------------------------ |
| `|= P`              | `true` (no postcondition) |
| `|= P => Q`         | Q                         |
| `|= => Q`           | Q                         |
| (no contract clause) | `true` (always satisfied) |


#### 14.5.3 `@result` Binding in Postconditions

**@result Intrinsic.** Refers to the return value in postcondition expressions.

**Static Semantics**

| Property     | Specification                                  |
| :----------- | :--------------------------------------------- |
| Availability | Postcondition expressions only (right of `=>`) |
| Type         | The return type of the enclosing procedure     |
| Value        | The value being returned from the procedure    |
| Unit Returns | If procedure returns `()`, `@result` has `()`  |

**Constraints**

Use of `@result` outside postcondition expressions is ill-formed (`E-SEM-2806`).

**Dynamic Semantics**

At each return point r with returned value v_r, postconditions are evaluated with `@result` bound to v_r.


#### 14.5.4 `@entry()` Expression

**@entry Operator.** Evaluates `expr` in the entry state of the procedure.

**Static Semantics**

| Property              | Specification                                             |
| :-------------------- | :-------------------------------------------------------- |
| Availability          | Postcondition expressions only (right of `=>`)            |
| Semantics             | Evaluates `expr` in entry state of the procedure          |
| Evaluation Point      | After parameter binding, before body execution            |
| Expression Constraint | `expr` MUST be pure                                       |
| Expression Scope      | `expr` MUST reference only parameters and receiver        |
| Type Constraint       | Result type of `expr` MUST implement `Bitcopy` or `Clone` |

**(Entry-Type)**

Γ_post ⊢ e : T    (T <: `Bitcopy` ∨ T <: `Clone`)
────────────────────────────────────────────────────────
Γ_post ⊢ @entry(e) : T

**Dynamic Semantics**

**Capture Semantics**

When `@entry(expr)` appears in a postcondition:
1. The implementation evaluates `expr` immediately after parameter binding
2. The result is captured: `Bitcopy` types use bitwise copy; `Clone` types invoke `clone()`
3. In the postcondition, `@entry(expr)` refers to this captured value

**Constraints**

If the result type of `expr` does not implement `Bitcopy` or `Clone`, the program is ill-formed (`E-SEM-2805`).


### 14.6 Invariants

#### 14.6.1 Type Invariants

**Type Invariant.** A `where` clause attached to a `record`, `enum`, or `modal` type declaration. The invariant constrains all instances of the type.

**Syntax**

```ebnf
type_invariant ::= "where" "{" predicate_expr "}"
```

```cursive
record BoundedCounter { value: u32, max: u32 }
where { self.value <= self.max }
```

**Static Semantics**

**Invariant Predicate Context**

Within a type invariant predicate:
- `self` refers to an instance of the type being defined
- Field access on `self` is permitted
- Method calls on `self` are permitted if the method is pure

**Enforcement Points**

| Enforcement Point   | Description                                       |
| :------------------ | :------------------------------------------------ |
| Post-Construction   | After constructor or literal initialization       |
| Pre-Call (Public)   | Before any public procedure with receiver         |
| Post-Call (Mutator) | Before any procedure taking `unique self` returns |

**Public Field Constraint**

Types with type invariants MUST NOT declare public mutable fields.

**Private Procedure Exemption**

Private procedures (`internal` or less) are exempt from the Pre-Call enforcement point. The type invariant MUST be verified when a private procedure returns to a public caller.


#### 14.6.2 Loop Invariants

**Loop Invariant.** A `where` clause attached to a `loop` expression. The invariant specifies a predicate that MUST hold at the beginning of every iteration and after termination.

**Syntax**

```ebnf
loop_expression ::= "loop" loop_condition? loop_invariant? block_expr
loop_condition  ::= expression
loop_invariant  ::= "where" "{" predicate_expr "}"
```

```cursive
var sum = 0
var i = 0
loop i < n where { sum == i * (i - 1) / 2 && i <= n } {
    sum += i
    i += 1
}
```

**Static Semantics**

**Enforcement Points**

| Point          | Description                               | Formal                                                 |
| :------------- | :---------------------------------------- | :----------------------------------------------------- |
| Initialization | Before the first iteration begins         | Γ_0 ⊢ Inv                                              |
| Maintenance    | At the start of each subsequent iteration | Γ_i ⊢ Inv ⇒ Γ_(i+1) ⊢ Inv                               |
| Termination    | Immediately after loop terminates         | Γ_exit ⊢ Inv                                           |

**Verification Fact Generation**

Upon successful verification at the Termination point, the implementation generates a Verification Fact F(Inv, L_exit) for use as a postcondition of the loop.


#### 14.6.3 Invariant Verification

**Static Semantics**

Invariant verification follows the same rules as contract verification (§14.7):
- Static verification required by default
- `[[dynamic]]` attribute permits runtime verification when static proof fails


### 14.7 Verification Logic

#### 14.7.1 Contract Verification Modes

**Contract Verification.** Determines how predicates are ensured to hold. By default, **static verification is required**. The `[[dynamic]]` attribute permits runtime verification as an explicit opt-in.

**Static Semantics**

**Default: Static Verification Required**

**(Contract-Static-OK)**

StaticProof(Γ_S, P)
──────────────────────────────────────────────
P : verified (no runtime check)

**(Contract-Static-Fail)**

¬ StaticProof(Γ_S, P)    ¬ InDynamicContext
──────────────────────────────────────────────
program is ill-formed (E-SEM-2801)

**With `[[dynamic]]`: Runtime Verification Permitted**

**(Contract-Dynamic-Elide)**

StaticProof(Γ_S, P)
──────────────────────────────────────────────
P : verified (no runtime check)

**(Contract-Dynamic-Check)**

¬ StaticProof(Γ_S, P)    InDynamicContext
────────────────────────────────────────────────────
emit runtime check ContractCheck(P, k, s, ρ)

Here k is the ContractKind at verification location s, and ρ is the corresponding contract environment.

**Mandatory Proof Techniques**

| Technique                | Description                                        |
| :----------------------- | :------------------------------------------------- |
| Constant propagation     | Evaluate expressions with compile-time constants   |
| Linear integer reasoning | Prove inequalities over bounded integer arithmetic |
| Boolean algebra          | Simplify and prove boolean expressions             |
| Control flow analysis    | Track predicates established by conditionals       |
| Type-derived bounds      | Use type constraints (e.g., `usize >= 0`)          |
| Verification facts       | Use facts established by prior checks (§14.7.2)    |

**StaticProof Definition**

Let FactsAt(S) = { P | F(P, L) ∈ Facts ∧ L dom S }.

Define Decidable(P) as the smallest set closed under:

1. `true`, `false`
2. Comparisons of linear integer expressions over literals and variables
3. Syntactic equality (up to alpha-renaming) between identifiers and literal constants
4. Boolean combinations of decidable predicates using `!`, `&&`, `||`

Define entailment FactsAt(S) ⊢ P by the rules:

**(Ent-True)**
P ≡ `true`
──────────────────────────────
FactsAt(S) ⊢ P

**(Ent-Fact)**
P ∈ FactsAt(S)
──────────────────────────────
FactsAt(S) ⊢ P

**(Ent-And)**
FactsAt(S) ⊢ P    FactsAt(S) ⊢ Q
────────────────────────────────────────
FactsAt(S) ⊢ P ∧ Q

**(Ent-Or-L)**  
FactsAt(S) ⊢ P
────────────────────────
FactsAt(S) ⊢ P ∨ Q

**(Ent-Or-R)**  
FactsAt(S) ⊢ Q
────────────────────────
FactsAt(S) ⊢ P ∨ Q

**(Ent-Linear)**
LinearEntails(FactsAt(S), P)
─────────────────────────────
FactsAt(S) ⊢ P

**Linear Integer Entailment**

Let LinExpr be expressions of the form ∑_i a_i x_i + c where a_i, c ∈ ℤ and each x_i is an integer-typed variable. Let LinPred be predicates comparing two LinExpr with `==`, `!=`, `<`, `<=`, `>`, or `>=`.

Define LinFactsAt(S) = { P ∈ FactsAt(S) | P ∈ LinPred }.

Then:

LinearEntails(FactsAt(S), P) ⇔ P ∈ LinPred ∧ ⋀ LinFactsAt(S) ⊨_ℤ P

Implementations MAY use any sound decision procedure; they MUST be complete for LinPred entailment.

Then:

StaticProof(Γ_S, P) ⇔ Decidable(P) ∧ FactsAt(S) ⊢ P

where S is the verification location for P (Table §14.7.1), and Γ_S is the typing environment at S.

**Satisfaction Judgment**

Γ_S ⊢ P : satisfied ⇔ StaticProof(Γ_S, P)

**Verification Location**

| Contract Type  | Verified Where    | `[[dynamic]]` Context Determined By  |
| :------------- | :---------------- | :----------------------------------- |
| Precondition   | Call site         | The call expression's context        |
| Postcondition  | Definition site   | The procedure's `[[dynamic]]` status |
| Type invariant | Enforcement point | The expression's context             |
| Loop invariant | Enforcement point | The enclosing scope's context        |

**Dynamic Semantics**

**Runtime Check Failure**

When a runtime-checked predicate evaluates to `false`:
1. The runtime MUST trigger a Panic
2. The panic payload MUST include predicate text, source location, and contract type
3. Normal panic propagation rules apply

**Diagnostics:** See Appendix H, Table H.1 (`P-SEM-2850`).


#### 14.7.2 Verification Facts

**Verification Fact.** A static guarantee that a predicate P holds at program location L. Used for static analysis, contract elision, and type narrowing.

A Verification Fact is a triple F(P, L, S) where:
- P is a predicate expression
- L is a program location (CFG node)
- S is the source of the fact (control flow, runtime check, or assumption)

**Static Semantics**

**Zero-Size Property**

Verification Facts:
- Have zero runtime size
- Have no runtime representation
- MUST NOT be stored in variables, passed as parameters, or returned

**Fact Dominance**

**(Fact-Dominate)**

F(P, L) ∈ Facts    L dom S    L ≠ S
────────────────────────────────────
P satisfied at S

**Fact Generation Rules**

| Construct                    | Generated Fact                  | Location             |
| :--------------------------- | :------------------------------ | :------------------- |
| `if P { ... }`               | F(P, _)                | Entry of then-branch |
| `if !P { } else { ... }`     | F(P, _)                | Entry of else-branch |
| `match x { Pat => ... }`     | F(x matches Pat, _)    | Entry of match arm   |
| Runtime check for P          | F(P, _)                | After check          |
| Loop invariant Inv at exit   | F(Inv, _)              | After loop           |


#### 14.7.3 Fact Propagation

**Static Semantics**

**Type Narrowing**

When a Verification Fact F(P, L) is active for binding x at location L:

typeof(x) -[F(P, L)]-> typeof(x) `where` {P}

**Dynamic Semantics**

**Dynamic Fact Injection**

When a `[[dynamic]]` scope requires a runtime check and no static fact dominates:

1. Identify requirement P at program point S
2. Construct check block: `if !P { panic("Contract violation: {P}") }`
3. Insert check into CFG such that it dominates S
4. Successful execution establishes F(P, exit(C))


#### 14.7.4 Small-Step and Big-Step Semantics

At each verification location (Table §14.7.1), any required runtime check is elaborated to `ContractCheck(P, k, s, ρ)` (with ρ determined by ContractKind) and inserted so that it dominates the guarded program point.

**Definitions**

Let `ContractKind = Pre | Post | TypeInv | LoopInv | ForeignPre | ForeignPost`.

Define the meta-expression:

ContractCheck(P, k, s, ρ) = `if` !P[ρ] { `panic`(ContractViolation(k, P, s)) }

where `P[ρ]` denotes capture-by-value substitution of `@result` and `@entry(expr)` with values in ρ. If `P` contains an intrinsic not bound in ρ, the program is ill-formed. `ContractViolation(k, P, s)` packages the predicate text, source location `s`, and contract kind `k`. The payload representation is Implementation-Defined but MUST include these fields.

**Contract Environments**

Let ρ_emptyset = ∅.

Let EntryCapture(f, σ_entry) be a map from each syntactically distinct `@entry(expr)` in `postcondition(f)` to its captured value, where each `expr` is evaluated in the entry state σ_entry using Γ_pre; if any capture panics, the panic propagates.

`EntryCapture` is computed once per procedure invocation and reused for all postcondition checks in that invocation.

For a procedure return at point r with returned value v_r:

- ρ_post = EntryCapture(f, σ_entry) ∪ {`@result` ↦ v_r}
- ρ_foreign_post = {`@result` ↦ v_r}

Then use the following environments:

| ContractKind | Environment |
| :----------- | :---------- |
| `Pre`        | ρ_emptyset |
| `Post`       | ρ_post |
| `TypeInv`    | ρ_emptyset |
| `LoopInv`    | ρ_emptyset |
| `ForeignPre` | ρ_emptyset |
| `ForeignPost`| ρ_foreign_post |

**Small-Step (Contract Check Machine)**

CheckState = {CheckStart(P, k, s, ρ, σ), CheckDone(σ), CheckPanic(σ)}

**(Check-True)**
Γ ⊢ EvalSigma(P[ρ], σ) ⇓ (Val(true), σ')
──────────────────────────────────────────────────────────────────────────────
⟨CheckStart(P, k, s, ρ, σ)⟩ → ⟨CheckDone(σ')⟩

**(Check-False)**
Γ ⊢ EvalSigma(P[ρ], σ) ⇓ (Val(false), σ')
───────────────────────────────────────────────────────────────────────────────
⟨CheckStart(P, k, s, ρ, σ)⟩ → ⟨CheckPanic(σ')⟩

**(Check-Panic)**
Γ ⊢ EvalSigma(P[ρ], σ) ⇓ (Ctrl(Panic), σ')
──────────────────────────────────────────────────────────────────────────────
⟨CheckStart(P, k, s, ρ, σ)⟩ → ⟨CheckPanic(σ')⟩

`CheckPanic` denotes a panic with payload `ContractViolation(k, P, s)` when the predicate evaluates to `false`, and it propagates any inner panic produced by evaluating `P`.

**Big-Step**

**(Check-Ok)**
⟨CheckStart(P, k, s, ρ, σ)⟩ →* ⟨CheckDone(σ')⟩
───────────────────────────────────────────────────────────────────────
Γ ⊢ ContractCheck(P, k, s, ρ, σ) ⇓ σ'

**(Check-Fail)**
⟨CheckStart(P, k, s, ρ, σ)⟩ →* ⟨CheckPanic(σ')⟩
───────────────────────────────────────────────────────────────────────
Γ ⊢ ContractCheck(P, k, s, ρ, σ) ⇑ panic


### 14.8 Behavioral Subtyping

#### 14.8.1 Liskov Substitution Principle

**Liskov Substitution.** When a type implements a class, procedure implementations MUST adhere to the Liskov Substitution Principle with respect to class-defined contracts.

**Static Semantics**

**Precondition Weakening**

An implementation MAY weaken (require less than) the preconditions defined in the class. An implementation MUST NOT strengthen (require more than) the preconditions.

**Postcondition Strengthening**

An implementation MAY strengthen (guarantee more than) the postconditions defined in the class. An implementation MUST NOT weaken (guarantee less than) the postconditions.

**Verification Strategy**

Behavioral subtyping constraints are verified **statically at compile-time**:

1. **Precondition Check**: Verify that the implementation's precondition logically implies the class's precondition
2. **Postcondition Check**: Verify that the class's postcondition logically implies the implementation's postcondition

No runtime checks are generated for behavioral subtyping; violations are structural errors.


#### 14.8.2 Contract Inheritance

*[REF: Behavioral subtyping constraints in §14.8.1 govern contract inheritance.]*


## 21. Foreign Function Interface

### 21.4 Foreign Contracts

#### 21.4.1 Foreign Preconditions

**Foreign Preconditions.** Conditions that callers must satisfy before invoking foreign procedures, specified using the `@foreign_assumes` clause.

**Syntax**

```ebnf
ffi_verification_attr ::= "[[" ffi_verification_mode "]]"
ffi_verification_mode ::= "static" | "dynamic" | "assume" | "trust"

foreign_contract ::= "|=" "@foreign_assumes" "(" predicate_list ")"

predicate_list   ::= predicate ("," predicate)*

predicate        ::= comparison_expr | null_check | range_check

null_check       ::= expression "!=" null_literal | expression "==" null_literal

range_check      ::= expression "in" range_expression
```

```cursive
extern "C" {
    procedure write(fd: c_int, buf: *imm u8, count: usize) -> isize
        |= @foreign_assumes(fd >= 0, buf != null, count > 0);
}
```

**Static Semantics**

**Predicate Context**

Predicates may reference:

- Parameter names from the procedure signature
- Literal constants
- Pure functions and operators
- Fields of parameter values (for record types)

Predicates MUST NOT reference:

- Global mutable state
- Values not in scope at the call site
- Effectful operations

**Verification Modes**

| Mode                   | Behavior                                                   |
| :--------------------- | :--------------------------------------------------------- |
| `[[static]]` (default) | Caller must prove predicates at compile time               |
| `[[dynamic]]`          | Runtime checks inserted before `unsafe` call               |
| `[[assume]]`           | Predicates assumed true without checks (optimization only) |

`[[static]]` uses `StaticProof` as defined in §14.7.1. `[[dynamic]]` inserts `ContractCheck(P, ForeignPre, s, ρ_emptyset)` immediately before the foreign call.

**Dynamic Semantics**

A failed `ForeignPre` check triggers a panic (`P-SEM-2860`).


#### 21.4.2 Foreign Postconditions

**Foreign Postconditions.** Conditions that foreign code guarantees upon successful return, specified using the `@foreign_ensures` clause. These are trusted assertions about foreign behavior.

**Syntax**

```ebnf
foreign_ensures ::= "@foreign_ensures" "(" ensures_predicate_list ")"

ensures_predicate_list ::= ensures_predicate ("," ensures_predicate)*

ensures_predicate ::= predicate
                    | "@error" ":" predicate
                    | "@null_result" ":" predicate
```

```cursive
extern "C" {
    procedure malloc(size: usize) -> *mut opaque c_void
        |= @foreign_assumes(size > 0)
        |= @foreign_ensures(@result != null implies aligned(@result, 16));

    procedure read(fd: c_int, buf: *mut opaque c_void, count: usize) -> isize
        |= @foreign_assumes(fd >= 0, buf != null)
        |= @foreign_ensures(@result >= -1, @result <= count as isize);
}
```

**Static Semantics**

**Predicate Bindings**

Postcondition predicates may reference:

- `@result`: The return value of the foreign procedure
- Parameter names (for checking output parameters)
- `@error`: Predicates that hold when the call fails
- `@null_result`: Predicates that hold when result is null

**Success, Error, and Null Classification**

Let U be the set of unconditional predicates, E the list of `@error` predicates, and N the list of `@null_result` predicates.

Define:

ErrCond =
 ⋀_(P ∈ E) P    if E ≠ ∅
 `false`        otherwise

NullCond = (`@result` == `null`)

SuccessCond = ¬ ErrCond

The foreign call is classified as an error iff `ErrCond` holds; otherwise it is classified as success.

Then the foreign postcondition obligations are:

1. For each P ∈ U, require SuccessCond ⇒ P
2. For each P ∈ E, require ErrCond ⇒ P
3. For each P ∈ N, require NullCond ⇒ P

`@null_result` predicates are well-formed only when the return type is a nullable pointer type; otherwise they are invalid (`E-SEM-2853`). A nullable pointer type is one of:
1. `Ptr<T>@Null`
2. `*imm T`
3. `*mut T`

`@error` predicates are well-formed only when the return type is not `()`. Using `@error` on a void-returning foreign procedure is ill-formed (`E-SEM-2855`).

**Verification Modes**

| Mode                   | Behavior                                                      |
| :--------------------- | :------------------------------------------------------------ |
| `[[static]]` (default) | Postconditions available as assumptions for downstream proofs |
| `[[dynamic]]`          | Runtime assertions after foreign call returns                 |
| `[[assume]]`           | Postconditions assumed without checks (optimization only)     |
| `[[trust]]`            | Postconditions trusted without runtime checks (audited code)  |

`[[static]]` uses `StaticProof` as defined in §14.7.1 with `SuccessCond` and `ErrCond` gating the obligations.

**Dynamic Semantics**

In `[[dynamic]]` mode, the implementation MUST evaluate `ErrCond` and `NullCond` in left-to-right predicate order and insert runtime checks enforcing the implications above immediately after the foreign call returns. Each inserted check is `ContractCheck(P, ForeignPost, s, ρ_foreign_post)`. A failed runtime check triggers a panic with payload `ContractViolation(ForeignPost, P, s)` at the call site (`P-SEM-2861`).


#### 21.4.3 Trust Boundaries

**Trust Boundaries.** Verification behavior definitions for foreign contracts, controlling the trade-off between safety guarantees and performance.

**Syntax**

```cursive
[[trust]]
extern "C" {
    procedure optimized_memcpy(dest: *mut u8, src: *imm u8, n: usize) -> *mut u8
        |= @foreign_assumes(dest != null, src != null)
        |= @foreign_ensures(@result == dest);
}
```

**Static Semantics**

**Trust Annotation**

The `[[trust]]` attribute on an extern block suppresses runtime checks for all contracts within that block. Postconditions are assumed true without verification.

**Safety Implications**

Incorrect postconditions under `[[trust]]` constitute Unverifiable Behavior (UVB). The programmer asserts that the foreign code satisfies declared contracts.

**Verification Hierarchy**

| Level         | Precondition Check | Postcondition Check      |
| :------------ | :----------------- | :----------------------- |
| `[[static]]`  | Compile-time proof | Available as assumptions |
| `[[dynamic]]` | Runtime assertion  | Runtime assertion        |
| `[[assume]]`  | No check           | No check                 |
| `[[trust]]`   | No check           | No check (trusted)       |


### H.1 Contract and Refinement Diagnostic Tables

| Code         | Severity | Detection    | Condition                                                   | Source  |
| ------------ | -------- | ------------ | ----------------------------------------------------------- | ------- |
| `E-TYP-1953` | Error    | Compile-time | Refinement not provable outside `[[dynamic]]` scope         | §13.7.3 |
| `E-TYP-1954` | Error    | Compile-time | Impure expression in refinement predicate                   | §13.7.2 |
| `E-TYP-1955` | Error    | Compile-time | Predicate does not evaluate to `bool`                       | §13.7.2 |
| `E-TYP-1956` | Error    | Compile-time | `self` used in inline parameter constraint                  | §13.7.2 |
| `E-TYP-1957` | Error    | Compile-time | Circular type dependency in refinement predicate            | §13.7.2 |
| `E-SEM-2801` | Error    | Compile-time | Contract predicate not provable outside `[[dynamic]]` scope | §14.7.1 |
| `E-SEM-2802` | Error    | Compile-time | Impure expression in contract predicate                     | §14.4.2 |
| `E-SEM-2803` | Error    | Compile-time | Implementation strengthens class precondition               | §14.8.1 |
| `E-SEM-2804` | Error    | Compile-time | Implementation weakens class postcondition                  | §14.8.1 |
| `E-SEM-2805` | Error    | Compile-time | `@entry()` result type not `Bitcopy` or `Clone`             | §14.5.4 |
| `E-SEM-2806` | Error    | Compile-time | `@result` used outside postcondition                        | §14.5.3 |
| `E-SEM-2820` | Error    | See §14.7    | Type invariant violated at construction                     | §14.5.1 |
| `E-SEM-2821` | Error    | See §14.7    | Type invariant violated at public entry                     | §14.5.1 |
| `E-SEM-2822` | Error    | See §14.7    | Type invariant violated at mutator return                   | §14.5.1 |
| `E-SEM-2823` | Error    | See §14.7    | Type invariant violated at private-to-public return         | §14.5.1 |
| `E-SEM-2824` | Error    | Compile-time | Public mutable field on type with invariant                 | §14.5.1 |
| `E-SEM-2830` | Error    | See §14.7    | Loop invariant not established at initialization            | §14.8.1 |
| `E-SEM-2831` | Error    | See §14.7    | Loop invariant not maintained across iteration              | §14.8.1 |
| `E-SEM-2850` | Error    | Compile-time | Cannot prove `@foreign_assumes` predicate                   | §21.4.1 |
| `E-SEM-2851` | Error    | Compile-time | Invalid predicate in foreign contract                       | §21.4.1 |
| `E-SEM-2852` | Error    | Compile-time | Predicate references out-of-scope value                     | §21.4.1 |
| `E-SEM-2853` | Error    | Compile-time | Invalid predicate in `@foreign_ensures`                     | §21.4.2 |
| `E-SEM-2854` | Error    | Compile-time | `@result` used in non-return context                        | §21.4.2 |
| `E-SEM-2855` | Error    | Compile-time | `@error` predicate on void-returning procedure              | §21.4.2 |
| `E-SEM-3004` | Error    | Compile-time | Impure expression in contract clause                        | §14.4.2 |
| `P-SEM-2850` | Panic    | Runtime      | Contract predicate failed at runtime                        | §14.7.1 |
| `P-TYP-1953` | Panic    | Runtime      | Refinement predicate failed at runtime                      | §13.7.3 |
| `P-SEM-2860` | Panic    | Runtime      | Foreign precondition failed at runtime                      | §21.4.1 |
| `P-SEM-2861` | Panic    | Runtime      | Foreign postcondition failed at runtime                     | §21.4.2 |
## 1. Lexical additions

### 1.1 Reserved keywords

C0X adds the following **reserved keywords** beyond the C0 baseline reserved set:

- `parallel`, `spawn`, `dispatch`, `where`

C0X also treats the identifier **lexeme** `wait` as a *contextual keyword* (recognized only in the `wait` expression form defined in §6.4).

### 1.2 Fixed identifier lexemes

The following identifier lexemes are **fixed** within specific syntactic positions (they are *not* globally reserved keywords):

- Key System: `read`, `write`, `dynamic`, `speculative`, `release`
- `parallel` options: `cancel`, `name`
- `spawn` options: `name`, `affinity`, `priority`
- `dispatch` options: `reduce`, `ordered`, `chunk`, and reduction operators `min`, `max`, `and`, `or`

An implementation MUST tokenize these as identifiers and disambiguate them by syntactic position.

---

## 2. Concrete syntax

This section gives *normative* EBNF for the constructs added by C0X. Nonterminals not defined here are inherited from Cursive0.md.

### 2.1 Attributes

```ebnf
attribute_list     ::= attribute+
attribute          ::= "[[" attribute_spec ("," attribute_spec)* "]]"
attribute_spec     ::= attribute_name ("(" attribute_args ")")?
attribute_name     ::= identifier | vendor_prefix "::" identifier
vendor_prefix      ::= identifier ("." identifier)*
attribute_args     ::= attribute_arg ("," attribute_arg)*
attribute_arg      ::= literal
                    | identifier
                    | identifier ":" literal
                    | identifier "(" attribute_args ")"

expression         ::= attributed_expr | unattributed_expr
attributed_expr    ::= attribute_list expression
```

**Attachment sites.** An `attribute_list` MAY appear:

- Immediately before any *top-level* or *member* declaration.
- As an *expression prefix* (`attributed_expr`).

### 2.2 Generics

```ebnf
generic_params         ::= "<" type_param (";" type_param)* ">"
type_param             ::= identifier type_bounds? type_default?
type_bounds            ::= "<:" bound_list
bound_list             ::= class_bound ("," class_bound)*
class_bound            ::= type_path generic_args?

where_clause            ::= "where" where_predicate_list
where_predicate_list    ::= where_predicate (predicate_separator where_predicate)* predicate_separator?
predicate_separator     ::= terminator
where_predicate         ::= identifier type_bounds

generic_args            ::= "<" type_arg_list ">"
type_arg_list           ::= type ("," type)*

qualified_type_expr     ::= type_path ("::" generic_args)?
```

Notes:

- `qualified_type_expr` enables “turbofish” instantiation at expression sites (e.g., `Foo:: <T>(...)`).
- `where_clause` uses `terminator` separators (newline/semicolon) to avoid ambiguity in nested generic contexts.

### 2.3 Imports and `using`

```ebnf
import_decl ::= visibility? "import" module_path ("as" identifier)? terminator

using_decl      ::= visibility? "using" using_clause terminator
using_clause    ::= using_path ("as" identifier)?
                 | using_path "::" using_list
                 | using_path "::" "*"
using_path      ::= module_path ("::" identifier)?
using_list      ::= "{" using_specifier ("," using_specifier)* ","? "}"
using_specifier ::= identifier ("as" identifier)?
                 | "self" ("as" identifier)?
```

### 2.4 Shared permission and receiver shorthand

C0X makes the `shared` permission fully supported in all syntactic locations where C0 parses it.

Receiver shorthand is extended as follows:

```ebnf
receiver_shorthand ::= "~" | "~!" | "~%"
```

- `~`   abbreviates `self: const Self`
- `~!`  abbreviates `self: unique Self`
- `~%`  abbreviates `self: shared Self`

### 2.5 Full classes

This section defines the extended class syntax (associated types and abstract states).

```ebnf
class_decl ::=
    attribute_list? visibility?
    "modal"?
    "class" identifier generic_params?
    ("<:" superclass_bounds)?
    where_clause?
    "{" class_item* "}"

superclass_bounds ::= class_bound ("+" class_bound)*

class_item ::=
      class_method_decl
    | associated_type_decl
    | abstract_field_decl
    | abstract_state_decl

class_method_decl ::=
    attribute_list?
    visibility?
    "procedure" identifier generic_params?
    "(" receiver ("," param)* ")"
    ("->" type)?
    contract_clause?
    (block_expr | terminator)

associated_type_decl ::=
    attribute_list?
    visibility?
    "type" identifier ("=" type)? terminator

abstract_field_decl ::=
    attribute_list?
    visibility?
    key_boundary?
    identifier ":" type terminator

abstract_state_decl ::=
    attribute_list?
    visibility?
    "@" identifier "{" abstract_field_decl* "}" terminator?

key_boundary ::= "#"
```

### 2.6 Key System

Key System constructs extend statement and member grammars.

#### 2.6.1 Key blocks

```ebnf
key_block_stmt ::= "#" key_path_list key_block_mod* key_mode? block_expr

key_path_list  ::= key_path_expr ("," key_path_expr)*

key_block_mod  ::= "dynamic" | "speculative" | "release"

key_mode       ::= "read" | "write"
```

#### 2.6.2 Key paths and coarsening markers

```ebnf
key_path_expr  ::= key_root key_seg*
key_root       ::= identifier
key_seg        ::= ("." key_field) | ("[" key_index "]")

key_field      ::= key_marker? identifier
key_index      ::= key_marker? expression

key_marker     ::= "#"
```

#### 2.6.3 Type-level boundaries

Record/enum/modal field declarations are extended with an optional boundary marker:

```ebnf
record_field_decl ::= attribute_list? visibility? key_boundary? identifier ":" type terminator
```

(The same `key_boundary` marker is admitted for enum payload fields and modal state payload fields.)

### 2.7 Structured parallelism

```ebnf
parallel_block    ::= "parallel" domain_expr block_options? block_expr

domain_expr       ::= expression

block_options     ::= "[" block_option ("," block_option)* "]"
block_option      ::= "cancel" ":" expression
                   | "name" ":" string_literal

spawn_expr        ::= "spawn" spawn_option_list? block_expr
spawn_option_list ::= "[" spawn_option ("," spawn_option)* "]"
spawn_option      ::= "name" ":" string_literal
                   | "affinity" ":" expression
                   | "priority" ":" expression

wait_expr         ::= "wait" expression

dispatch_expr     ::= "dispatch" pattern "in" range_expression
                      key_clause?
                      dispatch_option_list?
                      block_expr

key_clause        ::= "key" key_path_expr key_mode

dispatch_option_list ::= "[" dispatch_option ("," dispatch_option)* "]"

dispatch_option   ::= "reduce" ":" reduce_op
                   | "ordered"
                   | "chunk" ":" expression

reduce_op         ::= "+" | "*" | "min" | "max" | "and" | "or" | identifier
```

`spawn_expr`, `dispatch_expr`, and `wait_expr` are expressions (and thus may occur wherever an expression may occur), but are constrained by well-formedness rules (§6.2).

### 2.8 Contracts and invariants

```ebnf
contract_clause    ::= "|=" contract_body
contract_body      ::= precondition_expr
                     | precondition_expr "=>" postcondition_expr
                     | "=>" postcondition_expr

precondition_expr  ::= predicate_expr
postcondition_expr ::= predicate_expr
predicate_expr     ::= logical_or_expr
contract_intrinsic ::= "@result" | "@entry" "(" expression ")"
```

Contract clauses MUST appear after the return type annotation (or after the parameter list if no return type) and before the procedure body.

```ebnf
type_invariant ::= "where" "{" predicate_expr "}"
loop_invariant ::= "where" "{" predicate_expr "}"
```

Loop expressions admit an optional `loop_invariant` between any loop condition and the loop body:

```ebnf
loop_expression ::= "loop" loop_condition? loop_invariant? block_expr
```

Type invariants appear immediately after a `record`, `enum`, or `modal` declaration.

---

## 3. Abstract syntax

This section defines the AST extensions (in the style of the C0 baseline AST definitions).

### 3.1 Attributes

Let:

- `AttrName` be a sequence of identifiers separated by `.` and `::` (as per §2.1).
- `AttrArg` be a literal, an identifier, a named literal, or a nested attribute-arg application (per §2.1).

Define:

- `AttributeSpec ::= Attr(name: AttrName, args: [AttrArg])`
- `AttributeList ::= [AttributeSpec]` (non-empty list)

Every declaration node and every expression node is extended with an `attrs: AttributeList?` field.

### 3.2 Generic parameters and instantiation

Define:

- `Variance ::= Covariant | Contravariant | Invariant | Bivariant`
- `TypeParam ::= (name: Ident, bounds: [ClassBound], default: Type?, variance: Variance)`
- `GenericParams ::= [TypeParam]`
- `GenericArgs ::= [Type]`
- `WherePred ::= (name: Ident, bounds: [ClassBound])`
- `WhereClause ::= [WherePred]`

### 3.3 Imports

Define:

- `ImportDecl ::= (vis: Vis, path: ModulePath, alias: Ident?)`
- Extend `TopLevelItem` with `ImportDecl`.

### 3.4 Key System

Define:

- `KeyMode ::= Read | Write`
- `KeyBlockMod ::= Dynamic | Speculative | Release`

Key paths:

- `KeySeg ::= Field(marked: Bool, name: Ident) | Index(marked: Bool, expr: Expr)`
- `KeyPathExpr ::= (root: Ident, segs: [KeySeg])`

Key blocks:

- `KeyBlockStmt ::= (paths: [KeyPathExpr], mods: [KeyBlockMod], mode: KeyMode?, body: BlockExpr)`

Boundary marker:

- Record/enum/state payload fields extend with `boundary: Bool`.

### 3.5 Concurrency

Add expression forms:

- `ParallelExpr(domain: Expr, opts: [ParallelOpt], body: BlockExpr)`
- `SpawnExpr(opts: [SpawnOpt], body: BlockExpr)`
- `DispatchExpr(pat: Pattern, range: Expr, key_clause: (KeyPathExpr, KeyMode)?, opts: [DispatchOpt], body: BlockExpr)`
- `WaitExpr(handle: Expr)`

### 3.6 Contracts and invariants

Define:

- `ContractClause ::= (pre: Expr, post: Expr)`
- `Invariant ::= Expr`

Extend:

- `ProcedureDecl` with `contract: ContractClause?`
- `RecordDecl`, `EnumDecl`, `ModalDecl` with `invariant: Invariant?`
- `LoopExpr` with `invariant: Invariant?`

Normalization:

- Absent contract clause desugars to `pre = true`, `post = true`.
- `|= P` desugars to `pre = P`, `post = true`.
- `|= P => Q` desugars to `pre = P`, `post = Q`.
- `|= => Q` desugars to `pre = true`, `post = Q`.

---

## 4. Permission system extension: `shared`

C0X adopts the permission lattice and coercion rules from the original design and integrates them into the C0 baseline.

### 4.1 Permission lattice

Let `Perm = { unique, shared, const }`.

Define the partial order `PermSub` (“permission-subtype”) as:

- `unique PermSub shared`
- `shared PermSub const`
- Reflexive and transitive closure

Equivalently:

```
unique <: shared <: const
```

### 4.2 Permission-qualified type subtyping

Let `TypePerm(p, T)` denote the permission-qualified type in the C0 baseline (Cursive0.md).

Define:

p PermSub q
────────────────────────────
p T <: q T    (T-Perm-Sub)

All other subtyping rules are inherited from Cursive0.md.

### 4.3 Receiver permissions

A method declared with receiver permission `p_m` is callable through a path with permission `p_c` iff:

p_c PermSub p_m

In particular:

- `const` callers may call only `~` receivers.
- `shared` callers may call `~` and `~%` receivers.
- `unique` callers may call `~`, `~%`, and `~!` receivers.

### 4.4 Unique downgrade state machine

C0X adopts the binding state model from the design:

- A `unique` binding is **Active** when no downgraded references are live.
- It becomes **Inactive** for the duration of any downgrade to `const` or `shared`.

Formally, for a unique binding `b`:

- `Active(b)` and `Inactive(b)` are mutually exclusive.
- A downgrade operation introduces `Inactive(b)` until the downgrade scope ends.
- While `Inactive(b)`, `b` MUST NOT be read, written, moved, or re-downgraded.

This state machine integrates with the C0 baseline’s move/borrow responsibility rules.

---

## 5. Attributes

### 5.1 Attribute registry and well-formedness

An implementation MUST maintain a registry of supported attributes.

Define judgment:

Γ ⊢ AttrList(A) wf@τ

meaning: attribute list `A` is well-formed for attachment target kind τ (declaration kind or expression kind).

Well-formedness requires:

1. Every attribute name in `A` is registered.
2. The target kind τ is permitted for the attribute.
3. Attribute arguments are syntactically valid and satisfy the attribute’s argument schema.

Unknown attribute names are a hard error.

### 5.2 `[[static_dispatch_only]]`

**Meaning.** Excludes a class procedure from dynamic dispatch via `$Class`.

- Target: class procedure declarations and class procedure implementations.
- Arguments: none.

Let `StaticOnly(p)` hold iff `p` has this attribute.

Constraint:

- If `StaticOnly(p)` and `p` is called on a `$Class` value, the program is ill-typed.

### 5.3 `[[dynamic]]`

**Meaning.** Permits runtime synchronization in the Key System where static key-disjointness cannot be proven.

- Target: expressions and blocks.
- Arguments: none.

Let `DynamicScope(e)` hold iff `e` is under a `[[dynamic]]`-attributed expression prefix.

Effect:

- In `DynamicScope`, Key System checks that would otherwise be compile-time errors MAY instead be discharged by inserting runtime synchronization (see §7.6).

### 5.4 Memory-ordering attributes for shared accesses

C0X supports the following memory-ordering attributes when attached to a statement or expression that performs shared memory access:

- `[[relaxed]]`, `[[acquire]]`, `[[release]]`, `[[acqrel]]`, `[[seqcst]]`

Semantics are specified in §7.7.

---

## 6. Generics

C0X introduces generics via monomorphization (compile-time specialization).

### 6.1 Generic environments

Let Δ be a type-parameter environment mapping type parameter identifiers to `(bounds, default, variance)`.

Typing judgments are extended to carry Δ implicitly (or explicitly, depending on the baseline judgment form):

- When type-checking inside a generic item, Δ includes that item’s type parameters.
- `where` predicates introduce additional bounds constraints into Δ.

### 6.2 Well-formedness of generic parameter lists

Judgment:

Γ ⊢ GenParams(G) wf ▷ Δ

Rules:

- Parameter names are unique.
- Each bound `T <: Cl` is well-formed and refers to a class `Cl`.
- Each default type (if present) is well-formed under the bounds of previously-declared parameters.
- Variance is computed after type-checking the generic item body (see §6.5).

### 6.3 Well-formedness of instantiation arguments

Judgment:

Γ; Δ ⊢ GenArgs(A) ok for G ▷ θ

Where θ is a substitution mapping each parameter name in `G` to a concrete type.

Rules:

1. If `A` provides an argument for each parameter, θ is the pointwise mapping.
2. If fewer arguments are provided, missing arguments are filled by defaults; if any missing parameter lacks a default, the instantiation is ill-formed.
3. For each parameter `X` with bounds `B`, θ(X) MUST satisfy all bounds (see §6.4).

### 6.4 Bound satisfaction

Let Implements(Γ, T, Cl) denote the class-implementation judgment already present in the C0 baseline for classes.

Define:

Γ ⊢ T ⊨ (Cl_1 + … + Cl_n) ⇔ ∀ i. Implements(Γ, T, Cl_i)

### 6.5 Variance

C0X adopts variance definitions and the generic subtyping rule from the design.

#### 6.5.1 Variance classification

For a type parameter `X`, its variance is one of:

- `+` (Covariant)
- `-` (Contravariant)
- `=` (Invariant)
- `±` (Bivariant)

Variance is computed from **usage** within a type definition (records, enums, modals) and from **method signatures** exposed under a given permission view.

Define two variance views for a nominal type constructor `N`:

- `VarMut(N, X)` — variance observable through `shared/unique` access.
- `VarConst(N, X)` — variance observable through `const` access.

**Contribution rules (informal but normative):**

- Occurrence of `X` in a **mutable storage position** (any field/state payload reachable through `shared` or `unique`) contributes `Invariant`.
- Occurrence of `X` in an **immutable storage position** (reachable only through `const`) contributes `Covariant`.
- Occurrence of `X` in a **parameter type** of a callable procedure contributes `Contravariant`.
- Occurrence of `X` in a **return type** contributes `Covariant`.

Method inclusion by view:

- For `VarConst`, only procedures callable via a `const` receiver contribute.
- For `VarMut`, all procedures callable via `shared` or `unique` receivers contribute.

Combining contributions:

- If no occurrences: `Bivariant`.
- If only covariant: `Covariant`.
- If only contravariant: `Contravariant`.
- Otherwise: `Invariant`.

#### 6.5.2 Generic subtyping

For a nominal type constructor `C` with type arguments `A_i` and `B_i`:

∀ i. Var(C, i) = + ⇒ A_i <: B_i    Var(C, i) = - ⇒ B_i <: A_i    Var(C, i) == ⇒ A_i = B_i
────────────────────────────────────────────────────────────────────────────────────────────────────────────────
C⟨A_1, …, A_n⟩ <: C⟨B_1, …, B_n⟩    (T-Gen-Sub)

Where `Var(C,i)` is selected by permission view:

- If the *outer* permission of the type is `const`, use `VarConst(C,i)`.
- If the outer permission is `shared` or `unique`, use `VarMut(C,i)`.

This captures the design property that `const` enables covariance for otherwise invariant generic types when invariance arises from mutability rather than observable contravariance.

### 6.6 Monomorphization

C0X defines generics by elaboration to a monomorphic program.

Let `Instantiate(d, θ)` denote substitution of type parameters in a declaration `d`.

**Instantiation graph.** The compiler maintains a set `Inst` of demanded instantiations `(d, θ)` discovered from:

- Type applications `N<...>`
- Procedure calls with explicit generic args (turbofish)
- Type inference of generic calls

**Algorithm (normative):**

1. Initialize `Inst` with all concrete uses from the program’s entry module.
2. While `Inst` has an unprocessed pair `(d, θ)`:
   1. Produce `d' = Instantiate(d, θ)`.
   2. Type-check `d'` with no remaining type parameters.
   3. Scan `d'` for further generic uses and add them to `Inst`.
3. The final monomorphic program is the set of all produced `d'`.

**Termination.** Recursive instantiation cycles are permitted only if they correspond to well-founded recursion through runtime values; non-terminating type-level recursion is ill-formed.

---

## 7. Shared permission and the Key System

C0X adopts the Key System from the original design, with minor alignment to the C0 baseline terminology.

### 7.1 Keys

A **key** is a triple:

K = (P, M, S)

- `P` is a key path
- `M ∈ {Read, Write}` is a mode
- `S` is a scope identifier

### 7.2 Key paths

Key paths are derived from place expressions over `shared` values.

A key path is a sequence of segments:

P = (r, s_1, …, s_k)

where `r` is a root identifier (or boundary root) and each segment is either:

- field access `.f`
- indexing `[i]`

A segment may be **marked** by the coarsening marker `#`, which truncates the key path at that segment.

### 7.3 Key contexts

A task carries a key context 𝒦 (set of held keys).

Define:

- `Covers(𝒦, (P, Read))` iff 𝒦 contains a key `(P', M', S')` such that `P'` is a prefix of `P` and `M'` is at least `Read`.
- `Covers(𝒦, (P, Write))` iff 𝒦 contains `(P', Write, S')` with `P'` prefix of `P`.

### 7.4 Implicit acquisition

When evaluating a `shared` access requiring `(P, M)`, the runtime:

- If `Covers(𝒦, (P, M))`, no action is taken.
- Otherwise, acquires key `(P, M, S_current)` and adds it to 𝒦.

The compiler MUST ensure (statically or via runtime synchronization in `[[dynamic]]` scope) that concurrent tasks do not acquire conflicting keys.

### 7.5 Conflicts and disjointness

Two path-mode pairs conflict if they overlap and at least one is a write:

Conflict((P_1, M_1), (P_2, M_2)) ⇔ Overlap(P_1, P_2) ∧ (M_1 = Write ∨ M_2 = Write)

`Overlap` is defined by prefix overlap on normalized paths, with type-level boundaries (§7.8) treated as root separators.

### 7.6 Key blocks

A key block `# paths mods mode { body }` pre-acquires a set of keys for the lexical extent of `body`.

Let `Mode(mode?)` be:

- `Write` if explicit `write`
- `Read` otherwise

Let `Mods` contain any of `{Dynamic, Speculative, Release}`.

#### 7.6.1 Static semantics of key blocks

Entering a non-`release` key block extends the key context:

- Acquire each key `(P_i, Mode, S_body)` for each listed path `P_i`.
- Acquisition order is canonical (lexicographic on normalized path segments) to avoid deadlock.

`release` blocks shorten the scope of keys (see §7.6.3).

#### 7.6.2 Dynamic vs static checking

- Outside `Dynamic` scope, any key acquisition that depends on non-constant indices or otherwise cannot be proven disjoint is a compile-time error.
- Inside `Dynamic` scope (or under `[[dynamic]]`), the compiler MAY lower the acquisition to runtime locks.

#### 7.6.3 Release blocks

A `release` key block causes keys acquired in an outer scope to be released (logically) for the duration of the inner block and reacquired after.

The key mode of the `release` block MUST match the outer mode for the released paths.

### 7.7 Memory ordering

When runtime synchronization is used (dynamic key blocks or runtime key derivation), acquisition and release operations are atomic operations over an implementation-defined lock table.

Memory ordering for these operations is controlled by attributes:

- `[[relaxed]]` → relaxed ordering
- `[[acquire]]` → acquire on lock acquisition
- `[[release]]` → release on lock release
- `[[acqrel]]` → acquire-release
- `[[seqcst]]` → sequentially consistent

Absent an attribute, the default is `[[acqrel]]`.

### 7.8 Type-level boundaries

A field marked with `#` in a record/enum/modal state payload is a **type-level boundary**.

When deriving a key path, traversal MUST stop at a boundary field; the boundary field becomes the last segment included in the key path.

This prevents deep structural locking across encapsulation boundaries.

### 7.9 `shared $Class` restriction

Let `DynMethods(Cl)` be the set of procedures callable via vtable dispatch on `$Cl` (vtable-eligible and not `[[static_dispatch_only]]`).

A `shared $Cl` value is well-formed iff:

- Every method in `DynMethods(Cl)` is callable with receiver `~` only (i.e., none require `~%` or `~!`).

Otherwise `shared $Cl` is ill-formed.

---

## 8. Full classes

C0X extends classes beyond the C0 baseline by adding:

- generic parameters and `where` clauses on classes
- associated types (abstract or with defaults)
- abstract states for modal classes
- dispatchability defined in terms of vtable eligibility and `[[static_dispatch_only]]`

### 8.1 Class declaration well-formedness

Judgment:

Γ ⊢ ClassDecl(Cl) wf

A class declaration is well-formed iff:

1. Its generic params and `where` clause are well-formed (§6.2).
2. All superclass bounds are well-formed and acyclic.
3. Names are disjoint across: procedures, associated types, abstract fields, abstract states.
4. In a modal class, abstract states are permitted; in a non-modal class, abstract states MUST be empty.

### 8.2 Implementation completeness

A type `T` implements a class `Cl` (written `T <: Cl`) if:

- For every abstract procedure in `Cl`, `T` provides an implementation.
- For every abstract associated type in `Cl`, `T` provides a binding.
- For every abstract field in `Cl`, `T` has a compatible field.
- For every abstract state in `Cl`, `T` has a compatible state (and is modal).

### 8.3 Override rules

- Implementing an **abstract** procedure: `override` MUST NOT be used.
- Overriding a **concrete** class procedure: `override` MUST be used.

### 8.4 Coherence and orphan rules

- A type may implement a class at most once.
- For `T <: Cl`, at least one of `T` or `Cl` MUST be defined in the current assembly.

### 8.5 Dispatchability and vtable eligibility

A procedure is **vtable-eligible** iff all hold:

1. It has a receiver.
2. It has **no** generic type parameters.
3. It does not use `Self` by value in parameter or return types (except behind pointer indirection).
4. It is not marked `[[static_dispatch_only]]`.

A class is **dispatchable** iff every procedure is either vtable-eligible or marked `[[static_dispatch_only]]`.

---

## 9. Module importing

C0X adds `import` and extends `using` to match the original design.

### 9.1 Assemblies and modules

A program consists of an **assembly graph**. Each assembly contains modules; modules contain items.

- `public` items are visible to dependent assemblies.
- `internal` items are visible only within the defining assembly.
- `private` items are visible only within the defining module.

### 9.2 `import`

An `import` declaration:

1. Establishes a dependency edge from the current assembly to the imported assembly.
2. Makes the imported module’s namespace available for **qualified** access.
3. Does **not** introduce unqualified names.
4. Introduces a local module alias (either explicit via `as`, or the last path component).

### 9.3 `using`

`using` introduces names into the current module scope.

Variants:

- Item using: `using path::Item [as Alias]`
- List using: `using path::{A, B, ...}`
- Self using: `using path::{self, A, ...}`
- Wildcard using: `using path::*`

Name conflicts are forbidden.

### 9.4 Re-exports

`public using ...` re-exports items.

Constraint:

- A `public using` may re-export only `public` items.

---

## 10. Structured concurrency

C0X adopts structured parallelism semantics with Key System integration.

### 10.1 Parallel blocks

Typing rule:

Γ ⊢ D : `$ExecutionDomain`    Γ_P ⊢ B : T
────────────────────────────────────────────────
Γ ⊢ `parallel` D {B} : T    (T-Parallel)

where Γ_P extends Γ with the parallel context.

Within a parallel block:

- `spawn` and `dispatch` expressions are permitted.
- Each spawned task has its own Key System context (keys are task-local).

### 10.2 Spawn

Typing:

Γ[parallel_context] = D    Γ_cap ⊢ e : T
──────────────────────────────────────────────────────
Γ ⊢ `spawn` {e} : SpawnHandle⟨T⟩    (T-Spawn)

`SpawnHandle<T>` is a modal type with states `@Pending` and `@Ready`.

### 10.3 Wait

Typing:

Γ ⊢ h : SpawnHandle⟨T⟩
──────────────────────────────────────────
Γ ⊢ `wait` h : T    (T-Wait)

**Key restriction.** No keys may be held across a `wait` suspension point. Therefore, `wait` is well-formed only when the current key context is empty.

### 10.4 Dispatch

Typing without reduction:

Γ ⊢ range : Range⟨I⟩    Γ, i:I ⊢ B : T
──────────────────────────────────────────────────────────
Γ ⊢ `dispatch` i `in range` {B} : ()    (T-Dispatch)

Typing with reduction:

Γ ⊢ range : Range⟨I⟩    Γ, i:I ⊢ B : T    Γ ⊢ ⊕ : (T, T) → T
─────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ `dispatch` i `in range [reduce: ⊕]` {B} : T    (T-Dispatch-Reduce)

Parallelism is determined by key disjointness; where static disjointness cannot be proven, dynamic synchronization may be used only under `[[dynamic]]`.

### 10.5 Panic and cancellation

- Panics in spawned tasks are captured and rethrown at the parallel block boundary after all work settles.
- Cancellation is cooperative; the runtime MAY request cancellation on first panic.

---

## 11. Dynamic semantics overview

### 11.1 Generics

Generics are elaborated away by monomorphization (§6.6). The runtime semantics is that of the resulting monomorphic program.

### 11.2 Key System

- In statically proven contexts, key operations may be erased.
- In dynamic contexts, key acquisition/release is lowered to runtime lock operations with memory ordering (§7.7).

### 11.3 Parallel execution

A `parallel` block:

1. Evaluates its domain.
2. Creates a worker pool.
3. Executes the block body, enqueuing work on `spawn`/`dispatch`.
4. Joins at block end, propagating panics as specified.

### 11.4 Contracts and invariants

- Contract and invariant predicates are verified statically by default.
- Within `[[dynamic]]` scopes, runtime checks are inserted per Appendix H (§14.7).
- Runtime checks are elaborated into ordinary control flow; dynamic semantics follow the Cursive0 evaluation rules for the inserted constructs.

---

## 12. Conformance checklist

An implementation conforms to C0X iff:

- It supports `shared` with the permission lattice and receiver rules (§4).
- It implements the Key System semantics (§7), including boundary markers and key blocks.
- It supports attribute parsing and enforces semantics for `[[dynamic]]`, `[[static_dispatch_only]]`, and memory-order attributes (§5).
- It supports generics with bounds, defaults, `where` clauses, variance, and monomorphization (§6).
- It supports full classes with associated types and abstract states (§8).
- It supports `import` and full `using` semantics (§9).
- It supports `parallel`, `spawn`, `dispatch`, and `wait` semantics (§10) with Key System interaction.
- It supports procedure contracts, type invariants, and loop invariants with static verification and `[[dynamic]]` runtime checks (Appendix H, §§14.4–14.7).
- It supports refinement types and refinement verification (Appendix E, §13.7).
- It supports foreign contract clauses and verification modes for `extern` procedures when FFI is used (Appendix H, §21.4).

---

# Appendices: Full Formalization Extracts

The main body of this document integrates C0X into the C0 baseline and provides the consolidated grammars for the added constructs. The appendices below provide **complete, formal, and normative** definitions (judgements, inference rules, and semantics) for the requested feature areas, aligned with the original design document. Appendix H (Contracts and Verification) appears immediately after §0.3; Appendices A–G follow below.

If any statement in the main body conflicts with an appendix, the appendix is authoritative.


# Appendix A: Module Importing (Normative)

The following section is incorporated from the original design and is normative for C0X.

### 6.4 Import and Using Declarations


#### 6.4.1 Import Declaration Syntax

**Import Declaration.** Declares a dependency on a module from an external assembly, making that module's public items available for qualified access.

**Syntax**

```ebnf
import_decl ::= "import" module_path ("as" identifier)?
```

```cursive
import core::io
import core::collections as col
```

**Static Semantics**

An `import` declaration:

1. Establishes a dependency edge from the current assembly to the imported assembly
2. Makes the imported module's namespace available for qualified access
3. Does **NOT** introduce any names into the current scope's unqualified namespace
4. If `as alias` is specified, the alias becomes the local name for the module; otherwise, the last component of the module path is used

**Import Requirement for External Access**

For modules in **external assemblies**, an `import` declaration MUST precede any `using` declaration that references items from that assembly.

For modules in the **same assembly** (intra-assembly access), `using` declarations MAY directly reference module paths without a preceding `import`.

**Diagnostics:** See Appendix A, codes `E-MOD-1201`, `E-MOD-1202`.


#### 6.4.2 Using Declaration Variants (Item, List, Self, Wildcard)

**Using Declaration.** Creates a scope alias that brings items into the current scope, enabling unqualified access to otherwise qualified names.

**Syntax**

```ebnf
using_decl      ::= visibility? "using" using_clause
using_clause    ::= using_path ("as" identifier)?
                 | using_path "::" using_list
                 | using_path "::" "*"
using_path      ::= module_path ("::" identifier)?
using_list      ::= "{" using_specifier ("," using_specifier)* ","? "}"
using_specifier ::= identifier ("as" identifier)?
                 | "self" ("as" identifier)?
```

```cursive
using core::io::File
using core::io::File as FileHandle
using core::collections::{Vec, HashMap}
using core::io::{self, File, Reader}
using core::prelude::*
```

**Static Semantics**

**Item Using** (`using path::Item`)

Resolves `Item` and introduces a binding with its original name (or the alias if `as alias` is specified).

**List Using** (`using path::{A, B, C}`)

Resolves each item in the list and introduces bindings for all of them. Each item in the list MUST be unique.

**Self Using** (`using path::{self, A}`)

The special specifier `self` in a using list introduces a binding for the module path itself, enabling qualified access via the module name alongside unqualified access to other items.

**Wildcard Using** (`using path::*`)

Brings all accessible (public or internal, depending on assembly relationship) items from the target module into the current scope. Each item retains its original name.

**(WF-Using-Item)**

Γ ⊢ resolve_item(path::i) ⇒ item    Γ ⊢ can_access(item)    name ∉ dom(Γ)
───────────────────────────────────────────────────────────────────────────────
Γ ⊢ using path::i as name : WF

**(WF-Using-Wildcard)**

Γ ⊢ resolve_module(path) ⇒ m    ∀ i ∈ accessible(m), name(i) ∉ dom(Γ)
───────────────────────────────────────────────────────────────────────────────
Γ ⊢ using path::* : WF

**Diagnostics:** See Appendix A, codes `E-MOD-1204`, `E-MOD-1206`, `W-MOD-1201`.


#### 6.4.3 Re-export Semantics

**Re-export.** A `public using` declaration re-exports an imported item, making it available to modules that depend on the current assembly.

**Syntax**

```cursive
public using core::io::File
```

**Static Semantics**

A `public using` declaration:

1. Performs the same resolution as a regular `using`
2. Additionally marks the introduced binding as `public`, making it visible to external dependents
3. The source item MUST itself be `public`; re-exporting `internal` or `private` items is forbidden

**(WF-Using-Public)**

Γ ⊢ using path::i as name : WF    visibility(item) = public
───────────────────────────────────────────────────────────────────────────────
Γ ⊢ public using path::i as name : WF

**Diagnostics:** See Appendix A, code `E-MOD-1205`.


#### 6.4.4 Name Conflict Prevention

**Static Semantics**

Names introduced by `import as` or `using` declarations MUST NOT conflict with existing names in the same scope.

**Diagnostics:** See Appendix A, code `E-MOD-1203`.

---

## 7. Attributes and Metadata

---

### 7.1 Attribute Syntax and Registry


#### 7.1.1 Attribute Syntax

**Attribute.** A compile-time annotation attached to a declaration or expression that influences code generation, memory layout, diagnostics, verification strategies, and interoperability.

An attribute A is defined by:

A = (Name, Args)

where Name is an identifier and Args is a (possibly empty) sequence of arguments.

An **attribute list** is a sequence of one or more attributes:

AttributeList = ⟨A_1, A_2, …, A_n⟩

**Syntax**

```ebnf
attribute_list     ::= attribute+
attribute          ::= "[[" attribute_spec ("," attribute_spec)* "]]"
attribute_spec     ::= attribute_name ("(" attribute_args ")")?
attribute_name     ::= identifier | vendor_prefix "::" identifier
vendor_prefix      ::= identifier ("." identifier)*
attribute_args     ::= attribute_arg ("," attribute_arg)*
attribute_arg      ::= literal
                     | identifier
                     | identifier ":" literal
                     | identifier "(" attribute_args ")"
expression         ::= attributed_expr | unattributed_expr
attributed_expr    ::= attribute_list expression
```

```cursive
[[attr1, attr2]]
declaration

[[attr1]]
[[attr2]]
declaration
```

**Static Semantics**

An attribute list MUST appear immediately before the declaration or expression it modifies. Multiple attributes MAY be specified in a single block or in separate blocks; the forms are semantically equivalent.

# Appendix B: Attributes and Metadata (Normative)

This appendix defines the attribute syntax and the semantics of non-comptime attributes. Subsections that define comptime/metaprogramming behavior (e.g., `[[reflect]]`) are intentionally omitted from this extension.

## 7. Attributes and Metadata

---

### 7.1 Attribute Syntax and Registry


#### 7.1.1 Attribute Syntax

**Attribute.** A compile-time annotation attached to a declaration or expression that influences code generation, memory layout, diagnostics, verification strategies, and interoperability.

An attribute A is defined by:

A = (Name, Args)

where Name is an identifier and Args is a (possibly empty) sequence of arguments.

An **attribute list** is a sequence of one or more attributes:

AttributeList = ⟨A_1, A_2, …, A_n⟩

**Syntax**

```ebnf
attribute_list     ::= attribute+
attribute          ::= "[[" attribute_spec ("," attribute_spec)* "]]"
attribute_spec     ::= attribute_name ("(" attribute_args ")")?
attribute_name     ::= identifier | vendor_prefix "::" identifier
vendor_prefix      ::= identifier ("." identifier)*
attribute_args     ::= attribute_arg ("," attribute_arg)*
attribute_arg      ::= literal
                     | identifier
                     | identifier ":" literal
                     | identifier "(" attribute_args ")"
expression         ::= attributed_expr | unattributed_expr
attributed_expr    ::= attribute_list expression
```

```cursive
[[attr1, attr2]]
declaration

[[attr1]]
[[attr2]]
declaration
```

**Static Semantics**

An attribute list MUST appear immediately before the declaration or expression it modifies. Multiple attributes MAY be specified in a single block or in separate blocks; the forms are semantically equivalent.

The order of attribute application within equivalent forms is Unspecified Behavior (USB).

**Diagnostics:** See Appendix A, code `E-MOD-2450`.


#### 7.1.2 Attribute Categories

**Attribute Registry.** The set (𝓡) of all attributes recognized by a conforming implementation:

𝓡 = 𝓡_spec ⊎ 𝓡_vendor

where 𝓡_spec contains specification-defined attributes and 𝓡_vendor contains vendor-defined extensions.

**Static Semantics**

**Attribute Processing**

For each attribute A applied to declaration D:

1. The implementation MUST verify A.Name ∈ 𝓡
2. The implementation MUST verify D is a valid target for A
3. The implementation MUST verify A.Args conforms to the argument specification
4. If all verifications succeed, the implementation applies the defined effect

**Attribute Target Matrix**

The matrix below enumerates all specification-defined attributes that may appear in Cursive source. Examples of vendor or custom attributes (e.g., `[[attr1]]`, `[[com.vendor.foo]]`) are not part of this registry.

| Attribute                                                                                              | `record` | `enum` | `modal` |     `procedure`     | Field | Binding |     `expression`      |
| :----------------------------------------------------------------------------------------------------- | :------: | :----: | :-----: | :-----------------: | :---: | :-----: | :-------------------: |
| `[[layout(...)]]`                                                                                      |   Yes    |  Yes   |   No    |         No          |  No   |   No    |          No           |
| `[[inline(...)]]`                                                                                      |    No    |   No   |   No    |         Yes         |  No   |   No    |          No           |
| `[[cold]]`                                                                                             |    No    |   No   |   No    |         Yes         |  No   |   No    |          No           |
| `[[static_dispatch_only]]`                                                                             |    No    |   No   |   No    |         Yes         |  No   |   No    |          No           |
| `[[deprecated(...)]]`                                                                                  |   Yes    |  Yes   |   Yes   |         Yes         |  Yes  |   Yes   |          No           |
| `[[reflect]]`                                                                                          |   Yes    |  Yes   |   Yes   |         No          |  No   |   No    |          No           |
| `[[derive(...)]]`                                                                                      |   Yes    |  Yes   |   Yes   |         No          |  No   |   No    |          No           |
| `[[symbol(...)]]`                                                                                      |    No    |   No   |   No    |      Yes (FFI)      |  No   |   No    |          No           |
| `[[library(...)]]`                                                                                     |    No    |   No   |   No    |      Yes (FFI)      |  No   |   No    |          No           |
| `[[no_mangle]]`                                                                                        |    No    |   No   |   No    |      Yes (FFI)      |  No   |   No    |          No           |
| `[[unwind(...)]]`                                                                                      |    No    |   No   |   No    |      Yes (FFI)      |  No   |   No    |          No           |
| `[[export(...)]]`                                                                                      |    No    |   No   |   No    |      Yes (FFI)      |  No   |   No    |          No           |
| `[[ffi_pass_by_value]]`                                                                                |   Yes    |  Yes   |   No    |         No          |  No   |   No    |          No           |
| `[[dynamic]]`                                                                                          |   Yes    |  Yes   |   Yes   |         Yes         |  No   |   No    |          Yes          |
| Verification-mode attributes (`[[static]]`, `[[assume]]`, `[[trust]]`)                                 |    No    |   No   |   No    | Yes (FFI contracts) |  No   |   No    |          No           |
| Memory-ordering attributes (`[[relaxed]]`, `[[acquire]]`, `[[release]]`, `[[acq_rel]]`, `[[seq_cst]]`) |    No    |   No   |   No    |         No          |  No   |   No    |          Yes          |
| `[[stale_ok]]`                                                                                         |    No    |   No   |   No    |         No          |  No   |   Yes   |          No           |
| `[[emit]]`                                                                                             |    No    |   No   |   No    |         No          |  No   |   No    | Yes (comptime blocks) |
| `[[files]]`                                                                                            |    No    |   No   |   No    |         No          |  No   |   No    | Yes (comptime blocks) |

**Diagnostics:** See Appendix A, codes `E-MOD-2451`, `E-MOD-2452`.


#### 7.1.3 Custom Attributes

**Vendor-Defined Attributes.** Attributes in 𝓡_vendor that extend the specification-defined set.

**Static Semantics**

1. Vendor attributes MUST use reverse-domain-style prefixes: `[[com.vendor.attribute_name]]`
2. The `cursive.*` namespace is reserved
3. Vendor-defined attributes are Implementation-Defined Behavior (IDB)

An implementation encountering an unknown attribute (not in 𝓡) MUST diagnose error `E-MOD-2451`.


### 7.2 Layout Attributes (`[[layout]]`)


#### 7.2.1 `[[layout(C)]]`

**`[[layout(C)]]`.** Specifies C-compatible memory layout.

**Syntax**

```ebnf
layout_attribute ::= "[[" "layout" "(" layout_args ")" "]]"
layout_args      ::= layout_kind ("," layout_kind)*
layout_kind      ::= "C" | "packed" | "align" "(" integer_literal ")" | int_type
int_type         ::= "i8" | "i16" | "i32" | "i64" | "u8" | "u16" | "u32" | "u64"
```

**Static Semantics**

**For `record` declarations:**

1. Fields MUST be laid out in declaration order
2. Padding MUST be inserted only as required by the target platform's C ABI
3. Total size MUST be a multiple of the record's alignment

**For `enum` declarations:**

1. The discriminant MUST be represented as a C-compatible integer tag
2. Default tag type is Implementation-Defined (IDB)
3. Layout MUST conform to tagged union per target C ABI

**`[[layout(IntType)]]` (Explicit Discriminant):**

For an `enum` marked `[[layout(IntType)]]` where `IntType` is `i8`–`i64` or `u8`–`u64`:

1. The discriminant MUST use the specified integer type
2. Each variant's discriminant value MUST be representable in that type
3. This form is valid only on `enum` declarations


#### 7.2.2 `[[layout(packed)]]`

**`[[layout(packed)]]`.** Removes inter-field padding.

**Static Semantics**

For a `record` marked `[[layout(packed)]]`:

1. All inter-field padding is removed
2. Each field MUST be laid out with alignment 1
3. The record's overall alignment becomes 1

**Access Classification:**

| Operation                          | Classification         |
| :--------------------------------- | :--------------------- |
| `packed.field = value`             | Direct write (safe)    |
| `let x = packed.field`             | Direct read (safe)     |
| `packed.field~>method()`           | Reference-taking (UVB) |
| Pass to `move` parameter           | Direct read (safe)     |
| Pass to `const`/`unique` parameter | Reference-taking (UVB) |

Taking a reference to a packed field is Unverifiable Behavior (UVB); such operations MUST occur within an `unsafe` block.

**Diagnostics:** See Appendix A, code `E-MOD-2454`.


#### 7.2.3 `[[layout(align(N))]]`

**`[[layout(align(N))]]`.** Sets a minimum alignment.

**Static Semantics**

1. N MUST be a positive integer that is a power of two
2. Effective alignment is max(N, natural alignment)
3. If N < natural alignment, natural alignment is used (warning emitted)
4. Type size is padded to a multiple of the effective alignment

**Diagnostics:** See Appendix A, codes `E-MOD-2453`, `W-MOD-2451`.


#### 7.2.4 Compile-Time Layout Verification

**Layout Verification.** Layout attribute combinations are verified at compile time for validity and applicability.

**Static Semantics**

**Valid Combinations:**

| Combination                | Validity | Effect                                  |
| :------------------------- | :------- | :-------------------------------------- |
| `layout(C)`                | Valid    | C-compatible layout                     |
| `layout(packed)`           | Valid    | Packed layout (records only)            |
| `layout(align(N))`         | Valid    | Minimum alignment N                     |
| `layout(C, packed)`        | Valid    | C-compatible packed layout              |
| `layout(C, align(N))`      | Valid    | C-compatible with minimum alignment N   |
| `layout(u8)` (enum)        | Valid    | 8-bit unsigned discriminant             |
| `layout(packed, align(N))` | Invalid  | Conflicting directives                  |

**Applicability Constraints:**

| Declaration Kind          |  `C`  | `packed` | `align(N)` | `IntType` |
| :------------------------ | :---: | :------: | :--------: | :-------: |
| `record`                  |   ✓   |    ✓     |     ✓      |     ✗     |
| `enum`                    |   ✓   |    ✗     |     ✓      |     ✓     |
| `modal`                   |   ✗   |    ✗     |     ✗      |     ✗     |
| Generic (unmonomorphized) |   ✗   |    ✗     |     ✗      |     ✗     |

**Diagnostics:** See Appendix A, code `E-MOD-2455`.


### 7.3 Optimization Hints


#### 7.3.1 `[[inline]]` and `[[inline(always)]]`

**`[[inline(...)]]`.** Specifies procedure inlining behavior.

**Syntax**

```ebnf
inline_attribute ::= "[[" "inline" ("(" inline_mode ")")? "]]"
inline_mode      ::= "always" | "never" | "default"
```

**Static Semantics**

**`[[inline]]`:**

The implementation SHOULD inline the procedure at call sites when feasible. No diagnostic is required if inlining is not performed.

**`[[inline(always)]]`:**

The implementation SHOULD inline the procedure at all call sites. If inlining is not possible (recursive, address taken), warning `W-MOD-2452` MUST be emitted.

**`[[inline(default)]]`:**

The implementation applies default heuristics. Semantically equivalent to omitting the attribute.

**No Attribute:**

The implementation MAY inline at its discretion.

**Diagnostics:** See Appendix A, code `W-MOD-2452`.


#### 7.3.2 `[[inline(never)]]`

**`[[inline(never)]]`.** Prohibits inlining.

**Static Semantics**

The implementation MUST NOT inline the procedure. The procedure body MUST be emitted as a separate callable unit; all calls MUST use the standard calling convention.


#### 7.3.3 `[[cold]]`

**`[[cold]]`.** Marks a procedure as unlikely to execute during typical runs.

**Syntax**

```ebnf
cold_attribute ::= "[[" "cold" "]]"
```

**Static Semantics**

Effects include:

1. Placement in separate code section for instruction cache locality
2. Reduced optimization effort
3. Branch prediction bias toward not calling

The attribute takes no arguments and is valid only on procedure declarations. It is a hint; implementations MAY ignore it.


### 7.4 Diagnostics and Metadata


#### 7.4.1 `[[deprecated]]`

**`[[deprecated(...)]]`.** Marks a declaration as deprecated.

**Syntax**

```ebnf
deprecated_attribute ::= "[[" "deprecated" ("(" string_literal ")")? "]]"
```

**Static Semantics**

When a program references a declaration marked `[[deprecated(...)]]`:

1. The implementation MUST emit warning `W-CNF-0601` (per §1.5)
2. If a message argument is present, the diagnostic SHOULD include it
3. The deprecated declaration remains fully functional


#### 7.4.3 `[[dynamic]]`

**`[[dynamic]]`.** Marks a declaration or expression as requiring runtime verification when static verification is insufficient.

**Syntax**

```ebnf
dynamic_attribute ::= "[[" "dynamic" "]]"
```

**Valid Targets:**

| Target       | Effect                                                     |
| :----------- | :--------------------------------------------------------- |
| `record`     | Operations on instances may use runtime verification       |
| `enum`       | Operations on instances may use runtime verification       |
| `modal`      | State transitions may use runtime verification             |
| `procedure`  | Contracts and body operations may use runtime verification |
| `expression` | The specific expression may use runtime verification       |

**Static Semantics**

**Scope Determination:**

An expression e is within a `[[dynamic]]` scope if and only if:
1. e is syntactically enclosed in a declaration marked `[[dynamic]]`, OR
2. e is syntactically enclosed in an expression marked `[[dynamic]]`

**Propagation:**

1. Declaration-level: All expressions within the declaration are in scope
2. Expression-level: Only that expression and sub-expressions are in scope
3. Scope is determined lexically; does not propagate through procedure calls

**Effect on Key System (§17):**

Within a `[[dynamic]]` scope, if static key safety analysis fails, runtime synchronization MAY be inserted.

**Effect on Contract System (§14):**

Within a `[[dynamic]]` scope, if a contract predicate cannot be proven statically, a runtime check is inserted. Failed checks panic (`P-SEM-2850`).

**Effect on Refinement Types (§13.7):**

Within a `[[dynamic]]` scope, if a refinement predicate cannot be proven statically, a runtime check is inserted. Failed checks panic (`P-TYP-1953`).

**Dynamic Semantics**

Even within a `[[dynamic]]` scope, if a property is statically provable, no runtime code is generated.

**Diagnostics:** See Cursive0 Appendix A for `E-CON-0410`–`E-CON-0412` and `W-CON-0401`. See Appendix H, Table H.1 for `E-SEM-2801`, `E-TYP-1953`, `P-SEM-2850`, and `P-TYP-1953`. For `E-CON-0020` (key safety outside `[[dynamic]]` scope), see §17.6.1.


#### 7.4.4 `[[static_dispatch_only]]`

**`[[static_dispatch_only]]`.** Excludes a procedure in a class from dynamic dispatch via `$Class` (vtable).

**Syntax**

```ebnf
static_dispatch_attr ::= "[[" "static_dispatch_only" "]]"
```

**Static Semantics**

**VTable Eligibility:**

A procedure p in a class is vtable-eligible if:

vtable_eligible(p) ⇔ has_receiver(p) ∧ ¬ is_generic(p) ∧ compatible_signature(p)

**Dispatchability (Dynamic Dispatch Safety):**

A class is **dispatchable** (usable as a dynamic class type `$Class`) if every procedure is either vtable-eligible or has `[[static_dispatch_only]]`:

dispatchable(C) ⇔ ∀ p ∈ procedures(C). vtable_eligible(p) ∨ has_static_dispatch_attr(p)

**Call Resolution:**

1. Static context: Calls on concrete types resolve normally
2. Dynamic context: Calls on dynamic class values (`$Class`) cannot resolve to `[[static_dispatch_only]]` procedures

**Constraints**

For diagnostic codes related to `[[static_dispatch_only]]` violations, see §13.5.3 (`E-TYP-2540`, `E-TYP-2542`).


#### 7.4.5 `[[stale_ok]]`

**`[[stale_ok]]`.** Suppresses staleness warnings (`W-CON-0011`) for a binding whose value was derived from `shared` data before a `release`/`yield release` boundary (§17.4.1, §19.2.2).

This attribute is intended for optimistic concurrency patterns where reusing a potentially stale snapshot is deliberate.

**Syntax**

```ebnf
stale_ok_attribute ::= "[[" "stale_ok" "]]"
```

**Static Semantics**

1. Takes no arguments.
2. Valid only on `let` and `var` binding declarations.
3. When present, the implementation MUST NOT emit `W-CON-0011` for uses of that binding.

The attribute does not alter key acquisition, release, or program behavior.

**Diagnostics:** See Appendix A, code `E-MOD-2465`.


#### 7.4.6 Verification-mode attributes (`[[static]]`, `[[assume]]`, `[[trust]]`)

**Syntax**

```ebnf
ffi_verification_attr ::= "[[" ffi_verification_mode "]]"
ffi_verification_mode ::= "static" | "dynamic" | "assume" | "trust"
```

**Static Semantics**

Verification-mode attributes are interpreted only in foreign-contract contexts. The semantics of each mode are defined in Appendix H (§21.4). The `dynamic` mode reuses `[[dynamic]]` as defined in §7.4.3.


### 7.5 FFI Attributes


#### 7.5.1 `[[symbol]]`

**`[[symbol("name")]]`.** Overrides the linker symbol name for a procedure, specifying the exact symbol used during linking.

**Syntax**

```ebnf
symbol_attribute ::= "[[" "symbol" "(" string_literal ")" "]]"
```

```cursive
extern "C" {
    [[symbol("__real_malloc")]]
    procedure malloc(size: usize) -> *mut c_void;
}
```

**Static Semantics**

1. Valid on procedure declarations within `extern` blocks and on exported procedures
2. The string argument specifies the exact symbol name used during linking
3. The symbol name MUST be a valid identifier for the target platform's linker
4. Does not affect Cursive-side name resolution; the procedure is still called by its Cursive identifier
5. Overrides any name mangling that would otherwise apply

**Symbol Resolution:**

| Declaration Context    | Without `[[symbol]]` | With `[[symbol("X")]]` |
| :--------------------- | :------------------- | :--------------------- |
| `extern "C"` procedure | Literal identifier   | `X`                    |
| `extern` procedure     | Mangled name         | `X`                    |
| Exported procedure     | Mangled name         | `X`                    |

**Diagnostics:** See Appendix A, codes `E-SYS-3340`–`E-SYS-3342`.


#### 7.5.2 `[[library]]`

**`[[library(...)]]`.** Specifies library linkage for an extern block.

**Syntax**

```ebnf
library_attribute ::= "[[" "library" "(" library_args ")" "]]"
library_args      ::= "name" ":" string_literal ("," "kind" ":" string_literal)?
```

```cursive
[[library(name: "ssl", kind: "dylib")]]
extern "C" {
    procedure SSL_new(ctx: *mut SSL_CTX) -> *mut SSL;
}
```

**Static Semantics**

**Link Kinds:**

| Kind          | Meaning                   |
| :------------ | :------------------------ |
| `"dylib"`     | Dynamic library (default) |
| `"static"`    | Static library            |
| `"framework"` | macOS framework           |
| `"raw-dylib"` | Windows delay-load        |

1. Valid only on `extern` blocks
2. The `name` argument specifies the library name without platform prefix/suffix (e.g., `"ssl"` not `"libssl.so"`)
3. If `kind` is omitted, `"dylib"` is assumed
4. Platform-specific library resolution applies (e.g., `"ssl"` resolves to `libssl.so` on Linux, `ssl.dll` on Windows)

**Diagnostics:** See Appendix A, codes `E-SYS-3345`–`E-SYS-3347`.


#### 7.5.3 `[[no_mangle]]`

**`[[no_mangle]]`.** Disables name mangling, causing the procedure to be exported with its literal Cursive identifier as the symbol name.

**Syntax**

```ebnf
no_mangle_attribute ::= "[[" "no_mangle" "]]"
```

**Static Semantics**

**Name Mangling:**

By default, Cursive encodes procedure symbols with module path, parameter types, and generic instantiations. The mangling scheme is Implementation-Defined Behavior (IDB).

**Effect of `[[no_mangle]]`:**

1. The procedure is exported with its identifier unchanged (e.g., `process`)
2. Valid on procedure declarations within `extern` blocks and on exported procedures
3. Implicit for procedures in `extern "C"` blocks (specifying it is redundant but permitted)
4. MUST NOT be combined with `[[symbol]]` on the same declaration

**Constraints**

**Diagnostics:** See Appendix A, codes `E-SYS-3350`, `E-SYS-3351`, `W-SYS-3350`.


#### 7.5.4 `[[unwind]]`

**`[[unwind(mode)]]`.** Controls panic propagation behavior at FFI boundaries, determining what happens when a panic attempts to cross into or out of foreign code.

**Syntax**

```ebnf
unwind_attribute ::= "[[" "unwind" "(" unwind_mode ")" "]]"
unwind_mode      ::= string_literal
```

**Static Semantics**

**Modes:**

| Mode      | Behavior                                                                                                                                                                                                               |
| :-------- | :--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `"abort"` | If a panic reaches the FFI boundary, the process terminates immediately via `abort()`. No destructors run for frames beyond the boundary.                                                                              |
| `"catch"` | Panics are caught at the boundary. For imported procedures, foreign exceptions are converted to Cursive panics. For exported procedures, Cursive panics are caught and the procedure returns an error indicator (IDB). |

**Default Behavior:**

If `[[unwind]]` is not specified, `"abort"` is assumed.

**Applicability:**

1. Valid on `extern` procedure declarations (imported foreign procedures)
2. Valid on exported procedures (Cursive procedures callable from foreign code)
3. The `"catch"` mode requires the foreign code's exception model to be compatible (IDB)

**Diagnostics:** See Appendix A, codes `E-SYS-3355`, `E-SYS-3356`, `W-SYS-3355`.

---


# Appendix C: Variance and Subtyping for Generic Type Constructors (Normative)


**Class Implementation Subtyping**

A concrete type that implements a class is a subtype of that class. See §13.3 for class implementation and the formal rule (Sub-Class).

**Additional Subtyping Rules**

The following subtyping relationships are defined in their respective sections:

- Modal widening (M@S <: M, where M is the general modal type): §12.1
- Union member subtyping: §11.8
- Refinement type subtyping: §13.7
- Permission subtyping: §10.3

**Diagnostics:** See Appendix A, codes `E-TYP-1510`–`E-TYP-1512`.


### 9.3 Variance

**Variance.** Specifies how subtyping of type arguments relates to subtyping of parameterized types. For a generic type constructor F with parameter T, variance determines whether F[A] <: F[B] when A <: B, when B <: A, both, or neither.

**Static Semantics**

**Variance Classifications**

| Variance      | Symbol | Condition for F[A] <: F[B] | Position Requirement                              |
| :------------ | :----- | :--------------------------- | :------------------------------------------------ |
| Covariant     | `+`    | A <: B                       | Output positions (return types, immutable fields) |
| Contravariant | `-`    | B <: A                       | Input positions (parameter types)                 |
| Invariant     | `=`    | A ≡ B                        | Both input and output, or mutable storage         |
| Bivariant     | `±`    | Always (parameter unused)    | Parameter does not appear in type structure       |

**Generic Subtyping Rule**

For a generic type `Name<T1...Tn>` to be a subtype of `Name<U1...Un>`, each type argument pair `(Ti, Ui)` must satisfy the variance of the corresponding parameter:
- **Covariant (+)**: `Ti <: Ui`
- **Contravariant (-)**: `Ui <: Ti`
- **Invariant (=)**: `Ti == Ui`
- **Bivariant (+/-)**: Always compatible

**Function Type Variance**

Function types are contravariant in their parameter types and covariant in their return types. `(U) -> R1` is a subtype of `(T) -> R2` if `T <: U` and `R1 <: R2`.

**Permission Interaction**

The `const` permission enables covariance for otherwise invariant generic types: `const C<A>` is a subtype of `const C<B>` if `A <: B`. This applies because `const` prohibits mutation.

This relaxation does **not** apply to `unique` or `shared` permissions, which require strict invariance for mutable generic parameters.


**Constraints**

An implementation MUST reject a subtyping judgment that violates variance rules.

When a type parameter has invariant variance, the implementation MUST require exact type equivalence for that parameter in subtyping checks.

**Diagnostics:** See Appendix A, codes `E-TYP-1520`, `E-TYP-1521`.


### 9.4 Type Inference


#### 9.4.1 Bidirectional Type Checking

**Type Inference.** The process by which type information not explicitly annotated in the source program is derived. Cursive employs bidirectional type inference, combining two complementary modes of type propagation.

**Static Semantics**

**Judgment Classes**

| Judgment                        | Name      | Meaning                                                  |
| :------------------------------ | :-------- | :------------------------------------------------------- |
| Γ ⊢ e ⇒ T | Synthesis | Type T is derived from the structure of expression e |
| Γ ⊢ e ⇐ T | Checking  | Expression e is validated against expected type T    |

In synthesis mode, the type flows outward from the expression. In checking mode, an expected type flows inward from context to guide type derivation for subexpressions.

**Synthesis Rules**

*   **Variables**: Type is looked up in the context.
*   **Tuples**: Type is synthesized from component types.
*   **Calls**: Return type is synthesized from the function signature; arguments are checked against parameter types.
*   **Annotations**: Explicitly annotated expressions synthesize the annotated type.

**Checking Rules**

*   **Subsumption**: An expression checks against type `T` if it synthesizes type `S` and `S <: T`.
*   **Closures**: Checked against an expected function type by inferring parameter types from the expected signature and checking the body against the return type.



#### 9.4.2 Local Type Inference

**Local Type Inference.** Restricts inference to operate within a single procedure body.

**Static Semantics**

Type inference MUST be local: inference operates within a single procedure body and MUST NOT propagate type information across procedure boundaries.

An implementation MUST NOT infer types for public API boundaries.


#### 9.4.3 Inference Constraints

**Static Semantics**

**Mandatory Annotations**

The following positions MUST have explicit type annotations:

- Procedure parameter types
- Procedure return types
- Public and protected module-scope bindings

**Permitted Omissions**

The following positions MAY omit type annotations when the type is inferable:

- Local bindings within procedure bodies (`let`, `var`)
- Closure parameters when the closure is checked against a known function type

**Diagnostics:** See Appendix A, code `E-TYP-1505`.


#### 9.4.4 Inference Failures

**Static Semantics**

An implementation MUST reject a program if inference fails to derive a unique type for any expression.

When type arguments cannot be inferred, explicit type arguments may be supplied using turbofish syntax (`::<>`). See §13.1 for the full specification.

**Diagnostics:** See Appendix A, code `E-TYP-1530`.

---


# Appendix D: Permissions (Including `shared`) (Normative)

## 10. The Permission System

---

### 10.1 Permission Lattice (`const`, `unique`, `shared`)


#### 10.1.1 `const` Permission (Read-Only)

**Permission.** A type qualifier governing access, mutation, and aliasing of data referenced by a binding. When no permission is specified, `const` is the default.

**`const` Permission.** Grants read-only access to data with unlimited aliasing. Mutation through a `const` path is forbidden.

**Syntax**

```ebnf
permission       ::= "const" | "unique" | "shared"
permission_type  ::= type
```

```cursive
let cfg: const Config = load()
let x = cfg.value
```

**Static Semantics**

Let 𝒫 denote the set of permissions:

𝒫 = {`const`, `unique`, `shared`}

A **permission-qualified type** is a pair (P, T) where P ∈ 𝒫 and T is a base type. The notation P T denotes this qualification.

**Operations Permitted:**

| Operation | Permitted |
| :-------- | :-------- |
| Read      | Yes       |
| Write     | No        |
| Aliasing  | Unlimited |


#### 10.1.2 `unique` Permission (Exclusive Read-Write)

**`unique` Permission.** Grants exclusive read-write access to data. A live `unique` path to an object precludes any other path to that object or its sub-components.

**Static Semantics**

**Exclusivity Invariant:**

∀ p_1, p_2 ∈ Paths. (perm(p_1) = `unique` ∧ overlaps(p_1, p_2)) ⇒ p_1 = p_2

**Operations Permitted:**

| Operation | Permitted  |
| :-------- | :--------- |
| Read      | Yes        |
| Write     | Yes        |
| Aliasing  | No aliases |

The `unique` permission does **not** imply cleanup responsibility. A parameter of type `unique T` (without `move`) grants exclusive access while the caller retains responsibility.


#### 10.1.3 `shared` Permission (Synchronized Access)

**`shared` Permission.** Grants mutable access through implicit key acquisition. See §17 for complete key system semantics.

**Static Semantics**

**Operations Permitted:**

| Operation                   | Permitted | Key Mode  |
| :-------------------------- | :-------- | :-------- |
| Field read                  | Yes       | Read key  |
| Field mutation              | Yes       | Write key |
| Method call (`~` receiver)  | Yes       | Read key  |
| Method call (`~%` receiver) | Yes       | Write key |
| Method call (`~!` receiver) | No        | N/A       |

**Key Properties:**

| Property      | Description                                               |
| :------------ | :-------------------------------------------------------- |
| Path-specific | Keys acquired at accessed path granularity                |
| Implicit      | Accessing `shared` path acquires necessary key            |
| Minimal       | Keys held for minimal duration                            |
| Reentrant     | Covering key permits nested access without re-acquisition |

The `shared` permission does **not** imply cleanup responsibility.


#### 10.1.4 Lattice Ordering

**Lattice Ordering.** The permission lattice orders permissions by aliasing guarantee strength.

**Static Semantics**

`unique` <: `shared` <: `const`

**Lattice Diagram:**

```
    unique      (strongest: exclusive, mutable)
       ↓
    shared      (synchronized, mutable)
       ↓
     const      (weakest: unlimited, immutable)
```


### 10.2 Exclusivity and Aliasing Rules

**Aliasing.** Two paths alias when they refer to overlapping storage locations: aliases(p_1, p_2) ⇔ storage(p_1) ∩ storage(p_2) ≠ ∅.

**Binding State Machine**

A binding b with `unique` permission exists in one of two states:

| State    | Definition                                        | Operations Permitted         |
| :------- | :------------------------------------------------ | :--------------------------- |
| Active   | No downgraded references to b are live          | Read, write, move, downgrade |
| Inactive | One or more downgraded references to b are live | No operations                |

**Static Semantics**

**(Inactive-Enter)**
b : `unique` T    b is Active    downgrade to P where P ∈ {`const`, `shared`}
──────────────────────────────────────────────────────────────────────────────
b becomes Inactive

**(Inactive-Exit)**
b is Inactive    all downgraded references to b go out of scope
──────────────────────────────────────────────────────────────────────────────
b becomes Active with `unique` permission restored

**Constraints**

1. During the inactive period, the original `unique` binding MUST NOT be read, written, or moved.
2. The transition back to Active occurs deterministically when the downgrade scope ends.

**Coexistence Matrix**

| Active Permission | May Add `unique` | May Add `shared` | May Add `const` |
| :---------------- | :--------------- | :--------------- | :-------------- |
| `unique`          | No               | No               | No              |
| `shared`          | No               | Yes              | Yes             |
| `const`           | No               | Yes              | Yes             |


### 10.3 Permission Subtyping and Coercion

**Permission Subtyping.** This section formalizes the subtype relation between permission-qualified types. General subtyping is defined in §9.2.

**Subtyping Rules**

Permission subtyping allows treating a stronger permission as a weaker one:
- `unique T <: shared T`
- `unique T <: const T`
- `shared T <: const T`

Permissions coerce implicitly in one direction only: from stronger to weaker. Upgrade from weaker to stronger is forbidden.

**Diagnostics:** See Appendix A, codes `E-TYP-1601`, `E-TYP-1602`, `E-TYP-1604`.

**Method Receiver Permissions**

*[REF: Shorthand syntax (`~`, `~!`, `~%`) is defined in §14.2.2.]*

A method with receiver permission P_method is callable through a path with permission P_caller iff:

P_caller <: P_method

| Caller Permission | May Call `~` | May Call `~%` | May Call `~!` |
| :---------------- | :----------- | :------------ | :------------ |
| `const`           | Yes          | No            | No            |
| `shared`          | Yes          | Yes           | No            |
| `unique`          | Yes          | Yes           | Yes           |

**Diagnostics:** See Appendix A, code `E-TYP-1605`.

---


# Appendix E: Generics and Classes (Normative)

- Covariant return types

**Constraints**

**Structural Constraints**

1. Zero or more parameters, exactly one return type
2. Return type `()` for procedures returning no meaningful value
3. `move` modifier applies only to parameters, not return type

**FFI Constraints**

1. Closure types MUST NOT appear in `extern` signatures
2. Only sparse function pointer types are FFI-safe
3. Sparse function pointers in FFI MUST NOT have generic type parameters

**Invocation Constraints**

1. Argument count MUST equal parameter count
2. Each argument type MUST be a subtype of corresponding parameter type
3. For `move` parameters, argument MUST be explicit `move` expression

**Diagnostics:** See Appendix A, codes `E-TYP-2220`–`E-TYP-2226`. For `E-MOD-2411` (missing move expression at call site for transferring parameter), see §3.4.6.

---

## 13. Abstraction and Polymorphism

---

### 13.1 Generics


#### 13.1.1 Generic Type Parameters

**Generic Declaration.** Introduces one or more **type parameters** that serve as placeholders for concrete types supplied at instantiation.

A generic declaration D is defined by:

D = (Name, Params, Body)

where:
- Name is the declaration's identifier
- Params = ⟨P_1, P_2, …, P_n⟩ is an ordered sequence of type parameters
- Body is the declaration body (type definition or procedure body)

Each type parameter P_i is defined by:

P_i = (name_i, Bounds_i)

where:
- name_i is an identifier serving as the parameter name
- Bounds_i ⊆ 𝒯_class is a (possibly empty) set of class bounds

A type parameter with Bounds_i = ∅ is **unconstrained**. A type parameter with Bounds_i ≠ ∅ is **constrained**; only types implementing all classes in Bounds_i may be substituted.

**Syntax**

```ebnf
generic_params     ::= "<" generic_param_list ">"
generic_param_list ::= generic_param (";" generic_param)*
generic_param      ::= identifier bound_clause? default_clause?
bound_clause       ::= "<:" class_bound_list
default_clause     ::= "=" type
class_bound_list   ::= class_bound ("," class_bound)*

generic_args       ::= "<" type_arg_list ">"
type_arg_list      ::= type ("," type)*
turbofish          ::= "::" generic_args
```

```cursive
record Container<T> { value: T }
record Pair<K; V> { key: K, value: V }
```

**Static Semantics**

**(WF-Generic-Param)**

∀ i ≠ j, name_i ≠ name_j    ∀ i, ∀ B ∈ Bounds_i, Γ ⊢ B : Class
──────────────────────────────────────────────────────────────────────────
Γ ⊢ ⟨P_1; …; P_n⟩ wf

**(WF-Generic-Type)**

Γ ⊢ ⟨P_1, …, P_n⟩ wf    Γ, T_1 : P_1, …, T_n : P_n ⊢ Body wf
──────────────────────────────────────────────────────────────────────────
Γ ⊢ `type` Name⟨P_1, …, P_n⟩ Body wf

**Constraints**

| Separator | Role                                  | Scope                            |
| :-------- | :------------------------------------ | :------------------------------- |
| `;`       | Separates type parameters             | Between `<` and `>`              |
| `,`       | Separates bounds within one parameter | After `<:` until next `;` or `>` |

**Diagnostics:** See Appendix A, code `E-TYP-2304`.


#### 13.1.2 Generic Procedures

**Generic Procedure.** A procedure parameterized by type parameters. Type arguments may be explicit or inferred.

**Syntax**

```ebnf
generic_procedure ::= "procedure" identifier generic_params "(" param_list? ")" ("->" type)? block
```

```cursive
procedure identity<T>(x: T) -> T { result x }
procedure swap<T>(a: unique T, b: unique T) { ... }
```

**Static Semantics**

**(WF-Generic-Proc)**

Γ ⊢ ⟨P_1, …, P_n⟩ wf    Γ' = Γ, T_1 : P_1, …, T_n : P_n    Γ' ⊢ signature wf    Γ' ⊢ body wf
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ `procedure` f⟨P_1, …, P_n⟩(...) → R {…} wf

**(T-Generic-Call)**

f⟨T_1, …, T_n⟩(x_1 : S_1, …, x_m : S_m) → R declared
∀ i ∈ 1..n, Γ ⊢ A_i satisfies Bounds(T_i)
∀ j ∈ 1..m, Γ ⊢ e_j : S_j[A_1/T_1, …, A_n/T_n]
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ f⟨A_1, …, A_n⟩(e_1, …, e_m) : R[A_1/T_1, …, A_n/T_n]

**Type Argument Inference**

When type arguments are not explicitly provided, the implementation MUST infer them using bidirectional type inference (§9.4). Inference sources:
1. Types of value arguments at the call site
2. Expected return type from the surrounding context

Generic procedures in classes cannot participate in dynamic dispatch. Such procedures MUST be marked with `[[static_dispatch_only]]` (§7.4) and can only be called through concrete type references, not through `$Class` references.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-TYP-2301`, `E-TYP-2306`.


#### 13.1.3 Generic Constraints

**Constraint Satisfaction.** Determines whether a type argument satisfies the bounds declared for a type parameter.

**Syntax**

```ebnf
where_clause         ::= "where" where_predicate_list
where_predicate_list ::= where_predicate (predicate_separator where_predicate)* ";"?
where_predicate      ::= identifier "<:" class_bound_list
predicate_separator  ::= terminator
```

```cursive
procedure sort<T>(arr: unique [T]) where T <: Ord { ... }
```

**Static Semantics**

**(T-Constraint-Sat)**

∀ B ∈ Bounds, Γ ⊢ A <: B
─────────────────────────────────────
Γ ⊢ A satisfies Bounds

**(T-Generic-Inst)**

Name⟨P_1, …, P_n⟩ declared    ∀ i ∈ 1..n, Γ ⊢ A_i satisfies Bounds(P_i)
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ Name⟨A_1, …, A_n⟩ wf

When both inline bounds and a `where` clause specify constraints for the same parameter, the effective constraint is the conjunction of all bounds.

**Constraints**

1. A class bound MUST reference a valid class type; bounding by non-class types is forbidden.
2. A generic instantiation MUST provide exactly the number of type arguments declared.
3. Type arguments MUST satisfy all constraints declared for their corresponding parameters.

**Diagnostics:** See Appendix A, codes `E-TYP-2302`, `E-TYP-2303`, `E-TYP-2305`.


#### 13.1.4 Monomorphization

**Monomorphization.** The process of generating specialized code for each concrete instantiation of a generic type or procedure.

Given a generic declaration D⟨T_1, …, T_n⟩ and concrete type arguments A_1, …, A_n, monomorphization produces a specialized declaration D[A_1/T_1, …, A_n/T_n] where each occurrence of T_i in the body is replaced with A_i.

**Static Semantics**

**Monomorphization Requirements**

1. **Specialization:** For each instantiation D⟨A_1, …, A_n⟩, produce code equivalent to substituting each type argument for its corresponding parameter throughout the declaration body.

2. **Zero Overhead:** Calls to generic procedures MUST compile to direct static calls to the specialized instantiation. Virtual dispatch is prohibited for static polymorphism.

3. **Independent Instantiation:** Each distinct instantiation is an independent type or procedure. `Container<i32>` and `Container<i64>` are distinct types.

**Recursion Depth**

Monomorphization MAY produce recursive instantiations. Implementations MUST detect and reject infinite monomorphization recursion. The maximum instantiation depth is IDB; minimum supported depth is 128.

**Variance**

The variance of each type parameter is determined by its usage within the type definition. See §9.3 for variance specification.

**Dynamic Semantics**

**Layout Independence**

Each monomorphized instantiation has an independent memory layout:

sizeof(Name⟨A_1, …, A_n⟩) = sizeof(Name[A_1/T_1, …, A_n/T_n])

alignof(Name⟨A_1, …, A_n⟩) = alignof(Name[A_1/T_1, …, A_n/T_n])

**Constraints**

**Diagnostics:** See Appendix A, codes `E-TYP-2307`, `E-TYP-2308`.


### 13.2 Classes (Forms)


#### 13.2.1 Class Declaration Syntax

**Class.** A named declaration that defines an abstract interface consisting of procedure signatures, associated type declarations, abstract fields, and abstract states.

A class Cl is defined as a tuple:

Cl = (N, G, S, P_abs, P_con, A_abs, A_con, F, St)

where:
- N is the class name
- G is the (possibly empty) set of generic type parameters
- S is the (possibly empty) set of superclass bounds
- P_abs is the set of abstract procedure signatures
- P_con is the set of concrete procedure definitions (default implementations)
- A_abs is the set of abstract associated type declarations
- A_con is the set of concrete associated type bindings
- F is the (possibly empty) set of abstract field declarations
- St is the (possibly empty) set of abstract state declarations

A class with St ≠ ∅ is a **modal class**. Only modal types may implement modal classes.

A type T **implements** class Cl (written T <: Cl) when:

T <: Cl ⇔ ∀ p ∈ P_abs. T defines p ∧ ∀ a ∈ A_abs. T binds a ∧ ∀ f ∈ F. T has f ∧ ∀ s ∈ St. T has s

**Syntax**

```ebnf
class_declaration ::=
    visibility? "class" identifier generic_params?
    ("<:" superclass_bounds)? "{"
        class_item*
    "}"

superclass_bounds ::= class_bound ("+" class_bound)*
class_bound       ::= type_path generic_args?

class_item ::=
    abstract_procedure
  | concrete_procedure
  | associated_type
  | abstract_field
  | abstract_state

abstract_procedure ::= "procedure" identifier signature contract_clause?
concrete_procedure ::= "procedure" identifier signature contract_clause? block
abstract_field     ::= identifier ":" type
abstract_state     ::= "@" identifier "{" field_list? "}"
field_list         ::= abstract_field ("," abstract_field)* ","?
```

```cursive
class Printable {
    procedure print(~, out: $FileSystem)
}
```

**Static Semantics**

**(WF-Class)**

unique(N)    ∀ p ∈ P. Γ, Self : Type ⊢ p wf    ¬ cyclic(S)    names_disjoint(P, A, F, St)
────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ class N [<: S] {P, A, F, St} wf

**(T-Superclass)**

class A <: B    T <: A
────────────────────────
Γ ⊢ T <: B

Within a class declaration, `Self` denotes the (unknown) implementing type.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-TYP-2500`, `E-TYP-2504`, `E-TYP-2505`, `E-TYP-2508`, `E-TYP-2509`, `E-TYP-2408`, `E-TYP-2409`.


#### 13.2.2 Associated Methods

**Abstract Procedure.** A procedure signature within a class that lacks a body. Implementing types MUST provide a concrete implementation.

**Concrete Procedure.** A procedure definition within a class that includes a body. Implementing types automatically inherit this procedure but MAY override it using the `override` keyword.

**Static Semantics**

**(WF-Class-Self)**

Γ, Self : Type ⊢ body : ok
──────────────────────────────────────────
Γ ⊢ class T { body } : Class


#### 13.2.3 Associated Types

**Associated Type.** A type declaration within a class:
- If abstract (no `= T`): implementing types MUST provide a concrete type binding
- If concrete (`= T`): provides a default type that MAY be overridden

**Syntax**

```ebnf
associated_type ::= "type" identifier ("=" type)?
```

See the `Iterator` class definition in §13.9.5 for an example of associated types in practice.

**Static Semantics**

Generic class parameters (`class Foo<T>`) are specified at use-site. Associated types are specified by the implementer within the type body.

**Class Alias Equivalence (T-Alias-Equiv)**

type Alias = A + B
──────────────────────────────────────────────────────────────
Γ ⊢ T <: Alias ⇔ Γ ⊢ T <: A ∧ Γ ⊢ T <: B


### 13.3 Class Implementation


#### 13.3.1 Implementation Blocks

**Class Implementation.** A type implements a class by:
1. Declaring the class in its "implements clause" using the `<:` operator
2. Providing implementations for all abstract procedures
3. Providing type bindings for all abstract associated types
4. Having fields with matching names and compatible types for all abstract fields
5. Having states with matching names and required payload fields for all abstract states (modal classes only)

Class implementation MUST occur at the type's definition site. Extension implementations are prohibited.

**Syntax**

```ebnf
impl_procedure    ::= visibility? "override"? "procedure" identifier signature block
```

```cursive
record Point { x: f64, y: f64 } <: Display {
    procedure display(~) -> string { ... }
}
```

**Static Semantics**

**(T-Impl-Complete)**

T <: Cl    ∀ p ∈ P_abs(Cl). T defines p    ∀ a ∈ A_abs(Cl). T binds a    ∀ f ∈ F(Cl). T has f    ∀ s ∈ St(Cl). T has s
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ T implements Cl

**(WF-Impl)**

Γ ⊢ T wf    Γ ⊢ Cl wf    St(Cl) ≠ ∅ ⇒ T is modal
────────────────────────────────────────────────────────────────────────────
Γ ⊢ T <: Cl wf

**Override Semantics**

- **Implementing an abstract procedure**: `override` keyword MUST NOT be used
- **Overriding a concrete procedure**: `override` keyword MUST be used

**Coherence Rule**

A type `T` MAY implement a class `Cl` at most once.

**Orphan Rule**

For `T <: Cl`, at least one of `T` or `Cl` MUST be defined in the current assembly.

**(T-Field-Compat)**

f : T_c ∈ F(Cl)    f : T_i ∈ fields(R)    T_i <: T_c
──────────────────────────────────────────────────────────────
R ⊢ f present

**(T-Modal-Class)**

St(Cl) ≠ ∅    T <: Cl    T is not a modal type
────────────────────────────────────────────────────
ill-formed: E-TYP-2401

**Constraints**

**Diagnostics:** See Appendix A, codes `E-TYP-2501`–`E-TYP-2507`, `E-TYP-2401`–`E-TYP-2407`.


### 13.4 Class Constraints


#### 13.4.1 Single Constraints

**Class Constraint.** A generic parameter `T <: Cl` restricts valid type arguments to types implementing class `Cl`.

**Static Semantics**

**Constraint Satisfaction**

A generic instantiation is well-formed only if every constrained parameter `T <: Cl` is instantiated with a type that implements `Cl`.

**Method Availability**

Within the body of a generic item, methods of `Cl` are callable on values of type `T` via static dispatch; calls resolve at monomorphization with no vtable lookup.


#### 13.4.2 Multiple Constraints (`+`)

**Multiple Constraints.** A type parameter MAY have multiple class bounds using the `+` separator or comma-separated bounds after `<:`.

**Syntax**

```cursive
procedure process<T <: Display, Clone>(value: T) { ... }
```

**Static Semantics**

The type argument must implement all specified classes.


#### 13.4.3 Where Clauses

**Where Clauses.** As an alternative to inline bounds, constraints MAY be specified in a `where` clause. See §13.1.3 for full grammar.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-TYP-2530`, `E-TYP-2531`.


### 13.5 Dynamic Polymorphism (`$`)


#### 13.5.1 Dynamic Class Objects

**Dynamic Class Type.** A `$Class` is a concrete, sized type representing any value implementing a dispatchable class. Implemented as a dense pointer.

**Syntax**

```ebnf
dynamic_type ::= "$" class_path
class_path   ::= type_path
```


**Static Semantics**

**(T-Dynamic-Form)**

Γ ⊢ v : T    Γ ⊢ T <: Cl    dispatchable(Cl)
──────────────────────────────────────────────
Γ ⊢ v : $Cl

**Dispatchability**

A class is **dispatchable** if every procedure in the class is either:
1. VTable-eligible, OR
2. Explicitly excluded via `[[static_dispatch_only]]`

**VTable Eligibility**

A procedure is **vtable-eligible** if ALL of the following are true:
1. Has a receiver parameter (`self`, `~`, `~!`, `~%`)
2. Has NO generic type parameters
3. Does NOT return `Self` by value (except via pointer indirection)
4. Does NOT use `Self` in parameter types (except via pointer indirection)

**Formal Definition**

dispatchable(Cl) ⇔ ∀ p ∈ procedures(Cl). vtable_eligible(p) ∨ has_static_dispatch_attr(p)

**Dynamic Semantics**

**Dynamic Class Type Creation**

1. Let `v` be a value of concrete type `T` where `T <: Cl` and `dispatchable(Cl)`.
2. Let `dp` be a pointer to the storage of `v`.
3. Let `vt` be the static vtable for the `(T, Cl)` type-class pair.
4. Construct the dynamic class value as the pair `(dp, vt)`.

**(E-Dynamic-Form)**

Γ ⊢ v : T    T <: Cl    dispatchable(Cl)
────────────────────────────────────────
v ⇒_$ (ptr(v), vtable(T, Cl))

**Memory Layout**

A dynamic class type (`$Class`) is represented as a two-word structure:

- **Size**: `2 * sizeof(usize)` (16 bytes on 64-bit platforms)
- **Alignment**: `alignof(usize)`

```
struct DynRepr {
    data: *imm ()       // Pointer to concrete instance
    vtable: *imm VTable // Pointer to class vtable
}
```


#### 13.5.2 Virtual Method Dispatch

**Virtual Method Dispatch.** A procedure call on a dynamic class type dispatches through the vtable.

**Dynamic Semantics**

**VTable Dispatch Algorithm**

A procedure call `w~>method(args)` on dynamic class type `w: $Cl` executes as follows:

1. Let `(dp, vt)` be the data pointer and vtable pointer components of `w`.
2. Let `offset` be the vtable offset for `method`.
3. Let `fp` be the function pointer at `vt + header_size + offset * sizeof(usize)`.
4. Return the result of calling `fp(dp, args)`.

**(E-Dynamic-Dispatch)**

w = (dp, vt)    method ∈ interface(Cl)    vt[offset(method)] = fp
────────────────────────────────────────────────────────────────────────────
w~>method(args) → fp(dp, args)

**VTable Layout (Stable ABI)**

For non-modal classes (header size = 3):
1. `size: usize` — Size of concrete type
2. `align: usize` — Alignment requirement
3. `drop: *imm fn` — Destructor function pointer (null if no `Drop`)
4. `methods[..]` — Function pointers in class declaration order

For modal classes (header size = 4): includes additional `state_map` pointer.


#### 13.5.3 Object Safety

**Object Safety.** Determines whether a class can be used as a dynamic class type (dispatchability).

**Static Semantics**

A class that contains non-vtable-eligible procedures without `[[static_dispatch_only]]` is NOT dispatchable.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-TYP-2540`–`E-TYP-2542`.


### 13.6 Opaque Polymorphism (`opaque`)


#### 13.6.1 Opaque Return Types

**Opaque Return Type.** An `opaque Class` exposes only the class's interface while hiding the concrete implementation type.

**Syntax**
*[REF: Return-type grammar is defined in §14.1.1.]*

```cursive
procedure get_iterator() -> opaque Iterator {
    result [1, 2, 3]~>iter()
}
```

**Static Semantics**

**(T-Opaque-Return)**

Γ ⊢ body : T    Γ ⊢ T <: Cl    return_type(f) = opaque Cl
───────────────────────────────────────────────────────────────────
Γ ⊢ f : () → opaque Cl

**(T-Opaque-Project)**

At call sites, the opaque type is treated as an existential; callers may invoke only class methods:

Γ ⊢ f() : opaque Cl    m ∈ interface(Cl)
────────────────────────────────────────────
Γ ⊢ f()~>m(args) : R_m

**Type Encapsulation**

For a procedure returning `opaque Class`:
- The callee returns a concrete type implementing `Class`
- The caller observes only `Class` members
- Access to concrete type members is forbidden


#### 13.6.2 Type Erasure

**Type Erasure.** Opaque types provide type encapsulation without runtime overhead.

**Static Semantics**

**Opaque Type Equality**

Two opaque types `opaque Cl` are equivalent if and only if they originate from the same procedure definition:

f ≠ g
──────────────────────────────
typeof(f()) ≠ typeof(g())

**Zero Overhead**

Opaque types MUST incur zero runtime overhead. The returned value is the concrete type, not a dense pointer. Type encapsulation is enforced statically.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-TYP-2510`–`E-TYP-2512`.


### 13.7 Refinement Types


#### 13.7.1 Refinement Syntax

**Refinement Type.** A type constructed by attaching a predicate constraint to a base type. The refinement type `T where { P }` denotes the subset of values of type `T` for which predicate `P` evaluates to `true`.

A refinement type R is defined by:

R = (T_base, P)

where:
- T_base ∈ 𝒯 is the base type being refined
- P : T_base → `bool` is a pure predicate constraining the value set

Values(T where {P}) = { v ∈ Values(T) | P(v) = `true` }

A refinement type is a **proper subtype** of its base type.

**Syntax**

```ebnf
refinement_type       ::= type "where" "{" predicate_expr "}"
type_alias_decl       ::= visibility? "type" identifier "=" type "where" "{" predicate_expr "}"
param_with_constraint ::= identifier ":" type "where" "{" predicate_expr "}"
```

```cursive
type NonZero = i32 where { self != 0 }
procedure divide(a: i32, b: i32 where { b != 0 }) -> i32
```

Within standalone refinement types, `self` refers to the constrained value. In parameter constraints, the parameter name is used instead.


#### 13.7.2 Refinement Constraints

**Static Semantics**

**(WF-Refine-Type)**

Γ ⊢ T wf    Γ, `self` : T ⊢ P : `bool`    Pure(P)
──────────────────────────────────────────────────────────────
Γ ⊢ (T where {P}) wf

**(T-Refine-Intro)**

Γ ⊢ e : T    Γ ⊢ F(P[e/`self`], L)    L dominates current location
────────────────────────────────────────────────────────────────────────────────
Γ ⊢ e : T where {P}

**(T-Refine-Elim)**

Γ ⊢ e : T where {P}
────────────────────
Γ ⊢ e : T

**Subtyping Rules**

Γ ⊢ (T where {P}) <: T

Γ ⊢ P ⇒ Q
──────────────────────────────────────────────────
Γ ⊢ (T where {P}) <: (T where {Q})

**Nested Refinements**

(T where {P}) where {Q} ≡ T where {P ∧ Q}

**Decidable Predicate Subset**

Implementations MUST support:
1. Literal comparisons
2. Bound propagation from control flow
3. Syntactic equality (up to alpha-renaming)
4. Transitive inequalities (linear arithmetic over integers)
5. Boolean combinations of decidable predicates


#### 13.7.3 Refinement Verification

**Static Semantics**

Refinement predicates require static proof by default:

1. Implementation attempts static verification
2. If verification succeeds, no runtime code is generated
3. If verification fails and not in `[[dynamic]]` scope: ill-formed (E-TYP-1953)
4. If verification fails in `[[dynamic]]` scope: runtime check is generated

**Dynamic Semantics**

**Representation**

sizeof(T where {P}) = sizeof(T)

alignof(T where {P}) = alignof(T)

The predicate is a compile-time and runtime constraint only; it does not affect physical representation.

**Constraints**

Failed runtime checks trigger a panic (`P-TYP-1953`).

**Diagnostics:** See Appendix H, Table H.1.


### 13.8 Capability Classes

*[REF: Capability fundamentals defined in §4 (The Authority Model). This section covers type-system integration only.]*


#### 13.8.1 Capability Class

**Capability Class.** Regular classes that define system authority interfaces. The dynamic dispatch operator `$` applies uniformly to all classes, including capability classes.

**Syntax**

```cursive
procedure read_config(fs: $FileSystem, path: string@View) -> string {
    let file = fs~>open_read(path)
    file~>read_all()
}
```

**Static Semantics**

A parameter of type `$Class` accepts any concrete type `T` implementing `Class`. This is the same mechanism as for any other class—capability classes have no special type-system behavior.

See §4 for capability propagation, attenuation, and no-ambient-authority rules.


#### 13.8.2 Capability Bounds

**Capability Bounds.** Capability classes can be used as bounds in generics like any other class.

**Syntax**

```cursive
procedure with_logging<L <: Logger>(fs: $FileSystem, log: L) { ... }
```

**Static Semantics**

No syntactic distinction exists between capability class bounds and regular class bounds. The capability nature is semantic (enforcement of no-ambient-authority), not syntactic.


### 13.9 Foundational Classes

*[REF: Complete semantics in Appendix D (Standard Form Catalog).]*


#### 13.9.1 `Drop` Class

**Drop Class.** Defines a destructor for RAII cleanup. The `drop` method is invoked implicitly when a binding goes out of scope.

**Syntax**

```cursive
class Drop {
    procedure drop(~!)
}
```

**Static Semantics**

- `Drop::drop` MUST NOT be called directly by user code (`E-MEM-3005`, §3.5.3).
- `Bitcopy` and `Drop` are mutually exclusive on the same type.

**Constraints**

**Diagnostics:** See Appendix A, code `E-TYP-2621`.


#### 13.9.2 `Bitcopy` Class

**Bitcopy Class.** A marker class for types that can be implicitly duplicated via bitwise copy.

**Static Semantics**

- `Bitcopy` types are duplicated (not moved) on assignment and parameter passing.
- `Bitcopy` requires all fields implement `Bitcopy`.
- Any type implementing `Bitcopy` MUST also implement `Clone`.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-TYP-2622`, `E-TYP-2623`.


#### 13.9.3 `Clone` Class

**Clone Class.** Defines explicit deep copy via the `clone` method.

**Syntax**

```cursive
class Clone {
    procedure clone(~) -> Self
}
```

**Static Semantics**

For `Bitcopy` types, `clone()` performs a bitwise copy identical to the implicit copy operation.


#### 13.9.4 `Eq` and `Hash` Classes

**Eq and Hash Classes.** Define equality comparison and hashing for use in collections.

**Static Semantics**

*[REF: `Eq` signature defined in Appendix D §D.2.1.]*

Types implementing `Hash` MUST also implement `Eq`. If two values are equal per `Eq::eq`, they MUST produce the same hash value.


#### 13.9.5 `Iterator` Class

**Iterator Class.** Defines the iteration protocol for `loop ... in` expressions.

**Syntax**

```cursive
class Iterator {
    type Item
    procedure next(~!) -> Self::Item | ()
}
```

**Static Semantics**

Range types implement `Iterator` when their element type implements `Step`.


#### 13.9.6 `Step` Class

**Step Class.** Defines discrete stepping for range iteration.

**Syntax**

```cursive
class Step {
    procedure successor(~) -> Self | ()
    procedure predecessor(~) -> Self | ()
}
```

---

# Part V: Executable Semantics

---


# Appendix F: The Key System (Normative)

#### 17.1.1 Key Definition

**Key.** A static proof of access rights to a specific path within `shared` data.

**Static Semantics**

Keys are a compile-time verification mechanism. Key state is tracked during type checking. Runtime synchronization is introduced only when static analysis cannot prove safety (see §17.6).

**Key Invariants**

1. **Path-specificity:** A key to path P grants access only to P and paths for which P is a prefix.
2. **Implicit acquisition:** Accessing a `shared` path logically acquires the necessary key.
3. **Scoped lifetime:** Keys are valid for a bounded lexical scope and become invalid when that scope exits.
4. **Reentrancy:** If a key covering path P is already held, nested access to P or any path prefixed by P succeeds without conflict.
5. **Task locality:** Keys are associated with tasks. A key held by a task remains valid until its scope exits.


#### 17.1.2 Key Triple (Path, Mode, Scope)

**Key Triple.** A key consists of: **Path** (the memory location being accessed), **Mode** (`Read` or `Write`), and **Scope** (the lexical scope during which the key is valid).

**Syntax**

**Path Expression Grammar**

```ebnf
key_path_expr   ::= root_segment ("." path_segment)*
root_segment    ::= key_marker? identifier index_suffix?
path_segment    ::= key_marker? identifier index_suffix?
key_marker      ::= "#"
index_suffix    ::= "[" expression "]"
```

**Static Semantics**

**Path Well-Formedness**

A path expression is well-formed if each segment is valid for the type of the previous segment (field access on records, indexing on arrays/slices). At most one `#` marker may appear in the path.

**Key Analysis Requirement**

Key analysis is performed if and only if the path's root has `shared` permission. Paths with `const` or `unique` permission do not require keys.



#### 17.1.3 Read Mode

**Read Mode.** A key mode that permits read-only access to a path. Multiple Read keys to overlapping paths MAY coexist.

**Static Semantics**

**(K-Mode-Read)**
Γ ⊢ e : `shared` T    ReadContext(e)
────────────────────────────────────
RequiredMode(e) = Read

**Read Context Classification**

| Syntactic Position                           | Context |
| :------------------------------------------- | :------ |
| Right-hand side of `let`/`var` initializer   | Read    |
| Right-hand side of assignment (`=`)          | Read    |
| Operand of arithmetic/logical operator       | Read    |
| Argument to `const` or `shared` parameter    | Read    |
| Condition expression (`if`, `loop`, `match`) | Read    |
| Receiver of method with `~` receiver         | Read    |


#### 17.1.4 Write Mode

**Write Mode.** A key mode that permits read and write access to a path. A Write key excludes all other keys to overlapping paths.

**Static Semantics**

**(K-Mode-Write)**
Γ ⊢ e : `shared` T    WriteContext(e)
─────────────────────────────────────
RequiredMode(e) = Write

**Write Context Classification**

| Syntactic Position                                 | Context |
| :------------------------------------------------- | :------ |
| Left-hand side of assignment (`=`)                 | Write   |
| Left-hand side of compound assignment (`+=`, etc.) | Write   |
| Receiver of method with `~!` receiver              | Write   |
| Receiver of method with `~%` receiver              | Write   |
| Argument to `unique` parameter                     | Write   |

**Disambiguation Rule**

If an expression appears in multiple contexts, the more restrictive context (Write) applies.

**Mode Ordering**

Read < Write

A held mode is sufficient for a required mode when:

ModeSufficient(M_held, M_required) ⇔ M_required ≤ M_held

**Constraints**

**Diagnostics:** See Appendix A, code `E-CON-0005`.


#### 17.1.5 Key State Context

**Key State Context.** A mapping Γ_keys : ProgramPoint → ℘(Key) that associates each program point with the set of keys logically held at that point.

**Static Semantics**

**Key Set Operations**

Let Γ_keys be the current key set, a collection of (P, M, S) triples.

**(Acquire)**
Acquire(P, M, S, Γ_keys) = Γ_keys ∪ {(P, M, S)}

**(Release)**
Release(P, Γ_keys) = Γ_keys \ {(P, M, S) : (P, M, S) ∈ Γ_keys}

**(Release by Scope)**
ReleaseScope(S, Γ_keys) = Γ_keys \ {(P, M, S') : S' = S}

**(Mode Transition)**
ModeTransition(P, M_new, Γ_keys) = (Γ_keys \ {(P, M_old, S)}) ∪ {(P, M_new, S)}

**Panic Release Semantics**

PanicRelease(S, Γ_keys) = Γ_keys \ {(P, M, S') : S' ≤_nest S}

All keys held by the panicking scope and its nested scopes are released atomically before panic unwinding proceeds.


#### 17.1.6 Held Predicate

**Held Predicate.** A key is held at a program point if it is a member of the key state context at that point: Held(P, M, S, Γ_keys, p) ⇔ (P, M, S) ∈ Γ_keys(p).

**Static Semantics**

**Key Compatibility**

Two keys K_1 = (P_1, M_1, S_1) and K_2 = (P_2, M_2, S_2) are **compatible** if and only if:

Compatible(K_1, K_2) ⇔ Disjoint(P_1, P_2) ∨ (M_1 = Read ∧ M_2 = Read)

**Compatibility Matrix**

| Key A Mode | Key B Mode | Paths Overlap? | Compatible? |
| :--------- | :--------- | :------------- | :---------- |
| Read       | Read       | Yes            | Yes         |
| Read       | Write      | Yes            | No          |
| Write      | Read       | Yes            | No          |
| Write      | Write      | Yes            | No          |
| Any        | Any        | No (disjoint)  | Yes         |

**Dynamic Semantics**

**Progress Guarantee**

Implementations MUST guarantee eventual progress: any task blocked waiting for a key MUST eventually acquire that key, provided the holder eventually releases it.

Blocked(t, K) ∧ Held(K, t') ∧ ◇ Released(K, t') ⇒ ◇ Acquired(K, t)


#### 17.1.7 Key Roots and Key Path Formation

**Key Root.** The base storage location from which a key path is derived. Key roots arise from: (1) **Lexical roots**: identifiers bound in the current scope; (2) **Boundary roots**: runtime object identities created at key boundaries (pointer indirection and type-level boundaries). For any place expression e with `shared` permission, the **key path** KeyPath(e) is the normalized `key_path_expr` used for key acquisition and conflict analysis.

**Static Semantics**

**Path Root Extraction**

Define Root(e) for place expressions (§3.4.2) recursively:
Root(e) =
 x                 if e = x
 Root(e')          if e = e'.f
 Root(e')          if e = e'[i]
 Root(e')          if e = e' ~> m(...)
 ⊥_boundary        if e = (*e')

where ⊥_boundary indicates a **key boundary** introduced by pointer dereference.

**Object Identity**

The **identity** of a reference or pointer r, written id(r), is a unique runtime value denoting the storage location referred to by r.

Implementations MUST ensure:
1. **Uniqueness:** id(r_1) = id(r_2) iff r_1 and r_2 refer to overlapping storage.
2. **Stability:** id(r) remains constant for the lifetime of the referent.
3. **Opacity:** identities are not directly observable except through key semantics.

**KeyPath Formation**

Let e be a place expression accessing `shared` data and let e’s field/index tail be p_2 … p_n.

- **Lexical root case:** If Root(e) = x, then
  KeyPath(e) = x.p_2 … p_n truncated by any type-level boundary.

- **Boundary root case:** If Root(e) = ⊥_boundary and e = (*e').p_2 … p_n, then
  KeyPath(e) = id(*e').p_2 … p_n truncated by any type-level boundary.

Type-level boundaries are defined in §17.2.4.

**Pointer Indirection**

A pointer dereference establishes a new key boundary. Key paths do not extend across pointer indirections.

For `(*e').p` where `e' : shared Ptr<T>@Valid`:
1. Dereferencing `e'` requires a key to KeyPath(e') in Read mode.
2. Accessing `.p` on the dereferenced value uses a fresh key rooted at id(*e').

**Constraints**

**Diagnostics:** See Appendix A, code `E-CON-0034`.

### 17.2 Acquisition and `#` Blocks


#### 17.2.1 Implicit Key Acquisition

**Implicit Acquisition.** Keys are logically acquired on demand during expression evaluation and released at scope exit.

**Static Semantics**

**Key Lifecycle Phases**

1. **Acquisition Phase:** Keys are logically acquired as `shared` paths are evaluated, in evaluation order
2. **Execution Phase:** The statement or block body executes with all keys logically held
3. **Release Phase:** All keys become invalid when the scope exits

**Covered Predicate**

An access to path Q requiring mode M_Q is covered by the current key state if:

Covered(Q, M_Q, Γ_keys) ⇔ ∃ (P, M_P, S) ∈ Γ_keys : Prefix(P, Q) ∧ ModeSufficient(M_P, M_Q)

**Acquisition Rules**

**(K-Acquire-New)**
Γ ⊢ P : `shared` T    M = RequiredMode(P)    ¬ Covered(P, M, Γ_keys)    S = CurrentScope
──────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ'_keys = Γ_keys ∪ {(P, M, S)}

**(K-Acquire-Covered)**
Γ ⊢ P : `shared` T    M = RequiredMode(P)    Covered(P, M, Γ_keys)
──────────────────────────────────────────────────────────────
Γ'_keys = Γ_keys

**Evaluation Order**

Subexpressions are evaluated left-to-right, depth-first. Key acquisition follows evaluation order.

**(K-Eval-Order)**
eval(e_1 ⊕ e_2) → eval(e_1); eval(e_2); apply(⊕)

**Constraints**

**Diagnostics:** See Appendix A, codes `W-CON-0001`, `W-CON-0002`.


#### 17.2.2 Key Release

**Key Release.** Keys are released when their defining scope exits.

**Static Semantics**

**(K-Release-Scope)**
ScopeExit(S)
──────────────────────────────────────────────────────────────────
Γ'_keys = Γ_keys \ {(P, M, S') : S' = S}

**(K-Release-Order)**

Keys acquired within a scope are released per LIFO semantics (§3.5.2).

**Dynamic Semantics**

**Release Guarantee**

Keys are released when their scope exits, regardless of exit mechanism:

| Exit Mechanism    | Keys Released? |
| :---------------- | :------------- |
| Normal completion | Yes            |
| `return`          | Yes            |
| `break`           | Yes            |
| `continue`        | Yes            |
| Panic propagation | Yes            |
| Task cancellation | Yes            |

**Key and RAII Cleanup Interaction**

Keys are released at scope exit before any `Drop::drop` calls for bindings in that scope (see §3.5).

**Scope Definitions**

| Construct                      | Key Scope                  | Release Point          |
| :----------------------------- | :------------------------- | :--------------------- |
| Expression statement (`expr;`) | The statement              | Semicolon              |
| `let`/`var` declaration        | The declaration            | Semicolon              |
| Assignment statement           | The statement              | Semicolon              |
| `if` condition                 | The condition only         | Before entering branch |
| `match` scrutinee              | The scrutinee only         | Before entering arm    |
| `loop` condition               | Each iteration's condition | Before entering body   |
| `#path { ... }` block          | The entire block           | Closing brace          |
| Procedure body                 | Callee's body              | Procedure return       |
| `defer` body                   | The defer block            | Defer completion       |

**Constraints**

**Diagnostics:** See Appendix A, code `E-CON-0006`.


#### 17.2.3 `#` Block Syntax

**Key Block.** The `#` key block is the explicit key acquisition construct. It acquires a key to one or more paths at block entry and holds those keys until block exit.

**Syntax**

```ebnf
key_block       ::= "#" path_list mode_modifier* block
path_list       ::= key_path_expr ("," key_path_expr)*
mode_modifier   ::= "write" | "read" | release_modifier | "ordered" | "speculative"
release_modifier ::= "release" ("write" | "read")
```

**Mode Semantics**

| Block Form        | Key Mode | Mutation Permitted |
| :---------------- | :------- | :----------------- |
| `#path { }`       | Read     | No                 |
| `#path write { }` | Write    | Yes                |

**Static Semantics**

**(K-Block-Acquire)**
Γ ⊢ P_1, …, P_m : `shared` T_i    M = BlockMode(B)    (Q_1, …, Q_m) = CanonicalSort(P_1, …, P_m)    S = NewScope
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ'_keys = Γ_keys ∪ {(Q_i, M, S) : i ∈ 1..m}

**(K-Read-Block-No-Write)**
BlockMode(B) = Read    P ∈ KeyedPaths(B)    WriteOf(P) ∈ Body(B)
──────────────────────────────────────────────────────────────────────────────
Emit(`E-CON-0070`)

**Dynamic Semantics**

**Execution Order**

1. Acquire keys (mode per block specification) to all listed paths in canonical order
2. Execute the block body
3. Release all keys in reverse acquisition order

**Concurrent Access**

| Block A        | Block B        | Same/Overlapping Path | Result                          |
| :------------- | :------------- | :-------------------- | :------------------------------ |
| `#p { }`       | `#p { }`       | Yes                   | Both proceed concurrently       |
| `#p { }`       | `#p write { }` | Yes                   | One blocks until other releases |
| `#p write { }` | `#p write { }` | Yes                   | One blocks until other releases |

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CON-0001`–`E-CON-0004`, `E-CON-0030`–`E-CON-0032`, `E-CON-0070`, `W-CON-0003`.


#### 17.2.4 Key Coarsening

**Key Coarsening.** The `#` marker in a path expression sets the key granularity. The key is acquired at the marked position, covering all subsequent segments.

**Syntax**

**Inline Coarsening**

```ebnf
coarsened_path  ::= path_segment* "#" path_segment+
```

**Static Semantics**

**(K-Coarsen-Inline)**
P = p_1 … p_(k-1).#p_k … p_n    Γ ⊢ P : `shared` T    M = RequiredMode(P)
────────────────────────────────────────────────────────────────────────────
AcquireKey(p_1 … p_k, M, Γ_keys)

**Type-Level Key Boundary**

A field declared with `#` establishes a permanent key boundary:

```ebnf
key_field_decl      ::= "#"? visibility? identifier ":" type
```

**(K-Type-Boundary)**
Γ ⊢ r : R    R.fields ∋ (#f : U)    P = r.f.q_1 … q_n
────────────────────────────────────────────────────────────
KeyPath(P) = r.f

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CON-0033`, `W-CON-0013`.


#### 17.2.5 Closure Capture of `shared` Bindings

**Shared Closure Capture.** Closures that capture `shared` bindings participate in key analysis. Because a closure may be invoked outside its defining scope, key path roots depend on whether the closure escapes.

**Classification**

A closure expression C is **local** if it appears only in:
1. Argument position of an immediate procedure or method call
2. The body of a `spawn` expression
3. The body of a `dispatch` expression
4. Operand of immediate invocation

A closure is **escaping** if it is:
1. Bound to a `let` or `var` binding
2. Returned from a procedure
3. Stored into a record or enum field

Let SharedCaptures(C) be the set of captured bindings with `shared` permission.

**Static Semantics**

**Local Closure Key Paths**

For a local closure, key analysis treats the closure body as if it executed in the defining scope. Any access `x.p` where `x ∈ SharedCaptures(C)` uses lexical rooting (§17.1.7):

KeyPath(C, x.p) = KeyPath(x.p)

Keys are acquired at invocation, not at definition.

**Escaping Closure Type Requirement**

An escaping closure that captures `shared` bindings MUST carry a shared dependency set in its closure type (§12.4.3):

**(K-Closure-Escape-Type)**
C is escaping    SharedCaptures(C) = {x_1, …, x_n}
─────────────────────────────────────────────────────────────────────────────────────────────────────────────
Type(C) = |vec_T| → R [`shared`: {x_1 : `shared` T_1, …, x_n : `shared` T_n}]

The dependency set MAY be inferred when the closure is assigned to a binding with an expected closure type; otherwise it MUST be written explicitly.

**Escaping Closure Key Paths**

For an escaping closure, key paths are rooted at runtime identities of captured references:

**(K-Closure-Escape-Keys)**
C : |vec_T| → R [`shared`: {x : `shared` T}]    Access(x.p, M) ∈ C.body
─────────────────────────────────────────────────────────────────────────────
KeyPath(C, x.p) = id(C.x).p    KeyMode = M

where C.x denotes the captured reference to x stored in the closure environment.

**Lifetime Constraint**

An escaping closure MUST NOT outlive any captured `shared` binding. Implementations MUST reject any flow where the closure’s lifetime exceeds the lifetime of a captured `shared` root.

**Dynamic Semantics**

**Local Closure Invocation**

1. Determine accessed `shared` paths in the closure body.
2. Acquire required keys using lexical roots.
3. Execute the closure body.
4. Release keys at the end of the invocation.

**Escaping Closure Invocation**

1. For each dependency (x : shared T) in the closure type, let r = C.x.
2. Acquire required keys for paths rooted at id(r).
3. Execute the closure body.
4. Release keys at the end of the invocation.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CON-0085`, `E-CON-0086`, `W-CON-0009`.


#### 17.2.6 `shared` Dynamic Class Objects (`shared $Class`)

**Shared Dynamic Class.** Dynamic class objects (`$Cl`, §13.5) erase the concrete type at runtime. When such an object is viewed with `shared` permission, key analysis cannot inspect the concrete method bodies chosen by vtable dispatch. Therefore, `shared $Cl` is restricted to read-only dynamic interfaces.

**Static Semantics**

**Well-Formedness**

A dynamic class object type may be qualified with `shared` permission only if every vtable-eligible procedure in the class (including inherited ones) has a `const` receiver (`~`):

Let DynMethods(Cl) denote the set of procedures callable via vtable dispatch on `$Cl` (i.e., vtable‑eligible and not marked `[[static_dispatch_only]]`).

**(K-Witness-Shared-WF)**
∀ m ∈ DynMethods(Cl). m.receiver = `~`
──────────────────────────────────────────────
Γ ⊢ `shared` $Cl wf

If any method requires `shared` (`~%`) or `unique` (`~!`) receiver permission, `shared $Cl` is ill‑formed. Such methods may still exist on `Cl` if they are excluded from dynamic dispatch via `[[static_dispatch_only]]`.

**Dynamic Calls Through `shared $Cl`**

For a call `e~>m(args)` where `e : shared $Cl`, the key mode is Read and the key path is the root of `e`:

KeyPath(e~>m(...)) = Root(e)    KeyMode = Read

**Constraints**

**Diagnostics:** See Appendix A, code `E-CON-0083`.


### 17.3 Conflict Detection


#### 17.3.1 Path Prefix and Disjointness

**Path Relations.** The prefix relation and disjointness relation on paths determine key coverage and static safety of concurrent accesses.

**Static Semantics**

**Path Prefix**

Path P is a prefix of path Q (written Prefix(P, Q)) if P is an initial subsequence of Q:

Prefix(p_1 … p_m, q_1 … q_n) ⇔ m ≤ n ∧ ∀ i ∈ 1..m, p_i ≡_seg q_i

**Path Disjointness**

Two paths P and Q are disjoint (written Disjoint(P, Q)) if neither is a prefix of the other:

Disjoint(P, Q) ⇔ ¬ Prefix(P, Q) ∧ ¬ Prefix(Q, P)

**Segment Equivalence**

p_i ≡_seg q_i ⇔ name(p_i) = name(q_i) ∧ IndexEquiv(p_i, q_i)

**Index Expression Equivalence**

Two index expressions e_1 and e_2 are provably equivalent (e_1 ≡_idx e_2) if:

1. Both are the same integer literal
2. Both are references to the same `const` binding
3. Both are references to the same generic const parameter
4. Both are references to the same variable binding in scope
5. Both normalize to the same canonical form under constant folding

**Disjointness for Static Safety**

**(K-Disjoint-Safe)**
Disjoint(P, Q)
──────────────────────────────────────────────────
ConcurrentAccess(P, Q) is statically safe

**Prefix for Coverage**

**(K-Prefix-Coverage)**
Prefix(P, Q)    Held(P, M, Γ_keys)
──────────────────────────────────
Covers((P, M), Q)


#### 17.3.2 Static Conflict Analysis

**Static Conflict Analysis.** When multiple dynamic indices access the same array within a single statement, they MUST be provably disjoint; otherwise, the program is ill-formed.

**Static Semantics**

**Provable Disjointness for Indices**

Two index expressions are provably disjoint (ProvablyDisjoint(e_1, e_2)) if:

1. **Static Disjointness:** Both are statically resolvable with different values
2. **Control Flow Facts:** A Verification Fact establishes e_1 ≠ e_2
3. **Contract-Based:** A precondition asserts e_1 ≠ e_2
4. **Refinement Types:** An index has a refinement type constraining its relationship
5. **Algebraic Offset:** Expressions share a common base but differ by constant offsets
6. **Dispatch Iteration:** Within a `dispatch` block, accesses indexed by the iteration variable are automatically disjoint
7. **Disjoint Loop Ranges:** Iteration variables from loops with non-overlapping ranges

**(K-Dynamic-Index-Conflict)**
P_1 = a[e_1]    P_2 = a[e_2]    SameStatement(P_1, P_2)    (Dynamic(e_1) ∨ Dynamic(e_2))    ¬ ProvablyDisjoint(e_1, e_2)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Emit(`E-CON-0010`)

**Constraints**

**Diagnostics:** See Appendix A, code `E-CON-0010`.


#### 17.3.3 Read-Then-Write Prohibition

**Read-Then-Write.** A pattern that occurs when a statement contains both a read and a write to the same `shared` path without a covering Write key.

**Static Semantics**

**Formal Definition**

ReadThenWrite(P, S) ⇔ ∃ e_r, e_w ∈ Subexpressions(S) : ReadsPath(e_r, P) ∧ WritesPath(e_w, P)

**(K-Read-Write-Reject)**
Γ ⊢ P : `shared` T    ReadThenWrite(P, S)    ¬ ∃ (Q, Write, S') ∈ Γ_keys : Prefix(Q, P)
────────────────────────────────────────────────────────────────────────────────────────────────────────────
Emit(`E-CON-0060`)

**(K-RMW-Permitted)**
Γ ⊢ P : `shared` T    ReadThenWrite(P, S)    ∃ (Q, Write, S') ∈ Γ_keys : Prefix(Q, P)
────────────────────────────────────────────────────────────────────────────────────────────────────────────
Permitted

**Pattern Classification**

| Pattern                  | Covering Write Key?         | Action                |
| :----------------------- | :-------------------------- | :-------------------- |
| `p += e`                 | Yes (desugars to `#` block) | Permitted             |
| `#p write { p = p + e }` | Yes                         | Permitted             |
| `p = p + e`              | No                          | **Reject E-CON-0060** |
| `p.a = p.b + 1`          | No, but disjoint paths      | Permitted             |

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CON-0060`, `W-CON-0004`, `W-CON-0006`.


### 17.4 Nested Release


#### 17.4.1 Nested Key Semantics

**Nested Keys.** Nested `#` blocks acquire keys independently. The outer block's keys remain held while the inner block executes.

**Syntax**

The `release` modifier enables mode transitions in nested blocks:

| Outer Mode | `release` Target | Operation                                      |
| :--------- | :--------------- | :--------------------------------------------- |
| Read       | `write`          | Escalation: Release Read → Gap → Acquire Write |
| Write      | `read`           | Downgrade: Release Write → Gap → Acquire Read  |

**Static Semantics**

**(K-Nested-Same-Path)**
Held(P, M_outer, Γ_keys)    #P M_inner { ... }
──────────────────────────────────────────────────────────────
if M_inner = M_outer:
 Covered (no acquisition)
otherwise if M_inner ≠ M_outer ∧ `release` ∈ modifiers:
 Release-and-Reacquire per §17.4.1
otherwise:
 Emit(`E-CON-0012`)

**Dynamic Semantics**

**Release-and-Reacquire Sequence**

When entering `#path release <target> { body }`:

1. **Release:** Release the outer key held by the enclosing block
2. **Acquire:** Acquire the target mode key to `path` (blocking if contended)
3. **Execute:** Evaluate `body`
4. **Release:** Release the inner key
5. **Reacquire:** Acquire the outer mode key for the enclosing block's remaining scope

**(K-Release-Sequence)**
Held(P, M_outer, S_outer)    #P `release` M_inner { B }
────────────────────────────────────────────────────────────
Release(P, Γ_keys);
Acquire(P, M_inner, S_inner);
Eval(B);
Release(P, Γ_keys);
Acquire(P, M_outer, S_outer)

**Interleaving Windows**

Between steps 1 and 2 (entry), and between steps 4 and 5 (exit), other tasks MAY acquire keys to the same path.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CON-0011`, `E-CON-0012`, `E-CON-0017`, `E-CON-0018`, `W-CON-0010`–`W-CON-0012`.

**Staleness Suppression (`[[stale_ok]]`)**

`[[stale_ok]]` suppresses warning `W-CON-0011` on a binding derived from `shared` data across a `release` or `yield release` boundary. See §7.4.5 for the full definition. The attribute does not affect dynamic semantics.


#### 17.4.2 Key Reentrancy

**Key Reentrancy.** If a caller holds a key covering the callee's access paths, the callee's accesses are covered without additional acquisition.

**Static Semantics**

**(K-Reentrant)**
Held(P, M, Γ_keys)    Prefix(P, Q)    CalleeAccesses(Q)
──────────────────────────────────────────────────────────
CalleeCovered(Q)

**(K-Procedure-Boundary)**

Passing a `shared` value as a procedure argument does not constitute key acquisition:

Γ ⊢ f : (`shared` T) → R    Γ ⊢ v : `shared` T
──────────────────────────────────────────────────────────────
call(f, v) → no key acquisition at call site

**Constraints**

**Diagnostics:** See Appendix A, code `W-CON-0005`.


### 17.5 Speculative Execution


#### 17.5.1 Speculative Block Semantics

**Speculative Block.** A key block that executes without acquiring an exclusive key, relying on optimistic concurrency control. The block body executes against a snapshot; at block exit, writes are atomically committed if and only if the snapshot values remain unchanged.

**Syntax**

```ebnf
speculative_block ::= "#" path_list "speculative" "write" block
```

**Static Semantics**

**(K-Spec-Write-Required)**
#P `speculative` M {B}    M ≠ `write`
────────────────────────────────────────
Emit(`E-CON-0095`)

**(K-Spec-Pure-Body)**
#P `speculative write` {B}    Writes(B) ⊄ CoveredPaths(P)
──────────────────────────────────────────────────────────
Emit(`E-CON-0091`)

**Permitted Operations**

- Read from keyed paths
- Write to keyed paths
- Pure computation (arithmetic, logic, local bindings)
- Procedure calls to `const` receiver methods on keyed data

**Prohibited Operations**

- Write to paths outside the keyed set
- Nested key blocks
- `wait` expressions
- Procedure calls with side effects
- `defer` statements

**(K-Spec-No-Nested-Key)**
#P `speculative write` {B}    #Q _ {…} ∈ Subexpressions(B)
──────────────────────────────────────────────────────────
Emit(`E-CON-0090`)


#### 17.5.2 Snapshot-Execute-Commit Cycle

**Speculative Commit.** SpeculativeCommit(R, W) ⇔ Atomic(⋀_((p, v) ∈ R) p = v ⇒ ⋀_((q, w) ∈ W) q := w)

**Dynamic Semantics**

**Execution Model**

1. **Initialize**: Set retries := 0
2. **Snapshot**: Read all paths in the keyed set, recording (path, value) pairs in read set R
3. **Execute**: Evaluate body, collecting writes in write set W
4. **Commit**: Atomically verify ⋀_((p, v) ∈ R) (current(p) = v) and, if true, apply all writes in W
5. **On success**: Return the value of body
6. **On failure**: Increment retries; if below limit, goto step 2; otherwise fallback
7. **Fallback**: Acquire a blocking Write key, execute body, release key, return value

**Retry Limit**

`MAX_SPECULATIVE_RETRIES` is IDB.

**Interaction with Blocking Keys**

| Concurrent Operation      | Speculative Block Behavior                  |
| :------------------------ | :------------------------------------------ |
| Another speculative block | Race; one commits, others retry             |
| Blocking Read key held    | Speculative may commit (compatible)         |
| Blocking Write key held   | Speculative commit fails, retry or fallback |

**Snapshot and Commit Requirements**

The snapshot step (2) MUST be observationally equivalent to an atomic snapshot over the keyed set. The commit step (4) MUST be atomic with respect to all other key operations on overlapping paths and MUST satisfy the `SpeculativeCommit` predicate above. The concrete mechanisms used to realize these properties are Implementation‑Defined (IDB).

*[Informative: implementations typically use atomic loads or seqlock‑style reads for snapshots and CAS or brief locks for commits. Hidden version counters may be used as an implementation technique.]* 


#### 17.5.3 Speculation and Panics

**Dynamic Semantics**

If a panic occurs during execution (step 3):

1. The write set W is discarded (no writes are committed)
2. No key is held (nothing to release)
3. The panic propagates normally

**Memory Ordering**

- Snapshot reads: Acquire semantics
- Successful commit: Release semantics
- Failed commit: No ordering guarantees (writes discarded)

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CON-0090`–`E-CON-0096`, `W-CON-0020`, `W-CON-0021`.


### 17.6 Runtime Realization (`[[dynamic]]`)


#### 17.6.1 Dynamic Key Verification

**Dynamic Verification.** Keys are a compile-time abstraction. Runtime synchronization is emitted only when the `[[dynamic]]` attribute is present and static verification fails.

**Static Semantics**

**Static Safety Conditions**

| Condition              | Description                                                 | Rule   |
| :--------------------- | :---------------------------------------------------------- | :----- |
| **No escape**          | `shared` value never escapes to another task                | K-SS-1 |
| **Disjoint paths**     | Concurrent accesses target provably disjoint paths          | K-SS-2 |
| **Sequential context** | No `parallel` block encloses the access                     | K-SS-3 |
| **Unique origin**      | Value is `unique` at origin, temporarily viewed as `shared` | K-SS-4 |
| **Dispatch-indexed**   | Access indexed by `dispatch` iteration variable             | K-SS-5 |
| **Speculative-only**   | All accesses within speculative blocks with fallback        | K-SS-6 |

**(K-Static-Safe)**
Access(P, M)    StaticallySafe(P)
────────────────────────────────
NoRuntimeSync(P)

**(K-Static-Required)**
¬ StaticallySafe(P)    ¬ InDynamicContext
──────────────────────────────────────────
Emit(`E-CON-0020`)

**(K-Dynamic-Permitted)**
¬ StaticallySafe(P)    InDynamicContext
────────────────────────────────────────
EmitRuntimeSync(P)

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CON-0014`, `E-CON-0020`, `I-CON-0011`, `I-CON-0013`.


#### 17.6.2 Runtime Lock Acquisition

**Dynamic Semantics**

**Runtime Realization**

When runtime synchronization is required:

1. Mutual exclusion per key compatibility rules (§17.1.6)
2. Blocking when incompatible keys are held
3. Release on scope exit (including panic)
4. Progress guarantee (no indefinite starvation)

**Canonical Order**

A canonical order on paths defines deterministic acquisition order:

P <_canon Q ⇔ ∃ k ≥ 1 : (∀ i < k, p_i =_seg q_i) ∧ (p_k <_seg q_k ∨ (k > m ∧ m < n))

**Segment Ordering**

1. **Identifiers:** Lexicographic by UTF-8 byte sequence
2. **Indexed segments:** By index value for statically resolvable indices
3. **Bare vs indexed:** Bare identifier precedes any indexed form

**Dynamic Ordering Guarantee**

Within `[[dynamic]]` scope, incomparable dynamic indices require runtime ordering satisfying:

1. **Totality**: For distinct paths P and Q, exactly one of DynOrder(P, Q) or DynOrder(Q, P) holds
2. **Antisymmetry**: DynOrder(P, Q) ∧ DynOrder(Q, P) ⇒ P = Q
3. **Transitivity**: DynOrder(P, Q) ∧ DynOrder(Q, R) ⇒ DynOrder(P, R)
4. **Cross-Task Consistency**: All tasks compute the same ordering
5. **Value-Determinism**: Ordering depends only on runtime values, not timing

**Observational Equivalence**

∀ P : Observable(StaticSafe(P)) = Observable(RuntimeSync(P))

**Constraints**

**Diagnostics:** See Appendix A, code `W-CON-0021`.


### 17.7 Memory Ordering


#### 17.7.1 Default Sequential Consistency

**Sequential Consistency.** Key acquisition uses acquire semantics. Key release uses release semantics. Memory accesses default to sequentially consistent ordering.


#### 17.7.2 Relaxed Ordering Levels

**Syntax**

```cursive
[[relaxed]]
counter += 1

[[acquire]]
let ready = flag

[[release]]
flag = true
```

| Ordering  | Guarantee                             |
| :-------- | :------------------------------------ |
| `relaxed` | Atomicity only—no ordering            |
| `acquire` | Subsequent reads see prior writes     |
| `release` | Prior writes visible to acquire reads |
| `acq_rel` | Both acquire and release              |
| `seq_cst` | Total global order (default)          |


#### 17.7.3 Memory Ordering Attributes

**Syntax**

```ebnf
memory_order_attribute ::= "[[" memory_order "]]"
memory_order           ::= "relaxed" | "acquire" | "release" | "acq_rel" | "seq_cst"
```

**Static Semantics**

Memory ordering annotations are permitted inside standard `#` blocks. The annotation affects only the data access ordering; it does not modify the key's acquire/release semantics at block entry and exit.

Memory ordering annotations MUST NOT appear inside speculative blocks (§17.5).


#### 17.7.4 Fence Operations

**Syntax**

```cursive
fence(acquire)
fence(release)
fence(seq_cst)
```

---

# Part VI: Concurrency


# Appendix G: Structured Parallelism (Normative)

## 18. Structured Parallelism

---

### 18.1 Parallel Blocks


#### 18.1.1 `parallel` Block Syntax

**Parallel Block.** A scope within which work executes concurrently across multiple workers. A parallel block P is defined as P = (D, A, B) where D is the execution domain expression, A is the set of block options, and B is the block body.

**Syntax**

```ebnf
parallel_block    ::= "parallel" domain_expr block_options? block

domain_expr       ::= expression

block_options     ::= "[" block_option ("," block_option)* "]"

block_option      ::= "cancel" ":" expression
                    | "name" ":" string_literal

block             ::= "{" statement* "}"
```

| Option   | Type          | Effect                                       |
| :------- | :------------ | :------------------------------------------- |
| `cancel` | `CancelToken` | Provides cooperative cancellation capability |
| `name`   | `string`      | Labels the block for debugging and profiling |

**Static Semantics**

**Typing Rule**

Γ ⊢ D : `$ExecutionDomain`    Γ_P = Γ[parallel_context ↦ D]    Γ_P ⊢ B : T
────────────────────────────────────────────────────────────────────────────
Γ ⊢ `parallel` D {B} : T    (T-Parallel)

**Well-Formedness**

A parallel block is well-formed if and only if:

1. The domain expression evaluates to a type implementing `ExecutionDomain`.
2. All `spawn` and `dispatch` expressions within the block body reference the enclosing parallel block.
3. No `spawn` or `dispatch` expression appears outside a parallel block.
4. All captured bindings satisfy the permission requirements of §18.3.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CON-0101`–`E-CON-0103`.


#### 18.1.2 Fork-Join Semantics

**Structured Concurrency Invariant.** Let 𝒲 denote the set of work items spawned within a parallel block P: ∀ w ∈ 𝒲. lifetime(w) ⊆ lifetime(P). A **work item** is a unit of computation queued for execution by a worker (created by `spawn` and `dispatch` expressions). A **worker** is an execution context that executes work items. A **task** is the runtime representation of a work item during execution; tasks may suspend and resume, and are associated with key state per §17.1.

**Dynamic Semantics**

Evaluation of `parallel D [opts] { B }`:

1. Evaluate domain expression D to obtain d.
2. Initialize the worker pool according to d's configuration.
3. If the `cancel` option is present, associate its token with the block.
4. Evaluate statements in B sequentially; `spawn` and `dispatch` expressions enqueue work items.
5. Block at the closing brace until all enqueued work items complete.
6. If any work item panicked, propagate the first panic (by completion order) after all work settles.
7. Release workers back to the domain.
8. Return the collected results per §18.1.3.

Work items complete in Unspecified order. The parallel block guarantees only that ALL work completes before the block exits.


#### 18.1.3 Parallel Block Completion

**Parallel Completion.** A parallel block is an expression that yields a value. The result type depends on the block's contents.

**Static Semantics**

**Result Type Determination**

| Block Contents                     | Result Type          |
| :--------------------------------- | :------------------- |
| No `spawn` or `dispatch`           | `()`                 |
| Single `spawn { e }` where e : T | T                   |
| Multiple spawns e_1, …, e_n     | (T_1, …, T_n)        |
| `dispatch` without `[reduce]`    | `()`                 |
| `dispatch ... [reduce: op] { e }`| T where e : T        |
| Mixed spawns and reducing dispatch | Tuple of all results |

**Typing Rules**

────────────────────────────────────────────
Γ ⊢ `parallel` D {} : ()    (T-Parallel-Empty)

Γ ⊢ `spawn` {e} : SpawnHandle⟨T⟩
────────────────────────────────────────────────────────
Γ ⊢ `parallel` D {`spawn` {e}} : T    (T-Parallel-Single)

Γ ⊢ `spawn` {e_i} : SpawnHandle⟨T_i⟩    ∀ i ∈ 1..n
──────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ `parallel` D {`spawn` {e_1}; …; `spawn` {e_n}} : (T_1, …, T_n)    (T-Parallel-Multi)


### 18.2 Execution Domains


#### 18.2.1 CPU Domain

**CPU Domain.** An execution domain that executes work items on operating system threads managed by the runtime.

**Static Semantics**

The runtime provides a CPU execution domain via `ctx.cpu(...)`. This constructor, its parameters, and helper types (e.g., `CpuSet`, `Priority`) are core‑library/runtime interfaces and are not normative in this document. Any value produced by `ctx.cpu(...)` MUST implement `ExecutionDomain`.


#### 18.2.2 GPU Domain

**GPU Domain.** An execution domain that executes work items on graphics processing unit compute shaders.

**Static Semantics**

The runtime may provide a GPU execution domain via `ctx.gpu(...)`. Its constructor parameters, buffer types, and GPU intrinsics are core‑library/runtime interfaces and are not normative here. Within GPU-domain work items, capturing `shared` bindings, host pointers, or heap‑provenanced values is ill‑formed; only GPU‑accessible data may be captured.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CON-0150`–`E-CON-0152`.


#### 18.2.3 Inline Domain

**Inline Domain.** An execution domain that executes work items sequentially on the current thread.

**Syntax**

```cursive
ctx.inline()
```

**Dynamic Semantics**

- `spawn { e }` evaluates e immediately and blocks until complete.
- `dispatch i in range { e }` executes as a sequential loop.
- No actual parallelism occurs.
- All capture rules and permission requirements remain enforced.


#### 18.2.4 Domain Selection

**Execution Domain.** A capability that provides access to computational resources. Domains implement the `ExecutionDomain` class.

**Static Semantics**

```cursive
class ExecutionDomain {
    procedure name(~) -> string
    procedure max_concurrency(~) -> usize
}
```

This class is dispatchable, enabling heterogeneous domain handling.


### 18.3 Capture Semantics

See §10.1 for permission definitions. This section defines parallel-specific capture constraints.


#### 18.3.1 Capture Rules for `const`

**Const Capture.** Bindings with `const` permission may be captured by reference into `spawn` and `dispatch` bodies. Captured `const` references are guaranteed valid because structured concurrency (§18.1.2) ensures all work completes before the parallel block exits.

**Static Semantics**

`const` capture does not transfer responsibility. Multiple work items may capture the same `const` binding concurrently.


#### 18.3.2 Capture Rules for `shared`

**Shared Capture.** Bindings with `shared` permission may be captured by reference into `spawn` and `dispatch` bodies. Accesses are synchronized via the key system (§17).

**Static Semantics**

`shared` capture does not transfer responsibility. Concurrent access follows key acquisition semantics per §17.2.


#### 18.3.3 Capture Rules for `unique` (Move Required)

See §3.4 for move semantics and §10.1.2 for `unique` permission rules.

**Unique Capture.** Bindings with `unique` permission MUST NOT be implicitly captured. To use a `unique` binding in a work item, explicit `move` is required. Only one work item may receive ownership.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CON-0120`–`E-CON-0122`.


### 18.4 Tasks and Spawning


#### 18.4.1 `spawn` Expression Syntax

**Spawn Expression.** An expression that creates a work item for concurrent execution within the enclosing parallel block.

**Syntax**

```ebnf
spawn_expr      ::= "spawn" spawn_option_list? block

spawn_option_list  ::= "[" spawn_option ("," spawn_option)* "]"

spawn_option       ::= "name" ":" string_literal
                    | "affinity" ":" expression
                    | "priority" ":" expression
```

| Attribute  | Type       | Default            | Effect                         |
| :--------- | :--------- | :----------------- | :----------------------------- |
| `name`     | `string`   | Anonymous          | Labels for debugging/profiling |
| `affinity` | `CpuSet`   | Domain default     | CPU core affinity hint         |
| `priority` | `Priority` | `Priority::Normal` | Scheduling priority hint       |

**Static Semantics**

**Typing Rule**

Γ[parallel_context] = D    Γ_capture ⊢ e : T
────────────────────────────────────────────────────────────
Γ ⊢ `spawn` {e} : SpawnHandle⟨T⟩    (T-Spawn)

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CON-0130`, `E-CON-0131`.


#### 18.4.2 Task Creation

**SpawnHandle Type.** A modal type representing a pending or completed spawn result:

```cursive
modal SpawnHandle<T> {
    @Pending { }
    @Ready { value: T }
}
```

**Dynamic Semantics**

Evaluation of `spawn [attrs] { e }`:

1. Capture free variables from enclosing scope per §18.3.
2. Package the captured environment with expression e into a work item.
3. Enqueue the work item to the parallel block's worker pool.
4. Return `SpawnHandle<T>@Pending` immediately (non-blocking).
5. A worker eventually dequeues and evaluates e.
6. Upon completion, the handle transitions to `@Ready` with the result value.


#### 18.4.3 Task Result Collection

**Result Collection.** Spawn results are collected through three mechanisms:

| Mechanism           | Description                                                                 |
| :------------------ | :-------------------------------------------------------------------------- |
| Implicit collection | When a parallel block contains only spawn expressions, results form a tuple |
| Explicit wait       | Use `wait handle` to retrieve a specific result                             |
| Ignored             | Results not collected are discarded; work still completes                   |

**Dynamic Semantics**

`SpawnHandle<T>` supports the `wait` expression for result retrieval. Per §17, keys MUST NOT be held across `wait` suspension points.


### 18.5 Data Parallelism (`dispatch`)


#### 18.5.1 `dispatch` Expression Syntax

**Dispatch Expression.** An expression that expresses data-parallel iteration where each iteration may execute concurrently.

**Syntax**

```ebnf
dispatch_expr   ::= "dispatch" pattern "in" range_expression
                    key_clause?
                    dispatch_option_list?
                    block

key_clause      ::= "key" key_path_expr key_mode

key_mode        ::= "read" | "write"

dispatch_option_list  ::= "[" dispatch_option ("," dispatch_option)* "]"

dispatch_option       ::= "reduce" ":" reduce_op
                        | "ordered"
                        | "chunk" ":" expression

reduce_op       ::= "+" | "*" | "min" | "max" | "and" | "or" | identifier
```

| Attribute | Type         | Effect                                        |
| :-------- | :----------- | :-------------------------------------------- |
| `reduce`  | Reduction op | Combines iteration results using specified op |
| `ordered` | (flag)       | Forces sequential side-effect ordering        |
| `chunk`   | `usize`      | Groups iterations into chunks for granularity |

**Static Semantics**

**Typing Rule (Without Reduction)**

Γ ⊢ range : Range⟨I⟩    Γ, i : I ⊢ B : T
──────────────────────────────────────────────────────────
Γ ⊢ `dispatch` i `in range` {B} : ()    (T-Dispatch)

**Typing Rule (With Reduction)**

Γ ⊢ range : Range⟨I⟩    Γ, i : I ⊢ B : T    Γ ⊢ ⊕ : (T, T) → T
─────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ `dispatch` i `in range [reduce: ⊕]` {B} : T    (T-Dispatch-Reduce)

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CON-0140`–`E-CON-0143`, `W-CON-0140`.


#### 18.5.2 Iteration Space

**Dynamic Semantics**

Evaluation of `dispatch i in range [attrs] { B }`:

1. Evaluate `range` to determine iteration count n.
2. Analyze key patterns to partition iterations into conflict-free groups.
3. For each group, enqueue iterations as work items to the worker pool.
4. Workers execute iterations, acquiring keys as needed per §17.2.
5. If `[reduce: op]` is present, combine partial results using `op`.
6. Block until all iterations complete.
7. Return the reduced value (if reduction) or unit.

**Reduction Semantics**

Reduction operators MUST be associative:

∀ a, b, c. (a ⊕ b) ⊕ c = a ⊕ (b ⊕ c)

For non-associative operations, the `[ordered]` attribute is required, which forces sequential execution.

Parallel reduction:

1. Partition iterations across workers.
2. Each worker reduces its partition to a partial result.
3. Combine partial results in a deterministic tree pattern.
4. Return the final result.


#### 18.5.3 Key-Based Parallelism Determination

See §17 for key system definition. This section defines dispatch-specific key usage.

**Static Semantics**

**Key Inference**

When no key clause is provided, the body is analyzed to infer key paths and modes:

| Body Access Pattern       | Inferred Key Path        | Inferred Mode   |
| :------------------------ | :----------------------- | :-------------- |
| `data[i] = ...`           | `data[i]`                | `write`         |
| `... = data[i]`           | `data[i]`                | `read`          |
| `data[i] = data[i] + ...` | `data[i]`                | `write`         |
| `result[i] = source[j]`   | `result[i]`, `source[j]` | `write`, `read` |

**Disjointness Guarantee**

`dispatch` v `in` r { … a[v] … }
────────────────────────────────────────────────────────────
∀ v_1, v_2 ∈ r, v_1 ≠ v_2 ⇒ ProvablyDisjoint(a[v_1], a[v_2])    (K-Disjoint-Dispatch)

**Parallelism Determination**

| Key Pattern   | Keys Generated      | Parallelism Degree    |
| :------------ | :------------------ | :-------------------- |
| `data[i]`     | n distinct keys     | Full parallel         |
| `data[i / 2]` | n/2 distinct keys   | Pairs serialize       |
| `data[i % k]` | k distinct keys     | k-way parallel        |
| `data[f(i)]`  | Unknown at compile  | Runtime serialization |


### 18.6 Cancellation


#### 18.6.1 Cancellation Propagation

**Cancellation.** The cooperative mechanism by which in-progress parallel work may be requested to stop early. Work items MUST explicitly check for cancellation; the runtime does not forcibly terminate work.

**Syntax**

**CancelToken Type**

```cursive
modal CancelToken {
    @Active { } {
        procedure cancel(~%)
        procedure is_cancelled(~) -> bool
        procedure wait_cancelled(~) -> Async<()>
        procedure child(~) -> CancelToken@Active
    }

    @Cancelled { } {
        procedure is_cancelled(~) -> bool { result true }
    }

    procedure new() -> CancelToken@Active
}
```

**Dynamic Semantics**

When a cancel token is attached to a parallel block via the `cancel` option, the token is implicitly available within all `spawn` and `dispatch` bodies.


#### 18.6.2 Cancellation Cleanup

**Dynamic Semantics**

Cancellation is a request, not a guarantee:

| Scenario                       | Behavior                          |
| :----------------------------- | :-------------------------------- |
| Work checks and returns early  | Iteration completes immediately   |
| Work ignores cancellation      | Iteration runs to completion      |
| Work is queued but not started | MAY be dequeued without executing |
| Work is mid-execution          | Continues until next check point  |


### 18.7 Panic Handling in Parallel


#### 18.7.1 Single Panic Semantics

**Parallel Panic Handling.** When a work item panics within a parallel block, the panic is captured and propagated after all work settles.

**Dynamic Semantics**

1. The panicking work item captures panic information.
2. Other work items continue to completion (or cancellation if a token is attached).
3. After all work settles, the panic is re-raised at the block boundary.


#### 18.7.2 Multiple Panic Handling

**Dynamic Semantics**

1. Each panic is captured independently.
2. All work completes or is cancelled.
3. The first panic (by completion order) is raised.
4. Other panics are discarded; implementations MAY log them.


#### 18.7.3 Panic and Cancellation

**Dynamic Semantics**

Implementations MAY request cancellation on first panic to expedite block completion. This behavior is Implementation-Defined.


### 18.8 Determinism and Nesting


#### 18.8.1 Determinism Guarantees

**Determinism.** Given identical inputs and parallel structure, execution produces identical results.

**Static Semantics**

Dispatch is deterministic when:

1. Key patterns produce identical partitioning across runs.
2. Iterations with the same key execute in index order.
3. Reduction uses deterministic tree combination.

The `[ordered]` attribute forces sequential side-effect ordering. Iterations MAY execute in parallel, but side effects (I/O, shared mutation) are buffered and applied in index order.


#### 18.8.2 Nested Parallelism Rules

**Nested Parallelism.** Parallel blocks may be nested. Inner blocks execute within the context established by outer blocks.

**Dynamic Semantics**

**CPU Nesting**

Inner CPU parallel blocks share the worker pool with outer blocks. The `workers` parameter on inner blocks is a hint or limit, not additional workers.

**Heterogeneous Nesting**

CPU and GPU blocks may be nested.

**Constraints**

1. GPU parallel blocks MUST NOT be nested inside other GPU parallel blocks.
2. Inner CPU blocks share the outer block's worker pool.
3. Capture rules apply independently at each nesting level.


#### 18.8.3 Memory Allocation in Parallel

**Parallel Allocation.** Work items may allocate memory using captured allocator capabilities.

**Dynamic Semantics**

**Captured Allocator**

Work items may capture `ctx.heap` and invoke allocation methods.

**Region Allocation**

Work items executing within a `region` block may allocate from that region using the `^` operator.

---

