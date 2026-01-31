// =============================================================================
// MIGRATION MAPPING: intrinsics_interface.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   - Intrinsic function interface patterns
//   - Foreign contract interface (lines in §6.5 FFI)
//   - Runtime symbol conventions
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/abi/abi_params.cpp
//   - Lines 1-50: RuntimeSymbols set definition
//   - Runtime function declarations
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/ir_lowering.cpp
//   - Intrinsic call emission patterns
//   - Runtime function invocation
//
// DEPENDENCIES:
//   - cursive/include/05_codegen/intrinsics/intrinsics_interface.h
//   - cursive/include/05_codegen/ir/ir_model.h
//   - cursive/include/05_codegen/abi/abi_calls.h
//
// REFACTORING NOTES:
//   1. Intrinsics have special calling conventions
//   2. Some intrinsics inline, others call runtime
//   3. Runtime symbols prefixed with __cursive_
//   4. Intrinsic interface validates arguments
//   5. Type checking happens before lowering
//
// INTRINSIC INTERFACE PATTERN:
//   1. Lookup intrinsic by name
//   2. Validate argument count and types
//   3. Select implementation strategy:
//      a. Inline IR generation
//      b. Runtime function call
//      c. LLVM intrinsic mapping
//   4. Generate appropriate IR
//
// RUNTIME SYMBOLS:
//   __cursive_panic - panic handling
//   __cursive_alloc - heap allocation
//   __cursive_dealloc - heap deallocation
//   __cursive_memcpy - memory copy
//   __cursive_memset - memory set
//   __cursive_abort - program abort
//
// LLVM INTRINSIC MAPPINGS:
//   - Overflow arithmetic: llvm.sadd.with.overflow, etc.
//   - Memory: llvm.memcpy, llvm.memset, llvm.memmove
//   - Math: llvm.sqrt, llvm.pow, llvm.fabs, etc.
//   - Atomics: llvm.atomicrmw, llvm.cmpxchg
// =============================================================================

#include "cursive/05_codegen/intrinsics/intrinsics_interface.h"

// TODO: Implement intrinsic dispatch and interface

