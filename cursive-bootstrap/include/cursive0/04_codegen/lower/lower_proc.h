#pragma once

#include <string>
#include <vector>

#include "cursive0/04_codegen/ir_model.h"
#include "cursive0/04_codegen/lower/lower_expr.h"
#include "cursive0/02_syntax/ast.h"

namespace cursive0::codegen {

// ============================================================================
// §6.x Procedure Lowering
// ============================================================================

// LowerProc - Lower a procedure declaration to ProcIR
// Handles parameter binding, body lowering, and return handling.
ProcIR LowerProc(const syntax::ProcedureDecl& decl,
                 const syntax::ModulePath& module_path,
                 LowerCtx& ctx);

// AnchorProcRules - Emit SPEC_RULE anchors
void AnchorProcRules();

}  // namespace cursive0::codegen
