#include <cassert>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string_view>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/diagnostics.h"
#include "cursive0/project/link.h"
#include "cursive0/project/outputs.h"
#include "cursive0/project/project.h"

namespace {

using cursive0::core::DiagnosticStream;
using cursive0::core::HasError;
using cursive0::project::Assembly;
using cursive0::project::LinkDeps;
using cursive0::project::LinkResult;
using cursive0::project::LinkStatus;
using cursive0::project::LinkWithDeps;
using cursive0::project::Project;
using cursive0::project::RuntimeRequiredSyms;
using cursive0::project::RuntimeLibPath;
using cursive0::project::ValidatedAssembly;

struct LinkConfig {
  bool resolve_tool_ok = true;
  bool resolve_runtime_lib_ok = true;
  bool linker_syms_ok = true;
  bool invoke_linker_ok = true;
  std::vector<std::string> linker_syms;
};

LinkConfig* g_cfg = nullptr;

std::optional<std::filesystem::path> ResolveToolStub(const Project&,
                                                     std::string_view tool) {
  if (!g_cfg || !g_cfg->resolve_tool_ok) {
    return std::nullopt;
  }
  return std::filesystem::path("/tools") / std::string(tool);
}

std::optional<std::filesystem::path> ResolveRuntimeLibStub(const Project& project) {
  if (!g_cfg || !g_cfg->resolve_runtime_lib_ok) {
    return std::nullopt;
  }
  return RuntimeLibPath(project);
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

bool InvokeLinkerStub(const std::filesystem::path&,
                      const std::vector<std::filesystem::path>&,
                      const std::filesystem::path&) {
  return g_cfg && g_cfg->invoke_linker_ok;
}

bool HasCode(const DiagnosticStream& diags, std::string_view code) {
  for (const auto& diag : diags) {
    if (diag.code == code) {
      return true;
    }
  }
  return false;
}

Project MakeProject(const std::filesystem::path& root) {
  Project project;
  project.root = root;
  project.source_root = root / "src";
  const ValidatedAssembly spec{"app", "executable", "src", std::nullopt, std::nullopt};
  Assembly assembly;
  assembly.name = spec.name;
  assembly.kind = spec.kind;
  assembly.root = spec.root;
  assembly.out_dir = spec.out_dir;
  assembly.emit_ir = spec.emit_ir;
  assembly.source_root = project.source_root;
  assembly.outputs = cursive0::project::ComputeOutputPaths(project.root, spec);
  project.outputs = assembly.outputs;
  project.assembly = assembly;
  project.assemblies = {assembly};
  return project;
}

}  // namespace

int main() {
  {
    SPEC_COV("ResolveRuntimeLib-Ok");
    const auto tmp = std::filesystem::temp_directory_path() / "cursive0_rt_ok";
    std::error_code ec;
    std::filesystem::create_directories(tmp / "runtime", ec);
    std::ofstream out(tmp / "runtime" / "cursive0_rt.lib", std::ios::binary);
    out << "stub";
    out.close();
    const Project project = MakeProject(tmp);
    const auto path = cursive0::project::ResolveRuntimeLib(project);
    assert(path.has_value());
  }

  {
    SPEC_COV("ResolveRuntimeLib-Err");
    const auto tmp = std::filesystem::temp_directory_path() / "cursive0_rt_missing";
    std::error_code ec;
    std::filesystem::create_directories(tmp, ec);
    const Project project = MakeProject(tmp);
    const auto path = cursive0::project::ResolveRuntimeLib(project);
    assert(!path.has_value());
  }

  const LinkDeps deps = {
      ResolveToolStub,
      ResolveRuntimeLibStub,
      LinkerSymsStub,
      InvokeLinkerStub,
  };

  {
    SPEC_COV("Link-Ok");
    LinkConfig cfg;
    cfg.linker_syms = RuntimeRequiredSyms();
    g_cfg = &cfg;
    const Project project = MakeProject("/root");
    const LinkResult result = LinkWithDeps({}, project, deps);
    assert(result.status == LinkStatus::Ok);
    assert(!HasError(result.diags));
  }

  {
    SPEC_COV("Link-NotFound");
    LinkConfig cfg;
    cfg.resolve_tool_ok = false;
    g_cfg = &cfg;
    const Project project = MakeProject("/root");
    const LinkResult result = LinkWithDeps({}, project, deps);
    assert(result.status == LinkStatus::NotFound);
    assert(HasError(result.diags));
    assert(HasCode(result.diags, "E-OUT-0405"));
  }

  {
    SPEC_COV("Link-Runtime-Missing");
    LinkConfig cfg;
    cfg.resolve_runtime_lib_ok = false;
    g_cfg = &cfg;
    const Project project = MakeProject("/root");
    const LinkResult result = LinkWithDeps({}, project, deps);
    assert(result.status == LinkStatus::RuntimeMissing);
    assert(HasError(result.diags));
    assert(HasCode(result.diags, "E-OUT-0407"));
  }

  {
    SPEC_COV("Link-Runtime-Incompatible");
    LinkConfig cfg;
    cfg.linker_syms_ok = false;
    g_cfg = &cfg;
    const Project project = MakeProject("/root");
    const LinkResult result = LinkWithDeps({}, project, deps);
    assert(result.status == LinkStatus::RuntimeIncompatible);
    assert(HasError(result.diags));
    assert(HasCode(result.diags, "E-OUT-0408"));
  }

  {
    SPEC_COV("Link-Fail");
    LinkConfig cfg;
    cfg.linker_syms = RuntimeRequiredSyms();
    cfg.invoke_linker_ok = false;
    g_cfg = &cfg;
    const Project project = MakeProject("/root");
    const LinkResult result = LinkWithDeps({}, project, deps);
    assert(result.status == LinkStatus::Fail);
    assert(HasError(result.diags));
    assert(HasCode(result.diags, "E-OUT-0404"));
  }

  return 0;
}
