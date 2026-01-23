# Cursive Language Specification

## Clause 14 — Interoperability and ABI

**Part**: XIV — Interoperability and ABI  
**File**: 15-6_ABI-Specification.md  
**Section**: §14.6 ABI Specification  
**Stable label**: [interop.abi]  
**Forward references**: §15.3 [interop.c], §15.5 [interop.linkage], Clause 7 §7.2 [type.primitive], Clause 11 §11.6 [memory.layout], §1.2.3 [intro.refs.abi]

---

### §14.6 ABI Specification [interop.abi]

#### §14.6.1 Overview

[1] The Application Binary Interface (ABI) defines the low-level interface between compiled code modules: calling conventions, register usage, stack layout, data representation, and symbol naming. ABI compliance enables linking Cursive code with foreign code and using system libraries.

[2] Cursive's ABI is platform-specific and implementation-defined. Implementations shall document their ABI choices and ensure consistency with platform conventions (System V ABI, Windows x64 calling convention, ARM AAPCS, etc.).

[3] This subclause specifies ABI requirements for calling conventions, data layout, register allocation, and platform-specific details.

#### §14.6.2 Calling Conventions

##### §14.6.2.1 Overview

[4] _Calling conventions_ define how procedures receive parameters, return results, and manage the call stack. Cursive supports multiple calling conventions via explicit `[[extern(...)]]` attributes (Decision D16).

##### §14.6.2.2 Supported Calling Conventions

[5] Implementations shall support at least the following calling conventions:

**"C" — C calling convention** (mandatory):
[6] The standard C calling convention for the target platform:

- **Linux/Unix**: System V AMD64 ABI (x86-64), ARM AAPCS (ARM64)
- **Windows**: Microsoft x64 calling convention (x86-64), ARM64 calling convention (ARM64)
- **32-bit platforms**: Platform-specific C ABI (cdecl, etc.)

[7] The "C" convention is mandatory for all implementations. It enables calling standard C libraries and system APIs.

**"stdcall"** (Windows platforms):
[8] Windows standard call convention (callee cleans stack on 32-bit, identical to C on 64-bit):

```cursive
[[extern(stdcall)]]
procedure WindowsAPI(param: i32): i32
    [[ ffi::call |- true => true ]]
```

[9] On 64-bit Windows, stdcall is equivalent to the C calling convention. On 32-bit Windows, stdcall differs in stack cleanup responsibility.

**"system"** (platform default):
[10] Use the platform's system calling convention:

- Unix/Linux: Same as "C"
- Windows: Same as "stdcall" (32-bit) or "C" (64-bit)

[11] The "system" convention allows platform-agnostic code when the specific convention doesn't matter.

**Platform-specific conventions** (optional):
[12] Implementations may support additional conventions:

- `"fastcall"`: Fast calling convention (registers for first args)
- `"vectorcall"`: SIMD-optimized convention (Windows)
- `"aapcs"`: ARM Architecture Procedure Call Standard
- `"sysv64"`: Explicit System V AMD64

[13] Platform-specific conventions are implementation-defined. Using unsupported conventions produces diagnostic E15-080.

##### §14.6.2.3 Parameter Passing

[14] Calling conventions specify how parameters are passed:

**System V AMD64 (Linux x86-64)**:

- First 6 integer/pointer args: RDI, RSI, RDX, RCX, R8, R9
- First 8 floating args: XMM0-XMM7
- Remaining args: Stack (right-to-left)

**Microsoft x64 (Windows x86-64)**:

- First 4 integer/pointer args: RCX, RDX, R8, R9
- First 4 floating args: XMM0-XMM3
- Stack shadow space: 32 bytes
- Remaining args: Stack

**ARM64 (AAPCS)**:

- First 8 integer/pointer args: X0-X7
- First 8 floating args: V0-V7
- Remaining args: Stack

[15] Implementations shall follow platform ABI specifications exactly. Deviations produce link errors or undefined behavior.

##### §14.6.2.4 Return Values

[16] Return value conventions:

- **Integer/pointer returns**: Returned in RAX (x86-64), X0 (ARM64), etc.
- **Floating returns**: Returned in XMM0 (x86-64), V0 (ARM64)
- **Large structures**: Returned via hidden pointer parameter (caller allocates space)

[17] The never type `!` indicates non-returning procedures (noreturn attribute in C).

##### §14.6.2.5 Stack Alignment

[18] The call stack shall be aligned per platform ABI:

- **System V AMD64**: 16-byte alignment before call
- **Microsoft x64**: 16-byte alignment before call
- **ARM64**: 16-byte alignment

[19] Implementations shall ensure stack alignment is maintained across FFI boundaries.

#### §14.6.3 Data Layout

##### §14.6.3.1 Primitive Type Representation

[20] Primitive types in FFI contexts use platform C representations:

**Table 15.6.1 — C type mappings**

| Cursive Type | C Type      | Size (64-bit) | Alignment |
| ------------ | ----------- | ------------- | --------- |
| `i8`         | `int8_t`    | 1             | 1         |
| `i16`        | `int16_t`   | 2             | 2         |
| `i32`        | `int32_t`   | 4             | 4         |
| `i64`        | `int64_t`   | 8             | 8         |
| `isize`      | `intptr_t`  | 8             | 8         |
| `u8`         | `uint8_t`   | 1             | 1         |
| `u16`        | `uint16_t`  | 2             | 2         |
| `u32`        | `uint32_t`  | 4             | 4         |
| `u64`        | `uint64_t`  | 8             | 8         |
| `usize`      | `uintptr_t` | 8             | 8         |
| `f32`        | `float`     | 4             | 4         |
| `f64`        | `double`    | 8             | 8         |
| `bool`       | `uint8_t`   | 1             | 1         |
| `()`         | `void`      | 0             | 1         |
| `*const T`   | `const T*`  | 8             | 8         |
| `*mut T`     | `T*`        | 8             | 8         |

[21] On 32-bit platforms, pointer types and `isize`/`usize` are 4 bytes.

##### §14.6.3.2 repr(C) Structure Layout

[22] Records with `[[repr(C)]]` use C structure layout rules:

**Layout algorithm** (Decision D15):

```
For record R { f₁: T₁, ..., fₙ: Tₙ } with [[repr(C)]]:

offset₀ = 0
For i = 1 to n:
    alignᵢ = alignof(Tᵢ)
    offsetᵢ = next_multiple(offsetᵢ₋₁ + sizeof(Tᵢ₋₁), alignᵢ)

total_size = offsetₙ + sizeof(Tₙ)
struct_align = max(align₁, ..., alignₙ)
padded_size = next_multiple(total_size, struct_align)
```

[23] This matches C compilers for the target platform. Implementations shall test repr(C) compatibility against platform C compiler.

**Example 15.6.3.1 (repr(C) layout matching C):**

```cursive
[[repr(C)]]
record Example {
    a: u8,      // Offset 0
    // 3 bytes padding (automatic)
    b: i32,     // Offset 4
    c: u16,     // Offset 8
    // 2 bytes tail padding
}
// Size: 12, Alignment: 4
// Matches: struct Example { uint8_t a; int32_t b; uint16_t c; };
```

#### §14.6.4 Endianness

[24] Integer and floating-point representation endianness follows the target platform:

- **x86/x86-64**: Little-endian
- **ARM64**: Little-endian (typically)
- **Some platforms**: Big-endian or bi-endian

[25] Implementations shall document endianness for each target platform. Cross-platform serialization must handle endianness explicitly (not a language feature).

#### §14.6.5 Platform-Specific ABI Details

##### §14.6.5.1 System V AMD64 ABI (Linux/Unix x86-64)

[26] **Reference**: System V Application Binary Interface AMD64 Architecture Processor Supplement (§1.2.3[12]).

**Key features**:

- Parameter passing: Registers then stack
- Red zone: 128 bytes below stack pointer (leaf functions may use)
- Stack alignment: 16 bytes before call

##### §14.6.5.2 Microsoft x64 Calling Convention (Windows x86-64)

[27] **Reference**: Microsoft x64 Calling Convention documentation (§1.2.3[13]).

**Key features**:

- Parameter passing: First 4 in registers (RCX, RDX, R8, R9)
- Shadow space: 32 bytes on stack for register parameters
- Stack alignment: 16 bytes before call
- No red zone

##### §14.6.5.3 ARM AAPCS (ARM 32-bit and 64-bit)

[28] **Reference**: ARM Architecture Procedure Call Standard.

**Key features**:

- ARM64: X0-X7 for integer args, V0-V7 for float args
- ARM32: R0-R3 for integer args
- Stack alignment: 16 bytes (ARM64), 8 bytes (ARM32)

##### §14.6.5.4 Implementation Obligations

[29] Implementations shall:

(29.1) Document which platform ABIs are supported

(29.2) Ensure extern(C) procedures use correct platform C ABI

(29.3) Provide ABI compliance test suite verifying interoperability with C

(29.4) Document deviations from standard ABIs (if any)

#### §14.6.6 Diagnostics

[30] ABI diagnostics:

| Code    | Condition                         | Section |
| ------- | --------------------------------- | ------- |
| E15-080 | Unsupported calling convention    | [13]    |
| E15-081 | ABI mismatch (incompatible types) | [22]    |
| E15-082 | Stack misalignment detected       | [19]    |

#### §14.6.7 Conformance Requirements

[31] Implementations shall:

1. Support "C" calling convention on all platforms
2. Implement platform-specific calling conventions correctly
3. Follow platform ABI specifications (System V, Microsoft x64, AAPCS)
4. Generate correct register usage and stack layout
5. Maintain stack alignment per platform requirements
6. Document supported calling conventions
7. Document platform ABI choices
8. Ensure repr(C) types match C compiler layouts
9. Provide ABI compatibility test suite

---

**Previous**: §15.5 Linkage and Symbol Visibility (§15.5 [interop.linkage]) | **Next**: §15.7 Binary Compatibility (§15.7 [interop.compatibility])
