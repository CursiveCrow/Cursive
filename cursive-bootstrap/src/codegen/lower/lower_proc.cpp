#include "cursive0/codegen/lower/lower_proc.h"

#include "cursive0/codegen/abi/abi.h"
#include "cursive0/codegen/checks.h"
#include "cursive0/codegen/cleanup.h"
#include "cursive0/codegen/globals.h"
#include "cursive0/codegen/lower/lower_expr.h"
#include "cursive0/codegen/lower/lower_stmt.h"
#include "cursive0/codegen/layout/layout.h"
#include "cursive0/codegen/mangle.h"
#include "cursive0/core/assert_spec.h"
#include "cursive0/analysis/memory/regions.h"

namespace cursive0::codegen {

using namespace syntax;

namespace {

bool IsUnitType(const analysis::TypeRef& type) {
  if (!type) {
    return false;
  }
  if (const auto* prim = std::get_if<analysis::TypePrim>(&type->node)) {
    return prim->name == "()";
  }
  return false;
}

bool BlockEndsWithReturn(const syntax::Block& block) {
  if (block.stmts.empty()) {
    return false;
  }
  return std::holds_alternative<syntax::ReturnStmt>(block.stmts.back());
}

// Check if [[dynamic]] attribute is present for runtime contract checks
bool HasDynamicContractAttr(const AttributeList& attrs) {
  for (const auto& attr : attrs) {
    if (attr.name == "dynamic") {
      return true;
    }
  }
  return false;
}

// Emit precondition check: if (!pre) panic(ContractPre)
IRPtr EmitPreconditionCheck(const ExprPtr& precond, LowerCtx& ctx) {
  if (!precond) {
    return nullptr;
  }
  SPEC_RULE("Contract-Pre-Check");

  auto pre_result = LowerExpr(*precond, ctx);

  // Emit: if (!precond) panic(ContractPre)
  IRIf check;
  check.cond = pre_result.value;
  check.then_ir = nullptr;  // Pass if true
  check.else_ir = LowerPanic(PanicReason::ContractPre, ctx);  // Panic if false
  check.result = ctx.FreshTempValue("pre_check");

  return SeqIR({pre_result.ir, MakeIR(std::move(check))});
}

// Emit postcondition check: if (!post) panic(ContractPost)
IRPtr EmitPostconditionCheck(const ExprPtr& postcond, LowerCtx& ctx) {
  if (!postcond) {
    return nullptr;
  }
  SPEC_RULE("Contract-Post-Check");

  auto post_result = LowerExpr(*postcond, ctx);

  // Emit: if (!postcond) panic(ContractPost)
  IRIf check;
  check.cond = post_result.value;
  check.then_ir = nullptr;  // Pass if true
  check.else_ir = LowerPanic(PanicReason::ContractPost, ctx);  // Panic if false
  check.result = ctx.FreshTempValue("post_check");

  return SeqIR({post_result.ir, MakeIR(std::move(check))});
}

}  // namespace

ProcIR LowerProc(const ProcedureDecl& decl,
                 const ModulePath& module_path,
                 LowerCtx& ctx) {
  SPEC_RULE("Lower-Proc");

  ProcIR ir;

  // Mangle symbol name
  ir.symbol = MangleProc(module_path, decl);
  ctx.module_path = module_path;

  // Prepare scope context for type lowering
  analysis::ScopeContext scope;
  if (ctx.sigma) {
    scope.sigma = *ctx.sigma;
    scope.current_module = module_path;
  }

  ctx.expr_prov.clear();
  ctx.expr_region.clear();
  if (ctx.sigma && decl.body) {
    const auto prov =
        analysis::ComputeExprProvenanceMap(scope, module_path, decl.params,
                                           decl.body, std::nullopt);
    if (prov.ok) {
      ctx.expr_prov = prov.expr_prov;
      ctx.expr_region = prov.expr_region;
    }
  }

  // Establish function root scope for parameters and cleanup
  ctx.PushScope(false, false);

  // Lower parameters
  for (const auto& param : decl.params) {
    IRParam p;
    p.mode = param.mode.has_value() ? std::optional<analysis::ParamMode>(analysis::ParamMode::Move)
                                   : std::nullopt;
    p.name = param.name;
    if (param.type && ctx.sigma) {
      if (const auto lowered = LowerTypeForLayout(scope, param.type)) {
        p.type = *lowered;
      }
    }
    ir.params.push_back(p);

    const bool has_resp = param.mode.has_value();
    ctx.RegisterVar(param.name, p.type, has_resp, false,
                    analysis::ProvenanceKind::Param);
  }

  // Lower return type
  if (decl.return_type_opt && ctx.sigma) {
    if (const auto lowered = LowerTypeForLayout(scope, decl.return_type_opt)) {
      ir.ret = *lowered;
    }
  }
  if (!ir.ret) {
    ir.ret = analysis::MakeTypePrim("()");
  }

  // Add panic out-parameter when required
  if (NeedsPanicOut(ir.symbol)) {
    IRParam panic_param;
    panic_param.mode = analysis::ParamMode::Move;
    panic_param.name = std::string(kPanicOutName);
    panic_param.type = PanicOutType();
    ir.params.push_back(std::move(panic_param));
  }

  // Check for dynamic contract checks (§5.4)
  const bool has_dynamic_contract = HasDynamicContractAttr(decl.attrs) &&
                                    decl.contract.has_value();

  // Emit precondition check if [[dynamic]] contract present
  IRPtr precond_ir = nullptr;
  if (has_dynamic_contract && decl.contract->precondition) {
    SPEC_RULE("Lower-Proc-ContractPre");
    precond_ir = EmitPreconditionCheck(decl.contract->precondition, ctx);
  }

  // Lower body
  auto body_res = LowerBlock(*decl.body, ctx);

  // Emit postcondition check if [[dynamic]] contract present
  IRPtr postcond_ir = nullptr;
  if (has_dynamic_contract && decl.contract->postcondition) {
    SPEC_RULE("Lower-Proc-ContractPost");
    postcond_ir = EmitPostconditionCheck(decl.contract->postcondition, ctx);
  }

  // Cleanup for parameters on fallthrough
  CleanupPlan cleanup_plan = ComputeCleanupPlanForCurrentScope(ctx);
  IRPtr cleanup_ir = EmitCleanup(cleanup_plan, ctx);
  ctx.PopScope();

  std::vector<IRPtr> body_seq;
  if (decl.name == "main") {
    IRPtr init_ir = EmitInitPlan(ctx.init_order, ctx);
    if (init_ir && !std::holds_alternative<IROpaque>(init_ir->node)) {
      body_seq.push_back(init_ir);
    }
  }

  // Add precondition check at procedure entry
  if (precond_ir) {
    body_seq.push_back(precond_ir);
  }

  if (body_res.ir) {
    body_seq.push_back(body_res.ir);
  }
  const bool ends_with_return = decl.body && BlockEndsWithReturn(*decl.body);
  if (cleanup_ir && !ends_with_return) {
    body_seq.push_back(cleanup_ir);
  }

  const bool has_tail = decl.body && decl.body->tail_opt;
  const bool ret_is_unit = IsUnitType(ir.ret);
  if (has_tail || (!ends_with_return && ret_is_unit)) {
    // Add postcondition check before return
    if (postcond_ir) {
      body_seq.push_back(postcond_ir);
    }
    IRReturn ret;
    ret.value = body_res.value;
    body_seq.push_back(MakeIR(std::move(ret)));
  }

  ir.body = SeqIR(std::move(body_seq));

  return ir;
}

void AnchorProcRules() {
  SPEC_RULE("Lower-Proc");
}

}  // namespace cursive0::codegen
