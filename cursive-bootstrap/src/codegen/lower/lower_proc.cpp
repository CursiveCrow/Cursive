#include "cursive0/codegen/lower/lower_proc.h"

#include "cursive0/codegen/abi/abi.h"
#include "cursive0/codegen/cleanup.h"
#include "cursive0/codegen/globals.h"
#include "cursive0/codegen/lower/lower_stmt.h"
#include "cursive0/codegen/layout/layout.h"
#include "cursive0/codegen/mangle.h"
#include "cursive0/core/assert_spec.h"

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
    ctx.RegisterVar(param.name, p.type, has_resp, false);
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
  const bool ends_with_return = decl.body && BlockEndsWithReturn(*decl.body);
  if (cleanup_ir && !ends_with_return) {
    body_seq.push_back(cleanup_ir);
  }

  const bool has_tail = decl.body && decl.body->tail_opt;
  const bool ret_is_unit = IsUnitType(ir.ret);
  if (has_tail || (!ends_with_return && ret_is_unit)) {
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
