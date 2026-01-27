# Spec-Test Coverage Gaps Report

This document identifies untested or under-tested rules from Cursive0.md, prioritized by severity.

## Executive Summary

| Priority | Category | Gap Count | Status |
|----------|----------|-----------|--------|
| ~~Critical~~ | Codegen/Layout | ~~16~~ 0 | ✅ COMPLETE |
| ~~Critical~~ | Codegen/ABI | ~~11~~ 0 | ✅ COMPLETE |
| ~~High~~ | Codegen/Lowering | ~~45~~ 0 | ✅ COMPLETE |
| ~~Medium~~ | Cleanup/Drop | ~~4~~ 0 | ✅ COMPLETE |
| Low | Dynamic Semantics | 3 | Interpreter-specific (N/A) |
| **TOTAL** | | **0** | **All gaps filled** |

### Implementation Status (2024-01)

All critical gaps have been addressed with 54 new test files:
- **16 Layout tests** in `codegen/layout/`
- **11 ABI tests** in `codegen/abi/`
- **23 Lowering tests** in `codegen/lowering/`
- **4 Cleanup tests** in `run-fail/`

## Critical Gaps (P0) - Layout Rules

These rules define memory layout and are essential for FFI correctness, memory safety, and ABI compatibility.

### Layout-Ptr (No test)
- **Rule**: `Layout-Ptr` (§6.1)
- **Specification**: Safe pointer layout: `sizeof = PtrSize, alignof = PtrAlign`
- **Required Test**: `codegen/layout/pointer-layout.cursive`
- **CHECK directive**: `// CHECK: %Ptr = type { ptr }` or sizeof verification

### Layout-RawPtr (No test)
- **Rule**: `Layout-RawPtr` (§6.1)
- **Specification**: Raw pointer layout: `sizeof = PtrSize, alignof = PtrAlign`
- **Required Test**: `codegen/layout/raw-pointer-layout.cursive`

### Layout-Func (No test)
- **Rule**: `Layout-Func` (§6.1)
- **Specification**: Function pointer layout
- **Required Test**: `codegen/layout/function-pointer-layout.cursive`

### Layout-Union-Niche (No test)
- **Rule**: `Layout-Union-Niche` (§6.1.4)
- **Specification**: Niche optimization for single-payload unions
- **Required Test**: `codegen/layout/union-niche.cursive`
- **Example**: `i32 | ()` should use niche in i32

### Layout-Union-Tagged (No test)
- **Rule**: `Layout-Union-Tagged` (§6.1.4)
- **Specification**: Tagged union with discriminant
- **Required Test**: `codegen/layout/union-tagged.cursive`
- **Verification**: discriminant offset, payload offset, alignment

### Layout-String-* (No tests)
- **Rules**: `Layout-String-Managed`, `Layout-String-View` (§6.1)
- **Specification**: String layout (ptr, len, cap for managed; ptr, len for view)
- **Required Test**: `codegen/layout/string-layout.cursive`

### Layout-Bytes-* (No tests)
- **Rules**: `Layout-Bytes-Managed`, `Layout-Bytes-View` (§6.1)
- **Specification**: Bytes layout
- **Required Test**: `codegen/layout/bytes-layout.cursive`

### Layout-Tuple (No test)
- **Rule**: `Layout-Tuple` (§6.1)
- **Specification**: Tuple field offsets with padding
- **Required Test**: `codegen/layout/tuple-layout.cursive`

### Layout-Array (No test)
- **Rule**: `Layout-Array` (§6.1)
- **Specification**: Array element stride
- **Required Test**: `codegen/layout/array-layout.cursive`

### Layout-Slice (No test)
- **Rule**: `Layout-Slice` (§6.1)
- **Specification**: Fat pointer (ptr + len)
- **Required Test**: `codegen/layout/slice-layout.cursive`

### Layout-Range (No test)
- **Rule**: `Layout-Range` (§6.1)
- **Specification**: Range struct (start, end)
- **Required Test**: `codegen/layout/range-layout.cursive`

### Layout-Enum-* (No tests)
- **Rules**: `Layout-Enum-Tagged`, `Layout-Enum` (§6.1.4)
- **Specification**: Enum discriminant and payload layout
- **Required Test**: `codegen/layout/enum-layout.cursive`

### Layout-Modal-* (No tests)
- **Rules**: `Layout-Modal-Niche`, `Layout-Modal-Tagged`, `Layout-Modal`, `Layout-ModalState` (§6.1)
- **Specification**: Modal type layout with state discrimination
- **Required Test**: `codegen/layout/modal-layout.cursive`

### Layout-DynamicClass (No test)
- **Rule**: `Layout-DynamicClass` (§6.1)
- **Specification**: Dynamic class type layout (vtable pointer)
- **Required Test**: `codegen/layout/dynamic-class-layout.cursive`

## Critical Gaps (P0) - ABI Rules

These rules define the calling convention and are essential for FFI interoperability.

### ABI-Param-ByValue-Move (No test)
- **Rule**: `ABI-Param-ByValue-Move` (§6.2)
- **Specification**: Move parameter passed by value
- **Required Test**: `codegen/abi/param-byvalue-move.cursive`

### ABI-Param-ByRef-Move (No test)
- **Rule**: `ABI-Param-ByRef-Move` (§6.2)
- **Specification**: Move parameter passed by reference for large types
- **Required Test**: `codegen/abi/param-byref-move.cursive`

### ABI-Ret-ByValue (No test)
- **Rule**: `ABI-Ret-ByValue` (§6.2)
- **Specification**: Return value in register
- **Required Test**: `codegen/abi/ret-byvalue.cursive`

### ABI-Ret-ByRef (No test)
- **Rule**: `ABI-Ret-ByRef` (§6.2)
- **Specification**: Sret convention for large return types
- **Required Test**: `codegen/abi/ret-byref.cursive`

### ABI-Record/Tuple/Array (No tests)
- **Rules**: `ABI-Record`, `ABI-Tuple`, `ABI-Array` (§6.2)
- **Specification**: Aggregate type passing conventions
- **Required Test**: `codegen/abi/aggregate-passing.cursive`

### ABI-Slice (No test)
- **Rule**: `ABI-Slice` (§6.2)
- **Specification**: Fat pointer passing
- **Required Test**: `codegen/abi/slice-passing.cursive`

### ABI-Union/Enum/Modal (No tests)
- **Rules**: `ABI-Union`, `ABI-Enum`, `ABI-Modal` (§6.2)
- **Specification**: Sum type passing conventions
- **Required Test**: `codegen/abi/sum-type-passing.cursive`

### ABI-Dynamic (No test)
- **Rule**: `ABI-Dynamic` (§6.2)
- **Specification**: Dynamic class parameter passing
- **Required Test**: `codegen/abi/dynamic-passing.cursive`

### ABI-StringBytes (No test)
- **Rule**: `ABI-StringBytes` (§6.2)
- **Specification**: String/bytes parameter passing
- **Required Test**: `codegen/abi/string-bytes-passing.cursive`

## High Priority Gaps (P1) - Lowering Rules

These rules define IR generation and affect correctness of compiled code.

### Lower-Expr-* (Expression lowering)
Missing IR verification for:
- Tuple construction and access
- Array construction and indexing
- Record construction and field access
- Enum construction and matching
- Binary and unary operations
- Cast and transmute
- Union propagation
- Control flow (if, match, loop)
- Block scope
- Move semantics
- Address-of and dereference
- Region allocation

**Recommendation**: Create `codegen/lowering/` directory with IR verification tests.

### Lower-Stmt-* (Statement lowering)
Missing IR verification for:
- Let/var binding allocation
- Assignment lowering
- Return/break/continue
- Defer registration
- Region/frame statements

### Lower-Place-* (Place expression lowering)
Missing IR verification for:
- Field access GEP generation
- Tuple element access
- Array index with bounds check
- Pointer dereference

## Medium Priority Gaps (P2) - Cleanup Rules

### Cleanup-Step-Drop-Panic (No test)
- **Specification**: When drop panics during cleanup, abort
- **Required Test**: `run-fail/cleanup-drop-panic.cursive`

### Cleanup-Step-Defer-Panic (No test)
- **Specification**: When defer panics during cleanup, continue then abort
- **Required Test**: `run-fail/cleanup-defer-panic.cursive`

### Cleanup-Cons-Drop-Panic / Cleanup-Cons-Defer-Panic
- **Specification**: Panic propagation through cleanup list
- **Required Tests**: `run-fail/double-panic-*.cursive`

## Low Priority Gaps (P3) - Dynamic Semantics

### EvalSigma-* (Interpreter rules)
Most EvalSigma rules are interpreter-specific and not directly testable in compiled mode. However, they define observable behavior that should be verified:

- EvalSigma-Propagate-Error - Union propagation early return
- EvalSigma-Match - Match arm selection
- EvalSigma-Loop-* - Loop iteration semantics

These are largely covered by run-pass tests but lack explicit specification compliance markers.

## Recommended Test Creation Order

### Phase 1: Layout Verification (16 tests)
1. `codegen/layout/pointer-layout.cursive`
2. `codegen/layout/tuple-layout.cursive`
3. `codegen/layout/array-layout.cursive`
4. `codegen/layout/slice-layout.cursive`
5. `codegen/layout/union-niche.cursive`
6. `codegen/layout/union-tagged.cursive`
7. `codegen/layout/string-layout.cursive`
8. `codegen/layout/bytes-layout.cursive`
9. `codegen/layout/range-layout.cursive`
10. `codegen/layout/enum-layout.cursive`
11. `codegen/layout/modal-layout.cursive`
12. `codegen/layout/dynamic-class-layout.cursive`
13. `codegen/layout/function-pointer-layout.cursive`
14. `codegen/layout/raw-pointer-layout.cursive`
15. `codegen/layout/type-alias-layout.cursive`
16. `codegen/layout/modal-state-layout.cursive`

### Phase 2: ABI Verification (11 tests)
1. `codegen/abi/param-byvalue-move.cursive`
2. `codegen/abi/param-byref-move.cursive`
3. `codegen/abi/ret-byvalue.cursive`
4. `codegen/abi/ret-byref-sret.cursive`
5. `codegen/abi/aggregate-passing.cursive`
6. `codegen/abi/slice-passing.cursive`
7. `codegen/abi/union-passing.cursive`
8. `codegen/abi/enum-passing.cursive`
9. `codegen/abi/modal-passing.cursive`
10. `codegen/abi/dynamic-passing.cursive`
11. `codegen/abi/string-bytes-passing.cursive`

### Phase 3: Lowering Verification (20+ tests)
Create `codegen/lowering/` directory with:
- expr-tuple.cursive
- expr-array.cursive
- expr-record.cursive
- expr-enum.cursive
- expr-binary-ops.cursive
- expr-unary-ops.cursive
- expr-cast.cursive
- expr-transmute.cursive
- expr-propagate.cursive
- expr-if.cursive
- expr-match.cursive
- expr-loop.cursive
- expr-block.cursive
- expr-move.cursive
- expr-addressof.cursive
- expr-deref.cursive
- expr-alloc.cursive
- stmt-binding.cursive
- stmt-assign.cursive
- stmt-defer.cursive

### Phase 4: Cleanup Edge Cases (4 tests)
1. `run-fail/cleanup-drop-panic.cursive`
2. `run-fail/cleanup-defer-panic.cursive`
3. `run-fail/double-panic-drop.cursive`
4. `run-fail/double-panic-defer.cursive`

## Test Template for Layout Verification

```cursive
// SPEC: 6.1.x Layout-TypeName
// CHECK-LABEL: @test_typename_layout
// CHECK: sizeof(TypeName) = N
// CHECK: alignof(TypeName) = M

public procedure main(move ctx: Context) -> i32 {
    // Verify layout via size/align intrinsics or IR inspection
    return 0
}
```

## Test Template for ABI Verification

```cursive
// SPEC: 6.2.x ABI-ParamMode
// CHECK-LABEL: define {{.*}} @test_abi_param
// CHECK: {{(byval|sret|ptr)}}

extern "C" {
    procedure foreign_call(x: TypeName) -> RetType
}

public procedure main(move ctx: Context) -> i32 {
    // Call that exercises the ABI rule
    return 0
}
```

## Verification Methodology

For codegen tests, use FileCheck-style directives:
- `// CHECK:` - Verify IR contains pattern
- `// CHECK-LABEL:` - Mark function boundaries
- `// CHECK-NOT:` - Verify IR does NOT contain pattern
- `// CHECK-DAG:` - Order-independent check

Tests should emit IR with `--emit-ir ll` and verify against expected patterns.
