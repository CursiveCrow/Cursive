// =============================================================================
// Expression Lowering: MethodCallExpr
// =============================================================================
//
// SPEC REFERENCE: CursiveSpecification.md Section 6.4 (Expression Lowering)
//   - Lines 16153-16156: (Lower-Expr-MethodCall)
//     Gamma |- LowerMethodCall(...) => <IR, v_call>
//   - Section 6.10: Dynamic Dispatch (Lines 17161-17202)
//
// MIGRATED FROM:
//   - cursive-bootstrap/src/04_codegen/lower/lower_expr_calls.cpp
//   - Lines 527-736: LowerMethodCall function
//
// =============================================================================

#include "05_codegen/lower/expr/method_call.h"
#include "05_codegen/abi/abi.h"
#include "05_codegen/checks/checks.h"
#include "05_codegen/cleanup/unwind.h"
#include "05_codegen/dyn_dispatch/dyn_dispatch.h"
#include "05_codegen/intrinsics/builtins.h"
#include "05_codegen/intrinsics/intrinsics_interface.h"
#include "05_codegen/lower/expr/call.h"
#include "05_codegen/lower/expr/expr_common.h"
#include "05_codegen/symbols/mangle.h"
#include "04_analysis/caps/cap_concurrency.h"
#include "04_analysis/caps/cap_filesystem.h"
#include "04_analysis/caps/cap_heap.h"
#include "04_analysis/caps/cap_network.h"
#include "04_analysis/caps/cap_system.h"
#include "04_analysis/composite/classes.h"
#include "04_analysis/composite/record_methods.h"
#include "04_analysis/modal/builtin_modal_intrinsics.h"
#include "04_analysis/modal/modal.h"
#include "04_analysis/modal/modal_transitions.h"
#include "04_analysis/memory/string_bytes.h"
#include "04_analysis/typing/type_expr.h"
#include "04_analysis/memory/calls.h"
#include "00_core/assert_spec.h"
#include "00_core/symbols.h"

#include <algorithm>
#include <variant>

namespace cursive::codegen {

namespace {

// Extract parameter modes from function parameters
ParamModeList ParamModesFromFuncParams(const std::vector<analysis::TypeFuncParam>& params) {
    ParamModeList modes;
    modes.reserve(params.size());
    for (const auto& param : params) {
        modes.push_back(param.mode);
    }
    return modes;
}

// Extract parameter modes from syntax parameters
ParamModeList ParamModesFromParams(const std::vector<ast::Param>& params) {
    ParamModeList modes;
    modes.reserve(params.size());
    for (const auto& param : params) {
        if (param.mode.has_value()) {
            modes.push_back(analysis::ParamMode::Move);
        } else {
            modes.push_back(std::nullopt);
        }
    }
    return modes;
}

// Extract modal state info from a type
std::optional<std::pair<analysis::TypePath, std::string>> ModalStateInfo(
    const analysis::TypeRef& type) {
    const auto stripped = analysis::StripPerm(type);
    if (!stripped) {
        return std::nullopt;
    }
    if (const auto* modal = std::get_if<analysis::TypeModalState>(&stripped->node)) {
        return std::make_pair(modal->path, modal->state);
    }
    return std::nullopt;
}

const ast::Expr* UnwrapReceiverExpr(const ast::ExprPtr& expr) {
    const ast::Expr* current = expr.get();
    while (current) {
        if (const auto* attr = std::get_if<ast::AttributedExpr>(&current->node)) {
            current = attr->expr.get();
            continue;
        }
        if (const auto* move_expr = std::get_if<ast::MoveExpr>(&current->node)) {
            current = move_expr->place.get();
            continue;
        }
        break;
    }
    return current;
}

std::optional<std::string> ReceiverRootName(const ast::ExprPtr& receiver) {
    const ast::Expr* unwrapped = UnwrapReceiverExpr(receiver);
    if (!unwrapped) {
        return std::nullopt;
    }
    if (const auto* ident = std::get_if<ast::IdentifierExpr>(&unwrapped->node)) {
        return ident->name;
    }
    return std::nullopt;
}

analysis::TypeRef ReceiverBindingType(const ast::ExprPtr& receiver,
                                      const LowerCtx& ctx) {
    const auto root = ReceiverRootName(receiver);
    if (!root.has_value()) {
        return nullptr;
    }
    const BindingState* state = ctx.GetBindingState(*root);
    if (!state) {
        return nullptr;
    }
    return state->type;
}

// Materialize a temporary for non-`move` receiver expressions that do not
// already carry source provenance and must be passed by reference.
LowerResult LowerRefReceiverWithTemp(const ast::ExprPtr& expr, LowerCtx& ctx) {
    if (!expr) {
        return LowerResult{EmptyIR(), IRValue{}};
    }

    if (analysis::HasSourceProvenance(expr)) {
        return LowerAddrOf(*expr, ctx);
    }

    auto value_result = LowerExpr(*expr, ctx);
    std::string temp_name = ctx.FreshTempValue("method_recv_tmp").name;

    analysis::TypeRef temp_type = ctx.LookupValueType(value_result.value);
    if (!temp_type && ctx.expr_type) {
        temp_type = ctx.expr_type(*expr);
    }
    if (!temp_type) {
        temp_type = analysis::MakeTypePrim("()");
    }

    IRBindVar bind;
    bind.name = temp_name;
    bind.value = value_result.value;
    bind.type = temp_type;
    bind.prov = analysis::ProvenanceKind::Stack;

    ctx.RegisterVar(temp_name, temp_type, false, false,
                    analysis::ProvenanceKind::Stack, std::nullopt);

    ast::Expr temp_ident;
    temp_ident.span = expr->span;
    temp_ident.node = ast::IdentifierExpr{temp_name};
    auto addr_result = LowerAddrOf(temp_ident, ctx);

    return LowerResult{SeqIR({value_result.ir, MakeIR(std::move(bind)), addr_result.ir}),
                       addr_result.value};
}

// Lower receiver argument expression - either by reference or by move
LowerResult LowerRecvArgExpr(const ast::ExprPtr& base, LowerCtx& ctx) {
    if (!base) {
        return LowerResult{EmptyIR(), IRValue{}};
    }
    if (std::holds_alternative<ast::MoveExpr>(base->node)) {
        SPEC_RULE("Lower-RecvArg-Move");
        return LowerExpr(*base, ctx);
    }
    SPEC_RULE("Lower-RecvArg-Ref");
    return LowerRefReceiverWithTemp(base, ctx);
}

std::optional<ParamModeList> BuiltinCapabilityParamModes(
    const analysis::TypePath& cap_path,
    std::string_view method_name) {
    ast::ClassPath class_path;
    class_path.reserve(cap_path.size());
    for (const auto& seg : cap_path) {
        class_path.push_back(seg);
    }

    if (analysis::IsFileSystemClassPath(class_path)) {
        if (const auto sig = analysis::LookupFileSystemMethodSig(method_name)) {
            return ParamModesFromParams(sig->params);
        }
        return std::nullopt;
    }

    if (analysis::IsHeapAllocatorClassPath(class_path)) {
        if (const auto sig = analysis::LookupHeapAllocatorMethodSig(method_name)) {
            return ParamModesFromParams(sig->params);
        }
        return std::nullopt;
    }

    if (analysis::IsNetworkClassPath(class_path)) {
        if (const auto sig = analysis::LookupNetworkMethodSig(method_name)) {
            return ParamModesFromParams(sig->params);
        }
        return std::nullopt;
    }

  if (analysis::IsExecutionDomainTypePath(cap_path)) {
    if (const auto sig = analysis::LookupExecutionDomainMethodSig(method_name)) {
      return ParamModesFromParams(sig->params);
    }
    return std::nullopt;
  }

  if (analysis::IsSystemTypePath(cap_path)) {
    if (const auto sig = analysis::LookupSystemMethodSig(method_name)) {
      return ParamModesFromParams(sig->params);
    }
    return std::nullopt;
  }

  return std::nullopt;
}

std::optional<std::string> AsyncCombinatorRuntimeSymbol(std::string_view name) {
    const analysis::TypePath async_modal_path = {"Async"};
    if (!analysis::IsBuiltinModalGeneralMember(async_modal_path, name)) {
        return std::nullopt;
    }
    return analysis::LookupBuiltinModalRuntimeSymbol(
        async_modal_path,
        std::nullopt,
        name);
}

}  // namespace

// =============================================================================
// LowerMethodCall - Lower a method call expression to IR
// =============================================================================
// SPEC: (Lower-Expr-MethodCall)
//   Gamma |- LowerRecvArg(recv) => <IR_r, v_recv>
//   Gamma |- LowerArgs(args) => <IR_a, vs>
//   ResolveMethod(recv_type, name) = symbol
//   --------------------------------------------------------
//   Gamma |- LowerExpr(recv~>name(args)) => <SeqIR(IR_r, IR_a, Call), v_result>
//
// Method calls are lowered differently based on:
// 1. Static dispatch: receiver type is concrete, method symbol resolved at compile time
// 2. Dynamic dispatch: receiver is $ClassName, vtable lookup at runtime
// 3. Capability methods: built-in methods on FileSystem, HeapAllocator, etc.
// 4. Modal methods: methods on modal types in specific states
// =============================================================================

LowerResult LowerMethodCall(const ast::MethodCallExpr& expr, LowerCtx& ctx) {
    SPEC_RULE("Lower-Expr-MethodCall");

    // Get receiver type for dispatch determination
    analysis::TypeRef recv_type = ReceiverBindingType(expr.receiver, ctx);
    if (ctx.expr_type) {
        if (!recv_type) {
            recv_type = ctx.expr_type(*expr.receiver);
        }
    }
    analysis::TypeRef stripped = recv_type ? analysis::StripPerm(recv_type) : recv_type;
    const auto* dyn_type = stripped ? std::get_if<analysis::TypeDynamic>(&stripped->node) : nullptr;

    // Handle shared-value until(pred, action) specially.
    if (analysis::IdEq(expr.name, "until") &&
        expr.args.size() == 2 &&
        expr.args[0].value &&
        expr.args[1].value) {
        SPEC_RULE("Lower-MethodCall-Until");

        auto recv_result = LowerExpr(*expr.receiver, ctx);

        analysis::TypeRef recv_value_type = ctx.LookupValueType(recv_result.value);
        if (!recv_value_type && ctx.expr_type) {
            recv_value_type = ctx.expr_type(*expr.receiver);
        }
        if (!recv_value_type) {
            recv_value_type = analysis::MakeTypePrim("()");
        }

        const std::string recv_temp_name = ctx.FreshTempValue("until_recv").name;
        IRBindVar bind_recv;
        bind_recv.name = recv_temp_name;
        bind_recv.value = recv_result.value;
        bind_recv.type = recv_value_type;
        bind_recv.prov = analysis::ProvenanceKind::Stack;
        ctx.RegisterVar(recv_temp_name,
                        recv_value_type,
                        false,
                        false,
                        analysis::ProvenanceKind::Stack,
                        std::nullopt);

        ast::Expr recv_ident;
        recv_ident.span = expr.receiver ? expr.receiver->span : core::Span{};
        recv_ident.node = ast::IdentifierExpr{recv_temp_name};
        auto recv_ident_ptr = std::make_shared<ast::Expr>(recv_ident);

        ast::CallExpr pred_call_expr;
        pred_call_expr.callee = expr.args[0].value;
        ast::Arg pred_arg;
        pred_arg.value = recv_ident_ptr;
        pred_arg.moved = false;
        pred_call_expr.args.push_back(std::move(pred_arg));
        auto pred_result = LowerCallExpr(pred_call_expr, ctx);

        ast::CallExpr action_call_expr;
        action_call_expr.callee = expr.args[1].value;
        ast::Arg action_arg;
        action_arg.value = recv_ident_ptr;
        action_arg.moved = false;
        action_call_expr.args.push_back(std::move(action_arg));
        auto action_result = LowerCallExpr(action_call_expr, ctx);

        analysis::TypeRef action_result_type = ctx.LookupValueType(action_result.value);
        if (!action_result_type && ctx.expr_type) {
            ast::Expr action_call_wrapper;
            action_call_wrapper.node = action_call_expr;
            action_result_type = ctx.expr_type(action_call_wrapper);
        }
        if (!action_result_type && ctx.expr_type) {
            analysis::TypeRef action_callee_type = ctx.expr_type(*expr.args[1].value);
            if (action_callee_type) {
                action_callee_type = analysis::StripPerm(action_callee_type);
                if (const auto* fn = std::get_if<analysis::TypeFunc>(&action_callee_type->node)) {
                    action_result_type = fn->ret;
                }
            }
        }
        if (!action_result_type) {
            action_result_type = analysis::MakeTypePrim("()");
        }

        const analysis::TypeRef until_async_type =
            analysis::MakeTypePath({"Async"},
                                   {analysis::MakeTypePrim("()"),
                                    analysis::MakeTypePrim("()"),
                                    action_result_type,
                                    analysis::MakeTypePrim("!")});

        IRAsyncComplete complete;
        complete.value = action_result.value;
        complete.result = ctx.FreshTempValue("until_async");
        complete.async_type = until_async_type;
        complete.result_type = action_result_type;
        ctx.RegisterValueType(complete.result, until_async_type);
        IRValue complete_value = complete.result;

        IRIf guard;
        guard.cond = pred_result.value;
        guard.then_ir = SeqIR({action_result.ir, MakeIR(std::move(complete))});
        guard.then_value = complete_value;
        guard.else_ir = LowerPanic(PanicReason::AsyncFailed, ctx);
        guard.else_value = complete_value;
        guard.result = ctx.FreshTempValue("until_result");
        ctx.RegisterValueType(guard.result, until_async_type);

        IRValue until_value = guard.result;
        return LowerResult{
            SeqIR({recv_result.ir,
                   MakeIR(std::move(bind_recv)),
                   pred_result.ir,
                   MakeIR(std::move(guard))}),
            until_value};
    }

    // Async combinators are lowered to pseudo-calls that are interpreted
    // directly by LLVM emission. This preserves combinator semantics instead
    // of the old receiver-preserving fallback behavior.
    const auto async_combinator_symbol = AsyncCombinatorRuntimeSymbol(expr.name);
    const auto* async_path =
        stripped ? std::get_if<analysis::TypePathType>(&stripped->node) : nullptr;
    if (stripped &&
        async_path &&
        analysis::IsAsyncType(stripped) &&
        async_combinator_symbol.has_value()) {
        SPEC_RULE("Lower-MethodCall-AsyncCombinator");
        auto recv_result = LowerExpr(*expr.receiver, ctx);
        ParamModeList param_modes;
        // Preserve callable/function arguments as direct values for combinator
        // emission; address-of lowering here turns procedure identifiers into
        // non-callable references at LLVM emission time.
        param_modes.reserve(expr.args.size());
        for (std::size_t i = 0; i < expr.args.size(); ++i) {
            param_modes.push_back(analysis::ParamMode::Move);
        }
        auto [args_ir, arg_values] = LowerArgs(param_modes, expr.args, ctx);
        IRCall comb_call;
        comb_call.callee = IRValue{IRValue::Kind::Symbol,
                                   *async_combinator_symbol,
                                   {}};
        comb_call.args.reserve(1 + arg_values.size());
        comb_call.args.push_back(recv_result.value);
        for (const auto& arg : arg_values) {
            comb_call.args.push_back(arg);
        }
        comb_call.result = ctx.FreshTempValue("async_comb");
        IRValue comb_result = comb_call.result;

        if (ctx.expr_type) {
            ast::Expr wrapper;
            wrapper.span = expr.receiver ? expr.receiver->span : core::Span{};
            wrapper.node = expr;
            if (auto result_type = ctx.expr_type(wrapper)) {
                ctx.RegisterValueType(comb_call.result, result_type);
            }
        }

        return LowerResult{
            SeqIR({recv_result.ir, args_ir, MakeIR(std::move(comb_call))}),
            comb_result};
    }

    // Handle builtin modal calls that lower to receiver-backed allocation.
    if (stripped) {
        if (const auto* modal = std::get_if<analysis::TypeModalState>(&stripped->node)) {
            const auto sig = analysis::LookupBuiltinModalMemberSig(modal->path, modal->state, expr.name);
            if (sig.has_value() &&
                sig->lowering == analysis::BuiltinModalLoweringOp::AllocInReceiver) {
                SPEC_RULE("Lower-MethodCall-Region-Alloc");
                if (expr.args.size() == 1 && expr.args[0].value) {
                    auto recv_result = LowerExpr(*expr.receiver, ctx);
                    auto value_result = LowerExpr(*expr.args[0].value, ctx);
                    analysis::TypeRef value_type;
                    if (ctx.expr_type) {
                        value_type = ctx.expr_type(*expr.args[0].value);
                    }
                    if (!value_type) {
                        value_type = ctx.LookupValueType(value_result.value);
                    }
                    IRAlloc alloc;
                    alloc.region = recv_result.value;
                    alloc.value = value_result.value;
                    alloc.type = value_type;
                    IRValue ptr_value = ctx.FreshTempValue("alloc_ptr");
                    alloc.result = ptr_value;
                    IRValue alloc_val = ctx.FreshTempValue("alloc_val");
                    DerivedValueInfo info;
                    info.kind = DerivedValueInfo::Kind::LoadFromAddr;
                    info.base = ptr_value;
                    ctx.RegisterDerivedValue(alloc_val, info);
                    if (value_type) {
                        ctx.RegisterValueType(alloc_val, value_type);
                    }
                    return LowerResult{SeqIR({recv_result.ir, value_result.ir, MakeIR(std::move(alloc))}),
                                       alloc_val};
                }
            }
        }
    }

    ParamModeList param_modes;
    bool move_receiver = false;
    const analysis::ScopeContext& scope = ScopeForLowering(ctx);

    // Handle Context builtin methods (cpu, gpu, inline)
    if (const auto* path = stripped ? std::get_if<analysis::TypePathType>(&stripped->node) : nullptr) {
    if (analysis::IsContextTypePath(path->path) &&
        (expr.name == "cpu" || expr.name == "gpu" || expr.name == "inline")) {
            SPEC_RULE("Lower-MethodCall-ContextBuiltin");
            auto recv_result = LowerRecvArgExpr(expr.receiver, ctx);
            auto [args_ir, arg_values] = LowerArgs(param_modes, expr.args, ctx);

            std::vector<IRValue> all_args;
            all_args.push_back(recv_result.value);
            all_args.insert(all_args.end(), arg_values.begin(), arg_values.end());

            const std::string qualified = "Context::" + expr.name;
            std::string callee_sym = BuiltinSym(qualified);
            IRValue result_value = ctx.FreshTempValue("method_call");

            IRCall call;
            call.callee = IRValue{IRValue::Kind::Symbol, callee_sym, {}};
            call.args = std::move(all_args);
            call.result = result_value;

            return LowerResult{SeqIR({recv_result.ir, args_ir, MakeIR(std::move(call))}),
                               result_value};
        }

        if (analysis::IsSystemTypePath(path->path)) {
            const auto sig = analysis::LookupSystemMethodSig(expr.name);
            if (sig.has_value()) {
                SPEC_RULE("Lower-MethodCall-SystemBuiltin");
                analysis::TypeRef result_type = sig->ret;
                const std::string qualified = "System::" + expr.name;
                std::string callee_sym = BuiltinSym(qualified);
                if (const auto runtime_info = GetRuntimeFuncInfo(callee_sym)) {
                    param_modes.reserve(runtime_info->params.size());
                    for (const auto& param : runtime_info->params) {
                        param_modes.push_back(param.mode);
                    }
                    if (runtime_info->ret) {
                        result_type = runtime_info->ret;
                    }
                } else {
                    param_modes = ParamModesFromParams(sig->params);
                }
                auto recv_result = LowerRecvArgExpr(expr.receiver, ctx);
                auto [args_ir, arg_values] = LowerArgs(param_modes, expr.args, ctx);

                std::vector<IRValue> all_args;
                all_args.insert(all_args.end(), arg_values.begin(), arg_values.end());

                IRValue result_value = ctx.FreshTempValue("method_call");
                if (result_type) {
                    ctx.RegisterValueType(result_value, result_type);
                }

                IRCall call;
                call.callee = IRValue{IRValue::Kind::Symbol, callee_sym, {}};
                call.args = std::move(all_args);
                call.result = result_value;

                return LowerResult{SeqIR({recv_result.ir, args_ir, MakeIR(std::move(call))}),
                                   result_value};
            }
        }
    }

    // Handle dynamic dispatch ($ClassName)
    if (dyn_type && ctx.sigma) {
        const bool is_builtin = ::cursive::codegen::IsBuiltinCapClass(dyn_type->path);
        const auto* class_method = analysis::LookupClassMethod(scope, dyn_type->path, expr.name);
        if (class_method) {
            param_modes = ParamModesFromParams(class_method->params);
        }

        // Capability methods (FileSystem, HeapAllocator, etc.)
        if (is_builtin) {
            SPEC_RULE("Lower-MethodCall-Capability");
            if (const auto builtin_param_modes =
                    BuiltinCapabilityParamModes(dyn_type->path, expr.name)) {
                param_modes = *builtin_param_modes;
            }
            auto recv_result = LowerRecvArgExpr(expr.receiver, ctx);
            auto [args_ir, arg_values] = LowerArgs(param_modes, expr.args, ctx);

            std::vector<IRValue> all_args;
            all_args.push_back(recv_result.value);
            all_args.insert(all_args.end(), arg_values.begin(), arg_values.end());

            std::string callee_sym = expr.name;
            if (auto sym = BuiltinMethodSym(dyn_type->path, expr.name)) {
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

        // Dynamic dispatch via vtable
        const ast::ClassDecl* class_decl = nullptr;
        const auto class_key = analysis::PathKeyOf(dyn_type->path);
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

    // Static dispatch - look up method from type
    if (!dyn_type && ctx.sigma) {
        if (auto modal_info = ModalStateInfo(recv_type)) {
            const auto* modal_decl = analysis::LookupModalDecl(scope, modal_info->first);
            if (modal_decl) {
                if (const auto* state_method =
                        analysis::LookupStateMethodDecl(*modal_decl, modal_info->second, expr.name)) {
                    param_modes = ParamModesFromParams(state_method->params);
                } else if (const auto* transition =
                               analysis::LookupTransitionDecl(*modal_decl, modal_info->second, expr.name)) {
                    param_modes = ParamModesFromParams(transition->params);
                    move_receiver = true;
                }
            }
        } else if (stripped) {
            if (const auto builtin_sig =
                    analysis::LookupStringBytesBuiltinMethodSig(stripped, expr.name)) {
                param_modes = ParamModesFromFuncParams(builtin_sig->params);
            }
            const auto lookup = analysis::LookupMethodStatic(scope, stripped, expr.name);
            if (lookup.record_method) {
                param_modes = ParamModesFromParams(lookup.record_method->params);
            } else if (lookup.class_method) {
                param_modes = ParamModesFromParams(lookup.class_method->params);
            }
        }
    }

    // Lower receiver and arguments
    LowerResult recv_result;
    if (move_receiver && expr.receiver) {
        // Modal transitions consume the source state receiver.
        if (std::holds_alternative<ast::MoveExpr>(expr.receiver->node)) {
            recv_result = LowerExpr(*expr.receiver, ctx);
        } else {
            recv_result = LowerMovePlace(*expr.receiver, ctx);
        }
    } else {
        recv_result = LowerRecvArgExpr(expr.receiver, ctx);
    }
    auto [args_ir, arg_values] = LowerArgs(param_modes, expr.args, ctx);

    // Build argument list with receiver first
    std::vector<IRValue> all_args;
    all_args.push_back(recv_result.value);
    all_args.insert(all_args.end(), arg_values.begin(), arg_values.end());

    // Resolve method symbol
    std::string callee_sym = expr.name;
    if (ctx.sigma && ctx.expr_type) {
        const analysis::ScopeContext& sym_scope = ScopeForLowering(ctx);
        auto recv_type_for_sym = recv_type;
        if (!recv_type_for_sym) {
            recv_type_for_sym = ctx.expr_type(*expr.receiver);
        }
        if (recv_type_for_sym) {
            const auto stripped_for_sym = analysis::StripPerm(recv_type_for_sym);
            if (stripped_for_sym) {
                if (const auto builtin_sig =
                        analysis::LookupStringBytesBuiltinMethodSig(stripped_for_sym, expr.name)) {
                    (void)builtin_sig;
                    if (std::holds_alternative<analysis::TypeString>(stripped_for_sym->node)) {
                        if (const std::string builtin =
                                BuiltinSym(std::string("string::") + expr.name);
                            !builtin.empty()) {
                            callee_sym = builtin;
                        }
                    } else if (std::holds_alternative<analysis::TypeBytes>(stripped_for_sym->node)) {
                        if (const std::string builtin =
                                BuiltinSym(std::string("bytes::") + expr.name);
                            !builtin.empty()) {
                            callee_sym = builtin;
                        }
                    }
                }
            }
            if (auto sym = MethodSymbol(sym_scope, recv_type_for_sym, expr.name)) {
                callee_sym = *sym;
            }
        }
    }

    IRValue result_value = ctx.FreshTempValue("method_call");

    IRCall call;
    call.callee = IRValue{IRValue::Kind::Symbol, callee_sym, {}};
    call.args = std::move(all_args);
    call.result = result_value;

    bool needs_panic_out = ctx.NeedsPanicOutForSymbol(callee_sym);
    // Async::resume executes a generated resume function via the runtime.
    // The runtime forwards panic_out to that callback, so resume calls must
    // always receive a concrete panic record pointer.
    if (callee_sym == BuiltinSymAsyncResume()) {
        needs_panic_out = true;
    }

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

}  // namespace cursive::codegen
