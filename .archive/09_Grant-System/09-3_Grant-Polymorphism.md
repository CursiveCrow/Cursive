# Part IX: Grant System
## Section 9.3: Grant Polymorphism

**File**: `09-3_Grant-Polymorphism.md`
**Version**: 1.0
**Status**: Normative
**Previous**: [09-2_Grant-Tracking.md](09-2_Grant-Tracking.md) | **Next**: [09-4_Built-In-Grants.md](09-4_Built-In-Grants.md)

---

## 9.3.1 Grant Parameter Fundamentals

### 9.3.1.1 Grant Parameters

**Definition 9.3 (Grant Parameter)**: A grant parameter is a generic parameter that ranges over grant sets, enabling procedures to be polymorphic over the capabilities they require.

**Normative Statement 9.3.1**: Grant parameters shall be declared in generic parameter lists and shall be denoted by identifiers (conventionally lowercase Greek letters such as ε, φ, ψ).

**Syntax**:
```ebnf
GrantParam ::= Ident
```

**Example (informative)**:
```cursive
procedure process<T, grants G>(data: T): T
    sequent { [grants(G), alloc::heap] |- true => true }
{
    // Can use grants in G, plus alloc::heap
    result transform(data)
}
```

### 9.3.1.2 Grant Parameter Reference

**Normative Statement 9.3.2**: A grant parameter is referenced in the grant context using the syntax `grants(G)` where G is the grant parameter identifier.

**Syntax**:
```ebnf
GrantRef ::= "grants" "(" Ident ")"
```

**Semantics**: `grants(G)` represents the set of grants bound to the parameter G at the call site.

**Example (informative)**:
```cursive
procedure generic<grants G>(x: i32)
    sequent { [grants(G)] |- true => true }
{
    // Procedure body can perform operations requiring grants in G
    operation_requiring_grants(x)
}
```

### 9.3.1.3 Multiple Grant Parameters

**Normative Statement 9.3.3**: A procedure may declare multiple grant parameters. Each grant parameter represents an independent grant set.

**Example (informative)**:
```cursive
procedure multi_param<grants G₁, grants G₂>(a: i32, b: i32)
    sequent { [grants(G₁), grants(G₂), alloc::heap] |- true => true }
{
    // Can use grants in G₁, grants in G₂, and alloc::heap
    operation1()  // May require grants from G₁
    operation2()  // May require grants from G₂
    allocate(100) // Requires alloc::heap
}
```

---

## 9.3.2 Grant Parameter Bounds

### 9.3.2.1 Bound Syntax

**Normative Statement 9.3.4**: Grant parameters may have bounds specified using `where` clauses with the subset operator `<:`.

**Syntax**:
```ebnf
WherePredicate ::= Ident "<:" "{" GrantSet "}"
```

**Semantics**: A bound `G <: {g₁, g₂, ..., gₙ}` constrains the grant parameter G to be instantiated only with grant sets that are subsets of `{g₁, g₂, ..., gₙ}`.

**Example (informative)**:
```cursive
procedure constrained<grants G>(x: i32)
    sequent { [grants(G)] |- true => true }
    where G <: {fs::read, fs::write}
{
    // G can only contain fs::read and/or fs::write
    perform_io(x)
}
```

### 9.3.2.2 Bound Semantics

**Normative Statement 9.3.5**: At a call site, the grant set instantiated for G shall satisfy the bound constraint.

**Formal Rule**:
```
If G has bound G <: B, then at instantiation: G_actual <: B
```

**Example (informative)**:
```cursive
procedure bounded<grants G>()
    sequent { [grants(G)] |- true => true }
    where G <: {fs::read, alloc::heap}
{
    // Implementation
}

procedure caller1()
    sequent { [fs::read] |- true => true }
{
    // Valid: {fs::read} <: {fs::read, alloc::heap}
    bounded::<fs::read>()
}

procedure caller2()
    sequent { [fs::read, alloc::heap] |- true => true }
{
    // Valid: {fs::read, alloc::heap} <: {fs::read, alloc::heap}
    bounded::<fs::read, alloc::heap>()
}

procedure caller3()
    sequent { [net::write] |- true => true }
{
    // ERROR: {net::write} ⊄ {fs::read, alloc::heap}
    // bounded::<net::write>()  // ERROR
}
```

### 9.3.2.3 Empty Bound

**Normative Statement 9.3.6**: A grant parameter with no bound may be instantiated with any grant set.

**Example (informative)**:
```cursive
procedure unbounded<grants G>()
    sequent { [grants(G)] |- true => true }
{
    // G can be any grant set
}
```

### 9.3.2.4 Multiple Bounds

**Normative Statement 9.3.7**: Multiple grant parameters may have independent bounds.

**Example (informative)**:
```cursive
procedure multi_bounded<grants G₁, grants G₂>()
    sequent { [grants(G₁), grants(G₂)] |- true => true }
    where G₁ <: {fs::read, fs::write},
          G₂ <: {net::read, net::write}
{
    // G₁ constrained to file system operations
    // G₂ constrained to network operations
}
```

---

## 9.3.3 Grant Parameter Instantiation

### 9.3.3.1 Explicit Instantiation

**Normative Statement 9.3.8**: Grant parameters may be instantiated explicitly at call sites using angle bracket syntax.

**Syntax**:
```cursive
procedure_name::<grant_set>(arguments)
```

**Example (informative)**:
```cursive
procedure generic<grants G>(x: i32)
    sequent { [grants(G), alloc::heap] |- true => true }
{
    process(x)
}

procedure caller()
    sequent { [fs::read, fs::write, alloc::heap] |- true => true }
{
    // Explicit instantiation with {fs::read, fs::write}
    generic::<fs::read, fs::write>(42)
}
```

### 9.3.3.2 Grant Inference

**Informative Note**: Implementations may optionally infer grant parameter instantiations from context.

**Example (informative)**:
```cursive
procedure generic<grants G>(x: i32)
    sequent { [grants(G)] |- true => true }
{
    operation_requiring_grants(x)
}

procedure caller()
    sequent { [fs::read] |- true => true }
{
    // Grant inference: G inferred as {fs::read}
    generic(42)
}
```

**Non-Normative**: Grant parameter inference is not required for conformance.

### 9.3.3.3 Instantiation Validity

**Normative Statement 9.3.9**: At a call site, the instantiated grant set for a grant parameter shall be a subset of the caller's available grants.

**Formal Rule**:
```
For call A → B<G_inst>:
  grants(G_inst) <: grants(A)
```

**Example (informative)**:
```cursive
procedure callee<grants G>()
    sequent { [grants(G)] |- true => true }
{
    // ...
}

procedure caller()
    sequent { [fs::read, alloc::heap] |- true => true }
{
    // Valid: {fs::read} <: {fs::read, alloc::heap}
    callee::<fs::read>()

    // ERROR: {net::write} ⊄ {fs::read, alloc::heap}
    // callee::<net::write>()  // ERROR
}
```

---

## 9.3.4 Grant Parameter Composition

### 9.3.4.1 Combining Grant Parameters with Concrete Grants

**Normative Statement 9.3.10**: A procedure may combine grant parameters with concrete grants in its grant clause.

**Semantics**: The effective grant set is the union of all grant references.

**Example (informative)**:
```cursive
procedure mixed<grants G>(x: i32)
    sequent { [grants(G), alloc::heap, fs::write] |- true => true }
{
    // Can use: grants(G) ∪ {alloc::heap, fs::write}
    allocate(100)      // Uses alloc::heap
    write_file("log.txt", [0; 10])  // Uses fs::write
    parameterized_operation(x)       // Uses grants(G)
}
```

### 9.3.4.2 Multiple Grant Parameter Composition

**Normative Statement 9.3.11**: Multiple grant parameters in a grant clause represent the union of their respective grant sets.

**Formal Rule**:
```
grants(G₁), grants(G₂) ≡ grants G₁ ∪ G₂
```

**Example (informative)**:
```cursive
procedure multi<grants G₁, grants G₂>()
    sequent { [grants(G₁), grants(G₂)] |- true => true }
{
    // Effective grants: G₁ ∪ G₂
}

procedure caller()
    sequent { [fs::read, net::write, alloc::heap] |- true => true }
{
    // Valid: {fs::read} ∪ {net::write} <: {fs::read, net::write, alloc::heap}
    multi::<fs::read, net::write>()
}
```

---

## 9.3.5 Grant Polymorphic Patterns

### 9.3.5.1 Grant-Preserving Wrappers

**Pattern**: Procedures that wrap other procedures while preserving grant requirements.

**Example (informative)**:
```cursive
procedure with_logging<T, grants G>(operation: () -> T sequent { [grants(G)] |- true => true }): T
    sequent { [grants(G), fs::write] |- true => true }
{
    write_file("log.txt", [0; 10])  // Logging requires fs::write
    result operation()               // Operation requires grants(G)
}
```

### 9.3.5.2 Capability-Generic Algorithms

**Pattern**: Algorithms that work with any capability set.

**Example (informative)**:
```cursive
procedure repeat<grants G>(count: i32, operation: () -> () sequent { [grants(G)] |- true => true })
    sequent { [grants(G)] |- true => true }
{
    loop i in 0..count {
        operation()
    }
}

procedure caller()
    sequent { [fs::write] |- true => true }
{
    repeat::<fs::write>(10, || write_file("log.txt", [0; 1]))
}
```

### 9.3.5.3 Bounded Polymorphism

**Pattern**: Algorithms that work with capabilities from a specific domain.

**Example (informative)**:
```cursive
procedure io_processor<grants G>(path: string)
    sequent { [grants(G)] |- true => true }
    where G <: {fs::read, fs::write, fs::delete}
{
    // Can perform file operations, but not network or other operations
    perform_file_operation(path)
}
```

---

## 9.3.6 Grant Parameter Verification

### 9.3.6.1 Verification Algorithm

**Normative Statement 9.3.12**: A conforming implementation shall verify that:

1. Grant parameter references `grants(G)` refer to declared grant parameters
2. Grant parameter bounds are satisfied at instantiation
3. Instantiated grant sets are subsets of caller's grants
4. Procedure bodies respect the grant set including parameter grants

**Algorithm** (informative):

```
For each procedure P<grants G> with sequent { [grants(G), concrete_grants] |- ... }:
  1. Verify G is declared in generic parameters
  2. For each call site P<G_inst>:
     a. If G has bound B: verify G_inst <: B
     b. Verify G_inst <: grants(caller)
  3. For procedure body of P:
     a. Assume grants(P) = grants(G) ∪ concrete_grants
     b. Verify all operations in body respect grants(P)
```

### 9.3.6.2 Grant Parameter Scoping

**Normative Statement 9.3.13**: Grant parameters are scoped to the procedure declaration. They are not visible outside the procedure.

**Example (informative)**:
```cursive
procedure outer<grants G>()
    sequent { [grants(G)] |- true => true }
{
    procedure inner()
        // ERROR: G not in scope
        // sequent { [grants(G)] |- true => true }  // ERROR
    {
        // ...
    }
}
```

---

## 9.3.7 Advanced Grant Polymorphism

### 9.3.7.1 Higher-Order Grant Polymorphism

**Pattern**: Procedures that accept grant-polymorphic procedures as parameters.

**Example (informative)**:
```cursive
procedure map<T, U, grants G>(
    items: [T; 100],
    f: (T) -> U sequent { [grants(G)] |- true => true }
): [U; 100]
    sequent { [grants(G), alloc::heap] |- true => true }
{
    let result = heap_allocate_array<U>(100)
    loop i in 0..100 {
        result[i] = f(items[i])
    }
    result result
}
```

### 9.3.7.2 Grant Parameter Constraints

**Pattern**: Multiple bounds on a single grant parameter.

**Example (informative)**:
```cursive
procedure constrained<grants G>()
    sequent { [grants(G)] |- true => true }
    where G <: {fs::read, fs::write}
{
    // G must be subset of {fs::read, fs::write}
    // Valid instantiations: ∅, {fs::read}, {fs::write}, {fs::read, fs::write}
}
```

### 9.3.7.3 Grant Parameter Propagation

**Pattern**: Forwarding grant parameters through call chains.

**Example (informative)**:
```cursive
procedure low_level<grants G>(x: i32)
    sequent { [grants(G)] |- true => true }
{
    // Implementation
}

procedure mid_level<grants G>(x: i32)
    sequent { [grants(G), alloc::heap] |- true => true }
{
    let buffer = allocate(100)
    low_level::<G>(x)
}

procedure high_level()
    sequent { [fs::read, alloc::heap] |- true => true }
{
    mid_level::<fs::read>(42)
}
```

---

## 9.3.8 Limitations and Restrictions

### 9.3.8.1 No Grant Parameter Arithmetic

**Normative Statement 9.3.14**: Grant parameters do not support arithmetic operations (union, intersection, difference) within grant clauses.

**Example (informative - INVALID)**:
```cursive
// INVALID: No grant set arithmetic
procedure bad<grants G₁, grants G₂>()
    // sequent { [grants(G₁) ∪ grants(G₂)] |- true => true }  // ERROR: No union operator
{
    // ...
}
```

**Workaround**: Use explicit composition:
```cursive
// Valid: Explicit listing
procedure good<grants G₁, grants G₂>()
    sequent { [grants(G₁), grants(G₂)] |- true => true }
{
    // Implicitly: grants(G₁) ∪ grants(G₂)
}
```

### 9.3.8.2 No Negative Bounds

**Normative Statement 9.3.15**: Grant parameters do not support negative bounds (exclusion constraints).

**Example (informative - INVALID)**:
```cursive
// INVALID: No exclusion syntax
procedure bad<grants G>()
    sequent { [grants(G)] |- true => true }
    // where G ⊄ {fs::write}  // ERROR: No exclusion operator
{
    // ...
}
```

### 9.3.8.3 No First-Class Grant Sets

**Normative Statement 9.3.16**: Grant sets and grant parameters are not first-class values. They cannot be stored, returned, or manipulated as runtime values.

**Example (informative - INVALID)**:
```cursive
// INVALID: Grant sets are not values
// let g = grants(G)  // ERROR
// result g           // ERROR
```

---

## 9.3.9 Conformance Requirements

A conforming implementation shall:

1. **Parse grant parameters** (§9.3.1): Accept grant parameter declarations in generic parameter lists
2. **Support bounds** (§9.3.2): Verify grant parameter bounds at instantiation
3. **Verify instantiation** (§9.3.3): Check instantiated grants are subsets of caller grants
4. **Support composition** (§9.3.4): Allow combining grant parameters with concrete grants
5. **Scope correctly** (§9.3.6.2): Enforce proper scoping of grant parameters

**Optional Features**:
- Grant parameter inference (§9.3.3.2) - recommended but not required

---

**Previous**: [09-2_Grant-Tracking.md](09-2_Grant-Tracking.md) | **Next**: [09-4_Built-In-Grants.md](09-4_Built-In-Grants.md)
