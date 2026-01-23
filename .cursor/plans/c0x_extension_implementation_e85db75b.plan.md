---
name: C0X Extension Implementation
overview: Implement the C0X extension features (shared permission, key system, generics, full classes, and contracts) into the cursive0 bootstrap compiler, and amend the Cursive0.md specification document with fully formalized definitions.
todos:
  - id: phase1-shared
    content: "Phase 1: Enable shared permission in type system, implement downgrade state machine, update subtyping"
    status: completed
  - id: phase2-keys
    content: "Phase 2: Implement Key System - AST nodes, parser, key context, conflict detection, dynamic verification"
    status: completed
  - id: phase3-generics
    content: "Phase 3: Implement Generics - parser, type parameter environment, where clauses, variance, monomorphization"
    status: completed
  - id: phase4-classes
    content: "Phase 4: Implement Full Classes - associated types, abstract states, dispatchability, coherence rules"
    status: completed
  - id: phase5-contracts
    content: "Phase 5: Implement Contract System - parser, type checking, verification logic, dynamic checks, invariants"
    status: completed
  - id: phase6-attrs
    content: "Phase 6: Implement Attributes System - parser, registry, key attributes like [[dynamic]]"
    status: completed
  - id: phase7-imports
    content: "Phase 7: Implement Module Importing - import declarations, extended using variants"
    status: completed
  - id: phase8-spec
    content: "Phase 8: Amend Cursive0.md specification with all new features, typing rules, and diagnostics"
    status: in_progress
isProject: false
---

# C0X Extension Implementation Plan

This plan implements the shared permission, key system, generics, full classes, and contract systems into the cursive0 bootstrap compiler as defined in [C0X_Extension.md](cursive-bootstrap/docs/C0X_Extension.md) and the original [Cursive.md](Cursive-Language-Specification/Cursive.md) specification.

---

## Phase 1: Foundation - Shared Permission System

The shared permission enables the Key System and is foundational for concurrent data access.

### 1.1 Enable Shared Permission in Type System

**Files to modify:**

- [src/analysis/types/conformance.cpp](cursive-bootstrap/src/analysis/types/conformance.cpp) - Remove `CheckC0SubsetPermTokens()` rejection of `shared`/`~%`
- [src/analysis/types/types.cpp](cursive-bootstrap/src/analysis/types/types.cpp) - Update `IsPermInC0()` to return `true` for `Shared`
- [src/syntax/parser_types.cpp](cursive-bootstrap/src/syntax/parser_types.cpp) - Remove `Parse-Type-Generic-Unsupported` rejection for shared

**Implementation:**

- Permission lattice: `unique <: shared <: const`
- Subtyping rule `(T-Perm-Sub)`: `p T <: q T` when `p PermSub q`
- Receiver shorthand: `~%` abbreviates `self: shared Self`

### 1.2 Implement Unique Downgrade State Machine

**Files to modify:**

- [src/analysis/memory/borrow_bind.cpp](cursive-bootstrap/src/analysis/memory/borrow_bind.cpp) - Add `Active`/`Inactive` binding states

**Implementation:**

- Track binding states: `Active(b)` (no downgraded references) and `Inactive(b)` (during downgrade)
- While `Inactive(b)`: binding MUST NOT be read, written, moved, or re-downgraded
- State transitions at downgrade/restore points

### 1.3 Update Subtyping for Permissions

**Files to modify:**

- [src/analysis/types/subtyping.cpp](cursive-bootstrap/src/analysis/types/subtyping.cpp) - Add shared permission subtyping

**Implementation:**

- `unique` callers may call `~`, `~%`, and `~!` receivers
- `shared` callers may call `~` and `~%` receivers
- `const` callers may call only `~` receivers

---

## Phase 2: Key System

The Key System provides compile-time verification of concurrent access to `shared` data.

### 2.1 AST Nodes for Key System

**Files to modify:**

- [src/syntax/ast.h](cursive-bootstrap/src/syntax/ast.h) (or equivalent) - Add Key System AST nodes

**Add AST nodes:**

```cpp
KeyMode = { Read, Write }
KeyBlockMod = { Dynamic, Speculative, Release }
KeySeg = Field(marked: bool, name: Ident) | Index(marked: bool, expr: Expr)
KeyPathExpr = { root: Ident, segs: [KeySeg] }
KeyBlockStmt = { paths: [KeyPathExpr], mods: [KeyBlockMod], mode: KeyMode?, body: BlockExpr }
```

### 2.2 Parser for Key Blocks and Paths

**Files to modify:**

- [src/syntax/parser_stmt.cpp](cursive-bootstrap/src/syntax/parser_stmt.cpp) - Parse key block statements
- [src/syntax/parser_expr.cpp](cursive-bootstrap/src/syntax/parser_expr.cpp) - Parse key path expressions

**Grammar (from C0X spec):**

```ebnf
key_block_stmt ::= "#" key_path_list key_block_mod* key_mode? block_expr
key_path_list  ::= key_path_expr ("," key_path_expr)*
key_block_mod  ::= "dynamic" | "speculative" | "release"
key_mode       ::= "read" | "write"
key_path_expr  ::= key_root key_seg*
key_seg        ::= ("." key_field) | ("[" key_index "]")
key_field      ::= key_marker? identifier
key_marker     ::= "#"
```

### 2.3 Type-Level Boundaries

**Files to modify:**

- [src/syntax/parser_items.cpp](cursive-bootstrap/src/syntax/parser_items.cpp) - Parse `#` boundary marker on fields
- [src/analysis/composite/records.cpp](cursive-bootstrap/src/analysis/composite/records.cpp) - Store boundary flag

**Implementation:**

- Record/enum/modal field declarations extended with optional `#` boundary marker
- When deriving key paths, traversal stops at boundary fields

### 2.4 Key Context and Conflict Detection

**New files:**

- `src/analysis/keys/key_context.cpp` - Key context management
- `src/analysis/keys/key_conflict.cpp` - Conflict detection

**Implementation:**

- Key context `Γ_keys`: set of held keys `(P, M, S)`
- `Covers(Γ_keys, (P, M))`: check if path is covered by held keys
- `Conflict((P₁, M₁), (P₂, M₂))`: true if `Overlap(P₁, P₂) ∧ (M₁ = Write ∨ M₂ = Write)`
- Path prefix and disjointness checking
- Index equivalence for static disjointness proofs

### 2.5 Key Acquisition Semantics

**Files to modify:**

- [src/analysis/types/type_stmt.cpp](cursive-bootstrap/src/analysis/types/type_stmt.cpp) - Type-check key blocks

**Implementation:**

- `(K-Acquire-New)`: Acquire key if not covered
- `(K-Acquire-Covered)`: No acquisition if already covered
- Canonical acquisition order (lexicographic) to avoid deadlock
- Nested key block handling with release semantics

### 2.6 Dynamic Key Verification

**Files to modify:**

- [src/codegen/lower/lower_stmt.cpp](cursive-bootstrap/src/codegen/lower/lower_stmt.cpp) - Lower dynamic key operations

**Implementation:**

- In `[[dynamic]]` scope: emit runtime synchronization
- Memory ordering attributes: `[[relaxed]]`, `[[acquire]]`, `[[release]]`, `[[acqrel]]`, `[[seqcst]]`
- Default ordering: `[[acqrel]]`

---

## Phase 3: Generics

Generics via monomorphization (compile-time specialization).

### 3.1 Generic Parameter AST and Parsing

**Files to modify:**

- [src/syntax/parser_items.cpp](cursive-bootstrap/src/syntax/parser_items.cpp) - Remove rejection, parse generic params
- [src/syntax/parser_types.cpp](cursive-bootstrap/src/syntax/parser_types.cpp) - Parse generic type arguments

**Grammar:**

```ebnf
generic_params     ::= "<" type_param (";" type_param)* ">"
type_param         ::= identifier type_bounds? type_default?
type_bounds        ::= "<:" bound_list
type_default       ::= "=" type
generic_args       ::= "<" type_arg_list ">"
qualified_type_expr ::= type_path ("::" generic_args)?  // turbofish
```

**AST nodes:**

```cpp
TypeParam = { name: Ident, bounds: [ClassBound], default: Type?, variance: Variance }
GenericParams = [TypeParam]
GenericArgs = [Type]
Variance = { Covariant, Contravariant, Invariant, Bivariant }
```

### 3.2 Where Clauses

**Files to modify:**

- [src/syntax/parser_items.cpp](cursive-bootstrap/src/syntax/parser_items.cpp) - Remove `RejectWhereClause()`, parse where clauses

**Grammar:**

```ebnf
where_clause        ::= "where" where_predicate_list
where_predicate     ::= identifier type_bounds
predicate_separator ::= terminator
```

### 3.3 Type Parameter Environment

**Files to modify:**

- [src/analysis/types/types.h](cursive-bootstrap/src/analysis/types/types.h) - Add generic type representation
- [src/analysis/types/types.cpp](cursive-bootstrap/src/analysis/types/types.cpp) - Type parameter environment Δ

**Implementation:**

- Type parameter environment `Δ`: maps parameter names to `(bounds, default, variance)`
- `(WF-GenParams)`: Well-formedness of generic parameter lists
- `(WF-GenArgs)`: Well-formedness of instantiation arguments

### 3.4 Bound Satisfaction

**Files to modify:**

- [src/analysis/composite/classes.cpp](cursive-bootstrap/src/analysis/composite/classes.cpp) - Bound satisfaction checking

**Implementation:**

- `Implements(Γ, T, Cl)`: class implementation judgment
- `Γ ⊢ T ⊨ (Cl₁ + … + Clₙ)` iff `∀i. Implements(Γ, T, Clᵢ)`

### 3.5 Variance Computation

**New file:**

- `src/analysis/types/variance.cpp` - Variance computation

**Implementation:**

- `VarMut(N, X)`: variance through `shared/unique` access
- `VarConst(N, X)`: variance through `const` access
- Contribution rules for mutable/immutable storage positions, parameters, returns
- Generic subtyping rule `(T-Gen-Sub)`

### 3.6 Monomorphization

**New file:**

- `src/analysis/generics/monomorphize.cpp` - Monomorphization pass

**Implementation:**

- `Instantiate(d, θ)`: substitute type parameters in declaration
- Instantiation graph: track demanded instantiations `(d, θ)`
- Algorithm: Initialize with entry module uses, iterate until fixed point
- Termination check: reject non-terminating type-level recursion (IDB max depth: 128)

### 3.7 Type Inference for Generic Calls

**Files to modify:**

- [src/analysis/types/type_infer.cpp](cursive-bootstrap/src/analysis/types/type_infer.cpp) - Bidirectional inference for generics

**Implementation:**

- Infer type arguments from value argument types and expected return type
- Turbofish syntax for explicit instantiation: `Foo::<T>(...)`

---

## Phase 4: Full Classes

Extend classes with associated types, abstract states, and dispatchability rules.

### 4.1 Extended Class AST

**Files to modify:**

- [src/syntax/ast.h](cursive-bootstrap/src/syntax/ast.h) - Extended class items

**AST nodes:**

```cpp
ClassItem = ClassMethodDecl | AssociatedTypeDecl | AbstractFieldDecl | AbstractStateDecl
AssociatedTypeDecl = { vis: Vis, name: Ident, default: Type? }
AbstractFieldDecl = { vis: Vis, boundary: bool, name: Ident, ty: Type }
AbstractStateDecl = { vis: Vis, name: Ident, fields: [AbstractFieldDecl] }
ClassDecl = { 
  vis: Vis, modal: bool, name: Ident, generic_params: GenericParams?,
  superclass_bounds: [ClassBound], where_clause: WhereClause?,
  items: [ClassItem]
}
```

### 4.2 Parser for Full Class Syntax

**Files to modify:**

- [src/syntax/parser_items.cpp](cursive-bootstrap/src/syntax/parser_items.cpp) - Parse full class declarations

**Grammar:**

```ebnf
class_decl ::= visibility? "modal"? "class" identifier generic_params?
               ("<:" superclass_bounds)? where_clause? "{" class_item* "}"
class_item ::= class_method_decl | associated_type_decl | abstract_field_decl | abstract_state_decl
associated_type_decl ::= "type" identifier ("=" type)? terminator
abstract_state_decl  ::= "@" identifier "{" abstract_field_decl* "}" terminator?
```

### 4.3 Modal Classes and Abstract States

**Files to modify:**

- [src/analysis/composite/classes.cpp](cursive-bootstrap/src/analysis/composite/classes.cpp) - Modal class validation

**Implementation:**

- Modal class: class with abstract states (`St ≠ ∅`)
- Only modal types may implement modal classes
- `(T-Modal-Class)`: records/enums implementing modal classes → `E-TYP-2401`
- State compatibility checking for implementations

### 4.4 Associated Types

**Files to modify:**

- [src/analysis/composite/classes.cpp](cursive-bootstrap/src/analysis/composite/classes.cpp) - Associated type handling

**Implementation:**

- Abstract associated types: implementing types MUST provide binding
- Concrete associated types (with defaults): MAY be overridden
- Type alias resolution through class implementations

### 4.5 Object-Safety and Dispatchability

**Files to modify:**

- [src/analysis/composite/classes.cpp](cursive-bootstrap/src/analysis/composite/classes.cpp) - Dispatchability rules
- [src/codegen/dyn_dispatch.cpp](cursive-bootstrap/src/codegen/dyn_dispatch.cpp) - VTable generation updates

**Implementation:**

- VTable eligibility: has receiver, no generic params, no `Self` by value (except pointers)
- `dispatchable(Cl)`: all procedures vtable-eligible or `[[static_dispatch_only]]`
- `[[static_dispatch_only]]` attribute parsing and enforcement
- Error if non-vtable-eligible procedure called through `$Class`

### 4.6 Implementation Coherence

**Files to modify:**

- [src/analysis/composite/classes.cpp](cursive-bootstrap/src/analysis/composite/classes.cpp) - Coherence rules

**Implementation:**

- `(T-Impl-Complete)`: completeness check for all abstract members
- Orphan rule: at least one of `T` or `Cl` defined in current assembly
- Override rules: `override` keyword required for concrete procedure overrides

---

## Phase 5: Contract System

Procedure contracts, type invariants, loop invariants, and verification logic.

### 5.1 Contract AST Nodes

**Files to modify:**

- [src/syntax/ast.h](cursive-bootstrap/src/syntax/ast.h) - Contract AST nodes

**AST nodes:**

```cpp
ContractClause = { pre: Expr, post: Expr }
ContractIntrinsic = AtResult | AtEntry(expr: Expr)
Invariant = Expr
```

### 5.2 Contract Parser

**Files to modify:**

- [src/syntax/parser_items.cpp](cursive-bootstrap/src/syntax/parser_items.cpp) - Parse `|=` contract clauses
- [src/syntax/parser_expr.cpp](cursive-bootstrap/src/syntax/parser_expr.cpp) - Parse `@result`, `@entry()`

**Grammar:**

```ebnf
contract_clause    ::= "|=" contract_body
contract_body      ::= precondition_expr ("=>" postcondition_expr)?
                     | "=>" postcondition_expr
contract_intrinsic ::= "@result" | "@entry" "(" expression ")"
```

**Desugaring:**

- `|= P` → `pre = P, post = true`
- `|= P => Q` → `pre = P, post = Q`
- `|= => Q` → `pre = true, post = Q`

### 5.3 Type/Loop Invariant Parser

**Files to modify:**

- [src/syntax/parser_items.cpp](cursive-bootstrap/src/syntax/parser_items.cpp) - Type invariants
- [src/syntax/parser_expr.cpp](cursive-bootstrap/src/syntax/parser_expr.cpp) - Loop invariants

**Grammar:**

```ebnf
type_invariant ::= "where" "{" predicate_expr "}"
loop_invariant ::= "where" "{" predicate_expr "}"
```

### 5.4 Contract Type Checking

**New file:**

- `src/analysis/contracts/contract_check.cpp` - Contract verification

**Implementation:**

- `(WF-Contract)`: pure predicates, correct typing
- Precondition context `Γ_pre`: parameters, receiver, enclosing scope (no `@result`, `@entry`)
- Postcondition context `Γ_post`: add `@result`, `@entry()`, mutable params at post-state
- Purity checking: no capability params, no observable mutation

### 5.5 Verification Logic

**New file:**

- `src/analysis/contracts/verification.cpp` - Static verification

**Implementation:**

- `StaticProof(Γ_S, P)`: decidable predicate provability
- Entailment rules: `Ent-True`, `Ent-Fact`, `Ent-And`, `Ent-Or-L/R`, `Ent-Linear`
- Mandatory proof techniques: constant propagation, linear integer reasoning, boolean algebra, control flow analysis, type-derived bounds
- Verification facts: `F(P, L, S)` generation and dominance

### 5.6 Dynamic Contract Checks

**Files to modify:**

- [src/codegen/checks.cpp](cursive-bootstrap/src/codegen/checks.cpp) - Emit runtime checks
- [src/codegen/lower/lower_proc.cpp](cursive-bootstrap/src/codegen/lower/lower_proc.cpp) - @entry capture

**Implementation:**

- `[[dynamic]]` attribute: permit runtime verification
- `ContractCheck(P, k, s, ρ)` emission: `if !P[ρ] { panic(ContractViolation(k, P, s)) }`
- Entry capture semantics for `@entry()`: capture at procedure entry, Bitcopy/Clone

### 5.7 Type Invariants

**Files to modify:**

- [src/analysis/types/type_decls.cpp](cursive-bootstrap/src/analysis/types/type_decls.cpp) - Type invariant checking

**Implementation:**

- Enforcement points: post-construction, pre-call (public), post-call (mutator)
- Public field constraint: types with invariants MUST NOT have public mutable fields
- Private procedure exemption with public return verification

### 5.8 Loop Invariants

**Files to modify:**

- [src/analysis/types/type_stmt.cpp](cursive-bootstrap/src/analysis/types/type_stmt.cpp) - Loop invariant checking

**Implementation:**

- Enforcement points: initialization, maintenance, termination
- Verification fact generation at loop exit

### 5.9 Behavioral Subtyping

**Files to modify:**

- [src/analysis/composite/classes.cpp](cursive-bootstrap/src/analysis/composite/classes.cpp) - LSP checking

**Implementation:**

- Precondition weakening: impl precondition ≥ class precondition
- Postcondition strengthening: impl postcondition ≤ class postcondition
- Static verification at compile time; violations are structural errors

---

## Phase 6: Attributes System

### 6.1 Attribute Parser

**Files to modify:**

- [src/syntax/parser_items.cpp](cursive-bootstrap/src/syntax/parser_items.cpp) - Remove attribute rejection
- New: `src/syntax/parser_attrs.cpp` - Attribute parsing

**Grammar:**

```ebnf
attribute_list ::= attribute+
attribute      ::= "[[" attribute_spec ("," attribute_spec)* "]]"
attribute_spec ::= attribute_name ("(" attribute_args ")")?
```

### 6.2 Attribute Registry

**New file:**

- `src/analysis/attrs/attr_registry.cpp` - Attribute validation

**Implementation:**

- Registry `𝓡 = 𝓡_spec ⊎ 𝓡_vendor`
- Target validation per attribute
- Key attributes: `[[dynamic]]`, `[[static_dispatch_only]]`, memory-ordering attrs

---

## Phase 7: Module Importing

### 7.1 Import Declaration Parser

**Files to modify:**

- [src/syntax/parser_items.cpp](cursive-bootstrap/src/syntax/parser_items.cpp) - Parse import declarations

**Grammar:**

```ebnf
import_decl ::= visibility? "import" module_path ("as" identifier)? terminator
```

### 7.2 Extended Using Declarations

**Files to modify:**

- [src/syntax/parser_items.cpp](cursive-bootstrap/src/syntax/parser_items.cpp) - Full using variants

**Grammar:**

```ebnf
using_clause    ::= using_path ("as" identifier)?
                  | using_path "::" using_list
                  | using_path "::" "*"
using_list      ::= "{" using_specifier ("," using_specifier)* ","? "}"
using_specifier ::= identifier ("as" identifier)? | "self" ("as" identifier)?
```

### 7.3 Name Resolution Updates

**Files to modify:**

- [src/analysis/resolve/resolver_modules.cpp](cursive-bootstrap/src/analysis/resolve/resolver_modules.cpp) - Import/using resolution

---

## Phase 8: Specification Document Amendment

Amend [Cursive0.md](cursive-bootstrap/docs/Cursive0.md) with fully formalized content.

### 8.1 Sections to Add/Modify

**New sections to add:**

- Section 1.1.1: Update `S0` subset to include new constructs
- Section 5.X: "Shared Permission and Key System"
  - Permission lattice with `shared`
  - Key context and key blocks
  - Conflict detection rules
  - Type-level boundaries
  - Dynamic key verification
- Section 5.Y: "Generics"
  - Type parameter syntax and well-formedness
  - Where clauses
  - Bound satisfaction
  - Variance computation
  - Monomorphization algorithm
- Section 5.Z: "Full Classes"
  - Extended class syntax
  - Associated types
  - Abstract states (modal classes)
  - Dispatchability and vtable eligibility
  - Implementation coherence and orphan rules
- Section 5.W: "Contracts and Verification"
  - Contract clause syntax and well-formedness
  - Preconditions and postconditions
  - @result and @entry intrinsics
  - Type invariants
  - Loop invariants
  - Verification logic (StaticProof, entailment rules)
  - Dynamic contract checks
  - Behavioral subtyping
- Section 6.X: Codegen for generics (monomorphization)
- Section 6.Y: Codegen for key system (runtime locks)
- Section 6.Z: Codegen for contracts (runtime checks)

### 8.2 Diagnostic Code Additions

**New diagnostic codes to add to Appendix A:**

- `E-TYP-2401`: Modal class implemented by non-modal type
- `E-SEM-2801` through `E-SEM-2855`: Contract/verification errors
- `E-CON-0012`, `E-CON-0060`: Key system conflicts
- `P-SEM-2850`, `P-SEM-2860`, `P-SEM-2861`: Contract runtime panics

### 8.3 Notation and Helper Function Additions

**Add to Appendix B (Notation):**

- Key context `Γ_keys`
- Permission subtyping relation `PermSub`
- Variance computation functions
- Verification fact notation `F(P, L, S)`

---

## Implementation Order and Dependencies

```
Phase 1 (Shared Permission)
    │
    ├──► Phase 2 (Key System) ──► depends on shared permission
    │
    └──► Phase 3 (Generics) ──► independent of Key System
              │
              └──► Phase 4 (Full Classes) ──► depends on Generics
                        │
                        └──► Phase 5 (Contracts) ──► uses class constraints

Phase 6 (Attributes) ──► parallel, needed by Key System [[dynamic]]
Phase 7 (Imports) ──► independent, can be done early
Phase 8 (Spec Amendment) ──► ongoing throughout all phases
```

---

## Testing Strategy

For each phase:

1. Unit tests for parsing new syntax
2. Semantic analysis tests for type checking rules
3. Codegen tests for correct lowering
4. End-to-end tests from the test harness
5. Spec verification tests (oracle comparison)

Key test categories:

- Permission subtyping and coercion
- Key conflict detection (static and dynamic)
- Generic instantiation and monomorphization
- Class implementation completeness
- Contract verification (static proof and runtime checks)
- Behavioral subtyping violations