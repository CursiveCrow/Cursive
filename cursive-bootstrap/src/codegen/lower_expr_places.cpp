#include "cursive0/codegen/lower_expr.h"
#include "cursive0/codegen/checks.h"
#include "cursive0/codegen/cleanup.h"
#include "cursive0/codegen/globals.h"
#include "cursive0/core/assert_spec.h"
#include "cursive0/sema/scopes.h"

#include <cassert>
#include <algorithm>
#include <variant>

namespace cursive0::codegen {

namespace {

IRRange ToIRRange(const RangeVal& range) {
  IRRange out;
  out.kind = range.kind;
  out.lo = range.lo;
  out.hi = range.hi;
  return out;
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

std::optional<std::string> PlaceRoot(const syntax::Expr& expr) {
  return std::visit(
      [&](const auto& node) -> std::optional<std::string> {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
          return node.name;
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
          return PlaceRoot(*node.base);
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
          return PlaceRoot(*node.base);
        } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
          return PlaceRoot(*node.base);
        } else if constexpr (std::is_same_v<T, syntax::DerefExpr>) {
          return PlaceRoot(*node.value);
        }
        return std::nullopt;
      },
      expr.node);
}

std::optional<std::string> FieldHead(const syntax::Expr& expr) {
  return std::visit(
      [&](const auto& node) -> std::optional<std::string> {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
          return std::nullopt;
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
          auto head = FieldHead(*node.base);
          if (head.has_value()) {
            return head;
          }
          return node.name;
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
          return FieldHead(*node.base);
        } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
          return FieldHead(*node.base);
        } else if constexpr (std::is_same_v<T, syntax::DerefExpr>) {
          return std::nullopt;
        }
        return std::nullopt;
      },
      expr.node);
}


static void UpdateBindingAfterFieldAssign(const syntax::Expr& place,
                                          LowerCtx& ctx) {
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

std::string FormatRangeBound(const syntax::ExprPtr& expr) {
  if (!expr) {
    return "";
  }
  if (const auto* lit = std::get_if<syntax::LiteralExpr>(&expr->node)) {
    return lit->literal.lexeme;
  }
  return "?";
}

std::string FormatRangeExpr(const syntax::RangeExpr& expr) {
  const std::string lo = FormatRangeBound(expr.lhs);
  const std::string hi = FormatRangeBound(expr.rhs);
  switch (expr.kind) {
    case syntax::RangeKind::Full:
      return "..";
    case syntax::RangeKind::From:
      return lo + "..";
    case syntax::RangeKind::To:
      return ".." + hi;
    case syntax::RangeKind::ToInclusive:
      return "..=" + hi;
    case syntax::RangeKind::Exclusive:
      return lo + ".." + hi;
    case syntax::RangeKind::Inclusive:
      return lo + "..=" + hi;
  }
  return "..";
}

std::string FormatIndexExpr(const syntax::Expr& expr) {
  if (const auto* lit = std::get_if<syntax::LiteralExpr>(&expr.node)) {
    return lit->literal.lexeme;
  }
  if (const auto* range = std::get_if<syntax::RangeExpr>(&expr.node)) {
    return FormatRangeExpr(*range);
  }
  return "?";
}

std::string BuildPlaceRepr(const syntax::Expr& expr) {
  return std::visit(
      [&](const auto& node) -> std::string {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
          return node.name;
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
          std::string base = BuildPlaceRepr(*node.base);
          if (base.empty()) {
            return node.name;
          }
          return base + "." + node.name;
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
          std::string base = BuildPlaceRepr(*node.base);
          std::string idx = node.index.lexeme;
          if (base.empty()) {
            return idx;
          }
          return base + "." + idx;
        } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
          std::string base = BuildPlaceRepr(*node.base);
          std::string idx = FormatIndexExpr(*node.index);
          if (base.empty()) {
            return "[" + idx + "]";
          }
          return base + "[" + idx + "]";
        } else if constexpr (std::is_same_v<T, syntax::DerefExpr>) {
          return "*" + BuildPlaceRepr(*node.value);
        }
        return "";
      },
      expr.node);
}

}  // namespace

// ============================================================================
// §6.4 Place Lowering
// ============================================================================

// §6.4 LowerPlace - lower a place to its representation
IRPlace LowerPlace(const syntax::Expr& place, LowerCtx& /*ctx*/) {
  SPEC_RULE("Lower-Place-Ident");
  SPEC_RULE("Lower-Place-Field");
  SPEC_RULE("Lower-Place-Tuple");
  SPEC_RULE("Lower-Place-Index");
  SPEC_RULE("Lower-Place-Deref");

  IRPlace result;
  result.repr = BuildPlaceRepr(place);
  return result;
}

// ============================================================================
// §6.4 LowerReadPlace - Read from a place
// ============================================================================

LowerResult LowerReadPlace(const syntax::Expr& place, LowerCtx& ctx) {
  return std::visit(
      [&](const auto& node) -> LowerResult {
        using T = std::decay_t<decltype(node)>;

        if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
          if (ctx.GetBindingState(node.name)) {
            SPEC_RULE("Lower-ReadPlace-Ident-Local");
            IRReadVar read;
            read.name = node.name;
            IRValue value;
            value.kind = IRValue::Kind::Local;
            value.name = node.name;
            return LowerResult{MakeIR(std::move(read)), value};
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
          return LowerResult{MakeIR(std::move(read)), value};
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
          SPEC_RULE("Lower-ReadPlace-Field");
          auto base_result = LowerReadPlace(*node.base, ctx);
          IRValue field_value;
          field_value.kind = IRValue::Kind::Opaque;
          field_value.name = base_result.value.name + "." + node.name;
          return LowerResult{base_result.ir, field_value};
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
          SPEC_RULE("Lower-ReadPlace-Tuple");
          auto base_result = LowerReadPlace(*node.base, ctx);
          IRValue elem_value;
          elem_value.kind = IRValue::Kind::Opaque;
          elem_value.name = base_result.value.name + "." + node.index.lexeme;
          return LowerResult{base_result.ir, elem_value};
        } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
          auto base_result = LowerReadPlace(*node.base, ctx);

          if (std::holds_alternative<syntax::RangeExpr>(node.index->node)) {
            SPEC_RULE("Lower-ReadPlace-Index-Range");
            const auto& range_node = std::get<syntax::RangeExpr>(node.index->node);
            auto range_result = LowerRangeExpr(range_node, ctx);

            IRCheckRange check;
            check.base = base_result.value;
            check.range = ToIRRange(range_result.value);

            IRValue slice_value;
            slice_value.kind = IRValue::Kind::Opaque;
            slice_value.name = base_result.value.name + "[range]";

            return LowerResult{SeqIR({base_result.ir, range_result.ir,
                                      MakeIR(std::move(check))}),
                               slice_value};
          }

          SPEC_RULE("Lower-ReadPlace-Index-Scalar");
          auto index_result = LowerExpr(*node.index, ctx);

          IRCheckIndex check;
          check.base = base_result.value;
          check.index = index_result.value;

          IRValue elem_value;
          elem_value.kind = IRValue::Kind::Opaque;
          elem_value.name = base_result.value.name + "[idx]";

          return LowerResult{SeqIR({base_result.ir, index_result.ir,
                                    MakeIR(std::move(check))}),
                             elem_value};
        } else if constexpr (std::is_same_v<T, syntax::DerefExpr>) {
          SPEC_RULE("Lower-ReadPlace-Deref");
          auto ptr_result = LowerReadPlace(*node.value, ctx);
          sema::TypeRef ptr_type;
          if (ctx.expr_type) {
            ptr_type = ctx.expr_type(*node.value);
          }
          if (ptr_type) {
            auto deref_result = LowerRawDeref(ptr_result.value, ptr_type, ctx);
            return LowerResult{SeqIR({ptr_result.ir, deref_result.ir}),
                               deref_result.value};
          }
          IRReadPtr read;
          read.ptr = ptr_result.value;
          IRValue value;
          value.kind = IRValue::Kind::Opaque;
          value.name = "deref_result";
          return LowerResult{SeqIR({ptr_result.ir, MakeIR(std::move(read))}), value};
        }

        IRValue value;
        value.kind = IRValue::Kind::Opaque;
        value.name = "place_read";
        return LowerResult{EmptyIR(), value};
      },
      place.node);
}

// ============================================================================
// §6.4 LowerWritePlace - Write to a place
// ============================================================================

namespace {

IRPtr LowerWritePlaceImpl(const syntax::Expr& place,
                          const IRValue& value,
                          LowerCtx& ctx,
                          bool allow_drop) {
  return std::visit(
      [&](const auto& node) -> IRPtr {
        using T = std::decay_t<decltype(node)>;

        if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
          if (ctx.GetBindingState(node.name)) {
            SPEC_RULE(allow_drop ? "Lower-WritePlace-Ident-Local"
                                 : "LowerWriteSub-Ident-Local");
            if (allow_drop) {
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
            store_nodrop.name = node.name;
            store_nodrop.value = value;
            return MakeIR(std::move(store_nodrop));
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
          if (full == ctx.module_path) {
            IRCheckPoison check;
            check.module = ModulePathString(full);
            poison_ir = MakeIR(std::move(check));
          }

          IRPtr drop_ir = EmptyIR();
          if (allow_drop) {
            if (auto flags = StaticBindFlagsFor(full, resolved_name, ctx)) {
              if (flags->has_responsibility) {
                sema::TypeRef static_type;
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
          }
          if (!is_noop(drop_ir)) {
            parts.push_back(drop_ir);
          }
          parts.push_back(MakeIR(std::move(store)));

          if (parts.size() == 1) {
            return parts.front();
          }
          return SeqIR(std::move(parts));
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
          SPEC_RULE(allow_drop ? "Lower-WritePlace-Field"
                               : "LowerWriteSub-Field");
          auto base_result = LowerReadPlace(*node.base, ctx);

          IRPtr drop_ir = EmptyIR();
          if (allow_drop && DropOnAssignRoot(*node.base, ctx)) {
            sema::TypeRef field_type;
            if (ctx.expr_type) {
              field_type = ctx.expr_type(place);
            }
            IRValue field_value;
            field_value.kind = IRValue::Kind::Opaque;
            field_value.name = base_result.value.name + "." + node.name;
            drop_ir = EmitDrop(field_type, field_value, ctx);
          }

          IRWritePlace write;
          write.place = LowerPlace(place, ctx);
          write.value = value;

          if (allow_drop) {
            UpdateBindingAfterFieldAssign(place, ctx);
          }

          return SeqIR({base_result.ir, drop_ir, MakeIR(std::move(write))});
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
          SPEC_RULE(allow_drop ? "Lower-WritePlace-Tuple"
                               : "LowerWriteSub-Tuple");
          auto base_result = LowerReadPlace(*node.base, ctx);

          IRPtr drop_ir = EmptyIR();
          if (allow_drop && DropOnAssignRoot(*node.base, ctx)) {
            sema::TypeRef elem_type;
            if (ctx.expr_type) {
              elem_type = ctx.expr_type(place);
            }
            IRValue elem_value;
            elem_value.kind = IRValue::Kind::Opaque;
            elem_value.name = base_result.value.name + "." +
                              node.index.lexeme;
            drop_ir = EmitDrop(elem_type, elem_value, ctx);
          }

          IRWritePlace write;
          write.place = LowerPlace(place, ctx);
          write.value = value;

          return SeqIR({base_result.ir, drop_ir, MakeIR(std::move(write))});
        } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
          auto base_result = LowerReadPlace(*node.base, ctx);

          if (std::holds_alternative<syntax::RangeExpr>(node.index->node)) {
            SPEC_RULE(allow_drop ? "Lower-WritePlace-Index-Range"
                                 : "LowerWriteSub-Index-Range");
            const auto& range_node = std::get<syntax::RangeExpr>(node.index->node);
            auto range_result = LowerRangeExpr(range_node, ctx);

            IRCheckRange check;
            check.base = base_result.value;
            check.range = ToIRRange(range_result.value);

            IRCheckSliceLen len_check;
            len_check.base = base_result.value;
            len_check.range = ToIRRange(range_result.value);
            len_check.value = value;

            IRWritePlace write;
            write.place = LowerPlace(place, ctx);
            write.value = value;

            return SeqIR({base_result.ir, range_result.ir,
                          MakeIR(std::move(check)),
                          MakeIR(std::move(len_check)),
                          MakeIR(std::move(write))});
          }

          SPEC_RULE(allow_drop ? "Lower-WritePlace-Index-Scalar"
                               : "LowerWriteSub-Index-Scalar");
          auto index_result = LowerExpr(*node.index, ctx);

          IRCheckIndex check;
          check.base = base_result.value;
          check.index = index_result.value;

          IRPtr drop_ir = EmptyIR();
          if (allow_drop && DropOnAssignRoot(*node.base, ctx)) {
            sema::TypeRef elem_type;
            if (ctx.expr_type) {
              elem_type = ctx.expr_type(place);
            }
            IRValue elem_value;
            elem_value.kind = IRValue::Kind::Opaque;
            elem_value.name = base_result.value.name + "[idx]";
            drop_ir = EmitDrop(elem_type, elem_value, ctx);
          }

          IRWritePlace write;
          write.place = LowerPlace(place, ctx);
          write.value = value;

          return SeqIR({base_result.ir, index_result.ir,
                        MakeIR(std::move(check)),
                        drop_ir,
                        MakeIR(std::move(write))});
        } else if constexpr (std::is_same_v<T, syntax::DerefExpr>) {
          SPEC_RULE(allow_drop ? "Lower-WritePlace-Deref"
                               : "LowerWriteSub-Deref");
          auto ptr_result = LowerReadPlace(*node.value, ctx);
          IRWritePtr write;
          write.ptr = ptr_result.value;
          write.value = value;
          return SeqIR({ptr_result.ir, MakeIR(std::move(write))});
        }

        return EmptyIR();
      },
      place.node);
}

}  // namespace

IRPtr LowerWritePlace(const syntax::Expr& place,
                      const IRValue& value,
                      LowerCtx& ctx) {
  return LowerWritePlaceImpl(place, value, ctx, true);
}

IRPtr LowerWritePlaceSub(const syntax::Expr& place,
                         const IRValue& value,
                         LowerCtx& ctx) {
  return LowerWritePlaceImpl(place, value, ctx, false);
}

// ============================================================================
// §6.4 LowerAddrOf - Get address of a place
// ============================================================================

LowerResult LowerAddrOf(const syntax::Expr& place, LowerCtx& ctx) {
  SPEC_RULE("Lower-AddrOf-Ident-Local");
  SPEC_RULE("Lower-AddrOf-Ident-Path");
  SPEC_RULE("Lower-AddrOf-Field");
  SPEC_RULE("Lower-AddrOf-Tuple");
  SPEC_RULE("Lower-AddrOf-Index");
  SPEC_RULE("Lower-AddrOf-Index-OOB");
  SPEC_RULE("Lower-AddrOf-Deref");
  SPEC_RULE("Lower-AddrOf-Deref-Null");
  SPEC_RULE("Lower-AddrOf-Deref-Expired");
  SPEC_RULE("Lower-AddrOf-Deref-Raw");

  return std::visit(
      [&](const auto& node) -> LowerResult {
        using T = std::decay_t<decltype(node)>;

        if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
          IRAddrOf addr;
          addr.place = LowerPlace(place, ctx);

          IRValue ptr_value;
          ptr_value.kind = IRValue::Kind::Opaque;
          ptr_value.name = "addr_of";

          if (ctx.GetBindingState(node.name)) {
            SPEC_RULE("Lower-AddrOf-Ident-Local");
            return LowerResult{MakeIR(std::move(addr)), ptr_value};
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

          SPEC_RULE("Lower-AddrOf-Ident-Path");
          IRPtr poison_ir = EmptyIR();
          if (full == ctx.module_path) {
            IRCheckPoison check;
            check.module = ModulePathString(full);
            poison_ir = MakeIR(std::move(check));
          }
          if (poison_ir && !std::holds_alternative<IROpaque>(poison_ir->node)) {
            return LowerResult{SeqIR({poison_ir, MakeIR(std::move(addr))}), ptr_value};
          }
          return LowerResult{MakeIR(std::move(addr)), ptr_value};
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
          auto base_result = LowerReadPlace(*node.base, ctx);
          IRAddrOf addr;
          addr.place = LowerPlace(place, ctx);
          IRValue ptr_value{IRValue::Kind::Opaque, "addr_of", {}};
          return LowerResult{SeqIR({base_result.ir, MakeIR(std::move(addr))}),
                             ptr_value};
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
          auto base_result = LowerReadPlace(*node.base, ctx);
          IRAddrOf addr;
          addr.place = LowerPlace(place, ctx);
          IRValue ptr_value{IRValue::Kind::Opaque, "addr_of", {}};
          return LowerResult{SeqIR({base_result.ir, MakeIR(std::move(addr))}),
                             ptr_value};
        }

        IRAddrOf addr;
        addr.place = LowerPlace(place, ctx);
        IRValue ptr_value{IRValue::Kind::Opaque, "addr_of", {}};
        return LowerResult{MakeIR(std::move(addr)), ptr_value};
      },
      place.node);
}

// ============================================================================
// §6.4 LowerMovePlace - Move value out of a place
// ============================================================================

LowerResult LowerMovePlace(const syntax::Expr& place, LowerCtx& ctx) {
  SPEC_RULE("Lower-MovePlace");

  auto read_result = LowerReadPlace(place, ctx);

  if (auto root = PlaceRoot(place)) {
    if (auto head = FieldHead(place)) {
      ctx.MarkFieldMoved(*root, *head);
    } else {
      ctx.MarkMoved(*root);
    }
  }

  IRPlace ir_place = LowerPlace(place, ctx);
  IRMoveState move_state;
  move_state.place = ir_place;

  return LowerResult{SeqIR({read_result.ir, MakeIR(std::move(move_state))}),
                     read_result.value};
}

// ============================================================================
// Anchor function
// ============================================================================

void AnchorPlacesRules() {
  SPEC_RULE("Lower-Place-Ident");
  SPEC_RULE("Lower-Place-Field");
  SPEC_RULE("Lower-Place-Tuple");
  SPEC_RULE("Lower-Place-Index");
  SPEC_RULE("Lower-Place-Deref");
  SPEC_RULE("Lower-ReadPlace-Ident-Local");
  SPEC_RULE("Lower-ReadPlace-Ident-Path");
  SPEC_RULE("Lower-ReadPlace-Field");
  SPEC_RULE("Lower-ReadPlace-Tuple");
  SPEC_RULE("Lower-ReadPlace-Index-Scalar");
  SPEC_RULE("Lower-ReadPlace-Index-Scalar-OOB");
  SPEC_RULE("Lower-ReadPlace-Index-Range");
  SPEC_RULE("Lower-ReadPlace-Index-Range-OOB");
  SPEC_RULE("Lower-ReadPlace-Deref");
  SPEC_RULE("Lower-WritePlace-Ident-Local");
  SPEC_RULE("Lower-WritePlace-Ident-Path");
  SPEC_RULE("Lower-WritePlace-Field");
  SPEC_RULE("Lower-WritePlace-Tuple");
  SPEC_RULE("Lower-WritePlace-Index-Scalar");
  SPEC_RULE("Lower-WritePlace-Index-Scalar-OOB");
  SPEC_RULE("Lower-WritePlace-Index-Range");
  SPEC_RULE("Lower-WritePlace-Index-Range-OOB");
  SPEC_RULE("Lower-WritePlace-Index-Range-Len");
  SPEC_RULE("Lower-WritePlace-Deref");
  SPEC_RULE("LowerWriteSub-Ident-Local");
  SPEC_RULE("LowerWriteSub-Ident-Path");
  SPEC_RULE("LowerWriteSub-Field");
  SPEC_RULE("LowerWriteSub-Tuple");
  SPEC_RULE("LowerWriteSub-Index-Scalar");
  SPEC_RULE("LowerWriteSub-Index-Scalar-OOB");
  SPEC_RULE("LowerWriteSub-Index-Range");
  SPEC_RULE("LowerWriteSub-Index-Range-OOB");
  SPEC_RULE("LowerWriteSub-Index-Range-Len");
  SPEC_RULE("LowerWriteSub-Deref");
  SPEC_RULE("Lower-AddrOf-Ident-Local");
  SPEC_RULE("Lower-AddrOf-Ident-Path");
  SPEC_RULE("Lower-AddrOf-Field");
  SPEC_RULE("Lower-AddrOf-Tuple");
  SPEC_RULE("Lower-AddrOf-Index");
  SPEC_RULE("Lower-AddrOf-Index-OOB");
  SPEC_RULE("Lower-AddrOf-Deref");
  SPEC_RULE("Lower-AddrOf-Deref-Null");
  SPEC_RULE("Lower-AddrOf-Deref-Expired");
  SPEC_RULE("Lower-AddrOf-Deref-Raw");
  SPEC_RULE("Lower-MovePlace");
}

}  // namespace cursive0::codegen
