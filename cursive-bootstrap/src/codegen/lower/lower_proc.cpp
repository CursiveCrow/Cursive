#include "cursive0/codegen/lower/lower_proc.h"

#include "cursive0/codegen/abi/abi.h"
#include "cursive0/codegen/checks.h"
#include "cursive0/codegen/cleanup.h"
#include "cursive0/codegen/globals.h"
#include "cursive0/codegen/lower/lower_expr.h"
#include "cursive0/codegen/lower/lower_stmt.h"
#include "cursive0/codegen/layout/layout.h"
#include "cursive0/codegen/async_frame.h"
#include "cursive0/codegen/mangle.h"
#include "cursive0/core/assert_spec.h"
#include "cursive0/analysis/memory/regions.h"
#include "cursive0/analysis/types/types.h"

#include <unordered_map>

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

// §19.1.3 Check if procedure returns an async type
bool IsAsyncProc(const analysis::TypeRef& ret_type) {
  return analysis::IsAsyncType(ret_type);
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

std::uint64_t AlignUp(std::uint64_t value, std::uint64_t align) {
  if (align == 0) {
    return value;
  }
  const std::uint64_t rem = value % align;
  if (rem == 0) {
    return value;
  }
  return value + (align - rem);
}

struct AsyncIRInfo {
  std::size_t next_state = 1;
  std::unordered_map<std::string, analysis::TypeRef> slot_types;
  std::vector<std::string> slot_order;
};

void AddAsyncSlot(AsyncIRInfo& info,
                  const std::string& name,
                  const analysis::TypeRef& type) {
  if (!type) {
    return;
  }
  if (info.slot_types.emplace(name, type).second) {
    info.slot_order.push_back(name);
  }
}

void CollectAsyncIR(IRPtr& ir, AsyncIRInfo& info) {
  if (!ir) {
    return;
  }
  std::visit(
      [&](auto& node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, IRYield>) {
          node.state_index = info.next_state++;
        } else if constexpr (std::is_same_v<T, IRYieldFrom>) {
          node.state_index = info.next_state++;
        } else if constexpr (std::is_same_v<T, IRBindVar>) {
          AddAsyncSlot(info, node.name, node.type);
        } else if constexpr (std::is_same_v<T, IRSeq>) {
          for (auto& item : node.items) {
            CollectAsyncIR(item, info);
          }
        } else if constexpr (std::is_same_v<T, IRIf>) {
          CollectAsyncIR(node.then_ir, info);
          CollectAsyncIR(node.else_ir, info);
        } else if constexpr (std::is_same_v<T, IRBlock>) {
          CollectAsyncIR(node.setup, info);
          CollectAsyncIR(node.body, info);
        } else if constexpr (std::is_same_v<T, IRLoop>) {
          CollectAsyncIR(node.iter_ir, info);
          CollectAsyncIR(node.cond_ir, info);
          CollectAsyncIR(node.body_ir, info);
        } else if constexpr (std::is_same_v<T, IRMatch>) {
          for (auto& arm : node.arms) {
            CollectAsyncIR(arm.body, info);
          }
        } else if constexpr (std::is_same_v<T, IRRegion>) {
          CollectAsyncIR(node.body, info);
        } else if constexpr (std::is_same_v<T, IRFrame>) {
          CollectAsyncIR(node.body, info);
        } else if constexpr (std::is_same_v<T, IRPanicCheck>) {
          CollectAsyncIR(node.cleanup_ir, info);
        } else if constexpr (std::is_same_v<T, IRLowerPanic>) {
          CollectAsyncIR(node.cleanup_ir, info);
        } else if constexpr (std::is_same_v<T, IRParallel>) {
          CollectAsyncIR(node.body, info);
        } else if constexpr (std::is_same_v<T, IRSpawn>) {
          CollectAsyncIR(node.captured_env, info);
          CollectAsyncIR(node.body, info);
        } else if constexpr (std::is_same_v<T, IRDispatch>) {
          CollectAsyncIR(node.captured_env, info);
          CollectAsyncIR(node.body, info);
        } else if constexpr (std::is_same_v<T, IRRaceReturn>) {
          for (auto& arm : node.arms) {
            CollectAsyncIR(arm.async_ir, info);
            CollectAsyncIR(arm.handler_ir, info);
          }
        } else if constexpr (std::is_same_v<T, IRRaceYield>) {
          for (auto& arm : node.arms) {
            CollectAsyncIR(arm.async_ir, info);
            CollectAsyncIR(arm.handler_ir, info);
          }
        } else if constexpr (std::is_same_v<T, IRAll>) {
          for (auto& item : node.async_irs) {
            CollectAsyncIR(item, info);
          }
        }
      },
      ir->node);
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

  // Set proc_ret_type for async procedure detection in return statement lowering
  ctx.proc_ret_type = ir.ret;

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

  if (IsAsyncProc(ir.ret)) {
    const auto sig = analysis::GetAsyncSig(ir.ret);
    if (sig.has_value()) {
      AsyncIRInfo async_ir;
      std::vector<std::string> param_names;
      for (const auto& param : ir.params) {
        if (param.name == kPanicOutName) {
          continue;
        }
        param_names.push_back(param.name);
        AddAsyncSlot(async_ir, param.name, param.type);
      }
      CollectAsyncIR(ir.body, async_ir);

      analysis::ScopeContext scope;
      if (ctx.sigma) {
        scope.sigma = *ctx.sigma;
        scope.current_module = module_path;
      }

      LowerCtx::AsyncProcInfo async_info;
      async_info.async_type = ir.ret;
      async_info.out_type = sig->out;
      async_info.in_type = sig->in;
      async_info.result_type = sig->result;
      async_info.err_type = sig->err;
      async_info.resume_symbol = ir.symbol + "$resume";
      async_info.resume_needs_panic_out = NeedsPanicOut(async_info.resume_symbol);
      async_info.param_names = param_names;

      std::uint64_t offset = kAsyncFrameHeaderSize;
      std::uint64_t frame_align = kAsyncFrameHeaderAlign;
      for (const auto& name : async_ir.slot_order) {
        const auto it = async_ir.slot_types.find(name);
        if (it == async_ir.slot_types.end()) {
          continue;
        }
        const auto& type = it->second;
        const auto size_opt = SizeOf(scope, type);
        const auto align_opt = AlignOf(scope, type);
        if (!size_opt.has_value()) {
          ctx.ReportCodegenFailure();
          continue;
        }
        const std::uint64_t size = *size_opt;
        const std::uint64_t align = align_opt.value_or(1);
        offset = AlignUp(offset, align);
        LowerCtx::AsyncFrameSlot slot;
        slot.type = type;
        slot.offset = offset;
        slot.size = size;
        slot.align = align;
        async_info.slots[name] = slot;
        offset += size;
        frame_align = std::max(frame_align, align);
      }
      async_info.frame_align = frame_align;
      async_info.frame_size = AlignUp(offset, frame_align);

      LowerCtx::AsyncProcInfo wrapper_info = async_info;
      wrapper_info.is_wrapper = true;
      wrapper_info.is_resume = false;

      LowerCtx::AsyncProcInfo resume_info = async_info;
      resume_info.is_wrapper = false;
      resume_info.is_resume = true;

      ctx.async_procs[ir.symbol] = std::move(wrapper_info);
      ctx.async_procs[async_info.resume_symbol] = std::move(resume_info);

      ProcIR resume = ir;
      resume.symbol = async_info.resume_symbol;
      resume.params.clear();

      IRParam frame_param;
      frame_param.mode = analysis::ParamMode::Move;
      frame_param.name = "__c0_async_frame";
      frame_param.type = analysis::MakeTypePtr(analysis::MakeTypePrim("u8"),
                                               analysis::PtrState::Valid);
      resume.params.push_back(frame_param);

      IRParam input_param;
      input_param.mode = analysis::ParamMode::Move;
      input_param.name = "__c0_async_input";
      input_param.type = analysis::MakeTypePtr(analysis::MakeTypePrim("u8"),
                                               analysis::PtrState::Valid);
      resume.params.push_back(input_param);

      if (async_info.resume_needs_panic_out) {
        IRParam panic_param;
        panic_param.mode = analysis::ParamMode::Move;
        panic_param.name = std::string(kPanicOutName);
        panic_param.type = PanicOutType();
        resume.params.push_back(std::move(panic_param));
      }

      ctx.RegisterProcSig(resume);
      ctx.extra_procs.push_back(std::move(resume));
    }
  }

  return ir;
}

void AnchorProcRules() {
  SPEC_RULE("Lower-Proc");
}

}  // namespace cursive0::codegen
