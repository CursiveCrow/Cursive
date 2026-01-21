#include "cursive0/eval/exec.h"
#include <memory>
#include <utility>

#include "cursive0/core/int128.h"
#include "cursive0/analysis/resolve/collect_toplevel.h"
#include "cursive0/analysis/resolve/resolver.h"
#include "cursive0/analysis/types/type_expr.h"
#include "cursive0/eval/cleanup.h"
#include "cursive0/eval/eval.h"

namespace cursive0::eval {

namespace {

Outcome PanicOutcome() {
  Control ctrl;
  ctrl.kind = ControlKind::Panic;
  return MakeCtrl(ctrl);
}

StmtOut StmtOutOf(const Outcome& out) {
  if (IsCtrl(out)) {
    return MakeCtrlOut(std::get<Control>(out.node));
  }
  return MakeOk();
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

SemanticsContext WithoutTemps(const SemanticsContext& ctx) {
  SemanticsContext inner = ctx;
  inner.temp_ctx = nullptr;
  return inner;
}

analysis::TypeRef BlockResultType(const SemanticsContext& ctx,
                              const syntax::Block& block) {
  if (block.tail_opt) {
    if (ctx.sema && ctx.sema->expr_types) {
      const auto it = ctx.sema->expr_types->find(block.tail_opt.get());
      if (it != ctx.sema->expr_types->end()) {
        return it->second;
      }
    }
    return nullptr;
  }
  return analysis::MakeTypePrim("()");
}

CleanupStatus CleanupTemps(const SemanticsContext& ctx,
                           const std::vector<TempValue>& temps,
                           Sigma& sigma) {
  CleanupStatus status = CleanupStatus::Ok;
  for (auto it = temps.rbegin(); it != temps.rend(); ++it) {
    const auto drop_status =
        DropValueOut(ctx, it->type, it->value, {}, sigma);
    const bool item_panic = drop_status == DropStatus::Panic;
    if (item_panic) {
      if (status == CleanupStatus::Panic) {
        return CleanupStatus::Abort;
      }
      status = CleanupStatus::Panic;
    }
  }
  return status;
}

StmtOut ApplyTempCleanup(const SemanticsContext& ctx,
                         const StmtOut& out,
                         CleanupStatus status) {
  if (status == CleanupStatus::Ok) {
    return out;
  }
  Outcome base;
  if (std::holds_alternative<Control>(out.node)) {
    base = MakeCtrl(std::get<Control>(out.node));
  } else {
    base = MakeVal(Value{UnitVal{}});
  }
  Outcome combined = ExitOutcome(base, status);
  if (IsCtrl(combined)) {
    return MakeCtrlOut(std::get<Control>(combined.node));
  }
  return MakeOk();
}

bool IsMoveExpr(const syntax::Expr& expr) {
  return std::holds_alternative<syntax::MoveExpr>(expr.node);
}

bool IsPlaceExpr(const syntax::Expr& expr) {
  return std::visit(
      [&](const auto& node) -> bool {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
          return true;
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
          return node.base ? IsPlaceExpr(*node.base) : false;
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
          return node.base ? IsPlaceExpr(*node.base) : false;
        } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
          return node.base ? IsPlaceExpr(*node.base) : false;
        } else if constexpr (std::is_same_v<T, syntax::DerefExpr>) {
          return node.value ? IsPlaceExpr(*node.value) : false;
        }
        return false;
      },
      expr.node);
}

Responsibility RespOfInit(const syntax::Expr& init) {
  if (!IsPlaceExpr(init)) {
    return Responsibility::Resp;
  }
  if (IsMoveExpr(init)) {
    return Responsibility::Resp;
  }
  return Responsibility::Alias;
}

Movability MovOf(const syntax::Token& op) {
  if (op.lexeme == ":=") {
    return Movability::Immov;
  }
  return Movability::Mov;
}

BindInfo BindInfoForBinding(const syntax::Binding& binding) {
  BindInfo info;
  info.mov = MovOf(binding.op);
  if (binding.init) {
    info.resp = RespOfInit(*binding.init);
  }
  return info;
}

BindInfo BindInfoForShadow(const syntax::Expr& init) {
  BindInfo info;
  info.mov = Movability::Mov;
  info.resp = RespOfInit(init);
  return info;
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

bool TargetExists(const Sigma& sigma, RegionTarget target) {
  for (const auto& entry : sigma.region_stack) {
    if (entry.target == target) {
      return true;
    }
  }
  return false;
}

bool TagExists(const Sigma& sigma, RegionTag tag) {
  for (const auto& entry : sigma.region_stack) {
    if (entry.tag == tag) {
      return true;
    }
  }
  return false;
}

RegionTarget FreshArena(Sigma& sigma) {
  RegionTarget candidate = sigma.next_region_target;
  while (TargetExists(sigma, candidate)) {
    ++candidate;
  }
  sigma.next_region_target = candidate + 1;
  return candidate;
}

RegionTag FreshTag(Sigma& sigma) {
  RegionTag candidate = sigma.next_region_tag;
  while (TagExists(sigma, candidate)) {
    ++candidate;
  }
  sigma.next_region_tag = candidate + 1;
  return candidate;
}

bool RegionNew(Sigma& sigma,
               const Value& /*opts*/,
               RegionTarget* out_target,
               ScopeId* out_scope) {
  ScopeEntry scope;
  if (!PushScope(sigma, &scope)) {
    return false;
  }
  const RegionTarget target = FreshArena(sigma);
  RegionEntry entry;
  entry.tag = target;
  entry.target = target;
  entry.scope = scope.id;
  entry.mark = std::nullopt;
  sigma.region_stack.push_back(entry);
  if (out_target) {
    *out_target = target;
  }
  if (out_scope) {
    *out_scope = scope.id;
  }
  return true;
}

bool FrameEnter(Sigma& sigma,
                RegionTarget target,
                RegionTag* out_tag,
                ScopeId* out_scope,
                std::size_t* out_mark) {
  ScopeEntry scope;
  if (!PushScope(sigma, &scope)) {
    return false;
  }
  const RegionTag tag = FreshTag(sigma);
  const std::size_t mark = sigma.region_stack.size();
  RegionEntry entry;
  entry.tag = tag;
  entry.target = target;
  entry.scope = scope.id;
  entry.mark = mark;
  sigma.region_stack.push_back(entry);
  if (out_tag) {
    *out_tag = tag;
  }
  if (out_scope) {
    *out_scope = scope.id;
  }
  if (out_mark) {
    *out_mark = mark;
  }
  return true;
}

bool PopRegion(Sigma& sigma, RegionTarget target) {
  for (auto it = sigma.region_stack.rbegin();
       it != sigma.region_stack.rend();
       ++it) {
    if (it->target == target) {
      sigma.region_stack.erase(std::next(it).base());
      return true;
    }
  }
  return false;
}

bool ReleaseArena(Sigma& sigma, RegionTarget target) {
  return PopRegion(sigma, target);
}

bool ResetArena(Sigma& sigma, RegionTarget target, std::size_t /*mark*/) {
  return PopRegion(sigma, target);
}

Outcome RegionRelease(const SemanticsContext& ctx,
                      RegionTarget target,
                      ScopeId scope,
                      const Outcome& out,
                      Sigma& sigma) {
  auto scope_ptr = MutableScopeById(sigma, scope);
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
  if (!ReleaseArena(sigma, target)) {
    return PanicOutcome();
  }
  ScopeEntry popped;
  if (!PopScope(sigma, &popped)) {
    return PanicOutcome();
  }
  return out2;
}

Outcome FrameReset(const SemanticsContext& ctx,
                   RegionTarget target,
                   ScopeId scope,
                   std::size_t mark,
                   const Outcome& out,
                   Sigma& sigma) {
  auto scope_ptr = MutableScopeById(sigma, scope);
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
  if (!ResetArena(sigma, target, mark)) {
    return PanicOutcome();
  }
  ScopeEntry popped;
  if (!PopScope(sigma, &popped)) {
    return PanicOutcome();
  }
  return out2;
}

IntVal USizeVal(std::uint64_t value) {
  IntVal out;
  out.type = "usize";
  out.negative = false;
  out.magnitude = core::UInt128FromU64(value);
  return out;
}

Value MakeRegionValue(RegionTarget target) {
  RecordVal record;
  record.record_type = analysis::MakeTypeModalState({"Region"}, "Active");
  record.fields = {{"handle", Value{USizeVal(static_cast<std::uint64_t>(target))}}};
  return Value{record};
}

}  // namespace

std::optional<BindEnv> BindPatternVal(const SemanticsContext& ctx,
                                      const syntax::Pattern& pattern,
                                      const Value& value) {
  return MatchPattern(ctx, pattern, value);
}

BindListInput BindOrder(const syntax::Pattern& pattern,
                        const BindEnv& env) {
  BindListInput out;
  const auto names = analysis::PatNames(pattern);
  out.reserve(names.size());
  for (const auto& name : names) {
    const auto it = env.find(name);
    if (it == env.end()) {
      continue;
    }
    out.emplace_back(name, it->second);
  }
  return out;
}

BindResult BindList(Sigma& sigma,
                    const BindListInput& binds,
                    BindInfo info) {
  BindResult result;
  result.ok = true;
  if (binds.empty()) {
    SPEC_RULE("BindList-Empty");
    return result;
  }
  for (const auto& [name, value] : binds) {
    Binding binding;
    if (!BindVal(sigma, name, value, info, &binding)) {
      result.ok = false;
      result.bindings.clear();
      return result;
    }
    result.bindings.push_back(binding);
    SPEC_RULE("BindList-Cons");
  }
  return result;
}

BindResult BindPattern(Sigma& sigma,
                       const SemanticsContext& ctx,
                       const syntax::Pattern& pattern,
                       const Value& value,
                       BindInfo info) {
  const auto env = BindPatternVal(ctx, pattern, value);
  if (!env.has_value()) {
    return {};
  }
  const auto binds = BindOrder(pattern, *env);
  return BindList(sigma, binds, info);
}

bool BlockEnter(Sigma& sigma,
                const BindListInput& binds,
                BindInfo info,
                ScopeId* out_scope) {
  ScopeEntry scope;
  if (!PushScope(sigma, &scope)) {
    return false;
  }
  if (out_scope) {
    *out_scope = scope.id;
  }
  const auto bound = BindList(sigma, binds, info);
  return bound.ok;
}

Outcome EvalBlockBodySigma(const SemanticsContext& ctx,
                           const syntax::Block& block,
                           Sigma& sigma) {
  const auto seq_out = ExecSeqSigma(ctx, block.stmts, sigma);
  if (std::holds_alternative<Control>(seq_out.node)) {
    const auto& ctrl = std::get<Control>(seq_out.node);
    if (ctrl.kind == ControlKind::Result) {
      return MakeVal(ctrl.value ? *ctrl.value : Value{UnitVal{}});
    }
    return MakeCtrl(ctrl);
  }
  if (block.tail_opt) {
    return EvalSigma(ctx, *block.tail_opt, sigma);
  }
  return MakeVal(Value{UnitVal{}});
}

Outcome EvalBlockSigma(const SemanticsContext& ctx,
                       const syntax::Block& block,
                       Sigma& sigma) {
  ScopeId scope = 0;
  if (!BlockEnter(sigma, {}, BindInfo{}, &scope)) {
    return PanicOutcome();
  }
  Outcome out = EvalBlockBodySigma(ctx, block, sigma);
  return BlockExit(ctx, scope, out, sigma);
}

Outcome EvalBlockBindSigma(const SemanticsContext& ctx,
                           const syntax::Pattern& pattern,
                           const Value& value,
                           const syntax::Block& block,
                           BindInfo info,
                           Sigma& sigma) {
  const auto env = BindPatternVal(ctx, pattern, value);
  if (!env.has_value()) {
    return PanicOutcome();
  }
  const auto binds = BindOrder(pattern, *env);
  ScopeId scope = 0;
  if (!BlockEnter(sigma, binds, info, &scope)) {
    return PanicOutcome();
  }
  Outcome out = EvalBlockBodySigma(ctx, block, sigma);
  return BlockExit(ctx, scope, out, sigma);
}

Outcome EvalInScopeSigma(const SemanticsContext& ctx,
                         const syntax::Block& block,
                         ScopeId scope,
                         Sigma& sigma) {
  if (CurrentScopeId(sigma) != scope) {
    return PanicOutcome();
  }
  return EvalBlockBodySigma(ctx, block, sigma);
}

StmtOut ExecSeqSigma(const SemanticsContext& ctx,
                     const std::vector<syntax::Stmt>& stmts,
                     Sigma& sigma) {
  if (stmts.empty()) {
    SPEC_RULE("ExecSeq-Empty");
    SPEC_RULE("Step-ExecSeq-Ok");
    return MakeOk();
  }
  for (const auto& stmt : stmts) {
    const auto out = ExecSigma(ctx, stmt, sigma);
    if (std::holds_alternative<Control>(out.node)) {
      SPEC_RULE("ExecSeq-Cons-Ctrl");
      SPEC_RULE("Step-ExecSeq-Ctrl");
      return out;
    }
    SPEC_RULE("ExecSeq-Cons-Ok");
  }
  SPEC_RULE("Step-ExecSeq-Ok");
  return MakeOk();
}

StmtOut ExecSigma(const SemanticsContext& ctx,
                  const syntax::Stmt& stmt,
                  Sigma& sigma) {
  TempContext temp_state;
  std::vector<TempValue> temps;
  temp_state.sink = &temps;
  temp_state.depth = 0;
  temp_state.suppress_depth = std::nullopt;

  SemanticsContext stmt_ctx = ctx;
  stmt_ctx.temp_ctx = &temp_state;

  StmtOut out = std::visit(
      [&](const auto& node) -> StmtOut {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::LetStmt>) {
          auto prev_suppress = temp_state.suppress_depth;
          temp_state.suppress_depth = temp_state.depth + 1;
          Outcome init_out = EvalSigma(stmt_ctx, *node.binding.init, sigma);
          temp_state.suppress_depth = prev_suppress;
          if (IsCtrl(init_out)) {
            SPEC_RULE("ExecSigma-Let-Ctrl");
            SPEC_RULE("Step-Exec-Other-Ctrl");
            return MakeCtrlOut(std::get<Control>(init_out.node));
          }
          const BindInfo info = BindInfoForBinding(node.binding);
          const auto bound =
              BindPattern(sigma, stmt_ctx, *node.binding.pat,
                          std::get<Value>(init_out.node), info);
          if (!bound.ok) {
            return MakeCtrlOut(Control{ControlKind::Panic, std::nullopt});
          }
          SPEC_RULE("ExecSigma-Let");
          SPEC_RULE("Step-Exec-Other-Ok");
          return MakeOk();
        } else if constexpr (std::is_same_v<T, syntax::VarStmt>) {
          auto prev_suppress = temp_state.suppress_depth;
          temp_state.suppress_depth = temp_state.depth + 1;
          Outcome init_out = EvalSigma(stmt_ctx, *node.binding.init, sigma);
          temp_state.suppress_depth = prev_suppress;
          if (IsCtrl(init_out)) {
            SPEC_RULE("ExecSigma-Var-Ctrl");
            return MakeCtrlOut(std::get<Control>(init_out.node));
          }
          const BindInfo info = BindInfoForBinding(node.binding);
          const auto bound =
              BindPattern(sigma, stmt_ctx, *node.binding.pat,
                          std::get<Value>(init_out.node), info);
          if (!bound.ok) {
            return MakeCtrlOut(Control{ControlKind::Panic, std::nullopt});
          }
          SPEC_RULE("ExecSigma-Var");
          return MakeOk();
        } else if constexpr (std::is_same_v<T, syntax::ShadowLetStmt>) {
          auto prev_suppress = temp_state.suppress_depth;
          temp_state.suppress_depth = temp_state.depth + 1;
          Outcome init_out = EvalSigma(stmt_ctx, *node.init, sigma);
          temp_state.suppress_depth = prev_suppress;
          if (IsCtrl(init_out)) {
            SPEC_RULE("ExecSigma-ShadowLet-Ctrl");
            return MakeCtrlOut(std::get<Control>(init_out.node));
          }
          const BindInfo info = BindInfoForShadow(*node.init);
          Binding binding;
          if (!BindVal(sigma, node.name,
                       std::get<Value>(init_out.node),
                       info, &binding)) {
            return MakeCtrlOut(Control{ControlKind::Panic, std::nullopt});
          }
          SPEC_RULE("ExecSigma-ShadowLet");
          return MakeOk();
        } else if constexpr (std::is_same_v<T, syntax::ShadowVarStmt>) {
          auto prev_suppress = temp_state.suppress_depth;
          temp_state.suppress_depth = temp_state.depth + 1;
          Outcome init_out = EvalSigma(stmt_ctx, *node.init, sigma);
          temp_state.suppress_depth = prev_suppress;
          if (IsCtrl(init_out)) {
            SPEC_RULE("ExecSigma-ShadowVar-Ctrl");
            return MakeCtrlOut(std::get<Control>(init_out.node));
          }
          const BindInfo info = BindInfoForShadow(*node.init);
          Binding binding;
          if (!BindVal(sigma, node.name,
                       std::get<Value>(init_out.node),
                       info, &binding)) {
            return MakeCtrlOut(Control{ControlKind::Panic, std::nullopt});
          }
          SPEC_RULE("ExecSigma-ShadowVar");
          return MakeOk();
        } else if constexpr (std::is_same_v<T, syntax::AssignStmt>) {
          Outcome val_out = EvalSigma(stmt_ctx, *node.value, sigma);
          if (IsCtrl(val_out)) {
            SPEC_RULE("ExecSigma-Assign-Ctrl");
            return MakeCtrlOut(std::get<Control>(val_out.node));
          }
          const auto sout =
              WritePlaceSigma(stmt_ctx, *node.place,
                              std::get<Value>(val_out.node), sigma);
          SPEC_RULE("ExecSigma-Assign");
          return sout;
        } else if constexpr (std::is_same_v<T, syntax::CompoundAssignStmt>) {
          Outcome base_out = ReadPlaceSigma(stmt_ctx, *node.place, sigma);
          if (IsCtrl(base_out)) {
            SPEC_RULE("ExecSigma-CompoundAssign-Left-Ctrl");
            return MakeCtrlOut(std::get<Control>(base_out.node));
          }
          Outcome rhs_out = EvalSigma(stmt_ctx, *node.value, sigma);
          if (IsCtrl(rhs_out)) {
            SPEC_RULE("ExecSigma-CompoundAssign-Right-Ctrl");
            return MakeCtrlOut(std::get<Control>(rhs_out.node));
          }
          std::string_view op = node.op;
          if (!op.empty() && op.back() == '=') {
            op = op.substr(0, op.size() - 1);
          }
          const auto value = EvalBinOp(op,
                                       std::get<Value>(base_out.node),
                                       std::get<Value>(rhs_out.node));
          if (!value.has_value()) {
            return MakeCtrlOut(Control{ControlKind::Panic, std::nullopt});
          }
          const auto sout =
              WritePlaceSigma(stmt_ctx, *node.place, *value, sigma);
          SPEC_RULE("ExecSigma-CompoundAssign");
          return sout;
        } else if constexpr (std::is_same_v<T, syntax::ExprStmt>) {
          Outcome out = EvalSigma(stmt_ctx, *node.value, sigma);
          SPEC_RULE("ExecSigma-ExprStmt");
          return StmtOutOf(out);
        } else if constexpr (std::is_same_v<T, syntax::UnsafeBlockStmt>) {
          SemanticsContext inner_ctx = WithoutTemps(stmt_ctx);
          Outcome out = EvalBlockSigma(inner_ctx, *node.body, sigma);
          if (temp_state.sink && IsVal(out)) {
            analysis::TypeRef block_type = BlockResultType(stmt_ctx, *node.body);
            temp_state.sink->push_back(
                TempValue{block_type, std::get<Value>(out.node)});
          }
          SPEC_RULE("ExecSigma-UnsafeStmt");
          return StmtOutOf(out);
        } else if constexpr (std::is_same_v<T, syntax::DeferStmt>) {
          if (!AppendCleanup(sigma, DeferBlock{node.body})) {
            return MakeCtrlOut(Control{ControlKind::Panic, std::nullopt});
          }
          SPEC_RULE("ExecSigma-Defer");
          SPEC_RULE("Step-Exec-Defer");
          return MakeOk();
        } else if constexpr (std::is_same_v<T, syntax::RegionStmt>) {
          std::optional<Value> opts_value;
          if (node.opts_opt) {
            Outcome opts_out = EvalSigma(stmt_ctx, *node.opts_opt, sigma);
            if (IsCtrl(opts_out)) {
              SPEC_RULE("ExecSigma-Region-Ctrl");
              SPEC_RULE("Step-Exec-Region-Enter-Ctrl");
              return MakeCtrlOut(std::get<Control>(opts_out.node));
            }
            opts_value = std::get<Value>(opts_out.node);
          } else {
            auto ident = std::make_shared<syntax::Expr>();
            ident->node = syntax::IdentifierExpr{"RegionOptions"};
            syntax::CallExpr call;
            call.callee = ident;
            call.args = {};
            syntax::Expr opts_expr;
            opts_expr.node = std::move(call);
            Outcome opts_out = EvalSigma(stmt_ctx, opts_expr, sigma);
            if (IsCtrl(opts_out)) {
              SPEC_RULE("ExecSigma-Region-Ctrl");
              SPEC_RULE("Step-Exec-Region-Enter-Ctrl");
              return MakeCtrlOut(std::get<Control>(opts_out.node));
            }
            opts_value = std::get<Value>(opts_out.node);
          }
          RegionTarget target = 0;
          ScopeId scope = 0;
          if (!RegionNew(sigma, *opts_value, &target, &scope)) {
            return MakeCtrlOut(Control{ControlKind::Panic, std::nullopt});
          }
          if (node.alias_opt.has_value()) {
            Binding binding;
            BindInfo info;
            info.mov = Movability::Mov;
            info.resp = Responsibility::Resp;
            if (!BindVal(sigma, *node.alias_opt,
                         MakeRegionValue(target),
                         info, &binding)) {
              return MakeCtrlOut(Control{ControlKind::Panic, std::nullopt});
            }
          }
          SPEC_RULE("Step-Exec-Region-Enter");
          SemanticsContext inner_ctx = WithoutTemps(stmt_ctx);
          Outcome out = EvalInScopeSigma(inner_ctx, *node.body, scope, sigma);
          SPEC_RULE("Step-Exec-Region-Body");
          Outcome out2 = RegionRelease(inner_ctx, target, scope, out, sigma);
          if (temp_state.sink && IsVal(out2)) {
            analysis::TypeRef block_type = BlockResultType(stmt_ctx, *node.body);
            temp_state.sink->push_back(
                TempValue{block_type, std::get<Value>(out2.node)});
          }
          StmtOut sout = StmtOutOf(out2);
          if (std::holds_alternative<Control>(sout.node)) {
            SPEC_RULE("Step-Exec-Region-Exit-Ctrl");
          } else {
            SPEC_RULE("Step-Exec-Region-Exit-Ok");
          }
          SPEC_RULE("ExecSigma-Region");
          return sout;
        } else if constexpr (std::is_same_v<T, syntax::FrameStmt>) {
          RegionTarget target = 0;
          const bool explicit_target = node.target_opt.has_value();
          if (explicit_target) {
            const auto value =
                LookupVal(stmt_ctx, sigma, *node.target_opt);
            if (!value.has_value()) {
              return MakeCtrlOut(Control{ControlKind::Panic, std::nullopt});
            }
            const auto handle = RegionTargetOf(*value);
            if (!handle.has_value()) {
              return MakeCtrlOut(Control{ControlKind::Panic, std::nullopt});
            }
            const auto resolved = ResolveTarget(sigma, *handle);
            if (!resolved.has_value()) {
              return MakeCtrlOut(Control{ControlKind::Panic, std::nullopt});
            }
            target = *resolved;
            SPEC_RULE("ExecSigma-Frame-Explicit");
          } else {
            const auto active = ActiveTarget(sigma);
            if (!active.has_value()) {
              return MakeCtrlOut(Control{ControlKind::Panic, std::nullopt});
            }
            target = *active;
            SPEC_RULE("ExecSigma-Frame-Implicit");
          }
          RegionTag tag = 0;
          ScopeId scope = 0;
          std::size_t mark = 0;
          if (!FrameEnter(sigma, target, &tag, &scope, &mark)) {
            return MakeCtrlOut(Control{ControlKind::Panic, std::nullopt});
          }
          if (explicit_target) {
            SPEC_RULE("Step-Exec-Frame-Enter-Explicit");
          } else {
            SPEC_RULE("Step-Exec-Frame-Enter-Implicit");
          }
          SemanticsContext inner_ctx = WithoutTemps(stmt_ctx);
          Outcome out = EvalInScopeSigma(inner_ctx, *node.body, scope, sigma);
          SPEC_RULE("Step-Exec-Frame-Body");
          Outcome out2 = FrameReset(inner_ctx, target, scope, mark, out, sigma);
          if (temp_state.sink && IsVal(out2)) {
            analysis::TypeRef block_type = BlockResultType(stmt_ctx, *node.body);
            temp_state.sink->push_back(
                TempValue{block_type, std::get<Value>(out2.node)});
          }
          StmtOut sout = StmtOutOf(out2);
          if (std::holds_alternative<Control>(sout.node)) {
            SPEC_RULE("Step-Exec-Frame-Exit-Ctrl");
          } else {
            SPEC_RULE("Step-Exec-Frame-Exit-Ok");
          }
          return sout;
        } else if constexpr (std::is_same_v<T, syntax::ReturnStmt>) {
          if (!node.value_opt) {
            Control ctrl;
            ctrl.kind = ControlKind::Return;
            ctrl.value = Value{UnitVal{}};
            SPEC_RULE("ExecSigma-Return-Unit");
            return MakeCtrlOut(ctrl);
          }
          auto prev_suppress = temp_state.suppress_depth;
          temp_state.suppress_depth = temp_state.depth + 1;
          Outcome out = EvalSigma(stmt_ctx, *node.value_opt, sigma);
          temp_state.suppress_depth = prev_suppress;
          if (IsCtrl(out)) {
            SPEC_RULE("ExecSigma-Return-Ctrl");
            return MakeCtrlOut(std::get<Control>(out.node));
          }
          Control ctrl;
          ctrl.kind = ControlKind::Return;
          ctrl.value = std::get<Value>(out.node);
          SPEC_RULE("ExecSigma-Return");
          return MakeCtrlOut(ctrl);
        } else if constexpr (std::is_same_v<T, syntax::ResultStmt>) {
          auto prev_suppress = temp_state.suppress_depth;
          temp_state.suppress_depth = temp_state.depth + 1;
          Outcome out = EvalSigma(stmt_ctx, *node.value, sigma);
          temp_state.suppress_depth = prev_suppress;
          if (IsCtrl(out)) {
            SPEC_RULE("ExecSigma-Result-Ctrl");
            return MakeCtrlOut(std::get<Control>(out.node));
          }
          Control ctrl;
          ctrl.kind = ControlKind::Result;
          ctrl.value = std::get<Value>(out.node);
          SPEC_RULE("ExecSigma-Result");
          return MakeCtrlOut(ctrl);
        } else if constexpr (std::is_same_v<T, syntax::BreakStmt>) {
          if (!node.value_opt) {
            Control ctrl;
            ctrl.kind = ControlKind::Break;
            ctrl.value = std::nullopt;
            SPEC_RULE("ExecSigma-Break-Unit");
            return MakeCtrlOut(ctrl);
          }
          auto prev_suppress = temp_state.suppress_depth;
          temp_state.suppress_depth = temp_state.depth + 1;
          Outcome out = EvalSigma(stmt_ctx, *node.value_opt, sigma);
          temp_state.suppress_depth = prev_suppress;
          if (IsCtrl(out)) {
            SPEC_RULE("ExecSigma-Break-Ctrl");
            return MakeCtrlOut(std::get<Control>(out.node));
          }
          Control ctrl;
          ctrl.kind = ControlKind::Break;
          ctrl.value = std::get<Value>(out.node);
          SPEC_RULE("ExecSigma-Break");
          return MakeCtrlOut(ctrl);
        } else if constexpr (std::is_same_v<T, syntax::ContinueStmt>) {
          Control ctrl;
          ctrl.kind = ControlKind::Continue;
          SPEC_RULE("ExecSigma-Continue");
          return MakeCtrlOut(ctrl);
        } else if constexpr (std::is_same_v<T, syntax::ErrorStmt>) {
          Control ctrl;
          ctrl.kind = ControlKind::Panic;
          SetPanicReason(sigma, PanicReason::ErrorStmt);
          SPEC_RULE("ExecSigma-Error");
          return MakeCtrlOut(ctrl);
        }
        return MakeCtrlOut(Control{ControlKind::Panic, std::nullopt});
      },
      stmt);

  CleanupStatus temp_status = CleanupTemps(stmt_ctx, temps, sigma);
  return ApplyTempCleanup(stmt_ctx, out, temp_status);
}

}  // namespace cursive0::eval