# The Cursive Language Specification

**Part**: X - Modal Types
**File**: 10_Modals.md
**Previous**: [Functions and Procedures](09_Functions.md) | **Next**: [Metaprogramming](11_Metaprogramming.md)

---

## Abstract

This chapter is the primary, canonical specification of modal types in Cursive. Modal types are compile-time verified state machines that encode typestate patterns as first-class nominal types, enabling stateful protocol enforcement at zero runtime cost. A modal type declares a finite set of named states with associated data fields and explicit transitions between states. Operations consume a value in one state and produce a value in a new state, with the compiler statically verifying that only declared transitions occur. State annotations are erased after type checking, leaving only the data fields at runtime (zero-cost abstraction).

**Definition 10.1 (Modal Types):** A **modal type** is a nominal type parameterized by a compile-time **state** annotation. A modal type declaration defines:
- A finite set of named states `@State`, each with associated data fields
- Explicit state transitions via procedures that consume `self: M@S₁` and return `M@S₂`
- Optional state coercions `@S₁ <: @S₂` for implicit state transitions
- Optional state invariants and transition contracts

Modal types enforce **linear consumption**: transitioning consumes the source state value, preventing use-after-transition. All state changes are explicit via procedure calls. Modal types are NOT copyable (CITE: Part IV §4.5). This chapter is the authoritative specification for modal type declarations, state annotations, transition verification, pattern matching, and integration with permissions, effects, and contracts.

**Key components**:
- Modal type declarations with states and procedures (§10.1)
- State annotation syntax `M@State` and type formation rules (§10.2)
- Transition procedures inside modal bodies with `procedure @S₁ -> @S₂` syntax (§10.3)
- State coercion declarations `coerce @S₁ <: @S₂` (§10.4)
- Pattern matching on modal states with type refinement (§10.5)
- Integration with permissions, effects, contracts (§10.6-10.8)
- State inference and holes `M@_?` (§10.9)
- Verification algorithms and soundness theorems (§10.10-10.11)
- Complete diagnostic catalog (§10.12)

**Key design decisions**:
- States are compile-time only; runtime representation contains only data fields
- Procedures are declared INSIDE modal bodies (consistent with records)
- Explicit transition specification `procedure @StateFrom -> @StateTo` for clarity
- No subtyping between states except via explicit `coerce` declarations
- Linear consumption enforced via move semantics
- Scope operator `::` used for procedure calls (consistent with type-scope dispatch)

**Conformance:** See Part I §1.5-1.7 for conformance criteria. Implementations MUST support all normative features specified in this chapter. State transition verification is REQUIRED. Implementations MUST prevent invalid state transitions at compile time. Runtime state tags are FORBIDDEN (zero-cost abstraction requirement).

---

## 10.0 Foundations

### 10.0.1 Scope

This chapter provides complete normative rules for:

- **Modal type declarations**: Syntax for `modal` keyword, state declarations, state coercions, procedure declarations inside modal bodies
- **State annotations**: Formation rules for `M@State` types, well-formedness constraints
- **State coercions**: Explicit `coerce @S₁ <: @S₂` declarations with cost and permission constraints
- **Transition procedures**: Type-scope procedures with explicit state transitions (`procedure @S₁ -> @S₂`)
- **Non-transition procedures**: State-preserving operations on modal values
- **Pattern matching**: State-based pattern matching with type refinement
- **Linear consumption**: Move semantics preventing use-after-transition
- **State inference**: Holes `M@_?` and constraint solving for state annotations
- **Integration**: Interaction with permissions (Part IV), effects (Part VII), contracts (Part VII)
- **Verification**: Algorithms for transition verification, reachability analysis, deadlock detection
- **Soundness**: Formal theorems establishing state safety and zero-cost abstraction

Where other parts remain authoritative (e.g., function types in Part II §2.10, permissions in Part IV), this chapter integrates those rules at modal type boundaries.

### 10.0.2 Cross-Part Dependencies

This chapter depends on: Part I (notation, judgments, conformance); Part II (modal type formation §2.3.3, function types §2.10, tuples §2.8, generics §2.9, subtyping §2.11); Part III (declarations §3.1, const evaluation §3.3, scope §3.7, name resolution §3.9); Part IV (permission lattice §4.2, move semantics §4.5, coercion §4.6, call boundaries §4.9); Part V (call expressions §5.4, pattern matching §5.8, evaluation order §5.2, closures §5.10); Part VI (blocks §6.2, return §6.3, state-aware matching §6.10, control flow §6.7); Part VII (effect clauses §7.3, preconditions §7.4, postconditions §7.5, state invariants §7.6, contract checking §7.2); Part VIII (hole syntax §8.1-8.3); Part IX (functions §9.2-9.3, parameters §9.4, dispatch §9.6); Part XIII (regions §13.3-13.4, thread safety §13.7); Appendix A (grammar).

### 10.0.3 Design Principles

This chapter enforces Cursive's core principles as applied to stateful protocols:

1. **Explicit over implicit** — All state changes are explicit procedure calls; no implicit state coercions occur except those declared via `coerce` syntax; state annotations must be written explicitly (no default state for user-defined modals); transition specifications use explicit `procedure @S₁ -> @S₂` syntax.

2. **Local reasoning** — The type `M@State` encodes all information about valid operations at that state; no global analysis required to determine valid transitions; pattern matching refines types locally within each branch; no hidden aliasing or state changes.

3. **Zero-cost abstractions** — State annotations erased after type checking; runtime representation contains only data fields (no discriminant tag); transition procedures compile to normal function calls; all verification performed at compile time with zero runtime overhead.

4. **Determinism** — State transitions are deterministic based on procedure called; evaluation order follows Part V §5.2 rules; no hidden state changes or side-effects in transitions; failure modes explicit via `Result` types.

5. **Simple ownership** — Modal values cannot be copied (NOT `Copy`); transitions consume ownership (`own` required); linear consumption prevents use-after-transition; no borrow checker complexity (deterministic ownership only).

6. **Fail-fast compilation** — Invalid state transitions rejected at compile time; unreachable states detected and warned; missing procedures for declared transitions are hard errors; use-after-transition prevented statically.

### 10.0.4 Mathematical Foundations

Modal type semantics are formalized using standard type-theoretic judgments:

**Typing judgment for modal types:**
```
Γ ⊢ M@S : Type
```
Read as: "In context Γ, M@S is a well-formed type where M is a modal type and S is a valid state of M."

**Transition typing judgment:**
```
Γ ⊢ e : M@S₁
(@S₁ --p--> @S₂) ∈ transitions(M)
Γ ⊢ e::p(args) : M@S₂ ! ε
```
Read as: "Expression e has type M@S₁, there exists a transition from S₁ to S₂ via procedure p in M's transition set, therefore calling p on e yields M@S₂ with effects ε."

**State reachability:**
```
S₁ ⇝* S₂
```
Read as: "State S₂ is reachable from state S₁ via zero or more transitions."

**Linear consumption:**
```
Γ ⊢ e : M@S₁
(@S₁ --p--> @S₂) ∈ transitions(M)
------------------------------
Γ ⊢ e::p(...) : M@S₂ ! ε
Γ \ {e} ⊢ ... (e invalidated)
```
Read as: "After transition, the original binding e is removed from context Γ (consumed)."

**State coercion:**
```
(@S₁ <: @S₂) ∈ coercions(M)
Γ ⊢ e : M@S₁
------------------------------
Γ ⊢ e : M@S₂ (implicit coercion)
```

### 10.0.5 Notational Conventions

This part reuses judgment and metavariable conventions from Part I. Additional metavariables used throughout §10:

```
M, N            ∈ Name          (modal type names)
S, S₁, S₂       ∈ StateName     (state names, prefixed with @)
p, q            ∈ Name          (procedure names for transitions)
T, U, τ, σ      ∈ Type          (types)
ε, ε'           ∈ Effect        (effect sets)
perm            ∈ {own, mut, imm}  (permissions)
e, v            ∈ Expr          (expressions, values)
Γ               : Context       (typing context)
Θ               : TransitionSet (transition declarations)
Σ               : StateContext  (state knowledge context)
```

**State notation:**
- States are written `@StateName` with `@` sigil
- State variables: `@S`, `@S₁`, `@S₂`
- State sets: `S = {@S₁, @S₂, ..., @Sₙ}`

**Transition notation:**
- Transition: `@S₁ --p--> @S₂` (state S₁ to state S₂ via procedure p)
- Transition set: `Θ = {(@S₁ --p₁--> @S₂), (@S₂ --p₂--> @S₃), ...}`
- Reachability: `S₁ ⇝* S₂` (S₂ reachable from S₁)

**Inference rule format:**
```
[Rule-Name]
premise₁    premise₂    ...
----------------------------
conclusion
```

### 10.0.6 Organization

The remainder of Part X is organized as follows:

- **§10.1** Modal Declarations — Syntax and grammar for `modal` keyword, state declarations, procedure declarations inside bodies
- **§10.2** State Annotations and Type Formation — `M@State` syntax, type formation rules, well-formedness
- **§10.3** State Transitions and Procedures — Transition procedures with `procedure @S₁ -> @S₂` syntax, semantics
- **§10.4** State Coercions — Explicit `coerce @S₁ <: @S₂` declarations, structural subset requirements
- **§10.5** Pattern Matching on States — State-based patterns, type refinement, exhaustiveness checking
- **§10.6** Integration with Permissions — Ownership requirements, linear consumption, no-copy enforcement
- **§10.7** Integration with Effects — Effect clauses on transitions, effect propagation
- **§10.8** Integration with Contracts — State invariants, transition preconditions/postconditions
- **§10.9** State Inference and Holes — `M@_?` syntax, constraint generation, inference algorithm
- **§10.10** Verification Algorithms — Transition verification, reachability analysis, deadlock detection
- **§10.11** Soundness Theorems — State safety, transition safety, zero-cost abstraction proofs
- **§10.12** Diagnostics — Complete error catalog with E4xxx/E6xxx codes
- **§10.13** Grammar Summary — Deltas to Appendix A
- **§10.14** Examples — Comprehensive real-world modal type examples

---

## 10.1 Modal Declarations

### 10.1.1 Syntax

Modal type declarations use the `modal` keyword followed by a type name and a body containing state declarations, optional state coercions, and procedure declarations. The canonical grammar resides in Appendix A; this subsection presents relevant productions.

**Canonical grammar (from Appendix A):**

```ebnf
ModalDecl       ::= Attribute* Visibility? "modal" Ident GenericParams? ModalBody

ModalBody       ::= "{" ModalState+ StateCoercion* ProcedureDecl* "}"

ModalState      ::= "@" Ident RecordBody WhereClause?

StateCoercion   ::= "coerce" "@" Ident "<:" "@" Ident CoercionConstraint?

CoercionConstraint ::= "{"
                       ("cost" ":" Cost)?
                       ("requires" ":" Permission)?
                       ("uses" ":" EffectSet)?
                       "}"

Cost            ::= "O(1)"

ProcedureDecl   ::= Attribute* Visibility? "procedure" TransitionSpec?
                    Ident GenericParams?
                    "(" ParamList? ")" (":" Type)?
                    WhereClause?
                    ContractClause*
                    BlockStmt

TransitionSpec  ::= "@" Ident "->" "@" Ident

RecordBody      ::= "{" FieldDecl* "}"

FieldDecl       ::= Ident ":" Type
```

**Syntactic constraints:**

1. A modal type MUST declare at least one state (`ModalState+` requires one or more states).

2. State names MUST be unique within the modal declaration.

3. State names MUST begin with an uppercase letter by convention (enforced by `Ident` production in Appendix A).

4. State field declarations follow record syntax; fields MAY vary arbitrarily between states.

5. Procedure declarations inside modal bodies MAY include optional `TransitionSpec` to denote state-changing procedures.

6. Procedures without `TransitionSpec` are non-transition procedures (state-preserving operations).

7. Generic parameters (type, const, effect) MAY be declared on modal types and apply to all states.

8. State coercions MAY be declared to enable implicit state transitions under specified constraints.

**Examples:**

```cursive
// Simple modal type with two states
modal File {
    @Closed { path: string }
    @Open { path: string, handle: FileHandle }

    procedure @Closed -> @Open
        open(self: own File@Closed): Result<File@Open, Error>
        uses fs.open
        must file_exists(self.path)
    {
        match open_file_handle(self.path) {
            Result@Ok { value: handle } => {
                result Result@Ok { value: File@Open { path: self.path, handle } }
            },
            Result@Err { error } => {
                result Result@Err { error }
            }
        }
    }

    procedure @Open -> @Closed
        close(self: own File@Open): File@Closed
        uses fs.close
    {
        close_file_handle(self.handle)
        result File@Closed { path: self.path }
    }

    // Non-transition procedure (no state change)
    procedure read(self: File@Open, buf: mut [u8]): Result<usize, Error>
        uses fs.read
        must buf.len() > 0
    {
        self.handle::read(buf)
    }
}

// EXAMPLE: Generic modal type demonstrating state machine pattern
// NOTE: This is a pedagogical example showing how modal types can represent
// optional values with state transitions. The standard library Option<T> is
// defined as an ENUM (not a modal type) in Part II §2.2.3.
// This example illustrates that modal types CAN express optional-like semantics,
// but the actual stdlib implementation uses a simpler enum approach.
modal OptionExample<T> {
    @None
    @Some { value: T }

    procedure @None -> @Some
        set(self: own OptionExample<T>@None, value: T): OptionExample<T>@Some
    {
        result OptionExample@Some { value }
    }

    procedure @Some -> @None
        clear(self: own OptionExample<T>@Some): OptionExample<T>@None
    {
        result OptionExample@None {}
    }

    procedure map<U>(self: OptionExample<T>@Some, f: (T) -> U ! ∅): OptionExample<U>@Some {
        result OptionExample@Some { value: f(self.value) }
    }
}

// Modal with state coercion
modal Buffer<T> {
    @Owning { data: *T, len: usize, cap: usize }
    @ReadOnly { data: *T, len: usize }

    // Zero-cost implicit coercion
    coerce @Owning <: @ReadOnly {
        cost: O(1)
        uses: ∅
    }

    procedure get(self: Buffer<T>@ReadOnly, index: usize): T
        uses mem.read
        must index < self.len
    {
        result mem::load(self.data, index)
    }
}
```

### 10.1.2 State Declarations

#### Syntax and Semantics

Each state declaration introduces a variant of the modal type:

```ebnf
ModalState ::= "@" Ident RecordBody WhereClause?
```

The state name follows the `@` sigil and MUST be a valid identifier. The record body declares fields for that state using standard field declaration syntax. An optional `where` clause specifies state invariants.

**Well-formedness rules:**

```
[State-WF]
modal M declares state @S { f₁: T₁, ..., fₙ: Tₙ } where P
∀i. Γ ⊢ Tᵢ : Type           (field types well-formed)
Γ, self: M@S ⊢ P : bool     (invariant well-formed boolean predicate)
--------------------------------------------------------
Γ ⊢ @S valid state of M
```

```
[State-Unique]
modal M declares states @S₁, ..., @Sₙ
∀i≠j. Sᵢ ≠ Sⱼ
--------------------------------------------------------
state names unique
```

```
[State-Unique-Violation]
modal M declares states containing duplicate @S
--------------------------------------------------------
ERROR (E4001): duplicate state name in modal declaration
```

**Field declarations:**

Fields within a state follow standard type-checking rules (CITE: Part II §2.5). Each field MUST have a well-formed type. Fields MAY reference the enclosing modal type for recursion (e.g., `@Node { value: T, next: Option<LinkedList<T>@Node> }`), subject to standard recursion constraints.

**State invariants:**

The optional `where` clause specifies a boolean predicate that MUST hold for all instances in that state:

```cursive
modal Counter {
    @Zero { count: i32 }
        where self.count == 0

    @Positive { count: i32 }
        where self.count > 0

    @Negative { count: i32 }
        where self.count < 0

    procedure increment(self: own Counter@Zero): Counter@Positive {
        result Counter@Positive { count: 1 }
    }
}
```

State invariants are enforced according to contract checking rules (CITE: Part VII §7.6). Implementations MUST verify invariants:
- At state construction (literal expressions)
- At transition target states (procedure postconditions)
- Optionally at runtime in debug/verification mode

### 10.1.3 Procedure Declarations Inside Modal Bodies

**Critical design principle:** Procedures are declared INSIDE modal bodies, consistent with record declarations (CITE: Part IX §9.2.1). This provides:
- **Consistency**: Matches record pattern where procedures are scoped to type body
- **Locality**: All type behavior in one place
- **Scoping**: Procedures naturally scoped to modal type via body
- **No redundancy**: Single declaration includes specification and implementation

#### Transition Procedures

Transition procedures explicitly specify state changes using `TransitionSpec`:

```ebnf
ProcedureDecl ::= "procedure" TransitionSpec? Ident "(" ParamList? ")" ":" Type? ...
TransitionSpec ::= "@" Ident "->" "@" Ident
```

**Syntax:** `procedure @StateFrom -> @StateTo name(params): ReturnType { body }`

**Semantics:**

```
[Transition-Proc-WF]
modal M declares state @S₁ and @S₂
procedure @S₁ -> @S₂ p(self: own M@S₁, params): M@S₂ { body }
Γ, self: M@S₁, params ⊢ body : M@S₂
--------------------------------------------------------
transition procedure well-formed
```

**Requirements:**

1. Both `@StateFrom` and `@StateTo` MUST be declared states of the enclosing modal.

2. The `self` parameter MUST have type `M@StateFrom` with `own` permission (defaults to `own` if not specified).

3. The return type MUST be `M@StateTo` or `Result<M@StateTo, E>` for fallible transitions.

4. The procedure body MUST produce a value of type `M@StateTo` on all control paths.

**Examples:**

```cursive
modal Connection {
    @Disconnected { addr: string }
    @Connecting { addr: string, timeout: Duration }
    @Connected { addr: string, socket: Socket }
    @Failed { addr: string, error: Error }

    // Transition with effects and contracts
    procedure @Disconnected -> @Connecting
        connect(self: own Connection@Disconnected, timeout: Duration): Connection@Connecting
        uses net.connect
        must valid_address(self.addr)
        will result.timeout == timeout
    {
        start_async_connect(self.addr, timeout)
        result Connection@Connecting { addr: self.addr, timeout }
    }

    // Fallible transition returning Result
    procedure @Connecting -> @Connected
        complete(self: own Connection@Connecting): Result<Connection@Connected, Connection@Failed>
        uses net.poll
    {
        match poll_connection_status() {
            Result@Ok { value: socket } => {
                result Result@Ok { value: Connection@Connected {
                    addr: self.addr,
                    socket
                } }
            },
            Result@Err { error: cause } => {
                result Result@Err { error: Connection@Failed {
                    addr: self.addr,
                    error: cause
                } }
            }
        }
    }

    procedure @Connected -> @Disconnected
        disconnect(self: own Connection@Connected): Connection@Disconnected
        uses net.close
    {
        self.socket::close()
        result Connection@Disconnected { addr: self.addr }
    }
}
```

#### Non-Transition Procedures

Procedures without `TransitionSpec` are state-preserving operations:

```cursive
modal File {
    @Closed { path: string }
    @Open { path: string, handle: FileHandle }

    // Non-transition procedure on @Open (no state change)
    procedure write(self: File@Open, data: [u8]): Result<usize, Error>
        uses fs.write
        must data.len() > 0
    {
        self.handle::write(data)
    }

    // Non-transition accessor (works on any state)
    procedure get_path(self: File@Closed): string {
        result self.path
    }

    procedure get_path(self: File@Open): string {
        result self.path
    }
}
```

Non-transition procedures:
- MAY use `imm`, `mut`, or `own` permissions on `self`
- MUST NOT change the state annotation of `self`
- MAY be overloaded for different states (separate declarations)
- Are called using `::` syntax: `file::write(data)`

#### Associated Functions

Associated functions (no `self` parameter) MAY be declared inside modal bodies:

```cursive
modal File {
    @Closed { path: string }
    @Open { path: string, handle: FileHandle }

    // Associated function (constructor)
    function new(path: string): File@Closed {
        result File@Closed { path }
    }

    // Associated function with validation
    function open_or_create(path: string): Result<File@Open, Error>
        uses fs.open, fs.create
    {
        if file_exists(path) {
            let closed = File::new(path)
            result closed::open()
        } else {
            match create_file(path) {
                Result@Ok { value: _ } => {
                    let closed = File::new(path)
                    result closed::open()
                },
                Result@Err { error } => {
                    result Result@Err { error }
                }
            }
        }
    }
}
```

### 10.1.4 Well-Formedness of Modal Declarations

A modal declaration is **well-formed** if all of the following conditions hold. This section specifies comprehensive well-formedness requirements for modal types, states, procedures, and coercions.

#### Top-Level Modal Well-Formedness

**[Modal-WF]**

```
modal M<T₁, ..., Tₘ> { states; coercions; procedures }
|states| ≥ 1                                   (at least one state required)
∀ generic parameter Tᵢ. Tᵢ is well-formed      (generic parameters valid)
∀@S ∈ states. state @S is well-formed          (all states well-formed)
∀c ∈ coercions. coercion c is well-formed      (all coercions well-formed)
∀p ∈ procedures. procedure p is well-formed    (all procedures well-formed)
all state names are unique in M                (no duplicate states)
recursion(M) is well-founded                   (recursive references valid)
--------------------------------------------------------
Γ ⊢ modal M : ModalType
```

**Requirements:**

1. **At least one state**: A modal type MUST declare at least one state. A modal with zero states is ill-formed.

2. **Generic parameter well-formedness**: All generic type parameters MUST be valid type parameters with proper bounds.

3. **State well-formedness**: Each state declaration MUST be well-formed (see below).

4. **Coercion well-formedness**: Each coercion declaration MUST be well-formed (see §10.4).

5. **Procedure well-formedness**: Each procedure declaration MUST be well-formed (see below).

6. **Uniqueness**: State names MUST be unique within the modal type.

7. **Well-founded recursion**: Recursive references to the modal type MUST be well-founded (no infinite types).

#### State Declaration Well-Formedness

**[State-WF]**

```
modal M declares state @S { f₁: T₁, ..., fₙ: Tₙ } where P
∀i ∈ {1..n}. Γ ⊢ Tᵢ : Type                    (all field types well-formed)
∀i, j. i ≠ j ⇒ fᵢ ≠ fⱼ                         (field names unique)
Γ, self: M@S ⊢ P : bool                        (invariant well-typed)
--------------------------------------------------------
state @S is well-formed in M
```

**Requirements:**

1. **Field type validity**: Every field type `Tᵢ` MUST be a well-formed type.

2. **Field name uniqueness**: Field names MUST be unique within the state.

3. **Invariant well-typed**: If an invariant `P` is declared, it MUST be a well-typed boolean expression with `self: M@S` in scope.

4. **Field count**: States MAY be empty (zero fields) or have any positive number of fields.

#### Procedure Declaration Well-Formedness

Procedures fall into two categories: **transition procedures** (`@S₁ -> @S₂`) and **non-transition procedures** (no state change).

**[Transition-Procedure-WF]**

```
modal M declares states @S₁, @S₂
procedure @S₁ -> @S₂ p(self: own M@S₁, x₁: T₁, ..., xₙ: Tₙ): R { body }
R = M@S₂ OR R = Result<M@S₂, E> for some type E
∀i. Γ ⊢ Tᵢ : Type                              (parameter types well-formed)
Γ, self: M@S₁, x₁: T₁, ..., xₙ: Tₙ ⊢ body : R (body well-typed)
--------------------------------------------------------
transition procedure p is well-formed in M
```

**Requirements:**

1. **State validity**: Both `@S₁` and `@S₂` MUST be declared states of `M`.

2. **Self parameter**: The `self` parameter MUST have type `M@S₁` with `own` permission.

3. **Return type**: The return type MUST be either:
   - `M@S₂` (infallible transition)
   - `Result<M@S₂, E>` for some error type `E` (fallible transition)

4. **Parameter types**: All parameter types MUST be well-formed.

5. **Body typing**: The procedure body MUST type-check and produce the declared return type on all control paths.

**[Non-Transition-Procedure-WF]**

```
modal M declares state @S
procedure @S p(self: Permission M@S, x₁: T₁, ..., xₙ: Tₙ): R { body }
Permission ∈ {imm, mut, own}
∀i. Γ ⊢ Tᵢ : Type
Γ, self: Permission M@S, x₁: T₁, ..., xₙ: Tₙ ⊢ body : R
--------------------------------------------------------
non-transition procedure p is well-formed in M
```

**Requirements:**

1. **State validity**: State `@S` MUST be declared in `M`.

2. **Self parameter permission**: The `self` parameter MAY have permission `imm`, `mut`, or `own`.

3. **Return type**: Return type `R` can be any well-formed type (not restricted to modal states).

4. **Body typing**: The procedure body MUST type-check correctly.

#### Coercion Declaration Well-Formedness

**[Coercion-WF]**

```
modal M declares states @S₁, @S₂
coerce @S₁ <: @S₂ { cost: C, requires: P, uses: ε }
fields(@S₂) ⊆ fields(@S₁)                      (structural subset)
∀f ∈ fields(@S₂). type_of(@S₁, f) = type_of(@S₂, f)  (exact type match)
C = O(1)                                       (constant-time projection)
--------------------------------------------------------
coercion (@S₁ <: @S₂) is well-formed in M
```

**Requirements:**

1. **State validity**: Both `@S₁` and `@S₂` MUST be declared states of `M`.

2. **Structural subset**: Fields of `@S₂` MUST be a subset of fields of `@S₁` with matching types.

3. **Cost specification**: The cost annotation MUST be `O(1)` (constant-time projection).

See §10.4 for complete coercion well-formedness rules.

#### Generic Parameter Well-Formedness

**[Generic-Parameter-WF]**

```
modal M<T₁: B₁, ..., Tₘ: Bₘ> { ... }
∀i ∈ {1..m}. Tᵢ is a valid type parameter
∀i ∈ {1..m}. Γ ⊢ Bᵢ : TypeBound                (bounds well-formed)
∀i, j. i ≠ j ⇒ Tᵢ ≠ Tⱼ                         (parameter names unique)
--------------------------------------------------------
generic parameters are well-formed for M
```

**Requirements:**

1. **Parameter validity**: Each type parameter MUST be a valid identifier.

2. **Bound well-formedness**: Each bound `Bᵢ` MUST be a well-formed type bound.

3. **Uniqueness**: Type parameter names MUST be unique.

**Example:**

```cursive
modal Result<T, E: Error> {  // T unbounded, E bounded by Error
    @Ok { value: T }
    @Err { error: E }
}
```

#### Recursive Type Well-Formedness

Modal types MAY reference themselves recursively, but such recursion MUST be **well-founded** (not produce infinite types).

**[Recursive-Modal-WF]**

```
modal M { states }
@S ∈ states(M)
f: T ∈ fields(@S)
M appears in T
T is structurally smaller than M (e.g., behind pointer, in Option, in Vec)
--------------------------------------------------------
recursive reference to M in @S is well-formed
```

**Allowed recursive patterns:**

1. **Behind indirection**: `*M`, `Box<M>`, `Rc<M>`, `Arc<M>`
2. **Inside Option**: `Option<M>`
3. **Inside collections**: `Vec<M>`, `HashMap<K, M>`
4. **Inside other generics**: `Result<M, E>`

**Forbidden recursive pattern:**

```cursive
// ERROR: Infinite type
modal Bad {
    @State { self_field: Bad@State }  // Direct recursion!
}
```

**Allowed recursive pattern:**

```cursive
// OK: Indirection via Option
modal LinkedList<T> {
    @Node { value: T, next: Option<Box<LinkedList<T>@Node>> }
    @Empty
}
```

#### Procedure Name Resolution Well-Formedness

Procedure names MUST be unique within their state scope, but MAY be overloaded across different states.

**[Procedure-Name-WF]**

```
modal M declares procedures p₁, p₂, ..., pₖ
∀ procedures pᵢ, pⱼ with same name n and same source state @S:
    signature(pᵢ) ≠ signature(pⱼ)  OR  i = j
--------------------------------------------------------
procedure names are well-formed in M
```

**Allowed:**

```cursive
modal File {
    @Closed
    @Open

    // Same name, different states: OK
    procedure @Closed close(self: own File@Closed): File@Closed { }
    procedure @Open close(self: own File@Open): File@Closed { }
}
```

**Forbidden:**

```cursive
modal Bad {
    @State

    // Same name, same state, same signature: ERROR
    procedure @State process(self: imm Bad@State): i32 { }
    procedure @State process(self: imm Bad@State): i32 { }  // Duplicate!
}
```

#### Complete Well-Formedness Summary

A modal type `M` is well-formed if and only if:

1. ✓ At least one state is declared
2. ✓ All state names are unique
3. ✓ All states are individually well-formed
4. ✓ All field types in all states are well-formed
5. ✓ All procedures are individually well-formed
6. ✓ All procedure names are unique within their state scope
7. ✓ All coercions are structurally valid
8. ✓ All generic parameters are well-formed
9. ✓ All recursive references are well-founded
10. ✓ All invariants are well-typed

**Ill-formed modal examples:**

```cursive
// ERROR E4001: Duplicate state name
modal Bad {
    @Active { x: i32 }
    @Active { y: string }  // Duplicate!
}

// ERROR E4001: Undefined state in transition
modal Bad2 {
    @A {}

    procedure @A -> @B  // @B not declared!
        go(self: own Bad2@A): Bad2@B {
            result Bad2@B {}
        }
}

// ERROR E4001: Transition signature mismatch
modal Bad3 {
    @X { data: i32 }
    @Y { data: string }

    procedure @X -> @Y
        wrong(self: own Bad3@X): Bad3@X {  // Returns @X not @Y!
            result Bad3@X { data: self.data }
        }
}

// ERROR E4001: No states declared
modal Empty {
    // No states! At least one required.
}

// ERROR E4001: Infinite type (direct recursion)
modal Infinite {
    @State { myself: Infinite@State }  // Not behind indirection!
}
```

**Well-formed modal examples:**

```cursive
// OK: Single state, simple modal
modal Flag {
    @Set
    @Unset
}

// OK: Generic with bounds
modal Result<T, E: Error> {
    @Ok { value: T }
    @Err { error: E }
}

// OK: Recursive via indirection
modal Tree<T> {
    @Leaf { value: T }
    @Branch { value: T, left: Option<Box<Tree<T>@Branch>>, right: Option<Box<Tree<T>@Branch>> }
}

// OK: Procedures with same name, different states
modal Connection {
    @Disconnected
    @Connected

    procedure @Disconnected connect(self: own Connection@Disconnected): Connection@Connected { }
    procedure @Connected disconnect(self: own Connection@Connected): Connection@Disconnected { }
}
```

This completes the well-formedness specification for modal declarations. The next section covers state annotation types.

---

## 10.2 State Annotations and Type Formation

### 10.2.1 State Annotation Syntax

Given a declared modal type `M` with states `S = {@S₁, @S₂, ..., @Sₙ}`, the notation `M@S` denotes a concrete type representing modal M in state S.

**Canonical grammar:**

```ebnf
Type        ::= ... | ModalStateType
ModalStateType ::= Ident "@" Ident
```

Where the first `Ident` MUST resolve to a modal type name, and the second `Ident` MUST resolve to a valid state of that modal.

**Type formation rule:**

```
[T-Modal-State]
Γ ⊢ M : ModalType
@S ∈ states(M)
----------------------------------------
Γ ⊢ M@S : Type
```

**Invalid state error:**

```
[T-Modal-Invalid-State]
Γ ⊢ M : ModalType
@S ∉ states(M)
----------------------------------------
ERROR (E4001): invalid state annotation '@S' for modal type 'M'
```

**Examples:**

```cursive
// Valid state types
let f1: File@Closed = File@Closed { path: "/tmp/data.txt" }
let f2: File@Open = ...
let conn: Connection@Disconnected = ...

// ERROR: Invalid state
let bad: File@Invalid = ...  // E4001: '@Invalid' not a state of 'File'

// Generic modal with state (using example type)
let opt: OptionExample<i32>@Some = OptionExample@Some { value: 42 }
```

### 10.2.2 State Type Equivalence

Two modal state types are equivalent if and only if:

```
[Type-Eq-Modal]
----------------------------------------
M@S₁ ≡ M@S₂  ⟺  (M = M) ∧ (S₁ = S₂)
```

**Critical:** `M@S₁` and `M@S₂` are DISTINCT types if `S₁ ≠ S₂`. There is NO implicit subtyping between states.

```
[No-State-Subtyping]
S₁ ≠ S₂
(@S₁ <: @S₂) ∉ coercions(M)
----------------------------------------
M@S₁ ⊄ M@S₂  (no subtyping)
```

This means:
- Cannot assign `File@Open` to a variable of type `File@Closed`
- Cannot pass `Connection@Disconnected` to a function expecting `Connection@Connected`
- Must use explicit transitions or state coercions

**Exception:** Explicit state coercions declared via `coerce` (§10.4) enable limited subtyping.

### 10.2.3 Construction and Literals

Modal state values are constructed using state constructor syntax:

```ebnf
ModalLiteral ::= TypeName "@" StateName "{" FieldInit* "}"
```

**Type rule:**

```
[T-Modal-Literal]
modal M declares @S { f₁: T₁, ..., fₙ: Tₙ }
∀i. Γ ⊢ eᵢ : Tᵢ
----------------------------------------
Γ ⊢ M@S { f₁: e₁, ..., fₙ: eₙ } : M@S
```

All fields declared in state `@S` MUST be initialized. Missing or extra fields are errors.

**Examples:**

```cursive
// Literal construction
let file = File@Closed { path: "/data.txt" }

// ERROR: Missing field
let bad1 = File@Open { path: "/data.txt" }  // Missing 'handle'

// ERROR: Extra field
let bad2 = File@Closed { path: "/data.txt", handle: h }  // 'handle' not in @Closed

// ERROR: Wrong field type
let bad3 = File@Closed { path: 42 }  // Type mismatch: expected 'string'
```

### 10.2.4 Generic Modal Types

Modal types MAY have generic parameters:

```cursive
modal Result<T, E> {
    @Ok { value: T }
    @Err { error: E }

    procedure @Ok -> @Err
        to_error(self: own Result<T, E>@Ok, error: E): Result<T, E>@Err
    {
        result Result@Err { error }
    }

    procedure map<U>(self: Result<T, E>@Ok, f: (T) -> U ! ∅): Result<U, E>@Ok {
        result Result@Ok { value: f(self.value) }
    }
}

// Usage with type parameters
let ok: Result<i32, string>@Ok = Result@Ok { value: 42 }
let err: Result<i32, string>@Err = Result@Err { error: "failed" }
```

Generic parameters apply uniformly to all states. The syntax `M<T>@S` instantiates generic `M` with type `T` in state `S`.

### 10.2.5 Properties of Modal Types

**Theorem 10.1 (Modal Non-Copyability):** For all modal types `M` and states `S` of `M`, values of type `M@S` are NOT `Copy`.

**Proof:** Modal types are defined as linear resources requiring explicit consumption during transitions. Allowing implicit copies would enable:
1. Multiple values with same identity in different states (violates linearity)
2. Use-after-transition via copy (violates safety)
3. Undefined protocol states (violates state machine semantics)

Therefore, modal types MUST NOT implement `Copy` contract. Implementations MUST reject any attempt to derive or implement `Copy` for modal types.

**Corollary:** Passing a modal value to a function consumes it (move semantics). Assignment moves ownership.

**Examples:**

```cursive
let f1 = File@Closed { path: "/data.txt" }
let f2 = f1  // OK: Move
// let x = f1.path  // ERROR E4006: use of moved value 'f1'

function takes_file(file: File@Closed) { ... }

let f3 = File@Closed { path: "/other.txt" }
takes_file(f3)  // Move
// f3::open()  // ERROR E4006: 'f3' was moved
```

### 10.2.6 Memory Layout and Runtime Representation

Modal types implement **zero-cost abstraction**: state annotations are compile-time only and impose no runtime overhead. This subsection formally specifies the memory layout and runtime representation of modal values.

#### Memory Layout Rules

**Definition 10.2 (Memory Layout):** The memory layout of a modal value `M@S` is determined solely by the fields declared in state `@S`, with no additional runtime metadata.

**Formal specification:**

```
[Memory-Layout-Size]
modal M declares @S { f₁: T₁, f₂: T₂, ..., fₙ: Tₙ }
size_of(M@S) = ∑ᵢ₌₁ⁿ size_of(Tᵢ) + padding(@S)
where padding(@S) is determined by alignment requirements
--------------------------------------------------------
sizeof(M@S) computed without state metadata

[Memory-Layout-Alignment]
modal M declares @S { f₁: T₁, f₂: T₂, ..., fₙ: Tₙ }
align_of(M@S) = max{align_of(T₁), align_of(T₂), ..., align_of(Tₙ)}
--------------------------------------------------------
Alignment is maximum of field alignments
```

**Field offset calculation:**

```
[Field-Offset]
modal M declares @S { f₁: T₁, ..., fᵢ: Tᵢ, ..., fₙ: Tₙ }
offset(M@S, f₁) = 0
offset(M@S, fᵢ) = align_up(offset(M@S, fᵢ₋₁) + size_of(Tᵢ₋₁), align_of(Tᵢ))
where align_up(n, a) = ⌈n / a⌉ * a
--------------------------------------------------------
Field offsets calculated sequentially with alignment
```

**Layout equivalence:**

```
[Layout-Equivalence]
modal M declares @S₁ { f₁: T₁, ..., fₙ: Tₙ }
modal M declares @S₂ { f₁: T₁, ..., fₙ: Tₙ }
fields(@S₁) = fields(@S₂)
--------------------------------------------------------
layout(M@S₁) ≡ layout(M@S₂)
```

**Critical invariant:** States with identical field signatures have identical memory layouts, enabling zero-cost transitions between such states.

#### Runtime Representation

**Zero-cost requirement:**

```
[No-State-Tag]
modal M, state @S
v : M@S at runtime
--------------------------------------------------------
representation(v) contains ONLY data fields
representation(v) contains NO state discriminant/tag
representation(v) contains NO type metadata
```

Implementations MUST NOT add:
- State discriminant tags
- Type tags or identifiers
- Virtual tables or dispatch metadata
- Any other runtime metadata

**Representation diagram:**

For `modal File { @Closed { path: string }, @Open { path: string, handle: FileHandle } }`:

```
File@Closed in memory:
┌--------------┐
│ path (string)│  <- Only field data, no tag
└--------------┘

File@Open in memory:
┌--------------┬------------------┐
│ path (string)│ handle (Handle) │  <- Only field data, no tag
└--------------┴------------------┘
```

#### Size and Alignment Examples

```cursive
modal State {
    @A { x: i32, y: i64 }              // size: 16, align: 8
    @B { a: i8, b: i8, c: i8, d: i8 }  // size: 4, align: 1
    @C { p: *u8, q: *u8 }              // size: 16, align: 8 (64-bit)
}

// Layout calculation for State@A:
// offset(x) = 0, size(i32) = 4
// offset(y) = align_up(4, 8) = 8, size(i64) = 8
// total size = 16, alignment = 8
```

#### Casting and Transmutation

**Prohibition on state casting:**

```
[No-State-Cast]
v : M@S₁
transmute(v, M@S₂)  // Attempting to reinterpret state
--------------------------------------------------------
FORBIDDEN unless fields(@S₁) = fields(@S₂)
```

Transmuting between states with different fields is **undefined behavior** even in `unsafe` blocks. The only safe transmutation is between states with identical field layouts.

### 10.2.7 Field Access

Field access on modal values follows standard struct field access semantics, with state-specific typing.

#### Syntax

```ebnf
FieldAccess ::= Expr "." Ident
```

#### Static Semantics (Type Rule)

```
[T-Field-Access]
Γ ⊢ e : M@S
modal M declares @S { ..., f: T, ... }
f ∈ fields(@S)
----------------------------------------
Γ ⊢ e.f : T

[T-Field-Access-Error]
Γ ⊢ e : M@S
f ∉ fields(@S)
----------------------------------------
ERROR (E4001): field 'f' not found in state '@S'
```

**Examples:**

```cursive
modal File {
    @Closed { path: string }
    @Open { path: string, handle: FileHandle }
}

let closed: File@Closed = File@Closed { path: "/data.txt" }
let p1: string = closed.path  // OK: 'path' in @Closed

// ERROR: 'handle' not in @Closed
// let h1 = closed.handle

let opened: File@Open = /* ... */
let p2: string = opened.path     // OK: 'path' in @Open
let h2: FileHandle = opened.handle  // OK: 'handle' in @Open
```

#### Dynamic Semantics (Evaluation Rule)

```
[E-Field-Access]
v ∈ [[M@S]]
@S declares f: T at offset o
σ(v + o) = v_f ∈ [[T]]
----------------------------------------
⟨v.f, σ⟩ ⇓ ⟨v_f, σ⟩
```

Field access evaluates to the value stored at the computed offset. No runtime state checking occurs.

#### Permission and Field Access

Field access respects permissions:

```
[T-Field-Access-Permission]
Γ ⊢ e : perm M@S
modal M declares @S { ..., f: T, ... }
----------------------------------------
Γ ⊢ e.f : perm T  (permission propagates)
```

**Examples:**

```cursive
let owned: own File@Open = /* ... */
let path: own string = owned.path  // Ownership propagates

let imm_file: imm File@Open = /* ... */
let imm_path: imm string = imm_file.path  // Immutability propagates

let mut_file: mut File@Open = /* ... */
let mut_handle: mut FileHandle = mut_file.handle  // Mutability propagates
```

### 10.2.8 Union Types with States

Modal state types MAY appear in union types, enabling type-safe handling of multiple possible states.

#### Formation Rule

```
[T-Union-States]
Γ ⊢ M@S₁ : Type
Γ ⊢ M@S₂ : Type
...
Γ ⊢ M@Sₙ : Type
----------------------------------------
Γ ⊢ M@S₁ | M@S₂ | ... | M@Sₙ : Type
```

**Subtyping (introduction):**

```
[T-Union-Intro]
Γ ⊢ e : M@Sᵢ
Sᵢ ∈ {S₁, S₂, ..., Sₙ}
----------------------------------------
Γ ⊢ e : M@S₁ | M@S₂ | ... | M@Sₙ
```

#### Pattern Matching Elimination

Union types with states are eliminated via pattern matching:

```
[T-Match-Union-States]
Γ ⊢ e : M@S₁ | M@S₂ | ... | M@Sₙ
∀i. match arm i covers M@Sᵢ with pattern pᵢ
∀i. Γ, bindings(pᵢ) ⊢ bodyᵢ : T
covered_states = {S₁, S₂, ..., Sₙ}
----------------------------------------
Γ ⊢ match e { M@S₁ patt₁ => body₁, ... } : T
```

**Exhaustiveness requirement:**

```
[Match-Union-Exhaustive]
match e : M@S₁ | M@S₂ | ... | M@Sₙ { arms }
covered_states(arms) ≠ {S₁, S₂, ..., Sₙ}
----------------------------------------
ERROR (E4005): non-exhaustive pattern match
```

#### Runtime Representation

**Critical:** Union types with states REQUIRE a runtime discriminant (unlike plain modal types):

```
[Union-Runtime-Discriminant]
v : M@S₁ | M@S₂ | ... | M@Sₙ
----------------------------------------
representation(v) = (discriminant: usize, data: max_size(states))
where discriminant ∈ {0, 1, ..., n-1} indicates active state
```

This is the ONLY case where state information exists at runtime. The discriminant enables pattern matching.

**Memory layout:**

```
Union<M@S₁, M@S₂> in memory:
┌--------------┬-------------------------┐
│ discriminant │ data (max of S₁, S₂)   │
│   (usize)    │                          │
└--------------┴-------------------------┘
```

#### Examples

```cursive
modal Result<T, E> {
    @Ok { value: T }
    @Err { error: E }
}

// Union type for uncertain state
let result: Result<i32, string>@Ok | Result<i32, string>@Err = /* ... */

match result {
    Result@Ok { value } => println("Success: {}", value),
    Result@Err { error } => eprintln("Error: {}", error)
}

// Function accepting multiple states
function handle_result(r: Result<i32, string>@Ok | Result<i32, string>@Err) {
    match r {
        Result@Ok { value } => value,
        Result@Err { error } => {
            eprintln("Error: {}", error)
            result 0
        }
    }
}
```

This section completed the formal specification of modal state types, their memory layout, field access semantics, and union types. Next section covers state transitions.

---

## 10.3 State Transitions and Procedures

State transitions in modal types are realized through procedure calls. A transition procedure consumes a modal value in one state and produces a modal value in a different state, enforcing protocol correctness at compile time. This section specifies the semantics of transition procedures, their typing rules, linear consumption requirements, and verification algorithms.

### 10.3.1 Transition Procedure Semantics

#### Syntax and Declaration

Transition procedures are declared inside modal bodies using the `TransitionSpec` annotation:

```ebnf
ProcedureDecl ::= Attribute* Visibility? "procedure" TransitionSpec?
                  Ident GenericParams?
                  "(" ParamList? ")" (":" Type)?
                  WhereClause?
                  ContractClause*
                  BlockStmt

TransitionSpec ::= "@" Ident "->" "@" Ident
```

The `TransitionSpec` explicitly declares the source and target states. A procedure declared as `procedure @S₁ -> @S₂` indicates that it:
1. Accepts `self` parameter of type `M@S₁` with `own` permission
2. Returns a value of type `M@S₂` (or `Result<M@S₂, E>` for fallible transitions)
3. Consumes the source state value via move semantics
4. Produces a new target state value

**Well-formedness rule:**

```
[Transition-Proc-WF]
modal M declares states @S₁, @S₂
procedure @S₁ -> @S₂ p(self: own M@S₁, x₁: T₁, ..., xₙ: Tₙ): M@S₂
    uses ε
    must P
    will Q
{ body }
Γ, self: M@S₁, x₁: T₁, ..., xₙ: Tₙ ⊢ body : M@S₂
Γ, self: M@S₁, x₁: T₁, ..., xₙ: Tₙ ⊢ P : bool    (precondition well-formed)
Γ, result: M@S₂, x₁: T₁, ..., xₙ: Tₙ ⊢ Q : bool  (postcondition well-formed)
--------------------------------------------------------
transition procedure p is well-formed in M
```

**Requirements:**

1. **State validity**: Both `@S₁` and `@S₂` MUST be declared states of modal type `M`.

2. **Self parameter type**: The `self` parameter MUST have type `M@S₁` matching the source state in `TransitionSpec`.

3. **Self permission**: The `self` parameter MUST have `own` permission (defaults to `own` if not specified).

4. **Return type**: The return type MUST be `M@S₂` or `Result<M@S₂, E>` for some error type `E`, matching the target state in `TransitionSpec`.

5. **Body typing**: The procedure body MUST produce a value of the declared return type on all control paths.

**Type rule for transition procedures:**

```
[T-Transition-Proc]
modal M declares @S₁, @S₂
procedure @S₁ -> @S₂ p(self: own M@S₁, x₁: T₁, ..., xₙ: Tₙ): M@S₂
    uses ε
    must P
    will Q
{ body }
Γ, self: M@S₁, x₁: T₁, ..., xₙ: Tₙ ⊢ body : M@S₂
Γ ⊢ P : bool
Γ, result: M@S₂ ⊢ Q : bool
--------------------------------------------------------
Γ ⊢ p : (M@S₁, T₁, ..., Tₙ) -> M@S₂ ! ε
```

This type rule establishes that procedure `p` has a function type accepting parameters typed `M@S₁, T₁, ..., Tₙ` and returning `M@S₂` with effects `ε`.

#### Transition Call Typing

Calling a transition procedure requires:
1. A receiver expression of the source state type
2. Arguments matching the procedure's additional parameters
3. Available effects matching the procedure's `uses` clause

**Type rule for transition calls:**

```
[T-Transition-Call]
Γ ⊢ e : M@S₁
procedure @S₁ -> @S₂ p(self: own M@S₁, params) declared in M
Γ ⊢ args : T₁, ..., Tₙ
ε ⊆ available_effects(Γ)
--------------------------------------------------------
Γ ⊢ e::p(args) : M@S₂ ! ε
Γ \ {e} ⊢ ...  (e consumed and invalidated in subsequent context)
```

The critical consequence is that after the transition call, the original binding `e` is consumed and removed from the typing context `Γ`, preventing any subsequent use.

**Example:**

```cursive
modal File {
    @Closed { path: string }
    @Open { path: string, handle: FileHandle }

    procedure @Closed -> @Open
        open(self: own File@Closed): Result<File@Open, Error>
        uses fs.open
        must file_exists(self.path)
        will result.is_ok() implies result.unwrap().handle.is_valid()
    {
        match open_file_handle(self.path) {
            Result@Ok { value: handle } => {
                result Result@Ok { value: File@Open { path: self.path, handle } }
            },
            Result@Err { error } => {
                result Result@Err { error }
            }
        }
    }
}

// Usage demonstration
procedure example()
    uses fs.open
{
    let file = File@Closed { path: "/data.txt" }

    match file::open() {
        Result@Ok { value: opened } => {
            // 'file' is consumed; use 'opened' which has type File@Open
            opened::read(buffer)
        },
        Result@Err { error } => {
            eprintln("Failed to open file: {}", error)
        }
    }
    // ERROR E4006 if we try to use 'file' here: consumed by transition
}

### 10.3.2 Linear Consumption Semantics

Modal types enforce **linear consumption**: transitioning from one state to another consumes the source state value exactly once, preventing use-after-transition errors at compile time. This section specifies the permission requirements and consumption rules for transition procedures.

#### Own Permission Requirement

Transition procedures MUST consume their `self` parameter via `own` permission:

```
[Transition-Requires-Own]
procedure @S₁ -> @S₂ p(self: perm M@S₁, ...): M@S₂
perm ≠ own
--------------------------------------------------------
ERROR (E4003): transition procedures require 'own' permission on self
```

Permitting weaker permissions would allow several classes of unsound behavior:

1. **Aliasing violations**: If a transition procedure accepted `mut` permission, multiple `imm` aliases to the same value could observe inconsistent states:
   ```cursive
   let file_ref1 = imm file
   let file_ref2 = imm file
   file::open()  // If this were allowed with mut, file_ref1 and file_ref2
                 // would still be typed File@Closed but observe File@Open data
   ```

2. **Type unsoundness**: A `mut` reference typed as `M@S₁` that mutates in-place to state `@S₂` would violate type safety—the reference's type wouldn't match the runtime state.

3. **Use-after-transition**: Keeping an `imm` reference while the value transitions would allow accessing fields that no longer exist:
   ```cursive
   let ref = imm file  // File@Closed
   file::open()        // Transitions to File@Open
   // ref.path is still valid, but what if @Open has different fields?
   ```

Therefore the specification requires `own` permission so that the transition consumes the value entirely and prevents dangling references.

#### Use-After-Transition Prevention

After a transition call, the original binding is consumed and cannot be used:

```
[Use-After-Transition]
Γ ⊢ e : M@S₁
Γ ⊢ e::p(args) : M@S₂
Γ \ {e} ⊢ ...
e referenced in subsequent context
--------------------------------------------------------
ERROR (E4006): use of moved value (consumed by transition)
```

The compiler tracks consumption through the typing context. When a transition procedure call succeeds, the original binding `e` is removed from `Γ`, making any subsequent reference to `e` a compile error.

**Example: Use-after-transition error**

```cursive
procedure incorrect_usage() {
    let file = File@Closed { path: "/data.txt" }
    let opened = file::open()  // 'file' consumed here

    // ERROR E4006: use of moved value 'file'
    let path = file.path

    // OK: Use the returned value
    let path = opened.path
}
```

The error occurs because `file` was consumed by the call to `open()`, which returns a new binding `opened` of type `File@Open`. The original `file` binding is no longer valid.

#### Moved Value Tracking

The compiler tracks moved values through control flow:

```cursive
procedure conditional_transition(should_open: bool) {
    let file = File@Closed { path: "/data.txt" }

    if should_open {
        let opened = file::open()
        // 'file' consumed in this branch
    } else {
        // 'file' still valid in this branch
        println("Path: {}", file.path)
    }

    // ERROR E4006: 'file' may have been consumed
    // (moved in one branch but not the other)
    let x = file.path
}
```

To fix this, both branches must handle the file consistently:

```cursive
procedure fixed_conditional_transition(should_open: bool) {
    let file = File@Closed { path: "/data.txt" }

    let final_file = if should_open {
        match file::open() {
            Result@Ok { value: opened } => {
                // Use opened, then close it
                opened::close()
            },
            Result@Err { error: _ } => {
                // file consumed even on error path
                File@Closed { path: "/data.txt" }  // Reconstruct
            }
        }
    } else {
        file  // Move file out of the else branch
    }

    // OK: final_file available regardless of branch
    println("Path: {}", final_file.path)
}
```

### 10.3.3 Self-Loops and Non-Transition Procedures

#### Self-Loop Transitions

Procedures MAY transition from a state back to the same state, called a **self-loop transition**. These are useful for operations that logically "reset" or "update" a value while maintaining the same protocol state.

**Self-loop syntax:**

```cursive
modal Counter {
    @Active { count: i32 }

    // Self-loop: @Active -> @Active
    procedure @Active -> @Active
        increment(self: own Counter@Active): Counter@Active
    {
        result Counter@Active { count: self.count + 1 }
    }

    procedure @Active -> @Active
        add(self: own Counter@Active, amount: i32): Counter@Active
    {
        result Counter@Active { count: self.count + amount }
    }
}

// Usage: self-loop transitions still consume the original value
let counter = Counter@Active { count: 0 }
let counter = counter::increment()  // Returns new Counter@Active
let counter = counter::add(5)       // Returns another new Counter@Active
```

**Self-loop semantics:**

Even though the source and target states are the same (`@Active -> @Active`), the transition still:
1. Consumes the source value (requires `own`)
2. Produces a new target value
3. Invalidates the original binding

This enforces explicit value flow and prevents accidental aliasing, even for same-state transitions.

#### Non-Transition Procedures

Procedures declared without `TransitionSpec` are **non-transition procedures**. They operate on a modal value without changing its state:

```cursive
modal File {
    @Open { path: string, handle: FileHandle }

    // Non-transition: stays @Open
    procedure read(self: File@Open, buf: mut [u8]): Result<usize, Error>
        uses fs.read
    {
        self.handle::read(buf)
    }

    // Non-transition accessor with imm
    procedure size(self: imm File@Open): usize
        uses fs.stat
    {
        self.handle::stat().size
    }

    // Non-transition with own (consumes but returns same state)
    procedure reopen(self: own File@Open): File@Open
        uses fs.close, fs.open
    {
        // Close and reopen the file handle
        let path = self.path
        close_file_handle(self.handle)
        let new_handle = open_file_handle(path)
        result File@Open { path, handle: new_handle }
    }
}
```

**Non-transition procedure properties:**

1. **No `TransitionSpec`**: Absence of `@S₁ -> @S₂` annotation indicates state preservation.

2. **Flexible permissions**: MAY use `imm`, `mut`, or `own` permissions on `self`:
   - `imm`: Read-only access, no consumption
   - `mut`: Temporary mutable access, no consumption
   - `own`: Consumption, but returns same state type

3. **State preservation**: MUST NOT change the state annotation in the return type. If the procedure accepts `File@Open`, any returned modal value must also be `File@Open`.

4. **No consumption tracking** (unless `own`): Procedures with `imm` or `mut` permissions do NOT consume `self`, so the binding remains valid after the call.

**Type rule for non-transition procedures:**

```
[T-Non-Transition-Proc]
modal M declares @S
procedure p(self: perm M@S, x₁: T₁, ..., xₙ: Tₙ): U
    uses ε
{ body }
(no TransitionSpec present)
Γ, self: perm M@S, x₁: T₁, ..., xₙ: Tₙ ⊢ body : U
perm = imm OR perm = mut OR (perm = own AND U = M@S)
--------------------------------------------------------
Γ ⊢ p : (perm M@S, T₁, ..., Tₙ) -> U ! ε
```

The constraint ensures that non-transition procedures either:
- Use `imm` or `mut` and return any type (no consumption)
- Use `own` and return the same state type `M@S` (consumption with reconstruction)

#### Comparison: Transition vs. Non-Transition

| Aspect | Transition Procedure | Non-Transition Procedure |
|--------|---------------------|--------------------------|
| Syntax | `procedure @S₁ -> @S₂` | No `TransitionSpec` |
| Permission | MUST be `own` | MAY be `imm`, `mut`, or `own` |
| State change | Changes state (`@S₁` to `@S₂`) | Preserves state (`@S` to `@S`) |
| Consumption | Always consumes source | Consumes only if `own` |
| Binding invalidation | Always invalidates original binding | Only if `own` |
| Return type | `M@S₂` or `Result<M@S₂, E>` | Any type (typically `M@S` for `own`) |

**Example showing both:**

```cursive
modal Connection {
    @Connected { socket: Socket }
    @Disconnected { addr: string }

    // Transition procedure: changes state
    procedure @Connected -> @Disconnected
        disconnect(self: own Connection@Connected): Connection@Disconnected
        uses net.close
    {
        let addr = self.socket.remote_addr()
        self.socket::close()
        result Connection@Disconnected { addr }
    }

    // Non-transition procedure: preserves state with imm
    procedure peer_addr(self: imm Connection@Connected): string {
        result self.socket.remote_addr()
    }

    // Non-transition procedure: preserves state with mut
    procedure set_timeout(self: mut Connection@Connected, timeout: Duration) {
        self.socket::set_timeout(timeout)
    }

    // Non-transition procedure: preserves state with own
    procedure refresh(self: own Connection@Connected): Connection@Connected
        uses net.keepalive
    {
        self.socket::send_keepalive()
        result self  // Return same instance, same state
    }
}
```

### 10.3.4 Fallible Transitions

Many real-world state transitions can fail due to external conditions (I/O errors, network failures, resource exhaustion). Cursive supports **fallible transitions** that return `Result<M@S₂, E>` instead of directly returning `M@S₂`.

#### Fallible Transition Syntax

A fallible transition procedure returns a `Result` type where the success case contains the target state:

```cursive
modal Connection {
    @Disconnected { addr: string }
    @Connecting { addr: string, timeout: Duration }
    @Connected { addr: string, socket: Socket }
    @Failed { addr: string, error: Error }

    procedure @Connecting -> @Connected
        complete(self: own Connection@Connecting): Result<Connection@Connected, Connection@Failed>
        uses net.poll
    {
        match poll_connection() {
            Result@Ok { value: socket } => {
                result Result@Ok { value: Connection@Connected {
                    addr: self.addr,
                    socket
                } }
            },
            Result@Err { error } => {
                result Result@Err { error: Connection@Failed {
                    addr: self.addr,
                    error
                } }
            }
        }
    }
}
```

**Type rule for fallible transitions:**

```
[T-Fallible-Transition]
modal M declares @S₁, @S₂
procedure @S₁ -> @S₂ p(self: own M@S₁, params): Result<M@S₂, E>
Γ, self: M@S₁, params ⊢ body : Result<M@S₂, E>
--------------------------------------------------------
Γ ⊢ p : (M@S₁, params) -> Result<M@S₂, E> ! ε
```

The key constraint is that the success type MUST be `M@S₂`, matching the target state in the `TransitionSpec`.

#### Error Types in Fallible Transitions

The error type `E` in `Result<M@S₂, E>` has three common patterns:

**Pattern 1: Modal Error State**

The error type is another state of the same modal, enabling protocol-aware error handling:

```cursive
modal File {
    @Closed { path: string }
    @Open { path: string, handle: FileHandle }
    @Error { path: string, error: IOError }

    procedure @Closed -> @Open
        open(self: own File@Closed): Result<File@Open, File@Error>
        uses fs.open
    {
        match open_file_handle(self.path) {
            Result@Ok { value: handle } => {
                result Result@Ok { value: File@Open { path: self.path, handle } }
            },
            Result@Err { error } => {
                result Result@Err { error: File@Error { path: self.path, error } }
            }
        }
    }
}

// Usage: both branches receive modal values
let file = File@Closed { path: "/data.txt" }
match file::open() {
    Result@Ok { value: opened } => {
        // opened: File@Open
        opened::read(buffer)
    },
    Result@Err { error: error_state } => {
        // error_state: File@Error (still a modal value!)
        eprintln("Error: {}", error_state.error)
        let closed = error_state::reset()  // Transition back to Closed
    }
}
```

**Pattern 2: External Error Type**

The error type is a separate error type (not a modal state):

```cursive
modal Parser {
    @Start { input: string, pos: usize }
    @InString { input: string, pos: usize, start: usize }
    @Complete { result: AST }

    procedure @Start -> @InString
        begin_string(self: own Parser@Start): Result<Parser@InString, ParseError>
    {
        if self.input[self.pos] == '"' {
            result Result@Ok { value: Parser@InString {
                input: self.input,
                pos: self.pos + 1,
                start: self.pos
            } }
        } else {
            result Result@Err { error: ParseError.expected_quote(self.pos) }
        }
    }
}
```

In this pattern, the error does NOT produce a modal value. The original modal value is consumed, and the caller receives a non-modal error.

**Pattern 3: Hybrid (Union Type)**

The error type is a union of a modal state and other errors:

```cursive
modal Transaction {
    @Active { id: TxId }
    @Committed { id: TxId, log: CommitLog }
    @Aborted { id: TxId, reason: string }

    procedure @Active -> @Committed
        commit(self: own Transaction@Active): Result<Transaction@Committed, Transaction@Aborted | DBError>
        uses db.write
    {
        match try_commit(self.id) {
            Result@Ok { value: log } => {
                result Result@Ok { value: Transaction@Committed { id: self.id, log } }
            },
            Result@Err { error: CommitError::Conflict { reason } } => {
                result Result@Err { error: Transaction@Aborted { id: self.id, reason } }
            },
            Result@Err { error: db_error } => {
                result Result@Err { error: db_error }
            }
        }
    }
}
```

#### Consumption in Fallible Transitions

**Critical rule:** The source state is consumed regardless of success or failure. Once a fallible transition procedure is called, the original binding is invalid:

```cursive
let conn = Connection@Connecting { addr: "127.0.0.1:80", timeout: Duration.seconds(5) }
let result = conn::complete()

// ERROR E4006: 'conn' consumed by complete(), even though it returned Result
let timeout = conn.timeout
```

Both `Result@Ok` and `Result@Err` outcomes represent consumption of the original `Connection@Connecting` value.

#### Pattern Matching on Fallible Transitions

Pattern matching on `Result` values from fallible transitions is the idiomatic way to handle both success and failure:

```cursive
procedure attempt_connection(addr: string): Result<(), Error>
    uses net.connect, net.poll, net.write
{
    let conn = Connection@Disconnected { addr }

    match conn::connect(Duration.seconds(5)) {
        Result@Ok { value: connecting } => {
            match connecting::complete() {
                Result@Ok { value: connected } => {
                    match connected::send(b"Hello") {
                        Result@Ok { value: _ } => {
                            result Result@Ok { value: () }
                        },
                        Result@Err { error } => result Result@Err { error }
                    }
                },
                Result@Err { error: failed } => {
                    eprintln("Connection failed: {}", failed.error)
                    result Result@Err { error: Error.from(failed.error) }
                }
            }
        },
        Result@Err { error } => {
            result Result@Err { error }
        }
    }
}
```

#### Fallible Transition Recommendations

1. Programmers SHOULD use modal error states when the error is part of the protocol (for example, file opening failures map to `File@Error`, connection failures to `Connection@Failed`, and transaction conflicts to `Transaction@Aborted`).

2. Programmers SHOULD use external error types for unexpected or exceptional errors such as parse failures, validation errors, or programmer mistakes that warrant termination.

3. Specifications SHOULD document which conditions select each error outcome in the procedure's contract, as illustrated below:
   ```cursive
   procedure @Closed -> @Open
       open(self: own File@Closed): Result<File@Open, File@Error>
       uses fs.open
       must file_exists(self.path)
       // Returns File@Error if: permission denied, file locked, disk full
       // Panics if: path is invalid UTF-8 (programmer error)
   ```

### 10.3.5 Transition Graphs and Reachability

The collection of all transition procedures in a modal type forms a **transition graph** that represents the valid protocol flow. Understanding this graph is essential for verifying protocol correctness, detecting unreachable states, and analyzing modal type properties.

#### Transition Graph Structure

A transition graph for modal type `M` is a directed graph `G = (V, E)` where:

- **Vertices `V`**: The set of all declared states `states(M) = {@S₁, @S₂, ..., @Sₙ}`
- **Edges `E`**: Ordered pairs `(@Sᵢ, @Sⱼ)` representing transition procedures

Each transition procedure `procedure @Sᵢ -> @Sⱼ name(...)` induces an edge from `@Sᵢ` to `@Sⱼ`. If multiple procedures transition between the same states, they represent parallel edges (or can be treated as a single edge labeled with multiple procedure names).

**Graph construction:**

```
graph(M) = (V, E) where
  V = {@S | @S ∈ states(M)}
  E = {(@Sᵢ, @Sⱼ) | procedure @Sᵢ -> @Sⱼ p declared in M}
```

**Example:**

```cursive
modal File {
    @Closed { path: string }
    @Open { path: string, handle: FileHandle }
    @Error { path: string, error: IOError }

    procedure @Closed -> @Open
        open(self: own File@Closed): Result<File@Open, File@Error>
    { /* ... */ }

    procedure @Open -> @Closed
        close(self: own File@Open): File@Closed
    { /* ... */ }

    procedure @Error -> @Closed
        reset(self: own File@Error): File@Closed
    { /* ... */ }
}

// Transition graph:
// Vertices: {@Closed, @Open, @Error}
// Edges: {(@Closed, @Open), (@Open, @Closed), (@Error, @Closed)}
//
// Visual representation:
//        open
//   @Closed ⇄ @Open
//      ↑
//      │ reset
//   @Error
```

Note that `@Closed -> @Open` also has a potential failure path to `@Error` (via `Result` error case), but this is typically not represented as a direct graph edge unless there's an explicit transition procedure.

#### Reachability Analysis

**Reachability relation `⇝*`**: State `@S₂` is reachable from state `@S₁` if there exists a sequence of zero or more transition procedures that transform `@S₁` into `@S₂`.

**Formal definition:**

```
[Reachability-Base]
----------------------------------------
@S ⇝* @S  (reflexive: every state reaches itself in zero steps)

[Reachability-Step]
procedure @S₁ -> @S₂ p declared in M
@S₂ ⇝* @S₃
----------------------------------------
@S₁ ⇝* @S₃  (transitive: compose transitions)
```

**Reachability properties:**

1. **Reflexivity**: Every state is reachable from itself (`@S ⇝* @S`)
2. **Transitivity**: If `@S₁ ⇝* @S₂` and `@S₂ ⇝* @S₃`, then `@S₁ ⇝* @S₃`
3. **Not symmetric**: `@S₁ ⇝* @S₂` does NOT imply `@S₂ ⇝* @S₁`

**Computing reachability:**

Reachability can be computed via standard graph algorithms (DFS, BFS, or transitive closure). Given initial states `I ⊆ states(M)`, the set of reachable states is:

```
reachable(M, I) = {S | ∃S_init ∈ I. S_init ⇝* S}
```

#### Initial and Terminal States

**Initial states** are states with no incoming transitions (or states with constructors):

```
initial_states(M) = {@S ∈ states(M) | ∀(@Sᵢ, @S) ∈ E. false}
                    ∪ {@S | function constructing M@S exists}
```

In practice, initial states are often those that can be directly constructed without transitioning from another state.

**Terminal states** are states with no outgoing transitions:

```
terminal_states(M) = {@S ∈ states(M) | ∀(@S, @Sⱼ) ∈ E. false}
```

Terminal states represent "final" states in the protocol, where no further transitions are defined.

**Example:**

```cursive
modal Transaction {
    @NotStarted
    @Active { id: TxId }
    @Committed { id: TxId, log: CommitLog }
    @RolledBack { id: TxId, reason: string }

    procedure @NotStarted -> @Active
        begin(self: own Transaction@NotStarted): Transaction@Active
    { /* ... */ }

    procedure @Active -> @Committed
        commit(self: own Transaction@Active): Transaction@Committed
    { /* ... */ }

    procedure @Active -> @RolledBack
        rollback(self: own Transaction@Active, reason: string): Transaction@RolledBack
    { /* ... */ }
}

// Initial states: {@NotStarted}
// Terminal states: {@Committed, @RolledBack}
// Reachable from @NotStarted: {@NotStarted, @Active, @Committed, @RolledBack}
```

#### Unreachable States

An unreachable state is one that cannot be reached from any initial state:

```
[Unreachable-State]
modal M declares @S
∀@S_init ∈ initial_states(M). @S_init ⇝̸* @S
----------------------------------------
@S is unreachable
```

Unreachable states indicate a specification error or dead code. Implementations SHOULD issue a warning:

```
WARNING: state '@S' in modal 'M' is unreachable from initial states
note: no transition path leads to this state
help: consider adding a transition to this state or removing it
```

**Example of unreachable state:**

```cursive
modal Process {
    @NotStarted
    @Running { pid: i32 }
    @Stopped { exit_code: i32 }
    @Zombie { pid: i32 }  // No transition leads here!

    procedure @NotStarted -> @Running
        start(self: own Process@NotStarted): Process@Running
    { /* ... */ }

    procedure @Running -> @Stopped
        stop(self: own Process@Running): Process@Stopped
    { /* ... */ }

    // Missing: @Running -> @Zombie or @Stopped -> @Zombie
}

// WARNING: state '@Zombie' is unreachable
```

####Transition Graph Properties

Well-designed modal types often exhibit certain graph properties:

**1. Strong Connectivity**

A modal type is **strongly connected** if every state is reachable from every other state:

```
∀@S₁, @S₂ ∈ states(M). @S₁ ⇝* @S₂
```

Example: A light switch with `@On` and `@Off` states with bidirectional transitions is strongly connected.

**2. Acyclicity (DAG)**

A modal type forms a **Directed Acyclic Graph** (DAG) if there are no cycles:

```
∀@S ∈ states(M). @S ⇝* @S implies @S has self-loop only
```

Example: A parser with states `@Start -> @Parsing -> @Complete` (one-way flow) is a DAG.

**3. Tree Structure**

A modal type has a **tree structure** if:
- There is exactly one initial state (root)
- Each non-initial state has exactly one incoming edge
- No cycles exist

Tree-structured modals represent strictly hierarchical protocols.

**4. Reachability from All Initial States**

A **well-formed** modal type ensures all states are reachable:

```
∀@S ∈ states(M). ∃@S_init ∈ initial_states(M). @S_init ⇝* @S
```

Implementations SHOULD enforce this or warn when violated.

### 10.3.6 Dynamic Semantics

This section specifies the **operational semantics** of modal state transitions and procedure calls using big-step evaluation semantics. These rules define precisely how modal values are evaluated at runtime.

#### Big-Step Evaluation Notation

The dynamic semantics use **big-step evaluation judgments** of the form:

```
⟨e, σ⟩ ⇓ ⟨v, σ'⟩
```

This judgment states: "Expression `e` evaluated in store `σ` reduces to value `v` and produces updated store `σ'`."

Where:
- `e` is an expression
- `v` is a value (irreducible expression)
- `σ` is the store/heap mapping locations to values
- `σ'` is the updated store after evaluation

For modal types:
- Values `v ∈ [[M@S]]` denote runtime values of modal type `M` in state `@S`
- The store `σ` tracks the heap representation of modal values
- State transitions update the store by consuming and producing modal values

#### Transition Procedure Call Evaluation

**[E-Transition-Call]**

```
procedure @S₁ -> @S₂ p(self: own M@S₁, x₁: T₁, ..., xₙ: Tₙ): M@S₂ { body }
⟨recv, σ₀⟩ ⇓ ⟨v_self, σ₁⟩    (evaluate receiver)
v_self ∈ [[M@S₁]]
∀i ∈ {1..n}. ⟨argᵢ, σᵢ⟩ ⇓ ⟨vᵢ, σᵢ₊₁⟩   (evaluate arguments left-to-right)
vᵢ ∈ [[Tᵢ]]
⟨body[self ↦ v_self, x₁ ↦ v₁, ..., xₙ ↦ vₙ], σₙ₊₁⟩ ⇓ ⟨v_result, σ'⟩
v_result ∈ [[M@S₂]]
----------------------------------------
⟨recv::p(arg₁, ..., argₙ), σ₀⟩ ⇓ ⟨v_result, σ'⟩
```

This rule specifies the evaluation of a transition procedure call `recv::p(args)`:

1. **Receiver evaluation**: Evaluate `recv` to get `v_self`, a value of type `M@S₁`
2. **Argument evaluation**: Evaluate each argument `argᵢ` left-to-right to get values `vᵢ`
3. **Body evaluation**: Evaluate the procedure body with substitution `[self ↦ v_self, xᵢ ↦ vᵢ]`
4. **Result**: The body produces `v_result` of type `M@S₂`

**Key property:** The receiver `v_self` is **consumed** during the transition—it is moved into the procedure and cannot be used afterward. The store is threaded through evaluation to track this consumption.

#### Fallible Transition Evaluation

For procedures that return `Result<M@S₂, E>`:

**[E-Transition-Call-Fallible]**

```
procedure @S₁ -> @S₂ p(self: own M@S₁, ...): Result<M@S₂, E> { body }
⟨recv, σ₀⟩ ⇓ ⟨v_self, σ₁⟩
⟨body[self ↦ v_self, ...], σ₁⟩ ⇓ ⟨Result@Ok { value: v_result }, σ'⟩
v_result ∈ [[M@S₂]]
----------------------------------------
⟨recv::p(...), σ₀⟩ ⇓ ⟨Result@Ok { value: v_result }, σ'⟩

procedure @S₁ -> @S₂ p(self: own M@S₁, ...): Result<M@S₂, E> { body }
⟨recv, σ₀⟩ ⇓ ⟨v_self, σ₁⟩
⟨body[self ↦ v_self, ...], σ₁⟩ ⇓ ⟨Result@Err { error: e }, σ'⟩
e ∈ [[E]]
----------------------------------------
⟨recv::p(...), σ₀⟩ ⇓ ⟨Result@Err { error: e }, σ'⟩
```

When a transition fails (returns `Err`), the original value `v_self` is **consumed** but the state does **not** transition. The error `e` is returned instead.

**Critical invariant:** Even on failure, the original modal value is consumed. The caller cannot recover the original `M@S₁` value after a failed transition.

#### Non-Transition Procedure Evaluation

For procedures that don't perform state transitions (e.g., `procedure @S p(...)`):

**[E-Query-Call]**

```
procedure @S p(self: Permission M@S, x₁: T₁, ..., xₙ: Tₙ): R { body }
Permission ∈ {imm, mut}
⟨recv, σ₀⟩ ⇓ ⟨v_self, σ₁⟩
v_self ∈ [[M@S]]
∀i. ⟨argᵢ, σᵢ⟩ ⇓ ⟨vᵢ, σᵢ₊₁⟩
⟨body[self ↦ v_self, xᵢ ↦ vᵢ], σₙ₊₁⟩ ⇓ ⟨v_result, σ'⟩
v_result ∈ [[R]]
----------------------------------------
⟨recv::p(arg₁, ..., argₙ), σ₀⟩ ⇓ ⟨v_result, σ'⟩
```

Non-transition procedures **do not consume** the receiver. If the permission is `imm`, the receiver can be used again after the call. If `mut`, the receiver may be modified but remains accessible.

#### State Construction Evaluation

**[E-State-Construction]**

```
modal M declares @S { f₁: T₁, ..., fₙ: Tₙ }
∀i ∈ {1..n}. ⟨eᵢ, σᵢ₋₁⟩ ⇓ ⟨vᵢ, σᵢ⟩
vᵢ ∈ [[Tᵢ]]
σ' = allocate(σₙ, {f₁ ↦ v₁, ..., fₙ ↦ vₙ})
----------------------------------------
⟨M@S { f₁: e₁, ..., fₙ: eₙ }, σ₀⟩ ⇓ ⟨v, σ'⟩
where v ∈ [[M@S]] is the allocated value
```

Constructing a modal value in state `@S` allocates storage for all fields declared in `@S` and initializes them with the evaluated field expressions.

**Example evaluation trace:**

```cursive
modal File {
    @Closed
    @Open { fd: i32 }

    procedure @Closed -> @Open
        open(self: own File@Closed, path: string): Result<File@Open, IoError>
    { /* ... */ }
}

// Evaluation of: closed_file::open("/etc/passwd")
⟨closed_file, σ₀⟩ ⇓ ⟨v₁, σ₁⟩           // v₁ ∈ [[File@Closed]]
⟨"/etc/passwd", σ₁⟩ ⇓ ⟨v₂, σ₂⟩         // v₂ ∈ [[string]]
⟨body[self ↦ v₁, path ↦ v₂], σ₂⟩ ⇓ ⟨Result@Ok { value: v₃ }, σ₃⟩  // v₃ ∈ [[File@Open]]
----------------------------------------
⟨closed_file::open("/etc/passwd"), σ₀⟩ ⇓ ⟨Result@Ok { value: v₃ }, σ₃⟩
```

After this evaluation:
- `v₁` (the `File@Closed` value) is consumed and no longer accessible
- `v₃` (the new `File@Open` value) is available
- The store `σ₃` reflects the allocation of the new file descriptor

#### Determinism

**Theorem [Modal-Evaluation-Determinism]:**

For any modal procedure call expression `e` and initial store `σ`:

```
If ⟨e, σ⟩ ⇓ ⟨v₁, σ₁⟩ and ⟨e, σ⟩ ⇓ ⟨v₂, σ₂⟩
then v₁ = v₂ and σ₁ = σ₂
```

Modal evaluation is **deterministic**: given the same initial state, a transition always produces the same result value and final store.

**Proof sketch:**

1. Cursive expressions are deterministic (no undefined evaluation order)
2. Arguments are evaluated left-to-right (specified order)
3. State transitions are pure transformations (modulo effects)
4. No non-deterministic primitives in base language
5. Therefore, modal evaluation inherits determinism ∎

This determinism is critical for reasoning about modal protocols and ensures that state transitions are predictable and reproducible.

This completes the specification of state transitions and procedures. The next section covers state coercions.

---

---

## 10.4 State Coercions

By default, modal states have no subtyping relationship—`M@S₁` and `M@S₂` are completely distinct types even if `S₁` and `S₂` have similar fields. However, modal types MAY declare explicit **state coercions** that enable implicit conversion from one state to another under specified constraints. This section specifies the syntax, semantics, and well-formedness rules for state coercions.

### 10.4.1 Coercion Declaration Syntax

State coercions are declared inside modal bodies using the `coerce` keyword:

```ebnf
StateCoercion ::= "coerce" "@" Ident "<:" "@" Ident CoercionConstraint?

CoercionConstraint ::= "{"
                       ("cost" ":" Cost)?
                       ("requires" ":" Permission)?
                       ("uses" ":" EffectSet)?
                       "}"

Cost            ::= "O(1)"
Permission      ::= "own" | "mut" | "imm"
EffectSet       ::= Ident ("." Ident)* | "∅"
```

A coercion declaration `coerce @S₁ <: @S₂` establishes that values of type `M@S₁` can be implicitly coerced to type `M@S₂`, subject to the constraints specified in the optional `CoercionConstraint` block.

**Basic example:**

```cursive
modal string {
    @Owned { ptr: *u8, len: usize, cap: usize }
    @View { ptr: *u8, len: usize }

    coerce @Owned <: @View {
        cost: O(1)
        uses: ∅
    }
}
```

This declares that `string@Owned` can be implicitly coerced to `string@View` with zero runtime cost while performing no effects. Because no `requires` clause is present, the coercion accepts whichever permission is held by the operand and preserves it.

### 10.4.2 Structural Subset Requirement

**Fundamental constraint:** State coercions MUST represent **field projection**. The target state's fields must be a structural subset of the source state's fields.

**Formal rule:**

```
[Coercion-Structural-Subset]
modal M declares @S₁ { f₁: T₁, ..., fₙ: Tₙ, g₁: U₁, ..., gₘ: Uₘ }
modal M declares @S₂ { f₁: T₁, ..., fₙ: Tₙ }
fields(@S₂) = {f₁: T₁, ..., fₙ: Tₙ}
fields(@S₁) = {f₁: T₁, ..., fₙ: Tₙ, g₁: U₁, ..., gₘ: Uₘ}
fields(@S₂) ⊆ fields(@S₁)  (structural subset with matching types)
coerce @S₁ <: @S₂ declared in M
--------------------------------------------------------
coercion is structurally valid
```

**Requirements:**

1. Every field in `@S₂` MUST appear in `@S₁` with the **exact same type**
2. Field names and types must match exactly (no variance)
3. `@S₁` MAY have additional fields not present in `@S₂`
4. Field order is not significant

**Valid coercion:**

```cursive
modal Buffer<T> {
    @Owning { data: *T, len: usize, cap: usize, allocator: Allocator }
    @ReadOnly { data: *T, len: usize }

    // Valid: ReadOnly fields are subset of Owning fields
    coerce @Owning <: @ReadOnly {
        cost: O(1)
        uses: ∅
    }
}
```

**Invalid coercions:**

```cursive
modal Bad {
    @StateA { x: i32, y: string }
    @StateB { x: i64, y: string }  // Type mismatch: i32 vs i64

    // ERROR: Field 'x' has different types
    coerce @StateA <: @StateB
}

modal Bad2 {
    @StateC { a: i32 }
    @StateD { a: i32, b: string }  // StateD has MORE fields

    // ERROR: Cannot coerce to a state with additional fields
    coerce @StateC <: @StateD
}
```

The structural subset requirement ensures coercions are **zero-cost field projections** at runtime; the compiler can simply ignore fields not present in the target state with no data transformation.

### 10.4.3 Coercion Constraints

Each coercion may specify constraints via the `CoercionConstraint` block:

#### Cost Constraint

The `cost` field SHALL be `O(1)`. Any other annotation is ill-formed.

**Constraint:**

```
[Coercion-Cost-O1-Implies-Projection]
coerce @S₁ <: @S₂ { cost: O(1), ... }
--------------------------------------------------------
Coercion MUST be pure field projection (no computation, no effects)
```

Whenever a cost is declared, the coercion MUST NOT:
- Perform any computation
- Allocate memory
- Invoke functions
- Perform I/O or effects

**Example:**

```cursive
modal Slice<T> {
    @Owned { ptr: *T, len: usize, cap: usize }
    @Borrowed { ptr: *T, len: usize }

    // O(1): pure field projection
    coerce @Owned <: @Borrowed {
        cost: O(1)
        uses: ∅
    }
}
```

#### Permission Constraint

The `requires` field specifies the minimum permission needed for the coercion:

- **`own`**: Requires ownership (consumes the source value)
- **`mut`**: Requires mutable permission
- **`imm`**: Requires immutable permission (read-only)

If the `requires` clause is omitted, the coercion accepts whatever permission already applies to the operand and preserves that permission on the coerced value.

**Type rule with permission:**

```
[T-Coercion-Permission]
Γ ⊢ e : M@S₁ with permission perm
(@S₁ <: @S₂) ∈ coercions(M)
requires_perm = requires(coercion)
perm ≥ requires_perm  (permission lattice: own ≥ mut ≥ imm)
--------------------------------------------------------
Γ ⊢ e : M@S₂ (coercion allowed)
```

**Examples:**

```cursive
modal Resource {
    @Owned { handle: Handle, metadata: Metadata }
    @View { handle: Handle }

    // Requires ownership (consumes source)
    coerce @Owned <: @View {
        cost: O(1)
        requires: own
        uses: ∅
    }
}

// Usage:
let resource = Resource@Owned { handle, metadata }
let view: Resource@View = resource  // OK: consumes 'resource'
// resource is no longer valid here
```

```cursive
modal Data {
    @Full { values: [i32; 100], count: usize }
    @Summary { count: usize }

    // Works with the caller's existing permission (no consumption)
    coerce @Full <: @Summary {
        cost: O(1)
        uses: ∅
    }
}

// Usage:
let data = imm Data@Full { values, count }
let summary: Data@Summary = data  // OK: no consumption
// data is still valid here
```

#### Effect Constraint

Implicit coercions MUST be effect-free. The optional `uses` clause, when present, SHALL be `uses: ∅`. Any coercion that requires non-empty effects MUST be expressed as an explicit transition procedure instead of a coercion.

### 10.4.4 Acyclicity Requirement

The set of all coercions in a modal type forms a **coercion graph**. This graph MUST be acyclic (a DAG).

**Formal requirement:**

```
[Coercion-Acyclic]
modal M
coercion_graph(M) = (states(M), {(@S₁, @S₂) | coerce @S₁ <: @S₂ ∈ M})
coercion_graph(M) contains cycle
--------------------------------------------------------
ERROR: cyclic state coercion detected
```

Cycles in the coercion graph create ambiguity in type checking and can lead to non-termination in coercion resolution, so acyclicity is mandatory.

**Invalid (cyclic):**

```cursive
modal Bad {
    @A { x: i32 }
    @B { x: i32 }
    @C { x: i32 }

    coerce @A <: @B
    coerce @B <: @C
    coerce @C <: @A  // ERROR: Creates cycle A -> B -> C -> A
}
```

**Valid (acyclic):**

```cursive
modal Good {
    @Full { a: i32, b: i32, c: i32 }
    @Partial { a: i32, b: i32 }
    @Minimal { a: i32 }

    coerce @Full <: @Partial
    coerce @Partial <: @Minimal
    // Forms a DAG: Full -> Partial -> Minimal
}
```

### 10.4.5 Implicit Coercion Semantics

When a coercion is declared, the type system allows implicit conversion in certain contexts:

**Implicit coercion contexts:**

1. **Assignment:**
   ```cursive
   let view: string@View = owned  // owned: string@Owned
   ```

2. **Function arguments:**
   ```cursive
   function takes_view(s: string@View) { }
   let owned: string@Owned = ...
   takes_view(owned)  // Implicit coercion
   ```

3. **Return expressions:**
   ```cursive
   function to_view(): string@View {
       let owned: string@Owned = ...
       result owned  // Implicit coercion
   }
   ```

**Type rule for implicit coercion:**

```
[T-Implicit-Coercion]
Γ ⊢ e : M@S₁
(@S₁ <: @S₂) ∈ coercions(M)
permission(e) ≥ requires(coercion)
uses(coercion) ⊆ available_effects(Γ)
Γ expects type M@S₂
--------------------------------------------------------
Γ ⊢ e : M@S₂  (coercion inserted implicitly)
```

### 10.4.6 Explicit vs. Implicit Coercions

**Implicit coercions** (via `coerce` declaration):
- Automatically applied by the type system
- MUST declare `cost: O(1)` and either omit `uses` or set `uses: ∅`
- Are limited to pure field projections

**Explicit transitions** (via procedure):
- Require explicit procedure call
- Can perform arbitrary computation
- Suitable for costly or effectful conversions

**Example showing both:**

```cursive
modal string {
    @Owned { ptr: *u8, len: usize, cap: usize }
    @View { ptr: *u8, len: usize }

    // Implicit: zero-cost projection
    coerce @Owned <: @View {
        cost: O(1)
        uses: ∅
    }

    // Explicit: requires allocation
    procedure @View -> @Owned
        to_owned(self: string@View): string@Owned
        uses alloc.heap
    {
        // Allocate new buffer and copy data
        let ptr = allocate(self.len)
        copy_memory(ptr, self.ptr, self.len)
        result string@Owned { ptr, len: self.len, cap: self.len }
    }
}

// Usage:
let owned: string@Owned = string.from("hello")
let view: string@View = owned        // Implicit coercion
let owned2 = view::to_owned()        // Explicit procedure call
```

This completes the specification of state coercions.

---

## 10.5 Pattern Matching on States

### 10.5.1 State Pattern Syntax

Modal values MAY be pattern matched by state:

```ebnf
Pattern ::= ... | ModalStatePattern
ModalStatePattern ::= TypeName "@" StateName "{" FieldPattern* "}"
                    | TypeName "@" StateName
```

**Type refinement:**

```
[Match-Modal-Refine]
Γ ⊢ e : M@S₁ | M@S₂ | ... | M@Sₙ
match e {
    M@S₁ pattern₁ => body₁,
    M@S₂ pattern₂ => body₂,
    ...
}
∀i. Γ, patternᵢ bindings ⊢ bodyᵢ : T
----------------------------------------
Γ ⊢ match e { ... } : T
```

**Exhaustiveness:**

```
[Match-Modal-Non-Exhaustive]
Γ ⊢ e : M@S₁ | M@S₂ | ... | M@Sₙ
match e { arms }
∃Sᵢ. Sᵢ ∉ covered_states(arms)
----------------------------------------
ERROR (E4005): non-exhaustive pattern match (missing state '@Sᵢ')
```

**Example:**

```cursive
// NOTE: Using OptionExample for demonstration (stdlib Option is an enum)
modal OptionExample<T> {
    @None
    @Some { value: T }
}

procedure unwrap_or<T>(opt: OptionExample<T>@None | OptionExample<T>@Some, default: T): T {
    match opt {
        OptionExample@None => result default,
        OptionExample@Some { value } => result value
    }
}
```

---

## 10.6 Integration with Permissions

Modal types integrate with Cursive's permission system (CITE: Part IV §4) to enforce safe state transitions and prevent use-after-transition errors. This section specifies the interaction between modal state annotations and permission annotations (`own`, `mut`, `imm`), permission requirements for procedures, and restrictions on copyability.

### 10.6.1 Permission Requirements for Transition Procedures

**Fundamental rule:** Transition procedures MUST require `own` permission on the `self` parameter.

```
[Modal-Transition-Own-Required]
procedure @S₁ -> @S₂ p(self: perm M@S₁, ...): M@S₂
perm ≠ own
----------------------------------------
ERROR (E4003): transition procedures require 'own' permission on self
```

This requirement prevents multiple aliases to a modal value from observing inconsistent states. If `mut` were allowed, the following program would be unsound:

```cursive
// UNSOUND (not allowed in Cursive)
let file = File@Closed { path: "/data.txt" }
let alias1 = imm file  // alias1: File@Closed
let alias2 = imm file  // alias2: File@Closed

file::open()  // If this were allowed with mut...
// alias1 and alias2 still typed as File@Closed
// but the underlying data is now File@Open
// Type system violated!
```

Requiring `own` ensures that no aliases exist when a transition occurs, maintaining type soundness.

**Default permission:** If no permission is specified on a transition procedure's `self` parameter, it defaults to `own`:

```cursive
modal State {
    @A { x: i32 }
    @B { x: i32, y: i32 }

    // These are equivalent:
    procedure @A -> @B
        transition1(self: State@A): State@B { }

    procedure @A -> @B
        transition2(self: own State@A): State@B { }
}
```

### 10.6.2 Permission Flexibility for Non-Transition Procedures

Non-transition procedures (those without `TransitionSpec`) MAY use any permission on `self`:

```
[Modal-Non-Transition-Permission-Flexible]
procedure p(self: perm M@S, ...): U
(no TransitionSpec present)
perm ∈ {own, mut, imm}
----------------------------------------
procedure is well-formed
```

**Imm permission** (read-only access):

```cursive
modal File {
    @Open { path: string, handle: FileHandle, size: usize }

    // Read-only accessor
    procedure get_size(self: imm File@Open): usize {
        result self.size
    }

    // Read-only inspection
    procedure is_large(self: imm File@Open): bool {
        result self.size > 1024 * 1024
    }
}

// Usage: no consumption
let file = File@Open { path: "/data.txt", handle, size: 1024 }
let size = file::get_size()    // file NOT consumed
let large = file::is_large()   // file still valid
println("Size: {}", size)       // file still valid
```

**Mut permission** (temporary mutable access):

```cursive
modal File {
    @Open { path: string, handle: FileHandle, buffer: [u8] }

    // Mutable operation that doesn't change state
    procedure write(self: mut File@Open, data: [u8]): Result<usize, Error>
        uses fs.write
    {
        self.handle::write(data)
    }

    // In-place buffer modification
    procedure clear_buffer(self: mut File@Open) {
        self.buffer::clear()
    }
}

// Usage: no consumption, but temporary exclusive access required
let file = mut File@Open { path: "/data.txt", handle, buffer }
file::write(b"Hello")           // file still valid (mut access)
file::clear_buffer()            // file still valid
```

**Own permission** (consumption without state change):

```cursive
modal File {
    @Open { path: string, handle: FileHandle }

    // Consumes and returns same state (perhaps after cleanup)
    procedure reopen(self: own File@Open): File@Open
        uses fs.close, fs.open
    {
        let path = self.path
        close_file_handle(self.handle)
        let new_handle = open_file_handle(path)
        result File@Open { path, handle: new_handle }
    }
}

// Usage: consumption, but returns same state
let file = File@Open { path: "/data.txt", handle }
let file = file::reopen()  // Rebind to returned value
```

### 10.6.3 Permission Interaction with State Annotations

Permission annotations and state annotations are orthogonal but complementary:

**Syntax:** `perm M@S` where:
- `perm` controls access rights (`own`, `mut`, `imm`)
- `M@S` specifies the modal type and state

**Examples:**

```cursive
let owned_closed: own File@Closed = File@Closed { path: "/data.txt" }
let mut_open: mut File@Open = /* ... */
let imm_open: imm File@Open = /* ... */
```

**Permission weakening** applies to modal types:

```cursive
let owned: own File@Open = File@Open { path, handle }
let mutable: mut File@Open = owned    // Weaken own to mut
let immutable: imm File@Open = owned  // Weaken own to imm
```

**State is preserved during permission weakening:**

```cursive
let owned: own File@Open = File@Open { path, handle }
let imm: imm File@Open = owned  // OK: permission weakens, state preserved

// NOT allowed: cannot change state via permission weakening
let imm_closed: imm File@Closed = owned  // ERROR: state mismatch
```

### 10.6.4 Modal Types and the Copy Contract

**Rule:** Modal types MUST NOT implement the `Copy` contract.

```
[Modal-Not-Copy]
M is modal type
----------------------------------------
M@S MUST NOT implement Copy for any state S
```

Allowing `Copy` would violate linear consumption semantics:

```cursive
// UNSOUND if Copy were allowed:
let file1 = File@Closed { path: "/data.txt" }  // Copyable
let file2 = file1                               // Implicit copy

match file1::open() {  // Transition file1 to @Open
    Result@Ok { value: opened1 } => {
        // file1 consumed and now @Open
        // But file2 is still @Closed!
        // Two references to same underlying resource in different states
        // Disaster!
    }
}
```

The linear consumption model requires that transitions consume the source value, which is incompatible with implicit copying.

**Explicit cloning:** Modal types MAY implement explicit cloning procedures if semantically appropriate:

```cursive
modal Config {
    @Draft { settings: Map<string, string> }
    @Validated { settings: Map<string, string>, checksum: u64 }

    // Explicit clone (not Copy)
    procedure clone_draft(self: imm Config@Draft): Config@Draft {
        result Config@Draft {
            settings: self.settings::clone()
        }
    }
}

// Usage: explicit call required
let config1 = Config@Draft { settings }
let config2 = config1::clone_draft()  // Explicit
```

### 10.6.5 Borrowing and Modal Types

Borrowing (creating `imm` or `mut` references) works with modal types:

```cursive
procedure process_file(file: imm File@Open) {
    // Read from file without consuming it
    let size = file.size
    println("Processing file of size {}", size)
}

let file = File@Open { path: "/data.txt", handle, size: 1024 }
process_file(imm file)  // Borrow as imm
// file still valid here
```

**Limitation:** Cannot transition through a borrowed reference:

```cursive
procedure try_close(file: mut File@Open) {
    // ERROR: cannot transition through mut reference
    // file::close()  // Would require 'own', not 'mut'
}
```

This completes the specification of modal type integration with permissions.

---

---

## 10.7 Integration with Effects

Modal procedures integrate with Cursive's effect system (CITE: Part VII §7) to declare and track computational effects performed during state transitions and operations. This section specifies how effects are declared on modal procedures, checked at call sites, and propagated through transition chains.

### 10.7.1 Effect Declarations on Transition Procedures

Transition procedures MAY declare effects via the `uses` clause:

```cursive
modal File {
    @Closed { path: string }
    @Open { path: string, handle: FileHandle }

    procedure @Closed -> @Open
        open(self: own File@Closed): Result<File@Open, Error>
        uses fs.open           // Effect requirement
    {
        match open_file_handle(self.path) {
            Result@Ok { value: handle } => {
                result Result@Ok { value: File@Open { path: self.path, handle } }
            },
            Result@Err { error } => {
                result Result@Err { error }
            }
        }
    }
}
```

**Effect checking rule:**

```
[T-Modal-Transition-Effect]
procedure @S₁ -> @S₂ p(self: own M@S₁, ...): M@S₂
    uses ε
Γ ⊢ e::p(args) : M@S₂
ε ⊆ available_effects(Γ)
----------------------------------------
call is well-formed

ε ⊄ available_effects(Γ)
----------------------------------------
ERROR: effect ε not available in calling context
```

**Caller must provide the required effects:**

```cursive
// Caller provides fs.open effect
procedure use_file()
    uses fs.open           // Declares fs.open available
{
    let file = File@Closed { path: "/data.txt" }
    let result = file::open()  // OK: fs.open available
}

// ERROR: effect not available
procedure bad_use_file() {
    let file = File@Closed { path: "/data.txt" }
    let result = file::open()  // ERROR: fs.open not available
}
```

### 10.7.2 State-Specific Effects

Different states may have different effect requirements:

```cursive
modal Connection {
    @Disconnected { addr: string }
    @Connecting { addr: string, socket: Socket }
    @Connected { addr: string, socket: Socket }

    // Effect required: network connect
    procedure @Disconnected -> @Connecting
        connect(self: own Connection@Disconnected): Result<Connection@Connecting, Error>
        uses net.connect
    {
        match create_socket(self.addr) {
            Result@Ok { value: socket } => {
                result Result@Ok { value: Connection@Connecting {
                    addr: self.addr,
                    socket
                } }
            },
            Result@Err { error } => result Result@Err { error }
        }
    }

    // Effect required: network poll
    procedure @Connecting -> @Connected
        complete(self: own Connection@Connecting): Result<Connection@Connected, Error>
        uses net.poll
    {
        match poll_connection(self.socket) {
            Result@Ok { value: _ } => {
                result Result@Ok { value: Connection@Connected {
                    addr: self.addr,
                    socket: self.socket
                } }
            },
            Result@Err { error } => result Result@Err { error }
        }
    }

    // Effect required: network send (only when Connected)
    procedure send(self: mut Connection@Connected, data: [u8]): Result<usize, Error>
        uses net.send
    {
        self.socket::send(data)
    }
}
```

### 10.7.3 Effect Composition in Transition Chains

When multiple transitions are chained, effects accumulate:

```cursive
procedure full_connection_flow(addr: string): Result<(), Error>
    uses net.connect, net.poll, net.send  // All effects required
{
    let conn = Connection@Disconnected { addr }

    match conn::connect() {  // Requires net.connect
        Result@Ok { value: connecting } => {
            match connecting::complete() {  // Requires net.poll
                Result@Ok { value: connected } => {
                    match connected::send(b"Hello") {  // Requires net.send
                        Result@Ok { value: _ } => {
                            result Result@Ok { value: () }
                        },
                        Result@Err { error } => result Result@Err { error }
                    }
                },
                Result@Err { error } => result Result@Err { error }
            }
        },
        Result@Err { error } => result Result@Err { error }
    }
}
```

**Effect set union:**

```
effects_required(chain) = ε₁ ∪ ε₂ ∪ ... ∪ εₙ
where εᵢ are the effects of each transition in the chain
```

### 10.7.4 Pure Transitions

Transitions that perform no effects have empty effect sets:

```cursive
modal Validation<T> {
    @Unvalidated { value: T }
    @Validated { value: T }

    // Pure transition (no effects)
    procedure @Unvalidated -> @Validated
        validate(self: own Validation<T>): Result<Validation@Validated, Error>
        uses ∅  // Or omit `uses` clause entirely
        must is_valid(self.value)
    {
        if check_invariants(self.value) {
            result Result@Ok { value: Validation@Validated { value: self.value } }
        } else {
            result Result@Err { error: Error.validation_failed() }
        }
    }
}

// No effect requirements
procedure validate_data<T>(data: T): Result<Validation<T>@Validated, Error> {
    let unvalidated = Validation@Unvalidated { value: data }
    result unvalidated::validate()  // No effects required
}
```

### 10.7.5 Effect Polymorphism with Modals

Modal procedures can be effect-polymorphic, abstracting over effects:

```cursive
modal Resource<T> {
    @Acquired { value: T }
    @Released

    // Effect-polymorphic transition
    procedure @Acquired -> @Released
        release<ε>(self: own Resource<T>@Acquired, cleanup: () -> () ! ε): Resource<T>@Released
        uses ε  // Polymorphic over effect ε
    {
        cleanup()  // Performs effect ε
        result Resource@Released
    }
}

// Usage with specific effect
procedure use_resource()
    uses io.write
{
    let resource = Resource@Acquired { value: data }

    let released = resource::release(|| {
        println("Cleaning up")  // Requires io.write
    })
}
```

### 10.7.6 Non-Transition Procedures and Effects

Non-transition procedures also declare effects:

```cursive
modal Database {
    @Connected { conn: DBConnection }

    // Non-transition procedure with effect
    procedure query(self: imm Database@Connected, sql: string): Result<Rows, Error>
        uses db.read
    {
        self.conn::execute(sql)
    }

    // Non-transition procedure with effect
    procedure execute(self: mut Database@Connected, sql: string): Result<(), Error>
        uses db.write
    {
        self.conn::execute_mut(sql)
    }
}
```

This completes the specification of modal type integration with effects.

---

---

## 10.8 Integration with Contracts

Modal types integrate with Cursive's contract system (CITE: Part VII §7) to specify and verify state invariants and transition contracts. This section specifies how contracts are declared on states and transition procedures, when they are checked, and how they interact with modal verification.

### 10.8.1 State Invariants

**State invariants** are boolean predicates that MUST hold for all values of a given state. They are declared using `where` clauses on state declarations:

```cursive
modal Counter {
    @Zero { count: i32 }
        where self.count == 0           // State invariant

    @Positive { count: i32 }
        where self.count > 0

    @Negative { count: i32 }
        where self.count < 0
}
```

**Invariant checking points:**

1. **At construction:** When a modal value is constructed in state `@S`, the invariant for `@S` is checked:
   ```cursive
   let counter = Counter@Zero { count: 0 }  // OK: invariant satisfied
   let bad = Counter@Zero { count: 1 }      // ERROR: invariant violated
   ```

2. **At transition targets:** When a transition produces a value in state `@S₂`, the invariant for `@S₂` is checked:
   ```cursive
   procedure @Zero -> @Positive
       increment(self: own Counter@Zero): Counter@Positive
   {
       result Counter@Positive { count: self.count + 1 }
       // Invariant checked: result.count > 0
   }
   ```

3. **At pattern match binding:** When pattern matching refines a type to a specific state, the invariant is assumed to hold within that arm.

**Formal rule:**

```
[State-Invariant-Check]
modal M declares @S { fields } where I(self)
e constructs or produces M@S
----------------------------------------
I(e) MUST hold (checked statically or at runtime)
```

### 10.8.2 Transition Preconditions

Transition procedures MAY declare preconditions via `must` clauses:

```cursive
modal BankAccount {
    @Active { balance: i64, overdraft_limit: i64 }
    @Closed { final_balance: i64 }

    procedure @Active -> @Closed
        close(self: own BankAccount@Active): BankAccount@Closed
        must self.balance >= 0  // Cannot close with negative balance
    {
        result BankAccount@Closed {
            final_balance: self.balance
        }
    }
}

// Usage:
let account = BankAccount@Active { balance: 100, overdraft_limit: 50 }
let closed = account::close()  // OK: precondition satisfied

let overdrawn = BankAccount@Active { balance: -25, overdraft_limit: 50 }
let closed2 = overdrawn::close()  // ERROR (or panic): precondition violated
```

**Formal rule:**

```
[Transition-Precondition]
procedure @S₁ -> @S₂ p(self: own M@S₁, params): M@S₂
    must P(self, params)
e::p(args) called
----------------------------------------
P(e, args) MUST hold at call site
```

### 10.8.3 Transition Postconditions

Transition procedures MAY declare postconditions via `will` clauses:

```cursive
modal Counter {
    @Zero { count: i32 }
    @Positive { count: i32 }

    procedure @Zero -> @Positive
        increment(self: own Counter@Zero): Counter@Positive
        will result.count == self.count + 1  // Postcondition
        will result.count > 0                 // Additional postcondition
    {
        result Counter@Positive {
            count: self.count + 1
        }
    }
}
```

**Using `@old` for initial state references:**

```cursive
modal File {
    @Closed { path: string }
    @Open { path: string, handle: FileHandle, bytes_read: usize }

    procedure @Closed -> @Open
        open(self: own File@Closed): Result<File@Open, Error>
        will result.is_ok() implies result.unwrap().path == @old(self.path)
        will result.is_ok() implies result.unwrap().bytes_read == 0
    {
        match open_file_handle(self.path) {
            Result@Ok { value: handle } => {
                result Result@Ok { value: File@Open {
                    path: self.path,
                    handle,
                    bytes_read: 0
                } }
            },
            Result@Err { error } => result Result@Err { error }
        }
    }
}
```

**Formal rule:**

```
[Transition-Postcondition]
procedure @S₁ -> @S₂ p(self: own M@S₁, params): M@S₂
    will Q(result, @old(self), params)
result = e::p(args)
----------------------------------------
Q(result, e, args) MUST hold after call
```

### 10.8.4 State Invariant Preservation

State invariants interact with transition contracts to ensure correctness:

**Theorem (Invariant Preservation):** If a transition produces a value in state `@S₂`, the invariant for `@S₂` holds.

```
Proof:
1. Transition produces v: M@S₂
2. By construction check, invariant I₂(v) is verified
3. Therefore, all values of type M@S₂ satisfy I₂
```

**Example showing invariant+postcondition interaction:**

```cursive
modal SafeCounter {
    @InRange { value: i32 }
        where self.value >= 0 && self.value <= 100  // Invariant

    procedure @InRange -> @InRange
        increment(self: own SafeCounter@InRange): Result<SafeCounter@InRange, Error>
        must self.value < 100                    // Precondition
        will result.is_ok() implies result.unwrap().value == @old(self.value) + 1
    {
        if self.value < 100 {
            result Result@Ok { value: SafeCounter@InRange {
                value: self.value + 1
                // Invariant automatically checked:
                // 0 <= self.value + 1 <= 100
            } }
        } else {
            result Result@Err { error: Error.overflow() }
        }
    }
}
```

### 10.8.5 Multi-State Contracts

Contracts can reference multiple states for complex protocols:

```cursive
modal Transaction {
    @NotStarted
    @Active { id: TxId, operations: Vec<Operation> }
    @Committed { id: TxId, operations: Vec<Operation>, commit_time: Timestamp }

    procedure @Active -> @Committed
        commit(self: own Transaction@Active): Transaction@Committed
        must self.operations.len() > 0  // Must have operations
        will result.id == @old(self.id)  // ID preserved
        will result.operations == @old(self.operations)  // Operations preserved
        will result.commit_time > @old(current_time())   // Committed in future
    {
        result Transaction@Committed {
            id: self.id,
            operations: self.operations,
            commit_time: current_time()
        }
    }
}
```

### 10.8.6 Contract Checking Modes

Contract checking behavior depends on compilation mode:

1. **Debug mode:** All contracts checked at runtime
   - State invariants verified at construction and after transitions
   - Preconditions verified before transition calls
   - Postconditions verified after transition returns

2. **Release mode:** Contracts may be elided (implementation-defined)
   - Static analysis attempts to prove contracts
   - Runtime checks inserted only if proof fails
   - Contracts serve as optimization hints

3. **Verification mode:** Contracts fully verified statically
   - SMT solver or proof assistant used
   - Compilation fails if contracts cannot be proven
   - No runtime overhead

**Recommendation:** Always design contracts to be provable statically when possible.

This completes the specification of modal type integration with contracts.

---

---

## 10.9 State Inference and Holes

Modal state annotations can often be inferred by the compiler, reducing syntactic burden while maintaining type safety. This section specifies the syntax of state holes, inference algorithm, constraint generation, and solving mechanisms.

### 10.9.1 State Hole Syntax

State holes use the `_?` syntax to indicate that a state annotation should be inferred:

```ebnf
ModalStateType ::= Ident "@" StateAnnotation

StateAnnotation ::= Ident          // Explicit state
                  | "_?"            // Inferred state (hole)
```

**Examples:**

```cursive
// Explicit state annotation
let file1: File@Closed = File@Closed { path: "/data.txt" }

// Inferred state annotation (hole)
let file2: File@_? = File@Closed { path: "/data.txt" }
// Compiler infers: File@Closed

// Completely inferred (no annotation)
let file3 = File@Closed { path: "/data.txt" }
// Compiler infers: File@Closed
```

### 10.9.2 Inference Contexts

State inference operates in contexts where the state can be determined from surrounding code:

**1. Construction expressions:**

```cursive
let file: File@_? = File@Closed { path: "/data.txt" }
// Inferred from constructor: File@Closed
```

**2. Procedure call returns:**

```cursive
let file = File@Closed { path: "/data.txt" }
let opened: Result<File@_?, Error> = file::open()
// Inferred from procedure signature: Result<File@Open, Error>
```

**3. Pattern matching:**

```cursive
match result {
    Result@Ok { value: file } => {
        // file inferred as File@Open from Result<File@Open, Error>
    },
    Result@Err { error: _ } => { }
}
```

**4. Function arguments:**

```cursive
function process_open_file(f: File@Open) { }

let file = File@Closed { path: "/data.txt" }
let opened = file::open().unwrap()
process_open_file(opened)  // State inferred as @Open from parameter type
```

### 10.9.3 Constraint Generation

The compiler generates constraints from expressions involving modal types:

**Constraint types:**

1. **Equality constraints** `S₁ = S₂`:
   ```cursive
   let file: File@S₁ = expr_of_type_File@S₂
   // Generates constraint: S₁ = S₂
   ```

2. **Construction constraints** `S = @StateName`:
   ```cursive
   let x: M@S = M@Closed { ... }
   // Generates constraint: S = @Closed
   ```

3. **Procedure constraints** from signatures:
   ```cursive
   procedure @S₁ -> @S₂ p(...): M@S₂
   let result: M@S = e::p(...)
   // Generates constraints: e: M@S₁, result: M@S₂, S = S₂
   ```

**Formal constraint generation:**

```
[Constraint-Construction]
e = M@S_concrete { fields }
Γ ⊢ e : M@S_hole
----------------------------------------
Generate constraint: S_hole = S_concrete

[Constraint-Procedure-Call]
procedure @S₁ -> @S₂ p declared in M
e: M@S_arg
result = e::p(...)
result: M@S_result
----------------------------------------
Generate constraints: S_arg = S₁, S_result = S₂
```

### 10.9.4 Constraint Solving

**Algorithm:** State constraint solving via unification.

**Input:** Set of constraints `C = {c₁, c₂, ..., cₙ}`

**Output:** Substitution `σ: StateVar → State` or FAILURE

**Procedure:**

1. **Initialize:** `σ = ∅` (empty substitution)

2. **For each constraint `c` in `C`:**
   - If `c = (S₁ = S₂)`:
     - If both concrete: check `S₁ = S₂`, fail if not
     - If one is hole: add to substitution `σ[hole ↦ concrete]`
     - If both holes: record equivalence class

3. **Propagate substitutions:** Apply `σ` to all remaining constraints

4. **Check consistency:** No conflicting assignments to same hole

5. **Return:** `σ` or FAILURE if inconsistent

### 10.9.5 Inference Limitations

**Ambiguity errors:**

```cursive
modal Status {
    @Active { code: i32 }
    @Inactive { code: i32 }
}

// ERROR: ambiguous state
let status: Status@_? = if condition {
    Status@Active { code: 1 }
} else {
    Status@Inactive { code: 0 }
}
```

**Solution:** Use explicit annotation or union type.

---

## 10.10 Verification

This section specifies the verification algorithm for ensuring modal type soundness.

### 10.10.1 Algorithm 10.1: Modal Transition Verification

**Purpose:** Verify that a modal type declaration is sound and all transitions are valid.

**Input:**
- Modal type `M` with states `S = {@S₁, @S₂, ..., @Sₙ}`
- Transition procedures `P = {p₁, p₂, ..., pₖ}`
- State coercions `C = {c₁, c₂, ..., cₘ}`

**Output:** `VERIFIED` or `ERROR(details)`

**Preconditions:**
- All states are syntactically well-formed
- All procedures are type-checked

**Postconditions:**
- All states are reachable or warning issued
- All transitions have valid signatures
- Coercion graph is acyclic
- Permission requirements satisfied

**Algorithm:**

```
Algorithm 10.1 (Modal Transition Verification)

Input: M, S, P, C
Output: VERIFIED or ERROR(details)

1. Extract initial states:
   InitialStates = {s ∈ S | ∀p ∈ P. p.target ≠ s}
                 ∪ {s | constructor for M@s exists}

2. Build transition graph G = (V, E):
   V = S
   E = {(s₁, s₂) | ∃p ∈ P. p.source = s₁ ∧ p.target = s₂}

3. Verify reachability via DFS:
   Reachable = DFS(G, InitialStates)
   Unreachable = S \ Reachable

   For each s ∈ Unreachable:
       WARN: "State @s is unreachable from initial states"

4. Verify procedure signatures:
   For each transition procedure p with spec @S₁ -> @S₂:
       a. Check self parameter type:
          IF p.self_type ≠ M@S₁ THEN
              ERROR: "Self parameter must be M@S₁"

       b. Check self permission:
          IF p.self_permission ≠ own THEN
              ERROR: "Transition procedures require 'own' permission"

       c. Check return type:
          IF p.return_type ≠ M@S₂ AND
             p.return_type ≠ Result<M@S₂, E> for some E THEN
              ERROR: "Return type must be M@S₂ or Result<M@S₂, E>"

       d. Verify effects available:
          IF p.uses ≠ ∅ THEN
              Check caller contexts provide required effects

5. Verify state coercions:
   Build coercion graph GC = (S, EC):
       EC = {(s₁, s₂) | coerce @s₁ <: @s₂ ∈ C}

   IF GC has cycle THEN
       ERROR: "Cyclic state coercion detected"

   For each coercion (s₁ <: s₂):
       Check fields(s₂) ⊆ fields(s₁)
       IF NOT subset THEN
           ERROR: "Target state must have subset of source fields"

6. Verify state invariants:
   For each state s with invariant I(s):
       Check I is well-typed boolean expression
       Mark for runtime/static checking

7. Return VERIFIED if no errors

Complexity: O(|S| + |P| + |C|)
```

### 10.10.2 Worked Example

Given modal:

```cursive
modal Connection {
    @Disconnected { url: string }
    @Connected { url: string, socket: Socket }
    @Failed { url: string, error: Error }

    procedure @Disconnected -> @Connected
        connect(self: own Connection@Disconnected): Result<Connection@Connected, Connection@Failed>
    { }
}
```

**Verification steps:**

1. Initial states: `{@Disconnected}`
2. Transition graph: `@Disconnected → {@Connected, @Failed}`
3. Reachable: All states reachable
4. Procedure checks: All pass
5. No coercions to check
6. Result: `VERIFIED`

### 10.10.3 Compilation Requirements

This section specifies **normative requirements** for compiling modal types to ensure zero-cost abstraction and correct runtime behavior. These requirements are MANDATORY for conforming implementations.

#### State Erasure Requirement

**[COMPILE-REQ-1: State Erasure]**

Implementations MUST erase all state annotations after type checking:

```
∀ modal types M, states @S:
    runtime_representation(M@S) = runtime_representation(M)
    sizeof(M@S) = sizeof(fields(@S))
    No runtime state discriminant/tag is stored
```

State annotations are compile-time information only. Runtime representation contains ONLY the data fields declared in the state, with no metadata overhead.

**Verification:**

```
Given modal M with @S { f₁: T₁, ..., fₙ: Tₙ }:

    size_of(M@S) = Σᵢ size_of(Tᵢ) + alignment_padding

    NOT: size_of(M@S) = size_of(state_tag) + Σᵢ size_of(Tᵢ)
```

Any implementation that includes state tags or discriminants violates this requirement.

#### Zero-Cost Procedure Resolution

**[COMPILE-REQ-2: Static Dispatch]**

Implementations MUST resolve all transition procedure calls statically at compile time:

```
∀ procedure calls recv::p(args) where recv : M@S:
    Compile-time: Resolve p using @S and signature
    Runtime: Direct function call (NO vtable, NO dynamic dispatch)
```

**Requirement details:**

1. **No virtual dispatch**: Procedure calls MUST NOT use vtables, function pointers, or any indirect call mechanism
2. **Monomorphization**: Generic modal procedures MUST be monomorphized to concrete implementations
3. **Inlining eligible**: All procedure calls MUST be eligible for inlining if the optimizer chooses

**Example:**

```cursive
modal File {
    @Open { fd: i32 }

    procedure @Open -> @Open
        read(self: imm File@Open, buf: mut []u8): usize {
        result syscall_read(self.fd, buf);
    }
}

let f: File@Open = /* ... */;
let n = f::read(buffer);
```

The call `f::read(buffer)` MUST compile to:
```
// Direct static call (assembly pseudocode)
mov rdi, [f.fd]      ; Pass fd
mov rsi, buffer      ; Pass buffer
call file_open_read  ; Direct call to static function
```

NOT to:
```
// Illegal: dynamic dispatch
mov rdi, [f.vtable]  ; ❌ No vtable allowed
call [rdi + offset]  ; ❌ No indirect call
```

#### Monomorphization Requirement

**[COMPILE-REQ-3: Monomorphization]**

Generic modal types MUST be monomorphized to concrete types during compilation:

```
∀ generic modal M<T>, concrete type U:
    M<U> is a distinct compiled type
    All M<U> procedures are instantiated with T = U
    No runtime type parameters remain
```

**Example:**

```cursive
// NOTE: Using OptionExample for demonstration (stdlib Option is an enum)
modal OptionExample<T> {
    @Some { value: T }
    @None

    procedure @Some -> T
        unwrap(self: own OptionExample<T>@Some): T {
        result self.value;
    }
}

// Usage
let x: OptionExample<i32>@Some = /* ... */;
let y: OptionExample<string>@Some = /* ... */;
```

The compiler MUST generate:
- `OptionExample<i32>@Some` with `unwrap` returning `i32`
- `OptionExample<string>@Some` with `unwrap` returning `string`

Each instantiation is a separate compile-time type with no shared runtime representation.

#### Transition Verification Requirement

**[COMPILE-REQ-4: Compile-Time Verification]**

Implementations MUST verify all transition paths at compile time:

```
∀ expressions e : M@S₁:
    ∀ procedure calls e::p(...) : M@S₂:
        Verify @S₁ -> @S₂ transition exists at COMPILE TIME
        Reject program if transition not declared
```

**Error conditions:**

1. **Undeclared transition**: Calling a procedure `@S₁ -> @S₂` when no such procedure exists
2. **Permission violation**: Calling transition with insufficient permission
3. **Effect mismatch**: Calling procedure with effects not available in context

**Example errors:**

```cursive
modal Connection {
    @Connected
    @Closed

    procedure @Connected -> @Closed
        close(self: own Connection@Connected): Connection@Closed { }
}

let conn: Connection@Closed = /* ... */;
conn::close();  // ❌ ERROR: No transition @Closed -> @Closed
```

The compiler MUST reject this program with:
```
ERROR: no procedure 'close' for state @Closed
  note: 'close' requires @Connected state
  help: ensure value is in @Connected state before calling close()
```

#### Layout Optimization Requirement

**[COMPILE-REQ-5: Field Layout Optimization]**

Implementations MAY optimize field layout within each state, but MUST maintain:

1. **Consistent layout per state**: All values `M@S` have identical field layout
2. **Alignment requirements**: All fields meet their alignment requirements
3. **No padding between state variants**: Different states MAY have different layouts

**Permitted optimization:**

```cursive
modal Data {
    @StateA { x: u8, y: u64, z: u8 }
    @StateB { a: u64, b: u32 }
}
```

Compiler MAY reorder fields within `@StateA`:
```
@StateA layout: [y: u64 (offset 0), x: u8 (offset 8), z: u8 (offset 9)]
    Reduces size from 24 bytes (with padding) to 16 bytes
```

But MUST NOT share layout between `@StateA` and `@StateB`.

#### No Runtime Type Information Requirement

**[COMPILE-REQ-6: No RTTI for States]**

Implementations MUST NOT generate runtime type information (RTTI) for modal states:

```
∀ modal values v : M@S:
    No runtime function state_of(v) -> StateName
    No runtime type_id(v) comparison
    No reflection on state annotations
```

State information is entirely compile-time, so exposing reflection would violate the zero-cost abstraction requirement.

**Exception:** Union types `M@S₁ | M@S₂` MAY include a discriminant tag for pattern matching (see §10.2.8).

#### Summary of Compilation Requirements

| Requirement | Description | Cost |
|-------------|-------------|------|
| State Erasure | No state tags in runtime representation | Zero |
| Static Dispatch | No vtables or indirect calls | Zero |
| Monomorphization | Instantiate generics at compile time | Zero runtime |
| Compile-Time Verification | Check all transitions statically | Zero runtime |
| Layout Optimization | Optimize field ordering per state | Zero or negative (smaller size) |
| No RTTI | No runtime state reflection | Zero |

All requirements ensure that modal types have **zero runtime overhead** compared to hand-written state machines.

### 10.10.4 Procedure Resolution Algorithm

This section specifies the **compile-time algorithm** for resolving procedure calls on modal types. Procedure resolution determines which procedure to invoke based on the receiver's state and the call's signature.

#### Algorithm 10.2: Procedure Resolution

**Purpose:** Resolve a procedure call `recv::proc_name(args)` to a specific procedure declaration.

**Input:**
- Receiver expression `recv` with static type `M@S`
- Procedure name `proc_name : Ident`
- Argument expressions `args : [Expr]`
- Type context `Γ`

**Output:**
- Resolved procedure declaration `p : ProcedureDecl`
- OR compilation error

**Algorithm:**

```
Algorithm 10.2 (Procedure Resolution)

Input: recv : M@S, proc_name, args, Γ
Output: Resolved procedure p or ERROR

1. Determine receiver state:
   State = static_state(recv)  // Extract @S from recv : M@S

   IF State is unknown/polymorphic THEN
       ERROR: "Cannot resolve procedure on polymorphic state"

2. Find candidate procedures:
   Candidates = {p ∈ procedures(M) | p.name = proc_name}

   IF Candidates = ∅ THEN
       ERROR: "No procedure named '{proc_name}' in modal {M}"

3. Filter by source state:
   StateCandidates = {p ∈ Candidates | p.source_state = State}

   IF StateCandidates = ∅ THEN
       ERROR: "No procedure '{proc_name}' for state @{State}"
       HINT: List states that DO have '{proc_name}'

4. Check argument count:
   ArgCount = |args|
   MatchingArity = {p ∈ StateCandidates | |p.parameters| - 1 = ArgCount}
       // -1 because self is implicit

   IF MatchingArity = ∅ THEN
       ERROR: "Procedure '{proc_name}' expects {expected} arguments, got {ArgCount}"

5. Type-check arguments:
   FOR each p ∈ MatchingArity:
       valid = true
       FOR i IN 0..|args|:
           param_type = p.parameters[i+1].type  // Skip self parameter
           arg_type = infer_type(args[i], Γ)

           IF NOT (arg_type <: param_type) THEN
               valid = false
               BREAK

       IF valid THEN
           RETURN p  // Found matching procedure

6. No match after type checking:
   ERROR: "No matching procedure for '{proc_name}' with argument types {arg_types}"
   HINT: Show expected signatures

Complexity: O(|procedures(M)| × |args|)
```

#### Resolution Examples

**Example 1: Successful resolution**

```cursive
modal File {
    @Closed { path: string }
    @Open { path: string, fd: i32 }

    procedure @Closed -> @Open
        open(self: own File@Closed): Result<File@Open, IoError> { }

    procedure @Open -> @Open
        read(self: imm File@Open, buf: mut []u8): usize { }
}

let f: File@Closed = /* ... */;
let result = f::open();  // Resolve to procedure @Closed -> @Open open
```

**Resolution trace:**
1. Receiver state: `@Closed`
2. Candidates: `{open, read}`
3. State filter: `{open}` (only `open` has source `@Closed`)
4. Arity check: `open` expects 0 args (excluding self), call has 0 args ✓
5. Type check: No args to check ✓
6. **Resolved:** `procedure @Closed -> @Open open`

**Example 2: State mismatch error**

```cursive
let f: File@Open = /* ... */;
let result = f::open();  // ❌ ERROR
```

**Resolution trace:**
1. Receiver state: `@Open`
2. Candidates: `{open, read}`
3. State filter: `{}` (no procedure with source `@Open` named `open`)
4. **ERROR:** "No procedure 'open' for state @Open"
   **HINT:** "Procedure 'open' is available for states: @Closed"

**Example 3: Argument type mismatch**

```cursive
let f: File@Open = /* ... */;
let x: i32 = 42;
let n = f::read(x);  // ❌ ERROR
```

**Resolution trace:**
1. Receiver state: `@Open`
2. Candidates: `{read}`
3. State filter: `{read}`
4. Arity check: `read` expects 1 arg, call has 1 arg ✓
5. Type check:
   - Expected: `mut []u8`
   - Got: `i32`
   - `i32 <: mut []u8`? NO ✗
6. **ERROR:** "No matching procedure 'read' with argument types (i32)"
   **HINT:** "Expected signature: read(self: imm File@Open, buf: mut []u8): usize"

#### Ambiguity Resolution

If multiple procedures match after type checking, the compiler MUST apply the following **tie-breaking rules**:

**Rule 1: Exact match preferred**
```
Prefer p₁ over p₂ if:
    ∀i. arg_type[i] = p₁.param_type[i] AND
       arg_type[i] <: p₂.param_type[i] (but not equal)
```

**Rule 2: No ambiguity allowed**
```
IF multiple procedures remain after Rule 1 THEN
    ERROR: "Ambiguous procedure call"
    HINT: Show all matching signatures
    SUGGEST: Add explicit type annotations
```

**Example ambiguity:**

```cursive
modal Parser {
    @Ready

    procedure @Ready -> @Ready
        parse(self: imm Parser@Ready, input: string): Result { }

    procedure @Ready -> @Ready
        parse(self: imm Parser@Ready, input: []u8): Result { }
}

let p: Parser@Ready = /* ... */;
let r = p::parse("data");  // Exact match: string overload

let bytes: []u8 = /* ... */;
let r2 = p::parse(bytes);  // Exact match: []u8 overload

let data = get_data();  // Type unknown
let r3 = p::parse(data);  // ❌ ERROR: Ambiguous
```

#### Integration with Type Inference

Procedure resolution integrates with type inference:

```
[Procedure-Resolution-Typing]
recv : M@S
resolve_procedure(M, @S, proc_name, args) = p
p.signature = @S₁ -> @S₂ (self: perm M@S₁, params): R
----------------------------------------
Γ ⊢ recv::proc_name(args) : R
```

The inferred type of the call expression is the return type `R` of the resolved procedure.

**For transition procedures:**
- If `R = M@S₂`, the call produces a value in state `@S₂`
- If `R = Result<M@S₂, E>`, the call produces a result that must be handled

**For non-transition procedures:**
- `R` can be any type (not necessarily a modal state)

This resolution algorithm ensures that all procedure calls are **resolved statically** at compile time, enabling zero-cost dispatch and comprehensive error checking.

---

## 10.11 Soundness Theorems

This section states and proves (via sketches) the key soundness properties of modal types.

### 10.11.1 Theorem 10.1 (State Safety)

**Theorem 10.1 (State Safety):** Well-typed programs never access fields not present in the current state.

**Formal statement:**

```
If Γ ⊢ e : M@S and e ⇓ v, then v has exactly the fields declared in state S.
```

**Proof sketch:**

1. By type preservation during evaluation
2. Construction of `M@S` requires providing all fields of state `S`
3. Field access `e.f` is only well-typed if `f ∈ fields(S)`
4. Transitions produce values with exactly the target state's fields
5. Therefore, no field access can retrieve non-existent fields

**Corollary:** Memory safety for modal field access.

### 10.11.2 Theorem 10.2 (Transition Safety)

**Theorem 10.2 (Transition Safety):** All state transitions respect the declared transition graph.

**Formal statement:**

```
If Γ ⊢ e::p(args) : M@S₂ where e : M@S₁,
then there exists a declared transition @S₁ -> @S₂ in M.
```

**Proof sketch:**

1. By typing rule [T-Transition-Call]
2. The rule requires procedure `p` with `TransitionSpec @S₁ -> @S₂`
3. Such procedures must be declared in modal body
4. Therefore, transition (@S₁, @S₂) exists in transition graph
5. No transitions possible outside declared graph

**Corollary:** Protocol adherence is enforced statically.

### 10.11.3 Theorem 10.3 (Linear Consumption)

**Theorem 10.3 (Linear Consumption):** Transition procedures consume their source value exactly once.

**Formal statement:**

```
If Γ ⊢ x::p(args) : M@S₂ where x : M@S₁ and p is a transition procedure,
then x is invalidated in Γ' (the context after the call).
```

**Proof sketch:**

1. Transition procedures require `own` permission
2. By permission semantics, `own` parameters consume their argument
3. Consumption removes binding from typing context: `Γ' = Γ \ {x}`
4. Any subsequent use of `x` is ill-typed (E4006 error)
5. Therefore, source value consumed exactly once

**Corollary:** No use-after-transition errors.

### 10.11.4 Theorem 10.4 (Zero-Cost Abstraction)

**Theorem 10.4 (Zero-Cost Abstraction):** State annotations have zero runtime overhead.

**Formal statement:**

```
The runtime representation of M@S₁ and M@S₂ are identical if fields(S₁) = fields(S₂).
```

**Proof sketch:**

1. State annotations are erased during compilation
2. Runtime representation contains only data fields
3. Two states with identical fields have identical memory layouts
4. State checking occurs entirely at compile time
5. No runtime tags, discriminants, or metadata required

**Corollary:** Modal types incur no performance cost.

### 10.11.5 Theorem 10.5 (Progress for Non-Terminal States)

**Theorem 10.5 (Progress):** Non-terminal states always have available transitions.

**Formal statement:**

```
If S is not a terminal state in modal M,
then there exists a procedure p with source state S.
```

**Proof sketch:**

1. By definition, non-terminal states have outgoing edges
2. Edges correspond to transition procedures
3. Therefore, at least one procedure exists with source state S
4. Well-typed programs in non-terminal states can progress

**Corollary:** No unintentional deadlocks in modal protocols.

---

## 10.12 Diagnostics

This section specifies error messages and diagnostic codes for modal type errors.

### 10.12.1 Error Code Table

Modal-specific errors use existing error codes from the Cursive diagnostic system:

| Error Code | Category | Description |
|------------|----------|-------------|
| E4001 | Type Error | Invalid state reference, duplicate state, state type mismatch |
| E4003 | Permission Error | Insufficient permission for transition (not `own`) |
| E4005 | Pattern Error | Non-exhaustive pattern match on modal states |
| E4006 | Move Error | Use of value after consumption by transition |
| E6004 | Transition Error | Invalid transition (not declared in modal) |
| W10-001 | Warning | Unreachable state |
| W10-002 | Warning | State never constructed |

### 10.12.2 Detailed Diagnostic Examples

**Example 1: Use After Transition (E4006)**

```cursive
modal File {
    @Closed { path: string }
    @Open { path: string, handle: FileHandle }

    procedure @Closed -> @Open
        open(self: own File@Closed): Result<File@Open, Error> { }
}

procedure example() {
    let file = File@Closed { path: "/data.txt" }
    let opened = file::open()

    println(file.path)  // ERROR
}
```

**Diagnostic:**

```
ERROR E4006: use of moved value 'file'
  --> example.cursive:12:13
   |
10 |     let file = File@Closed { path: "/data.txt" }
   |         ---- move occurs because 'file' has type 'File@Closed'
11 |     let opened = file::open()
   |                  ---- value moved here due to state transition
12 |     println(file.path)
   |             ^^^^^^^^^ value used here after move
   |
note: 'file' was consumed by transition to File@Open
note: transition procedures require 'own' permission and consume the source value
help: use the returned value instead
   |
12 |     println(opened.unwrap().path)
   |             ~~~~~~~~~~~~~~~~~~~~
```

**Example 2: Invalid Transition (E6004)**

```cursive
modal State {
    @A { x: i32 }
    @B { y: i32 }

    procedure @A -> @B
        go(self: own State@A): State@B { }
}

procedure example(s: State@B) {
    let result = s::go()  // ERROR: no transition from @B
}
```

**Diagnostic:**

```
ERROR E6004: no transition procedure 'go' for state '@B'
  --> example.cursive:10:18
   |
10 |     let result = s::go()
   |                  ^ no transition from '@B' via 'go'
   |
note: procedure 'go' transitions from '@A' to '@B'
note: 's' has type 'State@B'
help: ensure 's' is in state '@A' before calling 'go'
```

**Example 3: Non-Exhaustive Pattern Match (E4005)**

```cursive
modal Traffic {
    @Red
    @Yellow
    @Green
}

procedure can_go(light: Traffic@Red | Traffic@Yellow | Traffic@Green): bool {
    match light {
        Traffic@Green => result true,
        Traffic@Red => result false
        // Missing: @Yellow
    }
}
```

**Diagnostic:**

```
ERROR E4005: non-exhaustive pattern match on modal states
  --> example.cursive:8:5
   |
 8 |     match light {
   |     ^^^^^^^^^^^^ pattern does not cover all possible states
   |
note: missing state '@Yellow' in match arms
help: add an arm for '@Yellow'
   |
 8 |     match light {
 9 |         Traffic@Green => result true,
10 |         Traffic@Red => result false,
   |         Traffic@Yellow => result false  // Add this arm
   |
help: or use a wildcard pattern to handle remaining states
   |
11 |         _ => result false
```

**Example 4: Permission Violation (E4003)**

```cursive
modal File {
    @Closed { path: string }
    @Open { path: string, handle: FileHandle }

    procedure @Closed -> @Open
        open(self: mut File@Closed): File@Open { }  // ERROR: should be 'own'
}
```

**Diagnostic:**

```
ERROR E4003: transition procedure requires 'own' permission on self
  --> example.cursive:6:14
   |
 6 |         open(self: mut File@Closed): File@Open { }
   |              ^^^^^^^^^^^^^^^^^^^^^ must be 'own File@Closed'
   |
note: transition procedures consume the source state value
note: 'mut' permission allows aliasing, which would violate state safety
help: change permission to 'own'
   |
 6 |         open(self: own File@Closed): File@Open { }
   |                    ~~~
```

**Example 5: State Type Mismatch (E4001)**

```cursive
modal Status {
    @Active { count: i32 }
    @Inactive { count: i32 }
}

procedure example() {
    let active: Status@Active = Status@Inactive { count: 0 }  // ERROR
}
```

**Diagnostic:**

```
ERROR E4001: state type mismatch
  --> example.cursive:7:34
   |
 7 |     let active: Status@Active = Status@Inactive { count: 0 }
   |                 -------------   ^^^^^^^^^^^^^^^^^^^^^^^^^^^^
   |                 |               expected '@Active', found '@Inactive'
   |                 expected due to this type annotation
   |
help: change the state annotation to match
   |
 7 |     let active: Status@Inactive = Status@Inactive { count: 0 }
   |                        ~~~~~~~~
```

### 10.12.3 Formal Error Conditions

This section provides **formal specifications** for each error condition related to modal types. Each error is specified using inference rules that define precisely when the error MUST be raised.

#### E4001: Type Errors

**[Error-Invalid-State]**

Invalid state reference when the state is not declared in the modal type:

```
Γ ⊢ M : ModalType
@S ∉ states(M)
----------------------------------------
ERROR E4001: invalid state '@S' for modal type 'M'
```

**Example:**
```cursive
modal File {
    @Open
    @Closed
}

let f: File@Invalid;  // ERROR E4001: '@Invalid' not a state of 'File'
```

**[Error-Duplicate-State]**

Duplicate state names within a modal declaration:

```
modal M declares states S = {@S₁, ..., @Sᵢ, ..., @Sⱼ, ..., @Sₙ}
i ≠ j
@Sᵢ = @Sⱼ  (same name)
----------------------------------------
ERROR E4001: duplicate state name '@Sᵢ' in modal 'M'
```

**Example:**
```cursive
modal Bad {
    @Active { x: i32 }
    @Active { y: string }  // ERROR E4001: duplicate state 'Active'
}
```

**[Error-State-Type-Mismatch]**

State type mismatch in assignments or initializations:

```
Γ ⊢ e : M@S₁
expected type: M@S₂
@S₁ ≠ @S₂
----------------------------------------
ERROR E4001: state type mismatch
    expected 'M@S₂', found 'M@S₁'
```

**Example:**
```cursive
let x: Connection@Connected = Connection@Disconnected { };
// ERROR E4001: expected '@Connected', found '@Disconnected'
```

#### E4003: Permission Errors

**[Error-Transition-Permission]**

Transition procedure declared with insufficient permission:

```
modal M declares procedure @S₁ -> @S₂ p
p has parameter self: Permission M@S₁
Permission ≠ own
----------------------------------------
ERROR E4003: transition procedure requires 'own' permission on self
    found: 'Permission'
```

**Example:**
```cursive
modal File {
    @Closed
    @Open

    procedure @Closed -> @Open
        open(self: mut File@Closed): File@Open { }
    // ERROR E4003: must be 'own', found 'mut'
}
```

**[Error-Insufficient-Permission-Call]**

Calling a procedure with insufficient permission:

```
Γ ⊢ recv : M@S
procedure p requires permission P
recv has permission Q
Q ⊈ P  (Q does not provide P)
----------------------------------------
ERROR E4003: insufficient permission for call to 'p'
    required: 'P'
    available: 'Q'
```

#### E4005: Pattern Matching Errors

**[Error-Non-Exhaustive-Match]**

Pattern match on modal union type does not cover all states:

```
Γ ⊢ e : M@S₁ | M@S₂ | ... | M@Sₙ
match e { arms }
covered_states(arms) ⊂ {S₁, S₂, ..., Sₙ}  (proper subset)
----------------------------------------
ERROR E4005: non-exhaustive pattern match on modal states
    missing state(s): {S₁, S₂, ..., Sₙ} \ covered_states(arms)
```

**Example:**
```cursive
modal Status {
    @Active
    @Inactive
    @Paused
}

fn handle(s: Status@Active | Status@Inactive | Status@Paused) {
    match s {
        Status@Active => { },
        Status@Inactive => { }
        // ERROR E4005: missing '@Paused'
    }
}
```

#### E4006: Move Errors

**[Error-Use-After-Move]**

Using a value after it has been consumed by a transition:

```
Γ ⊢ e₁ : M@S₁
e₁::p(...) transitions e₁ to e₂ : M@S₂  (consumes e₁)
e₃ uses e₁ after transition
----------------------------------------
ERROR E4006: use of moved value 'e₁'
    note: value moved here: e₁::p(...)
    note: value used here after move: e₃
```

**Example:**
```cursive
let f = File@Closed { path: "/tmp/data" };
let g = f::open();  // f consumed here
println(f.path);    // ERROR E4006: use of moved value 'f'
```

**[Error-Move-While-Borrowed]**

Attempting to transition a value while it is borrowed:

```
Γ ⊢ e : M@S₁
borrow b of e is active (lifetime still valid)
e::p(...) attempts transition requiring own permission
----------------------------------------
ERROR E4006: cannot move out of borrowed value 'e'
    note: value borrowed here
    note: transition requires ownership
```

#### E6004: Transition Errors

**[Error-Undefined-Transition]**

Calling a transition that is not declared:

```
Γ ⊢ recv : M@S₁
recv::p(...) invokes procedure p
@S₁ -> @S₂ transition via p NOT declared in M
----------------------------------------
ERROR E6004: no transition procedure 'p' for state '@S₁'
    note: available procedures for '@S₁': [list]
```

**Example:**
```cursive
modal Connection {
    @Connected
    @Disconnected

    procedure @Connected -> @Disconnected
        disconnect(self: own Connection@Connected): Connection@Disconnected { }
}

let conn: Connection@Disconnected = /* ... */;
conn::disconnect();
// ERROR E6004: no procedure 'disconnect' for state '@Disconnected'
```

**[Error-Return-Type-Mismatch]**

Transition procedure body returns wrong state type:

```
modal M declares procedure @S₁ -> @S₂ p(...): M@S₂ { body }
Γ ⊢ body : M@S₃
@S₃ ≠ @S₂
----------------------------------------
ERROR E6004: transition procedure must return target state
    expected: 'M@S₂'
    found: 'M@S₃'
```

**Example:**
```cursive
modal State {
    @A
    @B
    @C

    procedure @A -> @B
        go(self: own State@A): State@B {
        result State@C { }
        // ERROR E6004: expected 'State@B', found 'State@C'
    }
}
```

#### W10-001: Warning - Unreachable State

**[Warning-Unreachable-State]**

State cannot be reached from any initial state:

```
modal M declares state @S
initial_states(M) = I
∀@S_init ∈ I. @S_init ⇝̸* @S  (S not reachable from any initial state)
----------------------------------------
WARNING W10-001: state '@S' is unreachable from initial states
    note: no transition path leads to this state
    help: consider adding a transition to '@S' or removing it
```

**Example:**
```cursive
modal Process {
    @NotStarted
    @Running
    @Zombie  // No transition leads here

    procedure @NotStarted -> @Running
        start(self: own Process@NotStarted): Process@Running { }
}
// WARNING W10-001: state '@Zombie' is unreachable
```

#### W10-002: Warning - State Never Constructed

**[Warning-Unconstructed-State]**

State is never directly constructed (no constructor exists):

```
modal M declares state @S
@S ∈ initial_states(M)  (@S has no incoming transitions)
no constructor function produces M@S
----------------------------------------
WARNING W10-002: state '@S' is never constructed
    note: no constructor or function produces 'M@S'
    help: add a constructor for this state
```

**Example:**
```cursive
modal Status {
    @Active  // No constructor!
    @Inactive

    function create_inactive(): Status@Inactive {
        result Status@Inactive { }
    }
}
// WARNING W10-002: state '@Active' is never constructed
```

#### Error Condition Summary

The following table summarizes when each error MUST be raised:

| Error Code | Condition | Action |
|------------|-----------|--------|
| E4001 | Invalid state reference, duplicate state, or state type mismatch | MUST reject program |
| E4003 | Insufficient permission for transition or procedure call | MUST reject program |
| E4005 | Non-exhaustive pattern match on modal states | MUST reject program |
| E4006 | Use of moved value or move while borrowed | MUST reject program |
| E6004 | Invalid transition (undeclared) or return type mismatch | MUST reject program |
| W10-001 | Unreachable state detected | SHOULD warn user |
| W10-002 | State never constructed | SHOULD warn user |

**Normative requirements:**

1. Implementations MUST reject programs that trigger errors E4001, E4003, E4005, E4006, or E6004.
2. Implementations SHOULD issue warnings W10-001 and W10-002 but MAY allow compilation to proceed.
3. All error messages MUST include location information (file, line, column).
4. Error messages SHOULD include contextual notes and actionable help text.

---

## 10.13 Grammar Summary

This section records the grammar deltas introduced in Part X. Appendix A remains authoritative and MUST incorporate the productions below without modification.

```ebnf
ModalDecl       ::= Attribute* Visibility? "modal" Ident GenericParams? ModalBody
ModalBody       ::= "{" ModalState+ StateCoercion* ProcedureDecl* "}"
ModalState      ::= "@" Ident RecordBody WhereClause?
StateCoercion   ::= "coerce" "@" Ident "<:" "@" Ident CoercionConstraint?
CoercionConstraint ::= "{"
                       ("cost" ":" Cost)?
                       ("requires" ":" Permission)?
                       ("uses" ":" EffectSet)?
                       "}"
Cost            ::= "O(1)"
Permission      ::= "own" | "mut" | "imm"
EffectSet       ::= Ident ("." Ident)* | "∅"
TransitionSpec  ::= "@" Ident "->" "@" Ident
ModalStateType  ::= Ident "@" Ident
ModalLiteral    ::= Ident "@" Ident "{" FieldInit* "}"
ModalStatePattern ::= Ident "@" Ident ("{" FieldPattern* "}")?
```

Any additional non-terminals referenced above are defined in Appendix A. No other grammar changes are introduced by this chapter.

---

## 10.14 Summary

See preceding sections (§10.1-10.13) for detailed examples and diagnostics covering modal declarations, state transitions, pattern matching, permissions integration, effects integration, contracts integration, and verification.


---
