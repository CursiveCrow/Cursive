# Cursive Language Specification

## Clause 11 — Contracts

**Clause**: 11 — Contracts
**File**: 12-8_Verification-Modes.md
**Section**: §11.8 Verification Modes and Dynamic Hooks to Witnesses
**Stable label**: [contract.verification]  
**Forward references**: §12.4 [contract.precondition], §12.5 [contract.postcondition], §12.7 [contract.checking], Clause 13 [witness], Annex E §E.2 [implementation.algorithms]

---

### §11.8 Verification Modes and Dynamic Hooks to Witnesses [contract.verification]

#### §11.8.1 Overview

[1] _Verification modes_ control when and how contractual sequents are checked. Cursive supports three explicit modes (`static`, `dynamic`, `trusted`) plus a build-mode-dependent default. Modes determine whether contracts are proven at compile time, checked at runtime, or trusted without verification.

[2] Verification modes balance safety, performance, and formal verification requirements. They are specified via attributes and do not affect procedure type signatures or sequent semantics—two procedures with identical sequents but different verification modes have the same type.

[3] This subclause specifies verification mode syntax, semantics, checking strategies, and integration with the witness system (Clause 12) for dynamic verification hooks.

#### §11.8.2 Verification Mode Specification [contract.verification.modes]

##### §11.8.2.1 Attribute Syntax

[4] Verification modes are specified using the `verify` attribute on procedure declarations:

```ebnf
verification_attribute
    ::= "[[" "verify" "(" verification_mode ")" "]]"

verification_mode
    ::= "static"
     | "dynamic"
     | "trusted"
```

[5] The attribute appears before the procedure declaration:

```cursive
[[verify(static)]]
procedure critical_operation(x: i32): i32
    [[ |- x > 0 => result > x ]]
{ }
```

##### §11.8.2.2 Mode Semantics

**static mode**:
[6] Contracts must be proven at compile time. If static verification fails, compilation fails with diagnostic E11-056. No runtime checks are inserted.

**dynamic mode**:
[7] Runtime checks are always inserted for preconditions and postconditions, regardless of whether they can be proven statically. This provides maximum safety at runtime cost.

**trusted mode**:
[8] Contracts are trusted without verification. No static proof is attempted and no runtime checks are inserted. The programmer assumes responsibility for contract satisfaction. This mode is for performance-critical code with externally verified contracts.

**default mode (no attribute)**:
[9] Verification strategy is build-mode dependent:

- **Debug builds** (`--build=debug` or `NDEBUG` undefined): Insert runtime checks for unprovable contracts
- **Release builds** (`--build=release` or `NDEBUG` defined): Only proven checks execute; unprovable contracts are skipped
- **Unsafe mode** (`--verify=none` flag): Skip all contract checking

#### §11.8.3 Verification Mode Selection [contract.verification.selection]

[10] The effective verification mode for a procedure is determined by:

```
select_verification_mode(procedure):
    if procedure has [[verify(mode)]] attribute:
        return mode  // Explicit override

    // Build-mode default
    if compiler_flag(--verify=none):
        return Trusted
    else if compiler_flag(--build=debug) or !defined(NDEBUG):
        return Dynamic  // Debug: insert checks
    else if compiler_flag(--build=release) or defined(NDEBUG):
        return ProvenOnly  // Release: proven checks only
    else:
        return Dynamic  // Conservative default
```

[11] Explicit mode attributes override build-mode defaults, enabling fine-grained control when needed.

#### §11.8.4 Static Verification Strategy [contract.verification.static]

[12] In `static` verification mode, the compiler must prove contracts or reject compilation. The verification strategy employs:

**Symbolic execution**:
[13] The compiler symbolically executes procedure bodies, propagating constraints through assignments, conditionals, and calls. Symbolic states track variable values and path conditions.

**SMT solver integration** (optional):
[14] Implementations may invoke SMT (Satisfiability Modulo Theories) solvers to discharge proof obligations. SMT integration is optional; implementations without solvers may reject more programs conservatively.

**Path-sensitive analysis**:
[15] The compiler analyzes each control-flow path independently, proving that postconditions hold on all paths and preconditions are satisfied at all call sites.

**Example 12.8.4.1 (Static verification)**:

```cursive
[[verify(static)]]
procedure factorial(n: i32): i32
    [[ |- n >= 0, n <= 12 => result >= 1 ]]
{
    if n <= 1 {
        result 1  // Path 1: result = 1 ≥ 1 ✅
    } else {
        result n * factorial(n - 1)
        // Path 2: Requires proof by induction
        // If provable: ✅
        // If not provable: ❌ E11-056
    }
}
```

#### §11.8.5 Dynamic Verification Strategy [contract.verification.dynamic]

[16] In `dynamic` verification mode, the compiler inserts runtime assertions for all contracts:

**Precondition checks**:
[17] Inserted immediately before procedure call:

```cursive
// Source
procedure divide(a: i32, b: i32): i32
    [[ |- b != 0 => true ]]
{ result a / b }

let result = divide(x, y)

// Compiled (dynamic mode)
if !(y != 0) {
    panic("precondition violated: b != 0")
}
let result = divide(x, y)
```

**Postcondition checks**:
[18] Inserted at all return points:

```cursive
// Source
procedure abs(x: i32): i32
    [[ |- true => result >= 0 ]]
{
    if x < 0 { result -x } else { result x }
}

// Compiled (dynamic mode)
procedure abs(x: i32): i32 {
    let __result = if x < 0 { -x } else { x }
    if !(__result >= 0) {
        panic("postcondition violated: result >= 0")
    }
    __result
}
```

#### §11.8.6 Trusted Mode [contract.verification.trusted]

[19] In `trusted` mode, no verification occurs. The programmer certifies that contracts are satisfied:

```cursive
[[verify(trusted)]]
procedure hot_path(data: Buffer): i32
    [[ |- data.len() > 0 => result >= 0 ]]
{
    // No checks inserted
    // Programmer responsible for ensuring contract
    result unsafe { data.unchecked_get(0) }
}
```

[20] Trusted mode is appropriate for:

- Performance-critical inner loops with externally verified contracts
- Procedures with contracts proven by external tools
- FFI boundaries where contracts are enforced by foreign code
- Micro-optimizations after extensive testing

[ Warning: Violating contracts in trusted mode may produce undefined behavior. Use trusted mode only when contract satisfaction can be assured through other means.
— end warning ]

#### §11.8.7 Build-Mode Defaults [contract.verification.buildmode]

[21] Without explicit `[[verify(...)]]` attributes, verification follows build-mode conventions:

**Table 12.3 — Build-mode verification defaults**

| Build Mode   | Compiler Flag     | Static Proof Attempted | Unprovable Contracts | Proven Contracts   |
| ------------ | ----------------- | ---------------------- | -------------------- | ------------------ |
| Debug        | `--build=debug`   | Yes                    | Insert checks        | No checks (proven) |
| Release      | `--build=release` | Yes                    | Skip checks          | No checks (proven) |
| Unsafe       | `--verify=none`   | No                     | Skip checks          | Skip checks        |
| Verification | `--verify=static` | Yes (required)         | Error E11-056        | No checks (proven) |

[22] The default mode when no flags are provided is Debug (conservative, safe).

#### §11.8.8 Verification Mode and Type Signatures [contract.verification.types]

[23] Verification modes are metadata and do not affect procedure types. Two procedures with identical sequents but different verification modes have identical types and are interchangeable for type checking purposes.

**Example 12.8.8.1 (Verification mode does not affect types)**:

```cursive
[[verify(static)]]
procedure strict_add(a: i32, b: i32): i32
    [[ |- a > 0, b > 0 => result > a ]]
{ result a + b }

[[verify(dynamic)]]
procedure checked_add(a: i32, b: i32): i32
    [[ |- a > 0, b > 0 => result > a ]]
{ result a + b }

// Both have type: (i32, i32) -> i32 ! ∅ [[ |- a > 0, b > 0 => result > a ]]
// Type equivalence: strict_add ≡ checked_add
```

#### §11.8.9 Dynamic Hooks to Witness System [contract.verification.witness]

[24] When dynamic verification is enabled, the compiler may generate _witness hooks_: runtime evidence that contracts are satisfied. Witnesses integrate with the witness system (Clause 12) for dynamic polymorphism.

[25] Witness hooks capture:

- Successful precondition validation (proves caller met obligations)
- Successful postcondition validation (proves callee met guarantees)
- Contract satisfaction evidence for polymorphic dispatch

[26] The witness system specification (Clause 12) defines how runtime evidence is represented, stored, and used for dynamic dispatch. The contract system provides the verification points where witnesses are generated. Complete witness integration semantics are specified in Clause 12 [witness].

#### §11.8.10 Constraints [contract.verification.constraints]

[1] _Attribute placement._ The `[[verify(...)]]` attribute shall appear directly before the procedure declaration it modifies. Placing it elsewhere produces diagnostic E11-080.

[2] _Mode validity._ The verification mode shall be one of: `static`, `dynamic`, `trusted`. Invalid modes produce diagnostic E11-081.

[3] _Static verification requirement._ When mode is `static`, all contracts in the procedure must be provable at compile time. Unprovable contracts in static mode produce diagnostic E11-056.

[4] _Trusted safety obligation._ When mode is `trusted`, the programmer assumes responsibility for contract satisfaction. Violating contracts in trusted mode may produce undefined behavior if the violation affects memory safety.

[5] _Build flag validity._ The compiler flags `--build=debug`, `--build=release`, and `--verify=none` are implementation-defined but shall follow the semantics in Table 11.3.

#### §11.8.11 Examples [contract.verification.examples]

**Example 12.8.11.1 (All verification modes)**:

```cursive
// Static mode: Must prove or fail compilation
[[verify(static)]]
procedure proven_square(x: i32): i32
    [[ |- x >= 0, x <= 46340 => result >= 0 ]]
{
    result x * x  // Provable: non-negative * non-negative ≥ 0
}

// Dynamic mode: Always insert checks
[[verify(dynamic)]]
procedure runtime_checked(value: i32): i32
    [[ |- value > 0 => result > value ]]
{
    result value + 1  // Check inserted even if provable
}

// Trusted mode: No verification
[[verify(trusted)]]
procedure performance_critical(data: [i32]): i32
    [[ |- data.len() > 0 => result >= 0 ]]
{
    // Programmer ensures contract; no checks inserted
    result data[0]
}

// Default mode: Build-dependent
procedure normal_procedure(x: i32): i32
    [[ |- x > 0 => result > x ]]
{
    result x + 1
    // Debug build: Insert check
    // Release build: Proven statically, no check
}
```

**Example 12.8.11.2 (Build-mode behavior)**:

```cursive
procedure compute(input: i32): i32
    [[ |- input != 0 => result != 0 ]]
{
    result input * 2
}

// Debug build (--build=debug):
// Inserts: if !(input != 0) { panic!("precondition violated") }
// Inserts: if !(result != 0) { panic!("postcondition violated") }

// Release build (--build=release):
// Static proof: input != 0 ⇒ input * 2 != 0 ✅
// No checks inserted (proven)

// Unsafe mode (--verify=none):
// No checks inserted (all contracts trusted)
```

#### §11.8.12 Diagnostics [contract.verification.diagnostics]

[6] Verification mode diagnostics:

| Code    | Condition                                              | Constraint |
| ------- | ------------------------------------------------------ | ---------- |
| E11-056 | Static verification failed (mode=static)               | [3]        |
| E11-080 | verify attribute misplaced (not before procedure)      | [1]        |
| E11-081 | Invalid verification mode (not static/dynamic/trusted) | [2]        |

#### §11.8.13 Conformance Requirements [contract.verification.requirements]

[7] Implementations shall:

1. Support three explicit verification modes: static, dynamic, trusted
2. Provide build-mode-dependent defaults per Table 11.3
3. Parse and apply `[[verify(mode)]]` attributes on procedures
4. Enforce static verification requirements: prove or reject in static mode
5. Insert runtime checks in dynamic mode for all contracts
6. Skip verification in trusted mode
7. Apply build-mode defaults when no explicit mode is specified
8. Ensure verification modes do not affect type signatures
9. Generate witness hooks when dynamic verification is enabled
10. Emit diagnostics E11-056, E11-080, E11-081 for mode violations
11. Document build flags and their verification behavior

---

**Previous**: §11.7 Sequent Checking Flow (§11.7 [contract.checking]) | **Next**: §11.9 Grammar Reference (§11.9 [contract.grammar])
