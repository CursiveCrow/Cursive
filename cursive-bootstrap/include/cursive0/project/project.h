#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "cursive0/core/diagnostics.h"
#include "cursive0/core/path.h"
#include "cursive0/project/manifest.h"
#include "cursive0/project/module_discovery.h"
#include "cursive0/project/outputs.h"

namespace cursive0::project {

struct ValidatedAssembly {
  std::string name;
  std::string kind;
  std::string root;
  std::optional<std::string> out_dir;
  std::optional<std::string> emit_ir;
};

struct Assembly {
  std::string name;
  std::string kind;
  std::string root;
  std::optional<std::string> out_dir;
  std::optional<std::string> emit_ir;
  std::filesystem::path source_root;
  OutputPaths outputs;
  std::vector<ModuleInfo> modules;
};

struct ManifestValidationResult {
  std::vector<ValidatedAssembly> assemblies;
  core::DiagnosticStream diags;
};

ManifestValidationResult ValidateManifest(
    const std::filesystem::path& project_root,
    const TOMLTable& table);

bool IsProjectRoot(const std::filesystem::path& root);

struct Project {
  std::filesystem::path root;
  std::vector<Assembly> assemblies;
  Assembly assembly;
  std::filesystem::path source_root;
  OutputPaths outputs;
  std::vector<ModuleInfo> modules;
};

struct LoadProjectResult {
  std::optional<Project> project;
  core::DiagnosticStream diags;
};

struct LoadProjectDeps {
  ManifestParseResult (*parse)(const std::filesystem::path& project_root);
  ManifestValidationResult (*validate)(const std::filesystem::path& project_root,
                                       const TOMLTable& table);
  std::optional<core::ResolveResult> (*resolve)(std::string_view root,
                                                std::string_view path);
  bool (*is_dir)(const std::filesystem::path& path);
  ModulesResult (*modules)(const std::filesystem::path& source_root,
                           std::string_view assembly_name);
  OutputPaths (*outputs)(const std::filesystem::path& project_root,
                         const ValidatedAssembly& assembly);
};

LoadProjectResult LoadProjectWithDeps(const std::filesystem::path& project_root,
                                      const std::optional<std::string>& target,
                                      const LoadProjectDeps& deps);

LoadProjectResult LoadProject(const std::filesystem::path& project_root,
                              const std::optional<std::string>& target);

LoadProjectResult LoadProject(const std::filesystem::path& project_root);

}  // namespace cursive0::project
