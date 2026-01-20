#include "cursive0/codegen/lower_proc.h"

#include "cursive0/codegen/abi.h"
#include "cursive0/codegen/cleanup.h"
#include "cursive0/codegen/globals.h"
#include "cursive0/codegen/lower_stmt.h"
#include "cursive0/codegen/layout.h"
#include "cursive0/codegen/mangle.h"
#include "cursive0/core/assert_spec.h"

namespace cursive0::codegen {

using namespace syntax;

ProcIR LowerProc(const ProcedureDecl& decl,
                 const ModulePath& module_path,
                 LowerCtx& ctx) {
  SPEC_RULE("Lower-Proc");

  ProcIR ir;

  // Mangle symbol name
  ir.symbol = MangleProc(module_path, decl);
  ctx.module_path = module_path;

  // Prepare scope context for type lowering
  sema::ScopeContext scope;
  if (ctx.sigma) {
    scope.sigma = *ctx.sigma;
    scope.current_module = module_path;
  }

  // Establish function root scope for parameters and cleanup
  ctx.PushScope(false, false);

  // Lower parameters
  for (const auto& param : decl.params) {
    IRParam p;
    p.mode = param.mode.has_value() ? std::optional<sema::ParamMode>(sema::ParamMode::Move)
                                   : std::nullopt;
    p.name = param.name;
    if (param.type && ctx.sigma) {
      if (const auto lowered = LowerTypeForLayout(scope, param.type)) {
        p.type = *lowered;
      }
    }
    ir.params.push_back(p);

    const bool has_resp = param.mode.has_value();
    ctx.RegisterVar(param.name, p.type, has_resp, false);
  }

  // Lower return type
  if (decl.return_type_opt && ctx.sigma) {
    if (const auto lowered = LowerTypeForLayout(scope, decl.return_type_opt)) {
      ir.ret = *lowered;
    }
  }
  if (!ir.ret) {
    ir.ret = sema::MakeTypePrim("()");
  }

  // Add panic out-parameter when required
  if (NeedsPanicOut(ir.symbol)) {
    IRParam panic_param;
    panic_param.mode = sema::ParamMode::Move;
    panic_param.name = std::string(kPanicOutName);
    panic_param.type = PanicOutType();
    ir.params.push_back(std::move(panic_param));
  }

  // Lower body
  auto body_res = LowerBlock(*decl.body, ctx);

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
  if (body_res.ir) {
    body_seq.push_back(body_res.ir);
  }
  if (cleanup_ir) {
    body_seq.push_back(cleanup_ir);
  }

  IRReturn ret;
  ret.value = body_res.value;
  body_seq.push_back(MakeIR(std::move(ret)));

  ir.body = SeqIR(std::move(body_seq));

  return ir;
}

void AnchorProcRules() {
  SPEC_RULE("Lower-Proc");
}

}  // namespace cursive0::codegen
