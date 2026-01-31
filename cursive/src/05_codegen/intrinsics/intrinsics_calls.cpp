// =============================================================================
// MIGRATION MAPPING: intrinsics_calls.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   - Section 6.4 Expression Lowering (lines 15665-15992)
//   - Lower-Transmute rule (lines 17255-17258)
//   - Lower-Transmute-Err rule (lines 17260-17263)
//   - TransmuteVal judgment for bit reinterpretation
//   - widen operator lowering (lines 12814-12845)
//   - Contract intrinsics @result, @entry (lines 3357, 23204-23266)
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/lower/lower_expr_core.cpp
//   - Lines 255-268: MakeNullPtr using IRTransmute
//   - Transmute IR generation for type punning
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/ir_lowering.cpp
//   - Lines 3900-4050: Modal widen implementation
//   - Lines 4183+: IRTransmute LLVM lowering
//   - String/bytes widening (View -> general)
//   - Modal state widening (specific -> general)
//
// DEPENDENCIES:
//   - cursive/include/05_codegen/intrinsics/intrinsics_calls.h
//   - cursive/include/05_codegen/ir/ir_model.h (IRTransmute, IRWiden)
//   - cursive/include/05_codegen/layout/layout.h (SizeOf, AlignOf)
//   - cursive/include/04_analysis/types/types.h (TypeRef)
//
// REFACTORING NOTES:
//   1. Transmute performs bit-level reinterpretation
//   2. Size and alignment must match for transmute
//   3. Widen converts specific modal state to general
//   4. String/bytes widening packs discriminant + payload
//   5. Modal widening may use niche optimization
//   6. @result/@entry evaluated at runtime in dynamic contracts
//
// TRANSMUTE LOWERING:
//   - Verify SizeOf(T1) == SizeOf(T2)
//   - Verify AlignOf(T1) == AlignOf(T2) (or relax per spec)
//   - Generate bitcast IR instruction
//   - No runtime checks (unsafe operation)
//
// WIDEN LOWERING:
//   For modal types with niche:
//     - Niche state: use niche value directly
//     - Payload state: extract payload
//   For modal types without niche (tagged):
//     - Allocate space for general type
//     - Store discriminant
//     - Copy payload at aligned offset
//
// STRING/BYTES WIDENING:
//   - View/Managed -> general string/bytes
//   - Pack: discriminant (u8) + ptr + len + cap
//   - Managed uses cap, View uses 0
// =============================================================================

#include "cursive/05_codegen/intrinsics/intrinsics_calls.h"

// TODO: Migrate implementation from cursive-bootstrap/src/04_codegen/ir_lowering.cpp
// TODO: Migrate implementation from cursive-bootstrap/src/04_codegen/lower/lower_expr_core.cpp

