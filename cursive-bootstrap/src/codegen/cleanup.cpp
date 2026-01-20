#include "cursive0/codegen/cleanup.h"

#include "cursive0/codegen/checks.h"
#include "cursive0/codegen/abi.h"
#include "cursive0/codegen/layout.h"
#include "cursive0/codegen/mangle.h"
#include "cursive0/codegen/globals.h"
#include "cursive0/runtime/runtime_interface.h"
#include "cursive0/core/assert_spec.h"
#include "cursive0/core/symbols.h"
#include "cursive0/sema/classes.h"
#include "cursive0/sema/scopes.h"
#include "cursive0/sema/type_expr.h"

#include <algorithm>
#include <functional>
#include <cassert>
#include <cstdint>
#include <unordered_set>

namespace cursive0::codegen {

// ============================================================================
// Helper functions
// ============================================================================

// Helpers moved to lower_expr.h

// Helper to check if a TypeRef holds a specific type
template <typename T>
static bool HoldsType(const sema::TypeRef& type) {
  return type && std::holds_alternative<T>(type->node);
}

template <typename T>
static const T& GetType(const sema::TypeRef& type) {
  return std::get<T>(type->node);
}

struct StaticBindFlags {
  bool has_responsibility = false;
  bool immovable = false;
};

static bool IsMoveExprLite(const syntax::ExprPtr& expr) {
  return expr && std::holds_alternative<syntax::MoveExpr>(expr->node);
}

static bool IsPlaceExprLite(const syntax::ExprPtr& expr) {
  if (!expr) {
    return false;
  }
  return std::visit(
      [&](const auto& node) -> bool {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
          return true;
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
          return true;
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
          return true;
        } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
          return true;
        } else if constexpr (std::is_same_v<T, syntax::DerefExpr>) {
          return IsPlaceExprLite(node.value);
        }
        return false;
      },
      expr->node);
}

static std::optional<StaticBindFlags> StaticBindFlagsFor(
    const syntax::ModulePath& module_path,
    const std::string& name,
    LowerCtx& ctx) {
  if (!ctx.sigma) {
    return std::nullopt;
  }

  const syntax::ASTModule* module = nullptr;
  for (const auto& mod : ctx.sigma->mods) {
    if (sema::PathEq(mod.path, module_path)) {
      module = &mod;
      break;
    }
  }
  if (!module) {
    return std::nullopt;
  }

  for (const auto& item : module->items) {
    const auto* decl = std::get_if<syntax::StaticDecl>(&item);
    if (!decl) {
      continue;
    }
    const auto names = StaticBindList(decl->binding);
    if (std::find(names.begin(), names.end(), name) == names.end()) {
      continue;
    }

    bool has_resp = true;
    if (decl->binding.init && IsPlaceExprLite(decl->binding.init) &&
        !IsMoveExprLite(decl->binding.init)) {
      has_resp = false;
    }
    bool immovable = decl->binding.op.lexeme == ":=" || !has_resp;
    return StaticBindFlags{has_resp, immovable};
  }

  return std::nullopt;
}



static std::optional<sema::TypeRef> LowerTypeForDrop(
    const std::shared_ptr<syntax::Type>& type,
    LowerCtx& ctx) {
  if (!type || !ctx.sigma) {
    return std::nullopt;
  }
  sema::ScopeContext scope;
  scope.sigma = *ctx.sigma;
  scope.current_module = ctx.module_path;
  return LowerTypeForLayout(scope, type);
}

static syntax::Path ToSyntaxPath(const sema::TypePath& path) {
  syntax::Path out;
  out.reserve(path.size());
  for (const auto& seg : path) {
    out.push_back(seg);
  }
  return out;
}

static const syntax::StateBlock* FindModalState(
    const syntax::ModalDecl& decl,
    std::string_view name) {
  for (const auto& state : decl.states) {
    if (sema::IdEq(state.name, name)) {
      return &state;
    }
  }
  return nullptr;
}

static std::optional<std::vector<std::pair<std::string, sema::TypeRef>>>
CollectModalFields(const syntax::StateBlock& state, LowerCtx& ctx) {
  std::vector<std::pair<std::string, sema::TypeRef>> fields;
  for (const auto& member : state.members) {
    const auto* field = std::get_if<syntax::StateFieldDecl>(&member);
    if (!field) {
      continue;
    }
    const auto lowered = LowerTypeForDrop(field->type, ctx);
    if (!lowered.has_value()) {
      return std::nullopt;
    }
    fields.emplace_back(field->name, *lowered);
  }
  return fields;
}

static IRValue BoolImmediate(bool value) {
  IRValue v;
  v.kind = IRValue::Kind::Immediate;
  v.name = value ? "true" : "false";
  v.bytes = {static_cast<std::uint8_t>(value ? 1 : 0)};
  return v;
}

static IRValue U32Immediate(std::uint32_t value) {
  IRValue v;
  v.kind = IRValue::Kind::Immediate;
  v.name = std::to_string(value);
  v.bytes = {
      static_cast<std::uint8_t>(value & 0xFFu),
      static_cast<std::uint8_t>((value >> 8) & 0xFFu),
      static_cast<std::uint8_t>((value >> 16) & 0xFFu),
      static_cast<std::uint8_t>((value >> 24) & 0xFFu),
  };
  return v;
}

static IRValue USizeImmediate(std::size_t value) {
  IRValue v;
  v.kind = IRValue::Kind::Immediate;
  v.name = std::to_string(value);
  const std::uint64_t encoded = static_cast<std::uint64_t>(value);
  v.bytes = {
      static_cast<std::uint8_t>(encoded & 0xFFu),
      static_cast<std::uint8_t>((encoded >> 8) & 0xFFu),
      static_cast<std::uint8_t>((encoded >> 16) & 0xFFu),
      static_cast<std::uint8_t>((encoded >> 24) & 0xFFu),
      static_cast<std::uint8_t>((encoded >> 32) & 0xFFu),
      static_cast<std::uint8_t>((encoded >> 40) & 0xFFu),
      static_cast<std::uint8_t>((encoded >> 48) & 0xFFu),
      static_cast<std::uint8_t>((encoded >> 56) & 0xFFu),
  };
  return v;
}

static IRValue UnitValue() {
  IRValue v;
  v.kind = IRValue::Kind::Opaque;
  v.name = "unit";
  return v;
}

static bool IsNoopIR(const IRPtr& ir) {
  return !ir || std::holds_alternative<IROpaque>(ir->node);
}

struct PanicAccess {
  IRValue panic_ptr;
  IRValue flag_ptr;
  IRValue code_ptr;
};

static PanicAccess BuildPanicAccess(LowerCtx& ctx) {
  IRValue panic_ptr = ctx.FreshTempValue("panic_ptr");
  DerivedValueInfo base_info;
  base_info.kind = DerivedValueInfo::Kind::AddrDeref;
  base_info.base.kind = IRValue::Kind::Local;
  base_info.base.name = std::string(kPanicOutName);
  ctx.RegisterDerivedValue(panic_ptr, base_info);
  ctx.RegisterValueType(panic_ptr, PanicOutType());

  IRValue flag_ptr = ctx.FreshTempValue("panic_flag_ptr");
  DerivedValueInfo flag_info;
  flag_info.kind = DerivedValueInfo::Kind::AddrTuple;
  flag_info.base = panic_ptr;
  flag_info.tuple_index = 0;
  ctx.RegisterDerivedValue(flag_ptr, flag_info);
  ctx.RegisterValueType(flag_ptr,
                        sema::MakeTypeRawPtr(sema::RawPtrQual::Mut,
                                             sema::MakeTypePrim("bool")));

  IRValue code_ptr = ctx.FreshTempValue("panic_code_ptr");
  DerivedValueInfo code_info;
  code_info.kind = DerivedValueInfo::Kind::AddrTuple;
  code_info.base = panic_ptr;
  code_info.tuple_index = 1;
  ctx.RegisterDerivedValue(code_ptr, code_info);
  ctx.RegisterValueType(code_ptr,
                        sema::MakeTypeRawPtr(sema::RawPtrQual::Mut,
                                             sema::MakeTypePrim("u32")));

  return PanicAccess{panic_ptr, flag_ptr, code_ptr};
}

struct PanicSnapshot {
  IRPtr ir;
  IRValue flag;
  IRValue code;
};

static PanicSnapshot ReadPanicRecord(const PanicAccess& access, LowerCtx& ctx) {
  IRValue flag = ctx.FreshTempValue("panic_flag");
  ctx.RegisterValueType(flag, sema::MakeTypePrim("bool"));
  IRReadPtr read_flag;
  read_flag.ptr = access.flag_ptr;
  read_flag.result = flag;

  IRValue code = ctx.FreshTempValue("panic_code");
  ctx.RegisterValueType(code, sema::MakeTypePrim("u32"));
  IRReadPtr read_code;
  read_code.ptr = access.code_ptr;
  read_code.result = code;

  IRPtr ir = SeqIR({MakeIR(std::move(read_flag)), MakeIR(std::move(read_code))});
  return PanicSnapshot{ir, flag, code};
}

static IRPtr WritePanicRecord(const PanicAccess& access,
                              const IRValue& flag,
                              const IRValue& code) {
  IRWritePtr write_flag;
  write_flag.ptr = access.flag_ptr;
  write_flag.value = flag;

  IRWritePtr write_code;
  write_code.ptr = access.code_ptr;
  write_code.value = code;

  return SeqIR({MakeIR(std::move(write_flag)), MakeIR(std::move(write_code))});
}

// ============================================================================
// §6.8 CleanupPlan - Compute cleanup actions for a scope
// ============================================================================

static void AppendCleanupItemToPlan(const CleanupItem& item,
                                    LowerCtx& ctx,
                                    CleanupPlan& plan) {
  switch (item.kind) {
    case CleanupItem::Kind::DropBinding: {
      const BindingState* state = ctx.GetBindingState(item.name);
      if (!state) {
        return;
      }
      if (!state->has_responsibility) {
        return;
      }

      BindValidity validity = GetBindValidity(item.name, ctx);
      if (validity == BindValidity::Moved) {
        return;
      }

      CleanupAction action;
      action.kind = CleanupAction::Kind::DropVar;
      action.name = item.name;
      action.type = state->type;

      if (validity == BindValidity::PartiallyMoved) {
        action.skip_fields = GetMovedFields(item.name, ctx);
      }

      plan.push_back(std::move(action));
      return;
    }
    case CleanupItem::Kind::DeferBlock: {
      CleanupAction action;
      action.kind = CleanupAction::Kind::RunDefer;
      action.defer_ir = item.defer_ir;
      plan.push_back(std::move(action));
      return;
    }
  }
}

static CleanupPlan ComputeCleanupPlanForScopes(LowerCtx& ctx, bool stop_at_loop) {
  CleanupPlan plan;
  LowerCtx tmp = ctx;

  while (!tmp.scope_stack.empty()) {
    const bool is_loop = tmp.scope_stack.back().is_loop;
    const auto items = tmp.scope_stack.back().cleanup_items;
    for (auto it = items.rbegin(); it != items.rend(); ++it) {
      AppendCleanupItemToPlan(*it, tmp, plan);
    }
    tmp.PopScope();
    if (stop_at_loop && is_loop) {
      break;
    }
  }

  return plan;
}

CleanupPlan ComputeCleanupPlan(const std::vector<CleanupItem>& cleanup_items,
                               LowerCtx& ctx) {
  SPEC_RULE("CleanupPlan");

  CleanupPlan plan;
  for (const auto& item : cleanup_items) {
    AppendCleanupItemToPlan(item, ctx, plan);
  }

  return plan;
}

CleanupPlan ComputeCleanupPlanForCurrentScope(LowerCtx& ctx) {
  if (ctx.scope_stack.empty()) {
    return {};
  }
  const auto& items = ctx.scope_stack.back().cleanup_items;
  std::vector<CleanupItem> ordered(items.rbegin(), items.rend());
  return ComputeCleanupPlan(ordered, ctx);
}

CleanupPlan ComputeCleanupPlanToLoopScope(LowerCtx& ctx) {
  return ComputeCleanupPlanForScopes(ctx, true);
}

CleanupPlan ComputeCleanupPlanToFunctionRoot(LowerCtx& ctx) {
  return ComputeCleanupPlanForScopes(ctx, false);
}

CleanupPlan ComputeCleanupPlanRemainder(CleanupTarget target, LowerCtx& ctx) {
  LowerCtx tmp = ctx;
  switch (target) {
    case CleanupTarget::CurrentScope: {
      if (!tmp.scope_stack.empty()) {
        tmp.PopScope();
      }
      return ComputeCleanupPlanToFunctionRoot(tmp);
    }
    case CleanupTarget::ToLoopScope: {
      while (!tmp.scope_stack.empty()) {
        const bool is_loop = tmp.scope_stack.back().is_loop;
        tmp.PopScope();
        if (is_loop) {
          break;
        }
      }
      return ComputeCleanupPlanToFunctionRoot(tmp);
    }
    case CleanupTarget::ToFunctionRoot:
    default:
      return {};
  }
}


// ============================================================================
// A6.8 EmitCleanup - Emit IR for a cleanup plan
// ============================================================================

static IRPtr EmitCleanupAction(const CleanupAction& action, LowerCtx& ctx) {
  switch (action.kind) {
    case CleanupAction::Kind::DropVar: {
      IRValue var_value;
      var_value.kind = IRValue::Kind::Local;
      var_value.name = action.name;

      if (action.skip_fields.empty()) {
        return EmitDrop(action.type, var_value, ctx);
      }
      return EmitDropFields(action.type, var_value, action.skip_fields, ctx);
    }
    case CleanupAction::Kind::DropTemp: {
      if (action.value) {
        return EmitDrop(action.type, *action.value, ctx);
      }
      return EmptyIR();
    }
    case CleanupAction::Kind::DropField: {
      if (action.value) {
        return EmitDrop(action.type, *action.value, ctx);
      }
      return EmptyIR();
    }
    case CleanupAction::Kind::ReleaseRegion: {
      IRValue region_value;
      if (action.value) {
        region_value = *action.value;
      } else {
        region_value.kind = IRValue::Kind::Local;
        region_value.name = action.name;
      }

      IRCall call;
      call.callee.kind = IRValue::Kind::Symbol;
      call.callee.name = "cursive::runtime::region::free_unchecked";
      call.args.push_back(region_value);
      return MakeIR(std::move(call));
    }
    case CleanupAction::Kind::RunDefer: {
      if (action.defer_ir) {
        return *action.defer_ir;
      }
      return EmptyIR();
    }
  }
  return EmptyIR();
}

static IRPtr EmitCleanupImpl(const CleanupPlan& plan,
                             const CleanupPlan& remainder,
                             LowerCtx& ctx,
                             bool start_panicking,
                             bool emit_panic_check) {
  SPEC_RULE("EmitCleanup");

  if (plan.empty()) {
    return EmptyIR();
  }

  const PanicAccess access = BuildPanicAccess(ctx);

  IRValue panicking = BoolImmediate(start_panicking);
  IRValue panic_code = U32Immediate(0);

  std::vector<IRPtr> parts;

  if (start_panicking) {
    auto snapshot = ReadPanicRecord(access, ctx);
    parts.push_back(snapshot.ir);
    panic_code = snapshot.code;
  }

  for (const auto& action : plan) {
    IRPtr action_ir = EmitCleanupAction(action, ctx);
    if (IsNoopIR(action_ir)) {
      continue;
    }

    // Clear panic before running each cleanup action.
    parts.push_back(MakeIR(IRClearPanic{}));
    parts.push_back(action_ir);

    // Read panic flag/code after the action.
    auto snapshot = ReadPanicRecord(access, ctx);
    parts.push_back(snapshot.ir);
    IRValue flag = snapshot.flag;
    IRValue code = snapshot.code;

    // Double panic: abort.
    IRValue double_cond = ctx.FreshTempValue("double_panic");
    IRBinaryOp double_and;
    double_and.op = "&&";
    double_and.lhs = panicking;
    double_and.rhs = flag;
    double_and.result = double_cond;
    parts.push_back(MakeIR(std::move(double_and)));

    IRCall panic_call;
    panic_call.callee.kind = IRValue::Kind::Symbol;
    panic_call.callee.name = RuntimePanicSym();
    panic_call.args.push_back(code);
    panic_call.result = ctx.FreshTempValue("panic_abort");

    IRIf if_double;
    if_double.cond = double_cond;
    if_double.then_ir = MakeIR(std::move(panic_call));
    if_double.then_value = UnitValue();
    if_double.else_ir = EmptyIR();
    if_double.else_value = UnitValue();
    if_double.result = ctx.FreshTempValue("panic_double_if");
    parts.push_back(MakeIR(std::move(if_double)));

    // Restore panic record when continuing cleanup during a panic.
    IRValue not_flag = ctx.FreshTempValue("panic_clear");
    IRUnaryOp not_op;
    not_op.op = "!";
    not_op.operand = flag;
    not_op.result = not_flag;
    parts.push_back(MakeIR(std::move(not_op)));

    IRValue restore_cond = ctx.FreshTempValue("panic_restore");
    IRBinaryOp restore_and;
    restore_and.op = "&&";
    restore_and.lhs = panicking;
    restore_and.rhs = not_flag;
    restore_and.result = restore_cond;
    parts.push_back(MakeIR(std::move(restore_and)));

    IRPtr restore_ir = WritePanicRecord(access, BoolImmediate(true), panic_code);
    IRIf if_restore;
    if_restore.cond = restore_cond;
    if_restore.then_ir = restore_ir;
    if_restore.then_value = UnitValue();
    if_restore.else_ir = EmptyIR();
    if_restore.else_value = UnitValue();
    if_restore.result = ctx.FreshTempValue("panic_restore_if");
    parts.push_back(MakeIR(std::move(if_restore)));

    // Update panicking state.
    IRValue new_panicking = ctx.FreshTempValue("panicking");
    IRBinaryOp or_op;
    or_op.op = "||";
    or_op.lhs = panicking;
    or_op.rhs = flag;
    or_op.result = new_panicking;
    parts.push_back(MakeIR(std::move(or_op)));
    panicking = new_panicking;

    // Update panic code when a new panic occurs.
    IRValue new_code = ctx.FreshTempValue("panic_code_sel");
    IRIf code_if;
    code_if.cond = flag;
    code_if.then_ir = EmptyIR();
    code_if.then_value = code;
    code_if.else_ir = EmptyIR();
    code_if.else_value = panic_code;
    code_if.result = new_code;
    parts.push_back(MakeIR(std::move(code_if)));
    panic_code = new_code;
  }

  if (emit_panic_check) {
    IRPanicCheck check;
    check.cleanup_ir =
        remainder.empty() ? EmptyIR() : EmitCleanupOnPanic(remainder, ctx);
    parts.push_back(MakeIR(std::move(check)));
  }

  return SeqIR(std::move(parts));
}

IRPtr EmitCleanup(const CleanupPlan& plan, LowerCtx& ctx) {
  return EmitCleanupWithRemainder(plan, {}, ctx);
}

IRPtr EmitCleanupOnPanic(const CleanupPlan& plan, LowerCtx& ctx) {
  return EmitCleanupImpl(plan, {}, ctx, true, false);
}

IRPtr EmitCleanupWithRemainder(const CleanupPlan& plan,
                               const CleanupPlan& remainder,
                               LowerCtx& ctx) {
  return EmitCleanupImpl(plan, remainder, ctx, false, true);
}

static IRPtr SeqWithPanicStop(std::vector<IRPtr> drops, LowerCtx& ctx) {
  drops.erase(std::remove_if(drops.begin(), drops.end(), IsNoopIR), drops.end());
  if (drops.empty()) {
    return EmptyIR();
  }
  if (drops.size() == 1) {
    return drops.front();
  }

  const PanicAccess access = BuildPanicAccess(ctx);
  IRPtr tail = drops.back();
  for (std::size_t i = drops.size() - 1; i-- > 0;) {
    IRValue flag = ctx.FreshTempValue("drop_panic_flag");
    ctx.RegisterValueType(flag, sema::MakeTypePrim("bool"));
    IRReadPtr read_flag;
    read_flag.ptr = access.flag_ptr;
    read_flag.result = flag;

    IRValue no_panic = ctx.FreshTempValue("drop_no_panic");
    IRUnaryOp not_op;
    not_op.op = "!";
    not_op.operand = flag;
    not_op.result = no_panic;

    IRIf if_ir;
    if_ir.cond = no_panic;
    if_ir.then_ir = tail;
    if_ir.then_value = UnitValue();
    if_ir.else_ir = EmptyIR();
    if_ir.else_value = UnitValue();
    if_ir.result = ctx.FreshTempValue("drop_seq");

    tail = SeqIR({drops[i],
                  MakeIR(std::move(read_flag)),
                  MakeIR(std::move(not_op)),
                  MakeIR(std::move(if_ir))});
  }

  return tail;
}

static IRPtr EmitDropMethodCall(const sema::TypeRef& type,
                                const IRValue& value,
                                const std::optional<IRValue>& panic_out,
                                LowerCtx& ctx) {
  if (!ctx.sigma) {
    return EmptyIR();
  }

  sema::TypeRef stripped = sema::StripPerm(type);
  if (!stripped) {
    return EmptyIR();
  }

  sema::ScopeContext scope;
  scope.sigma = *ctx.sigma;
  scope.current_module = ctx.module_path;

  syntax::ClassPath drop_path;
  drop_path.push_back("Drop");
  if (!sema::TypeImplementsClass(scope, stripped, drop_path)) {
    return EmptyIR();
  }

  auto sym = MethodSymbol(scope, stripped, "drop");
  if (!sym.has_value()) {
    return EmptyIR();
  }

  IRCall call;
  call.callee.kind = IRValue::Kind::Symbol;
  call.callee.name = *sym;
  call.args.push_back(value);
  if (panic_out.has_value() && NeedsPanicOut(*sym)) {
    call.args.push_back(*panic_out);
  }
  return MakeIR(std::move(call));
}
// ============================================================================
// §6.8 EmitDrop - Emit IR to drop a value of a given type
// ============================================================================

static IRPtr EmitDropImpl(const sema::TypeRef& type,
                          const IRValue& value,
                          LowerCtx& ctx,
                          bool allow_drop_glue,
                          const std::optional<IRValue>& panic_out) {
  if (!type) {
    return EmptyIR();
  }

  if (value.kind == IRValue::Kind::Opaque) {
    ctx.RegisterValueType(value, type);
  }

  if (HoldsType<sema::TypePerm>(type)) {
    const auto& perm = GetType<sema::TypePerm>(type);
    return EmitDropImpl(perm.base, value, ctx, allow_drop_glue, panic_out);
  }

  auto call_drop_glue = [&]() -> IRPtr {
    std::string drop_sym = DropGlueSym(type, ctx);
    IRCall call;
    call.callee.kind = IRValue::Kind::Symbol;
    call.callee.name = drop_sym;
    call.args.push_back(value);
    if (panic_out.has_value() && NeedsPanicOut(drop_sym)) {
      call.args.push_back(*panic_out);
    }
    return MakeIR(std::move(call));
  };

  if (HoldsType<sema::TypePrim>(type)) {
    return EmptyIR();
  }

  if (HoldsType<sema::TypeRawPtr>(type)) {
    return EmptyIR();
  }

  if (HoldsType<sema::TypePtr>(type)) {
    return EmptyIR();
  }

  if (HoldsType<sema::TypeRange>(type)) {
    return EmptyIR();
  }

  if (HoldsType<sema::TypeFunc>(type)) {
    return EmptyIR();
  }

  if (HoldsType<sema::TypeSlice>(type)) {
    return EmptyIR();
  }

  if (HoldsType<sema::TypeString>(type)) {
    const auto& str_type = GetType<sema::TypeString>(type);
    if (str_type.state.has_value() &&
        *str_type.state == sema::StringState::Managed) {
      const std::string drop_sym = BuiltinSymStringDropManaged();
      if (drop_sym.empty()) {
        SPEC_RULE("StringDropSym-Err");
        ctx.ReportCodegenFailure();
        return EmptyIR();
      }
      IRCall call;
      call.callee.kind = IRValue::Kind::Symbol;
      call.callee.name = drop_sym;
      call.args.push_back(value);
      return MakeIR(std::move(call));
    }
    return EmptyIR();
  }

  if (HoldsType<sema::TypeBytes>(type)) {
    const auto& bytes_type = GetType<sema::TypeBytes>(type);
    if (bytes_type.state.has_value() &&
        *bytes_type.state == sema::BytesState::Managed) {
      const std::string drop_sym = BuiltinSymBytesDropManaged();
      if (drop_sym.empty()) {
        SPEC_RULE("BytesDropSym-Err");
        ctx.ReportCodegenFailure();
        return EmptyIR();
      }
      IRCall call;
      call.callee.kind = IRValue::Kind::Symbol;
      call.callee.name = drop_sym;
      call.args.push_back(value);
      return MakeIR(std::move(call));
    }
    return EmptyIR();
  }

  if (HoldsType<sema::TypeArray>(type)) {
    const auto& arr_type = GetType<sema::TypeArray>(type);
    std::vector<IRPtr> drops;

    for (std::size_t i = arr_type.length; i > 0; --i) {
      const std::size_t index = i - 1;
      IRValue elem;
      elem.kind = IRValue::Kind::Opaque;
      elem.name = value.name + "[" + std::to_string(index) + "]";
      ctx.RegisterValueType(elem, arr_type.element);
      {
        DerivedValueInfo info;
        info.kind = DerivedValueInfo::Kind::Index;
        info.base = value;
        info.index = USizeImmediate(index);
        ctx.RegisterDerivedValue(elem, info);
      }
      drops.push_back(
          EmitDropImpl(arr_type.element, elem, ctx, allow_drop_glue, panic_out));
    }

    return SeqWithPanicStop(std::move(drops), ctx);
  }

  if (HoldsType<sema::TypeTuple>(type)) {
    const auto& tuple_type = GetType<sema::TypeTuple>(type);
    std::vector<IRPtr> drops;

    for (std::size_t i = tuple_type.elements.size(); i > 0; --i) {
      const std::size_t index = i - 1;
      IRValue elem;
      elem.kind = IRValue::Kind::Opaque;
      elem.name = value.name + "_" + std::to_string(index);
      ctx.RegisterValueType(elem, tuple_type.elements[index]);
      {
        DerivedValueInfo info;
        info.kind = DerivedValueInfo::Kind::Tuple;
        info.base = value;
        info.tuple_index = index;
        ctx.RegisterDerivedValue(elem, info);
      }
      drops.push_back(
          EmitDropImpl(tuple_type.elements[index],
                       elem,
                       ctx,
                       allow_drop_glue,
                       panic_out));
    }

    return SeqWithPanicStop(std::move(drops), ctx);
  }

  if (HoldsType<sema::TypeUnion>(type)) {
    const auto& uni_type = GetType<sema::TypeUnion>(type);
    if (uni_type.members.empty()) {
      return EmptyIR();
    }

    std::vector<IRMatchArm> arms;
    arms.reserve(uni_type.members.size());
    for (std::size_t i = 0; i < uni_type.members.size(); ++i) {
      syntax::TypedPattern typed;
      typed.name = "__case" + std::to_string(i);
      typed.type = nullptr;

      auto pattern = std::make_shared<syntax::Pattern>();
      pattern->node = std::move(typed);
      pattern->span = core::Span{};

      IRValue case_val;
      case_val.kind = IRValue::Kind::Opaque;
      case_val.name = value.name + "_case_" + std::to_string(i);
      ctx.RegisterValueType(case_val, uni_type.members[i]);
      {
        DerivedValueInfo info;
        info.kind = DerivedValueInfo::Kind::UnionPayload;
        info.base = value;
        info.union_index = i;
        ctx.RegisterDerivedValue(case_val, info);
      }

      IRPtr body = EmitDropImpl(uni_type.members[i],
                                case_val,
                                ctx,
                                allow_drop_glue,
                                panic_out);

      IRValue unit;
      unit.kind = IRValue::Kind::Opaque;
      unit.name = "()";

      IRMatchArm arm;
      arm.pattern = std::move(pattern);
      arm.body = body;
      arm.value = unit;
      arms.push_back(std::move(arm));
    }

    IRMatch match;
    match.scrutinee = value;
    match.arms = std::move(arms);
    return MakeIR(std::move(match));
  }

  if (HoldsType<sema::TypeModalState>(type)) {
    if (allow_drop_glue) {
      return call_drop_glue();
    }
    if (!ctx.sigma) {
      return EmptyIR();
    }
    const auto& modal_state = GetType<sema::TypeModalState>(type);
    const auto syntax_path = ToSyntaxPath(modal_state.path);
    const auto it = ctx.sigma->types.find(sema::PathKeyOf(syntax_path));
    if (it == ctx.sigma->types.end()) {
      return EmptyIR();
    }
    const auto* decl = std::get_if<syntax::ModalDecl>(&it->second);
    if (!decl) {
      return EmptyIR();
    }
    const auto* state = FindModalState(*decl, modal_state.state);
    if (!state) {
      return EmptyIR();
    }
    const auto fields = CollectModalFields(*state, ctx);
    if (!fields.has_value()) {
      return EmptyIR();
    }

    IRPtr drop_method = EmitDropMethodCall(type, value, panic_out, ctx);
    std::vector<IRPtr> drops;
    drops.reserve(fields->size() + 1);
    if (!IsNoopIR(drop_method)) {
      drops.push_back(drop_method);
    }
    for (auto rit = fields->rbegin(); rit != fields->rend(); ++rit) {
      IRValue field_val;
      field_val.kind = IRValue::Kind::Opaque;
      field_val.name = value.name + "_" + rit->first;
      ctx.RegisterValueType(field_val, rit->second);
      {
        DerivedValueInfo info;
        info.kind = DerivedValueInfo::Kind::ModalField;
        info.base = value;
        info.modal_state = modal_state.state;
        info.field = rit->first;
        ctx.RegisterDerivedValue(field_val, info);
      }
      drops.push_back(
          EmitDropImpl(rit->second, field_val, ctx, allow_drop_glue, panic_out));
    }
    return SeqWithPanicStop(std::move(drops), ctx);
  }

  if (HoldsType<sema::TypeDynamic>(type)) {
    // Spec: DropValue/DropChildren has no dynamic-specific behavior.
    // TypeDynamic does not implement Drop and has no children, so drop is a no-op.
    return EmptyIR();
  }

  if (HoldsType<sema::TypePathType>(type)) {
    if (!ctx.sigma) {
      return EmptyIR();
    }

    const auto& type_path = GetType<sema::TypePathType>(type);
    const auto syntax_path = ToSyntaxPath(type_path.path);
    const auto it = ctx.sigma->types.find(sema::PathKeyOf(syntax_path));
    if (it == ctx.sigma->types.end()) {
      return EmptyIR();
    }

    if (const auto* alias = std::get_if<syntax::TypeAliasDecl>(&it->second)) {
      const auto lowered = LowerTypeForDrop(alias->type, ctx);
      if (!lowered.has_value()) {
        return EmptyIR();
      }
      return EmitDropImpl(*lowered, value, ctx, allow_drop_glue, panic_out);
    }

    if (allow_drop_glue) {
      return call_drop_glue();
    }

    if (std::holds_alternative<syntax::RecordDecl>(it->second)) {
      IRPtr drop_method = EmitDropMethodCall(type, value, panic_out, ctx);
      IRPtr field_drops = EmitDropFields(type, value, {}, ctx);
      return SeqWithPanicStop({drop_method, field_drops}, ctx);
    }

    if (const auto* enum_decl = std::get_if<syntax::EnumDecl>(&it->second)) {
      std::vector<IRMatchArm> arms;
      arms.reserve(enum_decl->variants.size());
      for (const auto& variant : enum_decl->variants) {
        syntax::EnumPattern enum_pat;
        enum_pat.path = syntax_path;
        enum_pat.name = variant.name;
        enum_pat.payload_opt = std::nullopt;

        auto pattern = std::make_shared<syntax::Pattern>();
        pattern->node = std::move(enum_pat);
        pattern->span = core::Span{};

        IRPtr body = EmptyIR();
        if (variant.payload_opt.has_value()) {
          std::vector<IRPtr> drops;
          if (const auto* tuple_payload =
                  std::get_if<syntax::VariantPayloadTuple>(&*variant.payload_opt)) {
            for (std::size_t i = tuple_payload->elements.size(); i > 0; --i) {
              const auto lowered =
                  LowerTypeForDrop(tuple_payload->elements[i - 1], ctx);
              if (!lowered.has_value()) {
                continue;
              }
              const std::size_t index = i - 1;
              IRValue elem;
              elem.kind = IRValue::Kind::Opaque;
              elem.name = value.name + "_payload_" + std::to_string(index);
              ctx.RegisterValueType(elem, *lowered);
              {
                DerivedValueInfo info;
                info.kind = DerivedValueInfo::Kind::EnumPayloadIndex;
                info.base = value;
                info.variant = variant.name;
                info.tuple_index = index;
                ctx.RegisterDerivedValue(elem, info);
              }
              drops.push_back(
                  EmitDropImpl(*lowered, elem, ctx, allow_drop_glue, panic_out));
            }
          } else if (const auto* record_payload =
                         std::get_if<syntax::VariantPayloadRecord>(
                             &*variant.payload_opt)) {
            for (std::size_t i = record_payload->fields.size(); i > 0; --i) {
              const auto& field = record_payload->fields[i - 1];
              const auto lowered = LowerTypeForDrop(field.type, ctx);
              if (!lowered.has_value()) {
                continue;
              }
              IRValue field_val;
              field_val.kind = IRValue::Kind::Opaque;
              field_val.name = value.name + "_payload_" + field.name;
              ctx.RegisterValueType(field_val, *lowered);
              {
                DerivedValueInfo info;
                info.kind = DerivedValueInfo::Kind::EnumPayloadField;
                info.base = value;
                info.variant = variant.name;
                info.field = field.name;
                ctx.RegisterDerivedValue(field_val, info);
              }
              drops.push_back(
                  EmitDropImpl(*lowered,
                               field_val,
                               ctx,
                               allow_drop_glue,
                               panic_out));
            }
          }
          body = SeqWithPanicStop(std::move(drops), ctx);
        }

        IRValue unit;
        unit.kind = IRValue::Kind::Opaque;
        unit.name = "()";

        IRMatchArm arm;
        arm.pattern = std::move(pattern);
        arm.body = body;
        arm.value = unit;
        arms.push_back(std::move(arm));
      }

      IRMatch match;
      match.scrutinee = value;
      match.arms = std::move(arms);
      IRPtr payload_match = MakeIR(std::move(match));
      IRPtr drop_method = EmitDropMethodCall(type, value, panic_out, ctx);
      return SeqWithPanicStop({drop_method, payload_match}, ctx);
    }

    if (const auto* modal_decl = std::get_if<syntax::ModalDecl>(&it->second)) {
      std::vector<IRMatchArm> arms;
      arms.reserve(modal_decl->states.size());
      for (const auto& state : modal_decl->states) {
        syntax::ModalPattern modal_pat;
        modal_pat.state = state.name;
        modal_pat.fields_opt = std::nullopt;

        auto pattern = std::make_shared<syntax::Pattern>();
        pattern->node = std::move(modal_pat);
        pattern->span = core::Span{};

        IRPtr body = EmptyIR();
        const auto fields = CollectModalFields(state, ctx);
        if (fields.has_value()) {
          std::vector<IRPtr> drops;
          drops.reserve(fields->size());
          for (auto rit = fields->rbegin(); rit != fields->rend(); ++rit) {
            IRValue field_val;
            field_val.kind = IRValue::Kind::Opaque;
            field_val.name = value.name + "_" + rit->first;
            ctx.RegisterValueType(field_val, rit->second);
            {
              DerivedValueInfo info;
              info.kind = DerivedValueInfo::Kind::ModalField;
              info.base = value;
              info.modal_state = state.name;
              info.field = rit->first;
              ctx.RegisterDerivedValue(field_val, info);
            }
            drops.push_back(
                EmitDropImpl(rit->second,
                             field_val,
                             ctx,
                             allow_drop_glue,
                             panic_out));
          }
          body = SeqWithPanicStop(std::move(drops), ctx);
        }

        IRValue unit;
        unit.kind = IRValue::Kind::Opaque;
        unit.name = "()";

        IRMatchArm arm;
        arm.pattern = std::move(pattern);
        arm.body = body;
        arm.value = unit;
        arms.push_back(std::move(arm));
      }

      IRMatch match;
      match.scrutinee = value;
      match.arms = std::move(arms);
      IRPtr state_match = MakeIR(std::move(match));
      IRPtr drop_method = EmitDropMethodCall(type, value, panic_out, ctx);
      return SeqWithPanicStop({drop_method, state_match}, ctx);
    }

    return EmptyIR();
  }

  return EmptyIR();
}

IRPtr EmitDrop(const sema::TypeRef& type, const IRValue& value, LowerCtx& ctx) {
  SPEC_RULE("EmitDrop");
  IRValue panic_out;
  panic_out.kind = IRValue::Kind::Local;
  panic_out.name = std::string(kPanicOutName);
  return EmitDropImpl(type, value, ctx, true, panic_out);
}

// ============================================================================
// §6.8 EmitDropFields - Emit IR to drop specific fields of a record
// ============================================================================

IRPtr EmitDropFields(const sema::TypeRef& type,
                     const IRValue& value,
                     const std::vector<std::string>& skip_fields,
                     LowerCtx& ctx) {
  SPEC_RULE("FieldDropSeq");

  if (!type) {
    return EmptyIR();
  }

  // Only applicable to TypePathType (records)
  if (!HoldsType<sema::TypePathType>(type)) {
    return EmptyIR();
  }

  // Get type path
  const auto& type_path = GetType<sema::TypePathType>(type);

  // Look up RecordDecl from sigma
  if (!ctx.sigma) {
    return EmptyIR();
  }

  // Convert type path to syntax path key
  syntax::Path syntax_path;
  syntax_path.reserve(type_path.path.size());
  for (const auto& seg : type_path.path) {
    syntax_path.push_back(seg);
  }

  const auto path_key = sema::PathKeyOf(syntax_path);
  const auto it = ctx.sigma->types.find(path_key);
  if (it == ctx.sigma->types.end()) {
    return EmptyIR();
  }

  // Check if it's a RecordDecl
  const auto* record = std::get_if<syntax::RecordDecl>(&it->second);
  if (!record) {
    return EmptyIR();
  }

  // Build set of fields to skip for O(1) lookup
  std::unordered_set<std::string> skip_set(skip_fields.begin(), skip_fields.end());

  // Get Fields(R) - all FieldDecl members from record
  // Per spec: Fields(R) = [ f | f ∈ R.members ∧ f = FieldDecl(...) ]
  std::vector<const syntax::FieldDecl*> fields;
  for (const auto& member : record->members) {
    if (const auto* field = std::get_if<syntax::FieldDecl>(&member)) {
      fields.push_back(field);
    }
  }

  // FieldsRev(R) = rev(Fields(R)) - process in reverse order for LIFO drop
  std::vector<IRPtr> drops;
  drops.reserve(fields.size());

  for (auto rit = fields.rbegin(); rit != fields.rend(); ++rit) {
    const syntax::FieldDecl* field = *rit;

    // Skip fields in skip_fields set
    if (skip_set.count(field->name) > 0) {
      continue;
    }

    // FieldDropIR(slot, p, f, T) = EmitDrop(T, Load(FieldAddr(TypePath(p), slot, f), T))
    //
    // Create field value representation using opaque IR value pattern
    // (consistent with EmitDrop for tuples and arrays)
    IRValue field_val;
    field_val.kind = IRValue::Kind::Opaque;
    field_val.name = value.name + "." + field->name;

    // Lower field type from syntax::Type to sema::TypeRef
    sema::TypeRef field_type;
    if (field->type) {
      auto lowered = LowerTypeForDrop(field->type, ctx);
      if (lowered.has_value()) {
        field_type = *lowered;
      }
    }
    if (field_type) {
      ctx.RegisterValueType(field_val, field_type);
    }
    {
      DerivedValueInfo info;
      info.kind = DerivedValueInfo::Kind::Field;
      info.base = value;
      info.field = field->name;
      ctx.RegisterDerivedValue(field_val, info);
    }

    // Emit drop for this field
    drops.push_back(EmitDrop(field_type, field_val, ctx));
  }

  return SeqWithPanicStop(std::move(drops), ctx);
}

// ============================================================================
// §6.12.13 Drop Glue
// ============================================================================

std::string DropGlueSym(const sema::TypeRef& type, LowerCtx& ctx) {
  SPEC_RULE("DropGlueSym");

  std::vector<std::string> path = {"cursive", "runtime", "drop"};

  sema::TypeRef drop_type = type;
  if (HoldsType<sema::TypePerm>(drop_type)) {
    drop_type = GetType<sema::TypePerm>(drop_type).base;
  }

  const auto type_path = PathOfType(drop_type);
  if (type_path.empty()) {
    path.push_back("unknown");
  } else {
    path.insert(path.end(), type_path.begin(), type_path.end());
  }

  const std::string sym = core::Mangle(core::StringOfPath(path));
  ctx.RegisterDropGlueType(sym, drop_type);
  return sym;
}

IRPtr DropGlueIR(const sema::TypeRef& type, LowerCtx& ctx) {
  SPEC_RULE("DropGlueIR");

  // The drop glue reads the value from the data pointer and drops it
  IRValue data_ptr;
  data_ptr.kind = IRValue::Kind::Local;
  data_ptr.name = "data";

  // Read the value from the pointer
  IRReadPtr read;
  read.ptr = data_ptr;

  IRValue loaded_value;
  loaded_value = ctx.FreshTempValue("loaded");
  read.result = loaded_value;
  ctx.RegisterValueType(loaded_value, type);

  // Emit drop for the loaded value
  IRValue panic_out;
  panic_out.kind = IRValue::Kind::Local;
  panic_out.name = std::string(kPanicOutName);
  IRPtr drop_ir = EmitDropImpl(type, loaded_value, ctx, false, panic_out);

  return SeqIR({MakeIR(std::move(read)), drop_ir});
}

IRPtr EmitDropGlue(const sema::TypeRef& type, LowerCtx& ctx) {
  SPEC_RULE("EmitDropGlue-Decl");

  // Return the drop glue body IR
  // The procedure wrapper (ProcIR) would be created at the emit phase
  return DropGlueIR(type, ctx);
}

// ============================================================================
// §6.8 DropOnAssign - Drop value before assignment
// ============================================================================

IRPtr DropOnAssign(const std::string& name,
                   const IRPlace& /*slot*/,
                   LowerCtx& ctx) {
  SPEC_RULE("DropOnAssign");
  SPEC_RULE("DropOnAssign-NotApplicable");

  if (!DropOnAssignApplicable(name, ctx)) {
    SPEC_RULE("DropOnAssign-NotApplicable");
    return EmptyIR();
  }

  const BindingState* state = ctx.GetBindingState(name);
  if (!state || !state->type) {
    SPEC_RULE("DropOnAssign-Err");
    ctx.ReportCodegenFailure();
    return EmptyIR();
  }

  BindValidity validity = GetBindValidity(name, ctx);

  if (validity == BindValidity::Moved) {
    if (HoldsType<sema::TypePathType>(state->type)) {
      SPEC_RULE("DropOnAssign-Record-Moved");
    } else if (HoldsType<sema::TypeArray>(state->type) ||
               HoldsType<sema::TypeTuple>(state->type) ||
               HoldsType<sema::TypeUnion>(state->type) ||
               HoldsType<sema::TypeModalState>(state->type)) {
      SPEC_RULE("DropOnAssign-Aggregate-Moved");
    }
    return EmptyIR();
  }

  IRValue current_value;
  current_value.kind = IRValue::Kind::Local;
  current_value.name = name;

  if (HoldsType<sema::TypePathType>(state->type)) {
    if (validity == BindValidity::PartiallyMoved) {
      SPEC_RULE("DropOnAssign-Record-Partial");
      std::vector<std::string> moved = GetMovedFields(name, ctx);
      return EmitDropFields(state->type, current_value, moved, ctx);
    }
    SPEC_RULE("DropOnAssign-Record-Valid");
    return EmitDrop(state->type, current_value, ctx);
  }

  if (HoldsType<sema::TypeArray>(state->type) ||
      HoldsType<sema::TypeTuple>(state->type) ||
      HoldsType<sema::TypeUnion>(state->type) ||
      HoldsType<sema::TypeModalState>(state->type)) {
    SPEC_RULE("DropOnAssign-Aggregate-Ok");
    return EmitDrop(state->type, current_value, ctx);
  }

  return EmitDrop(state->type, current_value, ctx);
}

bool DropOnAssignApplicable(const std::string& name, LowerCtx& ctx) {
  SPEC_RULE("DropOnAssignApplicable");

  const BindingState* state = ctx.GetBindingState(name);
  if (!state) {
    return false;
  }
  return state->has_responsibility && state->is_immovable;
}

bool DropOnAssignRoot(const syntax::Expr& place, LowerCtx& ctx) {
  SPEC_RULE("DropOnAssignRoot");

  std::function<std::optional<std::string>(const syntax::Expr&)> root_name =
      [&](const syntax::Expr& expr) -> std::optional<std::string> {
        return std::visit(
            [&](const auto& node) -> std::optional<std::string> {
              using T = std::decay_t<decltype(node)>;
              if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
                return node.name;
              } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
                return root_name(*node.base);
              } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
                return root_name(*node.base);
              } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
                return root_name(*node.base);
              } else if constexpr (std::is_same_v<T, syntax::DerefExpr>) {
                return root_name(*node.value);
              } else {
                return std::nullopt;
              }
            },
            expr.node);
      };

  auto root = root_name(place);
  if (!root.has_value()) {
    return false;
  }

  if (ctx.GetBindingState(*root)) {
    return DropOnAssignApplicable(*root, ctx);
  }

  if (ctx.resolve_name) {
    auto resolved = ctx.resolve_name(*root);
    if (resolved.has_value() && !resolved->empty()) {
      std::vector<std::string> full = *resolved;
      const std::string resolved_name = full.back();
      full.pop_back();
      if (auto flags = StaticBindFlagsFor(full, resolved_name, ctx)) {
        return flags->has_responsibility && flags->immovable;
      }
      return false;
    }
  }

  return DropOnAssignApplicable(*root, ctx);
}

// ============================================================================
// §6.8 Binding Validity State
// ============================================================================

BindValidity GetBindValidity(const std::string& name, LowerCtx& ctx) {
  SPEC_RULE("BindValid");
  SPEC_RULE("BindValid-Sigma");

  // Look up binding state from context
  const BindingState* state = ctx.GetBindingState(name);
  if (!state) {
    if (!ctx.binding_states.empty()) {
      SPEC_RULE("BindValid-Err");
      ctx.ReportCodegenFailure();
    }
    return BindValidity::Valid;
  }
  
  if (state->is_moved) {
    return BindValidity::Moved;
  }
  
  if (!state->moved_fields.empty()) {
    return BindValidity::PartiallyMoved;
  }
  
  return BindValidity::Valid;
}

std::vector<std::string> GetMovedFields(const std::string& name,
                                        LowerCtx& ctx) {
  SPEC_RULE("PartiallyMoved");

  // Look up binding state from context
  const BindingState* state = ctx.GetBindingState(name);
  if (!state) {
    return {};
  }
  
  return state->moved_fields;
}

// ============================================================================
// Anchor function for SPEC_RULE markers
// ============================================================================

void AnchorCleanupRules() {
  SPEC_RULE("CleanupPlan");
  SPEC_RULE("EmitCleanup");
  SPEC_RULE("EmitDrop");
  SPEC_RULE("FieldDropSeq");
  SPEC_RULE("DropGlueSym");
  SPEC_RULE("DropGlueIR");
  SPEC_RULE("EmitDropGlue-Decl");
  SPEC_RULE("DropOnAssign");
  SPEC_RULE("DropOnAssign-NotApplicable");
  SPEC_RULE("DropOnAssign-Record-Valid");
  SPEC_RULE("DropOnAssign-Record-Partial");
  SPEC_RULE("DropOnAssign-Record-Moved");
  SPEC_RULE("DropOnAssign-Aggregate-Ok");
  SPEC_RULE("DropOnAssign-Aggregate-Moved");
  SPEC_RULE("DropOnAssignApplicable");
  SPEC_RULE("DropOnAssignRoot");
  SPEC_RULE("BindValid");
  SPEC_RULE("PartiallyMoved");
}

}  // namespace cursive0::codegen
