# Cursive Language Specification

## Clause 6 — Types

**Part**: VI — Type System  
**File**: 07-2_Primitive-Types.md
**Section**: §6.2 Primitive Types  
**Stable label**: [type.primitive]  
**Forward references**: §6.3 [type.composite], §6.4 [type.function], §6.5 [type.pointer], §6.7 [type.relation], §7.4 [expr.structured], §7.3 [expr.operator], Clause 8 [generic], Clause 9 [memory]

---

### §6.2 Primitive Types [type.primitive]

[1] Primitive types are the built-in scalar types whose size, representation, and value sets are fixed by this specification. They form the base cases for well-formed type construction (§7.1.3) and participate directly in the arithmetic, logical, and conversion operators defined in Clause 8.

[2] Cursive provides six primitive categories: integer types (§7.2.2), floating-point types (§7.2.3), the boolean type (§7.2.4), the character type (§7.2.5), the unit type (§7.2.6), and the never type (§7.2.7). All primitive types satisfy the `Copy` predicate (§11.4) and impose no ownership obligations beyond those of their containing bindings.

[3] Primitive types integrate with other language components as follows:

- **Expressions** (§8.2–§8.6) evaluate primitive literals, arithmetic, and logical operators.
- **Generics** (Clause 10) permit primitive types as actual arguments and support specialization.
- **Memory model** (Clause 11) defines layout, alignment, and region interactions for primitive values.
- **Contracts and predicates** (Clause 12, §10.4) use primitive types in preconditions and grants without additional witness requirements.

#### §7.2.1 Syntax [type.primitive.syntax]

[4] Primitive types include integer types (`i8`, `i16`, `i32`, `i64`, `i128`, `isize`, `u8`, `u16`, `u32`, `u64`, `u128`, `usize`), floating-point types (`f32`, `f64`), `bool`, `char`, unit type `()`, and never type `!`.

[ Note: See Annex A §A.3 [grammar.type] for the normative `PrimitiveType` production. — end note ]

[5] These identifiers are reserved (§5.6). Attempting to redeclare or shadow them is ill-formed (diagnostic E06-001).

#### §7.2.2 Integer Types [type.primitive.int]

##### §7.2.2.1 Overview

[6] Integer types represent fixed-width, twos-complement (signed) or binary (unsigned) integers. They support exact arithmetic operations defined in §8.5.2 and participate in bitwise operations (§8.5.4), shifts (§8.5.5), and comparison operators (§8.5.6).

##### §7.2.2.2 Value Sets and Layout [type.primitive.int.values]

[7] Table 7.2.1 enumerates the value set and layout of each integer type.

**Table 7.2.1 — Integer type ranges and layout**

| Type    | Value set $\llbracket T \rrbracket$     | Minimum                    | Maximum                    | Size        | Alignment       |
| ------- | --------------------------------------- | -------------------------- | -------------------------- | ----------- | --------------- |
| `i8`    | $\mathbb{Z} \cap [-2^7, 2^7-1]$         | −128                       | 127                        | 1 byte      | 1 byte          |
| `i16`   | $\mathbb{Z} \cap [-2^{15}, 2^{15}-1]$   | −32,768                    | 32,767                     | 2 bytes     | 2 bytes         |
| `i32`   | $\mathbb{Z} \cap [-2^{31}, 2^{31}-1]$   | −2,147,483,648             | 2,147,483,647              | 4 bytes     | 4 bytes         |
| `i64`   | $\mathbb{Z} \cap [-2^{63}, 2^{63}-1]$   | −9,223,372,036,854,775,808 | 9,223,372,036,854,775,807  | 8 bytes     | 8 bytes         |
| `i128`  | $\mathbb{Z} \cap [-2^{127}, 2^{127}-1]$ | −$2^{127}$                 | $2^{127}-1$                | 16 bytes    | 8 or 16 bytes\* |
| `isize` | $\mathbb{Z} \cap [-2^{n-1}, 2^{n-1}-1]$ | platform                   | platform                   | $n/8$ bytes | $n/8$ bytes     |
| `u8`    | $\mathbb{N} \cap [0, 2^8-1]$            | 0                          | 255                        | 1 byte      | 1 byte          |
| `u16`   | $\mathbb{N} \cap [0, 2^{16}-1]$         | 0                          | 65,535                     | 2 bytes     | 2 bytes         |
| `u32`   | $\mathbb{N} \cap [0, 2^{32}-1]$         | 0                          | 4,294,967,295              | 4 bytes     | 4 bytes         |
| `u64`   | $\mathbb{N} \cap [0, 2^{64}-1]$         | 0                          | 18,446,744,073,709,551,615 | 8 bytes     | 8 bytes         |
| `u128`  | $\mathbb{N} \cap [0, 2^{128}-1]$        | 0                          | $2^{128}-1$                | 16 bytes    | 8 or 16 bytes\* |
| `usize` | $\mathbb{N} \cap [0, 2^{n}-1]$          | 0                          | platform                   | $n/8$ bytes | $n/8$ bytes     |

\*Implementations shall document whether 128-bit integers align to 8 or 16 bytes.

[8] The pointer-width-dependent parameter $n$ equals 32 on 32-bit targets and 64 on 64-bit targets. Implementations targeting additional widths shall publish their alignment choices and conformance evidence.

[9] Signed integers use two’s complement representation. The bit pattern for value $v$ is $v$ when $v \ge 0$ and $2^n + v$ otherwise. Unsigned integers use standard binary representation.

[10] Endianness is target-dependent. Implementations shall document the endianness used for integer storage.

##### §7.2.2.3 Syntax [type.primitive.int.syntax]

[11] Integer literals follow the lexical productions in §2.3, supporting decimal, hexadecimal (`0x`), octal (`0o`), and binary (`0b`) forms with optional `_` separators. Literal suffixes may be any integer type token (`123u16`, `0x10usize`).

##### §7.2.2.4 Constraints [type.primitive.int.constraints]

[12] Integer types are always well-formed:

$$
\dfrac{T \in \{i8,i16,i32,i64,i128,isize,u8,u16,u32,u64,u128,usize\}}{\Gamma \vdash T : \text{Type}}
\tag{WF-Int-Type}
$$

[13] An unsuffixed integer literal defaults to type `i32`. When a literal appears in a context requiring type $T$ and $\text{value}(n) \in \llbracket T \rrbracket$, the literal is typed as $T$ (contextual typing). Suffixes override both defaults and contextual expectations provided the value lies within $\llbracket T \rrbracket$.

##### §7.2.2.5 Semantics [type.primitive.int.semantics]

[14] Arithmetic operators `+`, `-`, `*`, `/`, `%` are defined on matching integer operand types (§8.5.2). Overflow behaviour is determined by operator form (wrapping, checked, saturating, or panicking). Bitwise operators (`&`, `|`, `^`, `~`, shifts) require the operands to be integer types (§8.5.4–§8.5.5).

[15] Integer-to-integer conversions follow these rules:

- Widening conversions (e.g., `u16` → `u32`, `i16` → `i32`) are implicit when required by the typing rules of §8, otherwise explicit via `as`.
- Narrowing conversions require `as` and shall diagnose potential data loss at compile time when the source expression is constant and out of range.
- Conversions between signed and unsigned integers are implementation-defined only with respect to wraparound; explicit casts define the bit interpretation as specified in §8.5.7.

##### §7.2.2.6 Diagnostic Requirements [type.primitive.int.diagnostics]

[16] Implementations shall emit at least the following diagnostics:

- **E02-206** — Invalid separator placement or literal out of range (§2.3.3).
- **E02-201** — Invalid escape sequence in numeric literal (§2.3.3).
- **E07-100** — Constant folding overflow (Clause 8 diagnostic catalog).
- **E07-101** — Division or remainder by zero detected statically (§8.5.2).

##### §7.2.2.7 Examples (Informative)

**Example 7.2.2.1 (Literal forms):**

```cursive
let decimal: const i32 = 1_000_000
let hex: const u32 = 0xFF00_FF00
let octal: const u16 = 0o755
let binary: const u8 = 0b1010_0101
let suffixed = 42usize
```

**Example 7.2.2.2 (Contextual typing and casts):**

```cursive
let timer: const u64 = 1_000          // literal widened from default i32
let pages: const u16 = 4096u16        // suffix ensures exact type
let bits = (timer as u128) << 8
```

---

#### §7.2.3 Floating-Point Types [type.primitive.float]

##### §7.2.3.1 Overview

[18] `f32` and `f64` implement IEEE 754-2019 binary floating-point arithmetic. They support finite numbers, subnormals, infinities, signed zeros, and NaNs. Operations follow IEEE rounding and exception semantics.

##### §7.2.3.2 Value Sets and Layout [type.primitive.float.values]

**Table 7.2.2 — Floating-point properties**

| Type  | IEEE 754 name | Size    | Alignment | Significand bits | Exponent range |
| ----- | ------------- | ------- | --------- | ---------------- | -------------- |
| `f32` | binary32      | 4 bytes | 4 bytes   | 24               | −126 to +127   |
| `f64` | binary64      | 8 bytes | 8 bytes   | 53               | −1022 to +1023 |

[19] Floating-point storage format matches the IEEE encoding. Implementations shall specify whether subnormals are flushed to zero; deviations render the implementation non-conforming.

##### §7.2.3.3 Syntax [type.primitive.float.syntax]

[20] Literals accept decimal and scientific notation with optional `_` separators (`0.5`, `3.14`, `6.022e23`, `1.0e-6`). Suffixes `f32` or `f64` select the type. Absence of suffix defaults to `f64`.

##### §7.2.3.4 Constraints [type.primitive.float.constraints]

$$
\dfrac{T \in \{f32, f64\}}{\Gamma \vdash T : \text{Type}}
\tag{WF-Float-Type}
$$

[21] Unsuffixed literals default to `f64`. Contextual typing may downcast to `f32` when the literal is representable (no rounding required beyond IEEE rules).

##### §7.2.3.5 Semantics [type.primitive.float.semantics]

[22] Arithmetic follows IEEE 754-2019 (§8.5.5). NaNs propagate through operations; `NaN != NaN`. Signed zeros compare equal yet produce sign-sensitive results for division and some transcendental functions. Comparison operators implement total ordering for `f64::total_cmp` style operations only when explicitly requested; ordinary comparisons follow IEEE partial ordering.

##### §7.2.3.6 Diagnostic Requirements [type.primitive.float.diagnostics]

[23] Diagnostics include:

- **E02-204** — Unterminated floating literal (§2.3, Annex E §E.5 diagnostics).
- **E02-212** — Literal out of range for chosen suffix (Annex E §E.5).
- **E07-102** — Invalid literal suffix (Annex E §E.5).
- **E07-103** — Constant folding overflow or division by zero at compile time (Annex E §E.5).

##### §7.2.3.7 Examples (Informative)

**Example 7.2.3.1 (Special values):**

```cursive
let pos_inf: const f64 = 1.0 / 0.0
let neg_inf: const f64 = -1.0 / 0.0
let nan_div: const f64 = 0.0 / 0.0
let signed_zero = (1.0 / pos_inf, 1.0 / neg_inf)  // (+0.0, -0.0)
```

**Example 7.2.3.2 (Precision considerations):**

```cursive
let delta64 = (0.1 + 0.2) - 0.3          // ≈ 5.551e-17
let delta32 = (0.1f32 + 0.2f32) - 0.3f32 // ≈ 5.960e-08
```

---

#### §7.2.4 Boolean Type [type.primitive.bool]

##### §7.2.4.1 Overview

[24] `bool` encodes two-valued logic. It serves as the condition type for control-flow constructs (§9.2) and supports logical operators (`&&`, `||`, `!`, `^`).

##### §7.2.4.2 Syntax and Constraints

$$
\dfrac{}{\Gamma \vdash bool : \text{Type}}
\tag{WF-Bool-Type}
$$

$$
\dfrac{}{\Gamma \vdash true : bool}
\tag{T-Bool-True}
$$

$$
\dfrac{}{\Gamma \vdash false : bool}
\tag{T-Bool-False}
$$

[25] `bool` literal tokens are `true` and `false`. No other tokens implicitly convert to `bool`; conversions shall use explicit tests.

##### §7.2.4.3 Semantics

[26] Logical operators short-circuit: `e₁ && e₂` skips `e₂` when `e₁` is `false`; `e₁ || e₂` skips `e₂` when `e₁` is `true`. `!` flips the truth value. Bitwise `^` corresponds to exclusive-or.

##### §7.2.4.4 Diagnostics

[27] **E07-104** — Non-`bool` operand supplied to logical operator (Clause 8 diagnostic). **E07-105** — `bool` used where numeric type required.

##### §7.2.4.5 Examples (Informative)

**Example 7.2.4.1 (Boolean operations):**

```cursive
let feature_enabled = config::flag("experimental")
let ready = feature_enabled && service().probe()
```

---

#### §7.2.5 Character Type [type.primitive.char]

##### §7.2.5.1 Overview

[28] `char` represents Unicode scalar values (code points excluding U+D800–U+DFFF). It stores a value as a 32-bit unsigned integer and interacts with string conversions (§7.3.4) and pattern matching (§8.4.6).

##### §7.2.5.2 Syntax and Constraints

$$
\dfrac{}{\Gamma \vdash char : \text{Type}}
\tag{WF-Char-Type}
$$

$$
\dfrac{c \in \text{ScalarValues}}{\Gamma \vdash c : char}
\tag{T-Char-Lit}
$$

[29] Character literals use single quotes and may include escape sequences listed in Table 7.2.3.

**Table 7.2.3 — Character literal escapes**

| Escape     | Meaning                         | Unicode |
| ---------- | ------------------------------- | ------- |
| `\n`       | Line feed                       | U+000A  |
| `\r`       | Carriage return                 | U+000D  |
| `\t`       | Horizontal tab                  | U+0009  |
| `\\`       | Backslash                       | U+005C  |
| `\'`       | Single quote                    | U+0027  |
| `\"`       | Double quote                    | U+0022  |
| `\0`       | Null                            | U+0000  |
| `\u{HHHH}` | Unicode scalar (1–6 hex digits) | U+HHHH  |

[30] Surrogates trigger diagnostic E07-106. Literals shall denote exactly one scalar value.

##### §7.2.5.3 Semantics

[31] Ordering compares code points numerically. `char` supports conversion to UTF-8/UTF-16 sequences via standard library routines (informative reference: Annex F §F.2).

##### §7.2.5.4 Examples (Informative)

**Example 7.2.5.1 (Character literals):**

```cursive
let letter: const char = 'A'
let emoji: const char = '\u{1F4A1}'
```

---

#### §7.2.6 Unit Type [type.primitive.unit]

##### §7.2.6.1 Overview

[32] The unit type `()` has a single inhabitant `()`. It represents absence of meaningful value, serves as the return type of procedures that only produce effects, and corresponds to the zero-element tuple.

##### §7.2.6.2 Constraints and Semantics

$$
\dfrac{}{\Gamma \vdash () : \text{Type}}
\tag{WF-Unit-Type}
$$

$$
\dfrac{}{\Gamma \vdash () : ()}
\tag{T-Unit-Lit}
$$

[33] `sizeof(()) = 0`, `alignof(()) = 1`. No storage is allocated for unit values; they may be freely elided.

##### §7.2.6.3 Examples (Informative)

**Example 7.2.6.1 (Unit return type):**

```cursive
procedure log_event(event: string)
    [[ io::write |- true => () ]]
{
    println("event: {}", event)
}
```

---

#### §7.2.7 Never Type [type.primitive.never]

##### §7.2.7.1 Overview

[34] The never type `!` is uninhabited. Expressions of type `!` represent computations that never produce a value (diverge, panic, or exit). `!` acts as the bottom of the subtype lattice; any value of type `!` can coerce to any type.

##### §7.2.7.2 Constraints and Semantics

$$
\dfrac{}{\Gamma \vdash ! : \text{Type}}
\tag{WF-Never-Type}
$$

$$
\dfrac{\Gamma \vdash e : ! \quad \Gamma \vdash \tau : \text{Type}}{\Gamma \vdash e : \tau}
\tag{Coerce-Never}
$$

[35] Since `!` has no values, `sizeof(!) = 0` and `alignof(!) = 1`. Moves and drops never occur.

##### §7.2.7.3 Examples (Informative)

```cursive
procedure fatal(error: string): !
    [[ diag::panic |- true => false ]]
{
    panic(error)
}

procedure compute_or_exit(code: i32): i32
    [[ process::exit |- true => true ]]
{
    if code >= 0 {
        result code
    } else {
        std::process::exit(code)  // type ! coerces to i32
    }
}
```

---

#### §7.2.8 Summary of Copy Behaviors [type.primitive.copy]

[ Note: Formal proofs that primitive types satisfy Copy are provided in Annex C §C.4 [formal.permission]. — end note ]

| Type           | Copy behavior               |
| -------------- | ---------------------------- |
| `IntegerTypes` | `T : Copy`                   |
| `f32`, `f64`   | `T : Copy`                   |
| `bool`         | `Copy`                      |
| `char`         | `Copy`                      |
| `()`           | `Copy`                      |
| `!`            | Vacuously `Copy`             |

All primitive types are therefore movable without consuming permissions.

#### §7.2.9 Conformance Requirements [type.primitive.requirements]

[36] A conforming implementation shall:

- Support every primitive type enumerated in paragraph [2] with the sizes, alignments, and value sets in §§7.2.2–7.2.7.
- Honour two-phase typing and inference: primitive literals shall type-check using the rules in this subclause without requiring speculative rewriting.
- Emit diagnostics listed in the corresponding subsections when literals or operations violate constraints.
- Expose primitive type metadata (size, alignment, name) through introspection facilities (§7.7.2) consistent with this clause.
- Prevent user programs from redeclaring primitive identifiers or altering their semantics.

[37] Deviations render the implementation non-conforming unless explicitly permitted as implementation-defined behaviour elsewhere in the specification.

---

**Previous**: §7.1 Type System Overview (§7.1 [type.overview]) | **Next**: §7.3 Composite Types (§7.3 [type.composite])
