# Cursive Language Specification

## Clause 14 — Interoperability and ABI

**Part**: XIV — Interoperability and ABI  
**File**: 15-4_Platform-Features.md  
**Section**: §14.4 Platform-Specific Features  
**Stable label**: [interop.platform]  
**Forward references**: Clause 2 §2.2 [lex.phases], Clause 8 §8.7 [expr.constant], Clause 16 [comptime]

---

### §14.4 Platform-Specific Features [interop.platform]

#### §14.4.1 Overview

[1] Platform-specific features enable Cursive programs to access platform-dependent capabilities: inline assembly (future edition), SIMD intrinsics (future edition), and platform-conditional compilation. This subclause specifies the mechanisms for handling platform variability while maintaining portability where possible.

[2] Cursive's approach to platform-specific code emphasizes explicit opt-in through compile-time evaluation rather than preprocessor directives or attributes. This maintains the language's "execution over declaration" philosophy.

#### §14.4.2 Platform-Conditional Compilation

##### §14.4.2.1 Comptime Platform Queries

[3] Platform-specific code uses comptime blocks with platform queries (Decision D9):

```cursive
comptime {
    if platform::os() == "linux" {
        // Linux-specific declarations
        codegen::declare_extern(
            calling_convention: "C",
            name: "epoll_create",
            params: [(id: "maxevents", ty: codegen::type_i32())],
            return_ty: codegen::type_i32()
        )
    } else if platform::os() == "windows" {
        // Windows-specific declarations
        codegen::declare_extern(
            calling_convention: "stdcall",
            name: "CreateIoCompletionPort",
            params: [
                (id: "handle", ty: codegen::type_ptr()),
                // ...
            ],
            return_ty: codegen::type_ptr()
        )
    }
}
```

[4] Platform queries are available in comptime contexts only. Runtime platform detection uses standard library facilities, not language features.

##### §14.4.2.2 Available Platform Queries

[5] The `platform` module (provided by comptime environment) offers:

**OS detection**:

```cursive
platform::os(): string@View
// Returns: "linux", "windows", "macos", "freebsd", etc.
```

**Architecture detection**:

```cursive
platform::arch(): string@View
// Returns: "x86_64", "aarch64", "x86", "arm", etc.
```

**Pointer width**:

```cursive
platform::pointer_width(): usize
// Returns: 32, 64, etc.
```

**Endianness**:

```cursive
platform::endian(): string@View
// Returns: "little", "big"
```

[6] Complete platform query APIs are specified in Clause 16 §16.2 [comptime.reflection].

##### §14.4.2.3 Comptime Code Generation

[7] Platform-specific declarations use the `codegen` API (Clause 16 §16.3):

```cursive
comptime {
    let os = platform::os()

    match os {
        "linux" => {
            codegen::declare_extern(
                calling_convention: "C",
                name: "pthread_create",
                // ...
            )
        }
        "windows" => {
            codegen::declare_extern(
                calling_convention: "stdcall",
                name: "CreateThread",
                // ...
            )
        }
        _ => {
            // Fallback or error
            codegen::error("Unsupported platform: {}", os)
        }
    }
}
```

[8] The `codegen` API generates FFI declarations at compile time based on platform characteristics.

#### §14.4.3 Inline Assembly (Future Edition)

[9] Inline assembly is deferred to a future edition of this specification (Decision D6).

[ Note: Cursive version 1.0 does not support inline assembly. Users requiring assembly code should:

1. Write assembly in separate `.s` or `.asm` files and link them, OR
2. Wrap assembly in C functions and call via FFI, OR
3. Use platform-specific intrinsics when available (future feature)

Future editions (v1.1+) may introduce inline assembly with syntax like:

```cursive
procedure atomic_fence()
    [[ unsafe::asm |- true => true ]]
{
    unsafe {
        asm! {
            template: "mfence",
            clobbers: ["memory"],
        }
    }
}
```

The exact syntax and semantics will be designed based on field experience and community feedback, ensuring the feature aligns with Cursive's explicit and safe-by-default philosophy.
— end note ]

#### §14.4.4 SIMD Intrinsics (Future Edition)

[10] SIMD (Single Instruction Multiple Data) intrinsics are deferred to a future edition.

[ Note: SIMD support is planned for future editions. The design will likely use:

- Explicit vector types: `v128`, `v256` (bit-width types)
- Intrinsic functions in `simd::` namespace
- Platform-conditional availability via comptime queries

Example (future):

```cursive
comptime {
    if platform::has_feature("avx2") {
        codegen::import_intrinsics("simd::avx2")
    }
}

procedure vectorized_add(a: v256, b: v256): v256
    [[ unsafe::simd |- true => true ]]
{
    result simd::avx2::add_ps(a, b)
}
```

Design will be finalized in v1.1+ based on implementation experience.
— end note ]

#### §14.4.5 Platform-Specific Attributes

[11] Platform-specific attributes are implementation-defined extensions. Implementations may provide attributes for:

- Target-specific optimizations: `[[target_feature("+avx2")]]`
- Calling conventions: `[[extern("vectorcall")]]` (Windows)
- Memory layout: `[[repr(align(64))]]` for cache-line alignment

[12] Implementation-defined attributes shall be documented in implementation guides and shall not alter standard language semantics beyond their documented effects.

#### §14.4.6 Diagnostics

[13] Platform feature diagnostics:

| Code    | Condition                                         | Section |
| ------- | ------------------------------------------------- | ------- |
| E15-050 | Platform query used outside comptime context      | [3]     |
| E15-051 | Inline assembly attempted (not supported in v1.0) | [9]     |
| E15-052 | SIMD intrinsic attempted (not supported in v1.0)  | [10]    |
| E15-053 | Unsupported platform-specific attribute           | [11]    |

#### §14.4.7 Examples

**Example 15.4.7.1 (Platform-conditional extern declarations):**

```cursive
// Platform-specific system calls

comptime {
    if platform::os() == "linux" {
        codegen::emit("""
            [[extern(C)]]
            procedure open(path: *const u8 [[null_terminated]], flags: i32): i32
                [[ ffi::call, unsafe::ptr |- path != null => true ]]
        """)
    } else if platform::os() == "windows" {
        codegen::emit("""
            [[extern(stdcall)]]
            procedure CreateFileA(
                filename: *const u8 [[null_terminated]],
                access: u32,
                share_mode: u32,
                security: *const (),
                creation: u32,
                flags: u32,
                template: *const ()
            ): *mut ()
                [[ ffi::call, unsafe::ptr |- filename != null => true ]]
        """)
    }
}
```

**Example 15.4.7.2 (Platform-specific constants):**

```cursive
let PATH_SEPARATOR: const string@View = comptime {
    if platform::os() == "windows" {
        result "\\"
    } else {
        result "/"
    }
}
```

#### §14.4.8 Conformance Requirements

[14] Implementations shall:

1. Provide `platform::os()`, `platform::arch()`, `platform::pointer_width()`, `platform::endian()` in comptime contexts
2. Support comptime-based platform-conditional code generation
3. Document all implementation-defined platform-specific attributes
4. Reserve inline assembly and SIMD for future editions
5. Emit diagnostic E15-051 if inline assembly is attempted
6. Emit diagnostic E15-052 if SIMD intrinsics are attempted
7. Integrate platform queries with Clause 16 comptime evaluation

---

**Previous**: §15.3 C Compatibility (§15.3 [interop.c]) | **Next**: §15.5 Linkage and Symbol Visibility (§15.5 [interop.linkage])
