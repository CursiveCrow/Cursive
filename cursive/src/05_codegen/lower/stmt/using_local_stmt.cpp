// =============================================================================
// using_local_stmt.cpp - Lowering for UsingLocalStmt
// =============================================================================
//
// SPEC REFERENCE:
//   CursiveSpecification.md §18.3.6 Lowering
//     (Lower-Stmt-UsingLocal) - UsingLocalStmt lowers to NoOpIR.
//
// `using` is compile-time only: by the time lowering runs, all aliases have
// been resolved at the symbol-table level. No IR is emitted.
// =============================================================================

#include "00_core/assert_spec.h"
#include "02_source/ast/ast.h"
#include "05_codegen/ir/ir_model.h"
#include "05_codegen/lower/lower_stmt.h"

namespace cursive::codegen {

IRPtr LowerUsingLocalStmt(const cursive::ast::UsingLocalStmt& stmt,
                          LowerCtx& ctx) {
  SPEC_RULE("Lower-Stmt-UsingLocal");
  (void)stmt;
  (void)ctx;
  return std::make_shared<IR>(IR{IROpaque{}});
}

}  // namespace cursive::codegen
