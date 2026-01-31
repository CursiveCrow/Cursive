#pragma once

#include <vector>
#include "cursive0/04_codegen/ir_model.h"
#include "cursive0/04_codegen/lower/lower_expr.h"
#include "cursive0/02_syntax/ast.h"

namespace cursive0::codegen {

// LowerModule - Lower an entire module to IR declarations
IRDecls LowerModule(const syntax::ASTModule& module,
                    LowerCtx& ctx);

}  // namespace cursive0::codegen
