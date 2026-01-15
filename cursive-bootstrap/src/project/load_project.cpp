#include "cursive0/project/project.h"

#include <algorithm>
#include <string_view>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/diagnostic_messages.h"
#include "cursive0/core/diagnostics.h"
#include "cursive0/project/deterministic_order.h"
#include "cursive0/project/module_discovery.h"
#include "cursive0/project/outputs.h"

namespace cursive0::project {

namespace {

std::optional<core::Diagnostic> MakeExternalDiag(std::string_view code) {
  auto diag = core::MakeDiagnostic(code);
  if (diag) {
    diag->span.reset();
  }
  return diag;
}

void EmitExternal(core::DiagnosticStream& diags, std::string_view code) {
  if (auto diag = MakeExternalDiag(code)) {
    diags = core::Emit(diags, *diag);
  }
}

bool IsDirDefault(const std::filesystem::path& path) {
  std::error_code ec;
  const bool ok = std::filesystem::is_directory(path, ec);
  if (ec) {
    return false;
  }
  return ok;
}

std::optional<Assembly> BuildAssembly(const std::filesystem::path& project_root,
                                      const ValidatedAssembly& spec,
                                      const LoadProjectDeps& deps,
                                      core::DiagnosticStream& diags) {
  const std::string root_str = project_root.generic_string();
  const std::optional<core::ResolveResult> resolved =
      deps.resolve(root_str, spec.root);
  if (!resolved.has_value()) {
    SPEC_RULE("BuildAssembly-Err-Resolve");
    EmitExternal(diags, "E-PRJ-0304");
    return std::nullopt;
  }

  const std::filesystem::path source_root = resolved->path;
  if (!deps.is_dir(source_root)) {
    SPEC_RULE("WF-Source-Root-Err");
    SPEC_RULE("BuildAssembly-Err-Root");
    EmitExternal(diags, "E-PRJ-0302");
    return std::nullopt;
  }
  SPEC_RULE("WF-Source-Root");

  const ModulesResult modules_result = deps.modules(source_root, spec.name);
  for (const auto& diag : modules_result.diags) {
    diags = core::Emit(diags, diag);
  }
  if (core::HasError(modules_result.diags)) {
    SPEC_RULE("ModuleList-Err");
    SPEC_RULE("BuildAssembly-Err-Modules");
    return std::nullopt;
  }

  std::vector<ModuleInfo> modules = modules_result.modules;
  std::stable_sort(modules.begin(), modules.end(),
                   [](const ModuleInfo& lhs, const ModuleInfo& rhs) {
                     const std::string lhs_fold = Fold(lhs.path);
                     const std::string rhs_fold = Fold(rhs.path);
                     if (lhs_fold == rhs_fold) {
                       return Utf8LexLess(lhs.path, rhs.path);
                     }
                     return Utf8LexLess(lhs_fold, rhs_fold);
                   });
  SPEC_RULE("ModuleList-Ok");

  Assembly assembly;
  assembly.name = spec.name;
  assembly.kind = spec.kind;
  assembly.root = spec.root;
  assembly.out_dir = spec.out_dir;
  assembly.emit_ir = spec.emit_ir;
  assembly.source_root = source_root;
  assembly.outputs = deps.outputs(project_root, spec);
  assembly.modules = std::move(modules);

  SPEC_RULE("BuildAssembly-Ok");
  return assembly;
}

std::optional<Assembly> SelectAssembly(
    const std::vector<Assembly>& assemblies,
    const std::optional<std::string>& target,
    core::DiagnosticStream& diags) {
  if (!target.has_value()) {
    if (assemblies.size() == 1) {
      SPEC_RULE("Select-Only");
      return assemblies.front();
    }
    SPEC_RULE("Select-Err");
    EmitExternal(diags, "E-PRJ-0205");
    return std::nullopt;
  }
  for (const auto& assembly : assemblies) {
    if (assembly.name == *target) {
      SPEC_RULE("Select-By-Name");
      return assembly;
    }
  }
  SPEC_RULE("Select-Err");
  EmitExternal(diags, "E-PRJ-0205");
  return std::nullopt;
}

LoadProjectResult LoadProjectImpl(const std::filesystem::path& project_root,
                                  const std::optional<std::string>& target,
                                  const LoadProjectDeps& deps) {
  LoadProjectResult result;

  const ManifestParseResult parsed = deps.parse(project_root);
  for (const auto& diag : parsed.diags) {
    result.diags = core::Emit(result.diags, diag);
  }
  if (!parsed.table.has_value()) {
    SPEC_RULE("Step-Parse-Err");
    SPEC_RULE("LoadProject-Err");
    return result;
  }
  SPEC_RULE("Step-Parse");

  const ManifestValidationResult validated =
      deps.validate(project_root, *parsed.table);
  for (const auto& diag : validated.diags) {
    result.diags = core::Emit(result.diags, diag);
  }
  if (core::HasError(validated.diags) || validated.assemblies.empty()) {
    SPEC_RULE("Step-Validate-Err");
    SPEC_RULE("LoadProject-Err");
    return result;
  }
  SPEC_RULE("Step-Validate");

  SPEC_RULE("Step-Asm-Init");
  std::vector<Assembly> assemblies;
  assemblies.reserve(validated.assemblies.size());
  for (const auto& spec : validated.assemblies) {
    SPEC_RULE("Step-Asm-Cons");
    const auto built = BuildAssembly(project_root, spec, deps, result.diags);
    if (!built.has_value()) {
      SPEC_RULE("Step-Asm-Err");
      SPEC_RULE("LoadProject-Err");
      return result;
    }
    assemblies.push_back(*built);
  }

  const auto selected = SelectAssembly(assemblies, target, result.diags);
  if (!selected.has_value()) {
    SPEC_RULE("Step-Asm-Done-Err");
    SPEC_RULE("LoadProject-Err");
    return result;
  }

  Project project;
  project.root = project_root;
  project.assemblies = std::move(assemblies);
  project.assembly = *selected;
  project.source_root = selected->source_root;
  project.outputs = selected->outputs;
  project.modules = selected->modules;

  result.project = std::move(project);
  SPEC_RULE("Step-Asm-Done");
  SPEC_RULE("LoadProject-Ok");
  return result;
}

}  // namespace

LoadProjectResult LoadProjectWithDeps(const std::filesystem::path& project_root,
                                      const std::optional<std::string>& target,
                                      const LoadProjectDeps& deps) {
  return LoadProjectImpl(project_root, target, deps);
}

LoadProjectResult LoadProject(const std::filesystem::path& project_root,
                              const std::optional<std::string>& target) {
  const LoadProjectDeps deps = {
      ParseManifest,
      ValidateManifest,
      core::Resolve,
      IsDirDefault,
      Modules,
      ComputeOutputPaths,
  };
  return LoadProjectImpl(project_root, target, deps);
}

LoadProjectResult LoadProject(const std::filesystem::path& project_root) {
  return LoadProject(project_root, std::nullopt);
}

}  // namespace cursive0::project
