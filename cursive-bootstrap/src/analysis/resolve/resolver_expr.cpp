#include "cursive0/analysis/resolve/resolver.h"

#include <utility>
#include <type_traits>

#include "cursive0/core/assert_spec.h"
#include "cursive0/analysis/resolve/collect_toplevel.h"
#include "cursive0/analysis/resolve/resolve_qual.h"
#include "cursive0/analysis/resolve/scopes.h"
#include "cursive0/analysis/resolve/scopes_intro.h"
#include "cursive0/analysis/resolve/visibility.h"

namespace cursive0::analysis {

namespace {

static inline void SpecDefsResolverExpr() {
  SPEC_DEF("ResolveExpr", "5.1.7");
  SPEC_DEF("ResolveStmt", "5.1.7");
  SPEC_DEF("ResolveStmtSeq", "5.1.7");
  SPEC_DEF("ResolveExprList", "5.1.7");
  SPEC_DEF("ResolveEnumPayload", "5.1.7");
  SPEC_DEF("ResolveCallee", "5.1.7");
  SPEC_DEF("ResolveArm", "5.1.7");
  SPEC_DEF("BindNames", "5.1.7");
  SPEC_DEF("BindPattern", "5.1.7");
  SPEC_DEF("ResolveExprOpt", "5.1.7");
  SPEC_DEF("ResolveTypeOpt", "5.1.7");
  SPEC_DEF("ResolveTypeRef", "5.1.7");
}

struct ScopeGuard {
  ScopeContext* ctx = nullptr;
  bool active = false;

  explicit ScopeGuard(ScopeContext& ctx_in) : ctx(&ctx_in), active(true) {
    ctx->scopes.insert(ctx->scopes.begin(), Scope{});
  }

  ~ScopeGuard() {
    if (active && ctx && !ctx->scopes.empty()) {
      ctx->scopes.erase(ctx->scopes.begin());
    }
  }

  ScopeGuard(const ScopeGuard&) = delete;
  ScopeGuard& operator=(const ScopeGuard&) = delete;
};

syntax::ExprPtr MakeExpr(const core::Span& span, syntax::ExprNode node) {
  auto expr = std::make_shared<syntax::Expr>();
  expr->span = span;
  expr->node = std::move(node);
  return expr;
}

syntax::Path FullPath(const syntax::Path& path, std::string_view name) {
  syntax::Path out = path;
  out.emplace_back(name);
  return out;
}

ResolveQualContext BuildResolveQualContext(ResolveContext& ctx) {
  ResolveQualContext qual_ctx;
  qual_ctx.ctx = ctx.ctx;
  qual_ctx.name_maps = ctx.name_maps;
  qual_ctx.module_names = ctx.module_names;
  qual_ctx.can_access = ctx.can_access;
  qual_ctx.resolve_expr = [](const ScopeContext& qctx,
                             const NameMapTable& name_maps,
                             const ModuleNames& module_names,
                             const syntax::ExprPtr& expr) {
    ResolveContext local_ctx;
    local_ctx.ctx = const_cast<ScopeContext*>(&qctx);
    local_ctx.name_maps = &name_maps;
    local_ctx.module_names = &module_names;
    local_ctx.can_access = CanAccess;
    const auto resolved = ResolveExpr(local_ctx, expr);
    return ResolveExprResult{resolved.ok, resolved.diag_id, resolved.value};
  };
  qual_ctx.resolve_type_path = [](const ScopeContext& qctx,
                                  const NameMapTable& name_maps,
                                  const ModuleNames& module_names,
                                  const syntax::TypePath& path) {
    ResolveContext local_ctx;
    local_ctx.ctx = const_cast<ScopeContext*>(&qctx);
    local_ctx.name_maps = &name_maps;
    local_ctx.module_names = &module_names;
    local_ctx.can_access = CanAccess;
    const auto resolved = ResolveTypePath(local_ctx, path);
    return ResolveTypePathResult{resolved.ok, resolved.diag_id, resolved.value};
  };
  return qual_ctx;
}

struct BindNamesResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  std::optional<core::Span> span;
};

BindNamesResult BindNames(ResolveContext& ctx,
                          const std::vector<syntax::Identifier>& names,
                          const std::optional<core::Span>& span) {
  if (!ctx.ctx) {
    return {false, std::nullopt, span};
  }
  for (const auto& name : names) {
    const auto res = Intro(*ctx.ctx, name,
                           Entity{EntityKind::Value, std::nullopt, std::nullopt,
                                  EntitySource::Decl});
    if (!res.ok) {
      return {false, res.diag_id, span};
    }
  }
  SPEC_RULE("BindNames-Cons");
  return {true, std::nullopt, span};
}

BindNamesResult BindPattern(ResolveContext& ctx,
                            const syntax::PatternPtr& pat) {
  if (!pat) {
    return {true, std::nullopt, std::nullopt};
  }
  const auto names = PatNames(pat);
  if (names.empty()) {
    SPEC_RULE("BindNames-Empty");
    return {true, std::nullopt, pat->span};
  }
  const auto result = BindNames(ctx, names, pat->span);
  if (result.ok) {
    SPEC_RULE("BindPattern");
  }
  return result;
}

ResExprResult ResolveExprOpt(ResolveContext& ctx,
                            const syntax::ExprPtr& expr_opt) {
  if (!expr_opt) {
    SPEC_RULE("ResolveExprOpt-None");
    return {true, std::nullopt, std::nullopt, expr_opt};
  }
  const auto resolved = ResolveExpr(ctx, expr_opt);
  if (!resolved.ok) {
    return {false, resolved.diag_id, resolved.span, {}};
  }
  SPEC_RULE("ResolveExprOpt-Some");
  return {true, std::nullopt, std::nullopt, resolved.value};
}

ResTypeResult ResolveTypeOpt(ResolveContext& ctx,
                             const std::shared_ptr<syntax::Type>& type_opt) {
  if (!type_opt) {
    SPEC_RULE("ResolveTypeOpt-None");
    return {true, std::nullopt, std::nullopt, type_opt};
  }
  const auto resolved = ResolveType(ctx, type_opt);
  if (!resolved.ok) {
    return {false, resolved.diag_id, resolved.span, {}};
  }
  SPEC_RULE("ResolveTypeOpt-Some");
  return {true, std::nullopt, std::nullopt, resolved.value};
}

ResolveResult<std::vector<syntax::Arg>> ResolveArgs(ResolveContext& ctx,
                                                    const std::vector<syntax::Arg>& args) {
  ResolveResult<std::vector<syntax::Arg>> result;
  result.ok = true;
  if (args.empty()) {
    SPEC_RULE("ResolveExprList-Empty");
    return result;
  }
  result.value.reserve(args.size());
  for (const auto& arg : args) {
    const auto resolved = ResolveExpr(ctx, arg.value);
    if (!resolved.ok) {
      return {false, resolved.diag_id, resolved.span, {}};
    }
    syntax::Arg next = arg;
    next.value = resolved.value;
    result.value.push_back(std::move(next));
    SPEC_RULE("ResolveExprList-Cons");
  }
  return result;
}

ResolveResult<std::vector<syntax::FieldInit>> ResolveFieldInits(
    ResolveContext& ctx,
    const std::vector<syntax::FieldInit>& fields) {
  ResolveResult<std::vector<syntax::FieldInit>> result;
  result.ok = true;
  result.value.reserve(fields.size());
  for (const auto& field : fields) {
    const auto resolved = ResolveExpr(ctx, field.value);
    if (!resolved.ok) {
      return {false, resolved.diag_id, resolved.span, {}};
    }
    syntax::FieldInit next = field;
    next.value = resolved.value;
    result.value.push_back(std::move(next));
  }
  return result;
}

ResolveResult<std::optional<syntax::EnumPayload>> ResolveEnumPayload(
    ResolveContext& ctx,
    const std::optional<syntax::EnumPayload>& payload_opt) {
  ResolveResult<std::optional<syntax::EnumPayload>> result;
  result.ok = true;
  if (!payload_opt.has_value()) {
    SPEC_RULE("ResolveEnumPayload-None");
    return result;
  }
  return std::visit(
      [&](const auto& node) -> ResolveResult<std::optional<syntax::EnumPayload>> {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::EnumPayloadParen>) {
          std::vector<syntax::ExprPtr> elems;
          elems.reserve(node.elements.size());
          for (const auto& elem : node.elements) {
            const auto resolved = ResolveExpr(ctx, elem);
            if (!resolved.ok) {
              return {false, resolved.diag_id, resolved.span, std::nullopt};
            }
            elems.push_back(resolved.value);
          }
          syntax::EnumPayloadParen out;
          out.elements = std::move(elems);
          SPEC_RULE("ResolveEnumPayload-Tuple");
          return {true, std::nullopt, std::nullopt, out};
        } else {
          const auto fields = ResolveFieldInits(ctx, node.fields);
          if (!fields.ok) {
            return {false, fields.diag_id, fields.span, std::nullopt};
          }
          syntax::EnumPayloadBrace out;
          out.fields = fields.value;
          SPEC_RULE("ResolveEnumPayload-Record");
          return {true, std::nullopt, std::nullopt, out};
        }
      },
      *payload_opt);
}

ResolveResult<std::variant<syntax::TypePath, syntax::ModalStateRef>>
ResolveTypeRef(ResolveContext& ctx,
               const std::variant<syntax::TypePath, syntax::ModalStateRef>& target) {
  ResolveResult<std::variant<syntax::TypePath, syntax::ModalStateRef>> result;
  return std::visit(
      [&](const auto& node)
          -> ResolveResult<std::variant<syntax::TypePath, syntax::ModalStateRef>> {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::TypePath>) {
          const auto resolved = ResolveTypePath(ctx, node);
          if (!resolved.ok) {
            return {false, resolved.diag_id, resolved.span, {}};
          }
          SPEC_RULE("ResolveTypeRef-Path");
          result.ok = true;
          result.value = resolved.value;
          return result;
        } else {
          const auto resolved = ResolveTypePath(ctx, node.path);
          if (!resolved.ok) {
            return {false, resolved.diag_id, resolved.span, {}};
          }
          syntax::ModalStateRef out = node;
          out.path = resolved.value;
          SPEC_RULE("ResolveTypeRef-ModalState");
          result.ok = true;
          result.value = out;
          return result;
        }
      },
      target);
}

ResExprResult ResolveCallee(ResolveContext& ctx,
                            const syntax::ExprPtr& callee,
                            const std::vector<syntax::Arg>& args) {
  if (!callee) {
    return {false, "ResolveExpr-Ident-Err", std::nullopt, {}};
  }
  return std::visit(
      [&](const auto& node) -> ResExprResult {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
          const auto ent = ResolveValueName(*ctx.ctx, node.name);
          if (ent.has_value()) {
            SPEC_RULE("ResolveCallee-Ident-Value");
            return {true, std::nullopt, std::nullopt, callee};
          }
          if (args.empty() && IdEq(node.name, "RegionOptions")) {
            SPEC_RULE("ResolveCallee-Ident-Record");
            return {true, std::nullopt, std::nullopt, callee};
          }
          if (args.empty()) {
            const auto type_ent = ResolveTypeName(*ctx.ctx, node.name);
            if (type_ent.has_value()) {
              const auto name = type_ent->target_opt.value_or(node.name);
              syntax::ModulePath module;
              if (type_ent->origin_opt.has_value()) {
                module = *type_ent->origin_opt;
              }
              const auto path = FullPath(module, name);
              const auto it = ctx.ctx->sigma.types.find(PathKeyOf(path));
              if (it != ctx.ctx->sigma.types.end() &&
                  std::holds_alternative<syntax::RecordDecl>(it->second)) {
                SPEC_RULE("ResolveCallee-Ident-Record");
                return {true, std::nullopt, std::nullopt, callee};
              }
            }
          }
          return ResolveExpr(ctx, callee);
        } else if constexpr (std::is_same_v<T, syntax::PathExpr>) {
          const auto value = ResolveQualified(
              *ctx.ctx, *ctx.name_maps, *ctx.module_names, node.path, node.name,
              EntityKind::Value, ctx.can_access);
          if (value.ok) {
            SPEC_RULE("ResolveCallee-Path-Value");
            return {true, std::nullopt, std::nullopt, callee};
          }
          const ResolveQualContext qual_ctx = BuildResolveQualContext(ctx);
          if (args.empty()) {
            const auto record = ResolveRecordPath(qual_ctx, node.path, node.name);
            if (record.ok) {
              SPEC_RULE("ResolveCallee-Path-Record");
              return {true, std::nullopt, std::nullopt, callee};
            }
          }
          return ResolveExpr(ctx, callee);
        } else {
          SPEC_RULE("ResolveCallee-Other");
          return ResolveExpr(ctx, callee);
        }
      },
      callee->node);
}

ResolveResult<syntax::MatchArm> ResolveArm(ResolveContext& ctx,
                                          const syntax::MatchArm& arm) {
  if (!ctx.ctx) {
    return {false, std::nullopt, std::nullopt, {}};
  }
  ScopeGuard guard(*ctx.ctx);
  const auto resolved_pat = ResolvePattern(ctx, arm.pattern);
  if (!resolved_pat.ok) {
    return {false, resolved_pat.diag_id, resolved_pat.span, {}};
  }
  const auto bind = BindPattern(ctx, resolved_pat.value);
  if (!bind.ok) {
    return {false, bind.diag_id, bind.span, {}};
  }
  const auto resolved_guard = ResolveExprOpt(ctx, arm.guard_opt);
  if (!resolved_guard.ok) {
    return {false, resolved_guard.diag_id, resolved_guard.span, {}};
  }
  const auto resolved_body = ResolveExpr(ctx, arm.body);
  if (!resolved_body.ok) {
    return {false, resolved_body.diag_id, resolved_body.span, {}};
  }
  syntax::MatchArm out = arm;
  out.pattern = resolved_pat.value;
  out.guard_opt = resolved_guard.value;
  out.body = resolved_body.value;
  SPEC_RULE("ResolveArm");
  return {true, std::nullopt, std::nullopt, out};
}

ResolveResult<std::vector<syntax::MatchArm>> ResolveArms(
    ResolveContext& ctx,
    const std::vector<syntax::MatchArm>& arms) {
  ResolveResult<std::vector<syntax::MatchArm>> result;
  result.ok = true;
  if (arms.empty()) {
    SPEC_RULE("ResolveArms-Empty");
    return result;
  }
  result.value.reserve(arms.size());
  for (const auto& arm : arms) {
    const auto resolved = ResolveArm(ctx, arm);
    if (!resolved.ok) {
      return {false, resolved.diag_id, resolved.span, {}};
    }
    result.value.push_back(resolved.value);
    SPEC_RULE("ResolveArms-Cons");
  }
  return result;
}

}  // namespace

ResExprResult ResolveExpr(ResolveContext& ctx,
                          const syntax::ExprPtr& expr) {
  SpecDefsResolverExpr();
  ResExprResult result;
  if (!expr || !ctx.ctx || !ctx.name_maps || !ctx.module_names) {
    return result;
  }

  ResolveQualContext qual_ctx = BuildResolveQualContext(ctx);

  return std::visit(
      [&](const auto& node) -> ResExprResult {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
          const auto ent = ResolveValueName(*ctx.ctx, node.name);
          if (!ent.has_value()) {
            SPEC_RULE("ResolveExpr-Ident-Err");
            return {false, "ResolveExpr-Ident-Err", expr->span, {}};
          }
          SPEC_RULE("ResolveExpr-Ident");
          return {true, std::nullopt, std::nullopt, expr};
        } else if constexpr (std::is_same_v<T, syntax::QualifiedNameExpr> ||
                             std::is_same_v<T, syntax::QualifiedApplyExpr>) {
          const auto resolved = ResolveQualifiedForm(qual_ctx, *expr);
          if (!resolved.ok) {
            const auto diag = resolved.diag_id.value_or("ResolveExpr-Ident-Err");
            SPEC_RULE("ResolveExpr-Qualified-Err");
            return {false, diag, expr->span, {}};
          }
          SPEC_RULE("ResolveExpr-Qualified");
          return {true, std::nullopt, std::nullopt, resolved.expr};
        } else if constexpr (std::is_same_v<T, syntax::PathExpr>) {
          return {true, std::nullopt, std::nullopt, expr};
        } else if constexpr (std::is_same_v<T, syntax::EnumLiteralExpr>) {
          const auto payload = ResolveEnumPayload(ctx, node.payload_opt);
          if (!payload.ok) {
            return {false, payload.diag_id, payload.span, {}};
          }
          auto out = *expr;
          auto& out_node = std::get<syntax::EnumLiteralExpr>(out.node);
          out_node.payload_opt = payload.value;
          SPEC_RULE("ResolveExpr-EnumLiteral");
          return {true, std::nullopt, std::nullopt,
                  std::make_shared<syntax::Expr>(std::move(out))};
        } else if constexpr (std::is_same_v<T, syntax::CallExpr>) {
          const auto resolved_args = ResolveArgs(ctx, node.args);
          if (!resolved_args.ok) {
            return {false, resolved_args.diag_id, resolved_args.span, {}};
          }
          const auto resolved_callee = ResolveCallee(ctx, node.callee, resolved_args.value);
          if (!resolved_callee.ok) {
            return {false, resolved_callee.diag_id, resolved_callee.span, {}};
          }
          auto out = *expr;
          auto& out_node = std::get<syntax::CallExpr>(out.node);
          out_node.callee = resolved_callee.value;
          out_node.args = resolved_args.value;
          SPEC_RULE("ResolveExpr-Call");
          return {true, std::nullopt, std::nullopt,
                  std::make_shared<syntax::Expr>(std::move(out))};
        } else if constexpr (std::is_same_v<T, syntax::RecordExpr>) {
          const auto target = ResolveTypeRef(ctx, node.target);
          if (!target.ok) {
            return {false, target.diag_id, target.span, {}};
          }
          const auto fields = ResolveFieldInits(ctx, node.fields);
          if (!fields.ok) {
            return {false, fields.diag_id, fields.span, {}};
          }
          auto out = *expr;
          auto& out_node = std::get<syntax::RecordExpr>(out.node);
          out_node.target = target.value;
          out_node.fields = fields.value;
          SPEC_RULE("ResolveExpr-RecordExpr");
          return {true, std::nullopt, std::nullopt,
                  std::make_shared<syntax::Expr>(std::move(out))};
        } else if constexpr (std::is_same_v<T, syntax::MatchExpr>) {
          const auto resolved_scrutinee = ResolveExpr(ctx, node.value);
          if (!resolved_scrutinee.ok) {
            return {false, resolved_scrutinee.diag_id, resolved_scrutinee.span, {}};
          }
          const auto resolved_arms = ResolveArms(ctx, node.arms);
          if (!resolved_arms.ok) {
            return {false, resolved_arms.diag_id, resolved_arms.span, {}};
          }
          auto out = *expr;
          auto& out_node = std::get<syntax::MatchExpr>(out.node);
          out_node.value = resolved_scrutinee.value;
          out_node.arms = resolved_arms.value;
          SPEC_RULE("ResolveExpr-Match");
          return {true, std::nullopt, std::nullopt,
                  std::make_shared<syntax::Expr>(std::move(out))};
        } else if constexpr (std::is_same_v<T, syntax::LoopIterExpr>) {
          const auto resolved_pat = ResolvePattern(ctx, node.pattern);
          if (!resolved_pat.ok) {
            return {false, resolved_pat.diag_id, resolved_pat.span, {}};
          }
          const auto resolved_ty = ResolveTypeOpt(ctx, node.type_opt);
          if (!resolved_ty.ok) {
            return {false, resolved_ty.diag_id, resolved_ty.span, {}};
          }
          const auto resolved_iter = ResolveExpr(ctx, node.iter);
          if (!resolved_iter.ok) {
            return {false, resolved_iter.diag_id, resolved_iter.span, {}};
          }
          ScopeGuard guard(*ctx.ctx);
          const auto bind = BindPattern(ctx, resolved_pat.value);
          if (!bind.ok) {
            return {false, bind.diag_id, bind.span, {}};
          }
          syntax::LoopIterExpr out = node;
          out.pattern = resolved_pat.value;
          out.type_opt = resolved_ty.value;
          out.iter = resolved_iter.value;
          if (node.body) {
            const auto body = ResolveBlock(ctx, *node.body);
            if (!body.ok) {
              return {false, body.diag_id, body.span, {}};
            }
            out.body = std::make_shared<syntax::Block>(body.block);
          }
          SPEC_RULE("ResolveExpr-LoopIter");
          return {true, std::nullopt, std::nullopt,
                  MakeExpr(expr->span, out)};
        } else if constexpr (std::is_same_v<T, syntax::BlockExpr>) {
          if (!node.block) {
            return {true, std::nullopt, std::nullopt, expr};
          }
          const auto block = ResolveBlock(ctx, *node.block);
          if (!block.ok) {
            return {false, block.diag_id, block.span, {}};
          }
          auto out = *expr;
          auto& out_node = std::get<syntax::BlockExpr>(out.node);
          out_node.block = std::make_shared<syntax::Block>(block.block);
          SPEC_RULE("ResolveExpr-Block");
          return {true, std::nullopt, std::nullopt,
                  std::make_shared<syntax::Expr>(std::move(out))};
        } else if constexpr (std::is_same_v<T, syntax::UnsafeBlockExpr>) {
          if (!node.block) {
            return {true, std::nullopt, std::nullopt, expr};
          }
          const auto block = ResolveBlock(ctx, *node.block);
          if (!block.ok) {
            return {false, block.diag_id, block.span, {}};
          }
          auto out = *expr;
          auto& out_node = std::get<syntax::UnsafeBlockExpr>(out.node);
          out_node.block = std::make_shared<syntax::Block>(block.block);
          SPEC_RULE("ResolveExpr-Block");
          return {true, std::nullopt, std::nullopt,
                  std::make_shared<syntax::Expr>(std::move(out))};
        } else if constexpr (std::is_same_v<T, syntax::AllocExpr>) {
          if (node.region_opt.has_value()) {
            const auto ent = ResolveValueName(*ctx.ctx, *node.region_opt);
            if (!ent.has_value()) {
              return {false, "ResolveExpr-Ident-Err", expr->span, {}};
            }
          }
          const auto resolved = ResolveExpr(ctx, node.value);
          if (!resolved.ok) {
            return {false, resolved.diag_id, resolved.span, {}};
          }
          auto out = *expr;
          auto& out_node = std::get<syntax::AllocExpr>(out.node);
          out_node.value = resolved.value;
          if (node.region_opt.has_value()) {
            SPEC_RULE("ResolveExpr-Alloc-Explicit");
          } else {
            SPEC_RULE("ResolveExpr-Alloc-Implicit");
          }
          return {true, std::nullopt, std::nullopt,
                  std::make_shared<syntax::Expr>(std::move(out))};
        } else if constexpr (std::is_same_v<T, syntax::BinaryExpr>) {
          if (node.op == "^") {
            if (node.lhs && std::holds_alternative<syntax::IdentifierExpr>(node.lhs->node)) {
              const auto& ident = std::get<syntax::IdentifierExpr>(node.lhs->node).name;
              const auto ent = ResolveValueName(*ctx.ctx, ident);
              if (ent.has_value() && RegionAlias(*ent)) {
                const auto resolved_rhs = ResolveExpr(ctx, node.rhs);
                if (!resolved_rhs.ok) {
                  return {false, resolved_rhs.diag_id, resolved_rhs.span, {}};
                }
                syntax::AllocExpr alloc;
                alloc.region_opt = ident;
                alloc.value = resolved_rhs.value;
                SPEC_RULE("ResolveExpr-Alloc-Explicit-ByAlias");
                return {true, std::nullopt, std::nullopt,
                        MakeExpr(expr->span, alloc)};
              }
            }
          }
          const auto resolved_lhs = ResolveExpr(ctx, node.lhs);
          if (!resolved_lhs.ok) {
            return {false, resolved_lhs.diag_id, resolved_lhs.span, {}};
          }
          const auto resolved_rhs = ResolveExpr(ctx, node.rhs);
          if (!resolved_rhs.ok) {
            return {false, resolved_rhs.diag_id, resolved_rhs.span, {}};
          }
          auto out = *expr;
          auto& out_node = std::get<syntax::BinaryExpr>(out.node);
          out_node.lhs = resolved_lhs.value;
          out_node.rhs = resolved_rhs.value;
          SPEC_RULE("ResolveExpr-Hom");
          return {true, std::nullopt, std::nullopt,
                  std::make_shared<syntax::Expr>(std::move(out))};
        } else if constexpr (std::is_same_v<T, syntax::UnaryExpr>) {
          const auto resolved = ResolveExpr(ctx, node.value);
          if (!resolved.ok) {
            return {false, resolved.diag_id, resolved.span, {}};
          }
          auto out = *expr;
          auto& out_node = std::get<syntax::UnaryExpr>(out.node);
          out_node.value = resolved.value;
          SPEC_RULE("ResolveExpr-Hom");
          return {true, std::nullopt, std::nullopt,
                  std::make_shared<syntax::Expr>(std::move(out))};
        } else if constexpr (std::is_same_v<T, syntax::CastExpr>) {
          const auto resolved_val = ResolveExpr(ctx, node.value);
          if (!resolved_val.ok) {
            return {false, resolved_val.diag_id, resolved_val.span, {}};
          }
          const auto resolved_ty = ResolveType(ctx, node.type);
          if (!resolved_ty.ok) {
            return {false, resolved_ty.diag_id, resolved_ty.span, {}};
          }
          auto out = *expr;
          auto& out_node = std::get<syntax::CastExpr>(out.node);
          out_node.value = resolved_val.value;
          out_node.type = resolved_ty.value;
          SPEC_RULE("ResolveExpr-Hom");
          return {true, std::nullopt, std::nullopt,
                  std::make_shared<syntax::Expr>(std::move(out))};
        } else if constexpr (std::is_same_v<T, syntax::RangeExpr>) {
          auto out = *expr;
          auto& out_node = std::get<syntax::RangeExpr>(out.node);
          if (node.lhs) {
            const auto resolved = ResolveExpr(ctx, node.lhs);
            if (!resolved.ok) {
              return {false, resolved.diag_id, resolved.span, {}};
            }
            out_node.lhs = resolved.value;
          }
          if (node.rhs) {
            const auto resolved = ResolveExpr(ctx, node.rhs);
            if (!resolved.ok) {
              return {false, resolved.diag_id, resolved.span, {}};
            }
            out_node.rhs = resolved.value;
          }
          SPEC_RULE("ResolveExpr-Hom");
          return {true, std::nullopt, std::nullopt,
                  std::make_shared<syntax::Expr>(std::move(out))};
        } else if constexpr (std::is_same_v<T, syntax::DerefExpr>) {
          const auto resolved = ResolveExpr(ctx, node.value);
          if (!resolved.ok) {
            return {false, resolved.diag_id, resolved.span, {}};
          }
          auto out = *expr;
          auto& out_node = std::get<syntax::DerefExpr>(out.node);
          out_node.value = resolved.value;
          SPEC_RULE("ResolveExpr-Hom");
          return {true, std::nullopt, std::nullopt,
                  std::make_shared<syntax::Expr>(std::move(out))};
        } else if constexpr (std::is_same_v<T, syntax::AddressOfExpr>) {
          const auto resolved = ResolveExpr(ctx, node.place);
          if (!resolved.ok) {
            return {false, resolved.diag_id, resolved.span, {}};
          }
          auto out = *expr;
          auto& out_node = std::get<syntax::AddressOfExpr>(out.node);
          out_node.place = resolved.value;
          SPEC_RULE("ResolveExpr-Hom");
          return {true, std::nullopt, std::nullopt,
                  std::make_shared<syntax::Expr>(std::move(out))};
        } else if constexpr (std::is_same_v<T, syntax::MoveExpr>) {
          const auto resolved = ResolveExpr(ctx, node.place);
          if (!resolved.ok) {
            return {false, resolved.diag_id, resolved.span, {}};
          }
          auto out = *expr;
          auto& out_node = std::get<syntax::MoveExpr>(out.node);
          out_node.place = resolved.value;
          SPEC_RULE("ResolveExpr-Hom");
          return {true, std::nullopt, std::nullopt,
                  std::make_shared<syntax::Expr>(std::move(out))};
        } else if constexpr (std::is_same_v<T, syntax::TupleExpr>) {
          auto out = *expr;
          auto& out_node = std::get<syntax::TupleExpr>(out.node);
          for (auto& elem : out_node.elements) {
            const auto resolved = ResolveExpr(ctx, elem);
            if (!resolved.ok) {
              return {false, resolved.diag_id, resolved.span, {}};
            }
            elem = resolved.value;
          }
          SPEC_RULE("ResolveExpr-Hom");
          return {true, std::nullopt, std::nullopt,
                  std::make_shared<syntax::Expr>(std::move(out))};
        } else if constexpr (std::is_same_v<T, syntax::ArrayExpr>) {
          auto out = *expr;
          auto& out_node = std::get<syntax::ArrayExpr>(out.node);
          for (auto& elem : out_node.elements) {
            const auto resolved = ResolveExpr(ctx, elem);
            if (!resolved.ok) {
              return {false, resolved.diag_id, resolved.span, {}};
            }
            elem = resolved.value;
          }
          SPEC_RULE("ResolveExpr-Hom");
          return {true, std::nullopt, std::nullopt,
                  std::make_shared<syntax::Expr>(std::move(out))};
        } else if constexpr (std::is_same_v<T, syntax::IfExpr>) {
          const auto resolved_cond = ResolveExpr(ctx, node.cond);
          if (!resolved_cond.ok) {
            return {false, resolved_cond.diag_id, resolved_cond.span, {}};
          }
          const auto resolved_then = ResolveExpr(ctx, node.then_expr);
          if (!resolved_then.ok) {
            return {false, resolved_then.diag_id, resolved_then.span, {}};
          }
          std::shared_ptr<syntax::Expr> resolved_else_expr = nullptr;
          if (node.else_expr) {
            const auto resolved_else = ResolveExpr(ctx, node.else_expr);
            if (!resolved_else.ok) {
              return {false, resolved_else.diag_id, resolved_else.span, {}};
            }
            resolved_else_expr = resolved_else.value;
          }
          auto out = *expr;
          auto& out_node = std::get<syntax::IfExpr>(out.node);
          out_node.cond = resolved_cond.value;
          out_node.then_expr = resolved_then.value;
          out_node.else_expr = resolved_else_expr;
          SPEC_RULE("ResolveExpr-Hom");
          return {true, std::nullopt, std::nullopt,
                  std::make_shared<syntax::Expr>(std::move(out))};
        } else if constexpr (std::is_same_v<T, syntax::LoopInfiniteExpr>) {
          auto out = *expr;
          auto& out_node = std::get<syntax::LoopInfiniteExpr>(out.node);
          if (node.body) {
            const auto body = ResolveBlock(ctx, *node.body);
            if (!body.ok) {
              return {false, body.diag_id, body.span, {}};
            }
            out_node.body = std::make_shared<syntax::Block>(body.block);
          }
          SPEC_RULE("ResolveExpr-Hom");
          return {true, std::nullopt, std::nullopt,
                  std::make_shared<syntax::Expr>(std::move(out))};
        } else if constexpr (std::is_same_v<T, syntax::LoopConditionalExpr>) {
          const auto cond = ResolveExpr(ctx, node.cond);
          if (!cond.ok) {
            return {false, cond.diag_id, cond.span, {}};
          }
          auto out = *expr;
          auto& out_node = std::get<syntax::LoopConditionalExpr>(out.node);
          out_node.cond = cond.value;
          if (node.body) {
            const auto body = ResolveBlock(ctx, *node.body);
            if (!body.ok) {
              return {false, body.diag_id, body.span, {}};
            }
            out_node.body = std::make_shared<syntax::Block>(body.block);
          }
          SPEC_RULE("ResolveExpr-Hom");
          return {true, std::nullopt, std::nullopt,
                  std::make_shared<syntax::Expr>(std::move(out))};
        } else if constexpr (std::is_same_v<T, syntax::TransmuteExpr>) {
          const auto val = ResolveExpr(ctx, node.value);
          if (!val.ok) {
            return {false, val.diag_id, val.span, {}};
          }
          const auto src = ResolveType(ctx, node.from);
          if (!src.ok) {
            return {false, src.diag_id, src.span, {}};
          }
          const auto dst = ResolveType(ctx, node.to);
          if (!dst.ok) {
            return {false, dst.diag_id, dst.span, {}};
          }
          auto out = *expr;
          auto& out_node = std::get<syntax::TransmuteExpr>(out.node);
          out_node.value = val.value;
          out_node.from = src.value;
          out_node.to = dst.value;
          SPEC_RULE("ResolveExpr-Hom");
          return {true, std::nullopt, std::nullopt,
                  std::make_shared<syntax::Expr>(std::move(out))};
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
          const auto base = ResolveExpr(ctx, node.base);
          if (!base.ok) {
            return {false, base.diag_id, base.span, {}};
          }
          auto out = *expr;
          auto& out_node = std::get<syntax::FieldAccessExpr>(out.node);
          out_node.base = base.value;
          SPEC_RULE("ResolveExpr-Hom");
          return {true, std::nullopt, std::nullopt,
                  std::make_shared<syntax::Expr>(std::move(out))};
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
          const auto base = ResolveExpr(ctx, node.base);
          if (!base.ok) {
            return {false, base.diag_id, base.span, {}};
          }
          auto out = *expr;
          auto& out_node = std::get<syntax::TupleAccessExpr>(out.node);
          out_node.base = base.value;
          SPEC_RULE("ResolveExpr-Hom");
          return {true, std::nullopt, std::nullopt,
                  std::make_shared<syntax::Expr>(std::move(out))};
        } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
          const auto base = ResolveExpr(ctx, node.base);
          if (!base.ok) {
            return {false, base.diag_id, base.span, {}};
          }
          const auto index = ResolveExpr(ctx, node.index);
          if (!index.ok) {
            return {false, index.diag_id, index.span, {}};
          }
          auto out = *expr;
          auto& out_node = std::get<syntax::IndexAccessExpr>(out.node);
          out_node.base = base.value;
          out_node.index = index.value;
          SPEC_RULE("ResolveExpr-Hom");
          return {true, std::nullopt, std::nullopt,
                  std::make_shared<syntax::Expr>(std::move(out))};
        } else if constexpr (std::is_same_v<T, syntax::MethodCallExpr>) {
          const auto recv = ResolveExpr(ctx, node.receiver);
          if (!recv.ok) {
            return {false, recv.diag_id, recv.span, {}};
          }
          const auto args = ResolveArgs(ctx, node.args);
          if (!args.ok) {
            return {false, args.diag_id, args.span, {}};
          }
          auto out = *expr;
          auto& out_node = std::get<syntax::MethodCallExpr>(out.node);
          out_node.receiver = recv.value;
          out_node.args = args.value;
          SPEC_RULE("ResolveExpr-Hom");
          return {true, std::nullopt, std::nullopt,
                  std::make_shared<syntax::Expr>(std::move(out))};
        } else if constexpr (std::is_same_v<T, syntax::PropagateExpr>) {
          const auto val = ResolveExpr(ctx, node.value);
          if (!val.ok) {
            return {false, val.diag_id, val.span, {}};
          }
          auto out = *expr;
          auto& out_node = std::get<syntax::PropagateExpr>(out.node);
          out_node.value = val.value;
          SPEC_RULE("ResolveExpr-Hom");
          return {true, std::nullopt, std::nullopt,
                  std::make_shared<syntax::Expr>(std::move(out))};
        } else {
          return {true, std::nullopt, std::nullopt, expr};
        }
      },
      expr->node);
}

ResolveStmtResult ResolveStmt(ResolveContext& ctx,
                              const syntax::Stmt& stmt) {
  SpecDefsResolverExpr();
  return std::visit(
      [&](const auto& node) -> ResolveStmtResult {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::LetStmt>) {
          auto out = node;
          const auto init = ResolveExpr(ctx, node.binding.init);
          if (!init.ok) {
            return {false, init.diag_id, init.span, {}};
          }
          out.binding.init = init.value;
          const auto ty = ResolveTypeOpt(ctx, node.binding.type_opt);
          if (!ty.ok) {
            return {false, ty.diag_id, ty.span, {}};
          }
          out.binding.type_opt = ty.value;
          if (node.binding.pat) {
            const auto pat = ResolvePattern(ctx, node.binding.pat);
            if (!pat.ok) {
              return {false, pat.diag_id, pat.span, {}};
            }
            out.binding.pat = pat.value;
            const auto bind = BindPattern(ctx, pat.value);
            if (!bind.ok) {
              return {false, bind.diag_id, bind.span, {}};
            }
          }
          SPEC_RULE("ResolveStmt-Let");
          return {true, std::nullopt, std::nullopt, out};
        } else if constexpr (std::is_same_v<T, syntax::VarStmt>) {
          auto out = node;
          const auto init = ResolveExpr(ctx, node.binding.init);
          if (!init.ok) {
            return {false, init.diag_id, init.span, {}};
          }
          out.binding.init = init.value;
          const auto ty = ResolveTypeOpt(ctx, node.binding.type_opt);
          if (!ty.ok) {
            return {false, ty.diag_id, ty.span, {}};
          }
          out.binding.type_opt = ty.value;
          if (node.binding.pat) {
            const auto pat = ResolvePattern(ctx, node.binding.pat);
            if (!pat.ok) {
              return {false, pat.diag_id, pat.span, {}};
            }
            out.binding.pat = pat.value;
            const auto bind = BindPattern(ctx, pat.value);
            if (!bind.ok) {
              return {false, bind.diag_id, bind.span, {}};
            }
          }
          SPEC_RULE("ResolveStmt-Var");
          return {true, std::nullopt, std::nullopt, out};
        } else if constexpr (std::is_same_v<T, syntax::ShadowLetStmt>) {
          auto out = node;
          const auto init = ResolveExpr(ctx, node.init);
          if (!init.ok) {
            return {false, init.diag_id, init.span, {}};
          }
          out.init = init.value;
          const auto ty = ResolveTypeOpt(ctx, node.type_opt);
          if (!ty.ok) {
            return {false, ty.diag_id, ty.span, {}};
          }
          out.type_opt = ty.value;
          const auto res = ShadowIntro(*ctx.ctx, node.name,
                                       Entity{EntityKind::Value, std::nullopt,
                                              std::nullopt,
                                              EntitySource::Decl});
          if (!res.ok) {
            return {false, res.diag_id, node.span, {}};
          }
          SPEC_RULE("ResolveStmt-ShadowLet");
          return {true, std::nullopt, std::nullopt, out};
        } else if constexpr (std::is_same_v<T, syntax::ShadowVarStmt>) {
          auto out = node;
          const auto init = ResolveExpr(ctx, node.init);
          if (!init.ok) {
            return {false, init.diag_id, init.span, {}};
          }
          out.init = init.value;
          const auto ty = ResolveTypeOpt(ctx, node.type_opt);
          if (!ty.ok) {
            return {false, ty.diag_id, ty.span, {}};
          }
          out.type_opt = ty.value;
          const auto res = ShadowIntro(*ctx.ctx, node.name,
                                       Entity{EntityKind::Value, std::nullopt,
                                              std::nullopt,
                                              EntitySource::Decl});
          if (!res.ok) {
            return {false, res.diag_id, node.span, {}};
          }
          SPEC_RULE("ResolveStmt-ShadowVar");
          return {true, std::nullopt, std::nullopt, out};
        } else if constexpr (std::is_same_v<T, syntax::DeferStmt>) {
          auto out = node;
          if (node.body) {
            const auto resolved = ResolveBlock(ctx, *node.body);
            if (!resolved.ok) {
              return {false, resolved.diag_id, resolved.span, {}};
            }
            out.body = std::make_shared<syntax::Block>(resolved.block);
          }
          SPEC_RULE("ResolveStmt-Defer");
          return {true, std::nullopt, std::nullopt, out};
        } else if constexpr (std::is_same_v<T, syntax::FrameStmt>) {
          auto out = node;
          if (node.target_opt) {
            const auto ent = ResolveValueName(*ctx.ctx, *node.target_opt);
            if (!ent.has_value()) {
              return {false, "ResolveExpr-Ident-Err", node.span, {}};
            }
            SPEC_RULE("ResolveStmt-Frame-Explicit");
          } else {
            SPEC_RULE("ResolveStmt-Frame-Implicit");
          }
          if (node.body) {
            const auto resolved = ResolveBlock(ctx, *node.body);
            if (!resolved.ok) {
              return {false, resolved.diag_id, resolved.span, {}};
            }
            out.body = std::make_shared<syntax::Block>(resolved.block);
          }
          return {true, std::nullopt, std::nullopt, out};
        } else if constexpr (std::is_same_v<T, syntax::RegionStmt>) {
          auto out = node;
          const auto opts = ResolveExprOpt(ctx, node.opts_opt);
          if (!opts.ok) {
            return {false, opts.diag_id, opts.span, {}};
          }
          out.opts_opt = opts.value;
          std::optional<Scope> saved_scope;
          if (node.alias_opt) {
            if (!ctx.ctx || ctx.ctx->scopes.empty()) {
              return {false, "ResolveExpr-Ident-Err", node.span, {}};
            }
            saved_scope = ctx.ctx->scopes.front();
            const auto res = Intro(*ctx.ctx, *node.alias_opt,
                                   Entity{EntityKind::Value, std::nullopt,
                                          std::nullopt,
                                          EntitySource::RegionAlias});
            if (!res.ok) {
              return {false, res.diag_id, node.span, {}};
            }
          }
          if (node.body) {
            const auto resolved = ResolveBlock(ctx, *node.body);
            if (!resolved.ok) {
              return {false, resolved.diag_id, resolved.span, {}};
            }
            out.body = std::make_shared<syntax::Block>(resolved.block);
          }
          if (saved_scope.has_value()) {
            ctx.ctx->scopes.front() = *saved_scope;
          }
          SPEC_RULE("ResolveStmt-Region");
          return {true, std::nullopt, std::nullopt, out};
        } else if constexpr (std::is_same_v<T, syntax::AssignStmt>) {
          auto out = node;
          const auto place = ResolveExpr(ctx, node.place);
          if (!place.ok) {
            return {false, place.diag_id, place.span, {}};
          }
          const auto value = ResolveExpr(ctx, node.value);
          if (!value.ok) {
            return {false, value.diag_id, value.span, {}};
          }
          out.place = place.value;
          out.value = value.value;
          return {true, std::nullopt, std::nullopt, out};
        } else if constexpr (std::is_same_v<T, syntax::CompoundAssignStmt>) {
          auto out = node;
          const auto place = ResolveExpr(ctx, node.place);
          if (!place.ok) {
            return {false, place.diag_id, place.span, {}};
          }
          const auto value = ResolveExpr(ctx, node.value);
          if (!value.ok) {
            return {false, value.diag_id, value.span, {}};
          }
          out.place = place.value;
          out.value = value.value;
          return {true, std::nullopt, std::nullopt, out};
        } else if constexpr (std::is_same_v<T, syntax::ExprStmt>) {
          auto out = node;
          const auto value = ResolveExpr(ctx, node.value);
          if (!value.ok) {
            return {false, value.diag_id, value.span, {}};
          }
          out.value = value.value;
          return {true, std::nullopt, std::nullopt, out};
        } else if constexpr (std::is_same_v<T, syntax::ReturnStmt>) {
          auto out = node;
          const auto value = ResolveExprOpt(ctx, node.value_opt);
          if (!value.ok) {
            return {false, value.diag_id, value.span, {}};
          }
          out.value_opt = value.value;
          return {true, std::nullopt, std::nullopt, out};
        } else if constexpr (std::is_same_v<T, syntax::ResultStmt>) {
          auto out = node;
          const auto value = ResolveExpr(ctx, node.value);
          if (!value.ok) {
            return {false, value.diag_id, value.span, {}};
          }
          out.value = value.value;
          return {true, std::nullopt, std::nullopt, out};
        } else if constexpr (std::is_same_v<T, syntax::BreakStmt>) {
          auto out = node;
          const auto value = ResolveExprOpt(ctx, node.value_opt);
          if (!value.ok) {
            return {false, value.diag_id, value.span, {}};
          }
          out.value_opt = value.value;
          return {true, std::nullopt, std::nullopt, out};
        } else if constexpr (std::is_same_v<T, syntax::ContinueStmt>) {
          return {true, std::nullopt, std::nullopt, node};
        } else if constexpr (std::is_same_v<T, syntax::UnsafeBlockStmt>) {
          auto out = node;
          if (node.body) {
            const auto resolved = ResolveBlock(ctx, *node.body);
            if (!resolved.ok) {
              return {false, resolved.diag_id, resolved.span, {}};
            }
            out.body = std::make_shared<syntax::Block>(resolved.block);
          }
          return {true, std::nullopt, std::nullopt, out};
        } else {
          return {true, std::nullopt, std::nullopt, node};
        }
      },
      stmt);
}

ResolveStmtSeqResult ResolveStmtSeq(ResolveContext& ctx,
                                    const std::vector<syntax::Stmt>& stmts) {
  SpecDefsResolverExpr();
  ResolveStmtSeqResult result;
  result.ok = true;
  if (stmts.empty()) {
    SPEC_RULE("ResolveStmtSeq-Empty");
    return result;
  }
  result.stmts.reserve(stmts.size());
  for (const auto& stmt : stmts) {
    const auto resolved = ResolveStmt(ctx, stmt);
    if (!resolved.ok) {
      return {false, resolved.diag_id, resolved.span, {}};
    }
    result.stmts.push_back(resolved.stmt);
    SPEC_RULE("ResolveStmtSeq-Cons");
  }
  return result;
}

ResolveBlockResult ResolveBlock(ResolveContext& ctx,
                                const syntax::Block& block) {
  SpecDefsResolverExpr();
  ResolveBlockResult result;
  if (!ctx.ctx) {
    return result;
  }
  ScopeGuard guard(*ctx.ctx);
  const auto stmts = ResolveStmtSeq(ctx, block.stmts);
  if (!stmts.ok) {
    return {false, stmts.diag_id, stmts.span, {}};
  }
  const auto tail = ResolveExprOpt(ctx, block.tail_opt);
  if (!tail.ok) {
    return {false, tail.diag_id, tail.span, {}};
  }
  result.ok = true;
  result.block = block;
  result.block.stmts = stmts.stmts;
  result.block.tail_opt = tail.value;
  return result;
}

}  // namespace cursive0::analysis
