# Cursive Language Specification

## Clause 11 — Contracts

**Clause**: 11 — Contracts
**File**: 12-6_Invariants.md
**Section**: §11.6 Invariants: where
**Stable label**: [contract.invariant]  
**Forward references**: §12.2 [contract.sequent], §12.5 [contract.postcondition], §12.7 [contract.checking], Clause 5 §5.5 [decl.type], Clause 7 §7.3 [type.composite], Clause 9 §9.3 [stmt.control]

---

### §11.6 Invariants: where [contract.invariant]

#### §11.6.1 Overview

[1] _Invariants_ are conditions that must hold persistently throughout an object's lifetime or at specific program points. Cursive uses the `where` keyword to declare invariants in multiple contexts: type declarations, loop statements, and potentially other scoping constructs.

[2] Invariants are syntactic sugar: they desugar to conjunctions in sequent postconditions. The `where { inv }` clause automatically extends postconditions of relevant procedures with `&& inv`, eliminating repetition and ensuring invariants are maintained automatically.

[3] This subclause specifies invariant syntax, desugaring rules, checking semantics, and the unified `where` terminology across all contexts.

#### §11.6.2 Type Invariants [contract.invariant.type]

##### §11.6.2.1 Overview

[4] Type invariants are declared on record, enum, and modal type bodies using `where` clauses. They specify conditions that must hold for all instances of the type at all observable program points.

##### §11.6.2.2 Syntax

[5] Type invariant syntax (from §5.5.3):

```ebnf
type_declaration
    ::= attribute* visibility? type_keyword identifier generic_params?
        type_body where_clause?

where_clause
    ::= "where" "{" invariant_list "}"

invariant_list
    ::= invariant_expression ("," invariant_expression)*

invariant_expression
    ::= expression
```

[6] Invariant expressions are boolean predicates that may reference type fields using `self` or field names directly.

**Example 12.6.2.1 (Type invariants)**:

```cursive
record BankAccount {
    balance: i64,
    overdraft_limit: i64,

    where {
        balance >= -overdraft_limit,
        overdraft_limit >= 0
    }
}

enum Status {
    Pending { priority: i32 },
    Active { started: Timestamp },
    Completed { duration: i64 },

    where {
        match self {
            Status::Pending { priority } => priority >= 0,
            Status::Active { started } => started.is_valid(),
            Status::Completed { duration } => duration > 0
        }
    }
}
```

##### §11.6.2.3 Desugaring Rules

[7] Type invariants automatically extend the postconditions of:

1. All constructors (record literal expressions, enum variant construction)
2. All mutating methods (procedures with receiver `~!` or `~%`)
3. All procedures that return instances of the type

[8] **Type invariant desugaring rule**:

[ Given: Type $T$ with invariant $I$ ]

$$
\frac{\text{record } T \{ \ldots \} \text{ where } \{ I \} \quad \text{procedure } m: T \; \texttt{[[} G \texttt{|-} P \texttt{=>} Q \texttt{]]}}
     {\text{Effective postcondition: } Q' = Q \land I}
\tag{Desugar-Type-Inv}
$$

[9] The compiler automatically conjoins the invariant with explicit postconditions. Programmers need not write the invariant in every method's will clause.

**Example 12.6.2.2 (Desugaring demonstration)**:

```cursive
record Counter {
    value: i32,

    where {
        value >= 0
    }

    // Original procedure sequent:
    procedure increment(~!)
        [[ |- true => true ]]
    {
        self.value += 1
    }

    // Compiler desugars to:
    // [[ |- true => (true && self.value >= 0) ]]
    // Simplified: [[ |- true => self.value >= 0 ]]
}

// Constructor literal
let counter = Counter { value: 0 }
// Desugars to check: counter.value >= 0

// Invalid construction
let invalid = Counter { value: -5 }
// Error: invariant violated (value >= 0)
```

##### §11.6.2.4 Checking Points

[10] Type invariants are verified at:

- **Construction**: Record/enum literal expressions
- **Mutation**: After any procedure modifying type fields (receivers `~!`, `~%`)
- **Assignment**: After field assignment statements
- **Return**: When procedures return instances of the type

[11] The compiler inserts checks according to verification mode (§12.8). Statically provable invariants have zero runtime cost; unprovable invariants insert assertions.

#### §11.6.3 Loop Invariants [contract.invariant.loop]

##### §11.6.3.1 Overview

[12] Loop invariants are conditions that must hold on loop entry, at the start of each iteration, and on loop exit. They are specified using `where` clauses on loop statements.

##### §11.6.3.2 Syntax

[13] Loop invariant syntax:

```ebnf
loop_statement
    ::= "loop" loop_condition? "where" "{" invariant_list "}" block

invariant_list
    ::= invariant_expression ("," invariant_expression)*
```

[14] Loop invariants may reference loop variables, parameters, and outer scope bindings visible at the loop site.

**Example 12.6.3.1 (Loop invariants)**:

```cursive
procedure sum_array(numbers: [i32]): i32
    [[ |- numbers.len() > 0 => result >= 0 ]]
{
    var total: i32 = 0
    var i: usize = 0

    loop i < numbers.len() where { total >= 0, i <= numbers.len() } {
        total = total + numbers[i]
        i = i + 1
    }

    result total
}
```

##### §11.6.3.3 Loop Invariant Verification

[15] Loop invariants are verified at three points:

1. **Entry**: Before first iteration
2. **Iteration**: At the start of each loop iteration
3. **Exit**: After loop completes

[16] **Loop invariant checking rule**:

[ Given: Loop with invariant $I$, loop body $B$ ]

$$
\frac{\text{Before loop: } I \quad \text{Each iteration: } I \land B \Rightarrow I \quad \text{After loop: } I}
     {\text{Loop invariant } I \text{ verified}}
\tag{WF-Loop-Inv}
$$

[17] The invariant must hold before entering the loop, be preserved by each iteration, and therefore hold upon exit.

**Example 12.6.3.2 (Loop invariant verification)**:

```cursive
procedure binary_search<T: Ord>(array: [T; n], target: T): i32 \/ None
    [[ |- array.is_sorted() => true ]]
{
    var low: usize = 0
    var high: usize = array.len()

    loop low < high where { low <= high, high <= array.len() } {
        let mid = (low + high) / 2
        match array[mid].compare(target) {
            cmp: i32 if cmp < 0 => { low = mid + 1 }
            cmp: i32 if cmp > 0 => { high = mid }
            _ => { result mid }
        }
    }
    result None { }
}

// Verification:
// Entry: low = 0, high = n → low <= high ✅, high <= n ✅
// Iteration: Updates preserve invariant
// Exit: Invariant still holds
```

#### §11.6.4 Invariant Desugaring [contract.invariant.desugaring]

##### §11.6.4.1 Type Invariant Desugaring

[18] For a type $T$ with invariant $I$, the compiler automatically extends sequents:

**Constructor desugaring**:

$$
\text{Literal: } T \{ \ldots \} \quad \Rightarrow \quad \texttt{check } I \text{ after construction}
$$

**Mutator desugaring**:

$$
\text{procedure } m(\sim!, \ldots) \; \texttt{[[} G \texttt{|-} P \texttt{=>} Q \texttt{]]} \quad \Rightarrow \quad \texttt{[[} G \texttt{|-} P \texttt{=>} Q \land I \texttt{]]}
$$

[19] The invariant $I$ is conjoined with the explicit postcondition $Q$. If $Q = \texttt{true}$, the effective postcondition becomes simply $I$.

##### §11.6.4.2 Loop Invariant Desugaring

[20] Loop invariants desugar to verification points:

```
loop condition where { I } body

Desugars to:

assert!(I)                    // Entry check
loop condition {
    assert!(I)                // Iteration check (start of each iteration)
    body
}
assert!(I)                    // Exit check
```

[21] Assertion insertion follows verification mode (§12.8). Static verification mode requires proving invariant maintenance at compile time.

#### §11.6.5 Constraints [contract.invariant.constraints]

[1] _Type requirement._ Invariant expressions shall have type `bool`. Non-boolean invariants produce diagnostic E12-060.

[2] _Purity requirement._ Invariants shall be pure expressions without side effects. Effectful invariants produce diagnostic E12-061.

[3] _Scope restrictions._ Type invariants may reference `self` and type fields. Loop invariants may reference loop variables and outer bindings. Referencing inaccessible bindings produces diagnostic E12-062.

[4] _No @old in invariants._ Invariants are not temporal predicates; they assert current state properties. Using `@old` in invariants produces diagnostic E12-063.

[5] _No result in type invariants._ Type invariants apply to all instances; `result` is specific to procedure returns. Using `result` in type invariants produces diagnostic E12-064.

[6] _Invariant maintainability._ For type invariants, the compiler shall verify that the invariant can be satisfied. Unsatisfiable invariants (e.g., `where { false }`) produce diagnostic E12-065 warning that the type is uninhabitable.

#### §11.6.6 Semantics [contract.invariant.semantics]

##### §11.6.6.1 Type Invariant Semantics

[7] A type invariant $I$ asserts that all observable instances of the type satisfy $I$. An instance is _observable_ after construction and between method calls.

[8] **Type invariant satisfaction**:

[ Given: Type $T$ with invariant $I$, instance $v: T$ ]

$$
\frac{\text{type } T \text{ where } \{ I \} \quad v : T \text{ observable}}
     {I[self \mapsto v] = \text{true}}
\tag{P-Type-Inv-Holds}
$$

[9] The invariant must hold for value $v$ at all observable points. Temporary violations during method execution (while `self` is uniquely held) are permitted; the invariant must be restored before the method returns.

**Example 12.6.6.1 (Temporary invariant violation)**:

```cursive
record SortedList<T: Ord> {
    elements: [T],

    where {
        is_sorted(elements)
    }

    procedure insert(~!, item: T)
        [[ alloc::heap |- true => is_sorted(self.elements) ]]
    {
        self.elements.push(item)      // Temporarily violates invariant
        self.elements.sort()           // Restores invariant
        // Exit: invariant checked → is_sorted(self.elements) ✅
    }
}
```

##### §11.6.6.2 Loop Invariant Semantics

[10] A loop invariant $I$ is an inductive assertion proved by:

1. **Base case**: $I$ holds before the first iteration
2. **Inductive case**: If $I$ holds at iteration start, it holds at iteration end
3. **Conclusion**: $I$ holds after loop exits

[11] **Loop invariant induction**:

$$
\frac{I_{entry} \quad \forall \text{iteration}. \; I_{start} \Rightarrow I_{end} \quad \lnot condition \Rightarrow I_{exit}}
     {\text{Loop invariant } I \text{ verified}}
\tag{Loop-Inv-Induction}
$$

**Example 12.6.6.2 (Loop invariant induction)**:

```cursive
procedure count_positive(numbers: [i32]): i32
    [[ |- true => result >= 0 ]]
{
    var count: i32 = 0
    var i: usize = 0

    loop i < numbers.len() where { count >= 0, i <= numbers.len() } {
        if numbers[i] > 0 {
            count = count + 1
        }
        i = i + 1
    }

    result count
}

// Verification:
// Entry: count = 0 → count >= 0 ✅, i = 0 → i <= len ✅
// Iteration: Assume (count >= 0, i <= len)
//            Execute body: count unchanged or +1 → count >= 0 ✅
//                         i = i + 1 → i <= len (by loop condition) ✅
// Exit: Invariant holds → count >= 0 ✅
```

#### §11.6.7 Automatic Invariant Checking [contract.invariant.automatic]

[12] The compiler automatically inserts invariant checks without programmer intervention:

**For type invariants**:

- After every constructor expression
- After every mutating method returns
- After field assignment to types with invariants

**For loop invariants**:

- Before loop entry
- At loop iteration boundaries (checked per verification mode)
- After loop exit

[13] Programmers write invariants once; the compiler enforces them everywhere automatically.

#### §11.6.8 Integration with Sequent Postconditions [contract.invariant.integration]

[14] When a procedure has both explicit postconditions and type invariants, they are conjoined:

**Example 12.6.8.1 (Invariant and postcondition conjunction)**:

```cursive
record Account {
    balance: i64,

    where {
        balance >= 0
    }

    procedure deposit(~!, amount: i64)
        [[
            ledger::post
            |-
            amount > 0
            =>
            self.balance >= amount      // Explicit postcondition
        ]]
    {
        self.balance += amount
    }
}

// Desugared sequent:
// [[
//     ledger::post
//     |-
//     amount > 0
//     =>
//     (self.balance >= amount) && (self.balance >= 0)
//     ^^^^^^^^^^^^^^^^^^^^^^^^    ^^^^^^^^^^^^^^^^^^^
//     Explicit                    Type invariant (automatic)
// ]]
```

[15] The conjunction occurs automatically. Both conditions are verified using the same verification strategy (static proof or dynamic checking).

#### §11.6.9 Constraints [contract.invariant.constraints]

[1] _Boolean type._ Invariant expressions shall have type `bool`. Non-boolean invariants produce diagnostic E12-060.

[2] _Purity._ Invariants shall be pure expressions. Effectful invariants produce diagnostic E12-061.

[3] _Scope validity._ Type invariants may reference `self` and type fields. Loop invariants may reference loop variables and outer bindings. Invalid references produce diagnostic E12-062.

[4] _No temporal operators._ Invariants assert current state; `@old` is not valid in where clauses. Violations produce diagnostic E12-063.

[5] _No result in type invariants._ The `result` identifier is procedure-specific; type invariants are type-wide. Using `result` in type where clauses produces diagnostic E12-064.

[6] _Satisfiability._ Implementations should warn when type invariants are unsatisfiable (e.g., `where { false }`). Diagnostic E12-065 indicates the type is uninhabitable.

#### §11.6.10 Examples [contract.invariant.examples]

**Example 12.6.10.1 (Comprehensive type invariant)**:

```cursive
record Rectangle {
    width: f64,
    height: f64,

    where {
        width > 0.0,
        height > 0.0
    }

    procedure area(~): f64
        [[ |- true => result > 0.0 ]]
    {
        result self.width * self.height
    }

    procedure scale(~!, factor: f64)
        [[ |- factor > 0.0 => true ]]
    {
        self.width = self.width * factor
        self.height = self.height * factor
        // Automatic check: width > 0.0 && height > 0.0
    }
}

// Construction
let rect = Rectangle { width: 10.0, height: 5.0 }
// Check: 10.0 > 0.0 && 5.0 > 0.0 ✅

// Invalid construction
let bad = Rectangle { width: -1.0, height: 5.0 }
// Error: invariant violated (width > 0.0 fails)
```

**Example 12.6.10.2 (Loop invariant with verification)**:

```cursive
procedure find_max(numbers: [i32]): i32
    [[ |- numbers.len() > 0 => result >= numbers[0] ]]
{
    var max: i32 = numbers[0]
    var i: usize = 1

    loop i < numbers.len() where {
        max >= numbers[0],
        i > 0,
        i <= numbers.len()
    } {
        if numbers[i] > max {
            max = numbers[i]
        }
        i = i + 1
    }

    result max
}
```

**Example 12.6.10.3 (Invariant on enum)**:

```cursive
enum Temperature {
    Celsius { value: f64 },
    Fahrenheit { value: f64 },
    Kelvin { value: f64 },

    where {
        match self {
            Temperature::Kelvin { value } => value >= 0.0,  // Absolute zero
            _ => true
        }
    }
}

// Valid
let abs_zero = Temperature::Kelvin { value: 0.0 }

// Invalid
let impossible = Temperature::Kelvin { value: -1.0 }
// Error: invariant violated (Kelvin value must be >= 0.0)
```

#### §11.6.11 Diagnostics [contract.invariant.diagnostics]

[16] Invariant diagnostics:

| Code    | Condition                                         | Constraint |
| ------- | ------------------------------------------------- | ---------- |
| E12-060 | Invariant expression has non-bool type            | [1]        |
| E12-061 | Invariant performs side effects (not pure)        | [2]        |
| E12-062 | Invariant references inaccessible binding         | [3]        |
| E12-063 | `@old` operator used in where clause              | [4]        |
| E12-064 | `result` identifier used in type invariant        | [5]        |
| E12-065 | Unsatisfiable type invariant (type uninhabitable) | [6]        |
| E12-066 | Invariant violated at construction                | [10]       |
| E12-067 | Invariant violated after mutation                 | [10]       |
| E12-068 | Loop invariant violated at entry                  | [15]       |
| E12-069 | Loop invariant not preserved by iteration         | [15]       |

#### §11.6.12 Conformance Requirements [contract.invariant.requirements]

[17] Implementations shall:

1. Parse `where { ... }` clauses on type declarations and loop statements
2. Desugar type invariants to postcondition conjunctions on constructors and mutators
3. Desugar loop invariants to entry/iteration/exit verification points
4. Enforce boolean type requirement for all invariants
5. Enforce purity requirement for all invariants
6. Validate scope restrictions (self/fields for types, loop vars for loops)
7. Reject `@old` and inappropriate `result` uses in invariants
8. Verify invariants at all specified checking points
9. Insert dynamic checks when static proof fails (per verification mode)
10. Warn about unsatisfiable type invariants
11. Emit diagnostics E12-060 through E12-069 for violations
12. Maintain invariant metadata for tooling and documentation

---

**Previous**: §12.5 Postconditions: will (§12.5 [contract.postcondition]) | **Next**: §12.7 Sequent Checking Flow (§12.7 [contract.checking])
