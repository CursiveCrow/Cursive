#include "cursive0/codegen/lower/lower_expr.h"
#include "cursive0/codegen/checks.h"
#include "cursive0/codegen/layout/layout.h"
#include "cursive0/core/assert_spec.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <variant>

namespace cursive0::codegen {

namespace {

std::string StripIntSuffix(std::string text) {
  static const char* suffixes[] = {
      "isize", "usize", "i128", "u128", "i64", "u64", "i32", "u32", "i16", "u16", "i8", "u8"
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
  try {
    size_t idx = 0;
    std::uint64_t out = std::stoull(text, &idx, 0);
    if (idx != text.size()) {
      return std::nullopt;
    }
    return out;
  } catch (...) {
    return std::nullopt;
  }
}

std::vector<std::uint8_t> EncodeU64BE(std::uint64_t value) {
  std::vector<std::uint8_t> bytes;
  if (value == 0) {
    bytes.push_back(0);
    return bytes;
  }
  while (value > 0) {
    bytes.push_back(static_cast<std::uint8_t>(value & 0xFF));
    value >>= 8;
  }
  std::reverse(bytes.begin(), bytes.end());
  return bytes;
}

syntax::ExprPtr WrapBlockExpr(const std::shared_ptr<syntax::Block>& block) {
  if (!block) {
    return nullptr;
  }
  auto expr = std::make_shared<syntax::Expr>();
  expr->span = block->span;
  expr->node = syntax::BlockExpr{block};
  return expr;
}

bool IsPlaceExprForTemp(const syntax::Expr& expr) {
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
          return true;
        }
        return false;
      },
      expr.node);
}

bool IsTempValueExpr(const syntax::Expr& expr) {
  return !IsPlaceExprForTemp(expr);
}

std::vector<syntax::ExprPtr> ArgsExprs(const std::vector<syntax::Arg>& args) {
  if (args.empty()) {
    SPEC_RULE("ArgsExprs-Empty");
    return {};
  }
  SPEC_RULE("ArgsExprs-Cons");
  std::vector<syntax::ExprPtr> result;
  result.reserve(args.size());
  for (const auto& arg : args) {
    result.push_back(arg.value);
  }
  return result;
}

std::vector<syntax::ExprPtr> FieldExprs(const std::vector<syntax::FieldInit>& fields) {
  if (fields.empty()) {
    SPEC_RULE("FieldExprs-Empty");
    return {};
  }
  SPEC_RULE("FieldExprs-Cons");
  std::vector<syntax::ExprPtr> result;
  result.reserve(fields.size());
  for (const auto& field : fields) {
    result.push_back(field.value);
  }
  return result;
}

std::vector<syntax::ExprPtr> OptExprs(const syntax::ExprPtr& lo_opt,
                                     const syntax::ExprPtr& hi_opt) {
  if (!lo_opt && !hi_opt) {
    SPEC_RULE("OptExprs-None");
    return {};
  }
  if (lo_opt && !hi_opt) {
    SPEC_RULE("OptExprs-Lo");
    return {lo_opt};
  }
  if (!lo_opt && hi_opt) {
    SPEC_RULE("OptExprs-Hi");
    return {hi_opt};
  }
  SPEC_RULE("OptExprs-Both");
  return {lo_opt, hi_opt};
}




IRRange ToIRRange(const RangeVal& range) {
  IRRange out;
  out.kind = range.kind;
  out.lo = range.lo;
  out.hi = range.hi;
  return out;
}

}  // namespace

// Helpers moved to lower_expr.h / ir_model.cpp

// ============================================================================
// §6.4 LowerList - Lower a list of expressions in LTR order
// ============================================================================

std::pair<IRPtr, std::vector<IRValue>> LowerList(
    const std::vector<syntax::ExprPtr>& exprs, LowerCtx& ctx) {
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
// §6.4 LowerFieldInits - Lower field initializers in LTR order
// ============================================================================

std::pair<IRPtr, std::vector<std::pair<std::string, IRValue>>> LowerFieldInits(
    const std::vector<syntax::FieldInit>& fields, LowerCtx& ctx, bool suppress_temps) {
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

// ============================================================================
// §6.4 LowerOpt - Lower optional expression
// ============================================================================

std::pair<IRPtr, std::optional<IRValue>> LowerOpt(
    const syntax::ExprPtr& expr_opt, LowerCtx& ctx) {
  SPEC_RULE("LowerOpt-None");
  SPEC_RULE("LowerOpt-Some");
  
  if (!expr_opt) {
    return {EmptyIR(), std::nullopt};
  }
  
  auto result = LowerExpr(*expr_opt, ctx);
  return {result.ir, result.value};
}

// ============================================================================
// §6.4 Children_LTR - Evaluation order helpers
// ============================================================================

std::vector<syntax::ExprPtr> ChildrenLTR(const syntax::Expr& expr) {
  return std::visit(
      [](const auto& node) -> std::vector<syntax::ExprPtr> {
        using T = std::decay_t<decltype(node)>;
        
        if constexpr (std::is_same_v<T, syntax::ErrorExpr>) {
          return {};
        } else if constexpr (std::is_same_v<T, syntax::LiteralExpr>) {
          SPEC_RULE("EvalOrder-Literal");
          return {};
        } else if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
          SPEC_RULE("EvalOrder-Ident");
          return {};
        } else if constexpr (std::is_same_v<T, syntax::PathExpr>) {
          SPEC_RULE("EvalOrder-Path");
          return {};
        } else if constexpr (std::is_same_v<T, syntax::PtrNullExpr>) {
          SPEC_RULE("EvalOrder-PtrNull");
          return {};
        } else if constexpr (std::is_same_v<T, syntax::TupleExpr>) {
          SPEC_RULE("EvalOrder-Tuple");
          return node.elements;
        } else if constexpr (std::is_same_v<T, syntax::ArrayExpr>) {
          SPEC_RULE("EvalOrder-Array");
          return node.elements;
        } else if constexpr (std::is_same_v<T, syntax::RecordExpr>) {
          SPEC_RULE("EvalOrder-Record");
          return FieldExprs(node.fields);
        } else if constexpr (std::is_same_v<T, syntax::EnumLiteralExpr>) {
          if (!node.payload_opt.has_value()) {
            SPEC_RULE("EvalOrder-Enum-Unit");
            return {};
          }
          return std::visit(
              [](const auto& payload) -> std::vector<syntax::ExprPtr> {
                using P = std::decay_t<decltype(payload)>;
                if constexpr (std::is_same_v<P, syntax::EnumPayloadParen>) {
                  SPEC_RULE("EvalOrder-Enum-Tuple");
                  return payload.elements;
                } else {
                  SPEC_RULE("EvalOrder-Enum-Record");
                  return FieldExprs(payload.fields);
                }
              },
              *node.payload_opt);
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
          SPEC_RULE("EvalOrder-FieldAccess");
          return {node.base};
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
          SPEC_RULE("EvalOrder-TupleAccess");
          return {node.base};
        } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
          SPEC_RULE("EvalOrder-IndexAccess");
          return {node.base, node.index};
        } else if constexpr (std::is_same_v<T, syntax::CallExpr>) {
          SPEC_RULE("EvalOrder-Call");
          auto result = ArgsExprs(node.args);
          result.insert(result.begin(), node.callee);
          return result;
        } else if constexpr (std::is_same_v<T, syntax::MethodCallExpr>) {
          SPEC_RULE("EvalOrder-MethodCall");
          auto result = ArgsExprs(node.args);
          result.insert(result.begin(), node.receiver);
          return result;
        } else if constexpr (std::is_same_v<T, syntax::UnaryExpr>) {
          SPEC_RULE("EvalOrder-Unary");
          return {node.value};
        } else if constexpr (std::is_same_v<T, syntax::BinaryExpr>) {
          SPEC_RULE("EvalOrder-Binary");
          return {node.lhs, node.rhs};
        } else if constexpr (std::is_same_v<T, syntax::CastExpr>) {
          SPEC_RULE("EvalOrder-Cast");
          return {node.value};
        } else if constexpr (std::is_same_v<T, syntax::TransmuteExpr>) {
          SPEC_RULE("EvalOrder-Transmute");
          return {node.value};
        } else if constexpr (std::is_same_v<T, syntax::PropagateExpr>) {
          SPEC_RULE("EvalOrder-Propagate");
          return {node.value};
        } else if constexpr (std::is_same_v<T, syntax::RangeExpr>) {
          SPEC_RULE("EvalOrder-Range");
          return OptExprs(node.lhs, node.rhs);
        } else if constexpr (std::is_same_v<T, syntax::IfExpr>) {
          SPEC_RULE("EvalOrder-If");
          return {node.cond};
        } else if constexpr (std::is_same_v<T, syntax::MatchExpr>) {
          SPEC_RULE("EvalOrder-Match");
          return {node.value};
        } else if constexpr (std::is_same_v<T, syntax::LoopInfiniteExpr>) {
          SPEC_RULE("EvalOrder-Loop");
          return {WrapBlockExpr(node.body)};
        } else if constexpr (std::is_same_v<T, syntax::LoopConditionalExpr>) {
          SPEC_RULE("EvalOrder-Loop");
          return {node.cond, WrapBlockExpr(node.body)};
        } else if constexpr (std::is_same_v<T, syntax::LoopIterExpr>) {
          SPEC_RULE("EvalOrder-Loop");
          return {node.iter, WrapBlockExpr(node.body)};
        } else if constexpr (std::is_same_v<T, syntax::BlockExpr>) {
          SPEC_RULE("EvalOrder-Block");
          return {};
        } else if constexpr (std::is_same_v<T, syntax::UnsafeBlockExpr>) {
          SPEC_RULE("EvalOrder-UnsafeBlock");
          return {};
        } else if constexpr (std::is_same_v<T, syntax::MoveExpr>) {
          SPEC_RULE("EvalOrder-Move");
          return {};
        } else if constexpr (std::is_same_v<T, syntax::AddressOfExpr>) {
          SPEC_RULE("EvalOrder-AddressOf");
          return {};
        } else if constexpr (std::is_same_v<T, syntax::DerefExpr>) {
          SPEC_RULE("EvalOrder-Deref");
          return {node.value};
        } else if constexpr (std::is_same_v<T, syntax::AllocExpr>) {
          SPEC_RULE("EvalOrder-Alloc");
          return {node.value};
        } else {
          return {};
        }
      },
      expr.node);
}

// ============================================================================
// §6.4 Core Expression Lowering
// ============================================================================

// Forward declarations for specialized lowering functions
LowerResult LowerLiteral(const syntax::Expr& expr, const syntax::LiteralExpr& lit, LowerCtx& ctx) {
  SPEC_RULE("Lower-Expr-Literal");

  IRValue value;
  value.kind = IRValue::Kind::Immediate;
  value.name = lit.literal.lexeme;

  if (lit.literal.kind == syntax::TokenKind::StringLiteral) {
    SPEC_RULE("StringLiteralVal");
    if (auto decoded = DecodeStringLiteralBytes(lit.literal.lexeme)) {
      value.bytes = std::move(*decoded);
    } else {
      ctx.ReportCodegenFailure();
    }
  }

  if (ctx.expr_type) {
    const auto lit_type = ctx.expr_type(expr);
    if (lit_type) {
      if (auto bytes = EncodeConst(lit_type, lit.literal)) {
        value.bytes = std::move(*bytes);
      }
    }
  }

  if (value.bytes.empty() && lit.literal.kind == syntax::TokenKind::IntLiteral) {
    if (auto parsed = ParseIntLiteralLexeme(lit.literal.lexeme)) {
      value.bytes = EncodeU64BE(*parsed);
    }
  } else if (value.bytes.empty() && lit.literal.kind == syntax::TokenKind::BoolLiteral) {
    value.bytes = {static_cast<std::uint8_t>(lit.literal.lexeme == "true" ? 1 : 0)};
  } else if (value.bytes.empty() && lit.literal.kind == syntax::TokenKind::NullLiteral) {
    value.bytes = {0};
  }

  return LowerResult{EmptyIR(), value};
}

// §6.4 Lower-Expr-Ident-Local / Lower-Expr-Ident-Path / Lower-Expr-Ident-Path
LowerResult LowerIdentifier(const syntax::IdentifierExpr& expr, LowerCtx& ctx) {
  const std::string& name = expr.name;  // Identifier is std::string

  if (ctx.GetBindingState(name)) {
    // Local variable
    SPEC_RULE("Lower-Expr-Ident-Local");

    IRReadVar read;
    read.name = name;

    IRValue value;
    value.kind = IRValue::Kind::Local;
    value.name = name;

    return LowerResult{MakeIR(std::move(read)), value};
  }

  if (!ctx.resolve_name) {
    ctx.ReportResolveFailure(name);
    SPEC_RULE("Lower-Expr-Ident-Path");

    IRReadPath read;
    read.path = ctx.module_path;
    read.name = name;

    IRValue value;
    value.kind = IRValue::Kind::Symbol;
    value.name = name;

    return LowerResult{MakeIR(std::move(read)), value};
  }

  auto resolved = ctx.resolve_name(name);

  if (!resolved.has_value() || resolved->empty()) {
    ctx.ReportResolveFailure(name);
    SPEC_RULE("Lower-Expr-Ident-Path");

    IRReadPath read;
    read.path = ctx.module_path;
    read.name = name;

    IRValue value;
    value.kind = IRValue::Kind::Symbol;
    value.name = name;

    return LowerResult{MakeIR(std::move(read)), value};
  }

  // Path (imported/global name)
  SPEC_RULE("Lower-Expr-Ident-Path");

  std::vector<std::string> full = *resolved;
  const std::string resolved_name = full.back();
  full.pop_back();

  IRReadPath read;
  read.path = std::move(full);
  read.name = resolved_name;

  IRValue value;
  value.kind = IRValue::Kind::Symbol;
  value.name = resolved_name;

  return LowerResult{MakeIR(std::move(read)), value};
}

// §6.4 Lower-Expr-Path
LowerResult LowerPath(const syntax::PathExpr& expr, LowerCtx& /*ctx*/) {
  SPEC_RULE("Lower-Expr-Path");
  
  IRReadPath read;
  // Path is std::vector<Identifier> where Identifier is std::string
  read.path = expr.path;
  read.name = expr.name;
  
  IRValue value;
  value.kind = IRValue::Kind::Symbol;
  value.name = expr.name;
  
  return LowerResult{MakeIR(std::move(read)), value};
}

// §6.4 Lower-Expr-PtrNull
LowerResult LowerPtrNull(const syntax::PtrNullExpr& /*expr*/, LowerCtx& ctx) {
  SPEC_RULE("Lower-Expr-PtrNull");
  
  IRValue value;
  value.kind = IRValue::Kind::Immediate;
  value.name = "null";
  // Null pointer is represented as 0x0
  value.bytes = {0, 0, 0, 0, 0, 0, 0, 0};  // 64-bit null
  
  return LowerResult{EmptyIR(), value};
}

// §6.4 Lower-Expr-Error
LowerResult LowerError(const syntax::ErrorExpr& /*expr*/, LowerCtx& ctx) {
  SPEC_RULE("Lower-Expr-Error");
  
  // Error expressions lower to a panic
  IRValue value = ctx.FreshTempValue("unreachable");
  
  return LowerResult{LowerPanic(PanicReason::ErrorExpr, ctx), value};
}

// §6.4 Lower-Expr-Tuple
LowerResult LowerTuple(const syntax::TupleExpr& expr, LowerCtx& ctx) {
  SPEC_RULE("Lower-Expr-Tuple");
  
  auto [ir, values] = LowerList(expr.elements, ctx);
  
  IRValue tuple_value = ctx.FreshTempValue("tuple");
  DerivedValueInfo info;
  info.kind = DerivedValueInfo::Kind::TupleLit;
  info.elements = values;
  ctx.RegisterDerivedValue(tuple_value, info);
  
  return LowerResult{ir, tuple_value};
}

// §6.4 Lower-Expr-Array
LowerResult LowerArray(const syntax::ArrayExpr& expr, LowerCtx& ctx) {
  SPEC_RULE("Lower-Expr-Array");
  
  auto [ir, values] = LowerList(expr.elements, ctx);
  
  IRValue array_value = ctx.FreshTempValue("array");
  DerivedValueInfo info;
  info.kind = DerivedValueInfo::Kind::ArrayLit;
  info.elements = values;
  ctx.RegisterDerivedValue(array_value, info);
  
  return LowerResult{ir, array_value};
}

// §6.4 Lower-Expr-Record
LowerResult LowerRecord(const syntax::RecordExpr& expr, LowerCtx& ctx) {
  SPEC_RULE("Lower-Expr-Record");
  
  // Field initializer values are consumed by the record construction, so they
  // should not be tracked as temporaries. This applies to both regular records
  // (TypePath) and modal state records (ModalStateRef).
  auto [ir, field_values] = LowerFieldInits(expr.fields, ctx, /*suppress_temps=*/true);
  
  IRValue record_value = ctx.FreshTempValue("record");
  DerivedValueInfo info;
  info.kind = DerivedValueInfo::Kind::RecordLit;
  info.fields = field_values;
  ctx.RegisterDerivedValue(record_value, info);
  
  return LowerResult{ir, record_value};
}

// §6.4 Lower-Expr-Enum-Unit / Lower-Expr-Enum-Tuple / Lower-Expr-Enum-Record
LowerResult LowerEnumLiteral(const syntax::EnumLiteralExpr& expr, LowerCtx& ctx) {
  if (!expr.payload_opt.has_value()) {
    // Unit variant
    SPEC_RULE("Lower-Expr-Enum-Unit");
    
    IRValue enum_value = ctx.FreshTempValue("enum_unit");
    DerivedValueInfo info;
    info.kind = DerivedValueInfo::Kind::EnumLit;
    info.variant = expr.path.empty() ? std::string() : expr.path.back();
    ctx.RegisterDerivedValue(enum_value, info);
    
    return LowerResult{EmptyIR(), enum_value};
  }
  
  return std::visit(
      [&ctx, &expr](const auto& payload) -> LowerResult {
        using T = std::decay_t<decltype(payload)>;
        
        if constexpr (std::is_same_v<T, syntax::EnumPayloadParen>) {
          // Tuple variant
          SPEC_RULE("Lower-Expr-Enum-Tuple");
          
          auto [ir, values] = LowerList(payload.elements, ctx);
          
          IRValue enum_value = ctx.FreshTempValue("enum_tuple");
          DerivedValueInfo info;
          info.kind = DerivedValueInfo::Kind::EnumLit;
          info.variant = expr.path.empty() ? std::string() : expr.path.back();
          info.payload_elems = values;
          ctx.RegisterDerivedValue(enum_value, info);
          
          return LowerResult{ir, enum_value};
        } else {
          // Record variant
          SPEC_RULE("Lower-Expr-Enum-Record");
          
          auto [ir, field_values] = LowerFieldInits(payload.fields, ctx, true);
          
          IRValue enum_value = ctx.FreshTempValue("enum_record");
          DerivedValueInfo info;
          info.kind = DerivedValueInfo::Kind::EnumLit;
          info.variant = expr.path.empty() ? std::string() : expr.path.back();
          info.payload_fields = field_values;
          ctx.RegisterDerivedValue(enum_value, info);
          
          return LowerResult{ir, enum_value};
        }
      },
      *expr.payload_opt);
}

// ============================================================================
// Anchor function for SPEC_RULE coverage
// ============================================================================

void AnchorExprLoweringRules() {
  // §6.4 Evaluation Order
  SPEC_RULE("EvalOrder-Literal");
  SPEC_RULE("EvalOrder-PtrNull");
  SPEC_RULE("EvalOrder-Ident");
  SPEC_RULE("EvalOrder-Path");
  SPEC_RULE("EvalOrder-Tuple");
  SPEC_RULE("EvalOrder-Array");
  SPEC_RULE("EvalOrder-Record");
  SPEC_RULE("EvalOrder-Enum-Unit");
  SPEC_RULE("EvalOrder-Enum-Tuple");
  SPEC_RULE("EvalOrder-Enum-Record");
  SPEC_RULE("EvalOrder-FieldAccess");
  SPEC_RULE("EvalOrder-TupleAccess");
  SPEC_RULE("EvalOrder-IndexAccess");
  SPEC_RULE("EvalOrder-Call");
  SPEC_RULE("EvalOrder-MethodCall");
  SPEC_RULE("EvalOrder-Unary");
  SPEC_RULE("EvalOrder-Binary");
  SPEC_RULE("EvalOrder-Cast");
  SPEC_RULE("EvalOrder-Transmute");
  SPEC_RULE("EvalOrder-Propagate");
  SPEC_RULE("EvalOrder-Range");
  SPEC_RULE("EvalOrder-If");
  SPEC_RULE("EvalOrder-Match");
  SPEC_RULE("EvalOrder-Loop");
  SPEC_RULE("EvalOrder-Block");
  SPEC_RULE("EvalOrder-UnsafeBlock");
  SPEC_RULE("EvalOrder-Move");
  SPEC_RULE("EvalOrder-AddressOf");
  SPEC_RULE("EvalOrder-Deref");
  SPEC_RULE("EvalOrder-Alloc");

  // §6.4 Eval helpers
  SPEC_RULE("ArgsExprs-Empty");
  SPEC_RULE("ArgsExprs-Cons");
  SPEC_RULE("FieldExprs-Empty");
  SPEC_RULE("FieldExprs-Cons");
  SPEC_RULE("OptExprs-None");
  SPEC_RULE("OptExprs-Lo");
  SPEC_RULE("OptExprs-Hi");
  SPEC_RULE("OptExprs-Both");

  // §6.4 Expression Lowering
  SPEC_RULE("Lower-Expr-Correctness");
  SPEC_RULE("Lower-Expr-Literal");
  SPEC_RULE("Lower-Expr-PtrNull");
  SPEC_RULE("Lower-Expr-Ident-Local");
  SPEC_RULE("Lower-Expr-Ident-Path");
  SPEC_RULE("Lower-Expr-Path");
  SPEC_RULE("Lower-Expr-Error");
  SPEC_RULE("Lower-Expr-Tuple");
  SPEC_RULE("Lower-Expr-Array");
  SPEC_RULE("Lower-Expr-Record");
  SPEC_RULE("Lower-Expr-Enum-Unit");
  SPEC_RULE("Lower-Expr-Enum-Tuple");
  SPEC_RULE("Lower-Expr-Enum-Record");
  SPEC_RULE("Lower-Expr-FieldAccess");
  SPEC_RULE("Lower-Expr-TupleAccess");
  SPEC_RULE("Lower-Expr-Index-Scalar");
  SPEC_RULE("Lower-Expr-Index-Scalar-OOB");
  SPEC_RULE("Lower-Expr-Index-Range");
  SPEC_RULE("Lower-Expr-Index-Range-OOB");
  SPEC_RULE("Lower-Expr-MethodCall");
  SPEC_RULE("Lower-Expr-Unary");
  SPEC_RULE("Lower-Expr-Binary");
  SPEC_RULE("Lower-Expr-Cast");
  SPEC_RULE("Lower-Expr-Transmute");
  SPEC_RULE("Lower-Expr-Range");
  SPEC_RULE("Lower-Expr-If");
  SPEC_RULE("Lower-Expr-Match");
  SPEC_RULE("Lower-Expr-LoopInf");
  SPEC_RULE("Lower-Expr-LoopCond");
  SPEC_RULE("Lower-Expr-LoopIter");
  SPEC_RULE("Lower-Expr-Block");
  SPEC_RULE("Lower-Expr-UnsafeBlock");
  SPEC_RULE("Lower-Expr-Move");
  SPEC_RULE("Lower-Expr-AddressOf");
  SPEC_RULE("Lower-Expr-Deref");
  SPEC_RULE("Lower-Expr-Alloc");

  // §6.4 List Lowering
  SPEC_RULE("LowerList-Empty");
  SPEC_RULE("LowerList-Cons");
  SPEC_RULE("LowerFieldInits-Empty");
  SPEC_RULE("LowerFieldInits-Cons");
  SPEC_RULE("LowerOpt-None");
  SPEC_RULE("LowerOpt-Some");
}


// ============================================================================
// §6.4 Main LowerExpr Dispatcher
// ============================================================================

// Forward declarations for expression handlers defined in other files
LowerResult LowerCallExpr(const syntax::CallExpr& expr, LowerCtx& ctx);
LowerResult LowerBinaryExpr(const syntax::BinaryExpr& expr, LowerCtx& ctx);
LowerResult LowerCastExpr(const syntax::CastExpr& expr, LowerCtx& ctx);
LowerResult LowerPropagateExpr(const syntax::PropagateExpr& expr, LowerCtx& ctx);
LowerResult LowerIfExpr(const syntax::IfExpr& expr, LowerCtx& ctx);

// §6.4 LowerExpr - main expression lowering dispatcher
static LowerResult LowerExprImpl(const syntax::Expr& expr, LowerCtx& ctx) {
  return std::visit(
      [&ctx, &expr](const auto& node) -> LowerResult {
        using T = std::decay_t<decltype(node)>;

        if constexpr (std::is_same_v<T, syntax::ErrorExpr>) {
          return LowerError(node, ctx);
        } else if constexpr (std::is_same_v<T, syntax::LiteralExpr>) {
          return LowerLiteral(expr, node, ctx);
        } else if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
          return LowerIdentifier(node, ctx);
        } else if constexpr (std::is_same_v<T, syntax::PathExpr>) {
          return LowerPath(node, ctx);
        } else if constexpr (std::is_same_v<T, syntax::PtrNullExpr>) {
          return LowerPtrNull(node, ctx);
        } else if constexpr (std::is_same_v<T, syntax::TupleExpr>) {
          return LowerTuple(node, ctx);
        } else if constexpr (std::is_same_v<T, syntax::ArrayExpr>) {
          return LowerArray(node, ctx);
        } else if constexpr (std::is_same_v<T, syntax::RecordExpr>) {
          return LowerRecord(node, ctx);
        } else if constexpr (std::is_same_v<T, syntax::EnumLiteralExpr>) {
          return LowerEnumLiteral(node, ctx);
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
          SPEC_RULE("Lower-Expr-FieldAccess");
          auto base_result = LowerExpr(*node.base, ctx);
          IRValue field_value = ctx.FreshTempValue("field");
          DerivedValueInfo info;
          info.kind = DerivedValueInfo::Kind::Field;
          info.base = base_result.value;
          info.field = node.name;
          ctx.RegisterDerivedValue(field_value, info);
          return LowerResult{base_result.ir, field_value};
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
          SPEC_RULE("Lower-Expr-TupleAccess");
          auto base_result = LowerExpr(*node.base, ctx);
          IRValue elem_value = ctx.FreshTempValue("tuple_elem");
          DerivedValueInfo info;
          info.kind = DerivedValueInfo::Kind::Tuple;
          info.base = base_result.value;
          info.tuple_index = static_cast<std::size_t>(std::stoull(node.index.lexeme));
          ctx.RegisterDerivedValue(elem_value, info);
          return LowerResult{base_result.ir, elem_value};
        } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
          auto base_result = LowerExpr(*node.base, ctx);
          if (std::holds_alternative<syntax::RangeExpr>(node.index->node)) {
            SPEC_RULE("Lower-Expr-Index-Range");
            const auto& range_node = std::get<syntax::RangeExpr>(node.index->node);
            auto range_result = LowerRangeExpr(range_node, ctx);
            IRCheckRange check;
            check.base = base_result.value;
            check.range = ToIRRange(range_result.value);
            IRValue slice_value = ctx.FreshTempValue("slice");
            DerivedValueInfo info;
            info.kind = DerivedValueInfo::Kind::Slice;
            info.base = base_result.value;
            info.range = ToIRRange(range_result.value);
            ctx.RegisterDerivedValue(slice_value, info);
            return LowerResult{SeqIR({base_result.ir, range_result.ir, MakeIR(std::move(check)),
                                      PanicCheck(ctx)}),
                               slice_value};
          }
          SPEC_RULE("Lower-Expr-Index-Scalar");
          auto index_result = LowerExpr(*node.index, ctx);
          IRCheckIndex check;
          check.base = base_result.value;
          check.index = index_result.value;
          IRValue elem_value = ctx.FreshTempValue("index_elem");
          DerivedValueInfo info;
          info.kind = DerivedValueInfo::Kind::Index;
          info.base = base_result.value;
          info.index = index_result.value;
          ctx.RegisterDerivedValue(elem_value, info);
          return LowerResult{SeqIR({base_result.ir, index_result.ir, MakeIR(std::move(check)),
                                    PanicCheck(ctx)}),
                             elem_value};
        } else if constexpr (std::is_same_v<T, syntax::CallExpr>) {
          return LowerCallExpr(node, ctx);
        } else if constexpr (std::is_same_v<T, syntax::MethodCallExpr>) {
          return LowerMethodCall(node, ctx);
        } else if constexpr (std::is_same_v<T, syntax::UnaryExpr>) {
          SPEC_RULE("Lower-Expr-Unary");
          return LowerUnOp(node.op, *node.value, ctx);
        } else if constexpr (std::is_same_v<T, syntax::BinaryExpr>) {
          return LowerBinaryExpr(node, ctx);
        } else if constexpr (std::is_same_v<T, syntax::CastExpr>) {
          return LowerCastExpr(node, ctx);
        } else if constexpr (std::is_same_v<T, syntax::TransmuteExpr>) {
          SPEC_RULE("Lower-Expr-Transmute");
          analysis::TypeRef to_type;
          analysis::TypeRef from_type;
          if (ctx.sigma) {
            analysis::ScopeContext scope;
            scope.sigma = *ctx.sigma;
            scope.current_module = ctx.module_path;
            if (node.to) {
              if (auto lowered = LowerTypeForLayout(scope, node.to)) {
                to_type = *lowered;
              }
            }
            if (node.from) {
              if (auto lowered = LowerTypeForLayout(scope, node.from)) {
                from_type = *lowered;
              }
            }
          }
          if (!to_type && ctx.expr_type) {
            to_type = ctx.expr_type(expr);
          }
          if (!from_type && ctx.expr_type) {
            from_type = ctx.expr_type(*node.value);
          }
          return LowerTransmute(from_type, to_type, *node.value, ctx);
        } else if constexpr (std::is_same_v<T, syntax::PropagateExpr>) {
          return LowerPropagateExpr(node, ctx);
        } else if constexpr (std::is_same_v<T, syntax::RangeExpr>) {
          SPEC_RULE("Lower-Expr-Range");
          auto range_result = LowerRangeExpr(node, ctx);
          IRValue range_value = ctx.FreshTempValue("range");
          DerivedValueInfo info;
          info.kind = DerivedValueInfo::Kind::RangeLit;
          info.range = ToIRRange(range_result.value);
          ctx.RegisterDerivedValue(range_value, info);
          return LowerResult{range_result.ir, range_value};
        } else if constexpr (std::is_same_v<T, syntax::IfExpr>) {
          return LowerIfExpr(node, ctx);
        } else if constexpr (std::is_same_v<T, syntax::MatchExpr>) {
          SPEC_RULE("Lower-Expr-Match");
          return LowerMatch(*node.value, node.arms, ctx);
        } else if constexpr (std::is_same_v<T, syntax::LoopInfiniteExpr>) {
          SPEC_RULE("Lower-Expr-LoopInf");
          return LowerLoop(expr, ctx);
        } else if constexpr (std::is_same_v<T, syntax::LoopConditionalExpr>) {
          SPEC_RULE("Lower-Expr-LoopCond");
          return LowerLoop(expr, ctx);
        } else if constexpr (std::is_same_v<T, syntax::LoopIterExpr>) {
          SPEC_RULE("Lower-Expr-LoopIter");
          return LowerLoop(expr, ctx);
        } else if constexpr (std::is_same_v<T, syntax::BlockExpr>) {
          SPEC_RULE("Lower-Expr-Block");
          return LowerBlock(*node.block, ctx);
        } else if constexpr (std::is_same_v<T, syntax::UnsafeBlockExpr>) {
          SPEC_RULE("Lower-Expr-UnsafeBlock");
          return LowerBlock(*node.block, ctx);
        } else if constexpr (std::is_same_v<T, syntax::MoveExpr>) {
          SPEC_RULE("Lower-Expr-Move");
          return LowerMovePlace(*node.place, ctx);
        } else if constexpr (std::is_same_v<T, syntax::AddressOfExpr>) {
          SPEC_RULE("Lower-Expr-AddressOf");
          return LowerAddrOf(*node.place, ctx);
        } else if constexpr (std::is_same_v<T, syntax::DerefExpr>) {
          SPEC_RULE("Lower-Expr-Deref");
          auto ptr_result = LowerExpr(*node.value, ctx);
          analysis::TypeRef ptr_type;
          if (ctx.expr_type) {
            ptr_type = ctx.expr_type(*node.value);
          }
          if (ptr_type) {
            auto deref_result = LowerRawDeref(ptr_result.value, ptr_type, ctx);
            return LowerResult{SeqIR({ptr_result.ir, deref_result.ir}), deref_result.value};
          }
          IRValue value = ctx.FreshTempValue("deref");
          IRReadPtr read;
          read.ptr = ptr_result.value;
          read.result = value;
          return LowerResult{SeqIR({ptr_result.ir, MakeIR(std::move(read))}), value};
        } else if constexpr (std::is_same_v<T, syntax::AllocExpr>) {
          SPEC_RULE("Lower-Expr-Alloc");
          auto value_result = LowerExpr(*node.value, ctx);
          IRAlloc alloc;
          alloc.value = value_result.value;
          if (node.region_opt) {
            syntax::IdentifierExpr ident;
            ident.name = *node.region_opt;
            syntax::Expr region_expr;
            region_expr.span = expr.span;
            region_expr.node = ident;
            auto region_result = LowerExpr(region_expr, ctx);
            alloc.region = region_result.value;
            IRValue ptr_value = ctx.FreshTempValue("alloc_ptr");
            alloc.result = ptr_value;
            return LowerResult{SeqIR({value_result.ir, region_result.ir, MakeIR(std::move(alloc))}),
                               ptr_value};
          }
          IRValue ptr_value = ctx.FreshTempValue("alloc_ptr");
          alloc.result = ptr_value;
          return LowerResult{SeqIR({value_result.ir, MakeIR(std::move(alloc))}), ptr_value};
        } else {
          IRValue value = ctx.FreshTempValue("unknown_expr");
          return LowerResult{EmptyIR(), value};
        }
      },
      expr.node);
}

LowerResult LowerExpr(const syntax::Expr& expr, LowerCtx& ctx) {
  ctx.temp_depth += 1;
  LowerResult result = LowerExprImpl(expr, ctx);
  const int depth = ctx.temp_depth;

  if (ctx.expr_type) {
    ctx.RegisterValueType(result.value, ctx.expr_type(expr));
  }
  ctx.temp_depth -= 1;

  bool suppress = ctx.suppress_temp_at_depth.has_value() &&
                  *ctx.suppress_temp_at_depth == depth;
  if (suppress) {
    ctx.suppress_temp_at_depth.reset();
    return result;
  }

  if (ctx.temp_sink && IsTempValueExpr(expr)) {
    analysis::TypeRef type;
    if (ctx.expr_type) {
      type = ctx.expr_type(expr);
    }
    ctx.RegisterTempValue(result.value, type);
  }

  return result;
}

// ============================================================================
// §6.8 LowerCtx Scope Tracking Methods
// ============================================================================

void LowerCtx::PushScope(bool is_loop, bool is_region) {
  ScopeInfo scope;
  scope.is_loop = is_loop;
  scope.is_region = is_region;
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
                            bool is_immovable) {
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

  binding_states[name].push_back(std::move(state));
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

void LowerCtx::RegisterDefer(const IRPtr& defer_ir) {
  if (!scope_stack.empty()) {
    CleanupItem item;
    item.kind = CleanupItem::Kind::DeferBlock;
    item.defer_ir = defer_ir;
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
  if (value.kind != IRValue::Kind::Opaque) {
    return;
  }
  derived_values[value.name] = info;
}

const DerivedValueInfo* LowerCtx::LookupDerivedValue(const IRValue& value) const {
  if (value.kind != IRValue::Kind::Opaque) {
    return nullptr;
  }
  auto it = derived_values.find(value.name);
  if (it == derived_values.end()) {
    return nullptr;
  }
  return &it->second;
}

IRValue LowerCtx::FreshTempValue(std::string_view prefix) {
  IRValue value;
  value.kind = IRValue::Kind::Opaque;
  value.name = std::string(prefix) + "_" + std::to_string(temp_counter++);
  return value;
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
  if (std::getenv("CURSIVE0_DEBUG_OBJ")) {
    std::cerr << "[cursivec0] codegen failure at " << loc.file_name() << ":"
              << loc.line() << "\n";
  }
}

void LowerCtx::RegisterValueType(const IRValue& value, analysis::TypeRef type) {
  if (!type) {
    return;
  }
  if (value.kind != IRValue::Kind::Opaque) {
    return;
  }
  value_types[value.name] = type;
}

analysis::TypeRef LowerCtx::LookupValueType(const IRValue& value) const {
  if (value.kind == IRValue::Kind::Local) {
    if (const auto* state = GetBindingState(value.name)) {
      return state->type;
    }
    return nullptr;
  }
  if (value.kind == IRValue::Kind::Symbol) {
    return LookupStaticType(value.name);
  }
  if (value.kind != IRValue::Kind::Opaque) {
    return nullptr;
  }
  auto it = value_types.find(value.name);
  if (it != value_types.end()) {
    return it->second;
  }
  return nullptr;
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
  return nullptr;
}

void LowerCtx::RegisterStaticModule(const std::string& sym,
                                    const syntax::ModulePath& module_path) {
  static_modules[sym] = module_path;
}

const std::vector<std::string>* LowerCtx::LookupStaticModule(
    const std::string& sym) const {
  auto it = static_modules.find(sym);
  if (it != static_modules.end()) {
    return &it->second;
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
  return nullptr;
}

void LowerCtx::RegisterProcSig(const ProcIR& proc) {
  ProcSigInfo info;
  info.params = proc.params;
  info.ret = proc.ret;
  proc_sigs[proc.symbol] = std::move(info);
}

const LowerCtx::ProcSigInfo* LowerCtx::LookupProcSig(const std::string& sym) const {
  auto it = proc_sigs.find(sym);
  if (it != proc_sigs.end()) {
    return &it->second;
  }
  return nullptr;
}

void LowerCtx::RegisterProcModule(const std::string& sym, const syntax::ModulePath& module_path) {
  proc_modules[sym] = module_path;
}

const std::vector<std::string>* LowerCtx::LookupProcModule(const std::string& sym) const {
  auto it = proc_modules.find(sym);
  if (it != proc_modules.end()) {
    return &it->second;
  }
  return nullptr;
}

}  // namespace cursive0::codegen
