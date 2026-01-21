#include "cursive0/eval/apply.h"

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/analysis/composite/classes.h"
#include "cursive0/analysis/composite/function_types.h"
#include "cursive0/analysis/composite/record_methods.h"
#include "cursive0/analysis/resolve/resolver.h"
#include "cursive0/analysis/types/type_equiv.h"
#include "cursive0/analysis/types/type_expr.h"
#include "cursive0/analysis/resolve/visibility.h"
#include "cursive0/eval/builtins.h"
#include "cursive0/eval/cleanup.h"
#include "cursive0/eval/exec.h"
#include "cursive0/eval/eval.h"

namespace cursive0::eval {

namespace {

bool IsSystemExit(const analysis::TypePath& path, std::string_view name) {
  return path.size() == 1 && path[0] == "System" && name == "exit";
}


Outcome PanicOutcome() {
  Control ctrl;
  ctrl.kind = ControlKind::Panic;
  ctrl.value = std::nullopt;
  return MakeCtrl(ctrl);
}

Outcome ExitOutcome(const Outcome& out, CleanupStatus status) {
  if (status == CleanupStatus::Abort) {
    Control ctrl;
    ctrl.kind = ControlKind::Abort;
    return MakeCtrl(ctrl);
  }
  if (status == CleanupStatus::Ok) {
    return out;
  }
  if (IsCtrl(out)) {
    const auto& ctrl = std::get<Control>(out.node);
    if (ctrl.kind == ControlKind::Abort) {
      return out;
    }
    if (ctrl.kind == ControlKind::Panic) {
      Control abort_ctrl;
      abort_ctrl.kind = ControlKind::Abort;
      return MakeCtrl(abort_ctrl);
    }
  }
  Control panic_ctrl;
  panic_ctrl.kind = ControlKind::Panic;
  return MakeCtrl(panic_ctrl);
}

std::optional<ScopeEntry*> MutableScopeById(Sigma& sigma, ScopeId sid) {
  for (auto& scope : sigma.scope_stack) {
    if (scope.id == sid) {
      return &scope;
    }
  }
  return std::nullopt;
}

Outcome BlockExit(const SemanticsContext& ctx,
                  ScopeId scope_id,
                  const Outcome& out,
                  Sigma& sigma) {
  auto scope_ptr = MutableScopeById(sigma, scope_id);
  if (!scope_ptr.has_value()) {
    return PanicOutcome();
  }
  const CleanupStatus cleanup = CleanupScope(ctx, **scope_ptr, sigma);
  const Outcome out2 = ExitOutcome(out, cleanup);
  if (IsCtrl(out2)) {
    const auto& ctrl = std::get<Control>(out2.node);
    if (ctrl.kind == ControlKind::Abort) {
      return out2;
    }
  }
  ScopeEntry popped;
  if (!PopScope(sigma, &popped)) {
    return PanicOutcome();
  }
  return out2;
}

Outcome ReturnOut(const Outcome& out) {
  if (!IsCtrl(out)) {
    return out;
  }
  const auto& ctrl = std::get<Control>(out.node);
  if (ctrl.kind == ControlKind::Return) {
    if (ctrl.value.has_value()) {
      return MakeVal(*ctrl.value);
    }
    return MakeVal(Value{UnitVal{}});
  }
  if (ctrl.kind == ControlKind::Panic || ctrl.kind == ControlKind::Abort) {
    return out;
  }
  return PanicOutcome();
}

bool ParamIsMove(const syntax::Param& param) {
  return param.mode.has_value();
}

BindInfo BindInfoForParam(const syntax::Param& param) {
  BindInfo info;
  info.mov = Movability::Mov;
  info.resp = ParamIsMove(param) ? Responsibility::Resp
                                 : Responsibility::Alias;
  return info;
}

BindInfo BindInfoForRecv(bool move_mode) {
  BindInfo info;
  info.mov = Movability::Mov;
  info.resp = move_mode ? Responsibility::Resp
                        : Responsibility::Alias;
  return info;
}

std::optional<analysis::Permission> LowerPermission(syntax::TypePerm perm) {
  switch (perm) {
    case syntax::TypePerm::Const:
      return analysis::Permission::Const;
    case syntax::TypePerm::Unique:
      return analysis::Permission::Unique;
    case syntax::TypePerm::Shared:
      return analysis::Permission::Shared;
  }
  return std::nullopt;
}

std::optional<analysis::RawPtrQual> LowerRawPtrQual(syntax::RawPtrQual qual) {
  switch (qual) {
    case syntax::RawPtrQual::Imm:
      return analysis::RawPtrQual::Imm;
    case syntax::RawPtrQual::Mut:
      return analysis::RawPtrQual::Mut;
  }
  return std::nullopt;
}

std::optional<analysis::StringState> LowerStringState(
    const std::optional<syntax::StringState>& state) {
  if (!state.has_value()) {
    return std::nullopt;
  }
  switch (*state) {
    case syntax::StringState::Managed:
      return analysis::StringState::Managed;
    case syntax::StringState::View:
      return analysis::StringState::View;
  }
  return std::nullopt;
}

std::optional<analysis::BytesState> LowerBytesState(
    const std::optional<syntax::BytesState>& state) {
  if (!state.has_value()) {
    return std::nullopt;
  }
  switch (*state) {
    case syntax::BytesState::Managed:
      return analysis::BytesState::Managed;
    case syntax::BytesState::View:
      return analysis::BytesState::View;
  }
  return std::nullopt;
}

std::optional<analysis::PtrState> LowerPtrState(
    const std::optional<syntax::PtrState>& state) {
  if (!state.has_value()) {
    return std::nullopt;
  }
  switch (*state) {
    case syntax::PtrState::Valid:
      return analysis::PtrState::Valid;
    case syntax::PtrState::Null:
      return analysis::PtrState::Null;
    case syntax::PtrState::Expired:
      return analysis::PtrState::Expired;
  }
  return std::nullopt;
}

analysis::TypePath TypePathOf(const syntax::TypePath& path) {
  analysis::TypePath out;
  out.reserve(path.size());
  for (const auto& part : path) {
    out.push_back(part);
  }
  return out;
}

std::optional<analysis::TypePath> ResolveTypePath(const SemanticsContext& ctx,
                                              const syntax::TypePath& path) {
  if (!ctx.sema || !ctx.name_maps || !ctx.module_names) {
    return TypePathOf(path);
  }
  analysis::ResolveContext resolve_ctx;
  resolve_ctx.ctx = const_cast<analysis::ScopeContext*>(ctx.sema);
  resolve_ctx.name_maps = ctx.name_maps;
  resolve_ctx.module_names = ctx.module_names;
  resolve_ctx.can_access = analysis::CanAccess;
  const auto resolved = analysis::ResolveTypePath(resolve_ctx, path);
  if (!resolved.ok) {
    return std::nullopt;
  }
  return TypePathOf(resolved.value);
}

std::optional<analysis::TypeRef> LowerType(const SemanticsContext& ctx,
                                       const std::shared_ptr<syntax::Type>& type) {
  if (!type) {
    return std::nullopt;
  }
  return std::visit(
      [&](const auto& node) -> std::optional<analysis::TypeRef> {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::TypePrim>) {
          return analysis::MakeTypePrim(node.name);
        } else if constexpr (std::is_same_v<T, syntax::TypePermType>) {
          const auto perm = LowerPermission(node.perm);
          const auto base = LowerType(ctx, node.base);
          if (!perm.has_value() || !base.has_value()) {
            return std::nullopt;
          }
          return analysis::MakeTypePerm(*perm, *base);
        } else if constexpr (std::is_same_v<T, syntax::TypeUnion>) {
          std::vector<analysis::TypeRef> members;
          members.reserve(node.types.size());
          for (const auto& member : node.types) {
            const auto lowered = LowerType(ctx, member);
            if (!lowered.has_value()) {
              return std::nullopt;
            }
            members.push_back(*lowered);
          }
          return analysis::MakeTypeUnion(std::move(members));
        } else if constexpr (std::is_same_v<T, syntax::TypeFunc>) {
          std::vector<analysis::TypeFuncParam> params;
          params.reserve(node.params.size());
          for (const auto& param : node.params) {
            const auto lowered = LowerType(ctx, param.type);
            if (!lowered.has_value()) {
              return std::nullopt;
            }
            analysis::TypeFuncParam out_param;
            if (param.mode.has_value()) {
              out_param.mode = analysis::ParamMode::Move;
            }
            out_param.type = *lowered;
            params.push_back(std::move(out_param));
          }
          const auto ret = LowerType(ctx, node.ret);
          if (!ret.has_value()) {
            return std::nullopt;
          }
          return analysis::MakeTypeFunc(std::move(params), *ret);
        } else if constexpr (std::is_same_v<T, syntax::TypeTuple>) {
          std::vector<analysis::TypeRef> elements;
          elements.reserve(node.elements.size());
          for (const auto& elem : node.elements) {
            const auto lowered = LowerType(ctx, elem);
            if (!lowered.has_value()) {
              return std::nullopt;
            }
            elements.push_back(*lowered);
          }
          return analysis::MakeTypeTuple(std::move(elements));
        } else if constexpr (std::is_same_v<T, syntax::TypeArray>) {
          const auto elem = LowerType(ctx, node.element);
          if (!elem.has_value() || !ctx.sema) {
            return std::nullopt;
          }
          const auto len = analysis::ConstLen(*ctx.sema, node.length);
          if (!len.ok || !len.value.has_value()) {
            return std::nullopt;
          }
          return analysis::MakeTypeArray(*elem, *len.value);
        } else if constexpr (std::is_same_v<T, syntax::TypeSlice>) {
          const auto elem = LowerType(ctx, node.element);
          if (!elem.has_value()) {
            return std::nullopt;
          }
          return analysis::MakeTypeSlice(*elem);
        } else if constexpr (std::is_same_v<T, syntax::TypePtr>) {
          const auto elem = LowerType(ctx, node.element);
          if (!elem.has_value()) {
            return std::nullopt;
          }
          return analysis::MakeTypePtr(*elem, LowerPtrState(node.state));
        } else if constexpr (std::is_same_v<T, syntax::TypeRawPtr>) {
          const auto elem = LowerType(ctx, node.element);
          const auto qual = LowerRawPtrQual(node.qual);
          if (!elem.has_value() || !qual.has_value()) {
            return std::nullopt;
          }
          return analysis::MakeTypeRawPtr(*qual, *elem);
        } else if constexpr (std::is_same_v<T, syntax::TypeString>) {
          return analysis::MakeTypeString(LowerStringState(node.state));
        } else if constexpr (std::is_same_v<T, syntax::TypeBytes>) {
          return analysis::MakeTypeBytes(LowerBytesState(node.state));
        } else if constexpr (std::is_same_v<T, syntax::TypeDynamic>) {
          analysis::TypePath path(node.path.begin(), node.path.end());
          return analysis::MakeTypeDynamic(std::move(path));
        } else if constexpr (std::is_same_v<T, syntax::TypeModalState>) {
          const auto path = ResolveTypePath(ctx, node.path);
          if (!path.has_value()) {
            return std::nullopt;
          }
          return analysis::MakeTypeModalState(*path, node.state);
        } else if constexpr (std::is_same_v<T, syntax::TypePathType>) {
          const auto path = ResolveTypePath(ctx, node.path);
          if (!path.has_value()) {
            return std::nullopt;
          }
          return analysis::MakeTypePath(*path);
        }
        return std::nullopt;
      },
      type->node);
}

analysis::TypeRef ReturnTypeOf(const SemanticsContext& ctx,
                           const std::shared_ptr<syntax::Type>& type_opt) {
  if (!type_opt) {
    return analysis::MakeTypePrim("()");
  }
  const auto lowered = LowerType(ctx, type_opt);
  if (lowered.has_value()) {
    return *lowered;
  }
  return analysis::MakeTypePrim("()");
}

analysis::TypeRef ProcReturnType(const SemanticsContext& ctx,
                             const syntax::ProcedureDecl& proc) {
  if (!ctx.sema) {
    return ReturnTypeOf(ctx, proc.return_type_opt);
  }
  const auto res = analysis::ProcType(*ctx.sema, proc);
  if (!res.ok || !res.type) {
    return ReturnTypeOf(ctx, proc.return_type_opt);
  }
  if (const auto* fn = std::get_if<analysis::TypeFunc>(&res.type->node)) {
    return fn->ret;
  }
  return ReturnTypeOf(ctx, proc.return_type_opt);
}

const syntax::RecordDecl* FindRecordDecl(const analysis::ScopeContext& ctx,
                                         const analysis::TypePath& path) {
  const auto key = analysis::PathKeyOf(path);
  const auto it = ctx.sigma.types.find(key);
  if (it == ctx.sigma.types.end()) {
    return nullptr;
  }
  return std::get_if<syntax::RecordDecl>(&it->second);
}

}  // namespace

Outcome ApplyRegionProc(const SemanticsContext& ctx,
                        std::string_view name,
                        const std::vector<BindingValue>& args,
                        Sigma& sigma) {
  (void)ctx;
  if (!IsRegionProcName(name)) {
    return PanicOutcome();
  }
  analysis::TypePath module_path;
  module_path.emplace_back("Region");
  const auto result = BuiltinCall(module_path, name, args, sigma);
  if (!result.has_value()) {
    return PanicOutcome();
  }
  if (name == "new_scoped") {
    SPEC_RULE("ApplyRegionProc-NewScoped");
  } else if (name == "alloc") {
    SPEC_RULE("ApplyRegionProc-Alloc");
  } else if (name == "reset_unchecked") {
    SPEC_RULE("ApplyRegionProc-Reset");
  } else if (name == "freeze") {
    SPEC_RULE("ApplyRegionProc-Freeze");
  } else if (name == "thaw") {
    SPEC_RULE("ApplyRegionProc-Thaw");
  } else if (name == "free_unchecked") {
    SPEC_RULE("ApplyRegionProc-Free");
  } else {
    return PanicOutcome();
  }
  return MakeVal(*result);
}

Outcome ApplyProcSigma(const SemanticsContext& ctx,
                       const syntax::ProcedureDecl& proc,
                       const std::vector<BindingValue>& args,
                       Sigma& sigma) {
  if (proc.params.size() != args.size()) {
    return PanicOutcome();
  }
  ScopeEntry scope;
  if (!PushScope(sigma, &scope)) {
    return PanicOutcome();
  }
  const ScopeId scope_id = scope.id;
  for (std::size_t i = 0; i < proc.params.size(); ++i) {
    const auto& param = proc.params[i];
    const BindInfo info = BindInfoForParam(param);
    if (!BindVal(sigma, param.name, args[i], info, nullptr)) {
      return BlockExit(ctx, scope_id, PanicOutcome(), sigma);
    }
  }
  SemanticsContext inner = ctx;
  inner.ret_type = ProcReturnType(ctx, proc);
  Outcome out = EvalBlockBodySigma(inner, *proc.body, sigma);
  Outcome out2 = BlockExit(inner, scope_id, out, sigma);
  SPEC_RULE("ApplyProcSigma");
  return ReturnOut(out2);
}

Outcome ApplyRecordCtorSigma(const SemanticsContext& ctx,
                             const analysis::TypePath& path,
                             Sigma& sigma) {
  if (!ctx.sema) {
    return PanicOutcome();
  }
  const auto key = analysis::PathKeyOf(path);
  const auto it = ctx.sema->sigma.types.find(key);
  if (it == ctx.sema->sigma.types.end()) {
    return PanicOutcome();
  }
  const auto* record_decl = std::get_if<syntax::RecordDecl>(&it->second);
  if (!record_decl) {
    return PanicOutcome();
  }
  std::vector<syntax::FieldInit> fields;
  for (const auto& member : record_decl->members) {
    const auto* field = std::get_if<syntax::FieldDecl>(&member);
    if (!field) {
      continue;
    }
    if (!field->init_opt) {
      return PanicOutcome();
    }
    syntax::FieldInit init;
    init.name = field->name;
    init.value = field->init_opt;
    init.span = field->span;
    fields.push_back(std::move(init));
  }
  auto eval_out = EvalFieldInitsSigma(ctx, fields, sigma);
  if (std::holds_alternative<Control>(eval_out)) {
    SPEC_RULE("ApplyRecordCtorSigma-Ctrl");
    return MakeCtrl(std::get<Control>(eval_out));
  }
  RecordVal record;
  record.record_type = analysis::MakeTypePath(path);
  record.fields = std::move(
      std::get<std::vector<std::pair<std::string, Value>>>(eval_out));
  SPEC_RULE("ApplyRecordCtorSigma");
  return MakeVal(Value{record});
}

Outcome ApplyMethodSigma(const SemanticsContext& ctx,
                         const syntax::Expr& base,
                         std::string_view name,
                         const Value& self_value,
                         const BindingValue& self_arg,
                         const std::vector<BindingValue>& args,
                         const MethodTarget& target,
                         Sigma& sigma) {
  (void)base;
  const syntax::Block* body = nullptr;
  std::vector<syntax::Param> params;
  bool recv_move = false;
  analysis::TypeRef ret_type = analysis::MakeTypePrim("()");

  if (target.kind == MethodTarget::Kind::Record) {
    if (!target.record_method) {
      return PanicOutcome();
    }
    const auto& method = *target.record_method;
    if (const auto* explicit_recv =
            std::get_if<syntax::ReceiverExplicit>(&method.receiver)) {
      recv_move = explicit_recv->mode_opt.has_value();
    }
    params = method.params;
    ret_type = ReturnTypeOf(ctx, method.return_type_opt);
    body = method.body.get();
  } else if (target.kind == MethodTarget::Kind::State) {
    if (!target.state_method) {
      return PanicOutcome();
    }
    const auto& method = *target.state_method;
    params = method.params;
    ret_type = ReturnTypeOf(ctx, method.return_type_opt);
    body = method.body.get();
  } else {
    if (!target.class_method) {
      std::vector<Value> arg_values;
      arg_values.reserve(args.size());
      for (const auto& arg : args) {
        if (const auto* val = std::get_if<Value>(&arg)) {
          arg_values.push_back(*val);
          continue;
        }
        const auto alias = std::get<Alias>(arg);
        const auto value = ReadAddr(sigma, alias.addr);
        if (!value.has_value()) {
          return PanicOutcome();
        }
        arg_values.push_back(*value);
      }
      if (IsSystemExit(target.owner_class, name)) {
        Control ctrl;
        ctrl.kind = ControlKind::Abort;
        ctrl.value = std::nullopt;
        SPEC_RULE("ApplyMethodSigma-Prim");
        SPEC_RULE("ApplyMethod-Prim");
        SPEC_RULE("ApplyMethod-Prim-Step");
        return MakeCtrl(ctrl);
      }
      const auto prim = PrimCall(target.owner_class, name, self_value, arg_values, sigma);
      if (!prim.has_value()) {
        return PanicOutcome();
      }
      SPEC_RULE("ApplyMethodSigma-Prim");
      SPEC_RULE("ApplyMethod-Prim");
      SPEC_RULE("ApplyMethod-Prim-Step");
      return MakeVal(*prim);
    }
    const auto& method = *target.class_method;
    if (!method.body_opt) {
      std::vector<Value> arg_values;
      arg_values.reserve(args.size());
      for (const auto& arg : args) {
        if (const auto* val = std::get_if<Value>(&arg)) {
          arg_values.push_back(*val);
          continue;
        }
        const auto alias = std::get<Alias>(arg);
        const auto value = ReadAddr(sigma, alias.addr);
        if (!value.has_value()) {
          return PanicOutcome();
        }
        arg_values.push_back(*value);
      }
      if (IsSystemExit(target.owner_class, name)) {
        Control ctrl;
        ctrl.kind = ControlKind::Abort;
        ctrl.value = std::nullopt;
        SPEC_RULE("ApplyMethodSigma-Prim");
        SPEC_RULE("ApplyMethod-Prim");
        SPEC_RULE("ApplyMethod-Prim-Step");
        return MakeCtrl(ctrl);
      }
      const auto prim = PrimCall(target.owner_class, name, self_value, arg_values, sigma);
      if (!prim.has_value()) {
        return PanicOutcome();
      }
      SPEC_RULE("ApplyMethodSigma-Prim");
      SPEC_RULE("ApplyMethod-Prim");
      SPEC_RULE("ApplyMethod-Prim-Step");
      return MakeVal(*prim);
    }
    if (const auto* explicit_recv =
            std::get_if<syntax::ReceiverExplicit>(&method.receiver)) {
      recv_move = explicit_recv->mode_opt.has_value();
    }
    params = method.params;
    ret_type = ReturnTypeOf(ctx, method.return_type_opt);
    body = method.body_opt.get();
  }

  if (target.kind == MethodTarget::Kind::State) {
    const auto* rec = std::get_if<RecordVal>(&self_value.node);
    const auto* modal = (rec && rec->record_type)
        ? std::get_if<analysis::TypeModalState>(&rec->record_type->node)
        : nullptr;
    if (modal && modal->path.size() == 1 &&
        (modal->path[0] == "File" || modal->path[0] == "DirIter")) {
      std::vector<Value> arg_values;
      arg_values.reserve(args.size());
      for (const auto& arg : args) {
        if (const auto* val = std::get_if<Value>(&arg)) {
          arg_values.push_back(*val);
          continue;
        }
        const auto alias = std::get<Alias>(arg);
        const auto value = ReadAddr(sigma, alias.addr);
        if (!value.has_value()) {
          return PanicOutcome();
        }
        arg_values.push_back(*value);
      }
      const auto prim = PrimCall(modal->path, name, self_value, arg_values, sigma);
      if (!prim.has_value()) {
        return PanicOutcome();
      }
      SPEC_RULE("ApplyMethodSigma-Prim");
      SPEC_RULE("ApplyMethod-Prim");
      SPEC_RULE("ApplyMethod-Prim-Step");
      return MakeVal(*prim);
    }
  }

  if (!body) {
    return PanicOutcome();
  }

  if (params.size() != args.size()) {
    return PanicOutcome();
  }

  ScopeEntry scope;
  if (!PushScope(sigma, &scope)) {
    return PanicOutcome();
  }
  const ScopeId scope_id = scope.id;
  const BindInfo self_info = BindInfoForRecv(recv_move);
  if (!BindVal(sigma, "self", self_arg, self_info, nullptr)) {
    return BlockExit(ctx, scope_id, PanicOutcome(), sigma);
  }
  for (std::size_t i = 0; i < params.size(); ++i) {
    const auto& param = params[i];
    const BindInfo info = BindInfoForParam(param);
    if (!BindVal(sigma, param.name, args[i], info, nullptr)) {
      return BlockExit(ctx, scope_id, PanicOutcome(), sigma);
    }
  }

  SemanticsContext inner = ctx;
  inner.ret_type = ret_type;
  Outcome out = EvalBlockBodySigma(inner, *body, sigma);
  Outcome out2 = BlockExit(inner, scope_id, out, sigma);
  SPEC_RULE("ApplyMethodSigma");
  return ReturnOut(out2);
}

}  // namespace cursive0::eval
