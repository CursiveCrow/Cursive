#pragma once

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "00_core/diagnostics.h"
#include "01_project/link.h"
#include "01_project/module_discovery.h"
#include "02_source/parser/parse_modules.h"

namespace cursive::project {

struct Assembly;
struct ValidatedAssembly;
struct Project;

struct OutputPaths {
  std::filesystem::path root;
  std::filesystem::path obj_dir;
  std::filesystem::path ir_dir;
  std::filesystem::path bin_dir;
  std::filesystem::path lib_dir;
};

OutputPaths ComputeOutputPaths(const std::filesystem::path& project_root,
                               const ValidatedAssembly& assembly);
Project AssemblyProject(const Project& base_project, const Assembly& assembly);

std::filesystem::path ObjPath(const Project& project,
                              TargetProfile target_profile,
                              const ModuleInfo& module);
std::filesystem::path IRPath(const Project& project,
                             TargetProfile target_profile,
                             const ModuleInfo& module,
                             std::string_view emit_ir);
std::filesystem::path ExePath(const Project& project,
                              TargetProfile target_profile);
std::filesystem::path SharedLibPath(const Project& project,
                                    TargetProfile target_profile);
std::filesystem::path StaticLibPath(const Project& project,
                                    TargetProfile target_profile);
std::optional<std::filesystem::path> ImportLibPath(const Project& project,
                                                   TargetProfile target_profile);
std::optional<std::filesystem::path> MapPath(const Project& project,
                                             TargetProfile target_profile);
std::optional<std::filesystem::path> PrimaryArtifactPath(
    const Project& project,
    TargetProfile target_profile);
std::vector<std::filesystem::path> LibraryArtifactInputs(
    const std::vector<std::filesystem::path>& inputs);
std::optional<LinkOutputKind> LinkMode(const Project& project);
std::optional<std::filesystem::path> LinkOutputPath(
    const Project& project,
    TargetProfile target_profile);
bool UsesBinDir(const Project& project, TargetProfile target_profile);
bool UsesLibDir(const Project& project, TargetProfile target_profile);

std::vector<std::filesystem::path> ObjPaths(
    const Project& project,
    TargetProfile target_profile,
    const std::vector<ModuleInfo>& modules);
std::vector<std::filesystem::path> IRPaths(
    const Project& project,
    TargetProfile target_profile,
    const std::vector<ModuleInfo>& modules,
    std::string_view emit_ir);

std::vector<std::filesystem::path> RequiredOutputs(const Project& project,
                                                   TargetProfile target_profile);
bool OutputHygiene(const Project& project, TargetProfile target_profile);
std::vector<std::string> DumpProject(const Project& project,
                                     TargetProfile target_profile,
                                     bool dump_files);

struct OutputArtifacts {
  std::vector<std::filesystem::path> objs;
  std::vector<std::filesystem::path> irs;
  std::optional<std::filesystem::path> primary_artifact;
  std::optional<std::filesystem::path> import_lib;
  std::optional<std::filesystem::path> map_file;
};

struct OutputPipelineResult {
  std::optional<OutputArtifacts> artifacts;
  core::DiagnosticStream diags;
};

struct IncrementalModuleInfo {
  std::string source_hash;
  std::string public_hash;
  std::string full_hash;
  std::vector<std::string> dependencies;
};

struct SharedLibraryExports {
  std::vector<std::string> export_symbols;
  std::vector<std::string> data_export_symbols;
};

struct OutputPipelineDeps {
  std::function<bool(const std::filesystem::path& path)> ensure_dir;
  std::function<std::optional<std::string>(const ModuleInfo& module,
                                           const Project& project)> codegen_obj;
  std::function<std::optional<std::string>(const ModuleInfo& module,
                                           const Project& project,
                                           std::string_view emit_ir)>
      codegen_ir;
  std::function<bool(const std::filesystem::path& path, std::string_view bytes)>
      write_file;
  std::function<std::optional<std::filesystem::path>(const Project& project,
                                                     TargetProfile target_profile,
                                                     std::string_view tool)>
      resolve_tool;
  std::function<std::optional<std::string>(const std::filesystem::path& tool,
                                           std::string_view ir_text)>
      assemble_ir;
  std::function<std::optional<std::filesystem::path>(const Project& project,
                                                     TargetProfile target_profile)>
      resolve_runtime_lib;
  std::function<LinkInvocationResult(
      const std::filesystem::path& tool,
      const std::vector<std::filesystem::path>& inputs,
      const std::filesystem::path& output,
      const std::optional<std::filesystem::path>& import_lib,
      const LinkPlan& plan)>
      invoke_linker;
  std::function<std::optional<std::vector<std::string>>(
      const std::filesystem::path& tool,
      const std::vector<std::filesystem::path>& inputs,
      const std::filesystem::path& output)>
      linker_syms;
  std::function<std::optional<std::vector<std::filesystem::path>>(
      const std::filesystem::path& archive)>
      archive_members;
  std::function<bool(const std::filesystem::path& tool,
                     const std::vector<std::filesystem::path>& inputs,
                     const std::filesystem::path& output)>
      invoke_archiver;
  std::function<std::optional<std::reference_wrapper<const std::vector<ast::ASTModule>>>(
      const Project& project)>
      resolve_project_ast_modules;
  std::function<frontend::ParseModulesResult(const Project& project)>
      resolve_ast_modules;
  std::function<std::optional<SharedLibraryExports>(const Project& project)>
      resolve_shared_library_exports;
  std::function<bool(const Project& project,
                     const SharedLibraryExports& exports)>
      prepare_codegen_context;
  std::function<std::optional<IncrementalModuleInfo>(const ModuleInfo& module,
                                                     const Project& project)>
      incremental_module;
  std::function<std::optional<std::string>(const Project& project)>
      incremental_build_key;
  bool codegen_obj_thread_safe = false;
};

OutputPipelineResult OutputPipeline(const Project& project,
                                    TargetProfile target_profile,
                                    const OutputPipelineDeps& deps);

}  // namespace cursive::project
