// =============================================================================
// MIGRATION MAPPING: expr/method_call.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Section 6.4 (Expression Lowering)
//   - Lines 16153-16156: (Lower-Expr-MethodCall)
//     Gamma |- LowerMethodCall(...) => <IR, v_call>
//   - Section 6.10: Dynamic Dispatch (Lines 17161-17202)
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/lower/lower_expr_calls.cpp
//   - MethodCallExpr lowering with receiver handling
//   - Static vs dynamic dispatch determination
//   - VTable lookup for dynamic dispatch
//
// DEPENDENCIES:
//   - cursive/src/05_codegen/ir_model.h (IRCall, IRVTableLookup)
//   - cursive/src/05_codegen/dyn_dispatch.h (vtable resolution)
//   - cursive/src/04_analysis/composite/record_methods.h (LookupMethodStatic)
//
// =============================================================================

#include "cursive/05_codegen/lower/expr/method_call.h"

namespace cursive::codegen {

// TODO: Migrate implementation from cursive-bootstrap

}  // namespace cursive::codegen
