#include "04_analysis/composite/reflect_bridge.h"

#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "00_core/unicode.h"
#include "02_source/module_paths.h"
#include "04_analysis/composite/classes.h"
#include "04_analysis/resolve/scopes.h"
#include "04_analysis/typing/type_lower.h"

namespace cursive::analysis {
namespace {

bool IdEqLocal(std::string_view lhs, std::string_view rhs) {
  return core::NFC(lhs) == core::NFC(rhs);
}

bool PathEqLocal(const ast::ModulePath& lhs, const ast::ModulePath& rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }
  for (std::size_t i = 0; i < lhs.size(); ++i) {
    if (!IdEqLocal(lhs[i], rhs[i])) {
      return false;
    }
  }
  return true;
}

bool SameAssembly(const ast::ModulePath& lhs, const ast::ModulePath& rhs) {
  return !lhs.empty() && !rhs.empty() && IdEqLocal(lhs.front(), rhs.front());
}

bool CanAccessVis(const ast::ModulePath& accessor_module,
                  const ast::ModulePath& decl_module,
                  ast::Visibility vis) {
  switch (vis) {
    case ast::Visibility::Public:
      return true;
    case ast::Visibility::Internal:
      return SameAssembly(accessor_module, decl_module);
    case ast::Visibility::Private:
    case ast::Visibility::Protected:
      return PathEqLocal(accessor_module, decl_module);
  }
  return true;
}

const ast::ASTModule* FindModule(const std::vector<const ast::ASTModule*>& modules,
                                 const ast::ModulePath& path) {
  for (const ast::ASTModule* module : modules) {
    if (module != nullptr && PathEqLocal(module->path, path)) {
      return module;
    }
  }
  return nullptr;
}

std::vector<std::pair<ast::ModulePath, const ast::ClassDecl*>>
FindVisibleClassDeclsInModule(const std::vector<const ast::ASTModule*>& modules,
                              const ast::ModulePath& module_path,
                              const ast::ModulePath& accessor_module,
                              std::string_view name) {
  std::vector<std::pair<ast::ModulePath, const ast::ClassDecl*>> out;
  const ast::ASTModule* module = FindModule(modules, module_path);
  if (module == nullptr) {
    return out;
  }

  for (const auto& item : module->items) {
    const auto* cls = std::get_if<ast::ClassDecl>(&item);
    if (cls == nullptr || !IdEqLocal(cls->name, name) ||
        !CanAccessVis(accessor_module, module_path, cls->vis)) {
      continue;
    }
    out.emplace_back(module_path, cls);
  }
  return out;
}

std::optional<ast::ModulePath> ResolveVisibleModulePath(
    const std::vector<const ast::ASTModule*>& modules,
    const source::ModuleNames& available_module_names,
    const ast::ModulePath& accessor_module,
    const ast::ModulePath& path) {
  const auto resolved =
      source::ResolveImportModulePath(accessor_module, available_module_names,
                                      path);
  if (!resolved.has_value() || FindModule(modules, *resolved) == nullptr) {
    return std::nullopt;
  }
  return resolved;
}

void AppendUsingBoundClassDecls(
    std::vector<std::pair<ast::ModulePath, const ast::ClassDecl*>>& out,
    const std::vector<const ast::ASTModule*>& modules,
    const source::ModuleNames& available_module_names,
    const ast::ModulePath& accessor_module,
    const ast::UsingDecl& decl,
    std::string_view lookup_name) {
  std::visit(
      [&](const auto& clause) {
        using T = std::decay_t<decltype(clause)>;
        if constexpr (std::is_same_v<T, ast::UsingItem>) {
          const auto resolved_module = ResolveVisibleModulePath(
              modules, available_module_names, accessor_module,
              clause.module_path);
          if (!resolved_module.has_value()) {
            return;
          }
          const ast::Identifier bind_name = clause.alias_opt.value_or(clause.name);
          if (!IdEqLocal(bind_name, lookup_name)) {
            return;
          }
          auto matches = FindVisibleClassDeclsInModule(
              modules, *resolved_module, accessor_module, clause.name);
          out.insert(out.end(), matches.begin(), matches.end());
        } else if constexpr (std::is_same_v<T, ast::UsingList>) {
          const auto resolved_module = ResolveVisibleModulePath(
              modules, available_module_names, accessor_module,
              clause.module_path);
          if (!resolved_module.has_value()) {
            return;
          }
          for (const auto& spec : clause.specs) {
            const ast::Identifier bind_name = spec.alias_opt.value_or(spec.name);
            if (!IdEqLocal(bind_name, lookup_name)) {
              continue;
            }
            auto matches = FindVisibleClassDeclsInModule(
                modules, *resolved_module, accessor_module, spec.name);
            out.insert(out.end(), matches.begin(), matches.end());
          }
        } else if constexpr (std::is_same_v<T, ast::UsingWildcard>) {
          const auto resolved_module = ResolveVisibleModulePath(
              modules, available_module_names, accessor_module,
              clause.module_path);
          if (!resolved_module.has_value()) {
            return;
          }
          auto matches = FindVisibleClassDeclsInModule(
              modules, *resolved_module, accessor_module, lookup_name);
          out.insert(out.end(), matches.begin(), matches.end());
        }
      },
      decl.clause);
}

ast::ClassPath ResolveReflectClassPath(
    const std::vector<const ast::ASTModule*>& modules,
    const source::ModuleNames& available_module_names,
    const ast::ModulePath& accessor_module,
    const ast::ClassPath& path) {
  if (path.empty()) {
    return path;
  }

  if (path.size() == 1) {
    auto matches =
        FindVisibleClassDeclsInModule(modules, accessor_module, accessor_module,
                                      path.front());
    const ast::ASTModule* module = FindModule(modules, accessor_module);
    if (module != nullptr) {
      for (const auto& item : module->items) {
        if (const auto* using_decl = std::get_if<ast::UsingDecl>(&item)) {
          AppendUsingBoundClassDecls(matches, modules, available_module_names,
                                     accessor_module, *using_decl, path.front());
        }
      }
    }
    if (matches.size() == 1) {
      ast::ClassPath resolved = matches.front().first;
      resolved.push_back(matches.front().second->name);
      return resolved;
    }
    return path;
  }

  ast::ModulePath module_prefix(path.begin(), path.end() - 1);
  const auto resolved_module = ResolveVisibleModulePath(
      modules, available_module_names, accessor_module, module_prefix);
  if (!resolved_module.has_value()) {
    return path;
  }
  auto matches = FindVisibleClassDeclsInModule(modules, *resolved_module,
                                               accessor_module, path.back());
  if (matches.size() == 1) {
    ast::ClassPath resolved = matches.front().first;
    resolved.push_back(matches.front().second->name);
    return resolved;
  }
  return path;
}

std::vector<ast::ClassPath> ResolveReflectClassPathList(
    const std::vector<const ast::ASTModule*>& modules,
    const source::ModuleNames& available_module_names,
    const ast::ModulePath& accessor_module,
    const std::vector<ast::ClassPath>& paths) {
  std::vector<ast::ClassPath> out;
  out.reserve(paths.size());
  for (const auto& path : paths) {
    out.push_back(ResolveReflectClassPath(modules, available_module_names,
                                          accessor_module, path));
  }
  return out;
}

ScopeContext BuildReflectScopeContext(
    const std::vector<const ast::ASTModule*>& available_modules,
    const source::ModuleNames& available_module_names,
    const ast::ModulePath& current_module) {
  ScopeContext ctx;
  ctx.current_module = current_module;
  ctx.scopes = {UniverseBindings()};

  for (const ast::ASTModule* module : available_modules) {
    if (module == nullptr) {
      continue;
    }
    for (const auto& item : module->items) {
      std::visit(
          [&](const auto& node) {
            using T = std::decay_t<decltype(node)>;
            ast::Path full_path(module->path.begin(), module->path.end());

            if constexpr (std::is_same_v<T, ast::RecordDecl>) {
              auto decl = node;
              decl.implements = ResolveReflectClassPathList(
                  available_modules, available_module_names, module->path,
                  node.implements);
              full_path.push_back(node.name);
              ctx.sigma.types[PathKeyOf(full_path)] = std::move(decl);
            } else if constexpr (std::is_same_v<T, ast::EnumDecl>) {
              auto decl = node;
              decl.implements = ResolveReflectClassPathList(
                  available_modules, available_module_names, module->path,
                  node.implements);
              full_path.push_back(node.name);
              ctx.sigma.types[PathKeyOf(full_path)] = std::move(decl);
            } else if constexpr (std::is_same_v<T, ast::ModalDecl>) {
              auto decl = node;
              decl.implements = ResolveReflectClassPathList(
                  available_modules, available_module_names, module->path,
                  node.implements);
              full_path.push_back(node.name);
              ctx.sigma.types[PathKeyOf(full_path)] = std::move(decl);
            } else if constexpr (std::is_same_v<T, ast::TypeAliasDecl>) {
              full_path.push_back(node.name);
              ctx.sigma.types[PathKeyOf(full_path)] = node;
            } else if constexpr (std::is_same_v<T, ast::ClassDecl>) {
              auto decl = node;
              decl.supers = ResolveReflectClassPathList(
                  available_modules, available_module_names, module->path,
                  node.supers);
              full_path.push_back(node.name);
              ctx.sigma.classes[PathKeyOf(full_path)] = std::move(decl);
            }
          },
          item);
    }
  }

  return ctx;
}

}  // namespace

bool ReflectImplementsForm(
    const std::vector<const ast::ASTModule*>& available_modules,
    const source::ModuleNames& available_module_names,
    const ast::ModulePath& current_module,
    const std::shared_ptr<ast::Type>& type_syntax,
    const ast::ClassPath& form_path) {
  const auto ctx =
      BuildReflectScopeContext(available_modules, available_module_names,
                               current_module);
  const auto lowered = LowerType(ctx, type_syntax);
  if (!lowered.ok || !lowered.type) {
    return false;
  }
  return TypeImplementsClass(ctx, lowered.type, form_path);
}

}  // namespace cursive::analysis
