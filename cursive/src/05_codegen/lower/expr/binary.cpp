// =============================================================================
// MIGRATION MAPPING: expr/binary.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Section 6.4 (Expression Lowering)
//   - Lines 16163-16176: Short-circuit and non-short-circuit binary ops
//     - (Lower-Expr-Bin-And): && with short-circuit
//     - (Lower-Expr-Bin-Or): || with short-circuit
//     - (Lower-Expr-Binary): All other operators
//   - Lines 16328-16336: (Lower-BinOp-Ok) and (Lower-BinOp-Panic)
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/lower/lower_expr_core.cpp
//   - BinaryExpr visitor with operator dispatch
//   - Short-circuit lowering produces IRIf for && and ||
//   - Panic checks for div/mod by zero, overflow, shift
//
// DEPENDENCIES:
//   - cursive/src/05_codegen/ir_model.h (IRBinaryOp, IRIf, IRCheckOp)
//   - cursive/src/05_codegen/checks.h (OpPanicReason)
//
// BINARY OPERATORS:
//   Arithmetic: + - * / % **
//   Comparison: == != < <= > >=
//   Logical: && ||
//   Bitwise: & | ^ << >>
//
// =============================================================================

#include "cursive/05_codegen/lower/expr/binary.h"

namespace cursive::codegen {

// TODO: Migrate implementation from cursive-bootstrap

}  // namespace cursive::codegen
