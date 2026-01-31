#include "cursive0/03_analysis/resolve/resolve_qual.h"

#include <utility>

#include "cursive0/00_core/assert_spec.h"
#include "cursive0/03_analysis/resolve/scopes.h"
#include "cursive0/03_analysis/memory/string_bytes.h"

namespace cursive0::analysis {

namespace {

static inline void SpecDefsResolveQual() {
  SPEC_DEF("ResolveQualifiedForm", "5.1.6");
  SPEC_DEF("ResolveArgs", "5.1.6");
  SPEC_DEF("ResolveFieldInits", "5.1.6");
  SPEC_DEF("ResolveRecordPath", "5.1.6");
  SPEC_DEF("ResolveEnumUnit", "5.1.6");
  SPEC_DEF("ResolveEnumTuple", "5.1.6");
  SPEC_DEF("ResolveEnumRecord", "5.1.6");
  SPEC_DEF("ResolvePathJudg", "5.1.6");
  SPEC_DEF("PathOfModule", "3.4.1");
  SPEC_DEF("SplitLast", "5.1.5");
  SPEC_DEF("FullPath", "5.12");
  SPEC_DEF("ArgsExprs", "9.3");
}

bool IsRuntimePanicPath(const syntax::Path& path, std::string_view name) {
  return path.size() == 2 && IdEq(path[0], "cursive") &&
         IdEq(path[1], "runtime") && name == "panic";
}

syntax::ExprPtr MakeExpr(const core::Span& span, syntax::ExprNode node) {
  auto expr = std::make_shared<syntax::Expr>();
  expr->span = span;
  expr->node = std::move(node);
  return expr;
}

std::optional<std::pair<syntax::ModulePath, syntax::Identifier>> SplitLast(
    const syntax::Path& path) {
  SpecDefsResolveQual();
  if (path.size() < 2) {
    return std::nullopt;
  }
  syntax::ModulePath prefix(path.begin(), path.end() - 1);
  return std::make_pair(prefix, path.back());
}

syntax::Path FullPath(const syntax::Path& path, std::string_view name) {
  SpecDefsResolveQual();
  syntax::Path out = path;
  out.emplace_back(name);
  return out;
}

std::vector<syntax::ExprPtr> ArgsExprs(const std::vector<syntax::Arg>& args) {
  SpecDefsResolveQual();
  std::vector<syntax::ExprPtr> out;
  out.reserve(args.size());
  for (const auto& arg : args) {
    out.push_back(arg.value);
  }
  return out;
}

const syntax::RecordDecl* FindRecordDecl(const ScopeContext& ctx,
                                         const syntax::TypePath& path) {
  const auto it = ctx.sigma.types.find(PathKeyOf(path));
  if (it == ctx.sigma.types.end()) {
    return nullptr;
  }
  return std::get_if<syntax::RecordDecl>(&it->second);
}

const syntax::EnumDecl* FindEnumDecl(const ScopeContext& ctx,
                                     const syntax::TypePath& path) {
  const auto it = ctx.sigma.types.find(PathKeyOf(path));
  if (it == ctx.sigma.types.end()) {
    return nullptr;
  }
  return std::get_if<syntax::EnumDecl>(&it->second);
}

const syntax::VariantDecl* FindVariant(const syntax::EnumDecl& decl,
                                       std::string_view name) {
  for (const auto& variant : decl.variants) {
    if (IdEq(variant.name, name)) {
      return &variant;
    }
  }
  return nullptr;
}

enum class VariantKind {
  Unit,
  Tuple,
  Record,
};

std::optional<VariantKind> VariantPayloadKind(
    const syntax::VariantDecl& variant) {
  if (!variant.payload_opt) {
    return VariantKind::Unit;
  }
  if (std::holds_alternative<syntax::VariantPayloadTuple>(
          *variant.payload_opt)) {
    return VariantKind::Tuple;
  }
  if (std::holds_alternative<syntax::VariantPayloadRecord>(
          *variant.payload_opt)) {
    return VariantKind::Record;
  }
  return std::nullopt;
}

}  // namespace

ResolveArgsResult ResolveArgs(const ResolveQualContext& ctx,
                              const std::vector<syntax::Arg>& args) {
  SpecDefsResolveQual();
  if (args.empty()) {
    SPEC_RULE("ResolveArgs-Empty");
    return {true, std::nullopt, {}};
  }
  if (!ctx.ctx || !ctx.name_maps || !ctx.module_names || !ctx.resolve_expr) {
    return {false, std::nullopt, {}};
  }
  std::vector<syntax::Arg> out;
  out.reserve(args.size());
  for (const auto& arg : args) {
    const auto resolved =
        ctx.resolve_expr(*ctx.ctx, *ctx.name_maps, *ctx.module_names,
                         arg.value);
    if (!resolved.ok) {
      return {false, resolved.diag_id, {}};
    }
    syntax::Arg next = arg;
    next.value = resolved.expr;
    out.push_back(std::move(next));
    SPEC_RULE("ResolveArgs-Cons");
  }
  return {true, std::nullopt, std::move(out)};
}

ResolveFieldInitsResult ResolveFieldInits(
    const ResolveQualContext& ctx,
    const std::vector<syntax::FieldInit>& fields) {
  SpecDefsResolveQual();
  if (fields.empty()) {
    SPEC_RULE("ResolveFieldInits-Empty");
    return {true, std::nullopt, {}};
  }
  if (!ctx.ctx || !ctx.name_maps || !ctx.module_names || !ctx.resolve_expr) {
    return {false, std::nullopt, {}};
  }
  std::vector<syntax::FieldInit> out;
  out.reserve(fields.size());
  for (const auto& field : fields) {
    const auto resolved =
        ctx.resolve_expr(*ctx.ctx, *ctx.name_maps, *ctx.module_names,
                         field.value);
    if (!resolved.ok) {
      return {false, resolved.diag_id, {}};
    }
    syntax::FieldInit next = field;
    next.value = resolved.expr;
    out.push_back(std::move(next));
    SPEC_RULE("ResolveFieldInits-Cons");
  }
  return {true, std::nullopt, std::move(out)};
}

ResolveRecordPathResult ResolveRecordPath(const ResolveQualContext& ctx,
                                          const syntax::ModulePath& path,
                                          std::string_view name) {
  SpecDefsResolveQual();
  if (!ctx.ctx || !ctx.name_maps || !ctx.module_names || !ctx.resolve_type_path) {
    return {false, std::nullopt, {}};
  }
  const auto full = FullPath(path, name);
  const auto resolved =
      ctx.resolve_type_path(*ctx.ctx, *ctx.name_maps, *ctx.module_names, full);
  if (!resolved.ok) {
    return {false, std::nullopt, {}};
  }
  if (!FindRecordDecl(*ctx.ctx, resolved.path)) {
    return {false, std::nullopt, {}};
  }
  SPEC_RULE("Resolve-RecordPath");
  return {true, std::nullopt, resolved.path};
}

ResolveEnumPathResult ResolveEnumUnit(const ResolveQualContext& ctx,
                                      const syntax::ModulePath& path,
                                      std::string_view name) {
  SpecDefsResolveQual();
  if (!ctx.ctx || !ctx.name_maps || !ctx.module_names || !ctx.resolve_type_path) {
    return {false, std::nullopt, {}};
  }
  const auto resolved =
      ctx.resolve_type_path(*ctx.ctx, *ctx.name_maps, *ctx.module_names, path);
  if (!resolved.ok) {
    return {false, std::nullopt, {}};
  }
  const auto* decl = FindEnumDecl(*ctx.ctx, resolved.path);
  if (!decl) {
    return {false, std::nullopt, {}};
  }
  const auto* variant = FindVariant(*decl, name);
  if (!variant) {
    return {false, std::nullopt, {}};
  }
  const auto kind = VariantPayloadKind(*variant);
  if (!kind || *kind != VariantKind::Unit) {
    return {false, std::nullopt, {}};
  }
  SPEC_RULE("Resolve-EnumUnit");
  return {true, std::nullopt, resolved.path};
}

ResolveEnumPathResult ResolveEnumTuple(const ResolveQualContext& ctx,
                                       const syntax::ModulePath& path,
                                       std::string_view name) {
  SpecDefsResolveQual();
  if (!ctx.ctx || !ctx.name_maps || !ctx.module_names || !ctx.resolve_type_path) {
    return {false, std::nullopt, {}};
  }
  const auto resolved =
      ctx.resolve_type_path(*ctx.ctx, *ctx.name_maps, *ctx.module_names, path);
  if (!resolved.ok) {
    return {false, std::nullopt, {}};
  }
  const auto* decl = FindEnumDecl(*ctx.ctx, resolved.path);
  if (!decl) {
    return {false, std::nullopt, {}};
  }
  const auto* variant = FindVariant(*decl, name);
  if (!variant) {
    return {false, std::nullopt, {}};
  }
  const auto kind = VariantPayloadKind(*variant);
  if (!kind || *kind != VariantKind::Tuple) {
    return {false, std::nullopt, {}};
  }
  SPEC_RULE("Resolve-EnumTuple");
  return {true, std::nullopt, resolved.path};
}

ResolveEnumPathResult ResolveEnumRecord(const ResolveQualContext& ctx,
                                        const syntax::ModulePath& path,
                                        std::string_view name) {
  SpecDefsResolveQual();
  if (!ctx.ctx || !ctx.name_maps || !ctx.module_names || !ctx.resolve_type_path) {
    return {false, std::nullopt, {}};
  }
  const auto resolved =
      ctx.resolve_type_path(*ctx.ctx, *ctx.name_maps, *ctx.module_names, path);
  if (!resolved.ok) {
    return {false, std::nullopt, {}};
  }
  const auto* decl = FindEnumDecl(*ctx.ctx, resolved.path);
  if (!decl) {
    return {false, std::nullopt, {}};
  }
  const auto* variant = FindVariant(*decl, name);
  if (!variant) {
    return {false, std::nullopt, {}};
  }
  const auto kind = VariantPayloadKind(*variant);
  if (!kind || *kind != VariantKind::Record) {
    return {false, std::nullopt, {}};
  }
  SPEC_RULE("Resolve-EnumRecord");
  return {true, std::nullopt, resolved.path};
}

ResolveQualifiedFormResult ResolveQualifiedForm(const ResolveQualContext& ctx,
                                                const syntax::Expr& expr) {
  SpecDefsResolveQual();
  if (!ctx.ctx || !ctx.name_maps || !ctx.module_names) {
    return {false, std::nullopt, {}};
  }
  return std::visit(
      [&](const auto& node) -> ResolveQualifiedFormResult {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::QualifiedNameExpr>) {
          if (IsStringBytesBuiltinPath(node.path)) {
            const bool is_string = node.path.size() == 1 &&
                                   IdEq(node.path[0], "string");
            const bool is_bytes = node.path.size() == 1 &&
                                  IdEq(node.path[0], "bytes");
            if ((IsStringBuiltinName(node.name) && is_string) ||
                (IsBytesBuiltinName(node.name) && is_bytes)) {
              syntax::PathExpr path;
              path.path = node.path;
              path.name = node.name;
              SPEC_RULE("ResolveQual-Name-Value");
              return {true, std::nullopt, MakeExpr(expr.span, path)};
            }
            SPEC_RULE("ResolveQual-Name-Err");
            return {false, "ResolveExpr-Ident-Err", {}};
          }
          if (IsRuntimePanicPath(node.path, node.name)) {
            syntax::PathExpr path;
            path.path = node.path;
            path.name = node.name;
            SPEC_RULE("ResolveQual-Name-Value");
            return {true, std::nullopt, MakeExpr(expr.span, path)};
          }
          const auto value = ResolveQualified(
              *ctx.ctx, *ctx.name_maps, *ctx.module_names, node.path, node.name,
              EntityKind::Value, ctx.can_access);
          if (value.ok && value.entity && value.entity->origin_opt) {
            const auto& ent = *value.entity;
            const syntax::Identifier name =
                ent.target_opt ? *ent.target_opt
                               : syntax::Identifier(node.name);
            syntax::PathExpr path;
            path.path = *ent.origin_opt;
            path.name = name;
            SPEC_RULE("ResolveQual-Name-Value");
            return {true, std::nullopt, MakeExpr(expr.span, path)};
          }
          const auto record = ResolveRecordPath(ctx, node.path, node.name);
          if (record.ok) {
            const auto split = SplitLast(record.path);
            if (!split) {
              return {false, std::nullopt, {}};
            }
            syntax::PathExpr path;
            path.path = split->first;
            path.name = split->second;
            SPEC_RULE("ResolveQual-Name-Record");
            return {true, std::nullopt, MakeExpr(expr.span, path)};
          }
          const auto unit = ResolveEnumUnit(ctx, node.path, node.name);
          if (unit.ok) {
            syntax::EnumLiteralExpr literal;
            literal.path = FullPath(unit.path, node.name);
            literal.payload_opt = std::nullopt;
            SPEC_RULE("ResolveQual-Name-Enum");
            return {true, std::nullopt, MakeExpr(expr.span, literal)};
          }
          SPEC_RULE("ResolveQual-Name-Err");
          return {false, "ResolveExpr-Ident-Err", {}};
        } else if constexpr (std::is_same_v<T, syntax::QualifiedApplyExpr>) {
          if (std::holds_alternative<syntax::ParenArgs>(node.args)) {
            const auto resolved_args =
                ResolveArgs(ctx, std::get<syntax::ParenArgs>(node.args).args);
            if (!resolved_args.ok) {
              return {false, resolved_args.diag_id, {}};
            }
            if (IsStringBytesBuiltinPath(node.path)) {
              const bool is_string = node.path.size() == 1 &&
                                     IdEq(node.path[0], "string");
              const bool is_bytes = node.path.size() == 1 &&
                                    IdEq(node.path[0], "bytes");
              if ((IsStringBuiltinName(node.name) && is_string) ||
                  (IsBytesBuiltinName(node.name) && is_bytes)) {
                syntax::PathExpr path;
                path.path = node.path;
                path.name = node.name;
                syntax::CallExpr call;
                call.callee = MakeExpr(expr.span, path);
                call.args = resolved_args.args;
                SPEC_RULE("ResolveQual-Apply-Value");
                return {true, std::nullopt, MakeExpr(expr.span, call)};
              }
              SPEC_RULE("ResolveQual-Apply-Err");
              return {false, "ResolveExpr-Ident-Err", {}};
            }
            if (IsRuntimePanicPath(node.path, node.name)) {
              syntax::PathExpr path;
              path.path = node.path;
              path.name = node.name;
              syntax::CallExpr call;
              call.callee = MakeExpr(expr.span, path);
              call.args = resolved_args.args;
              SPEC_RULE("ResolveQual-Apply-Value");
              return {true, std::nullopt, MakeExpr(expr.span, call)};
            }
            if (node.path.size() == 1 && IdEq(node.path[0], "CancelToken") &&
                IdEq(node.name, "new")) {
              syntax::PathExpr path;
              path.path = node.path;
              path.name = node.name;
              syntax::CallExpr call;
              call.callee = MakeExpr(expr.span, path);
              call.args = resolved_args.args;
              SPEC_RULE("ResolveQual-Apply-Value");
              return {true, std::nullopt, MakeExpr(expr.span, call)};
            }
            const auto value = ResolveQualified(
                *ctx.ctx, *ctx.name_maps, *ctx.module_names, node.path,
                node.name, EntityKind::Value, ctx.can_access);
            if (value.ok && value.entity && value.entity->origin_opt) {
              const auto& ent = *value.entity;
              const syntax::Identifier name =
                  ent.target_opt ? *ent.target_opt
                                 : syntax::Identifier(node.name);
              syntax::PathExpr path;
              path.path = *ent.origin_opt;
              path.name = name;
              syntax::CallExpr call;
              call.callee = MakeExpr(expr.span, path);
              call.args = resolved_args.args;
              SPEC_RULE("ResolveQual-Apply-Value");
              return {true, std::nullopt, MakeExpr(expr.span, call)};
            }
            const auto record = ResolveRecordPath(ctx, node.path, node.name);
            if (record.ok) {
              const auto split = SplitLast(record.path);
              if (!split) {
                return {false, std::nullopt, {}};
              }
              syntax::PathExpr path;
              path.path = split->first;
              path.name = split->second;
              syntax::CallExpr call;
              call.callee = MakeExpr(expr.span, path);
              call.args = resolved_args.args;
              SPEC_RULE("ResolveQual-Apply-Record");
              return {true, std::nullopt, MakeExpr(expr.span, call)};
            }
            const auto tuple = ResolveEnumTuple(ctx, node.path, node.name);
            if (tuple.ok) {
              syntax::EnumLiteralExpr literal;
              literal.path = FullPath(tuple.path, node.name);
              syntax::EnumPayloadParen payload;
              payload.elements = ArgsExprs(resolved_args.args);
              literal.payload_opt = syntax::EnumPayload{payload};
              SPEC_RULE("ResolveQual-Apply-Enum-Tuple");
              return {true, std::nullopt, MakeExpr(expr.span, literal)};
            }
            SPEC_RULE("ResolveQual-Apply-Err");
            return {false, "ResolveExpr-Ident-Err", {}};
          }
          if (std::holds_alternative<syntax::BraceArgs>(node.args)) {
            const auto resolved_fields = ResolveFieldInits(
                ctx, std::get<syntax::BraceArgs>(node.args).fields);
            if (!resolved_fields.ok) {
              return {false, resolved_fields.diag_id, {}};
            }
            const auto record = ResolveRecordPath(ctx, node.path, node.name);
            if (record.ok) {
              syntax::RecordExpr rec;
              rec.target = record.path;
              rec.fields = resolved_fields.fields;
              SPEC_RULE("ResolveQual-Apply-RecordLit");
              return {true, std::nullopt, MakeExpr(expr.span, rec)};
            }
            const auto record_enum = ResolveEnumRecord(ctx, node.path, node.name);
            if (record_enum.ok) {
              syntax::EnumLiteralExpr literal;
              literal.path = FullPath(record_enum.path, node.name);
              syntax::EnumPayloadBrace payload;
              payload.fields = resolved_fields.fields;
              literal.payload_opt = syntax::EnumPayload{payload};
              SPEC_RULE("ResolveQual-Apply-Enum-Record");
              return {true, std::nullopt, MakeExpr(expr.span, literal)};
            }
            SPEC_RULE("ResolveQual-Apply-Brace-Err");
            return {false, "ResolveExpr-Ident-Err", {}};
          }
          return {false, std::nullopt, {}};
        } else {
          return {false, std::nullopt, {}};
        }
      },
      expr.node);
}

}  // namespace cursive0::analysis
