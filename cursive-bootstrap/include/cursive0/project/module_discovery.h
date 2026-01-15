#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "cursive0/core/diagnostics.h"

namespace cursive0::project {

struct ModuleInfo {
  std::string path;
  std::filesystem::path dir;
};

struct ModulesResult {
  std::vector<ModuleInfo> modules;
  core::DiagnosticStream diags;
};

struct CompilationUnitResult {
  std::vector<std::filesystem::path> files;
  core::DiagnosticStream diags;
};

CompilationUnitResult CompilationUnit(const std::filesystem::path& module_dir);

ModulesResult Modules(const std::filesystem::path& source_root,
                      std::string_view assembly_name);

}  // namespace cursive0::project
