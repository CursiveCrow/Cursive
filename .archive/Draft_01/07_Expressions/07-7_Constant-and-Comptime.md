# Cursive Language Specification

## Clause 7 — Expressions

**Clause**: 7 — Expressions  
**File**: 08-7_Constant-and-Comptime.md  
**Section**: §7.7 Constant Expressions and Comptime Contexts  
**Stable label**: [expr.constant]  
**Forward references**: Clause 2 §2.2 [lex.phases], Clause 5 §5.2 [decl.variable], Clause 16 [comptime], Clause 10 [generic]

---

### §7.7 Constant Expressions and Comptime Contexts [expr.constant]

#### §7.7.1 Overview

[1] Constant expressions are those evaluable during the translation phases (§2.2). They appear in `const` bindings, array lengths, enum discriminants, attribute arguments, and `comptime` blocks. This subclause defines the admissible constructs inside const contexts, the grants permitted during compile-time execution, and the diagnostics for violations.

#### §7.7.2 Const contexts

[2] An expression is required to be constant when:

- It belongs to a `const`/`static` binding (§5.2).
- It appears in a position that influences type formation (array length `[T; n]`, repeat literal `[value; n]`, const generic argument).
- It occurs inside `comptime { … }` (§8.7.3) or within a `comptime procedure` body (Clause 10).
- It supplies default values for const generic parameters (Clause 11).

[3] Const expressions may contain:

- Literals and literal operators.
- References to previously defined constants.
- Calls to `comptime procedure`s whose grants are comptime-safe.
- Non-capturing closures whose bodies obey these restrictions.
- Type introspection intrinsics (`type_name`, `type_id`, `type_info`; Clause 7.8) provided their results remain compile-time data.

[4] Disallowed constructs include: runtime IO, heap allocation, raw pointer arithmetic on runtime pointers, accesses to mutable statics, and any call that requires a runtime-only grant. Using such constructs inside const contexts emits E08-700.

#### §7.7.3 Comptime blocks

[5] Grammar: `ComptimeExpr ::= 'comptime' Block` (Annex A §A.4.18).

[6] Semantics:

- The block executes during the compile-time execution phase (§2.2). Its statements follow the same rules as runtime blocks but must obey the const restrictions above.
- The block introduces a new lexical scope; bindings declared inside it do not leak to runtime.
- The `result` expression becomes a literal embedded into the compiled program. If the block omits `result`, its type is `()` and the expression is useless—implementations may warn.

[7] Grants: comptime blocks may only request grants whose names begin with `comptime.` (e.g., `comptime.alloc`, `comptime.codegen`). Attempting to invoke a runtime grant (e.g., `io.write`) emits E08-701 identifying the offending grant.

[8] Error reporting: any panic or error encountered during comptime evaluation is reported at compile time with the original source span plus the enclosing `comptime` block span for context.

#### §7.7.4 Interaction with generics

[9] Const generics (Clause 11) substitute concrete values before const evaluation. Expressions referencing const parameters are therefore treated identically to literal constants once monomorphisation occurs. Type-level intrinsics (e.g., `type_name<T>()`) may appear inside comptime contexts as long as their results are assigned to const bindings or used as compile-time metadata.

#### §7.7.5 Diagnostics

| Code | Condition |
| --- | --- |
| E08-700 | Runtime-only construct used in const/comptime context |
| E08-701 | Comptime block requested non-comptime grant |

#### §7.7.6 Canonical example

````cursive
const POWERS: [u32; 8] = comptime {
    let mut table = [0; 8]
    var i = 0
    loop i < table.len() {
        table[i] = (1u32 << i)
        i += 1
    }
    result table
}
````

[10] The block executes at compile time, constructing an array literal without using any runtime-only grants.

---
