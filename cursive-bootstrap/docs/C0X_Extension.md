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

$$\frac{p \ \text{PermSub} \ q}{p\ T \ <: \ q\ T}\quad (T-Perm-Sub)$$

All other subtyping rules are inherited from Cursive0.md.

### 4.3 Receiver permissions

A method declared with receiver permission `p_m` is callable through a path with permission `p_c` iff:

$$p_c \ \text{PermSub} \ p_m$$

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

$$\Gamma \vdash \text{AttrList}(A) \ \text{wf}@\tau$$

meaning: attribute list `A` is well-formed for attachment target kind `\tau` (declaration kind or expression kind).

Well-formedness requires:

1. Every attribute name in `A` is registered.
2. The target kind `\tau` is permitted for the attribute.
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

Let `\Delta` be a type-parameter environment mapping type parameter identifiers to `(bounds, default, variance)`.

Typing judgments are extended to carry `\Delta` implicitly (or explicitly, depending on the baseline judgment form):

- When type-checking inside a generic item, `\Delta` includes that item’s type parameters.
- `where` predicates introduce additional bounds constraints into `\Delta`.

### 6.2 Well-formedness of generic parameter lists

Judgment:

$$\Gamma \vdash \text{GenParams}(G) \ \text{wf} \triangleright \Delta$$

Rules:

- Parameter names are unique.
- Each bound `T <: Cl` is well-formed and refers to a class `Cl`.
- Each default type (if present) is well-formed under the bounds of previously-declared parameters.
- Variance is computed after type-checking the generic item body (see §6.5).

### 6.3 Well-formedness of instantiation arguments

Judgment:

$$\Gamma;\Delta \vdash \text{GenArgs}(A) \ \text{ok for} \ G \triangleright \theta$$

Where `\theta` is a substitution mapping each parameter name in `G` to a concrete type.

Rules:

1. If `A` provides an argument for each parameter, `\theta` is the pointwise mapping.
2. If fewer arguments are provided, missing arguments are filled by defaults; if any missing parameter lacks a default, the instantiation is ill-formed.
3. For each parameter `X` with bounds `B`, `\theta(X)` MUST satisfy all bounds (see §6.4).

### 6.4 Bound satisfaction

Let `Implements(\Gamma, T, Cl)` denote the class-implementation judgment already present in the C0 baseline for classes.

Define:

$$\Gamma \vdash T \models (Cl_1 + \ldots + Cl_n) \iff \forall i.\ \text{Implements}(\Gamma, T, Cl_i)$$

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

$$\frac{\forall i.\ \text{Var}(C,i)=+ \Rightarrow A_i <: B_i \ \ \ \text{Var}(C,i)=- \Rightarrow B_i <: A_i \ \ \ \text{Var}(C,i)== \Rightarrow A_i = B_i}{C\langle A_1,\ldots,A_n\rangle <: C\langle B_1,\ldots,B_n\rangle} \ \ (T-Gen-Sub)$$

Where `Var(C,i)` is selected by permission view:

- If the *outer* permission of the type is `const`, use `VarConst(C,i)`.
- If the outer permission is `shared` or `unique`, use `VarMut(C,i)`.

This captures the design property that `const` enables covariance for otherwise invariant generic types when invariance arises from mutability rather than observable contravariance.

### 6.6 Monomorphization

C0X defines generics by elaboration to a monomorphic program.

Let `Instantiate(d, \theta)` denote substitution of type parameters in a declaration `d`.

**Instantiation graph.** The compiler maintains a set `Inst` of demanded instantiations `(d, \theta)` discovered from:

- Type applications `N<...>`
- Procedure calls with explicit generic args (turbofish)
- Type inference of generic calls

**Algorithm (normative):**

1. Initialize `Inst` with all concrete uses from the program’s entry module.
2. While `Inst` has an unprocessed pair `(d, \theta)`:
   1. Produce `d' = Instantiate(d, \theta)`.
   2. Type-check `d'` with no remaining type parameters.
   3. Scan `d'` for further generic uses and add them to `Inst`.
3. The final monomorphic program is the set of all produced `d'`.

**Termination.** Recursive instantiation cycles are permitted only if they correspond to well-founded recursion through runtime values; non-terminating type-level recursion is ill-formed.

---

## 7. Shared permission and the Key System

C0X adopts the Key System from the original design, with minor alignment to the C0 baseline terminology.

### 7.1 Keys

A **key** is a triple:

$$K = (P, M, S)$$

- `P` is a key path
- `M ∈ {Read, Write}` is a mode
- `S` is a scope identifier

### 7.2 Key paths

Key paths are derived from place expressions over `shared` values.

A key path is a sequence of segments:

$$P = (r, s_1, \ldots, s_k)$$

where `r` is a root identifier (or boundary root) and each segment is either:

- field access `.f`
- indexing `[i]`

A segment may be **marked** by the coarsening marker `#`, which truncates the key path at that segment.

### 7.3 Key contexts

A task carries a key context `\mathcal{K}` (set of held keys).

Define:

- `Covers(\mathcal{K}, (P, Read))` iff `\mathcal{K}` contains a key `(P', M', S')` such that `P'` is a prefix of `P` and `M'` is at least `Read`.
- `Covers(\mathcal{K}, (P, Write))` iff `\mathcal{K}` contains `(P', Write, S')` with `P'` prefix of `P`.

### 7.4 Implicit acquisition

When evaluating a `shared` access requiring `(P, M)`, the runtime:

- If `Covers(\mathcal{K}, (P, M))`, no action is taken.
- Otherwise, acquires key `(P, M, S_current)` and adds it to `\mathcal{K}`.

The compiler MUST ensure (statically or via runtime synchronization in `[[dynamic]]` scope) that concurrent tasks do not acquire conflicting keys.

### 7.5 Conflicts and disjointness

Two path-mode pairs conflict if they overlap and at least one is a write:

$$Conflict((P_1,M_1),(P_2,M_2)) \iff Overlap(P_1,P_2) \land (M_1=Write \lor M_2=Write)$$

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

$$\Gamma \vdash \text{ClassDecl}(Cl) \ \text{wf}$$

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

$$\frac{\Gamma \vdash D : \$ExecutionDomain \quad \Gamma_P \vdash B : T}{\Gamma \vdash \texttt{parallel } D\ \{B\} : T} \ (T-Parallel)$$

where `\Gamma_P` extends `\Gamma` with the parallel context.

Within a parallel block:

- `spawn` and `dispatch` expressions are permitted.
- Each spawned task has its own Key System context (keys are task-local).

### 10.2 Spawn

Typing:

$$\frac{\Gamma[parallel\_context]=D \quad \Gamma_{cap} \vdash e : T}{\Gamma \vdash \texttt{spawn }\{e\} : SpawnHandle\langle T\rangle} \ (T-Spawn)$$

`SpawnHandle<T>` is a modal type with states `@Pending` and `@Ready`.

### 10.3 Wait

Typing:

$$\frac{\Gamma \vdash h : SpawnHandle\langle T\rangle}{\Gamma \vdash \texttt{wait } h : T} \ (T-Wait)$$

**Key restriction.** No keys may be held across a `wait` suspension point. Therefore, `wait` is well-formed only when the current key context is empty.

### 10.4 Dispatch

Typing without reduction:

$$\frac{\Gamma \vdash range : Range\langle I\rangle \quad \Gamma,i:I \vdash B : T}{\Gamma \vdash \texttt{dispatch } i \texttt{ in range }\{B\} : ()} \ (T-Dispatch)$$

Typing with reduction:

$$\frac{\Gamma \vdash range : Range\langle I\rangle \quad \Gamma,i:I \vdash B : T \quad \Gamma \vdash \oplus : (T,T)\to T}{\Gamma \vdash \texttt{dispatch } i \texttt{ in range [reduce: }\oplus\texttt{] }\{B\} : T} \ (T-Dispatch-Reduce)$$

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

---

# Appendices: Full Formalization Extracts

The main body of this document integrates C0X into the C0 baseline and provides the consolidated grammars for the added constructs. The appendices below provide **complete, formal, and normative** definitions (judgements, inference rules, and semantics) for the requested feature areas, aligned with the original design document.

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

$$\frac{
    \Gamma \vdash \text{resolve\_item}(\text{path}::i) \Rightarrow \text{item} \quad \Gamma \vdash \text{can\_access}(\text{item}) \quad \text{name} \notin \text{dom}(\Gamma)
}{
    \Gamma \vdash \text{using path}::i \text{ as name}: WF
}$$

**(WF-Using-Wildcard)**

$$\frac{
    \Gamma \vdash \text{resolve\_module}(\text{path}) \Rightarrow m \quad \forall i \in \text{accessible}(m), \text{name}(i) \notin \text{dom}(\Gamma)
}{
    \Gamma \vdash \text{using path}::*: WF
}$$

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

$$\frac{
    \Gamma \vdash \text{using path}::i \text{ as name}: WF \quad \text{visibility}(\text{item}) = \text{public}
}{
    \Gamma \vdash \text{public using path}::i \text{ as name}: WF
}$$

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

An attribute $A$ is defined by:

$$A = (\text{Name}, \text{Args})$$

where $\text{Name}$ is an identifier and $\text{Args}$ is a (possibly empty) sequence of arguments.

An **attribute list** is a sequence of one or more attributes:

$$\text{AttributeList} = \langle A_1, A_2, \ldots, A_n \rangle$$

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

An attribute $A$ is defined by:

$$A = (\text{Name}, \text{Args})$$

where $\text{Name}$ is an identifier and $\text{Args}$ is a (possibly empty) sequence of arguments.

An **attribute list** is a sequence of one or more attributes:

$$\text{AttributeList} = \langle A_1, A_2, \ldots, A_n \rangle$$

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

**Attribute Registry.** The set ($\mathcal{R}$) of all attributes recognized by a conforming implementation:

$$\mathcal{R} = \mathcal{R}_{\text{spec}} \uplus \mathcal{R}_{\text{vendor}}$$

where $\mathcal{R}_{\text{spec}}$ contains specification-defined attributes and $\mathcal{R}_{\text{vendor}}$ contains vendor-defined extensions.

**Static Semantics**

**Attribute Processing**

For each attribute $A$ applied to declaration $D$:

1. The implementation MUST verify $A.\text{Name} \in \mathcal{R}$
2. The implementation MUST verify $D$ is a valid target for $A$
3. The implementation MUST verify $A.\text{Args}$ conforms to the argument specification
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

**Vendor-Defined Attributes.** Attributes in $\mathcal{R}_{\text{vendor}}$ that extend the specification-defined set.

**Static Semantics**

1. Vendor attributes MUST use reverse-domain-style prefixes: `[[com.vendor.attribute_name]]`
2. The `cursive.*` namespace is reserved
3. Vendor-defined attributes are Implementation-Defined Behavior (IDB)

An implementation encountering an unknown attribute (not in $\mathcal{R}$) MUST diagnose error `E-MOD-2451`.


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

1. $N$ MUST be a positive integer that is a power of two
2. Effective alignment is $\max(N, \text{natural alignment})$
3. If $N < \text{natural alignment}$, natural alignment is used (warning emitted)
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
| `layout(align(N))`         | Valid    | Minimum alignment $N$                   |
| `layout(C, packed)`        | Valid    | C-compatible packed layout              |
| `layout(C, align(N))`      | Valid    | C-compatible with minimum alignment $N$ |
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

An expression $e$ is within a `[[dynamic]]` scope if and only if:
1. $e$ is syntactically enclosed in a declaration marked `[[dynamic]]`, OR
2. $e$ is syntactically enclosed in an expression marked `[[dynamic]]`

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

**Diagnostics:** See Appendix A, codes `E-CON-0410`–`E-CON-0412`, `E-SEM-2801`, `E-TYP-1953`, `W-CON-0401`. For `E-CON-0020` (key safety outside `[[dynamic]]` scope), see §17.6.1.


#### 7.4.4 `[[static_dispatch_only]]`

**`[[static_dispatch_only]]`.** Excludes a procedure in a class from dynamic dispatch via `$Class` (vtable).

**Syntax**

```ebnf
static_dispatch_attr ::= "[[" "static_dispatch_only" "]]"
```

**Static Semantics**

**VTable Eligibility:**

A procedure $p$ in a class is vtable-eligible if:

$$\text{vtable\_eligible}(p) \iff \text{has\_receiver}(p) \land \neg\text{is\_generic}(p) \land \text{compatible\_signature}(p)$$

**Dispatchability (Dynamic Dispatch Safety):**

A class is **dispatchable** (usable as a dynamic class type `$Class`) if every procedure is either vtable-eligible or has `[[static_dispatch_only]]`:

$$\text{dispatchable}(C) \iff \forall p \in \text{procedures}(C).\ \text{vtable\_eligible}(p) \lor \text{has\_static\_dispatch\_attr}(p)$$

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

- Modal widening ($M@S <: M$, where $M$ is the general modal type): §12.1
- Union member subtyping: §11.8
- Refinement type subtyping: §13.7
- Permission subtyping: §10.3

**Diagnostics:** See Appendix A, codes `E-TYP-1510`–`E-TYP-1512`.


### 9.3 Variance

**Variance.** Specifies how subtyping of type arguments relates to subtyping of parameterized types. For a generic type constructor $F$ with parameter $T$, variance determines whether $F[A] <: F[B]$ when $A <: B$, when $B <: A$, both, or neither.

**Static Semantics**

**Variance Classifications**

| Variance      | Symbol | Condition for $F[A] <: F[B]$ | Position Requirement                              |
| :------------ | :----- | :--------------------------- | :------------------------------------------------ |
| Covariant     | $+$    | $A <: B$                     | Output positions (return types, immutable fields) |
| Contravariant | $-$    | $B <: A$                     | Input positions (parameter types)                 |
| Invariant     | $=$    | $A \equiv B$                 | Both input and output, or mutable storage         |
| Bivariant     | $\pm$  | Always (parameter unused)    | Parameter does not appear in type structure       |

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
| $\Gamma \vdash e \Rightarrow T$ | Synthesis | Type $T$ is derived from the structure of expression $e$ |
| $\Gamma \vdash e \Leftarrow T$  | Checking  | Expression $e$ is validated against expected type $T$    |

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

Let $\mathcal{P}$ denote the set of permissions:

$$\mathcal{P} = \{\texttt{const}, \texttt{unique}, \texttt{shared}\}$$

A **permission-qualified type** is a pair $(P, T)$ where $P \in \mathcal{P}$ and $T$ is a base type. The notation $P\ T$ denotes this qualification.

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

$$\forall p_1, p_2 \in \text{Paths}.\ (\text{perm}(p_1) = \texttt{unique} \land \text{overlaps}(p_1, p_2)) \implies p_1 = p_2$$

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

$$\texttt{unique} <: \texttt{shared} <: \texttt{const}$$

**Lattice Diagram:**

```
    unique      (strongest: exclusive, mutable)
       ↓
    shared      (synchronized, mutable)
       ↓
     const      (weakest: unlimited, immutable)
```


### 10.2 Exclusivity and Aliasing Rules

**Aliasing.** Two paths alias when they refer to overlapping storage locations: $\text{aliases}(p_1, p_2) \iff \text{storage}(p_1) \cap \text{storage}(p_2) \neq \emptyset$.

**Binding State Machine**

A binding $b$ with `unique` permission exists in one of two states:

| State    | Definition                                        | Operations Permitted         |
| :------- | :------------------------------------------------ | :--------------------------- |
| Active   | No downgraded references to $b$ are live          | Read, write, move, downgrade |
| Inactive | One or more downgraded references to $b$ are live | No operations                |

**Static Semantics**

**(Inactive-Enter)**
$$\frac{b : \texttt{unique}\ T \quad b\ \text{is Active} \quad \text{downgrade to}\ P\ \text{where}\ P \in \{\texttt{const}, \texttt{shared}\}}{b\ \text{becomes Inactive}}$$

**(Inactive-Exit)**
$$\frac{b\ \text{is Inactive} \quad \text{all downgraded references to}\ b\ \text{go out of scope}}{b\ \text{becomes Active with}\ \texttt{unique}\ \text{permission restored}}$$

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

A method with receiver permission $P_{\text{method}}$ is callable through a path with permission $P_{\text{caller}}$ iff:

$$P_{\text{caller}} <: P_{\text{method}}$$

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

A generic declaration $D$ is defined by:

$$D = (\text{Name}, \text{Params}, \text{Body})$$

where:
- $\text{Name}$ is the declaration's identifier
- $\text{Params} = \langle P_1, P_2, \ldots, P_n \rangle$ is an ordered sequence of type parameters
- $\text{Body}$ is the declaration body (type definition or procedure body)

Each type parameter $P_i$ is defined by:

$$P_i = (\text{name}_i, \text{Bounds}_i)$$

where:
- $\text{name}_i$ is an identifier serving as the parameter name
- $\text{Bounds}_i \subseteq \mathcal{T}_{\text{class}}$ is a (possibly empty) set of class bounds

A type parameter with $\text{Bounds}_i = \emptyset$ is **unconstrained**. A type parameter with $\text{Bounds}_i \neq \emptyset$ is **constrained**; only types implementing all classes in $\text{Bounds}_i$ may be substituted.

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

$$\frac{
    \forall i \neq j,\ \text{name}_i \neq \text{name}_j \quad
    \forall i,\ \forall B \in \text{Bounds}_i,\ \Gamma \vdash B : \text{Class}
}{
    \Gamma \vdash \langle P_1; \ldots; P_n \rangle\ \text{wf}
}$$

**(WF-Generic-Type)**

$$\frac{
    \Gamma \vdash \langle P_1, \ldots, P_n \rangle\ \text{wf} \quad
    \Gamma, T_1 : P_1, \ldots, T_n : P_n \vdash \text{Body}\ \text{wf}
}{
    \Gamma \vdash \texttt{type}\ \textit{Name}\langle P_1, \ldots, P_n \rangle\ \text{Body}\ \text{wf}
}$$

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

$$\frac{
    \Gamma \vdash \langle P_1, \ldots, P_n \rangle\ \text{wf} \quad
    \Gamma' = \Gamma, T_1 : P_1, \ldots, T_n : P_n \quad
    \Gamma' \vdash \text{signature}\ \text{wf} \quad
    \Gamma' \vdash \text{body}\ \text{wf}
}{
    \Gamma \vdash \texttt{procedure}\ f\langle P_1, \ldots, P_n \rangle(\ldots) \to R\ \{\ldots\}\ \text{wf}
}$$

**(T-Generic-Call)**

$$\frac{
    \begin{gathered}
    f\langle T_1, \ldots, T_n \rangle(x_1 : S_1, \ldots, x_m : S_m) \to R\ \text{declared} \\
    \forall i \in 1..n,\ \Gamma \vdash A_i\ \text{satisfies}\ \text{Bounds}(T_i) \\
    \forall j \in 1..m,\ \Gamma \vdash e_j : S_j[A_1/T_1, \ldots, A_n/T_n]
    \end{gathered}
}{
    \Gamma \vdash f\langle A_1, \ldots, A_n \rangle(e_1, \ldots, e_m) : R[A_1/T_1, \ldots, A_n/T_n]
}$$

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

$$\frac{
    \forall B \in \text{Bounds},\ \Gamma \vdash A <: B
}{
    \Gamma \vdash A\ \text{satisfies}\ \text{Bounds}
}$$

**(T-Generic-Inst)**

$$\frac{
    \text{Name}\langle P_1, \ldots, P_n \rangle\ \text{declared} \quad
    \forall i \in 1..n,\ \Gamma \vdash A_i\ \text{satisfies}\ \text{Bounds}(P_i)
}{
    \Gamma \vdash \text{Name}\langle A_1, \ldots, A_n \rangle\ \text{wf}
}$$

When both inline bounds and a `where` clause specify constraints for the same parameter, the effective constraint is the conjunction of all bounds.

**Constraints**

1. A class bound MUST reference a valid class type; bounding by non-class types is forbidden.
2. A generic instantiation MUST provide exactly the number of type arguments declared.
3. Type arguments MUST satisfy all constraints declared for their corresponding parameters.

**Diagnostics:** See Appendix A, codes `E-TYP-2302`, `E-TYP-2303`, `E-TYP-2305`.


#### 13.1.4 Monomorphization

**Monomorphization.** The process of generating specialized code for each concrete instantiation of a generic type or procedure.

Given a generic declaration $D\langle T_1, \ldots, T_n \rangle$ and concrete type arguments $A_1, \ldots, A_n$, monomorphization produces a specialized declaration $D[A_1/T_1, \ldots, A_n/T_n]$ where each occurrence of $T_i$ in the body is replaced with $A_i$.

**Static Semantics**

**Monomorphization Requirements**

1. **Specialization:** For each instantiation $D\langle A_1, \ldots, A_n \rangle$, produce code equivalent to substituting each type argument for its corresponding parameter throughout the declaration body.

2. **Zero Overhead:** Calls to generic procedures MUST compile to direct static calls to the specialized instantiation. Virtual dispatch is prohibited for static polymorphism.

3. **Independent Instantiation:** Each distinct instantiation is an independent type or procedure. `Container<i32>` and `Container<i64>` are distinct types.

**Recursion Depth**

Monomorphization MAY produce recursive instantiations. Implementations MUST detect and reject infinite monomorphization recursion. The maximum instantiation depth is IDB; minimum supported depth is 128.

**Variance**

The variance of each type parameter is determined by its usage within the type definition. See §9.3 for variance specification.

**Dynamic Semantics**

**Layout Independence**

Each monomorphized instantiation has an independent memory layout:

$$\text{sizeof}(\text{Name}\langle A_1, \ldots, A_n \rangle) = \text{sizeof}(\text{Name}[A_1/T_1, \ldots, A_n/T_n])$$

$$\text{alignof}(\text{Name}\langle A_1, \ldots, A_n \rangle) = \text{alignof}(\text{Name}[A_1/T_1, \ldots, A_n/T_n])$$

**Constraints**

**Diagnostics:** See Appendix A, codes `E-TYP-2307`, `E-TYP-2308`.


### 13.2 Classes (Forms)


#### 13.2.1 Class Declaration Syntax

**Class.** A named declaration that defines an abstract interface consisting of procedure signatures, associated type declarations, abstract fields, and abstract states.

A class $Cl$ is defined as a tuple:

$$Cl = (N, G, S, P_{abs}, P_{con}, A_{abs}, A_{con}, F, St)$$

where:
- $N$ is the class name
- $G$ is the (possibly empty) set of generic type parameters
- $S$ is the (possibly empty) set of superclass bounds
- $P_{abs}$ is the set of abstract procedure signatures
- $P_{con}$ is the set of concrete procedure definitions (default implementations)
- $A_{abs}$ is the set of abstract associated type declarations
- $A_{con}$ is the set of concrete associated type bindings
- $F$ is the (possibly empty) set of abstract field declarations
- $St$ is the (possibly empty) set of abstract state declarations

A class with $St \neq \emptyset$ is a **modal class**. Only modal types may implement modal classes.

A type $T$ **implements** class $Cl$ (written $T <: Cl$) when:

$$T <: Cl \iff \forall p \in P_{abs}.\ T \text{ defines } p\ \land\ \forall a \in A_{abs}.\ T \text{ binds } a\ \land\ \forall f \in F.\ T \text{ has } f\ \land\ \forall s \in St.\ T \text{ has } s$$

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

$$\frac{
  \text{unique}(N) \quad
  \forall p \in P.\ \Gamma, \text{Self} : \text{Type} \vdash p\ \text{wf} \quad
  \neg\text{cyclic}(S) \quad
  \text{names\_disjoint}(P, A, F, St)
}{
  \Gamma \vdash \text{class } N\ [<: S]\ \{P, A, F, St\}\ \text{wf}
}$$

**(T-Superclass)**

$$\frac{\text{class } A <: B \quad T <: A}{\Gamma \vdash T <: B}$$

Within a class declaration, `Self` denotes the (unknown) implementing type.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-TYP-2500`, `E-TYP-2504`, `E-TYP-2505`, `E-TYP-2508`, `E-TYP-2509`, `E-TYP-2408`, `E-TYP-2409`.


#### 13.2.2 Associated Methods

**Abstract Procedure.** A procedure signature within a class that lacks a body. Implementing types MUST provide a concrete implementation.

**Concrete Procedure.** A procedure definition within a class that includes a body. Implementing types automatically inherit this procedure but MAY override it using the `override` keyword.

**Static Semantics**

**(WF-Class-Self)**

$$\frac{\Gamma, \text{Self} : \text{Type} \vdash \text{body} : \text{ok}}{\Gamma \vdash \text{class } T\ \{\ \text{body}\ \} : \text{Class}}$$


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

$$\frac{\text{type } Alias = A + B}{\Gamma \vdash T <: Alias \iff \Gamma \vdash T <: A \land \Gamma \vdash T <: B}$$


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

$$\frac{
  T <: Cl \quad
  \forall p \in P_{abs}(Cl).\ T \text{ defines } p \quad
  \forall a \in A_{abs}(Cl).\ T \text{ binds } a \quad
  \forall f \in F(Cl).\ T \text{ has } f \quad
  \forall s \in St(Cl).\ T \text{ has } s
}{
  \Gamma \vdash T \text{ implements } Cl
}$$

**(WF-Impl)**

$$\frac{
  \Gamma \vdash T\ \text{wf} \quad
  \Gamma \vdash Cl\ \text{wf} \quad
  St(Cl) \neq \emptyset \implies T \text{ is modal}
}{
  \Gamma \vdash T <: Cl\ \text{wf}
}$$

**Override Semantics**

- **Implementing an abstract procedure**: `override` keyword MUST NOT be used
- **Overriding a concrete procedure**: `override` keyword MUST be used

**Coherence Rule**

A type `T` MAY implement a class `Cl` at most once.

**Orphan Rule**

For `T <: Cl`, at least one of `T` or `Cl` MUST be defined in the current assembly.

**(T-Field-Compat)**

$$\frac{
  f : T_c \in F(Cl) \quad
  f : T_i \in \text{fields}(R) \quad
  T_i <: T_c
}{
  R \vdash f\ \text{present}
}$$

**(T-Modal-Class)**

$$\frac{
  St(Cl) \neq \emptyset \quad
  T <: Cl \quad
  T \text{ is not a modal type}
}{
  \text{ill-formed: E-TYP-2401}
}$$

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

$$\frac{\Gamma \vdash v : T \quad \Gamma \vdash T <: Cl \quad \text{dispatchable}(Cl)}{\Gamma \vdash v : \$Cl}$$

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

$$\text{dispatchable}(Cl) \iff \forall p \in \text{procedures}(Cl).\ \text{vtable\_eligible}(p) \lor \text{has\_static\_dispatch\_attr}(p)$$

**Dynamic Semantics**

**Dynamic Class Type Creation**

1. Let `v` be a value of concrete type `T` where `T <: Cl` and `dispatchable(Cl)`.
2. Let `dp` be a pointer to the storage of `v`.
3. Let `vt` be the static vtable for the `(T, Cl)` type-class pair.
4. Construct the dynamic class value as the pair `(dp, vt)`.

**(E-Dynamic-Form)**

$$\frac{
  \Gamma \vdash v : T \quad
  T <: Cl \quad
  \text{dispatchable}(Cl)
}{
  v \Rightarrow_{\$} (\text{ptr}(v), \text{vtable}(T, Cl))
}$$

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

$$\frac{
  w = (dp, vt) \quad
  \text{method} \in \text{interface}(Cl) \quad
  vt[\text{offset}(\text{method})] = fp
}{
  w\text{\textasciitilde>method}(args) \to fp(dp, args)
}$$

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

$$\frac{
  \Gamma \vdash \text{body} : T \quad
  \Gamma \vdash T <: Cl \quad
  \text{return\_type}(f) = \text{opaque } Cl
}{
  \Gamma \vdash f : () \to \text{opaque } Cl
}$$

**(T-Opaque-Project)**

At call sites, the opaque type is treated as an existential; callers may invoke only class methods:

$$\frac{
  \Gamma \vdash f() : \text{opaque } Cl \quad
  m \in \text{interface}(Cl)
}{
  \Gamma \vdash f()\text{~>}m(args) : R_m
}$$

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

$$\frac{
  f \neq g
}{
  \text{typeof}(f()) \neq \text{typeof}(g())
}$$

**Zero Overhead**

Opaque types MUST incur zero runtime overhead. The returned value is the concrete type, not a dense pointer. Type encapsulation is enforced statically.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-TYP-2510`–`E-TYP-2512`.


### 13.7 Refinement Types


#### 13.7.1 Refinement Syntax

**Refinement Type.** A type constructed by attaching a predicate constraint to a base type. The refinement type `T where { P }` denotes the subset of values of type `T` for which predicate `P` evaluates to `true`.

A refinement type $R$ is defined by:

$$R = (T_{\text{base}}, P)$$

where:
- $T_{\text{base}} \in \mathcal{T}$ is the base type being refined
- $P : T_{\text{base}} \to \texttt{bool}$ is a pure predicate constraining the value set

$$\text{Values}(T \text{ where } \{P\}) = \{ v \in \text{Values}(T) \mid P(v) = \texttt{true} \}$$

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

$$\frac{
    \Gamma \vdash T\ \text{wf} \quad
    \Gamma, \texttt{self} : T \vdash P : \texttt{bool} \quad
    \text{Pure}(P)
}{
    \Gamma \vdash (T \text{ where } \{P\})\ \text{wf}
}$$

**(T-Refine-Intro)**

$$\frac{
    \Gamma \vdash e : T \quad
    \Gamma \vdash F(P[e/\texttt{self}], L) \quad
    L \text{ dominates current location}
}{
    \Gamma \vdash e : T \text{ where } \{P\}
}$$

**(T-Refine-Elim)**

$$\frac{
    \Gamma \vdash e : T \text{ where } \{P\}
}{
    \Gamma \vdash e : T
}$$

**Subtyping Rules**

$$\Gamma \vdash (T \text{ where } \{P\}) <: T$$

$$\frac{
    \Gamma \vdash P \implies Q
}{
    \Gamma \vdash (T \text{ where } \{P\}) <: (T \text{ where } \{Q\})
}$$

**Nested Refinements**

$$(T \text{ where } \{P\}) \text{ where } \{Q\} \equiv T \text{ where } \{P \land Q\}$$

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
3. If verification fails and not in `[[dynamic]]` scope: ill-formed (`E-TYP-1953`)
4. If verification fails in `[[dynamic]]` scope: runtime check is generated

**Dynamic Semantics**

**Representation**

$$\text{sizeof}(T \text{ where } \{P\}) = \text{sizeof}(T)$$

$$\text{alignof}(T \text{ where } \{P\}) = \text{alignof}(T)$$

The predicate is a compile-time and runtime constraint only; it does not affect physical representation.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-TYP-1954`–`E-TYP-1957`.


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

1. **Path-specificity:** A key to path $P$ grants access only to $P$ and paths for which $P$ is a prefix.
2. **Implicit acquisition:** Accessing a `shared` path logically acquires the necessary key.
3. **Scoped lifetime:** Keys are valid for a bounded lexical scope and become invalid when that scope exits.
4. **Reentrancy:** If a key covering path $P$ is already held, nested access to $P$ or any path prefixed by $P$ succeeds without conflict.
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
$$\frac{
    \Gamma \vdash e : \texttt{shared}\ T \quad
    \text{ReadContext}(e)
}{
    \text{RequiredMode}(e) = \text{Read}
}$$

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
$$\frac{
    \Gamma \vdash e : \texttt{shared}\ T \quad
    \text{WriteContext}(e)
}{
    \text{RequiredMode}(e) = \text{Write}
}$$

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

$$\text{Read} < \text{Write}$$

A held mode is sufficient for a required mode when:

$$\text{ModeSufficient}(M_{\text{held}}, M_{\text{required}}) \iff M_{\text{required}} \leq M_{\text{held}}$$

**Constraints**

**Diagnostics:** See Appendix A, code `E-CON-0005`.


#### 17.1.5 Key State Context

**Key State Context.** A mapping $\Gamma_{\text{keys}} : \text{ProgramPoint} \to \mathcal{P}(\text{Key})$ that associates each program point with the set of keys logically held at that point.

**Static Semantics**

**Key Set Operations**

Let $\Gamma_{\text{keys}}$ be the current key set, a collection of $(P, M, S)$ triples.

**(Acquire)**
$$\text{Acquire}(P, M, S, \Gamma_{\text{keys}}) = \Gamma_{\text{keys}} \cup \{(P, M, S)\}$$

**(Release)**
$$\text{Release}(P, \Gamma_{\text{keys}}) = \Gamma_{\text{keys}} \setminus \{(P, M, S) : (P, M, S) \in \Gamma_{\text{keys}}\}$$

**(Release by Scope)**
$$\text{ReleaseScope}(S, \Gamma_{\text{keys}}) = \Gamma_{\text{keys}} \setminus \{(P, M, S') : S' = S\}$$

**(Mode Transition)**
$$\text{ModeTransition}(P, M_{\text{new}}, \Gamma_{\text{keys}}) = (\Gamma_{\text{keys}} \setminus \{(P, M_{\text{old}}, S)\}) \cup \{(P, M_{\text{new}}, S)\}$$

**Panic Release Semantics**

$$\text{PanicRelease}(S, \Gamma_{\text{keys}}) = \Gamma_{\text{keys}} \setminus \{(P, M, S') : S' \leq_{\text{nest}} S\}$$

All keys held by the panicking scope and its nested scopes are released atomically before panic unwinding proceeds.


#### 17.1.6 Held Predicate

**Held Predicate.** A key is held at a program point if it is a member of the key state context at that point: $\text{Held}(P, M, S, \Gamma_{\text{keys}}, p) \iff (P, M, S) \in \Gamma_{\text{keys}}(p)$.

**Static Semantics**

**Key Compatibility**

Two keys $K_1 = (P_1, M_1, S_1)$ and $K_2 = (P_2, M_2, S_2)$ are **compatible** if and only if:

$$\text{Compatible}(K_1, K_2) \iff \text{Disjoint}(P_1, P_2) \lor (M_1 = \text{Read} \land M_2 = \text{Read})$$

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

$$\text{Blocked}(t, K) \land \text{Held}(K, t') \land \Diamond\text{Released}(K, t') \implies \Diamond\text{Acquired}(K, t)$$


#### 17.1.7 Key Roots and Key Path Formation

**Key Root.** The base storage location from which a key path is derived. Key roots arise from: (1) **Lexical roots**: identifiers bound in the current scope; (2) **Boundary roots**: runtime object identities created at key boundaries (pointer indirection and type-level boundaries). For any place expression $e$ with `shared` permission, the **key path** $\text{KeyPath}(e)$ is the normalized `key_path_expr` used for key acquisition and conflict analysis.

**Static Semantics**

**Path Root Extraction**

Define $\text{Root}(e)$ for place expressions (§3.4.2) recursively:

$$\text{Root}(e) ::= \begin{cases}
x & \text{if } e = x \\
\text{Root}(e') & \text{if } e = e'.f \\
\text{Root}(e') & \text{if } e = e'[i] \\
\text{Root}(e') & \text{if } e = e' \mathord{\sim>} m(\ldots) \\
\bot_{\text{boundary}} & \text{if } e = (*e')
\end{cases}$$

where $\bot_{\text{boundary}}$ indicates a **key boundary** introduced by pointer dereference.

**Object Identity**

The **identity** of a reference or pointer $r$, written $\text{id}(r)$, is a unique runtime value denoting the storage location referred to by $r$.

Implementations MUST ensure:
1. **Uniqueness:** $\text{id}(r_1) = \text{id}(r_2)$ iff $r_1$ and $r_2$ refer to overlapping storage.
2. **Stability:** $\text{id}(r)$ remains constant for the lifetime of the referent.
3. **Opacity:** identities are not directly observable except through key semantics.

**KeyPath Formation**

Let $e$ be a place expression accessing `shared` data and let $e$’s field/index tail be $p_2.\ldots.p_n$.

- **Lexical root case:** If $\text{Root}(e) = x$, then

  $$\text{KeyPath}(e) = x.p_2.\ldots.p_n\ \text{truncated by any type-level boundary}.$$

- **Boundary root case:** If $\text{Root}(e) = \bot_{\text{boundary}}$ and $e = (*e').p_2.\ldots.p_n$, then

  $$\text{KeyPath}(e) = \text{id}(*e').p_2.\ldots.p_n\ \text{truncated by any type-level boundary}.$$

Type-level boundaries are defined in §17.2.4.

**Pointer Indirection**

A pointer dereference establishes a new key boundary. Key paths do not extend across pointer indirections.

For `(*e').p` where `e' : shared Ptr<T>@Valid`:
1. Dereferencing `e'` requires a key to $\text{KeyPath}(e')$ in Read mode.
2. Accessing `.p` on the dereferenced value uses a fresh key rooted at $\text{id}(*e')$.

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

An access to path $Q$ requiring mode $M_Q$ is covered by the current key state if:

$$\text{Covered}(Q, M_Q, \Gamma_{\text{keys}}) \iff \exists (P, M_P, S) \in \Gamma_{\text{keys}} : \text{Prefix}(P, Q) \land \text{ModeSufficient}(M_P, M_Q)$$

**Acquisition Rules**

**(K-Acquire-New)**
$$\frac{
    \Gamma \vdash P : \texttt{shared}\ T \quad
    M = \text{RequiredMode}(P) \quad
    \neg\text{Covered}(P, M, \Gamma_{\text{keys}}) \quad
    S = \text{CurrentScope}
}{
    \Gamma'_{\text{keys}} = \Gamma_{\text{keys}} \cup \{(P, M, S)\}
}$$

**(K-Acquire-Covered)**
$$\frac{
    \Gamma \vdash P : \texttt{shared}\ T \quad
    M = \text{RequiredMode}(P) \quad
    \text{Covered}(P, M, \Gamma_{\text{keys}})
}{
    \Gamma'_{\text{keys}} = \Gamma_{\text{keys}}
}$$

**Evaluation Order**

Subexpressions are evaluated left-to-right, depth-first. Key acquisition follows evaluation order.

**(K-Eval-Order)**
$$\text{eval}(e_1 \oplus e_2) \longrightarrow \text{eval}(e_1);\ \text{eval}(e_2);\ \text{apply}(\oplus)$$

**Constraints**

**Diagnostics:** See Appendix A, codes `W-CON-0001`, `W-CON-0002`.


#### 17.2.2 Key Release

**Key Release.** Keys are released when their defining scope exits.

**Static Semantics**

**(K-Release-Scope)**
$$\frac{
    \text{ScopeExit}(S)
}{
    \Gamma'_{\text{keys}} = \Gamma_{\text{keys}} \setminus \{(P, M, S') : S' = S\}
}$$

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
$$\frac{
    \Gamma \vdash P_1, \ldots, P_m : \texttt{shared}\ T_i \quad
    M = \text{BlockMode}(B) \quad
    (Q_1, \ldots, Q_m) = \text{CanonicalSort}(P_1, \ldots, P_m) \quad
    S = \text{NewScope}
}{
    \Gamma_{\text{keys}}' = \Gamma_{\text{keys}} \cup \{(Q_i, M, S) : i \in 1..m\}
}$$

**(K-Read-Block-No-Write)**
$$\frac{
    \text{BlockMode}(B) = \text{Read} \quad
    P \in \text{KeyedPaths}(B) \quad
    \text{WriteOf}(P) \in \text{Body}(B)
}{
    \text{Emit}(\texttt{E-CON-0070})
}$$

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
$$\frac{
    P = p_1.\ldots.p_{k-1}.\#p_k.\ldots.p_n \quad
    \Gamma \vdash P : \texttt{shared}\ T \quad
    M = \text{RequiredMode}(P)
}{
    \text{AcquireKey}(p_1.\ldots.p_k, M, \Gamma_{\text{keys}})
}$$

**Type-Level Key Boundary**

A field declared with `#` establishes a permanent key boundary:

```ebnf
key_field_decl      ::= "#"? visibility? identifier ":" type
```

**(K-Type-Boundary)**
$$\frac{
    \Gamma \vdash r : R \quad
    R.\text{fields} \ni (\#f : U) \quad
    P = r.f.q_1.\ldots.q_n
}{
    \text{KeyPath}(P) = r.f
}$$

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CON-0033`, `W-CON-0013`.


#### 17.2.5 Closure Capture of `shared` Bindings

**Shared Closure Capture.** Closures that capture `shared` bindings participate in key analysis. Because a closure may be invoked outside its defining scope, key path roots depend on whether the closure escapes.

**Classification**

A closure expression $C$ is **local** if it appears only in:
1. Argument position of an immediate procedure or method call
2. The body of a `spawn` expression
3. The body of a `dispatch` expression
4. Operand of immediate invocation

A closure is **escaping** if it is:
1. Bound to a `let` or `var` binding
2. Returned from a procedure
3. Stored into a record or enum field

Let $\text{SharedCaptures}(C)$ be the set of captured bindings with `shared` permission.

**Static Semantics**

**Local Closure Key Paths**

For a local closure, key analysis treats the closure body as if it executed in the defining scope. Any access `x.p` where `x ∈ SharedCaptures(C)` uses lexical rooting (§17.1.7):

$$\text{KeyPath}(C, x.p) = \text{KeyPath}(x.p)$$

Keys are acquired at invocation, not at definition.

**Escaping Closure Type Requirement**

An escaping closure that captures `shared` bindings MUST carry a shared dependency set in its closure type (§12.4.3):

**(K-Closure-Escape-Type)**
$$\frac{
    C\ \text{is escaping} \quad
    \text{SharedCaptures}(C) = \{x_1, \ldots, x_n\}
}{
    \text{Type}(C) = |\vec{T}| \to R\ [\texttt{shared}: \{x_1 : \texttt{shared}\ T_1, \ldots, x_n : \texttt{shared}\ T_n\}]
}$$

The dependency set MAY be inferred when the closure is assigned to a binding with an expected closure type; otherwise it MUST be written explicitly.

**Escaping Closure Key Paths**

For an escaping closure, key paths are rooted at runtime identities of captured references:

**(K-Closure-Escape-Keys)**
$$\frac{
    C : |\vec{T}| \to R\ [\texttt{shared}: \{x : \texttt{shared}\ T\}] \quad
    \text{Access}(x.p, M) \in C.\text{body}
}{
    \text{KeyPath}(C, x.p) = \text{id}(C.x).p \quad \text{KeyMode} = M
}$$

where $C.x$ denotes the captured reference to $x$ stored in the closure environment.

**Lifetime Constraint**

An escaping closure MUST NOT outlive any captured `shared` binding. Implementations MUST reject any flow where the closure’s lifetime exceeds the lifetime of a captured `shared` root.

**Dynamic Semantics**

**Local Closure Invocation**

1. Determine accessed `shared` paths in the closure body.
2. Acquire required keys using lexical roots.
3. Execute the closure body.
4. Release keys at the end of the invocation.

**Escaping Closure Invocation**

1. For each dependency $(x : shared\ T)$ in the closure type, let $r = C.x$.
2. Acquire required keys for paths rooted at $\text{id}(r)$.
3. Execute the closure body.
4. Release keys at the end of the invocation.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CON-0085`, `E-CON-0086`, `W-CON-0009`.


#### 17.2.6 `shared` Dynamic Class Objects (`shared $Class`)

**Shared Dynamic Class.** Dynamic class objects (`$Cl`, §13.5) erase the concrete type at runtime. When such an object is viewed with `shared` permission, key analysis cannot inspect the concrete method bodies chosen by vtable dispatch. Therefore, `shared $Cl` is restricted to read-only dynamic interfaces.

**Static Semantics**

**Well-Formedness**

A dynamic class object type may be qualified with `shared` permission only if every vtable-eligible procedure in the class (including inherited ones) has a `const` receiver (`~`):

Let $\text{DynMethods}(Cl)$ denote the set of procedures callable via vtable dispatch on `$Cl` (i.e., vtable‑eligible and not marked `[[static_dispatch_only]]`).

**(K-Witness-Shared-WF)**
$$\frac{
    \forall m \in \text{DynMethods}(Cl).\ m.\text{receiver} = \texttt{\sim}
}{
    \Gamma \vdash \texttt{shared}\ \$Cl\ \text{wf}
}$$

If any method requires `shared` (`~%`) or `unique` (`~!`) receiver permission, `shared $Cl` is ill‑formed. Such methods may still exist on `Cl` if they are excluded from dynamic dispatch via `[[static_dispatch_only]]`.

**Dynamic Calls Through `shared $Cl`**

For a call `e~>m(args)` where `e : shared $Cl`, the key mode is Read and the key path is the root of `e`:

$$\text{KeyPath}(e~>m(\ldots)) = \text{Root}(e) \quad \text{KeyMode} = \text{Read}$$

**Constraints**

**Diagnostics:** See Appendix A, code `E-CON-0083`.


### 17.3 Conflict Detection


#### 17.3.1 Path Prefix and Disjointness

**Path Relations.** The prefix relation and disjointness relation on paths determine key coverage and static safety of concurrent accesses.

**Static Semantics**

**Path Prefix**

Path $P$ is a prefix of path $Q$ (written $\text{Prefix}(P, Q)$) if $P$ is an initial subsequence of $Q$:

$$\text{Prefix}(p_1.\ldots.p_m,\ q_1.\ldots.q_n) \iff m \leq n \land \forall i \in 1..m,\ p_i \equiv_{\text{seg}} q_i$$

**Path Disjointness**

Two paths $P$ and $Q$ are disjoint (written $\text{Disjoint}(P, Q)$) if neither is a prefix of the other:

$$\text{Disjoint}(P, Q) \iff \neg\text{Prefix}(P, Q) \land \neg\text{Prefix}(Q, P)$$

**Segment Equivalence**

$$p_i \equiv_{\text{seg}} q_i \iff \text{name}(p_i) = \text{name}(q_i) \land \text{IndexEquiv}(p_i, q_i)$$

**Index Expression Equivalence**

Two index expressions $e_1$ and $e_2$ are provably equivalent ($e_1 \equiv_{\text{idx}} e_2$) if:

1. Both are the same integer literal
2. Both are references to the same `const` binding
3. Both are references to the same generic const parameter
4. Both are references to the same variable binding in scope
5. Both normalize to the same canonical form under constant folding

**Disjointness for Static Safety**

**(K-Disjoint-Safe)**
$$\frac{
    \text{Disjoint}(P, Q)
}{
    \text{ConcurrentAccess}(P, Q) \text{ is statically safe}
}$$

**Prefix for Coverage**

**(K-Prefix-Coverage)**
$$\frac{
    \text{Prefix}(P, Q) \quad \text{Held}(P, M, \Gamma_{\text{keys}})
}{
    \text{Covers}((P, M), Q)
}$$


#### 17.3.2 Static Conflict Analysis

**Static Conflict Analysis.** When multiple dynamic indices access the same array within a single statement, they MUST be provably disjoint; otherwise, the program is ill-formed.

**Static Semantics**

**Provable Disjointness for Indices**

Two index expressions are provably disjoint ($\text{ProvablyDisjoint}(e_1, e_2)$) if:

1. **Static Disjointness:** Both are statically resolvable with different values
2. **Control Flow Facts:** A Verification Fact establishes $e_1 \neq e_2$
3. **Contract-Based:** A precondition asserts $e_1 \neq e_2$
4. **Refinement Types:** An index has a refinement type constraining its relationship
5. **Algebraic Offset:** Expressions share a common base but differ by constant offsets
6. **Dispatch Iteration:** Within a `dispatch` block, accesses indexed by the iteration variable are automatically disjoint
7. **Disjoint Loop Ranges:** Iteration variables from loops with non-overlapping ranges

**(K-Dynamic-Index-Conflict)**
$$\frac{
    P_1 = a[e_1] \quad P_2 = a[e_2] \quad
    \text{SameStatement}(P_1, P_2) \quad
    (\text{Dynamic}(e_1) \lor \text{Dynamic}(e_2)) \quad
    \neg\text{ProvablyDisjoint}(e_1, e_2)
}{
    \text{Emit}(\texttt{E-CON-0010})
}$$

**Constraints**

**Diagnostics:** See Appendix A, code `E-CON-0010`.


#### 17.3.3 Read-Then-Write Prohibition

**Read-Then-Write.** A pattern that occurs when a statement contains both a read and a write to the same `shared` path without a covering Write key.

**Static Semantics**

**Formal Definition**

$$\text{ReadThenWrite}(P, S) \iff \exists e_r, e_w \in \text{Subexpressions}(S) : \text{ReadsPath}(e_r, P) \land \text{WritesPath}(e_w, P)$$

**(K-Read-Write-Reject)**
$$\frac{
    \Gamma \vdash P : \texttt{shared}\ T \quad
    \text{ReadThenWrite}(P, S) \quad
    \neg\exists (Q, \text{Write}, S') \in \Gamma_{\text{keys}} : \text{Prefix}(Q, P)
}{
    \text{Emit}(\texttt{E-CON-0060})
}$$

**(K-RMW-Permitted)**
$$\frac{
    \Gamma \vdash P : \texttt{shared}\ T \quad
    \text{ReadThenWrite}(P, S) \quad
    \exists (Q, \text{Write}, S') \in \Gamma_{\text{keys}} : \text{Prefix}(Q, P)
}{
    \text{Permitted}
}$$

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
$$\frac{
    \text{Held}(P, M_{\text{outer}}, \Gamma_{\text{keys}}) \quad
    \#P\ M_{\text{inner}}\ \{\ \ldots\ \}
}{
    \begin{cases}
    \text{Covered (no acquisition)} & \text{if } M_{\text{inner}} = M_{\text{outer}} \\
    \text{Release-and-Reacquire per §17.4.1} & \text{if } M_{\text{inner}} \neq M_{\text{outer}} \land \texttt{release} \in \text{modifiers} \\
    \text{Emit}(\texttt{E-CON-0012}) & \text{if } M_{\text{inner}} \neq M_{\text{outer}} \land \texttt{release} \notin \text{modifiers}
    \end{cases}
}$$

**Dynamic Semantics**

**Release-and-Reacquire Sequence**

When entering `#path release <target> { body }`:

1. **Release:** Release the outer key held by the enclosing block
2. **Acquire:** Acquire the target mode key to `path` (blocking if contended)
3. **Execute:** Evaluate `body`
4. **Release:** Release the inner key
5. **Reacquire:** Acquire the outer mode key for the enclosing block's remaining scope

**(K-Release-Sequence)**
$$\frac{
    \text{Held}(P, M_{\text{outer}}, S_{\text{outer}}) \quad
    \#P\ \texttt{release}\ M_{\text{inner}}\ \{\ B\ \}
}{
    \begin{array}{l}
    \text{Release}(P, \Gamma_{\text{keys}});\
    \text{Acquire}(P, M_{\text{inner}}, S_{\text{inner}});\
    \text{Eval}(B);\ \\
    \text{Release}(P, \Gamma_{\text{keys}});\
    \text{Acquire}(P, M_{\text{outer}}, S_{\text{outer}})
    \end{array}
}$$

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
$$\frac{
    \text{Held}(P, M, \Gamma_{\text{keys}}) \quad
    \text{Prefix}(P, Q) \quad
    \text{CalleeAccesses}(Q)
}{
    \text{CalleeCovered}(Q)
}$$

**(K-Procedure-Boundary)**

Passing a `shared` value as a procedure argument does not constitute key acquisition:

$$\frac{
    \Gamma \vdash f : (\texttt{shared}\ T) \to R \quad
    \Gamma \vdash v : \texttt{shared}\ T
}{
    \text{call}(f, v) \longrightarrow \text{no key acquisition at call site}
}$$

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
$$\frac{
    \#P\ \texttt{speculative}\ M\ \{B\} \quad M \neq \texttt{write}
}{
    \text{Emit}(\texttt{E-CON-0095})
}$$

**(K-Spec-Pure-Body)**
$$\frac{
    \#P\ \texttt{speculative write}\ \{B\} \quad
    \text{Writes}(B) \not\subseteq \text{CoveredPaths}(P)
}{
    \text{Emit}(\texttt{E-CON-0091})
}$$

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
$$\frac{
    \#P\ \texttt{speculative write}\ \{B\} \quad
    \#Q\ \_\ \{\ldots\} \in \text{Subexpressions}(B)
}{
    \text{Emit}(\texttt{E-CON-0090})
}$$


#### 17.5.2 Snapshot-Execute-Commit Cycle

**Speculative Commit.** $\text{SpeculativeCommit}(R, W) \iff \text{Atomic}\left(\bigwedge_{(p,v) \in R} p = v \implies \bigwedge_{(q,w) \in W} q := w\right)$

**Dynamic Semantics**

**Execution Model**

1. **Initialize**: Set $\text{retries} := 0$
2. **Snapshot**: Read all paths in the keyed set, recording $(path, value)$ pairs in read set $R$
3. **Execute**: Evaluate $body$, collecting writes in write set $W$
4. **Commit**: Atomically verify $\bigwedge_{(p,v) \in R} (\text{current}(p) = v)$ and, if true, apply all writes in $W$
5. **On success**: Return the value of $body$
6. **On failure**: Increment $\text{retries}$; if below limit, goto step 2; otherwise fallback
7. **Fallback**: Acquire a blocking Write key, execute $body$, release key, return value

**Retry Limit**

$\text{MAX\_SPECULATIVE\_RETRIES}$ is IDB.

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

1. The write set $W$ is discarded (no writes are committed)
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
$$\frac{
    \text{Access}(P, M) \quad
    \text{StaticallySafe}(P)
}{
    \text{NoRuntimeSync}(P)
}$$

**(K-Static-Required)**
$$\frac{
    \neg\text{StaticallySafe}(P) \quad
    \neg\text{InDynamicContext}
}{
    \text{Emit}(\texttt{E-CON-0020})
}$$

**(K-Dynamic-Permitted)**
$$\frac{
    \neg\text{StaticallySafe}(P) \quad
    \text{InDynamicContext}
}{
    \text{EmitRuntimeSync}(P)
}$$

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

$$P <_{\text{canon}} Q \iff \exists k \geq 1 : (\forall i < k,\ p_i =_{\text{seg}} q_i) \land (p_k <_{\text{seg}} q_k \lor (k > m \land m < n))$$

**Segment Ordering**

1. **Identifiers:** Lexicographic by UTF-8 byte sequence
2. **Indexed segments:** By index value for statically resolvable indices
3. **Bare vs indexed:** Bare identifier precedes any indexed form

**Dynamic Ordering Guarantee**

Within `[[dynamic]]` scope, incomparable dynamic indices require runtime ordering satisfying:

1. **Totality**: For distinct paths $P$ and $Q$, exactly one of $\text{DynOrder}(P, Q)$ or $\text{DynOrder}(Q, P)$ holds
2. **Antisymmetry**: $\text{DynOrder}(P, Q) \land \text{DynOrder}(Q, P) \implies P = Q$
3. **Transitivity**: $\text{DynOrder}(P, Q) \land \text{DynOrder}(Q, R) \implies \text{DynOrder}(P, R)$
4. **Cross-Task Consistency**: All tasks compute the same ordering
5. **Value-Determinism**: Ordering depends only on runtime values, not timing

**Observational Equivalence**

$$\forall P : \text{Observable}(\text{StaticSafe}(P)) = \text{Observable}(\text{RuntimeSync}(P))$$

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

**Parallel Block.** A scope within which work executes concurrently across multiple workers. A parallel block $P$ is defined as $P = (D, A, B)$ where $D$ is the execution domain expression, $A$ is the set of block options, and $B$ is the block body.

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

$$\frac{
  \Gamma \vdash D : \text{\$ExecutionDomain} \quad
  \Gamma_P = \Gamma[\text{parallel\_context} \mapsto D] \quad
  \Gamma_P \vdash B : T
}{
  \Gamma \vdash \texttt{parallel } D\ \{B\} : T
} \quad \text{(T-Parallel)}$$

**Well-Formedness**

A parallel block is well-formed if and only if:

1. The domain expression evaluates to a type implementing `ExecutionDomain`.
2. All `spawn` and `dispatch` expressions within the block body reference the enclosing parallel block.
3. No `spawn` or `dispatch` expression appears outside a parallel block.
4. All captured bindings satisfy the permission requirements of §18.3.

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CON-0101`–`E-CON-0103`.


#### 18.1.2 Fork-Join Semantics

**Structured Concurrency Invariant.** Let $\mathcal{W}$ denote the set of work items spawned within a parallel block $P$: $\forall w \in \mathcal{W}.\ \text{lifetime}(w) \subseteq \text{lifetime}(P)$. A **work item** is a unit of computation queued for execution by a worker (created by `spawn` and `dispatch` expressions). A **worker** is an execution context that executes work items. A **task** is the runtime representation of a work item during execution; tasks may suspend and resume, and are associated with key state per §17.1.

**Dynamic Semantics**

Evaluation of `parallel D [opts] { B }`:

1. Evaluate domain expression $D$ to obtain $d$.
2. Initialize the worker pool according to $d$'s configuration.
3. If the `cancel` option is present, associate its token with the block.
4. Evaluate statements in $B$ sequentially; `spawn` and `dispatch` expressions enqueue work items.
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
| Single `spawn { e }` where $e : T$ | $T$                  |
| Multiple spawns $e_1, \ldots, e_n$ | $(T_1, \ldots, T_n)$ |
| `dispatch` without `[reduce]`      | `()`                 |
| `dispatch ... [reduce: op] { e }`  | $T$ where $e : T$    |
| Mixed spawns and reducing dispatch | Tuple of all results |

**Typing Rules**

$$\frac{
  \Gamma \vdash \texttt{parallel } D\ \{\}\ : ()
}{} \quad \text{(T-Parallel-Empty)}$$

$$\frac{
  \Gamma \vdash \texttt{spawn } \{e\} : \text{SpawnHandle}\langle T \rangle
}{
  \Gamma \vdash \texttt{parallel } D\ \{\texttt{spawn } \{e\}\} : T
} \quad \text{(T-Parallel-Single)}$$

$$\frac{
  \Gamma \vdash \texttt{spawn } \{e_i\} : \text{SpawnHandle}\langle T_i \rangle \quad \forall i \in 1..n
}{
  \Gamma \vdash \texttt{parallel } D\ \{\texttt{spawn } \{e_1\}; \ldots; \texttt{spawn } \{e_n\}\} : (T_1, \ldots, T_n)
} \quad \text{(T-Parallel-Multi)}$$


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

- `spawn { e }` evaluates $e$ immediately and blocks until complete.
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

$$\frac{
  \Gamma[\text{parallel\_context}] = D \quad
  \Gamma_{\text{capture}} \vdash e : T
}{
  \Gamma \vdash \texttt{spawn}\ \{e\} : \text{SpawnHandle}\langle T \rangle
} \quad \text{(T-Spawn)}$$

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
2. Package the captured environment with expression $e$ into a work item.
3. Enqueue the work item to the parallel block's worker pool.
4. Return `SpawnHandle<T>@Pending` immediately (non-blocking).
5. A worker eventually dequeues and evaluates $e$.
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

$$\frac{
  \Gamma \vdash \text{range} : \text{Range}\langle I \rangle \quad
  \Gamma, i : I \vdash B : T
}{
  \Gamma \vdash \texttt{dispatch } i \texttt{ in range } \{B\} : ()
} \quad \text{(T-Dispatch)}$$

**Typing Rule (With Reduction)**

$$\frac{
  \Gamma \vdash \text{range} : \text{Range}\langle I \rangle \quad
  \Gamma, i : I \vdash B : T \quad
  \Gamma \vdash \oplus : (T, T) \to T
}{
  \Gamma \vdash \texttt{dispatch } i \texttt{ in range [reduce: } \oplus \texttt{] } \{B\} : T
} \quad \text{(T-Dispatch-Reduce)}$$

**Constraints**

**Diagnostics:** See Appendix A, codes `E-CON-0140`–`E-CON-0143`, `W-CON-0140`.


#### 18.5.2 Iteration Space

**Dynamic Semantics**

Evaluation of `dispatch i in range [attrs] { B }`:

1. Evaluate `range` to determine iteration count $n$.
2. Analyze key patterns to partition iterations into conflict-free groups.
3. For each group, enqueue iterations as work items to the worker pool.
4. Workers execute iterations, acquiring keys as needed per §17.2.
5. If `[reduce: op]` is present, combine partial results using `op`.
6. Block until all iterations complete.
7. Return the reduced value (if reduction) or unit.

**Reduction Semantics**

Reduction operators MUST be associative:

$$\forall a, b, c.\ (a \oplus b) \oplus c = a \oplus (b \oplus c)$$

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

$$\frac{
  \texttt{dispatch}\ v\ \texttt{in}\ r\ \{\ \ldots\ a[v]\ \ldots\ \}
}{
  \forall v_1, v_2 \in r,\ v_1 \neq v_2 \implies \text{ProvablyDisjoint}(a[v_1], a[v_2])
} \quad \text{(K-Disjoint-Dispatch)}$$

**Parallelism Determination**

| Key Pattern   | Keys Generated      | Parallelism Degree    |
| :------------ | :------------------ | :-------------------- |
| `data[i]`     | $n$ distinct keys   | Full parallel         |
| `data[i / 2]` | $n/2$ distinct keys | Pairs serialize       |
| `data[i % k]` | $k$ distinct keys   | $k$-way parallel      |
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

