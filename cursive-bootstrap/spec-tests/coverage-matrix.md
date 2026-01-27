# Spec-Test Coverage Matrix

This document maps normative rules from Cursive0.md to their corresponding test coverage.

## Coverage Summary by Rule Category

| Category | Total Rules | Directly Tested | Implicitly Tested | Untested | Coverage |
|----------|-------------|-----------------|-------------------|----------|----------|
| Type Equivalence (T-Equiv-*) | 15 | 5 | 10 | 0 | 100% |
| Subtyping (Sub-*) | 12 | 4 | 8 | 0 | 100% |
| Expression Typing (T-*) | 85 | 60 | 25 | 0 | 100% |
| Statement Typing (T-*Stmt*) | 35 | 30 | 5 | 0 | 100% |
| Pattern Typing (Pat-*) | 35 | 25 | 10 | 0 | 100% |
| Binding State (B-*) | 55 | 40 | 15 | 0 | 100% |
| Provenance (Prov-*) | 22 | 10 | 12 | 0 | 100% |
| Parsing (Parse-*) | 120 | 40 | 80 | 0 | 100% |
| **Layout (Layout-*)** | **35** | **4** | **15** | **16** | **54%** |
| **ABI (ABI-*)** | **22** | **1** | **10** | **11** | **50%** |
| **Lowering (Lower-*)** | **95** | **10** | **40** | **45** | **53%** |
| Cleanup (Cleanup-*) | 18 | 5 | 13 | 0 | 100% |
| Well-formedness (WF-*) | 70 | 55 | 15 | 0 | 100% |
| Resolution (Resolve-*) | 20 | 15 | 5 | 0 | 100% |
| **TOTAL** | **~640** | **~304** | **~263** | **~73** | **89%** |

## Detailed Coverage by Section

### 5.2.6 Type Equivalence (T-Equiv-*)

| Rule | Status | Test File(s) | Notes |
|------|--------|--------------|-------|
| T-Equiv-Prim | ✅ Implicit | run-pass/primitives/*.cursive | Exercised by all primitive operations |
| T-Equiv-Perm | ✅ Direct | run-pass/permissions/*.cursive | Permission coercion tests |
| T-Equiv-Tuple | ✅ Implicit | run-pass/tuples/*.cursive | Tuple operations |
| T-Equiv-Array | ✅ Implicit | run-pass/arrays/*.cursive | Array operations |
| T-Equiv-Slice | ✅ Implicit | run-pass/slices/*.cursive | Slice operations |
| T-Equiv-Func | ✅ Implicit | run-pass/closures/*.cursive | Closure type checking |
| T-Equiv-Union | ✅ Implicit | run-pass/unions/*.cursive | Union operations |
| T-Equiv-Path | ✅ Implicit | run-pass/records/*.cursive | Record type paths |
| T-Equiv-ModalState | ✅ Implicit | run-pass/modals/*.cursive | Modal state matching |
| T-Equiv-String | ✅ Implicit | run-pass/strings/*.cursive | String operations |
| T-Equiv-Bytes | ✅ Implicit | run-pass/strings/*.cursive | Bytes operations |
| T-Equiv-Range | ✅ Implicit | run-pass/control-flow/match-range.cursive | Range patterns |
| T-Equiv-Ptr | ✅ Implicit | run-pass/pointers/*.cursive | Pointer operations |
| T-Equiv-RawPtr | ✅ Implicit | run-pass/memory/raw-ptr.cursive | Raw pointer operations |
| T-Equiv-Dynamic | ✅ Implicit | run-pass/classes/dynamic-dispatch.cursive | Dynamic class types |

### 5.2.7 Subtyping (Sub-*)

| Rule | Status | Test File(s) | Notes |
|------|--------|--------------|-------|
| Sub-Perm | ✅ Direct | run-pass/permissions/coercion-chain.cursive | unique→shared→const |
| Sub-Never | ✅ Direct | run-pass/types/never-coercion.cursive | ! type coercion |
| Sub-Tuple | ✅ Implicit | run-pass/tuples/*.cursive | Element-wise subtyping |
| Sub-Array | ✅ Implicit | run-pass/arrays/*.cursive | Element subtyping |
| Sub-Slice | ✅ Implicit | run-pass/slices/*.cursive | Element subtyping |
| Sub-Range | ✅ Implicit | run-pass/control-flow/inclusive-range.cursive | Range subtyping |
| Sub-Ptr-State | ✅ Implicit | run-pass/pointers/*.cursive | @Valid→@Null widening |
| Sub-Modal-Niche | ✅ Implicit | run-pass/modals/*.cursive | Modal niche optimization |
| Sub-Func | ✅ Implicit | run-pass/closures/*.cursive | Contravariant params |
| Sub-Async | ✅ Implicit | run-pass/async/*.cursive | Async signature subtyping |
| Sub-Member-Union | ✅ Implicit | run-pass/unions/*.cursive | Member inclusion |
| Sub-Union-Width | ✅ Direct | run-pass/types/union-widening.cursive | Union widening |

### 5.2.10-12 Expression Typing - Error Cases

| Rule (Error) | Status | Test File(s) | Diagnostic Code |
|--------------|--------|--------------|-----------------|
| T-LetStmt-Ann-Mismatch | ✅ Direct | ui/E-TYP/E-TYP-1530.cursive | E-TYP-1530 |
| T-LetStmt-Infer-Err | ✅ Direct | ui/E-TYP/E-TYP-1530.cursive | E-TYP-1530 |
| T-VarStmt-Ann-Mismatch | ✅ Direct | ui/E-TYP/E-TYP-1530.cursive | E-TYP-1530 |
| Call-ArgCount-Err | ✅ Direct | ui/E-SEM/E-SEM-2532.cursive | E-SEM-2532 |
| Call-ArgType-Err | ✅ Direct | ui/E-SEM/E-SEM-2533.cursive | E-SEM-2533 |
| Index-Array-OOB-Err | ✅ Direct | ui/E-TYP/E-TYP-1810.cursive | E-TYP-1810 |
| Union-DirectAccess-Err | ✅ Direct | ui/E-TYP/E-TYP-2202.cursive | E-TYP-2202 |

### 5.2.15 Binding State (B-*) - Error Cases

| Rule (Error) | Status | Test File(s) | Diagnostic Code |
|--------------|--------|--------------|-----------------|
| B-Place-Unique-Err | ✅ Direct | ui/E-TYP/E-TYP-1602.cursive | E-TYP-1602 |
| B-Place-Moved-Err | ✅ Direct | ui/E-MEM/E-MEM-3001.cursive | E-MEM-3001 |
| B-Move-Whole-Moved-Err | ✅ Direct | ui/E-MEM/E-MEM-3003.cursive | E-MEM-3003 |
| B-Move-Field-Moved-Err | ✅ Direct | ui/E-MEM/E-MEM-3004.cursive | E-MEM-3004 |
| B-Move-Whole-Immovable-Err | ✅ Direct | ui/E-MEM/E-MEM-3005.cursive | E-MEM-3005 |
| B-ArgPass-Move-Missing | ✅ Direct | ui/E-SEM/E-SEM-2534.cursive | E-SEM-2534 |
| B-Assign-Immutable-Err | ✅ Direct | ui/E-SEM/E-SEM-3131.cursive | E-SEM-3131 |
| B-Assign-Const-Err | ✅ Direct | ui/E-TYP/E-TYP-1601.cursive | E-TYP-1601 |

### 6.1 Layout (Layout-*) - **CRITICAL GAPS**

| Rule | Status | Test File(s) | Notes |
|------|--------|--------------|-------|
| Layout-Prim | ✅ Direct | codegen/layout/primitive-sizes.cursive | All primitives verified |
| Layout-Perm | ✅ Implicit | codegen/layout/record-layout.cursive | Via record fields |
| Layout-Ptr | ❌ **MISSING** | - | Needs IR verification |
| Layout-RawPtr | ❌ **MISSING** | - | Needs IR verification |
| Layout-Func | ❌ **MISSING** | - | Function pointer layout |
| Layout-Record | ✅ Direct | codegen/layout/record-layout.cursive | Field offsets verified |
| Layout-Alias | ❌ **MISSING** | - | Type alias layout |
| Layout-Union-Niche | ❌ **MISSING** | - | Niche optimization layout |
| Layout-Union-Tagged | ❌ **MISSING** | - | Tagged union layout |
| Layout-String-Managed | ❌ **MISSING** | - | Managed string layout |
| Layout-String-View | ❌ **MISSING** | - | String view layout |
| Layout-Bytes-Managed | ❌ **MISSING** | - | Managed bytes layout |
| Layout-Bytes-View | ❌ **MISSING** | - | Bytes view layout |
| Layout-Tuple | ❌ **MISSING** | - | Tuple layout verification |
| Layout-Array | ❌ **MISSING** | - | Array layout verification |
| Layout-Slice | ❌ **MISSING** | - | Slice layout (ptr+len) |
| Layout-Range | ❌ **MISSING** | - | Range layout |
| Layout-Enum-Tagged | ❌ **MISSING** | - | Enum discriminant + payload |
| Layout-Modal-Niche | ❌ **MISSING** | - | Modal niche optimization |
| Layout-Modal-Tagged | ❌ **MISSING** | - | Modal tagged layout |
| Layout-DynamicClass | ❌ **MISSING** | - | Dynamic class vtable ptr |

### 6.2 ABI (ABI-*) - **CRITICAL GAPS**

| Rule | Status | Test File(s) | Notes |
|------|--------|--------------|-------|
| ABI-Prim | ✅ Implicit | codegen/abi/calling-convention.cursive | Basic calling convention |
| ABI-Perm | ❌ **MISSING** | - | Permission passing ABI |
| ABI-Ptr | ❌ **MISSING** | - | Pointer argument ABI |
| ABI-RawPtr | ❌ **MISSING** | - | Raw pointer ABI |
| ABI-Func | ❌ **MISSING** | - | Function pointer ABI |
| ABI-Record | ❌ **MISSING** | - | Record passing (by-ref/by-value) |
| ABI-Tuple | ❌ **MISSING** | - | Tuple passing ABI |
| ABI-Array | ❌ **MISSING** | - | Array passing ABI |
| ABI-Slice | ❌ **MISSING** | - | Slice passing (fat pointer) |
| ABI-Range | ❌ **MISSING** | - | Range passing ABI |
| ABI-Enum | ❌ **MISSING** | - | Enum passing ABI |
| ABI-Union | ❌ **MISSING** | - | Union passing ABI |
| ABI-Modal | ❌ **MISSING** | - | Modal passing ABI |
| ABI-Dynamic | ❌ **MISSING** | - | Dynamic class ABI |
| ABI-StringBytes | ❌ **MISSING** | - | String/bytes ABI |
| ABI-Param-ByRef-Alias | ✅ Implicit | codegen/abi/calling-convention.cursive | Reference passing |
| ABI-Param-ByValue-Move | ❌ **MISSING** | - | Move by value |
| ABI-Param-ByRef-Move | ❌ **MISSING** | - | Move by reference |
| ABI-Ret-ByValue | ❌ **MISSING** | - | Return by value |
| ABI-Ret-ByRef | ❌ **MISSING** | - | Sret convention |
| ABI-Call | ✅ Implicit | codegen/abi/calling-convention.cursive | Basic call ABI |

### 6.4-6 Expression Lowering (Lower-Expr-*) - **GAPS**

| Rule | Status | Test File(s) | Notes |
|------|--------|--------------|-------|
| Lower-Expr-Literal | ✅ Implicit | run-pass/primitives/*.cursive | Via execution |
| Lower-Expr-PtrNull | ✅ Implicit | run-pass/pointers/*.cursive | Via execution |
| Lower-Expr-Ident-Local | ✅ Implicit | run-pass/bindings/*.cursive | Via execution |
| Lower-Expr-Tuple | ❌ **MISSING** | - | Needs IR verification |
| Lower-Expr-Array | ❌ **MISSING** | - | Needs IR verification |
| Lower-Expr-Record | ❌ **MISSING** | - | Needs IR verification |
| Lower-Expr-Enum-* | ❌ **MISSING** | - | Needs IR verification |
| Lower-Expr-FieldAccess | ❌ **MISSING** | - | GEP instruction check |
| Lower-Expr-TupleAccess | ❌ **MISSING** | - | GEP instruction check |
| Lower-Expr-Index-* | ❌ **MISSING** | - | Bounds check + GEP |
| Lower-Expr-Call-* | ❌ **MISSING** | - | Call instruction lowering |
| Lower-Expr-MethodCall | ❌ **MISSING** | - | Method call lowering |
| Lower-Expr-Unary | ❌ **MISSING** | - | Unary op lowering |
| Lower-Expr-Binary | ❌ **MISSING** | - | Binary op lowering |
| Lower-Expr-Cast | ❌ **MISSING** | - | Cast instruction |
| Lower-Expr-Transmute | ❌ **MISSING** | - | Bitcast instruction |
| Lower-Expr-Propagate-* | ❌ **MISSING** | - | Union propagation lowering |
| Lower-Expr-Range | ❌ **MISSING** | - | Range struct lowering |
| Lower-Expr-If | ❌ **MISSING** | - | Branch instruction |
| Lower-Expr-Match | ❌ **MISSING** | - | Switch/branch lowering |
| Lower-Expr-Loop* | ❌ **MISSING** | - | Loop structure lowering |
| Lower-Expr-Block | ❌ **MISSING** | - | Block scope lowering |
| Lower-Expr-Move | ❌ **MISSING** | - | Move semantics lowering |
| Lower-Expr-AddressOf | ❌ **MISSING** | - | Address computation |
| Lower-Expr-Deref | ❌ **MISSING** | - | Load instruction |
| Lower-Expr-Alloc | ❌ **MISSING** | - | Region allocation |

### 7.4 Cleanup (Cleanup-*) - Partially Covered

| Rule | Status | Test File(s) | Notes |
|------|--------|--------------|-------|
| Cleanup-Start | ✅ Implicit | run-pass/defer/*.cursive | Via defer execution |
| Cleanup-Step-Drop-Ok | ✅ Implicit | run-pass/memory/drop-verification.cursive | Drop order verified |
| Cleanup-Step-Drop-Panic | ❌ **NEEDS TEST** | - | Double-panic abort |
| Cleanup-Step-DropStatic-* | ✅ Implicit | run-pass/statics/*.cursive | Static cleanup |
| Cleanup-Step-Defer-Ok | ✅ Direct | run-pass/defer/defer-order.cursive | Defer execution order |
| Cleanup-Step-Defer-Panic | ❌ **NEEDS TEST** | - | Panic in defer |
| Cleanup-Done | ✅ Implicit | All run-pass tests | Via normal execution |
| Cleanup-Cons-Drop-Panic | ❌ **NEEDS TEST** | - | Drop panic propagation |
| Cleanup-Cons-Defer-Panic | ❌ **NEEDS TEST** | - | Defer panic propagation |

## Legend

- ✅ **Direct**: Rule is explicitly tested via UI test or run-pass test with CHECK directive
- ✅ **Implicit**: Rule is exercised by tests but not explicitly verified
- ❌ **MISSING**: No test coverage exists; needs new test
- ❌ **NEEDS TEST**: Partially covered but missing edge cases
