// =============================================================================
// Expression Lowering Common Utilities Implementation
// =============================================================================
//
// SPEC REFERENCE: CursiveSpecification.md Section 6.4 (Expression Lowering)
//   - LowerExpr judgment (Lines 16048+)
//   - Place representation (Lines 16098+)
//   - Evaluation order (Lines 16024+)
//
// MIGRATED FROM:
//   - cursive-bootstrap/src/04_codegen/lower/lower_expr_core.cpp
//   - cursive-bootstrap/src/04_codegen/lower/lower_expr_places.cpp
//
// =============================================================================

#include "05_codegen/lower/expr/expr_common.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <string_view>
#include <variant>

#include "00_core/assert_spec.h"
#include "04_analysis/attributes/attribute_registry.h"
#include "04_analysis/generics/monomorphize.h"
#include "04_analysis/resolve/scopes.h"
#include "04_analysis/typing/type_expr.h"
#include "05_codegen/abi/abi.h"
#include "05_codegen/checks/checks.h"
#include "05_codegen/cleanup/cleanup.h"
#include "05_codegen/globals/globals.h"
#include "05_codegen/intrinsics/builtins.h"
#include "05_codegen/layout/layout.h"
#include "05_codegen/lower/expr/addr_of.h"
#include "05_codegen/lower/expr/all_expr.h"
#include "05_codegen/lower/expr/alloc_expr.h"
#include "05_codegen/lower/expr/array_literal.h"
#include "05_codegen/lower/expr/binary.h"
#include "05_codegen/lower/expr/block_expr.h"
#include "05_codegen/lower/expr/call.h"
#include "05_codegen/lower/expr/cast.h"
#include "05_codegen/lower/expr/closure_expr.h"
#include "05_codegen/lower/expr/contract_entry.h"
#include "05_codegen/lower/expr/contract_result.h"
#include "05_codegen/lower/expr/deref.h"
#include "05_codegen/lower/expr/dispatch_expr.h"
#include "05_codegen/lower/expr/enum_literal.h"
#include "05_codegen/lower/expr/error_expr.h"
#include "05_codegen/lower/expr/field_access.h"
#include "05_codegen/lower/expr/identifier.h"
#include "05_codegen/lower/expr/if_expr.h"
#include "05_codegen/lower/expr/index_access.h"
#include "05_codegen/lower/expr/literal.h"
#include "05_codegen/lower/expr/loop_conditional.h"
#include "05_codegen/lower/expr/loop_infinite.h"
#include "05_codegen/lower/expr/loop_iter.h"
#include "05_codegen/lower/expr/if_case_expr.h"
#include "05_codegen/lower/expr/method_call.h"
#include "05_codegen/lower/expr/move_expr.h"
#include "05_codegen/lower/expr/null_ptr.h"
#include "05_codegen/lower/expr/parallel_expr.h"
#include "05_codegen/lower/expr/path.h"
#include "05_codegen/lower/expr/pipeline_expr.h"
#include "05_codegen/lower/expr/propagate_expr.h"
#include "05_codegen/lower/expr/qualified_apply.h"
#include "05_codegen/lower/expr/qualified_name.h"
#include "05_codegen/lower/expr/race_expr.h"
#include "05_codegen/lower/expr/range.h"
#include "05_codegen/lower/expr/record_literal.h"
#include "05_codegen/lower/expr/spawn_expr.h"
#include "05_codegen/lower/expr/sync_expr.h"
#include "05_codegen/lower/expr/transmute_expr.h"
#include "05_codegen/lower/expr/tuple_access.h"
#include "05_codegen/lower/expr/tuple_literal.h"
#include "05_codegen/lower/expr/unary.h"
#include "05_codegen/lower/expr/unsafe_block_expr.h"
#include "05_codegen/lower/expr/wait_expr.h"
#include "05_codegen/lower/expr/yield_expr.h"
#include "05_codegen/lower/expr/yield_from_expr.h"

namespace cursive::codegen {

const analysis::ScopeContext& ScopeForLowering(const LowerCtx& ctx) {
  static const analysis::ScopeContext kEmptyScope{};

  struct ScopeCache {
    const analysis::Sigma* sigma = nullptr;
    analysis::ExprTypeMap* expr_types = nullptr;
    analysis::DynamicRefineExprMap* dynamic_refine_checks = nullptr;
    std::vector<std::string> module_path;
    analysis::ScopeContext scope;
  };

  thread_local ScopeCache cache;
  if (!ctx.sigma) {
    return kEmptyScope;
  }

  if (cache.sigma != ctx.sigma || cache.expr_types != ctx.expr_types ||
      cache.dynamic_refine_checks != ctx.dynamic_refine_checks ||
      cache.module_path != ctx.module_path) {
    cache.sigma = ctx.sigma;
    cache.expr_types = ctx.expr_types;
    cache.dynamic_refine_checks = ctx.dynamic_refine_checks;
    cache.module_path = ctx.module_path;
    cache.scope = analysis::ScopeContext{};
    cache.scope.sigma = *ctx.sigma;
    cache.scope.sigma_source = ctx.sigma;
    cache.scope.current_module = ctx.module_path;
    cache.scope.expr_types = ctx.expr_types;
    cache.scope.dynamic_refine_checks = ctx.dynamic_refine_checks;
  }

  return cache.scope;
}

const analysis::ScopeContext& ScopeForLowering(const LowerCtx* ctx) {
  static const analysis::ScopeContext kEmptyScope{};
  if (!ctx) {
    return kEmptyScope;
  }
  return ScopeForLowering(*ctx);
}

namespace {

bool DynamicChecksEnabledForExpr(const ast::Expr& expr, bool inherited_dynamic) {
  (void)expr;
  return inherited_dynamic;
}

ast::ExprPtr MakeLoweringExpr(const core::Span& span, ast::ExprNode node) {
  auto expr = std::make_shared<ast::Expr>();
  expr->span = span;
  expr->node = std::move(node);
  return expr;
}

ast::ExprPtr SubstituteRefinementSelfWithResult(const ast::ExprPtr& expr) {
  if (!expr) {
    return expr;
  }
  if (const auto* ident = std::get_if<ast::IdentifierExpr>(&expr->node)) {
    if (analysis::IdEq(ident->name, "self")) {
      return MakeLoweringExpr(expr->span, ast::ResultExpr{});
    }
    return expr;
  }

  return std::visit(
      [&](const auto& node) -> ast::ExprPtr {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, ast::BinaryExpr>) {
          auto out = node;
          out.lhs = SubstituteRefinementSelfWithResult(node.lhs);
          out.rhs = SubstituteRefinementSelfWithResult(node.rhs);
          return MakeLoweringExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, ast::UnaryExpr>) {
          auto out = node;
          out.value = SubstituteRefinementSelfWithResult(node.value);
          return MakeLoweringExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, ast::FieldAccessExpr>) {
          auto out = node;
          out.base = SubstituteRefinementSelfWithResult(node.base);
          return MakeLoweringExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, ast::TupleAccessExpr>) {
          auto out = node;
          out.base = SubstituteRefinementSelfWithResult(node.base);
          return MakeLoweringExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, ast::IndexAccessExpr>) {
          auto out = node;
          out.base = SubstituteRefinementSelfWithResult(node.base);
          out.index = SubstituteRefinementSelfWithResult(node.index);
          return MakeLoweringExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, ast::CallExpr>) {
          auto out = node;
          out.callee = SubstituteRefinementSelfWithResult(node.callee);
          for (auto& arg : out.args) {
            arg.value = SubstituteRefinementSelfWithResult(arg.value);
          }
          return MakeLoweringExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, ast::QualifiedApplyExpr>) {
          auto out = node;
          if (std::holds_alternative<ast::ParenArgs>(node.args)) {
            auto paren = std::get<ast::ParenArgs>(node.args);
            for (auto& arg : paren.args) {
              arg.value = SubstituteRefinementSelfWithResult(arg.value);
            }
            out.args = std::move(paren);
          } else {
            auto brace = std::get<ast::BraceArgs>(node.args);
            for (auto& field : brace.fields) {
              field.value = SubstituteRefinementSelfWithResult(field.value);
            }
            out.args = std::move(brace);
          }
          return MakeLoweringExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, ast::MethodCallExpr>) {
          auto out = node;
          out.receiver = SubstituteRefinementSelfWithResult(node.receiver);
          for (auto& arg : out.args) {
            arg.value = SubstituteRefinementSelfWithResult(arg.value);
          }
          return MakeLoweringExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, ast::CastExpr>) {
          auto out = node;
          out.value = SubstituteRefinementSelfWithResult(node.value);
          return MakeLoweringExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, ast::RangeExpr>) {
          auto out = node;
          out.lhs = SubstituteRefinementSelfWithResult(node.lhs);
          out.rhs = SubstituteRefinementSelfWithResult(node.rhs);
          return MakeLoweringExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, ast::AttributedExpr>) {
          auto out = node;
          out.expr = SubstituteRefinementSelfWithResult(node.expr);
          return MakeLoweringExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, ast::IfExpr>) {
          auto out = node;
          out.cond = SubstituteRefinementSelfWithResult(node.cond);
          out.then_expr = SubstituteRefinementSelfWithResult(node.then_expr);
          out.else_expr = SubstituteRefinementSelfWithResult(node.else_expr);
          return MakeLoweringExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, ast::IfCaseExpr>) {
          auto out = node;
          out.scrutinee = SubstituteRefinementSelfWithResult(node.scrutinee);
          for (auto& case_clause : out.cases) {
            case_clause.body =
                SubstituteRefinementSelfWithResult(case_clause.body);
          }
          out.else_expr = SubstituteRefinementSelfWithResult(node.else_expr);
          return MakeLoweringExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, ast::IfIsExpr>) {
          auto out = node;
          out.scrutinee = SubstituteRefinementSelfWithResult(node.scrutinee);
          out.then_expr = SubstituteRefinementSelfWithResult(node.then_expr);
          out.else_expr = SubstituteRefinementSelfWithResult(node.else_expr);
          return MakeLoweringExpr(expr->span, out);
        } else if constexpr (std::is_same_v<T, ast::EntryExpr>) {
          auto out = node;
          out.expr = SubstituteRefinementSelfWithResult(node.expr);
          return MakeLoweringExpr(expr->span, out);
        } else {
          return expr;
        }
      },
      expr->node);
}

IRPtr EmitDynamicRefinementChecksImpl(const ast::Expr& expr,
                                      const IRValue& value,
                                      analysis::TypeRef value_type,
                                      LowerCtx& ctx) {
  const bool dynamic_checks_enabled =
      DynamicChecksEnabledForExpr(expr, ctx.dynamic_checks);
  if (!dynamic_checks_enabled || !ctx.dynamic_refine_checks) {
    return EmptyIR();
  }

  const auto it = ctx.dynamic_refine_checks->find(&expr);
  if (it == ctx.dynamic_refine_checks->end() || it->second.empty()) {
    return EmptyIR();
  }

  const analysis::TypeRef bool_type = analysis::MakeTypePrim("bool");
  std::vector<IRPtr> parts;
  for (const auto& refine_type : it->second) {
    if (!refine_type) {
      continue;
    }
    const auto* refine = std::get_if<analysis::TypeRefine>(&refine_type->node);
    if (!refine || !refine->predicate) {
      continue;
    }

    const auto predicate = SubstituteRefinementSelfWithResult(refine->predicate);
    if (!predicate) {
      continue;
    }

    const auto prev_result = ctx.contract_result_value;
    const auto prev_proc_ret_type = ctx.proc_ret_type;
    ctx.contract_result_value = value;
    ctx.proc_ret_type = value_type ? value_type : refine->base;
    auto pred_result = LowerExpr(*predicate, ctx);
    ctx.proc_ret_type = prev_proc_ret_type;
    ctx.contract_result_value = prev_result;

    ctx.RegisterValueType(pred_result.value, bool_type);

    IRIf check;
    check.cond = pred_result.value;
    check.then_ir = EmptyIR();
    check.else_ir = LowerContractViolation(ContractKind::TypeInv,
                                           ctx,
                                           refine->predicate.get(),
                                           expr.span);
    check.result = ctx.FreshTempValue("refine_check");
    ctx.RegisterValueType(check.result, bool_type);

    parts.push_back(pred_result.ir);
    parts.push_back(MakeIR(std::move(check)));
  }

  if (parts.empty()) {
    return EmptyIR();
  }
  return SeqIR(std::move(parts));
}

std::vector<std::uint8_t> LEBytesU64(std::uint64_t value) {
  std::vector<std::uint8_t> bytes;
  bytes.reserve(8);
  for (std::size_t i = 0; i < 8; ++i) {
    bytes.push_back(static_cast<std::uint8_t>((value >> (8 * i)) & 0xFFu));
  }
  return bytes;
}

IRValue StringImmediate(std::string_view text) {
  IRValue value;
  value.kind = IRValue::Kind::Immediate;
  value.name = "\"" + std::string(text) + "\"";
  value.bytes.assign(text.begin(), text.end());
  return value;
}

IRValue U64Immediate(std::uint64_t value) {
  IRValue out;
  out.kind = IRValue::Kind::Immediate;
  out.name = std::to_string(value);
  out.bytes = LEBytesU64(value);
  return out;
}

struct RuntimeTraceSpanFields {
  std::string file = "-";
  std::uint64_t start_line = 0;
  std::uint64_t start_col = 0;
  std::uint64_t end_line = 0;
  std::uint64_t end_col = 0;
};

RuntimeTraceSpanFields NormalizeRuntimeTraceSpan(
    const std::optional<core::Span>& span) {
  RuntimeTraceSpanFields out;
  if (!span.has_value()) {
    return out;
  }
  if (!span->file.empty()) {
    out.file = span->file;
  }
  out.start_line = static_cast<std::uint64_t>(span->start_line);
  out.start_col = static_cast<std::uint64_t>(span->start_col);
  out.end_line = static_cast<std::uint64_t>(span->end_line);
  out.end_col = static_cast<std::uint64_t>(span->end_col);
  return out;
}

void AppendRuntimeTraceSpanArgs(IRCall& call,
                                const std::optional<core::Span>& span) {
  const RuntimeTraceSpanFields fields = NormalizeRuntimeTraceSpan(span);
  call.args.push_back(StringImmediate(fields.file));
  call.args.push_back(U64Immediate(fields.start_line));
  call.args.push_back(U64Immediate(fields.start_col));
  call.args.push_back(U64Immediate(fields.end_line));
  call.args.push_back(U64Immediate(fields.end_col));
}

IRPtr EmitRuntimeTraceInternal(std::string_view rule_id,
                               const std::optional<core::Span>& span,
                               std::string_view payload) {
  if (rule_id.empty()) {
    return EmptyIR();
  }
  IRCall call;
  call.callee.kind = IRValue::Kind::Symbol;
  call.callee.name = RuntimeConformanceEmitSym();
  call.args.push_back(StringImmediate(rule_id));
  AppendRuntimeTraceSpanArgs(call, span);
  call.args.push_back(StringImmediate(payload));
  return MakeIR(std::move(call));
}

IRValue BoolConstValue(bool value) {
  IRValue out;
  out.kind = IRValue::Kind::Immediate;
  out.name = value ? "true" : "false";
  out.bytes = {static_cast<std::uint8_t>(value ? 1u : 0u)};
  return out;
}

IRValue U8ConstValue(std::uint8_t value) {
  IRValue out;
  out.kind = IRValue::Kind::Immediate;
  out.name = std::to_string(value);
  out.bytes = {value};
  return out;
}

enum class LogActualKind {
  Unsupported,
  Integer,
  Boolean,
  Floating,
  Pointer,
  StringView,
  StringManaged,
  BytesView,
  BytesManaged,
};

struct LogActualDispatch {
  LogActualKind kind = LogActualKind::Unsupported;
  bool is_signed = false;
  std::uint8_t bits = 0;
};

LogActualDispatch SelectLogActualDispatch(const analysis::TypeRef& type) {
  LogActualDispatch dispatch;
  analysis::TypeRef cur = analysis::StripPerm(type);
  if (!cur) {
    cur = type;
  }
  while (cur) {
    if (const auto* refine = std::get_if<analysis::TypeRefine>(&cur->node)) {
      cur = analysis::StripPerm(refine->base);
      if (!cur) {
        cur = refine->base;
      }
      continue;
    }
    break;
  }
  if (!cur) {
    return dispatch;
  }

  if (std::holds_alternative<analysis::TypeRawPtr>(cur->node) ||
      std::holds_alternative<analysis::TypePtr>(cur->node)) {
    dispatch.kind = LogActualKind::Pointer;
    return dispatch;
  }

  if (const auto* string_ty = std::get_if<analysis::TypeString>(&cur->node)) {
    if (!string_ty->state.has_value() ||
        *string_ty->state == analysis::StringState::View) {
      dispatch.kind = LogActualKind::StringView;
    } else {
      dispatch.kind = LogActualKind::StringManaged;
    }
    return dispatch;
  }

  if (const auto* bytes_ty = std::get_if<analysis::TypeBytes>(&cur->node)) {
    if (!bytes_ty->state.has_value() ||
        *bytes_ty->state == analysis::BytesState::View) {
      dispatch.kind = LogActualKind::BytesView;
    } else {
      dispatch.kind = LogActualKind::BytesManaged;
    }
    return dispatch;
  }

  if (const auto* path_ty = std::get_if<analysis::TypePathType>(&cur->node)) {
    if (!path_ty->path.empty()) {
      const std::string& leaf = path_ty->path.back();
      if (leaf == "string" || leaf == "String") {
        dispatch.kind = LogActualKind::StringView;
        return dispatch;
      }
      if (leaf == "bytes" || leaf == "Bytes") {
        dispatch.kind = LogActualKind::BytesView;
        return dispatch;
      }
    }
  }

  const auto* prim = std::get_if<analysis::TypePrim>(&cur->node);
  if (!prim) {
    return dispatch;
  }

  if (prim->name == "bool") {
    dispatch.kind = LogActualKind::Boolean;
    dispatch.bits = 8u;
    return dispatch;
  }

  if (prim->name == "f16") {
    dispatch.kind = LogActualKind::Floating;
    dispatch.bits = 16u;
    return dispatch;
  }
  if (prim->name == "f32") {
    dispatch.kind = LogActualKind::Floating;
    dispatch.bits = 32u;
    return dispatch;
  }
  if (prim->name == "f64") {
    dispatch.kind = LogActualKind::Floating;
    dispatch.bits = 64u;
    return dispatch;
  }

  if (prim->name == "char") {
    dispatch.kind = LogActualKind::Integer;
    dispatch.is_signed = false;
    dispatch.bits = 32u;
    return dispatch;
  }

  if (prim->name == "i8") {
    dispatch.kind = LogActualKind::Integer;
    dispatch.is_signed = true;
    dispatch.bits = 8u;
    return dispatch;
  }
  if (prim->name == "i16") {
    dispatch.kind = LogActualKind::Integer;
    dispatch.is_signed = true;
    dispatch.bits = 16u;
    return dispatch;
  }
  if (prim->name == "i32") {
    dispatch.kind = LogActualKind::Integer;
    dispatch.is_signed = true;
    dispatch.bits = 32u;
    return dispatch;
  }
  if (prim->name == "i64" || prim->name == "isize") {
    dispatch.kind = LogActualKind::Integer;
    dispatch.is_signed = true;
    dispatch.bits = 64u;
    return dispatch;
  }

  if (prim->name == "u8") {
    dispatch.kind = LogActualKind::Integer;
    dispatch.is_signed = false;
    dispatch.bits = 8u;
    return dispatch;
  }
  if (prim->name == "u16") {
    dispatch.kind = LogActualKind::Integer;
    dispatch.is_signed = false;
    dispatch.bits = 16u;
    return dispatch;
  }
  if (prim->name == "u32") {
    dispatch.kind = LogActualKind::Integer;
    dispatch.is_signed = false;
    dispatch.bits = 32u;
    return dispatch;
  }
  if (prim->name == "u64" || prim->name == "usize") {
    dispatch.kind = LogActualKind::Integer;
    dispatch.is_signed = false;
    dispatch.bits = 64u;
    return dispatch;
  }

  return dispatch;
}

std::string FormatActualValue(const IRValue& actual) {
  if (actual.kind == IRValue::Kind::Immediate && !actual.name.empty()) {
    return actual.name;
  }
  if (!actual.name.empty()) {
    return "$" + actual.name;
  }
  return "<value>";
}

analysis::TypeRef NormalizeLogCompareType(const analysis::TypeRef& type) {
  analysis::TypeRef cur = analysis::StripPerm(type);
  if (!cur) {
    cur = type;
  }
  while (cur) {
    if (const auto* refine = std::get_if<analysis::TypeRefine>(&cur->node)) {
      cur = analysis::StripPerm(refine->base);
      if (!cur) {
        cur = refine->base;
      }
      continue;
    }
    break;
  }
  return cur;
}

analysis::TypeRef ResolveAliasTypeForLogCompare(
    const analysis::TypeRef& type,
    const LowerCtx& ctx,
    std::size_t depth = 0) {
  analysis::TypeRef stripped = NormalizeLogCompareType(type);
  if (!stripped || depth > 16 || !ctx.sigma) {
    return stripped;
  }

  const auto* path = std::get_if<analysis::TypePathType>(&stripped->node);
  if (!path) {
    return stripped;
  }

  ast::Path syntax_path;
  syntax_path.reserve(path->path.size());
  for (const auto& seg : path->path) {
    syntax_path.push_back(seg);
  }
  auto it = ctx.sigma->types.find(analysis::PathKeyOf(syntax_path));
  if (it == ctx.sigma->types.end() &&
      path->path.size() == 1 &&
      ctx.resolve_type_name) {
    if (const auto resolved = ctx.resolve_type_name(path->path.front());
        resolved.has_value() && !resolved->empty()) {
      ast::Path resolved_path;
      resolved_path.reserve(resolved->size());
      for (const auto& seg : *resolved) {
        resolved_path.push_back(seg);
      }
      const auto resolved_it =
          ctx.sigma->types.find(analysis::PathKeyOf(resolved_path));
      if (resolved_it != ctx.sigma->types.end()) {
        syntax_path = std::move(resolved_path);
        it = resolved_it;
      }
    }
  }
  if (it == ctx.sigma->types.end()) {
    return stripped;
  }

  const auto* alias = std::get_if<ast::TypeAliasDecl>(&it->second);
  if (!alias) {
    return stripped;
  }

  const analysis::ScopeContext& scope = ScopeForLowering(ctx);
  const auto lowered = LowerTypeForLayout(scope, alias->type);
  if (!lowered.has_value()) {
    return stripped;
  }

  analysis::TypeRef inst = *lowered;
  if (alias->generic_params &&
      !alias->generic_params->params.empty() &&
      !path->generic_args.empty()) {
    analysis::TypeSubst subst =
        analysis::BuildSubstitution(alias->generic_params->params,
                                    path->generic_args);
    inst = analysis::InstantiateType(inst, subst);
  }

  return ResolveAliasTypeForLogCompare(inst, ctx, depth + 1);
}

// Build an IRValue immediate for the expected literal, matching the observed
// runtime value type as checked by semantic analysis.
std::optional<IRValue> BuildLogExpectedImmediate(
    const ast::Token& expected_token,
    const analysis::TypeRef& actual_type,
    const LowerCtx& ctx) {
  const analysis::TypeRef compare_type =
      ResolveAliasTypeForLogCompare(actual_type, ctx);
  if (!compare_type) {
    return std::nullopt;
  }

  if (auto encoded = EncodeConst(compare_type, expected_token)) {
    IRValue value;
    value.kind = IRValue::Kind::Immediate;
    value.name = expected_token.lexeme;
    value.bytes = std::move(*encoded);
    return value;
  }

  const LogActualDispatch dispatch = SelectLogActualDispatch(compare_type);
  switch (dispatch.kind) {
    case LogActualKind::StringView:
    case LogActualKind::StringManaged:
    case LogActualKind::BytesView:
    case LogActualKind::BytesManaged: {
      if (expected_token.kind != lexer::TokenKind::StringLiteral) {
        return std::nullopt;
      }
      auto decoded = DecodeStringLiteralBytes(expected_token.lexeme);
      if (!decoded.has_value()) {
        return std::nullopt;
      }
      IRValue value;
      value.kind = IRValue::Kind::Immediate;
      value.name = expected_token.lexeme;
      value.bytes = std::move(*decoded);
      return value;
    }
    default:
      return std::nullopt;
  }
}

std::string BuildLogPayloadPrefixImpl(
    const ast::AttributeList& attrs,
    const core::Span& span,
    std::string_view target_kind,
    std::optional<std::string_view> cmp_result) {
  (void)span;

  std::string payload;
  payload.reserve(128);
  const auto append_field = [&](std::string_view key, std::string_view value) {
    if (value.empty()) {
      return;
    }
    if (!payload.empty()) {
      payload += ";";
    }
    payload += key;
    payload += "=";
    payload += value;
  };

  append_field("category", "log");
  append_field("level", "info");

  const auto normalize_expected_display =
      [](std::string text, lexer::TokenKind kind) {
        if (kind == lexer::TokenKind::IntLiteral) {
          if (auto parsed = ParseIntLiteralLexeme(text)) {
            return std::to_string(*parsed);
          }
          return text;
        }
        if (kind != lexer::TokenKind::FloatLiteral) {
          return text;
        }

        text.erase(std::remove(text.begin(), text.end(), '_'), text.end());

        static constexpr std::string_view kFloatSuffixes[] = {
            "f16", "f32", "f64", "f"};
        for (const auto suffix : kFloatSuffixes) {
          if (text.size() >= suffix.size() &&
              text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0) {
            text.resize(text.size() - suffix.size());
            break;
          }
        }

        std::string exponent;
        const std::size_t exp_pos = text.find_first_of("eE");
        if (exp_pos != std::string::npos) {
          exponent = text.substr(exp_pos);
          text.resize(exp_pos);
        }

        const std::size_t dot = text.find('.');
        if (dot != std::string::npos) {
          std::string int_part = text.substr(0, dot);
          std::string frac_part = text.substr(dot + 1);
          while (!frac_part.empty() && frac_part.back() == '0') {
            frac_part.pop_back();
          }
          if (frac_part.empty()) {
            frac_part = "0";
          }
          text = int_part + "." + frac_part;
        }

        if (text.empty()) {
          text = "0";
        }

        return text + exponent;
      };

  const ast::AttributeItem* log_attr = FindLogAttr(attrs);
  if (log_attr) {
    if (const ast::Token* label = FindNamedTokenArg(*log_attr, "label")) {
      append_field("label", SanitizePayloadText(StripQuotedLiteral(label->lexeme)));
    } else if (!target_kind.empty()) {
      append_field("label", std::string(target_kind));
    }
    if (const ast::Token* expected = FindNamedTokenArg(*log_attr, "expected")) {
      if (expected->kind != lexer::TokenKind::Identifier) {
        append_field("expected",
                     SanitizePayloadText(
                         normalize_expected_display(expected->lexeme,
                                                    expected->kind)));
      }
    }
  } else if (!target_kind.empty()) {
    append_field("label", std::string(target_kind));
  }
  if (cmp_result.has_value()) {
    append_field("cmp", *cmp_result);
  }
  if (!payload.empty()) {
    payload += ";";
  }
  payload += "actual=";
  return payload;
}

}  // namespace

// =============================================================================
// Shared [[log]] attribute utilities (public API)
// =============================================================================

std::string BuildLogPayloadPrefix(
    const ast::AttributeList& attrs,
    const core::Span& span,
    std::string_view target_kind,
    std::optional<std::string_view> cmp_result) {
  return BuildLogPayloadPrefixImpl(attrs, span, target_kind, cmp_result);
}

std::string SanitizePayloadText(std::string text) {
  for (char& c : text) {
    if (c == '\t' || c == '\n' || c == '\r') {
      c = ' ';
    } else if (c == ';') {
      c = ':';
    }
  }
  return text;
}

std::string StripQuotedLiteral(std::string text) {
  if (text.size() >= 2 &&
      ((text.front() == '"' && text.back() == '"') ||
       (text.front() == '\'' && text.back() == '\''))) {
    return text.substr(1, text.size() - 2);
  }
  return text;
}

const ast::Token* FindNamedTokenArg(const ast::AttributeItem& attr,
                                    std::string_view key) {
  for (const auto& arg : attr.args) {
    if (!arg.key.has_value() || *arg.key != key) {
      continue;
    }
    return std::get_if<ast::Token>(&arg.value);
  }
  return nullptr;
}

const ast::AttributeItem* FindLogAttr(const ast::AttributeList& attrs) {
  for (const auto& attr : attrs) {
    if (attr.name == analysis::attrs::kLog) {
      return &attr;
    }
  }
  return nullptr;
}

std::optional<std::string> LogLabelFor(const ast::AttributeList& attrs) {
  const auto* attr = FindLogAttr(attrs);
  if (!attr) {
    return std::nullopt;
  }
  const ast::Token* tok = FindNamedTokenArg(*attr, "label");
  if (!tok) {
    return std::nullopt;
  }
  return StripQuotedLiteral(tok->lexeme);
}

std::optional<std::string> LogExpectedFor(const ast::AttributeList& attrs) {
  const auto* attr = FindLogAttr(attrs);
  if (!attr) {
    return std::nullopt;
  }
  const ast::Token* tok = FindNamedTokenArg(*attr, "expected");
  if (!tok) {
    return std::nullopt;
  }
  return tok->lexeme;
}

std::string BuildProcedureLogPayloadBase(
    std::string_view phase,
    std::string_view procedure_name,
    const std::optional<std::string>& label,
    const std::optional<std::string>& expected,
    std::optional<lexer::TokenKind> expected_token_kind,
    bool include_expected) {
  std::string payload;
  payload.reserve(128);
  const auto append_field = [&](std::string_view key, std::string_view value) {
    if (value.empty()) {
      return;
    }
    if (!payload.empty()) {
      payload += ";";
    }
    payload += key;
    payload += "=";
    payload += value;
  };
  append_field("category", "log");
  append_field("level", "info");

  std::string label_value;
  if (label.has_value()) {
    label_value = *label;
  } else {
    label_value = std::string(procedure_name);
    if (!phase.empty()) {
      if (!label_value.empty()) {
        label_value += ":";
      }
      label_value += std::string(phase);
    }
  }
  append_field("label", SanitizePayloadText(label_value));
  if (include_expected && expected.has_value()) {
    std::string expected_text = *expected;
    if (expected_token_kind.has_value()) {
      const auto normalize_expected_display =
          [](std::string text, lexer::TokenKind kind) {
            if (kind == lexer::TokenKind::IntLiteral) {
              if (auto parsed = ParseIntLiteralLexeme(text)) {
                return std::to_string(*parsed);
              }
              return text;
            }
            if (kind != lexer::TokenKind::FloatLiteral) {
              return text;
            }

            text.erase(std::remove(text.begin(), text.end(), '_'), text.end());

            static constexpr std::string_view kFloatSuffixes[] = {
                "f16", "f32", "f64", "f"};
            for (const auto suffix : kFloatSuffixes) {
              if (text.size() >= suffix.size() &&
                  text.compare(text.size() - suffix.size(),
                               suffix.size(),
                               suffix) == 0) {
                text.resize(text.size() - suffix.size());
                break;
              }
            }

            std::string exponent;
            const std::size_t exp_pos = text.find_first_of("eE");
            if (exp_pos != std::string::npos) {
              exponent = text.substr(exp_pos);
              text.resize(exp_pos);
            }

            const std::size_t dot = text.find('.');
            if (dot != std::string::npos) {
              std::string int_part = text.substr(0, dot);
              std::string frac_part = text.substr(dot + 1);
              while (!frac_part.empty() && frac_part.back() == '0') {
                frac_part.pop_back();
              }
              if (frac_part.empty()) {
                frac_part = "0";
              }
              text = int_part + "." + frac_part;
            }

            if (text.empty()) {
              text = "0";
            }

            return text + exponent;
          };
      expected_text =
          normalize_expected_display(std::move(expected_text), *expected_token_kind);
    }
    append_field("expected", SanitizePayloadText(expected_text));
  }
  return payload;
}

IRPtr EmitRuntimeTraceRecord(std::string_view rule_id,
                             std::string_view payload,
                             const std::optional<core::Span>& span) {
  return EmitRuntimeTraceInternal(rule_id, span, payload);
}

IRPtr EmitRuntimeTraceWithActual(std::string_view rule_id,
                                 std::string payload_prefix,
                                 const IRValue& actual,
                                 const analysis::TypeRef& actual_type,
                                 const std::optional<core::Span>& span,
                                 const LowerCtx* lower_ctx) {
  if (rule_id.empty()) {
    return EmptyIR();
  }

  analysis::TypeRef dispatch_type = actual_type;
  if (!dispatch_type && lower_ctx) {
    dispatch_type = lower_ctx->LookupValueType(actual);
  }
  if (!dispatch_type && lower_ctx && actual.kind == IRValue::Kind::Local) {
    if (const BindingState* state = lower_ctx->GetBindingState(actual.name)) {
      dispatch_type = state->type;
    }
  }
  if (dispatch_type && lower_ctx) {
    dispatch_type = ResolveAliasTypeForLogCompare(dispatch_type, *lower_ctx);
  }

  const LogActualDispatch dispatch = SelectLogActualDispatch(dispatch_type);
  if (dispatch.kind == LogActualKind::Unsupported) {
    std::string payload = std::move(payload_prefix);
    payload += SanitizePayloadText(FormatActualValue(actual));
    return EmitRuntimeTraceInternal(rule_id, span, payload);
  }

  // Preserve immediate float lexemes directly in trace payloads. This keeps
  // `[[log]]` output aligned with source-level float values without requiring
  // ABI coercion through the runtime helper path.
  if (dispatch.kind == LogActualKind::Floating &&
      actual.kind == IRValue::Kind::Immediate && !actual.name.empty()) {
    std::string payload = std::move(payload_prefix);
    payload += SanitizePayloadText(actual.name);
    return EmitRuntimeTraceInternal(rule_id, span, payload);
  }

  IRCall call;
  call.callee.kind = IRValue::Kind::Symbol;
  call.callee.name = RuntimeConformanceEmitSym();
  call.args.push_back(StringImmediate(rule_id));
  AppendRuntimeTraceSpanArgs(call, span);
  call.args.push_back(StringImmediate(payload_prefix));

  switch (dispatch.kind) {
    case LogActualKind::Integer:
      call.callee.name = RuntimeConformanceEmitIntSym();
      call.args.push_back(actual);
      call.args.push_back(U8ConstValue(dispatch.bits));
      call.args.push_back(BoolConstValue(dispatch.is_signed));
      return MakeIR(std::move(call));
    case LogActualKind::Boolean:
      call.callee.name = RuntimeConformanceEmitBoolSym();
      call.args.push_back(actual);
      return MakeIR(std::move(call));
    case LogActualKind::Floating:
      call.callee.name = RuntimeConformanceEmitFloatSym();
      call.args.push_back(actual);
      call.args.push_back(U8ConstValue(dispatch.bits));
      return MakeIR(std::move(call));
    case LogActualKind::Pointer:
      call.callee.name = RuntimeConformanceEmitPtrSym();
      call.args.push_back(actual);
      return MakeIR(std::move(call));
    case LogActualKind::StringView:
      call.callee.name = RuntimeConformanceEmitStringSym();
      call.args.push_back(actual);
      return MakeIR(std::move(call));
    case LogActualKind::StringManaged:
      call.callee.name = RuntimeConformanceEmitStringManagedSym();
      call.args.push_back(actual);
      return MakeIR(std::move(call));
    case LogActualKind::BytesView:
      call.callee.name = RuntimeConformanceEmitBytesSym();
      call.args.push_back(actual);
      return MakeIR(std::move(call));
    case LogActualKind::BytesManaged:
      call.callee.name = RuntimeConformanceEmitBytesManagedSym();
      call.args.push_back(actual);
      return MakeIR(std::move(call));
    case LogActualKind::Unsupported:
      break;
  }

  std::string payload = std::move(payload_prefix);
  payload += SanitizePayloadText(FormatActualValue(actual));
  return EmitRuntimeTraceInternal(rule_id, span, payload);
}

IRPtr EmitRuntimeTraceUnitActual(std::string_view rule_id,
                                 std::string payload_prefix,
                                 const std::optional<core::Span>& span) {
  payload_prefix += "()";
  return EmitRuntimeTraceInternal(rule_id, span, payload_prefix);
}

IRValue USizeConstValue(std::uint64_t value) {
  IRValue v;
  v.kind = IRValue::Kind::Immediate;
  v.name = std::to_string(value);
  v.bytes = LEBytesU64(value);
  return v;
}

IRPtr EmitRuntimeScopeEnter(std::uint64_t scope_id, LowerCtx& ctx) {
  IRCall call;
  call.callee.kind = IRValue::Kind::Symbol;
  call.callee.name = BuiltinModalSymRegionScopeEnter();
  call.args.push_back(USizeConstValue(scope_id));
  IRValue result = ctx.FreshTempValue("scope_enter");
  call.result = result;
  ctx.RegisterValueType(result, analysis::MakeTypePrim("()"));
  return MakeIR(std::move(call));
}

// =============================================================================
// Â§6.4 Literal Parsing Utilities
// =============================================================================

std::string StripIntSuffix(const std::string& text) {
  static const char* suffixes[] = {
      "isize", "usize", "i128", "u128", "i64", "u64",
      "i32", "u32", "i16", "u16", "i8", "u8"
  };
  for (const char* suf : suffixes) {
    const std::string_view sv{suf};
    if (text.size() >= sv.size() &&
        text.compare(text.size() - sv.size(), sv.size(), sv) == 0) {
      return text.substr(0, text.size() - sv.size());
    }
  }
  return text;
}

std::optional<std::uint64_t> ParseIntLiteralLexeme(const std::string& lexeme) {
  std::string text = StripIntSuffix(lexeme);

  // Binary literal: 0b or 0B prefix
  if (text.rfind("0b", 0) == 0 || text.rfind("0B", 0) == 0) {
    text.erase(0, 2);
    if (text.empty()) {
      return std::nullopt;
    }
    std::uint64_t out = 0;
    for (char c : text) {
      if (c == '_') {
        continue;
      }
      if (c != '0' && c != '1') {
        return std::nullopt;
      }
      out = (out << 1) | (c == '1');
    }
    return out;
  }

  // Octal literal: 0o or 0O prefix
  if (text.rfind("0o", 0) == 0 || text.rfind("0O", 0) == 0) {
    text.erase(0, 2);
    if (text.empty()) {
      return std::nullopt;
    }
    std::uint64_t out = 0;
    for (char c : text) {
      if (c == '_') {
        continue;
      }
      if (c < '0' || c > '7') {
        return std::nullopt;
      }
      out = (out << 3) | static_cast<std::uint64_t>(c - '0');
    }
    return out;
  }

  // Decimal or hex (stoull handles 0x prefix)
  try {
    std::size_t idx = 0;
    std::uint64_t out = std::stoull(text, &idx, 0);
    if (idx != text.size()) {
      return std::nullopt;
    }
    return out;
  } catch (...) {
    return std::nullopt;
  }
}

std::vector<std::uint8_t> EncodeU64LE(std::uint64_t value) {
  std::vector<std::uint8_t> bytes;
  if (value == 0) {
    bytes.push_back(0);
    return bytes;
  }
  while (value > 0) {
    bytes.push_back(static_cast<std::uint8_t>(value & 0xFF));
    value >>= 8;
  }
  return bytes;
}

// =============================================================================
// Â§6.4 Expression Classification
// =============================================================================

bool IsPlaceExpr(const ast::Expr& expr) {
  return std::visit(
      [&](const auto& node) -> bool {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, ast::IdentifierExpr>) {
          return true;
        } else if constexpr (std::is_same_v<T, ast::FieldAccessExpr>) {
          return true;
        } else if constexpr (std::is_same_v<T, ast::TupleAccessExpr>) {
          return true;
        } else if constexpr (std::is_same_v<T, ast::AttributedExpr>) {
          return node.expr ? IsPlaceExpr(*node.expr) : false;
        } else if constexpr (std::is_same_v<T, ast::IndexAccessExpr>) {
          return true;
        } else if constexpr (std::is_same_v<T, ast::DerefExpr>) {
          return true;
        }
        return false;
      },
      expr.node);
}

bool IsTempValueExpr(const ast::Expr& expr) {
  return !IsPlaceExpr(expr);
}

bool NeedsIndexCheck(const ast::Expr& base, const LowerCtx& ctx) {
  if (!ctx.expr_type) {
    return true;
  }
  analysis::TypeRef base_type = ctx.expr_type(base);
  analysis::TypeRef stripped = analysis::StripPerm(base_type);
  if (stripped && std::holds_alternative<analysis::TypeArray>(stripped->node)) {
    // Static arrays only need runtime checks if [[dynamic]] is enabled
    return ctx.dynamic_checks;
  }
  // Slices always need runtime checks
  return true;
}

bool IsRangeIndexExpr(const ast::Expr& expr, const LowerCtx& ctx) {
  if (std::holds_alternative<ast::RangeExpr>(expr.node)) {
    return true;
  }
  if (!ctx.expr_type) {
    return false;
  }
  const analysis::TypeRef index_type = ctx.expr_type(expr);
  return analysis::IsRangeType(index_type) &&
         analysis::IsRangeIndexType(index_type);
}

std::optional<ast::RangeKind> RangeIndexKindOf(const ast::Expr& expr,
                                               const LowerCtx& ctx) {
  if (const auto* range = std::get_if<ast::RangeExpr>(&expr.node)) {
    return range->kind;
  }
  if (!ctx.expr_type) {
    return std::nullopt;
  }

  analysis::TypeRef index_type = ctx.expr_type(expr);
  while (index_type) {
    if (const auto* perm = std::get_if<analysis::TypePerm>(&index_type->node)) {
      index_type = perm->base;
      continue;
    }
    if (const auto* refine =
            std::get_if<analysis::TypeRefine>(&index_type->node)) {
      index_type = refine->base;
      continue;
    }
    break;
  }

  if (!index_type) {
    return std::nullopt;
  }
  if (std::holds_alternative<analysis::TypeRange>(index_type->node)) {
    return ast::RangeKind::Exclusive;
  }
  if (std::holds_alternative<analysis::TypeRangeInclusive>(index_type->node)) {
    return ast::RangeKind::Inclusive;
  }
  if (std::holds_alternative<analysis::TypeRangeFrom>(index_type->node)) {
    return ast::RangeKind::From;
  }
  if (std::holds_alternative<analysis::TypeRangeTo>(index_type->node)) {
    return ast::RangeKind::To;
  }
  if (std::holds_alternative<analysis::TypeRangeToInclusive>(
          index_type->node)) {
    return ast::RangeKind::ToInclusive;
  }
  if (std::holds_alternative<analysis::TypeRangeFull>(index_type->node)) {
    return ast::RangeKind::Full;
  }
  return std::nullopt;
}

bool IsMoveExpr(const ast::ExprPtr& expr) {
  return expr && std::holds_alternative<ast::MoveExpr>(expr->node);
}

// =============================================================================
// Â§6.4 Place Analysis
// =============================================================================

std::optional<std::string> PlaceRoot(const ast::Expr& expr) {
  return std::visit(
      [&](const auto& node) -> std::optional<std::string> {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, ast::AttributedExpr>) {
          return node.expr ? PlaceRoot(*node.expr) : std::nullopt;
        } else if constexpr (std::is_same_v<T, ast::IdentifierExpr>) {
          return node.name;
        } else if constexpr (std::is_same_v<T, ast::FieldAccessExpr>) {
          return PlaceRoot(*node.base);
        } else if constexpr (std::is_same_v<T, ast::TupleAccessExpr>) {
          return PlaceRoot(*node.base);
        } else if constexpr (std::is_same_v<T, ast::IndexAccessExpr>) {
          return PlaceRoot(*node.base);
        } else if constexpr (std::is_same_v<T, ast::DerefExpr>) {
          return PlaceRoot(*node.value);
        }
        return std::nullopt;
      },
      expr.node);
}

std::optional<std::string> FieldHead(const ast::Expr& expr) {
  return std::visit(
      [&](const auto& node) -> std::optional<std::string> {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, ast::AttributedExpr>) {
          return node.expr ? FieldHead(*node.expr) : std::nullopt;
        } else if constexpr (std::is_same_v<T, ast::IdentifierExpr>) {
          return std::nullopt;
        } else if constexpr (std::is_same_v<T, ast::FieldAccessExpr>) {
          auto head = FieldHead(*node.base);
          if (head.has_value()) {
            return head;
          }
          return node.name;
        } else if constexpr (std::is_same_v<T, ast::TupleAccessExpr>) {
          return FieldHead(*node.base);
        } else if constexpr (std::is_same_v<T, ast::IndexAccessExpr>) {
          return FieldHead(*node.base);
        } else if constexpr (std::is_same_v<T, ast::DerefExpr>) {
          return std::nullopt;
        }
        return std::nullopt;
      },
      expr.node);
}

namespace {

std::string FormatRangeBound(const ast::ExprPtr& expr) {
  if (!expr) {
    return "";
  }
  if (const auto* attr = std::get_if<ast::AttributedExpr>(&expr->node)) {
    if (attr->expr) {
      return FormatRangeBound(attr->expr);
    }
  }
  if (const auto* lit = std::get_if<ast::LiteralExpr>(&expr->node)) {
    return lit->literal.lexeme;
  }
  return "?";
}

}  // namespace

std::string BuildPlaceRepr(const ast::Expr& expr) {
  return std::visit(
      [&](const auto& node) -> std::string {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, ast::AttributedExpr>) {
          return node.expr ? BuildPlaceRepr(*node.expr) : "";
        } else if constexpr (std::is_same_v<T, ast::IdentifierExpr>) {
          return node.name;
        } else if constexpr (std::is_same_v<T, ast::FieldAccessExpr>) {
          std::string base = BuildPlaceRepr(*node.base);
          if (base.empty()) {
            return node.name;
          }
          return base + "." + node.name;
        } else if constexpr (std::is_same_v<T, ast::TupleAccessExpr>) {
          std::string base = BuildPlaceRepr(*node.base);
          std::string idx = node.index.lexeme;
          if (base.empty()) {
            return idx;
          }
          return base + "." + idx;
        } else if constexpr (std::is_same_v<T, ast::IndexAccessExpr>) {
          std::string base = BuildPlaceRepr(*node.base);
          std::string idx = FormatIndexExpr(*node.index);
          if (base.empty()) {
            return "[" + idx + "]";
          }
          return base + "[" + idx + "]";
        } else if constexpr (std::is_same_v<T, ast::DerefExpr>) {
          return "*" + BuildPlaceRepr(*node.value);
        }
        return "";
      },
      expr.node);
}

// =============================================================================
// Â§6.4 Attribute Handling
// =============================================================================

[[maybe_unused]] bool HasDynamicAttr(const ast::AttributeList& attrs) {
  for (const auto& attr : attrs) {
    if (attr.name == analysis::attrs::kDynamic) {
      return true;
    }
  }
  return false;
}

bool HasLogAttr(const ast::AttributeList& attrs) {
  return FindLogAttr(attrs) != nullptr;
}

// Helper: emit comparison IR and branching trace for a given expected IRValue.
static IRPtr EmitLogCmpBranch(const IRValue& actual,
                              const analysis::TypeRef& actual_type,
                              const IRValue& expected_val,
                              const std::string& payload_pass,
                              const std::string& payload_fail,
                              std::string_view rule_id,
                              LowerCtx& ctx,
                              const std::optional<core::Span>& span) {
  IRBinaryOp cmp;
  cmp.op = "==";
  cmp.lhs = actual;
  cmp.rhs = expected_val;
  cmp.result = ctx.FreshTempValue("log_cmp");
  ctx.RegisterValueType(cmp.result, analysis::MakeTypePrim("bool"));

  IRValue unit_val;
  unit_val.kind = IRValue::Kind::Immediate;
  unit_val.name = "unit";

  IRIf if_ir;
  if_ir.cond = cmp.result;
  if_ir.then_ir = EmitRuntimeTraceWithActual(
      rule_id, payload_pass, actual, actual_type, span, &ctx);
  if_ir.then_value = unit_val;
  if_ir.else_ir = EmitRuntimeTraceWithActual(
      rule_id, payload_fail, actual, actual_type, span, &ctx);
  if_ir.else_value = unit_val;
  if_ir.result = ctx.FreshTempValue("log_cmp_done");
  ctx.RegisterValueType(if_ir.result, analysis::MakeTypePrim("()"));

  return SeqIR({MakeIR(std::move(cmp)), MakeIR(std::move(if_ir))});
}

[[maybe_unused]] IRPtr EmitLogAttributeTrace(const ast::AttributeList& attrs,
                                             const core::Span& span,
                                             const IRValue& actual,
                                             const analysis::TypeRef& actual_type,
                                             std::string_view target_kind,
                                             LowerCtx& ctx) {
  if (!HasLogAttr(attrs)) {
    return EmptyIR();
  }

  const auto* log_attr = FindLogAttr(attrs);
  const ast::Token* expected_token =
      log_attr ? FindNamedTokenArg(*log_attr, "expected") : nullptr;

  if (!expected_token) {
    const auto payload_prefix = BuildLogPayloadPrefix(attrs, span, target_kind);
    return EmitRuntimeTraceWithActual("Log-Expr", payload_prefix, actual,
                                      actual_type, span, &ctx);
  }
  analysis::TypeRef observed_type = actual_type;
  if (!observed_type) {
    observed_type = ctx.LookupValueType(actual);
  }
  if (!observed_type && actual.kind == IRValue::Kind::Local) {
    if (const BindingState* state = ctx.GetBindingState(actual.name)) {
      observed_type = state->type;
    }
  }
  if (!observed_type) {
    return EmitRuntimeTraceWithActual(
        "Log-Expr", BuildLogPayloadPrefix(attrs, span, target_kind, "unsupported"),
        actual, observed_type, span, &ctx);
  }

  if (expected_token->kind == lexer::TokenKind::Identifier) {
    const std::string& var_name = expected_token->lexeme;
    const BindingState* binding = ctx.GetBindingState(var_name);
    if (!binding) {
      return EmitRuntimeTraceWithActual(
          "Log-Expr", BuildLogPayloadPrefix(attrs, span, target_kind, "unsupported"),
          actual, observed_type, span, &ctx);
    }

    IRReadVar read;
    read.name = var_name;
    IRValue expected_val;
    expected_val.kind = IRValue::Kind::Local;
    expected_val.name = var_name;
    if (binding->type) {
      ctx.RegisterValueType(expected_val, binding->type);
    }

    auto payload_pass = BuildLogPayloadPrefix(attrs, span, target_kind, "pass");
    auto payload_fail = BuildLogPayloadPrefix(attrs, span, target_kind, "fail");

    return SeqIR({MakeIR(std::move(read)),
                  EmitLogCmpBranch(actual, observed_type, expected_val,
                                   payload_pass, payload_fail, "Log-Expr", ctx,
                                   span)});
  }

  auto expected_imm = BuildLogExpectedImmediate(*expected_token, observed_type, ctx);
  if (!expected_imm.has_value()) {
    return EmitRuntimeTraceWithActual(
        "Log-Expr", BuildLogPayloadPrefix(attrs, span, target_kind, "unsupported"),
        actual, observed_type, span, &ctx);
  }
  expected_imm->literal_id = ++ctx.temp_counter;
  ctx.RegisterValueType(*expected_imm, observed_type);

  auto payload_pass = BuildLogPayloadPrefix(attrs, span, target_kind, "pass");
  auto payload_fail = BuildLogPayloadPrefix(attrs, span, target_kind, "fail");

  return EmitLogCmpBranch(actual, observed_type, *expected_imm, payload_pass,
                          payload_fail, "Log-Expr", ctx, span);
}

IRPtr EmitProcLogTraceWithCmp(std::string_view rule_id,
                              std::string payload_base,
                              const std::optional<std::string>& expected_lexeme,
                              std::optional<ast::TokenKind> expected_token_kind,
                              const IRValue& actual,
                              const analysis::TypeRef& actual_type,
                              LowerCtx& ctx,
                              const std::optional<core::Span>& span) {
  if (!expected_lexeme.has_value()) {
    payload_base += ";actual=";
    return EmitRuntimeTraceWithActual(rule_id, std::move(payload_base), actual,
                                      actual_type, span, &ctx);
  }
  analysis::TypeRef observed_type = actual_type;
  if (!observed_type) {
    observed_type = ctx.LookupValueType(actual);
  }
  if (!observed_type && actual.kind == IRValue::Kind::Local) {
    if (const BindingState* state = ctx.GetBindingState(actual.name)) {
      observed_type = state->type;
    }
  }
  if (!observed_type) {
    payload_base += ";cmp=unsupported;actual=";
    return EmitRuntimeTraceWithActual(rule_id, std::move(payload_base), actual,
                                      observed_type, span, &ctx);
  }

  if (ctx.proc_log_expected_is_ident) {
    const std::string& var_name = *expected_lexeme;
    const BindingState* binding = ctx.GetBindingState(var_name);
    if (!binding) {
      payload_base += ";cmp=unsupported;actual=";
      return EmitRuntimeTraceWithActual(rule_id, std::move(payload_base), actual,
                                        observed_type, span, &ctx);
    }

    IRReadVar read;
    read.name = var_name;
    IRValue expected_val;
    expected_val.kind = IRValue::Kind::Local;
    expected_val.name = var_name;
    if (binding->type) {
      ctx.RegisterValueType(expected_val, binding->type);
    }

    std::string payload_pass = payload_base + ";cmp=pass;actual=";
    std::string payload_fail = payload_base + ";cmp=fail;actual=";

    return SeqIR({MakeIR(std::move(read)),
                  EmitLogCmpBranch(actual, observed_type, expected_val,
                                   payload_pass, payload_fail, rule_id, ctx,
                                   span)});
  }

  if (!expected_token_kind.has_value()) {
    payload_base += ";cmp=unsupported;actual=";
    return EmitRuntimeTraceWithActual(rule_id, std::move(payload_base), actual,
                                      observed_type, span, &ctx);
  }
  ast::Token expected_token;
  expected_token.kind = *expected_token_kind;
  expected_token.lexeme = *expected_lexeme;
  auto expected_imm = BuildLogExpectedImmediate(expected_token, observed_type, ctx);
  if (!expected_imm.has_value()) {
    payload_base += ";cmp=unsupported;actual=";
    return EmitRuntimeTraceWithActual(rule_id, std::move(payload_base), actual,
                                      observed_type, span, &ctx);
  }
  expected_imm->literal_id = ++ctx.temp_counter;
  ctx.RegisterValueType(*expected_imm, observed_type);

  std::string payload_pass = payload_base + ";cmp=pass;actual=";
  std::string payload_fail = payload_base + ";cmp=fail;actual=";

  return EmitLogCmpBranch(actual, observed_type, *expected_imm, payload_pass,
                          payload_fail, rule_id, ctx, span);
}

const ast::Expr* UnwrapAttributed(const ast::Expr& expr) {
  if (const auto* attr = std::get_if<ast::AttributedExpr>(&expr.node)) {
    if (attr->expr) {
      return UnwrapAttributed(*attr->expr);
    }
    return nullptr;
  }
  return &expr;
}

// =============================================================================
// Â§6.4 Range/Index Formatting
// =============================================================================

std::string FormatRangeExpr(const ast::RangeExpr& expr) {
  const std::string lo = FormatRangeBound(expr.lhs);
  const std::string hi = FormatRangeBound(expr.rhs);
  switch (expr.kind) {
    case ast::RangeKind::Full:
      return "..";
    case ast::RangeKind::From:
      return lo + "..";
    case ast::RangeKind::To:
      return ".." + hi;
    case ast::RangeKind::ToInclusive:
      return "..=" + hi;
    case ast::RangeKind::Exclusive:
      return lo + ".." + hi;
    case ast::RangeKind::Inclusive:
      return lo + "..=" + hi;
  }
  return "..";
}

std::string FormatIndexExpr(const ast::Expr& expr) {
  if (const auto* attr = std::get_if<ast::AttributedExpr>(&expr.node)) {
    if (attr->expr) {
      return FormatIndexExpr(*attr->expr);
    }
  }
  if (const auto* lit = std::get_if<ast::LiteralExpr>(&expr.node)) {
    return lit->literal.lexeme;
  }
  if (const auto* range = std::get_if<ast::RangeExpr>(&expr.node)) {
    return FormatRangeExpr(*range);
  }
  return "?";
}

// =============================================================================
// Â§6.4 Argument/Field Expression Extraction
// =============================================================================

std::vector<ast::ExprPtr> ArgsExprs(const std::vector<ast::Arg>& args) {
  if (args.empty()) {
    SPEC_RULE("ArgsExprs-Empty");
    return {};
  }
  SPEC_RULE("ArgsExprs-Cons");
  std::vector<ast::ExprPtr> result;
  result.reserve(args.size());
  for (const auto& arg : args) {
    result.push_back(arg.value);
  }
  return result;
}

std::vector<ast::ExprPtr> FieldExprs(const std::vector<ast::FieldInit>& fields) {
  if (fields.empty()) {
    SPEC_RULE("FieldExprs-Empty");
    return {};
  }
  SPEC_RULE("FieldExprs-Cons");
  std::vector<ast::ExprPtr> result;
  result.reserve(fields.size());
  for (const auto& field : fields) {
    result.push_back(field.value);
  }
  return result;
}

// =============================================================================
// Â§6.4 Parallel/Dispatch Expression Detection
// =============================================================================

bool DispatchHasReduce(const ast::DispatchExpr& expr) {
  for (const auto& opt : expr.opts) {
    if (opt.kind == ast::DispatchOptionKind::Reduce) {
      return true;
    }
  }
  return false;
}

bool IsCollectableParallelExpr(const ast::Expr& expr, bool& needs_wait) {
  if (std::holds_alternative<ast::SpawnExpr>(expr.node)) {
    needs_wait = true;
    return true;
  }
  if (const auto* dispatch = std::get_if<ast::DispatchExpr>(&expr.node)) {
    if (DispatchHasReduce(*dispatch)) {
      needs_wait = false;
      return true;
    }
  }
  return false;
}

// =============================================================================
// Â§6.4 Block Expression Utilities
// =============================================================================

ast::ExprPtr WrapBlockExpr(const std::shared_ptr<ast::Block>& block) {
  if (!block) {
    return nullptr;
  }
  auto expr = std::make_shared<ast::Expr>();
  expr->span = block->span;
  expr->node = ast::BlockExpr{block};
  return expr;
}

// =============================================================================
// Â§6.4 Binding State Update
// =============================================================================

void UpdateBindingAfterFieldAssign(const ast::Expr& place, LowerCtx& ctx) {
  auto root = PlaceRoot(place);
  auto head = FieldHead(place);
  if (!root.has_value() || !head.has_value()) {
    return;
  }
  auto it = ctx.binding_states.find(*root);
  if (it == ctx.binding_states.end() || it->second.empty()) {
    return;
  }
  auto& state = it->second.back();
  if (state.is_moved) {
    return;
  }
  auto& fields = state.moved_fields;
  fields.erase(std::remove(fields.begin(), fields.end(), *head), fields.end());
}

// =============================================================================
// Anchor function for SPEC_RULE markers
// =============================================================================

void AnchorExprCommonRules() {
  // Literal parsing
  SPEC_RULE("LiteralValue-Int");
  SPEC_RULE("LiteralValue-Float");
  SPEC_RULE("LiteralValue-Bool");
  SPEC_RULE("LiteralValue-Char");
  SPEC_RULE("LiteralValue-String");
  SPEC_RULE("LiteralValue-Null");

  // Expression classification
  SPEC_RULE("IsPlace-Ident");
  SPEC_RULE("IsPlace-Field");
  SPEC_RULE("IsPlace-Tuple");
  SPEC_RULE("IsPlace-Index");
  SPEC_RULE("IsPlace-Deref");

  // Place analysis
  SPEC_RULE("PlaceRoot-Ident");
  SPEC_RULE("PlaceRoot-Field");
  SPEC_RULE("PlaceRoot-Tuple");
  SPEC_RULE("PlaceRoot-Index");

  // Argument extraction
  SPEC_RULE("ArgsExprs-Empty");
  SPEC_RULE("ArgsExprs-Cons");
  SPEC_RULE("FieldExprs-Empty");
  SPEC_RULE("FieldExprs-Cons");
}

// ============================================================================
// Â§6.8 LowerCtx Scope Tracking Methods
// ============================================================================

void LowerCtx::PushScope(bool is_loop, bool is_region) {
  ScopeInfo scope;
  scope.is_loop = is_loop;
  scope.is_region = is_region;
  scope.runtime_scope_id = next_runtime_scope_id++;
  scope_stack.push_back(std::move(scope));
}

void LowerCtx::PopScope() {
  if (scope_stack.empty()) {
    return;
  }

  const auto& vars = scope_stack.back().variables;
  for (auto it = vars.rbegin(); it != vars.rend(); ++it) {
    auto map_it = binding_states.find(*it);
    if (map_it == binding_states.end()) {
      continue;
    }
    if (!map_it->second.empty()) {
      map_it->second.pop_back();
    }
    if (map_it->second.empty()) {
      binding_states.erase(map_it);
    }
  }

  scope_stack.pop_back();
}

std::vector<std::string> LowerCtx::CurrentScopeVars() const {
  if (scope_stack.empty()) {
    return {};
  }
  return scope_stack.back().variables;
}

std::vector<std::string> LowerCtx::VarsToLoopScope() const {
  std::vector<std::string> vars;

  // Collect variables from innermost scope outward until we hit a loop scope
  for (auto it = scope_stack.rbegin(); it != scope_stack.rend(); ++it) {
    for (const auto& var : it->variables) {
      vars.push_back(var);
    }
    if (it->is_loop) {
      break;
    }
  }

  return vars;
}

std::vector<std::string> LowerCtx::VarsToFunctionRoot() const {
  std::vector<std::string> vars;

  // Collect all variables from all scopes
  for (auto it = scope_stack.rbegin(); it != scope_stack.rend(); ++it) {
    for (const auto& var : it->variables) {
      vars.push_back(var);
    }
  }

  return vars;
}

void LowerCtx::RegisterVar(const std::string& name,
                            analysis::TypeRef type,
                            bool has_responsibility,
                            bool is_immovable,
                            analysis::ProvenanceKind prov,
                            std::optional<std::string> prov_region) {
  if (!scope_stack.empty()) {
    scope_stack.back().variables.push_back(name);
    if (has_responsibility) {
      CleanupItem item;
      item.kind = CleanupItem::Kind::DropBinding;
      item.name = name;
      scope_stack.back().cleanup_items.push_back(std::move(item));
    }
  }

  BindingState state;
  state.type = type;
  state.has_responsibility = has_responsibility;
  state.is_immovable = is_immovable;
  state.is_moved = false;
  state.prov = prov;
  state.prov_region = std::move(prov_region);
  if (!scope_stack.empty()) {
    state.scope_runtime_id = scope_stack.back().runtime_scope_id;
  }

  binding_states[name].push_back(std::move(state));
}

void LowerCtx::RegisterRuntimeScopeExit() {
  if (scope_stack.empty()) {
    return;
  }
  CleanupItem item;
  item.kind = CleanupItem::Kind::RuntimeScopeExit;
  item.scope_runtime_id = scope_stack.back().runtime_scope_id;
  scope_stack.back().cleanup_items.push_back(std::move(item));
}

std::optional<std::uint64_t> LowerCtx::CurrentRuntimeScopeId() const {
  if (scope_stack.empty()) {
    return std::nullopt;
  }
  return scope_stack.back().runtime_scope_id;
}

void LowerCtx::MarkMoved(const std::string& name) {
  auto it = binding_states.find(name);
  if (it == binding_states.end() || it->second.empty()) {
    SPEC_RULE("UpdateValid-Err");
    ReportCodegenFailure();
    return;
  }
  SPEC_RULE("UpdateValid-MoveRoot");
  it->second.back().is_moved = true;
}

void LowerCtx::MarkFieldMoved(const std::string& name, const std::string& field) {
  auto it = binding_states.find(name);
  if (it == binding_states.end() || it->second.empty()) {
    SPEC_RULE("UpdateValid-Err");
    ReportCodegenFailure();
    return;
  }
  SPEC_RULE("UpdateValid-PartialMove-Init");
  SPEC_RULE("UpdateValid-PartialMove-Step");
  it->second.back().moved_fields.push_back(field);
}

const BindingState* LowerCtx::GetBindingState(const std::string& name) const {
  auto it = binding_states.find(name);
  if (it != binding_states.end() && !it->second.empty()) {
    return &it->second.back();
  }
  return nullptr;
}

std::optional<analysis::ProvenanceKind> LowerCtx::LookupExprProv(
    const ast::Expr& expr) const {
  if (!expr_prov) {
    return std::nullopt;
  }
  const auto it = expr_prov->find(&expr);
  if (it == expr_prov->end()) {
    return std::nullopt;
  }
  return it->second;
}

std::optional<std::string> LowerCtx::LookupExprRegion(
    const ast::Expr& expr) const {
  if (!expr_region) {
    return std::nullopt;
  }
  const auto it = expr_region->find(&expr);
  if (it == expr_region->end()) {
    return std::nullopt;
  }
  return it->second;
}

const std::vector<analysis::TypeRef>* LowerCtx::LookupDynamicRefinementTypes(
    const ast::Expr& expr) const {
  if (!dynamic_refine_checks) {
    return nullptr;
  }
  const auto it = dynamic_refine_checks->find(&expr);
  if (it == dynamic_refine_checks->end()) {
    return nullptr;
  }
  return &it->second;
}

void LowerCtx::RegisterDefer(const IRPtr& defer_ir) {
  if (!scope_stack.empty()) {
    CleanupItem item;
    item.kind = CleanupItem::Kind::DeferBlock;
    item.defer_ir = defer_ir;
    scope_stack.back().cleanup_items.push_back(std::move(item));
  }
}

void LowerCtx::RegisterRegionRelease(const std::string& name) {
  if (!scope_stack.empty()) {
    CleanupItem item;
    item.kind = CleanupItem::Kind::ReleaseRegion;
    item.name = name;
    scope_stack.back().cleanup_items.push_back(std::move(item));
  }
}

void LowerCtx::RegisterKeyScopeExit(const std::string& scope_name) {
  if (!scope_stack.empty()) {
    CleanupItem item;
    item.kind = CleanupItem::Kind::ReleaseKeyScope;
    item.name = scope_name;
    scope_stack.back().cleanup_items.push_back(std::move(item));
  }
}

void LowerCtx::RegisterTempValue(const IRValue& value, const analysis::TypeRef& type) {
  if (!temp_sink) {
    return;
  }
  TempValue temp;
  temp.value = value;
  temp.type = type;
  temp_sink->push_back(std::move(temp));
}

void LowerCtx::RegisterDerivedValue(const IRValue& value, const DerivedValueInfo& info) {
  auto derived_key = [](const IRValue& v) -> std::string {
    if (v.kind == IRValue::Kind::Opaque) {
      return "o:" + v.name;
    }
    if (v.kind == IRValue::Kind::Local) {
      return "l:" + v.name;
    }
    return "";
  };
  const std::string key = derived_key(value);
  if (key.empty()) {
    return;
  }
  derived_values[key] = info;
}

const DerivedValueInfo* LowerCtx::LookupDerivedValue(const IRValue& value) const {
  auto derived_key = [](const IRValue& v) -> std::string {
    if (v.kind == IRValue::Kind::Opaque) {
      return "o:" + v.name;
    }
    if (v.kind == IRValue::Kind::Local) {
      return "l:" + v.name;
    }
    return "";
  };
  const std::string key = derived_key(value);
  if (key.empty()) {
    return nullptr;
  }
  auto it = derived_values.find(key);
  if (it == derived_values.end()) {
    if (map_parent != nullptr) {
      return map_parent->LookupDerivedValue(value);
    }
    return nullptr;
  }
  return &it->second;
}

const CaptureAccess* LowerCtx::LookupCapture(const std::string& name) const {
  if (!capture_env.has_value()) {
    return nullptr;
  }
  auto it = capture_env->captures.find(name);
  if (it == capture_env->captures.end()) {
    return nullptr;
  }
  return &it->second;
}

IRValue LowerCtx::CaptureFieldPtr(const CaptureAccess& access) {
  IRValue ptr = FreshTempValue("capture_ptr");
  if (!capture_env.has_value()) {
    return ptr;
  }
  DerivedValueInfo info;
  info.kind = DerivedValueInfo::Kind::AddrTuple;
  info.base = capture_env->env_param;
  info.tuple_index = access.index;
  info.byte_offset = access.byte_offset;
  RegisterDerivedValue(ptr, info);
  auto elem_ptr = analysis::MakeTypeRawPtr(analysis::RawPtrQual::Mut, access.field_type);
  RegisterValueType(ptr, elem_ptr);
  return ptr;
}

IRValue LowerCtx::FreshTempValue(std::string_view prefix) {
  IRValue value;
  value.kind = IRValue::Kind::Opaque;
  value.name = std::string(prefix) + "_" + std::to_string(temp_counter++);
  return value;
}

std::string LowerCtx::FreshRegionAlias() {
  auto name_used = [this](const std::string& name) -> bool {
    auto it = binding_states.find(name);
    if (it != binding_states.end() && !it->second.empty()) {
      return true;
    }
    for (const auto& scope : scope_stack) {
      if (std::find(scope.region_tags.begin(), scope.region_tags.end(), name) !=
          scope.region_tags.end()) {
        return true;
      }
    }
    return false;
  };

  for (std::size_t i = 0;; ++i) {
    std::string name = std::string("region$") + std::to_string(i);
    if (!name_used(name)) {
      return name;
    }
  }
}

void LowerCtx::ReserveRegionTag(const std::string& name) {
  if (!scope_stack.empty()) {
    scope_stack.back().region_tags.push_back(name);
  }
}

void LowerCtx::ReportResolveFailure(const std::string& name) {
  resolve_failed = true;
  if (std::find(resolve_failures.begin(), resolve_failures.end(), name) ==
      resolve_failures.end()) {
    resolve_failures.push_back(name);
  }
}

void LowerCtx::ReportCodegenFailure(std::source_location loc) {
  codegen_failed = true;
  std::cerr << "[cursive] codegen failure at " << loc.file_name() << ":"
            << loc.line() << "\n";
}

void LowerCtx::RegisterValueType(const IRValue& value, analysis::TypeRef type) {
  if (!type) {
    return;
  }
  if (value.kind == IRValue::Kind::Symbol) {
    std::string key = "sym:";
    key += value.name;
    const auto [it, inserted] = value_types.emplace(key, type);
    if (!inserted) {
      it->second = type;
    } else if (value_type_insert_sink) {
      value_type_insert_sink->push_back(key);
    }
    return;
  }
  if (value.kind == IRValue::Kind::Opaque) {
    const auto [it, inserted] = value_types.emplace(value.name, type);
    if (!inserted) {
      it->second = type;
    } else if (value_type_insert_sink) {
      value_type_insert_sink->push_back(value.name);
    }
    return;
  }
  if (value.kind == IRValue::Kind::Immediate) {
    std::string key = "imm:";
    key += std::to_string(value.literal_id);
    const auto [it, inserted] = value_types.emplace(key, type);
    if (!inserted) {
      it->second = type;
    } else if (value_type_insert_sink) {
      value_type_insert_sink->push_back(key);
    }
  }
}

analysis::TypeRef LowerCtx::LookupValueType(const IRValue& value) const {
  if (value.kind == IRValue::Kind::Local) {
    if (const auto* state = GetBindingState(value.name)) {
      return state->type;
    }
    if (map_parent != nullptr) {
      return map_parent->LookupValueType(value);
    }
    return nullptr;
  }
  if (value.kind == IRValue::Kind::Symbol) {
    std::string key = "sym:";
    key += value.name;
    auto sym_it = value_types.find(key);
    if (sym_it != value_types.end()) {
      return sym_it->second;
    }
    if (map_parent != nullptr) {
      if (analysis::TypeRef inherited = map_parent->LookupValueType(value)) {
        return inherited;
      }
    }
    if (analysis::TypeRef static_type = LookupStaticType(value.name)) {
      return static_type;
    }
    if (const auto* state = GetBindingState(value.name)) {
      return state->type;
    }
    if (const auto* capture = LookupCapture(value.name)) {
      return capture->value_type;
    }
    return nullptr;
  }
  if (value.kind == IRValue::Kind::Opaque) {
    auto it = value_types.find(value.name);
    if (it != value_types.end()) {
      return it->second;
    }
    if (map_parent != nullptr) {
      return map_parent->LookupValueType(value);
    }
    return nullptr;
  }
  if (value.kind == IRValue::Kind::Immediate && value.literal_id != 0) {
    std::string key = "imm:";
    key += std::to_string(value.literal_id);
    auto it = value_types.find(key);
    if (it != value_types.end()) {
      return it->second;
    }
    if (map_parent != nullptr) {
      return map_parent->LookupValueType(value);
    }
  }
  return nullptr;
}

void LowerCtx::FreezeLookupTables() {
  if (baseline_tables != nullptr) {
    return;
  }
  auto tables = std::make_shared<LookupTables>();
  tables->static_types = std::move(static_types);
  tables->static_modules = std::move(static_modules);
  tables->record_ctor_paths = std::move(record_ctor_paths);
  tables->proc_sigs = std::move(proc_sigs);
  tables->proc_linkages = std::move(proc_linkages);
  tables->proc_visibilities = std::move(proc_visibilities);
  tables->proc_modules = std::move(proc_modules);
  tables->export_unwind_modes = std::move(export_unwind_modes);
  tables->foreign_contracts = std::move(foreign_contracts);
  tables->local_contracts = std::move(local_contracts);
  tables->async_procs = std::move(async_procs);
  baseline_tables = std::move(tables);
}

void LowerCtx::RegisterStaticType(const std::string& sym, analysis::TypeRef type) {
  if (!type) {
    return;
  }
  static_types[sym] = type;
}

analysis::TypeRef LowerCtx::LookupStaticType(const std::string& sym) const {
  auto it = static_types.find(sym);
  if (it != static_types.end()) {
    return it->second;
  }
  if (baseline_tables != nullptr) {
    const auto base_it = baseline_tables->static_types.find(sym);
    if (base_it != baseline_tables->static_types.end()) {
      return base_it->second;
    }
  }
  return nullptr;
}

void LowerCtx::RegisterStaticModule(const std::string& sym,
                                    const ast::ModulePath& module_path) {
  static_modules[sym] = module_path;
}

const std::vector<std::string>* LowerCtx::LookupStaticModule(
    const std::string& sym) const {
  auto it = static_modules.find(sym);
  if (it != static_modules.end()) {
    return &it->second;
  }
  if (baseline_tables != nullptr) {
    const auto base_it = baseline_tables->static_modules.find(sym);
    if (base_it != baseline_tables->static_modules.end()) {
      return &base_it->second;
    }
  }
  return nullptr;
}

void LowerCtx::RegisterDropGlueType(const std::string& sym, analysis::TypeRef type) {
  if (!type) {
    return;
  }
  drop_glue_types[sym] = type;
}

analysis::TypeRef LowerCtx::LookupDropGlueType(const std::string& sym) const {
  auto it = drop_glue_types.find(sym);
  if (it != drop_glue_types.end()) {
    return it->second;
  }
  if (map_parent != nullptr) {
    if (analysis::TypeRef inherited = map_parent->LookupDropGlueType(sym)) {
      return inherited;
    }
  }
  return nullptr;
}

void LowerCtx::RegisterRecordCtor(const std::string& sym,
                                  const std::vector<std::string>& path) {
  record_ctor_paths[sym] = path;
}

const std::vector<std::string>* LowerCtx::LookupRecordCtor(
    const std::string& sym) const {
  auto it = record_ctor_paths.find(sym);
  if (it != record_ctor_paths.end()) {
    return &it->second;
  }
  if (baseline_tables != nullptr) {
    const auto base_it = baseline_tables->record_ctor_paths.find(sym);
    if (base_it != baseline_tables->record_ctor_paths.end()) {
      return &base_it->second;
    }
  }
  return nullptr;
}

void LowerCtx::RegisterProcSig(const ProcIR& proc) {
  ProcSigInfo info;
  info.params = proc.params;
  info.ret = proc.ret;
  info.abi = proc.abi;

  if (auto existing = proc_sigs.find(proc.symbol); existing != proc_sigs.end()) {
    info.ffi_import = existing->second.ffi_import;
    info.ffi_import_unwind_mode = existing->second.ffi_import_unwind_mode;
    if (!info.abi.has_value()) {
      info.abi = existing->second.abi;
    }
  } else if (baseline_tables != nullptr) {
    const auto base_it = baseline_tables->proc_sigs.find(proc.symbol);
    if (base_it != baseline_tables->proc_sigs.end()) {
      info.ffi_import = base_it->second.ffi_import;
      info.ffi_import_unwind_mode = base_it->second.ffi_import_unwind_mode;
      if (!info.abi.has_value()) {
        info.abi = base_it->second.abi;
      }
    }
  }

  proc_sigs[proc.symbol] = std::move(info);
}

const LowerCtx::ProcSigInfo* LowerCtx::LookupProcSig(const std::string& sym) const {
  auto it = proc_sigs.find(sym);
  if (it != proc_sigs.end()) {
    return &it->second;
  }
  if (baseline_tables != nullptr) {
    const auto base_it = baseline_tables->proc_sigs.find(sym);
    if (base_it != baseline_tables->proc_sigs.end()) {
      return &base_it->second;
    }
  }
  return nullptr;
}

void LowerCtx::RegisterProcLinkage(const std::string& sym,
                                   LinkageKind linkage) {
  proc_linkages[sym] = linkage;
}

std::optional<LinkageKind> LowerCtx::LookupProcLinkage(
    const std::string& sym) const {
  auto it = proc_linkages.find(sym);
  if (it != proc_linkages.end()) {
    return it->second;
  }
  if (baseline_tables != nullptr) {
    const auto base_it = baseline_tables->proc_linkages.find(sym);
    if (base_it != baseline_tables->proc_linkages.end()) {
      return base_it->second;
    }
  }
  return std::nullopt;
}

const std::unordered_map<std::string, LinkageKind>& LowerCtx::AllProcLinkages()
    const {
  if (baseline_tables != nullptr) {
    return baseline_tables->proc_linkages;
  }
  return proc_linkages;
}

void LowerCtx::RegisterProcVisibility(const std::string& sym,
                                      ast::Visibility visibility) {
  proc_visibilities[sym] = visibility;
}

std::optional<ast::Visibility> LowerCtx::LookupProcVisibility(
    const std::string& sym) const {
  auto it = proc_visibilities.find(sym);
  if (it != proc_visibilities.end()) {
    return it->second;
  }
  if (baseline_tables != nullptr) {
    const auto base_it = baseline_tables->proc_visibilities.find(sym);
    if (base_it != baseline_tables->proc_visibilities.end()) {
      return base_it->second;
    }
  }
  return std::nullopt;
}

void LowerCtx::RegisterProcFfiImport(const std::string& sym,
                                     FfiImportUnwindMode mode) {
  auto mark_import = [&](ProcSigInfo& info) {
    info.ffi_import = true;
    info.ffi_import_unwind_mode = mode;
  };

  auto it = proc_sigs.find(sym);
  if (it != proc_sigs.end()) {
    mark_import(it->second);
    return;
  }

  if (baseline_tables != nullptr) {
    const auto base_it = baseline_tables->proc_sigs.find(sym);
    if (base_it != baseline_tables->proc_sigs.end()) {
      ProcSigInfo copied = base_it->second;
      mark_import(copied);
      proc_sigs[sym] = std::move(copied);
      return;
    }
  }

  ProcSigInfo info;
  mark_import(info);
  proc_sigs[sym] = std::move(info);
}

bool LowerCtx::NeedsPanicOutForSymbol(const std::string& sym) const {
  if (const auto* sig = LookupProcSig(sym)) {
    return !sig->params.empty() &&
           sig->params.back().name == std::string(kPanicOutName);
  }
  return NeedsPanicOut(sym);
}

void LowerCtx::RegisterProcModule(const std::string& sym, const ast::ModulePath& module_path) {
  if (proc_modules.find(sym) != proc_modules.end()) {
    return;
  }
  if (proc_modules.find(sym) == proc_modules.end() &&
      baseline_tables != nullptr) {
    if (baseline_tables->proc_modules.find(sym) !=
        baseline_tables->proc_modules.end()) {
      return;
    }
  }
  proc_modules[sym] = module_path;
}

const std::vector<std::string>* LowerCtx::LookupProcModule(const std::string& sym) const {
  auto it = proc_modules.find(sym);
  if (it != proc_modules.end()) {
    return &it->second;
  }
  if (baseline_tables != nullptr) {
    const auto base_it = baseline_tables->proc_modules.find(sym);
    if (base_it != baseline_tables->proc_modules.end()) {
      return &base_it->second;
    }
  }
  return nullptr;
}

void LowerCtx::QueueExtraProc(ProcIR proc,
                              std::optional<LinkageKind> linkage,
                              const ast::ModulePath* module_path) {
  RegisterProcSig(proc);
  if (linkage.has_value()) {
    RegisterProcLinkage(proc.symbol, *linkage);
  }
  if (module_path != nullptr) {
    RegisterProcModule(proc.symbol, *module_path);
  }
  extra_procs.push_back(std::move(proc));
}

void LowerCtx::RegisterExportUnwindMode(const std::string& sym,
                                        ExportUnwindMode mode) {
  export_unwind_modes[sym] = mode;
}

std::optional<LowerCtx::ExportUnwindMode> LowerCtx::LookupExportUnwindMode(
    const std::string& sym) const {
  auto it = export_unwind_modes.find(sym);
  if (it != export_unwind_modes.end()) {
    return it->second;
  }
  if (baseline_tables != nullptr) {
    const auto base_it = baseline_tables->export_unwind_modes.find(sym);
    if (base_it != baseline_tables->export_unwind_modes.end()) {
      return base_it->second;
    }
  }
  return std::nullopt;
}

void LowerCtx::RegisterForeignContractInfo(const std::string& sym,
                                           ForeignContractInfo info) {
  foreign_contracts[sym] = std::move(info);
}

const LowerCtx::ForeignContractInfo* LowerCtx::LookupForeignContractInfo(
    const std::string& sym) const {
  auto it = foreign_contracts.find(sym);
  if (it != foreign_contracts.end()) {
    return &it->second;
  }
  if (baseline_tables != nullptr) {
    const auto base_it = baseline_tables->foreign_contracts.find(sym);
    if (base_it != baseline_tables->foreign_contracts.end()) {
      return &base_it->second;
    }
  }
  return nullptr;
}

void LowerCtx::RegisterLocalContractInfo(const std::string& sym,
                                         LocalContractInfo info) {
  local_contracts[sym] = std::move(info);
}

const LowerCtx::LocalContractInfo* LowerCtx::LookupLocalContractInfo(
    const std::string& sym) const {
  auto it = local_contracts.find(sym);
  if (it != local_contracts.end()) {
    return &it->second;
  }
  if (baseline_tables != nullptr) {
    const auto base_it = baseline_tables->local_contracts.find(sym);
    if (base_it != baseline_tables->local_contracts.end()) {
      return &base_it->second;
    }
  }
  return nullptr;
}

const LowerCtx::AsyncProcInfo* LowerCtx::LookupAsyncProc(const std::string& sym) const {
  auto it = async_procs.find(sym);
  if (it != async_procs.end()) {
    return &it->second;
  }
  if (baseline_tables != nullptr) {
    const auto base_it = baseline_tables->async_procs.find(sym);
    if (base_it != baseline_tables->async_procs.end()) {
      return &base_it->second;
    }
  }
  return nullptr;
}

// ============================================================================
// Â§6.4 LowerList - Lower a list of expressions in LTR order
// ============================================================================

std::pair<IRPtr, std::vector<IRValue>> LowerList(
    const std::vector<ast::ExprPtr>& exprs, LowerCtx& ctx) {
  SPEC_RULE("LowerList-Empty");
  SPEC_RULE("LowerList-Cons");

  if (exprs.empty()) {
    return {EmptyIR(), {}};
  }

  std::vector<IRPtr> ir_parts;
  std::vector<IRValue> values;

  for (const auto& expr : exprs) {
    // Element values are consumed by the parent aggregate (tuple/array), so
    // they should not be tracked as temporaries requiring cleanup.
    auto prev_suppress = ctx.suppress_temp_at_depth;
    ctx.suppress_temp_at_depth = ctx.temp_depth + 1;
    auto result = LowerExpr(*expr, ctx);
    ctx.suppress_temp_at_depth = prev_suppress;
    ir_parts.push_back(result.ir);
    values.push_back(result.value);
  }

  return {SeqIR(std::move(ir_parts)), std::move(values)};
}

// ============================================================================
// Â§6.4 LowerFieldInits - Lower field initializers in LTR order
// ============================================================================

std::pair<IRPtr, std::vector<std::pair<std::string, IRValue>>> LowerFieldInits(
    const std::vector<ast::FieldInit>& fields, LowerCtx& ctx, bool suppress_temps) {
  SPEC_RULE("LowerFieldInits-Empty");
  SPEC_RULE("LowerFieldInits-Cons");

  if (fields.empty()) {
    return {EmptyIR(), {}};
  }

  std::vector<IRPtr> ir_parts;
  std::vector<std::pair<std::string, IRValue>> field_values;

  for (const auto& field : fields) {
    auto prev_suppress = ctx.suppress_temp_at_depth;
    if (suppress_temps) {
      ctx.suppress_temp_at_depth = ctx.temp_depth + 1;
    }
    auto result = LowerExpr(*field.value, ctx);
    ctx.suppress_temp_at_depth = prev_suppress;
    ir_parts.push_back(result.ir);
    field_values.emplace_back(field.name, result.value);
  }

  return {SeqIR(std::move(ir_parts)), std::move(field_values)};
}

IRValue RegisterLoweredRecordValue(
    std::vector<std::pair<std::string, IRValue>> field_values,
    std::optional<analysis::TypeRef> record_type,
    std::string_view temp_prefix,
    LowerCtx& ctx) {
  IRValue record_value = ctx.FreshTempValue(std::string(temp_prefix));
  DerivedValueInfo info;
  info.kind = DerivedValueInfo::Kind::RecordLit;
  info.fields = std::move(field_values);
  ctx.RegisterDerivedValue(record_value, info);
  if (record_type.has_value() && *record_type) {
    ctx.RegisterValueType(record_value, *record_type);
  }
  return record_value;
}

// ============================================================================
// Â§6.4 LowerOpt - Lower optional expression
// ============================================================================

std::pair<IRPtr, std::optional<IRValue>> LowerOpt(
    const ast::ExprPtr& expr_opt, LowerCtx& ctx) {
  SPEC_RULE("LowerOpt-None");
  SPEC_RULE("LowerOpt-Some");

  if (!expr_opt) {
    return {EmptyIR(), std::nullopt};
  }

  auto result = LowerExpr(*expr_opt, ctx);
  return {result.ir, result.value};
}

// ============================================================================
// Â§6.4 LowerReadPlace - Read from a place expression
// ============================================================================

namespace {

// Helper to convert RangeVal to IRRange
IRRange ToIRRange(const RangeVal& range) {
  IRRange out;
  out.kind = range.kind;
  out.lo = range.lo;
  out.hi = range.hi;
  return out;
}

// Join module path components with "::"
std::string ModulePathString(const std::vector<std::string>& path) {
  std::string out;
  for (std::size_t i = 0; i < path.size(); ++i) {
    if (i) {
      out += "::";
    }
    out += path[i];
  }
  return out;
}

}  // namespace

LowerResult LowerReadPlace(const ast::Expr& place, LowerCtx& ctx) {
  return std::visit(
      [&](const auto& node) -> LowerResult {
        using T = std::decay_t<decltype(node)>;

        if constexpr (std::is_same_v<T, ast::IdentifierExpr>) {
          if (const BindingState* binding = ctx.GetBindingState(node.name)) {
            SPEC_RULE("Lower-ReadPlace-Ident-Local");
            IRReadVar read;
            read.name = node.name;
            IRValue value;
            value.kind = IRValue::Kind::Local;
            value.name = node.name;
            if (binding->type) {
              ctx.RegisterValueType(value, binding->type);
            } else if (ctx.expr_type) {
              ctx.RegisterValueType(value, ctx.expr_type(place));
            }
            return LowerResult{MakeIR(std::move(read)), value};
          }
          if (const auto* capture = ctx.LookupCapture(node.name)) {
            SPEC_RULE("Lower-ReadPlace-Ident-Capture");
            IRPtr ir = EmptyIR();
            IRValue field_ptr = ctx.CaptureFieldPtr(*capture);
            IRValue value = ctx.FreshTempValue("capture_val");
            if (capture->by_ref) {
              IRValue captured_ptr = ctx.FreshTempValue("capture_ptr");
              IRReadPtr load_ptr;
              load_ptr.ptr = field_ptr;
              load_ptr.result = captured_ptr;
              ctx.RegisterValueType(captured_ptr, capture->field_type);
              IRReadPtr load_val;
              load_val.ptr = captured_ptr;
              load_val.result = value;
              ir = SeqIR({MakeIR(std::move(load_ptr)), MakeIR(std::move(load_val))});
            } else {
              IRReadPtr load_val;
              load_val.ptr = field_ptr;
              load_val.result = value;
              ir = MakeIR(std::move(load_val));
            }
            if (ctx.expr_type) {
              ctx.RegisterValueType(value, ctx.expr_type(place));
            } else {
              ctx.RegisterValueType(value, capture->value_type);
            }
            return LowerResult{ir, value};
          }

          std::vector<std::string> full;
          std::string resolved_name = node.name;
          if (!ctx.resolve_name) {
            ctx.ReportResolveFailure(node.name);
            full = ctx.module_path;
          } else {
            auto resolved = ctx.resolve_name(node.name);
            if (!resolved.has_value() || resolved->empty()) {
              ctx.ReportResolveFailure(node.name);
              full = ctx.module_path;
            } else {
              full = *resolved;
              resolved_name = full.back();
              full.pop_back();
            }
          }

          SPEC_RULE("Lower-ReadPlace-Ident-Path");
          IRReadPath read;
          read.path = std::move(full);
          read.name = resolved_name;
          IRValue value;
          value.kind = IRValue::Kind::Symbol;
          value.name = resolved_name;
          if (ctx.expr_type) {
            ctx.RegisterValueType(value, ctx.expr_type(place));
          }
          return LowerResult{MakeIR(std::move(read)), value};
        } else if constexpr (std::is_same_v<T, ast::FieldAccessExpr>) {
          SPEC_RULE("Lower-ReadPlace-Field");
          auto base_result = LowerReadPlace(*node.base, ctx);
          IRValue field_value = ctx.FreshTempValue("place_field");
          if (ctx.expr_type) {
            ctx.RegisterValueType(field_value, ctx.expr_type(place));
          }
          DerivedValueInfo info;
          info.kind = DerivedValueInfo::Kind::Field;
          info.base = base_result.value;
          info.field = node.name;
          ctx.RegisterDerivedValue(field_value, info);
          return LowerResult{base_result.ir, field_value};
        } else if constexpr (std::is_same_v<T, ast::TupleAccessExpr>) {
          SPEC_RULE("Lower-ReadPlace-Tuple");
          auto base_result = LowerReadPlace(*node.base, ctx);
          IRValue elem_value = ctx.FreshTempValue("place_tuple_elem");
          if (ctx.expr_type) {
            ctx.RegisterValueType(elem_value, ctx.expr_type(place));
          }
          DerivedValueInfo info;
          info.kind = DerivedValueInfo::Kind::Tuple;
          info.base = base_result.value;
          info.tuple_index = static_cast<std::size_t>(std::stoull(node.index.lexeme));
          ctx.RegisterDerivedValue(elem_value, info);
          return LowerResult{base_result.ir, elem_value};
        } else if constexpr (std::is_same_v<T, ast::AttributedExpr>) {
          return node.expr ? LowerReadPlace(*node.expr, ctx)
                           : LowerResult{EmptyIR(), ctx.FreshTempValue("place_attr")};
        } else if constexpr (std::is_same_v<T, ast::IndexAccessExpr>) {
          auto base_result = LowerReadPlace(*node.base, ctx);

          if (std::holds_alternative<ast::RangeExpr>(node.index->node)) {
            SPEC_RULE("Lower-ReadPlace-Index-Range");
            const auto& range_node = std::get<ast::RangeExpr>(node.index->node);
            auto range_result = LowerRangeExpr(range_node, ctx);

            IRCheckRange check;
            check.base = base_result.value;
            check.range = ToIRRange(range_result.value);

            IRValue slice_value = ctx.FreshTempValue("place_slice");
            if (ctx.expr_type) {
              ctx.RegisterValueType(slice_value, ctx.expr_type(place));
            }
            DerivedValueInfo info;
            info.kind = DerivedValueInfo::Kind::Slice;
            info.base = base_result.value;
            info.range = ToIRRange(range_result.value);
            ctx.RegisterDerivedValue(slice_value, info);

            return LowerResult{SeqIR({base_result.ir, range_result.ir,
                                      MakeIR(std::move(check)),
                                      PanicCheck(ctx)}),
                               slice_value};
          }

          if (IsRangeIndexExpr(*node.index, ctx)) {
            SPEC_RULE("Lower-ReadPlace-Index-Range");
            auto range_result = LowerExpr(*node.index, ctx);
            const auto range_kind = RangeIndexKindOf(*node.index, ctx);

            IRCheckRange check;
            check.base = base_result.value;
            check.range_value = range_result.value;
            if (range_kind.has_value()) {
              check.range.kind = *range_kind;
            }

            IRValue slice_value = ctx.FreshTempValue("place_slice");
            if (ctx.expr_type) {
              ctx.RegisterValueType(slice_value, ctx.expr_type(place));
            }
            DerivedValueInfo info;
            info.kind = DerivedValueInfo::Kind::Slice;
            info.base = base_result.value;
            info.range_value = range_result.value;
            if (range_kind.has_value()) {
              info.range.kind = *range_kind;
            }
            ctx.RegisterDerivedValue(slice_value, info);

            return LowerResult{SeqIR({base_result.ir, range_result.ir,
                                      MakeIR(std::move(check)),
                                      PanicCheck(ctx)}),
                               slice_value};
          }

          SPEC_RULE("Lower-ReadPlace-Index-Scalar");
          auto index_result = LowerExpr(*node.index, ctx);
          const bool needs_check = NeedsIndexCheck(*node.base, ctx);
          IRCheckIndex check;
          check.base = base_result.value;
          check.index = index_result.value;

          IRValue elem_value = ctx.FreshTempValue("place_index_elem");
          if (ctx.expr_type) {
            ctx.RegisterValueType(elem_value, ctx.expr_type(place));
          }
          DerivedValueInfo info;
          info.kind = DerivedValueInfo::Kind::Index;
          info.base = base_result.value;
          info.index = index_result.value;
          ctx.RegisterDerivedValue(elem_value, info);

          std::vector<IRPtr> seq;
          seq.push_back(base_result.ir);
          seq.push_back(index_result.ir);
          if (needs_check) {
            seq.push_back(MakeIR(std::move(check)));
            seq.push_back(PanicCheck(ctx));
          }
          return LowerResult{SeqIR(std::move(seq)), elem_value};
        } else if constexpr (std::is_same_v<T, ast::DerefExpr>) {
          SPEC_RULE("Lower-ReadPlace-Deref");
          // Dereference operand is a full expression; lower it accordingly.
          auto ptr_result = LowerExpr(*node.value, ctx);
          analysis::TypeRef ptr_type = ctx.LookupValueType(ptr_result.value);
          if (ctx.expr_type) {
            analysis::TypeRef expr_ptr_type = ctx.expr_type(*node.value);
            auto pointer_specificity = [](const analysis::TypeRef& type) -> int {
              if (!type) {
                return 0;
              }
              analysis::TypeRef stripped = analysis::StripPerm(type);
              if (!stripped) {
                stripped = type;
              }
              if (const auto* ptr = std::get_if<analysis::TypePtr>(&stripped->node)) {
                return ptr->state.has_value() ? 3 : 2;
              }
              if (std::holds_alternative<analysis::TypeRawPtr>(stripped->node)) {
                return 2;
              }
              return 1;
            };
            if (pointer_specificity(expr_ptr_type) > pointer_specificity(ptr_type)) {
              ptr_type = expr_ptr_type;
            }
          }
          if (ptr_type) {
            auto deref_result = LowerRawDeref(ptr_result.value, ptr_type, ctx);
            return LowerResult{SeqIR({ptr_result.ir, deref_result.ir}),
                               deref_result.value};
          }
          IRReadPtr read;
          read.ptr = ptr_result.value;
          IRValue value = ctx.FreshTempValue("deref");
          read.result = value;
          if (ctx.expr_type) {
            ctx.RegisterValueType(value, ctx.expr_type(place));
          }

          IRCheckOp null_check;
          null_check.op = "nonnull";
          null_check.reason = PanicReasonString(PanicReason::NullDeref);
          null_check.lhs = ptr_result.value;

          IRCall active_call;
          active_call.callee.kind = IRValue::Kind::Symbol;
          active_call.callee.name = BuiltinModalSymRegionAddrIsActive();
          active_call.args.push_back(ptr_result.value);
          IRValue active_value = ctx.FreshTempValue("addr_active");
          active_call.result = active_value;
          ctx.RegisterValueType(active_value, analysis::MakeTypePrim("bool"));

          IRCheckOp active_check;
          active_check.op = "addr_active";
          active_check.reason = PanicReasonString(PanicReason::ExpiredDeref);
          active_check.lhs = active_value;

          return LowerResult{SeqIR({
                                 ptr_result.ir,
                                 MakeIR(std::move(null_check)),
                                 PanicCheck(ctx),
                                 MakeIR(std::move(active_call)),
                                 MakeIR(std::move(active_check)),
                                 PanicCheck(ctx),
                                 MakeIR(std::move(read)),
                             }),
                             value};
        }

        IRValue value = ctx.FreshTempValue("place_unknown");
        value.name = "place_read";
        return LowerResult{EmptyIR(), value};
      },
      place.node);
}

// ============================================================================
// Â§6.4 LowerWritePlace / LowerWritePlaceSub - Write to a place expression
// ============================================================================

namespace {

IRPtr LowerWritePlaceImpl(const ast::Expr& place,
                          const IRValue& value,
                          LowerCtx& ctx,
                          bool allow_drop);

}  // namespace

IRPtr LowerWritePlace(const ast::Expr& place,
                      const IRValue& value,
                      LowerCtx& ctx) {
  return LowerWritePlaceImpl(place, value, ctx, true);
}

IRPtr LowerWritePlaceSub(const ast::Expr& place,
                         const IRValue& value,
                         LowerCtx& ctx) {
  return LowerWritePlaceImpl(place, value, ctx, false);
}

namespace {

// Check if a static binding has responsibility
struct StaticBindFlags {
  bool has_responsibility = false;
  bool immovable = false;
};

bool IsPlaceExprLite(const ast::ExprPtr& expr);
bool IsMoveExprLite(const ast::ExprPtr& expr);

bool IsMoveExprLite(const ast::ExprPtr& expr) {
  return expr && std::holds_alternative<ast::MoveExpr>(expr->node);
}

bool IsPlaceExprLite(const ast::ExprPtr& expr) {
  if (!expr) {
    return false;
  }
  return std::visit(
      [&](const auto& node) -> bool {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, ast::AttributedExpr>) {
          return node.expr ? IsPlaceExprLite(node.expr) : false;
        } else if constexpr (std::is_same_v<T, ast::IdentifierExpr>) {
          return true;
        } else if constexpr (std::is_same_v<T, ast::FieldAccessExpr>) {
          return IsPlaceExprLite(node.base);
        } else if constexpr (std::is_same_v<T, ast::TupleAccessExpr>) {
          return IsPlaceExprLite(node.base);
        } else if constexpr (std::is_same_v<T, ast::IndexAccessExpr>) {
          return IsPlaceExprLite(node.base);
        } else if constexpr (std::is_same_v<T, ast::DerefExpr>) {
          return IsPlaceExprLite(node.value);
        }
        return false;
      },
      expr->node);
}

std::optional<StaticBindFlags> StaticBindFlagsFor(
    const ast::ModulePath& module_path,
    const std::string& name,
    LowerCtx& ctx) {
  if (!ctx.sigma) {
    return std::nullopt;
  }

  const ast::ASTModule* module = nullptr;
  for (const auto& mod : ctx.sigma->mods) {
    if (analysis::PathEq(mod.path, module_path)) {
      module = &mod;
      break;
    }
  }
  if (!module) {
    return std::nullopt;
  }

  for (const auto& item : module->items) {
    const auto* decl = std::get_if<ast::StaticDecl>(&item);
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

IRPtr LowerWritePlaceImpl(const ast::Expr& place,
                          const IRValue& value,
                          LowerCtx& ctx,
                          bool allow_drop) {
  auto register_ptr_type = [&](IRValue& ptr_value,
                               const analysis::TypeRef& elem_type) {
    if (!elem_type) {
      return;
    }
    auto ptr_type = analysis::MakeTypePtr(elem_type, analysis::PtrState::Valid);
    ctx.RegisterValueType(ptr_value, ptr_type);
  };

  std::function<analysis::TypeRef(const analysis::TypeRef&)> unwrap_elem_type =
      [&](const analysis::TypeRef& type) -> analysis::TypeRef {
    if (!type) {
      return nullptr;
    }
    if (const auto* perm = std::get_if<analysis::TypePerm>(&type->node)) {
      return unwrap_elem_type(perm->base);
    }
    if (const auto* slice = std::get_if<analysis::TypeSlice>(&type->node)) {
      return slice->element;
    }
    if (const auto* arr = std::get_if<analysis::TypeArray>(&type->node)) {
      return arr->element;
    }
    return nullptr;
  };

  return std::visit(
      [&](const auto& node) -> IRPtr {
        using T = std::decay_t<decltype(node)>;

        if constexpr (std::is_same_v<T, ast::IdentifierExpr>) {
          if (ctx.GetBindingState(node.name)) {
            SPEC_RULE(allow_drop ? "Lower-WritePlace-Ident-Local"
                                 : "LowerWriteSub-Ident-Local");
            if (allow_drop) {
              SPEC_RULE("UpdateValid-StoreVar");
              auto it = ctx.binding_states.find(node.name);
              if (it != ctx.binding_states.end() && !it->second.empty()) {
                it->second.back().is_moved = false;
                it->second.back().moved_fields.clear();
              }
            }
            IRStoreVar store;
            IRStoreVarNoDrop store_nodrop;
            if (allow_drop) {
              store.name = node.name;
              store.value = value;
              return MakeIR(std::move(store));
            }
            SPEC_RULE("UpdateValid-StoreVarNoDrop");
            store_nodrop.name = node.name;
            store_nodrop.value = value;
            return MakeIR(std::move(store_nodrop));
          }
          if (const auto* capture = ctx.LookupCapture(node.name)) {
            SPEC_RULE(allow_drop ? "Lower-WritePlace-Ident-Capture"
                                 : "LowerWriteSub-Ident-Capture");
            IRValue field_ptr = ctx.CaptureFieldPtr(*capture);
            if (capture->by_ref) {
              IRValue captured_ptr = ctx.FreshTempValue("capture_ptr");
              IRReadPtr load_ptr;
              load_ptr.ptr = field_ptr;
              load_ptr.result = captured_ptr;
              register_ptr_type(captured_ptr, capture->value_type);
              IRWritePtr write;
              write.ptr = captured_ptr;
              write.value = value;
              return SeqIR({MakeIR(std::move(load_ptr)), MakeIR(std::move(write))});
            }
            IRWritePtr write;
            write.ptr = field_ptr;
            write.value = value;
            register_ptr_type(field_ptr, capture->value_type);
            return MakeIR(std::move(write));
          }

          std::vector<std::string> full;
          std::string resolved_name = node.name;
          if (!ctx.resolve_name) {
            ctx.ReportResolveFailure(node.name);
            full = ctx.module_path;
          } else {
            auto resolved = ctx.resolve_name(node.name);
            if (!resolved.has_value() || resolved->empty()) {
              ctx.ReportResolveFailure(node.name);
              full = ctx.module_path;
            } else {
              full = *resolved;
              resolved_name = full.back();
              full.pop_back();
            }
          }

          SPEC_RULE(allow_drop ? "Lower-WritePlace-Ident-Path"
                               : "LowerWriteSub-Ident-Path");
          IRPtr poison_ir = EmptyIR();
          IRCheckPoison check;
          check.module = ModulePathString(full);
          poison_ir = MakeIR(std::move(check));

          IRPtr drop_ir = EmptyIR();
          if (allow_drop) {
            if (auto flags = StaticBindFlagsFor(full, resolved_name, ctx)) {
              if (flags->has_responsibility) {
                analysis::TypeRef static_type;
                if (ctx.expr_type) {
                  static_type = ctx.expr_type(place);
                }
                IRReadPath read;
                read.path = full;
                read.name = resolved_name;
                IRValue current_value;
                current_value.kind = IRValue::Kind::Symbol;
                current_value.name = resolved_name;
                drop_ir = SeqIR({MakeIR(std::move(read)),
                                 EmitDrop(static_type, current_value, ctx)});
              }
            }
          }

          IRStoreGlobal store;
          store.symbol = StaticSymPath(full, resolved_name);
          store.value = value;

          auto is_noop = [](const IRPtr& ir) {
            return !ir || std::holds_alternative<IROpaque>(ir->node);
          };

          std::vector<IRPtr> parts;
          if (!is_noop(poison_ir)) {
            parts.push_back(poison_ir);
            parts.push_back(PanicCheck(ctx));
          }
          if (!is_noop(drop_ir)) {
            parts.push_back(drop_ir);
          }
          parts.push_back(MakeIR(std::move(store)));

          if (parts.size() == 1) {
            return parts.front();
          }
          return SeqIR(std::move(parts));
        } else if constexpr (std::is_same_v<T, ast::FieldAccessExpr>) {
          SPEC_RULE(allow_drop ? "Lower-WritePlace-Field"
                               : "LowerWriteSub-Field");
          auto base_addr = LowerAddrOf(*node.base, ctx);

          IRValue ptr_value = ctx.FreshTempValue("addr_of_field");
          if (ctx.expr_type) {
            register_ptr_type(ptr_value, ctx.expr_type(place));
          }
          DerivedValueInfo info;
          info.kind = DerivedValueInfo::Kind::AddrField;
          info.base = base_addr.value;
          info.field = node.name;
          ctx.RegisterDerivedValue(ptr_value, info);

          IRPtr drop_ir = EmptyIR();
          if (allow_drop && DropOnAssignRoot(*node.base, ctx)) {
            analysis::TypeRef field_type;
            if (ctx.expr_type) {
              field_type = ctx.expr_type(place);
            }
            IRValue field_value = ctx.FreshTempValue("place_field_old");
            ctx.RegisterValueType(field_value, field_type);
            IRReadPtr read;
            read.ptr = ptr_value;
            read.result = field_value;
            drop_ir = SeqIR({MakeIR(std::move(read)),
                             EmitDrop(field_type, field_value, ctx)});
          }

          IRAddrOf addr_marker;
          addr_marker.place = LowerPlace(place, ctx);
          addr_marker.result = ptr_value;

          IRWritePtr write;
          write.ptr = ptr_value;
          write.value = value;

          if (allow_drop) {
            UpdateBindingAfterFieldAssign(place, ctx);
          }

          return SeqIR({base_addr.ir,
                        MakeIR(std::move(addr_marker)),
                        drop_ir,
                        MakeIR(std::move(write))});
        } else if constexpr (std::is_same_v<T, ast::TupleAccessExpr>) {
          SPEC_RULE(allow_drop ? "Lower-WritePlace-Tuple"
                               : "LowerWriteSub-Tuple");
          auto base_addr = LowerAddrOf(*node.base, ctx);

          IRValue ptr_value = ctx.FreshTempValue("addr_of_tuple");
          if (ctx.expr_type) {
            register_ptr_type(ptr_value, ctx.expr_type(place));
          }
          DerivedValueInfo info;
          info.kind = DerivedValueInfo::Kind::AddrTuple;
          info.base = base_addr.value;
          info.tuple_index = static_cast<std::size_t>(std::stoull(node.index.lexeme));
          ctx.RegisterDerivedValue(ptr_value, info);

          IRPtr drop_ir = EmptyIR();
          if (allow_drop && DropOnAssignRoot(*node.base, ctx)) {
            analysis::TypeRef elem_type;
            if (ctx.expr_type) {
              elem_type = ctx.expr_type(place);
            }
            IRValue elem_value = ctx.FreshTempValue("place_tuple_old");
            ctx.RegisterValueType(elem_value, elem_type);
            IRReadPtr read;
            read.ptr = ptr_value;
            read.result = elem_value;
            drop_ir = SeqIR({MakeIR(std::move(read)),
                             EmitDrop(elem_type, elem_value, ctx)});
          }

          IRAddrOf addr_marker;
          addr_marker.place = LowerPlace(place, ctx);
          addr_marker.result = ptr_value;

          IRWritePtr write;
          write.ptr = ptr_value;
          write.value = value;

          return SeqIR({base_addr.ir,
                        MakeIR(std::move(addr_marker)),
                        drop_ir,
                        MakeIR(std::move(write))});
        } else if constexpr (std::is_same_v<T, ast::AttributedExpr>) {
          return node.expr ? LowerWritePlaceImpl(*node.expr, value, ctx, allow_drop)
                           : EmptyIR();
        } else if constexpr (std::is_same_v<T, ast::IndexAccessExpr>) {
          auto base_addr = LowerAddrOf(*node.base, ctx);

          analysis::TypeRef base_type;
          if (ctx.expr_type) {
            base_type = ctx.expr_type(*node.base);
          }
          IRValue base_value = ctx.FreshTempValue("place_index_base");
          ctx.RegisterValueType(base_value, base_type);
          IRReadPtr read_base;
          read_base.ptr = base_addr.value;
          read_base.result = base_value;
          IRPtr base_read_ir = MakeIR(std::move(read_base));

          if (std::holds_alternative<ast::RangeExpr>(node.index->node)) {
            SPEC_RULE(allow_drop ? "Lower-WritePlace-Index-Range"
                                 : "LowerWriteSub-Index-Range");
            const auto& range_node = std::get<ast::RangeExpr>(node.index->node);
            auto range_result = LowerRangeExpr(range_node, ctx);

            IRCheckRange check;
            check.base = base_value;
            check.range = ToIRRange(range_result.value);

            IRCheckSliceLen len_check;
            len_check.base = base_value;
            len_check.range = ToIRRange(range_result.value);
            len_check.value = value;

            IRValue ptr_value = ctx.FreshTempValue("addr_of_range");
            if (ctx.expr_type) {
              auto elem_type = unwrap_elem_type(ctx.expr_type(place));
              register_ptr_type(ptr_value, elem_type);
            }
            DerivedValueInfo info;
            info.kind = DerivedValueInfo::Kind::AddrIndex;
            info.base = base_addr.value;
            info.range = ToIRRange(range_result.value);
            ctx.RegisterDerivedValue(ptr_value, info);

            IRWritePtr write;
            write.ptr = ptr_value;
            write.value = value;

            IRAddrOf addr_marker;
            addr_marker.place = LowerPlace(place, ctx);
            addr_marker.result = ptr_value;

            return SeqIR({base_addr.ir, base_read_ir, range_result.ir,
                          MakeIR(std::move(check)),
                          PanicCheck(ctx),
                          MakeIR(std::move(len_check)),
                          PanicCheck(ctx),
                          MakeIR(std::move(addr_marker)),
                          MakeIR(std::move(write))});
          }

          if (IsRangeIndexExpr(*node.index, ctx)) {
            SPEC_RULE(allow_drop ? "Lower-WritePlace-Index-Range"
                                 : "LowerWriteSub-Index-Range");
            auto range_result = LowerExpr(*node.index, ctx);
            const auto range_kind = RangeIndexKindOf(*node.index, ctx);

            IRCheckRange check;
            check.base = base_value;
            check.range_value = range_result.value;
            if (range_kind.has_value()) {
              check.range.kind = *range_kind;
            }

            IRCheckSliceLen len_check;
            len_check.base = base_value;
            len_check.range_value = range_result.value;
            if (range_kind.has_value()) {
              len_check.range.kind = *range_kind;
            }
            len_check.value = value;

            IRValue ptr_value = ctx.FreshTempValue("addr_of_range");
            if (ctx.expr_type) {
              auto elem_type = unwrap_elem_type(ctx.expr_type(place));
              register_ptr_type(ptr_value, elem_type);
            }
            DerivedValueInfo info;
            info.kind = DerivedValueInfo::Kind::AddrIndex;
            info.base = base_addr.value;
            info.range_value = range_result.value;
            if (range_kind.has_value()) {
              info.range.kind = *range_kind;
            }
            ctx.RegisterDerivedValue(ptr_value, info);

            IRWritePtr write;
            write.ptr = ptr_value;
            write.value = value;

            IRAddrOf addr_marker;
            addr_marker.place = LowerPlace(place, ctx);
            addr_marker.result = ptr_value;

            return SeqIR({base_addr.ir, base_read_ir, range_result.ir,
                          MakeIR(std::move(check)),
                          PanicCheck(ctx),
                          MakeIR(std::move(len_check)),
                          PanicCheck(ctx),
                          MakeIR(std::move(addr_marker)),
                          MakeIR(std::move(write))});
          }

          SPEC_RULE(allow_drop ? "Lower-WritePlace-Index-Scalar"
                               : "LowerWriteSub-Index-Scalar");
          auto index_result = LowerExpr(*node.index, ctx);

          const bool needs_check = NeedsIndexCheck(*node.base, ctx);
          IRCheckIndex check;
          check.base = base_value;
          check.index = index_result.value;

          IRValue ptr_value = ctx.FreshTempValue("addr_of_index");
          if (ctx.expr_type) {
            register_ptr_type(ptr_value, ctx.expr_type(place));
          }
          DerivedValueInfo info;
          info.kind = DerivedValueInfo::Kind::AddrIndex;
          info.base = base_addr.value;
          info.index = index_result.value;
          ctx.RegisterDerivedValue(ptr_value, info);

          IRPtr drop_ir = EmptyIR();
          if (allow_drop && DropOnAssignRoot(*node.base, ctx)) {
            analysis::TypeRef elem_type;
            if (ctx.expr_type) {
              elem_type = ctx.expr_type(place);
            }
            IRValue elem_value = ctx.FreshTempValue("place_index_old");
            ctx.RegisterValueType(elem_value, elem_type);
            IRReadPtr read;
            read.ptr = ptr_value;
            read.result = elem_value;
            drop_ir = SeqIR({MakeIR(std::move(read)),
                             EmitDrop(elem_type, elem_value, ctx)});
          }

          IRAddrOf addr_marker;
          addr_marker.place = LowerPlace(place, ctx);
          addr_marker.result = ptr_value;

          IRWritePtr write;
          write.ptr = ptr_value;
          write.value = value;

          std::vector<IRPtr> seq;
          seq.push_back(base_addr.ir);
          seq.push_back(base_read_ir);
          seq.push_back(index_result.ir);
          if (needs_check) {
            seq.push_back(MakeIR(std::move(check)));
            seq.push_back(PanicCheck(ctx));
          }
          seq.push_back(MakeIR(std::move(addr_marker)));
          seq.push_back(drop_ir);
          seq.push_back(MakeIR(std::move(write)));
          return SeqIR(std::move(seq));
        } else if constexpr (std::is_same_v<T, ast::DerefExpr>) {
          SPEC_RULE(allow_drop ? "Lower-WritePlace-Deref"
                               : "LowerWriteSub-Deref");
          // Dereference operand is a full expression; lower it accordingly.
          auto ptr_result = LowerExpr(*node.value, ctx);
          analysis::TypeRef ptr_type = ctx.LookupValueType(ptr_result.value);
          if (ctx.expr_type) {
            analysis::TypeRef expr_ptr_type = ctx.expr_type(*node.value);
            auto pointer_specificity = [](const analysis::TypeRef& type) -> int {
              if (!type) {
                return 0;
              }
              analysis::TypeRef stripped = analysis::StripPerm(type);
              if (!stripped) {
                stripped = type;
              }
              if (const auto* ptr = std::get_if<analysis::TypePtr>(&stripped->node)) {
                return ptr->state.has_value() ? 3 : 2;
              }
              if (std::holds_alternative<analysis::TypeRawPtr>(stripped->node)) {
                return 2;
              }
              return 1;
            };
            if (pointer_specificity(expr_ptr_type) > pointer_specificity(ptr_type)) {
              ptr_type = expr_ptr_type;
            }
          }
          ptr_type = analysis::StripPerm(ptr_type);
          if (ptr_type) {
            if (const auto* ptr = std::get_if<analysis::TypePtr>(&ptr_type->node)) {
              if (ptr->state.has_value()) {
                if (*ptr->state == analysis::PtrState::Null) {
                  return SeqIR({ptr_result.ir, LowerPanic(PanicReason::NullDeref, ctx)});
                }
                if (*ptr->state == analysis::PtrState::Expired) {
                  return SeqIR({ptr_result.ir, LowerPanic(PanicReason::ExpiredDeref, ctx)});
                }
                if (*ptr->state == analysis::PtrState::Valid) {
                  IRCheckOp null_check;
                  null_check.op = "nonnull";
                  null_check.reason = PanicReasonString(PanicReason::NullDeref);
                  null_check.lhs = ptr_result.value;

                  IRCall active_call;
                  active_call.callee.kind = IRValue::Kind::Symbol;
                  active_call.callee.name = BuiltinModalSymRegionAddrIsActive();
                  active_call.args.push_back(ptr_result.value);
                  IRValue active_value = ctx.FreshTempValue("addr_active");
                  active_call.result = active_value;
                  ctx.RegisterValueType(active_value, analysis::MakeTypePrim("bool"));

                  IRCheckOp active_check;
                  active_check.op = "addr_active";
                  active_check.reason = PanicReasonString(PanicReason::ExpiredDeref);
                  active_check.lhs = active_value;

                  IRWritePtr write;
                  write.ptr = ptr_result.value;
                  write.value = value;
                  return SeqIR({
                      ptr_result.ir,
                      MakeIR(std::move(null_check)),
                      PanicCheck(ctx),
                      MakeIR(std::move(active_call)),
                      MakeIR(std::move(active_check)),
                      PanicCheck(ctx),
                      MakeIR(std::move(write)),
                  });
                }
              } else {
                IRCheckOp null_check;
                null_check.op = "nonnull";
                null_check.reason = PanicReasonString(PanicReason::NullDeref);
                null_check.lhs = ptr_result.value;

                IRCall active_call;
                active_call.callee.kind = IRValue::Kind::Symbol;
                active_call.callee.name = BuiltinModalSymRegionAddrIsActive();
                active_call.args.push_back(ptr_result.value);
                IRValue active_value = ctx.FreshTempValue("addr_active");
                active_call.result = active_value;
                ctx.RegisterValueType(active_value, analysis::MakeTypePrim("bool"));

                IRCheckOp active_check;
                active_check.op = "addr_active";
                active_check.reason = PanicReasonString(PanicReason::ExpiredDeref);
                active_check.lhs = active_value;

                IRWritePtr write;
                write.ptr = ptr_result.value;
                write.value = value;
                return SeqIR({
                    ptr_result.ir,
                    MakeIR(std::move(null_check)),
                    PanicCheck(ctx),
                    MakeIR(std::move(active_call)),
                    MakeIR(std::move(active_check)),
                    PanicCheck(ctx),
                    MakeIR(std::move(write)),
                });
              }
            } else if (const auto* raw = std::get_if<analysis::TypeRawPtr>(&ptr_type->node)) {
              if (raw->qual == analysis::RawPtrQual::Imm) {
                return SeqIR({ptr_result.ir, LowerPanic(PanicReason::Other, ctx)});
              }
            } else if (const auto* path = std::get_if<analysis::TypePathType>(&ptr_type->node)) {
              if (!path->path.empty() && path->path.back() == "Ptr") {
                IRCheckOp null_check;
                null_check.op = "nonnull";
                null_check.reason = PanicReasonString(PanicReason::NullDeref);
                null_check.lhs = ptr_result.value;

                IRCall active_call;
                active_call.callee.kind = IRValue::Kind::Symbol;
                active_call.callee.name = BuiltinModalSymRegionAddrIsActive();
                active_call.args.push_back(ptr_result.value);
                IRValue active_value = ctx.FreshTempValue("addr_active");
                active_call.result = active_value;
                ctx.RegisterValueType(active_value, analysis::MakeTypePrim("bool"));

                IRCheckOp active_check;
                active_check.op = "addr_active";
                active_check.reason = PanicReasonString(PanicReason::ExpiredDeref);
                active_check.lhs = active_value;

                IRWritePtr write;
                write.ptr = ptr_result.value;
                write.value = value;
                return SeqIR({
                    ptr_result.ir,
                    MakeIR(std::move(null_check)),
                    PanicCheck(ctx),
                    MakeIR(std::move(active_call)),
                    MakeIR(std::move(active_check)),
                    PanicCheck(ctx),
                    MakeIR(std::move(write)),
                });
              }
            }
          }
          IRCheckOp null_check;
          null_check.op = "nonnull";
          null_check.reason = PanicReasonString(PanicReason::NullDeref);
          null_check.lhs = ptr_result.value;

          IRCall active_call;
          active_call.callee.kind = IRValue::Kind::Symbol;
          active_call.callee.name = BuiltinModalSymRegionAddrIsActive();
          active_call.args.push_back(ptr_result.value);
          IRValue active_value = ctx.FreshTempValue("addr_active");
          active_call.result = active_value;
          ctx.RegisterValueType(active_value, analysis::MakeTypePrim("bool"));

          IRCheckOp active_check;
          active_check.op = "addr_active";
          active_check.reason = PanicReasonString(PanicReason::ExpiredDeref);
          active_check.lhs = active_value;

          IRWritePtr write;
          write.ptr = ptr_result.value;
          write.value = value;
          return SeqIR({
              ptr_result.ir,
              MakeIR(std::move(null_check)),
              PanicCheck(ctx),
              MakeIR(std::move(active_call)),
              MakeIR(std::move(active_check)),
              PanicCheck(ctx),
              MakeIR(std::move(write)),
          });
        }

        return EmptyIR();
      },
      place.node);
}

}  // namespace

// ============================================================================
// Â§6.4 LowerExprImpl - Internal dispatch for expression lowering
// ============================================================================

namespace {

LowerResult LowerExprImpl(const ast::Expr& expr, LowerCtx& ctx) {
  return std::visit(
      [&ctx, &expr](const auto& node) -> LowerResult {
        using T = std::decay_t<decltype(node)>;

        if constexpr (std::is_same_v<T, ast::ErrorExpr>) {
          return LowerError(node, ctx);
        } else if constexpr (std::is_same_v<T, ast::AttributedExpr>) {
          // Attributes are semantically transparent wrappers around the
          // underlying expression.
          return node.expr ? LowerExprImpl(*node.expr, ctx) : LowerResult{};
        } else if constexpr (std::is_same_v<T, ast::LiteralExpr>) {
          return LowerLiteral(expr, node, ctx);
        } else if constexpr (std::is_same_v<T, ast::IdentifierExpr>) {
          return LowerIdentifier(expr, node, ctx);
        } else if constexpr (std::is_same_v<T, ast::PathExpr>) {
          return LowerPath(node, ctx);
        } else if constexpr (std::is_same_v<T, ast::PtrNullExpr>) {
          return LowerPtrNull(node, ctx);
        } else if constexpr (std::is_same_v<T, ast::TupleExpr>) {
          return LowerTuple(node, ctx);
        } else if constexpr (std::is_same_v<T, ast::ArrayExpr>) {
          return LowerArrayLiteral(node, ctx);
        } else if constexpr (std::is_same_v<T, ast::ArrayRepeatExpr>) {
          SPEC_RULE("Lower-Expr-ArrayRepeat");
          auto value_result = LowerExpr(*node.value, ctx);
          auto count_result = LowerExpr(*node.count, ctx);
          IRValue array_value = ctx.FreshTempValue("array_repeat");
          DerivedValueInfo info;
          info.kind = DerivedValueInfo::Kind::ArrayRepeat;
          info.repeat_value = value_result.value;
          info.repeat_count = count_result.value;
          ctx.RegisterDerivedValue(array_value, info);
          return LowerResult{SeqIR({value_result.ir, count_result.ir}), array_value};
        } else if constexpr (std::is_same_v<T, ast::SizeofExpr>) {
          SPEC_RULE("Lower-Expr-Sizeof");
          if (!ctx.sigma) {
            ctx.ReportCodegenFailure();
            return LowerResult{EmptyIR(), IRValue{}};
          }
          const analysis::ScopeContext& scope = ScopeForLowering(ctx);
          auto lowered = LowerTypeForLayout(scope, node.type);
          if (!lowered) {
            ctx.ReportCodegenFailure();
            return LowerResult{EmptyIR(), IRValue{}};
          }
          auto layout = LayoutOf(scope, *lowered);
          if (!layout) {
            ctx.ReportCodegenFailure();
            return LowerResult{EmptyIR(), IRValue{}};
          }
          IRValue value;
          value.kind = IRValue::Kind::Immediate;
          value.name = std::to_string(layout->size);
          value.bytes = EncodeU64LE(layout->size);
          return LowerResult{EmptyIR(), value};
        } else if constexpr (std::is_same_v<T, ast::AlignofExpr>) {
          SPEC_RULE("Lower-Expr-Alignof");
          if (!ctx.sigma) {
            ctx.ReportCodegenFailure();
            return LowerResult{EmptyIR(), IRValue{}};
          }
          const analysis::ScopeContext& scope = ScopeForLowering(ctx);
          auto lowered = LowerTypeForLayout(scope, node.type);
          if (!lowered) {
            ctx.ReportCodegenFailure();
            return LowerResult{EmptyIR(), IRValue{}};
          }
          auto layout = LayoutOf(scope, *lowered);
          if (!layout) {
            ctx.ReportCodegenFailure();
            return LowerResult{EmptyIR(), IRValue{}};
          }
          IRValue value;
          value.kind = IRValue::Kind::Immediate;
          value.name = std::to_string(layout->align);
          value.bytes = EncodeU64LE(layout->align);
          return LowerResult{EmptyIR(), value};
        } else if constexpr (std::is_same_v<T, ast::RecordExpr>) {
          return LowerRecord(node, ctx);
        } else if constexpr (std::is_same_v<T, ast::EnumLiteralExpr>) {
          return LowerEnumLiteral(node, ctx);
        } else if constexpr (std::is_same_v<T, ast::FieldAccessExpr>) {
          return LowerReadPlaceFieldAccess(node, expr, ctx);
        } else if constexpr (std::is_same_v<T, ast::TupleAccessExpr>) {
          return LowerTupleAccess(node, ctx);
        } else if constexpr (std::is_same_v<T, ast::IndexAccessExpr>) {
          return LowerIndexAccess(node, ctx);
        } else if constexpr (std::is_same_v<T, ast::CallExpr>) {
          return LowerCallExpr(node, ctx);
        } else if constexpr (std::is_same_v<T, ast::MethodCallExpr>) {
          return LowerMethodCall(node, ctx);
        } else if constexpr (std::is_same_v<T, ast::UnaryExpr>) {
          return LowerUnaryExpr(node, ctx);
        } else if constexpr (std::is_same_v<T, ast::BinaryExpr>) {
          return LowerBinaryExpr(node, ctx);
        } else if constexpr (std::is_same_v<T, ast::CastExpr>) {
          return LowerCastExpr(node, ctx);
        } else if constexpr (std::is_same_v<T, ast::TransmuteExpr>) {
          return LowerTransmuteExpr(expr, node, ctx);
        } else if constexpr (std::is_same_v<T, ast::ClosureExpr>) {
          return LowerClosureExpr(expr, node, ctx);
        } else if constexpr (std::is_same_v<T, ast::PipelineExpr>) {
          return LowerPipelineExpr(node, ctx);
        } else if constexpr (std::is_same_v<T, ast::PropagateExpr>) {
          return LowerPropagateExpr(node, ctx);
        } else if constexpr (std::is_same_v<T, ast::IfExpr>) {
          return LowerIfExpr(node, ctx);
        } else if constexpr (std::is_same_v<T, ast::IfCaseExpr>) {
          return LowerIfCases(*node.scrutinee, node.cases, node.else_expr,
                              false, ctx);
        } else if constexpr (std::is_same_v<T, ast::IfIsExpr>) {
          std::vector<ast::IfCaseClause> cases;
          ast::IfCaseClause case_clause;
          case_clause.pattern = node.pattern;
          case_clause.body = node.then_expr;
          cases.push_back(std::move(case_clause));
          return LowerIfCases(*node.scrutinee, cases, node.else_expr,
                              true, ctx);
        } else if constexpr (std::is_same_v<T, ast::BlockExpr>) {
          return LowerBlock(*node.block, ctx);
        } else if constexpr (std::is_same_v<T, ast::LoopInfiniteExpr>) {
          return LowerLoopInfinite(node, ctx);
        } else if constexpr (std::is_same_v<T, ast::LoopConditionalExpr>) {
          return LowerLoopConditional(node, ctx);
        } else if constexpr (std::is_same_v<T, ast::LoopIterExpr>) {
          return LowerLoopIter(node, ctx);
        } else if constexpr (std::is_same_v<T, ast::AddressOfExpr>) {
          return LowerAddrOf(*node.place, ctx);
        } else if constexpr (std::is_same_v<T, ast::DerefExpr>) {
          return LowerReadPlaceDeref(node, expr, ctx);
        } else if constexpr (std::is_same_v<T, ast::MoveExpr>) {
          return LowerMovePlace(*node.place, ctx);
        } else if constexpr (std::is_same_v<T, ast::RangeExpr>) {
          return LowerRange(expr, node, ctx);
        } else if constexpr (std::is_same_v<T, ast::AllocExpr>) {
          return LowerAllocExpr(expr, node, ctx);
        } else if constexpr (std::is_same_v<T, ast::UnsafeBlockExpr>) {
          return LowerUnsafeBlockExpr(node, ctx);
        } else if constexpr (std::is_same_v<T, ast::ComptimeExpr>) {
          return LowerExpr(*node.body, ctx);
        } else if constexpr (std::is_same_v<T, ast::ParallelExpr>) {
          return LowerParallelExpr(node, ctx);
        } else if constexpr (std::is_same_v<T, ast::SpawnExpr>) {
          return LowerSpawnExpr(expr, node, ctx);
        } else if constexpr (std::is_same_v<T, ast::DispatchExpr>) {
          return LowerDispatchExpr(node, ctx);
        } else if constexpr (std::is_same_v<T, ast::WaitExpr>) {
          return LowerWaitExpr(node, ctx);
        } else if constexpr (std::is_same_v<T, ast::FenceExpr>) {
          IRValue result = ctx.FreshTempValue("fence");
          IRFence fence;
          fence.order = node.order;
          fence.result = result;
          return LowerResult{MakeIR(std::move(fence)), result};
        } else if constexpr (std::is_same_v<T, ast::YieldExpr>) {
          return LowerYieldExpr(node, ctx);
        } else if constexpr (std::is_same_v<T, ast::YieldFromExpr>) {
          return LowerYieldFromExpr(node, ctx);
        } else if constexpr (std::is_same_v<T, ast::SyncExpr>) {
          return LowerSyncExpr(node, ctx);
        } else if constexpr (std::is_same_v<T, ast::RaceExpr>) {
          return LowerRaceExpr(node, ctx);
        } else if constexpr (std::is_same_v<T, ast::AllExpr>) {
          return LowerAllExpr(node, ctx);
        } else if constexpr (std::is_same_v<T, ast::EntryExpr>) {
          return LowerEntryExpr(node, ctx);
        } else if constexpr (std::is_same_v<T, ast::ResultExpr>) {
          return LowerResultExpr(node, ctx);
        } else if constexpr (std::is_same_v<T, ast::QualifiedNameExpr>) {
          return LowerQualifiedName(node, ctx);
        } else if constexpr (std::is_same_v<T, ast::QualifiedApplyExpr>) {
          return LowerQualifiedApply(node, ctx);
        } else {
          // Unknown expression form
          IRValue value = ctx.FreshTempValue("unknown_expr");
          return LowerResult{EmptyIR(), value};
        }
      },
      expr.node);
}

}  // namespace

// ============================================================================
// Â§6.4 LowerExpr - main expression lowering entry point
// ============================================================================

LowerResult LowerExpr(const ast::Expr& expr, LowerCtx& ctx) {
  ctx.temp_depth += 1;
  LowerResult result = LowerExprImpl(expr, ctx);
  const int depth = ctx.temp_depth;
  analysis::TypeRef value_type = ctx.LookupValueType(result.value);

  if (ctx.expr_type) {
    // Preserve a type established by expression-specific lowering logic.
    // This is required for reconstructed AST nodes (for example, contract
    // implication recovery) where expr_type(expr) may not have a precise type.
    if (!value_type) {
      if (analysis::TypeRef inferred = ctx.expr_type(expr)) {
        ctx.RegisterValueType(result.value, inferred);
        value_type = inferred;
      }
    }
  }
  ctx.temp_depth -= 1;

  bool suppress = ctx.suppress_temp_at_depth.has_value() &&
                  *ctx.suppress_temp_at_depth == depth;
  if (suppress) {
    ctx.suppress_temp_at_depth.reset();
  }

  if (!suppress && ctx.temp_sink && IsTempValueExpr(expr)) {
    ctx.RegisterTempValue(result.value, value_type);
  }

  if (const auto* attr = std::get_if<ast::AttributedExpr>(&expr.node)) {
    if (IRPtr log_ir =
            EmitLogAttributeTrace(attr->attrs, expr.span, result.value,
                                  value_type, "expression", ctx);
        log_ir && !std::holds_alternative<IROpaque>(log_ir->node)) {
      result.ir = SeqIR({result.ir, log_ir});
    }
  }

  if (IRPtr refine_check_ir =
          EmitDynamicRefinementChecksImpl(expr, result.value, value_type, ctx);
      refine_check_ir && !std::holds_alternative<IROpaque>(refine_check_ir->node)) {
    result.ir = SeqIR({result.ir, refine_check_ir});
  }

  return result;
}

IRPtr EmitDynamicRefinementChecksForExpr(const ast::Expr& expr,
                                        const IRValue& value,
                                        analysis::TypeRef value_type,
                                        LowerCtx& ctx) {
  return EmitDynamicRefinementChecksImpl(expr, value, value_type, ctx);
}

// ============================================================================
// Â§6.4 LowerPlace - lower a place to its representation
// ============================================================================

IRPlace LowerPlace(const ast::Expr& place, LowerCtx& /*ctx*/) {
  SPEC_RULE("Lower-Place-Ident");
  SPEC_RULE("Lower-Place-Field");
  SPEC_RULE("Lower-Place-Tuple");
  SPEC_RULE("Lower-Place-Index");
  SPEC_RULE("Lower-Place-Deref");

  IRPlace result;
  result.repr = BuildPlaceRepr(place);
  return result;
}

// SplitModulePathString is defined in llvm_ir_panic.cpp (canonical location)

}  // namespace cursive::codegen
