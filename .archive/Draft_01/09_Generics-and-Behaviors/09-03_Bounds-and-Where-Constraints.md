# Cursive Language Specification

## Clause 9 — Generics and Behaviors

**Clause**: 9 — Generics and Behaviors
**File**: 10-3_Bounds-and-Where-Constraints.md
**Section**: §9.3 Bounds and Where-Constraints
**Stable label**: [generic.bounds]  
**Forward references**: §10.2 [generic.parameter], §10.4 [generic.behavior], §10.5 [generic.implementation], §10.6 [generic.resolution], Clause 7 [type], Clause 12 [contract]

---

### §9.3 Bounds and Where-Constraints [generic.bounds]

#### §9.3.1 Overview

[1] Bounds constrain generic type parameters to types satisfying specific behaviors or contracts. They enable generic algorithms to assume certain operations are available without knowing the concrete type.

[2] Cursive provides two syntactic forms for expressing bounds:

- **Inline bounds** (`<T: Behavior>`) — bounds attached directly to parameter declarations
- **Where clauses** (`where T: Behavior`) — bounds listed after the procedure signature or type declaration header

[3] This subclause specifies bound syntax, satisfaction rules, associated type constraints, grant bounds, and the interaction between inline and where-clause forms.

#### §9.3.2 Syntax

[4] Bound syntax follows:

**Type parameters with bounds** match the pattern:
```
<identifier> ":" <behavior_bounds>
```

**Behavior bounds** match the pattern:
```
<behavior_bound> [ "+" <behavior_bound> ... ]
```

where each **behavior bound** is:
```
<type_expression>    // Must resolve to behavior or contract name
```

**Where clauses** match the pattern:
```
"where" <where_predicate_list>
```

where **where predicate lists** match:
```
<where_predicate> [ "," <where_predicate> ... ]
```

**Where predicates** take one of the following forms:
```
<type_param> ":" <behavior_bounds>
<associated_type_equality>
<grant_bound>
```

**Associated type equalities** match the pattern:
```
<type_param> "::" <identifier> "=" <type>
```

**Grant bounds** match the pattern:
```
<grant_param> "⊆" "{" <grant_set> "}"
```

[ Note: See Annex A §A.6 [grammar.declaration] for the normative bounds grammar.
— end note ]

[5] The `+` operator combines multiple behavior or contract bounds. Bounds may be specified inline or in `where` clauses; both forms are semantically equivalent.

#### §9.3.3 Inline Bounds [generic.bounds.inline]

##### §9.3.3.1 Syntax and Formation

[6] Inline bounds attach directly to type parameter declarations:

```cursive
procedure display_all<T: Display>(items: [T])
    [[ io::write |- items.len() > 0 => true ]]
{
    loop item: T in items {
        println("{}", item.show())
    }
}
```

[7] Multiple inline bounds use `+` syntax:

```cursive
procedure serialize<T: Display + Clone>(value: T): string@Managed
    [[ alloc::heap |- true => true ]]
{
    result value.clone().show().to_managed()
}
```

##### §9.3.3.2 Constraints

[8] Each bound in an inline bound list shall resolve to a behavior or contract visible in the current scope. Invalid bounds produce diagnostic E10-201.

[9] Duplicate bounds in the same parameter are permitted but redundant. Implementations may warn about duplicates as a quality-of-implementation feature.

[10] Inline bounds apply only to the parameter they are attached to. Cross-parameter constraints require `where` clauses.

#### §9.3.4 Where Clauses [generic.bounds.where]

##### §9.3.4.1 Syntax and Placement

[11] Where clauses appear after the parameter list in procedure signatures or after the generic parameter list in type declarations:

```cursive
procedure complex<T, U>(a: T, b: U): Result
    where T: Display + Clone,
          U: Serializable,
          T::Output = U
{
    // Implementation
}

record Processor<T, U>
    where T: Parseable,
          U: Writable
{
    parser: T,
    writer: U,
}
```

##### §9.3.4.2 Where Clause Constraints

[12] Where clauses support three forms of constraints:

1. **Behavior bounds** (`T: Behavior`) — type parameter must satisfy behavior
2. **Associated type equality** (`T::Item = U`) — associated type must equal specified type
3. **Grant bounds** (`ε ⊆ {...}`) — grant parameter must be subset of specified grants

[13] All three forms may appear in the same where clause. Constraints are evaluated in the order written; later constraints may reference earlier ones.

##### §9.3.4.3 Equivalence with Inline Bounds

[14] Inline bounds and where clause bounds are semantically equivalent:

```cursive
// These two declarations are equivalent:

procedure f1<T: Display + Clone>(value: T)
{
    // ...
}

procedure f2<T>(value: T)
    where T: Display + Clone
{
    // ...
}
```

[15] Mixing inline and where clauses is permitted; bounds accumulate:

```cursive
procedure combined<T: Display>(value: T)
    where T: Clone
{
    // T must satisfy both Display and Clone
}
```

#### §9.3.5 Behavior and Contract Bounds [generic.bounds.predicate]

##### §9.3.5.1 Bound Satisfaction

[16] A type `τ` satisfies bound `B` (written `τ : B`) when:

- If `B` is a behavior: a behavior implementation `behavior B for τ` exists (§10.5)
- If `B` is a contract: a contract implementation `τ: B` exists in the type declaration (Clause 12)

[17] **Satisfaction judgment**:

[ Given: Type `τ`, bound `B` (behavior or contract) ]

$$
\frac{\text{behavior } B \text{ for } \tau \text{ implemented}}{\Gamma \vdash \tau : B}
\tag{WF-Bound-Satisfied}
$$

[18] For contract bounds (Clause 12):

$$
\frac{\text{record } \tau \text{: } B \text{ declared} \quad \text{all } B \text{ obligations met}}{\Gamma \vdash \tau : B}
\tag{WF-Contract-Bound}
$$

##### §9.3.5.2 Multiple Bounds

[19] A type parameter with multiple bounds must satisfy all bounds simultaneously:

$$
\frac{\Gamma \vdash \tau : B_1 \quad \Gamma \vdash \tau : B_2 \quad \cdots \quad \Gamma \vdash \tau : B_n}{\Gamma \vdash \tau : (B_1 + B_2 + \cdots + B_n)}
\tag{WF-Multi-Bound}
$$

[20] Bound checking occurs at instantiation time (§10.6). If any bound is not satisfied, the instantiation is ill-formed and produces diagnostic E10-202.

##### §9.3.5.3 Examples

**Example 10.3.5.1 (Single behavior bound):**

```cursive
behavior Ord {
    procedure compare(~, other: Self): i32
    {
        // Default implementation
        result 0
    }
}

procedure min<T: Ord>(a: T, b: T): T
{
    if a.compare(b) <= 0 {
        result a
    } else {
        result b
    }
}
```

**Example 10.3.5.2 (Multiple bounds):**

```cursive
procedure log_and_clone<T: Display + Clone>(value: T)
    [[ io::write, alloc::heap |- true => true ]]
{
    println("Value: {}", value.show())
    let copy = value.clone()
    process(copy)
}
```

#### §9.3.6 Associated Type Constraints [generic.bounds.assoc]

##### §9.3.6.1 Associated Type Equality

[21] Where clauses may constrain associated types to specific types:

```cursive
procedure sum_items<I>(iter: I): i32
    [[ alloc::heap |- true => true ]]
    where I: Iterator,
          I::Item = i32
{
    var total: i32 = 0
    loop item: i32 in iter {
        total = total + item
    }
    result total
}
```

[22] The constraint `I::Item = i32` requires that the associated type `Item` of behavior or contract `Iterator` equals `i32` for type `I`.

##### §9.3.6.2 Associated Type Projection

[23] Within a generic context, associated types are referenced using projection syntax `T::AssocType`:

$$
\frac{\Gamma \vdash T : \text{Behavior} \quad \text{Behavior declares type } I}{\Gamma \vdash T::I : \text{Type}}
\tag{T-Assoc-Proj}
$$

[24] For type variables with behavior bounds, the projection remains abstract until instantiation. For concrete types, the projection resolves to the specified associated type.

##### §9.3.6.3 Associated Type Bounds

[25] Associated types may themselves have bounds:

```cursive
procedure process<C>(container: C)
    where C: Container,
          C::Element: Display + Clone
{
    let first = container.get(0)
    println("{}", first.show())
    let copy = first.clone()
}
```

[26] The bound `C::Element: Display` applies to the associated type `Element` of `C`. Both the container bound and the element bound must be satisfied at instantiation.

##### §9.3.6.4 Examples

**Example 10.3.6.1 (Associated type equality constraint):**

```cursive
behavior Iterator {
    type Item = ()

    procedure next(~%): Item
    { result () }
}

procedure collect_bytes<I>(iter: I): [u8]
    [[ alloc::heap |- true => true ]]
    where I: Iterator,
          I::Item = u8
{
    let result = array::new()
    loop item: u8 in iter {
        result.push(item)
    }
    result result
}

// Valid: ByteIterator::Item = u8
collect_bytes(byte_stream)

// Invalid: Range::Item = i32 ≠ u8
// collect_bytes(range)  // error[E10-203]: associated type mismatch
```

#### §9.3.7 Grant Bounds [generic.bounds.grant]

##### §9.3.7.1 Syntax and Semantics

[27] Grant parameters may be bounded using the subset operator `⊆`:

```cursive
procedure controlled<T, ε>(f: () -> T ! ε): T
    [[ ε |- true => true ]]
    where ε ⊆ {io::write, alloc::heap}
{
    result f()
}
```

[28] The bound `ε ⊆ {io::write, alloc::heap}` requires that grant set `ε` be a subset of the specified grants. At instantiation, the actual grant set must satisfy:

$$
\varepsilon_{\text{actual}} \subseteq \{\text{io::write}, \text{alloc::heap}\}
$$

[29] Violating grant bounds produces diagnostic E10-301 listing the actual grant set and the bound.

##### §9.3.7.2 Grant Bound Checking

[30] Grant bounds are checked during instantiation:

[ Given: Grant parameter `ε`, bound `ε ⊆ G`, instantiation with grant set `ε'` ]

$$
\frac{\varepsilon' \subseteq G}{\text{grant bound satisfied}}
\tag{WF-Grant-Bound}
$$

[31] Grant bounds compose with behavior bounds: a procedure may constrain both types and grants:

```cursive
procedure transform<T, U, ε>(
    values: [T],
    f: (T) -> U ! ε
): [U]
    [[ alloc::heap, ε |- values.len() > 0 => result.len() == values.len() ]]
    where T: Clone,
          U: Display,
          ε ⊆ {io::write}
{
    // Can clone T, display U, and f can use only io::write
}
```

##### §9.3.7.3 Examples

**Example 10.3.7.1 (Grant bounds limiting capabilities):**

```cursive
procedure with_logging<ε>(action: () -> () ! ε)
    [[ io::write, ε |- true => true ]]
    where ε ⊆ {alloc::heap, fs::read}
{
    println("Starting action")
    action()
    println("Completed action")
}

// Valid: alloc::heap ⊆ {alloc::heap, fs::read}
with_logging(|| [[ alloc::heap ]] { allocate_buffer() })

// Invalid: net::send ⊄ {alloc::heap, fs::read}
// with_logging(|| [[ net::send ]] { send_packet() })
// error[E10-301]: grant set exceeds bound
```

#### §9.3.8 Bound Entailment and Subsumption

##### §9.3.8.1 Entailment Rules

[32] Bounds participate in logical entailment: if a type satisfies a stronger bound, it automatically satisfies weaker bounds derived from it.

[33] **Transitive closure**: If behavior `P` extends behavior `Q` (§10.4), then `T: P` entails `T: Q`:

$$
\frac{\text{predicate } P \text{ with } Q \{ \ldots \} \quad \Gamma \vdash T : P}{\Gamma \vdash T : Q}
\tag{Bound-Entail}
$$

[34] This allows procedures to accept types with more specific bounds when only a weaker bound is required.

##### §9.3.8.2 Grant Subsumption

[35] Grant bounds follow subset subsumption:

$$
\frac{\varepsilon_1 \subseteq \varepsilon_2 \subseteq G \quad \text{bound is } \varepsilon \subseteq G}{\varepsilon_1 \text{ satisfies bound}}
\tag{Grant-Subsume}
$$

[36] A grant set that is a subset of another grant set satisfies any bound that the larger set satisfies.

##### §9.3.8.3 Examples

**Example 10.3.8.1 (Bound entailment):**

```cursive
behavior Display {
    procedure show(~): string@View
    { result "Display" }
}

behavior Debug with Display {
    procedure debug(~): string@View
    { result self.show() }

    procedure show(~): string@View
    { result "Debug default" }
}

procedure print<T: Display>(value: T)
    [[ io::write |- true => true ]]
{
    println("{}", value.show())
}

record Point with Debug {
    x: f64,
    y: f64,

    procedure show(~): string@View
    { result "Point" }

    procedure debug(~): string@View
    { result "Point(x, y)" }
}

// Point: Debug, and Debug with Display
// Therefore Point: Display (via entailment)
print(Point { x: 1.0, y: 2.0 })  // OK: Debug entails Display
```

#### §9.3.9 Constraints

[37] _Bound visibility._ Behaviors and contracts referenced in bounds must be visible in the scope where the generic entity is declared. Non-visible bounds produce diagnostic E10-201.

[38] _Bound satisfaction at instantiation._ When a generic is instantiated with type argument `τ`, every bound on the corresponding type parameter must be satisfied by `τ`. Unsatisfied bounds produce diagnostic E10-202 listing the type, the bound, and available implementations.

[39] _Associated type validity._ Associated type equality constraints shall reference associated types declared in the behaviors or contracts listed in the type parameter's bounds. Referencing undeclared associated types produces diagnostic E10-203.

[40] _Grant bound validity._ Grant bounds shall use the subset operator `⊆`, not the type bound operator `:`. Grant sets in bounds must contain only valid grant identifiers. Invalid grant identifiers produce diagnostic E10-204.

[41] _Cyclic bounds._ Bounds shall not form cycles through associated type projections. For example, `where T: C, T::Item: D, D::Value = T` may create a cycle. Cyclic bound dependencies produce diagnostic E10-205.

[42] _Conflicting equality constraints._ A where clause shall not contain conflicting associated type equalities for the same projection. For example, `where T::Item = i32, T::Item = u32` is ill-formed (diagnostic E10-206).

#### §9.3.10 Semantics

##### §9.3.10.1 Bound Checking

[43] Bound checking occurs during the resolution phase (§10.6) after type arguments are determined. The compiler verifies that each type argument satisfies all bounds on its corresponding parameter.

[44] **Checking algorithm**:

```
check_bounds(parameters, arguments, bounds):
    for each (param, arg) in zip(parameters, arguments):
        for each bound B in bounds(param):
            if bound is behavior/contract:
                verify: behavior B for arg exists
                if not: error E10-202
            if bound is associated type equality T::A = U:
                verify: arg::A ≡ U
                if not: error E10-203

    for each grant bound (ε ⊆ G):
        verify: actual_grant_set ⊆ G
        if not: error E10-301

    return bounds_satisfied
```

##### §9.3.10.2 Bound Propagation

[45] Bounds propagate through the generic entity's body. Within a generic procedure or type body, operations on type variables are limited to those guaranteed by their bounds:

```cursive
procedure use_bounded<T: Display>(value: T)
    [[ io::write |- true => true ]]
{
    println("{}", value.show())    // OK: Display guarantees show()
    // value.clone()               // error: Clone not in bounds
}
```

[46] The type checker enforces that only methods guaranteed by the bounds are called on type variables.

##### §9.3.10.3 Associated Type Resolution

[47] Associated type projections are resolved based on the concrete type at instantiation:

$$
\frac{\Gamma \vdash \tau : P \quad \text{behavior } P \text{ for } \tau \{ \text{type } I = \upsilon \}}{\Gamma \vdash \tau::I \equiv \upsilon}
\tag{Assoc-Resolve}
$$

[48] For generic type variables, projections remain abstract until monomorphization substitutes concrete types.

#### §9.3.11 Higher-Kinded Bounds

[49] Cursive does not support higher-kinded type parameters (type parameters that are themselves generic). Bounds may not abstract over type constructors:

```cursive
// NOT SUPPORTED:
// procedure map_container<F<_>, T, U>(c: F<T>) -> F<U>
```

[50] All type parameters are first-order: they range over concrete types, not type constructors. Higher-kinded patterns may be encoded using associated types when needed.

#### §9.3.12 Diagnostics

[51] Bound-related diagnostics:

| Code    | Condition                                                       |
| ------- | --------------------------------------------------------------- |
| E10-201 | Bound references undefined or non-visible behavior/contract     |
| E10-202 | Type argument does not satisfy bound                            |
| E10-203 | Associated type equality constraint: types do not match         |
| E10-204 | Grant bound references invalid grant identifier                 |
| E10-205 | Cyclic bound dependencies through associated type projections   |
| E10-206 | Conflicting associated type equality constraints                |
| E10-301 | Grant parameter exceeds bound (grant set not subset of allowed) |

#### §9.3.13 Examples

**Example 10.3.13.1 (Where clause with multiple constraint kinds):**

```cursive
procedure convert_and_log<T, U, ε>(
    value: T,
    converter: (T) -> U ! ε
): U
    [[ io::write, ε |- true => true ]]
    where T: Display + Clone,
          U: Serializable,
          ε ⊆ {alloc::heap, io::write}
{
    println("Converting: {}", value.show())
    let result = converter(value.clone())
    log_output(result.serialize())
    result result
}
```

**Example 10.3.13.2 (Associated type with bounds):**

```cursive
behavior Container {
    type Element = ()

    procedure get(~, index: usize): Element
    { result () }
}

procedure display_first<C>(container: C)
    [[ io::write |- true => true ]]
    where C: Container,
          C::Element: Display
{
    let first = container.get(0)
    println("First element: {}", first.show())
}
```

**Example 10.3.13.3 - invalid (Unsatisfied bound):**

```cursive
record Point {
    x: f64,
    y: f64,
}

procedure show_item<T: Display>(item: T)
    [[ io::write |- true => true ]]
{
    println("{}", item.show())
}

// error[E10-202]: Point does not satisfy bound Display
// show_item(Point { x: 1.0, y: 2.0 })
```

#### §9.3.14 Conformance Requirements

[52] Implementations shall:

1. Support inline bounds (`T: Behavior`) and where clauses
2. Accept multiple bounds with `+` syntax and comma-separated where behaviors
3. Check bound satisfaction at instantiation time (§10.6)
4. Support associated type equality constraints in where clauses
5. Support grant parameter bounds using `⊆` operator
6. Validate bound references resolve to visible behaviors or contracts (E10-201)
7. Diagnose unsatisfied bounds with structured payloads (E10-202) listing type, bound, and available implementations
8. Resolve associated type projections to concrete types at monomorphization
9. Detect and reject cyclic bound dependencies (E10-205)
10. Detect and reject conflicting associated type constraints (E10-206)
11. Integrate bounds with the resolution algorithm (§10.6)
12. Propagate bounds through generic entity bodies for type checking

---

**Previous**: §10.2 Type Parameters (§10.2 [generic.parameter]) | **Next**: §10.4 Behaviors: Declarations and Items (§10.4 [generic.behavior])
