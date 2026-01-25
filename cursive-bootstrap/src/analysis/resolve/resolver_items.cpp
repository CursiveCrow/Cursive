#include "cursive0/analysis/resolve/resolver.h"

#include <utility>
#include <type_traits>

#include "cursive0/core/assert_spec.h"
#include "cursive0/analysis/resolve/collect_toplevel.h"
#include "cursive0/analysis/resolve/scopes.h"
#include "cursive0/analysis/resolve/scopes_intro.h"

namespace cursive0::analysis {

namespace {

static inline void SpecDefsResolverItems() {
  SPEC_DEF("ResolveItem", "5.1.7");
  SPEC_DEF("ResolveParams", "5.1.7");
  SPEC_DEF("ResolveParam", "5.1.7");
  SPEC_DEF("ResolveTypeList", "5.1.7");
  SPEC_DEF("ResolveFieldDecl", "5.1.7");
  SPEC_DEF("ResolveFieldDeclList", "5.1.7");
  SPEC_DEF("ResolveRecordMember", "5.1.7");
  SPEC_DEF("ResolveRecordMemberList", "5.1.7");
  SPEC_DEF("ResolveClassItem", "5.1.7");
  SPEC_DEF("ResolveClassItemList", "5.1.7");
  SPEC_DEF("ResolveVariant", "5.1.7");
  SPEC_DEF("ResolveVariantList", "5.1.7");
  SPEC_DEF("ResolveStateMember", "5.1.7");
  SPEC_DEF("ResolveStateMemberList", "5.1.7");
  SPEC_DEF("ResolveStateBlockList", "5.1.7");
  SPEC_DEF("BindSelfRecord", "5.1.7");
  SPEC_DEF("BindSelfClass", "5.1.7");
}

ResolveResult<std::optional<syntax::TypeInvariant>> ResolveTypeInvariantOpt(
    ResolveContext& ctx,
    const std::optional<syntax::TypeInvariant>& inv_opt) {
  ResolveResult<std::optional<syntax::TypeInvariant>> result;
  if (!inv_opt.has_value()) {
    result.ok = true;
    result.value = std::nullopt;
    return result;
  }
  ScopeContext inv_scope = *ctx.ctx;
  inv_scope.scopes.insert(inv_scope.scopes.begin(), Scope{});
  IntroResult intro = Intro(inv_scope, "self",
                            Entity{EntityKind::Value, std::nullopt,
                                   std::nullopt, EntitySource::Decl});
  if (!intro.ok) {
    result.ok = false;
    result.diag_id = intro.diag_id;
    return result;
  }
  ResolveContext inv_ctx = ctx;
  inv_ctx.ctx = &inv_scope;
  const auto pred = ResolveExpr(inv_ctx, inv_opt->predicate);
  if (!pred.ok) {
    result.ok = false;
    result.diag_id = pred.diag_id;
    result.span = pred.span;
    return result;
  }
  syntax::TypeInvariant inv;
  inv.predicate = pred.value;
  inv.span = inv_opt->span;
  result.ok = true;
  result.value = inv;
  return result;
}

std::optional<core::Span> SpanOfItem(const syntax::ASTItem& item) {
  return std::visit(
      [](const auto& it) -> std::optional<core::Span> { return it.span; },
      item);
}

ResTypeResult ResolveTypeOpt(ResolveContext& ctx,
                             const std::shared_ptr<syntax::Type>& type_opt) {
  ResTypeResult result;
  result.ok = true;
  result.value = type_opt;
  if (!type_opt) {
    SPEC_RULE("ResolveTypeOpt-None");
    return result;
  }
  const auto resolved = ResolveType(ctx, type_opt);
  if (!resolved.ok) {
    return {false, resolved.diag_id, resolved.span, {}};
  }
  SPEC_RULE("ResolveTypeOpt-Some");
  return {true, std::nullopt, std::nullopt, resolved.value};
}

ResExprResult ResolveExprOpt(ResolveContext& ctx,
                            const syntax::ExprPtr& expr_opt) {
  ResExprResult result;
  result.ok = true;
  result.value = expr_opt;
  if (!expr_opt) {
    SPEC_RULE("ResolveExprOpt-None");
    return result;
  }
  const auto resolved = ResolveExpr(ctx, expr_opt);
  if (!resolved.ok) {
    return {false, resolved.diag_id, resolved.span, {}};
  }
  SPEC_RULE("ResolveExprOpt-Some");
  return {true, std::nullopt, std::nullopt, resolved.value};
}

ResolveResult<syntax::Receiver> ResolveReceiver(ResolveContext& ctx,
                                                const syntax::Receiver& recv) {
  ResolveResult<syntax::Receiver> result;
  return std::visit(
      [&](const auto& node) -> ResolveResult<syntax::Receiver> {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::ReceiverShorthand>) {
          SPEC_RULE("ResolveReceiver-Shorthand");
          result.ok = true;
          result.value = recv;
          return result;
        } else {
          const auto resolved = ResolveType(ctx, node.type);
          if (!resolved.ok) {
            return {false, resolved.diag_id, resolved.span, {}};
          }
          syntax::ReceiverExplicit out = node;
          out.type = resolved.value;
          SPEC_RULE("ResolveReceiver-Explicit");
          result.ok = true;
          result.value = out;
          return result;
        }
      },
      recv);
}

ResolveResult<std::vector<syntax::Param>> ResolveParams(
    ResolveContext& ctx,
    const std::vector<syntax::Param>& params) {
  ResolveResult<std::vector<syntax::Param>> result;
  result.ok = true;
  if (params.empty()) {
    SPEC_RULE("ResolveParams-Empty");
    return result;
  }
  result.value.reserve(params.size());
  for (const auto& param : params) {
    auto resolved_param = param;
    const auto resolved_type = ResolveType(ctx, param.type);
    if (!resolved_type.ok) {
      return {false, resolved_type.diag_id, resolved_type.span, {}};
    }
    resolved_param.type = resolved_type.value;
    SPEC_RULE("ResolveParam");
    result.value.push_back(std::move(resolved_param));
    SPEC_RULE("ResolveParams-Cons");
  }
  return result;
}

ResolveResult<std::vector<std::shared_ptr<syntax::Type>>> ResolveTypeList(
    ResolveContext& ctx,
    const std::vector<std::shared_ptr<syntax::Type>>& types) {
  ResolveResult<std::vector<std::shared_ptr<syntax::Type>>> result;
  result.ok = true;
  if (types.empty()) {
    SPEC_RULE("ResolveTypeList-Empty");
    return result;
  }
  result.value.reserve(types.size());
  for (const auto& type : types) {
    const auto resolved = ResolveType(ctx, type);
    if (!resolved.ok) {
      return {false, resolved.diag_id, resolved.span, {}};
    }
    result.value.push_back(resolved.value);
    SPEC_RULE("ResolveTypeList-Cons");
  }
  return result;
}

ResolveResult<std::vector<syntax::ClassPath>> ResolveClassPathList(
    ResolveContext& ctx,
    const std::vector<syntax::ClassPath>& paths) {
  ResolveResult<std::vector<syntax::ClassPath>> result;
  result.ok = true;
  result.value.reserve(paths.size());
  for (const auto& path : paths) {
    const auto resolved = ResolveClassPath(ctx, path);
    if (!resolved.ok) {
      return {false, resolved.diag_id, resolved.span, {}};
    }
    result.value.push_back(resolved.value);
    SPEC_RULE("ResolveClassPathList-Cons");
  }
  if (paths.empty()) {
    SPEC_RULE("ResolveClassPathList-Empty");
  }
  return result;
}

ResolveResult<std::vector<syntax::FieldDecl>> ResolveFieldDeclList(
    ResolveContext& ctx,
    const std::vector<syntax::FieldDecl>& fields) {
  ResolveResult<std::vector<syntax::FieldDecl>> result;
  result.ok = true;
  if (fields.empty()) {
    SPEC_RULE("ResolveFieldDeclList-Empty");
    return result;
  }
  result.value.reserve(fields.size());
  for (const auto& field : fields) {
    auto out = field;
    const auto resolved_type = ResolveType(ctx, field.type);
    if (!resolved_type.ok) {
      return {false, resolved_type.diag_id, resolved_type.span, {}};
    }
    out.type = resolved_type.value;
    const auto resolved_init = ResolveExprOpt(ctx, field.init_opt);
    if (!resolved_init.ok) {
      return {false, resolved_init.diag_id, resolved_init.span, {}};
    }
    out.init_opt = resolved_init.value;
    SPEC_RULE("ResolveFieldDecl");
    result.value.push_back(std::move(out));
    SPEC_RULE("ResolveFieldDeclList-Cons");
  }
  return result;
}

ResolveResult<std::vector<syntax::VariantDecl>> ResolveVariantList(
    ResolveContext& ctx,
    const std::vector<syntax::VariantDecl>& vars) {
  ResolveResult<std::vector<syntax::VariantDecl>> result;
  result.ok = true;
  if (vars.empty()) {
    SPEC_RULE("ResolveVariantList-Empty");
    return result;
  }
  result.value.reserve(vars.size());
  for (const auto& var : vars) {
    auto out = var;
    if (var.payload_opt.has_value()) {
      if (std::holds_alternative<syntax::VariantPayloadTuple>(*var.payload_opt)) {
        auto payload = std::get<syntax::VariantPayloadTuple>(*var.payload_opt);
        const auto types = ResolveTypeList(ctx, payload.elements);
        if (!types.ok) {
          return {false, types.diag_id, types.span, {}};
        }
        payload.elements = types.value;
        out.payload_opt = payload;
        SPEC_RULE("ResolveVariantPayloadOpt-Tuple");
      } else {
        auto payload = std::get<syntax::VariantPayloadRecord>(*var.payload_opt);
        const auto fields = ResolveFieldDeclList(ctx, payload.fields);
        if (!fields.ok) {
          return {false, fields.diag_id, fields.span, {}};
        }
        payload.fields = fields.value;
        out.payload_opt = payload;
        SPEC_RULE("ResolveVariantPayloadOpt-Record");
      }
    } else {
      SPEC_RULE("ResolveVariantPayloadOpt-None");
    }
    SPEC_RULE("ResolveVariant");
    result.value.push_back(std::move(out));
    SPEC_RULE("ResolveVariantList-Cons");
  }
  return result;
}

ResolveResult<std::vector<syntax::RecordMember>> ResolveRecordMemberList(
    ResolveContext& ctx,
    const syntax::RecordDecl& record,
    const std::vector<syntax::RecordMember>& members) {
  ResolveResult<std::vector<syntax::RecordMember>> result;
  result.ok = true;
  if (members.empty()) {
    SPEC_RULE("ResolveRecordMemberList-Empty");
    return result;
  }
  result.value.reserve(members.size());
  for (const auto& member : members) {
    auto resolved = std::visit(
        [&](const auto& node) -> ResolveResult<syntax::RecordMember> {
          using T = std::decay_t<decltype(node)>;
          if constexpr (std::is_same_v<T, syntax::FieldDecl>) {
            auto out = node;
            const auto resolved_type = ResolveType(ctx, node.type);
            if (!resolved_type.ok) {
              return {false, resolved_type.diag_id, resolved_type.span, {}};
            }
            out.type = resolved_type.value;
            const auto resolved_init = ResolveExprOpt(ctx, node.init_opt);
            if (!resolved_init.ok) {
              return {false, resolved_init.diag_id, resolved_init.span, {}};
            }
            out.init_opt = resolved_init.value;
            SPEC_RULE("ResolveRecordMember-Field");
            return {true, std::nullopt, std::nullopt, out};
          } else {
            auto out = node;
            ScopeContext proc_ctx;
            proc_ctx.project = ctx.ctx->project;
            proc_ctx.sigma = ctx.ctx->sigma;
            proc_ctx.current_module = ctx.ctx->current_module;
            Scope proc_scope;
            for (const auto& param : node.params) {
              proc_scope.emplace(IdKeyOf(param.name),
                                 Entity{EntityKind::Value, std::nullopt,
                                        std::nullopt, EntitySource::Decl});
            }
            proc_scope.emplace(
                IdKeyOf("self"),
                Entity{EntityKind::Value, std::nullopt, std::nullopt,
                       EntitySource::Decl});
            proc_scope.emplace(
                IdKeyOf("Self"),
                Entity{EntityKind::Type, proc_ctx.current_module, record.name,
                       EntitySource::Decl});
            SPEC_RULE("BindSelf-Record");
            proc_ctx.scopes = {proc_scope, ModuleScope(ctx.ctx->scopes),
                               UniverseScope(ctx.ctx->scopes)};
            ResolveContext method_ctx = ctx;
            method_ctx.ctx = &proc_ctx;

            const auto resolved_recv = ResolveReceiver(method_ctx, node.receiver);
            if (!resolved_recv.ok) {
              return {false, resolved_recv.diag_id, resolved_recv.span, {}};
            }
            out.receiver = resolved_recv.value;
            const auto resolved_params = ResolveParams(method_ctx, node.params);
            if (!resolved_params.ok) {
              return {false, resolved_params.diag_id, resolved_params.span, {}};
            }
            out.params = resolved_params.value;
            const auto resolved_ret = ResolveTypeOpt(method_ctx, node.return_type_opt);
            if (!resolved_ret.ok) {
              return {false, resolved_ret.diag_id, resolved_ret.span, {}};
            }
            out.return_type_opt = resolved_ret.value;
            if (node.body) {
              const auto resolved_body = ResolveBlock(method_ctx, *node.body);
              if (!resolved_body.ok) {
                return {false, resolved_body.diag_id, resolved_body.span, {}};
              }
              out.body = std::make_shared<syntax::Block>(resolved_body.block);
            }
            SPEC_RULE("ResolveRecordMember-Method");
            return {true, std::nullopt, std::nullopt, out};
          }
        },
        member);
    if (!resolved.ok) {
      return {false, resolved.diag_id, resolved.span, {}};
    }
    result.value.push_back(std::move(resolved.value));
    SPEC_RULE("ResolveRecordMemberList-Cons");
  }
  return result;
}

ResolveResult<std::vector<syntax::ClassItem>> ResolveClassItemList(
    ResolveContext& ctx,
    const std::vector<syntax::ClassItem>& items) {
  ResolveResult<std::vector<syntax::ClassItem>> result;
  result.ok = true;
  if (items.empty()) {
    SPEC_RULE("ResolveClassItemList-Empty");
    return result;
  }
  result.value.reserve(items.size());
  for (const auto& item : items) {
    auto resolved = std::visit(
        [&](const auto& node) -> ResolveResult<syntax::ClassItem> {
          using T = std::decay_t<decltype(node)>;
          if constexpr (std::is_same_v<T, syntax::ClassFieldDecl>) {
            auto out = node;
            const auto resolved_type = ResolveType(ctx, node.type);
            if (!resolved_type.ok) {
              return {false, resolved_type.diag_id, resolved_type.span, {}};
            }
            out.type = resolved_type.value;
            SPEC_RULE("ResolveClassItem-Field");
            return {true, std::nullopt, std::nullopt, out};
          } else if constexpr (std::is_same_v<T, syntax::AssociatedTypeDecl>) {
            // C0X Extension: Associated type declaration
            auto out = node;
            if (node.default_type) {
              const auto resolved_default = ResolveType(ctx, node.default_type);
              if (!resolved_default.ok) {
                return {false, resolved_default.diag_id, resolved_default.span, {}};
              }
              out.default_type = resolved_default.value;
            }
            SPEC_RULE("ResolveClassItem-AssociatedType");
            return {true, std::nullopt, std::nullopt, out};
          } else if constexpr (std::is_same_v<T, syntax::AbstractFieldDecl>) {
            // C0X Extension: Abstract field declaration
            auto out = node;
            const auto resolved_type = ResolveType(ctx, node.type);
            if (!resolved_type.ok) {
              return {false, resolved_type.diag_id, resolved_type.span, {}};
            }
            out.type = resolved_type.value;
            SPEC_RULE("ResolveClassItem-AbstractField");
            return {true, std::nullopt, std::nullopt, out};
          } else if constexpr (std::is_same_v<T, syntax::AbstractStateDecl>) {
            // C0X Extension: Abstract state declaration
            auto out = node;
            for (auto& field : out.fields) {
              const auto resolved_type = ResolveType(ctx, field.type);
              if (!resolved_type.ok) {
                return {false, resolved_type.diag_id, resolved_type.span, {}};
              }
              field.type = resolved_type.value;
            }
            SPEC_RULE("ResolveClassItem-AbstractState");
            return {true, std::nullopt, std::nullopt, out};
          } else {
            // ClassMethodDecl
            auto out = node;
            ScopeContext proc_ctx;
            proc_ctx.project = ctx.ctx->project;
            proc_ctx.sigma = ctx.ctx->sigma;
            proc_ctx.current_module = ctx.ctx->current_module;
            Scope proc_scope;
            for (const auto& param : node.params) {
              proc_scope.emplace(IdKeyOf(param.name),
                                 Entity{EntityKind::Value, std::nullopt,
                                        std::nullopt, EntitySource::Decl});
            }
            proc_scope.emplace(
                IdKeyOf("self"),
                Entity{EntityKind::Value, std::nullopt, std::nullopt,
                       EntitySource::Decl});
            proc_scope.emplace(
                IdKeyOf("Self"),
                Entity{EntityKind::Type, std::nullopt, std::nullopt,
                       EntitySource::Decl});
            SPEC_RULE("BindSelf-Class");
            proc_ctx.scopes = {proc_scope, ModuleScope(ctx.ctx->scopes),
                               UniverseScope(ctx.ctx->scopes)};
            ResolveContext method_ctx = ctx;
            method_ctx.ctx = &proc_ctx;

            const auto resolved_recv = ResolveReceiver(method_ctx, node.receiver);
            if (!resolved_recv.ok) {
              return {false, resolved_recv.diag_id, resolved_recv.span, {}};
            }
            out.receiver = resolved_recv.value;
            const auto resolved_params = ResolveParams(method_ctx, node.params);
            if (!resolved_params.ok) {
              return {false, resolved_params.diag_id, resolved_params.span, {}};
            }
            out.params = resolved_params.value;
            const auto resolved_ret = ResolveTypeOpt(method_ctx, node.return_type_opt);
            if (!resolved_ret.ok) {
              return {false, resolved_ret.diag_id, resolved_ret.span, {}};
            }
            out.return_type_opt = resolved_ret.value;
            if (node.body_opt) {
              const auto resolved_body = ResolveBlock(method_ctx, *node.body_opt);
              if (!resolved_body.ok) {
                return {false, resolved_body.diag_id, resolved_body.span, {}};
              }
              out.body_opt = std::make_shared<syntax::Block>(resolved_body.block);
              SPEC_RULE("ResolveClassItem-Method-Concrete");
            } else {
              SPEC_RULE("ResolveClassItem-Method-Abstract");
            }
            return {true, std::nullopt, std::nullopt, out};
          }
        },
        item);
    if (!resolved.ok) {
      return {false, resolved.diag_id, resolved.span, {}};
    }
    result.value.push_back(std::move(resolved.value));
    SPEC_RULE("ResolveClassItemList-Cons");
  }
  return result;
}

ResolveResult<std::vector<syntax::StateMember>> ResolveStateMemberList(
    ResolveContext& ctx,
    const std::vector<syntax::StateMember>& members) {
  ResolveResult<std::vector<syntax::StateMember>> result;
  result.ok = true;
  if (members.empty()) {
    SPEC_RULE("ResolveStateMemberList-Empty");
    return result;
  }
  result.value.reserve(members.size());
  for (const auto& member : members) {
    auto resolved = std::visit(
        [&](const auto& node) -> ResolveResult<syntax::StateMember> {
          using T = std::decay_t<decltype(node)>;
          if constexpr (std::is_same_v<T, syntax::StateFieldDecl>) {
            auto out = node;
            const auto resolved_type = ResolveType(ctx, node.type);
            if (!resolved_type.ok) {
              return {false, resolved_type.diag_id, resolved_type.span, {}};
            }
            out.type = resolved_type.value;
            SPEC_RULE("ResolveStateMember-Field");
            return {true, std::nullopt, std::nullopt, out};
          } else if constexpr (std::is_same_v<T, syntax::StateMethodDecl>) {
            auto out = node;
            ScopeContext proc_ctx;
            proc_ctx.project = ctx.ctx->project;
            proc_ctx.sigma = ctx.ctx->sigma;
            proc_ctx.current_module = ctx.ctx->current_module;
            Scope proc_scope;
            for (const auto& param : node.params) {
              proc_scope.emplace(IdKeyOf(param.name),
                                 Entity{EntityKind::Value, std::nullopt,
                                        std::nullopt, EntitySource::Decl});
            }
            proc_scope.emplace(
                IdKeyOf("self"),
                Entity{EntityKind::Value, std::nullopt, std::nullopt,
                       EntitySource::Decl});
            proc_ctx.scopes = {proc_scope, ModuleScope(ctx.ctx->scopes),
                               UniverseScope(ctx.ctx->scopes)};
            ResolveContext method_ctx = ctx;
            method_ctx.ctx = &proc_ctx;
            const auto resolved_params = ResolveParams(method_ctx, node.params);
            if (!resolved_params.ok) {
              return {false, resolved_params.diag_id, resolved_params.span, {}};
            }
            out.params = resolved_params.value;
            const auto resolved_ret = ResolveTypeOpt(method_ctx, node.return_type_opt);
            if (!resolved_ret.ok) {
              return {false, resolved_ret.diag_id, resolved_ret.span, {}};
            }
            out.return_type_opt = resolved_ret.value;
            if (node.body) {
              const auto resolved_body = ResolveBlock(method_ctx, *node.body);
              if (!resolved_body.ok) {
                return {false, resolved_body.diag_id, resolved_body.span, {}};
              }
              out.body = std::make_shared<syntax::Block>(resolved_body.block);
            }
            SPEC_RULE("ResolveStateMember-Method");
            return {true, std::nullopt, std::nullopt, out};
          } else {
            auto out = node;
            ScopeContext proc_ctx;
            proc_ctx.project = ctx.ctx->project;
            proc_ctx.sigma = ctx.ctx->sigma;
            proc_ctx.current_module = ctx.ctx->current_module;
            Scope proc_scope;
            for (const auto& param : node.params) {
              proc_scope.emplace(IdKeyOf(param.name),
                                 Entity{EntityKind::Value, std::nullopt,
                                        std::nullopt, EntitySource::Decl});
            }
            proc_scope.emplace(
                IdKeyOf("self"),
                Entity{EntityKind::Value, std::nullopt, std::nullopt,
                       EntitySource::Decl});
            proc_ctx.scopes = {proc_scope, ModuleScope(ctx.ctx->scopes),
                               UniverseScope(ctx.ctx->scopes)};
            ResolveContext method_ctx = ctx;
            method_ctx.ctx = &proc_ctx;
            const auto resolved_params = ResolveParams(method_ctx, node.params);
            if (!resolved_params.ok) {
              return {false, resolved_params.diag_id, resolved_params.span, {}};
            }
            out.params = resolved_params.value;
            if (node.body) {
              const auto resolved_body = ResolveBlock(method_ctx, *node.body);
              if (!resolved_body.ok) {
                return {false, resolved_body.diag_id, resolved_body.span, {}};
              }
              out.body = std::make_shared<syntax::Block>(resolved_body.block);
            }
            SPEC_RULE("ResolveStateMember-Transition");
            return {true, std::nullopt, std::nullopt, out};
          }
        },
        member);
    if (!resolved.ok) {
      return {false, resolved.diag_id, resolved.span, {}};
    }
    result.value.push_back(std::move(resolved.value));
    SPEC_RULE("ResolveStateMemberList-Cons");
  }
  return result;
}

ResolveResult<std::vector<syntax::StateBlock>> ResolveStateBlockList(
    ResolveContext& ctx,
    const std::vector<syntax::StateBlock>& states) {
  ResolveResult<std::vector<syntax::StateBlock>> result;
  result.ok = true;
  if (states.empty()) {
    SPEC_RULE("ResolveStateBlockList-Empty");
    return result;
  }
  result.value.reserve(states.size());
  for (const auto& state : states) {
    auto out = state;
    const auto members = ResolveStateMemberList(ctx, state.members);
    if (!members.ok) {
      return {false, members.diag_id, members.span, {}};
    }
    out.members = members.value;
    SPEC_RULE("ResolveStateBlock");
    result.value.push_back(std::move(out));
    SPEC_RULE("ResolveStateBlockList-Cons");
  }
  return result;
}



}  // namespace

ResolveResult<syntax::ASTItem> ResolveItem(ResolveContext& ctx,
                                           const syntax::ASTItem& item) {
  SpecDefsResolverItems();
  return std::visit(
      [&](const auto& node) -> ResolveResult<syntax::ASTItem> {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::StaticDecl>) {
          auto out = node;
          if (node.binding.pat) {
            const auto resolved_pat = ResolvePattern(ctx, node.binding.pat);
            if (!resolved_pat.ok) {
              return {false, resolved_pat.diag_id, resolved_pat.span, {}};
            }
            out.binding.pat = resolved_pat.value;
          }
          const auto resolved_ty = ResolveTypeOpt(ctx, node.binding.type_opt);
          if (!resolved_ty.ok) {
            return {false, resolved_ty.diag_id, resolved_ty.span, {}};
          }
          out.binding.type_opt = resolved_ty.value;
          const auto resolved_init = ResolveExpr(ctx, node.binding.init);
          if (!resolved_init.ok) {
            return {false, resolved_init.diag_id, resolved_init.span, {}};
          }
          out.binding.init = resolved_init.value;
          SPEC_RULE("ResolveItem-Static");
          return {true, std::nullopt, std::nullopt, out};
        } else if constexpr (std::is_same_v<T, syntax::ProcedureDecl>) {
          auto out = node;
          ScopeContext proc_ctx;
          proc_ctx.project = ctx.ctx->project;
          proc_ctx.sigma = ctx.ctx->sigma;
          proc_ctx.current_module = ctx.ctx->current_module;
          Scope proc_scope;
          for (const auto& param : node.params) {
            proc_scope.emplace(IdKeyOf(param.name),
                               Entity{EntityKind::Value, std::nullopt,
                                      std::nullopt, EntitySource::Decl});
          }
          proc_ctx.scopes = {proc_scope, ModuleScope(ctx.ctx->scopes),
                             UniverseScope(ctx.ctx->scopes)};
          ResolveContext proc_res = ctx;
          proc_res.ctx = &proc_ctx;

          const auto resolved_params = ResolveParams(proc_res, node.params);
          if (!resolved_params.ok) {
            return {false, resolved_params.diag_id, resolved_params.span, {}};
          }
          out.params = resolved_params.value;
          const auto resolved_ret = ResolveTypeOpt(proc_res, node.return_type_opt);
          if (!resolved_ret.ok) {
            return {false, resolved_ret.diag_id, resolved_ret.span, {}};
          }
          out.return_type_opt = resolved_ret.value;
          if (node.body) {
            const auto resolved_body = ResolveBlock(proc_res, *node.body);
            if (!resolved_body.ok) {
              return {false, resolved_body.diag_id, resolved_body.span, {}};
            }
            out.body = std::make_shared<syntax::Block>(resolved_body.block);
          }
          SPEC_RULE("ResolveItem-Procedure");
          return {true, std::nullopt, std::nullopt, out};
        } else if constexpr (std::is_same_v<T, syntax::UsingDecl>) {
          SPEC_RULE("ResolveItem-Using");
          return {true, std::nullopt, std::nullopt, node};
        } else if constexpr (std::is_same_v<T, syntax::TypeAliasDecl>) {
          auto out = node;
          const auto resolved = ResolveType(ctx, node.type);
          if (!resolved.ok) {
            return {false, resolved.diag_id, resolved.span, {}};
          }
          out.type = resolved.value;
          SPEC_RULE("ResolveItem-TypeAlias");
          return {true, std::nullopt, std::nullopt, out};
        } else if constexpr (std::is_same_v<T, syntax::RecordDecl>) {
          auto out = node;
          const auto impls = ResolveClassPathList(ctx, node.implements);
          if (!impls.ok) {
            return {false, impls.diag_id, impls.span, {}};
          }
          out.implements = impls.value;
          const auto invariant = ResolveTypeInvariantOpt(ctx, node.invariant);
          if (!invariant.ok) {
            return {false, invariant.diag_id, invariant.span, {}};
          }
          out.invariant = invariant.value;
          const auto members = ResolveRecordMemberList(ctx, node, node.members);
          if (!members.ok) {
            return {false, members.diag_id, members.span, {}};
          }
          out.members = members.value;
          SPEC_RULE("ResolveItem-Record");
          return {true, std::nullopt, std::nullopt, out};
        } else if constexpr (std::is_same_v<T, syntax::EnumDecl>) {
          auto out = node;
          const auto impls = ResolveClassPathList(ctx, node.implements);
          if (!impls.ok) {
            return {false, impls.diag_id, impls.span, {}};
          }
          out.implements = impls.value;
          const auto invariant = ResolveTypeInvariantOpt(ctx, node.invariant);
          if (!invariant.ok) {
            return {false, invariant.diag_id, invariant.span, {}};
          }
          out.invariant = invariant.value;
          const auto vars = ResolveVariantList(ctx, node.variants);
          if (!vars.ok) {
            return {false, vars.diag_id, vars.span, {}};
          }
          out.variants = vars.value;
          SPEC_RULE("ResolveItem-Enum");
          return {true, std::nullopt, std::nullopt, out};
        } else if constexpr (std::is_same_v<T, syntax::ModalDecl>) {
          auto out = node;
          const auto impls = ResolveClassPathList(ctx, node.implements);
          if (!impls.ok) {
            return {false, impls.diag_id, impls.span, {}};
          }
          out.implements = impls.value;
          const auto invariant = ResolveTypeInvariantOpt(ctx, node.invariant);
          if (!invariant.ok) {
            return {false, invariant.diag_id, invariant.span, {}};
          }
          out.invariant = invariant.value;
          const auto states = ResolveStateBlockList(ctx, node.states);
          if (!states.ok) {
            return {false, states.diag_id, states.span, {}};
          }
          out.states = states.value;
          SPEC_RULE("ResolveItem-Modal");
          return {true, std::nullopt, std::nullopt, out};
        } else if constexpr (std::is_same_v<T, syntax::ClassDecl>) {
          auto out = node;
          const auto supers = ResolveClassPathList(ctx, node.supers);
          if (!supers.ok) {
            return {false, supers.diag_id, supers.span, {}};
          }
          out.supers = supers.value;
          const auto items = ResolveClassItemList(ctx, node.items);
          if (!items.ok) {
            return {false, items.diag_id, items.span, {}};
          }
          out.items = items.value;
          SPEC_RULE("ResolveItem-Class");
          return {true, std::nullopt, std::nullopt, out};
        } else {
          SPEC_RULE("ResolveItem-Error");
          return {true, std::nullopt, std::nullopt, node};
        }
      },
      item);
}

}  // namespace cursive0::analysis
