// =============================================================================
// Region Statement Lowering Implementation
// =============================================================================
//
// SPEC REFERENCE: CursiveSpecification.md Lines 16659-16662 (Lower-Stmt-Region)
//   - LowerExpr(opts_opt) or default RegionOptions
//   - RegionIR(v_o, alias_opt, IR_b, v_b)
//
// MIGRATED FROM:
//   - cursive-bootstrap/src/04_codegen/lower/lower_stmt.cpp
//   - Lines 702-746: RegionStmt case in LowerStmt dispatch
//
// =============================================================================

#include "05_codegen/lower/stmt/region_stmt.h"

#include <string>
#include <variant>

#include "00_core/assert_spec.h"
#include "04_analysis/memory/regions.h"
#include "04_analysis/typing/types.h"
#include "05_codegen/ir/ir_model.h"
#include "05_codegen/lower/lower_expr.h"
#include "05_codegen/lower/lower_stmt.h"

namespace cursive::codegen {

// ============================================================================
// Lower-Stmt-Region
// ============================================================================
//
// Per the spec (Lines 16659-16662):
//   LowerExpr(opts_opt) or default RegionOptions => <IR_o, v_o>
//   PushScope with is_region=true
//   LowerBlock(body) => <IR_b, v_b>
//   Result: SeqIR(IR_o, RegionIR(v_o, alias_opt, IR_b, v_b))
//
// The implementation handles:
//   - Default RegionOptions if not provided
//   - Region alias registration
//   - Scope management for region lifetime tracking
//
IRPtr LowerRegionStmt(const ast::RegionStmt& stmt, LowerCtx& ctx) {
  SPEC_RULE("Lower-Stmt-Region");

  if (!stmt.body) {
    return EmptyIR();
  }

  // Create default RegionOptions if not provided
  ast::ExprPtr opts_expr = stmt.opts_opt;
  if (!opts_expr) {
    opts_expr = analysis::MakeDefaultRegionOptionsExpr();
  }

  // Lower the options expression
  auto opts_result = LowerExpr(*opts_expr, ctx);

  // Keep user-visible alias binding separate from the internal active-region
  // slot used by implicit allocations and cleanup lowering.
  const std::optional<std::string> user_alias = stmt.alias_opt;
  const std::string runtime_alias =
      user_alias.has_value() ? *user_alias : ctx.FreshRegionAlias();

  // Push scope with is_region=true for region tracking
  ctx.PushScope(false, true);

  // Region statements correspond to RegionNew, so enter the runtime scope
  // before opening the region. This lets Region::new_scoped observe the
  // pushed scope as CurrentScopeId(sigma).
  ctx.RegisterRuntimeScopeExit();
  IRPtr scope_enter_ir = EmptyIR();
  if (const auto scope_id = ctx.CurrentRuntimeScopeId()) {
    scope_enter_ir = EmitRuntimeScopeEnter(*scope_id, ctx);
  }

  // Register the region release action for cleanup
  ctx.RegisterRegionRelease(runtime_alias);

  // Create the modal type for the region: Region@Active
  const auto region_type = analysis::RegionActiveTypeRef();

  // Register the user-visible region alias only when the source declared one.
  if (user_alias.has_value()) {
    ctx.RegisterVar(*user_alias, region_type, false, true);
  }

  // Track this as the active region for allocations
  ctx.active_region_aliases.push_back(runtime_alias);

  // Lower the body block
  auto body_result = LowerBlock(*stmt.body, ctx);

  // Remove from active region tracking
  ctx.active_region_aliases.pop_back();

  // Pop the scope
  ctx.PopScope();

  // Register the result as a temp if needed
  if (ctx.temp_sink) {
    analysis::TypeRef result_type;
    if (stmt.body->tail_opt && ctx.expr_type) {
      result_type = ctx.expr_type(*stmt.body->tail_opt);
    } else if (!stmt.body->tail_opt) {
      result_type = analysis::MakeTypePrim("()");
    }
    ctx.RegisterTempValue(body_result.value, result_type);
  }

  // Create the region IR
  IRRegion region;
  region.owner = opts_result.value;
  region.alias = runtime_alias;
  region.body = body_result.ir;
  region.value = body_result.value;

  return SeqIR({opts_result.ir, scope_enter_ir, MakeIR(std::move(region))});
}

}  // namespace cursive::codegen
