// =============================================================================
// defer_stmt.cpp - Defer statement typing
// =============================================================================
//
// SPEC REFERENCE: CursiveSpecification.md
//   Section 5.2.11: Statement Typing
//   - defer { block } executes block at scope exit
//   - T-DeferStmt: Defer statement typing
//   - Defer-NonUnit-Err: Non-unit deferred block (warning)
//   - Defer-NonLocal-Err: Non-local control flow in defer
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_stmt.cpp
//
// =============================================================================

#include "04_analysis/typing/type_stmt.h"

#include <optional>
#include <string_view>

#include "00_core/assert_spec.h"
#include "02_source/ast/ast.h"

namespace cursive::analysis {

// Forward declaration - defined in stmt_common.cpp
bool DeferSafe(const ast::Block& block);

namespace {

static inline void SpecDefsDeferStmt() {
  SPEC_DEF("T-DeferStmt", "5.2.11");
  SPEC_DEF("Defer-NonUnit-Err", "5.2.11");
  SPEC_DEF("Defer-NonLocal-Err", "5.2.11");
  SPEC_DEF("DeferSafe", "5.2.11");
}

// Check for non-local control flow in a block
static bool HasNonLocalCtrlStmt(const ast::Stmt& stmt, bool in_loop);
static bool HasNonLocalCtrlExpr(const ast::ExprPtr& expr, bool in_loop);

static bool HasNonLocalCtrlBlock(const ast::Block& block, bool in_loop) {
  for (const auto& stmt : block.stmts) {
    if (HasNonLocalCtrlStmt(stmt, in_loop)) {
      return true;
    }
  }
  if (block.tail_opt && HasNonLocalCtrlExpr(block.tail_opt, in_loop)) {
    return true;
  }
  return false;
}

static bool HasNonLocalCtrlStmt(const ast::Stmt& stmt, bool in_loop) {
  return std::visit(
      [&](const auto& node) -> bool {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, ast::ReturnStmt>) {
          return true;
        } else if constexpr (std::is_same_v<T, ast::BreakStmt>) {
          return !in_loop;
        } else if constexpr (std::is_same_v<T, ast::ContinueStmt>) {
          return !in_loop;
        } else if constexpr (std::is_same_v<T, ast::DeferStmt>) {
          if (node.body && HasNonLocalCtrlBlock(*node.body, in_loop)) {
            return true;
          }
          return false;
        } else {
          return false;
        }
      },
      stmt);
}

static bool HasNonLocalCtrlExpr(const ast::ExprPtr& expr, bool in_loop) {
  if (!expr) {
    return false;
  }
  return std::visit(
      [&](const auto& node) -> bool {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, ast::LoopInfiniteExpr> ||
                      std::is_same_v<T, ast::LoopConditionalExpr> ||
                      std::is_same_v<T, ast::LoopIterExpr>) {
          // Loops establish their own context
          if (node.body && HasNonLocalCtrlBlock(*node.body, true)) {
            return true;
          }
          return false;
        } else if constexpr (std::is_same_v<T, ast::BlockExpr>) {
          if (node.block && HasNonLocalCtrlBlock(*node.block, in_loop)) {
            return true;
          }
          return false;
        } else {
          return false;
        }
      },
      expr->node);
}

static bool LocalDeferSafe(const ast::Block& block) {
  return !HasNonLocalCtrlBlock(block, false);
}

}  // namespace

StmtTypeResult TypeDeferStmt(const ScopeContext& ctx,
                             const StmtTypeContext& type_ctx,
                             const ast::DeferStmt& node,
                             const TypeEnv& env,
                             const ExprTypeFn& type_expr,
                             const IdentTypeFn& type_ident,
                             const PlaceTypeFn& type_place) {
  SpecDefsDeferStmt();

  if (!node.body) {
    return {false, std::nullopt, {}, {}};
  }

  // Type check the deferred block - it should have unit type
  const auto check = CheckBlock(ctx, type_ctx, *node.body, env,
                                MakeTypePrim("()"), type_expr,
                                type_ident, type_place,
                                type_ctx.env_ref);
  if (!check.ok) {
    if (!check.diag_id.has_value()) {
      SPEC_RULE("Defer-NonUnit-Err");
      return {false, "Defer-NonUnit-Err", {}, {}};
    }
    return {false, check.diag_id, {}, {}};
  }

  // Check that defer block doesn't contain non-local control flow
  if (!LocalDeferSafe(*node.body)) {
    SPEC_RULE("Defer-NonLocal-Err");
    return {false, "Defer-NonLocal-Err", {}, {}};
  }

  SPEC_RULE("T-DeferStmt");
  return {true, std::nullopt, env, {}};
}

}  // namespace cursive::analysis
