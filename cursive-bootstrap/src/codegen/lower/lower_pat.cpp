#include "cursive0/codegen/lower/lower_pat.h"

#include "cursive0/codegen/checks.h"
#include "cursive0/codegen/cleanup.h"
#include "cursive0/codegen/lower/lower_expr.h"
#include "cursive0/analysis/resolve/scopes.h"
#include "cursive0/analysis/types/type_equiv.h"
#include "cursive0/codegen/layout/layout.h"
#include "cursive0/core/assert_spec.h"

#include <cassert>
#include <algorithm>
#include <set>
#include <variant>

namespace cursive0::codegen {

namespace {

IRValue BoolImmediate(bool value) {
  IRValue v;
  v.kind = IRValue::Kind::Immediate;
  v.name = value ? "true" : "false";
  v.bytes = {static_cast<std::uint8_t>(value ? 1 : 0)};
  return v;
}

}  // namespace

// ============================================================================
// §6.6 TagOf - Get discriminant tag
// ============================================================================

IRValue TagOf(const IRValue& value, TagOfKind kind, LowerCtx& ctx) {
  if (kind == TagOfKind::Enum) {
    SPEC_RULE("TagOf-Enum");
  } else {
    SPEC_RULE("TagOf-Modal");
  }

  return ctx.FreshTempValue(kind == TagOfKind::Enum ? "enum_tag" : "modal_tag");
}


namespace {

static analysis::TypeRef LowerSyntaxType(const std::shared_ptr<syntax::Type>& type,
                                    LowerCtx& ctx) {
  if (!type || !ctx.sigma) {
    return nullptr;
  }
  analysis::ScopeContext scope;
  scope.sigma = *ctx.sigma;
  scope.current_module = ctx.module_path;
  if (const auto lowered = LowerTypeForLayout(scope, type)) {
    return *lowered;
  }
  return nullptr;
}

static const syntax::RecordDecl* LookupRecordDecl(const syntax::TypePath& path,
                                                  const LowerCtx& ctx) {
  if (!ctx.sigma) {
    return nullptr;
  }
  const auto it = ctx.sigma->types.find(analysis::PathKeyOf(path));
  if (it == ctx.sigma->types.end()) {
    return nullptr;
  }
  return std::get_if<syntax::RecordDecl>(&it->second);
}

static const syntax::EnumDecl* LookupEnumDecl(const syntax::TypePath& path,
                                              const LowerCtx& ctx) {
  if (!ctx.sigma) {
    return nullptr;
  }
  const auto it = ctx.sigma->types.find(analysis::PathKeyOf(path));
  if (it == ctx.sigma->types.end()) {
    return nullptr;
  }
  return std::get_if<syntax::EnumDecl>(&it->second);
}

static const syntax::ModalDecl* LookupModalDecl(const analysis::TypePath& path,
                                                const LowerCtx& ctx) {
  if (!ctx.sigma) {
    return nullptr;
  }
  syntax::TypePath syntax_path;
  syntax_path.reserve(path.size());
  for (const auto& seg : path) {
    syntax_path.push_back(seg);
  }
  const auto it = ctx.sigma->types.find(analysis::PathKeyOf(syntax_path));
  if (it == ctx.sigma->types.end()) {
    return nullptr;
  }
  return std::get_if<syntax::ModalDecl>(&it->second);
}

static analysis::TypeRef RecordFieldType(const syntax::RecordDecl& decl,
                                     std::string_view name,
                                     LowerCtx& ctx) {
  for (const auto& member : decl.members) {
    if (const auto* field = std::get_if<syntax::FieldDecl>(&member)) {
      if (analysis::IdEq(field->name, name)) {
        return LowerSyntaxType(field->type, ctx);
      }
    }
  }
  return nullptr;
}

static const syntax::VariantDecl* FindVariant(const syntax::EnumDecl& decl,
                                              std::string_view name) {
  for (const auto& variant : decl.variants) {
    if (analysis::IdEq(variant.name, name)) {
      return &variant;
    }
  }
  return nullptr;
}

static analysis::TypeRef EnumPayloadFieldType(const syntax::VariantDecl& variant,
                                          std::string_view name,
                                          LowerCtx& ctx) {
  if (!variant.payload_opt.has_value()) {
    return nullptr;
  }
  if (const auto* record = std::get_if<syntax::VariantPayloadRecord>(&*variant.payload_opt)) {
    for (const auto& field : record->fields) {
      if (analysis::IdEq(field.name, name)) {
        return LowerSyntaxType(field.type, ctx);
      }
    }
  }
  return nullptr;
}

static analysis::TypeRef EnumPayloadTupleType(const syntax::VariantDecl& variant,
                                          std::size_t index,
                                          LowerCtx& ctx) {
  if (!variant.payload_opt.has_value()) {
    return nullptr;
  }
  if (const auto* tuple = std::get_if<syntax::VariantPayloadTuple>(&*variant.payload_opt)) {
    if (index < tuple->elements.size()) {
      return LowerSyntaxType(tuple->elements[index], ctx);
    }
  }
  return nullptr;
}

static const syntax::StateBlock* FindState(const syntax::ModalDecl& decl,
                                           std::string_view name) {
  for (const auto& state : decl.states) {
    if (analysis::IdEq(state.name, name)) {
      return &state;
    }
  }
  return nullptr;
}

static analysis::TypeRef ModalFieldType(const syntax::ModalDecl& decl,
                                    std::string_view state_name,
                                    std::string_view field_name,
                                    LowerCtx& ctx) {
  const auto* state = FindState(decl, state_name);
  if (!state) {
    return nullptr;
  }
  for (const auto& member : state->members) {
    if (const auto* field = std::get_if<syntax::StateFieldDecl>(&member)) {
      if (analysis::IdEq(field->name, field_name)) {
        return LowerSyntaxType(field->type, ctx);
      }
    }
  }
  return nullptr;
}


static void MergeFailures(LowerCtx& base, const LowerCtx& branch) {
  if (branch.resolve_failed) {
    base.resolve_failed = true;
  }
  if (branch.codegen_failed) {
    base.codegen_failed = true;
  }
  for (const auto& name : branch.resolve_failures) {
    if (std::find(base.resolve_failures.begin(), base.resolve_failures.end(), name) ==
        base.resolve_failures.end()) {
      base.resolve_failures.push_back(name);
    }
  }
}

static void MergeMoveStates(LowerCtx& base,
                            const std::vector<const LowerCtx*>& branches) {
  for (auto& [name, stack] : base.binding_states) {
    if (stack.empty()) {
      continue;
    }
    auto& state = stack.back();

    bool moved_any = state.is_moved;
    std::set<std::string> fields;
    if (!moved_any) {
      fields.insert(state.moved_fields.begin(), state.moved_fields.end());
    }

    for (const auto* branch : branches) {
      if (!branch) {
        continue;
      }
      const BindingState* bstate = branch->GetBindingState(name);
      if (!bstate) {
        continue;
      }
      if (bstate->is_moved) {
        moved_any = true;
      } else if (!moved_any) {
        fields.insert(bstate->moved_fields.begin(), bstate->moved_fields.end());
      }
    }

    if (moved_any) {
      state.is_moved = true;
      state.moved_fields.clear();
    } else {
      state.is_moved = false;
      state.moved_fields.assign(fields.begin(), fields.end());
    }
  }
}

}  // namespace

void RegisterPatternBindings(const syntax::Pattern& pattern,
                             const analysis::TypeRef& type_hint,
                             LowerCtx& ctx,
                             bool is_immovable,
                             analysis::ProvenanceKind prov,
                             std::optional<std::string> prov_region) {
  std::function<void(const syntax::Pattern&, analysis::TypeRef)> walk =
      [&](const syntax::Pattern& pat, analysis::TypeRef hint) {
        std::visit(
            [&](const auto& node) {
              using T = std::decay_t<decltype(node)>;
              if constexpr (std::is_same_v<T, syntax::WildcardPattern>) {
                return;
              } else if constexpr (std::is_same_v<T, syntax::LiteralPattern>) {
                return;
              } else if constexpr (std::is_same_v<T, syntax::IdentifierPattern>) {
                ctx.RegisterVar(node.name, hint, true, is_immovable, prov,
                                prov_region);
                return;
              } else if constexpr (std::is_same_v<T, syntax::TypedPattern>) {
                analysis::TypeRef typed = LowerSyntaxType(node.type, ctx);
                if (!typed) {
                  typed = hint;
                }
                ctx.RegisterVar(node.name, typed, true, is_immovable, prov,
                                prov_region);
                return;
              } else if constexpr (std::is_same_v<T, syntax::TuplePattern>) {
                const analysis::TypeTuple* tuple_type = nullptr;
                if (hint && std::holds_alternative<analysis::TypeTuple>(hint->node)) {
                  tuple_type = &std::get<analysis::TypeTuple>(hint->node);
                }
                for (std::size_t i = 0; i < node.elements.size(); ++i) {
                  analysis::TypeRef elem_type;
                  if (tuple_type && i < tuple_type->elements.size()) {
                    elem_type = tuple_type->elements[i];
                  }
                  walk(*node.elements[i], elem_type);
                }
                return;
              } else if constexpr (std::is_same_v<T, syntax::RecordPattern>) {
                const syntax::RecordDecl* record = LookupRecordDecl(node.path, ctx);
                for (const auto& field : node.fields) {
                  analysis::TypeRef field_type;
                  if (record) {
                    field_type = RecordFieldType(*record, field.name, ctx);
                  }
                  if (field.pattern_opt) {
                    walk(*field.pattern_opt, field_type);
                  } else {
                    ctx.RegisterVar(field.name, field_type, true, is_immovable, prov,
                                    prov_region);
                  }
                }
                return;
              } else if constexpr (std::is_same_v<T, syntax::EnumPattern>) {
                const syntax::EnumDecl* enum_decl = LookupEnumDecl(node.path, ctx);
                const syntax::VariantDecl* variant = nullptr;
                if (enum_decl) {
                  variant = FindVariant(*enum_decl, node.name);
                }
                if (!node.payload_opt.has_value()) {
                  return;
                }
                std::visit(
                    [&](const auto& payload) {
                      using P = std::decay_t<decltype(payload)>;
                      if constexpr (std::is_same_v<P, syntax::TuplePayloadPattern>) {
                        for (std::size_t i = 0; i < payload.elements.size(); ++i) {
                          analysis::TypeRef elem_type;
                          if (variant) {
                            elem_type = EnumPayloadTupleType(*variant, i, ctx);
                          }
                          walk(*payload.elements[i], elem_type);
                        }
                      } else {
                        for (const auto& field : payload.fields) {
                          analysis::TypeRef field_type;
                          if (variant) {
                            field_type = EnumPayloadFieldType(*variant, field.name, ctx);
                          }
                          if (field.pattern_opt) {
                            walk(*field.pattern_opt, field_type);
                          } else {
                            ctx.RegisterVar(field.name, field_type, true, is_immovable, prov,
                                            prov_region);
                          }
                        }
                      }
                    },
                    *node.payload_opt);
                return;
              } else if constexpr (std::is_same_v<T, syntax::ModalPattern>) {
                analysis::TypePath modal_path;
                if (hint && std::holds_alternative<analysis::TypeModalState>(hint->node)) {
                  modal_path = std::get<analysis::TypeModalState>(hint->node).path;
                } else if (hint && std::holds_alternative<analysis::TypePathType>(hint->node)) {
                  modal_path = std::get<analysis::TypePathType>(hint->node).path;
                }
                const syntax::ModalDecl* modal_decl = modal_path.empty() ? nullptr : LookupModalDecl(modal_path, ctx);
                if (!node.fields_opt.has_value()) {
                  return;
                }
                for (const auto& field : node.fields_opt->fields) {
                  analysis::TypeRef field_type;
                  if (modal_decl) {
                    field_type = ModalFieldType(*modal_decl, node.state, field.name, ctx);
                  }
                  if (field.pattern_opt) {
                    walk(*field.pattern_opt, field_type);
                  } else {
                    ctx.RegisterVar(field.name, field_type, true, is_immovable, prov,
                                    prov_region);
                  }
                }
                return;
              } else if constexpr (std::is_same_v<T, syntax::RangePattern>) {
                if (node.lo) {
                  walk(*node.lo, hint);
                }
                if (node.hi) {
                  walk(*node.hi, hint);
                }
                return;
              }
            },
            pat.node);
      };

  walk(pattern, type_hint);
}

// ============================================================================
// §6.6 LowerBindPattern - Bind value to pattern
// ============================================================================

IRPtr LowerBindPattern(const syntax::Pattern& pattern,
                       const IRValue& value,
                       LowerCtx& ctx) {
  SPEC_RULE("Lower-Pat-General");
  auto lookup_bind_type = [&ctx](const std::string& name) -> analysis::TypeRef {
    if (const auto* state = ctx.GetBindingState(name)) {
      return state->type;
    }
    return nullptr;
  };
  auto lookup_bind_prov = [&ctx](const std::string& name) -> analysis::ProvenanceKind {
    if (const auto* state = ctx.GetBindingState(name)) {
      return state->prov;
    }
    return analysis::ProvenanceKind::Bottom;
  };
  auto lookup_bind_region = [&ctx](const std::string& name) -> std::optional<std::string> {
    if (const auto* state = ctx.GetBindingState(name)) {
      return state->prov_region;
    }
    return std::nullopt;
  };
  return std::visit(
      [&value, &ctx, &lookup_bind_type, &lookup_bind_prov,
       &lookup_bind_region](const auto& pat) -> IRPtr {
        using T = std::decay_t<decltype(pat)>;

        if constexpr (std::is_same_v<T, syntax::WildcardPattern>) {
          return EmptyIR();
        } else if constexpr (std::is_same_v<T, syntax::IdentifierPattern>) {
          IRBindVar bind;
          bind.name = pat.name;
          bind.value = value;
          bind.type = lookup_bind_type(pat.name);
          bind.prov = lookup_bind_prov(pat.name);
          bind.prov_region = lookup_bind_region(pat.name);
          return MakeIR(std::move(bind));
        } else if constexpr (std::is_same_v<T, syntax::TypedPattern>) {
          IRValue bind_value = value;
          if (ctx.sigma) {
            analysis::TypeRef target = LowerSyntaxType(pat.type, ctx);
            const auto base_type = ctx.LookupValueType(value);
            if (target && base_type) {
              analysis::TypeRef stripped = base_type;
              if (const auto* perm = std::get_if<analysis::TypePerm>(&stripped->node)) {
                stripped = perm->base;
              }
              if (stripped && std::holds_alternative<analysis::TypeUnion>(stripped->node)) {
                const auto& uni = std::get<analysis::TypeUnion>(stripped->node);
                analysis::ScopeContext scope;
                scope.sigma = *ctx.sigma;
                scope.current_module = ctx.module_path;
                if (const auto layout = UnionLayoutOf(scope, uni)) {
                  const auto& members = layout->member_list;
                  std::optional<std::size_t> member_index;
                  for (std::size_t i = 0; i < members.size(); ++i) {
                    if (analysis::TypeEquiv(target, members[i]).equiv) {
                      member_index = i;
                      break;
                    }
                  }
                  if (member_index.has_value()) {
                    IRValue payload = ctx.FreshTempValue("pat_union_payload");
                    DerivedValueInfo info;
                    info.kind = DerivedValueInfo::Kind::UnionPayload;
                    info.base = value;
                    info.union_index = *member_index;
                    ctx.RegisterDerivedValue(payload, info);
                    bind_value = payload;
                  }
                }
              }
            }
          }
          IRBindVar bind;
          bind.name = pat.name;
          bind.value = bind_value;
          bind.type = lookup_bind_type(pat.name);
          if (!bind.type) {
            bind.type = LowerSyntaxType(pat.type, ctx);
          }
          bind.prov = lookup_bind_prov(pat.name);
          bind.prov_region = lookup_bind_region(pat.name);
          return MakeIR(std::move(bind));
        } else if constexpr (std::is_same_v<T, syntax::LiteralPattern>) {
          return EmptyIR();
        } else if constexpr (std::is_same_v<T, syntax::TuplePattern>) {
          std::vector<IRPtr> bindings;
          for (std::size_t i = 0; i < pat.elements.size(); ++i) {
            IRValue elem = ctx.FreshTempValue("pat_tuple_elem");
            DerivedValueInfo info;
            info.kind = DerivedValueInfo::Kind::Tuple;
            info.base = value;
            info.tuple_index = i;
            ctx.RegisterDerivedValue(elem, info);
            bindings.push_back(LowerBindPattern(*pat.elements[i], elem, ctx));
          }
          return SeqIR(std::move(bindings));
        } else if constexpr (std::is_same_v<T, syntax::RecordPattern>) {
          std::vector<IRPtr> bindings;
          for (const auto& field : pat.fields) {
            IRValue field_val = ctx.FreshTempValue("pat_field");
            DerivedValueInfo info;
            info.kind = DerivedValueInfo::Kind::Field;
            info.base = value;
            info.field = field.name;
            ctx.RegisterDerivedValue(field_val, info);
            if (field.pattern_opt) {
              bindings.push_back(
                  LowerBindPattern(*field.pattern_opt, field_val, ctx));
            } else {
              IRBindVar bind;
              bind.name = field.name;
              bind.value = field_val;
              bind.type = lookup_bind_type(field.name);
              bind.prov = lookup_bind_prov(field.name);
              bind.prov_region = lookup_bind_region(field.name);
              bindings.push_back(MakeIR(std::move(bind)));
            }
          }
          return SeqIR(std::move(bindings));
        } else if constexpr (std::is_same_v<T, syntax::EnumPattern>) {
          if (!pat.payload_opt) {
            return EmptyIR();
          }
          return std::visit(
              [&value, &ctx, &pat, &lookup_bind_type, &lookup_bind_prov,
               &lookup_bind_region](const auto& payload) -> IRPtr {
                using P = std::decay_t<decltype(payload)>;
                if constexpr (std::is_same_v<P, syntax::TuplePayloadPattern>) {
                  std::vector<IRPtr> bindings;
                  for (std::size_t i = 0; i < payload.elements.size(); ++i) {
                    IRValue elem = ctx.FreshTempValue("pat_enum_payload_elem");
                    DerivedValueInfo info;
                    info.kind = DerivedValueInfo::Kind::EnumPayloadIndex;
                    info.base = value;
                    info.variant = pat.name;
                    info.tuple_index = i;
                    ctx.RegisterDerivedValue(elem, info);
                    bindings.push_back(
                        LowerBindPattern(*payload.elements[i], elem, ctx));
                  }
                  return SeqIR(std::move(bindings));
                } else {
                  std::vector<IRPtr> bindings;
                  for (const auto& field : payload.fields) {
                    IRValue field_val = ctx.FreshTempValue("pat_enum_payload_field");
                    DerivedValueInfo info;
                    info.kind = DerivedValueInfo::Kind::EnumPayloadField;
                    info.base = value;
                    info.variant = pat.name;
                    info.field = field.name;
                    ctx.RegisterDerivedValue(field_val, info);
                    if (field.pattern_opt) {
                      bindings.push_back(
                          LowerBindPattern(*field.pattern_opt, field_val, ctx));
                    } else {
                      IRBindVar bind;
                      bind.name = field.name;
                      bind.value = field_val;
                      bind.type = lookup_bind_type(field.name);
                      bind.prov = lookup_bind_prov(field.name);
                      bind.prov_region = lookup_bind_region(field.name);
                      bindings.push_back(MakeIR(std::move(bind)));
                    }
                  }
                  return SeqIR(std::move(bindings));
                }
              },
              *pat.payload_opt);
        } else if constexpr (std::is_same_v<T, syntax::ModalPattern>) {
          if (!pat.fields_opt) {
            return EmptyIR();
          }
          std::vector<IRPtr> bindings;
          for (const auto& field : pat.fields_opt->fields) {
            IRValue field_val = ctx.FreshTempValue("pat_modal_field");
            DerivedValueInfo info;
            info.kind = DerivedValueInfo::Kind::ModalField;
            info.base = value;
            info.modal_state = pat.state;
            info.field = field.name;
            ctx.RegisterDerivedValue(field_val, info);
            if (field.pattern_opt) {
              bindings.push_back(
                  LowerBindPattern(*field.pattern_opt, field_val, ctx));
            } else {
              IRBindVar bind;
              bind.name = field.name;
              bind.value = field_val;
              bind.type = lookup_bind_type(field.name);
              bind.prov = lookup_bind_prov(field.name);
              bind.prov_region = lookup_bind_region(field.name);
              bindings.push_back(MakeIR(std::move(bind)));
            }
          }
          return SeqIR(std::move(bindings));
        } else if constexpr (std::is_same_v<T, syntax::RangePattern>) {
          return EmptyIR();
        } else {
          SPEC_RULE("Lower-Pat-Err");
          return EmptyIR();
        }
      },
      pattern.node);
}


// ============================================================================
// §6.6 LowerBindList - Bind values to patterns
// ============================================================================

IRPtr LowerBindList(
    const std::vector<std::shared_ptr<syntax::Pattern>>& patterns,
    const std::vector<IRValue>& values,
    LowerCtx& ctx) {
  SPEC_RULE("Lower-BindList-Empty");
  SPEC_RULE("Lower-BindList-Cons");

  if (patterns.empty() || values.empty()) {
    return EmptyIR();
  }

  std::vector<IRPtr> bindings;
  std::size_t count = std::min(patterns.size(), values.size());
  for (std::size_t i = 0; i < count; ++i) {
    bindings.push_back(LowerBindPattern(*patterns[i], values[i], ctx));
  }

  return SeqIR(std::move(bindings));
}

// ============================================================================
// §6.6 PatternCheck - Check if value matches pattern
// ============================================================================

IRValue PatternCheck(const syntax::Pattern& pattern,
                     const IRValue& value,
                     LowerCtx& ctx) {
  return std::visit(
      [&value, &ctx](const auto& pat) -> IRValue {
        using T = std::decay_t<decltype(pat)>;

        if constexpr (std::is_same_v<T, syntax::WildcardPattern>) {
          // Wildcard always matches
          return BoolImmediate(true);
        } else if constexpr (std::is_same_v<T, syntax::IdentifierPattern>) {
          // Identifier always matches (binds)
          return BoolImmediate(true);
        } else if constexpr (std::is_same_v<T, syntax::TypedPattern>) {
          // Typed pattern always matches (binds)
          return BoolImmediate(true);
        } else if constexpr (std::is_same_v<T, syntax::LiteralPattern>) {
          // Compare with literal
          return ctx.FreshTempValue("pat_literal_match");
        } else if constexpr (std::is_same_v<T, syntax::EnumPattern>) {
          // Compare tag
          (void)TagOf(value, TagOfKind::Enum, ctx);
          return ctx.FreshTempValue("pat_enum_tag_match");
        } else if constexpr (std::is_same_v<T, syntax::ModalPattern>) {
          (void)TagOf(value, TagOfKind::Modal, ctx);
          return ctx.FreshTempValue("pat_modal_tag_match");
        }
        return ctx.FreshTempValue("pat_check");
      },
      pattern.node);
}

// ============================================================================
// §6.6 LowerMatchArm - Lower a single match arm
// ============================================================================

LowerResult LowerMatchArm(const syntax::MatchArm& arm,
                          const IRValue& scrutinee,
                          const analysis::TypeRef& scrutinee_type,
                          analysis::ProvenanceKind scrutinee_prov,
                          std::optional<std::string> scrutinee_region,
                          LowerCtx& ctx) {
  ctx.PushScope(false, false);
  RegisterPatternBindings(*arm.pattern, scrutinee_type, ctx, false,
                          scrutinee_prov, scrutinee_region);

  // Bind the pattern
  IRPtr bind_ir = LowerBindPattern(*arm.pattern, scrutinee, ctx);

  // Lower the body
  auto body_result = LowerExpr(*arm.body, ctx);

  CleanupPlan cleanup_plan = ComputeCleanupPlanForCurrentScope(ctx);
  CleanupPlan remainder = ComputeCleanupPlanRemainder(CleanupTarget::CurrentScope, ctx);
  IRPtr cleanup_ir = EmitCleanupWithRemainder(cleanup_plan, remainder, ctx);
  ctx.PopScope();

  return LowerResult{SeqIR({bind_ir, body_result.ir, cleanup_ir}), body_result.value};
}

// ============================================================================
// §6.6 LowerMatch - Lower match expression
// ============================================================================

LowerResult LowerMatch(const syntax::Expr& scrutinee,
                       const std::vector<syntax::MatchArm>& arms,
                       LowerCtx& ctx) {
  SPEC_RULE("Lower-Match");

  // Lower the scrutinee
  auto prev_suppress = ctx.suppress_temp_at_depth;
  if (std::holds_alternative<syntax::MoveExpr>(scrutinee.node)) {
    ctx.suppress_temp_at_depth = ctx.temp_depth + 1;
  }
  auto scrutinee_result = LowerExpr(scrutinee, ctx);
  ctx.suppress_temp_at_depth = prev_suppress;
  analysis::TypeRef scrutinee_type;
  if (ctx.expr_type) {
    scrutinee_type = ctx.expr_type(scrutinee);
  }
  analysis::ProvenanceKind scrutinee_prov = analysis::ProvenanceKind::Bottom;
  if (auto prov = ctx.LookupExprProv(scrutinee)) {
    scrutinee_prov = *prov;
  }
  std::optional<std::string> scrutinee_region;
  if (scrutinee_prov == analysis::ProvenanceKind::Region) {
    scrutinee_region = ctx.LookupExprRegion(scrutinee);
  }
  if (scrutinee_type) {
    ctx.RegisterValueType(scrutinee_result.value, scrutinee_type);
  }

  // Create match arms IR
  std::vector<IRMatchArm> ir_arms;
  std::vector<LowerCtx> arm_ctxs;
  arm_ctxs.reserve(arms.size());
  for (const auto& arm : arms) {
    LowerCtx arm_ctx = ctx;
    auto arm_result = LowerMatchArm(arm, scrutinee_result.value, scrutinee_type,
                                    scrutinee_prov, scrutinee_region, arm_ctx);
    for (const auto& [name, type] : arm_ctx.value_types) {
      if (!ctx.value_types.count(name)) {
        ctx.value_types.emplace(name, type);
      }
    }
    for (const auto& [name, info] : arm_ctx.derived_values) {
      if (!ctx.derived_values.count(name)) {
        ctx.derived_values.emplace(name, info);
      }
    }
    for (const auto& [name, type] : arm_ctx.static_types) {
      if (!ctx.static_types.count(name)) {
        ctx.static_types.emplace(name, type);
      }
    }
    for (const auto& [name, type] : arm_ctx.drop_glue_types) {
      if (!ctx.drop_glue_types.count(name)) {
        ctx.drop_glue_types.emplace(name, type);
      }
    }
    ctx.temp_counter = std::max(ctx.temp_counter, arm_ctx.temp_counter);
    IRMatchArm ir_arm;
    ir_arm.pattern = arm.pattern;
    ir_arm.body = arm_result.ir;
    ir_arm.value = arm_result.value;
    ir_arms.push_back(std::move(ir_arm));
    arm_ctxs.push_back(std::move(arm_ctx));
  }

  std::vector<const LowerCtx*> branches;
  branches.reserve(arm_ctxs.size());
  for (const auto& arm_ctx : arm_ctxs) {
    branches.push_back(&arm_ctx);
  }
  MergeMoveStates(ctx, branches);
  for (const auto& arm_ctx : arm_ctxs) {
    MergeFailures(ctx, arm_ctx);
  }

  IRMatch match;
  match.scrutinee = scrutinee_result.value;
  match.scrutinee_type = scrutinee_type;
  match.arms = std::move(ir_arms);

  IRValue result = ctx.FreshTempValue("match");
  match.result = result;
  if (ctx.expr_type) {
    analysis::TypeRef result_type;
    for (const auto& arm : arms) {
      if (!arm.body) {
        continue;
      }
      analysis::TypeRef arm_type = ctx.expr_type(*arm.body);
      if (!arm_type) {
        continue;
      }
      if (!result_type) {
        result_type = arm_type;
        continue;
      }
      if (!analysis::TypeEquiv(result_type, arm_type).equiv) {
        result_type = arm_type;
      }
    }
    if (result_type) {
      ctx.RegisterValueType(result, result_type);
    }
  }

  return LowerResult{SeqIR({scrutinee_result.ir, MakeIR(std::move(match))}),
                     result};
}

// ============================================================================
// Anchor function
// ============================================================================

void AnchorPatternRules() {
  SPEC_RULE("Lower-Pat-Correctness");
  SPEC_RULE("Lower-Match-Correctness");
  SPEC_RULE("TagOf-Enum");
  SPEC_RULE("TagOf-Modal");
  SPEC_RULE("Lower-BindList-Empty");
  SPEC_RULE("Lower-BindList-Cons");
  SPEC_RULE("Lower-Pat-General");
  SPEC_RULE("Lower-Pat-Err");
  SPEC_RULE("Lower-Match");
}


}  // namespace cursive0::codegen
