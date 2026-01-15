#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "cursive0/core/diagnostics.h"
#include "cursive0/core/source_load.h"
#include "cursive0/project/module_discovery.h"
#include "cursive0/syntax/parser.h"

namespace cursive0::frontend {

struct ReadBytesResult {
  std::optional<std::vector<std::uint8_t>> bytes;
  core::DiagnosticStream diags;
};

struct InspectResult {
  core::DiagnosticStream diags;
  bool subset_ok = true;
};

struct ParseModuleDeps {
  project::CompilationUnitResult (*compilation_unit)(
      const std::filesystem::path& module_dir);
  ReadBytesResult (*read_bytes)(const std::filesystem::path& path);
  core::SourceLoadResult (*load_source)(
      std::string_view path,
      const std::vector<std::uint8_t>& bytes);
  syntax::ParseFileResult (*parse_file)(const core::SourceFile& source);
  InspectResult (*inspect_source)(const core::SourceFile& source) = nullptr;
};

struct ParseModuleResult {
  std::optional<syntax::ASTModule> module;
  core::DiagnosticStream diags;
  bool subset_ok = true;
};

struct ParseModulesResult {
  std::optional<std::vector<syntax::ASTModule>> modules;
  core::DiagnosticStream diags;
  bool subset_ok = true;
};

ReadBytesResult ReadBytesDefault(const std::filesystem::path& path);

ParseModuleResult ParseModuleWithDeps(std::string_view module_path,
                                      const std::filesystem::path& source_root,
                                      std::string_view assembly_name,
                                      const ParseModuleDeps& deps);

ParseModulesResult ParseModulesWithDeps(
    const std::vector<project::ModuleInfo>& modules,
    const std::filesystem::path& source_root,
    std::string_view assembly_name,
    const ParseModuleDeps& deps);

ParseModuleResult ParseModule(std::string_view module_path,
                              const std::filesystem::path& source_root,
                              std::string_view assembly_name);

ParseModulesResult ParseModules(const std::vector<project::ModuleInfo>& modules,
                                const std::filesystem::path& source_root,
                                std::string_view assembly_name);

}  // namespace cursive0::frontend
