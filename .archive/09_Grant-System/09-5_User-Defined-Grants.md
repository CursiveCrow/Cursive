# Part IX: Grant System
## Section 9.5: User-Defined Grants

**File**: `09-5_User-Defined-Grants.md`
**Version**: 1.0
**Status**: Normative
**Previous**: [09-4_Built-In-Grants.md](09-4_Built-In-Grants.md) | **Next**: [09-6_Grant-Integration.md](09-6_Grant-Integration.md)

---

## 9.5.1 Grant Declaration Fundamentals

### 9.5.1.1 Grant Declaration Syntax

**Normative Statement 9.5.1**: Grants may be declared at module scope using the `grant` keyword with optional visibility modifiers.

**Syntax**:
```ebnf
GrantDecl ::= Visibility? "grant" Ident
```

**Example (informative)**:
```cursive
public grant query
internal grant admin
private grant internal_cache
```

**Placement**: Grant declarations shall appear at module level, outside of any procedure, function, or type declaration.

### 9.5.1.2 Grant Naming

**Normative Statement 9.5.2**: Grant names shall follow identifier naming rules and shall not conflict with reserved grant namespaces.

**Reserved Namespaces**: The following top-level grant namespaces are reserved for built-in grants and shall not be used as grant names in user code:
- `alloc`
- `fs`
- `net`
- `thread`
- `time`
- `ffi`
- `unsafe`
- `panic`

**Example (informative - INVALID)**:
```cursive
// ERROR: 'alloc' is a reserved namespace
public grant alloc  // ERROR
```

### 9.5.1.3 Module Scoping

**Normative Statement 9.5.3**: User-defined grants are scoped to the module in which they are declared. The fully qualified grant path is automatically constructed from the module path and grant name.

**Formal Rule**: For a grant `g` declared in module with path `M`, the fully qualified grant path is `M::g`.

**Example (informative)**:
```cursive
// File: storage/database.cursive
public grant query
public grant write

// Fully qualified paths:
// storage::database::query
// storage::database::write
```

---

## 9.5.2 Grant Path Resolution

### 9.5.2.1 Qualified Grant Paths

**Normative Statement 9.5.4**: User-defined grants shall be referenced using their fully qualified module path followed by `::` and the grant name.

**Syntax**: `ModulePath "::" GrantName`

Where:
- `ModulePath` uses double-colon notation (e.g., `storage::database`)
- `::` separates the module path from the grant name
- `GrantName` is the identifier used in the grant declaration

**Example (informative)**:
```cursive
// File: application.cursive
import storage::database

procedure fetch_records()
    sequent { [storage::database::query] |- true => true }
{
    // Implementation
}
```

### 9.5.2.2 Resolution Algorithm

**Normative Statement 9.5.5**: Grant path resolution shall proceed as follows:

1. Parse the grant path into module path and grant name components
2. Verify the module path refers to an imported or current module
3. Verify the grant is declared in that module
4. Verify the grant is visible from the current module (§9.5.3)
5. If any step fails, issue a compile-time diagnostic

**Implementation Requirement**: A conforming implementation shall resolve user-defined grant paths using the same module resolution mechanism as type and procedure names.

### 9.5.2.3 Ambiguity Resolution

**Normative Statement 9.5.6**: Built-in grants take precedence over user-defined grants. If a user-defined grant path conflicts with a built-in grant path, the built-in grant shall be selected.

**Example (informative)**:
```cursive
// Even if module 'alloc' exists with grant 'heap':
procedure example()
    sequent { [alloc::heap] |- true => true }  // Always refers to built-in alloc::heap
{
    // ...
}
```

---

## 9.5.3 Visibility and Access Control

### 9.5.3.1 Visibility Modifiers

**Normative Statement 9.5.7**: Grant declarations shall support the same visibility modifiers as other module-level declarations: `public`, `internal`, `private`, and `protected`.

**Visibility Semantics**:

| Modifier | Scope |
|----------|-------|
| `public` | Visible to all modules |
| `internal` | Visible within same package |
| `private` | Visible within declaring module only |
| `protected` | Visible to declaring module and submodules |

**Default Visibility**: If no visibility modifier is specified, grants shall have `private` visibility.

### 9.5.3.2 Visibility Verification

**Normative Statement 9.5.8**: When a procedure declares a user-defined grant in its grant clause, the compiler shall verify that the grant is visible from the procedure's module.

**Diagnostic Requirement 9.5.1**: When a procedure attempts to declare an invisible grant, the implementation shall issue a compile-time diagnostic indicating the grant name, its visibility, and the declaring module.

**Example (informative)**:
```cursive
// File: storage/database.cursive
private grant vacuum

// File: application.cursive
import storage::database

procedure maintenance()
    sequent { [storage::database::vacuum] |- true => true }  // ERROR: grant is private
{
    // ...
}
```

---

## 9.5.4 Grant Usage and Propagation

### 9.5.4.1 Using User-Defined Grants

**Normative Statement 9.5.9**: User-defined grants shall be used in grant clauses using the same syntax as built-in grants.

**Example (informative)**:
```cursive
// File: storage/database.cursive
public grant query

procedure execute_select(sql: string): [i32]
    sequent { [query] |- true => true }
{
    // Low-level query implementation
}

// File: application.cursive
import storage::database

procedure list_items(): [i32]
    sequent { [storage::database::query] |- true => true }
{
    result storage::database::execute_select("SELECT id FROM items")
}
```

### 9.5.4.2 Mixed Grant Sets

**Normative Statement 9.5.10**: Procedures may declare grant clauses containing both built-in grants and user-defined grants. All grants shall be verified using the same subset rules.

**Example (informative)**:
```cursive
import storage::database

procedure process_query(): [i32]
    sequent { [storage::database::query, alloc::heap, fs::write] |- true => true }
{
    // Uses database query, heap allocation, and file writing
}
```

### 9.5.4.3 Subset Verification

**Normative Statement 9.5.11**: User-defined grants shall be subject to the same subset verification rules as built-in grants (§9.2.1). For a call from procedure A to procedure B:

```
grants(B) <: grants(A)
```

This rule applies to all grants, regardless of whether they are built-in or user-defined.

### 9.5.4.4 Transitive Propagation

**Normative Statement 9.5.12**: User-defined grants shall propagate through call chains using the same transitive closure algorithm as built-in grants (§9.2.2).

**Example (informative)**:
```cursive
import storage::database

procedure low_level()
    sequent { [storage::database::query] |- true => true }
{
    // Direct operation
}

procedure mid_level()
    sequent { [storage::database::query, alloc::heap] |- true => true }
{
    low_level()  // Valid: {storage::database::query} <: {storage::database::query, alloc::heap}
}

procedure high_level()
    sequent { [storage::database::query, alloc::heap, fs::write] |- true => true }
{
    mid_level()  // Valid: {storage::database::query, alloc::heap} <: {storage::database::query, alloc::heap, fs::write}
}
```

---

## 9.5.5 Wildcard Grants

### 9.5.5.1 Module Wildcard Syntax

**Normative Statement 9.5.13**: User-defined grants shall support wildcard syntax using `ModulePath::*`, which grants all visible grants from the specified module.

**Syntax**: `ModulePath "::" "*"`

**Example (informative)**:
```cursive
import storage::database

procedure admin_operation()
    sequent { [storage::database::*] |- true => true }
{
    // Has all visible grants from storage::database module
}
```

### 9.5.5.2 Wildcard Expansion

**Normative Statement 9.5.14**: A wildcard user grant `M::*` shall be expanded to the set of all grants declared in module M that are visible from the current module.

**Formal Definition**:
```
M::* = {M::g₁, M::g₂, ..., M::gₙ}
```

Where `g₁, g₂, ..., gₙ` are all grants declared in module M that satisfy the visibility constraints from the current module.

**Visibility Constraint**: Private grants shall not be included in wildcard expansion for external modules.

---

## 9.5.6 Grant Polymorphism with User Grants

### 9.5.6.1 Generic Procedures

**Normative Statement 9.5.15**: User-defined grants may be used in grant parameter bounds and instantiations in the same manner as built-in grants.

**Example (informative)**:
```cursive
import storage::database

procedure with_grant<grants G>(operation: () -> () sequent { [grants(G)] |- true => true })
    sequent { [grants(G)] |- true => true }
    where G <: {storage::database::query, alloc::heap}
{
    operation()
}
```

### 9.5.6.2 Mixed Grant Bounds

**Normative Statement 9.5.16**: Grant parameter bounds may include both built-in and user-defined grants.

**Example (informative)**:
```cursive
import storage::database

procedure mixed_bounds<grants G>()
    sequent { [grants(G), fs::write] |- true => true }
    where G <: {storage::database::query, storage::database::write, alloc::heap}
{
    // Can use grants(G) and fs::write
}
```

---

## 9.5.7 Well-Formedness Constraints

A grant declaration is well-formed if and only if:

1. **Valid Identifier**: The grant name is a valid identifier
2. **No Reserved Conflicts**: The grant name does not conflict with reserved namespaces (§9.5.1.2)
3. **No Duplicates**: No other grant with the same name is declared in the same module
4. **Valid Visibility**: The visibility modifier is one of: `public`, `internal`, `private`, `protected`
5. **Module Scope**: The declaration appears at module scope, not nested within other declarations

**Ill-Formed Examples** (informative):
```cursive
// ERROR: Reserved namespace
public grant alloc  // ERROR

// ERROR: Duplicate in same module
public grant query
public grant query  // ERROR

// ERROR: Invalid visibility
external grant write  // ERROR: 'external' is not a valid visibility modifier
```

---

## 9.5.8 Verification Algorithm

### 9.5.8.1 Extended Grant Set Construction

**Normative Statement 9.5.17**: The grant set construction algorithm (§9.2.3.2) shall be extended to recognize user-defined grants.

**Algorithm Extension** (informative):
```
For each procedure P:
  1. Parse grant clause
  2. For each grant path G in the clause:
     a. If G matches a built-in grant pattern: validate against §9.4
     b. Else: resolve as user-defined grant (§9.5.2)
     c. Verify visibility constraints (§9.5.3)
  3. Construct set grants(P) from validated grant paths
  4. Check for duplicates (ill-formed if duplicates found)
```

### 9.5.8.2 Diagnostic Requirements

**Diagnostic Requirement 9.5.2**: When a user-defined grant cannot be resolved, the implementation shall issue a diagnostic containing:

1. The unresolved grant path
2. The module in which resolution was attempted
3. Suggestions for:
   - Missing import statements
   - Visibility issues
   - Typographical errors in grant name

**Example Diagnostic** (informative):
```
error[E0420]: cannot find grant `storage::database::queryx` in scope
  --> src/application.cursive:15:12
   |
15 |     grants storage::database::queryx
   |            ^^^^^^^^^^^^^^^^^^^^^^^^ grant not found
   |
   = help: did you mean `storage::database::query`?
   = note: grant must be imported and visible from this module
```

---

## 9.5.9 Compile-Time Erasure

**Normative Statement 9.5.18**: User-defined grants shall be erased at compile time, imposing zero runtime overhead, consistent with built-in grants (§9.2.5).

**No Runtime Representation**: User-defined grant declarations and grant clauses shall have no runtime representation.

**No Runtime Checks**: Generated code shall not contain any checks for user-defined grants.

---

## 9.5.10 Interaction with Built-In Grants

### 9.5.10.1 Uniform Treatment

**Normative Statement 9.5.19**: After resolution and visibility verification, user-defined grants shall be treated identically to built-in grants for purposes of:

1. Subset verification (§9.2.1)
2. Transitive propagation (§9.2.2)
3. Grant polymorphism (§9.3)
4. Wildcard expansion (§9.5.5)
5. Compile-time erasure (§9.5.9)

### 9.5.10.2 No Semantic Difference

**Normative Statement 9.5.20**: The grant system shall make no semantic distinction between built-in and user-defined grants once resolution is complete. Both categories are compile-time capability tokens subject to identical verification rules.

**Rationale**: This uniform treatment ensures consistency, simplifies the verification algorithm, and allows user code to define application-specific capabilities using the same mechanisms as the language runtime.

---

## 9.5.11 Grammar Reference

The complete grammar for user-defined grant declarations is specified in Appendix A.6 (Declaration Grammar).

**Productions**:
```ebnf
Decl      ::= ... | GrantDecl
GrantDecl ::= Visibility? "grant" Ident
```

**Cross-Reference**: See Appendix A §A.6 for authoritative grammar.

---

## 9.5.12 Conformance Requirements

A conforming implementation shall:

1. **Parse grant declarations** (§9.5.1): Accept `grant` declarations at module scope with visibility modifiers
2. **Enforce reserved namespaces** (§9.5.1.2): Reject grant names that conflict with reserved namespaces
3. **Resolve grant paths** (§9.5.2): Resolve user-defined grant paths to declared grants
4. **Verify visibility** (§9.5.3): Enforce visibility constraints on grant usage
5. **Apply subset rules** (§9.5.4.3): Verify user-defined grants using same subset rules as built-in grants
6. **Support wildcards** (§9.5.5): Expand `Module::*` to all visible grants in the module
7. **Support grant polymorphism** (§9.5.6): Allow user-defined grants in generic bounds and instantiations
8. **Erase at compile time** (§9.5.9): Generate code with zero runtime overhead for user-defined grants
9. **Issue diagnostics** (§9.5.8.2): Provide clear error messages for resolution and visibility failures
10. **Treat uniformly** (§9.5.10): Apply identical verification rules to built-in and user-defined grants

---

**Previous**: [09-4_Built-In-Grants.md](09-4_Built-In-Grants.md) | **Next**: [09-6_Grant-Integration.md](09-6_Grant-Integration.md)
