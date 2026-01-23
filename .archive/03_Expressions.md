# Part III: Expressions - §1. Loops

**Section**: §1 | **Part**: Expressions (Part III)
**Next**: [To be determined]

---

**Definition 1.1 (Loop Expression):** A loop is a unified control flow construct for iteration, supporting infinite loops, conditional loops, and pattern-based iteration with optional verification through invariants (`with`) and termination metrics (`by`). The loop expression evaluates its body repeatedly until a break condition is met, optionally yielding a value.

## 1. Loops

### 1.1 Overview

**Key innovation/purpose:** Cantrip provides a single `loop` keyword that handles all iteration patterns—infinite loops, while-style conditions, for-style iteration, and iterator-based loops—with built-in support for formal verification through loop invariants and variants.

**When to use loops:**
- Indefinite iteration (while-style: unknown number of iterations)
- Definite iteration (for-style: known range or collection)
- Infinite loops with explicit break conditions
- Verified iterative algorithms requiring correctness proofs
- State machines with repeated transitions
- Event loops and server loops

**When NOT to use loops:**
- Single conditional execution → use `if`
- Recursive algorithms → use recursive functions
- Collection transformations → use iterator methods (`map`, `filter`, `fold`)
- Parallel iteration → use concurrency primitives

**Relationship to other features:**
- **Break/Continue**: Loop control flow (§1.2)
- **Contracts**: Verification through invariants and variants
- **Iterators**: Pattern-based iteration over collections
- **Effect System**: Loops inherit effects from their body

### 1.2 Syntax

#### 1.2.1 Concrete Syntax

**Grammar:**
```ebnf
LoopExpr ::= "loop" LoopHead? LoopVerification? BlockExpr

LoopHead ::= LoopPattern
           | LoopCondition

LoopPattern ::= Pattern "in" Expr                  // For-style iteration

LoopCondition ::= Expr                             // While-style condition
                                                   // (when no "in" keyword)

LoopVerification ::= LoopVariant? LoopInvariant?

LoopVariant ::= "by" Expr                          // Termination metric

LoopInvariant ::= "with" PredicateBlock        // Loop invariants

PredicateBlock ::= Assertion                       // Single invariant
                 | "{" AssertionList "}"           // Multiple invariants

AssertionList ::= Assertion ("," Assertion)* ","?
```

**Disambiguation:**
- If the expression after `loop` contains the keyword `in`, it's a pattern
- Otherwise, it's a condition or infinite loop

**Loop styles (all using `loop` keyword):**

```cantrip
// Infinite loop
loop {
    body
}

// While-style (condition only)
loop condition {
    body
}

// For-style (pattern + range)
loop i in 0..n {
    body
}

// Iterator-style (pattern + collection)
loop item in collection {
    body
}

// With termination metric
loop condition by metric {
    body
}

// With invariants
loop condition
    with invariants;
{
    body
}

// Complete verification
loop pattern by metric
    with invariants;
{
    body
}
```

#### 1.2.2 Abstract Syntax

**Loop structure:**
```
loop ::= Loop(head, variant, invariants, body)

head ::= Infinite                         // No head
       | Condition(expr)                  // While-style
       | Pattern(pat, expr)               // For-style

variant   ::= Some(expr) | None          // Optional termination metric
invariants ::= [assertion]                // List of invariants (may be empty)
body       ::= block_expr                 // Loop body
```

**Invariant and variant positions:**
- `variant` (if present) appears inline with loop head
- `invariants` (if present) appear after variant, before body
- Both are optional for simple loops

#### 1.2.3 Basic Examples

**Example 1: Infinite loop**
```cantrip
loop {
    let command = read_command()
    if command == "quit" {
        break
    }
    process(command)
}
```

**Example 2: While-style loop**
```cantrip
var i = 0
loop i < 10 {
    println("{}", i)
    i += 1
}
```

**Example 3: For-style loop**
```cantrip
loop i in 0..10 {
    println("{}", i)
}
```

**Example 4: Iterator loop**
```cantrip
loop item in collection {
    process(item)
}
```

### 1.3 Static Semantics

#### 1.3.1 Well-Formedness Rules

**Definition 1.2 (Well-Formed Loop):** A loop expression is well-formed if its head, variant, invariants, and body are all well-formed in the appropriate context.

**[WF-Loop-Infinite] Well-formed infinite loop:**
```
Γ ⊢ body : τ
body may contain break
─────────────────────────
Γ ⊢ loop { body } well-formed
```

**[WF-Loop-Condition] Well-formed conditional loop:**
```
Γ ⊢ condition : bool
Γ ⊢ body well-formed
body may contain break or continue
───────────────────────────────────
Γ ⊢ loop condition { body } well-formed
```

**[WF-Loop-Pattern] Well-formed pattern loop:**
```
Γ ⊢ expr : Iterable<T>
Γ, pat : T ⊢ body well-formed
body may contain break or continue
─────────────────────────────────────
Γ ⊢ loop pat in expr { body } well-formed
```

**[WF-Loop-Variant] Well-formed with variant:**
```
Γ ⊢ loop_head well-formed
Γ ⊢ variant : τ where τ is orderable
body must decrease variant
────────────────────────────────────
Γ ⊢ loop head by variant { body } well-formed
```

**[WF-Loop-Invariant] Well-formed with invariants:**
```
Γ ⊢ loop_head well-formed
Γ ⊢ invariant_i : bool for each invariant
body must preserve all invariants
──────────────────────────────────────────
Γ ⊢ loop head with { inv₁, ..., invₙ } ; { body } well-formed
```

#### 1.3.2 Type Rules

**[T-Loop-Infinite] Infinite loop without break:**
```
[T-Loop-Infinite]
Γ ⊢ body : τ
no break in body
─────────────────────
Γ ⊢ loop { body } : Never
```

**Explanation:** An infinite loop that never breaks has type `Never` (diverges).

**[T-Loop-Break] Loop with break:**
```
[T-Loop-Break]
Γ ⊢ body : τ_body
break expressions in body have type τ_break
────────────────────────────────────────────
Γ ⊢ loop { body } : τ_break
```

**Explanation:** A loop's type is determined by the values yielded by `break` statements.

**Example:**
```cantrip
// Type: i32
let result: i32 = loop {
    let x = compute()
    if x > threshold {
        break x  // breaks with i32
    }
}
```

**[T-Loop-Condition] Conditional loop:**
```
[T-Loop-Condition]
Γ ⊢ condition : bool
Γ ⊢ body : τ_body
break expressions have type τ
──────────────────────────────
Γ ⊢ loop condition { body } : τ
```

**Explanation:** Condition must be boolean; loop type is break type (or `()` if no breaks).

**[T-Loop-Pattern] Pattern-based loop:**
```
[T-Loop-Pattern]
Γ ⊢ expr : Iterable<T>
Γ, pat : T ⊢ body : τ_body
break expressions have type τ
─────────────────────────────────
Γ ⊢ loop pat in expr { body } : τ
```

**Explanation:** Pattern is bound to iterator element type; loop type is break type.

#### 1.3.3 Type Properties

**Property 1.1 (Loop Body Effects):** A loop inherits all effects from its body.

```
effect(loop head { body }) = effect(body)
```

**Property 1.2 (Break Type Consistency):** All break expressions in a loop must have the same type.

```
If loop contains break e₁, ..., break eₙ
Then Γ ⊢ eᵢ : τ for all i, for some common type τ
```

**Property 1.3 (Variant Decrease):** If a loop has a variant `by expr`, then `expr` must be strictly decreasing across iterations.

```
If loop has variant by v
Then for each iteration: v_after < v_before
```

### 1.4 Dynamic Semantics

#### 1.4.1 Evaluation Rules

**[E-Loop-Infinite] Infinite loop evaluation:**
```
[E-Loop-Infinite]
⟨body, σ⟩ ⇓ ⟨σ'⟩  (no break)
⟨loop { body }, σ'⟩ ⇓ result
────────────────────────────────
⟨loop { body }, σ⟩ ⇓ result
```

**Explanation:** Execute body, then loop again (tail recursion).

**[E-Loop-Break] Loop with break:**
```
[E-Loop-Break]
⟨body, σ⟩ ⇓ break v, σ'
────────────────────────────
⟨loop { body }, σ⟩ ⇓ ⟨v, σ'⟩
```

**Explanation:** When body evaluates to `break v`, loop terminates with value `v`.

**[E-Loop-Continue] Loop with continue:**
```
[E-Loop-Continue]
⟨body, σ⟩ ⇓ continue, σ'
⟨loop { body }, σ'⟩ ⇓ result
─────────────────────────────────
⟨loop { body }, σ⟩ ⇓ result
```

**Explanation:** Continue skips to next iteration.

**[E-Loop-Condition-True] Condition loop (true):**
```
[E-Loop-Condition-True]
⟨condition, σ⟩ ⇓ ⟨true, σ₁⟩
⟨body, σ₁⟩ ⇓ ⟨σ₂⟩  (no break)
⟨loop condition { body }, σ₂⟩ ⇓ result
────────────────────────────────────────
⟨loop condition { body }, σ⟩ ⇓ result
```

**[E-Loop-Condition-False] Condition loop (false):**
```
[E-Loop-Condition-False]
⟨condition, σ⟩ ⇓ ⟨false, σ'⟩
─────────────────────────────────────────
⟨loop condition { body }, σ⟩ ⇓ ⟨(), σ'⟩
```

**Explanation:** When condition is false, loop exits with unit value.

**[E-Loop-Pattern] Pattern loop evaluation:**
```
[E-Loop-Pattern]
⟨expr, σ⟩ ⇓ ⟨iter, σ₁⟩
⟨iter.next(), σ₁⟩ ⇓ ⟨Some(v), σ₂⟩
pat matches v, binding Γ'
⟨body, σ₂, Γ'⟩ ⇓ ⟨σ₃⟩  (no break)
⟨loop pat in iter { body }, σ₃⟩ ⇓ result
──────────────────────────────────────────
⟨loop pat in expr { body }, σ⟩ ⇓ result
```

**[E-Loop-Pattern-End] Pattern loop exhaustion:**
```
[E-Loop-Pattern-End]
⟨expr, σ⟩ ⇓ ⟨iter, σ₁⟩
⟨iter.next(), σ₁⟩ ⇓ ⟨None, σ₂⟩
────────────────────────────────────────
⟨loop pat in expr { body }, σ⟩ ⇓ ⟨(), σ₂⟩
```

**Explanation:** When iterator is exhausted, loop exits with unit value.

#### 1.4.2 Memory Representation

**Loop frame:**
- Loop body creates a new stack frame per iteration
- Variables declared in loop body are scoped to that iteration
- Pattern bindings are fresh per iteration
- Loop head variables are evaluated once before loop

**Stack layout (per iteration):**
```
┌─────────────────────┐
│ Previous frame      │
├─────────────────────┤
│ Loop locals         │  ← body variables
├─────────────────────┤
│ Pattern bindings    │  ← pat bound variables
├─────────────────────┤
│ Condition/expr      │  ← evaluated per iteration
└─────────────────────┘
```

#### 1.4.3 Operational Behavior

**Evaluation order:**
1. Evaluate loop head (condition or iterator expression)
2. Check invariants (if verification enabled)
3. Execute body
4. Check variant decreased (if verification enabled)
5. On `continue`: go to step 1
6. On `break v`: exit loop with value `v`
7. On condition false or iterator exhaustion: exit with `()`

**Verification behavior:**
- Invariants checked:
  - Before first iteration
  - After each iteration (maintained)
  - At loop exit
- Variant checked:
  - Measured before iteration
  - Measured after iteration
  - Must strictly decrease

### 1.5 Loop Verification

#### 1.5.1 Loop Invariants (`with`)

**Definition 1.3 (Loop Invariant):** A loop invariant is a property that holds before the loop begins, remains true after each iteration, and holds when the loop terminates.

**Syntax:**
```cantrip
loop condition
    with invariant
{
    body
}

// Multiple invariants
loop condition
    with {
        inv1,
        inv2,
        inv3
    }
{
    body
}
```

**Verification conditions:**
```
[VC-Invariant-Init] Initialization:
precondition ⇒ invariant

[VC-Invariant-Maintain] Maintenance:
{invariant ∧ condition} body {invariant}

[VC-Invariant-Exit] Exit:
invariant ∧ ¬condition ⇒ postcondition
```

**Example: Array sum**
```cantrip
procedure sum_array(arr: [i32; n]): i32
    will result == sum_of(arr[0..n])
{
    var sum = 0
    var i = 0

    loop i < n
        with {
            0 <= i,
            i <= n,
            sum == sum_of(arr[0..i]),
        }
    {
        sum += arr[i]
        i += 1
    }

    sum
}
```

**Proof sketch:**
- **Init:** `i = 0`, `sum = 0` → invariants hold
- **Maintain:** After `sum += arr[i]; i += 1`, invariants still hold
- **Exit:** When `i == n`, `sum == sum_of(arr[0..n])` (postcondition)

#### 1.5.2 Loop Variants (`by`)

**Definition 1.4 (Loop Variant):** A loop variant is an expression that strictly decreases on each iteration, proving the loop terminates.

**Syntax:**
```cantrip
loop condition by variant {
    body
}

// With invariants
loop condition by variant
    with invariants;
{
    body
}
```

**Verification condition:**
```
[VC-Variant-Decrease] Variant must decrease:
{invariant ∧ condition ∧ variant = v₀} body {variant < v₀}

[VC-Variant-Bounded] Variant must be bounded below:
variant ≥ 0  (or some lower bound)
```

**Example: Binary search**
```cantrip
procedure binary_search(arr: [i32], target: i32): Option<usize> {
    var low = 0
    var high = arr.len()

    loop low < high by high - low
        with {
            0 <= low,
            high <= arr.len(),
            low <= high,
        }
    {
        let mid = (low + high) / 2
        if arr[mid] < target {
            low = mid + 1
        } else {
            high = mid
        }
    }

    if low < arr.len() && arr[low] == target {
        Some(low)
    } else {
        None
    }
}
```

**Proof sketch:**
- **Variant:** `high - low`
- **Bounded:** `low <= high` (from invariant) → `high - low >= 0`
- **Decreases:** 
  - Branch 1: `low' = mid + 1`, `high' = high` → `high' - low' < high - low` ✓
  - Branch 2: `low' = low`, `high' = mid` → `high' - low' < high - low` ✓
- **Conclusion:** Loop must terminate

#### 1.5.3 Optional Verification

**Verification is optional:**
- Simple loops don't need `with` or `by`
- Compiler attempts inference for common patterns
- Complex loops require explicit verification

**Inference examples:**
```cantrip
// Compiler can infer: by n - i
loop i in 0..n {
    process(i)
}

// Compiler can infer: by n - i
var i = 0
loop i < n {
    i += 1
}
```

**When inference fails:**
```
error[E9010]: cannot prove loop termination
  --> src/main.ct:42:5
   |
42 |     loop low < high {
   |     ^^^^^^^^^^^^^^^ loop variant required
   |
   = help: add `by <expr>` to specify decreasing metric
   = example: loop low < high by high - low { ... }
   = note: common patterns:
           - counting up:   by n - i
           - counting down: by i
           - binary search: by high - low
```

### 1.6 Advanced Features

#### 1.6.1 Pattern Matching in Loops

**Destructuring tuples:**
```cantrip
loop (key, value) in map {
    println("{}: {}", key, value)
}
```

**Enum matching:**
```cantrip
loop Ok(value) in results {
    // Only processes Ok variants, skips Err
    process(value)
}
```

**With guards:**
```cantrip
loop x in items where x > 0 {
    process_positive(x)
}
```

#### 1.6.2 Nested Loops with Verification

```cantrip
loop i in 0..n by n - i
    with { i >= 0, i <= n }
{
    loop j in 0..i by i - j
        with { j >= 0, j <= i }
    {
        matrix[i][j] = compute(i, j)
    }
}
```

**Note:** Each loop has its own invariants and variant.

#### 1.6.3 Loop Labels

```cantrip
'outer: loop i in 0..n {
    'inner: loop j in 0..m {
        if should_exit_all() {
            break 'outer
        }
        if should_skip_inner() {
            continue 'inner
        }
    }
}
```

### 1.7 Examples and Patterns

#### 1.7.1 Accumulation Pattern

**Pattern:** Iterate and accumulate a result.

```cantrip
procedure product(numbers: Vec<i32>): i32 {
    var result = 1

    loop num in numbers {
        result *= num
    }

    result
}
```

**When to use:** Reducing a collection to a single value.

#### 1.7.2 Search Pattern

**Pattern:** Find first element matching a condition.

```cantrip
procedure find_first<T>(items: Vec<T>, predicate: fn(T) -> bool): Option<T> {
    loop item in items {
        if predicate(item) {
            break Some(item)
        }
    }

    None
}
```

**When to use:** Early exit when condition is met.

#### 1.7.3 Verified Algorithm Pattern

**Pattern:** Iterative algorithm with correctness proof.

```cantrip
procedure gcd(a: u64, b: u64): u64
    must a > 0, b > 0
    will result divides a, result divides b
{
    var x = a
    var y = b

    loop y > 0 by y
        with {
            x > 0,
            y >= 0,
            gcd(x, y) == gcd(a, b),
        }
    {
        let temp = y
        y = x % y
        x = temp
    }

    x
}
```

**When to use:** Algorithms requiring formal verification.

#### 1.7.4 Event Loop Pattern

**Pattern:** Infinite loop processing events.

```cantrip
procedure event_loop(server: mut Server) {
    loop {
        match server.receive_event() {
            Event::Request(req) => handle_request(req),
            Event::Shutdown => break,
            Event::Error(e) => log_error(e)
        }
    }
}
```

**When to use:** Servers, UI loops, state machines.

---

**Previous**: [To be determined] | **Next**: [To be determined]
