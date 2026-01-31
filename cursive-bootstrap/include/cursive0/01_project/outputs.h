#pragma once

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "cursive0/00_core/diagnostics.h"
#include "cursive0/01_project/module_discovery.h"

namespace cursive0::project {

struct ValidatedAssembly;
struct Project;

struct OutputPaths {
  std::filesystem::path root;
  std::filesystem::path obj_dir;
  std::filesystem::path ir_dir;
  std::filesystem::path bin_dir;
};

OutputPaths ComputeOutputPaths(const std::filesystem::path& project_root,
                               const ValidatedAssembly& assembly);

std::filesystem::path ObjPath(const Project& project, const ModuleInfo& module);
std::filesystem::path IRPath(const Project& project,
                             const ModuleInfo& module,
                             std::string_view emit_ir);
std::filesystem::path ExePath(const Project& project);

std::vector<std::filesystem::path> ObjPaths(const Project& project,
                                            const std::vector<ModuleInfo>& modules);
std::vector<std::filesystem::path> IRPaths(const Project& project,
                                           const std::vector<ModuleInfo>& modules,
                                           std::string_view emit_ir);

std::vector<std::filesystem::path> RequiredOutputs(const Project& project);
bool OutputHygiene(const Project& project);
std::vector<std::string> DumpProject(const Project& project, bool dump_files);

struct OutputArtifacts {
  std::vector<std::filesystem::path> objs;
  std::vector<std::filesystem::path> irs;
  std::optional<std::filesystem::path> exe;
};

struct OutputPipelineResult {
  std::optional<OutputArtifacts> artifacts;
  core::DiagnosticStream diags;
};

struct OutputPipelineDeps {
  std::function<bool(const std::filesystem::path& path)> ensure_dir;
  std::function<std::optional<std::string>(const ModuleInfo& module,
                                           const Project& project)> codegen_obj;
  std::function<std::optional<std::string>(const ModuleInfo& module,
                                           const Project& project,
                                           std::string_view emit_ir)> codegen_ir;
  std::function<bool(const std::filesystem::path& path,
                     std::string_view bytes)> write_file;
  std::function<std::optional<std::filesystem::path>(const Project& project,
                                                     std::string_view tool)> resolve_tool;
  std::function<std::optional<std::string>(const std::filesystem::path& tool,
                                           std::string_view ir_text)> assemble_ir;
  std::function<std::optional<std::filesystem::path>(const Project& project)>
      resolve_runtime_lib;
  std::function<bool(const std::filesystem::path& tool,
                     const std::vector<std::filesystem::path>& inputs,
                     const std::filesystem::path& exe)> invoke_linker;
  std::function<std::optional<std::vector<std::string>>(
      const std::filesystem::path& tool,
      const std::vector<std::filesystem::path>& inputs,
      const std::filesystem::path& exe)> linker_syms;
};

OutputPipelineResult OutputPipelineWithDeps(const Project& project,
                                            const OutputPipelineDeps& deps);

OutputPipelineResult OutputPipeline(const Project& project);

}  // namespace cursive0::project
