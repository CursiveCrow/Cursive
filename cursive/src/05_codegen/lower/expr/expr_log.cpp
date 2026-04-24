// =============================================================================
// Expression Lowering Log and Refinement Helpers
// =============================================================================

#include "05_codegen/lower/expr/expr_common.h"

#include <algorithm>
#include <string_view>
#include <variant>

#include "04_analysis/attributes/attribute_registry.h"
#include "04_analysis/generics/monomorphize.h"
#include "04_analysis/resolve/scopes.h"
#include "04_analysis/typing/type_expr.h"
#include "05_codegen/checks/checks.h"
#include "05_codegen/intrinsics/builtins.h"
#include "04_analysis/layout/layout.h"

namespace cursive::codegen {

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
  const auto lowered = ::cursive::analysis::layout::LowerTypeForLayout(scope, alias->type);
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

std::optional<IRValue> BuildLogExpectedImmediate(
    const ast::Token& expected_token,
    const analysis::TypeRef& actual_type,
    const LowerCtx& ctx) {
  const analysis::TypeRef compare_type =
      ResolveAliasTypeForLogCompare(actual_type, ctx);
  if (!compare_type) {
    return std::nullopt;
  }

  if (auto encoded = ::cursive::analysis::layout::EncodeConst(compare_type, expected_token)) {
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
      auto decoded = ::cursive::analysis::layout::DecodeStringLiteralBytes(expected_token.lexeme);
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

bool HasLogAttr(const ast::AttributeList& attrs) {
  return FindLogAttr(attrs) != nullptr;
}

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

IRPtr EmitLogAttributeTrace(const ast::AttributeList& attrs,
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
  expected_imm->literal_id = ++(*ctx.temp_counter);
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
  expected_imm->literal_id = ++(*ctx.temp_counter);
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

IRPtr EmitDynamicRefinementChecksForExpr(const ast::Expr& expr,
                                        const IRValue& value,
                                        analysis::TypeRef value_type,
                                        LowerCtx& ctx) {
  return EmitDynamicRefinementChecksImpl(expr, value, value_type, ctx);
}

}  // namespace cursive::codegen
