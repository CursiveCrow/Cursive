# Cursive Language Specification

## Clause 7 — Expressions

**Clause**: 7 — Expressions  
**File**: 08-4_Structured-Expressions.md  
**Section**: §7.4 Structured Expressions  
**Stable label**: [expr.structured]  
**Forward references**: §8.5 [expr.pattern], §8.7 [expr.constant], Clause 5 §5.5 [decl.type], Clause 7 §7.3 [type.composite], Clause 9 §9.2 [stmt.control], Clause 10 [generic]

---

### §7.4 Structured Expressions [expr.structured]

#### §7.4.1 Overview

[1] Structured expressions construct composite values (records, tuples, arrays, enums) and perform control flow (`if`, `match`, `loop`). They introduce nested scopes, bindings, and evaluation paths that interact with patterns (§8.5) and the typing discipline (§8.8). Grammar references: Annex A §A.4.10–§A.4.16.

#### §7.4.2 Record, Tuple, and Enum Construction

**Record literals**

[2] Grammar: `RecordExpr ::= Path '{' FieldInit (',' FieldInit)* '}'` (Annex A §A.4.10).

[3] Typing:

- Let record `R` declare fields `{f₁: τ₁, …, fₙ: τₙ}`. A literal `R { f_i : e_i }` is well-formed iff each field appears exactly once, no extra fields are supplied, and `Γ ⊢ e_i : τ_i`.
- Shorthand `{ field }` expands to `{ field: field }`.
- Field visibility is enforced: referencing an internal/private field from another module emits E08-402.

[4] Evaluation: evaluate field expressions left-to-right as written (not declaration order). The record value is then formed with the evaluated fields. Temporaries drop after the literal completes.

**Tuple literals**

[5] Grammar: `TupleExpr ::= '(' Expr ',' Expr (',' Expr)* ')'` (Annex A §A.4.11).

[6] Typing: `Γ ⊢ (e₁,…,eₙ) : (τ₁,…,τₙ)` with `n ≥ 2`. Single-element tuple syntax is prohibited (E08-403); use parentheses or arrays as appropriate.

[7] Evaluation: evaluate elements left-to-right, storing them in a contiguous tuple layout (Clause 7 §7.3.2).

**Enum variants**

[8] Grammar: `EnumExpr ::= Path | Path '(' ExprList ')' | Path '{' FieldInit* '}'` (Annex A §A.4.12).

[9] Typing: the referenced variant must belong to the enum. Payload expressions shall match the variant’s tuple or record payload types. Missing or extra payload elements emit E08-404.

#### §7.4.3 Array Literals and Repetition

[10] Grammar: `ArrayLiteral ::= '[' Expr (',' Expr)* ']' | '[' Expr ';' ConstExpr ']'` (Annex A §A.4.13).

[11] Typing:

- Elements must share the same type `τ`. If the literal appears in a context with type `[τ; n]`, the number of expressions shall equal `n`.
- Repeat form `[value; n]` requires `τ : Copy` and `n` to be comptime evaluable (§8.7). Violations produce E08-430/E08-431.

[12] Evaluation: element expressions evaluate left-to-right. Repeat form evaluates `value` once and duplicates it `n` times.

#### §7.4.4 Conditional Expressions (`if` and `if let`)

[13] Grammar: `IfExpr ::= 'if' Expr Block ('else' Clause)?`, `IfLetExpr ::= 'if' 'let' Pattern '=' Expr Block ('else' Block)?` (Annex A §A.4.14).

[14] Typing rules:

- `if`: condition must have type `bool`. Branch blocks shall produce a common type `τ`; if `else` is omitted, `τ` must be `()`.
- `if let`: pattern typing follows §8.5. Bindings introduced by the pattern exist only inside the `then` block. Both branches shall converge to the same type; `else` may be omitted only when the expression is used in a `()` context.

[15] Guards on `if let` (`if let pattern = expr if guard`) shall have type `bool` and are evaluated only after the pattern succeeds.

#### §7.4.5 Match Expressions

[16] Grammar: `MatchExpr ::= 'match' Expr '{' MatchArm (',' MatchArm)* '}'` (Annex A §A.4.15).

[17] Semantics:

- The scrutinee expression is evaluated once.
- Arms are tried sequentially. Each arm consists of `pattern => expr` or `pattern if guard => expr`.
- Pattern typing follows §8.5. Each arm expression shall yield a common result type `τ`.
- Guards evaluate after the pattern matches and may use bindings introduced by the pattern.

[18] Type annotation requirement: when a match expression's result type cannot be inferred unambiguously, an explicit type annotation shall be provided using a typed hole (`let output: _ = match …`). The compiler shall infer the match result type when all arms produce compatible types; when inference fails or arms produce union types, the programmer may provide an explicit annotation or allow the hole to trigger diagnostic E08-002 requesting clarification.

[19] Exhaustiveness: matches over enums, unions, modal states, and booleans must cover all constructors. Non-exhaustive matches emit E07-451 with a list of missing cases. Catch-all `_` arms satisfy the requirement.

[20] Reachability: arms shadowed by earlier arms are reported via E08-452.

#### §7.4.6 Loop Expressions

[21] Grammar: `LoopExpr ::= 'loop' Block | 'loop' Expr Block | 'loop' Pattern ':' Type 'in' Expr Block` (Annex A §A.4.16).

**Infinite loops**

[22] `loop { body }` has type `!` (never) unless a `break value` appears. When all break statements supply the same type `τ`, the loop expression has type `τ`. Inconsistent break types produce E08-460.

**Conditional loops**

[23] `loop condition { body }` requires `condition : bool`. The loop type is `()` unless breaks specify otherwise.

**Iterator loops**

[24] Syntax enforces explicit iterator annotation: `loop item: ItemType in iterator_expr { … }`. Missing `ItemType` yields E08-461. The iterator expression shall satisfy the iteration protocol defined by Clause 11; otherwise E08-462 is emitted. Inside the loop, `item` has type `ItemType` and inherits permissions from the iterator yield.

**Break/continue**

[25] `break` exits the nearest loop, optionally yielding a value. `continue` restarts the current loop iteration. Using either outside a loop is ill-formed (E08-463).

#### §7.4.7 Closure Expressions and Capture [expr.structured.closure]

##### §7.4.7.1 Overview

[26] Closure expressions create anonymous callable values that can capture variables from enclosing scopes. They are first-class values of function type (§7.4) and enable higher-order programming patterns.

##### §7.4.7.2 Syntax

**Closure expressions** match the pattern:
```
"|" [ <parameter_list> ] "|" [ <sequent_clause> ] <block_expr>
```

[ Note: See Annex A §A.4 [grammar.expression] for the normative `closure_expr` production.
— end note ]

[27] Closures use vertical bars `|...|` to delimit parameters (which may be empty). The sequent clause is optional and defaults to `[[ |- true => true ]]` (pure closure). The block expression forms the closure body.

##### §7.4.7.3 Free Variable Identification [expr.structured.closure.free]

[28] A variable is _free_ in a closure if it is referenced within the closure body but not declared within the closure itself (not a parameter, not a local binding within the closure).

**Example:**

```cursive
let factor = 2.0
let scale = |value| [[ |- true => true ]] {
    result value * factor  // `factor` is free (captured), `value` is bound (parameter)
}
```

##### §7.4.7.4 Capture Modes [expr.structured.closure.modes]

[29] Free variables are automatically captured. The capture mode depends on the binding category of the captured variable:

**Table 8.4.1 — Closure capture modes**

| Binding Category | Capture Mode           | Semantics                                       |
| ---------------- | ---------------------- | ----------------------------------------------- |
| `let x = value`  | By reference (default) | Closure holds reference to binding location     |
| `let x <- value` | By reference           | Closure holds non-responsible reference         |
| `var x = value`  | By reference           | Closure holds mutable reference                 |
| `var x <- value` | By reference           | Closure holds non-responsible mutable reference |
| Parameters       | By reference           | Closure references parameter storage            |

[30] **Explicit move capture.** When a closure body explicitly uses `move x` for a captured responsible binding, cleanup responsibility transfers to the closure environment and the original binding becomes invalid:

```cursive
let data = Buffer::new()
let consumer = || [[ |- true => true ]] {
    process(move data)  // Explicit move transfers responsibility to closure
}
// data is now invalid (moved into closure environment)
```

[31] Without explicit `move`, all captures default to by-reference mode, preserving the binding's validity in the outer scope while allowing the closure to access or mutate the value.

##### §7.4.7.5 Environment Record Synthesis [expr.structured.closure.environment]

[32] The compiler synthesizes an environment record type `_Closure_N` containing fields for each captured variable. The closure value is represented as:

```cursive
// Conceptual representation (compiler-internal)
record _Closure_N {
    captured_var1: Type1,
    captured_var2: Type2,
    // ... one field per capture
}
```

[33] The closure callable has type `(Params) -> ReturnType ! Grants` and is implemented as a procedure taking the environment record plus declared parameters.

##### §7.4.7.6 Copy Behavior for Closures [expr.structured.closure.copy]

[34] A closure is `Copy` only when:

1. The environment record is empty (no captures), OR
2. Every captured value satisfies `Copy` AND is captured by immutable reference

[35] Most closures with captures are move-only. Empty closures (capturing no variables) may be freely copied.

##### §7.4.7.7 Constraints

[36] **Permission requirements.** Captured bindings must have permissions compatible with closure usage:

- Closures mutating captured `var` bindings require those bindings to have mutable permission
- Capturing `unique` bindings by reference requires the unique binding to be temporarily inactive while the closure holds the reference
- Closures escaping the capturing scope must not capture stack-local references (escape analysis applies)

[37] **Grant propagation.** The closure's grant set includes the union of:

- Grants declared in the closure's sequent clause
- Grants required by captured procedure calls within the closure body
- Grants from any subexpressions in the closure

##### §7.4.7.8 Diagnostics

| Code    | Condition                                                          |
| ------- | ------------------------------------------------------------------ |
| E08-470 | Closure captures moved-from value                                  |
| E08-471 | Closure outlives captured reference (escape violation)             |
| E08-472 | Closure captures unique binding while uniqueness is active         |
| E08-473 | Closure permission incompatible with captured variable permissions |

##### §7.4.7.9 Examples

**Example 8.4.7.1 (Closure capturing var binding by reference):**

```cursive
procedure make_counter(): () -> i32
    [[ alloc::heap |- true => true ]]
{
    var count = 0
    result || [[ |- true => true ]] {
        count += 1
        result count
    }
    // Closure captures `count` by reference; each call mutates the same counter
}
```

**Example 8.4.7.2 (Closure with explicit move capture):**

```cursive
let data = Buffer::from("hello")
let consumer = || [[ alloc::heap |- true => true ]] {
    consume(move data)  // Cleanup responsibility transfers to closure
}
// data is invalid here (moved into closure environment)
consumer()  // Executes with moved data
```

**Example 8.4.7.3 (Multiple captures with mixed modes):**

```cursive
let config = load_config()       // let binding (immutable)
var counter = 0                  // var binding (mutable)

let process = |input: string@View| [[ io::write |- true => true ]] {
    counter += 1                 // counter captured by reference (mutable)
    let prefix = config.prefix   // config captured by reference (immutable)
    println("{}{}", prefix, input)
}

process("test")  // Both counter and config remain valid
```

**Example 8.4.7.4 - invalid (Capturing moved value):**

```cursive
let data = Buffer::new()
consume(move data)
let closure = || [[ |- true => true ]] {
    data.size()  // error[E08-470]: cannot capture moved-from value
}
```

#### §7.4.8 Scope interaction and temporaries

[38] Structured expressions introduce nested lexical scopes. Bindings declared inside an `if`, `match`, or `loop` block follow the scope rules in Clause 6 §6.2. Temporaries drop at scope exit in reverse order of their creation (Clause 12 §12.2). Developers shall not rely on unspecified destruction order across branches; each branch is analysed separately for move/drop semantics.

#### §7.4.8 Diagnostics summary

| Code    | Condition                                                                  |
| ------- | -------------------------------------------------------------------------- |
| E08-400 | Record literal missing required field                                      |
| E08-401 | Record literal duplicates field                                            |
| E08-402 | Record literal references field not visible in scope                       |
| E08-403 | Single-element tuple literal disallowed                                    |
| E08-404 | Enum variant payload mismatch                                              |
| E08-430 | Array literal elements have mismatched types                               |
| E08-431 | Repeat array requires `Copy` element or comptime length                    |
| E08-440 | `if` expression requires `else` but none provided                          |
| E08-450 | Match result type inference failed (use typed hole or explicit annotation) |
| E07-451 | Match is non-exhaustive                                                    |
| E08-452 | Match arm unreachable                                                      |
| E08-460 | Loop break values disagree                                                 |
| E08-461 | Iterator loop missing element type annotation                              |
| E08-462 | Iterator does not satisfy iteration protocol                               |
| E08-463 | `break`/`continue` used outside loop                                       |

#### §7.4.9 Canonical examples

**Example 8.4.9.1 (Record literal with explicit fields).**

```cursive
let profile = UserProfile {
    id: request.user_id,
    name: request.name.clone(),
    is_admin: request.flags.contains(Flag::Admin),
}
```

**Example 8.4.9.2 (Match with guards and union return type).**

```cursive
let action: Order \/ Error = match command {
    Command::Buy(req) if req.amount > 0 => execute_buy(req),
    Command::Sell(req) => execute_sell(req),
    Command::Cancel(id) => cancel(id),
}
```

**Example 8.4.9.3 (Iterator loop).**

```cursive
loop entry: LogEntry in log_stream.records() {
    if entry.level == Level::Warning {
        warn(entry.message)
    }
}
```

[39] The iterator loop explicitly states the item type, ensuring both the compiler and readers understand the type flowing through the loop.

---
