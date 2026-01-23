# Cursive Language Specification

## Clause 9 — Generics and Behaviors

**Clause**: 9 — Generics and Behaviors
**File**: 10-6_Resolution-and-Monomorphization.md
**Section**: §9.6 Resolution and Monomorphization
**Stable label**: [generic.resolution]  
**Forward references**: §10.2 [generic.parameter], §10.3 [generic.bounds], §10.4 [generic.behavior], §10.5 [generic.implementation], §10.7 [generic.variance], Clause 2 §2.2 [lex.phases], Clause 7 [type], Clause 8 [expr], Annex E §E.2 [implementation.algorithms]

---

### §9.6 Resolution and Monomorphization [generic.resolution]

#### §9.6.1 Overview

[1] Resolution is the process of determining concrete type, const, and grant arguments for generic parameters. Monomorphization generates specialized code for each distinct instantiation, ensuring zero runtime overhead for generic abstractions.

[2] This subclause specifies the resolution algorithm, type argument inference, explicit instantiation with turbofish syntax, monomorphization semantics, and the interaction with behavior dispatch.

[3] Resolution occurs during the type-checking phase (§2.2.4.3) after name resolution completes. Monomorphization occurs during code generation (§2.2.4.4), producing specialized procedures and types for each instantiation.

#### §9.6.2 Resolution Algorithm [generic.resolution.algorithm]

##### §9.6.2.1 Resolution Overview

[4] Generic resolution determines arguments for type, const, and grant parameters through:

1. **Explicit arguments**: Turbofish syntax `function::<Type>()`
2. **Inference from call site**: Argument types, return type context
3. **Inference from bounds**: Associated type constraints, grant usage
4. **Default values**: Parameters with defaults (§10.2.6)

[5] Resolution attempts these strategies in order. If all fail, diagnostic E09-601 requests explicit type arguments.

##### §9.6.2.2 Resolution Judgment

[6] Resolution is formalized as:

$$
\text{resolve}(\Gamma, e, \langle\alpha_1, \ldots, \alpha_n\rangle) = \langle\tau_1, \ldots, \tau_n\rangle
$$

where:

- $\Gamma$ is the typing environment
- $e$ is the expression requiring resolution
- $\langle\alpha_1, \ldots, \alpha_n\rangle$ are generic parameters
- $\langle\tau_1, \ldots, \tau_n\rangle$ are resolved concrete arguments

[7] The resolution algorithm is specified in Annex E §E.2.4 [implementation.algorithms.predicate]. The following paragraphs summarize the normative requirements.

##### §9.6.2.3 Explicit Type Arguments

[8] When explicit type arguments are provided via turbofish syntax, resolution uses them directly:

$$
\frac{\text{call } f::\langle\tau_1, \ldots, \tau_n\rangle(\ldots)}{\text{resolve}(f) = \langle\tau_1, \ldots, \tau_n\rangle}
\tag{Resolve-Explicit}
$$

[9] Explicit arguments bypass inference. The compiler validates that:

- Argument count matches parameter count (or defaults fill remaining)
- Each argument satisfies the corresponding parameter's bounds
- Arguments are well-formed types in the current context

##### §9.6.2.4 Type Inference from Arguments

[10] When explicit arguments are absent, the compiler infers type parameters from procedure arguments:

$$
\frac{
\begin{array}{c}
\text{procedure } f\langle T \rangle(x: T) \\
\text{call } f(e) \quad \Gamma \vdash e : \tau
\end{array}
}{\text{resolve}(T) = \tau}
\tag{Resolve-From-Arg}
$$

[11] The unification algorithm matches formal parameter types against actual argument types, solving for type variables.

##### §9.6.2.5 Type Inference from Context

[12] Return type context contributes to inference:

$$
\frac{
\begin{array}{c}
\text{procedure } f\langle T \rangle() : T \\
\text{binding } let x: \tau = f() \\
\end{array}
}{\text{resolve}(T) = \tau}
\tag{Resolve-From-Context}
$$

[13] Bidirectional type checking (§8.1.6) flows contextual type information into the resolution algorithm.

##### §9.6.2.6 Const Parameter Inference

[14] Const parameters are inferred from:

- Explicit arguments: `buffer::<1024>()`
- Array length context: `let arr: [T; 512] = create_array()` infers `N = 512`
- Default values when no other information is available

[15] Const inference follows the same unification rules as type inference, treating const values as compile-time constants.

##### §9.6.2.7 Grant Parameter Inference

[16] Grant parameters are inferred from callable arguments:

$$
\frac{
\begin{array}{c}
\text{procedure } g\langle\varepsilon\rangle(f: () \to T \; ! \varepsilon) \\
\text{call } g(h) \quad h : () \to U \; ! \varepsilon'
\end{array}
}{\text{resolve}(\varepsilon) = \varepsilon'}
\tag{Resolve-Grant}
$$

[17] The grant set of the actual callable argument becomes the grant parameter's value.

##### §9.6.2.8 Inference Failure

[18] When inference cannot determine a unique solution, the compiler emits diagnostic E09-601 requesting explicit type arguments:

$$
\frac{\text{resolve}(\Gamma, e, \langle\alpha_1, \ldots\rangle) = \text{ambiguous}}{\text{ERROR E09-601: cannot infer type arguments}}
\tag{Resolve-Fail}
$$

[19] The diagnostic includes:

- Which parameters could not be inferred
- Partial inference results (if any)
- Suggested turbofish syntax with inferred and placeholder arguments

#### §9.6.3 Bound Checking [generic.resolution.bounds]

[20] After type arguments are resolved, the compiler checks that each argument satisfies its parameter's bounds:

```
check_bounds(params, args):
    for each (param, arg) in zip(params, args):
        for bound in bounds(param):
            if bound is behavior P:
                verify: behavior P for arg exists
                if not: error E09-602
            if bound is contract C:
                verify: arg: C declared
                if not: error E09-603
            if bound is associated type equality T::A = U:
                resolve: arg::A
                verify: arg::A ≡ U
                if not: error E09-604
            if bound is grant bound ε ⊆ G:
                verify: arg ⊆ G
                if not: error E09-605
```

[21] Bound violations produce diagnostics identifying the parameter, bound, and actual argument.

#### §9.6.4 Monomorphization [generic.resolution.monomorphization]

##### §9.6.4.1 Overview

[22] Monomorphization generates specialized code for each distinct instantiation of a generic procedure or type. The process occurs during code generation (§2.2.4.4) after type checking completes.

[23] Each instantiation `Generic<ConcreteArgs>` produces a separate entity in the generated code. Generic source code exists only at compile time; runtime code contains only monomorphized specializations.

##### §9.6.4.2 Monomorphization Process

[24] The monomorphization algorithm:

```
monomorphize(entity, type_args, const_args, grant_args):
    1. Substitute type parameters with type arguments
    2. Substitute const parameters with const values
    3. Substitute grant parameters with grant sets
    4. Resolve associated type projections to concrete types
    5. Generate specialized entity with substituted types
    6. Perform type checking on specialized entity
    7. Emit specialized code
```

[25] **Substitution judgment**:

$$
\frac{
\begin{array}{c}
\text{procedure } f\langle T, \text{const } N, \varepsilon \rangle \\
\text{instantiation } f\langle\tau, n, G\rangle
\end{array}
}{f\langle\tau, n, G\rangle \equiv f[\alpha \mapsto \tau, N \mapsto n, \varepsilon \mapsto G]}
\tag{Mono-Subst}
$$

[26] After substitution, the specialized entity is a concrete procedure or type with no remaining generic parameters.

##### §9.6.4.3 Distinct Instantiations

[27] Each distinct combination of type, const, and grant arguments produces a separate instantiation:

```cursive
procedure identity<T>(value: T): T
{
    result value
}

let a = identity(42)          // Instantiation 1: T = i32
let b = identity("hello")     // Instantiation 2: T = string@View
let c = identity(3.14)        // Instantiation 3: T = f64
```

[28] Three instantiations are generated: `identity_i32`, `identity_string_View`, `identity_f64` (names are illustrative; actual mangled names are implementation-defined).

##### §9.6.4.4 Code Sharing and Optimization

[29] Implementations may share code between compatible instantiations provided observable behavior is preserved. For example, pointer instantiations `Generic<T>` and `Generic<U>` with identical layouts may share compiled code.

[30] Such sharing is a quality-of-implementation optimization and shall not affect semantics. Programs must behave as if each instantiation were compiled independently.

##### §9.6.4.5 Monomorphization and Behaviors

[31] Behavior procedure calls are monomorphized: the compiler selects the specific implementation for the concrete type and generates a direct call:

$$
\frac{
\begin{array}{c}
\Gamma \vdash e : T \quad \text{behavior } P \text{ for } T \text{ exists} \\
\text{behavior } P \{ \text{procedure } m \}
\end{array}
}{\Gamma \vdash e.m() \Rightarrow T.m() \text{ (direct call)}}
\tag{Mono-Pred-Call}
$$

[32] No vtable lookup or indirection occurs; behavior dispatch is fully static.

#### §9.6.5 Associated Type Resolution [generic.resolution.assoc]

##### §9.6.5.1 Projection Resolution

[33] Associated type projections `T::AssocType` are resolved during monomorphization:

```
resolve_projection(T::A):
    1. Resolve T to concrete type τ
    2. Find behavior or contract implementation for τ
    3. Extract associated type specification: type A = υ
    4. Replace T::A with υ
    5. Recursively resolve if υ contains projections
```

[34] **Resolution rule**:

$$
\frac{
\begin{array}{c}
\Gamma \vdash T \equiv \tau \quad \text{behavior } P \text{ for } \tau \{ \text{type } A = \upsilon \}
\end{array}
}{\Gamma \vdash T::A \equiv \upsilon}
\tag{Resolve-Assoc}
$$

##### §9.6.5.2 Nested Projections

[35] Projections may be nested: `T::Container::Element`. Resolution proceeds left-to-right:

1. Resolve `T::Container` to type `τ₁`
2. Resolve `τ₁::Element` to type `τ₂`
3. Final result is `τ₂`

[36] Cyclic projections are detected during resolution and produce diagnostic E09-606.

##### §9.6.5.3 Examples

**Example 10.6.5.1 (Associated type resolution):**

```cursive
behavior Container {
    type Element = ()

    procedure get(~, index: usize): Element
    { result () }
}

behavior Container for List<T> {
    type Element = T

    procedure get(~, index: usize): T
    {
        result self.data[index]
    }
}

procedure first<C>(container: C): C::Element
    where C: Container
{
    result container.get(0)
}

let list: List<i32> = List::from([1, 2, 3])
let value = first(list)
// C = List<i32>
// C::Element = i32 (via projection resolution)
// value: i32
```

#### §9.6.6 Zero-Cost Abstraction [generic.resolution.zerocost]

##### §9.6.6.1 Zero-Cost Guarantee

[37] Generic abstractions shall impose no runtime overhead beyond the equivalent hand-written specialized code. Monomorphization ensures:

- No indirection or vtable lookup for behavior calls
- No runtime type information for generic parameters
- No boxing or allocation for generic values
- Identical assembly output to specialized non-generic code

[38] **Performance property**:

$$
\text{perf}(\text{generic code}) = \text{perf}(\text{hand-specialized equivalent})
\tag{Prop-Zero-Cost}
$$

[39] The only cost is compile time (monomorphization analysis) and code size (multiple instantiations).

##### §9.6.6.2 Code Size Implications

[40] Each distinct instantiation generates separate machine code. Programs with many generic instantiations may have larger binaries:

```cursive
procedure process<T>(value: T): T
{
    result value
}

// 100 different types used with process
// → 100 instantiations in binary
```

[41] Implementations may apply code sharing optimizations (§10.6.4.4) provided semantics are preserved. Programs requiring minimal code size should limit generic usage or use dynamic dispatch via witnesses (Clause 12).

#### §9.6.7 Behavior Method Resolution [generic.resolution.behavior]

##### §9.6.7.1 Resolution Algorithm

[42] When a behavior method is called, resolution proceeds:

```
resolve_predicate_call(receiver_type, behavior, method):
    1. Determine concrete type τ of receiver
    2. Find behavior implementation: behavior P for τ
    3. Locate method in implementation:
       a. Check for override in implementation body
       b. If not found, use default from behavior declaration
    4. Generate direct call to selected method
    5. Validate grants and permissions
```

[43] Method resolution is deterministic and occurs at compile time. No runtime dispatch is involved.

##### §9.6.7.2 Override Selection

[44] When a type overrides a behavior method, the override takes precedence:

$$
\frac{
\begin{array}{c}
\text{behavior } P \text{ for } \tau \{ \text{procedure } m \{ \text{body}_{\text{override}} \} \} \\
\text{predicate } P \{ \text{procedure } m \{ \text{body}_{\text{default}} \} \}
\end{array}
}{\text{call } \tau.m() \Rightarrow \text{body}_{\text{override}}}
\tag{Resolve-Override}
$$

[45] Inherited defaults are used only when no override is provided:

$$
\frac{
\begin{array}{c}
\text{behavior } P \text{ for } \tau \{ \text{no procedure } m \} \\
\text{predicate } P \{ \text{procedure } m \{ \text{body}_{\text{default}} \} \}
\end{array}
}{\text{call } \tau.m() \Rightarrow \text{body}_{\text{default}}[\text{Self} \mapsto \tau]}
\tag{Resolve-Inherit}
$$

##### §9.6.7.3 Blanket Implementation Resolution

[46] When multiple behavior implementations could apply (specific vs blanket), resolution prefers specific implementations:

1. Check for direct implementation: `behavior P for ConcreteType`
2. If not found, check blanket implementations matching type's bounds
3. If multiple blankets match, error E09-607 (ambiguous implementation)
4. If no implementation found, error E09-608 (behavior not satisfied)

[47] This ordering ensures that specific implementations can specialize behavior without conflicting with blanket defaults.

##### §9.6.7.4 Examples

**Example 10.6.7.1 (Method resolution with override):**

```cursive
behavior Display {
    procedure show(~): string@View
    {
        result type_name::<Self>()  // Default
    }
}

record Point with Display {
    x: f64,
    y: f64,

    procedure show(~): string@View
    {
        result "Point"  // Override
    }
}

record Generic with Display {
    // No override - uses inherited default
}

println("{}", Point{x:1.0, y:2.0}.show())  // "Point" (override)
println("{}", Generic{}.show())             // "Generic" (inherited default)
```

#### §9.6.8 Constraint Solving [generic.resolution.constraints]

##### §9.6.8.1 Constraint Generation

[48] During type checking, the compiler generates constraints for generic parameters:

```
Constraint ::= τ₁ ≡ τ₂              (equality)
             | τ : Behavior         (bound)
             | τ::A ≡ υ              (associated type)
             | ε ⊆ G                 (grant subset)
```

[49] Constraints are collected from:

- Procedure call arguments and return types
- Pattern matches and variable bindings
- Behavior bounds and where clauses
- Associated type projections

##### §9.6.8.2 Constraint Solving

[50] The solver attempts to find a solution satisfying all constraints:

```
solve_constraints(constraints):
    unification_map = {}

    // Phase 1: Equality constraints
    for constraint (τ₁ ≡ τ₂):
        unify(τ₁, τ₂, unification_map)
        if unification fails:
            error E09-609 (type mismatch)

    // Phase 2: Bound constraints
    for constraint (τ : P):
        verify: behavior P for τ exists after substitution
        if not found:
            error E09-602 (unsatisfied bound)

    // Phase 3: Associated type constraints
    for constraint (τ::A ≡ υ):
        resolve: τ::A to concrete type υ'
        verify: υ' ≡ υ
        if not: error E10-604

    // Phase 4: Grant constraints
    for constraint (ε ⊆ G):
        verify: ε ⊆ G after substitution
        if not: error E10-605

    return unification_map
```

[51] The algorithm is specified formally in Annex E §E.2.4 with correctness proofs.

##### §9.6.8.3 Principal Type

[52] When constraint solving succeeds, the result is the _principal type_ (§1.3[25]): the most general type satisfying all constraints.

$$
\text{principal}(\Gamma, e) = \tau \quad \text{iff} \quad \Gamma \vdash e : \tau \text{ and } \forall \tau'. \; \Gamma \vdash e : \tau' \Rightarrow \tau' <: \tau
\tag{Principal-Type}
$$

[53] The principal type is unique when it exists. Ambiguous cases where no principal type exists produce diagnostic E09-610.

#### §9.6.9 Monomorphization Instantiation [generic.resolution.instantiation]

##### §9.6.9.1 Instantiation Cache

[54] The compiler maintains a cache of instantiated generics. When a generic is used with previously seen arguments, the existing instantiation is reused:

```
instantiate(generic, args):
    cache_key = (generic, args)
    if cache_key in instantiation_cache:
        return instantiation_cache[cache_key]

    specialized = monomorphize(generic, args)
    instantiation_cache[cache_key] = specialized
    return specialized
```

[55] This ensures that `List<i32>` used in multiple locations generates code only once.

##### §9.6.9.2 Recursive Instantiation

[56] Generic types may recursively instantiate themselves:

```cursive
record Tree<T> {
    value: T,
    left: Ptr<Tree<T>>@Valid,
    right: Ptr<Tree<T>>@Valid,
}
```

[57] Recursive instantiation is permitted provided the recursion is guarded by indirection (pointers). Direct recursion without indirection creates infinite-size types and is rejected (diagnostic E10-611, inherited from §5.5.3[15]).

##### §9.6.9.3 Monomorphization Limits

[58] Implementations shall support at least:

- 1024 distinct instantiations of a single generic entity
- 32 levels of nested generic instantiation depth
- 256 generic parameters in a single entity

[59] Exceeding these limits may produce diagnostic E10-612 (monomorphization limit exceeded). Implementations should document actual limits.

#### §9.6.10 Diagnostics [generic.resolution.diagnostics]

[60] Resolution and monomorphization diagnostics:

| Code    | Condition                                                |
| ------- | -------------------------------------------------------- |
| E10-601 | Cannot infer type arguments; explicit turbofish required |
| E10-602 | Type argument does not satisfy behavior bound            |
| E10-603 | Type argument does not satisfy contract bound            |
| E10-604 | Associated type constraint not satisfied                 |
| E10-605 | Grant parameter exceeds bound                            |
| E10-606 | Cyclic associated type projection                        |
| E10-607 | Ambiguous blanket implementation (multiple match)        |
| E10-608 | Behavior not satisfied for type                          |
| E10-609 | Type unification failed                                  |
| E10-610 | No principal type exists (ambiguous inference)           |
| E10-611 | Infinite-size recursive type                             |
| E10-612 | Monomorphization limit exceeded                          |

#### §9.6.11 Examples [generic.resolution.examples]

**Example 10.6.11.1 (Complete resolution example):**

```cursive
behavior Converter<U> {
    procedure convert(~): U
    {
        panic("not implemented")
    }
}

behavior Converter<string@Managed> for i32 {
    procedure convert(~): string@Managed
        [[ alloc::heap |- true => true ]]
    {
        result string.from_int(self)
    }
}

procedure transform<T, U>(value: T): U
    where T: Converter<U>
{
    result value.convert()
}

let s: string@Managed = transform(42)
// T inferred as i32 from argument
// U inferred as string@Managed from return context
// Bound check: i32: Converter<string@Managed> ✓
// Monomorphize: transform_i32_string_Managed
// Behavior call: 42.convert() → i32::convert() → direct call
```

**Example 10.6.11.2 (Grant parameter resolution):**

```cursive
procedure apply<T, U, ε>(value: T, f: (T) -> U ! ε): U
    [[ ε |- true => true ]]
{
    result f(value)
}

let pure = |x: i32| { result x * 2 }
let doubled = apply(5, pure)
// T = i32 (from first argument)
// U = i32 (from closure return)
// ε = ∅ (from closure grant set)
// Monomorphize: apply_i32_i32_empty

let logger = |x: i32| [[ io::write |- true => true ]] {
    println("{}", x)
    result x
}
let logged = apply(5, logger)
// T = i32
// U = i32
// ε = {io::write}
// Monomorphize: apply_i32_i32_io_write
```

**Example 10.6.11.3 (Inference failure requiring turbofish):**

```cursive
procedure convert<T, U>(): U
    where T: Into<U>
{
    result T::default().into()
}

// error[E10-601]: cannot infer T (no argument provides hint)
// let value = convert()

// Fixed with turbofish:
let value = convert::<i32, string@Managed>()
```

#### §9.6.12 Conformance Requirements [generic.resolution.requirements]

[61] Implementations shall:

1. Implement resolution algorithm inferring type, const, and grant arguments
2. Support explicit type arguments via turbofish syntax
3. Perform bound checking after resolution completes
4. Generate specialized code for each distinct instantiation via monomorphization
5. Cache and reuse instantiations with identical arguments
6. Resolve associated type projections to concrete types
7. Select behavior method implementations (override > inherited default)
8. Prefer specific implementations over blanket implementations
9. Guarantee zero runtime overhead: monomorphized code equivalent to hand-specialized
10. Support recursive generic types with indirection
11. Document and enforce monomorphization limits
12. Emit diagnostics E10-601 through E10-612 for resolution and monomorphization failures
13. Provide structured diagnostic payloads (Annex E §E.5) including inference state
14. Integrate with code generation phase (§2.2.4.4) for emission

---

**Previous**: §10.5 Behavior Implementations and Coherence (§10.5 [generic.implementation]) | **Next**: §10.7 Variance and Its Inference (§10.7 [generic.variance])
