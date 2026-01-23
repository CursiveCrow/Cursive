# Cursive Language Specification

## Clause 12 — Witness System

**Clause**: 12 — Witness System
**File**: 13-2_Witness-Kinds.md
**Section**: §12.2 Witness Kinds
**Stable label**: [witness.kind]  
**Forward references**: §13.3 [witness.formation], §13.4 [witness.representation], §13.5 [witness.dispatch], Clause 10 §10.4 [generic.behavior], Clause 12 [contract], §7.6 [type.modal]

---

### §12.2 Witness Kinds [witness.kind]

#### §12.2.1 Overview

[1] Witnesses provide runtime evidence for three categories of type-level properties: behavior implementations, contract satisfactions, and modal state memberships. While all witness kinds use the same dense pointer representation and share common mechanisms, they differ in what property is being witnessed and how dispatch semantics operate.

[2] This subclause classifies witness kinds, specifies their semantic differences, and provides examples distinguishing their use cases. The classification is semantic (for documentation and understanding); all witnesses use the unified `witness<Property>` type system.

#### §12.2.2 Behavior Witnesses [witness.kind.behavior]

##### §12.2.2.1 Definition

[3] A _behavior witness_ provides runtime evidence that a value implements a behavior (§10.4). The witness type `witness<B>` where `B` is a behavior identifier packages a value with its behavior implementation vtable.

##### §12.2.2.2 Purpose

[4] Behavior witnesses enable:

- **Heterogeneous collections**: Arrays/lists containing different types sharing a behavior
- **Plugin systems**: Loading types at runtime that implement known behaviors
- **Dynamic dispatch**: Calling behavior procedures without knowing concrete type
- **Type erasure**: Hiding implementation details behind behavior interface

##### §12.2.2.3 Example

**Example 13.2.2.1 (Behavior witness)**:

```cursive
behavior Display {
    procedure show(~): string@View
    { result "Display" }
}

record Point with Display {
    x: f64,
    y: f64,

    procedure show(~): string@View
    { result "Point" }
}

record Circle with Display {
    radius: f64,

    procedure show(~): string@View
    { result "Circle" }
}

// Create behavior witnesses
let point = Point { x: 1.0, y: 2.0 }
let circle = Circle { radius: 5.0 }

let w1: witness<Display> = point    // Witness of Point implementing Display
let w2: witness<Display> = circle   // Witness of Circle implementing Display

// Heterogeneous collection
let shapes: [witness<Display>] = [w1, w2]

// Dynamic dispatch
loop shape: witness<Display> in shapes {
    println("{}", shape::show())  // Runtime dispatch to concrete implementation
}
```

#### §12.2.3 Contract Witnesses [witness.kind.contract]

##### §12.2.3.1 Definition

[5] A _contract witness_ provides runtime evidence that a value satisfies a contract (Clause 12). The witness type `witness<C>` where `C` is a contract identifier packages a value with its contract implementation vtable.

##### §12.2.3.2 Distinction from Behavior Witnesses

[6] Contract witnesses and behavior witnesses use identical runtime representation and dispatch mechanisms. The distinction is semantic only:

- **Behaviors** provide concrete default implementations (§10.4)
- **Contracts** specify abstract requirements (Clause 12)

[7] At runtime, both compile to vtables. A witness for a contract points to the type's implementation of the contract's required procedures.

##### §12.2.3.3 Example

**Example 13.2.3.1 (Contract witness)**:

```cursive
public contract Serializable {
    procedure serialize(~): string@Managed
        [[ alloc::heap |- true => result.len() > 0 ]]

    procedure deserialize(data: string@Managed): Self \/ ParseError
        [[ alloc::heap |- data.len() > 0 => true ]]
}

record User: Serializable {
    id: u64,
    name: string@Managed,

    procedure serialize(~): string@Managed
        [[ alloc::heap |- true => result.len() > 0 ]]
    { }

    procedure deserialize(data: string@Managed): Self \/ ParseError
        [[ alloc::heap |- data.len() > 0 => true ]]
    { }
}

// Create contract witness
let user = User { id: 1, name: string.from("Alice") }
let w: witness<Serializable> = user

// Dynamic dispatch to contract methods
let serialized = w::serialize()
```

#### §12.2.4 Modal State Witnesses [witness.kind.modal]

##### §12.2.4.1 Definition

[8] A _modal state witness_ provides runtime evidence that a modal value is in a specific state. The witness type `witness<T@State>` where `T` is a modal type and `@State` is a state identifier packages a value with runtime state verification.

##### §12.2.4.2 Purpose

[9] Modal state witnesses enable dynamic state tracking when compile-time state tracking is insufficient:

- **Deserialization**: Loading modal values from external sources with unknown states
- **Network protocols**: Receiving stateful values over network
- **Dynamic state machines**: State determined by runtime conditions
- **Plugin boundaries**: External code providing values in specific states

[10] **Static vs dynamic modal verification**:

**Static** (default, zero-cost):

```cursive
let file: FileHandle@Open = get_file()
// Type system proves file is @Open
// Zero runtime overhead
```

**Dynamic** (opt-in, runtime checking):

```cursive
let file_witness: witness<FileHandle@Open> = file
// Runtime state tag verifies file is @Open
// State checked dynamically
```

##### §12.2.4.3 Example

**Example 13.2.4.1 (Modal state witness)**:

```cursive
modal Connection {
    @Disconnected
    @Connected { socket: Socket }
}

// Static mode: Type system tracks state
let conn: Connection@Connected = establish_connection()
conn::send(data)  // Type system verifies @Connected

// Dynamic mode: Runtime state checking
procedure handle_connection(conn: witness<Connection@Connected>)
    [[ net::send |- true => true ]]
{
    conn::send(data)  // Runtime verifies state is @Connected before send
}

// Use case: State determined at runtime
let conn: Connection = load_from_config()  // State unknown at compile time

match conn {
    @Connected => {
        let w: witness<Connection@Connected> = conn
        handle_connection(w)
    }
    @Disconnected => {
        println("Not connected")
    }
}
```

#### §12.2.5 Witness Kind Classification [witness.kind.classification]

[11] Table 13.2 summarizes witness kind properties:

**Table 13.2 — Witness kind properties**

| Witness Kind | Syntax                     | What It Proves                  | VTable Contains           | State Tag |
| ------------ | -------------------------- | ------------------------------- | ------------------------- | --------- |
| Behavior     | `witness<Display>`         | Implements behavior Display     | Behavior procedures       | None      |
| Contract     | `witness<Serializable>`    | Satisfies contract Serializable | Contract procedures       | None      |
| Modal State  | `witness<FileHandle@Open>` | Is in state @Open               | State-specific procedures | @Open     |

[12] Despite these semantic differences, all use the unified witness type system with the same representation and construction mechanisms.

#### §12.2.6 When to Use Each Kind [witness.kind.usage]

**Behavior witnesses**:
[13] Use when you need heterogeneous collections of types sharing a behavior or when concrete types are determined at runtime (plugins, dynamic loading).

**Contract witnesses**:
[14] Use when you need dynamic polymorphism over contract-satisfying types, particularly for plugin systems with abstract interfaces defined by contracts.

**Modal state witnesses**:
[15] Use when modal states are determined at runtime (deserialization, network protocols, configuration-driven state machines) rather than tracked statically by the type system.

**Static verification preferred**:
[16] When types and states are known at compile time, use generic bounds (`<T: B>`) and type annotations (`T@State`) instead of witnesses. Static verification is zero-cost and should be the default.

#### §12.2.7 Conformance Requirements [witness.kind.requirements]

[17] Implementations shall:

1. Support all three witness kinds with unified `witness<Property>` syntax
2. Use identical dense pointer representation for all witness kinds
3. Compile behavior implementations to vtable entries for behavior witnesses
4. Compile contract implementations to vtable entries for contract witnesses
5. Track modal state tags for modal state witnesses
6. Distinguish witness kinds semantically while maintaining unified implementation
7. Maintain type safety for all witness operations regardless of kind
8. Document when each witness kind is appropriate vs static verification

---

**Previous**: §13.1 Overview and Purpose (§13.1 [witness.overview]) | **Next**: §13.3 Formation and Construction (§13.3 [witness.formation])
