# Cursive Language Specification

## Clause 8 — Statements and Control Flow

**Part**: VIII — Statements and Control Flow
**File**: 09-2_Simple-Statements.md  
**Section**: §8.2 Simple Statements  
**Stable label**: [stmt.simple]  
**Forward references**: §4.2 [decl.variable], §4.7 [decl.initialization], Clause 5 [name], Clause 6 [type], Clause 7 [expr], Clause 10 [memory]

---

### §8.2 Simple Statements [stmt.simple]

#### §8.2.1 Overview

[1] Simple statements perform basic operations without introducing complex control flow. They include variable declarations, assignments, expression evaluation for side effects, explicit empty statements, and defer registration for cleanup.

[2] This subclause specifies execution semantics for these forms. Syntactic details for variable declarations reside in Clause 5 §5.2; this subclause defines only their execution behavior as statements.

#### §8.2.2 Variable Declaration Statements [stmt.simple.vardecl]

##### §8.2.2.1 Syntax

[3] Variable declaration statements follow the grammar in §5.2 [decl.variable]. The forms are:

```ebnf
variable_declaration_stmt
    ::= binding_head pattern type_annotation? binding_operator initializer
```

[ Note: See Annex A §A.5 [grammar.statement] and §A.7 [grammar.declaration] for complete grammar.
— end note ]

[4] Variable declarations are statements (not expressions). They introduce bindings into the current scope and participate in definite assignment analysis (§5.7).

##### §8.2.2.2 Constraints

[5] All constraints from §5.2 apply: explicit shadowing requirements, single-assignment for `let`, type annotation requirements for multi-identifier patterns, and initializer completeness.

##### §8.2.2.3 Semantics

[6] Execution of a variable declaration statement:

[ Given: Binding $x$, initializer expression $e$, store $\sigma$ ]

$$
\frac{\langle e, \sigma \rangle \Downarrow \langle v, \sigma' \rangle}{\langle \texttt{let } x = e, \sigma \rangle \Downarrow \text{Normal}(\sigma'[x \mapsto v])}
\tag{E-Let}
$$

[7] The initializer expression evaluates to a value, which is bound to the identifier. The store is extended with the new binding. Execution continues normally.

[8] For `var` bindings, the semantics are identical except the binding is marked mutable, permitting subsequent reassignment (§9.2.3).

##### §8.2.2.4 Examples

**Example 9.2.2.1 (Variable declarations as statements):**

```cursive
procedure compute(input: i32): i32
    [[ |- true => true ]]
{
    let doubled = input * 2      // Immutable binding statement
    var accumulator = 0          // Mutable binding statement
    accumulator = doubled + 10   // Assignment (next subsection)
    result accumulator
}
```

#### §8.2.3 Assignment Statements [stmt.simple.assign]

##### §8.2.3.1 Overview

[9] Assignment statements modify the value stored at a place expression. They require the target to be a mutable location and the source expression to produce a compatible type.

##### §8.2.3.2 Syntax

[10] Assignment syntax follows:

```ebnf
assignment_stmt
    ::= place_expr "=" expression
     | place_expr "<-" expression
     | place_expr compound_op expression

compound_op
    ::= "+=" | "-=" | "*=" | "/=" | "%="
     | "&=" | "|=" | "^=" | "<<=" | ">>="
```

[ Note: See Annex A §A.5 [grammar.statement] for complete assignment grammar.
— end note ]

[11] The left operand must be a place expression (§8.1.4): a variable, field access, tuple projection, array index, or pointer dereference.

##### §8.2.3.3 Constraints

[12] **Mutability requirement.** The place expression shall refer to a `var` binding or a field of a structure accessed through a mutable permission. Assigning to `let` bindings produces diagnostic E08-101.

[13] **Type compatibility.** The right-hand expression type shall be compatible with the place type. Mismatches produce diagnostic E08-102 (type mismatch in assignment).

[14] **Permission requirement.** The place expression shall have permission allowing writes. Permission violations produce diagnostics specified in Clause 9.

[15] **Binding operator restriction.** Value assignment (`=`) may only assign to bindings originally created with `=` (responsible bindings). Reference assignment (`<-`) may only assign to bindings originally created with `<-` (non-responsible bindings). Mixing assignment operators produces diagnostic E08-103 (assignment operator mismatch).

##### §8.2.3.4 Semantics

[16] **Value assignment (`=`):**

[ Given: Place expression $p$, value expression $e$, store $\sigma$ ]

$$
\frac{\langle e, \sigma \rangle \Downarrow \langle v, \sigma' \rangle \quad p \mapsto \ell \text{ in } \sigma'}{\langle p = e, \sigma \rangle \Downarrow \text{Normal}(\sigma'[\ell \mapsto v])}
\tag{E-Assign-Value}
$$

[17] The right-hand expression evaluates first, then the place resolves to a location, then the value is stored. For responsible `var` bindings, the old value's destructor is invoked before the new value is stored.

[18] **Reference assignment (`<-`):**

[ Given: Non-responsible `var` binding $x$, value expression $e$, store $\sigma$ ]

$$
\frac{\langle e, \sigma \rangle \Downarrow \langle v, \sigma' \rangle \quad x \text{ is non-responsible}}{\langle x \texttt{ <- } e, \sigma \rangle \Downarrow \text{Normal}(\sigma'[x \mapsto v])}
\tag{E-Assign-Reference}
$$

[19] Reference assignment updates a non-responsible binding to refer to a different value. No destructor is invoked because the binding has no cleanup responsibility. The binding simply updates its reference to the new value.

[20] **Compound assignment:**

[ Given: Place $p$, operator $\oplus$, expression $e$ ]

$$
\frac{p \mapsto \ell \quad \sigma(\ell) = v_{\text{old}} \quad \langle e, \sigma \rangle \Downarrow \langle v_{\text{rhs}}, \sigma' \rangle \quad v_{\text{result}} = v_{\text{old}} \oplus v_{\text{rhs}}}{\langle p \oplus\!= e, \sigma \rangle \Downarrow \text{Normal}(\sigma'[\ell \mapsto v_{\text{result}}])}
\tag{E-Assign-Compound}
$$

[18] Compound assignment reads the current value, evaluates the right-hand expression, applies the operator, and stores the result. The place is evaluated once, preserving side-effect ordering.

##### §8.2.3.5 Examples

**Example 9.2.3.1 (Assignment statements):**

```cursive
var counter: i32 = 0
counter = 10                    // Simple assignment
counter += 5                    // Compound assignment: counter = counter + 5

var point = Point { x: 0.0, y: 0.0 }
point.x = 3.14                  // Field assignment
```

**Example 9.2.3.2 (Reference assignment to var):**

```cursive
var ref <- buffer1                     // Non-responsible binding
ref <- buffer2                         // Update reference (no cleanup)
ref <- buffer3                         // Update again (no cleanup)
```

**Example 9.2.3.3 - invalid (Assignment to immutable binding):**

```cursive
let constant = 42
constant = 100                  // error[E08-101]: cannot assign to immutable binding
```

**Example 9.2.3.4 - invalid (Assignment operator mismatch):**

```cursive
var data = Buffer::new()        // Responsible binding (created with =)
data <- other_buffer            // error[E08-103]: cannot use <- on responsible binding

var ref <- buffer               // Non-responsible binding (created with <-)
ref = other_buffer              // error[E08-103]: cannot use = on non-responsible binding
```

#### §8.2.4 Expression Statements [stmt.simple.expr]

##### §8.2.4.1 Overview

[19] Expression statements evaluate expressions solely for their side effects, discarding any produced value.

##### §8.2.4.2 Syntax

[20] Any expression from Clause 8 may be used as a statement:

```ebnf
expr_stmt
    ::= expression
```

[21] Expression statements are distinguished from expression-producing blocks by context: standalone expressions at statement position are expression statements.

##### §8.2.4.3 Constraints

[22] The expression shall be well-formed per Clause 8. No additional constraints apply.

##### §8.2.4.4 Semantics

[23] Expression statement execution:

[ Given: Expression $e$, store $\sigma$ ]

$$
\frac{\langle e, \sigma \rangle \Downarrow \langle v, \sigma' \rangle}{\langle e, \sigma \rangle \Downarrow \text{Normal}(\sigma')}
\tag{E-Expr-Stmt}
$$

[24] The expression evaluates and its value is discarded. Only side effects (store modifications, I/O) persist.

##### §8.2.4.5 Examples

**Example 9.2.4.1 (Expression statements for side effects):**

```cursive
procedure demo(data: Buffer)
    [[ io::write |- true => true ]]
{
    println("Processing data")   // Expression statement: procedure call
    data.clear()                  // Expression statement: method call
    compute_checksum(data)        // Value discarded (if non-unit type, may warn)
}
```

#### §8.2.5 Empty Statements [stmt.simple.empty]

##### §8.2.5.1 Overview

[25] Empty statements consist of a standalone semicolon or empty lines. They have no effect and serve as placeholders or formatting aids.

##### §8.2.5.2 Syntax

```ebnf
empty_stmt
    ::= ";"
```

##### §8.2.5.3 Semantics

[26] Empty statement execution:

$$
\frac{}{\langle ; , \sigma \rangle \Downarrow \text{Normal}(\sigma)}
\tag{E-Empty}
$$

[27] The store is unchanged; execution continues normally.

#### §8.2.6 Defer Statements [stmt.simple.defer]

##### §8.2.6.1 Overview

[28] Defer statements register blocks for execution at scope exit. Deferred blocks execute in LIFO (last-in, first-out) order, ensuring cleanup occurs on all control-flow paths including normal returns, early returns, breaks, and panics.

##### §8.2.6.2 Syntax

```ebnf
defer_stmt
    ::= "defer" block
```

[ Note: See Annex A §A.5 [grammar.statement] for complete defer grammar.
— end note ]

[29] The deferred block shall be a block statement with unit type. Value-producing blocks are ill-formed in defer context (diagnostic E08-120).

##### §8.2.6.3 Constraints

[30] **Unit type requirement.** The deferred block shall have type `()`. Blocks producing values trigger diagnostic E08-120.

[31] **No control transfer in defers.** Defer blocks shall not contain `return`, `break`, or `continue` statements referencing outer scopes. Violations produce diagnostic E08-121.

(31.1) Defer blocks may contain their own internal control flow (loops, conditional branches) provided control remains within the defer block.

##### §8.2.6.4 Semantics

[32] **Defer registration:**

[ Given: Defer block $b$, store $\sigma$ ]

$$
\frac{}{\langle \texttt{defer } b, \sigma \rangle \Downarrow \text{Normal}(\sigma)}
\tag{E-Defer-Register}
$$

[33] Executing a defer statement does not execute the block immediately. The block is registered in the current scope's defer queue for later execution.

[34] **Defer execution at scope exit:**

When a scope exits (block end, function return, break out of block), deferred blocks execute in reverse registration order:

[ Given: Defer queue $[d_1, \ldots, d_n]$ in registration order ]

$$
\frac{\langle d_n, \sigma \rangle \Downarrow \text{Normal}(\sigma_n) \quad \cdots \quad \langle d_1, \sigma_1 \rangle \Downarrow \text{Normal}(\sigma_{\text{final}})}{\text{execute defers} \Rightarrow \sigma_{\text{final}}}
\tag{E-Defer-Execute}
$$

[35] Defers execute after the triggering event (return value computed, break value computed) but before control transfers or the scope ends.

##### §8.2.6.5 Defer on Panic

[36] When a panic occurs, defer blocks execute during stack unwinding. If unwinding is disabled (implementation-defined), defers may not execute. Defer behavior during panic is specified in Clause 11 §11.2.5.5 [memory.object.destructor.panic].

##### §8.2.6.6 Examples

**Example 9.2.6.1 (Defer LIFO execution):**

```cursive
procedure demo()
    [[ io::write |- true => true ]]
{
    defer { println("First defer") }
    defer { println("Second defer") }
    defer { println("Third defer") }
    println("Body")
}
// Output order: Body, Third defer, Second defer, First defer
```

**Example 9.2.6.2 (Defer with early return):**

```cursive
procedure process_file(path: string@View): i32 \/ FileError
    [[ fs::open, fs::read |- path.len() > 0 => true ]]
{
    let file = open_file(path)
    defer { close_file(file) }   // Executes on BOTH return paths

    if file.is_empty() {
        result FileError::Empty  // defer executes before return
    }

    result read_size(file)       // defer executes before normal return
}
```

**Example 9.2.6.3 - invalid (Defer with value-producing block):**

```cursive
defer { result 42 }             // error[E08-120]: defer block must have unit type
```

#### §8.2.7 Diagnostics

[37] Diagnostics defined in this subclause:

| Code    | Condition                                                  |
| ------- | ---------------------------------------------------------- |
| E08-101 | Assignment to immutable (`let`) binding                    |
| E08-102 | Type mismatch in assignment                                |
| E08-120 | Defer block produces non-unit value                        |
| E08-121 | Defer block contains control transfer escaping defer scope |

#### §8.2.8 Conformance Requirements

[38] Implementations shall:

1. Execute variable declarations per §5.2, extending the environment and store
2. Enforce mutability and type constraints for assignments, emitting E08-101 and E08-102
3. Support compound assignment operators with single place evaluation
4. Discard values from expression statements, preserving only side effects
5. Support empty statements with no runtime cost
6. Register defer blocks for LIFO execution on all scope exits
7. Enforce unit-type and control-transfer constraints on defer blocks

---

**Previous**: §9.1 Statement Fundamentals (§9.1 [stmt.fundamental]) | **Next**: §9.3 Control Flow Statements (§9.3 [stmt.control])
