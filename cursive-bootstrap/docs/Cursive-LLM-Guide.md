# Cursive Language Guide for LLMs

This document enables LLMs to generate correct Cursive code. All content is verified against the Cursive0.md specification.

---

## Critical Rules

1. **CURSIVE IS NOT RUST** - Do not use Rust syntax, semantics, or idioms
2. **No single-line trailing commas** (trailing commas only allowed when closing delimiter is on a different line)
3. **Method calls use `~>`**, field/tuple access uses `.`
4. **Procedures require explicit return type**; non-unit procedures must end with `return <expr>`
5. **Statement terminators**: Both `;` and newline are valid terminators
6. **Generic params use semicolons** `<T; U>`, **generic args use commas** `<T, U>`
7. **Static verification is default** for contracts; `[[dynamic]]` required for runtime checks
8. **Union types are unordered**: `A | B` is equivalent to `B | A`
9. **No subtyping between numeric types**: `i32` is NOT a subtype of `i64`

---

## 1. Quick Reference

### Declaration Keywords
```
procedure  record  enum  modal  class  type  static  using  import  extern
```

### Control Flow
```
if  else  match  loop  break  continue  return  defer
```

### Memory/Safety
```
region  frame  unsafe  move  widen  transmute
```

### Concurrency
```
spawn  wait  yield  sync  race  all  parallel  dispatch
```

### Permissions
```
const  unique  shared
```

### Binding Operators
- `=` creates a movable binding
- `:=` creates an immovable binding

### Type Constructors
- Tuples: `(T1, T2)`, single-element: `(T;)`
- Arrays: `[T; n]`
- Slices: `[T]`
- Unions: `T1 | T2`
- Function types: `(T1, T2) -> R`
- Safe pointers: `Ptr<T>@Valid`, `Ptr<T>@Null`, `Ptr<T>@Expired`
- Raw pointers: `*imm T`, `*mut T`
- String/Bytes: `string@View`, `string@Managed`, `bytes@View`, `bytes@Managed`
- Dynamic types: `$ClassName`
- Opaque types: `opaque TypePath`
- Refinement types: `T where { predicate }`

---

## 2. Lexical Rules

### 2.1. Comments
```cursive
// Line comment
/* Block comment (can be /* nested */) */

/// Documentation comment (for items)
//! Module documentation comment
```

### 2.2. Reserved Keywords
```
all  as  break  class  continue  dispatch  else  enum  false  defer  frame  from
if  imm  import  internal  let  loop  match  modal  move  mut  null  parallel
private  procedure  protected  public  race  record  region  return  shadow
shared  spawn  sync  transition  transmute  true  type  unique  unsafe  var
widen  where  using  yield  const  override
```

### 2.3. Reserved Namespaces and Prefixes
- `cursive`, `cursive::*`, `std` - reserved namespace prefixes
- `gen_` - reserved identifier prefix
- Do not shadow: `Self`, `Drop`, `Bitcopy`, `Clone`, `Eq`, `Hash`, `Hasher`, `Iterator`, `Step`, `FfiSafe`, `string`, `bytes`, `Modal`, `Region`, `RegionOptions`, `CancelToken`, `Context`, `System`, `ExecutionDomain`, `CpuSet`, `Priority`, `Reactor`

### 2.4. Contextual Keywords
These are valid identifiers except in specific syntactic positions:
- `in` - loop iteration context (`loop x in range`)
- `key` - dispatch key clause (`dispatch i in range key path mode`)
- `wait` - wait expression context

### 2.5. Unicode Restrictions (LexSecure)

**CRITICAL**: The following Unicode characters are **disallowed outside `unsafe { }`**:
- Bidirectional controls: U+200E, U+200F, U+202A-U+202E
- Zero-width joiners: U+200C, U+200D

Identifiers are NFC-normalized for equality comparisons.

### 2.6. Escape Sequences
```
\n    newline
\r    carriage return
\t    tab
\\    backslash
\"    double quote
\'    single quote
\0    null character
\xHH  hex byte (two hex digits)
\u{H+} unicode codepoint (1+ hex digits)
```

### 2.7. Statement Termination
Statements require a terminator (`;` or newline). Use newlines by default.

```cursive
let x: i32 = 10       // newline terminates
let y: i32 = 20;      // semicolon terminates (same line allowed)
```

### 2.8. Trailing Commas
Trailing commas are **only allowed** when the closing delimiter is on a **different line** than the comma. On a single line, trailing commas are **not allowed**.

```cursive
// CORRECT - single line, no trailing comma:
f(a, b)
Point{ x: 1, y: 2 }

// CORRECT - multi-line, trailing comma allowed:
Point{
    x: 1,
    y: 2,
}

// WRONG - single-line trailing comma:
f(a, b,)
Point{ x: 1, y: 2, }
```

**Recommendation**: Avoid trailing commas entirely to be safe.

### 2.9. Match Arms Are Comma-Separated
Match arms use commas as separators (not newlines alone):
```cursive
match value {
    0 => { "zero" },
    1 => { "one" },
    _: i32 => { "other" }
}
```

### 2.10. Unsupported Constructs (Cursive0)

The following constructs are **not supported** in Cursive0. Do not generate code using these:

| Construct | Category | Status |
|-----------|----------|--------|
| `metaprogramming` | Grammar | Unsupported in Cursive0 |
| `closure` | Feature | Use named procedures instead |
| `pipeline` | Feature | Not available |
| `Network` | Capability | Not provided in Context |
| `GPUFactory` | Capability | Not provided in Context |
| `CPUFactory` | Capability | Not provided in Context |

```cursive
// WRONG - closures not supported
let f = |x| x + 1

// CORRECT - use named procedure
procedure add_one(x: i32) -> i32 {
    return x + 1
}
```

---

## 3. Types

### 3.1. Primitive Types

| Category | Types |
|----------|-------|
| Signed integers | `i8`, `i16`, `i32`, `i64`, `i128`, `isize` |
| Unsigned integers | `u8`, `u16`, `u32`, `u64`, `u128`, `usize` |
| Floating point | `f16`, `f32`, `f64` |
| Boolean | `bool` |
| Character | `char` |
| Unit | `()` |
| Never | `!` |

### 3.2. Compound Types

**Tuples:**
```cursive
let pair: (i32, bool) = (42, true)
let single: (i32;) = (42;)           // Single-element tuple requires semicolon
let unit: () = ()
```

**Arrays:**
```cursive
let arr: [i32; 3] = [1, 2, 3]
```

**Slices:**
```cursive
let slice: [i32] = arr[0..2]
```

**Unions (unordered - `A | B` is equivalent to `B | A`):**
```cursive
let result: i32 | bool = 42
let result2: i32 | bool = true
```

**Function Types:**
```cursive
let f: (i32, i32) -> i32 = add
let consuming: (move i32) -> () = consume
```

### 3.3. Permission Qualifiers

Permissions form a lattice with subtyping: `unique` <: `shared` <: `const`

```cursive
let x: const i32 = 10       // Read-only access
let y: unique i32 = 20      // Exclusive mutable access
let z: shared i32 = 30      // Synchronized shared access (requires key system)
```

**Permission Operations:**

| Permission | Read | Write | Aliasing |
|------------|------|-------|----------|
| `const` | Yes | No | Unlimited |
| `unique` | Yes | Yes | No aliases |
| `shared` | Yes (with key) | Yes (with key) | Multiple shared |

**Method Receiver Compatibility:**

| Caller Permission | May Call `~` | May Call `~%` | May Call `~!` |
|-------------------|--------------|---------------|---------------|
| `const` | Yes | No | No |
| `shared` | Yes | Yes | No |
| `unique` | Yes | Yes | Yes |

### 3.4. Pointer Types

**Safe Pointers (`Ptr<T>`):**
```cursive
let p: Ptr<i32>@Valid = &x      // Valid pointer
let q: Ptr<i32>@Null = Ptr::null()  // Null pointer (requires type annotation)
// @Expired - pointer to deallocated memory (internal state)
```

**Raw Pointers:**
```cursive
let rp: *imm u8 = null    // Immutable raw pointer (requires type annotation)
let mp: *mut u8 = null    // Mutable raw pointer
```

### 3.5. String and Bytes Types

```cursive
let view: string@View = "hello"           // String view (borrowed)
let managed: string@Managed = ...         // Heap-allocated string

let bview: bytes@View = ...               // Bytes view (borrowed)
let bmanaged: bytes@Managed = ...         // Heap-allocated bytes
```

### 3.6. Dynamic/Capability Types

```cursive
let fs: $FileSystem = ctx.fs             // FileSystem capability
let heap: $HeapAllocator = ctx.heap      // Heap allocator capability
let domain: $ExecutionDomain = ctx.cpu() // Execution domain
```

### 3.7. Modal State Types

```cursive
let conn: Connection@Open = ...          // Modal type in specific state
let conn2: Connection@Closed = ...
```

### 3.8. Type Equivalence and Subtyping

**CRITICAL RULES:**

1. **Union types are unordered**: `i32 | bool` is equivalent to `bool | i32`
2. **No subtyping between numeric types**: `i32` is NOT a subtype of `i64`
3. **Permission subtyping**: `unique T <: shared T <: const T`
4. **Array types require both element type AND length to match**
5. **Modal state types must have the same state to be equivalent**

---

## 4. Declarations

### 4.1. Procedures

**Syntax:**
```cursive
visibility? "procedure" name generic_params? "(" param_list? ")" "->" type contract? block
```

**Always specify return type.** Non-unit procedures must end with `return <expr>`.

```cursive
// Unit return type
public procedure greet() -> () {
    // No return needed for ()
}

// Non-unit return - MUST end with return
public procedure add(a: i32, b: i32) -> i32 {
    return a + b
}

// With move parameter
procedure consume(move data: unique Buffer) -> () {
    // data is consumed here
}

// main signature for executables
public procedure main(move ctx: Context) -> i32 {
    return 0
}
```

### 4.2. Records

```cursive
public record Point {
    x: i32,
    y: i32,

    // Method with const receiver
    procedure distance(~) -> f64 {
        return ((self.x * self.x + self.y * self.y) as f64)~>sqrt()
    }

    // Method with unique receiver (can mutate)
    procedure translate(~!, dx: i32, dy: i32) -> () {
        self.x = self.x + dx
        self.y = self.y + dy
    }
}
```

**Receiver Shorthands:**
- `~` expands to `const Self`, binds `self` (read-only)
- `~!` expands to `unique Self`, binds `self` (mutable)
- `~%` expands to `shared Self`, binds `self` (synchronized)

### 4.3. Enums

```cursive
public enum Result {
    Ok(i32),
    Err(string@View)
}

public enum Direction {
    North,
    South,
    East,
    West
}

// With explicit discriminants
public enum Status {
    Active = 1,
    Inactive = 0
}
```

### 4.4. Modal Types

```cursive
public modal Connection {
    @Disconnected {
        host: string@View

        transition connect(port: i32) -> @Connected {
            // ... connection logic
            return Connection@Connected{ socket: ... }
        }
    }

    @Connected {
        socket: i32

        procedure send(~!, data: bytes@View) -> i32 {
            // ... send logic
            return bytes_sent
        }

        transition disconnect() -> @Disconnected {
            // ... cleanup
            return Connection@Disconnected{ host: self.host }
        }
    }
}
```

**State-Specific Fields**: Fields declared in a state block are only accessible when the modal is in that state.

### 4.5. Built-in Modal Types

The following modal types are built into Cursive:

**`Region`** - Memory arena lifecycle:
```cursive
States: @Active, @Frozen, @Freed
Methods: new_scoped, alloc, reset_unchecked, freeze, thaw, free_unchecked
```

**`CancelToken`** - Cooperative cancellation:
```cursive
States: @Active, @Cancelled
Methods:
  @Active: cancel(~%), is_cancelled(~), wait_cancelled(~), child(~)
  @Cancelled: is_cancelled(~)
```

**`Spawned<T>`** - Parallel task result:
```cursive
States: @Pending, @Ready
Fields:
  @Ready: value: T
```

**`Tracked<T, E>`** - Tracked async result:
```cursive
States: @Pending, @Ready
Fields:
  @Ready: value: T | E
```

**`Async<Out, In, Result, E>`** - Async state machine:
```cursive
States: @Suspended, @Completed, @Failed
Fields:
  @Suspended: output: Out
  @Completed: value: Result
  @Failed: error: E
Methods:
  @Suspended: resume(~!, input: In) -> Async@Suspended | Async@Completed | Async@Failed
```

**Async Type Aliases:**
```cursive
type Sequence<T> = Async<T, (), (), !>
type Future<T; E = !> = Async<(), (), T, E>
type Pipe<In; Out> = Async<Out, In, (), !>
type Exchange<T> = Async<T, T, T, !>
type Stream<T; E> = Async<T, (), (), E>
```

### 4.6. Classes

```cursive
public class Comparable {
    procedure compare(~, other: const Self) -> i32
}

public class Printable {
    procedure print(~) -> string@Managed
}

// Record implementing classes
public record MyType <: Comparable, Printable {
    value: i32,

    override procedure compare(~, other: const Self) -> i32 {
        return self.value - other.value
    }

    override procedure print(~) -> string@Managed {
        // ... implementation
    }
}
```

**Associated Types in Classes:**
```cursive
public class Container {
    type Item                    // Abstract associated type
    procedure get(~) -> Self::Item
}
```

### 4.7. Type Aliases

```cursive
public type Result = i32 | AllocationError
public type Callback = (i32) -> ()
```

### 4.7.1. Type Refinements

Types can have predicate refinements using `where { predicate }`:
```cursive
// In type alias
public type PositiveInt = i32 where { self > 0 }

// In parameter
procedure sqrt(x: f64 where { x >= 0.0f64 }) -> f64 {
    // x is guaranteed non-negative
    return ...
}

// In return type
procedure abs(x: i32) -> i32 where { self >= 0 } {
    return if x < 0 { -x } else { x }
}
```

### 4.8. Generics

**CRITICAL: Generic parameters vs arguments:**
- **Parameters** (in declarations): use **semicolons** `<T; U>`
- **Arguments** (when instantiating): use **commas** `<T, U>`

```cursive
// DECLARATION - semicolons separate type parameters
procedure swap<T; U>(a: T, b: U) -> (U, T) {
    return (b, a)
}

record Pair<T; U> {
    first: T,
    second: U
}

// INSTANTIATION - commas separate type arguments
let p: Pair<i32, bool> = Pair{ first: 1, second: true }
let result: (bool, i32) = swap<i32, bool>(10, true)
```

**Type bounds (class constraints) use `<:`:**
```cursive
procedure print_all<T <: Printable>(items: [T]) -> () {
    // T must implement Printable
}
```

**Type parameter defaults:**
```cursive
record Container<T; Alloc = DefaultAllocator> {
    data: T,
    alloc: Alloc
}
```

**Where clauses with predicates:**

Available predicates: `Bitcopy`, `Clone`, `Drop`, `FfiSafe`

```cursive
procedure copy_value<T>(x: T) -> T
    where Bitcopy(T)
{
    return x
}

procedure clone_pair<T; U>(p: Pair<T, U>) -> Pair<T, U>
    where Clone(T)
    Clone(U)
{
    return Pair{ first: p.first~>clone(), second: p.second~>clone() }
}
```

**IMPORTANT:** Where clause syntax uses `PredicateName(Type)`, NOT Rust-style `where T: Predicate`.

**Class constraint where clauses:**
```cursive
procedure compare<T>(a: T, b: T) -> i32
    where T <: Comparable
{
    return a~>compare(b)
}
```

### 4.9. Contracts (Preconditions/Postconditions)

Contracts specify procedure invariants using `|=`:
```cursive
procedure divide(a: i32, b: i32) -> i32
    |= b != 0
{
    return a / b
}

// With postcondition using @result
procedure abs(x: i32) -> i32
    |= => @result >= 0
{
    return if x < 0 { -x } else { x }
}

// Both precondition and postcondition
procedure clamp(x: i32, min: i32, max: i32) -> i32
    |= min <= max => @result >= min && @result <= max
{
    return if x < min { min } else if x > max { max } else { x }
}
```

**Contract Syntax:**
- `|= P` - Precondition only
- `|= P => Q` - Precondition and postcondition
- `|= => Q` - Postcondition only (precondition is `true`)

**Contract Intrinsics:**
- `@result` - references the return value in postconditions (right of `=>` only)
- `@entry(expr)` - captures the entry/old value of an expression

```cursive
// Using @entry to compare against initial value
procedure increment(~!) -> i32
    |= self.value >= 0 => @result > @entry(self.value)
{
    self.value = self.value + 1
    return self.value
}
```

**Purity Constraints (CRITICAL):**
Contract predicates MUST be pure:
1. MUST NOT invoke any procedure that accepts capability parameters
2. MUST NOT mutate state observable outside the expression's evaluation
3. Built-in operators on primitive types are always pure

**Evaluation Contexts:**
- **Precondition context**: Parameters at entry state only
- **Postcondition context**: Parameters at post-state, `@result`, `@entry()`, receiver

**@entry Requirements:**
- The result type of `@entry(expr)` MUST satisfy `BitcopyType` or `CloneType`

**Verification Modes (CRITICAL):**

**Static verification is REQUIRED by default.** If the contract cannot be statically proven, the program is ill-formed.

To allow runtime verification, use `[[dynamic]]`:
```cursive
[[dynamic]]
procedure user_input_divide(a: i32, b: i32) -> i32
    |= b != 0
{
    return a / b
}
```

### 4.10. Type Invariants

Types can have invariants using `where { predicate }`:
```cursive
public record PositiveCounter where { self.value > 0 } {
    value: i32,

    procedure increment(~!) -> () {
        self.value = self.value + 1
    }
}
```

**Enforcement Points:**
- After construction (constructor/literal initialization)
- Before any public procedure with receiver (Pre-Call)
- Before any procedure taking `unique self` returns (Post-Call)

**Constraint:** Types with type invariants MUST NOT declare public mutable fields.

### 4.11. Loop Invariants

Loops can have invariants using `where { predicate }`:
```cursive
loop i in 0..n where { i >= 0 && i <= n } {
    // Invariant checked at each iteration
    process(i)
}
```

**Enforcement Points:**
- Before first iteration (Initialization)
- At start of each subsequent iteration (Maintenance)
- After loop terminates (Termination)

### 4.12. Static Declarations

```cursive
public let MAX_SIZE: usize = 1024
public var counter: i32 = 0
```

### 4.13. Import Declarations

`import` brings a module into scope **as a namespace**:
```cursive
import mylib::networking              // Access as networking::foo()
import mylib::utils as u              // Access as u::foo()
```

**Note:** `import` is distinct from `using`:
- `import` creates a namespace alias for a module
- `using` brings items directly into scope

### 4.14. Using Declarations

`using` brings specific items or all items from a module:
```cursive
using cursive::runtime::string
using cursive::runtime::bytes
using mymodule::{foo, bar}
using othermodule::*
using somemodule::SomeType as AliasedType
```

### 4.15. Extern Blocks (FFI)

**Basic extern declaration:**
```cursive
extern "C" {
    procedure malloc(size: usize) -> *mut u8
    procedure free(ptr: *mut u8) -> ()
}
```

**Extern with foreign contracts:**
```cursive
extern "C" {
    procedure read_file(path: *imm u8, buf: *mut u8, len: usize) -> i32
        |= @foreign_assumes(path != null, buf != null, len > 0)
        |= @foreign_ensures(@result >= 0)
        |= @foreign_ensures(@error: @result < 0)

    procedure get_data() -> *imm u8
        |= @foreign_ensures(@null_result: @result == null)
}
```

**Foreign contract syntax:**
- `|= @foreign_assumes(pred, pred, ...)` - preconditions the foreign code expects
- `|= @foreign_ensures(pred)` - postconditions on success
- `|= @foreign_ensures(@error: pred)` - postconditions on error
- `|= @foreign_ensures(@null_result: pred)` - postconditions when result is null

**Note:** Calls to extern procedures require `unsafe { }` blocks.

**FfiSafe Types (CRITICAL):**

Only certain types can cross the FFI boundary:
- Primitive types: `i8`, `i16`, `i32`, `i64`, `i128`, `u8`, `u16`, `u32`, `u64`, `u128`, `isize`, `usize`, `f16`, `f32`, `f64`, `char`, `()`
- Raw pointers: `*imm T`, `*mut T`
- Arrays of FfiSafe types
- Function pointers with FfiSafe parameters/return
- Records/Enums with `[[layout(C)]]` attribute and all FfiSafe fields

**NOT FfiSafe (prohibited in FFI signatures):**
- `bool`
- Modal types
- Safe pointers `Ptr<T>`
- Dynamic class objects `$ClassName`
- Tuples, Unions, Slices
- String and bytes types
- `Context`

**RAII by-value rule:** If a type has `Drop` and is passed by value in FFI, it requires `[[ffi_pass_by_value]]` attribute.

**Capability Isolation:** Extern procedures CANNOT access Context capabilities directly. Capabilities must be passed explicitly.

### 4.16. Attributes

Attributes annotate declarations:
```cursive
[[inline]]
procedure fast_add(a: i32, b: i32) -> i32 {
    return a + b
}

[[export]]
public procedure lib_init() -> () {
    // ...
}

// Multiple attributes
[[inline, cold]]
procedure rarely_called() -> () {
    // ...
}
```

**Complete Attribute List:**

| Attribute | Purpose |
|-----------|---------|
| `[[inline]]` | Suggest inlining |
| `[[inline(always)]]` | Force inlining |
| `[[inline(never)]]` | Never inline |
| `[[inline(hint)]]` | Hint to inline |
| `[[cold]]` | Mark as rarely executed |
| `[[export]]` | Make callable from foreign code |
| `[[no_mangle]]` | Disable name mangling (FFI) |
| `[[symbol("name")]]` | Custom symbol name |
| `[[unwind]]` | Control unwinding at FFI boundary |
| `[[layout(C)]]` | C-compatible memory layout |
| `[[ffi_pass_by_value]]` | Allow by-value passing of Drop types in FFI |
| `[[weak]]` | Weak linkage |
| `[[dynamic]]` | Allow runtime contract/refinement verification |
| `[[static_dispatch_only]]` | Prevent dynamic dispatch for generic class methods |

### 4.17. Visibility Modifiers

| Modifier | Scope |
|----------|-------|
| `public` | Visible everywhere |
| `internal` | Visible within the assembly (default) |
| `private` | Visible only in declaring module |
| `protected` | Visible in declaring module and submodules |

```cursive
public procedure api_function() -> ()
internal procedure assembly_helper() -> ()
private procedure module_only() -> ()
```

---

## 5. Expressions

### 5.1. Method Calls vs Field Access

**Method calls use `~>`:**
```cursive
receiver~>method(arg1, arg2)
str~>length()
list~>push(item)
```

**Field/tuple access uses `.`:**
```cursive
point.x
point.y
tuple.0
tuple.1
```

### 5.2. Operator Precedence (highest to lowest)

| Level | Operators | Associativity |
|-------|-----------|---------------|
| 1 | Postfix: `.` `[]` `~>` `()` `?` | Left |
| 2 | Unary: `!` `-` `*` `&` `widen` | Right |
| 3 | `as` (cast) | Left |
| 4 | `**` (power) | Right |
| 5 | `*` `/` `%` | Left |
| 6 | `+` `-` | Left |
| 7 | `<<` `>>` | Left |
| 8 | `&` (bitwise) | Left |
| 9 | `^` (bitwise) | Left |
| 10 | `\|` (bitwise) | Left |
| 11 | `==` `!=` `<` `<=` `>` `>=` | Left |
| 12 | `&&` | Left |
| 13 | `\|\|` | Left |
| 14 | `..` `..=` (range) | - |
| 15 | `=` `:=` `+=` `-=` etc. | Right |

### 5.3. Literals

**CRITICAL: Float literals MUST have a suffix:**
```cursive
// CORRECT:
let f: f32 = 3.14f        // 'f' suffix defaults to f32
let g: f64 = 3.14f64      // explicit f64
let h: f16 = 3.14f16      // explicit f16
let i: f32 = 1.0e10f      // exponent with suffix

// WRONG - will not parse:
let bad = 3.14        // ERROR: no suffix
let bad2 = 1.0e10     // ERROR: no suffix
```

**Valid float suffixes:** `f` (defaults to f32), `f16`, `f32`, `f64`

**Integers:**
```cursive
let a: i32 = 42
let b: i64 = 42i64
let c: u8 = 0xFF_u8
let d: i32 = 0b1010
let e: i32 = 0o777
```

**Floats (suffix required):**
```cursive
let f: f32 = 3.14f       // 'f' defaults to f32
let g: f64 = 3.14f64     // explicit f64
let h: f16 = 1.5f16      // explicit f16
let i: f32 = 1.0e10f     // exponent notation
```

**Boolean:**
```cursive
let t: bool = true
let f: bool = false
```

**Character:**
```cursive
let ch: char = 'a'
let esc: char = '\n'
```

**String:**
```cursive
let s: string@View = "hello"
```

**Null (raw pointer only, requires type context):**
```cursive
let np: *imm u8 = null
```

### 5.4. Control Flow Expressions

**If Expression:**
```cursive
let x: i32 = if cond { 1 } else { 2 }

if cond {
    // ...
} else if other_cond {
    // ...
} else {
    // ...
}
```

**Match Expression (arms are comma-separated):**
```cursive
// Braces optional for simple expressions
let result: i32 = match value {
    0 => 100,                       // Simple expression, no braces
    1 => { complex_computation() }, // Block for multiple statements
    n: i32 => n * 10                // Last arm, no trailing comma
}

// With guards
match opt {
    x: i32 if x > 0 => x,
    _: i32 => 0,
    _: () => -1
}
```

**Loop Expressions:**
```cursive
// Infinite loop
loop {
    if done { break }
}

// Conditional loop
loop condition {
    // ...
}

// Iterator loop
loop x in 0..10 {
    // x is i32
}

// Loop with break value
let result: i32 = loop {
    if found {
        break 42
    }
}

// Loop with invariant
loop i in 0..n where { i >= 0 && i <= n } {
    // Invariant checked each iteration
    process(i)
}
```

### 5.5. Block Expressions

```cursive
let x: i32 = {
    let temp: i32 = compute()
    temp * 2
}
```

### 5.6. Allocation and Pointers

```cursive
// Region allocation
region {
    let p: Ptr<i32>@Valid = ^42    // Allocate in current region
}

// Address-of
var x: i32 = 10
let p: Ptr<unique i32>@Valid = &x

// Dereference
*p = 20
let v: i32 = *p
```

### 5.7. Move Expression

```cursive
let a: Buffer = create_buffer()
let b: Buffer = move a           // a is now invalid
consume(move b)                  // pass by move
```

### 5.8. Union Propagation (`?`)

```cursive
// Propagates non-success cases to caller
procedure read_file(path: string@View) -> string@Managed | IOError {
    let file: File = open(path)?    // Returns early if error
    let content: string@Managed = file~>read_all()?
    return content
}
```

### 5.9. Modal Widening (`widen`)

Converts a state-specific modal type to the general modal type:
```cursive
let conn: Connection@Connected = ...
let general: Connection = widen conn    // Now can be any state
```

### 5.10. Transmute (unsafe)

```cursive
unsafe {
    let bits: u32 = transmute<f32, u32>(3.14f)
}
```

---

## 6. Statements

### 6.1. Bindings

```cursive
let x: i32 = 10              // Immutable, movable
let y := 20                  // Immutable, immovable
var z: i32 = 30              // Mutable, movable
var w := 40                  // Mutable, immovable

// Shadow binding (reuses name)
shadow let x: i32 = x + 1
shadow var y: i32 = y * 2
```

### 6.2. Assignment

```cursive
var x: i32 = 10
x = 20                       // Simple assignment
x += 5                       // Compound: +=, -=, *=, /=, %=, &=, |=, ^=, <<=, >>=
```

### 6.3. Control Flow Statements

```cursive
return value                 // Return from procedure
return                       // Return unit

break                        // Exit loop
break value                  // Exit loop with value
continue                     // Next iteration
```

### 6.4. Defer

```cursive
procedure example() -> () {
    let handle: Handle = open()
    defer { handle~>close() }    // Runs when scope exits

    // ... use handle
}
```

### 6.5. Region and Frame

```cursive
// Create a region
region {
    let p: Ptr<i32>@Valid = ^42
    // p is valid within this region
}
// p is now Expired

// With options
region (RegionOptions{ size: 4096 }) {
    // ...
}

// Named region
region as r {
    let p: Ptr<i32>@Valid = r ^ 42    // Allocate in specific region
}

// Frame (subdivides region)
region {
    frame {
        let p: Ptr<i32>@Valid = ^1
    }
    // p deallocated, region still active
}
```

### 6.6. Unsafe Block

```cursive
unsafe {
    let rp: *mut i32 = raw_alloc(4)
    *rp = 42
}
```

### 6.7. Key Blocks (Synchronized Access)

Key blocks acquire locks for `shared` data access using `#`:

```cursive
// Basic key block
#shared_data {
    shared_data.value = shared_data.value + 1
}

// Multiple paths
#data1, data2 {
    // Access both atomically
}
```

**Key modes (read/write):**
```cursive
#data read {
    let x: i32 = data.value    // Read-only access
}

#data write {
    data.value = 42            // Write access (default)
}
```

**Key block modifiers:**
```cursive
#data dynamic {
    // Runtime key acquisition
}

#data speculative {
    // Speculative execution (may roll back)
}

#data release {
    // Release keys after block completes
}
```

**Available modifiers:**
| Modifier | Meaning |
|----------|---------|
| `dynamic` | Runtime key acquisition |
| `speculative` | Speculative execution that may roll back |
| `release` | Release held keys after block completes |

**Available modes:**
| Mode | Meaning |
|------|---------|
| `read` | Read-only access |
| `write` | Write access (default) |

**Key boundary fields:**
Fields declared with `#` prefix establish permanent key boundaries:
```cursive
public record SharedContainer {
    #data: shared DataStore,     // # creates key boundary
    metadata: i32                // No boundary
}

// Key acquisition stops at # boundary
#container.data write {
    // Acquires key for container.data only
    // Not for nested paths beyond the boundary
}
```

---

## 7. Patterns

### 7.1. Irrefutable Patterns (for let/var)

```cursive
let _: i32 = expr                         // Wildcard
let x: i32 = expr                         // Identifier
let (a, b): (i32, bool) = expr           // Tuple
let Point{ x, y }: Point = expr          // Record
```

### 7.2. Refutable Patterns (for match)

**Note: Match arms are comma-separated.**

```cursive
match value {
    42 => { 0 },                          // Literal
    x: i32 => { x },                      // Typed (binds x)
    (a: i32, b: bool) => { a },          // Tuple with types
    Point{ x, y } => { x + y },          // Record
    Result::Ok(v) => { v },              // Enum
    @Connected{ socket } => { socket },  // Modal state
    0..10 => { 1 },                       // Range (exclusive)
    0..=10 => { 2 },                      // Range (inclusive)
    _ => { -1 }                           // Wildcard (last arm, no trailing comma)
}
```

### 7.3. Pattern Guards

```cursive
match value {
    x: i32 if x > 0 => { x },
    x: i32 if x < 0 => { -x },
    _: i32 => { 0 }
}
```

---

## 8. Memory Model

### 8.1. Ownership

Every resource has exactly one responsible binding. The `move` keyword transfers responsibility.

```cursive
let a: Buffer = create()      // a is responsible
let b: Buffer = move a        // b is now responsible, a is invalid

procedure consume(move data: Buffer) -> () {
    // This procedure takes ownership
}

consume(move b)               // Caller transfers ownership
```

### 8.2. Binding Operators

| Syntax | Mutability | Movability |
|--------|------------|------------|
| `let x = v` | Immutable | Movable |
| `let x := v` | Immutable | Immovable |
| `var x = v` | Mutable | Movable |
| `var x := v` | Mutable | Immovable |

### 8.3. Binding States

Bindings have states tracked by the compiler:
- **Alive** - Valid and usable
- **Moved** - Transferred to another binding, no longer usable
- **PartiallyMoved** - Some fields moved (for records)
- **Poisoned** - Error state

### 8.4. Permissions

| Permission | Meaning |
|------------|---------|
| `const` | Read-only access |
| `unique` | Exclusive mutable access |
| `shared` | Thread-safe shared access (requires key acquisition) |

```cursive
procedure read_only(data: const Buffer) -> () { ... }
procedure modify(data: unique Buffer) -> () { ... }
procedure thread_safe(data: shared Buffer) -> () { ... }
```

---

## 9. Capabilities and Context

### 9.1. Context Record

The `Context` record provides capabilities to the program:

```cursive
public procedure main(move ctx: Context) -> i32 {
    let fs: $FileSystem = ctx.fs
    let heap: $HeapAllocator = ctx.heap
    return 0
}
```

### 9.2. Available Capabilities

| Capability | Type | Purpose |
|------------|------|---------|
| `ctx.fs` | `$FileSystem` | File system operations |
| `ctx.heap` | `$HeapAllocator` | Heap memory allocation |
| `ctx.cpu()` | `$ExecutionDomain` | CPU parallel execution domain |
| `ctx.gpu()` | `$ExecutionDomain` | GPU execution domain |
| `ctx.inline()` | `$ExecutionDomain` | Sequential inline execution |

### 9.3. FileSystem Capability

```cursive
let content: string@Managed | IOError = fs~>read_file(path, heap)
fs~>write_file(path, data)
let exists: bool = fs~>exists(path)
```

### 9.4. HeapAllocator Capability

```cursive
let managed: string@Managed | AllocationError = string::from(view, heap)
let buffer: bytes@Managed | AllocationError = bytes::with_capacity(1024, heap)
```

---

## 10. Structured Parallelism

### 10.1. Parallel Blocks

```cursive
parallel ctx.cpu() {
    spawn { compute_a() }
    spawn { compute_b() }
}
```

**Fork-Join Guarantee:** All spawned work completes before the parallel block exits.

**Parallel block options:**
```cursive
parallel ctx.cpu() [name: "main_work", cancel: cancel_token] {
    spawn { work_a() }
    spawn { work_b() }
}
```

| Option | Type | Effect |
|--------|------|--------|
| `name` | `string` | Labels block for debugging/profiling |
| `cancel` | `CancelToken` | Provides cooperative cancellation |

### 10.2. Execution Domains

| Domain | Method | Behavior |
|--------|--------|----------|
| CPU | `ctx.cpu()` | Parallel execution on OS threads |
| GPU | `ctx.gpu()` | Compute shader execution |
| Inline | `ctx.inline()` | Sequential execution (no actual parallelism) |

```cursive
// CPU parallel
parallel ctx.cpu() {
    spawn { compute() }
}

// GPU parallel
parallel ctx.gpu() {
    dispatch i in 0..1000 {
        gpu_compute(i)
    }
}

// Inline (sequential, for testing)
parallel ctx.inline() {
    spawn { work() }  // Executes immediately, blocks until complete
}
```

### 10.3. Spawn Expression

```cursive
parallel ctx.cpu() {
    // Basic spawn
    let handle: Spawned<i32> = spawn { expensive_computation() }

    // Spawn with options
    let handle2: Spawned<i32> = spawn [name: "worker", priority: High] {
        other_work()
    }

    let result: i32 = wait handle
}
```

**Spawn options:**
| Option | Type | Default | Effect |
|--------|------|---------|--------|
| `name` | `string` | Anonymous | Labels for debugging/profiling |
| `affinity` | `CpuSet` | Domain default | CPU core affinity hint |
| `priority` | `Priority` | `Normal` | Scheduling priority (`Low`, `Normal`, `High`) |

### 10.4. Wait Expression

```cursive
let handle: Spawned<i32> = spawn { compute() }
let result: i32 = wait handle    // Blocks until complete
```

**CRITICAL:** Keys MUST NOT be held across `wait` suspension points.

### 10.5. Dispatch (Data Parallelism)

```cursive
// Basic parallel iteration
dispatch i in 0..1000 {
    process(data[i])
}

// With reduction (sum)
let sum: i32 = dispatch i in 0..100 [reduce: +] {
    arr[i]
}

// With reduction (product)
let product: i32 = dispatch i in 0..10 [reduce: *] {
    arr[i]
}

// Ordered execution
dispatch i in 0..100 [ordered] {
    print_in_order(i)
}

// With chunk size
dispatch i in 0..10000 [chunk: 100] {
    process(i)
}

// Combined options
let total: i32 = dispatch i in 0..1000 [reduce: +, chunk: 64] {
    compute(i)
}

// With key clause
dispatch i in 0..100 key data[i] write {
    data[i] = compute(i)
}
```

**Dispatch options:**
| Option | Meaning |
|--------|---------|
| `reduce: op` | Reduction with `+`, `*`, `min`, `max`, `and`, `or`, or custom identifier |
| `ordered` | Preserve iteration order for side effects |
| `chunk: expr` | Set chunk size for work distribution |

**Key-Based Parallelism:**
| Key Pattern | Keys Generated | Parallelism |
|-------------|----------------|-------------|
| `data[i]` | n distinct keys | Full parallel |
| `data[i / 2]` | n/2 distinct keys | Pairs serialize |
| `data[i % k]` | k distinct keys | k-way parallel |
| `data[f(i)]` | Unknown | Runtime serialization |

### 10.6. Capture Semantics

| Permission | Capture Behavior |
|------------|------------------|
| `const` | Captured by reference; unlimited sharing |
| `shared` | Captured by reference; synchronized via keys |
| `unique` | MUST use explicit `move`; at most one work item receives ownership |

```cursive
// WRONG - unique cannot be implicitly captured
let unique_data: unique Buffer = ...
parallel ctx.cpu() {
    spawn { use(unique_data) }  // ERROR
}

// CORRECT - explicit move
parallel ctx.cpu() {
    spawn { use(move unique_data) }  // OK
}
```

### 10.7. Cancellation

```cursive
let token: CancelToken@Active = CancelToken::new()

parallel ctx.cpu() [cancel: token] {
    spawn {
        loop {
            if token~>is_cancelled() {
                break
            }
            work()
        }
    }
}

// Request cancellation from elsewhere
token~>cancel()
```

**CancelToken Methods:**
- `cancel(~%)` - Request cancellation
- `is_cancelled(~)` - Check if cancelled
- `wait_cancelled(~)` - Async wait for cancellation
- `child(~)` - Create child token

### 10.8. Panic Handling

- Panics are captured, not propagated immediately
- All work continues to completion (or cancellation)
- First panic (by completion order) is raised at block boundary
- If cancel token attached, cancellation is requested on first panic

---

## 11. Asynchronous Operations

### 11.1. Async Procedures

A procedure is async if its return type is `Async<Out, In, Result, E>` or an alias thereof:
```cursive
procedure fetch_data() -> Future<Data, IOError> {
    let response: Response = yield from http_get(url)
    return parse(response)
}
```

### 11.2. Yield Expression

```cursive
// Basic yield
yield value

// Yield and release held keys
yield release value

// Yield from another async (delegate iteration)
yield from other_async

// Yield from with release
yield release from other_async
```

**Yield forms:**
| Form | Meaning |
|------|---------|
| `yield expr` | Yield a single value and suspend |
| `yield release expr` | Yield value, release held keys, then suspend |
| `yield from expr` | Delegate to another async, yielding all its values |
| `yield release from expr` | Delegate with key release |

**CRITICAL:** `yield` and `yield from` MUST NOT occur while keys are held unless the `release` modifier is present.

### 11.3. Sync Expression

Runs an async to completion synchronously (blocking):
```cursive
let result: Data | IOError = sync fetch_data()
```

**Constraints:**
- Only allowed in non-async context
- The async must have `Out = ()` and `In = ()`

### 11.4. Race Expression

Returns the first completed result:
```cursive
// Race with return handlers
let result: i32 = race {
    fetch_primary() -> |v: i32| v,
    fetch_backup() -> |v: i32| v
}

// Race with yield handlers (streaming)
race {
    source_a() -> |v| yield v,
    source_b() -> |v| yield v
}
```

**Race handlers:**
- `-> |v| v` - Return the value (RaceReturn)
- `-> |v| yield v` - Yield the value (RaceYield)

**Requirements:**
- At least 2 arms
- All handlers must be same type (all return OR all yield)

### 11.5. All Expression

Waits for all to complete:
```cursive
let results: (i32, i32) = all {
    compute_a(),
    compute_b()
}
```

If any fails, cancels remaining and propagates first error.

### 11.6. Async-Key Integration

**CRITICAL:** Keys cannot be held across yield points.

```cursive
// WRONG - key held across yield
#shared_data {
    yield value        // ERROR: E-CON-0213
}

// CORRECT - use yield release
#shared_data {
    yield release value  // OK: keys released before suspend
}
```

After `yield release`, bindings derived from shared data before the yield are potentially stale.

### 11.7. Async Combinators

```cursive
// Transform yielded values
let mapped: Async<U, In, Result, E> = async_val~>map(f)

// Filter yielded values
let filtered: Async<T, (), (), E> = async_val~>filter(predicate)

// Take first n values
let limited: Async<T, (), (), E> = async_val~>take(n)

// Fold to single result
let folded: Future<A, E> = async_val~>fold(init, accumulator)

// Chain futures
let chained: Future<U, E> = future~>chain(f)
```

---

## 12. Foreign Function Interface

### 12.1. FfiSafe Types

Only FfiSafe types can cross the FFI boundary.

**FfiSafe Types:**
- Primitives: `i8-i128`, `u8-u128`, `isize`, `usize`, `f16`, `f32`, `f64`, `char`, `()`
- Raw pointers: `*imm T`, `*mut T`
- Arrays of FfiSafe types
- Function pointers with FfiSafe signature
- Records/Enums with `[[layout(C)]]` and all FfiSafe fields

**NOT FfiSafe:**
- `bool`
- Modal types
- Safe pointers `Ptr<T>`
- Dynamic class objects `$ClassName`
- Tuples, Unions, Slices
- String and bytes types
- `Context`

### 12.2. Extern Declarations

```cursive
extern "C" {
    procedure malloc(size: usize) -> *mut u8
    procedure free(ptr: *mut u8) -> ()
}
```

**ABI Strings:** `"C"`, `"C-unwind"`, `"system"`, `"stdcall"`, `"fastcall"`, `"vectorcall"`

**Call Safety:** Calls to extern procedures MUST appear within `unsafe { }`.

### 12.3. Exported Procedures

```cursive
[[export]]
public procedure lib_init() -> () {
    // Callable from foreign code
}
```

### 12.4. Foreign Contracts

```cursive
extern "C" {
    procedure read_file(path: *imm u8, buf: *mut u8, len: usize) -> i32
        |= @foreign_assumes(path != null, buf != null, len > 0)
        |= @foreign_ensures(@result >= 0)
        |= @foreign_ensures(@error: @result < 0)
}
```

**Verification Modes:**
| Mode | Behavior |
|------|----------|
| `[[static]]` (default) | Caller must prove predicates at compile time |
| `[[dynamic]]` | Runtime checks before call |
| `[[assume]]` | Assume true without checks (optimization) |

### 12.5. Capability Isolation

Foreign code CANNOT receive or return capability-bearing values. Any FFI signature containing `Context`, capability classes, or dynamic class objects is ill-formed.

---

## 13. Canonical Templates

### 13.1. Minimal Executable

```cursive
public procedure main(move ctx: Context) -> i32 {
    return 0
}
```

### 13.2. Hello World

```cursive
using cursive::runtime::System

public procedure main(move ctx: Context) -> i32 {
    System::write_stdout("Hello, World!\n")
    return 0
}
```

### 13.3. Union Handling

```cursive
procedure classify(u: i32 | bool) -> i32 {
    return match u {
        v: i32 => { v },
        b: bool => { if b { 1 } else { 0 } }
    }
}
```

### 13.4. Safe Pointer Usage

```cursive
procedure increment() -> i32 {
    var x: i32 = 10
    let p: Ptr<unique i32>@Valid = &x
    *p = *p + 1
    return x
}
```

### 13.5. Region Allocation

```cursive
procedure use_region() -> i32 {
    var result: i32 = 0
    region {
        let p: Ptr<i32>@Valid = ^42
        result = *p
    }
    return result
}
```

### 13.6. File Reading

```cursive
procedure read_config(ctx: Context) -> string@Managed | IOError {
    let content: string@Managed | IOError = ctx.fs~>read_file("config.txt", ctx.heap)
    return content
}
```

### 13.7. Record with Methods

```cursive
public record Counter {
    value: i32,

    procedure new(initial: i32) -> Counter {
        return Counter{ value: initial }
    }

    procedure get(~) -> i32 {
        return self.value
    }

    procedure increment(~!) -> () {
        self.value = self.value + 1
    }
}
```

### 13.8. Enum with Payload

```cursive
public enum Option {
    Some(i32),
    None
}

procedure unwrap_or(opt: Option, default: i32) -> i32 {
    return match opt {
        Option::Some(v) => { v },
        Option::None => { default }
    }
}
```

### 13.9. Parallel Computation

```cursive
public procedure parallel_sum(ctx: Context, data: [i32]) -> i32 {
    let sum: i32 = parallel ctx.cpu() {
        dispatch i in 0..data~>len() [reduce: +] {
            data[i]
        }
    }
    return sum
}
```

### 13.10. Async Sequence

```cursive
procedure generate_numbers() -> Sequence<i32> {
    loop i in 0..10 {
        yield i
    }
}
```

---

## 14. Validation Checklist

Before emitting Cursive code, verify:

1. **No trailing commas on single lines** (allowed only when closing delimiter is on a different line)
2. **Match arms use commas** as separators (no trailing comma on last arm)
3. **Generic parameters use semicolons** `<T; U>`, **generic arguments use commas** `<T, U>`
4. **Method calls use `~>`**, not `.`
5. **All procedures have explicit return types**
6. **Non-unit procedures end with `return <expr>`**
7. **`main` is `public`, takes `Context`, returns `i32`**
8. **No Rust syntax**: `impl`, `fn`, `struct`, `pub`, `mut`, `&`, `&mut`, lifetime annotations
9. **No Rust types**: `Option<>`, `Result<>`, `Box<>`, `Vec<>`, `Rc<>`, `Arc<>`
10. **`move` used for consuming parameters and arguments**
11. **`Ptr::null()` and `null` only used with sufficient type context**
12. **`^` allocation only inside `region` blocks**
13. **Unsafe operations only inside `unsafe { ... }`**
14. **No use of reserved namespaces** (`cursive.*`, `std`, `gen_*`)
15. **No unsupported constructs**: `closure`, `pipeline`, `metaprogramming`, `Network`, `GPUFactory`, `CPUFactory`
16. **Float literals have suffix**: `3.14f` not `3.14`
17. **Where clauses use predicate syntax**: `where Bitcopy(T)` not `where T: Bitcopy`
18. **Shadow bindings use `shadow let`/`shadow var`** for rebinding names
19. **Loop invariants use `where { pred }`** after the loop condition
20. **Foreign contracts use `|= @foreign_assumes(...)`** and **`|= @foreign_ensures(...)`** syntax
21. **Contract postconditions can use `@result`** and **`@entry(expr)`** intrinsics
22. **Key boundary fields use `#` prefix** in record declarations
23. **Static verification is default** for contracts; use `[[dynamic]]` for runtime checks
24. **Union types are unordered** (`A | B` is equivalent to `B | A`)
25. **No subtyping between numeric types** (`i32` is NOT a subtype of `i64`)
26. **Keys cannot be held across yield points** unless using `yield release`
27. **Contract predicates must be pure** (no capability calls, no mutation)
28. **@entry(expr) requires BitcopyType or CloneType**
29. **FfiSafe types required for FFI boundary crossing**
30. **Capability isolation**: extern procedures cannot access Context

---

## 15. Common Mistakes to Avoid

### Wrong: Using dot for method calls
```cursive
// WRONG
str.length()
list.push(item)

// CORRECT
str~>length()
list~>push(item)
```

### Wrong: Single-line trailing commas
```cursive
// WRONG - trailing comma on single line
f(a, b,)
Point{ x: 1, y: 2, }

// CORRECT - no trailing comma on single line
f(a, b)
Point{ x: 1, y: 2 }

// CORRECT - trailing comma allowed in multi-line
Point{
    x: 1,
    y: 2,
}
```

### Wrong: Missing return in non-unit procedure
```cursive
// WRONG
procedure add(a: i32, b: i32) -> i32 {
    a + b
}

// CORRECT
procedure add(a: i32, b: i32) -> i32 {
    return a + b
}
```

### Wrong: Rust-style references
```cursive
// WRONG (Rust syntax)
fn foo(x: &i32, y: &mut i32)

// CORRECT (Cursive syntax)
procedure foo(x: const i32, y: unique i32) -> ()
```

### Wrong: Missing type annotation for null/Ptr::null()
```cursive
// WRONG - no type context
let p = Ptr::null()
let rp = null

// CORRECT - with type annotation
let p: Ptr<i32>@Null = Ptr::null()
let rp: *imm u8 = null
```

### Wrong: Allocation outside region
```cursive
// WRONG
let p: Ptr<i32>@Valid = ^42

// CORRECT
region {
    let p: Ptr<i32>@Valid = ^42
}
```

### Wrong: Commas in generic parameters
```cursive
// WRONG (Rust-style)
procedure swap<T, U>(a: T, b: U) -> (U, T)

// CORRECT (Cursive uses semicolons)
procedure swap<T; U>(a: T, b: U) -> (U, T)
```

### Wrong: Missing commas between match arms
```cursive
// WRONG - no commas between arms
match x {
    0 => { "zero" }
    1 => { "one" }
    _ => { "other" }
}

// CORRECT - commas separate arms
match x {
    0 => { "zero" },
    1 => { "one" },
    _ => { "other" }
}
```

### Wrong: Float literal without suffix
```cursive
// WRONG
let x = 3.14
let y = 1.0e10

// CORRECT (valid suffixes: f, f16, f32, f64)
let x: f32 = 3.14f
let y: f64 = 1.0e10f64
let z: f16 = 0.5f16
```

### Wrong: Rust-style where clause
```cursive
// WRONG (Rust syntax)
procedure foo<T>(x: T) where T: Clone

// CORRECT (Cursive syntax)
procedure foo<T>(x: T) -> ()
    where Clone(T)
{
    // ...
}
```

### Wrong: Using unsupported constructs
```cursive
// WRONG - closures not supported
let f = |x| x + 1

// CORRECT - use named procedure
procedure add_one(x: i32) -> i32 {
    return x + 1
}
```

### Wrong: Missing shadow keyword for rebinding
```cursive
// WRONG - duplicate binding error
let x: i32 = 10
let x: i32 = x + 1

// CORRECT - use shadow
let x: i32 = 10
shadow let x: i32 = x + 1
```

### Wrong: Confusing generic params and args separators
```cursive
// WRONG - commas in parameter declaration
record Pair<T, U> {
    first: T,
    second: U
}

// CORRECT - semicolons in declaration, commas in instantiation
record Pair<T; U> {
    first: T,
    second: U
}

let p: Pair<i32, bool> = Pair{ first: 1, second: true }
```

### Wrong: Old extern contract syntax
```cursive
// WRONG - obsolete syntax
extern "C" {
    procedure read_file(path: *imm u8) -> i32
        assumes { path != null }
        ensures { result >= 0 }
}

// CORRECT - use |= @foreign_assumes/@foreign_ensures
extern "C" {
    procedure read_file(path: *imm u8) -> i32
        |= @foreign_assumes(path != null)
        |= @foreign_ensures(@result >= 0)
}
```

### Wrong: Assuming numeric subtyping
```cursive
// WRONG - i32 is NOT a subtype of i64
procedure take_i64(x: i64) -> () { }
let val: i32 = 42
take_i64(val)  // ERROR

// CORRECT - explicit cast
take_i64(val as i64)
```

### Wrong: Holding keys across yield
```cursive
// WRONG - key held across yield
#shared_data {
    yield value  // ERROR: E-CON-0213
}

// CORRECT - use yield release
#shared_data {
    yield release value
}
```

### Wrong: Impure contract predicate
```cursive
// WRONG - contract calls capability method
procedure example(ctx: Context, x: i32) -> i32
    |= ctx.fs~>exists("file.txt")  // ERROR: impure
{
    return x
}

// CORRECT - only pure expressions
procedure example(x: i32) -> i32
    |= x > 0
{
    return x
}
```

---

*This guide is derived from the Cursive0.md specification. When uncertain about a construct, refer to the specification or use a more explicit alternative.*
