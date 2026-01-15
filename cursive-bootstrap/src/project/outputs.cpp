#include "cursive0/project/outputs.h"

#include <algorithm>
#include <fstream>
#include <unordered_set>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/diagnostic_messages.h"
#include "cursive0/core/diagnostics.h"
#include "cursive0/core/host_primitives.h"
#include "cursive0/core/path.h"
#include "cursive0/core/symbols.h"
#include "cursive0/project/link.h"
#include "cursive0/project/ir_assembly.h"
#include "cursive0/project/tool_resolution.h"
#include "cursive0/project/project.h"

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

std::string_view EmitIrMode(const Project& project) {
  if (project.assembly.emit_ir.has_value()) {
    return *project.assembly.emit_ir;
  }
  return "none";
}

bool IsExecutable(const Project& project) {
  return project.assembly.kind == "executable";
}

bool IsDistinct(const std::vector<std::filesystem::path>& paths) {
  std::unordered_set<std::string> seen;
  seen.reserve(paths.size());
  for (const auto& path : paths) {
    const std::string key = path.generic_string();
    if (!seen.insert(key).second) {
      return false;
    }
  }
  return true;
}

bool UnderPath(const std::filesystem::path& path,
               const std::filesystem::path& root) {
  const std::string path_norm = core::Normalize(path.generic_string());
  const std::string root_norm = core::Normalize(root.generic_string());
  return core::Prefix(path_norm, root_norm);
}

bool EnsureDirDefault(const std::filesystem::path& path) {
  std::error_code ec;
  if (std::filesystem::exists(path, ec)) {
    if (ec) {
      return false;
    }
    return std::filesystem::is_directory(path, ec) && !ec;
  }
  std::filesystem::create_directories(path, ec);
  if (ec) {
    return false;
  }
  return std::filesystem::is_directory(path, ec) && !ec;
}

bool WriteFileDefault(const std::filesystem::path& path,
                      std::string_view bytes) {
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  if (!out) {
    core::HostPrimFail(core::HostPrim::WriteFile, true);
    return false;
  }
  out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
  if (!out) {
    core::HostPrimFail(core::HostPrim::WriteFile, true);
    return false;
  }
  return true;
}

std::optional<std::string> CodegenObjDefault(const ModuleInfo&,
                                             const Project&) {
  return std::nullopt;
}

std::optional<std::string> CodegenIRDefault(const ModuleInfo&,
                                            const Project&,
                                            std::string_view) {
  return std::nullopt;
}

std::optional<std::filesystem::path> ResolveToolDefault(const Project& project,
                                                        std::string_view tool) {
  return ResolveTool(project, tool);
}

std::optional<std::string> AssembleIRDefault(const std::filesystem::path& tool,
                                             std::string_view ir_text) {
  return AssembleIR(tool, ir_text);
}

bool InvokeLinkerDefault(const std::filesystem::path&,
                         const std::vector<std::filesystem::path>&,
                         const std::filesystem::path&) {
  return false;
}

std::optional<std::vector<std::string>> LinkerSymsDefault(
    const std::filesystem::path& tool,
    const std::vector<std::filesystem::path>& inputs,
    const std::filesystem::path& exe) {
  return LinkerSyms(tool, inputs, exe);
}

}  // namespace

OutputPaths ComputeOutputPaths(const std::filesystem::path& project_root,
                               const ValidatedAssembly& assembly) {
  const std::filesystem::path root =
      assembly.out_dir.has_value() ? (project_root / *assembly.out_dir)
                                   : (project_root / "build");
  OutputPaths paths;
  paths.root = root;
  paths.obj_dir = root / "obj";
  paths.ir_dir = root / "ir";
  paths.bin_dir = root / "bin";
  return paths;
}

std::filesystem::path ObjPath(const Project& project, const ModuleInfo& module) {
  const std::string mangled = core::MangleModulePath(module.path);
  return project.outputs.obj_dir / (mangled + ".obj");
}

std::filesystem::path IRPath(const Project& project,
                             const ModuleInfo& module,
                             std::string_view emit_ir) {
  std::string ext = ".ll";
  if (emit_ir == "bc") {
    ext = ".bc";
  }
  const std::string mangled = core::MangleModulePath(module.path);
  return project.outputs.ir_dir / (mangled + ext);
}

std::filesystem::path ExePath(const Project& project) {
  return project.outputs.bin_dir / (project.assembly.name + ".exe");
}

std::vector<std::filesystem::path> ObjPaths(
    const Project& project,
    const std::vector<ModuleInfo>& modules) {
  std::vector<std::filesystem::path> out;
  out.reserve(modules.size());
  for (const auto& module : modules) {
    out.push_back(ObjPath(project, module));
  }
  return out;
}

std::vector<std::filesystem::path> IRPaths(
    const Project& project,
    const std::vector<ModuleInfo>& modules,
    std::string_view emit_ir) {
  std::vector<std::filesystem::path> out;
  if (!(emit_ir == "ll" || emit_ir == "bc")) {
    return out;
  }
  out.reserve(modules.size());
  for (const auto& module : modules) {
    out.push_back(IRPath(project, module, emit_ir));
  }
  return out;
}

std::vector<std::filesystem::path> RequiredOutputs(const Project& project) {
  std::vector<std::filesystem::path> out;
  const auto objs = ObjPaths(project, project.modules);
  out.insert(out.end(), objs.begin(), objs.end());
  const std::string_view emit_ir = EmitIrMode(project);
  if (emit_ir == "ll" || emit_ir == "bc") {
    const auto irs = IRPaths(project, project.modules, emit_ir);
    out.insert(out.end(), irs.begin(), irs.end());
  }
  if (IsExecutable(project)) {
    out.push_back(ExePath(project));
  }
  return out;
}

bool OutputHygiene(const Project& project) {
  const auto required = RequiredOutputs(project);
  for (const auto& path : required) {
    if (!UnderPath(path, project.outputs.root)) {
      return false;
    }
  }
  return true;
}

OutputPipelineResult OutputPipelineWithDeps(const Project& project,
                                            const OutputPipelineDeps& deps) {
  OutputPipelineResult result;

  SPEC_RULE("Out-Start");

  const std::string_view emit_ir = EmitIrMode(project);

  const bool executable = IsExecutable(project);

  if (!deps.ensure_dir(project.outputs.root) ||
      !deps.ensure_dir(project.outputs.obj_dir) ||
      (executable && !deps.ensure_dir(project.outputs.bin_dir)) ||
      ((emit_ir == "ll" || emit_ir == "bc") &&
       !deps.ensure_dir(project.outputs.ir_dir))) {
    SPEC_RULE("Out-Dirs-Err");
    EmitExternal(result.diags, "E-OUT-0401");
    SPEC_RULE("Output-Pipeline-Err");
    return result;
  }
  SPEC_RULE("Out-Dirs-Ok");

  const auto obj_paths = ObjPaths(project, project.modules);
  if (!IsDistinct(obj_paths)) {
    SPEC_RULE("Out-Obj-Collision");
    EmitExternal(result.diags, "E-OUT-0406");
    SPEC_RULE("Output-Pipeline-Err");
    return result;
  }

  std::vector<std::filesystem::path> objs;
  objs.reserve(project.modules.size());
  for (const auto& module : project.modules) {
    const auto bytes = deps.codegen_obj(module, project);
    if (!bytes.has_value()) {
      SPEC_RULE("Out-Obj-Err");
      EmitExternal(result.diags, "E-OUT-0402");
      SPEC_RULE("Output-Pipeline-Err");
      return result;
    }
    SPEC_RULE("CodegenObj-LLVM");
    if (!deps.write_file(ObjPath(project, module), *bytes)) {
      core::HostPrimFail(core::HostPrim::WriteFile, true);
      SPEC_RULE("Out-Obj-Err");
      EmitExternal(result.diags, "E-OUT-0402");
      SPEC_RULE("Output-Pipeline-Err");
      return result;
    }
    SPEC_RULE("Emit-Objects-Cons");
    SPEC_RULE("Out-Obj-Cons");
    objs.push_back(ObjPath(project, module));
  }
  if (project.modules.empty()) {
    SPEC_RULE("Emit-Objects-Empty");
  }
  SPEC_RULE("Out-Obj-Done");

  std::vector<std::filesystem::path> irs;
  if (emit_ir == "none") {
    SPEC_RULE("Emit-IR-None");
    if (executable) {
      SPEC_RULE("Out-IR-None");
    } else {
      SPEC_RULE("Out-IR-None-NoLink");
    }
  } else {
    const auto ir_paths = IRPaths(project, project.modules, emit_ir);
    if (!IsDistinct(ir_paths)) {
      SPEC_RULE("Out-IR-Collision");
      EmitExternal(result.diags, "E-OUT-0406");
      SPEC_RULE("Output-Pipeline-Err");
      return result;
    }

    irs.reserve(project.modules.size());
    for (const auto& module : project.modules) {
      if (emit_ir == "ll") {
        const auto ll_bytes = deps.codegen_ir(module, project, "ll");
        if (!ll_bytes.has_value()) {
          SPEC_RULE("Emit-IR-Err");
          SPEC_RULE("Out-IR-Err");
          EmitExternal(result.diags, "E-OUT-0403");
          SPEC_RULE("Output-Pipeline-Err");
          return result;
        }
        SPEC_RULE("CodegenIR-LLVM");
        if (!deps.write_file(IRPath(project, module, "ll"), *ll_bytes)) {
          core::HostPrimFail(core::HostPrim::WriteFile, true);
          SPEC_RULE("Emit-IR-Err");
          SPEC_RULE("Out-IR-Err");
          EmitExternal(result.diags, "E-OUT-0403");
          SPEC_RULE("Output-Pipeline-Err");
          return result;
        }
        SPEC_RULE("Emit-IR-Cons-LL");
        SPEC_RULE("Out-IR-Cons-LL");
        irs.push_back(IRPath(project, module, "ll"));
      } else {
        const auto ll_bytes = deps.codegen_ir(module, project, "ll");
        if (!ll_bytes.has_value()) {
          SPEC_RULE("Emit-IR-Err");
          SPEC_RULE("Out-IR-Err");
          EmitExternal(result.diags, "E-OUT-0403");
          SPEC_RULE("Output-Pipeline-Err");
          return result;
        }
        SPEC_RULE("CodegenIR-LLVM");
        const auto tool = deps.resolve_tool(project, "llvm-as");
        if (!tool.has_value()) {
          SPEC_RULE("Emit-IR-Err");
          SPEC_RULE("Out-IR-Err");
          EmitExternal(result.diags, "E-OUT-0403");
          SPEC_RULE("Output-Pipeline-Err");
          return result;
        }
        const auto bc_bytes = deps.assemble_ir(*tool, *ll_bytes);
        if (!bc_bytes.has_value()) {
          SPEC_RULE("Emit-IR-Err");
          SPEC_RULE("Out-IR-Err");
          EmitExternal(result.diags, "E-OUT-0403");
          SPEC_RULE("Output-Pipeline-Err");
          return result;
        }
        if (!deps.write_file(IRPath(project, module, "bc"), *bc_bytes)) {
          core::HostPrimFail(core::HostPrim::WriteFile, true);
          SPEC_RULE("Emit-IR-Err");
          SPEC_RULE("Out-IR-Err");
          EmitExternal(result.diags, "E-OUT-0403");
          SPEC_RULE("Output-Pipeline-Err");
          return result;
        }
        SPEC_RULE("Emit-IR-Cons-BC");
        SPEC_RULE("Out-IR-Cons-BC");
        irs.push_back(IRPath(project, module, "bc"));
      }
    }
    if (executable) {
      SPEC_RULE("Out-IR-Done");
    } else {
      SPEC_RULE("Out-IR-Done-NoLink");
    }
  }

  if (!executable) {
    OutputArtifacts artifacts;
    artifacts.objs = std::move(objs);
    artifacts.irs = std::move(irs);
    artifacts.exe.reset();
    result.artifacts = std::move(artifacts);
    SPEC_RULE("Output-Pipeline-Lib");
    return result;
  }

  const LinkDeps link_deps = {
      deps.resolve_tool,
      deps.resolve_runtime_lib,
      deps.linker_syms,
      deps.invoke_linker,
  };

  const LinkResult link_result = LinkWithDeps(objs, project, link_deps);
  for (const auto& diag : link_result.diags) {
    result.diags = core::Emit(result.diags, diag);
  }

  switch (link_result.status) {
    case LinkStatus::Ok:
      SPEC_RULE("Out-Link-Ok");
      break;
    case LinkStatus::NotFound:
      SPEC_RULE("Out-Link-NotFound");
      SPEC_RULE("Output-Pipeline-Err");
      return result;
    case LinkStatus::RuntimeMissing:
      SPEC_RULE("Out-Link-Runtime-Missing");
      SPEC_RULE("Output-Pipeline-Err");
      return result;
    case LinkStatus::RuntimeIncompatible:
      SPEC_RULE("Out-Link-Runtime-Incompatible");
      SPEC_RULE("Output-Pipeline-Err");
      return result;
    case LinkStatus::Fail:
      SPEC_RULE("Out-Link-Fail");
      SPEC_RULE("Output-Pipeline-Err");
      return result;
  }

  OutputArtifacts artifacts;
  artifacts.objs = std::move(objs);
  artifacts.irs = std::move(irs);
  artifacts.exe = ExePath(project);
  result.artifacts = std::move(artifacts);
  SPEC_RULE("Output-Pipeline");
  return result;
}

OutputPipelineResult OutputPipeline(const Project& project) {
  const OutputPipelineDeps deps = {
      EnsureDirDefault,
      CodegenObjDefault,
      CodegenIRDefault,
      WriteFileDefault,
      ResolveToolDefault,
      AssembleIRDefault,
      ResolveRuntimeLib,
      InvokeLinkerDefault,
      LinkerSymsDefault,
  };
  return OutputPipelineWithDeps(project, deps);
}

}  // namespace cursive0::project
