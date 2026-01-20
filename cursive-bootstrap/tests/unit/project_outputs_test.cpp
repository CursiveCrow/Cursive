#include <cassert>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/diagnostic_messages.h"
#include "cursive0/core/diagnostics.h"
#include "cursive0/project/link.h"
#include "cursive0/project/outputs.h"
#include "cursive0/project/project.h"

namespace {

using cursive0::core::DiagnosticStream;
using cursive0::core::HasError;
using cursive0::project::Assembly;
using cursive0::project::ModuleInfo;
using cursive0::project::OutputPipelineDeps;
using cursive0::project::OutputPipelineResult;
using cursive0::project::OutputPipelineWithDeps;
using cursive0::project::Project;
using cursive0::project::RuntimeRequiredSyms;
using cursive0::project::ValidatedAssembly;

struct TestConfig {
  bool ensure_dir_ok = true;
  bool write_file_ok = true;
  bool codegen_obj_ok = true;
  bool codegen_ir_ok = true;
  bool resolve_tool_ok = true;
  bool assemble_ir_ok = true;
  bool resolve_runtime_lib_ok = true;
  bool invoke_linker_ok = true;
  bool linker_syms_ok = true;
  std::string codegen_obj_bytes = "OBJ";
  std::string codegen_ir_bytes = "IR";
  std::string assemble_ir_bytes = "BC";
  std::vector<std::string> linker_syms;
};

TestConfig* g_cfg = nullptr;

bool EnsureDirStub(const std::filesystem::path&) {
  return g_cfg && g_cfg->ensure_dir_ok;
}

std::optional<std::string> CodegenObjStub(const ModuleInfo&,
                                          const Project&) {
  if (!g_cfg || !g_cfg->codegen_obj_ok) {
    return std::nullopt;
  }
  return g_cfg->codegen_obj_bytes;
}

std::optional<std::string> CodegenIrStub(const ModuleInfo&,
                                         const Project&,
                                         std::string_view) {
  if (!g_cfg || !g_cfg->codegen_ir_ok) {
    return std::nullopt;
  }
  return g_cfg->codegen_ir_bytes;
}

bool WriteFileStub(const std::filesystem::path&, std::string_view) {
  return g_cfg && g_cfg->write_file_ok;
}

std::optional<std::filesystem::path> ResolveToolStub(const Project&,
                                                     std::string_view tool) {
  if (!g_cfg || !g_cfg->resolve_tool_ok) {
    return std::nullopt;
  }
  return std::filesystem::path("/tools") / std::string(tool);
}

std::optional<std::string> AssembleIrStub(const std::filesystem::path&,
                                          std::string_view) {
  if (!g_cfg || !g_cfg->assemble_ir_ok) {
    return std::nullopt;
  }
  return g_cfg->assemble_ir_bytes;
}

std::optional<std::filesystem::path> ResolveRuntimeLibStub(const Project& project) {
  if (!g_cfg || !g_cfg->resolve_runtime_lib_ok) {
    return std::nullopt;
  }
  return project.root / "runtime" / "cursive0_rt.lib";
}

bool InvokeLinkerStub(const std::filesystem::path&,
                      const std::vector<std::filesystem::path>&,
                      const std::filesystem::path&) {
  return g_cfg && g_cfg->invoke_linker_ok;
}

std::optional<std::vector<std::string>> LinkerSymsStub(
    const std::filesystem::path&,
    const std::vector<std::filesystem::path>&,
    const std::filesystem::path&) {
  if (!g_cfg || !g_cfg->linker_syms_ok) {
    return std::nullopt;
  }
  return g_cfg->linker_syms;
}

OutputPipelineDeps MakeDeps() {
  return OutputPipelineDeps{
      EnsureDirStub,
      CodegenObjStub,
      CodegenIrStub,
      WriteFileStub,
      ResolveToolStub,
      AssembleIrStub,
      ResolveRuntimeLibStub,
      InvokeLinkerStub,
      LinkerSymsStub,
  };
}

bool HasCode(const DiagnosticStream& diags, std::string_view code) {
  for (const auto& diag : diags) {
    if (diag.code == code) {
      return true;
    }
  }
  return false;
}

Project MakeProject(const std::filesystem::path& root,
                    std::string_view kind,
                    std::optional<std::string> emit_ir,
                    const std::vector<std::string>& module_paths) {
  Project project;
  project.root = root;
  project.source_root = root / "src";
  const ValidatedAssembly spec{"app", std::string(kind), "src", std::nullopt, emit_ir};
  Assembly assembly;
  assembly.name = spec.name;
  assembly.kind = spec.kind;
  assembly.root = spec.root;
  assembly.out_dir = spec.out_dir;
  assembly.emit_ir = spec.emit_ir;
  assembly.source_root = project.source_root;
  assembly.outputs = cursive0::project::ComputeOutputPaths(project.root, spec);
  project.outputs = assembly.outputs;
  for (const auto& path : module_paths) {
    ModuleInfo info;
    info.path = path;
    info.dir = project.source_root / path;
    project.modules.push_back(info);
  }
  assembly.modules = project.modules;
  project.assembly = assembly;
  project.assemblies = {assembly};
  return project;
}

}  // namespace

int main() {
  {
    SPEC_COV("Out-Start");
    SPEC_COV("Out-Dirs-Ok");
    SPEC_COV("Emit-Objects-Cons");
    SPEC_COV("Out-Obj-Cons");
    SPEC_COV("Out-Obj-Done");
    SPEC_COV("CodegenObj-LLVM");
    SPEC_COV("Emit-IR-None");
    SPEC_COV("Out-IR-None");
    SPEC_COV("Out-Link-Ok");
    SPEC_COV("Output-Pipeline");
    SPEC_COV("Link-Ok");

    TestConfig cfg;
    cfg.linker_syms = RuntimeRequiredSyms();
    g_cfg = &cfg;
    const Project project = MakeProject("/root", "executable", std::nullopt, {"main"});
    const OutputPipelineResult result = OutputPipelineWithDeps(project, MakeDeps());
    assert(result.artifacts.has_value());
    assert(!HasError(result.diags));
    assert(result.artifacts->objs.size() == 1);
  }

  {
    SPEC_COV("Emit-Objects-Empty");
    TestConfig cfg;
    cfg.linker_syms = RuntimeRequiredSyms();
    g_cfg = &cfg;
    const Project project = MakeProject("/root", "executable", std::nullopt, {});
    const OutputPipelineResult result = OutputPipelineWithDeps(project, MakeDeps());
    assert(result.artifacts.has_value());
    assert(!HasError(result.diags));
    assert(result.artifacts->objs.empty());
  }

  {
    SPEC_COV("Out-Dirs-Err");
    SPEC_COV("Output-Pipeline-Err");
    TestConfig cfg;
    cfg.ensure_dir_ok = false;
    g_cfg = &cfg;
    const Project project = MakeProject("/root", "executable", std::nullopt, {"main"});
    const OutputPipelineResult result = OutputPipelineWithDeps(project, MakeDeps());
    assert(!result.artifacts.has_value());
    assert(HasError(result.diags));
    assert(HasCode(result.diags, "E-OUT-0401"));
  }

  {
    SPEC_COV("Out-Obj-Collision");
    TestConfig cfg;
    cfg.linker_syms = RuntimeRequiredSyms();
    g_cfg = &cfg;
    const Project project = MakeProject("/root", "executable", std::nullopt, {"dup", "dup"});
    const OutputPipelineResult result = OutputPipelineWithDeps(project, MakeDeps());
    assert(!result.artifacts.has_value());
    assert(HasError(result.diags));
    assert(HasCode(result.diags, "E-OUT-0406"));
  }

  {
    SPEC_COV("Out-Obj-Err");
    TestConfig cfg;
    cfg.codegen_obj_ok = false;
    g_cfg = &cfg;
    const Project project = MakeProject("/root", "executable", std::nullopt, {"main"});
    const OutputPipelineResult result = OutputPipelineWithDeps(project, MakeDeps());
    assert(!result.artifacts.has_value());
    assert(HasError(result.diags));
    assert(HasCode(result.diags, "E-OUT-0402"));
  }

  {
    SPEC_COV("Emit-IR-Cons-LL");
    SPEC_COV("Out-IR-Cons-LL");
    SPEC_COV("Out-IR-Done");
    SPEC_COV("CodegenIR-LLVM");
    TestConfig cfg;
    cfg.linker_syms = RuntimeRequiredSyms();
    g_cfg = &cfg;
    const Project project = MakeProject("/root", "executable", std::optional<std::string>("ll"), {"main"});
    const OutputPipelineResult result = OutputPipelineWithDeps(project, MakeDeps());
    assert(result.artifacts.has_value());
    assert(!HasError(result.diags));
    assert(result.artifacts->irs.size() == 1);
  }

  {
    SPEC_COV("Emit-IR-Cons-BC");
    SPEC_COV("Out-IR-Cons-BC");
    SPEC_COV("CodegenIR-LLVM");
    TestConfig cfg;
    cfg.linker_syms = RuntimeRequiredSyms();
    g_cfg = &cfg;
    const Project project = MakeProject("/root", "executable", std::optional<std::string>("bc"), {"main"});
    const OutputPipelineResult result = OutputPipelineWithDeps(project, MakeDeps());
    assert(result.artifacts.has_value());
    assert(!HasError(result.diags));
    assert(result.artifacts->irs.size() == 1);
  }

  {
    SPEC_COV("Out-IR-Collision");
    TestConfig cfg;
    cfg.linker_syms = RuntimeRequiredSyms();
    g_cfg = &cfg;
    const Project project = MakeProject("/root", "executable", std::optional<std::string>("ll"), {"dup", "dup"});
    const OutputPipelineResult result = OutputPipelineWithDeps(project, MakeDeps());
    assert(!result.artifacts.has_value());
    assert(HasError(result.diags));
    assert(HasCode(result.diags, "E-OUT-0406"));
  }

  {
    SPEC_COV("Emit-IR-Err");
    SPEC_COV("Out-IR-Err");
    TestConfig cfg;
    cfg.codegen_ir_ok = false;
    g_cfg = &cfg;
    const Project project = MakeProject("/root", "executable", std::optional<std::string>("ll"), {"main"});
    const OutputPipelineResult result = OutputPipelineWithDeps(project, MakeDeps());
    assert(!result.artifacts.has_value());
    assert(HasError(result.diags));
    assert(HasCode(result.diags, "E-OUT-0403"));
  }

  {
    SPEC_COV("Out-Link-NotFound");
    SPEC_COV("Link-NotFound");
    TestConfig cfg;
    cfg.resolve_tool_ok = false;
    g_cfg = &cfg;
    const Project project = MakeProject("/root", "executable", std::nullopt, {"main"});
    const OutputPipelineResult result = OutputPipelineWithDeps(project, MakeDeps());
    assert(!result.artifacts.has_value());
    assert(HasError(result.diags));
    assert(HasCode(result.diags, "E-OUT-0405"));
  }

  {
    SPEC_COV("Out-Link-Runtime-Missing");
    SPEC_COV("Link-Runtime-Missing");
    TestConfig cfg;
    cfg.resolve_runtime_lib_ok = false;
    g_cfg = &cfg;
    const Project project = MakeProject("/root", "executable", std::nullopt, {"main"});
    const OutputPipelineResult result = OutputPipelineWithDeps(project, MakeDeps());
    assert(!result.artifacts.has_value());
    assert(HasError(result.diags));
    assert(HasCode(result.diags, "E-OUT-0407"));
  }

  {
    SPEC_COV("Out-Link-Runtime-Incompatible");
    SPEC_COV("Link-Runtime-Incompatible");
    TestConfig cfg;
    cfg.linker_syms_ok = false;
    g_cfg = &cfg;
    const Project project = MakeProject("/root", "executable", std::nullopt, {"main"});
    const OutputPipelineResult result = OutputPipelineWithDeps(project, MakeDeps());
    assert(!result.artifacts.has_value());
    assert(HasError(result.diags));
    assert(HasCode(result.diags, "E-OUT-0408"));
  }

  {
    SPEC_COV("Out-Link-Runtime-Incompatible");
    SPEC_COV("Link-Runtime-Incompatible");
    TestConfig cfg;
    cfg.linker_syms = RuntimeRequiredSyms();
    if (!cfg.linker_syms.empty()) {
      cfg.linker_syms.pop_back();
    }
    g_cfg = &cfg;
    const Project project = MakeProject("/root", "executable", std::nullopt, {"main"});
    const OutputPipelineResult result = OutputPipelineWithDeps(project, MakeDeps());
    assert(!result.artifacts.has_value());
    assert(HasError(result.diags));
    assert(HasCode(result.diags, "E-OUT-0408"));
  }

  {
    SPEC_COV("Out-Link-Fail");
    SPEC_COV("Link-Fail");
    TestConfig cfg;
    cfg.linker_syms = RuntimeRequiredSyms();
    cfg.invoke_linker_ok = false;
    g_cfg = &cfg;
    const Project project = MakeProject("/root", "executable", std::nullopt, {"main"});
    const OutputPipelineResult result = OutputPipelineWithDeps(project, MakeDeps());
    assert(!result.artifacts.has_value());
    assert(HasError(result.diags));
    assert(HasCode(result.diags, "E-OUT-0404"));
  }

  {
    SPEC_COV("Output-Pipeline-Lib");
    SPEC_COV("Out-IR-None-NoLink");
    TestConfig cfg;
    cfg.invoke_linker_ok = false;
    cfg.resolve_runtime_lib_ok = false;
    cfg.linker_syms_ok = false;
    g_cfg = &cfg;
    const Project project = MakeProject("/root", "library", std::nullopt, {"main"});
    const OutputPipelineResult result = OutputPipelineWithDeps(project, MakeDeps());
    assert(result.artifacts.has_value());
    assert(!HasError(result.diags));
    assert(!result.artifacts->exe.has_value());
  }

  {
    SPEC_COV("Out-IR-Done-NoLink");
    SPEC_COV("CodegenIR-LLVM");
    TestConfig cfg;
    g_cfg = &cfg;
    const Project project =
        MakeProject("/root", "library", std::optional<std::string>("ll"), {"main"});
    const OutputPipelineResult result = OutputPipelineWithDeps(project, MakeDeps());
    assert(result.artifacts.has_value());
    assert(!HasError(result.diags));
    assert(result.artifacts->irs.size() == 1);
    assert(!result.artifacts->exe.has_value());
  }

  {
    const Project project =
        MakeProject("/root", "executable", std::optional<std::string>("ll"), {"main"});
    const auto lines = cursive0::project::DumpProject(project, false);
    assert(lines.size() == 7);
    assert(lines[0] == "project_root: /root");
    assert(lines[1] == "assemblies: [app]");
    assert(lines[2] == "assembly_name: app");
    assert(lines[3] == "source_root: /root/src");
    assert(lines[4] == "output_root: /root/build");
    assert(lines[5] == "module_list: [main]");
    const std::string expected =
        std::string("module: main obj: ") +
        cursive0::project::ObjPath(project, project.modules[0]).generic_string() +
        " ir: " +
        cursive0::project::IRPath(project, project.modules[0], "ll").generic_string();
    assert(lines[6] == expected);
  }

  return 0;
}
