# Cursive Language Specification

## Clause 4 — Declarations

**Clause**: 4 — Declarations
**File**: 05-1_Declaration-Overview.md
**Section**: §4.1 Declaration Overview
**Stable label**: [decl.overview]  
**Forward references**: §2.5 [lex.units], Clause 3 [module], Clause 5 [name], Clause 6 [type], Clause 7 [expr], Clause 9 [generic], Clause 10 [memory], Clause 11 [contract], Clause 12 [witness]

---

### §4.1 Declaration Overview [decl.overview]

#### §4.1.1 Overview

[1] Clause 4 specifies the syntactic and semantic forms that introduce names in Cursive programs. A _declaration_ binds an identifier to a category (value, type, callable, predicate, contract, or grant), assigns visibility and storage attributes, and determines how later references participate in the permission, grant, and modal systems.

#### §4.1.2 Declaration Categories

[3] Every declaration belongs to exactly one of the following categories:

- **Variable bindings** (`let`, `var`, `shadow let`, `shadow var`) introduce bindings with explicit mutability and optional pattern destructuring.
- **Binding patterns** extend variable bindings with structured destructuring that binds multiple identifiers simultaneously.
- **Callable declarations** (`procedure`, `comptime procedure`, `extern procedure`) introduce named computations, including their parameter lists, contracts, and grant obligations.
- **Type declarations** (`record`, `enum`, `modal`, `type`) introduce nominal or transparent types into the unified namespace.
- **Visibility modifiers** (`public`, `internal`, `private`, `protected`) refine the accessibility of the associated declaration.
- **Auxiliary declarations** (behaviors, contracts, grants) rely on their dedicated clauses for semantics but use this clause for name introduction and visibility rules. Grant declarations are specified in §5.9 [decl.grant]; grant system semantics are specified in Clause 12 [contract].

[4] Declarations located at module scope participate in module initialisation ordering (§4.6) and contribute to the program entry model (§5.8). Declarations inside block scopes follow the storage-duration rules established in §5.7 and the scoping rules of Clause 6.

#### §5.1.3 Namespace and Scope Invariants

[5] Cursive maintains a single lexical identifier namespace per scope: within a given scope, types and values share the same identifier space. Redeclaration within that scope is ill-formed. Shadowing an outer-scope binding requires an explicit `shadow` keyword.

[6] Two-phase compilation (§2.2) guarantees that declarations may appear in any order within a translation unit. Parsing records the complete declaration inventory before semantic analysis, so forward references are resolved without separate stub declarations.

#### §5.1.4 Examples (Informative)

**Example 5.1.5.1 (Representative module declarations):**

```cursive
public record Account { id: u64, balance: i64 }

use bank::ledger::post_entry

let DEFAULT_LIMIT = 10_000
shadow var session_state = Session::new()

public procedure create_account(id: u64): Account
{
    result Account { id, balance: 0 }
}
```

[1] This module defines a public record, imports a ledger operation, introduces immutable and mutable bindings, and exposes a callable—all forms elaborated in subsequent subclauses.

### §5.1.5 Conformance Requirements [decl.overview.requirements]

[1] Implementations shall recognise each declaration category enumerated in §5.1.2 and apply the specialised rules defined in §§5.2–5.8 to every occurrence.

[2] Implementations shall enforce the single lexical identifier namespace described in §5.1.3 and require `shadow` when redeclaring identifiers from an enclosing scope.
