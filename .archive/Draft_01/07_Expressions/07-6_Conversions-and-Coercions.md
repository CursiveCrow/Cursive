# Cursive Language Specification

## Clause 7 — Expressions

**Clause**: 7 — Expressions  
**File**: 08-6_Conversions-and-Coercions.md  
**Section**: §7.6 Conversions and Coercions  
**Stable label**: [expr.conversion]  
**Forward references**: §8.7 [expr.constant], Clause 6 §6.2 [type.primitive], Clause 7 §7.5 [type.pointer], Clause 7 §7.6 [type.modal], Clause 10 [generic]

---

### §7.6 Conversions and Coercions [expr.conversion]

#### §7.6.1 Overview

[1] Conversions translate values between compatible types. Cursive differentiates **explicit casts** (`expr as Type`) from the small set of **implicit coercions** performed by the type checker. Annex A §A.4.17 provides the grammar for cast expressions.

#### §7.6.2 Explicit casts (`as`)

[2] Cast expressions use the `as` keyword: `unary_expr as Type`.

[ Note: See Annex A §A.4 [grammar.expression] for the normative `CastExpr` production. — end note ]

[3] Legal cast categories:

1. **Numeric-to-numeric.** Conversions between primitive numeric types (Clause 7 §7.2). Narrowing conversions may truncate; constant expressions that cannot fit emit E07-600.
2. **Pointer casts.** Safe pointers `Ptr<T>@State` may cast to `Ptr<U>@State` when the pointee layouts are proven equivalent (Clause 12). Casting to the unconstrained pointer `Ptr<T>` is always legal (widening). Raw pointer casts (`*const T` ↔ `*mut U`) require grant `unsafe.ptr` and are unchecked at runtime.
3. **Modal widening.** `ModalType@State as ModalType` discards state information. Narrowing (adding a state) is illegal without a witness from a transition procedure and emits E08-601.
4. **Enum discriminants.** Casting an enum value to `usize` yields its discriminant. Casting integers back to enums is forbidden to avoid bypassing exhaustiveness.
5. **Opaque handles.** Implementation-defined opaque types may specify additional cast hooks (e.g., ABI handles). Such casts shall be documented in Clause 16 and require explicit `as`.

[4] Any cast outside these categories is rejected with E08-601. Implementations may provide future extensions only through the specification process; ad-hoc casts are non-conforming.

[5] Evaluation: the operand evaluates first. Numeric casts follow two’s-complement or IEEE rounding rules. Pointer casts are representational; they do not alter permissions or provenance. Enum-to-integer casts read the discriminant without affecting the payload.

#### §7.6.3 Implicit coercions

[6] The type checker performs only the following coercions automatically:

- **Unit insertion:** expressions lacking `result` implicitly yield `()` to match contexts expecting unit.
- **Pointer/modal widening:** `Ptr<T>@State <: Ptr<T>` and `Modal@State <: Modal` (Clause 7 §7.7). This is the only implicit pointer conversion.
- **String state defaulting:** bare `string` resolves to `string@View`; `string@Managed` may coerce to `string@View` when required.
- **Pipeline annotations:** when a pipeline stage omits its annotation under §8.1.3[9], the checker inserts an equality coercion `τ → τ` after proving equivalence.

[7] No numeric promotions occur implicitly; developers must cast explicitly when combining different numeric types. Likewise, there is no implicit conversion between `bool` and integers.

#### §7.6.4 Diagnostics

| Code    | Condition                            |
| ------- | ------------------------------------ |
| E07-600 | Constant cast loses information      |
| E08-601 | Cast not covered by a legal category |

#### §7.6.5 Canonical example

```cursive
let magnitude: f64 = (vector.x as f64).hypot(vector.y as f64)
let bucket: u8 = ((hash as u32) % 256u32) as u8
let ptr: Ptr<u8> = some_slice.as_ptr() as Ptr<u8>
```

[8] The example showcases explicit numeric casts (with narrowing), as well as safe-pointer widening via `as` to document intent.

---
