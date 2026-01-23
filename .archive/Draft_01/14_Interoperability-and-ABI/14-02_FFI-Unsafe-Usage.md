# Cursive Language Specification

## Clause 14 — Interoperability and ABI

**Part**: XIV — Interoperability and ABI  
**File**: 15-2_FFI-Unsafe-Usage.md  
**Section**: §14.2 FFI-Specific Unsafe Usage  
**Stable label**: [interop.unsafe]  
**Forward references**: §15.1 [interop.ffi], §15.3 [interop.c], Clause 11 §11.8 [memory.unsafe], Clause 12 [contract]

---

### §14.2 FFI-Specific Unsafe Usage [interop.unsafe]

#### §14.2.1 Overview

[1] FFI operations require `unsafe` blocks (§11.8) because foreign code operates outside Cursive's safety guarantees. This subclause specifies the additional unsafe obligations specific to FFI contexts: raw pointer handling, memory lifetime management across language boundaries, and panic unwinding behavior.

[2] While Clause 11 §11.8 defines general unsafe block semantics, this subclause focuses on FFI-specific concerns where Cursive's type system and permission model interact with foreign memory management and calling conventions.

#### §14.2.2 Raw Pointers in FFI Context

##### §14.2.2.1 Pointer Validity Obligations

[3] When passing raw pointers to foreign functions or receiving them as return values, programmers must ensure:

**Passing pointers to foreign code**:
[4] Pointers passed as arguments must be valid for the duration of the foreign call:

```cursive
[[extern(C)]]
procedure c_process(data: *const Data): ()
    [[ ffi::call, unsafe::ptr |- data != null => true ]]

procedure use_ffi()
    [[ ffi::call, unsafe::ptr, alloc::heap |- true => true ]]
{
    let value = Data::new()

    unsafe {
        let ptr: *const Data = &value as *const Data
        c_process(ptr)  // OK: value lives for entire call
    }
    // value destroyed here; ptr would be invalid
}
```

[5] Passing pointers to stack-allocated or region-allocated values requires ensuring the pointed-to memory remains valid for the foreign call's duration.

**Receiving pointers from foreign code**:
[6] Pointers returned from foreign functions have unknown provenance and lifetime:

```cursive
[[extern(C)]]
procedure c_allocate(): *mut Buffer
    [[ ffi::call, unsafe::ptr |- true => result != null \/ result == null ]]

procedure use_foreign_alloc()
    [[ ffi::call, unsafe::ptr |- true => true ]]
{
    let ptr: *mut Buffer = unsafe {
        c_allocate()
    }

    if !ptr.is_null() {
        unsafe {
            (*ptr).use()
            // Programmer responsible for calling c_free(ptr) eventually
        }
    }
}
```

[7] Programmers must manually track foreign-allocated memory and call appropriate cleanup functions (e.g., `free`, foreign destructors).

##### §14.2.2.2 Pointer Alignment

[8] Foreign functions may have alignment requirements. Programmers must ensure passed pointers satisfy alignment constraints:

$$
\frac{\text{foreign func expects alignment } a \quad \text{pointer } p \text{ has alignment } a'}
     {a' \bmod a = 0 \text{ required}}
\tag{FFI-Align-Requirement}
$$

[9] Violating alignment requirements produces undefined behavior [UB-ID: B.2.10]. The compiler cannot verify foreign code alignment requirements; they must be documented in contracts.

**Example 15.2.2.1 (Alignment obligation):**

```cursive
[[extern(C)]]
procedure c_vectorized(data: *const f32): ()
    [[
        ffi::call, unsafe::ptr
        |-
        data != null,
        align_of(data) >= 16  // SSE alignment requirement
        =>
        true
    ]]

procedure call_vectorized(numbers: [f32; 8])
    [[ ffi::call, unsafe::ptr |- true => true ]]
{
    // Ensure array is 16-byte aligned for SSE
    unsafe {
        let aligned_ptr = numbers.as_ptr()  // May not be aligned!
        c_vectorized(aligned_ptr)  // UB if misaligned
    }
}
```

[10] The contract documents alignment requirement; the compiler cannot enforce it. Programmers must use aligned allocation or repr(align(N)) to guarantee alignment.

#### §14.2.3 Memory Lifetime Management

##### §14.2.3.1 Stack Pointers to Foreign Code

[11] Passing stack-allocated data to foreign functions is permitted provided the data remains live for the call duration:

```cursive
procedure pass_stack_value()
    [[ ffi::call, unsafe::ptr |- true => true ]]
{
    let local = 42

    unsafe {
        let ptr: *const i32 = &local as *const i32
        c_function(ptr)  // OK: local lives for call duration
    }
    // local destroyed here
}
```

[12] **Prohibition**: Foreign functions shall not store stack pointers for later use. If a foreign function stores a pointer, the pointed-to data must have static or heap lifetime. Violations produce undefined behavior [UB-ID: B.2.01].

##### §14.2.3.2 Foreign-Allocated Memory

[13] Memory allocated by foreign functions (e.g., C `malloc`) has unknown provenance from Cursive's perspective:

$$
\text{prov}(\text{foreign\_alloc}()) = \text{Foreign}
$$

[14] Foreign-allocated memory does not participate in Cursive's RAII system. Programmers must manually free such memory using appropriate foreign functions:

**Example 15.2.3.1 (Foreign memory management):**

```cursive
[[extern(C)]]
procedure malloc(size: usize): *mut u8
    [[ ffi::call, unsafe::ptr |- size > 0 => result != null \/ result == null ]]

[[extern(C)]]
procedure free(ptr: *mut u8)
    [[ ffi::call, unsafe::ptr |- ptr != null => true ]]

procedure use_c_memory()
    [[ ffi::call, unsafe::ptr |- true => true ]]
{
    let ptr: *mut u8 = unsafe { malloc(1024) }

    if !ptr.is_null() {
        unsafe {
            // Use memory...
            *ptr = 42
        }

        unsafe {
            free(ptr)  // Manual cleanup required
        }
    }
}
```

[15] Failing to free foreign-allocated memory produces memory leaks. Dereferencing freed foreign memory produces undefined behavior [UB-ID: B.2.01].

##### §14.2.3.3 Region-Allocated Data to Foreign Code

[16] **Prohibition**: Region-allocated Cursive values shall not be passed to foreign functions that store pointers for later access. Region lifetimes are Cursive-specific; foreign code cannot respect region destruction:

```cursive
[[extern(C)]]
procedure c_store_pointer(ptr: *const Data)
    [[ ffi::call, unsafe::ptr |- ptr != null => stores_for_later_access(ptr) ]]

procedure invalid_region_pass()
    [[ ffi::call, unsafe::ptr, alloc::region |- true => true ]]
{
    region temp {
        let data = ^Data::new()

        unsafe {
            let ptr: *const Data = &data as *const Data
            c_store_pointer(ptr)  // ⚠️ DANGER: C stores pointer to region memory
        }
    }
    // temp region destroyed - C now has dangling pointer!
}
```

[17] If the contract specifies the foreign function stores pointers, only pass heap-allocated or static data. The compiler cannot enforce this; it is a programmer obligation documented in safety notes.

#### §14.2.4 Panic and Unwinding

##### §14.2.4.1 Default Abort Behavior

[18] By default, panics in extern procedures called from foreign code abort the process:

$$
\frac{\text{[[extern(C)]] proc called from foreign} \quad \text{panic occurs} \quad \text{no unwind attr}}
     {\text{std::process::abort()}}
\tag{E-FFI-Panic-Abort}
$$

[19] Abort prevents unwinding through foreign stack frames, which could corrupt foreign program state. The process terminates immediately without running destructors for foreign-held resources.

##### §14.2.4.2 Catch Behavior

[20] When `[[unwind(catch)]]` is specified, panics are caught at the FFI boundary:

$$
\frac{\text{[[extern(C), unwind(catch)]] proc} \quad \text{panic occurs}}
     {\text{return error value (implementation-defined mapping)}}
\tag{E-FFI-Panic-Catch}
$$

[21] The caught panic shall not propagate into foreign code. The implementation converts the panic into a return value:

- For procedures returning `T \/ ErrorCode`, return the error variant
- For procedures returning primitive integers, return a designated error value (e.g., `-1`, `0`)
- For procedures returning pointers, return null

[22] The exact panic-to-error mapping is implementation-defined and shall be documented. Programmers should use explicit union return types to make error cases clear:

**Example 15.2.4.1 (Catch with explicit error type):**

```cursive
[[extern(C), unwind(catch)]]
public procedure safe_divide(a: i32, b: i32): i32 \/ i32
    [[
        panic
        |-
        true
        =>
        match result {
            quotient: i32 => b != 0,
            error: i32 => error == -1 && b == 0
        }
    ]]
{
    if b == 0 {
        panic("division by zero")  // Caught; returns -1
    }
    result a / b
}
```

#### §14.2.5 Null Pointer Handling

[23] Foreign functions may return null pointers to indicate errors or absence. Cursive provides null checking via raw pointer methods:

```cursive
let ptr: *mut u8 = unsafe { c_allocate() }

if ptr.is_null() {
    // Handle allocation failure
} else {
    unsafe {
        *ptr = 42  // Safe: checked non-null
    }
}
```

[24] **Null dereference**: Dereferencing null pointers produces undefined behavior [UB-ID: B.2.01]. Programmers must check pointers before dereferencing.

[25] Contracts may document null return conditions:

```cursive
[[extern(C)]]
procedure maybe_allocate(size: usize): *mut u8
    [[
        ffi::call, unsafe::ptr
        |-
        size > 0
        =>
        result != null \/ (result == null && allocation_failed())
    ]]
```

#### §14.2.6 Calling Convention Mismatch

[26] **Obligation**: Extern procedure calling conventions must match foreign function expectations. Mismatched calling conventions produce undefined behavior [UB-ID: B.2.53].

```cursive
// C function uses stdcall convention on Windows
[[extern(stdcall)]]  // MUST match C declaration
procedure WindowsAPI(hwnd: *mut (),  message: u32): i32
    [[ ffi::call, unsafe::ptr |- true => true ]]
```

[27] The compiler generates code for the specified calling convention but cannot verify the foreign function actually uses that convention. Programmers must ensure correctness.

#### §14.2.7 Data Races Across FFI

[28] **Obligation**: Foreign functions accessed from multiple threads must be thread-safe. Cursive's permission system does not apply to foreign code:

```cursive
[[extern(C)]]
procedure c_global_counter(): i32
    [[ ffi::call |- true => true ]]

// If c_global_counter uses global state without synchronization:
// Calling from multiple Cursive threads produces data race
```

[29] Thread safety is a contractual obligation documented in sequents. The compiler cannot verify foreign thread safety.

#### §14.2.8 Diagnostics

[30] FFI unsafe usage diagnostics:

| Code    | Condition                                   | Section |
| ------- | ------------------------------------------- | ------- |
| E15-020 | Dereference of potentially null FFI pointer | [24]    |
| E15-021 | FFI call with mismatched calling convention | [26]    |
| E15-022 | Region pointer passed to storing FFI        | [17]    |

Diagnostics E15-020 through E15-022 are quality-of-implementation warnings; violations produce UB but may not be detectable statically.

#### §14.2.9 Conformance Requirements

[31] Implementations shall:

1. Require unsafe blocks for all FFI pointer operations
2. Document panic behavior (abort vs catch) clearly
3. Implement `unwind(abort)` and `unwind(catch)` per §15.2.4
4. Generate calling convention code matching [[extern(...)]] specification
5. Document safety obligations for FFI users in implementation guide
6. Warn about common FFI pitfalls (null derefs, lifetime issues) as quality-of-implementation feature
7. Preserve Cursive panic messages when catching (for debugging)

---

**Previous**: §15.1 FFI Declarations and Safety Obligations (§15.1 [interop.ffi]) | **Next**: §15.3 C Compatibility (§15.3 [interop.c])
