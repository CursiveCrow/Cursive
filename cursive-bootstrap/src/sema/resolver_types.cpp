#include "cursive0/sema/resolver.h"

#include <utility>
#include <type_traits>

#include "cursive0/core/assert_spec.h"
#include "cursive0/sema/scopes.h"
#include "cursive0/sema/cap_filesystem.h"
#include "cursive0/sema/cap_heap.h"

namespace cursive0::sema {

namespace {

static inline void SpecDefsResolverTypes() {
  SPEC_DEF("ResolveTypePath", "5.1.7");
  SPEC_DEF("ResolveClassPath", "5.1.7");
  SPEC_DEF("ResolveType", "5.1.7");
  SPEC_DEF("ResolveTypeRef", "5.1.7");
  SPEC_DEF("ResolveParams", "5.1.7");
  SPEC_DEF("ResolveParam", "5.1.7");
  SPEC_DEF("FullPath", "5.12");
}


static bool IsFoundationalClassPath(const syntax::ClassPath& path) {
  if (path.size() != 1) {
    return false;
  }
  return IdEq(path[0], "Drop") || IdEq(path[0], "Bitcopy") ||
         IdEq(path[0], "Clone");
}
syntax::Path FullPath(const syntax::Path& path, std::string_view name) {
  syntax::Path out = path;
  out.emplace_back(name);
  return out;
}

}  // namespace

ResTypePathResult ResolveTypePath(ResolveContext& ctx,
                                 const syntax::TypePath& path) {
  SpecDefsResolverTypes();
  ResTypePathResult result;
  if (!ctx.ctx || !ctx.name_maps || !ctx.module_names) {
    return result;
  }
  if (path.empty()) {
    return result;
  }
  if (path.size() == 1) {
    if (IsFileSystemBuiltinTypePath(path) ||
        IsHeapAllocatorBuiltinTypePath(path)) {
      SPEC_RULE("ResolveTypePath-Ident-Local");
      return {true, std::nullopt, std::nullopt, syntax::TypePath{path[0]}};
    }
    const auto ent = ResolveTypeName(*ctx.ctx, path[0]);
    if (!ent.has_value()) {
      return {false, "ResolveExpr-Ident-Err", std::nullopt, {}};
    }
    const auto name = ent->target_opt.value_or(path[0]);
    if (ent->origin_opt.has_value()) {
      SPEC_RULE("ResolveTypePath-Ident");
      return {true, std::nullopt, std::nullopt,
              FullPath(*ent->origin_opt, name)};
    }
    SPEC_RULE("ResolveTypePath-Ident-Local");
    return {true, std::nullopt, std::nullopt, syntax::TypePath{name}};
  }

  const auto prefix = syntax::ModulePath(path.begin(), path.end() - 1);
  const auto name = path.back();
  const auto qualified =
      ResolveQualified(*ctx.ctx, *ctx.name_maps, *ctx.module_names, prefix,
                       name, EntityKind::Type, ctx.can_access);
  if (!qualified.ok || !qualified.entity.has_value()) {
    return {false, qualified.diag_id, std::nullopt, {}};
  }
  const auto ent = *qualified.entity;
  if (!ent.origin_opt.has_value()) {
    return {false, "ResolveExpr-Ident-Err", std::nullopt, {}};
  }
  const auto resolved_name = ent.target_opt.value_or(name);
  SPEC_RULE("ResolveTypePath-Qual");
  return {true, std::nullopt, std::nullopt,
          FullPath(*ent.origin_opt, resolved_name)};
}

ResClassPathResult ResolveClassPath(ResolveContext& ctx,
                                   const syntax::ClassPath& path) {
  SpecDefsResolverTypes();
  ResClassPathResult result;
  if (!ctx.ctx || !ctx.name_maps || !ctx.module_names) {
    return result;
  }
  if (path.empty()) {
    return result;
  }
  if (path.size() == 1) {
    if (IsFileSystemClassPath(path) || IsHeapAllocatorClassPath(path) ||
        IsFoundationalClassPath(path)) {
      SPEC_RULE("ResolveClassPath-Ident");
      return {true, std::nullopt, std::nullopt, syntax::ClassPath{path[0]}};
    }
    const auto ent = ResolveClassName(*ctx.ctx, path[0]);
    if (!ent.has_value() || !ent->origin_opt.has_value()) {
      return {false, "ResolveExpr-Ident-Err", std::nullopt, {}};
    }
    const auto name = ent->target_opt.value_or(path[0]);
    SPEC_RULE("ResolveClassPath-Ident");
    return {true, std::nullopt, std::nullopt,
            FullPath(*ent->origin_opt, name)};
  }

  const auto prefix = syntax::ModulePath(path.begin(), path.end() - 1);
  const auto name = path.back();
  const auto qualified =
      ResolveQualified(*ctx.ctx, *ctx.name_maps, *ctx.module_names, prefix,
                       name, EntityKind::Class, ctx.can_access);
  if (!qualified.ok || !qualified.entity.has_value()) {
    return {false, qualified.diag_id, std::nullopt, {}};
  }
  const auto ent = *qualified.entity;
  if (!ent.origin_opt.has_value()) {
    return {false, "ResolveExpr-Ident-Err", std::nullopt, {}};
  }
  const auto resolved_name = ent.target_opt.value_or(name);
  SPEC_RULE("ResolveClassPath-Qual");
  return {true, std::nullopt, std::nullopt,
          FullPath(*ent.origin_opt, resolved_name)};
}

ResTypeResult ResolveType(ResolveContext& ctx,
                          const std::shared_ptr<syntax::Type>& type) {
  SpecDefsResolverTypes();
  ResTypeResult result;
  if (!type) {
    return result;
  }
  result.value = type;
  return std::visit(
      [&](const auto& node) -> ResTypeResult {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::TypePathType>) {
          const auto resolved = ResolveTypePath(ctx, node.path);
          if (!resolved.ok) {
            return {false, resolved.diag_id, std::nullopt, {}};
          }
          auto out = *type;
          auto& out_node = std::get<syntax::TypePathType>(out.node);
          out_node.path = resolved.value;
          SPEC_RULE("ResolveType-Path");
          return {true, std::nullopt, std::nullopt,
                  std::make_shared<syntax::Type>(std::move(out))};
        } else if constexpr (std::is_same_v<T, syntax::TypeDynamic>) {
          const auto resolved = ResolveClassPath(ctx, node.path);
          if (!resolved.ok) {
            return {false, resolved.diag_id, std::nullopt, {}};
          }
          auto out = *type;
          auto& out_node = std::get<syntax::TypeDynamic>(out.node);
          out_node.path = resolved.value;
          SPEC_RULE("ResolveType-Dynamic");
          return {true, std::nullopt, std::nullopt,
                  std::make_shared<syntax::Type>(std::move(out))};
        } else if constexpr (std::is_same_v<T, syntax::TypeModalState>) {
          const auto resolved = ResolveTypePath(ctx, node.path);
          if (!resolved.ok) {
            return {false, resolved.diag_id, std::nullopt, {}};
          }
          auto out = *type;
          auto& out_node = std::get<syntax::TypeModalState>(out.node);
          out_node.path = resolved.value;
          SPEC_RULE("ResolveType-ModalState");
          return {true, std::nullopt, std::nullopt,
                  std::make_shared<syntax::Type>(std::move(out))};
        } else if constexpr (std::is_same_v<T, syntax::TypePermType>) {
          const auto resolved = ResolveType(ctx, node.base);
          if (!resolved.ok) {
            return {false, resolved.diag_id, std::nullopt, {}};
          }
          auto out = *type;
          auto& out_node = std::get<syntax::TypePermType>(out.node);
          out_node.base = resolved.value;
          SPEC_RULE("ResolveType-Hom");
          return {true, std::nullopt, std::nullopt,
                  std::make_shared<syntax::Type>(std::move(out))};
        } else if constexpr (std::is_same_v<T, syntax::TypeUnion>) {
          auto out = *type;
          auto& out_node = std::get<syntax::TypeUnion>(out.node);
          for (auto& elem : out_node.types) {
            const auto resolved = ResolveType(ctx, elem);
            if (!resolved.ok) {
              return {false, resolved.diag_id, std::nullopt, {}};
            }
            elem = resolved.value;
          }
          SPEC_RULE("ResolveType-Hom");
          return {true, std::nullopt, std::nullopt,
                  std::make_shared<syntax::Type>(std::move(out))};
        } else if constexpr (std::is_same_v<T, syntax::TypeFunc>) {
          auto out = *type;
          auto& out_node = std::get<syntax::TypeFunc>(out.node);
          for (auto& param : out_node.params) {
            const auto resolved = ResolveType(ctx, param.type);
            if (!resolved.ok) {
              return {false, resolved.diag_id, std::nullopt, {}};
            }
            param.type = resolved.value;
          }
          if (out_node.ret) {
            const auto resolved = ResolveType(ctx, out_node.ret);
            if (!resolved.ok) {
              return {false, resolved.diag_id, std::nullopt, {}};
            }
            out_node.ret = resolved.value;
          }
          SPEC_RULE("ResolveType-Hom");
          return {true, std::nullopt, std::nullopt,
                  std::make_shared<syntax::Type>(std::move(out))};
        } else if constexpr (std::is_same_v<T, syntax::TypeTuple>) {
          auto out = *type;
          auto& out_node = std::get<syntax::TypeTuple>(out.node);
          for (auto& elem : out_node.elements) {
            const auto resolved = ResolveType(ctx, elem);
            if (!resolved.ok) {
              return {false, resolved.diag_id, std::nullopt, {}};
            }
            elem = resolved.value;
          }
          SPEC_RULE("ResolveType-Hom");
          return {true, std::nullopt, std::nullopt,
                  std::make_shared<syntax::Type>(std::move(out))};
        } else if constexpr (std::is_same_v<T, syntax::TypeArray>) {
          auto out = *type;
          auto& out_node = std::get<syntax::TypeArray>(out.node);
          const auto resolved_elem = ResolveType(ctx, out_node.element);
          if (!resolved_elem.ok) {
            return {false, resolved_elem.diag_id, std::nullopt, {}};
          }
          out_node.element = resolved_elem.value;
          if (out_node.length) {
            const auto resolved_len = ResolveExpr(ctx, out_node.length);
            if (!resolved_len.ok) {
              return {false, resolved_len.diag_id, std::nullopt, {}};
            }
            out_node.length = resolved_len.value;
          }
          SPEC_RULE("ResolveType-Hom");
          return {true, std::nullopt, std::nullopt,
                  std::make_shared<syntax::Type>(std::move(out))};
        } else if constexpr (std::is_same_v<T, syntax::TypeSlice>) {
          auto out = *type;
          auto& out_node = std::get<syntax::TypeSlice>(out.node);
          const auto resolved = ResolveType(ctx, out_node.element);
          if (!resolved.ok) {
            return {false, resolved.diag_id, std::nullopt, {}};
          }
          out_node.element = resolved.value;
          SPEC_RULE("ResolveType-Hom");
          return {true, std::nullopt, std::nullopt,
                  std::make_shared<syntax::Type>(std::move(out))};
        } else if constexpr (std::is_same_v<T, syntax::TypePtr>) {
          auto out = *type;
          auto& out_node = std::get<syntax::TypePtr>(out.node);
          const auto resolved = ResolveType(ctx, out_node.element);
          if (!resolved.ok) {
            return {false, resolved.diag_id, std::nullopt, {}};
          }
          out_node.element = resolved.value;
          SPEC_RULE("ResolveType-Hom");
          return {true, std::nullopt, std::nullopt,
                  std::make_shared<syntax::Type>(std::move(out))};
        } else if constexpr (std::is_same_v<T, syntax::TypeRawPtr>) {
          auto out = *type;
          auto& out_node = std::get<syntax::TypeRawPtr>(out.node);
          const auto resolved = ResolveType(ctx, out_node.element);
          if (!resolved.ok) {
            return {false, resolved.diag_id, std::nullopt, {}};
          }
          out_node.element = resolved.value;
          SPEC_RULE("ResolveType-Hom");
          return {true, std::nullopt, std::nullopt,
                  std::make_shared<syntax::Type>(std::move(out))};
        } else {
          SPEC_RULE("ResolveType-Hom");
          return {true, std::nullopt, std::nullopt, type};
        }
      },
      type->node);
}

}  // namespace cursive0::sema
