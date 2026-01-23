# Cursive Language Specification

## Clause 14 — Interoperability and ABI

**Part**: XIV — Interoperability and ABI  
**File**: 15-01_FFI-Declarations.md  
**Section**: §14.1 FFI Declarations and Safety Obligations  
**Stable label**: [interop.ffi]  
**Forward references**: §14.2 [interop.unsafe], §14.3 [interop.c], §14.5 [interop.linkage], §14.6 [interop.abi], Clause 5 §5.4 [type.function], Clause 9 §9.8 [memory.unsafe], Clause 10 §10.3 [contract.grant], Annex A §A.6 [grammar.declaration]

---

### §14.1 FFI Declarations and Safety Obligations [interop.ffi]

#### §14.1.1 Overview

[1] The Foreign Function Interface (FFI) enables Cursive programs to interoperate with code written in other languages, primarily C. FFI declarations specify foreign procedures, their signatures, calling conventions, and safety obligations that programmers must satisfy when crossing language boundaries. FFI introduces controlled unsafety: the compiler cannot verify that foreign code respects Cursive's memory model, permission system, or contracts. Programmers assume responsibility for ensuring foreign functions maintain safety invariants.

#### §14.1.2 Syntax

[4] FFI declarations use the `extern` attribute with calling convention specification, matching the pattern:

```
"[[" "extern" "(" <calling_convention> ")" [ "," <ffi_attributes> ] "]]"
    [ <visibility> ] "procedure" <identifier>
    [ <generic_params> ] "(" [ <parameter_list> ] ")" [ ":" <type> ]
    [ <sequent_clause> ]
    <callable_body>
```

where **calling conventions** are:

```
<string_literal>    // e.g., "C", "stdcall", "fastcall", "system"
```

**FFI attributes** match the pattern:

```
<ffi_attribute> [ "," <ffi_attribute> ... ]
```

**FFI attribute forms** are:

```
"unwind" "(" <unwind_behavior> ")"
"no_mangle"
"weak"
```

**Unwind behaviors** are:

```
"abort"
"catch"
```

[ Note: See Annex A §A.6 [grammar.declaration] for the normative `procedure_decl` production with extern attributes.
— end note ]

[5] The `extern` attribute marks a procedure as using a foreign calling convention. The calling convention string specifies the ABI: `"C"` for C calling convention, `"stdcall"` for Windows stdcall, `"fastcall"` for fastcall, `"system"` for platform system ABI.

[6] FFI procedures may provide bodies (Cursive implementations exported to foreign code) or omit bodies (foreign implementations imported to Cursive code):

```cursive
// Foreign import (no body)
[[extern(C)]]
procedure malloc(size: usize): *mut u8
    [[ ffi::call, unsafe::ptr |- size > 0 => result != null ]]

// Cursive export to C (with body)
[[extern(C)]]
public procedure cursive_add(a: i32, b: i32): i32
    [[ |- true => true ]]
{
    result a + b
}
```

#### §15.1.3 Constraints

[1] _Extern attribute requirement._ Procedures declared with `[[extern(...)]]` shall specify a valid calling convention string. Invalid conventions produce diagnostic E14-001. The calling convention shall be one of: `"C"`, `"stdcall"`, `"fastcall"`, `"system"`, or an implementation-defined platform-specific convention documented by the implementation.

[2] _FFI-safe type restriction._ Extern procedure signatures shall use only FFI-safe types (§14.1.4). Non-FFI-safe types in extern signatures produce diagnostic E14-002. FFI-safe types include:

- Primitive integer types: `i8`, `i16`, `i32`, `i64`, `i128`, `isize`, `u8`, `u16`, `u32`, `u64`, `u128`, `usize`
- Primitive floating-point types: `f32`, `f64`
- Boolean type: `bool` (represented as `u8` in FFI)
- Unit type: `()` (represented as C `void`)
- Raw pointers: `*const T`, `*mut T` where `T` is FFI-safe
- Records with `[[repr(C)]]` attribute
- Enums with `[[repr(C)]]` attribute
- Function pointers with extern calling conventions

[3] _Grant requirements._ Extern procedures shall declare the `ffi::call` grant in their contractual sequent. Foreign procedures accepting or returning pointers shall additionally declare `unsafe::ptr`. Omitting required grants produces diagnostic E14-003.

[4] _Visibility requirements._ Extern procedures with bodies (exports) shall be declared `public` to be callable from foreign code. Non-public extern procedures with bodies produce diagnostic E14-004 with note suggesting either making the procedure public or removing the extern attribute.

[5] _No variadic support._ Cursive does not support calling C variadic functions (e.g., `printf`) directly. Extern signatures shall not include ellipsis (`...`) or variadic parameters. Attempting to declare variadic extern procedures produces diagnostic E14-005.

[ Note: Users requiring variadic C function calls should wrap them in non-variadic C helper functions that accept explicit parameters. For example, instead of calling `printf` directly, create a C wrapper `printf_int(format, value)` that calls `printf` internally.
— end note ]

[6] _Body requirements._ Extern procedures without bodies (foreign imports) are declarations only; they shall be defined externally and linked at link time. Extern procedures with bodies (Cursive exports) shall provide complete implementations following standard procedure semantics (Clause 4 §4.4).

[7] _Permission stripping._ Extern procedure parameters and return types shall not include permission qualifiers (`const`, `unique`, `shared`). Permissions are Cursive-specific and have no C representation. Using permissions in extern signatures produces diagnostic E14-006.

[8] _Modal type prohibition._ Extern signatures shall not use modal types (state-annotated types like `File@Open`) or safe modal pointers (`Ptr<T>@Valid`). Modal states are Cursive-specific and cannot be represented in foreign ABIs. Violations produce diagnostic E14-007.

[9] _No captures for extern procedures._ Extern procedures shall not capture variables from enclosing scopes. Closures cannot be declared as extern. Violations produce diagnostic E14-008.

[10] _Unwind attribute placement._ The `unwind(...)` attribute may only appear on extern procedures. Using it on non-extern procedures produces diagnostic E14-009.

#### §15.1.4 FFI-Safe Types

##### §15.1.4.1 Definition

[11] A type is _FFI-safe_ when it has a defined representation compatible with foreign calling conventions and can be passed across language boundaries without special handling.

##### §15.1.4.2 FFI-Safe Type Catalog

[12] The following types are FFI-safe:

**Primitive types** (§7.2):

- All integer types: `i8`, `i16`, `i32`, `i64`, `i128`, `isize`, `u8`, `u16`, `u32`, `u64`, `u128`, `usize`
- Floating-point types: `f32` (as C `float`), `f64` (as C `double`)
- Boolean: `bool` (represented as C `uint8_t` with values 0 or 1)
- Unit: `()` (represented as C `void`)

**Pointer types** (§7.5):

- Raw pointers: `*const T`, `*mut T` where `T` is FFI-safe
- Function pointers with extern calling conventions: `[[extern(C)]] procedure(...): T`

**Composite types with repr(C)** (§15.3.2):

- Records: `[[repr(C)]] record Name { ... }` with FFI-safe fields
- Enums: `[[repr(C)]] enum Name { ... }` with explicit discriminant representation

**Never type**:

- `!` (represented as C `void` in return position, indicates non-returning function)

##### §15.1.4.3 Non-FFI-Safe Types

[13] The following types are **not** FFI-safe and shall not appear in extern signatures:

- String types: `string@Managed`, `string@View` (use `*const u8` with `[[null_terminated]]` instead)
- Safe pointers: `Ptr<T>@State` (use raw pointers `*const T`, `*mut T` instead)
- Modal types: `Type@State` (no foreign equivalent)
- Witness types: `witness<B>` (no foreign equivalent)
- Union types: `T \/ U` (use repr(C) enum instead)
- Slices: `[T]` (use pointer + length pair in repr(C) struct)
- Arrays without repr(C): `[T; n]` alone is not guaranteed FFI-safe without repr(C) on containing struct
- Tuples: `(T, U)` (no guaranteed C layout, use repr(C) struct instead)
- Permission-qualified types: `const T`, `unique T`, `shared T` (permissions are Cursive-only)

##### §15.1.4.4 Type Compatibility Checking

[14] The compiler shall enforce FFI-safe type requirements at compile time:

[ Given: Extern procedure signature with parameter type $\tau$ or return type $\tau$ ]

$$
\frac{\tau \in \text{FFISafeTypes}}
     {\text{extern signature valid}}
\tag{WF-FFI-Type-Safe}
$$

[15] When $\tau \notin \text{FFISafeTypes}$, the compiler shall emit diagnostic E14-002 identifying the non-FFI-safe type and suggesting FFI-safe alternatives.

#### §15.1.5 Grant and Contract Requirements

##### §15.1.5.1 FFI Grants

[16] Extern procedures require explicit capability grants documenting their foreign interactions:

**Required grants**:

- `ffi::call`: Required for all extern procedure declarations and calls
- `unsafe::ptr`: Required when signature includes raw pointers
- `panic`: Required for extern procedures that may panic (if `unwind(catch)` specified)

[17] **Grant checking rule**:

[ Given: Extern procedure with signature including pointers ]

$$
\frac{\text{extern signature uses } *\text{const } T \text{ or } *\text{mut } T}
     {\text{sequent shall include } \texttt{ffi::call} \cup \texttt{unsafe::ptr}}
\tag{WF-FFI-Grant-Ptr}
$$

[18] Missing grants produce diagnostic E14-003 listing required grants.

##### §15.1.5.2 Contractual Sequents on Extern Procedures

[19] Extern procedures should declare contractual sequents documenting:

- **Grants**: Capabilities required (ffi::call, unsafe::ptr, etc.)
- **Preconditions**: Caller obligations (pointer validity, null checks, alignment)
- **Postconditions**: Foreign function guarantees (return value properties, side effects)

[20] Sequents on extern procedures serve as machine-readable documentation of foreign function contracts. The compiler cannot verify foreign code satisfies these contracts; programmers assume responsibility.

**Example 15.1.5.1 (Extern contracts):**

```cursive
[[extern(C)]]
procedure malloc(size: usize): *mut u8
    [[
        ffi::call, unsafe::ptr
        |-
        size > 0
        =>
        (result != null) \/ (result == null)
    ]]

[[extern(C)]]
procedure free(ptr: *mut u8)
    [[
        ffi::call, unsafe::ptr
        |-
        ptr != null
        =>
        true
    ]]
```

#### §15.1.6 Calling Foreign Functions

[21] Calls to extern procedures require `unsafe` blocks (§11.8) because the compiler cannot verify foreign code maintains Cursive invariants:

```cursive
let ptr: *mut u8 = unsafe {
    malloc(1024)  // FFI call in unsafe block
}

unsafe {
    free(ptr)
}
```

[22] **Call site requirements**:

(22.1) All calls to extern procedures shall occur within `unsafe` blocks. Calling extern procedures outside unsafe produces diagnostic E14-010.

(22.2) Caller's grant clause shall include all grants required by the extern procedure's sequent. Missing grants produce diagnostic E10-030 (standard grant checking, Clause 10).

(22.3) Preconditions in the extern procedure's sequent shall be satisfied at the call site. The compiler performs standard precondition checking (§11.7) but cannot verify the foreign implementation respects postconditions.

#### §15.1.7 Exporting Cursive Procedures to Foreign Code

[23] Cursive procedures marked with `[[extern(C)]]` and providing bodies are exported for foreign code to call:

**Example 15.1.7.1 (Cursive export to C):**

```cursive
[[extern(C)]]
public procedure cursive_compute(x: i32, y: i32): i32
    [[ |- x > 0, y > 0 => result > 0 ]]
{
    result x * y + x + y
}
```

[24] **Export requirements**:

(24.1) Exported procedures shall be declared `public` (visible for export).

(24.2) Exported procedures shall have FFI-safe signatures (primitives, raw pointers, repr(C) types only).

(24.3) Exported procedures should be pure or have minimal grants. Foreign code cannot provide grants; complex Cursive operations (I/O, allocation) may fail when called from foreign contexts.

(24.4) Exported procedures shall not panic unless `unwind(catch)` is specified. The default `unwind(abort)` causes process termination on panic (§15.2.4).

#### §15.1.8 Unwind Behavior

[25] The `unwind` attribute controls panic behavior when extern procedures are called from foreign code:

**unwind(abort)** (default):
[26] If a panic occurs while executing an extern procedure called from foreign code, the process aborts immediately. Unwinding does not cross the FFI boundary into foreign stack frames.

$$
\frac{\text{extern proc called from foreign} \quad \text{panic occurs}}
     {\text{process aborts (never unwinds into foreign code)}}
\tag{E-FFI-Unwind-Abort}
$$

**unwind(catch)**:
[27] Panics are caught at the FFI boundary. The extern procedure returns via a designated error path (union return type or error code). Implementations shall document how caught panics map to return values.

**Example 15.1.8.1 (Unwind behavior):**

```cursive
// Default: abort on panic
[[extern(C)]]
public procedure may_panic(x: i32): i32
    [[ panic |- true => true ]]
{
    if x < 0 {
        panic("negative value")  // Aborts process if called from C
    }
    result x * 2
}

// Explicit catch: return error code
[[extern(C), unwind(catch)]]
public procedure safe_ffi(x: i32): i32 \/ i32
    [[ panic |- true => match result { _: i32 => true, err: i32 => err == -1 } ]]
{
    if x < 0 {
        panic("negative")  // Caught; returns -1
    }
    result x * 2
}
```

[28] When `unwind(catch)` is specified, the return type should be a union accommodating both success values and panic-error indicators. The mapping from panic to error value is implementation-defined.

#### §14.1.9 Diagnostics

[29] [Note: Diagnostics defined in this subsection are cataloged in Annex E §E.5.1.14. — end note]

#### §15.1.10 Safety Obligations

[30] Programmers using FFI assume the following safety obligations:

##### §15.1.10.1 Pointer Validity

[31] **Obligation**: Pointers passed to or returned from foreign functions must be valid:

- Non-null (unless null is explicitly permitted by contract)
- Properly aligned for the pointed-to type
- Point to initialized memory (for reads)
- Point to live storage (not freed or out of scope)

[32] **Violation consequences**: Passing invalid pointers to foreign functions or dereferencing invalid pointers returned from foreign functions produces undefined behavior [UB-ID: B.2.01].

##### §15.1.10.2 Memory Ownership

[33] **Obligation**: Clarify memory ownership at FFI boundaries:

- Pointers passed to foreign functions: Foreign code shall not free memory unless contract specifies ownership transfer
- Pointers returned from foreign functions: Cursive code must free memory if contract specifies ownership transfer (via explicit free call or wrapper)

[34] Foreign functions returning allocated memory shall document ownership in postconditions:

```cursive
[[extern(C)]]
procedure malloc(size: usize): *mut u8
    [[
        ffi::call, unsafe::ptr
        |-
        size > 0
        =>
        result != null \/ result == null,
        result != null => caller_must_free(result)
    ]]
```

[35] The predicate `caller_must_free(result)` is documentation; the compiler cannot enforce it. Programmers must manually call `free(result)` before the pointer goes out of scope.

##### §15.1.10.3 Thread Safety

[36] **Obligation**: Foreign functions accessed from multiple Cursive threads must be thread-safe. The compiler cannot verify foreign code thread safety.

[37] Permissions are Cursive-specific and do not cross FFI boundaries. At the FFI boundary:

- Cursive `const` → C does not see permission (just pointer)
- Cursive `unique` → C does not see exclusivity guarantee
- Cursive `shared` → C does not see coordination requirement

[38] Programmers must ensure foreign code respects Cursive semantics documented in contracts. If a foreign function is documented as thread-safe, Cursive code may call it concurrently; otherwise, Cursive code must serialize access.

##### §15.1.10.4 Panic Safety

[39] **Obligation**: Extern procedures called from foreign code should avoid panicking unless `unwind(catch)` is specified and the return type accommodates error indicators.

[40] Default `unwind(abort)` provides safety (never corrupts foreign stack) but poor debuggability (aborts process). Use `unwind(catch)` for recoverable errors when foreign code can handle error returns.

#### §15.1.11 Examples

**Example 15.1.11.1 (Complete FFI import):**

```cursive
// Import C standard library functions

[[extern(C)]]
procedure strlen(s: *const u8): usize
    [[
        ffi::call, unsafe::ptr
        |-
        s != null, is_null_terminated(s)
        =>
        result >= 0
    ]]

[[extern(C)]]
procedure memcpy(dest: *mut u8, src: *const u8, n: usize): *mut u8
    [[
        ffi::call, unsafe::ptr
        |-
        dest != null,
        src != null,
        n > 0,
        no_overlap(dest, src, n)
        =>
        result == dest
    ]]

procedure safe_strlen(s: string@View): usize
    [[ ffi::call, unsafe::ptr |- s.len() > 0 => result == s.len() ]]
{
    unsafe {
        strlen(s.as_c_ptr())
    }
}
```

**Example 15.1.11.2 (Cursive export to C):**

```cursive
[[extern(C)]]
public procedure add_numbers(a: i32, b: i32): i32
    [[ |- true => result == a + b ]]
{
    result a + b
}

[[extern(C), no_mangle]]
public procedure multiply(x: i32, y: i32): i32
    [[ |- true => true ]]
{
    result x * y
}
// Exported symbol name: exactly "multiply" (no mangling)
```

**Example 15.1.11.3 - invalid (Non-FFI-safe type):**

```cursive
[[extern(C)]]
procedure bad_signature(s: string@View): i32
    [[ ffi::call |- true => true ]]
{
    result s.len() as i32
}
// error[E14-002]: string@View is not FFI-safe
// Use *const u8 with [[null_terminated]] instead
```

**Example 15.1.11.4 - invalid (Permission in signature):**

```cursive
[[extern(C)]]
procedure invalid(data: const Buffer): ()
    [[ ffi::call |- true => true ]]
{
    // error[E14-006]: permission qualifier 'const' in extern signature
}
```

**Example 15.1.11.5 - invalid (Variadic not supported):**

```cursive
[[extern(C)]]
procedure printf(format: *const u8, ...): i32
    [[ ffi::call, unsafe::ptr |- format != null => true ]]
// error[E14-005]: variadic parameters not supported
// Wrap in C helper: printf_int, printf_str, etc.
```

#### §14.1.12 Conformance Requirements

[41] Implementations shall generate correct calling convention code per §14.6 and document safety obligations for FFI users.

---

**Previous**: Clause 13 — Concurrency and Memory Ordering (§13.4 [concurrency.synchronization]) | **Next**: §14.2 FFI-Specific Unsafe Usage (§14.2 [interop.unsafe])
