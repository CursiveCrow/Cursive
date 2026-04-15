// =============================================================================
// unsafe_block_stmt.cpp - Unsafe block statement typing
// =============================================================================
//
// SPEC REFERENCE: CursiveSpecification.md
//   Section 5.5: Memory and Pointers - Unsafe blocks
//   - unsafe block as statement
//   - See also expr/unsafe_block_expr.cpp for expression form
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

namespace {

static inline void SpecDefsUnsafeBlockStmt() {
  SPEC_DEF("T-UnsafeStmt", "5.5");
}

}  // namespace

StmtTypeResult TypeUnsafeBlockStmt(const ScopeContext& ctx,
                                   const StmtTypeContext& type_ctx,
                                   const ast::UnsafeBlockStmt& node,
                                   const TypeEnv& env,
                                   const ExprTypeFn& type_expr,
                                   const IdentTypeFn& type_ident,
                                   const PlaceTypeFn& type_place) {
  SpecDefsUnsafeBlockStmt();

  if (!node.body) {
    return {false, std::nullopt, {}, {}};
  }

  // Create unsafe context
  StmtTypeContext unsafe_ctx = type_ctx;
  unsafe_ctx.in_unsafe = true;

  // Type the unsafe block body with unsafe context enabled
  const auto typed = TypeBlock(ctx, unsafe_ctx, *node.body, env,
                               type_expr, type_ident, type_place,
                               unsafe_ctx.env_ref);
  if (!typed.ok) {
    return {false, typed.diag_id, {}, {}, typed.diag_detail};
  }

  // Unsafe block statement produces unit type (statement form)
  // Environment is unchanged (block scope exits)
  SPEC_RULE("T-UnsafeStmt");
  return {true, std::nullopt, env, {}};
}

}  // namespace cursive::analysis
