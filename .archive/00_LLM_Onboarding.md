# Cursive — Complete LLM Onboarding Guide

**Part:** 0 - Comprehensive Quick Start for AI-Assisted Development  
**File:** 00_LLM_Onboarding.md  
**Version:** 2.0 (Enhanced)  
**Last Updated:** 2025-11-02  
**Purpose:** Deep alignment of LLM understanding with Cursive language design

---

## Document Structure & Reading Guide

**For LLMs generating code:**

- Read sections 0.0-0.5 for foundational understanding
- Reference sections 0.6-0.15 as needed during code generation
- Keep sections 0.16-0.20 in active context for validation

**For LLMs learning Cursive:**

- Read entire document linearly
- Study all examples in sections 0.10-0.12
- Memorize decision trees in section 0.13
- Practice with patterns in section 0.19

**Document Navigation:**

- § 0.0-0.2: Philosophy and mental models
- § 0.3-0.5: Core syntax and semantics
- § 0.6-0.9: Type system and memory model
- § 0.10-0.12: Practical patterns and examples
- § 0.13-0.15: Decision trees and troubleshooting
- § 0.16-0.20: Advanced topics and validation

---

## Specification Structure

This specification is organized into the following parts:

| Part | File                                     | Chapter Title               | Status   |
| ---- | ---------------------------------------- | --------------------------- | -------- |
| 0    | 00_LLM_Onboarding.md                     | LLM Onboarding Guide        | Complete |
| I    | 01_Foundations.md                        | Foundations                 | Complete |
| II   | 02_Type-System.md                        | Type System                 | Complete |
| III  | 03_Declarations-and-Scope.md             | Declarations and Scope      | Complete |
| IV   | 04_Lexical-Permissions.md                | Lexical Permission System   | Complete |
| V    | 05_Expressions-and-Operators.md          | Expressions and Operators   | Complete |
| VI   | 06_Statements-and-Control-Flow.md        | Statements and Control Flow | Complete |
| VII  | 07_Contracts-and-Effects.md              | Contracts and Effects       | Complete |
| VIII | 08_Holes-and-Inference.md                | Holes and Inference         | Complete |
| IX   | 09_Functions.md                          | Functions and Procedures    | Complete |
| X    | 10_Modals.md                             | Modal Types                 | Complete |
| XI   | 11_Metaprogramming.md                    | Metaprogramming             | Complete |
| XII  | 12_Modules-Packages-and-Imports.md       | Modules                     | Deferred |
| XIII | 13_Memory-Permissions-and-Concurrancy.md | Memory Model                | Deferred |
| XIV  | 14_Errors-and-Exceptions.md              | Errors                      | Deferred |
| XV   | 15_Unsafe-Behaviors-and-FFI.md           | Unsafe & FFI                | Deferred |

**Cross-Reference Format:** `CITE: Part X §Y.Z — Title`  
**Implementation Scope:** Parts 0-XI are complete and normative  
**Future Work:** Parts XII-XV planned for v1.1

---

## 0.0 Purpose, Philosophy & Design Goals

### 0.0.1 What Cursive Is

**Cursive is a systems programming language optimized for AI-assisted development.**

**Primary Goals:**

1. **Memory safety** without garbage collection or complex borrow checking
2. **Deterministic performance** through explicit resource management
3. **Local reasoning** enabling understanding from any single point in code
4. **LLM-friendly syntax** with predictable patterns and required explicitness
5. **Zero-cost abstractions** with compile-time safety guarantees

**Target Use Cases:**

- Systems programming (OS kernels, device drivers)
- Performance-critical applications
- Real-time systems
- Embedded development
- Network services and infrastructure
- AI-generated production code

### 0.0.2 Core Design Principles (Memorize These)

**Principle 1: Explicit Over Implicit**

Every significant choice must be spelled out in code:

- Block results require explicit `result` keyword
- Pipeline stages require explicit type annotations
- Match bindings require explicit types
- Loop iterators require explicit types
- Grants (capabilities) require explicit sequent declarations

**Why:** Eliminates ambiguity for both humans and LLMs. No "magic" behavior to discover.

**Principle 2: Local Reasoning**

You can understand code by reading only what's visible:

- Permission requirements visible in procedure signatures
- Effect capabilities declared at the procedure boundary
- State transitions explicit in modal type usage
- Resource lifetimes tied to lexical scopes (regions)

**Why:** LLMs don't need global program analysis to generate correct code.

**Principle 3: Deterministic Evaluation**

Evaluation order is always predictable:

- **Strict left-to-right** for all expressions
- **Call-by-value** parameter passing
- **No lazy evaluation** except `&&` and `||` short-circuit

**Why:** Makes refactoring safe and enables precise effect reasoning.

**Principle 4: Permissions, Not Borrows**

Memory safety through permission wrappers, not borrow analysis:

- `own T` — Exclusive ownership (can move)
- `mut T` — Mutable access (can modify)
- `imm T` — Immutable access (read-only, default)

**Why:** Simpler mental model, no lifetime parameters, clearer intent.

**Principle 5: Regions, Not Lifetimes**

Resource lifetime management through lexical scopes:

- `region r { ... }` creates allocation scope
- All region allocations freed at scope exit (O(1))
- No lifetime annotations in types (`'a` not needed)

**Why:** Lifetime is visible in code structure, not hidden in type annotations.

**Principle 6: Contracts & Grants**

Every procedure declares its behavioral contract using sequent calculus:

- `[[ ε |- P => Q ]]` — Contractual sequent using semantic brackets
  - `ε` — Grant context (required capabilities)
  - `|-` — Turnstile (entailment), ASCII for `⊢`
  - `P` — Precondition (caller obligations)
  - `=>` — Implication, ASCII for `⇒`
  - `Q` — Postcondition (callee guarantees)

**Why:** Makes capability requirements and behavioral contracts explicit, checkable, and grounded in formal logic.

### 0.0.3 What Makes Cursive Different

**vs. Rust:**

- ✅ No lifetime parameters (`'a` annotations)
- ✅ No exclusive mutable borrowing rules
- ✅ Simpler ownership model (permissions are explicit wrappers)
- ✅ Region-based memory (not borrow checker)
- ❌ More verbose (required type annotations)
- ❌ Multiple mutable aliases allowed (you manage safety)

**vs. C/C++:**

- ✅ Memory safety guaranteed at compile time
- ✅ No null pointer dereferences (Option type)
- ✅ No use-after-free (region escape analysis)
- ✅ Effect system tracks side effects
- ❌ Stricter syntax requirements
- ❌ Requires explicit permission annotations

**vs. Go:**

- ✅ No garbage collection (deterministic performance)
- ✅ Richer type system (modals, effects)
- ✅ Compile-time memory safety
- ✅ Zero-cost abstractions
- ❌ More complex (explicit permissions)
- ❌ Steeper learning curve

### 0.0.4 Mental Model: Think in Capabilities

**Cursive programs are about capability management:**

```
Function signature = What capabilities do I need?
    sequent clause = Contractual sequent [[ grants |- pre => post ]]
    grants = What capabilities can I use?
    |- (turnstile) = Logical entailment separator
    pre = What must be true when called?
    => (implication) = Logical implies
    post = What will be true when done?

Function body      = How do I use my capabilities?

Permission types   = What can I do with this value?
    own T          = I can move it, consume it
    mut T          = I can modify it
    imm T          = I can read it
```

**Key Insight:** In Cursive, types tell you **what you can do**, not just **what something is**.

---

## 0.1 Language Quick Rules (Critical Reference)

### 0.1.1 Required Explicitness (These WILL Error If Omitted)

**Rule 1: Block Expressions Must Use `result`**

```cursive
// ❌ WRONG: Implicit return
let x: i32 = {
    let a = 2
    a * 3  // This returns (), not 6
}

// ✅ CORRECT: Explicit result
let x: i32 = {
    let a = 2
    result a * 3  // Explicitly returns 6
}
```

**Rule 2: Match Bindings Must Annotate Type**

```cursive
// ❌ WRONG: Type inference not allowed
let msg = match result {
    Result::Ok(v) => v,
    Result::Err(e) => "error"
}

// ✅ CORRECT: Explicit type annotation
let msg: string = match result {
    Result::Ok(v) => v,
    Result::Err(e) => "error"
}
```

**Rule 3: Pipeline Stages Must Annotate Type**

```cursive
// ❌ WRONG: Stage types inferred
let output = input
    => parse
    => validate
    => serialize

// ✅ CORRECT: Each stage type explicit
let output: string = input
    => parse: Data
    => validate: Result<Data, Error>
    => serialize: string
```

**Rule 4: Loop Iterators Must Annotate Type**

```cursive
// ❌ WRONG: Iterator type inferred
loop item in items {
    process(item)
}

// ✅ CORRECT: Iterator type explicit
loop item: Item in items {
    process(item)
}
```

**Why These Rules Exist:**

1. **Eliminates type inference puzzles** - No ambiguity for LLMs or humans
2. **Makes intent explicit** - Reader knows exactly what type is expected
3. **Enables better error messages** - Compiler knows what you intended
4. **Prevents accidental type changes** - Refactoring won't silently change types

### 0.1.2 Deterministic Evaluation (Guaranteed Order)

**Left-to-Right Evaluation:**

```cursive
// Arguments evaluate left to right (guaranteed)
let result = f(first(), second(), third())
// Order: first(), then second(), then third(), then f()

// Tuple fields evaluate left to right (guaranteed)
let pair = (compute_x(), compute_y())
// Order: compute_x(), then compute_y()

// Record fields evaluate left to right (guaranteed)
let point = Point { x: calc_x(), y: calc_y() }
// Order: calc_x(), then calc_y()

// Binary operators evaluate left to right (guaranteed)
let sum = a() + b() + c()
// Order: a(), then b(), then +, then c(), then +
```

**Short-Circuit Exception (Only Two Operators):**

```cursive
// && may skip right side if left is false
if expensive_check() && another_check() {
    // another_check() NOT called if expensive_check() returns false
}

// || may skip right side if left is true
if cheap_check() || expensive_check() {
    // expensive_check() NOT called if cheap_check() returns true
}

// ALL OTHER operators evaluate both sides
let x = a() / b()  // Both a() and b() always evaluate
```

**Why This Matters:**

- Side effects happen in predictable order
- Refactoring is safe (order won't change)
- LLMs can reason about execution precisely
- No "undefined behavior" from evaluation order

### 0.1.3 Permission System (Access Control)

**Three Permission Levels:**

```cursive
// own T - Exclusive ownership
procedure consume(data: own Data) {
    // Can move data, pass to own parameters, call procedures consuming self
    process(move data)  // data now invalid
}

// mut T - Mutable access
procedure modify(data: mut Data) {
    data.field = new_value  // Can mutate fields
    data.method()           // Can call mut methods
}

// imm T - Immutable access (default)
procedure read(data: Data) {  // Implicitly imm Data
    let x = data.field      // Can read fields
    data.read_method()      // Can call imm methods
}
```

**Permission Rules:**

1. **Ownership move invalidates source:**

   ```cursive
   let data: own Data = Data::new()
   consume(move data)
   // data is now invalid, cannot use
   ```

2. **Multiple mutable aliases allowed:**

   ```cursive
   var x: i32 = 0
   increment(mut x)  // Passes mutable reference
   increment(mut x)  // Can pass again - no exclusive borrow
   ```

3. **You are responsible for aliasing safety:**

   ```cursive
   var arr: [i32; 10] = [0; 10]
   let p1: mut i32 = mut arr[0]
   let p2: mut i32 = mut arr[0]  // Two mutable aliases - YOU ensure safety
   ```

4. **Permission weakening only:**

   ```cursive
   let data: own Data = Data::new()
   read(data)          // OK: own -> imm (automatic weakening)
   modify(mut data)    // OK: own -> mut (automatic weakening)

   let readonly: imm Data = get_data()
   modify(mut readonly)  // ❌ ERROR: Cannot upgrade imm to mut
   ```

### 0.1.4 Effect System (Capability Tracking)

**Grant Declaration:**

```cursive
// Procedure declares what capabilities it needs
procedure read_config(path: string): string
    sequent { [fs::read, alloc::heap] |- true => true }  // Required capabilities
{
    let content = file::read(path)  // Requires fs::read
    result content.to_owned()        // Requires alloc::heap
}
```

**Grant Composition:**

```cursive
// Grants compose through calls
procedure process()
    sequent { [fs::read, fs::write, alloc::heap] |- true => true }
{
    let data = read_config("config.json")  // Needs fs::read, alloc::heap
    file::write("output.txt", data)         // Needs fs::write
}
// process must declare ALL grants used by itself and callees
```

**Grant Checking Rule:**

```
Caller grants ⊇ Callee grants + Argument grants

If caller has {fs::read, fs::write}:
    ✅ Can call procedure needing {fs::read}
    ✅ Can call procedure needing {fs::write}
    ✅ Can call procedure needing {fs::read, fs::write}
    ❌ Cannot call procedure needing {alloc::heap}
```

**Common Grants:**

```cursive
fs::read        // Read from files
fs::write       // Write to files
net::connect    // Network connections
alloc::heap     // Heap allocation
ffi::call       // Foreign function interface
unsafe::ptr     // Raw pointer operations
panic           // May panic at runtime
```

### 0.1.5 Region System (Lifetime Management)

**Region Declaration:**

```cursive
region temp {
    // All allocations in 'temp' freed at closing brace
    let data = alloc_in<temp>(Data::new())
    process(data)
} // data freed here (O(1) deallocation)
```

**Escape Analysis:**

```cursive
// ❌ ERROR: Cannot return region-allocated value
procedure bad() -> Data {
    region r {
        let data = alloc_in<r>(Data::new())
        result data  // ERROR: data cannot escape region r
    }
}

// ✅ CORRECT: Allocate on heap or outside region
procedure good() -> Data
    sequent { [alloc::heap] |- true => true }
{
    let data = Data::new()  // Heap allocation
    result data             // OK: not region-allocated
}
```

**Region Nesting:**

```cursive
region outer {
    let x = alloc_in<outer>(10)

    region inner {
        let y = alloc_in<inner>(20)
        let z = x + y  // OK: can use outer from inner
    } // y freed here

} // x freed here
```

### 0.1.6 Statement Termination (Newline Rules)

**Default: Newline Ends Statement**

```cursive
let x = 1
let y = 2
let z = x + y
```

**Four Continuation Cases:**

**Case 1: Open Delimiter**

```cursive
let result = function(
    arg1,
    arg2,
    arg3)  // Continues because ( is open
```

**Case 2: Trailing Operator**

```cursive
let sum = a +
          b +
          c  // Continues because lines end with +
```

**Case 3: Leading Dot**

```cursive
let result = object
    .method1()
    .method2()
    .method3()  // Continues because lines start with .
```

**Case 4: Leading Pipeline**

```cursive
let output = input
    => stage1: Type1
    => stage2: Type2
    => stage3: Type3  // Continues because lines start with =>
```

**Semicolons (Optional):**

```cursive
// Use semicolons for multiple statements on one line
let x = 1; let y = 2; let z = x + y
```

---

## 0.2 Comprehensive Mental Models

### 0.2.1 The Permission Mental Model

**Think of permissions as access control badges:**

```
own T  = Master key - can do anything
         • Move the value
         • Pass to own parameters
         • Call consuming procedures
         • Mutate fields (if mutable)
         • Read fields

mut T  = Maintenance access - can modify
         • Mutate fields
         • Call mut procedures
         • Read fields
         • Cannot move or consume

imm T  = Visitor access - can only observe
         • Read fields
         • Call imm procedures
         • Cannot mutate
         • Cannot move
```

**Permission Lattice (Weakening Only):**

```
own T
  ↓ (automatic)
mut T
  ↓ (automatic)
imm T

Cannot go upward (no strengthening)
```

**Examples of Permission Reasoning:**

```cursive
// Scenario 1: Consuming data
procedure transform(data: own Data) -> ProcessedData {
    // I own data, so I can consume it
    let processed = expensive_transform(move data)
    result processed
}

// Scenario 2: Modifying in place
procedure increment_all(numbers: mut [i32]) {
    // I have mut access, so I can modify elements
    var i: usize = 0
    loop i < numbers.len() {
        numbers[i] = numbers[i] + 1
        i = i + 1
    }
}

// Scenario 3: Reading only
procedure sum(numbers: [i32]) -> i32 {  // Implicitly imm [i32]
    // I have imm access, so I can only read
    var total: i32 = 0
    loop n: i32 in numbers {
        total = total + n
    }
    result total
}
```

### 0.2.2 The Grant Mental Model

**Think of grants as capability tokens:**

```
Procedure signature with empty grant context = Pure procedure
    • No I/O
    • No allocation
    • No foreign procedure calls that require grants
    • No unsafe operations
    • Only computation on inputs

Procedure with [fs::read] grant = Has file read capability
    • Can call procedures requiring fs::read
    • Can call file::read, etc.
    • Still cannot allocate or write

Procedure with [fs::read, alloc::heap] grants = Has both tokens
    • Can do everything fs::read allows
    • Can also allocate on heap
    • Still cannot write or use unsafe
```

**Grant Flow:**

```cursive
// Pure procedure - no grants
procedure add(a: i32, b: i32) -> i32
    sequent { [ ] |- true => true }
{
    result a + b
}

// Procedure with one grant
procedure read_file(path: string) -> string
    sequent { [fs::read] |- true => true }
{
    // Can call anything requiring fs::read
    result file::read(path)
}

// Procedure with multiple grants
procedure process_file(path: string) -> string
    sequent { [fs::read, alloc::heap] |- true => true }
{
    let content = read_file(path)  // Needs fs::read
    result content.to_uppercase()   // Needs alloc::heap
}

// Procedure composing all grants
procedure main()
    sequent { [fs::read, fs::write, alloc::heap] |- true => true }
{
    let data = process_file("input.txt")  // Needs fs::read, alloc::heap
    file::write("output.txt", data)        // Needs fs::write
}
```

### 0.2.3 The Region Mental Model

**Think of regions as stack frames for allocations:**

```
procedure call creates stack frame
    ↓
region block creates allocation frame
    ↓
alloc_in<region>() allocates in that frame
    ↓
region exit frees entire frame at once (O(1))
    ↓
no values can escape (compile-time check)
```

**Region Lifetime Visualization:**

```cursive
procedure example() {
    // Stack frame for example()

    region outer {
        // Allocation frame 'outer' created
        let x = alloc_in<outer>(10)

        region inner {
            // Allocation frame 'inner' created (nested)
            let y = alloc_in<inner>(20)

            // Both x and y accessible here

        } // inner frame freed (all 'inner' allocations gone)

        // x still accessible
        // y no longer accessible

    } // outer frame freed (all 'outer' allocations gone)

} // Stack frame for example() freed
```

**Region Benefits:**

1. **Deterministic deallocation** - Exact point where memory freed
2. **O(1) deallocation** - Free entire frame at once, not individual objects
3. **No fragmentation** - Arena-style allocation
4. **Cache-friendly** - Sequential allocations in same region
5. **No GC pauses** - No garbage collector

### 0.2.4 The Modal Types Mental Model

**Think of modal types as state machines enforced by the type system:**

```
Type@State1
    ↓ transition1()
Type@State2
    ↓ transition2()
Type@State3

Each state has:
    • Different fields (structure)
    • Different available procedures (operations)
    • Type system prevents invalid states
```

**State Machine Example:**

```cursive
modal Connection {
    // State 1: Not connected
    @Disconnected {
        address: string,
        port: u16
    }

    // State 2: Connected
    @Connected {
        address: string,
        port: u16,
        socket: Socket
    }

    // State 3: Failed
    @Failed {
        address: string,
        port: u16,
        error: Error
    }

    // Transitions
    procedure @Disconnected -> @Connected
        connect(self: Connection@Disconnected): Connection@Connected
        sequent { [net::connect] |- true => true }

    procedure @Disconnected -> @Failed
        fail(self: Connection@Disconnected, err: Error): Connection@Failed

    procedure @Connected -> @Disconnected
        disconnect(self: Connection@Disconnected): Connection@Disconnected
        sequent { [net::close] |- true => true }
}

// Usage enforces state machine
let conn: Connection@Disconnected = Connection::new("localhost", 8080)

// Can only call procedures available in Disconnected state
let active: Connection@Connected = conn.connect()  // Transition

// Can only call procedures available in Connected state
active::send(data)  // Only available in Connected state

// Cannot call connect() again - not in Disconnected state
// active.connect()  // ❌ ERROR: Type is @Connected, not @Disconnected
```

**Modal Benefits:**

1. **Illegal states unrepresentable** - Cannot have disconnected connection with open socket
2. **Protocol enforcement** - Cannot send before connecting
3. **Zero runtime cost** - States erase at compile time
4. **Clear state tracking** - Type tells you current state

---

## 0.3 Type System Deep Dive

### 0.3.1 Primitive Types (Complete List)

**Signed Integers:**

```cursive
i8      // -128 to 127
i16     // -32,768 to 32,767
i32     // -2,147,483,648 to 2,147,483,647
i64     // -9,223,372,036,854,775,808 to 9,223,372,036,854,775,807
isize   // Pointer-sized signed integer (32 or 64 bit)
```

**Unsigned Integers:**

```cursive
u8      // 0 to 255
u16     // 0 to 65,535
u32     // 0 to 4,294,967,295
u64     // 0 to 18,446,744,073,709,551,615
usize   // Pointer-sized unsigned integer (32 or 64 bit)
```

**Floating Point:**

```cursive
f32     // IEEE 754 single precision (32-bit)
f64     // IEEE 754 double precision (64-bit)
```

**Other Primitives:**

```cursive
bool    // true or false
char    // Unicode scalar value (32-bit)
()      // Unit type (zero-sized)
!       // Never type (diverges)
```

**No Implicit Conversions:**

```cursive
// ❌ WRONG: No implicit conversion
let x: i64 = 42_i32

// ✅ CORRECT: Explicit conversion
let x: i64 = 42_i32.to_i64()

// ❌ WRONG: No implicit widening
let y: f64 = 3.14_f32

// ✅ CORRECT: Explicit conversion
let y: f64 = 3.14_f32.to_f64()
```

### 0.3.2 String Types (Modal)

**Cursive has ONE string type with TWO states:**

```cursive
string@View   // Non-owning, read-only view
string@Owned  // Owning, heap-allocated buffer
```

**State Details:**

```cursive
// string@View structure
@View {
    ptr: *u8,    // Pointer to data (not owned)
    len: usize   // Length in bytes
}

// string@Owned structure
@Owned {
    ptr: *u8,    // Pointer to data (owned)
    len: usize,  // Length in bytes
    cap: usize   // Capacity (allocated)
}
```

**Default State Rules:**

| Context              | Default State | Rationale                     |
| -------------------- | ------------- | ----------------------------- |
| Parameter            | `@View`       | Borrow by default (zero-cost) |
| Return type          | `@Owned`      | Transfer ownership            |
| Local variable (imm) | `@View`       | Minimal permission            |
| Local variable (mut) | `@Owned`      | Need ownership to mutate      |
| Record field         | `@Owned`      | Store owned data              |
| Tuple element        | `@Owned`      | Tuple owns elements           |
| Array element        | `@Owned`      | Array owns elements           |
| Generic argument     | No default    | MUST be explicit              |

**Conversions:**

```cursive
// Owned -> View (automatic, zero-cost)
let owned: string@Owned = string::from("hello")
let view: string@View = owned  // Automatic coercion

// View -> Owned (explicit, requires allocation)
let view: string@View = "hello"  // String literal is View
let owned: string@Owned = view.to_owned()  // Requires alloc.heap
```

**String Operations:**

```cursive
// Reading (available on both states)
let s: string = "hello"  // Defaults to @View in this context
let len = s.len()        // Works on View
let ch = s.char_at(0)    // Works on View

// Mutation (requires @Owned)
var s: string@Owned = string::from("hello")
s.append(" world")       // Requires @Owned and alloc.heap
s.push('!')             // Requires @Owned

// Construction
let s1: string@Owned = string::from("literal")     // From literal
let s2: string@Owned = string::from_chars(['a', 'b'])  // From chars
let s3: string@View = "literal"                    // View to literal
```

### 0.3.3 Compound Types

**Tuples:**

```cursive
// Tuple types
let pair: (i32, string) = (42, "hello")
let triple: (i32, i32, i32) = (1, 2, 3)

// Tuple access (zero-indexed)
let first = pair.0   // 42
let second = pair.1  // "hello"

// Tuple destructuring
let (x, y) = pair
```

**Records:**

```cursive
// Record declaration
record Point {
    x: f64,
    y: f64
}

// Record construction
let p: Point = Point { x: 1.0, y: 2.0 }

// Field access
let x_coord = p.x

// Field update (requires mut)
var p2: mut Point = p
p2.x = 3.0
```

**Arrays:**

```cursive
// Fixed-size arrays
let arr: [i32; 5] = [1, 2, 3, 4, 5]
let zeros: [i32; 10] = [0; 10]  // Repeat syntax

// Array access
let first = arr[0]

// Array iteration
loop item: i32 in arr {
    println(item)
}
```

**Slices:**

```cursive
// Slices are views into arrays
let arr: [i32; 5] = [1, 2, 3, 4, 5]
let slice: [i32] = arr[1..4]  // Elements 1, 2, 3

// Slice operations
let len = slice.len()
let item = slice[0]
```

### 0.3.4 Enum Types

**Basic Enums:**

```cursive
enum Status {
    Success,
    Failure,
    Pending
}

// Usage
let s: Status = Status::Success

// Match on enums
let msg: string = match s {
    Status::Success => "ok",
    Status::Failure => "error",
    Status::Pending => "waiting"
}
```

**Enums with Data:**

```cursive
enum Option<T> {
    Some(T),
    None
}

enum Result<T, E> {
    Ok(T),
    Err(E)
}

// Usage
let maybe: Option<i32> = Option::Some(42)
let value: i32 = match maybe {
    Option::Some(v) => v,
    Option::None => 0
}
```

**Important:** Use `::` for enum variant access, not `.`:

```cursive
// ✅ CORRECT
Option::Some(42)
Result::Ok("success")
Status::Pending

// ❌ WRONG (this is Rust/other language syntax)
Option.Some(42)
Result.Ok("success")
```

### 0.3.5 Function Types

**Function Type Syntax:**

```cursive
// Function taking two i32, returning i32, no effects
(i32, i32) -> i32

// Function taking string, returning bool, with io.read effect
(string) -> bool ! {io.read}

// Function taking nothing, returning nothing, with multiple effects
() -> () ! {io.read, io.write, alloc.heap}
```

**Function Values:**

```cursive
// Function as parameter
procedure apply(f: (i32) -> i32, x: i32) -> i32 {
    result f(x)
}

// Function as return value
procedure make_adder(n: i32) -> (i32) -> i32 {
    // Returns a closure
    result closure(x: i32) -> i32 {
        result x + n
    }
}
```

### 0.3.6 Permission Types (Access Control)

**Permission Wrappers:**

```cursive
own<T>   // Exclusive ownership
mut<T>   // Mutable access
imm<T>   // Immutable access (default)
```

**In Practice:**

```cursive
// As parameter types
procedure consume(data: own Data) { ... }
procedure modify(data: mut Data) { ... }
procedure read(data: imm Data) { ... }   // 'imm' usually omitted
procedure read2(data: Data) { ... }       // Same as imm Data

// As variable types
let x: own Data = Data::new()
var y: mut Data = get_data()
let z: imm Data = get_data()
```

---

## 0.4 Expressions and Control Flow Deep Dive

### 0.4.1 Block Expressions

**Basic Block:**

```cursive
let result: i32 = {
    let x = 10
    let y = 20
    result x + y  // Must use 'result' keyword
}
```

**Block as Expression:**

```cursive
// Blocks can be used anywhere expressions are allowed
let message: string = if condition {
    result "success"
} else {
    result "failure"
}

// Blocks in function returns
procedure calculate() -> i32 {
    result {
        let a = 10
        let b = 20
        result a + b
    }
}
```

**Labeled Blocks (Early Exit):**

```cursive
let outcome: Result<i32, string> = 'compute: {
    if error_condition {
        break 'compute Err("error occurred")
    }

    let value = expensive_calculation()

    if value < 0 {
        break 'compute Err("negative value")
    }

    result Ok(value)
}
```

### 0.4.2 Pipeline Expressions

**Pipeline Structure:**

```cursive
let output: FinalType = input_value
    => stage1: IntermediateType1
    => stage2: IntermediateType2
    => stage3: FinalType
```

**Each stage is a function call:**

```cursive
// This pipeline:
let result: string = data
    => parse: ParsedData
    => validate: Result<ParsedData, Error>
    => serialize: string

// Is equivalent to:
let temp1: ParsedData = parse(data)
let temp2: Result<ParsedData, Error> = validate(temp1)
let result: string = serialize(temp2)
```

**Type annotations are REQUIRED:**

```cursive
// ❌ WRONG: Missing type annotations
let result = input
    => process
    => transform

// ✅ CORRECT: Each stage annotated
let result: OutputType = input
    => process: IntermediateType
    => transform: OutputType
```

**Complex Pipeline Example:**

```cursive
procedure process_request(req: Request) -> Response
    uses io.read, alloc.heap
{
    result req
        => validate: Result<Request, Error>
        => parse_body: RequestData
        => enrich_data: EnrichedData
        => process: ProcessedData
        => format_response: Response
}
```

### 0.4.3 Match Expressions

**Basic Match:**

```cursive
let result: string = match value {
    Option::Some(x) => "found: {x}",
    Option::None => "not found"
}
```

**Match with Guards:**

```cursive
let category: string = match age {
    x if x < 13 => "child",
    x if x < 20 => "teenager",
    x if x < 65 => "adult",
    _ => "senior"
}
```

**Match with Destructuring:**

```cursive
let msg: string = match result {
    Result::Ok(value) => "success: {value}",
    Result::Err(error) => "error: {error}"
}

// Tuple destructuring
let description: string = match pair {
    (0, 0) => "origin",
    (0, y) => "on y-axis",
    (x, 0) => "on x-axis",
    (x, y) => "point at ({x}, {y})"
}
```

**Type Annotation Required When Binding:**

```cursive
// ✅ CORRECT: Type annotation on binding
let msg: string = match opt {
    Option::Some(x) => x,
    Option::None => "default"
}

// ❌ WRONG: No type annotation
let msg = match opt {
    Option::Some(x) => x,
    Option::None => "default"
}
```

### 0.4.4 Loop Expressions

**Infinite Loop:**

```cursive
loop {
    if should_exit {
        break
    }
    process()
}
```

**Conditional Loop:**

```cursive
var i: i32 = 0
loop i < 10 {
    println(i)
    i = i + 1
}
```

**Iterator Loop (Type Annotation Required):**

```cursive
let items: [i32; 5] = [1, 2, 3, 4, 5]

// ✅ CORRECT: Type annotation on iterator
loop item: i32 in items {
    println(item)
}

// ❌ WRONG: No type annotation
loop item in items {
    println(item)
}
```

**Loop with Break Value:**

```cursive
let found: Option<i32> = loop item: i32 in items {
    if item > 10 {
        break Option::Some(item)
    }
}
// If loop completes without break, result is the type's default
// or last evaluated expression
```

**Loop with Label:**

```cursive
'outer: loop i: i32 in outer_items {
    'inner: loop j: i32 in inner_items {
        if should_exit_both {
            break 'outer
        }
        if should_exit_inner {
            break 'inner
        }
    }
}
```

### 0.4.5 Conditional Expressions

**If Expression:**

```cursive
let status: string = if success {
    result "ok"
} else {
    result "error"
}
```

**If-Let Pattern Matching:**

```cursive
if let Option::Some(value) = maybe_value {
    println("Found: {value}")
} else {
    println("Not found")
}
```

**Multiple Conditions:**

```cursive
let category: string = if age < 13 {
    result "child"
} else if age < 20 {
    result "teen"
} else if age < 65 {
    result "adult"
} else {
    result "senior"
}
```

---

## 0.5 Declaration and Binding Patterns

### 0.5.1 Variable Declarations

**Immutable Bindings (`let`):**

```cursive
// Basic binding
let x: i32 = 42

// Immutable binding to mutable value
let y: mut i32 = get_value()
y = y + 1  // ❌ ERROR: Cannot reassign y
*y = *y + 1  // ✅ OK: Can mutate through mut reference

// Multiple bindings
let a: i32 = 1
let b: i32 = 2
let c: i32 = a + b
```

**Mutable Bindings (`var`):**

```cursive
// Basic mutable binding
var counter: i32 = 0
counter = counter + 1  // ✅ OK: Can reassign

// Mutable binding to mutable value
var buffer: mut [u8; 1024] = [0; 1024]
buffer = other_buffer  // ✅ OK: Can reassign binding
buffer[0] = 255        // ✅ OK: Can mutate value
```

**CRITICAL: Never use `let mut` (Rust syntax):**

```cursive
// ❌ WRONG: Rust syntax, invalid in Cursive
let mut counter = 0

// ✅ CORRECT: Cursive syntax
var counter: i32 = 0
```

### 0.5.2 Pattern Matching in Bindings

**Tuple Destructuring:**

```cursive
let (x, y): (i32, i32) = (10, 20)
let (first, second, third): (i32, i32, i32) = (1, 2, 3)
```

**Record Destructuring:**

```cursive
record Point { x: f64, y: f64 }

let p: Point = Point { x: 1.0, y: 2.0 }
let Point { x, y } = p  // Destructure into x and y bindings
```

**Enum Destructuring (in match):**

```cursive
let opt: Option<i32> = Option::Some(42)

let value: i32 = match opt {
    Option::Some(v) => v,
    Option::None => 0
}
```

### 0.5.3 Shadowing

**Shadowing is allowed:**

```cursive
let x: i32 = 10
println(x)  // 10

let x: string = "hello"  // Shadows previous x
println(x)  // "hello"

let x: i32 = 42  // Shadows again
println(x)  // 42
```

**Shadowing vs. Mutation:**

```cursive
// Shadowing: New binding, can change type
let x: i32 = 10
let x: string = "hello"  // ✅ OK: Different binding

// Mutation: Same binding, same type
var y: i32 = 10
y = 20  // ✅ OK: Reassignment
y = "hello"  // ❌ ERROR: Type mismatch
```

---

## 0.6 Modal Types In Depth

### 0.6.1 Modal Type Declaration

**Complete Modal Type:**

```cursive
modal File {
    // State 1: Closed file
    @Closed {
        path: string@View
    }

    // State 2: Open file
    @Open {
        path: string@View,
        handle: FileHandle
    }

    // State 3: Error state
    @Error {
        path: string@View,
        error: ErrorInfo
    }

    // Transition: Closed -> Open
    procedure @Closed -> @Open
        open(self: File@Closed): File@Open
        uses io.open
        must self.path.len() > 0
    {
        // Implementation
    }

    // Transition: Closed -> Error
    procedure @Closed -> @Error
        fail_open(self: File@Closed, err: ErrorInfo): File@Error
    {
        // Implementation
    }

    // Transition: Open -> Closed
    procedure @Open -> @Closed
        close(self: File@Open): File@Closed
        uses io.close
    {
        // Implementation
    }

    // Self-loop: Open -> Open
    procedure @Open -> @Open
        read(self: mut File@Open, buffer: mut [u8]): usize
        uses io.read
    {
        // Implementation
    }
}
```

### 0.6.2 Modal State Coercions

**Structural Subset Coercion:**

```cursive
modal Connection {
    @Full {
        socket: Socket,
        buffer: [u8; 1024],
        metadata: Metadata
    }

    @Minimal {
        socket: Socket,
        buffer: [u8; 1024]
    }

    // Automatic coercion: Full -> Minimal (fields are subset)
    coerce @Full <: @Minimal {
        cost: O(1)
        uses: ∅
    }
}

// Usage
let full: Connection@Full = get_full_connection()
let minimal: Connection@Minimal = full  // Automatic coercion
```

### 0.6.3 Modal Usage Patterns

**Pattern 1: Builder with States:**

```cursive
modal RequestBuilder {
    @Empty {
        // No fields
    }

    @WithUrl {
        url: string@Owned
    }

    @WithHeaders {
        url: string@Owned,
        headers: Map<string, string>
    }

    @Complete {
        url: string@Owned,
        headers: Map<string, string>,
        body: string@Owned
    }

    // Transitions form a builder chain
    procedure @Empty -> @WithUrl
        url(self: RequestBuilder@Empty, url: string) -> RequestBuilder@WithUrl

    procedure @WithUrl -> @WithHeaders
        headers(self: RequestBuilder@WithUrl, h: Map<string, string>)
            -> RequestBuilder@WithHeaders

    procedure @WithHeaders -> @Complete
        body(self: RequestBuilder@WithHeaders, b: string)
            -> RequestBuilder@Complete

    // Final build only available in Complete state
    procedure @Complete -> @Complete
        build(self: RequestBuilder@Complete) -> Request
}

// Usage
let request: Request = RequestBuilder::new()
    .url("https://example.com")
    .headers(headers)
    .body("data")
    .build()
```

**Pattern 2: Protocol State Machine:**

```cursive
modal TcpConnection {
    @Listen { port: u16 }
    @SynReceived { port: u16, remote: Address }
    @Established { port: u16, remote: Address, socket: Socket }
    @FinWait { port: u16, reason: string }
    @Closed { }

    // TCP state machine transitions
    procedure @Listen -> @SynReceived
        receive_syn(self: TcpConnection@Listen, from: Address)
            -> TcpConnection@SynReceived

    procedure @SynReceived -> @Established
        send_ack(self: TcpConnection@SynReceived)
            -> TcpConnection@Established

    procedure @Established -> @FinWait
        close_connection(self: TcpConnection@Established, reason: string)
            -> TcpConnection@FinWait

    procedure @FinWait -> @Closed
        finalize(self: TcpConnection@FinWait)
            -> TcpConnection@Closed
}
```

**Pattern 3: Resource Lifecycle:**

```cursive
modal Database {
    @Disconnected { config: Config }
    @Connected { config: Config, connection: DbConnection }
    @InTransaction { config: Config, connection: DbConnection, tx: Transaction }

    procedure @Disconnected -> @Connected
        connect(self: Database@Disconnected) -> Database@Connected
        uses io.connect, alloc.heap

    procedure @Connected -> @InTransaction
        begin_transaction(self: Database@Connected) -> Database@InTransaction
        uses io.write

    procedure @InTransaction -> @Connected
        commit(self: Database@InTransaction) -> Database@Connected
        uses io.write

    procedure @InTransaction -> @Connected
        rollback(self: Database@InTransaction) -> Database@Connected
        uses io.write

    procedure @Connected -> @Disconnected
        disconnect(self: Database@Connected) -> Database@Disconnected
        uses io.close
}
```

### 0.6.4 Modal Type Benefits and Costs

**Benefits:**

1. **Illegal states impossible** - Cannot have open file without handle
2. **Protocol enforcement** - Cannot send data before connecting
3. **Zero runtime overhead** - States erase at compile time
4. **Clear documentation** - Type shows available operations
5. **Refactoring safety** - State changes cause type errors

**Costs:**

1. **More verbose** - Must track states explicitly
2. **More complex types** - Generics with modal states can be complex
3. **Learning curve** - Requires understanding state machines

**When to Use Modal Types:**

- ✅ Protocol implementations (network, hardware)
- ✅ Resource lifecycle management (files, connections)
- ✅ Builder patterns
- ✅ State machines with illegal states
- ❌ Simple data structures
- ❌ When states can change arbitrarily
- ❌ When runtime state checking preferred

---

## 0.7 Contracts and Effects In Depth

### 0.7.1 Unified Sequent Syntax (`sequent`)

**Contract Declaration:**

Cursive uses sequent calculus for contractual sequents:

```cursive
procedure name(params): ReturnType
    [[ grants |- precondition => postcondition ]]
{
    body
}
```

Where:

- `grants` — Grant context (required capabilities)
- `|-` — Turnstile (entailment operator)
- `precondition` — What must be true when called
- `=>` — Implication
- `postcondition` — What will be true when done

**Example with Grants:**

```cursive
procedure read_config(path: string) -> Config
    sequent { [fs::read, alloc::heap] |- true => true }
{
    let content: string = file::read(path)  // Needs fs::read
    let parsed: Config = parse(content)      // Needs alloc::heap
    result parsed
}
```

**Grant Categories:**

```cursive
// File System Grants
fs::read        // Read from files
fs::write       // Write to files
fs::delete      // Delete files
fs::metadata    // Read/modify file metadata

// Network Grants
net::read       // Read from network
net::write      // Write to network
net::connect    // Initiate connections
net::listen     // Accept connections

// Memory Grants
alloc::heap     // Heap allocation
alloc::region   // Region allocation

// Interop Grants
ffi::call       // Foreign function interface
unsafe::ptr     // Raw pointer operations
unsafe::transmute // Type transmutation

// Control Flow Grants
panic           // May panic at runtime
```

**Grant Composition:**

```cursive
// Pure function - no grants
function pure_add(a: i32, b: i32) -> i32 {
    result a + b
}

// Procedure with grants
procedure with_io()
    sequent { [fs::read, fs::write] |- true => true }
{
    let data = read_file("input.txt")   // Needs fs::read
    write_file("output.txt", data)       // Needs fs::write
}

// Procedure composing grants transitively
procedure main()
    sequent { [fs::read, fs::write, alloc::heap] |- true => true }
{
    with_io()  // OK: main has fs::read, fs::write

    let data2 = complex_process()  // Needs alloc::heap
}
```

### 0.7.2 Preconditions

**Basic Preconditions:**

```cursive
procedure divide(a: i32, b: i32) -> i32
    sequent { [] |- b != 0 => true }
{
    result a / b  // Safe: b != 0 guaranteed by caller
}

// Caller must ensure precondition
let result: i32 = divide(10, 2)  // OK: 2 != 0
let bad: i32 = divide(10, 0)      // Runtime error or compile error
```

**Multiple Preconditions:**

```cursive
procedure process_array(arr: [i32], index: usize) -> i32
    sequent { [] |- arr.len() > 0 && index < arr.len() => true }
{
    result arr[index]  // Safe: bounds checked by preconditions
}
```

**Preconditions with Permissions:**

```cursive
procedure append(list: mut List, item: own Item)
    sequent { [] |- list.capacity() > list.len() => true }
{
    // Safe to append: guaranteed space available
}
```

### 0.7.3 Postconditions

**Basic Postconditions:**

```cursive
procedure allocate_buffer(size: usize) -> [u8]
    sequent { [alloc::heap] |- true => result.len() == size }
{
    // Implementation must ensure result length equals size
}
```

**Postconditions with State:**

```cursive
procedure increment(counter: mut i32)
    sequent { [] |- counter < i32::MAX => counter == @old(counter) + 1 }
{
    counter = counter + 1
}
// @old(counter) refers to counter's value at function entry
```

**Postconditions with Modal States:**

```cursive
modal File {
    @Closed { path: string }
    @Open { path: string, handle: Handle }

    procedure @Closed -> @Open
        open(self: File@Closed) -> File@Open
        sequent {
            [fs::open]
            |- true
            => result.path == self.path && result.handle.is_valid()
        }
    {
        // Implementation
    }
}
```

### 0.7.4 Contract Verification Modes

**Runtime Verification:**

```cursive
[[verify(runtime)]]
procedure safe_divide(a: i32, b: i32) -> i32
    must b != 0
    will result == a / b
{
    result a / b
}
// Generates runtime check: if b == 0, panic
```

**Static Verification:**

```cursive
[[verify(static)]]
procedure proven_divide(a: i32, b: i32) -> i32
    must b != 0
    will result == a / b
{
    result a / b
}
// Compiler attempts to prove b != 0 at all call sites
// Compile error if proof fails
```

**No Verification:**

```cursive
[[verify(none)]]
procedure trusted_divide(a: i32, b: i32) -> i32
    must b != 0
    will result == a / b
{
    result a / b
}
// No checks generated - caller responsible for correctness
```

### 0.7.5 Contract Inheritance and Composition

**Contract Weakening:**

```cursive
contract Reader {
    procedure read() -> string
        uses io.read
}

// Implementation can have weaker preconditions
record FileReader implements Reader {
    procedure read() -> string
        uses io.read
        // No preconditions - accepts all cases
    {
        // Implementation
    }
}
```

**Contract Strengthening:**

```cursive
contract Writer {
    procedure write(data: string)
        uses io.write
}

// Implementation can have stronger postconditions
record SafeWriter implements Writer {
    procedure write(data: string)
        uses io.write
        will self.bytes_written() == old(self.bytes_written()) + data.len()
    {
        // Implementation with guarantee
    }
}
```

---

## 0.8 Region System In Depth

### 0.8.1 Region Syntax and Semantics

**Basic Region:**

```cursive
region temp {
    let data = alloc_in<temp>(Data::new())
    process(data)
} // All 'temp' allocations freed here (O(1))
```

**Multiple Regions:**

```cursive
region outer {
    let x = alloc_in<outer>(10)

    region inner {
        let y = alloc_in<inner>(20)
        // Both x and y available
    } // y freed

    // x still available, y gone
} // x freed
```

**Region with Loop:**

```cursive
var total: i32 = 0

loop item: Item in items {
    region iter {
        // New iteration, fresh region
        let temp = alloc_in<iter>(process(item))
        total = total + temp.value
    } // temp freed each iteration
}
```

### 0.8.2 Region Escape Analysis

**Escape Rules:**

```
1. Value allocated in region R cannot be returned from R
2. Value allocated in region R cannot be stored in outer scope
3. Value allocated in region R cannot outlive R
4. References to region-allocated values cannot escape R
```

**Escape Examples:**

```cursive
// ❌ ERROR: Cannot return region-allocated value
procedure bad1() -> Data {
    region r {
        let data = alloc_in<r>(Data::new())
        result data  // ERROR: Escapes region r
    }
}

// ❌ ERROR: Cannot store in outer scope
procedure bad2() {
    var outer_ref: Data = undefined

    region r {
        let data = alloc_in<r>(Data::new())
        outer_ref = data  // ERROR: Escapes region r
    }
}

// ✅ OK: Copy value out (if Data implements Copy)
procedure good1() -> i32 {
    region r {
        let data = alloc_in<r>(42)
        result data.copy()  // OK: Copies value
    }
}

// ✅ OK: Allocate on heap instead
procedure good2() -> Data
    sequent { [alloc::heap] |- true => true }
{
    let data = Data::new()  // Heap allocation
    result data             // OK: Not region-allocated
}
```

### 0.8.3 Region Performance Benefits

**Memory Locality:**

```cursive
// Bad: Each allocation separate, poor locality
var items: [Item] = []
loop i: i32 in range(0, 1000) {
    let item = Item::new()  // 1000 separate heap allocations
    items.push(item)
}

// Good: All allocations in one region, excellent locality
region batch {
    var items: [Item] = []
    loop i: i32 in range(0, 1000) {
        let item = alloc_in<batch>(Item::new())  // All in batch region
        items.push(item)
    }
    process_batch(items)
} // Free all 1000 items at once (O(1))
```

**Cache Performance:**

```cursive
// Region allocations are sequential
region proc {
    let a = alloc_in<proc>(Data1::new())  // Address: base + 0
    let b = alloc_in<proc>(Data2::new())  // Address: base + size(Data1)
    let c = alloc_in<proc>(Data3::new())  // Address: base + size(Data1) + size(Data2)
    // a, b, c are sequential in memory - cache-friendly
}
```

### 0.8.4 Region Patterns

**Pattern 1: Per-Request Region:**

```cursive
procedure handle_request(req: Request) -> Response {
    region request {
        // All request processing uses request region
        let parsed = alloc_in<request>(parse(req))
        let processed = alloc_in<request>(process(parsed))
        let response = format_response(processed)

        result response  // Response is NOT region-allocated
    } // All request data freed here
}
```

**Pattern 2: Per-Iteration Region:**

```cursive
loop item: Item in items {
    region iter {
        // Fresh region each iteration
        let intermediate1 = alloc_in<iter>(transform1(item))
        let intermediate2 = alloc_in<iter>(transform2(intermediate1))
        let result = transform3(intermediate2)

        emit(result)
    } // Iteration data freed, no accumulation
}
```

**Pattern 3: Nested Processing:**

```cursive
region outer {
    let input_data = alloc_in<outer>(load_input())

    var results: [Result] = []

    loop chunk: Chunk in input_data.chunks() {
        region inner {
            let processed = alloc_in<inner>(process_chunk(chunk))
            let result = extract_result(processed)
            results.push(result)  // result is NOT region-allocated
        } // processed freed, result retained
    }

    let final_output = combine_results(results)
    result final_output
} // input_data freed
```

---

## 0.9 Error Handling Patterns

### 0.9.1 Result Type

**Result Definition:**

```cursive
enum Result<T, E> {
    Ok(T),
    Err(E)
}
```

**Basic Usage:**

```cursive
procedure parse_int(s: string) -> Result<i32, string> {
    // Implementation
    if valid {
        result Result::Ok(42)
    } else {
        result Result::Err("invalid integer")
    }
}

// Using result
let parsed: Result<i32, string> = parse_int("42")
let value: i32 = match parsed {
    Result::Ok(n) => n,
    Result::Err(e) => {
        println("Error: {e}")
        result 0
    }
}
```

### 0.9.2 Error Propagation (No ? Operator)

**Cursive does NOT have ? operator. Use explicit match:**

```cursive
// ❌ WRONG: ? operator doesn't exist
procedure process(s: string) -> Result<i32, string> {
    let value = parse_int(s)?;
    result Result::Ok(value * 2)
}

// ✅ CORRECT: Explicit match
procedure process(s: string) -> Result<i32, string> {
    let parsed: Result<i32, string> = parse_int(s)
    let value: i32 = match parsed {
        Result::Ok(n) => n,
        Result::Err(e) => return Result::Err(e)
    }
    result Result::Ok(value * 2)
}

// ✅ ALSO CORRECT: Early exit with labeled block
procedure process2(s: string) -> Result<i32, string> {
    result 'process: {
        let parsed: Result<i32, string> = parse_int(s)
        let value: i32 = match parsed {
            Result::Ok(n) => n,
            Result::Err(e) => break 'process Result::Err(e)
        }
        result Result::Ok(value * 2)
    }
}
```

### 0.9.3 Option Type

**Option Definition:**

```cursive
enum Option<T> {
    Some(T),
    None
}
```

**Usage Patterns:**

```cursive
// Returning optional value
procedure find(arr: [i32], target: i32) -> Option<usize> {
    var i: usize = 0
    loop i < arr.len() {
        if arr[i] == target {
            result Option::Some(i)
        }
        i = i + 1
    }
    result Option::None
}

// Using optional value
let arr: [i32; 5] = [1, 2, 3, 4, 5]
let index: Option<usize> = find(arr, 3)

let msg: string = match index {
    Option::Some(i) => "found at index {i}",
    Option::None => "not found"
}
```

### 0.9.4 Panic Handling

**Operations that may panic:**

```cursive
// Array indexing
let arr: [i32; 5] = [1, 2, 3, 4, 5]
let x = arr[10]  // Panics if index out of bounds (debug mode)

// Integer overflow
let x: i32 = i32::MAX
let y = x + 1  // Panics on overflow (debug mode)

// Division by zero
let z = 10 / 0  // Panics

// Explicit panic
panic("something went wrong")
```

**Preventing panics:**

```cursive
// Use checked operations
let arr: [i32; 5] = [1, 2, 3, 4, 5]
let x: Option<i32> = arr.get(10)  // Returns Option::None

// Use wrapping arithmetic
let x: i32 = i32::MAX
let y: i32 = x.wrapping_add(1)  // Wraps around

// Check before division
procedure safe_div(a: i32, b: i32) -> Option<i32> {
    if b == 0 {
        result Option::None
    }
    result Option::Some(a / b)
}
```

**No-panic guarantee:**

```cursive
[[no_panic]]
procedure guaranteed_safe(x: i32) -> i32 {
    // Compiler ensures this cannot panic
    result x + 1  // Must prove no overflow or use wrapping
}
```

---

## 0.10 Comprehensive Code Patterns

### 0.10.1 Iterator Processing with Regions

```cursive
[[verify(runtime)]]
procedure process_items(items: [Item])
    sequent { [fs::write, alloc::heap] |- true => true }
{
    var total: i32 = 0

    loop item: Item in items
        with { items.len() >= 0 }  // Loop invariant
    {
        region iter {
            // Allocate temporary data in iteration region
            let encoded = alloc_in<iter>(encode(item))
            let processed = alloc_in<iter>(process(encoded))

            // Extract result (not region-allocated)
            total = total + processed.value

            // Write output
            println(processed)
        } // Free encoded and processed
    }

    println("Total: {total}")
}
```

### 0.10.2 Builder Pattern with Modal States

```cursive
modal HttpRequestBuilder {
    @Empty { }

    @WithMethod {
        method: string@Owned
    }

    @WithUrl {
        method: string@Owned,
        url: string@Owned
    }

    @WithHeaders {
        method: string@Owned,
        url: string@Owned,
        headers: Map<string, string>
    }

    @Complete {
        method: string@Owned,
        url: string@Owned,
        headers: Map<string, string>,
        body: Option<string@Owned>
    }

    procedure @Empty -> @WithMethod
        method(self: HttpRequestBuilder@Empty, m: string)
            -> HttpRequestBuilder@WithMethod
        uses alloc.heap
    {
        result HttpRequestBuilder@WithMethod {
            method: m.to_owned()
        }
    }

    procedure @WithMethod -> @WithUrl
        url(self: HttpRequestBuilder@WithMethod, u: string)
            -> HttpRequestBuilder@WithUrl
        uses alloc.heap
    {
        result HttpRequestBuilder@WithUrl {
            method: self.method,
            url: u.to_owned()
        }
    }

    procedure @WithUrl -> @WithHeaders
        headers(self: HttpRequestBuilder@WithUrl, h: Map<string, string>)
            -> HttpRequestBuilder@WithHeaders
    {
        result HttpRequestBuilder@WithHeaders {
            method: self.method,
            url: self.url,
            headers: h
        }
    }

    procedure @WithHeaders -> @Complete
        body(self: HttpRequestBuilder@WithHeaders, b: string)
            -> HttpRequestBuilder@Complete
        uses alloc.heap
    {
        result HttpRequestBuilder@Complete {
            method: self.method,
            url: self.url,
            headers: self.headers,
            body: Option::Some(b.to_owned())
        }
    }

    procedure @WithHeaders -> @Complete
        no_body(self: HttpRequestBuilder@WithHeaders)
            -> HttpRequestBuilder@Complete
    {
        result HttpRequestBuilder@Complete {
            method: self.method,
            url: self.url,
            headers: self.headers,
            body: Option::None
        }
    }

    procedure @Complete -> @Complete
        build(self: HttpRequestBuilder@Complete) -> HttpRequest
        uses alloc.heap
    {
        result HttpRequest {
            method: self.method,
            url: self.url,
            headers: self.headers,
            body: self.body
        }
    }
}

// Usage
procedure make_request()
    sequent { [net::write, alloc::heap] |- true => true }
{
    let request: HttpRequest = HttpRequestBuilder::new()
        .method("POST")
        .url("https://api.example.com/data")
        .headers(default_headers())
        .body("{\"key\": \"value\"}")
        .build()

    send_request(request)
}
```

### 0.10.3 Resource Management with RAII

```cursive
modal FileHandle {
    @Closed { path: string@View }
    @Open { path: string@View, fd: i32 }

    procedure @Closed -> @Open
        open(self: FileHandle@Closed) -> FileHandle@Open
        uses io.open
    {
        let fd = system::open(self.path)
        result FileHandle@Open {
            path: self.path,
            fd: fd
        }
    }

    procedure @Open -> @Closed
        close(self: FileHandle@Open) -> FileHandle@Closed
        sequent { [fs::close] |- true => true }
    {
        system::close(self.fd)
        result FileHandle@Closed {
            path: self.path
        }
    }
}

// RAII pattern with defer
procedure process_file(path: string)
    sequent { [fs::open, fs::read, fs::close] |- true => true }
{
    let handle: FileHandle@Closed = FileHandle::new(path)
    let opened: FileHandle@Open = handle.open()

    // Ensure file is closed even if early return or panic
    defer {
        let _closed: FileHandle@Closed = opened.close()
    }

    // Process file
    let data = opened::read()
    process(data)

    // File will be closed by defer
}
```

### 0.10.4 Pipeline with Error Handling

```cursive
record ParseError { message: string }
record ValidationError { message: string }
record SerializationError { message: string }

procedure process_request(input: string) -> Result<string, string>
    uses io.read, alloc.heap
{
    result 'pipeline: {
        // Stage 1: Parse
        let parsed: Result<RequestData, ParseError> = input
            => parse: Result<RequestData, ParseError>

        let request_data: RequestData = match parsed {
            Result::Ok(data) => data,
            Result::Err(e) => break 'pipeline Result::Err("Parse error: {e.message}")
        }

        // Stage 2: Validate
        let validated: Result<RequestData, ValidationError> = request_data
            => validate: Result<RequestData, ValidationError>

        let valid_data: RequestData = match validated {
            Result::Ok(data) => data,
            Result::Err(e) => break 'pipeline Result::Err("Validation error: {e.message}")
        }

        // Stage 3: Process
        let processed: ProcessedData = valid_data
            => process_data: ProcessedData

        // Stage 4: Serialize
        let serialized: Result<string, SerializationError> = processed
            => serialize: Result<string, SerializationError>

        let output: string = match serialized {
            Result::Ok(s) => s,
            Result::Err(e) => break 'pipeline Result::Err("Serialization error: {e.message}")
        }

        result Result::Ok(output)
    }
}
```

### 0.10.5 Complex Modal State Machine

```cursive
modal WebSocket {
    @Disconnected {
        url: string@Owned
    }

    @Connecting {
        url: string@Owned,
        connection_id: u64
    }

    @Connected {
        url: string@Owned,
        connection_id: u64,
        socket: Socket
    }

    @Closing {
        url: string@Owned,
        connection_id: u64,
        socket: Socket,
        close_code: u16
    }

    @Closed {
        url: string@Owned,
        close_code: Option<u16>
    }

    @Error {
        url: string@Owned,
        error: ErrorInfo
    }

    // Transitions
    procedure @Disconnected -> @Connecting
        connect(self: WebSocket@Disconnected) -> WebSocket@Connecting
        uses io.connect
    {
        let id = generate_connection_id()
        initiate_connection(self.url, id)
        result WebSocket@Connecting {
            url: self.url,
            connection_id: id
        }
    }

    procedure @Connecting -> @Connected
        complete_handshake(self: WebSocket@Connecting, socket: Socket)
            -> WebSocket@Connected
    {
        result WebSocket@Connected {
            url: self.url,
            connection_id: self.connection_id,
            socket: socket
        }
    }

    procedure @Connecting -> @Error
        fail_connection(self: WebSocket@Connecting, err: ErrorInfo)
            -> WebSocket@Error
    {
        result WebSocket@Error {
            url: self.url,
            error: err
        }
    }

    procedure @Connected -> @Connected
        send(self: mut WebSocket@Connected, data: string) -> Result<(), ErrorInfo>
        uses io.write
    {
        // Implementation
    }

    procedure @Connected -> @Connected
        receive(self: mut WebSocket@Connected) -> Result<string, ErrorInfo>
        uses io.read
    {
        // Implementation
    }

    procedure @Connected -> @Closing
        close(self: WebSocket@Connected, code: u16) -> WebSocket@Closing
        uses io.write
    {
        send_close_frame(self.socket, code)
        result WebSocket@Closing {
            url: self.url,
            connection_id: self.connection_id,
            socket: self.socket,
            close_code: code
        }
    }

    procedure @Closing -> @Closed
        finalize_close(self: WebSocket@Closing) -> WebSocket@Closed
        uses io.close
    {
        system::close_socket(self.socket)
        result WebSocket@Closed {
            url: self.url,
            close_code: Option::Some(self.close_code)
        }
    }

    procedure @Connected -> @Error
        error(self: WebSocket@Connected, err: ErrorInfo) -> WebSocket@Error
    {
        result WebSocket@Error {
            url: self.url,
            error: err
        }
    }
}

// Usage
procedure websocket_example()
    uses io.connect, io.read, io.write, io.close
{
    let ws: WebSocket@Disconnected = WebSocket::new("wss://example.com")
    let connecting: WebSocket@Connecting = ws.connect()

    // Wait for connection (simplified)
    let socket: Socket = wait_for_socket()
    let connected: WebSocket@Connected = connecting.complete_handshake(socket)

    // Send and receive
    let _send_result = connected::send("Hello")
    let received: Result<string, ErrorInfo> = connected::receive()

    // Close
    let closing: WebSocket@Closing = connected.close(1000)
    let closed: WebSocket@Closed = closing.finalize_close()
}
```

---

## 0.11 Complete Working Examples

### 0.11.1 Command-Line Tool

```cursive
record Config {
    input_file: string@Owned,
    output_file: string@Owned,
    verbose: bool
}

procedure parse_args(args: [string]) -> Result<Config, string> {
    if args.len() < 3 {
        result Result::Err("Usage: program <input> <output>")
    }

    result Result::Ok(Config {
        input_file: args[1].to_owned(),
        output_file: args[2].to_owned(),
        verbose: args.len() > 3 && args[3] == "--verbose"
    })
}

procedure process_file(config: Config) -> Result<(), string>
    uses io.read, io.write, alloc.heap
{
    result 'process: {
        // Read input
        let content: Result<string, string> = file::read(config.input_file)
        let input: string = match content {
            Result::Ok(s) => s,
            Result::Err(e) => break 'process Result::Err("Read error: {e}")
        }

        if config.verbose {
            println("Read {input.len()} bytes")
        }

        // Process
        region processing {
            let lines = alloc_in<processing>(input.split('\n'))
            let processed = alloc_in<processing>(transform_lines(lines))
            let output = processed.join('\n')

            // Write output
            let write_result: Result<(), string> = file::write(config.output_file, output)
            match write_result {
                Result::Ok(_) => {
                    if config.verbose {
                        println("Wrote {output.len()} bytes")
                    }
                },
                Result::Err(e) => break 'process Result::Err("Write error: {e}")
            }
        }

        result Result::Ok(())
    }
}

procedure main(args: [string])
    sequent { [fs::read, fs::write, alloc::heap] |- true => true }
{
    let config: Result<Config, string> = parse_args(args)

    let cfg: Config = match config {
        Result::Ok(c) => c,
        Result::Err(e) => {
            eprintln(e)
            system::exit(1)
        }
    }

    let result: Result<(), string> = process_file(cfg)

    match result {
        Result::Ok(_) => system::exit(0),
        Result::Err(e) => {
            eprintln("Error: {e}")
            system::exit(1)
        }
    }
}
```

### 0.11.2 Simple HTTP Server

```cursive
record Request {
    method: string@Owned,
    path: string@Owned,
    headers: Map<string, string>,
    body: Option<string@Owned>
}

record Response {
    status: u16,
    headers: Map<string, string>,
    body: string@Owned
}

procedure parse_request(raw: string) -> Result<Request, string>
    uses alloc.heap
{
    region parsing {
        let lines = alloc_in<parsing>(raw.split('\n'))

        if lines.len() == 0 {
            result Result::Err("Empty request")
        }

        // Parse request line
        let request_line = lines[0]
        let parts = alloc_in<parsing>(request_line.split(' '))

        if parts.len() < 2 {
            result Result::Err("Invalid request line")
        }

        let method = parts[0].to_owned()
        let path = parts[1].to_owned()

        // Parse headers
        var headers: Map<string, string> = Map::new()
        var i: usize = 1

        loop i < lines.len() {
            let line = lines[i]
            if line.is_empty() {
                break
            }

            let header_parts = alloc_in<parsing>(line.split(':'))
            if header_parts.len() >= 2 {
                let key = header_parts[0].trim().to_owned()
                let value = header_parts[1].trim().to_owned()
                headers.insert(key, value)
            }

            i = i + 1
        }

        // Parse body (if present)
        var body: Option<string@Owned> = Option::None
        if i + 1 < lines.len() {
            let body_lines = lines[i+1..]
            let body_str = body_lines.join('\n')
            body = Option::Some(body_str.to_owned())
        }

        result Result::Ok(Request {
            method: method,
            path: path,
            headers: headers,
            body: body
        })
    }
}

procedure handle_request(request: Request) -> Response
    uses alloc.heap
{
    // Route based on path
    let response_body: string = match request.path {
        "/" => "Welcome to Cursive HTTP Server!",
        "/api/status" => "{\"status\": \"ok\"}",
        _ => "404 Not Found"
    }

    let status: u16 = if request.path == "/" || request.path == "/api/status" {
        result 200
    } else {
        result 404
    }

    var response_headers: Map<string, string> = Map::new()
    response_headers.insert("Content-Type".to_owned(), "text/plain".to_owned())
    response_headers.insert("Content-Length".to_owned(), "{response_body.len()}".to_owned())

    result Response {
        status: status,
        headers: response_headers,
        body: response_body.to_owned()
    }
}

procedure format_response(response: Response) -> string
    uses alloc.heap
{
    var output: string@Owned = string::from("HTTP/1.1 {response.status} OK\r\n")

    loop (key, value): (string, string) in response.headers {
        output.append("{key}: {value}\r\n")
    }

    output.append("\r\n")
    output.append(response.body)

    result output
}

procedure serve()
    sequent { [net::listen, net::read, net::write, alloc::heap] |- true => true }
{
    let listener: TcpListener = TcpListener::bind("127.0.0.1:8080")

    println("Server listening on port 8080")

    loop {
        let connection: Result<TcpStream, string> = listener.accept()

        let stream: TcpStream = match connection {
            Result::Ok(s) => s,
            Result::Err(e) => {
                eprintln("Accept error: {e}")
                continue
            }
        }

        region request {
            // Read request
            var buffer: [u8; 4096] = [0; 4096]
            let bytes_read: Result<usize, string> = stream.read(mut buffer)

            let n: usize = match bytes_read {
                Result::Ok(num) => num,
                Result::Err(e) => {
                    eprintln("Read error: {e}")
                    continue
                }
            }

            let request_str = string::from_utf8(buffer[0..n])

            // Parse and handle
            let parsed: Result<Request, string> = parse_request(request_str)

            let request: Request = match parsed {
                Result::Ok(req) => req,
                Result::Err(e) => {
                    eprintln("Parse error: {e}")
                    continue
                }
            }

            let response: Response = handle_request(request)
            let response_str: string = format_response(response)

            // Send response
            let _write_result = stream.write(response_str.as_bytes())
        }
    }
}
```

### 0.11.3 Data Structure with Regions

```cursive
record TreeNode<T> {
    value: T,
    left: Option<own TreeNode<T>>,
    right: Option<own TreeNode<T>>
}

procedure build_tree<T>(values: [T]) -> Option<own TreeNode<T>>
    uses alloc.heap
{
    if values.len() == 0 {
        result Option::None
    }

    let mid: usize = values.len() / 2

    let left_tree: Option<own TreeNode<T>> = if mid > 0 {
        result build_tree(values[0..mid])
    } else {
        result Option::None
    }

    let right_tree: Option<own TreeNode<T>> = if mid + 1 < values.len() {
        result build_tree(values[mid+1..])
    } else {
        result Option::None
    }

    result Option::Some(TreeNode {
        value: values[mid],
        left: left_tree,
        right: right_tree
    })
}

procedure traverse<T>(node: Option<TreeNode<T>>) {
    match node {
        Option::Some(n) => {
            traverse(n.left)
            println("{n.value}")
            traverse(n.right)
        },
        Option::None => {}
    }
}

procedure tree_example()
    sequent { [alloc::heap, fs::write] |- true => true }
{
    let values: [i32; 7] = [1, 2, 3, 4, 5, 6, 7]

    let tree: Option<own TreeNode<i32>> = build_tree(values)

    println("Traversing tree:")
    traverse(tree)
}
```

---

## 0.12 Decision Trees for LLMs

### 0.12.1 When to Use Regions

```
START: Need to allocate memory?
  ↓
  Is the data long-lived (survives function return)?
    YES -> Use heap allocation (alloc.heap effect)
    NO -> Continue to next question
  ↓
  Is the data used in a loop?
    YES -> Use per-iteration region
    NO -> Continue to next question
  ↓
  Is the data part of request/batch processing?
    YES -> Use per-request/batch region
    NO -> Use heap allocation (simpler)

RESULT:
  - Heap allocation: Data::new() with alloc.heap
  - Loop region: region iter { alloc_in<iter>(...) }
  - Request region: region request { alloc_in<request>(...) }
```

### 0.12.2 Permission Selection

```
START: What permission should parameter have?
  ↓
  Does function consume the value?
    YES -> Use 'own T'
    NO -> Continue to next question
  ↓
  Does function modify the value?
    YES -> Use 'mut T'
    NO -> Use 'imm T' (or plain 'T', which defaults to imm)

SPECIAL CASES:
  - Function needs both read and write -> 'mut T'
  - Function transfers ownership -> 'own T'
  - Function only observes -> 'imm T' or 'T'
  - Multiple functions need access -> 'imm T' (can copy references)
```

### 0.12.3 Type Annotation Requirements

```
START: Do I need a type annotation?
  ↓
  Is this a block expression result?
    YES -> Annotation required on binding: let x: T = { ... }
    NO -> Continue
  ↓
  Is this a match expression result bound to variable?
    YES -> Annotation required on binding: let x: T = match { ... }
    NO -> Continue
  ↓
  Is this a pipeline stage?
    YES -> Annotation required: => stage: T
    NO -> Continue
  ↓
  Is this a loop iterator?
    YES -> Annotation required: loop item: T in items
    NO -> Continue
  ↓
  Is this a function parameter?
    YES -> Annotation always required: procedure f(x: T)
    NO -> Continue
  ↓
  Is this a function return?
    YES -> Annotation always required: procedure f() -> T
    NO -> Continue
  ↓
  Annotation recommended but may be inferred

REMEMBER: When in doubt, add annotation (explicit over implicit)
```

### 0.12.4 Grant Selection

```
START: What grants does my procedure need?
  ↓
  Does it read files?
    YES -> Add 'fs::read' to grant context [...]
  ↓
  Does it write to files?
    YES -> Add 'fs::write' to grant context [...]
  ↓
  Does it create network connections?
    YES -> Add 'net::connect' to grant context [...]
  ↓
  Does it read/write network sockets?
    YES -> Add 'net::read' and/or 'net::write' to grant context [...]
  ↓
  Does it allocate on heap (String, Vec, etc.)?
    YES -> Add 'alloc::heap' to grant context [...]
  ↓
  Does it call foreign functions?
    YES -> Add 'ffi::call' to grant context [...]
  ↓
  Does it use raw pointers?
    YES -> Add 'unsafe::ptr' to grant context [...]
  ↓
  Does it call other procedures?
    YES -> Add all grants from called procedures
  ↓
  RESULT: sequent { [union of all grants] |- P => Q }
```

### 0.12.5 Modal State Usage

```
START: Should I use a modal type?
  ↓
  Does my type have distinct states?
    NO -> Use regular record/enum
    YES -> Continue
  ↓
  Are some operations only valid in certain states?
    NO -> Use regular record/enum with runtime checks
    YES -> Continue
  ↓
  Can the type be in multiple states simultaneously?
    YES -> Use regular enum (not modal)
    NO -> Continue
  ↓
  Are state transitions explicit and important?
    YES -> Use modal type
    NO -> Consider modal type, but runtime checks may suffice

EXAMPLES OF GOOD MODAL CANDIDATES:
  - File (Closed/Open)
  - Network connection (Disconnected/Connected)
  - Builder (Empty/Partial/Complete)
  - Protocol state machine

EXAMPLES OF BAD MODAL CANDIDATES:
  - Simple data with optional fields
  - State that changes frequently and arbitrarily
  - Types where state is mainly for documentation
```

### 0.12.6 Error Handling Choice

```
START: How should I handle errors?
  ↓
  Is the error recoverable?
    NO -> Use panic! or assertion
    YES -> Continue
  ↓
  Is the error expected/common?
    YES -> Use Result<T, E>
    NO -> Continue
  ↓
  Is it a missing value (not an error)?
    YES -> Use Option<T>
    NO -> Use Result<T, E>

ERROR PROPAGATION:
  - ❌ NEVER use ? operator (doesn't exist)
  - ✅ ALWAYS use explicit match
  - ✅ Consider labeled blocks for early exit
```

---

## 0.13 Common Mistakes and How to Avoid Them

### 0.13.1 Mistake: Using Rust Syntax

**Wrong:**

```cursive
let mut counter = 0
let result = value?;
```

**Correct:**

```cursive
var counter: i32 = 0
let result: T = match value {
    Result::Ok(v) => v,
    Result::Err(e) => return Result::Err(e)
}
```

**How to Avoid:**

- Memorize: `var` for mutable, NOT `let mut`
- Memorize: NO `?` operator in Cursive
- Use explicit match for error propagation

### 0.13.2 Mistake: Missing Required Annotations

**Wrong:**

```cursive
let x = {
    let a = 10
    a + 20  // Missing 'result'
}

loop item in items {  // Missing type annotation
    process(item)
}
```

**Correct:**

```cursive
let x: i32 = {
    let a = 10
    result a + 20  // Explicit result
}

loop item: Item in items {  // Type annotated
    process(item)
}
```

**How to Avoid:**

- Always use `result` in block expressions
- Always annotate loop iterators
- Always annotate match bindings
- Always annotate pipeline stages

### 0.13.3 Mistake: Forgetting Effect Declarations

**Wrong:**

```cursive
procedure process() {  // Missing 'uses' clause
    let data = file::read("input.txt")  // Needs io.read
}
```

**Correct:**

```cursive
procedure process()
    sequent { [fs::read, alloc::heap] |- true => true }
{
    let data: string = file::read("input.txt")
}
```

**How to Avoid:**

- Think: "What side effects does my code have?"
- Check: File I/O -> io.read/io.write
- Check: Allocation -> alloc.heap
- Check: Network -> io.connect/io.listen
- Include effects from all called procedures

### 0.13.4 Mistake: Trying to Escape Regions

**Wrong:**

```cursive
procedure get_data() -> Data {
    region temp {
        let data = alloc_in<temp>(Data::new())
        result data  // ERROR: Cannot escape region
    }
}
```

**Correct:**

```cursive
// Option 1: Allocate on heap
procedure get_data() -> Data
    sequent { [alloc::heap] |- true => true }
{
    let data = Data::new()  // Heap allocation
    result data
}

// Option 2: Copy out if Data implements Copy
procedure get_data() -> Data {
    region temp {
        let data = alloc_in<temp>(Data::new())
        result data.copy()  // Copy escapes, not original
    }
}
```

**How to Avoid:**

- Region-allocated values CANNOT leave their region
- Use heap allocation for data that must escape
- Use regions only for temporary data
- Copy values if you need them outside region

### 0.13.5 Mistake: Implicit Conversions

**Wrong:**

```cursive
let x: i64 = 42_i32  // No implicit widening
let y: f64 = 3.14_f32  // No implicit conversion
```

**Correct:**

```cursive
let x: i64 = 42_i32.to_i64()  // Explicit conversion
let y: f64 = 3.14_f32.to_f64()  // Explicit conversion
```

**How to Avoid:**

- Cursive has NO implicit numeric conversions
- Exception: string@Owned -> string@View (modal coercion)
- Always use explicit .to\_\* methods for conversions

### 0.13.6 Mistake: Wrong Permission for Operation

**Wrong:**

```cursive
procedure modify(data: Data) {  // Defaults to imm
    data.field = new_value  // ERROR: Cannot mutate immutable
}
```

**Correct:**

```cursive
procedure modify(data: mut Data) {  // Mutable permission
    data.field = new_value  // OK: Can mutate
}
```

**How to Avoid:**

- Need to mutate -> Use `mut T`
- Need to consume -> Use `own T`
- Just reading -> Use `imm T` or `T`

### 0.13.7 Mistake: Enum Variant Syntax

**Wrong:**

```cursive
Option.Some(42)  // Wrong: dot syntax
Result.Ok(value)  // Wrong: dot syntax
```

**Correct:**

```cursive
Option::Some(42)  // Correct: :: syntax
Result::Ok(value)  // Correct: :: syntax
```

**How to Avoid:**

- ALWAYS use `::` for enum variants
- NEVER use `.` for enum variants (that's Rust/other languages)

---

## 0.14 LLM Code Generation Checklist

Use this checklist when generating Cursive code:

### Syntax Correctness

- [ ] All mutable bindings use `var`, NEVER `let mut`
- [ ] All block expressions use `result` keyword
- [ ] All match bindings have type annotations
- [ ] All pipeline stages have type annotations
- [ ] All loop iterators have type annotations
- [ ] All enum variants use `::` not `.`
- [ ] No `?` operator anywhere (use explicit match)

### Permission Annotations

- [ ] Functions consuming values use `own T` parameters
- [ ] Functions modifying values use `mut T` parameters
- [ ] Functions only reading use `imm T` or `T` parameters
- [ ] Move operations use explicit `move` keyword

### Grant Declarations

- [ ] File read has `[fs::read]` in grant context
- [ ] File write has `[fs::write]` in grant context
- [ ] Heap allocation has `[alloc::heap]` in grant context
- [ ] Network connections have `[net::connect]` in grant context
- [ ] Network listen has `[net::listen]` in grant context
- [ ] FFI calls have `[ffi::call]` in grant context
- [ ] Unsafe operations have `[unsafe::ptr]` in grant context
- [ ] All callee grants included in caller

### Region Usage

- [ ] Loop allocations use per-iteration regions
- [ ] Batch processing uses per-batch regions
- [ ] No region-allocated values returned from regions
- [ ] Region syntax correct: `region name { ... }`

### Type Annotations

- [ ] All function parameters have types
- [ ] All function returns have types
- [ ] All block expression bindings have types
- [ ] All match result bindings have types
- [ ] Pipeline stages have types

### Modal Types

- [ ] State transitions follow modal declaration
- [ ] Cannot call procedures unavailable in current state
- [ ] State annotations present: `Type@State`

### Error Handling

- [ ] Use `Result<T, E>` for recoverable errors
- [ ] Use `Option<T>` for missing values
- [ ] NO `?` operator (use explicit match)
- [ ] Early exit with labeled blocks when appropriate

### General Quality

- [ ] Code follows "explicit over implicit" principle
- [ ] All types are clear and unambiguous
- [ ] Effect requirements are visible
- [ ] Permission intent is clear

---

## 0.15 Troubleshooting Guide

### Error: "Cannot use value after move"

**Problem:**

```cursive
let data: own Data = Data::new()
consume(move data)
println(data)  // ERROR: data moved
```

**Solution:**

```cursive
// Option 1: Don't use after move
let data: own Data = Data::new()
consume(move data)
// Don't access data after this

// Option 2: Copy before move (if Data implements Copy)
let data: own Data = Data::new()
consume(move data.copy())
println(data)  // OK: original not moved

// Option 3: Use reference instead
let data: own Data = Data::new()
read(data)  // Passes immutable reference, not ownership
println(data)  // OK: still owns data
```

### Error: "Grant not in context"

**Problem:**

```cursive
procedure process() {  // Missing grant context
    let data = file::read("input.txt")  // Needs fs::read
}
```

**Solution:**

```cursive
procedure process()
    sequent { [fs::read, alloc::heap] |- true => true }
{
    let data: string = file::read("input.txt")
}
```

### Error: "Cannot escape region"

**Problem:**

```cursive
procedure get_data() -> Data {
    region temp {
        let data = alloc_in<temp>(Data::new())
        result data  // ERROR
    }
}
```

**Solution:**

```cursive
// Use heap allocation instead
procedure get_data() -> Data
    sequent { [alloc::heap] |- true => true }
{
    let data = Data::new()
    result data
}
```

### Error: "Type annotation required"

**Problem:**

```cursive
let x = { result 42 }  // ERROR: Type required
```

**Solution:**

```cursive
let x: i32 = { result 42 }  // OK: Type annotated
```

### Error: "Cannot mutate immutable value"

**Problem:**

```cursive
procedure modify(x: Data) {
    x.field = new_value  // ERROR: x is immutable
}
```

**Solution:**

```cursive
procedure modify(x: mut Data) {
    x.field = new_value  // OK: x is mutable
}
```

### Error: "Block missing result keyword"

**Problem:**

```cursive
let x: i32 = {
    let a = 10
    a + 20  // ERROR: Missing result
}
```

**Solution:**

```cursive
let x: i32 = {
    let a = 10
    result a + 20  // OK: result keyword
}
```

---

## 0.16 Advanced Topics

### 0.16.1 Effect Polymorphism

**Effect polymorphism allows procedures to be generic over effects:**

```cursive
procedure with_logging<E>(action: () -> () ! E)
    sequent { [fs::write, E] |- true => true }
{
    println("Starting action")
    action()  // May have effect E
    println("Action complete")
}

// Usage with different effects
procedure read_file()
    sequent { [fs::read] |- true => true }
{
    // Implementation
}

procedure write_file()
    sequent { [fs::write] |- true => true }
{
    // Implementation
}

procedure main()
    sequent { [fs::read, fs::write] |- true => true }
{
    with_logging(read_file)   // E = {io.read}
    with_logging(write_file)  // E = {io.write}
}
```

### 0.16.2 Generic Modal Types

**Modal types can be generic:**

```cursive
modal Container<T> {
    @Empty { }

    @Filled {
        value: T
    }

    procedure @Empty -> @Filled
        fill(self: Container<T>@Empty, v: T) -> Container<T>@Filled
    {
        result Container<T>@Filled { value: v }
    }

    procedure @Filled -> @Empty
        empty(self: Container<T>@Filled) -> (T, Container<T>@Empty)
    {
        result (self.value, Container<T>@Empty {})
    }
}
```

### 0.16.3 Higher-Order Functions with Effects

**Functions can take and return functions with effects:**

```cursive
procedure compose<A, B, C, E1, E2>(
    f: (A) -> B ! E1,
    g: (B) -> C ! E2
) -> (A) -> C ! (E1 ∪ E2) {
    result closure(x: A) -> C uses E1, E2 {
        let intermediate: B = f(x)
        result g(intermediate)
    }
}
```

### 0.16.4 Comptime Evaluation

**Some expressions can be evaluated at compile time:**

```cursive
comptime {
    let x: i32 = 10 + 20  // Evaluated at compile time
    let y: i32 = x * 2    // Also compile time
}

// Generate code at compile time
comptime procedure generate_array() -> [i32; 5] {
    var arr: [i32; 5] = [0; 5]
    var i: usize = 0
    loop i < 5 {
        arr[i] = i * i
        i = i + 1
    }
    result arr
}

let squares: [i32; 5] = generate_array()  // Computed at compile time
```

---

## 0.17 Comparison with Other Languages

### 0.17.1 Cursive vs. Rust

| Feature           | Rust                                | Cursive                              |
| ----------------- | ----------------------------------- | ------------------------------------ |
| Mutable binding   | `let mut x`                         | `var x: T`                           |
| Ownership         | Implicit                            | Explicit with `own`                  |
| Borrowing         | `&T`, `&mut T` with exclusive rules | `imm T`, `mut T` without exclusivity |
| Lifetimes         | `'a` annotations in types           | Regions, no annotations in types     |
| Error propagation | `?` operator                        | Explicit `match`                     |
| Block returns     | Implicit last expression            | Explicit `result` keyword            |
| Function types    | `Fn`, `FnMut`, `FnOnce` predicates  | `(T) -> U ! E` with effects          |
| Effects           | Implicit in predicates              | Explicit `uses` clause               |
| State machines    | Enums with runtime checks           | Modal types with compile-time checks |

### 0.17.2 Cursive vs. C++

| Feature           | C++                | Cursive                 |
| ----------------- | ------------------ | ----------------------- |
| Memory management | Manual or RAII     | Regions + ownership     |
| Memory safety     | Runtime            | Compile-time            |
| Null pointers     | Allowed            | `Option<T>` instead     |
| Move semantics    | Implicit with `&&` | Explicit `move` keyword |
| Templates         | `template<T>`      | Generics with contracts |
| Side effects      | Invisible          | Explicit `uses` clause  |
| Destructors       | RAII               | `defer` + regions       |

### 0.17.3 Cursive vs. Go

| Feature           | Go                   | Cursive                              |
| ----------------- | -------------------- | ------------------------------------ |
| Memory management | Garbage collected    | Regions + ownership                  |
| Concurrency       | Goroutines (runtime) | Effects + permissions (compile-time) |
| Error handling    | Multiple returns     | `Result<T, E>`                       |
| Interfaces        | Implicit             | Contracts (explicit)                 |
| Type system       | Simple               | Rich (modals, effects, permissions)  |
| Performance       | GC pauses            | Deterministic                        |

---

## 0.18 Quick Syntax Reference Card

### Declarations

```cursive
// Variables
let x: i32 = 42                    // Immutable
var y: i32 = 0                     // Mutable

// Functions
function pure(x: i32) -> i32 {     // Pure function
    result x * 2
}

procedure effectful()
    sequent { [fs::write] |- true => true }  // Procedure with grants
{
    println("hello")
}

// Records
record Point { x: f64, y: f64 }

// Enums
enum Option<T> {
    Some(T),
    None
}

// Modal Types
modal File {
    @Closed { path: string }
    @Open { path: string, handle: Handle }

    procedure @Closed -> @Open
        open(self: File@Closed) -> File@Open
        uses io.open
}
```

### Expressions

```cursive
// Blocks
let x: i32 = {
    result 42
}

// If
let y: i32 = if condition {
    result 1
} else {
    result 0
}

// Match
let msg: string = match opt {
    Option::Some(v) => "found",
    Option::None => "not found"
}

// Loops
loop item: T in items {
    process(item)
}

// Pipelines
let result: T = input
    => stage1: T1
    => stage2: T2
    => stage3: T
```

### Permissions

```cursive
own T       // Ownership
mut T       // Mutable access
imm T       // Immutable access (default)
move x      // Explicit move
```

### Effects

```cursive
uses io.read           // Read files/sockets
uses io.write          // Write files/sockets
uses alloc.heap        // Heap allocation
uses ffi.call          // Foreign functions
uses unsafe.ptr        // Raw pointers
```

### Regions

```cursive
region name {
    let x = alloc_in<name>(Data::new())
    // Use x
} // x freed here
```

---

## 0.19 Complete Example Programs

### 0.19.1 Text Processing Tool

```cursive
record Options {
    input: string@Owned,
    output: string@Owned,
    mode: ProcessingMode
}

enum ProcessingMode {
    Uppercase,
    Lowercase,
    Reverse
}

procedure parse_args(args: [string]) -> Result<Options, string>
    uses alloc.heap
{
    if args.len() < 4 {
        result Result::Err("Usage: program <input> <output> <mode>")
    }

    let mode: ProcessingMode = match args[3] {
        "upper" => ProcessingMode::Uppercase,
        "lower" => ProcessingMode::Lowercase,
        "reverse" => ProcessingMode::Reverse,
        _ => return Result::Err("Invalid mode")
    }

    result Result::Ok(Options {
        input: args[1].to_owned(),
        output: args[2].to_owned(),
        mode: mode
    })
}

procedure process_line(line: string, mode: ProcessingMode) -> string@Owned
    uses alloc.heap
{
    result match mode {
        ProcessingMode::Uppercase => line.to_uppercase(),
        ProcessingMode::Lowercase => line.to_lowercase(),
        ProcessingMode::Reverse => line.reverse()
    }
}

procedure process_file(opts: Options) -> Result<(), string>
    uses io.read, io.write, alloc.heap
{
    result 'process: {
        // Read entire file
        let content: Result<string, string> = file::read(opts.input)
        let text: string = match content {
            Result::Ok(s) => s,
            Result::Err(e) => break 'process Result::Err("Read error: {e}")
        }

        // Process line by line
        region processing {
            let lines = alloc_in<processing>(text.split('\n'))
            var processed_lines: Vec<string> = Vec::new()

            loop line: string in lines {
                let processed = process_line(line, opts.mode)
                processed_lines.push(processed)
            }

            let output = processed_lines.join('\n')

            // Write result
            let write_result: Result<(), string> = file::write(opts.output, output)
            match write_result {
                Result::Ok(_) => result Result::Ok(()),
                Result::Err(e) => break 'process Result::Err("Write error: {e}")
            }
        }
    }
}

procedure main(args: [string])
    sequent { [fs::read, fs::write, alloc::heap] |- true => true }
{
    let options: Result<Options, string> = parse_args(args)

    let opts: Options = match options {
        Result::Ok(o) => o,
        Result::Err(e) => {
            eprintln("Error: {e}")
            system::exit(1)
        }
    }

    let result: Result<(), string> = process_file(opts)

    match result {
        Result::Ok(_) => {
            println("Processing complete")
            system::exit(0)
        },
        Result::Err(e) => {
            eprintln("Processing failed: {e}")
            system::exit(1)
        }
    }
}
```

---

## 0.20 Final Reminders for LLMs

### Critical Rules (Never Violate)

1. **NEVER use `let mut`** - Always use `var` for mutable bindings
2. **NEVER use `?` operator** - Always use explicit `match`
3. **NEVER omit `result` in blocks** - Block expressions require it
4. **NEVER forget type annotations** - Required for match bindings, pipeline stages, loop iterators
5. **NEVER forget `uses` clauses** - All effects must be declared
6. **NEVER use `.` for enum variants** - Always use `::`
7. **NEVER return region-allocated values** - They cannot escape their region

### Best Practices (Always Follow)

1. **DO annotate types explicitly** - Embrace "explicit over implicit"
2. **DO declare all effects** - Make capabilities visible
3. **DO use regions for temporary data** - Better performance
4. **DO use modal types for state machines** - Compile-time safety
5. **DO use contracts** - Document requirements and guarantees
6. **DO check permissions** - `own` for consume, `mut` for modify, `imm` for read

### When in Doubt

1. **Add more annotations** - Explicit is better than implicit
2. **Declare more effects** - Better to over-declare than under-declare
3. **Use heap allocation** - Regions are an optimization, heap is simpler
4. **Use explicit match** - Clearer than trying to be clever
5. **Copy the patterns** - Use examples from this document

### Success Metrics

**Your generated Cursive code is correct when:**

- ✅ All examples compile without syntax errors
- ✅ All effects are declared
- ✅ All permissions are appropriate
- ✅ All type annotations are present where required
- ✅ No Rust syntax (`let mut`, `?`)
- ✅ Follows "explicit over implicit" principle

---

**Document Status:** Complete and comprehensive for LLM onboarding  
**Target Audience:** Large Language Models, AI coding assistants  
**Usage:** Keep this entire document in context when generating Cursive code  
**Version:** 2.0 (Enhanced)  
**Maintenance:** Update when language changes

---

**End of Enhanced LLM Onboarding Guide**
