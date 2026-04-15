#pragma once

// =============================================================================
// Shadow Let Statement Lowering
// =============================================================================
//
// SPEC REFERENCE: CursiveSpecification.md Lines 16629-16632 (Lower-Stmt-ShadowLet)
//
// =============================================================================

#include "05_codegen/lower/lower_stmt.h"

namespace cursive::codegen {

// Lower shadow let statement - shadowing immutable binding
IRPtr LowerShadowLetStmt(const ast::ShadowLetStmt& stmt, LowerCtx& ctx);

}  // namespace cursive::codegen
