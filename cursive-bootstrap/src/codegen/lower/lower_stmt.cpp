#include "cursive0/codegen/lower/lower_stmt.h"

#include "cursive0/codegen/checks.h"
#include "cursive0/codegen/cleanup.h"
#include "cursive0/codegen/layout/layout.h"
#include "cursive0/codegen/lower/lower_expr.h"
#include "cursive0/codegen/lower/lower_pat.h"
#include "cursive0/core/assert_spec.h"

#include <cassert>
#include <variant>

namespace cursive0::codegen {

namespace {

bool BlockEndsWithReturn(const syntax::Block& block) {
  if (block.stmts.empty()) {
    return false;
  }
  return std::holds_alternative<syntax::ReturnStmt>(block.stmts.back());
}

}  // namespace

namespace {

void CollectPatternNames(const syntax::Pattern& pattern,
                         std::vector<std::string>& out) {
  std::visit(
      [&out](const auto& pat) {
        using T = std::decay_t<decltype(pat)>;
        if constexpr (std::is_same_v<T, syntax::IdentifierPattern>) {
          out.push_back(pat.name);
        } else if constexpr (std::is_same_v<T, syntax::TypedPattern>) {
          out.push_back(pat.name);
        } else if constexpr (std::is_same_v<T, syntax::TuplePattern>) {
          for (const auto& elem : pat.elements) {
            if (elem) {
              CollectPatternNames(*elem, out);
            }
          }
        } else if constexpr (std::is_same_v<T, syntax::RecordPattern>) {
          for (const auto& field : pat.fields) {
            if (field.pattern_opt) {
              CollectPatternNames(*field.pattern_opt, out);
            } else {
              out.push_back(field.name);
            }
          }
        } else if constexpr (std::is_same_v<T, syntax::EnumPattern>) {
          if (!pat.payload_opt) {
            return;
          }
          std::visit(
              [&out](const auto& payload) {
                using P = std::decay_t<decltype(payload)>;
                if constexpr (std::is_same_v<P, syntax::TuplePayloadPattern>) {
                  for (const auto& elem : payload.elements) {
                    if (elem) {
                      CollectPatternNames(*elem, out);
                    }
                  }
                } else {
                  for (const auto& field : payload.fields) {
                    if (field.pattern_opt) {
                      CollectPatternNames(*field.pattern_opt, out);
                    } else {
                      out.push_back(field.name);
                    }
                  }
                }
              },
              *pat.payload_opt);
        } else if constexpr (std::is_same_v<T, syntax::ModalPattern>) {
          if (!pat.fields_opt) {
            return;
          }
          for (const auto& field : pat.fields_opt->fields) {
            if (field.pattern_opt) {
              CollectPatternNames(*field.pattern_opt, out);
            } else {
              out.push_back(field.name);
            }
          }
        } else {
          return;
        }
      },
      pattern.node);
}


static analysis::TypeRef LoopPatternType(const analysis::TypeRef& iter_type) {
  if (!iter_type) {
    return nullptr;
  }
  if (std::holds_alternative<analysis::TypeArray>(iter_type->node)) {
    return std::get<analysis::TypeArray>(iter_type->node).element;
  }
  if (std::holds_alternative<analysis::TypeSlice>(iter_type->node)) {
    return std::get<analysis::TypeSlice>(iter_type->node).element;
  }
  return nullptr;
}

analysis::TypeRef LowerBindingType(const std::shared_ptr<syntax::Type>& type_opt,
                               LowerCtx& ctx) {
  if (!type_opt || !ctx.sigma) {
    return nullptr;
  }
  analysis::ScopeContext scope;
  scope.sigma = *ctx.sigma;
  scope.current_module = ctx.module_path;
  if (const auto lowered = LowerTypeForLayout(scope, type_opt)) {
    return *lowered;
  }
  return nullptr;
}

}  // namespace

// ============================================================================
// §6.5 Statement List Lowering
// ============================================================================

IRPtr LowerStmtList(const std::vector<syntax::Stmt>& stmts, LowerCtx& ctx) {
  SPEC_RULE("Lower-StmtList-Empty");
  SPEC_RULE("Lower-StmtList-Cons");

  if (stmts.empty()) {
    return EmptyIR();
  }

  std::vector<IRPtr> ir_parts;
  for (const auto& stmt : stmts) {
    ir_parts.push_back(LowerStmt(stmt, ctx));
  }

  return SeqIR(std::move(ir_parts));
}

// ============================================================================
// §6.5 Block Lowering
// ============================================================================

LowerResult LowerBlock(const syntax::Block& block, LowerCtx& ctx) {
  // Push a new scope for this block
  ctx.PushScope(false, false);

  // Lower all statements
  IRPtr stmts_ir = LowerStmtList(block.stmts, ctx);

  // Lower the tail expression if present
  IRPtr tail_ir = EmptyIR();
  IRValue result_value;

  if (block.tail_opt) {
    SPEC_RULE("Lower-Block-Tail");
    auto tail_result = LowerExpr(*block.tail_opt, ctx);
    tail_ir = tail_result.ir;
    result_value = tail_result.value;
  } else {
    SPEC_RULE("Lower-Block-Unit");
    // No tail expression - block produces unit
    result_value.kind = IRValue::Kind::Opaque;
    result_value.name = "unit";
  }

  // §6.8 Emit cleanup for variables in this scope
  CleanupPlan cleanup_plan = ComputeCleanupPlanForCurrentScope(ctx);
  CleanupPlan remainder = ComputeCleanupPlanRemainder(CleanupTarget::CurrentScope, ctx);
  IRPtr cleanup_ir = EmitCleanupWithRemainder(cleanup_plan, remainder, ctx);
  ctx.PopScope();

  const bool ends_with_return = BlockEndsWithReturn(block);
  IRBlock block_ir;
  block_ir.setup = stmts_ir;
  block_ir.body = ends_with_return ? tail_ir : SeqIR({tail_ir, cleanup_ir});
  block_ir.value = result_value;

  return LowerResult{MakeIR(std::move(block_ir)), result_value};
}

// ============================================================================
// §6.5 Let Statement Lowering
// ============================================================================

IRPtr LowerLetStmt(const syntax::LetStmt& stmt, LowerCtx& ctx) {
  SPEC_RULE("Lower-Stmt-Let");

  const auto& binding = stmt.binding;
  if (!binding.init) {
    return EmptyIR();
  }

  auto prev_suppress = ctx.suppress_temp_at_depth;
  ctx.suppress_temp_at_depth = ctx.temp_depth + 1;
  auto init_result = LowerExpr(*binding.init, ctx);
  ctx.suppress_temp_at_depth = prev_suppress;

  IRPtr bind_ir = EmptyIR();

  analysis::TypeRef var_type;
  var_type = LowerBindingType(binding.type_opt, ctx);
  if (!var_type && ctx.expr_type && binding.init) {
    var_type = ctx.expr_type(*binding.init);
  }
  const bool immovable = binding.op.lexeme == ":=";
  if (binding.pat) {
    RegisterPatternBindings(*binding.pat, var_type, ctx, immovable);
    bind_ir = LowerBindPattern(*binding.pat, init_result.value, ctx);
  } else {
    IRBindVar bind;
    bind.name = "anon_binding";
    bind.value = init_result.value;
    bind.type = var_type;
    bind_ir = MakeIR(std::move(bind));
    ctx.RegisterVar(bind.name, var_type, true, immovable);
  }

  return SeqIR({init_result.ir, bind_ir});
}

// ============================================================================
// §6.5 Var Statement Lowering
// ============================================================================

IRPtr LowerVarStmt(const syntax::VarStmt& stmt, LowerCtx& ctx) {
  SPEC_RULE("Lower-Stmt-Var");

  const auto& binding = stmt.binding;
  if (!binding.init) {
    return EmptyIR();
  }

  auto prev_suppress = ctx.suppress_temp_at_depth;
  ctx.suppress_temp_at_depth = ctx.temp_depth + 1;
  auto init_result = LowerExpr(*binding.init, ctx);
  ctx.suppress_temp_at_depth = prev_suppress;

  IRPtr bind_ir = EmptyIR();

  analysis::TypeRef var_type;
  var_type = LowerBindingType(binding.type_opt, ctx);
  if (!var_type && ctx.expr_type && binding.init) {
    var_type = ctx.expr_type(*binding.init);
  }
  const bool immovable = binding.op.lexeme == ":=";
  if (binding.pat) {
    RegisterPatternBindings(*binding.pat, var_type, ctx, immovable);
    bind_ir = LowerBindPattern(*binding.pat, init_result.value, ctx);
  } else {
    IRBindVar bind;
    bind.name = "anon_var";
    bind.value = init_result.value;
    bind.type = var_type;
    bind_ir = MakeIR(std::move(bind));
    ctx.RegisterVar(bind.name, var_type, true, immovable);
  }

  return SeqIR({init_result.ir, bind_ir});
}

// ============================================================================
// §6.5 Assignment Statement Lowering
// ============================================================================

IRPtr LowerAssignStmt(const syntax::AssignStmt& stmt, LowerCtx& ctx) {
  SPEC_RULE("Lower-Stmt-Assign");

  // Lower the RHS value
  auto rhs_result = LowerExpr(*stmt.value, ctx);

  // Write to the place
  IRPtr write_ir = LowerWritePlace(*stmt.place, rhs_result.value, ctx);

  return SeqIR({rhs_result.ir, write_ir});
}

// ============================================================================
// §6.5 Compound Assignment Statement Lowering
// ============================================================================

IRPtr LowerCompoundAssignStmt(const syntax::CompoundAssignStmt& stmt,
                              LowerCtx& ctx) {
  SPEC_RULE("Lower-Stmt-CompoundAssign");

  auto lhs_result = LowerReadPlace(*stmt.place, ctx);
  auto rhs_result = LowerExpr(*stmt.value, ctx);

  std::string op = stmt.op;
  if (!op.empty() && op.back() == '=') {
    op.pop_back();
  }

  IRValue new_value = ctx.FreshTempValue("binop");

  std::vector<IRPtr> op_parts;
  auto panic_reason = [&]() -> std::optional<PanicReason> {
    if (op == "/" || op == "%") {
      return PanicReason::DivZero;
    }
    if (op == "<<" || op == ">>") {
      return PanicReason::Shift;
    }
    if (op == "+" || op == "-" || op == "*" || op == "**") {
      return PanicReason::Overflow;
    }
    return std::nullopt;
  }();

  if (panic_reason.has_value()) {
    IRCheckOp check;
    check.op = op;
    check.reason = PanicReasonString(*panic_reason);
    check.lhs = lhs_result.value;
    check.rhs = rhs_result.value;
    op_parts.push_back(MakeIR(std::move(check)));
    op_parts.push_back(PanicCheck(ctx));
  }

  IRBinaryOp binop;
  binop.op = op;
  binop.lhs = lhs_result.value;
  binop.rhs = rhs_result.value;
  op_parts.push_back(MakeIR(std::move(binop)));

  IRPtr op_ir = SeqIR(std::move(op_parts));
  IRPtr write_ir = LowerWritePlace(*stmt.place, new_value, ctx);

  return SeqIR({lhs_result.ir, rhs_result.ir, op_ir, write_ir});
}

// ============================================================================
// §6.5 Expression Statement Lowering
// ============================================================================

IRPtr LowerExprStmt(const syntax::ExprStmt& stmt, LowerCtx& ctx) {
  SPEC_RULE("Lower-Stmt-Expr");

  auto expr_result = LowerExpr(*stmt.value, ctx);

  // The expression is evaluated for side effects
  // The result value is discarded
  return expr_result.ir;
}

// ============================================================================
// §6.5 Return Statement Lowering
// ============================================================================

IRPtr LowerReturnStmt(const syntax::ReturnStmt& stmt,
                      LowerCtx& ctx,
                      const std::vector<TempValue>& temps) {
  std::vector<IRPtr> ir_parts;

  // Lower the return expression if present
  IRValue return_value;
  if (stmt.value_opt) {
    SPEC_RULE("Lower-Stmt-Return");
    auto prev_suppress = ctx.suppress_temp_at_depth;
    ctx.suppress_temp_at_depth = ctx.temp_depth + 1;
    auto expr_result = LowerExpr(*stmt.value_opt, ctx);
    ctx.suppress_temp_at_depth = prev_suppress;
    ir_parts.push_back(expr_result.ir);
    return_value = expr_result.value;
  } else {
    SPEC_RULE("Lower-Stmt-Return-Unit");
    return_value = IRValue{IRValue::Kind::Opaque, "unit", {}};
  }

  // Drop statement-scoped temporaries before unwinding scopes.
  if (!temps.empty()) {
    ir_parts.push_back(CleanupList(temps, ctx));
  }

  // §6.8 Emit cleanup for all variables from current scope to function root
  CleanupPlan cleanup_plan = ComputeCleanupPlanToFunctionRoot(ctx);
  ir_parts.push_back(EmitCleanup(cleanup_plan, ctx));

  // Emit the return
  IRReturn ret;
  ret.value = return_value;
  ir_parts.push_back(MakeIR(std::move(ret)));

  return SeqIR(std::move(ir_parts));
}

// ============================================================================
// §6.5 Break Statement Lowering
// ============================================================================

IRPtr LowerBreakStmt(const syntax::BreakStmt& stmt,
                     LowerCtx& ctx,
                     const std::vector<TempValue>& temps) {
  std::vector<IRPtr> ir_parts;

  // Lower the break value if present
  IRValue break_value;
  if (stmt.value_opt) {
    SPEC_RULE("Lower-Stmt-Break");
    auto prev_suppress = ctx.suppress_temp_at_depth;
    ctx.suppress_temp_at_depth = ctx.temp_depth + 1;
    auto expr_result = LowerExpr(*stmt.value_opt, ctx);
    ctx.suppress_temp_at_depth = prev_suppress;
    ir_parts.push_back(expr_result.ir);
    break_value = expr_result.value;
  } else {
    SPEC_RULE("Lower-Stmt-Break-Unit");
    break_value = IRValue{IRValue::Kind::Opaque, "unit", {}};
  }

  // Drop statement-scoped temporaries before unwinding scopes.
  if (!temps.empty()) {
    ir_parts.push_back(CleanupList(temps, ctx));
  }

  // §6.8 Emit cleanup for variables from current scope to loop scope
  CleanupPlan cleanup_plan = ComputeCleanupPlanToLoopScope(ctx);
  CleanupPlan remainder = ComputeCleanupPlanRemainder(CleanupTarget::ToLoopScope, ctx);
  ir_parts.push_back(EmitCleanupWithRemainder(cleanup_plan, remainder, ctx));

  // Emit the break
  IRBreak brk;
  brk.value = break_value;
  ir_parts.push_back(MakeIR(std::move(brk)));

  return SeqIR(std::move(ir_parts));
}

// ============================================================================
// §6.5 Continue Statement Lowering
// ============================================================================

IRPtr LowerContinueStmt(const syntax::ContinueStmt& /*stmt*/,
                        LowerCtx& ctx,
                        const std::vector<TempValue>& temps) {
  SPEC_RULE("Lower-Stmt-Continue");

  std::vector<IRPtr> ir_parts;

  if (!temps.empty()) {
    ir_parts.push_back(CleanupList(temps, ctx));
  }

  // §6.8 Emit cleanup for variables from current scope to loop scope
  CleanupPlan cleanup_plan = ComputeCleanupPlanToLoopScope(ctx);
  CleanupPlan remainder = ComputeCleanupPlanRemainder(CleanupTarget::ToLoopScope, ctx);
  ir_parts.push_back(EmitCleanupWithRemainder(cleanup_plan, remainder, ctx));

  // Emit the continue
  IRContinue cont;
  ir_parts.push_back(MakeIR(std::move(cont)));

  return SeqIR(std::move(ir_parts));
}

// ============================================================================
// §6.5 Main Statement Dispatch
// ============================================================================

IRPtr LowerStmt(const syntax::Stmt& stmt, LowerCtx& ctx) {
  std::vector<TempValue> temps;
  auto* prev_sink = ctx.temp_sink;
  ctx.temp_sink = &temps;

  bool temps_handled = false;
  auto is_noop = [](const IRPtr& ir) {
    return !ir || std::holds_alternative<IROpaque>(ir->node);
  };

  IRPtr ir = std::visit(
      [&ctx, &temps, &temps_handled](const auto& node) -> IRPtr {
        using T = std::decay_t<decltype(node)>;

        auto block_result_type = [&ctx](const std::shared_ptr<syntax::Block>& block) -> analysis::TypeRef {
          if (!block) {
            return nullptr;
          }
          if (block->tail_opt && ctx.expr_type) {
            return ctx.expr_type(*block->tail_opt);
          }
          if (!block->tail_opt) {
            return analysis::MakeTypePrim("()");
          }
          return nullptr;
        };

        if constexpr (std::is_same_v<T, syntax::ErrorStmt>) {
          SPEC_RULE("Lower-Stmt-Error");
          return LowerPanic(PanicReason::ErrorStmt, ctx);
        } else if constexpr (std::is_same_v<T, syntax::LetStmt>) {
          return LowerLetStmt(node, ctx);
        } else if constexpr (std::is_same_v<T, syntax::VarStmt>) {
          return LowerVarStmt(node, ctx);
        } else if constexpr (std::is_same_v<T, syntax::ShadowLetStmt>) {
          SPEC_RULE("Lower-Stmt-ShadowLet");
          if (!node.init) {
            return EmptyIR();
          }
          auto prev_suppress = ctx.suppress_temp_at_depth;
          ctx.suppress_temp_at_depth = ctx.temp_depth + 1;
          auto init_result = LowerExpr(*node.init, ctx);
          ctx.suppress_temp_at_depth = prev_suppress;
          IRBindVar bind;
          bind.name = node.name;
          bind.value = init_result.value;
          analysis::TypeRef var_type;
          var_type = LowerBindingType(node.type_opt, ctx);
          if (!var_type && ctx.expr_type) {
            var_type = ctx.expr_type(*node.init);
          }
          bind.type = var_type;
          ctx.RegisterVar(node.name, var_type, true, false);
          return SeqIR({init_result.ir, MakeIR(std::move(bind))});
        } else if constexpr (std::is_same_v<T, syntax::ShadowVarStmt>) {
          SPEC_RULE("Lower-Stmt-ShadowVar");
          if (!node.init) {
            return EmptyIR();
          }
          auto prev_suppress = ctx.suppress_temp_at_depth;
          ctx.suppress_temp_at_depth = ctx.temp_depth + 1;
          auto init_result = LowerExpr(*node.init, ctx);
          ctx.suppress_temp_at_depth = prev_suppress;
          IRBindVar bind;
          bind.name = node.name;
          bind.value = init_result.value;
          analysis::TypeRef var_type;
          var_type = LowerBindingType(node.type_opt, ctx);
          if (!var_type && ctx.expr_type) {
            var_type = ctx.expr_type(*node.init);
          }
          bind.type = var_type;
          ctx.RegisterVar(node.name, var_type, true, false);
          return SeqIR({init_result.ir, MakeIR(std::move(bind))});
        } else if constexpr (std::is_same_v<T, syntax::AssignStmt>) {
          return LowerAssignStmt(node, ctx);
        } else if constexpr (std::is_same_v<T, syntax::CompoundAssignStmt>) {
          return LowerCompoundAssignStmt(node, ctx);
        } else if constexpr (std::is_same_v<T, syntax::ExprStmt>) {
          return LowerExprStmt(node, ctx);
        } else if constexpr (std::is_same_v<T, syntax::DeferStmt>) {
          SPEC_RULE("Lower-Stmt-Defer");
          IRDefer defer;
          defer.block = node.body;
          IRPtr defer_ir = MakeIR(std::move(defer));
          ctx.RegisterDefer(defer_ir);
          return defer_ir;
        } else if constexpr (std::is_same_v<T, syntax::RegionStmt>) {
          SPEC_RULE("Lower-Stmt-Region");
          if (!node.body) {
            return EmptyIR();
          }

          syntax::ExprPtr opts_expr = node.opts_opt;
          if (!opts_expr) {
            auto ident = std::make_shared<syntax::Expr>();
            ident->node = syntax::IdentifierExpr{"RegionOptions"};
            syntax::CallExpr call;
            call.callee = ident;
            call.args = {};
            auto expr = std::make_shared<syntax::Expr>();
            expr->node = std::move(call);
            opts_expr = expr;
          }

          auto opts_result = LowerExpr(*opts_expr, ctx);
          if (node.alias_opt.has_value()) {
            ctx.PushScope(false, true);
            analysis::TypePath region_path;
            region_path.push_back("Region");
            const auto region_type = analysis::MakeTypeModalState(std::move(region_path),
                                                              "Active");
            ctx.RegisterVar(*node.alias_opt, region_type, false, true);
          }

          auto body_result = LowerBlock(*node.body, ctx);

          if (node.alias_opt.has_value()) {
            ctx.PopScope();
          }
          if (ctx.temp_sink) {
            ctx.RegisterTempValue(body_result.value, block_result_type(node.body));
          }

          IRRegion region;
          region.owner = opts_result.value;
          region.alias = node.alias_opt;
          region.body = body_result.ir;
          region.value = body_result.value;

          return SeqIR({opts_result.ir, MakeIR(std::move(region))});
        } else if constexpr (std::is_same_v<T, syntax::FrameStmt>) {
          if (!node.body) {
            return EmptyIR();
          }

          auto body_result = LowerBlock(*node.body, ctx);
          if (ctx.temp_sink) {
            ctx.RegisterTempValue(body_result.value, block_result_type(node.body));
          }

          if (!node.target_opt.has_value()) {
            SPEC_RULE("Lower-Stmt-Frame-Implicit");
            IRFrame frame;
            frame.region = std::nullopt;
            frame.body = body_result.ir;
            frame.value = body_result.value;
            return MakeIR(std::move(frame));
          }

          SPEC_RULE("Lower-Stmt-Frame-Explicit");
          syntax::IdentifierExpr ident;
          ident.name = *node.target_opt;
          syntax::Expr region_expr;
          region_expr.span = node.span;
          region_expr.node = ident;
          auto region_result = LowerExpr(region_expr, ctx);

          IRFrame frame;
          frame.region = region_result.value;
          frame.body = body_result.ir;
          frame.value = body_result.value;

          return SeqIR({region_result.ir, MakeIR(std::move(frame))});
        } else if constexpr (std::is_same_v<T, syntax::ReturnStmt>) {
          temps_handled = true;
          return LowerReturnStmt(node, ctx, temps);
        } else if constexpr (std::is_same_v<T, syntax::ResultStmt>) {
          temps_handled = true;
          SPEC_RULE("Lower-Stmt-Result");
          auto prev_suppress = ctx.suppress_temp_at_depth;
          ctx.suppress_temp_at_depth = ctx.temp_depth + 1;
          auto expr_result = LowerExpr(*node.value, ctx);
          ctx.suppress_temp_at_depth = prev_suppress;
          std::vector<IRPtr> parts;
          parts.push_back(expr_result.ir);
          if (!temps.empty()) {
            parts.push_back(CleanupList(temps, ctx));
          }
          IRResult res;
          res.value = expr_result.value;
          parts.push_back(MakeIR(std::move(res)));
          return SeqIR(std::move(parts));
        } else if constexpr (std::is_same_v<T, syntax::BreakStmt>) {
          temps_handled = true;
          return LowerBreakStmt(node, ctx, temps);
        } else if constexpr (std::is_same_v<T, syntax::ContinueStmt>) {
          temps_handled = true;
          return LowerContinueStmt(node, ctx, temps);
        } else if constexpr (std::is_same_v<T, syntax::UnsafeBlockStmt>) {
          SPEC_RULE("Lower-Stmt-UnsafeBlock");
          if (node.body) {
            auto body_result = LowerBlock(*node.body, ctx);
            if (ctx.temp_sink) {
              ctx.RegisterTempValue(body_result.value, block_result_type(node.body));
            }
            return body_result.ir;
          }
          return EmptyIR();
        } else {
          // Unknown statement form
          return EmptyIR();
        }
      },
      stmt);

  ctx.temp_sink = prev_sink;

  if (!temps_handled) {
    IRPtr temp_cleanup = CleanupList(temps, ctx);
    if (!is_noop(temp_cleanup)) {
      return SeqIR({ir, temp_cleanup});
    }
  }

  return ir;
}

// ============================================================================
// §6.5 Loop Lowering
// ============================================================================

LowerResult LowerLoopInfinite(const syntax::LoopInfiniteExpr& expr,
                              LowerCtx& ctx) {
  SPEC_RULE("Lower-Loop-Infinite");

  // Push a loop scope for break/continue cleanup tracking
  ctx.PushScope(true, false);

  // Lower the body
  LowerResult body_result = LowerBlock(*expr.body, ctx);

  // Pop the loop scope
  ctx.PopScope();

  // Create infinite loop IR
  IRLoop loop;
  loop.kind = IRLoopKind::Infinite;
  loop.body_ir = body_result.ir;
  loop.body_value = body_result.value;

  IRValue result = ctx.FreshTempValue("loop");

  return LowerResult{MakeIR(std::move(loop)), result};
}

LowerResult LowerLoopConditional(const syntax::LoopConditionalExpr& expr,
                                 LowerCtx& ctx) {
  SPEC_RULE("Lower-Loop-Cond");

  // Push a loop scope for break/continue cleanup tracking
  ctx.PushScope(true, false);

  // Lower the condition
  auto cond_result = LowerExpr(*expr.cond, ctx);

  // Lower the body
  LowerResult body_result = LowerBlock(*expr.body, ctx);

  // Pop the loop scope
  ctx.PopScope();

  // Create while loop IR
  IRLoop loop;
  loop.kind = IRLoopKind::Conditional;
  loop.cond_ir = cond_result.ir;
  loop.cond_value = cond_result.value;
  loop.body_ir = body_result.ir;
  loop.body_value = body_result.value;

  IRValue result = ctx.FreshTempValue("while");

  return LowerResult{MakeIR(std::move(loop)), result};
}

LowerResult LowerLoopIter(const syntax::LoopIterExpr& expr, LowerCtx& ctx) {
  SPEC_RULE("Lower-Loop-Iter");

  // Push a loop scope for break/continue cleanup tracking
  ctx.PushScope(true, false);

  // Lower the iterator expression
  auto iter_result = LowerExpr(*expr.iter, ctx);

  analysis::TypeRef pattern_type;
  if (ctx.expr_type) {
    pattern_type = LoopPatternType(ctx.expr_type(*expr.iter));
  }
  RegisterPatternBindings(*expr.pattern, pattern_type, ctx);

  // Lower the body
  LowerResult body_result = LowerBlock(*expr.body, ctx);

  // Pop the loop scope
  ctx.PopScope();

  // Create iter loop IR
  IRLoop loop;
  loop.kind = IRLoopKind::Iter;
  loop.pattern = expr.pattern;
  loop.type_opt = expr.type_opt;
  loop.iter_ir = iter_result.ir;
  loop.iter_value = iter_result.value;
  loop.body_ir = body_result.ir;
  loop.body_value = body_result.value;

  IRValue result = ctx.FreshTempValue("for");

  return LowerResult{MakeIR(std::move(loop)), result};
}

LowerResult LowerLoop(const syntax::Expr& loop, LowerCtx& ctx) {
  return std::visit(
      [&ctx](const auto& node) -> LowerResult {
        using T = std::decay_t<decltype(node)>;

        if constexpr (std::is_same_v<T, syntax::LoopInfiniteExpr>) {
          return LowerLoopInfinite(node, ctx);
        } else if constexpr (std::is_same_v<T, syntax::LoopConditionalExpr>) {
          return LowerLoopConditional(node, ctx);
        } else if constexpr (std::is_same_v<T, syntax::LoopIterExpr>) {
          return LowerLoopIter(node, ctx);
        } else {
          // Not a loop expression
          IRValue error_value = ctx.FreshTempValue("not_a_loop");
          return LowerResult{EmptyIR(), error_value};
        }
      },
      loop.node);
}

// ============================================================================
// §6.5 Cleanup
// ============================================================================

IRPtr CleanupList(const std::vector<TempValue>& temps, LowerCtx& ctx) {
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

// ============================================================================
// Anchor function
// ============================================================================

void AnchorStmtLoweringRules() {
  SPEC_RULE("Lower-Stmt-Correctness");
  SPEC_RULE("Lower-Block-Correctness");
  SPEC_RULE("Lower-Loop-Correctness");

  SPEC_RULE("Lower-StmtList-Empty");
  SPEC_RULE("Lower-StmtList-Cons");

  SPEC_RULE("Lower-Stmt-Let");
  SPEC_RULE("Lower-Stmt-Var");
  SPEC_RULE("Lower-Stmt-ShadowLet");
  SPEC_RULE("Lower-Stmt-ShadowVar");
  SPEC_RULE("Lower-Stmt-Assign");
  SPEC_RULE("Lower-Stmt-CompoundAssign");
  SPEC_RULE("Lower-Stmt-Expr");
  SPEC_RULE("Lower-Stmt-Defer");
  SPEC_RULE("Lower-Stmt-Region");
  SPEC_RULE("Lower-Stmt-Frame-Implicit");
  SPEC_RULE("Lower-Stmt-Frame-Explicit");
  SPEC_RULE("Lower-Stmt-Return");
  SPEC_RULE("Lower-Stmt-Return-Unit");
  SPEC_RULE("Lower-Stmt-Result");
  SPEC_RULE("Lower-Stmt-Break");
  SPEC_RULE("Lower-Stmt-Break-Unit");
  SPEC_RULE("Lower-Stmt-Continue");
  SPEC_RULE("Lower-Stmt-UnsafeBlock");
  SPEC_RULE("Lower-Stmt-Error");

  SPEC_RULE("Lower-Block-Tail");
  SPEC_RULE("Lower-Block-Unit");

  SPEC_RULE("Lower-Loop-Infinite");
  SPEC_RULE("Lower-Loop-Cond");
  SPEC_RULE("Lower-Loop-Iter");
}


}  // namespace cursive0::codegen
