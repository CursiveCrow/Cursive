#pragma once

// =============================================================================
// Shadow Var Statement Lowering
// =============================================================================
//
// SPEC REFERENCE: CursiveSpecification.md Lines 16634-16637 (Lower-Stmt-ShadowVar)
//
// =============================================================================

#include "05_codegen/lower/lower_stmt.h"

namespace cursive::codegen {

// Lower shadow var statement - shadowing mutable binding
IRPtr LowerShadowVarStmt(const ast::ShadowVarStmt& stmt, LowerCtx& ctx);

}  // namespace cursive::codegen
