#pragma once

#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "cursive0/00_core/span.h"
#include "cursive0/02_syntax/ast.h"

namespace cursive0::analysis {

// C0X Extension: Module Import Resolution

// Visibility level for imports
enum class ImportVisibility {
  Public,    // Visible outside assembly
  Internal,  // Visible within assembly only
  Private,   // Visible within module only
};

// Resolved import target
struct ImportTarget {
  syntax::Path resolved_path;
  bool is_type = false;
  bool is_value = false;
  bool is_class = false;
  bool is_module = false;
  ImportVisibility visibility = ImportVisibility::Public;
};

// Import resolution result
struct ImportResolutionResult {
  bool ok = true;
  std::optional<std::string_view> diag_id;
  std::optional<core::Span> span;
  std::vector<ImportTarget> targets;
};

// Cross-assembly import context
struct AssemblyImportContext {
  std::string assembly_name;
  std::map<std::string, syntax::Path> exported_items;  // name -> path
  std::set<std::string> exported_modules;
};

// Import resolution context
class ImportResolver {
 public:
  ImportResolver() = default;
  
  // Register an assembly's exports
  void RegisterAssembly(const AssemblyImportContext& ctx);
  
  // Resolve an import declaration
  ImportResolutionResult ResolveImport(
      const syntax::ImportDecl& import,
      const syntax::ModulePath& current_module);
  
  // Check if a path is accessible from given location
  bool IsAccessible(const syntax::Path& target,
                    const syntax::ModulePath& from,
                    ImportVisibility min_visibility);
  
  // Get all registered assemblies
  const std::map<std::string, AssemblyImportContext>& Assemblies() const {
    return assemblies_;
  }
  
 private:
  std::map<std::string, AssemblyImportContext> assemblies_;
};

// Import validation result
struct ImportValidationResult {
  bool ok = true;
  std::optional<std::string_view> diag_id;
  std::optional<core::Span> span;
  std::string message;
};

// Validate import declaration
ImportValidationResult ValidateImport(
    const syntax::ImportDecl& import,
    const syntax::ModulePath& current_module);

// Check for circular imports
struct CircularImportResult {
  bool has_cycle = false;
  std::vector<syntax::Path> cycle_path;
};

CircularImportResult CheckCircularImports(
    const std::vector<syntax::ImportDecl>& imports,
    const syntax::ModulePath& current_module);

// Build import graph from module
struct ImportGraph {
  std::map<std::string, std::set<std::string>> edges;  // module -> imports
};

ImportGraph BuildImportGraph(const syntax::ASTModule& module);

// Using declaration enhancement for C0X
// Extended using clause with generic arguments:
// using path<T> as Alias
struct ExtendedUsingClause {
  syntax::Path path;
  std::vector<std::shared_ptr<syntax::Type>> generic_args;
  std::optional<syntax::Identifier> alias;
};

// Resolve extended using clause
ImportResolutionResult ResolveExtendedUsing(
    const ExtendedUsingClause& clause,
    const syntax::ModulePath& current_module);

}  // namespace cursive0::analysis
