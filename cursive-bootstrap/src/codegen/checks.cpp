#include "cursive0/codegen/checks.h"
#include "cursive0/codegen/layout/layout.h"
#include "cursive0/codegen/lower/lower_expr.h"
#include "cursive0/codegen/cleanup.h"
#include "cursive0/runtime/runtime_interface.h"
#include "cursive0/core/assert_spec.h"
#include "cursive0/core/symbols.h"

#include <cassert>
#include <limits>
#include <unordered_map>
#include <vector>

namespace cursive0::codegen {
namespace {

std::vector<std::string> PoisonSetFor(const std::string& module_path,
                                      const LowerCtx& ctx) {
  std::vector<std::string> out;
  if (ctx.init_modules.empty()) {
    out.push_back(module_path);
    return out;
  }

  std::unordered_map<std::string, std::size_t> index;
  index.reserve(ctx.init_modules.size());
  for (std::size_t i = 0; i < ctx.init_modules.size(); ++i) {
    index.emplace(core::StringOfPath(ctx.init_modules[i]), i);
  }

  const auto it = index.find(module_path);
  if (it == index.end()) {
    out.push_back(module_path);
    return out;
  }

  const std::size_t target = it->second;
  const std::size_t n = ctx.init_modules.size();
  std::vector<std::vector<std::size_t>> outgoing(n);
  for (const auto& edge : ctx.init_eager_edges) {
    if (edge.first < n && edge.second < n) {
      outgoing[edge.first].push_back(edge.second);
    }
  }

  std::vector<char> visited(n, false);
  std::vector<std::size_t> stack;
  visited[target] = true;
  stack.push_back(target);
  while (!stack.empty()) {
    const std::size_t cur = stack.back();
    stack.pop_back();
    for (const auto succ : outgoing[cur]) {
      if (!visited[succ]) {
        visited[succ] = true;
        stack.push_back(succ);
      }
    }
  }

  out.reserve(n);
  for (std::size_t i = 0; i < n; ++i) {
    if (visited[i]) {
      out.push_back(core::StringOfPath(ctx.init_modules[i]));
    }
  }
  if (out.empty()) {
    out.push_back(module_path);
  }
  return out;
}

IRValue StringImmediate(std::string_view text) {
  IRValue value;
  value.kind = IRValue::Kind::Immediate;
  value.name = "\"" + std::string(text) + "\"";
  value.bytes.assign(text.begin(), text.end());
  return value;
}

IRPtr EmitRuntimeTrace(std::string_view rule_id) {
  if (rule_id.empty()) {
    return EmptyIR();
  }
  IRCall call;
  call.callee.kind = IRValue::Kind::Symbol;
  call.callee.name = RuntimeSpecTraceEmitSym();
  call.args.push_back(StringImmediate(rule_id));
  call.args.push_back(StringImmediate(""));
  return MakeIR(std::move(call));
}

}  // namespace


// §6.8 PanicCode mapping
std::uint16_t PanicCode(PanicReason reason) {
  SPEC_RULE("PanicCode");
  switch (reason) {
    case PanicReason::ErrorExpr:
      return 0x0001;
    case PanicReason::ErrorStmt:
      return 0x0002;
    case PanicReason::DivZero:
      return 0x0003;
    case PanicReason::Overflow:
      return 0x0004;
    case PanicReason::Shift:
      return 0x0005;
    case PanicReason::Bounds:
      return 0x0006;
    case PanicReason::Cast:
      return 0x0007;
    case PanicReason::NullDeref:
      return 0x0008;
    case PanicReason::ExpiredDeref:
      return 0x0009;
    case PanicReason::InitPanic:
      return 0x000A;
    case PanicReason::Other:
    default:
      return 0x00FF;
  }
}

std::string PanicReasonString(PanicReason reason) {
  switch (reason) {
    case PanicReason::ErrorExpr:
      return "ErrorExpr";
    case PanicReason::ErrorStmt:
      return "ErrorStmt";
    case PanicReason::DivZero:
      return "DivZero";
    case PanicReason::Overflow:
      return "Overflow";
    case PanicReason::Shift:
      return "Shift";
    case PanicReason::Bounds:
      return "Bounds";
    case PanicReason::Cast:
      return "Cast";
    case PanicReason::NullDeref:
      return "NullDeref";
    case PanicReason::ExpiredDeref:
      return "ExpiredDeref";
    case PanicReason::InitPanic:
      return "InitPanic";
    case PanicReason::Other:
    default:
      return "Other";
  }
}

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

std::optional<std::uint64_t> ParseImmediateU64(const IRValue& value) {
  if (value.kind != IRValue::Kind::Immediate) {
    return std::nullopt;
  }
  if (!value.bytes.empty()) {
    if (value.bytes.size() > sizeof(std::uint64_t)) {
      return std::nullopt;
    }
    std::uint64_t out = 0;
    for (std::uint8_t b : value.bytes) {
      out = (out << 8) | static_cast<std::uint64_t>(b);
    }
    return out;
  }
  std::string text = StripIntSuffix(value.name);
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

std::optional<std::uint64_t> OptImmediateU64(const std::optional<IRValue>& value) {
  if (!value.has_value()) {
    return std::nullopt;
  }
  return ParseImmediateU64(*value);
}

}  // namespace

// §6.11 Check-Index-Ok / Check-Index-Err
bool CheckIndex(std::uint64_t len, std::uint64_t idx) {
  SPEC_RULE("Check-Index-Ok");
  SPEC_RULE("Check-Index-Err");
  return idx < len;
}

// §6.11 Check-Range-Ok / Check-Range-Err
bool CheckRange(std::uint64_t len, const RangeVal& range) {
  SPEC_RULE("Check-Range-Ok");
  SPEC_RULE("Check-Range-Err");
  return SliceBounds(range, len).has_value();
}

// §6.11 SliceBounds computation
std::optional<std::pair<std::uint64_t, std::uint64_t>> SliceBounds(
    const RangeVal& range, std::uint64_t len) {
  std::uint64_t start = 0;
  std::uint64_t end = len;

  switch (range.kind) {
    case syntax::RangeKind::Full:
      // Full range: 0..len
      start = 0;
      end = len;
      break;

    case syntax::RangeKind::From: {
      // start..
      const auto lo = OptImmediateU64(range.lo);
      if (!lo.has_value()) {
        return std::nullopt;
      }
      start = *lo;
      end = len;
      break;
    }

    case syntax::RangeKind::To: {
      // ..end (exclusive)
      const auto hi = OptImmediateU64(range.hi);
      if (!hi.has_value()) {
        return std::nullopt;
      }
      start = 0;
      end = *hi;
      break;
    }

    case syntax::RangeKind::ToInclusive: {
      // ..=end (inclusive)
      const auto hi = OptImmediateU64(range.hi);
      if (!hi.has_value()) {
        return std::nullopt;
      }
      start = 0;
      if (*hi >= len) {
        return std::nullopt;
      }
      end = *hi + 1;
      break;
    }

    case syntax::RangeKind::Exclusive: {
      // start..end (exclusive)
      const auto lo = OptImmediateU64(range.lo);
      const auto hi = OptImmediateU64(range.hi);
      if (!lo.has_value() || !hi.has_value()) {
        return std::nullopt;
      }
      start = *lo;
      end = *hi;
      break;
    }

    case syntax::RangeKind::Inclusive: {
      // start..=end (inclusive)
      const auto lo = OptImmediateU64(range.lo);
      const auto hi = OptImmediateU64(range.hi);
      if (!lo.has_value() || !hi.has_value()) {
        return std::nullopt;
      }
      start = *lo;
      if (*hi == std::numeric_limits<std::uint64_t>::max()) {
        return std::nullopt;
      }
      end = *hi + 1;
      break;
    }
  }

  if (start > end || end > len) {
    return std::nullopt;
  }

  return std::make_pair(start, end);
}

// §6.8 PanicSym - runtime panic handler symbol
std::string PanicSym() {
  SPEC_RULE("PanicSym");
  return RuntimePanicSym();
}

// MakeIR is now in lower_expr.h

// §6.8 LowerPanic - emit panic IR
IRPtr LowerPanic(PanicReason reason, LowerCtx& ctx) {
  SPEC_RULE("LowerPanic");
  IRLowerPanic panic_ir;
  panic_ir.reason = PanicReasonString(reason);
  CleanupPlan cleanup_plan = ComputeCleanupPlanToFunctionRoot(ctx);
  panic_ir.cleanup_ir = EmitCleanupOnPanic(cleanup_plan, ctx);
  IRPtr trace_ir = EmitRuntimeTrace("LowerPanic");
  IRPtr panic_node = MakeIR(panic_ir);
  return SeqIR({trace_ir, panic_node});
}

// §6.8 ClearPanic - emit IR to clear panic flag
IRPtr ClearPanic(LowerCtx& /*ctx*/) {
  SPEC_RULE("ClearPanic");
  return MakeIR(IRClearPanic{});
}

// §6.8 PanicCheck - emit IR to check panic after call
IRPtr PanicCheck(LowerCtx& ctx) {
  SPEC_RULE("PanicCheck");
  IRPanicCheck check;
  CleanupPlan cleanup_plan = ComputeCleanupPlanToFunctionRoot(ctx);
  check.cleanup_ir = EmitCleanupOnPanic(cleanup_plan, ctx);
  IRPtr trace_ir = EmitRuntimeTrace("PanicCheck");
  IRPtr check_node = MakeIR(check);
  return SeqIR({trace_ir, check_node});
}

// §6.8 InitPanicHandle - module init panic handling
IRPtr InitPanicHandle(const std::string& module_path, LowerCtx& ctx) {
  SPEC_RULE("InitPanicHandle");
  IRInitPanicHandle handle;
  handle.module = module_path;
  handle.poison_modules = PoisonSetFor(module_path, ctx);
  IRPtr trace_ir = EmitRuntimeTrace("InitPanicHandle");
  IRPtr handle_node = MakeIR(handle);
  return SeqIR({trace_ir, handle_node});
}

// Helper to create a sequence of IR nodes
// SeqIR moved to ir_model.h

// §6.11 Lower-Range-* rules
LowerRangeResult LowerRangeExpr(const syntax::RangeExpr& expr, LowerCtx& ctx) {
  RangeVal value;
  value.kind = expr.kind;
  std::vector<IRPtr> ir_parts;

  auto lower_opt = [&](const syntax::ExprPtr& opt, std::optional<IRValue>& out) {
    if (!opt) {
      return;
    }
    auto result = LowerExpr(*opt, ctx);
    ir_parts.push_back(result.ir);
    out = result.value;
  };

  switch (expr.kind) {
    case syntax::RangeKind::Full:
      // Lower-Range-Full: no subexpressions
      SPEC_RULE("Lower-Range-Full");
      break;
    case syntax::RangeKind::From:
      SPEC_RULE("Lower-Range-From");
      lower_opt(expr.lhs, value.lo);
      break;
    case syntax::RangeKind::To:
      SPEC_RULE("Lower-Range-To");
      lower_opt(expr.rhs, value.hi);
      break;
    case syntax::RangeKind::ToInclusive:
      SPEC_RULE("Lower-Range-ToInclusive");
      lower_opt(expr.rhs, value.hi);
      break;
    case syntax::RangeKind::Exclusive:
      SPEC_RULE("Lower-Range-Exclusive");
      lower_opt(expr.lhs, value.lo);
      lower_opt(expr.rhs, value.hi);
      break;
    case syntax::RangeKind::Inclusive:
      SPEC_RULE("Lower-Range-Inclusive");
      lower_opt(expr.lhs, value.lo);
      lower_opt(expr.rhs, value.hi);
      break;
  }

  return LowerRangeResult{SeqIR(std::move(ir_parts)), value};
}

// §6.11 Lower-Transmute

LowerResult LowerTransmute(analysis::TypeRef from_type,
                           analysis::TypeRef to_type,
                           const syntax::Expr& expr,
                           LowerCtx& ctx) {
  SPEC_RULE("Lower-Transmute");
  SPEC_RULE("Lower-Transmute-Err");

  auto expr_result = LowerExpr(expr, ctx);

  if (!from_type && ctx.expr_type) {
    from_type = ctx.expr_type(expr);
  }

  if (!to_type && from_type) {
    to_type = from_type;
  }

  std::optional<std::uint64_t> from_size;
  std::optional<std::uint64_t> to_size;
  if (ctx.sigma && from_type && to_type) {
    analysis::ScopeContext scope;
    scope.sigma = *ctx.sigma;
    from_size = SizeOf(scope, from_type);
    to_size = SizeOf(scope, to_type);
  }

  if (from_size.has_value() && to_size.has_value() && *from_size != *to_size) {
    IRValue unreachable = ctx.FreshTempValue("unreachable");
    return LowerResult{SeqIR({expr_result.ir, LowerPanic(PanicReason::Cast, ctx)}), unreachable};
  }

  IRValue result = ctx.FreshTempValue("transmute");

  IRTransmute transmute;
  transmute.from = from_type;
  transmute.to = to_type;
  transmute.value = expr_result.value;
  transmute.result = result;

  return LowerResult{SeqIR({expr_result.ir, MakeIR(std::move(transmute))}), result};
}

// §6.11 Lower-RawDeref
LowerResult LowerRawDeref(const IRValue& ptr_value,
                          analysis::TypeRef ptr_type,
                          LowerCtx& ctx) {
  // Determine pointer state from type
  auto* ptr = std::get_if<analysis::TypePtr>(&ptr_type->node);
  auto* raw_ptr = std::get_if<analysis::TypeRawPtr>(&ptr_type->node);

  analysis::TypeRef elem_type;
  if (raw_ptr != nullptr) {
    elem_type = raw_ptr->element;
  } else if (ptr != nullptr) {
    elem_type = ptr->element;
  }

  IRValue result = ctx.FreshTempValue("deref");

  if (raw_ptr != nullptr) {
    // Lower-RawDeref-Raw: raw pointer dereference (unchecked)
    SPEC_RULE("Lower-RawDeref-Raw");
    IRReadPtr read;
    read.ptr = ptr_value;
    read.result = result;
    ctx.RegisterValueType(result, elem_type);
    return LowerResult{MakeIR(read), result};
  }

  if (ptr != nullptr) {
    if (ptr->state.has_value()) {
      switch (*ptr->state) {
        case analysis::PtrState::Valid:
          // Lower-RawDeref-Safe: valid pointer
          SPEC_RULE("Lower-RawDeref-Safe");
          {
            IRReadPtr read;
            read.ptr = ptr_value;
            read.result = result;
            ctx.RegisterValueType(result, elem_type);
            return LowerResult{MakeIR(read), result};
          }

        case analysis::PtrState::Null:
          // Lower-RawDeref-Null: emit panic
          SPEC_RULE("Lower-RawDeref-Null");
          {
            result = ctx.FreshTempValue("unreachable");
            return LowerResult{LowerPanic(PanicReason::NullDeref, ctx), result};
          }

        case analysis::PtrState::Expired:
          // Lower-RawDeref-Expired: emit panic
          SPEC_RULE("Lower-RawDeref-Expired");
          {
            result = ctx.FreshTempValue("unreachable");
            return LowerResult{LowerPanic(PanicReason::ExpiredDeref, ctx),
                               result};
          }
      }
    }

    // No state specified - treat as potentially valid
    SPEC_RULE("Lower-RawDeref-Safe");
    IRReadPtr read;
    read.ptr = ptr_value;
    read.result = result;
    ctx.RegisterValueType(result, elem_type);
    return LowerResult{MakeIR(read), result};
  }

  // Fallback for other types (shouldn't happen in well-typed code)
  return LowerResult{MakeIR(IROpaque{}), result};
}

// Emit SPEC_RULE anchors for coverage
void AnchorChecksAndPanicRules() {
  // §6.8 Cleanup, Drop, and Unwinding
  SPEC_RULE("PanicCode");
  SPEC_RULE("PanicSym");
  SPEC_RULE("LowerPanic");
  SPEC_RULE("ClearPanic");
  SPEC_RULE("PanicCheck");
  SPEC_RULE("InitPanicHandle");

  // §6.11 Checks and Panic
  SPEC_RULE("Lower-Range-Full");
  SPEC_RULE("Lower-Range-To");
  SPEC_RULE("Lower-Range-ToInclusive");
  SPEC_RULE("Lower-Range-From");
  SPEC_RULE("Lower-Range-Inclusive");
  SPEC_RULE("Lower-Range-Exclusive");
  SPEC_RULE("Check-Index-Ok");
  SPEC_RULE("Check-Index-Err");
  SPEC_RULE("Check-Range-Ok");
  SPEC_RULE("Check-Range-Err");
  SPEC_RULE("Lower-Transmute");
  SPEC_RULE("Lower-Transmute-Err");
  SPEC_RULE("Lower-RawDeref-Safe");
  SPEC_RULE("Lower-RawDeref-Raw");
  SPEC_RULE("Lower-RawDeref-Null");
  SPEC_RULE("Lower-RawDeref-Expired");
}

}  // namespace cursive0::codegen
