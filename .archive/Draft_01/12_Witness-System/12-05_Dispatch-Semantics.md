# Cursive Language Specification

## Clause 12 — Witness System

**Clause**: 12 — Witness System
**File**: 13-5_Dispatch-Semantics.md
**Section**: §12.5 Dispatch Semantics
**Stable label**: [witness.dispatch]  
**Forward references**: §13.4 [witness.representation], §13.6 [witness.memory], Clause 10 [generic], Clause 12 [contract]

---

### §12.5 Dispatch Semantics [witness.dispatch]

#### §12.5.1 Overview

[1] _Dispatch_ is the process of invoking a procedure on a witness value. Witness dispatch is dynamic: the concrete implementation is determined at runtime by loading a function pointer from the witness vtable and performing an indirect call.

[2] This subclause specifies method invocation syntax, vtable lookup mechanics, the dispatch algorithm, performance characteristics, and comparison with static monomorphization.

#### §12.5.2 Method Invocation Syntax [witness.dispatch.syntax]

[3] Witness method invocation uses the scope operator `::` (same as all procedure calls):

**Witness method calls** match the pattern:
```
<witness_expression> "::" <method_identifier> "(" [ <argument_list> ] ")"
```

[4] The syntax is identical to static procedure calls. The witness type determines that dispatch is dynamic.

**Example 13.5.2.1 (Witness method calls)**:

```cursive
let w: witness<Display> = shape

w::show()                    // Dispatch to show()
let name = w::get_name()     // Dispatch to get_name()
w::render(canvas)            // Dispatch with arguments
```

#### §12.5.3 Dispatch Algorithm [witness.dispatch.algorithm]

[5] The dispatch algorithm for `witness::method(args)`:

```
dispatch_witness_method(witness, method_name, args):
    // 1. Load witness components
    data_ptr = witness.data_ptr
    metadata_ptr = witness.metadata_ptr

    // 2. Load witness table
    wtable = load(metadata_ptr)

    // 3. Lookup method in vtable
    method_fn = wtable.vtable[method_name]
    if method_fn == null:
        panic("method not found")  // Should not happen if type-checked

    // 4. Indirect call through function pointer
    result = method_fn(data_ptr, args)

    return result
```

[6] **Dispatch typing rule**:

[ Given: Witness $w : \texttt{witness}\langle B \rangle$, procedure $m$ in behavior $B$ ]

$$
\frac{\Gamma \vdash w : \texttt{witness}\langle B \rangle \quad B \text{ declares } m : (\tau_1, \ldots) \to \tau_r}
     {\Gamma \vdash w\texttt{::}m(args) : \tau_r}
\tag{T-Witness-Dispatch}
$$

[7] The method must exist in the behavior/contract being witnessed. Calling non-existent methods produces diagnostic E13-020 during type checking.

#### §12.5.4 Dynamic Dispatch Evaluation [witness.dispatch.evaluation]

[8] The evaluation semantics for witness dispatch:

[ Given: Witness $w$ wrapping concrete value $v : T$ ]

$$
\frac{
\begin{array}{c}
w = \{\text{data}: v, \text{metadata}: \text{wtable}\} \\
\text{wtable.vtable}[m] = f_m \\
\langle f_m(v, args), \sigma \rangle \Downarrow \langle \text{result}, \sigma' \rangle
\end{array}
}{
\langle w\texttt{::}m(args), \sigma \rangle \Downarrow \langle \text{result}, \sigma' \rangle
}
\tag{E-Witness-Dispatch}
$$

[9] The concrete function $f_m$ is the implementation of method $m$ for the concrete type $T$. The dispatch is resolved at runtime via vtable lookup.

**Example 13.5.4.1 (Dispatch evaluation)**:

```cursive
// Static (type known):
let point = Point { x: 1.0, y: 2.0 }
point::show()
// Compiles to: Point::show(&point)  // Direct call

// Dynamic (type erased):
let w: witness<Display> = point
w::show()
// Compiles to:
// let vtable = load(w.metadata_ptr)
// let show_fn = vtable.vtable["show"]
// show_fn(w.data_ptr)  // Indirect call through function pointer
```

#### §12.5.5 Method Resolution [witness.dispatch.resolution]

[10] Method resolution for witnesses follows behavior/contract declarations:

**For behavior witnesses**:

- Methods are those declared in the behavior (§10.4)
- Includes inherited methods from extended behaviors
- Overrides in concrete implementation are reflected in vtable

**For contract witnesses**:

- Methods are those required by the contract (Clause 12)
- Includes inherited methods from extended contracts
- Type's implementation provides vtable entries

**For modal state witnesses**:

- Methods are those available in the witnessed state (§7.6)
- State-specific procedures only
- Attempting to call methods from different states produces compile-time diagnostic

**Example 13.5.5.1 (Modal state method resolution)**:

```cursive
modal FileHandle {
    @Closed {
        procedure open(~!): FileHandle@Open
        [[ io::open |- true => true ]]
    }

    @Open {
        procedure read(~, buffer: [u8]): FileHandle@Open
        [[ io::read |- true => true ]]

        procedure close(~!): FileHandle@Closed
        [[ io::close |- true => true ]]
    }
}

let w: witness<FileHandle@Open> = file

w::read(buffer)  // ✅ OK: read available in @Open
w::close()       // ✅ OK: close available in @Open
// w::open()     // ❌ ERROR E13-021: open not available in @Open (only in @Closed)
```

#### §12.5.6 Performance Characteristics [witness.dispatch.performance]

[11] Witness dispatch has the following performance profile:

**Per method call**:

- 1 metadata pointer load (likely cached)
- 1 vtable entry load (function pointer)
- 1 indirect call (branch predictor penalty possible)

**Comparison to static dispatch**:

- Static: 0 indirections (direct call, inlinable)
- Witness: 1-2 pointer loads + 1 indirect call

**Optimization opportunities**:

- Vtable loads can be CSE'd (common subexpression elimination) across multiple calls
- Branch prediction can reduce indirect call overhead for stable patterns
- Monomorphization erasure eliminates all overhead when concrete type known

[12] The overhead is small but non-zero. Programs should prefer static dispatch (generics) unless dynamic behavior is actually required.

#### §12.5.7 Grant Propagation [witness.dispatch.grants]

[13] Witness method calls propagate grants from the method's sequent to the calling context:

$$
\frac{B \text{ declares } m \; \texttt{[[} G \texttt{|-} \cdots \texttt{]]} \quad w : \texttt{witness}\langle B \rangle}
     {w\texttt{::}m() \text{ requires grants } G}
\tag{Witness-Grant-Propagate}
$$

[14] The caller must have all grants required by the method being called, just as with static dispatch.

**Example 13.5.7.1 (Grant propagation through witnesses)**:

```cursive
behavior Processor {
    procedure process(~)
        [[ io::write, alloc::heap |- true => true ]]
    { }
}

procedure run_processor(w: witness<Processor>)
    [[ io::write, alloc::heap |- true => true ]]
{
    w::process()  // Requires io::write, alloc::heap from Processor::process
}

procedure invalid_caller(w: witness<Processor>)
    [[ |- true => true ]]
{
    // w::process()  // ❌ E12-030: missing grants io::write, alloc::heap
}
```

#### §12.5.8 Diagnostics [witness.dispatch.diagnostics]

[15] Dispatch diagnostics:

| Code    | Condition                                   | Section |
| ------- | ------------------------------------------- | ------- |
| E13-020 | Method not found in behavior/contract       | [7]     |
| E13-021 | Method not available in modal state         | [10]    |
| E13-022 | Insufficient grants for witness method call | [14]    |

#### §12.5.9 Conformance Requirements [witness.dispatch.requirements]

[16] Implementations shall:

1. Support witness method calls using `::` operator
2. Implement dynamic dispatch via vtable lookup
3. Load function pointers from witness tables
4. Perform indirect calls through vtable entries
5. Type-check method existence at compile time
6. Verify method availability for modal state witnesses
7. Propagate grants from method sequents to calling context
8. Optimize witness dispatch when concrete type known (optional)
9. Emit diagnostics E13-020, E13-021, E13-022 for dispatch violations
10. Maintain type safety throughout dispatch process

---

**Previous**: §13.4 Representation and Erasure (§13.4 [witness.representation]) | **Next**: §13.6 Regions, Permissions, and Grants Interplay (§13.6 [witness.memory])
