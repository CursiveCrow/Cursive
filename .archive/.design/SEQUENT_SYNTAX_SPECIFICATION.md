# Contractual Sequent Syntax Specification

## Official Syntax (Version 1.0)

### Semantic Brackets

Cursive uses **semantic brackets** `⟦ ⟧` (Unicode U+27E6, U+27E7) or ASCII equivalent `[[ ]]` for contractual sequents.

**Rationale:**
- Consistent with semantic bracket notation for type value sets: `⟦ bool ⟧ = {true, false}`
- Standard notation in formal semantics and programming language theory
- Signals "formal/semantic specification content"
- LLM-friendly - trained on this notation from PL literature

---

## Full Syntax

```cursive
procedure name(params): ReturnType
    [[ grants |- preconditions => postconditions ]]
{ body }
```

Where:
- `grants` = Comma-separated list of capability tokens (e.g., `io::write, alloc::heap`)
- `|-` = Turnstile (ASCII for ⊢), denoting logical entailment
- `preconditions` = Boolean expression that must be true when called
- `=>` = Implication (ASCII for ⇒), denoting logical consequence
- `postconditions` = Boolean expression guaranteed true when procedure completes

---

## Smart Defaulting Rules

### Rule 1: Pure Functions Omit Sequent Entirely

**The most important rule:**

```cursive
// Pure function - NO SEQUENT NEEDED!
procedure add(a: i32, b: i32): i32
{
    result a + b
}

// Implicitly expands to: [[ |- true => true ]]
```

**When to omit:**
- No grants required
- No preconditions
- No postconditions
- Pure computation

**50%+ of procedures benefit from this!**

### Rule 2: Grant-Only Form

```cursive
// Written:
procedure print(text: string)
    [[ io::write ]]
{ println(text) }

// Expands to:
procedure print(text: string)
    [[ io::write |- true => true ]]
{ println(text) }
```

**Use when:**
- Procedure needs grants (effects)
- No preconditions or postconditions
- ~30% of procedures

### Rule 3: Precondition-Only Form

```cursive
// Written:
procedure divide(a: i32, b: i32): i32
    [[ |- b != 0 ]]
{ result a / b }

// Expands to:
procedure divide(a: i32, b: i32): i32
    [[ |- b != 0 => true ]]
{ result a / b }
```

**Use when:**
- Enforcing caller obligations
- No grants or specific postconditions

### Rule 4: Postcondition-Only Form

```cursive
// Written:
procedure increment(~!)
    [[ |- => self.value > @old(self.value) ]]
{ self.value += 1 }

// Expands to:
procedure increment(~!)
    [[ |- true => self.value > @old(self.value) ]]
{ self.value += 1 }
```

**Use when:**
- Guaranteeing results
- No grants or preconditions

### Rule 5: Full Sequent Form

```cursive
// Full form - nothing expanded
procedure deposit(~!, amount: i64)
    [[ ledger::post |- amount > 0 => self.balance >= amount ]]
{
    self.balance += amount
}
```

**Use when:**
- Need all three components
- ~20% of procedures

### Rule 6: No Grants Form (Optional Turnstile)

```cursive
// With turnstile (explicit)
procedure safe_divide(a: i32, b: i32): i32
    [[ |- b != 0 => result == a / b ]]
{ result a / b }

// Without turnstile (implicit - sugar)
procedure safe_divide(a: i32, b: i32): i32
    [[ b != 0 => result == a / b ]]
{ result a / b }

// Both expand to same form internally
```

**Use when:**
- No grants needed
- Have preconditions and/or postconditions
- Turnstile can be omitted (parsed from context)

---

## Complete Expansion Table

| Written Syntax | Expands To | Canonical Form | Use Case |
|----------------|------------|----------------|----------|
| *(omitted)* | `[[ |- true => true ]]` | Pure | Pure functions (50%) |
| `[[ G ]]` | `[[ G |- true => true ]]` | Grant-only | Effects without contracts (30%) |
| `[[ |- P ]]` | `[[ |- P => true ]]` | Precondition | Caller obligations |
| `[[ |- => Q ]]` | `[[ |- true => Q ]]` | Postcondition | Guarantees |
| `[[ G |- P ]]` | `[[ G |- P => true ]]` | Grants + pre | Effect with precondition |
| `[[ G |- => Q ]]` | `[[ G |- true => Q ]]` | Grants + post | Effect with guarantee |
| `[[ P => Q ]]` | `[[ |- P => Q ]]` | Pre + post | Contract without effects |
| `[[ G |- P => Q ]]` | (no expansion) | Full sequent | Complete contract (20%) |

---

## Expression-Bodied Procedures

**Syntax:**
```cursive
procedure name(params): ReturnType = expression ;
```

**Rules:**
1. Expression body implies pure procedure
2. Sequent is implicitly `[[ |- true => true ]]`
3. **Cannot** include explicit sequent (error E05-408)

**Example:**
```cursive
// ✓ Valid
procedure double(x: i32): i32 = x * 2 ;

// ✗ Invalid
procedure double(x: i32): i32 [[ io::write ]] = x * 2 ;  // error[E05-408]
```

---

## Real-World Examples

### Before (Old Syntax)

```cursive
public procedure main(): i32
    {| io::write |- true => true |}
{
    println("Hello, Cursive!")
    result 0
}

procedure add(a: i32, b: i32): i32
    {| |- true => true |}
{
    result a + b
}

procedure execute_query(sql: string@View): [i32]
    {| query |- sql.len() > 0 => true |}
{
    result perform_query(sql)
}

procedure File.open(self: unique Self@Closed, path: string@View): Self@Open
    {| fs::open |- path.exists() => result.state() == @Open |}
{
    let handle = os::open(path)
    result FileHandle@Open { handle, path: path.to_owned() }
}
```

### After (New Syntax)

```cursive
public procedure main(): i32
    [[ io::write ]]
{
    println("Hello, Cursive!")
    result 0
}

procedure add(a: i32, b: i32): i32
{
    result a + b
}

procedure execute_query(sql: string@View): [i32]
    [[ query |- sql.len() > 0 ]]
{
    result perform_query(sql)
}

procedure File.open(self: unique Self@Closed, path: string@View): Self@Open
    [[ fs::open |- path.exists() => result.state() == @Open ]]
{
    let handle = os::open(path)
    result FileHandle@Open { handle, path: path.to_owned() }
}
```

**Improvements:**
- ✅ Pure function: 19 tokens → **0 tokens**
- ✅ Grant-only: 27 tokens → **12 tokens**
- ✅ Full contract: Unchanged length but clearer brackets
- ✅ Maintains full formal sequent calculus structure

---

## Grammar (EBNF)

```ebnf
procedure_signature
    ::= generic_params? "(" parameter_list? ")" return_clause? sequent_clause?

sequent_clause
    ::= "[[" sequent_expr "]]"

sequent_expr
    ::= grant_list "|-" precond "=>" postcond    // Full form
    |   grant_list "|-" precond "=>"             // Omit postcond (defaults to true)
    |   grant_list "|-" "=>" postcond            // Omit precond (defaults to true)
    |   grant_list "|-" precond                  // Omit => postcond
    |   grant_list "|-"                          // Just grants (true => true)
    |   "|-" precond "=>" postcond               // No grants
    |   "|-" precond "=>"                        // No grants, no postcond
    |   "|-" "=>" postcond                       // No grants, no precond
    |   "|-" precond                             // Precond only
    |   precond "=>" postcond                    // No turnstile (no grants)
    |   grant_list                               // Grant-only (sugar)

grant_list
    ::= grant_path ("," grant_path)*

precond
    ::= assertion_expr

postcond
    ::= assertion_expr
```

---

## For LLM Reasoning

**Internal canonical form (what LLMs see):**

All sequents, regardless of surface syntax, are normalized to:

```
Γ; ε ⊢ P ⇒ Q
```

Where:
- `Γ` = Type environment
- `ε` = Grant set
- `P` = Preconditions
- `Q` = Postconditions

**Surface syntax → Canonical form mapping:**

| Surface | Canonical |
|---------|-----------|
| *(omit)* | `∅ ⊢ ⊤ ⇒ ⊤` |
| `[[ io::write ]]` | `{io::write} ⊢ ⊤ ⇒ ⊤` |
| `[[ |- x > 0 ]]` | `∅ ⊢ (x > 0) ⇒ ⊤` |
| `[[ g |- P => Q ]]` | `{g} ⊢ P ⇒ Q` |

**This means:**
- Humans write ergonomic surface syntax
- Compiler normalizes to canonical form
- LLMs reason about formal sequent calculus
- No loss of formal power!

---

## Comparison with Other Languages

| Language | Syntax | Formalism Level |
|----------|--------|-----------------|
| **Cursive** | `[[ grants |- pre => post ]]` | ✓✓✓ Full sequent calculus |
| Dafny | `requires P ensures Q` | ✓✓ Keywords only |
| Ada/SPARK | `Pre => P, Post => Q` | ✓✓ Keywords with =>  |
| F* | Refinement types `{P}` | ✓✓✓ Dependent types |
| Hoare Logic | `{P} C {Q}` | ✓✓✓ Full formal |
| Coq | Lemmas/theorems | ✓✓✓ Proof objects |

**Cursive uniquely combines:**
- Sequent calculus formalism (like logic papers)
- Practical surface syntax (like Dafny/Ada)
- Smart defaults (better than all others)

---

## Migration from Old Syntax

### Automated Migration Rules

```
{| |- true => true |}  →  (delete - pure function)
{| io::write |}         →  [[ io::write ]]
{| G |- P => Q |}       →  [[ G |- P => Q ]]
{| |- P => Q |}         →  [[ |- P => Q ]]
```

### Migration Tool (Planned)

```bash
cursive migrate --sequent-syntax-v2
```

Automatically:
1. Replaces `{| |}` with `[[ ]]`
2. Removes `[[ |- true => true ]]` from pure functions
3. Applies smart defaulting rules
4. Preserves formatting and comments

---

## Benefits Summary

### For Programmers

✅ **Pure functions have zero boilerplate** - Most common case is cleanest
✅ **Effects are explicit and minimal** - `[[ io::write ]]` is clear
✅ **Full power when needed** - Complete sequent calculus available
✅ **Progressive disclosure** - Complexity scales with needs

### For LLMs

✅ **Formal sequent calculus** - Standard logical notation (`⊢`, `⇒`)
✅ **Compositional semantics** - Grant contexts compose formally
✅ **Proof-friendly** - Maps directly to verification conditions
✅ **Consistent notation** - `⟦ ⟧` used for all formal content

### For Implementers

✅ **Unambiguous parsing** - Context-sensitive but deterministic
✅ **Simple desugaring** - Clear expansion rules
✅ **Verification-ready** - Direct mapping to proof obligations
✅ **Backward compatible** - Can support old syntax temporarily

### For Specification

✅ **Mathematically grounded** - Standard formal semantics notation
✅ **Cursive-consistent** - Already use `⟦ ⟧` for value sets
✅ **ISO-appropriate** - Clear, formal, well-defined
✅ **Reduced verbosity** - Specification examples cleaner

---

## Conformance

### Implementations Shall

1. Accept sequent clauses in form `[[ sequent_expr ]]`
2. Accept Unicode `⟦ ⟧` and ASCII `[[ ]]` as equivalent
3. Apply smart defaulting rules as specified
4. Treat omitted sequents as `[[ |- true => true ]]`
5. Reject explicit sequents on expression-bodied procedures (E05-408)
6. Normalize all forms to canonical sequent internally

### Implementations May

1. Pretty-print `[[ ]]` as `⟦ ⟧` in output/diagnostics
2. Suggest sequent simplifications (e.g., recommend omitting `[[ |- true => true ]]`)
3. Display expanded forms in IDE tooltips
4. Support old `{| |}` syntax with deprecation warnings (transitional)

---

## Examples By Category

### Pure Functions (50% - Omit Sequent)

```cursive
procedure add(a: i32, b: i32): i32 { result a + b }
procedure max(a: i32, b: i32): i32 { result if a > b { a } else { b } }
procedure identity<T>(value: T): T { result value }
```

### Effects Only (30% - Grant-Only Form)

```cursive
procedure print_line(text: string) [[ io::write ]] { println(text) }
procedure load_config() [[ fs::read ]] { result parse_config_file() }
procedure allocate_buffer() [[ alloc::heap ]] { result Buffer::new() }
```

### Preconditions (10% - Precondition Form)

```cursive
procedure divide(a: i32, b: i32): i32 [[ |- b != 0 ]] { result a / b }
procedure access_array(arr: [i32], idx: usize) [[ |- idx < arr.len() ]] { result arr[idx] }
```

### Full Contracts (10% - Full Sequent Form)

```cursive
procedure deposit(~!, amount: i64)
    [[ ledger::post |- amount > 0 => self.balance >= amount ]]
{
    self.balance += amount
}

procedure File.open(self: unique Self@Closed, path: string@View): Self@Open
    [[ fs::open |- path.exists() => result.state() == @Open ]]
{
    let handle = os::open(path)
    result FileHandle@Open { handle, path: path.to_owned() }
}

procedure verify_transfer(from: Account, to: Account, amount: i64): bool
    [[ ledger::read |- 
       from.balance >= amount && amount > 0 =>
       from.balance + to.balance == @old(from.balance + to.balance) ]]
{
    // Verification logic - sum preserved invariant
}
```

---

## Formal Semantics

### Denotational Interpretation

The semantic brackets denote the **formal meaning** of a procedure:

```
⟦ procedure f(x: T): U [[ ε |- P => Q ]] { body } ⟧

= λx: T. { requires: P, ensures: Q, effects: ε, implementation: ⟦ body ⟧ }
```

This parallels type value sets:

```
⟦ i32 ⟧ = {-2³¹, ..., 2³¹-1}
⟦ bool ⟧ = {true, false}
⟦ procedure f ⟧ = [ε |- P => Q]
```

**Creating a unified semantic notation family!**

---

## Implementation Notes

### Parsing Strategy

1. **Lexer**: Recognize `[[` and `]]` as distinct tokens (not two `[` or `]`)
2. **Parser**: Build sequent AST node with three optional fields
3. **Desugarer**: Apply expansion rules to fill defaults
4. **Type Checker**: Validate against canonical `[ε |- P => Q]` form

### AST Representation

```rust
struct Sequent {
    grants: GrantSet,         // Empty set if omitted
    preconditions: Formula,   // `true` if omitted
    postconditions: Formula,  // `true` if omitted
}

// All surface forms normalize to this
```

### Diagnostic Integration

When reporting sequent-related errors, show:
1. Surface syntax (what user wrote)
2. Expanded form (what compiler sees)
3. Violation (what rule failed)

Example:
```
error[E05-406]: procedure with empty grant set attempts grant-requiring operation
  --> example.cursive:5:12
   |
 5 | procedure add(a: i32, b: i32): i32
   |           ^^^ sequent: [[ |- true => true ]] (pure, no grants)
 6 | {
 7 |     println("{}", a + b)  // requires io::write grant
   |     ^^^^^^^ operation requires grant 'io::write'
   |
help: add required grant to sequent
   |
 5 | procedure add(a: i32, b: i32): i32 [[ io::write ]]
   |                                    +++++++++++++++
```

---

## Summary

**New contractual sequent syntax:**

- **Brackets**: `[[ ]]` (semantic brackets, ASCII for `⟦ ⟧`)
- **Optional**: Pure functions omit sequent entirely
- **Smart defaults**: Omit redundant `true` values
- **Formal**: Maintains full sequent calculus structure
- **LLM-friendly**: Standard notation from formal semantics
- **Ergonomic**: 50% less boilerplate for typical code

**Result: Maximum formalism with minimum verbosity!**

