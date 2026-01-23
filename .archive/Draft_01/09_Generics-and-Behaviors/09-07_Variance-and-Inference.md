# Cursive Language Specification

## Clause 9 — Generics and Behaviors

**Clause**: 9 — Generics and Behaviors
**File**: 10-7_Variance-and-Inference.md
**Section**: §9.7 Variance and Its Inference
**Stable label**: [generic.variance]  
**Forward references**: §10.2 [generic.parameter], §10.3 [generic.bounds], §10.6 [generic.resolution], Clause 7 §7.7 [type.relation], Clause 11 §11.4 [memory.permission]

---

### §9.7 Variance and Its Inference [generic.variance]

#### §9.7.1 Overview

[1] Variance describes how subtyping of type parameters relates to subtyping of generic type constructors. Understanding variance is essential for determining when generic types may substitute for one another and for ensuring type safety in the presence of mutable references and permissions.

[2] Cursive employs a conservative variance model: most type constructors are invariant, ensuring that generic types are used with precise type arguments. Variance becomes relevant primarily for callable types, where contravariance in parameters and covariance in results enable flexible higher-order programming.

[3] This subclause defines variance categories, specifies variance for each Cursive type constructor, describes the inference algorithm, and provides examples illustrating variance in practice.

#### §9.7.2 Variance Definitions [generic.variance.definitions]

##### §9.7.2.1 Covariance

[4] A type constructor `F` is _covariant_ in parameter `T` when subtyping is preserved:

$$
\frac{\tau <: \upsilon}{F\langle\tau\rangle <: F\langle\upsilon\rangle}
\tag{Variance-Covariant}
$$

[5] Covariant positions allow substituting more specific types with more general types. Example: if `F` is covariant and `Cat <: Animal`, then `F<Cat> <: F<Animal>`.

##### §9.7.2.2 Contravariance

[6] A type constructor `F` is _contravariant_ in parameter `T` when subtyping is reversed:

$$
\frac{\tau <: \upsilon}{F\langle\upsilon\rangle <: F\langle\tau\rangle}
\tag{Variance-Contravariant}
$$

[7] Contravariant positions flip the subtyping direction. Example: if `F` is contravariant and `Cat <: Animal`, then `F<Animal> <: F<Cat>`.

##### §9.7.2.3 Invariance

[8] A type constructor `F` is _invariant_ in parameter `T` when no subtyping relationship holds except through equivalence:

$$
\frac{\tau \equiv \upsilon}{F\langle\tau\rangle <: F\langle\upsilon\rangle} \quad \text{(no other subtyping)}
\tag{Variance-Invariant}
$$

[9] Invariant positions require exact type matching. Most Cursive type constructors are invariant to preserve safety with mutable references and permissions.

#### §9.7.3 Variance Table [generic.variance.table]

[10] Table 10.7.1 specifies variance for all Cursive type constructors:

**Table 10.7.1 — Type constructor variance**

| Type Constructor                    | Variance in Parameters                              | Rationale                                        |
| ----------------------------------- | --------------------------------------------------- | ------------------------------------------------ |
| Primitives (`i32`, `bool`, etc.)    | N/A (not generic)                                   | No parameters                                    |
| Tuples `(T₁, ..., Tₙ)`              | Invariant in all `Tᵢ`                               | Mutation requires exact type matching            |
| Records `Record<T>`                 | Invariant in all parameters                         | Nominal types, field mutation requires exactness |
| Enums `Enum<T>`                     | Invariant in all parameters                         | Variant safety, discriminant integrity           |
| Modal types `Modal<T>@State`        | Invariant in `T`, state-specific                    | State machine integrity                          |
| Arrays `[T; n]`                     | Invariant in `T`                                    | Element mutation requires type preservation      |
| Slices `[T]`                        | Invariant in `T`                                    | Mutation through slices requires exact type      |
| Pointers `Ptr<T>@State`             | Invariant in `T`, covariant in state widening       | Dereference safety, modal subtyping (§7.7.3)     |
| Union types `T₁ \/ T₂ \/ ... \/ Tₙ` | Covariant in all `Tᵢ`                               | Each `Tᵢ <: T₁ \/ ... \/ Tₙ` by construction     |
| Callable types (parameters)         | Contravariant in each parameter `Tᵢ`                | Accept more general inputs                       |
| Callable types (return)             | Covariant in return type `R`                        | Return more specific outputs                     |
| Callable types (grants)             | Contravariant in grant set `ε` (fewer is stronger)  | Fewer grants = more specific callable            |
| Callable types (must)               | Contravariant in preconditions                      | Weaker preconditions accept more inputs          |
| Callable types (will)               | Covariant in postconditions                         | Stronger postconditions guarantee more           |
| Permission qualification            | Covariant in downgrade: `unique <: shared <: const` | Permission lattice (§11.4.3)                     |

[11] Generic parameters in user-defined types default to invariant unless the type constructor is purely read-only and structurally covariant.

#### §9.7.4 Callable Type Variance [generic.variance.callable]

##### §9.7.4.1 Function Subtyping with Variance

[12] Callable types exhibit complex variance patterns captured by the subtyping rule:

$$
\frac{
\begin{array}{c}
\upsilon_1 <: \tau_1 \quad \cdots \quad \upsilon_n <: \tau_n \\
\tau_r <: \upsilon_r \\
\varepsilon_1 \subseteq \varepsilon_2 \\
M_2 \Rightarrow M_1 \quad W_1 \Rightarrow W_2
\end{array}
}{
(\tau_1, \ldots, \tau_n) \to \tau_r \; ! \varepsilon_1 \; [[ |- M_1 => W_1 ]]
<:
(\upsilon_1, \ldots, \upsilon_n) \to \upsilon_r \; ! \varepsilon_2 \; [[ |- M_2 => W_2 ]]
}
\tag{Sub-Function-Variance}
$$

[13] Variance summary for callable types:

- **Parameters**: Contravariant (accept more general inputs)
- **Return**: Covariant (return more specific outputs)
- **Grants**: Contravariant (fewer grants is stronger/more specific)
- **Preconditions (must)**: Contravariant (weaker preconditions accept more)
- **Postconditions (will)**: Covariant (stronger postconditions guarantee more)

##### §9.7.4.2 Examples

**Example 10.7.4.1 (Function parameter contravariance):**

```cursive
record Animal {
    name: string@Managed,
}

record Cat {
    name: string@Managed,
    meow_volume: i32,
}

// Assuming Cat <: Animal (structural subtyping hypothetically)
// (Animal) -> () is a supertype of (Cat) -> ()
// Functions accepting animals can accept functions expecting cats? No.

// In Cursive: nominal types are invariant
// This example illustrates the principle, not actual Cursive subtyping
```

**Example 10.7.4.2 (Function grant contravariance):**

```cursive
// Pure function (no grants)
let pure: (i32) -> i32 = |x| { result x * 2 }

// Function with grants
let writer: (i32) -> i32 ! {io::write} = pure

// Subtyping: (i32) -> i32 ! ∅ <: (i32) -> i32 ! {io::write}
// Pure function (fewer grants) is more specific than grant-requiring function
```

#### §9.7.5 Permission Variance [generic.variance.permission]

##### §9.7.5.1 Permission Lattice Variance

[14] Permission qualifiers exhibit covariant subtyping following the permission lattice (§11.4.3):

$$
\text{unique } T <: \text{shared } T <: \text{const } T
\tag{Sub-Permission}
$$

[15] More restrictive permissions subtype less restrictive permissions. This allows passing `unique` values where `shared` or `const` is expected (permission downgrade).

##### §9.7.5.2 Permission in Generic Types

[16] Permission annotations compose with generic type parameters:

```cursive
record Container<T> {
    data: Ptr<T>@Valid,
}

let unique_container: unique Container<i32> = Container::new()
let shared_container: shared Container<i32> = unique_container  // Downgrade
let const_container: const Container<i32> = shared_container   // Downgrade
```

[17] The generic parameter `T` remains invariant; permission downgrades apply to the binding, not the type constructor.

#### §9.7.6 Variance Inference [generic.variance.inference]

##### §9.7.6.1 Variance Inference Algorithm

[18] For user-defined generic types, variance is inferred by analyzing how type parameters appear in the type's definition:

```
infer_variance(type_definition, param):
    occurrences = find_all_occurrences(param, type_definition)

    for occurrence in occurrences:
        position = classify_position(occurrence)
        match position:
            CovariantPosition:
                contribute Covariant
            ContravariantPosition:
                contribute Contravariant
            InvariantPosition:
                contribute Invariant

    combine contributions:
        if all Covariant:
            return Covariant
        else if all Contravariant:
            return Contravariant
        else:
            return Invariant  // Mixed or any invariant forces invariant
```

##### §9.7.6.2 Position Classification

[19] Type parameter occurrences are classified by position:

- **Covariant positions**: Return types, immutable fields (if type is read-only)
- **Contravariant positions**: Procedure parameter types, callable parameter types
- **Invariant positions**: Mutable fields, array elements, slice elements, record fields

[20] Any occurrence in an invariant position forces the entire parameter to be invariant.

##### §9.7.6.3 Inferred Variance is Conservative

[21] The inference algorithm is conservative: when in doubt, it defaults to invariant. This preserves safety at the cost of some flexibility.

[ Note: User-defined types cannot override inferred variance. Variance is a property derived from the type's structure, not a declared attribute. This ensures that variance annotations cannot introduce unsoundness.
— end note ]

#### §9.7.7 Variance Safety [generic.variance.safety]

##### §9.7.7.1 Safety Rationale

[22] Invariance in most positions prevents type confusion through mutable references:

```cursive
// Hypothetical (if arrays were covariant):
// let cats: [Cat] = [Cat::new(), Cat::new()]
// let animals: [Animal] = cats  // If covariant
// animals[0] = Dog::new()       // Store Dog in Cat array!
// let cat: Cat = cats[0]        // Type confusion!

// Cursive: arrays are invariant, preventing this
```

[23] By making mutable structures invariant, Cursive ensures that references to generic containers remain type-safe.

##### §9.7.7.2 Read-Only Covariance

[24] Purely read-only structures could be covariant, but Cursive defaults to invariance for user-defined types. The only built-in covariant structure is the discriminated union `\/`:

$$
\frac{\tau_i <: \tau_1 \/ \cdots \/ \tau_n}{F\langle\tau_i\rangle <: F\langle\tau_1 \/ \cdots \/ \tau_n\rangle}
\tag{Variance-Union-Covariant}
$$

[25] Union types are inherently covariant because each component is a subtype of the union by construction (§7.3.6, §7.7.3).

#### §9.7.8 Variance and Permissions [generic.variance.permissions]

##### §9.7.8.1 Permission Impact on Variance

[26] Permission qualifiers affect variance through the subtyping lattice. While generic type parameters remain invariant, the permission applied to the entire type can vary:

```cursive
record Buffer<T> {
    data: [T],
}

// T is invariant (Buffer<i32> ≠ Buffer<i64>)
// But permissions downgrade:
let unique_buf: unique Buffer<i32> = Buffer::new()
let shared_buf: shared Buffer<i32> = unique_buf  // Permission downgrade
```

[27] The type parameter invariance is separate from permission covariance. Both systems compose without interference.

##### §9.7.8.2 Permission Variance Rule

[28] Permission downgrades are always safe regardless of type parameter variance:

$$
\frac{P_1 <: P_2 \text{ in permission lattice}}{P_1\, F\langle\tau\rangle <: P_2\, F\langle\tau\rangle}
\tag{Variance-Permission-Downgrade}
$$

where $P_1, P_2 \in \{\text{unique}, \text{shared}, \text{const}\}$ and $P_1 <: P_2$ follows the permission lattice (§11.4.3).

#### §9.7.9 Variance Examples [generic.variance.examples]

**Example 10.7.9.1 (Invariant parameters in records):**

```cursive
record Container<T> {
    items: [T],
}

let int_container: Container<i32> = Container { items: [1, 2, 3] }

// Invalid: Container is invariant in T
// let num_container: Container<i64> = int_container
// error[E07-700]: type mismatch, Container<i32> ≠ Container<i64>
```

**Example 10.7.9.2 (Function variance):**

```cursive
// Callable contravariant in parameters, covariant in results

procedure accepts_callback(f: (i32) -> string@View)
{
    let result = f(42)
}

// More specific parameter, more specific return
let specific: (i16) -> string@Managed = |x: i16| [[ alloc::heap ]] {
    result string.from_int(x as i32)
}

// Less specific parameter, less specific return
let general: (i64) -> string@View = |x: i64| {
    result "value"
}

// Variance allows flexible callable substitution
```

**Example 10.7.9.3 (Grant variance in callables):**

```cursive
procedure run_pure(f: () -> i32): i32
{
    result f()
}

let pure_fn = || { result 42 }
run_pure(pure_fn)  // ∅ ⊆ ∅ ✓

// Cannot pass grant-requiring function to pure context
let grant_fn = || [[ io::write |- true => true ]] {
    println("computing")
    result 42
}
// run_pure(grant_fn)  // error: {io::write} ⊄ ∅
```

**Example 10.7.9.4 (Union covariance):**

```cursive
let value: i32 = 42
let union: i32 \/ string@Managed = value  // i32 <: i32 \/ string@Managed

procedure process(input: i32 \/ string@View \/ bool)
{
    match input {
        n: i32 => println("Number: {}", n),
        s: string@View => println("String: {}", s),
        b: bool => println("Boolean: {}", b),
    }
}

let narrow: i32 \/ string@View = 42
process(narrow)  // OK: i32 \/ string@View <: i32 \/ string@View \/ bool
```

**Example 10.7.9.5 (Permission and type variance composition):**

```cursive
record Data<T> {
    value: T,
}

let unique_data: unique Data<i32> = Data { value: 42 }
let shared_data: shared Data<i32> = unique_data      // Permission downgrade ✓
// let wrong: shared Data<i64> = unique_data         // Type invariance violation ✗
```

#### §9.7.10 Variance Checking [generic.variance.checking]

##### §9.7.10.1 Subtyping with Generic Types

[30] When checking whether `F<τ₁> <: F<τ₂>`, the compiler:

1. Retrieves variance of `F` in its parameter
2. Applies variance rule:
   - Covariant: check `τ₁ <: τ₂`
   - Contravariant: check `τ₂ <: τ₁`
   - Invariant: check `τ₁ ≡ τ₂`

[31] For multi-parameter generics, each parameter is checked independently and all must satisfy their variance requirements.

##### §9.7.10.2 Variance Checking Rule

$$
\frac{
\begin{array}{c}
F \text{ has variance } V_1, \ldots, V_n \text{ in parameters } \alpha_1, \ldots, \alpha_n \\
\forall i. \; \text{check}(V_i, \tau_i, \upsilon_i) \text{ succeeds}
\end{array}
}{F\langle\tau_1, \ldots, \tau_n\rangle <: F\langle\upsilon_1, \ldots, \upsilon_n\rangle}
\tag{WF-Variance-Check}
$$

where:

$$
\text{check}(V, \tau, \upsilon) = \begin{cases}
\tau <: \upsilon & \text{if } V = \text{Covariant} \\
\upsilon <: \tau & \text{if } V = \text{Contravariant} \\
\tau \equiv \upsilon & \text{if } V = \text{Invariant}
\end{cases}
$$

##### §9.7.10.3 Variance Violations

[32] Attempting subtyping that violates variance produces diagnostic E10-701:

```cursive
let container: Container<i32> = Container::new()
// let wrong: Container<i64> = container  // error[E10-701]: variance violation
```

[33] The diagnostic includes the type constructor, its variance, and the attempted substitution.

#### §9.7.11 Variance Inference Algorithm [generic.variance.inference]

##### §9.7.11.1 Algorithm Specification

[34] Variance inference analyzes type definitions to determine parameter variance:

```
infer_variance(type T<α₁, ..., αₙ>):
    for each parameter αᵢ:
        positions = []

        // Collect all positions where αᵢ appears
        for occurrence of αᵢ in T's definition:
            classify occurrence as Covariant, Contravariant, or Invariant
            append to positions

        // Combine positions
        if all positions are Covariant:
            variance[αᵢ] = Covariant
        else if all positions are Contravariant:
            variance[αᵢ] = Contravariant
        else:
            variance[αᵢ] = Invariant

    return variance
```

##### §9.7.11.2 Position Classification Rules

[35] Type parameter positions are classified recursively:

- **Covariant context**:

  - Return type of procedure
  - Right side of union `\/`
  - Inside covariant type constructor
  - Negated contravariant position (double flip)

- **Contravariant context**:

  - Parameter type of procedure
  - Inside contravariant type constructor
  - Negated covariant position

- **Invariant context**:
  - Mutable field in record
  - Array element type `[T; n]`
  - Slice element type `[T]`
  - Any position in invariant type constructor

[36] Once a parameter appears in an invariant position anywhere in the type definition, the parameter is invariant for the entire type.

##### §9.7.11.3 Examples

**Example 10.7.11.1 (Variance inference for read-only wrapper):**

```cursive
record ReadOnly<T> {
    value: const T,    // Immutable field
}

// Variance inference:
// T appears in immutable field position
// But records are nominal and default to invariant
// Result: ReadOnly is invariant in T

// Even though semantically could be covariant, structural analysis forces invariant
```

**Example 10.7.11.2 (Callable parameter variance):**

```cursive
record Callback<T, U> {
    func: (T) -> U,
}

// Variance inference:
// T appears in parameter position of func (contravariant)
// U appears in return position of func (covariant)
// func is in immutable field (would preserve variance)
// BUT: Callback is nominal record → invariant in all parameters
// Result: Callback is invariant in both T and U
```

#### §9.7.12 Diagnostics [generic.variance.diagnostics]

[37] Variance-related diagnostics:

| Code    | Condition                                          |
| ------- | -------------------------------------------------- |
| E10-701 | Variance violation in generic type substitution    |
| E10-702 | Conflicting variance requirements (internal error) |

#### §9.7.13 Conformance Requirements [generic.variance.requirements]

[38] Implementations shall:

1. Enforce variance rules per Table 10.7.1 for all type constructors
2. Implement variance inference algorithm for user-defined generic types
3. Default user-defined types to invariant unless proven otherwise
4. Check variance during subtyping (§7.7) and type checking
5. Support permission variance through permission lattice (§11.4.3)
6. Recognize callable types as contravariant in parameters, covariant in results
7. Support grant variance (fewer grants is more specific)
8. Enforce precondition contravariance and postcondition covariance
9. Detect variance violations and emit E10-701 with detailed substitution info
10. Integrate variance with monomorphization (§10.6) and bound checking (§10.3)
11. Document variance behavior in implementation guidance

---

**Previous**: §10.6 Resolution and Monomorphization (§10.6 [generic.resolution]) | **Next**: Clause 11 — Memory Model, Regions, and Permissions (§11.1 [memory.overview])
