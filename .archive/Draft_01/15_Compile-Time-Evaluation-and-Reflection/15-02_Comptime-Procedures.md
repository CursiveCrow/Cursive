# Cursive Language Specification

## Clause 15 — Compile-Time Evaluation and Reflection

**Part**: XV — Compile-Time Evaluation and Reflection  
**File**: 16-2_Comptime-Procedures.md  
**Section**: §15.2 Comptime Procedures  
**Stable label**: [comptime.procedures]  
**Forward references**: §16.3 [comptime.blocks], §16.4 [comptime.intrinsics], §16.6 [comptime.reflect.queries], §16.8 [comptime.codegen.api], Clause 2 §2.2 [lex.phases], Clause 5 §5.4 [decl.function], Clause 8 §8.7 [expr.constant], Clause 12 [contract], Annex A §A.6 [grammar.declaration]

---

### §15.2 Comptime Procedures [comptime.procedures]

#### §15.2.1 Overview [comptime.procedures.overview]

[1] _Comptime procedures_ are procedures that execute exclusively during compilation. They enable pure, deterministic computation at compile time to produce constants, drive code generation, and perform type introspection.

[2] This subclause extends §8.7 [expr.constant] (constant expressions) and §5.4 [decl.function] (procedure declarations) with the semantics specific to compile-time execution: grant restrictions, purity requirements, and termination guarantees.

[3] Comptime procedures differ from runtime procedures in execution context and permitted operations. They share the same declaration syntax (§5.4) with the addition of the `comptime` keyword modifier.

#### §15.2.2 Syntax [comptime.procedures.syntax]

[4] Comptime procedure syntax extends procedure declarations (§5.4.2):

**Comptime procedure declarations** match the pattern:
```
[ <attributes> ] [ <visibility> ] "comptime" "procedure" <identifier>
    [ <generic_params> ] "(" [ <parameter_list> ] ")" [ ":" <type> ] [ <sequent_clause> ]
    <callable_body>
```

**Callable bodies** take one of the following forms:
```
<block_stmt>
"=" <expression>
```

[ Note: See Annex A §A.6 [grammar.declaration] for the normative `procedure_decl` production with `comptime` modifier.
— end note ]

[5] The `comptime` keyword appears immediately before `procedure`. All other syntax elements (parameters, return types, sequents, bodies) follow standard procedure rules.

[6] Expression-bodied form (`= expression`) is permitted for simple comptime procedures and implies pure sequent `[[ |- true => true ]]`.

#### §15.2.3 Constraints [comptime.procedures.constraints]

##### §15.2.3.1 Grant Restrictions

[7] _Comptime-safe grants only._ Comptime procedures shall declare only compile-time-safe grants in their contractual sequents. The permitted grants are:

- `comptime::alloc` — Compile-time memory allocation
- `comptime::codegen` — Code generation operations
- `comptime::config` — Configuration and platform queries
- `comptime::diag` — Diagnostic emission

[8] **Grant restriction rule:**

[ Given: Comptime procedure with sequent grants $G$ ]

$$
\frac{G \not\subseteq \{\texttt{comptime::alloc}, \texttt{comptime::codegen}, \texttt{comptime::config}, \texttt{comptime::diag}\}}{\text{ERROR E16-001: runtime grant in comptime procedure}}
\tag{WF-Comptime-Grants}
$$

[9] Attempting to declare runtime grants (`alloc::heap`, `fs::read`, `io::write`, `net::connect`, `thread::spawn`, etc.) in comptime procedure sequents produces diagnostic E16-001.

**Example 16.2.3.1 - invalid** (Runtime grant in comptime procedure):

```cursive
comptime procedure read_config_file(): string@View
    [[ fs::read |- true => true ]]  // error[E16-001]
{
    fs::read_to_string("config.toml")
}
```

[1] The `fs::read` grant is a runtime grant and cannot be used in comptime procedures.

##### §15.2.3.2 Purity Requirement

[10] _No observable side effects._ Comptime procedures shall be pure: they may not perform operations with observable runtime side effects beyond code generation (which is controlled by the `comptime::codegen` grant).

[11] Prohibited operations in comptime procedures:

- File system operations (reading/writing files)
- Network operations
- Process spawning or termination
- Accessing mutable static variables
- FFI calls
- Unsafe operations (except comptime-safe transmute for type conversions)

[12] Attempting prohibited operations produces diagnostic E16-002 identifying the specific violation.

##### §15.2.3.3 Termination Requirement

[13] _Bounded execution._ Comptime procedures shall terminate within implementation-defined resource limits:

- Recursion depth: minimum 256 levels
- Evaluation steps: minimum 1,000,000 per procedure invocation
- Memory allocation: minimum 64 MiB per compilation unit

[14] These limits align with the compile-time execution phase requirements in §2.2.3. Exceeding limits produces diagnostics E16-003 (recursion), E16-004 (steps), or E16-005 (memory).

##### §15.2.3.4 Type Restrictions

[15] _Return type evaluability._ Comptime procedure return types shall be compile-time representable:

- Primitive types (integers, floats, bool, char, unit)
- Strings (`string@View` for compile-time constants)
- Arrays of compile-time representable types
- Tuples of compile-time representable types
- TypeRef, ProcedureSpec, TypeSpec (codegen types)
- Union types where all components are compile-time representable

[16] Modal types with non-trivial states, witness types, and other runtime-only types cannot be returned from comptime procedures. Violations produce diagnostic E16-008.

##### §15.2.3.5 Parameter Restrictions

[17] _Comptime parameter types._ Parameters to comptime procedures shall have compile-time representable types (same constraint as return types, [15]).

[18] _No receivers._ Comptime procedures shall not have receiver parameters (`~`, `~%`, `~!`). They are always static. Attempting to declare a comptime procedure with a receiver produces diagnostic E16-009.

#### §15.2.4 Semantics [comptime.procedures.semantics]

##### §15.2.4.1 Execution Environment

[19] Comptime procedures execute in a compile-time evaluation environment separate from runtime execution:

[ Given: Comptime procedure `p` with parameters $x_1: \tau_1, \ldots, x_n: \tau_n$ and body $e$ ]

$$
\frac{\Gamma_{\text{ct}}, x_1: \tau_1, \ldots, x_n: \tau_n \vdash e : \tau_r \quad \langle e, \sigma_{\emptyset} \rangle \Downarrow_{\text{comptime}} \langle v, \sigma' \rangle}{\text{comptime}(p)(v_1, \ldots, v_n) = v}
\tag{E-Comptime-Proc-Call}
$$

[20] The compile-time store $\sigma$ contains bindings created during comptime execution. It is separate from the runtime store and is discarded after compilation completes.

##### §15.2.4.2 Grant Checking

[21] Grant checking for comptime procedures follows the general grant checking algorithm (§12.7.2) with the additional restriction that all grants must be comptime-safe:

$$
\frac{\text{procedure } p \text{ sequent grants } G \quad G \subseteq \text{ComptimeGrants}}{\text{Comptime grant check passes}}
\tag{WF-Comptime-Grant-Valid}
$$

where $\text{ComptimeGrants} = \{\texttt{comptime::alloc}, \texttt{comptime::codegen}, \texttt{comptime::config}, \texttt{comptime::diag}\}$.

##### §15.2.4.3 Recursion and Control Flow

[22] Comptime procedures may use recursion and all control flow constructs (if, match, loop) subject to recursion depth limits (§16.2.3.3).

[23] **Recursive comptime procedure:**

**Example 16.2.4.1** (Recursive factorial):

```cursive
comptime procedure factorial(n: usize): usize
    [[ |- n <= 20 => result > 0 ]]
{
    if n <= 1 {
        result 1
    } else {
        result n * factorial(n - 1)
    }
}

let FACT_10: const usize = factorial(10)  // Evaluates at compile time
```

[1] Recursion is bounded by the depth limit (256 levels). Factorial(30) would exceed limit for typical stack depths.

##### §15.2.4.4 Generic Comptime Procedures

[24] Comptime procedures may be generic over types, const parameters, and grant parameters:

**Example 16.2.4.2** (Generic comptime procedure):

```cursive
comptime procedure array_repeat<T, const N: usize>(value: T): [T; N]
    [[ comptime::alloc |- N > 0, N <= 10000 => true ]]
{
    var result: [T; N] = [value; N]
    result result
}

let ZEROS: const [i32; 100] = array_repeat::<i32, 100>(0)
```

#### §15.2.5 Comptime Procedure Invocation [comptime.procedures.invocation]

##### §15.2.5.1 Call Contexts

[25] Comptime procedures may be called from:

- Other comptime procedures
- Comptime blocks (`comptime { ... }`)
- Const binding initializers: `let X: const T = comptime_proc()`
- Array length expressions: `[T; comptime_size()]`
- Generic const parameter defaults
- Attribute arguments

[26] Calling comptime procedures from runtime contexts produces diagnostic E16-010.

##### §15.2.5.2 Evaluation Timing

[27] Comptime procedure calls evaluate during the compile-time execution phase (§2.2.4.2), in dependency order. Each call is memoized: repeated calls with identical arguments return cached results.

##### §15.2.5.3 Examples

**Example 16.2.5.1** (Comptime procedure in array length):

```cursive
comptime procedure next_power_of_two(n: usize): usize
    [[ |- n > 0 => result >= n ]]
{
    var power: usize = 1

    loop power < n {
        power = power * 2
    }

    result power
}

type AlignedBuffer<const SIZE: usize> = [u8; next_power_of_two(SIZE)]

let buffer: AlignedBuffer<100> = [0; 128]  // SIZE=100 → 128
```

**Example 16.2.5.2** (Comptime procedure in const binding):

```cursive
comptime procedure compute_checksum(data: [u8]): u32
    [[ comptime::alloc |- data.len() > 0 => true ]]
{
    var sum: u32 = 0

    loop byte: u8 in data {
        sum = sum + (byte as u32)
    }

    result sum
}

let HEADER: const [u8; 4] = [0x4D, 0x5A, 0x00, 0x00]
let HEADER_CHECKSUM: const u32 = compute_checksum(HEADER)
```

#### §15.2.6 Comptime-Only Data Structures [comptime.procedures.structures]

##### §15.2.6.1 Compile-Time Collections

[28] Comptime procedures may manipulate collections (arrays, tuples) allocated with `comptime::alloc`:

**Example 16.2.6.1** (Building lookup table):

```cursive
comptime procedure build_lookup_table<const N: usize>(): [u8; N]
    [[ comptime::alloc |- N > 0, N <= 256 => true ]]
{
    var table: [u8; N] = [0; N]
    var i: usize = 0

    loop i < N {
        table[i] = ((i * i) % 256) as u8
        i = i + 1
    }

    result table
}

let SQUARES: const [u8; 256] = build_lookup_table::<256>()
```

##### §15.2.6.2 Comptime String Manipulation

[29] String operations in comptime procedures use `string@View` for immutable compile-time strings:

**Example 16.2.6.2** (String concatenation at compile time):

```cursive
comptime procedure build_message(prefix: string@View, suffix: string@View): string@View
    [[ comptime::alloc |- prefix.len() + suffix.len() < 1048576 => true ]]
{
    result string_concat(prefix, suffix)
}

let GREETING: const string@View = build_message("Hello, ", "Cursive!")
```

#### §15.2.7 Diagnostics [comptime.procedures.diagnostics]

[30] Comptime procedure diagnostics:

**Table 16.2 — Comptime procedure diagnostics**

| Code    | Condition                                      | Constraint |
| ------- | ---------------------------------------------- | ---------- |
| E16-001 | Runtime grant in comptime procedure sequent    | [8]        |
| E16-002 | Non-pure operation in comptime procedure       | [12]       |
| E16-003 | Recursion depth exceeded (>256)                | [14]       |
| E16-004 | Evaluation steps exceeded (>1M)                | [14]       |
| E16-005 | Memory allocation exceeded (>64MiB)            | [14]       |
| E16-008 | Return type not compile-time representable     | [16]       |
| E16-009 | Comptime procedure with receiver               | [18]       |
| E16-010 | Comptime procedure called from runtime context | [26]       |

#### §15.2.8 Conformance Requirements [comptime.procedures.requirements]

[31] A conforming implementation SHALL:

1. Support `comptime procedure` declarations with syntax per §16.2.2
2. Restrict comptime procedure grants to comptime-safe grants only (E16-001)
3. Enforce purity: prohibit runtime-observable side effects (E16-002)
4. Enforce minimum resource limits: 256 recursion depth, 1M steps, 64MiB memory
5. Execute comptime procedures during compile-time execution phase (§2.2.4.2)
6. Memoize comptime procedure results for identical arguments
7. Support generic comptime procedures (type, const, grant parameters)
8. Allow comptime procedures in const bindings, array lengths, type expressions
9. Prohibit comptime procedures with receivers (E16-009)
10. Emit diagnostics E16-001 through E16-010 for violations

[32] A conforming implementation MAY:

1. Exceed minimum resource limits
2. Provide additional comptime intrinsics
3. Optimize comptime evaluation for performance
4. Cache results across compilation units

[33] A conforming implementation SHALL NOT:

1. Allow runtime grants in comptime procedures
2. Execute comptime procedures non-deterministically
3. Permit non-terminating comptime procedures without diagnostics
4. Allow comptime procedures to observe or modify runtime state (except via codegen)

---

**Previous**: §16.1 Overview and Integration (§16.1 [comptime.overview]) | **Next**: §16.3 Comptime Blocks and Contexts (§16.3 [comptime.blocks])
