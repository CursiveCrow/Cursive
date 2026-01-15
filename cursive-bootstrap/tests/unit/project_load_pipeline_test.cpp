#include <cassert>
#include <filesystem>
#include <optional>
#include <string_view>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/diagnostic_messages.h"
#include "cursive0/core/diagnostics.h"
#include "cursive0/project/project.h"

namespace {

using cursive0::core::DiagnosticStream;
using cursive0::core::Emit;
using cursive0::core::HasError;
using cursive0::core::MakeDiagnostic;
using cursive0::project::LoadProjectDeps;
using cursive0::project::LoadProjectResult;
using cursive0::project::LoadProjectWithDeps;
using cursive0::project::ManifestParseResult;
using cursive0::project::ManifestValidationResult;
using cursive0::project::ModuleInfo;
using cursive0::project::ModulesResult;
using cursive0::project::OutputPaths;
using cursive0::project::TOMLTable;
using cursive0::project::ValidatedAssembly;

DiagnosticStream DiagWithCode(std::string_view code) {
  DiagnosticStream diags;
  auto diag = MakeDiagnostic(code);
  assert(diag.has_value());
  diag->span.reset();
  diags = Emit(diags, *diag);
  return diags;
}

bool HasCode(const DiagnosticStream& diags, std::string_view code) {
  for (const auto& diag : diags) {
    if (diag.code == code) {
      return true;
    }
  }
  return false;
}

ManifestParseResult ParseOk(const std::filesystem::path&) {
  ManifestParseResult result;
  result.table = TOMLTable{};
  return result;
}

ManifestParseResult ParseFailMissing(const std::filesystem::path&) {
  ManifestParseResult result;
  result.diags = DiagWithCode("E-PRJ-0101");
  return result;
}

ManifestValidationResult ValidateOk(const std::filesystem::path&,
                                   const TOMLTable&) {
  ManifestValidationResult result;
  result.assemblies = {ValidatedAssembly{"demo",
                                       "executable",
                                       "src",
                                       std::nullopt,
                                       std::nullopt}};
  return result;
}

ManifestValidationResult ValidateFail(const std::filesystem::path&,
                                     const TOMLTable&) {
  ManifestValidationResult result;
  result.diags = DiagWithCode("E-PRJ-0103");
  return result;
}

ManifestValidationResult ValidateTwo(const std::filesystem::path&,
                                     const TOMLTable&) {
  ManifestValidationResult result;
  result.assemblies = {
      ValidatedAssembly{"demo_a", "executable", "src", std::nullopt, std::nullopt},
      ValidatedAssembly{"demo_b", "executable", "src", std::nullopt, std::nullopt},
  };
  return result;
}

std::optional<cursive0::core::ResolveResult> ResolveOk(std::string_view root,
                                                       std::string_view) {
  cursive0::core::ResolveResult result;
  result.root = std::string(root);
  result.path = std::string(root) + "/src";
  return result;
}

std::optional<cursive0::core::ResolveResult> ResolveFail(std::string_view,
                                                         std::string_view) {
  return std::nullopt;
}

bool IsDirTrue(const std::filesystem::path&) {
  return true;
}

bool IsDirFalse(const std::filesystem::path&) {
  return false;
}

ModulesResult ModulesOk(const std::filesystem::path&, std::string_view) {
  ModulesResult result;
  result.modules.push_back(ModuleInfo{"b", "/root/src/b"});
  result.modules.push_back(ModuleInfo{"a", "/root/src/a"});
  return result;
}

ModulesResult ModulesErr(const std::filesystem::path&, std::string_view) {
  ModulesResult result;
  result.diags = DiagWithCode("E-PRJ-0305");
  return result;
}

OutputPaths OutputsMock(const std::filesystem::path& project_root,
                        const ValidatedAssembly& assembly) {
  return cursive0::project::ComputeOutputPaths(project_root, assembly);
}

}  // namespace

int main() {
  {
    SPEC_COV("Step-Parse-Err");
    SPEC_COV("LoadProject-Err");
    const LoadProjectDeps deps = {
        ParseFailMissing,
        ValidateOk,
        ResolveOk,
        IsDirTrue,
        ModulesOk,
        OutputsMock,
    };
    const LoadProjectResult result = LoadProjectWithDeps("/root", std::nullopt, deps);
    assert(!result.project.has_value());
    assert(HasError(result.diags));
    assert(HasCode(result.diags, "E-PRJ-0101"));
  }

  {
    SPEC_COV("Step-Parse");
    SPEC_COV("Step-Validate-Err");
    SPEC_COV("LoadProject-Err");
    const LoadProjectDeps deps = {
        ParseOk,
        ValidateFail,
        ResolveOk,
        IsDirTrue,
        ModulesOk,
        OutputsMock,
    };
    const LoadProjectResult result = LoadProjectWithDeps("/root", std::nullopt, deps);
    assert(!result.project.has_value());
    assert(HasError(result.diags));
    assert(HasCode(result.diags, "E-PRJ-0103"));
  }

  {
    SPEC_COV("Step-Validate");
    SPEC_COV("Step-Resolve-Err");
    SPEC_COV("Step-Asm-Init");
    SPEC_COV("Step-Asm-Cons");
    SPEC_COV("Step-Asm-Err");
    SPEC_COV("BuildAssembly-Err-Resolve");
    SPEC_COV("LoadProject-Err");
    const LoadProjectDeps deps = {
        ParseOk,
        ValidateOk,
        ResolveFail,
        IsDirTrue,
        ModulesOk,
        OutputsMock,
    };
    const LoadProjectResult result = LoadProjectWithDeps("/root", std::nullopt, deps);
    assert(!result.project.has_value());
    assert(HasError(result.diags));
    assert(HasCode(result.diags, "E-PRJ-0304"));
  }

  {
    SPEC_COV("Step-Resolve");
    SPEC_COV("Step-Discover-Err-Root");
    SPEC_COV("Step-Asm-Init");
    SPEC_COV("Step-Asm-Cons");
    SPEC_COV("Step-Asm-Err");
    SPEC_COV("BuildAssembly-Err-Root");
    SPEC_COV("LoadProject-Err");
    const LoadProjectDeps deps = {
        ParseOk,
        ValidateOk,
        ResolveOk,
        IsDirFalse,
        ModulesOk,
        OutputsMock,
    };
    const LoadProjectResult result = LoadProjectWithDeps("/root", std::nullopt, deps);
    assert(!result.project.has_value());
    assert(HasError(result.diags));
    assert(HasCode(result.diags, "E-PRJ-0302"));
  }

  {
    SPEC_COV("ModuleList-Err");
    SPEC_COV("Step-Discover-Err-Modules");
    SPEC_COV("Step-Asm-Init");
    SPEC_COV("Step-Asm-Cons");
    SPEC_COV("Step-Asm-Err");
    SPEC_COV("BuildAssembly-Err-Modules");
    SPEC_COV("LoadProject-Err");
    const LoadProjectDeps deps = {
        ParseOk,
        ValidateOk,
        ResolveOk,
        IsDirTrue,
        ModulesErr,
        OutputsMock,
    };
    const LoadProjectResult result = LoadProjectWithDeps("/root", std::nullopt, deps);
    assert(!result.project.has_value());
    assert(HasError(result.diags));
    assert(HasCode(result.diags, "E-PRJ-0305"));
  }

  {
    SPEC_COV("Step-Discover");
    SPEC_COV("ModuleList-Ok");
    SPEC_COV("Step-Asm-Init");
    SPEC_COV("Step-Asm-Cons");
    SPEC_COV("Step-Asm-Done");
    SPEC_COV("BuildAssembly-Ok");
    SPEC_COV("LoadProject-Ok");
    const LoadProjectDeps deps = {
        ParseOk,
        ValidateOk,
        ResolveOk,
        IsDirTrue,
        ModulesOk,
        OutputsMock,
    };
    const LoadProjectResult result = LoadProjectWithDeps("/root", std::nullopt, deps);
    assert(result.project.has_value());
    assert(result.diags.empty());
    assert(result.project->modules.size() == 2);
    assert(result.project->modules[0].path == "a");
    assert(result.project->modules[1].path == "b");
    assert(result.project->outputs.root == std::filesystem::path("/root/build"));
  }

  {
    SPEC_COV("Step-Asm-Init");
    SPEC_COV("Step-Asm-Cons");
    SPEC_COV("Step-Asm-Done-Err");
    SPEC_COV("BuildAssembly-Ok");
    SPEC_COV("LoadProject-Err");
    const LoadProjectDeps deps = {
        ParseOk,
        ValidateTwo,
        ResolveOk,
        IsDirTrue,
        ModulesOk,
        OutputsMock,
    };
    const LoadProjectResult result = LoadProjectWithDeps("/root", std::nullopt, deps);
    assert(!result.project.has_value());
    assert(HasError(result.diags));
    assert(HasCode(result.diags, "E-PRJ-0205"));
  }

  return 0;
}
