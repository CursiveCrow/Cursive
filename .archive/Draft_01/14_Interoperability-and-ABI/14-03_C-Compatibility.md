# Cursive Language Specification

## Clause 14 — Interoperability and ABI

**Part**: XIV — Interoperability and ABI  
**File**: 15-3_C-Compatibility.md  
**Section**: §14.3 C Compatibility  
**Stable label**: [interop.c]  
**Forward references**: §15.1 [interop.ffi], §15.6 [interop.abi], Clause 5 §5.5 [decl.type], Clause 7 §7.3 [type.composite], Clause 11 §11.6 [memory.layout]

---

### §14.3 C Compatibility [interop.c]

#### §14.3.1 Overview

[1] C compatibility enables Cursive programs to interoperate with C libraries and system APIs. This subclause specifies type representation compatibility through the `repr(C)` attribute, string conversion mechanisms, function pointer compatibility, and the restrictions inherent in C interoperability.

[2] C compatibility is achieved through explicit opt-in: types must be marked `[[repr(C)]]` to guarantee C-compatible layout. This ensures Cursive's native layout optimizations do not break FFI while maintaining zero-cost abstractions for non-FFI code.

#### §14.3.2 Type Representation Compatibility

##### §14.3.2.1 The repr(C) Attribute

[3] The `[[repr(C)]]` attribute instructs the compiler to lay out a type using C-compatible rules, matching the pattern:

```
"[[" "repr" "(" "C" ")" "]]"
```

[ Note: See §1.4.3 [intro.attributes] for the general attribute grammar.
— end note ]

[4] repr(C) may be applied to records and enums. It guarantees the type's memory layout matches what a C compiler would produce on the target platform.

**Example 15.3.2.1 (repr(C) record):**

```cursive
[[repr(C)]]
public record Point {
    x: f32,
    y: f32,
}

// Guaranteed C-compatible:
// struct Point {
//     float x;
//     float y;
// };
```

##### §14.3.2.2 repr(C) Padding Rules

[5] Records with `[[repr(C)]]` follow platform C compiler padding rules:

- Fields laid out in declaration order
- Padding inserted between fields to satisfy alignment
- Tail padding ensures struct size is multiple of alignment
- Padding bytes have indeterminate values

[6] **Automatic padding** (Decision D15): The compiler automatically inserts padding matching the platform's C compiler. Programmers need not calculate padding manually.

[7] **Manual padding** (optional): For complete control, programmers may insert explicit padding fields:

**Example 15.3.2.2 (Explicit padding fields):**

```cursive
[[repr(C)]]
record ExplicitPadding {
    a: u8,
    _pad1: [u8; 3],    // Explicit padding (3 bytes)
    b: i32,
    c: u8,
    _pad2: [u8; 3],    // Explicit tail padding
}
// Total size: 12 bytes, alignment: 4
```

[8] Explicit padding fields provide documentation and ensure exact layout matching legacy C structures with unusual padding.

##### §14.3.2.3 repr(C) Enum Representation

[9] Enums with `[[repr(C)]]` require explicit discriminant type specification:

```cursive
[[repr(C, tag_type = "u32")]]
enum Status {
    Idle = 0,
    Running = 1,
    Complete = 2,
}

// C equivalent:
// enum Status {
//     STATUS_IDLE = 0,
//     STATUS_RUNNING = 1,
//     STATUS_COMPLETE = 2,
// };
```

[10] The `tag_type` specifies the discriminant representation. Valid tag types: `u8`, `u16`, `u32`, `u64`, `i8`, `i16`, `i32`, `i64`.

[11] Enums without explicit discriminant values are assigned sequentially from 0. Explicit values may be assigned using `= N` syntax.

[ Rationale: Tuple-records are not FFI-safe because C does not have a direct equivalent and their layout, even with `repr(C)`, is not guaranteed to be compatible across different C compilers and platforms. Standard `record` types with `repr(C)` provide the necessary layout stability for reliable FFI. — end rationale ]

##### §14.3.2.4 repr(C) Restrictions

[12] Types marked `[[repr(C)]]` shall satisfy:

(12.1) All fields have FFI-safe types (primitives, raw pointers, other repr(C) types)

(12.2) No fields with permissions (`const`, `unique`, `shared`) - use raw types instead

(12.3) No modal states - C has no state machine representation

(12.4) No union types (`T \/ U`) - use repr(C) enum with explicit variants instead

[13] Violations produce diagnostic E15-030 identifying the non-C-compatible field.

#### §14.3.3 String Conversion

##### §14.3.3.1 Null-Terminated String Attribute

[14] C strings are null-terminated byte arrays. The `[[null_terminated]]` attribute documents this requirement:

```cursive
[[extern(C)]]
procedure c_strlen(s: *const u8 [[null_terminated]]): usize
    [[
        ffi::call, unsafe::ptr
        |-
        s != null, is_null_terminated(s)
        =>
        result >= 0
    ]]
```

[15] The attribute is documentation; the compiler does not insert null terminators automatically. Programmers must ensure passed strings are null-terminated.

##### §14.3.3.2 String Conversion Methods

[16] Converting between Cursive strings and C strings requires explicit methods (standard library, not language built-ins):

**Cursive → C** (Decision D3):

```cursive
let cursive_str: string@View = "hello"

unsafe {
    let c_ptr: *const u8 = cursive_str.as_c_ptr()
    // Returns pointer to null-terminated UTF-8 data
    // Valid for cursive_str's lifetime

    c_function(c_ptr)
}
```

[17] The `as_c_ptr()` method (standard library) ensures null termination and returns a raw pointer. The pointer is valid while the Cursive string lives.

**C → Cursive**:

```cursive
[[extern(C)]]
procedure c_get_string(): *const u8 [[null_terminated]]
    [[ ffi::call, unsafe::ptr |- true => result != null ]]

procedure convert_from_c(): string@Managed
    [[ ffi::call, unsafe::ptr, alloc::heap |- true => true ]]
{
    let c_str: *const u8 = unsafe { c_get_string() }

    // Convert to Cursive string (copies data)
    let cursive_str: string@Managed = unsafe {
        string::from_c_str(c_str)  // Validates UTF-8, copies, adds null check
    }

    result cursive_str
}
```

[18] The `string::from_c_str()` function (standard library) validates UTF-8, copies the null-terminated data, and produces a Cursive `string@Managed`. Invalid UTF-8 produces an error (panic or error return, depending on method variant).

##### §14.3.3.3 String Lifetime Obligations

[19] **Obligation**: Pointers from `as_c_ptr()` are valid only while the source Cursive string lives:

```cursive
let ptr: *const u8 = unsafe {
    let temp: string@View = "temporary"
    temp.as_c_ptr()
    // ⚠️ DANGER: temp destroyed, ptr is dangling!
}
```

[20] To pass strings with extended lifetime, use static strings or managed strings that outlive the FFI call:

```cursive
let managed: string@Managed = string::from("persistent")

unsafe {
    let ptr: *const u8 = managed.as_c_ptr()
    c_function(ptr)  // OK: managed outlives call
}
```

#### §14.3.4 Function Pointers and Callbacks

##### §14.3.4.1 Extern Procedure Pointers

[21] Only procedures declared with `[[extern(C)]]` can be used as C function pointers:

```cursive
[[extern(C)]]
procedure cursive_callback(value: i32): i32
    [[ |- value >= 0 => result >= value ]]
{
    result value + 1
}

[[extern(C)]]
procedure register_callback(cb: [[extern(C)]] procedure(i32): i32)
    [[ ffi::call |- cb != null => true ]]

procedure use_callback()
    [[ ffi::call |- true => true ]]
{
    unsafe {
        register_callback(cursive_callback)  // OK: both extern(C)
    }
}
```

[22] **Restriction** (Decision D4): Only procedures explicitly declared `[[extern(C)]]` can be passed as C function pointers. Regular Cursive procedures, including non-capturing closures, cannot be converted to C callbacks. Attempting conversion produces diagnostic E15-040.

##### §14.3.4.2 Function Pointer Types

[23] Function pointer types in FFI contexts use extern calling convention:

```cursive
type CCallback = [[extern(C)]] procedure(i32): i32

[[repr(C)]]
record Handler {
    callback: CCallback,
    context: *mut (),
}
```

[24] Function pointers without extern declarations are not FFI-compatible.

##### §14.3.4.3 Callback Safety Obligations

[25] **Obligation**: Cursive callbacks called from C code must not:

- Panic (unless using `unwind(catch)` and C code handles errors)
- Access Cursive-specific features (regions, witnesses, contracts) that C cannot understand
- Capture Cursive closures (C cannot manage captured environments)
- Return non-FFI-safe types

[26] Callbacks should be pure or have minimal effects. Complex Cursive operations may fail when invoked from foreign contexts without grant availability.

#### §14.3.5 Size and Alignment Compatibility

[27] repr(C) types guarantee size and alignment match C:

$$
\text{sizeof}_{\text{Cursive}}(\text{repr(C) } T) = \text{sizeof}_{\text{C}}(T)
$$

$$
\text{alignof}_{\text{Cursive}}(\text{repr(C) } T) = \text{alignof}_{\text{C}}(T)
$$

[28] Implementations shall ensure repr(C) types have identical layout to equivalent C structures compiled with the platform's C compiler.

**Example 15.3.5.1 (Platform C compatibility):**

```cursive
[[repr(C)]]
record sockaddr_in {
    sin_family: u16,
    sin_port: u16,
    sin_addr: u32,
    sin_zero: [u8; 8],
}

// Matches C: struct sockaddr_in from <netinet/in.h>
// Size: 16 bytes, Alignment: 4 bytes (on typical platforms)
```

#### §14.3.6 Constraints

[1] _repr(C) field types._ All fields in a repr(C) record shall have FFI-safe types. Non-FFI-safe fields produce diagnostic E15-030.

[2] _Discriminant specification._ repr(C) enums shall specify `tag_type`. Missing tag_type produces diagnostic E15-031.

[3] _Callback signature compatibility._ Function pointers in extern contexts shall use extern calling conventions. Non-extern function types produce diagnostic E15-040.

[4] _String conversion requirement._ Cursive strings shall not appear directly in extern signatures. Use `*const u8` with `[[null_terminated]]` attribute instead. Violations produce diagnostic E15-002 (non-FFI-safe type).

#### §14.3.7 Examples

**Example 15.3.7.1 (Complete C interop scenario):**

```cursive
// C header: example.h
// void process_data(const char *name, int32_t value);
// struct Point { float x; float y; };
// void draw_point(const struct Point *p);

// Cursive declarations:

[[repr(C)]]
public record Point {
    x: f32,
    y: f32,
}

[[extern(C)]]
procedure process_data(name: *const u8 [[null_terminated]], value: i32)
    [[ ffi::call, unsafe::ptr |- name != null => true ]]

[[extern(C)]]
procedure draw_point(p: *const Point)
    [[ ffi::call, unsafe::ptr |- p != null => true ]]

procedure use_c_library()
    [[ ffi::call, unsafe::ptr |- true => true ]]
{
    let name: string@View = "example"
    let point = Point { x: 10.0, y: 20.0 }

    unsafe {
        process_data(name.as_c_ptr(), 42)
        draw_point(&point as *const Point)
    }
}
```

**Example 15.3.7.2 - invalid (Non-repr(C) in FFI):**

```cursive
record RegularPoint {  // No repr(C)!
    x: f32,
    y: f32,
}

[[extern(C)]]
procedure bad_draw(p: *const RegularPoint)
    [[ ffi::call, unsafe::ptr |- p != null => true ]]
// error[E15-030]: RegularPoint is not repr(C)
```

#### §14.3.8 Conformance Requirements

[29] Implementations shall:

1. Support `[[repr(C)]]` attribute on records and enums
2. Generate C-compatible layouts matching platform C compiler
3. Follow platform C padding and alignment rules
4. Support explicit padding fields for exact layout control
5. Require `tag_type` specification for repr(C) enums
6. Enforce FFI-safe type restrictions for repr(C) fields
7. Provide `as_c_ptr()` and `from_c_str()` string conversion (standard library)
8. Document platform-specific C layout details (padding, alignment, sizes)
9. Emit diagnostics E15-030, E15-031, E15-040 for C compatibility violations

---

**Previous**: §15.2 FFI-Specific Unsafe Usage (§15.2 [interop.unsafe]) | **Next**: §15.4 Platform-Specific Features (§15.4 [interop.platform])
