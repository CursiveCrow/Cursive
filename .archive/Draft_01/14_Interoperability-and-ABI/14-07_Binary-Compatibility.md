# Cursive Language Specification

## Clause 14 — Interoperability and ABI

**Part**: XIV — Interoperability and ABI  
**File**: 15-7_Binary-Compatibility.md  
**Section**: §14.7 Binary Compatibility  
**Stable label**: [interop.compatibility]  
**Forward references**: §15.5 [interop.linkage], §15.6 [interop.abi], §1.7 [intro.versioning], Clause 5 §5.5 [decl.type]

---

### §14.7 Binary Compatibility [interop.compatibility]

#### §14.7.1 Overview

[1] Binary compatibility determines whether compiled code from different language versions or compiler versions can be linked together and execute correctly. This subclause specifies stable ABI guarantees, breaking vs non-breaking changes, and versioning considerations for binary compatibility.

[2] Cursive aims for source-level compatibility across minor versions (§1.7) but does not guarantee binary compatibility. Binary compatibility is an implementation-defined quality-of-implementation feature that implementations may provide with documented limitations.

#### §14.7.2 ABI Stability

##### §14.7.2.1 No Guaranteed Binary Compatibility

[3] **Normative**: Cursive does not guarantee binary compatibility across:

- Different compiler versions
- Different language versions (even minor versions)
- Different compilation modes (debug vs release)
- Different optimization levels

[4] Source-level compatibility (§1.7) is guaranteed for minor versions; binary compatibility is not. Programs should be recompiled when upgrading compilers or language versions.

##### §14.7.2.2 Implementation-Defined Stability

[5] Implementations may provide binary compatibility guarantees for specific scenarios:

- Stable ABI for a compiler's lifetime
- Compatible binaries within a major language version
- ABI versioning for library distribution

[6] Such guarantees are implementation-defined and shall be documented. Implementations providing stability guarantees shall specify:

(6.1) Which ABI elements are stable (calling convention, type layout, symbol names)

(6.2) Under what conditions compatibility is maintained

(6.3) How ABI breaks are communicated to users

(6.4) Migration strategies when ABI changes

##### §14.7.2.3 Explicit ABI Versioning

[7] Implementations may use ABI version markers in symbol names or metadata:

```
_Cursive_v1_math_add      // ABI version 1
_Cursive_v2_math_add      // ABI version 2 (incompatible)
```

[8] ABI versioning allows multiple versions to coexist in the same binary.

#### §14.7.3 Breaking Changes

##### §14.7.3.1 ABI-Breaking Changes

[9] The following changes break binary compatibility:

**Type layout changes**:

- Changing field order in records
- Changing field types
- Adding or removing fields from repr(C) types
- Changing enum discriminant representation

**Calling convention changes**:

- Changing extern calling convention
- Changing parameter order
- Changing return type

**Symbol changes**:

- Changing name mangling scheme
- Renaming exported symbols (without aliases)
- Changing symbol visibility (public → internal)

[10] Any of these changes require recompilation of all dependent code.

##### §14.7.3.2 Non-Breaking Changes

[11] The following changes do NOT break binary compatibility (when stability is provided):

**Additive changes**:

- Adding new procedures (new symbols)
- Adding new types
- Adding private fields to types without repr(C)
- Adding inline procedure bodies

**Documentation changes**:

- Changing contracts (grants, preconditions, postconditions)
- Changing comments and documentation
- Adding attributes that don't affect ABI

[12] Non-breaking changes allow library updates without recompiling clients (when implementations provide ABI stability).

#### §14.7.4 Separate Compilation

[13] Cursive supports separate compilation: each compilation unit compiles independently to an object file. The linker combines object files, resolves symbols, and produces the final binary.

**Compilation model**:

```
Source files → Compiler → Object files (.o, .obj)
                          ↓
                       Linker
                          ↓
                    Executable or Library
```

[14] **Object file contents**:

- Compiled code for procedures
- Exported symbols (external linkage)
- Local symbols (internal linkage)
- Relocation information
- Debug information (optional)

[15] Object file formats are platform-specific (ELF on Unix, PE on Windows, Mach-O on macOS). Cursive compilers shall generate valid object files for the target platform.

#### §14.7.5 Dynamic Libraries

[16] Cursive code may be compiled into dynamic libraries (shared objects):

- **Linux**: `.so` files
- **Windows**: `.dll` files
- **macOS**: `.dylib` files

[17] Dynamic libraries export symbols with external linkage. Symbol visibility is controlled by linkage and may be further restricted by platform-specific mechanisms (ELF visibility, Windows \_\_declspec(dllexport)).

[18] **Version compatibility**: Dynamic libraries should maintain ABI compatibility across updates to avoid breaking dependent applications. This is an implementation concern; the specification does not mandate versioning schemes.

#### §14.7.6 Static Libraries

[19] Cursive code may be compiled into static libraries (archives):

- **Linux/macOS**: `.a` files (ar archives)
- **Windows**: `.lib` files

[20] Static libraries contain object files that are linked directly into the final binary. ABI compatibility is less critical for static linking (code is relinked each build).

#### §14.7.7 Practical Guidelines

[21] For maximum binary compatibility, libraries should:

**Use stable types**:

- Mark public FFI types with `[[repr(C)]]`
- Avoid changing field order or types
- Use opaque pointers for implementation details

**Use stable symbols**:

- Use `[[no_mangle]]` for critical symbols
- Avoid renaming public APIs
- Provide symbol aliases for renamed functions

**Version conservatively**:

- Use semantic versioning (§1.7)
- Treat ABI changes as major version bumps
- Provide migration guides for breaking changes

**Document ABI**:

- Specify which symbols are stable
- Document calling conventions
- Provide C header files for FFI-exposed APIs

#### §14.7.8 Examples

**Example 15.7.8.1 (Stable library design):**

```cursive
// version 1.0 - Initial stable API

[[repr(C)]]
public record Config {
    version: u32,
    flags: u32,
    // Future: Can add fields at end if documented as ABI-unstable region
}

[[no_mangle, extern(C)]]
public procedure lib_init(config: *const Config): i32
    [[ ffi::call, unsafe::ptr |- config != null => result >= 0 ]]
{
    unsafe {
        if (*config).version != 1 {
            result -1  // Version mismatch
        }
        // Initialize library
        result 0
    }
}

// version 1.1 - Compatible addition

[[no_mangle, extern(C)]]
public procedure lib_init_v2(config: *const Config, options: u32): i32
    [[ ffi::call, unsafe::ptr |- config != null => result >= 0 ]]
{
    // New function - doesn't break v1.0 users
    result lib_init(config)
}
```

**Example 15.7.8.2 (Opaque pointer pattern for ABI stability):**

```cursive
// Opaque handle - implementation details hidden

record ConnectionImpl {
    socket: i32,
    buffer: Buffer,
    state: ConnectionState,
}

[[no_mangle, extern(C)]]
public procedure connection_create(): *mut ()
    [[ ffi::call, unsafe::ptr, alloc::heap |- true => true ]]
{
    let conn = ConnectionImpl::new()
    result &conn as *mut ()  // Opaque pointer to C
}

[[no_mangle, extern(C)]]
public procedure connection_send(handle: *mut (), data: *const u8, len: usize): i32
    [[ ffi::call, unsafe::ptr |- handle != null, data != null => true ]]
{
    unsafe {
        let conn: *mut ConnectionImpl = handle as *mut ConnectionImpl
        (*conn).send(data, len)
    }
}

// C code sees opaque handle; internal structure can change without ABI break
```

#### §14.7.9 Diagnostics

[22] Binary compatibility diagnostics:

| Code    | Condition                                          | Section |
| ------- | -------------------------------------------------- | ------- |
| E14-090 | Link-time symbol conflict (ODR violation detected) | [10]    |
| E14-091 | ABI version mismatch                               | [8]     |
| E14-092 | Incompatible object file format                    | [15]    |

These are link-time or load-time errors, not compile-time.

#### §14.7.10 Conformance Requirements

[23] Implementations shall:

1. Document binary compatibility guarantees (or lack thereof)
2. Follow platform ABI specifications (System V, Windows x64, etc.)
3. Generate valid object files for target platform
4. Support static and dynamic library generation
5. Detect ODR violations at link time when possible
6. Document ABI versioning scheme (if any)
7. Provide tools for symbol inspection and demangling
8. Maintain repr(C) layout compatibility with platform C compiler
9. Warn when ABI-breaking changes occur (quality-of-implementation)

---

**Previous**: §15.6 ABI Specification (§15.6 [interop.abi]) | **Next**: Clause 16 — Compile-Time Evaluation and Reflection (§16 [comptime])
