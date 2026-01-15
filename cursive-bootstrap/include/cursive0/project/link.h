#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "cursive0/core/diagnostics.h"
#include "cursive0/project/project.h"

namespace cursive0::project {

enum class LinkStatus {
  Ok,
  NotFound,
  RuntimeMissing,
  RuntimeIncompatible,
  Fail,
};

struct LinkDeps {
  std::optional<std::filesystem::path> (*resolve_tool)(const Project& project,
                                                       std::string_view tool);
  std::optional<std::filesystem::path> (*resolve_runtime_lib)(
      const Project& project);
  std::optional<std::vector<std::string>> (*linker_syms)(
      const std::filesystem::path& tool,
      const std::vector<std::filesystem::path>& inputs,
      const std::filesystem::path& exe);
  bool (*invoke_linker)(const std::filesystem::path& tool,
                        const std::vector<std::filesystem::path>& inputs,
                        const std::filesystem::path& exe);
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

LinkResult LinkWithDeps(const std::vector<std::filesystem::path>& objs,
                        const Project& project,
                        const LinkDeps& deps);

LinkResult Link(const std::vector<std::filesystem::path>& objs,
                const Project& project);

}  // namespace cursive0::project
