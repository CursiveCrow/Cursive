#include "cursive0/codegen/lower_expr.h"
#include "cursive0/codegen/checks.h"
#include "cursive0/codegen/abi.h"
#include "cursive0/codegen/dyn_dispatch.h"
#include "cursive0/codegen/layout.h"
#include "cursive0/codegen/cleanup.h"
#include "cursive0/core/assert_spec.h"
#include "cursive0/core/symbols.h"
#include "cursive0/runtime/runtime_interface.h"
#include "cursive0/sema/classes.h"
#include "cursive0/sema/modal.h"
#include "cursive0/sema/modal_transitions.h"
#include "cursive0/sema/record_methods.h"
#include "cursive0/sema/scopes.h"
#include "cursive0/sema/subtyping.h"
#include "cursive0/sema/type_expr.h"

#include <cassert>
#include <algorithm>
#include <set>
#include <variant>

namespace cursive0::codegen {

namespace {

IRValue BoolImmediate(bool value) {
  IRValue v;
  v.kind = IRValue::Kind::Immediate;
  v.name = value ? "true" : "false";
  v.bytes = {static_cast<std::uint8_t>(value ? 1 : 0)};
  return v;
}

static void MergeFailures(LowerCtx& base, const LowerCtx& branch) {
  if (branch.resolve_failed) {
    base.resolve_failed = true;
  }
  if (branch.codegen_failed) {
    base.codegen_failed = true;
  }
  for (const auto& name : branch.resolve_failures) {
    if (std::find(base.resolve_failures.begin(), base.resolve_failures.end(), name) ==
        base.resolve_failures.end()) {
      base.resolve_failures.push_back(name);
    }
  }
}

static void MergeMoveStates(LowerCtx& base,
                            const std::vector<const LowerCtx*>& branches) {
  for (auto& [name, stack] : base.binding_states) {
    if (stack.empty()) {
      continue;
    }
    auto& state = stack.back();

    bool moved_any = state.is_moved;
    std::set<std::string> fields;
    if (!moved_any) {
      fields.insert(state.moved_fields.begin(), state.moved_fields.end());
    }

    for (const auto* branch : branches) {
      if (!branch) {
        continue;
      }
      const BindingState* bstate = branch->GetBindingState(name);
      if (!bstate) {
        continue;
      }
      if (bstate->is_moved) {
        moved_any = true;
      } else if (!moved_any) {
        fields.insert(bstate->moved_fields.begin(), bstate->moved_fields.end());
      }
    }

    if (moved_any) {
      state.is_moved = true;
      state.moved_fields.clear();
    } else {
      state.is_moved = false;
      state.moved_fields.assign(fields.begin(), fields.end());
    }
  }
}

static void MergeLowerCtxTemps(LowerCtx& base, const LowerCtx& branch) {
  for (const auto& [name, type] : branch.value_types) {
    if (!base.value_types.count(name)) {
      base.value_types.emplace(name, type);
    }
  }
  for (const auto& [name, info] : branch.derived_values) {
    if (!base.derived_values.count(name)) {
      base.derived_values.emplace(name, info);
    }
  }
  for (const auto& [name, type] : branch.static_types) {
    if (!base.static_types.count(name)) {
      base.static_types.emplace(name, type);
    }
  }
  for (const auto& [name, type] : branch.drop_glue_types) {
    if (!base.drop_glue_types.count(name)) {
      base.drop_glue_types.emplace(name, type);
    }
  }
  base.temp_counter = std::max(base.temp_counter, branch.temp_counter);
}

static bool IsNoopIR(const IRPtr& ir) {
  return !ir || std::holds_alternative<IROpaque>(ir->node);
}

static bool EndsWithTerminator(const IRPtr& ir) {
  if (!ir) {
    return false;
  }
  return std::visit(
      [&](const auto& node) -> bool {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, IRReturn> ||
                      std::is_same_v<T, IRBreak> ||
                      std::is_same_v<T, IRContinue> ||
                      std::is_same_v<T, IRResult> ||
                      std::is_same_v<T, IRLowerPanic>) {
          return true;
        } else if constexpr (std::is_same_v<T, IRSeq>) {
          for (auto it = node.items.rbegin(); it != node.items.rend(); ++it) {
            if (!IsNoopIR(*it)) {
              return EndsWithTerminator(*it);
            }
          }
          return false;
        } else if constexpr (std::is_same_v<T, IRBlock>) {
          return EndsWithTerminator(node.body);
        } else if constexpr (std::is_same_v<T, IRRegion>) {
          return EndsWithTerminator(node.body);
        } else if constexpr (std::is_same_v<T, IRFrame>) {
          return EndsWithTerminator(node.body);
        } else if constexpr (std::is_same_v<T, IRIf>) {
          return EndsWithTerminator(node.then_ir) && EndsWithTerminator(node.else_ir);
        } else if constexpr (std::is_same_v<T, IRMatch>) {
          for (const auto& arm : node.arms) {
            if (!EndsWithTerminator(arm.body)) {
              return false;
            }
          }
          return !node.arms.empty();
        }
        return false;
      },
      ir->node);
}

static IRPtr CleanupTemps(const std::vector<TempValue>& temps, LowerCtx& ctx) {
  if (temps.empty()) {
    return EmptyIR();
  }

  CleanupPlan plan;
  plan.reserve(temps.size());
  for (auto it = temps.rbegin(); it != temps.rend(); ++it) {
    CleanupAction action;
    action.kind = CleanupAction::Kind::DropTemp;
    action.type = it->type;
    action.value = it->value;
    plan.push_back(std::move(action));
  }
  CleanupPlan remainder = ComputeCleanupPlanToFunctionRoot(ctx);
  return EmitCleanupWithRemainder(plan, remainder, ctx);
}

bool IsPlaceExpr(const syntax::ExprPtr& expr) {
  if (!expr) {
    return false;
  }
  if (std::holds_alternative<syntax::IdentifierExpr>(expr->node)) {
    return true;
  }
  if (std::holds_alternative<syntax::FieldAccessExpr>(expr->node)) {
    return true;
  }
  if (std::holds_alternative<syntax::TupleAccessExpr>(expr->node)) {
    return true;
  }
  if (std::holds_alternative<syntax::IndexAccessExpr>(expr->node)) {
    return true;
  }
  if (const auto* deref = std::get_if<syntax::DerefExpr>(&expr->node)) {
    return IsPlaceExpr(deref->value);
  }
  return false;
}

syntax::ExprPtr MovedArgExpr(const syntax::Arg& arg) {
  if (!arg.moved || !IsPlaceExpr(arg.value)) {
    return arg.value;
  }
  auto moved = std::make_shared<syntax::Expr>();
  moved->span = arg.span.file.empty() && arg.value ? arg.value->span : arg.span;
  moved->node = syntax::MoveExpr{arg.value};
  return moved;
}

ParamModeList ParamModesFromFuncParams(const std::vector<sema::TypeFuncParam>& params) {
  ParamModeList modes;
  modes.reserve(params.size());
  for (const auto& param : params) {
    modes.push_back(param.mode);
  }
  return modes;
}

ParamModeList ParamModesFromParams(const std::vector<syntax::Param>& params) {
  ParamModeList modes;
  modes.reserve(params.size());
  for (const auto& param : params) {
    if (param.mode.has_value()) {
      modes.push_back(sema::ParamMode::Move);
    } else {
      modes.push_back(std::nullopt);
    }
  }
  return modes;
}

std::optional<std::pair<sema::TypePath, std::string>> ModalStateInfo(
    const sema::TypeRef& type) {
  const auto stripped = sema::StripPerm(type);
  if (!stripped) {
    return std::nullopt;
  }
  if (const auto* modal = std::get_if<sema::TypeModalState>(&stripped->node)) {
    return std::make_pair(modal->path, modal->state);
  }
  return std::nullopt;
}

std::optional<sema::TypeRef> SuccessMemberType(const sema::ScopeContext& scope,
                                               const sema::TypeRef& ret_type,
                                               const sema::TypeRef& expr_type) {
  if (!ret_type || !expr_type) {
    return std::nullopt;
  }
  const auto* uni = std::get_if<sema::TypeUnion>(&expr_type->node);
  if (!uni) {
    return std::nullopt;
  }

  std::optional<sema::TypeRef> success;
  for (const auto& member : uni->members) {
    const auto sub = sema::Subtyping(scope, member, ret_type);
    if (!sub.ok) {
      return std::nullopt;
    }
    if (!sub.subtype) {
      if (success.has_value()) {
        return std::nullopt;
      }
      success = member;
    }
  }
  if (!success.has_value()) {
    return std::nullopt;
  }
  for (const auto& member : uni->members) {
    if (member == *success) {
      continue;
    }
    const auto sub = sema::Subtyping(scope, member, ret_type);
    if (!sub.ok || !sub.subtype) {
      return std::nullopt;
    }
  }
  return success;
}

struct RecordCtorInfo {
  syntax::Path path;
  const syntax::RecordDecl* record = nullptr;
};

std::optional<RecordCtorInfo> ResolveRecordCtor(const syntax::ExprPtr& callee,
                                                const std::vector<syntax::Arg>& args,
                                                LowerCtx& ctx) {
  if (!callee || !ctx.sigma || !args.empty()) {
    return std::nullopt;
  }

  auto lookup_record = [&](const syntax::Path& path) -> const syntax::RecordDecl* {
    const auto it = ctx.sigma->types.find(sema::PathKeyOf(path));
    if (it == ctx.sigma->types.end()) {
      return nullptr;
    }
    return std::get_if<syntax::RecordDecl>(&it->second);
  };

  auto make_info = [&](syntax::Path path) -> std::optional<RecordCtorInfo> {
    if (const auto* record = lookup_record(path)) {
      return RecordCtorInfo{std::move(path), record};
    }
    return std::nullopt;
  };

  return std::visit(
      [&](const auto& node) -> std::optional<RecordCtorInfo> {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
          if (node.name == "RegionOptions") {
            syntax::Path path;
            path.emplace_back("RegionOptions");
            if (auto info = make_info(std::move(path))) {
              return info;
            }
          }
          if (ctx.resolve_type_name) {
            if (auto resolved = ctx.resolve_type_name(node.name)) {
              if (!resolved->empty()) {
                return make_info(*resolved);
              }
            }
          }
          syntax::Path path = ctx.module_path;
          path.emplace_back(node.name);
          return make_info(std::move(path));
        } else if constexpr (std::is_same_v<T, syntax::PathExpr>) {
          syntax::Path path = node.path;
          path.emplace_back(node.name);
          return make_info(std::move(path));
        }
        return std::nullopt;
      },
      callee->node);
}

LowerResult LowerRecvArgExpr(const syntax::Expr& base, LowerCtx& ctx) {
  if (std::holds_alternative<syntax::MoveExpr>(base.node)) {
    SPEC_RULE("Lower-RecvArg-Move");
    return LowerExpr(base, ctx);
  }
  SPEC_RULE("Lower-RecvArg-Ref");
  return LowerAddrOf(base, ctx);
}

}  // namespace


// ============================================================================
// §6.4 Call Lowering
// ============================================================================

// §6.4 LowerArgs - lower call arguments (LTR order)
std::pair<IRPtr, std::vector<IRValue>> LowerArgs(
    const ParamModeList& params,
    const std::vector<syntax::Arg>& args,
    LowerCtx& ctx) {
  if (params.empty() && args.empty()) {
    SPEC_RULE("Lower-Args-Empty");
    return {EmptyIR(), {}};
  }

  std::vector<IRPtr> ir_parts;
  std::vector<IRValue> values;

  const bool use_params = params.size() == args.size() && !params.empty();

  for (std::size_t i = 0; i < args.size(); ++i) {
    if (!args[i].value) {
      continue;
    }

    if (!use_params) {
      auto result = LowerExpr(*args[i].value, ctx);
      ir_parts.push_back(result.ir);
      values.push_back(result.value);
      continue;
    }

    if (params[i].has_value()) {
      SPEC_RULE("Lower-Args-Cons-Move");
      auto moved_expr = MovedArgExpr(args[i]);
      auto result = LowerExpr(*moved_expr, ctx);
      ir_parts.push_back(result.ir);
      values.push_back(result.value);
      continue;
    }

    SPEC_RULE("Lower-Args-Cons-Ref");
    auto result = LowerAddrOf(*args[i].value, ctx);
    ir_parts.push_back(result.ir);
    values.push_back(result.value);
  }

  return {SeqIR(std::move(ir_parts)), std::move(values)};
}

// Forward declaration of LowerExpr (defined elsewhere)
LowerResult LowerCallExpr(const syntax::CallExpr& expr, LowerCtx& ctx) {
  SPEC_RULE("Lower-Expr-Call-PanicOut");
  SPEC_RULE("Lower-Expr-Call-NoPanicOut");

  if (auto record_info = ResolveRecordCtor(expr.callee, expr.args, ctx)) {
    SPEC_RULE("Lower-CallIR-RecordCtor");
    std::vector<syntax::FieldInit> field_inits;
    for (const auto& member : record_info->record->members) {
      const auto* field = std::get_if<syntax::FieldDecl>(&member);
      if (!field) {
        continue;
      }
      if (!field->init_opt) {
        ctx.ReportCodegenFailure();
        IRValue bad = ctx.FreshTempValue("record_ctor_err");
        return LowerResult{EmptyIR(), bad};
      }
      syntax::FieldInit init;
      init.name = field->name;
      init.value = field->init_opt;
      init.span = field->span;
      field_inits.push_back(std::move(init));
    }

    std::vector<std::string> saved_module = ctx.module_path;
    if (!record_info->path.empty()) {
      ctx.module_path = record_info->path;
      ctx.module_path.pop_back();
    }

    auto [ir, field_values] = LowerFieldInits(field_inits, ctx, true);

    ctx.module_path = std::move(saved_module);

    IRValue record_value = ctx.FreshTempValue("record_ctor");
    DerivedValueInfo info;
    info.kind = DerivedValueInfo::Kind::RecordLit;
    info.fields = std::move(field_values);
    ctx.RegisterDerivedValue(record_value, info);
    return LowerResult{ir, record_value};
  }

  auto callee_result = LowerExpr(*expr.callee, ctx);

  ParamModeList param_modes;
  if (ctx.expr_type) {
    auto callee_type = ctx.expr_type(*expr.callee);
    if (callee_type) {
      auto stripped = sema::StripPerm(callee_type);
      if (stripped) {
        if (const auto* func = std::get_if<sema::TypeFunc>(&stripped->node)) {
          param_modes = ParamModesFromFuncParams(func->params);
        }
      }
    }
  }
  auto [args_ir, arg_values] = LowerArgs(param_modes, expr.args, ctx);

  IRValue result_value = ctx.FreshTempValue("call");

  IRCall call;
  call.callee = callee_result.value;
  call.args = std::move(arg_values);
  call.result = result_value;

  bool needs_panic_out = true;
  if (call.callee.kind == IRValue::Kind::Symbol) {
    needs_panic_out = NeedsPanicOut(call.callee.name);
    if (needs_panic_out) {
      auto check_builtin = [&](const std::vector<std::string>& full_path,
                               const std::vector<std::string>& container_path) {
        if (full_path.empty()) {
          return;
        }
        const std::string qualified = core::StringOfPath(full_path);
        const std::string builtin = BuiltinSym(qualified);
        const bool is_string_bytes =
            container_path.size() == 1 &&
            (sema::IdEq(container_path[0], "string") ||
             sema::IdEq(container_path[0], "bytes"));
        if (!builtin.empty() || is_string_bytes) {
          needs_panic_out = false;
        }
      };
      if (const auto* path_expr =
              std::get_if<syntax::PathExpr>(&expr.callee->node)) {
        std::vector<std::string> full = path_expr->path;
        full.push_back(path_expr->name);
        check_builtin(full, path_expr->path);
      } else if (const auto* qname =
                     std::get_if<syntax::QualifiedNameExpr>(&expr.callee->node)) {
        std::vector<std::string> full = qname->path;
        full.push_back(qname->name);
        check_builtin(full, qname->path);
      } else if (const auto* ident =
                     std::get_if<syntax::IdentifierExpr>(&expr.callee->node)) {
        if (ctx.resolve_name) {
          if (auto resolved = ctx.resolve_name(ident->name)) {
            std::vector<std::string> container = *resolved;
            if (!container.empty()) {
              container.pop_back();
            }
            check_builtin(*resolved, container);
          }
        }
      }
    }
  }

  if (needs_panic_out) {
    IRValue panic_out;
    panic_out.kind = IRValue::Kind::Local;
    panic_out.name = std::string(kPanicOutName);
    call.args.push_back(panic_out);
    return LowerResult{
        SeqIR({callee_result.ir, args_ir, MakeIR(std::move(call)),
               PanicCheck(ctx)}),
        result_value};
  }

  return LowerResult{
      SeqIR({callee_result.ir, args_ir, MakeIR(std::move(call))}),
      result_value};
}

// §6.4 LowerMethodCall
LowerResult LowerMethodCall(const syntax::MethodCallExpr& expr, LowerCtx& ctx) {
  SPEC_RULE("Lower-Expr-MethodCall");

  sema::TypeRef recv_type;
  if (ctx.expr_type) {
    recv_type = ctx.expr_type(*expr.receiver);
  }
  sema::TypeRef stripped = recv_type ? sema::StripPerm(recv_type) : recv_type;
  const auto* dyn_type = stripped ? std::get_if<sema::TypeDynamic>(&stripped->node) : nullptr;

  ParamModeList param_modes;
  sema::ScopeContext scope;
  if (ctx.sigma) {
    scope.sigma = *ctx.sigma;
    scope.current_module = ctx.module_path;
  }

  if (dyn_type && ctx.sigma) {
    const bool is_builtin = dyn_type->path.size() == 1 && IsBuiltinCapClass(dyn_type->path[0]);
    const auto* class_method = sema::LookupClassMethod(scope, dyn_type->path, expr.name);
    if (class_method) {
      param_modes = ParamModesFromParams(class_method->params);
    }

    if (is_builtin) {
      SPEC_RULE("Lower-MethodCall-Capability");
      auto recv_result = LowerRecvArgExpr(*expr.receiver, ctx);
      auto [args_ir, arg_values] = LowerArgs(param_modes, expr.args, ctx);

      std::vector<IRValue> all_args;
      all_args.push_back(recv_result.value);
      all_args.insert(all_args.end(), arg_values.begin(), arg_values.end());

      std::string callee_sym = expr.name;
      if (auto sym = BuiltinMethodSym(dyn_type->path[0], expr.name)) {
        callee_sym = *sym;
      }

      IRValue result_value = ctx.FreshTempValue("method_call");

      IRCall call;
      call.callee = IRValue{IRValue::Kind::Symbol, callee_sym, {}};
      call.args = std::move(all_args);
      call.result = result_value;

      return LowerResult{SeqIR({recv_result.ir, args_ir, MakeIR(std::move(call))}),
                         result_value};
    }

    const syntax::ClassDecl* class_decl = nullptr;
    const auto class_key = sema::PathKeyOf(dyn_type->path);
    auto it = ctx.sigma->classes.find(class_key);
    if (it != ctx.sigma->classes.end()) {
      class_decl = &it->second;
    }

    if (class_decl) {
      SPEC_RULE("Lower-MethodCall-Dynamic");
      auto recv_result = LowerExpr(*expr.receiver, ctx);
      auto [args_ir, arg_values] = LowerArgs(param_modes, expr.args, ctx);
      IRValue panic_out;
      panic_out.kind = IRValue::Kind::Local;
      panic_out.name = std::string(kPanicOutName);
      arg_values.push_back(panic_out);

      auto dyn_result = LowerDynCall(recv_result.value,
                                     "",
                                     *class_decl,
                                     expr.name,
                                     arg_values,
                                     ctx);

      return LowerResult{SeqIR({recv_result.ir, args_ir, dyn_result.ir}),
                         dyn_result.value};
    }
  }

  if (!dyn_type && ctx.sigma) {
    if (auto modal_info = ModalStateInfo(recv_type)) {
      const auto* modal_decl = sema::LookupModalDecl(scope, modal_info->first);
      if (modal_decl) {
        if (const auto* state_method =
                sema::LookupStateMethodDecl(*modal_decl, modal_info->second, expr.name)) {
          param_modes = ParamModesFromParams(state_method->params);
        } else if (const auto* transition =
                       sema::LookupTransitionDecl(*modal_decl, modal_info->second, expr.name)) {
          param_modes = ParamModesFromParams(transition->params);
        }
      }
    } else if (stripped) {
      const auto lookup = sema::LookupMethodStatic(scope, stripped, expr.name);
      if (lookup.record_method) {
        param_modes = ParamModesFromParams(lookup.record_method->params);
      } else if (lookup.class_method) {
        param_modes = ParamModesFromParams(lookup.class_method->params);
      }
    }
  }

  auto recv_result = LowerRecvArgExpr(*expr.receiver, ctx);
  auto [args_ir, arg_values] = LowerArgs(param_modes, expr.args, ctx);

  std::vector<IRValue> all_args;
  all_args.push_back(recv_result.value);
  all_args.insert(all_args.end(), arg_values.begin(), arg_values.end());

  std::string callee_sym = expr.name;
  if (ctx.sigma && ctx.expr_type) {
    sema::ScopeContext sym_scope;
    sym_scope.sigma = *ctx.sigma;
    sym_scope.current_module = ctx.module_path;
    auto recv_type = ctx.expr_type(*expr.receiver);
    if (recv_type) {
      if (auto sym = MethodSymbol(sym_scope, recv_type, expr.name)) {
        callee_sym = *sym;
      }
    }
  }

  IRValue result_value = ctx.FreshTempValue("method_call");

  IRCall call;
  call.callee = IRValue{IRValue::Kind::Symbol, callee_sym, {}};
  call.args = std::move(all_args);
  call.result = result_value;

  bool needs_panic_out = NeedsPanicOut(callee_sym);

  if (needs_panic_out) {
    SPEC_RULE("Lower-MethodCall-Static-PanicOut");
    IRValue panic_out;
    panic_out.kind = IRValue::Kind::Local;
    panic_out.name = std::string(kPanicOutName);
    call.args.push_back(panic_out);
  } else {
    SPEC_RULE("Lower-MethodCall-Static-NoPanicOut");
  }

  if (needs_panic_out) {
    return LowerResult{
        SeqIR({recv_result.ir, args_ir, MakeIR(std::move(call)),
               PanicCheck(ctx)}),
        result_value};
  }

  return LowerResult{
      SeqIR({recv_result.ir, args_ir, MakeIR(std::move(call))}),
      result_value};
}

// ============================================================================
// §6.4 Unary Operator Lowering
// ============================================================================

// Determine panic reason for unary operator
static PanicReason UnOpPanicReason(const std::string& op) {
  if (op == "-") {
    return PanicReason::Overflow;
  }
  return PanicReason::Other;
}

// §6.4 LowerUnOp
LowerResult LowerUnOp(const std::string& op,
                      const syntax::Expr& operand,
                      LowerCtx& ctx) {
  SPEC_RULE("Lower-UnOp-Ok");
  SPEC_RULE("Lower-UnOp-Panic");

  auto operand_result = LowerExpr(operand, ctx);

  IRValue result_value = ctx.FreshTempValue("unop");

  std::vector<IRPtr> parts;
  parts.push_back(operand_result.ir);

  bool needs_check = (op == "-");
  if (needs_check) {
    IRCheckOp check;
    check.op = op;
    check.reason = PanicReasonString(UnOpPanicReason(op));
    check.lhs = operand_result.value;
    parts.push_back(MakeIR(std::move(check)));
    parts.push_back(PanicCheck(ctx));
  }

  IRUnaryOp unop;
  unop.op = op;
  unop.operand = operand_result.value;
  unop.result = result_value;
  parts.push_back(MakeIR(std::move(unop)));

  return LowerResult{SeqIR(std::move(parts)), result_value};
}

// ============================================================================
// §6.4 Binary Operator Lowering
// ============================================================================

// Determine panic reason for binary operator
static PanicReason BinOpPanicReason(const std::string& op) {
  if (op == "/" || op == "%") {
    return PanicReason::DivZero;  // Or Overflow for INT_MIN / -1
  }
  if (op == "<<" || op == ">>") {
    return PanicReason::Shift;
  }
  if (op == "+" || op == "-" || op == "*" || op == "**") {
    return PanicReason::Overflow;
  }
  return PanicReason::Other;
}

// §6.4 LowerBinOp - for non-short-circuit operators
LowerResult LowerBinOp(const std::string& op,
                       const syntax::Expr& lhs,
                       const syntax::Expr& rhs,
                       LowerCtx& ctx) {
  SPEC_RULE("Lower-BinOp-Ok");
  SPEC_RULE("Lower-BinOp-Panic");

  auto lhs_result = LowerExpr(lhs, ctx);
  auto rhs_result = LowerExpr(rhs, ctx);

  IRValue result_value = ctx.FreshTempValue("binop");

  std::vector<IRPtr> parts;
  parts.push_back(lhs_result.ir);
  parts.push_back(rhs_result.ir);

  bool needs_check =
      (op == "/" || op == "%" || op == "<<" || op == ">>" || op == "+" ||
       op == "-" || op == "*" || op == "**");

  if (needs_check) {
    IRCheckOp check;
    check.op = op;
    check.reason = PanicReasonString(BinOpPanicReason(op));
    check.lhs = lhs_result.value;
    check.rhs = rhs_result.value;
    parts.push_back(MakeIR(std::move(check)));
    parts.push_back(PanicCheck(ctx));
  }

  IRBinaryOp binop;
  binop.op = op;
  binop.lhs = lhs_result.value;
  binop.rhs = rhs_result.value;
  binop.result = result_value;
  parts.push_back(MakeIR(std::move(binop)));

  return LowerResult{SeqIR(std::move(parts)), result_value};
}

// §6.4 Lower-Expr-Bin-And (short-circuit)
LowerResult LowerBinAnd(const syntax::Expr& lhs,
                        const syntax::Expr& rhs,
                        LowerCtx& ctx) {
  SPEC_RULE("Lower-Expr-Bin-And");

  auto lhs_result = LowerExpr(lhs, ctx);
  LowerCtx rhs_ctx = ctx;
  auto rhs_result = LowerExpr(rhs, rhs_ctx);
  MergeLowerCtxTemps(ctx, rhs_ctx);
  MergeMoveStates(ctx, {&rhs_ctx});
  MergeFailures(ctx, rhs_ctx);

  // Short-circuit: if lhs is false, result is false (don't evaluate rhs)

  IRValue result_value = ctx.FreshTempValue("and");

  IRIf if_ir;
  if_ir.cond = lhs_result.value;
  if_ir.then_ir = rhs_result.ir;
  if_ir.then_value = rhs_result.value;
  if_ir.else_ir = EmptyIR();
  if_ir.else_value = BoolImmediate(false);
  if_ir.result = result_value;

  return LowerResult{SeqIR({lhs_result.ir, MakeIR(std::move(if_ir))}),
                     result_value};
}

// §6.4 Lower-Expr-Bin-Or (short-circuit)
LowerResult LowerBinOr(const syntax::Expr& lhs,
                       const syntax::Expr& rhs,
                       LowerCtx& ctx) {
  SPEC_RULE("Lower-Expr-Bin-Or");

  auto lhs_result = LowerExpr(lhs, ctx);
  LowerCtx rhs_ctx = ctx;
  auto rhs_result = LowerExpr(rhs, rhs_ctx);
  MergeLowerCtxTemps(ctx, rhs_ctx);
  MergeMoveStates(ctx, {&rhs_ctx});
  MergeFailures(ctx, rhs_ctx);

  // Short-circuit: if lhs is true, result is true (don't evaluate rhs)
  IRValue result_value = ctx.FreshTempValue("or");

  IRIf if_ir;
  if_ir.cond = lhs_result.value;
  if_ir.then_ir = EmptyIR();
  if_ir.then_value = BoolImmediate(true);
  if_ir.else_ir = rhs_result.ir;
  if_ir.else_value = rhs_result.value;
  if_ir.result = result_value;

  return LowerResult{SeqIR({lhs_result.ir, MakeIR(std::move(if_ir))}),
                     result_value};
}

// Dispatch binary expression lowering
LowerResult LowerBinaryExpr(const syntax::BinaryExpr& expr, LowerCtx& ctx) {
  const std::string& op = expr.op;

  // Handle short-circuit operators specially
  if (op == "&&") {
    return LowerBinAnd(*expr.lhs, *expr.rhs, ctx);
  }
  if (op == "||") {
    return LowerBinOr(*expr.lhs, *expr.rhs, ctx);
  }

  // All other operators
  SPEC_RULE("Lower-Expr-Binary");
  return LowerBinOp(op, *expr.lhs, *expr.rhs, ctx);
}

// ============================================================================
// §6.4 Cast Lowering
// ============================================================================

// §6.4 LowerCast
LowerResult LowerCast(const syntax::Expr& expr,
                      sema::TypeRef target_type,
                      LowerCtx& ctx) {
  SPEC_RULE("Lower-Cast");
  SPEC_RULE("Lower-Cast-Panic");

  if (target_type) {
    auto stripped_target = sema::StripPerm(target_type);
    if (stripped_target && std::holds_alternative<sema::TypeDynamic>(stripped_target->node)) {
      SPEC_RULE("Eval-Dynamic-Form");
      SPEC_RULE("Eval-Dynamic-Form-Ctrl");

      const auto* dyn = std::get_if<sema::TypeDynamic>(&stripped_target->node);
      if (!dyn || !ctx.sigma || !ctx.expr_type) {
        ctx.ReportCodegenFailure();
        IRValue dyn_value = ctx.FreshTempValue("dyn");
        return LowerResult{EmptyIR(), dyn_value};
      }

      sema::TypeRef expr_type = ctx.expr_type(expr);
      sema::TypeRef stripped_expr = expr_type ? sema::StripPerm(expr_type) : expr_type;
      const auto class_it = ctx.sigma->classes.find(sema::PathKeyOf(dyn->path));
      if (!stripped_expr || class_it == ctx.sigma->classes.end()) {
        ctx.ReportCodegenFailure();
        IRValue dyn_value = ctx.FreshTempValue("dyn");
        return LowerResult{EmptyIR(), dyn_value};
      }

      DynPackResult pack = DynPack(stripped_expr, expr, dyn->path, class_it->second, ctx);

      IRValue dyn_value = ctx.FreshTempValue("dyn");
      DerivedValueInfo info;
      info.kind = DerivedValueInfo::Kind::DynLit;
      info.base = pack.data_ptr;
      info.vtable_sym = pack.vtable_sym;
      ctx.RegisterDerivedValue(dyn_value, info);

      return LowerResult{pack.ir, dyn_value};
    }
  }

  auto expr_result = LowerExpr(expr, ctx);

  IRValue result_value = ctx.FreshTempValue("cast");

  if (!target_type) {
    return LowerResult{expr_result.ir, result_value};
  }

  std::vector<IRPtr> parts;
  parts.push_back(expr_result.ir);

  IRCheckCast check;
  check.target = target_type;
  check.value = expr_result.value;
  parts.push_back(MakeIR(std::move(check)));
  parts.push_back(PanicCheck(ctx));

  IRCast cast;
  cast.target = target_type;
  cast.value = expr_result.value;
  cast.result = result_value;
  parts.push_back(MakeIR(std::move(cast)));

  return LowerResult{SeqIR(std::move(parts)), result_value};
}

LowerResult LowerCastExpr(const syntax::CastExpr& expr, LowerCtx& ctx) {
  SPEC_RULE("Lower-Expr-Cast");

  sema::TypeRef target_type;
  if (expr.type && ctx.sigma) {
    sema::ScopeContext scope;
    scope.sigma = *ctx.sigma;
    if (auto lowered = LowerTypeForLayout(scope, expr.type)) {
      target_type = *lowered;
    }
  }
  if (!target_type && ctx.expr_type) {
    syntax::Expr wrapped{expr.value->span, expr};
    target_type = ctx.expr_type(wrapped);
  }

  return LowerCast(*expr.value, target_type, ctx);
}

// ============================================================================
// §6.4 Propagate Expression Lowering
// ============================================================================

LowerResult LowerPropagateExpr(const syntax::PropagateExpr& expr,
                               LowerCtx& ctx) {
  SPEC_RULE("Lower-Expr-Propagate-Success");
  SPEC_RULE("Lower-Expr-Propagate-Return");

  auto inner_result = LowerExpr(*expr.value, ctx);

  std::optional<sema::TypeRef> success_type;
  if (ctx.expr_type && ctx.proc_ret_type && ctx.sigma) {
    sema::ScopeContext scope;
    scope.sigma = *ctx.sigma;
    scope.current_module = ctx.module_path;
    auto expr_type = ctx.expr_type(*expr.value);
    success_type = SuccessMemberType(scope, ctx.proc_ret_type, expr_type);
  }

  std::string suffix = inner_result.value.name;
  if (suffix.empty()) {
    suffix = "value";
  }
  std::string tag = success_type ? suffix : "unknown_" + suffix;

  IRValue cond = ctx.FreshTempValue("propagate_is_success_" + tag);
  IRValue result_value = ctx.FreshTempValue("propagate_result");
  IRValue success_value = result_value;
  IRValue error_value = ctx.FreshTempValue("propagate_error_" + tag);

  std::vector<IRPtr> error_parts;

  CleanupPlan cleanup_plan = ComputeCleanupPlanToFunctionRoot(ctx);
  error_parts.push_back(EmitCleanup(cleanup_plan, ctx));

  IRReturn ret;
  ret.value = error_value;
  error_parts.push_back(MakeIR(std::move(ret)));

  IRIf if_ir;
  if_ir.cond = cond;
  if_ir.then_ir = EmptyIR();
  if_ir.then_value = success_value;
  if_ir.else_ir = SeqIR(std::move(error_parts));

  IRValue unreach_value = ctx.FreshTempValue("propagate_unreach");
  if_ir.else_value = unreach_value;

  return LowerResult{SeqIR({inner_result.ir, MakeIR(std::move(if_ir))}),
                     result_value};
}

// ============================================================================
// §6.4 Control Flow Expression Lowering
// ============================================================================

LowerResult LowerIfExpr(const syntax::IfExpr& expr, LowerCtx& ctx) {
  SPEC_RULE("Lower-Expr-If");

  // Lower condition
  auto* prev_sink = ctx.temp_sink;
  std::vector<TempValue> cond_temps;
  ctx.temp_sink = &cond_temps;
  auto prev_suppress = ctx.suppress_temp_at_depth;
  ctx.suppress_temp_at_depth = ctx.temp_depth + 1;
  auto cond_result = LowerExpr(*expr.cond, ctx);
  ctx.suppress_temp_at_depth = prev_suppress;
  ctx.temp_sink = prev_sink;
  IRPtr cond_cleanup = CleanupTemps(cond_temps, ctx);
  if (IsNoopIR(cond_cleanup)) {
    cond_cleanup = EmptyIR();
  }

  // Lower then branch
  LowerCtx then_ctx = ctx;
  std::vector<TempValue> then_temps;
  then_ctx.temp_sink = &then_temps;
  auto then_prev_suppress = then_ctx.suppress_temp_at_depth;
  then_ctx.suppress_temp_at_depth = then_ctx.temp_depth + 1;
  auto then_result = LowerExpr(*expr.then_expr, then_ctx);
  then_ctx.suppress_temp_at_depth = then_prev_suppress;
  IRPtr then_cleanup = CleanupTemps(then_temps, then_ctx);
  if (!IsNoopIR(then_cleanup) && !EndsWithTerminator(then_result.ir)) {
    then_result.ir = SeqIR({then_result.ir, then_cleanup});
  }

  // Lower else branch (if present)
  LowerCtx else_ctx = ctx;
  else_ctx.temp_counter = std::max(else_ctx.temp_counter, then_ctx.temp_counter);
  LowerResult else_result;
  if (expr.else_expr) {
    std::vector<TempValue> else_temps;
    else_ctx.temp_sink = &else_temps;
    auto else_prev_suppress = else_ctx.suppress_temp_at_depth;
    else_ctx.suppress_temp_at_depth = else_ctx.temp_depth + 1;
    else_result = LowerExpr(*expr.else_expr, else_ctx);
    else_ctx.suppress_temp_at_depth = else_prev_suppress;
    IRPtr else_cleanup = CleanupTemps(else_temps, else_ctx);
    if (!IsNoopIR(else_cleanup) && !EndsWithTerminator(else_result.ir)) {
      else_result.ir = SeqIR({else_result.ir, else_cleanup});
    }
  } else {
    else_result.ir = EmptyIR();
    else_result.value = IRValue{IRValue::Kind::Opaque, "unit", {}};
  }

  MergeLowerCtxTemps(ctx, then_ctx);
  MergeLowerCtxTemps(ctx, else_ctx);
  MergeMoveStates(ctx, {&then_ctx, &else_ctx});
  MergeFailures(ctx, then_ctx);
  MergeFailures(ctx, else_ctx);

  // Create if IR
  IRValue result_value = ctx.FreshTempValue("if");

  IRIf if_ir;
  if_ir.cond = cond_result.value;
  if_ir.then_ir = then_result.ir;
  if_ir.then_value = then_result.value;
  if_ir.else_ir = else_result.ir;
  if_ir.else_value = else_result.value;
  if_ir.result = result_value;

  return LowerResult{SeqIR({cond_result.ir, cond_cleanup, MakeIR(std::move(if_ir))}),
                     result_value};
}

// ============================================================================
// Anchor function
// ============================================================================

void AnchorCallsRules() {
  SPEC_RULE("Lower-Expr-Call-PanicOut");
  SPEC_RULE("Lower-Expr-Call-NoPanicOut");
  SPEC_RULE("Lower-Expr-MethodCall");
  SPEC_RULE("Lower-UnOp-Ok");
  SPEC_RULE("Lower-UnOp-Panic");
  SPEC_RULE("Lower-BinOp-Ok");
  SPEC_RULE("Lower-BinOp-Panic");
  SPEC_RULE("Lower-Expr-Bin-And");
  SPEC_RULE("Lower-Expr-Bin-Or");
  SPEC_RULE("Lower-Expr-Binary");
  SPEC_RULE("Lower-Cast");
  SPEC_RULE("Lower-Cast-Panic");
  SPEC_RULE("Lower-Expr-Cast");
  SPEC_RULE("Lower-Expr-Propagate-Success");
  SPEC_RULE("Lower-Expr-Propagate-Return");
  SPEC_RULE("Lower-Expr-If");
}

}  // namespace cursive0::codegen
