#pragma once

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "cursive0/00_core/diagnostics.h"
#include "cursive0/01_project/project.h"

namespace cursive0::project {

enum class LinkStatus {
  Ok,
  NotFound,
  RuntimeMissing,
  RuntimeIncompatible,
  Fail,
};

struct LinkDeps {
  std::function<std::optional<std::filesystem::path>(const Project& project,
                                                     std::string_view tool)>
      resolve_tool;
  std::function<std::optional<std::filesystem::path>(const Project& project)>
      resolve_runtime_lib;
  std::function<std::optional<std::vector<std::string>>(
      const std::filesystem::path& tool,
      const std::vector<std::filesystem::path>& inputs,
      const std::filesystem::path& exe)>
      linker_syms;
  std::function<bool(const std::filesystem::path& tool,
                     const std::vector<std::filesystem::path>& inputs,
                     const std::filesystem::path& exe)>
      invoke_linker;
};

struct LinkResult {
  LinkStatus status = LinkStatus::Fail;
  core::DiagnosticStream diags;
};

std::filesystem::path RuntimeLibPath(const Project& project);
std::vector<std::string> RuntimeRequiredSyms();

std::optional<std::filesystem::path> ResolveRuntimeLib(const Project& project);
std::optional<std::vector<std::string>> LinkerSyms(
    const std::filesystem::path& tool,
    const std::vector<std::filesystem::path>& inputs,
    const std::filesystem::path& exe);
bool InvokeLinker(const std::filesystem::path& tool,
                  const std::vector<std::filesystem::path>& inputs,
                  const std::filesystem::path& exe);

LinkResult LinkWithDeps(const std::vector<std::filesystem::path>& objs,
                        const Project& project,
                        const LinkDeps& deps);

LinkResult Link(const std::vector<std::filesystem::path>& objs,
                const Project& project);

}  // namespace cursive0::project
