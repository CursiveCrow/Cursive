# Cursive Language Specification

## Clause 9 — Generics and Behaviors

**Clause**: 9 — Generics and Behaviors
**File**: 10-2_Type-Parameters.md
**Section**: §9.2 Type Parameters
**Stable label**: [generic.parameter]  
**Forward references**: §10.3 [generic.bounds], §10.4 [generic.behavior], §10.6 [generic.resolution], §10.7 [generic.variance], Clause 5 §5.5 [decl.type], Clause 7 [type], Clause 8 [expr], Clause 11 [memory], Clause 12 [contract]

---

### §9.2 Type Parameters [generic.parameter]

#### §9.2.1 Overview

[1] Type parameters enable parametric polymorphism: procedures and types may abstract over types, compile-time constants, and grant sets. This subclause specifies the syntax for declaring parameters, their scope and shadowing rules, default values, and explicit instantiation syntax.

[2] Cursive provides three kinds of generic parameters:

- **Type parameters** (`<T>`, `<T, U>`) — abstract over types
- **Const parameters** (`<const N: usize>`) — abstract over compile-time constant values
- **Grant parameters** (`<ε>`, `<ε ⊆ {...}>`) — abstract over grant sets for grant polymorphism

[3] All three kinds may appear in the same parameter list. Monomorphization (§10.6) generates specialized code for each distinct instantiation, ensuring zero runtime overhead.

#### §9.2.2 Syntax

[4] Generic parameter syntax follows:

**Generic parameters** match the pattern:
```
"<" <generic_param_list> ">"
```

where **generic parameter lists** match:
```
<generic_param> [ "," <generic_param> ... ]
```

**Generic parameters** take one of the following forms:
```
<type_param>
<const_param>
<grant_param>
```

**Type parameters** match the pattern:
```
<identifier> [ ":" <behavior_bounds> ]
```

**Behavior bounds** match the pattern:
```
<behavior_bound> [ "+" <behavior_bound> ... ]
```

where each **behavior bound** is:
```
<type_expression>    // Resolves to behavior or contract
```

**Const parameters** match the pattern:
```
"const" <identifier> ":" <const_type>
```

where **const types** are:
```
"usize" | "u8" | "u16" | "u32" | "u64" | "u128"
"isize" | "i8" | "i16" | "i32" | "i64" | "i128"
"bool"
```

**Grant parameters** take one of the following forms:
```
<identifier>                                  // Unbounded grant variable
<identifier> "⊆" "{" <grant_set> "}"         // Bounded grant variable
```

where **grant sets** match:
```
<grant_identifier> [ "," <grant_identifier> ... ]
```

and each **grant identifier** is:
```
<qualified_name>    // Module-qualified grant name
```

[ Note: See Annex A §A.6 [grammar.declaration] for the normative generic parameter grammar.
— end note ]

[5] Type parameters may have bounds specifying behaviors or contracts that constrain valid instantiations. Bounds may be declared inline (`<T: Display>`) or in `where` clauses (§10.3).

[6] Const parameters abstract over compile-time constant values of integer or boolean type. The parameter identifier may be used in type expressions, array lengths, and other compile-time contexts.

[7] Grant parameters abstract over grant sets. They enable grant-polymorphic procedures that work with callables requiring different grants.

#### §9.2.3 Type Parameters [generic.parameter.type]

##### §9.2.3.1 Declaration and Scope

[8] Type parameters are declared in generic parameter lists on:

- Type declarations: `record List<T> { ... }`
- Procedure declarations: `procedure map<T, U>(...) { ... }`
- Behavior declarations: `behavior Container<T> { ... }`
- Contract declarations (Clause 12): `contract Serializable<T> { ... }`

[9] Type parameters introduce bindings into the declaring entity's scope. Within that scope, the parameter identifier shadows any outer type binding with the same name (§6.3). Parameters are visible throughout the entity body and in `where` clauses.

[10] **Scope rule**:

[ Given: Generic declaration with type parameter `T` ]

$$
\frac{\text{declaration } D\langle T \rangle \{ \text{body} \}}{\text{In body: } T \in \Gamma \text{ as type variable}}
\tag{WF-TypeParam-Scope}
$$

##### §9.2.3.2 Bounds

[11] Type parameters may have bounds constraining which types may be used as arguments:

```cursive
procedure sort<T>(values: [T])
    [[ |- values.len() > 0 => true ]]
    where T: Ord
{
    // Implementation can use T::compare
}
```

[12] Bounds are checked at instantiation (§10.6). Multiple bounds are specified using `+` syntax or `where` clauses (§10.3).

##### §9.2.3.3 Type Parameter Inference

[13] Type parameters may be inferred from usage context:

```cursive
procedure identity<T>(value: T): T
{
    result value
}

let x = identity(42)          // T inferred as i32
let y: string = identity("hello")  // T inferred as string from return context
```

[14] When inference fails, diagnostic E10-101 requests explicit type arguments using turbofish syntax (§10.2.7).

##### §9.2.3.4 Examples

**Example 10.2.3.1 (Type parameters in record):**

```cursive
record Pair<T, U> {
    first: T,
    second: U,
}

let coordinates: Pair<f64, f64> = Pair { first: 3.0, second: 4.0 }
let mixed: Pair<i32, string@Managed> = Pair { first: 1, second: string.from("hello") }
```

**Example 10.2.3.2 (Type parameters in procedure):**

```cursive
procedure swap<T>(a: unique T, b: unique T): (T, T)
{
    result (b, a)
}

let (y, x) = swap(x, y)  // T inferred from arguments
```

#### §9.2.4 Const Parameters [generic.parameter.const]

##### §9.2.4.1 Declaration and Valid Types

[15] Const parameters abstract over compile-time constant values. They must have an integer or boolean type as specified in §10.2.2.

[16] Const parameters enable types and procedures parameterized by compile-time values:

```cursive
record FixedBuffer<T, const N: usize> {
    data: [T; N],
    len: usize,
}

procedure create_buffer<T, const SIZE: usize>(): FixedBuffer<T, SIZE>
    [[ alloc::heap |- SIZE > 0 => true ]]
{
    result FixedBuffer { data: [T::default(); SIZE], len: 0 }
}
```

##### §9.2.4.2 Const Parameter Values

[17] Const parameter arguments must be evaluable at compile time (§8.7):

- Integer or boolean literals
- Const bindings
- Arithmetic expressions over const values
- `sizeof(T)` or `alignof(T)` introspics
- Other const parameters

[18] Runtime values cannot be used as const arguments. Violations produce diagnostic E10-102.

##### §9.2.4.3 Const Expression Evaluation

[19] Const parameter values are evaluated during the compile-time execution phase (§2.2.4.2). The evaluation follows the same rules as `comptime` blocks but is restricted to the const parameter's type.

[20] **Evaluation rule**:

[ Given: Const parameter `const N: usize`, argument expression `e` ]

$$
\frac{\langle e, \emptyset \rangle \Downarrow_{\text{comptime}} \langle v, \emptyset \rangle \quad v \in \llbracket \text{usize} \rrbracket}{\text{const arg } N = v \text{ ok}}
\tag{E-Const-Arg}
$$

##### §9.2.4.4 Usage in Type Expressions

[21] Const parameters may appear in:

- Array length expressions: `[T; N]`
- Repeat array literals: `[value; N]`
- Const generic arguments to other types: `FixedBuffer<T, N>`
- Compile-time assertions in sequents: `[[ |- N > 0 => true ]]`

[22] Using const parameters in runtime-only contexts (assignments to `var` bindings, mutable operations) is ill-formed (diagnostic E10-103).

##### §9.2.4.5 Examples

**Example 10.2.4.1 (Const parameter for array size):**

```cursive
record Matrix<T, const ROWS: usize, const COLS: usize> {
    data: [[T; COLS]; ROWS],
}

procedure identity_matrix<const N: usize>(): Matrix<f64, N, N>
    [[ alloc::heap |- N > 0 => true ]]
{
    var matrix = Matrix { data: [[0.0; N]; N] }
    var i: usize = 0
    loop i < N {
        matrix.data[i][i] = 1.0
        i = i + 1
    }
    result matrix
}

let mat3: Matrix<f64, 3, 3> = identity_matrix()  // N inferred as 3
```

**Example 10.2.4.2 - invalid (Runtime value as const argument):**

```cursive
let size = read_input()
let buffer: FixedBuffer<i32, size> = create_buffer()
// error[E10-102]: const argument must be compile-time evaluable
```

#### §9.2.5 Grant Parameters [generic.parameter.grant]

##### §9.2.5.1 Declaration and Bounds

[23] Grant parameters enable grant-polymorphic programming: procedures may abstract over the grant sets required by higher-order callables.

[24] Grant parameters are declared as identifiers optionally bounded by grant sets using the subset operator `⊆`:

```cursive
procedure apply<T, U, ε>(value: T, f: (T) -> U ! ε): U
    [[ ε |- true => true ]]
{
    result f(value)
}
```

[25] The sequent clause `[[ ε |- ... ]]` uses the grant parameter `ε` directly as the grants clause. This indicates that the procedure requires exactly the grants that `f` requires.

##### §9.2.5.2 Grant Parameter Bounds

[26] Grant parameters may be bounded to limit the permitted grants:

```cursive
procedure safe_apply<T, U, ε>(value: T, f: (T) -> U ! ε): U
    [[ ε |- true => true ]]
    where ε ⊆ {io::write, alloc::heap}
{
    result f(value)
}
```

[27] The bound `ε ⊆ {io::write, alloc::heap}` constrains `ε` to be a subset of the specified grant set. At instantiation, the actual grant set must satisfy this constraint.

##### §9.2.5.3 Integration with Contractual Sequents

[28] Contractual sequents support generic syntax with grant parameters integrated directly into the sequent specification:

```cursive
procedure transform<T, U, [[ε |- P => Q]]>(data: T, f: (T) -> U ! ε): U ! ε
    [[ ε |- P => Q ]]
```

[29] The notation `<T, U, [[ε |- P => Q]]>` declares type parameters `T`, `U`, and embeds the grant parameter with its sequent in the generic parameter list. This is syntactic sugar for:

```cursive
procedure transform<T, U, ε>(data: T, f: (T) -> U ! ε): U ! ε
    [[ ε |- P => Q ]]
```

Both forms are equivalent; the embedded form documents the grant parameter's role more explicitly.

##### §9.2.5.4 Grant Parameter Inference

[30] Grant parameters are inferred from callable arguments:

```cursive
procedure twice<T, ε>(f: () -> T ! ε): (T, T)
    [[ ε |- true => true ]]
{
    result (f(), f())
}

// Inference:
let pure = || { result 42 }
let pair = twice(pure)  // ε inferred as ∅

let writer = || [[ io::write |- true => true ]] { println("hi") }
let pair2 = twice(writer)  // ε inferred as {io::write}
```

##### §9.2.5.5 Examples

**Example 10.2.5.1 (Grant-polymorphic map):**

```cursive
procedure map<T, U, ε>(values: [T], f: (T) -> U ! ε): [U]
    [[ alloc::heap, ε |- values.len() > 0 => result.len() == values.len() ]]
{
    let result = array::with_capacity(values.len())
    loop i in 0..values.len() {
        result.push(f(values[i]))
    }
    result result
}

// Usage with pure function
let numbers = [1, 2, 3, 4, 5]
let doubled = map(numbers, |x| { result x * 2 })

// Usage with grant-requiring function
let logged = map(numbers, |x| [[ io::write |- true => true ]] {
    println("Processing {}", x)
    result x * 2
})
```

**Example 10.2.5.2 (Bounded grant parameter):**

```cursive
procedure with_resource<T, ε>(f: () -> T ! ε): T ! ε
    [[ alloc::heap, ε |- true => true ]]
    where ε ⊆ {io::write, io::read}
{
    let resource = acquire_resource()
    defer { release_resource(resource) }
    result f()
}

// Valid: io::write ⊆ {io::write, io::read}
with_resource(|| [[ io::write ]] { println("hello") })

// Invalid: fs::write ⊄ {io::write, io::read}
with_resource(|| [[ fs::write ]] { write_file() })
// error[E10-301]: grant set violates bound
```

#### §9.2.6 Parameter Ordering and Defaults

##### §9.2.6.1 Parameter Order

[31] Parameters shall appear in the following order within a generic parameter list:

1. Type parameters
2. Const parameters
3. Grant parameters

[32] Implementations may relax this ordering provided doing so does not introduce ambiguity during parsing or instantiation. The recommended style places type parameters first for consistency with established conventions.

##### §9.2.6.2 Default Values

[33] Type and const parameters may have default values:

```cursive
record Buffer<T = u8, const SIZE: usize = 1024> {
    data: [T; SIZE],
}

let bytes: Buffer = Buffer::new()              // T = u8, SIZE = 1024
let custom: Buffer<i32> = Buffer::new()        // T = i32, SIZE = 1024 (default)
let large: Buffer<u8, 4096> = Buffer::new()    // T = u8, SIZE = 4096
```

[34] Parameters with defaults must appear after parameters without defaults. Violating this ordering produces diagnostic E10-104.

[35] Grant parameters do not support defaults. They must be explicitly provided or inferred from usage.

##### §9.2.6.3 Default Value Constraints

[36] Default values shall be:

- **Type defaults**: Any well-formed type visible in the declaring scope
- **Const defaults**: Compile-time evaluable expressions of the parameter's const type
- **No grant defaults**: Grant parameters have no default mechanism

[37] Default values are evaluated in the scope of the generic declaration. They may reference earlier parameters but not later ones:

```cursive
record Pair<T, U = T> {     // Valid: U defaults to T
    first: T,
    second: U,
}

record Invalid<T = U, U> {  // error[E10-105]: default references later parameter
    // ...
}
```

#### §9.2.7 Explicit Type Arguments (Turbofish)

[38] Explicit type arguments use turbofish syntax: `::<TypeArgs>` following an identifier.

```cursive
generic_instantiation
    ::= identifier "::" "<" type_argument_list ">"

type_argument_list
    ::= type_argument ("," type_argument)*

type_argument
    ::= type_expression              // Type argument
     | const_expression              // Const argument
     | "{" grant_set "}"             // Grant argument
```

[39] Turbofish syntax disambiguates instantiations when inference fails or when explicit types are desired for clarity:

```cursive
let values = make_vec::<i32>()                       // Explicit T = i32
let buffer = create_buffer::<u8, 512>()              // T = u8, SIZE = 512
let mapped = apply::<i32, string, {io::write}>(...)  // All parameters explicit
```

##### §9.2.7.1 Partial Explicit Arguments

[40] Implementations shall support partial explicit type arguments: providing only leading parameters allows trailing parameters to be inferred.

```cursive
procedure convert<T, U>(value: T): U
    where T: Into<U>
{
    result value.into()
}

let result = convert::<i32>(value)   // T = i32, U inferred from context
```

[41] Partial application must respect parameter order. Skipping early parameters while providing later ones is ill-formed (diagnostic E10-106).

##### §9.2.7.2 Turbofish Parsing

[42] The turbofish operator `::` binds tightly to the preceding identifier. In generic call expressions, `function::<Type>(args)` parses as `(function::<Type>)(args)`.

[43] When the `<` token could begin both a comparison operator and a type argument list, the parser shall disambiguate using lookahead: if the token sequence matches `:: <` followed by a valid type expression and `>`, it is a turbofish; otherwise it is the scope operator followed by a comparison.

#### §9.2.8 Constraints

[44] _Parameter uniqueness._ Parameter identifiers shall be unique within a generic parameter list. Duplicate parameters produce diagnostic E10-107.

[45] _Valid const types._ Const parameters shall have types from the `const_type` grammar (integer types or `bool`). Other types are ill-formed (diagnostic E10-108).

[46] _Bound well-formedness._ Behavior and contract bounds must refer to visible behaviors or contracts. Invalid bounds produce diagnostic E10-201 (§10.3).

[47] _Grant bound syntax._ Grant parameter bounds use `⊆` (subset) operator, not `:` (type constraint). Using `:` for grant bounds is a syntax error (diagnostic E10-109).

[48] _Shadowing outer bindings._ Type parameters shadow outer type bindings with the same identifier. No explicit `shadow` keyword is needed—parameters implicitly shadow (§6.3).

[49] _Const parameter scope._ Const parameters are available in type expressions and compile-time contexts within the declaring entity. They are not available in runtime expressions except when embedded in types. Using a const parameter as a runtime value is ill-formed (diagnostic E10-110).

#### §9.2.9 Semantics

##### §9.2.9.1 Type Parameter Semantics

[50] Type parameters are placeholders replaced during monomorphization (§10.6). They introduce universally quantified types:

$$
\text{record } R\langle T \rangle \equiv \forall T. \text{ RecordType}(R, T)
$$

$$
\text{procedure } f\langle T \rangle \equiv \forall T. \text{ ProcedureType}(f, T)
$$

[51] Within the generic entity's body, `T` is treated as an opaque type satisfying its bounds. Operations on values of type `T` are limited to those guaranteed by the bounds.

##### §9.2.9.2 Const Parameter Semantics

[52] Const parameters are compile-time values substituted during monomorphization:

$$
\text{FixedBuffer}\langle T, N \rangle \text{ instantiated with } N=1024 \equiv \text{FixedBuffer}_{\text{specialized}}(T, 1024)
$$

[53] After substitution, const parameters become literal values in the generated code. Array lengths, repeat counts, and other compile-time expressions are resolved to constants.

##### §9.2.9.3 Grant Parameter Semantics

[54] Grant parameters represent grant set variables. They participate in grant checking (Clause 12) as placeholders for actual grant sets determined at instantiation:

$$
\frac{\Gamma \vdash f : (T) \to U \; ! \varepsilon}{\Gamma \vdash \text{apply}(v, f) : U \; ! \varepsilon}
\tag{Grant-Param-Subst}
$$

[55] The procedure `apply` inherits the grant set `ε` from its callable argument `f`. Grant checking ensures that `apply`'s sequent lists `ε` in its grants clause.

#### §9.2.10 Diagnostics

[56] Type parameter diagnostics:

| Code    | Condition                                                          |
| ------- | ------------------------------------------------------------------ |
| E10-101 | Type parameter cannot be inferred; explicit type argument required |
| E10-102 | Const argument not compile-time evaluable                          |
| E10-103 | Const parameter used in runtime-only context                       |
| E10-104 | Parameter with default appears before parameter without default    |
| E10-105 | Default value references later parameter                           |
| E10-106 | Partial explicit arguments skip early parameters                   |
| E10-107 | Duplicate parameter name in generic parameter list                 |
| E10-108 | Const parameter has invalid type (not integer or bool)             |
| E10-109 | Grant parameter uses `:` instead of `⊆` for bound                  |
| E10-110 | Const parameter used as runtime value                              |

#### §9.2.11 Conformance Requirements

[57] Implementations shall:

1. Support type, const, and grant parameters in generic parameter lists
2. Enforce parameter ordering constraints and uniqueness (E10-107)
3. Validate const parameter types are limited to integers and bool (E10-108)
4. Evaluate const parameter arguments at compile time (E10-102)
5. Infer type and grant parameters from usage context when possible
6. Support turbofish syntax for explicit type arguments
7. Allow partial explicit type arguments (leading parameters only)
8. Support default values for type and const parameters
9. Enforce default parameter ordering (defaults after non-defaults)
10. Integrate grant parameters with contractual sequent specifications
11. Emit structured diagnostics (Annex E §E.5) for parameter violations

---

**Previous**: §10.1 Region Parameters Overview (§10.1 [generic.regions]) | **Next**: §10.3 Bounds and Where-Constraints (§10.3 [generic.bounds])
