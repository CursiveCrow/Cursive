// =============================================================================
// MIGRATION MAPPING: intrinsics_decls.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   - Contract intrinsics @result, @entry (lines 3357, 23204-23266)
//   - @result: references return value in postcondition
//   - @entry(expr): captures entry/old value of expression
//   - ContractViolation panic payload (line 23668)
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/ir_model.cpp
//   - IRTransmute node definition
//   - IR intrinsic declarations
//
// DEPENDENCIES:
//   - cursive/include/05_codegen/intrinsics/intrinsics_decls.h
//   - cursive/include/05_codegen/ir/ir_model.h
//   - cursive/include/04_analysis/types/types.h
//
// REFACTORING NOTES:
//   1. Intrinsic declarations are compiler-known functions
//   2. @result bound only in postcondition context
//   3. @entry(expr) requires BitcopyType or CloneType
//   4. Transmute is unsafe intrinsic
//   5. Widen is safe but may warn for large copies
//
// INTRINSIC CATEGORIES:
//   - Type manipulation: transmute
//   - Modal operations: widen
//   - Contract support: @result, @entry
//   - Memory operations: (handled in other modules)
//
// CONTRACT INTRINSICS:
//   @result:
//     - Type: matches procedure return type
//     - Scope: postcondition only (right of =>)
//     - Binding: implicit, bound to return value
//
//   @entry(expr):
//     - Type: matches expr type (must be Bitcopy or Clone)
//     - Scope: postcondition only
//     - Semantics: evaluates expr at entry, captures value
//     - Storage: compiler generates temp at procedure entry
// =============================================================================

#include "cursive/05_codegen/intrinsics/intrinsics_decls.h"

// TODO: Define intrinsic declarations and metadata

