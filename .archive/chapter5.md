# Chapter 5: Declarations

**Forward references**: §2.2 [lex.phases], §2.5 [lex.units], Chapter 4 [module], Chapter 6 [name], Chapter 7 [type], Chapter 8 [expr], Chapter 9 [stmt], Chapter 10 [generic], Chapter 11 [memory], Chapter 12 [contract], Chapter 13 [witness], Chapter 15 [interop], Annex A [grammar]. 

This chapter defines the syntactic forms that introduce names in Cursive programs and the rules that govern their scope, visibility, initialization, lifetime responsibilities, and interaction with the grant and contract systems. It consolidates and refines the material from the declaration subchapters into a single, coherent specification that follows the house style and uses consistent terminology and normative language.

---

## 5.1 Declaration overview [decl.overview]

### 5.1.1 Overview [decl.overview.main]

A **declaration** binds an identifier to a category (value, type, procedure, predicate, contract, or grant), assigns visibility and storage attributes, and determines how later references participate in the permission, grant, and modal systems. Declarations appear at module scope and within block scopes; both participate in a single lexical identifier namespace per scope. Shadowing an outer binding requires the `shadow` keyword. Implementations use two‑phase compilation to record the complete declaration inventory before semantic analysis, so forward references resolve without stubs. 

Cursive recognizes these declaration categories:

* **Variable bindings** (`let`, `var`, `shadow let`, `shadow var`) with optional binding patterns and explicit responsibility operators (`=` responsible, `<-` non‑responsible);
* **Binding patterns** for record, tuple, and nested destructuring;
* **Procedures** (`procedure`, `comptime procedure`, `extern procedure`) with receiver shorthands, contractual sequents, and move‑responsibility on parameters;
* **Type declarations** (`record`, tuple record, `enum`, `modal`, `type` alias), including member procedures;
* **Visibility modifiers** (`public`, `internal`, `private`, `protected`);
* **Grant declarations** for capability tokens used in sequents. 

### 5.1.2 Namespace and scope invariants [decl.overview.ns]

Cursive maintains a **single lexical identifier namespace** per scope: types and values share identifiers. A declaration must not redeclare an identifier in the same scope. A declaration may shadow an outer scope only when written with `shadow`. Name lookup and shadowing rules in Chapter 6 operate on visibility and scope metadata recorded by this chapter. 

### 5.1.3 Integration points [decl.overview.integration]

* **Permissions** (Chapter 11) interpret mutability, region, and ownership qualifiers attached to declarations.
* **Contracts and grants** (Chapter 12) extend procedures with capability requirements and pre/postconditions; this chapter specifies where sequents attach.
* **Module initialization** (Chapter 4) determines ordering for module‑scope bindings (§5.7) and the program entry model (§5.8). 

> [informative!]
> Example 5.1.3.1 (Representative module declarations):
>
> ```cursive
> public record Account { id: u64, balance: i64 }
>
> use bank::ledger::post_entry
>
> let DEFAULT_LIMIT = 10_000
> shadow var session_state = Session::new()
>
> public procedure create_account(id: u64): Account
> {
>     result Account { id, balance: 0 }
> }
> ```
>
> The module defines a public record, imports a ledger operation, introduces immutable and mutable bindings, and exposes a callable. 

---

## 5.2 Variable bindings and initializers [decl.variable]

### 5.2.1 Overview [decl.variable.overview]

Variable bindings introduce identifiers with explicit rebindability (`let` vs `var`), optional pattern destructuring (§5.3), and a responsibility operator that governs cleanup (`=` responsible, `<-` non‑responsible). Bindings may appear at module or block scope; storage duration and initialization ordering depend on location (§5.7). 

### 5.2.2 Syntax [decl.variable.syntax]

**Binding**:

```ebnf
binding
    ::= binding_head pattern [ ":" type ] binding_operator expression

binding_head
    ::= 'let' | 'var' | 'shadow' 'let' | 'shadow' 'var'

binding_operator
    ::= '=' | '<-'
```

The `pattern` nonterminal is defined in §5.3. Multi‑identifier patterns require a type annotation on the enclosing binding (§5.3.3). See the statement grammar in Annex A for `var_decl_stmt`.

### 5.2.3 Constraints [decl.variable.constraints]

* **Explicit shadowing.** `shadow let` and `shadow var` must only be used when an enclosing scope already binds the same identifier.
* **Single assignment.** `let` must not be reassigned; `var` may be reassigned.
* **Pattern uniformity.** Multi‑identifier patterns must have a uniform type annotation `: T` and must bind distinct identifiers.
* **Initializer required.** Every binding form must have an initializer; the operator determines responsibility metadata, not the type.
* **Compile‑time constants.** A binding is a compile‑time constant only when it is a module‑scope `let` with a compile‑time evaluable initializer or is declared within a `comptime` context.
* **Permission independence.** Binding rebindability controls assignment, not value mutation; mutation permissions come from types (Chapter 11).
* **Forward references.** Two‑phase compilation permits forward references; shadowing is validated after parsing. 

### 5.2.4 Semantics [decl.variable.semantics]

Evaluating a pattern binding evaluates the initializer **once**, then destructures it into constituent bindings. Patterns do not change visibility or storage semantics: each introduced identifier inherits the binding’s visibility and scope. The responsibility operator (`=` or `<-`) affects cleanup responsibility recorded in the binding table and used by move‑checking and definite assignment but does not participate in type checking. 

### 5.2.5 Binding operator semantics [decl.variable.operator]

#### 5.2.5.1 Responsible assignment `=` [decl.variable.operator.equal]

Using `=` creates a **responsible** binding. The binding will invoke the value’s destructor at scope exit in reverse declaration order. A responsible `let` may transfer responsibility using `move`; a responsible `var` may be rebound, destroying the old value first. 

> **Example 5.2.5.1.1** (Responsible drop at scope exit):
>
> ```cursive
> {
>     let f = File::open("data.txt")   // responsible
>     // use f
> } // f.drop() runs here
> ```
>
> The binding owns cleanup. 

#### 5.2.5.2 Non‑responsible binding `<-` [decl.variable.operator.arrow]

Using `<-` creates a **non‑responsible** binding to the same typed value. The binding **must not** call a destructor at scope exit and **cannot** be the target of `move`. Multiple non‑responsible bindings to the same object are allowed; their validity follows the source object’s lifetime (§5.7.5). 

> **Example 5.2.5.2.1** (Type preservation and non‑responsibility):
>
> ```cursive
> let buf: Buffer = Buffer::new()  // responsible
> let view: Buffer <- buf          // non‑responsible; same type, no cleanup
> ```
>
> Only responsibility metadata differs. 

#### 5.2.5.3 Rebinding rules [decl.variable.operator.rebind]

Rebinding a **responsible** `var` destroys the old value before assignment. Rebinding a **non‑responsible** `var` simply retargets the binding; no destructor runs. 

### 5.2.6 Diagnostics and conformance [decl.variable.requirements]

Implementations must:

* Diagnose missing outer bindings for `shadow` (E05‑201), reassignments to `let` (E05‑202), pattern non‑uniformity or duplicates (E05‑203), and destructuring failures (E05‑204);
* Track cleanup responsibility per operator and enforce invalidation of non‑responsible bindings when a source is moved to a responsible parameter (§5.7.5), and reject `move` on non‑responsible bindings (E11‑502);
* Preserve type identity across operators. 

---

## 5.3 Binding patterns [decl.pattern]

### 5.3.1 Overview [decl.pattern.overview]

Binding patterns provide declarative destructuring for variable bindings, projecting fields or positional elements from composite values. Patterns inherit visibility, storage duration, and shadowing from the enclosing binding. 

### 5.3.2 Syntax [decl.pattern.syntax]

```ebnf
pattern
    ::= identifier
     |  '{' record_field_list '}'
     |  '(' pattern_list ')'

record_field_list
    ::= record_field (',' record_field)*

record_field
    ::= identifier
     |  identifier 'as' identifier
     |  identifier ':' pattern

pattern_list
    ::= pattern (',' pattern)*
```

Patterns appear as `let pattern: T = expr` or `var pattern: T = expr`. When a pattern binds more than one identifier, the type annotation on the enclosing binding applies to the whole pattern. 

### 5.3.3 Constraints [decl.pattern.constraints]

* **Type annotation required.** Patterns that bind more than one identifier must include a type annotation on the enclosing binding; the type must match the pattern’s shape.
* **Field matching.** Record fields must exist in the annotated record type; tuple patterns must match the exact arity. Pattern matching is all‑or‑nothing.
* **Distinct identifiers.** A pattern must not introduce duplicate identifiers.
* **Single evaluation.** The initializer must be evaluated once and projected recursively. 

### 5.3.4 Semantics [decl.pattern.semantics]

* `let {f as x}: R = e` binds `x` to `e.f`; absent `as`, the binding name is `f`.
* `let (l, r): (T0, T1) = e` binds `l` to `e.0` and `r` to `e.1`.
* After destructuring, only the introduced identifiers remain in scope; the composite is not retained implicitly. 

> **Example 5.3.4.1** (Record pattern with renaming):
>
> ```cursive
> let {x, y as vertical}: Point2 = where_am_i()
> ```
>
> Binds `x` and `vertical`.
> **Example 5.3.4.2** (Invalid: missing annotation):
>
> ```cursive
> let {real, imag} = z   // ERROR E05-304: missing type annotation
> ```
>
> The annotation is required because the pattern binds two identifiers. 

### 5.3.5 Conformance [decl.pattern.requirements]

Compilers must require type annotations for multi‑identifier patterns and enforce record and tuple shape checking, uniqueness, and single evaluation as specified above (E05‑301–E05‑304). 

---

## 5.4 Procedures [decl.function]

### 5.4.1 Overview [decl.function.overview]

Procedure declarations introduce named computations at module scope or as members of types. Procedures are the only callable form. Purity is determined by the contractual sequent’s grant set: an empty set denotes a pure procedure; a non‑empty set permits the named capabilities. A procedure may declare a receiver parameter `self` via shorthands `~` (const), `~%` (shared), or `~!` (unique). 

### 5.4.2 Syntax [decl.function.syntax]

```ebnf
procedure_decl
    ::= [attributes] [visibility] procedure_head procedure_signature procedure_body

procedure_head
    ::= procedure_kind identifier

procedure_kind
    ::= 'procedure'
     |  'comptime' 'procedure'
     |  'extern' ['"' string_literal '"'] 'procedure'

procedure_signature
    ::= [generic_params] '(' [parameter_list] ')' [ ':' type ] [ sequent_clause ]

parameter
    ::= receiver_shorthand
     |  ['move'] identifier ':' type

receiver_shorthand
    ::= '~' | '~%' | '~!'

sequent_clause
    ::= '[[' sequent_expr ']]'   // see Annex A (sequent grammar)

procedure_body
    ::= block
     |  '=' expression ';'       // pure procedures only
     |  ';'                      // extern declarations
```

Expression‑bodied forms are syntactic sugar for **pure** procedures and must not include an explicit sequent. If a sequent is omitted anywhere, it defaults to `[[ |- true => true ]]` (empty grant set). Sequent shorthands expand per §5.4.4. 

### 5.4.3 Constraints [decl.function.constraints]

* **Visibility defaults.** Module‑scope procedures default to `internal`; associated procedures inherit the containing type’s visibility unless overridden.
* **Receiver position.** A receiver, if present, must be first, written explicitly (`self: const/shared/unique Self`) or via shorthand.
* **Parameter responsibility.** The optional `move` modifier marks a parameter as **responsible** (the callee takes cleanup responsibility). The call site must write `move` for such arguments; it must not for others.
* **Purity.** Procedures with empty grant sets must not perform grant‑requiring operations nor call non‑pure procedures.
* **Comptime and extern.** `comptime procedure` bodies must be compile‑time evaluable; `extern procedure` declarations omit bodies.
* **Expression bodies.** Expression‑bodied procedures must not include explicit sequents. 

### 5.4.4 Semantics [decl.function.semantics]

A procedure’s type records parameter types, return type, grant set, and sequent. Sequents govern: the **grants** required, the **precondition** that must hold on entry, and the **postcondition** that will hold on normal exit. Receiver shorthands desugar to an explicit `self: <perm> Self`. Comptime procedures execute during compile‑time evaluation. 

#### 5.4.4.1 Parameter responsibility equivalence [decl.function.params.responsibility]

Responsibility at parameters mirrors local bindings:

| Parameter form | Equivalent local binding | Responsibility | Destructor at return |
| -------------- | ------------------------ | -------------- | -------------------- |
| `x: T`         | `let x <- argument`      | no             | no                   |
| `move x: T`    | `let x = argument`       | yes            | yes                  |

Call sites must use `move` exactly when the callee’s parameter requires it. 

> **Example 5.4.4.1.1** (Move‑responsibility at call sites):
>
> ```cursive
> procedure consume(move b: Buffer) { /* ... */ }
> let buf = Buffer::new()
> consume(move buf)        // OK: buf becomes invalid
> // consume(buf)          // ERROR E05-409: missing move
> ```
>
> The `move` keyword at the call site signals transfer of cleanup responsibility. 

### 5.4.5 Conformance [decl.function.requirements]

Implementations must validate receiver placement (E05‑401), parameter responsibility and call‑site usage (E05‑409, E05‑410), purity restrictions (E05‑406), comptime restrictions (E05‑402), and the prohibition on sequents for expression‑bodied procedures (E05‑408). 

---

## 5.5 Type declarations [decl.type]

### 5.5.1 Overview [decl.type.overview]

Type declarations introduce nominal product types (records and tuple records), sum types (enums), modal state machines, and transparent type aliases. Member procedures obey §5.4. Contract and behavior clauses attach obligations and implementations resolved in Chapters 10 and 12. 

### 5.5.2 Syntax (abridged) [decl.type.syntax]

```ebnf
type_decl
    ::= record_decl
     |  record_tuple_decl
     |  enum_decl
     |  modal_decl
     |  type_alias

record_decl
    ::= [attributes] [visibility] 'record' Id [generic_params]
        [contract_clause] [behavior_clause] record_body

record_body
    ::= '{' [record_member (',' record_member)*] '}'

record_member
    ::= [visibility] Id ':' type
     |  procedure_decl

record_tuple_decl
    ::= [attributes] [visibility] 'record' Id [generic_params]
        [contract_clause] [behavior_clause] '(' [field (',' field)*] ')' [record_body]

enum_decl
    ::= [attributes] [visibility] 'enum' Id [generic_params]
        [contract_clause] [behavior_clause] enum_body

enum_body
    ::= '{' [variant (',' variant)*] '}'

variant
    ::= [visibility] Id
     |  [visibility] Id '(' [type (',' type)*] ')'
     |  [visibility] Id '{' [field (',' field)*] '}'

modal_decl
    ::= [attributes] [visibility] 'modal' Id [generic_params]
        [contract_clause] [behavior_clause] modal_body

modal_body
    ::= '{' modal_state_block+ '}'

modal_state_block
    ::= '@' Id ['{' field_list '}'] state_transition*

state_transition
    ::= '@' Id '::' Id '(' [parameter_list] ')' '->' '@' Id

type_alias
    ::= [visibility] 'type' Id [generic_params] '=' type
```

Modal transition **signatures** declare the state graph with `->`. Corresponding **procedure implementations** use the normal procedure syntax, returning a transition type `@Source -> @Target`, which is sugar for `(Self@Source, ...params) -> Self@Target`. 

### 5.5.3 Constraints and semantics [decl.type.constraints]

* **Visibility defaults.** Type declarations at module scope default to `internal`. Members inherit the containing declaration’s visibility unless overridden.
* **Namespace uniqueness.** Type names share the lexical namespace with other identifiers (§5.1.2).
* **Field requirements.** Record and enum field names must be unique with explicit types; tuple record fields list types only.
* **Member procedures.** Embedded procedures follow §5.4 and may use receiver shorthands; placing sequents on the following line improves readability.
* **Enum variants.** Unit, tuple, and record‑style variants are supported; identifiers must be unique.
* **Contracts and behaviors.** Referenced items must be visible; duplicates are diagnosed. The type must satisfy all obligations; conflicting required signatures make the declaration ill‑formed.
* **Modal structure.** Modal types must declare at least one state; each transition signature must have a matching procedure implementation (receiver, parameters, return transition).
* **Type aliases.** Aliases are transparent and must not participate in cycles.
* **Recursive types.** Recursive definitions are permitted if they do not require infinite size. 

> **Example 5.5.3.1** (Modal type with transitions and implementations):
>
> ```cursive
> modal File {
>     @Closed { path: Path }
>     @Closed::open(~!, path: Path) -> @Open
>
>     @Open { path: Path, handle: Handle }
>     @Open::read(~%, buffer: BufferView) -> @Open
>     @Open::close(~!) -> @Closed
> }
>
> procedure File.open(~!, path: Path): @Closed -> @Open
>     [[ fs::open |- path.exists() => result.state() == @Open ]]
> { /* ... */ }
> ```
>
> The signature declares the edge; the procedure implements it with a transition return type. 

### 5.5.4 Conformance [decl.type.requirements]

Compilers must enforce uniqueness of type names, fields, variants, and modal states; validate contract/behavior clauses; validate modal transitions and their implementations; and reject alias cycles and unsupported representation attributes. 

---

## 5.6 Visibility rules [decl.visibility]

### 5.6.1 Overview and placement [decl.visibility.overview]

Visibility modifiers control which scopes may refer to a declaration: `public`, `internal`, `private`, and `protected`. Module‑scope declarations default to `internal`. Visibility must not be written on local (block‑scoped) bindings. Members inside types inherit the type’s visibility unless overridden. 

### 5.6.2 Semantics [decl.visibility.semantics]

* **public** — accessible from any module once imported; re‑exports preserve this visibility.
* **internal** — accessible only within the declaring module; attempts to widen visibility via re‑export are ill‑formed.
* **private** — accessible only within the smallest enclosing declaration (e.g., the declaring type’s own procedures). Writing `private` at module scope is ill‑formed.
* **protected** — may be applied only to members inside a type or modal state block; accessible within the declaring type and within behavior/contract implementations for that type; behaves as `internal` elsewhere. Re‑exports must not widen or narrow visibility. 

### 5.6.3 Conformance [decl.visibility.requirements]

Implementations must reject `private` or `protected` at module scope, forbid visibility on local bindings, prevent re‑exports from widening or narrowing visibility, and enforce the `protected` access context described above. 

---

## 5.7 Initialization and definite assignment [decl.initialization]

### 5.7.1 Overview [decl.initialization.overview]

This section defines when and how declarations acquire initial values and how the compiler proves that each binding is definitely assigned before use. It also formalizes validity and invalidation rules for non‑responsible bindings created with `<-`. 

### 5.7.2 Module‑scope initialization [decl.initialization.module]

* Module‑scope `let`/`var` bindings initialize exactly once before code in the module executes.
* Implementations must construct the eager dependency graph and evaluate initializers in topological order; cycles are ill‑formed.
* If an initializer fails, dependents are blocked and diagnosed.
* A module‑scope `let` with a compile‑time evaluable initializer becomes a compile‑time constant. 

### 5.7.3 Block‑scoped initialization [decl.initialization.block]

Block‑scoped bindings initialize when control reaches the declaration. A binding must not be used before it is assigned on every path. Diagnostics report potential unassignment across branches and loops. 

### 5.7.4 Definite assignment rules [decl.initialization.da]

* A `let` or `var` must be assigned on every control‑flow path before each use.
* For conditionals, each branch must assign before the post‑dominating use.
* Before entering a loop, any value used in the loop condition or body must already be assigned for code that runs before the first iteration.
* Pattern bindings must bind every identifier in the pattern; omissions are ill‑formed. 

### 5.7.5 Non‑responsible binding validity [decl.initialization.nonresp]

Non‑responsible bindings created with `<-` bind to **objects**, not to other bindings. Their validity depends on whether the object still exists. Because the language enforces zero‑cost abstractions, the compiler uses **parameter responsibility** (from §5.4) as the static signal for potential destruction:

* Passing a source binding to a **non‑responsible parameter** preserves the object; derived non‑responsible bindings remain valid.
* Moving a source binding to a **responsible parameter** (`move`) may destroy the object; the source and all derived non‑responsible bindings become invalid after the call site. 

The core invalidation rule:

$$
\frac{\text{argument is } \texttt{move}\ \text{to a responsible parameter}}
{\text{the source binding and all derived non‑responsible bindings become invalid}}
\quad \text{[WF‑NonResp‑Invalidate‑OnDestroy]}
$$

This rule prevents use‑after‑free without runtime tracking. 

> **Example 5.7.5.1** (Invalidation on move to responsible parameter):
>
> ```cursive
> procedure consume(move data: Buffer) { /* may destroy */ }
>
> let owner = Buffer::new()         // responsible
> let view  <- owner                // non‑responsible
>
> consume(move owner)               // owner moved
> // view.read()                    // ERROR E11-504: invalidated reference
> ```
>
> The call may destroy the object; all derived non‑responsible bindings become invalid. 

Additional validity conditions:

* A non‑responsible binding becomes invalid at its own scope exit.
* If the source binding’s scope ends, any non‑responsible views must not outlive it.
* Permission coercions for non‑responsible bindings follow Chapter 11 (e.g., `unique → const` is permitted; upgrades and cross‑coercions are not). 

### 5.7.6 Reassignment and drop order [decl.initialization.drop]

Reassigning a `let` is ill‑formed. When a scope exits, responsible bindings drop in reverse declaration order. Clause‑level destructor semantics are specified in Chapter 11. 

### 5.7.7 Conformance [decl.initialization.requirements]

Implementations must build the module initialization graph and diagnose cycles and blocked dependents; enforce definite assignment; track dependencies for non‑responsible bindings and propagate invalidation on moves to responsible parameters; and ensure pattern bindings are complete. 

---

## 5.8 Program entry and execution model [decl.entry]

### 5.8.1 Entry point requirements [decl.entry.requirements]

An executable must define **exactly one** top‑level `public procedure main` with one of these signatures:

```cursive
public procedure main(): i32
```

or

```cursive
public procedure main(args: [string]): i32
```

`main` must be `public`. It may omit a sequent only when it performs no grant‑requiring operations; otherwise, it must explicitly list the required grants. `main` cannot be `comptime` or asynchronous. Prohibited attributes must not decorate `main`. 

### 5.8.2 Execution order and exit status [decl.entry.exec]

Before `main` executes, all module‑scope initializers in the root module and its dependencies run according to §5.7 and Chapter 4. `main` runs on the initial thread. If `main` returns normally, its `i32` result maps to the platform’s exit status. Panic or abort yields a non‑zero exit status; destructor semantics are implementation‑defined per Chapter 11. 

> **Example 5.8.2.1** (Entry with explicit grants):
>
> ```cursive
> public procedure main(): i32
>     [[ io::write |- true => true ]]
> {
>     println("Hello, Cursive!")
>     result 0
> }
> ```
>
> The sequent documents capability requirements. 

### 5.8.3 Conformance [decl.entry.conformance]

Implementations must require exactly one `public` `main` (E05‑801), reject non‑public entry points (E05‑802), forbid `comptime`/async `main` (E05‑803), and reject disallowed attributes (E05‑804). 

---

## 5.9 Grant declarations [decl.grant]

### 5.9.1 Overview and syntax [decl.grant.overview]

Grant declarations introduce user‑defined capability tokens at module scope:

```ebnf
grant_decl ::= [visibility] 'grant' Id
```

A grant has no runtime representation; it is referenced in contractual sequents by its fully qualified path and is erased during compilation. Built‑in grant namespaces are reserved. 

### 5.9.2 Constraints and visibility [decl.grant.constraints]

* Declared at **module scope only**; names must be unique within the module and must not conflict with reserved namespaces (e.g., `alloc`, `fs`, `net`, `time`, `ffi`, `panic`, and `comptime.*`).
* Default visibility is `internal`; `public`, `internal`, `private`, and `protected` apply as in §5.6.
* Two‑phase compilation permits forward references to grants within the same module. 

> **Example 5.9.2.1** (Using user‑defined and built‑in grants):
>
> ```cursive
> public grant query
>
> procedure run(sql: string@View): [i32]
>     [[ query, fs::write |- sql.len() > 0 => result.len() >= 0 ]]
> { /* ... */ }
> ```
>
> The procedure requires `query` and `fs::write`. 

### 5.9.3 Conformance [decl.grant.requirements]

Implementations must validate scope, uniqueness, and reserved‑namespace conflicts (E05‑901–E05‑903); construct fully qualified grant paths; enforce visibility constraints on usage; and integrate grant declarations with the contract system. 

---

## 5.10 Cross‑cutting diagnostics and terminology [decl.terms]

* **Ill‑formed** means the program must be rejected with a diagnostic.
* **Responsible binding** means the binding must run the value’s destructor at scope exit unless responsibility is moved.
* **Non‑responsible binding** means the binding must not run a destructor and must follow the validity rules in §5.7.5.
* **Pure procedure** means a procedure with an empty grant set in its sequent (or an expression‑bodied procedure); it must not perform grant‑requiring operations.

---

## 5.11 Editorial notes (non‑normative) [decl.editorial]

> [note!]
> This chapter follows the Cursive specification style guide: it uses **must/should/may**, American spelling, numbered headings with stable labels, example numbering, and cross‑references of the form `§X.Y [label]`. Grammar excerpts use `ebnf` code fences; programs use `cursive` fences. 

---

### Index of examples

* **Example 5.1.3.1** — Representative module declarations
* **Example 5.2.5.1.1** — Responsible drop at scope exit
* **Example 5.2.5.2.1** — Type preservation and non‑responsibility
* **Example 5.3.4.1** — Record pattern with renaming
* **Example 5.3.4.2** — Invalid: missing annotation
* **Example 5.4.4.1.1** — Move‑responsibility at call sites
* **Example 5.5.3.1** — Modal type with transitions and implementations
* **Example 5.7.5.1** — Invalidation on move to responsible parameter
* **Example 5.8.2.1** — Entry with explicit grants

---

### Consolidated conformance summary [decl.conformance.summary]

Implementations must:

* Recognize all declaration categories and enforce single‑namespace and explicit‑shadowing rules (§5.1–§5.3);
* Enforce binding responsibility semantics and diagnostics for `=`/`<-` and `move` (§5.2, §5.4);
* Validate patterns for shape, arity, uniqueness, and annotation (§5.3);
* Validate procedure receivers, purity, sequents, comptime/extern rules, and call‑site `move` usage (§5.4);
* Validate types, members, contracts/behaviors, modal transitions, and alias transparency (§5.5);
* Enforce visibility defaults and re‑export constraints (§5.6);
* Construct module initialization graphs, perform definite‑assignment analysis, and track non‑responsible binding validity with parameter responsibility (§5.7);
* Require exactly one `public` `main` with an allowed signature and proper grants (§5.8);
* Enforce grant declaration scope, visibility, and reserved namespaces (§5.9).

---

> [informative!]
> **Change log highlights vs. prior drafts**
>
> * Unified responsibility semantics across bindings and parameters; made `<-` type‑preserving and responsibility‑only.
> * Promoted modal transition separation (signature vs. implementation) and clarified `->` (mapping) vs. `:` (type annotation).
> * Consolidated visibility rules and prohibited scope‑level misuse consistently.
> * Specified the precise invalidation rule for non‑responsible bindings using parameter responsibility as the static signal.

---

**End of Chapter 5.**
