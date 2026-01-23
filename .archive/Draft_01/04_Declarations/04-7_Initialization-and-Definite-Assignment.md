# Cursive Language Specification

## Clause 4 — Declarations

**Clause**: 4 — Declarations
**File**: 05-7_Initialization-and-Definite-Assignment.md
**Section**: §4.7 Initialization and Definite Assignment
**Stable label**: [decl.initialization]  
**Forward references**: §3.6 [module.initialization], §4.2 [decl.variable], Clause 8 [stmt], Clause 10 [memory]

---

### §4.7 Initialization and Definite Assignment [decl.initialization]

#### §4.7.1 Overview

[1] This subclause specifies how declarations acquire initial values and how the compiler ensures every binding is definitely assigned before use. Cursive provides two binding operators: `=` for responsible bindings and `<-` for non-responsible bindings.

[2] Module-scope initialization interacts with the dependency model in §3.6; block-scoped initialization and reassignment rules integrate with permissions in Clause 10.

#### §5.7.2 Module-Scope Initialization

[1] Module-scope `let`/`var` bindings are initialized exactly once before code in the module executes. Implementations construct the eager dependency graph described in §4.6 and evaluate initializers in topological order.

[2] Cycles in the eager dependency graph are ill-formed (diagnostic E05-701). Implementations shall report every binding in the cycle.

[3] If an initializer fails (e.g., panics), dependent bindings are not evaluated. Each blocked binding receives diagnostic E05-702, identifying the failing dependency.

[4] Module-scope `let` bindings whose initializers are compile-time evaluable become compile-time constants; Clause 7 treats them as `ConstExpr`.

[5] Module-scope `var` bindings evaluate their initializers at module initialization. Subsequent assignments follow the rules in Clause 12.

#### §5.7.3 Block-Scoped Initialization

[1] Block-scoped bindings (`let`/`var`) are initialized when execution reaches the declaration. Control flow shall not use a binding before it has been assigned.

[2] The compiler diagnoses uses of unassigned bindings as E05-703 (“binding may be unassigned here”). This check applies across branches and loop iterations.

#### §5.7.4 Definite Assignment Rules

[1] A binding declared with `let` or `var` must be assigned on every control-flow path before each use. `let` forbids reassignment; `var` permits it.

[2] For conditionals (`if`, `match`), each branch must assign the binding before it is used afterwards.

[3] Before entering a loop, the binding must already have a value. Assignments that occur only inside the loop body do not satisfy definite assignment for code that executes before the first iteration.

[4] Pattern bindings (§5.2–§5.3) shall bind every identifier in the pattern. Omitting a field produces E05-704.

[5] **Dependency tracking for non-responsible bindings**: When a non-responsible binding is created via `let n <- source`, the compiler records that `n` depends on the same object as `source`. The non-responsible binding remains valid as long as the object exists. Since the compiler cannot track object destruction at runtime (zero-cost principle), it uses **parameter responsibility** as a compile-time approximation:

- When `source` is moved to a **responsible parameter** (`move` modifier), the callee might destroy the object, so `n` becomes invalid
- When `source` is passed to a **non-responsible parameter** (no `move`), the callee will not destroy the object, so `n` remains valid
- When `source`'s scope ends, the object is destroyed, so `n` must also be out of scope or invalid

This compile-time approximation ensures memory safety without runtime tracking: non-responsible bindings cannot access potentially destroyed objects.

[6] With non-responsible bindings (`<-`), the compiler enforces permissions from Clause 11. Assignments that violate permission rules are diagnosed in that clause.

#### §5.7.5 Non-Responsible Binding Validity

##### §5.7.5.1 Overview

[1] Non-responsible bindings (created with `<-`) have distinct validity rules from responsible bindings. This subsection specifies when non-responsible bindings become invalid and their interaction with responsibility transfer.

##### §5.7.5.2 Invalidation on Potential Object Destruction

[2] **THIS SECTION IS AUTHORITATIVE** for non-responsible binding invalidation. Non-responsible bindings bind to the **object**, not the binding. The `<-` operator creates a non-responsible binding that remains valid as long as the object exists. However, non-responsible bindings become invalid when the object **might be destroyed**.

[3] The compiler uses **parameter responsibility** as the compile-time signal for potential destruction. When a binding is moved to a procedure with a **responsible parameter** (marked with `move`), the callee might destroy the object, so all non-responsible bindings referencing that object become invalid:

**Invalidation Rule:**

[ Given: Binding $r$, non-responsible binding $n$ created via $n \gets r$, procedure with parameter `move x: T` ]

$$
\frac{r \text{ moved to responsible parameter (move at call site)}}
     {r \text{ and } n \text{ both become invalid}}
\tag{WF-NonResp-Invalidate}
$$

[3.1] This rule is the **COMPLETE** invalidation condition. Non-responsible bindings remain valid when:

1. Source binding is passed to NON-responsible parameters (no `move` at call site)
2. Source binding remains in scope and not moved
3. Non-responsible binding has not exited its own scope

**Example 5.7.5.1 (Invalidation when object might be destroyed):**

```cursive
procedure consume(move data: Buffer)   // Responsible parameter (might destroy)
{
    data.process()
    // data.drop() called at scope exit
}

let owner = Buffer::new()              // Responsible
let viewer <- owner                    // Non-responsible (binds to object)

consume(move owner)                    // Object might be destroyed by consume
// owner becomes invalid (moved-from state)
// viewer becomes invalid (object might be destroyed)
// viewer.read()                       // ERROR E11-504: use of invalidated non-responsible binding
```

[4] This invalidation ensures memory safety:
- No use-after-free: Non-responsible bindings become invalid before object destruction
- No double-free: Non-responsible bindings never call destructors
- Zero runtime cost: Parameter responsibility known at compile time

##### §5.7.5.3 Non-Responsible Parameters Preserve Validity

[5] When a binding is passed to a procedure with a **non-responsible parameter** (no `move` modifier), the object is guaranteed to survive the call. Non-responsible bindings remain valid:

**Example 5.7.5.2 (Non-responsible parameter preserves validity):**

```cursive
procedure inspect(data: Buffer)        // Non-responsible parameter (no move)
{
    println("Size: {}", data.size())
    // data.drop() NOT called (non-responsible parameter)
}

let owner = Buffer::new()              // Responsible binding
let viewer <- owner                    // Non-responsible (references object)

inspect(owner)                         // ✅ OK: passed to non-responsible param
viewer.read()                          // ✅ VALID: object guaranteed to survive inspect()
owner.use()                            // ✅ VALID: owner still responsible

// At scope exit: owner.drop() called, viewer does not call drop
```

[6] The key distinction: parameter responsibility determines object lifetime:
- **Non-responsible parameter**: Object survives the call, non-responsible bindings remain valid
- **Responsible parameter** (`move`): Object might be destroyed, non-responsible bindings become invalid

##### §5.7.5.4 Scope-Based Invalidation

[7] Non-responsible bindings also become invalid when they exit their lexical scope:

```cursive
let owner = Data::new()
{
    let temp_view <- owner             // Valid within this scope
    temp_view.read()                   // ✅ OK: owner not moved, scope active
}  // temp_view exits scope (invalidated by scope exit)
// owner still valid (not moved)
```

[8] Invalidation conditions (cumulative):
1. Non-responsible binding exits its own lexical scope
2. Source binding is moved to a responsible parameter

##### §5.7.5.5 Type Preservation

[9] Non-responsible bindings have the same type as the value they reference. The `<-` operator affects binding metadata (cleanup responsibility) but not type:

**Example 5.7.5.3 (Type identity):**

```cursive
let buffer: Buffer = Buffer::new()     // Type: Buffer, responsible
let ref: Buffer <- buffer              // Type: Buffer, non-responsible
// Same type, different cleanup responsibility
```

[10] Type checking, permission enforcement, and all type system rules apply identically to both responsible and non-responsible bindings. Only cleanup responsibility and transferability differ.

##### §5.7.5.6 Rebinding Non-Responsible Vars

[11] `var` bindings created with `<-` may be rebound to reference different values:

**Example 5.7.5.4 (Rebinding non-responsible var):**

```cursive
var current_view <- buffer1
current_view.read()

current_view <- buffer2                // Rebind to different object
current_view.read()                    // Now reads from buffer2
// No destructors called during rebinding
```

[12] Rebinding a non-responsible `var` does not invoke any destructors. The binding simply refers to a different value.

##### §5.7.5.7 Permission Compatibility

[13] Non-responsible bindings must have permission-compatible types with the source value. Permission coercion rules (§11.4) apply: `unique` → `const` and `shared` → `const` are permitted, but upgrades and cross-coercions are forbidden.

**Example 5.7.5.5 (Permission coercion):**

```cursive
let data: unique = Data::new()
let const_view: const <- data          // ✅ OK: unique → const coercion
let unique_view: unique <- data        // ❌ ERROR: cannot create second unique binding
```

[14] Complete permission semantics are specified in Clause 11 [memory].

##### §5.7.5.8 Safe Usage Patterns for Non-Responsible Bindings

[15] Non-responsible bindings are safe when used according to the following patterns:

**Pattern 1: Pass to non-responsible parameters**

When the source binding is passed to procedures with non-responsible parameters (no `move` modifier), the object survives the call and non-responsible bindings remain valid. This is the primary use case for non-responsible bindings.

**Pattern 2: Keep source binding alive**

When the source binding remains in scope and is not moved, all non-responsible bindings derived from it remain valid until they exit their own scopes.

**Pattern 3: Use with Copy types**

Copy types (§10.4.5.2) have no destructors, so concerns about object destruction don't apply. Non-responsible bindings to Copy-type values are always safe.

**Example 5.7.5.6 (Complete safe usage pattern):**

```cursive
procedure inspect(data: Buffer)        // Non-responsible parameter (no move modifier)
{
    println("Size: {}", data.size())
    // data.drop() NOT called (non-responsible parameter)
}

procedure consume(move data: Buffer)   // Responsible parameter (might destroy)
{
    data.process()
    // data.drop() IS called at scope exit
}

let owner = Buffer::new()
let viewer <- owner                    // Non-responsible binding (references object)

// Safe: pass to non-responsible parameter
inspect(owner)                         // ✅ Object survives
viewer.read()                          // ✅ VALID: object alive

// Safe: use without moving
viewer.read()                          // ✅ VALID: owner not moved
owner.use()                            // ✅ VALID: owner still responsible

// Unsafe: move to responsible parameter
// consume(move owner)                 // Would invalidate owner AND viewer
// viewer.read()                       // ERROR E11-504: object might be destroyed

// At scope exit: owner.drop() called, viewer does not call drop
```

[16] **Recommended practice**: Use non-responsible bindings to create multiple temporary views of data that remains under the original binding's control. Avoid moving the source binding to responsible parameters while non-responsible bindings exist, or ensure non-responsible bindings exit scope before the move.

##### §5.7.5.9 Validity Summary

[17] **Table 5.7.1 — Non-responsible binding validity conditions**

| Operation on Source                     | Non-Responsible Binding State | Rationale                       |
|-----------------------------------------|-------------------------------|---------------------------------|
| Create `let n <- source`                | Valid                         | Binds to object                 |
| Pass `source` to non-responsible param  | Remains valid                 | Object survives call            |
| Pass `inspect(source)` (no move)        | Remains valid                 | Object not destroyed            |
| Move to responsible param `move source` | Becomes invalid               | Object might be destroyed       |
| Call `consume(move source)`             | Becomes invalid               | Callee might destroy object     |
| Source scope ends                       | Must be out of scope          | Object destroyed at scope exit  |
| Non-responsible binding scope ends      | Becomes invalid               | Binding exits scope             |

[18] The key insight: **parameter responsibility** (presence of `move` modifier on procedure parameter) signals whether the callee will destroy the object. This compile-time information enables safe non-responsible binding tracking without runtime overhead.

#### §5.7.6 Reassignment and Drop Order

[1] Reassigning a `let` binding is ill-formed (E05-705). `var` bindings may be reassigned, subject to permission checks.

[2] When a scope exits, bindings are dropped in reverse declaration order. Drop behavior and destructor semantics are governed by Clause 11.

#### §5.7.7 Examples (Informative)

**Example 5.7.7.1 (Module-level constants):**

```cursive
let VERSION: string = "1.2.0"        // compile-time constant
var current_locale = detect_locale()  // runtime initialization
```

**Example 5.7.7.2 (Definite assignment across branches):**

```cursive
var total: i64
if config.has_value {
    total = config.value
} else {
    total = 0
}
use_total(total)  // OK: both branches assign total
```

**Example 5.7.7.3 - invalid (Unassigned variable):**

```cursive
var data: Buffer
if should_allocate {
    data = make_buffer()
}
consume(data)  // error[E05-703]: data may be unassigned
```

**Example 5.7.7.4 - invalid (Pattern mismatch):**

```cursive
let {x, y}: Point = make_point()   // OK
let {x}: Point = make_point()      // error[E05-704]: field y missing
```

### §5.7.8 Conformance Requirements [decl.initialization.requirements]

[1] Implementations shall construct the module initialisation dependency graph of §4.6, diagnose cycles (E05-701), and report blocked bindings when an initialiser fails (E05-702).

[2] Compilers shall enforce definite-assignment analysis for block-scoped bindings, emitting E05-703 when a binding might be uninitialised and E05-705 when a `let` binding is reassigned.

[3] Compilers shall track dependencies for non-responsible bindings: when `let n <- source` is declared, record that `n` depends on `source`. When `source` is moved to a responsible parameter (indicated by `move` at call site), propagate invalidation to all dependent non-responsible bindings and emit E11-504 when invalidated bindings are used. When `source` is passed to non-responsible parameters (no `move` at call site), non-responsible bindings remain valid because the object is guaranteed to survive.

[4] Pattern bindings shall bind every identifier declared in the pattern; omissions shall be diagnosed with E05-704 and prevent the program from compiling.
