#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "cursive0/core/diagnostics.h"
#include "cursive0/project/module_discovery.h"

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
  bool (*ensure_dir)(const std::filesystem::path& path);
  std::optional<std::string> (*codegen_obj)(const ModuleInfo& module,
                                            const Project& project);
  std::optional<std::string> (*codegen_ir)(const ModuleInfo& module,
                                           const Project& project,
                                           std::string_view emit_ir);
  bool (*write_file)(const std::filesystem::path& path, std::string_view bytes);
  std::optional<std::filesystem::path> (*resolve_tool)(const Project& project,
                                                       std::string_view tool);
  std::optional<std::string> (*assemble_ir)(const std::filesystem::path& tool,
                                            std::string_view ir_text);
  std::optional<std::filesystem::path> (*resolve_runtime_lib)(
      const Project& project);
  bool (*invoke_linker)(const std::filesystem::path& tool,
                        const std::vector<std::filesystem::path>& inputs,
                        const std::filesystem::path& exe);
  std::optional<std::vector<std::string>> (*linker_syms)(
      const std::filesystem::path& tool,
      const std::vector<std::filesystem::path>& inputs,
      const std::filesystem::path& exe);
};

OutputPipelineResult OutputPipelineWithDeps(const Project& project,
                                            const OutputPipelineDeps& deps);

OutputPipelineResult OutputPipeline(const Project& project);

}  // namespace cursive0::project
