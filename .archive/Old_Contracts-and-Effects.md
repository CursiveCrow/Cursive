# The Cursive Language Specification

**Part**: VII - Contracts and Effects  
**File**: 07_Contracts-and-Effects.md  
**Previous**: [Statements and Control Flow](06_Statements-and-Control-Flow.md) | **Next**: [Holes and Inference](08_Holes-and-Inference.md)

## Abstract

Part VII specifies Cursive’s behavioral contract system: contract declarations, effect clauses (`uses`), preconditions (`must`), postconditions (`will`), and type invariants (`where`). It defines the grammar that extends Appendix A, the static and dynamic semantics of each clause, contract composition rules, and the diagnostics that enforce them.

Notation: Function types in this part use arrow syntax `(T₁, …, Tₙ) → U ! ε`.

---

## 7.0 Contracts and Effects Foundations

### Definition 7.1 (Contracts and Effects Foundations)

This part is the authoritative specification for behavioral contracts, effect clauses, preconditions, postconditions, and type-level invariants in Cursive. It defines the syntax, static semantics, dynamic semantics, and diagnostic catalog for contract-driven programming. Wherever other parts of the specification refer to contracts or effects, they rely on the rules in this chapter.

### 7.0.1 Scope

This chapter provides the complete normative description of:

* behavioral contracts declared with the `contract` keyword;
* `uses` (effect clauses) on procedures and functions;
* `must` (precondition clauses);
* `will` (postcondition clauses);
* `where` (type constraint clauses on records and other nominal types);
* composition rules that govern how contracts interact across calls; and
* the diagnostics that signal contract or effect violations.

### 7.0.2 Cross-Part Dependencies

Other chapters remain authoritative for the topics they introduce:

* **Part II** — Types, modal state machines, trait coherence, and effect-polymorphic map types;
* **Part III** — Declaration forms (`let`, `var`, shadowing) and scope rules;
* **Part V** — Expression formation, closure capture rules, effect inference, and value categories;
* **Part VI** — Block contracts, effect narrowing, loop verification, and `[[no_panic]]`; and
* **Part I (Foundations)** — Attributes, diagnostic formatting, judgment notation, statement separators, and grammar conventions.

Note: Part V (Expressions and Operators) retains the §4.x internal numbering used in earlier editions; all references in this chapter follow that numbering. CITE: Part V §4.0 — Expression Foundations.

CITE: Part II §2.7 — Traits and Dispatch; Part III §3.2 — Declarations and Scope; Part V §4.19 — Effect Composition; Part VI §6.13 — Block Contracts and Verification Modes; Part I §2.7 — Statement Separators.

### 7.0.3 Design Principles

Contracts and effects reinforce Cursive’s core philosophy:

* **Explicit over implicit** — Call-site obligations (`uses`, `must`, `will`) are spelled out in signatures.
* **Local reasoning** — Implementations verify clauses using only their own bodies and declared contracts.
* **Zero-cost abstraction** — Contracts add no runtime dispatch unless coupled with explicit dynamic mechanisms (§7.2.11).

### 7.0.4 Notational Conventions

The chapter relies on the standard judgment forms and metavariables defined in Part I. In particular:

* `Γ ⊢` judgments follow the typing/effect conventions of Part V;
* effect sets (`ε`) are finite sets closed under union/subset inclusion;
* predicates reuse the assertion grammar from Appendix A.8; and
* inference rules follow the format established in Part I §1.6.

### 7.0.5 Organization

The remainder of this chapter is organized as follows:

1. Overview (`§7.1`)
2. Behavioral contracts (`§7.2`)
3. Effect clauses (`§7.3`)
4. Precondition clauses (`§7.4`)
5. Postcondition clauses (`§7.5`)
6. Type constraint clauses (`§7.6`)
7. Clause composition (`§7.7`)
8. Diagnostics (`§7.8`)

Each section presents concrete grammar, abstract syntax, static semantics, dynamic semantics (where applicable), and illustrative examples.

---

## 7.1 Overview

This section provides a high-level introduction to Cursive's contract system before diving into the detailed specifications of each component.

### 7.1.1 Contract System Components

Cursive's contract system consists of five primary components that work together to enable design-by-contract programming:

1. **Behavioral contracts** (`contract` keyword) — Abstract interfaces specifying procedure signatures with clauses
2. **Effect clauses** (`uses`) — Declarations of computational side effects
3. **Precondition clauses** (`must`) — Requirements that MUST hold at procedure entry
4. **Postcondition clauses** (`will`) — Guarantees that MUST hold at procedure exit
5. **Type constraint clauses** (`where`) — Invariants that MUST hold for all instances of a type

### 7.1.2 Design Rationale

The contract system enforces three key principles:

* **Explicit obligations** — All behavioral requirements are visible in signatures, enabling local reasoning without examining implementations
* **Static verification** — Contracts are checked at compile time when possible, with optional runtime checks for dynamic conditions
* **Zero-cost abstraction** — Contract checking adds no runtime overhead when statically verified; runtime checks can be disabled in production builds

### 7.1.3 Relationship to Other Language Features

Contracts integrate with other Cursive features:

* **Permissions (Part IV)** — Contract predicates may reference permissions but MUST NOT violate permission invariants
* **Effects (this chapter)** — Effect clauses compose with the effect system to track side effects through call chains
* **Regions (Part VIII)** — Contracts may reference region-allocated data subject to escape analysis
* **Modals (Part X)** — Contracts may constrain modal state transitions and state-dependent operations

The following sections specify each component in detail.

---

## 7.2 Behavioral Contracts

Behavioral contracts define abstract interfaces consisting purely of procedure signatures annotated with clauses (`uses`, `must`, `will`). Unlike traits (Part II §2.7), contracts never provide implementations; they establish obligations that concrete types MUST satisfy.

### 7.2.1 Motivation

Contracts specify abstract behavioral interfaces (no bodies); traits provide concrete code reuse (with bodies). Types may implement both.

### 7.2.2 Grammar

The productions for contracts are added to Appendix A.6 (Declaration Grammar), Appendix A.7 (Contract Grammar), Appendix A.8 (Assertion Grammar), and Appendix A.9 (Effect Grammar) and repeated here for convenience:

```ebnf
ContractDecl   ::= Attribute* Visibility? "contract" Ident GenericParams?
                    ExtendsClause? "{" ContractItem* "}"
ExtendsClause  ::= "extends" Ident ("," Ident)*
ContractItem   ::= ProcedureSignature | AssociatedTypeDecl
ProcedureSignature
                ::= "procedure" Ident "(" ParamList? ")"
                    (":" Type)? WhereClause? ContractClause*
AssociatedTypeDecl
                ::= "type" Ident TypeBound?
```

`Attribute`, `Visibility`, `GenericParams`, `ParamList`, `TypeBound`, `WhereClause`, and `ContractClause` are defined in Appendix A.6 and Appendix A.7. Generic parameter lists MAY include grant parameters (e.g., `ε`) in addition to type and const parameters (see the `GrantParam` production in Appendix A.6), as detailed in Part II §2.9.4. The contract clause (when present) MUST be a single `sequent` block containing the behavioral specification; multiple `sequent` clauses are rejected with E7C18. Contract items follow the standard statement separator rules (newline or semicolon). CITE: Part I §2.7 — Statement Separators.

### 7.2.3 Static Semantics

#### Contract well-formedness

```
[Contract-Item-WF]
Γ, GenericParams ⊢ Signature well-formed
Signature MUST have no body
──────────────────────────────────
Γ ⊢ ContractItem(Signature) well-formed

[Contract-WF]
∀i. Γ ⊢ ContractItemᵢ well-formed
Identifiers(Signatureᵢ) are pairwise distinct
────────────────────────────────────────────
Γ ⊢ contract C { ContractItem₁; …; ContractItemₙ; } ok
```

A contract item is well-formed when its parameters, result type, and sequent specification satisfy the rules in §7.3 (Sequent Clauses). Contracts MUST NOT declare fields or provide bodies.

#### Implementing a contract

A record (or other nominal type) implements a contract by listing it after the colon in its declaration:

```cursive
record Circle: Drawable {
    radius: f64

    procedure draw(self: Self)
        sequent { [fs.write] |- true => true }
    {
        println("Circle radius {}", self.radius)
    }

    procedure bounds(self: Self): Rectangle
        sequent { [] |- true => result.width >= 0.0 && result.height >= 0.0 }
    {
        Rectangle {
            x: self.radius * -1.0,
            y: self.radius * -1.0,
            width: self.radius * 2.0,
            height: self.radius * 2.0,
        }
    }
}
```

Static semantics enforce that every contract procedure is implemented exactly once with compatible clauses:

```
[Contract-Impl]
contract C { Signature₁; …; Signatureₙ; }
record T: C { Proc₁; …; Procₙ; Members }
∀i. Signatureᵢ ≡ Procᵢ[Self ↦ T]
────────────────────────────────────────────
Γ ⊢ record T: C well-formed

[Contract-Sequent-Compat]
Signature ≡ procedure p(...) sequent { [ε_c] |- P_c => Q_c }
Procedure ≡ procedure p(...) sequent { [ε_m] |- P_m => Q_m } { body }
Γ ⊢ body : τ
P_c ⇒ P_m        (precondition weakening: impl accepts more inputs)
Q_m ⇒ Q_c        (postcondition strengthening: impl guarantees more)
ε_m ⊆ ε_c        (grant reduction: impl uses fewer capabilities)
────────────────────────────────────────────
Procedure satisfies contract sequent compatibility
```

Omitting sequent components uses the identity element for that component: omitting `[ε]` denotes the empty grant set `ε = ∅`, while omitting antecedent P or consequent Q denotes predicates that are trivially `true`. The compatibility relations above therefore still apply.

##### Generic implementations

Contracts and implementing types MAY both be generic. During checking, the compiler universally quantifies the contract’s `GenericParams` and instantiates them with the type’s parameters before comparing signatures:

```
[Contract-Impl-Generic]
contract C<Ξ_c> { Signature₁; …; Signatureₙ; }
record T<Ξ_t>: C<Φ> { Proc₁; …; Procₙ; Members }
Φ : Ξ_c ← Ξ_t          // argument list respects bounds/constraints
∀i. Signatureᵢ[Ξ_c ↦ Φ] ≡ Procᵢ[Self ↦ T<Ξ_t>]
──────────────────────────────────────────────────────────────
Γ ⊢ record T<Ξ_t>: C well-formed
```

Associated type obligations are checked after substitution: for every `type Item : Bounds` in the contract, the implementation MUST provide `type Item = Concrete` such that `Concrete[Ξ_t]` satisfies `Bounds[Self ↦ T<Ξ_t>]`. Blanket implementations (`record T<A>: C` where constraints involve `A`) are valid provided the coherence rules in §7.2.6 are met.

**Example — Generic iterator:**

```cursive
contract Iterator<T> {
    type Item : Copy
    procedure next(self: mut Self): Option<Self::Item>
}

record SliceIterator<T>: Iterator<T> {
    buffer: [T]
    index: usize
    type Item = T  // T must satisfy Copy bound from contract

    procedure next(self: mut Self): Option<Self::Item> {
        if self.index < self.buffer.len() {
            let item = self.buffer[self.index]
            self.index += 1
            Option::Some(item)
        } else {
            Option::None
        }
    }
}
```

If any compatibility test fails, the implementation is rejected with the diagnostics listed in §7.8 (E7C07–E7C09).
CITE: Appendix A.7 — ProcedureSignature; Part I §8 — Diagnostics Directory.

### 7.2.4 Contract Extension

Contracts MAY extend one or more contracts. The grammar adds an optional `extends` clause:

```ebnf
ContractDecl ::= "contract" Ident GenericParams?
                  ("extends" Ident ("," Ident)*)?
                  "{" ContractItem* "}"
```

Static semantics:

```
[Contract-Ext-WF]
contract C extends B₁, …, Bₖ { Items }
∀j. Γ ⊢ Bⱼ well-formed contract
CombinedSignatures = Signatures(B₁) ∪ … ∪ Signatures(Bₖ) ∪ Signatures(C)
Identifiers(CombinedSignatures) distinct
────────────────────────────────────────────────────────
Γ ⊢ contract C extends B₁, …, Bₖ { Items } well-formed
```

When a type implements an extending contract, it MUST satisfy every inherited signature and clause. Any conflict between inherited signatures is diagnosed at contract declaration time (E7C10).

```cursive
contract Transformable extends Drawable {
    procedure translate(self: mut Self, dx: f64, dy: f64)
    procedure scale(self: mut Self, factor: f64)
        sequent { [] |- factor > 0.0 => true }
}
```

##### Conflict detection in inheritance graphs

The compiler constructs the transitive closure of all parent contracts before validating `contract C`. For every procedure name `p` appearing in the closure, it ensures the signatures are pairwise compatible:

```
[Contract-Ext-Conflict]
contract C extends B₁, …, Bₖ
signatures(Bᵢ, p) = {σ₁, …, σ_m}
∃(σ_a, σ_b). σ_a ≠ σ_b        // after α-renaming and clause normalisation
───────────────────────────────────────────────────────
ERROR E7C10 (incompatible inherited contract member)
```

Compatibility requires matching parameter lists, return types, and clause sets (after instantiating each parent’s generics with C’s arguments). Diamond inheritance is therefore well-defined only if all inheritance paths (through different parent contracts) agree on every shared member. Informative notes SHOULD indicate which parents contributed conflicting declarations. A contract that appears anywhere in its own inheritance closure triggers E7C15 before this rule is applied.

Contracts MUST NOT provide bodies for inherited members; if an extending contract wishes to refine a clause, it MUST redeclare the signature with strengthened or weakened constraints subject to the substitution rules below.
CITE: Appendix A.7 — ContractBody; Part I §8 — Diagnostics Directory.

### 7.2.5 Associated Types

Contracts MAY declare associated types that implementations MUST fix to concrete types. The grammar reuses `AssociatedTypeDecl` from Appendix A:

```ebnf
AssociatedTypeDecl ::= "type" Ident TypeBound?
```

Static semantics:

```
[AssocType-WF]
contract C { … type Item : Bounds?; … }
Bounds (when present) well-formed trait/contract bounds (Part II §2.9)
───────────────────────────────────────────────────────
Associated type declaration well-formed

[AssocType-Impl]
contract C { type Item : Bounds; … }
record T: C { … type Item = Concrete; … }
Γ ⊢ Concrete : Type
Γ ⊢ Concrete satisfies Bounds[Self ↦ T]
───────────────────────────────────────────────────────
Associated type implementation well-formed
```

```cursive
contract Iterator {
    type Item
    procedure next(self: mut Self): Option<Self::Item>
}

record Range: Iterator {
    start: i32
    end: i32
    type Item = i32

    procedure next(self: mut Self): Option<Self::Item> {
        if self.start < self.end {
            let value = self.start
            self.start += 1
            Option::Some(value)
        } else {
            Option::None
        }
    }
}
```

Associated types MAY carry their own bounds (e.g., `type Item : Copy`). Implementations MUST supply concrete types satisfying those bounds. Recursive definitions that reference the associated type without an intervening constructor are rejected with E7C14. Contracts do not declare associated constants; use associated types together with const generics to express constant requirements. CITE: Part I §8 — Diagnostics Directory.

#### Contract variance

Contracts are invariant in all of their generic parameters: two instantiations `C<T>` and `C<U>` are considered distinct obligations unless `T` and `U` are definitionally equal. This mirrors the variance rules for traits in Part II and prevents unsound substitutability across different concrete types. CITE: Part II §2.7 — Trait Variance.

Formally:

```
[Contract-Variance]
contract C<T>
T ≢ U  (T and U are not definitionally equal)
────────────────────────────────────────
C<T> ≢ C<U>  (distinct contract obligations)
```

**Rationale:** Invariance ensures that a type implementing `C<T>` cannot be substituted where `C<U>` is required, preventing type confusion in contract dispatch. This is necessary because contracts may expose associated types and methods that depend on the exact type parameter, and covariant or contravariant substitution would violate type safety.

##### Completeness of implementations

Implementations are obligated to provide a body (or associated type definition) for every item declared in the contract closure. Missing members are diagnosed immediately after the compatibility pass:

```
[Contract-Member-Missing]
contract C { Signature₁; …; Signatureₙ; }
record T: C { Proc₁; …; Procₖ; Members }
∃i. Signatureᵢ has no corresponding Procⱼ
────────────────────────────────────────────
ERROR E7C11 (missing implementation)
```

The check occurs before coherence analysis to ensure error messages are localized to the implementing type.

### 7.2.6 Coherence and Orphan Rules

Cursive follows the same coherence principles as traits (Part II §2.7):

```
[Contract-Orphan]
implementation of contract C for type T is provided in compilation unit K
K does not define C and K does not define T
────────────────────────────────────────
ERROR E7C12 (orphan implementation)

[Contract-Coherence]
Two implementations of (T, C) exist in the compilation
────────────────────────────────────────
ERROR E7C13 (duplicate implementation)
```

These rules guarantee that contract dispatch is unambiguous within a compilation unit.
CITE: Part II §2.7 — Trait Coherence; Part I §8 — Diagnostics Directory.

Contract implementation checks run when the compilation unit that defines the `record` (or equivalent nominal type) is compiled. Linkers and downstream packages MAY re-run coherence and completeness checks to guard against stale or incorrectly versioned artifacts, but the language requires each compilation unit to be internally valid before it is exported. CITE: Part XII (Modules, Packages, and Imports) §12.4 — Separate Compilation.

### 7.2.7 Usage and Examples

**Example — Polymorphic procedures:**

```cursive
procedure render_all<T>(items: [T])
    where T: Drawable
    sequent { [fs.write] |- true => true }
{
    for item in items {
        item::draw()
    }
}
```

Monomorphization instantiates the procedure for each concrete `T` used, preserving zero-cost abstractions.

**Example — Contract vs trait attachment:** A type MAY attach traits for code reuse while separately implementing a contract; the governing trait rules remain in Part II.

**Example — Higher-order usage:** Contracts can contain methods that accept other contracts as parameters, provided the callee is statically known to implement those contracts:

```cursive
contract Reducer<T> {
    procedure apply(self: mut Self, element: T)
}

procedure fold<T, R>(mut items: [T], reducer: mut R)
    where R: Reducer<T>
{
    for item in items {
        reducer.apply(item)
    }
}
```

Here `reducer` MAY be any type implementing `Reducer<T>`, including adapter wrappers around closures (see §7.2.9 for closure-to-contract adaptation patterns).

---

### 7.2.8 Edge Cases and Restrictions

* **Empty contracts:** A contract MAY contain zero items; implementing it is a no-op beyond coherence participation.
* **Recursive associated types:** An associated type definition that refers to itself without an intervening type constructor is rejected with E7C14 (in addition to the type well-formedness checks in Part II).
* **Effect conflicts:** If an implementation’s `uses` clause introduces effects not present in the contract (after closure), E7C09 is emitted.
* **Never-returning members:** Procedures whose result type is `!` need not supply postconditions; any declared `will` clause applies only to normal return paths.
* **Always-false preconditions:** Contracts MAY declare contradictory `must` predicates; implementations need only prove they do not strengthen the precondition. Call sites will inevitably fail verification, triggering E7C02.
* **Self-extension:** A contract that extends itself (directly or via a cycle) is rejected with E7C15 before inheritance closure is formed.
* **Recursive signatures:** Contracts MAY reference `Self` within member signatures (`Self::Item`, `procedure next(self: Self)`, etc.) but MUST NOT reference the contract name in a position that would require an as-yet-unspecified implementation (such as `procedure f(): ContractName`). Such illegal recursion produces E7C14.
* **Duplicate clauses:** Repeating a `uses`, `must`, or `will` clause within a single signature is rejected with E7C18.
* **Conflicting postconditions:** If multiple inherited signatures declare `will` clauses that cannot be jointly satisfied, the conflict is diagnosed with E7C10 during inheritance closure.
* **Extra members:** Implementations MAY define additional methods or associated items beyond those required by the contract; such additions are not subject to contract clause compatibility checking.

Implementers SHOULD surface these diagnostics eagerly so that violating contracts are caught before downstream types depend on them.
CITE: Part I §8 — Diagnostics Directory.

---

### 7.2.9 Contracts and closures

Anonymous functions and closures are values of function type (`(params) → τ ! ε`) and MAY NOT directly implement contracts (the `record/contract` syntax applies only to nominal types). Nevertheless, closures can satisfy contract-bounded parameters by being wrapped in nominal adapters that forward to the captured callable while preserving clause obligations.

```cursive
contract Handler {
    procedure handle(self: mut Self, message: string)
        sequent { [fs.write] |- true => true }
}

record ClosureHandler {
    callback: (string) → () ! {fs.write}

    procedure handle(self: mut Self, message: string)
        sequent { [fs.write] |- true => true }
    {
        self.callback(message)
    }
}
```

Generic bounds such as `where T: Handler` therefore accept ordinary records as well as adapter wrappers around closures. Effect variables declared on contracts compose with closure captures using the inference rules in Part V §4.19.5. CITE: Part V §4.16 — Contract Annotation Expressions.

### 7.2.10 Contracts and unsafe or foreign code

Contract clauses remain in force inside `unsafe` blocks and across FFI boundaries: the compiler still checks `uses`, `must`, and `will`, but the programmer assumes responsibility for upholding the contract when invoking unchecked operations. Unsafe code MUST re-establish any invariants it MAY temporarily violate before control returns to safe code, and FFI shims SHOULD document how the foreign implementation satisfies the declared clauses. CITE: Part XV (Unsafe Behaviors and FFI) §15.2 — Safety Obligations.

#### Contract checking in unsafe contexts

The contract checking algorithm for unsafe code follows these rules:

```
[Unsafe-Contract-Check]
unsafe { body }
body contains call to f with contract clauses (uses ε, must P, will Q)
────────────────────────────────────────────────────────────────
Compiler checks:
  1. Effect ε is declared in surrounding scope's uses clause
  2. Precondition P is satisfied at call site (static or runtime check)
  3. Postcondition Q is assumed to hold after call (programmer responsibility)
  4. Type invariants are checked at unsafe block boundaries
```

**Programmer obligations in unsafe code:**

1. **Effect discipline:** All effects performed in unsafe blocks MUST be declared in the enclosing procedure's `uses` clause
2. **Precondition satisfaction:** Preconditions MUST be satisfied before calling procedures, even when the call is inside an unsafe block
3. **Postcondition establishment:** When an unsafe block calls foreign code or performs unchecked operations, the programmer MUST ensure postconditions hold
4. **Invariant restoration:** Type invariants MAY be temporarily violated within an unsafe block, but MUST be restored before the block exits

**Example — Unsafe FFI with contracts:**

```cursive
// Foreign function declaration with contract
extern "C" procedure c_sqrt(x: f64): f64
    sequent { [] |- x >= 0.0 => result >= 0.0 }

procedure safe_sqrt(x: f64): f64
    sequent { [ffi.call] |- x >= 0.0 => result >= 0.0 }
{
    unsafe {
        // Antecedent checked before unsafe call
        // Consequent assumed to hold (programmer verifies C implementation)
        c_sqrt(x)
    }
}
```

**Verification strategy:** Implementations SHOULD provide a mode that inserts runtime checks for contract clauses at unsafe block boundaries, enabling detection of contract violations during testing even when static verification is not possible.

### 7.2.11 Dynamic dispatch

Contracts introduce no implicit runtime dispatch. To obtain dynamic polymorphism, programs MUST use contract witnesses as specified in Part II §2.13. Witnesses provide explicit type erasure with permission tracking, effect polymorphism, and modal state verification. See §7.2.12 for contract-specific usage patterns and examples. CITE: Part II §2.13 — Contract Witnesses.

### 7.2.12 Runtime Polymorphism

#### 7.2.12.1 Overview

Runtime polymorphism enables dynamic dispatch for contracts through **contract witnesses**. Unlike compile-time polymorphism via monomorphization (§7.2.7), witnesses allow:

- **Heterogeneous collections:** Store different types implementing the same contract
- **Plugin systems:** Load implementations at runtime
- **Dependency injection:** Swap implementations for testing or configuration
- **Effect polymorphism:** Abstract over effect sets at runtime

**Key principle:** Witnesses make allocation and indirection explicit while preserving Cursive's safety guarantees.

CITE: Part II §2.13 — Contract Witnesses (complete type system specification).

#### 7.2.12.2 Contract Witnesses

**Basic usage:**

```cursive
contract Drawable {
    procedure draw(self)
        sequent { [io.write] |- true => true }
}

// Create witness (explicit heap allocation)
let shape: own witness<Drawable, {io.write}> = witness::heap(Circle{radius: 5.0})

// Dynamic dispatch
    shape::draw()  // Calls Circle::draw() via vtable
```

**Witness construction:**

```cursive
// Heap allocation (default)
witness::heap(value)

// Inline storage (stack, fixed size)
witness::inline::<64>(value)

// Region allocation
region r {
    witness::region::<r>(value)
}

// Borrowed (no allocation)
witness::borrow(&value)
```

#### 7.2.12.3 Testing and Mocking

Effect polymorphism enables mock implementations with ∅ effects while real implementations require effects (∅ ⊆ {net.read}).

CITE: Part II §2.13 — Contract Witnesses (authoritative runtime polymorphism); Part IV — Permission System; Part X — Modals.

### 7.2.13 Modal integration

Contracts interact with modal types by constraining the operations available in each state. Implementations that expose modal transitions MUST ensure their contract clauses hold for every reachable state, and modal transition procedures MAY reference contract predicates when defining admissible transitions. Detailed rules for modal state machines reside in Part X (Modals) §10.4; this chapter assumes those semantics. CITE: Part X §10.4 — Modal State Machines.

#### Modal state contracts

Contracts specify state-dependent operations; compiler verifies preconditions reference fields in source state, postconditions reference fields in target state.


### 7.2.14 Contract Patterns

Common patterns: interface contracts (Serializable), invariant-preserving contracts (Bounded), protocol contracts (Connection), resource management contracts (Resource). Each establishes behavioral obligations verified statically.


---


## 7.3 Sequent Clauses

Sequent clauses provide unified behavioral specifications combining grant context, preconditions (antecedent), and postconditions (consequent) in the form `[ε] ⊢ P ⇒ Q`. If a caller cannot prove the antecedent statically, the compiler emits a runtime check in verification modes that require it (§7.8).

## Definition 7.3 (Sequent Clauses)

A sequent clause is a judgment expressing the behavioral contract of a procedure. It has the form `sequent { [ε] |- P => Q }` where:
- `[ε]` is the grant context (capability assumptions)
- `P` is the antecedent (preconditions over parameters and accessible immutable state)
- `Q` is the consequent (postconditions over `result` and `@old` expressions)

The typing judgment `[Sequent-WF]` ensures all components are pure and well-typed. CITE: Appendix A.7 — SequentClause; Appendix A.8 — Assertion Grammar.

### 7.3.1 Syntax

```cursive
procedure divide(n: f64, d: f64): f64
    sequent { [] |- d != 0.0 => result == n / d }
{
    n / d
}
```

Sequent clauses consist of:
1. **Grant context** `[ε]`: List of grant tokens enclosed in brackets (may be empty `[]`)
2. **Turnstile** `|-`: ASCII representation of `⊢` (entailment/judgment)
3. **Antecedent** `P`: Preconditions as pure boolean expressions using `&&`, `||`, `!` for conjunction, disjunction, and negation
4. **Implication** `=>`: ASCII representation of `⇒` (logical implication)
5. **Consequent** `Q`: Postconditions as pure boolean expressions, may reference `result` and `@old(expr)`

Formal syntax is given in Appendix A.7 (`SequentClause ::= "sequent" "{" SequentSpec "}"`). Components may be omitted: empty grants become `[]`, missing antecedent becomes `true`, missing consequent omits the `=> Q` portion.

### 7.3.2 Static Semantics

```
[Sequent-WF]
Γ ⊢ ε : GrantSet
Γ, params ⊢ P : bool    effects(P) = ∅
Γ, params, result: τ ⊢ Q : bool    effects(Q) = ∅
────────────────────────────────────────────────
sequent { [ε] |- P => Q } well-formed

[Call-Sequent]
Γ ⊢ f : (params) → τ    sequent { [ε] |- P => Q }
Γ ⊢ args : params
ε ⊆ grants(Γ)                        (caller has required grants)
Γ ⊢ P[params ↦ args] provable or checkable    (antecedent holds)
──────────────────────────────────────────────
Call to f(args) permitted
```

Implementations of a contract procedure MAY only:
- **Weaken antecedents**: `P_contract ⇒ P_impl` (accept more inputs)
- **Strengthen consequents**: `Q_impl ⇒ Q_contract` (guarantee more outputs)
- **Reduce grants**: `ε_impl ⊆ ε_contract` (use fewer capabilities)

Violations emit diagnostics E7C07 (antecedent), E7C08 (consequent), or E7C09 (grants).

"Provable" means the verification pipeline described in Part VI §6.13 discharges the assertion statically (`[[verify(static)]]` or stronger analyses); "checkable" means the runtime mode in effect is permitted to insert a dynamic guard (e.g., `[[verify(runtime)]]`). If neither condition holds, the call is rejected during type checking.

#### 7.3.2.1 Provable vs checkable calls

Let `call f(args)` be a procedure invocation with sequent `{ [ε] |- P => Q }`.

* The call is **provable** when Part VI's static verifier establishes `Γ ⊢ P[params ↦ args]` for the antecedent using compile-time reasoning (`[[verify(static)]]` or stronger modes).
* The call is **checkable** when the compilation pipeline is configured to emit a runtime guard that enforces `P` (typically via `[[verify(runtime)]]`), and the surrounding effect set includes `panic` if the guard can fail.

Calls that are neither provable nor checkable are ill-formed. CITE: Part VI §6.13 — Block Contracts and Verification Modes.

### 7.3.3 Dynamic Semantics

```
[Sequent-Static]
Γ ⊢ P[params ↦ args] proved true by analysis or constant evaluation
──────────────────────────────────────────────────────────────
No runtime check emitted for antecedent

[Sequent-Runtime-Ante]
Γ ⊢ P[params ↦ args] not proven
──────────────────────────────────────────────────────────────
Emit runtime assertion: if !P(args) panic("sequent antecedent violated: …")

[Sequent-Runtime-Cons]
Γ ⊢ Q[result ↦ v, params ↦ args_entry] not proven
──────────────────────────────────────────────────────────────
Emit runtime assertion: if !Q(v, args_entry) panic("sequent consequent violated: …")
```

Runtime diagnostics follow the format in Part I §8 (E7S02). Verification attributes (`[[verify(static)]]`, `[[verify(runtime)]]`, `[[verify(none)]]`) in Part VI §6.13 control whether the runtime assertion is generated.
CITE: Part VI §6.13 — Verification Modes; Part I §8 — Diagnostics Directory.

The antecedent guard executes before the body; the consequent guard executes at each return point. If evaluating either guard panics (for example, due to a divide-by-zero inside the predicate), the call fails with E7S02 rather than propagating into or out of the body.

---

### 7.3.4 Predicate expression restrictions

Sequent assertions (antecedents and consequents) reuse the assertion grammar in Appendix A.8. Consequently:

* Assertions MUST be pure; any expression with non-empty effects is rejected.
* Calls inside assertions are limited to declarations whose effect set is `∅`.
* Quantifiers (`forall`, `exists`), boolean connectives (`&&`, `||`, `!`), comparisons, `@old`, and `result` are permitted; arbitrary loops or effectful constructs are not.
* Accessing heap data is allowed only through immutable views. Mutating a captured value violates the purity requirement and fails the `[Sequent-WF]` rule.

Implementations SHOULD surface precise diagnostics indicating which subexpression introduced disallowed effects.

---

### 7.3.5 Quantifiers in sequent assertions

Quantified predicates reuse the assertion grammar productions `forall` and `exists` from Appendix A.8. A quantifier has the form `forall (x in expr) { Predicate }` (likewise for `exists`) where `expr` evaluates to an iterable domain at verification time. The verifier interprets the body predicate with `x` bound to each element of the domain. Runtime checks are emitted only when the verifier cannot statically prove the quantified statement. CITE: Appendix A.8 — Assertion Grammar.

```cursive
procedure sorted(slice: [i32])
    sequent {
        [] |- true
        => forall (i in 0 .. slice.len() - 2) {
            slice[i] <= slice[i + 1]
        }
    }
{
    // body that establishes ascending order
}
```

Quantifiers MUST be pure (their body predicates obey `[Sequent-WF]`), and any domain expression appearing inside a quantifier is evaluated exactly once per dynamic check to avoid recomputation side effects.

---

## 7.4 Sequent Consequents and Special Constructs

Sequent consequents (the `Q` in `[ε] ⊢ P ⇒ Q`) describe guarantees that hold whenever a procedure returns normally. They MAY reference the return value via `result` and pre-state expressions via `@old(expr)`.

## Definition 7.4 (Sequent Consequents)

A sequent consequent is the postcondition portion of a sequent judgment, appearing after the `=>` operator. It is a predicate over the procedure's result (and optionally the pre-state captured through `@old`) that MUST hold on every normal return path. The `[Sequent-WF]` rule types consequents and enforces purity. CITE: Appendix A.7 — SequentClause; Appendix A.8 — Assertion Grammar.

### 7.4.1 The `result` Keyword

```cursive
procedure increment(x: i32): i32
    sequent { [] |- true => result == x + 1 }
{
    x + 1
}
```

`result` denotes the return value of the procedure. It is only valid in the consequent (after `=>`) of a sequent. Using `result` in the antecedent or in a procedure with no return type is ill-formed.

### 7.4.2 The `@old` Construct

For mutating procedures that need to express postconditions in terms of pre-state:

```cursive
procedure push<T>(buffer: mut Vec<T>, item: T)
    sequent { [] |- true => buffer.len() == @old(buffer.len()) + 1 }
{
    buffer.push(item)
}
```

`@old(expr)` captures the value of `expr` at procedure entry; `expr` MAY reference parameters (and fields reachable from `self`) and MUST be pure. The captured value is available in the consequent for comparison with post-state values.

### 7.4.3 Static Semantics of Special Constructs

```
[Result-Valid]
Γ ⊢ procedure p(...): τ    sequent { [ε] |- P => Q }
result appears only in Q
result : τ
────────────────────────────────────────────
result usage valid

[Old-Valid]
Γ, params ⊢ expr : σ
effects(expr) = ∅
@old(expr) appears only in consequent Q
────────────────────────────────────────────
@old(expr) usage valid
```

The `[Sequent-WF]` rule (§7.3.2) governs overall sequent well-formedness including consequents. These rules add specific constraints for `result` and `@old`.

### 7.4.4 Dynamic Semantics of Special Constructs

```
[Old-Capture]
@old(expr) appears in consequent Q
────────────────────────────────────────────
Evaluate expr at procedure entry; retain snapshot for consequent evaluation

[Result-Binding]
procedure returns value v : τ
────────────────────────────────────────────
Bind result to v for consequent evaluation
```

The `[Sequent-Runtime-Cons]` rule (§7.3.3) governs consequent checking. Violations raise diagnostic E7S03. Verification attributes in Part VI §6.13 determine whether runtime checks are emitted.

Implementations MAY apply the same optimization strategy as antecedents: if static analysis or `[[verify(static)]]` proves the consequent, runtime checks are elided; `[[verify(runtime)]]` retains the checks for debugging or safety-critical builds.
CITE: Part VI §6.13 — Verification Modes; Part I §8 — Diagnostics Directory.

Every return site (explicit `return`, fall-through, or early exit) executes the consequent check; compilers MUST NOT defer the evaluation to a single epilogue if doing so would miss intermediate exits.
CITE: Part VI §6.13 — Verification Modes; Part I §8 — Diagnostics Directory.

---

### 7.4.5 `@old` capture semantics and exceptional exits

* Each `@old(expr)` is evaluated once on entry, before any mutation occurs. The expression MUST satisfy the same purity requirements as antecedents (`effects(expr) = ∅`).
* Captured values are logically immutable snapshots; moving or mutating the underlying storage after capture does not affect the recorded value.
* For `Copy` types the snapshot is a by-value copy taken at entry. For non-`Copy` types, the snapshot expression MUST either call a pure helper that produces an immutable copy (e.g., a `Clone` function whose effects are declared) or use an equivalent copy-on-write strategy that preserves the entry state. CITE: Part II §2.3 — Copy Types.
* Nested `@old(@old(expr))` is ill-formed; only one level is permitted.
* If a procedure panics or otherwise diverges, its sequent consequents are not enforced—the guarantees only apply to normal returns.

Expressions whose values cannot be duplicated (e.g., unique resources or region-limited references) are ineligible for `@old` unless the program explicitly provides a safe, effect-free snapshot mechanism (such as a documented `clone` procedure). Attempting to capture such a value with `@old` emits diagnostic E7C16 (see §7.8). CITE: Part I §8 — Diagnostics Directory.

**Example — snapshotting state:**

```cursive
procedure pop(mut stack: Vec<i32>): i32
    sequent {
        [] |- stack.len() > 0
        => stack.len() == @old(stack.len()) - 1
    }
{
    stack.pop().expect("stack underflow")
}
```

---

## 7.5 Type Constraint Clauses (`where`)

Type-level `where` clauses define invariants that MUST hold for every externally visible instance of a nominal type.

*Terminology:* In this chapter "invariant" refers to type invariants attached to nominal declarations. These are distinct from block-level invariants introduced in Part VI §6.13, which govern statement scopes rather than type structure.

## Definition 7.5 (Type Constraint Clauses)

A type constraint clause `where { Invariant* }` specifies predicates that MUST hold for every observable state of the declared type. Each invariant is an assertion over the type's fields evaluated in both static and dynamic contexts as described in §7.5.2–§7.5.4. CITE: Appendix A.6 — TypeConstraint; Appendix A.8 — Assertion Grammar.

### 7.5.1 Syntax

```cursive
record Percentage {
    value: i32
}
where {
    value >= 0,
    value <= 100
}
```

```ebnf
TypeConstraint ::= "where" "{" Invariant ("," Invariant)* "}"
Invariant      ::= Expression  // Must be pure bool over the type’s fields
```

### 7.5.2 Static Semantics

```
[Where-WF]
Γ, fields ⊢ Invariantᵢ : bool
effects(Invariantᵢ) = ∅
────────────────────────────────────────
record T where { Invariant* } well-formed

[Where-Update]
record T where { Invariant* }
public operation mutates T
────────────────────────────────────────
Operation must ensure each invariant holds after mutation
```

### 7.5.3 Dynamic Semantics

Where invariants cannot be discharged statically, the compiler inserts checks according to the following formal rules:

```
[Where-Check-Construct]
record T where { Invariant₁, ..., Invariantₙ }
value v : T constructed
∀i. Invariantᵢ[fields ↦ v.fields] not proven statically
────────────────────────────────────────────────────────────────
Emit runtime check: if !Invariantᵢ(v) panic("invariant violated: ...")

[Where-Check-MutatingExit]
procedure p(self: mut T) where T has invariants { Invariant₁, ..., Invariantₙ }
exit path e (normal return or early exit)
∀i. Invariantᵢ[fields ↦ self.fields] not proven statically at e
────────────────────────────────────────────────────────────────
Emit runtime check at e: if !Invariantᵢ(self) panic("invariant violated: ...")

[Where-Check-FieldWrite]
record T where { Invariant₁, ..., Invariantₙ }
field write: t.field = value
∀i. Invariantᵢ[fields ↦ t.fields] not proven statically after write
────────────────────────────────────────────────────────────────
Emit runtime check after write: if !Invariantᵢ(t) panic("invariant violated: ...")

[Where-Check-ReturnByValue]
procedure p(...): T where T has invariants { Invariant₁, ..., Invariantₙ }
return value v : T
∀i. Invariantᵢ[fields ↦ v.fields] not proven statically
────────────────────────────────────────────────────────────────
Emit runtime check before return: if !Invariantᵢ(v) panic("invariant violated: ...")

[Where-Check-Elision]
Invariant I proven statically at site s
────────────────────────────────────────────────────────────────
No runtime check emitted for I at s
```

**Check site semantics:**

* *Construct* events include record literals, builder functions, and composite initializers that yield `T`. Constructors with explicit bodies run to completion first; the check executes immediately afterward using the fully initialized state.
* *MutatingExit* applies to any procedure (public or private) whose signature grants mutable access (`self: mut Self`, `mut T`, etc.). Private helpers MAY temporarily violate invariants internally provided they restore them before returning to a caller visible outside the defining module.
* *FieldWrite* checks apply when a field is directly assigned. The compiler MAY batch multiple field writes and check invariants once after the final write if it can prove no observable intermediate state exists.
* *ReturnByValue* checks apply when a procedure returns a value of type `T` by value (not by reference).

**Unsafe code obligations:** Mutations performed through unsafe pointers (`unsafe.ptr`) transfer responsibility to the surrounding scope; failing to re-establish the invariant triggers E7C05. When a type participates in inheritance, every override is responsible for re-validating the invariants declared by its ancestors in addition to its own declarations.

If static verification (e.g., via `[[verify(static)]]`) proves the invariants for a site, the corresponding runtime checks are suppressed. Violations raise diagnostics E7C04 (construction) or E7C05 (mutation). CITE: Part VI §6.13 — Verification Modes; Part I §8 — Diagnostics Directory.

### 7.5.4 Performance considerations

Repeated invariant checking can be expensive for large aggregates. Implementations SHOULD:

* combine multiple checks on the same path into a single verification point;
* respect `[[verify(runtime)]]` / `[[verify(none)]]` attributes, falling back to runtime only when mandated; and
* allow profiling hooks that identify hotspots where additional static proof annotations would remove checks.

Because invariants are part of the public contract, compilers MUST NOT omit checks without proof that the constraints hold.
CITE: Part VI §6.13 — Verification Modes; Part I §8 — Diagnostics Directory.

#### Performance complexity analysis

**Time complexity:**

Let `T` be a type with `n` invariants, each of complexity `O(f(k))` where `k` is the size of the type's data:

* **Construction check:** `O(n · f(k))` — all invariants evaluated once
* **Mutation check:** `O(n · f(k))` per mutating operation exit
* **Field write check:** `O(n · f(k))` per write (unless batched)
* **Return check:** `O(n · f(k))` per return site

**Optimization strategies:**

1. **Static elimination:** When the verifier proves an invariant holds, the check is elided entirely (`O(0)`)
2. **Batching:** Multiple field writes in sequence can share a single check at the end (`O(n · f(k))` instead of `m · O(n · f(k))` for `m` writes)
3. **Incremental checking:** For invariants that depend on a subset of fields, only re-check when those fields change
4. **Caching:** For pure invariants over immutable data, cache the result and reuse it

**Space complexity:**

* **`@old` captures:** `O(k)` per postcondition using `@old`, where `k` is the size of captured data
* **Invariant evaluation:** `O(1)` stack space for predicate evaluation (assuming no deep recursion)
* **Check metadata:** `O(n)` per type for storing invariant predicates

**Worst-case scenario:** A type with `n` complex invariants (`O(k²)` each) mutated `m` times in a tight loop incurs `O(m · n · k²)` overhead. Implementations SHOULD warn when invariant checking dominates execution time and suggest static verification or check elision strategies.

**Best practices for performance:**

* Keep invariants simple (`O(1)` or `O(k)` complexity preferred)
* Use `[[verify(static)]]` to eliminate checks in hot paths
* Batch related mutations to share invariant checks
* Consider splitting types with expensive invariants into smaller components

---

## 7.6 Sequent Composition

### 7.6.1 Grant propagation

```
[Compose-Grants]
Γ ⊢ f : (params) → τ    sequent { [ε_f] |- P => Q }
caller grants ε_call
─────────────────────────────────────────────
call permitted only if ε_f ⊆ ε_call
```

Composite expressions and statement blocks accumulate grant requirements using the union rules in Part IX §9.2; procedure calls inherit the callee's declared grant set from the sequent context. For grant-polymorphic callees (`∀ε`), the instantiated grant set determined by inference (see §9.3) is the one checked against the caller budget. Failure raises E7C01 or E9004. CITE: Part IX §9.2 — Grant Union.

E7C01 is emitted when a procedure's body performs an operation requiring an undeclared grant; E9004 is emitted when a caller's grant budget does not subsume the callee's requirements. Diagnostics SHOULD clearly report the offending grant token. CITE: Part I §8 — Diagnostics Directory.

### 7.6.2 Antecedents across call chains

```
[Compose-Antecedent]
Γ ⊢ f : (params) → τ    sequent { [ε] |- P => Q }
Γ ⊢ args : params
Γ ⊢ P[params ↦ args] provable or checkable
─────────────────────────────────────────────
Γ ⊢ call f(args) safe
```

Implementations MAY weaken inherited preconditions (`P_contract ⇒ P_impl`) but MUST NOT strengthen them (§7.2.3); violations trigger E7C07. "Weaker" here means the implementation accepts every argument the contract allows (possibly more). For multiple inheritance, the implementation MUST satisfy the conjunction of all inherited preconditions.
CITE: Part I §8 — Diagnostics Directory.

**Example — Valid weakening:**

```cursive
contract Positive {
    procedure consume(n: i32)
        sequent { [] |- n > 0 => true }
}

record Counter: Positive {
    value: i32

    procedure consume(self: mut Self, n: i32)
        sequent { [] |- n >= 0 => true }    // allowed: (n > 0) ⇒ (n >= 0)
    {
        if n == 0 {
            // additional case permitted by the weaker antecedent
            return
        }
        self.value -= n
    }
}
```

### 7.6.3 Consequents and assumptions

```
[Compose-Consequent]
Γ ⊢ f : (params) → τ    sequent { [ε] |- P => Q }
call returns value v
─────────────────────────────────────────────
Caller may assume Q[params ↦ args, result ↦ v]
```

Implementations MUST strengthen inherited consequents (`Q_impl ⇒ Q_contract`); weakening emits E7C08. Callers MAY rely on the conjunction of consequents from all contracts satisfied by the callee.
CITE: Part I §8 — Diagnostics Directory.

**Example — Violated strengthening (diagnostic E7C08):**

```cursive
contract Sized {
    procedure len(self: Self): usize
        sequent { [] |- true => result >= 0 }
}

record MaybeSized: Sized {
    procedure len(self: Self): usize
        sequent { [] |- true => result <= 0 }   // incompatible: does not imply result >= 0
    { 0 }
}
```

### 7.6.4 Sequent substitution checklist

When substituting an implementation for a contract member, the compiler MUST verify sequent compatibility:

Given contract sequent `{ [ε_c] |- P_c => Q_c }` and implementation sequent `{ [ε_i] |- P_i => Q_i }`:

1. **Grants**: `ε_i ⊆ ε_c` (implementation uses fewer or equal capabilities, else E7C09)
2. **Antecedent**: `P_c ⇒ P_i` (implementation weakens or maintains precondition, else E7C07)
3. **Consequent**: `Q_i ⇒ Q_c` (implementation strengthens or maintains postcondition, else E7C08)

Only after these checks succeed does the call graph treat the implementation as a drop-in replacement for the contract signature. Diagnostics MUST identify which sequent component failed and, when applicable, the conflicting contracts in an inheritance graph.
CITE: Part I §8 — Diagnostics Directory.

---

## 7.7 Diagnostics

This section catalogs all errors, warnings, and lints related to contracts, grants, sequents (antecedents and consequents), and type invariants.

### 7.7.1 Contract Declaration Errors (E7C-01xx)

| Code | Description | Example |
|------|-------------|---------|
| E7C-0101 | Contract procedure contains body (contracts must be abstract) | `contract C { procedure p() { ... } }` |
| E7C-0102 | Contract extends itself (direct or indirect cycle) | `contract C extends C` |
| E7C-0103 | Conflicting inherited contract requirements | Contract extends two contracts with incompatible signatures |
| E7C-0104 | Recursive associated type definition | `contract C { type T = C::T }` |
| E7C-0105 | Contract associated type lacks required bounds | Implementation doesn't satisfy bounds |
| E7C-0106 | Duplicate procedure signature in contract | Same procedure declared twice |
| E7C-0107 | Duplicate contract clause in signature | `uses fs.read, fs.read` |

### 7.7.2 Contract Implementation Errors (E7C-02xx)

| Code | Description | Example |
|------|-------------|---------|
| E7C-0201 | Missing implementation for contract member | Type implements contract but omits procedure |
| E7C-0202 | Implementation signature does not match contract | Wrong parameter types or return type |
| E7C-0203 | Implementation strengthens contract precondition | `must` weaker in contract, stronger in implementation |
| E7C-0204 | Implementation weakens contract postcondition | `will` stronger in contract, weaker in implementation |
| E7C-0205 | Implementation effect set exceeds contract declaration | Implementation uses undeclared effects |
| E7C-0206 | Orphan contract implementation | Neither type nor contract is local to crate |
| E7C-0207 | Duplicate contract implementation for type | Type implements same contract twice |
| E7C-0208 | Associated type not specified in implementation | Missing `type Item = T` |

### 7.7.3 Grant System Errors (E7G-01xx)

| Code | Description | Example |
|------|-------------|---------|
| E7E-0101 | Effect operation names not distinct | `effect E { op(), op() }` |
| E7E-0102 | Effect operation body type mismatch | Body returns wrong type |
| E7E-0103 | Effect path must be valid identifier | `effect 123.invalid` |
| E7E-0104 | Marker effect cannot have operations | `effect marker { op() }` |
| E7E-0105 | `continue()` only allowed in effect implementations | Used outside effect body |
| E7E-0106 | `abort()` only allowed in effect implementations | Used outside effect body |
| E7E-0107 | `continue()` type mismatch with operation return | Continuing with wrong type |
| E7E-0108 | Undeclared effect used in body | Effect not in `uses` clause |
| E7E-0109 | Undeclared effect operation used | Specific operation not granted |
| E7E-0110 | Effect not available in context | Called function requires unavailable effect |
| E7E-0111 | `with` block implementation signature mismatch | Handler doesn't match operation |
| E7E-0112 | `with` block references undefined effect | Unknown effect in `with` |
| E7E-0113 | `with` block references undefined operation | Unknown operation in handler |
| E7E-0114 | Effect requires implementation but none provided | No default and no `with` block |
| E7E-0115 | Multiple `continue()` in non-copyable context | Continuation consumed twice |

### 7.7.4 Sequent Antecedent Errors (E7S-01xx)

| Code | Description | Example |
|------|-------------|---------|
| E7S-0101 | Sequent antecedent violated at runtime | Antecedent (before `=>`) evaluated to false |
| E7S-0102 | Sequent antecedent contains non-pure expression | Antecedent uses effectful operation |
| E7S-0103 | Sequent antecedent type is not `bool` | Antecedent doesn't evaluate to boolean |
| E7S-0104 | Sequent antecedent references undefined parameter | Antecedent refers to non-existent parameter |
| E7S-0105 | Sequent antecedent captures mutable state | Antecedent references non-immutable data |

### 7.7.5 Sequent Consequent Errors (E7S-02xx)

| Code | Description | Example |
|------|-------------|---------|
| E7S-0201 | Sequent consequent violated at runtime | Consequent (after `=>`) evaluated to false |
| E7S-0202 | Sequent consequent contains non-pure expression | Consequent uses effectful operation |
| E7S-0203 | Sequent consequent type is not `bool` | Consequent doesn't evaluate to boolean |
| E7S-0204 | Sequent consequent references unavailable binding | Consequent refers to invalid scope |
| E7S-0205 | Illegal `@old` capture (nested or non-snapshot) | `@old(@old(x))` or `@old` of non-Copy type |
| E7S-0206 | `result` used outside consequent | `result` keyword in antecedent or grant context |

### 7.7.6 Type Invariant Errors (E7I-01xx)

| Code | Description | Example |
|------|-------------|---------|
| E7I-0101 | Type invariant violated at construction | `where` clause false after initialization |
| E7I-0102 | Type invariant violated after mutation | `where` clause false after procedure call |
| E7I-0103 | Type invariant contains non-pure expression | `where` uses effectful operation |
| E7I-0104 | Type invariant type is not `bool` | `where` doesn't evaluate to boolean |
| E7I-0105 | Type invariant references non-field data | `where` accesses external state |

### 7.7.7 Sequent Composition Errors (E7X-01xx)

| Code | Description | Example |
|------|-------------|---------|
| E7X-0101 | Effect set union exceeds available effects | Callee requires effects not in caller's `uses` |
| E7X-0102 | Conflicting postconditions from multiple sources | Diamond inheritance with incompatible `will` clauses |
| E7X-0103 | Effect inference ambiguity (no principal effect) | Multiple valid effect sets, none most general |
| E7X-0104 | Circular dependency in contract clauses | `must` depends on `will` from same signature |

### 7.7.8 Warnings

| Code | Description | Example |
|------|-------------|---------|
| W7C-0001 | Unused contract parameter | Contract procedure parameter never referenced |
| W7C-0002 | Contract always satisfied (trivial precondition) | `must true` |
| W7C-0003 | Contract never satisfied (contradictory precondition) | `must false` |
| W7C-0004 | Effect declared but never used | `uses fs.read` but no file operations |
| W7C-0005 | Postcondition redundant with type system | `will result >= 0` for `usize` return |

### 7.7.9 Selected Diagnostic Examples

**E7C-0203: Implementation Strengthens Contract Antecedent**

```cursive
contract Processor {
    procedure process(self: mut Self, value: i32)
        sequent { [] |- value >= 0 => true }
}

record StrictProcessor: Processor {
    procedure process(self: mut Self, value: i32)
        sequent { [] |- value > 0 => true }  // ERROR: stricter than contract
    {
        // Implementation
    }
}
// ERROR E7C-0203: implementation strengthens contract antecedent
```

**E7G-0108: Undeclared Grant Used in Body**

```cursive
procedure save_data(path: string, data: string) {
    FileSystem::write(path, data)
}
// ERROR E7G-0108: undeclared grant used in body
// help: add grant to sequent: `sequent { [fs.write] |- true => true }`
```

**E7S-0101: Sequent Antecedent Violated at Runtime**

```cursive
procedure divide(numerator: i32, denominator: i32): i32
    sequent { [] |- denominator != 0 => result == numerator / denominator }
{
    result numerator / denominator
}

procedure example() {
    let result = divide(10, 0)
}
// ERROR E7S-0101: sequent antecedent violated at runtime
```

Diagnostics MUST be reported using the standard format defined in Part I §8.

