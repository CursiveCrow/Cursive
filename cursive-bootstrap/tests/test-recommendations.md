# Spec-Test Recommendations

This document provides detailed test case skeletons for each identified coverage gap in the Cursive0 test suite.

## Overview

| Priority | Category | Gap Count | Estimated Effort |
|----------|----------|-----------|------------------|
| P0 Critical | Layout | 16 | High |
| P0 Critical | ABI | 11 | High |
| P1 High | Lowering | 45 | Medium |
| P2 Medium | Cleanup | 4 | Low |
| **Total** | | **76** | |

---

## Phase 1: Layout Tests (P0 Critical)

These tests verify memory layout correctness using IR inspection with `--emit-ir ll`.

### 1.1 Layout-Ptr (pointer-layout.cursive)

**Rule**: `Layout-Ptr` (§6.1)
**Specification**: Safe pointer layout: `sizeof = PtrSize, alignof = PtrAlign`

```cursive
// SPEC: 6.1 Layout-Ptr
// CHECK-LABEL: define {{.*}} @test_ptr_layout
// CHECK: %"Ptr<i32>" = type { ptr }

public procedure main(move ctx: Context) -> i32 {
    var x: i32 = 42
    let p: Ptr<i32>@Valid = &x
    return 0
}
```

**Test Mode**: codegen-check
**Path**: `codegen/layout/pointer-layout.cursive`
**Verification**: Confirm Ptr<T> is represented as `{ ptr }` in LLVM IR

---

### 1.2 Layout-RawPtr (raw-pointer-layout.cursive)

**Rule**: `Layout-RawPtr` (§6.1)
**Specification**: Raw pointer layout: `sizeof = PtrSize, alignof = PtrAlign`

```cursive
// SPEC: 6.1 Layout-RawPtr
// CHECK-LABEL: define {{.*}} @test_raw_ptr_layout
// CHECK: ptr

public procedure main(move ctx: Context) -> i32 {
    let rp: *imm i32 = null
    return 0
}
```

**Test Mode**: codegen-check
**Path**: `codegen/layout/raw-pointer-layout.cursive`
**Verification**: Confirm *imm T and *mut T are lowered to LLVM `ptr`

---

### 1.3 Layout-Func (function-pointer-layout.cursive)

**Rule**: `Layout-Func` (§6.1)
**Specification**: Function pointer layout

```cursive
// SPEC: 6.1 Layout-Func
// CHECK-LABEL: define {{.*}} @test_func_ptr_layout
// CHECK: ptr

procedure helper(x: i32) -> i32 {
    return x
}

public procedure main(move ctx: Context) -> i32 {
    let fp: (i32) -> i32 = helper
    return 0
}
```

**Test Mode**: codegen-check
**Path**: `codegen/layout/function-pointer-layout.cursive`
**Verification**: Confirm procedure types are lowered to function pointers

---

### 1.4 Layout-Tuple (tuple-layout.cursive)

**Rule**: `Layout-Tuple` (§6.1)
**Specification**: Tuple field offsets with padding

```cursive
// SPEC: 6.1 Layout-Tuple
// CHECK-LABEL: define {{.*}} @test_tuple_layout
// CHECK: %"(i8, i32, i8)" = type { i8, i32, i8 }

public procedure main(move ctx: Context) -> i32 {
    let t: (i8, i32, i8) = (1i8, 2i32, 3i8)
    return 0
}
```

**Test Mode**: codegen-check
**Path**: `codegen/layout/tuple-layout.cursive`
**Verification**: Confirm tuple elements are laid out with proper alignment padding

---

### 1.5 Layout-Array (array-layout.cursive)

**Rule**: `Layout-Array` (§6.1)
**Specification**: Array element stride

```cursive
// SPEC: 6.1 Layout-Array
// CHECK-LABEL: define {{.*}} @test_array_layout
// CHECK: [4 x i32]

public procedure main(move ctx: Context) -> i32 {
    let arr: [i32; 4] = [1, 2, 3, 4]
    return 0
}
```

**Test Mode**: codegen-check
**Path**: `codegen/layout/array-layout.cursive`
**Verification**: Confirm arrays are lowered to LLVM array types with correct element count

---

### 1.6 Layout-Slice (slice-layout.cursive)

**Rule**: `Layout-Slice` (§6.1)
**Specification**: Fat pointer (ptr + len)

```cursive
// SPEC: 6.1 Layout-Slice
// CHECK-LABEL: define {{.*}} @test_slice_layout
// CHECK: %"[i32]" = type { ptr, i64 }

procedure take_slice(s: const [i32]) -> i32 {
    return 0
}

public procedure main(move ctx: Context) -> i32 {
    let arr: [i32; 4] = [1, 2, 3, 4]
    let s: const [i32] = arr[0..4]
    return take_slice(s)
}
```

**Test Mode**: codegen-check
**Path**: `codegen/layout/slice-layout.cursive`
**Verification**: Confirm slice is a fat pointer with ptr and length

---

### 1.7 Layout-Range (range-layout.cursive)

**Rule**: `Layout-Range` (§6.1)
**Specification**: Range struct (start, end)

```cursive
// SPEC: 6.1 Layout-Range
// CHECK-LABEL: define {{.*}} @test_range_layout
// CHECK: %Range = type { i64, i64 }

public procedure main(move ctx: Context) -> i32 {
    let r: Range<i32> = 0..10
    return 0
}
```

**Test Mode**: codegen-check
**Path**: `codegen/layout/range-layout.cursive`
**Verification**: Confirm Range is laid out as (start, end) struct

---

### 1.8 Layout-Union-Niche (union-niche.cursive)

**Rule**: `Layout-Union-Niche` (§6.1.4)
**Specification**: Niche optimization for single-payload unions

```cursive
// SPEC: 6.1.4 Layout-Union-Niche
// CHECK-LABEL: define {{.*}} @test_union_niche
// CHECK-NOT: discriminant
// CHECK: i32

public procedure main(move ctx: Context) -> i32 {
    let u: i32 | () = 42
    return 0
}
```

**Test Mode**: codegen-check
**Path**: `codegen/layout/union-niche.cursive`
**Verification**: Confirm `i32 | ()` uses niche optimization (no separate discriminant)

---

### 1.9 Layout-Union-Tagged (union-tagged.cursive)

**Rule**: `Layout-Union-Tagged` (§6.1.4)
**Specification**: Tagged union with discriminant

```cursive
// SPEC: 6.1.4 Layout-Union-Tagged
// CHECK-LABEL: define {{.*}} @test_union_tagged
// CHECK: i8
// CHECK: i32
// CHECK: i64

public procedure main(move ctx: Context) -> i32 {
    let u: i32 | i64 = 42i32
    return 0
}
```

**Test Mode**: codegen-check
**Path**: `codegen/layout/union-tagged.cursive`
**Verification**: Confirm `i32 | i64` has discriminant + payload

---

### 1.10 Layout-String-Managed (string-layout.cursive)

**Rule**: `Layout-String-Managed` (§6.1)
**Specification**: String layout (ptr, len, cap for managed)

```cursive
// SPEC: 6.1 Layout-String-Managed
// CHECK-LABEL: define {{.*}} @test_string_managed
// CHECK: %"string@Managed" = type { ptr, i64, i64 }

public procedure main(move ctx: Context) -> i32 {
    return 0
}
```

**Test Mode**: codegen-check
**Path**: `codegen/layout/string-managed-layout.cursive`
**Verification**: Confirm managed string has ptr, len, capacity fields

---

### 1.11 Layout-String-View (string-view-layout.cursive)

**Rule**: `Layout-String-View` (§6.1)
**Specification**: String view layout (ptr, len)

```cursive
// SPEC: 6.1 Layout-String-View
// CHECK-LABEL: define {{.*}} @test_string_view
// CHECK: %"string@View" = type { ptr, i64 }

public procedure main(move ctx: Context) -> i32 {
    let s: string@View = "hello"
    return 0
}
```

**Test Mode**: codegen-check
**Path**: `codegen/layout/string-view-layout.cursive`
**Verification**: Confirm string view has ptr and len fields only

---

### 1.12 Layout-Bytes-Managed (bytes-managed-layout.cursive)

**Rule**: `Layout-Bytes-Managed` (§6.1)
**Specification**: Bytes layout (ptr, len, cap for managed)

```cursive
// SPEC: 6.1 Layout-Bytes-Managed
// CHECK-LABEL: define {{.*}} @test_bytes_managed
// CHECK: %"bytes@Managed" = type { ptr, i64, i64 }

public procedure main(move ctx: Context) -> i32 {
    return 0
}
```

**Test Mode**: codegen-check
**Path**: `codegen/layout/bytes-managed-layout.cursive`
**Verification**: Confirm managed bytes has ptr, len, capacity fields

---

### 1.13 Layout-Bytes-View (bytes-view-layout.cursive)

**Rule**: `Layout-Bytes-View` (§6.1)
**Specification**: Bytes view layout (ptr, len)

```cursive
// SPEC: 6.1 Layout-Bytes-View
// CHECK-LABEL: define {{.*}} @test_bytes_view
// CHECK: %"bytes@View" = type { ptr, i64 }

public procedure main(move ctx: Context) -> i32 {
    return 0
}
```

**Test Mode**: codegen-check
**Path**: `codegen/layout/bytes-view-layout.cursive`
**Verification**: Confirm bytes view has ptr and len fields only

---

### 1.14 Layout-Enum (enum-layout.cursive)

**Rule**: `Layout-Enum-Tagged` (§6.1.4)
**Specification**: Enum discriminant and payload layout

```cursive
// SPEC: 6.1.4 Layout-Enum-Tagged
// CHECK-LABEL: define {{.*}} @test_enum_layout
// CHECK: i8
// CHECK: i32

enum Color {
    Red
    Green
    Blue
    Custom(i32)
}

public procedure main(move ctx: Context) -> i32 {
    let c: Color = Color::Red
    return 0
}
```

**Test Mode**: codegen-check
**Path**: `codegen/layout/enum-layout.cursive`
**Verification**: Confirm enum has discriminant + largest variant payload

---

### 1.15 Layout-Modal (modal-layout.cursive)

**Rule**: `Layout-Modal-Tagged` (§6.1)
**Specification**: Modal type layout with state discrimination

```cursive
// SPEC: 6.1 Layout-Modal-Tagged
// CHECK-LABEL: define {{.*}} @test_modal_layout
// CHECK: i8

modal Connection {
    @Open { fd: i32 }
    @Closed { }
}

public procedure main(move ctx: Context) -> i32 {
    return 0
}
```

**Test Mode**: codegen-check
**Path**: `codegen/layout/modal-layout.cursive`
**Verification**: Confirm modal has state discriminant + state-specific fields

---

### 1.16 Layout-DynamicClass (dynamic-class-layout.cursive)

**Rule**: `Layout-DynamicClass` (§6.1)
**Specification**: Dynamic class type layout (vtable pointer)

```cursive
// SPEC: 6.1 Layout-DynamicClass
// CHECK-LABEL: define {{.*}} @test_dynamic_layout
// CHECK: ptr

class Drawable {
    procedure draw(~) -> ()
}

public procedure main(move ctx: Context) -> i32 {
    return 0
}
```

**Test Mode**: codegen-check
**Path**: `codegen/layout/dynamic-class-layout.cursive`
**Verification**: Confirm dynamic class contains vtable pointer

---

## Phase 2: ABI Tests (P0 Critical)

These tests verify calling convention correctness for FFI compatibility.

### 2.1 ABI-Param-ByValue-Move (param-byvalue-move.cursive)

**Rule**: `ABI-Param-ByValue-Move` (§6.2)
**Specification**: Move parameter passed by value for small types

```cursive
// SPEC: 6.2 ABI-Param-ByValue-Move
// CHECK-LABEL: define {{.*}} @take_small
// CHECK: i32 %

procedure take_small(move x: i32) -> i32 {
    return x
}

public procedure main(move ctx: Context) -> i32 {
    let v: i32 = 42
    return take_small(move v)
}
```

**Test Mode**: codegen-check
**Path**: `codegen/abi/param-byvalue-move.cursive`
**Verification**: Confirm small types passed by value in registers

---

### 2.2 ABI-Param-ByRef-Move (param-byref-move.cursive)

**Rule**: `ABI-Param-ByRef-Move` (§6.2)
**Specification**: Move parameter passed by reference for large types

```cursive
// SPEC: 6.2 ABI-Param-ByRef-Move
// CHECK-LABEL: define {{.*}} @take_large
// CHECK: ptr {{.*}}byval

record LargeRecord {
    a: i64
    b: i64
    c: i64
    d: i64
    e: i64
}

procedure take_large(move x: LargeRecord) -> i64 {
    return x.a
}

public procedure main(move ctx: Context) -> i32 {
    let r: LargeRecord = LargeRecord { a: 1, b: 2, c: 3, d: 4, e: 5 }
    let _ = take_large(move r)
    return 0
}
```

**Test Mode**: codegen-check
**Path**: `codegen/abi/param-byref-move.cursive`
**Verification**: Confirm large types passed by reference with byval attribute

---

### 2.3 ABI-Ret-ByValue (ret-byvalue.cursive)

**Rule**: `ABI-Ret-ByValue` (§6.2)
**Specification**: Return value in register for small types

```cursive
// SPEC: 6.2 ABI-Ret-ByValue
// CHECK-LABEL: define {{.*}} i32 @return_small
// CHECK: ret i32

procedure return_small() -> i32 {
    return 42
}

public procedure main(move ctx: Context) -> i32 {
    return return_small()
}
```

**Test Mode**: codegen-check
**Path**: `codegen/abi/ret-byvalue.cursive`
**Verification**: Confirm small return values in registers

---

### 2.4 ABI-Ret-ByRef (ret-byref-sret.cursive)

**Rule**: `ABI-Ret-ByRef` (§6.2)
**Specification**: Sret convention for large return types

```cursive
// SPEC: 6.2 ABI-Ret-ByRef
// CHECK-LABEL: define {{.*}} @return_large
// CHECK: sret

record LargeRecord {
    a: i64
    b: i64
    c: i64
    d: i64
    e: i64
}

procedure return_large() -> LargeRecord {
    return LargeRecord { a: 1, b: 2, c: 3, d: 4, e: 5 }
}

public procedure main(move ctx: Context) -> i32 {
    let r: LargeRecord = return_large()
    return 0
}
```

**Test Mode**: codegen-check
**Path**: `codegen/abi/ret-byref-sret.cursive`
**Verification**: Confirm sret attribute for large return types

---

### 2.5 ABI-Record (aggregate-record.cursive)

**Rule**: `ABI-Record` (§6.2)
**Specification**: Record passing conventions

```cursive
// SPEC: 6.2 ABI-Record
// CHECK-LABEL: define {{.*}} @pass_record

record Point {
    x: i32
    y: i32
}

procedure pass_record(p: const Point) -> i32 {
    return p.x
}

public procedure main(move ctx: Context) -> i32 {
    let pt: Point = Point { x: 10, y: 20 }
    return pass_record(pt)
}
```

**Test Mode**: codegen-check
**Path**: `codegen/abi/aggregate-record.cursive`
**Verification**: Confirm record parameter passing mode

---

### 2.6 ABI-Tuple (aggregate-tuple.cursive)

**Rule**: `ABI-Tuple` (§6.2)
**Specification**: Tuple passing conventions

```cursive
// SPEC: 6.2 ABI-Tuple
// CHECK-LABEL: define {{.*}} @pass_tuple

procedure pass_tuple(t: const (i32, i32)) -> i32 {
    return t.0
}

public procedure main(move ctx: Context) -> i32 {
    let t: (i32, i32) = (10, 20)
    return pass_tuple(t)
}
```

**Test Mode**: codegen-check
**Path**: `codegen/abi/aggregate-tuple.cursive`
**Verification**: Confirm tuple parameter passing mode

---

### 2.7 ABI-Slice (slice-passing.cursive)

**Rule**: `ABI-Slice` (§6.2)
**Specification**: Fat pointer passing

```cursive
// SPEC: 6.2 ABI-Slice
// CHECK-LABEL: define {{.*}} @pass_slice
// CHECK: ptr
// CHECK: i64

procedure pass_slice(s: const [i32]) -> i32 {
    return 0
}

public procedure main(move ctx: Context) -> i32 {
    let arr: [i32; 4] = [1, 2, 3, 4]
    let s: const [i32] = arr[0..4]
    return pass_slice(s)
}
```

**Test Mode**: codegen-check
**Path**: `codegen/abi/slice-passing.cursive`
**Verification**: Confirm slice passed as (ptr, len) pair

---

### 2.8 ABI-Union (union-passing.cursive)

**Rule**: `ABI-Union` (§6.2)
**Specification**: Union type passing conventions

```cursive
// SPEC: 6.2 ABI-Union
// CHECK-LABEL: define {{.*}} @pass_union

procedure pass_union(u: i32 | bool) -> i32 {
    return match u {
        v: i32 => { v }
        b: bool => { if b { 1 } else { 0 } }
    }
}

public procedure main(move ctx: Context) -> i32 {
    let u: i32 | bool = 42
    return pass_union(u)
}
```

**Test Mode**: codegen-check
**Path**: `codegen/abi/union-passing.cursive`
**Verification**: Confirm union passing with discriminant + payload

---

### 2.9 ABI-Enum (enum-passing.cursive)

**Rule**: `ABI-Enum` (§6.2)
**Specification**: Enum passing conventions

```cursive
// SPEC: 6.2 ABI-Enum
// CHECK-LABEL: define {{.*}} @pass_enum

enum Option {
    None
    Some(i32)
}

procedure pass_enum(opt: Option) -> i32 {
    return match opt {
        Option::None => { 0 }
        Option::Some(v) => { v }
    }
}

public procedure main(move ctx: Context) -> i32 {
    let o: Option = Option::Some(42)
    return pass_enum(o)
}
```

**Test Mode**: codegen-check
**Path**: `codegen/abi/enum-passing.cursive`
**Verification**: Confirm enum passing mode

---

### 2.10 ABI-Modal (modal-passing.cursive)

**Rule**: `ABI-Modal` (§6.2)
**Specification**: Modal passing conventions

```cursive
// SPEC: 6.2 ABI-Modal
// CHECK-LABEL: define {{.*}} @pass_modal

modal Status {
    @Active { code: i32 }
    @Inactive { }
}

procedure pass_modal(s: Status@Active) -> i32 {
    return s.code
}

public procedure main(move ctx: Context) -> i32 {
    return 0
}
```

**Test Mode**: codegen-check
**Path**: `codegen/abi/modal-passing.cursive`
**Verification**: Confirm modal parameter passing mode

---

### 2.11 ABI-Dynamic (dynamic-passing.cursive)

**Rule**: `ABI-Dynamic` (§6.2)
**Specification**: Dynamic class parameter passing

```cursive
// SPEC: 6.2 ABI-Dynamic
// CHECK-LABEL: define {{.*}} @pass_dynamic
// CHECK: ptr

class Printable {
    procedure print(~) -> ()
}

procedure pass_dynamic(p: $Printable) -> () {
    return
}

public procedure main(move ctx: Context) -> i32 {
    return 0
}
```

**Test Mode**: codegen-check
**Path**: `codegen/abi/dynamic-passing.cursive`
**Verification**: Confirm dynamic class passed via vtable pointer

---

## Phase 3: Lowering Tests (P1 High)

These tests verify IR generation for expressions and statements.

### 3.1 Lower-Expr-Tuple (expr-tuple.cursive)

**Rule**: `Lower-Expr-Tuple` (§6.4)
**Specification**: Tuple construction lowering

```cursive
// SPEC: 6.4 Lower-Expr-Tuple
// CHECK-LABEL: define {{.*}} @test_tuple_construct
// CHECK: insertvalue

public procedure main(move ctx: Context) -> i32 {
    let t: (i32, i32) = (1, 2)
    return t.0
}
```

**Test Mode**: codegen-check
**Path**: `codegen/lowering/expr-tuple.cursive`

---

### 3.2 Lower-Expr-TupleAccess (expr-tuple-access.cursive)

**Rule**: `Lower-Expr-TupleAccess` (§6.4)
**Specification**: Tuple element access lowering

```cursive
// SPEC: 6.4 Lower-Expr-TupleAccess
// CHECK-LABEL: define {{.*}} @test_tuple_access
// CHECK: extractvalue
// CHECK: getelementptr

public procedure main(move ctx: Context) -> i32 {
    let t: (i32, i32) = (1, 2)
    return t.1
}
```

**Test Mode**: codegen-check
**Path**: `codegen/lowering/expr-tuple-access.cursive`

---

### 3.3 Lower-Expr-Array (expr-array.cursive)

**Rule**: `Lower-Expr-Array` (§6.4)
**Specification**: Array construction lowering

```cursive
// SPEC: 6.4 Lower-Expr-Array
// CHECK-LABEL: define {{.*}} @test_array_construct
// CHECK: store

public procedure main(move ctx: Context) -> i32 {
    let arr: [i32; 3] = [1, 2, 3]
    return arr[0]
}
```

**Test Mode**: codegen-check
**Path**: `codegen/lowering/expr-array.cursive`

---

### 3.4 Lower-Expr-Index (expr-array-index.cursive)

**Rule**: `Lower-Expr-Index-Array` (§6.4)
**Specification**: Array index with bounds check

```cursive
// SPEC: 6.4 Lower-Expr-Index-Array
// CHECK-LABEL: define {{.*}} @test_array_index
// CHECK: icmp
// CHECK: getelementptr

public procedure main(move ctx: Context) -> i32 {
    let arr: [i32; 3] = [1, 2, 3]
    return arr[1]
}
```

**Test Mode**: codegen-check
**Path**: `codegen/lowering/expr-array-index.cursive`

---

### 3.5 Lower-Expr-Record (expr-record.cursive)

**Rule**: `Lower-Expr-Record` (§6.4)
**Specification**: Record construction lowering

```cursive
// SPEC: 6.4 Lower-Expr-Record
// CHECK-LABEL: define {{.*}} @test_record_construct
// CHECK: insertvalue

record Point {
    x: i32
    y: i32
}

public procedure main(move ctx: Context) -> i32 {
    let p: Point = Point { x: 1, y: 2 }
    return p.x
}
```

**Test Mode**: codegen-check
**Path**: `codegen/lowering/expr-record.cursive`

---

### 3.6 Lower-Expr-FieldAccess (expr-field-access.cursive)

**Rule**: `Lower-Expr-FieldAccess` (§6.4)
**Specification**: Field access GEP generation

```cursive
// SPEC: 6.4 Lower-Expr-FieldAccess
// CHECK-LABEL: define {{.*}} @test_field_access
// CHECK: getelementptr

record Point {
    x: i32
    y: i32
}

public procedure main(move ctx: Context) -> i32 {
    let p: Point = Point { x: 1, y: 2 }
    return p.y
}
```

**Test Mode**: codegen-check
**Path**: `codegen/lowering/expr-field-access.cursive`

---

### 3.7 Lower-Expr-Enum (expr-enum.cursive)

**Rule**: `Lower-Expr-Enum-*` (§6.4)
**Specification**: Enum construction lowering

```cursive
// SPEC: 6.4 Lower-Expr-Enum
// CHECK-LABEL: define {{.*}} @test_enum_construct
// CHECK: store

enum Color {
    Red
    Green
    Blue
}

public procedure main(move ctx: Context) -> i32 {
    let c: Color = Color::Green
    return 0
}
```

**Test Mode**: codegen-check
**Path**: `codegen/lowering/expr-enum.cursive`

---

### 3.8 Lower-Expr-Binary (expr-binary-ops.cursive)

**Rule**: `Lower-Expr-Binary` (§6.4)
**Specification**: Binary operation lowering

```cursive
// SPEC: 6.4 Lower-Expr-Binary
// CHECK-LABEL: define {{.*}} @test_binary_ops
// CHECK: add
// CHECK: sub
// CHECK: mul

public procedure main(move ctx: Context) -> i32 {
    let a: i32 = 10
    let b: i32 = 5
    let sum: i32 = a + b
    let diff: i32 = a - b
    let prod: i32 = a * b
    return sum
}
```

**Test Mode**: codegen-check
**Path**: `codegen/lowering/expr-binary-ops.cursive`

---

### 3.9 Lower-Expr-Unary (expr-unary-ops.cursive)

**Rule**: `Lower-Expr-Unary` (§6.4)
**Specification**: Unary operation lowering

```cursive
// SPEC: 6.4 Lower-Expr-Unary
// CHECK-LABEL: define {{.*}} @test_unary_ops
// CHECK: sub {{.*}} 0
// CHECK: xor

public procedure main(move ctx: Context) -> i32 {
    let a: i32 = 10
    let neg: i32 = -a
    let inv: bool = !true
    return neg
}
```

**Test Mode**: codegen-check
**Path**: `codegen/lowering/expr-unary-ops.cursive`

---

### 3.10 Lower-Expr-Cast (expr-cast.cursive)

**Rule**: `Lower-Expr-Cast` (§6.4)
**Specification**: Cast instruction lowering

```cursive
// SPEC: 6.4 Lower-Expr-Cast
// CHECK-LABEL: define {{.*}} @test_cast
// CHECK: sext
// CHECK: trunc

public procedure main(move ctx: Context) -> i32 {
    let a: i8 = 10i8
    let b: i32 = a as i32
    let c: i8 = b as i8
    return b
}
```

**Test Mode**: codegen-check
**Path**: `codegen/lowering/expr-cast.cursive`

---

### 3.11 Lower-Expr-Transmute (expr-transmute.cursive)

**Rule**: `Lower-Expr-Transmute` (§6.4)
**Specification**: Bitcast instruction lowering

```cursive
// SPEC: 6.4 Lower-Expr-Transmute
// CHECK-LABEL: define {{.*}} @test_transmute
// CHECK: bitcast

public procedure main(move ctx: Context) -> i32 {
    let a: i32 = 42
    let b: u32 = unsafe { transmute<i32, u32>(a) }
    return 0
}
```

**Test Mode**: codegen-check
**Path**: `codegen/lowering/expr-transmute.cursive`

---

### 3.12 Lower-Expr-Propagate (expr-propagate.cursive)

**Rule**: `Lower-Expr-Propagate-*` (§6.4)
**Specification**: Union propagation lowering

```cursive
// SPEC: 6.4 Lower-Expr-Propagate
// CHECK-LABEL: define {{.*}} @test_propagate
// CHECK: br

procedure may_fail() -> i32 | () {
    return 42
}

procedure caller() -> i32 | () {
    let v: i32 = may_fail()?
    return v
}

public procedure main(move ctx: Context) -> i32 {
    return match caller() {
        v: i32 => { v }
        _: () => { 0 }
    }
}
```

**Test Mode**: codegen-check
**Path**: `codegen/lowering/expr-propagate.cursive`

---

### 3.13 Lower-Expr-If (expr-if.cursive)

**Rule**: `Lower-Expr-If` (§6.4)
**Specification**: Branch instruction lowering

```cursive
// SPEC: 6.4 Lower-Expr-If
// CHECK-LABEL: define {{.*}} @test_if
// CHECK: br i1
// CHECK: phi

public procedure main(move ctx: Context) -> i32 {
    let cond: bool = true
    let result: i32 = if cond { 1 } else { 0 }
    return result
}
```

**Test Mode**: codegen-check
**Path**: `codegen/lowering/expr-if.cursive`

---

### 3.14 Lower-Expr-Match (expr-match.cursive)

**Rule**: `Lower-Expr-Match` (§6.4)
**Specification**: Switch/branch lowering

```cursive
// SPEC: 6.4 Lower-Expr-Match
// CHECK-LABEL: define {{.*}} @test_match
// CHECK: switch

enum Color {
    Red
    Green
    Blue
}

public procedure main(move ctx: Context) -> i32 {
    let c: Color = Color::Green
    return match c {
        Color::Red => { 1 }
        Color::Green => { 2 }
        Color::Blue => { 3 }
    }
}
```

**Test Mode**: codegen-check
**Path**: `codegen/lowering/expr-match.cursive`

---

### 3.15 Lower-Expr-Loop (expr-loop.cursive)

**Rule**: `Lower-Expr-Loop*` (§6.4)
**Specification**: Loop structure lowering

```cursive
// SPEC: 6.4 Lower-Expr-Loop
// CHECK-LABEL: define {{.*}} @test_loop
// CHECK: br label
// CHECK: br i1

public procedure main(move ctx: Context) -> i32 {
    var i: i32 = 0
    loop i < 10 {
        i = i + 1
    }
    return i
}
```

**Test Mode**: codegen-check
**Path**: `codegen/lowering/expr-loop.cursive`

---

### 3.16 Lower-Expr-Block (expr-block.cursive)

**Rule**: `Lower-Expr-Block` (§6.4)
**Specification**: Block scope lowering

```cursive
// SPEC: 6.4 Lower-Expr-Block
// CHECK-LABEL: define {{.*}} @test_block

public procedure main(move ctx: Context) -> i32 {
    let result: i32 = {
        let x: i32 = 1
        let y: i32 = 2
        x + y
    }
    return result
}
```

**Test Mode**: codegen-check
**Path**: `codegen/lowering/expr-block.cursive`

---

### 3.17 Lower-Expr-Move (expr-move.cursive)

**Rule**: `Lower-Expr-Move` (§6.4)
**Specification**: Move semantics lowering

```cursive
// SPEC: 6.4 Lower-Expr-Move
// CHECK-LABEL: define {{.*}} @test_move
// CHECK: memcpy

record Data {
    value: i32
}

procedure consume(move d: Data) -> i32 {
    return d.value
}

public procedure main(move ctx: Context) -> i32 {
    let d: Data = Data { value: 42 }
    return consume(move d)
}
```

**Test Mode**: codegen-check
**Path**: `codegen/lowering/expr-move.cursive`

---

### 3.18 Lower-Expr-AddressOf (expr-addressof.cursive)

**Rule**: `Lower-Expr-AddressOf` (§6.4)
**Specification**: Address computation lowering

```cursive
// SPEC: 6.4 Lower-Expr-AddressOf
// CHECK-LABEL: define {{.*}} @test_addressof
// CHECK: alloca
// CHECK: store

public procedure main(move ctx: Context) -> i32 {
    var x: i32 = 42
    let p: Ptr<i32>@Valid = &x
    return *p
}
```

**Test Mode**: codegen-check
**Path**: `codegen/lowering/expr-addressof.cursive`

---

### 3.19 Lower-Expr-Deref (expr-deref.cursive)

**Rule**: `Lower-Expr-Deref` (§6.4)
**Specification**: Load instruction lowering

```cursive
// SPEC: 6.4 Lower-Expr-Deref
// CHECK-LABEL: define {{.*}} @test_deref
// CHECK: load

public procedure main(move ctx: Context) -> i32 {
    var x: i32 = 42
    let p: Ptr<const i32>@Valid = &x
    return *p
}
```

**Test Mode**: codegen-check
**Path**: `codegen/lowering/expr-deref.cursive`

---

### 3.20 Lower-Expr-Alloc (expr-alloc.cursive)

**Rule**: `Lower-Expr-Alloc` (§6.4)
**Specification**: Region allocation lowering

```cursive
// SPEC: 6.4 Lower-Expr-Alloc
// CHECK-LABEL: define {{.*}} @test_alloc
// CHECK: call {{.*}}alloc

public procedure main(move ctx: Context) -> i32 {
    region {
        let p: Ptr<i32>@Valid = ^42
        return *p
    }
    return 0
}
```

**Test Mode**: codegen-check
**Path**: `codegen/lowering/expr-alloc.cursive`

---

### 3.21 Lower-Stmt-Binding (stmt-binding.cursive)

**Rule**: `Lower-Stmt-Let/Var` (§6.5)
**Specification**: Let/var binding allocation

```cursive
// SPEC: 6.5 Lower-Stmt-Binding
// CHECK-LABEL: define {{.*}} @test_binding
// CHECK: alloca
// CHECK: store

public procedure main(move ctx: Context) -> i32 {
    let x: i32 = 10
    var y: i32 = 20
    y = 30
    return x + y
}
```

**Test Mode**: codegen-check
**Path**: `codegen/lowering/stmt-binding.cursive`

---

### 3.22 Lower-Stmt-Assign (stmt-assign.cursive)

**Rule**: `Lower-Stmt-Assign` (§6.5)
**Specification**: Assignment lowering

```cursive
// SPEC: 6.5 Lower-Stmt-Assign
// CHECK-LABEL: define {{.*}} @test_assign
// CHECK: store

public procedure main(move ctx: Context) -> i32 {
    var x: i32 = 10
    x = 20
    x = x + 5
    return x
}
```

**Test Mode**: codegen-check
**Path**: `codegen/lowering/stmt-assign.cursive`

---

### 3.23 Lower-Stmt-Defer (stmt-defer.cursive)

**Rule**: `Lower-Stmt-Defer` (§6.5)
**Specification**: Defer registration lowering

```cursive
// SPEC: 6.5 Lower-Stmt-Defer
// CHECK-LABEL: define {{.*}} @test_defer
// CHECK: invoke
// CHECK: landingpad

public procedure main(move ctx: Context) -> i32 {
    var counter: i32 = 0
    {
        defer { counter = counter + 1 }
        counter = 10
    }
    return counter
}
```

**Test Mode**: codegen-check
**Path**: `codegen/lowering/stmt-defer.cursive`

---

## Phase 4: Cleanup Tests (P2 Medium)

These tests verify cleanup behavior during panic and error scenarios.

### 4.1 Cleanup-Step-Drop-Panic (cleanup-drop-panic.cursive)

**Rule**: `Cleanup-Step-Drop-Panic` (§7.4)
**Specification**: When drop panics during cleanup, abort

```cursive
// SPEC: 7.4 Cleanup-Step-Drop-Panic
// RUN: %cursive run %s 2>&1
// CHECK: abort

record PanicOnDrop {
    value: i32
}

// Note: This test requires Drop implementation that panics

public procedure main(move ctx: Context) -> i32 {
    return 0
}
```

**Test Mode**: run-fail
**Path**: `run-fail/cleanup-drop-panic.cursive`
**Expected**: Process aborts (double-panic)

---

### 4.2 Cleanup-Step-Defer-Panic (cleanup-defer-panic.cursive)

**Rule**: `Cleanup-Step-Defer-Panic` (§7.4)
**Specification**: When defer panics during cleanup, continue then abort

```cursive
// SPEC: 7.4 Cleanup-Step-Defer-Panic
// RUN: %cursive run %s 2>&1
// CHECK: abort

public procedure main(move ctx: Context) -> i32 {
    {
        defer { panic!("defer panic") }
        panic!("initial panic")
    }
    return 0
}
```

**Test Mode**: run-fail
**Path**: `run-fail/cleanup-defer-panic.cursive`
**Expected**: Process aborts after running all defers

---

### 4.3 Cleanup-Cons-Drop-Panic (double-panic-drop.cursive)

**Rule**: `Cleanup-Cons-Drop-Panic` (§7.4)
**Specification**: Drop panic propagation through cleanup list

```cursive
// SPEC: 7.4 Cleanup-Cons-Drop-Panic
// RUN: %cursive run %s 2>&1
// CHECK: abort

public procedure main(move ctx: Context) -> i32 {
    return 0
}
```

**Test Mode**: run-fail
**Path**: `run-fail/double-panic-drop.cursive`
**Expected**: Abort on second panic during drop cleanup

---

### 4.4 Cleanup-Cons-Defer-Panic (double-panic-defer.cursive)

**Rule**: `Cleanup-Cons-Defer-Panic` (§7.4)
**Specification**: Defer panic propagation through cleanup list

```cursive
// SPEC: 7.4 Cleanup-Cons-Defer-Panic
// RUN: %cursive run %s 2>&1
// CHECK: abort

public procedure main(move ctx: Context) -> i32 {
    defer { panic!("second") }
    defer { panic!("first") }
    return 0
}
```

**Test Mode**: run-fail
**Path**: `run-fail/double-panic-defer.cursive`
**Expected**: First defer executes, second causes abort

---

## Implementation Checklist

### Phase 1: Layout (16 tests) ✅ COMPLETE
- [x] codegen/layout/pointer-layout.cursive
- [x] codegen/layout/raw-pointer-layout.cursive
- [x] codegen/layout/function-pointer-layout.cursive
- [x] codegen/layout/tuple-layout.cursive
- [x] codegen/layout/array-layout.cursive
- [x] codegen/layout/slice-layout.cursive
- [x] codegen/layout/range-layout.cursive
- [x] codegen/layout/union-niche.cursive
- [x] codegen/layout/union-tagged.cursive
- [x] codegen/layout/string-managed-layout.cursive
- [x] codegen/layout/string-view-layout.cursive
- [x] codegen/layout/bytes-managed-layout.cursive
- [x] codegen/layout/bytes-view-layout.cursive
- [x] codegen/layout/enum-layout.cursive
- [x] codegen/layout/modal-layout.cursive
- [x] codegen/layout/dynamic-class-layout.cursive

### Phase 2: ABI (11 tests) ✅ COMPLETE
- [x] codegen/abi/param-byvalue-move.cursive
- [x] codegen/abi/param-byref-move.cursive
- [x] codegen/abi/ret-byvalue.cursive
- [x] codegen/abi/ret-byref-sret.cursive
- [x] codegen/abi/aggregate-record.cursive
- [x] codegen/abi/aggregate-tuple.cursive
- [x] codegen/abi/slice-passing.cursive
- [x] codegen/abi/union-passing.cursive
- [x] codegen/abi/enum-passing.cursive
- [x] codegen/abi/modal-passing.cursive
- [x] codegen/abi/dynamic-passing.cursive

### Phase 3: Lowering (23 tests) ✅ COMPLETE
- [x] codegen/lowering/expr-tuple.cursive
- [x] codegen/lowering/expr-tuple-access.cursive
- [x] codegen/lowering/expr-array.cursive
- [x] codegen/lowering/expr-array-index.cursive
- [x] codegen/lowering/expr-record.cursive
- [x] codegen/lowering/expr-field-access.cursive
- [x] codegen/lowering/expr-enum.cursive
- [x] codegen/lowering/expr-binary-ops.cursive
- [x] codegen/lowering/expr-unary-ops.cursive
- [x] codegen/lowering/expr-cast.cursive
- [x] codegen/lowering/expr-transmute.cursive
- [x] codegen/lowering/expr-propagate.cursive
- [x] codegen/lowering/expr-if.cursive
- [x] codegen/lowering/expr-match.cursive
- [x] codegen/lowering/expr-loop.cursive
- [x] codegen/lowering/expr-block.cursive
- [x] codegen/lowering/expr-move.cursive
- [x] codegen/lowering/expr-addressof.cursive
- [x] codegen/lowering/expr-deref.cursive
- [x] codegen/lowering/expr-alloc.cursive
- [x] codegen/lowering/stmt-binding.cursive
- [x] codegen/lowering/stmt-assign.cursive
- [x] codegen/lowering/stmt-defer.cursive

### Phase 4: Cleanup (4 tests) ✅ COMPLETE
- [x] run-fail/cleanup-drop-panic.cursive
- [x] run-fail/cleanup-defer-panic.cursive
- [x] run-fail/double-panic-drop.cursive
- [x] run-fail/double-panic-defer.cursive

---

## Test Harness Requirements

For codegen tests to work, the test harness needs:

1. **IR Emission**: `cursivec0 --emit-ir ll <file>` to produce LLVM IR
2. **FileCheck Integration**: Pattern matching against IR output
3. **Check Directives**:
   - `// CHECK:` - Pattern must appear
   - `// CHECK-LABEL:` - Function boundary marker
   - `// CHECK-NOT:` - Pattern must NOT appear
   - `// CHECK-DAG:` - Order-independent check

The existing test runner should be extended to support `codegen-check` mode that:
1. Compiles the test file with `--emit-ir ll`
2. Runs FileCheck-style verification against the output
3. Reports pass/fail based on CHECK directive matches

