# Cursive Language Specification

## Clause 15 — Compile-Time Evaluation and Reflection

**Part**: XV — Compile-Time Evaluation and Reflection  
**File**: 16-4_Comptime-Intrinsics-and-Configuration.md  
**Section**: §15.4 Comptime Intrinsics and Configuration  
**Stable label**: [comptime.intrinsics]  
**Forward references**: §16.2 [comptime.procedures], §16.3 [comptime.blocks], §16.8 [comptime.codegen.api], Clause 7 §7.8 [type.introspection], Clause 12 §12.3 [contract.grant], Annex E §E.5 [implementation.diagnostics]

---

### §15.4 Comptime Intrinsics and Configuration [comptime.intrinsics]

#### §15.4.1 Overview [comptime.intrinsics.overview]

[1] _Comptime intrinsics_ are built-in procedures provided by the compiler for compile-time validation, diagnostic emission, and platform/configuration queries. They enable assertion-based validation, conditional compilation, and platform-specific code generation.

[2] This subclause specifies all comptime intrinsic procedures, their signatures, semantics, grant requirements, and usage patterns.

#### §15.4.2 Assertion and Validation Intrinsics [comptime.intrinsics.assertions]

##### §15.4.2.1 comptime_assert

[3] Compile-time assertion procedure:

```cursive
comptime procedure comptime_assert(condition: bool, message: string@View): ()
    [[ comptime::diag |- true => true ]]
```

[4] **Semantics.** Evaluates `condition` at compile time:

- If `condition == true`: Execution continues normally
- If `condition == false`: Compilation fails with diagnostic E15-030 and the provided `message`

[5] **Typing rule:**

$$
\frac{\Gamma_{\text{ct}} \vdash condition : \texttt{bool} \quad \Gamma_{\text{ct}} \vdash message : \texttt{string@View}}{\Gamma_{\text{ct}} \vdash \texttt{comptime\_assert}(condition, message) : ()}
\tag{T-Comptime-Assert}
$$

[6] **Evaluation rule (success case):**

$$
\frac{\langle condition, \sigma_{\text{ct}} \rangle \Downarrow_{\text{ct}} \langle \texttt{true}, \sigma_{\text{ct}} \rangle}{\langle \texttt{comptime\_assert}(condition, message), \sigma_{\text{ct}} \rangle \Downarrow_{\text{ct}} \langle (), \sigma_{\text{ct}} \rangle}
\tag{E-Assert-Success}
$$

[7] **Evaluation rule (failure case):**

$$
\frac{\langle condition, \sigma_{\text{ct}} \rangle \Downarrow_{\text{ct}} \langle \texttt{false}, \sigma_{\text{ct}} \rangle}{\langle \texttt{comptime\_assert}(condition, message), \sigma_{\text{ct}} \rangle \Downarrow_{\text{ct}} \text{ERROR E15-030}}
\tag{E-Assert-Fail}
$$

**Example 16.4.2.1** (Type layout assertion):

```cursive
[[reflect]]
record Packet {
    header: [u8; 16],
    payload: [u8; 1024],
}

comptime {
    let info <- reflect_type::<Packet>()
    comptime_assert(info.size == 1040, "Packet size must be 1040 bytes")
}
```

##### §15.4.2.2 comptime_error

[8] Unconditional compile-time error:

```cursive
comptime procedure comptime_error(message: string@View): !
    [[ comptime::diag |- true => false ]]
```

[9] **Semantics.** Unconditionally fails compilation with diagnostic E16-031 and the provided `message`. Returns never type `!` because execution does not continue.

[10] **Evaluation rule:**

$$
\frac{\langle message, \sigma_{\text{ct}} \rangle \Downarrow_{\text{ct}} \langle msg, \sigma_{\text{ct}} \rangle}{\langle \texttt{comptime\_error}(message), \sigma_{\text{ct}} \rangle \Downarrow_{\text{ct}} \text{ERROR E16-031}(msg)}
\tag{E-Comptime-Error}
$$

**Example 16.4.2.2** (Configuration validation):

```cursive
comptime {
    let feature_enabled <- cfg("required_feature")

    if !feature_enabled {
        comptime_error("This module requires 'required_feature' to be enabled")
    }
}
```

##### §15.4.2.3 comptime_warning

[11] Compile-time warning emission:

```cursive
comptime procedure comptime_warning(message: string@View): ()
    [[ comptime::diag |- true => true ]]
```

[12] **Semantics.** Emits warning diagnostic E16-032 with the provided `message`. Compilation continues normally.

**Example 16.4.2.3** (Deprecation warning):

```cursive
comptime {
    if cfg("legacy_mode") {
        comptime_warning("legacy_mode is deprecated, use new_mode instead")
    }
}
```

##### §15.4.2.4 comptime_note

[13] Informational note emission:

```cursive
comptime procedure comptime_note(message: string@View): ()
    [[ comptime::diag |- true => true ]]
```

[14] **Semantics.** Emits informational note to compilation output. Does not affect compilation success. Useful for debugging and documenting compile-time decisions.

**Example 16.4.2.4** (Logging code generation):

```cursive
comptime {
    let type_count: usize = 5

    comptime_note("Generating specialized procedures...")

    loop i in 0..type_count {
        comptime_note(string_concat("  Processing type ", usize_to_string(i)))
        // Generate code...
    }

    comptime_note("Code generation complete")
}
```

#### §15.4.3 Configuration Query Intrinsics [comptime.intrinsics.config]

##### §15.4.3.1 Boolean Configuration Queries

[15] Check whether configuration flags are enabled:

```cursive
comptime procedure cfg(key: string@View): bool
    [[ comptime::config |- true => true ]]
```

[16] **Semantics.** Returns `true` if the configuration flag identified by `key` is enabled for the current compilation, `false` otherwise.

[17] **Common configuration keys:**

- `"debug_assertions"` — Debug mode enabled
- `"test"` — Test compilation mode
- `"feature_{name}"` — Custom feature flags
- Platform-specific features (implementation-defined)

**Example 16.4.3.1** (Debug-mode code generation):

```cursive
comptime {
    if cfg("debug_assertions") {
        codegen::declare_constant(
            name: "DEBUG_CHECKS",
            ty: codegen::type_named("bool"),
            value: quote { true }
        )
    } else {
        codegen::declare_constant(
            name: "DEBUG_CHECKS",
            ty: codegen::type_named("bool"),
            value: quote { false }
        )
    }
}
```

##### §15.4.3.2 String Configuration Queries

[18] Retrieve configuration values as strings:

```cursive
comptime procedure cfg_value(key: string@View): string@View
    [[ comptime::config |- true => true ]]
```

[19] **Semantics.** Returns the string value associated with configuration `key`. If the key is not set, returns empty string `""`.

**Example 16.4.3.2** (Retrieving version from config):

```cursive
let VERSION: const string@View = comptime {
    let ver <- cfg_value("package_version")

    if ver.len() == 0 {
        result "0.0.0"  // Default
    } else {
        result ver
    }
}
```

##### §15.4.3.3 Platform Query Intrinsics

[20] Query target platform characteristics:

**target_os**:

```cursive
comptime procedure target_os(): string@View
    [[ comptime::config |- true => true ]]
```

[21] **Semantics.** Returns the target operating system identifier. Common values include:

- `"linux"` — Linux systems
- `"windows"` — Windows systems
- `"macos"` — macOS systems
- `"freebsd"` — FreeBSD systems
- Other values are implementation-defined

**target_arch**:

```cursive
comptime procedure target_arch(): string@View
    [[ comptime::config |- true => true ]]
```

[22] **Semantics.** Returns the target CPU architecture. Common values:

- `"x86_64"` — x86-64 (AMD64)
- `"aarch64"` — ARM64
- `"x86"` — 32-bit x86
- `"arm"` — 32-bit ARM

**target_endian**:

```cursive
comptime procedure target_endian(): string@View
    [[ comptime::config |- true => true ]]
```

[23] **Semantics.** Returns `"little"` for little-endian platforms, `"big"` for big-endian platforms.

**target_pointer_width**:

```cursive
comptime procedure target_pointer_width(): usize
    [[ comptime::config |- true => true ]]
```

[24] **Semantics.** Returns pointer width in bits: `32` for 32-bit platforms, `64` for 64-bit platforms.

##### §15.4.3.4 Platform-Conditional Code

**Example 16.4.3.3** (OS-specific constants):

```cursive
let LINE_ENDING: const string@View = comptime {
    let os <- target_os()

    match os {
        "windows" => result "\r\n",
        "linux" => result "\n",
        "macos" => result "\n",
        _ => {
            comptime_warning("Unknown OS, using Unix line ending")
            result "\n"
        }
    }
}
```

**Example 16.4.3.4** (Architecture-specific buffer sizes):

```cursive
let CACHE_LINE_SIZE: const usize = comptime {
    let arch <- target_arch()

    if arch == "x86_64" || arch == "aarch64" {
        result 64  // 64-byte cache lines
    } else {
        result 32  // Conservative default
    }
}

[[repr(align(CACHE_LINE_SIZE))]]
record CacheAligned {
    data: [u8; CACHE_LINE_SIZE],
}
```

#### §15.4.4 Type Property Intrinsics [comptime.intrinsics.types]

##### §15.4.4.1 Basic Type Queries

[25] These intrinsics extend §7.8 [type.introspection] for comptime contexts:

**type_name**:

```cursive
comptime procedure type_name<T>(): string@View
    [[ |- true => true ]]
```

[26] Returns the fully-qualified name of type `T`.

**type_id**:

```cursive
comptime procedure type_id<T>(): u64
    [[ |- true => true ]]
```

[27] Returns a unique identifier for type `T`. Identical types have identical IDs.

**size_of**:

```cursive
comptime procedure size_of<T>(): usize
    [[ |- true => true ]]
```

[28] Returns the size of type `T` in bytes (§10.6 [memory.layout]).

**align_of**:

```cursive
comptime procedure align_of<T>(): usize
    [[ |- true => true ]]
```

[29] Returns the alignment requirement of type `T` as a power of two.

##### §15.4.4.2 Type Property Predicates

[30] Query type properties at compile time:

```cursive
comptime procedure is_copy<T>(): bool
    [[ |- true => true ]]

comptime procedure is_sized<T>(): bool
    [[ |- true => true ]]
```

[31] **Semantics:**

- `is_copy<T>()`: Returns `true` if `T` satisfies the `Copy` behavior (§10.4.5.2)
- `is_sized<T>()`: Returns `true` if `T` is a sized type (§10.4.5.2 [generic.behavior.sized])

**Example 16.4.4.1** (Conditional compilation based on Copy):

```cursive
comptime procedure generate_clone<T>(): ()
    [[ comptime::codegen |- true => true ]]
{
    if !is_copy::<T>() {
        // Generate explicit clone procedure for non-Copy types
        codegen::add_procedure(
            target: TypeRef::TypeId(type_id<T>()),
            spec: build_clone_spec::<T>()
        )
    }
    // Copy types get default clone behavior
}
```

#### §15.4.5 Utility Intrinsics [comptime.intrinsics.utilities]

##### §15.4.5.1 String Manipulation

[32] Compile-time string operations:

```cursive
comptime procedure string_concat(a: string@View, b: string@View): string@View
    [[ comptime::alloc |- a.len() + b.len() < 1048576 => true ]]
```

[33] **Semantics.** Concatenates strings `a` and `b`. Result is a compile-time constant string. Total length shall not exceed 1 MiB (§16.2.3.3[14]).

**Example 16.4.5.1** (Building identifiers):

```cursive
comptime {
    let prefix: const string@View = "get_"
    let field_name: const string@View = "balance"
    let getter_name = string_concat(prefix, field_name)

    // getter_name = "get_balance"
}
```

##### §15.4.5.2 Numeric Conversions

[34] Convert between numbers and strings at compile time:

```cursive
comptime procedure usize_to_string(value: usize): string@View
    [[ comptime::alloc |- true => true ]]

comptime procedure parse_usize(text: string@View): usize \/ Unit
    [[ |- text.len() > 0 => true ]]
```

[35] **Semantics:**

- `usize_to_string`: Converts integer to decimal string representation
- `parse_usize`: Parses decimal string to integer; returns unit on failure

**Example 16.4.5.2** (Numeric configuration):

```cursive
let THREAD_COUNT: const usize = comptime {
    let config_value <- cfg_value("thread_count")

    match parse_usize(config_value) {
        count: usize => {
            if count > 0 && count <= 64 {
                result count
            } else {
                comptime_warning("Invalid thread count, using default")
                result 4
            }
        },
        _: () => result 4,  // Default
    }
}
```

##### §15.4.5.3 Arithmetic Utilities

[36] Helper procedures for comptime arithmetic:

```cursive
comptime procedure is_power_of_two(n: usize): bool
    [[ |- true => true ]]
{
    result n > 0 && (n & (n - 1)) == 0
}

comptime procedure round_up_to_power_of_two(n: usize): usize
    [[ |- n > 0 => result >= n ]]
{
    var power: usize = 1

    loop power < n {
        power = power * 2
    }

    result power
}
```

#### §15.4.6 Comprehensive Examples [comptime.intrinsics.examples]

##### §15.4.6.1 Configuration-Driven Code Generation

**Example 16.4.6.1** (Feature-gated procedures):

```cursive
comptime {
    if cfg("experimental_features") {
        comptime_warning("Experimental features enabled")

        codegen::declare_procedure(codegen::ProcedureSpec {
            name: "experimental_optimize",
            visibility: codegen::Visibility::Public,
            receiver: codegen::ReceiverSpec::None,
            params: [
                codegen::ParamSpec {
                    name: "data",
                    ty: TypeRef::Slice(TypeRef::Named("u8")),
                    permission: Permission::Const,
                    responsible: false,
                },
            ],
            return_type: TypeRef::Slice(TypeRef::Named("u8")),
            sequent: codegen::sequent_pure(),
            body: quote {
                // Experimental optimization logic
                result optimized_data
            },
        })
    } else {
        comptime_note("Experimental features disabled")
    }
}
```

##### §15.4.6.2 Platform-Specific Optimization

**Example 16.4.6.2** (SIMD-aware buffer size):

```cursive
let SIMD_WIDTH: const usize = comptime {
    let arch <- target_arch()

    if arch == "x86_64" {
        if cfg("avx2") {
            comptime_note("Using AVX2 SIMD width")
            result 32  // 256-bit AVX2
        } else {
            result 16  // 128-bit SSE
        }
    } else if arch == "aarch64" {
        result 16  // 128-bit NEON
    } else {
        comptime_warning("Unknown architecture for SIMD")
        result 8  // Conservative
    }
}

type SimdBuffer = [f32; SIMD_WIDTH]
```

#### §15.4.7 Diagnostics [comptime.intrinsics.diagnostics]

[37] Intrinsic-related diagnostics:

[Note: Diagnostics defined in this subsection are cataloged in Annex E §E.5.1.15. — end note]

#### §15.4.8 Conformance Requirements [comptime.intrinsics.requirements]

[38] A conforming implementation SHALL:

1. Provide comptime_assert, comptime_error, comptime_warning, comptime_note intrinsics
2. Provide cfg and cfg_value configuration query intrinsics
3. Provide target_os, target_arch, target_endian, target_pointer_width platform queries
4. Provide type property intrinsics: type_name, type_id, size_of, align_of, is_copy, is_sized
5. Provide string manipulation: string_concat
6. Provide numeric conversion: usize_to_string, parse_usize
7. Execute intrinsics during compile-time execution phase only
8. Emit diagnostics E16-030, E16-031, E16-032 for assertion/error/warning intrinsics
9. Return accurate platform information consistent with compilation target
10. Guarantee deterministic results for all intrinsic calls

[39] A conforming implementation MAY:

1. Provide additional comptime intrinsics as extensions
2. Provide additional configuration keys beyond standard set
3. Optimize intrinsic execution for performance

[40] A conforming implementation SHALL NOT:

1. Execute intrinsics non-deterministically
2. Allow platform queries in runtime contexts
3. Return inaccurate platform information

---

**Previous**: §16.3 Comptime Blocks and Contexts (§16.3 [comptime.blocks]) | **Next**: §16.5 Reflection Opt-In and Attributes (§16.5 [comptime.reflect.optin])
