# Cursive Language Specification

## Clause 9 — Generics and Behaviors

**Clause**: 9 — Generics and Behaviors
**File**: 10-5_Behavior-Implementations.md
**Section**: §9.5 Behavior Implementations and Coherence
**Stable label**: [generic.implementation]  
**Forward references**: §10.4 [generic.behavior], §10.6 [generic.resolution], Clause 4 [module], Clause 5 §5.5 [decl.type], Clause 7 [type], Clause 12 [contract]

---

### §9.5 Behavior Implementations and Coherence [generic.implementation]

#### §9.5.1 Overview

[1] Types attach behaviors to gain the behavior's procedures and associated types. Cursive provides two implementation mechanisms:

- **Inline implementation**: Using `with Behavior` in the type declaration
- **Standalone implementation**: Separate `behavior B for T` declarations

[2] Both mechanisms are subject to coherence and orphan rules that ensure each (Behavior, Type) pair has at most one implementation visible in any compilation context.

[3] This subclause specifies implementation syntax, compatibility checking, coherence rules, orphan rules, blanket implementations, and the interaction with monomorphization.

#### §9.5.2 Inline Implementation Syntax [generic.implementation.inline]

[4] Inline implementations use the behavior clause in type declarations (§5.5.2):

**Record declarations with behaviors** match the pattern:
```
[ <visibility_modifier> ] "record" <identifier> [ <generic_params> ]
    [ <contract_clause> ] [ <behavior_clause> ] <record_body>
```

**Behavior clauses** match the pattern:
```
"with" <behavior_reference> [ "," <behavior_reference> ... ]
```

where each **behavior reference** is a `<type_expression>` that must resolve to a behavior.

[5] Example:

```cursive
record Point with Display, Clone {
    x: f64,
    y: f64,

    procedure show(~): string@View
    {
        result "Point"
    }

    procedure clone(~): Self
        [[ alloc::heap |- true => true ]]
    {
        result Point { x: self.x, y: self.y }
    }
}
```

[6] The behavior clause `with Display, Clone` attaches both behaviors to `Point`. Procedures implementing behavior methods appear in the record body.

#### §9.5.3 Standalone Implementation Syntax [generic.implementation.standalone]

[7] Standalone implementations use the following syntax:

**Behavior implementations** match the pattern:
```
[ <visibility_modifier> ] "behavior" <behavior_name> [ <generic_params> ]
    "for" <for_type> [ <where_clause> ] <implementation_body>
```

**For-type clauses** take one of the following forms:
```
<type_expression>
"for" "<" <generic_param_list> ">" <type_expression>
```

**Implementation bodies** match the pattern:
```
"{" [ <implementation_item> ... ] "}"
```

where each **implementation item** takes one of the following forms:
```
<associated_type_spec>
<procedure_declaration>
```

**Associated type specifications** match the pattern:
```
"type" <identifier> "=" <type_expression>
```

[ Note: See Annex A §A.7 [grammar.declaration] for complete behavior implementation grammar.
— end note ]

[8] Example:

```cursive
behavior Display for Point {
    procedure show(~): string@View
    {
        result "Point({}, {})".format(self.x, self.y)
    }
}
```

[9] Standalone implementations may appear at module scope and are subject to the same visibility and coherence rules as inline implementations.

#### §9.5.4 Implementation Requirements [generic.implementation.requirements]

##### §9.5.4.1 Completeness

[10] Behavior implementations must provide:

1. All associated type specifications required by the behavior
2. Procedure implementations for all behavior procedures (or use inherited defaults)

[11] **Completeness checking**:

[ Given: Behavior `P` declaring items `{I₁, ..., Iₙ}`, implementation for type `T` ]

$$
\frac{\forall i. \; \text{item } I_i \text{ is specified in implementation or has default}}{\text{behavior } P \text{ for } T \text{ complete}}
\tag{WF-Impl-Complete}
$$

[12] Incomplete implementations produce diagnostic E10-501 listing missing items.

##### §9.5.4.2 Procedure Signature Compatibility

[13] Procedure implementations must match behavior signatures:

- Parameter types must be equivalent (after substituting `Self`)
- Return type must be equivalent
- Receiver permission must match exactly
- Grants must be a subset of behavior grants (or equal)
- Preconditions may be weaker (accept more inputs)
- Postconditions may be stronger (guarantee more)

[14] **Signature compatibility**:

$$
\frac{
\begin{array}{c}
\text{predicate procedure: } m(self: \pi_P\, Self, p_1: \tau_1, \ldots) : \tau_r \; ! \varepsilon_P \; [[ G_P |- M_P => W_P ]] \\
\text{implementation: } m(self: \pi_I\, T, p_1: \upsilon_1, \ldots) : \upsilon_r \; ! \varepsilon_I \; [[ G_I |- M_I => W_I ]] \\
\pi_P = \pi_I \quad \tau_i \equiv \upsilon_i \quad \tau_r \equiv \upsilon_r \quad \varepsilon_I \subseteq \varepsilon_P \\
M_P \Rightarrow M_I \quad W_I \Rightarrow W_P
\end{array}
}{\text{procedure } m \text{ compatible}}
\tag{WF-Impl-Sig-Compat}
$$

[15] Incompatible signatures produce diagnostic E10-502 identifying the mismatched component.

##### §9.5.4.3 Associated Type Specification

[16] Associated types declared in behaviors must be specified in implementations:

```cursive
behavior Iterator {
    type Item = ()

    procedure next(~%): Item
    { result () }
}

behavior Iterator for Range {
    type Item = i32    // Required specification

    procedure next(~%): i32
    {
        // Implementation
    }
}
```

[17] Omitting required associated types produces diagnostic E10-503.

#### §9.5.5 Blanket Implementations [generic.implementation.blanket]

##### §9.5.5.1 Overview

[18] Blanket implementations provide behavior implementations for all types satisfying specified bounds. They enable deriving functionality for entire type families.

##### §9.5.5.2 Syntax

[19] Blanket implementation syntax uses the generic for-clause:

```cursive
behavior ToString for<T> T where T: Display {
    procedure to_string(~): string@Managed
        [[ alloc::heap |- true => true ]]
    {
        result self.show().to_managed()
    }
}
```

[20] The syntax `for<T> T where T: Display` declares a generic implementation: for all types `T` satisfying `Display`, implement `ToString`.

##### §9.5.5.3 Blanket Constraints

[21] Blanket implementations must:

1. Include a where clause constraining the type variable
2. Satisfy the orphan rule (§10.5.6): behavior must be local
3. Not overlap with other implementations (coherence, §10.5.7)

[22] Blanket implementations without where clauses are forbidden (diagnostic E10-504) as they would match all types, violating coherence.

##### §9.5.5.4 Resolution with Blanket Implementations

[23] When resolving behavior implementations (§10.6), the compiler checks:

1. Specific implementations (concrete type `T`)
2. Blanket implementations matching `T`'s bounds

[24] Specific implementations take precedence over blanket implementations. If multiple blanket implementations match, the most specific one is chosen (based on bound specificity).

[ Note: This edition does not support specialization (§10.5.8). If multiple blanket implementations could match without a clear specificity ordering, the program is ill-formed.
— end note ]

##### §9.5.5.5 Examples

**Example 10.5.5.1 (Blanket implementation):**

```cursive
behavior Display {
    procedure show(~): string@View
    {
        result "Display"
    }
}

behavior Debug {
    procedure debug(~): string@View
    {
        result "Debug"
    }
}

// Blanket: all Display types get Debug
behavior Debug for<T> T where T: Display {
    procedure debug(~): string@View
    {
        result "Debug: " ++ self.show()
    }
}

record Point with Display {
    x: f64,
    y: f64,

    procedure show(~): string@View
    {
        result "Point"
    }
}

// Point automatically gets Debug through blanket implementation
let p = Point { x: 1.0, y: 2.0 }
println("{}", p.debug())  // "Debug: Point"
```

#### §9.5.6 Orphan Rule [generic.implementation.orphan]

##### §9.5.6.1 Orphan Rule Statement

[25] A behavior implementation `behavior P for T` is permitted if and only if at least one of the following holds:

1. Behavior `P` is defined in the current module, OR
2. The outermost type constructor of `T` is defined in the current module

[26] **Orphan rule judgment**:

[ Given: Behavior `P`, type `T`, implementing module `M` ]

$$
\frac{P \in \text{Module}(M) \lor \text{outer}(T) \in \text{Module}(M)}{\text{behavior } P \text{ for } T \text{ in } M \text{ permitted}}
\tag{WF-Orphan-Allowed}
$$

[27] Implementations violating the orphan rule are ill-formed (diagnostic E10-505).

##### §9.5.6.2 Outermost Type Constructor

[28] For generic types, the _outermost type constructor_ is the top-level type name:

- `List<T>` → outermost is `List`
- `Pair<i32, string>` → outermost is `Pair`
- `Ptr<Buffer>@Valid` → outermost is `Ptr` (built-in)
- `(i32, i32)` → tuple (built-in structural type)

[29] For nested generics `Container<Inner<T>>`, the outermost constructor is `Container`.

##### §9.5.6.3 Rationale

[ Rationale: The orphan rule prevents conflicting implementations from different modules. Without it, two modules could each provide an implementation of the same behavior for the same external type, causing ambiguity. By requiring local definition of either the behavior or the type, coherence is maintained across separate compilation units.
— end rationale ]

##### §9.5.6.4 Orphan Rule for Blanket Implementations

[30] Blanket implementations must satisfy the orphan rule:

```cursive
// Module A defines Display
behavior Display { ... }

// Module A can provide blanket implementation
behavior ToString for<T> T where T: Display {
    // OK: Display is local to module A
}

// Module B CANNOT provide blanket over Display
// behavior Logging for<T> T where T: Display {
//     // error[E10-505]: orphan violation (Display not local)
// }
```

[31] The behavior referenced in the blanket must be local to satisfy the orphan rule.

##### §9.5.6.5 Examples

**Example 10.5.6.1 (Valid: local behavior):**

```cursive
// Module: my_app::display

public behavior PrettyPrint {
    procedure pretty(~): string@Managed
        [[ alloc::heap |- true => true ]]
    {
        result "pretty"
    }
}

// Valid: PrettyPrint is local
behavior PrettyPrint for std::collections::List {
    procedure pretty(~): string@Managed
    {
        result "List pretty"
    }
}
```

**Example 10.5.6.2 (Valid: local type):**

```cursive
// Module: my_app::types

public record MyType {
    value: i32,
}

// Valid: MyType is local
behavior std::display::Display for MyType {
    procedure show(~): string@View
    {
        result "MyType"
    }
}
```

**Example 10.5.6.3 - invalid (Orphan violation):**

```cursive
// Module: my_app::bad

// error[E10-505]: orphan rule violation
// Neither std::display::Display nor std::collections::List is local
behavior std::display::Display for std::collections::List {
    // Cannot implement external behavior for external type
}
```

#### §9.5.7 Coherence Rule [generic.implementation.coherence]

##### §9.5.7.1 Coherence Statement

[32] For any (Behavior, Type) pair, at most one implementation shall be visible within a compilation context. Multiple implementations produce diagnostic E10-506.

[33] **Coherence judgment**:

[ Given: Behavior `P`, type `T`, compilation unit set `U` ]

$$
\frac{|\{\text{impls of } P \text{ for } T \text{ in } U\}| > 1}{\text{ERROR E10-506: duplicate behavior implementation}}
\tag{WF-Coherence-Unique}
$$

##### §9.5.7.2 Coherence Checking

[34] Coherence checking occurs during module compilation. The compiler verifies that:

1. No duplicate inline implementations (multiple `with P` clauses for same behavior)
2. No conflicts between inline and standalone implementations for the same behavior
3. No overlapping blanket implementations
4. Cross-module implementations respect orphan rules

[35] Coherence violations are fatal errors; the program is ill-formed until resolved.

##### §9.5.7.3 Blanket Implementation Overlap

[36] Two blanket implementations overlap when both could match the same type:

```cursive
// Overlapping blanket implementations (ill-formed)

behavior Printer for<T> T where T: Display {
    // Implementation 1
}

behavior Printer for<T> T where T: Debug {
    // Implementation 2
}

// error[E10-507]: overlapping blanket implementations
// If a type satisfies both Display and Debug, which behavior applies?
```

[37] Implementations shall not overlap. The compiler detects overlap by checking whether bound constraints could be simultaneously satisfied.

[38] **Overlap detection**:

$$
\frac{\exists \tau. \; \tau : B_1 \land \tau : B_2}{\text{blanket impls with bounds } B_1 \text{ and } B_2 \text{ overlap}}
\tag{WF-Overlap-Detect}
$$

[39] Overlapping blanket implementations produce diagnostic E10-507 identifying both implementations and suggesting bound refinement.

##### §9.5.7.4 Coherence Examples

**Example 10.5.7.1 (Valid: non-overlapping blankets):**

```cursive
behavior ShowInt {
    procedure show_as_int(~): i32
    {
        result 0
    }
}

// Non-overlapping: disjoint bounds
behavior ShowInt for<T> T where T: NumericType {
    // Only numeric types
}

behavior ShowInt for<T> T where T: Container {
    // Only container types
}

// OK if NumericType and Container are mutually exclusive
```

**Example 10.5.7.2 - invalid (Duplicate implementation):**

```cursive
record Data with Display {
    procedure show(~): string@View
    { result "Data1" }
}

behavior Display for Data {
    procedure show(~): string@View
    { result "Data2" }
}

// error[E10-506]: Data has two implementations of Display
```

#### §9.5.8 No Specialization [generic.implementation.specialization]

[40] Cursive does not support implementation specialization. Each (Behavior, Type) pair may have exactly one implementation; more specific implementations cannot override more general ones.

[41] The following pattern is **not** supported:

```cursive
// NOT SUPPORTED: Specialization

behavior Formatter for<T> T {
    // General implementation
}

behavior Formatter for i32 {
    // Specialized for i32 - NOT ALLOWED
}
```

[42] Attempting to provide specialized implementations alongside blanket implementations produces coherence error E10-506 or overlap error E10-507.

[ Rationale: Specialization adds substantial complexity to coherence checking and type resolution. The current specification prioritizes predictability and deterministic dispatch. Future editions may introduce specialization if field experience demonstrates clear necessity, but the design must maintain the deterministic resolution guarantee.
— end rationale ]

#### §9.5.9 Method Override and Associated Type Override [generic.implementation.override]

##### §9.5.9.1 Procedure Override

[43] When attaching a behavior with `with Behavior`, types may override behavior procedures by providing custom implementations in the type body:

```cursive
behavior Display {
    procedure show(~): string@View
    {
        result type_name::<Self>()  // Default
    }
}

record Custom with Display {
    value: i32,

    procedure show(~): string@View
    {
        result "Custom: " ++ self.value.to_string()  // Override
    }
}
```

[44] Overridden procedures must be signature-compatible per §10.5.4.2.

##### §9.5.9.2 Using Inherited Defaults

[45] Types may omit procedure implementations to inherit behavior defaults:

```cursive
record Simple with Display {
    // No show() implementation provided
    // Inherits default from Display behavior
}

let s = Simple {}
println("{}", s.show())  // Uses inherited default
```

##### §9.5.9.3 Associated Type Override

[46] Types override associated type defaults by specifying the associated type in the implementation:

```cursive
behavior Producer {
    type Output = ()

    procedure produce(~): Output
    { result () }
}

record IntProducer with Producer {
    type Output = i32    // Override default

    value: i32,

    procedure produce(~): i32
    {
        result self.value
    }
}
```

#### §9.5.10 Generic Implementations [generic.implementation.generic]

##### §9.5.10.1 Generic Behavior for Generic Type

[47] When both behavior and type are generic, implementation parameters must be compatible:

```cursive
behavior Converter<U> {
    procedure convert(~): U
    {
        panic("Not implemented")
    }
}

record Wrapper<T> {
    value: T,
}

behavior Converter<string@Managed> for<T> Wrapper<T> where T: Display {
    procedure convert(~): string@Managed
        [[ alloc::heap |- true => true ]]
    {
        result self.value.show().to_managed()
    }
}
```

[48] The implementation `for<T> Wrapper<T>` is generic over `T`. Instantiation of `Wrapper<ConcreteType>` requires `ConcreteType: Display` for the behavior implementation to apply.

##### §9.5.10.2 Parameter Substitution

[49] During implementation checking, generic parameters are substituted:

- Type parameters from the behavior are instantiated with implementation arguments
- Type parameters from the implementing type are universally quantified
- Bounds are checked after substitution

$$
\frac{
\begin{array}{c}
\text{behavior } P\langle\alpha\rangle \text{ for } T\langle\beta\rangle \quad \text{where } \beta : B \\
\text{behavior } P\langle\tau\rangle \text{ declared} \quad T\langle\beta\rangle \text{ declared}
\end{array}
}{\text{implementation valid if bounds and signatures match after substitution}}
\tag{WF-Impl-Generic}
$$

#### §9.5.11 Diagnostics [generic.implementation.diagnostics]

[50] Behavior implementation diagnostics:

| Code    | Condition                                               |
| ------- | ------------------------------------------------------- |
| E10-501 | Incomplete implementation (missing procedures or types) |
| E10-502 | Procedure signature incompatible with behavior          |
| E10-503 | Associated type specification missing                   |
| E10-504 | Blanket implementation lacks where clause               |
| E10-505 | Orphan rule violation                                   |
| E10-506 | Duplicate implementation (coherence violation)          |
| E10-507 | Overlapping blanket implementations                     |
| E10-508 | Implementation for non-predicate (e.g., contract)       |
| E10-509 | Implementation type does not exist or is not visible    |
| E10-510 | Generic implementation parameter mismatch               |

#### §9.5.12 Examples [generic.implementation.examples]

**Example 10.5.12.1 (Complete standalone implementation):**

```cursive
behavior Serializable {
    type Format = string@View

    procedure serialize(~): Self::Format
        [[ alloc::heap |- true => true ]]
    {
        result "default serialization"
    }
}

record User {
    id: u64,
    name: string@Managed,
}

behavior Serializable for User {
    type Format = string@Managed    // Override default

    procedure serialize(~): string@Managed
        [[ alloc::heap |- true => true ]]
    {
        result "User(id=" ++ self.id.to_string() ++ ", name=" ++ self.name ++ ")"
    }
}
```

**Example 10.5.12.2 (Inline implementation with override):**

```cursive
behavior Formatter {
    procedure format(~): string@Managed
        [[ alloc::heap |- true => true ]]
    {
        result type_name::<Self>()
    }

    procedure format_verbose(~): string@Managed
        [[ alloc::heap |- true => true ]]
    {
        result "Verbose: " ++ self.format()
    }
}

record Status with Formatter {
    code: i32,

    procedure format(~): string@Managed
        [[ alloc::heap |- true => true ]]
    {
        result "Status(" ++ self.code.to_string() ++ ")"
    }

    // format_verbose() inherited from Formatter behavior
}
```

**Example 10.5.12.3 (Blanket with nested generics):**

```cursive
behavior Display {
    procedure show(~): string@View
    { result "Display" }
}

behavior Display for<T> (T, T) where T: Display {
    procedure show(~): string@View
    {
        result "(" ++ self.0.show() ++ ", " ++ self.1.show() ++ ")"
    }
}

// Applies to any tuple (T, T) where T: Display
let pair = (Point {x: 1.0, y: 2.0}, Point {x: 3.0, y: 4.0})
println("{}", pair.show())  // Uses blanket implementation
```

**Example 10.5.12.4 - invalid (Orphan violation):**

```cursive
// Module: third_party

// error[E10-505]: orphan rule violation
// Cannot implement external behavior for external type
behavior std::fmt::Display for std::collections::List {
    procedure show(~): string@View
    {
        result "List"
    }
}
```

**Example 10.5.12.5 - invalid (Overlapping blankets):**

```cursive
behavior Logger {
    procedure log(~, message: string@View)
        [[ io::write |- true => true ]]
    {
        println("{}", message)
    }
}

// Implementation 1
behavior Logger for<T> T where T: Display {
    procedure log(~, message: string@View)
        [[ io::write |- true => true ]]
    {
        println("{}: {}", self.show(), message)
    }
}

// Implementation 2 - OVERLAPS with Implementation 1
behavior Logger for<T> T where T: Debug {
    procedure log(~, message: string@View)
        [[ io::write |- true => true ]]
    {
        println("{}: {}", self.debug(), message)
    }
}

// error[E10-507]: overlapping blanket implementations
// Type satisfying both Display and Debug is ambiguous
```

#### §9.5.13 Conformance Requirements [generic.implementation.requirements]

[51] Implementations shall:

1. Support inline behavior attachment via `with Behavior` clause
2. Support standalone behavior implementations via `behavior P for T`
3. Support blanket implementations via `behavior P for<T> T where ...`
4. Enforce that all behavior procedures and associated types are provided
5. Check procedure signature compatibility (parameters, return, receiver, grants, sequent)
6. Enforce orphan rule: require local behavior or local type constructor (E10-505)
7. Enforce coherence: reject duplicate implementations (E10-506)
8. Detect overlapping blanket implementations (E10-507)
9. Support procedure override in inline implementations
10. Support associated type override in implementations
11. Allow inherited defaults when override not provided
12. Integrate with monomorphization (§10.6) for code generation
13. Emit structured diagnostics for violations (Annex E §E.5)

---

**Previous**: §10.4 Behaviors: Declarations and Items (§10.4 [generic.behavior]) | **Next**: §10.6 Resolution and Monomorphization (§10.6 [generic.resolution])
