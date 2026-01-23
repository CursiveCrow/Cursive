# Cursive Language Specification

## Clause 10 — Memory Model, Regions, and Permissions

**Part**: X — Memory Model, Regions, and Permissions
**File**: 11-8_Unsafe-Blocks.md  
**Section**: §10.8 Unsafe Blocks and Safety Obligations  
**Stable label**: [memory.unsafe]  
**Forward references**: Clause 7 §7.5 [type.pointer], Clause 12 [contract], Clause 15 [interop]

---

### §10.8 Unsafe Blocks and Safety Obligations [memory.unsafe]

#### §10.8.1 Overview

[1] Unsafe blocks explicitly mark code where the programmer assumes responsibility for memory safety. They enable operations that the compiler cannot verify safe: raw pointer dereference, pointer arithmetic, transmute, and FFI calls.

[2] Safety obligations in unsafe code are programmer responsibilities. The compiler does not enforce memory safety within `unsafe` blocks but still requires syntactic and type correctness.

[3] Unsafe code is necessary for FFI (Clause 15), performance-critical operations, and implementing safe abstractions over unsafe primitives.

#### §10.8.2 Unsafe Block Syntax [memory.unsafe.syntax]

##### §10.8.2.1 Block Form

[4] Unsafe block syntax:

**Unsafe blocks** match the pattern:
```
"unsafe" <block_stmt>
```

[ Note: See Annex A §A.5 [grammar.statement] for complete unsafe syntax.
— end note ]

[5] The `unsafe` keyword marks a block where memory safety is programmer-verified rather than compiler-verified.

**Example 11.8.2.1 (Unsafe block):**

```cursive
procedure raw_write(ptr: *mut i32, value: i32)
    [[ unsafe::ptr |- true => true ]]
{
    unsafe {
        *ptr = value    // Raw pointer write
    }
}
```

#### §10.8.3 Unsafe Operations [memory.unsafe.operations]

##### §10.8.3.1 Operations Requiring Unsafe

[6] The following operations are permitted only within `unsafe` blocks:

(6.1) **Raw pointer dereference**: `*ptr` where `ptr: *const T` or `*mut T`

(6.2) **Raw pointer write**: `*ptr = value` where `ptr: *mut T`

(6.3) **Pointer arithmetic**: `ptr.offset(n)`, `ptr.add(n)`, `ptr.sub(n)`

(6.4) **Transmute**: `transmute::<From, To>(value)` (bitwise reinterpretation)

(6.5) **Calling unsafe functions**: Functions marked `unsafe` in their signature

(6.6) **Inline assembly**: Platform-specific assembly blocks (Clause 15)

[7] Attempting these operations outside `unsafe` blocks produces diagnostics (E11-801 series).

##### §10.8.3.2 Raw Pointers

[8] Raw pointer types `*const T` and `*mut T` (§7.5.5) bypass safety guarantees:

- No lifetime tracking
- No provenance verification
- No null checking
- No alignment verification
- Dereference requires `unsafe` block

**Example 11.8.3.1 (Raw pointer usage):**

```cursive
procedure unsafe_read(ptr: *const i32): i32
    [[ unsafe::ptr |- true => true ]]
{
    unsafe {
        result *ptr  // Programmer ensures: ptr valid, aligned, initialized
    }
}
```

##### §10.8.3.3 Transmute

[9] Transmute reinterprets bit patterns between types:

```cursive
unsafe {
    let bits: u32 = 0x4048_0000
    let float: f32 = transmute::<u32, f32>(bits)  // Reinterpret as f32
}
```

[10] **Safety obligations**:

- Source and target types must have same size
- Bit pattern must be valid for target type
- Alignment must be compatible

[11] Violating transmute safety produces undefined behavior [UB-ID: B.2.25].

#### §10.8.4 Safety Obligations [memory.unsafe.obligations]

##### §10.8.4.1 Programmer Responsibilities

[12] Within `unsafe` blocks, programmers must manually ensure:

(12.1) **Pointer validity**: Dereferenced pointers are valid, aligned, and initialized

(12.2) **Lifetime correctness**: Accessed memory has not been freed

(12.3) **Type safety**: Transmute target types match bit patterns

(12.4) **Alignment**: Pointer addresses satisfy type alignment requirements

(12.5) **Initialization**: Read values are properly initialized

[13] Violating these obligations produces undefined behavior. The compiler provides no safety net.

##### §10.8.4.2 Unsafe Function Declaration

[14] Functions containing unsafe operations shall declare required grants:

**Example 11.8.4.1 (Unsafe function signature):**

```cursive
procedure unsafe_operation(ptr: *mut Data)
    [[ unsafe::ptr |- true => true ]]  // Declares unsafe grant requirement
{
    unsafe {
        (*ptr).field = compute_value()
    }
}
```

[15] The `unsafe::ptr` grant documents that the function performs unsafe pointer operations.

#### §10.8.5 Unsafe and FFI [memory.unsafe.ffi]

##### §10.8.5.1 FFI Integration

[16] Foreign function calls (Clause 15) implicitly require unsafe:

```cursive
extern "C" procedure c_function(ptr: *const i32): i32

procedure call_c()
    [[ ffi::call |- true => true ]]
{
    let value: i32 = 42
    unsafe {
        let result = c_function(&value as *const i32)
    }
}
```

[17] Complete FFI safety obligations are specified in Clause 15 §15.1 [interop.ffi] and §15.2 [interop.unsafe].

#### §10.8.6 Diagnostics

[18] Unsafe diagnostics:

| Code    | Condition                                |
| ------- | ---------------------------------------- |
| E11-801 | Raw pointer dereference outside `unsafe` |
| E11-802 | Raw pointer write outside `unsafe`       |
| E11-803 | Transmute outside `unsafe`               |
| E11-804 | Transmute size mismatch                  |
| E11-805 | Inline assembly outside `unsafe`         |

#### §10.8.7 Conformance Requirements

[19] Implementations shall:

1. Support `unsafe { }` blocks marking unchecked operations
2. Require `unsafe` blocks for raw pointer operations
3. Require `unsafe` blocks for transmute operations
4. Enforce grant requirements (unsafe::ptr, ffi::call) in contractual sequents
5. Document undefined behavior triggered by unsafe violations
6. Provide diagnostic guidance for common unsafe pitfalls
7. Integrate unsafe with FFI (Clause 15)
8. Maintain type checking within unsafe blocks (syntax must still be valid)

---

**Previous**: §11.7 Aliasing and Uniqueness (§11.7 [memory.aliasing]) | **Next**: Clause 12 — Contracts (§12 [contract])
