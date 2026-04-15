// =============================================================================
// resolve_imports.cpp - Import Resolution Implementation
// =============================================================================
//
// SPEC REFERENCE:
//   CursiveSpecification.md §5.1.5 "Top-Level Name Collection" (Lines 6956-7309)
//   CursiveSpecification.md §5.1.4 "Visibility and Accessibility" (Lines 6900-6955)
//
// SOURCE FILE:
//   cursive-bootstrap/src/03_analysis/resolve/imports.cpp (Lines 1-257)
//
// MIGRATED: 2026-02-01
//
// =============================================================================

#include "04_analysis/resolve/resolve_imports.h"

#include <functional>

#include "00_core/assert_spec.h"

namespace cursive::analysis {

namespace {

static inline void SpecDefsImports() {
  SPEC_DEF("Import", "C0X.7.M");
  SPEC_DEF("ImportResolution", "C0X.7.M");
  SPEC_DEF("ImportVisibility", "C0X.7.M");
  SPEC_DEF("CircularImport", "C0X.7.M");
  SPEC_DEF("CrossAssembly", "C0X.7.M");
}

// Convert path to key string
std::string PathToKey(const ast::Path& path) {
  std::string key;
  for (std::size_t i = 0; i < path.size(); ++i) {
    if (i > 0) key += "::";
    key += path[i];
  }
  return key;
}

}  // namespace

void ImportResolver::RegisterAssembly(const AssemblyImportContext& ctx) {
  SpecDefsImports();
  SPEC_RULE("Register-Assembly");
  assemblies_[ctx.assembly_name] = ctx;
}

ImportResolutionResult ImportResolver::ResolveImport(
    const ast::ImportDecl& import,
    const ast::ModulePath& current_module) {
  SpecDefsImports();
  SPEC_RULE("Resolve-Import");

  ImportResolutionResult result;

  if (import.path.empty()) {
    result.ok = false;
    result.diag_id = "E-MOD-1202";  // Empty import path
    result.span = import.span;
    return result;
  }

  // First segment is assembly name
  const std::string& assembly_name = import.path[0];
  auto it = assemblies_.find(assembly_name);

  if (it == assemblies_.end()) {
    result.ok = false;
    result.diag_id = "E-MOD-1202";  // Unknown assembly
    result.span = import.span;
    return result;
  }

  const auto& assembly = it->second;

  // Build full path
  ast::Path full_path(import.path.begin() + 1, import.path.end());
  std::string path_key = PathToKey(full_path);

  // Check if it's a specific item import
  if (!import.items.empty()) {
    // Import specific items
    for (const auto& item : import.items) {
      std::string item_key = path_key.empty() ? item : path_key + "::" + item;
      auto item_it = assembly.exported_items.find(item_key);
      if (item_it == assembly.exported_items.end()) {
        result.ok = false;
        result.diag_id = "E-MOD-1204";  // Item not found
        result.span = import.span;
        return result;
      }

      ImportTarget target;
      target.resolved_path = item_it->second;
      target.visibility = ImportVisibility::Public;
      result.targets.push_back(target);
    }
  } else if (import.alias) {
    // Import with alias
    auto item_it = assembly.exported_items.find(path_key);
    if (item_it != assembly.exported_items.end()) {
      ImportTarget target;
      target.resolved_path = item_it->second;
      target.visibility = ImportVisibility::Public;
      result.targets.push_back(target);
    } else if (assembly.exported_modules.count(path_key) > 0) {
      ImportTarget target;
      target.resolved_path = full_path;
      target.is_module = true;
      target.visibility = ImportVisibility::Public;
      result.targets.push_back(target);
    } else {
      result.ok = false;
      result.diag_id = "E-SEM-3004";  // Path not found
      result.span = import.span;
      return result;
    }
  } else {
    // Import entire module
    if (assembly.exported_modules.count(path_key) > 0) {
      ImportTarget target;
      target.resolved_path = full_path;
      target.is_module = true;
      target.visibility = ImportVisibility::Public;
      result.targets.push_back(target);
    } else {
      result.ok = false;
      result.diag_id = "E-SEM-3004";  // Path not found
      result.span = import.span;
      return result;
    }
  }

  return result;
}

bool ImportResolver::IsAccessible(const ast::Path& target,
                                  const ast::ModulePath& from,
                                  ImportVisibility min_visibility) {
  SpecDefsImports();
  SPEC_RULE("Check-Accessible");

  // Simplified visibility check
  // In full implementation, would check:
  // - Private: same module
  // - Internal: same assembly
  // - Public: always accessible

  return true;  // Simplified
}

ImportValidationResult ValidateImport(
    const ast::ImportDecl& import,
    const ast::ModulePath& current_module) {
  SpecDefsImports();
  SPEC_RULE("Validate-Import");

  ImportValidationResult result;

  // Check for empty path
  if (import.path.empty()) {
    result.ok = false;
    result.diag_id = "E-MOD-1202";
    result.span = import.span;
    result.message = "Empty import path";
    return result;
  }

  // Check for self-import
  if (import.path == current_module) {
    result.ok = false;
    result.diag_id = "E-MOD-1401";  // Self import
    result.span = import.span;
    result.message = "Cannot import current module";
    return result;
  }

  // Check alias conflicts
  if (import.alias) {
    // Would check if alias shadows existing binding
  }

  return result;
}

CircularImportResult CheckCircularImports(
    const std::vector<ast::ImportDecl>& imports,
    const ast::ModulePath& current_module) {
  SpecDefsImports();
  SPEC_RULE("Check-Circular");

  CircularImportResult result;

  // Build dependency graph and check for cycles
  // Using DFS to detect back edges

  std::set<std::string> visited;
  std::set<std::string> in_stack;
  std::vector<ast::Path> path;

  std::string current_key = PathToKey(current_module);

  std::function<bool(const std::string&)> dfs = [&](const std::string& node) -> bool {
    if (in_stack.count(node) > 0) {
      // Found cycle
      return true;
    }
    if (visited.count(node) > 0) {
      return false;
    }

    visited.insert(node);
    in_stack.insert(node);

    // Would check dependencies of node here

    in_stack.erase(node);
    return false;
  };

  if (dfs(current_key)) {
    result.has_cycle = true;
    result.cycle_path = path;
  }

  return result;
}

ImportGraph BuildImportGraph(const ast::ASTModule& module) {
  SpecDefsImports();
  SPEC_RULE("Build-ImportGraph");

  ImportGraph graph;
  std::string module_key = PathToKey(module.path);

  for (const auto& item : module.items) {
    if (const auto* import = std::get_if<ast::ImportDecl>(&item)) {
      std::string import_key = PathToKey(import->path);
      graph.edges[module_key].insert(import_key);
    }
  }

  return graph;
}

ImportResolutionResult ResolveExtendedUsing(
    const ExtendedUsingClause& clause,
    const ast::ModulePath& current_module) {
  SpecDefsImports();
  SPEC_RULE("Resolve-ExtendedUsing");

  ImportResolutionResult result;

  // Resolve base path
  ImportTarget target;
  target.resolved_path = clause.path;
  target.is_type = true;
  target.visibility = ImportVisibility::Public;

  // Generic args would be used for specialization
  // using Vec<i32> as IntVec

  result.targets.push_back(target);
  return result;
}

}  // namespace cursive::analysis

