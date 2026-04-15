#include "03_comptime/comptime.h"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "00_core/assert_spec.h"
#include "00_core/symbols.h"
#include "02_source/module_paths.h"
#include "03_comptime/comptime_internal.h"

namespace cursive::frontend {
namespace {

std::vector<std::size_t> DeterministicDependencyOrder(
    const std::vector<ast::ASTModule>& modules) {
  if (modules.empty()) {
    return {};
  }

  std::unordered_map<std::string, std::size_t> index_by_path;
  source::ModuleNames module_names;
  index_by_path.reserve(modules.size());
  module_names.reserve(modules.size());
  for (std::size_t i = 0; i < modules.size(); ++i) {
    const std::string path = core::StringOfPath(modules[i].path);
    index_by_path[path] = i;
    module_names.insert(path);
  }

  std::vector<std::vector<std::size_t>> outgoing(modules.size());
  std::vector<std::size_t> indegree(modules.size(), 0);
  std::unordered_set<std::string> seen_edges;
  for (std::size_t i = 0; i < modules.size(); ++i) {
    const auto& module = modules[i];
    for (const auto& item : module.items) {
      const auto* import = std::get_if<ast::ImportDecl>(&item);
      if (!import) {
        continue;
      }
      const auto resolved =
          source::ResolveImportModulePath(module.path, module_names, import->path);
      if (!resolved.has_value()) {
        continue;
      }
      const std::string dep_path = core::StringOfPath(*resolved);
      auto dep_it = index_by_path.find(dep_path);
      if (dep_it == index_by_path.end()) {
        continue;
      }
      const std::size_t dep_idx = dep_it->second;
      if (dep_idx == i) {
        continue;
      }
      const std::string edge_key =
          dep_path + "->" + core::StringOfPath(module.path);
      if (!seen_edges.insert(edge_key).second) {
        continue;
      }
      outgoing[dep_idx].push_back(i);
      ++indegree[i];
    }
  }

  auto path_less = [&](std::size_t lhs, std::size_t rhs) {
    const std::string lhs_path = core::StringOfPath(modules[lhs].path);
    const std::string rhs_path = core::StringOfPath(modules[rhs].path);
    return lhs_path < rhs_path;
  };

  std::vector<std::size_t> ready;
  ready.reserve(modules.size());
  for (std::size_t i = 0; i < modules.size(); ++i) {
    if (indegree[i] == 0) {
      ready.push_back(i);
    }
  }
  std::stable_sort(ready.begin(), ready.end(), path_less);

  std::vector<std::size_t> order;
  order.reserve(modules.size());
  while (!ready.empty()) {
    const std::size_t current = ready.front();
    ready.erase(ready.begin());
    order.push_back(current);

    for (const std::size_t next : outgoing[current]) {
      if (indegree[next] == 0) {
        continue;
      }
      --indegree[next];
      if (indegree[next] == 0) {
        ready.push_back(next);
      }
    }
    std::stable_sort(ready.begin(), ready.end(), path_less);
  }

  if (order.size() == modules.size()) {
    return order;
  }

  std::vector<std::size_t> unresolved;
  unresolved.reserve(modules.size() - order.size());
  std::vector<bool> emitted(modules.size(), false);
  for (const std::size_t idx : order) {
    emitted[idx] = true;
  }
  for (std::size_t i = 0; i < modules.size(); ++i) {
    if (!emitted[i]) {
      unresolved.push_back(i);
    }
  }
  std::stable_sort(unresolved.begin(), unresolved.end(), path_less);
  order.insert(order.end(), unresolved.begin(), unresolved.end());
  return order;
}

}  // namespace

ComptimeResult ComptimePass(const std::vector<ast::ASTModule>& modules,
                            const std::filesystem::path& project_root,
                            const std::filesystem::path& source_root) {
  SPEC_RULE("ComptimePass");
  const std::vector<std::size_t> order = DeterministicDependencyOrder(modules);
  auto project_files = comptime_internal::CaptureProjectFileSnapshot(project_root);

  ComptimeResult result;
  std::vector<ast::ASTModule> expanded = modules;
  std::vector<const ast::ASTModule*> available_modules;
  available_modules.reserve(order.size());
  for (const std::size_t module_index : order) {
    SPEC_RULE("Phase2-Deterministic-Dependency-Order");
    const auto& module = modules[module_index];
    available_modules.push_back(&module);

    source::ModuleNames available_module_names;
    available_module_names.reserve(available_modules.size());
    for (const ast::ASTModule* available : available_modules) {
      if (available != nullptr) {
        available_module_names.insert(core::StringOfPath(available->path));
      }
    }

    comptime_internal::CtEnv env;
    env.diags = &result.diags;
    env.project_root = project_root;
    env.source_root = source_root;
    env.next_hygiene = 0;
    env.current_module = module.path;
    env.site.module_path = module.path;
    env.site.ordinal = 0;
    env.available_modules = available_modules;
    env.available_module_names = std::move(available_module_names);
    env.files = project_files;

    ast::ASTModule out = module;
    auto expanded_items = comptime_internal::ExpandModuleItems(module.items, env);
    if (!expanded_items.has_value() || core::HasError(result.diags)) {
      return result;
    }
    out.items = std::move(*expanded_items);
    expanded[module_index] = std::move(out);
  }

  result.modules = std::move(expanded);
  return result;
}

ComptimeResult ExecuteComptime(const std::vector<ast::ASTModule>& modules,
                               const std::filesystem::path& project_root,
                               const std::filesystem::path& source_root) {
  SPEC_RULE("ExecuteComptime");
  SPEC_RULE("ExecuteComptime-By-ComptimePass");
  return ComptimePass(modules, project_root, source_root);
}

}  // namespace cursive::frontend
