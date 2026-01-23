# Part IV: Contracts and Clauses - §1. Overview

**Section**: §1 | **Part**: Contracts and Clauses (Part IV)
**Next**: [Behavioral Contracts](02_BehavioralContracts.md)

---

**Definition 1.1 (Contract System):** The contract system in Cantrip provides mechanisms for specifying and enforcing behavioral guarantees on procedures and types. Contracts include effects declarations (`uses`), preconditions (`must`), postconditions (`will`), type-level invariants (`where`), and behavioral contracts (`contract`) for polymorphism.

## 1. Contract System Overview

### 1.1 Introduction

**Purpose:** The contract system enables developers to specify precise behavioral requirements and guarantees, supporting:
- **Effect tracking**: What computational effects a procedure may perform
- **Preconditions**: What must be true before a procedure executes
- **Postconditions**: What must be true after a procedure executes
- **Type invariants**: What must always be true for a type's instances

**Design philosophy:**
- **Explicit over implicit**: All effects and requirements must be declared
- **Compositional**: Contracts compose naturally through procedure calls
- **Verifiable**: Contracts are checked statically where possible, dynamically where necessary
- **Zero-cost when unused**: Contracts that can be proven statically have no runtime cost

### 1.2 Contract Components

The Cantrip contract system consists of four major components:

#### 1.2.1 Effects (`uses`)

Effects declare what computational side effects a procedure may perform:

```cantrip
procedure log(message: String)
    uses io.write, alloc.heap
{
    println("{}", message)
}
```

**Properties:**
- Effects must be explicitly declared
- Effects compose transitively through calls
- Pure procedures have no `uses` clause
- See [§3. Effect Clauses](03_Effects.md)

#### 1.2.2 Preconditions (`must`)

Preconditions specify what must be true before a procedure executes:

```cantrip
procedure divide(numerator: f64, denominator: f64): f64
    must denominator != 0.0
{
    numerator / denominator
}
```

**Properties:**
- Checked at procedure entry
- Caller's responsibility to ensure
- Can be statically verified or dynamically checked
- See [§4. Precondition Clauses](04_Preconditions.md)

#### 1.2.3 Postconditions (`will`)

Postconditions specify what must be true after a procedure executes:

```cantrip
procedure increment(counter: mut i32): i32
    will result == @old(counter) + 1
{
    counter += 1
    counter
}
```

**Properties:**
- Checked at procedure exit
- Procedure's responsibility to ensure
- Can reference `result` and `@old(expr)` for pre-values
- See [§5. Postcondition Clauses](05_Postconditions.md)

#### 1.2.4 Type Constraints (`where`)

Type constraints specify invariants that must hold for all instances of a type:

```cantrip
record Percentage {
    value: i32

    where {
        value >= 0,
        value <= 100
    }
}
```

**Properties:**
- Checked at construction and mutation
- Define valid state space
- Maintained across all operations
- See [§6. Constraint Clauses](06_TypeConstraints.md)

### 1.3 Contract Syntax Summary

**Complete procedure signature with contracts:**

```cantrip
procedure name<Generics>(params): ReturnType
    uses effect1, effect2, ...
    must {
        precondition1,
        precondition2,
        ...
    }
    will {
        postcondition1,
        postcondition2,
        ...
    }
{
    body
}
```

**Type with constraints:**

```cantrip
record TypeName<Generics> {
    fields

    where {
        constraint1,
        constraint2,
        ...
    }
}
```

### 1.4 Contract Checking

Contracts are checked through a combination of static and dynamic verification:

**Static checking (compile-time):**
- Effect type checking
- Simple precondition/postcondition verification
- Type constraint checking for const generics
- Flow-sensitive analysis

**Dynamic checking (runtime):**
- Preconditions that depend on runtime values
- Postconditions that cannot be proven statically
- Type constraints on field values
- Assertion-based verification

**Optimization:**
The compiler eliminates redundant checks and proven contracts to minimize runtime overhead.

### 1.5 Contract Hierarchy

```
Contract System
├── Procedure-Level Contracts
│   ├── Effects (uses)
│   ├── Preconditions (must)
│   └── Postconditions (will)
│
├── Type-Level Contracts
│   └── Invariants (where)
│
└── Behavioral Contracts
    └── Abstract Interfaces (contract)
```

**Interaction:**
- Type constraints are always maintained
- Preconditions can reference type constraints
- Postconditions can reference type constraints
- Effects are orthogonal to conditions
- Behavioral contracts declare abstract procedures with contracts

### 1.6 Contract Composition

Contracts compose naturally through procedure calls:

```cantrip
procedure helper(x: i32): i32
    must x > 0
    will result > x
{
    x + 1
}

procedure caller(y: i32): i32
    must y > 5        // Implies y > 0
    will result > y    // Transitively > 5
{
    helper(y)              // helper's contracts apply
}
```

**Composition rules:**
1. **Effects aggregate**: Caller uses all effects of callees
2. **Preconditions strengthen**: Caller must ensure callee preconditions
3. **Postconditions weaken**: Caller can rely on callee postconditions
4. **Type constraints persist**: Always maintained

See [§7. Clause Composition](07_ContractComposition.md)

### 1.7 Relationship to Other Features

**Generics (Part II, §7):**
- Contracts can constrain generic parameters
- Generic procedures inherit contract requirements

**Traits (Part II, §6):**
- Trait procedures can declare contracts
- Attaching types inherit contract obligations

**Self Type (Part II, §12):**
- Contracts can reference `Self` in trait/type contexts

**Permissions (Part III):**
- Contracts work with permission system
- Postconditions can specify permission changes

**Effect Clauses (Part IV, §3):**
- Effects are part of procedure type signatures
- Effect checking is statically enforced

### 1.8 Design Rationale

**Why explicit contracts?**

1. **Correctness**: Catch errors at compile time or procedure boundaries
2. **Documentation**: Contracts serve as machine-checked documentation
3. **Optimization**: Compiler can optimize based on contract guarantees
4. **Verification**: Enable formal verification and proof
5. **Clarity**: Make implicit assumptions explicit

**Why multiple contract types?**

Each contract type serves a distinct purpose:
- **Effects**: Track computational side effects
- **Preconditions**: Specify caller obligations
- **Postconditions**: Specify procedure guarantees
- **Type constraints**: Define valid state spaces

**Why both static and dynamic checking?**

Some contracts are decidable at compile time (e.g., effect checking, const generic constraints), while others depend on runtime values (e.g., `x > 0` where `x` is a parameter). Hybrid checking provides the best balance of safety and flexibility.

### 1.9 Examples

**Example 1: Safe division**
```cantrip
procedure safe_divide(a: f64, b: f64): Result<f64, String>
    will match result {
        Ok(v) => v == a / b,
        Err(_) => b == 0.0
    }
{
    if b == 0.0 {
        Err("Division by zero")
    } else {
        Ok(a / b)
    }
}
```

**Example 2: Array bounds**
```cantrip
procedure get<T>(arr: [T], index: usize): T
    must index < arr.len()
    will result == arr[index]
{
    arr[index]  // Safe: precondition ensures bounds
}
```

**Example 3: Sorted insertion**
```cantrip
record SortedVec<T: Ord> {
    elements: Vec<T>

    where {
        is_sorted(elements)
    }

    procedure insert(mut $, item: T)
        will is_sorted($.elements)
    {
        let pos = $.elements.binary_search(item).unwrap_or_else(|e| e)
        $.elements.insert(pos, item)
        // Postcondition automatically checked
    }
}
```

**Example 4: Effect composition**
```cantrip
procedure read_config(path: String): Result<Config, Error>
    uses io.read, alloc.heap
{
    let contents = std::fs::read_to_string(path)?  // uses io.read
    parse_config(contents)                           // uses alloc.heap
}

procedure parse_config(json: String): Result<Config, Error>
    uses alloc.heap
{
    // Parse JSON into Config
    ...
}
```

### 1.10 Specification Structure

This part of the specification is organized as follows:

1. **Overview** (this document) - Introduction and motivation
2. **Behavioral Contracts** - The `contract` keyword for polymorphism and extension
3. **Effect Clauses** - The `uses` clause and effect system
4. **Precondition Clauses** - The `must` clause
5. **Postcondition Clauses** - The `will` clause
6. **Constraint Clauses** - The `where` clause for types
7. **Clause Composition** - How contracts and clauses combine and interact

Each section provides:
- Concrete and abstract syntax
- Static semantics (well-formedness and type rules)
- Dynamic semantics (evaluation and checking)
- Examples and patterns
- Interaction with other language features

---

**Definition 2.1 (Behavioral Contract):** A behavioral contract defines an abstract set of procedure signatures with associated contracts (effects, preconditions, postconditions) that types can implement to participate in polymorphic behavior.

## 2. Behavioral Contracts

### 2.1 Overview

**Key innovation/purpose:** Behavioral contracts enable:
- **Abstract behavioral specifications**: Define what types must do, not how
- **Polymorphism through contracts**: Any type satisfying a contract can be used polymorphically
- **Contract extension**: Contracts can extend other contracts, inheriting requirements
- **Compile-time verification**: Contract satisfaction is statically verified
- **Zero abstraction cost**: No vtables or runtime dispatch overhead

**Contrast with traits:**

| Aspect | Contracts | Traits |
|--------|-----------|--------|
| **Purpose** | Abstract behavioral specification | Concrete code reuse |
| **Implementations** | **Abstract only** (signatures + clauses) | **Concrete only** (template code) |
| **Polymorphism** | ✓ Yes (interface-based) | ✗ No (mixin-based) |
| **Code sharing** | ✗ No (purely abstract) | ✓ Yes (template implementations) |
| **Use for** | Defining interfaces, polymorphism | Sharing code across types |

**Critical distinction:**
- **Contracts are ABSTRACT**: Only procedure signatures with contract clauses (uses, must, will). No procedure bodies.
- **Traits are CONCRETE**: Template implementations that are attached to types. Always have procedure bodies.

**Example - Contract (abstract):**
```cantrip
contract Drawable {
    procedure draw($)
        uses io.write           // Abstract requirement

    procedure bounds($): Rectangle
        will {                  // Abstract guarantee
            result.width >= 0.0,
            result.height >= 0.0
        }

    // NO IMPLEMENTATIONS - contracts are purely abstract
}
```

**Example - Trait (concrete):**
```cantrip
trait DefaultRenderable {
    procedure render($) {         // Concrete implementation
        println("Rendering default")
    }

    procedure render_with_color($, color: Color) {  // Concrete template
        $.set_color(color)
        $.render()
    }

    // ALL PROCEDURES HAVE BODIES - traits are concrete templates
}
```

**When to use contracts:**
- Defining abstract interfaces for polymorphism
- Specifying behavioral requirements across unrelated types
- Building extensible APIs with strong guarantees
- Replacing traditional inheritance hierarchies
- When you need interface polymorphism (different types, same interface)

**When to use traits:**
- Sharing concrete implementation code across types
- Providing default/template behavior
- Mixins and code reuse patterns
- When multiple types need the same implementation

**When not to use contracts:**
- Sharing concrete implementation code (use traits instead)
- Providing default implementations (use traits instead)
- Simple type aliases or wrappers
- Internal implementation details

### 2.2 Contract Declaration

#### 2.2.1 Basic Syntax

**Concrete syntax:**
```cantrip
contract ContractName {
    procedure_signature;
    procedure_signature;
    ...
}
```

**Example:**
```cantrip
contract Drawable {
    procedure draw($)
    procedure bounds($): Rectangle
}
```

#### 2.2.2 Contracts with Requirements

**Syntax with contracts:**
```cantrip
contract Comparable {
    procedure compare($, other: Self): i32
        will {
            (result < 0) == ($ < other),
            (result == 0) == ($ == other),
            (result > 0) == ($ > other)
        }
}
```

**Example with effects:**
```cantrip
contract Serializable {
    procedure serialize($): Result<String, Error>
        uses alloc.heap

    procedure deserialize(data: String): Result<Self, Error>
        must !data.is_empty()
        uses alloc.heap
}
```

### 2.3 Contract Implementation

Types implement contracts by providing concrete implementations of all required procedures.

#### 2.3.1 Implementation Syntax

**Concrete syntax:**
```cantrip
record TypeName: ContractName {
    // fields

    procedure signature { body }
    procedure signature { body }
    ...
}
```

**Example:**
```cantrip
record Point: Drawable {
    x: f64
    y: f64

    procedure draw($) {
        println("Point at ({}, {})", $.x, $.y)
    }

    procedure bounds($): Rectangle {
        Rectangle { x: $.x, y: $.y, width: 0.0, height: 0.0 }
    }
}
```

#### 2.3.2 Contract Satisfaction Rules

**Rule:**
```
contract C {
    procedure m(T₁) -> T₂
        must P;
        will Q;
        uses ε;
}

record T: C {
    procedure m(T₁) -> T₂
        must P';
        will Q';
        uses ε';
}
─────────────────────────────────────────────────
Valid iff:
  P ⇒ P'      (implementation weakens precondition)
  Q' ⇒ Q      (implementation strengthens postcondition)
  ε' ⊆ ε      (implementation uses fewer effects)
```

**Example:**
```cantrip
contract Validator {
    procedure validate($, value: i32): bool
        must value >= 0
        will result => value < 100
}

record StrictValidator: Validator {
    procedure validate($, value: i32): bool
        must value >= -10          // Weaker (accepts more inputs)
        will result => value < 50   // Stronger (better guarantee)
    {
        value >= -10 && value < 50
    }
}
```

### 2.4 Contract Extension

Contracts can extend other contracts, inheriting all their requirements.

#### 2.4.1 Extension Syntax

**Concrete syntax:**
```cantrip
contract SubContract extends SuperContract {
    additional_signatures;
}
```

**Example:**
```cantrip
contract Displayable {
    procedure display($)
}

contract Interactive extends Displayable {
    procedure handle_click($, x: i32, y: i32)
    procedure handle_key($, key: char)
}
```

#### 2.4.2 Multiple Extension

Contracts can extend multiple contracts:

```cantrip
contract Serializable {
    procedure serialize($): String
}

contract Comparable {
    procedure compare($, other: Self): i32
}

contract Storable extends Serializable, Comparable {
    procedure id($): u64
}
```

#### 2.4.3 Extension Semantics

**Rule:**
```
contract C₂ extends C₁

record T: C₂ requires:
  - Implement all procedures from C₁
  - Implement all procedures from C₂
  - Satisfy all contracts from both C₁ and C₂
```

**Example:**
```cantrip
contract Shape {
    procedure area($): f64
        will result >= 0.0
}

contract ColoredShape extends Shape {
    procedure color($): Color
    procedure set_color(mut $, c: Color)
}

record Circle: ColoredShape {
    radius: f64
    color: Color

    // Must implement Shape's requirements
    procedure area($): f64
        will result >= 0.0
    {
        std::f64::consts::PI * $.radius * $.radius
    }

    // Plus ColoredShape's requirements
    procedure color($): Color { $.color }
    procedure set_color(mut $, c: Color) { $.color = c }
}
```

### 2.5 Polymorphism Through Contracts

#### 2.5.1 Contract Types

Any type implementing a contract can be used where that contract is required.

**Type syntax:**
```cantrip
procedure process_drawable<T>(d: T) where T: Drawable {
    d.draw()
}

procedure process_drawables<T>(items: Vec<T>) where T: Drawable {
    for item in items {
        item.draw()
    }
}
```

#### 2.5.2 Static Dispatch

Contract-based polymorphism uses static dispatch (monomorphization):

```cantrip
contract Processor {
    procedure process($, data: [u8]): Result<Vec<u8>, Error>
        uses alloc.heap
}

procedure batch_process<T: Processor>(
    processor: T,
    datasets: Vec<[u8]>
): Vec<Result<Vec<u8>, Error>> {
    datasets.map(|data| processor.process(data))
}
// Monomorphized for each concrete type implementing Processor
```

#### 2.5.3 Contract Bounds

Type parameters can be bounded by contracts:

```cantrip
record Container<T> where T: Serializable {
    items: Vec<T>

    procedure save($, path: String): Result<(), Error>
        uses io.write, alloc.heap
    {
        let data = $.items
            .iter()
            .map(|item| item.serialize())
            .collect::<Result<Vec<_>, _>>()?
        std::fs::write(path, data.join("\n"))
    }
}
```

### 2.6 Associated Types in Contracts

Contracts can declare associated types that implementations must specify.

#### 2.6.1 Associated Type Syntax

```cantrip
contract Iterator {
    type Item

    procedure next(mut $): Option<Self::Item>
}

record Range: Iterator {
    current: i32
    end: i32

    type Item = i32

    procedure next(mut $): Option<i32> {
        if $.current < $.end {
            let result = $.current
            $.current += 1
            Some(result)
        } else {
            None
        }
    }
}
```

#### 2.6.2 Associated Types with Constraints

```cantrip
contract Container {
    type Element where Element: Clone

    procedure get($, index: usize): Option<Self::Element>
    procedure set(mut $, index: usize, value: Self::Element)
}
```

### 2.7 Implementing Contracts with Different Types

When you need to work with different types implementing the same contract, use enums for explicit, zero-cost polymorphism:

```cantrip
contract Logger {
    procedure log($, message: String)
        uses io.write
}

record FileLogger {
    path: String

    procedure log($, message: String) uses io.write {
        // Write to file
    }
}

record ConsoleLogger {
    procedure log($, message: String) uses io.write {
        println("{}", message)
    }
}

// Explicit, zero-cost polymorphism with enums
enum AnyLogger {
    File(FileLogger),
    Console(ConsoleLogger),
}

record Application {
    logger: AnyLogger  // Explicit, zero-cost

    procedure log_event($, event: String)
        uses io.write
    {
        match $.logger {
            AnyLogger.File(l) => l.log(event),
            AnyLogger.Console(l) => l.log(event),
        }
    }
}
```

**Why enums instead of dynamic dispatch:**
- Zero runtime cost (no vtables or indirection)
- Explicit (you see all possible types)
- Exhaustive checking (compiler ensures all cases handled)
- Aligns with Cantrip's "explicit over implicit" philosophy

### 2.8 Coherence and Orphan Rules

Contract implementations must follow coherence rules to ensure uniqueness.

#### 2.8.1 Orphan Rule

You can implement a contract for a type only if:
- The contract is defined in your crate, OR
- The type is defined in your crate

**Valid:**
```cantrip
// In crate A:
contract MyContract { ... }

// In crate B:
record MyType: MyContract {
    // fields
    // procedures implementing MyContract
}  // OK: MyType is local
```

**Invalid:**
```cantrip
// In crate C (different from A and B):
// Cannot implement MyContract for MyType  // ERROR: Neither is local
```

#### 2.8.2 Blanket Implementations

Generic implementations must not overlap:

```cantrip
contract Debug {
    procedure debug($): String
}

// Note: Blanket implementations require language support
// for automatic implementation across types with specific constraints
// Syntax for blanket implementations is TBD

// ERROR: Would overlap if String also tries to implement Debug
// when blanket implementation exists for all T: Display
```

### 2.9 Advanced Contract Patterns

#### 2.9.1 Marker Contracts

Contracts with no procedures serve as markers:

```cantrip
contract Send {}  // Types safe to send between threads
contract Sync {}  // Types safe to share between threads

record ThreadSafeCounter: Send, Sync {
    count: AtomicU32;

    // Marker contracts have no procedures to implement
}
```

#### 2.9.2 Sealed Contracts

Contracts can be sealed to prevent external implementation:

```cantrip
contract Sealed {
    private procedure seal_marker();
}

contract MyContract extends Sealed {
    procedure my_method($);
}

// Only types in this module can implement MyContract
// (cannot implement private procedure from outside)
```

#### 2.9.3 Supertrait Requirements

Contracts can require implementers to also implement other contracts:

```cantrip
contract Eq {
    procedure eq($, other: Self): bool
}

contract Ord where Self: Eq {
    procedure cmp($, other: Self): Ordering
}

// To implement Ord, must also implement Eq
record Priority: Eq, Ord {
    value: i32

    procedure eq($, other: Self): bool {
        $.value == other.value
    }

    procedure cmp($, other: Self): Ordering {
        $.value.cmp(other.value)
    }
}
```

### 2.10 Formal Semantics

#### 2.10.1 Contract Well-Formedness

**Rule [ContractWF]:**
```
Γ ⊢ signature₁ : τ₁
Γ ⊢ signature₂ : τ₂
...
Γ ⊢ signatureₙ : τₙ
All signatures distinct
─────────────────────────────────────────────────
Γ ⊢ contract C { signature₁; ...; signatureₙ } ok
```

#### 2.10.2 Implementation Well-Formedness

**Rule [ImplWF]:**
```
Γ ⊢ contract C { s₁; ...; sₙ }
Γ ⊢ record T: C { m₁; ...; mₙ }
∀i. signature(mᵢ) matches sᵢ
∀i. contracts(mᵢ) satisfy contracts(sᵢ)
─────────────────────────────────────────────────
Γ ⊢ implementation valid
```

#### 2.10.3 Contract Extension

**Rule [ExtensionWF]:**
```
Γ ⊢ contract C₁ { s₁; ...; sₘ }
Γ ⊢ contract C₂ extends C₁ { t₁; ...; tₙ }
{s₁, ..., sₘ} ∩ {t₁, ..., tₙ} = ∅
─────────────────────────────────────────────────
Γ ⊢ C₂ inherits s₁, ..., sₘ, t₁, ..., tₙ
```

### 2.11 Complete Example

A comprehensive example showing contracts, extension, and polymorphism:

```cantrip
// Base contract
contract Drawable {
    procedure draw($)
        uses io.write

    procedure bounds($): Rectangle
        will {
            result.width >= 0.0,
            result.height >= 0.0
        }
}

// Extended contract
contract Transformable extends Drawable {
    procedure translate(mut $, dx: f64, dy: f64)
    procedure scale(mut $, factor: f64)
        must factor > 0.0
}

// Record implementing extended contract
record Circle: Transformable {
    x: f64
    y: f64
    radius: f64

    where {
        radius > 0.0
    }

    procedure draw($)
        uses io.write
    {
        println("Circle at ({}, {}) with radius {}", $.x, $.y, $.radius)
    }

    procedure bounds($): Rectangle
        will {
            result.width >= 0.0,
            result.height >= 0.0
        }
    {
        Rectangle {
            x: $.x - $.radius,
            y: $.y - $.radius,
            width: $.radius * 2.0,
            height: $.radius * 2.0
        }
    }

    procedure translate(mut $, dx: f64, dy: f64) {
        $.x += dx
        $.y += dy
    }

    procedure scale(mut $, factor: f64)
        must factor > 0.0
    {
        $.radius *= factor
    }
}

// Polymorphic function using contract bound
procedure render_shapes<T>(shapes: Vec<T>)
    where T: Drawable
{
    for shape in shapes {
        shape.draw()
        let bounds = shape.bounds()
        println("  Bounds: {}x{}", bounds.width, bounds.height)
    }
}

// Usage
procedure main() {
    let circles = vec![
        Circle { x: 0.0, y: 0.0, radius: 5.0 },
        Circle { x: 10.0, y: 10.0, radius: 3.0 }
    ]

    render_shapes(circles)  // Monomorphized for Circle
}
```

---

**Definition 3.1 (Effect Clause):** An effect clause (`uses`) specifies the computational side effects that a procedure may perform. Effects include I/O operations, memory allocation, network access, and other observable interactions with the environment outside of pure computation.

## 3. Effect Clauses

### 3.1 Overview

**Key innovation/purpose:** The effect system makes side effects explicit in procedure signatures, enabling:
- **Static tracking**: Know what effects a procedure performs without reading its body
- **Effect polymorphism**: Generic procedures can be bounded by effect requirements
- **Compositional reasoning**: Effects compose transitively through calls
- **Optimization**: Pure procedures (no effects) can be optimized aggressively

**When to use `uses`:**
- Procedure performs I/O (file, network, terminal)
- Procedure allocates heap memory
- Procedure accesses global state
- Procedure spawns threads or tasks
- Procedure performs FFI calls
- Any observable interaction beyond pure computation

**When NOT to use `uses`:**
- Pure computation (arithmetic, logic, local variables)
- Stack allocation (automatic in Cantrip)
- Immutable data structure operations
- Const expression evaluation

**Relationship to other features:**
- **Procedures**: Effects are part of procedure type signatures
- **Map Types (§10)**: Map types can include effect bounds
- **Traits**: Trait procedures can declare effect requirements
- **Generics**: Generic procedures can be bounded by effects
- **Contracts**: Effects compose with preconditions/postconditions

### 3.2 Syntax

#### 3.2.1 Concrete Syntax

**Effect declaration:**
```ebnf
UsesClause ::= "uses" EffectExpr

EffectExpr  ::= EffectPath ("," EffectPath)*
              | "pure"

EffectPath  ::= Ident ("." Ident)*
```

**Examples:**
```cantrip
// Single effect
procedure print(s: String)
    uses io.write
{
    println("{}", s)
}

// Multiple effects
procedure load_config(path: String): Config
    uses io.read, alloc.heap
{
    let contents = std::fs::read(path)?
    parse_json(contents)
}

// Explicit purity (optional)
procedure add(x: i32, y: i32): i32
    uses pure
{
    x + y
}

// No uses clause = pure by default
procedure multiply(x: i32, y: i32): i32 {
    x * y
}
```

#### 3.2.2 Abstract Syntax

**Effect representation:**
```
ε ::= ∅                           (no effects / pure)
    | {e₁, e₂, ..., eₙ}          (effect set)
    | ε₁ ∪ ε₂                     (effect union)

e ::= io.read | io.write
    | net.tcp | net.udp
    | alloc.heap
    | thread.spawn
    | ffi.call
    | ...
```

**Procedure signature with effects:**
```
procedure f(τ₁, ..., τₙ): τ uses ε

Desugars to map type:
map(τ₁, ..., τₙ) -> τ ! ε
```

**Effect lattice:**
```
               ⊤ (all effects)
              / | \
          io  net  alloc  ...
         / \   |
    read write |
              ...
               |
               ∅ (pure)
```

#### 3.2.3 Basic Examples

**Example 1: I/O effects**
```cantrip
procedure read_file(path: String): Result<String, Error>
    uses io.read, alloc.heap
{
    std::fs::read_to_string(path)
}

procedure write_file(path: String, content: String): Result<(), Error>
    uses io.write
{
    std::fs::write(path, content)
}
```

**Example 2: Network effects**
```cantrip
procedure fetch_url(url: String): Result<Response, Error>
    uses net.tcp, alloc.heap
{
    http::get(url)
}
```

**Example 3: Pure procedure (no effects)**
```cantrip
procedure fibonacci(n: i32): i32 {
    if n <= 1 {
        n
    } else {
        fibonacci(n - 1) + fibonacci(n - 2)
    }
}
```

### 3.3 Static Semantics

#### 3.3.1 Well-Formedness Rules

**[WF-Effect] Well-formed effect:**
```
[WF-Effect]
e ∈ EffectDomain
─────────────────
e well-formed
```

**Explanation:** Effects must come from the defined effect domain.

**[WF-EffectSet] Well-formed effect set:**
```
[WF-EffectSet]
e₁ well-formed    ...    eₙ well-formed
─────────────────────────────────────────
{e₁, ..., eₙ} well-formed
```

**Explanation:** An effect set is well-formed if all its elements are well-formed effects.

**[WF-UsesClause] Well-formed uses clause:**
```
[WF-UsesClause]
ε well-formed
procedure f(...): τ { body } well-formed
─────────────────────────────────────────
procedure f(...): τ uses ε { body } well-formed
```

**Explanation:** A uses clause is well-formed if its effect set is well-formed.

#### 3.3.2 Type Rules

**[T-PureProc] Pure procedure:**
```
[T-PureProc]
Γ ⊢ procedure f(params): τ { body }
body performs no effects
─────────────────────────────────────
Γ ⊢ f : map(params) -> τ ! ∅
```

**Explanation:** Procedures without uses clauses and no effectful operations are pure.

**[T-EffectProc] Effectful procedure:**
```
[T-EffectProc]
Γ ⊢ procedure f(params): τ uses ε { body }
Γ ⊢ body : τ
effects(body) ⊆ ε
─────────────────────────────────────
Γ ⊢ f : map(params) -> τ ! ε
```

**Explanation:** A procedure's declared effects must include all effects performed in its body.

**[T-EffectCall] Procedure call with effects:**
```
[T-EffectCall]
Γ ⊢ f : map(τ₁, ..., τₙ) -> τ ! ε_f
Γ ⊢ args : τ₁, ..., τₙ
─────────────────────────────────────
Γ ⊢ f(args) : τ
effects(f(args)) = ε_f
```

**Explanation:** Calling a procedure incurs its declared effects.

**[T-EffectCompose] Effect composition:**
```
[T-EffectCompose]
Γ ⊢ e₁ : τ₁    effects(e₁) = ε₁
Γ ⊢ e₂ : τ₂    effects(e₂) = ε₂
─────────────────────────────────────
Γ ⊢ {e₁; e₂} : τ₂
effects({e₁; e₂}) = ε₁ ∪ ε₂
```

**Explanation:** Sequential composition unions effects.

**[T-EffectSubsume] Effect subsumption:**
```
[T-EffectSubsume]
Γ ⊢ procedure f(...): τ uses ε_declared { body }
effects(body) = ε_actual
ε_actual ⊆ ε_declared
─────────────────────────────────────
procedure f well-typed
```

**Explanation:** Actual effects must be a subset of declared effects.

**[T-EffectViolation] Effect violation:**
```
[T-EffectViolation]
Γ ⊢ procedure f(...): τ uses ε_declared { body }
effects(body) = ε_actual
ε_actual ⊈ ε_declared
─────────────────────────────────────
ERROR: Undeclared effect
```

**Explanation:** Using effects not in the uses clause is an error.

#### 3.3.3 Type Properties

**Theorem 2.1 (Effect Soundness):**

If a procedure declares effects ε, then it performs at most effects in ε:

```
Γ ⊢ f : map(τ) -> U ! ε
f executes with trace σ
effects_observed(σ) ⊆ ε
```

**Proof sketch:** By effect type checking. The compiler verifies that all effectful operations in the body are covered by the declared effects.

**Theorem 2.2 (Effect Composition):**

Effects compose transitively through procedure calls:

```
f uses ε_f
g uses ε_g, calls f
────────────────────
g's actual effects ⊇ ε_f ∪ ε_g
```

**Proof sketch:** By [T-EffectCompose]. Each call adds its effects to the total.

**Theorem 2.3 (Pure Procedure Optimization):**

Procedures with no effects can be:
- Reordered (no observable side effects)
- Memoized (pure functions)
- Eliminated if result unused (dead code elimination)

**Property 2.1 (Effect Monotonicity):**

Adding effects to a procedure makes it less restrictive (subsumption):

```
f : map(τ) -> U ! ε₁
ε₁ ⊆ ε₂
────────────────────
f can be used where map(τ) -> U ! ε₂ is expected
```

**Property 2.2 (Effect Lattice):**

Effects form a lattice:
- Bottom: ∅ (pure)
- Join: ε₁ ∪ ε₂ (union)
- Top: all effects
- Ordering: ε₁ ⊆ ε₂

### 3.4 Dynamic Semantics

#### 3.4.1 Evaluation Rules

**[E-PureEval] Pure procedure evaluation:**
```
[E-PureEval]
procedure f(params): τ { body }
no effects declared
⟨body[params ↦ args], σ⟩ ⇓ ⟨v, σ⟩
─────────────────────────────────────
⟨f(args), σ⟩ ⇓ ⟨v, σ⟩
```

**Explanation:** Pure procedures don't modify external state (σ unchanged).

**[E-EffectEval] Effectful procedure evaluation:**
```
[E-EffectEval]
procedure f(params): τ uses ε { body }
⟨body[params ↦ args], σ, π⟩ ⇓ ⟨v, σ', π'⟩
effects_performed(π') ⊆ ε
─────────────────────────────────────
⟨f(args), σ, π⟩ ⇓ ⟨v, σ', π'⟩
```

**Explanation:** Effectful procedures can modify state and perform effects in ε.

Where:
- σ = memory state
- π = effect trace (sequence of effects performed)

**[E-EffectIO] I/O effect:**
```
[E-EffectIO]
procedure write_file uses io.write
⟨write_file(path, data), σ, π⟩
performs file write operation
─────────────────────────────────────
⟨write_file(path, data), σ, π · io.write⟩ ⇓ ⟨(), σ, π · io.write⟩
```

**Explanation:** I/O effects are recorded in the effect trace.

#### 3.4.2 Effect Checking

**Static checking (compile-time):**

The compiler tracks effects through control flow:

```cantrip
procedure example(flag: bool) uses io.write {
    if flag {
        println("true")    // io.write
    } else {
        println("false")   // io.write
    }
    // Both branches have io.write, so uses io.write
}
```

**Conservative analysis:**

The compiler is conservative - if an effect might occur, it must be declared:

```cantrip
procedure conditional_io(flag: bool)
    uses io.write  // Required even though might not execute
{
    if flag {
        println("Hello")  // io.write
    }
    // No else branch, but io.write is possible
}
```

**Effect inference:**

The compiler can infer effects for procedure bodies:

```cantrip
procedure helper(x: i32): i32 {
    x + 1  // Compiler infers: pure (no effects)
}

procedure caller() uses io.write {
    let y = helper(5)  // OK: pure ⊆ {io.write}
    println("{}", y)  // io.write
}
```

#### 3.4.3 Runtime Behavior

Effects have no runtime representation - they are purely compile-time information:

```cantrip
// Source
procedure log(msg: String) uses io.write {
    println("{}", msg)
}

// Compiled (no effect metadata)
void log(String msg) {
    println(msg)  // Just the operation
}
```

**No runtime cost:** Effect checking is entirely static. There's no runtime effect tracking or checking overhead.

### 3.5 Standard Effects

Cantrip provides a standard hierarchy of effects:

#### 3.5.1 I/O Effects

```
io.read      - Read from files, stdin
io.write     - Write to files, stdout, stderr
io.seek      - File seeking operations
io.metadata  - File metadata queries
```

#### 3.5.2 Network Effects

```
net.tcp      - TCP socket operations
net.udp      - UDP socket operations
net.dns      - DNS lookups
net.http     - HTTP client operations
```

#### 3.5.3 Memory Effects

```
alloc.heap   - Heap allocation (Vec, Box, String, etc.)
alloc.global - Global allocator access
```

#### 3.5.4 Concurrency Effects

```
thread.spawn - Spawn new threads
thread.join  - Join threads
sync.lock    - Acquire locks
sync.atomic  - Atomic operations
```

#### 3.5.5 System Effects

```
sys.env      - Environment variable access
sys.time     - System time queries
sys.random   - Random number generation
sys.exit     - Process termination
```

#### 3.5.6 FFI Effects

```
ffi.call     - Foreign function calls
unsafe.ptr   - Unsafe pointer operations
```

### 3.6 Examples and Patterns

#### 3.6.1 Effect Polymorphism

```cantrip
procedure map<T, U, E>(
    items: Vec<T>,
    f: map(T) -> U ! E
): Vec<U> ! E
    uses E, alloc.heap
{
    var result = Vec::with_capacity(items.len())
    for item in items {
        result.push(f(item))  // f's effects propagate
    }
    result
}

// Usage with pure function
let numbers = vec![1, 2, 3]
let doubled = map(numbers, |x| x * 2)  // E = ∅ (pure)

// Usage with effectful function
let messages = vec!["a", "b", "c"]
let logged = map(messages, |s| {
    println("{}", s)  // E = {io.write}
    s
})
```

#### 3.6.2 Effect Bounds on Traits

```cantrip
contract Processor {
    procedure process($, data: Data): Result
        uses alloc.heap  // All implementations can use heap
}

record SimpleProcessor: Processor {
    // fields...

    procedure process($, data: Data): Result
        uses alloc.heap  // Exactly matches contract
    {
        // Process with allocation
        ...
    }
}

record PureProcessor: Processor {
    // fields...

    procedure process($, data: Data): Result
        // No uses clause: ∅ ⊆ {alloc.heap}
    {
        // Process without allocation
        ...
    }
}
```

#### 3.6.3 Conditional Effects

```cantrip
procedure save_if_changed<T>(
    cache: mut Cache<T>,
    key: String,
    value: T
) uses alloc.heap, io.write {
    if cache.get(&key) != Some(&value) {
        cache.insert(key.clone(), value)  // alloc.heap
        persist_cache(&cache)             // io.write
    }
    // Effects declared even though conditionally executed
}
```

#### 3.6.4 Effect Isolation

```cantrip
procedure run_with_io<T>(f: map() -> T ! {io.read, io.write}): T
    uses io.read, io.write
{
    f()  // f's effects contained within this procedure
}

procedure main() uses io.read, io.write {
    let config = run_with_io(|| {
        load_config("config.toml")  // io.read
    })

    run_with_io(|| {
        save_results(config)  // io.write
    })
}
```

#### 3.6.5 Pure Procedures

```cantrip
// Explicitly pure
procedure factorial(n: i32): i32 uses pure {
    if n <= 1 {
        1
    } else {
        n * factorial(n - 1)
    }
}

// Implicitly pure (no uses clause)
procedure sum(numbers: [i32]): i32 {
    var total = 0
    for num in numbers {
        total += num
    }
    total
}
```

### 3.7 Advanced Topics

#### 3.7.1 Effect Refinement

Procedures can be more specific than their contract bounds:

```cantrip
contract Logger {
    procedure log($, msg: String)
        uses io.write, alloc.heap  // Upper bound
}

record BufferedLogger: Logger {
    buffer: Vec<String>

    procedure log($, msg: String)
        uses alloc.heap  // More specific (no io.write yet)
    {
        $.buffer.push(msg)  // Just buffering
    }
}
```

#### 3.7.2 Effect Annotations for Optimization

Pure procedures enable aggressive optimization:

```cantrip
procedure expensive_computation(x: i32): i32 {
    // Pure - can be memoized
    let mut result = 1
    for _ in 0..x {
        result = result * 2 + 1
    }
    result
}

// Compiler can:
// - Memoize results
// - Reorder calls
// - Eliminate if unused
```

#### 3.7.3 Effect Tracking Through Closures

```cantrip
procedure with_effects() uses io.write, alloc.heap {
    let logger = |msg: String| {
        println("{}", msg)  // io.write
    }

    let data = vec![1, 2, 3]  // alloc.heap

    logger("Processing")
    for x in data {
        logger(format!("{}", x))  // alloc.heap + io.write
    }
}
```

---

**Definition 4.1 (Precondition Clause):** A precondition clause (`must`) is a predicate that must be true when a procedure is called. Preconditions specify the caller's obligations and define the valid input space for a procedure.

## 4. Precondition Clauses

### 4.1 Overview

**Key innovation/purpose:** Preconditions make procedure requirements explicit, enabling:
- **Input validation**: Specify valid inputs without defensive checks in every procedure
- **Contract-based programming**: Caller and callee responsibilities are clear
- **Static verification**: Many preconditions can be verified at compile time
- **Documentation**: Self-documenting valid input ranges
- **Optimization**: Compiler can assume preconditions hold

**When to use `must`:**
- Input values must satisfy constraints (e.g., `x > 0`)
- Pointers/references must be non-null
- Array indices must be in bounds
- Division by zero must be prevented
- Resources must be in valid states
- Relationships between parameters must hold

**When NOT to use `must`:**
- Validating untrusted external input → use Result types
- Type constraints (use where clauses instead)
- Effect requirements (use uses clause instead)
- Postconditions about results (use ensures instead)

**Relationship to other features:**
- **Postcondition Clauses (§5):** Preconditions of callee become obligations of caller
- **Constraint Clauses (§6):** Where clauses are always-true; must are call-specific
- **Effect Clauses (§3):** Preconditions are orthogonal to effects
- **Procedures**: Preconditions are checked at procedure entry

### 4.2 Syntax

#### 4.2.1 Concrete Syntax

**Precondition clause:**
```ebnf
MustClause ::= "must" PredicateList

PredicateList  ::= Predicate ("," Predicate)*

Predicate      ::= Expression
```

**Examples:**
```cantrip
// Single precondition
procedure sqrt(x: f64): f64
    must x >= 0.0
{
    x.sqrt()
}

// Multiple preconditions
procedure divide(a: i32, b: i32): i32
    must b != 0, a >= i32::MIN, b >= i32::MIN
{
    a / b
}

// Complex preconditions
procedure binary_search<T: Ord>(arr: [T], target: T): Option<usize>
    must is_sorted(arr)
{
    // Binary search implementation
    ...
}

// Preconditions on relationships
procedure transfer(from: mut Account, to: mut Account, amount: i64)
    must {
        amount > 0,
        from.balance >= amount,
        from.id != to.id
    }
{
    from.balance -= amount
    to.balance += amount
}
```

#### 4.2.2 Abstract Syntax

**Precondition representation:**
```
procedure f(x₁: τ₁, ..., xₙ: τₙ): τ must P(x₁, ..., xₙ);

Where:
  P = predicate over parameters
  P : (τ₁, ..., τₙ) → bool
```

**Precondition semantics:**
```
{P} f(args) {Q}

Interpretation:
  If P holds before calling f, then Q holds after f returns
  If P doesn't hold, behavior is undefined or panics
```

**Multiple preconditions:**
```
must P₁, P₂, ..., Pₙ;

Equivalent to:
  must P₁ ∧ P₂ ∧ ... ∧ Pₙ;
```

#### 4.2.3 Basic Examples

**Example 1: Numeric constraints**
```cantrip
procedure factorial(n: i32): i32
    must n >= 0, n <= 12  // Prevent overflow
{
    if n <= 1 {
        1
    } else {
        n * factorial(n - 1)
    }
}
```

**Example 2: Array bounds**
```cantrip
procedure get<T>(arr: [T], index: usize): T
    must index < arr.len()
    must T: Copy
{
    arr[index]  // Safe: precondition ensures bounds
}
```

**Example 3: Non-null precondition**
```cantrip
procedure process_data(data: *Data)
    must data != null  // Raw pointer must not be null
    uses unsafe.ptr
{
    unsafe {
        (*data).process()
    }
}
```

### 4.3 Static Semantics

#### 4.3.1 Well-Formedness Rules

**[WF-Requires] Well-formed precondition:**
```
[WF-Requires]
Γ, params ⊢ P : bool
P references only parameters and pure functions
─────────────────────────────────────────────────
must P well-formed
```

**Explanation:** Preconditions must be boolean expressions over parameters.

**[WF-RequiresProc] Well-formed procedure with precondition:**
```
[WF-RequiresProc]
procedure f(params): τ { body } well-formed
must P well-formed
P references only params
─────────────────────────────────────────────────
procedure f(params): τ must P { body } well-formed
```

**Explanation:** Preconditions can only reference procedure parameters and pure helper functions.

**[WF-RequiresPure] Preconditions must be pure:**
```
[WF-RequiresPure]
must P;
effects(P) = ∅
─────────────────────────────────────────────────
must P well-formed
```

**Explanation:** Preconditions cannot have side effects.

#### 4.3.2 Type Rules

**[T-PrecondCall] Procedure call with precondition:**
```
[T-PrecondCall]
Γ ⊢ f : map(τ₁, ..., τₙ) -> τ must P
Γ ⊢ args : τ₁, ..., τₙ
Γ ⊢ P[params ↦ args] : bool
─────────────────────────────────────────────────
Γ ⊢ f(args) : τ
obligation: P[params ↦ args] must hold
```

**Explanation:** Calling a procedure with precondition P obligates the caller to ensure P holds for the arguments.

**[T-PrecondVerify] Static precondition verification:**
```
[T-PrecondVerify]
procedure f must P
caller context proves P[params ↦ args]
─────────────────────────────────────────────────
precondition statically verified (no runtime check)
```

**Explanation:** If the compiler can prove the precondition holds, no runtime check is needed.

**[T-PrecondCheck] Dynamic precondition checking:**
```
[T-PrecondCheck]
procedure f must P
compiler cannot prove P[params ↦ args]
─────────────────────────────────────────────────
runtime check inserted: assert!(P[params ↦ args])
```

**Explanation:** If the precondition cannot be proven statically, a runtime check is inserted.

**[T-PrecondStrengthen] Precondition strengthening:**
```
[T-PrecondStrengthen]
contract T { procedure m must P₁; }
record Type: T { procedure m must P₂ { ... } }
P₂ ⇒ P₁  (P₂ stronger than P₁)
─────────────────────────────────────────────────
ERROR: Cannot strengthen preconditions
```

**Explanation:** Implementations cannot have stronger preconditions than their trait declarations (Liskov substitution principle).

#### 4.3.3 Type Properties

**Theorem 3.1 (Precondition Soundness):**

If a procedure's precondition holds at call time, the procedure executes safely:

```
Γ ⊢ f(args) must P
P[params ↦ args] = true
─────────────────────────────────────────────────
f(args) executes without precondition violation
```

**Proof sketch:** By precondition checking. Either statically verified or dynamically checked at runtime.

**Theorem 3.2 (Precondition Obligation Transfer):**

Calling a procedure with precondition transfers the obligation to the caller:

```
procedure f must P { body }
procedure g { ... f(args) ... }
─────────────────────────────────────────────────
g must ensure P[params ↦ args] before calling f
```

**Proof sketch:** By [T-PrecondCall]. The type system enforces that callers satisfy callee preconditions.

**Theorem 3.3 (Static Elimination):**

Provably satisfied preconditions have zero runtime cost:

```
procedure f(x: i32) must x > 0 { ... }
procedure g() { f(5); }  // x > 0 provable
─────────────────────────────────────────────────
No runtime check needed in g
```

**Proof sketch:** Compiler can prove 5 > 0 statically, eliminating the check.

**Property 3.1 (Precondition Weakening):**

Callees can weaken (relax) preconditions in compatible ways:

```
contract T { procedure m(x: i32) must x > 0; }

record Type: T { procedure m(x: i32) must x >= 0; }  // Weaker - OK
```

**Property 3.2 (Precondition Independence):**

Preconditions are independent of effects and postconditions:

```cantrip
procedure f(x: i32)
    uses io.write
    must x > 0
    ensures result > x
{
    ...
}
```

Each clause serves a distinct purpose.

### 4.4 Dynamic Semantics

#### 4.4.1 Evaluation Rules

**[E-PrecondSatisfied] Precondition satisfied:**
```
[E-PrecondSatisfied]
procedure f(params) must P { body }
⟨P[params ↦ args], σ⟩ ⇓ ⟨true, σ⟩
⟨body[params ↦ args], σ⟩ ⇓ ⟨v, σ'⟩
─────────────────────────────────────────────────
⟨f(args), σ⟩ ⇓ ⟨v, σ'⟩
```

**Explanation:** If precondition holds, proceed with body execution.

**[E-PrecondViolated] Precondition violated:**
```
[E-PrecondViolated]
procedure f(params) must P { body }
⟨P[params ↦ args], σ⟩ ⇓ ⟨false, σ⟩
─────────────────────────────────────────────────
⟨f(args), σ⟩ ⇓ panic("precondition violated: P")
```

**Explanation:** If precondition doesn't hold, execution panics.

**[E-PrecondStatic] Statically verified precondition:**
```
[E-PrecondStatic]
procedure f(params) must P { body }
compiler proves P[params ↦ args]
⟨body[params ↦ args], σ⟩ ⇓ ⟨v, σ'⟩
─────────────────────────────────────────────────
⟨f(args), σ⟩ ⇓ ⟨v, σ'⟩  (no runtime check)
```

**Explanation:** Statically proven preconditions skip runtime checking.

#### 4.4.2 Checking Strategies

**Static verification (compile-time):**

The compiler attempts to prove preconditions using:
- Constant propagation
- Range analysis
- Flow-sensitive typing
- SMT solvers (optionally)

```cantrip
procedure needs_positive(x: i32) must x > 0 { ... }

procedure caller() {
    needs_positive(5)   // Proven: 5 > 0 ✓
    let y = 10
    needs_positive(y)   // Proven: 10 > 0 ✓
}
```

**Dynamic checking (runtime):**

When static proof fails, insert runtime checks:

```cantrip
procedure sqrt(x: f64) must x >= 0.0 { ... }

procedure compute(input: f64) {
    sqrt(input)  // Runtime check: assert!(input >= 0.0)
}

// Compiles to:
procedure compute(input: f64) {
    if !(input >= 0.0) {
        panic!("precondition violated: x >= 0.0")
    }
    sqrt(input)
}
```

**Optimization levels:**

```
--checks=none     : Skip all dynamic checks (unsafe, for production after testing)
--checks=debug    : Check in debug builds only
--checks=always   : Always check (default)
--checks=verify   : Use formal verification (must proof annotations)
```

#### 4.4.3 Error Messages

**Precondition violation at runtime:**
```
thread 'main' panicked at 'precondition violated: denominator != 0'
  must b != 0;
  ^^^^^^^^^^^^^^^
note: in procedure `divide` at src/math.ct:45:5
note: called from `calculate` at src/main.ct:12:9
```

**Static verification failure (warning):**
```
warning: cannot verify precondition statically
  --> src/main.ct:15:5
   |
15 |     divide(x, y)
   |     ^^^^^^^^^^^^ precondition `y != 0` cannot be proven
   |
   = note: runtime check will be inserted
   = help: consider adding: must y != 0;
```

### 4.5 Examples and Patterns

#### 4.5.1 Array Access

```cantrip
procedure safe_get<T>(arr: [T], index: usize): T
    must index < arr.len()
    must T: Copy
{
    arr[index]
}

procedure process_array(data: [i32]) {
    for i in 0..data.len() {
        let item = safe_get(data, i)  // Proven: i < data.len()
        println("{}", item)
    }
}
```

#### 4.5.2 Division Safety

```cantrip
procedure safe_divide(a: i32, b: i32): i32
    must b != 0
{
    a / b
}

procedure calculate(x: i32, y: i32): Option<i32> {
    if y != 0 {
        Some(safe_divide(x, y))  // Proven in this branch
    } else {
        None
    }
}
```

#### 4.5.3 Resource State

```cantrip
record File {
    handle: FileHandle
    is_open: bool

    procedure read($): Result<String, Error>
        must $.is_open
        uses io.read, alloc.heap
    {
        // Safe: file is guaranteed open
        $.handle.read_to_string()
    }

    procedure write($, data: String): Result<(), Error>
        must $.is_open
        uses io.write
    {
        $.handle.write_all(data.as_bytes())
    }
}
```

#### 4.5.4 Sorted Data Structures

```cantrip
record SortedVec<T: Ord> {
    elements: Vec<T>

    where {
        is_sorted(elements)
    }

    procedure binary_search($, target: T): Result<usize, usize>
        must is_sorted($.elements)  // Redundant with where, but explicit
    {
        $.elements.binary_search(target)
    }
}
```

#### 4.5.5 Temporal Preconditions

```cantrip
record Transaction {
    started: Instant
    completed: Option<Instant>

    procedure commit(mut $)
        must $.completed.is_none()
    {
        $.completed = Some(Instant::now())
    }

    procedure rollback(mut $)
        must $.completed.is_none()
    {
        // Can only rollback active transactions
        ...
    }
}
```

### 4.6 Advanced Topics

#### 4.6.1 Precondition Inference

The compiler can infer preconditions from procedure bodies:

```cantrip
procedure inferred_precond(x: i32, y: i32): i32 {
    x / y  // Compiler infers: must y != 0
}

// Warning: Missing explicit precondition
// help: add: must y != 0;
```

#### 4.6.2 Preconditions with Quantifiers

```cantrip
procedure all_positive(numbers: [i32]): bool
    must numbers.len() > 0
{
    numbers.iter().all(|n| n > 0)
}

procedure process_positive_list(nums: [i32])
    must {
        nums.len() > 0,
        forall(|i| i < nums.len() => nums[i] > 0)
    }
{
    // All numbers guaranteed positive
    ...
}
```

#### 4.6.3 Frame Conditions

Preconditions can specify framing constraints:

```cantrip
procedure update_field(obj: mut Record, value: i32)
    must obj.state == State::Active
{
    obj.value = value
    // obj.state remains Active (frame condition)
}
```

#### 4.6.4 Preconditions in Generic Code

```cantrip
procedure binary_search_generic<T, F>(arr: [T], predicate: F): Option<usize>
    where F: Fn(T) -> bool
    must {
        arr.len() > 0,
        is_partitioned(arr, predicate)  // arr partitioned by predicate
    }
{
    // Binary search implementation
    ...
}
```

---


**Definition 5.1 (Postcondition Clause):** A postcondition clause (`will`) is a predicate that must be true when a procedure returns. Postconditions specify the procedure's guarantees and define what callers can rely upon after the call completes.

## 5. Postcondition Clauses

### 5.1 Overview

**Key innovation/purpose:** Postconditions make procedure guarantees explicit, enabling:
- **Output validation**: Specify properties of return values and mutated parameters
- **Contract-based programming**: Procedure guarantees are explicit
- **Static verification**: Many postconditions can be verified at compile time
- **Documentation**: Self-documenting behavior and results
- **Optimization**: Compiler can use postconditions for optimization

**When to use `will`:**
- Return values must satisfy properties (e.g., `result > 0`)
- Mutated parameters must maintain invariants
- Relationships between inputs and outputs
- State transitions are valid
- Collections maintain properties (sorted, non-empty, etc.)
- Successful operation guarantees

**When NOT to use `will`:**
- Requirements on inputs (use must instead)
- Effect declarations (use uses clause instead)
- Type invariants (use where clauses instead)
- Error cases (use Result types)

**Relationship to other features:**
- **Precondition Clauses (§4):** Ensures specifies outputs; must specifies inputs
- **Constraint Clauses (§6):** Where clauses are always-true; will apply to specific calls
- **Effect Clauses (§3):** Postconditions are orthogonal to effects
- **Procedures**: Postconditions are checked at procedure exit

### 5.2 Syntax

#### 5.2.1 Concrete Syntax

**Postcondition clause:**
```ebnf
WillClause ::= "will" PredicateList

PredicateList ::= Predicate ("," Predicate)*

Predicate     ::= Expression

Expression    ::= "result" RelOp Expression
                | "@old" "(" Expression ")"
                | ParamRef RelOp Expression
                | BooleanExpr
```

**Special identifiers:**
- `result` — The return value of the procedure
- `@old(expr)` — The value of `expr` at procedure entry

**Examples:**
```cantrip
// Return value constraint
procedure abs(x: i32): i32
    will result >= 0
{
    if x < 0 { -x } else { x }
}

// Relationship with input
procedure increment(x: i32): i32
    will result == x + 1
{
    x + 1
}

// Mutation guarantee
procedure push<T>(vec: mut Vec<T>, item: T)
    will vec.len() == @old(vec.len()) + 1
{
    vec.push(item)
}

// Multiple postconditions
procedure clamp(value: i32, min: i32, max: i32): i32
    must min <= max
    will {
        result >= min,
        result <= max,
        (value >= min && value <= max) => result == value
    }
{
    if value < min {
        min
    } else if value > max {
        max
    } else {
        value
    }
}
```

#### 5.2.2 Abstract Syntax

**Postcondition representation:**
```
procedure f(x₁: τ₁, ..., xₙ: τₙ): τ will Q(x₁, ..., xₙ, result);

Where:
  Q = predicate over parameters and result
  Q : (τ₁, ..., τₙ, τ) → bool
```

**Hoare triple semantics:**
```
{P} f(args) {Q}

Where:
  P = precondition (requires)
  Q = postcondition (will)

If P holds before f, then Q holds after f
```

**@old operator:**
```
@old(expr) = value of expr at procedure entry

Semantics:
  @old(expr) is evaluated and saved before procedure body executes
```

**Multiple postconditions:**
```
will Q₁, Q₂, ..., Qₙ;

Equivalent to:
  will Q₁ ∧ Q₂ ∧ ... ∧ Qₙ;
```

#### 5.2.3 Basic Examples

**Example 1: Return value properties**
```cantrip
procedure square(x: i32): i32
    will result >= 0
{
    x * x
}
```

**Example 2: Relationship between input and output**
```cantrip
procedure double(x: i32): i32
    will result == 2 * x
{
    x + x
}
```

**Example 3: Mutation postcondition**
```cantrip
procedure increment(counter: mut i32)
    will counter == @old(counter) + 1
{
    counter += 1
}
```

### 5.3 Static Semantics

#### 5.3.1 Well-Formedness Rules

**[WF-Ensures] Well-formed postcondition:**
```
[WF-Ensures]
procedure f(params): τ { body }
Γ, params, result: τ ⊢ Q : bool
Q can reference params, result, @old(expr)
─────────────────────────────────────────────────
will Q well-formed
```

**Explanation:** Postconditions must be boolean expressions over parameters, result, and @old values.

**[WF-EnsuresProc] Well-formed procedure with postcondition:**
```
[WF-EnsuresProc]
procedure f(params): τ { body } well-formed
will Q well-formed
Q does not perform effects
─────────────────────────────────────────────────
procedure f(params): τ will Q { body } well-formed
```

**Explanation:** Postconditions must be pure and side-effect free.

**[WF-OldExpr] Well-formed @old expression:**
```
[WF-OldExpr]
Γ, params ⊢ expr : τ
expr references parameters or their fields
─────────────────────────────────────────────────
Γ ⊢ @old(expr) : τ
```

**Explanation:** @old can only reference expressions that are valid at procedure entry.

#### 5.3.2 Type Rules

**[T-EnsuresProc] Procedure with postcondition:**
```
[T-EnsuresProc]
Γ ⊢ procedure f(params): τ { body }
Γ, params ⊢ body : τ
Γ, params, result: τ ⊢ Q : bool
body guarantees Q
─────────────────────────────────────────────────
Γ ⊢ f : map(params) -> τ will Q
```

**Explanation:** Procedure body must guarantee the postcondition holds for all return paths.

**[T-EnsuresCall] Call site relies on postcondition:**
```
[T-EnsuresCall]
Γ ⊢ f : map(τ₁, ..., τₙ) -> τ will Q
Γ ⊢ args : τ₁, ..., τₙ
let result = f(args)
─────────────────────────────────────────────────
Γ ⊢ Q[params ↦ args, result ↦ result] : bool (can be assumed)
```

**Explanation:** After calling f, the caller can assume the postcondition holds.

**[T-EnsuresVerify] Static postcondition verification:**
```
[T-EnsuresVerify]
procedure f will Q { body }
compiler proves body ⇒ Q for all return paths
─────────────────────────────────────────────────
postcondition statically verified (no runtime check)
```

**Explanation:** If the compiler can prove the postcondition always holds, no runtime check is needed.

**[T-EnsuresWeaken] Postcondition weakening:**
```
[T-EnsuresWeaken]
contract T { procedure m will Q₁; }
record Type: T { procedure m will Q₂ { ... } }
Q₂ ⇒ Q₁  (Q₂ stronger than Q₁)
─────────────────────────────────────────────────
OK: Can strengthen postconditions
```

**Explanation:** Implementations can provide stronger guarantees (Liskov substitution).

#### 5.3.3 Type Properties

**Theorem 4.1 (Postcondition Soundness):**

If a procedure will Q and executes successfully, Q holds at return:

```
Γ ⊢ f : map(params) -> τ will Q
⟨f(args), σ⟩ ⇓ ⟨result, σ'⟩
─────────────────────────────────────────────────
Q[params ↦ args, result ↦ result] holds in σ'
```

**Proof sketch:** By postcondition checking. Either statically verified or dynamically checked.

**Theorem 4.2 (Postcondition Guarantee Transfer):**

Calling a procedure with postcondition allows caller to assume the guarantee:

```
procedure f will Q { body }
procedure g { let x = f(args); ... }
─────────────────────────────────────────────────
After f(args), g can assume Q[result ↦ x]
```

**Proof sketch:** By [T-EnsuresCall]. Type system allows callers to rely on callee postconditions.

**Theorem 4.3 (Static Elimination):**

Provably satisfied postconditions have zero runtime cost:

```
procedure f(x: i32): i32 will result > 0 {
    x.abs() + 1  // Always > 0
}
─────────────────────────────────────────────────
No runtime check needed (proven by compiler)
```

**Proof sketch:** Compiler analyzes body and proves postcondition always holds.

**Property 4.1 (Postcondition Strengthening):**

Callees can strengthen (provide better) postconditions:

```
contract T { procedure m(): i32 will result > 0; }

record Type: T { procedure m(): i32 will result > 10; }  // Stronger - OK
```

**Property 4.2 (@old Capture):**

@old expressions are evaluated at procedure entry:

```
procedure f(x: mut i32) will x == @old(x) + 1 {
    x += 1;  // @old(x) captured before this
}
```

### 5.4 Dynamic Semantics

#### 5.4.1 Evaluation Rules

**[E-EnsuresSatisfied] Postcondition satisfied:**
```
[E-EnsuresSatisfied]
procedure f(params): τ will Q { body }
⟨body[params ↦ args], σ⟩ ⇓ ⟨v, σ'⟩
⟨Q[params ↦ args, result ↦ v], σ'⟩ ⇓ ⟨true, σ'⟩
─────────────────────────────────────────────────
⟨f(args), σ⟩ ⇓ ⟨v, σ'⟩
```

**Explanation:** If body produces result and postcondition holds, return result.

**[E-EnsuresViolated] Postcondition violated:**
```
[E-EnsuresViolated]
procedure f(params): τ will Q { body }
⟨body[params ↦ args], σ⟩ ⇓ ⟨v, σ'⟩
⟨Q[params ↦ args, result ↦ v], σ'⟩ ⇓ ⟨false, σ'⟩
─────────────────────────────────────────────────
⟨f(args), σ⟩ ⇓ panic("postcondition violated: Q")
```

**Explanation:** If postcondition doesn't hold at return, panic.

**[E-OldCapture] @old expression capture:**
```
[E-OldCapture]
procedure f(...) will ... @old(expr) ... { body }
⟨expr, σ_entry⟩ ⇓ ⟨v_old, σ_entry⟩
⟨body, σ_entry⟩ ⇓ ⟨v_result, σ_exit⟩
─────────────────────────────────────────────────
@old(expr) in postcondition evaluates to v_old
```

**Explanation:** @old expressions are captured at procedure entry, before body executes.

#### 5.4.2 Checking Strategies

**Static verification (compile-time):**

The compiler attempts to prove postconditions using:
- Symbolic execution
- Path-sensitive analysis
- Dataflow analysis
- SMT solvers (optionally)

```cantrip
procedure abs(x: i32): i32
    will result >= 0
{
    if x < 0 { -x } else { x }
}

// Compiler proves:
// Path 1: x < 0 → result = -x → -x >= 0 ✓ (when x < 0)
// Path 2: x >= 0 → result = x → x >= 0 ✓
```

**Dynamic checking (runtime):**

When static proof fails, insert runtime checks:

```cantrip
procedure complex_calc(x: i32): i32
    will result > 0
{
    // Complex calculation
    let r = compute(x)
    r  // Runtime check: assert!(r > 0)
}

// Compiles to:
procedure complex_calc(x: i32): i32 {
    let r = compute(x)
    if !(r > 0) {
        panic!("postcondition violated: result > 0")
    }
    r
}
```

**Multiple return points:**

Postconditions are checked at all return points:

```cantrip
procedure find_positive(numbers: [i32]): Option<i32>
    will match result {
        Some(n) => n > 0,
        None => true
    }
{
    for n in numbers {
        if n > 0 {
            return Some(n)  // Check here
        }
    }
    None  // Check here too
}
```

#### 5.4.3 @old Implementation

**Capture mechanism:**

@old expressions are evaluated and stored before body execution:

```cantrip
procedure increment(counter: mut i32)
    will counter == @old(counter) + 1
{
    counter += 1
}

// Compiles to:
procedure increment(counter: mut i32) {
    let __old_counter = counter  // Capture @old value
    counter += 1
    if !(counter == __old_counter + 1) {  // Check using captured value
        panic!("postcondition violated")
    }
}
```

**Multiple @old references:**

Each @old expression is captured once:

```cantrip
procedure swap(a: mut i32, b: mut i32)
    will a == @old(b), b == @old(a)
{
    let temp = a
    a = b
    b = temp
}

// Captures:
// let __old_a = a
// let __old_b = b
// Then checks: a == __old_b && b == __old_a
```

### 5.5 Examples and Patterns

#### 5.5.1 Sorted Insertion

```cantrip
record SortedVec<T> where T: Ord {
    elements: Vec<T>

    procedure insert(mut $, item: T)
        will {
            $.len() == @old($.len()) + 1,
            is_sorted($.elements),
            $.contains(item)
        }
    {
        let pos = $.elements.binary_search(item).unwrap_or_else(|e| e)
        $.elements.insert(pos, item)
    }
}
```

#### 5.5.2 Bank Account Transfer

```cantrip
procedure transfer(from: mut Account, to: mut Account, amount: i64)
    must {
        amount > 0,
        from.balance >= amount,
        from.id != to.id
    }
    will {
        from.balance == @old(from.balance) - amount,
        to.balance == @old(to.balance) + amount
    }
{
    from.balance -= amount
    to.balance += amount
}
```

#### 5.5.3 Cache Guarantees

```cantrip
record Cache<K, V> where V: Copy {
    data: HashMap<K, V>

    procedure get_or_insert(mut $, key: K, value: V): V
        will {
            $.contains_key(key),
            result == $.get(key).unwrap()
        }
    {
        if !$.contains_key(key) {
            $.insert(key, value)
        }
        $.get(key).unwrap()
    }
}
```

#### 5.5.4 Search Guarantees

```cantrip
procedure binary_search<T: Ord>(arr: [T], target: T): Result<usize, usize>
    must is_sorted(arr)
    will match result {
        Ok(index) => {
            index < arr.len(),
            arr[index] == target
        },
        Err(index) => {
            index <= arr.len(),
            forall(|i| i < arr.len() => arr[i] != target)
        }
    }
{
    arr.binary_search(target)
}
```

#### 5.5.5 State Machine Transitions

```cantrip
record Connection {
    state: State

    procedure connect(mut $)
        must $.state == State::Disconnected
        will $.state == State::Connected
        uses net.tcp
    {
        // Perform connection
        $.state = State::Connected
    }

    procedure disconnect(mut $)
        must $.state == State::Connected
        will $.state == State::Disconnected
    {
        // Perform disconnection
        $.state = State::Disconnected
    }
}
```

### 5.6 Advanced Topics

#### 5.6.1 Quantified Postconditions

```cantrip
procedure map_all<T, U>(items: Vec<T>, f: map(T) -> U): Vec<U>
    will {
        result.len() == items.len(),
        forall(|i| i < result.len() =>
            result[i] == f(items[i])
        )
    }
{
    items.into_iter().map(f).collect()
}
```

#### 5.6.2 Frame Conditions

Postconditions can specify what didn't change:

```cantrip
procedure update_name(person: mut Person, name: String)
    will {
        person.name == name,
        person.age == @old(person.age),  // Frame: age unchanged
        person.id == @old(person.id)    // Frame: id unchanged
    }
{
    person.name = name
}
```

#### 5.6.3 Postconditions with Result Types

```cantrip
procedure parse_int(s: String): Result<i32, ParseError>
    will match result {
        Ok(n) => {
            n.to_string() == s.trim()
        },
        Err(e) => {
            !is_valid_int(s)
        }
    }
{
    s.trim().parse()
}
```

---
