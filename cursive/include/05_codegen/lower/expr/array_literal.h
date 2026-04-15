#pragma once

#include "05_codegen/lower/lower_expr.h"

namespace cursive::codegen {

// =============================================================================
// Section 6.4 Array Literal Lowering
// =============================================================================

// Lower-Expr-Array - Lower an array literal expression [e1, e2, ...]
// Lowers each element expression left-to-right via LowerList, then
// produces a synthetic array value tracked via DerivedValueInfo.
LowerResult LowerArrayLiteral(const ast::ArrayExpr& expr, LowerCtx& ctx);

// Lower-Expr-ArrayRepeat - Lower an array repeat expression [value; count]
// Lowers the value and count expressions, then produces a synthetic array
// value tracked via DerivedValueInfo with kind ArrayRepeat.
LowerResult LowerArrayRepeat(const ast::ArrayRepeatExpr& expr, LowerCtx& ctx);

}  // namespace cursive::codegen
