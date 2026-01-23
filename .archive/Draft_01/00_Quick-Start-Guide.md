# Clause 0: Language Foundations and Quick Reference

**Navigation**: Next: [§1 Introduction and Conformance](01_Introduction-and-Conformance/) | Up: [Specification](Cursive-Language-Specification-Complete.md)

---

## §0.1 Purpose and Audience [quickstart.purpose]

[1] This clause provides rapid orientation to the Cursive programming language for implementers, language designers, and experienced programmers. It presents the essential concepts, syntax patterns, and design decisions before the formal specification in Clauses 1-16.

[2] **Intended audiences**:

- **Compiler implementers**: High-level architecture before diving into formal semantics
- **Language designers**: Core principles and design decisions at a glance
- **Experienced programmers**: Conceptual model and syntax overview for rapid learning
- **Specification reviewers**: Foundation for understanding later formal details

[3] **Not a substitute for**: This clause does not replace the formal specification. All normative requirements appear in Clauses 1-16. This clause provides orientation and quick reference only.

[4] **Reading paths**:

- **Implementers**: Read this clause, then §2 (Translation Phases), §7 (Type System), §6 (Memory Model)
- **Language learners**: Read this clause, then consult tutorial documentation (out of scope for this specification)
- **Quick reference**: Use tables and decision trees in §§0.4-0.7 as syntax reference

---

## §0.2 Core Design Principles [quickstart.principles]

[5] Cursive achieves memory safety without garbage collection or borrow checking through six foundational principles that inform every language feature:

### §0.2.1 Explicit Over Implicit [quickstart.principles.explicit]

[6] **Principle**: All operational semantics are visible in source code. Hidden behavior is prohibited.

**Manifestations**:

| Feature                         | Explicit Marker                        | Implicit Alternative (Rejected)  |
| ------------------------------- | -------------------------------------- | -------------------------------- |
| Cleanup responsibility transfer | `move` keyword required                | Implicit move on last use (Rust) |
| Region allocation               | `^` operator visible                   | Hidden stack/heap decision       |
| Mutable access                  | `unique` or `shared` permission        | Implicit &mut inference          |
| Effect capabilities             | Grant lists in sequents `[[ grants ]]` | Unchecked side effects           |
| State transitions               | Modal type states `@State`             | Runtime state tracking           |

[7] **Rationale**: Explicitness enables local reasoning. Programmers understand code behavior by reading the local context alone, without whole-program analysis or hidden compiler decisions.

### §0.2.2 Zero Abstraction Cost [quickstart.principles.zerocost]

[8] **Principle**: Safety mechanisms impose zero runtime overhead. All safety checking occurs at compile time.

**Guarantees**:

- Permission checking: Compile-time only, zero runtime cost
- Region escape analysis: Compile-time only
- Modal state tracking: Compile-time only (states erased at runtime)
- Move semantics: Compile-time tracking, no runtime metadata
- Grant checking: Compile-time verification, no runtime enforcement

[9] **Performance property**: Safe Cursive code compiles to identical assembly as equivalent unsafe C code performing the same operations.

### §0.2.3 Local Reasoning [quickstart.principles.local]

[10] **Principle**: Code behavior is understandable from local context. No global analysis required to understand a procedure.

**Manifestations**:

- Procedure signatures include complete contracts (grants, preconditions, postconditions)
- Permission constraints visible in type annotations
- Cleanup responsibility visible in binding operators (`=` vs `<-`)
- Move transfers visible at call sites (`move` keyword)

[11] **Benefit for LLMs**: Local reasoning makes code predictable for AI-assisted development. LLMs can generate correct code by understanding local patterns without whole-program context.

### §0.2.4 Deterministic Semantics [quickstart.principles.deterministic]

[12] **Principle**: Program behavior is fully determined by source code. No unspecified evaluation order or non-deterministic operations.

**Guarantees**:

- Expression evaluation: Strict left-to-right
- Statement execution: Sequential in textual order
- Cleanup order: LIFO (last-in, first-out), deterministic
- Module initialization: Topological order by dependencies
- Comptime evaluation: Deterministic for identical inputs

### §0.2.5 No Hidden Mechanisms [quickstart.principles.nomacros]

[13] **Principle**: Cursive provides no textual macro system, no preprocessor, no hidden code generation.

**What Cursive does NOT have**:

- ❌ Textual macros or preprocessor
- ❌ Garbage collection
- ❌ Borrow checker / lifetime annotations
- ❌ Automatic reference counting
- ❌ Implicit type conversions (except documented coercions)
- ❌ Interior mutability
- ❌ Hidden allocations

[14] **Alternative**: Compile-time evaluation (Clause 16) provides metaprogramming through ordinary procedures executed at compile time, not textual substitution.

### §0.2.6 Permissions Not Markers [quickstart.principles.permissions]

[15] **Principle**: Thread safety is encoded through the permission system (`const`/`unique`/`shared`), not separate marker behaviors.

**Comparison to other languages**:

| Rust Markers    | Cursive Equivalent    | Purpose                                 |
| --------------- | --------------------- | --------------------------------------- |
| `Send` trait    | `unique` permission   | Safe to transfer ownership to thread    |
| `Sync` trait    | `const` permission    | Safe to share references across threads |
| `Arc<Mutex<T>>` | `shared` + `Mutex<T>` | Shared mutable data with sync           |

[16] **Rationale**: Permissions are fundamental to Cursive's type system and appear explicitly in signatures. Adding separate markers would create redundancy.

---

## §0.3 The Two-Axis Memory Model [quickstart.memory]

[17] Cursive's memory model separates concerns into two **orthogonal and independent** axes:

### §0.3.1 Axis 1: Cleanup Responsibility [quickstart.memory.responsibility]

[18] **Determined by**: Assignment operator choice

**Two operators**:

```cursive
let x = value      // RESPONSIBLE: calls destructor at scope exit
let y <- value     // NON-RESPONSIBLE: does NOT call destructor
```

[19] **Responsibility semantics**:

| Operator | Calls Destructor    | Multiple Allowed                                | Transferable via `move` |
| -------- | ------------------- | ----------------------------------------------- | ----------------------- |
| `=`      | YES (at scope exit) | No (only one responsible binding per object)    | YES (if `let`)          |
| `<-`     | NO                  | YES (multiple non-responsible bindings allowed) | NO                      |

[20] **Key insight**: Both create bindings of the SAME TYPE. The difference is metadata (cleanup responsibility), not type.

```cursive
let owner: Buffer = Buffer::new()    // Type: Buffer, responsible
let viewer: Buffer <- owner          // Type: Buffer, non-responsible
// Both have type Buffer; cleanup responsibility differs
```

### §0.3.2 Axis 2: Access Permissions [quickstart.memory.permissions]

[21] **Determined by**: Permission qualifier in type

**Three permissions**:

```cursive
let immutable: const Buffer = buffer        // READ-ONLY, unlimited aliasing
let exclusive: unique Buffer = buffer       // EXCLUSIVE mutable, no aliasing
let coordinated: shared Buffer = buffer     // SHARED mutable, programmer coordinates
```

[22] **Permission lattice** (subtyping):

```
unique <: shared <: const
(more restrictive) → (less restrictive)
```

[23] **Permission semantics**:

| Permission | Read | Mutate | Multiple Bindings      | Thread Safety                |
| ---------- | ---- | ------ | ---------------------- | ---------------------------- |
| `const`    | ✅   | ❌     | ✅ (unlimited)         | ✅ (safe to share)           |
| `unique`   | ✅   | ✅     | ❌ (exclusive)         | ✅ (safe to transfer)        |
| `shared`   | ✅   | ✅     | ✅ (with coordination) | ⚠️ (requires sync primitive) |

### §0.3.3 Axis Independence [quickstart.memory.orthogonal]

[24] **Critical property**: The two axes are **completely orthogonal**. They may be combined freely:

**Table 0.3.1 — Complete binding forms**

| Binding Form         | Cleanup         | Permission          | Rebindable | Transferable | Use Case                   |
| -------------------- | --------------- | ------------------- | ---------- | ------------ | -------------------------- |
| `let x: const = v`   | Responsible     | Read-only           | No         | Yes          | Immutable owned value      |
| `let x: unique = v`  | Responsible     | Exclusive mutable   | No         | Yes          | **Mutable + transferable** |
| `let x: shared = v`  | Responsible     | Coordinated mutable | No         | Yes          | Shared mutable owned       |
| `var x: const = v`   | Responsible     | Read-only           | Yes        | No           | Rebindable immutable       |
| `var x: unique = v`  | Responsible     | Exclusive mutable   | Yes        | No           | Rebindable mutable         |
| `var x: shared = v`  | Responsible     | Coordinated mutable | Yes        | No           | Rebindable shared          |
| `let x: const <- v`  | Non-responsible | Read-only           | No         | No           | View of immutable          |
| `let x: unique <- v` | Non-responsible | Exclusive mutable   | No         | No           | Exclusive view             |
| `let x: shared <- v` | Non-responsible | Coordinated mutable | No         | No           | Shared view                |
| `var x: const <- v`  | Non-responsible | Read-only           | Yes        | No           | Rebindable view            |
| `var x: unique <- v` | Non-responsible | Exclusive mutable   | Yes        | No           | Rebindable exclusive       |
| `var x: shared <- v` | Non-responsible | Coordinated mutable | Yes        | No           | Rebindable shared          |

[25] **Common pattern**: For mutable + transferable variables, use `let x: unique = value`.

---

## §0.4 Syntax at a Glance [quickstart.syntax]

### §0.4.1 Variable Declarations [quickstart.syntax.variables]

[26] **Complete syntax table**:

```cursive
// Immutable, responsible, transferable
let x = value
let x: Type = value
let x: permission Type = value

// Mutable, responsible, NOT transferable
var y = value

// Immutable, NON-responsible, NOT transferable
let z <- value

// Mutable, NON-responsible, NOT transferable
var w <- value

// Pattern destructuring
let {field1, field2}: RecordType = value
let (first, second): (T1, T2) = value
```

### §0.4.2 Procedure Declarations [quickstart.syntax.procedures]

[27] **Syntax patterns**:

```cursive
// Pure procedure (no grants, omit sequent)
procedure add(a: i32, b: i32): i32
{
    result a + b
}

// Procedure with grants and contract
procedure write_log(message: string)
    [[ io::write |- message.len() > 0 => true ]]
{
    println("{}", message)
}

// Complete contract form
procedure transfer(~!, to: unique Account, amount: i64)
    [[
        ledger::post
        |-
        amount > 0,
        self.balance >= amount
        =>
        self.balance == @old(self.balance) - amount,
        to.balance == @old(to.balance) + amount
    ]]
{
    self.balance -= amount
    to.balance += amount
}

// Expression-bodied (pure only)
procedure double(x: i32): i32
= x * 2

// Receiver shorthands
procedure method(~)      // const Self
procedure method(~%)     // shared Self
procedure method(~!)     // unique Self
```

### §0.4.3 Type Declarations [quickstart.syntax.types]

[28] **All type declaration forms**:

```cursive
// Record (named fields)
record Point {
    x: f64,
    y: f64,
}

// Record with permissions and privacy
record Account {
    public id: u64,
    private balance: i64,
}

// Tuple record (positional fields)
record Velocity(f64, f64, f64)

// Enum (discriminated union)
enum Status {
    Pending,
    Running(i32),
    Complete { code: i32, message: string },
}

// Modal type (state machine)
modal FileHandle {
    @Closed {
        path: string,
    }
    @Closed::open(~!, path: string) -> @Open

    @Open {
        path: string,
        handle: i32,
    }
    @Open::read(~%, buffer: [u8]) -> @Open
    @Open::close(~!) -> @Closed
}

// Type alias
type UserId = u64

// Generic type
record Container<T> {
    items: [T],
}
```

### §0.4.4 Control Flow [quickstart.syntax.control]

[29] **Expression-based control flow**:

```cursive
// If expression (can produce value)
let value = if condition { 42 } else { 0 }

// Match expression (exhaustive)
let result = match status {
    Status::Pending => 0,
    Status::Running(n) => n,
    Status::Complete { code, message } => code,
}

// Loop expressions
loop { ... }                     // Infinite loop
loop condition { ... }           // While loop
loop item: T in collection { ... }  // Iterator loop (type annotation REQUIRED)

// Early exit
break                           // Exit loop
break value                     // Exit with value
continue                        // Next iteration
result value                    // Return from procedure
```

---

## §0.5 Decision Trees [quickstart.decisions]

### §0.5.1 Should I Use `=` or `<-`? [quickstart.decisions.operator]

[30] **Decision tree**:

```
Do you want the binding to call the destructor at scope exit?
├─ YES → Use `=` (responsible binding)
│  └─ Will you need to transfer ownership via `move`?
│     ├─ YES → Use `let x = value` (responsible + transferable)
│     └─ NO → Use `var x = value` (responsible + rebindable)
│
└─ NO → Use `<-` (non-responsible binding)
   └─ Creating multiple views of same object?
      ├─ YES → Use `<-` (multiple non-responsible bindings allowed)
      └─ NO → Still use `<-` if original binding handles cleanup
```

### §0.5.2 Should I Use `let` or `var`? [quickstart.decisions.mutability]

[31] **Decision tree**:

```
Will you reassign the binding (x = new_value)?
├─ NO → Use `let` (immutable binding)
│  └─ Might transfer ownership later?
│     ├─ YES → Use `let` (transferable via move)
│     └─ NO → Use `let` (default choice)
│
└─ YES → Use `var` (mutable binding)
   └─ ⚠️ Note: `var` bindings are NOT transferable
```

### §0.5.3 Which Permission Should I Use? [quickstart.decisions.permission]

[32] **Decision tree**:

```
Does the value need to be mutated?
├─ NO → Use `const` (default permission, often omitted)
│  └─ Can use value.method() for const methods only
│
└─ YES → Will multiple bindings need mutable access simultaneously?
   ├─ NO → Use `unique` (exclusive mutable access)
   │  └─ Perfect for: builders, owned resources, exclusive data
   │
   └─ YES → Use `shared` (coordinated mutable access)
      └─ ⚠️ Requires: field-level partitioning or external synchronization
```

### §0.5.4 Should I Use Regions or Heap? [quickstart.decisions.allocation]

[33] **Decision tree**:

```
Does allocated data need to outlive the creating scope?
├─ NO → Use region allocation (`^`)
│  └─ Lifetime bounded by region block?
│     ├─ YES → region { let x = ^Data::new() }
│     └─ NO → Pass arena to procedures, allocate with workspace.alloc()
│
└─ YES → Use heap allocation or escape region value
   └─ Strategy:
      ├─ Direct heap: Data::new() (no `^`, allocates on heap)
      └─ Escape region: let x = ^Data::new(); x.to_heap()
```

### §0.5.5 Should I Use Behaviors or Contracts? [quickstart.decisions.polymorphism]

[34] **Decision tree**:

```
Do you want to share CODE or specify REQUIREMENTS?
├─ Share CODE (default implementations) → Use BEHAVIOR
│  └─ behavior Display {
│        procedure show(~): string { result "Display" }
│      }
│  └─ All procedures MUST have bodies
│  └─ Attach: `record T with Display`
│
└─ Specify REQUIREMENTS (abstract interface) → Use CONTRACT
   └─ contract Serializable {
         procedure serialize(~): [u8]
         // NO body - pure requirement
       }
   └─ NO procedures may have bodies
   └─ Attach: `record T: Serializable`
```

---

## §0.6 Complete Working Example [quickstart.example]

[35] **Example 0.6.1** (Comprehensive demonstration of all major features):

```cursive
//! Bank account system demonstrating:
//! - Modal types (Account state machine)
//! - Permissions (const/unique/shared)
//! - Binding responsibility (= vs <-)
//! - Contracts (preconditions/postconditions)
//! - Region allocation
//! - Error handling with unions
//! - Behaviors for code reuse

use std::io::println

// User-defined capability grant
public grant ledger_modify

// Contract: abstract interface requirement
public contract Auditable {
    procedure audit_log(~): string
        [[ |- true => result.len() > 0 ]]
}

// Behavior: concrete code reuse
public behavior Display {
    procedure show(~): string
    {
        result type_name::<Self>()  // Default implementation
    }
}

// Modal type: compile-time state machine
public modal Account {
    @Active {
        id: u64,
        balance: i64,
    }

    @Active::deposit(~!, amount: i64) -> @Active
    @Active::withdraw(~!, amount: i64) -> @Active \/ InsufficientFunds
    @Active::freeze(~!) -> @Frozen

    @Frozen {
        id: u64,
        balance: i64,
        freeze_reason: string,
    }

    @Frozen::unfreeze(~!) -> @Active
}

public record InsufficientFunds {
    requested: i64,
    available: i64,
}

// Implement contract for Account
behavior Auditable for Account {
    procedure audit_log(~): string
        [[ alloc::heap |- true => result.len() > 0 ]]
    {
        result string::from("Account audit log")
    }
}

// Implement behavior for Account
behavior Display for Account {
    procedure show(~): string
    {
        result string::concat("Account(id=", usize::to_string(self.id))
    }
}

// Modal transition implementation
procedure Account.deposit(self: unique Self@Active, amount: i64): Self@Active
    [[
        ledger_modify
        |-
        amount > 0
        =>
        self.balance == @old(self.balance) + amount
    ]]
{
    self.balance += amount
    result self  // Returns same account in @Active state
}

procedure Account.withdraw(self: unique Self@Active, amount: i64): Self@Active \/ InsufficientFunds
    [[
        ledger_modify
        |-
        amount > 0
        =>
        match result {
            acct: Account@Active => acct.balance == @old(self.balance) - amount,
            err: InsufficientFunds => self.balance == @old(self.balance)
        }
    ]]
{
    if self.balance >= amount {
        self.balance -= amount
        result self
    } else {
        result InsufficientFunds {
            requested: amount,
            available: self.balance
        }
    }
}

procedure Account.freeze(self: unique Self@Active, reason: string): Self@Frozen
    [[ ledger_modify, alloc::heap |- reason.len() > 0 => true ]]
{
    result Account@Frozen {
        id: self.id,
        balance: self.balance,
        freeze_reason: reason.to_managed()
    }
}

procedure Account.unfreeze(self: unique Self@Frozen): Self@Active
    [[ ledger_modify |- true => true ]]
{
    result Account@Active {
        id: self.id,
        balance: self.balance,
    }
}

// Batch processing with region allocation
procedure process_transactions(accounts: [unique Account@Active], transactions: [Transaction])
    [[
        ledger_modify,
        alloc::region,
        io::write
        |-
        accounts.len() > 0,
        transactions.len() > 0
        =>
        true
    ]]
{
    region batch {
        // Temporary allocations freed in O(1) when region exits
        loop txn: Transaction in transactions {
            let report = ^format_transaction(txn)  // Region-allocated

            match txn {
                Transaction::Deposit { account_id, amount } => {
                    let acct = find_account_mut(accounts, account_id)

                    // State transition: @Active -> @Active
                    acct = acct.deposit(amount)

                    println("Deposited {} into account {}", amount, account_id)
                }

                Transaction::Withdraw { account_id, amount } => {
                    let acct = find_account_mut(accounts, account_id)

                    // State transition with error handling
                    match acct.withdraw(amount) {
                        updated: Account@Active => {
                            acct = updated
                            println("Withdrew {} from account {}", amount, account_id)
                        }
                        err: InsufficientFunds => {
                            println("Insufficient funds: requested {}, available {}",
                                err.requested, err.available)
                        }
                    }
                }
            }
        }
    }
    // Region 'batch' destroyed here - all temporary allocations freed in O(1)
}

// Helper: demonstrate non-responsible bindings
procedure find_account_mut(accounts: [unique Account@Active], id: u64): unique Account@Active
    [[ |- accounts.len() > 0, id > 0 => true ]]
{
    loop acct: unique Account@Active in accounts {
        // Non-responsible binding for checking
        let acct_ref: const <- acct

        if acct_ref.id == id {
            result acct  // Transfer ownership of found account
        }
    }

    panic("Account not found")
}

// Entry point
public procedure main(): i32
    [[
        ledger_modify,
        alloc::heap,
        alloc::region,
        io::write
        |-
        true
        =>
        true
    ]]
{
    // Create accounts
    let account1 = Account@Active { id: 1, balance: 1000 }
    let account2 = Account@Active { id: 2, balance: 500 }

    var accounts: [unique Account@Active] = [account1, account2]

    // Process transactions
    let transactions: [Transaction] = [
        Transaction::Deposit { account_id: 1, amount: 100 },
        Transaction::Withdraw { account_id: 2, amount: 200 },
    ]

    process_transactions(accounts, transactions)

    // Demonstrate freezing
    let acct = accounts[0]
    let frozen = acct.freeze("Suspicious activity")

    // Modal state prevents operations: frozen.deposit() would be compile error

    println("Account frozen: {}", frozen.freeze_reason)

    result 0
}
```

[36] **Features demonstrated**:

- ✅ Modal types with state transitions (`Account@Active` → `Account@Frozen`)
- ✅ Contracts (`Auditable`) vs behaviors (`Display`)
- ✅ Permissions (`const`, `unique`, `shared`)
- ✅ Non-responsible bindings (`let acct_ref <- acct`)
- ✅ Region allocation (`^format_transaction`)
- ✅ Contractual sequents with grants, preconditions, postconditions, `@old`
- ✅ Error handling with unions (`Account@Active \/ InsufficientFunds`)
- ✅ Move semantics (implicit in ownership transfer)
- ✅ Pattern matching on enums and unions
- ✅ Explicit grants for all effects

---

## §0.7 Comparison to Other Languages [quickstart.comparison]

### §0.7.1 Memory Management Comparison [quickstart.comparison.memory]

[37] **Table 0.7.1 — Memory safety approaches**

| Feature                 | C/C++            | Rust                 | Zig                | Cursive                                       |
| ----------------------- | ---------------- | -------------------- | ------------------ | --------------------------------------------- |
| **Memory safety**       | Manual (unsafe)  | Borrow checker       | Manual (unsafe)    | Regions + Permissions                         |
| **Ownership**           | Implicit         | Exclusive borrowing  | Programmer tracked | Explicit `move`                               |
| **Multiple references** | Allowed (unsafe) | Shared XOR mutable   | Allowed (unsafe)   | `const` (unlimited) OR `shared` (coordinated) |
| **Lifetime tracking**   | Manual           | Lifetime params `'a` | Manual             | Lexical regions (no params)                   |
| **Garbage collection**  | No               | No                   | No                 | No                                            |
| **Runtime overhead**    | Zero             | Zero                 | Zero               | Zero                                          |
| **Learning curve**      | Moderate         | Steep                | Moderate           | Moderate-Steep                                |

### §0.7.2 Cursive vs Rust Detailed Comparison [quickstart.comparison.rust]

[38] **When Cursive is simpler**:

| Aspect                   | Rust                              | Cursive                                                       |
| ------------------------ | --------------------------------- | ------------------------------------------------------------- |
| **Mutable access**       | Exclusive `&mut` (one at a time)  | `unique` (exclusive) OR `shared` (multiple w/ coordination)   |
| **Move syntax**          | Implicit in many contexts         | Always explicit `move` keyword                                |
| **Lifetime annotations** | Pervasive `'a`, `'b` params       | None (lexical regions instead)                                |
| **Borrowing rules**      | Complex aliasing rules            | Simple: const=unlimited, unique=exclusive, shared=coordinated |
| **Mental model**         | Ownership + borrowing + lifetimes | Responsibility + permissions (two axes)                       |

[39] **When Rust is safer**:

- Rust prevents iterator invalidation at compile time
- Rust prevents data races more comprehensively (no `shared` escape hatch)
- Rust's borrow checker catches more errors earlier

[40] **Trade-off**: Cursive accepts slightly less compile-time enforcement in exchange for **simpler mental model** and **multiple mutable references** (via `shared` permission).

### §0.7.3 Cursive vs C++ Detailed Comparison [quickstart.comparison.cpp]

[41] **Cursive improvements over C++**:

| Feature              | C++                                     | Cursive                                        |
| -------------------- | --------------------------------------- | ---------------------------------------------- |
| **Memory safety**    | Undefined behavior minefield            | Compile-time enforced (outside unsafe blocks)  |
| **Macros**           | Textual substitution, unhygienic        | Compile-time evaluation, type-safe             |
| **Move semantics**   | Implicit via `std::move`                | Explicit `move` keyword                        |
| **Resource cleanup** | Manual or RAII                          | Automatic RAII everywhere                      |
| **Templates**        | Turing-complete, compile-time explosion | Constrained generics, bounded monomorphization |
| **Modern features**  | Bolted-on (C++11, 14, 17, 20, 23)       | Designed holistically from start               |

---

## §0.8 Common Patterns [quickstart.patterns]

### §0.8.1 Pattern: Ownership Transfer [quickstart.patterns.ownership]

[42] **When to use**: Transferring resource ownership to a procedure that will clean it up.

```cursive
// Procedure takes ownership (responsible parameter)
procedure consume(move data: Buffer)
    [[ |- true => true ]]
{
    process(data)
    // data.drop() called automatically when procedure returns
}

// Caller transfers ownership
let buffer = Buffer::new()
consume(move buffer)     // Explicit move required
// buffer invalid here (moved)
```

### §0.8.2 Pattern: Non-Owning View [quickstart.patterns.view]

[43] **When to use**: Multiple views of same data without transferring ownership.

```cursive
// Procedure inspects without taking ownership (non-responsible parameter)
procedure inspect(data: Buffer)
    [[ |- true => true ]]
{
    println("Size: {}", data.size())
    // data NOT destroyed when returning (non-responsible)
}

// Caller retains ownership
let buffer = Buffer::new()
inspect(buffer)          // No move - buffer still valid after call
buffer.use()             // Still accessible
```

### §0.8.3 Pattern: Multiple Views [quickstart.patterns.multiview]

[44] **When to use**: Creating multiple non-owning references to same object.

```cursive
let owner = Data::new()      // owner is responsible

let view1 <- owner           // Non-responsible binding
let view2 <- owner           // Another non-responsible binding
let view3 <- owner           // Multiple allowed

// All views valid while owner exists
view1.read()
view2.read()
view3.read()

// At scope exit: only owner calls destructor
```

### §0.8.4 Pattern: Mutable + Transferable [quickstart.patterns.muttransfer]

[45] **When to use**: Need both mutation AND ability to transfer ownership.

```cursive
// Use let + unique (not var)
let builder: unique = Builder::new()

builder.add_field(1)         // Mutate via unique permission
builder.add_field(2)

consume(move builder)        // Transfer via let binding
```

### §0.8.5 Pattern: Temporary Allocations [quickstart.patterns.temporary]

[46] **When to use**: Short-lived allocations in loops or recursive procedures.

```cursive
procedure process_batch(items: [Item])
    [[ alloc::region, io::write |- items.len() > 0 => true ]]
{
    loop item: Item in items {
        region iteration {
            let temp_buffer = ^Buffer::new(4096)
            let processed = ^transform(item)
            output(temp_buffer, processed)
        }
        // O(1) bulk free - no accumulation
    }
}
```

### §0.8.6 Pattern: Error Propagation [quickstart.patterns.errors]

[47] **When to use**: Procedures that may fail need to propagate errors.

```cursive
procedure load_config(path: string): Config \/ IoError \/ ParseError
    [[ fs::read, alloc::heap |- path.len() > 0 => true ]]
{
    // Read file (may fail with IoError)
    let contents = read_file(path)
    match contents {
        data: string => {
            // Parse (may fail with ParseError)
            let parsed = parse_toml(data)
            match parsed {
                config: Config => result config,      // Success
                err: ParseError => result err,         // Propagate parse error
            }
        }
        err: IoError => result err,  // Propagate I/O error
    }
}

// Caller handles all error cases
match load_config("config.toml") {
    cfg: Config => use_config(cfg),
    io_err: IoError => handle_io_error(io_err),
    parse_err: ParseError => handle_parse_error(parse_err),
}
```

### §0.8.7 Pattern: State Machine [quickstart.patterns.statemachine]

[48] **When to use**: Resources with lifecycle stages (connections, files, transactions).

```cursive
modal Connection {
    @Disconnected
    @Disconnected::connect(~!) -> @Connected \/ NetworkError

    @Connected {
        socket: Socket,
    }
    @Connected::send(~%, data: [u8]) -> @Connected \/ NetworkError
    @Connected::disconnect(~!) -> @Disconnected
}

// Type system enforces valid transitions
let conn = Connection@Disconnected::new()

match conn.connect() {
    connected: Connection@Connected => {
        // Can only call @Connected methods here
        connected.send(data)
        connected.disconnect()
    }
    err: NetworkError => handle_error(err),
}
```

### §0.8.8 Pattern: Compile-Time Code Generation [quickstart.patterns.comptime]

[49] **When to use**: Repetitive boilerplate, platform-specific code, derive-like functionality.

```cursive
[[reflect]]
record User {
    name: string,
    email: string,
    age: u32,
}

// Generate getter procedures at compile time
comptime {
    let fields <- fields_of::<User>()

    loop field: FieldInfo in fields {
        codegen::add_procedure(
            target: TypeRef::TypeId(type_id<User>()),
            spec: ProcedureSpec {
                name: string::concat("get_", field.name),
                visibility: Visibility::Public,
                receiver: ReceiverSpec::Const,
                params: [],
                return_type: TypeRef::Named(field.ty_name),
                sequent: sequent_pure(),
                body: quote {
                    result self.$(field.name)
                },
            }
        )
    }
}

// Generated procedures automatically available:
let user = User { name: "Alice", email: "alice@example.com", age: 30 }
let name = user.get_name()     // Generated at compile time
let email = user.get_email()   // Generated at compile time
let age = user.get_age()       // Generated at compile time
```

---

## §0.9 Syntax Quick Reference Tables [quickstart.reference]

### §0.9.1 Keywords [quickstart.reference.keywords]

[50] **Reserved keywords** (cannot be used as identifiers):

```
async       await      behavior   break      by         case       comptime
const       continue   contract   defer      else       enum       exists
false       forall     grant      if         import     internal   invariant
let         loop       match      modal      module     move       must
new         none       private    procedure  protected  public     record
region      result     select     self       Self       shadow     shared
state       static     true       type       unique     unsafe     use
var         where      will       with       witness
```

### §0.9.2 Operators [quickstart.reference.operators]

[51] **Complete operator table**:

| Operator           | Meaning                         | Precedence               | Example                                    |
| ------------------ | ------------------------------- | ------------------------ | ------------------------------------------ |
| `()` `[]` `.` `::` | Postfix                         | 1 (highest)              | `f()`, `arr[0]`, `x.field`, `Type::method` |
| `!` `-` `&` `move` | Prefix                          | 2                        | `!flag`, `-x`, `&place`, `move value`      |
| `**`               | Power                           | 3 (right assoc)          | `2 ** 3` = 8                               |
| `*` `/` `%`        | Multiplicative                  | 4                        | `a * b`, `a / b`, `a % b`                  |
| `+` `-`            | Additive                        | 5                        | `a + b`, `a - b`                           |
| `<<` `>>`          | Shift                           | 6                        | `x << 2`, `y >> 1`                         |
| `..` `..=`         | Range                           | 7                        | `0..10`, `0..=9`                           |
| `&`                | Bitwise AND                     | 8                        | `a & b`                                    |
| `^`                | Bitwise XOR                     | 9                        | `a ^ b`                                    |
| `\|`               | Bitwise OR                      | 10                       | `a \| b`                                   |
| `<` `<=` `>` `>=`  | Comparison                      | 11                       | `a < b`, `a >= b`                          |
| `==` `!=`          | Equality                        | 12                       | `a == b`, `a != b`                         |
| `&&`               | Logical AND                     | 13 (short-circuit)       | `a && b`                                   |
| `\|\|`             | Logical OR                      | 14 (short-circuit)       | `a \|\| b`                                 |
| `=` `+=` `-=` etc. | Assignment                      | 15 (right assoc, lowest) | `x = y`, `x += 1`                          |
| `<-`               | Non-responsible binding         | 15 (right assoc)         | `let x <- y`                               |
| `=>`               | Pipeline, implication           | Special                  | `x => f`, `P => Q` in sequents             |
| `->`               | Function type, modal transition | Special                  | `(T) -> U`, `@A -> @B`                     |
| `\/`               | Union type                      | Type-level               | `i32 \/ Error`                             |

### §0.9.3 Sequent Forms [quickstart.reference.sequents]

[52] **All valid contractual sequent forms** (smart defaulting):

```cursive
// Complete form
[[ grants |- must => will ]]

// Grant-only (expands to: [[ grants |- true => true ]])
[[ io::write ]]

// Precondition-only (expands to: [[ |- must => true ]])
[[ x > 0 ]]

// Postcondition-only (expands to: [[ |- true => will ]])
[[ => result > 0 ]]

// Must + will, no grants (expands to: [[ |- must => will ]])
[[ x > 0 => result > x ]]

// Omitted entirely (defaults to: [[ |- true => true ]])
procedure f() { }
```

### §0.9.4 Type Syntax Summary [quickstart.reference.types]

[53] **Type expression syntax**:

| Type Category | Syntax                                  | Example                       |
| ------------- | --------------------------------------- | ----------------------------- |
| Primitives    | `i32`, `bool`, `f64`, `char`, `()`, `!` | `let x: i32`                  |
| Tuples        | `(T1, T2, ...)`                         | `let pair: (i32, f64)`        |
| Arrays        | `[T; n]`                                | `let arr: [u8; 100]`          |
| Slices        | `[T]`                                   | `let slice: [i32]`            |
| Pointers      | `Ptr<T>`, `Ptr<T>@State`                | `Ptr<i32>@Valid`              |
| Raw pointers  | `*const T`, `*mut T`                    | `*mut u8`                     |
| Unions        | `T \/ U \/ ...`                         | `i32 \/ Error`                |
| Functions     | `(T1, T2) -> R ! {grants}`              | `(i32) -> bool ! {io::write}` |
| Permissions   | `const T`, `unique T`, `shared T`       | `unique Buffer`               |
| Modal states  | `Type@State`                            | `File@Open`                   |
| Generics      | `Type<T, U>`                            | `List<i32>`                   |

---

## §0.10 Reading Guide for Different Audiences [quickstart.reading]

### §0.10.1 For Compiler Implementers [quickstart.reading.implementers]

[54] **Recommended reading order**:

1. **Clause 0** (this clause) — Conceptual foundation
2. **§1.1-1.5** — Conformance requirements, normative vocabulary
3. **Clause 2** — Lexical structure and translation phases (parsing pipeline)
4. **Clause 5** — Type system foundations (types before declarations use them)
5. **Clause 6** — Memory model (permissions and responsibility before usage)
6. **Clause 7** — Names and scopes (resolution mechanisms)
7. **Clause 8** — Declarations (uses types, permissions, scopes)
8. **Clause 9** — Expressions (uses all previous)
9. **Clause 10** — Statements (control flow)
10. **Clause 11** — Generics and behaviors (polymorphism)
11. **Clause 12** — Contracts (effect system)
12. **Clause 13** — Witnesses (dynamic dispatch)
13. **Clause 14** — Concurrency
14. **Clause 15** — FFI and interop
15. **Clause 16** — Compile-time evaluation
16. **Annex A** — Authoritative grammar (reference throughout)
17. **Annex E** — Diagnostic catalog and algorithms

[55] **Critical sections for implementation bootstrap**:

- §2.2 Translation Phases — Pipeline architecture
- §5.2 Primitive Types — Foundation for type checking
- §6.2 Objects and Storage — Memory model basics
- §8.2 Variable Bindings — Binding semantics (most complex)
- §11.6 Monomorphization — Code generation strategy

### §0.10.2 For Language Designers [quickstart.reading.designers]

[56] **Recommended reading order**:

1. **Clause 0** — Design principles and philosophy
2. **§1.3** — Terms and definitions (vocabulary)
3. **§6.1-6.4** — Memory model overview and two-axis design
4. **§5.6** — Modal types (novel feature)
5. **§12.1-12.2** — Contract system (sequent calculus)
6. **§11.1** — Generics with behaviors vs contracts
7. **Rationale sections** — Scattered throughout (search for "\[ Rationale:")
8. **Design decision notes** — Marked "(Decision DNN)"

[57] **Key design innovations to study**:

- Two-axis memory model (§6.1.2) — orthogonal responsibility + permissions
- Non-responsible binding invalidation via parameter responsibility (§8.2.5.2)
- Modal types for state machines (§5.6)
- Sequent calculus for contracts (§12.1.3)
- Permission-based thread safety without markers (§14.1.2)

### §0.10.3 For Specification Reviewers [quickstart.reading.reviewers]

[58] **Recommended review strategy**:

1. **Clause 0** — Understand design space
2. **§1.4** — Notation and conventions (how to read formal rules)
3. **Linear read** — Clauses 1-16 in order (after reorganization, forward refs minimal)
4. **Cross-check** — Verify cross-references resolve correctly
5. **Example validation** — Ensure all examples compile under specified rules
6. **Diagnostic coverage** — Check all ill-formed cases have diagnostics

[59] **Common review questions answered**:

- **Q**: How does Cursive avoid borrow checking complexity?
  - **A**: Lexical regions (no lifetime params) + explicit move + permission system
- **Q**: How are data races prevented?
  - **A**: Permission system: `const` (shareable), `unique` (transferable), `shared` (coordinated)
- **Q**: How does cleanup happen without GC?
  - **A**: RAII + explicit responsibility (`=` vs `<-`) + deterministic destruction order
- **Q**: How does it avoid Rust's verbosity?
  - **A**: Simpler model (no lifetimes), explicit move (but more verbose in some cases), `shared` permission (more flexible)

---

## §0.11 Terminology Index [quickstart.terminology]

[60] **Core terms with authoritative definition locations**:

| Term                                 | Meaning                                                | Defined in | Common Confusion                   |
| ------------------------------------ | ------------------------------------------------------ | ---------- | ---------------------------------- |
| **Binding** (name binding)           | Identifier-to-entity association                       | §7.3       | NOT same as variable declaration   |
| **Binding** (cleanup responsibility) | Whether destructor is called                           | §8.2.5     | Context-dependent meaning          |
| **Behavior**                         | Code-reuse mechanism with default implementations      | §11.4      | NOT same as "predicate expression" |
| **Contract**                         | Abstract interface specification                       | §12.1      | NOT same as "contractual sequent"  |
| **Contractual sequent**              | Complete `[[ grants \|- must => will ]]` specification | §12.2      | Attached to procedures             |
| **Grant**                            | Capability token (e.g., `io::write`)                   | §12.3      | Compile-time only                  |
| **Modal type**                       | Type with named states (@State)                        | §5.6       | State machines                     |
| **Permission**                       | Access control (const/unique/shared)                   | §6.4       | NOT same as responsibility         |
| **Predicate expression**             | Boolean expression in sequents                         | §12.4      | NOT same as "behavior"             |
| **Procedure**                        | Cursive source-level callable                          | §8.4       | NOT "function"                     |
| **Function type**                    | Type system callable type `(T) -> U`                   | §5.4       | Describes procedure signatures     |
| **Region**                           | Lexical allocation arena                               | §6.3       | NOT Rust lifetimes                 |
| **Responsibility**                   | Cleanup obligation                                     | §8.2.5     | Axis 1 of memory model             |
| **Witness**                          | Runtime evidence of type property                      | §13.1      | Dynamic dispatch mechanism         |

---

## §0.12 Conformance Summary [quickstart.conformance]

[61] **What conforming implementations must provide**:

**Memory Model**:

- Two-axis model (responsibility + permissions)
- Region-based allocation with escape analysis
- RAII with LIFO destruction order
- Move semantics with explicit `move` keyword

**Type System**:

- Modal types with compile-time state tracking
- Union types with automatic widening
- Permission-qualified types
- Generic types with monomorphization

**Effect System**:

- Grant tracking (compile-time only)
- Contractual sequents (optional on procedures)
- Static verification (best-effort) or dynamic checking

**Compile-Time Evaluation**:

- Comptime procedures and blocks
- Type reflection (opt-in via `[[reflect]]`)
- Code generation via explicit APIs
- Quote expressions with interpolation

**Concurrency**:

- Permission-based thread safety
- Modal types for sync primitives
- Happens-before memory model
- Atomic operations

**Interop**:

- FFI via `[[extern(C)]]`
- repr(C) for compatible layouts
- Raw pointers in unsafe blocks

[62] **Implementation obligations documented in**: Conformance Requirements subsections throughout Clauses 1-16.

---

**End of Clause 0: Quick Start Guide**

**Next**: [§1 Introduction and Conformance](01_Introduction-and-Conformance/)
