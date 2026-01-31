#include "cursive0/04_codegen/lower/lower_expr.h"
#include "cursive0/04_codegen/checks.h"
#include "cursive0/04_codegen/cleanup.h"
#include "cursive0/04_codegen/globals.h"
#include "cursive0/runtime/runtime_interface.h"
#include "cursive0/00_core/assert_spec.h"
#include "cursive0/03_analysis/resolve/scopes.h"
#include "cursive0/03_analysis/types/type_expr.h"

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

bool NeedsIndexCheck(const syntax::Expr& base, const LowerCtx& ctx) {
  if (!ctx.expr_type) {
    return true;
  }
  analysis::TypeRef base_type = ctx.expr_type(base);
  analysis::TypeRef stripped = analysis::StripPerm(base_type);
  if (stripped && std::holds_alternative<analysis::TypeArray>(stripped->node)) {
    return ctx.dynamic_checks;
  }
  return true;
}

bool HasDynamicAttr(const syntax::AttributeList& attrs) {
  for (const auto& attr : attrs) {
    if (attr.name == "dynamic") {
      return true;
    }
  }
  return false;
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
        if constexpr (std::is_same_v<T, syntax::AttributedExpr>) {
          return node.expr ? IsPlaceExprLite(node.expr) : false;
        } else if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
          return true;
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
          return IsPlaceExprLite(node.base);
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
          return IsPlaceExprLite(node.base);
        } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
          return IsPlaceExprLite(node.base);
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
    if (analysis::PathEq(mod.path, module_path)) {
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
        if constexpr (std::is_same_v<T, syntax::AttributedExpr>) {
          return node.expr ? PlaceRoot(*node.expr) : std::nullopt;
        } else if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
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
        if constexpr (std::is_same_v<T, syntax::AttributedExpr>) {
          return node.expr ? FieldHead(*node.expr) : std::nullopt;
        } else if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
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
  if (const auto* attr = std::get_if<syntax::AttributedExpr>(&expr->node)) {
    if (attr->expr) {
      return FormatRangeBound(attr->expr);
    }
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
  if (const auto* attr = std::get_if<syntax::AttributedExpr>(&expr.node)) {
    if (attr->expr) {
      return FormatIndexExpr(*attr->expr);
    }
  }
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
        if constexpr (std::is_same_v<T, syntax::AttributedExpr>) {
          return node.expr ? BuildPlaceRepr(*node.expr) : "";
        } else if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
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
          return LowerResult{MakeIR(std::move(read)), value};
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
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
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
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
        } else if constexpr (std::is_same_v<T, syntax::AttributedExpr>) {
          const bool prev_dynamic = ctx.dynamic_checks;
          if (HasDynamicAttr(node.attrs)) {
            ctx.dynamic_checks = true;
          }
          LowerResult out = node.expr ? LowerReadPlace(*node.expr, ctx)
                                      : LowerResult{EmptyIR(), ctx.FreshTempValue("place_attr")};
          ctx.dynamic_checks = prev_dynamic;
          return out;
        } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
          auto base_result = LowerReadPlace(*node.base, ctx);

          if (std::holds_alternative<syntax::RangeExpr>(node.index->node)) {
            SPEC_RULE("Lower-ReadPlace-Index-Range");
            const auto& range_node = std::get<syntax::RangeExpr>(node.index->node);
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

            return LowerResult{SeqIR(std::vector<IRPtr>{base_result.ir, range_result.ir,
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
        } else if constexpr (std::is_same_v<T, syntax::DerefExpr>) {
          SPEC_RULE("Lower-ReadPlace-Deref");
          auto ptr_result = LowerReadPlace(*node.value, ctx);
          analysis::TypeRef ptr_type;
          if (ctx.expr_type) {
            ptr_type = ctx.expr_type(*node.value);
          }
          if (ptr_type) {
            auto deref_result = LowerRawDeref(ptr_result.value, ptr_type, ctx);
            return LowerResult{SeqIR(std::vector<IRPtr>{ptr_result.ir, deref_result.ir}),
                               deref_result.value};
          }
          IRReadPtr read;
          read.ptr = ptr_result.value;
          IRValue value = ctx.FreshTempValue("deref");
          read.result = value;
          if (ctx.expr_type) {
            ctx.RegisterValueType(value, ctx.expr_type(place));
          }
          return LowerResult{SeqIR(std::vector<IRPtr>{ptr_result.ir, MakeIR(std::move(read))}), value};
        }

        IRValue value = ctx.FreshTempValue("place_unknown");
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
  auto register_ptr_type = [&](IRValue& ptr_value,
                               const analysis::TypeRef& elem_type) {
    if (!elem_type) {
      return;
    }
    auto ptr_type = analysis::MakeTypePtr(elem_type, analysis::PtrState::Valid);
    ctx.RegisterValueType(ptr_value, ptr_type);
  };

  std::function<analysis::TypeRef(const analysis::TypeRef&)> unwrap_elem_type = [&](const analysis::TypeRef& type) -> analysis::TypeRef {
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

        if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
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
                drop_ir = SeqIR(std::vector<IRPtr>{MakeIR(std::move(read)),
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
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
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
            drop_ir = SeqIR(std::vector<IRPtr>{MakeIR(std::move(read)),
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

          return SeqIR(std::vector<IRPtr>{base_addr.ir,
                        MakeIR(std::move(addr_marker)),
                        drop_ir,
                        MakeIR(std::move(write))});
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
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
            drop_ir = SeqIR(std::vector<IRPtr>{MakeIR(std::move(read)),
                             EmitDrop(elem_type, elem_value, ctx)});
          }

          IRAddrOf addr_marker;
          addr_marker.place = LowerPlace(place, ctx);
          addr_marker.result = ptr_value;

          IRWritePtr write;
          write.ptr = ptr_value;
          write.value = value;

          return SeqIR(std::vector<IRPtr>{base_addr.ir,
                        MakeIR(std::move(addr_marker)),
                        drop_ir,
                        MakeIR(std::move(write))});
        } else if constexpr (std::is_same_v<T, syntax::AttributedExpr>) {
          const bool prev_dynamic = ctx.dynamic_checks;
          if (HasDynamicAttr(node.attrs)) {
            ctx.dynamic_checks = true;
          }
          IRPtr out = node.expr ? LowerWritePlaceImpl(*node.expr, value, ctx, allow_drop)
                                : EmptyIR();
          ctx.dynamic_checks = prev_dynamic;
          return out;
        } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
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

          if (std::holds_alternative<syntax::RangeExpr>(node.index->node)) {
            SPEC_RULE(allow_drop ? "Lower-WritePlace-Index-Range"
                                 : "LowerWriteSub-Index-Range");
            const auto& range_node = std::get<syntax::RangeExpr>(node.index->node);
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

            return SeqIR(std::vector<IRPtr>{base_addr.ir, base_read_ir, range_result.ir,
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
            drop_ir = SeqIR(std::vector<IRPtr>{MakeIR(std::move(read)),
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
        } else if constexpr (std::is_same_v<T, syntax::DerefExpr>) {
          SPEC_RULE(allow_drop ? "Lower-WritePlace-Deref"
                               : "LowerWriteSub-Deref");
          auto ptr_result = LowerReadPlace(*node.value, ctx);
          IRWritePtr write;
          write.ptr = ptr_result.value;
          write.value = value;
          return SeqIR(std::vector<IRPtr>{ptr_result.ir, MakeIR(std::move(write))});
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

  auto register_ptr_type = [&](IRValue& value) {
    if (!ctx.expr_type) {
      return;
    }
    analysis::TypeRef base_type = ctx.expr_type(place);
    if (!base_type) {
      return;
    }
    auto ptr_type = analysis::MakeTypePtr(base_type, analysis::PtrState::Valid);
    ctx.RegisterValueType(value, ptr_type);
  };

  auto tag_from = [&](const IRValue& addr, const IRValue& base) -> IRPtr {
    IRCall tag_call;
    tag_call.callee.kind = IRValue::Kind::Symbol;
    tag_call.callee.name = RegionSymAddrTagFrom();
    tag_call.args.push_back(addr);
    tag_call.args.push_back(base);
    IRValue tag_value = ctx.FreshTempValue("addr_tag");
    tag_call.result = tag_value;
    ctx.RegisterValueType(tag_value, analysis::MakeTypePrim("()"));
    return MakeIR(std::move(tag_call));
  };

  return std::visit(
      [&](const auto& node) -> LowerResult {
        using T = std::decay_t<decltype(node)>;

        if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
          IRAddrOf addr;
          addr.place = LowerPlace(place, ctx);

          IRValue ptr_value = ctx.FreshTempValue("addr_of");
          register_ptr_type(ptr_value);
          addr.result = ptr_value;

          if (ctx.GetBindingState(node.name)) {
            SPEC_RULE("Lower-AddrOf-Ident-Local");
            DerivedValueInfo info;
            info.kind = DerivedValueInfo::Kind::AddrLocal;
            info.name = node.name;
            ctx.RegisterDerivedValue(ptr_value, info);
            return LowerResult{MakeIR(std::move(addr)), ptr_value};
          }
          if (const auto* capture = ctx.LookupCapture(node.name)) {
            SPEC_RULE("Lower-AddrOf-Ident-Capture");
            IRValue field_ptr = ctx.CaptureFieldPtr(*capture);
            if (capture->by_ref) {
              IRReadPtr load_ptr;
              load_ptr.ptr = field_ptr;
              load_ptr.result = ptr_value;
              return LowerResult{MakeIR(std::move(load_ptr)), ptr_value};
            }
            return LowerResult{EmptyIR(), field_ptr};
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
          DerivedValueInfo info;
          info.kind = DerivedValueInfo::Kind::AddrStatic;
          info.static_path = full;
          info.name = resolved_name;
          ctx.RegisterDerivedValue(ptr_value, info);

          IRPtr poison_ir = EmptyIR();
          IRCheckPoison check;
          check.module = ModulePathString(full);
          poison_ir = MakeIR(std::move(check));
          if (poison_ir && !std::holds_alternative<IROpaque>(poison_ir->node)) {
            return LowerResult{SeqIR(std::vector<IRPtr>{poison_ir,
                                                        PanicCheck(ctx),
                                                        MakeIR(std::move(addr))}),
                               ptr_value};
          }
          return LowerResult{MakeIR(std::move(addr)), ptr_value};
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
          auto base_result = LowerAddrOf(*node.base, ctx);
          IRAddrOf addr;
          addr.place = LowerPlace(place, ctx);
          IRValue ptr_value = ctx.FreshTempValue("addr_of");
          register_ptr_type(ptr_value);
          addr.result = ptr_value;

          DerivedValueInfo info;
          info.kind = DerivedValueInfo::Kind::AddrField;
          info.base = base_result.value;
          info.field = node.name;
          ctx.RegisterDerivedValue(ptr_value, info);

          IRPtr tag_ir = tag_from(ptr_value, base_result.value);
          return LowerResult{SeqIR(std::vector<IRPtr>{base_result.ir,
                                                      MakeIR(std::move(addr)),
                                                      tag_ir}),
                             ptr_value};
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
          auto base_result = LowerAddrOf(*node.base, ctx);
          IRAddrOf addr;
          addr.place = LowerPlace(place, ctx);
          IRValue ptr_value = ctx.FreshTempValue("addr_of");
          register_ptr_type(ptr_value);
          addr.result = ptr_value;

          DerivedValueInfo info;
          info.kind = DerivedValueInfo::Kind::AddrTuple;
          info.base = base_result.value;
          info.tuple_index = static_cast<std::size_t>(std::stoull(node.index.lexeme));
          ctx.RegisterDerivedValue(ptr_value, info);

          IRPtr tag_ir = tag_from(ptr_value, base_result.value);
          return LowerResult{SeqIR(std::vector<IRPtr>{base_result.ir,
                                                      MakeIR(std::move(addr)),
                                                      tag_ir}),
                             ptr_value};
        } else if constexpr (std::is_same_v<T, syntax::AttributedExpr>) {
          const bool prev_dynamic = ctx.dynamic_checks;
          if (HasDynamicAttr(node.attrs)) {
            ctx.dynamic_checks = true;
          }
          LowerResult out = node.expr ? LowerAddrOf(*node.expr, ctx)
                                      : LowerResult{EmptyIR(), ctx.FreshTempValue("addr_of_attr")};
          ctx.dynamic_checks = prev_dynamic;
          return out;
        } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
          auto base_result = LowerAddrOf(*node.base, ctx);
          IRValue ptr_value = ctx.FreshTempValue("addr_of");
          register_ptr_type(ptr_value);

          IRAddrOf addr;
          addr.place = LowerPlace(place, ctx);
          addr.result = ptr_value;

          if (std::holds_alternative<syntax::RangeExpr>(node.index->node)) {
            const auto& range_node = std::get<syntax::RangeExpr>(node.index->node);
            auto range_result = LowerRangeExpr(range_node, ctx);

            IRCheckRange check;
            check.base = base_result.value;
            check.range = ToIRRange(range_result.value);

            DerivedValueInfo info;
            info.kind = DerivedValueInfo::Kind::AddrIndex;
            info.base = base_result.value;
            info.range = ToIRRange(range_result.value);
            ctx.RegisterDerivedValue(ptr_value, info);

            IRPtr tag_ir = tag_from(ptr_value, base_result.value);
            return LowerResult{SeqIR(std::vector<IRPtr>{base_result.ir, range_result.ir,
                                      MakeIR(std::move(check)),
                                      PanicCheck(ctx),
                                      MakeIR(std::move(addr)),
                                      tag_ir}),
                               ptr_value};
          }

          auto index_result = LowerExpr(*node.index, ctx);
          const bool needs_check = NeedsIndexCheck(*node.base, ctx);
          IRCheckIndex check;
          check.base = base_result.value;
          check.index = index_result.value;

          DerivedValueInfo info;
          info.kind = DerivedValueInfo::Kind::AddrIndex;
          info.base = base_result.value;
          info.index = index_result.value;
          ctx.RegisterDerivedValue(ptr_value, info);

          IRPtr tag_ir = tag_from(ptr_value, base_result.value);
          std::vector<IRPtr> seq;
          seq.push_back(base_result.ir);
          seq.push_back(index_result.ir);
          if (needs_check) {
            seq.push_back(MakeIR(std::move(check)));
            seq.push_back(PanicCheck(ctx));
          }
          seq.push_back(MakeIR(std::move(addr)));
          seq.push_back(tag_ir);
          return LowerResult{SeqIR(std::move(seq)), ptr_value};
        } else if constexpr (std::is_same_v<T, syntax::DerefExpr>) {
          auto ptr_result = LowerExpr(*node.value, ctx);
          analysis::TypeRef ptr_type;
          if (ctx.expr_type) {
            ptr_type = ctx.expr_type(*node.value);
          }
          if (ptr_type) {
            auto stripped = analysis::StripPerm(ptr_type);
            if (stripped) {
              if (const auto* raw = std::get_if<analysis::TypeRawPtr>(&stripped->node)) {
                (void)raw;
                SPEC_RULE("Lower-AddrOf-Deref-Raw");
                return LowerResult{ptr_result.ir, ptr_result.value};
              }
              if (const auto* ptr = std::get_if<analysis::TypePtr>(&stripped->node)) {
                if (ptr->state == analysis::PtrState::Null) {
                  SPEC_RULE("Lower-AddrOf-Deref-Null");
                  IRValue unreachable = ctx.FreshTempValue("unreachable");
                  return LowerResult{SeqIR({ptr_result.ir,
                                            LowerPanic(PanicReason::NullDeref, ctx)}),
                                     unreachable};
                }
                if (ptr->state == analysis::PtrState::Expired) {
                  SPEC_RULE("Lower-AddrOf-Deref-Expired");
                  IRValue unreachable = ctx.FreshTempValue("unreachable");
                  return LowerResult{SeqIR({ptr_result.ir,
                                            LowerPanic(PanicReason::ExpiredDeref, ctx)}),
                                     unreachable};
                }
                SPEC_RULE("Lower-AddrOf-Deref");
                IRCall call;
                call.callee.kind = IRValue::Kind::Symbol;
                call.callee.name = RegionSymAddrIsActive();
                call.args.push_back(ptr_result.value);
                IRValue active_value = ctx.FreshTempValue("addr_active");
                call.result = active_value;
                ctx.RegisterValueType(active_value, analysis::MakeTypePrim("bool"));
                IRCheckOp check;
                check.op = "addr_active";
                check.reason = PanicReasonString(PanicReason::ExpiredDeref);
                check.lhs = active_value;
                return LowerResult{SeqIR({ptr_result.ir,
                                          MakeIR(std::move(call)),
                                          MakeIR(std::move(check)),
                                          PanicCheck(ctx)}),
                                   ptr_result.value};
              }
            }
          }
          SPEC_RULE("Lower-AddrOf-Deref");
          return LowerResult{ptr_result.ir, ptr_result.value};
        }

        IRAddrOf addr;
        addr.place = LowerPlace(place, ctx);
        IRValue ptr_value = ctx.FreshTempValue("addr_of");
        register_ptr_type(ptr_value);
        addr.result = ptr_value;
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
    if (ctx.GetBindingState(*root)) {
      if (auto head = FieldHead(place)) {
        ctx.MarkFieldMoved(*root, *head);
      } else {
        ctx.MarkMoved(*root);
      }
    } else if (ctx.LookupCapture(*root)) {
      // Captured bindings are not tracked in local binding states here.
    } else {
      ctx.MarkMoved(*root);
    }
  }

  IRPlace ir_place = LowerPlace(place, ctx);
  IRMoveState move_state;
  move_state.place = ir_place;

  return LowerResult{SeqIR(std::vector<IRPtr>{read_result.ir, MakeIR(std::move(move_state))}),
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
