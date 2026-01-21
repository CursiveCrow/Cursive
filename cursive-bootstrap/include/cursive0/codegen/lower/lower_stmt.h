#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "cursive0/codegen/ir_model.h"
#include "cursive0/codegen/lower/lower_expr.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::codegen {

// ============================================================================
// §6.5 Statement Lowering Judgments
// ============================================================================

// §6.5 LowerStmt - main statement lowering entry point
IRPtr LowerStmt(const syntax::Stmt& stmt, LowerCtx& ctx);

// §6.5 LowerStmtList - lower a list of statements
IRPtr LowerStmtList(const std::vector<syntax::Stmt>& stmts, LowerCtx& ctx);

// §6.5 LowerBlock - lower a block (statements + optional tail expression)
LowerResult LowerBlock(const syntax::Block& block, LowerCtx& ctx);

// ============================================================================
// §6.5 Specific Statement Forms
// ============================================================================

// §6.5 Lower-Stmt-Let-* (let bindings)
IRPtr LowerLetStmt(const syntax::LetStmt& stmt, LowerCtx& ctx);

// §6.5 Lower-Stmt-Var-* (var bindings)
IRPtr LowerVarStmt(const syntax::VarStmt& stmt, LowerCtx& ctx);

// §6.5 Lower-Stmt-Assignment
IRPtr LowerAssignStmt(const syntax::AssignStmt& stmt, LowerCtx& ctx);

// §6.5 Lower-Stmt-OpAssignment
IRPtr LowerCompoundAssignStmt(const syntax::CompoundAssignStmt& stmt,
                              LowerCtx& ctx);

// §6.5 Lower-Stmt-Expr
IRPtr LowerExprStmt(const syntax::ExprStmt& stmt, LowerCtx& ctx);

// §6.5 Lower-Stmt-Return
IRPtr LowerReturnStmt(const syntax::ReturnStmt& stmt,
                      LowerCtx& ctx,
                      const std::vector<TempValue>& temps);

// §6.5 Lower-Stmt-Break
IRPtr LowerBreakStmt(const syntax::BreakStmt& stmt,
                     LowerCtx& ctx,
                     const std::vector<TempValue>& temps);

// §6.5 Lower-Stmt-Continue
IRPtr LowerContinueStmt(const syntax::ContinueStmt& stmt,
                        LowerCtx& ctx,
                        const std::vector<TempValue>& temps);

// ============================================================================
// §6.5 Loop Lowering
// ============================================================================

// §6.5 Lower-Loop-Infinite
LowerResult LowerLoopInfinite(const syntax::LoopInfiniteExpr& expr,
                              LowerCtx& ctx);

// §6.5 Lower-Loop-Conditional
LowerResult LowerLoopConditional(const syntax::LoopConditionalExpr& expr,
                                 LowerCtx& ctx);

// §6.5 Lower-Loop-Iter
LowerResult LowerLoopIter(const syntax::LoopIterExpr& expr, LowerCtx& ctx);

// §6.5 LowerLoop - dispatch to appropriate loop lowering
LowerResult LowerLoop(const syntax::Expr& loop, LowerCtx& ctx);

// ============================================================================
// §6.5 Cleanup at Statement Boundaries
// ============================================================================

// §6.5 CleanupList - temporaries to drop at end of statement
IRPtr CleanupList(const std::vector<TempValue>& temps, LowerCtx& ctx);

// Emit SPEC_RULE anchors for all §6.5 rules
void AnchorStmtLoweringRules();

}  // namespace cursive0::codegen
