// =============================================================================
// MIGRATION MAPPING: globals.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   - Section 6.0 CG-Item-Static rule (lines 14272-14275)
//   - EmitGlobal judgment (line 14273)
//   - GlobalConst, GlobalZero IR declarations (referenced in ir_model)
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/globals.cpp
//   - EmitGlobal function for static declarations
//   - Global initialization handling
//
// DEPENDENCIES:
//   - cursive/include/05_codegen/globals/globals.h
//   - cursive/include/05_codegen/ir/ir_model.h (GlobalConst, GlobalZero)
//   - cursive/include/05_codegen/layout/layout.h (SizeOf, AlignOf)
//   - cursive/include/05_codegen/symbols/mangle.h (MangleStatic)
//
// REFACTORING NOTES:
//   1. Static declarations generate global IR:
//      - Const statics with init: GlobalConst (immutable data)
//      - Mutable statics: GlobalZero (zeroed, runtime init)
//   2. EmitGlobal returns IRDecls for the static
//   3. Symbol mangled per §6.3.1
//   4. Linkage determined by visibility
//   5. Initial value encoded as bytes for GlobalConst
//   6. GlobalZero just needs size/alignment
//
// GLOBAL IR TYPES:
//   GlobalConst {
//     symbol: string,
//     bytes: vector<u8>,
//     size: u64,
//     align: u64,
//     linkage: LinkageKind
//   }
//
//   GlobalZero {
//     symbol: string,
//     size: u64,
//     align: u64,
//     linkage: LinkageKind
//   }
// =============================================================================

#include "cursive/05_codegen/globals/globals.h"

// TODO: Migrate implementation from cursive-bootstrap/src/04_codegen/globals.cpp
