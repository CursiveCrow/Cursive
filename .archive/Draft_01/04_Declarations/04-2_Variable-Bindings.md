# Cursive Language Specification

## Clause 4 — Declarations

**Clause**: 4 — Declarations
**File**: 05-2_Variable-Bindings.md
**Section**: §4.2 Variable Bindings and Initialisers
**Stable label**: [decl.variable]  
**Forward references**: §3.6 [module.initialization], §4.3 [decl.pattern], §4.7 [decl.initialization], §9.4 [memory.permission]

---

### §4.2 Variable Bindings and Initialisers [decl.variable]

#### §4.2.1 Overview

[1] Variable bindings introduce identifiers into the lexical namespace with explicit binding mutability (`let`, `var`) and optional pattern destructuring.

[2] Bindings may appear at module scope or within block scopes; their storage duration and initialisation ordering depend on location (§4.7, §3.6).

[3] This subclause defines binding syntax, pattern forms, initialiser requirements, and the interaction with shadowing, permissions, and compile-time constants.

#### §4.2.2 Syntax

[ Note: See Annex A §A.5 [grammar.statement] for the normative `var_decl_stmt` production. — end note ]

[3] Variable bindings consist of a binding head (`let`, `var`, `shadow let`, or `shadow var`), a pattern (identifier or destructuring), an optional type annotation, a binding operator (`=` for responsible or `<-` for non-responsible), and an initializer expression.

[1] Every binding introduces at least one identifier. Patterns specify that identifier set explicitly; anonymous bindings are ill-formed.

[2] When a pattern binds more than one identifier, a `:{Type}` annotation is mandatory and applies uniformly to every identifier in the pattern.

[3] `initializer` must be present for every binding form. The binding operator determines whether the binding is responsible for cleanup (`=`) or non-responsible (`<-`).

[4] **Type independence.** The type of the binding is determined by the initializer expression and is independent of the binding operator choice. Both `let x: T = value` and `let x: T <- value` create bindings of type `T`; the difference lies in cleanup responsibility (binding metadata), not in the type itself.

#### §4.2.3 Constraints

[1] _Explicit shadowing._ `shadow let` and `shadow var` shall only appear when an enclosing scope already defines the same identifier. Violations: E05-201.

[2] _Single assignment._ `let` bindings shall not be reassigned after initialisation. `var` bindings may be reassigned. Violations: E05-202.

[3] _Pattern uniformity._ Multi-identifier patterns require a type annotation `:{Type}` that applies to every identifier in the pattern. All identifiers in the pattern must be distinct. Violations: E05-203.

[4] _Initialiser completeness._ The initialiser expression shall provide values for every identifier in the pattern. Violations: E05-204.

[5] _Compile-time constants._ A binding is a compile-time constant only when it is a module-scope `let` with a compile-time evaluable initialiser or a binding declared within a `comptime` block. Function- or block-scope bindings remain runtime values regardless of their initialiser expression.

[6] _Permission independence._ Binding mutability controls reassignment, not value mutation. Value mutation permissions arise from the type's permission qualifiers or region attributes (§11.4).

[7] _Forward references._ Due to two-phase compilation (§2.2), bindings may reference declarations that appear later in the translation unit. Shadowing rules are evaluated after parsing, ensuring forward references do not bypass explicit shadow requirements.

#### §4.2.4 Semantics

[1] Binding introduction inserts the identifier or identifiers into the current lexical scope's namespace, honouring the single-namespace rule (§5.1.3) and shadowing constraints.

[2] For patterns, the initialiser expression is evaluated once; its value is destructured into the constituent bindings. The implementation behaves as if a temporary value were created, followed by individual bindings drawing from that value.

[3] Module-scope bindings participate in the eager dependency graph (§3.6); their initialisers shall not depend on bindings that, directly or indirectly, require the binding being initialised.

[4] Pattern bindings do not change visibility or storage semantics: each identifier inherits the visibility modifier and scope of the encompassing binding.

[5] The binding operator (`=` or `<-`) does not affect the binding's type but determines whether the binding is recorded as responsible or non-responsible in the binding table (§5.3). This metadata is used by move checking (Clause 9) and definite assignment analysis (§4.7) but does not participate in type checking (Clause 6).

#### §4.2.5 Binding Operator Semantics [decl.variable.operator]

##### §4.2.5.1 Value Assignment Operator (`=`)

[5] The value assignment operator `=` creates a responsible binding. The binding assumes cleanup responsibility for the value and shall invoke the value's destructor when the binding goes out of scope.

**Formation:**

```cursive
let identifier: Type = expression     // Responsible, non-rebindable
var identifier: Type = expression     // Responsible, rebindable
```

**Cleanup Responsibility:**

[6] At scope exit, the binding's destructor is invoked in LIFO order (§11.2). If the binding was created with `let`, it may transfer responsibility via the `move` keyword (§11.5). If the binding was created with `var`, responsibility cannot be transferred but the binding may be rebound in place.

**Example 4.2.5.1 (Responsible bindings):**

```cursive
{
    let resource = File::open("data.txt")   // Responsible binding
    // Use resource...
}  // resource.drop() called automatically
```

##### §4.2.5.2 Non-Responsible Binding Operator (`<-`)

[7] The non-responsible binding operator `<-` creates a binding that does NOT assume cleanup responsibility. The binding shall NOT invoke a destructor when it goes out of scope.

[ Note: The `<-` operator does NOT create a reference type or pointer type. Both `let x: T = value` and `let x: T <- value` create bindings of type `T`; the difference is only in cleanup responsibility (binding metadata), not in the type itself. The terminology "non-responsible binding" emphasizes that this is about cleanup semantics, not type semantics.
— end note ]

**Formation:**

```cursive
let identifier: Type <- expression    // Non-responsible, non-rebindable
var identifier: Type <- expression    // Non-responsible, rebindable
```

**Type Semantics:**

[8] The type of a binding created with `<-` is identical to the type of the initializer expression. The `<-` operator creates a non-responsible binding to the value with the value's type but without cleanup responsibility. It does NOT create a reference type, pointer type, or any other type-level construct.

**Example 4.2.5.2 (Type preservation):**

```cursive
let buffer: Buffer = Buffer::new()     // Type: Buffer, responsible
let ref: Buffer <- buffer              // Type: Buffer, non-responsible
// Both bindings have type Buffer
// Only difference is cleanup responsibility (binding metadata)
```

**Non-Responsibility:**

[9] Non-responsible bindings do not call destructors:

```cursive
let owner = Resource::new()
{
    let viewer <- owner                // Non-responsible binding
    viewer.read()                      // Access value
}  // viewer.drop() NOT called (owner still responsible)
// owner still valid here
```

**Multiple Non-Responsible Bindings:**

[10] Multiple non-responsible bindings to the same value are permitted:

```cursive
let data = load_data()
let view1 <- data
let view2 <- data
let view3 <- data
// All three non-responsible bindings valid simultaneously
// Only 'data' will call destructor at scope exit
```

**Invalidation After Responsibility Transfer:**

[11] Non-responsible binding validity and invalidation rules are specified completely in §5.7.5 [decl.initialization]. In summary, non-responsible bindings become invalid when the source binding is moved to a responsible parameter (one with the `move` modifier), as the object might be destroyed by the callee.

[12] This invalidation propagation prevents use-after-free: when a value's ownership transfers to a procedure that might destroy it, all references derived from the original binding become inaccessible. The compiler tracks dependencies through definite assignment analysis with zero runtime overhead.

**Transferability:**

[13] Non-responsible bindings cannot be moved because they have no cleanup responsibility to transfer. Violations: E11-502.

##### §4.2.5.3 Binding Operator Choice

[14] The choice between `=` and `<-` determines cleanup responsibility:

| Operator | Cleanup Responsibility | Destructor Called   | Transferable via `move` |
| -------- | ---------------------- | ------------------- | ----------------------- |
| `=`      | YES                    | YES (at scope exit) | YES (if `let`)          |
| `<-`     | NO                     | NO                  | NO                      |

[15] The binding operator is orthogonal to rebindability (`let` vs `var`) and permissions (`const`, `unique`, `shared`), creating a fully compositional binding system.

##### §4.2.5.4 Validity and Invalidation Rules

[15.1] **Summary of non-responsible binding lifetime tracking**:

Non-responsible bindings reference **objects**, not bindings. Their validity depends on whether the object exists:

**Remains valid when**:

- Source binding is in scope and not moved
- Source binding is passed to non-responsible parameters (no `move` at call site)
- Object is guaranteed to survive (non-destroying callees)

**Becomes invalid when**:

- Source binding is moved to responsible parameter (`move` at call site)
- Source binding's scope ends (object destroyed)
- Non-responsible binding's own scope ends

The compiler uses **parameter responsibility** (visible in procedure signatures) to approximate object destruction at compile time, maintaining zero runtime overhead while preventing use-after-free.

##### §4.2.5.5 Rebinding Semantics

[16] `var` bindings may be rebound using the same assignment operator:

**Responsible `var` rebinding:**

```cursive
var data = Buffer::new()               // Responsible
data = Buffer::new()                   // Old buffer destroyed, new buffer bound
```

When rebinding a responsible `var`, the old value's destructor is invoked before the new value is assigned.

**Non-responsible `var` rebinding:**

```cursive
var ref <- buffer1                     // Non-responsible
ref <- buffer2                         // Rebind to different object (no cleanup)
```

When rebinding a non-responsible `var`, no destructor is invoked; the binding simply refers to a different value.

#### §4.2.6 Additional Examples [decl.variable.examples]

[ Note: Examples demonstrating the interaction between binding operators and parameter responsibility are provided in §4.4.5 [decl.function] and §4.7.5 [decl.initialization]. — end note ]

**Example 4.2.6.2 (Rebinding with different operators):**

```cursive
var responsible = Data::new()          // Responsible, rebindable
responsible = Data::new()              // Old value destroyed, new value bound

var non_responsible <- some_value      // Non-responsible, rebindable
non_responsible <- other_value         // Rebind to different object (no cleanup)
```

**Example 4.2.6.3 - invalid (Implicit shadowing):**

```cursive
let limit = 10
{
    let limit = limit + 5  // error[E04-201]: use 'shadow let limit'
}
```

**Example 4.2.6.4 - invalid (Pattern arity mismatch):**

```cursive
let {left, right}: Pair = make_triple()  // error[E04-204]
```

**Example 4.2.6.5 - invalid (Move from non-responsible binding):**

```cursive
let buffer = Buffer::new()
let ref <- buffer
consume(move ref)                      // error[E10-502]: cannot move non-responsible binding
```

#### §4.2.7 Conformance Requirements [decl.variable.requirements]

[1] Implementations shall track cleanup responsibility and invalidation per binding operator semantics specified in §4.2.5.

[2] Module-scope bindings shall participate in the dependency analysis of §3.6; implementations shall detect initialiser cycles (E04-701) and block dependent initialisers per §4.7.
