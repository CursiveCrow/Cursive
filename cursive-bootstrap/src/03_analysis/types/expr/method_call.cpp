// =================================================================
// File: 03_analysis/types/expr/method_call.cpp
// Construct: Method Call Expression Type Checking
// Spec Section: 5.2.12
// Spec Rules: T-MethodCall, T-Record-MethodCall, T-Modal-Transition,
//             T-Modal-Method, T-Dynamic-MethodCall, T-Opaque-Project,
//             MethodCall-RecvPerm-Err, Step-MethodCall
// =================================================================
#include "cursive0/03_analysis/types/expr/method_call.h"

#include "cursive0/00_core/assert_spec.h"
#include "cursive0/03_analysis/caps/cap_concurrency.h"
#include "cursive0/03_analysis/caps/cap_filesystem.h"
#include "cursive0/03_analysis/caps/cap_heap.h"
#include "cursive0/03_analysis/caps/cap_system.h"
#include "cursive0/03_analysis/composite/classes.h"
#include "cursive0/03_analysis/composite/record_methods.h"
#include "cursive0/03_analysis/modal/modal.h"
#include "cursive0/03_analysis/modal/modal_transitions.h"
#include "cursive0/03_analysis/resolve/scopes.h"
#include "cursive0/03_analysis/types/subtyping.h"
#include "cursive0/03_analysis/types/type_expr.h"
#include "cursive0/03_analysis/types/type_lower.h"

namespace cursive0::analysis::expr {

using namespace cursive0::analysis;

ExprTypeResult TypeMethodCallExprImpl(const ScopeContext& ctx,
                                      const StmtTypeContext& type_ctx,
                                      const syntax::MethodCallExpr& expr,
                                      const TypeEnv& env,
                                      const core::Span& span) {
  ExprTypeResult result;

  if (!expr.receiver) {
    return result;
  }

  auto type_expr = [&](const syntax::ExprPtr& inner) {
    return TypeExpr(ctx, type_ctx, inner, env);
  };
  PlaceTypeFn type_place = [&](const syntax::ExprPtr& inner) {
    return TypePlace(ctx, type_ctx, inner, env);
  };
  auto lower_type = [&](const std::shared_ptr<syntax::Type>& type) -> LowerTypeResult {
    const auto lowered = LowerType(ctx, type);
    if (!lowered.ok) {
      return {false, lowered.diag_id, {}};
    }
    return {true, std::nullopt, lowered.type};
  };

  TypeRef lookup_base;
  Permission caller_perm = Permission::Const;
  const auto place = TypePlace(ctx, type_ctx, expr.receiver, env);
  if (place.ok) {
    lookup_base = StripPerm(place.type);
    caller_perm = PermOfType(place.type);
  } else {
    const auto base_expr = TypeExpr(ctx, type_ctx, expr.receiver, env);
    if (!base_expr.ok) {
      result.diag_id = base_expr.diag_id;
      return result;
    }
    lookup_base = StripPerm(base_expr.type);
    caller_perm = PermOfType(base_expr.type);
  }

  if (!lookup_base) {
    return result;
  }
  if (type_ctx.require_pure && IsCapabilityType(lookup_base)) {
    result.diag_id = "E-SEM-2802";
    return result;
  }

  if (const auto* opaque = std::get_if<TypeOpaque>(&lookup_base->node)) {
    const auto class_table = ClassMethodTable(ctx, opaque->class_path);
    if (!class_table.ok) {
      result.diag_id = class_table.diag_id;
      return result;
    }

    const syntax::ClassMethodDecl* method = nullptr;
    syntax::ClassPath owner;
    for (const auto& entry : class_table.methods) {
      if (!entry.method) {
        continue;
      }
      if (!IdEq(entry.method->name, expr.name)) {
        continue;
      }
      method = entry.method;
      owner = entry.owner;
      break;
    }
    if (!method) {
      SPEC_RULE("T-Opaque-Project");
      result.diag_id = "E-TYP-2510";
      return result;
    }

    if (!owner.empty() && IdEq(owner.back(), "Drop") &&
        IdEq(method->name, "drop")) {
      SPEC_RULE("Drop-Call-Err-Dyn");
      result.diag_id = "Drop-Call-Err-Dyn";
      return result;
    }

    auto lower_type_self = [&](const std::shared_ptr<syntax::Type>& type)
        -> LowerTypeResult {
      const auto lowered = LowerType(ctx, type);
      if (!lowered.ok) {
        return {false, lowered.diag_id, {}};
      }
      return {true, std::nullopt, SubstSelfType(lookup_base, lowered.type)};
    };

    const auto mode = RecvModeOf(method->receiver);
    const auto recv_base =
        RecvBaseType(expr.receiver, mode, type_place, type_expr);
    if (!recv_base.ok) {
      result.diag_id = recv_base.diag_id;
      return result;
    }
    const auto recv_type =
        RecvTypeForReceiver(ctx, lookup_base, method->receiver, lower_type_self);
    if (!recv_type.ok) {
      result.diag_id = recv_type.diag_id;
      return result;
    }
    const auto method_perm = PermOfType(recv_type.type);
    if (!PermSub(recv_base.perm, method_perm)) {
      SPEC_RULE("MethodCall-RecvPerm-Err");
      result.diag_id = "MethodCall-RecvPerm-Err";
      return result;
    }

    const auto recv_arg = RecvArgOk(expr.receiver, mode, type_expr);
    if (!recv_arg.ok) {
      result.diag_id = recv_arg.diag_id;
      return result;
    }

    const auto args_ok =
        ArgsOk(ctx, method->params, expr.args, type_expr, &type_place,
               lower_type_self);
    if (!args_ok.ok) {
      result.diag_id = args_ok.diag_id;
      return result;
    }

    LowerTypeResult ret_type;
    if (!method->return_type_opt) {
      ret_type = {true, std::nullopt, MakeTypePrim("()")} ;
    } else {
      ret_type = lower_type_self(method->return_type_opt);
    }
    if (!ret_type.ok) {
      result.diag_id = ret_type.diag_id;
      return result;
    }

    SPEC_RULE("T-Opaque-Project");
    result.ok = true;
    result.type = ret_type.type;
    return result;
  }

  if (const auto* modal = std::get_if<TypeModalState>(&lookup_base->node)) {
    if (modal->path.size() == 1 && IdEq(modal->path[0], "Region") &&
        (IdEq(expr.name, "reset_unchecked") ||
         IdEq(expr.name, "free_unchecked")) &&
        (IdEq(modal->state, "Active") || IdEq(modal->state, "Frozen")) &&
        !IsInUnsafeSpan(ctx, span)) {
      SPEC_RULE("Region-Unchecked-Unsafe-Err");
      result.diag_id = "Region-Unchecked-Unsafe-Err";
      return result;
    }

    const auto* decl = LookupModalDecl(ctx, modal->path);
    if (!decl) {
      return result;
    }

    if (const auto* transition =
            LookupTransitionDecl(*decl, modal->state, expr.name)) {
      if (caller_perm != Permission::Unique) {
        SPEC_RULE("Transition-Source-Err");
        result.diag_id = "Transition-Source-Err";
        return result;
      }
      if (!StateMemberVisible(ctx, modal->path, transition->vis)) {
        SPEC_RULE("Transition-NotVisible");
        result.diag_id = "Transition-NotVisible";
        return result;
      }
      const auto args_ok =
          ArgsOk(ctx, transition->params, expr.args, type_expr, &type_place,
                 lower_type);
      if (!args_ok.ok) {
        result.diag_id = args_ok.diag_id;
        return result;
      }
      const auto recv_arg =
          RecvArgOk(expr.receiver, ParamMode::Move, type_expr);
      if (!recv_arg.ok) {
        if (recv_arg.diag_id.has_value()) {
          result.diag_id = recv_arg.diag_id;
        } else {
          SPEC_RULE("Call-Move-Missing");
          result.diag_id = "Call-Move-Missing";
        }
        return result;
      }
      SPEC_RULE("T-Modal-Transition");
      result.ok = true;
      result.type = MakeTypeModalState(modal->path, transition->target_state);
      return result;
    }

    const auto* method =
        LookupStateMethodDecl(*decl, modal->state, expr.name);
    if (!method) {
      SPEC_RULE("Modal-Method-NotFound");
      result.diag_id = "Modal-Method-NotFound";
      return result;
    }
    if (!StateMemberVisible(ctx, modal->path, method->vis)) {
      SPEC_RULE("Modal-Method-NotVisible");
      result.diag_id = "Modal-Method-NotVisible";
      return result;
    }
    if (!PermSub(caller_perm, Permission::Const)) {
      SPEC_RULE("MethodCall-RecvPerm-Err");
      result.diag_id = "MethodCall-RecvPerm-Err";
      return result;
    }
    const auto args_ok =
        ArgsOk(ctx, method->params, expr.args, type_expr, &type_place,
               lower_type);
    if (!args_ok.ok) {
      result.diag_id = args_ok.diag_id;
      return result;
    }
    LowerTypeResult ret_type;
    if (!method->return_type_opt) {
      ret_type = {true, std::nullopt, MakeTypePrim("()")} ;
    } else {
      ret_type = LowerType(ctx, method->return_type_opt);
    }
    if (!ret_type.ok) {
      result.diag_id = ret_type.diag_id;
      return result;
    }
    SPEC_RULE("T-Modal-Method");
    result.ok = true;
    result.type = ret_type.type;
    return result;
  }

  if (const auto* dyn = std::get_if<TypeDynamic>(&lookup_base->node)) {
    if (IsFileSystemClassPath(dyn->path)) {
      const auto sig = LookupFileSystemMethodSig(expr.name);
      if (!sig.has_value()) {
        SPEC_RULE("LookupClassMethod-NotFound");
        result.diag_id = "LookupMethod-NotFound";
        return result;
      }

      const auto recv_base =
          RecvBaseType(expr.receiver, std::nullopt, type_place, type_expr);
      if (!recv_base.ok) {
        result.diag_id = recv_base.diag_id;
        return result;
      }
      if (!recv_base.base ||
          !std::holds_alternative<TypeDynamic>(recv_base.base->node)) {
        return result;
      }
      if (!PermSub(recv_base.perm, sig->recv_perm)) {
        SPEC_RULE("MethodCall-RecvPerm-Err");
        result.diag_id = "MethodCall-RecvPerm-Err";
        return result;
      }
      const auto recv_arg = RecvArgOk(expr.receiver, std::nullopt, type_expr);
      if (!recv_arg.ok) {
        result.diag_id = recv_arg.diag_id;
        return result;
      }
      const auto args_ok =
          ArgsOk(ctx, sig->params, expr.args, type_expr, &type_place,
                 lower_type);
      if (!args_ok.ok) {
        result.diag_id = args_ok.diag_id;
        return result;
      }

      SPEC_RULE("T-Dynamic-MethodCall");
      result.ok = true;
      result.type = sig->ret;
      return result;
    }
    if (IsHeapAllocatorClassPath(dyn->path)) {
      const auto sig = LookupHeapAllocatorMethodSig(expr.name);
      if (!sig.has_value()) {
        SPEC_RULE("LookupClassMethod-NotFound");
        result.diag_id = "LookupMethod-NotFound";
        return result;
      }

      if (sig->kind == HeapAllocatorMethodKind::AllocRaw &&
          !IsInUnsafeSpan(ctx, span)) {
        SPEC_RULE("AllocRaw-Unsafe-Err");
        result.diag_id = "AllocRaw-Unsafe-Err";
        return result;
      }
      if (sig->kind == HeapAllocatorMethodKind::DeallocRaw &&
          !IsInUnsafeSpan(ctx, span)) {
        SPEC_RULE("DeallocRaw-Unsafe-Err");
        result.diag_id = "DeallocRaw-Unsafe-Err";
        return result;
      }

      const auto recv_base =
          RecvBaseType(expr.receiver, std::nullopt, type_place, type_expr);
      if (!recv_base.ok) {
        result.diag_id = recv_base.diag_id;
        return result;
      }
      if (!recv_base.base ||
          !std::holds_alternative<TypeDynamic>(recv_base.base->node)) {
        return result;
      }
      if (!PermSub(recv_base.perm, sig->recv_perm)) {
        SPEC_RULE("MethodCall-RecvPerm-Err");
        result.diag_id = "MethodCall-RecvPerm-Err";
        return result;
      }
      const auto recv_arg = RecvArgOk(expr.receiver, std::nullopt, type_expr);
      if (!recv_arg.ok) {
        result.diag_id = recv_arg.diag_id;
        return result;
      }

      if (sig->kind == HeapAllocatorMethodKind::AllocRaw) {
        const auto args_ok =
            ArgsOk(ctx, sig->params, expr.args, type_expr, &type_place,
                   lower_type);
        if (!args_ok.ok) {
          result.diag_id = args_ok.diag_id;
          return result;
        }
        // alloc_raw requires an expected raw pointer type for its element.
        return result;
      }

      if (sig->kind == HeapAllocatorMethodKind::DeallocRaw) {
        if (expr.args.size() != 2) {
          SPEC_RULE("Call-ArgCount-Err");
          result.diag_id = "Call-ArgCount-Err";
          return result;
        }
        if (expr.args[0].moved || expr.args[1].moved) {
          SPEC_RULE("Call-Move-Unexpected");
          result.diag_id = "Call-Move-Unexpected";
          return result;
        }
        if (!IsPlaceExpr(expr.args[0].value) ||
            !IsPlaceExpr(expr.args[1].value)) {
          SPEC_RULE("Call-Arg-NotPlace");
          result.diag_id = "Call-Arg-NotPlace";
          return result;
        }
        const auto ptr_type = type_expr(expr.args[0].value);
        if (!ptr_type.ok) {
          result.diag_id = ptr_type.diag_id;
          return result;
        }
        const auto ptr_strip = StripPerm(ptr_type.type);
        const auto* raw_ptr = ptr_strip
                                  ? std::get_if<TypeRawPtr>(&ptr_strip->node)
                                  : nullptr;
        if (!raw_ptr || raw_ptr->qual != RawPtrQual::Mut) {
          SPEC_RULE("Call-ArgType-Err");
          result.diag_id = "Call-ArgType-Err";
          return result;
        }
        const auto count_type = type_expr(expr.args[1].value);
        if (!count_type.ok) {
          result.diag_id = count_type.diag_id;
          return result;
        }
        const auto sub = Subtyping(ctx, count_type.type, MakeTypePrim("usize"));
        if (!sub.ok) {
          result.diag_id = sub.diag_id;
          return result;
        }
        if (!sub.subtype) {
          SPEC_RULE("Call-ArgType-Err");
          result.diag_id = "Call-ArgType-Err";
          return result;
        }
        SPEC_RULE("T-Dynamic-MethodCall");
        result.ok = true;
        result.type = sig->ret;
        return result;
      }

      const auto args_ok =
          ArgsOk(ctx, sig->params, expr.args, type_expr, &type_place,
                 lower_type);
      if (!args_ok.ok) {
        result.diag_id = args_ok.diag_id;
        return result;
      }

      SPEC_RULE("T-Dynamic-MethodCall");
      result.ok = true;
      result.type = sig->ret;
      return result;
    }
    if (IsReactorClassPath(dyn->path)) {
      if (!(IdEq(expr.name, "run") || IdEq(expr.name, "register"))) {
        SPEC_RULE("LookupClassMethod-NotFound");
        result.diag_id = "LookupMethod-NotFound";
        return result;
      }

      const auto recv_base =
          RecvBaseType(expr.receiver, std::nullopt, type_place, type_expr);
      if (!recv_base.ok) {
        result.diag_id = recv_base.diag_id;
        return result;
      }
      if (!recv_base.base ||
          !std::holds_alternative<TypeDynamic>(recv_base.base->node)) {
        return result;
      }
      if (!PermSub(recv_base.perm, Permission::Const)) {
        SPEC_RULE("MethodCall-RecvPerm-Err");
        result.diag_id = "MethodCall-RecvPerm-Err";
        return result;
      }
      const auto recv_arg = RecvArgOk(expr.receiver, std::nullopt, type_expr);
      if (!recv_arg.ok) {
        result.diag_id = recv_arg.diag_id;
        return result;
      }
      if (expr.args.size() != 1) {
        SPEC_RULE("Call-ArgCount-Err");
        result.diag_id = "Call-ArgCount-Err";
        return result;
      }
      if (expr.args[0].moved) {
        SPEC_RULE("Call-Move-Unexpected");
        result.diag_id = "Call-Move-Unexpected";
        return result;
      }
      const auto arg_type = type_expr(expr.args[0].value);
      if (!arg_type.ok) {
        result.diag_id = arg_type.diag_id;
        return result;
      }
      const auto async_sig = AsyncSigOf(ctx, arg_type.type);
      if (!async_sig.has_value() ||
          !IsPrimType(async_sig->out, "()") ||
          !IsPrimType(async_sig->in, "()")) {
        SPEC_RULE("Call-ArgType-Err");
        result.diag_id = "Call-ArgType-Err";
        return result;
      }

      SPEC_RULE("T-Dynamic-MethodCall");
      result.ok = true;
      if (IdEq(expr.name, "run")) {
        result.type = MakeTypeUnion({async_sig->result, async_sig->err});
      } else {
        result.type = MakeTrackedType(async_sig->result, async_sig->err);
      }
      return result;
    }
    if (IsExecutionDomainTypePath(dyn->path)) {
      const auto sig = LookupExecutionDomainMethodSig(expr.name);
      if (!sig.has_value()) {
        SPEC_RULE("LookupClassMethod-NotFound");
        result.diag_id = "LookupMethod-NotFound";
        return result;
      }

      const auto recv_base =
          RecvBaseType(expr.receiver, std::nullopt, type_place, type_expr);
      if (!recv_base.ok) {
        result.diag_id = recv_base.diag_id;
        return result;
      }
      if (!recv_base.base ||
          !std::holds_alternative<TypeDynamic>(recv_base.base->node)) {
        return result;
      }
      if (!PermSub(recv_base.perm, sig->recv_perm)) {
        SPEC_RULE("MethodCall-RecvPerm-Err");
        result.diag_id = "MethodCall-RecvPerm-Err";
        return result;
      }
      const auto recv_arg = RecvArgOk(expr.receiver, std::nullopt, type_expr);
      if (!recv_arg.ok) {
        result.diag_id = recv_arg.diag_id;
        return result;
      }
      const auto args_ok =
          ArgsOk(ctx, sig->params, expr.args, type_expr, &type_place,
                 lower_type);
      if (!args_ok.ok) {
        result.diag_id = args_ok.diag_id;
        return result;
      }

      SPEC_RULE("T-Dynamic-MethodCall");
      result.ok = true;
      result.type = sig->ret;
      return result;
    }
    const auto table = ClassMethodTable(ctx, dyn->path);
    if (!table.ok) {
      result.diag_id = table.diag_id;
      return result;
    }

    const syntax::ClassMethodDecl* method = nullptr;
    syntax::ClassPath owner;
    for (const auto& entry : table.methods) {
      if (!entry.method) {
        continue;
      }
      if (!IdEq(entry.method->name, expr.name)) {
        continue;
      }
      method = entry.method;
      owner = entry.owner;
      break;
    }
    if (!method) {
      SPEC_RULE("LookupClassMethod-NotFound");
      result.diag_id = "LookupMethod-NotFound";
      return result;
    }

    if (!owner.empty() && IdEq(owner.back(), "Drop") &&
        IdEq(method->name, "drop")) {
      SPEC_RULE("Drop-Call-Err-Dyn");
      result.diag_id = "Drop-Call-Err-Dyn";
      return result;
    }

    const auto mode = RecvModeOf(method->receiver);
    const auto recv_base =
        RecvBaseType(expr.receiver, mode, type_place, type_expr);
    if (!recv_base.ok) {
      result.diag_id = recv_base.diag_id;
      return result;
    }
    if (!recv_base.base ||
        !std::holds_alternative<TypeDynamic>(recv_base.base->node)) {
      return result;
    }

    const auto self_var = MakeTypePath({"Self"});
    const auto recv_type =
        RecvTypeForReceiver(ctx, self_var, method->receiver, lower_type);
    if (!recv_type.ok) {
      result.diag_id = recv_type.diag_id;
      return result;
    }
    const auto method_perm = PermOfType(recv_type.type);
    if (type_ctx.require_pure) {
      if (method_perm != Permission::Const ||
          !ParamsPure(ctx, method->params, lower_type)) {
        result.diag_id = "E-SEM-2802";
        return result;
      }
    }
    if (!PermSub(recv_base.perm, method_perm)) {
      SPEC_RULE("MethodCall-RecvPerm-Err");
      result.diag_id = "MethodCall-RecvPerm-Err";
      return result;
    }

    const auto recv_arg = RecvArgOk(expr.receiver, mode, type_expr);
    if (!recv_arg.ok) {
      result.diag_id = recv_arg.diag_id;
      return result;
    }

    const auto args_ok =
        ArgsOk(ctx, method->params, expr.args, type_expr, &type_place,
               lower_type);
    if (!args_ok.ok) {
      result.diag_id = args_ok.diag_id;
      return result;
    }

    LowerTypeResult ret_type;
    if (!method->return_type_opt) {
      ret_type = {true, std::nullopt, MakeTypePrim("()")} ;
    } else {
      ret_type = LowerType(ctx, method->return_type_opt);
    }
    if (!ret_type.ok) {
      result.diag_id = ret_type.diag_id;
      return result;
    }

    SPEC_RULE("T-Dynamic-MethodCall");
    result.ok = true;
    result.type = ret_type.type;
    return result;
  }

  if (const auto* path = std::get_if<TypePathType>(&lookup_base->node)) {
    if (path->path.size() == 1 && IdEq(path->path[0], "System")) {
      const auto sig = LookupSystemMethodSig(expr.name);
      if (!sig.has_value()) {
        SPEC_RULE("LookupMethod-NotFound");
        result.diag_id = "LookupMethod-NotFound";
        return result;
      }
      if (type_ctx.require_pure) {
        if (sig->recv_perm != Permission::Const ||
            !ParamsPure(ctx, sig->params, lower_type)) {
          result.diag_id = "E-SEM-2802";
          return result;
        }
      }

      const auto recv_base =
          RecvBaseType(expr.receiver, std::nullopt, type_place, type_expr);
      if (!recv_base.ok) {
        result.diag_id = recv_base.diag_id;
        return result;
      }
      const auto* recv_path = recv_base.base
                                  ? std::get_if<TypePathType>(&recv_base.base->node)
                                  : nullptr;
      if (!recv_path || recv_path->path.size() != 1 ||
          !IdEq(recv_path->path[0], "System")) {
        return result;
      }
      if (!PermSub(recv_base.perm, sig->recv_perm)) {
        SPEC_RULE("MethodCall-RecvPerm-Err");
        result.diag_id = "MethodCall-RecvPerm-Err";
        return result;
      }

      const auto recv_arg = RecvArgOk(expr.receiver, std::nullopt, type_expr);
      if (!recv_arg.ok) {
        result.diag_id = recv_arg.diag_id;
        return result;
      }

      const auto args_ok =
          ArgsOk(ctx, sig->params, expr.args, type_expr, &type_place, lower_type);
      if (!args_ok.ok) {
        result.diag_id = args_ok.diag_id;
        return result;
      }

      SPEC_RULE("T-Record-MethodCall");
      SPEC_RULE("Step-MethodCall");
      result.ok = true;
      result.type = sig->ret;
      return result;
    }

    // C0X Extension: Context method lookup
    if (path->path.size() == 1 && IdEq(path->path[0], "Context")) {
      const auto sig = LookupContextMethodSig(expr.name);
      if (sig.has_value()) {
        if (type_ctx.require_pure) {
          if (sig->recv_perm != Permission::Const ||
              !ParamsPure(ctx, sig->params, lower_type)) {
            result.diag_id = "E-SEM-2802";
            return result;
          }
        }
        const auto recv_base =
            RecvBaseType(expr.receiver, std::nullopt, type_place, type_expr);
        if (!recv_base.ok) {
          result.diag_id = recv_base.diag_id;
          return result;
        }
        const auto* recv_path = recv_base.base
                                    ? std::get_if<TypePathType>(&recv_base.base->node)
                                    : nullptr;
        if (!recv_path || recv_path->path.size() != 1 ||
            !IdEq(recv_path->path[0], "Context")) {
          return result;
        }
        if (!PermSub(recv_base.perm, sig->recv_perm)) {
          SPEC_RULE("MethodCall-RecvPerm-Err");
          result.diag_id = "MethodCall-RecvPerm-Err";
          return result;
        }

        const auto recv_arg = RecvArgOk(expr.receiver, std::nullopt, type_expr);
        if (!recv_arg.ok) {
          result.diag_id = recv_arg.diag_id;
          return result;
        }

        const auto args_ok =
            ArgsOk(ctx, sig->params, expr.args, type_expr, &type_place, lower_type);
        if (!args_ok.ok) {
          result.diag_id = args_ok.diag_id;
          return result;
        }

        SPEC_RULE("T-Context-MethodCall");
        result.ok = true;
        result.type = sig->ret;
        return result;
      }
    }
  }

  const auto lookup = LookupMethodStatic(ctx, lookup_base, expr.name);
  if (!lookup.ok) {
    result.diag_id = lookup.diag_id;
    return result;
  }

  const auto* record_method = lookup.record_method;
  const auto* class_method = lookup.class_method;
  if (!record_method && !class_method) {
    return result;
  }

  if (IdEq(expr.name, "drop")) {
    const syntax::ClassPath drop_path = {"Drop"};
    if (TypeImplementsClass(ctx, lookup_base, drop_path)) {
      SPEC_RULE("Drop-Call-Err");
      result.diag_id = "Drop-Call-Err";
      return result;
    }
  }

  if (class_method) {
    if (!lookup.owner_class.empty() && IdEq(lookup.owner_class.back(), "Drop") &&
        IdEq(class_method->name, "drop")) {
      SPEC_RULE("Drop-Call-Err");
      result.diag_id = "Drop-Call-Err";
      return result;
    }
  }

  const syntax::Receiver& receiver =
      record_method ? record_method->receiver : class_method->receiver;
  const auto mode = RecvModeOf(receiver);
  const auto recv_base =
      RecvBaseType(expr.receiver, mode, type_place, type_expr);
  if (!recv_base.ok) {
    result.diag_id = recv_base.diag_id;
    return result;
  }

  const auto recv_type = RecvTypeForReceiver(ctx, lookup_base, receiver, lower_type);
  if (!recv_type.ok) {
    result.diag_id = recv_type.diag_id;
    return result;
  }
  const auto& params = record_method ? record_method->params
                                     : class_method->params;
  const auto method_perm = PermOfType(recv_type.type);
  if (type_ctx.require_pure) {
    if (method_perm != Permission::Const ||
        !ParamsPure(ctx, params, lower_type)) {
      result.diag_id = "E-SEM-2802";
      return result;
    }
  }
  if (!PermSub(recv_base.perm, method_perm)) {
    SPEC_RULE("MethodCall-RecvPerm-Err");
    result.diag_id = "MethodCall-RecvPerm-Err";
    return result;
  }

  const auto recv_arg = RecvArgOk(expr.receiver, mode, type_expr);
  if (!recv_arg.ok) {
    result.diag_id = recv_arg.diag_id;
    return result;
  }

  const auto args_ok =
      ArgsOk(ctx, params, expr.args, type_expr, &type_place, lower_type);
  if (!args_ok.ok) {
    result.diag_id = args_ok.diag_id;
    return result;
  }

  LowerTypeResult ret_type;
  const auto ret_opt = record_method ? record_method->return_type_opt
                                     : class_method->return_type_opt;
  if (!ret_opt) {
    ret_type = {true, std::nullopt, MakeTypePrim("()")} ;
  } else {
    ret_type = LowerType(ctx, ret_opt);
  }
  if (!ret_type.ok) {
    result.diag_id = ret_type.diag_id;
    return result;
  }

  if (record_method) {
    SPEC_RULE("T-Record-MethodCall");
    SPEC_RULE("Step-MethodCall");
  } else {
    SPEC_RULE("T-MethodCall");
  }
  result.ok = true;
  result.type = ret_type.type;
  return result;
}

}  // namespace cursive0::analysis::expr
