# Cursive Language Specification

## Clause 10 — Memory Model, Regions, and Permissions

**Part**: X — Memory Model, Regions, and Permissions
**File**: 11-5_Move-Copy-Clone.md  
**Section**: §10.5 Move, Copy, and Clone Semantics  
**Stable label**: [memory.move]  
**Forward references**: §9.2 [memory.object], §9.4 [memory.permission], Clause 4 §4.2 [decl.variable], Clause 6 [type], Clause 8 [generic]

---

### §10.5 Move, Copy, and Clone Semantics [memory.move]

#### §10.5.1 Overview

[1] This section specifies how values transfer between bindings, when cleanup responsibility transfers, and how types opt into copying behavior. Cursive uses explicit `move` for responsibility transfer and explicit `Copy`/`Clone` behaviors for duplication.

[2] The binding category (`let` vs `var`) combined with assignment operator (`=` vs `<-`) determines transfer behavior. Permissions (§11.4) are orthogonal to transfer semantics.

#### §10.5.2 Move Semantics [memory.move.move]

##### §10.5.2.1 Overview

[3] The `move` keyword explicitly transfers cleanup responsibility from one binding to another. After a move, the source binding becomes invalid.

##### §10.5.2.2 Syntax

[4] Move expression syntax:

**Move expressions** match the pattern:

```
"move" <identifier>
```

[ Note: See Annex A §A.4 [grammar.expression] for complete move syntax.
— end note ]

##### §10.5.2.3 Transfer Rules by Binding Category

[5] Transfer eligibility follows the binding semantics specified in §4.2.5 [decl.variable.operator], Table 4.2.5 in that section.

##### §10.5.2.4 Move Semantics

[7] Move execution:

[ Given: Source binding $x$ with cleanup responsibility ]

$$
\frac{\Gamma \vdash x : \tau \quad x \text{ is responsible and transferable}}{\Gamma \vdash \texttt{move } x : \tau \quad x \text{ becomes invalid}}
\tag{E-Move}
$$

[8] After `move x`:

- Cleanup responsibility transfers to the receiving binding
- `x` becomes invalid (moved-from state)
- Using `x` produces diagnostic E10-503 (use of moved value)

**Example 11.5.2.1 (Move transfers responsibility):**

```cursive
procedure consume(data: Buffer)
    [[ |- true => true ]]
{
    // data is responsible; will call destructor at end
}

procedure demo()
    [[ alloc::heap |- true => true ]]
{
    let buffer = Buffer::new()      // buffer is responsible
    consume(move buffer)             // Responsibility transferred to parameter
    // buffer is now invalid
    // buffer.size()                 // error[E10-503]: use of moved value
}
```

**Example 11.5.2.2 - invalid (Move from var):**

```cursive
var counter = Buffer::new()
consume(move counter)  // error[E10-501]: cannot transfer from var binding
```

[1] `var` bindings cannot transfer because allowing moves would require whole-function analysis to track whether the binding might be rebound after the move, violating Cursive's local reasoning principle.

#### §10.5.3 Copy Semantics [memory.move.copy]

##### §10.5.3.1 Copy Behavior

[9] Types satisfying the `Copy` behavior can be duplicated bitwise. Copying creates a new independent object with its own cleanup responsibility.

[10] **Copy behavior**: The `Copy` behavior is defined in Clause 8 (Generics and Behaviors). This section specifies how Copy interacts with the memory model.

[11] Types that satisfy `Copy`:

- All primitive types (§7.2): integers, floats, bool, char, ()
- Tuples and records where all fields satisfy `Copy`
- Types without custom destructors

[12] Types that do NOT satisfy `Copy`:

- Types with custom destructors
- Types containing non-Copy fields
- Most resource-managing types (File, Socket, Buffer)

##### §10.5.3.2 Copy Syntax

[13] Copying requires explicit `copy` keyword:

```cursive
let original: i32 = 42
let duplicate = copy original    // Explicit bitwise copy
```

[14] Attempting to copy non-Copy types produces diagnostic E10-510.

**Example 11.5.3.1 (Copy for primitive types):**

```cursive
let a: i32 = 10
let b = copy a       // OK: i32 satisfies Copy
let c = copy a       // OK: can copy multiple times

// All three bindings (a, b, c) are responsible for their own values
```

#### §10.5.4 Clone Semantics [memory.move.clone]

##### §10.5.4.1 Clone Behavior

[15] The `Clone` behavior enables deep copying for types that cannot use bitwise `Copy`. Clone creates a semantically equivalent but independent object. The `Clone` behavior is defined in Clause 8 (Generics and Behaviors).

[16] Types implement `Clone` by providing a `clone` method that performs type-specific duplication (allocating new resources, copying contents, etc.).

**Example 11.5.4.1 (Clone for complex types):**

```cursive
let original: Buffer = Buffer::from_data(data)
let cloned = original.clone()    // Deep copy: allocates new buffer

// Both original and cloned are responsible for their own buffers
```

#### §10.5.5 Moved-From State [memory.move.movedFrom]

##### §10.5.5.1 Invalid Binding State

[17] After `move x`, the binding `x` enters moved-from state. Definite assignment analysis (§4.7) tracks this state and prevents further use.

[18] **Moved-from constraint**:

[ Given: Binding $x$ in moved-from state ]

$$
\frac{x \text{ in moved-from state} \quad \text{attempt to use } x}{\text{ERROR E10-503: use of moved value}}
\tag{WF-No-Use-After-Move}
$$

##### §10.5.5.2 Invalidation of Derived Non-Responsible Bindings

[18.1] Non-responsible bindings become invalid when their source binding is moved to a responsible parameter. Complete invalidation rules and validity tracking are specified in §4.7.5 [decl.initialization].

##### §10.5.5.3 Rebinding Var Without Transfer

[19] While `var` bindings cannot be moved (they are non-transferable per §4.2.5 [decl.variable.operator]), they can be reassigned in place, with the old value being destroyed before the new value is bound:

**Example 11.5.5.3 (Var rebinding without move):**

```cursive
var data = Buffer::new()      // data is responsible
// consume(move data)         // error[E10-501]: cannot transfer from var

// However, var bindings can be reassigned:
data = Buffer::new()           // Old buffer destroyed, new buffer bound
data.size()                    // OK: data is valid with new buffer
```

[20] When rebinding a `var`, the old value (if valid) is automatically cleaned up before the new value is assigned. This is distinct from transfer: the binding retains cleanup responsibility for both the old and new values.

**Example 11.5.5.4 (Pattern: Use let + unique for mutable+transferable):**

```cursive
// When you need both mutation AND transfer:
let builder: unique = Builder::new()   // Use let + unique, not var
builder.add(1)                         // Can mutate via unique
builder.add(2)
consume(move builder)                  // Can transfer via let
```

[20] `let` bindings cannot be reassigned (§4.2); once moved, they remain invalid for their entire scope. See §4.2.5 Example 4.2.5.1 for binding operator semantics.

#### §10.5.6 Diagnostics

[21] [Note: Diagnostics defined in this subsection are cataloged in Annex E §E.5.1.10. — end note]

#### §10.5.7 Conformance Requirements

[22] Implementations shall integrate move tracking with definite assignment analysis (§4.7) and maintain orthogonality between transfer rules and permissions.

---

**Previous**: §11.4 Permissions (§11.4 [memory.permission]) | **Next**: §11.6 Layout and Alignment (§11.6 [memory.layout])
