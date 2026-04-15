// =============================================================================
// Shadow Var Statement Lowering Implementation
// =============================================================================
//
// SPEC REFERENCE: CursiveSpecification.md Lines 16634-16637 (Lower-Stmt-ShadowVar)
//   - LowerExpr(init) produces IR_i, v
//   - SeqIR(IR_i, BindVarIR(x, v))
//   - Mutable shadow binding
//
// MIGRATED FROM:
//   - cursive-bootstrap/src/04_codegen/lower/lower_stmt.cpp
//   - Lines 661-688: ShadowVarStmt case in LowerStmt dispatch
//
// =============================================================================

#include "05_codegen/lower/stmt/shadow_var_stmt.h"

#include "00_core/assert_spec.h"
#include "04_analysis/typing/context.h"
#include "05_codegen/ir/ir_model.h"
#include "05_codegen/layout/layout.h"
#include "05_codegen/lower/expr/expr_common.h"
#include "05_codegen/lower/lower_expr.h"

namespace cursive::codegen {

namespace {

// Provenance tracking helpers

struct ProvInfo {
  analysis::ProvenanceKind kind = analysis::ProvenanceKind::Bottom;
  std::optional<std::string> region;
};

ProvInfo BindProvInfo(const ProvInfo& init) {
  if (init.kind == analysis::ProvenanceKind::Bottom) {
    return ProvInfo{analysis::ProvenanceKind::Stack, std::nullopt};
  }
  return init;
}

ProvInfo ExprProvInfo(const ast::Expr& expr, const LowerCtx& ctx) {
  ProvInfo info;
  if (auto prov = ctx.LookupExprProv(expr)) {
    info.kind = *prov;
  }
  if (info.kind == analysis::ProvenanceKind::Region) {
    info.region = ctx.LookupExprRegion(expr);
  }
  return info;
}

analysis::TypeRef LowerBindingType(const ast::TypePtr& type_opt,
                                   LowerCtx& ctx) {
  if (!type_opt) {
    return nullptr;
  }
  const analysis::ScopeContext& scope = ScopeForLowering(ctx);
  if (const auto lowered = LowerTypeForLayout(scope, type_opt)) {
    return *lowered;
  }
  return nullptr;
}

}  // namespace

// ============================================================================
// Lower-Stmt-ShadowVar
// ============================================================================
//
// Per the spec (Lines 16634-16637):
//   LowerExpr(init) => <IR_i, v>
//   Result: SeqIR(IR_i, BindVarIR(x, v))
//
// Shadow var bindings introduce a new mutable binding with the same name,
// shadowing any previous binding of that name in the current scope.
// This is structurally identical to LowerShadowLetStmt but registers
// as mutable.
//
IRPtr LowerShadowVarStmt(const ast::ShadowVarStmt& stmt, LowerCtx& ctx) {
  SPEC_RULE("Lower-Stmt-ShadowVar");

  if (!stmt.init) {
    return EmptyIR();
  }

  // Suppress temp registration for the initializer
  auto prev_suppress = ctx.suppress_temp_at_depth;
  ctx.suppress_temp_at_depth = ctx.temp_depth + 1;
  auto init_result = LowerExpr(*stmt.init, ctx);
  ctx.suppress_temp_at_depth = prev_suppress;

  // Determine the binding type
  analysis::TypeRef var_type;
  var_type = LowerBindingType(stmt.type_opt, ctx);
  if (!var_type && ctx.expr_type) {
    var_type = ctx.expr_type(*stmt.init);
  }
  if (!var_type) {
    var_type = ctx.LookupValueType(init_result.value);
  }

  // Compute provenance
  ProvInfo init_prov;
  if (stmt.init) {
    init_prov = ExprProvInfo(*stmt.init, ctx);
  }
  const ProvInfo bind_prov = BindProvInfo(init_prov);

  // Create the binding
  IRBindVar bind;
  bind.name = stmt.name;
  bind.value = init_result.value;
  bind.type = var_type;
  bind.prov = bind_prov.kind;
  bind.prov_region = bind_prov.region;

  // Register the variable as mutable (shadows previous binding of same name)
  ctx.RegisterVar(stmt.name, var_type, true, false, bind_prov.kind,
                  bind_prov.region);

  IRValue checked_value;
  checked_value.kind = IRValue::Kind::Local;
  checked_value.name = stmt.name;
  if (var_type) {
    ctx.RegisterValueType(checked_value, var_type);
  }

  IRPtr log_ir = EmitLogAttributeTrace(stmt.attrs, stmt.span, init_result.value,
                                       var_type, "binding", ctx);
  IRPtr refine_ir = EmitDynamicRefinementChecksForExpr(
      *stmt.init, checked_value, var_type, ctx);
  return SeqIR({init_result.ir, log_ir, MakeIR(std::move(bind)), refine_ir});
}

}  // namespace cursive::codegen
