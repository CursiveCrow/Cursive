#pragma once

#include "05_codegen/lower/lower_expr.h"

namespace cursive::codegen {

LowerResult LowerMethodCall(const ast::MethodCallExpr& expr, LowerCtx& ctx);

}  // namespace cursive::codegen
